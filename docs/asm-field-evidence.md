# Reading Field Layout from the Instruction Stream

**Every HLASM instruction that touches storage is evidence about the field
it touches.** The instruction fixes the field's offset, its width, and
usually its type — before you have seen a single DSECT.

Use that evidence. A C struct that disagrees with the instruction stream
is wrong, and it is wrong in the most expensive way: it compiles, it
links, it runs, and it reads the wrong bytes.

This document is the procedure for extracting that evidence and for
resolving the contradictions it exposes. It is written for anyone
converting their own installation's exits — the two worked examples in
§5 and §6 are real defects found in this repository by applying it.

> **The rule this document exists to enforce:**
> **Make the header fit the assembler. Never make the assembler fit the header.**

---

## 0. Scope — the assembler is the specification

**This project assumes the source assembler is correct.** It is the
specification for the conversion, and the only authority on the layout
of the control blocks it touches. Whether the exit's own logic is
sensible, safe, or does what its author intended is a different
question, for a different exercise — do not answer it here.

Two consequences:

- **A conclusion derived from the instruction stream is a fact, not a
  hypothesis.** It does not need confirming against a vendor manual
  before you act on it. If the assembler moves 8 bytes out of a field,
  the field is 8 bytes.
- **A field the assembler never touches is unknown** — not suspect.
  You have no evidence either way, and no amount of re-reading the
  assembler will produce any. That is a missing input: you need the
  DSECT. Do not confuse it with a verification problem.

Keeping those apart is what stops "we should double-check this" from
becoming an excuse to leave a derivable defect in place.

---

## 1. Why this comes first

The usual conversion order is: read the DSECT, write the struct, then
translate the instructions. That order fails whenever the DSECT is not
available — no macro library, a vendor block documented only in a
manual, a hand-maintained mapping copied from a sample.

When that happens, the struct gets *invented*, and the instructions get
bent to fit it. The bend is silent. Two examples from this repository:

- A field declared 2 bytes wide that the assembler treats as 8 bytes of
  character text. The comparison against it can never be true, so the
  exit became a no-op (§5).
- A macro that contributed an invented 2-byte `reserved` field, pushing
  nine vendor parameter blocks 2 bytes out of alignment (§6).

Neither was caught by review. Both were caught by asking a mechanical
question: *what does the instruction say the field is?*

---

## 2. The evidence table

What each instruction proves about its storage operand.

### 2.1 Width and offset

| Instruction | Proves | Notes |
|---|---|---|
| `CLI  f,X'..'` / `MVI f,C'x'` | 1 byte at `f` | Immediate operand — always exactly one byte |
| `TM   f,X'80'` | 1 byte at `f`, used as bit flags | -> `uint8_t` |
| `CLC  d(n,B),x` | `n` bytes at displacement `d` | **Explicit length — strongest evidence** |
| `MVC  d(n,B),x` | `n` bytes at `d` | Explicit form |
| `MVC  A,B` | `L'A` bytes moved **from** `B` | Implicit length comes from the **first** operand; it therefore pins the width of the *source* too |
| `L` / `ST` | 4 bytes | Fullword; normally aligned |
| `LH` / `STH` | 2 bytes | Signed halfword |
| `LG` / `STG` | 8 bytes | 64-bit — see `amode64-exits.md` |
| `ICM R,B'0111',f` | 3 bytes at `f` | Mask bit count = byte count |
| `LA   R,f` | nothing about width | Address only |
| `MVCL` / `CLCL` | length in the odd register | Read the register setup, not the instruction |
| `PACK` / `UNPK` / `AP` / `CP` | packed decimal | `complex-asm-patterns.md` §7 |

### 2.2 Type

| Evidence | Field type |
|---|---|
| Compared with `C'...'` or `=C'...'` / `=CL8'...'` | Character (EBCDIC) — `char[n]` |
| Compared with `X'..'` via `TM` | Flag byte — `uint8_t` |
| Compared with `=F'...'`, loaded with `L`/`LH` | Binary integer — `int32_t` / `int16_t` |
| Target of `PACK` / operand of `AP`,`CP` | Packed decimal |
| Loaded with `L` then used as a base in `USING` | Pointer |

### 2.3 The strongest evidence of all

```asm
         CLC   16(7,R10),=C'PAYROLL'
```

Explicit base-displacement addressing — `d(len,base)` — pins **offset and
length together** and needs no DSECT, no macro library, and no vendor
manual. An exit written this way documents its own parameter block.

Symbolic references (`CLI JCTJOBID,C'J'` under a `USING`) still prove
width and type, but the offset comes from the DSECT you may not have.

### 2.4 Evidence outside the instruction

Do not overlook these — in the worked examples below they were decisive:

- **The prologue comment block.** Most exits document their parameter
  list. It is not authoritative, but it is a strong prior.
- **Local `DS` declarations that the field is copied into.** `MSGJOBID DS
  CL8` plus `MVC MSGJOBID,JCTJOBID` proves JCTJOBID is read 8 bytes wide.
- **Literals.** `=CL8'SYSADM'` proves an 8-byte character compare.
- **Other exits in the same product.** Two exits touching the same
  control block are two independent witnesses.

---

## 3. The procedure

Do this during triage (`pre-conversion-triage.md`), before writing the
struct.

**Step 1 — Build an evidence ledger.** One row per storage reference:

| Offset | Width | Type evidence | Instruction | Source |
|---|---|---|---|---|
| +4 | 1 | binary, value 3 | `CLI 4(R10),3` | DSN3ATH:57 |
| +8 | 8 | char (`=CL8'SYSADM'`) | `CLC 8(8,R10),...` | DSN3ATH:79 |
| +16 | 7 | char (`=C'PAYROLL'`) | `CLC 16(7,R10),...` | DSN3ATH:63 |
| +60 | 4 | binary (`=F'200'`) | `MVC 60(4,R10),...` | DSN3ATH:84 |

**Step 2 — Write the struct from the ledger.** Fields the ledger does
not cover become explicit `_filler[n]`. A gap you cannot account for is
a finding, not a rounding error.

**Step 3 — Reconcile against any existing header.** If a header already
maps this block, diff it against the ledger before reusing it. That diff
is where both defects in this document were hiding.

**Step 4 — Record contradictions as findings**, using §4.

---

## 4. Contradiction rules

When the C declaration and the instruction stream disagree, the
assembler wins — it is what the running system actually did.

**Rule 1 — A character comparison against a non-character field is a
layout defect, not a cast problem.**

```asm
         CLI   JCTJOBID,C'J'
```
```c
uint16_t jctjobid;                  /* declared 2-byte binary */
if (jct->jctjobid == 'J') { ... }   /* WRONG */
```

The C compares all 16 bits against `0x00D1`. Do **not** repair this with
`*(char *)&jct->jctjobid == 'J'`. That silences the symptom and keeps the
wrong width — every field after it is still misplaced. Fix the
declaration.

**Rule 2 — The width the instruction uses must equal `sizeof` the C
field.** If `MVC` moves 8 bytes out of a field the header calls 2 bytes,
the header is too narrow and every following offset is wrong.

**Rule 3 — If the C field cannot accept the operation, stop.** The
failure mode is substitution: the converter reaches for a nearby field
that *does* fit. That produces code which compiles and reads entirely
the wrong storage (§5).

**Rule 4 — An offset comment is a claim, not a fact.** Check it against
the ledger. `make layout` checks comments against what the compiler
produces; only the ledger checks either one against reality.

**Rule 5 — A shared macro must not contribute fields the vendor block
does not have.** See §6.

**Rule 6 — Where the evidence runs out, say so.** Mark the field
*unevidenced* — not *unverified*. Nothing about the assembler is in
doubt (§0); you simply have no ledger row for that offset, and need the
DSECT to get one. Raise it in the verification matrix. An honest gap is
recoverable; an invented field is not.

---

## 5. Worked example — `JCTJOBID` (JES2)

### The evidence

Two exits, two independent witnesses:

```asm
* HASPEX20.asm:126
EXIT200  CLI   JCTJOBID,C'J'          byte 0 is character 'J'

* HASPEX02.asm:135, with MSGJOBID DS CL8
         MVC   MSGJOBID,JCTJOBID      implicit length L'MSGJOBID = 8
```

`CLI` proves byte 0 holds character data. `MVC` into a `CL8` field
proves 8 bytes are read. Combined: **`JCTJOBID` is `CL8`** — consistent
with the JES2 convention of `JOB00123` / `STC00042` / `TSU00007`, and
with `HASPEX20`'s own purpose of testing for a `JOB`-prefixed ID.

### What the header said

```c
struct jct {
    char      jctid[4];      /* +0  'JCT ' identifier */
    uint16_t  jctjobid;      /* +4  JES2 job number   */   /* <-- 2 bytes */
    char      jctjname[8];   /* +6  Job name          */
```

A 2-byte binary "job number". This contradicts both witnesses, and it
places `jctjname` at +6 where the evidence puts it at +12.

### The two failures it caused

**In `HASPEX20.c` — the exit is a no-op.**

```c
if (jct->jctjobid == 'J') {          /* uint16_t == 0x00D1 */
    jes2_set_msgclass(jct, 'E');
}
```

True only if the whole halfword equals 209. Batch jobs are never forced
to msgclass `E`. The exit's single function silently does not happen.

**In `HASPEX02.c` — a different field is read (Rule 3).**

```asm
         MVC   MSGJOBID,JCTJOBID      8 bytes from the job ID
```
```c
memcpy_inline(work->msgjobid, jct->jctid, 8);   /* jctid, not jctjobid */
```

`jctjobid` was declared `uint16_t` and could not supply 8 bytes, so the
conversion took `jctid` — the 4-byte `'JCT '` eyecatcher at +0. The WTO
prints the eyecatcher plus 4 bytes of whatever follows, and the
`memcpy` over-reads a `char[4]` by 4 bytes. Compiles clean; wrong
output, every time.

### Where it came from

`asm-to-c-conversion-guide.md` §3 used this exact layout as an
*illustration* of offset comments:

```c
uint16_t  jctjobid;      /* +4   Job number         */
char      jctjname[8];   /* +6   Job name           */
/* Verify: offset of jctjname should be 6 */
```

The illustration was copied into `metalc_jes2.h` as fact, then into
`docs/asm-to-metalc-jes2.md`, then into `tests/verify_structs.c` — which
now *asserts* the wrong offset, so the whole toolchain agrees with
itself and reports green.

**A layout that appears in a teaching example is not a source.** Trace
every struct back to a DSECT, a vendor manual, or an evidence ledger.

---

## 6. Worked example — `EXIT_PARM_HEADER` (DB2, TCP/IP, and 7 others)

### The macro

```c
#define EXIT_PARM_HEADER       \
    void          *work;       /* +0  Work area pointer */ \
    uint8_t        func;       /* +4  Function code     */ \
    uint8_t        flags;      /* +5  Flags             */ \
    uint16_t       reserved    /* +6  Reserved          */
```

Eight bytes, +0 through +7. Nine structs use it and then declare their
own first field at **+6**, inside `reserved` — so every later field sits
2 bytes off and each declared total is 2 bytes short. 117 mismatches,
tracked in `tools/layout_known_issues.txt`.

`layout-findings.md` recorded two possible readings and could not choose
between them without vendor documentation:

1. the offset comments are wrong and the vendor block starts at +8; or
2. the comments are right and the real common header is 6 bytes.

### The evidence settles it

`DSN3ATH.asm` addresses its parameter block by explicit displacement —
no DSECT required:

```asm
         CLI   4(R10),3                func at +4, 1 byte
         CLC   16(7,R10),=C'PAYROLL'   object name at +16
         MVC   ...(8),8(R10)           auth ID at +8, 8 bytes
         CLC   8(8,R10),=CL8'SYSADM'   auth ID at +8, 8 bytes
         MVC   60(4,R10),=F'200'       reason code at +60, 4 bytes
```

with the prologue documenting `+6(2) Privilege requested`.

`FTCHKCMD.asm` gives the same shape for TCP/IP:

```asm
         CLI   4(R10),2                func at +4
         MVC   ...(8),8(R10)           user ID at +8
         CLI   16(R10),10              client IP first octet at +16
```

with `+6(2) Command code` in its prologue.

**Reading 2 is correct.** The real common header is **6 bytes** —
`work` (+0), `func` (+4), `flags` (+5) — and +6 holds a *real,
product-specific* field. The macro's `uint16_t reserved` is an invention
that collides with it. This is Rule 5: the shared macro contributed a
field the vendor blocks do not have.

The offset comments on all nine structs were right the whole time.

### Scope of the resolution

| Structs | Status |
|---|---|
| `db2_ath_parm`, `tcpsec_parm`, `ipflt_parm` | **Resolved** — ASM in this repo pins the offsets |
| `db2_xac_parm`, `db2_sgn_parm`, `db2_edit_parm`, `db2_field_parm`, `sa_rec_parm` | Same +6 pattern and internally consistent, but **no exit in this repository touches them**. Derive from your own ASM for those exits, or from the DSECT — there is no evidence here to reason from |
| `ims_flgx_parm` | Different — declares `flgxtype` at **+5**, colliding with the macro's `flags`. Off by 3, needs its own resolution |
| The other 25 users of the macro | Document their next field at +8 and are consistent with it. Leave alone |

The macro is not wrong everywhere — it is wrong where a product's block
has a real field at +6. That is why the fix is per-struct and not a
single edit to `metalc_base.h`.

### Guidance for your own conversions

`ai-conversion-steering.md` §2 mandates `EXIT_PARM_HEADER` when the
block starts with `(work, func, flags, reserved)` at +0..+7. **Verify
the `reserved` half before you accept it.** If your ASM references
anything at +6 or +7, the macro does not describe your block: declare
the four fields explicitly instead, and keep the vendor's own name for
the field at +6.

---

## 7. Checklist

Before a struct is trusted:

- [ ] Every storage reference in the ASM appears in the evidence ledger
- [ ] Every ledger row's width equals `sizeof` the corresponding C field
- [ ] Every character comparison targets a `char` field, not an integer
- [ ] No `memcpy_inline` / `memcmp_inline` length exceeds its field's size
- [ ] The C code reads the *same field* the ASM read — not a nearby one
      that happened to fit
- [ ] Fields with no ledger evidence are named `_filler` or flagged
- [ ] Struct provenance is recorded: DSECT, vendor manual, or ledger —
      **never "copied from an example"**
- [ ] Any contradiction is written up in the verification matrix

---

## 8. What the offline tools do and do not catch

| Check | Catches | Misses |
|---|---|---|
| `make layout` | Offset comments vs. compiler-computed offsets | Both being wrong together |
| `tests/verify_structs.c` | Same, at compile time on z/OS | Same — it asserts the comments |
| `make conform` | CLAUDE.md rule violations | Anything about field semantics |

All three compare the header against **itself**. Only the evidence
ledger compares it against the assembler. That is the gap this document
covers, and it is why §5 stayed green through the entire harness.

---

## 9. Related documents

- [`pre-conversion-triage.md`](pre-conversion-triage.md) — build the ledger here
- [`asm-to-c-conversion-guide.md`](asm-to-c-conversion-guide.md) §3 — DSECT→struct mechanics
- [`complex-asm-patterns.md`](complex-asm-patterns.md) — `ICM`, `MVCL`, packed decimal widths
- [`layout-findings.md`](layout-findings.md) — the open findings in this repository
- [`amode64-exits.md`](amode64-exits.md) — pointer widths under AMODE 64
