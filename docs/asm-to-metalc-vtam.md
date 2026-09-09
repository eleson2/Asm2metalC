# AI Translation Rules: VTAM Exit Assembler to Metal C

## Supplement to General Translation Rules

This document supplements the general assembler-to-Metal-C translation rules
with VTAM (Virtual Telecommunications Access Method, now z/OS Communications
Server SNA) exit-specific guidance.

---

## 1. VTAM Exit Overview

VTAM exits are called at specific points in SNA session management:

| Exit Module | Exit Point | Purpose |
|-------------|-----------|---------|
| `ISTEXCAA` | Accounting | Session accounting |
| `ISTEXCLY` | Logon Verify | Validate logon requests |
| `ISTEXCSD` | Session Data | Access/modify session data |
| `ISTEXCDA` | Dynamic Alias | Dynamic LU alias assignment |
| `ISTEXCNM` | Node Management | Node state change notification |
| `ISTEXCUA` | Unsolicited ATTN | Unformatted message handling |

Exit module names follow the pattern `ISTEXC<xx>` (IBM reserved prefix `IST`).
Installation-written replacements often use different names registered in
`ATCSTRxx` member.

---

## 2. Exit Architecture

### Standard VTAM Exit Entry

VTAM passes a typed parameter block via R1:

```c
#pragma prolog(ISTEXCLY, "SAVE(14,12),LR(12,15)")
#pragma epilog(ISTEXCLY, "RETURN(14,12)")

int ISTEXCLY(struct vtam_ly_parm *parm) {
    if (parm->func != LY_FUNC_LOGON) {
        return VTAM_LY_DEFER;
    }
    ...
}
```

### Calling Environment

- VTAM exits run in the **VTAM address space** (`VTAM` job).
- Execution mode: supervisor state, key 0.
- Exits are non-preemptable; minimize processing time.

---

## 3. VTAM Control Block Structures

### VTAM_LY_PARM — Logon Verify Exit Parameter Block

```c
#pragma pack(1)
struct vtam_ly_parm {
    EXIT_PARM_HEADER;         /* +0   work, func, flags, reserved   */
    char      lyluname[8];    /* +8   LU name of requester          */
    char      lyappl[8];      /* +16  Application name (PLU)        */
    char      lyuserid[8];    /* +24  User ID (if provided)         */
    uint8_t   lytype;         /* +32  Logon type                    */
    uint8_t   lyflags;        /* +33  Flags                         */
    uint16_t  lyreserved;     /* +34  Reserved                      */
    uint32_t  lysense;        /* +36  Sense data (output for reject) */
    int32_t   lyreasn;        /* +40  Reason code (output)          */
};
#pragma pack()

/* func values for logon verify */
#define LY_FUNC_LOGON    1    /* New logon request                  */
#define LY_FUNC_UNBIND   2    /* Session unbind notification        */
#define LY_FUNC_SESSEND  3    /* Session end notification           */

/* lysense values for rejection */
#define VTAM_SENSE_SECURITY   0x080F0000  /* Security violation     */
#define VTAM_SENSE_INOPERATIVE 0x08400000 /* Resource inoperative   */
```

### VTAM_SD_PARM — Session Data Exit Parameter Block

```c
#pragma pack(1)
struct vtam_sd_parm {
    EXIT_PARM_HEADER;         /* +0   work, func, flags, reserved   */
    char      sdlu[8];        /* +8   LU name                       */
    char      sdappl[8];      /* +16  Application (PLU) name        */
    void     *sdsscb;         /* +24  Session control block (SSCB)  */
    uint8_t   sdflags;        /* +28  Flags                         */
    uint8_t   sdreserved[3];  /* +29  Reserved                      */
};
#pragma pack()
```

### SSCB — Session Control Block (key fields)

```c
#pragma pack(1)
struct sscb {
    char      sscbid[4];      /* +0   Eye-catcher 'SSCB'            */
    char      sscbplu[8];     /* +4   Primary LU name               */
    char      sscbslu[8];     /* +12  Secondary LU name             */
    uint8_t   sscbstat;       /* +20  Session status                */
    uint8_t   sscbflg1;       /* +21  Flags                         */
    /* Additional fields are VTAM-release sensitive */
};
#pragma pack()

/* sscbstat values */
#define SSCB_ACTIVE       0x01  /* Session is active                */
#define SSCB_PENDING      0x02  /* Session activation pending       */
```

---

## 4. Exit Return Codes

### Logon Verify Exit Return Codes

| Return Code | C Constant | Meaning |
|-------------|------------|---------|
| 0 | `VTAM_LY_ACCEPT` | Accept the logon |
| 4 | `VTAM_LY_REJECT` | Reject with `lysense` data |
| 8 | `VTAM_LY_DEFER` | Defer to RACF/SAF |

**When rejecting (RC=4):** Set `parm->lysense` to the appropriate SNA sense
code before returning.  VTAM sends this sense code to the requesting LU.

### Session Data Exit Return Codes

| Return Code | C Constant | Meaning |
|-------------|------------|---------|
| 0 | `VTAM_SD_CONTINUE` | Continue normal processing |
| 4 | `VTAM_SD_MODIFIED` | Data was modified |

---

## 5. Common VTAM Exit Patterns

### Pattern 1: Logon Verify with LU Restriction (ISTEXCLY)

**Assembler:**
```asm
ISTEXCLY CSECT
         USING *,R12
         LR    R12,R15
         L     R10,0(,R1)         LY parameter block
         USING ISTEXCLT,R10
         CLI   LYFUNC,1           Logon?
         BNE   DEFER
*        Log the attempt
         ...
*        Check if target is ADMIN application
         CLC   LYAPPL(5),=CL5'ADMIN'
         BNE   DEFER
*        Check if LU name starts with ADM
         CLC   LYLUNAME(3),=CL3'ADM'
         BE    DEFER
*        Non-admin LU to ADMIN app - reject
         MVC   LYSENSE(4),=X'080F0000'
         LA    R15,4
         BR    R14
DEFER    LA    R15,8
         BR    R14
```

**Metal C:**
```c
#include "metalc_base.h"
#include "metalc_vtam.h"

#pragma prolog(ISTEXCLY, "SAVE(14,12),LR(12,15)")
#pragma epilog(ISTEXCLY, "RETURN(14,12)")

int ISTEXCLY(struct vtam_ly_parm *parm) {

    /* Only process logon requests */
    if (parm->func != LY_FUNC_LOGON) {
        return VTAM_LY_DEFER;
    }

    /* Log the logon attempt */
    {
        char msg[80];
        int pos = 0;
        msg_append_str(msg, &pos, "ISTEXCLY LOGON LU=");
        msg_append_field(msg, &pos, parm->lyluname, 8);
        msg_append_str(msg, &pos, " APPL=");
        msg_append_field(msg, &pos, parm->lyappl, 8);
        wto_write(msg, pos, WTO_ROUTE_SYSTEM_SECURITY | WTO_ROUTE_PROGRAMMER_INFO, 0);
    }

    /* Check if target is ADMIN application */
    if (match_prefix(parm->lyappl, "ADMIN", 5)) {
        if (match_prefix(parm->lyluname, "ADM", 3)) {
            /* Admin LU — defer to RACF */
            return VTAM_LY_DEFER;
        }
        /* Non-admin LU to ADMIN app — reject */
        parm->lysense = VTAM_SENSE_SECURITY;
        parm->lyreasn = 100;
        return VTAM_LY_REJECT;
    }

    return VTAM_LY_DEFER;
}
```

---

## 6. VTAM-Specific Considerations

### 6.1 Non-Preemptable Execution

VTAM exits run in a non-preemptable environment.  They must complete quickly.
Do not issue I/O, waits, or long-running operations inside a VTAM exit.

### 6.2 SNA Sense Codes

When rejecting a logon (`VTAM_LY_REJECT`), the `lysense` field must contain a
valid **SNA sense code** (4 bytes, big-endian).  Common values:

| Sense code | Hex | Meaning |
|------------|-----|---------|
| `VTAM_SENSE_SECURITY` | `0x080F0000` | Security violation |
| `VTAM_SENSE_INOPERATIVE` | `0x08400000` | Resource inoperative |
| `VTAM_SENSE_SESSION_LIMIT` | `0x08050000` | Session limit exceeded |

Setting `lysense = 0` and returning RC=4 sends a generic session refused.

### 6.3 RACF / SAF Integration

For most logon validation, `VTAM_LY_DEFER` (RC=8) is the correct response:
VTAM will then call RACF (via SAF) to perform the security check.  Only use
`VTAM_LY_ACCEPT` or `VTAM_LY_REJECT` when the exit itself is the
authoritative security decision point.

### 6.4 VTAM Control Block Versioning

VTAM/Communications Server control block layouts change between z/OS releases.
Verify `ISTEXCLT` (logon verify DSECT) fields against the current
Communications Server customization manual.

### 6.5 ATCSTRxx Registration

VTAM exits are registered in the `ATCSTRxx` member of `SYS1.VTAMLST`:

```
EXIT ISTEXCLY,TYPE=LY,OPTION=ALWAYS
```

---

## 7. Build and Installation

### Compilation

```jcl
//COMPILE  EXEC PGM=CCNDRVR,PARM='METAL,LIST,NOSEQ'
//STEPLIB  DD DSN=CEE.SCEERUN2,DISP=SHR
//SYSPRINT DD SYSOUT=*
//SYSIN    DD DSN=your.src(ISTEXCLY),DISP=SHR
//SYSLIN   DD DSN=your.obj(ISTEXCLY),DISP=SHR
```

### Linkage

```jcl
//LINK     EXEC PGM=IEWL,PARM='LIST,MAP,RENT,REFR,RMODE=ANY,AMODE=31'
//SYSLIB   DD DSN=SYS1.VTAMLIB,DISP=SHR
//SYSLMOD  DD DSN=your.vtam.exits(ISTEXCLY),DISP=SHR
//SYSLIN   DD DSN=your.obj(ISTEXCLY),DISP=SHR
```

---

## 8. Testing

1. **Test VTAM node**: Use a dedicated VTAM system or VTAM APPL test environment.
2. **VTAM trace**: `MODIFY VTAM,TRACE,TYPE=EXIT` to trace exit invocations.
3. **Vary active**: Vary the exit active with `VARY NET,ACT,ID=ISTEXCLY`.
4. **Test rejections**: Verify SNA sense code delivery to the LU.

---

## 9. Verification Checklist (VTAM-Specific)

- [ ] SNA sense code set correctly before `VTAM_LY_REJECT` return
- [ ] Exit does not issue I/O or waits (non-preemptable environment)
- [ ] `VTAM_LY_DEFER` used as default pass-through (not ACCEPT)
- [ ] ATCSTRxx `EXIT` statement registered correctly
- [ ] Exit is reentrant — no static writable data
- [ ] Control block offsets verified against current Communications Server DSECT macros
- [ ] Tested with real SNA logon scenarios on target z/OS release
