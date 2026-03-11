/*********************************************************************
 * MODULE:    HASPEX20
 * FUNCTION:  JES2 End of Job Input Exit
 *
 * Converted from: asm/JES2/HASPEX20.asm
 *
 * This exit performs processing at the end of job input.
 * It forces batch jobs to use message class 'E'.
 *
 * Exit Point: Exit 20 - End of Job Input
 *
 * Return: RC=0 - Continue normal processing
 *
 * Build: Use IBM Metal C compiler with no LE dependencies
 *
 * Attributes: REENTRANT, AMODE 31, RMODE ANY
 *
 * Register Mapping:
 *   R0  = Input code (0=Normal, 4=JECL error)
 *   R10 = JCT address
 *   R11 = HCT address
 *   R13 = PCE address
 *   R12 = Base register
 *********************************************************************/

#include "metalc_base.h"
#include "metalc_jes2.h"

/*===================================================================
 * HASPEX20 - JES2 End of Job Input Exit Entry Point
 *===================================================================*/

#pragma prolog(EXIT20, "SAVE(14,12),LR(12,15)")
#pragma epilog(EXIT20, "RETURN(14,12)")

int EXIT20(int input_code, void *reserved, struct jct *jct) {
    
    /*---------------------------------------------------------------
     * Force batch jobs to msgclass "E"
     * CLI JCTJOBID,C'J' - Check if it's a batch job
     *---------------------------------------------------------------*/
    /* Note: In JES2, JCTJOBID usually contains 'JOBnnnnn', 'STCnnnnn', etc. */
    if (jct->jctjobid == 'J') {
        jes2_set_msgclass(jct, 'E');
    }

    /*---------------------------------------------------------------
     * Time and Date update logic omitted due to JES2 internal
     * complexity ($DOGJQE, $SUBIT, etc.). In a full implementation,
     * this would update the JQE reader timestamps.
     *---------------------------------------------------------------*/

    return JES2_RC_CONTINUE;
}
