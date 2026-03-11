/*********************************************************************
 * TCP/IP FTP Command Exit Example (FTCHKCMD)
 *
 * This exit validates FTP commands, enforcing access restrictions
 * based on user, client IP, and command type.
 *
 * Exit Point: FTCHKCMD - FTP Command Validation Exit
 *
 * Build: Use IBM Metal C compiler with no LE dependencies
 *   xlc -qmetal -S tcpip_ftp_exit.c
 *   as -o tcpip_ftp_exit.o tcpip_ftp_exit.s
 *   ld -o FTCHKCMD tcpip_ftp_exit.o
 *
 * Installation: Configure in FTP.DATA
 *********************************************************************/

#include "metalc_base.h"
#include "metalc_tcpip.h"

/*-------------------------------------------------------------------
 * Configuration - Customize for your installation
 *-------------------------------------------------------------------*/

/* Blocked FTP commands (restrict for security) */
static const uint16_t blocked_commands[] = {
    FTP_CMD_SITE,    /* SITE commands - often dangerous */
    FTP_CMD_RMD,     /* Remove directory - restrict */
    0                /* End marker */
};

/* Commands requiring special authorization */
static const uint16_t elevated_commands[] = {
    FTP_CMD_DELE,    /* Delete files */
    FTP_CMD_MKD,     /* Make directory */
    FTP_CMD_RNFR,    /* Rename from */
    FTP_CMD_RNTO,    /* Rename to */
    FTP_CMD_STOR,    /* Store (upload) */
    FTP_CMD_APPE,    /* Append */
    0                /* End marker */
};

/* Users with elevated privileges */
static const char elevated_users[][8] = {
    "FTPADMIN",
    "BATCHFTP",
    "PRODUSER",
    ""           /* End marker */
};

/* Blocked IP address ranges (first 3 octets) */
typedef struct {
    uint8_t network[3];   /* First 3 octets of network */
} blocked_network_t;

static const blocked_network_t blocked_networks[] = {
    { { 10, 99, 99 } },    /* Test network */
    { { 192, 168, 99 } },  /* Lab network */
    { { 0, 0, 0 } }        /* End marker */
};

/* Dataset HLQ patterns to block from FTP */
static const char blocked_hlq[][8] = {
    "SYS1",       /* System datasets */
    "RACF",       /* Security datasets */
    "CICS",       /* CICS datasets */
    ""            /* End marker */
};

/* Custom reason codes for this exit */
#define FTPX_RSN_COMMAND_BLOCKED     4001
#define FTPX_RSN_IP_BLOCKED          4002
#define FTPX_RSN_NOT_AUTHORIZED      4003
#define FTPX_RSN_DATASET_BLOCKED     4004

/*-------------------------------------------------------------------
 * is_command_blocked - Check if command is blocked
 *-------------------------------------------------------------------*/
static int is_command_blocked(uint16_t cmd) {
    for (int i = 0; blocked_commands[i] != 0; i++) {
        if (cmd == blocked_commands[i]) {
            return 1;
        }
    }
    return 0;
}

/*-------------------------------------------------------------------
 * is_elevated_command - Check if command requires elevation
 *-------------------------------------------------------------------*/
static int is_elevated_command(uint16_t cmd) {
    for (int i = 0; elevated_commands[i] != 0; i++) {
        if (cmd == elevated_commands[i]) {
            return 1;
        }
    }
    return 0;
}

/*-------------------------------------------------------------------
 * is_elevated_user - Check if user has elevated privileges
 *-------------------------------------------------------------------*/
static int is_elevated_user(const char user[8]) {
    for (int i = 0; elevated_users[i][0] != '\0'; i++) {
        if (memcmp_inline(user, elevated_users[i], 8) == 0) {
            return 1;
        }
    }
    return 0;
}

/*-------------------------------------------------------------------
 * is_blocked_ip - Check if client IP is from blocked network
 *-------------------------------------------------------------------*/
static int is_blocked_ip(const struct ipv4_addr *addr) {
    for (int i = 0; blocked_networks[i].network[0] != 0 ||
                    blocked_networks[i].network[1] != 0 ||
                    blocked_networks[i].network[2] != 0; i++) {
        if (addr->addr[0] == blocked_networks[i].network[0] &&
            addr->addr[1] == blocked_networks[i].network[1] &&
            addr->addr[2] == blocked_networks[i].network[2]) {
            return 1;
        }
    }
    return 0;
}

/*-------------------------------------------------------------------
 * is_blocked_dataset - Check if dataset HLQ is blocked
 *-------------------------------------------------------------------*/
static int is_blocked_dataset(const char dsn[44]) {
    for (int i = 0; blocked_hlq[i][0] != '\0'; i++) {
        int hlen = 0;
        while (hlen < 8 && blocked_hlq[i][hlen] != '\0') hlen++;

        /* Check if DSN starts with blocked HLQ */
        if (memcmp_inline(dsn, blocked_hlq[i], hlen) == 0) {
            /* Verify it's the HLQ (followed by . or end) */
            if (dsn[hlen] == '.' || dsn[hlen] == ' ') {
                return 1;
            }
        }
    }
    return 0;
}

/*-------------------------------------------------------------------
 * get_command_name - Get FTP command name for logging
 *-------------------------------------------------------------------*/
static const char *get_command_name(uint16_t cmd) {
    switch (cmd) {
        case FTP_CMD_USER: return "USER";
        case FTP_CMD_PASS: return "PASS";
        case FTP_CMD_RETR: return "RETR";
        case FTP_CMD_STOR: return "STOR";
        case FTP_CMD_DELE: return "DELE";
        case FTP_CMD_MKD:  return "MKD";
        case FTP_CMD_RMD:  return "RMD";
        case FTP_CMD_LIST: return "LIST";
        case FTP_CMD_SITE: return "SITE";
        default:           return "UNKN";
    }
}

/*-------------------------------------------------------------------
 * log_ftp_activity - Log FTP activity
 *-------------------------------------------------------------------*/
static void log_ftp_activity(const char *action,
                            const struct ftp_chkcmd_parm *parm) {
    /*
     * Production implementation would:
     * 1. Write to SMF type 119 record
     * 2. Write to security log
     * 3. Issue WTO for security events
     *
     * Log format:
     * FTP action user IP:port command dataset/path
     */
    (void)action;
    (void)parm;
}

/*===================================================================
 * FTCHKCMD - FTP Command Validation Exit
 *
 * Called before each FTP command is processed.
 *
 * Input:
 *   parm - Pointer to FTP exit parameter list containing:
 *          - ftpcmd:    FTP command code
 *          - ftpuser:   User ID
 *          - ftpclient: Client IP address
 *          - ftpdsn:    Dataset name (if applicable)
 *          - ftppath:   Unix path (if applicable)
 *
 * Return:
 *   FTP_RC_CONTINUE  (0)  - Continue with command
 *   FTP_RC_REJECT    (4)  - Reject command
 *   FTP_RC_MODIFIED  (8)  - Parameters modified
 *   FTP_RC_TERMINATE (12) - Terminate session
 *===================================================================*/

#pragma prolog(FTCHKCMD, "SAVE(14,12),LR(12,15)")
#pragma epilog(FTCHKCMD, "RETURN(14,12)")

int FTCHKCMD(struct ftp_chkcmd_parm *parm) {
    /* Validate input */
    if (parm == NULL) {
        return FTP_RC_CONTINUE;
    }

    /* Skip validation for USER/PASS commands (handled separately) */
    if (parm->ftpcmd == FTP_CMD_USER || parm->ftpcmd == FTP_CMD_PASS) {
        return FTP_RC_CONTINUE;
    }

    /* Check if client IP is blocked */
    if (is_blocked_ip(&parm->ftpclient)) {
        parm->ftpreasn = FTPX_RSN_IP_BLOCKED;
        parm->ftpreply = FTP_REPLY_NOACCESS;
        log_ftp_activity("BLOCKED-IP", parm);
        return FTP_RC_TERMINATE;  /* Terminate session from blocked IP */
    }

    /* Check if command is globally blocked */
    if (is_command_blocked(parm->ftpcmd)) {
        parm->ftpreasn = FTPX_RSN_COMMAND_BLOCKED;
        parm->ftpreply = FTP_REPLY_NOTIMPL;
        log_ftp_activity("BLOCKED-CMD", parm);
        return FTP_RC_REJECT;
    }

    /* Check if command requires elevated privileges */
    if (is_elevated_command(parm->ftpcmd)) {
        if (!is_elevated_user(parm->ftpuser)) {
            parm->ftpreasn = FTPX_RSN_NOT_AUTHORIZED;
            parm->ftpreply = FTP_REPLY_NOACCESS;
            log_ftp_activity("DENIED-AUTH", parm);
            return FTP_RC_REJECT;
        }
    }

    /* Check dataset access for MVS commands */
    if (parm->ftpdsn[0] != ' ' && parm->ftpdsn[0] != '\0') {
        if (is_blocked_dataset(parm->ftpdsn)) {
            parm->ftpreasn = FTPX_RSN_DATASET_BLOCKED;
            parm->ftpreply = FTP_REPLY_NOACCESS;
            log_ftp_activity("BLOCKED-DSN", parm);
            return FTP_RC_REJECT;
        }
    }

    /* Log successful commands for audit trail */
    if (is_elevated_command(parm->ftpcmd)) {
        log_ftp_activity("ALLOWED", parm);
    }

    return FTP_RC_CONTINUE;
}

/*===================================================================
 * Additional Exit: FTP Post-Processing (FTPOSTPR)
 *
 * Called after FTP command completes for audit logging.
 *===================================================================*/

#pragma prolog(FTPOSTPR, "SAVE(14,12),LR(12,15)")
#pragma epilog(FTPOSTPR, "RETURN(14,12)")

int FTPOSTPR(struct ftp_chkcmd_parm *parm) {
    /* Log completed file transfers */
    if (parm->ftpcmd == FTP_CMD_RETR || parm->ftpcmd == FTP_CMD_STOR) {
        log_ftp_activity("COMPLETE", parm);
    }

    /* Always continue after command completion */
    return FTP_RC_CONTINUE;
}
