# Two Levels of Conversion

A HLASM exit can be converted at two different levels, and they catch
different classes of error. **Neither is sufficient alone.** This
repository has live examples of both failure classes, which is the
argument for doing both.

| Level | What it produces | What it preserves |
|---|---|---|
| **Transliteration** | C that mirrors the ASM construct by construct | **Structure.** Auditable line by line against the source |
| **Specification** | A statement of what the program must *do*, then C that satisfies it | **Intent.** Testable without reference to the source |

---

## 1. Transliteration fails by keeping the shape and losing the meaning

`converted/JES2/HASPEX20.c`, before it was fixed:

```asm
EXIT200  CLI   JCTJOBID,C'J'          →   if (jct->jctjobid == 'J')
         MVI   JCTMCLAS,C'E'          →       jes2_set_msgclass(jct, 'E');
```

Structurally parallel. Reviewable. Recorded in the verification matrix
as a correspondence, which it genuinely was. And **the branch was never
taken for any job** — `jctjobid` was declared `uint16_t`, so the test
compared a halfword against EBCDIC `0x00D1`, while the real field is
`CL8` and its first two bytes are always letters (`'JO'` = `0xD1D6`).
The exit's only surviving function did not happen, ever.

`HASPEX02.c` failed the same way in the opposite direction:
`MVC MSGJOBID,JCTJOBID` became a `memcpy` from `jctid` — **a
neighbouring field that happened to fit**. The narrow declaration could
not supply 8 bytes, so the translation reached for something that could.
Locally plausible at every step; the audit message printed an
eyecatcher.

**What went wrong in both cases is not visible at the line level.** Only
a statement of purpose — *"batch jobs get message class E"*, *"the audit
message shows the job ID"* — contradicts them.

## 2. Specification fails by getting the behaviour right on the wrong substrate

`converted/DB2/DSN3ATH.c` is the mirror image. Its logic was correct:
check for `SYSADM`, match the `PAYROLL` prefix, set a reason code. Any
behavioural specification would have said exactly that, and the code
satisfied it.

It was reading offsets +10, +18 and +62 instead of +8, +16 and +60,
because `EXIT_PARM_HEADER` contributed two bytes the real parameter
block does not have. A DB2 authorization exit comparing the wrong bytes
against `SYSADM`.

**No specification catches that.** Only `CLC 8(8,R10),=CL8'SYSADM'`
does — the instruction stream, read as evidence.

## 3. So: both, and they must be derived independently

The value is not in having two documents. It is in having **two
derivations that must agree**. If the specification is produced by
paraphrasing the transliteration, it adds a file and no information —
and worse, it launders the transliteration's errors into a
"requirement".

This is the same trap as `tests/verify_structs.c` asserting
`VERIFY_OFFSET(jct, jctjname, 6)`: the harness certified the defect
because the assertion and the declaration came from the same wrong
source. **At the specification level the trap is identical, one
abstraction higher.**

Practical rule: record where each statement came from, and prefer
statements with more than one source. `docs/specs/HASPEX20_jes2.md`
tags each one:

| Path | Source |
|---|---|
| **P** | The ASM prologue, in the author's own English |
| **I** | The instruction stream |
| **D** | Product documentation |

HASPEX20's S1 — *"a batch job's message class is forced to `E`"* — is
supported by **P and I independently**. The prologue says *"Forces batch
jobs to output class E"* in plain English; the instruction stream says
`CLI JCTJOBID,C'J'` / `MVI JCTMCLAS,C'E'`. The conversion satisfied
neither, and either one alone would have caught it.

A statement resting on **I alone shares the conversion's own source**
and cannot cross-check it. Such statements need a reviewer who knows
what the exit is *for*.

## 4. What each artefact is for

| Artefact | Level | Answers | Needs a human? |
|---|---|---|---|
| `docs/triage/` | — | Can this be converted at all? What inputs are missing? | To obtain the inputs |
| `docs/specs/` | Specification | What must it do? What is explicitly out of scope? | To confirm purpose, not just code |
| `converted/` | Transliteration | — | — |
| `docs/verification-matrices/` | Transliteration | Does each C construct correspond to its ASM? | **Yes** — requires reading both |
| `tests/test_*.c` | Specification | Does it actually do it, on the target? | No — runs unattended |

The asymmetry in the last column is the practical argument for the
specification level. A verification matrix is checkable only by someone
fluent in both languages, which is why the sign-off boxes in
`docs/verification-matrices/` are still empty. A spec-derived test runs
on every build, forever, and needs nobody.

## 5. Worked example

`docs/specs/HASPEX20_jes2.md` and `tests/test_haspex20.c` are the
reference pair. The test asserts numbered statements from the spec, not
lines of the C, and it demonstrates the point concretely: compiled
against the original broken header it fails **before it links** —

```
error: '_chk_jct_jctjobid_w' declared as an array with a negative size
```

— because the spec's layout precondition is expressed as
`VERIFY_FIELD_SIZE(jct, jctjobid, 8)`, and the assembler proves the
field is 8 bytes wide.

Note what that assertion adds over the existing harness:
`check_layout.py` and `verify_structs.c` assert **offsets**, and a field
declared the wrong **width** shifts every later offset while both
comments and declaration stay internally consistent. Width is the
dimension finding 5 turned on. Assert it wherever an instruction pins a
length:

```asm
CLC  8(8,R10),=CL8'SYSADM'   ->  VERIFY_FIELD_SIZE(x, authid, 8)
CLI  4(R10),3                ->  VERIFY_FIELD_SIZE(x, func,   1)
MVC  60(4,R10),=F'200'       ->  VERIFY_FIELD_SIZE(x, reasn,  4)
```

## 6. When to skip the specification level

Honestly: for a genuinely trivial exit it is overhead.
`tests/test_iefu83.c` was the only behavioural test in this repository
for a long time, and IEFU83 is also the only exit whose matrix concerns
all resolved as *"equivalent, no concern"*. That is `n=1`, and IEFU83 is
the simplest exit here, so simplicity may be doing the work rather than
the test.

The mechanism is still real: **nothing anywhere in this repository
stated "batch jobs get msgclass E", so nothing could contradict
HASPEX20 being inert.**

Write the spec when any of these is true:

- The exit makes a **security or access decision** (RACF, SAF, DB2
  authorization, logon, sign-on) — always, regardless of size
- The conversion is **partial**, so the non-goals need recording
  (`docs/partial-scope-policy.md`)
- A control block it reads is **unsourced**, so a layout error is
  plausible
- The exit's effect is a **side effect on a control block** rather than a
  return code — those are exactly the cases a matrix review eyeballs and
  a test nails

HASPEX20 hit three of the four.

## Related

- [`pre-conversion-triage.md`](pre-conversion-triage.md) — step 1, inputs and blockers
- [`specs/`](specs/) — behavioural specifications
- [`verification-matrices/`](verification-matrices/) — transliteration audit
- [`asm-field-evidence.md`](asm-field-evidence.md) — what an instruction proves; §3.1 on when it proves nothing
- [`partial-scope-policy.md`](partial-scope-policy.md) — recording non-goals
- [`layout-findings.md`](layout-findings.md) — findings 4 and 5, the two worked failures above
