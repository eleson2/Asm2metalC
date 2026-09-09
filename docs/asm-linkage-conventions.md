# ASM Linkage Conventions — Detection and Metal C Mapping

This document covers the three linkage families found in z/OS HLASM exits and explains
how to detect each, what its correct Metal C `#pragma prolog/epilog` is, and what
register setup differences matter for the converted C code.

---

## 1. Overview — Why Linkage Matters

Every HLASM exit must save the caller's registers on entry and restore them on exit.
The mechanism used determines:

- Which `#pragma prolog` / `#pragma epilog` to emit in Metal C
- Whether a save area on the stack or on the linkage stack is used
- Whether `R12` must be set as a base register
- Whether static writable data in the module is safe (reentrant) or not

Getting the linkage wrong produces a module that will abend (S0C4 or S0C3) when called.

---

## 2. The Three Linkage Families

### 2.1 Standard IBM Linkage — `SAVE(14,12)` / `RETURN(14,12)`

**Detection keywords (ASM source)**

```
STM  R14,R12,12(R13)   ← manual save, or
SAVE (14,12)            ← IBM convenience macro
...
RETURN (14,12),RC=n    ← IBM convenience macro, or
LM   R14,R12,12(R13)   ← manual restore
BR   R14
```

Also look for explicit save-area chaining:
```
ST   R13,4(,Rx)         ← forward chain: caller's save area addr in new area +4
ST   Rx,8(,R13)         ← backward chain: new area addr in caller's save area +8
LR   R13,Rx             ← R13 now points to our save area
```

**Metal C pragmas**

```c
#pragma prolog(MYEXIT, "SAVE(14,12),LR(12,15)")
#pragma epilog(MYEXIT, "RETURN(14,12)")
```

`LR(12,15)` sets R12 as the base register from R15 (entry point address), which is
required when the module has a `USING MYEXIT,R12` CSECT base.

**Register setup**

At function entry:
- R1  → first parameter (usually parameter block pointer)
- R13 → save area address (pre-allocated by caller, or module allocates its own)
- R14 → return address
- R15 → entry point address (used to establish R12 base)

**Reentrant / Storage**

Modules using standard linkage may or may not be reentrant.  If the ASM allocates
a work area with `GETMAIN R,LV=...`, the module is reentrant.  If it uses `DS`
fields in the CSECT body for writeable data, it is **not** reentrant (see §5).

---

### 2.2 Linkage Stack Linkage — `BAKR R14,0` / `PR`

**Detection keywords (ASM source)**

```
BAKR R14,0             ← Branch and Stack — saves ALL registers on AR/linkage stack
...
PR                      ← Program Return — restores all registers and returns
```

Also frequently combined with `EREG` / `ESAR` / `EPAR` to retrieve saved registers
on return, and with `STORAGE OBTAIN` / `STORAGE RELEASE` instead of GETMAIN/FREEMAIN.

**Metal C pragmas**

```c
#pragma prolog(MYEXIT, "BAKR 14,0")
#pragma epilog(MYEXIT, "PR")
```

> **CRITICAL**: Do NOT emit `SAVE(14,12),LR(12,15)` for BAKR-based exits.
> BAKR pushes a linkage stack entry; `SAVE(14,12)` writes to a save area pointed
> to by R13 — doing both corrupts the stack.

**Register setup**

At function entry R1, R14, R15 have the same roles as standard linkage, but:
- There is no save area on the stack — BAKR saves everything internally.
- R12 base setup still needed if the ASM uses `USING MYEXIT,R12`:

```c
#pragma prolog(MYEXIT, "BAKR 14,0,LR(12,15)")
```

Most modern z/OS system exits (IMS, DFSMS, some RACF) use BAKR/PR.

**STORAGE vs GETMAIN**

BAKR-based exits usually use:
```
STORAGE OBTAIN,LENGTH=n,SP=nnn   → maps to getmain(n, SP_nnn)
STORAGE RELEASE,LENGTH=n,SP=nnn  → maps to freemain(p, n, SP_nnn)
```

The `metalc_base.h` `getmain`/`freemain` wrappers accept a subpool parameter and
work correctly for both GETMAIN and STORAGE OBTAIN.

---

### 2.3 JES2 Macro Linkage — `$SAVE` / `$RETURN` / `$ENTRY`

**Detection keywords (ASM source)**

```
$MODULE ...            ← JES2 module prologue (sets up $SAVE/$RETURN conventions)
$ENTRY name            ← define an entry point (implies $SAVE on first invocation)
$SAVE                  ← JES2-specific register save macro
$RETURN (14,12),RC=n   ← JES2-specific return macro
$GETWORK DON=YES       ← allocate JES2 work area (like GETMAIN but JES2-managed)
$RETWORK               ← release JES2 work area
```

**What `$SAVE` / `$RETURN` expand to**

Internally these expand to the same `STM R14,R12,12(R13)` / `LM R14,R12,12(R13)` pattern
as standard IBM linkage, using R13 for the save area.  The distinction is that JES2
provides the save area in its own work area pool rather than requiring the caller to
supply one.  For Metal C purposes the pragma is identical to standard linkage:

```c
#pragma prolog(EXIT02, "SAVE(14,12),LR(12,15)")
#pragma epilog(EXIT02, "RETURN(14,12)")
```

**`$GETWORK` / `$RETWORK` mapping**

`$GETWORK` allocates a module-private work area from the JES2 GETMAIN pool.
Map to `getmain(sizeof(struct mywork), SP_229)` (JES2 uses SP 229 for exit work areas).
`$RETWORK` maps to `freemain(p, sizeof(struct mywork), SP_229)`.

The exact subpool may differ per JES2 release; check the JES2 Macros manual.

**`$WTO` mapping**

`$WTO 'text',ROUTCDE=(2)` is JES2's WTO convenience macro.
Map to `wto_simple("text")` or `wto_write(msg, pos, routcde, desc)`.

---

## 3. Detection Flowchart

```
Read ASM source
    │
    ├─ contains BAKR R14,0 ?
    │       YES → Linkage-Stack (§2.2)  pragma: "BAKR 14,0" / "PR"
    │
    ├─ contains $SAVE or $MODULE ?
    │       YES → JES2 Macro (§2.3)     pragma: "SAVE(14,12),LR(12,15)" / "RETURN(14,12)"
    │
    └─ default → Standard IBM (§2.1)   pragma: "SAVE(14,12),LR(12,15)" / "RETURN(14,12)"
```

---

## 4. Base Register Setup

Most z/OS exits establish R12 as the CSECT base register:

```asm
MYEXIT   CSECT
MYEXIT   AMODE 31
MYEXIT   RMODE ANY
         LR   R12,R15        ← copy entry address to R12
         USING MYEXIT,R12    ← tell assembler R12 = base
```

The Metal C compiler generates position-independent code, so no explicit base register
is needed.  The `LR(12,15)` part of the prolog pragma sets R12 to match the ASM
convention; keep it so the generated listing lines up with the original when the two
are compared side by side.  A converted exit has no inline assembler of its own to
reference R12 (CLAUDE.md rule 8) — the service wrappers in `metalc_svc.h` are the only
assembler involved, and they never rely on the caller's base register.

If the ASM uses **two** base registers (e.g., `USING MYEXIT,R12,R11` for exits > 4 KB),
the prolog pragma does not need to change — the compiler handles multi-page modules.

---

## 5. Non-Reentrant Static Data — The Reentrant-ification Rule

An exit with static writeable `DS` fields cannot run as reentrant.  This is common
when the original exit was assembled with `RMODE 24,AMODE 24` for older z/OS levels.

**Detection**

```asm
MYWORK   DS    0F
SAVEAREA DS    18F        ← 72 bytes for standard save area
SAF_RC   DS    F          ← writeable word in CSECT body
```

If `DS` fields with names that are *written at runtime* appear in the main CSECT body
(not inside a `DSECT`), the module is not reentrant.

**Metal C reentrant-ification**

1. Move all writeable `DS` fields to a local `struct mywork` on the C stack or
   allocated via `getmain`.
2. For RACROUTE static template fields, see `docs/racroute-metalc-patterns.md`.
3. Document the change in the module header comment block:
   ```c
   /* Reentrant-ification: SAF_RC moved from CSECT static to local variable.
    * Original ASM used 'SAF_RC DS F' — not reentrant.
    * Metal C uses 'int32_t saf_rc' on stack.                               */
   ```
4. Add a concern entry in the verification matrix (type: scope change, not defect).

---

## 6. Summary Table

| Convention | Detect by | Prolog pragma | Epilog pragma | Storage |
|------------|-----------|---------------|---------------|---------|
| Standard IBM | `SAVE (14,12)` / `STM R14,R12` | `"SAVE(14,12),LR(12,15)"` | `"RETURN(14,12)"` | GETMAIN/FREEMAIN |
| Linkage Stack | `BAKR R14,0` | `"BAKR 14,0"` | `"PR"` | STORAGE OBTAIN/RELEASE |
| JES2 Macros | `$SAVE` / `$MODULE` | `"SAVE(14,12),LR(12,15)"` | `"RETURN(14,12)"` | `$GETWORK`/`$RETWORK` |

---

## 7. Related Documents

- `docs/racroute-metalc-patterns.md` — RACROUTE MF=(E,list) static template and stubs
- `docs/asm-to-metalc-jes2.md` — JES2 serialization macros and work area conventions
- `docs/complex-asm-patterns.md` — BAKR with EREG, STORAGE OBTAIN details
- `includes/metalc_base.h` — `getmain`/`freemain` prototypes
