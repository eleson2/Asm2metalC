# AI Translation Rules: Mainframe Assembler to IBM Metal C

## Purpose

This document provides instructions for AI assistants translating IBM mainframe assembler code to IBM Metal C. The target environment is z/OS system exits where no Language Environment (LE) runtime is available.

---

## 1. Understanding Metal C

Metal C is IBM's C compiler option that generates code without Language Environment dependencies. It produces "bare metal" code suitable for:

- System exits and hooks
- Cross-memory services
- SRB-mode routines
- Any code that must run without LE

### Key Metal C Characteristics

- No standard C library (no printf, malloc, strcpy, etc.)
- No automatic initialization/termination
- Direct control over register usage
- Inline assembler via `__asm` statements
- HLASM-compatible object code output

---

## 2. General Translation Rules

### 2.1 Entry Point Convention

**Assembler pattern:**
```asm
MYEXIT   CSECT
         USING *,R15            Entry point addressability
         STM   R14,R12,12(R13)  Save caller's registers
         LR    R12,R15          Establish base register
         DROP  R15
         USING MYEXIT,R12
```

**Metal C equivalent:**
```c
#pragma prolog(myexit,"STM(14,12,12(13)),LR(12,15)")
#pragma epilog(myexit,"LM(14,12,12(13)),BR(14)")

void myexit(void *parm) {
    /* Function body */
}
```

**Rule:** Always generate explicit prolog/epilog pragmas. Do not assume standard linkage.

### 2.2 Register Usage

| Register | Assembler Convention | Metal C Handling |
|----------|---------------------|------------------|
| R0-R1    | Parameter/work      | Volatile, use `__asm` if specific values needed |
| R2-R11   | Work registers      | Available for compiler allocation |
| R12      | Base register       | Often reserved via `#pragma`      |
| R13      | Save area pointer   | Managed by prolog/epilog          |
| R14      | Return address      | Managed by prolog/epilog          |
| R15      | Entry point/RC      | Return value in Metal C           |

**Rule:** When assembler code expects specific register contents on exit, use `__asm` to set them explicitly.

### 2.3 Data Types and Sizes

| Assembler | Metal C | Notes |
|-----------|---------|-------|
| DS F / DC F | `int` or `int32_t` | 4-byte fullword |
| DS H / DC H | `short` or `int16_t` | 2-byte halfword |
| DS D / DC D | `long long` or `int64_t` | 8-byte doubleword |
| DS CL*n* | `char[n]` | Character array, NOT null-terminated |
| DS XL*n* | `unsigned char[n]` | Hex/binary data |
| DS A | `void *` | 4-byte address (31-bit mode) |
| DS AD | `void *` | 8-byte address (64-bit mode) |

**Rule:** Never assume null termination. Mainframe strings are typically fixed-length, space-padded.

### 2.4 Control Block Mapping

**Assembler (DSECT):**
```asm
MYBLK    DSECT
BLKFLAG  DS    X          Flag byte
BLKLEN   DS    H          Length field
         DS    X          Reserved
BLKPTR   DS    A          Pointer to data
BLKNAME  DS    CL8        8-character name
```

**Metal C (struct with explicit packing):**
```c
#pragma pack(1)
struct myblk {
    unsigned char  blkflag;     /* +0 Flag byte        */
    short          blklen;      /* +1 Length field     */
    unsigned char  _reserved;   /* +3 Reserved         */
    void          *blkptr;      /* +4 Pointer to data  */
    char           blkname[8];  /* +8 8-character name */
};                              /* Total: 16 bytes     */
#pragma pack(reset)
```

**Rules:**
- Always use `#pragma pack(1)` for control block structures
- Add offset comments for verification
- Include reserved fields to maintain alignment
- Verify total structure size matches DSECT length

### 2.5 Bit Manipulation

**Assembler:**
```asm
         TM    FLAGS,X'80'      Test high bit
         BZ    NOTSET
         OI    FLAGS,X'40'      Set bit 6
         NI    FLAGS,X'FF'-X'20' Clear bit 5
```

**Metal C:**
```c
#define FLAG_HIGH  0x80
#define FLAG_BIT6  0x40
#define FLAG_BIT5  0x20

if (flags & FLAG_HIGH) {
    flags |= FLAG_BIT6;      /* Set bit 6   */
}
flags &= ~FLAG_BIT5;         /* Clear bit 5 */
```

**Rule:** Define bit masks as named constants. Include the original hex values in comments.

### 2.6 Branching and Control Flow

**Assembler:**
```asm
         CLC   FIELD1,FIELD2
         BH    GREATER
         BL    LESSER
         B     EQUAL
```

**Metal C:**
```c
int cmp = memcmp_inline(field1, field2, len);
if (cmp > 0) {
    /* GREATER */
} else if (cmp < 0) {
    /* LESSER */
} else {
    /* EQUAL */
}
```

**Note:** `memcmp` is not available in Metal C. You must provide an inline implementation or use `__asm`.

### 2.7 String/Memory Operations Without C Library

Since standard library functions are unavailable, provide inline implementations:

```c
/* Inline memory compare */
static int memcmp_inline(const void *s1, const void *s2, size_t n) {
    const unsigned char *p1 = s1, *p2 = s2;
    while (n--) {
        if (*p1 != *p2) return *p1 - *p2;
        p1++; p2++;
    }
    return 0;
}

/* Inline memory copy */
static void memcpy_inline(void *dest, const void *src, size_t n) {
    unsigned char *d = dest;
    const unsigned char *s = src;
    while (n--) *d++ = *s++;
}

/* Inline memory set */
static void memset_inline(void *s, int c, size_t n) {
    unsigned char *p = s;
    while (n--) *p++ = (unsigned char)c;
}
```

**Alternative:** Use `__asm` with MVCL, CLC, or XC for performance-critical paths.

### 2.8 Inline Assembler for Privileged/Special Operations

**When to use `__asm`:**
- Setting specific register values for return
- Privileged instructions (not typically in exits, but possible)
- Performance-critical tight loops
- Instructions with no C equivalent (STCK, STCKF, etc.)

**Example - Return code in R15:**
```c
void myexit(void *parm) {
    int rc = 0;
    
    /* Processing logic */
    if (error_condition) {
        rc = 8;
    }
    
    __asm(" LR 15,%0" : : "r"(rc));
    return;
}
```

---

## 3. Translation Process

### Step 1: Analyze the Original Code

1. Identify the entry point and linkage convention
2. Map all DSECTs to C structures
3. List all external control blocks accessed
4. Note any self-modifying code or EXECUTE instructions
5. Identify reentrant vs. non-reentrant requirements

### Step 2: Create Header Structures

1. Define all control block structures with `#pragma pack(1)`
2. Add offset verification comments
3. Define all flag/constant values with meaningful names

### Step 3: Translate Logic

1. Convert linear code sections to C statements
2. Map branch tables to switch statements where appropriate
3. Convert loops (BCT/BXLE/BXH patterns) to for/while loops
4. Keep original assembler as comments for verification

### Step 4: Handle Special Cases

1. Replace EXECUTE instructions with direct logic where possible
2. Flag any self-modifying code for manual review
3. Use `__asm` for instructions without C equivalents

### Step 5: Verify

1. Compare C structure sizes with DSECT lengths
2. Walk through critical paths with both versions
3. Test with identical inputs for identical outputs

---

## 4. Common Patterns

### 4.1 Parameter List Access

**Assembler:**
```asm
         L     R2,0(,R1)        First parameter
         L     R3,4(,R1)        Second parameter
         TM    4(R1),X'80'      Check high bit (last parm)
```

**Metal C:**
```c
void myexit(void **parmlist) {
    void *parm1 = parmlist[0];
    void *parm2 = (void *)((uintptr_t)parmlist[1] & 0x7FFFFFFF);
    int is_last = ((uintptr_t)parmlist[1] & 0x80000000) != 0;
}
```

### 4.2 Return Code Setting

**Assembler:**
```asm
         LA    R15,0            RC=0
         L     R14,12(,R13)     Restore R14
         LM    R0,R12,20(R13)   Restore R0-R12
         BR    R14
```

**Metal C:**
```c
#pragma epilog(myexit,"L(14,12(13)),LM(0,12,20(13)),BR(14)")

int myexit(void *parm) {
    return 0;  /* Return value goes to R15 */
}
```

### 4.3 Chained Control Block Navigation

**Assembler:**
```asm
         L     R2,CVTPTR        Get CVT address
         USING CVT,R2
         L     R3,CVTTCBP       TCB/ASCB words
         L     R4,4(,R3)        Current TCB
         USING TCB,R4
```

**Metal C:**
```c
struct cvt *cvt = *(struct cvt **)0x10;  /* CVTPTR at PSA+16 */
void **tcbascb = cvt->cvttcbp;
struct tcb *tcb = tcbascb[1];            /* Current TCB */
```

---

## 5. What NOT to Translate Automatically

Flag these for manual review:

1. **Self-modifying code** - Requires redesign
2. **EXECUTE with variable target** - Needs case-by-case analysis
3. **Timing-dependent code** - May need `__asm` preservation
4. **Channel programs / EXCP** - Keep in assembler
5. **Cross-memory services** - Often needs `__asm` for PC/PT
6. **Code using AR mode** - Complex, keep in assembler initially

---

## 6. Compilation Notes

### Compiler Options

```
xlc -qmetal -S -qlist myexit.c
```

Key options:
- `-qmetal` - Enable Metal C mode
- `-S` - Generate assembler listing for verification
- `-qlist` - Generate compiler listing
- `-q64` - For 64-bit mode (if needed)
- `-qnoinline` - Disable inlining for debugging

### Linkage

Metal C object code is HLASM-compatible. Link with other assembler modules normally using the binder.

---

## 7. Verification Checklist

- [ ] All structures match DSECT offsets and total length
- [ ] Entry/exit register conventions preserved
- [ ] All bit flags defined with original hex values
- [ ] No C library functions used
- [ ] Return codes set correctly (R15)
- [ ] Reserved fields included in structures
- [ ] Reentrant code uses no static/global writable data
- [ ] Original assembler preserved as comments for critical sections
