/*********************************************************************
 * METALC_NETVIEW.H - NetView Exit definitions for Metal C
 *
 * This header provides control block structures, constants, and
 * utility functions for IBM NetView for z/OS exit development.
 *
 * Supported exits:
 *   Command exits:
 *     DSIEX01  - Command preprocessing exit
 *     DSIEX02  - Command postprocessing exit
 *     DSIEX02A - Immediate command exit
 *     DSIEX16  - Unsolicited message exit
 *
 *   Automation exits:
 *     DSIEX03  - Automated operations exit
 *     DSIEX04  - Message revision exit
 *     DSIEX17  - Automation table exit
 *
 *   Session exits:
 *     DSIEX05  - Session monitor exit
 *     DSIEX06  - Operator logon exit
 *     DSIEX07  - Operator logoff exit
 *
 *   Alert exits:
 *     DSIEX08  - Alert processing exit
 *     DSIEX09  - Alert forwarding exit
 *
 * Usage:
 *   #include "metalc_base.h"
 *   #include "metalc_netview.h"
 *
 * IMPORTANT: NetView exits run in the NetView address space.
 *            Follow NetView exit programming guidelines.
 *********************************************************************/

#ifndef METALC_NETVIEW_H
#define METALC_NETVIEW_H

#include "metalc_base.h"

/*===================================================================
 * NetView Exit Return Codes
 *===================================================================*/

/* General exit return codes */
#define NV_RC_CONTINUE         RC_OK     /* Continue processing           */
#define NV_RC_REJECT           RC_WARNING  /* Reject/suppress               */
#define NV_RC_MODIFIED         RC_ERROR    /* Data was modified             */
#define NV_RC_ERROR            RC_SEVERE   /* Error occurred                */

/* Command exit (DSIEX01/02) return codes */
#define NV_CMD_CONTINUE        RC_OK     /* Continue command processing   */
#define NV_CMD_SUPPRESS        RC_WARNING  /* Suppress command              */
#define NV_CMD_MODIFIED        RC_ERROR    /* Command was modified          */
#define NV_CMD_ROUTE           RC_SEVERE   /* Route to different operator   */

/* Automation exit (DSIEX03) return codes */
#define NV_AUTO_CONTINUE       RC_OK     /* Continue automation           */
#define NV_AUTO_SUPPRESS       RC_WARNING  /* Suppress automation           */
#define NV_AUTO_DONE           RC_ERROR    /* Exit handled the message      */

/* Message exit (DSIEX16) return codes */
#define NV_MSG_CONTINUE        RC_OK     /* Continue message processing   */
#define NV_MSG_SUPPRESS        RC_WARNING  /* Suppress message              */
#define NV_MSG_MODIFIED        RC_ERROR    /* Message was modified          */
#define NV_MSG_DELETE          RC_SEVERE   /* Delete message                */

/* Logon exit (DSIEX06) return codes */
#define NV_LOGON_ALLOW         RC_OK     /* Allow logon                   */
#define NV_LOGON_REJECT        RC_WARNING  /* Reject logon                  */
#define NV_LOGON_DEFER         RC_ERROR    /* Defer to security system      */

/* Alert exit (DSIEX08) return codes */
#define NV_ALERT_CONTINUE      RC_OK     /* Continue alert processing     */
#define NV_ALERT_SUPPRESS      RC_WARNING  /* Suppress alert                */
#define NV_ALERT_MODIFIED      RC_ERROR    /* Alert was modified            */

/*===================================================================
 * NetView Message Types
 *===================================================================*/

#define NV_MSGTYPE_WTO         0x01  /* WTO message                   */
#define NV_MSGTYPE_WTOR        0x02  /* WTOR message                  */
#define NV_MSGTYPE_HARDCOPY    0x04  /* Hardcopy only                 */
#define NV_MSGTYPE_MLWTO       0x08  /* Multi-line WTO                */
#define NV_MSGTYPE_NETWORK     0x10  /* Network message               */
#define NV_MSGTYPE_INTERNAL    0x20  /* NetView internal message      */
#define NV_MSGTYPE_AUTOMATION  0x40  /* Automation message            */

/*===================================================================
 * NetView Command Types
 *===================================================================*/

#define NV_CMDTYPE_NETVIEW     0x01  /* NetView command               */
#define NV_CMDTYPE_MVS         0x02  /* MVS command                   */
#define NV_CMDTYPE_VTAM        0x04  /* VTAM command                  */
#define NV_CMDTYPE_JES         0x08  /* JES command                   */
#define NV_CMDTYPE_CLIST       0x10  /* CLIST/REXX command            */
#define NV_CMDTYPE_OTHER       0x80  /* Other command                 */

/*===================================================================
 * NetView Alert Severity
 *===================================================================*/

#define NV_SEV_UNKNOWN         0x00  /* Unknown                       */
#define NV_SEV_CRITICAL        0x01  /* Critical                      */
#define NV_SEV_MAJOR           0x02  /* Major                         */
#define NV_SEV_MINOR           0x03  /* Minor                         */
#define NV_SEV_WARNING         0x04  /* Warning                       */
#define NV_SEV_INFO            0x05  /* Informational                 */

/*===================================================================
 * Command Exit Parameter List (DSIEX01/DSIEX02)
 *===================================================================*/

#pragma pack(1)

struct nv_cmd_parm {
    EXIT_PARM_HEADER;                /* +0   Common header            */
    char           nvoper[8];        /* +8   Operator ID              */
    char           nvdomain[8];      /* +16  Domain ID                */
    char          *nvcmd;            /* +24  Command text pointer     */
    uint32_t       nvcmdlen;         /* +28  Command length           */
    char          *nvnewcmd;         /* +32  New command (output)     */
    uint32_t       nvnewlen;         /* +36  New command length       */
    char           nvtarget[8];      /* +40  Target operator (route)  */
    uint32_t       nvreasn;          /* +48  Reason code              */
    char          *nvmsg;            /* +52  Message area             */
    uint16_t       nvmsglen;         /* +56  Message length           */
    uint16_t       _reserved;        /* +58  Reserved                 */
};                                   /* Total: 60 bytes               */

/* NVFUNC function codes */
#define NV_FUNC_PRE        0x01      /* Pre-processing                */
#define NV_FUNC_POST       0x02      /* Post-processing               */
#define NV_FUNC_IMMED      0x03      /* Immediate command             */
#define NV_FUNC_INIT       0x04      /* Initialization                */
#define NV_FUNC_TERM       0x05      /* Termination                   */

/* NVFLAGS bits */
#define NV_FLG_AUTH        0x8000    /* Authorized operator           */
#define NV_FLG_CONSOLE     0x4000    /* Console operator              */
#define NV_FLG_REMOTE      0x2000    /* Remote operator               */
#define NV_FLG_AUTOMATION  0x1000    /* Automation task               */

#pragma pack()

/*===================================================================
 * Message Exit Parameter List (DSIEX16)
 *===================================================================*/

#pragma pack(1)

struct nv_msg_parm {
    EXIT_PARM_HEADER;                /* +0   Common header            */
    char           nvmsgid[10];      /* +8   Message ID               */
    char           _reserved1[2];    /* +18  Reserved                 */
    char          *nvmsgtext;        /* +20  Message text pointer     */
    uint32_t       nvmsgtlen;        /* +24  Message text length      */
    char           nvjobname[8];     /* +28  Job name                 */
    char           nvsysname[8];     /* +36  System name              */
    uint32_t       nvmsgtime;        /* +44  Message time             */
    uint32_t       nvmsgdate;        /* +48  Message date             */
    char          *nvnewmsg;         /* +52  New message (output)     */
    uint32_t       nvnewlen;         /* +56  New message length       */
    uint8_t        nvroute[16];      /* +60  Routing codes            */
    uint8_t        nvdesc[2];        /* +76  Descriptor codes         */
    uint16_t       _reserved2;       /* +78  Reserved                 */
    uint32_t       nvreasn;          /* +80  Reason code              */
};                                   /* Total: 84 bytes               */

/* NVFLAGS bits for message exit */
#define NV_MFLG_REPLY      0x8000    /* Reply to WTOR                 */
#define NV_MFLG_SUPPRESS   0x4000    /* Suppress display              */
#define NV_MFLG_RETAINED   0x2000    /* Retained message              */
#define NV_MFLG_AUTOMATION 0x1000    /* For automation                */

#pragma pack()

/*===================================================================
 * Automation Exit Parameter List (DSIEX03)
 *===================================================================*/

#pragma pack(1)

struct nv_auto_parm {
    EXIT_PARM_HEADER;                /* +0   Common header            */
    char           nvmsgid[10];      /* +8   Message ID               */
    char           _reserved1[2];    /* +18  Reserved                 */
    char          *nvmsgtext;        /* +20  Message text pointer     */
    uint32_t       nvmsgtlen;        /* +24  Message text length      */
    char           nvtable[8];       /* +28  Automation table name    */
    char           nvstmt[8];        /* +36  Statement name           */
    char          *nvcmd;            /* +44  Command to execute       */
    uint32_t       nvcmdlen;         /* +48  Command length           */
    char          *nvnewcmd;         /* +52  New command (output)     */
    uint32_t       nvnewlen;         /* +56  New command length       */
    uint32_t       nvreasn;          /* +60  Reason code              */
};                                   /* Total: 64 bytes               */

/* NVACTION - Automation action codes */
#define NV_ACT_CMD         0x01      /* Execute command               */
#define NV_ACT_DISPLAY     0x02      /* Display message               */
#define NV_ACT_SUPPRESS    0x03      /* Suppress message              */
#define NV_ACT_BEEP        0x04      /* Beep operator                 */
#define NV_ACT_HIGHLIGHT   0x05      /* Highlight message             */

#pragma pack()

/*===================================================================
 * Logon Exit Parameter List (DSIEX06)
 *===================================================================*/

#pragma pack(1)

struct nv_logon_parm {
    EXIT_PARM_HEADER;                /* +0   Common header            */
    char           nvoper[8];        /* +8   Operator ID              */
    char           nvpass[8];        /* +16  Password (masked)        */
    char           nvnewpass[8];     /* +24  New password (masked)    */
    char           nvdomain[8];      /* +32  Domain ID                */
    char           nvlterm[8];       /* +40  Logical terminal         */
    char           nvpterm[8];       /* +48  Physical terminal        */
    char           nvprofile[8];     /* +56  Operator profile         */
    uint32_t       nvtime;           /* +64  Logon time               */
    uint32_t       nvdate;           /* +68  Logon date               */
    uint32_t       nvreasn;          /* +72  Reason code (output)     */
    char          *nvmsg;            /* +76  Message area             */
    uint16_t       nvmsglen;         /* +80  Message length           */
    uint16_t       _reserved2;       /* +82  Reserved                 */
};                                   /* Total: 84 bytes               */

/* NVFLAGS for logon exit */
#define NV_LFLG_PWCHG      0x80      /* Password change               */
#define NV_LFLG_RELOGON    0x40      /* Re-logon                      */
#define NV_LFLG_FORCE      0x20      /* Forced logon                  */
#define NV_LFLG_CONSOLE    0x10      /* Console logon                 */

#pragma pack()

/*===================================================================
 * Alert Exit Parameter List (DSIEX08)
 *===================================================================*/

#pragma pack(1)

struct nv_alert_parm {
    EXIT_PARM_HEADER;                /* +0   Common header            */
    char           nvalertid[8];     /* +8   Alert ID                 */
    char           nvresource[32];   /* +16  Resource name            */
    char           nvrestype[8];     /* +48  Resource type            */
    char           nvsysname[8];     /* +56  System name              */
    char          *nvdesc;           /* +64  Description pointer      */
    uint32_t       nvdesclen;        /* +68  Description length       */
    char          *nvprobcause;      /* +72  Probable cause pointer   */
    uint32_t       nvpclen;          /* +76  Probable cause length    */
    char          *nvaction;         /* +80  Recommended action ptr   */
    uint32_t       nvactlen;         /* +84  Action length            */
    uint32_t       nvtime;           /* +88  Alert time               */
    uint32_t       nvdate;           /* +92  Alert date               */
    uint8_t        nvnewsev;         /* +96  New severity (output)    */
    uint8_t        _reserved[3];     /* +97  Reserved                 */
    uint32_t       nvreasn;          /* +100 Reason code              */
};                                   /* Total: 104 bytes              */

/* Mapping for nv_alert_parm:
 * flags = nvseverity
 * reserved high byte = nvalerttype
 * reserved low byte = nvflags
 */

/* NVALERTTYPE - Alert types */
#define NV_ATYPE_NETWORK   0x01      /* Network alert                 */
#define NV_ATYPE_SYSTEM    0x02      /* System alert                  */
#define NV_ATYPE_APPL      0x03      /* Application alert             */
#define NV_ATYPE_STORAGE   0x04      /* Storage alert                 */
#define NV_ATYPE_SECURITY  0x05      /* Security alert                */
#define NV_ATYPE_PERFORM   0x06      /* Performance alert             */

/* NVFLAGS for alert exit */
#define NV_AFLG_FORWARD    0x80      /* Forward alert                 */
#define NV_AFLG_LOG        0x40      /* Log alert                     */
#define NV_AFLG_DISPLAY    0x20      /* Display alert                 */
#define NV_AFLG_SOUND      0x10      /* Sound alarm                   */

#pragma pack()

/*===================================================================
 * Session Monitor Exit Parameter List (DSIEX05)
 *===================================================================*/

#pragma pack(1)

struct nv_session_parm {
    EXIT_PARM_HEADER;                /* +0   Common header            */
    char           nvluname[8];      /* +8   LU name                  */
    char           nvappl[8];        /* +16  Application name         */
    char           nvmode[8];        /* +24  Mode name                */
    char           nvsysname[8];     /* +32  System name              */
    uint32_t       nvsessid;         /* +40  Session ID               */
    uint32_t       nvtime;           /* +44  Event time               */
    uint32_t       nvdate;           /* +48  Event date               */
    uint32_t       nvbytes;          /* +52  Bytes transferred        */
    uint32_t       nvreasn;          /* +56  Reason code              */
};                                   /* Total: 60 bytes               */

/* Mapping for nv_session_parm:
 * reserved high byte = nvevent
 */

/* NVEVENT - Session events */
#define NV_SESS_INIT       0x01      /* Session initiation            */
#define NV_SESS_TERM       0x02      /* Session termination           */
#define NV_SESS_FAIL       0x03      /* Session failure               */
#define NV_SESS_RECOVER    0x04      /* Session recovery              */
#define NV_SESS_TIMEOUT    0x05      /* Session timeout               */

#pragma pack()

/*===================================================================
 * NetView Utility Functions
 *===================================================================*/

/**
 * nv_match_msgid - Compare message IDs
 * @msgid1: First message ID (10 chars)
 * @msgid2: Second message ID (10 chars)
 * Returns: 1 if equal, 0 otherwise
 */
static inline int nv_match_msgid(const char msgid1[10], const char msgid2[10]) {
    return match_field(msgid1, msgid2, 10);
}

/**
 * nv_match_msgid_prefix - Match message ID prefix
 * @msgid: Message ID (10 chars)
 * @prefix: Prefix to match
 * @len: Prefix length
 * Returns: 1 if matches, 0 otherwise
 */
static inline int nv_match_msgid_prefix(const char msgid[10],
                                         const char *prefix,
                                         size_t len) {
    return match_prefix(msgid, prefix, len);
}

/**
 * nv_match_operator - Compare operator IDs
 * @op1: First operator ID (8 chars)
 * @op2: Second operator ID (8 chars)
 * Returns: 1 if equal, 0 otherwise
 */
static inline int nv_match_operator(const char op1[8], const char op2[8]) {
    return match_field(op1, op2, 8);
}

/**
 * nv_is_critical - Check if alert is critical
 * @severity: Alert severity
 * Returns: 1 if critical, 0 otherwise
 */
static inline int nv_is_critical(uint8_t severity) {
    return severity == NV_SEV_CRITICAL;
}

/**
 * nv_is_major_or_critical - Check if alert is major or critical
 * @severity: Alert severity
 * Returns: 1 if major or critical, 0 otherwise
 */
static inline int nv_is_major_or_critical(uint8_t severity) {
    return severity == NV_SEV_CRITICAL || severity == NV_SEV_MAJOR;
}

/**
 * nv_severity_name - Get severity name string
 * @severity: Alert severity
 * Returns: Static string name
 */
static inline const char *nv_severity_name(uint8_t severity) {
    switch (severity) {
        case NV_SEV_CRITICAL: return "CRITICAL";
        case NV_SEV_MAJOR:    return "MAJOR";
        case NV_SEV_MINOR:    return "MINOR";
        case NV_SEV_WARNING:  return "WARNING";
        case NV_SEV_INFO:     return "INFO";
        default:              return "UNKNOWN";
    }
}

/**
 * nv_is_authorized - Check if operator is authorized
 * @flags: NVFLAGS value
 * Returns: 1 if authorized, 0 otherwise
 */
static inline int nv_is_authorized(uint16_t flags) {
    return (flags & NV_FLG_AUTH) != 0;
}

/*===================================================================
 * Example Usage
 *===================================================================*

// Command preprocessing exit - control command execution
#pragma prolog(my_dsiex01,"SAVE(14,12),LR(12,15)")
#pragma epilog(my_dsiex01,"RETURN(14,12)")

int my_dsiex01(struct nv_cmd_parm *parm) {
    if (parm->nvfunc != NV_FUNC_PRE) {
        return NV_CMD_CONTINUE;
    }

    // Block certain commands for non-authorized operators
    if (!nv_is_authorized(parm->nvflags)) {
        // Check for sensitive MVS commands
        if (parm->nvcmdtype == NV_CMDTYPE_MVS) {
            if (memcmp_inline(parm->nvcmd, "CANCEL", 6) == 0 ||
                memcmp_inline(parm->nvcmd, "FORCE", 5) == 0) {
                parm->nvreasn = 100;
                return NV_CMD_SUPPRESS;
            }
        }
    }

    // Log all commands
    // log_command(parm->nvoper, parm->nvcmd, parm->nvcmdlen);

    return NV_CMD_CONTINUE;
}

// Unsolicited message exit - message filtering
#pragma prolog(my_dsiex16,"SAVE(14,12),LR(12,15)")
#pragma epilog(my_dsiex16,"RETURN(14,12)")

int my_dsiex16(struct nv_msg_parm *parm) {
    // Suppress certain messages
    if (nv_match_msgid_prefix(parm->nvmsgid, "IEF", 3)) {
        // Filter routine JCL messages
        if (nv_match_msgid(parm->nvmsgid, "IEF403I   ")) {
            return NV_MSG_SUPPRESS;
        }
    }

    // Flag certain messages for automation
    if (nv_match_msgid_prefix(parm->nvmsgid, "IOS", 3)) {
        // I/O subsystem messages - flag for attention
        parm->nvflags |= NV_MFLG_AUTOMATION;
    }

    return NV_MSG_CONTINUE;
}

// Logon exit - additional authentication
#pragma prolog(my_dsiex06,"SAVE(14,12),LR(12,15)")
#pragma epilog(my_dsiex06,"RETURN(14,12)")

int my_dsiex06(struct nv_logon_parm *parm) {
    // Check for time restrictions
    // if (is_restricted_time(parm->nvtime)) {
    //     parm->nvreasn = 200;
    //     return NV_LOGON_REJECT;
    // }

    // Log all logon attempts
    // log_logon(parm->nvoper, parm->nvdomain, parm->nvlterm);

    // Defer to RACF for authentication
    return NV_LOGON_DEFER;
}

// Alert exit - alert processing
#pragma prolog(my_dsiex08,"SAVE(14,12),LR(12,15)")
#pragma epilog(my_dsiex08,"RETURN(14,12)")

int my_dsiex08(struct nv_alert_parm *parm) {
    // Escalate critical alerts
    if (nv_is_critical(parm->nvseverity)) {
        // send_critical_notification(parm);
        parm->nvflags |= NV_AFLG_FORWARD | NV_AFLG_SOUND;
    }

    // Log all alerts
    // log_alert(parm->nvalertid, parm->nvresource, parm->nvseverity);

    return NV_ALERT_CONTINUE;
}

 *===================================================================*/

#endif /* METALC_NETVIEW_H */
