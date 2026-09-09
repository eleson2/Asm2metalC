# AI Steering Guide: Assembler to Metal C Conversion

This document provides specific instructions for AI models to ensure consistent, idiomatic, and standardized conversion from IBM z/OS Assembler to Metal C using the `metalc_*.h` header framework.

## 1. Mandatory Header Usage

Every conversion must include the following headers:
```c
#include "metalc_base.h"
#include "metalc_<product>.h"  /* e.g., metalc_jes2.h, metalc_cics.h */
#include "metalc_saf.h"        /* only if the exit calls SAF/RACROUTE */
```

`metalc_base.h` pulls in `metalc_svc.h`, which holds every z/OS system
service. Never include `metalc_svc.h` directly, and never include a
standard C header.

## 2. Exit Parameter Standardization

### Rule: Use `EXIT_PARM_HEADER`
If the Assembler entry point receives a parameter list where the first 8 bytes follow the pattern `(Work Area, Function Code, Flags, Reserved)`, you **must** use the `EXIT_PARM_HEADER` macro in the C structure definition.

**Assembler DSECT Pattern:**
```asm
PARMLIST DSECT
WORKADDR DS    A        +0 Work area
FUNCCODE DS    X        +4 Function
FLAGS    DS    X        +5 Flags
RESERVED DS    H        +6 Reserved
DATAADDR DS    A        +8 Product data
```

**Correct C Mapping:**
```c
struct product_parm {
    EXIT_PARM_HEADER;   /* Maps +0 to +7 */
    void *data_addr;    /* +8 */
};
```

**Field Access:** Use the standardized names: `parm->work`, `parm->func`, `parm->flags`.

## 3. Return Code Standardization

### Rule: Map to `RC_*` Constants
Never use literal integers for return codes. Use the product-specific macros which are synchronized with `metalc_base.h` generic codes.

| Intent | Generic RC | Typical Product Mapping |
|--------|------------|-------------------------|
| Success / Continue | `RC_OK` (0) | `JES2_RC_CONTINUE`, `CICS_UERCNORM`, etc. |
| Warning / Skip | `RC_WARNING` (4) | `JES2_RC_SKIP`, `DB2_XAC_REJECT` |
| Error / Reject | `RC_ERROR` (8) | `JES2_RC_FAIL`, `ALLOC_RC_REJECT` |
| Severe / Bypass | `RC_SEVERE` (12) | `CAT_RC_BYPASS`, `IMS_RC_ABORT` |
| Critical / Terminate | `RC_CRITICAL` (16) | `JES2_RC_TERMINATE`, `IMS_RC_TERMINATE` |

## 4. Bit Manipulation (Mapping TM/OI/NI)

### Rule: Use Semantic Bit Macros
Do not use raw bitwise operators for Assembler `TM`, `OI`, `NI` patterns. Use the macros in `metalc_base.h`.

| Assembler Pattern | C Semantic Macro |
|-------------------|------------------|
| `TM FLAG,BIT` + `BO` (All On) | `if (TM_ALL(flag, BIT))` |
| `TM FLAG,BIT` + `BZ` (All Off) | `if (TM_NONE(flag, BIT))` |
| `TM FLAG,BIT` + `BNZ` (Any On) | `if (TM_ANY(flag, BIT))` |
| `OI FLAG,BIT` (Set) | `OI(flag, BIT);` |
| `NI FLAG,255-BIT` (Clear) | `NI(flag, BIT);` |
| `XI FLAG,BIT` (Toggle) | `XI(flag, BIT);` |

## 5. String and Field Comparison

### Rule: Use Field-Aware Utilities
Mapping `CLC` instructions should use the appropriate utility from `metalc_base.h`.

| Instruction Pattern | C Utility |
|---------------------|-----------|
| `CLC FIELD1,FIELD2` (Exact match) | `match_field(f1, f2, len)` |
| `CLC FIELD(3),=C'ABC'` (Prefix) | `match_prefix(f, "ABC", 3)` |
| `MVC FIELD,=CL8' '` (Padding) | `set_fixed_string(f, "", 8)` |
| `MVC FIELD,=CL8'NAME'` (Copy+Pad) | `set_fixed_string(f, "NAME", 8)` |

## 6. Logic and Flow Steering

1.  **Register to Variable Mapping:** Always document the mapping in the function header.
    ```c
    /* Register Mapping:
     * R1  = parm (struct my_parm *)
     * R10 = jct  (struct jct *)
     * R11 = hct  (struct hct *)
     */
    ```
2.  **Function Entry:** Use `#pragma prolog` matching the original linkage (usually `SAVE(14,12)`).
3.  **Variable Widths:** Always use `int32_t`, `uint16_t`, etc. from `metalc_base.h`. Never use raw `int` or `long`.
4.  **Pointer Arithmetic:** Avoid raw additions. Use `ADDR_AT_OFFSET(base, offset)` or `PTR_AT_OFFSET(base, offset)`.

## 7. System Services — No Inline Assembler

### Rule: a converted `.c` file contains no `__asm`

Every z/OS system service is a C function call. All inline assembler in the
framework lives in `includes/metalc_svc.h`; all standalone assembler lives
in `asm/stubs/`.

```
grep -rl __asm converted/ includes/ examples/    # -> includes/metalc_svc.h only
```

**Look up every service macro in `docs/system-services-catalog.md`** — it
carries the full table and the procedure for services not yet wrapped. The
common ones:

| Assembler | C |
|-----------|---|
| `WTO 'text'` | `wto_simple(text, len)` |
| `WTO ...,ROUTCDE=,DESC=` | `wto_write(text, len, route, desc)` |
| `GETMAIN R,LV=` | `getmain(size, subpool)` |
| `FREEMAIN R,LV=,A=` | `freemain(addr, size, subpool)` |
| `STORAGE OBTAIN` | `storage_obtain(size, subpool)` |
| `STORAGE RELEASE` | `storage_release(addr, size, subpool)` |
| `STCK` | `get_tod_clock(&tod)` |
| `RACROUTE REQUEST=AUTH` | `saf_auth(class, entity, userid, attr, detail)` |
| `RACROUTE REQUEST=AUTH,CLASS='APPL'` | `saf_auth_appl(applid, userid)` |
| `SPLEVEL`, `SYSSTATE`, `TITLE`, `EJECT` | nothing — assembly-time only |

Match the storage family to the source: `GETMAIN` is unconditional and
abends on failure, `storage_obtain` returns NULL. Do not swap them.

### Rule: a service you cannot implement blocks the conversion

If a macro has no wrapper and no stub, add one (catalog §4 and §5). If you
cannot, **stop and report it**. Do not inline `__asm`, and do not leave a
placeholder that returns a success value.

```
BLOCKED: <MACRO> has no wrapper in metalc_svc.h and no stub in asm/stubs/.
Conversion of <MODULE> is incomplete. Required: <wrapper|stub> for <MACRO>.
```

This rule is not stylistic. A missing stub fails the link edit; an inline
placeholder link-edits clean and ships. `converted/IMS/DFSWHU00.c` carried
`__asm(" XR 15,15")` where a RACROUTE belonged, and allowed every IMS
sign-on regardless of RACF.

### Rule: security services return a decision, not a return code

Only the explicit "authorized" code allows. For SAF that is RC=0 alone —
RC=4 (no decision: RACF or the class inactive) and RC=8 (not authorized)
both deny, as does a service that could not be called at all. Use
`saf_auth()` / `saf_auth_appl()`, which apply this; do not test raw SAF
return codes in exit logic. See `docs/racroute-metalc-patterns.md` §5.

## 8. Example Conversion Template

**Source Assembler:**
```asm
MYEXIT   $ENTRY BASE=R12
         LR    R12,R15
         L     R10,20(,R1)      R10 = JCT
         USING JCT,R10
         TM    JCTFLG1,JCTFHELD
         BO    IS_HELD
         ...
```

**Target Metal C:**
```c
#include "metalc_base.h"
#include "metalc_jes2.h"

#pragma prolog(MYEXIT, "SAVE(14,12),LR(12,15)")
#pragma epilog(MYEXIT, "RETURN(14,12)")

int MYEXIT(struct jes2_xpl *parm) {
    /* R10 = JCT pointer from XPL offset +20 */
    struct jct *jct = (struct jct *)parm->xpljct;

    /* TM JCTFLG1,JCTFHELD + BO */
    if (TM_ALL(jct->jctflg1, JCTFLG1_HELD)) {
        /* ... */
    }
    
    return JES2_RC_CONTINUE;
}
```
