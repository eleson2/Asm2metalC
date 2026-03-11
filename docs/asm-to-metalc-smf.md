# AI Translation Rules: SMF Exit Assembler to Metal C

## Supplement to General Translation Rules

This document supplements the general assembler-to-Metal-C translation rules with SMF-specific guidance.

---

## 1. SMF Exit Overview

SMF (System Management Facilities) exits intercept SMF records at various points:

| Exit | Point | Common Use |
|------|-------|------------|
| IEFU29 | Dump request | Suppress dumps |
| IEFU83 | Before write | Filter/modify records |
| IEFU84 | After write | Audit/copy records |
| IEFU85 | Before write (extended) | Filter/modify with context |
| IEFACTRT | Job/step termination | Augment type 30 |
| IEFUJI | Job initiation | Validate job, modify resources |
| IEFUSI | Step initiation | Control region size |
| IEFUSO | SMF buffer processing | Custom processing |

---

## 2. SMF Exit Parameter Lists

### IEFU83/IEFU84 Parameter Structure

**Assembler DSECT:**
```asm
SMFEPARM DSECT
SMESSION DS    A              SMF session ID
SMESSION_SUBSYS DS CL4        Subsystem ID
SMERECRD DS    A              Pointer to SMF record
SMERECLN DS    F              Record length
SMERETCD DS    F              Return code area
SMEFLAGS DS    X              Processing flags
```

**Metal C mapping:**
```c
#pragma pack(1)
struct smf_exit_parm {
    void          *smf_session;      /* +0  SMF session ID          */
    char           subsys_id[4];     /* +4  Subsystem ID            */
    void          *record_ptr;       /* +8  Pointer to SMF record   */
    int            record_len;       /* +12 Record length           */
    int            return_code;      /* +16 Return code area        */
    unsigned char  flags;            /* +20 Processing flags        */
};
#pragma pack(reset)

/* Return codes */
#define SMF_RC_WRITE     0    /* Write the record              */
#define SMF_RC_SUPPRESS  4    /* Suppress (do not write)       */
#define SMF_RC_HALT      8    /* Halt SMF recording            */
```

### IEFU29 Parameter Structure

```c
#pragma pack(1)
struct iefu29_parm {
    void          *dump_parm;        /* +0  Dump parameter block    */
    unsigned char  dump_type;        /* +4  Dump type code          */
    unsigned char  flags;            /* +5  Processing flags        */
    short          _reserved;        /* +6  Reserved                */
    char           jobname[8];       /* +8  Job name                */
    char           stepname[8];      /* +16 Step name               */
};
#pragma pack(reset)

/* Return codes for IEFU29 */
#define IEFU29_ALLOW     0    /* Allow dump                    */
#define IEFU29_SUPPRESS  4    /* Suppress dump                 */
```

---

## 3. SMF Record Header Structure

All SMF records share a common header:

```c
#pragma pack(1)
struct smf_header {
    short          smflen;           /* +0  Record length (includes header) */
    short          smfseg;           /* +2  Segment descriptor             */
    unsigned char  smfflg;           /* +4  System indicator flags         */
    unsigned char  smfrty;           /* +5  Record type (0-255)            */
    unsigned int   smftme;           /* +6  Time (binary 0.01 sec)         */
    unsigned char  smfdte[4];        /* +10 Date (0cyydddF packed)         */
    char           smfsid[4];        /* +14 System ID                      */
};
#pragma pack(reset)

/* Common record types */
#define SMF_TYPE_4    4     /* Step termination              */
#define SMF_TYPE_5    5     /* Job termination               */
#define SMF_TYPE_14   14    /* Dataset activity (input)      */
#define SMF_TYPE_15   15    /* Dataset activity (output)     */
#define SMF_TYPE_30   30    /* Common address space work     */
#define SMF_TYPE_42   42    /* DFSMS statistics              */
#define SMF_TYPE_70   70    /* RMF processor activity        */
#define SMF_TYPE_80   80    /* RACF processing               */
#define SMF_TYPE_89   89    /* Usage data                    */
#define SMF_TYPE_92   92    /* File system activity          */
```

---

## 4. Common SMF Exit Patterns

### Pattern 1: Record Type Filtering (IEFU83)

**Assembler:**
```asm
MYIEFU83 CSECT
         USING *,R15
         L     R2,8(,R1)          Get record pointer
         CLI   5(R2),30           Is it type 30?
         BNE   WRITEOK            No, let it write
         CLI   5(R2),42           Is it type 42?
         BE    SUPPRESS           Yes, suppress it
WRITEOK  LA    R15,0              RC=0, write record
         BR    R14
SUPPRESS LA    R15,4              RC=4, suppress
         BR    R14
```

**Metal C:**
```c
#pragma prolog(iefu83,"")
#pragma epilog(iefu83,"BR(14)")

int iefu83(struct smf_exit_parm *parm) {
    struct smf_header *record = parm->record_ptr;
    
    /* Suppress type 42 records */
    if (record->smfrty == SMF_TYPE_42) {
        return SMF_RC_SUPPRESS;
    }
    
    /* Write all other records */
    return SMF_RC_WRITE;
}
```

### Pattern 2: Record Modification

**Assembler:**
```asm
         L     R2,8(,R1)          Get record pointer
         MVC   14(4,R2),MYSID     Replace system ID
         LA    R15,0
         BR    R14
MYSID    DC    CL4'PRD1'
```

**Metal C:**
```c
int iefu83_modify(struct smf_exit_parm *parm) {
    struct smf_header *record = parm->record_ptr;
    
    /* Replace system ID */
    static const char new_sid[4] = "PRD1";
    memcpy_inline(record->smfsid, new_sid, 4);
    
    return SMF_RC_WRITE;
}
```

**Note:** For reentrant exits, avoid static data or use read-only constants only.

### Pattern 3: Job Name Based Filtering

**Assembler:**
```asm
         L     R2,8(,R1)          Record pointer
         CLI   5(R2),30           Type 30?
         BNE   WRITEOK
         LA    R3,24(,R2)         Point to type-30 jobname
         CLC   0(4,R3),=C'SYS*'   System job prefix?
         BE    SUPPRESS           Suppress system jobs
WRITEOK  LA    R15,0
         BR    R14
```

**Metal C:**
```c
/* SMF Type 30 record layout (partial) */
#pragma pack(1)
struct smf30_record {
    struct smf_header header;       /* +0  Standard header      */
    /* ... subsystem section ... */
    char              smf30jbn[8];  /* +24 Job name (offset varies by subtype) */
    /* Additional fields omitted */
};
#pragma pack(reset)

int iefu83_jobfilter(struct smf_exit_parm *parm) {
    struct smf_header *hdr = parm->record_ptr;
    
    if (hdr->smfrty != SMF_TYPE_30) {
        return SMF_RC_WRITE;
    }
    
    struct smf30_record *rec30 = parm->record_ptr;
    
    /* Suppress records for jobs starting with "SYS" */
    if (rec30->smf30jbn[0] == 'S' &&
        rec30->smf30jbn[1] == 'Y' &&
        rec30->smf30jbn[2] == 'S') {
        return SMF_RC_SUPPRESS;
    }
    
    return SMF_RC_WRITE;
}
```

### Pattern 4: Time-Based Filtering

**Assembler:**
```asm
         L     R2,8(,R1)          Record pointer
         L     R3,6(,R2)          Get time field
         C     R3,=F'3600000'     After 10:00 AM?
         BL    WRITEOK            Before 10 AM, write
         C     R3,=F'5400000'     Before 3:00 PM?
         BH    WRITEOK            After 3 PM, write
         LA    R15,4              Peak hours, suppress
         BR    R14
```

**Metal C:**
```c
/* Time is in 0.01 second units since midnight */
#define TIME_10AM  (10 * 60 * 60 * 100)   /* 3,600,000  */
#define TIME_3PM   (15 * 60 * 60 * 100)   /* 5,400,000  */

int iefu83_timefilter(struct smf_exit_parm *parm) {
    struct smf_header *hdr = parm->record_ptr;
    
    /* Suppress during peak hours (10 AM - 3 PM) */
    if (hdr->smftme >= TIME_10AM && hdr->smftme <= TIME_3PM) {
        return SMF_RC_SUPPRESS;
    }
    
    return SMF_RC_WRITE;
}
```

---

## 5. SMF-Specific Considerations

### 5.1 Performance

SMF exits are called frequently. Keep them fast:

- Avoid complex logic in the common path
- Check record type early and exit quickly for non-matching types
- Minimize memory access and branching
- Consider `__asm` for critical inner loops

### 5.2 Reentrancy

SMF exits must be reentrant. Rules:

- No writable static data
- Read-only constants (like SID replacement values) are acceptable
- Use only stack-allocated local variables
- Parameter area is provided by SMF — don't assume you can modify it

### 5.3 Record Buffer Ownership

- For IEFU83, the buffer can be modified in place
- Do NOT change the record length without updating SMERECLN
- For IEFU84, the record has already been written — modifications have no effect on the permanent record

### 5.4 Date Format

SMF date is packed decimal format `0cyydddF`:

```c
/* Extract date components from SMF packed date */
void parse_smf_date(unsigned char smfdte[4], int *year, int *day) {
    /* Format: 0cyydddF where c=century (0=19xx, 1=20xx) */
    int century = (smfdte[0] & 0x0F);
    int yy = ((smfdte[1] >> 4) * 10) + (smfdte[1] & 0x0F);
    int ddd = ((smfdte[2] >> 4) * 100) + 
              ((smfdte[2] & 0x0F) * 10) + 
              (smfdte[3] >> 4);
    
    *year = (century == 0 ? 1900 : 2000) + yy;
    *day = ddd;
}
```

---

## 6. Installation Notes

SMF exits are installed via SYS1.PARMLIB(SMFPRMxx):

```
EXITS(IEFU83,IEFU84,...)
```

Or dynamically via operator command:

```
SET SMF=xx
```

The exit load modules must be in an APF-authorized library.

### Metal C Build Example

```jcl
//COMPILE EXEC PGM=CCNDRVR,PARM='METAL,LIST,SOURCE'
//STEPLIB  DD DSN=CEE.SCEERUN,DISP=SHR
//SYSPRINT DD SYSOUT=*
//SYSIN    DD DSN=your.source(IEFU83),DISP=SHR
//SYSLIN   DD DSN=your.obj(IEFU83),DISP=SHR
```

---

## 7. Verification Checklist (SMF-Specific)

- [ ] Exit returns correct codes (0=write, 4=suppress, 8=halt)
- [ ] Record type checks use correct offset (+5 in header)
- [ ] Time comparisons account for 0.01 second units
- [ ] Date parsing handles century byte correctly
- [ ] No modification of records in IEFU84 (post-write)
- [ ] Exit is fully reentrant
- [ ] Performance tested under load
