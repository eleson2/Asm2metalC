/*********************************************************************
 * ACF2 Password Validation Exit Example
 *
 * This exit is called during password validation to enforce custom
 * password complexity rules beyond ACF2's standard rules.
 *
 * Exit Point: LGNPW (Logon Password Validation)
 *
 * Build: Use IBM Metal C compiler with no LE dependencies
 *   xlc -qmetal -S acf2_password_exit.c
 *   as -o acf2_password_exit.o acf2_password_exit.s
 *   ld -o LGNPWEXI acf2_password_exit.o
 *
 * Installation: Define in ACF2 GSO EXITS record
 *********************************************************************/

#include "metalc_base.h"
#include "metalc_acf2.h"

/*-------------------------------------------------------------------
 * Local password rules - customize for your installation
 *-------------------------------------------------------------------*/
static const struct acf2_pwd_rules pwd_rules = {
    .min_length    = 8,       /* Minimum 8 characters               */
    .max_length    = 64,      /* Maximum 64 characters              */
    .min_alpha     = 2,       /* At least 2 alpha chars             */
    .min_numeric   = 2,       /* At least 2 numeric chars           */
    .min_special   = 1,       /* At least 1 special char            */
    .max_repeated  = 2,       /* No more than 2 repeated chars      */
    .history_count = 12,      /* Remember last 12 passwords         */
    .flags         = ACF2_PWD_RULE_MIXCASE | ACF2_PWD_RULE_NOUSERID | ACF2_PWD_RULE_NOSEQ
};

/*-------------------------------------------------------------------
 * Common weak passwords to reject
 * These are checked in addition to complexity rules
 *-------------------------------------------------------------------*/
static const char *weak_passwords[] = {
    "PASSWORD",
    "12345678",
    "QWERTYUI",
    "ABCD1234",
    "CHANGEME",
    "WELCOME1",
    "LETMEIN1",
    NULL
};

/*-------------------------------------------------------------------
 * get_password_length - Get actual length of password
 * Passwords are 8 chars, blank padded
 *-------------------------------------------------------------------*/
static size_t get_password_length(const char pwd[8]) {
    size_t len = 8;
    while (len > 0 && pwd[len - 1] == ' ') {
        len--;
    }
    return len;
}

/*-------------------------------------------------------------------
 * is_weak_password - Check against list of weak passwords
 *-------------------------------------------------------------------*/
static int is_weak_password(const char *pwd, size_t len) {
    for (int i = 0; weak_passwords[i] != NULL; i++) {
        const char *weak = weak_passwords[i];
        size_t weak_len = 0;

        /* Get length of weak password */
        while (weak[weak_len] != '\0') weak_len++;

        if (len != weak_len) continue;

        /* Case-insensitive compare */
        int match = 1;
        for (size_t j = 0; j < len; j++) {
            char p = pwd[j];
            char w = weak[j];
            /* Convert to uppercase for comparison */
            if (p >= 'a' && p <= 'z') p -= 32;
            if (w >= 'a' && w <= 'z') w -= 32;
            if (p != w) {
                match = 0;
                break;
            }
        }
        if (match) return 1;
    }
    return 0;
}

/*-------------------------------------------------------------------
 * has_keyboard_pattern - Check for keyboard patterns like QWERTY
 *-------------------------------------------------------------------*/
static int has_keyboard_pattern(const char *pwd, size_t len) {
    /* Common keyboard row patterns */
    static const char *patterns[] = {
        "QWERTY", "ASDFGH", "ZXCVBN",
        "QAZWSX", "123456", "098765",
        NULL
    };

    if (len < 4) return 0;

    for (int i = 0; patterns[i] != NULL; i++) {
        const char *pat = patterns[i];
        size_t pat_len = 0;
        while (pat[pat_len] != '\0') pat_len++;

        if (pat_len > len) continue;

        /* Look for pattern anywhere in password */
        for (size_t j = 0; j <= len - pat_len; j++) {
            int match = 1;
            for (size_t k = 0; k < pat_len; k++) {
                char p = pwd[j + k];
                char t = pat[k];
                if (p >= 'a' && p <= 'z') p -= 32;
                if (t >= 'a' && t <= 'z') t -= 32;
                if (p != t) {
                    match = 0;
                    break;
                }
            }
            if (match) return 1;
        }
    }
    return 0;
}

/*===================================================================
 * LGNPW Exit - Password Validation
 *
 * Called when a user attempts to set a new password.
 *
 * Input:
 *   parmlist[0] - Pointer to ACVALD (validation parameter block)
 *
 * Return:
 *   ACF2_PWD_ACCEPT (0) - Accept the password
 *   ACF2_PWD_REJECT (4) - Reject the password
 *   ACF2_PWD_REVOKE (8) - Reject and revoke (severe violation)
 *===================================================================*/

#pragma prolog(LGNPWEXI, "SAVE(14,12),LR(12,15)")
#pragma epilog(LGNPWEXI, "RETURN(14,12)")

int LGNPWEXI(void **parmlist) {
    struct acvald *vp;
    size_t pwd_len;
    int16_t reason;

    /* Get validation parameter block */
    vp = (struct acvald *)parmlist[0];

    /* Only check new passwords, not verification */
    if (!(vp->acvalflg & ACVALF_NEWPWD)) {
        return ACF2_PWD_ACCEPT;
    }

    /* Get the new password and its length */
    pwd_len = get_password_length(vp->acvalnpw);

    /* Check minimum length */
    if (pwd_len < pwd_rules.min_length) {
        acf2_set_reason(vp, ACF2_RSN_PWD_SHORT);
        return ACF2_PWD_REJECT;
    }

    /* Check if password contains userid */
    if (pwd_rules.flags & ACF2_PWD_RULE_NOUSERID) {
        if (acf2_pwd_contains_userid(vp->acvalnpw, pwd_len, vp->acvallid)) {
            acf2_set_reason(vp, ACF2_RSN_PWD_USERID);
            return ACF2_PWD_REJECT;
        }
    }

    /* Check for sequential characters */
    if (pwd_rules.flags & ACF2_PWD_RULE_NOSEQ) {
        if (acf2_pwd_has_sequential(vp->acvalnpw, pwd_len, 3)) {
            acf2_set_reason(vp, ACF2_RSN_PWD_SEQUENTIAL);
            return ACF2_PWD_REJECT;
        }
    }

    /* Check complexity rules */
    reason = acf2_pwd_complexity_check(vp->acvalnpw, pwd_len, &pwd_rules);
    if (reason != 0) {
        acf2_set_reason(vp, reason);
        return ACF2_PWD_REJECT;
    }

    /* Check against weak password list */
    if (is_weak_password(vp->acvalnpw, pwd_len)) {
        acf2_set_reason(vp, ACF2_RSN_PWD_DICTIONARY);
        return ACF2_PWD_REJECT;
    }

    /* Check for keyboard patterns */
    if (has_keyboard_pattern(vp->acvalnpw, pwd_len)) {
        acf2_set_reason(vp, ACF2_RSN_PWD_TRIVIAL);
        return ACF2_PWD_REJECT;
    }

    /* Password passes all checks */
    return ACF2_PWD_ACCEPT;
}
