/*********************************************************************
 * MODULE:    CSQXLIB
 * FUNCTION:  MQ Channel Security Exit
 *
 * Converted from: asm/MQ/CSQXLIB.asm
 *
 * Called by MQ during channel startup. Authenticates the partner
 * queue manager against an allow list.
 *
 * Exit Point: MQ Channel Security Exit
 *
 * Return: RC=0 (Standard MQ exit linkage)
 *         exitResponse set in MQCXP to control channel status.
 *
 * Build: Use IBM Metal C compiler with no LE dependencies
 *
 * Attributes: REENTRANT, AMODE 31, RMODE ANY
 *
 * Register Mapping:
 *   R1  = Address of parameter list (4 pointers)
 *   R2  = p_cxp (struct mqcxp *)
 *   R3  = p_cd (struct mqcd *)
 *   R12 = Base register
 *********************************************************************/

#include "metalc_base.h"
#include "metalc_mq.h"

/*===================================================================
 * Allowed partner list
 *===================================================================*/

static const char ALLOWED_PARTNERS[][20] = {
    "PROD.QMGR1          ",
    "PROD.QMGR2          ",
    "DR.QMGR1            ",
    "TEST.QMGR1          "
};

#define NUM_PARTNERS (sizeof(ALLOWED_PARTNERS) / sizeof(ALLOWED_PARTNERS[0]))

/*===================================================================
 * CSQXLIB - MQ Channel Security Exit Entry Point
 *===================================================================*/

#pragma prolog(CSQXLIB, "SAVE(14,12),LR(12,15)")
#pragma epilog(CSQXLIB, "RETURN(14,12)")

int CSQXLIB(void **parmlist) {
    struct mqcxp *p_cxp = (struct mqcxp *)parmlist[0];
    struct mqcd  *p_cd  = (struct mqcd *)parmlist[1];

    /*---------------------------------------------------------------
     * Check exit reason - handle INIT_SEC (CLC 12(4,R2),=F'6')
     *---------------------------------------------------------------*/
    if (p_cxp->exitReason != MQXR_INIT_SEC) {
        p_cxp->exitResponse = MQXCC_OK;
        return 0;
    }

    /*---------------------------------------------------------------
     * Log the connection attempt
     *---------------------------------------------------------------*/
    {
        char msg[100];
        int pos = 0;
        msg_append_str(msg, &pos, "CSQXLIB MQ CHAN=");
        msg_append_field(msg, &pos, p_cd->channelName, 20);
        msg_append_str(msg, &pos, " PARTNER=");
        msg_append_field(msg, &pos, p_cxp->partnerName, 20);

        wto_write(msg, pos, WTO_ROUTE_PROGRAMMER_INFO, 0);
    }

    /*---------------------------------------------------------------
     * Check partner name against allow list
     *---------------------------------------------------------------*/
    int found = 0;
    for (int i = 0; i < (int)NUM_PARTNERS; i++) {
        if (match_field(p_cxp->partnerName, ALLOWED_PARTNERS[i], 20)) {
            found = 1;
            break;
        }
    }

    if (found) {
        p_cxp->exitResponse = MQXCC_OK;
    } else {
        /* Partner not in allow list - reject */
        char msg[80];
        int pos = 0;
        msg_append_str(msg, &pos, "CSQXLIB MQ REJECTED=");
        msg_append_field(msg, &pos, p_cxp->partnerName, 20);
        wto_write(msg, pos, WTO_ROUTE_MASTER_CONSOLE | WTO_ROUTE_SYSTEM_SECURITY, 
                  WTO_DESC_CRITICAL_ACTION);

        p_cxp->exitResponse = MQXCC_CLOSE_CHANNEL;
    }

    return 0;
}
