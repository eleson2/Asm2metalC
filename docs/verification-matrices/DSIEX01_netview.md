# Verification Matrix — DSIEX01 (NetView Command Preprocessing)

**Status:** Draft
**Date:** 2026-03-14
**Verifier:** (assign before In Review)

---

## 1. Module Identity

| Item | Value |
|------|-------|
| Module name | DSIEX01 (entry: DSIEX01) |
| ASM source | `asm/NETVIEW/DSIEX01.asm` |
| C source | `converted/NETVIEW/DSIEX01.c` |
| Product | IBM NetView for z/OS |
| Exit point | DSIEX01 — Command Preprocessing Exit |
| Header | `includes/metalc_netview.h` |
| Entry label | `DSIEX01` |
| AMODE | 31 |
| Attributes | REENTRANT, RMODE ANY |

---

## 2. Entry Conditions — Register-to-Variable Mapping

| Register | ASM role | C variable | Type |
|----------|----------|-----------|------|
| R1 | NV command parameter block | `parm` | `struct nv_cmd_parm *` |
| R12 | Base register | — | — |
| R15 | Return code | function return value | `int` |

Note: `parm->func` (+4) = `nvfunc`; `parm->flags` (+5) = `nvcmdtype`; `parm->reserved` (+6) = `nvflags` (2-byte auth flags).

---

## 3. Logic Equivalence Table

| ASM Label | ASM Instruction(s) | C Equivalent | Verified? | Concern # |
|-----------|--------------------|--------------|-----------|-----------|
| Entry | `LR R12,R15` | `#pragma prolog(DSIEX01, "SAVE(14,12),LR(12,15)")` | Yes | — |
| Check pre-proc | `CLI NVFUNC,1` / `BNE RETURN` | `if (parm->func != NV_FUNC_PRE) return NV_CMD_CONTINUE` | Yes | — |
| Check MVS cmd | `CLI NVCMDTYPE,2` / `BNE CHKVTAM` | `if (parm->flags == NV_CMDTYPE_MVS)` | Yes | — |
| Check auth flag | `TM NVFLAGS,X'80'` / `BO RETURN` | `if (TM_ALL(parm->reserved, NV_FLG_AUTH)) return NV_CMD_CONTINUE` | Yes | C1 |
| Load cmd ptr | `L R5,NVPCMDP` | `char *cmd = parm->nvcmd` | Yes | — |
| Load cmd len | `L R6,NVPCMDL` | `uint32_t len = parm->nvcmdlen` | Yes | — |
| Check CANCEL | `CLC 0(6,R5),=CL6'CANCEL'` / `BE REJECT` | `if (len >= 6 && match_prefix(cmd, "CANCEL", 6)) goto REJECT` | Yes | — |
| Check FORCE | `CLC 0(5,R5),=CL5'FORCE'` / `BE REJECT` | `if (len >= 5 && match_prefix(cmd, "FORCE", 5)) goto REJECT` | Yes | — |
| CHKVTAM | `CLI NVCMDTYPE,4` / `BNE RETURN` | `else if (parm->flags == NV_CMDTYPE_VTAM)` | Yes | — |
| Log VTAM cmd | Build WTO | `msg_append_str` + `wto_write(PROG_INFO)` | Yes | — |
| RETURN continue | `SR R15,R15 / BR R14` | `return NV_CMD_CONTINUE` | Yes | — |
| REJECT path | Issue alert WTO + set reason | `wto_write(MASTER_CONSOLE\|SYS_SECURITY, IMMEDIATE_ACTION)` + `parm->nvreasn = 100` | Yes | — |
| REJECT return | `LA R15,4 / BR R14` | `return NV_CMD_SUPPRESS` | Yes | — |

---

## 4. Scope Reduction — Functions Present in ASM but Absent in C

No scope reduction. All ASM functions are represented in the C conversion.

---

## 5. Return Code Path Inventory

| Path | Condition | Expected RC | C matches ASM? |
|------|-----------|-------------|----------------|
| Continue — non-pre function | `func != NV_FUNC_PRE` | `NV_CMD_CONTINUE` (0) | Yes |
| Continue — non-MVS, non-VTAM cmd | Other command types | `NV_CMD_CONTINUE` (0) | Yes |
| Continue — authorized operator | `NV_FLG_AUTH` set | `NV_CMD_CONTINUE` (0) | Yes |
| Continue — benign MVS command | Not CANCEL or FORCE | `NV_CMD_CONTINUE` (0) | Yes |
| Continue — VTAM command (logged) | VTAM cmd, any operator | `NV_CMD_CONTINUE` (0) | Yes |
| Suppress — CANCEL by non-auth | Unauthorized CANCEL | `NV_CMD_SUPPRESS` (4) | Yes |
| Suppress — FORCE by non-auth | Unauthorized FORCE | `NV_CMD_SUPPRESS` (4) | Yes |

---

## 6. Flagged Concerns

**C1 — `nvflags` as 2-byte field in `parm->reserved`**
> The `reserved` field in `EXIT_PARM_HEADER` is 2 bytes (+6 to +7).
> The `nvflags` field in the ASM DSECT occupies the same 2 bytes.
> `NV_FLG_AUTH` is `0x8000`, which is the high bit of the first byte.
> The C check `TM_ALL(parm->reserved, NV_FLG_AUTH)` tests a 2-byte value.
> This is correct on big-endian z/OS, but the `TM_ALL` macro must work
> on `uint16_t` (the type of `reserved`).
> **Assessment:** Open — verify `TM_ALL` macro handles `uint16_t` correctly,
> or cast `parm->reserved` to `uint8_t` and use `0x80` for the bit check.

---

## 7. Sign-off Checklist

- [ ] All in-scope ASM labels mapped to C equivalents
- [ ] C1 `NV_FLG_AUTH` bit check with `uint16_t` verified
- [ ] CANCEL and FORCE command text comparisons verified
- [ ] VTAM audit logging format verified
- [ ] `nvreasn` value set correctly (100) for suppressed commands
- [ ] `verify_structs.c` compiles clean on target z/OS level
- [ ] Runtime tested for authorized and non-authorized operators
- [ ] Second reviewer sign-off: ___________________  Date: ________
