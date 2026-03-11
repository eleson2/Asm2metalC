/*********************************************************************
 * JES2_EXIT1_BLOCK_USER.C - Example JES2 Exit 1 (Job Selection)
 * 
 * Purpose: Prevent user PELLE from running batch jobs
 * 
 * Exit Point: Exit 1 - Job Selection
 * Called:     When JES2 considers a job for execution
 * 
 * Build:
 *   xlc -qmetal -S -qlist jes2_exit1_block_user.c
 * 
 * Link with JES2 libraries and install as dynamic exit.
 *********************************************************************/

#include "metalc_base.h"
#include "metalc_jes2.h"

/*-------------------------------------------------------------------
 * Configuration - users to block
 *-------------------------------------------------------------------*/

/* Blocked user ID (8 chars, blank padded) */
static const char BLOCKED_USER[8] = "PELLE   ";

/*-------------------------------------------------------------------
 * Exit 1 - Job Selection
 * 
 * Return codes:
 *   EXIT1_SELECT (0) - Select job for execution
 *   EXIT1_DEFER  (4) - Defer job, try again later
 *   EXIT1_REJECT (8) - Reject job, fail it
 *-------------------------------------------------------------------*/

#pragma prolog(exit1_block_user,"SAVE(14,12),LR(12,15)")
#pragma epilog(exit1_block_user,"RETURN(14,12)")

int exit1_block_user(struct jes2_exit1_parm *parm) {
    struct jct *jct;
    
    /* Get JCT pointer */
    jct = (struct jct *)parm->e1jct;
    if (jct == NULL) {
        return EXIT1_SELECT;  /* No JCT, allow job */
    }
    
    /* Allow started tasks and TSO sessions without restriction */
    if (jes2_is_stc(jct) || jes2_is_tso(jct)) {
        return EXIT1_SELECT;
    }
    
    /* Check if this is user PELLE */
    if (memcmp_inline(jct->jcttsuid, BLOCKED_USER, 8) == 0) {
        /* 
         * User PELLE is not allowed to run batch jobs.
         * Return EXIT1_REJECT to fail the job.
         *
         * Note: In production, you would typically issue a message
         * to the job log explaining why the job was rejected.
         */
        return EXIT1_REJECT;
    }
    
    /* All other users - allow job to run */
    return EXIT1_SELECT;
}

/*-------------------------------------------------------------------
 * Alternative: Using Exit 8 (JOB Statement Processing)
 * 
 * Exit 8 is called earlier, during JCL processing. Rejecting here
 * gives the user immediate feedback rather than waiting until
 * execution time.
 *-------------------------------------------------------------------*/

#pragma prolog(exit8_block_user,"SAVE(14,12),LR(12,15)")
#pragma epilog(exit8_block_user,"RETURN(14,12)")

int exit8_block_user(struct jes2_exit8_parm *parm) {
    struct jct *jct;
    
    /* Get JCT pointer */
    jct = (struct jct *)parm->e8jct;
    if (jct == NULL) {
        return EXIT8_ACCEPT;
    }
    
    /* Allow STC and TSO */
    if (jes2_is_stc(jct) || jes2_is_tso(jct)) {
        return EXIT8_ACCEPT;
    }
    
    /* Check if this is user PELLE */
    if (memcmp_inline(jct->jcttsuid, BLOCKED_USER, 8) == 0) {
        /*
         * Reject at JOB statement processing.
         * Job will fail immediately with JCL error.
         */
        return EXIT8_REJECT;
    }
    
    return EXIT8_ACCEPT;
}

/*-------------------------------------------------------------------
 * Extended version: Block multiple users with logging
 *-------------------------------------------------------------------*/

/* List of blocked users */
static const char BLOCKED_USERS[][8] = {
    "PELLE   ",
    "BADUSER ",
    "TESTID  "
};
#define NUM_BLOCKED_USERS 3

/* Check if user is in blocked list */
static int is_user_blocked(const char userid[8]) {
    int i;
    for (i = 0; i < NUM_BLOCKED_USERS; i++) {
        if (memcmp_inline(userid, BLOCKED_USERS[i], 8) == 0) {
            return 1;  /* User is blocked */
        }
    }
    return 0;  /* User is allowed */
}

#pragma prolog(exit1_block_users_extended,"SAVE(14,12),LR(12,15)")
#pragma epilog(exit1_block_users_extended,"RETURN(14,12)")

int exit1_block_users_extended(struct jes2_exit1_parm *parm) {
    struct jct *jct;
    
    jct = (struct jct *)parm->e1jct;
    if (jct == NULL) {
        return EXIT1_SELECT;
    }
    
    /* Allow STC and TSO */
    if (jes2_is_stc(jct) || jes2_is_tso(jct)) {
        return EXIT1_SELECT;
    }
    
    /* Check blocked user list */
    if (is_user_blocked(jct->jcttsuid)) {
        /*
         * In production, add WTO or logging here:
         * 
         * char msg[80];
         * format_message(msg, "JOB %.8s REJECTED - USER %.8s NOT AUTHORIZED",
         *                jct->jctjname, jct->jcttsuid);
         * issue_wto(msg);
         */
        return EXIT1_REJECT;
    }
    
    return EXIT1_SELECT;
}
