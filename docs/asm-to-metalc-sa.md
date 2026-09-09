# AI Translation Rules: IBM System Automation (SA) Exit Assembler to Metal C

## Supplement to General Translation Rules

This document supplements the general assembler-to-Metal-C translation rules
with IBM System Automation for z/OS (SA z/OS) exit-specific guidance.
It covers resource state change exits, automation exits, and message exits.

---

## 1. SA z/OS Exit Overview

IBM System Automation for z/OS (SA z/OS, formerly NetView SA/390) provides
installation exits at automation decision points:

| Exit Module | Exit Point | Purpose |
|-------------|-----------|---------|
| `AOFEXC00` | Message intercept | Intercept automation messages |
| `AOFEXC01` | Automation action | Called before an automation action |
| `AOFEXC02` | Resource state change | Resource state transition notification |
| `AOFEXC03` | Operator notify | Operator notification                |
| `AOFEXC04` | Recovery | Recovery decision exit             |
| `AOFEXWSF` | Workstation | Workstation interaction            |

Exit names follow the pattern `AOFEXC<nn>` (IBM prefix `AOF`).
Exits are defined in the automation policy and activated via the SA startup
procedure.

---

## 2. Exit Architecture

### SA Resource State Change Exit (AOFEXC02)

The resource state change exit receives control when a monitored resource
changes state.  It receives a typed parameter block via R1.

```c
#pragma prolog(AOFEXC02, "SAVE(14,12),LR(12,15)")
#pragma epilog(AOFEXC02, "RETURN(14,12)")

int AOFEXC02(struct sa_res_parm *parm) {
    if (parm->func != SA_FUNC_STATCHG) {
        return SA_RES_CONTINUE;
    }
    ...
}
```

---

## 3. SA Control Block Structures

### SA_RES_PARM — Resource State Change Exit Parameter Block

```c
#pragma pack(1)
struct sa_res_parm {
    EXIT_PARM_HEADER;         /* +0   work(+0), func(+4), flags(+5), reserved(+6) */
    /* parm->reserved (at +6) maps to recovery flags */
    struct sa_resource *sares;/* +8   Pointer to resource block       */
    uint8_t   sanewstate;     /* +12  New resource state              */
    uint8_t   saoldstate;     /* +13  Previous resource state         */
    uint16_t  sapadding;      /* +14  Reserved                        */
    char      samsgid[8];     /* +16  SA message ID triggering change */
    char      sasysplex[8];   /* +24  Sysplex name                    */
};
#pragma pack()

/* func values */
#define SA_FUNC_INIT       1  /* Exit initialization                  */
#define SA_FUNC_STATCHG    3  /* Resource state change                */
#define SA_FUNC_TERM       9  /* Exit termination                     */

/* sanewstate / saoldstate values */
#define SA_STATE_UP        0  /* Resource is up/active                */
#define SA_STATE_HARDDOWN  1  /* Resource is hard down (failed)       */
#define SA_STATE_SOFTDOWN  2  /* Resource is soft down (orderly stop) */
#define SA_STATE_PROBLEM   3  /* Resource has a problem               */
#define SA_STATE_DEGRADED  4  /* Resource is degraded                 */
#define SA_STATE_STARTING  5  /* Resource is starting                 */
#define SA_STATE_STOPPING  6  /* Resource is stopping                 */
#define SA_STATE_UNKNOWN   7  /* Resource state unknown               */

/* Recovery flags (parm->reserved at +6) */
#define SA_FLG_RECOVERY    0x80  /* Recovery in progress              */
#define SA_FLG_MAINT       0x40  /* Maintenance window active         */
```

### SA_RESOURCE — Resource Block

```c
#pragma pack(1)
struct sa_resource {
    char      resid[4];       /* +0   Eye-catcher 'SARC'              */
    char      resname[32];    /* +4   Resource name                   */
    char      restype[8];     /* +36  Resource type (APL/JOB/MVS/etc) */
    char      jobname[8];     /* +44  Associated job name             */
    uint8_t   flags;          /* +52  Resource flags                  */
    uint8_t   automation;     /* +53  Automation mode                 */
    uint8_t   priority;       /* +54  Priority                        */
    uint8_t   reserved;       /* +55  Reserved                        */
    uint32_t  restart_count;  /* +56  Number of restart attempts      */
    uint32_t  last_state_time;/* +60  Time of last state change       */
};
#pragma pack()

/* flags bits */
#define SA_RES_FLG_CRITICAL  0x80  /* Critical path resource          */
#define SA_RES_FLG_MONITORED 0x40  /* Actively monitored              */
#define SA_RES_FLG_CLONED    0x20  /* Cloned resource                 */

/* automation values */
#define SA_AUTO_AUTO         1   /* Fully automated                   */
#define SA_AUTO_ASSIST       2   /* Assist mode (operator notified)   */
#define SA_AUTO_MANUAL       3   /* Manual only                       */
#define SA_AUTO_INACTIVE     4   /* Automation inactive               */
```

---

## 4. Exit Return Codes

### Resource State Change Exit Return Codes

| Return Code | C Constant | Meaning |
|-------------|------------|---------|
| 0 | `SA_RES_CONTINUE` | Continue normal SA processing |
| 4 | `SA_RES_SUPPRESS` | Suppress the automation action |

**When returning `SA_RES_SUPPRESS` (4):** SA will not take the default automated
action for this state change.  The operator may still be notified (depending on
automation mode).  Use when the exit itself handles recovery or when a
maintenance window is active.

---

## 5. Common SA Exit Patterns

### Pattern 1: Critical Resource Alert and Maintenance Window Check (AOFEXC02)

**Assembler:**
```asm
AOFEXC02 CSECT
         USING *,R12
         LR    R12,R15
         L     R10,0(,R1)         SA parm block
         USING AOFRPARM,R10
         CLI   AOFFUNC,3           State change call?
         BNE   RETURN
         L     R3,SARESPTR         Resource block
         LTR   R3,R3
         BZ    RETURN
         USING SARESBLK,R3
*        Check for HARDDOWN
         CLI   SANEWSTATE,1
         BNE   CHKCRIT
*        Log HARDDOWN
         ...
CHKCRIT  TM    RESFLAGS,SARESCRIT  Critical?
         BZ    RETURN
*        Critical resource - issue alert
         ...
*        Check maintenance flag
         TM    SAFLAGS,SARECOV     Recovery?
         BZ    RETURN
         CLI   RESAUTO,2           Assist mode?
         BNE   RETURN
*        Suppress during recovery in assist mode
         LA    R15,4
         BR    R14
RETURN   SR    R15,R15
         BR    R14
```

**Metal C:**
```c
#include "metalc_base.h"
#include "metalc_sa.h"

#pragma prolog(AOFEXC02, "SAVE(14,12),LR(12,15)")
#pragma epilog(AOFEXC02, "RETURN(14,12)")

int AOFEXC02(struct sa_res_parm *parm) {
    struct sa_resource *res;

    if (parm->func != SA_FUNC_STATCHG) {
        return SA_RES_CONTINUE;
    }

    res = parm->sares;
    if (res == NULL) {
        return SA_RES_CONTINUE;
    }

    /* Log HARDDOWN transitions */
    if (parm->sanewstate == SA_STATE_HARDDOWN) {
        char msg[100];
        int pos = 0;
        msg_append_str(msg, &pos, "AOFEXC02 HARDDOWN RES=");
        msg_append_field(msg, &pos, res->resname, 32);
        msg_append_str(msg, &pos, " JOB=");
        msg_append_field(msg, &pos, res->jobname, 8);
        wto_write(msg, pos, WTO_ROUTE_MASTER_CONSOLE | WTO_ROUTE_SYSTEM_ERROR, 0);
    }

    /* Alert for critical resource failures */
    if (TM_ALL(res->flags, SA_RES_FLG_CRITICAL)) {
        if (parm->sanewstate == SA_STATE_PROBLEM  ||
            parm->sanewstate == SA_STATE_HARDDOWN ||
            parm->sanewstate == SA_STATE_DEGRADED) {
            char msg[80];
            int pos = 0;
            msg_append_str(msg, &pos, "AOFEXC02 CRITICAL ALERT RES=");
            msg_append_field(msg, &pos, res->resname, 32);
            wto_write(msg, pos, WTO_ROUTE_MASTER_CONSOLE | WTO_ROUTE_SYSTEM_SECURITY,
                      WTO_DESC_SYSTEM_FAILURE);
        }
    }

    /* Suppress auto-recovery during maintenance/recovery in assist mode */
    if (TM_ALL(parm->reserved, SA_FLG_RECOVERY)) {
        if (res->automation == SA_AUTO_ASSIST) {
            return SA_RES_SUPPRESS;
        }
    }

    return SA_RES_CONTINUE;
}
```

---

## 6. SA-Specific Considerations

### 6.1 Address Space Context

SA z/OS exits run in the **NetView address space** (or the SA automation
address space, depending on product version).  Execution is in problem state.

### 6.2 SA_RES_SUPPRESS Implications

Suppressing an automation action removes SA's default recovery response.
If the exit suppresses but does not handle recovery itself, the resource
may remain in a failed state indefinitely.  Always document why suppression
is appropriate.

### 6.3 Maintenance Window Logic

The `SA_FLG_MAINT` and `SA_FLG_RECOVERY` flags in `parm->reserved` indicate
that SA is aware of a maintenance window or recovery in progress.  Exiting
with `SA_RES_SUPPRESS` during a maintenance window prevents SA from taking
automated actions during planned outages.

### 6.4 Resource Types

`sa_resource.restype` contains an 8-byte identifier:
- `"APL     "` — Application resource
- `"JOB     "` — Batch job resource
- `"MVS     "` — MVS subsystem resource
- `"MVSCOMP "` — MVS component resource

Use `match_prefix` or `match_field` for type checks.

### 6.5 SA Policy Registration

Exits are registered in the SA automation policy (NetView PARMS member or
SA policy database):

```
EXIT ROUTINE=AOFEXC02,ENABLE
```

---

## 7. Build and Installation

### Compilation

```jcl
//COMPILE  EXEC PGM=CCNDRVR,PARM='METAL,LIST,NOSEQ'
//STEPLIB  DD DSN=CEE.SCEERUN2,DISP=SHR
//SYSPRINT DD SYSOUT=*
//SYSIN    DD DSN=your.src(AOFEXC02),DISP=SHR
//SYSLIN   DD DSN=your.obj(AOFEXC02),DISP=SHR
```

### Linkage

```jcl
//LINK     EXEC PGM=IEWL,PARM='LIST,MAP,RENT,REFR,RMODE=ANY,AMODE=31'
//SYSLIB   DD DSN=CNM.SCNMLINK,DISP=SHR
//SYSLMOD  DD DSN=your.sa.exits(AOFEXC02),DISP=SHR
//SYSLIN   DD DSN=your.obj(AOFEXC02),DISP=SHR
```

---

## 8. Testing

1. **Test SA environment**: Use a non-production SA/NetView environment.
2. **SA log**: Review NetView log and SA message log for exit activity.
3. **Simulate state changes**: Use SA operator commands to simulate resource failures.
4. **Verify suppression**: Confirm that SA does not take action when `SA_RES_SUPPRESS` is returned.

---

## 9. Verification Checklist (SA-Specific)

- [ ] `SA_RES_SUPPRESS` use fully justified (no unintended suppression)
- [ ] Maintenance window flags correctly checked before suppression
- [ ] Critical resource flag (`SA_RES_FLG_CRITICAL`) tested correctly
- [ ] `parm->sares` null-checked before dereferencing
- [ ] Resource type checks use `match_field` or `match_prefix` (8-char padded)
- [ ] Exit registered in SA automation policy
- [ ] SA environment restarted or exit refreshed after load module update
- [ ] Exit is reentrant — no static writable data
- [ ] Tested for all state transitions: UP, HARDDOWN, SOFTDOWN, PROBLEM, DEGRADED
