# Verification Matrix — DFSMSCE0 (IMS MSC Message Routing Exit)

**Status:** Draft
**Date:** 2026-03-14
**Verifier:** (assign before In Review)

---

## 1. Module Identity

| Item | Value |
|------|-------|
| Module name | DFSMSCE0 (entry: DFSMSCE0) |
| ASM source | `asm/IMS/DFSMSCE0.asm` |
| C source | `converted/IMS/DFSMSCE0.c` |
| Product | IMS/TM (Multiple Systems Coupling) |
| Exit point | DFSMSCE0 — MSC Message Routing and Control Exit |
| Header | `includes/metalc_ims.h` |
| Entry label | `DFSMSCE0` |
| AMODE | 31 |
| Attributes | REENTRANT, RMODE ANY |

---

## 2. Entry Conditions — Register-to-Variable Mapping

| Register | ASM role | C variable | Type |
|----------|----------|-----------|------|
| R1 | Pointer to pointer list | `parmlist` | `void **` |
| R2 | MSCP pointer (from parmlist[0]) | `parm` | `struct mscp *` |
| R4 | MSCD pointer (destination block) | `dest` | `struct mscd *` |
| R12 | Base register | — | — |
| R15 | Return code | function return value | `int` |

---

## 3. Logic Equivalence Table

| ASM Label | ASM Instruction(s) | C Equivalent | Verified? | Concern # |
|-----------|--------------------|--------------|-----------|-----------|
| Entry | `$ENTRY BASE=R12` | `#pragma prolog(DFSMSCE0, "SAVE(14,12),LR(12,15)")` | Yes | — |
| Load MSCP | `L R2,0(,R1)` | `parm = (struct mscp *)parmlist[0]` | Yes | — |
| Switch on func | `L R3,MSCPFUNC` / compare + branch | `switch (parm->mscpfunc)` | Yes | — |
| INIT case | No-op for init function | `case MSC_FUNC_INIT: break` | Yes | — |
| ROUTE case | Route function dispatch | `case MSC_FUNC_ROUTE:` | Yes | — |
| Load MSCD | `L R4,MSCPDEST` | `dest = parm->mscpdest` | Yes | — |
| Check LOG flag | `TM MSCDFLG1,MSCDLOG` / `BZ done` | `if (TM_NONE(dest->mscdflg1, MSCD_LOG)) break` | Yes | — |
| Check TERM1 | `CLC MSCDNAME(8),=CL8'TERM1   '` / `BNE done` | `if (match_field(dest->mscdname, "TERM1   ", 8))` | Yes | — |
| Set TERM2 | `MVC MSCDNAME(8),=CL8'TERM2   '` | `memcpy_inline(dest->mscdname, "TERM2   ", 8)` | Yes | — |
| Set REQD flag | `OI MSCPFLG1,MSCPREQD` | `OI(parm->mscpflg1, MSCP_REQD)` | Yes | C1 |
| Return | `SR R15,R15 / BR R14` | `return IMS_RC_CONTINUE` | Yes | — |

---

## 4. Scope Reduction — Functions Present in ASM but Absent in C

No scope reduction. All ASM functions are represented in the C conversion.

---

## 5. Return Code Path Inventory

| Path | Condition | Expected RC | C matches ASM? |
|------|-----------|-------------|----------------|
| Continue — INIT function | `mscpfunc == MSC_FUNC_INIT` | `IMS_RC_CONTINUE` (0) | Yes |
| Continue — non-logical terminal | `MSCD_LOG` flag not set | `IMS_RC_CONTINUE` (0) | Yes |
| Continue — destination not TERM1 | Name != `"TERM1   "` | `IMS_RC_CONTINUE` (0) | Yes |
| Continue — rerouted to TERM2 | Name == `"TERM1   "`, rerouted | `IMS_RC_CONTINUE` (0) | Yes |
| Continue — unknown func | `default:` case | `IMS_RC_CONTINUE` (0) | Yes |

---

## 6. Flagged Concerns

**C1 — MSCP_REQD flag mandatory after destination change**
> The `OI MSCPFLG1,MSCPREQD` instruction (C: `OI(parm->mscpflg1, MSCP_REQD)`)
> tells IMS that the destination was modified.  Without this flag, IMS ignores
> the destination change.
> Verify that `MSCP_REQD` in `metalc_ims.h` corresponds to the `MSCPREQD`
> bit defined in the `DFSMSCR` macro at the same bit position.
> **Assessment:** Open — verify bit value against current IMS DFSMSCR macro.

---

## 7. Sign-off Checklist

- [ ] All in-scope ASM labels mapped to C equivalents
- [ ] C1 `MSCP_REQD` bit value verified against current IMS DFSMSCR macro
- [ ] Destination rerouting logic verified (TERM1 → TERM2)
- [ ] `verify_structs.c` compiles clean on target z/OS level
- [ ] Runtime tested via test harness
- [ ] Second reviewer sign-off: ___________________  Date: ________
