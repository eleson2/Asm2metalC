# AI Translation Rules: JES2 Exit Assembler to Metal C

## Supplement to General Translation Rules

This document supplements the general assembler-to-Metal-C translation rules with JES2-specific guidance.

---

## 1. JES2 Exit Overview

JES2 provides extensive exit points for customizing job processing. There are two generations of exit interfaces:

### Legacy Exits ($EXIT Macro)

- Use $MODULE, $ENTRY, $SAVE, $RETURN macros
- Tightly coupled to JES2 internals
- Harder to translate to C due to JES2-specific conventions

### Dynamic Exits (Installation Exits)

- Modern interface (z/OS 1.4+)
- Cleaner parameter passing
- Better suited for Metal C translation
- Recommended for new development

---

## 2. Dynamic Exit Architecture

### Exit Routine Environment

Dynamic exits receive control via a standard interface:

```c
#pragma pack(1)
struct jes2_exit_parm {
    void          *xpl;              /* +0  Exit parameter list       */
    void          *xrt;              /* +4  Exit routine table entry  */
    unsigned int   exit_id;          /* +8  Exit identifier           */
    unsigned int   exit_point;       /* +12 Exit point within exit    */
    void          *work_area;        /* +16 Exit work area            */
    void          *jes2_anchor;      /* +20 JES2 anchor block         */
};
#pragma pack(reset)
```

### Common Exit Points

| Exit | Point | Description |
|------|-------|-------------|
| 1 | Job select | Before job selection for execution |
| 2 | Job purge | Before job purge |
| 3 | JCL scan | During JCL processing |
| 4 | Output processing | Before output processing |
| 5 | TSO submit | TSO SUBMIT processing |
| 6 | Print/punch | Spool output processing |
| 7 | Converter | JCL conversion |
| 8 | Job statement | JOB statement processing |
| 20 | END statement | Job end processing |
| 50 | SSI request | Subsystem interface requests |
| 51 | NJE | Network job entry processing |

---

## 3. JES2 Control Block Structures

### JCT - Job Control Table

```c
#pragma pack(1)
struct jct {
    char           jctid[4];         /* +0   Control block ID 'JCT '  */
    unsigned short jctjobid;         /* +4   JES2 job ID              */
    char           jctjname[8];      /* +6   Job name                 */
    char           jctjclas;         /* +14  Job class                */
    unsigned char  jctprio;          /* +15  Selection priority       */
    char           jctmclas;         /* +16  Message class            */
    char           jctroute[8];      /* +17  Execution routing        */
    unsigned int   jctflags;         /* +25  Processing flags         */
    char           jctpname[20];     /* +29  Programmer name          */
    char           jctacct[32];      /* +49  Account field            */
    /* Note: Actual offsets vary by JES2 release */
    /* Always verify against current $JCT macro */
};
#pragma pack(reset)

/* JCT flag bits (jctflags) - common examples */
#define JCTF_HELD    0x80000000    /* Job is held                   */
#define JCTF_TSU     0x40000000    /* TSO user                      */
#define JCTF_STC     0x20000000    /* Started task                  */
#define JCTF_BATCH   0x10000000    /* Batch job                     */
```

### JQE - Job Queue Element

```c
#pragma pack(1)
struct jqe {
    char           jqeid[4];         /* +0   Control block ID 'JQE '  */
    unsigned short jqejobid;         /* +4   JES2 job ID              */
    unsigned char  jqetype;          /* +6   Queue type               */
    unsigned char  jqeprio;          /* +7   Queue priority           */
    unsigned int   jqeflags;         /* +8   Status flags             */
    void          *jqejct;           /* +12  Pointer to JCT           */
    void          *jqenext;          /* +16  Next JQE in queue        */
    void          *jqeprev;          /* +20  Previous JQE in queue    */
    /* Additional fields vary by release */
};
#pragma pack(reset)

/* Queue types */
#define JQE_INPUT     1    /* Input queue                    */
#define JQE_EXECUTE   2    /* Execution queue                */
#define JQE_OUTPUT    3    /* Output queue                   */
#define JQE_HARDCOPY  4    /* Hardcopy queue                 */
#define JQE_PURGE     5    /* Purge queue                    */
```

### PCE - Processor Control Element

```c
#pragma pack(1)
struct pce {
    char           pceid[4];         /* +0   Control block ID 'PCE '  */
    unsigned char  pcetype;          /* +4   Processor type           */
    unsigned char  pceflags;         /* +5   Status flags             */
    unsigned short pcereserved;      /* +6   Reserved                 */
    void          *pcejct;           /* +8   Current JCT pointer      */
    void          *pceanchr;         /* +12  JES2 anchor              */
    char           pcework[256];     /* +16  Work area                */
    /* Actual layout varies significantly */
};
#pragma pack(reset)
```

---

## 4. Exit Return Codes

### Standard Return Code Convention

| Return Code | Meaning |
|-------------|---------|
| 0 | Continue normal processing |
| 4 | Skip current function, continue job |
| 8 | Fail the job |
| 12 | Bypass exit point processing |
| 16 | Terminate JES2 (use with extreme caution) |

### Exit-Specific Variations

Return code meanings vary by exit point. Always verify against the specific exit documentation.

---

## 5. Common JES2 Exit Patterns

### Pattern 1: Job Selection (Exit 1)

**Assembler (Legacy):**
```asm
EXIT1    $MODULE
         $ENTRY BASE=R12
         $SAVE
         L     R2,0(,R1)          JCT pointer
         USING JCT,R2
*        Skip jobs with class Z
         CLI   JCTJCLAS,C'Z'
         BE    SKIP
         LA    R15,0              Allow job
         B     RETURN
SKIP     LA    R15,4              Skip this job
RETURN   $RETURN RC=(R15)
```

**Metal C (Dynamic Exit):**
```c
#pragma prolog(exit1_select,"SAVE(14,12),LR(12,15)")
#pragma epilog(exit1_select,"RETURN(14,12)")

#define RC_CONTINUE  0
#define RC_SKIP      4

int exit1_select(struct jes2_exit_parm *parm) {
    struct jct *jct = (struct jct *)parm->work_area;
    
    /* Skip jobs with class 'Z' */
    if (jct->jctjclas == 'Z') {
        return RC_SKIP;
    }
    
    return RC_CONTINUE;
}
```

### Pattern 2: Job Statement Processing (Exit 8)

**Metal C:**
```c
#pragma pack(1)
struct exit8_parm {
    struct jct    *jct;              /* JCT pointer                   */
    char          *job_stmt;         /* JOB statement text            */
    unsigned short job_stmt_len;     /* Statement length              */
    unsigned int   flags;            /* Processing flags              */
};
#pragma pack(reset)

#define EXIT8_RC_ACCEPT   0    /* Accept JOB statement          */
#define EXIT8_RC_REJECT   8    /* Reject job                    */

int exit8_job_stmt(struct jes2_exit_parm *parm) {
    struct exit8_parm *ep = (struct exit8_parm *)parm->xpl;
    struct jct *jct = ep->jct;
    
    /* Enforce naming convention: jobs must start with department code */
    /* Valid prefixes: PAY, FIN, HR, IT */
    static const char *valid_prefix[] = {"PAY", "FIN", "HR", "IT"};
    static const int prefix_count = 4;
    
    int valid = 0;
    for (int i = 0; i < prefix_count; i++) {
        if (memcmp_inline(jct->jctjname, valid_prefix[i], 3) == 0) {
            valid = 1;
            break;
        }
    }
    
    /* STC and TSU are exempt from naming convention */
    if (jct->jctflags & (JCTF_STC | JCTF_TSU)) {
        valid = 1;
    }
    
    if (!valid) {
        /* Job name doesn't follow convention */
        /* Note: In real exit, would set error message */
        return EXIT8_RC_REJECT;
    }
    
    return EXIT8_RC_ACCEPT;
}
```

### Pattern 3: Output Routing (Exit 4)

**Metal C:**
```c
struct exit4_parm {
    struct jct    *jct;              /* JCT pointer                   */
    struct jqe    *jqe;              /* Job queue element             */
    char          *dest;             /* Destination pointer           */
    unsigned short dest_len;         /* Destination length            */
};

/* Route SYSOUT based on job class */
int exit4_output(struct jes2_exit_parm *parm) {
    struct exit4_parm *ep = (struct exit4_parm *)parm->xpl;
    struct jct *jct = ep->jct;
    
    /* Route class 'X' output to special printer */
    if (jct->jctmclas == 'X') {
        /* Modify destination */
        static const char special_dest[8] = "PRT001  ";
        memcpy_inline(ep->dest, special_dest, 8);
    }
    
    /* Route class 'H' to HOLD */
    if (jct->jctmclas == 'H') {
        /* Set hold flag in JQE */
        ep->jqe->jqeflags |= 0x80000000;  /* HELD flag */
    }
    
    return 0;
}
```

### Pattern 4: Resource Limit Enforcement (Exit 20)

**Metal C:**
```c
struct exit20_parm {
    struct jct    *jct;              /* JCT pointer                   */
    unsigned int   cpu_time;         /* CPU time used (seconds)       */
    unsigned int   print_lines;      /* Lines printed                 */
    unsigned int   cards_punched;    /* Cards punched                 */
};

#define MAX_CPU_SECONDS    3600      /* 1 hour max CPU                */
#define MAX_PRINT_LINES    1000000   /* 1 million lines max           */

int exit20_limits(struct jes2_exit_parm *parm) {
    struct exit20_parm *ep = (struct exit20_parm *)parm->xpl;
    struct jct *jct = ep->jct;
    
    /* Log excessive resource usage */
    if (ep->cpu_time > MAX_CPU_SECONDS ||
        ep->print_lines > MAX_PRINT_LINES) {
        /* Would invoke logging routine here */
        /* log_resource_abuse(jct, ep); */
    }
    
    /* Could set flags for future job restrictions */
    
    return 0;
}
```

---

## 6. JES2-Specific Considerations

### 6.1 JES2 Address Space Context

JES2 exits run in the JES2 address space with special considerations:

- **Supervisor state**: Most exits run in supervisor state
- **System key**: Typically key 1
- **Cross-memory**: JES2 uses cross-memory services extensively
- **Checkpoint processing**: Some exits interact with checkpoint data

### 6.2 JES2 Serialization

JES2 uses its own serialization mechanisms:

```c
/* JES2 provides these services - DO NOT implement your own */
/* These would be called via __asm or JES2 service macros */

/* $QSUSE - Obtain queue serialization */
/* $QSREL - Release queue serialization */
/* $CKPT  - Checkpoint services */
```

**Rule:** Never hold JES2 serialization across I/O or long operations.

### 6.3 Dynamic Exit Registration

Dynamic exits are registered via JES2 initialization:

```
EXIT001  ROUTINE=MYEXIT01,ENABLE,TRACE=NO
```

Or dynamically via operator command:

```
$T EXIT(1),ROUTINE=MYEXIT01,ENABLE
```

### 6.4 Exit Work Area

JES2 provides a work area for exit use:

- Size configured at exit definition
- Persists for duration of exit routine
- Use for temporary data only
- Do not assume contents between calls

### 6.5 Control Block Versioning

**CRITICAL:** JES2 control block layouts change between releases.

**Recommendations:**
- Always verify offsets against current $JCT, $JQE, etc. macros
- Use symbolic offsets where possible
- Include version checks in production code
- Test on target z/OS release

```c
/* Example version check */
#define JES2_MIN_VERSION  0x0240    /* z/OS 2.4 minimum */

int check_jes2_version(struct jes2_exit_parm *parm) {
    /* Access JES2 version from anchor block */
    unsigned short version = *(unsigned short *)
        ((char *)parm->jes2_anchor + VERSION_OFFSET);
    
    if (version < JES2_MIN_VERSION) {
        return -1;  /* Version not supported */
    }
    return 0;
}
```

### 6.6 Message Issuance

JES2 provides message services. For Metal C, you'll need to interface via `__asm`:

```c
/* Simplified example - actual implementation requires JES2 services */
void issue_message(const char *msgid, const char *text) {
    __asm(
        " LA    1,%0           Message parameter area    \n"
        " L     15,=V($MSG)    JES2 message routine      \n"
        " BALR  14,15                                    \n"
        : : "m"(*text) : "0", "1", "14", "15"
    );
}
```

---

## 7. Legacy Exit Migration Strategy

When translating legacy $EXIT-based exits:

### Step 1: Understand the Legacy Code

1. Document all $SAVE/$RETURN patterns
2. Map all DSECT usages to structures
3. Identify all JES2 service calls ($MSG, $QSUSE, etc.)

### Step 2: Evaluate Conversion Path

**Consider converting to dynamic exit if:**
- The exit is straightforward logic
- No heavy use of JES2 internal services
- Performance is not hyper-critical

**Keep in assembler if:**
- Exit uses many JES2 internal services
- Exit is performance-critical (micro-optimizations matter)
- Exit has complex interactions with JES2 internals

### Step 3: Hybrid Approach

For complex exits, consider:

1. Metal C for main logic
2. Assembler stubs for JES2 service calls
3. Link both together

```c
/* Metal C main logic */
extern int jes2_get_job_info(void *anchor, struct jct *jct);
extern void jes2_issue_msg(const char *msgid);

int my_exit_logic(struct jes2_exit_parm *parm) {
    struct jct jct_local;
    
    /* Call assembler stub for JES2 service */
    if (jes2_get_job_info(parm->jes2_anchor, &jct_local) != 0) {
        jes2_issue_msg("MYEX001E");
        return 8;
    }
    
    /* C logic here */
    
    return 0;
}
```

---

## 8. Build and Installation

### Compilation

```jcl
//COMPILE EXEC PGM=CCNDRVR,PARM='METAL,LIST,LP64'
//STEPLIB  DD DSN=CEE.SCEERUN,DISP=SHR
//SYSPRINT DD SYSOUT=*
//SYSIN    DD DSN=your.source(EXIT001),DISP=SHR
//SYSLIN   DD DSN=your.obj(EXIT001),DISP=SHR
```

### Linkage

JES2 exits link with JES2 libraries:

```jcl
//LINK    EXEC PGM=IEWL,PARM='LIST,MAP,RENT,REFR'
//SYSLIB   DD DSN=SYS1.JES2.SHASLINK,DISP=SHR
//         DD DSN=CEE.SCEELKED,DISP=SHR
//SYSLMOD  DD DSN=your.jes2.exits(EXIT001),DISP=SHR
//SYSLIN   DD DSN=your.obj(EXIT001),DISP=SHR
```

### Dynamic Exit Definition

In JES2 PARM:

```
EXIT001  ROUTINE=EXIT001,
         STATUS=ENABLED,
         TRACE=NO,
         WORKSIZE=1024
```

---

## 9. Testing

### JES2 Exit Testing Strategy

1. **Isolated JES2**: Test on development LPAR with separate JES2
2. **Trace mode**: Enable exit trace during testing
3. **Limited scope**: Start with disabled exit, enable for specific jobs
4. **Rollback plan**: Have assembler version ready to restore

### Test Commands

```
$T EXIT(1),ROUTINE=EXIT001,ENABLE     Enable exit
$T EXIT(1),ROUTINE=EXIT001,DISABLE    Disable exit
$D EXIT(1)                            Display exit status
$T EXIT(1),TRACE=YES                  Enable trace
```

---

## 10. Verification Checklist (JES2-Specific)

- [ ] Control block offsets verified against current JES2 macros
- [ ] Return codes match exit point expectations
- [ ] Exit is reentrant and refreshable (RENT, REFR)
- [ ] No JES2 serialization held across long operations
- [ ] Work area usage within configured size
- [ ] Tested on target z/OS release
- [ ] Fallback assembler exit available
- [ ] Exit trace tested and working
- [ ] Performance compared to assembler version
- [ ] Message IDs registered (if issuing messages)
- [ ] Documentation updated for operations team
