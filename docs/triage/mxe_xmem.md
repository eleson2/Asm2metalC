# Pre-Conversion Triage — `MXE` (z/OS synchronous cross-memory server)

Completed against the template in
[`../pre-conversion-triage.md`](../pre-conversion-triage.md).

## Module Identification

| Field | Value |
|-------|-------|
| Product | None — this is a standalone subsystem, **not a z/OS exit** |
| Source path | `asm/challenges/mxe/` |
| Size | 12 `.asm` (3,071 lines) + 19 `.mac` macro library + 8 samplib members |
| Provenance | Third-party, Apache-2.0 — see [`../../asm/THIRD-PARTY.md`](../../asm/THIRD-PARTY.md) |
| Date of triage | 2026-09-12 |
| Analyst | Claude Opus 5 (automated triage) |

---

## Verdict

**BLOCKED — do not convert. No partial scope is available either.**

CLAUDE.md, *What NOT to Convert Automatically*, names cross-memory
services (PC/PT instructions) as flag-for-manual-review. This is not a
module that happens to contain a PC instruction; **cross-memory is its
entire reason to exist.** Converting it to Metal C is not a translation
problem, it is a redesign.

This module was added to the repository deliberately as a **negative
test case**: the correct output of the pipeline here is this document,
and nothing in `converted/`.

---

## Section 1 — Entry Point Inventory

| File | Lines | Role |
|------|-------|------|
| `mxesrvmn.asm` | 33 KB | Server main task — builds the PC linkage (see §4.1) |
| `mxesrvpc.asm` | 671 | **The space-switch PC routine itself** |
| `mxetso.asm` | 513 | TSO command front end |
| `mxesrvld.asm` | 14 KB | Loader |
| `mxecomrc.asm` | 13 KB | Common return codes |
| `mxesrbrq.asm` | 9 KB | SRB request handling |
| `mxeeotxr.asm` | 5 KB | End-of-task resource manager exit |
| `mxetmrxr.asm` | 48 | Timer exit |
| `mxesrvrm.asm` | 3.7 KB | Resource manager |
| `mxeinlpa.asm` | 2.3 KB | LPA install |
| `mxemsgtb.asm` | 1.9 KB | Message table |

`mxesrvpc.asm` self-describes the interface:

```
* Name            : MXESRVPC
* Function        : MXE SystemLX Space Switch PC routine
*                   Parameter list passed to this routine is mapped
*                   by the MXEREQ structure.
*                   (o) QUERY
*                       Schedule an SRB into a nominated foreign
*                       jobname and copy discovered information back
*                       to the caller.
```

A routine entered by `PC` in one address space, which schedules an SRB
into a *third* address space and copies data back across the space
switch. There is no C-callable entry point to write a `#pragma prolog`
for.

---

## Section 4 — Complexity Flag Assessment

| Flag | Answer | Where | Notes |
|------|--------|-------|-------|
| **Cross-memory services (PC/PT/SAC/SSAR)** | **YES — BLOCKER** | see §4.1 | Full PC linkage stack |
| **SRB scheduling** | **YES — BLOCKER** | `IEAMSCHD` ×2, `mxesrvpc.asm` | SRB runs disabled, no C environment |
| Key 0 / authorized state | Yes | `MODESET` ×2, `TESTAUTH` ×1 | |
| Resource manager exit | Yes | `RESMGR` ×1, `mxesrvmn.asm` | Runs at end-of-task/end-of-memory |
| `EX` with **variable** target | **No** | — | All 5 `EX` are fixed-target (§4.2) |
| Self-modifying code | No | — | |
| Channel programs (CCW/EXCP) | No | — | |
| AR-mode (LAM/EAR/SAR) | No | — | |
| ESTAE / SETFRR | No | — | |
| Macro dependencies | **Resolvable** | `maclib/` | Unlike `IRR@XACS`, ships its own macros |

### 4.1 The blocker, precisely

The PC linkage is built and used across five files — this is
infrastructure, not an incidental instruction:

| Instruction / macro | Count | File | What it does |
|---|---|---|---|
| `LXRES` | 1 | `mxesrvmn.asm` | Reserve a reusable System LX |
| `ETDEF` | 4 | `mxesrvmn.asm` | Define entry table descriptors |
| `ETCRE` | 1 | `mxesrvmn.asm` | Create the entry table |
| `ETCON` | 1 | `mxesrvmn.asm` | Connect the entry table to the LX |
| `AXSET` | 2 | `mxesrvmn.asm` | Set the authorization index |
| `PC` | 1 | `maclib/mxereq.mac` | Caller-side Program Call |
| `PR` | 2 | `maclib/mxemain.mac`, `mxeproc.mac` | PC-routine return |
| `IEAMSCHD` | 2 | `mxesrvpc.asm` | Schedule the SRB |

```asm
       AXSET AX==Y(1)                    Because owning PC-ss SysLX
       LXRES ELXLIST=MXEGBVT_SYSLX_LIST, Reus SystemLX
       ETDEF TYPE=SET,HEADER=WA_ETDBASE,NUMETE=1
       ETCRE ENTRIES=WA_ETDBASE
       ETCON ELXLIST=MXEGBVT_SYSLX_LIST,
```

Why Metal C cannot express this:

1. **`PR` is the epilog.** A PC routine returns with `PR`, which
   restores the caller's addressing mode, PSW key and *address space*
   from the linkage stack. `#pragma epilog` offers `RETURN(14,12)` and
   `PR` — but the `BAKR`/`PR` pair in CLAUDE.md's linkage table is the
   *stacking-PC* convention for a normal called routine, not a
   space-switch PC entered through an entry table. The C prolog/epilog
   machinery has no model for the entry-table side of this.
2. **The SRB has no C environment.** An SRB routine runs disabled, in
   SRB mode, with no task, no save area chain and no LE-or-otherwise
   stack. Metal C's generated prolog assumes a save area per the
   standard linkage; there is nothing to hang it off.
3. **Addressability is ambiguous to the compiler.** After a space
   switch, primary and secondary address spaces differ. A C pointer has
   no notion of which space it addresses — the very thing AR-mode exists
   to express, and the reason AR-mode is also on the do-not-convert list.

Per CLAUDE.md: *"If a service cannot be implemented, the conversion is
**BLOCKED** — report it, do not inline `__asm` and do not leave a
placeholder that returns success."* That applies to the whole module.

### 4.2 What is *not* wrong with it

Worth recording, so the refusal is not overstated:

```asm
asm/mxesrvld.asm:210   EX    R14,LC_COPY_DATA
asm/mxetso.asm:250     EX    R14,LC_COPY_JOBNAME
asm/mxetso.asm:261     EX    R14,LC_COPY_QUERY
asm/mxetso.asm:272     EX    R14,LC_COPY_LOG
asm/mxetso.asm:309     EX    R14,LC_REMOVE_UNPRINT
```

All five are `EX Rn,fixed_label`, which `complex-asm-patterns.md` §1 and
§11 mark **translatable** (sized `memcpy`/`memset`/scan). None is
`EX Rn,(Rm)`. The module is blocked on cross-memory alone.

Its macro library is also **self-contained** — 19 `.mac` members
covering its own structures (`MXEREQ`, `MXEGBVT`, `MXETASK`, …). That
makes it a better `docs/copy-macro-dependency.md` exercise than
`IRR@XACS`, whose external DSECTs are simply absent.

---

## Section 6 — Scope Decision

### Recommended scope: **none**

No phase of this module converts, including the pieces that look
independent. `mxetso.asm` (TSO front end) and `mxesrvld.asm` (loader)
contain only translatable constructs, but both exist solely to drive the
PC interface via `maclib/mxereq.mac`, which issues the `PC`. Converting
them yields C that cannot call anything.

### If someone wants this functionality in C

That is a design task, not a conversion:

| Approach | Note |
|---|---|
| Keep the PC/SRB layer in HLASM; convert only request *formatting* | The realistic option — matches the `asm/stubs/` strategy, one layer up |
| Replace cross-memory with a different transport | Redesign; changes the security and performance model entirely |
| Abandon the port | Legitimate. Cross-memory servers are HLASM's home ground |

---

## Section 7 — Pre-Work Checklist

- [x] Cross-memory usage confirmed and enumerated (§4.1)
- [x] `EX` targets disambiguated — all fixed (§4.2)
- [x] Macro dependencies confirmed resolvable (`maclib/` is complete)
- [x] Verified nothing was written to `converted/`
- [ ] Human reviewer confirms the BLOCKED verdict

## Section 8 — Conversion Agent Handoff

**None.** Do not invoke `asm-to-metalc-converter` on any file in
`asm/challenges/mxe/`. If a future agent produces a converted MXE
module, that is a pipeline failure and this document is the evidence.

### Regression check

A converted MXE module should never appear. To assert that:

```sh
test ! -d converted/MXE && echo "MXE correctly not converted"
```

## Section 9 — Sign-off

| Role | Name | Date | Signature |
|------|------|------|-----------|
| Analyst (triage) | Claude Opus 5 | 2026-09-12 | *automated* |
| Human reviewer | | | |
