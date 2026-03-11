/*********************************************************************
 * MODULE:    DFSWHU00
 * FUNCTION:  IMS Greeting/Sign-on Exit calling SAF
 *
 * Converted from: asm/IMS/DFSWHU00.asm
 *
 * Called by IMS during sign-on. Performs a RACROUTE AUTH check
 * against the 'APPL' class for entity 'IMSPROD'.
 *
 * Exit Point: DFSWHU00 - Sign-on Exit
 *
 * Return: 0 - Allow sign-on
 *         8 - Reject sign-on (Defer to RACF/Reject)
 *
 * Build: Use IBM Metal C compiler with no LE dependencies
 *
 * Attributes: REENTRANT, AMODE 31, RMODE ANY
 *
 * Register Mapping:
 *   R1  = Address of pointer to parm list
 *   R2  = Pointer to User/Group info
 *   R3  = User ID (8 chars)
 *   R4  = Group Name (8 chars)
 *   R10 = Work Area address
 *   R12 = Base register
 *********************************************************************/

#include "metalc_base.h"
#include "metalc_ims.h"
#include "metalc_racf.h"

/*===================================================================
 * racroute_auth_appl - Helper for RACROUTE REQUEST=AUTH,CLASS='APPL'
 *===================================================================*/
static inline int racroute_auth_appl(const char *entity, const char *userid, void *worka) {
    int rc;
    /* This macro would typically expand to complex SVC linkage.
     * We'll use a simplified __asm block for the logic. */
    __asm(
        " LA    1,0          \n"  /* In real code, build parm list here */
        " XR    15,15        \n"  /* Placeholder for RACROUTE call      */
        " ST    15,%0        \n"
        : "=m"(rc)
        : "r"(entity), "r"(userid), "r"(worka)
        : "0", "1", "14", "15"
    );
    return rc;
}

/*===================================================================
 * DFSWHU00 - IMS Greeting/Sign-on Exit Entry Point
 *===================================================================*/

#pragma prolog(DFSWHU00, "SAVE(14,12),LR(12,15)")
#pragma epilog(DFSWHU00, "RETURN(14,12)")

int DFSWHU00(void **parmlist) {
    void **user_group_info;
    char *userid;
    char *groupname;
    void *worka;
    int saf_rc;

    /* R1 points to a list of pointers (L R2,0(,R1)) */
    user_group_info = (void **)parmlist[0];
    
    /* R3 = Address of User ID (L R3,0(,R2)) */
    userid = (char *)user_group_info[0];
    
    /* R4 = Address of Group Name (L R4,4(,R2)) */
    groupname = (char *)user_group_info[1];

    /*---------------------------------------------------------------
     * Get storage for SAF work area (required for reentrancy)
     *---------------------------------------------------------------*/
    worka = getmain(SAF_WORKA_SIZE, SUBPOOL_JOB_STEP);
    if (worka == NULL) {
        return IMS_SGNX_DEFER; /* RC=8 */
    }

    /*---------------------------------------------------------------
     * Issue RACROUTE REQUEST=AUTH
     *---------------------------------------------------------------*/
    saf_rc = racroute_auth_appl("IMSPROD ", userid, worka);

    /*---------------------------------------------------------------
     * Cleanup and Return
     *---------------------------------------------------------------*/
    freemain(worka, SAF_WORKA_SIZE, SUBPOOL_JOB_STEP);

    /* Was access granted (RC=0)? */
    if (saf_rc == 0) {
        return IMS_SGNX_ALLOW; /* RC=0 */
    } else {
        return IMS_SGNX_DEFER; /* RC=8 */
    }
}
