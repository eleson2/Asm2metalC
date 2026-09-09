# AI Translation Rules: NetView Exit Assembler to Metal C

## Supplement to General Translation Rules

This document supplements the general assembler-to-Metal-C translation rules
with IBM NetView for z/OS exit-specific guidance.  It covers command
preprocessing exits, message modification exits, and operator command exits.

---

## 1. NetView Exit Overview

IBM NetView for z/OS provides installation exits for:

| Exit Module | Exit Point | Purpose |
|-------------|-----------|---------|
| `DSIEX01` | Command preprocessing | Intercept/suppress commands before execution |
| `DSIEX02` | Message preprocessing | Intercept/modify messages |
| `DSIEX03` | Session exit | Terminal session lifecycle |
| `DSIEX04` | Operator command | Filter operator commands |
| `DSIEX09` | Hard copy | Control hardcopy logging |

Exit names follow the pattern `DSIEX<nn>` (IBM prefix `DSI`).
Exits are activated via the NetView `CNMSTYLE` member or `LOADMOD` initialization.

---

## 2. Exit Architecture

### Command Preprocessing Exit (DSIEX01)

DSIEX01 is called before every command issued to NetView.  It receives a typed
parameter block via R1.  The exit can allow, suppress, or modify the command.

```c
#pragma prolog(DSIEX01, "SAVE(14,12),LR(12,15)")
#pragma epilog(DSIEX01, "RETURN(14,12)")

int DSIEX01(struct nv_cmd_parm *parm) {
    if (parm->func != NV_FUNC_PRE) {
        return NV_CMD_CONTINUE;
    }
    ...
}
```

The parameter block uses `EXIT_PARM_HEADER` with fields at offsets +4, +5, +6
carrying the function, command type, and authorization flags respectively.

---

## 3. NetView Control Block Structures

### NV_CMD_PARM — Command Preprocessing Exit Parameter Block

```c
#pragma pack(1)
struct nv_cmd_parm {
    EXIT_PARM_HEADER;         /* +0   work(+0), func(+4), flags(+5), reserved(+6) */
    /* func (at +4)     = nvfunc   (1=pre, 2=post, etc.)                */
    /* flags (at +5)    = nvcmdtype (1=NV cmd, 2=MVS cmd, 4=VTAM cmd)  */
    /* reserved (at +6) = nvflags  (x'8000'=auth, x'4000'=console)     */
    char     *nvcmd;          /* +8   Pointer to command text           */
    uint32_t  nvcmdlen;       /* +12  Command text length               */
    char      nvoper[8];      /* +16  Operator ID issuing the command   */
    char      nvdomain[8];    /* +24  NetView domain name               */
    int32_t   nvreasn;        /* +32  Reason code (output for suppress) */
    uint8_t   nvpadding[4];   /* +36  Alignment                         */
};
#pragma pack()

/* func values (at EXIT_PARM_HEADER +4) */
#define NV_FUNC_PRE      1    /* Pre-processing (before execution)      */
#define NV_FUNC_POST     2    /* Post-processing (after execution)      */
#define NV_FUNC_INIT     9    /* Exit initialization                    */

/* flags values (nvcmdtype, at EXIT_PARM_HEADER +5) */
#define NV_CMDTYPE_NV    1    /* NetView internal command               */
#define NV_CMDTYPE_MVS   2    /* MVS operator command (MODIFY, CANCEL)  */
#define NV_CMDTYPE_VTAM  4    /* VTAM operator command                  */
#define NV_CMDTYPE_JES   8    /* JES operator command                   */

/* authorization flags (nvflags, at EXIT_PARM_HEADER +6, 2-byte) */
#define NV_FLG_AUTH      0x8000   /* Operator has AUTHTASK authority    */
#define NV_FLG_CONSOLE   0x4000   /* Command came from hardware console */
#define NV_FLG_NOLOG     0x2000   /* Do not log this command            */
```

### NV_MSG_PARM — Message Exit Parameter Block

```c
#pragma pack(1)
struct nv_msg_parm {
    EXIT_PARM_HEADER;         /* +0   work, func, flags, reserved       */
    char     *nvmsgtext;      /* +8   Pointer to message text           */
    uint32_t  nvmsglen;       /* +12  Message text length               */
    char      nvmsgid[8];     /* +16  Message identifier                */
    char      nvdomain[8];    /* +24  NetView domain                    */
    char      nvsource[8];    /* +32  Source of message                 */
    uint8_t   nvroute;        /* +40  Routing code                      */
    uint8_t   nvdesc;         /* +41  Descriptor code                   */
    uint8_t   nvreserved[2];  /* +42  Reserved                          */
};
#pragma pack()
```

---

## 4. Exit Return Codes

### Command Preprocessing Exit Return Codes

| Return Code | C Constant | Meaning |
|-------------|------------|---------|
| 0 | `NV_CMD_CONTINUE` | Allow the command to execute |
| 4 | `NV_CMD_SUPPRESS` | Suppress the command |

**When returning `NV_CMD_SUPPRESS` (4):** Set `parm->nvreasn` to a non-zero
reason code for the NetView log.  The operator will receive a "command not
accepted" message.

### Message Preprocessing Exit Return Codes

| Return Code | C Constant | Meaning |
|-------------|------------|---------|
| 0 | `NV_MSG_CONTINUE` | Process the message normally |
| 4 | `NV_MSG_SUPPRESS` | Suppress the message |
| 8 | `NV_MSG_MODIFIED` | Message text was modified |

---

## 5. Common NetView Exit Patterns

### Pattern 1: Command Authorization Filter (DSIEX01)

**Assembler:**
```asm
DSIEX01  CSECT
         USING *,R12
         LR    R12,R15
         L     R10,0(,R1)         NV command parm block
         USING NVXCPARM,R10
         CLI   NVFUNC,1            Pre-processing?
         BNE   RETURN
*        Check command type
         CLI   NVCMDTYPE,2         MVS command?
         BNE   CHKVTAM
*        Check authority
         TM    NVFLAGS,X'80'       Authorized?
         BO    RETURN
*        Non-auth: check for CANCEL
         L     R5,NVPCMDP          Command text pointer
         L     R6,NVPCMDL          Command text length
         CHI   R6,6
         BL    RETURN
         CLC   0(6,R5),=CL6'CANCEL'
         BE    REJECT
         CHI   R6,5
         BL    RETURN
         CLC   0(5,R5),=CL5'FORCE'
         BE    REJECT
         B     RETURN
REJECT   ...
         LA    R15,4
         BR    R14
CHKVTAM  ...
RETURN   SR    R15,R15
         BR    R14
```

**Metal C:**
```c
#include "metalc_base.h"
#include "metalc_netview.h"

#pragma prolog(DSIEX01, "SAVE(14,12),LR(12,15)")
#pragma epilog(DSIEX01, "RETURN(14,12)")

int DSIEX01(struct nv_cmd_parm *parm) {

    /* Only pre-processing */
    if (parm->func != NV_FUNC_PRE) {
        return NV_CMD_CONTINUE;
    }

    if (parm->flags == NV_CMDTYPE_MVS) {
        /* Authorized operators pass through */
        if (TM_ALL(parm->reserved, NV_FLG_AUTH)) {
            return NV_CMD_CONTINUE;
        }

        /* Non-auth: block CANCEL and FORCE */
        char *cmd  = parm->nvcmd;
        uint32_t len = parm->nvcmdlen;

        if (len >= 6 && match_prefix(cmd, "CANCEL", 6)) {
            goto REJECT;
        }
        if (len >= 5 && match_prefix(cmd, "FORCE", 5)) {
            goto REJECT;
        }

    } else if (parm->flags == NV_CMDTYPE_VTAM) {
        /* Audit VTAM commands */
        char msg[80];
        int pos = 0;
        msg_append_str(msg, &pos, "DSIEX01 VTAM CMD OPER=");
        msg_append_field(msg, &pos, parm->nvoper, 8);
        wto_write(msg, pos, WTO_ROUTE_PROGRAMMER_INFO, 0);
    }

    return NV_CMD_CONTINUE;

REJECT:
    {
        char msg[80];
        int pos = 0;
        msg_append_str(msg, &pos, "DSIEX01 BLOCKED OPER=");
        msg_append_field(msg, &pos, parm->nvoper, 8);
        msg_append_str(msg, &pos, " NOT AUTHORIZED");
        wto_write(msg, pos, WTO_ROUTE_MASTER_CONSOLE | WTO_ROUTE_SYSTEM_SECURITY,
                  WTO_DESC_IMMEDIATE_ACTION);
    }
    parm->nvreasn = 100;
    return NV_CMD_SUPPRESS;
}
```

---

## 6. NetView-Specific Considerations

### 6.1 EXIT_PARM_HEADER Field Remapping

The NetView command exit parameter block reuses the `EXIT_PARM_HEADER` fields:
- `parm->func` (+4): `nvfunc` — function code (1=pre, 2=post)
- `parm->flags` (+5): `nvcmdtype` — command type (1=NV, 2=MVS, 4=VTAM)
- `parm->reserved` (+6): `nvflags` — 2-byte authorization flags

Note that `nvflags` is **2 bytes** overlapping the 2-byte `reserved` field.
The `NV_FLG_AUTH` bit is `0x8000`, checked with `TM_ALL(parm->reserved, NV_FLG_AUTH)`.

### 6.2 Command Text Pointer

`parm->nvcmd` is a **pointer** to the command text, not the text itself.
The command text is in NetView-managed storage; do not write to it from the exit.
Use `match_prefix` for read-only comparisons.

### 6.3 Suppress vs. Log

Suppressing a command (`NV_CMD_SUPPRESS`) generates a NetView log entry.
Always set `nvreasn` to a non-zero value to provide context in the log.

### 6.4 NetView Address Space

Exits run in the **NetView address space**.  They are called from NetView
operator tasks.  Do not issue long I/O or waits that would block the operator task.

### 6.5 DSIEX01 Invocation

DSIEX01 is called for **every** command processed by NetView, including
internal automation commands.  Check the `func` field and return quickly
for calls you do not handle.

### 6.6 Activation

```
DSIEX01 LOADMOD=DSIEX01,TYPE=CMD
```

In CNMSTYLE member, or via NetView `LOAD` command.

---

## 7. Build and Installation

### Compilation

```jcl
//COMPILE  EXEC PGM=CCNDRVR,PARM='METAL,LIST,NOSEQ'
//STEPLIB  DD DSN=CEE.SCEERUN2,DISP=SHR
//SYSPRINT DD SYSOUT=*
//SYSIN    DD DSN=your.src(DSIEX01),DISP=SHR
//SYSLIN   DD DSN=your.obj(DSIEX01),DISP=SHR
```

### Linkage

```jcl
//LINK     EXEC PGM=IEWL,PARM='LIST,MAP,RENT,REFR,RMODE=ANY,AMODE=31'
//SYSLIB   DD DSN=CNM.SCNMLINK,DISP=SHR
//SYSLMOD  DD DSN=your.netview.exits(DSIEX01),DISP=SHR
//SYSLIN   DD DSN=your.obj(DSIEX01),DISP=SHR
```

---

## 8. Testing

1. **NetView test domain**: Use a dedicated NetView domain for exit testing.
2. **NetView trace**: Enable `TRACE NETVIEW` to capture exit invocations.
3. **Command simulation**: Issue test commands as authorized and non-authorized operators.
4. **Review hardcopy log**: Verify suppression messages appear correctly.

---

## 9. Verification Checklist (NetView-Specific)

- [ ] `parm->flags` correctly interpreted as `nvcmdtype` (not authorization flags)
- [ ] `parm->reserved` correctly used as 2-byte `nvflags` for authorization check
- [ ] `parm->nvcmd` treated as a read-only pointer (not modified)
- [ ] `nvreasn` set before `NV_CMD_SUPPRESS` return
- [ ] Command type checks handle all types (NV, MVS, VTAM, JES)
- [ ] Exit returns quickly for unhandled `func` values
- [ ] Exit is reentrant — no static writable data
- [ ] Registered in CNMSTYLE or via LOAD command
- [ ] Tested for authorized and non-authorized operators
- [ ] Tested for each command type: NV, MVS, VTAM
