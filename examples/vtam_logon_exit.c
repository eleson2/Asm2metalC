/*********************************************************************
 * VTAM Logon Exit Example (ISTEXCLY)
 *
 * This exit validates VTAM logon requests, enforcing application
 * access restrictions and logging session activity.
 *
 * Exit Point: ISTEXCLY - Logon Verify Exit
 *
 * Build: Use IBM Metal C compiler with no LE dependencies
 *   xlc -qmetal -S vtam_logon_exit.c
 *   as -o vtam_logon_exit.o vtam_logon_exit.s
 *   ld -o ISTEXCLY vtam_logon_exit.o
 *
 * Installation: Define in VTAM APPL definition
 *********************************************************************/

#include "metalc_base.h"
#include "metalc_vtam.h"

/*-------------------------------------------------------------------
 * Configuration - Customize for your installation
 *-------------------------------------------------------------------*/

/* Applications requiring special authorization */
typedef struct {
    char appl_name[8];      /* Application name (blank padded) */
    int  admin_only;        /* Requires admin user */
    int  lu_restricted;     /* Restrict to specific LU patterns */
    char lu_prefix[4];      /* Required LU prefix */
} controlled_appl_t;

static const controlled_appl_t controlled_appls[] = {
    { "CICSADMN", 1, 0, ""    },   /* CICS Admin - admin only */
    { "CICSPROD", 0, 1, "TRM" },   /* CICS Prod - TRMxxx terminals */
    { "TSO     ", 0, 0, ""    },   /* TSO - no restrictions */
    { "IMSCTL  ", 1, 0, ""    },   /* IMS Control - admin only */
    { "",         0, 0, ""    }    /* End marker */
};

/* Admin users */
static const char admin_users[][8] = {
    "SYSADMIN",
    "NETADMIN",
    "SECADMIN",
    ""           /* End marker */
};

/* LU names blocked from all access */
static const char blocked_lus[][8] = {
    "BADTERM1",
    "TESTONLY",
    ""           /* End marker */
};

/* Custom reason codes for this exit */
#define VTMX_RSN_LU_BLOCKED        2001
#define VTMX_RSN_ADMIN_REQUIRED    2002
#define VTMX_RSN_LU_RESTRICTED     2003
#define VTMX_RSN_APPL_UNAVAILABLE  2004

/*-------------------------------------------------------------------
 * find_controlled_appl - Find application in controlled list
 *-------------------------------------------------------------------*/
static const controlled_appl_t *find_controlled_appl(const char appl[8]) {
    for (int i = 0; controlled_appls[i].appl_name[0] != '\0'; i++) {
        if (memcmp_inline(appl, controlled_appls[i].appl_name, 8) == 0) {
            return &controlled_appls[i];
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
 * is_blocked_lu - Check if LU is blocked
 *-------------------------------------------------------------------*/
static int is_blocked_lu(const char lu[8]) {
    for (int i = 0; blocked_lus[i][0] != '\0'; i++) {
        if (memcmp_inline(lu, blocked_lus[i], 8) == 0) {
            return 1;
        }
    }
    return 0;
}

/*-------------------------------------------------------------------
 * matches_lu_prefix - Check if LU matches required prefix
 *-------------------------------------------------------------------*/
static int matches_lu_prefix(const char lu[8], const char prefix[4]) {
    int plen = 0;
    while (plen < 4 && prefix[plen] != '\0' && prefix[plen] != ' ') {
        plen++;
    }
    if (plen == 0) return 1;  /* No prefix = all allowed */
    return memcmp_inline(lu, prefix, plen) == 0;
}

/*-------------------------------------------------------------------
 * log_logon_attempt - Log logon attempt
 *-------------------------------------------------------------------*/
static void log_logon_attempt(const char *status,
                             const char lu[8],
                             const char appl[8],
                             const char user[8]) {
    /*
     * Production implementation would:
     * 1. Write to SMF record
     * 2. Write to security log
     * 3. Issue WTO for security events
     */
    (void)status;
    (void)lu;
    (void)appl;
    (void)user;
}

/*===================================================================
 * ISTEXCLY - VTAM Logon Verify Exit
 *
 * Called when a user attempts to log on to a VTAM application.
 *
 * Input:
 *   parm - Pointer to logon exit parameter list containing:
 *          - lyfunc:   Function code (logon/logoff/verify)
 *          - lyluname: Logical unit name
 *          - lyappl:   Application name
 *          - lyuserid: User ID
 *          - lypass:   Password (usually masked)
 *          - lyflags:  Processing flags
 *
 * Return:
 *   VTAM_LY_ACCEPT   (0)  - Accept logon
 *   VTAM_LY_REJECT   (4)  - Reject logon
 *   VTAM_LY_DEFER    (8)  - Defer to RACF/ACF2
 *   VTAM_LY_REDIRECT (12) - Redirect to application in lynewapp
 *===================================================================*/

#pragma prolog(ISTEXCLY, "SAVE(14,12),LR(12,15)")
#pragma epilog(ISTEXCLY, "RETURN(14,12)")

int ISTEXCLY(struct vtam_ly_parm *parm) {
    const controlled_appl_t *ctrl_appl;

    /* Validate input */
    if (parm == NULL) {
        return VTAM_LY_DEFER;
    }

    /* Handle different function codes */
    switch (parm->lyfunc) {

    case LY_FUNC_LOGON:
        /*
         * Logon request - perform validation
         */

        /* Check if LU is blocked */
        if (is_blocked_lu(parm->lyluname)) {
            parm->lyreasn = VTMX_RSN_LU_BLOCKED;
            parm->lysense = VTAM_SENSE_SECURITY;
            log_logon_attempt("BLOCKED", parm->lyluname,
                            parm->lyappl, parm->lyuserid);
            return VTAM_LY_REJECT;
        }

        /* Find application in controlled list */
        ctrl_appl = find_controlled_appl(parm->lyappl);

        if (ctrl_appl != NULL) {
            /* Check admin requirement */
            if (ctrl_appl->admin_only) {
                if (!is_admin_user(parm->lyuserid)) {
                    parm->lyreasn = VTMX_RSN_ADMIN_REQUIRED;
                    parm->lysense = VTAM_SENSE_SECURITY;
                    log_logon_attempt("DENIED-AUTH", parm->lyluname,
                                    parm->lyappl, parm->lyuserid);
                    return VTAM_LY_REJECT;
                }
            }

            /* Check LU restriction */
            if (ctrl_appl->lu_restricted) {
                if (!matches_lu_prefix(parm->lyluname, ctrl_appl->lu_prefix)) {
                    parm->lyreasn = VTMX_RSN_LU_RESTRICTED;
                    parm->lysense = VTAM_SENSE_APPL_REJ;
                    log_logon_attempt("DENIED-LU", parm->lyluname,
                                    parm->lyappl, parm->lyuserid);
                    return VTAM_LY_REJECT;
                }
            }
        }

        /* Log successful validation (before RACF check) */
        log_logon_attempt("PENDING", parm->lyluname,
                        parm->lyappl, parm->lyuserid);

        /* Defer to RACF for password verification */
        return VTAM_LY_DEFER;

    case LY_FUNC_LOGOFF:
        /*
         * Logoff notification - log session end
         */
        log_logon_attempt("LOGOFF", parm->lyluname,
                        parm->lyappl, parm->lyuserid);
        return VTAM_LY_ACCEPT;

    case LY_FUNC_VERIFY:
        /*
         * Verification only - no actual logon
         */
        return VTAM_LY_DEFER;

    case LY_FUNC_INIT:
    case LY_FUNC_TERM:
        /*
         * Exit initialization/termination
         */
        return VTAM_LY_ACCEPT;

    default:
        return VTAM_LY_DEFER;
    }
}

/*===================================================================
 * Additional Exit: USS Verification (ISTEXCUV)
 *
 * Process USS (Unformatted System Services) commands.
 *===================================================================*/

/* USS commands to block */
static const char blocked_uss_cmds[][8] = {
    "IBMTEST ",   /* Test command */
    "DEBUG   ",   /* Debug command */
    ""            /* End marker */
};

static int is_blocked_uss_cmd(const char *cmd, uint32_t len) {
    for (int i = 0; blocked_uss_cmds[i][0] != '\0'; i++) {
        int clen = 0;
        while (clen < 8 && blocked_uss_cmds[i][clen] != ' ') clen++;
        if (len >= (uint32_t)clen &&
            memcmp_inline(cmd, blocked_uss_cmds[i], clen) == 0) {
            return 1;
        }
    }
    return 0;
}

#pragma prolog(ISTEXCUV, "SAVE(14,12),LR(12,15)")
#pragma epilog(ISTEXCUV, "RETURN(14,12)")

int ISTEXCUV(struct vtam_uv_parm *parm) {
    /* Validate input */
    if (parm == NULL) {
        return VTAM_USS_CONTINUE;
    }

    /* Check for blocked USS commands */
    if (parm->uvfunc == UV_FUNC_OTHER) {
        if (parm->uvcmd != NULL && parm->uvcmdlen > 0) {
            if (is_blocked_uss_cmd(parm->uvcmd, parm->uvcmdlen)) {
                parm->uvreasn = 3001;
                return VTAM_USS_REJECT;
            }
        }
    }

    return VTAM_USS_CONTINUE;
}
