/*********************************************************************
 * MODULE:    DFSWHU00
 * FUNCTION:  IMS Greeting/Sign-on Exit calling SAF
 *
 * ASM source:  asm/IMS/DFSWHU00.asm
 * Product:     IMS/TM
 * Exit point:  DFSWHU00 - Sign-on Exit
 *
 * Called by IMS during sign-on.  Performs a RACROUTE REQUEST=AUTH
 * check against the APPL class for entity IMSPROD.
 *
 * Register mapping at entry:
 *   R1  = parmlist   (void **)  - address of a list of pointers
 *   R2  = user/group info list  - parmlist[0]
 *   R3  = userid     (char *)   - user_group_info[0], 8 bytes
 *   R4  = groupname  (char *)   - user_group_info[1], 8 bytes
 *   R12 = base register (established by prolog)
 *   R15 = return code (set by epilog)
 *
 * Verification matrix: docs/verification-matrices/DFSWHU00_ims.md
 *
 * Return: RC=0 (IMS_SGNX_ALLOW) - Allow sign-on
 *         RC=8 (IMS_SGNX_DEFER) - Reject sign-on
 *
 * Build: xlc -qmetal -S -qlist -I./includes converted/IMS/DFSWHU00.c
 *        The SAFAUTH stub must be link-edited with this module:
 *          as -o SAFAUTH.o asm/stubs/SAFAUTH.asm
 *          ld -o DFSWHU00 DFSWHU00.o SAFAUTH.o
 *
 * Attributes: REENTRANT, AMODE 31, RMODE ANY
 *
 * DIVERGENCES FROM THE ASM SOURCE
 *   1. RACROUTE.  The ASM issues RACROUTE REQUEST=AUTH inline against
 *      a static MF=L list (RACLIST), which is not reentrant - two
 *      concurrent sign-ons overwrite each other's parameter list.
 *      The C version calls saf_auth_appl(), backed by the reentrant
 *      SAFAUTH stub.  A previous conversion of this module inlined
 *      "XR 15,15" in place of the RACROUTE, which made every sign-on
 *      succeed regardless of RACF; that stub is gone.
 *   2. Work area.  The ASM does STORAGE OBTAIN LENGTH=512 for the
 *      RACROUTE WORKA and releases it before PR.  The work area is
 *      now owned by the SAFAUTH stub, so this module obtains no
 *      storage at all.  Net behaviour is unchanged.
 *   3. Storage failure.  The ASM does not check the STORAGE OBTAIN
 *      return code.  The stub reports a storage shortage as SAF RC=8,
 *      which reaches the default-deny path here.
 *   4. Group name.  Loaded by the ASM into R4 but never referenced.
 *      Kept and marked unused so the parameter-list walk stays a
 *      faithful record of the layout.
 *   5. Null guards on the parameter list are added; the ASM
 *      dereferences unconditionally.
 *********************************************************************/

#include "metalc_base.h"
#include "metalc_ims.h"
#include "metalc_saf.h"

/* ASM: ENTITY=('IMSPROD') - blank padded to the APPL class length */
static const char IMS_APPLID[8] = { 'I','M','S','P','R','O','D',' ' };

/*===================================================================
 * DFSWHU00 - IMS Greeting/Sign-on Exit Entry Point
 *===================================================================*/

/* ASM: BAKR R14,0 ... PR  (linkage stack, not a savearea chain).
 * See docs/asm-linkage-conventions.md.                             */
#pragma prolog(DFSWHU00, "BAKR 14,0")
#pragma epilog(DFSWHU00, "PR")

int DFSWHU00(void **parmlist) {
    void **user_group_info;
    const char *userid;
    const char *groupname;
    int decision;

    /* ASM: L R2,0(,R1) - R1 points to a list of pointers */
    if (parmlist == NULL) {
        return IMS_SGNX_DEFER; /* RC=8 */
    }
    user_group_info = (void **)parmlist[0];
    if (user_group_info == NULL) {
        return IMS_SGNX_DEFER; /* RC=8 */
    }

    /* ASM: L R3,0(,R2) - R3 = address of user id (8 bytes)    */
    userid = (const char *)user_group_info[0];

    /* ASM: L R4,4(,R2) - R4 = address of group name (8 bytes).
     * Loaded by the ASM but never used.                       */
    groupname = (const char *)user_group_info[1];
    (void)groupname;

    if (userid == NULL) {
        return IMS_SGNX_DEFER; /* RC=8 */
    }

    /*---------------------------------------------------------------
     * ASM: RACROUTE REQUEST=AUTH,CLASS='APPL',ENTITY=('IMSPROD'),
     *      USERID=(R3),WORKA=(R10),ATTR=READ,MF=(E,RACLIST)
     *
     * saf_auth_appl() applies the default-deny rule: only SAF RC=0
     * allows.  RC=4 (no SAF decision -- RACF or the APPL class is
     * inactive, or no profile covers IMSPROD) and RC=8 (not authorized,
     * or the stub could not obtain storage) both reject.
     *---------------------------------------------------------------*/
    decision = saf_auth_appl(IMS_APPLID, userid);

    /* ASM: LTR R15,R15 / BZ ALLOW_USER */
    if (decision == SAF_ALLOWED) {
        return IMS_SGNX_ALLOW; /* ASM: ALLOW_USER - XR R15,R15 - RC=0 */
    }

    /* ASM: LA R15,8 - RC=8: reject the sign-on */
    return IMS_SGNX_DEFER;
}
