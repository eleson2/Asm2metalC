/*********************************************************************
 * METALC_RACF.H - RACF Exit definitions for Metal C
 *
 * This header provides control block structures, constants, and
 * utility functions for RACF security exit development.
 *
 * Supported exits:
 *   ICHRIX01/02 - RACINIT pre/post processing (sign-on)
 *   ICHRCX01/02 - RACROUTE AUTH pre/post (authorization)
 *   ICHRDX01/02 - RACDEF pre/post (profile definition)
 *   ICHRFX01/02 - RACF database I/O pre/post
 *   ICHPWX01    - Password verification exit
 *   ICHNCV00    - New password validation
 *   ICHCNX00    - RACF command processing
 *
 * Usage:
 *   #include "metalc_base.h"
 *   #include "metalc_racf.h"
 *
 * IMPORTANT: Control block layouts vary by RACF version.
 *            Verify offsets against your installation's macros.
 *
 * SECURITY WARNING: Exits affect security decisions. Test thoroughly
 *                   and review with security team before deployment.
 *********************************************************************/

#ifndef METALC_RACF_H
#define METALC_RACF_H

#include "metalc_base.h"

/*===================================================================
 * RACF Exit Return Codes
 *===================================================================*/

/* ICHRIX01/02 (RACINIT) return codes */
#define RACF_RIXRC_CONTINUE    RC_OK     /* Continue normal processing    */
#define RACF_RIXRC_BYPASS      RC_WARNING  /* Bypass RACF processing        */
#define RACF_RIXRC_FAIL        RC_ERROR    /* Fail the request              */

/* ICHRCX01/02 (RACROUTE AUTH) return codes */
#define RACF_RCXRC_CONTINUE    RC_OK     /* Continue normal processing    */
#define RACF_RCXRC_ALLOW       RC_WARNING  /* Allow access (bypass check)   */
#define RACF_RCXRC_DENY        RC_ERROR    /* Deny access                   */

/* ICHRDX01/02 (RACDEF) return codes */
#define RACF_RDXRC_CONTINUE    RC_OK     /* Continue processing           */
#define RACF_RDXRC_BYPASS      RC_WARNING  /* Bypass RACF processing        */
#define RACF_RDXRC_FAIL        RC_ERROR    /* Fail the request              */

/* ICHPWX01 (Password verification) return codes */
#define RACF_PWXRC_ACCEPT      RC_OK     /* Accept password               */
#define RACF_PWXRC_FAIL        RC_WARNING  /* Fail - use RACF default msg   */
#define RACF_PWXRC_FAILMSG     RC_ERROR    /* Fail - exit provides message  */

/* ICHNCV00 (New password validation) return codes */
#define RACF_NCVRC_ACCEPT      RC_OK     /* Accept new password           */
#define RACF_NCVRC_REJECT      RC_WARNING  /* Reject new password           */

/* ICHCNX00 (Command processing) return codes */
#define RACF_CNXRC_CONTINUE    RC_OK     /* Continue command processing   */
#define RACF_CNXRC_BYPASS      RC_WARNING  /* Bypass command processing     */
#define RACF_CNXRC_FAIL        RC_ERROR    /* Fail the command              */

/*===================================================================
 * RACF SAF Return/Reason Codes
 *===================================================================*/

/* SAF return codes */
#define SAF_RC_OK              0     /* Request successful            */
#define SAF_RC_RACF_NOT_ACTIVE 4     /* RACF not active               */
#define SAF_RC_FAILED          8     /* Request failed                */

/* RACROUTE work area size */
#define SAF_WORKA_SIZE         512   /* RACROUTE SAF work area        */

/* Common RACF return codes */
#define RACF_RC_OK             0     /* Successful                    */
#define RACF_RC_NOT_AUTH       8     /* Not authorized                */
#define RACF_RC_NOT_DEFINED    4     /* Resource not defined          */

/* Password reason codes */
#define RACF_RSN_PWD_EXPIRED   0x10  /* Password expired              */
#define RACF_RSN_PWD_REVOKED   0x14  /* User revoked                  */
#define RACF_RSN_PWD_INVALID   0x18  /* Invalid password              */
#define RACF_RSN_NEWPWD_REQ    0x1C  /* New password required         */

/*===================================================================
 * RACF Access Levels
 *===================================================================*/

#define RACF_ACCESS_NONE       0x00  /* No access                     */
#define RACF_ACCESS_EXECUTE    0x01  /* Execute                       */
#define RACF_ACCESS_READ       0x02  /* Read                          */
#define RACF_ACCESS_UPDATE     0x04  /* Update                        */
#define RACF_ACCESS_CONTROL    0x08  /* Control                       */
#define RACF_ACCESS_ALTER      0x80  /* Alter                         */

/*===================================================================
 * RACF Resource Classes
 *===================================================================*/

#define RACF_CLASS_USER        "USER    "
#define RACF_CLASS_GROUP       "GROUP   "
#define RACF_CLASS_DATASET     "DATASET "
#define RACF_CLASS_FACILITY    "FACILITY"
#define RACF_CLASS_PROGRAM     "PROGRAM "
#define RACF_CLASS_TERMINAL    "TERMINAL"
#define RACF_CLASS_SDSF        "SDSF    "
#define RACF_CLASS_OPERCMDS    "OPERCMDS"
#define RACF_CLASS_SURROGAT    "SURROGAT"
#define RACF_CLASS_STARTED     "STARTED "
#define RACF_CLASS_CICS        "CICS    "
#define RACF_CLASS_APPL        "APPL    "

/*===================================================================
 * ACEE - Accessor Environment Element
 *===================================================================*/

#pragma pack(1)

struct acee {
    char           aceeacee[4];      /* +0   'ACEE' identifier        */
    uint8_t        aceesp;           /* +4   Subpool                  */
    uint8_t        aceelen;          /* +5   Length in doublewords    */
    uint8_t        aceevrsn;         /* +6   Version                  */
    uint8_t        aceeflg1;         /* +7   Flags                    */
    void          *aceeiep;          /* +8   Installation exit parm   */
    char           aceeuser[8];      /* +12  User ID                  */
    char           aceeusri[8];      /* +20  User ID (internal)       */
    char           aceepass[8];      /* +28  Password (masked)        */
    void          *aceegrpn;         /* +36  Group name pointer       */
    char           aceedefg[8];      /* +40  Default group            */
    uint8_t        aceeflg2;         /* +48  Flags byte 2             */
    uint8_t        aceeflg3;         /* +49  Flags byte 3             */
    uint8_t        aceeflg4;         /* +50  Flags byte 4             */
    uint8_t        aceeflg5;         /* +51  Flags byte 5             */
    void          *aceeunam;         /* +52  User name pointer        */
    void          *aceetrlv;         /* +56  Terminal level pointer   */
    void          *aceeinst;         /* +60  Installation data ptr    */
    uint32_t       aceecre8;         /* +64  Creation timestamp       */
    void          *aceetokn;         /* +68  UTOKEN pointer           */
    void          *aceeproc;         /* +72  Procedure name pointer   */
    void          *aceetrmp;         /* +76  Terminal name pointer    */
    void          *aceeapts;         /* +80  APTSB pointer            */
    void          *aceefcgp;         /* +84  Field level crypto ptr   */
    /* Additional fields vary by RACF version */
};                                   /* Minimum: 88 bytes             */

/* ACEEFLG1 bits */
#define ACEEFLG1_SPEC    0x80        /* Special attribute             */
#define ACEEFLG1_ADSP    0x40        /* Automatic dataset protection  */
#define ACEEFLG1_OIDC    0x20        /* Operations attribute          */
#define ACEEFLG1_AUDIT   0x10        /* Auditor attribute             */
#define ACEEFLG1_GRPA    0x08        /* Group access attribute        */
#define ACEEFLG1_NOPW    0x04        /* No password for user          */
#define ACEEFLG1_EXPW    0x02        /* Password is expired           */
#define ACEEFLG1_PASS    0x01        /* PassTicket used               */

/* ACEEFLG2 bits */
#define ACEEFLG2_LNK     0x80        /* ACEE created by RACINIT link  */
#define ACEEFLG2_TRU     0x40        /* Trusted attribute             */
#define ACEEFLG2_PRIV    0x20        /* Privileged attribute          */
#define ACEEFLG2_PWX     0x10        /* Password exit called          */
#define ACEEFLG2_MFA     0x08        /* MFA verified                  */

/* ACEEFLG3 bits */
#define ACEEFLG3_ROAU    0x80        /* ROAUDIT attribute             */
#define ACEEFLG3_CLAU    0x40        /* CLAUTH attribute              */
#define ACEEFLG3_PROT    0x20        /* Protected user ID             */

#pragma pack()

/*===================================================================
 * RACINIT Exit Parameter List (ICHRIX01/02)
 *===================================================================*/

#pragma pack(1)

struct racf_rix_parm {
    void          *rixacee;          /* +0   ACEE pointer             */
    void          *rixwork;          /* +4   Work area pointer        */
    char          *rixuser;          /* +8   User ID pointer          */
    uint8_t        rixuserl;         /* +12  User ID length           */
    uint8_t        rixflg1;          /* +13  Flags byte 1             */
    uint8_t        rixflg2;          /* +14  Flags byte 2             */
    uint8_t        rixflg3;          /* +15  Flags byte 3             */
    char          *rixpass;          /* +16  Password pointer         */
    uint8_t        rixpassl;         /* +20  Password length          */
    uint8_t        _reserved1[3];    /* +21  Reserved                 */
    char          *rixnpass;         /* +24  New password pointer     */
    uint8_t        rixnpasl;         /* +28  New password length      */
    uint8_t        _reserved2[3];    /* +29  Reserved                 */
    char          *rixgroup;         /* +32  Group pointer            */
    uint8_t        rixgrupl;         /* +36  Group length             */
    uint8_t        _reserved3[3];    /* +37  Reserved                 */
    char          *rixappl;          /* +40  Application pointer      */
    uint8_t        rixappll;         /* +44  Application length       */
    uint8_t        _reserved4[3];    /* +45  Reserved                 */
    char          *rixterm;          /* +48  Terminal pointer         */
    uint8_t        rixterml;         /* +52  Terminal length          */
    uint8_t        _reserved5[3];    /* +53  Reserved                 */
    uint32_t       rixreasn;         /* +56  Reason code area         */
    void          *rixinstd;         /* +60  Installation data ptr    */
};                                   /* Total: 64 bytes               */

/* RIXFLG1 bits */
#define RIXFLG1_VERIFY   0x80        /* Verify only (RACINIT ENVIR)   */
#define RIXFLG1_DELETE   0x40        /* Delete ACEE                   */
#define RIXFLG1_CHANGE   0x20        /* Password change               */
#define RIXFLG1_STAT     0x10        /* STAT request                  */
#define RIXFLG1_LOGON    0x08        /* Logon request                 */
#define RIXFLG1_TRUSTED  0x04        /* Trusted request               */

#pragma pack()

/*===================================================================
 * RACROUTE AUTH Exit Parameter List (ICHRCX01/02)
 *===================================================================*/

#pragma pack(1)

struct racf_rcx_parm {
    void          *rcxacee;          /* +0   ACEE pointer             */
    void          *rcxwork;          /* +4   Work area pointer        */
    char          *rcxclass;         /* +8   Class name pointer       */
    uint8_t        rcxclsln;         /* +12  Class name length        */
    uint8_t        rcxflg1;          /* +13  Flags byte 1             */
    uint8_t        rcxflg2;          /* +14  Flags byte 2             */
    uint8_t        rcxflg3;          /* +15  Flags byte 3             */
    char          *rcxentty;         /* +16  Entity name pointer      */
    uint16_t       rcxentyl;         /* +20  Entity name length       */
    uint8_t        rcxattr;          /* +22  Access authority         */
    uint8_t        rcxcsect;         /* +23  CSECT flags              */
    void          *rcxinstd;         /* +24  Installation data ptr    */
    void          *rcxprof;          /* +28  Profile pointer          */
    void          *rcxowner;         /* +32  Owner pointer            */
    void          *rcxvol;           /* +36  Volume pointer           */
    uint8_t        rcxvolln;         /* +40  Volume length            */
    uint8_t        _reserved1[3];    /* +41  Reserved                 */
    uint32_t       rcxreasn;         /* +44  Reason code area         */
    char          *rcxlogst;         /* +48  Log string pointer       */
    uint16_t       rcxlgsln;         /* +52  Log string length        */
    uint16_t       _reserved2;       /* +54  Reserved                 */
};                                   /* Total: 56 bytes               */

/* RCXFLG1 bits */
#define RCXFLG1_LOG      0x80        /* Logging requested             */
#define RCXFLG1_GENR     0x40        /* Generic profile check         */
#define RCXFLG1_FCLASS   0x20        /* Fail if class not active      */
#define RCXFLG1_SPEC     0x10        /* Special processing            */
#define RCXFLG1_STAT     0x08        /* STATUS request                */

/* RCXATTR - Access attribute values */
#define RCXATTR_READ     0x02        /* Read access                   */
#define RCXATTR_UPDATE   0x04        /* Update access                 */
#define RCXATTR_CONTROL  0x08        /* Control access                */
#define RCXATTR_ALTER    0x80        /* Alter access                  */

#pragma pack()

/*===================================================================
 * Password Exit Parameter List (ICHPWX01)
 *===================================================================*/

#pragma pack(1)

struct racf_pwx_parm {
    void          *pwxwork;          /* +0   Work area pointer        */
    char          *pwxuser;          /* +4   User ID pointer          */
    uint8_t        pwxuserl;         /* +8   User ID length           */
    uint8_t        pwxflg1;          /* +9   Flags byte 1             */
    uint8_t        pwxflg2;          /* +10  Flags byte 2             */
    uint8_t        _reserved1;       /* +11  Reserved                 */
    char          *pwxpass;          /* +12  Password pointer         */
    uint8_t        pwxpassl;         /* +16  Password length          */
    uint8_t        _reserved2[3];    /* +17  Reserved                 */
    char          *pwxnpass;         /* +20  New password pointer     */
    uint8_t        pwxnpasl;         /* +24  New password length      */
    uint8_t        _reserved3[3];    /* +25  Reserved                 */
    void          *pwxcallr;         /* +28  Caller flag pointer      */
    void          *pwxinstd;         /* +32  Installation data ptr    */
    char          *pwxmsgp;          /* +36  Message area pointer     */
    uint16_t       pwxmsgln;         /* +40  Message area length      */
    uint16_t       _reserved4;       /* +42  Reserved                 */
};                                   /* Total: 44 bytes               */

/* Caller flags (from PWXCALLR) */
#define PWXRINIT         0x01        /* Called by RACINIT             */
#define PWXPWORD         0x02        /* Called by PASSWORD command    */
#define PWXALUSR         0x03        /* Called by ALTUSER command     */

/* PWXFLG1 bits */
#define PWXFLG1_CHANGE   0x80        /* Password change request       */
#define PWXFLG1_INTERVAL 0x40        /* Interval expired              */
#define PWXFLG1_PHRASE   0x20        /* Password phrase               */
#define PWXFLG1_VERIFY   0x10        /* Verification call             */

#pragma pack()

/*===================================================================
 * New Password Validation Parameter List (ICHNCV00)
 *===================================================================*/

#pragma pack(1)

struct racf_ncv_parm {
    void          *ncvwork;          /* +0   Work area pointer        */
    char          *ncvuser;          /* +4   User ID pointer          */
    uint8_t        ncvuserl;         /* +8   User ID length           */
    uint8_t        ncvflg1;          /* +9   Flags                    */
    uint16_t       _reserved1;       /* +10  Reserved                 */
    char          *ncvnpass;         /* +12  New password pointer     */
    uint8_t        ncvnpasl;         /* +16  New password length      */
    uint8_t        _reserved2[3];    /* +17  Reserved                 */
    char          *ncvopass;         /* +20  Old password pointer     */
    uint8_t        ncvopasl;         /* +24  Old password length      */
    uint8_t        _reserved3[3];    /* +25  Reserved                 */
    char          *ncvmsgp;          /* +28  Message area pointer     */
    uint16_t       ncvmsgln;         /* +32  Message length (output)  */
    uint16_t       ncvmsgmx;         /* +34  Message max length       */
    void          *ncvinstd;         /* +36  Installation data ptr    */
};                                   /* Total: 40 bytes               */

/* NCVFLG1 bits */
#define NCVFLG1_PHRASE   0x80        /* Password phrase               */
#define NCVFLG1_REPLAY   0x40        /* Replay detection              */

#pragma pack()

/*===================================================================
 * RACF Command Exit Parameter List (ICHCNX00)
 *===================================================================*/

#pragma pack(1)

struct racf_cnx_parm {
    void          *cnxwork;          /* +0   Work area pointer        */
    void          *cnxacee;          /* +4   Issuer's ACEE            */
    char          *cnxcmd;           /* +8   Command text pointer     */
    uint16_t       cnxcmdln;         /* +12  Command text length      */
    uint8_t        cnxflg1;          /* +14  Flags byte 1             */
    uint8_t        cnxflg2;          /* +15  Flags byte 2             */
    char          *cnxverb;          /* +16  Command verb pointer     */
    uint8_t        cnxvrbln;         /* +20  Verb length              */
    uint8_t        _reserved1[3];    /* +21  Reserved                 */
    char          *cnxprof;          /* +24  Profile name pointer     */
    uint16_t       cnxprofn;         /* +28  Profile name length      */
    uint8_t        _reserved2[2];    /* +30  Reserved                 */
    char          *cnxclass;         /* +32  Class name pointer       */
    uint8_t        cnxclsln;         /* +36  Class name length        */
    uint8_t        _reserved3[3];    /* +37  Reserved                 */
};                                   /* Total: 40 bytes               */

/* CNXFLG1 - Command type bits */
#define CNXFLG1_ADDUSER  0x80        /* ADDUSER command               */
#define CNXFLG1_ALTUSER  0x40        /* ALTUSER command               */
#define CNXFLG1_DELUSER  0x20        /* DELUSER command               */
#define CNXFLG1_ADDGRP   0x10        /* ADDGROUP command              */
#define CNXFLG1_ALTGRP   0x08        /* ALTGROUP command              */
#define CNXFLG1_DELGRP   0x04        /* DELGROUP command              */
#define CNXFLG1_PERMIT   0x02        /* PERMIT command                */
#define CNXFLG1_CONNECT  0x01        /* CONNECT command               */

/* CNXFLG2 - Additional command type bits */
#define CNXFLG2_ADDSD    0x80        /* ADDSD command                 */
#define CNXFLG2_ALTSD    0x40        /* ALTDSD command                */
#define CNXFLG2_DELSD    0x20        /* DELDSD command                */
#define CNXFLG2_RDEFINE  0x10        /* RDEFINE command               */
#define CNXFLG2_RALTER   0x08        /* RALTER command                */
#define CNXFLG2_RDELETE  0x04        /* RDELETE command               */
#define CNXFLG2_SETROPTS 0x02        /* SETROPTS command              */
#define CNXFLG2_PASSWORD 0x01        /* PASSWORD command              */

#pragma pack()

/*===================================================================
 * RACF Utility Functions
 *===================================================================*/

/**
 * racf_is_special - Check if user has SPECIAL attribute
 * @acee: Pointer to ACEE
 * Returns: 1 if SPECIAL, 0 otherwise
 */
static inline int racf_is_special(const struct acee *acee) {
    return (acee->aceeflg1 & ACEEFLG1_SPEC) != 0;
}

/**
 * racf_is_operations - Check if user has OPERATIONS attribute
 * @acee: Pointer to ACEE
 * Returns: 1 if OPERATIONS, 0 otherwise
 */
static inline int racf_is_operations(const struct acee *acee) {
    return (acee->aceeflg1 & ACEEFLG1_OIDC) != 0;
}

/**
 * racf_is_auditor - Check if user has AUDITOR attribute
 * @acee: Pointer to ACEE
 * Returns: 1 if AUDITOR, 0 otherwise
 */
static inline int racf_is_auditor(const struct acee *acee) {
    return (acee->aceeflg1 & ACEEFLG1_AUDIT) != 0;
}

/**
 * racf_is_trusted - Check if ACEE is trusted
 * @acee: Pointer to ACEE
 * Returns: 1 if trusted, 0 otherwise
 */
static inline int racf_is_trusted(const struct acee *acee) {
    return (acee->aceeflg2 & ACEEFLG2_TRU) != 0;
}

/**
 * racf_is_privileged - Check if ACEE is privileged
 * @acee: Pointer to ACEE
 * Returns: 1 if privileged, 0 otherwise
 */
static inline int racf_is_privileged(const struct acee *acee) {
    return (acee->aceeflg2 & ACEEFLG2_PRIV) != 0;
}

/**
 * racf_password_expired - Check if password is expired
 * @acee: Pointer to ACEE
 * Returns: 1 if expired, 0 otherwise
 */
static inline int racf_password_expired(const struct acee *acee) {
    return (acee->aceeflg1 & ACEEFLG1_EXPW) != 0;
}

/**
 * racf_is_protected_user - Check if user is protected
 * @acee: Pointer to ACEE
 * Returns: 1 if protected, 0 otherwise
 */
static inline int racf_is_protected_user(const struct acee *acee) {
    return (acee->aceeflg3 & ACEEFLG3_PROT) != 0;
}

/**
 * racf_get_userid - Get user ID from ACEE (8 chars, blank padded)
 * @acee: Pointer to ACEE
 * @userid: Output buffer (8 bytes minimum)
 */
static inline void racf_get_userid(const struct acee *acee, char userid[8]) {
    memcpy_inline(userid, acee->aceeuser, 8);
}

/**
 * racf_get_group - Get default group from ACEE (8 chars, blank padded)
 * @acee: Pointer to ACEE
 * @group: Output buffer (8 bytes minimum)
 */
static inline void racf_get_group(const struct acee *acee, char group[8]) {
    memcpy_inline(group, acee->aceedefg, 8);
}

/**
 * racf_validate_password_strength - Basic password strength check
 * @password: Password to validate
 * @length: Password length
 * @userid: User ID (to check if password contains it)
 * @userid_len: User ID length
 * Returns: 0 if valid, non-zero reason code if invalid
 */
static inline int racf_validate_password_strength(const char *password,
                                                   size_t length,
                                                   const char *userid,
                                                   size_t userid_len) {
    if (length < 8) return 1;   /* Too short */

    int has_alpha = 0, has_numeric = 0, has_special = 0;

    for (size_t i = 0; i < length; i++) {
        char c = password[i];
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) {
            has_alpha = 1;
        } else if (c >= '0' && c <= '9') {
            has_numeric = 1;
        } else {
            has_special = 1;
        }
    }

    if (!has_alpha) return 2;   /* No alphabetic */
    if (!has_numeric) return 3; /* No numeric */

    /* Check if password contains userid (case-insensitive) */
    if (userid_len <= length) {
        for (size_t i = 0; i <= length - userid_len; i++) {
            int match = 1;
            for (size_t j = 0; j < userid_len; j++) {
                char p = password[i + j];
                char u = userid[j];
                if (p >= 'a' && p <= 'z') p -= 32;
                if (u >= 'a' && u <= 'z') u -= 32;
                if (p != u) { match = 0; break; }
            }
            if (match) return 4; /* Contains userid */
        }
    }

    return 0; /* Valid */
}

/*===================================================================
 * Example Usage
 *===================================================================*

// RACINIT pre-processing exit
#pragma prolog(my_ichrix01,"SAVE(14,12),LR(12,15)")
#pragma epilog(my_ichrix01,"RETURN(14,12)")

int my_ichrix01(struct racf_rix_parm *parm) {
    // Log all logon attempts
    if (parm->rixflg1 & RIXFLG1_LOGON) {
        // Custom logging logic here
    }

    // Allow RACF to continue processing
    return RACF_RIXRC_CONTINUE;
}

// Password verification exit
#pragma prolog(my_ichpwx01,"SAVE(14,12),LR(12,15)")
#pragma epilog(my_ichpwx01,"RETURN(14,12)")

int my_ichpwx01(struct racf_pwx_parm *parm) {
    // Additional password validation
    int rc = racf_validate_password_strength(
        parm->pwxpass, parm->pwxpassl,
        parm->pwxuser, parm->pwxuserl);

    if (rc != 0) {
        return RACF_PWXRC_FAIL;
    }

    return RACF_PWXRC_ACCEPT;
}

// New password validation exit
#pragma prolog(my_ichncv00,"SAVE(14,12),LR(12,15)")
#pragma epilog(my_ichncv00,"RETURN(14,12)")

int my_ichncv00(struct racf_ncv_parm *parm) {
    // Enforce password complexity
    int rc = racf_validate_password_strength(
        parm->ncvnpass, parm->ncvnpasl,
        parm->ncvuser, parm->ncvuserl);

    if (rc != 0) {
        // Could set custom message in parm->ncvmsgp
        return RACF_NCVRC_REJECT;
    }

    return RACF_NCVRC_ACCEPT;
}

// Authorization checking exit
#pragma prolog(my_ichrcx02,"SAVE(14,12),LR(12,15)")
#pragma epilog(my_ichrcx02,"RETURN(14,12)")

int my_ichrcx02(struct racf_rcx_parm *parm) {
    struct acee *acee = (struct acee *)parm->rcxacee;

    // Log all access denials for auditing
    if (parm->rcxreasn != 0) {
        // log_access_denial(acee, parm);
    }

    return RACF_RCXRC_CONTINUE;
}

 *===================================================================*/

#endif /* METALC_RACF_H */
