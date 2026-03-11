/*********************************************************************
 * SMF Record Filtering Exit Example (IEFU83)
 *
 * This exit filters SMF records before they are written to the
 * SMF data set, allowing selective suppression based on record
 * type, job name, time of day, or content.
 *
 * Exit Point: IEFU83 - SMF Record Filter
 *
 * Build: Use IBM Metal C compiler with no LE dependencies
 *   xlc -qmetal -S smf_record_filter_exit.c
 *   as -o smf_record_filter_exit.o smf_record_filter_exit.s
 *   ld -o IEFU83EX smf_record_filter_exit.o
 *
 * Installation: Add to SMFPRMxx: EXITS(IEFU83(IEFU83EX))
 *********************************************************************/

#include "metalc_base.h"
#include "metalc_smf.h"

/*-------------------------------------------------------------------
 * Configuration - Customize for your installation
 *-------------------------------------------------------------------*/

/* Record types to always suppress (reduces SMF volume) */
static const uint8_t suppress_types[] = {
    SMF_TYPE_42,     /* DFSMS statistics - often high volume      */
    SMF_TYPE_99,     /* User records (if unused)                  */
    0                /* End of list marker                        */
};

/* Job prefixes to always allow (bypass other filters) */
static const char *allow_prefixes[] = {
    "PROD",          /* Production jobs                           */
    "SEC",           /* Security jobs                             */
    "AUDIT",         /* Audit jobs                                */
    NULL             /* End of list marker                        */
};

/* Job prefixes to always suppress */
static const char *suppress_prefixes[] = {
    "TEST",          /* Test jobs                                 */
    "DEV",           /* Development jobs                          */
    NULL             /* End of list marker                        */
};

/*-------------------------------------------------------------------
 * is_suppressed_type - Check if record type should be suppressed
 *-------------------------------------------------------------------*/
static int is_suppressed_type(uint8_t record_type) {
    for (int i = 0; suppress_types[i] != 0; i++) {
        if (record_type == suppress_types[i]) {
            return 1;
        }
    }
    return 0;
}

/*-------------------------------------------------------------------
 * matches_prefix_list - Check if job name matches any prefix in list
 *-------------------------------------------------------------------*/
static int matches_prefix_list(const char jobname[8], const char **prefixes) {
    for (int i = 0; prefixes[i] != NULL; i++) {
        const char *prefix = prefixes[i];
        int prefix_len = 0;
        while (prefix[prefix_len] != '\0' && prefix_len < 8) {
            prefix_len++;
        }
        if (smf_match_prefix(jobname, prefix, prefix_len)) {
            return 1;
        }
    }
    return 0;
}

/*-------------------------------------------------------------------
 * get_jobname_from_record - Extract job name based on record type
 * Returns pointer to 8-char job name or NULL if not available
 *-------------------------------------------------------------------*/
static const char *get_jobname_from_record(const void *record) {
    const struct smf_header *hdr = (const struct smf_header *)record;
    uint8_t rtype = hdr->smfrty;

    switch (rtype) {
        case SMF_TYPE_4: {
            const struct smf4_record *r4 = (const struct smf4_record *)record;
            return r4->smf4jbn;
        }
        case SMF_TYPE_5: {
            const struct smf5_record *r5 = (const struct smf5_record *)record;
            return r5->smf5jbn;
        }
        case SMF_TYPE_14:
        case SMF_TYPE_15: {
            const struct smf1415_record *r14 = (const struct smf1415_record *)record;
            return r14->smf14jbn;
        }
        case SMF_TYPE_30: {
            /* Type 30 requires navigating to ID section via offset */
            const struct smf30_header *r30 = (const struct smf30_header *)record;
            if (r30->smf30sol >= 8) {
                const char *base = (const char *)record;
                const struct smf30_id_section *id =
                    (const struct smf30_id_section *)(base + r30->smf30sof);
                return id->smf30jbn;
            }
            return NULL;
        }
        default:
            return NULL;
    }
}

/*-------------------------------------------------------------------
 * is_business_hours - Check if time is during business hours
 * Business hours: 07:00 - 18:00 (7 AM to 6 PM)
 *-------------------------------------------------------------------*/
static int is_business_hours(uint32_t smf_time) {
    /* SMF time is in 0.01 second units since midnight */
    /* 07:00:00 = 7 * 60 * 60 * 100 = 2520000 */
    /* 18:00:00 = 18 * 60 * 60 * 100 = 6480000 */
    return smf_time_in_range(smf_time, 2520000, 6480000);
}

/*===================================================================
 * IEFU83EX - SMF Record Filter Exit
 *
 * Called before each SMF record is written to the SMF data set.
 *
 * Input:
 *   parm - Pointer to SMF exit parameter list containing:
 *          - record_ptr: Pointer to the SMF record
 *          - record_len: Length of the record
 *
 * Return:
 *   SMF_RC_WRITE    (0) - Write the record
 *   SMF_RC_SUPPRESS (4) - Suppress the record (do not write)
 *   SMF_RC_HALT     (8) - Halt SMF recording (use with caution!)
 *
 * Note: This exit runs in cross-memory mode and must be reentrant.
 *       Do not issue any SVCs or use non-reentrant code.
 *===================================================================*/

#pragma prolog(IEFU83EX, "STM(14,12,12(13)),LR(12,15)")
#pragma epilog(IEFU83EX, "LM(14,12,12(13)),BR(14)")

int IEFU83EX(struct smf_exit_parm *parm) {
    const struct smf_header *record;
    uint8_t record_type;
    const char *jobname;

    /* Validate input */
    if (parm == NULL || parm->record_ptr == NULL) {
        return SMF_RC_WRITE;  /* Safety: write on error */
    }

    record = (const struct smf_header *)parm->record_ptr;
    record_type = smf_get_record_type(record);

    /* Step 1: Check if this record type should always be suppressed */
    if (is_suppressed_type(record_type)) {
        return SMF_RC_SUPPRESS;
    }

    /* Step 2: Get job name if available */
    jobname = get_jobname_from_record(record);

    if (jobname != NULL) {
        /* Step 3: Always allow critical job prefixes */
        if (matches_prefix_list(jobname, allow_prefixes)) {
            return SMF_RC_WRITE;
        }

        /* Step 4: Always suppress test/dev job prefixes */
        if (matches_prefix_list(jobname, suppress_prefixes)) {
            return SMF_RC_SUPPRESS;
        }
    }

    /* Step 5: Suppress high-volume records outside business hours */
    /* (Allows collecting full data during day for performance analysis) */
    if (record_type == SMF_TYPE_30 ||
        record_type == SMF_TYPE_72 ||
        record_type == SMF_TYPE_78) {

        if (!is_business_hours(record->smftme)) {
            /* Check subtype - always keep job termination records */
            if (record_type == SMF_TYPE_30 && smf_has_subtype(record)) {
                uint16_t subtype = smf_get_subtype(record);
                /* Keep subtypes 3 and 4 (step/job termination) */
                if (subtype == SMF30_SUBTYPE_3 || subtype == SMF30_SUBTYPE_4) {
                    return SMF_RC_WRITE;
                }
            }
            /* Suppress interval records at night */
            return SMF_RC_SUPPRESS;
        }
    }

    /* Default: write the record */
    return SMF_RC_WRITE;
}
