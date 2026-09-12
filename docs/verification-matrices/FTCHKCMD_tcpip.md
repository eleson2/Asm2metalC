# Verification Matrix — FTCHKCMD (z/OS FTP Command Validation)

**Status:** Draft
**Date:** 2026-03-14
**Verifier:** (assign before In Review)

---

## 1. Module Identity

| Item | Value |
|------|-------|
| Module name | FTCHKCMD (entry: FTCHKCMD) |
| ASM source | `asm/TCPIP/FTCHKCMD.asm` |
| C source | `converted/TCPIP/FTCHKCMD.c` |
| Product | z/OS Communications Server (TCP/IP FTP) |
| Exit point | FTCHKCMD — FTP Command Validation Exit |
| Header | `includes/metalc_tcpip.h` |
| Entry label | `FTCHKCMD` |
| AMODE | 31 |
| Attributes | REENTRANT, RMODE ANY |

---

## 2. Entry Conditions — Register-to-Variable Mapping

| Register | ASM role | C variable | Type |
|----------|----------|-----------|------|
| R1 | FTP command check parameter block | `parm` | `struct ftp_chkcmd_parm *` |
| R12 | Base register | — | — |
| R15 | Return code | function return value | `int` |

Note: `parm->func` = `ftpfunc` (init/cmd/term); `parm->flags` = `ftpflags` (SSL/anon); `parm->ftpcmd` = the 2-byte command code at +6, now a named field.

---

## 3. Logic Equivalence Table

| ASM Label | ASM Instruction(s) | C Equivalent | Verified? | Concern # |
|-----------|--------------------|--------------|-----------|-----------|
| Entry | `LR R12,R15` | `#pragma prolog(FTCHKCMD, "SAVE(14,12),LR(12,15)")` | Yes | — |
| Check cmd func | `CLI FTPFUNC,2` / `BNE RETURN` | `if (parm->func != FTP_FUNC_CMD) return FTP_RC_CONTINUE` | Yes | — |
| Check DELE (23) | `CLC 6(2,R10),=H'23'` / `BNE CHKSTOR` | `if (parm->ftpcmd == FTP_CMD_DELE)` | Yes | C1 |
| Check SYS1 prefix | `CLC FTPDSN(5),=CL5'SYS1.'` / `BNE RETURN` | `if (match_prefix(parm->ftpdsn, "SYS1.", 5))` | Yes | — |
| Set reply 550 | `MVC FTPREPLY(2),=H'550'` | `parm->ftpreply = FTP_REPLY_NOACCESS` | Yes | — |
| Set reason 100 | `MVC FTPREASN(4),=F'100'` | `parm->ftpreasn = 100` | Yes | — |
| Reject DELE | `LA R15,4 / BR R14` | `return FTP_RC_REJECT` | Yes | — |
| Check STOR (15) | `CLC 6(2,R10),=H'15'` / `BNE CHKSITE` | `else if (parm->ftpcmd == FTP_CMD_STOR)` | Yes | — |
| Log upload | Build WTO | `msg_append_str/field` + `wto_write(PROG_INFO)` | Yes | — |
| Check SITE (29) | `CLC 6(2,R10),=H'29'` / `BNE RETURN` | `else if (parm->ftpcmd == FTP_CMD_SITE)` | Yes | — |
| Check internal IP | `CLC FTPCLIENT(1),=X'0A'` (10) | `if (parm->ftpclient.addr[0] != 10)` | Yes | C2 |
| Set reply 500 | `MVC FTPREPLY(2),=H'500'` | `parm->ftpreply = FTP_REPLY_CMDERR` | Yes | — |
| Reject SITE | `LA R15,4 / BR R14` | `return FTP_RC_REJECT` | Yes | — |
| RETURN | `SR R15,R15 / BR R14` | `return FTP_RC_CONTINUE` | Yes | — |

---

## 4. Scope Reduction — Functions Present in ASM but Absent in C

No scope reduction. All ASM functions are represented in the C conversion.

---

## 5. Return Code Path Inventory

| Path | Condition | Expected RC | C matches ASM? |
|------|-----------|-------------|----------------|
| Continue — non-command call | `func != FTP_FUNC_CMD` | `FTP_RC_CONTINUE` (0) | Yes |
| Reject — DELE of SYS1.* | `cmd==23` and DSN starts `SYS1.` | `FTP_RC_REJECT` (4) | Yes |
| Continue — DELE of non-SYS1 | `cmd==23` and DSN does not start `SYS1.` | `FTP_RC_CONTINUE` (0) | Yes |
| Continue — STOR (logged) | `cmd==15` | `FTP_RC_CONTINUE` (0) | Yes |
| Reject — SITE from external IP | `cmd==29` and `addr[0] != 10` | `FTP_RC_REJECT` (4) | Yes |
| Continue — SITE from internal IP | `cmd==29` and `addr[0] == 10` | `FTP_RC_CONTINUE` (0) | Yes |
| Continue — other command | `cmd` not 23, 15, or 29 | `FTP_RC_CONTINUE` (0) | Yes |

---

## 6. Flagged Concerns

**C1 — command code width and name — RESOLVED**
> This matrix previously transcribed the ASM as `CLI FTPCMD,23` and asked
> whether `ftpcmd` is 1 byte or 2.  The source is a **halfword compare**,
> three times over:
>
> ```asm
>          CLC   6(2,R10),=H'23'    DELE
>          CLC   6(2,R10),=H'15'    STOR
>          CLC   6(2,R10),=H'29'    SITE
> ```
>
> `CLC` with an `=H` literal reads **2 bytes at +6**, so a `uint16_t`
> compared against a small integer is correct and no cast is needed.
>
> The defect was that the field had no name: +6 was reachable only as
> `EXIT_PARM_HEADER`'s `reserved`, and the exit read `parm->reserved`.
> That fetched the right two bytes, so the exit behaved correctly, but
> the block's only mandatory field was labelled "reserved".
>
> **Assessment:** RESOLVED.  `ftp_chkcmd_parm` uses
> `EXIT_PARM_HEADER_6` and declares `uint16_t ftpcmd; /* +6 */`.  The
> exit compares against `FTP_CMD_DELE`, `FTP_CMD_STOR` and
> `FTP_CMD_SITE` instead of bare literals.  Offsets unchanged — 6 + 2
> is the same 8 bytes the macro produced, so `ftpuser` stays at +8.

**C2 — Internal network check: 10.x.x.x only**
> The ASM checks `CLC FTPCLIENT(1),=X'0A'` — first byte of client IP == 10.
> The C checks `parm->ftpclient.addr[0] != 10`.
> This only considers 10.x.x.x as "internal."  If the site uses 172.16.x.x
> or 192.168.x.x internal ranges, those are not protected.
> **Assessment:** Functional match to ASM. Site-specific policy question.
> Verify internal network range with network team before deployment.

---

**CX — `EXIT_PARM_HEADER` 2-byte shift — does not apply to this exit**
> An earlier version of this concern named `tcpsec_parm` and
> `ipflt_parm`.  **This exit uses neither.**  `FTCHKCMD` takes a
> `struct ftp_chkcmd_parm`, which was never in the layout baseline: its
> offsets already matched the assembler at every field
> (`ftpuser` +8, `ftpclient` +16, `ftpdsn` +40, `ftpreasn` +340,
> `ftpreply` +344), all re-verified against the instruction stream.
>
> `tcpsec_parm` (EZACSEC/EZASSEC) and `ipflt_parm` (EZBIPMXT) *were*
> shifted by 2 and are now corrected, but no assembler in this
> repository addresses either one, so their layouts rest on their own
> offset comments.
>
> **Assessment:** Not applicable to FTCHKCMD.  See
> `docs/layout-findings.md` finding 4 for the two structs that do carry
> it.

## 7. Sign-off Checklist

- [ ] All in-scope ASM labels mapped to C equivalents
- [x] C1 `ftpcmd` is 2 bytes at +6 (`CLC 6(2,R10),=H'nn'`) and now a named field
- [ ] C2 internal IP range policy confirmed with network team
- [ ] FTP reply codes (550, 500) verified against FTP server documentation
- [ ] `FTP_FUNC_CMD` value (2) verified against FTPxx documentation
- [ ] `verify_structs.c` compiles clean on target z/OS level
- [ ] Runtime tested for DELE, STOR, SITE commands
- [ ] FTP server restarted after exit update
- [x] CX EXIT_PARM_HEADER shift does not affect `ftp_chkcmd_parm` (offsets ASM-verified)
      (docs/layout-findings.md finding 4)
- [ ] Second reviewer sign-off: ___________________  Date: ________
