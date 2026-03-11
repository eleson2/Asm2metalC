# AI Translation Rules: ACF2 Exit Assembler to Metal C

## Supplement to General Translation Rules

This document supplements the general assembler-to-Metal-C translation rules with CA ACF2 (Broadcom ACF2) specific guidance.

---

## 1. ACF2 Exit Overview

ACF2 provides numerous exit points for customizing security decisions. Key categories:

| Exit Category | Purpose |
|---------------|---------|
| Logonid exits | User validation, password processing |
| Resource exits | Dataset, resource rule evaluation |
| CICS exits | CICS-specific security |
| Utility exits | ACFRPTPP, ACFRPTRV customization |
| Validation exits | Field validation, data transformation |

### Common Exits

| Exit | Called When |
|------|-------------|
| LGNPW | Password validation |
| LGNTST | Logonid validation |
| LIDPOST | After logonid retrieval |
| INFOPRE | Before INFO rules |
| INFOPOST | After INFO rules |
| RESXPRE | Before resource rule access |
| RESXPOST | After resource rule access |

---

## 2. ACF2 Control Block Structures

### ACVALD - Validation Parameter Block

**Assembler DSECT (from ACFAEMAC):**
```asm
ACVALD   DSECT
ACVALFLG DS    X              Flags
ACVALRC  DS    X              Return code
ACVALRSN DS    H              Reason code
ACVALLID DS    CL8            Logonid
ACVALNAM DS    CL20           User name
         DS    CL2            Reserved
ACVALPWD DS    CL8            Password (masked)
ACVALNPW DS    CL8            New password (masked)
```

**Metal C:**
```c
#pragma pack(1)
struct acvald {
    unsigned char  acvalflg;         /* +0  Flags                    */
    unsigned char  acvalrc;          /* +1  Return code              */
    short          acvalrsn;         /* +2  Reason code              */
    char           acvallid[8];      /* +4  Logonid                  */
    char           acvalnam[20];     /* +12 User name                */
    char           _reserved1[2];    /* +32 Reserved                 */
    char           acvalpwd[8];      /* +34 Password (masked)        */
    char           acvalnpw[8];      /* +42 New password (masked)    */
};                                   /* Total: 50 bytes              */
#pragma pack(reset)

/* ACVALFLG bits */
#define ACVALF_NEWPWD   0x80    /* New password provided          */
#define ACVALF_VERIFY   0x40    /* Verify mode                    */
#define ACVALF_CHANGE   0x20    /* Password change request        */

/* ACVALRC return codes */
#define ACVAL_RC_ALLOW   0      /* Allow access                   */
#define ACVAL_RC_DENY    4      /* Deny access                    */
#define ACVAL_RC_REVOKE  8      /* Revoke logonid                 */
```

### ACUCB - User Control Block

```c
#pragma pack(1)
struct acucb {
    char           acucblid[8];      /* +0   Logonid                 */
    char           acucbnam[20];     /* +8   User name               */
    unsigned char  acucbflg[4];      /* +28  Flag bytes              */
    unsigned int   acucbprv[8];      /* +32  Privilege masks         */
    char           acucbpgm[8];      /* +64  Default program         */
    char           acucbgrp[8];      /* +72  Default group           */
    unsigned int   acucbpwdt;        /* +80  Password date           */
    unsigned int   acucbpwit;        /* +84  Password interval       */
    unsigned char  acucbpwvc;        /* +88  Password violation cnt  */
    unsigned char  acucbsrc[3];      /* +89  Source                  */
    /* ... additional fields vary by ACF2 version ... */
};
#pragma pack(reset)

/* ACUCBFLG[0] bits */
#define ACUCBF_SUSPEND  0x80    /* Logonid suspended              */
#define ACUCBF_CANCEL   0x40    /* Logonid cancelled              */
#define ACUCBF_NOSTAT   0x20    /* No SMF statistics              */
#define ACUCBF_RESTRICT 0x10    /* Restricted logonid             */
```

### ACFASVT - ACF2 Address Space Vector Table

The ACFASVT is the anchor for ACF2 control blocks:

```c
/* Access ACFASVT from CVT */
#define CVT_PTR       (*(void **)0x10)
#define CVTUSER_OFF   0x0CC          /* Offset to CVTUSER in CVT */

struct acfasvt *get_acfasvt(void) {
    void *cvt = CVT_PTR;
    void **cvtuser = (void **)((char *)cvt + CVTUSER_OFF);
    /* ACFASVT is typically at a fixed offset in CVTUSER chain */
    /* Actual offset depends on installation */
    return (struct acfasvt *)*cvtuser;
}
```

**Warning:** The exact method to locate ACFASVT varies by installation. Consult local documentation or existing exits.

---

## 3. ACF2 Exit Calling Convention

### Standard ACF2 Exit Linkage

**Assembler:**
```asm
MYEXIT   CSECT
         USING *,R15
         SAVE  (14,12)
         LR    R12,R15
         DROP  R15
         USING MYEXIT,R12
         L     R2,0(,R1)          First parameter
         USING ACVALD,R2
```

**Metal C:**
```c
#pragma prolog(myexit,"SAVE(14,12),LR(12,15)")
#pragma epilog(myexit,"RETURN(14,12)")

int myexit(void **parmlist) {
    struct acvald *valparm = (struct acvald *)parmlist[0];
    /* Exit logic */
    return 0;
}
```

### Return Code Convention

Most ACF2 exits use R15 for return codes:

| Return Code | Meaning |
|-------------|---------|
| 0 | Continue normal processing |
| 4 | Deny/reject (specifics vary by exit) |
| 8 | Severe error/revoke (some exits) |

---

## 4. Common ACF2 Exit Patterns

### Pattern 1: Password Validation (LGNPW)

**Assembler:**
```asm
LGNPW    CSECT
         SAVE  (14,12)
         LR    R12,R15
         USING LGNPW,R12
         L     R2,0(,R1)          ACVALD pointer
         USING ACVALD,R2
*        Check for trivial password (same as LID)
         CLC   ACVALPWD,ACVALLID  Password = Logonid?
         BE    REJECT             Yes, reject it
         LA    R15,0              Accept
         B     EXIT
REJECT   LA    R15,4              Reject
         MVC   ACVALRSN,=H'1001'  Set reason code
EXIT     RETURN (14,12),RC=(15)
```

**Metal C:**
```c
#pragma prolog(lgnpw_exit,"SAVE(14,12),LR(12,15)")
#pragma epilog(lgnpw_exit,"RETURN(14,12)")

#define RSN_TRIVIAL_PWD  1001

int lgnpw_exit(void **parmlist) {
    struct acvald *vp = (struct acvald *)parmlist[0];
    
    /* Reject if password equals logonid */
    if (memcmp_inline(vp->acvalpwd, vp->acvallid, 8) == 0) {
        vp->acvalrsn = RSN_TRIVIAL_PWD;
        return ACVAL_RC_DENY;
    }
    
    return ACVAL_RC_ALLOW;
}
```

### Pattern 2: Logonid Validation (LGNTST)

**Metal C:**
```c
#define RSN_SUSPENDED   2001
#define RSN_TIME_RESTRICT 2002

/* Time restrictions - no login between 11 PM and 6 AM */
#define RESTRICTED_START  (23 * 60 * 60 * 100)  /* 11 PM in 0.01 sec */
#define RESTRICTED_END    (6 * 60 * 60 * 100)   /* 6 AM in 0.01 sec  */

int lgntst_exit(void **parmlist) {
    struct acvald *vp = (struct acvald *)parmlist[0];
    struct acucb *ucb = (struct acucb *)parmlist[1];
    
    /* Check if logonid is suspended */
    if (ucb->acucbflg[0] & ACUCBF_SUSPEND) {
        vp->acvalrsn = RSN_SUSPENDED;
        return ACVAL_RC_DENY;
    }
    
    /* Time restriction check (example) */
    unsigned int current_time = get_current_time();  /* Implementation needed */
    if (current_time >= RESTRICTED_START || current_time <= RESTRICTED_END) {
        /* Check if user has override privilege */
        if (!(ucb->acucbprv[0] & PRIV_OVERRIDE_TIME)) {
            vp->acvalrsn = RSN_TIME_RESTRICT;
            return ACVAL_RC_DENY;
        }
    }
    
    return ACVAL_RC_ALLOW;
}
```

### Pattern 3: Resource Access Post-Processing (RESXPOST)

**Assembler:**
```asm
RESXPOST CSECT
         SAVE  (14,12)
         LR    R12,R15
         USING RESXPOST,R12
         L     R2,0(,R1)          Resource parm block
         USING ACRESBLK,R2
*        Log all denied accesses
         CLI   ACRESRC,4          Was access denied?
         BNE   EXIT               No, exit
*        Call logging routine
         LA    R1,LOGPARMS
         L     R15,=V(LOGDENIED)
         BALR  R14,R15
EXIT     LA    R15,0
         RETURN (14,12),RC=(15)
```

**Metal C:**
```c
#pragma pack(1)
struct acresblk {
    unsigned char  acresflg;         /* +0  Flags                    */
    unsigned char  acresrc;          /* +1  Return code from ACF2    */
    short          acresrsn;         /* +2  Reason code              */
    char           acresrsn_name[8]; /* +4  Resource name (8 chars)  */
    char           acrestyp[8];      /* +12 Resource type            */
    char           acreslid[8];      /* +20 Requesting logonid       */
    /* Additional fields vary */
};
#pragma pack(reset)

/* External logging function - must be linked */
extern void log_denied_access(struct acresblk *res);

int resxpost_exit(void **parmlist) {
    struct acresblk *res = (struct acresblk *)parmlist[0];
    
    /* Log all denied accesses */
    if (res->acresrc == 4) {  /* Access denied */
        log_denied_access(res);
    }
    
    return 0;  /* Don't change ACF2's decision */
}
```

### Pattern 4: Custom Field Validation

**Metal C:**
```c
/* Validate custom phone field in logonid record */
/* Format must be: NNN-NNN-NNNN */

int validate_phone(char *phone, int len) {
    if (len != 12) return 4;  /* Wrong length */
    
    /* Check format: NNN-NNN-NNNN */
    for (int i = 0; i < 12; i++) {
        if (i == 3 || i == 7) {
            if (phone[i] != '-') return 4;
        } else {
            if (phone[i] < '0' || phone[i] > '9') return 4;
        }
    }
    
    return 0;  /* Valid */
}
```

---

## 5. ACF2-Specific Considerations

### 5.1 Security Implications

**CRITICAL:** ACF2 exits control security decisions. Errors can:
- Allow unauthorized access
- Lock out legitimate users
- Create audit gaps

**Rules:**
- Test exhaustively in isolated environment
- Log all decisions during testing
- Never weaken security — only strengthen or maintain
- Review with security team before production deployment

### 5.2 Password Handling

Passwords in ACF2 exits are often masked or encrypted:

- `ACVALPWD` may contain masked password, not clear text
- Do NOT log or expose password values
- Compare passwords using ACF2-provided services when possible
- Use constant-time comparison to prevent timing attacks:

```c
/* Constant-time comparison to prevent timing attacks */
int secure_compare(const void *a, const void *b, size_t len) {
    const unsigned char *pa = a, *pb = b;
    unsigned char result = 0;
    for (size_t i = 0; i < len; i++) {
        result |= pa[i] ^ pb[i];
    }
    return result == 0 ? 0 : 1;  /* 0 if equal, 1 if different */
}
```

### 5.3 ACF2 Version Differences

Control block layouts can vary between ACF2 versions. Recommendations:

- Use ACFAEMAC macros as reference for your specific version
- Include version checks if supporting multiple versions
- Test on exact production version before deployment

### 5.4 Reentrancy and Serialization

- ACF2 exits must be reentrant
- Multiple tasks may execute simultaneously
- Use only local (stack) variables for writable data
- If shared data is required, ACF2 provides serialization services

### 5.5 Performance

ACF2 exits can be called extremely frequently:

- Every security decision may invoke exits
- Keep path length minimal
- Exit quickly for non-applicable cases
- Avoid I/O in the main decision path

---

## 6. ACF2 Macro References

When translating, reference these ACF2 macros for accurate control block mappings:

| Macro | Purpose |
|-------|---------|
| ACFAEMAC | General exit macros |
| ACFAEPWD | Password exit parameters |
| ACFAELGN | Logon exit parameters |
| ACFAERES | Resource exit parameters |
| ACUCB | User control block |
| ACFASVT | Address space vector table |

Obtain these from your ACF2 installation library (typically SYS1.ACFMAC or similar).

---

## 7. Build Considerations

### APF Authorization

ACF2 exits must run APF-authorized. Ensure:

- Load library is APF-authorized
- Program is link-edited with AC=1 (if required)

### Linkage with ACF2

```jcl
//LINK    EXEC PGM=IEWL,PARM='LIST,MAP,AC=1'
//SYSLIB   DD DSN=SYS1.ACF2.LINKLIB,DISP=SHR
//         DD DSN=CEE.SCEELKED,DISP=SHR
//SYSLMOD  DD DSN=your.acf2.exits(MYEXIT),DISP=SHR
//SYSLIN   DD DSN=your.obj(MYEXIT),DISP=SHR
//         DD *
  NAME MYEXIT(R)
/*
```

### Installation

Exits are defined via ACF2 GSO records:

```
SET CONTROL(GSO)
INSERT EXITS EXIT-name(MYEXIT) EXITMOD(MYEXIT)
```

---

## 8. Verification Checklist (ACF2-Specific)

- [ ] Return codes match ACF2 expectations (0=allow, 4=deny, etc.)
- [ ] Control block mappings verified against ACFAEMAC for your version
- [ ] Password fields are never logged or exposed
- [ ] Timing-safe comparisons used for security-sensitive data
- [ ] Exit is fully reentrant
- [ ] Error paths do not weaken security (fail secure)
- [ ] Performance tested under realistic load
- [ ] Security team review completed
- [ ] Tested with exact production ACF2 version
- [ ] Audit logging implemented for all deny decisions
