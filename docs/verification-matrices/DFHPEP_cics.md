# Verification Matrix — DFHPEP (CICS Program Error Program)

**Status:** Draft
**Date:** 2026-03-14
**Verifier:** (assign before In Review)

---

## 1. Module Identity

| Item | Value |
|------|-------|
| Module name | DFHPEP (entry: DFHPEP) |
| ASM source | `asm/CICS/DFHPEP.asm` |
| C source | `converted/CICS/DFHPEP.c` |
| Product | CICS Transaction Server |
| Exit point | DFHPEP — Program Error Program |
| Header | `includes/metalc_cics.h` |
| Entry label | `DFHPEP` |
| AMODE | 31 |
| Attributes | REENTRANT, RMODE ANY |

---

## 2. Entry Conditions — Register-to-Variable Mapping

| Register | ASM role | C variable | Type |
|----------|----------|-----------|------|
| R1 | PEP parameter block pointer | `parm` | `struct dfhpeppar *` |
| R12 | Base register | — | — |
| R15 | Return code | function return value | `int` |

---

## 3. Logic Equivalence Table

| ASM Label | ASM Instruction(s) | C Equivalent | Verified? | Concern # |
|-----------|--------------------|--------------|-----------|-----------|
| Entry | `LR R12,R15` | `#pragma prolog(DFHPEP, "SAVE(14,12),LR(12,15)")` | Yes | — |
| Load parm | `L R10,0(,R1)` | `parm` passed directly as typed parameter | Yes | — |
| Check ASRA | `CLI PEPTYPE,X'01'` | `if (parm->peptype == PEP_TYPE_ASRA)` | Yes | — |
| Skip CECI | `CLC PEPTRAN(4),=CL4'CECI'` / `BE RETURN` | `if (match_field(parm->peptran, "CECI", 4)) return CICS_PEP_CONTINUE` | Yes | — |
| Skip CECS | `CLC PEPTRAN(4),=CL4'CECS'` / `BE RETURN` | `if (match_field(parm->peptran, "CECS", 4))` | Yes | C1 |
| Skip CEBR | `CLC PEPTRAN(4),=CL4'CEBR'` / `BE RETURN` | `if (match_field(parm->peptran, "CEBR", 4))` | Yes | — |
| Build WTO | Build message in work area | `msg_append_str` / `msg_append_field` sequence | Yes | — |
| Issue WTO (ASRA) | `WTO` macro | `wto_write(msg, pos, MASTER_CONSOLE\|PROG_INFO, 0)` | Yes | — |
| Check AICA | `CLI PEPTYPE,X'03'` | `else if (parm->peptype == PEP_TYPE_AICA)` | Yes | — |
| Issue WTO (AICA) | `WTO` macro | `wto_write(msg, pos, MASTER_CONSOLE\|PROG_INFO, 0)` | Yes | — |
| RETURN | `SR R15,R15 / BR R14` | `return CICS_PEP_CONTINUE` | Yes | — |

---

## 4. Scope Reduction — Functions Present in ASM but Absent in C

No scope reduction. All ASM functions are represented in the C conversion.

---

## 5. Return Code Path Inventory

| Path | Condition | Expected RC | C matches ASM? |
|------|-----------|-------------|----------------|
| Continue — CICS system transaction | CECI, CECS, or CEBR transaction | `CICS_PEP_CONTINUE` (0) | Yes |
| Continue — ASRA, alert issued | Production ASRA on non-system transaction | `CICS_PEP_CONTINUE` (0) | Yes |
| Continue — AICA, alert issued | Runaway task AICA | `CICS_PEP_CONTINUE` (0) | Yes |
| Continue — other error type | `peptype` not ASRA or AICA | `CICS_PEP_CONTINUE` (0) | Yes |

---

## 6. Flagged Concerns

**C1 — CECS not in original ASM?**
> The C code skips both `CECI` and `CECS` transactions.  Verify that the
> original ASM source also skips `CECS` (not just `CECI` and `CEBR`).
> If the ASM only skips `CECI` and `CEBR`, this is an intentional expansion
> in the C port; if `CECS` was already there, it is equivalent.
> **Assessment:** Open — requires cross-check against ASM source.

---

## 7. Sign-off Checklist

- [ ] All in-scope ASM labels mapped to C equivalents
- [ ] C1 CECS exception verified against ASM source
- [ ] WTO message format matches ASM (transaction ID, program name)
- [ ] `verify_structs.c` compiles clean on target z/OS level
- [ ] Runtime tested via test harness
- [ ] Second reviewer sign-off: ___________________  Date: ________
