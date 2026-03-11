# Assembler to C Conversion Guide for AI

This document describes considerations for AI systems when converting IBM z/OS assembler code to C equivalents, specifically targeting Metal C (freestanding C without runtime library).

## 1. Understand the Target Environment

### Metal C vs Standard C

Before converting, determine the target environment:

| Aspect | Standard C | Metal C |
|--------|-----------|---------|
| Runtime library | Full libc available | **None** - no libc |
| Memory allocation | `malloc()`/`free()` | `GETMAIN`/`FREEMAIN` via SVC |
| String functions | `memcpy()`, `strlen()`, etc. | Must provide inline implementations |
| Console output | `printf()` | WTO (Write To Operator) via SVC 35 |
| Entry/exit | Compiler-managed | Manual prolog/epilog pragmas |
| Stack | Automatic | Manual save area chaining |

**Key Decision:** If the assembler code is a system exit, authorized program, or runs without Language Environment, target Metal C and implement all utilities inline.

```c
/* Metal C requires inline implementations */
static inline void *memcpy_inline(void *dest, const void *src, size_t n) {
    unsigned char *d = (unsigned char *)dest;
    const unsigned char *s = (const unsigned char *)src;
    while (n--) *d++ = *s++;
    return dest;
}
```

## 2. Register Conventions Map to Parameters

### Standard z/OS Linkage

Assembler registers have conventional meanings:

| Register | Convention | C Equivalent |
|----------|-----------|--------------|
| R0 | Parameter/work | First parameter or return value modifier |
| R1 | Parameter list pointer | `void **parmlist` or structured parameter |
| R2-R9 | Work registers | Local variables |
| R10-R11 | Base registers (often) | N/A - compiler manages |
| R12 | Base register (common) | N/A - compiler manages |
| R13 | Save area pointer | Managed by prolog/epilog |
| R14 | Return address | Managed by prolog/epilog |
| R15 | Entry point / return code | Function return value |

### Conversion Example

**Assembler:**
```asm
* Input: R0 = function code
*        R1 = parameter list address
*        R10 = JCT address
*        R15 = entry point
* Output: R15 = return code

EXIT20   $ENTRY BASE=R12
         LR    R12,R15          Set base register
         ST    R0,FUNCCODE      Save function code
         L     R3,0(,R1)        Get first parameter
```

**C Equivalent:**
```c
#pragma prolog(EXIT20, "SAVE(14,12),LR(12,15)")
#pragma epilog(EXIT20, "RETURN(14,12)")

int EXIT20(int func_code, void **parmlist, struct jct *jct) {
    void *first_parm = parmlist[0];
    /* ... */
    return rc;  /* Returned in R15 */
}
```

### Product-Specific Register Conventions

Different products have different conventions. Document these in comments:

```c
/*
 * JES2 Exit Register Conventions:
 *   R10 = JCT address
 *   R11 = HCT address
 *   R13 = PCE address
 *
 * IMS Exit Register Conventions:
 *   R1  = Parameter list
 *   R14 = Return address
 *
 * RACF Exit Register Conventions:
 *   R1  = Exit-specific parameter list (e.g., PWXPL)
 */
```

## 3. DSECT to Struct Mapping

### Basic DSECT Conversion

DSECTs (Dummy Sections) define data layouts without allocating storage. They map directly to C structures.

**Assembler DSECT:**
```asm
WORKAREA DSECT
RETCODE  DS    F              Return code (fullword)
JOBBUFP  DS    A              Pointer (address)
JCLASS   DS    CL1            Single character
JOBCLASS DS    CL8            8-character field
JCLASSL  DS    H              Halfword (short)
MSGAREA  DS    0C             Start of message area
MSGIDENT DS    XL2            2 hex bytes
MSGJOBID DS    CL8            8 characters
WORKLEN  EQU   *-WORKAREA     Length calculation
```

**C Struct:**
```c
#pragma pack(1)  /* CRITICAL: Byte alignment like assembler */

struct workarea {
    int32_t        retcode;       /* +0   Return code           */
    void          *jobbufp;       /* +4   Job buffer pointer    */
    char           jclass;        /* +8   Single character      */
    char           jobclass[8];   /* +9   8-character field     */
    int16_t        jclassl;       /* +17  Halfword              */
    /* Message area */
    uint16_t       msgident;      /* +19  Message identifier    */
    char           msgjobid[8];   /* +21  Job ID                */
};                                /* Total: 29 bytes            */

#pragma pack()
```

### Critical Considerations

1. **Always use `#pragma pack(1)`** - Assembler has no padding; C compilers add padding by default

2. **Verify offsets** - Comment offsets and verify they match assembler:
   ```c
   struct jct {
       char      jctid[4];      /* +0   'JCT ' identifier  */
       uint16_t  jctjobid;      /* +4   Job number         */
       char      jctjname[8];   /* +6   Job name           */
       /* Verify: offset of jctjname should be 6 */
   };
   ```

3. **Use fixed-width types** - Never use `int` or `long` without knowing size:
   ```c
   /* Correct */
   int32_t   fullword;    /* Always 4 bytes */
   int16_t   halfword;    /* Always 2 bytes */
   uint8_t   byte;        /* Always 1 byte  */

   /* Avoid */
   int       unknown;     /* Size varies by platform */
   ```

4. **Handle alignment fillers** - Assembler often has explicit fillers:
   ```asm
   FIELD1   DS    CL3
            DS    CL1            Alignment filler
   FIELD2   DS    F
   ```
   ```c
   struct example {
       char      field1[3];
       char      _filler1;      /* Explicit alignment */
       int32_t   field2;
   };
   ```

## 4. Macro Expansion

### Understanding Macros

Assembler macros expand to multiple instructions. Understand what they do before converting.

**Common Macro Patterns:**

| Macro | Purpose | C Equivalent |
|-------|---------|--------------|
| `SAVE (14,12)` | Save registers | Prolog pragma |
| `RETURN (14,12)` | Restore and return | Epilog pragma |
| `GETMAIN` | Allocate storage | `getmain()` inline function |
| `FREEMAIN` | Release storage | `freemain()` inline function |
| `WTO` | Write to operator | `wto_write()` inline function |
| `TIME` | Get time/date | `get_tod_clock()` or TIME SVC |
| `$SAVE`/`$RETURN` | JES2 save/return | Prolog/epilog pragmas |
| `$GETWORK`/`$RETWORK` | JES2 work area | `getmain()`/`freemain()` |

### Macro Conversion Example

**Assembler with macros:**
```asm
         SAVE  (14,12)           Save registers
         LR    R12,R15           Set base
         GETMAIN R,LV=WORKLEN    Get storage
         LR    R9,R1             Save address
         ...
         FREEMAIN R,LV=WORKLEN,A=(R9)
         RETURN (14,12)          Return
```

**C Equivalent:**
```c
#pragma prolog(myexit, "SAVE(14,12),LR(12,15)")
#pragma epilog(myexit, "RETURN(14,12)")

int myexit(void *parm) {
    struct workarea *work;

    work = (struct workarea *)getmain(sizeof(struct workarea), 0);
    if (work == NULL) return 8;

    memset_inline(work, 0, sizeof(struct workarea));

    /* ... processing ... */

    freemain(work, sizeof(struct workarea), 0);
    return 0;
}
```

## 5. Bit Manipulation and Flags

### Flag Byte Patterns

Assembler frequently uses individual bits as flags:

**Assembler:**
```asm
JCTFLG1  DS    X                 Flag byte
JCTFHELD EQU   X'80'             Bit 0 - Job held
JCTFTSU  EQU   X'40'             Bit 1 - TSO user
JCTFSTC  EQU   X'20'             Bit 2 - Started task
JCTFBAT  EQU   X'10'             Bit 3 - Batch job

* Testing flags
         TM    JCTFLG1,JCTFHELD  Test held bit
         BO    ISHELD            Branch if on

* Setting flags
         OI    JCTFLG1,JCTFHELD  Set held bit

* Clearing flags
         NI    JCTFLG1,255-JCTFHELD  Clear held bit
```

**C Equivalent:**
```c
/* Flag definitions */
#define JCTFLG1_HELD    0x80    /* Bit 0 - Job held    */
#define JCTFLG1_TSU     0x40    /* Bit 1 - TSO user    */
#define JCTFLG1_STC     0x20    /* Bit 2 - Started task */
#define JCTFLG1_BATCH   0x10    /* Bit 3 - Batch job   */

/* Utility macros */
#define BIT_TEST(val, bit)   (((val) & (bit)) != 0)
#define BIT_SET(val, bit)    ((val) |= (bit))
#define BIT_CLEAR(val, bit)  ((val) &= ~(bit))

/* Testing flags */
if (BIT_TEST(jct->jctflg1, JCTFLG1_HELD)) {
    /* Job is held */
}

/* Setting flags */
BIT_SET(jct->jctflg1, JCTFLG1_HELD);

/* Clearing flags */
BIT_CLEAR(jct->jctflg1, JCTFLG1_HELD);
```

### CLI/TM Instruction Patterns

```asm
         CLI   FIELD,C'Y'        Compare logical immediate
         BE    ISYES             Branch if equal

         TM    FLAGS,X'80'       Test under mask
         BZ    BITOFF            Branch if zero (bit off)
         BO    BITON             Branch if one (bit on)
         BM    MIXED             Branch if mixed
```

```c
/* CLI equivalent */
if (field == 'Y') {
    /* Is yes */
}

/* TM equivalents */
if ((flags & 0x80) == 0) {
    /* Bit is off */
} else if ((flags & 0x80) == 0x80) {
    /* Bit is on */
}
```

## 6. Loop and Branch Patterns

### BCT (Branch on Count)

**Assembler:**
```asm
         LA    R3,10             Load count
LOOP     ...                     Loop body
         BCT   R3,LOOP           Decrement R3, branch if non-zero
```

**C Equivalent:**
```c
for (int i = 10; i > 0; i--) {
    /* Loop body */
}

/* Or using while for closer semantic match */
int count = 10;
while (count-- > 0) {
    /* Loop body */
}
```

### BXLE/BXH (Branch on Index)

**Assembler:**
```asm
         LA    R4,TABLE          Start address
         LA    R5,8              Increment
         LA    R6,TABLEEND       End address
LOOP     ...                     Process entry
         BXLE  R4,R5,LOOP        Increment R4 by R5, branch if <= R6
```

**C Equivalent:**
```c
for (char *ptr = table; ptr <= table_end; ptr += 8) {
    /* Process entry */
}
```

### EX (Execute)

The EX instruction modifies and executes another instruction, commonly used for variable-length operations:

**Assembler:**
```asm
         LR    R5,LENGTH         Get length
         BCTR  R5,0              Subtract 1 for execute
         EX    R5,MVCLINST       Execute with modified length
         ...
MVCLINST MVC   TARGET(0),SOURCE  Length will be ORed from R5
```

**C Equivalent:**
```c
/* EX with MVC is variable-length copy */
memcpy_inline(target, source, length);
```

### TRT (Translate and Test)

**Assembler:**
```asm
* Find first non-alphabetic character
         TRT   PASSWORD,NONALPHA
         BZ    ALLALPHA          Branch if all zeros (all alpha)
         ...                     R1 = address of non-alpha, R2 = table value
```

**C Equivalent:**
```c
/* TRT equivalent - scan for characters in table */
int found_non_alpha = 0;
for (int i = 0; i < length; i++) {
    if (non_alpha_table[(unsigned char)password[i]] != 0) {
        found_non_alpha = 1;
        break;
    }
}
```

## 7. String and Memory Operations

### CLC (Compare Logical Characters)

**Assembler:**
```asm
         CLC   FIELD1,FIELD2     Compare (implicit length from FIELD1)
         BE    EQUAL

         CLC   0(8,R3),=C'KEYWORD '  Explicit length
         BE    FOUND
```

**C Equivalent:**
```c
if (memcmp_inline(field1, field2, sizeof(field1)) == 0) {
    /* Equal */
}

if (memcmp_inline(ptr, "KEYWORD ", 8) == 0) {
    /* Found */
}
```

### MVC (Move Characters)

**Assembler:**
```asm
         MVC   TARGET,SOURCE     Move (length from TARGET)
         MVC   0(8,R4),=CL8' '   Clear 8 bytes with blanks
         MVC   TARGET+1(255),TARGET  Propagate first byte (clear pattern)
```

**C Equivalent:**
```c
memcpy_inline(target, source, sizeof(target));
memcpy_inline(ptr, "        ", 8);  /* 8 blanks */
memset_inline(target, target[0], sizeof(target));  /* Propagate */
```

### MVCL (Move Long)

**Assembler:**
```asm
* Clear area: R14/R15 = target/length, R0/R1 = source/length+pad
         LR    R14,R9            Target address
         LA    R15,AREALEN       Target length
         XR    R1,R1             Source length = 0, pad = 0
         MVCL  R14,R0            Clear to zeros
```

**C Equivalent:**
```c
memset_inline(area, 0, area_length);
```

## 8. Condition Code Handling

Assembler instructions set condition codes that affect subsequent branches:

| CC | Meaning | Branch Instructions |
|----|---------|-------------------|
| 0 | Equal/Zero | BE, BZ |
| 1 | Low/Negative/Mixed | BL, BM |
| 2 | High/Positive | BH, BP |
| 3 | Overflow | BO |

**Assembler:**
```asm
         LTR   R5,R5             Load and test (sets CC)
         BZ    ISZERO            Branch if zero
         BM    ISNEG             Branch if negative
         BP    ISPOS             Branch if positive

         ICM   R3,B'1111',FIELD  Insert characters under mask
         BZ    FIELDZERO         Branch if all inserted bytes zero
```

**C Equivalent:**
```c
if (value == 0) {
    /* Is zero */
} else if (value < 0) {
    /* Is negative */
} else {
    /* Is positive */
}

/* ICM with zero check */
if (field == NULL || *(uint32_t*)field == 0) {
    /* Field is zero/null */
}
```

## 9. System Services

### WTO (Write To Operator)

**Assembler:**
```asm
         WTO   'ICHPWX01 Password rejected',ROUTCDE=(11)
```

**C Equivalent:**
```c
wto_simple("ICHPWX01 Password rejected", 26);

/* Or with routing codes */
wto_write("ICHPWX01 Password rejected", 26,
          WTO_ROUTE_PROGRAMMER_INFO, 0);
```

### GETMAIN/FREEMAIN

**Assembler:**
```asm
         GETMAIN R,LV=WORKLEN
         LR    R9,R1             Save address
         ...
         FREEMAIN R,LV=WORKLEN,A=(R9)
```

**C Equivalent:**
```c
void *work = getmain(WORK_LEN, SUBPOOL_JOB_STEP);
if (work == NULL) {
    return RC_ERROR;
}
/* ... */
freemain(work, WORK_LEN, SUBPOOL_JOB_STEP);
```

### TIME

**Assembler:**
```asm
         TIME  DEC,TIMEAREA,DATETYPE=MMDDYYYY,LINKAGE=SYSTEM
```

**C Equivalent:**
```c
/* Simplified - use STCK for basic timing */
uint64_t tod;
get_tod_clock(&tod);

/* For formatted time, implement TIME SVC wrapper */
```

## 10. Reentrant Code Considerations

### What Makes Code Reentrant

Reentrant code can be executed simultaneously by multiple callers:

1. **No static/global writable data** - All work areas must be dynamically allocated
2. **Parameters via registers/stack** - Not fixed memory locations
3. **No self-modifying code** - Code section is read-only

**Assembler Pattern:**
```asm
* Reentrant - get dynamic work area
         GETMAIN R,LV=WORKLEN
         LR    R9,R1
         USING WORKAREA,R9       Address work area
         XC    0(WORKLEN,R9),0(R9)  Clear work area
         ...
         FREEMAIN R,LV=WORKLEN,A=(R9)
```

**C Equivalent:**
```c
int myexit(void *parm) {
    /* All variables are automatic (stack) or dynamically allocated */
    struct workarea *work;
    int rc = 0;

    work = getmain(sizeof(struct workarea), 0);
    memset_inline(work, 0, sizeof(struct workarea));

    /* Never use: static int counter = 0; (not reentrant) */

    freemain(work, sizeof(struct workarea), 0);
    return rc;
}
```

### Static Read-Only Data is OK

```c
/* These are OK in reentrant code - read-only */
static const char PASSTAB[][8] = {
    "JAN     ", "FEB     ", "MAR     "
};

static const unsigned char translate_table[256] = { ... };
```

## 11. Common Conversion Mistakes

### Mistake 1: Forgetting Pack Pragma

```c
/* WRONG - compiler adds padding */
struct jct {
    char     id[4];
    uint16_t jobid;
    char     name[8];  /* May not be at offset 6! */
};

/* CORRECT */
#pragma pack(1)
struct jct {
    char     id[4];     /* +0 */
    uint16_t jobid;     /* +4 */
    char     name[8];   /* +6 - guaranteed */
};
#pragma pack()
```

### Mistake 2: Using Standard Library Functions

```c
/* WRONG in Metal C - no libc */
memcpy(dest, src, len);
printf("Debug: %d\n", value);

/* CORRECT */
memcpy_inline(dest, src, len);
wto_simple("Debug message", 13);
```

### Mistake 3: Incorrect Integer Sizes

```c
/* WRONG - size of 'int' varies */
int halfword;  /* Might be 2 or 4 bytes */

/* CORRECT - explicit sizes */
int16_t halfword;   /* Always 2 bytes */
int32_t fullword;   /* Always 4 bytes */
```

### Mistake 4: Ignoring Endianness

z/OS is big-endian. When converting multi-byte fields:

```c
/* Reading a halfword from a byte buffer */
uint16_t value = ((uint16_t)buffer[0] << 8) | buffer[1];

/* Or use the natural alignment if struct is packed */
uint16_t value = *(uint16_t *)buffer;  /* OK on z/OS */
```

### Mistake 5: Missing Prolog/Epilog

```c
/* WRONG - no linkage convention */
int myexit(void *parm) {
    return 0;
}

/* CORRECT - explicit linkage */
#pragma prolog(myexit, "SAVE(14,12),LR(12,15)")
#pragma epilog(myexit, "RETURN(14,12)")

int myexit(void *parm) {
    return 0;
}
```

## 12. Conversion Checklist

Before converting:
- [ ] Identify target environment (Metal C vs standard C)
- [ ] Document register usage at entry
- [ ] List all macros used and their expansion
- [ ] Identify all DSECTs and their field layouts
- [ ] Note any system services called (WTO, GETMAIN, etc.)

During conversion:
- [ ] Use `#pragma pack(1)` for all control block structures
- [ ] Use fixed-width integer types (int32_t, int16_t, uint8_t)
- [ ] Implement inline versions of required library functions
- [ ] Add prolog/epilog pragmas for proper linkage
- [ ] Preserve reentrancy (no static writable data)
- [ ] Document original register mappings in comments

After conversion:
- [ ] Verify structure offsets match assembler DSECTs
- [ ] Verify return codes match documented values
- [ ] Test with same inputs as original assembler
- [ ] Review for security issues introduced in translation

## 13. Reference: Common Type Mappings

| Assembler | Size | C Type |
|-----------|------|--------|
| `DS C` | 1 | `char` |
| `DS CL8` | 8 | `char[8]` |
| `DS X` | 1 | `uint8_t` |
| `DS XL4` | 4 | `uint8_t[4]` or `uint32_t` |
| `DS H` | 2 | `int16_t` |
| `DS F` | 4 | `int32_t` |
| `DS D` | 8 | `int64_t` or `double` |
| `DS A` | 4 | `void *` (31-bit) |
| `DS AD` | 8 | `void *` (64-bit) |
| `DS P` | varies | Packed decimal (special handling) |
| `DS 0C` | 0 | Label/alignment marker |
| `DS 0D` | 0 | Doubleword alignment |

## 14. Reference: Condition Code After Arithmetic

| Operation | CC=0 | CC=1 | CC=2 | CC=3 |
|-----------|------|------|------|------|
| Add/Subtract | Zero | Negative | Positive | Overflow |
| Compare | Equal | First Low | First High | - |
| Load & Test | Zero | Negative | Positive | - |
| TM (Test Mask) | All Zero | Mixed | All One | - |
| ICM | All Zero | - | Not Zero | - |

---

*Document Version: 1.0*
*Based on conversions of RACF, JES2, and IMS exits from CBTTape sources*
