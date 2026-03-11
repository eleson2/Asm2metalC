/*********************************************************************
 * MODULE:    DSN3ATH
 * FUNCTION:  DB2 Authorization Exit
 *
 * Converted from: asm/DB2/DSN3ATH.asm
 *
 * Called by DB2 to perform external authorization checking.
 * This exit supplements (or replaces) DB2's internal auth
 * and RACF checking for object access.
 *
 * This example:
 *   1) Logs all access attempts to tables with HLQ 'PAYROLL'
 *   2) Denies access to PAYROLL tables from batch connections
 *      unless the auth ID has SYSADM privilege
 *   3) Defers all other authorization to normal DB2 processing
 *
 * Exit Point: DSN3ATH - Authorization Exit Routine
 *
 * Return: DB2_ATH_ALLOW (0)    - allow access
 *         DB2_ATH_REJECT (4)   - deny access
 *         DB2_ATH_CONTINUE (8) - continue DB2 auth check
 *
 * Build: Use IBM Metal C compiler with no LE dependencies
 *   xlc -qmetal -S DSN3ATH.c
 *   as -o DSN3ATH.o DSN3ATH.s
 *   ld -o DSN3ATH DSN3ATH.o
 *
 * Attributes: REENTRANT, AMODE 31, RMODE ANY
 *
 * Register Mapping:
 *   R1  = parm (struct db2_ath_parm *)
 *   R12 = Base register
 *********************************************************************/

#include "metalc_base.h"
#include "metalc_db2.h"

/*===================================================================
 * DSN3ATH - DB2 Authorization Exit Entry Point
 *===================================================================*/

#pragma prolog(DSN3ATH, "SAVE(14,12),LR(12,15)")
#pragma epilog(DSN3ATH, "RETURN(14,12)")

int DSN3ATH(struct db2_ath_parm *parm) {

    /*---------------------------------------------------------------
     * Check function code - only handle table access (func=3)
     *---------------------------------------------------------------*/
    if (parm->func != ATH_FUNC_TABLE) {
        return DB2_ATH_CONTINUE;
    }

    /*---------------------------------------------------------------
     * Check if object name starts with 'PAYROLL'
     *---------------------------------------------------------------*/
    if (!match_prefix(parm->athobj, "PAYROLL", 7)) {
        return DB2_ATH_CONTINUE;
    }

    /*---------------------------------------------------------------
     * PAYROLL table access - log the attempt
     *---------------------------------------------------------------*/
    {
        char msg[80];
        int pos = 0;
        msg_append_str(msg, &pos, "DSN3ATH PAYROLL AUTH=");
        msg_append_field(msg, &pos, parm->athauth, 8);
        msg_append_str(msg, &pos, " OBJ=");
        msg_append_field(msg, &pos, parm->athobj, 18);

        wto_write(msg, pos, WTO_ROUTE_SYSTEM_SECURITY | WTO_ROUTE_PROGRAMMER_INFO, 0);
    }

    /*---------------------------------------------------------------
     * Check for SYSADM - SYSADM can always access
     *---------------------------------------------------------------*/
    if (match_field(parm->athauth, "SYSADM  ", 8)) {
        return DB2_ATH_ALLOW;
    }

    /*---------------------------------------------------------------
     * Non-SYSADM accessing PAYROLL - deny with reason code
     *---------------------------------------------------------------*/
    parm->athreasn = 200;
    return DB2_ATH_REJECT;
}
