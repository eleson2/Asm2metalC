/*********************************************************************
 * MODULE:    ICHPWX01
 * FUNCTION:  RACF Password Quality Exit
 *
 * ASM source:  asm/RACF/ICHPWX01.asm
 * Product:     RACF
 * Exit point:  ICHPWX01 — Password Quality Exit
 *
 * DSECT → struct mapping:
 *   PWXPARM  → struct racf_pwx_parm  (includes/metalc_racf.h)
 *
 * Register mapping at entry:
 *   R1  = parm         (struct racf_pwx_parm *) — parameter list
 *   R3  = caller_flag  (uint8_t *)              — from PWXCALLR +28
 *   R4  = new_pw       (uint8_t *)              — new password data
 *   R5  = new_len      (uint8_t)                — new password length
 *   R6  = old_pw       (uint8_t *)              — old password data
 *   R7  = old_len      (uint8_t)                — old password length
 *   R12 = base register (established by prolog)
 *   R15 = return code (set by epilog)
 *
 * Scope notes:
 *   Rules 2, 5, 6, 7, 8 are implemented (matching ASM body).
 *   Rule #1 (minimum length) is enforced by RACF before calling this
 *   exit — not present in ASM body, not present in C.
 *   Rule #3 (first/last char non-numeric) is mentioned in the ASM
 *   module header but is NOT coded in the ASM body.  This is a
 *   PRE-EXISTING GAP in the original, not a conversion error.
 *   GETMAIN/FREEMAIN: ASM uses $GETWORK; C uses stack variables
 *   (deliberate simplification — safe for Metal C).
 *
 * Verification matrix: docs/verification-matrices/ICHPWX01_racf.md
 *
 * Return: RC=0 (RACF_PWXRC_ACCEPT) — Accept password
 *         RC=4 (RACF_PWXRC_FAIL)   — Reject password
 *
 * Build: xlc -qmetal -S -qlist -I./includes converted/RACF/ICHPWX01.c
 *
 * Attributes: REENTRANT, AMODE 31, RMODE ANY
 *********************************************************************/

#include "metalc_base.h"
#include "metalc_racf.h"

/*===================================================================
 * Restricted Password table
 * ASM: BADWORDS DS table of 8-byte blank-padded entries
 *===================================================================*/
static const char PASSTAB[][8] = {
    "JAN     ", "FEB     ", "MAR     ", "APR     ", "MAY     ", "JUN     ",
    "JUL     ", "AUG     ", "SEP     ", "OCT     ", "NOV     ", "DEC     ",
    "PASS    ", "TEST    ", "USER    ", "1234    ", "QWERTY  "
};
#define NUM_KEYWORDS (sizeof(PASSTAB) / sizeof(PASSTAB[0]))

/*===================================================================
 * ICHPWX01 - RACF Password Quality Exit Entry Point
 *===================================================================*/

#pragma prolog(ICHPWX01, "SAVE(14,12),LR(12,15)")
#pragma epilog(ICHPWX01, "RETURN(14,12)")

int ICHPWX01(struct racf_pwx_parm *parm) {
    uint8_t *new_pw;
    uint8_t  new_len;
    uint8_t *old_pw;
    uint8_t  old_len;
    uint8_t *userid;
    uint8_t  user_len;
    uint8_t *caller_flag;

    /* ASM: X01A010 - Determine who is calling (L R3,PWXCALLR) */
    caller_flag = (uint8_t *)parm->pwxcallr; /* +28 */

    /* DIVERGENCE: added NULL guard not in ASM.
     * ASM unconditionally dereferences the caller flag pointer.      */
    if (caller_flag == NULL) return RACF_PWXRC_ACCEPT;

    /* ASM: X01CALL/X01CALL2 - CLI 0(R3),PWXRINIT / CLI 0(R3),PWXPWORD */
    if (*caller_flag != PWXRINIT && *caller_flag != PWXPWORD) {
        /* ASM: X01Z900 - SR R15,R15 / BR R14 */
        return RACF_PWXRC_ACCEPT; /* ASM: X01Z900 - RC=0 */
    }

    /* ASM: X01A020 - ICM R0,B'1111',PWXNEWPW / BZ X01Z900 */
    if (parm->pwxnpass == NULL) { /* +20 */
        return RACF_PWXRC_ACCEPT; /* ASM: X01Z900 - RC=0 */
    }

    /* ASM: X01A030 - L R4,PWXNEWPW / length byte at start of field */
    /* Get new password info (RACF stores length byte at start)      */
    new_pw  = (uint8_t *)parm->pwxnpass + 1; /* +20 — skip length byte */
    new_len = *(uint8_t *)parm->pwxnpass;    /* length byte at pwxnpass+0 */

    /*---------------------------------------------------------------
     * ASM: X01A200 - Rule #2 - At Least One Non-Alpha Character
     *   TRT 0(L,R4),ALPHATAB — scan password for non-alphabetic byte
     *
     * DIVERGENCE: ASM uses a 256-byte TRT translate table where
     * alphabetic positions are 0 and all others are non-zero.
     * C uses three EBCDIC uppercase ranges instead of a table.
     * Equivalent for uppercase-only passwords (this installation).
     * See concern C2 in the verification matrix.
     *---------------------------------------------------------------*/
    int has_non_alpha = 0;
    for (int i = 0; i < new_len; i++) {
        uint8_t c = new_pw[i];
        /* ASM: ALPHATAB - A-I=0xC1-0xC9, J-R=0xD1-0xD9, S-Z=0xE2-0xE9 */
        if (!((c >= 'A' && c <= 'I') || (c >= 'J' && c <= 'R') || (c >= 'S' && c <= 'Z'))) {
            has_non_alpha = 1;
            break;
        }
    }
    if (!has_non_alpha) {
        wto_important("ICHPWX02 Password must contain at least one non-alphabetic character", 62);
        return RACF_PWXRC_FAIL; /* ASM: X01Z100 - LA R15,4 / BR R14 - RC=4 */
    }

    /*---------------------------------------------------------------
     * ASM: X01A300 - Rule #5 - Not Four Consecutive Chars of Previous
     *   ICM R0,B'1111',PWXOLDPW / BZ X01A400
     *   Inner loop: CLC 0(4,R4),0(R6)
     *---------------------------------------------------------------*/
    if (parm->pwxpass != NULL) { /* +12 */
        old_pw  = (uint8_t *)parm->pwxpass + 1; /* +12 — skip length byte */
        old_len = *(uint8_t *)parm->pwxpass;    /* length byte at pwxpass+0 */

        if (new_len >= 4 && old_len >= 4) {
            for (int i = 0; i <= new_len - 4; i++) {
                for (int j = 0; j <= old_len - 4; j++) {
                    if (match_field((char *)&new_pw[i], (char *)&old_pw[j], 4)) {
                        wto_important("ICHPWX05 Password must not contain 4 consecutive characters of previous password", 73);
                        return RACF_PWXRC_FAIL; /* ASM: X01Z200 - LA R15,4 - RC=4 */
                    }
                }
            }
        }
    }

    /*---------------------------------------------------------------
     * ASM: X01A400 - Rule #6 - Not Three Consecutive Equal Chars
     *   Call TRIPCHR subroutine / BNZ X01Z300
     *---------------------------------------------------------------*/
    if (pwd_has_triple_repeat((char *)new_pw, new_len)) {
        wto_important("ICHPWX06 Password must not contain 3 identical adjacent characters", 61);
        return RACF_PWXRC_FAIL; /* ASM: X01Z300 - LA R15,4 - RC=4 */
    }

    /*---------------------------------------------------------------
     * ASM: X01A500 - Rule #7 - Password Does not Contain User ID
     *   ICM R0,B'1111',PWXUSRID / BZ X01A600
     *   Inner loop: CLC 0(L,R4),0(R8)
     *---------------------------------------------------------------*/
    if (parm->pwxuser != NULL) { /* +4 */
        userid   = (uint8_t *)parm->pwxuser + 1; /* +4 — skip length byte */
        user_len = *(uint8_t *)parm->pwxuser;    /* length byte at pwxuser+0 */

        if (new_len >= user_len) {
            for (int i = 0; i <= new_len - user_len; i++) {
                if (match_field((char *)&new_pw[i], (char *)userid, user_len)) {
                    wto_important("ICHPWX07 Password must not contain your User ID", 46);
                    return RACF_PWXRC_FAIL; /* ASM: X01Z400 - LA R15,4 - RC=4 */
                }
            }
        }
    }

    /*---------------------------------------------------------------
     * ASM: X01A600 - Rule #8 - Password Does not Contain Reserved words
     *   Table scan against BADWORDS DS
     *---------------------------------------------------------------*/
    for (int i = 0; i < (int)NUM_KEYWORDS; i++) {
        /* Keywords are blank padded in PASSTAB, find real length */
        int kw_len = 0;
        while (kw_len < 8 && PASSTAB[i][kw_len] != ' ') kw_len++;

        if (new_len >= kw_len) {
            for (int j = 0; j <= new_len - kw_len; j++) {
                if (match_field((char *)&new_pw[j], PASSTAB[i], kw_len)) {
                    wto_important("ICHPWX08 Password has invalid character combination", 53);
                    return RACF_PWXRC_FAIL; /* ASM: X01Z500 - LA R15,4 - RC=4 */
                }
            }
        }
    }

    /* ASM: X01Z900 - SR R15,R15 / BR R14 */
    return RACF_PWXRC_ACCEPT; /* RC=0 */
}
