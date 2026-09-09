# AI Translation Rules: DFSMS Exit Assembler to Metal C

## Supplement to General Translation Rules

This document supplements the general assembler-to-Metal-C translation rules
with DFSMS (Data Facility Storage Management Subsystem) exit-specific guidance.
It covers dynamic allocation exits, storage class exits, and catalog exits.

---

## 1. DFSMS Exit Overview

DFSMS provides installation exits at key dataset management points:

| Exit Module | Exit Point | Purpose |
|-------------|-----------|---------|
| `IEFDB401` | Dynamic Allocation | Modify or reject allocation requests |
| `IGDSMSxx` | ACS (Automatic Class Selection) | Assign SMS classes |
| `ICBCX001` | Catalog search | Intercept catalog operations |
| `ARCBDXT` | HSM Migration | Control data set migration |
| `IEFUSI` | Step initiation | Initialize step before execution |

---

## 2. Exit Architecture

### Dynamic Allocation Exit (IEFDB401)

IEFDB401 is called by the allocation routines (IEFAB4A0/SVC99) before and after
allocation.  It receives a structured parameter block via R1.

```c
#pragma prolog(IEFDB401, "SAVE(14,12),LR(12,15)")
#pragma epilog(IEFDB401, "RETURN(14,12)")

int IEFDB401(struct alloc_exit_parm *parm) {
    if (parm->func != ALLOC_FUNC_ALLOC) {
        return ALLOC_RC_CONTINUE;
    }
    ...
}
```

---

## 3. DFSMS Control Block Structures

### ALLOC_EXIT_PARM — Dynamic Allocation Exit Parameter Block

```c
#pragma pack(1)
struct alloc_exit_parm {
    EXIT_PARM_HEADER;             /* +0   work, func, flags, reserved    */
    char      alcjob[8];          /* +8   Job name                       */
    char      alcstep[8];         /* +16  Step name                      */
    char      alcdsn[44];         /* +24  Dataset name                   */
    char      alcmemb[8];         /* +68  Member name (PDS/PDSE)         */
    char      alcvol[6];          /* +76  Volume serial                  */
    char      alcunit[8];         /* +82  Unit type                      */
    uint8_t   alcspace;           /* +90  Space type                     */
    uint8_t   alcflags;           /* +91  Flags                          */
    uint16_t  alcreserved;        /* +92  Reserved                       */
    uint32_t  alcpri;             /* +94  Primary space quantity         */
    uint32_t  alcsec;             /* +98  Secondary space quantity       */
    char      alcsclas[8];        /* +102 SMS storage class (output)     */
    char      alcmclas[8];        /* +110 SMS management class (output)  */
    char      alcdc[8];           /* +118 SMS data class (output)        */
};
#pragma pack()

/* func values */
#define ALLOC_FUNC_ALLOC     1    /* Dataset allocation                  */
#define ALLOC_FUNC_UNALLOC   2    /* Dataset unallocation                */
#define ALLOC_FUNC_CONCAT    3    /* Concatenation                       */
#define ALLOC_FUNC_PREALLOC  4    /* Pre-allocation check                */

/* alcspace values */
#define ALLOC_SPACE_TRK      0    /* Tracks                              */
#define ALLOC_SPACE_CYL      1    /* Cylinders                           */
#define ALLOC_SPACE_BLK      2    /* Blocks                              */
#define ALLOC_SPACE_AVG      3    /* Average block size                  */

/* alcflags bits */
#define ALLOC_FLAG_TEMP      0x80  /* Temporary dataset                  */
#define ALLOC_FLAG_NEW       0x40  /* New dataset (not existing)         */
#define ALLOC_FLAG_SHR       0x20  /* DISP=SHR                           */
#define ALLOC_FLAG_OLD       0x10  /* DISP=OLD                           */
```

---

## 4. Exit Return Codes

### Dynamic Allocation Exit Return Codes

| Return Code | C Constant | Meaning |
|-------------|------------|---------|
| 0 | `ALLOC_RC_CONTINUE` | Allow allocation, no modification |
| 4 | `ALLOC_RC_MODIFIED` | Allow allocation, parameters modified |
| 8 | `ALLOC_RC_REJECT` | Reject the allocation |

**When returning `ALLOC_RC_MODIFIED`:** The exit has changed one or more output
fields (e.g., `alcsclas`, `alcmclas`, `alcdc`, `alcvol`).  DFSMS will use the
modified values.

**When returning `ALLOC_RC_REJECT`:** The allocation fails with an IEFDD message
and IKJ56225I reason code.  The job step may abend with S213 or similar.

---

## 5. Common DFSMS Exit Patterns

### Pattern 1: Storage Class Override (IEFDB401)

**Assembler:**
```asm
IEFDB401 CSECT
         USING *,R12
         LR    R12,R15
         L     R10,0(,R1)          Parameter block
         USING IEFDB4DS,R10
         CLI   ALCFUNC,1            Allocation call?
         BNE   CONTINUE
*        Large cylinder allocation?
         CLI   ALCSPACE,1           Cylinders?
         BNE   CHKPROD
         CLC   ALCPRI(4),=F'1000'   > 1000 cyl?
         BNH   CHKPROD
*        Override to LARGE storage class
         MVC   ALCSCLAS(8),=CL8'LARGE   '
         LA    R15,4
         BR    R14
CHKPROD  ...
CONTINUE SR    R15,R15
         BR    R14
```

**Metal C:**
```c
#include "metalc_base.h"
#include "metalc_dfsms.h"

#pragma prolog(IEFDB401, "SAVE(14,12),LR(12,15)")
#pragma epilog(IEFDB401, "RETURN(14,12)")

int IEFDB401(struct alloc_exit_parm *parm) {

    /* Only process allocation requests */
    if (parm->func != ALLOC_FUNC_ALLOC) {
        return ALLOC_RC_CONTINUE;
    }

    /* Large cylinder allocation → LARGE storage class */
    if (parm->alcspace == ALLOC_SPACE_CYL && parm->alcpri > 1000) {
        set_fixed_string(parm->alcsclas, "LARGE", 8);
        /* Log it */
        char msg[80];
        int pos = 0;
        msg_append_str(msg, &pos, "IEFDB401 LARGE ALLOC JOB=");
        msg_append_field(msg, &pos, parm->alcjob, 8);
        wto_important(msg, pos);
        return ALLOC_RC_MODIFIED;
    }

    /* PROD.* datasets → enforce FAST storage class */
    if (sms_match_dsn_hlq(parm->alcdsn, "PROD")) {
        if (!match_field(parm->alcsclas, "FAST    ", 8)) {
            set_fixed_string(parm->alcsclas, "FAST", 8);
            return ALLOC_RC_MODIFIED;
        }
    }

    return ALLOC_RC_CONTINUE;
}
```

### Pattern 2: Volume Restriction

**Metal C:**
```c
static const char RESTRICTED_VOLS[][6] = {
    "RSVD01",
    "RSVD02"
};
#define NUM_RESTRICTED (sizeof(RESTRICTED_VOLS) / sizeof(RESTRICTED_VOLS[0]))

int IEFDB401(struct alloc_exit_parm *parm) {
    if (parm->func != ALLOC_FUNC_ALLOC) {
        return ALLOC_RC_CONTINUE;
    }

    for (int i = 0; i < (int)NUM_RESTRICTED; i++) {
        if (match_field(parm->alcvol, RESTRICTED_VOLS[i], 6)) {
            char msg[80];
            int pos = 0;
            msg_append_str(msg, &pos, "IEFDB401 REJECT VOL=");
            msg_append_field(msg, &pos, parm->alcvol, 6);
            msg_append_str(msg, &pos, " JOB=");
            msg_append_field(msg, &pos, parm->alcjob, 8);
            wto_write(msg, pos, WTO_ROUTE_MASTER_CONSOLE, WTO_DESC_IMPORTANT_INFO);
            return ALLOC_RC_REJECT;
        }
    }
    return ALLOC_RC_CONTINUE;
}
```

---

## 6. DFSMS-Specific Considerations

### 6.1 Address Space Context

IEFDB401 runs in the **caller's address space** (the address space issuing the
SVC 99).  This means:

- It may run in a batch job's address space, a started task, or TSO.
- The execution key matches the caller's key.
- Do not assume supervisor state.

### 6.2 ALLOC_RC_MODIFIED Contract

When returning `ALLOC_RC_MODIFIED` (4):
- Only fields in the parameter block that you explicitly set are used.
- You must **not** set fields you didn't intend to change.
- If you set `alcsclas`, DFSMS ignores the original SMS ACS routine output for
  storage class and uses your value.

### 6.3 SMS Class Name Format

SMS class names (`alcsclas`, `alcmclas`, `alcdc`) are **8-byte space-padded
EBCDIC** fields.  Use `set_fixed_string(field, "CLASSNAME", 8)` to set them.

### 6.4 Dataset Name Format

`alcdsn` is a 44-byte EBCDIC field.  The `sms_match_dsn_hlq(dsn, hlq)` function
from `metalc_dfsms.h` compares the first `strlen(hlq)` bytes and checks that the
next byte is a `.` or space.

### 6.5 IEFDB401 Invocation Frequency

IEFDB401 is called for **every** SVC 99 (dynamic allocation) in the system.
Keep processing fast.  A slow or looping exit can degrade system throughput.

### 6.6 Pre-allocation vs. Allocation

`ALLOC_FUNC_PREALLOC` (4) is called **before** the actual allocation to let the
exit pre-check whether to proceed.  Returning `ALLOC_RC_REJECT` here avoids the
overhead of a full allocation attempt.

---

## 7. Build and Installation

### Compilation

```jcl
//COMPILE  EXEC PGM=CCNDRVR,PARM='METAL,LIST,NOSEQ'
//STEPLIB  DD DSN=CEE.SCEERUN2,DISP=SHR
//SYSPRINT DD SYSOUT=*
//SYSIN    DD DSN=your.src(IEFDB401),DISP=SHR
//SYSLIN   DD DSN=your.obj(IEFDB401),DISP=SHR
```

### Linkage

```jcl
//LINK     EXEC PGM=IEWL,PARM='LIST,MAP,RENT,REFR,RMODE=ANY,AMODE=31'
//SYSLMOD  DD DSN=SYS1.LNKLIB(IEFDB401),DISP=SHR
//SYSLIN   DD DSN=your.obj(IEFDB401),DISP=SHR
```

IEFDB401 must reside in **LPA** or in a LINKLIST library accessible at IPL time.
An IPL is required to pick up changes unless SETPROG LPA,ADD is used.

---

## 8. Testing

1. **Isolated test system**: Test on a non-production LPAR.
2. **WTO messages**: Use WTO to log decisions during testing.
3. **Allocation test JCL**: Write JCL that exercises each code path.
4. **DISPLAY**: `DISPLAY SMS,DDNAME(...)` can show SMS class assignments.

---

## 9. Verification Checklist (DFSMS-Specific)

- [ ] `ALLOC_RC_MODIFIED` used when fields were changed (not `ALLOC_RC_CONTINUE`)
- [ ] SMS class names are 8-byte space-padded (use `set_fixed_string`)
- [ ] `sms_match_dsn_hlq` used for HLQ checks (not raw `match_prefix`)
- [ ] Exit processes only `ALLOC_FUNC_ALLOC` unless other funcs are intentional
- [ ] Exit is in LPA or LINKLIST library accessible at IPL
- [ ] IPL or `SETPROG LPA,ADD` performed after load module update
- [ ] Exit is reentrant — no static writable data
- [ ] Performance impact assessed (called for every SVC 99 on the system)
- [ ] Tested against PROD.*, restricted volumes, and large-space allocations
