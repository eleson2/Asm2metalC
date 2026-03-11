/*********************************************************************
 * MODULE:    IEFDB401
 * FUNCTION:  DFSMS Dynamic Allocation Installation Exit
 *
 * Converted from: asm/DFSMS/IEFDB401.asm
 *
 * Called by the allocation routines before and after allocation.
 * This exit can modify allocation parameters (unit, volume,
 * space, SMS classes).
 *
 * This example:
 *   1) Redirects large allocations (>1000 cyl) to LARGE storage cls
 *   2) Enforces FAST storage class for PROD.* HLQ datasets
 *   3) Blocks allocation to restricted volumes (RSVD01, RSVD02)
 *   4) Logs allocations exceeding threshold
 *
 * Exit Point: IEFDB401 - Dynamic Allocation Exit
 *
 * Return: ALLOC_RC_CONTINUE (0) - continue unchanged
 *         ALLOC_RC_MODIFIED (4) - allocation was modified
 *         ALLOC_RC_REJECT (8)   - reject allocation
 *
 * Build: Use IBM Metal C compiler with no LE dependencies
 *   xlc -qmetal -S IEFDB401.c
 *   as -o IEFDB401.o IEFDB401.s
 *   ld -o IEFDB401 IEFDB401.o
 *
 * Attributes: REENTRANT, AMODE 31, RMODE ANY
 *
 * Register Mapping:
 *   R1  = parm (struct alloc_exit_parm *)
 *   R12 = Base register
 *********************************************************************/

#include "metalc_base.h"
#include "metalc_dfsms.h"

/* Restricted volume serials */
static const char RESTRICTED_VOLS[][6] = {
    "RSVD01",
    "RSVD02"
};
#define NUM_RESTRICTED_VOLS (sizeof(RESTRICTED_VOLS) / sizeof(RESTRICTED_VOLS[0]))

/*===================================================================
 * IEFDB401 - Dynamic Allocation Exit Entry Point
 *===================================================================*/

#pragma prolog(IEFDB401, "SAVE(14,12),LR(12,15)")
#pragma epilog(IEFDB401, "RETURN(14,12)")

int IEFDB401(struct alloc_exit_parm *parm) {

    /*---------------------------------------------------------------
     * Only process allocation requests (not unalloc/concat)
     *---------------------------------------------------------------*/
    if (parm->func != ALLOC_FUNC_ALLOC) {
        return ALLOC_RC_CONTINUE;
    }

    /*---------------------------------------------------------------
     * Rule 1: Large allocation - override storage class to LARGE
     * Check if space type is cylinders and primary > 1000
     *---------------------------------------------------------------*/
    if (parm->alcspace == ALLOC_SPACE_CYL && parm->alcpri > 1000) {
        set_fixed_string(parm->alcsclas, "LARGE", 8);

        /* Log the large allocation */
        char msg[100];
        int pos = 0;
        msg_append_str(msg, &pos, "IEFDB401 LARGE ALLOC JOB=");
        msg_append_field(msg, &pos, parm->alcjob, 8);
        msg_append_str(msg, &pos, " DSN=");
        
        /* Copy DSN until blank or max length */
        for (int i = 0; i < 44 && parm->alcdsn[i] != ' '; i++) {
            msg[pos++] = parm->alcdsn[i];
        }

        wto_important(msg, pos);
        return ALLOC_RC_MODIFIED;
    }

    /*---------------------------------------------------------------
     * Rule 2: PROD.* datasets - enforce FAST storage class
     *---------------------------------------------------------------*/
    if (sms_match_dsn_hlq(parm->alcdsn, "PROD")) {
        if (!match_field(parm->alcsclas, "FAST    ", 8)) {
            set_fixed_string(parm->alcsclas, "FAST", 8);
            return ALLOC_RC_MODIFIED;
        }
    }

    /*---------------------------------------------------------------
     * Rule 3: Block allocation to restricted volumes
     *---------------------------------------------------------------*/
    for (int i = 0; i < (int)NUM_RESTRICTED_VOLS; i++) {
        if (match_field(parm->alcvol, RESTRICTED_VOLS[i], 6)) {
            /* Log the rejection */
            char msg[80];
            int pos = 0;
            msg_append_str(msg, &pos, "IEFDB401 ALLOC REJECTED JOB=");
            msg_append_field(msg, &pos, parm->alcjob, 8);
            msg_append_str(msg, &pos, " VOL=");
            msg_append_field(msg, &pos, parm->alcvol, 6);

            wto_write(msg, pos, WTO_ROUTE_MASTER_CONSOLE, WTO_DESC_IMPORTANT_INFO);
            return ALLOC_RC_REJECT;
        }
    }

    /*---------------------------------------------------------------
     * No rules matched - continue unchanged
     *---------------------------------------------------------------*/
    return ALLOC_RC_CONTINUE;
}
