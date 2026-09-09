# Verification Matrix — DFSWHU00 (IMS Sign-on Exit with SAF)

**Status:** Draft
**Date:** 2026-09-09 (revised — RACROUTE stub replaced)
**Verifier:** (assign before In Review)

---

## 1. Module Identity

| Item | Value |
|------|-------|
| Module name | DFSWHU00 (entry: DFSWHU00) |
| ASM source | `asm/IMS/DFSWHU00.asm` |
| C source | `converted/IMS/DFSWHU00.c` |
| Product | IMS/TM (Transaction Manager) |
| Exit point | DFSWHU00 — Greeting/Sign-on Exit |
| Header | `includes/metalc_ims.h`, `includes/metalc_saf.h` |
| Companion stub | `asm/stubs/SAFAUTH.asm` (must be link-edited in) |
| Entry label | `DFSWHU00` |
| AMODE | 31 |
| Attributes | REENTRANT, RMODE ANY |

---

## 2. Entry Conditions — Register-to-Variable Mapping

| Register | ASM role | C variable | Type |
|----------|----------|-----------|------|
| R1 | Pointer to pointer list | `parmlist` | `void **` |
| R2 | User/group info pointer (from parmlist[0]) | `user_group_info` | `void **` |
| R3 | User ID (8 chars, from user_group_info[0]) | `userid` | `const char *` |
| R4 | Group name (8 chars, from user_group_info[1]) | `groupname` | `const char *` |
| R10 | RACROUTE work area | — (now owned by SAFAUTH stub) | — |
| R11 | STORAGE OBTAIN result | — (now owned by SAFAUTH stub) | — |
| R12 | Base register | — | — |
| R15 | Return code | function return value | `int` |

---

## 3. Logic Equivalence Table

| ASM Label | ASM Instruction(s) | C Equivalent | Verified? | Concern # |
|-----------|--------------------|--------------|-----------|-----------|
| Entry | `BAKR R14,0` / `LR R12,R15` | `#pragma prolog(DFSWHU00, "BAKR 14,0")` | Yes | — |
| Load parmlist | `L R2,0(,R1)` | `user_group_info = (void **)parmlist[0]` | Yes | — |
| Load userid | `L R3,0(,R2)` | `userid = (const char *)user_group_info[0]` | Yes | — |
| Load group | `L R4,4(,R2)` | `groupname = (const char *)user_group_info[1]` | Yes | C1 |
| Obtain work area | `STORAGE OBTAIN,LENGTH=512,ADDR=(R11),LOC=ANY` | none — obtained inside `SAFAUTH` | Yes | C3 |
| RACROUTE AUTH | `RACROUTE REQUEST=AUTH,CLASS='APPL',ENTITY=('IMSPROD'),USERID=(R3),ATTR=READ,MF=(E,RACLIST)` | `saf_auth_appl(IMS_APPLID, userid)` → `SAFAUTH` stub | Yes | C2 |
| Save SAF RC | `ST R15,SAF_RC` | `parm->saf_rc` in `struct saf_auth_parm` | Yes | — |
| Release work area | `STORAGE RELEASE,LENGTH=512,ADDR=(R11)` | none — released inside `SAFAUTH` | Yes | C3 |
| Check SAF RC | `LTR R15,R15` / `BZ ALLOW_USER` | `if (decision == SAF_ALLOWED)` | Yes | C4 |
| ALLOW path | `XR R15,R15` / `PR` | `return IMS_SGNX_ALLOW` (0) | Yes | — |
| REJECT path | `LA R15,8` / `PR` | `return IMS_SGNX_DEFER` (8) | Yes | — |
| Exit | `PR` | `#pragma epilog(DFSWHU00, "PR")` | Yes | — |

---

## 4. Scope Reduction — Functions Present in ASM but Absent in C

None. All ASM function is present; the RACROUTE call and its work-area
management have moved into the `SAFAUTH` assembler stub rather than
being dropped.

---

## 5. Return Code Path Inventory

| Path | Condition | Expected RC | C matches ASM? |
|------|-----------|-------------|----------------|
| Defer — null parameter list | `parmlist == NULL` or `parmlist[0] == NULL` | `IMS_SGNX_DEFER` (8) | Added — see C5 |
| Defer — null userid | `userid == NULL` | `IMS_SGNX_DEFER` (8) | Added — see C5 |
| Allow — SAF granted | `saf_rc == 0` | `IMS_SGNX_ALLOW` (0) | Yes |
| Defer — no SAF decision (RACF or class inactive) | `saf_rc == 4` | `IMS_SGNX_DEFER` (8) | Yes |
| Defer — not authorized | `saf_rc == 8` | `IMS_SGNX_DEFER` (8) | Yes |
| Defer — stub storage shortage | stub reports `saf_rc = 8` | `IMS_SGNX_DEFER` (8) | Improvement — see C3 |

---

## 6. Flagged Concerns

**C1 — `groupname` variable unused**
> The C code loads `groupname` from `user_group_info[1]` but does not use
> it.  Confirmed against the ASM: `L R4,4(,R2)` loads it and no later
> instruction references R4, and the RACROUTE codes `USERID=(R3)` with no
> group operand.  The C keeps the load, cast to `(void)`, so the
> parameter-list walk stays a faithful record of the layout.
> **Assessment:** LOW — closed. Matches the ASM exactly.

**C2 — RACROUTE now calls a real SAF stub, which is not yet assembled**
> RESOLVED IN PART. The previous conversion inlined
> `" XR 15,15 "` in place of the RACROUTE, so the exit allowed every
> sign-on regardless of RACF. That inline stub has been removed. The C
> now calls `saf_auth_appl()` (`includes/metalc_saf.h`), which invokes
> the `SAFAUTH` stub in `asm/stubs/SAFAUTH.asm` and applies default-deny:
> only SAF RC=0 allows; RC=4 (no decision) and RC=8 (denied) both reject.
>
> Residual risk: `SAFAUTH.asm` has never been assembled — there is no
> z/OS system in this repository. Its header lists the three items a
> systems programmer must confirm against the RACF Macro Reference for
> the installed level (RELEASE= value, WORKA size, and the register
> operand forms for `CLASS=`/`ENTITY=`/`USERID=`).
> **Assessment:** MEDIUM — down from HIGH. The fail-open behaviour is
> gone; a mis-coded macro operand now fails the assembly rather than
> silently allowing access. Production deployment stays blocked until
> the stub assembles clean and a live RACF test confirms all three RCs.

**C3 — Work area ownership moved into the stub**
> The ASM obtains a 512-byte `LOC=ANY` work area for `WORKA=` and
> releases it before `PR`, without checking the `STORAGE OBTAIN` return
> code. The work area is now obtained and released inside `SAFAUTH`,
> with `COND=YES`, so a storage shortage returns SAF RC=8 instead of
> abending the IMS sign-on.
> **Assessment:** LOW — deliberate improvement. Behaviour is unchanged
> on the success path; the failure path is strictly safer.

**C4 — ASM uses a non-reentrant static parameter list**
> The ASM declares `RACLIST RACROUTE REQUEST=AUTH,MF=L` in the CSECT and
> executes `MF=(E,RACLIST)` directly against it, writing runtime values
> into module storage. The module is marked reentrant but is not: two
> concurrent sign-ons corrupt each other's parameter list. `SAFAUTH`
> copies the `MF=L` model into obtained storage before the `MF=(E,...)`,
> so the C path is genuinely reentrant.
> **Assessment:** LOW — pre-existing ASM defect, fixed by the
> conversion. See `docs/reentrant-ification-policy.md`.

**C5 — Null guards added**
> The ASM dereferences the parameter list unconditionally. The C returns
> `IMS_SGNX_DEFER` (reject) if any pointer in the chain is null.
> **Assessment:** LOW — fail-safe direction, consistent with the
> default-deny rule.

---

## 7. Sign-off Checklist

- [ ] All in-scope ASM labels mapped to C equivalents
- [x] Inline `XR 15,15` RACROUTE placeholder removed from the C source
- [x] Three-way RC (0/4/8) handling implemented
- [x] Non-zero SAF RC defaults to deny (RC=4 no decision, RC=8 denied)
- [x] `groupname` usage verified against ASM RACROUTE call (C1 closed)
- [x] `BAKR`/`PR` linkage matches the ASM (was `SAVE`/`RETURN`)
- [ ] `asm/stubs/SAFAUTH.asm` assembles clean on the target z/OS level
- [ ] `SAFAUTH` macro operands reviewed against the RACF Macro Reference
      for the installed RACF release (see the stub header)
- [ ] `SAFAUTH` link-edited with `DFSWHU00`
- [ ] Runtime tested against real RACF: granted (0), RACF-inactive (4), denied (8)
- [ ] `verify_structs.c` compiles clean on target z/OS level
- [ ] **PRODUCTION DEPLOYMENT BLOCKED** until C2 residual risk is cleared
- [ ] Second reviewer sign-off: ___________________  Date: ________
