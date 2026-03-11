# AI Steering Guide: Assembler to Metal C Conversion

This document provides specific instructions for AI models to ensure consistent, idiomatic, and standardized conversion from IBM z/OS Assembler to Metal C using the `metalc_*.h` header framework.

## 1. Mandatory Header Usage

Every conversion must include the following headers:
```c
#include "metalc_base.h"
#include "metalc_<product>.h"  /* e.g., metalc_jes2.h, metalc_cics.h */
```

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

## 7. Example Conversion Template

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
