# AI Translation Rules: IBM MQ Exit Assembler to Metal C

## Supplement to General Translation Rules

This document supplements the general assembler-to-Metal-C translation rules
with IBM MQ (formerly MQSeries) for z/OS exit-specific guidance.
It covers channel security exits, channel auto-definition exits,
and message exits.

---

## 1. IBM MQ Exit Overview

IBM MQ on z/OS provides exit points for customizing channel and queue manager behavior:

| Exit Type | Purpose | Typical Module Name |
|-----------|---------|---------------------|
| Channel Security | Authenticate partner QMs; TLS supplement | `CSQXLIB` |
| Channel Auto-Definition | Control auto-definition of channels | `CSQXDEFN` |
| Message | Intercept/modify messages on channels | `CSQXMSG` |
| Channel | Full channel lifecycle monitoring | `CSQXCHAN` |
| Cluster Workload | Route messages within a cluster | `CSQXCLWL` |

Exit names are registered in the MQ channel definitions (`CSQXCHAN`, `SECURITYEXIT`, etc.)
or in the Queue Manager configuration.

---

## 2. Exit Architecture

### MQ Channel Exit Calling Convention

MQ exits receive control via a list of pointers passed in R1:

```
R1 → ptr[0] = MQCXP (Channel Exit Parameter Block)
     ptr[1] = MQCD  (Channel Definition)
     ptr[2] = MQAXP (Agent Exit Parameters, channel exits)
     ptr[3] = MQAGC (Agent Global Control)
```

```c
#pragma prolog(CSQXLIB, "SAVE(14,12),LR(12,15)")
#pragma epilog(CSQXLIB, "RETURN(14,12)")

int CSQXLIB(void **parmlist) {
    struct mqcxp *p_cxp = (struct mqcxp *)parmlist[0];
    struct mqcd  *p_cd  = (struct mqcd  *)parmlist[1];
    ...
}
```

The exit sets its result code in `p_cxp->exitResponse` (not in R15).
R15 is always returned as 0.

---

## 3. IBM MQ Control Block Structures

### MQCXP — Channel Exit Parameter Block

```c
#pragma pack(1)
struct mqcxp {
    char      strucid[4];         /* +0   Structure identifier 'CXP ' */
    int32_t   version;            /* +4   Structure version           */
    int32_t   strucLength;        /* +8   Length of structure         */
    int32_t   exitReason;         /* +12  Why exit was called         */
    int32_t   exitResponse;       /* +16  Exit response (output)      */
    int32_t   exitResponse2;      /* +20  Secondary response (output) */
    int32_t   feedbackCode;       /* +24  Feedback code               */
    int32_t   mqccCode;           /* +28  MQ completion code          */
    int32_t   mqrcCode;           /* +32  MQ reason code              */
    char      channelName[20];    /* +36  Channel name                */
    int32_t   channelType;        /* +56  Channel type                */
    char      partnerName[20];    /* +60  Partner QM name             */
    /* Additional fields — verify against current MQ headers */
};
#pragma pack()

/* exitReason values */
#define MQXR_INIT          1   /* Exit initialization               */
#define MQXR_TERM          2   /* Exit termination                  */
#define MQXR_INIT_SEC      6   /* Security exit initialization      */
#define MQXR_SEC_MSG      11   /* Security message received         */
#define MQXR_SEND         13   /* Message about to be sent          */
#define MQXR_RECEIVE      14   /* Message received                  */

/* exitResponse values */
#define MQXCC_OK           0   /* Proceed normally                  */
#define MQXCC_SUPPRESS_FUNCTION 8 /* Suppress the function         */
#define MQXCC_CLOSE_CHANNEL   16  /* Close the channel             */
#define MQXCC_SEND_SEC_MSG    20  /* Send security message         */
#define MQXCC_SEND_AND_REQUEST 21 /* Send and request response     */
```

### MQCD — Channel Definition

```c
#pragma pack(1)
struct mqcd {
    char      strucid[4];         /* +0   Structure identifier 'CD  ' */
    int32_t   version;            /* +4   Structure version           */
    char      channelName[20];    /* +8   Channel name                */
    int32_t   channelType;        /* +28  Channel type                */
    int32_t   transportType;      /* +32  Transport type              */
    char      desc[64];           /* +36  Channel description         */
    char      qmgrName[48];       /* +100 Queue manager name          */
    char      xmitQName[48];      /* +148 Transmission queue name     */
    /* Large structure — verify against current MQ cmqxc.h */
};
#pragma pack()
```

### MQAXP — Agent Exit Parameter Block (short form)

```c
#pragma pack(1)
struct mqaxp {
    char      strucid[4];         /* +0   Structure identifier 'AXP ' */
    int32_t   version;            /* +4   Structure version           */
    int32_t   exitId;             /* +8   Exit identifier             */
    void     *agentBuffer;        /* +12  Agent buffer pointer        */
    int32_t   agentBufferLength;  /* +16  Agent buffer length         */
};
#pragma pack()
```

---

## 4. Exit Return Codes / Response Codes

**Important:** MQ exits do **not** use R15 as the return code.
The exit sets `p_cxp->exitResponse` to control MQ's behavior, then returns `0` in R15.

| exitResponse | C Constant | Meaning |
|--------------|------------|---------|
| 0 | `MQXCC_OK` | Proceed normally |
| 8 | `MQXCC_SUPPRESS_FUNCTION` | Suppress the function |
| 16 | `MQXCC_CLOSE_CHANNEL` | Close the channel (security reject) |
| 20 | `MQXCC_SEND_SEC_MSG` | Send a security message to partner |
| 21 | `MQXCC_SEND_AND_REQUEST` | Send security message and request response |

When using `MQXCC_CLOSE_CHANNEL`, MQ terminates the channel and logs the event.

---

## 5. Common MQ Exit Patterns

### Pattern 1: Channel Security Exit — Partner Allow List (CSQXLIB)

**Assembler:**
```asm
CSQXLIB  CSECT
         USING *,R12
         LR    R12,R15
         L     R2,0(,R1)          MQCXP pointer
         USING MQCXP,R2
         L     R3,4(,R1)          MQCD pointer
         USING MQCD,R3
*        Check exit reason = INIT_SEC (6)
         CLC   CXPEXITR(4),=F'6'
         BNE   RESPOND_OK
*        Check partner name against allow list
         ...
RESPOND_OK LA  R15,0
         MVC   CXPEXITRESP(4),=F'0'   MQXCC_OK
         BR    R14
RESPOND_CLOSE  ...
         MVC   CXPEXITRESP(4),=F'16'  MQXCC_CLOSE_CHANNEL
         SR    R15,R15
         BR    R14
```

**Metal C:**
```c
#include "metalc_base.h"
#include "metalc_mq.h"

static const char ALLOWED_PARTNERS[][20] = {
    "PROD.QMGR1          ",
    "PROD.QMGR2          ",
    "DR.QMGR1            "
};
#define NUM_PARTNERS (sizeof(ALLOWED_PARTNERS) / sizeof(ALLOWED_PARTNERS[0]))

#pragma prolog(CSQXLIB, "SAVE(14,12),LR(12,15)")
#pragma epilog(CSQXLIB, "RETURN(14,12)")

int CSQXLIB(void **parmlist) {
    struct mqcxp *p_cxp = (struct mqcxp *)parmlist[0];
    struct mqcd  *p_cd  = (struct mqcd  *)parmlist[1];

    /* Only handle security initialization */
    if (p_cxp->exitReason != MQXR_INIT_SEC) {
        p_cxp->exitResponse = MQXCC_OK;
        return RC_OK;
    }

    /* Log the connection attempt */
    {
        char msg[80];
        int pos = 0;
        msg_append_str(msg, &pos, "CSQXLIB CHAN=");
        msg_append_field(msg, &pos, p_cd->channelName, 20);
        msg_append_str(msg, &pos, " PARTNER=");
        msg_append_field(msg, &pos, p_cxp->partnerName, 20);
        wto_write(msg, pos, WTO_ROUTE_PROGRAMMER_INFO, 0);
    }

    /* Check allow list */
    for (int i = 0; i < (int)NUM_PARTNERS; i++) {
        if (match_field(p_cxp->partnerName, ALLOWED_PARTNERS[i], 20)) {
            p_cxp->exitResponse = MQXCC_OK;
            return RC_OK;
        }
    }

    /* Not in allow list — reject */
    char msg[80];
    int pos = 0;
    msg_append_str(msg, &pos, "CSQXLIB REJECTED PARTNER=");
    msg_append_field(msg, &pos, p_cxp->partnerName, 20);
    wto_write(msg, pos, WTO_ROUTE_MASTER_CONSOLE | WTO_ROUTE_SYSTEM_SECURITY,
              WTO_DESC_CRITICAL_ACTION);

    p_cxp->exitResponse = MQXCC_CLOSE_CHANNEL;
    return RC_OK;
}
```

---

## 6. MQ-Specific Considerations

### 6.1 Return Code Convention

**This is the single most common MQ exit conversion mistake.**

MQ exits return `0` in R15 **always**.  The actual decision is communicated via
`p_cxp->exitResponse`.  In assembler, the exit sets `CXPEXITRSP` (the response
field) and then does `SR R15,R15 / BR R14`.  Do not map R15 values to return codes.

### 6.2 Structure Versioning

MQCXP, MQCD, and other MQ structures are versioned.  The `version` field
indicates which fields are valid.  For z/OS MQ 9.x, version is typically
`MQCXP_VERSION_5` or higher.  Use the `version` field defensively if
accessing later fields.

### 6.3 Partner Name Format

`p_cxp->partnerName` is a **20-byte space-padded** field.  Always use
`match_field` with length 20, not shorter comparisons.

### 6.4 Channel Definition in Exit

The `MQCD` structure passed to the exit is a **copy** of the channel definition.
Modifying it in the exit does not permanently change the channel definition;
changes only affect the current channel instance.

### 6.5 Thread Safety

MQ channel exits are called from multiple threads simultaneously.  The exit
must be fully reentrant.  The `static const` allowed-partner table is safe;
no other static writable data is permitted.

### 6.6 MQ Channel Definition Registration

Security exits are registered in the channel definition:

```
DEFINE CHANNEL(PROD.TO.DR) CHLTYPE(SDR)
       SCYEXIT('CSQXLIB') SCYDATA('optional data')
```

Or via MQ SYSTEM.CHANNEL.INITQ for global application.

---

## 7. Build and Installation

### Compilation

```jcl
//COMPILE  EXEC PGM=CCNDRVR,PARM='METAL,LIST,NOSEQ'
//STEPLIB  DD DSN=CEE.SCEERUN2,DISP=SHR
//SYSPRINT DD SYSOUT=*
//SYSIN    DD DSN=your.src(CSQXLIB),DISP=SHR
//SYSLIN   DD DSN=your.obj(CSQXLIB),DISP=SHR
```

### Linkage

```jcl
//LINK     EXEC PGM=IEWL,PARM='LIST,MAP,RENT,REFR,RMODE=ANY,AMODE=31'
//SYSLIB   DD DSN=thlqual.SCSQLOAD,DISP=SHR
//SYSLMOD  DD DSN=your.mq.exits(CSQXLIB),DISP=SHR
//SYSLIN   DD DSN=your.obj(CSQXLIB),DISP=SHR
```

The exit library must be in the MQ channel initiator's STEPLIB.

---

## 8. Testing

1. **MQ test queue manager**: Use a dedicated QM for security exit testing.
2. **MQ trace**: `TRACE(CHIN) DETAIL(HIGH)` to capture exit invocations.
3. **Test channel startup**: `START CHANNEL(PROD.TO.DR)` and observe WTO messages.
4. **Verify rejection**: Test with an unauthorized partner QM name.

---

## 9. Verification Checklist (MQ-Specific)

- [ ] R15 always returned as 0; `exitResponse` used for all decisions
- [ ] `MQXCC_OK` set for pass-through cases (not left unset)
- [ ] `MQXCC_CLOSE_CHANNEL` used to reject (not a non-zero R15)
- [ ] Partner name comparison uses full 20-byte `match_field`
- [ ] Structure version checked if accessing fields beyond base version
- [ ] Exit registered in channel definition (`SCYEXIT`) or global exit config
- [ ] MQ channel initiator restarted after exit library update
- [ ] Exit is fully reentrant — no static writable data
- [ ] Tested with both authorized and unauthorized partner QM names
