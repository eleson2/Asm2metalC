# Verification Matrix — HASPEX02 / EXIT02 (JES2 Job Statement Scan)

**Status:** Draft
**Date:** 2026-03-11
**Verifier:** (assign before In Review)

---

## 1. Module Identity

| Item | Value |
|------|-------|
| Module name | HASPEX02 (entry: EXIT02) |
| ASM source | `asm/JES2/HASPEX02.asm` |
| C source | `converted/JES2/HASPEX02.c` |
| Product | JES2 |
| Exit point | Exit 2 — JOB Statement Scan |
| Header | `includes/metalc_jes2.h` |
| Entry label | `EXIT02` |
| AMODE | 31 |
| Attributes | REENTRANT, RMODE ANY |

---

## 2. Entry Conditions — Register-to-Variable Mapping

| Register | ASM role | C variable | Type |
|----------|----------|-----------|------|
| R0 | Statement type (0=initial, 4=continuation) | `statement_type` | `int` |
| R1 | Parameter list pointer (3-word list) | `parmlist` | `void **` |
| R10 | JCT address | `jct` | `struct jct *` |
| R11 | HCT address | (unused in this exit) | — |
| R13 | PCE address | (implicit; not mapped to variable) | — |
| R12 | Base register | — | — |
| R15 | Return code | function return value | `int` |

---

## 3. Logic Equivalence Table

| ASM Label | ASM Instruction(s) | C Equivalent | Verified? | Concern # |
|-----------|--------------------|--------------|-----------|-----------|
| `EX02ENTR` | `USING *,R12` | `#pragma prolog(EXIT02,"SAVE(14,12),LR(12,15)")` | Yes | — |
| `EX02GETW` | `$GETWORK` (GETMAIN equivalent) | `getmain(sizeof(struct exit02w), SUBPOOL_JOB_STEP)` | Yes | — |
| `EX02CLRW` | `XC WORKAREA,WORKAREA` | `memset_inline(work, 0, sizeof(struct exit02w))` | Yes | — |
| `EX02BUF` | `L R2,0(,R1)` — load buffer ptr from parm[0] | `buffer = (char *)parmlist[0]` | Yes | — |
| `EX02TYPE` | `LTR R0,R0` / `BNZ EX02DONE` — skip if continuation | `if (statement_type != 0) goto CLEANUP;` | Yes | — |
| `EX02SCAN` | Loop: `CLC 0(7,R4),=C',CLASS='` / `BE EX02FND` | `match_prefix(&buffer[i],",CLASS=",7)` | Yes | — |
| `EX02SCAN2` | `CLC 0(7,R4),=C' CLASS='` / `BE EX02FND` | `match_prefix(&buffer[i]," CLASS=",7)` | Yes | — |
| `EX02FND` | `LA R5,7(,R4)` — point past "CLASS=" | `class_ptr = &buffer[i + 7]` | Yes | — |
| `EX02CHKC` | `CLI 1(R5),C' '` / `BE EX02DONE` | `if (class_ptr[1] == ' ' \|\| class_ptr[1] == ',') goto CLEANUP;` | Yes | C1 (off-by-one check) |
| `EX02LEN` | Inner loop measuring symbolic class length | `while (sym_len < 8 && class_ptr[sym_len] != ' ' && ...)` | Yes | — |
| `EX02COPY` | `MVC WORKJCLS,0(R5)` — copy symbolic class | `memcpy_inline(work->jobclass, class_ptr, sym_len)` | Yes | — |
| `EX02PAD` | `MVI ...,' '` padding remainder | `pad_blank(work->jobclass, sym_len, 8)` | Yes | — |
| `EX02MSG` | Build WTO message in work area | Build `work->msgident`, `work->msgjnam`, `work->msgsymj` | Yes | C2 (jctid vs. jctjobid) |
| `EX02WTO` | `WTO` — issue message | `wto_simple(...)` | Yes | — |
| `EX02DONE` | `$RETWORK` (FREEMAIN equivalent) | `freemain(work, sizeof(struct exit02w), SUBPOOL_JOB_STEP)` | Yes | — |
| `EX02RET` | `SR R15,R15` / `BR R14` | `return JES2_RC_CONTINUE; /* RC=0 */` | Yes | — |

---

## 4. Scope Reduction — Functions Present in ASM but Absent in C

> **IMPORTANT: The C conversion is a partial scope conversion.**
> The following ASM functions were explicitly excluded from the C
> conversion.  This is intentional, not an oversight.

| ASM Function | Description | Reason Excluded |
|-------------|-------------|-----------------|
| Function 1 — SWBTJCT extension | Initialises the JES2 SWBT (Scheduler Work Block) extension block fields when a new JCT is allocated | Out of scope for initial port; requires SWBT DSECT not yet mapped in headers |
| Function 3 — MSGCLASS= scan | Scans the JOB statement for the `MSGCLASS=` keyword and extracts the message class | Out of scope; only CLASS= was prioritised for initial delivery |

> These functions are documented here to ensure they are not mistakenly
> assumed to be present.  They must be implemented in a follow-on change
> before this exit can fully replace the ASM version.

---

## 5. Return Code Path Inventory

| Path | Condition | Expected RC | C matches ASM? |
|------|-----------|-------------|----------------|
| Continue — getmain failed | `getmain` returns NULL | `JES2_RC_CONTINUE` (0) | Yes (safe default) |
| Continue — continuation card | `statement_type != 0` | `JES2_RC_CONTINUE` (0) | Yes |
| Continue — no CLASS= found | Loop exhausts 64 chars | `JES2_RC_CONTINUE` (0) | Yes |
| Continue — simple 1-char class | `class_ptr[1] == ' '` or `','` | `JES2_RC_CONTINUE` (0) | Yes — see C1 |
| Continue — symbolic class found | Extracts class, issues WTO, returns | `JES2_RC_CONTINUE` (0) | Yes |

---

## 6. Flagged Concerns

**C1 — `class_ptr[1]` index vs. ASM `CLI 1(R5),C' '`**
> After locating `CLASS=` and pointing `class_ptr` past the `=`, the
> ASM checks byte `1(R5)` — the *second* character of the class value.
> The C checks `class_ptr[1]`.  These are the same byte.
>
> The intent is: a single-character class value is followed by a
> delimiter at position [1].  If the delimiter is there, skip (it is
> a simple 1-char class, handled by JES2 directly).  If not, extract
> the symbolic name.
>
> **Assessment:** `class_ptr[1]` is equivalent to ASM `CLI 1(R5),X`.
> No off-by-one error.  Verified.

**C2 — WTO message uses `jct->jctid` instead of job ID**
> The ASM builds the WTO message using the JES2 job ID field from the
> JCT (typically `JCTJOBID`, a 2-byte binary field that the ASM formats
> into EBCDIC).  The C copies `jct->jctid` (the 4-byte eye-catcher
> field `"JCT "`) into the message's jobid slot.
>
> This produces an incorrect WTO message (shows `"JCT "` instead of
> a job number like `"JOB00123"`).  The functional behaviour of the
> exit (return code, class extraction) is unaffected.
>
> **Assessment:** Message content bug.  Low severity (informational WTO
> only).  Should be corrected in a follow-on change.

**C3 — SWBTJCT and MSGCLASS= functions absent**
> See Section 4 above.  These functions exist in ASM and are absent in
> the C port.  The exit **cannot fully replace the ASM module** until
> they are implemented.
> **Assessment:** Open work item.  Track in change management.

---

## 7. Sign-off Checklist

- [ ] All in-scope ASM labels mapped to C equivalents
- [ ] Scope reduction documented and accepted by JES2 owner
- [ ] C1 off-by-one confirmed equivalent
- [ ] C2 WTO message bug raised as defect
- [ ] C3 tracked in change management
- [ ] `verify_structs.c` compiles clean on target z/OS level
- [ ] Runtime tested via `test_haspex02.c` (future work)
- [ ] Second reviewer sign-off: ___________________  Date: ________
