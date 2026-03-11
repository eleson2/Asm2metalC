/*********************************************************************
 * NetView Command Exit Example (DSIEX01)
 *
 * This exit intercepts NetView commands, enforcing authorization
 * and logging command activity.
 *
 * Exit Point: DSIEX01 - Command Pre-Processing Exit
 *
 * Build: Use IBM Metal C compiler with no LE dependencies
 *   xlc -qmetal -S netview_command_exit.c
 *   as -o netview_command_exit.o netview_command_exit.s
 *   ld -o DSIEX01 netview_command_exit.o
 *
 * Installation: Define in DSIPARM DSIEX01 member
 *********************************************************************/

#include "metalc_base.h"
#include "metalc_netview.h"

/*-------------------------------------------------------------------
 * Configuration - Customize for your installation
 *-------------------------------------------------------------------*/

/* Commands requiring authorization */
typedef struct {
    char cmd_prefix[12];      /* Command prefix */
    int  prefix_len;          /* Prefix length */
    int  admin_only;          /* Requires admin operator */
} controlled_cmd_t;

static const controlled_cmd_t controlled_cmds[] = {
    { "VARY",      4, 1 },    /* VARY commands - admin only */
    { "FORCE",     5, 1 },    /* FORCE commands - admin only */
    { "CANCEL",    6, 1 },    /* CANCEL commands - admin only */
    { "INACT",     5, 1 },    /* Inactivate - admin only */
    { "ACT",       3, 0 },    /* Activate - logged but allowed */
    { "RESET",     5, 1 },    /* Reset - admin only */
    { "CONFIG",    6, 1 },    /* Configuration - admin only */
    { "",          0, 0 }     /* End marker */
};

/* Admin operators */
static const char admin_operators[][8] = {
    "MASTER  ",
    "NETADMIN",
    "OPERSUP ",
    ""           /* End marker */
};

/* Blocked commands */
static const char blocked_cmds[][12] = {
    "HALT",         /* Never allow HALT */
    "SHUTDOWN",     /* Never allow SHUTDOWN */
    "RECYCLE",      /* Never allow RECYCLE */
    ""              /* End marker */
};

/* Custom reason codes for this exit */
#define NVX_RSN_COMMAND_BLOCKED    5001
#define NVX_RSN_NOT_AUTHORIZED     5002
#define NVX_RSN_AUDIT_REQUIRED     5003

/*-------------------------------------------------------------------
 * is_admin_operator - Check if operator has admin authority
 * Note: In production, use nv_is_authorized() or RACF
 *-------------------------------------------------------------------*/
static int is_admin_operator(const char oper[8]) {
    for (int i = 0; admin_operators[i][0] != '\0'; i++) {
        if (nv_match_operator(oper, admin_operators[i])) {
            return 1;
        }
    }
    return 0;
}

/*-------------------------------------------------------------------
 * is_blocked_command - Check if command is blocked
 *-------------------------------------------------------------------*/
static int is_blocked_command(const char *cmd, uint32_t len) {
    for (int i = 0; blocked_cmds[i][0] != '\0'; i++) {
        int clen = 0;
        while (clen < 12 && blocked_cmds[i][clen] != '\0') clen++;

        if (len >= (uint32_t)clen) {
            if (memcmp_inline(cmd, blocked_cmds[i], clen) == 0) {
                /* Check for word boundary */
                if (len == (uint32_t)clen || cmd[clen] == ' ') {
                    return 1;
                }
            }
        }
    }
    return 0;
}

/*-------------------------------------------------------------------
 * find_controlled_cmd - Find command in controlled list
 *-------------------------------------------------------------------*/
static const controlled_cmd_t *find_controlled_cmd(const char *cmd, uint32_t len) {
    for (int i = 0; controlled_cmds[i].prefix_len > 0; i++) {
        int plen = controlled_cmds[i].prefix_len;
        if (len >= (uint32_t)plen) {
            if (memcmp_inline(cmd, controlled_cmds[i].cmd_prefix, plen) == 0) {
                /* Check for word boundary */
                if (len == (uint32_t)plen || cmd[plen] == ' ') {
                    return &controlled_cmds[i];
                }
            }
        }
    }
    return NULL;
}

/*-------------------------------------------------------------------
 * copy_field - Copy fixed-length field, stop at blank/null
 * Returns number of characters copied
 *-------------------------------------------------------------------*/
static int copy_field(char *dest, const char *src, int maxlen) {
    int i;
    for (i = 0; i < maxlen; i++) {
        if (src[i] == ' ' || src[i] == '\0') break;
        dest[i] = src[i];
    }
    return i;
}

/*-------------------------------------------------------------------
 * log_command - Log command activity via WTO
 *
 * Issues a WTO message for command auditing. Security-related
 * events (BLOCKED, DENIED) are routed to security console.
 *
 * Message format:
 *   NVX001I CMD status OPER=operid CMD=command_text
 *   NVX002W CMD status OPER=operid CMD=command_text (security)
 *-------------------------------------------------------------------*/
static void log_command(const char *status,
                       const char oper[8],
                       const char *cmd,
                       uint32_t len) {
    char msg[120];
    int pos = 0;
    int is_security_event;
    uint16_t route;
    uint16_t desc;

    /* Determine if this is a security event */
    is_security_event = (memcmp_inline(status, "BLOCKED", 7) == 0 ||
                         memcmp_inline(status, "DENIED", 6) == 0);

    /* Build message ID */
    if (is_security_event) {
        memcpy_inline(&msg[pos], "NVX002W CMD ", 12);
        route = WTO_ROUTE_SYSTEM_SECURITY | WTO_ROUTE_MASTER_CONSOLE;
        desc = WTO_DESC_SYSTEM_STATUS;
    } else {
        memcpy_inline(&msg[pos], "NVX001I CMD ", 12);
        route = WTO_ROUTE_PROGRAMMER_INFO;
        desc = WTO_DESC_APPLICATION;
    }
    pos += 12;

    /* Add status */
    int slen = strlen_inline(status);
    memcpy_inline(&msg[pos], status, slen);
    pos += slen;

    /* Add " OPER=" */
    memcpy_inline(&msg[pos], " OPER=", 6);
    pos += 6;

    /* Operator ID */
    pos += copy_field(&msg[pos], oper, 8);

    /* Add " CMD=" */
    memcpy_inline(&msg[pos], " CMD=", 5);
    pos += 5;

    /* Command text (truncate if too long for message) */
    int cmd_space = 120 - pos - 1;  /* Leave room for message */
    if ((int)len > cmd_space) len = cmd_space;
    memcpy_inline(&msg[pos], cmd, len);
    pos += len;

    /* Issue the WTO */
    wto_write(msg, pos, route, desc);
}

/*===================================================================
 * DSIEX01 - NetView Command Pre-Processing Exit
 *
 * Called before NetView processes a command.
 *
 * Input:
 *   parm - Pointer to nv_cmd_parm structure (from metalc_netview.h):
 *          - nvoper:   Operator ID
 *          - nvcmd:    Command text pointer
 *          - nvcmdlen: Command length
 *          - nvflags:  Processing flags
 *          - nvcmdtype: Command type
 *
 * Return:
 *   NV_CMD_CONTINUE (0)  - Continue with command
 *   NV_CMD_SUPPRESS (4)  - Suppress command silently
 *   NV_CMD_MODIFIED (8)  - Command was modified
 *   NV_CMD_ROUTE    (12) - Route to different operator
 *===================================================================*/

#pragma prolog(DSIEX01, "SAVE(14,12),LR(12,15)")
#pragma epilog(DSIEX01, "RETURN(14,12)")

int DSIEX01(struct nv_cmd_parm *parm) {
    const controlled_cmd_t *ctrl_cmd;

    /* Validate input */
    if (parm == NULL || parm->nvcmd == NULL) {
        return NV_CMD_CONTINUE;
    }

    /* Handle different function codes */
    if (parm->nvfunc != NV_FUNC_PRE) {
        /* Post/Immed/Init/Term - just continue */
        return NV_CMD_CONTINUE;
    }

    /* Check if command is blocked */
    if (is_blocked_command(parm->nvcmd, parm->nvcmdlen)) {
        parm->nvreasn = NVX_RSN_COMMAND_BLOCKED;
        log_command("BLOCKED", parm->nvoper, parm->nvcmd, parm->nvcmdlen);
        return NV_CMD_SUPPRESS;
    }

    /* Check if command requires authorization */
    ctrl_cmd = find_controlled_cmd(parm->nvcmd, parm->nvcmdlen);

    if (ctrl_cmd != NULL) {
        /* Command is controlled */
        if (ctrl_cmd->admin_only) {
            /* Check if operator is authorized via flags or admin list */
            if (!nv_is_authorized(parm->nvflags) &&
                !is_admin_operator(parm->nvoper)) {
                parm->nvreasn = NVX_RSN_NOT_AUTHORIZED;
                log_command("DENIED", parm->nvoper, parm->nvcmd, parm->nvcmdlen);
                return NV_CMD_SUPPRESS;
            }
        }

        /* Log controlled command execution */
        log_command("ALLOWED", parm->nvoper, parm->nvcmd, parm->nvcmdlen);
    }

    return NV_CMD_CONTINUE;
}
