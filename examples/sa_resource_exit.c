/*********************************************************************
 * System Automation Resource Exit Example (AOFEXC02)
 *
 * This exit monitors resource state changes and enforces automation
 * policies for resource management.
 *
 * Exit Point: AOFEXC02 - Resource State Change Exit
 *
 * Build: Use IBM Metal C compiler with no LE dependencies
 *   xlc -qmetal -S sa_resource_exit.c
 *   as -o sa_resource_exit.o sa_resource_exit.s
 *   ld -o AOFEXC02 sa_resource_exit.o
 *
 * Installation: Define in SA policy database
 *********************************************************************/

#include "metalc_base.h"
#include "metalc_sa.h"

/*-------------------------------------------------------------------
 * Configuration - Customize for your installation
 *-------------------------------------------------------------------*/

/* Critical resources requiring immediate notification */
typedef struct {
    char name_prefix[16];     /* Resource name prefix */
    int  prefix_len;          /* Prefix length */
    int  priority;            /* Alert priority (1=highest) */
} critical_resource_t;

static const critical_resource_t critical_resources[] = {
    { "CICS",      4, 1 },    /* CICS regions - critical */
    { "DB2",       3, 1 },    /* DB2 subsystems - critical */
    { "MQ",        2, 1 },    /* MQ queue managers - critical */
    { "PROD.",     5, 2 },    /* Production apps - high */
    { "PAYMENT",   7, 1 },    /* Payment systems - critical */
    { "BATCH",     5, 3 },    /* Batch systems - medium */
    { "",          0, 0 }     /* End marker */
};

/* State transitions requiring approval */
typedef struct {
    uint8_t from_state;       /* From state */
    uint8_t to_state;         /* To state */
    int     requires_approval; /* Requires operator approval */
} state_transition_t;

static const state_transition_t controlled_transitions[] = {
    { SA_STATE_AVAILABLE, SA_STATE_STOPPING, 1 },     /* Stop from available */
    { SA_STATE_AVAILABLE, SA_STATE_SOFTDOWN, 1 },     /* Take down */
    { SA_STATE_DEGRADED,  SA_STATE_STOPPING, 1 },     /* Stop degraded */
    { 0, 0, 0 }   /* End marker */
};

/* Authorized operators for controlled transitions */
static const char authorized_operators[][8] = {
    "MASTER  ",
    "SAADMIN ",
    "AUTOOP  ",
    ""           /* End marker */
};

/* Custom reason codes for this exit */
#define SAX_RSN_APPROVAL_REQUIRED  6001
#define SAX_RSN_CRITICAL_RESOURCE  6002
#define SAX_RSN_POLICY_VIOLATION   6003

/*-------------------------------------------------------------------
 * get_resource_priority - Get priority for resource
 *-------------------------------------------------------------------*/
static int get_resource_priority(const char name[32]) {
    for (int i = 0; critical_resources[i].prefix_len > 0; i++) {
        int plen = critical_resources[i].prefix_len;
        if (memcmp_inline(name, critical_resources[i].name_prefix, plen) == 0) {
            return critical_resources[i].priority;
        }
    }
    return 99;  /* Low priority for unmatched resources */
}

/*-------------------------------------------------------------------
 * is_controlled_transition - Check if transition requires approval
 *-------------------------------------------------------------------*/
static int is_controlled_transition(uint8_t from, uint8_t to) {
    for (int i = 0; controlled_transitions[i].from_state != 0 ||
                    controlled_transitions[i].to_state != 0; i++) {
        if (controlled_transitions[i].from_state == from &&
            controlled_transitions[i].to_state == to) {
            return controlled_transitions[i].requires_approval;
        }
    }
    return 0;
}

/*-------------------------------------------------------------------
 * is_authorized_operator - Check if operator is authorized
 * Note: In production, use RACF or SA's built-in authorization
 *-------------------------------------------------------------------*/
static int is_authorized_operator(const char oper[8]) {
    for (int i = 0; authorized_operators[i][0] != '\0'; i++) {
        if (memcmp_inline(oper, authorized_operators[i], 8) == 0) {
            return 1;
        }
    }
    return 0;
}

/*-------------------------------------------------------------------
 * copy_field - Copy fixed-length field, stop at blank/null
 * Returns number of characters copied
 *-------------------------------------------------------------------*/
static int copy_field(char *dest, const char *src, int maxlen) {
    int i;
    for (i = 0; i < maxlen; i++) {
        if (src[i] == ' ' || src[i] == '\0') break;
        dest[i] = src[i];
    }
    return i;
}

/*-------------------------------------------------------------------
 * send_alert - Send alert for resource state change via WTO
 *
 * Issues a WTO message to the operator console with routing based
 * on the priority level of the resource.
 *
 * Message format:
 *   SAX001E RESOURCE resource_name STATE CHANGE: old -> new
 *   SAX002W RESOURCE resource_name STATE CHANGE: old -> new
 *-------------------------------------------------------------------*/
static void send_alert(int priority,
                      const char *resource,
                      uint8_t old_state,
                      uint8_t new_state) {
    char msg[120];
    int pos = 0;
    uint16_t route;
    uint16_t desc;

    /* Build message ID based on priority */
    if (priority == 1) {
        /* Critical - Error level message */
        memcpy_inline(&msg[pos], "SAX001E ", 8);
        pos += 8;
        route = WTO_ROUTE_MASTER_CONSOLE | WTO_ROUTE_SYSTEM_SECURITY;
        desc = WTO_DESC_CRITICAL_ACTION;
    } else {
        /* High priority - Warning level */
        memcpy_inline(&msg[pos], "SAX002W ", 8);
        pos += 8;
        route = WTO_ROUTE_MASTER_CONSOLE;
        desc = WTO_DESC_EVENTUAL_ACTION;
    }

    /* Add "RESOURCE " */
    memcpy_inline(&msg[pos], "RESOURCE ", 9);
    pos += 9;

    /* Copy resource name (stop at blank) */
    pos += copy_field(&msg[pos], resource, 32);

    /* Add " STATE CHANGE: " */
    memcpy_inline(&msg[pos], " STATE CHANGE: ", 15);
    pos += 15;

    /* Copy old state name - use header's sa_state_name() */
    const char *ostate = sa_state_name(old_state);
    int slen = strlen_inline(ostate);
    memcpy_inline(&msg[pos], ostate, slen);
    pos += slen;

    /* Add " -> " */
    memcpy_inline(&msg[pos], " -> ", 4);
    pos += 4;

    /* Copy new state name */
    const char *nstate = sa_state_name(new_state);
    slen = strlen_inline(nstate);
    memcpy_inline(&msg[pos], nstate, slen);
    pos += slen;

    /* Issue the WTO */
    wto_write(msg, pos, route, desc);
}

/*-------------------------------------------------------------------
 * log_state_change - Log all state changes for audit trail
 *
 * Issues an informational WTO for audit purposes.
 *
 * Message format:
 *   SAX010I AUDIT: resource_name old_state->new_state
 *-------------------------------------------------------------------*/
static void log_state_change(struct sa_res_parm *parm) {
    char msg[120];
    int pos = 0;

    /* Message ID */
    memcpy_inline(&msg[pos], "SAX010I AUDIT: ", 15);
    pos += 15;

    /* Resource name - access via sares pointer */
    pos += copy_field(&msg[pos], parm->sares->resname, 32);
    msg[pos++] = ' ';

    /* Old state - use header's sa_state_name() */
    const char *ostate = sa_state_name(parm->saoldstate);
    int slen = strlen_inline(ostate);
    memcpy_inline(&msg[pos], ostate, slen);
    pos += slen;

    /* Arrow */
    memcpy_inline(&msg[pos], "->", 2);
    pos += 2;

    /* New state */
    const char *nstate = sa_state_name(parm->sanewstate);
    slen = strlen_inline(nstate);
    memcpy_inline(&msg[pos], nstate, slen);
    pos += slen;

    /* Issue informational WTO - route to programmer info */
    wto_write(msg, pos, WTO_ROUTE_PROGRAMMER_INFO, WTO_DESC_APPLICATION);
}

/*-------------------------------------------------------------------
 * log_rejection - Log rejected state change
 *
 * Issues a security WTO when a state change is rejected.
 *-------------------------------------------------------------------*/
static void log_rejection(struct sa_res_parm *parm, const char *reason) {
    char msg[120];
    int pos = 0;

    /* Message ID - security event */
    memcpy_inline(&msg[pos], "SAX005W REJECTED: ", 18);
    pos += 18;

    /* Resource name */
    pos += copy_field(&msg[pos], parm->sares->resname, 32);
    msg[pos++] = ' ';

    /* Reason */
    int rlen = strlen_inline(reason);
    memcpy_inline(&msg[pos], reason, rlen);
    pos += rlen;

    /* Issue security WTO */
    wto_security(msg, pos);
}

/*===================================================================
 * AOFEXC02 - System Automation Resource State Change Exit
 *
 * Called when a resource state change is detected.
 *
 * Input:
 *   parm - Pointer to sa_res_parm structure (from metalc_sa.h):
 *          - sares:      Pointer to resource descriptor
 *          - saoldstate: Previous observed state
 *          - sanewstate: New observed state
 *          - safunc:     Function code
 *
 * Return:
 *   SA_RES_CONTINUE  (0)  - Continue with state change
 *   SA_RES_SUPPRESS  (4)  - Suppress the action
 *   SA_RES_OVERRIDE  (8)  - Override automation
 *   SA_RES_DEFER     (12) - Defer action
 *===================================================================*/

#pragma prolog(AOFEXC02, "SAVE(14,12),LR(12,15)")
#pragma epilog(AOFEXC02, "RETURN(14,12)")

int AOFEXC02(struct sa_res_parm *parm) {
    int resource_priority;

    /* Validate input */
    if (parm == NULL || parm->sares == NULL) {
        return SA_RES_CONTINUE;
    }

    /* Handle different function codes */
    if (parm->safunc != SA_FUNC_STATCHG) {
        /* Init/Term/Monitor/Command - just continue */
        return SA_RES_CONTINUE;
    }

    /* Get resource priority based on name prefix */
    resource_priority = get_resource_priority(parm->sares->resname);

    /* Check if transition requires approval (manual operation) */
    if (parm->saflags & SA_FLG_MANUAL) {
        if (is_controlled_transition(parm->saoldstate, parm->sanewstate)) {
            /*
             * In production, check RACF authorization or SA policy.
             * This example uses a simple operator list.
             */
            /* Note: operator info would come from SA context */
            log_rejection(parm, "APPROVAL REQUIRED");
            parm->sareasn = SAX_RSN_APPROVAL_REQUIRED;
            return SA_RES_SUPPRESS;
        }
    }

    /* Alert on state changes for critical resources */
    if (resource_priority <= 2) {
        /* Critical or high priority resource */

        /* Alert on any problematic state */
        if (sa_is_down(parm->sanewstate) ||
            sa_is_problem(parm->sanewstate)) {

            send_alert(resource_priority,
                      parm->sares->resname,
                      parm->saoldstate,
                      parm->sanewstate);

            /* For critical resources going hard down, could escalate */
            if (resource_priority == 1 &&
                parm->sanewstate == SA_STATE_HARDDOWN) {
                /* Recovery exit (AOFEXC30) would handle escalation */
            }
        }
    }

    /* Log all state changes for audit */
    log_state_change(parm);

    return SA_RES_CONTINUE;
}
