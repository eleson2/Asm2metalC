# AI Translation Rules: CICS Exit Assembler to Metal C

## Supplement to General Translation Rules

This document supplements the general assembler-to-Metal-C translation rules
with CICS-specific guidance.  It covers CICS global user exits (GLUEs),
task-related user exits (TRUEs), and the Program Error Program (PEP).

---

## 1. CICS Exit Overview

CICS provides exit points in three categories:

### Global User Exits (GLUEs)
- Invoked for system-wide events: task attach, program load, storage, file I/O
- Registered via `EXEC CICS ENABLE PROGRAM(...) EXIT(...)` or PLTPI
- Entry point receives a **GLUExxx** parameter block at R1

### Task-Related User Exits (TRUEs)
- Associated with a specific CICS task
- Registered via `EXEC CICS ENABLE PROGRAM(...) TASKSTART/TASKEND`

### Program Error Program (PEP) — DFHPEP
- Called on program check (ASRA), OS abend (ASRB), or runaway task (AICA)
- Fixed entry point name `DFHPEP`; IBM-reserved prefix

---

## 2. Exit Architecture

### Standard CICS Exit Entry

CICS exits receive a typed parameter block at R1:

```c
#pragma prolog(MYGLUEX, "SAVE(14,12),LR(12,15)")
#pragma epilog(MYGLUEX, "RETURN(14,12)")

int MYGLUEX(struct dfhglueparm *parm) {
    /* process exit */
    return CICS_UERCNORM;   /* 0 = continue normal processing */
}
```

### DFHPEP — Program Error Program

DFHPEP receives a `dfhpeppar` block:

```c
int DFHPEP(struct dfhpeppar *parm) {
    if (parm->peptype == PEP_TYPE_ASRA) { ... }
    return CICS_PEP_CONTINUE;
}
```

---

## 3. CICS Control Block Structures

### DFHPEPPAR — PEP Parameter Block

```c
#pragma pack(1)
struct dfhpeppar {
    char      pepid[4];       /* +0   Eye-catcher 'PEP '            */
    uint8_t   peptype;        /* +4   Error type (see below)        */
    uint8_t   pepflags;       /* +5   Flags                         */
    uint16_t  pepreserved;    /* +6   Reserved                      */
    char      peptran[4];     /* +8   Transaction ID                */
    char      pepprog[8];     /* +12  Program name                  */
    char      pepabnd[4];     /* +20  Abend code                    */
    uint32_t  peppsw;         /* +24  PSW at time of error          */
    void     *pepstk;         /* +28  Stack frame address           */
};
#pragma pack()

/* peptype values */
#define PEP_TYPE_ASRA    0x01  /* Program check                     */
#define PEP_TYPE_ASRB    0x02  /* OS abend                          */
#define PEP_TYPE_AICA    0x03  /* Runaway task                      */
#define PEP_TYPE_ASRD    0x04  /* Divide exception                  */
```

### DFHGLUEPARM — Generic GLUE Parameter Block

```c
#pragma pack(1)
struct dfhglueparm {
    EXIT_PARM_HEADER;         /* +0   work, func, flags, reserved   */
    char      gluexit[8];     /* +8   Exit point name               */
    void     *gluetctte;      /* +16  TCTTE address (current task)  */
    void     *gluecsa;        /* +20  CSA address                   */
};
#pragma pack()
```

### TCTTE — Terminal Control Table Terminal Entry

```c
#pragma pack(1)
struct tctte {
    char      tctteadr[4];    /* +0   Eye-catcher                   */
    char      tctteti[4];     /* +4   Terminal ID                   */
    char      tcttetrid[4];   /* +8   Transaction ID                */
    uint8_t   tcttests;       /* +12  Terminal status               */
    uint8_t   tctteflg;       /* +13  Flags                         */
    /* Additional fields are CICS-release specific */
};
#pragma pack()
```

---

## 4. Exit Return Codes

### PEP Return Codes

| Return Code | C Constant | Meaning |
|-------------|------------|---------|
| 0 | `CICS_PEP_CONTINUE` | Continue normal CICS abend processing |
| 4 | `CICS_PEP_RETRY` | Retry the failing operation |
| 8 | `CICS_PEP_SUPPRESS` | Suppress the abend (use with care) |

### GLUE Return Codes

| Return Code | C Constant | Meaning |
|-------------|------------|---------|
| 0 | `CICS_UERCNORM` | Normal — continue processing |
| 4 | `CICS_UERCBYP` | Bypass — skip the function |
| 8 | `CICS_UERCPURG` | Purge — terminate the task |

---

## 5. Common CICS Exit Patterns

### Pattern 1: Program Error Program (DFHPEP)

**Assembler:**
```asm
DFHPEP   CSECT
         USING *,R12
         LR    R12,R15
         L     R10,0(,R1)         PEP parameter block
         USING DFHPEPDS,R10
         CLI   PEPTYPE,X'01'      ASRA?
         BNE   CHKAICA
*        ASRA - check if system transaction
         CLC   PEPTRAN(4),=CL4'CECI'
         BE    RETURN
         CLC   PEPTRAN(4),=CL4'CEBR'
         BE    RETURN
*        Issue WTO
         ...
RETURN   SR    R15,R15
         BR    R14
```

**Metal C:**
```c
#include "metalc_base.h"
#include "metalc_cics.h"

#pragma prolog(DFHPEP, "SAVE(14,12),LR(12,15)")
#pragma epilog(DFHPEP, "RETURN(14,12)")

int DFHPEP(struct dfhpeppar *parm) {

    if (parm->peptype == PEP_TYPE_ASRA) {

        /* Skip CICS system transactions */
        if (match_field(parm->peptran, "CECI", 4) ||
            match_field(parm->peptran, "CEBR", 4)) {
            return CICS_PEP_CONTINUE;
        }

        /* Issue alert WTO */
        char msg[80];
        int pos = 0;
        msg_append_str(msg, &pos, "DFHPEP ASRA TRAN=");
        msg_append_field(msg, &pos, parm->peptran, 4);
        msg_append_str(msg, &pos, " PGM=");
        msg_append_field(msg, &pos, parm->pepprog, 8);
        wto_write(msg, pos, WTO_ROUTE_MASTER_CONSOLE | WTO_ROUTE_PROGRAMMER_INFO, 0);
    }

    return CICS_PEP_CONTINUE;
}
```

### Pattern 2: Task Attach GLUE Exit

**Metal C:**
```c
#include "metalc_base.h"
#include "metalc_cics.h"

#pragma prolog(MYATTACH, "SAVE(14,12),LR(12,15)")
#pragma epilog(MYATTACH, "RETURN(14,12)")

int MYATTACH(struct dfhglueparm *parm) {
    struct tctte *tctte;

    /* Only process task-attach function */
    if (parm->func != CICS_FUNC_ATTACH) {
        return CICS_UERCNORM;
    }

    tctte = (struct tctte *)parm->gluetctte;
    if (tctte == NULL) {
        return CICS_UERCNORM;
    }

    /* Log new task */
    char msg[60];
    int pos = 0;
    msg_append_str(msg, &pos, "CICS TASK TRAN=");
    msg_append_field(msg, &pos, tctte->tcttetrid, 4);
    msg_append_str(msg, &pos, " TERM=");
    msg_append_field(msg, &pos, tctte->tctteti, 4);
    wto_write(msg, pos, WTO_ROUTE_PROGRAMMER_INFO, 0);

    return CICS_UERCNORM;
}
```

---

## 6. CICS-Specific Considerations

### 6.1 Address Space Context

- CICS exits run in the **CICS address space**.
- Execution mode: problem state, key 8 (for user exits under CICS TS 3+).
- Some older exits ran in supervisor state — verify for your CICS release.

### 6.2 Reentrancy

CICS exits must be **reentrant and refreshable**.  Never use static writable data.
CICS does not provide a per-call work area; use `getmain` with
`SUBPOOL_JOB_STEP` (subpool 0) for any dynamic storage.

### 6.3 EXEC CICS Commands

Metal C exits **cannot issue `EXEC CICS` commands** — they are outside the
CICS command-level API environment.  Use direct control block access or
assembler stubs for CICS services.

### 6.4 Storage Subpools

CICS manages its own storage pools.  In CICS exits:
- Subpool 0 (CICS-managed user storage): use for work areas.
- Do not touch CICS-private storage pools (keys 7, 0 areas).

### 6.5 DFHPEP Return Code SUPPRESS

`CICS_PEP_SUPPRESS` (RC=8) suppresses the abend, leaving the task in an
unknown state.  Use only when the exit has fully resolved the condition.
Incorrect use causes CICS transaction hangs or data corruption.

### 6.6 Abend Code Format

CICS abend codes in `pepabnd` are **four-character EBCDIC** strings (e.g.,
`"ASRA"`, `"ASRB"`, `"AICA"`), not binary values.  Use `match_field` for
comparison, not integer tests.

---

## 7. Build and Installation

### Compilation

```jcl
//COMPILE  EXEC PGM=CCNDRVR,PARM='METAL,LIST,NOSEQ'
//STEPLIB  DD DSN=CEE.SCEERUN2,DISP=SHR
//SYSPRINT DD SYSOUT=*
//SYSIN    DD DSN=your.src(DFHPEP),DISP=SHR
//SYSLIN   DD DSN=your.obj(DFHPEP),DISP=SHR
```

### Linkage

```jcl
//LINK     EXEC PGM=IEWL,PARM='LIST,MAP,RENT,REFR,RMODE=ANY,AMODE=31'
//SYSLIB   DD DSN=CICSTS.SDFHLOAD,DISP=SHR
//SYSLMOD  DD DSN=your.cics.loadlib(DFHPEP),DISP=SHR
//SYSLIN   DD DSN=your.obj(DFHPEP),DISP=SHR
```

### Installation — PEP

Specify DFHPEP in the CICS SIT (System Initialization Table):

```
PEP=YES
```

Or activate dynamically:

```
CEMT SET PROGRAM(DFHPEP) ENABLED
```

### Installation — GLUE Exits

```
EXEC CICS ENABLE PROGRAM(MYATTACH)
          EXIT(XATTPROG)
          START
END-EXEC
```

---

## 8. Testing

1. **CICS test region**: Use a dedicated CICS region; never test PEP changes
   first on production.
2. **CICS trace**: Enable CICS exit trace for debugging.
3. **CICS CEMT**: `CEMT I PROGRAM(DFHPEP)` to check load status.
4. **Formatted dump**: Use DFHPDX utility to format CICS dumps.

---

## 9. Verification Checklist (CICS-Specific)

- [ ] PEP return codes used correctly (CONTINUE/RETRY/SUPPRESS understood)
- [ ] GLUE return codes match the expected exit-point behaviour
- [ ] No `EXEC CICS` commands in Metal C code
- [ ] Abend code comparisons use `match_field` (4-char EBCDIC, not integer)
- [ ] Exit is reentrant — no static writable data
- [ ] CICS storage key compatibility verified for target CICS release
- [ ] Exit registered in SIT or via CEMT/PLTPI as appropriate
- [ ] DFHPEP `SUPPRESS` use fully justified and documented
- [ ] Tested on target CICS TS release (control blocks are version-sensitive)
