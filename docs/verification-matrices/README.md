# Verification Matrices — Overview

This directory contains per-exit logic equivalence matrices for the
assembler-to-Metal-C conversions in `converted/`.

## Purpose

Each matrix provides a structured audit trail that answers: *does the
converted C code produce the same outcomes as the original assembler
for every reachable input combination?*

The matrices are not auto-generated. They are maintained by a human
reviewer who cross-checks the assembler listing against the C source
and documents every point of correspondence or deliberate divergence.

## Matrix Files

| File | Exit | Product | Status |
|------|------|---------|--------|
| [IEFU83_smf.md](IEFU83_smf.md) | IEFU83 | SMF | Draft |
| [ICHPWX01_racf.md](ICHPWX01_racf.md) | ICHPWX01 | RACF | Draft |
| [HASPEX02_jes2.md](HASPEX02_jes2.md) | EXIT02/HASPEX02 | JES2 | Draft |

## How to Read a Matrix

Each matrix has six sections:

1. **Module Identity** — names, sources, products, verifier.
2. **Entry Conditions** — register-to-variable mapping at function entry.
3. **Logic Equivalence Table** — row-per-significant-ASM-label with:
   - ASM label, ASM instruction(s), C equivalent, Verified flag, Concern notes.
4. **Return Code Path Inventory** — every exit path with expected RC and match status.
5. **Flagged Concerns** — numbered list of issues requiring follow-up.
6. **Sign-off Checklist** — reviewer checklist before marking a matrix "Verified".

## Verification Lifecycle

```
Draft  →  In Review  →  Verified
```

A matrix moves to **In Review** when all Logic Equivalence rows and RC
paths are filled in.  It moves to **Verified** when a second reviewer
has confirmed all concerns are either resolved or accepted as
intentional divergences.

## Related Files

- `includes/metalc_verify.h` — compile-time struct layout assertions
- `tests/verify_structs.c`  — compile-only struct size/offset checks
- `tests/test_iefu83.c`     — runtime test harness for IEFU83
