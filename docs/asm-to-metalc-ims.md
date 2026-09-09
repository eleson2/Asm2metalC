# AI Translation Rules: IMS Exit Assembler to Metal C

## Supplement to General Translation Rules

This document supplements the general assembler-to-Metal-C translation rules
with IMS-specific guidance.  It covers both IMS/TM (Transaction Manager) and
IMS/DB exits.

---

## 1. IMS Exit Overview

IMS provides exit points in two subsystems:

### IMS/TM (Transaction Manager)
- Message routing, scheduling, sign-on/sign-off, program error
- Exit names follow the pattern `DFS<name>` (e.g., `DFSMSCE0`, `DFSWHU00`)

### IMS/DB (Database Manager)
- Data capture, program isolation, log archive exits
- Exit names follow the pattern `DFS<name>` or installation-supplied

### Calling Convention
IMS exits receive control in **31-bit mode**, supervisor state (most exits).
R1 typically points to a **list of pointers** rather than a single parameter
block. Always load R1's first word to get the actual parameter address:

```c
/* R1 points to pointer list */
struct my_parm *parm = (struct my_parm *)parmlist[0];
```

---

## 2. Exit Architecture

### Standard IMS/TM Exit Parameter Block

Most IMS/TM exits receive an **MSCP** (Message Scheduling Control Block Pointer)
or product-specific block at `parmlist[0]`.

```c
/* Typical IMS exit entry */
#pragma prolog(DFSMSCE0, "SAVE(14,12),LR(12,15)")
#pragma epilog(DFSMSCE0, "RETURN(14,12)")

int DFSMSCE0(void **parmlist) {
    struct mscp *parm = (struct mscp *)parmlist[0];
    ...
}
```

### Sign-on Exits (DFSWHU00, DFSSGNX0)

Sign-on exits receive a pointer list with user/group information:

```c
int DFSWHU00(void **parmlist) {
    void **user_group_info = (void **)parmlist[0];
    char *userid    = (char *)user_group_info[0];   /* R3 = userid  */
    char *groupname = (char *)user_group_info[1];   /* R4 = group   */
    ...
}
```

---

## 3. IMS Control Block Structures

### MSCP — MSC Parameter Block

```c
#pragma pack(1)
struct mscp {
    char      mscpid[4];      /* +0   Eye-catcher 'MSCP'             */
    uint32_t  mscpfunc;       /* +4   Function code                  */
    uint8_t   mscpflg1;       /* +8   Flags                          */
    uint8_t   reserved1[3];   /* +9   Reserved                       */
    void     *mscpdest;       /* +12  Pointer to MSCD (destination)  */
    void     *mscpmsg;        /* +16  Pointer to message             */
};
#pragma pack()

/* mscpfunc values */
#define MSC_FUNC_INIT    0    /* Initialization                       */
#define MSC_FUNC_ROUTE   1    /* Message routing                      */
#define MSC_FUNC_TERM    2    /* Termination                          */

/* mscpflg1 bits */
#define MSCP_REQD        0x80 /* Destination was modified             */
```

### MSCD — MSC Destination Block

```c
#pragma pack(1)
struct mscd {
    char      mscdname[8];    /* +0   Destination terminal name      */
    uint8_t   mscdflg1;       /* +8   Flags                          */
    uint8_t   reserved[3];    /* +9   Reserved                       */
};
#pragma pack()

/* mscdflg1 bits */
#define MSCD_LOG         0x80 /* Logical terminal destination         */
#define MSCD_PHY         0x40 /* Physical terminal destination        */
```

### IOBUF — IMS I/O Buffer

```c
#pragma pack(1)
struct iobuf {
    char      iobufid[4];     /* +0   Eye-catcher                    */
    uint16_t  iobufl;         /* +4   Buffer length                  */
    uint8_t   iobufflg;       /* +6   Flags                          */
    uint8_t   reserved;       /* +7   Reserved                       */
    char      iobufdata[1];   /* +8   Data (variable length)         */
};
#pragma pack()
```

---

## 4. Exit Return Codes

### IMS/TM Return Codes

| Return Code | C Constant | Meaning |
|-------------|------------|---------|
| 0 | `IMS_RC_CONTINUE` | Continue normal IMS processing |
| 4 | `IMS_RC_SKIP` | Skip (exit-specific meaning) |
| 8 | `IMS_RC_ABORT` / `IMS_SGNX_DEFER` | Abort transaction / reject sign-on |
| 12 | `IMS_RC_SEVERE` | Severe error |
| 16 | `IMS_RC_TERMINATE` | Terminate IMS region |

### Sign-on Exit Return Codes

| Return Code | C Constant | Meaning |
|-------------|------------|---------|
| 0 | `IMS_SGNX_ALLOW` | Allow sign-on |
| 8 | `IMS_SGNX_DEFER` | Reject or defer to RACF |

---

## 5. Common IMS Exit Patterns

### Pattern 1: MSC Message Routing (DFSMSCE0)

**Assembler:**
```asm
DFSMSCE0 CSECT
         $ENTRY BASE=R12
         L     R2,0(,R1)         MSCP pointer
         USING MSCP,R2
         L     R3,MSCPFUNC       Function code
         CH    R3,=H'1'          Route function?
         BNE   RETURN
         L     R4,MSCPDEST       Destination block
         USING MSCD,R4
         TM    MSCDFLG1,MSCDLOG  Logical terminal?
         BZ    RETURN
         CLC   MSCDNAME(8),=CL8'TERM1   '
         BNE   RETURN
         MVC   MSCDNAME(8),=CL8'TERM2   '
         OI    MSCPFLG1,MSCPREQD
RETURN   SR    R15,R15
         BR    R14
```

**Metal C:**
```c
#include "metalc_base.h"
#include "metalc_ims.h"

#pragma prolog(DFSMSCE0, "SAVE(14,12),LR(12,15)")
#pragma epilog(DFSMSCE0, "RETURN(14,12)")

int DFSMSCE0(void **parmlist) {
    struct mscp *parm = (struct mscp *)parmlist[0];
    struct mscd *dest;

    if (parm->mscpfunc != MSC_FUNC_ROUTE) {
        return IMS_RC_CONTINUE;
    }

    dest = parm->mscpdest;

    if (TM_NONE(dest->mscdflg1, MSCD_LOG)) {
        return IMS_RC_CONTINUE;
    }

    if (match_field(dest->mscdname, "TERM1   ", 8)) {
        memcpy_inline(dest->mscdname, "TERM2   ", 8);
        OI(parm->mscpflg1, MSCP_REQD);
    }

    return IMS_RC_CONTINUE;
}
```

### Pattern 2: Sign-on with SAF Check (DFSWHU00)

The ASM codes `BAKR R14,0` / `PR`, so the pragmas are the linkage-stack pair,
not `SAVE`/`RETURN`.  The RACROUTE work area is owned by the `SAFAUTH` stub,
so the exit obtains no storage of its own.

**Metal C:**
```c
#include "metalc_base.h"
#include "metalc_ims.h"
#include "metalc_saf.h"

/* ENTITY=('IMSPROD') blank padded to the APPL class length */
static const char IMS_APPLID[8] = { 'I','M','S','P','R','O','D',' ' };

#pragma prolog(DFSWHU00, "BAKR 14,0")
#pragma epilog(DFSWHU00, "PR")

int DFSWHU00(void **parmlist) {
    void **user_group;
    const char *userid;

    if (parmlist == NULL) return IMS_SGNX_DEFER;
    user_group = (void **)parmlist[0];
    if (user_group == NULL) return IMS_SGNX_DEFER;

    userid = (const char *)user_group[0];
    if (userid == NULL) return IMS_SGNX_DEFER;

    /* RACROUTE REQUEST=AUTH,CLASS='APPL',ENTITY=('IMSPROD'),
     *          USERID=(R3),ATTR=READ                          */
    if (saf_auth_appl(IMS_APPLID, userid) != SAF_ALLOWED) {
        return IMS_SGNX_DEFER;   /* RC=8 — RACF denied, or SAF inactive */
    }
    return IMS_SGNX_ALLOW;       /* RC=0 */
}
```

Link edit must include the stub:

```
as -o SAFAUTH.o asm/stubs/SAFAUTH.asm
ld -o DFSWHU00 DFSWHU00.o SAFAUTH.o
```

See `converted/IMS/DFSWHU00.c` for the full conversion and
`docs/verification-matrices/DFSWHU00_ims.md` for the divergences.

---

## 6. IMS-Specific Considerations

### 6.1 Address Space Context

- IMS exits run in the **IMS control region** address space.
- Most TM exits: supervisor state, key 7 or key 0.
- DB exits: may run in the dependent region.

### 6.2 Reentrancy

**All IMS exits must be reentrant.** Never use static writable data.
IMS does not provide a per-invocation work area by default; allocate with
`storage_obtain` if needed and always `storage_release` before returning.
Prefer those over `getmain`/`freemain`: they map `STORAGE OBTAIN`/`RELEASE`
with `COND=YES`, so a storage shortage returns NULL instead of abending the
IMS control region.

### 6.3 SAF / RACROUTE Calls

DFSWHU00 and similar security exits call RACROUTE via SVC 119.  Metal C
cannot expand the RACROUTE macro, so the call goes through the assembler stub
`asm/stubs/SAFAUTH.asm` via `saf_auth()` / `saf_auth_appl()` in
`includes/metalc_saf.h`.  Do not inline `__asm` in the exit — see rule 8 in
`CLAUDE.md` and `docs/racroute-metalc-patterns.md`.

The wrappers apply the default-deny rule: only SAF RC=0 allows.  RC=4 (no SAF
decision — RACF or the class is inactive) and RC=8 (not authorized) both
reject, so deactivating RACF for maintenance cannot open the sign-on path.

### 6.4 MSC (Multiple Systems Coupling)

MSC exits (DFSMSCE0) modify the **destination block in place**.  Setting the
`MSCP_REQD` flag (`OI MSCPFLG1,MSCPREQD`) is mandatory to tell IMS that the
destination was changed; omitting this flag means IMS ignores the modification.

### 6.5 IMS Version Sensitivity

Control block layouts (MSCP, MSCD, IOBUF offsets) can change between IMS
releases.  Always verify field offsets against the `DFSMSCR`, `DFSMSSCD`,
and `DFSIOBUF` macros on the target IMS release.

---

## 7. Build and Installation

### Compilation

```jcl
//COMPILE  EXEC PGM=CCNDRVR,PARM='METAL,LIST,NOSEQ'
//STEPLIB  DD DSN=CEE.SCEERUN2,DISP=SHR
//SYSPRINT DD SYSOUT=*
//SYSIN    DD DSN=your.src(DFSMSCE0),DISP=SHR
//SYSLIN   DD DSN=your.obj(DFSMSCE0),DISP=SHR
```

### Linkage

```jcl
//LINK     EXEC PGM=IEWL,PARM='LIST,MAP,RENT,REFR,RMODE=ANY'
//SYSLIB   DD DSN=IMS.SDFSRESL,DISP=SHR
//SYSLMOD  DD DSN=your.imsexit(DFSMSCE0),DISP=SHR
//SYSLIN   DD DSN=your.obj(DFSMSCE0),DISP=SHR
```

### Installation

IMS exits are identified in the IMS system definition (`DFSVSMxx` or `DFSCGxxx`
members).  The exit library must be in the IMS STEPLIB or LINKLIST concatenation.

---

## 8. Testing

1. **IMS test region**: Use a dedicated IMS test subsystem to test exits without
   impacting production.
2. **IMS log**: IMS logs exit invocations; check the IMS log for unexpected
   return codes.
3. **Formatted dumps**: IMS DFSERA30 utility formats IMS control blocks for
   dump analysis.

---

## 9. Verification Checklist (IMS-Specific)

- [ ] Control block offsets verified against current DFS* macros on target IMS release
- [ ] `MSCP_REQD` flag set when MSCD destination is modified
- [ ] All RACROUTE calls go through `saf_auth()` / `saf_auth_appl()`, with
      `asm/stubs/SAFAUTH.asm` link-edited in; no `__asm` in the exit
- [ ] Exit is reentrant — no static writable data
- [ ] Work area properly `getmain`/`freemain` bracketed
- [ ] Return codes match exit point expectations for the specific exit
- [ ] Exit library in IMS STEPLIB or LINKLIST
- [ ] Tested on target IMS release (control blocks are version-sensitive)
