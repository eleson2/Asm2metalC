/*********************************************************************
 * METALC_TCPIP.H - TCP/IP Communications Server Exit definitions
 *
 * This header provides control block structures, constants, and
 * utility functions for z/OS Communications Server (TCP/IP) exits.
 *
 * Supported exits:
 *   FTP exits:
 *     FTCHKCMD - Command validation exit
 *     FTPOSTPR - Post-processing exit
 *     FTCHKPWD - Password validation exit
 *     FTCHKJES - JES interface exit
 *
 *   TN3270 exits:
 *     EZBTNCEX - TN3270 connection exit
 *     EZBTNLUX - TN3270 LU assignment exit
 *
 *   Security exits:
 *     EZACSEC  - Client security exit
 *     EZASSEC  - Server security exit
 *
 *   Stack exits:
 *     EZBIPMXT - IP filtering exit
 *     EZBSOCKT - Socket exit
 *
 * Usage:
 *   #include "metalc_base.h"
 *   #include "metalc_tcpip.h"
 *
 * IMPORTANT: TCP/IP exits may run in multiple address spaces.
 *            Ensure reentrancy and thread safety.
 *********************************************************************/

#ifndef METALC_TCPIP_H
#define METALC_TCPIP_H

#include "metalc_base.h"

/*===================================================================
 * TCP/IP Exit Return Codes
 *===================================================================*/

/* FTP exit return codes */
#define FTP_RC_CONTINUE        RC_OK     /* Continue processing           */
#define FTP_RC_REJECT          RC_WARNING  /* Reject command/request        */
#define FTP_RC_MODIFIED        RC_ERROR    /* Parameters modified           */
#define FTP_RC_TERMINATE       RC_SEVERE   /* Terminate session             */

/* TN3270 exit return codes */
#define TN3270_RC_CONTINUE     RC_OK     /* Continue processing           */
#define TN3270_RC_REJECT       RC_WARNING  /* Reject connection             */
#define TN3270_RC_REDIRECT     RC_ERROR    /* Redirect to different LU      */
#define TN3270_RC_TERMINATE    RC_SEVERE   /* Terminate connection          */

/* Security exit return codes */
#define TCPSEC_RC_ALLOW        RC_OK     /* Allow connection              */
#define TCPSEC_RC_DENY         RC_WARNING  /* Deny connection               */
#define TCPSEC_RC_AUDIT        RC_ERROR    /* Allow with audit              */

/* IP filter exit return codes */
#define IPFLT_RC_PERMIT        RC_OK     /* Permit packet                 */
#define IPFLT_RC_DENY          RC_WARNING  /* Deny packet                   */
#define IPFLT_RC_LOG           RC_ERROR    /* Permit and log                */
#define IPFLT_RC_DENYLOG       RC_SEVERE   /* Deny and log                  */

/*===================================================================
 * FTP Command Codes
 *===================================================================*/

#define FTP_CMD_USER           1     /* USER command                  */
#define FTP_CMD_PASS           2     /* PASS command                  */
#define FTP_CMD_ACCT           3     /* ACCT command                  */
#define FTP_CMD_CWD            4     /* CWD command                   */
#define FTP_CMD_CDUP           5     /* CDUP command                  */
#define FTP_CMD_SMNT           6     /* SMNT command                  */
#define FTP_CMD_QUIT           7     /* QUIT command                  */
#define FTP_CMD_REIN           8     /* REIN command                  */
#define FTP_CMD_PORT           9     /* PORT command                  */
#define FTP_CMD_PASV           10    /* PASV command                  */
#define FTP_CMD_TYPE           11    /* TYPE command                  */
#define FTP_CMD_STRU           12    /* STRU command                  */
#define FTP_CMD_MODE           13    /* MODE command                  */
#define FTP_CMD_RETR           14    /* RETR command                  */
#define FTP_CMD_STOR           15    /* STOR command                  */
#define FTP_CMD_STOU           16    /* STOU command                  */
#define FTP_CMD_APPE           17    /* APPE command                  */
#define FTP_CMD_ALLO           18    /* ALLO command                  */
#define FTP_CMD_REST           19    /* REST command                  */
#define FTP_CMD_RNFR           20    /* RNFR command                  */
#define FTP_CMD_RNTO           21    /* RNTO command                  */
#define FTP_CMD_ABOR           22    /* ABOR command                  */
#define FTP_CMD_DELE           23    /* DELE command                  */
#define FTP_CMD_RMD            24    /* RMD command                   */
#define FTP_CMD_MKD            25    /* MKD command                   */
#define FTP_CMD_PWD            26    /* PWD command                   */
#define FTP_CMD_LIST           27    /* LIST command                  */
#define FTP_CMD_NLST           28    /* NLST command                  */
#define FTP_CMD_SITE           29    /* SITE command                  */
#define FTP_CMD_SYST           30    /* SYST command                  */
#define FTP_CMD_STAT           31    /* STAT command                  */
#define FTP_CMD_HELP           32    /* HELP command                  */
#define FTP_CMD_NOOP           33    /* NOOP command                  */

/*===================================================================
 * FTP Reply Codes
 *===================================================================*/

#define FTP_REPLY_OK           200   /* Command OK                    */
#define FTP_REPLY_READY        220   /* Service ready                 */
#define FTP_REPLY_GOODBYE      221   /* Goodbye                       */
#define FTP_REPLY_TRANSFER_OK  226   /* Transfer complete             */
#define FTP_REPLY_PASV_OK      227   /* Entering passive mode         */
#define FTP_REPLY_LOGIN_OK     230   /* User logged in                */
#define FTP_REPLY_FILEOK       250   /* File action OK                */
#define FTP_REPLY_PATHNAME     257   /* Pathname created              */
#define FTP_REPLY_NEEDPASS     331   /* Need password                 */
#define FTP_REPLY_NEEDACCT     332   /* Need account                  */
#define FTP_REPLY_NOTAVAIL     421   /* Service not available         */
#define FTP_REPLY_CANTOPEN     425   /* Can't open data connection    */
#define FTP_REPLY_CONNABORT    426   /* Connection aborted            */
#define FTP_REPLY_NOTAKEN      450   /* File action not taken         */
#define FTP_REPLY_LOCALERR     451   /* Local error                   */
#define FTP_REPLY_NOSPACE      452   /* Insufficient storage          */
#define FTP_REPLY_CMDERR       500   /* Syntax error                  */
#define FTP_REPLY_PARMERR      501   /* Parameter error               */
#define FTP_REPLY_NOTIMPL      502   /* Command not implemented       */
#define FTP_REPLY_BADSEQ       503   /* Bad command sequence          */
#define FTP_REPLY_NOTLOGIN     530   /* Not logged in                 */
#define FTP_REPLY_NOACCESS     550   /* File not accessible           */
#define FTP_REPLY_BADFILE      553   /* File name not allowed         */

/*===================================================================
 * IP Address Structures
 *===================================================================*/

#pragma pack(1)

/* IPv4 address */
struct ipv4_addr {
    uint8_t        addr[4];          /* IPv4 address bytes            */
};

/* IPv6 address */
struct ipv6_addr {
    uint8_t        addr[16];         /* IPv6 address bytes            */
};

/* Socket address - IPv4 */
struct sockaddr_in {
    uint8_t        sin_len;          /* +0   Length                   */
    uint8_t        sin_family;       /* +1   Address family (AF_INET) */
    uint16_t       sin_port;         /* +2   Port number              */
    struct ipv4_addr sin_addr;       /* +4   IPv4 address             */
    char           sin_zero[8];      /* +8   Padding                  */
};                                   /* Total: 16 bytes               */

/* Socket address - IPv6 */
struct sockaddr_in6 {
    uint8_t        sin6_len;         /* +0   Length                   */
    uint8_t        sin6_family;      /* +1   Address family (AF_INET6)*/
    uint16_t       sin6_port;        /* +2   Port number              */
    uint32_t       sin6_flowinfo;    /* +4   Flow information         */
    struct ipv6_addr sin6_addr;      /* +8   IPv6 address             */
    uint32_t       sin6_scope_id;    /* +24  Scope ID                 */
};                                   /* Total: 28 bytes               */

#pragma pack()

/* Address families */
#define AF_INET        2             /* IPv4                          */
#define AF_INET6       19            /* IPv6                          */

/*===================================================================
 * FTP Command Exit Parameter List (FTCHKCMD)
 *===================================================================*/

#pragma pack(1)

struct ftp_chkcmd_parm {

    EXIT_PARM_HEADER;                /* +0   Common header            */

    char           ftpuser[8];       /* +8   User ID                  */

    struct ipv4_addr ftpclient;      /* +16  Client IP address        */

    uint16_t       ftpcport;         /* +20  Client port              */

    uint16_t       _reserved1;       /* +22  Reserved                 */

    char          *ftpcmdtxt;        /* +24  Command text pointer     */

    uint32_t       ftpcmdlen;        /* +28  Command text length      */

    char          *ftparg;           /* +32  Argument pointer         */

    uint32_t       ftparglen;        /* +36  Argument length          */

    char           ftpdsn[44];       /* +40  Dataset name             */

    char           ftppath[256];     /* +84  Unix path                */

    uint32_t       ftpreasn;         /* +340 Reason code (output)     */

    uint16_t       ftpreply;         /* +344 FTP reply code (output)  */

    uint16_t       _reserved2;       /* +346 Reserved                 */

    char          *ftpmsg;           /* +348 Reply message (output)   */

    uint32_t       ftpmsglen;         /* +352 Message length           */

};                                   /* Total: 356 bytes              */

/* FTPFUNC function codes */
#define FTP_FUNC_INIT      EXIT_FUNC_INIT      /* Initialization                */
#define FTP_FUNC_CMD       0x02      /* Command processing            */
#define FTP_FUNC_TERM      EXIT_FUNC_TERM      /* Termination                   */

/* FTPFLAGS bits */
#define FTP_FLG_SSL        0x80      /* SSL/TLS connection            */
#define FTP_FLG_ANON       0x40      /* Anonymous login               */
#define FTP_FLG_MVS        0x20      /* MVS dataset access            */
#define FTP_FLG_HFS        0x10      /* HFS/zFS access                */

#pragma pack()

/*===================================================================
 * FTP Post-Processing Exit Parameter List (FTPOSTPR)
 *===================================================================*/

#pragma pack(1)

struct ftp_postpr_parm {
    EXIT_PARM_HEADER;                /* +0   Common header            */
    char           ftpuser[8];       /* +8   User ID                  */
    struct ipv4_addr ftpclient;      /* +16  Client IP address        */
    uint16_t       ftpcport;         /* +20  Client port              */
    uint16_t       ftpresult;        /* +22  Command result           */
    char           ftpdsn[44];       /* +24  Dataset name             */
    char           ftppath[256];     /* +68  Unix path                */
    uint64_t       ftpbytes;         /* +324 Bytes transferred        */
    uint32_t       ftptime;          /* +332 Transfer time (ms)       */
    uint32_t       ftpreasn;         /* +336 Reason code (output)     */
};                                   /* Total: 340 bytes              */

/* FTPRESULT - Transfer result codes */
#define FTP_RESULT_OK        0x00    /* Successful                    */
#define FTP_RESULT_ABORT     0x01    /* Aborted                       */
#define FTP_RESULT_ERROR     0x02    /* Error                         */
#define FTP_RESULT_AUTH      0x03    /* Authorization failure         */

#pragma pack()

/*===================================================================
 * TN3270 Connection Exit Parameter List (EZBTNCEX)
 *===================================================================*/

#pragma pack(1)

struct tn3270_conn_parm {
    EXIT_PARM_HEADER;                /* +0   Common header            */
    struct ipv4_addr tnclient;       /* +8   Client IP address        */
    uint16_t       tncport;          /* +12  Client port              */
    uint16_t       tnsport;          /* +14  Server port              */
    char           tnluname[8];      /* +16  LU name                  */
    char           tnappl[8];        /* +24  Application name         */
    char           tndevtyp[8];      /* +32  Device type              */
    char           tnuserid[8];      /* +40  User ID (if SSL cert)    */
    uint32_t       tnreasn;          /* +48  Reason code (output)     */
    char           tnnewlu[8];       /* +52  New LU name (redirect)   */
    char           tnnewapp[8];      /* +60  New application (redir)  */
};                                   /* Total: 68 bytes               */

/* TNFUNC function codes */
#define TN_FUNC_CONNECT    0x01      /* Connection request            */
#define TN_FUNC_DISCONNECT 0x02      /* Disconnection                 */
#define TN_FUNC_LUASSIGN   0x03      /* LU assignment                 */

/* TNFLAGS bits */
#define TN_FLG_SSL         0x80      /* SSL/TLS connection            */
#define TN_FLG_EXPRESS     0x40      /* TN3270E protocol              */
#define TN_FLG_CERT        0x20      /* Client certificate present    */
#define TN_FLG_IPV6        0x10      /* IPv6 connection               */

#pragma pack()

/*===================================================================
 * IP Filter Exit Parameter List (EZBIPMXT)
 *===================================================================*/

#pragma pack(1)

struct ipflt_parm {
    EXIT_PARM_HEADER;                /* +0   Common header            */
    uint8_t        ipfdir;           /* +6   Direction (high reserved) */
    uint8_t        ipfflags;         /* +7   Flags (low reserved) */
    struct ipv4_addr ipfsrc;         /* +8   Source IP address        */
    struct ipv4_addr ipfdst;         /* +12  Destination IP address   */
    uint16_t       ipfsport;         /* +16  Source port              */
    uint16_t       ipfdport;         /* +18  Destination port         */
    void          *ipfpkt;           /* +20  Packet data pointer      */
    uint32_t       ipfpktln;         /* +24  Packet length            */
    char           ipfintf[16];      /* +28  Interface name           */
    uint32_t       ipfreasn;         /* +44  Reason code (output)     */
    uint32_t       ipfrule;          /* +48  Matching rule (output)   */
};                                   /* Total: 52 bytes               */

/* IPFFUNC function codes */
#define IPF_FUNC_INIT      EXIT_FUNC_INIT      /* Initialization                */
#define IPF_FUNC_FILTER    0x02      /* Filter packet                 */
#define IPF_FUNC_TERM      EXIT_FUNC_TERM      /* Termination                   */

/* IPFPROTO - Protocol codes */
#define IPF_PROTO_ICMP     1         /* ICMP                          */
#define IPF_PROTO_TCP      6         /* TCP                           */
#define IPF_PROTO_UDP      17        /* UDP                           */

/* IPFDIR - Direction codes */
#define IPF_DIR_INBOUND    0x01      /* Inbound packet                */
#define IPF_DIR_OUTBOUND   0x02      /* Outbound packet               */

#pragma pack()

/*===================================================================
 * TCP/IP Security Exit Parameter List (EZACSEC/EZASSEC)
 *===================================================================*/

#pragma pack(1)

struct tcpsec_parm {
    EXIT_PARM_HEADER;                /* +0   Common header            */
    uint8_t        secproto;         /* +6   Protocol                 */
    uint8_t        _reserved1;       /* +7   Reserved                 */
    struct ipv4_addr secclient;      /* +8   Client IP address        */
    struct ipv4_addr secserver;      /* +12  Server IP address        */
    uint16_t       seccport;         /* +16  Client port              */
    uint16_t       secsport;         /* +18  Server port              */
    char           secuser[8];       /* +20  User ID                  */
    char           secjob[8];        /* +28  Job name                 */
    char           secappl[8];       /* +36  Application name         */
    uint32_t       secreasn;         /* +44  Reason code (output)     */
};                                   /* Total: 48 bytes               */

/* SECFUNC function codes */
#define SEC_FUNC_CONNECT   0x01      /* Connection request            */
#define SEC_FUNC_BIND      0x02      /* Bind request                  */
#define SEC_FUNC_ACCEPT    0x03      /* Accept request                */
#define SEC_FUNC_SEND      0x04      /* Send request                  */
#define SEC_FUNC_RECEIVE   0x05      /* Receive request               */

/* SECFLAGS bits */
#define SEC_FLG_SERVER     0x80      /* Server-side exit              */
#define SEC_FLG_CLIENT     0x40      /* Client-side exit              */
#define SEC_FLG_SSL        0x20      /* SSL/TLS connection            */

#pragma pack()

/*===================================================================
 * TCP/IP Utility Functions
 *===================================================================*/

/**
 * tcpip_ipv4_to_str - Convert IPv4 address to string
 * @addr: IPv4 address
 * @buf: Output buffer (16 bytes minimum)
 */
static inline void tcpip_ipv4_to_str(const struct ipv4_addr *addr, char *buf) {
    int pos = 0;
    for (int i = 0; i < 4; i++) {
        uint8_t b = addr->addr[i];
        if (b >= 100) { buf[pos++] = '0' + b / 100; b %= 100; }
        if (b >= 10 || addr->addr[i] >= 100) { buf[pos++] = '0' + b / 10; b %= 10; }
        buf[pos++] = '0' + b;
        if (i < 3) buf[pos++] = '.';
    }
    buf[pos] = '\0';
}

/**
 * tcpip_match_ipv4 - Compare IPv4 addresses
 * @addr1: First address
 * @addr2: Second address
 * Returns: 1 if equal, 0 otherwise
 */
static inline int tcpip_match_ipv4(const struct ipv4_addr *addr1,
                                    const struct ipv4_addr *addr2) {
    return match_field((const char *)addr1->addr, (const char *)addr2->addr, 4);
}

/**
 * tcpip_ipv4_in_subnet - Check if IPv4 address is in subnet
 * @addr: Address to check
 * @network: Network address
 * @mask: Subnet mask
 * Returns: 1 if in subnet, 0 otherwise
 */
static inline int tcpip_ipv4_in_subnet(const struct ipv4_addr *addr,
                                        const struct ipv4_addr *network,
                                        const struct ipv4_addr *mask) {
    for (int i = 0; i < 4; i++) {
        if ((addr->addr[i] & mask->addr[i]) !=
            (network->addr[i] & mask->addr[i])) {
            return 0;
        }
    }
    return 1;
}

/**
 * tcpip_is_private_ipv4 - Check if IPv4 address is private (RFC 1918)
 * @addr: Address to check
 * Returns: 1 if private, 0 otherwise
 */
static inline int tcpip_is_private_ipv4(const struct ipv4_addr *addr) {
    /* 10.0.0.0/8 */
    if (addr->addr[0] == 10) return 1;
    /* 172.16.0.0/12 */
    if (addr->addr[0] == 172 && (addr->addr[1] & 0xF0) == 16) return 1;
    /* 192.168.0.0/16 */
    if (addr->addr[0] == 192 && addr->addr[1] == 168) return 1;
    return 0;
}

/**
 * tcpip_is_ftp_data_cmd - Check if FTP command is a data transfer command
 * @cmd: FTP command code
 * Returns: 1 if data command, 0 otherwise
 */
static inline int tcpip_is_ftp_data_cmd(uint16_t cmd) {
    return cmd == FTP_CMD_RETR || cmd == FTP_CMD_STOR ||
           cmd == FTP_CMD_STOU || cmd == FTP_CMD_APPE ||
           cmd == FTP_CMD_LIST || cmd == FTP_CMD_NLST;
}

/**
 * tcpip_ftp_cmd_name - Get FTP command name
 * @cmd: FTP command code
 * Returns: Static string name
 */
static inline const char *tcpip_ftp_cmd_name(uint16_t cmd) {
    switch (cmd) {
        case FTP_CMD_USER: return "USER";
        case FTP_CMD_PASS: return "PASS";
        case FTP_CMD_CWD:  return "CWD";
        case FTP_CMD_RETR: return "RETR";
        case FTP_CMD_STOR: return "STOR";
        case FTP_CMD_DELE: return "DELE";
        case FTP_CMD_MKD:  return "MKD";
        case FTP_CMD_RMD:  return "RMD";
        case FTP_CMD_LIST: return "LIST";
        case FTP_CMD_QUIT: return "QUIT";
        default:           return "OTHER";
    }
}

/*===================================================================
 * Example Usage
 *===================================================================*

// FTP command exit - control file access
#pragma prolog(my_ftchkcmd,"SAVE(14,12),LR(12,15)")
#pragma epilog(my_ftchkcmd,"RETURN(14,12)")

int my_ftchkcmd(struct ftp_chkcmd_parm *parm) {
    if (parm->ftpfunc != FTP_FUNC_CMD) {
        return FTP_RC_CONTINUE;
    }

    // Block DELETE commands for certain datasets
    if (parm->ftpcmd == FTP_CMD_DELE) {
        if (memcmp_inline(parm->ftpdsn, "SYS1.", 5) == 0) {
            parm->ftpreply = FTP_REPLY_NOACCESS;
            return FTP_RC_REJECT;
        }
    }

    // Log all data transfer commands
    if (tcpip_is_ftp_data_cmd(parm->ftpcmd)) {
        // log_ftp_transfer(parm->ftpuser, parm->ftpcmd, parm->ftpdsn);
    }

    return FTP_RC_CONTINUE;
}

// FTP post-processing exit - audit logging
#pragma prolog(my_ftpostpr,"SAVE(14,12),LR(12,15)")
#pragma epilog(my_ftpostpr,"RETURN(14,12)")

int my_ftpostpr(struct ftp_postpr_parm *parm) {
    // Log all successful transfers
    if (parm->ftpresult == FTP_RESULT_OK) {
        if (tcpip_is_ftp_data_cmd(parm->ftpcmd)) {
            // audit_ftp_transfer(parm->ftpuser, parm->ftpdsn,
            //                    parm->ftpbytes, parm->ftptime);
        }
    }

    return FTP_RC_CONTINUE;
}

// TN3270 connection exit - access control
#pragma prolog(my_ezbtncex,"SAVE(14,12),LR(12,15)")
#pragma epilog(my_ezbtncex,"RETURN(14,12)")

int my_ezbtncex(struct tn3270_conn_parm *parm) {
    if (parm->tnfunc == TN_FUNC_CONNECT) {
        // Block connections from external networks
        if (!tcpip_is_private_ipv4(&parm->tnclient)) {
            parm->tnreasn = 100;
            return TN3270_RC_REJECT;
        }

        // Log connection attempt
        // log_tn3270_connect(&parm->tnclient, parm->tnluname);
    }

    return TN3270_RC_CONTINUE;
}

// IP filter exit - custom packet filtering
#pragma prolog(my_ezbipmxt,"SAVE(14,12),LR(12,15)")
#pragma epilog(my_ezbipmxt,"RETURN(14,12)")

int my_ezbipmxt(struct ipflt_parm *parm) {
    if (parm->ipffunc != IPF_FUNC_FILTER) {
        return IPFLT_RC_PERMIT;
    }

    // Block inbound connections to sensitive ports
    if (parm->ipfdir == IPF_DIR_INBOUND) {
        if (parm->ipfdport == 23 || parm->ipfdport == 21) {
            // Only allow from internal network
            if (!tcpip_is_private_ipv4(&parm->ipfsrc)) {
                return IPFLT_RC_DENYLOG;
            }
        }
    }

    return IPFLT_RC_PERMIT;
}

 *===================================================================*/

#endif /* METALC_TCPIP_H */
