/*********************************************************************
 * DFSMS ACS Routine Example (Storage Class Selection)
 *
 * This ACS routine selects storage class based on dataset name
 * patterns, application requirements, and performance needs.
 *
 * Exit Point: IGDACSSC - Storage Class ACS Routine
 *
 * Build: Use IBM Metal C compiler with no LE dependencies
 *   xlc -qmetal -S dfsms_acs_routine.c
 *   as -o dfsms_acs_routine.o dfsms_acs_routine.s
 *   ld -o IGDACSSC dfsms_acs_routine.o
 *
 * Installation: Define via ISMF, specify in ACS source
 *********************************************************************/

#include "metalc_base.h"
#include "metalc_dfsms.h"

/*-------------------------------------------------------------------
 * Configuration - Storage Class Assignments
 *-------------------------------------------------------------------*/

/* Dataset HLQ to storage class mapping */
typedef struct {
    char hlq[8];              /* Dataset HLQ (blank padded) */
    int  hlq_len;             /* Length of HLQ */
    char storage_class[8];    /* Storage class name */
} hlq_class_map_t;

static const hlq_class_map_t hlq_mappings[] = {
    { "PAYROLL", 7, "SCFAST  " },   /* Payroll - fast storage */
    { "DB2",     3, "SCDB2   " },   /* DB2 - database optimized */
    { "CICS",    4, "SCCICS  " },   /* CICS - transaction storage */
    { "TEMP",    4, "SCTEMP  " },   /* Temporary - cheaper storage */
    { "ARCHIVE", 7, "SCARCHVE" },   /* Archive - slow/cheap */
    { "PROD",    4, "SCPROD  " },   /* Production - reliable */
    { "TEST",    4, "SCTEST  " },   /* Test - basic storage */
    { "",        0, ""        }     /* End marker */
};

/* Special dataset patterns for performance-critical data */
typedef struct {
    char pattern[20];         /* Pattern to match */
    int  pattern_len;         /* Pattern length */
    char storage_class[8];    /* Storage class */
} pattern_class_map_t;

static const pattern_class_map_t pattern_mappings[] = {
    { ".DBRM",             5, "SCDB2   " },  /* DB2 DBRMs */
    { ".LOADLIB",          8, "SCFAST  " },  /* Load libraries */
    { ".INDEX.",           7, "SCFAST  " },  /* Index datasets */
    { ".LOG.",             5, "SCFAST  " },  /* Log datasets */
    { ".SPOOL.",           7, "SCTEMP  " },  /* Spool datasets */
    { ".WORK.",            6, "SCTEMP  " },  /* Work datasets */
    { ".GDG",              4, "SCARCHVE" },  /* GDG base */
    { "",                  0, ""        }    /* End marker */
};

/* Default storage class for unmatched datasets */
static const char default_storclas[8] = "SCSTD   ";

/*-------------------------------------------------------------------
 * get_hlq - Extract HLQ from dataset name
 * Returns length of HLQ
 *-------------------------------------------------------------------*/
static int get_hlq(const char dsname[44], char hlq[8]) {
    int i;

    /* Initialize HLQ to blanks */
    for (i = 0; i < 8; i++) {
        hlq[i] = ' ';
    }

    /* Copy characters until dot or end */
    for (i = 0; i < 8 && i < 44; i++) {
        if (dsname[i] == '.' || dsname[i] == ' ') {
            break;
        }
        hlq[i] = dsname[i];
    }

    return i;
}

/*-------------------------------------------------------------------
 * contains_pattern - Check if dataset name contains pattern
 *-------------------------------------------------------------------*/
static int contains_pattern(const char dsname[44], const char *pattern, int plen) {
    for (int i = 0; i <= 44 - plen; i++) {
        if (dsname[i] == ' ') break;  /* End of name */

        int match = 1;
        for (int j = 0; j < plen; j++) {
            if (dsname[i + j] != pattern[j]) {
                match = 0;
                break;
            }
        }
        if (match) return 1;
    }
    return 0;
}

/*-------------------------------------------------------------------
 * match_hlq - Match HLQ against mapping table
 * Returns storage class or NULL if no match
 *-------------------------------------------------------------------*/
static const char *match_hlq(const char hlq[8], int hlq_len) {
    for (int i = 0; hlq_mappings[i].hlq_len > 0; i++) {
        if (hlq_len == hlq_mappings[i].hlq_len) {
            if (memcmp_inline(hlq, hlq_mappings[i].hlq, hlq_len) == 0) {
                return hlq_mappings[i].storage_class;
            }
        }
    }
    return NULL;
}

/*-------------------------------------------------------------------
 * match_pattern - Match dataset pattern against mapping table
 * Returns storage class or NULL if no match
 *-------------------------------------------------------------------*/
static const char *match_pattern(const char dsname[44]) {
    for (int i = 0; pattern_mappings[i].pattern_len > 0; i++) {
        if (contains_pattern(dsname, pattern_mappings[i].pattern,
                            pattern_mappings[i].pattern_len)) {
            return pattern_mappings[i].storage_class;
        }
    }
    return NULL;
}

/*-------------------------------------------------------------------
 * copy_storclas - Copy storage class name
 *-------------------------------------------------------------------*/
static void copy_storclas(char dest[8], const char src[8]) {
    for (int i = 0; i < 8; i++) {
        dest[i] = src[i];
    }
}

/*===================================================================
 * ACS Output Structure (simplified)
 *===================================================================*/

#pragma pack(1)

struct acs_output {
    char           storclas[8];       /* Storage class name */
    char           mgmtclas[8];       /* Management class name */
    char           dataclas[8];       /* Data class name */
    char           storgrp[8];        /* Storage group name */
    uint32_t       retcode;           /* Return code */
    uint32_t       reason;            /* Reason code */
};

#pragma pack()

/*===================================================================
 * IGDACSSC - Storage Class ACS Routine
 *
 * Called during dataset allocation to select storage class.
 *
 * Input:
 *   read_vars - Read-only variables containing:
 *               - dsname:  Dataset name
 *               - jobname: Job name
 *               - userid:  User ID
 *               - pgmname: Program name
 *               - dataclas: Requested data class
 *
 *   output - Output area for selected classes
 *
 * Return:
 *   ACS_RC_CONTINUE  (0) - No storage class selected
 *   ACS_RC_SELECTED  (4) - Storage class selected
 *   ACS_RC_FAIL      (8) - ACS failure
 *===================================================================*/

#pragma prolog(IGDACSSC, "SAVE(14,12),LR(12,15)")
#pragma epilog(IGDACSSC, "RETURN(14,12)")

int IGDACSSC(struct acs_read_vars *read_vars, struct acs_output *output) {
    char hlq[8];
    int hlq_len;
    const char *selected_class = NULL;

    /* Validate input */
    if (read_vars == NULL || output == NULL) {
        return ACS_RC_FAIL;
    }

    /* Initialize output to blanks */
    for (int i = 0; i < 8; i++) {
        output->storclas[i] = ' ';
    }

    /* If storage class already requested, honor it */
    if (read_vars->storclas[0] != ' ' && read_vars->storclas[0] != '\0') {
        /* User/JCL specified storage class - use it */
        copy_storclas(output->storclas, read_vars->storclas);
        return ACS_RC_SELECTED;
    }

    /* Extract HLQ from dataset name */
    hlq_len = get_hlq(read_vars->dsname, hlq);

    /* Try to match HLQ first */
    selected_class = match_hlq(hlq, hlq_len);

    /* If no HLQ match, try pattern matching */
    if (selected_class == NULL) {
        selected_class = match_pattern(read_vars->dsname);
    }

    /* If still no match, use default */
    if (selected_class == NULL) {
        selected_class = default_storclas;
    }

    /* Set the selected storage class */
    copy_storclas(output->storclas, selected_class);

    return ACS_RC_SELECTED;
}

/*===================================================================
 * Additional: Data Class ACS Routine (IGDACSDC)
 *
 * Select data class based on dataset characteristics.
 *===================================================================*/

/* Data class mappings based on RECFM and LRECL */
static const char dc_fb_small[8]  = "DCFBSML ";   /* FB, LRECL <= 80 */
static const char dc_fb_large[8]  = "DCFBLRG ";   /* FB, LRECL > 80 */
static const char dc_vb[8]        = "DCVB    ";   /* VB records */
static const char dc_default[8]   = "DCSTD   ";   /* Default */

#pragma prolog(IGDACSDC, "SAVE(14,12),LR(12,15)")
#pragma epilog(IGDACSDC, "RETURN(14,12)")

int IGDACSDC(struct acs_read_vars *read_vars, struct acs_output *output) {
    const char *selected_class = NULL;

    /* Validate input */
    if (read_vars == NULL || output == NULL) {
        return ACS_RC_FAIL;
    }

    /* Initialize output */
    for (int i = 0; i < 8; i++) {
        output->dataclas[i] = ' ';
    }

    /* If data class already requested, honor it */
    if (read_vars->dataclas[0] != ' ' && read_vars->dataclas[0] != '\0') {
        copy_storclas(output->dataclas, read_vars->dataclas);
        return ACS_RC_SELECTED;
    }

    /* Select based on record format */
    if (read_vars->recfm[0] == 'F') {
        /* Fixed records */
        if (read_vars->lrecl <= 80) {
            selected_class = dc_fb_small;
        } else {
            selected_class = dc_fb_large;
        }
    } else if (read_vars->recfm[0] == 'V') {
        /* Variable records */
        selected_class = dc_vb;
    } else {
        selected_class = dc_default;
    }

    copy_storclas(output->dataclas, selected_class);
    return ACS_RC_SELECTED;
}
