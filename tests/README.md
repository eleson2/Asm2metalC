# Tests — Metal C Exit Test Harness

This directory contains compile-time verification files and runtime
unit tests for the Metal C exit conversions in `converted/`.

---

## File Inventory

| File | Purpose |
|------|---------|
| `verify_structs.c` | Compile-only: asserts struct sizes and field offsets |
| `test_harness_template.c` | Template for new per-exit test programs |
| `test_iefu83.c` | Unit tests for IEFU83 (SMF record filtering) |

---

## Layer 1 — Compile-time Struct Verification

**File:** `verify_structs.c`

Uses the `VERIFY_SIZE` / `VERIFY_OFFSET` macros from
`includes/metalc_verify.h` to assert that every struct used by the
converted exits has exactly the right size and field offsets.

A compile error of the form:
```
error: size of array '_chk_jct_sz' is negative
```
means the named struct or field does not match the z/OS DSECT layout.
Fix the struct definition in the relevant header before proceeding.

**Build:**
```sh
xlc -qmetal -S -qlist -I../includes verify_structs.c
```

No assembly or link-edit step is needed.  A clean compile is the pass
criterion.  Run this **first** — if any struct is wrong, runtime tests
will produce meaningless results.

---

## Layer 4 — Runtime Unit Tests

### How the Harness Works

Each test program is a self-contained Metal C module:

- No Language Environment (LE) — no libc, no malloc, no printf.
- `test_main` is the ENTRY point, invoked by JCL.
- Pass/fail messages are issued via `wto_simple` / `wto_important`
  and appear in the system log (SYSLOG/OPERLOG).
- `format_int` from `metalc_base.h` formats numeric output.
- Returns **0** if all tests pass, **8** if any test fails.

### test_iefu83.c

Tests the four primary filtering paths of IEFU83:

| # | Record type | Job name | Expected RC |
|---|-------------|----------|-------------|
| 1 | 40 | — | `SMF_RC_SUPPRESS` (4) |
| 2 | 42 | — | `SMF_RC_SUPPRESS` (4) |
| 3 | 4 | `SYSPRINT` | `SMF_RC_SUPPRESS` (4) |
| 4 | 4 | `PAYROLL ` | `SMF_RC_WRITE` (0) |

**Build:**
```sh
xlc -qmetal -S -qlist -I../includes test_iefu83.c
xlc -qmetal -S -qlist -I../includes ../converted/SMF/IEFU83.c
```

**Link-edit and run (example JCL):**
```jcl
//LKED   EXEC PGM=IEWL,PARM='RENT,REUS,AMODE=31,RMODE=ANY,ENTRY=test_main'
//SYSLIN   DD *
  INCLUDE OBJLIB(TEST83)
  INCLUDE OBJLIB(IEFU83)
  NAME    TIEFU83(R)
/*
//RUN    EXEC PGM=TIEFU83
//SYSUDUMP DD SYSOUT=*
```

Expected SYSLOG output (all passing):
```
PASS: type40 suppress
PASS: type42 suppress
PASS: type4 SYS-prefix suppress
PASS: type4 non-SYS write
TESTS:    4 P:    4 F:    0
```

A step completion code of **0** means all tests passed.
A step completion code of **8** means at least one test failed;
look for `FAIL:` lines in the SYSLOG.

### Adding New Tests

1. Copy `test_harness_template.c` to `test_<MODULE>.c`.
2. Add the product header include and a `extern int <MODULE>(...)` declaration.
3. Implement `mock_build_*` helpers to construct test input structures.
4. Add `CHECK()` calls in `test_main`.
5. Build both the test module and the exit module; link together.

Planned future test programs:
- `test_ichpwx01.c` — password rule coverage for ICHPWX01
- `test_haspex02.c` — CLASS= scanning paths for HASPEX02

---

## Relationship to Verification Matrices

The matrices in `docs/verification-matrices/` provide the logic
audit trail.  The tests here provide the executable confirmation.
Both are required before marking an exit as **Verified**.

| Exit | Matrix | Test |
|------|--------|------|
| IEFU83 | `docs/verification-matrices/IEFU83_smf.md` | `test_iefu83.c` |
| ICHPWX01 | `docs/verification-matrices/ICHPWX01_racf.md` | (future) |
| HASPEX02 | `docs/verification-matrices/HASPEX02_jes2.md` | (future) |
