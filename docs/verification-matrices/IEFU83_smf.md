# Verification Matrix — IEFU83 (SMF Record Filtering)

**Status:** Draft
**Date:** 2026-03-11
**Verifier:** (assign before In Review)

---

## 1. Module Identity

| Item | Value |
|------|-------|
| Module name | IEFU83 |
| ASM source | `asm/SMF/IEFU83.asm` |
| C source | `converted/SMF/IEFU83.c` |
| Product | SMF (System Management Facilities) |
| Exit point | IEFU83 — SMF Record Write Exit (before write) |
| Header | `includes/metalc_smf.h` |
| Entry label | `IEFU83` |
| AMODE | 31 |
| Attributes | REENTRANT, RMODE ANY |

---

## 2. Entry Conditions — Register-to-Variable Mapping

| Register | ASM role | C variable | Type |
|----------|----------|-----------|------|
| R1 | Pointer to parameter list (address of SMF record pointer) | `parmlist` | `void **` |
| R2 | SMF record base address | `record` | `struct smf_header *` |
| R3 | Byte offset into record (loaded from `smf30sof`) | (implicit in `ADDR_AT_OFFSET`) | — |
| R12 | Base register (established by prolog) | — | — |
| R15 | Return code on exit | function return value | `int` |

---

## 3. Logic Equivalence Table

| ASM Label | ASM Instruction(s) | C Equivalent | Verified? | Concern # |
|-----------|--------------------|--------------|-----------|-----------|
| `U83ENTRY` | `USING *,R12` | `#pragma prolog(IEFU83,"SAVE(14,12),LR(12,15)")` | Yes | — |
| `U83INIT` | `L R2,0(,R1)` — load record addr from parm | `record = (struct smf_header *)parmlist[0]` | Yes | — |
| `U83NULL` | `LTR R2,R2` / `BZ U83WRITE` | `if (record == NULL) return SMF_RC_WRITE;` | Yes | C1 (null guard not in ASM) |
| `U83TYPE` | `TM 5(R2),X'FF'` / `CLI 5(R2),40` | `type = record->smfrty /* +5 */` | Yes | — |
| `U83T40` | `CLI 5(R2),40` / `BE U83SUPP` | `if (type == 40 \|\| type == 42) return SMF_RC_SUPPRESS;` | Yes | — |
| `U83T42` | `CLI 5(R2),42` / `BE U83SUPP` | (same combined if) | Yes | — |
| `U83T30` | `CLI 5(R2),30` / `BNE U83T4` | `if (type == SMF_TYPE_30)` | Yes | — |
| `U83SOF` | `LH R3,24(R2)` — load id-section offset (sign-extend 16-bit) | `uint16_t id_offset = smf30->smf30sof /* +24 */` | Yes | C2 (sign vs. unsigned) |
| `U83ADDR` | `AR R3,R2` — add base to offset | `ADDR_AT_OFFSET(record, id_offset)` | Yes | — |
| `U83SYS` | `CLC 0(3,R3),=C'SYS'` — compare 3-char prefix | `match_prefix(id->smf30jbn,"SYS",3)` | Yes | — |
| `U83T4` | `CLI 5(R2),4` / `BE U83JBN` | `else if (type == SMF_TYPE_4 \|\| ...)` | Yes | — |
| `U83JBN` | `LA R3,18(,R2)` — addr of jobname at +18 | `ADDR_AT_OFFSET(record, sizeof(struct smf_header))` — sizeof is 18 | Yes | C3 (hardcoded 18 vs. sizeof) |
| `U83SYSJB` | `CLC 0(3,R3),=C'SYS'` | `match_prefix(jobname,"SYS",3)` | Yes | — |
| `U83SUPP` | `LA R15,4` / `BR R14` | `return SMF_RC_SUPPRESS; /* 4 */` | Yes | — |
| `U83WRITE` | `SR R15,R15` / `BR R14` | `return SMF_RC_WRITE; /* 0 */` | Yes | — |

---

## 4. Return Code Path Inventory

| Path | Condition | Expected RC | C matches ASM? |
|------|-----------|-------------|----------------|
| Write (default) | None of the suppression conditions met | `SMF_RC_WRITE` (0) | Yes |
| Suppress — null guard | `parmlist[0]` is NULL (C only) | `SMF_RC_WRITE` (0) | N/A — deliberate divergence |
| Suppress — type 40 | `record->smfrty == 40` | `SMF_RC_SUPPRESS` (4) | Yes |
| Suppress — type 42 | `record->smfrty == 42` | `SMF_RC_SUPPRESS` (4) | Yes |
| Suppress — type 30 SYS job | Type 30 and `smf30jbn` starts with "SYS" | `SMF_RC_SUPPRESS` (4) | Yes |
| Suppress — type 4/5/14/15 SYS job | Job name at `record+18` starts with "SYS" | `SMF_RC_SUPPRESS` (4) | Yes |
| Write — type 30, zero offset | `smf30->smf30sof == 0` | `SMF_RC_WRITE` (0) | Yes |

---

## 5. Flagged Concerns

**C1 — Null guard on parmlist[0] added in C only**
> The ASM unconditionally dereferences `0(,R1)` (load address of record).
> If `R1` itself is NULL the program would abend; if the loaded word is zero,
> the ASM performs a CLI against address 0 which is the PSA — undefined behaviour
> on a real system.  The C adds an explicit `if (record == NULL)` guard.
> **Assessment:** Safe, conservative improvement.  Not a semantic mismatch for
> any valid caller.  Accepted as deliberate divergence.

**C2 — Sign extension: ASM `LH` vs. C `uint16_t`**
> `LH R3,24(R2)` sign-extends the 16-bit value at smf30sof into a 32-bit register.
> The C maps `smf30sof` as `uint16_t` (unsigned).  For any legitimate SMF offset
> the value is in the range 0–32767, making the results identical.  An offset
> ≥ 32768 would give different addresses (very unlikely; SMF records are < 32 KB).
> **Assessment:** Functionally equivalent for all real SMF type 30 records.
> Flag for documentation; no code change needed.

**C3 — Job-name offset: `LA R3,18(,R2)` vs. `sizeof(struct smf_header)`**
> The ASM uses a literal 18-byte displacement.  The C uses
> `sizeof(struct smf_header)` which evaluates to 18 with `#pragma pack(1)`.
> This is equivalent and actually safer (changes to the struct would be
> caught by `verify_structs.c` assertions).
> **Assessment:** Verified equivalent.  No concern.

---

## 6. Sign-off Checklist

- [ ] All ASM labels mapped to C equivalents
- [ ] All return code paths confirmed match
- [ ] All concerns reviewed and dispositioned
- [ ] `tests/test_iefu83.c` passes all four test cases on z/OS
- [ ] `verify_structs.c` compiles clean on target z/OS level
- [ ] Second reviewer sign-off: ___________________  Date: ________
