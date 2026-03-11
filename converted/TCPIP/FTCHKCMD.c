/*********************************************************************
 * MODULE:    FTCHKCMD
 * FUNCTION:  FTP Command Validation Exit
 *
 * Converted from: asm/TCPIP/FTCHKCMD.asm
 *
 * Called by z/OS FTP server before executing a client command.
 * This example:
 *   - Blocks DELETE (DELE) commands for SYS1.* datasets
 *   - Blocks SITE commands from non-internal IP addresses
 *   - Logs all STOR (upload) commands
 *
 * Exit Point: FTCHKCMD - Command Validation Exit
 *
 * Return: FTP_RC_CONTINUE (0) - allow command
 *         FTP_RC_REJECT (4)   - reject command
 *
 * Build: Use IBM Metal C compiler with no LE dependencies
 *
 * Attributes: REENTRANT, AMODE 31, RMODE ANY
 *
 * Register Mapping:
 *   R1  = parm (struct ftp_chkcmd_parm *)
 *   R12 = Base register
 *
 * Parameter Mapping (Standard Header):
 *   parm->func     = ftpfunc (1=init, 2=command, 3=term)
 *   parm->flags    = ftpflags (x'80'=SSL, x'40'=anon, etc)
 *   parm->reserved = ftpcmd (Command code)
 *********************************************************************/

#include "metalc_base.h"
#include "metalc_tcpip.h"

/*===================================================================
 * FTCHKCMD - FTP Command Validation Exit Entry Point
 *===================================================================*/

#pragma prolog(FTCHKCMD, "SAVE(14,12),LR(12,15)")
#pragma epilog(FTCHKCMD, "RETURN(14,12)")

int FTCHKCMD(struct ftp_chkcmd_parm *parm) {

    /*---------------------------------------------------------------
     * Check function code - only process command calls (func=2)
     *---------------------------------------------------------------*/
    if (parm->func != FTP_FUNC_CMD) {
        return FTP_RC_CONTINUE;
    }

    /*---------------------------------------------------------------
     * Check for DELETE (DELE) command - code 23
     * parm->reserved maps to ftpcmd
     *---------------------------------------------------------------*/
    if (parm->reserved == 23) {
        /* DELETE command - check dataset name for SYS1. */
        if (match_prefix(parm->ftpdsn, "SYS1.", 5)) {
            /* Reject deletion of SYS1.* datasets */
            parm->ftpreply = FTP_REPLY_NOACCESS; /* 550 */
            parm->ftpreasn = 100;
            return FTP_RC_REJECT;
        }
    }

    /*---------------------------------------------------------------
     * Check for STOR (upload) command - code 15
     *---------------------------------------------------------------*/
    else if (parm->reserved == 15) {
        /* Log the upload */
        char msg[80];
        int pos = 0;
        msg_append_str(msg, &pos, "FTCHKCMD FTP STOR USER=");
        msg_append_field(msg, &pos, parm->ftpuser, 8);
        wto_write(msg, pos, WTO_ROUTE_PROGRAMMER_INFO, 0);
    }

    /*---------------------------------------------------------------
     * Check for SITE command - code 29
     *---------------------------------------------------------------*/
    else if (parm->reserved == 29) {
        /* SITE command - check if from internal network (10.x.x.x) */
        if (parm->ftpclient.addr[0] != 10) {
            /* External IP trying SITE command - reject */
            parm->ftpreply = FTP_REPLY_CMDERR; /* 500 */
            parm->ftpreasn = 200;
            return FTP_RC_REJECT;
        }
    }

    return FTP_RC_CONTINUE;
}
