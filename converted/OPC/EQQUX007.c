/*********************************************************************
 * MODULE:    EQQUX007
 * FUNCTION:  OPC/TWS Operation Status Change Exit
 *
 * Converted from: asm/OPC/EQQUX007.asm
 *
 * Called by TWS when an operation changes status. This exit:
 *   1) Logs error status changes for critical path operations
 *   2) Issues WTO alert for critical path jobs ending in error
 *   3) Allows all status changes to proceed
 *
 * Exit Point: EQQUX007 - Operation Status Change
 *
 * Return: RC=0 - Continue normal processing
 *
 * Build: Use IBM Metal C compiler with no LE dependencies
 *
 * Attributes: REENTRANT, AMODE 31, RMODE ANY
 *
 * Register Mapping:
 *   R1  = parm (struct opc_ux007_parm *)
 *   R12 = Base register
 *********************************************************************/

#include "metalc_base.h"
#include "metalc_opc.h"

/*===================================================================
 * EQQUX007 - Operation Status Change Exit Entry Point
 *===================================================================*/

#pragma prolog(EQQUX007, "SAVE(14,12),LR(12,15)")
#pragma epilog(EQQUX007, "RETURN(14,12)")

int EQQUX007(struct opc_ux007_parm *parm) {

    /*---------------------------------------------------------------
     * Check function code - only process status changes (L R2,0(,R10))
     *---------------------------------------------------------------*/
    if (parm->ux007func != OPC_UX007_STATUS) {
        return OPC_UX007_CONTINUE;
    }

    /*---------------------------------------------------------------
     * Check if new status is Error ('E')
     *---------------------------------------------------------------*/
    if (parm->ux007newst != OPC_STATUS_ERROR) {
        return OPC_UX007_CONTINUE;
    }

    /*---------------------------------------------------------------
     * Operation ended in error - get operation info
     *---------------------------------------------------------------*/
    struct opc_oper *oper = parm->ux007oper;
    if (oper == NULL) {
        return OPC_UX007_CONTINUE;
    }

    /*---------------------------------------------------------------
     * Check if operation is on the critical path (TM 71(R3),X'80')
     *---------------------------------------------------------------*/
    if (opc_is_critical_path(oper)) {
        /* Critical path error - issue WTO alert */
        char msg[80];
        int pos = 0;
        msg_append_str(msg, &pos, "EQQUX007 CRITICAL PATH ERROR: ");
        msg_append_field(msg, &pos, oper->op_jobname, 8);
        msg_append_str(msg, &pos, " ENDED IN ERROR");

        wto_write(msg, pos, WTO_ROUTE_MASTER_CONSOLE | WTO_ROUTE_PROGRAMMER_INFO, 
                  WTO_DESC_EVENTUAL_ACTION);
    }

    /*---------------------------------------------------------------
     * Return RC=0 - continue processing
     *---------------------------------------------------------------*/
    return OPC_UX007_CONTINUE;
}
