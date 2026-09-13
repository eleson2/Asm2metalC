# Tests — Metal C Exit Test Harness

This directory contains compile-time verification files and runtime
unit tests for the Metal C exit conversions in `converted/`.

---

## File Inventory

| File | Purpose |
|------|---------|
| `verify_structs.c` | **Generated** - compile-only; asserts struct sizes and field offsets. Do not edit: run `make generate`. |
| `test_harness_template.c` | Template for new per-exit test programs |
| `test_iefu83.c` | Unit tests for IEFU83 (SMF record filtering) |
| `test_haspex20.c` | Unit tests for HASPEX20 / EXIT20 (JES2 exit 20); asserts `docs/specs/HASPEX20_jes2.md` |

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

---

## Writing a test from a specification, not from the code

`test_haspex20.c` is the reference for the pattern described in
[`../docs/conversion-levels.md`](../docs/conversion-levels.md). Each
test names the numbered statement it asserts:

```c
/*  T1 — S1, S3.  Ordinary batch job.  */
mock_build_jct(&jct, "JOB00123");
rc = EXIT20(EX20_CODE_NORMAL, NULL, &jct);
CHECK("S1 batch JOB00123 forced to E",
      jct.jctmclas == EXPECTED_MSGCLASS && rc == JES2_RC_CONTINUE);
```

The point is that the assertion comes from
`docs/specs/HASPEX20_jes2.md` — *"a batch job's message class is forced
to `E`"* — and **not** from reading `converted/JES2/HASPEX20.c`. A test
written by paraphrasing the C asserts whatever the C happens to do,
including its bugs. That is how `verify_structs.c` came to assert
`VERIFY_OFFSET(jct, jctjname, 6)`, the wrong offset.

### Three techniques worth copying

**1. A sentinel, so "unchanged" is observable.** `mock_build_jct` presets
`jctmclas` to `'A'`. Zeroing it would make "left alone" and "set to
something unexpected" indistinguishable.

**2. A negative case that pins the comparison width.** `CLI` tests one
byte, so `XJOB0001` — `'J'` present but not in byte 0 — must be left
alone. That fails any whole-field or substring comparison, which a
positive test cannot detect.

**3. A whole-struct byte compare for "nothing else changed."**

```c
memcpy_inline(&before, &jct, sizeof(struct jct));
rc = EXIT20(EX20_CODE_NORMAL, NULL, &jct);
CHECK("S4 only jctmclas modified",
      !jct_differs_outside_msgclass(&before, &jct));
```

This is the behavioural counterpart to a layout check: a struct whose
fields are misplaced writes `'E'` somewhere that is not `jctmclas`, and
this fails even if the positive test passes by accident.

### Assert field widths, not only offsets

`VERIFY_FIELD_SIZE` (in `metalc_verify.h`) asserts a field's **width**.
`check_layout.py` and `verify_structs.c` assert offsets, and a field
declared the wrong width shifts every later offset while the declaration
and its comment stay consistent with each other. Width is the dimension
`layout-findings.md` finding 5 turned on.

```c
VERIFY_FIELD_SIZE(jct, jctjobid, 8);   /* HASPEX02.asm: MVC MSGJOBID,JCTJOBID
                                          with MSGJOBID DS CL8 */
```

Compiled against the original broken header, `test_haspex20.c` fails
**before it links**:

```
error: '_chk_jct_jctjobid_w' declared as an array with a negative size
```

Add one wherever an instruction pins a length — `CLC 8(8,R10)`,
`CLI 4(R10)`, `MVC 60(4,R10)`.
