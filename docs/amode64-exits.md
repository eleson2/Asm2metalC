# AMODE 64 Exits — Metal C Conversion Guide

An increasing number of z/OS exit points are defined as AMODE 64 interfaces,
particularly in IMS 15+, MQ 9.3+, newer z/OS Workload Manager exits, and
some z/OS Communications Server extensions.  This document covers the
differences from AMODE 31 and the correct Metal C patterns for each.

---

## 1. How to Detect an AMODE 64 Exit

### From the ASM source

```asm
MYEXIT   CSECT
MYEXIT   AMODE 64        ← explicit AMODE 64 statement
MYEXIT   RMODE ANY
```

Also look for:
- 64-bit register operations: `LG`, `STG`, `AG`, `SG`, `LGR`, `LGFR`, `LGHI`
- 64-bit address loads: `LLGTR`, `LLGFR`, `LGF`
- Pointer arithmetic with 8-byte fields: `DS AD` (doubleword address)
- `BAKR R14,0` combined with `AMODE 64` (common in IMS 15 exits)
- `SAM64` instruction (switch to AMODE 64)

### From the product documentation

Each product's exit documentation states the AMODE requirement.  Examples:

| Product | Exit | AMODE |
|---------|------|-------|
| IMS 15+ | DFSWHU00 (IMS 15 version) | 64 |
| IMS 15+ | DFSMSCE0 (IMS 15 version) | 64 |
| MQ 9.3+ | CSQXLIB channel exits | 64 (optional; 31 still supported) |
| z/OS WLM | IWMXITP | 64 |
| DFSMS (newer) | Some DFSMS allocation exits | 64 |
| JES2 z/OS 3.1 | Some dynamic exits | 64 (check JESPARMS) |

When in doubt, check the product's Program Directory or Exit Reference for
the AMODE statement in the sample exit provided by IBM.

---


### 1.1 Detecting 64-bit mode in C

`metalc_base.h` defines `METALC_64` and every width in the framework keys off
it:

```c
#if defined(__LP64__) || defined(__64BIT__)
#define METALC_64  1
#else
#define METALC_64  0
#endif
```

Both spellings are tested because IBM XL C defines `__64BIT__` for `-q64`,
while `__LP64__` is the common Unix spelling and appears in some
configurations.  **Confirm which one your compiler level actually defines
before relying on a `-q64` build** — if neither is defined, every width
silently stays 31-bit and the struct offsets are wrong with no diagnostic.
A one-line check settles it:

```c
#if !defined(__LP64__) && !defined(__64BIT__)
#error "-q64 build but no 64-bit macro defined; adjust METALC_64"
#endif
```

Prefer `METALC_64` over the raw macros in new code, so there is one place to
change if the spelling turns out to be different.

### 1.2 Types that widen with the addressing mode

These are not just pointer fields inside control blocks:

| Type | 31-bit | 64-bit | Why |
|---|---|---|---|
| `ptr_t` | `uint32_t` | `uint64_t` | Pointer-sized field in a mapped struct |
| `uintptr_t` | `unsigned int` | `unsigned long` | Must hold a pointer; a 32-bit one truncates |
| `size_t` | `unsigned int` | `unsigned long` | Must match the compiler's own definition |

`size_t` matters beyond arithmetic: `metalc_base.h` defines it, and the
compiler's freestanding `<stddef.h>` defines it too.  In 31-bit mode the two
agree by accident.  Under `-q64` they do not, so any translation unit that
pulls in both fails to compile.  This is why `metalc_verify.h` defines
`offsetof` itself rather than including `<stddef.h>` — otherwise
`tests/verify_structs.c` could not be built `-q64`, which is exactly the mode
the AMODE 64 modules need.

**Rule:** do not include `<stddef.h>`, `<stdint.h>`, or any other freestanding
header in framework code.  `metalc_base.h` owns these definitions.

## 2. What Changes in AMODE 64

### 2.1 Prolog and epilog pragmas

AMODE 64 uses the same prolog/epilog form as AMODE 31 for the string — the
compiler generates appropriate 64-bit register saves.  However, the R15 base
setup differs:

```c
/* AMODE 31 — 31-bit entry */
#pragma prolog(MYEXIT, "SAVE(14,12),LR(12,15)")
#pragma epilog(MYEXIT, "RETURN(14,12)")

/* AMODE 64 — 64-bit entry (SAVE pattern is the same; compiler uses 64-bit regs) */
#pragma prolog(MYEXIT, "SAVE(14,12),LR(12,15)")
#pragma epilog(MYEXIT, "RETURN(14,12)")
```

The pragma string itself does not change.  The difference is that you must
compile with `-q64`:

```
xlc -qmetal -q64 -S -qlist myexit.c
```

The `-q64` flag switches the compiler to 64-bit addressing mode.  Without it,
pointer fields are 4 bytes; with it, they are 8 bytes.

**For BAKR/PR linkage in AMODE 64** (common in IMS 15):

```c
#pragma prolog(MYEXIT, "BAKR 14,0")
#pragma epilog(MYEXIT, "PR")
```

Same pragma; `-q64` controls addressing.

### 2.2 Pointer types

In AMODE 31: pointers are 4 bytes (`uint32_t`, `void *` = 4 bytes).
In AMODE 64: pointers are 8 bytes (`uint64_t`, `void *` = 8 bytes).

`metalc_base.h` handles this automatically via:

```c
#ifdef __LP64__
typedef uint64_t  ptr_t;
#else
typedef uint32_t  ptr_t;
#endif
```

**Rules for struct fields that hold addresses:**

```c
#pragma pack(1)
struct my_parm {
    /* AMODE 31 */
    uint32_t  ptr31;      /* +0  31-bit pointer (4 bytes) */

    /* AMODE 64 */
    uint64_t  ptr64;      /* +0  64-bit pointer (8 bytes) */

    /* Portable: use ptr_t */
    ptr_t     ptrany;     /* +0  4 bytes in 31-bit, 8 bytes in 64-bit */
};
#pragma pack()
```

For control block structs that have **different layouts in 31 vs 64 bit**, define
two separate structs and select at compile time:

```c
#ifdef __LP64__
struct my_parm {
    uint64_t  ptr;     /* +0  8 bytes in 64-bit */
    uint64_t  len;     /* +8  */
};
#else
struct my_parm {
    uint32_t  ptr;     /* +0  4 bytes in 31-bit */
    uint32_t  len;     /* +4  */
};
#endif
```

### 2.3 High-memory pointers

In AMODE 64, addresses can exceed 2 GB.  The `0x80000000` last-parameter-list
bit convention (`IS_LAST_PARM`) used in AMODE 31 parm lists cannot be used
for 64-bit pointers.  Products that define AMODE 64 exit interfaces typically
use a parameter count field or a separate end-of-list indicator.

Check the product documentation for the specific parm list convention.

### 2.4 Register operations

The pre-analyzer must flag 64-bit register instructions and map them to 64-bit
C types:

| Instruction | C equivalent |
|---|---|
| `LG Rx,field` | `uint64_t rx = *(uint64_t *)&field;` |
| `STG Rx,field` | `*(uint64_t *)&field = rx;` |
| `AG Rx,field` | `rx += *(uint64_t *)&field;` |
| `LGR Rx,Ry` | `rx = ry;` (both uint64_t) |
| `LGHI Rx,n` | `rx = (int64_t)n;` (sign-extends 16-bit immediate) |
| `LLGF Rx,field` | `rx = (uint64_t)*(uint32_t *)&field;` (zero-extend 32→64) |
| `LGF Rx,field` | `rx = (int64_t)*(int32_t *)&field;` (sign-extend 32→64) |

---

## 3. Struct Packing in AMODE 64

In AMODE 64, IBM control block DSECTs typically use 8-byte-aligned doubleword
fields for pointers.  This means natural alignment pads the struct — but since
we use `#pragma pack(1)`, there is no automatic padding.  Verify every pointer
field offset against the 64-bit DSECT layout explicitly.

**Example — IMS MSCP in AMODE 64 (illustrative):**

```c
#pragma pack(1)
struct mscp64 {
    char      mscpid[4];    /* +0   Eye-catcher 'MSCP'  */
    uint32_t  mscplen;      /* +4   Block length        */
    uint64_t  mscpnext;     /* +8   Next MSCP (64-bit ptr) */
    uint64_t  mscpuser;     /* +16  User data ptr       */
    uint8_t   mscpfunc;     /* +24  Function code       */
    uint8_t   mscpreqd;     /* +25  Required flag       */
    uint16_t  mscprsv;      /* +26  Reserved            */
};
#pragma pack()
```

Every 64-bit pointer field in the struct adds 8 bytes where the 31-bit version
had 4.  A struct that is 48 bytes in AMODE 31 may be 80 bytes in AMODE 64.

**Always use `verify_structs.c`** to assert the sizes and offsets compile-time:

```c
#ifdef __LP64__
ASSERT_OFFSET(struct mscp64, mscpnext, 8);
ASSERT_OFFSET(struct mscp64, mscpfunc, 24);
ASSERT_SIZE(struct mscp64, 28);
#endif
```

---

## 4. Conversion Rules for AMODE 64 Exits

1. **Compile flag**: always pass `-q64` when converting an AMODE 64 exit.
2. **Pointer fields in structs**: use `uint64_t` (not `uint32_t` or `ptr_t`) for
   fields whose DSECT layout is explicitly 8 bytes wide.  Use `ptr_t` only for
   fields whose width tracks the AMODE.
3. **No `PARM_LAST_MASK` (0x80000000)** on 64-bit parm list pointers — check
   the product interface for its specific end-of-list convention.
4. **STCK / STCKE**: unchanged — both return 64-bit or 128-bit values that fit
   naturally in AMODE 64.
5. **getmain / freemain**: the base wrappers work in both AMODE 31 and 64; in
   AMODE 64 storage above 2 GB is available (subpool dependent).
6. **WTO**: unchanged — WTO is a 31-bit SVC; the text pointer is 31-bit
   even in AMODE 64 exits.  The compiler handles this automatically.

---

## 5. Detection Changes in `asm-pre-analyzer`

When analyzing an AMODE 64 source, the pre-analyzer must:

- Report AMODE 64 explicitly in §1 (Module Identity)
- Note all 64-bit register instructions (LG, STG, LGR, LGHI, LLGF, etc.) in §8
- Flag pointer-sized fields in DSECTs as 8 bytes (not 4)
- Recommend `-q64` compile flag in §10 (Scope Assessment)
- Note that `IS_LAST_PARM` / `PARM_LAST_MASK` patterns do not apply

---

## 6. Verification Matrix Notes for AMODE 64

Add a row in §2 (Entry Conditions) to document the AMODE:

```
| AMODE | 64 — compile with xlc -qmetal -q64 |
```

Add an entry in §6 (Flagged Concerns) for any struct field whose size
differs between 31 and 64 bit:

```
Cx — Struct field <name> is 4 bytes in 31-bit layout, 8 bytes in 64-bit layout.
Verify offset against 64-bit DSECT definition in product macro library.
Assessment: Open — confirm with xlc -q64 verify_structs.c
```

---

## 7. Products With Known AMODE 64 Exits

### IMS 15+ AMODE 64 exits

IMS 15 introduced full 64-bit addressing for IMS exits.  The exit interface
uses BAKR/PR linkage and 64-bit parameter block pointers.  The key change
from IMS AMODE 31:
- `parmlist[0]` pointer is 8 bytes
- MSCP/MSCD structs have 8-byte pointer fields
- RACROUTE in IMS 15 may use 64-bit ACEE pointers

Use `metalc_ims.h` with `#ifdef __LP64__` guards for the pointer fields.

### MQ 9.3+ channel exits

CSQXLIB channel exits support both AMODE 31 and AMODE 64.  MQ passes a
version indicator in the exit parameter block; version ≥ 11 implies AMODE 64
capability.  Compile the module in AMODE 64 and check the version at runtime:

```c
if (parm->version >= MQCXP_VERSION_11) {
    /* 64-bit parm fields available */
}
```

### WLM exits

`IWMXITP` (WLM ITP exit) is strictly AMODE 64.  No AMODE 31 version exists.

---

## 8. Compile Flag Summary

| AMODE | Compile flag |
|-------|-------------|
| 31 | `xlc -qmetal -S -qlist module.c` |
| 64 | `xlc -qmetal -q64 -S -qlist module.c` |

The `-q64` flag is the only change needed at the Metal C compilation step.
Link-edit attributes are outside the scope of this conversion framework.

---

## 9. Related Documents

- `docs/asm-linkage-conventions.md` — BAKR/PR detection; applies equally to AMODE 64
- `docs/complex-asm-patterns.md` — 64-bit register instructions (LG, STG, etc.)
- `includes/metalc_ims.h` — IMS structs (update pointer fields for AMODE 64)
- `includes/metalc_mq.h` — MQ structs (MQI version field for 64-bit detection)
- `tests/verify_structs.c` — add `#ifdef __LP64__` offset assertions for 64-bit structs
