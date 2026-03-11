/*********************************************************************
 * MODULE:    DSIEX01
 * FUNCTION:  NetView Command Preprocessing Exit
 *
 * Converted from: asm/NETVIEW/DSIEX01.asm
 *
 * Called by NetView before a command is executed.
 * This example:
 *   - Blocks CANCEL and FORCE MVS commands for non-auth operators
 *   - Logs all VTAM commands for audit
 *   - Allows all other commands
 *
 * Exit Point: DSIEX01 - Command Preprocessing
 *
 * Return: NV_CMD_CONTINUE (0) - allow command
 *         NV_CMD_SUPPRESS (4) - suppress command
 *
 * Build: Use IBM Metal C compiler with no LE dependencies
 *
 * Attributes: REENTRANT, AMODE 31, RMODE ANY
 *
 * Register Mapping:
 *   R1  = parm (struct nv_cmd_parm *)
 *   R12 = Base register
 *
 * Parameter Mapping (Standard Header):
 *   parm->func     = nvfunc (1=pre, 2=post, etc)
 *   parm->flags    = nvcmdtype (1=NV, 2=MVS, 4=VTAM, etc)
 *   parm->reserved = nvflags (x'8000'=auth, x'4000'=console)
 *********************************************************************/

#include "metalc_base.h"
#include "metalc_netview.h"

/*===================================================================
 * DSIEX01 - NetView Command Preprocessing Exit Entry Point
 *===================================================================*/

#pragma prolog(DSIEX01, "SAVE(14,12),LR(12,15)")
#pragma epilog(DSIEX01, "RETURN(14,12)")

int DSIEX01(struct nv_cmd_parm *parm) {

    /*---------------------------------------------------------------
     * Check function code - only process pre-processing (func=1)
     *---------------------------------------------------------------*/
    if (parm->func != NV_FUNC_PRE) {
        return NV_CMD_CONTINUE;
    }

    /*---------------------------------------------------------------
     * Check command type (parm->flags maps to nvcmdtype)
     *---------------------------------------------------------------*/
    if (parm->flags == NV_CMDTYPE_MVS) {
        /* MVS command - check for dangerous commands */
        
        /* Check if operator is authorized (parm->reserved maps to nvflags) */
        if (TM_ALL(parm->reserved, NV_FLG_AUTH)) {
            return NV_CMD_CONTINUE;
        }

        /* Non-authorized operator - check for CANCEL and FORCE */
        char *cmd_text = parm->nvcmd;
        uint32_t cmd_len = parm->nvcmdlen;

        if (cmd_len >= 6 && match_prefix(cmd_text, "CANCEL", 6)) {
            goto REJECT;
        }

        if (cmd_len >= 5 && match_prefix(cmd_text, "FORCE", 5)) {
            goto REJECT;
        }

    } else if (parm->flags == NV_CMDTYPE_VTAM) {
        /* Log VTAM commands for audit */
        char msg[80];
        int pos = 0;
        msg_append_str(msg, &pos, "DSIEX01 VTAM CMD OPER=");
        msg_append_field(msg, &pos, parm->nvoper, 8);
        wto_write(msg, pos, WTO_ROUTE_PROGRAMMER_INFO, 0);
    }

    return NV_CMD_CONTINUE;

REJECT:
    /* Reject the command */
    {
        char msg[80];
        int pos = 0;
        msg_append_str(msg, &pos, "DSIEX01 CMD BLOCKED OPER=");
        msg_append_field(msg, &pos, parm->nvoper, 8);
        msg_append_str(msg, &pos, " NOT AUTHORIZED");
        wto_write(msg, pos, WTO_ROUTE_MASTER_CONSOLE | WTO_ROUTE_SYSTEM_SECURITY, 
                  WTO_DESC_IMMEDIATE_ACTION);
    }

    parm->nvreasn = 100;
    return NV_CMD_SUPPRESS;
}
