# Verification Matrix — EQQUX007 (OPC/TWS Operation Status Change)

**Status:** Draft
**Date:** 2026-03-14
**Verifier:** (assign before In Review)

---

## 1. Module Identity

| Item | Value |
|------|-------|
| Module name | EQQUX007 (entry: EQQUX007) |
| ASM source | `asm/OPC/EQQUX007.asm` |
| C source | `converted/OPC/EQQUX007.c` |
| Product | IBM TWS z/OS (OPC/ESA) |
| Exit point | EQQUX007 — Operation Status Change Exit |
| Header | `includes/metalc_opc.h` |
| Entry label | `EQQUX007` |
| AMODE | 31 |
| Attributes | REENTRANT, RMODE ANY |

---

## 2. Entry Conditions — Register-to-Variable Mapping

| Register | ASM role | C variable | Type |
|----------|----------|-----------|------|
| R1 | UX007 parameter block | `parm` | `struct opc_ux007_parm *` |
| R3 | Operation block (from parm->ux007oper) | `oper` | `struct opc_oper *` |
| R12 | Base register | — | — |
| R15 | Return code (always 0) | function return value | `int` |

---

## 3. Logic Equivalence Table

| ASM Label | ASM Instruction(s) | C Equivalent | Verified? | Concern # |
|-----------|--------------------|--------------|-----------|-----------|
| Entry | `LR R12,R15` | `#pragma prolog(EQQUX007, "SAVE(14,12),LR(12,15)")` | Yes | — |
| Load parm | `L R10,0(,R1)` | `parm` passed directly as typed parameter | Yes | — |
| Check func | `CLC 0(4,R10),=F'1'` | `if (parm->ux007func != OPC_UX007_STATUS)` | Yes | — |
| Check status E | `CLI UX007NEWST,C'E'` | `if (parm->ux007newst != OPC_STATUS_ERROR)` | Yes | C1 |
| Load oper block | `L R3,UX007OPER` | `struct opc_oper *oper = parm->ux007oper` | Yes | — |
| Null check | `LTR R3,R3` / `BZ RETURN` | `if (oper == NULL) return OPC_UX007_CONTINUE` | Yes | — |
| Check critical path | `TM 71(R3),X'80'` | `if (opc_is_critical_path(oper))` | Yes | C2 |
| Issue alert WTO | `WTO` macro | `wto_write(msg, pos, MASTER_CONSOLE\|PROG_INFO, DESC_EVENTUAL_ACTION)` | Yes | — |
| RETURN | `SR R15,R15 / BR R14` | `return OPC_UX007_CONTINUE` | Yes | — |

---

## 4. Scope Reduction — Functions Present in ASM but Absent in C

No scope reduction. All ASM functions are represented in the C conversion.

---

## 5. Return Code Path Inventory

| Path | Condition | Expected RC | C matches ASM? |
|------|-----------|-------------|----------------|
| Continue — non-status function | `ux007func != OPC_UX007_STATUS` | `OPC_UX007_CONTINUE` (0) | Yes |
| Continue — non-error status | `ux007newst != OPC_STATUS_ERROR` | `OPC_UX007_CONTINUE` (0) | Yes |
| Continue — null oper block | `oper == NULL` | `OPC_UX007_CONTINUE` (0) | Yes |
| Continue — non-critical error | Critical path flag not set | `OPC_UX007_CONTINUE` (0) | Yes |
| Continue — critical path error (alerted) | Critical flag set, error status | `OPC_UX007_CONTINUE` (0) | Yes |

---

## 6. Flagged Concerns

**C1 — `OPC_STATUS_ERROR` is character `'E'`**
> The ASM `CLI UX007NEWST,C'E'` compares a single EBCDIC byte.
> The C constant `OPC_STATUS_ERROR` must be defined as `'E'` (0xC5 EBCDIC).
> Verify the definition in `metalc_opc.h`.
> **Assessment:** Open — verify `OPC_STATUS_ERROR` value in header.

**C2 — Critical path flag at offset 71 vs. struct field `op_flags` at +11**
> The ASM uses `TM 71(R3),X'80'` where R3 is the EQQOPBK DSECT base.
> This implies the critical path flag byte is at offset +71 from the
> EQQOPBK base.
> The C struct `opc_oper` has `op_flags` at offset +11, and
> `opc_is_critical_path()` tests bit `OPC_OP_FLG_CRITICAL` (0x80) of `op_flags`.
> If the EQQOPBK DSECT places the flag byte at +11, then `TM 71(R3)` would
> imply R3 is based 60 bytes before the DSECT start — this seems unlikely.
> **Assessment:** HIGH — offset discrepancy.  Cross-reference `TM 71(R3),X'80'`
> against the actual EQQOPBK macro to determine the correct field offset.
> The `opc_oper` struct may need to be corrected in `metalc_opc.h`.

---

## 7. Sign-off Checklist

- [ ] All in-scope ASM labels mapped to C equivalents
- [ ] C1 `OPC_STATUS_ERROR` value verified as `'E'` in `metalc_opc.h`
- [ ] C2 critical path flag offset resolved against EQQOPBK macro
- [ ] `opc_oper` struct `op_flags` offset corrected if necessary
- [ ] `verify_structs.c` compiles clean on target z/OS level
- [ ] Runtime tested with critical and non-critical path jobs ending in error
- [ ] Second reviewer sign-off: ___________________  Date: ________
