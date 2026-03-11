/*********************************************************************
 * OPC/TWS Operation Status Alert Exit Example (EQQUX007)
 *
 * This exit monitors operation status changes and generates alerts
 * for critical path failures, SLA violations, and error conditions.
 *
 * Exit Point: EQQUX007 - Operation Status Change
 *
 * Build: Use IBM Metal C compiler with no LE dependencies
 *   xlc -qmetal -S opc_status_alert_exit.c
 *   as -o opc_status_alert_exit.o opc_status_alert_exit.s
 *   ld -o EQQUX007 opc_status_alert_exit.o
 *
 * Installation: Define in TWS parmlib EXITS statement
 *********************************************************************/

#include "metalc_base.h"
#include "metalc_opc.h"

/*-------------------------------------------------------------------
 * Configuration - Customize for your installation
 *-------------------------------------------------------------------*/

/* Severity levels for alerts */
#define ALERT_CRITICAL   1
#define ALERT_HIGH       2
#define ALERT_MEDIUM     3
#define ALERT_LOW        4

/* Maximum allowed duration variance before alerting (percentage) */
#define DURATION_VARIANCE_PCT  150

/* Applications requiring special monitoring */
typedef struct {
    char app_prefix[8];    /* Application name prefix */
    int  prefix_len;       /* Prefix length */
    int  alert_level;      /* Alert severity for errors */
} monitored_app_t;

static const monitored_app_t critical_apps[] = {
    { "PAYROLL", 7, ALERT_CRITICAL },  /* Payroll applications */
    { "FINANCE", 7, ALERT_CRITICAL },  /* Financial reporting */
    { "EOD",     3, ALERT_HIGH },      /* End of day processing */
    { "BACKUP",  6, ALERT_HIGH },      /* Backup jobs */
    { "BATCH",   5, ALERT_MEDIUM },    /* General batch */
    { "",        0, 0 }                /* End marker */
};

/* Workstations that are production critical */
static const char critical_workstations[][4] = {
    "PRD1",
    "PRD2",
    "CPU1",
    ""    /* End marker */
};

/*-------------------------------------------------------------------
 * get_app_alert_level - Get alert level for an application
 * Returns alert level or 0 if not monitored
 *-------------------------------------------------------------------*/
static int get_app_alert_level(const char app_name[16]) {
    for (int i = 0; critical_apps[i].prefix_len > 0; i++) {
        if (memcmp_inline(app_name, critical_apps[i].app_prefix,
                         critical_apps[i].prefix_len) == 0) {
            return critical_apps[i].alert_level;
        }
    }
    return ALERT_LOW;
}

/*-------------------------------------------------------------------
 * is_critical_workstation - Check if workstation is critical
 *-------------------------------------------------------------------*/
static int is_critical_workstation(const char ws_name[4]) {
    for (int i = 0; critical_workstations[i][0] != '\0'; i++) {
        if (memcmp_inline(ws_name, critical_workstations[i], 4) == 0) {
            return 1;
        }
    }
    return 0;
}

/*-------------------------------------------------------------------
 * calculate_duration_variance - Calculate percentage variance
 * Returns variance as percentage (100 = estimated, 150 = 50% over)
 *-------------------------------------------------------------------*/
static int calculate_duration_variance(uint32_t estimated, uint32_t actual) {
    if (estimated == 0) return 100;  /* No estimate, no variance */
    return (int)((actual * 100) / estimated);
}

/*-------------------------------------------------------------------
 * format_alert_message - Format an alert message
 *-------------------------------------------------------------------*/
static int format_alert_message(char *buffer, int max_len,
                                int severity,
                                const char *app_name,
                                uint16_t op_num,
                                const char *job_name,
                                const char *status_text) {
    int pos = 0;

    /* Severity prefix */
    const char *sev_text;
    switch (severity) {
        case ALERT_CRITICAL: sev_text = "CRITICAL"; break;
        case ALERT_HIGH:     sev_text = "HIGH    "; break;
        case ALERT_MEDIUM:   sev_text = "MEDIUM  "; break;
        default:             sev_text = "LOW     "; break;
    }

    /* Copy severity */
    for (int i = 0; i < 8 && sev_text[i] != '\0'; i++) {
        if (pos < max_len - 1) buffer[pos++] = sev_text[i];
    }

    if (pos < max_len - 1) buffer[pos++] = ' ';

    /* Copy application name (up to 16 chars) */
    for (int i = 0; i < 16 && app_name[i] != ' ' && app_name[i] != '\0'; i++) {
        if (pos < max_len - 1) buffer[pos++] = app_name[i];
    }

    if (pos < max_len - 1) buffer[pos++] = '/';

    /* Operation number */
    char num_buf[6];
    int num = op_num;
    int num_pos = 5;
    num_buf[5] = '\0';
    do {
        num_buf[--num_pos] = '0' + (num % 10);
        num /= 10;
    } while (num > 0 && num_pos > 0);

    for (int i = num_pos; i < 5; i++) {
        if (pos < max_len - 1) buffer[pos++] = num_buf[i];
    }

    if (pos < max_len - 1) buffer[pos++] = ' ';

    /* Job name */
    for (int i = 0; i < 8 && job_name[i] != ' ' && job_name[i] != '\0'; i++) {
        if (pos < max_len - 1) buffer[pos++] = job_name[i];
    }

    if (pos < max_len - 1) buffer[pos++] = ' ';

    /* Status text */
    for (int i = 0; status_text[i] != '\0'; i++) {
        if (pos < max_len - 1) buffer[pos++] = status_text[i];
    }

    buffer[pos] = '\0';
    return pos;
}

/*-------------------------------------------------------------------
 * send_alert - Send alert to operator and logging system
 * Note: In production, this would use WTO, write SMF records, or
 *       call external notification services
 *-------------------------------------------------------------------*/
static void send_alert(int severity, const char *message, int msg_len) {
    /*
     * Production implementation would:
     * 1. Issue WTO with appropriate routing codes
     * 2. Write to SMF user record
     * 3. Update alert log dataset
     * 4. Optionally trigger email/SMS via automation
     *
     * __asm__ volatile (
     *     "WTO   '%s',ROUTCDE=(1,2,11),DESC=(2)"
     *     : : "r"(message)
     * );
     */
    (void)severity;
    (void)message;
    (void)msg_len;
}

/*===================================================================
 * EQQUX007 - Operation Status Change Exit
 *
 * Called when an operation's status changes. Used to:
 * - Alert on critical path failures
 * - Detect SLA violations (late starts, long durations)
 * - Log status changes for audit
 * - Trigger notifications for important operations
 *
 * Input:
 *   parm - Pointer to UX007 parameter list containing:
 *          - ux007oper:  Operation information
 *          - ux007oldst: Previous status
 *          - ux007newst: New status
 *          - ux007rc:    Return code (if applicable)
 *          - ux007abnd:  Abend code (if applicable)
 *
 * Return:
 *   OPC_UX007_CONTINUE (0) - Continue with status change
 *   OPC_UX007_MODIFY   (4) - Status was modified by exit
 *   OPC_UX007_REJECT   (8) - Reject status change (use carefully!)
 *===================================================================*/

#pragma prolog(EQQUX007, "SAVE(14,12),LR(12,15)")
#pragma epilog(EQQUX007, "RETURN(14,12)")

int EQQUX007(struct opc_ux007_parm *parm) {
    struct opc_oper *oper;
    int alert_level;
    char msg_buffer[120];
    int msg_len;

    /* Validate input */
    if (parm == NULL || parm->ux007oper == NULL) {
        return OPC_UX007_CONTINUE;
    }

    oper = parm->ux007oper;

    /* Determine base alert level for this application */
    alert_level = get_app_alert_level(oper->op_adname);

    /* Elevate if critical path operation */
    if (opc_is_critical_path(oper) && alert_level > ALERT_HIGH) {
        alert_level = ALERT_HIGH;
    }

    /* Elevate if critical workstation */
    if (is_critical_workstation(oper->op_wsname) &&
        alert_level > ALERT_HIGH) {
        alert_level = ALERT_HIGH;
    }

    /* Check for error status */
    if (parm->ux007newst == OPC_STATUS_ERROR) {
        const char *error_text = "ENDED IN ERROR";

        /* Check for abend vs condition code */
        if (parm->ux007abnd != 0) {
            error_text = "ABENDED";
            /* Abends always elevate alert level */
            if (alert_level > ALERT_MEDIUM) {
                alert_level = ALERT_MEDIUM;
            }
        }

        msg_len = format_alert_message(msg_buffer, sizeof(msg_buffer),
                                       alert_level,
                                       oper->op_adname,
                                       oper->op_opnum,
                                       oper->op_jobname,
                                       error_text);

        send_alert(alert_level, msg_buffer, msg_len);
    }

    /* Check for interrupted status */
    if (parm->ux007newst == OPC_STATUS_INTERRUPT) {
        msg_len = format_alert_message(msg_buffer, sizeof(msg_buffer),
                                       alert_level,
                                       oper->op_adname,
                                       oper->op_opnum,
                                       oper->op_jobname,
                                       "INTERRUPTED");

        send_alert(alert_level, msg_buffer, msg_len);
    }

    /* Check for completion with high return code */
    if (parm->ux007newst == OPC_STATUS_COMPLETE &&
        oper->op_maxrc > 4) {
        /* High return code on completion */
        msg_len = format_alert_message(msg_buffer, sizeof(msg_buffer),
                                       ALERT_MEDIUM,
                                       oper->op_adname,
                                       oper->op_opnum,
                                       oper->op_jobname,
                                       "HIGH RC");

        send_alert(ALERT_MEDIUM, msg_buffer, msg_len);
    }

    /* Check for duration variance on completion */
    if (parm->ux007newst == OPC_STATUS_COMPLETE &&
        oper->op_estdur > 0 &&
        oper->op_actdur > 0) {

        int variance = calculate_duration_variance(oper->op_estdur,
                                                   oper->op_actdur);

        if (variance > DURATION_VARIANCE_PCT) {
            msg_len = format_alert_message(msg_buffer, sizeof(msg_buffer),
                                           ALERT_LOW,
                                           oper->op_adname,
                                           oper->op_opnum,
                                           oper->op_jobname,
                                           "DURATION EXCEEDED");

            send_alert(ALERT_LOW, msg_buffer, msg_len);
        }
    }

    /* Check for late start (only if estimated times are available) */
    if (parm->ux007newst == OPC_STATUS_STARTING &&
        oper->op_planstart > 0 &&
        oper->op_actstart > oper->op_planstart) {

        /* Started late */
        uint32_t late_seconds = oper->op_actstart - oper->op_planstart;

        /* Only alert if more than 5 minutes late */
        if (late_seconds > 300) {
            msg_len = format_alert_message(msg_buffer, sizeof(msg_buffer),
                                           alert_level,
                                           oper->op_adname,
                                           oper->op_opnum,
                                           oper->op_jobname,
                                           "LATE START");

            send_alert(alert_level, msg_buffer, msg_len);
        }
    }

    /* Always continue with the status change */
    return OPC_UX007_CONTINUE;
}
