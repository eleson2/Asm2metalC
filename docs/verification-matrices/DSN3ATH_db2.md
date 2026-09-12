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

**CX — `db2_ath_parm` was shifted by 2 bytes — RESOLVED**
> `db2_ath_parm` documented its first field after `EXIT_PARM_HEADER` at
> +6, but the macro occupied +0 through +7, so every field the exit
> touches sat 2 bytes past where the assembler puts it:
>
> | Read | Was at | ASM proves | Effect |
> |---|---|---|---|
> | `parm->athauth` | +10 | +8 | the `SYSADM` comparison read misaligned bytes |
> | `parm->athobj` | +18 | +16 | the `PAYROLL` prefix test read misaligned bytes |
> | `parm->athreasn` | +62 | +60 | the reason code went into the wrong word |
>
> `DSN3ATH.asm` settles the layout twice over — its prologue documents
> `+6(2) Privilege requested`, and the instruction stream pins each
> field independently:
>
> ```asm
>          CLI   4(R10),3                func at +4, 1 byte
>          CLC   8(8,R10),=CL8'SYSADM'   auth ID at +8, 8 bytes
>          CLC   16(7,R10),=C'PAYROLL'   object name at +16
>          MVC   60(4,R10),=F'200'       reason code at +60, 4 bytes
> ```
>
> **Assessment:** RESOLVED.  The common header for this block is 6
> bytes, so `db2_ath_parm` now uses `EXIT_PARM_HEADER_6` and declares
> `athpriv` at +6 itself.  No change was needed in this exit — the
> field names were already right; only the struct was wrong.  Offsets
> re-verified against the four instructions above.  See
> `docs/layout-findings.md` finding 4.

## 7. Sign-off Checklist

- [ ] All in-scope ASM labels mapped to C equivalents
- [ ] C1 PAYROLL prefix length (7 vs. 8 with dot) confirmed with DB2 owner
- [ ] `DB2_ATH_ALLOW` vs `DB2_ATH_CONTINUE` semantics confirmed correct
- [ ] `verify_structs.c` compiles clean on target z/OS level
- [ ] Runtime tested via test harness
- [ ] Privilege escalation risk reviewed (ALLOW path)
- [x] CX EXIT_PARM_HEADER 2-byte shift resolved from DSN3ATH.asm (finding 4)
      (docs/layout-findings.md finding 4)
- [ ] Second reviewer sign-off: ___________________  Date: ________
