# System Services Catalog — HLASM Macro to Metal C

The lookup table for every z/OS system service an exit can call, and the
procedure to follow when a macro is not in it yet.

This exists because a converter that meets an unfamiliar macro has exactly
one bad option available to it — write inline `__asm` that looks plausible —
and that option produced a real security hole in this repository
(`converted/IMS/DFSWHU00.c` carried `__asm(" XR 15,15")` in place of a
RACROUTE, allowing every IMS sign-on regardless of RACF). The rules below
exist to remove that option.

**Read this before converting any module whose triage report lists a system
service macro.**

---

## 1. The Rule

> A converted `.c` file contains **no** `__asm`. Every system service is a
> C function call.

All inline assembler in the framework lives in one file, `includes/metalc_svc.h`.
All standalone assembler lives in `asm/stubs/`. The invariant is greppable:

```
grep -rl __asm converted/ includes/ examples/     # -> includes/metalc_svc.h only
```

Why the rule is absolute rather than a preference:

| | Inline `__asm` placeholder | Missing wrapper / stub |
|---|---|---|
| Compiles? | Yes | Yes |
| Assembles? | Yes | Yes |
| Link edits? | **Yes** | **No — unresolved external** |
| Reaches production? | Possible | Impossible |
| Looks wrong in review? | No | n/a |

An incomplete stub fails the build. An incomplete inline placeholder ships.
That asymmetry is the whole argument.

---

## 2. Decision Procedure

For each system service macro in the ASM source:

```
Is it in the coverage table (§3)?
├── Status IMPLEMENTED
│     └── Call the listed C function. Done.
│
├── Status NO-OP
│     └── Emit nothing. Note it in the module header. Done.
│
├── Status ADD WRAPPER
│     └── Write the wrapper in includes/metalc_svc.h (§4), then call it.
│
├── Status ADD STUB
│     └── Write the stub in asm/stubs/ (§5), declare it, then call it.
│
└── Status DO NOT CONVERT
      └── Stop. Flag for manual review per docs/complex-asm-patterns.md.

Not in the table at all?
└── Treat as ADD WRAPPER or ADD STUB by the test in §4.1.
    If you cannot complete it, STOP and say so in your report.
    Do NOT inline __asm, and do NOT leave a placeholder that returns
    a success value.
```

**If you stop, say precisely this** in the conversion report and the module
header, so a human picks it up rather than the gap being silently absorbed:

```
BLOCKED: <MACRO> has no wrapper in metalc_svc.h and no stub in asm/stubs/.
Conversion of <MODULE> is incomplete. Required: <wrapper|stub> for <MACRO>
<REQUEST=/keyword forms used by this module>.
```

---

## 3. Coverage Table

Status values:

- **IMPLEMENTED** — call it, it exists today
- **ADD WRAPPER** — no wrapper yet; add one to `metalc_svc.h` per §4
- **ADD STUB** — macro expansion too complex to inline; stub per §5
- **NO-OP** — assembly-time directive, generates no code, emit nothing
- **DO NOT CONVERT** — flag for manual review

### 3.1 Console and log

| ASM macro | Status | C call |
|---|---|---|
| `WTO` (single-line) | IMPLEMENTED | `wto_simple(msg, len)` |
| `WTO` with `ROUTCDE=`/`DESC=` | IMPLEMENTED | `wto_write(msg, len, route, desc)` |
| `WTO` to security console | IMPLEMENTED | `wto_security(msg, len)` |
| `WTO` action-required | IMPLEMENTED | `wto_alert(msg, len)` |
| `WTO` important info | IMPLEMENTED | `wto_important(msg, len)` |
| `WTO` multi-line (`MF=`, connect id) | ADD WRAPPER | — |
| `WTOR` | ADD WRAPPER | Needs ECB wait; see §4.3 |
| `WTL` | ADD WRAPPER | — |

Message text must be EBCDIC and is capped at 126 bytes by `wto_write`.

### 3.2 Storage

| ASM macro | Status | C call |
|---|---|---|
| `GETMAIN R,LV=` | IMPLEMENTED | `getmain(size, subpool)` |
| `FREEMAIN R,LV=,A=` | IMPLEMENTED | `freemain(addr, size, subpool)` |
| `STORAGE OBTAIN` | IMPLEMENTED | `storage_obtain(size, subpool)` |
| `STORAGE RELEASE` | IMPLEMENTED | `storage_release(addr, size, subpool)` |
| `CPOOL BUILD/GET/FREE/DELETE` | ADD WRAPPER | — |
| `IARV64` (above-the-bar) | ADD STUB | 64-bit only; see `docs/amode64-exits.md` |

**Match the source macro.** `GETMAIN R` is unconditional — an unsatisfiable
request abends (S80A/S878) rather than returning. `storage_obtain` uses
`COND=YES` and returns `NULL`. Converting `STORAGE OBTAIN` to `getmain`
turns a recoverable path into an abend; converting `GETMAIN` to
`storage_obtain` invents a NULL path the ASM never had. Check the result
either way.

### 3.3 Time

| ASM macro | Status | C call |
|---|---|---|
| `STCK` | IMPLEMENTED | `get_tod_clock(&tod)` |
| `STCK` → time of day | IMPLEMENTED | `get_time_hundredths()` |
| `STCKE` (extended) | IMPLEMENTED | `get_tod_clock_extended(buf16)` |
| `TIME` (`DEC`, `BIN`, `TU`, `MIC`) | ADD WRAPPER | Formatted forms need the SVC, not STCK |

`TIME DEC,...,DATETYPE=` returns packed-decimal date/time. Do not
substitute `get_tod_clock` for it — the formats differ. Add the wrapper.

`STCK` and `STCKE` are also not interchangeable: STCKE's byte 0 is an epoch
index and its clock field is shifted one byte right. Match the source.

### 3.4 Security (SAF / RACF)

| ASM macro | Status | C call |
|---|---|---|
| `RACROUTE REQUEST=AUTH` | IMPLEMENTED | `saf_auth(class, entity, userid, attr, detail)` |
| `RACROUTE REQUEST=AUTH,CLASS='APPL'` | IMPLEMENTED | `saf_auth_appl(applid, userid)` |
| `RACROUTE REQUEST=VERIFY` | ADD STUB | Next to `SAFAUTH.asm` |
| `RACROUTE REQUEST=FASTAUTH` | ADD STUB | — |
| `RACROUTE REQUEST=EXTRACT` | ADD STUB | — |
| `RACROUTE REQUEST=LIST` | ADD STUB | — |
| `RACROUTE REQUEST=STAT` | ADD STUB | — |
| `ICHEINTY` / `ICHETEST` | ADD STUB | Undocumented interfaces — review first |
| `RACXTRT` | ADD STUB | — |

Interface: `includes/metalc_saf.h`. Stub: `asm/stubs/SAFAUTH.asm`.
Full detail: `docs/racroute-metalc-patterns.md`.

**Never** decide allow/deny on anything but SAF RC=0. See §7.

### 3.5 Serialization

| ASM macro | Status | C call |
|---|---|---|
| `ENQ` | ADD WRAPPER | — |
| `DEQ` | ADD WRAPPER | — |
| `RESERVE` | ADD WRAPPER | — |
| `ISGLOBT` / `ISGLREL` (latches) | ADD STUB | — |

An exit that takes a serialization resource must release it on **every**
return path, including error paths. In C that means a single exit point or
a `goto cleanup` — not a bare `return` inside the held region.

### 3.6 Task and program management

| ASM macro | Status | C call |
|---|---|---|
| `LOAD` | ADD WRAPPER | — |
| `DELETE` | ADD WRAPPER | — |
| `LINK` / `LINKX` | ADD STUB | — |
| `XCTL` / `XCTLX` | DO NOT CONVERT | Does not return; restructure by hand |
| `ATTACH` / `ATTACHX` | ADD STUB | — |
| `DETACH` | ADD WRAPPER | — |
| `WAIT` | ADD WRAPPER | — |
| `POST` | ADD WRAPPER | — |
| `IDENTIFY` | ADD WRAPPER | — |
| `CSVQUERY` | ADD STUB | — |

### 3.7 Recovery

| ASM macro | Status | C call |
|---|---|---|
| `ESTAE` / `ESTAEX` | ADD STUB | Recovery routine is a separate entry point |
| `SETRP` | ADD STUB | Valid only inside a recovery routine |
| `ESPIE` | ADD STUB | — |
| `ABEND` | ADD WRAPPER | — |
| `CALLRTM` | DO NOT CONVERT | Requires authorization; review |

Recovery routines are their own conversion problem: the recovery entry has
different linkage from the mainline and runs with the SDWA addressable.
Convert the mainline first and flag the recovery exit separately.

### 3.8 SMF and instrumentation

| ASM macro | Status | C call |
|---|---|---|
| `SMFEWTM` | ADD WRAPPER | — |
| `SMFWTM` | ADD WRAPPER | — |

See `docs/asm-to-metalc-smf.md` for record layout mapping.

### 3.9 Name/token and state

| ASM macro | Status | C call |
|---|---|---|
| `IEANTCR` / `IEANTRT` / `IEANTDL` | ADD STUB | Callable services, not macros — link directly |
| `MODESET` | DO NOT CONVERT | Changes key/state; review authorization |
| `TESTAUTH` | ADD WRAPPER | — |
| `EXTRACT` | ADD WRAPPER | — |

### 3.10 Allocation and data

| ASM macro | Status | C call |
|---|---|---|
| `DYNALLOC` (SVC 99) | ADD STUB | Parameter list is elaborate; see `docs/asm-to-metalc-dfsms.md` |
| `OPEN` / `CLOSE` | DO NOT CONVERT | Access-method control blocks; flag |
| `GET` / `PUT` / `READ` / `WRITE` / `POINT` | DO NOT CONVERT | Access-method; flag |
| `BLDL` / `FIND` | DO NOT CONVERT | Flag |
| `EXCP` / channel programs | DO NOT CONVERT | Named in CLAUDE.md as manual-review |

### 3.11 Cross-memory and AR-mode

| ASM instruction | Status |
|---|---|
| `PC` / `PT` / `SSAR` / `AXSET` / `LXRES` | DO NOT CONVERT |
| `ALESERV`, `DSPSERV` | DO NOT CONVERT |
| Any AR-mode code (`SAC`, `LAM`, `CPYA`) | DO NOT CONVERT |

Already listed under "What NOT to Convert Automatically" in `CLAUDE.md`.
Stop and report.

### 3.12 Hardware instructions with no C equivalent

Not macros, but they need the same treatment: a wrapper in `metalc_svc.h`,
never an `__asm` block in the exit.

| Instruction | Status | C call |
|---|---|---|
| `STCK` / `STCKE` | IMPLEMENTED | `get_tod_clock` / `get_tod_clock_extended` |
| `CVB` (packed → binary) | IMPLEMENTED | `packed_to_binary(field)` |
| `CVD` (binary → packed) | IMPLEMENTED | `binary_to_packed(field, value)` |
| `AP` `SP` `MP` `DP` `CP` `ZAP` | IMPLEMENTED (indirectly) | Convert to binary, compute in C, convert back |
| `PACK` / `UNPK` (zoned ↔ packed) | ADD WRAPPER | — |
| `ED` / `EDMK` (edit for display) | ADD WRAPPER | Often replaceable with `format_int()` |
| `TRT` `TR` `MVCL` `CLCL` `EX` | Plain C | Translate to loops — see `docs/complex-asm-patterns.md` |

`CVB` raises S0C7 on invalid packed data and S0C9 on overflow, with no
return code. Validate untrusted input before converting, or run under
recovery — this matters in exits reading packed fields out of user-supplied
JCL or control blocks.

Prefer a plain C loop over a wrapper whenever one exists. `TRT`, `MVCL` and
friends are clearer and safer as C; only reach for a wrapper when the
instruction has genuinely no C expression.

### 3.13 Assembly-time directives — emit nothing

| ASM directive | Status | Note |
|---|---|---|
| `SPLEVEL SET=` | NO-OP | Selects macro expansion level at assembly time |
| `SYSSTATE ARCHLVL=`/`AMODE64=` | NO-OP | Sets macro expansion assumptions |
| `TITLE` / `EJECT` / `SPACE` / `PRINT` | NO-OP | Listing control |
| `YREGS` | NO-OP | Register equates; C uses names |
| `COPY` | NO-OP in C | But resolve it — see `docs/copy-macro-dependency.md` |
| `LTORG` | NO-OP | Literal pool placement |
| `DROP` / `USING` | NO-OP | Addressability; C uses struct pointers |

These generate no object code. A converter that emits something for them
has misread the source. Record them in the triage macro inventory as
NO-OP so the reviewer can see they were considered, not missed.

---

## 4. Adding a Wrapper to `metalc_svc.h`

### 4.1 Wrapper or stub?

Write a **wrapper** (inline `__asm` in `metalc_svc.h`) when all of:

- the macro expansion is a handful of instructions plus an SVC or a
  branch-entry call;
- every operand can be passed in a register or a simple storage location;
- there is no `MF=L` model list to copy, and no keyword-only operand whose
  value the exit chooses at run time.

Write a **stub** (`asm/stubs/*.asm`) when any of:

- the macro has an `MF=L` / `MF=(E,list)` two-form pattern;
- an operand is keyword-only, so a run-time value cannot be passed
  (`ATTR=READ` on RACROUTE is the worked example — the stub branches to one
  expansion per value, see `asm/stubs/SAFAUTH.asm`);
- the macro needs its own work area or savearea;
- the expansion is long enough that reviewing it inline is impractical.

When in doubt, write the stub. It costs a link-edit entry and buys
reviewability.

### 4.2 Template

```c
/**
 * svc_name - one-line description
 * @arg: what it is
 *
 * Returns: what the RC means
 *
 * ASM equivalent: MACRO KEYWORD=value,...
 */
static inline int svc_name(uint32_t arg) {
    int rc = <fail-safe default>;

    __asm volatile(
        " L     2,%1         \n"   /* value, not address - see 6.1 */
        " MACRO KEYWORD=(2)  \n"
        " ST    15,%0        \n"
        : "=m"(rc)
        : "m"(arg)
        : "0", "1", "2", "14", "15"
    );

    return rc;
}
```

### 4.3 Checklist

- [ ] Initialise every output to a **fail-safe** value before the `__asm`,
      so a path that does not execute cannot read as success
- [ ] `__asm volatile` — without it the block can be deleted when its
      outputs look unused
- [ ] `L` not `LA` for a value from an `"m"` operand (§6.1)
- [ ] Clobber list names every register the macro touches — at minimum
      `"0"`, `"1"`, `"14"`, `"15"`, plus any register you load yourself
- [ ] Never clobber or load R13 (savearea) or R12 (base)
- [ ] Guard NULL pointer arguments in C before entering the `__asm`
- [ ] Document whether the macro is conditional (returns RC) or
      unconditional (abends)
- [ ] Note in the function comment if the macro needs SYS1.MACLIB at
      assemble time (§6.5)
- [ ] Add the row to §3 of this file with status IMPLEMENTED
- [ ] Add the row to the pre-analyzer's macro table
      (`.claude/agents/asm-pre-analyzer.md` §4)

Services that block (`WAIT`, `WTOR`, `ENQ` without `RET=`) need an ECB and
must not be added as a simple inline wrapper without deciding what the exit
does while suspended. Most exits run in environments where blocking is
forbidden. Flag rather than guess.

---

## 5. Adding a Stub to `asm/stubs/`

### 5.1 Contract

- Standard OS linkage: `R1 -> A(parmblock)`, `SAVE (14,12)` on entry,
  `RETURN (14,12),RC=(15)` on exit
- One C struct maps one `DSECT`, field for field, at identical offsets —
  say so in both files so the pair stays in step
- Reentrant: copy any `MF=L` model into obtained storage before the
  `MF=(E,...)`; never write into module storage
- Obtain storage with `COND=YES` and report the shortage through the
  parameter block; never abend the caller's address space
- Return **all** the service's return codes, not just R15 — R0 and R1 are
  usually diagnostic and belong in the parameter block

### 5.2 C declaration

```c
#pragma pack(1)
struct svc_parm {
    const char *input;    /* +0  */
    int32_t     option;   /* +4  */
    int32_t     rc;       /* +8  OUT */
};
#pragma pack()

#pragma linkage(svc_call, OS)
extern int svc_call(struct svc_parm *parm);
```

Put the declaration in the service header that owns it (`metalc_saf.h` for
SAF), not in `metalc_base.h`.

### 5.3 Module header must record the link edit

Every exit that calls a stub carries the build sequence in its header
comment, because forgetting it produces an unresolved external at link time
with no other clue:

```
 * Build: xlc -qmetal -S -qlist -I./includes converted/IMS/DFSWHU00.c
 *        as -o DFSWHU00.o DFSWHU00.s
 *        as -o SAFAUTH.o  asm/stubs/SAFAUTH.asm
 *        ld -o DFSWHU00 DFSWHU00.o SAFAUTH.o
```

### 5.4 Unassembled stubs must say so

There is no z/OS system in this repository, so a new stub has never been
assembled. Put a review block in the stub header naming exactly what a
systems programmer must confirm — macro operand forms, `RELEASE=` level,
work area size — and add the exit to the **Blocked on Assembler Stub
Validation** table in `docs/verification-matrices/README.md`.

`asm/stubs/SAFAUTH.asm` is the worked example of all of §5.

---

## 6. Writing the Inline Assembler Correctly

These are the errors actually found in this repository's wrappers, not a
generic style list.

### 6.1 `L` loads a value; `LA` loads an address

An `"m"` constraint substitutes a base-displacement expression — the
*location* of the variable, e.g. `160(13)`. So:

```
 L     0,%2      R0 = the size value          <- correct
 LA    0,%2      R0 = the address of size     <- wrong
```

`getmain`, `freemain` and `storage_release` all shipped with `LA` where
`L` was meant, passing an address where a length or a storage pointer was
required. Use `LA` only when you genuinely want the address of a storage
area — as `wto_write` does for the WTO parameter list.

### 6.2 `volatile` is not optional

Without it, a block whose outputs are unused is a candidate for deletion.
A WTO whose return code nobody reads is exactly that shape.

### 6.3 Zero the registers the interface requires

`SVC 35` needs `R0 = 0` for a single-line WTO with no connect id. Listing
R0 as clobbered does not set it. Read the macro's register conventions and
set every input it reads, including the ones that must be zero.

### 6.4 Alignment

`ST`/`STH` require aligned targets. Writing a halfword at
`text[len]` for a caller-supplied `len` is unaligned half the time — store
byte-wise instead. `wto_write` shows the pattern.

### 6.5 Macros need the macro library

`GETMAIN`, `FREEMAIN`, `STORAGE`, `WTO` with keywords, and every
`RACROUTE` are macros. They expand when the compiler-generated HLASM is
assembled, so **SYS1.MACLIB must be in the SYSLIB concatenation of the
assemble step**. Only raw instructions (`STCK`) and raw `SVC nn` are
macro-free. State the dependency in the wrapper comment.

### 6.6 Fail-safe initialisation

Initialise outputs to the value that denies, fails, or returns NULL —
never to the success value. If the `__asm` does not execute, or a future
edit breaks it, the wrong answer must be the safe one.

---

## 7. Security Services Are Different

For any service that yields an access decision:

- Only the explicit "authorized" code allows. For SAF that is **RC=0 and
  nothing else**: RC=4 (no decision — RACF or the class inactive, or no
  profile) and RC=8 (not authorized) both deny.
- A service that could not be called denies.
- A storage shortage in the stub denies.
- Never return the allow value from a path that did not get a decision.

The `saf_auth()` / `saf_auth_appl()` wrappers apply this so the exit cannot
get it wrong. Any new security service wrapper must do the same: return a
decision, not a raw return code, and make the caller ask for the raw codes
explicitly when it wants them for a message.

See `docs/racroute-metalc-patterns.md` §5.

---

## 8. Verification

The verifier checks these; check them yourself before handing over.

```
# 1. No inline assembler outside the service layer
grep -rl __asm converted/ includes/ examples/
# expect: includes/metalc_svc.h

# 2. Every service macro in the ASM has a call in the C
grep -oE '^ +(WTO|WTOR|GETMAIN|FREEMAIN|STORAGE|STCK|TIME|RACROUTE|ENQ|DEQ|LOAD|DELETE|WAIT|POST|ESTAE|SMFEWTM|ABEND)' asm/<P>/<M>.asm | sort | uniq -c

# 3. Stub callers record the link edit
grep -l 'asm/stubs/' converted/*/*.c | xargs grep -L 'ld -o'
# expect: no output
```

Also confirm: storage obtained on every path is released on every path,
including early returns; and the storage macro family in the C matches the
one in the ASM (§3.2).

---

## 9. Related Documents

- `CLAUDE.md` — rule 8, no inline assembler in exit source
- `docs/ai-conversion-steering.md` — authoritative conversion rules
- `docs/racroute-metalc-patterns.md` — SAF in full, including the three-way RC
- `docs/complex-asm-patterns.md` — instruction-level patterns (EX, TRT, MVCL)
- `docs/copy-macro-dependency.md` — resolving COPY members and macro libraries
- `docs/pre-conversion-triage.md` — §3 macro inventory, filled before conversion
- `includes/metalc_svc.h` — the wrappers themselves; the header comment is
  the authoritative list of what exists
- `includes/metalc_saf.h`, `asm/stubs/SAFAUTH.asm` — the worked stub example
