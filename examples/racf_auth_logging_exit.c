/*********************************************************************
 * RACF Authorization Logging Exit Example (ICHRCX02)
 *
 * This exit runs after RACF authorization checks to perform custom
 * logging and auditing of access decisions, particularly for
 * sensitive resources.
 *
 * Exit Point: ICHRCX02 - RACROUTE AUTH Post-processing
 *
 * Build: Use IBM Metal C compiler with no LE dependencies
 *   xlc -qmetal -S racf_auth_logging_exit.c
 *   as -o racf_auth_logging_exit.o racf_auth_logging_exit.s
 *   ld -o ICHRCX02 racf_auth_logging_exit.o
 *
 * Installation: Place in LPALIB and activate via SETROPTS
 *********************************************************************/

#include "metalc_base.h"
#include "metalc_racf.h"

/*-------------------------------------------------------------------
 * Configuration - Customize for your installation
 *-------------------------------------------------------------------*/

/* Resource classes that require enhanced logging */
typedef struct {
    char class_name[8];     /* Class name (blank padded) */
    int  log_success;       /* Log successful access */
    int  log_failure;       /* Log failed access */
} monitored_class_t;

static const monitored_class_t monitored_classes[] = {
    { "FACILITY", 1, 1 },    /* Log all facility checks */
    { "OPERCMDS", 1, 1 },    /* Log operator commands */
    { "SURROGAT", 1, 1 },    /* Log surrogate access */
    { "STARTED ", 0, 1 },    /* Log only failures for started tasks */
    { "CICS    ", 0, 1 },    /* Log only CICS failures */
    { "",         0, 0 }     /* End marker */
};

/* Resource name patterns that require enhanced logging */
typedef struct {
    char pattern[20];        /* Resource name prefix */
    int  pattern_len;        /* Length of prefix */
    int  always_log;         /* Log regardless of success/failure */
} monitored_resource_t;

static const monitored_resource_t monitored_resources[] = {
    { "BPX.SUPERUSER",     12, 1 },   /* Unix superuser */
    { "BPX.DAEMON",         9, 1 },   /* Unix daemon */
    { "BPX.SERVER",         9, 1 },   /* Unix server */
    { "IRR.RADMIN",         9, 1 },   /* RACF admin functions */
    { "IRR.DIGTCERT",      11, 1 },   /* Digital certificates */
    { "IEAABD.",            6, 1 },   /* ABEND controls */
    { "MVS.VARY.OFFLINE",  15, 1 },   /* Vary devices offline */
    { "",                   0, 0 }    /* End marker */
};

/* Users exempt from enhanced logging (system IDs) */
static const char exempt_users[][8] = {
    "RACF    ",
    "IBMUSER ",
    "OMVSSVR ",
    "FTPD    ",
    "SSHD    ",
    ""           /* End marker (first byte null) */
};

/*-------------------------------------------------------------------
 * is_monitored_class - Check if class requires enhanced logging
 * Returns pointer to monitored_class_t or NULL
 *-------------------------------------------------------------------*/
static const monitored_class_t *is_monitored_class(const char *class_name,
                                                    uint8_t class_len) {
    for (int i = 0; monitored_classes[i].class_name[0] != '\0'; i++) {
        if (memcmp_inline(class_name, monitored_classes[i].class_name,
                         class_len < 8 ? class_len : 8) == 0) {
            return &monitored_classes[i];
        }
    }
    return NULL;
}

/*-------------------------------------------------------------------
 * is_monitored_resource - Check if resource requires enhanced logging
 *-------------------------------------------------------------------*/
static int is_monitored_resource(const char *resource, uint16_t res_len) {
    for (int i = 0; monitored_resources[i].pattern_len > 0; i++) {
        int plen = monitored_resources[i].pattern_len;
        if (res_len >= plen) {
            if (memcmp_inline(resource, monitored_resources[i].pattern,
                             plen) == 0) {
                return 1;
            }
        }
    }
    return 0;
}

/*-------------------------------------------------------------------
 * is_exempt_user - Check if user is exempt from enhanced logging
 *-------------------------------------------------------------------*/
static int is_exempt_user(const char userid[8]) {
    for (int i = 0; exempt_users[i][0] != '\0'; i++) {
        if (memcmp_inline(userid, exempt_users[i], 8) == 0) {
            return 1;
        }
    }
    return 0;
}

/*-------------------------------------------------------------------
 * format_access_type - Get text description of access type
 *-------------------------------------------------------------------*/
static const char *format_access_type(uint8_t attr) {
    if (attr & RCXATTR_ALTER)   return "ALTER  ";
    if (attr & RCXATTR_CONTROL) return "CONTROL";
    if (attr & RCXATTR_UPDATE)  return "UPDATE ";
    if (attr & RCXATTR_READ)    return "READ   ";
    return "UNKNOWN";
}

/*-------------------------------------------------------------------
 * log_to_syslog - Write message to system log
 * Note: In production, this would use WTO or write to a log dataset
 *-------------------------------------------------------------------*/
static void log_to_syslog(const char *msg, int msg_len) {
    /*
     * In a real implementation, this would:
     * 1. Issue a WTO for operator visibility, or
     * 2. Write to an SMF record, or
     * 3. Append to a log dataset
     *
     * For this example, we just demonstrate the interface.
     * The actual WTO macro would be:
     *
     * __asm__ volatile (
     *     "WTO   '%s',ROUTCDE=(1,11),DESC=(6)"
     *     : : "r"(msg)
     * );
     */
    (void)msg;      /* Suppress unused parameter warning */
    (void)msg_len;
}

/*===================================================================
 * ICHRCX02 - RACROUTE AUTH Post-processing Exit
 *
 * Called after RACF has made an authorization decision. This exit
 * can log the decision but should not change it (use ICHRCX01 for
 * pre-processing if you need to modify the decision).
 *
 * Input:
 *   parm - Pointer to RCX parameter list containing:
 *          - rcxacee:   ACEE of requestor
 *          - rcxclass:  Resource class
 *          - rcxentty:  Resource name (entity)
 *          - rcxattr:   Access type requested
 *          - rcxreasn:  RACF reason code (0 = allowed)
 *
 * Return:
 *   RACF_RCXRC_CONTINUE (0) - Continue with RACF's decision
 *
 * Note: This is a post-processing exit. Return code doesn't change
 *       the authorization decision - it's already been made.
 *===================================================================*/

#pragma prolog(ICHRCX02, "SAVE(14,12),LR(12,15)")
#pragma epilog(ICHRCX02, "RETURN(14,12)")

int ICHRCX02(struct racf_rcx_parm *parm) {
    struct acee *acee;
    char userid[8];
    const monitored_class_t *mon_class;
    int should_log;
    int access_granted;
    char log_buffer[120];
    int log_pos;

    /* Validate input */
    if (parm == NULL || parm->rcxacee == NULL) {
        return RACF_RCXRC_CONTINUE;
    }

    acee = (struct acee *)parm->rcxacee;
    racf_get_userid(acee, userid);

    /* Skip exempt users (system accounts) */
    if (is_exempt_user(userid)) {
        return RACF_RCXRC_CONTINUE;
    }

    /* Determine if access was granted */
    access_granted = (parm->rcxreasn == 0);
    should_log = 0;

    /* Check if this class requires enhanced logging */
    mon_class = is_monitored_class(parm->rcxclass, parm->rcxclsln);
    if (mon_class != NULL) {
        if (access_granted && mon_class->log_success) {
            should_log = 1;
        }
        if (!access_granted && mon_class->log_failure) {
            should_log = 1;
        }
    }

    /* Check if this is a monitored resource (always log if matched) */
    if (!should_log && parm->rcxentty != NULL) {
        if (is_monitored_resource(parm->rcxentty, parm->rcxentyl)) {
            should_log = 1;
        }
    }

    /* Always log if user has elevated privileges and access is denied */
    if (!should_log && !access_granted) {
        if (racf_is_special(acee) || racf_is_operations(acee)) {
            should_log = 1;  /* Privileged user denied - unusual */
        }
    }

    /* Build and write log entry if required */
    if (should_log) {
        log_pos = 0;

        /* Status: ALLOWED or DENIED */
        if (access_granted) {
            memcpy_inline(log_buffer + log_pos, "ALLOWED ", 8);
        } else {
            memcpy_inline(log_buffer + log_pos, "DENIED  ", 8);
        }
        log_pos += 8;

        /* User ID */
        memcpy_inline(log_buffer + log_pos, userid, 8);
        log_pos += 8;

        /* Space separator */
        log_buffer[log_pos++] = ' ';

        /* Access type */
        const char *acc_type = format_access_type(parm->rcxattr);
        int acc_len = 7;
        memcpy_inline(log_buffer + log_pos, acc_type, acc_len);
        log_pos += acc_len;

        /* Space separator */
        log_buffer[log_pos++] = ' ';

        /* Class name */
        int class_len = parm->rcxclsln;
        if (class_len > 8) class_len = 8;
        memcpy_inline(log_buffer + log_pos, parm->rcxclass, class_len);
        log_pos += class_len;

        /* Space separator */
        log_buffer[log_pos++] = ' ';

        /* Resource name (truncate if too long) */
        int res_len = parm->rcxentyl;
        if (res_len > 44) res_len = 44;
        if (parm->rcxentty != NULL && res_len > 0) {
            memcpy_inline(log_buffer + log_pos, parm->rcxentty, res_len);
            log_pos += res_len;
        }

        /* Null terminate */
        log_buffer[log_pos] = '\0';

        /* Write to log */
        log_to_syslog(log_buffer, log_pos);
    }

    /* Always continue with RACF's decision */
    return RACF_RCXRC_CONTINUE;
}

/*===================================================================
 * Additional Exit: ICHRIX02 - RACINIT Post-processing
 *
 * Log all sign-on events for security monitoring.
 *===================================================================*/

#pragma prolog(ICHRIX02, "SAVE(14,12),LR(12,15)")
#pragma epilog(ICHRIX02, "RETURN(14,12)")

int ICHRIX02(struct racf_rix_parm *parm) {
    char log_buffer[80];
    int log_pos = 0;
    int success;

    /* Only process logon requests */
    if (!(parm->rixflg1 & RIXFLG1_LOGON)) {
        return RACF_RIXRC_CONTINUE;
    }

    /* Determine success/failure from reason code */
    success = (parm->rixreasn == 0);

    /* Format: LOGON SUCCESS|FAILURE userid terminal */
    memcpy_inline(log_buffer + log_pos, "LOGON ", 6);
    log_pos += 6;

    if (success) {
        memcpy_inline(log_buffer + log_pos, "SUCCESS ", 8);
    } else {
        memcpy_inline(log_buffer + log_pos, "FAILURE ", 8);
    }
    log_pos += 8;

    /* User ID */
    if (parm->rixuser != NULL && parm->rixuserl > 0) {
        int ulen = parm->rixuserl;
        if (ulen > 8) ulen = 8;
        memcpy_inline(log_buffer + log_pos, parm->rixuser, ulen);
        log_pos += ulen;
    }

    log_buffer[log_pos++] = ' ';

    /* Terminal */
    if (parm->rixterm != NULL && parm->rixterml > 0) {
        int tlen = parm->rixterml;
        if (tlen > 8) tlen = 8;
        memcpy_inline(log_buffer + log_pos, parm->rixterm, tlen);
        log_pos += tlen;
    }

    /* Reason code for failures */
    if (!success) {
        log_buffer[log_pos++] = ' ';
        log_buffer[log_pos++] = 'R';
        log_buffer[log_pos++] = 'S';
        log_buffer[log_pos++] = 'N';
        log_buffer[log_pos++] = '=';

        /* Convert reason code to hex */
        uint32_t rsn = parm->rixreasn;
        static const char hex[] = "0123456789ABCDEF";
        for (int i = 7; i >= 0; i--) {
            log_buffer[log_pos + i] = hex[rsn & 0xF];
            rsn >>= 4;
        }
        log_pos += 8;
    }

    log_buffer[log_pos] = '\0';
    log_to_syslog(log_buffer, log_pos);

    return RACF_RIXRC_CONTINUE;
}
