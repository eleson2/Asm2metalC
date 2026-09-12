# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This repository supports migration of IBM z/OS system exits from HLASM (High Level Assembler) to IBM Metal C. Metal C is a freestanding C environment (`xlc -qmetal`) with no Language Environment (LE) runtime — no standard C library, no malloc, no printf.

## Repository Structure

```
asm/           - Source assembler exits, organized by product (JES2, RACF, IMS, etc.)
asm/stubs/     - HLASM stubs for services Metal C cannot express (e.g. RACROUTE);
                 link-edited with the converted exit that calls them
converted/     - Metal C conversions of the assembler exits (target output)
includes/      - Custom Metal C header framework (metalc_base.h + product-specific headers)
examples/      - Standalone Metal C exit examples (not derived from asm/ sources)
docs/          - Conversion guides and steering documents for AI-assisted translation
```

## Compilation

Metal C files are compiled on z/OS with IBM XL C:
```
xlc -qmetal -S -qlist myexit.c
```
- `-qmetal` — Enable Metal C (no LE dependencies)
- `-S` — Generate assembler listing for verification
- `-qlist` — Generate compiler listing
- `-q64` — For 64-bit mode (optional)

Compilation happens on a target z/OS system; `docs/zos-build/ASMCBLD.jcl`
builds one exit and `docs/zos-build/README.md` covers the first build.

Everything that can be verified without a mainframe runs from `make`:

```
make            # all offline checks (what CI runs)
make layout     # struct offsets match their header comments
make conform    # converted exits obey the rules below
make lint       # host compiler parses the framework, layout assertions hold
```

`make lint` compiles the whole header set with `-DMETALC_HOST_LINT`, which
elides the inline assembler (see `metalc_svc.h`) so an ordinary compiler can
check struct layouts and types. It does not validate the assembler, the z/OS
macro expansion, or whether the headers match the real DSECTs.

## Header Framework

Every converted file must include:
```c
#include "metalc_base.h"
#include "metalc_<product>.h"   /* e.g., metalc_jes2.h, metalc_cics.h */
#include "metalc_saf.h"         /* only when the exit calls SAF/RACROUTE */
```

`metalc_base.h` provides:
- Fixed-width types (`int32_t`, `uint16_t`, etc.) — never use raw `int`/`long`
- Inline replacements for libc: `memcpy_inline`, `memset_inline`, `memcmp_inline`, `strlen_inline`
- Bit manipulation macros: `TM_ALL`, `TM_ANY`, `TM_NONE`, `OI`, `NI`, `XI`
- Return code constants: `RC_OK` (0), `RC_WARNING` (4), `RC_ERROR` (8), `RC_SEVERE` (12), `RC_CRITICAL` (16)
- System services: `wto_write`, `wto_simple`, `wto_security`, `getmain`, `freemain`
- Pointer helpers: `ADDR_AT_OFFSET`, `PTR_AT_OFFSET`
- `EXIT_PARM_HEADER` (8-byte) and `EXIT_PARM_HEADER_6` (6-byte, product
  owns +6) macros for standard parameter block layout

`metalc_svc.h` (pulled in by `metalc_base.h`, never included directly) holds
every z/OS system service and is the only file in the framework containing
inline assembler:
- `wto_write`, `wto_simple`, `wto_security`, `wto_alert`, `wto_important` — WTO
- `get_tod_clock`, `get_time_hundredths` — STCK
- `getmain` / `freemain` — GETMAIN R / FREEMAIN R (unconditional; abends on failure)
- `storage_obtain` / `storage_release` — STORAGE OBTAIN/RELEASE, `COND=YES`,
  `LOC=ANY`.  Use these when the ASM coded `STORAGE OBTAIN`, or whenever the
  exit must handle a storage shortage rather than abend.

`metalc_saf.h` wraps SAF/RACROUTE, backed by `asm/stubs/SAFAUTH.asm`:
- `saf_auth(class, entity, userid, attr, detail)` — general REQUEST=AUTH
- `saf_auth_appl(applid, userid)` — the common sign-on check
Both apply the default-deny rule.  Exits that use it must link-edit the stub.

## Conversion Rules

### Mandatory Patterns

1. **Prolog/Epilog** — Every exit function needs explicit linkage pragmas:
   ```c
   #pragma prolog(MYEXIT, "SAVE(14,12),LR(12,15)")
   #pragma epilog(MYEXIT, "RETURN(14,12)")
   ```

2. **Structure packing** — All control block structs must use `#pragma pack(1)` with offset comments:
   ```c
   #pragma pack(1)
   struct my_block {
       char     id[4];    /* +0 */
       uint16_t len;      /* +4 */
   };
   #pragma pack()
   ```

3. **Return codes** — Use `RC_*` constants or product-specific macros (e.g., `JES2_RC_CONTINUE`), never literal integers.

4. **Bit manipulation** — Map assembler TM/OI/NI to the `TM_ALL`/`OI`/`NI` macros from `metalc_base.h`.

5. **Standard EXIT_PARM_HEADER** — Use this macro when the parameter block
   starts with `(work area, function code, flags, reserved)` at offsets +0
   through +7.  **Prove +6 belongs to the header before you do.**  Several
   products put a real 2-byte field there (DB2 `DSN3@ATH` privilege, FTP
   command code); for those use `EXIT_PARM_HEADER_6`, which stops at +5, and
   declare the +6 field yourself.  Picking the wrong variant shifts every
   later field by 2 bytes.  Where the product owns +5 as well, declare all
   the header fields explicitly.  See `docs/layout-findings.md` finding 4.

6. **Register mapping comment** — Document the register-to-variable mapping at function entry:
   ```c
   /* Register Mapping:
    * R1  = parm (struct my_parm *)
    * R10 = jct  (struct jct *)
    */
   ```

7. **No static writable data** — Reentrant exits must not use static or global writable variables. Static `const` tables are allowed.

8. **No inline assembler in exit source** — a converted `.c` file must contain
   no `__asm`. Every system service goes through `metalc_svc.h`; anything it
   does not cover gets a new wrapper there, or an HLASM stub in `asm/stubs/`
   called via `#pragma linkage(name, OS)`. The invariant:

   ```
   grep -rl __asm converted/ includes/ examples/   # -> includes/metalc_svc.h only
   ```

   Look every service macro up in `docs/system-services-catalog.md`; it also
   carries the procedure for adding a wrapper or stub. If a service cannot be
   implemented, the conversion is **BLOCKED** — report it, do not inline
   `__asm` and do not leave a placeholder that returns success.

   This exists because inline assembler in an exit is where placeholders hide.
   `__asm(" XR 15,15")` standing in for a RACROUTE reads like working code and
   made an IMS sign-on exit allow every user. A stub that has not been written
   yet fails the link edit instead of shipping.

### AMODE 64

If the source contains `AMODE 64` or 64-bit register instructions (`LG`, `STG`,
`LGR`, `LGHI`), compile with `xlc -qmetal -q64`.  Pointer fields in structs
become 8 bytes (`uint64_t`).  See `docs/amode64-exits.md`.

### Exit Chaining

Initialize the return code variable to the **neutral pass-through RC** for the
product, not a reject RC.  Only assign a definitive RC when the exit has an
explicit decision for this invocation.  See `docs/exit-chaining.md` for the
neutral RC constant per product.  **VTAM exception**: neutral RC is
`VTAM_LY_DEFER` (8), not 0.

### Linkage Convention Detection

Before writing `#pragma prolog/epilog`, detect the linkage family from the ASM source:

| ASM keyword | Prolog | Epilog |
|---|---|---|
| `BAKR R14,0` | `"BAKR 14,0"` | `"PR"` |
| `$SAVE` / `$MODULE` (JES2) | `"SAVE(14,12),LR(12,15)"` | `"RETURN(14,12)"` |
| `SAVE (14,12)` or `STM R14,R12` | `"SAVE(14,12),LR(12,15)"` | `"RETURN(14,12)"` |

See `docs/asm-linkage-conventions.md` for details.

### RACROUTE / SAF Calls

Never stub a `RACROUTE` call with `XR 15,15` (always-allow).  Use the assembler
stub strategy in `docs/racroute-metalc-patterns.md`.  Default-deny when SAF is
unavailable (RC=8 → deny).

### What NOT to Convert Automatically

Flag these for manual review: self-modifying code, `EX Rn,(Rm)` with variable-target
(note: `EX Rn,fixed_label` is translatable), channel programs (EXCP), cross-memory
services (PC/PT instructions), and AR-mode code.

See `docs/complex-asm-patterns.md` for EX disambiguation and other complex patterns.

## Key Documents

- `docs/ai-conversion-steering.md` — Authoritative rules for AI-assisted conversion (supersedes general guides where they conflict)
- `docs/asm-to-metalc-general.md` — General translation reference (entry points, data types, control flow patterns)
- `docs/asm-to-c-conversion-guide.md` — DSECT-to-struct mapping, macro expansion, common conversion mistakes
- `docs/asm-field-evidence.md` — **What each instruction proves about a field's offset, width and type.** Build the struct from the instruction stream when the DSECT is unavailable; rules for when the header and the assembler disagree (the header loses)
- `docs/pre-conversion-triage.md` — Assessment checklist to complete before any conversion begins
- `docs/partial-scope-policy.md` — Policy for partial conversions: when allowed, documentation required, deployment gates
- `docs/system-services-catalog.md` — **HLASM macro → C call lookup for every system service**, plus how to add a wrapper (`metalc_svc.h`) or a stub (`asm/stubs/`) when one is missing
- `docs/asm-linkage-conventions.md` — BAKR/PR vs SAVE/RETURN vs JES2 $SAVE; correct `#pragma prolog/epilog` for each
- `docs/racroute-metalc-patterns.md` — RACROUTE MF=(E,list) assembler stub, three-way RC (0/4/8) handling, default-deny rule; Strategy A (inline SVC 119) is banned
- `docs/hlasm-structured-programming.md` — IF/ELSE/ENDIF, DO/ENDDO, SELECT/WHEN macro recognition and C equivalents
- `docs/complex-asm-patterns.md` — EX disambiguation (fixed vs. variable target), TRT, ICM, MVCL, BCT, BAS, packed decimal, STCK
- `docs/copy-macro-dependency.md` — COPY member and macro library dependency detection, resolution strategies
- `docs/reentrant-ification-policy.md` — Converting non-reentrant ASM (static DS fields) to reentrant Metal C; required documentation
- `docs/amode64-exits.md` — AMODE 64 detection, `-q64` compile flag, pointer type rules, struct layout differences, affected products (IMS 15+, MQ 9.3+, WLM)
- `docs/exit-chaining.md` — Chain-safe RC initialization, per-product neutral RC table, VTAM DEFER exception, chain position documentation
- Product-specific guides (full set):
  - `docs/asm-to-metalc-acf2.md` — CA ACF2
  - `docs/asm-to-metalc-cics.md` — CICS Transaction Server
  - `docs/asm-to-metalc-db2.md` — DB2 for z/OS
  - `docs/asm-to-metalc-dfsms.md` — DFSMS (Dynamic Allocation)
  - `docs/asm-to-metalc-ims.md` — IMS/TM and IMS/DB
  - `docs/asm-to-metalc-jes2.md` — JES2
  - `docs/asm-to-metalc-mq.md` — IBM MQ for z/OS
  - `docs/asm-to-metalc-netview.md` — IBM NetView
  - `docs/asm-to-metalc-opc.md` — OPC/TWS z/OS
  - `docs/asm-to-metalc-racf.md` — RACF (exit families, the RC=4 bypass hazard, password handling)
  - `docs/asm-to-metalc-sa.md` — IBM System Automation
  - `docs/asm-to-metalc-smf.md` — SMF Record Exits
  - `docs/asm-to-metalc-tcpip.md` — z/OS Communications Server TCP/IP
  - `docs/asm-to-metalc-vtam.md` — VTAM / SNA

## Conversion Pipeline (AI-Assisted)

Four specialized agents in `.claude/agents/` support the end-to-end workflow:

| Agent | When to use |
|-------|-------------|
| `asm-pre-analyzer` | **First step.** Reads an ASM file and produces a Pre-Conversion Analysis Report (entry points, DSECTs, macros, complexity flags, register map). |
| `asm-to-metalc-converter` | **Second step.** Performs the actual translation using the analysis report + steering docs. Writes to `converted/<PRODUCT>/<MODULE>.c`. |
| `metalc-verifier` | **Third step.** Reads ASM + C side-by-side; produces a draft Verification Matrix in `docs/verification-matrices/`. |
| `product-onboarder` | **Before a new product.** Creates `includes/metalc_<product>.h` and `docs/asm-to-metalc-<product>.md` for a product not yet in the framework. |

**Recommended workflow for a new conversion:**
1. Fill in `docs/pre-conversion-triage.md` (or run `asm-pre-analyzer`).
2. Run `asm-to-metalc-converter` with the triage output.
3. Run `metalc-verifier` to produce the draft matrix.
4. Human reviewer completes the matrix sign-off.
5. If partial scope, follow `docs/partial-scope-policy.md` before deployment.
