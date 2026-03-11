/*********************************************************************
 * CICS File Control Audit Exit Example (XFCREQ/XFCREQC)
 *
 * This exit audits file control operations, logging access to
 * sensitive files and enforcing access restrictions based on
 * transaction and user context.
 *
 * Exit Points:
 *   XFCREQ  - Before file control request
 *   XFCREQC - After file control request (completion)
 *
 * Build: Use IBM Metal C compiler with no LE dependencies
 *   xlc -qmetal -S cics_file_audit_exit.c
 *   as -o cics_file_audit_exit.o cics_file_audit_exit.s
 *   ld -o XFCAUDIT cics_file_audit_exit.o
 *
 * Installation: ENABLE PROGRAM(XFCAUDIT) EXIT(XFCREQ) START
 *********************************************************************/

#include "metalc_base.h"
#include "metalc_cics.h"

/*-------------------------------------------------------------------
 * Configuration - Customize for your installation
 *-------------------------------------------------------------------*/

/* Files requiring audit logging */
typedef struct {
    char file_name[8];      /* File name (blank padded) */
    int  log_read;          /* Log read operations */
    int  log_write;         /* Log write/rewrite/delete operations */
    int  restrict_write;    /* Restrict write to authorized users */
} monitored_file_t;

static const monitored_file_t monitored_files[] = {
    { "PAYROLL ", 1, 1, 1 },   /* Payroll data - full audit */
    { "CUSTOMER", 0, 1, 0 },   /* Customer data - write audit only */
    { "ACCOUNTS", 0, 1, 1 },   /* Account data - write audit + restrict */
    { "SECURITY", 1, 1, 1 },   /* Security data - full audit + restrict */
    { "CONFIG  ", 0, 1, 1 },   /* Config data - write only */
    { "",         0, 0, 0 }    /* End marker */
};

/* Transactions authorized for restricted file writes */
static const char authorized_transactions[][4] = {
    "PAY1",     /* Payroll update */
    "ACUP",     /* Account update */
    "ADMN",     /* Admin transaction */
    "SECU",     /* Security maintenance */
    ""          /* End marker */
};

/* Users authorized for restricted file writes */
static const char authorized_users[][8] = {
    "PAYROLL ",
    "ADMIN   ",
    "SECADMIN",
    ""          /* End marker */
};

/*-------------------------------------------------------------------
 * find_monitored_file - Find file in monitored list
 * Returns pointer to monitored_file_t or NULL
 *-------------------------------------------------------------------*/
static const monitored_file_t *find_monitored_file(const char file_name[8]) {
    for (int i = 0; monitored_files[i].file_name[0] != '\0'; i++) {
        if (memcmp_inline(file_name, monitored_files[i].file_name, 8) == 0) {
            return &monitored_files[i];
        }
    }
    return NULL;
}

/*-------------------------------------------------------------------
 * is_authorized_transaction - Check if transaction is authorized
 *-------------------------------------------------------------------*/
static int is_authorized_transaction(const char tran[4]) {
    for (int i = 0; authorized_transactions[i][0] != '\0'; i++) {
        if (memcmp_inline(tran, authorized_transactions[i], 4) == 0) {
            return 1;
        }
    }
    return 0;
}

/*-------------------------------------------------------------------
 * is_authorized_user - Check if user is authorized
 *-------------------------------------------------------------------*/
static int is_authorized_user(const char user[8]) {
    for (int i = 0; authorized_users[i][0] != '\0'; i++) {
        if (memcmp_inline(user, authorized_users[i], 8) == 0) {
            return 1;
        }
    }
    return 0;
}

/*-------------------------------------------------------------------
 * is_write_operation - Check if request is a write operation
 *-------------------------------------------------------------------*/
static int is_write_operation(uint8_t request_type) {
    return (request_type == FCP_REQ_WRITE ||
            request_type == FCP_REQ_REWRITE ||
            request_type == FCP_REQ_DELETE);
}

/*-------------------------------------------------------------------
 * get_operation_name - Get text name for operation
 *-------------------------------------------------------------------*/
static const char *get_operation_name(uint8_t request_type) {
    switch (request_type) {
        case FCP_REQ_READ:    return "READ   ";
        case FCP_REQ_WRITE:   return "WRITE  ";
        case FCP_REQ_REWRITE: return "REWRITE";
        case FCP_REQ_DELETE:  return "DELETE ";
        case FCP_REQ_BROWSE:  return "BROWSE ";
        default:              return "UNKNOWN";
    }
}

/*-------------------------------------------------------------------
 * write_audit_record - Write audit record to log
 * Note: In production, this would write to a TS queue, TD queue,
 *       or use CICS XPI to write an SMF record
 *-------------------------------------------------------------------*/
static void write_audit_record(const char *operation,
                               const char file[8],
                               const char user[8],
                               const char tran[4],
                               const char term[4],
                               int response) {
    /*
     * Production implementation would:
     * 1. Format audit record
     * 2. Write to TS queue for later batch processing
     * 3. Or write directly via CICS journal/TD facility
     *
     * Using CICS XPI:
     *   DFHTR TYPE=PUT,DEST=AUDT,DATA=record
     */
    (void)operation;
    (void)file;
    (void)user;
    (void)tran;
    (void)term;
    (void)response;
}

/*===================================================================
 * XFCREQ - File Control Pre-Request Exit
 *
 * Called before CICS processes a file control request. Can be used
 * to enforce restrictions or modify requests before execution.
 *
 * Input:
 *   parm - Pointer to file control parameter list containing:
 *          - fcpdsn:  Dataset/file name
 *          - fcpreq:  Request type (read/write/delete/etc)
 *          - base.uepuser: User ID
 *          - base.ueptran: Transaction ID
 *
 * Return:
 *   CICS_UERCNORM (0) - Continue with request
 *   CICS_UERCBYP  (4) - Bypass CICS processing (exit handles it)
 *   CICS_UERCPURG (8) - Purge the task
 *===================================================================*/

#pragma prolog(XFCREQ, "SAVE(14,12),LR(12,15)")
#pragma epilog(XFCREQ, "RETURN(14,12)")

int XFCREQ(struct dfhfcpar *parm) {
    const monitored_file_t *mon_file;

    /* Validate input */
    if (parm == NULL) {
        return CICS_UERCNORM;
    }

    /* Check if this file is monitored */
    mon_file = find_monitored_file(parm->fcpdsn);
    if (mon_file == NULL) {
        /* Not a monitored file - allow normally */
        return CICS_UERCNORM;
    }

    /* Check for write operation restrictions */
    if (is_write_operation(parm->fcpreq) && mon_file->restrict_write) {
        /* Check if transaction is authorized */
        int tran_auth = is_authorized_transaction(parm->base.ueptran);

        /* Check if user is authorized */
        int user_auth = is_authorized_user(parm->base.uepuser);

        if (!tran_auth && !user_auth) {
            /* Neither transaction nor user is authorized */
            /* Log the denial */
            write_audit_record("DENIED",
                             parm->fcpdsn,
                             parm->base.uepuser,
                             parm->base.ueptran,
                             parm->base.uepterm4,
                             -1);

            /*
             * Option 1: Return UERCPURG to abend the task
             * Option 2: Return UERCBYP and set response to NOTAUTH
             *
             * For security violations, purging is usually appropriate
             */
            return CICS_UERCPURG;
        }
    }

    /* Log read operations if required */
    if (parm->fcpreq == FCP_REQ_READ && mon_file->log_read) {
        write_audit_record(get_operation_name(parm->fcpreq),
                         parm->fcpdsn,
                         parm->base.uepuser,
                         parm->base.ueptran,
                         parm->base.uepterm4,
                         0);
    }

    /* Log write operations if required (pre-request) */
    if (is_write_operation(parm->fcpreq) && mon_file->log_write) {
        write_audit_record(get_operation_name(parm->fcpreq),
                         parm->fcpdsn,
                         parm->base.uepuser,
                         parm->base.ueptran,
                         parm->base.uepterm4,
                         0);
    }

    /* Continue with normal processing */
    return CICS_UERCNORM;
}

/*===================================================================
 * XFCREQC - File Control Completion Exit
 *
 * Called after CICS completes a file control request. Used for
 * logging the outcome of operations.
 *===================================================================*/

#pragma prolog(XFCREQC, "SAVE(14,12),LR(12,15)")
#pragma epilog(XFCREQC, "RETURN(14,12)")

int XFCREQC(struct dfhfcpar *parm) {
    const monitored_file_t *mon_file;

    /* Validate input */
    if (parm == NULL) {
        return CICS_UERCNORM;
    }

    /* Check if this file is monitored */
    mon_file = find_monitored_file(parm->fcpdsn);
    if (mon_file == NULL) {
        return CICS_UERCNORM;
    }

    /* Log completion for write operations with failure */
    if (is_write_operation(parm->fcpreq) &&
        mon_file->log_write &&
        parm->fcpresp != CICS_RESP_NORMAL) {

        /* Log the failure */
        write_audit_record("FAILED",
                         parm->fcpdsn,
                         parm->base.uepuser,
                         parm->base.ueptran,
                         parm->base.uepterm4,
                         parm->fcpresp);
    }

    /* Completion exit should always return CONTINUE */
    return CICS_UERCNORM;
}

/*===================================================================
 * Additional Exit: Program Error Handler (DFHPEP)
 *
 * Custom program error handling with logging.
 *===================================================================*/

#pragma prolog(DFHPEP, "SAVE(14,12),LR(12,15)")
#pragma epilog(DFHPEP, "RETURN(14,12)")

int DFHPEP(struct dfhpeppar *parm) {
    /*
     * Log all program abends for monitoring
     *
     * Format: ABEND <code> TRAN <tran> PROG <prog>
     */

    /* Check for runaway task (AICA) - these are informational */
    if (parm->peptype == PEP_TYPE_AICA) {
        /* Log runaway but allow retry if appropriate */
        return CICS_PEP_CONTINUE;
    }

    /* Check for program check (ASRA) */
    if (parm->peptype == PEP_TYPE_ASRA) {
        /* Log program check with PSW/registers */
        /* Could analyze PSW to determine error type */
        return CICS_PEP_CONTINUE;
    }

    /* For other abends, just continue with normal processing */
    return CICS_PEP_CONTINUE;
}
