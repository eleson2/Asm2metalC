/*********************************************************************
 * verify_structs.c - Compile-time struct layout verification
 *
 * This file has NO executable body.  It verifies that every struct
 * used in converted exits has the correct size and field offsets as
 * documented in the corresponding z/OS DSECTs.
 *
 * A compile error of the form:
 *   error: size of array '_chk_jct_sz' is negative
 * means the named struct/field does not match the expected layout.
 *
 * Build (on z/OS):
 *   xlc -qmetal -S -qlist -I../includes verify_structs.c
 *
 * A clean compile (no errors) is the pass criterion.
 * No assembly or link-edit step is needed.
 *
 * Assertions derived from inline offset comments in the headers and
 * the Functional Equivalence Verification Plan (2026-03-11).
 *********************************************************************/

#include "metalc_base.h"
#include "metalc_jes2.h"
#include "metalc_racf.h"
#include "metalc_smf.h"
#include "metalc_verify.h"

/*===================================================================
 * JES2 control blocks
 *===================================================================*/

/* struct jct - Job Control Table */
VERIFY_SIZE  (jct, 164);
VERIFY_OFFSET(jct, jctjname,  6);
VERIFY_OFFSET(jct, jctjclas, 14);
VERIFY_OFFSET(jct, jctflg1,  27);
VERIFY_OFFSET(jct, jctpname, 32);
VERIFY_OFFSET(jct, jctacct,  52);
VERIFY_OFFSET(jct, jctlines, 152);

/* struct jqe - Job Queue Element */
VERIFY_SIZE  (jqe, 44);
VERIFY_OFFSET(jqe, jqetype,   6);
VERIFY_OFFSET(jqe, jqejname, 32);

/* struct pce - Processor Control Element */
VERIFY_SIZE  (pce, 276);          /* 4+1+1+2+4+4+4+256 = 276        */
VERIFY_OFFSET(pce, pcejct,   8);
VERIFY_OFFSET(pce, pcework, 20);

/*===================================================================
 * RACF control blocks
 *===================================================================*/

/* struct acee - Accessor Environment Element */
VERIFY_OFFSET(acee, aceeflg1,  7);
VERIFY_OFFSET(acee, aceeuser, 12);
VERIFY_OFFSET(acee, aceepass, 28);
VERIFY_OFFSET(acee, aceedefg, 40);

/* struct racf_pwx_parm - ICHPWX01 parameter list */
VERIFY_SIZE  (racf_pwx_parm, 44);
VERIFY_OFFSET(racf_pwx_parm, pwxuser,   4);
VERIFY_OFFSET(racf_pwx_parm, pwxpass,  12);
VERIFY_OFFSET(racf_pwx_parm, pwxnpass, 20);
VERIFY_OFFSET(racf_pwx_parm, pwxcallr, 28);
VERIFY_OFFSET(racf_pwx_parm, pwxmsgp,  36);

/*===================================================================
 * SMF control blocks
 *===================================================================*/

/* struct smf_header - Standard SMF record header */
VERIFY_SIZE  (smf_header, 18);
VERIFY_OFFSET(smf_header, smfrty, 5);
VERIFY_OFFSET(smf_header, smftme, 6);

/* struct smf_subtype_header - Header for records with subtypes */
VERIFY_SIZE  (smf_subtype_header, 24);
VERIFY_OFFSET(smf_subtype_header, smfssi,   18);
VERIFY_OFFSET(smf_subtype_header, smfsubty, 22);

/*===================================================================
 * Base control blocks
 *===================================================================*/

/* struct wto_parm - WTO parameter list */
VERIFY_OFFSET(wto_parm, wto_mcsflags, 2);
VERIFY_OFFSET(wto_parm, wto_text,     4);

/* end of verify_structs.c - no main(), compile-only */
