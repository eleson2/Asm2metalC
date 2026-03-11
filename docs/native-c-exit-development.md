# Guide: Writing New Exits Directly in Metal C

This guide describes how to develop new z/OS system exits directly in C using the Metal C framework.

## 1. Project Structure
New exits should always include the base and product headers:
```c
#include "metalc_base.h"
#include "metalc_<product>.h"
```

## 2. Defining the Entry Point
Use the `METALC_ENTRY` macro to handle standard z/OS register linkage. This ensures your exit is reentrant and follows system conventions.

```c
/* Define linkage for the entry point */
#pragma prolog(MYEXIT, "SAVE(14,12),LR(12,15)")
#pragma epilog(MYEXIT, "RETURN(14,12)")

int MYEXIT(struct jes2_xpl *parm) {
    /* Implementation */
    return RC_OK;
}
```

## 3. Memory Management
### Stack Variables
Small local variables are stored on the stack automatically. This is the preferred method for temporary work.

### Dynamic Allocation (GETMAIN)
If you need a large work area or storage that persists beyond the life of the exit, use `getmain()`:

```c
struct my_work *work = getmain(sizeof(struct my_work), SUBPOOL_JOB_STEP);
if (!work) return RC_SEVERE;

/* ... use storage ... */

freemain(work, sizeof(struct my_work), SUBPOOL_JOB_STEP);
```

## 4. Working with Fixed-Width Fields (EBCDIC)
Mainframe control blocks use fixed-width, blank-padded strings. Do not use standard C string functions.

**Best Practices:**
- **Comparison:** Use `match_field(a, b, len)` or `match_prefix(s, pfx, len)`.
- **Assignment:** Use `set_fixed_string(dest, "VALUE", len)`.
- **Formatting:** Use `format_int()` and `format_hex()` to build messages.

## 5. Console Communication (WTO)
Use `WTO_LITERAL` for simple messages or `wto_write` for advanced routing.

```c
WTO_LITERAL("MYEXIT: Processing starting...");

char msg[80];
int pos = 0;
msg_append_str(msg, &pos, "MYEXIT: Action taken for user ");
msg_append_field(msg, &pos, parm->userid, 8);
wto_write(msg, pos, WTO_ROUTE_MASTER_CONSOLE, 0);
```

## 6. Logic and Bit-Testing
Use semantic macros to make your intent clear:
- `if (TM_ALL(flags, BIT))` instead of `if ((flags & BIT) == BIT)`
- `OI(flags, BIT);` instead of `flags |= BIT;`

## 7. Skeleton Template for a New Exit

```c
#include "metalc_base.h"
#include "metalc_jes2.h"

/* Standard Linkage */
#pragma prolog(JES2AUDT, "SAVE(14,12),LR(12,15)")
#pragma epilog(JES2AUDT, "RETURN(14,12)")

/**
 * JES2AUDT - Native C Audit Exit
 * Purpose: Audit all JOB class 'X' submissions
 */
int JES2AUDT(struct jes2_exit8_parm *parm) {
    struct jct *jct = (struct jct *)parm->e8jct;

    /* Check job class */
    if (jct->jctjclas == 'X') {
        char msg[80];
        int pos = 0;
        msg_append_str(msg, &pos, "AUDIT: JOB=");
        msg_append_field(msg, &pos, jct->jctjname, 8);
        msg_append_str(msg, &pos, " submitted in Class X");
        
        wto_important(msg, pos);
    }

    return JES2_RC_CONTINUE;
}
```
