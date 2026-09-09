/*********************************************************************
 * METALC_SAF.H - SAF / RACROUTE service layer for Metal C exits
 *
 * Metal C cannot expand the RACROUTE macro, and hand-building the SAF
 * parameter list with inline SVC 119 is the pattern that produced the
 * "XR 15,15 = always allow" security bypass in the first round of
 * conversions.  This header exposes SAF as ordinary C functions
 * backed by the assembler stub in asm/stubs/SAFAUTH.asm.
 *
 * Usage:
 *   #include "metalc_base.h"
 *   #include "metalc_<product>.h"
 *   #include "metalc_saf.h"        <- only when the exit calls SAF
 *
 * Link edit must include the SAFAUTH stub:
 *   xlc -qmetal -S -qlist -I./includes myexit.c
 *   as  -o myexit.o  myexit.s
 *   as  -o SAFAUTH.o asm/stubs/SAFAUTH.asm
 *   ld  -o MYEXIT myexit.o SAFAUTH.o
 *
 * This header contains no assembler.  All inline assembler in the
 * framework lives in metalc_svc.h; all standalone assembler lives in
 * asm/stubs/.
 *
 * See docs/racroute-metalc-patterns.md (Strategy B).
 *********************************************************************/

#ifndef METALC_SAF_H
#define METALC_SAF_H

#ifndef METALC_BASE_H
#include "metalc_base.h"
#endif

/*-------------------------------------------------------------------
 * SAF return codes (R15 from RACROUTE)
 *
 * These are the SAF router's codes, not RACF's.  The decision is
 * always made on this value; the RACF return and reason codes are
 * diagnostic only.
 *-------------------------------------------------------------------*/

#define SAF_RC_GRANTED         0    /* Authorized                    */
#define SAF_RC_NO_DECISION     4    /* SAF/RACF made no decision:
                                     * RACF not active, class not
                                     * active, or no profile found    */
#define SAF_RC_DENIED          8    /* Not authorized                 */

/* These match the SAF return codes documented in includes/metalc_racf.h
 * (SAF_RC_OK / SAF_RC_RACF_NOT_ACTIVE / SAF_RC_FAILED).  Note that
 * docs/racroute-metalc-patterns.md carried 4 and 8 the other way round
 * until 2026-09-09; the values themselves never changed, and the deny
 * rule below is unaffected because only RC=0 has ever allowed.        */

/*-------------------------------------------------------------------
 * Access intent (ATTR= on RACROUTE REQUEST=AUTH)
 *-------------------------------------------------------------------*/

#define SAF_ATTR_READ          0
#define SAF_ATTR_UPDATE        4
#define SAF_ATTR_CONTROL       8
#define SAF_ATTR_ALTER        16

/*-------------------------------------------------------------------
 * Decision values returned by saf_auth()
 *-------------------------------------------------------------------*/

#define SAF_ALLOWED            0    /* Access is authorised          */
#define SAF_DENIED             1    /* Access is not authorised      */

/* Fixed field widths expected by RACROUTE */
#define SAF_CLASS_LEN          8    /* Class name, blank padded      */
#define SAF_USERID_LEN         8    /* Userid, blank padded          */

/*-------------------------------------------------------------------
 * SAF REQUEST=AUTH parameter block
 *
 * Mapped by SAFPARM DSECT in asm/stubs/SAFAUTH.asm.  Keep the two in
 * step: any field added here must be added there at the same offset.
 *-------------------------------------------------------------------*/

#pragma pack(1)
struct saf_auth_parm {
    const char *entity;    /* +0  entity name, blank padded to the
                            *     class maximum length (8 for APPL)  */
    const char *class_nm;  /* +4  8-char class name, blank padded    */
    const char *userid;    /* +8  8-char userid, or NULL for the
                            *     current task's ACEE                */
    void       *acee;      /* +12 ACEE address, or NULL for the
                            *     current task's ACEE                */
    int32_t     attr;      /* +16 access intent, SAF_ATTR_*          */
    int32_t     saf_rc;    /* +20 OUT R15 - the decision             */
    int32_t     racf_rc;   /* +24 OUT R0  - diagnostic               */
    int32_t     racf_rsn;  /* +28 OUT R1  - diagnostic               */
};
#pragma pack()

/*-------------------------------------------------------------------
 * Assembler stub linkage
 *-------------------------------------------------------------------*/

#pragma linkage(saf_auth_call, OS)

/**
 * saf_auth_call - RACROUTE REQUEST=AUTH (asm/stubs/SAFAUTH.asm)
 * @parm: parameter block; output fields are filled on return
 *
 * Returns: the SAF return code, also stored in parm->saf_rc.
 *
 * Prefer saf_auth() below, which applies the default-deny rule.
 */
extern int saf_auth_call(struct saf_auth_parm *parm);

/*-------------------------------------------------------------------
 * Recommended C interface
 *-------------------------------------------------------------------*/

/**
 * saf_auth - Authorisation check with fail-safe interpretation
 * @class_nm: 8-char class name, blank padded (e.g. "APPL    ")
 * @entity:   entity name, blank padded to the class maximum length
 * @userid:   8-char userid, or NULL to use the current task's ACEE
 * @attr:     access intent, one of the SAF_ATTR_* values
 * @detail:   optional; receives the raw SAF/RACF codes, may be NULL
 *
 * Returns: SAF_ALLOWED or SAF_DENIED.
 *
 * DEFAULT-DENY RULE (docs/racroute-metalc-patterns.md section 5):
 * only SAF_RC_GRANTED (0) allows.  SAF_RC_NO_DECISION (4) means RACF
 * could not decide -- typically RACF or the class is inactive, or no
 * profile covers the resource -- and must deny, otherwise deactivating
 * RACF for maintenance silently opens every exit that calls this.
 * SAF_RC_DENIED (8) obviously denies.  Anything the stub could not
 * obtain arrives as 8 and denies.
 *
 * A caller that genuinely needs to distinguish "denied" from "no
 * decision" -- for instance to issue different operator messages --
 * reads detail->saf_rc.  The allow/deny decision stays here.
 */
static inline int saf_auth(const char *class_nm,
                           const char *entity,
                           const char *userid,
                           int32_t attr,
                           struct saf_auth_parm *detail) {
    struct saf_auth_parm local;
    struct saf_auth_parm *p = (detail != NULL) ? detail : &local;

    p->entity   = entity;
    p->class_nm = class_nm;
    p->userid   = userid;
    p->acee     = NULL;
    p->attr     = attr;
    p->saf_rc   = SAF_RC_DENIED;     /* fail-safe if the call is
                                      * somehow not made at all     */
    p->racf_rc  = 0;
    p->racf_rsn = 0;

    saf_auth_call(p);

    return (p->saf_rc == SAF_RC_GRANTED) ? SAF_ALLOWED : SAF_DENIED;
}

/**
 * saf_auth_appl - Authorisation check against the APPL class
 * @applid: 8-char application id, blank padded (e.g. "IMSPROD ")
 * @userid: 8-char userid, or NULL to use the current task's ACEE
 *
 * Returns: SAF_ALLOWED or SAF_DENIED.
 *
 * Convenience wrapper for the common sign-on pattern
 *   RACROUTE REQUEST=AUTH,CLASS='APPL',ENTITY=(applid),ATTR=READ
 * used by IMS, CICS and TSO sign-on exits.
 */
static inline int saf_auth_appl(const char *applid, const char *userid) {
    return saf_auth("APPL    ", applid, userid, SAF_ATTR_READ, NULL);
}

#endif /* METALC_SAF_H */
