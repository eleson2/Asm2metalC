# Verification Matrix — ICHPWX01 (RACF Password Quality Exit)

**Status:** Draft
**Date:** 2026-03-11
**Verifier:** (assign before In Review)

---

## 1. Module Identity

| Item | Value |
|------|-------|
| Module name | ICHPWX01 |
| ASM source | `asm/RACF/ICHPWX01.asm` |
| C source | `converted/RACF/ICHPWX01.c` |
| Product | RACF |
| Exit point | ICHPWX01 — Password Quality Exit |
| Header | `includes/metalc_racf.h` |
| Entry label | `ICHPWX01` |
| AMODE | 31 |
| Attributes | REENTRANT, RMODE ANY |

---

## 2. Entry Conditions — Register-to-Variable Mapping

| Register | ASM role | C variable | Type |
|----------|----------|-----------|------|
| R1 | Parameter list pointer (`struct racf_pwx_parm *`) | `parm` | `struct racf_pwx_parm *` |
| R3 | Caller flag pointer (loaded from `PWXCALLR`) | `caller_flag` | `uint8_t *` |
| R4 | New password pointer (from `PWXNEWPW`) | `new_pw` | `uint8_t *` |
| R5 | New password length | `new_len` | `uint8_t` |
| R6 | Old password pointer | `old_pw` | `uint8_t *` |
| R7 | Old password length | `old_len` | `uint8_t` |
| R12 | Base register | — | — |
| R15 | Return code | function return value | `int` |

---

## 3. Logic Equivalence Table

| ASM Label | ASM Instruction(s) | C Equivalent | Verified? | Concern # |
|-----------|--------------------|--------------|-----------|-----------|
| `X01ENTRY` | `USING *,R12` | `#pragma prolog(ICHPWX01,"SAVE(14,12),LR(12,15)")` | Yes | — |
| `X01A010` | `L R3,PWXCALLR` — load caller flag pointer | `caller_flag = (uint8_t *)parm->pwxcallr /* +28 */` | Yes | C1 (null guard) |
| `X01CALL` | `CLI 0(R3),PWXRINIT` / `BE X01A020` | `if (*caller_flag != PWXRINIT && *caller_flag != PWXPWORD)` | Yes | — |
| `X01CALL2` | `CLI 0(R3),PWXPWORD` / `BNE X01Z900` | (same combined condition) | Yes | — |
| `X01A020` | `ICM R0,B'1111',PWXNEWPW` / `BZ X01Z900` | `if (parm->pwxnpass == NULL) return RACF_PWXRC_ACCEPT;` | Yes | — |
| `X01A030` | `L R4,PWXNEWPW` / get length byte | `new_pw = (uint8_t *)parm->pwxnpass + 1; new_len = *parm->pwxnpass` | Yes | — |
| `X01A200` | `TRT 0(L,R4),ALPHATAB` — scan for non-alpha | `for(...) { if (!((c>='A'&&c<='I')\|\|(c>='J'&&c<='R')\|\|(c>='S'&&c<='Z')))` | Yes | C2 (TRT table vs. ranges) |
| `X01A210` | `BNZ X01A300` — branch if non-alpha found | `has_non_alpha = 1; break;` | Yes | — |
| `X01A220` | `LA R15,4` / `B X01Z100` — reject | `return RACF_PWXRC_FAIL; /* RC=4 */` | Yes | — |
| `X01A300` | `ICM R0,B'1111',PWXOLDPW` / `BZ X01A400` | `if (parm->pwxpass != NULL)` | Yes | — |
| `X01A310` | Inner loop: `CLC 0(4,R4),0(R6)` etc. | `match_field((char *)&new_pw[i],(char *)&old_pw[j],4)` | Yes | — |
| `X01A320` | `BE X01Z200` — reject: 4-char repeat | `return RACF_PWXRC_FAIL;` | Yes | — |
| `X01A400` | Call `TRIPCHR` routine | `pwd_has_triple_repeat((char *)new_pw, new_len)` | Yes | — |
| `X01A410` | `BNZ X01Z300` — reject: triple char | `return RACF_PWXRC_FAIL;` | Yes | — |
| `X01A500` | `ICM R0,B'1111',PWXUSRID` / scan for userid | `match_field(...)` inner loop over new_pw | Yes | C3 (null guard on userid) |
| `X01A510` | `BE X01Z400` — reject: contains userid | `return RACF_PWXRC_FAIL;` | Yes | — |
| `X01A600` | Table scan against `BADWORDS` | `pwd_in_table` / inline keyword scan | Yes | — |
| `X01A610` | `BE X01Z500` — reject: reserved word | `return RACF_PWXRC_FAIL;` | Yes | — |
| `X01Z900` | `SR R15,R15` / `BR R14` — accept | `return RACF_PWXRC_ACCEPT; /* RC=0 */` | Yes | — |
| `X01Z100`–`X01Z500` | `LA R15,4` / `BR R14` — reject paths | `return RACF_PWXRC_FAIL; /* RC=4 */` | Yes | — |

> **Note on missing Rule #1 and Rule #3:**
> The ASM source comment in the exit header mentions rules:
> - Rule #1: Minimum length of 6 characters
> - Rule #3: First and last character must not both be numeric
>
> **Rule #1** is handled by RACF itself before calling ICHPWX01; the exit
> receives only passwords that already pass RACF's minimum-length check.
> No implementation expected in the exit body.
>
> **Rule #3** is listed in the ASM header comment but is **not implemented
> in the ASM body** either.  This is a pre-existing gap in the original
> assembler code, not a conversion error.  See Concern C4.

---

## 4. Return Code Path Inventory

| Path | Condition | Expected RC | C matches ASM? |
|------|-----------|-------------|----------------|
| Accept — not our caller | `*pwxcallr` not RACINIT or PASSWORD | `RACF_PWXRC_ACCEPT` (0) | Yes |
| Accept — no new password | `pwxnpass == NULL` | `RACF_PWXRC_ACCEPT` (0) | Yes |
| Accept — null caller ptr | `pwxcallr == NULL` (C only) | `RACF_PWXRC_ACCEPT` (0) | N/A — C1 divergence |
| Accept — all rules pass | Normal exit | `RACF_PWXRC_ACCEPT` (0) | Yes |
| Fail — Rule #2 | All chars alphabetic | `RACF_PWXRC_FAIL` (4) | Yes |
| Fail — Rule #5 | 4 consecutive chars of old password found | `RACF_PWXRC_FAIL` (4) | Yes |
| Fail — Rule #6 | 3 identical adjacent chars | `RACF_PWXRC_FAIL` (4) | Yes |
| Fail — Rule #7 | Password contains user ID | `RACF_PWXRC_FAIL` (4) | Yes |
| Fail — Rule #8 | Password contains reserved keyword | `RACF_PWXRC_FAIL` (4) | Yes |

---

## 5. Flagged Concerns

**C1 — Null guard on pwxcallr added in C only**
> The ASM dereferences `PWXCALLR` directly without checking for zero.
> The C adds `if (caller_flag == NULL) return RACF_PWXRC_ACCEPT;`.
> In practice RACF always provides a valid caller flag; the null check
> is a defensive addition that cannot be reached by any documented caller.
> **Assessment:** Safe divergence.  Accepted.

**C2 — Rule #2 TRT translate table vs. EBCDIC range comparisons**
> The ASM uses a 256-byte `ALPHATAB` translate table where positions
> corresponding to alphabetic characters (EBCDIC A–Z, a–z) are zero;
> all other positions are non-zero.  `TRT` scans the password and sets
> the condition code based on whether any non-alpha byte was found.
>
> The C uses three EBCDIC range comparisons:
> ```c
> !((c >= 'A' && c <= 'I') || (c >= 'J' && c <= 'R') || (c >= 'S' && c <= 'Z'))
> ```
> These three ranges cover exactly the 26 uppercase EBCDIC alphabetic
> characters (A=0xC1–I=0xC9, J=0xD1–R=0xD9, S=0xE2–Z=0xE9).
>
> The ASM `ALPHATAB` in the original exit covers only uppercase letters
> (not a–z) because passwords on this system are uppercase-only.
> The C ranges also cover only uppercase.
>
> **Assessment:** Functionally equivalent for an uppercase-only password
> policy.  **Action required:** Confirm with security team that this
> installation enforces uppercase-only passwords.  If lowercase is
> permitted, both the ASM table and the C ranges would need updating.

**C3 — Null guard on pwxuser added in C only**
> The C checks `if (parm->pwxuser != NULL)` before Rule #7.
> The ASM tests `ICM R0,B'1111',PWXUSRID` which branches if the field
> is zero.  The C is semantically equivalent here — `ICM` followed by
> `BZ` is effectively a null check.
> **Assessment:** Equivalent.  No concern.

**C4 — Rule #3 ("first/last char non-numeric") not implemented**
> Listed in the ASM module header comment but not coded in the ASM body.
> The C conversion faithfully reproduces only what was actually coded.
> This is a **pre-existing gap** in the original assembler exit, not a
> conversion error.
> **Assessment:** Document gap.  Do not implement in C without explicit
> change-management approval; the gap exists in production assembly too.

**C5 — GETMAIN/FREEMAIN vs. stack variables**
> The ASM allocates a private work area via `$GETWORK` (a JES2/RACF
> convenience macro for GETMAIN).  The C uses stack (auto) variables.
> For Metal C exits running on the caller's TCB in 31-bit mode, stack
> allocation is safe and re-entrant.  No GETMAIN/FREEMAIN pair needed.
> **Assessment:** Deliberate simplification.  Correct for Metal C.

---

## 6. Sign-off Checklist

- [ ] All ASM labels mapped to C equivalents
- [ ] All return code paths confirmed match
- [ ] C2 code-page question resolved with security team
- [ ] C4 gap documented in change management system
- [ ] All other concerns reviewed and dispositioned
- [ ] Runtime tested via `test_ichpwx01.c` (future work)
- [ ] `verify_structs.c` compiles clean on target z/OS level
- [ ] Second reviewer sign-off: ___________________  Date: ________
