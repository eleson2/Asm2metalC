# Behavioural Specifications

What each exit must **do**, in statements a test can check.

These sit between the triage and the conversion, and they are the
artefact this framework was missing: the verification matrices audit
*transliteration* (does this C line correspond to that ASM line?), and
nothing audited *intent* (does the exit do its job?). See
[`../conversion-levels.md`](../conversion-levels.md) for why both are
needed.

| Exit | Product | Spec | Test |
|------|---------|------|------|
| `HASPEX20` / `EXIT20` | JES2 | [`HASPEX20_jes2.md`](HASPEX20_jes2.md) | `tests/test_haspex20.c` |

## Format

Numbered statements, each one testable, each tagged with where it came
from:

| Path | Source |
|---|---|
| **P** | The ASM prologue, in the author's own English |
| **I** | The instruction stream |
| **D** | Product documentation |

Plus **non-goals** — anything the conversion deliberately does not do,
with the consequence stated. A non-goal is not a requirement and gets no
test; it is recorded so nobody mistakes the gap for completeness.

Then a coverage table mapping each statement to the tests that assert
it.

## The one rule that makes this worth doing

**Derive the spec independently of the conversion.** A spec paraphrased
from the C adds a file and no information, and it launders the
conversion's errors into "requirements" — the same failure as
`verify_structs.c` asserting `VERIFY_OFFSET(jct, jctjname, 6)`, where
the assertion and the declaration came from one wrong source.

Prefer statements supported by more than one path. HASPEX20's S1 is
supported by **P and I** independently, and the conversion satisfied
neither — either one alone would have caught it.

A statement resting on **I** alone shares the conversion's own source
and cannot cross-check it. Those need a reviewer who knows what the exit
is *for*.

## When to write one

Always for a security or access decision. Otherwise when the conversion
is partial, when a control block it reads is unsourced, or when the
exit's effect is a side effect on a control block rather than a return
code. Reasoning in
[`../conversion-levels.md`](../conversion-levels.md) §6.
