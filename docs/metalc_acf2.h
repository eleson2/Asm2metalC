/*********************************************************************
 * METALC_ACF2.H - CA ACF2 (Broadcom) Exit definitions for Metal C
 * 
 * This header provides control block structures, constants, and
 * utility functions for ACF2 security exit development.
 * 
 * Supported exit types:
 *   - Logonid exits (LGNPW, LGNTST, LIDPRE, LIDPOST)
 *   - Resource exits (RESXPRE, RESXPOST)
 *   - Validation exits
 *   - CICS exits
 * 
 * Usage: 
 *   #include "metalc_base.h"
 *   #include "metalc_acf2.h"
 * 
 * IMPORTANT: Control block layouts vary by ACF2 version.
 *            Verify offsets against your installation's ACFAEMAC.
 * 
 * SECURITY WARNING: Exits affect security decisions. Test thoroughly
 *                   and review with security team before deployment.
 *********************************************************************/

#ifndef METALC_ACF2_H
#define METALC_ACF2_H

#include "metalc_base.h"

/*===================================================================
 * ACF2 Exit Return Codes
 *===================================================================*/

/* Standard validation return codes */
#define ACF2_RC_ALLOW          0     /* Allow access/continue         */
#define ACF2_RC_DENY           4     /* Deny access/reject            */
#define ACF2_RC_REVOKE         8     /* Revoke logonid (severe)       */
#define ACF2_RC_ABORT          12    /* Abort processing              */

/* Password exit return codes */
#define ACF2_PWD_ACCEPT        0     /* Accept password               */
#define ACF2_PWD_REJECT        4     /* Reject password               */
#define ACF2_PWD_REVOKE        8     /* Revoke due to violation       */

/* Resource exit return codes */
#define ACF2_RES_CONTINUE      0     /* Continue normal processing    */
#define ACF2_RES_ALLOW         4     /* Override: allow access        */
#define ACF2_RES_DENY          8     /* Override: deny access         */
#define ACF2_RES_LOG           12    /* Log only, continue            */

/*===================================================================
 * ACF2 Reason Codes (examples - customize for your installation)
 *===================================================================*/

/* Password rejection reasons */
#define ACF2_RSN_PWD_TRIVIAL     1001  /* Password too simple           */
#define ACF2_RSN_PWD_REUSED      1002  /* Password recently used        */
#define ACF2_RSN_PWD_DICTIONARY  1003  /* Dictionary word               */
#define ACF2_RSN_PWD_SEQUENTIAL  1004  /* Sequential characters         */
#define ACF2_RSN_PWD_USERID      1005  /* Contains userid               */
#define ACF2_RSN_PWD_SHORT       1006  /* Too short                     */
#define ACF2_RSN_PWD_NO_ALPHA    1007  /* No alphabetic characters      */
#define ACF2_RSN_PWD_NO_NUMERIC  1008  /* No numeric characters         */
#define ACF2_RSN_PWD_NO_SPECIAL  1009  /* No special characters         */

/* Logon rejection reasons */
#define ACF2_RSN_LGN_SUSPENDED   2001  /* Logonid suspended             */
#define ACF2_RSN_LGN_EXPIRED     2002  /* Logonid expired               */
#define ACF2_RSN_LGN_TIME        2003  /* Time restriction              */
#define ACF2_RSN_LGN_SOURCE      2004  /* Source restriction            */
#define ACF2_RSN_LGN_PROGRAM     2005  /* Program restriction           */

/* Resource rejection reasons */
#define ACF2_RSN_RES_UNAUTHORIZED 3001 /* Not authorized                */
#define ACF2_RSN_RES_TIME        3002  /* Time restriction              */
#define ACF2_RSN_RES_AUDIT       3003  /* Audit requirement             */

/*===================================================================
 * ACF2 Privilege Masks
 *===================================================================*/

/* ACUCBPRV[0] - System privileges */
#define ACF2_PRIV_SECURITY     0x80000000  /* Security officer        */
#define ACF2_PRIV_ACCOUNT      0x40000000  /* Account manager         */
#define ACF2_PRIV_AUDIT        0x20000000  /* Auditor                 */
#define ACF2_PRIV_LEADER       0x10000000  /* Group leader            */
#define ACF2_PRIV_CONSOLE      0x08000000  /* Console access          */
#define ACF2_PRIV_READALL      0x04000000  /* Read all datasets       */
#define ACF2_PRIV_WRITEALL     0x02000000  /* Write all datasets      */
#define ACF2_PRIV_ALLOCATE     0x01000000  /* Allocate any volume     */

/* ACUCBPRV[1] - Resource privileges */
#define ACF2_PRIV_BYPASS_RES   0x80000000  /* Bypass resource rules   */
#define ACF2_PRIV_BYPASS_PGM   0x40000000  /* Bypass program control  */
#define ACF2_PRIV_BYPASS_DSN   0x20000000  /* Bypass dataset rules    */

/* Custom privilege bits (installation-defined) */
#define ACF2_PRIV_OVERRIDE_TIME 0x00000080 /* Override time restrict  */
#define ACF2_PRIV_BATCH_SUBMIT 0x00000040  /* Submit batch jobs       */
#define ACF2_PRIV_TSO_LOGON    0x00000020  /* TSO logon allowed       */

/*===================================================================
 * ACVALD - Validation Parameter Block
 *===================================================================*/

#pragma pack(1)

struct acvald {
    uint8_t        acvalflg;         /* +0  Flags                     */
    uint8_t        acvalrc;          /* +1  Return code               */
    int16_t        acvalrsn;         /* +2  Reason code               */
    char           acvallid[8];      /* +4  Logonid                   */
    char           acvalnam[20];     /* +12 User name                 */
    char           _reserved1[2];    /* +32 Reserved                  */
    char           acvalpwd[8];      /* +34 Password (may be masked)  */
    char           acvalnpw[8];      /* +42 New password (may be masked) */
    char           acvalgrp[8];      /* +50 Group name                */
    uint8_t        acvalfl2;         /* +58 Additional flags          */
    uint8_t        _reserved2[5];    /* +59 Reserved                  */
};                                   /* Total: 64 bytes (verify)      */

/* ACVALFLG bits */
#define ACVALF_NEWPWD    0x80        /* New password provided         */
#define ACVALF_VERIFY    0x40        /* Verify mode only              */
#define ACVALF_CHANGE    0x20        /* Password change request       */
#define ACVALF_EXPIRED   0x10        /* Password expired              */
#define ACVALF_INTERVAL  0x08        /* Interval password             */
#define ACVALF_BATCH     0x04        /* Batch logon                   */
#define ACVALF_TSO       0x02        /* TSO logon                     */
#define ACVALF_CICS      0x01        /* CICS logon                    */

/* ACVALFL2 bits */
#define ACVALF2_MFA      0x80        /* Multi-factor auth             */
#define ACVALF2_PHRASE   0x40        /* Password phrase               */
#define ACVALF2_CERT     0x20        /* Certificate auth              */

#pragma pack()

/*===================================================================
 * ACUCB - User Control Block
 *===================================================================*/

#pragma pack(1)

struct acucb {
    char           acucblid[8];      /* +0   Logonid                  */
    char           acucbnam[20];     /* +8   User name                */
    uint8_t        acucbflg[4];      /* +28  Flag bytes               */
    uint32_t       acucbprv[8];      /* +32  Privilege masks          */
    char           acucbpgm[8];      /* +64  Default program          */
    char           acucbgrp[8];      /* +72  Default group            */
    uint32_t       acucbpwdt;        /* +80  Password change date     */
    uint16_t       acucbpwit;        /* +84  Password interval        */
    uint8_t        acucbpwvc;        /* +86  Password violation count */
    uint8_t        acucbsrc[3];      /* +87  Source restriction       */
    char           acucbdpt[8];      /* +90  Department               */
    char           acucbphn[16];     /* +98  Phone number             */
    uint32_t       acucbcrdt;        /* +114 Creation date            */
    uint32_t       acucbladt;        /* +118 Last access date         */
    uint32_t       acucblatm;        /* +122 Last access time         */
    char           _reserved[6];     /* +126 Reserved                 */
};                                   /* Total: 132 bytes (verify)     */

/* ACUCBFLG[0] bits */
#define ACUCBF_SUSPEND   0x80        /* Logonid suspended             */
#define ACUCBF_CANCEL    0x40        /* Logonid cancelled             */
#define ACUCBF_NOSTAT    0x20        /* No SMF statistics             */
#define ACUCBF_RESTRICT  0x10        /* Restricted logonid            */
#define ACUCBF_EXPIRE    0x08        /* Logonid expired               */
#define ACUCBF_IDLE      0x04        /* Idle timeout active           */
#define ACUCBF_NOPWCHG   0x02        /* Cannot change password        */
#define ACUCBF_PROGRAM   0x01        /* Program restriction           */

/* ACUCBFLG[1] bits */
#define ACUCBF1_MFA      0x80        /* MFA required                  */
#define ACUCBF1_PHRASE   0x40        /* Password phrase enabled       */
#define ACUCBF1_AUTOUID  0x20        /* Auto UID assignment           */

#pragma pack()

/*===================================================================
 * ACRESBLK - Resource Access Parameter Block
 *===================================================================*/

#pragma pack(1)

struct acresblk {
    uint8_t        acresflg;         /* +0  Flags                     */
    uint8_t        acresrc;          /* +1  Return code from ACF2     */
    int16_t        acresrsn;         /* +2  Reason code               */
    char           acresrnm[44];     /* +4  Resource name             */
    char           acrestyp[8];      /* +48 Resource type             */
    char           acreslid[8];      /* +56 Requesting logonid        */
    uint8_t        acresacc;         /* +64 Access requested          */
    uint8_t        acresfl2;         /* +65 Additional flags          */
    uint16_t       _reserved;        /* +66 Reserved                  */
    void          *acresucb;         /* +68 ACUCB pointer             */
    char           acresvol[6];      /* +72 Volume serial             */
    char           _reserved2[2];    /* +78 Reserved                  */
};                                   /* Total: 80 bytes (verify)      */

/* ACRESFLG bits */
#define ACRESF_DATASET   0x80        /* Dataset resource              */
#define ACRESF_RESOURCE  0x40        /* General resource              */
#define ACRESF_PROGRAM   0x20        /* Program resource              */
#define ACRESF_UPDATE    0x10        /* Update access                 */
#define ACRESF_LOGGED    0x08        /* Access was logged             */
#define ACRESF_CACHED    0x04        /* Rule was cached               */

/* ACRESACC - Access type */
#define ACRESA_READ      0x80        /* Read access                   */
#define ACRESA_UPDATE    0x40        /* Update access                 */
#define ACRESA_ALLOCATE  0x20        /* Allocate access               */
#define ACRESA_EXECUTE   0x10        /* Execute access                */
#define ACRESA_CONTROL   0x08        /* Control access                */

#pragma pack()

/*===================================================================
 * ACSRCBLK - Source Identification Block
 *===================================================================*/

#pragma pack(1)

struct acsrcblk {
    char           acsrctyp[8];      /* +0  Source type               */
    char           acsrcid[8];       /* +8  Source identifier         */
    uint8_t        acsrcflg;         /* +16 Flags                     */
    uint8_t        _reserved[3];     /* +17 Reserved                  */
    char           acsrctrm[8];      /* +20 Terminal ID               */
    uint32_t       acsrcadr;         /* +28 IP address (if applicable)*/
};                                   /* Total: 32 bytes               */

/* Source types */
#define ACSRC_TSO        "TSO     "  /* TSO source                    */
#define ACSRC_BATCH      "BATCH   "  /* Batch source                  */
#define ACSRC_CICS       "CICS    "  /* CICS source                   */
#define ACSRC_TCPIP      "TCPIP   "  /* TCP/IP source                 */
#define ACSRC_CONSOLE    "CONSOLE "  /* Console source                */

#pragma pack()

/*===================================================================
 * ACFASVT - ACF2 Address Space Vector Table (anchor block)
 *===================================================================*/

#pragma pack(1)

struct acfasvt {
    char           acfaid[4];        /* +0  'ACF2'                    */
    uint16_t       acfaver;          /* +4  ACF2 version              */
    uint16_t       acfarel;          /* +6  ACF2 release              */
    void          *acfaucb;          /* +8  Default ACUCB             */
    void          *acfarule;         /* +12 Rule anchor               */
    uint32_t       acfaflg;          /* +16 Global flags              */
    /* Additional fields vary by version */
};

#pragma pack()

/*===================================================================
 * Password Complexity Rules
 *===================================================================*/

/* Password complexity configuration */
struct acf2_pwd_rules {
    uint8_t        min_length;       /* Minimum length                */
    uint8_t        max_length;       /* Maximum length                */
    uint8_t        min_alpha;        /* Minimum alpha characters      */
    uint8_t        min_numeric;      /* Minimum numeric characters    */
    uint8_t        min_special;      /* Minimum special characters    */
    uint8_t        max_repeated;     /* Max repeated characters       */
    uint8_t        history_count;    /* Passwords to remember         */
    uint8_t        flags;            /* Rule flags                    */
};

#define ACF2_PWD_RULE_MIXCASE  0x80   /* Require mixed case            */
#define ACF2_PWD_RULE_NODICT   0x40   /* No dictionary words           */
#define ACF2_PWD_RULE_NOUSERID 0x20   /* Cannot contain userid         */
#define ACF2_PWD_RULE_NOSEQ    0x10   /* No sequential chars           */

/*===================================================================
 * ACF2 Utility Functions
 *===================================================================*/

/**
 * acf2_is_suspended - Check if logonid is suspended
 * @ucb: Pointer to ACUCB
 * Returns: 1 if suspended, 0 otherwise
 */
static inline int acf2_is_suspended(const struct acucb *ucb) {
    return (ucb->acucbflg[0] & ACUCBF_SUSPEND) != 0;
}

/**
 * acf2_is_cancelled - Check if logonid is cancelled
 * @ucb: Pointer to ACUCB
 * Returns: 1 if cancelled, 0 otherwise
 */
static inline int acf2_is_cancelled(const struct acucb *ucb) {
    return (ucb->acucbflg[0] & ACUCBF_CANCEL) != 0;
}

/**
 * acf2_is_expired - Check if logonid is expired
 * @ucb: Pointer to ACUCB
 * Returns: 1 if expired, 0 otherwise
 */
static inline int acf2_is_expired(const struct acucb *ucb) {
    return (ucb->acucbflg[0] & ACUCBF_EXPIRE) != 0;
}

/**
 * acf2_has_privilege - Check if user has a specific privilege
 * @ucb: Pointer to ACUCB
 * @priv_index: Privilege array index (0-7)
 * @priv_mask: Privilege bit mask
 * Returns: 1 if has privilege, 0 otherwise
 */
static inline int acf2_has_privilege(const struct acucb *ucb,
                                      int priv_index,
                                      uint32_t priv_mask) {
    if (priv_index < 0 || priv_index > 7) return 0;
    return (ucb->acucbprv[priv_index] & priv_mask) != 0;
}

/**
 * acf2_is_security_officer - Check if user is security officer
 * @ucb: Pointer to ACUCB
 * Returns: 1 if security officer, 0 otherwise
 */
static inline int acf2_is_security_officer(const struct acucb *ucb) {
    return acf2_has_privilege(ucb, 0, ACF2_PRIV_SECURITY);
}

/**
 * acf2_set_reason - Set reason code in validation block
 * @vp: Pointer to ACVALD
 * @reason: Reason code to set
 */
static inline void acf2_set_reason(struct acvald *vp, int16_t reason) {
    vp->acvalrsn = reason;
}

/**
 * acf2_pwd_contains_userid - Check if password contains userid
 * @pwd: Password (8 chars, may not be null-terminated)
 * @pwd_len: Actual password length
 * @lid: Logonid (8 chars, blank padded)
 * Returns: 1 if password contains userid, 0 otherwise
 */
static inline int acf2_pwd_contains_userid(const char *pwd, 
                                            size_t pwd_len,
                                            const char lid[8]) {
    /* Get actual logonid length (strip trailing blanks) */
    size_t lid_len = 8;
    while (lid_len > 0 && lid[lid_len - 1] == ' ') {
        lid_len--;
    }
    
    if (lid_len == 0 || lid_len > pwd_len) return 0;
    
    /* Check if password contains userid anywhere */
    for (size_t i = 0; i <= pwd_len - lid_len; i++) {
        int match = 1;
        for (size_t j = 0; j < lid_len; j++) {
            char p = pwd[i + j];
            char l = lid[j];
            /* Case-insensitive compare */
            if (p >= 'a' && p <= 'z') p -= 32;
            if (l >= 'a' && l <= 'z') l -= 32;
            if (p != l) {
                match = 0;
                break;
            }
        }
        if (match) return 1;
    }
    return 0;
}

/**
 * acf2_pwd_has_sequential - Check for sequential characters
 * @pwd: Password
 * @len: Password length
 * @max_seq: Maximum allowed sequential characters
 * Returns: 1 if has too many sequential, 0 otherwise
 */
static inline int acf2_pwd_has_sequential(const char *pwd, 
                                           size_t len,
                                           int max_seq) {
    if (len < 2) return 0;
    
    int seq_count = 1;
    for (size_t i = 1; i < len; i++) {
        if (pwd[i] == pwd[i-1] + 1 || pwd[i] == pwd[i-1] - 1) {
            seq_count++;
            if (seq_count > max_seq) return 1;
        } else {
            seq_count = 1;
        }
    }
    return 0;
}

/**
 * acf2_pwd_complexity_check - Comprehensive password complexity check
 * @pwd: Password to check
 * @len: Password length
 * @rules: Complexity rules to apply
 * Returns: 0 if valid, reason code if invalid
 */
static inline int16_t acf2_pwd_complexity_check(const char *pwd,
                                                 size_t len,
                                                 const struct acf2_pwd_rules *rules) {
    if (len < rules->min_length) return ACF2_RSN_PWD_SHORT;
    
    int alpha_count = 0;
    int numeric_count = 0;
    int special_count = 0;
    int upper_count = 0;
    int lower_count = 0;
    int repeated = 1;
    int max_repeated = 1;
    
    for (size_t i = 0; i < len; i++) {
        char c = pwd[i];
        
        if (c >= 'A' && c <= 'Z') {
            alpha_count++;
            upper_count++;
        } else if (c >= 'a' && c <= 'z') {
            alpha_count++;
            lower_count++;
        } else if (c >= '0' && c <= '9') {
            numeric_count++;
        } else {
            special_count++;
        }
        
        /* Check repeated characters */
        if (i > 0 && c == pwd[i-1]) {
            repeated++;
            if (repeated > max_repeated) {
                max_repeated = repeated;
            }
        } else {
            repeated = 1;
        }
    }
    
    if (alpha_count < rules->min_alpha) return ACF2_RSN_PWD_NO_ALPHA;
    if (numeric_count < rules->min_numeric) return ACF2_RSN_PWD_NO_NUMERIC;
    if (special_count < rules->min_special) return ACF2_RSN_PWD_NO_SPECIAL;
    if (max_repeated > rules->max_repeated) return ACF2_RSN_PWD_TRIVIAL;

    if ((rules->flags & ACF2_PWD_RULE_MIXCASE) &&
        (upper_count == 0 || lower_count == 0)) {
        return ACF2_RSN_PWD_TRIVIAL;
    }
    
    return 0;  /* Password is valid */
}

/*===================================================================
 * Example Usage
 *===================================================================*

// Password validation exit - use explicit prolog/epilog pragmas
#pragma prolog(my_lgnpw,"SAVE(14,12),LR(12,15)")
#pragma epilog(my_lgnpw,"RETURN(14,12)")

int my_lgnpw(void **parmlist) {
    struct acvald *vp = (struct acvald *)parmlist[0];

    // Reject trivial passwords (password = userid)
    if (memcmp_secure(vp->acvalpwd, vp->acvallid, 8) == 0) {
        acf2_set_reason(vp, ACF2_RSN_PWD_USERID);
        return ACF2_PWD_REJECT;
    }

    // Apply complexity rules
    static const struct acf2_pwd_rules rules = {
        .min_length = 8,
        .max_length = 64,
        .min_alpha = 1,
        .min_numeric = 1,
        .min_special = 0,
        .max_repeated = 3,
        .history_count = 12,
        .flags = ACF2_PWD_RULE_NOUSERID
    };

    int16_t rsn = acf2_pwd_complexity_check(vp->acvalpwd, 8, &rules);
    if (rsn != 0) {
        acf2_set_reason(vp, rsn);
        return ACF2_PWD_REJECT;
    }

    return ACF2_PWD_ACCEPT;
}

// Resource post-processing exit
#pragma prolog(my_resxpost,"SAVE(14,12),LR(12,15)")
#pragma epilog(my_resxpost,"RETURN(14,12)")

int my_resxpost(void **parmlist) {
    struct acresblk *res = (struct acresblk *)parmlist[0];

    // Log all denied dataset accesses
    if (res->acresrc == 4 && (res->acresflg & ACRESF_DATASET)) {
        // Call logging routine
        // log_access_denial(res);
    }

    return ACF2_RES_CONTINUE;  // Don't change ACF2's decision
}

 *===================================================================*/

#endif /* METALC_ACF2_H */
