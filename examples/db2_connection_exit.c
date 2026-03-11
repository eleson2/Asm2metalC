/*********************************************************************
 * DB2 Connection Exit Example (DSNX@XAC)
 *
 * This exit monitors and controls DB2 connections, logging all
 * connection activity and enforcing connection policies based on
 * connection type, user, and plan.
 *
 * Exit Point: DSNX@XAC - Connection Exit
 *
 * Build: Use IBM Metal C compiler with no LE dependencies
 *   xlc -qmetal -S db2_connection_exit.c
 *   as -o db2_connection_exit.o db2_connection_exit.s
 *   ld -o DSNX@XAC db2_connection_exit.o
 *
 * Installation: Define in ZPARM DSNTIP6 panel
 *********************************************************************/

#include "metalc_base.h"
#include "metalc_db2.h"

/*-------------------------------------------------------------------
 * Configuration - Customize for your installation
 *-------------------------------------------------------------------*/

/* Plans requiring special authorization */
typedef struct {
    char plan_name[8];      /* Plan name (blank padded) */
    int  admin_only;        /* Requires admin authorization */
    int  drda_blocked;      /* Block from DRDA connections */
    int  log_all;           /* Log all access */
} controlled_plan_t;

static const controlled_plan_t controlled_plans[] = {
    { "DSNTEP2 ", 1, 0, 1 },  /* SPUFI - admin only */
    { "DSNREXX ", 1, 0, 1 },  /* REXX - admin only */
    { "ADMINTL ", 1, 1, 1 },  /* Admin tools - no DRDA */
    { "PAYPLAN ", 0, 1, 1 },  /* Payroll - no external */
    { "FINPLAN ", 0, 1, 1 },  /* Finance - no external */
    { "",         0, 0, 0 }   /* End marker */
};

/* Users with administrative privileges */
static const char admin_users[][8] = {
    "DBADMIN ",
    "SYSADMIN",
    "DBA001  ",
    "DBA002  ",
    ""           /* End marker */
};

/* Connection types blocked during maintenance windows */
static const uint8_t maintenance_blocked[] = {
    DB2_CONN_DDF,    /* Block DRDA during maintenance */
    DB2_CONN_RRSAF,  /* Block RRSAF during maintenance */
    0                /* End marker */
};

/* Custom reason codes for this exit */
#define DB2X_RSN_ADMIN_REQUIRED    1001
#define DB2X_RSN_DRDA_BLOCKED      1002
#define DB2X_RSN_MAINTENANCE       1003
#define DB2X_RSN_PLAN_BLOCKED      1004
#define DB2X_RSN_CONNECTION_LIMIT  1005

/*-------------------------------------------------------------------
 * find_controlled_plan - Find plan in controlled list
 * Returns pointer to controlled_plan_t or NULL
 *-------------------------------------------------------------------*/
static const controlled_plan_t *find_controlled_plan(const char plan[8]) {
    for (int i = 0; controlled_plans[i].plan_name[0] != '\0'; i++) {
        if (memcmp_inline(plan, controlled_plans[i].plan_name, 8) == 0) {
            return &controlled_plans[i];
        }
    }
    return NULL;
}

/*-------------------------------------------------------------------
 * is_admin_user - Check if user has admin privileges
 *-------------------------------------------------------------------*/
static int is_admin_user(const char user[8]) {
    for (int i = 0; admin_users[i][0] != '\0'; i++) {
        if (memcmp_inline(user, admin_users[i], 8) == 0) {
            return 1;
        }
    }
    return 0;
}

/*-------------------------------------------------------------------
 * is_maintenance_window - Check if currently in maintenance window
 * Note: In production, would check against schedule
 *-------------------------------------------------------------------*/
static int is_maintenance_window(void) {
    /*
     * In a real implementation, would:
     * 1. Check system time against maintenance schedule
     * 2. Or check a flag set by operations
     *
     * For this example, return 0 (not in maintenance)
     */
    return 0;
}

/*-------------------------------------------------------------------
 * is_blocked_during_maintenance - Check if connection type is blocked
 *-------------------------------------------------------------------*/
static int is_blocked_during_maintenance(uint8_t conn_type) {
    for (int i = 0; maintenance_blocked[i] != 0; i++) {
        if (conn_type == maintenance_blocked[i]) {
            return 1;
        }
    }
    return 0;
}

/*-------------------------------------------------------------------
 * log_connection - Write connection log record
 * Note: In production, would write to SMF or log dataset
 *-------------------------------------------------------------------*/
static void log_connection(const char *action,
                          const struct db2_xac_parm *parm,
                          int result) {
    /*
     * Production implementation would:
     * 1. Format log record with timestamp
     * 2. Write to SMF type 101/102 user record
     * 3. Or write to dedicated log dataset
     *
     * Fields to log:
     * - Action (CONNECT/SIGNON/DISCONNECT)
     * - Auth ID
     * - Plan name
     * - Connection type
     * - Job name
     * - Correlation ID
     * - Result code
     */
    (void)action;
    (void)parm;
    (void)result;
}

/*===================================================================
 * DSNX@XAC - DB2 Connection Exit
 *
 * Called during connection processing:
 * - XAC_FUNC_CONNECT:    New connection request
 * - XAC_FUNC_SIGNON:     Sign-on during connection
 * - XAC_FUNC_PLAN:       Plan allocation
 * - XAC_FUNC_AUTHCHG:    Authorization ID change
 * - XAC_FUNC_DISCONNECT: Disconnection
 *
 * Input:
 *   parm - Pointer to XAC parameter list containing:
 *          - xacfunc:  Function code
 *          - xactype:  Connection type (Batch/TSO/CICS/DDF/etc)
 *          - xacauth:  Primary authorization ID
 *          - xacplan:  Plan name
 *          - xacjob:   Job name
 *          - xaccorr:  Correlation ID
 *
 * Return:
 *   DB2_XAC_CONTINUE (0)  - Continue normal processing
 *   DB2_XAC_REJECT   (4)  - Reject the connection
 *   DB2_XAC_MODIFIED (8)  - Parameters modified by exit
 *   DB2_XAC_ERROR    (12) - Error (check reason code)
 *===================================================================*/

#pragma prolog(DSNX_XAC, "SAVE(14,12),LR(12,15)")
#pragma epilog(DSNX_XAC, "RETURN(14,12)")

int DSNX_XAC(struct db2_xac_parm *parm) {
    const controlled_plan_t *ctrl_plan;
    int is_admin;

    /* Validate input */
    if (parm == NULL) {
        return DB2_XAC_CONTINUE;
    }

    /* Handle different function codes */
    switch (parm->xacfunc) {

    case XAC_FUNC_CONNECT:
        /*
         * New connection request
         */

        /* Check for maintenance window */
        if (is_maintenance_window()) {
            if (is_blocked_during_maintenance(parm->xactype)) {
                parm->xacreasn = DB2X_RSN_MAINTENANCE;
                log_connection("REJECT-MAINT", parm, DB2_XAC_REJECT);
                return DB2_XAC_REJECT;
            }
        }

        /* Log DRDA connections (external network access) */
        if (db2_is_drda_conn(parm->xactype)) {
            log_connection("CONNECT-DDF", parm, DB2_XAC_CONTINUE);
        }

        break;

    case XAC_FUNC_PLAN:
        /*
         * Plan allocation - check authorization
         */

        ctrl_plan = find_controlled_plan(parm->xacplan);
        if (ctrl_plan != NULL) {
            is_admin = is_admin_user(parm->xacauth);

            /* Check if admin required */
            if (ctrl_plan->admin_only && !is_admin) {
                parm->xacreasn = DB2X_RSN_ADMIN_REQUIRED;
                log_connection("REJECT-AUTH", parm, DB2_XAC_REJECT);
                return DB2_XAC_REJECT;
            }

            /* Check if DRDA blocked for this plan */
            if (ctrl_plan->drda_blocked && db2_is_drda_conn(parm->xactype)) {
                parm->xacreasn = DB2X_RSN_DRDA_BLOCKED;
                log_connection("REJECT-DRDA", parm, DB2_XAC_REJECT);
                return DB2_XAC_REJECT;
            }

            /* Log if required */
            if (ctrl_plan->log_all) {
                log_connection("PLAN-ALLOC", parm, DB2_XAC_CONTINUE);
            }
        }

        break;

    case XAC_FUNC_AUTHCHG:
        /*
         * Authorization ID change (SET CURRENT SQLID)
         * Log all auth changes for audit
         */
        log_connection("AUTH-CHANGE", parm, DB2_XAC_CONTINUE);
        break;

    case XAC_FUNC_DISCONNECT:
        /*
         * Disconnection - optional cleanup/logging
         */
        /* Could log session duration, resource usage, etc. */
        break;

    default:
        /* Unknown function - continue normally */
        break;
    }

    return DB2_XAC_CONTINUE;
}

/*===================================================================
 * Additional Exit: Authorization (DSN3@ATH)
 *
 * Custom authorization checking for specific objects.
 *===================================================================*/

/* Tables requiring enhanced logging */
static const char audit_tables[][18] = {
    "PAYROLL.SALARY   ",
    "HR.EMPLOYEES     ",
    "FINANCE.ACCOUNTS ",
    ""                    /* End marker */
};

static int is_audit_table(const char obj[18]) {
    for (int i = 0; audit_tables[i][0] != '\0'; i++) {
        if (memcmp_inline(obj, audit_tables[i], 18) == 0) {
            return 1;
        }
    }
    return 0;
}

#pragma prolog(DSN3_ATH, "SAVE(14,12),LR(12,15)")
#pragma epilog(DSN3_ATH, "RETURN(14,12)")

int DSN3_ATH(struct db2_ath_parm *parm) {
    /* Only interested in table access */
    if (parm->athfunc != ATH_FUNC_TABLE) {
        return DB2_ATH_CONTINUE;
    }

    /* Check for audit tables */
    if (is_audit_table(parm->athobj)) {
        /*
         * Log access to sensitive tables
         * Format: AUTH userid PRIV privilege TABLE tablename
         */

        /* Check for UPDATE/DELETE on sensitive data */
        if ((parm->athpriv & (DB2_TPRIV_UPDATE | DB2_TPRIV_DELETE)) != 0) {
            /* Enhanced logging for modifications */
            /* log_table_modification(parm); */
        }
    }

    /* Defer to normal DB2 authorization */
    return DB2_ATH_CONTINUE;
}
