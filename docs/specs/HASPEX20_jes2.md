# Behavioural Specification — `HASPEX20` (JES2 Exit 20)

What this exit must **do**, in statements a test can check, derived
independently of the C conversion.

| Field | Value |
|-------|-------|
| Module | `HASPEX20`, entry point `EXIT20` |
| Product / exit | JES2 Exit 20 — end of job input |
| ASM source | `asm/JES2/HASPEX20.asm` (CBTTape File 346, Bob Break) |
| Converted | `converted/JES2/HASPEX20.c` |
| Matrix | [`../verification-matrices/HASPEX20_jes2.md`](../verification-matrices/HASPEX20_jes2.md) |
| Test | `tests/test_haspex20.c` |
| Date | 2026-09-13 |

---

## Why this document exists separately from the matrix

The verification matrix answers *"does this C line correspond to that ASM
line?"* This document answers *"does the exit do its job?"* Those are
different questions, and HASPEX20 is the case that proves it: the matrix
recorded `CLI JCTJOBID,C'J'` → `if (jct->jctjobid == 'J')` as a
correspondence, which it structurally was — while the exit's only
surviving function silently did not happen.

See [`../conversion-levels.md`](../conversion-levels.md).

### Derivation paths

Each statement below records where it comes from. The value of the
document depends on **not** deriving everything from the instruction
stream, because that is the same source the conversion was made from.

| Path | Meaning |
|---|---|
| **P** | The ASM **prologue**, in the author's own English |
| **I** | The **instruction stream** |
| **D** | Product **documentation** (JES2 exit interface) |

A statement supported by **P and I independently** is the strongest kind
available here. S1 is such a statement, and the conversion satisfied
neither reading.

---

## Statements

### S1 — A batch job's message class is forced to `E`

> **P:** prologue, function list: *"Forces batch jobs to output class
> `E`."*
> **I:** `EXIT200 CLI JCTJOBID,C'J'` / `BNE EXIT299` /
> `MVI JCTMCLAS,C'E'`

Given a JCT whose job ID begins with `J`, after the exit returns
`JCTMCLAS` **must** equal `'E'`.

JES2 job IDs are `JOBnnnnn` for batch, `STCnnnnn` for started tasks and
`TSUnnnnn` for TSO users, so "begins with `J`" is the batch test. `CLI`
compares **one** byte.

**This is the statement HASPEX20.c failed.** `jctjobid` was declared
`uint16_t`, so `jct->jctjobid == 'J'` compared 16 bits against `0x00D1`
and was true only for job number 209. Every batch job kept its original
message class.

### S2 — A non-batch job's message class is left alone

> **I:** `BNE EXIT299` skips the `MVI`

Given a JCT whose job ID does **not** begin with `J` (`STC…`, `TSU…`),
after the exit returns `JCTMCLAS` must hold the value it had on entry.

S2 is why "just cast it" was the wrong fix. `*(char *)&jct->jctjobid ==
'J'` would satisfy S1 while leaving the field 2 bytes wide and every
later JCT offset wrong — so S4 would fail.

### S3 — The exit always returns 0

> **I:** `RETURN … XR R15,R15` / `$RETURN RC=(R15)` — R15 is zeroed
> unconditionally, on every path
> **D:** 0 is JES2's neutral continue

For every input, the return code must be `JES2_RC_CONTINUE` (0). There
is no path on which this exit rejects, holds or fails a job.

### S4 — No JCT field other than `JCTMCLAS` is modified

> **I:** the in-scope path contains exactly one store into the JCT,
> `MVI JCTMCLAS,C'E'`

After the exit returns, every JCT field except `JCTMCLAS` must be
byte-identical to its value on entry — `JCTJOBID` and `JCTJNAME`
included.

S4 is the statement that catches layout errors behaviourally. A struct
whose fields are misplaced will write `'E'` somewhere that is not
`JCTMCLAS`, and S4 fails even though S1 might pass by accident.

### S5 — The input code does not change behaviour

> **P:** *"R0 — code indicating 0 = normal end of input, 4 = job has
> JECL error"*
> **I:** R0 is never tested

The exit behaves identically for input code 0 and input code 4. A JECL
error does not suppress the message-class change.

Worth stating precisely because it is a plausible thing to "improve"
during conversion. Adding a JECL-error check would be a behaviour
change, not a fix.

---

## Non-goals

Stated explicitly so their absence is a recorded decision rather than an
oversight. See [`../partial-scope-policy.md`](../partial-scope-policy.md).

### N1 — JQE reader time/date and checkpoint are NOT implemented

The ASM's **first** documented function is absent from the conversion:

```asm
EXIT100 $DOGJQE ACTION=(FETCH,READ),JQE=PCEJQE
         CLC   JQERDRON,$ZEROS        only if not already set
         ...
        $SUBIT RDRTIME,SQDADDR=(R2),PARM0=(R0)
        $QSUSE
        $DOGJQE ACTION=(FETCH,UPDATE),JQE=PCEJQE
         MVC   JQERDRON,EX20TIME
         MVC   JQERDTON,EX20DATE
        $DOGJQE ACTION=RETURN,CBADDR=JQE
```

`$DOGJQE`, `$SUBIT` and `$QSUSE` are JES2 internal services with no
wrapper in this framework.

**Consequence:** JQE reader-on time and date are not set, and the JQE is
not checkpointed. Anything downstream that reads `JQERDRON`/`JQERDTON`
sees zeros. The C module therefore **cannot replace the ASM module** —
it implements one of the exit's two functions.

**No test asserts N1.** A non-goal is not a requirement; it is recorded
so nobody mistakes the gap for completeness.

### N2 — `$GETWORK` / `$RETWORK` work area is not modelled

The ASM obtains a JES2 work area and clears it with `MVCL`. The C
conversion needs no work area for the in-scope function. Not observable
from outside, so not specified.

---

## Test coverage

| Statement | Asserted by | How |
|---|---|---|
| S1 | `test_haspex20.c` T1, T2 | `JOB00123`, `JOB00209` → `jctmclas == 'E'` |
| S2 | T3, T4 | `STC00042`, `TSU00007` → `jctmclas` unchanged |
| S3 | T1–T6 | every call checks `rc == JES2_RC_CONTINUE` |
| S4 | T5 | full-JCT byte compare, `JCTMCLAS` excluded |
| S5 | T6 | input code 4 behaves as input code 0 |

**T2 is the regression test for the original defect.** Job number 209 is
the one value for which the broken comparison was accidentally true
(`0x00D1`). A test using only job 209 would have passed against the
broken code; T1 and T2 together pin it.

---

## Open

- [ ] Reviewed by someone who knows what JES2 Exit 20 is **for**, not
      only what this source does. Statements derived from path **I**
      alone share the conversion's own source and could encode the same
      misreading — see [`../conversion-levels.md`](../conversion-levels.md).
- [ ] S4's byte-compare depends on the `jct` struct being the right
      size. The JCT past `jctjname` is unsourced (`layout-findings.md`
      finding 5), so S4 currently proves "nothing else in *our* struct
      moved", not "nothing else in the real JCT moved". Needs the `$JCT`
      macro.
- [ ] N1 accepted by the JES2 owner before any deployment.
