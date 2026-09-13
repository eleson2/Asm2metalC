# Pre-Conversion Triage — `IRR@XACS` (RACF/DB2 External Security Module)

Completed against the template in
[`../pre-conversion-triage.md`](../pre-conversion-triage.md).

## Module Identification

| Field | Value |
|-------|-------|
| Module name | `IRR@XACS` (load module); CSECT `DSNX@XAC` |
| Source path | `asm/DB2/IRR@XACS.asm` |
| Product | DB2 for z/OS (exit point) + RACF (implementation) |
| Exit name / number | DB2 **access control authorization exit**, installed as `DSNX@XAC` |
| Size | 6,432 lines, 5 CSECTs, 54 DSECTs |
| Attributes | `AMODE 31`, `RMODE ANY`, reentrant |
| Provenance | Third-party, Apache-2.0 — see [`../../asm/THIRD-PARTY.md`](../../asm/THIRD-PARTY.md) |
| Date of triage | 2026-09-12 |
| Analyst | Claude Opus 5 (automated triage; needs human sign-off) |

---

## Verdict

**BLOCKED — do not begin conversion.** Two independent blockers, either
of which is sufficient on its own:

1. **The two mapping macros that define the entire interface are absent**
   (`DSNDXAPL`, `DSNDEXPL`, from DB2's `SDSNMACS`). All 36 XAPL fields
   and all 4 EXPL fields are referenced *symbolically*, so the source
   yields **no offset evidence whatsoever** for either block.
2. **Four RACROUTE request types are used; the framework implements
   none of them.** `metalc_saf.h` covers `REQUEST=AUTH` only.

Neither is a reason to abandon the module — both are missing inputs with
a defined acquisition path. See Section 7.

> **This module is the highest-value challenge in the repository**
> precisely because a plausible-looking wrong answer is so easy to
> produce. An agent that "converts" it without `DSNDXAPL` must invent
> 36 field offsets in a **security decision path**, and the result
> compiles, passes `make layout`, and grants or denies DB2 privileges
> against garbage. That is the exact failure the `__asm` ban and the
> evidence rules exist to prevent, one scale up from the IMS sign-on
> anecdote in CLAUDE.md.

---

## Section 1 — Entry Point Inventory

| Label | Type | Parameter block / register at entry | Notes |
|-------|------|--------------------------------------|-------|
| `DSNX@XAC` | Main entry | `R1` → 2-word address list | Line 1333. `L R6,0(R1)` = EXPL, `L R3,4(R1)` = XAPL |
| `@FRR_RTN` | FRR | `R1` → FRR parm, SDWA | Functional recovery, runs in key 0 |
| `DSNX@MSG` | Data CSECT | — | WTO message skeletons (line 4924) |
| `IRR@TPRV` | Data CSECT | — | Privilege table, 90 `PRIVILEGE` macro entries (line 5120) |
| `IRR@TRUL` | Data CSECT | — | Rule table, 101 `RULE` macro entries (line 5376) |
| `IRR@TRES` | Data CSECT | — | Resource/class table (line 5960) |

### 1.1 Linkage convention

```asm
DSNX@XAC CSECT
         USING *,R15
         B     @PROLOG
@PROLOG  STM   R14,R12,12(R13)         Save callers registers
         LR    R12,R15                 Load module address into R12
```

`STM R14,R12` + `LR R12,R15` → per CLAUDE.md's linkage table:

```c
#pragma prolog(DSNX@XAC, "SAVE(14,12),LR(12,15)")
#pragma epilog(DSNX@XAC, "RETURN(14,12)")
```

Note `DSNX@XAC` is not a valid C identifier — the C entry point needs a
different name plus a linker alias, or the `#pragma csect` directive.
**Open question for the converter.**

---

## Section 2 — DSECT / Mapping Inventory

The interface, from the prologue diagram at lines 255–286:

```
                       EXPL (DSNDEXPL)   Work Area - 4096 bytes (WA_MAP)
     +---------+      +---------+      +---------------------+
R1-->| EXPLPTR |----->| EXPLWA  |----->| WA_SAVE             |
     |---------|      | EXPLWL  |      | WA_CPID / WA_CSIZE  |
  ---| XAPLPTR |      | EXPLRC1 |      | WA_DFTACEE ...      |
  |  +---------+      | EXPLRC2 |      +---------------------+
  |                   +---------+
  |   XAPL (DSNDXAPL)
  --->| XAPLCBID | XAPLLEN | XAPLEYE | ... |
```

| DSECT | Macro source | Register | Mapped C struct | Status |
|-------|--------------|----------|-----------------|--------|
| `XAPL` | `DSNDXAPL` (DB2 `SDSNMACS`) | R3 | — | **MISSING — blocker** |
| `EXPL` | `DSNDEXPL` (DB2 `SDSNMACS`) | R6 | — | **MISSING — blocker** |
| `WA_MAP` | in-file (line ~691) | R4 / R15 | `struct xac_workarea` | Mappable — 4096-byte work area |
| `SAFP` | `ICHSAFP` (RACF) | — | — | Missing; needed for RACROUTE |
| `SDWA` | `IHASDWA` (MVS) | — | — | Missing; FRR only |
| `PSA` | `IHAPSA` (MVS) | — | — | Missing; FRR only |
| `FRRS` | `IHAFRRS` (MVS) | — | — | Missing; FRR only |
| `RULENTRY`, `RULENTHD` | in-file | `RULPTR` | rule table entry | Mappable |
| ~47 others | in-file | various | — | Mappable |

### 2.1 Field Evidence Ledger — **empty, and that is the finding**

`docs/asm-field-evidence.md` §2 admits only explicit base-displacement
references as offset evidence. This module has **none** for XAPL or
EXPL. Every one of the 399 XAPL references looks like this:

```asm
         USING XAPL,R3             Set up XAPL register
         LH    R4,XAPLFUNC         symbolic - proves nothing about offset
         ST    R4,EXPLRC1          symbolic - proves nothing about offset
```

Under `USING`, the assembler resolves `XAPLFUNC` from the absent
`DSNDXAPL` DSECT. The width of `LH` proves `XAPLFUNC` is 2 bytes, and
`STH R4,EXPLRC1` proves `EXPLRC1` is 2 bytes — **that is the entire
harvest from 6,432 lines.**

| Field | Width evidence | Instruction | Line | Offset evidence |
|-------|---------------|-------------|------|-----------------|
| `XAPLFUNC` | 2 bytes | `LH R4,XAPLFUNC` | 1375 | **none** |
| `EXPLRC1` | 2 bytes | `STH R4,EXPLRC1` | 1401 | **none** |

> **Correction to an assumption made when this module was selected.**
> It was picked partly on the expectation that it would supply the
> missing assembler evidence for `db2_xac_parm`. **It does not, and it
> cannot** — symbolic references carry no offsets. `db2_xac_parm`
> remains unsourced exactly as `layout-findings.md` finding 4 records.

### 2.2 `db2_xac_parm` is the wrong *shape*, not merely unsourced

This is a real finding about the existing framework, independent of
whether this module is ever converted.

`includes/metalc_db2.h` models the connection exit as one flat inline
block opening with `EXIT_PARM_HEADER_6`:

```c
struct db2_xac_parm {
    EXIT_PARM_HEADER_6;              /* +0   work, func, flags        */
    uint8_t        xactype;          /* +6   Connection type          */
    char           xacplan[8];       /* +8   Plan name                */
    ...
```

The real exit receives **`R1` → a 2-word list of pointers to two
separate control blocks**, and returns its decision in `EXPLRC1`/
`EXPLRC2` rather than R15. There is no work-area pointer at +0, no
function code at +4, and no flags byte at +5 — `XAPL` starts with
`XAPLCBID`, `XAPLLEN`, `XAPLEYE`.

| | `db2_xac_parm` (framework) | `DSNX@XAC` (real) |
|---|---|---|
| Shape | one inline block | `R1` → [`EXPLPTR`, `XAPLPTR`] |
| Header | work/func/flags at +0/+4/+5 | `XAPLCBID`/`XAPLLEN`/`XAPLEYE` |
| Function code | `func` at +4, 1 byte | `XAPLFUNC`, 2 bytes (`LH`) |
| Return code | R15 | `EXPLRC1` (2 bytes), reason in `EXPLRC2` |
| Work area | `void *work` at +0 | `EXPL→EXPLWA`, 4096 bytes, `EXPLWL` length |

**Recommended action regardless of conversion:** add a HIGH concern to
`db2_xac_parm` noting the shape mismatch, and do not present it as the
`DSNX@XAC` interface until `DSNDXAPL` is available. Note that the
struct's doc comment says `DSNX@XAC` while `docs/asm-to-metalc-db2.md`
calls it `DSNX@XAC`/`db2_xac_parm` interchangeably.

---

## Section 3 — Macro Inventory

| Macro | Category | Conversion approach | Status |
|-------|----------|---------------------|--------|
| `TITLE` / `EJECT` / `SPACE` / `MNOTE` | Assembly-time | emit nothing | NO-OP |
| `GBLC`/`GBLA`/`SETC`/`SETA`/`AIF`/`AGO`/`ANOP` | Conditional assembly | see Section 4 | **DESIGN DECISION** |
| `WTO` (17) | Console | `wto_write()` / `wto_simple()` | IMPLEMENTED |
| `RACROUTE REQUEST=FASTAUTH` | SAF | new stub | **MISSING** |
| `RACROUTE REQUEST=STAT` | SAF | new stub | **MISSING** |
| `RACROUTE REQUEST=VERIFY,ENVIR=CREATE\|DELETE` | SAF | new stub | **MISSING** |
| `RACROUTE REQUEST=LIST,GLOBAL=YES,ENVIR=CREATE\|DELETE` | SAF | new stub | **MISSING** |
| `CPOOL BUILD/GET/FREE/DELETE` | Storage | new wrapper or stub | **MISSING** |
| `MODESET EXTKEY=ZERO` / `KEYADDR=` | Authorized | stub; key 0 transition | **MISSING** |
| `SETFRR A` / `SETFRR D` | Recovery | stub; see Section 4 | **MISSING** |
| `ICHSAFP`, `IHASDWA`, `IHAPSA`, `IHAFRRS` | Mapping | struct definitions | **MISSING** |
| `DSNDXAPL`, `DSNDEXPL` | Mapping | struct definitions | **BLOCKER** |
| `PRIVILEGE` (90), `RULE` (101) | In-file table generators | static `const` tables | Mappable |

### 3.1 New services required

Look each of these up in `docs/system-services-catalog.md` §4 before
writing anything.

| Service (keyword forms actually used) | Wrapper or stub | Target file | Done |
|---|---|---|---|
| `RACROUTE REQUEST=FASTAUTH,WORKA=,RELEASE=2.4,MF=(E,..)` | **Stub** | `asm/stubs/SAFFAST.asm` | ☐ |
| `RACROUTE REQUEST=STAT,RELEASE=2.4` | **Stub** | `asm/stubs/SAFSTAT.asm` | ☐ |
| `RACROUTE REQUEST=VERIFY,ENVIR=CREATE/DELETE` | **Stub** | `asm/stubs/SAFVERIF.asm` | ☐ |
| `RACROUTE REQUEST=LIST,GLOBAL=YES,ENVIR=CREATE/DELETE` | **Stub** | `asm/stubs/SAFLIST.asm` | ☐ |
| `CPOOL BUILD/GET/FREE/DELETE` | Wrapper | `includes/metalc_svc.h` | ☐ |
| `UNPK` + `NC` + `TR` (18 + 17 uses) | **None needed** — `format_hex()` already covers it | — | ☑ |
| `MODESET EXTKEY=ZERO,SAVEKEY=/KEYADDR=` | **Stub** | `asm/stubs/MODESET.asm` | ☐ |
| `SETFRR A/D` | **Stub** | `asm/stubs/SETFRR.asm` | ☐ |

**`UNPK` needs no wrapper — corrected.** An earlier draft of this
triage recommended adding one, on the strength of
`docs/complex-asm-patterns.md` §7 listing `PACK`/`UNPK` as *"No wrapper
yet — ADD WRAPPER"*. **That was wrong**, and the doc has been fixed.

All 17 uses here are the binary→printable **hexadecimal** idiom, and
the give-away is the translate table, `CL16'0123456789ABCDEF'`:

```asm
         STCM  R15,7,PACKAREA           3 bytes of a binary value
         UNPK  UNPACKAREA,PACKAREA      split nibbles
         MVC   M904_RACROUTE_RC(3),UNPACKAREA+3
         NC    M904_RACROUTE_RC,ZONECHARS   X'0F0F0F' - isolate nibble
         TR    M904_RACROUTE_RC,TRTABLE0    CL16'0123456789ABCDEF'
```

`UNPK` is a nibble-splitter here, not packed-decimal arithmetic. In C
the whole sequence is one array subscript, which
`format_hex(buf, value, width)` in `metalc_base.h` has always done.
`format_int()` covers the decimal variant (`CVD`+`UNPK`+`OI`).

Both already existed. See `complex-asm-patterns.md` §7.1, added as a
result of this triage, for the recognition patterns.

**What this module did turn up** is that `format_int()` would write
past `width` when the value needed more digits than the field held —
a real overrun of a fixed-width WTO insert, now fixed and bounded.

### 3.2 RACROUTE reentrancy pattern — already correct upstream

The module keeps static `MF=L` **models** in the CSECT and copies them
to the dynamic work area before issuing `MF=(E,..)`:

```asm
*        Macro DEFINITIONS - DYNAMIC FORM     (in @DATD1 DSECT)
RACSTATD RACROUTE REQUEST=STAT,MF=L,RELEASE=2.4
*        Macro DEFINITIONS - STATIC FORM      (in CSECT, read-only model)
RACSTATS RACROUTE REQUEST=STAT,MF=L,RELEASE=2.4
RSTATLEN EQU  *-RACSTATS
```

This is exactly the strategy in `docs/racroute-metalc-patterns.md`, and
the static models are read-only, so CLAUDE.md rule 7 is satisfied
(`static const` tables are allowed). **No reentrant-ification needed**
— `docs/reentrant-ification-policy.md` does not apply.

---

## Section 4 — Complexity Flag Assessment

| Flag | Answer | Line(s) | Resolution / Notes |
|------|--------|---------|--------------------|
| Self-modifying code | **No** | — | |
| `EX` with variable target | **No** | — | No `EX` at all |
| Channel programs (CCW/EXCP) | **No** | — | |
| Cross-memory (PC/PT/SAC/SSAR) | **No** | — | |
| AR-mode (LAM/EAR/SAR) | **No** | — | |
| SVC calls | Indirect | — | Via RACROUTE (SVC 119) and MODESET; no raw `SVC` |
| ESTAE / SETFRR recovery | **Yes** | 1492, 1708 | `SETFRR A/D` with `EUT=YES`; needs stub |
| Key 0 / authorized state | **Yes** | 1490, 1512, 1707, 1709 | `MODESET EXTKEY=ZERO` around FRR setup |
| Conditional assembly | **Yes — significant** | 51 `AIF`, 20 `AGO`, 13 `MNOTE` | **See below** |
| Multiple CSECTs | **Yes** | 5 | 1 code + 4 data/table CSECTs |
| 64-bit instructions | **No** | — | 0 occurrences; `AMODE 31` only |
| `MVCL` / `ICM` | Yes (2 / 2) | — | Covered by `complex-asm-patterns.md` §3–4 |
| `TR` (not `TRT`) | Yes (17) | — | **`complex-asm-patterns.md` covers `TRT`, not `TR`** |
| Companion module | **Yes** | 68, 311 | Prologue refers to `IRR@XAC1` for `XAPLFUNC=1|3` — not in this file |

### 4.1 Conditional assembly is a scope decision, not a translation

The module is a **generator**. Global SET symbols at the very top select
which of several distinct modules the assembler emits:

```asm
&CLASSOPT     SETC  '2'     1 = Classification Model I  (classes per subsystem)
                            2 = Classification Model II (classes for all subsystems)
&CLASSNMT     SETC  'DSN'   DB2 subsystem name (up to 4 chars)
&CHAROPT      SETC  '1'     one character suffix
&ERROROPT     SETC  '1'     1 = defer to DB2 authorization on abend
                            2 = terminate DB2 on abend
```

C has no equivalent of assembly-time `AIF`/`AGO` over these. Three
options, in preference order:

1. **Convert one pinned configuration** and record it in the header
   (`&CLASSOPT=2, &CHAROPT=1, &ERROROPT=1`, the shipped defaults).
   Honest, testable, and matches how sites actually run it.
2. Convert `&CLASSOPT` and `&ERROROPT` to runtime configuration read
   from a parmlib member. Changes the module's behaviour envelope —
   needs the DB2 owner's agreement.
3. `#if`/`#ifdef` mirroring the SET symbols. Faithful but reproduces
   the combinatorial mess in C, and `make lint` would only ever check
   one arm.

**Recommend option 1**, with `docs/partial-scope-policy.md` followed and
the pinned values stated at the top of the converted file.

> `&ERROROPT` is a **security-relevant** switch: option 1 defers to DB2's
> own authorization when the exit abends, option 2 terminates DB2.
> Silently picking the wrong arm changes what happens on failure. It
> must be stated explicitly, never defaulted.

---

## Section 5 — Return Code Analysis

The decision is returned in **`EXPLRC1`**, not R15 — with `EXPLRC2`
carrying the reason. Offsets unknown (`DSNDEXPL` missing); widths from
the instruction stream.

| Condition | `EXPLRC1` | Meaning | Notes |
|-----------|-----------|---------|-------|
| Authorized | 0 | Permit | |
| Not authorized | 8 | Deny | `EXPLRC2` = reason |
| Unable to decide | 4 | Defer to DB2 | |
| Terminating error | 12 | Exit will not be called again | Line 1367, 1401 |
| Version mismatch | 8 / `EXPLRC2`=10 | Deny + IRR914I | Line 1388–1391 |

> **Default-deny check.** CLAUDE.md and
> `docs/racroute-metalc-patterns.md` require RC=8 (deny) when SAF is
> unavailable, and forbid treating RC=4 as a grant. Here **RC=4 means
> "defer to DB2's own authorization"** — a legitimate third state in
> *this* exit's protocol, not a bypass. A converter must not collapse
> 4 into either 0 or 8. Verify against the DB2 exit documentation
> before writing the RC constants, and do not reuse `RC_WARNING`
> blindly.

---

## Section 6 — Scope Decision

### Recommended scope

**None yet — blocked.** When the blockers clear, convert in this order:

| Phase | Content | Rationale |
|-------|---------|-----------|
| 0 | *(done)* `format_int()` bounds fix + `complex-asm-patterns.md` §7.1 | Completed during triage; no blocker dependency |
| 1 | `IRR@TPRV`, `IRR@TRUL`, `IRR@TRES` as `static const` tables | Pure data; needs no XAPL mapping |
| 2 | `DSNX@MSG` message skeletons + WTO | Self-contained; uses phase 0 |
| 3 | `WA_MAP` work area struct | In-file DSECT; fully mappable today |
| 4 | Mainline `DSNX@XAC` dispatch | **Requires `DSNDXAPL` + `DSNDEXPL`** |
| 5 | RACROUTE call sites | **Requires all four stubs** |
| 6 | FRR / `MODESET` recovery | Requires stubs; consider leaving in HLASM permanently |

### Excluded sections

| ASM function / label | Reason | Follow-on action |
|---|---|---|
| `@FRR_RTN` | FRR runs in key 0, disabled, on the FRR stack with SDWA | **Recommend keeping in HLASM.** Metal C is a poor fit for a recovery exit; a stub is the right boundary |
| `XAPLFUNC=1|3` paths | Prologue says `IRR@XAC1` owns them | Confirm whether this source really handles init/term (lines 3473–3479 suggest it does) — the prologue may predate a split |
| All XAPL/EXPL field access | DSECTs absent | Acquire `DSNDXAPL`/`DSNDEXPL` |

---

## Section 7 — Pre-Work Checklist

Blockers, in dependency order:

- [ ] **Obtain `DSNDXAPL` and `DSNDEXPL`** from `SDSNMACS` at your DB2
      level, or the equivalent tables from the *DB2 Administration
      Guide* ("Access control authorization exit"). Build
      `struct xapl` / `struct expl` from the macro, with an offset
      comment per field and the DB2 version recorded.
      **Until this is done there is nothing to convert.**
- [ ] Note that XAPL is **versioned** (`XAPLVERS`, `XAPLLVL`, `XAPLV7`,
      `XAPLV8`, and a `XAPLVERS_OK` check at line 3670). The struct must
      pin one DB2 level, and the exit must keep the version check.
- [ ] Write the four RACROUTE stubs per
      `docs/racroute-metalc-patterns.md` (`MF=(E,list)`, three-way RC,
      default-deny). **Strategy A (inline SVC 119) is banned.**
- [ ] Add `CPOOL`, `MODESET`, `SETFRR` to
      `docs/system-services-catalog.md`, then wrapper or stub each.
- [x] ~~Add the `UNPK`+`NC`+`TR` wrapper~~ — not needed; `format_hex()`
      already covers it. `complex-asm-patterns.md` §7.1 now documents
      the recognition pattern, and `format_int()`'s overrun is fixed.
- [ ] Decide the conditional-assembly configuration (Section 4.1) and
      get the `&ERROROPT` choice signed off.
- [ ] Resolve the `DSNX@XAC` → C identifier question (`#pragma csect`
      or a linker alias).
- [ ] Raise the `db2_xac_parm` shape mismatch (Section 2.2) as a
      separate defect — it is independent of this conversion.

## Section 8 — Conversion Agent Handoff

**Do not hand off.** `asm-to-metalc-converter` must not be run on this
module while Section 7's first item is open. With `DSNDXAPL` absent, the
only way to produce a compiling `DSNX@XAC` is to invent offsets for the
fields that decide DB2 authorization.

## Section 9 — Sign-off

| Role | Name | Date | Signature |
|------|------|------|-----------|
| Analyst (triage) | Claude Opus 5 | 2026-09-12 | *automated* |
| Human reviewer | | | |
| DB2 owner (`&ERROROPT`, RC=4 semantics) | | | |
| RACF owner (RACROUTE stubs) | | | |
