# AI Conversion Guide: Project Assessment & Critical Issue Summary

This document provides a concise assessment of the **Asm2C** repository for AI models (including Claude and other LLMs). It details the framework's purpose, strengths, and **especially highlights all technical issues, hazards, and limitations** that AI models must navigate during HLASM-to-Metal C conversion.

---

## 1. Project Purpose & Scope

The **Asm2C** project supports the translation of IBM z/OS High Level Assembler (HLASM) system exits into IBM Metal C (`xlc -qmetal`). 

Metal C is a freestanding C environment with **no Language Environment (LE) runtime** (no `libc`, no `malloc`, no `printf`, no standard stack linkage). The objective of this codebase is to provide custom headers, strict translation rules, offline verification tools, and steering docs so AI models can translate HLASM exits to Metal C **without errors or security vulnerabilities**.

---

## 2. Framework Strengths & Mandates

AI agents operating in this repository **must enforce** the following safety invariants:

* **Zero Inline `__asm` in Converted Exits**: Converted `.c` files in `converted/` must contain **no `__asm`**. All system services (`WTO`, `GETMAIN`, `STORAGE`, `STCK`) must use central wrappers in [includes/metalc_svc.h](file:///G:/projects/Asm2C/includes/metalc_svc.h) or HLASM stubs in [asm/stubs/](file:///G:/projects/Asm2C/asm/stubs). *Never insert inline assembly placeholders.*
* **SAF / Security Default-Deny**: Security checks in [includes/metalc_saf.h](file:///G:/projects/Asm2C/includes/metalc_saf.h) (`saf_auth`, `saf_auth_appl`) must enforce default-deny (RC=8 on failure or missing SAF classes). Never allow RC=4 (RACF inactive/no decision) to grant access.
* **Reentrancy**: Converted exits must be reentrant (no static writable variables).
* **Instruction Stream as Evidence**: Always follow [docs/asm-field-evidence.md](file:///G:/projects/Asm2C/docs/asm-field-evidence.md): *"Make the header fit the assembler. Never make the assembler fit the header."* Use explicit displacement instructions (`CLI`, `CLC`, `MVC`, `L`, `ST`) to prove field offset, width, and type.
* **Offline Verification Suite**: Run `make` (= `make check`: `gen_struct_asserts.py --check`, `check_layout.py`, `check_conformance.py`, `lint_host.sh`) to validate mechanical rules, `struct` packing, and host-compiler syntax off-platform.
  **Green is not clean.** `check_layout.py` carries a baseline of known mismatches in [tools/layout_known_issues.txt](file:///G:/projects/Asm2C/tools/layout_known_issues.txt); their assertions are emitted commented-out in `tests/verify_structs.c`, so a green run says nothing about them. The baseline is down to **3 entries** (finding 1's `ascb` fields) from 117. A baseline entry is a deferred decision, not an accepted defect — check it before trusting a green run, and read [docs/layout-findings.md](file:///G:/projects/Asm2C/docs/layout-findings.md).

---

## 3. Critical Issues, Hazards & Limitations (AI Must Avoid)

AI models must be specifically aware of the following technical issues and limitations in this repository:

### Issue 1: `EXIT_PARM_HEADER` Macro Layout Collisions (Offset +6 / +5 Overwrites)
* **The Hazard**: The `EXIT_PARM_HEADER` macro in [includes/metalc_base.h](file:///G:/projects/Asm2C/includes/metalc_base.h#L65) defines an 8-byte layout: `work` (+0, 4B), `func` (+4, 1B), `flags` (+5, 1B), `reserved` (+6, 2B).
* **Where It Fails**: Subsystem parameter blocks like DB2 (`DSN3ATH`), TCP/IP (`FTCHKCMD`), and IMS (`ims_flgx_parm`) carry **real product fields** at offset +6 or +5 rather than reserved space.
* **Impact**: Applying `EXIT_PARM_HEADER` blindly forces a **2-byte offset shift** across all subsequent fields in the `struct`, corrupting data reads.
* **Status - FIXED.** This is [layout-findings.md](file:///G:/projects/Asm2C/docs/layout-findings.md) Finding 4, and the assembler in this repository settled it: `DSN3ATH.asm` and `FTCHKCMD.asm` address their parm blocks by explicit base-displacement (`CLI 4(R10),3`, auth ID at `8(R10)`, client IP at `16(R10)`), and `FTCHKCMD.asm` reads +6 directly as a halfword three times (`CLC 6(2,R10),=H'23'`). **The common header is 6 bytes** (`work` +0, `func` +4, `flags` +5); the macro's `uint16_t reserved` at +6 was an invention. The nine structs' offset comments were right; the macro was wrong *for them*. It remains correct for the other 25 structs that document their next field at +8 - so the fix was per-struct, never a single edit to `metalc_base.h`.
* **What changed**: `metalc_base.h` now also provides **`EXIT_PARM_HEADER_6`**, which stops at +5 and leaves +6 to the product. The nine structs use it (`ims_flgx_parm` declares its header field by field, because it owns +5 too). `check_layout.py` and `gen_struct_asserts.py` know the width of each variant, so choosing the wrong one now fails a check instead of silently shifting every later field. 114 of the 117 baseline entries are gone.
* **Corrected structs** (9): `db2_xac_parm`, `db2_ath_parm`, `db2_sgn_parm`, `db2_edit_parm`, `db2_field_parm`, `ims_flgx_parm`, `sa_rec_parm`, `ipflt_parm`, `tcpsec_parm`. **Only `db2_ath_parm` has assembler evidence** (`DSN3ATH.asm`); the other seven are now self-consistent but **unsourced** — nothing here addresses `EZACSEC`, `EZBIPMXT`, `DSN3@SGN`, `DSN3@XAC`, the DB2 edit/field procedures, `AOFEXC30` or `DFSFLGX0`. Self-consistency is not provenance.
* **What it fixed in a live exit**: [converted/DB2/DSN3ATH.c](file:///G:/projects/Asm2C/converted/DB2/DSN3ATH.c) is a DB2 authorization exit and every field it read was 2 bytes out — `athauth` at +10 instead of +8 (so the `SYSADM` comparison read misaligned bytes), `athobj` at +18 instead of +16, and `athreasn` written at +62 instead of +60. Correcting the struct fixed the exit; no source change was needed, because the field names were already right.
* **`ftp_chkcmd_parm` was a different bug.** Its offsets already matched the assembler and it was never in the baseline — but +6 had **no field at all**, so [converted/TCPIP/FTCHKCMD.c](file:///G:/projects/Asm2C/converted/TCPIP/FTCHKCMD.c) reached the FTP command code through `parm->reserved`. It read the right two bytes, so the exit worked; the block's one mandatory field was just named "reserved". It now declares `uint16_t ftpcmd; /* +6 */` and compares against `FTP_CMD_DELE` / `FTP_CMD_STOR` / `FTP_CMD_SITE`.
* **AI Action**: Before using `EXIT_PARM_HEADER`, prove from the instruction stream or DSECT that +6 belongs to the header. If a product field sits at **+6**, use `EXIT_PARM_HEADER_6` and declare that field yourself under the vendor's own name; if the product owns **+5** as well, declare every header field explicitly. One instruction decides it — `CLC 6(2,R10),=H'23'` proves the product owns +6, while `CLI 4(R10),3` proves nothing either way. Never renumber the offset comments to match the macro - that is making the assembler fit the header. When no instruction touches +6, you have no evidence: say so in the matrix instead of picking the variant that makes the numbers line up.

### Issue 2: AMODE 31 vs AMODE 64 Layout Shifts
* **The Hazard**: Current C headers document 31-bit offsets with 4-byte pointers. Under AMODE 64 (`xlc -qmetal -q64`), raw C pointers (`void *`, `struct foo *`) expand to 8 bytes.
* **Impact**: Structs containing raw pointer fields expand in size under `-q64`, shifting all subsequent field offsets out of alignment with 64-bit z/OS control blocks.
* **Scope**: This is [layout-findings.md](file:///G:/projects/Asm2C/docs/layout-findings.md) Finding 3 - **no product header carries the guards**, so no exit can currently be built `-q64` with correct struct layouts (affects IMS 15+, MQ 9.3+, WLM). `make layout64` is informational and is not gated by `make check`; it mismatches on every pointer-bearing struct **by design**.
* **Widest blast radius**: `EXIT_PARM_HEADER` itself opens with a raw `void *work`, so under `-q64` all 34 structs using the macro shift by 4 bytes from +4 onward.
* **AI Action**: Use explicit fixed-width integers (`uint32_t` for 31-bit addresses, `uint64_t` for 64-bit) or the `#ifdef __LP64__` guards `docs/amode64-exits.md` specifies. Run `make layout64` to measure the remaining work.

### Issue 3: Self-Referential Layout Verification Hazard
* **The Hazard**: `make layout` and `tests/verify_structs.c` verify struct offsets against the **comments inside header files**, NOT against actual z/OS memory dumps or DSECTs.
* **Impact**: As demonstrated by the `JCTJOBID` defect ([docs/asm-field-evidence.md](file:///G:/projects/Asm2C/docs/asm-field-evidence.md#L195-L270)), if both the struct definition and header comments are wrong, `make layout` will pass green even while the code reads wrong memory at runtime.
* **Worked example, now FIXED** ([layout-findings.md](file:///G:/projects/Asm2C/docs/layout-findings.md) Finding 5): `jct.jctjobid` was declared `uint16_t` where the assembler proves `CL8` — `HASPEX20.asm` does `CLI JCTJOBID,C'J'` (byte 0 is character data) and `HASPEX02.asm` does `MVC MSGJOBID,JCTJOBID` with `MSGJOBID DS CL8` (8 bytes). Two shipped conversions were broken and the harness certified both, asserting `VERIFY_OFFSET(jct, jctjname, 6)`:
  * `converted/JES2/HASPEX20.c` - `jct->jctjobid == 'J'` compared 16 bits against `0x00D1`, true only for job number 209. **The exit's only function never happened.** Now `jctjobid[0] == 'J'`.
  * `converted/JES2/HASPEX02.c` - copied 8 bytes from `jct->jctid`, the 4-byte `'JCT '` eyecatcher, over-reading a `char[4]` by 4. Now reads `jct->jctjobid`.
  * `jctjobid` is `char[8]`, `jctjname` is at +12, and the assertion says 12. **The rest of the JCT is still unsourced** — no exit here addresses `jctjclas` or anything later by displacement, so those fields were shifted by 6 to preserve the documented relative layout. That makes the struct less wrong, not sourced; the header now carries a provenance comment saying so.
* **AI Action**: Do not rely solely on `make layout`. Always cross-check struct definitions against the instruction stream evidence ledger (`CLI`, `CLC`, `MVC` displacements). Treat a struct that no exit addresses by displacement as **unsourced**, not as verified.

### Issue 4: Offline Harness Cannot Expand HLASM Macros or Run `xlc`
* **The Hazard**: The offline test harness (`make check`, `lint_host.sh`) uses host compilers (`clang`/`gcc`) and Python off-platform. It cannot expand z/OS HLASM macros (`$MODULE`, `RACROUTE`, `WTO`) or invoke the z/OS IBM C compiler (`xlc -qmetal`).
* **Impact**: A converted file that passes `make check` offline is structurally sound, but is **not guaranteed** to compile or link on z/OS.
* **AI Action**: Acknowledge that offline checks are necessary but insufficient; final deployment requires z/OS compilation via [docs/zos-build/ASMCBLD.jcl](file:///G:/projects/Asm2C/docs/zos-build/ASMCBLD.jcl).

### Issue 5: Untranslatable HLASM Patterns
* **The Hazard**: Certain complex HLASM constructs cannot be safely translated 1-to-1 into freestanding C:
  * Self-modifying code (modifying instruction immediate bytes).
  * EXCP channel programs (CCW sequences).
  * `EX` (Execute) with a **variable** target register - `EX Rn,(Rm)`. Note that `EX Rn,fixed_label` **is** translatable.
  * Access Register (AR) mode and cross-memory (`PC`/`PT`) instructions.
* **Not on this list - packed decimal is translatable.** [complex-asm-patterns.md](file:///G:/projects/Asm2C/docs/complex-asm-patterns.md) §7 and its §11 translatability table mark `CVB`/`CVD` and the whole `AP`/`SP`/`MP`/`DP`/`CP`/`ZAP` family **Yes**: convert at the edges with `packed_to_binary()` / `binary_to_packed()` from `metalc_svc.h` and compute in binary C. `PACK`/`UNPK`/`ED`/`EDMK` have no wrapper *yet* - the action there is to **add one** per `system-services-catalog.md` §4, not to declare the conversion blocked. (`CVB` abends S0C7/S0C9 on an invalid field with no return code, so validate untrusted packed input first.)
* **AI Action**: Flag the genuinely untranslatable constructs above during pre-conversion triage (`asm-pre-analyzer`) and report the conversion as **BLOCKED** for human architectural redesign. A missing service wrapper is not a BLOCKED conversion - it is a wrapper to write.

### Issue 6: Partial Header Mapping & Unmapped DSECT Fields
* **The Hazard**: Product headers in `includes/metalc_*.h` map only a subset of DSECT fields required by sample exits.
* **Impact**: When converting new exits, missing fields may cause AI models to invent fields or perform invalid pointer casting.
* **AI Action**: Use the `product-onboarder` workflow to update `includes/metalc_<product>.h` with missing fields derived from DSECT evidence before attempting the conversion.

---

## 4. AI Pipeline & Execution Checklist

When translating an exit, AI models should strictly execute this 4-step pipeline:

```
[1. asm-pre-analyzer]  -->  [2. asm-to-metalc-converter]  -->  [3. metalc-verifier]  -->  [4. Human Matrix Sign-Off]
```

Before marking any conversion complete, verify:
- [ ] Converted `.c` file has **zero `__asm`** blocks.
- [ ] Linkage pragmas (`#pragma prolog/epilog`) match original linkage (`BAKR`, `SAVE/RETURN`, `$SAVE`).
- [ ] Every `struct` is enclosed in `#pragma pack(1)` and `#pragma pack()`.
- [ ] No literal integers used for return codes (must use `RC_*` or product constants).
- [ ] No standard `libc` headers or functions referenced (`printf`, `malloc`, `memcpy`, etc.).
- [ ] Storage references in HLASM match `sizeof` and offset of C struct fields.
- [ ] Verification matrix drafted in `docs/verification-matrices/`.
- [ ] `make` (all offline checks) passes with 0 **new** errors - and no struct the exit touches appears in `tools/layout_known_issues.txt`.

---

## 5. Related Steering Documents

- [CLAUDE.md](file:///G:/projects/Asm2C/CLAUDE.md) — Main developer guidelines and conversion rules
- [docs/ai-conversion-steering.md](file:///G:/projects/Asm2C/docs/ai-conversion-steering.md) — Authoritative AI steering rules
- [docs/asm-field-evidence.md](file:///G:/projects/Asm2C/docs/asm-field-evidence.md) — Extracting struct layouts from HLASM instruction stream
- [docs/system-services-catalog.md](file:///G:/projects/Asm2C/docs/system-services-catalog.md) — HLASM macro to Metal C function mapping
- [docs/racroute-metalc-patterns.md](file:///G:/projects/Asm2C/docs/racroute-metalc-patterns.md) — SAF / RACROUTE security patterns
- [docs/layout-findings.md](file:///G:/projects/Asm2C/docs/layout-findings.md) — **authoritative record for Issues 1, 2 and 3**: what the layout tools found, which findings are resolved, which fixes are still unapplied, and how the baseline works
- [docs/complex-asm-patterns.md](file:///G:/projects/Asm2C/docs/complex-asm-patterns.md) — per-instruction translatability table (Issue 5)
