# Verification Matrix — ISTEXCLY (VTAM Logon Verify Exit)

**Status:** Draft
**Date:** 2026-03-14
**Verifier:** (assign before In Review)

---

## 1. Module Identity

| Item | Value |
|------|-------|
| Module name | ISTEXCLY (entry: ISTEXCLY) |
| ASM source | `asm/VTAM/ISTEXCLY.asm` |
| C source | `converted/VTAM/ISTEXCLY.c` |
| Product | VTAM / z/OS Communications Server (SNA) |
| Exit point | ISTEXCLY — Logon Verify Exit |
| Header | `includes/metalc_vtam.h` |
| Entry label | `ISTEXCLY` |
| AMODE | 31 |
| Attributes | REENTRANT, RMODE ANY |

---

## 2. Entry Conditions — Register-to-Variable Mapping

| Register | ASM role | C variable | Type |
|----------|----------|-----------|------|
| R1 | VTAM logon verify parameter block | `parm` | `struct vtam_ly_parm *` |
| R12 | Base register | — | — |
| R15 | Return code | function return value | `int` |

---

## 3. Logic Equivalence Table

| ASM Label | ASM Instruction(s) | C Equivalent | Verified? | Concern # |
|-----------|--------------------|--------------|-----------|-----------|
| Entry | `LR R12,R15` | `#pragma prolog(ISTEXCLY, "SAVE(14,12),LR(12,15)")` | Yes | — |
| Check LOGON func | `CLI LYFUNC,1` / `BNE DEFER` | `if (parm->func != LY_FUNC_LOGON) return VTAM_LY_DEFER` | Yes | — |
| Log logon attempt | Build WTO with LU, APPL, USER | `msg_append_str/field` + `wto_write(SYS_SECURITY\|PROG_INFO)` | Yes | — |
| Check ADMIN appl | `CLC LYAPPL(5),=CL5'ADMIN'` / `BNE DEFER` | `if (match_prefix(parm->lyappl, "ADMIN", 5))` | Yes | C1 |
| Check ADM LU | `CLC LYLUNAME(3),=CL3'ADM'` / `BE DEFER` | `if (match_prefix(parm->lyluname, "ADM", 3)) return VTAM_LY_DEFER` | Yes | — |
| Set sense | `MVC LYSENSE(4),=X'080F0000'` | `parm->lysense = VTAM_SENSE_SECURITY` | Yes | C2 |
| Set reason | `MVC LYREASN(4),=F'100'` | `parm->lyreasn = 100` | Yes | — |
| REJECT return | `LA R15,4 / BR R14` | `return VTAM_LY_REJECT` | Yes | — |
| DEFER return | `LA R15,8 / BR R14` | `return VTAM_LY_DEFER` | Yes | — |

---

## 4. Scope Reduction — Functions Present in ASM but Absent in C

No scope reduction. All ASM functions are represented in the C conversion.

---

## 5. Return Code Path Inventory

| Path | Condition | Expected RC | C matches ASM? |
|------|-----------|-------------|----------------|
| Defer — non-logon function | `func != LY_FUNC_LOGON` | `VTAM_LY_DEFER` (8) | Yes |
| Defer — non-ADMIN application | App does not start with `ADMIN` | `VTAM_LY_DEFER` (8) | Yes |
| Defer — admin LU to ADMIN app | LU starts with `ADM` | `VTAM_LY_DEFER` (8) | Yes |
| Reject — non-admin LU to ADMIN app | LU does not start with `ADM`, app starts with `ADMIN` | `VTAM_LY_REJECT` (4) | Yes |

---

## 6. Flagged Concerns

**C1 — ADMIN application prefix length (5 vs. exact match)**
> The ASM checks `CLC LYAPPL(5),=CL5'ADMIN'` — first 5 bytes.
> The C uses `match_prefix(parm->lyappl, "ADMIN", 5)`.
> This matches `ADMIN   `, `ADMINPAY`, `ADMINIST`, etc.
> If the intent is only the application named exactly `ADMIN   `, the check
> should be `match_field(parm->lyappl, "ADMIN   ", 8)`.
> **Assessment:** Open — confirm intended application match scope with VTAM owner.

**C2 — `VTAM_SENSE_SECURITY` value `0x080F0000`**
> The sense code `0x080F0000` is a standard SNA security exception sense code.
> Verify that `VTAM_SENSE_SECURITY` in `metalc_vtam.h` is defined as `0x080F0000U`
> and that the byte order is correct (big-endian on z/OS, stored as-is).
> **Assessment:** Open — verify sense code value and byte order in header.

---

## 7. Sign-off Checklist

- [ ] All in-scope ASM labels mapped to C equivalents
- [ ] C1 ADMIN application prefix scope confirmed with VTAM owner
- [ ] C2 `VTAM_SENSE_SECURITY` value and byte order verified
- [ ] `VTAM_LY_DEFER` used as default (not `VTAM_LY_ACCEPT`)
- [ ] ATCSTRxx `EXIT ISTEXCLY,TYPE=LY,OPTION=ALWAYS` confirmed
- [ ] `verify_structs.c` compiles clean on target z/OS level
- [ ] Runtime tested with ADM* LUs and non-ADM LUs to ADMIN application
- [ ] Second reviewer sign-off: ___________________  Date: ________
