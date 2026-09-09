# Verification Matrix — CSQXLIB (MQ Channel Security Exit)

**Status:** Draft
**Date:** 2026-03-14
**Verifier:** (assign before In Review)

---

## 1. Module Identity

| Item | Value |
|------|-------|
| Module name | CSQXLIB (entry: CSQXLIB) |
| ASM source | `asm/MQ/CSQXLIB.asm` |
| C source | `converted/MQ/CSQXLIB.c` |
| Product | IBM MQ for z/OS |
| Exit point | Channel Security Exit (MQXR_INIT_SEC) |
| Header | `includes/metalc_mq.h` |
| Entry label | `CSQXLIB` |
| AMODE | 31 |
| Attributes | REENTRANT, RMODE ANY |

---

## 2. Entry Conditions — Register-to-Variable Mapping

| Register | ASM role | C variable | Type |
|----------|----------|-----------|------|
| R1 | Pointer list (4 pointers) | `parmlist` | `void **` |
| R2 | MQCXP pointer (from parmlist[0]) | `p_cxp` | `struct mqcxp *` |
| R3 | MQCD pointer (from parmlist[1]) | `p_cd` | `struct mqcd *` |
| R12 | Base register | — | — |
| R15 | Always 0 (exitResponse controls outcome) | 0 | `int` |

---

## 3. Logic Equivalence Table

| ASM Label | ASM Instruction(s) | C Equivalent | Verified? | Concern # |
|-----------|--------------------|--------------|-----------|-----------|
| Entry | `LR R12,R15` | `#pragma prolog(CSQXLIB, "SAVE(14,12),LR(12,15)")` | Yes | — |
| Load MQCXP | `L R2,0(,R1)` | `p_cxp = (struct mqcxp *)parmlist[0]` | Yes | — |
| Load MQCD | `L R3,4(,R1)` | `p_cd = (struct mqcd *)parmlist[1]` | Yes | — |
| Check exit reason | `CLC 12(4,R2),=F'6'` (MQXR_INIT_SEC) | `if (p_cxp->exitReason != MQXR_INIT_SEC)` | Yes | C1 |
| Set OK for non-SEC | `MVC 16(4,R2),=F'0'` | `p_cxp->exitResponse = MQXCC_OK; return 0` | Yes | — |
| Log connection | Build WTO | `msg_append_str` + `msg_append_field` + `wto_write` | Yes | — |
| Allow-list loop | Loop: `CLC CXPPARTN(20),PARTNERS+n` | `for loop: match_field(p_cxp->partnerName, ALLOWED_PARTNERS[i], 20)` | Yes | C2 |
| Set OK response | `MVC CXPEXITRESP(4),=F'0'` | `p_cxp->exitResponse = MQXCC_OK` | Yes | — |
| Set CLOSE response | `MVC CXPEXITRESP(4),=F'16'` | `p_cxp->exitResponse = MQXCC_CLOSE_CHANNEL` | Yes | — |
| Log rejection | WTO with MASTER_CONSOLE route | `wto_write(msg, pos, MASTER_CONSOLE\|SYS_SECURITY, DESC_CRITICAL_ACTION)` | Yes | — |
| Return | `SR R15,R15 / BR R14` | `return 0` | Yes | C3 |

---

## 4. Scope Reduction — Functions Present in ASM but Absent in C

No scope reduction. All ASM functions are represented in the C conversion.

---

## 5. Return Code Path Inventory

| Path | Condition | Expected RC (R15) | exitResponse | C matches ASM? |
|------|-----------|-------------------|-------------|----------------|
| Non-SEC reason | `exitReason != MQXR_INIT_SEC` | 0 | `MQXCC_OK` (0) | Yes |
| Partner in allow list | Name matches list entry | 0 | `MQXCC_OK` (0) | Yes |
| Partner not in allow list | Name not in list | 0 | `MQXCC_CLOSE_CHANNEL` (16) | Yes |

---

## 6. Flagged Concerns

**C1 — `exitReason` field offset**
> The ASM checks `CLC 12(4,R2),=F'6'` — field at offset +12 in MQCXP.
> The C checks `p_cxp->exitReason` which is defined at offset +12 in
> `struct mqcxp`.  Verify that the struct definition in `metalc_mq.h`
> places `exitReason` at +12.
> **Assessment:** Open — verify offset against current MQ CMQXC.H definitions.

**C2 — Partner name length in allow list (20 bytes)**
> Partner names in `ALLOWED_PARTNERS` are defined as 20-byte space-padded
> strings.  The comparison uses `match_field(p_cxp->partnerName, ALLOWED_PARTNERS[i], 20)`.
> Verify that `partnerName` in `metalc_mq.h` is exactly 20 bytes and at the
> correct offset in MQCXP.
> **Assessment:** Open — verify partner name offset and length in MQCXP struct.

**C3 — R15 always 0 in MQ exits**
> MQ exits always return 0 in R15; the channel decision is communicated via
> `exitResponse`.  The C correctly returns 0 from the function.
> This is a frequent source of confusion when converting from non-MQ exits.
> **Assessment:** Verified. Documented for future reference.

---

## 7. Sign-off Checklist

- [ ] All in-scope ASM labels mapped to C equivalents
- [ ] C1 `exitReason` field offset verified against current MQ CMQXC.H
- [ ] C2 `partnerName` offset and length verified
- [ ] Allow list contents verified against ASM source partner table
- [ ] `MQXCC_CLOSE_CHANNEL` vs `MQXCC_OK` semantics confirmed
- [ ] MQ channel initiator restarted after exit update
- [ ] `verify_structs.c` compiles clean on target z/OS level
- [ ] Runtime tested with authorized and unauthorized partner QM names
- [ ] Second reviewer sign-off: ___________________  Date: ________
