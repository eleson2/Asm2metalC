/*********************************************************************
 * MODULE:    ISTEXCLY
 * FUNCTION:  VTAM Logon Verify Exit
 *
 * Converted from: asm/VTAM/ISTEXCLY.asm
 *
 * Called by VTAM when a logon request is received.
 * This example:
 *   - Logs all logon attempts via WTO
 *   - Rejects logons to ADMIN application from non-admin LUs
 *   - Defers all other logons to RACF
 *
 * Exit Point: ISTEXCLY - Logon Verify Exit
 *
 * Return: VTAM_LY_ACCEPT (0)   - accept logon
 *         VTAM_LY_REJECT (4)   - reject logon
 *         VTAM_LY_DEFER (8)    - defer to RACF
 *
 * Build: Use IBM Metal C compiler with no LE dependencies
 *
 * Attributes: REENTRANT, AMODE 31, RMODE ANY
 *
 * Register Mapping:
 *   R1  = parm (struct vtam_ly_parm *)
 *   R12 = Base register
 *********************************************************************/

#include "metalc_base.h"
#include "metalc_vtam.h"

/*===================================================================
 * ISTEXCLY - VTAM Logon Verify Exit Entry Point
 *===================================================================*/

#pragma prolog(ISTEXCLY, "SAVE(14,12),LR(12,15)")
#pragma epilog(ISTEXCLY, "RETURN(14,12)")

int ISTEXCLY(struct vtam_ly_parm *parm) {

    /*---------------------------------------------------------------
     * Check function code - only process logon requests (func=1)
     *---------------------------------------------------------------*/
    if (parm->func != LY_FUNC_LOGON) {
        return VTAM_LY_DEFER;
    }

    /*---------------------------------------------------------------
     * Log the logon attempt
     *---------------------------------------------------------------*/
    {
        char msg[80];
        int pos = 0;
        msg_append_str(msg, &pos, "ISTEXCLY LOGON LU=");
        msg_append_field(msg, &pos, parm->lyluname, 8);
        msg_append_str(msg, &pos, " APPL=");
        msg_append_field(msg, &pos, parm->lyappl, 8);
        msg_append_str(msg, &pos, " USER=");
        msg_append_field(msg, &pos, parm->lyuserid, 8);

        wto_write(msg, pos, WTO_ROUTE_SYSTEM_SECURITY | WTO_ROUTE_PROGRAMMER_INFO, 0);
    }

    /*---------------------------------------------------------------
     * Check if target application is ADMIN
     *---------------------------------------------------------------*/
    if (match_prefix(parm->lyappl, "ADMIN", 5)) {
        /* ADMIN application - check LU name prefix */
        if (match_prefix(parm->lyluname, "ADM", 3)) {
            /* Admin LU - defer to RACF for authentication */
            return VTAM_LY_DEFER;
        }

        /* Non-admin LU trying to access ADMIN application - reject */
        parm->lysense = VTAM_SENSE_SECURITY; /* 0x080F0000 */
        parm->lyreasn = 100;
        return VTAM_LY_REJECT;
    }

    return VTAM_LY_DEFER;
}
