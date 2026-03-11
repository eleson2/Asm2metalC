/*********************************************************************
 * METALC_IMS.H - IMS Exit definitions for Metal C
 *
 * This header provides control block structures, constants, and
 * utility functions for IMS exit development.
 *
 * Supported exits:
 *   System exits:
 *     DFSCMPX0 - Conversation Message exit
 *     DFSCTRN0 - Transaction code edit exit
 *     DFSFEBJ0 - Front-end switch exit
 *     DFSFLGX0 - Logger exit
 *     DFSINSX0 - Input/output edit exit
 *     DFSINTX0 - Initialization exit
 *     DFSMSCE0 - Message control/format exit
 *     DFSNPRT0 - Non-printable character exit
 *     DFSSGUX0 - Sign-on/security exit
 *     DFSTXIT0 - Transaction exit
 *
 *   User exits:
 *     DFSME000 - Message edit exit
 *     DFSBSEX0 - BMP scheduling exit
 *
 * Usage:
 *   #include "metalc_base.h"
 *   #include "metalc_ims.h"
 *
 * IMPORTANT: Control block layouts vary by IMS version.
 *            Verify offsets against your installation.
 *********************************************************************/

#ifndef METALC_IMS_H
#define METALC_IMS_H

#include "metalc_base.h"

/*===================================================================
 * IMS Exit Return Codes
 *===================================================================*/

/* Common exit return codes */
#define IMS_RC_CONTINUE        RC_OK     /* Continue processing           */
#define IMS_RC_REJECT          RC_WARNING  /* Reject/fail request           */
#define IMS_RC_MODIFIED        RC_ERROR    /* Data was modified             */
#define IMS_RC_ABORT           RC_SEVERE   /* Abort processing              */
#define IMS_RC_TERMINATE       RC_CRITICAL /* Terminate IMS                 */

/* Sign-on exit (DFSSGUX0) return codes */
#define IMS_SGNX_ALLOW         RC_OK     /* Allow sign-on                 */
#define IMS_SGNX_REJECT        RC_WARNING  /* Reject sign-on                */
#define IMS_SGNX_DEFER         RC_ERROR    /* Defer to RACF/ACF2            */
#define IMS_SGNX_REVOKE        RC_SEVERE   /* Revoke user                   */

/* Transaction exit (DFSTXIT0) return codes */
#define IMS_TXIT_CONTINUE      RC_OK     /* Continue with transaction     */
#define IMS_TXIT_REJECT        RC_WARNING  /* Reject transaction            */
#define IMS_TXIT_MODIFIED      RC_ERROR    /* Transaction modified          */
#define IMS_TXIT_REROUTE       RC_SEVERE   /* Reroute transaction           */

/* Logger exit (DFSFLGX0) return codes */
#define IMS_FLGX_CONTINUE      RC_OK     /* Continue logging              */
#define IMS_FLGX_SUPPRESS      RC_WARNING  /* Suppress log record           */
#define IMS_FLGX_MODIFIED      RC_ERROR    /* Log record modified           */

/* Message edit exit (DFSME000) return codes */
#define IMS_ME_CONTINUE        RC_OK     /* Continue processing           */
#define IMS_ME_REJECT          RC_WARNING  /* Reject message                */
#define IMS_ME_MODIFIED        RC_ERROR    /* Message modified              */

/* BMP scheduling exit (DFSBSEX0) return codes */
#define IMS_BSEX_SCHEDULE      RC_OK     /* Schedule the BMP              */
#define IMS_BSEX_DEFER         RC_WARNING  /* Defer scheduling              */
#define IMS_BSEX_REJECT        RC_ERROR    /* Reject scheduling             */

/*===================================================================
 * IMS Status Codes
 *===================================================================*/

/* Common DL/I status codes */
#define IMS_STAT_OK            "  "  /* Successful (blank)            */
#define IMS_STAT_AC            "AC"  /* ACB not open                  */
#define IMS_STAT_AD            "AD"  /* Dead letter queue full        */
#define IMS_STAT_AK            "AK"  /* Invalid PSB name              */
#define IMS_STAT_AM            "AM"  /* PSB not found                 */
#define IMS_STAT_AO            "AO"  /* I/O error                     */
#define IMS_STAT_GA            "GA"  /* Unqualified GU                */
#define IMS_STAT_GB            "GB"  /* End of database               */
#define IMS_STAT_GC            "GC"  /* End of segment                */
#define IMS_STAT_GD            "GD"  /* No more databases             */
#define IMS_STAT_GE            "GE"  /* Segment not found             */
#define IMS_STAT_GK            "GK"  /* Invalid segment type          */
#define IMS_STAT_II            "II"  /* Insert failed                 */
#define IMS_STAT_NI            "NI"  /* No space for insert           */
#define IMS_STAT_QC            "QC"  /* No more messages              */
#define IMS_STAT_QD            "QD"  /* Destination not valid         */
#define IMS_STAT_QE            "QE"  /* Message too long              */
#define IMS_STAT_XA            "XA"  /* External subsys error         */

/*===================================================================
 * IMS Call Types
 *===================================================================*/

/* DL/I database calls */
#define IMS_CALL_GU            "GU  "  /* Get Unique                  */
#define IMS_CALL_GN            "GN  "  /* Get Next                    */
#define IMS_CALL_GNP           "GNP "  /* Get Next within Parent      */
#define IMS_CALL_GHU           "GHU "  /* Get Hold Unique             */
#define IMS_CALL_GHN           "GHN "  /* Get Hold Next               */
#define IMS_CALL_GHNP          "GHNP"  /* Get Hold Next within Parent */
#define IMS_CALL_ISRT          "ISRT"  /* Insert                      */
#define IMS_CALL_REPL          "REPL"  /* Replace                     */
#define IMS_CALL_DLET          "DLET"  /* Delete                      */

/* Message queue calls */
#define IMS_CALL_GU_MSG        "GU  "  /* Get Unique (message)        */
#define IMS_CALL_GN_MSG        "GN  "  /* Get Next (message)          */
#define IMS_CALL_ISRT_MSG      "ISRT"  /* Insert (message)            */
#define IMS_CALL_PURG          "PURG"  /* Purge                       */
#define IMS_CALL_CHNG          "CHNG"  /* Change                      */

/* System service calls */
#define IMS_CALL_PCB           "PCB "  /* Schedule PSB                */
#define IMS_CALL_TERM          "TERM"  /* Terminate PSB               */
#define IMS_CALL_CHKP          "CHKP"  /* Checkpoint                  */
#define IMS_CALL_SYNC          "SYNC"  /* Syncpoint                   */
#define IMS_CALL_ROLB          "ROLB"  /* Rollback                    */
#define IMS_CALL_ROLL          "ROLL"  /* Roll (abend)                */

/*===================================================================
 * IMS Transaction Types
 *===================================================================*/

#define IMS_TRAN_IFP           0x01  /* IMS Fast Path                 */
#define IMS_TRAN_MPP           0x02  /* Message Processing Program    */
#define IMS_TRAN_BMP           0x03  /* Batch Message Processing      */
#define IMS_TRAN_CONV          0x04  /* Conversational               */
#define IMS_TRAN_NONCONV       0x05  /* Non-conversational           */

/*===================================================================
 * PCB - Program Communication Block
 *===================================================================*/

#pragma pack(1)

/* I/O PCB - for message processing */
struct iopcb {
    char           lterm[8];         /* +0   Logical terminal name    */
    char           _reserved1[2];    /* +8   Reserved                 */
    char           status[2];        /* +10  Status code              */
    uint32_t       msgdate;          /* +12  Message date             */
    uint32_t       msgtime;          /* +16  Message time             */
    uint16_t       msginlen;         /* +20  Input message length     */
    uint16_t       msgseqno;         /* +22  Message sequence number  */
    char           modname[8];       /* +24  MID name                 */
    char           userid[8];        /* +32  User ID                  */
    char           groupid[8];       /* +40  Group ID                 */
    char           _reserved2[12];   /* +48  Reserved                 */
};                                   /* Total: 60 bytes               */

/* DB PCB - for database access */
struct dbpcb {
    char           dbdname[8];       /* +0   DBD name                 */
    char           seglevel[2];      /* +8   Segment level number     */
    char           status[2];        /* +10  Status code              */
    char           procopt[4];       /* +12  Processing options       */
    uint32_t       reserved;         /* +16  Reserved                 */
    char           segname[8];       /* +20  Segment name             */
    uint32_t       keyfdbk;          /* +28  Key feedback length      */
    uint32_t       sensegcnt;        /* +32  Sensitive segment count  */
    char           keyarea[256];     /* +36  Key feedback area        */
};                                   /* Total: 292 bytes              */

#pragma pack()

/*===================================================================
 * SCD - Scratch Pad Area / Communication Block
 *===================================================================*/

#pragma pack(1)

struct imsscd {
    char           scdtran[8];       /* +0   Transaction code         */
    char           scdpsb[8];        /* +8   PSB name                 */
    char           scdlterm[8];      /* +16  Logical terminal name    */
    char           scduser[8];       /* +24  User ID                  */
    char           scdgroup[8];      /* +32  Group ID                 */
    uint8_t        scdflags;         /* +40  Flags                    */
    uint8_t        scdtype;          /* +41  Transaction type         */
    uint16_t       scdprio;          /* +42  Transaction priority     */
    uint32_t       scddate;          /* +44  Date (Julian)            */
    uint32_t       scdtime;          /* +48  Time (packed)            */
    uint32_t       scdcpuid;         /* +52  CPU ID                   */
    void          *scdiopcb;         /* +56  I/O PCB address          */
    void          *scdpsba;          /* +60  PSB address              */
};                                   /* Total: 64 bytes               */

/* SCDFLAGS bits */
#define SCD_FLG_CONV     0x80        /* Conversational transaction    */
#define SCD_FLG_RESP     0x40        /* Response mode                 */
#define SCD_FLG_FAST     0x20        /* Fast Path                     */
#define SCD_FLG_LOCAL    0x10        /* Local transaction             */
#define SCD_FLG_REMOTE   0x08        /* Remote transaction            */

#pragma pack()

/*===================================================================
 * Sign-on Exit Parameter List (DFSSGUX0)
 *===================================================================*/

#pragma pack(1)

struct ims_sgnx_parm {
    EXIT_PARM_HEADER;                /* +0   Common header            */
    char           sgxuser[8];       /* +8   User ID                  */
    char           sgxpass[8];       /* +16  Password (may be masked) */
    char           sgxnpass[8];      /* +24  New password             */
    char           sgxgroup[8];      /* +32  Group ID                 */
    char           sgxlterm[8];      /* +40  Logical terminal         */
    char           sgxpterm[8];      /* +48  Physical terminal        */
    uint32_t       sgxreasn;         /* +56  Reason code (output)     */
    char          *sgxmsg;           /* +60  Message area             */
    uint16_t       sgxmsgln;         /* +64  Message length           */
    uint16_t       _reserved2;       /* +66  Reserved                 */
};                                   /* Total: 68 bytes               */

/* SGXFUNC function codes */
#define SGX_FUNC_SIGNON  0x01        /* Sign-on request               */
#define SGX_FUNC_SIGNOFF 0x02        /* Sign-off request              */
#define SGX_FUNC_VERIFY  0x03        /* Verification only             */
#define SGX_FUNC_PWCHG   0x04        /* Password change               */

/* SGXFLAGS bits */
#define SGX_FLG_RACF     0x80        /* RACF/ACF2 available           */
#define SGX_FLG_ESAS     0x40        /* Extended security             */
#define SGX_FLG_CONV     0x20        /* Conversational sign-on        */

#pragma pack()

/*===================================================================
 * Transaction Exit Parameter List (DFSTXIT0)
 *===================================================================*/

#pragma pack(1)

struct ims_txit_parm {
    EXIT_PARM_HEADER;                /* +0   Common header            */
    char           txittran[8];      /* +8   Transaction code         */
    char           txituser[8];      /* +16  User ID                  */
    char           txitlterm[8];     /* +24  Logical terminal         */
    char           txitpsb[8];       /* +32  PSB name                 */
    void          *txitmsg;          /* +40  Message pointer          */
    uint32_t       txitmlen;         /* +44  Message length           */
    uint32_t       txitreasn;        /* +48  Reason code (output)     */
    char           txitntran[8];     /* +52  New transaction (reroute)*/
};                                   /* Total: 60 bytes               */

/* TXITFUNC function codes */
#define TXIT_FUNC_SCHED  0x01        /* Transaction scheduling        */
#define TXIT_FUNC_INPUT  0x02        /* Input processing              */
#define TXIT_FUNC_OUTPUT 0x03        /* Output processing             */
#define TXIT_FUNC_TERM   0x04        /* Termination                   */

/* TXITFLAGS bits */
#define TXIT_FLG_CONV    0x80        /* Conversational                */
#define TXIT_FLG_RESP    0x40        /* Response mode                 */
#define TXIT_FLG_FAST    0x20        /* Fast Path                     */
#define TXIT_FLG_REMOTE  0x10        /* Remote origination            */

#pragma pack()

/*===================================================================
 * Logger Exit Parameter List (DFSFLGX0)
 *===================================================================*/

#pragma pack(1)

struct ims_flgx_parm {
    EXIT_PARM_HEADER;                /* +0   Common header            */
    uint8_t        flgxtype;         /* +5   Log record type (replaces flags) */
    uint16_t       _reserved1;       /* +6   Reserved                 */
    void          *flgxrec;          /* +8   Log record pointer       */
    uint32_t       flgxrlen;         /* +12  Record length            */
    char           flgxtran[8];      /* +16  Transaction code         */
    char           flgxpsb[8];       /* +24  PSB name                 */
    char           flgxuser[8];      /* +32  User ID                  */
    uint32_t       flgxtime;         /* +40  Timestamp                */
};                                   /* Total: 44 bytes               */

/* FLGXFUNC function codes */
#define FLGX_FUNC_INIT   EXIT_FUNC_INIT        /* Initialization                */
#define FLGX_FUNC_LOG    0x02        /* Log record                    */
#define FLGX_FUNC_TERM   EXIT_FUNC_TERM        /* Termination                   */

/* FLGXTYPE log record types */
#define FLGX_TYPE_01     0x01        /* System event                  */
#define FLGX_TYPE_02     0x02        /* Database change               */
#define FLGX_TYPE_03     0x03        /* Message (input)               */
#define FLGX_TYPE_04     0x04        /* Message (output)              */
#define FLGX_TYPE_07     0x07        /* Checkpoint                    */
#define FLGX_TYPE_20     0x20        /* Transaction statistics        */
#define FLGX_TYPE_31     0x31        /* Sign-on/sign-off              */

#pragma pack()

/*===================================================================
 * Message Edit Exit Parameter List (DFSME000)
 *===================================================================*/

#pragma pack(1)

struct ims_me_parm {
    EXIT_PARM_HEADER;                /* +0   Common header            */
    void          *memsg;            /* +8   Message buffer pointer   */
    uint32_t       memlen;           /* +12  Message length           */
    uint32_t       membufln;         /* +16  Buffer max length        */
    char           metran[8];        /* +20  Transaction code         */
    char           melterm[8];       /* +28  Logical terminal         */
    uint8_t        meio;             /* +36  Input (1) / Output (2)   */
    uint8_t        _reserved2[3];    /* +37  Reserved                 */
};                                   /* Total: 40 bytes               */

/* MEFUNC function codes */
#define ME_FUNC_INIT     EXIT_FUNC_INIT        /* Initialization                */
#define ME_FUNC_EDIT     0x02        /* Edit message                  */
#define ME_FUNC_TERM     EXIT_FUNC_TERM        /* Termination                   */

/* MEIO values */
#define ME_IO_INPUT      0x01        /* Input message                 */
#define ME_IO_OUTPUT     0x02        /* Output message                */

#pragma pack()

/*===================================================================
 * BMP Scheduling Exit Parameter List (DFSBSEX0)
 *===================================================================*/

#pragma pack(1)

struct ims_bsex_parm {
    EXIT_PARM_HEADER;                /* +0   Common header            */
    char           bsexjob[8];       /* +8   BMP job name             */
    char           bsexpsb[8];       /* +16  PSB name                 */
    char           bsexuser[8];      /* +24  User ID                  */
    uint32_t       bsexdlay;         /* +32  Delay time (seconds)     */
    uint32_t       bsexreasn;        /* +36  Reason code (output)     */
};                                   /* Total: 40 bytes               */

/* BSEXFUNC function codes */
#define BSEX_FUNC_SCHED  0x01        /* Schedule request              */
#define BSEX_FUNC_CHECK  0x02        /* Check resources               */
#define BSEX_FUNC_TERM   0x03        /* Termination                   */

/*===================================================================
 * MSC Control Block Mappings (DFSMSCE0)
 *===================================================================*/

/* MSCD - MSC Destination Block */
struct mscd {
    char           mscdname[8];      /* +0  Destination name          */
    uint8_t        mscdflg1;         /* +8  Type flags                */
};

/* MSCDFLG1 bits */
#define MSCD_LOG               0x01  /* Destination is an LTERM       */

/* MSCP - MSC Parameter List */
struct mscp {
    uint32_t       mscpfunc;         /* +0  Function code             */
    uint8_t        mscpflg1;         /* +4  Status flags              */
    uint8_t        _reserved[3];     /* +5  Reserved                  */
    struct mscd   *mscpdest;         /* +8  Pointer to destination    */
    void          *mscpmsga;         /* +12 Pointer to message prefix */
};

/* MSCPFLG1 bits */
#define MSCP_REQD              0x80  /* Reroute requested by exit     */

/* MSC Function Codes */
#define MSC_FUNC_INIT          1     /* Initialization                */
#define MSC_FUNC_ROUTE         2     /* Message routing               */

#pragma pack()

/*===================================================================
 * IMS Utility Functions
 *===================================================================*/

/**
 * ims_status_ok - Check if status code is blank (success)
 * @status: 2-character status code
 * Returns: 1 if successful, 0 otherwise
 */
static inline int ims_status_ok(const char status[2]) {
    return status[0] == ' ' && status[1] == ' ';
}

/**
 * ims_status_match - Check if status matches expected
 * @status: 2-character status code
 * @expected: Expected status code (2 chars)
 * Returns: 1 if match, 0 otherwise
 */
static inline int ims_status_match(const char status[2], const char *expected) {
    return status[0] == expected[0] && status[1] == expected[1];
}

/**
 * ims_is_end_of_db - Check for end of database (GB)
 * @status: 2-character status code
 * Returns: 1 if end of database, 0 otherwise
 */
static inline int ims_is_end_of_db(const char status[2]) {
    return status[0] == 'G' && status[1] == 'B';
}

/**
 * ims_is_not_found - Check for segment not found (GE)
 * @status: 2-character status code
 * Returns: 1 if not found, 0 otherwise
 */
static inline int ims_is_not_found(const char status[2]) {
    return status[0] == 'G' && status[1] == 'E';
}

/**
 * ims_is_no_messages - Check for no more messages (QC)
 * @status: 2-character status code
 * Returns: 1 if no messages, 0 otherwise
 */
static inline int ims_is_no_messages(const char status[2]) {
    return status[0] == 'Q' && status[1] == 'C';
}

/**
 * ims_match_tran - Compare transaction codes
 * @tran1: First transaction code (8 chars)
 * @tran2: Second transaction code (8 chars)
 * Returns: 1 if equal, 0 otherwise
 */
static inline int ims_match_tran(const char tran1[8], const char tran2[8]) {
    return match_field(tran1, tran2, 8);
}

/**
 * ims_is_conversational - Check if transaction is conversational
 * @flags: SCD flags byte
 * Returns: 1 if conversational, 0 otherwise
 */
static inline int ims_is_conversational(uint8_t flags) {
    return (flags & SCD_FLG_CONV) != 0;
}

/**
 * ims_is_fast_path - Check if transaction is Fast Path
 * @flags: SCD flags byte
 * Returns: 1 if Fast Path, 0 otherwise
 */
static inline int ims_is_fast_path(uint8_t flags) {
    return (flags & SCD_FLG_FAST) != 0;
}

/*===================================================================
 * Example Usage
 *===================================================================*

// Sign-on exit - custom authentication
#pragma prolog(my_dfssgux0,"SAVE(14,12),LR(12,15)")
#pragma epilog(my_dfssgux0,"RETURN(14,12)")

int my_dfssgux0(struct ims_sgnx_parm *parm) {
    if (parm->sgxfunc == SGX_FUNC_SIGNON) {
        // Custom sign-on validation
        // Could check time restrictions, terminal restrictions, etc.

        // Check for restricted terminal
        if (memcmp_inline(parm->sgxlterm, "RESTTERM", 8) == 0) {
            parm->sgxreasn = 100; // Custom reason code
            return IMS_SGNX_REJECT;
        }
    }

    // Defer to RACF/ACF2 for standard authentication
    return IMS_SGNX_DEFER;
}

// Transaction exit - authorization checking
#pragma prolog(my_dfstxit0,"SAVE(14,12),LR(12,15)")
#pragma epilog(my_dfstxit0,"RETURN(14,12)")

int my_dfstxit0(struct ims_txit_parm *parm) {
    if (parm->txitfunc == TXIT_FUNC_SCHED) {
        // Check authorization for sensitive transactions
        if (memcmp_inline(parm->txittran, "ADMIN   ", 8) == 0) {
            // Could check user authorization
            // if (!is_admin(parm->txituser)) {
            //     return IMS_TXIT_REJECT;
            // }
        }
    }

    return IMS_TXIT_CONTINUE;
}

// Logger exit - filter sensitive data
#pragma prolog(my_dfsflgx0,"SAVE(14,12),LR(12,15)")
#pragma epilog(my_dfsflgx0,"RETURN(14,12)")

int my_dfsflgx0(struct ims_flgx_parm *parm) {
    if (parm->flgxfunc == FLGX_FUNC_LOG) {
        // Could suppress logging of sensitive transactions
        if (memcmp_inline(parm->flgxtran, "PAYROLL ", 8) == 0) {
            // Or mask sensitive data in the log record
            // mask_sensitive_data(parm->flgxrec, parm->flgxrlen);
            return IMS_FLGX_MODIFIED;
        }
    }

    return IMS_FLGX_CONTINUE;
}

// Message edit exit - validate input
#pragma prolog(my_dfsme000,"SAVE(14,12),LR(12,15)")
#pragma epilog(my_dfsme000,"RETURN(14,12)")

int my_dfsme000(struct ims_me_parm *parm) {
    if (parm->mefunc == ME_FUNC_EDIT && parm->meio == ME_IO_INPUT) {
        // Custom input validation
        // validate_input(parm->memsg, parm->memlen);
    }

    return IMS_ME_CONTINUE;
}

 *===================================================================*/

#endif /* METALC_IMS_H */
