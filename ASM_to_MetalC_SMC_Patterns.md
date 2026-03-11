# Assembler to Metal-C Migration
## Self-Modifying Code Patterns and Resolution Guide

*z/OS Mainframe Modernization Project — IBM Metal-C Translation Reference — Version 1.0*

---

## 1. Introduction

This document catalogs the common assembler coding patterns that require special handling when translating z/OS mainframe assembler exits and routines to IBM Metal-C. It focuses specifically on self-modifying code (SMC) and related dynamic instruction techniques that cannot be mechanically translated by AI tools.

The patterns described here apply to exits written for:

- SMF recording exits (IEFU83, IEFU84, IEFU29, etc.)
- ACF2 security exits
- JES2 exits (both classic `$MODULE`/`$ENTRY` style and modern dynamic exits)
- General system service routines and performance-critical code

Each pattern section describes the original assembler technique, explains why it exists, and provides a recommended Metal-C resolution strategy. Where appropriate, before/after code examples are provided.

> **WARNING:** Self-modifying code requires manual redesign. AI translation tools will flag these patterns but cannot reliably convert them. Every instance must be reviewed by a developer who understands the original intent.

---

## 2. Pattern Classification

Self-modifying and dynamic code patterns in z/OS assembler fall into three categories based on translation difficulty:

| Pattern | Translation Approach |
|---|---|
| `EXECUTE (EX)` with template instruction | Straightforward — parameterize length/offset |
| Patched branch tables | Replace with function pointer arrays |
| Dynamic BCT counter embedding | Replace with standard for/while loops |
| Computed GOTO via patched branch | Replace with switch or function pointers |
| Instruction built in storage | Manual redesign required — no direct equivalent |
| SVC table patching | Replace with proper z/OS exit infrastructure |
| Opcode injection (STAP/STC) | Full architectural redesign required |

> **NOTE:** The EX instruction pattern is by far the most common. It often appears dozens of times in a single codebase and has a well-defined Metal-C equivalent. Prioritize these first to build translator confidence.

---

## 3. EXECUTE (EX) Instruction — Variable-Length Operations

### 3.1 Description

The EXECUTE instruction (opcode `X'44'`) executes a single target instruction with a modified second byte. The register in the EX operand supplies bits 8–15 of the target instruction at execution time, without modifying storage. This is the most common "semi-SMC" pattern and appears throughout legacy mainframe code.

The primary use case is variable-length MVC, CLC, and similar SS-format instructions where the length is not known at assembly time. The EX pattern avoids the overhead of a separate loop and was the idiomatic solution in the OS/360 era.

### 3.2 Assembler Example

```asm
* Variable-length field move using EX
* R5 contains (length - 1) at runtime
         LR    R5,R4           Load length
         BCTR  R5,0            Minus 1 for SS instruction encoding
         EX    R5,MVCPAT       Execute MVC with runtime length
         B     CONTINUE
         ...
MVCPAT   MVC   TARGET(0),SOURCE   Template — length byte ORed at runtime
```

### 3.3 Metal-C Resolution

Translate EX with a length register to a C function or inline code that accepts length as a parameter. Use memcpy() equivalents — note that standard library functions are not available in Metal-C, so inline implementations are required.

```c
/* Metal-C equivalent of the EX MVC pattern */

/* Inline memory copy — standard library unavailable in Metal-C */
static void mc_memcpy(void *dest, const void *src, size_t n) {
    unsigned char *d = (unsigned char *)dest;
    const unsigned char *s = (const unsigned char *)src;
    while (n--) *d++ = *s++;
}

/* Replace EX pattern: */
mc_memcpy(target, source, length);   /* length is exact, not length-1 */
```

### 3.4 EX with CLC (Compare)

```asm
* Variable-length compare
         LR    R6,R4
         BCTR  R6,0
         EX    R6,CLCPAT
         BE    EQUAL
         BH    GREATER
CLCPAT   CLC   0(0,R7),0(R8)    Template
```

Metal-C resolution:

```c
static int mc_memcmp(const void *s1, const void *s2, size_t n) {
    const unsigned char *p1 = (const unsigned char *)s1;
    const unsigned char *p2 = (const unsigned char *)s2;
    while (n--) {
        if (*p1 != *p2) return (int)*p1 - (int)*p2;
        p1++; p2++;
    }
    return 0;
}

int result = mc_memcmp(field1, field2, length);
if (result == 0) { /* EQUAL */ }
else if (result > 0) { /* GREATER */ }
else { /* LESS */ }
```

### 3.5 AI Translation Guidance

When directing AI to handle EX patterns:

1. Identify all EX instructions and their target template labels
2. Determine what register holds the modifier and what it encodes (length-1, register number, etc.)
3. Provide the inline utility functions (`mc_memcpy`, `mc_memcmp`, `mc_memset`) in the project header
4. Instruct AI to replace EX+template pairs with calls to these utilities

---

## 4. Patched Branch Tables — Dynamic Dispatch

### 4.1 Description

Branch tables in assembler consist of a contiguous array of unconditional branch instructions (B or BC 15). In self-modifying variants, individual entries are overwritten at initialization time to redirect dispatch to different routines — effectively implementing late binding or exit chaining.

This pattern appears in JES2 (dispatch tables patched at `$ENTRY` macro expansion time), ACF2 (exit chain setup), and general-purpose dispatcher routines.

### 4.2 Assembler Example

```asm
* Branch table — entries may be patched at init time
DISPTBL  B     ROUTINE_A        Entry 0 — index 0
         B     ROUTINE_B        Entry 1
         B     ROUTINE_C        Entry 2

* Patching Entry 1 to redirect to interceptor:
PATCHIT  DS    0H
         LA    R2,INTERCEPT     New target address
         LR    R1,R2
         O     R1,=X'47F00000'  BC 15 opcode skeleton
         ST    R1,DISPTBL+4     Overwrite Entry 1 (each entry = 4 bytes)

* Dispatching via table (R3 = index 0, 1, 2...)
         SLL   R3,2             Multiply index by 4
         LA    R4,DISPTBL
         AR    R4,R3
         BR    R4               Branch into table
```

### 4.3 Metal-C Resolution

Replace patched branch tables with arrays of function pointers. This is a clean, idiomatic C equivalent with identical semantics.

```c
/* Define function pointer type matching the exit interface */
typedef int (*dispatch_fn)(void *parm, void *work);

/* Static dispatch table — initialized at startup */
static dispatch_fn dispatch_table[3] = {
    routine_a,
    routine_b,
    routine_c
};

/* Patching equivalent — replace entry at runtime */
dispatch_table[1] = intercept_routine;

/* Dispatch equivalent */
int rc = dispatch_table[index](parm, work);
```

### 4.4 Considerations for Exit Chaining

When the original branch table implements a chain of exits (each entry calls the next), preserve this by storing both the function pointer and the chain pointer:

```c
typedef struct exit_entry {
    dispatch_fn  fn;          /* This exit's handler */
    dispatch_fn  next_fn;     /* Next in chain, or NULL */
} exit_entry_t;

/* In the exit handler, call next in chain if set */
int my_exit(void *parm, void *work) {
    int rc = 0;
    /* ... exit logic ... */
    if (my_entry.next_fn != NULL) {
        rc = my_entry.next_fn(parm, work);
    }
    return rc;
}
```

---

## 5. Dynamic Branch Construction — Instructions Built in Storage

### 5.1 Description

In this pattern, a branch or other instruction is constructed entirely at runtime by assembling the opcode bytes, base register, displacement, and target address into a storage area, then branching to or through that address. This technique predates C function pointers and was used for computed GOTOs, dynamic vectoring, and self-installing routines.

> **WARNING:** This pattern cannot be translated automatically. It requires manual redesign. Flag every instance for developer review.

### 5.2 Assembler Example

```asm
* Build a BC 15 (unconditional branch) instruction in storage
BUILDBC  DS    0H
         LA    R2,TARGETRTN         Load target address
         L     R1,=X'47F00000'      BC 15,0 opcode skeleton
         OR    R1,R2                Merge address into instruction
         ST    R1,DYNBR             Write patched branch to storage
         ...
DYNBR    DC    F'0'                 Placeholder — becomes a BC instruction
         ...
* Elsewhere: branch through DYNBR
         L     R15,DYNBR
         BALR  R14,R15              Call via constructed branch
```

### 5.3 Metal-C Resolution Strategy

The correct replacement depends on what problem the original code was solving:

| Original Purpose | Metal-C Replacement |
|---|---|
| Computed GOTO (jump to one of N targets) | `switch` statement or function pointer array |
| Runtime-selected callback | Function pointer variable |
| Self-installing routine (first call patches itself) | Flag variable + conditional dispatch |
| Dynamic SVC intercept | Proper z/OS exit registration via IEAMSCHD or equivalent |
| Performance bypass after initialization | Initialized function pointer, set once at startup |

### 5.4 Self-Installing Routine Example

```c
/* Original pattern: first call patches the branch to bypass init */
/* Metal-C equivalent using a flag: */

static int initialized = 0;
static my_work_t work_area;

int my_exit(void *parm) {
    if (!initialized) {
        /* One-time initialization */
        mc_memset(&work_area, 0, sizeof(work_area));
        work_area.version = CURRENT_VERSION;
        initialized = 1;
    }
    /* Main exit logic using work_area */
    return process_exit(parm, &work_area);
}
```

---

## 6. BCT Loop Counter Embedding

### 6.1 Description

The BCT (Branch on Count) instruction decrements a register and branches if non-zero. In some performance-critical legacy code, the initial loop count is stored directly inside the BCT instruction's register field (byte 1 of the instruction) rather than in a separate register. This embeds the loop limit into the instruction stream itself.

### 6.2 Assembler Example

```asm
* Store loop count into the register field of BCT instruction
* This patches the instruction to use a specific count
         LA    R0,100
         STC   R0,BCTINST+1        Patch byte 1 of BCT to set R1 field
LOOP     DS    0H
         ... loop body ...
BCTINST  BCT   0,LOOP              R0 field patched above; becomes BCT 1,LOOP
```

### 6.3 Metal-C Resolution

Replace with a standard for loop. This is the simplest SMC pattern to translate:

```c
/* Direct replacement — no magic required */
int i;
for (i = 0; i < count; i++) {
    /* loop body */
}

/* Or if downward-counting matters for compatibility: */
for (i = count; i > 0; i--) {
    /* loop body */
}
```

---

## 7. SVC Table Patching

### 7.1 Description

Some old system exits intercept SVCs by directly overwriting entries in the SVC table (pointed to by CVTSVCTB). The original SVC address is saved, the new intercept address is stored in the table, and the intercept routine eventually chains to the original. This requires supervisor state and key 0.

> **WARNING:** SVC table patching is technically illegal in modern z/OS. IBM documents that the SVC table is reserved and may be reorganized between releases. Use proper APF-authorized exit registration instead.

### 7.2 Metal-C Resolution

Replace with the appropriate z/OS exit interface:

- For SVC-related intercepts: use IEFUSI (SVC screening) or SVC routines registered via the SVC table installation interface
- For general intercepts: use the appropriate product exit (SMF, JES2, etc.)
- For performance monitoring: use IEAVSIFM (software instrumentation) or RMF exits

```c
/* Metal-C exit properly registered via z/OS exit infrastructure */
/* No direct SVC table manipulation */

#pragma prolog(MY_EXIT, "...")    /* Appropriate linkage for exit type */
#pragma epilog(MY_EXIT, "...")

int MY_EXIT(exit_parm_t *parm) {
    /* Exit logic here */
    /* Return code tells z/OS what to do next */
    return 0;   /* 0 = continue normal processing */
}
```

---

## 8. Opcode Injection — Full Instruction Construction

### 8.1 Description

The most extreme SMC variant: individual instruction bytes (opcode, length, base register, displacement) are assembled at runtime using MVI, STC, STCM, or OR instructions. The resulting instruction(s) are then executed directly. This was used in early system software for maximum flexibility but is virtually absent from modern exit code.

> **WARNING:** This pattern requires full architectural redesign. There is no line-for-line translation. Identify the original intent and reimplement using C idioms.

### 8.2 Assembler Example

```asm
* Build an MVC instruction entirely at runtime
         MVI   INSTR,X'D2'          MVC opcode
         STC   R5,INSTR+1           Length-1 in byte 1
         STCM  R3,B'0111',INSTR+2   Target base/displacement (3 bytes)
         STCM  R4,B'0111',INSTR+5   Source base/displacement (3 bytes)
INSTR    DS    XL6                   Receives constructed MVC instruction
         ...
         LA    R14,INSTR
         BALR  R14,R14              Execute the constructed instruction
```

### 8.3 Resolution Approach

1. Determine what the constructed instruction actually does when executed
2. Identify the range of values that could be injected (is length truly variable? is the target truly dynamic?)
3. Replace with the C equivalent of the highest-level operation being performed

In the example above, the constructed MVC is a variable-length move. The Metal-C resolution is simply:

```c
mc_memcpy(target_addr, source_addr, length);
```

---

## 9. AI Translation Prompt Guidance

### 9.1 Detection Instructions for AI

Include the following instructions in AI translation prompts to ensure SMC patterns are identified before translation begins:

```
BEFORE translating any code, scan for the following patterns and
LIST EACH OCCURRENCE with its label and line number:

1. EX instructions (opcode EX or X'44') — note the target label
2. STC or STCM writing to a label that is later branched to
3. MVI targeting a label that is later executed as an instruction
4. OR/NI/NII instructions that modify addresses before BALR/BR
5. ST instructions writing to labels in the instruction stream
6. Any label defined with DC F'0' or DS XL4/XL6 in the code flow

For each found pattern, output:
  PATTERN: [type]  LABEL: [label]  LINE: [approx location]  ACTION: [needed]

Do NOT translate self-modifying patterns automatically.
Flag them for manual review and substitute a placeholder comment.
```

### 9.2 Substitution Template for Flagged Patterns

When AI encounters an SMC pattern it cannot translate, it should emit this placeholder:

```c
/* ===================================================
 * SMC_PATTERN: [PATTERN_TYPE]
 * ORIGINAL_LABEL: [label name]
 * ORIGINAL_LINES: approximately [N] assembler instructions
 * INTENT: [describe what the code appears to do]
 * ACTION_REQUIRED: Manual redesign — see Section [N] of
 *                  Assembler-to-Metal-C SMC Pattern Guide
 * =================================================== */
/* TODO: Implement [PATTERN_TYPE] replacement here */
```

### 9.3 EX-Pattern Translation Template

For EX patterns (which CAN be translated automatically), provide AI with this template:

```c
/* Project-standard utilities — include in mc_utils.h */

static void mc_memcpy(void *d, const void *s, size_t n) {
    unsigned char *dp = (unsigned char *)d;
    const unsigned char *sp = (const unsigned char *)s;
    while (n--) *dp++ = *sp++;
}
static int mc_memcmp(const void *s1, const void *s2, size_t n) {
    const unsigned char *p1 = (const unsigned char *)s1;
    const unsigned char *p2 = (const unsigned char *)s2;
    while (n--) { if (*p1 != *p2) return (int)*p1-(int)*p2; p1++; p2++; }
    return 0;
}
static void mc_memset(void *s, unsigned char c, size_t n) {
    unsigned char *p = (unsigned char *)s; while (n--) *p++ = c;
}

/* Translation rule: EX Rn,LABELxxx  ->  mc_memxxx(dest, src, len) */
/* where len = value of Rn + 1 (EX uses length-1 encoding)          */
```

---

## 10. Quick Reference — Detection and Action

| Assembler Signature | Resolution |
|---|---|
| `EX Rx,LABELyyy` | Translate using `mc_memcpy`/`mc_memcmp` |
| `STC Rx,label+1` then `EX Rx,label` | As above — combined length/EX pattern |
| `ST Rx,DYNBR` then `L R15,DYNBR` `BR R15` | Replace with function pointer |
| `O R1,=X'47F00000'` `ST R1,TABLE+n` | Replace table entry with fn pointer array |
| `STC R0,BCTINST+1` `BCT 0,LOOP` | Replace with standard `for()` loop |
| `MVI INSTR,X'D2'` `STC Rx,INSTR+1` | Full redesign required — identify intent |
| `L R15,CVTSVCTB` `ST Rx,N(,R15)` | Replace with proper z/OS exit registration |
| `BALR R14,R14` (to constructed instr) | Full redesign — document intent first |

---

## 11. Pre-Translation Checklist

Before submitting any assembler routine to AI for Metal-C translation:

- [ ] Run a grep/search for: `EX `, `STC `, `STCM `, `MVI` targeting instruction labels
- [ ] Check for `DC F'0'` or `DS XL4`/`XL6` in the code flow (potential instruction placeholders)
- [ ] Look for `OR` instructions combining register values with opcode constants (`X'47F00000'`, `X'D2'`, etc.)
- [ ] Identify all BCT loops — verify none patch their own register field
- [ ] Check for `STx` instructions writing to labels that are later `BR`anched to
- [ ] Review `BALR`/`BR` targets — are they all static labels, or computed values?

After AI translation completes:

- [ ] Verify all `SMC_PATTERN` placeholder comments were generated for flagged patterns
- [ ] Manually redesign each flagged pattern using the appropriate section of this guide
- [ ] Confirm all EX template labels have been removed (no orphaned `MVCPAT`/`CLCPAT` labels)
- [ ] Validate structure sizes against original DSECT lengths
- [ ] Test with identical input data against both assembler and Metal-C versions
