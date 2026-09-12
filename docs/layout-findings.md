# Struct Layout Findings

What `tools/check_layout.py` and the host lint build found, and what is
still open.

Run them yourself:

```
make layout      # AMODE 31 - gated in CI
make layout64    # AMODE 64 - informational, see finding 3
make lint        # host compiler evaluates the assertions
```

---

## Resolved

### Finding 1 — MQ user-area fields declared as arrays of pointers

`mqcxp`, `mqaxp` and `mqaxc` each declared

```c
char          *exitUserArea[16];   /* +32  User area */
```

That is **16 pointers (64 bytes)**, not the 16-byte field MQ defines
(`MQ_EXIT_USER_AREA_LENGTH` = 16). Every field after it was displaced by
48 bytes, and the structs came out 48 bytes too long.

The headers' own comments proved the intent: the next field is
documented at +48, which is 32 + 16.

Fixed: the three fields are now `char exitUserArea[16]`. An MQ channel
or API exit reading `exitData` was previously reading 48 bytes past it.

### Finding 2 — `OPC_UX001_TERM` defined twice with different values

`metalc_opc.h` used one name for two unrelated things:

```c
#define OPC_UX001_TERM   RC_ERROR         /* +41  a return code, 8 */
#define OPC_UX001_TERM   EXIT_FUNC_TERM   /* +204 a function code, 3 */
```

The second definition wins, so an EQQUX001 exit returning
`OPC_UX001_TERM` to terminate the event reader returned **3, not 8**.

Fixed: the function codes are now `OPC_UX001_FUNC_INIT`,
`OPC_UX001_FUNC_PROCESS`, `OPC_UX001_FUNC_TERM`. The return code keeps
its name. `tools/lint_host.sh` now builds with `-Werror=macro-redefined`
so this cannot recur silently.

### Finding 4 — `EXIT_PARM_HEADER` structs were shifted by 2 bytes

**Was:** 117 mismatches across 9 structs in 5 headers.
**Now:** fixed. `EXIT_PARM_HEADER` expanded to four fields occupying
**+0 through +7**, but all nine structs documented their first field
after the macro at **+6**, inside `reserved` — so every later field was
2 bytes off and each declared total was 2 short.

**The assembler settled it.** Two products prove the common header is
**6 bytes** (`work` +0, `func` +4, `flags` +5) and that +6 belongs to
the product:

```asm
* DSN3ATH.asm - prologue documents "+6(2) Privilege requested"
         CLI   4(R10),3                func at +4, 1 byte
         CLC   8(8,R10),=CL8'SYSADM'   auth ID at +8, 8 bytes
         CLC   16(7,R10),=C'PAYROLL'   object name at +16
         MVC   60(4,R10),=F'200'       reason code at +60, 4 bytes

* FTCHKCMD.asm - reads +6 directly, as a 2-byte command code
         CLC   6(2,R10),=H'23'         DELE
         CLC   6(2,R10),=H'15'         STOR
         CLC   6(2,R10),=H'29'         SITE
```

`metalc_base.h` now carries both variants, and the layout tools know
the width of each:

```c
#define EXIT_PARM_HEADER      /* work +0, func +4, flags +5, reserved +6..7 */
#define EXIT_PARM_HEADER_6    /* work +0, func +4, flags +5 - product owns +6 */
```

**What was applied.** The nine structs switched to
`EXIT_PARM_HEADER_6`, which puts every field back where its comment
always said. 114 of the 117 baseline entries are gone; the 3 that
remain are finding 1's unrelated `ascb` fields.

| Struct | Header | Evidence |
|---|---|---|
| `db2_ath_parm` | `metalc_db2.h` | **ASM-proven** — `DSN3ATH.asm`, prologue and instruction stream agree |
| `ftp_chkcmd_parm` | `metalc_tcpip.h` | **ASM-proven** — `FTCHKCMD.asm` reads `6(2,R10)` three times |
| `db2_xac_parm`, `db2_sgn_parm`, `db2_edit_parm`, `db2_field_parm` | `metalc_db2.h` | Offset comments only — no exit here addresses them |
| `sa_rec_parm` | `metalc_sa.h` | Offset comments only |
| `ipflt_parm`, `tcpsec_parm` | `metalc_tcpip.h` | Offset comments only |
| `ims_flgx_parm` | `metalc_ims.h` | Offset comments only — separate shape, see below |

**Correction to an earlier version of this finding.** It listed
`tcpsec_parm` and `ipflt_parm` as *"Resolved — ASM in this repo pins
the offsets"*. They are not. The only TCP/IP assembler here is
`FTCHKCMD.asm`, and it pins `ftp_chkcmd_parm` — a **different** struct.
Nothing in this repository addresses `EZACSEC` or `EZBIPMXT`. Those two
structs are now self-consistent, which is not the same as sourced.

**`ftp_chkcmd_parm` was a different bug from the other eight.** Its
offsets were already right (`ftpuser` at +8), and it was never in the
baseline — but it had **no field at +6 at all**. The command code the
assembler reads was reachable only as the macro's `reserved`, and
`converted/TCPIP/FTCHKCMD.c` did exactly that:

```c
 *   parm->reserved = ftpcmd (Command code)     /* header comment */
    if (parm->reserved == 23) {                 /* DELE */
```

That read the correct two bytes, so the exit worked — but the field was
named `reserved` and the codes were bare literals. It now declares
`uint16_t ftpcmd; /* +6 */` and compares against `FTP_CMD_DELE`,
`FTP_CMD_STOR`, `FTP_CMD_SITE`.

**`ims_flgx_parm` needed its own shape.** It declares `flgxtype` at
**+5**, colliding with the header's `flags` byte, so it was off by 3 and
*neither* macro fits. Its header fields are now declared individually,
with `flgxtype` occupying the flags byte as its comment always
described.

**What this fixed in a converted exit.** `converted/DB2/DSN3ATH.c` is a
DB2 authorization exit, and every field it touched was 2 bytes out:

| Read | Was at | ASM says | Effect |
|---|---|---|---|
| `parm->athauth` | +10 | +8 | the `SYSADM` comparison read misaligned bytes |
| `parm->athobj` | +18 | +16 | the `PAYROLL` prefix test read misaligned bytes |
| `parm->athreasn` | +62 | +60 | wrote the reason code into the wrong word |

No source change was needed — the field names were right and only the
struct was wrong, so correcting the header fixed the exit.

**Still not sourced.** Seven of the nine structs rest on their own
offset comments. They are internally consistent and the +6 pattern now
has two independent witnesses in two different products, but neither is
evidence for `EZACSEC`, `EZBIPMXT`, `DSN3@SGN`, `DSN3@XAC`, the DB2
edit/field procedures, `AOFEXC30` or `DFSFLGX0`. Get the DSECT, or the
assembler of an exit that addresses them by displacement, before
trusting a field past +6 in any of them.

### Finding 5 — `jct.jctjobid` was declared 2 bytes; the assembler reads 8

**Fixed.** `metalc_jes2.h` declared:

```c
uint16_t  jctjobid;      /* +4   JES2 job number */
char      jctjname[8];   /* +6   Job name        */
```

Two exits contradicted it, and the second is decisive about the width:

```asm
* HASPEX20.asm:126
EXIT200  CLI   JCTJOBID,C'J'        byte 0 is character data

* HASPEX02.asm:135, with MSGJOBID DS CL8 in the work area
         MVC   MSGJOBID,JCTJOBID    8 bytes read from JCTJOBID
```

`MVC` takes its length from the first operand, and `MSGJOBID` is `CL8`.
`JCTJOBID` is therefore `CL8`, which puts `jctjname` at **+12** and
moves every field after it by 6.

**Two live defects, both now corrected:**

1. `converted/JES2/HASPEX20.c` compared a 16-bit field against `'J'`:

   ```c
   if (jct->jctjobid == 'J')      /* uint16_t == 0x00D1 */
   ```

   True only for job number 209, so batch jobs were never forced to
   msgclass `E` — **the exit's only function did not happen**. Now
   `jct->jctjobid[0] == 'J'`, matching what `CLI` tests.

2. `converted/JES2/HASPEX02.c` copied the **wrong field**:

   ```c
   memcpy_inline(work->msgjobid, jct->jctid, 8);   /* jctid, not jctjobid */
   ```

   The declared `uint16_t` could not supply 8 bytes, so the conversion
   substituted a neighbour — `jctid`, the 4-byte `'JCT '` eyecatcher at
   +0 — over-reading a `char[4]` by 4 bytes and printing the eyecatcher
   in the audit WTO. It now reads `jct->jctjobid`.

**Why no offline check caught it.** `make layout` compares the
declaration against the comment, and both were wrong together.
`tests/verify_structs.c` asserted `VERIFY_OFFSET(jct, jctjname, 6)` —
the harness certified the defect. Both now assert +12.

**Origin.** `asm-to-c-conversion-guide.md` section 3 used this layout to
illustrate offset comments. It was copied into `metalc_jes2.h`,
`docs/asm-to-metalc-jes2.md` and the assertion file as if it were a
source. The guide's example has been replaced and now carries a warning.

**The rest of the JCT is still unsourced.** `jctjobid` is settled — the
assembler is the specification and reads it 8 bytes wide. No exit here
addresses `jctjclas`, `jctprio`, `jctmclas` or anything after them by
displacement; they are reached symbolically under `USING JCT,R10`, so
this repository has never had evidence for their offsets. They were
shifted by 6 to preserve the relative layout the header documented,
which makes the struct **less wrong, not sourced**. `metalc_jes2.h`
now says so in a provenance comment on the struct itself. Supplying
real offsets needs the `$JCT` macro at your JES2 level.

See [`asm-field-evidence.md`](asm-field-evidence.md) section 5.

---

## Open

### Finding 3 — no header is AMODE 64 ready

`make layout64` reports a mismatch for every struct containing a
pointer. This is expected today and is a measure of work not yet done,
not a bug in the checker.

`docs/amode64-exits.md` requires pointer fields in mapped structs to be
guarded so they widen with the addressing mode:

```c
#ifdef __LP64__
    uint64_t  someptr;      /* +8  */
#else
    uint32_t  someptr;      /* +4  */
#endif
```

No product header carries those guards, and every offset comment records
the 31-bit layout. So **no exit can currently be built `-q64` with
correct struct layouts**, which affects the products `amode64-exits.md`
names: IMS 15+, MQ 9.3+, WLM.

This is not gating CI because nothing in `converted/` is an AMODE 64
exit yet. It must be resolved before the first one.

## How the baseline works

`tools/layout_known_issues.txt` lists `header:struct:field` for every
tracked mismatch. Both tools read it:

- `check_layout.py` reports them as known and does not fail the build;
  a **new** mismatch fails.
- `gen_struct_asserts.py` emits their assertions commented out, so the
  host lint build stays green.

If a baselined mismatch gets fixed, `check_layout.py` says so and asks
you to re-run `make baseline`. The baseline can only shrink by someone
deciding it should.

**A baseline entry is a deferred decision, not an accepted defect.**
Finding 4's diagnosis is closed — the assembler settled it. What remains
is applying the fix, and supplying evidence for the structs no exit in
this repository touches.
