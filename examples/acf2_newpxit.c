/*********************************************************************
 * MODULE:    NEWPXIT
 * FUNCTION:  ACF2 Custom Password Validation Exit
 *
 * This exit enforces custom password quality rules:
 *   1) Password must not contain the logonid (case-insensitive)
 *   2) Password must contain at least one digit
 *
 * Exit Point: LGNPW - ACF2 Password Validation
 *
 * Return: ACF2_PWD_ACCEPT (0) - accept password
 *         ACF2_PWD_REJECT (4) - reject password
 *********************************************************************/

#include "metalc_base.h"
#include "metalc_acf2.h"

/* Linkage for the entry point */
#pragma prolog(NEWPXIT, "SAVE(14,12),LR(12,15)")
#pragma epilog(NEWPXIT, "RETURN(14,12)")

/**
 * NEWPXIT - Custom Password Exit
 */
int NEWPXIT(void **parmlist) {
    struct acvald *vp = (struct acvald *)parmlist[0];

    /* Only check if a new password is being set */
    if (TM_NONE(vp->acvalflg, ACVALF_NEWPWD)) {
        return ACF2_RC_ALLOW;
    }

    /*---------------------------------------------------------------
     * Rule 1: Password must not contain the logonid
     * Uses acf2_pwd_contains_userid utility from metalc_acf2.h
     *---------------------------------------------------------------*/
    if (acf2_pwd_contains_userid(vp->acvalnpw, 8, vp->acvallid)) {
        acf2_set_reason(vp, ACF2_RSN_PWD_USERID);
        WTO_LITERAL("NEWPXIT: Password rejected - contains logonid");
        return ACF2_PWD_REJECT;
    }

    /*---------------------------------------------------------------
     * Rule 2: Password must contain at least one digit
     *---------------------------------------------------------------*/
    int has_digit = 0;
    for (int i = 0; i < 8; i++) {
        if (vp->acvalnpw[i] >= '0' && vp->acvalnpw[i] <= '9') {
            has_digit = 1;
            break;
        }
    }

    if (!has_digit) {
        acf2_set_reason(vp, ACF2_RSN_PWD_NO_NUMERIC);
        WTO_LITERAL("NEWPXIT: Password rejected - must contain a digit");
        return ACF2_PWD_REJECT;
    }

    /* All rules passed */
    return ACF2_PWD_ACCEPT;
}
