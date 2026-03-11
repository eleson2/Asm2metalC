/*********************************************************************
 * IBM MQ Channel Security Exit Example
 *
 * This exit validates channel connections, enforcing security policies
 * and logging channel activity.
 *
 * Exit Point: MQXR_SEC - Channel Security Exit
 *
 * Build: Use IBM Metal C compiler with no LE dependencies
 *   xlc -qmetal -S mq_channel_exit.c
 *   as -o mq_channel_exit.o mq_channel_exit.s
 *   ld -o MQCHLEXI mq_channel_exit.o
 *
 * Installation: Define in channel definition SCYEXIT
 *********************************************************************/

#include "metalc_base.h"
#include "metalc_mq.h"

/*-------------------------------------------------------------------
 * Configuration - Customize for your installation
 *-------------------------------------------------------------------*/

/* Allowed partner queue managers */
static const char allowed_qmgrs[][48] = {
    "QM.PROD.PARTNER1                                ",
    "QM.PROD.PARTNER2                                ",
    "QM.TEST.LOCAL                                   ",
    ""  /* End marker (first byte null) */
};

/* Blocked partner addresses (IP prefix) */
typedef struct {
    char prefix[16];    /* IP address prefix */
    int  prefix_len;    /* Prefix length */
} blocked_addr_t;

static const blocked_addr_t blocked_addrs[] = {
    { "192.168.99.", 11 },   /* Test network */
    { "10.99.",       6 },   /* Lab network */
    { "",             0 }    /* End marker */
};

/* Channel names requiring extra validation */
static const char high_security_channels[][20] = {
    "TO.PROD.PAYMENT",
    "TO.PROD.SECURITY",
    "FROM.EXTERNAL.",
    ""  /* End marker */
};

/* Custom reason codes for this exit (use MQRC_APPL_FIRST range) */
#define MQX_RSN_QMGR_NOT_ALLOWED   (MQRC_APPL_FIRST + 1)
#define MQX_RSN_ADDR_BLOCKED       (MQRC_APPL_FIRST + 2)
#define MQX_RSN_SECURITY_CHECK     (MQRC_APPL_FIRST + 3)

/*-------------------------------------------------------------------
 * is_qmgr_allowed - Check if partner queue manager is allowed
 *-------------------------------------------------------------------*/
static int is_qmgr_allowed(const char qmgr[48]) {
    /* Empty allowed list means all allowed */
    if (allowed_qmgrs[0][0] == '\0') {
        return 1;
    }

    for (int i = 0; allowed_qmgrs[i][0] != '\0'; i++) {
        if (memcmp_inline(qmgr, allowed_qmgrs[i], 48) == 0) {
            return 1;
        }
    }
    return 0;
}

/*-------------------------------------------------------------------
 * is_addr_blocked - Check if partner address is blocked
 *-------------------------------------------------------------------*/
static int is_addr_blocked(const char *addr) {
    if (addr == NULL) return 0;

    for (int i = 0; blocked_addrs[i].prefix_len > 0; i++) {
        if (memcmp_inline(addr, blocked_addrs[i].prefix,
                         blocked_addrs[i].prefix_len) == 0) {
            return 1;
        }
    }
    return 0;
}

/*-------------------------------------------------------------------
 * is_high_security_channel - Check if channel requires extra security
 *-------------------------------------------------------------------*/
static int is_high_security_channel(const char *channel_name) {
    for (int i = 0; high_security_channels[i][0] != '\0'; i++) {
        int plen = 0;
        while (high_security_channels[i][plen] != '\0' &&
               high_security_channels[i][plen] != ' ') {
            plen++;
        }

        /* Check for prefix match or exact match */
        if (memcmp_inline(channel_name, high_security_channels[i], plen) == 0) {
            return 1;
        }
    }
    return 0;
}

/*-------------------------------------------------------------------
 * log_channel_event - Log channel security event via WTO
 *-------------------------------------------------------------------*/
static void log_channel_event(const char *event,
                             const char *channel,
                             const char *partner_qmgr,
                             int result) {
    char msg[120];
    int pos = 0;
    uint16_t route;
    uint16_t desc;

    /* Build message - security events get routed to security console */
    if (result != MQXCC_OK) {
        memcpy_inline(&msg[pos], "MQX002W ", 8);
        route = WTO_ROUTE_SYSTEM_SECURITY | WTO_ROUTE_MASTER_CONSOLE;
        desc = WTO_DESC_SYSTEM_STATUS;
    } else {
        memcpy_inline(&msg[pos], "MQX001I ", 8);
        route = WTO_ROUTE_PROGRAMMER_INFO;
        desc = WTO_DESC_APPLICATION;
    }
    pos += 8;

    /* Add event type */
    int elen = strlen_inline(event);
    memcpy_inline(&msg[pos], event, elen);
    pos += elen;
    msg[pos++] = ' ';

    /* Add channel name (up to 20 chars, stop at blank) */
    for (int i = 0; i < 20 && channel[i] != ' ' && channel[i] != '\0'; i++) {
        msg[pos++] = channel[i];
    }
    msg[pos++] = ' ';

    /* Add partner (truncate to fit) */
    memcpy_inline(&msg[pos], "PARTNER=", 8);
    pos += 8;
    for (int i = 0; i < 20 && pos < 110 && partner_qmgr[i] != ' '; i++) {
        msg[pos++] = partner_qmgr[i];
    }

    wto_write(msg, pos, route, desc);
}

/*===================================================================
 * MQCHLEXI - MQ Channel Security Exit
 *
 * Called during channel connection to validate security.
 *
 * Input:
 *   cxp - Pointer to channel exit parameter (mqcxp from metalc_mq.h)
 *   cd  - Pointer to channel definition (mqcd from metalc_mq.h)
 *
 * Note: The MQ channel exit interface passes:
 *       - MQCXP (mqcxp) contains exit control information
 *       - MQCD (mqcd) contains channel definition details
 *
 * Return (via cxp->exitResponse):
 *   MQXCC_OK            (0) - Continue with channel
 *   MQXCC_CLOSE_CHANNEL (4) - Close the channel
 *===================================================================*/

#pragma prolog(MQCHLEXI, "SAVE(14,12),LR(12,15)")
#pragma epilog(MQCHLEXI, "RETURN(14,12)")

int MQCHLEXI(struct mqcxp *cxp, struct mqcd *cd) {
    /* Validate input */
    if (cxp == NULL || cd == NULL) {
        return MQXCC_FAILED;
    }

    /* Handle different exit reasons */
    switch (cxp->exitReason) {

    case MQXR_INIT:
    case MQXR_INIT_SEC:
        /*
         * Channel initialization - validate connection
         */

        /* Check if partner address is blocked */
        if (is_addr_blocked(cd->connectionName)) {
            cxp->exitResponse = MQXCC_CLOSE_CHANNEL;
            cxp->feedback = MQX_RSN_ADDR_BLOCKED;
            log_channel_event("BLOCKED-ADDR", cd->channelName,
                            cxp->partnerName, MQXCC_CLOSE_CHANNEL);
            return MQXCC_CLOSE_CHANNEL;
        }

        /* Check if partner queue manager is allowed */
        if (!is_qmgr_allowed(cxp->partnerName)) {
            cxp->exitResponse = MQXCC_CLOSE_CHANNEL;
            cxp->feedback = MQX_RSN_QMGR_NOT_ALLOWED;
            log_channel_event("BLOCKED-QMGR", cd->channelName,
                            cxp->partnerName, MQXCC_CLOSE_CHANNEL);
            return MQXCC_CLOSE_CHANNEL;
        }

        /* Extra validation for high-security channels */
        if (is_high_security_channel(cd->channelName)) {
            /* Could perform additional checks:
             * - Verify SSL/TLS certificate
             * - Check time-of-day restrictions
             * - Require specific authentication
             */
            log_channel_event("HIGH-SEC-CONNECT", cd->channelName,
                            cxp->partnerName, MQXCC_OK);
        }

        /* Log successful connection */
        log_channel_event("CONNECT", cd->channelName,
                        cxp->partnerName, MQXCC_OK);
        break;

    case MQXR_TERM:
        /*
         * Channel termination - cleanup
         */
        log_channel_event("DISCONNECT", cd->channelName,
                        cxp->partnerName, MQXCC_OK);
        break;

    case MQXR_SEC_PARMS:
        /*
         * Security parameters exchange
         */
        /* Could validate security tokens, certificates, etc. */
        break;

    default:
        /* Other reasons - continue normally */
        break;
    }

    cxp->exitResponse = MQXCC_OK;
    return MQXCC_OK;
}
