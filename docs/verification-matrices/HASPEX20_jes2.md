# Verification Matrix — HASPEX20 / EXIT20 (JES2 End of Job Input)

**Status:** Draft
**Date:** 2026-03-14
**Verifier:** (assign before In Review)

---

## 1. Module Identity

| Item | Value |
|------|-------|
| Module name | HASPEX20 (entry: EXIT20) |
| ASM source | `asm/JES2/HASPEX20.asm` |
| C source | `converted/JES2/HASPEX20.c` |
| Product | JES2 |
| Exit point | Exit 20 — End of Job Input |
| Header | `includes/metalc_jes2.h` |
| Entry label | `EXIT20` |
| AMODE | 31 |
| Attributes | REENTRANT, RMODE ANY |

---

## 2. Entry Conditions — Register-to-Variable Mapping

| Register | ASM role | C variable | Type |
|----------|----------|-----------|------|
| R0 | Input code (0=Normal, 4=JECL error) | `input_code` | `int` |
| R10 | JCT address | `jct` | `struct jct *` |
| R11 | HCT address | (unused in this exit) | — |
| R13 | PCE address | (implicit) | — |
| R12 | Base register | — | — |
| R15 | Return code | function return value | `int` |

---

## 3. Logic Equivalence Table

| ASM Label | ASM Instruction(s) | C Equivalent | Verified? | Concern # |
|-----------|--------------------|--------------|-----------|-----------|
| Entry | `$ENTRY BASE=R12` / `$SAVE` | `#pragma prolog(EXIT20, "SAVE(14,12),LR(12,15)")` | Yes | — |
| Check batch job | `CLI JCTJOBID,C'J'` | `if (jct->jctjobid == 'J')` | Yes | C1 |
| Set msgclass E | `MVI JCTMCLAS,C'E'` | `jes2_set_msgclass(jct, 'E')` | Yes | C2 |
| Timestamp logic | `$DOGJQE` / `$SUBIT` service calls | Omitted — see Section 4 | — | C3 |
| Return | `$RETURN RC=0` | `return JES2_RC_CONTINUE` | Yes | — |

---

## 4. Scope Reduction — Functions Present in ASM but Absent in C

> **IMPORTANT: The C conversion is a partial scope conversion.**

| ASM Function | Description | Reason Excluded |
|-------------|-------------|-----------------|
| JQE timestamp update | Updates JQE reader timestamps using `$DOGJQE` and `$SUBIT` JES2 internal service calls | JES2 internal services (`$DOGJQE`, `$SUBIT`) not available as Metal C bindings; requires custom assembler stubs |

---

## 5. Return Code Path Inventory

| Path | Condition | Expected RC | C matches ASM? |
|------|-----------|-------------|----------------|
| Continue — batch job | `jctjobid == 'J'`, msgclass set to 'E' | `JES2_RC_CONTINUE` (0) | Yes |
| Continue — non-batch | `jctjobid != 'J'`, no modification | `JES2_RC_CONTINUE` (0) | Yes |

---

## 6. Flagged Concerns

**C1 — `jctjobid` is declared 2 bytes; the assembler reads 8 — HIGH**
> The ASM `CLI JCTJOBID,C'J'` checks the first byte of the JES2 job ID
> against `'J'` (batch jobs whose IDs begin with `JOB`).  In the C struct
> `jct->jctjobid` is `uint16_t`, so `jct->jctjobid == 'J'` compares all
> 16 bits against `0x00D1`.  **That is true only for job number 209, so
> the exit never forces batch jobs to msgclass `E` — its only function
> silently does not happen.**
>
> This is a layout defect, not a comparison defect.  `HASPEX02.asm:135`
> has `MVC MSGJOBID,JCTJOBID` where `MSGJOBID DS CL8`, which reads 8
> bytes from the field.  With `CLI` proving byte 0 is character data,
> `JCTJOBID` is `CL8` — so `jctjname` belongs at +12, not +6, and every
> later JCT field moves by 6.
>
> **Do not** fix this as `*(char *)&jct->jctjobid == 'J'`.  That makes
> the comparison correct while leaving the field 2 bytes wide and every
> following offset wrong.
>
> **Assessment:** Open — HIGH.  Fix `metalc_jes2.h`, not the exit.
> The assembler is the specification and it reads the field 8 bytes
> wide, so the correction needs no further confirmation.  Note
> separately that the rest of the JCT struct has no provenance in this
> repository — those fields are reached symbolically, never by
> displacement — so a full mapping still needs the `$JCT` macro.  See
> `docs/layout-findings.md` finding 5 and `docs/asm-field-evidence.md`
> §5.

**C2 — `jes2_set_msgclass` function**
> The C calls `jes2_set_msgclass(jct, 'E')` which is declared in
> `metalc_jes2.h`.  Verify that this function correctly sets the JCT
> message class field (`JCTMCLAS`) to `'E'` and does not corrupt
> adjacent fields.
> **Assessment:** Open — verify `jes2_set_msgclass` implementation.

**C3 — Timestamp update absent**
> The JQE reader timestamp update (`$DOGJQE`/`$SUBIT`) is absent.
> See Section 4.  This means the JES2 JQE timestamps are not updated
> by the C exit.  Functional impact depends on whether downstream JES2
> processing relies on these timestamps.
> **Assessment:** Open work item.  Track in change management.

---

## 7. Sign-off Checklist

- [ ] All in-scope ASM labels mapped to C equivalents
- [ ] Scope reduction (timestamp update) documented and accepted by JES2 owner
- [ ] C1 jctjobid layout defect fixed in metalc_jes2.h (HIGH, see finding 5)
- [ ] C2 `jes2_set_msgclass` implementation verified
- [ ] C3 timestamp update tracked in change management
- [ ] `verify_structs.c` compiles clean on target z/OS level
- [ ] Runtime tested via test harness
- [ ] Second reviewer sign-off: ___________________  Date: ________
