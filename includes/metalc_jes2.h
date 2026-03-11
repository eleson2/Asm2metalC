/*********************************************************************
 * METALC_JES2.H - JES2 Exit definitions for Metal C
 * 
 * This header provides control block structures, constants, and
 * utility functions for JES2 exit development.
 * 
 * Supports both legacy ($EXIT) and dynamic exit interfaces.
 * 
 * Covered exit points:
 *   Exit 1  - Job select
 *   Exit 2  - Job purge
 *   Exit 3  - JCL scan
 *   Exit 4  - Output processing
 *   Exit 5  - TSO submit
 *   Exit 6  - Print/punch
 *   Exit 7  - Converter
 *   Exit 8  - JOB statement
 *   Exit 20 - END statement
 *   Exit 50 - SSI request
 *   Exit 51 - NJE processing
 * 
 * Usage: 
 *   #include "metalc_base.h"
 *   #include "metalc_jes2.h"
 * 
 * IMPORTANT: Control block layouts vary significantly by z/OS release.
 *            Verify offsets against your installation's $JCT, $JQE, etc.
 *********************************************************************/

#ifndef METALC_JES2_H
#define METALC_JES2_H

#include "metalc_base.h"

/*===================================================================
 * JES2 Exit Return Codes
 *===================================================================*/

/* Standard exit return codes */
#define JES2_RC_CONTINUE       RC_OK     /* Continue normal processing    */
#define JES2_RC_SKIP           RC_WARNING  /* Skip current function         */
#define JES2_RC_FAIL           RC_ERROR    /* Fail the job                  */
#define JES2_RC_BYPASS         RC_SEVERE   /* Bypass exit point             */
#define JES2_RC_TERMINATE      RC_CRITICAL /* Terminate JES2 (use caution!) */

/* Exit 1 (Job Select) specific codes */
#define EXIT1_SELECT           0     /* Select job for execution      */
#define EXIT1_DEFER            4     /* Defer job selection           */
#define EXIT1_REJECT           8     /* Reject job                    */

/* Exit 2 (Job Purge) specific codes */
#define EXIT2_PURGE            0     /* Allow purge                   */
#define EXIT2_RETAIN           4     /* Retain job                    */

/* Exit 4 (Output) specific codes */
#define EXIT4_PROCESS          0     /* Process output normally       */
#define EXIT4_REROUTE          4     /* Reroute output                */
#define EXIT4_DELETE           8     /* Delete output                 */

/* Exit 8 (JOB Statement) specific codes */
#define EXIT8_ACCEPT           0     /* Accept JOB statement          */
#define EXIT8_MODIFY           4     /* Statement modified            */
#define EXIT8_REJECT           8     /* Reject job                    */

/*===================================================================
 * JES2 Job Classes
 *===================================================================*/

#define JES2_CLASS_A           'A'
#define JES2_CLASS_B           'B'
#define JES2_CLASS_C           'C'
#define JES2_CLASS_D           'D'
#define JES2_CLASS_E           'E'
#define JES2_CLASS_F           'F'
#define JES2_CLASS_G           'G'
#define JES2_CLASS_H           'H'
#define JES2_CLASS_I           'I'
#define JES2_CLASS_J           'J'
#define JES2_CLASS_K           'K'
#define JES2_CLASS_L           'L'
#define JES2_CLASS_M           'M'
#define JES2_CLASS_N           'N'
#define JES2_CLASS_O           'O'
#define JES2_CLASS_P           'P'
#define JES2_CLASS_Q           'Q'
#define JES2_CLASS_R           'R'
#define JES2_CLASS_S           'S'
#define JES2_CLASS_T           'T'
#define JES2_CLASS_U           'U'
#define JES2_CLASS_V           'V'
#define JES2_CLASS_W           'W'
#define JES2_CLASS_X           'X'
#define JES2_CLASS_Y           'Y'
#define JES2_CLASS_Z           'Z'
#define JES2_CLASS_0           '0'
#define JES2_CLASS_1           '1'
#define JES2_CLASS_2           '2'
#define JES2_CLASS_3           '3'
#define JES2_CLASS_4           '4'
#define JES2_CLASS_5           '5'
#define JES2_CLASS_6           '6'
#define JES2_CLASS_7           '7'
#define JES2_CLASS_8           '8'
#define JES2_CLASS_9           '9'

/*===================================================================
 * JES2 Queue Types
 *===================================================================*/

#define JQE_QUEUE_INPUT        1     /* Input queue                   */
#define JQE_QUEUE_EXECUTE      2     /* Execution queue               */
#define JQE_QUEUE_OUTPUT       3     /* Output queue                  */
#define JQE_QUEUE_HARDCOPY     4     /* Hardcopy queue                */
#define JQE_QUEUE_PURGE        5     /* Purge queue                   */
#define JQE_QUEUE_HELD         6     /* Held queue                    */
#define JQE_QUEUE_CONV         7     /* Conversion queue              */

/*===================================================================
 * JCT - Job Control Table
 *===================================================================*/

#pragma pack(1)

struct jct {
    char           jctid[4];         /* +0   'JCT ' identifier        */
    uint16_t       jctjobid;         /* +4   JES2 job number          */
    char           jctjname[8];      /* +6   Job name                 */
    char           jctjclas;         /* +14  Job class                */
    uint8_t        jctprio;          /* +15  Selection priority (0-15)*/
    char           jctmclas;         /* +16  Message class            */
    char           jctroute[8];      /* +17  Execution routing        */
    uint8_t        _filler1[2];      /* +25  Filler                   */
    uint8_t        jctflg1;          /* +27  Flag byte 1              */
    uint8_t        jctflg2;          /* +28  Flag byte 2              */
    uint8_t        jctflg3;          /* +29  Flag byte 3              */
    uint8_t        jctflg4;          /* +30  Flag byte 4              */
    uint8_t        _filler2;         /* +31  Filler                   */
    char           jctpname[20];     /* +32  Programmer name          */
    char           jctacct[32];      /* +52  Account information      */
    char           jcttsuid[8];      /* +84  TSO user ID              */
    char           jctgroup[8];      /* +92  Security group           */
    char           jctnnode[8];      /* +100 Notify node              */
    char           jctnuser[8];      /* +108 Notify user              */
    uint32_t       jctsubsm;         /* +116 Submit time              */
    uint32_t       jctsubdt;         /* +120 Submit date              */
    uint32_t       jctstrte;         /* +124 Start time               */
    uint32_t       jctstrtd;         /* +128 Start date               */
    uint32_t       jctendtm;         /* +132 End time                 */
    uint32_t       jctenddt;         /* +136 End date                 */
    uint16_t       jctnstep;         /* +140 Number of steps          */
    uint16_t       jctestep;         /* +142 Executing step number    */
    int32_t        jctmxrc;          /* +144 Maximum return code      */
    char           jctabcod[4];      /* +148 ABEND code               */
    uint32_t       jctlines;         /* +152 Lines printed            */
    uint32_t       jctpages;         /* +156 Pages printed            */
    uint32_t       jctcards;         /* +160 Cards punched            */
    /* Note: Actual JCT is much larger; offsets vary by z/OS release */
    /* Add additional fields as needed for your z/OS version */
};                                   /* Minimum shown: 164 bytes      */

/* JCTFLG1 bits */
#define JCTFLG1_HELD     0x80        /* Job is held                   */
#define JCTFLG1_TSU      0x40        /* TSO user                      */
#define JCTFLG1_STC      0x20        /* Started task                  */
#define JCTFLG1_BATCH    0x10        /* Batch job                     */
#define JCTFLG1_APPC     0x08        /* APPC job                      */
#define JCTFLG1_SYSTEM   0x04        /* System job                    */

/* JCTFLG2 bits */
#define JCTFLG2_NJE      0x80        /* NJE job                       */
#define JCTFLG2_SPIN     0x40        /* SPIN=YES specified            */
#define JCTFLG2_RESTART  0x20        /* Restart job                   */
#define JCTFLG2_CANCEL   0x10        /* Job was cancelled             */

/* JCTFLG3 bits */
#define JCTFLG3_TYPRUN   0x80        /* TYPRUN specified              */
#define JCTFLG3_HOLD     0x40        /* HOLD specified                */
#define JCTFLG3_JCLHOLD  0x20        /* JCL hold                      */
#define JCTFLG3_DUPJOB   0x10        /* Duplicate job name allowed    */

#pragma pack()

/*===================================================================
 * JQE - Job Queue Element
 *===================================================================*/

#pragma pack(1)

struct jqe {
    char           jqeid[4];         /* +0   'JQE ' identifier        */
    uint16_t       jqejobid;         /* +4   JES2 job number          */
    uint8_t        jqetype;          /* +6   Queue type               */
    uint8_t        jqeprio;          /* +7   Queue priority           */
    uint8_t        jqeflg1;          /* +8   Flag byte 1              */
    uint8_t        jqeflg2;          /* +9   Flag byte 2              */
    uint8_t        jqeflg3;          /* +10  Flag byte 3              */
    uint8_t        jqeflg4;          /* +11  Flag byte 4              */
    void          *jqejct;           /* +12  Pointer to JCT           */
    void          *jqenext;          /* +16  Next JQE in queue        */
    void          *jqeprev;          /* +20  Previous JQE in queue    */
    uint32_t       jqeqtime;         /* +24  Time placed on queue     */
    uint32_t       jqeqdate;         /* +28  Date placed on queue     */
    char           jqejname[8];      /* +32  Job name (cached)        */
    char           jqejclas;         /* +40  Job class (cached)       */
    uint8_t        _reserved[3];     /* +41  Reserved                 */
};                                   /* Total: 44 bytes (verify)      */

/* JQEFLG1 bits */
#define JQEFLG1_HELD     0x80        /* Queue entry held              */
#define JQEFLG1_ACTIVE   0x40        /* Currently being processed     */
#define JQEFLG1_PURGE    0x20        /* Marked for purge              */
#define JQEFLG1_SPIN     0x10        /* Spin dataset                  */

/* JQEFLG2 bits */
#define JQEFLG2_OPERATOR 0x80        /* Operator hold                 */
#define JQEFLG2_SYSTEM   0x40        /* System hold                   */
#define JQEFLG2_DUPLEX   0x20        /* Duplex printing               */

#pragma pack()

/*===================================================================
 * PCE - Processor Control Element (simplified)
 *===================================================================*/

#pragma pack(1)

struct pce {
    char           pceid[4];         /* +0   'PCE ' identifier        */
    uint8_t        pcetype;          /* +4   Processor type           */
    uint8_t        pceflags;         /* +5   Status flags             */
    uint16_t       _reserved;        /* +6   Reserved                 */
    void          *pcejct;           /* +8   Current JCT pointer      */
    void          *pcejqe;           /* +12  Current JQE pointer      */
    void          *pceanchr;         /* +16  JES2 anchor              */
    char           pcework[256];     /* +20  Work area (size varies)  */
};

/* PCE types */
#define PCE_TYPE_INIT    1           /* Initiator                     */
#define PCE_TYPE_INPUT   2           /* Input processor               */
#define PCE_TYPE_OUTPUT  3           /* Output processor              */
#define PCE_TYPE_PURGE   4           /* Purge processor               */

#pragma pack()

/*===================================================================
 * Dynamic Exit Parameter Structures
 *===================================================================*/

#pragma pack(1)

/* Common exit parameter list (XPL) header */
struct jes2_xpl {
    void          *xplxrt;           /* +0   Exit routine table       */
    uint32_t       xplexitp;         /* +4   Exit point number        */
    uint32_t       xplsubpt;         /* +8   Sub-exit point           */
    void          *xplwork;          /* +12  Exit work area           */
    uint32_t       xplwsize;         /* +16  Work area size           */
    void          *xpljct;           /* +20  JCT pointer              */
    void          *xpljqe;           /* +24  JQE pointer              */
    void          *xplpce;           /* +28  PCE pointer              */
    uint32_t       xplflags;         /* +32  Processing flags         */
    uint32_t       xplrc;            /* +36  Return code area         */
    /* Exit-specific parameters follow */
};

/* XPLFLAGS bits */
#define XPLF_INIT        0x80000000  /* Initialization call           */
#define XPLF_TERM        0x40000000  /* Termination call              */
#define XPLF_RESTART     0x20000000  /* Restart in progress           */

/* Exit 1 - Job Select parameters */
struct jes2_exit1_parm {
    struct jes2_xpl base;            /* Common parameters             */
    void          *e1jct;            /* +40  JCT pointer              */
    void          *e1jqe;            /* +44  JQE pointer              */
    uint8_t        e1flags;          /* +48  Exit 1 flags             */
    uint8_t        _reserved[3];     /* +49  Reserved                 */
};

/* Exit 4 - Output processing parameters */
struct jes2_exit4_parm {
    struct jes2_xpl base;            /* Common parameters             */
    void          *e4jct;            /* +40  JCT pointer              */
    void          *e4jqe;            /* +44  JQE pointer              */
    void          *e4jds;            /* +48  JDS (Job Data Set) ptr   */
    char          *e4dest;           /* +52  Destination pointer      */
    uint16_t       e4destl;          /* +56  Destination length       */
    uint8_t        e4flags;          /* +58  Exit 4 flags             */
    uint8_t        _reserved;        /* +59  Reserved                 */
};

/* Exit 8 - JOB statement parameters */
struct jes2_exit8_parm {
    struct jes2_xpl base;            /* Common parameters             */
    void          *e8jct;            /* +40  JCT pointer              */
    char          *e8stmt;           /* +44  JOB statement text       */
    uint16_t       e8stmlen;         /* +48  Statement length         */
    uint8_t        e8flags;          /* +50  Exit 8 flags             */
    uint8_t        e8card;           /* +51  Card number              */
    char          *e8errmsg;         /* +52  Error message area       */
    uint16_t       e8errlen;         /* +56  Error message length     */
    uint16_t       _reserved;        /* +58  Reserved                 */
};

/* E8FLAGS bits */
#define E8F_CONTINUE     0x80        /* Continuation card             */
#define E8F_SCANNED      0x40        /* Statement already scanned     */
#define E8F_MODIFIED     0x20        /* Statement was modified        */

/* Exit 20 - END statement parameters */
struct jes2_exit20_parm {
    struct jes2_xpl base;            /* Common parameters             */
    void          *e20jct;           /* +40  JCT pointer              */
    uint32_t       e20cpu;           /* +44  CPU time used (seconds)  */
    uint32_t       e20lines;         /* +48  Lines printed            */
    uint32_t       e20pages;         /* +52  Pages printed            */
    uint32_t       e20cards;         /* +56  Cards punched            */
    int32_t        e20maxrc;         /* +60  Maximum return code      */
};

#pragma pack()

/*===================================================================
 * JES2 Utility Functions
 *===================================================================*/

/**
 * jes2_is_batch - Check if job is a batch job
 * @jct: Pointer to JCT
 * Returns: 1 if batch job, 0 otherwise
 */
static inline int jes2_is_batch(const struct jct *jct) {
    return (jct->jctflg1 & JCTFLG1_BATCH) != 0;
}

/**
 * jes2_is_tso - Check if job is TSO session
 * @jct: Pointer to JCT
 * Returns: 1 if TSO, 0 otherwise
 */
static inline int jes2_is_tso(const struct jct *jct) {
    return (jct->jctflg1 & JCTFLG1_TSU) != 0;
}

/**
 * jes2_is_stc - Check if job is started task
 * @jct: Pointer to JCT
 * Returns: 1 if STC, 0 otherwise
 */
static inline int jes2_is_stc(const struct jct *jct) {
    return (jct->jctflg1 & JCTFLG1_STC) != 0;
}

/**
 * jes2_is_held - Check if job is held
 * @jct: Pointer to JCT
 * Returns: 1 if held, 0 otherwise
 */
static inline int jes2_is_held(const struct jct *jct) {
    return (jct->jctflg1 & JCTFLG1_HELD) != 0;
}

/**
 * jes2_is_nje - Check if job is NJE (network job)
 * @jct: Pointer to JCT
 * Returns: 1 if NJE, 0 otherwise
 */
static inline int jes2_is_nje(const struct jct *jct) {
    return (jct->jctflg2 & JCTFLG2_NJE) != 0;
}

/**
 * jes2_is_system_job - Check if job is system job
 * @jct: Pointer to JCT
 * Returns: 1 if system job, 0 otherwise
 */
static inline int jes2_is_system_job(const struct jct *jct) {
    return (jct->jctflg1 & JCTFLG1_SYSTEM) != 0;
}

/**
 * jes2_get_job_type - Get job type string
 * @jct: Pointer to JCT
 * Returns: Static string describing job type
 */
static inline const char *jes2_get_job_type(const struct jct *jct) {
    if (jct->jctflg1 & JCTFLG1_STC)   return "STC";
    if (jct->jctflg1 & JCTFLG1_TSU)   return "TSO";
    if (jct->jctflg1 & JCTFLG1_BATCH) return "JOB";
    return "UNK";
}

/**
 * jes2_match_jobname_prefix - Check job name prefix
 * @jct: Pointer to JCT
 * @prefix: Prefix to match (null-terminated)
 * @len: Length of prefix
 * Returns: 1 if matches, 0 otherwise
 */
static inline int jes2_match_jobname_prefix(const struct jct *jct,
                                             const char *prefix,
                                             size_t len) {
    return match_prefix(jct->jctjname, prefix, len);
}

/**
 * jes2_set_hold - Set job hold flag
 * @jct: Pointer to JCT
 */
static inline void jes2_set_hold(struct jct *jct) {
    jct->jctflg1 |= JCTFLG1_HELD;
}

/**
 * jes2_clear_hold - Clear job hold flag
 * @jct: Pointer to JCT
 */
static inline void jes2_clear_hold(struct jct *jct) {
    jct->jctflg1 &= ~JCTFLG1_HELD;
}

/**
 * jes2_set_priority - Set job priority
 * @jct: Pointer to JCT
 * @priority: Priority value (0-15)
 */
static inline void jes2_set_priority(struct jct *jct, uint8_t priority) {
    if (priority > 15) priority = 15;
    jct->jctprio = priority;
}

/**
 * jes2_set_class - Set job class
 * @jct: Pointer to JCT
 * @jobclass: Class character (A-Z, 0-9)
 */
static inline void jes2_set_class(struct jct *jct, char jobclass) {
    jct->jctjclas = jobclass;
}

/**
 * jes2_set_msgclass - Set message class
 * @jct: Pointer to JCT
 * @msgclass: Message class character
 */
static inline void jes2_set_msgclass(struct jct *jct, char msgclass) {
    jct->jctmclas = msgclass;
}

/*===================================================================
 * JES2 Job Naming Validation
 *===================================================================*/

/**
 * jes2_validate_jobname - Validate job name format
 * @name: 8-character job name
 * Returns: 1 if valid, 0 if invalid
 * 
 * Rules: First character must be A-Z or @#$
 *        Remaining can be A-Z, 0-9, or @#$
 */
static inline int jes2_validate_jobname(const char name[8]) {
    /* First character */
    char c = name[0];
    if (!((c >= 'A' && c <= 'Z') || c == '@' || c == '#' || c == '$')) {
        return 0;
    }
    
    /* Remaining characters */
    for (int i = 1; i < 8; i++) {
        c = name[i];
        if (c == ' ') break;  /* Trailing blanks OK */
        if (!((c >= 'A' && c <= 'Z') || 
              (c >= '0' && c <= '9') ||
              c == '@' || c == '#' || c == '$')) {
            return 0;
        }
    }
    
    return 1;
}

/**
 * jes2_enforce_naming_convention - Check custom naming convention
 * @name: 8-character job name
 * @prefixes: Array of allowed prefixes
 * @num_prefixes: Number of prefixes
 * @prefix_len: Length of each prefix
 * Returns: 1 if matches convention, 0 otherwise
 */
static inline int jes2_enforce_naming_convention(const char name[8],
                                                  const char **prefixes,
                                                  int num_prefixes,
                                                  size_t prefix_len) {
    for (int i = 0; i < num_prefixes; i++) {
        if (match_prefix(name, prefixes[i], prefix_len)) {
            return 1;
        }
    }
    return 0;
}

/*===================================================================
 * Example Usage
 *===================================================================*

// Exit 1 - Job selection based on class
// Use explicit prolog/epilog pragmas for clarity
#pragma prolog(my_exit1,"SAVE(14,12),LR(12,15)")
#pragma epilog(my_exit1,"RETURN(14,12)")

int my_exit1(struct jes2_exit1_parm *parm) {
    struct jct *jct = (struct jct *)parm->e1jct;

    // Defer class Z jobs during business hours
    if (jct->jctjclas == 'Z') {
        // Would check time here
        return EXIT1_DEFER;
    }

    // Skip system jobs
    if (jes2_is_system_job(jct)) {
        return EXIT1_SELECT;
    }

    return EXIT1_SELECT;
}

// Exit 8 - Enforce job naming convention
#pragma prolog(my_exit8,"SAVE(14,12),LR(12,15)")
#pragma epilog(my_exit8,"RETURN(14,12)")

int my_exit8(struct jes2_exit8_parm *parm) {
    struct jct *jct = (struct jct *)parm->e8jct;

    // Allow STC and TSO without restriction
    if (jes2_is_stc(jct) || jes2_is_tso(jct)) {
        return EXIT8_ACCEPT;
    }

    // Enforce naming convention for batch
    static const char *valid_prefixes[] = {"PAY", "FIN", "HR"};
    if (!jes2_enforce_naming_convention(jct->jctjname,
                                         valid_prefixes, 3, 3)) {
        return EXIT8_REJECT;
    }

    return EXIT8_ACCEPT;
}

// Exit 20 - Log resource usage
#pragma prolog(my_exit20,"SAVE(14,12),LR(12,15)")
#pragma epilog(my_exit20,"RETURN(14,12)")

int my_exit20(struct jes2_exit20_parm *parm) {
    struct jct *jct = (struct jct *)parm->e20jct;

    // Log jobs exceeding resource thresholds
    if (parm->e20cpu > 3600 || parm->e20lines > 1000000) {
        // log_resource_usage(jct, parm);
    }

    return JES2_RC_CONTINUE;
}

 *===================================================================*/

#endif /* METALC_JES2_H */
