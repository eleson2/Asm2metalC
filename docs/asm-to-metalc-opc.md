# AI Translation Rules: OPC/TWS Exit Assembler to Metal C

## Supplement to General Translation Rules

This document supplements the general assembler-to-Metal-C translation rules
with IBM Tivoli Workload Scheduler for z/OS (TWS z/OS, formerly OPC/ESA)
exit-specific guidance.

---

## 1. OPC/TWS Exit Overview

IBM TWS z/OS provides installation exits at job scheduling decision points:

| Exit Module | Exit Point | Purpose |
|-------------|-----------|---------|
| `EQQUX000` | Submission | Called before a job is submitted |
| `EQQUX001` | Completion | Called when a job completes |
| `EQQUX002` | Error | Called when a job ends in error |
| `EQQUX007` | Status change | Called when an operation status changes |
| `EQQUX009` | Late start | Called when an operation is late |
| `EQQUX010` | WLM | Called for WLM service class assignment |

Exit module names follow the pattern `EQQUX<nnn>` (IBM prefix `EQQ`).
Exits are defined in the TWS initialization parameters (`EQQPRMXX` or
the `OPC PARM` DD in the TWS controller job).

---

## 2. Exit Architecture

### Operation Status Change Exit (EQQUX007)

EQQUX007 is called when any operation changes status.  It receives a typed
parameter block via R1.  The exit is notification-only for most paths —
the standard return is always continue (RC=0).

```c
#pragma prolog(EQQUX007, "SAVE(14,12),LR(12,15)")
#pragma epilog(EQQUX007, "RETURN(14,12)")

int EQQUX007(struct opc_ux007_parm *parm) {
    if (parm->ux007func != OPC_UX007_STATUS) {
        return OPC_UX007_CONTINUE;
    }
    ...
}
```

---

## 3. OPC/TWS Control Block Structures

### OPC_UX007_PARM — Status Change Exit Parameter Block

```c
#pragma pack(1)
struct opc_ux007_parm {
    char      ux007id[4];         /* +0   Eye-catcher 'UX07'             */
    uint32_t  ux007func;          /* +4   Function code                  */
    uint8_t   ux007flags;         /* +8   Flags                          */
    uint8_t   ux007reserved[3];   /* +9   Reserved                       */
    struct opc_oper *ux007oper;   /* +12  Pointer to operation block     */
    uint8_t   ux007newst;         /* +16  New status code                */
    uint8_t   ux007oldst;         /* +17  Previous status code           */
    uint16_t  ux007padding;       /* +18  Reserved                       */
    char      ux007applic[16];    /* +20  Application name               */
    char      ux007date[8];       /* +28  Input arrival date (YYYYMMDD)  */
};
#pragma pack()

/* ux007func values */
#define OPC_UX007_STATUS    1   /* Status change notification           */
#define OPC_UX007_INIT      9   /* Exit initialization                  */
#define OPC_UX007_TERM      10  /* Exit termination                     */

/* ux007newst / ux007oldst — operation status codes */
#define OPC_STATUS_WAITING  'W'  /* Waiting to start                    */
#define OPC_STATUS_READY    'R'  /* Ready to start                      */
#define OPC_STATUS_STARTED  'S'  /* Started (executing)                 */
#define OPC_STATUS_COMPLETE 'C'  /* Completed successfully              */
#define OPC_STATUS_ERROR    'E'  /* Ended in error                      */
#define OPC_STATUS_INTERRUPT 'I' /* Interrupted                         */
#define OPC_STATUS_PENDING  'P'  /* Pending                             */
```

### OPC_OPER — Operation Block

```c
#pragma pack(1)
struct opc_oper {
    char      op_jobname[8];      /* +0   Job name                       */
    char      op_opno[3];         /* +8   Operation number               */
    uint8_t   op_flags;           /* +11  Operation flags                */
    /* +11 bit 7 (0x80) = critical path flag (TM 71(R3),X'80' in ASM) */
    char      op_appl[16];        /* +12  Application name               */
    char      op_date[8];         /* +28  Planned start date (YYYYMMDD)  */
    char      op_time[6];         /* +36  Planned start time (HHMMSS)    */
    uint8_t   op_reserved[2];     /* +42  Reserved                       */
    char      op_workstat[8];     /* +44  Workstation name               */
    uint8_t   op_status;          /* +52  Current status                 */
    uint8_t   op_errcode;         /* +53  Error code (if status=E)       */
};
#pragma pack()

/* op_flags bits */
#define OPC_OP_FLG_CRITICAL  0x80  /* Operation is on critical path     */
#define OPC_OP_FLG_HELD      0x40  /* Operation is held                 */
#define OPC_OP_FLG_DEADLINE  0x20  /* Operation has a deadline          */
```

### OPC_UX000_PARM — Submission Exit Parameter Block

```c
#pragma pack(1)
struct opc_ux000_parm {
    char      ux000id[4];         /* +0   Eye-catcher 'UX00'             */
    uint32_t  ux000func;          /* +4   Function code                  */
    char      ux000jobname[8];    /* +8   Job name                       */
    char      ux000appl[16];      /* +16  Application name               */
    char      ux000wsname[8];     /* +32  Workstation name               */
    uint32_t  ux000rc;            /* +40  Return code (output)           */
};
#pragma pack()
```

---

## 4. Exit Return Codes

### Status Change Exit Return Codes

| Return Code | C Constant | Meaning |
|-------------|------------|---------|
| 0 | `OPC_UX007_CONTINUE` | Continue normal TWS processing |

EQQUX007 is notification-only — the return code is always 0.  The exit cannot
alter the status change.

### Submission Exit Return Codes

| Return Code | C Constant | Meaning |
|-------------|------------|---------|
| 0 | `OPC_UX000_CONTINUE` | Submit the job |
| 4 | `OPC_UX000_HOLD` | Hold the job (defer submission) |
| 8 | `OPC_UX000_CANCEL` | Cancel the operation |

---

## 5. Common OPC/TWS Exit Patterns

### Pattern 1: Critical Path Error Alert (EQQUX007)

**Assembler:**
```asm
EQQUX007 CSECT
         USING *,R12
         LR    R12,R15
         L     R10,0(,R1)          UX007 parm block
         USING EQQUX7DS,R10
         CLC   UX007FUNC(4),=F'1'  Status change?
         BNE   RETURN
*        Check new status = Error ('E')
         CLI   UX007NEWST,C'E'
         BNE   RETURN
*        Get operation block
         L     R3,UX007OPER
         LTR   R3,R3
         BZ    RETURN
         USING EQQOPBK,R3
*        Check critical path flag (bit 7 of OP_FLAGS at offset 71)
         TM    71(R3),X'80'         Critical path?
         BZ    RETURN
*        Issue alert
         ...
RETURN   SR    R15,R15
         BR    R14
```

**Metal C:**
```c
#include "metalc_base.h"
#include "metalc_opc.h"

#pragma prolog(EQQUX007, "SAVE(14,12),LR(12,15)")
#pragma epilog(EQQUX007, "RETURN(14,12)")

int EQQUX007(struct opc_ux007_parm *parm) {

    if (parm->ux007func != OPC_UX007_STATUS) {
        return OPC_UX007_CONTINUE;
    }

    /* Only process error status */
    if (parm->ux007newst != OPC_STATUS_ERROR) {
        return OPC_UX007_CONTINUE;
    }

    struct opc_oper *oper = parm->ux007oper;
    if (oper == NULL) {
        return OPC_UX007_CONTINUE;
    }

    /* Critical path operations get an alert */
    if (TM_ALL(oper->op_flags, OPC_OP_FLG_CRITICAL)) {
        char msg[80];
        int pos = 0;
        msg_append_str(msg, &pos, "EQQUX007 CRITICAL PATH ERROR: ");
        msg_append_field(msg, &pos, oper->op_jobname, 8);
        msg_append_str(msg, &pos, " ENDED IN ERROR");
        wto_write(msg, pos, WTO_ROUTE_MASTER_CONSOLE | WTO_ROUTE_PROGRAMMER_INFO,
                  WTO_DESC_EVENTUAL_ACTION);
    }

    return OPC_UX007_CONTINUE;
}
```

### Pattern 2: opc_is_critical_path() helper

The `opc_is_critical_path()` function from `metalc_opc.h` wraps the flag check:

```c
/* Implementation in metalc_opc.h */
static inline int opc_is_critical_path(const struct opc_oper *oper) {
    return TM_ALL(oper->op_flags, OPC_OP_FLG_CRITICAL);
}
```

Usage:
```c
if (opc_is_critical_path(oper)) { ... }
```

---

## 6. OPC/TWS-Specific Considerations

### 6.1 ASM Critical Path Offset

The critical path flag in the original ASM code is referenced as:
```asm
TM    71(R3),X'80'
```
This is **offset +71 from the operation block base**.  In the `opc_oper` struct,
offset +11 is `op_flags` which contains the critical path flag at bit 7 (0x80).
**Verify this offset matches** `TM 71(R3)` — the DSECT definition places the flags
field at +11 from the EQQOPBK DSECT, and the ASM addresses the operation block from
R3 at the DSECT base.  The absolute address `71(R3)` implies R3 is based at offset
-60 from the DSECT base in the original source.  Cross-reference with `EQQOPBK`
DSECT on the target TWS release.

### 6.2 Status Code Format

Operation status codes are **single EBCDIC characters** stored in a `uint8_t` field.
Compare with character literals: `parm->ux007newst == OPC_STATUS_ERROR` where
`OPC_STATUS_ERROR` is defined as `'E'` (EBCDIC `0xC5`).

### 6.3 Date/Time Format

`ux007date` is an 8-byte EBCDIC string in `YYYYMMDD` format.
`ux007time` (in the operation block) is a 6-byte string in `HHMMSS` format.
Use `match_prefix` for date range checks.

### 6.4 EQQUX007 Is Notification-Only

The status change exit cannot prevent or alter the status change — it is purely
observational.  Returning RC=0 is the only valid response.

### 6.5 TWS Controller Address Space

EQQUX007 runs in the **TWS Controller address space**.  It is called synchronously
during status change processing.  Keep processing fast.

### 6.6 TWS EQQPRMXX Registration

```
EXIT07  MODULE=EQQUX007,OPTION=ENABLE
```

In the `EQQPRMXX` member.  A TWS controller restart is required to pick up changes.

---

## 7. Build and Installation

### Compilation

```jcl
//COMPILE  EXEC PGM=CCNDRVR,PARM='METAL,LIST,NOSEQ'
//STEPLIB  DD DSN=CEE.SCEERUN2,DISP=SHR
//SYSPRINT DD SYSOUT=*
//SYSIN    DD DSN=your.src(EQQUX007),DISP=SHR
//SYSLIN   DD DSN=your.obj(EQQUX007),DISP=SHR
```

### Linkage

```jcl
//LINK     EXEC PGM=IEWL,PARM='LIST,MAP,RENT,REFR,RMODE=ANY,AMODE=31'
//SYSLIB   DD DSN=EQQS.SEQQLINK,DISP=SHR
//SYSLMOD  DD DSN=your.tws.exits(EQQUX007),DISP=SHR
//SYSLIN   DD DSN=your.obj(EQQUX007),DISP=SHR
```

---

## 8. Testing

1. **TWS test controller**: Use a dedicated TWS controller for exit testing.
2. **TWS trace**: Enable `TRACE EXIT(07)` in TWS PARMS.
3. **Test job failures**: Submit test jobs that end in error on the critical path.
4. **TWS log**: Review the TWS daily planning log and message log.

---

## 9. Verification Checklist (OPC/TWS-Specific)

- [ ] Critical path offset (`TM 71(R3),X'80'`) cross-referenced against `EQQOPBK` DSECT
- [ ] Status codes compared as single EBCDIC characters (not integers)
- [ ] `ux007oper` pointer null-checked before dereferencing
- [ ] EQQUX007 always returns `OPC_UX007_CONTINUE` (0) — no other valid RC
- [ ] Exit registered in EQQPRMXX with `EXIT07 MODULE=EQQUX007`
- [ ] TWS controller restarted after exit load module update
- [ ] Exit is reentrant — no static writable data
- [ ] Tested with critical path jobs ending in error, non-critical jobs ending in error
