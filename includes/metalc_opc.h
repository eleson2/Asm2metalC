/*********************************************************************
 * METALC_OPC.H - OPC/TWS Exit definitions for Metal C
 *
 * This header provides control block structures, constants, and
 * utility functions for IBM Tivoli Workload Scheduler (TWS) for z/OS
 * exit development. Formerly known as OPC (Operations Planning and
 * Control).
 *
 * Supported exits:
 *   EQQUX001 - Event reader exit
 *   EQQUX002 - Event filtering exit
 *   EQQUX003 - Job submit exit
 *   EQQUX004 - Job library read exit
 *   EQQUX005 - JCC message table exit
 *   EQQUX006 - JCC incident exit
 *   EQQUX007 - Operation status change exit
 *   EQQUX009 - Job tracking exit
 *   EQQUX010 - Workload restart exit
 *   EQQUX011 - ETT event exit
 *
 * Usage:
 *   #include "metalc_base.h"
 *   #include "metalc_opc.h"
 *
 * IMPORTANT: Control block layouts vary by TWS version.
 *            Verify offsets against your installation.
 *********************************************************************/

#ifndef METALC_OPC_H
#define METALC_OPC_H

#include "metalc_base.h"

/*===================================================================
 * OPC/TWS Exit Return Codes
 *===================================================================*/

/* EQQUX001 (Event Reader) return codes */
#define OPC_UX001_CONTINUE     RC_OK     /* Continue processing           */
#define OPC_UX001_REJECT       RC_WARNING  /* Reject the event              */
#define OPC_UX001_TERM         RC_ERROR    /* Terminate event reader        */

/* EQQUX002 (Event Filter) return codes */
#define OPC_UX002_ACCEPT       RC_OK     /* Accept the event              */
#define OPC_UX002_REJECT       RC_WARNING  /* Reject the event              */

/* EQQUX003 (Job Submit) return codes */
#define OPC_UX003_CONTINUE     RC_OK     /* Continue with submit          */
#define OPC_UX003_MODIFY       RC_WARNING  /* JCL was modified              */
#define OPC_UX003_REJECT       RC_ERROR    /* Reject the submission         */
#define OPC_UX003_DELAY        RC_SEVERE   /* Delay the submission          */

/* EQQUX004 (Job Library Read) return codes */
#define OPC_UX004_CONTINUE     RC_OK     /* Continue processing           */
#define OPC_UX004_REPLACE      RC_WARNING  /* JCL replaced by exit          */
#define OPC_UX004_REJECT       RC_ERROR    /* Reject - do not submit        */

/* EQQUX005 (JCC Message Table) return codes */
#define OPC_UX005_CONTINUE     RC_OK     /* Continue processing           */
#define OPC_UX005_MODIFIED     RC_WARNING  /* Entry was modified            */

/* EQQUX006 (JCC Incident) return codes */
#define OPC_UX006_CONTINUE     RC_OK     /* Continue processing           */
#define OPC_UX006_CANCEL       RC_WARNING  /* Cancel incident               */
#define OPC_UX006_MODIFY       RC_ERROR    /* Incident was modified         */

/* EQQUX007 (Operation Status Change) return codes */
#define OPC_UX007_CONTINUE     RC_OK     /* Continue processing           */
#define OPC_UX007_MODIFY       RC_WARNING  /* Status was modified           */
#define OPC_UX007_REJECT       RC_ERROR    /* Reject status change          */

/* EQQUX009 (Job Tracking) return codes */
#define OPC_UX009_CONTINUE     RC_OK     /* Continue tracking             */
#define OPC_UX009_IGNORE       RC_WARNING  /* Ignore this job               */

/* EQQUX010 (Workload Restart) return codes */
#define OPC_UX010_CONTINUE     RC_OK     /* Continue restart              */
#define OPC_UX010_REJECT       RC_WARNING  /* Reject restart                */

/* EQQUX011 (ETT Event) return codes */
#define OPC_UX011_CONTINUE     RC_OK     /* Continue processing           */
#define OPC_UX011_REJECT       RC_WARNING  /* Reject the event              */

/*===================================================================
 * OPC/TWS Operation Status Codes
 *===================================================================*/

#define OPC_STATUS_WAITING     'W'   /* Waiting                       */
#define OPC_STATUS_READY       'R'   /* Ready                         */
#define OPC_STATUS_STARTING    'S'   /* Starting                      */
#define OPC_STATUS_ACTIVE      'A'   /* Active/Executing              */
#define OPC_STATUS_COMPLETE    'C'   /* Complete                      */
#define OPC_STATUS_ERROR       'E'   /* Ended in error                */
#define OPC_STATUS_INTERRUPT   'I'   /* Interrupted                   */
#define OPC_STATUS_PENDING     'P'   /* Pending                       */
#define OPC_STATUS_UNDECIDED   'U'   /* Undecided                     */
#define OPC_STATUS_DELETE      'D'   /* Deleted                       */
#define OPC_STATUS_HOLD        'H'   /* Held                          */

/*===================================================================
 * OPC/TWS Event Types
 *===================================================================*/

#define OPC_EVENT_JOB_START    0x01  /* Job started                   */
#define OPC_EVENT_JOB_END      0x02  /* Job ended                     */
#define OPC_EVENT_STEP_END     0x03  /* Step ended                    */
#define OPC_EVENT_DATASET      0x04  /* Dataset triggered             */
#define OPC_EVENT_MANUAL       0x05  /* Manual event                  */
#define OPC_EVENT_TIME         0x06  /* Time event                    */
#define OPC_EVENT_RESOURCE     0x07  /* Resource event                */
#define OPC_EVENT_WLM          0x08  /* WLM event                     */

/*===================================================================
 * OPC/TWS Extended Status Codes
 *===================================================================*/

#define OPC_EXT_NORMAL         0x00  /* Normal completion             */
#define OPC_EXT_JCL_ERROR      0x01  /* JCL error                     */
#define OPC_EXT_ABEND          0x02  /* Abend                         */
#define OPC_EXT_COND_CODE      0x03  /* Condition code exceeded       */
#define OPC_EXT_TIMEOUT        0x04  /* Timeout                       */
#define OPC_EXT_CANCELLED      0x05  /* Cancelled by operator         */
#define OPC_EXT_RESTARTED      0x06  /* Restarted                     */
#define OPC_EXT_LATE           0x07  /* Late start                    */
#define OPC_EXT_DURATION       0x08  /* Duration exceeded             */

/*===================================================================
 * ADID - Application Description ID
 *===================================================================*/

#pragma pack(1)

struct opc_adid {
    char           ad_name[16];      /* +0   Application name         */
    char           ad_group[8];      /* +16  Application group        */
    uint32_t       ad_ia;            /* +24  Input arrival date       */
    uint16_t       ad_iatime;        /* +28  Input arrival time       */
    uint16_t       ad_oper;          /* +30  Operation number         */
};                                   /* Total: 32 bytes               */

#pragma pack()

/*===================================================================
 * Operation Information Block
 *===================================================================*/

#pragma pack(1)

struct opc_oper {
    char           op_adname[16];    /* +0   Application name         */
    char           op_group[8];      /* +16  Application group        */
    uint32_t       op_ia;            /* +24  Input arrival date       */
    uint16_t       op_opnum;         /* +28  Operation number         */
    uint8_t        op_status;        /* +30  Operation status         */
    uint8_t        op_extstatus;     /* +31  Extended status          */
    char           op_jobname[8];    /* +32  Job name                 */
    char           op_wsname[4];     /* +40  Workstation name         */
    uint32_t       op_estdur;        /* +44  Estimated duration (sec) */
    uint32_t       op_actdur;        /* +48  Actual duration (sec)    */
    uint32_t       op_planstart;     /* +52  Planned start time       */
    uint32_t       op_actstart;      /* +56  Actual start time        */
    uint32_t       op_planend;       /* +60  Planned end time         */
    uint32_t       op_actend;        /* +64  Actual end time          */
    int16_t        op_maxrc;         /* +68  Maximum return code      */
    uint8_t        op_priority;      /* +70  Priority                 */
    uint8_t        op_flags;         /* +71  Flags                    */
    char           op_desc[24];      /* +72  Operation description    */
    char           op_owner[8];      /* +96  Owner                    */
    char           op_jclmem[8];     /* +104 JCL member name          */
    char           op_jcllib[44];    /* +112 JCL library              */
};                                   /* Total: 156 bytes              */

/* OP_FLAGS bits */
#define OPC_OPF_CRITICAL     0x80    /* Critical path operation       */
#define OPC_OPF_WLM          0x40    /* WLM managed                   */
#define OPC_OPF_RESTARTABLE  0x20    /* Restartable                   */
#define OPC_OPF_AUTOCREATE   0x10    /* Auto-created                  */
#define OPC_OPF_PARALLEL     0x08    /* Parallel operation            */

#pragma pack()

/*===================================================================
 * EQQUX001 - Event Reader Exit Parameter List
 *===================================================================*/

#pragma pack(1)

struct opc_ux001_parm {
    void          *ux001work;        /* +0   Work area pointer        */
    uint8_t        ux001func;        /* +4   Function code            */
    uint8_t        ux001evty;        /* +5   Event type               */
    uint16_t       _reserved1;       /* +6   Reserved                 */
    char          *ux001data;        /* +8   Event data pointer       */
    uint32_t       ux001dlen;        /* +12  Event data length        */
    char           ux001jobn[8];     /* +16  Job name                 */
    char           ux001jnum[8];     /* +24  JES job number           */
    uint32_t       ux001retc;        /* +32  Return code              */
    uint32_t       ux001reas;        /* +36  Reason code              */
};                                   /* Total: 40 bytes               */

/* UX001FUNC function codes */
#define OPC_UX001_INIT       EXIT_FUNC_INIT    /* Initialization                */
#define OPC_UX001_PROCESS    0x02    /* Process event                 */
#define OPC_UX001_TERM       EXIT_FUNC_TERM    /* Termination                   */

#pragma pack()

/*===================================================================
 * EQQUX002 - Event Filter Exit Parameter List
 *===================================================================*/

#pragma pack(1)

struct opc_ux002_parm {
    void          *ux002work;        /* +0   Work area pointer        */
    uint8_t        ux002func;        /* +4   Function code            */
    uint8_t        ux002evty;        /* +5   Event type               */
    uint16_t       _reserved1;       /* +6   Reserved                 */
    char           ux002jobn[8];     /* +8   Job name                 */
    char           ux002jnum[8];     /* +16  JES job number           */
    int32_t        ux002rc;          /* +24  Job return code          */
    uint32_t       ux002abnd;        /* +28  Abend code               */
    char           ux002step[8];     /* +32  Step name                */
    char           ux002pgm[8];      /* +40  Program name             */
    uint32_t       ux002time;        /* +48  Event time               */
    uint32_t       ux002date;        /* +52  Event date               */
};                                   /* Total: 56 bytes               */

#pragma pack()

/*===================================================================
 * EQQUX003 - Job Submit Exit Parameter List
 *===================================================================*/

#pragma pack(1)

struct opc_ux003_parm {
    EXIT_PARM_HEADER;                /* +0   Common header            */
    struct opc_oper *ux003oper;      /* +8   Operation info pointer   */
    char          *ux003jcl;         /* +12  JCL buffer pointer       */
    uint32_t       ux003jlen;        /* +16  JCL buffer length        */
    uint32_t       ux003jact;        /* +20  Actual JCL length        */
    char           ux003jobn[8];     /* +24  Job name                 */
    char           ux003wsn[4];      /* +32  Workstation name         */
    uint32_t       ux003dlay;        /* +36  Delay time (seconds)     */
    char          *ux003msg;         /* +40  Message area pointer     */
    uint16_t       ux003msgl;        /* +44  Message length           */
    uint16_t       _reserved2;       /* +46  Reserved                 */
};                                   /* Total: 48 bytes               */

/* UX003FUNC function codes */
#define OPC_UX003_INIT       EXIT_FUNC_INIT    /* Initialization                */
#define OPC_UX003_SUBMIT     0x02    /* Submit request                */
#define OPC_UX003_TERM       EXIT_FUNC_TERM    /* Termination                   */

/* UX003FLAGS bits */
#define OPC_UX003_RERUN      0x80    /* This is a rerun               */
#define OPC_UX003_RESTART    0x40    /* This is a restart             */
#define OPC_UX003_HOLD       0x20    /* Submit with TYPRUN=HOLD       */

#pragma pack()

/*===================================================================
 * EQQUX004 - Job Library Read Exit Parameter List
 *===================================================================*/

#pragma pack(1)

struct opc_ux004_parm {
    EXIT_PARM_HEADER;                /* +0   Common header            */
    struct opc_oper *ux004oper;      /* +8   Operation info pointer   */
    char          *ux004jcl;         /* +12  JCL buffer pointer       */
    uint32_t       ux004jmax;        /* +16  JCL buffer max size      */
    uint32_t       ux004jact;        /* +20  Actual JCL length        */
    char           ux004mem[8];      /* +24  Member name              */
    char           ux004lib[44];     /* +32  Library name             */
};                                   /* Total: 76 bytes               */

#pragma pack()

/*===================================================================
 * EQQUX007 - Operation Status Change Exit Parameter List
 *===================================================================*/

#pragma pack(1)

struct opc_ux007_parm {
    uint32_t       ux007func;        /* +0   Function code            */
    struct opc_oper *ux007oper;      /* +4   Operation info pointer   */
    uint8_t        ux007oldst;       /* +8   Old status               */
    uint8_t        ux007newst;       /* +9   New status               */
    uint16_t       _reserved1;       /* +10  Reserved                 */
    int32_t        ux007rc;          /* +12  Return code              */
    uint32_t       ux007abnd;        /* +16  Abend code               */
    char          *ux007msg;         /* +20  Message area pointer     */
    uint16_t       ux007msgl;        /* +24  Message length           */
    uint16_t       _reserved2;       /* +26  Reserved                 */
};                                   /* Total: 28 bytes               */

/* UX007CHGTYP constants (actually function codes for this exit) */
#define OPC_UX007_INIT       EXIT_FUNC_INIT
#define OPC_UX007_STATUS     0x02
#define OPC_UX007_TERM       EXIT_FUNC_TERM

#pragma pack()

/*===================================================================
 * EQQUX009 - Job Tracking Exit Parameter List
 *===================================================================*/

#pragma pack(1)

struct opc_ux009_parm {
    EXIT_PARM_HEADER;                /* +0   Common header            */
    char           ux009jobn[8];     /* +8   Job name                 */
    char           ux009jnum[8];     /* +16  JES job number           */
    int32_t        ux009rc;          /* +24  Return code              */
    uint32_t       ux009abnd;        /* +28  Abend code               */
    char           ux009step[8];     /* +32  Step name                */
    char           ux009pgm[8];      /* +40  Program name             */
    char           ux009adnm[16];    /* +48  Application name         */
    char           ux009grp[8];      /* +64  Application group        */
    uint16_t       ux009opno;        /* +72  Operation number         */
    uint16_t       _reserved2;       /* +74  Reserved                 */
};                                   /* Total: 76 bytes               */

/* UX009FLAGS bits */
#define OPC_UX009_TRACKED    0x80    /* Job is currently tracked      */
#define OPC_UX009_ORPHAN     0x40    /* Orphan job (no operation)     */

#pragma pack()

/*===================================================================
 * OPC/TWS Utility Functions
 *===================================================================*/

/**
 * opc_status_to_char - Convert status code to character
 * @status: Numeric status
 * Returns: Status character
 */
static inline char opc_status_to_char(uint8_t status) {
    switch (status) {
        case 0x01: return 'W';  /* Waiting */
        case 0x02: return 'R';  /* Ready */
        case 0x03: return 'S';  /* Starting */
        case 0x04: return 'A';  /* Active */
        case 0x05: return 'C';  /* Complete */
        case 0x06: return 'E';  /* Error */
        case 0x07: return 'I';  /* Interrupted */
        default:   return '?';
    }
}

/**
 * opc_is_complete - Check if operation is complete
 * @status: Operation status
 * Returns: 1 if complete, 0 otherwise
 */
static inline int opc_is_complete(uint8_t status) {
    return status == OPC_STATUS_COMPLETE;
}

/**
 * opc_is_error - Check if operation ended in error
 * @status: Operation status
 * Returns: 1 if error, 0 otherwise
 */
static inline int opc_is_error(uint8_t status) {
    return status == OPC_STATUS_ERROR;
}

/**
 * opc_is_active - Check if operation is active
 * @status: Operation status
 * Returns: 1 if active, 0 otherwise
 */
static inline int opc_is_active(uint8_t status) {
    return status == OPC_STATUS_ACTIVE ||
           status == OPC_STATUS_STARTING;
}

/**
 * opc_is_critical_path - Check if operation is on critical path
 * @oper: Operation info block
 * Returns: 1 if critical path, 0 otherwise
 */
static inline int opc_is_critical_path(const struct opc_oper *oper) {
    return (oper->op_flags & OPC_OPF_CRITICAL) != 0;
}

/**
 * opc_match_jobname - Check if job name matches pattern
 * @jobname: 8-character job name
 * @pattern: Pattern to match (supports trailing *)
 * @patlen: Pattern length
 * Returns: 1 if match, 0 otherwise
 */
static inline int opc_match_jobname(const char jobname[8],
                                     const char *pattern,
                                     size_t patlen) {
    if (patlen > 0 && pattern[patlen-1] == '*') {
        /* Prefix match */
        return match_prefix(jobname, pattern, patlen - 1);
    }
    /* Exact match (ignoring trailing blanks) */
    for (size_t i = 0; i < 8 && i < patlen; i++) {
        if (jobname[i] != pattern[i]) return 0;
    }
    return 1;
}

/*===================================================================
 * Example Usage
 *===================================================================*

// Event filter exit - filter out certain jobs
#pragma prolog(my_eqqux002,"SAVE(14,12),LR(12,15)")
#pragma epilog(my_eqqux002,"RETURN(14,12)")

int my_eqqux002(struct opc_ux002_parm *parm) {
    // Ignore events from system jobs
    if (opc_match_jobname(parm->ux002jobn, "SYS*", 4)) {
        return OPC_UX002_REJECT;
    }

    return OPC_UX002_ACCEPT;
}

// Job submit exit - add custom JCL
#pragma prolog(my_eqqux003,"SAVE(14,12),LR(12,15)")
#pragma epilog(my_eqqux003,"RETURN(14,12)")

int my_eqqux003(struct opc_ux003_parm *parm) {
    if (parm->ux003func != OPC_UX003_SUBMIT) {
        return OPC_UX003_CONTINUE;
    }

    // Check if critical path operation
    if (opc_is_critical_path(parm->ux003oper)) {
        // Could modify JCL to add priority
        // parm->ux003jcl modifications here
    }

    return OPC_UX003_CONTINUE;
}

// Operation status change exit - notify on errors
#pragma prolog(my_eqqux007,"SAVE(14,12),LR(12,15)")
#pragma epilog(my_eqqux007,"RETURN(14,12)")

int my_eqqux007(struct opc_ux007_parm *parm) {
    // Check for error status
    if (parm->ux007newst == OPC_STATUS_ERROR) {
        // Custom error notification logic
        // send_alert(parm->ux007oper);
    }

    // Check for critical path operations
    struct opc_oper *oper = parm->ux007oper;
    if (opc_is_critical_path(oper) &&
        parm->ux007newst == OPC_STATUS_ERROR) {
        // Critical path failure - escalate
        // escalate_alert(oper);
    }

    return OPC_UX007_CONTINUE;
}

// Job tracking exit - identify orphan jobs
#pragma prolog(my_eqqux009,"SAVE(14,12),LR(12,15)")
#pragma epilog(my_eqqux009,"RETURN(14,12)")

int my_eqqux009(struct opc_ux009_parm *parm) {
    // Check for orphan jobs
    if (parm->ux009flags & OPC_UX009_ORPHAN) {
        // Log orphan job for investigation
        // log_orphan(parm->ux009jobn, parm->ux009jnum);
    }

    return OPC_UX009_CONTINUE;
}

 *===================================================================*/

#endif /* METALC_OPC_H */
