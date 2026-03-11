/*********************************************************************
 * METALC_SA.H - System Automation Exit definitions for Metal C
 *
 * This header provides control block structures, constants, and
 * utility functions for IBM System Automation for z/OS exits.
 *
 * Supported exits:
 *   Resource exits:
 *     AOFEXC01 - Resource monitoring exit
 *     AOFEXC02 - Resource state change exit
 *     AOFEXC03 - Automation policy exit
 *
 *   Message exits:
 *     AOFEXC10 - Message processing exit
 *     AOFEXC11 - Message reply exit
 *
 *   Event exits:
 *     AOFEXC20 - Timer event exit
 *     AOFEXC21 - Status event exit
 *     AOFEXC22 - Availability event exit
 *
 *   Recovery exits:
 *     AOFEXC30 - Recovery action exit
 *     AOFEXC31 - Escalation exit
 *
 *   Notify exits:
 *     AOFEXC40 - Notification exit
 *     AOFEXC41 - Alert distribution exit
 *
 * Usage:
 *   #include "metalc_base.h"
 *   #include "metalc_sa.h"
 *
 * IMPORTANT: SA exits run in the SA agent or manager address space.
 *********************************************************************/

#ifndef METALC_SA_H
#define METALC_SA_H

#include "metalc_base.h"

/*===================================================================
 * System Automation Exit Return Codes
 *===================================================================*/

/* General exit return codes */
#define SA_RC_CONTINUE         RC_OK     /* Continue processing           */
#define SA_RC_SUPPRESS         RC_WARNING  /* Suppress action               */
#define SA_RC_MODIFIED         RC_ERROR    /* Parameters modified           */
#define SA_RC_ERROR            RC_SEVERE   /* Error occurred                */
#define SA_RC_ABORT            RC_CRITICAL /* Abort processing              */

/* Resource exit return codes */
#define SA_RES_CONTINUE        RC_OK     /* Continue resource processing  */
#define SA_RES_SUPPRESS        RC_WARNING  /* Suppress resource action      */
#define SA_RES_OVERRIDE        RC_ERROR    /* Override automation           */
#define SA_RES_DEFER           RC_SEVERE   /* Defer action                  */

/* Recovery exit return codes */
#define SA_REC_CONTINUE        RC_OK     /* Continue recovery             */
#define SA_REC_SUPPRESS        RC_WARNING  /* Suppress recovery action      */
#define SA_REC_ALTERNATIVE     RC_ERROR    /* Use alternative action        */
#define SA_REC_ESCALATE        RC_SEVERE   /* Escalate immediately          */

/* Notification exit return codes */
#define SA_NOT_CONTINUE        RC_OK     /* Continue notification         */
#define SA_NOT_SUPPRESS        RC_WARNING  /* Suppress notification         */
#define SA_NOT_REDIRECT        RC_ERROR    /* Redirect notification         */

/*===================================================================
 * Resource Types
 *===================================================================*/

#define SA_RESTYPE_APL         0x01  /* Application                   */
#define SA_RESTYPE_SUBSYS      0x02  /* Subsystem                     */
#define SA_RESTYPE_JOB         0x03  /* Job                           */
#define SA_RESTYPE_DEVICE      0x04  /* Device                        */
#define SA_RESTYPE_DATASET     0x05  /* Dataset                       */
#define SA_RESTYPE_SYSTEM      0x06  /* System                        */
#define SA_RESTYPE_GROUP       0x07  /* Application group             */
#define SA_RESTYPE_SERVICE     0x08  /* Service                       */
#define SA_RESTYPE_SYSPLEX     0x09  /* Sysplex                       */
#define SA_RESTYPE_MONITOR     0x0A  /* Monitor resource              */

/*===================================================================
 * Resource States
 *===================================================================*/

/* Observed states */
#define SA_STATE_UNKNOWN       0x00  /* Unknown                       */
#define SA_STATE_HARDDOWN      0x01  /* Hard down                     */
#define SA_STATE_SOFTDOWN      0x02  /* Soft down                     */
#define SA_STATE_STARTING      0x03  /* Starting                      */
#define SA_STATE_AVAILABLE     0x04  /* Available                     */
#define SA_STATE_DEGRADED      0x05  /* Degraded                      */
#define SA_STATE_STOPPING      0x06  /* Stopping                      */
#define SA_STATE_PROBLEM       0x07  /* Problem                       */
#define SA_STATE_SYSGONE       0x08  /* System gone                   */

/* Desired states */
#define SA_DESIRE_UNAVAILABLE  0x01  /* Unavailable                   */
#define SA_DESIRE_AVAILABLE    0x02  /* Available                     */
#define SA_DESIRE_UNCHANGED    0x03  /* Unchanged                     */

/* Automation flags */
#define SA_AUTO_YES            0x01  /* Automation enabled            */
#define SA_AUTO_NO             0x02  /* Automation disabled           */
#define SA_AUTO_ASSIST         0x03  /* Assist mode                   */

/*===================================================================
 * Event Types
 *===================================================================*/

#define SA_EVENT_STATUS        0x01  /* Status change event           */
#define SA_EVENT_MESSAGE       0x02  /* Message event                 */
#define SA_EVENT_TIMER         0x03  /* Timer event                   */
#define SA_EVENT_COMMAND       0x04  /* Command event                 */
#define SA_EVENT_STARTUP       0x05  /* Startup event                 */
#define SA_EVENT_SHUTDOWN      0x06  /* Shutdown event                */
#define SA_EVENT_RECOVERY      0x07  /* Recovery event                */
#define SA_EVENT_HEALTH        0x08  /* Health check event            */
#define SA_EVENT_NOTIFY        0x09  /* Notification event            */
#define SA_EVENT_ESCALATE      0x0A  /* Escalation event              */
#define SA_EVENT_SYSPLEX       0x0B  /* Sysplex event                 */

/*===================================================================
 * Recovery Actions
 *===================================================================*/

#define SA_RECACT_START        0x01  /* Start resource                */
#define SA_RECACT_STOP         0x02  /* Stop resource                 */
#define SA_RECACT_RESTART      0x03  /* Restart resource              */
#define SA_RECACT_RECYCLE      0x04  /* Recycle resource              */
#define SA_RECACT_CANCEL       0x05  /* Cancel job                    */
#define SA_RECACT_FORCE        0x06  /* Force termination             */
#define SA_RECACT_IPL          0x07  /* IPL system                    */
#define SA_RECACT_MOVE         0x08  /* Move to another system        */
#define SA_RECACT_NOTIFY       0x09  /* Notify only                   */
#define SA_RECACT_CUSTOM       0x10  /* Custom action                 */

/*===================================================================
 * Notification Types
 *===================================================================*/

#define SA_NOTTYPE_EMAIL       0x01  /* Email notification            */
#define SA_NOTTYPE_PAGE        0x02  /* Pager notification            */
#define SA_NOTTYPE_SMS         0x03  /* SMS notification              */
#define SA_NOTTYPE_CONSOLE     0x04  /* Console message               */
#define SA_NOTTYPE_TRAP        0x05  /* SNMP trap                     */
#define SA_NOTTYPE_LOG         0x06  /* Log entry                     */
#define SA_NOTTYPE_SCRIPT      0x07  /* Script execution              */
#define SA_NOTTYPE_EVENT       0x08  /* Event forwarding              */

/*===================================================================
 * Severity Levels
 *===================================================================*/

#define SA_SEVERITY_INFO       0x01  /* Informational                 */
#define SA_SEVERITY_WARNING    0x02  /* Warning                       */
#define SA_SEVERITY_MINOR      0x03  /* Minor                         */
#define SA_SEVERITY_MAJOR      0x04  /* Major                         */
#define SA_SEVERITY_CRITICAL   0x05  /* Critical                      */
#define SA_SEVERITY_FATAL      0x06  /* Fatal                         */

/*===================================================================
 * Resource Description Block
 *===================================================================*/

#pragma pack(1)

struct sa_resource {
    char           resname[32];      /* +0   Resource name            */
    uint8_t        restype;          /* +32  Resource type            */
    uint8_t        obsstate;         /* +33  Observed state           */
    uint8_t        desstate;         /* +34  Desired state            */
    uint8_t        automation;       /* +35  Automation flag          */
    char           resgroup[32];     /* +36  Resource group           */
    char           system[8];        /* +68  System name              */
    char           sysplex[8];       /* +76  Sysplex name             */
    char           jobname[8];       /* +84  Job name                 */
    char           asid[4];          /* +92  ASID (hex)               */
    uint32_t       starttime;        /* +96  Start time               */
    uint32_t       startdate;        /* +100 Start date               */
    uint32_t       statetime;        /* +104 State change time        */
    uint32_t       statedate;        /* +108 State change date        */
    char           subsys[8];        /* +112 Subsystem ID             */
    char           policy[32];       /* +120 Policy name              */
    uint8_t        health;           /* +152 Health indicator         */
    uint8_t        priority;         /* +153 Resource priority        */
    uint8_t        movegroup;        /* +154 Move group ID            */
    uint8_t        flags;            /* +155 Flags                    */
};                                   /* Total: 156 bytes              */

/* FLAGS bits */
#define SA_RES_FLG_CRITICAL  0x80    /* Critical resource             */
#define SA_RES_FLG_MONITORED 0x40    /* Actively monitored            */
#define SA_RES_FLG_MOVEABLE  0x20    /* Can be moved                  */
#define SA_RES_FLG_RESTARTAB 0x10    /* Restartable                   */
#define SA_RES_FLG_HOLDSTART 0x08    /* Hold on start                 */

#pragma pack()

/*===================================================================
 * Resource Exit Parameter List (AOFEXC01/02)
 *===================================================================*/

#pragma pack(1)

struct sa_res_parm {
    EXIT_PARM_HEADER;                /* +0   Common header            */
    struct sa_resource *sares;       /* +8   Resource info pointer    */
    uint8_t        saoldstate;       /* +12  Old observed state       */
    uint8_t        sanewstate;       /* +13  New observed state       */
    uint8_t        saolddesire;      /* +14  Old desired state        */
    uint8_t        sanewdesire;      /* +15  New desired state        */
    char          *sacmd;            /* +16  Command pointer          */
    uint32_t       sacmdlen;         /* +20  Command length           */
    char          *sanewcmd;         /* +24  New command (output)     */
    uint32_t       sanewlen;         /* +28  New command length       */
    uint32_t       sareasn;          /* +32  Reason code              */
    char          *samsg;            /* +36  Message area             */
    uint16_t       samsglen;         /* +40  Message length           */
    uint16_t       _reserved;        /* +42  Reserved                 */
};                                   /* Total: 44 bytes               */

/* SAFUNC function codes */
#define SA_FUNC_INIT       EXIT_FUNC_INIT      /* Initialization                */
#define SA_FUNC_MONITOR    0x02      /* Monitor event                 */
#define SA_FUNC_STATCHG    0x03      /* State change                  */
#define SA_FUNC_COMMAND    0x04      /* Command execution             */
#define SA_FUNC_TERM       EXIT_FUNC_TERM      /* Termination                   */

/* SAFLAGS bits (mapped to reserved) */
#define SA_FLG_MANUAL      0x8000    /* Manual operation              */
#define SA_FLG_RECOVERY    0x4000    /* Recovery in progress          */
#define SA_FLG_SCHEDULED   0x2000    /* Scheduled action              */
#define SA_FLG_SYSPLEX     0x1000    /* Sysplex-wide action           */

#pragma pack()

/*===================================================================
 * Recovery Exit Parameter List (AOFEXC30)
 *===================================================================*/

#pragma pack(1)

struct sa_rec_parm {
    EXIT_PARM_HEADER;                /* +0   Common header            */
    uint8_t        saattempt;        /* +6   Attempt number           */
    uint8_t        samaxatt;         /* +7   Maximum attempts         */
    struct sa_resource *sares;       /* +8   Resource info pointer    */
    char          *sacommand;        /* +12  Recovery command         */
    uint32_t       sacmdlen;         /* +16  Command length           */
    char          *saaltcmd;         /* +20  Alternative command      */
    uint32_t       saaltlen;         /* +24  Alternative length       */
    uint32_t       sawait;           /* +28  Wait time (seconds)      */
    uint32_t       sareasn;          /* +32  Reason code              */
    uint8_t        saseverity;       /* +36  Severity                 */
    uint8_t        saescalate;       /* +37  Escalation level         */
    uint16_t       _reserved;        /* +38  Reserved                 */
    char          *samsg;            /* +40  Message area             */
    uint16_t       samsglen;         /* +44  Message length           */
    uint16_t       _reserved2;       /* +46  Reserved                 */
};                                   /* Total: 48 bytes               */

/* SAACTION values use SA_RECACT_* constants */

#pragma pack()

/*===================================================================
 * Message Exit Parameter List (AOFEXC10)
 *===================================================================*/

#pragma pack(1)

struct sa_msg_parm {
    EXIT_PARM_HEADER;                /* +0   Common header            */
    char           samsgid[10];      /* +8   Message ID               */
    char           _reserved1[2];    /* +18  Reserved                 */
    char          *samsgtext;        /* +20  Message text pointer     */
    uint32_t       samsgtlen;        /* +24  Message text length      */
    char           sajobname[8];     /* +28  Job name                 */
    char           sasystem[8];      /* +36  System name              */
    struct sa_resource *sares;       /* +44  Resource pointer         */
    char          *sanewmsg;         /* +48  New message (output)     */
    uint32_t       sanewlen;         /* +52  New message length       */
    uint32_t       sareasn;          /* +56  Reason code              */
};                                   /* Total: 60 bytes               */

/* SAMSGTYPE values */
#define SA_MSGTYPE_WTO     0x01      /* WTO                           */
#define SA_MSGTYPE_WTOR    0x02      /* WTOR                          */
#define SA_MSGTYPE_LOG     0x03      /* Log message                   */
#define SA_MSGTYPE_ALERT   0x04      /* Alert                         */

#pragma pack()

/*===================================================================
 * Notification Exit Parameter List (AOFEXC40)
 *===================================================================*/

#pragma pack(1)

struct sa_notify_parm {
    EXIT_PARM_HEADER;                /* +0   Common header            */
    struct sa_resource *sares;       /* +8   Resource pointer         */
    char           saevent[32];      /* +12  Event description        */
    char          *samessage;        /* +44  Message text pointer     */
    uint32_t       samsglen;         /* +48  Message length           */
    char           sarecip[64];      /* +52  Recipients               */
    char           sanewrec[64];     /* +116 New recipients (output)  */
    uint32_t       sareasn;          /* +180 Reason code              */
    uint32_t       satime;           /* +184 Notification time        */
    uint32_t       sadate;           /* +188 Notification date        */
};                                   /* Total: 192 bytes              */

/* SAFLAGS for notification */
#define SA_NOT_FLG_IMMEDIATE 0x80    /* Immediate notification        */
#define SA_NOT_FLG_AGGREGATE 0x40    /* Aggregate with others         */
#define SA_NOT_FLG_ESCALATE  0x20    /* Escalation notification       */

#pragma pack()

/*===================================================================
 * Timer Event Exit Parameter List (AOFEXC20)
 *===================================================================*/

#pragma pack(1)

struct sa_timer_parm {
    EXIT_PARM_HEADER;                /* +0   Common header            */
    char           satimer[32];      /* +8   Timer name               */
    uint32_t       sainterval;       /* +40  Interval (seconds)       */
    uint32_t       saexptime;        /* +44  Expiration time          */
    uint32_t       saexpdate;        /* +48  Expiration date          */
    char          *sacommand;        /* +52  Command to execute       */
    uint32_t       sacmdlen;         /* +56  Command length           */
    char          *sanewcmd;         /* +60  New command (output)     */
    uint32_t       sanewlen;         /* +64  New command length       */
    uint32_t       sareasn;          /* +68  Reason code              */
};                                   /* Total: 72 bytes               */

#pragma pack()

/*===================================================================
 * System Automation Utility Functions
 *===================================================================*/

/**
 * sa_is_available - Check if resource is available
 * @state: Observed state
 * Returns: 1 if available, 0 otherwise
 */
static inline int sa_is_available(uint8_t state) {
    return state == SA_STATE_AVAILABLE;
}

/**
 * sa_is_down - Check if resource is down
 * @state: Observed state
 * Returns: 1 if down, 0 otherwise
 */
static inline int sa_is_down(uint8_t state) {
    return state == SA_STATE_HARDDOWN || state == SA_STATE_SOFTDOWN;
}

/**
 * sa_is_transitioning - Check if resource is in transition
 * @state: Observed state
 * Returns: 1 if transitioning, 0 otherwise
 */
static inline int sa_is_transitioning(uint8_t state) {
    return state == SA_STATE_STARTING || state == SA_STATE_STOPPING;
}

/**
 * sa_is_problem - Check if resource has a problem
 * @state: Observed state
 * Returns: 1 if problem, 0 otherwise
 */
static inline int sa_is_problem(uint8_t state) {
    return state == SA_STATE_PROBLEM || state == SA_STATE_DEGRADED;
}

/**
 * sa_is_critical - Check if severity is critical or higher
 * @severity: Severity level
 * Returns: 1 if critical or fatal, 0 otherwise
 */
static inline int sa_is_critical(uint8_t severity) {
    return severity >= SA_SEVERITY_CRITICAL;
}

/**
 * sa_state_name - Get state name string
 * @state: Observed state
 * Returns: Static string name
 */
static inline const char *sa_state_name(uint8_t state) {
    switch (state) {
        case SA_STATE_UNKNOWN:   return "UNKNOWN";
        case SA_STATE_HARDDOWN:  return "HARDDOWN";
        case SA_STATE_SOFTDOWN:  return "SOFTDOWN";
        case SA_STATE_STARTING:  return "STARTING";
        case SA_STATE_AVAILABLE: return "AVAILABLE";
        case SA_STATE_DEGRADED:  return "DEGRADED";
        case SA_STATE_STOPPING:  return "STOPPING";
        case SA_STATE_PROBLEM:   return "PROBLEM";
        case SA_STATE_SYSGONE:   return "SYSGONE";
        default:                 return "UNKNOWN";
    }
}

/**
 * sa_severity_name - Get severity name string
 * @severity: Severity level
 * Returns: Static string name
 */
static inline const char *sa_severity_name(uint8_t severity) {
    switch (severity) {
        case SA_SEVERITY_INFO:     return "INFO";
        case SA_SEVERITY_WARNING:  return "WARNING";
        case SA_SEVERITY_MINOR:    return "MINOR";
        case SA_SEVERITY_MAJOR:    return "MAJOR";
        case SA_SEVERITY_CRITICAL: return "CRITICAL";
        case SA_SEVERITY_FATAL:    return "FATAL";
        default:                   return "UNKNOWN";
    }
}

/**
 * sa_match_resource - Compare resource names
 * @res1: First resource name (32 chars)
 * @res2: Second resource name (32 chars)
 * Returns: 1 if equal, 0 otherwise
 */
static inline int sa_match_resource(const char res1[32], const char res2[32]) {
    return match_field(res1, res2, 32);
}

/**
 * sa_is_automation_enabled - Check if automation is enabled
 * @res: Resource descriptor
 * Returns: 1 if enabled, 0 otherwise
 */
static inline int sa_is_automation_enabled(const struct sa_resource *res) {
    return res->automation == SA_AUTO_YES;
}

/*===================================================================
 * Example Usage
 *===================================================================*

// Resource state change exit
#pragma prolog(my_aofexc02,"SAVE(14,12),LR(12,15)")
#pragma epilog(my_aofexc02,"RETURN(14,12)")

int my_aofexc02(struct sa_res_parm *parm) {
    if (parm->safunc != SA_FUNC_STATCHG) {
        return SA_RES_CONTINUE;
    }

    struct sa_resource *res = parm->sares;

    // Log state changes for critical resources
    if (res->flags & SA_RES_FLG_CRITICAL) {
        // log_state_change(res->resname, parm->saoldstate, parm->sanewstate);
    }

    // Suppress auto-recovery for certain resources during maintenance
    // if (is_maintenance_window() && res->restype == SA_RESTYPE_APL) {
    //     return SA_RES_SUPPRESS;
    // }

    return SA_RES_CONTINUE;
}

// Recovery action exit
#pragma prolog(my_aofexc30,"SAVE(14,12),LR(12,15)")
#pragma epilog(my_aofexc30,"RETURN(14,12)")

int my_aofexc30(struct sa_rec_parm *parm) {
    struct sa_resource *res = parm->sares;

    // Check if we should escalate after multiple failed attempts
    if (parm->saattempt >= parm->samaxatt) {
        parm->saescalate = 1;
        parm->saseverity = SA_SEVERITY_CRITICAL;
        return SA_REC_ESCALATE;
    }

    // Use alternative action for certain resources
    if (res->restype == SA_RESTYPE_SUBSYS) {
        if (parm->saaction == SA_RECACT_RESTART &&
            parm->saattempt > 1) {
            // Try recycle instead of restart
            parm->saaction = SA_RECACT_RECYCLE;
            return SA_REC_ALTERNATIVE;
        }
    }

    return SA_REC_CONTINUE;
}

// Notification exit
#pragma prolog(my_aofexc40,"SAVE(14,12),LR(12,15)")
#pragma epilog(my_aofexc40,"RETURN(14,12)")

int my_aofexc40(struct sa_notify_parm *parm) {
    // Redirect critical notifications
    if (sa_is_critical(parm->saseverity)) {
        // Add on-call team to recipients
        // mq_set_string(parm->sanewrec, 64, "oncall@company.com");
        parm->saflags |= SA_NOT_FLG_IMMEDIATE;
    }

    // Suppress informational notifications during quiet hours
    // if (is_quiet_hours() && parm->saseverity == SA_SEVERITY_INFO) {
    //     return SA_NOT_SUPPRESS;
    // }

    return SA_NOT_CONTINUE;
}

// Message exit
#pragma prolog(my_aofexc10,"SAVE(14,12),LR(12,15)")
#pragma epilog(my_aofexc10,"RETURN(14,12)")

int my_aofexc10(struct sa_msg_parm *parm) {
    // Associate messages with resources
    if (parm->sares != NULL) {
        // Resource-specific processing
        if (sa_is_problem(parm->sares->obsstate)) {
            // Flag message for attention
        }
    }

    return SA_RC_CONTINUE;
}

 *===================================================================*/

#endif /* METALC_SA_H */
