# Completed Triage Reports

Filled-in copies of [`../pre-conversion-triage.md`](../pre-conversion-triage.md),
one per module assessed. Step 1 of the pipeline in CLAUDE.md; the
matching post-conversion documents live in
[`../verification-matrices/`](../verification-matrices/).

A triage report is written **before** any conversion and states whether
one may begin at all. `BLOCKED` here means `converted/` stays empty for
that module.

| Module | Product | Verdict | Report |
|--------|---------|---------|--------|
| `IRR@XACS` / `DSNX@XAC` | DB2 + RACF | **BLOCKED** — mapping macros absent, 4 RACROUTE types unimplemented | [`IRR@XACS_db2.md`](IRR@XACS_db2.md) |
| `MXE` | none (cross-memory server) | **BLOCKED** — cross-memory PC/PT + SRB; no partial scope | [`mxe_xmem.md`](mxe_xmem.md) |

Both sources are third-party Apache-2.0 code included as conversion
challenges — see [`../../asm/THIRD-PARTY.md`](../../asm/THIRD-PARTY.md).

## Why two BLOCKED reports are a useful result

The 16 modules in `converted/` are all small, single-purpose exits that
convert cleanly. Neither of these does, and they fail differently:

- **`IRR@XACS` is convertible in principle but missing its inputs.** The
  interface is defined by two DB2 macros that are not in the source, so
  all 399 field references are symbolic and yield *no* offset evidence.
  An agent that proceeds anyway must invent the offsets that decide DB2
  authorization — and the result compiles and passes `make layout`. It
  is the `layout-findings.md` failure mode at 6,432 lines and in a
  security path.
- **`MXE` is not convertible at all.** Cross-memory PC/PT is on
  CLAUDE.md's do-not-convert list, and here it is the architecture
  rather than an incidental instruction.

Together they exercise the two things the offline harness cannot check:
whether triage recognises a missing input, and whether it refuses
instead of emitting a placeholder that links.

## The most useful thing they showed

**More assembler produced less evidence, by roughly fifty times.**

`asm-field-evidence.md` accepts only explicit base-displacement
references as proof of an offset. `tools/evidence_density.py` counts
them:

| Source | per 100 lines | `USING` |
|---|---|---|
| median, 16 repo sample exits | **17.5** | 1–3 |
| `IRR@XACS.asm` (IBM production) | **1.3** | 51 |
| `mxesrvmn.asm`, `mxesrvpc.asm` | **0.0** | 35, 25 |

Not because production code is worse — because it is better.
`CLC 16(7,R10),=C'PAYROLL'` is what you write when you do *not* have a
DSECT. Real modules use mapping macros and address symbolically, and a
symbolic reference proves width and type but **never** an offset.

The repo's own `HASPEX20.asm` scores 0.0, and its JCT struct is exactly
the one that stayed unsourced through `layout-findings.md` finding 5.
It was already real-shaped; it was just small enough that nobody noticed.

So the instruction-stream ledger is the **fallback**, and obtaining the
mapping macro is the **main path** for real work. That correction is in
`asm-field-evidence.md` §3.1, and `copy-macro-dependency.md` has been
promoted into the pre-conversion workflow in CLAUDE.md.

## Findings about the framework itself

Assessing `IRR@XACS` turned up three items that stand on their own,
independent of whether it is ever converted:

1. **`db2_xac_parm` has the wrong shape**, not merely unsourced
   offsets. The real `DSNX@XAC` takes `R1` → [`EXPLPTR`, `XAPLPTR`] —
   two control blocks by indirection — and returns its decision in
   `EXPLRC1`, not R15. The framework models it as one flat inline block
   with a work/func/flags header. See `IRR@XACS_db2.md` §2.2.
2. **`format_int()` could overrun a fixed-width field** — if the value
   needed more digits than `width`, it wrote them anyway, past the end
   of a WTO message slot. Fixed: it now fills with `'*'` and never
   exceeds `width`. It also had undefined behaviour negating the most
   negative `int32_t`.
3. **`UNPK` was wrongly documented as needing a wrapper.**
   `complex-asm-patterns.md` §7 said "ADD WRAPPER"; in fact
   `format_int()` and `format_hex()` already covered all 17 uses in
   `IRR@XACS`. The doc now has §7.1 showing how to recognise `UNPK`
   used as a formatter rather than as packed-decimal arithmetic.
4. **The evidence doctrine did not survive contact with real code.**
   See below.
