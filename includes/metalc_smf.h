/*********************************************************************
 * METALC_SMF.H - SMF Exit definitions for Metal C
 * 
 * This header provides control block structures, constants, and
 * utility functions for SMF (System Management Facilities) exits.
 * 
 * Supported exits:
 *   IEFU29  - Dump suppression
 *   IEFU83  - SMF record filtering (before write)
 *   IEFU84  - SMF record processing (after write)
 *   IEFU85  - Extended record filtering
 *   IEFACTRT - Job/step accounting
 *   IEFUJI  - Job initiation
 *   IEFUSI  - Step initiation
 * 
 * Usage: 
 *   #include "metalc_base.h"
 *   #include "metalc_smf.h"
 * 
 * Note: Verify offsets against your z/OS release
 *********************************************************************/

#ifndef METALC_SMF_H
#define METALC_SMF_H

#include "metalc_base.h"

/*===================================================================
 * SMF Exit Return Codes
 *===================================================================*/

/* IEFU83/IEFU84/IEFU85 return codes */
#define SMF_RC_WRITE           RC_OK     /* Write the record              */
#define SMF_RC_SUPPRESS        RC_WARNING  /* Suppress (do not write)       */
#define SMF_RC_HALT            RC_ERROR    /* Halt SMF recording            */

/* IEFU29 return codes */
#define IEFU29_RC_ALLOW        RC_OK     /* Allow dump                    */
#define IEFU29_RC_SUPPRESS     RC_WARNING  /* Suppress dump                 */

/* IEFUJI return codes */
#define IEFUJI_RC_CONTINUE     RC_OK     /* Continue job initiation       */
#define IEFUJI_RC_CANCEL       RC_WARNING  /* Cancel the job                */

/* IEFUSI return codes */
#define IEFUSI_RC_CONTINUE     RC_OK     /* Continue step initiation      */
#define IEFUSI_RC_CANCEL       RC_WARNING  /* Cancel the step               */

/*===================================================================
 * SMF Record Types
 *===================================================================*/

#define SMF_TYPE_1             1    /* Wait state/ABEND              */
#define SMF_TYPE_2             2    /* Dump                          */
#define SMF_TYPE_3             3    /* End of memory dump            */
#define SMF_TYPE_4             4    /* Step termination              */
#define SMF_TYPE_5             5    /* Job termination               */
#define SMF_TYPE_6             6    /* Output                        */
#define SMF_TYPE_7             7    /* Data lost/purge               */
#define SMF_TYPE_8             8    /* I/O config                    */
#define SMF_TYPE_14            14   /* Dataset activity (input)      */
#define SMF_TYPE_15            15   /* Dataset activity (output)     */
#define SMF_TYPE_16            16   /* DFSORT statistics             */
#define SMF_TYPE_17            17   /* Scratch data set              */
#define SMF_TYPE_18            18   /* Rename data set               */
#define SMF_TYPE_20            20   /* Job initiation                */
#define SMF_TYPE_21            21   /* Tape mount                    */
#define SMF_TYPE_22            22   /* SMF configuration             */
#define SMF_TYPE_23            23   /* SMF status                    */
#define SMF_TYPE_24            24   /* JES2 spool offload            */
#define SMF_TYPE_25            25   /* JES2 spool restart            */
#define SMF_TYPE_26            26   /* JES2 job purge                */
#define SMF_TYPE_30            30   /* Common address space work     */
#define SMF_TYPE_40            40   /* Storage class memory          */
#define SMF_TYPE_42            42   /* DFSMS statistics              */
#define SMF_TYPE_60            60   /* VSAM activity                 */
#define SMF_TYPE_61            61   /* VVDS update                   */
#define SMF_TYPE_62            62   /* VSAM component open           */
#define SMF_TYPE_64            64   /* VSAM space allocation         */
#define SMF_TYPE_65            65   /* ICF catalog                   */
#define SMF_TYPE_70            70   /* RMF CPU activity              */
#define SMF_TYPE_71            71   /* RMF paging activity           */
#define SMF_TYPE_72            72   /* RMF workload activity         */
#define SMF_TYPE_73            73   /* RMF channel activity          */
#define SMF_TYPE_74            74   /* RMF device activity           */
#define SMF_TYPE_75            75   /* RMF page/swap data set        */
#define SMF_TYPE_76            76   /* RMF trace                     */
#define SMF_TYPE_77            77   /* RMF enqueue                   */
#define SMF_TYPE_78            78   /* RMF virtual storage           */
#define SMF_TYPE_79            79   /* RMF monitor I/O               */
#define SMF_TYPE_80            80   /* RACF processing               */
#define SMF_TYPE_81            81   /* RACF initialization           */
#define SMF_TYPE_83            83   /* RACF data                     */
#define SMF_TYPE_84            84   /* JES3                          */
#define SMF_TYPE_85            85   /* JES3 data                     */
#define SMF_TYPE_89            89   /* Usage data                    */
#define SMF_TYPE_90            90   /* System status                 */
#define SMF_TYPE_92            92   /* File system activity          */
#define SMF_TYPE_99            99   /* Application (user)            */
#define SMF_TYPE_100           100  /* DB2 accounting                */
#define SMF_TYPE_101           101  /* DB2 statistics                */
#define SMF_TYPE_102           102  /* DB2 performance               */
#define SMF_TYPE_110           110  /* CICS                          */
#define SMF_TYPE_119           119  /* TCP/IP                        */
#define SMF_TYPE_120           120  /* FTP/SSH                       */

/*===================================================================
 * SMF Record Header (Standard)
 *===================================================================*/

#pragma pack(1)

/* SMF Record Header - all records start with this */
struct smf_header {
    uint16_t       smflen;           /* +0  Record length             */
    uint16_t       smfseg;           /* +2  Segment descriptor        */
    uint8_t        smfflg;           /* +4  System indicator flags    */
    uint8_t        smfrty;           /* +5  Record type (0-255)       */
    uint32_t       smftme;           /* +6  Time (0.01 sec units)     */
    uint8_t        smfdte[4];        /* +10 Date (0cyydddF packed)    */
    char           smfsid[4];        /* +14 System ID                 */
};                                   /* Total: 18 bytes               */

/* SMFFLG bits */
#define SMFFLG_SUBTYPE   0x80        /* Subtypes present              */
#define SMFFLG_VS2       0x40        /* MVS or VS2                    */
#define SMFFLG_MVS       0x20        /* MVS/SP                        */
#define SMFFLG_INVALID   0x10        /* Record may be invalid         */

/* SMFSEG segment descriptor values */
#define SMFSEG_FIRST     0x0000      /* First/only segment            */
#define SMFSEG_MIDDLE    0x0001      /* Middle segment                */
#define SMFSEG_LAST      0x0002      /* Last segment                  */

/*===================================================================
 * SMF Record Subtypes Header (when SMFFLG_SUBTYPE set)
 *===================================================================*/

struct smf_subtype_header {
    struct smf_header header;        /* +0  Standard header           */
    char           smfssi[4];        /* +18 Subsystem ID              */
    uint16_t       smfsubty;         /* +22 Subtype number            */
};                                   /* Total: 24 bytes               */

/*===================================================================
 * SMF Type 30 - Common Address Space Work Record (partial)
 *===================================================================*/

/* Type 30 subtypes */
#define SMF30_SUBTYPE_1        1     /* Job start                     */
#define SMF30_SUBTYPE_2        2     /* Interval                      */
#define SMF30_SUBTYPE_3        3     /* Step termination              */
#define SMF30_SUBTYPE_4        4     /* Job termination               */
#define SMF30_SUBTYPE_5        5     /* TSO session                   */
#define SMF30_SUBTYPE_6        6     /* APPC/MVS                      */

/* Type 30 record header */
struct smf30_header {
    struct smf_subtype_header hdr;   /* +0  Standard + subtype header */
    /* Self-defining section follows */
    uint16_t       smf30sof;         /* +24 Offset to ID section      */
    uint16_t       smf30sol;         /* +26 Length of ID section      */
    uint16_t       smf30son;         /* +28 Number of ID sections     */
    /* Additional triplets follow for other sections */
};

/* Type 30 Identification Section */
struct smf30_id_section {
    char           smf30jbn[8];      /* +0  Job/session name          */
    char           smf30pgm[8];      /* +8  Program name              */
    char           smf30stm[8];      /* +16 Step name                 */
    char           smf30uif[8];      /* +24 User ID                   */
    char           smf30jnm[8];      /* +32 JES job ID                */
    uint16_t       smf30stn;         /* +40 Step number               */
    char           smf30cls;         /* +42 Job class                 */
    uint8_t        smf30pty;         /* +43 Job priority              */
    /* Additional fields omitted - refer to IBM documentation */
};

/*===================================================================
 * SMF Type 4 - Step Termination (partial)
 *===================================================================*/

struct smf4_record {
    struct smf_header header;        /* +0  Standard header           */
    char           smf4jbn[8];       /* +18 Job name                  */
    uint32_t       smf4rst;          /* +26 Reader start time         */
    uint32_t       smf4rsd;          /* +30 Reader start date         */
    char           smf4uif[8];       /* +34 User identification       */
    char           smf4stm[8];       /* +42 Step name                 */
    /* Additional fields follow */
};

/*===================================================================
 * SMF Type 5 - Job Termination (partial)
 *===================================================================*/

struct smf5_record {
    struct smf_header header;        /* +0  Standard header           */
    char           smf5jbn[8];       /* +18 Job name                  */
    uint32_t       smf5rst;          /* +26 Reader start time         */
    uint32_t       smf5rsd;          /* +30 Reader start date         */
    char           smf5uif[8];       /* +34 User identification       */
    uint16_t       smf5nst;          /* +42 Number of steps           */
    /* Additional fields follow */
};

/*===================================================================
 * SMF Type 14/15 - Dataset Activity (partial)
 *===================================================================*/

struct smf1415_record {
    struct smf_header header;        /* +0  Standard header           */
    char           smf14jbn[8];      /* +18 Job name                  */
    uint32_t       smf14rst;         /* +26 Reader start time         */
    uint32_t       smf14rsd;         /* +30 Reader start date         */
    char           smf14uif[8];      /* +34 User identification       */
    char           smf14dsn[44];     /* +42 Dataset name              */
    /* Additional fields follow */
};

/*===================================================================
 * SMF Type 80 - RACF Processing Record (partial)
 *===================================================================*/

struct smf80_record {
    struct smf_subtype_header hdr;   /* +0  Header with subtype       */
    char           smf80uid[8];      /* +24 User ID                   */
    uint8_t        smf80evn;         /* +32 Event code                */
    uint8_t        smf80evq;         /* +33 Event qualifier           */
    /* Additional fields follow */
};

/* SMF80 event codes */
#define SMF80_EVN_AUTH         1     /* Authorization attempt         */
#define SMF80_EVN_LOGON        2     /* Logon                         */
#define SMF80_EVN_PWD_CHANGE   3     /* Password change               */
#define SMF80_EVN_PROFILE      4     /* Profile access                */

/*===================================================================
 * SMF Exit Parameter Lists
 *===================================================================*/

/* IEFU83/IEFU84 parameter list */
struct smf_exit_parm {
    void          *smf_session;      /* +0  SMF session ID            */
    char           subsys_id[4];     /* +4  Subsystem ID              */
    void          *record_ptr;       /* +8  Pointer to SMF record     */
    int32_t        record_len;       /* +12 Record length             */
    int32_t        return_code;      /* +16 Return code area          */
    uint8_t        flags;            /* +20 Processing flags          */
    uint8_t        reserved[3];      /* +21 Reserved                  */
};

/* IEFU83/IEFU84 flags */
#define SMFX_FLAG_SRB     0x80       /* Running in SRB mode           */
#define SMFX_FLAG_AUTH    0x40       /* Authorized caller             */

/* IEFU29 parameter list */
struct iefu29_parm {
    void          *dump_parm;        /* +0  Dump parameter block      */
    uint8_t        dump_type;        /* +4  Dump type code            */
    uint8_t        flags;            /* +5  Processing flags          */
    uint16_t       reserved;         /* +6  Reserved                  */
    char           jobname[8];       /* +8  Job name                  */
    char           stepname[8];      /* +16 Step name                 */
    uint32_t       abend_code;       /* +24 ABEND code                */
    uint32_t       reason_code;      /* +28 Reason code               */
};

/* IEFU29 dump types */
#define DUMP_TYPE_SYSUDUMP 1         /* SYSUDUMP                      */
#define DUMP_TYPE_SYSABEND 2         /* SYSABEND                      */
#define DUMP_TYPE_SYSMDUMP 3         /* SYSMDUMP                      */
#define DUMP_TYPE_SNAP     4         /* SNAP dump                     */

/* IEFUJI parameter list (Job Initiation) */
struct iefuji_parm {
    void          *jmr;              /* +0  Job management record     */
    void          *intrdr;           /* +4  Internal reader work area */
    char           jobname[8];       /* +8  Job name                  */
    char           jobclass;         /* +16 Job class                 */
    uint8_t        priority;         /* +17 Job priority              */
    uint16_t       reserved;         /* +18 Reserved                  */
    char           programmer[20];   /* +20 Programmer name           */
    char           account[32];      /* +40 Account info              */
};

/* IEFUSI parameter list (Step Initiation) */
struct iefusi_parm {
    void          *jmr;              /* +0  Job management record     */
    char           jobname[8];       /* +4  Job name                  */
    char           stepname[8];      /* +12 Step name                 */
    char           procstep[8];      /* +20 Procedure step            */
    char           pgmname[8];       /* +28 Program name              */
    uint32_t       region_req;       /* +36 Requested region size     */
    uint32_t       region_lim;       /* +40 Region limit              */
    /* Exit can modify region values */
    uint32_t       region_below;     /* +44 Region below 16M (output) */
    uint32_t       region_above;     /* +48 Region above 16M (output) */
};

#pragma pack()

/*===================================================================
 * SMF Utility Functions
 *===================================================================*/

/**
 * smf_get_record_type - Get record type from SMF record
 * @record: Pointer to SMF record
 * Returns: Record type (0-255)
 */
static inline uint8_t smf_get_record_type(const void *record) {
    const struct smf_header *hdr = (const struct smf_header *)record;
    return hdr->smfrty;
}

/**
 * smf_get_record_length - Get record length
 * @record: Pointer to SMF record
 * Returns: Record length in bytes
 */
static inline uint16_t smf_get_record_length(const void *record) {
    const struct smf_header *hdr = (const struct smf_header *)record;
    return hdr->smflen;
}

/**
 * smf_get_subtype - Get subtype for records with subtypes
 * @record: Pointer to SMF record (must have SMFFLG_SUBTYPE set)
 * Returns: Subtype number
 */
static inline uint16_t smf_get_subtype(const void *record) {
    const struct smf_subtype_header *hdr = 
        (const struct smf_subtype_header *)record;
    return hdr->smfsubty;
}

/**
 * smf_has_subtype - Check if record has subtypes
 * @record: Pointer to SMF record
 * Returns: 1 if subtypes present, 0 otherwise
 */
static inline int smf_has_subtype(const void *record) {
    const struct smf_header *hdr = (const struct smf_header *)record;
    return (hdr->smfflg & SMFFLG_SUBTYPE) != 0;
}

/**
 * smf_get_time_seconds - Convert SMF time to seconds since midnight
 * @smf_time: Time in 0.01 second units
 * Returns: Seconds since midnight
 */
static inline uint32_t smf_get_time_seconds(uint32_t smf_time) {
    return smf_time / 100;
}

/**
 * smf_time_in_range - Check if time is within a range
 * @smf_time: Time to check (0.01 sec units)
 * @start:    Range start (0.01 sec units)
 * @end:      Range end (0.01 sec units)
 * Returns: 1 if in range, 0 otherwise
 */
static inline int smf_time_in_range(uint32_t smf_time, 
                                     uint32_t start, uint32_t end) {
    if (start <= end) {
        /* Normal range (e.g., 9 AM to 5 PM) */
        return (smf_time >= start && smf_time <= end);
    } else {
        /* Overnight range (e.g., 10 PM to 6 AM) */
        return (smf_time >= start || smf_time <= end);
    }
}

/**
 * smf_match_prefix - Check if job name matches a prefix
 * @jobname: 8-character job name (blank padded)
 * @prefix:  Prefix to match (null-terminated)
 * @prefix_len: Length of prefix
 * Returns: 1 if matches, 0 otherwise
 */
static inline int smf_match_prefix(const char jobname[8], 
                                    const char *prefix, 
                                    size_t prefix_len) {
    return match_prefix(jobname, prefix, prefix_len);
}

/*===================================================================
 * Example Usage
 *===================================================================*

// Filter type 42 records - use explicit prolog/epilog pragmas
#pragma prolog(my_iefu83,"STM(14,12,12(13)),LR(12,15)")
#pragma epilog(my_iefu83,"LM(14,12,12(13)),BR(14)")

int my_iefu83(struct smf_exit_parm *parm) {
    struct smf_header *record = (struct smf_header *)parm->record_ptr;

    if (record->smfrty == SMF_TYPE_42) {
        return SMF_RC_SUPPRESS;
    }

    return SMF_RC_WRITE;
}

// Suppress SYS* job dumps
#pragma prolog(my_iefu29,"STM(14,12,12(13)),LR(12,15)")
#pragma epilog(my_iefu29,"LM(14,12,12(13)),BR(14)")

int my_iefu29(struct iefu29_parm *parm) {
    if (smf_match_prefix(parm->jobname, "SYS", 3)) {
        return IEFU29_RC_SUPPRESS;
    }
    return IEFU29_RC_ALLOW;
}

 *===================================================================*/

#endif /* METALC_SMF_H */
