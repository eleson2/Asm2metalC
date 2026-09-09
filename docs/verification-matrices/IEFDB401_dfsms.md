# Verification Matrix — IEFDB401 (DFSMS Dynamic Allocation Exit)

**Status:** Draft
**Date:** 2026-03-14
**Verifier:** (assign before In Review)

---

## 1. Module Identity

| Item | Value |
|------|-------|
| Module name | IEFDB401 (entry: IEFDB401) |
| ASM source | `asm/DFSMS/IEFDB401.asm` |
| C source | `converted/DFSMS/IEFDB401.c` |
| Product | DFSMS (z/OS Storage Management) |
| Exit point | IEFDB401 — Dynamic Allocation Exit |
| Header | `includes/metalc_dfsms.h` |
| Entry label | `IEFDB401` |
| AMODE | 31 |
| Attributes | REENTRANT, RMODE ANY |

---

## 2. Entry Conditions — Register-to-Variable Mapping

| Register | ASM role | C variable | Type |
|----------|----------|-----------|------|
| R1 | Allocation exit parameter block | `parm` | `struct alloc_exit_parm *` |
| R12 | Base register | — | — |
| R15 | Return code | function return value | `int` |

---

## 3. Logic Equivalence Table

| ASM Label | ASM Instruction(s) | C Equivalent | Verified? | Concern # |
|-----------|--------------------|--------------|-----------|-----------|
| Entry | `LR R12,R15` | `#pragma prolog(IEFDB401, "SAVE(14,12),LR(12,15)")` | Yes | — |
| Check func | `CLI ALCFUNC,1` / `BNE CONTINUE` | `if (parm->func != ALLOC_FUNC_ALLOC) return ALLOC_RC_CONTINUE` | Yes | — |
| Check CYL | `CLI ALCSPACE,1` / `BNE CHKPROD` | `if (parm->alcspace == ALLOC_SPACE_CYL &&` | Yes | — |
| Check >1000 | `CLC ALCPRI(4),=F'1000'` / `BNH CHKPROD` | `parm->alcpri > 1000)` | Yes | C1 |
| Set LARGE | `MVC ALCSCLAS(8),=CL8'LARGE   '` | `set_fixed_string(parm->alcsclas, "LARGE", 8)` | Yes | — |
| Log large | Build WTO + `WTO` macro | `msg_append_str` / `wto_important` | Yes | — |
| Return MODIFIED | `LA R15,4 / BR R14` | `return ALLOC_RC_MODIFIED` | Yes | — |
| Check PROD HLQ | `CLC ALCDSN(4),=CL4'PROD'` / helper | `if (sms_match_dsn_hlq(parm->alcdsn, "PROD"))` | Yes | C2 |
| Check already FAST | `CLC ALCSCLAS(8),=CL8'FAST    '` | `if (!match_field(parm->alcsclas, "FAST    ", 8))` | Yes | — |
| Set FAST | `MVC ALCSCLAS(8),=CL8'FAST    '` | `set_fixed_string(parm->alcsclas, "FAST", 8)` | Yes | — |
| Vol restriction loop | Loop: `CLC ALCVOL(6),RSVDVOLS+n` | `for loop: match_field(parm->alcvol, RESTRICTED_VOLS[i], 6)` | Yes | — |
| Log rejection | Build WTO + `WTO` macro | `msg_append_str` + `wto_write` | Yes | — |
| Return REJECT | `LA R15,8 / BR R14` | `return ALLOC_RC_REJECT` | Yes | — |
| CONTINUE path | `SR R15,R15 / BR R14` | `return ALLOC_RC_CONTINUE` | Yes | — |

---

## 4. Scope Reduction — Functions Present in ASM but Absent in C

No scope reduction. All ASM functions are represented in the C conversion.

---

## 5. Return Code Path Inventory

| Path | Condition | Expected RC | C matches ASM? |
|------|-----------|-------------|----------------|
| Continue — non-alloc function | `func != ALLOC_FUNC_ALLOC` | `ALLOC_RC_CONTINUE` (0) | Yes |
| Modified — large cylinder alloc | `alcspace == CYL && alcpri > 1000` | `ALLOC_RC_MODIFIED` (4) | Yes |
| Modified — PROD.* wrong storage class | PROD HLQ and `alcsclas != "FAST"` | `ALLOC_RC_MODIFIED` (4) | Yes |
| Reject — restricted volume | `alcvol` matches RSVD01 or RSVD02 | `ALLOC_RC_REJECT` (8) | Yes |
| Continue — no rules matched | None of the above | `ALLOC_RC_CONTINUE` (0) | Yes |

---

## 6. Flagged Concerns

**C1 — Large allocation threshold: `> 1000` vs. `>= 1000`**
> The ASM uses `CLC ALCPRI(4),=F'1000'` / `BNH CHKPROD` — branch if
> Not High, meaning the large-allocation path is taken only if `alcpri > 1000`.
> A value of exactly 1000 cylinders takes the CHKPROD path, not the LARGE path.
> The C uses `parm->alcpri > 1000` which is equivalent.
> **Assessment:** Equivalent. No concern.

**C2 — `sms_match_dsn_hlq` vs. direct CLC**
> The ASM checks the first 4 bytes of `ALCDSN` against `'PROD'`.
> The C uses `sms_match_dsn_hlq(parm->alcdsn, "PROD")` which additionally
> verifies that byte 5 is a `.` or space (preventing `PRODTEST.x` from matching).
> This is stricter than the raw ASM comparison.
> **Assessment:** C is more precise. Intentional improvement. Verify with DFSMS owner.

---

## 7. Sign-off Checklist

- [ ] All in-scope ASM labels mapped to C equivalents
- [ ] C2 `sms_match_dsn_hlq` stricter match confirmed with DFSMS owner
- [ ] Restricted volumes list matches ASM source RSVDVOLS table
- [ ] `ALLOC_RC_MODIFIED` vs `ALLOC_RC_CONTINUE` used correctly
- [ ] Exit in LPA or LINKLIST; IPL performed after update
- [ ] `verify_structs.c` compiles clean on target z/OS level
- [ ] Runtime tested via test harness
- [ ] Second reviewer sign-off: ___________________  Date: ________
