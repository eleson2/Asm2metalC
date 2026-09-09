# Verification Matrix — DSN3ATH (DB2 Authorization Exit)

**Status:** Draft
**Date:** 2026-03-14
**Verifier:** (assign before In Review)

---

## 1. Module Identity

| Item | Value |
|------|-------|
| Module name | DSN3ATH (entry: DSN3ATH) |
| ASM source | `asm/DB2/DSN3ATH.asm` |
| C source | `converted/DB2/DSN3ATH.c` |
| Product | DB2 for z/OS |
| Exit point | DSN3ATH — Authorization Exit Routine |
| Header | `includes/metalc_db2.h` |
| Entry label | `DSN3ATH` |
| AMODE | 31 |
| Attributes | REENTRANT, RMODE ANY |

---

## 2. Entry Conditions — Register-to-Variable Mapping

| Register | ASM role | C variable | Type |
|----------|----------|-----------|------|
| R1 | ATH parameter block pointer | `parm` | `struct db2_ath_parm *` |
| R12 | Base register | — | — |
| R15 | Return code | function return value | `int` |

---

## 3. Logic Equivalence Table

| ASM Label | ASM Instruction(s) | C Equivalent | Verified? | Concern # |
|-----------|--------------------|--------------|-----------|-----------|
| Entry | `LR R12,R15` | `#pragma prolog(DSN3ATH, "SAVE(14,12),LR(12,15)")` | Yes | — |
| Load parm | `L R10,0(,R1)` | `parm` passed directly as typed parameter | Yes | — |
| Check func | `CLC ATHFUNC(4),=F'3'` / `BNE CONTINUE` | `if (parm->func != ATH_FUNC_TABLE) return DB2_ATH_CONTINUE` | Yes | — |
| Check PAYROLL | `CLC ATHOBJ(7),=C'PAYROLL'` / `BNE CONTINUE` | `if (!match_prefix(parm->athobj, "PAYROLL", 7)) return DB2_ATH_CONTINUE` | Yes | C1 |
| Log access | Build WTO message | `msg_append_str` / `msg_append_field` + `wto_write` | Yes | — |
| Check SYSADM | `CLC ATHAUTH(8),=CL8'SYSADM  '` / `BE ALLOW` | `if (match_field(parm->athauth, "SYSADM  ", 8)) return DB2_ATH_ALLOW` | Yes | — |
| Set reason | `MVC ATHREASN(4),=F'200'` | `parm->athreasn = 200` | Yes | — |
| REJECT path | `LA R15,4 / BR R14` | `return DB2_ATH_REJECT` | Yes | — |
| ALLOW path | `SR R15,R15 / BR R14` | `return DB2_ATH_ALLOW` | Yes | — |
| CONTINUE path | `LA R15,8 / BR R14` | `return DB2_ATH_CONTINUE` | Yes | — |

---

## 4. Scope Reduction — Functions Present in ASM but Absent in C

No scope reduction. All ASM functions are represented in the C conversion.

---

## 5. Return Code Path Inventory

| Path | Condition | Expected RC | C matches ASM? |
|------|-----------|-------------|----------------|
| Continue — non-table function | `func != ATH_FUNC_TABLE` | `DB2_ATH_CONTINUE` (8) | Yes |
| Continue — non-PAYROLL object | Object does not start with `PAYROLL` | `DB2_ATH_CONTINUE` (8) | Yes |
| Allow — SYSADM auth ID | Auth ID is `SYSADM  ` | `DB2_ATH_ALLOW` (0) | Yes |
| Reject — non-SYSADM accessing PAYROLL | Auth ID is not SYSADM | `DB2_ATH_REJECT` (4) | Yes |

---

## 6. Flagged Concerns

**C1 — PAYROLL prefix length (7 vs. full schema check)**
> The ASM checks the first 7 bytes of `ATHOBJ` against `'PAYROLL'`.
> The C uses `match_prefix(parm->athobj, "PAYROLL", 7)` which is equivalent.
> Note: `ATHOBJ` contains `schema.tablename`; this check matches any object
> in the `PAYROLL` schema (e.g., `PAYROLL.EMPLOYEE`, `PAYROLLEXT.DATA`).
> If the intent is to match only `PAYROLL.` (dot-terminated), the check
> should be 8 bytes (`"PAYROLL."`).
> **Assessment:** Open — verify intended scope of PAYROLL prefix in ASM.

---

**CX — EXIT_PARM_HEADER structs are shifted by 2 bytes**
> `db2_ath_parm` documents its first field after `EXIT_PARM_HEADER` at +6,
> but the macro occupies +0 through +7.  Every later field in the struct
> is 2 bytes off what the header claims, and the declared total is 2
> short.  Confirmed independently by `make layout` and by the host lint
> build; tracked in `tools/layout_known_issues.txt`.
>
> Whether the fix is to renumber the comments (+8) or to stop using
> `EXIT_PARM_HEADER` for this product needs the vendor documentation for
> this exit's parameter list — see `docs/layout-findings.md` finding 4.
> **Assessment:** HIGH — until resolved, every field access in this
> parameter block may be reading the wrong bytes.

## 7. Sign-off Checklist

- [ ] All in-scope ASM labels mapped to C equivalents
- [ ] C1 PAYROLL prefix length (7 vs. 8 with dot) confirmed with DB2 owner
- [ ] `DB2_ATH_ALLOW` vs `DB2_ATH_CONTINUE` semantics confirmed correct
- [ ] `verify_structs.c` compiles clean on target z/OS level
- [ ] Runtime tested via test harness
- [ ] Privilege escalation risk reviewed (ALLOW path)
- [ ] CX EXIT_PARM_HEADER 2-byte shift resolved against vendor docs
      (docs/layout-findings.md finding 4)
- [ ] Second reviewer sign-off: ___________________  Date: ________
