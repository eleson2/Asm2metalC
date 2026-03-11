/*********************************************************************
 * MODULE:    ACF2PWX
 * FUNCTION:  ACF2 Password Validation Exit (LGNPW)
 *
 * Converted from: asm/ACF2/ACF2PWX.asm
 *
 * Called by CA ACF2 during password change requests. Enforces
 * additional password quality rules beyond standard ACF2 policy.
 *
 * Password rules enforced:
 *   1) Password must not equal the logonid (constant-time compare)
 *   2) Password must contain at least one numeric character
 *   3) Password must not contain 3 or more repeated characters
 *   4) Password must not be a common word from the reject table
 *
 * Exit Point: LGNPW - ACF2 Password Validation
 *
 * Return: ACF2_PWD_ACCEPT (0) - accept password
 *         ACF2_PWD_REJECT (4) - reject password
 *
 * Build: Use IBM Metal C compiler with no LE dependencies
 *   xlc -qmetal -S ACF2PWX.c
 *   as -o ACF2PWX.o ACF2PWX.s
 *   ld -o ACF2PWX ACF2PWX.o
 *
 * Attributes: REENTRANT, AMODE 31, RMODE ANY
 *
 * Register Mapping:
 *   R1  = parmlist (void **)
 *   R2  = vp (struct acvald *)
 *   R12 = Base register
 *********************************************************************/

#include "metalc_base.h"
#include "metalc_acf2.h"

/*===================================================================
 * Bad password list - common/trivial passwords to reject
 *===================================================================*/

static const char BAD_PASSWORDS[][8] = {
    "PASSWORD",
    "PASSW0RD",
    "12345678",
    "QWERTY  ",
    "ABCD1234",
    "LETMEIN ",
    "WELCOME ",
    "CHANGE  ",
    "SECURITY",
    "TEST1234"
};

#define NUM_BAD_PASSWORDS (sizeof(BAD_PASSWORDS) / sizeof(BAD_PASSWORDS[0]))

/*===================================================================
 * ACF2PWX - ACF2 Password Validation Exit Entry Point
 *===================================================================*/

#pragma prolog(ACF2PWX, "SAVE(14,12),LR(12,15)")
#pragma epilog(ACF2PWX, "RETURN(14,12)")

int ACF2PWX(void **parmlist) {
    struct acvald *vp = (struct acvald *)parmlist[0];

    /*---------------------------------------------------------------
     * Check if new password is provided (TM acvalflg, ACVALF_NEWPWD)
     *---------------------------------------------------------------*/
    if (TM_NONE(vp->acvalflg, ACVALF_NEWPWD)) {
        return ACF2_PWD_ACCEPT;  /* No new password - accept */
    }

    /*---------------------------------------------------------------
     * Rule 1: Password must not equal logonid
     * Uses constant-time comparison to prevent timing attacks
     *---------------------------------------------------------------*/
    if (memcmp_secure(vp->acvalnpw, vp->acvallid, 8) == 0) {
        acf2_set_reason(vp, ACF2_RSN_PWD_USERID);
        wto_write("ACF2PWX Password rejected - equals logonid", 43,
                  WTO_ROUTE_SYSTEM_SECURITY, WTO_DESC_SYSTEM_STATUS);
        return ACF2_PWD_REJECT;
    }

    /*---------------------------------------------------------------
     * Rule 2: Password must contain at least one numeric character
     *---------------------------------------------------------------*/
    int has_numeric = 0;
    for (int i = 0; i < 8; i++) {
        if (vp->acvalnpw[i] >= '0' && vp->acvalnpw[i] <= '9') {
            has_numeric = 1;
            break;
        }
    }

    if (!has_numeric) {
        acf2_set_reason(vp, ACF2_RSN_PWD_NO_NUMERIC);
        wto_important("ACF2PWX Password rejected - no numeric character", 49);
        return ACF2_PWD_REJECT;
    }

    /*---------------------------------------------------------------
     * Rule 3: No 3 consecutive identical characters
     *---------------------------------------------------------------*/
    if (pwd_has_triple_repeat(vp->acvalnpw, 8)) {
        acf2_set_reason(vp, ACF2_RSN_PWD_SEQUENTIAL);
        wto_important("ACF2PWX Password rejected - repeated characters", 48);
        return ACF2_PWD_REJECT;
    }

    /*---------------------------------------------------------------
     * Rule 4: Not a common/dictionary word
     *---------------------------------------------------------------*/
    if (pwd_in_table(vp->acvalnpw, (const char *)BAD_PASSWORDS, 8, NUM_BAD_PASSWORDS)) {
        acf2_set_reason(vp, ACF2_RSN_PWD_DICTIONARY);
        wto_important("ACF2PWX Password rejected - common word", 40);
        return ACF2_PWD_REJECT;
    }

    /*---------------------------------------------------------------
     * All rules passed - accept the password
     *---------------------------------------------------------------*/
    return ACF2_PWD_ACCEPT;
}
