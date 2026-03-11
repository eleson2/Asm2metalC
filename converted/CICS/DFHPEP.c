/*********************************************************************
 * MODULE:    DFHPEP
 * FUNCTION:  CICS Program Error Program
 *
 * Converted from: asm/CICS/DFHPEP.asm
 *
 * Called by CICS when a program check (ASRA), operating system
 * abend (ASRB), or runaway task (AICA) occurs. This exit:
 *   1) Logs the abend information (transaction, program, abend code)
 *   2) Issues a WTO for ASRA abends in production transactions
 *   3) Returns to let CICS perform normal abend processing
 *
 * Exit Point: DFHPEP - Program Error Program Exit
 *
 * Return: CICS_PEP_CONTINUE (0) - continue normal abend processing
 *         CICS_PEP_RETRY (4)    - retry the operation
 *         CICS_PEP_SUPPRESS (8) - suppress the abend
 *
 * Build: Use IBM Metal C compiler with no LE dependencies
 *   xlc -qmetal -S DFHPEP.c
 *   as -o DFHPEP.o DFHPEP.s
 *   ld -o DFHPEP DFHPEP.o
 *
 * Attributes: REENTRANT, AMODE 31, RMODE ANY
 *
 * Register Mapping:
 *   R1  = parm (struct dfhpeppar *)
 *   R12 = Base register
 *********************************************************************/

#include "metalc_base.h"
#include "metalc_cics.h"

/*===================================================================
 * DFHPEP - CICS Program Error Program Exit Entry Point
 *===================================================================*/

#pragma prolog(DFHPEP, "SAVE(14,12),LR(12,15)")
#pragma epilog(DFHPEP, "RETURN(14,12)")

int DFHPEP(struct dfhpeppar *parm) {

    /*---------------------------------------------------------------
     * Check error type (CLI 36(R10), 1)
     *---------------------------------------------------------------*/
    if (parm->peptype == PEP_TYPE_ASRA) {
        /* ASRA - Program check abend */
        
        /* Skip alert for CICS system transactions */
        if (match_field(parm->peptran, "CECI", 4) ||
            match_field(parm->peptran, "CECS", 4) ||
            match_field(parm->peptran, "CEBR", 4)) {
            return CICS_PEP_CONTINUE;
        }

        /* Issue WTO for production ASRA */
        char msg[80];
        int pos = 0;
        msg_append_str(msg, &pos, "DFHPEP ASRA TRAN=");
        msg_append_field(msg, &pos, parm->peptran, 4);
        msg_append_str(msg, &pos, " PGM=");
        msg_append_field(msg, &pos, parm->pepprog, 8);
        msg_append_str(msg, &pos, " PROGRAM CHECK");

        wto_write(msg, pos, WTO_ROUTE_MASTER_CONSOLE | WTO_ROUTE_PROGRAMMER_INFO, 0);

    } else if (parm->peptype == PEP_TYPE_AICA) {
        /* AICA - Runaway task */
        
        char msg[80];
        int pos = 0;
        msg_append_str(msg, &pos, "DFHPEP AICA TRAN=");
        msg_append_field(msg, &pos, parm->peptran, 4);
        msg_append_str(msg, &pos, " PGM=");
        msg_append_field(msg, &pos, parm->pepprog, 8);
        msg_append_str(msg, &pos, " RUNAWAY TASK");

        wto_write(msg, pos, WTO_ROUTE_MASTER_CONSOLE | WTO_ROUTE_PROGRAMMER_INFO, 0);
    }

    /*---------------------------------------------------------------
     * Return RC=0 - let CICS continue with normal abend processing
     *---------------------------------------------------------------*/
    return CICS_PEP_CONTINUE;
}
