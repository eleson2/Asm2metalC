/*********************************************************************
 * MODULE:    DFSMSCE0
 * FUNCTION:  TM and MSC Message Routing and Control Exit
 *
 * Converted from: asm/IMS/DFSMSCE0.asm
 *
 * This exit inspects MSC message routing and can override the
 * destination terminal. In this example, messages destined for
 * logical terminal 'TERM1' are rerouted to 'TERM2'.
 *
 * Exit Point: DFSMSCE0 - MSC Message Routing Exit
 *
 * Return: 0 - Continue (routing accepted or modified)
 *
 * Build: Use IBM Metal C compiler with no LE dependencies
 *   xlc -qmetal -S DFSMSCE0.c
 *   as -o DFSMSCE0.o DFSMSCE0.s
 *   ld -o DFSMSCE0 DFSMSCE0.o
 *
 * Attributes: REENTRANT, AMODE 31, RMODE ANY
 *
 * Register Mapping:
 *   R1  = Address of pointer to MSCP
 *   R2  = parm (struct mscp *)
 *   R4  = dest (struct mscd *)
 *   R12 = Base register
 *********************************************************************/

#include "metalc_base.h"
#include "metalc_ims.h"

/*===================================================================
 * DFSMSCE0 - MSC Message Routing and Control Exit
 *===================================================================*/

#pragma prolog(DFSMSCE0, "SAVE(14,12),LR(12,15)")
#pragma epilog(DFSMSCE0, "RETURN(14,12)")

int DFSMSCE0(void **parmlist) {
    struct mscp *parm;
    struct mscd *dest;

    /* Get MSCP parameter list (L R2,0(,R1)) */
    parm = (struct mscp *)parmlist[0];

    /* Route based on function code (L R3,MSCPFUNC) */
    switch (parm->mscpfunc) {

    case MSC_FUNC_INIT:
        /* Initialization - nothing to do */
        break;

    case MSC_FUNC_ROUTE:
        /* Message routing - inspect and possibly override */

        dest = parm->mscpdest;

        /* Only process logical terminal destinations (TM MSCDFLG1,MSCDLOG) */
        if (TM_NONE(dest->mscdflg1, MSCD_LOG)) {
            break;
        }

        /* If destination is 'TERM1', reroute to 'TERM2' */
        if (match_field(dest->mscdname, "TERM1   ", 8)) {
            /* Override destination (MVC MSCDNAME(8),=CL8'TERM2') */
            memcpy_inline(dest->mscdname, "TERM2   ", 8);
            
            /* Tell IMS we modified dest (OI MSCPFLG1,MSCPREQD) */
            OI(parm->mscpflg1, MSCP_REQD);
        }

        break;

    default:
        /* Unknown function - ignore */
        break;
    }

    return IMS_RC_CONTINUE;
}
