/*********************************************************************
 * METALC_DB2.H - DB2 Exit definitions for Metal C
 *
 * This header provides control block structures, constants, and
 * utility functions for DB2 for z/OS exit development.
 *
 * Supported exits:
 *   Connection/Sign-on exits:
 *     DSNX@XAC - Connection exit (primary)
 *     DSN3@ATH - Authorization exit
 *     DSN3@SGN - Sign-on exit
 *
 *   Data exits:
 *     DSNXVDEP - Data validation exit (edit procedures)
 *     DSNXVFEP - Field procedures
 *
 *   Utility exits:
 *     DSNAOINI - ODBC initialization exit
 *     DSNTEJ6Z - EDITPROC example
 *
 *   Storage exits:
 *     DSN3SATH - Storage authorization
 *
 * Usage:
 *   #include "metalc_base.h"
 *   #include "metalc_db2.h"
 *
 * IMPORTANT: DB2 exits must be reentrant and follow DB2
 *            calling conventions strictly.
 *
 * Note: Control block layouts vary by DB2 version.
 *********************************************************************/

#ifndef METALC_DB2_H
#define METALC_DB2_H

#include "metalc_base.h"

/*===================================================================
 * DB2 Exit Return Codes
 *===================================================================*/

/* Connection exit (DSNX@XAC) return codes */
#define DB2_XAC_CONTINUE       RC_OK     /* Continue processing           */
#define DB2_XAC_REJECT         RC_WARNING  /* Reject connection             */
#define DB2_XAC_MODIFIED       RC_ERROR    /* Parameters modified           */
#define DB2_XAC_ERROR          RC_SEVERE   /* Error - check reason code     */

/* Authorization exit (DSN3@ATH) return codes */
#define DB2_ATH_ALLOW          RC_OK     /* Allow access                  */
#define DB2_ATH_REJECT         RC_WARNING  /* Reject access                 */
#define DB2_ATH_CONTINUE       RC_ERROR    /* Continue DB2 auth check       */
#define DB2_ATH_ERROR          RC_SEVERE   /* Error occurred                */

/* Sign-on exit (DSN3@SGN) return codes */
#define DB2_SGN_ALLOW          RC_OK     /* Allow sign-on                 */
#define DB2_SGN_REJECT         RC_WARNING  /* Reject sign-on                */
#define DB2_SGN_DEFER          RC_ERROR    /* Defer to RACF/ACF2            */
#define DB2_SGN_ERROR          RC_SEVERE   /* Error occurred                */

/* Edit procedure return codes */
#define DB2_EDIT_OK            RC_OK     /* Successful                    */
#define DB2_EDIT_WARNING       RC_WARNING  /* Warning (continue)            */
#define DB2_EDIT_ERROR         RC_ERROR    /* Error - reject operation      */
#define DB2_EDIT_SEVERE        RC_SEVERE   /* Severe error                  */

/* Field procedure return codes */
#define DB2_FIELD_OK           0     /* Successful                    */
#define DB2_FIELD_NULL         4     /* Null value                    */
#define DB2_FIELD_ERROR        8     /* Encoding/decoding error       */

/*===================================================================
 * DB2 SQLCODE Values
 *===================================================================*/

#define DB2_SQLCODE_OK         0     /* Successful completion         */
#define DB2_SQLCODE_NOT_FOUND  100   /* Row not found                 */
#define DB2_SQLCODE_DUP_KEY    -803  /* Duplicate key                 */
#define DB2_SQLCODE_DEADLOCK   -911  /* Deadlock/timeout              */
#define DB2_SQLCODE_CONNECT    -30081 /* Connection error             */
#define DB2_SQLCODE_AUTH       -551  /* Authorization failure         */

/*===================================================================
 * DB2 Privilege Types
 *===================================================================*/

/* Database privileges */
#define DB2_PRIV_DBADM         0x0001 /* Database administrator       */
#define DB2_PRIV_DBCTRL        0x0002 /* Database control             */
#define DB2_PRIV_DBMAINT       0x0004 /* Database maintenance         */
#define DB2_PRIV_CREATETAB     0x0008 /* Create table                 */
#define DB2_PRIV_CREATETS      0x0010 /* Create tablespace            */
#define DB2_PRIV_DISPLAYDB     0x0020 /* Display database             */
#define DB2_PRIV_DROP          0x0040 /* Drop objects                 */
#define DB2_PRIV_IMAGCOPY      0x0080 /* Image copy                   */
#define DB2_PRIV_LOAD          0x0100 /* Load                         */
#define DB2_PRIV_RECOVERDB     0x0200 /* Recover database             */
#define DB2_PRIV_REORG         0x0400 /* Reorganize                   */
#define DB2_PRIV_REPAIR        0x0800 /* Repair                       */
#define DB2_PRIV_STARTDB       0x1000 /* Start database               */
#define DB2_PRIV_STATS         0x2000 /* Run statistics               */
#define DB2_PRIV_STOPDB        0x4000 /* Stop database                */

/* Table privileges */
#define DB2_TPRIV_SELECT       0x0001 /* Select                       */
#define DB2_TPRIV_INSERT       0x0002 /* Insert                       */
#define DB2_TPRIV_UPDATE       0x0004 /* Update                       */
#define DB2_TPRIV_DELETE       0x0008 /* Delete                       */
#define DB2_TPRIV_ALTER        0x0010 /* Alter                        */
#define DB2_TPRIV_INDEX        0x0020 /* Index                        */
#define DB2_TPRIV_REFERENCES   0x0040 /* References                   */
#define DB2_TPRIV_TRIGGER      0x0080 /* Trigger                      */

/* System privileges */
#define DB2_SPRIV_SYSADM       0x0001 /* System administrator         */
#define DB2_SPRIV_SYSCTRL      0x0002 /* System control               */
#define DB2_SPRIV_SYSOPR       0x0004 /* System operator              */
#define DB2_SPRIV_BINDADD      0x0008 /* Bind add                     */
#define DB2_SPRIV_BINDAGENT    0x0010 /* Bind agent                   */
#define DB2_SPRIV_CREATEDBA    0x0020 /* Create database              */
#define DB2_SPRIV_CREATESG     0x0040 /* Create storage group         */
#define DB2_SPRIV_DISPLAY      0x0080 /* Display                      */
#define DB2_SPRIV_MONITOR1     0x0100 /* Monitor level 1              */
#define DB2_SPRIV_MONITOR2     0x0200 /* Monitor level 2              */
#define DB2_SPRIV_RECOVER      0x0400 /* Recover                      */
#define DB2_SPRIV_STOPALL      0x0800 /* Stop all                     */
#define DB2_SPRIV_STOSPACE     0x1000 /* Storage space                */
#define DB2_SPRIV_TRACE        0x2000 /* Trace                        */

/*===================================================================
 * DB2 Connection Types
 *===================================================================*/

#define DB2_CONN_BATCH         'B'   /* Batch                         */
#define DB2_CONN_TSO           'T'   /* TSO                           */
#define DB2_CONN_CICS          'C'   /* CICS                          */
#define DB2_CONN_IMS           'I'   /* IMS                           */
#define DB2_CONN_RRSAF         'R'   /* RRSAF                         */
#define DB2_CONN_DDF           'D'   /* DDF (DRDA)                    */
#define DB2_CONN_CAF           'A'   /* Call Attach Facility          */

/*===================================================================
 * Connection Exit Parameter List (DSNX@XAC)
 *===================================================================*/

#pragma pack(1)

struct db2_xac_parm {
    EXIT_PARM_HEADER;                /* +0   Common header            */
    uint8_t        xactype;          /* +6   Connection type          */
    uint8_t        _reserved1;       /* +7   Reserved                 */
    char           xacplan[8];       /* +8   Plan name                */
    char           xacpkg[18];       /* +16  Package name             */
    char           _reserved2[2];    /* +34  Reserved                 */
    char           xaccoll[18];      /* +36  Collection ID            */
    char           _reserved3[2];    /* +54  Reserved                 */
    char           xacauth[8];       /* +56  Primary auth ID          */
    char           xacsauth[8];      /* +64  Secondary auth ID        */
    char           xacjob[8];        /* +72  Job name                 */
    char           xacsys[4];        /* +80  Subsystem name           */
    char           xacconn[8];       /* +84  Connection name          */
    char           xaccorr[12];      /* +92  Correlation ID           */
    uint32_t       xacreasn;         /* +104 Reason code (output)     */
    char          *xacnauth;         /* +108 New auth ID (output)     */
    uint16_t       xacnaln;          /* +112 New auth ID length       */
    uint16_t       _reserved4;       /* +114 Reserved                 */
};                                   /* Total: 116 bytes              */

/* XACFUNC function codes */
#define XAC_FUNC_CONNECT   0x01      /* Connection request            */
#define XAC_FUNC_SIGNON    0x02      /* Sign-on request               */
#define XAC_FUNC_PLAN      0x03      /* Plan allocation               */
#define XAC_FUNC_AUTHCHG   0x04      /* Auth ID change                */
#define XAC_FUNC_DISCONNECT 0x05     /* Disconnection                 */

/* XACFLAGS bits */
#define XAC_FLG_TRUSTED    0x80      /* Trusted context               */
#define XAC_FLG_REUSE      0x40      /* Connection reuse              */
#define XAC_FLG_POOLED     0x20      /* Pooled connection             */
#define XAC_FLG_ENCPWD     0x10      /* Password encrypted            */

#pragma pack()

/*===================================================================
 * Authorization Exit Parameter List (DSN3@ATH)
 *===================================================================*/

#pragma pack(1)

struct db2_ath_parm {
    EXIT_PARM_HEADER;                /* +0   Common header            */
    uint16_t       athpriv;          /* +6   Privilege requested      */
    char           athauth[8];       /* +8   Authorization ID         */
    char           athobj[18];       /* +16  Object name              */
    char           _reserved1[2];    /* +34  Reserved                 */
    char           athowner[8];      /* +36  Object owner             */
    char           athtype[8];       /* +44  Object type              */
    char           athschm[8];       /* +52  Schema name              */
    uint32_t       athreasn;         /* +60  Reason code (output)     */
    char          *athmsg;           /* +64  Message area             */
    uint16_t       athmsgln;         /* +68  Message length           */
    uint16_t       _reserved2;       /* +70  Reserved                 */
};                                   /* Total: 72 bytes               */

/* ATHFUNC function codes */
#define ATH_FUNC_OBJECT    0x01      /* Object access                 */
#define ATH_FUNC_SYSTEM    0x02      /* System privilege              */
#define ATH_FUNC_TABLE     0x03      /* Table access                  */
#define ATH_FUNC_PLAN      0x04      /* Plan execution                */
#define ATH_FUNC_PACKAGE   0x05      /* Package execution             */
#define ATH_FUNC_ROUTINE   0x06      /* Routine execution             */

/* ATHTYPE object types */
#define ATH_TYPE_TABLE     "TABLE   "
#define ATH_TYPE_VIEW      "VIEW    "
#define ATH_TYPE_INDEX     "INDEX   "
#define ATH_TYPE_DATABASE  "DATABASE"
#define ATH_TYPE_TSPACE    "TBLSPACE"
#define ATH_TYPE_STOGROUP  "STOGROUP"
#define ATH_TYPE_PLAN      "PLAN    "
#define ATH_TYPE_PACKAGE   "PACKAGE "
#define ATH_TYPE_ROUTINE   "ROUTINE "

#pragma pack()

/*===================================================================
 * Sign-on Exit Parameter List (DSN3@SGN)
 *===================================================================*/

#pragma pack(1)

struct db2_sgn_parm {
    EXIT_PARM_HEADER;                /* +0   Common header            */
    uint8_t        sgntype;          /* +6   Connection type          */
    uint8_t        _reserved1;       /* +7   Reserved                 */
    char           sgnauth[8];       /* +8   Primary auth ID          */
    char           sgnpass[8];       /* +16  Password (masked)        */
    char           sgnnpass[8];      /* +24  New password (masked)    */
    char           sgnjob[8];        /* +32  Job name                 */
    char           sgnsys[4];        /* +40  Subsystem name           */
    char           sgnconn[8];       /* +44  Connection name          */
    uint32_t       sgnreasn;         /* +52  Reason code (output)     */
    char          *sgnmsg;           /* +56  Message area             */
    uint16_t       sgnmsgln;         /* +60  Message length           */
    uint16_t       _reserved2;       /* +62  Reserved                 */
};                                   /* Total: 64 bytes               */

/* SGNFUNC function codes */
#define SGN_FUNC_SIGNON    0x01      /* Sign-on request               */
#define SGN_FUNC_PWCHG     0x02      /* Password change               */
#define SGN_FUNC_VERIFY    0x03      /* Verification only             */

/* SGNFLAGS bits */
#define SGN_FLG_RACF       0x80      /* RACF/ACF2 available           */
#define SGN_FLG_ENCPWD     0x40      /* Password encrypted            */
#define SGN_FLG_TRUSTED    0x20      /* Trusted context               */

#pragma pack()

/*===================================================================
 * Edit Procedure Parameter List
 *===================================================================*/

#pragma pack(1)

struct db2_edit_parm {
    EXIT_PARM_HEADER;                /* +0   Common header            */
    uint16_t       _reserved1;       /* +6   Reserved                 */
    void          *editdata;         /* +8   Data area pointer        */
    uint32_t       editdlen;         /* +12  Data length              */
    void          *editout;          /* +16  Output area pointer      */
    uint32_t       editolen;         /* +20  Output max length        */
    uint32_t       editoact;         /* +24  Actual output length     */
    char           edittab[18];      /* +28  Table name               */
    char           _reserved2[2];    /* +46  Reserved                 */
    char           editschm[8];      /* +48  Schema name              */
    uint32_t       editreasn;        /* +56  Reason code (output)     */
};                                   /* Total: 60 bytes               */

/* EDITFUNC function codes */
#define EDIT_FUNC_ENCODE   0x01      /* Encode (input)                */
#define EDIT_FUNC_DECODE   0x02      /* Decode (output)               */
#define EDIT_FUNC_INIT     0x03      /* Initialization                */
#define EDIT_FUNC_TERM     0x04      /* Termination                   */

/* EDITFLAGS bits */
#define EDIT_FLG_NULL      0x80      /* Null value                    */
#define EDIT_FLG_INSERT    0x40      /* Insert operation              */
#define EDIT_FLG_UPDATE    0x20      /* Update operation              */
#define EDIT_FLG_SELECT    0x10      /* Select operation              */

#pragma pack()

/*===================================================================
 * Field Procedure Parameter List
 *===================================================================*/

#pragma pack(1)

struct db2_field_parm {
    EXIT_PARM_HEADER;                /* +0   Common header            */
    uint16_t       fldcolno;         /* +6   Column number            */
    void          *fldsrc;           /* +8   Source data pointer      */
    uint32_t       fldsrcln;         /* +12  Source data length       */
    void          *flddst;           /* +16  Destination pointer      */
    uint32_t       flddstln;         /* +20  Destination max length   */
    uint32_t       flddstact;        /* +24  Actual destination len   */
    uint16_t       fldsrctyp;        /* +28  Source data type         */
    uint16_t       flddsttyp;        /* +30  Destination data type    */
    char           fldcol[30];       /* +32  Column name              */
    char           _reserved1[2];    /* +62  Reserved                 */
    uint32_t       fldreasn;         /* +64  Reason code (output)     */
};                                   /* Total: 68 bytes               */

/* FLDFUNC function codes */
#define FLD_FUNC_ENCODE    0x01      /* Field to encoded form         */
#define FLD_FUNC_DECODE    0x02      /* Encoded to field form         */

/* FLDFLAGS bits */
#define FLD_FLG_NULL       0x80      /* Null value                    */
#define FLD_FLG_DEFAULT    0x40      /* Default value                 */

/* Data types */
#define DB2_TYPE_CHAR      448       /* CHAR                          */
#define DB2_TYPE_VARCHAR   449       /* VARCHAR                       */
#define DB2_TYPE_LONG      450       /* LONG VARCHAR                  */
#define DB2_TYPE_INTEGER   496       /* INTEGER                       */
#define DB2_TYPE_SMALLINT  500       /* SMALLINT                      */
#define DB2_TYPE_BIGINT    492       /* BIGINT                        */
#define DB2_TYPE_DECIMAL   484       /* DECIMAL                       */
#define DB2_TYPE_FLOAT     480       /* FLOAT                         */
#define DB2_TYPE_DATE      384       /* DATE                          */
#define DB2_TYPE_TIME      388       /* TIME                          */
#define DB2_TYPE_TIMESTAMP 392       /* TIMESTAMP                     */
#define DB2_TYPE_BLOB      404       /* BLOB                          */
#define DB2_TYPE_CLOB      408       /* CLOB                          */

#pragma pack()

/*===================================================================
 * DB2 Utility Functions
 *===================================================================*/

/**
 * db2_is_sysadm - Check for SYSADM privilege
 * @priv: System privilege mask
 * Returns: 1 if SYSADM, 0 otherwise
 */
static inline int db2_is_sysadm(uint16_t priv) {
    return (priv & DB2_SPRIV_SYSADM) != 0;
}

/**
 * db2_is_sysctrl - Check for SYSCTRL privilege
 * @priv: System privilege mask
 * Returns: 1 if SYSCTRL, 0 otherwise
 */
static inline int db2_is_sysctrl(uint16_t priv) {
    return (priv & DB2_SPRIV_SYSCTRL) != 0;
}

/**
 * db2_is_batch_conn - Check if batch connection
 * @type: Connection type
 * Returns: 1 if batch, 0 otherwise
 */
static inline int db2_is_batch_conn(uint8_t type) {
    return type == DB2_CONN_BATCH;
}

/**
 * db2_is_drda_conn - Check if DRDA (DDF) connection
 * @type: Connection type
 * Returns: 1 if DRDA, 0 otherwise
 */
static inline int db2_is_drda_conn(uint8_t type) {
    return type == DB2_CONN_DDF;
}

/**
 * db2_match_auth - Compare authorization IDs
 * @auth1: First auth ID (8 chars)
 * @auth2: Second auth ID (8 chars)
 * Returns: 1 if equal, 0 otherwise
 */
static inline int db2_match_auth(const char auth1[8], const char auth2[8]) {
    return match_field(auth1, auth2, 8);
}

/**
 * db2_match_plan - Compare plan names
 * @plan1: First plan name (8 chars)
 * @plan2: Second plan name (8 chars)
 * Returns: 1 if equal, 0 otherwise
 */
static inline int db2_match_plan(const char plan1[8], const char plan2[8]) {
    return match_field(plan1, plan2, 8);
}

/**
 * db2_has_table_priv - Check for table privilege
 * @priv: Table privilege mask
 * @check: Privilege to check
 * Returns: 1 if has privilege, 0 otherwise
 */
static inline int db2_has_table_priv(uint16_t priv, uint16_t check) {
    return (priv & check) != 0;
}

/**
 * db2_conn_type_name - Get connection type name
 * @type: Connection type code
 * Returns: Static string name
 */
static inline const char *db2_conn_type_name(uint8_t type) {
    switch (type) {
        case DB2_CONN_BATCH:  return "BATCH";
        case DB2_CONN_TSO:    return "TSO";
        case DB2_CONN_CICS:   return "CICS";
        case DB2_CONN_IMS:    return "IMS";
        case DB2_CONN_RRSAF:  return "RRSAF";
        case DB2_CONN_DDF:    return "DDF";
        case DB2_CONN_CAF:    return "CAF";
        default:              return "UNKNOWN";
    }
}

/*===================================================================
 * Example Usage
 *===================================================================*

// Connection exit - custom connection handling
#pragma prolog(my_dsnx_xac,"SAVE(14,12),LR(12,15)")
#pragma epilog(my_dsnx_xac,"RETURN(14,12)")

int my_dsnx_xac(struct db2_xac_parm *parm) {
    if (parm->xacfunc == XAC_FUNC_CONNECT) {
        // Log all DRDA connections
        if (parm->xactype == DB2_CONN_DDF) {
            // log_drda_connection(parm);
        }

        // Check for restricted plans
        if (db2_match_plan(parm->xacplan, "ADMIN   ")) {
            // Could check authorization here
            // if (!is_admin(parm->xacauth)) {
            //     parm->xacreasn = 100;
            //     return DB2_XAC_REJECT;
            // }
        }
    }

    return DB2_XAC_CONTINUE;
}

// Authorization exit - custom authorization
#pragma prolog(my_dsn3_ath,"SAVE(14,12),LR(12,15)")
#pragma epilog(my_dsn3_ath,"RETURN(14,12)")

int my_dsn3_ath(struct db2_ath_parm *parm) {
    // Check for sensitive tables
    if (parm->athfunc == ATH_FUNC_TABLE) {
        if (memcmp_inline(parm->athobj, "PAYROLL", 7) == 0) {
            // Custom authorization check
            // if (!is_authorized_for_payroll(parm->athauth)) {
            //     parm->athreasn = 200;
            //     return DB2_ATH_REJECT;
            // }
        }
    }

    // Defer to normal DB2 authorization
    return DB2_ATH_CONTINUE;
}

// Sign-on exit - additional authentication
#pragma prolog(my_dsn3_sgn,"SAVE(14,12),LR(12,15)")
#pragma epilog(my_dsn3_sgn,"RETURN(14,12)")

int my_dsn3_sgn(struct db2_sgn_parm *parm) {
    if (parm->sgnfunc == SGN_FUNC_SIGNON) {
        // Log all sign-on attempts
        // log_signon(parm->sgnauth, parm->sgntype, parm->sgnjob);

        // Check for time restrictions on DRDA connections
        if (parm->sgntype == DB2_CONN_DDF) {
            // if (is_restricted_time()) {
            //     parm->sgnreasn = 300;
            //     return DB2_SGN_REJECT;
            // }
        }
    }

    // Defer to RACF/ACF2
    return DB2_SGN_DEFER;
}

// Edit procedure - data encryption example
#pragma prolog(my_editproc,"SAVE(14,12),LR(12,15)")
#pragma epilog(my_editproc,"RETURN(14,12)")

int my_editproc(struct db2_edit_parm *parm) {
    if (parm->editfunc == EDIT_FUNC_ENCODE) {
        // Encrypt data on write
        // encrypt_data(parm->editdata, parm->editdlen,
        //              parm->editout, &parm->editoact);
        return DB2_EDIT_OK;
    }

    if (parm->editfunc == EDIT_FUNC_DECODE) {
        // Decrypt data on read
        // decrypt_data(parm->editdata, parm->editdlen,
        //              parm->editout, &parm->editoact);
        return DB2_EDIT_OK;
    }

    return DB2_EDIT_OK;
}

 *===================================================================*/

#endif /* METALC_DB2_H */
