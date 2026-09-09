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

Note: `parm->func` = `ftpfunc` (init/cmd/term); `parm->flags` = `ftpflags` (SSL/anon); `parm->reserved` = `ftpcmd` (command code).

---

## 3. Logic Equivalence Table

| ASM Label | ASM Instruction(s) | C Equivalent | Verified? | Concern # |
|-----------|--------------------|--------------|-----------|-----------|
| Entry | `LR R12,R15` | `#pragma prolog(FTCHKCMD, "SAVE(14,12),LR(12,15)")` | Yes | — |
| Check cmd func | `CLI FTPFUNC,2` / `BNE RETURN` | `if (parm->func != FTP_FUNC_CMD) return FTP_RC_CONTINUE` | Yes | — |
| Check DELE (23) | `CLI FTPCMD,23` / `BNE CHKSTOR` | `if (parm->reserved == 23)` | Yes | C1 |
| Check SYS1 prefix | `CLC FTPDSN(5),=CL5'SYS1.'` / `BNE RETURN` | `if (match_prefix(parm->ftpdsn, "SYS1.", 5))` | Yes | — |
| Set reply 550 | `MVC FTPREPLY(2),=H'550'` | `parm->ftpreply = FTP_REPLY_NOACCESS` | Yes | — |
| Set reason 100 | `MVC FTPREASN(4),=F'100'` | `parm->ftpreasn = 100` | Yes | — |
| Reject DELE | `LA R15,4 / BR R14` | `return FTP_RC_REJECT` | Yes | — |
| Check STOR (15) | `CLI FTPCMD,15` / `BNE CHKSITE` | `else if (parm->reserved == 15)` | Yes | — |
| Log upload | Build WTO | `msg_append_str/field` + `wto_write(PROG_INFO)` | Yes | — |
| Check SITE (29) | `CLI FTPCMD,29` / `BNE RETURN` | `else if (parm->reserved == 29)` | Yes | — |
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

**C1 — `parm->reserved` as command code (`ftpcmd`)**
> The FTP exit parameter block reuses the 2-byte `reserved` field from
> `EXIT_PARM_HEADER` as the FTP command code (`ftpcmd`).  The comparison
> `parm->reserved == 23` tests the full 2-byte field against a small integer.
> The ASM `CLI FTPCMD,23` tests only 1 byte.  If `ftpcmd` occupies only
> the first byte of `reserved`, the C comparison must cast to `uint8_t`:
> `*(uint8_t *)&parm->reserved == 23`.
> **Assessment:** Open — verify `ftpcmd` field width (1 byte vs. 2 bytes).

**C2 — Internal network check: 10.x.x.x only**
> The ASM checks `CLC FTPCLIENT(1),=X'0A'` — first byte of client IP == 10.
> The C checks `parm->ftpclient.addr[0] != 10`.
> This only considers 10.x.x.x as "internal."  If the site uses 172.16.x.x
> or 192.168.x.x internal ranges, those are not protected.
> **Assessment:** Functional match to ASM. Site-specific policy question.
> Verify internal network range with network team before deployment.

---

**CX — EXIT_PARM_HEADER structs are shifted by 2 bytes**
> `tcpsec_parm / ipflt_parm` documents its first field after `EXIT_PARM_HEADER` at +6,
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
- [ ] C1 `ftpcmd` field width (1 byte vs. 2 bytes in `reserved`) resolved
- [ ] C2 internal IP range policy confirmed with network team
- [ ] FTP reply codes (550, 500) verified against FTP server documentation
- [ ] `FTP_FUNC_CMD` value (2) verified against FTPxx documentation
- [ ] `verify_structs.c` compiles clean on target z/OS level
- [ ] Runtime tested for DELE, STOR, SITE commands
- [ ] FTP server restarted after exit update
- [ ] CX EXIT_PARM_HEADER 2-byte shift resolved against vendor docs
      (docs/layout-findings.md finding 4)
- [ ] Second reviewer sign-off: ___________________  Date: ________
