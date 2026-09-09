# HLASM Structured Programming Macros — Recognition and C Equivalents

Many z/OS site exits are written with IBM's HLASM Structured Programming (SP) macros
rather than raw branch instructions.  These macros expand inline to branch logic but
look like structured control flow to the programmer.  This document describes each
macro, how it expands, and its direct C equivalent.

---

## 1. Overview

IBM supplies the structured programming macros as part of the HLASM Toolkit.
They reside in `SYS1.MACLIB` and are invoked with the normal MACRO mechanism.
The macro names are uppercase keywords with no sigils.

The macros do not generate DSECT or data; they generate only branch instructions.
They can appear freely inside any HLASM CSECT.

---

## 2. IF / THEN / ELSE / ENDIF

### 2.1 Syntax

```asm
         IF  (condition)
           <then-body instructions>
         ELSE
           <else-body instructions>
         ENDIF
```

The condition is a parenthesized string that uses assembler condition code (CC) symbols:

| SP condition | CC meaning | Maps to C |
|---|---|---|
| `(Z)` | CC=0 (zero) | `== 0` / equality |
| `(NZ)` | CC≠0 | `!= 0` |
| `(P)` | CC=2 (positive) | `> 0` |
| `(M)` | CC=1 (minus/negative) | `< 0` |
| `(NM)` | CC≠1 | `>= 0` |
| `(NP)` | CC≠2 | `<= 0` |
| `(O)` | CC=3 (overflow) | rare |

Conditions can be compound:
```asm
         IF  (Z),OR,(P)        ← zero OR positive
         IF  (NZ),AND,(NM)     ← not-zero AND not-minus
```

### 2.2 Before IF: the condition-setting instruction

An `IF` always follows an instruction that sets the condition code:

```asm
         CLC FIELD1,FIELD2
         IF  (Z)               ← "if previous CLC was equal"
           ...
         ENDIF
```

The CC-setting instruction is **not** inside the `IF` keyword; it appears before it.

### 2.3 C translation

Translate the CC-setting instruction and the `IF` together:

```asm
         CLC JOBNAME(8),=CL8'PROD'
         IF  (Z)
           LA  R15,0
           B   RETURN
         ELSE
           LA  R15,4
         ENDIF
```

becomes:

```c
if (match_field(parm->jobname, "PROD    ", 8)) {
    return RC_OK;
} else {
    return RC_WARNING;
}
```

### 2.4 Common CC-setting instructions and their C tests

| Instruction | C test for `(Z)` | C test for `(NZ)` |
|---|---|---|
| `CLC a,b` | `match_field(a, b, n)` | `!match_field(a, b, n)` |
| `CLI byte,val` | `byte == val` | `byte != val` |
| `LTR Rx,Rx` | `rx == 0` | `rx != 0` |
| `CH Rx,=H'n'` | `rx == n` | `rx != n` |
| `C  Rx,=F'n'` | `rx == n` | `rx != n` |
| `TM flags,mask` + `(Z)` | `TM_NONE(flags,mask)` | `TM_ANY(flags,mask)` |
| `TM flags,mask` + `(O)` | `TM_ALL(flags,mask)` | — |
| `ICM Rx,mask,field` + `(Z)` | `field == 0` (all bytes 0) | `field != 0` |

---

## 3. DO / ENDDO

### 3.1 Counted loops

```asm
         LA  R4,COUNT
         DO  WHILE=(LTR,R4,NZ,R4)   ← while R4 != 0
           ...loop body...
           BCT R4,*                  ← may be implicit: DO handles decrement
         ENDDO
```

The `DO` macro family supports:
- `DO WHILE=(cond)` — test before body
- `DO UNTIL=(cond)` — test after body
- `DO FROM=(Rx),TO=(Ry),BY=(Rz)` — counted loop with registers

C equivalent for `DO WHILE`:
```c
while (condition) {
    /* loop body */
}
```

### 3.2 BCT inside DO

`BCT Rx,label` decrements R*x* and branches if non-zero.  When inside a DO loop
it is the loop counter step:

```asm
         LA  R5,8
LOOP     EQU *
         ...body...
         BCT R5,LOOP
```

C equivalent:
```c
for (int i = 8; i > 0; i--) {
    /* body */
}
```

---

## 4. SELECT / WHEN / OTHRWISE / ENDSEL

SELECT provides a multi-way branch (switch-like):

```asm
         SELECT CLI,FIELD,NE,=C'A'   ← compare FIELD to 'A'
         WHEN (EQ)                    ← if equal
           ...
         WHEN (LT)                    ← else if less-than
           ...
         OTHRWISE                     ← else
           ...
         ENDSEL
```

The header `SELECT CLI,FIELD,NE,=C'A'` sets a comparison operand; each `WHEN`
supplies the condition to test against that comparison result.

C equivalent:
```c
if (field == 'A') {
    /* ... */
} else if (field < 'A') {
    /* ... */
} else {
    /* ... */
}
```

For `CLI` against a set of discrete values, a C `switch` is often cleaner:
```asm
         SELECT CLI,FTPCMD,EQ
         WHEN (=X'17')              ← 23 decimal
           ...
         WHEN (=X'0F')              ← 15 decimal
           ...
         ENDSEL
```

```c
switch (parm->cmd) {
case 23:  /* DELE */
    ...
    break;
case 15:  /* STOR */
    ...
    break;
}
```

---

## 5. ITERATE / LEAVE

`ITERATE` and `LEAVE` are DO loop controls equivalent to C `continue` and `break`:

```asm
         DO  WHILE=(LTR,R4,NZ,R4)
           CLI  0(R5),C' '
           IF  (Z)
             ITERATE            ← continue to next iteration
           ENDIF
           ...process non-blank...
         ENDDO
```

```c
while (r4 != 0) {
    if (*r5 == ' ') continue;
    /* process non-blank */
}
```

---

## 6. Nested Structures

SP macros nest freely.  Match them by tracking depth: each `IF` must have a matching
`ENDIF`, each `DO` a matching `ENDDO`, each `SELECT` a matching `ENDSEL`.

When translating a deeply nested structure, work inside-out:
1. Identify the innermost `IF/ENDIF` pair and translate it first.
2. Work outward.

---

## 7. Recognizing SP Macros in Unlisted Code

Not all sites use SP macros.  Signs that SP macros are present:

- `IF (`, `ELSE`, `ENDIF` tokens not preceded by `*` (not comments)
- `DO ` followed by `WHILE=`, `UNTIL=`, `FROM=`, `TO=`
- `SELECT ` followed by `CLC`, `CLI`, `C`, `CH`
- `WHEN (`, `OTHRWISE`, `ENDSEL`, `ITERATE`, `LEAVE` keywords

If you see raw labels and branch instructions (`B`, `BE`, `BNE`, `BL`, `BH`, etc.),
the source does NOT use SP macros and can be translated directly to `if/else/while`.

---

## 8. HLASM Macro Invocations (non-SP)

Some exits define their own local macros or use product macros (e.g., `$WTO`, `$QSUSE`).
These are identified by:
- A `MACRO` / `MEND` block earlier in the file or in a COPY member
- An invocation token that matches a macro name defined above

For product macros (JES2 `$*`, IMS `IHB*`, CICS `DFH*`), see the relevant product guide.

---

## 9. Related Documents

- `docs/complex-asm-patterns.md` — TRT, MVCL, ICM, BCT, EX, packed decimal
- `docs/asm-to-metalc-general.md` — basic instruction-by-instruction translation
- `docs/asm-to-metalc-jes2.md` — JES2-specific macro set (`$IF`, `$DO`, `$WHEN`, etc.)
