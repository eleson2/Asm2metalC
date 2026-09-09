# AI Translation Rules: DB2 Exit Assembler to Metal C

## Supplement to General Translation Rules

This document supplements the general assembler-to-Metal-C translation rules
with DB2 (Db2 for z/OS) exit-specific guidance.  It covers authorization exits,
connection exits, change data capture exits, and audit exits.

---

## 1. DB2 Exit Overview

DB2 provides installation exits at key subsystem boundaries:

| Exit Module | Purpose |
|-------------|---------|
| `DSN3ATH` | Authorization exit — supplement or replace DB2 auth |
| `DSN3EPX` | Connection exit — control new connections |
| `DSN3@ATH` | Alternate authorization exit (mixed-case name) |
| `DSNTIAC` | CAF (Call Attach Facility) exit |
| `DSNXRPDR` | Data capture exit (change data replication) |
| `DSNACCOR` | Accounting/audit exit |

Most exits are **Installation-written** modules whose entry-point names are
registered in the DB2 DSNZPARM member.

---

## 2. Exit Architecture

### Standard DB2 Authorization Exit (DSN3ATH)

DB2 passes a single typed parameter block via R1:

```c
#pragma prolog(DSN3ATH, "SAVE(14,12),LR(12,15)")
#pragma epilog(DSN3ATH, "RETURN(14,12)")

int DSN3ATH(struct db2_ath_parm *parm) {
    /* parm->func tells the exit why it was called */
    if (parm->func != ATH_FUNC_TABLE) {
        return DB2_ATH_CONTINUE;
    }
    ...
}
```

### Connection Exit (DSN3EPX)

Connection exits receive a connection parameter block:

```c
int DSN3EPX(struct db2_epx_parm *parm) {
    /* Called at connect, disconnect, sign-on */
    ...
}
```

---

## 3. DB2 Control Block Structures

### DB2_ATH_PARM — Authorization Exit Parameter Block

```c
#pragma pack(1)
struct db2_ath_parm {
    char      athid[4];       /* +0   Eye-catcher 'ATH '            */
    uint32_t  func;           /* +4   Function code (see below)     */
    char      athauth[8];     /* +8   Authorization ID              */
    char      athobj[18];     /* +16  Object name (schema.table)    */
    uint16_t  athobjtype;     /* +34  Object type                   */
    uint32_t  athprivil;      /* +36  Privilege being checked       */
    int32_t   athreasn;       /* +40  Reason code (output)          */
    uint8_t   athflags;       /* +44  Flags                         */
    uint8_t   reserved[3];    /* +45  Reserved                      */
};
#pragma pack()

/* func values */
#define ATH_FUNC_SYSADM   1   /* SYSADM check                       */
#define ATH_FUNC_DBADM    2   /* DBADM check                        */
#define ATH_FUNC_TABLE    3   /* Table/view privilege check         */
#define ATH_FUNC_PACKAGE  4   /* Package privilege check            */
#define ATH_FUNC_PLAN     5   /* Plan privilege check               */

/* athprivil bit flags (subset) */
#define ATH_PRIV_SELECT   0x80000000  /* SELECT privilege           */
#define ATH_PRIV_INSERT   0x40000000  /* INSERT privilege           */
#define ATH_PRIV_UPDATE   0x20000000  /* UPDATE privilege           */
#define ATH_PRIV_DELETE   0x10000000  /* DELETE privilege           */
```

### DB2_EPX_PARM — Connection Exit Parameter Block

```c
#pragma pack(1)
struct db2_epx_parm {
    EXIT_PARM_HEADER;         /* +0   work, func, flags, reserved   */
    char      epxuserid[8];   /* +8   Connecting user ID            */
    char      epxjobname[8];  /* +16  Job name                      */
    char      epxplanname[8]; /* +24  Plan name                     */
    uint32_t  epxconnid;      /* +32  Connection ID                 */
    uint8_t   epxtype;        /* +36  Connection type               */
    uint8_t   epxreserved[3]; /* +37  Reserved                      */
};
#pragma pack()

/* epxtype values */
#define EPX_TYPE_BATCH    1   /* Batch connection                   */
#define EPX_TYPE_TSO      2   /* TSO connection                     */
#define EPX_TYPE_CICS     3   /* CICS connection                    */
#define EPX_TYPE_IMS      4   /* IMS connection                     */
#define EPX_TYPE_RRSAF    5   /* RRS attach                         */
```

---

## 4. Exit Return Codes

### Authorization Exit Return Codes

| Return Code | C Constant | Meaning |
|-------------|------------|---------|
| 0 | `DB2_ATH_ALLOW` | Explicitly allow the access |
| 4 | `DB2_ATH_REJECT` | Explicitly deny the access |
| 8 | `DB2_ATH_CONTINUE` | Continue with DB2 normal auth |

**Important:** `DB2_ATH_ALLOW` (0) bypasses all further DB2 authorization
checking for this object.  `DB2_ATH_CONTINUE` (8) defers to DB2's built-in
authorization, including RACF/SAF checks.

### Connection Exit Return Codes

| Return Code | C Constant | Meaning |
|-------------|------------|---------|
| 0 | `DB2_EPX_ALLOW` | Allow connection |
| 8 | `DB2_EPX_REJECT` | Reject connection |

---

## 5. Common DB2 Exit Patterns

### Pattern 1: Authorization Exit with Logging (DSN3ATH)

**Assembler:**
```asm
DSN3ATH  CSECT
         USING *,R12
         LR    R12,R15
         L     R10,0(,R1)        ATH parm block
         USING DSNXATTR,R10
         CLC   ATHFUNC(4),=F'3'  TABLE check?
         BNE   CONTINUE          No - continue
         CLC   ATHOBJ(7),=C'PAYROLL'
         BNE   CONTINUE
*        PAYROLL object - log it
         ...
*        Check for SYSADM
         CLC   ATHAUTH(8),=CL8'SYSADM  '
         BE    ALLOW
*        Reject
         MVC   ATHREASN(4),=F'200'
         LA    R15,4
         BR    R14
ALLOW    SR    R15,R15
         BR    R14
CONTINUE LA    R15,8
         BR    R14
```

**Metal C:**
```c
#include "metalc_base.h"
#include "metalc_db2.h"

#pragma prolog(DSN3ATH, "SAVE(14,12),LR(12,15)")
#pragma epilog(DSN3ATH, "RETURN(14,12)")

int DSN3ATH(struct db2_ath_parm *parm) {

    /* Only handle table access checks */
    if (parm->func != ATH_FUNC_TABLE) {
        return DB2_ATH_CONTINUE;
    }

    /* Only audit PAYROLL schema */
    if (!match_prefix(parm->athobj, "PAYROLL", 7)) {
        return DB2_ATH_CONTINUE;
    }

    /* Log the access attempt */
    {
        char msg[80];
        int pos = 0;
        msg_append_str(msg, &pos, "DSN3ATH PAYROLL AUTH=");
        msg_append_field(msg, &pos, parm->athauth, 8);
        msg_append_str(msg, &pos, " OBJ=");
        msg_append_field(msg, &pos, parm->athobj, 18);
        wto_write(msg, pos, WTO_ROUTE_SYSTEM_SECURITY | WTO_ROUTE_PROGRAMMER_INFO, 0);
    }

    /* SYSADM always allowed */
    if (match_field(parm->athauth, "SYSADM  ", 8)) {
        return DB2_ATH_ALLOW;
    }

    /* Others rejected */
    parm->athreasn = 200;
    return DB2_ATH_REJECT;
}
```

### Pattern 2: Connection Screening (DSN3EPX)

**Metal C:**
```c
#include "metalc_base.h"
#include "metalc_db2.h"

#pragma prolog(DSN3EPX, "SAVE(14,12),LR(12,15)")
#pragma epilog(DSN3EPX, "RETURN(14,12)")

int DSN3EPX(struct db2_epx_parm *parm) {

    /* Log every new connection */
    char msg[80];
    int pos = 0;
    msg_append_str(msg, &pos, "DSN3EPX DB2 CONNECT USER=");
    msg_append_field(msg, &pos, parm->epxuserid, 8);
    msg_append_str(msg, &pos, " JOB=");
    msg_append_field(msg, &pos, parm->epxjobname, 8);
    wto_write(msg, pos, WTO_ROUTE_PROGRAMMER_INFO, 0);

    /* Block direct CICS connections from unexpected plan names */
    if (parm->epxtype == EPX_TYPE_CICS) {
        if (!match_prefix(parm->epxplanname, "CICS", 4)) {
            return DB2_EPX_REJECT;
        }
    }

    return DB2_EPX_ALLOW;
}
```

---

## 6. DB2-Specific Considerations

### 6.1 Address Space Context

- Authorization exits run in the **DB2 address space** (DBM1 or DIST address spaces).
- Execution mode: supervisor state, key 7.
- Connection exits may run in either the DB2 or calling address space.

### 6.2 Authorization Exit — Allow vs. Continue

This is the most common mistake in DB2 auth exit coding:

- `DB2_ATH_ALLOW` (0): **Bypass all further DB2 auth** — use only when you are
  the authoritative source.
- `DB2_ATH_CONTINUE` (8): Let DB2 and RACF continue checking — use for
  pass-through or logging-only scenarios.

Returning `DB2_ATH_ALLOW` for objects you haven't explicitly approved can
create privilege escalation.

### 6.3 Object Name Format

`athobj` contains `schema.objectname` in a fixed 18-byte field, space-padded.
The schema and object name portions are **not separately stored** at this field.
Use `match_prefix` for HLQ/schema checks, but be aware the dot separator is
present: `"PAYROLL.EMPLOYEE      "`.

### 6.4 DB2 DSNZPARM Registration

Authorization and connection exits must be registered in DSNZPARM:

```
AUTHEXIT=DSN3ATH
CONNEXIT=DSN3EPX
```

After changing the exit load module, a DB2 restart is required to pick up
changes (exits are loaded at DB2 startup).

### 6.5 Exit Reentrancy

DB2 exits are **called concurrently** from multiple threads.  The exit must be
fully reentrant.  Never use static writable data.

---

## 7. Build and Installation

### Compilation

```jcl
//COMPILE  EXEC PGM=CCNDRVR,PARM='METAL,LIST,NOSEQ'
//STEPLIB  DD DSN=CEE.SCEERUN2,DISP=SHR
//SYSPRINT DD SYSOUT=*
//SYSIN    DD DSN=your.src(DSN3ATH),DISP=SHR
//SYSLIN   DD DSN=your.obj(DSN3ATH),DISP=SHR
```

### Linkage

```jcl
//LINK     EXEC PGM=IEWL,PARM='LIST,MAP,RENT,REFR,RMODE=ANY'
//SYSLIB   DD DSN=DSN.SDSNLOAD,DISP=SHR
//SYSLMOD  DD DSN=your.db2.exits(DSN3ATH),DISP=SHR
//SYSLIN   DD DSN=your.obj(DSN3ATH),DISP=SHR
```

The exit load library must be in DB2's STEPLIB concatenation.

---

## 8. Testing

1. **DB2 test subsystem**: Test on a dedicated DB2 subsystem.
2. **DB2 trace**: Enable `TRACE(ACCTG)` or `TRACE(AUDT)` to capture exit invocations.
3. **EXPLAIN**: Run EXPLAIN on plans/packages that should trigger the exit.
4. **DB2 DISPLAY**: `DISPLAY THREAD(*)` to see connection details.

---

## 9. Verification Checklist (DB2-Specific)

- [ ] `DB2_ATH_ALLOW` vs `DB2_ATH_CONTINUE` semantics understood and used correctly
- [ ] Object name format (`schema.object`) correctly parsed with appropriate prefix length
- [ ] Exit registered in DSNZPARM (`AUTHEXIT=`, `CONNEXIT=`)
- [ ] Exit library in DB2 STEPLIB
- [ ] DB2 restart performed after exit load module update
- [ ] Exit is fully reentrant — no static writable data
- [ ] Privilege escalation risk reviewed (ALLOW used conservatively)
- [ ] Tested on target DB2 for z/OS release
