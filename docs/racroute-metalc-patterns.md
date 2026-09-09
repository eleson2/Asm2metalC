# RACROUTE → Metal C Patterns

This document covers translation of IBM SAF/RACF service calls (`RACROUTE` macro)
from HLASM to Metal C.  RACROUTE calls are the most common security pathway in z/OS
exits; getting them wrong produces a security bypass — the converted exit must either
call the real service or refuse access by default (never allow by default).

---

## 1. RACROUTE Background

`RACROUTE` is the IBM macro that invokes the System Authorization Facility (SAF).
SAF routes the request to the active security product (RACF, ACF2, Top Secret, etc.)
via SVC 119.

### 1.1 Two-form macro pattern

RACROUTE always appears in the two-form `MF=` pattern in reentrant code:

```asm
RACPARM  RACROUTE REQUEST=AUTH,MF=L    ← MF=L: static parameter list template (CSECT)
...
         RACROUTE REQUEST=AUTH,MF=(E,RACPARM),   ← MF=(E,...): fill dynamic fields,
                  ENTITY=dsname,                    then execute (SVC 119)
                  ATTR=READ,
                  ACEE=0,
                  RACF=aceeptr
```

The `MF=L` form declares a pre-formatted parameter block in static storage.
The `MF=(E,list)` form copies it to a local area, fills in runtime values, and
issues SVC 119.

In non-reentrant code, the simpler one-form version may appear:
```asm
         RACROUTE REQUEST=AUTH,ENTITY=dsname,ATTR=READ
```
This is equivalent but cannot be used in reentrant modules.

---

## 2. Metal C Translation Strategy

Metal C cannot use RACROUTE directly — it has no LE runtime and no macro assembler.
There are three strategies in increasing order of correctness:

| Strategy | Use when | Risk |
|----------|----------|------|
| A. Inline `__asm` SVC 119 | **Never** — banned, see §4 | High — a placeholder ships silently as an always-allow |
| B. Assembler stub called via `#pragma linkage` | Any RACROUTE type — the current standard | Low — assembler handles plist |
| C. SAF callable service (`IRRSIA00`) | Long-term; LE-free callable, documented | Low — IBM-supported interface |

**Strategy B is the standard.**  The REQUEST=AUTH stub is already built:
`asm/stubs/SAFAUTH.asm` with `includes/metalc_saf.h`.  A new REQUEST type
means a new stub next to it, not inline assembler in the exit.
Strategy C remains the preferred long-term path but requires the
`IRRSIA00` load module to be available.

---

## 3. Strategy B — Assembler Stub Pattern

> **Implemented.**  The REQUEST=AUTH stub now exists as
> `asm/stubs/SAFAUTH.asm`, with the C interface in
> `includes/metalc_saf.h`.  Use those rather than re-deriving from the
> sketch below; the sketch is kept because it explains the shape.
> The shipped stub differs from it in three ways that matter: it copies
> the `MF=L` model into obtained storage so it is genuinely reentrant,
> it branches to a per-`ATTR` expansion because `ATTR=` takes a keyword
> and not a register, and it carries `CLASS=` and `USERID=` operands
> that the sketch omits.

### 3.1 Stub source (HLASM) — illustrative sketch

Create one assembler stub per RACROUTE request type.  Place in `asm/stubs/`:

```asm
*--------------------------------------------------------------------*
* SAFAUTH  — SAF REQUEST=AUTH stub callable from Metal C             *
* Called with standard linkage: R1 -> SAFAUTH_PARM                   *
* Returns: R15 = SAF RC, R0 = RACF RC, R1 = RACF reason code        *
*--------------------------------------------------------------------*
SAFAUTH  CSECT
SAFAUTH  AMODE 31
SAFAUTH  RMODE ANY
         SAVE  (14,12)
         LR    R12,R15
         USING SAFAUTH,R12
         L     R2,0(,R1)          entity name ptr (8-char field addr)
         L     R3,4(,R1)          ACEE ptr (0 = use task ACEE)
         L     R4,8(,R1)          access intent (0=READ 4=UPDATE 8=CONTROL 16=ALTER)
         RACROUTE REQUEST=AUTH,                                         X
               WORKA=WORKAREA,                                         X
               MF=L
WORKAREA DS    CL512              work area for RACROUTE
         RACROUTE REQUEST=AUTH,                                         X
               MF=(E,WORKAREA),                                        X
               ENTITY=((R2)),                                          X
               ATTR=((R4)),                                            X
               ACEE=((R3)),                                            X
               RELEASE=2.6
         RETURN (14,12)
         END
```

**Note**: The exact `WORKA` size requirement and RELEASE level depend on your z/OS
version.  Check the RACF Callable Services manual for the current level.

### 3.2 C declaration — `includes/metalc_saf.h`

The shipped declaration.  Keep `struct saf_auth_parm` and the `SAFPARM`
DSECT in `SAFAUTH.asm` in step: a field added to one must be added to
the other at the same offset.

```c
#pragma pack(1)
struct saf_auth_parm {
    const char *entity;    /* +0  entity name, blank padded to the class
                            *     maximum length (8 for APPL)           */
    const char *class_nm;  /* +4  8-char class name, blank padded       */
    const char *userid;    /* +8  8-char userid, or NULL for task ACEE  */
    void       *acee;      /* +12 ACEE address, or NULL for task ACEE   */
    int32_t     attr;      /* +16 SAF_ATTR_READ/UPDATE/CONTROL/ALTER    */
    int32_t     saf_rc;    /* +20 OUT R15 - the decision                */
    int32_t     racf_rc;   /* +24 OUT R0  - diagnostic                  */
    int32_t     racf_rsn;  /* +28 OUT R1  - diagnostic                  */
};
#pragma pack()

#pragma linkage(saf_auth_call, OS)
extern int saf_auth_call(struct saf_auth_parm *parm);
```

### 3.3 Usage in Metal C exit

Call `saf_auth()` or `saf_auth_appl()`, not `saf_auth_call()` directly —
the wrappers apply the §5 three-way rule so an exit cannot accidentally
treat RC=8 as an allow:

```c
#include "metalc_saf.h"

/* Sign-on style check against the APPL class */
if (saf_auth_appl("IMSPROD ", userid) != SAF_ALLOWED) {
    return IMS_SGNX_DEFER;
}

/* General form, with the raw codes kept for an operator message */
struct saf_auth_parm detail;
if (saf_auth("FACILITY", "BPX.SUPERUSER   ", NULL,
             SAF_ATTR_READ, &detail) != SAF_ALLOWED) {
    if (detail.saf_rc == SAF_RC_NO_DECISION) {
        wto_security("EXIT001E No SAF decision - access denied", 39);
    }
    return RC_ERROR;
}
```

Link edit must include the stub:

```
xlc -qmetal -S -qlist -I./includes myexit.c
as  -o myexit.o  myexit.s
as  -o SAFAUTH.o asm/stubs/SAFAUTH.asm
ld  -o MYEXIT myexit.o SAFAUTH.o
```

---

## 4. Strategy A — Inline `__asm` SVC 119

**Do not use.**  Kept only to document why it is banned.

Inline assembler is not permitted in a converted exit at all — see the
"No inline assembler in exit source" rule in `CLAUDE.md`.  Strategy A is
what produced the DFSWHU00 fail-open (§8): a placeholder that was easy to
write, easy to miss in review, and indistinguishable at a glance from a
working call.  A stub that does not exist yet fails the link edit; an
inline placeholder ships.

```c
/*
 * Minimal SAF REQUEST=VERIFY via SVC 119.
 * This is NOT a complete RACROUTE implementation — it omits ACEE handling,
 * work area, and many required fields.  Use the assembler stub (Strategy B)
 * for production code.
 */
static int saf_verify_minimal(const char *userid8) {
    int32_t rc = 8;   /* default deny */

    /*
     * SVC 119 with R1 pointing to a RACROUTE plist is complex.
     * This placeholder documents the intent and forces a fail-safe default.
     * Replace with assembler stub before production deployment.
     */
    (void)userid8;

    /* CONCERN: SVC 119 not invoked — always returns deny until stub is added */
    return rc;
}
```

---

## 5. Three-Way Return Code Interpretation

Every RACROUTE call returns three codes.  The Metal C caller must handle all three:

| R15 (SAF RC) | C constant | Meaning | Default action |
|---|---|---|---|
| 0 | `SAF_RC_GRANTED` | Authorized | Allow |
| 4 | `SAF_RC_NO_DECISION` | SAF/RACF made no decision — RACF not active, class not active, or no profile covers the resource | **Deny** (fail-safe) |
| 8 | `SAF_RC_DENIED` | Not authorized | Deny |

> **Corrected 2026-09-09.**  This table previously read 4 as "RACF denied"
> and 8 as "SAF bypassed", which is the reverse of the SAF router's
> documented codes and of `SAF_RC_RACF_NOT_ACTIVE`/`SAF_RC_FAILED` in
> `includes/metalc_racf.h`.  No shipped code changed meaning: only RC=0
> has ever allowed, so both 4 and 8 denied before and deny now.  Confirm
> against the RACF Macro Reference for your release when the SAFAUTH stub
> is validated.

> **Critical rule**: RC=4 must **deny**, not allow.  "No decision" is the
> code you get when RACF is deactivated for maintenance or the class is
> not active — an exit that treats it as allow opens a hole exactly when
> security is weakest.

RACF itself sets R0 (RACF RC) and R1 (reason code) for diagnostic purposes.
The primary decision is always made on R15.

---

## 6. RACROUTE REQUEST Types

### REQUEST=AUTH

Used in most security exits (ICHPWX01, DFSWHU00 IMS sign-on, IEFDB401 allocation).

Key fields:
- `ENTITY` — resource name (class + profile, or just 8-char name)
- `ATTR` — access intent: `READ`(0), `UPDATE`(4), `CONTROL`(8), `ALTER`(16)
- `ACEE` — address of Accessor Environment Element, or 0 for current task
- `CLASS` — resource class name (default = DATASET)

### REQUEST=VERIFY

Used in authentication exits (ICHPWX01 password change verification).

Key fields:
- `USERID` — 8-char userid being authenticated
- `PASSWRD` — current or new password (encrypted in SAF plist)
- `NEWPASS` — new password (for REQUEST=VERIFY,NEWPASS)
- `ACEE` — returned ACEE pointer after successful verify

### REQUEST=FASTAUTH

Faster in-storage check; skips external routing to RACF.

Key fields same as REQUEST=AUTH.  Used when performance is critical and the
RACLIST has been pre-built.

---

## 7. RACROUTE in the Verification Matrix

When a converted exit uses a RACROUTE stub or inline SVC:

1. Add a **concern** in §6 of the verification matrix:
   ```
   Cx — RACROUTE stub: always returns <value>
   Assessment: HIGH — replace stub with real RACROUTE before production
   ```
2. Add the exit to the **Production Deployment Blocks** table in
   `docs/verification-matrices/README.md` with concern type `RACROUTE-STUB`.
3. The sign-off checklist must include:
   ```
   - [ ] RACROUTE stub replaced with assembler stub (Strategy B)
   - [ ] Three-way RC (0/4/8) handling verified
   - [ ] RC=8 (SAF bypass) defaults to deny
   ```

---

## 8. RACROUTE in DFSWHU00 (IMS Sign-On Exit) — resolved

`converted/IMS/DFSWHU00.c` used to contain:

```c
/* STUB: RACROUTE REQUEST=AUTH not implemented */
__asm(" XR 15,15");   /* always RC=0 = allow */
```

That was the HIGH severity concern in the DFSWHU00 verification matrix:
the exit allowed every sign-on regardless of what RACF decided.  It has
been replaced with the Strategy B call:

```c
#include "metalc_saf.h"

static const char IMS_APPLID[8] = { 'I','M','S','P','R','O','D',' ' };

decision = saf_auth_appl(IMS_APPLID, userid);
if (decision != SAF_ALLOWED) {
    return IMS_SGNX_DEFER;   /* RC=8 */
}
return IMS_SGNX_ALLOW;       /* RC=0 */
```

`saf_auth_appl()` applies the three-way rule from §5 for the caller, so
an exit cannot accidentally treat RC=8 as an allow.  The residual work
is assembling and validating `asm/stubs/SAFAUTH.asm` on the target
system — see the review list in that file's header.

## 9. Related Documents

- `includes/metalc_saf.h` — the C interface: `saf_auth()`, `saf_auth_appl()`
- `asm/stubs/SAFAUTH.asm` — the REQUEST=AUTH stub, with its pre-assembly review list
- `docs/asm-linkage-conventions.md` — BAKR/PR vs SAVE/RETURN, non-reentrant static data
- `docs/asm-to-metalc-ims.md` — IMS DFSWHU00 RACROUTE context
- `docs/verification-matrices/DFSWHU00_ims.md` — existing HIGH concern
- `docs/verification-matrices/ICHPWX01_racf.md` — RACF password exit matrix
