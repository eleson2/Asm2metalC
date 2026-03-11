/*********************************************************************
 * MODULE:    AOFEXC02
 * FUNCTION:  SA Resource State Change Exit
 *
 * Converted from: asm/SA/AOFEXC02.asm
 *
 * Called by System Automation when a monitored resource changes
 * state.
 *
 * This example:
 *   - Issues WTO for critical resource state changes
 *   - Suppresses auto-recovery during maintenance windows
 *   - Logs all state transitions to HARDDOWN
 *
 * Exit Point: AOFEXC02 - Resource State Change Exit
 *
 * Return: SA_RES_CONTINUE (0) - continue normal processing
 *         SA_RES_SUPPRESS (4) - suppress action
 *
 * Build: Use IBM Metal C compiler with no LE dependencies
 *
 * Attributes: REENTRANT, AMODE 31, RMODE ANY
 *
 * Register Mapping:
 *   R1  = parm (struct sa_res_parm *)
 *   R3  = res (struct sa_resource *)
 *   R12 = Base register
 *********************************************************************/

#include "metalc_base.h"
#include "metalc_sa.h"

/*===================================================================
 * AOFEXC02 - SA Resource State Change Exit Entry Point
 *===================================================================*/

#pragma prolog(AOFEXC02, "SAVE(14,12),LR(12,15)")
#pragma epilog(AOFEXC02, "RETURN(14,12)")

int AOFEXC02(struct sa_res_parm *parm) {
    struct sa_resource *res;

    /*---------------------------------------------------------------
     * Check function code - only process state changes (func=3)
     *---------------------------------------------------------------*/
    if (parm->func != SA_FUNC_STATCHG) {
        return SA_RES_CONTINUE;
    }

    /*---------------------------------------------------------------
     * Get resource block pointer
     *---------------------------------------------------------------*/
    res = parm->sares;
    if (res == NULL) {
        return SA_RES_CONTINUE;
    }

    /*---------------------------------------------------------------
     * Check if transitioning to HARDDOWN (sanewstate = 0x01)
     *---------------------------------------------------------------*/
    if (parm->sanewstate == SA_STATE_HARDDOWN) {
        char msg[100];
        int pos = 0;
        msg_append_str(msg, &pos, "AOFEXC02 HARDDOWN RES=");
        msg_append_field(msg, &pos, res->resname, 32);
        msg_append_str(msg, &pos, " JOB=");
        msg_append_field(msg, &pos, res->jobname, 8);

        wto_write(msg, pos, WTO_ROUTE_MASTER_CONSOLE | WTO_ROUTE_SYSTEM_ERROR, 0);
    }

    /*---------------------------------------------------------------
     * Check if this is a critical resource
     *---------------------------------------------------------------*/
    if (TM_ALL(res->flags, SA_RES_FLG_CRITICAL)) {
        /* Critical resource state change - check for failure states */
        if (parm->sanewstate == SA_STATE_PROBLEM ||
            parm->sanewstate == SA_STATE_HARDDOWN ||
            parm->sanewstate == SA_STATE_DEGRADED) {
            
            char msg[80];
            int pos = 0;
            msg_append_str(msg, &pos, "AOFEXC02 CRITICAL ALERT RES=");
            msg_append_field(msg, &pos, res->resname, 32);

            wto_write(msg, pos, WTO_ROUTE_MASTER_CONSOLE | WTO_ROUTE_SYSTEM_SECURITY, 
                      WTO_DESC_SYSTEM_FAILURE);
        }
    }

    /*---------------------------------------------------------------
     * Check maintenance window - suppress auto-recovery if active
     * (TM flags, SA_FLG_RECOVERY)
     *---------------------------------------------------------------*/
    if (TM_ALL(parm->reserved, SA_FLG_RECOVERY)) {
        /* Recovery in progress - check automation flag */
        if (res->automation == SA_AUTO_ASSIST) {
            /* In assist mode during recovery - suppress automatic action */
            return SA_RES_SUPPRESS;
        }
    }

    return SA_RES_CONTINUE;
}
