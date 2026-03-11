/*********************************************************************
 * MODULE:    IEFU83
 * FUNCTION:  SMF Record Filtering Exit
 *
 * ASM source:  asm/SMF/IEFU83.asm
 * Product:     SMF (System Management Facilities)
 * Exit point:  IEFU83 — SMF Record Write Exit (before write)
 *
 * DSECT → struct mapping:
 *   SMFHDR        → struct smf_header        (includes/metalc_smf.h)
 *   SMF30HDR      → struct smf30_header      (includes/metalc_smf.h)
 *   SMF30ID       → struct smf30_id_section  (includes/metalc_smf.h)
 *   SMF_SUBTYHDR  → struct smf_subtype_header
 *
 * Register mapping at entry:
 *   R1  = parmlist  (void **)             — address of record pointer
 *   R2  = record    (struct smf_header *) — SMF record base address
 *   R3  = (implicit in ADDR_AT_OFFSET)   — byte offset within record
 *   R12 = base register (established by prolog)
 *   R15 = return code (set by epilog)
 *
 * Scope notes:
 *   Full scope conversion.  All ASM paths are present in C.
 *
 * Verification matrix: docs/verification-matrices/IEFU83_smf.md
 *
 * Return: SMF_RC_WRITE (0)    — write the record
 *         SMF_RC_SUPPRESS (4) — suppress the record
 *
 * Build: xlc -qmetal -S -qlist -I./includes converted/SMF/IEFU83.c
 *
 * Attributes: REENTRANT, AMODE 31, RMODE ANY
 *********************************************************************/

#include "metalc_base.h"
#include "metalc_smf.h"

/*===================================================================
 * IEFU83 - SMF Record Filtering Exit Entry Point
 *===================================================================*/

#pragma prolog(IEFU83, "SAVE(14,12),LR(12,15)")
#pragma epilog(IEFU83, "RETURN(14,12)")

int IEFU83(void **parmlist) {
    struct smf_header *record;
    uint8_t type;

    /* ASM: U83INIT - L R2,0(,R1)  load SMF record address from parm */
    record = (struct smf_header *)parmlist[0];

    /* DIVERGENCE: added NULL guard not in ASM.
     * ASM unconditionally uses the loaded address.  The C guard
     * protects against a zero word in the parameter list.          */
    if (record == NULL) {
        return SMF_RC_WRITE;
    }

    /* ASM: U83TYPE - CLI 5(R2),... load record type byte */
    type = record->smfrty; /* +5 */

    /*---------------------------------------------------------------
     * ASM: U83T40/U83T42 - Check record type; suppress 40 and 42
     *   CLI 5(R2),40  BE U83SUPP
     *   CLI 5(R2),42  BE U83SUPP
     *---------------------------------------------------------------*/
    if (type == 40 || type == 42) {
        return SMF_RC_SUPPRESS; /* ASM: U83SUPP - LA R15,4 / BR R14 */
    }

    /*---------------------------------------------------------------
     * ASM: U83T30/U83T4 - Check for job name based on record type
     *---------------------------------------------------------------*/
    if (type == SMF_TYPE_30) {
        /* ASM: U83T30 - CLI 5(R2),30 / BE U83SOF */

        /* Type 30 — Identification section offset at +24
         * ASM: U83SOF - LH R3,24(R2)  (sign-extend 16-bit offset)
         * C uses uint16_t (unsigned); equivalent for offsets < 32768 */
        struct smf30_header *smf30 = (struct smf30_header *)record;
        uint16_t id_offset = smf30->smf30sof; /* +24 — ASM: LH R3,24(R2) */

        if (id_offset > 0) {
            /* ASM: U83ADDR - AR R3,R2  add base address to offset */
            struct smf30_id_section *id =
                (struct smf30_id_section *)ADDR_AT_OFFSET(record, id_offset);

            /* ASM: U83SYS - CLC 0(3,R3),=C'SYS' */
            if (match_prefix(id->smf30jbn, "SYS", 3)) {
                return SMF_RC_SUPPRESS; /* ASM: U83SUPP - LA R15,4 / BR R14 */
            }
        }
    } else if (type == SMF_TYPE_4  || type == SMF_TYPE_5 ||
               type == SMF_TYPE_14 || type == SMF_TYPE_15) {
        /* ASM: U83T4 - CLI 5(R2),4 / BE U83JBN  (and similar for 5,14,15)
         * Types with Job Name immediately after the 18-byte header (+18)
         * ASM: U83JBN - LA R3,18(,R2)  (sizeof(struct smf_header) == 18) */
        char *jobname = (char *)ADDR_AT_OFFSET(record,
                            sizeof(struct smf_header)); /* ASM: LA R3,18(,R2) */

        /* ASM: U83SYSJB - CLC 0(3,R3),=C'SYS' */
        if (match_prefix(jobname, "SYS", 3)) {
            return SMF_RC_SUPPRESS; /* ASM: U83SUPP - LA R15,4 / BR R14 */
        }
    }

    /*---------------------------------------------------------------
     * ASM: U83WRITE - SR R15,R15 / BR R14  allow record to be written
     *---------------------------------------------------------------*/
    return SMF_RC_WRITE;
}
