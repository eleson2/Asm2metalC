/*********************************************************************
 * IMS Sign-on Security Exit Example (DFSSGUX0)
 *
 * This exit adds custom sign-on validation in addition to standard
 * RACF/ACF2 authentication, enforcing terminal restrictions, time
 * restrictions, and custom business rules.
 *
 * Exit Point: DFSSGUX0 - Sign-on/Security Exit
 *
 * Build: Use IBM Metal C compiler with no LE dependencies
 *   xlc -qmetal -S ims_signon_exit.c
 *   as -o ims_signon_exit.o ims_signon_exit.s
 *   ld -o DFSSGUX0 ims_signon_exit.o
 *
 * Installation: Link into IMS.SDFSRESL
 *********************************************************************/

#include "metalc_base.h"
#include "metalc_ims.h"

/*-------------------------------------------------------------------
 * Configuration - Customize for your installation
 *-------------------------------------------------------------------*/

/* Users requiring special validation */
typedef struct {
    char user_id[8];           /* User ID (blank padded) */
    uint8_t allowed_terminals; /* Bitmap: 1=specific, 2=any, 4=console */
    uint8_t time_restricted;   /* 1=business hours only */
    char allowed_prefix[4];    /* Terminal prefix (if specific) */
} user_restriction_t;

static const user_restriction_t restricted_users[] = {
    { "TELLER01", 0x01, 1, "TLR" },   /* Tellers: TLRxxx terms, business hours */
    { "TELLER02", 0x01, 1, "TLR" },
    { "TELLER03", 0x01, 1, "TLR" },
    { "BATCH   ", 0x04, 0, ""    },   /* Batch: console only */
    { "OPER    ", 0x06, 0, ""    },   /* Operators: any or console */
    { "",         0x00, 0, ""    }    /* End marker */
};

/* Business hours (0800-1800 Monday-Friday) */
#define BUSINESS_START_HOUR  8
#define BUSINESS_END_HOUR    18

/* Terminals requiring supervisor sign-on */
static const char supervisor_terminals[][8] = {
    "SUPTERM1",
    "SUPTERM2",
    "ADMIN001",
    ""            /* End marker */
};

/* Supervisor users */
static const char supervisor_users[][8] = {
    "SUPERVIS",
    "MANAGER ",
    "ADMIN   ",
    ""            /* End marker */
};

/* Custom reason codes for this exit */
#define IMSX_RSN_TERMINAL_RESTRICT   100    /* Terminal restriction */
#define IMSX_RSN_TIME_RESTRICT       101    /* Time restriction */
#define IMSX_RSN_SUPERVISOR_REQ      102    /* Supervisor required */
#define IMSX_RSN_ACCOUNT_LOCKED      103    /* Account locked */

/*-------------------------------------------------------------------
 * find_user_restriction - Find user in restriction table
 * Returns pointer to user_restriction_t or NULL
 *-------------------------------------------------------------------*/
static const user_restriction_t *find_user_restriction(const char user[8]) {
    for (int i = 0; restricted_users[i].user_id[0] != '\0'; i++) {
        if (memcmp_inline(user, restricted_users[i].user_id, 8) == 0) {
            return &restricted_users[i];
        }
    }
    return NULL;
}

/*-------------------------------------------------------------------
 * is_terminal_allowed - Check if user can sign on from terminal
 *-------------------------------------------------------------------*/
static int is_terminal_allowed(const user_restriction_t *rest,
                               const char lterm[8]) {
    /* Check terminal type flags */
    if (rest->allowed_terminals & 0x02) {
        /* Any terminal allowed */
        return 1;
    }

    if (rest->allowed_terminals & 0x04) {
        /* Console allowed - check for console terminal */
        if (lterm[0] == '*' || memcmp_inline(lterm, "CONS", 4) == 0) {
            return 1;
        }
    }

    if (rest->allowed_terminals & 0x01) {
        /* Specific prefix required */
        int plen = 0;
        while (plen < 4 && rest->allowed_prefix[plen] != '\0' &&
               rest->allowed_prefix[plen] != ' ') {
            plen++;
        }
        if (plen > 0 && memcmp_inline(lterm, rest->allowed_prefix, plen) == 0) {
            return 1;
        }
    }

    return 0;
}

/*-------------------------------------------------------------------
 * is_business_hours - Check if current time is within business hours
 * Note: In production, would need to check day of week too
 *-------------------------------------------------------------------*/
static int is_business_hours(uint32_t packed_time) {
    /*
     * IMS packed time format: 0HHMMSSTF
     * Extract hour from bits 4-11
     */
    int hour = ((packed_time >> 24) & 0x0F) * 10 +
               ((packed_time >> 20) & 0x0F);

    return (hour >= BUSINESS_START_HOUR && hour < BUSINESS_END_HOUR);
}

/*-------------------------------------------------------------------
 * is_supervisor_terminal - Check if terminal requires supervisor
 *-------------------------------------------------------------------*/
static int is_supervisor_terminal(const char lterm[8]) {
    for (int i = 0; supervisor_terminals[i][0] != '\0'; i++) {
        if (memcmp_inline(lterm, supervisor_terminals[i], 8) == 0) {
            return 1;
        }
    }
    return 0;
}

/*-------------------------------------------------------------------
 * is_supervisor_user - Check if user is a supervisor
 *-------------------------------------------------------------------*/
static int is_supervisor_user(const char user[8]) {
    for (int i = 0; supervisor_users[i][0] != '\0'; i++) {
        if (memcmp_inline(user, supervisor_users[i], 8) == 0) {
            return 1;
        }
    }
    return 0;
}

/*-------------------------------------------------------------------
 * get_current_time - Get current time in IMS packed format
 * Note: In production, would use STCK and convert
 *-------------------------------------------------------------------*/
static uint32_t get_current_time(void) {
    /*
     * In a real exit, you would:
     * 1. Use STCK to get TOD clock
     * 2. Convert to local time
     * 3. Pack into IMS format
     *
     * For this example, return a placeholder
     */
    return 0x09000000;  /* 09:00:00 */
}

/*===================================================================
 * DFSSGUX0 - IMS Sign-on Security Exit
 *
 * Called during user sign-on to perform additional validation.
 *
 * Input:
 *   parm - Pointer to sign-on exit parameter list containing:
 *          - sgxfunc:  Function code (signon/signoff/verify)
 *          - sgxuser:  User ID
 *          - sgxpass:  Password (usually masked)
 *          - sgxlterm: Logical terminal name
 *          - sgxflags: Processing flags
 *
 * Return:
 *   IMS_SGNX_ALLOW  (0)  - Allow sign-on
 *   IMS_SGNX_REJECT (4)  - Reject sign-on
 *   IMS_SGNX_DEFER  (8)  - Defer to RACF/ACF2
 *   IMS_SGNX_REVOKE (12) - Revoke user (severe violation)
 *===================================================================*/

#pragma prolog(DFSSGUX0, "SAVE(14,12),LR(12,15)")
#pragma epilog(DFSSGUX0, "RETURN(14,12)")

int DFSSGUX0(struct ims_sgnx_parm *parm) {
    const user_restriction_t *restriction;
    uint32_t current_time;

    /* Only process sign-on requests */
    if (parm->sgxfunc != SGX_FUNC_SIGNON) {
        /* Sign-off and verification - defer to RACF */
        return IMS_SGNX_DEFER;
    }

    /* Check if terminal requires supervisor */
    if (is_supervisor_terminal(parm->sgxlterm)) {
        if (!is_supervisor_user(parm->sgxuser)) {
            parm->sgxreasn = IMSX_RSN_SUPERVISOR_REQ;
            return IMS_SGNX_REJECT;
        }
    }

    /* Check user-specific restrictions */
    restriction = find_user_restriction(parm->sgxuser);

    if (restriction != NULL) {
        /* Check terminal restriction */
        if (!is_terminal_allowed(restriction, parm->sgxlterm)) {
            parm->sgxreasn = IMSX_RSN_TERMINAL_RESTRICT;

            /*
             * Could set a message for the user:
             * if (parm->sgxmsg != NULL && parm->sgxmsgln >= 40) {
             *     memcpy(parm->sgxmsg, "NOT AUTHORIZED FOR THIS TERMINAL", 32);
             * }
             */

            return IMS_SGNX_REJECT;
        }

        /* Check time restriction */
        if (restriction->time_restricted) {
            current_time = get_current_time();

            if (!is_business_hours(current_time)) {
                parm->sgxreasn = IMSX_RSN_TIME_RESTRICT;
                return IMS_SGNX_REJECT;
            }
        }
    }

    /*
     * All custom checks passed.
     * Defer to RACF/ACF2 for password verification.
     *
     * Note: Returning ALLOW would skip RACF entirely,
     *       which is usually not desired.
     */
    return IMS_SGNX_DEFER;
}

/*===================================================================
 * Additional Exit: Transaction Routing (DFSTXIT0)
 *
 * Route or reject transactions based on business rules.
 *===================================================================*/

/* Transactions requiring special authorization */
static const char high_value_trans[][8] = {
    "XFER1000",   /* Transfer over $1000 */
    "WIRELRG ",   /* Large wire transfer */
    "OVERRIDE",   /* Override transaction */
    ""            /* End marker */
};

#pragma prolog(DFSTXIT0, "SAVE(14,12),LR(12,15)")
#pragma epilog(DFSTXIT0, "RETURN(14,12)")

int DFSTXIT0(struct ims_txit_parm *parm) {
    /* Only check transaction scheduling */
    if (parm->txitfunc != TXIT_FUNC_SCHED) {
        return IMS_TXIT_CONTINUE;
    }

    /* Check for high-value transactions */
    for (int i = 0; high_value_trans[i][0] != '\0'; i++) {
        if (memcmp_inline(parm->txittran, high_value_trans[i], 8) == 0) {
            /* Require supervisor for high-value transactions */
            if (!is_supervisor_user(parm->txituser)) {
                parm->txitreasn = 200;  /* Custom reason code */
                return IMS_TXIT_REJECT;
            }
            break;
        }
    }

    /* All checks passed */
    return IMS_TXIT_CONTINUE;
}
