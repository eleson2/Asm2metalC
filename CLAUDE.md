# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This repository supports migration of IBM z/OS system exits from HLASM (High Level Assembler) to IBM Metal C. Metal C is a freestanding C environment (`xlc -qmetal`) with no Language Environment (LE) runtime — no standard C library, no malloc, no printf.

## Repository Structure

```
asm/           - Source assembler exits, organized by product (JES2, RACF, IMS, etc.)
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

There are no build scripts in this repository; compilation occurs on a target z/OS system.

## Header Framework

Every converted file must include:
```c
#include "metalc_base.h"
#include "metalc_<product>.h"   /* e.g., metalc_jes2.h, metalc_cics.h */
```

`metalc_base.h` provides:
- Fixed-width types (`int32_t`, `uint16_t`, etc.) — never use raw `int`/`long`
- Inline replacements for libc: `memcpy_inline`, `memset_inline`, `memcmp_inline`, `strlen_inline`
- Bit manipulation macros: `TM_ALL`, `TM_ANY`, `TM_NONE`, `OI`, `NI`, `XI`
- Return code constants: `RC_OK` (0), `RC_WARNING` (4), `RC_ERROR` (8), `RC_SEVERE` (12), `RC_CRITICAL` (16)
- System services: `wto_write`, `wto_simple`, `wto_security`, `getmain`, `freemain`
- Pointer helpers: `ADDR_AT_OFFSET`, `PTR_AT_OFFSET`
- `EXIT_PARM_HEADER` macro for standard parameter block layout

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

5. **Standard EXIT_PARM_HEADER** — Use this macro when the parameter block starts with `(work area, function code, flags, reserved)` at offsets +0 through +7.

6. **Register mapping comment** — Document the register-to-variable mapping at function entry:
   ```c
   /* Register Mapping:
    * R1  = parm (struct my_parm *)
    * R10 = jct  (struct jct *)
    */
   ```

7. **No static writable data** — Reentrant exits must not use static or global writable variables. Static `const` tables are allowed.

### What NOT to Convert Automatically

Flag these for manual review: self-modifying code, `EXECUTE` with variable targets, channel programs (EXCP), cross-memory services (PC/PT instructions), and AR-mode code.

## Key Documents

- `docs/ai-conversion-steering.md` — Authoritative rules for AI-assisted conversion (supersedes general guides where they conflict)
- `docs/asm-to-metalc-general.md` — General translation reference (entry points, data types, control flow patterns)
- `docs/asm-to-c-conversion-guide.md` — DSECT-to-struct mapping, macro expansion, common conversion mistakes
- Product-specific guides: `docs/asm-to-metalc-smf.md`, `docs/asm-to-metalc-acf2.md`, `docs/asm-to-metalc-jes2.md`
