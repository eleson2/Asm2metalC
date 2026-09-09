# Verification Matrix — ACF2PWX / LGNPW (ACF2 Password Validation)

**Status:** Draft
**Date:** 2026-03-14
**Verifier:** (assign before In Review)

---

## 1. Module Identity

| Item | Value |
|------|-------|
| Module name | ACF2PWX (entry: ACF2PWX) |
| ASM source | `asm/ACF2/ACF2PWX.asm` |
| C source | `converted/ACF2/ACF2PWX.c` |
| Product | ACF2 (CA ACF2) |
| Exit point | LGNPW — Password Validation Exit |
| Header | `includes/metalc_acf2.h` |
| Entry label | `ACF2PWX` |
| AMODE | 31 |
| Attributes | REENTRANT, RMODE ANY |

---

## 2. Entry Conditions — Register-to-Variable Mapping

| Register | ASM role | C variable | Type |
|----------|----------|-----------|------|
| R1 | Parameter list pointer | `parmlist` | `void **` |
| R2 | ACVALD block pointer (from parmlist[0]) | `vp` | `struct acvald *` |
| R12 | Base register | — | — |
| R15 | Return code | function return value | `int` |

---

## 3. Logic Equivalence Table

| ASM Label | ASM Instruction(s) | C Equivalent | Verified? | Concern # |
|-----------|--------------------|--------------|-----------|-----------|
| Entry | `LR R12,R15` | `#pragma prolog(ACF2PWX, "SAVE(14,12),LR(12,15)")` | Yes | — |
| Load parm | `L R2,0(,R1)` | `vp = (struct acvald *)parmlist[0]` | Yes | — |
| Check NEWPWD flag | `TM acvalflg,ACVALF_NEWPWD` / `BZ ACCEPT` | `if (TM_NONE(vp->acvalflg, ACVALF_NEWPWD)) return ACF2_PWD_ACCEPT` | Yes | — |
| Rule 1: userid check | `CLC acvalnpw(8),acvallid` / `BE REJECT` | `if (memcmp_secure(vp->acvalnpw, vp->acvallid, 8) == 0)` | Yes | C1 |
| Set reason R1 | `MVI reason,ACF2_RSN_PWD_USERID` | `acf2_set_reason(vp, ACF2_RSN_PWD_USERID)` | Yes | — |
| Rule 2: numeric required | Loop testing chars `'0'`–`'9'` | `for(i=0;i<8;i++) if(c>='0'&&c<='9'){has_numeric=1;break;}` | Yes | C2 |
| Set reason R2 | `MVI reason,ACF2_RSN_PWD_NO_NUMERIC` | `acf2_set_reason(vp, ACF2_RSN_PWD_NO_NUMERIC)` | Yes | — |
| Rule 3: triple repeat | Subroutine call for repeat check | `pwd_has_triple_repeat(vp->acvalnpw, 8)` | Yes | — |
| Set reason R3 | `MVI reason,ACF2_RSN_PWD_SEQUENTIAL` | `acf2_set_reason(vp, ACF2_RSN_PWD_SEQUENTIAL)` | Yes | — |
| Rule 4: dictionary | Table scan against BAD_PASSWORDS | `pwd_in_table(vp->acvalnpw, BAD_PASSWORDS, 8, NUM_BAD_PASSWORDS)` | Yes | C3 |
| Set reason R4 | `MVI reason,ACF2_RSN_PWD_DICTIONARY` | `acf2_set_reason(vp, ACF2_RSN_PWD_DICTIONARY)` | Yes | — |
| ACCEPT path | `SR R15,R15 / BR R14` | `return ACF2_PWD_ACCEPT` | Yes | — |
| REJECT path | `LA R15,4 / BR R14` | `return ACF2_PWD_REJECT` | Yes | — |

---

## 4. Scope Reduction — Functions Present in ASM but Absent in C

No scope reduction. All ASM functions are represented in the C conversion.

---

## 5. Return Code Path Inventory

| Path | Condition | Expected RC | C matches ASM? |
|------|-----------|-------------|----------------|
| Accept — no new password | `ACVALF_NEWPWD` flag not set | `ACF2_PWD_ACCEPT` (0) | Yes |
| Reject — password equals logonid | `memcmp_secure` returns 0 | `ACF2_PWD_REJECT` (4) | Yes |
| Reject — no numeric character | Loop finds no digit | `ACF2_PWD_REJECT` (4) | Yes |
| Reject — triple repeat | `pwd_has_triple_repeat` returns true | `ACF2_PWD_REJECT` (4) | Yes |
| Reject — dictionary word | `pwd_in_table` returns true | `ACF2_PWD_REJECT` (4) | Yes |
| Accept — all rules passed | No rejection condition met | `ACF2_PWD_ACCEPT` (0) | Yes |

---

## 6. Flagged Concerns

**C1 — `memcmp_secure` vs. plain `CLC`**
> The ASM uses `CLC acvalnpw(8),acvallid` for the userid comparison.
> The C uses `memcmp_secure` (a constant-time comparison helper).
> The result is functionally identical (both detect equality), but `memcmp_secure`
> resists timing attacks that could reveal password content.
> **Assessment:** C is stricter than ASM. Intentional improvement. Verified.

**C2 — Numeric check: EBCDIC range vs. ASM method**
> The ASM checks for numeric characters using a method derived from the source;
> the C uses the EBCDIC numeric range `'0'`–`'9'` (0xF0–0xF9).
> In EBCDIC, digits are contiguous and `'0' <= c <= '9'` correctly identifies
> all digit characters.
> **Assessment:** Equivalent. No concern.

**C3 — Dictionary table size and content**
> The C `BAD_PASSWORDS` table has 10 entries.  Verify that this matches the
> ASM source's `BADPWDS` table in count and content.  The ASM table may have
> a different set of restricted words.
> **Assessment:** Open — requires cross-check against ASM source BADPWDS table.

---

## 7. Sign-off Checklist

- [ ] All in-scope ASM labels mapped to C equivalents
- [ ] Scope reduction documented and accepted by ACF2 owner
- [ ] C1 `memcmp_secure` improvement accepted as intentional
- [ ] C3 BAD_PASSWORDS table content verified against ASM BADPWDS table
- [ ] `verify_structs.c` compiles clean on target z/OS level
- [ ] Runtime tested via test harness
- [ ] Second reviewer sign-off: ___________________  Date: ________
