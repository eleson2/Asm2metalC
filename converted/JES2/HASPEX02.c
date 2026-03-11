/*********************************************************************
 * MODULE:    HASPEX02 (entry: EXIT02)
 * FUNCTION:  JES2 Job Statement Scan Exit
 *
 * ASM source:  asm/JES2/HASPEX02.asm
 * Product:     JES2
 * Exit point:  Exit 2 — JOB Statement Scan
 *
 * DSECT → struct mapping:
 *   $JCT     → struct jct        (includes/metalc_jes2.h)
 *   EXITWORK → struct exit02w    (local, defined below)
 *
 * Register mapping at entry:
 *   R0  = statement_type  (int)         — 0=initial, 4=continuation
 *   R1  = parmlist        (void **)     — 3-word parameter list
 *   R10 = jct             (struct jct *)— JCT address
 *   R11 = HCT address (not used in this exit)
 *   R13 = PCE address     (implicit)
 *   R12 = base register (established by prolog)
 *   R15 = return code (set by epilog)
 *
 * SCOPE REDUCTION — Functions in ASM NOT present in this C conversion:
 *   Function 1: SWBTJCT extension block initialisation
 *     ASM initialises fields in the JES2 SWBT (Scheduler Work Block)
 *     extension when a new JCT is allocated.  Requires SWBT DSECT,
 *     not yet mapped in headers.  Must be added before this module
 *     can fully replace the ASM version.
 *   Function 3: MSGCLASS= scan
 *     ASM scans for MSGCLASS= keyword and extracts message class.
 *     Out of scope for initial delivery.
 *   See concern C3 in: docs/verification-matrices/HASPEX02_jes2.md
 *
 * Verification matrix: docs/verification-matrices/HASPEX02_jes2.md
 *
 * Return: RC=0 (JES2_RC_CONTINUE) — Continue normal processing
 *
 * Build: xlc -qmetal -S -qlist -I./includes converted/JES2/HASPEX02.c
 *
 * Attributes: REENTRANT, AMODE 31, RMODE ANY
 *********************************************************************/

#include "metalc_base.h"
#include "metalc_jes2.h"

/*===================================================================
 * Exit 02 Workarea Mapping
 * ASM: EXITWORK DSECT
 *===================================================================*/
#pragma pack(1)
struct exit02w {
    int32_t        retcode;       /* +0   Return code           */
    char          *jobbufp;       /* +4   Job statement buffer  */
    char           jclass;        /* +8   Actual jobclass       */
    char           jobclass[8];   /* +9   Symbolic jobclass     */
    int16_t        jclassl;       /* +17  Symbolic length       */
    /* WTO message area */
    uint16_t       msgident;      /* +19  Message identifier    */
    char           msgjobid[8];   /* +21  Job ID from JCT       */
    char           _space1;       /* +29  Space                 */
    char           msgjnam[8];    /* +30  Job name from JCT     */
    char           _space2;       /* +38  Space                 */
    char           msgsymj[8];    /* +39  Symbolic jobclass     */
};                                /* Total: 47 bytes            */
#pragma pack()

/*===================================================================
 * EXIT02 / HASPEX02 - JES2 Job Statement Scan Exit Entry Point
 *===================================================================*/

#pragma prolog(EXIT02, "SAVE(14,12),LR(12,15)")
#pragma epilog(EXIT02, "RETURN(14,12)")

int EXIT02(int statement_type, void **parmlist, struct jct *jct) {
    struct exit02w *work;
    char *buffer;
    int rc = JES2_RC_CONTINUE;

    /*---------------------------------------------------------------
     * ASM: EX02GETW - $GETWORK macro (GETMAIN equivalent)
     * Obtain a module workarea
     *---------------------------------------------------------------*/
    work = (struct exit02w *)getmain(sizeof(struct exit02w), SUBPOOL_JOB_STEP);
    if (work == NULL) {
        return JES2_RC_CONTINUE;
    }
    /* ASM: EX02CLRW - XC WORKAREA,WORKAREA */
    memset_inline(work, 0, sizeof(struct exit02w));

    /* ASM: EX02BUF - L R2,0(,R1)  load job statement buffer pointer */
    buffer = (char *)parmlist[0];
    work->jobbufp = buffer; /* +4 */

    /* ASM: EX02TYPE - LTR R0,R0 / BNZ EX02DONE
     * Only process initial JOB statement (statement_type == 0)      */
    if (statement_type != 0) {
        goto CLEANUP;
    }

    /*---------------------------------------------------------------
     * ASM: EX02SCAN - Search for CLASS= keyword
     *   CLC 0(7,R4),=C',CLASS='  BE EX02FND
     *   CLC 0(7,R4),=C' CLASS='  BE EX02FND
     *---------------------------------------------------------------*/
    char *class_ptr = NULL;
    for (int i = 0; i < 64; i++) {
        if (match_prefix(&buffer[i], ",CLASS=", 7) ||
            match_prefix(&buffer[i], " CLASS=", 7)) {
            /* ASM: EX02FND - LA R5,7(,R4)  point past "CLASS=" */
            class_ptr = &buffer[i + 7];
            break;
        }
    }

    if (class_ptr == NULL) {
        goto CLEANUP;
    }

    /*---------------------------------------------------------------
     * ASM: EX02CHKC - CLI 1(R5),C' ' / BE EX02DONE
     *   Check second character of class value.
     *   class_ptr[1] == ASM 1(R5) — same byte, no off-by-one.
     *   If delimiter follows immediately, this is a 1-char class;
     *   JES2 handles it directly, so skip symbolic extraction.
     *---------------------------------------------------------------*/
    if (class_ptr[1] == ' ' || class_ptr[1] == ',') {
        goto CLEANUP; /* Simple class - skip extraction */
    }

    /*---------------------------------------------------------------
     * ASM: EX02LEN - Loop counting symbolic class length
     * ASM: EX02COPY - MVC WORKJCLS,0(R5)
     * ASM: EX02PAD  - MVI ...,' '
     *---------------------------------------------------------------*/
    int sym_len = 0;
    while (sym_len < 8 && class_ptr[sym_len] != ' ' && class_ptr[sym_len] != ',') {
        sym_len++;
    }

    if (sym_len > 0) {
        work->jclassl = (int16_t)sym_len;          /* +17 */
        memcpy_inline(work->jobclass, class_ptr, (size_t)sym_len); /* +9 */
        pad_blank(work->jobclass, (size_t)sym_len, 8);

        /*-----------------------------------------------------------
         * ASM: EX02MSG - Build WTO message in work area
         * ASM: EX02WTO - WTO macro
         *-----------------------------------------------------------*/
        work->msgident = 0x900F;                          /* +19 */
        memcpy_inline(work->msgjobid, jct->jctid, 8); /* +21 — jct->jctid (+0) */
        work->_space1 = ' ';                              /* +29 */
        memcpy_inline(work->msgjnam, jct->jctjname, 8);  /* +30 — jctjname (+6) */
        work->_space2 = ' ';                              /* +38 */
        memcpy_inline(work->msgsymj, work->jobclass, 8); /* +39 */

        wto_simple((char *)&work->msgident, sizeof(work->msgident) + 26);
    }

CLEANUP:
    /*---------------------------------------------------------------
     * ASM: EX02DONE - $RETWORK (FREEMAIN equivalent)
     *---------------------------------------------------------------*/
    freemain(work, sizeof(struct exit02w), SUBPOOL_JOB_STEP);
    return rc; /* ASM: EX02RET - SR R15,R15 / BR R14 - RC=0 */
}
