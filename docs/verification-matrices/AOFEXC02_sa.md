# Verification Matrix — AOFEXC02 (SA Resource State Change Exit)

**Status:** Draft
**Date:** 2026-03-14
**Verifier:** (assign before In Review)

---

## 1. Module Identity

| Item | Value |
|------|-------|
| Module name | AOFEXC02 (entry: AOFEXC02) |
| ASM source | `asm/SA/AOFEXC02.asm` |
| C source | `converted/SA/AOFEXC02.c` |
| Product | IBM System Automation for z/OS |
| Exit point | AOFEXC02 — Resource State Change Exit |
| Header | `includes/metalc_sa.h` |
| Entry label | `AOFEXC02` |
| AMODE | 31 |
| Attributes | REENTRANT, RMODE ANY |

---

## 2. Entry Conditions — Register-to-Variable Mapping

| Register | ASM role | C variable | Type |
|----------|----------|-----------|------|
| R1 | SA resource parameter block | `parm` | `struct sa_res_parm *` |
| R3 | Resource block pointer (from parm->sares) | `res` | `struct sa_resource *` |
| R12 | Base register | — | — |
| R15 | Return code | function return value | `int` |

---

## 3. Logic Equivalence Table

| ASM Label | ASM Instruction(s) | C Equivalent | Verified? | Concern # |
|-----------|--------------------|--------------|-----------|-----------|
| Entry | `LR R12,R15` | `#pragma prolog(AOFEXC02, "SAVE(14,12),LR(12,15)")` | Yes | — |
| Check func | `CLI AOFFUNC,3` / `BNE RETURN` | `if (parm->func != SA_FUNC_STATCHG) return SA_RES_CONTINUE` | Yes | — |
| Load res block | `L R3,SARESPTR` | `res = parm->sares` | Yes | — |
| Null check | `LTR R3,R3` / `BZ RETURN` | `if (res == NULL) return SA_RES_CONTINUE` | Yes | — |
| Check HARDDOWN | `CLI SANEWSTATE,1` / `BNE CHKCRIT` | `if (parm->sanewstate == SA_STATE_HARDDOWN)` | Yes | — |
| Log HARDDOWN | Build WTO | `msg_append_str/field` + `wto_write(MASTER_CONSOLE\|SYS_ERROR, 0)` | Yes | — |
| Check CRITICAL flag | `TM RESFLAGS,SARESCRIT` / `BZ RETURN` | `if (TM_ALL(res->flags, SA_RES_FLG_CRITICAL))` | Yes | C1 |
| Check failure states | `CLI SANEWSTATE,3` / `CLI SANEWSTATE,1` / etc. | `if (parm->sanewstate == PROBLEM\|HARDDOWN\|DEGRADED)` | Yes | — |
| Issue critical alert | Build WTO | `wto_write(MASTER_CONSOLE\|SYS_SECURITY, DESC_SYSTEM_FAILURE)` | Yes | — |
| Check recovery flag | `TM SAFLAGS,SARECOV` / `BZ RETURN` | `if (TM_ALL(parm->reserved, SA_FLG_RECOVERY))` | Yes | C2 |
| Check assist mode | `CLI RESAUTO,2` / `BNE RETURN` | `if (res->automation == SA_AUTO_ASSIST)` | Yes | — |
| SUPPRESS return | `LA R15,4 / BR R14` | `return SA_RES_SUPPRESS` | Yes | — |
| CONTINUE return | `SR R15,R15 / BR R14` | `return SA_RES_CONTINUE` | Yes | — |

---

## 4. Scope Reduction — Functions Present in ASM but Absent in C

No scope reduction. All ASM functions are represented in the C conversion.

---

## 5. Return Code Path Inventory

| Path | Condition | Expected RC | C matches ASM? |
|------|-----------|-------------|----------------|
| Continue — non-state-change | `func != SA_FUNC_STATCHG` | `SA_RES_CONTINUE` (0) | Yes |
| Continue — null resource block | `res == NULL` | `SA_RES_CONTINUE` (0) | Yes |
| Continue — non-critical resource | Critical flag not set | `SA_RES_CONTINUE` (0) | Yes |
| Continue — non-failure state | State is UP, STARTING, STOPPING | `SA_RES_CONTINUE` (0) | Yes |
| Continue — not in recovery/not assist | Recovery flag or assist mode not set | `SA_RES_CONTINUE` (0) | Yes |
| Suppress — recovery + assist mode | Recovery flag set AND automation == ASSIST | `SA_RES_SUPPRESS` (4) | Yes |

---

## 6. Flagged Concerns

**C1 — `SA_RES_FLG_CRITICAL` bit value**
> The ASM `TM RESFLAGS,SARESCRIT` tests the critical flag.
> Verify that `SA_RES_FLG_CRITICAL` in `metalc_sa.h` has the same bit
> value as `SARESCRIT` in the SA automation DSECT.
> **Assessment:** Open — verify against SA automation DSECT macros.

**C2 — `SA_FLG_RECOVERY` in `parm->reserved`**
> The ASM `TM SAFLAGS,SARECOV` tests the recovery flag.
> In the C, `parm->reserved` (from `EXIT_PARM_HEADER`) is used to hold
> these flags.  Verify that `SA_FLG_RECOVERY` in `metalc_sa.h` corresponds
> to the `SARECOV` bit at the correct position in the `reserved` field.
> **Assessment:** Open — verify bit position against AOFRPARM DSECT.

---

## 7. Sign-off Checklist

- [ ] All in-scope ASM labels mapped to C equivalents
- [ ] C1 `SA_RES_FLG_CRITICAL` value verified against SA DSECT
- [ ] C2 `SA_FLG_RECOVERY` position in `reserved` field verified
- [ ] `SA_RES_SUPPRESS` use confirmed safe (not suppressing on security failures)
- [ ] WTO routing codes match ASM (MASTER_CONSOLE for HARDDOWN, SYS_FAILURE for critical)
- [ ] `verify_structs.c` compiles clean on target z/OS level
- [ ] Runtime tested for HARDDOWN, PROBLEM, DEGRADED, SOFTDOWN states
- [ ] Second reviewer sign-off: ___________________  Date: ________
