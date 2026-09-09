# Reentrant-ification Policy

When the original HLASM source is **not** reentrant — it uses writeable `DS` fields
in the CSECT body rather than a dynamically-obtained work area — the Metal C
conversion must make the module reentrant because `xlc -qmetal` produces RENT code
by default and the z/OS linker will enforce the RENT attribute.

This document defines the rules for identifying non-reentrant patterns and converting
them to reentrant equivalents.

---

## 1. What Makes a Module Non-Reentrant?

A module is non-reentrant if it writes to its own text (code) area at runtime.
In practice this means:

### 1.1 Writeable `DS` fields in the CSECT body

```asm
MYEXIT   CSECT
         ...code...
SAVEAREA DS    18F          ← 72-byte save area in the CSECT
SAF_RC   DS    F            ← static word modified at runtime
WORKFLAG DS    X            ← static byte modified at runtime
MSGBUF   DS    CL80         ← static message buffer
```

These fields are stored in the text section of the load module.  When two tasks
call the module concurrently, they share these fields and corrupt each other.

### 1.2 Self-modifying code

Instructions that write to nearby instruction bytes (see pre-conversion-triage.md).
This is a separate no-convert flag; do not confuse it with data fields.

### 1.3 Static save area

Many older exits have `SAVEAREA DS 18F` directly in the CSECT rather than
obtaining a save area via GETMAIN:

```asm
         STM  R14,R12,SAVEAREA    ← saves into static CSECT field — NOT reentrant
```

---

## 2. Detection Heuristics

The asm-pre-analyzer flags non-reentrant static data automatically.  The indicators:

1. `DS` or `DC` labels that appear as the **target** of `ST`, `MVC`, `MVI`, `STC`,
   `STH` instructions (i.e., they are written at runtime).
2. A `SAVEAREA DS 18F` that is used in `STM R14,R12,SAVEAREA`.
3. Absence of `GETMAIN` / `STORAGE OBTAIN` for a work area, combined with writeable
   data fields.
4. The module does NOT have `REUS=RENT` or `REENTRANT` attribute statements.

If the asm-pre-analyzer flags **non-reentrant static data = YES**, apply this policy
before converting.

---

## 3. Conversion Rules

### Rule R1 — Save area: replace with Metal C stack frame

The Metal C compiler generates a stack frame automatically.  The `#pragma prolog`
`SAVE(14,12)` / `BAKR 14,0` instructions save registers to the stack-managed area.
**Never** allocate a static save area `DS 18F` in the converted C source.

Simply delete the `SAVEAREA DS 18F` and its `STM`/`LM` pair when writing the C
equivalent — the prolog/epilog pragmas handle this.

### Rule R2 — Static work fields: move to local variables

Each writeable CSECT field becomes a local variable in the C function:

| ASM | C |
|---|---|
| `SAF_RC DS F` | `int32_t saf_rc;` |
| `WORKFLAG DS X` | `uint8_t workflag;` |
| `MSGBUF DS CL80` | `uint8_t msgbuf[80];` |
| `SAVEAREA DS 18F` | (dropped — handled by prolog pragma) |

### Rule R3 — Static work fields used across subroutine calls: use a work struct

If the writeable fields are accessed by multiple internal subroutines (called via
`BAS R14,sub`), collect them into a `struct mywork` on the stack and pass a pointer:

```c
struct myexit_work {
    int32_t  saf_rc;       /* formerly SAF_RC DS F   */
    uint8_t  workflag;     /* formerly WORKFLAG DS X  */
    uint8_t  msgbuf[80];   /* formerly MSGBUF DS CL80 */
};
```

Pass `&work` to all internal C functions that need it.

### Rule R4 — Large work areas: obtain via getmain

If the static area is large (> 256 bytes) or must survive across wait points,
obtain it from the JOB STEP or CSA subpool:

```c
struct myexit_work *work = getmain(sizeof(struct myexit_work), SUBPOOL_JOB_STEP);
if (work == NULL) return RC_SEVERE;
memset_inline(work, 0, sizeof(struct myexit_work));
/* ... use work ... */
freemain(work, sizeof(struct myexit_work), SUBPOOL_JOB_STEP);
```

Use getmain when the original ASM used `GETMAIN R,LV=n` or `STORAGE OBTAIN,LENGTH=n`.

### Rule R5 — RACROUTE static template: special handling

The `RACROUTE MF=L` static template is the most common legitimate use of a static
read-only field in a non-reentrant exit.  In a reentrant conversion:

1. The `MF=L` template field becomes a `static const` array in C (read-only; fine for reentrant).
2. The `MF=(E,list)` working copy is obtained via getmain or placed on the stack.

See `docs/racroute-metalc-patterns.md` for the assembler stub approach that avoids
the template entirely.

---

## 4. Required Documentation

Every reentrant-ification change must be documented in **two** places:

### 4.1 Module header comment (in the .c source)

Immediately after the standard file header, add:

```c
/*-------------------------------------------------------------------*
 * REENTRANT-IFICATION NOTES
 *
 * The original ASM module (asm/<PRODUCT>/<MODULE>.asm) was NOT fully
 * reentrant.  The following changes were made during conversion:
 *
 * 1. SAVEAREA DS 18F  — deleted; handled by #pragma prolog SAVE(14,12)
 * 2. SAF_RC   DS F    — moved to local variable int32_t saf_rc
 * 3. WORKFLAG DS X    — moved to local variable uint8_t workflag
 * 4. MSGBUF   DS CL80 — moved to local array uint8_t msgbuf[80]
 *
 * These changes make the converted module safe for concurrent use.
 * Verify against the ASM source if re-checking field usage.
 *-------------------------------------------------------------------*/
```

### 4.2 Verification matrix §4 (Scope Reduction / Changes)

Add a row in Section 4:

```markdown
## 4. Reentrant-ification Changes

| ASM Field | Type | C Equivalent | Notes |
|-----------|------|-------------|-------|
| `SAVEAREA DS 18F` | Save area | Dropped — prolog handles | Standard change |
| `SAF_RC DS F` | Static word | `int32_t saf_rc` (local) | Necessary for RENT |
| `MSGBUF DS CL80` | Buffer | `uint8_t msgbuf[80]` (stack) | Stack allocation |
```

---

## 5. Reentrant-ification and Partial Scope

If a block of static data is **too complex to reentrant-ify** during conversion
(e.g., a 2048-byte table that is partially writeable, partially read-only, with
internal cross-references), mark that section as deferred scope:

- Apply the `DEFERRED` reason code from `docs/partial-scope-policy.md`.
- Add a SCOPE REDUCTION comment block in the C source.
- List in the verification matrix §4.
- Note in the sign-off checklist.

---

## 6. Testing Reentrant-ified Modules

After reentrant-ification, test concurrency explicitly:

1. Submit two identical jobs simultaneously that will both hit the exit.
2. Verify neither job produces incorrect results.
3. In CICS/IMS environments, test with multiple threads/regions calling the exit concurrently.

On z/OS, the linker attribute `RENT` is not a runtime guarantee; the code itself must
be correct.  The tests above are the only real proof.

---

## 7. Summary Checklist

- [ ] All writeable `DS` fields in CSECT body identified
- [ ] Each field moved to local variable, stack struct, or getmain area (R2/R3/R4)
- [ ] `SAVEAREA DS 18F` deleted (R1)
- [ ] RACROUTE MF=L template converted per `docs/racroute-metalc-patterns.md` (R5)
- [ ] Reentrant-ification notes added to module header comment (§4.1)
- [ ] Verification matrix §4 updated with change table (§4.2)
- [ ] Concurrent test performed (§6)

---

## 8. Related Documents

- `docs/asm-linkage-conventions.md` — BAKR/PR vs SAVE/RETURN, static data detection
- `docs/racroute-metalc-patterns.md` — RACROUTE MF=L template handling
- `docs/partial-scope-policy.md` — when reentrant-ification must be deferred
- `docs/pre-conversion-triage.md` — triage checklist includes reentrancy assessment
- `docs/amode64-exits.md` — struct layout differences in AMODE 64 affect field sizes
