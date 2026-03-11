/*********************************************************************
 * JES2 Output Routing Exit Example (Exit 4)
 *
 * This exit controls output routing, rerouting job output based on
 * job class, destination, and print volume thresholds.
 *
 * Exit Point: Exit 4 - Output Processing
 *
 * Build: Use IBM Metal C compiler with no LE dependencies
 *   xlc -qmetal -S jes2_exit4_output_routing.c
 *   as -o jes2_exit4_output_routing.o jes2_exit4_output_routing.s
 *   ld -o JES2X4EX jes2_exit4_output_routing.o
 *
 * Installation: Define as dynamic exit in JES2 parmlib
 *********************************************************************/

#include "metalc_base.h"
#include "metalc_jes2.h"

/*-------------------------------------------------------------------
 * Configuration - Customize for your installation
 *-------------------------------------------------------------------*/

/* Page threshold for redirecting to archive printer */
#define HIGH_VOLUME_THRESHOLD  1000

/* Classes to redirect high-volume output */
static const char redirect_classes[] = {'A', 'B', 'C', 'D', 'X', 0};

/* Destination for high-volume output (8 chars, blank padded) */
static const char archive_dest[8] = "ARCHIVE ";

/* Destination for confidential output */
static const char secure_dest[8]  = "SECURE  ";

/* Job prefixes for confidential output */
static const char *confidential_prefixes[] = {
    "PAY",       /* Payroll jobs */
    "HR",        /* HR jobs */
    "SEC",       /* Security jobs */
    "FIN",       /* Finance jobs */
    NULL
};

/*-------------------------------------------------------------------
 * is_redirect_class - Check if class should be redirected when high volume
 *-------------------------------------------------------------------*/
static int is_redirect_class(char jobclass) {
    for (int i = 0; redirect_classes[i] != 0; i++) {
        if (jobclass == redirect_classes[i]) {
            return 1;
        }
    }
    return 0;
}

/*-------------------------------------------------------------------
 * is_confidential_job - Check if job requires secure printer
 *-------------------------------------------------------------------*/
static int is_confidential_job(const char jobname[8]) {
    for (int i = 0; confidential_prefixes[i] != NULL; i++) {
        const char *prefix = confidential_prefixes[i];
        int len = 0;
        while (prefix[len] != '\0') len++;

        if (memcmp_inline(jobname, prefix, len) == 0) {
            return 1;
        }
    }
    return 0;
}

/*-------------------------------------------------------------------
 * copy_destination - Copy destination string to output area
 *-------------------------------------------------------------------*/
static void copy_destination(char *dest, const char *src, int len) {
    for (int i = 0; i < len; i++) {
        dest[i] = src[i];
    }
}

/*===================================================================
 * JES2X4EX - Output Processing Exit (Exit 4)
 *
 * Called when JES2 processes job output for routing.
 *
 * Input:
 *   parm - Pointer to Exit 4 parameter list containing:
 *          - e4jct:   JCT pointer
 *          - e4jqe:   JQE pointer
 *          - e4dest:  Current destination
 *          - e4destl: Destination length
 *          - e4flags: Processing flags
 *
 * Return:
 *   EXIT4_PROCESS (0) - Process normally with current destination
 *   EXIT4_REROUTE (4) - Reroute to modified destination
 *   EXIT4_DELETE  (8) - Delete the output (suppress printing)
 *
 * Note: This exit can modify the destination by updating e4dest
 *===================================================================*/

#pragma prolog(JES2X4EX, "SAVE(14,12),LR(12,15)")
#pragma epilog(JES2X4EX, "RETURN(14,12)")

int JES2X4EX(struct jes2_exit4_parm *parm) {
    struct jct *jct;
    uint32_t pages;

    /* Validate input */
    if (parm == NULL || parm->e4jct == NULL) {
        return EXIT4_PROCESS;
    }

    jct = (struct jct *)parm->e4jct;

    /* Skip system jobs - let them route normally */
    if (jes2_is_system_job(jct)) {
        return EXIT4_PROCESS;
    }

    /* Skip started tasks - they usually handle their own output */
    if (jes2_is_stc(jct)) {
        return EXIT4_PROCESS;
    }

    /* Check for confidential jobs - route to secure printer */
    if (is_confidential_job(jct->jctjname)) {
        /* Only reroute if not already going to secure destination */
        if (memcmp_inline(parm->e4dest, secure_dest, 8) != 0) {
            copy_destination(parm->e4dest, secure_dest, 8);
            parm->e4destl = 8;
            return EXIT4_REROUTE;
        }
        return EXIT4_PROCESS;
    }

    /* Check for high-volume output that should be archived */
    pages = jct->jctpages;

    if (pages > HIGH_VOLUME_THRESHOLD) {
        /* Only redirect certain classes */
        if (is_redirect_class(jct->jctjclas)) {
            /* Redirect to archive printer */
            copy_destination(parm->e4dest, archive_dest, 8);
            parm->e4destl = 8;
            return EXIT4_REROUTE;
        }
    }

    /* Suppress empty SYSOUT datasets (0 pages, 0 lines) */
    if (jct->jctpages == 0 && jct->jctlines == 0) {
        /*
         * Note: This aggressively deletes empty output.
         * In production, you might want to check for specific
         * DD names or message classes before deleting.
         */
        /* return EXIT4_DELETE; */  /* Uncomment to enable */
    }

    /* Default: process with original destination */
    return EXIT4_PROCESS;
}

/*===================================================================
 * Extended Example: Time-based routing
 *
 * Routes output to different printers based on time of day.
 * Night shift output goes to a different printer.
 *===================================================================*/

/* Night shift hours: 22:00 - 06:00 */
#define NIGHT_START  (22 * 60 * 60 * 100)  /* 22:00 in 0.01 sec units */
#define NIGHT_END    (6 * 60 * 60 * 100)   /* 06:00 in 0.01 sec units */

static const char night_dest[8] = "NIGHTPRT";

static int is_night_shift(uint32_t current_time) {
    /* Night spans midnight, so check both ranges */
    return (current_time >= NIGHT_START || current_time < NIGHT_END);
}

#pragma prolog(exit4_time_routing, "SAVE(14,12),LR(12,15)")
#pragma epilog(exit4_time_routing, "RETURN(14,12)")

int exit4_time_routing(struct jes2_exit4_parm *parm) {
    struct jct *jct;
    uint32_t end_time;

    if (parm == NULL || parm->e4jct == NULL) {
        return EXIT4_PROCESS;
    }

    jct = (struct jct *)parm->e4jct;

    /* Skip system jobs and STCs */
    if (jes2_is_system_job(jct) || jes2_is_stc(jct)) {
        return EXIT4_PROCESS;
    }

    /* Get job end time */
    end_time = jct->jctendtm;

    /* Route night shift output to dedicated printer */
    if (is_night_shift(end_time)) {
        /* Only class A-D go to night printer */
        if (jct->jctjclas >= 'A' && jct->jctjclas <= 'D') {
            copy_destination(parm->e4dest, night_dest, 8);
            parm->e4destl = 8;
            return EXIT4_REROUTE;
        }
    }

    return EXIT4_PROCESS;
}

/*===================================================================
 * Extended Example: Department-based routing
 *
 * Routes output based on account code to department printers.
 *===================================================================*/

struct dept_route {
    char account_prefix[4];  /* First 4 chars of account */
    char destination[8];     /* Target printer */
};

static const struct dept_route dept_routes[] = {
    { "ACCT", "PRT$ACCT" },   /* Accounting department */
    { "ENGG", "PRT$ENGG" },   /* Engineering department */
    { "MKTG", "PRT$MKTG" },   /* Marketing department */
    { "HR01", "PRT$HR  " },   /* HR department */
    { "",     "" }            /* End marker */
};

#pragma prolog(exit4_dept_routing, "SAVE(14,12),LR(12,15)")
#pragma epilog(exit4_dept_routing, "RETURN(14,12)")

int exit4_dept_routing(struct jes2_exit4_parm *parm) {
    struct jct *jct;
    int i;

    if (parm == NULL || parm->e4jct == NULL) {
        return EXIT4_PROCESS;
    }

    jct = (struct jct *)parm->e4jct;

    /* Skip system jobs and STCs */
    if (jes2_is_system_job(jct) || jes2_is_stc(jct)) {
        return EXIT4_PROCESS;
    }

    /* Check account code against routing table */
    for (i = 0; dept_routes[i].account_prefix[0] != '\0'; i++) {
        if (memcmp_inline(jct->jctacct,
                         dept_routes[i].account_prefix, 4) == 0) {
            copy_destination(parm->e4dest,
                           dept_routes[i].destination, 8);
            parm->e4destl = 8;
            return EXIT4_REROUTE;
        }
    }

    return EXIT4_PROCESS;
}
