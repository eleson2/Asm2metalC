/*********************************************************************
 * METALC_CICS.H - CICS Exit definitions for Metal C
 *
 * This header provides control block structures, constants, and
 * utility functions for CICS exit and user exit development.
 *
 * Supported exit types:
 *   System exits:
 *     DFHPEP   - Program error program
 *     DFHTEP   - Terminal error program
 *     DFHNEP   - Node error program
 *     DFHSEP   - Sign-on exit program
 *     DFHSNP   - Sign-off exit program
 *
 *   Global User Exits (GLUE):
 *     XEIOUT/XEIIN   - Exec interface exits
 *     XFCREQ/XFCREQC - File control exits
 *     XTSEREQ        - Temporary storage exits
 *     XTDREQ         - Transient data exits
 *     XPCREQ/XPCREQC - Program control exits
 *     XICREQ         - Interval control exits
 *
 * Usage:
 *   #include "metalc_base.h"
 *   #include "metalc_cics.h"
 *
 * IMPORTANT: Use CICS-supplied XPI (Exit Programming Interface)
 *            for service calls. Direct SVC calls are prohibited.
 *
 * Note: Control block layouts vary by CICS TS version.
 *********************************************************************/

#ifndef METALC_CICS_H
#define METALC_CICS_H

#include "metalc_base.h"

/*===================================================================
 * CICS Exit Return Codes
 *===================================================================*/

/* Global User Exit return codes */
#define CICS_UERCNORM          RC_OK     /* Normal completion             */
#define CICS_UERCBYP           RC_WARNING  /* Bypass CICS processing        */
#define CICS_UERCPURG          RC_ERROR    /* Purge the task                */

/* Program Error Program (DFHPEP) return codes */
#define CICS_PEP_CONTINUE      RC_OK     /* Continue with abend           */
#define CICS_PEP_RETRY         RC_WARNING  /* Retry the operation           */
#define CICS_PEP_SUPPRESS      RC_ERROR    /* Suppress the abend            */

/* Terminal Error Program (DFHTEP) return codes */
#define CICS_TEP_CONTINUE      RC_OK     /* Continue error processing     */
#define CICS_TEP_RETRY         RC_WARNING  /* Retry the operation           */
#define CICS_TEP_IGNORE        RC_ERROR    /* Ignore the error              */

/* Sign-on Exit (DFHSEP) return codes */
#define CICS_SEP_ALLOW         RC_OK     /* Allow sign-on                 */
#define CICS_SEP_DENY          RC_WARNING  /* Deny sign-on                  */
#define CICS_SEP_DEFER         RC_ERROR    /* Defer to ESM (RACF/ACF2)      */

/*===================================================================
 * CICS Transaction Response Codes (EIBRESP)
 *===================================================================*/

#define CICS_RESP_NORMAL       0     /* Normal response               */
#define CICS_RESP_ERROR        1     /* Error                         */
#define CICS_RESP_RDATT        2     /* Read attention                */
#define CICS_RESP_WRBRK        3     /* Write break                   */
#define CICS_RESP_EOF          4     /* End of file                   */
#define CICS_RESP_EODS         5     /* End of data set               */
#define CICS_RESP_EOC          6     /* End of chain                  */
#define CICS_RESP_INBFMH       7     /* Inbound FMH received          */
#define CICS_RESP_ENDINPT      8     /* End input                     */
#define CICS_RESP_NONVAL       9     /* Non-valid request             */
#define CICS_RESP_NOSTART      10    /* No start                      */
#define CICS_RESP_TERMIDERR    11    /* Terminal ID error             */
#define CICS_RESP_FILENOTFND   12    /* File not found                */
#define CICS_RESP_NOTFND       13    /* Record not found              */
#define CICS_RESP_DUPREC       14    /* Duplicate record              */
#define CICS_RESP_DUPKEY       15    /* Duplicate key                 */
#define CICS_RESP_INVREQ       16    /* Invalid request               */
#define CICS_RESP_IOERR        17    /* I/O error                     */
#define CICS_RESP_NOSPACE      18    /* No space                      */
#define CICS_RESP_NOTOPEN      19    /* File not open                 */
#define CICS_RESP_ENDFILE      20    /* End of file                   */
#define CICS_RESP_ILLOGIC      21    /* VSAM logic error              */
#define CICS_RESP_LENGERR      22    /* Length error                  */
#define CICS_RESP_QZERO        23    /* Queue zero                    */
#define CICS_RESP_SIGNAL       24    /* Signal received               */
#define CICS_RESP_QBUSY        25    /* Queue busy                    */
#define CICS_RESP_ITEMERR      26    /* Item error                    */
#define CICS_RESP_PGMIDERR     27    /* Program ID error              */
#define CICS_RESP_TRANSIDERR   28    /* Transaction ID error          */
#define CICS_RESP_ENDDATA      29    /* End of data                   */
#define CICS_RESP_EXPIRED      31    /* Time expired                  */
#define CICS_RESP_DISABLED     84    /* Resource disabled             */

/*===================================================================
 * CICS Abend Codes
 *===================================================================*/

#define CICS_ABND_ASRA     "ASRA"    /* Program check                 */
#define CICS_ABND_ASRB     "ASRB"    /* Operating system abend        */
#define CICS_ABND_ASRD     "ASRD"    /* External CICS interface       */
#define CICS_ABND_AICA     "AICA"    /* Runaway task                  */
#define CICS_ABND_AEI0     "AEI0"    /* Exec interface error          */
#define CICS_ABND_AEIP     "AEIP"    /* Invalid command               */
#define CICS_ABND_AKCS     "AKCS"    /* Storage violation             */

/*===================================================================
 * CICS File Control Operations
 *===================================================================*/

#define CICS_FC_READ           0x01  /* Read                          */
#define CICS_FC_WRITE          0x02  /* Write                         */
#define CICS_FC_REWRITE        0x03  /* Rewrite                       */
#define CICS_FC_DELETE         0x04  /* Delete                        */
#define CICS_FC_UNLOCK         0x05  /* Unlock                        */
#define CICS_FC_STARTBR        0x06  /* Start browse                  */
#define CICS_FC_READNEXT       0x07  /* Read next                     */
#define CICS_FC_READPREV       0x08  /* Read previous                 */
#define CICS_FC_ENDBR          0x09  /* End browse                    */
#define CICS_FC_RESETBR        0x0A  /* Reset browse                  */

/*===================================================================
 * EIB - Exec Interface Block
 *===================================================================*/

#pragma pack(1)

struct dfheiblk {
    uint32_t       eibtime;          /* +0   Time (0HHMMSS packed)    */
    uint32_t       eibdate;          /* +4   Date (0CYYDDD packed)    */
    char           eibtrnid[4];      /* +8   Transaction ID           */
    char           eibtaskn[4];      /* +12  Task number              */
    char           eibtrmid[4];      /* +16  Terminal ID              */
    uint16_t       eibcposn;         /* +20  Cursor position          */
    uint16_t       eibcalen;         /* +22  COMMAREA length          */
    uint8_t        eibaid;           /* +24  Attention ID             */
    uint8_t        eibfn[2];         /* +25  Function code            */
    uint8_t        eibrcode[6];      /* +27  Response code            */
    char           eibds[8];         /* +33  Dataset name             */
    char           eibreqid[8];      /* +41  Request ID               */
    char           eibrsrce[8];      /* +49  Resource name            */
    uint8_t        eibsync;          /* +57  Syncpoint requested      */
    uint8_t        eibfree;          /* +58  Free requested           */
    uint8_t        eibrecv;          /* +59  Receive required         */
    uint8_t        eibsend;          /* +60  Send required            */
    uint8_t        eibatt;           /* +61  Attach received          */
    uint8_t        eibeoc;           /* +62  End of chain             */
    uint8_t        eibfmh;           /* +63  FMH received             */
    uint8_t        eibcompl;         /* +64  RRN complete             */
    uint8_t        eibsig;           /* +65  Signal received          */
    uint8_t        eibconf;          /* +66  Confirm requested        */
    uint8_t        eiberr;           /* +67  Error received           */
    uint8_t        eiberrcd[4];      /* +68  Error code               */
    uint8_t        eibsynrb;         /* +72  Syncpoint rollback       */
    uint8_t        eibnodat;         /* +73  No data                  */
    int32_t        eibresp;          /* +74  Response code            */
    int32_t        eibresp2;         /* +78  Response code 2          */
    uint8_t        eibrldbk;         /* +82  Rollback required        */
    uint8_t        _reserved[3];     /* +83  Reserved                 */
};                                   /* Total: 86 bytes               */

/* EIBAID attention identifier values */
#define EIBAID_ENTER       0x7D      /* Enter key                     */
#define EIBAID_CLEAR       0x6D      /* Clear key                     */
#define EIBAID_PA1         0x6C      /* PA1                           */
#define EIBAID_PA2         0x6E      /* PA2                           */
#define EIBAID_PA3         0x6B      /* PA3                           */
#define EIBAID_PF1         0xF1      /* PF1                           */
#define EIBAID_PF2         0xF2      /* PF2                           */
#define EIBAID_PF3         0xF3      /* PF3                           */
#define EIBAID_PF4         0xF4      /* PF4                           */
#define EIBAID_PF5         0xF5      /* PF5                           */
#define EIBAID_PF6         0xF6      /* PF6                           */
#define EIBAID_PF7         0xF7      /* PF7                           */
#define EIBAID_PF8         0xF8      /* PF8                           */
#define EIBAID_PF9         0xF9      /* PF9                           */
#define EIBAID_PF10        0x7A      /* PF10                          */
#define EIBAID_PF11        0x7B      /* PF11                          */
#define EIBAID_PF12        0x7C      /* PF12                          */

#pragma pack()

/*===================================================================
 * Global User Exit Parameter List
 *===================================================================*/

#pragma pack(1)

struct dfhuepar {
    void          *uepexn;           /* +0   Exit number              */
    void          *uepgaa;           /* +4   Global area address      */
    void          *ueptaa;           /* +8   Task area address        */
    void          *uephmsa;          /* +12  HSM area address         */
    uint8_t        uepactv;          /* +16  Exit active flag         */
    uint8_t        ueprecur;         /* +17  Recursive call flag      */
    uint8_t        uepterm;          /* +18  Termination flag         */
    uint8_t        uepflags;         /* +19  Flags                    */
    void          *uepcrsa;          /* +20  Current resource addr    */
    void          *ueptca;           /* +24  TCA address              */
    void          *uepeib;           /* +28  EIB address              */
    char           ueptran[4];       /* +32  Transaction ID           */
    char           uepterm4[4];      /* +36  Terminal ID              */
    char           uepuser[8];       /* +40  User ID                  */
    void          *uepwork;          /* +48  Work area address        */
    uint32_t       uepwklen;         /* +52  Work area length         */
};                                   /* Total: 56 bytes               */

/* UEPFLAGS bits */
#define UEPF_INIT        0x80        /* Initialization call           */
#define UEPF_TERM        0x40        /* Termination call              */
#define UEPF_TASK        0x20        /* Task-related call             */

#pragma pack()

/*===================================================================
 * File Control Exit Parameter List (XFCREQ/XFCREQC)
 *===================================================================*/

#pragma pack(1)

struct dfhfcpar {
    struct dfhuepar base;            /* +0   Base parameters          */
    char           fcpdsn[8];        /* +56  Dataset name             */
    uint8_t        fcpreq;           /* +64  Request type             */
    uint8_t        fcpopt;           /* +65  Options                  */
    uint8_t        fcpresp;          /* +66  Response code            */
    uint8_t        fcpresp2;         /* +67  Response code 2          */
    void          *fcpkey;           /* +68  Key address              */
    uint16_t       fcpklen;          /* +72  Key length               */
    uint16_t       fcprlen;          /* +74  Record length            */
    void          *fcprec;           /* +76  Record address           */
    uint32_t       fcprrn;           /* +80  Relative record number   */
};                                   /* Total: 84 bytes               */

/* FCPREQ - Request type */
#define FCP_REQ_READ       0x01      /* Read request                  */
#define FCP_REQ_WRITE      0x02      /* Write request                 */
#define FCP_REQ_REWRITE    0x03      /* Rewrite request               */
#define FCP_REQ_DELETE     0x04      /* Delete request                */
#define FCP_REQ_BROWSE     0x05      /* Browse request                */

/* FCPOPT - Options */
#define FCP_OPT_UPDATE     0x80      /* Update intent                 */
#define FCP_OPT_NOSUSPEND  0x40      /* No suspend                    */
#define FCP_OPT_MASSINS    0x20      /* Mass insert                   */

#pragma pack()

/*===================================================================
 * Program Control Exit Parameter List (XPCREQ/XPCREQC)
 *===================================================================*/

#pragma pack(1)

struct dfhpcpar {
    struct dfhuepar base;            /* +0   Base parameters          */
    char           pcpprog[8];       /* +56  Program name             */
    uint8_t        pcpreq;           /* +64  Request type             */
    uint8_t        pcpresp;          /* +65  Response code            */
    uint16_t       _reserved1;       /* +66  Reserved                 */
    void          *pcpcomm;          /* +68  COMMAREA address         */
    uint32_t       pcpclen;          /* +72  COMMAREA length          */
    char           pcptran[4];       /* +76  Transaction ID           */
    char           pcpuser[8];       /* +80  User ID                  */
};                                   /* Total: 88 bytes               */

/* PCPREQ - Request type */
#define PCP_REQ_LINK       0x01      /* LINK request                  */
#define PCP_REQ_XCTL       0x02      /* XCTL request                  */
#define PCP_REQ_LOAD       0x03      /* LOAD request                  */
#define PCP_REQ_RETURN     0x04      /* RETURN request                */

#pragma pack()

/*===================================================================
 * Temporary Storage Exit Parameter List (XTSEREQ)
 *===================================================================*/

#pragma pack(1)

struct dfhtsepar {
    struct dfhuepar base;            /* +0   Base parameters          */
    char           tsepque[8];       /* +56  Queue name               */
    uint8_t        tsepreq;          /* +64  Request type             */
    uint8_t        tsepmain;         /* +65  Main/Auxiliary           */
    uint16_t       tsepitem;         /* +66  Item number              */
    void          *tsepdata;         /* +68  Data address             */
    uint32_t       tseplen;          /* +72  Data length              */
    uint8_t        tsepresp;         /* +76  Response code            */
    uint8_t        _reserved[3];     /* +77  Reserved                 */
};                                   /* Total: 80 bytes               */

/* TSEPREQ - Request type */
#define TSE_REQ_WRITEQ     0x01      /* WRITEQ TS                     */
#define TSE_REQ_READQ      0x02      /* READQ TS                      */
#define TSE_REQ_DELETEQ    0x03      /* DELETEQ TS                    */

/* TSEPMAIN - Storage type */
#define TSE_MAIN           0x80      /* Main storage                  */
#define TSE_AUXILIARY      0x00      /* Auxiliary storage             */

#pragma pack()

/*===================================================================
 * Program Error Program Parameter List (DFHPEP)
 *===================================================================*/

#pragma pack(1)

struct dfhpeppar {
    void          *pepeib;           /* +0   EIB address              */
    void          *pepcomm;          /* +4   COMMAREA address         */
    char           peptran[4];       /* +8   Transaction ID           */
    char           pepprog[8];       /* +12  Program name             */
    char           pepabnd[4];       /* +20  Abend code               */
    uint32_t       peppsc;           /* +24  PSW at abend             */
    void          *peppsw;           /* +28  PSW address              */
    void          *pepregs;          /* +32  Registers at abend       */
    uint8_t        peptype;          /* +36  Error type               */
    uint8_t        pepflags;         /* +37  Flags                    */
    uint16_t       _reserved;        /* +38  Reserved                 */
};                                   /* Total: 40 bytes               */

/* PEPTYPE - Error types */
#define PEP_TYPE_ASRA      0x01      /* Program check (ASRA)          */
#define PEP_TYPE_ASRB      0x02      /* Operating system abend        */
#define PEP_TYPE_AICA      0x03      /* Runaway task                  */
#define PEP_TYPE_USER      0x04      /* User abend                    */

#pragma pack()

/*===================================================================
 * Sign-on Exit Parameter List (DFHSEP)
 *===================================================================*/

#pragma pack(1)

struct dfhseppar {
    void          *sepeib;           /* +0   EIB address              */
    char           sepuser[8];       /* +4   User ID                  */
    char           seppass[8];       /* +12  Password (masked)        */
    char           sepnpass[8];      /* +20  New password (masked)    */
    char           sepgrp[8];        /* +28  Group ID                 */
    char           septerm[4];       /* +36  Terminal ID              */
    uint8_t        sepflags;         /* +40  Flags                    */
    uint8_t        sepresp;          /* +41  Response                 */
    uint16_t       _reserved;        /* +42  Reserved                 */
    char          *sepmsg;           /* +44  Message area             */
    uint16_t       sepmsgln;         /* +48  Message length           */
    uint16_t       _reserved2;       /* +50  Reserved                 */
};                                   /* Total: 52 bytes               */

/* SEPFLAGS bits */
#define SEP_FLG_PWCHG      0x80      /* Password change request       */
#define SEP_FLG_ESM        0x40      /* ESM authentication requested  */
#define SEP_FLG_PERSIST    0x20      /* Persistent verification       */

#pragma pack()

/*===================================================================
 * CICS Utility Functions
 *===================================================================*/

/**
 * cics_is_pf_key - Check if attention ID is a PF key
 * @aid: Attention ID from EIB
 * Returns: 1 if PF key, 0 otherwise
 */
static inline int cics_is_pf_key(uint8_t aid) {
    return (aid >= EIBAID_PF1 && aid <= EIBAID_PF9) ||
           (aid >= EIBAID_PF10 && aid <= EIBAID_PF12);
}

/**
 * cics_get_pf_number - Get PF key number from AID
 * @aid: Attention ID from EIB
 * Returns: PF key number (1-12) or 0 if not PF key
 */
static inline int cics_get_pf_number(uint8_t aid) {
    if (aid >= EIBAID_PF1 && aid <= EIBAID_PF9) {
        return (aid - EIBAID_PF1) + 1;
    }
    if (aid == EIBAID_PF10) return 10;
    if (aid == EIBAID_PF11) return 11;
    if (aid == EIBAID_PF12) return 12;
    return 0;
}

/**
 * cics_is_enter - Check if ENTER was pressed
 * @aid: Attention ID from EIB
 * Returns: 1 if ENTER, 0 otherwise
 */
static inline int cics_is_enter(uint8_t aid) {
    return aid == EIBAID_ENTER;
}

/**
 * cics_is_clear - Check if CLEAR was pressed
 * @aid: Attention ID from EIB
 * Returns: 1 if CLEAR, 0 otherwise
 */
static inline int cics_is_clear(uint8_t aid) {
    return aid == EIBAID_CLEAR;
}

/**
 * cics_resp_ok - Check if response is normal
 * @eibresp: Response code from EIB
 * Returns: 1 if normal, 0 otherwise
 */
static inline int cics_resp_ok(int32_t eibresp) {
    return eibresp == CICS_RESP_NORMAL;
}

/**
 * cics_parse_time - Parse CICS packed time (0HHMMSS)
 * @eibtime: Time from EIB
 * @hour: Output hours
 * @min: Output minutes
 * @sec: Output seconds
 */
static inline void cics_parse_time(uint32_t eibtime,
                                    int *hour, int *min, int *sec) {
    *hour = ((eibtime >> 20) & 0x0F) * 10 + ((eibtime >> 16) & 0x0F);
    *min  = ((eibtime >> 12) & 0x0F) * 10 + ((eibtime >> 8) & 0x0F);
    *sec  = ((eibtime >> 4) & 0x0F) * 10 + (eibtime & 0x0F);
}

/**
 * cics_parse_date - Parse CICS packed date (0CYYDDD)
 * @eibdate: Date from EIB
 * @year: Output year
 * @day: Output day of year
 */
static inline void cics_parse_date(uint32_t eibdate,
                                    int *year, int *day) {
    int century = (eibdate >> 24) & 0x0F;
    int yy = ((eibdate >> 20) & 0x0F) * 10 + ((eibdate >> 16) & 0x0F);
    *year = (century == 0 ? 1900 : 2000) + yy;
    *day = ((eibdate >> 12) & 0x0F) * 100 +
           ((eibdate >> 8) & 0x0F) * 10 +
           ((eibdate >> 4) & 0x0F);
}

/**
 * cics_match_tran - Compare transaction IDs
 * @tran1: First transaction ID (4 chars)
 * @tran2: Second transaction ID (4 chars)
 * Returns: 1 if equal, 0 otherwise
 */
static inline int cics_match_tran(const char tran1[4], const char tran2[4]) {
    return match_field(tran1, tran2, 4);
}

/*===================================================================
 * Example Usage
 *===================================================================*

// File control exit - audit file access
#pragma prolog(my_xfcreq,"SAVE(14,12),LR(12,15)")
#pragma epilog(my_xfcreq,"RETURN(14,12)")

int my_xfcreq(struct dfhfcpar *parm) {
    // Log all write operations to sensitive files
    if (parm->fcpreq == FCP_REQ_WRITE) {
        if (memcmp_inline(parm->fcpdsn, "PAYROLL ", 8) == 0) {
            // Custom audit logging
            // audit_file_access(parm);
        }
    }

    return CICS_UERCNORM;
}

// Program control exit - check authorization
#pragma prolog(my_xpcreq,"SAVE(14,12),LR(12,15)")
#pragma epilog(my_xpcreq,"RETURN(14,12)")

int my_xpcreq(struct dfhpcpar *parm) {
    // Check authorization for sensitive programs
    if (memcmp_inline(parm->pcpprog, "ADMIN", 5) == 0) {
        // Could check user authorization here
        // if (!is_admin(parm->base.uepuser)) {
        //     return CICS_UERCPURG;
        // }
    }

    return CICS_UERCNORM;
}

// Program error program - custom abend handling
#pragma prolog(my_dfhpep,"SAVE(14,12),LR(12,15)")
#pragma epilog(my_dfhpep,"RETURN(14,12)")

int my_dfhpep(struct dfhpeppar *parm) {
    // Log program errors
    // log_abend(parm->peptran, parm->pepprog, parm->pepabnd);

    // Let CICS continue with normal abend processing
    return CICS_PEP_CONTINUE;
}

// Sign-on exit - additional authentication
#pragma prolog(my_dfhsep,"SAVE(14,12),LR(12,15)")
#pragma epilog(my_dfhsep,"RETURN(14,12)")

int my_dfhsep(struct dfhseppar *parm) {
    // Custom authentication logic
    // Could implement additional validation beyond ESM

    // Check for specific terminal restrictions
    if (memcmp_inline(parm->septerm, "XXXX", 4) == 0) {
        // Restricted terminal
        return CICS_SEP_DENY;
    }

    // Defer to ESM (RACF/ACF2) for standard authentication
    return CICS_SEP_DEFER;
}

 *===================================================================*/

#endif /* METALC_CICS_H */
