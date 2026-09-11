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

### Finding 4 — `EXIT_PARM_HEADER` structs are shifted by 2 bytes

**117 mismatches across 9 structs in 5 headers.** Tracked in
`tools/layout_known_issues.txt`; assertions for these structs are
emitted commented-out in `tests/verify_structs.c`.

Affected: `db2_xac_parm`, `db2_ath_parm`, `db2_sgn_parm`,
`db2_edit_parm`, `db2_field_parm`, `ims_flgx_parm`, `sa_rec_parm`,
`ipflt_parm`, `tcpsec_parm`.

`EXIT_PARM_HEADER` expands to four fields occupying **+0 through +7**:

```c
void     *work;      /* +0 */
uint8_t   func;      /* +4 */
uint8_t   flags;     /* +5 */
uint16_t  reserved;  /* +6 */
```

`docs/ai-conversion-steering.md` §2 confirms 8 bytes — its example DSECT
puts the next field at +8. But all nine structs document their first
field after the macro at **+6**, inside `reserved`:

```c
EXIT_PARM_HEADER;            /* +0   Common header   */
uint8_t   xactype;           /* +6   Connection type */   <-- overlaps
```

Every later field is 2 bytes off, and each declared total is 2 short.
(`ims_flgx_parm` documents +5, so it is off by 3.)

Both tools agree independently: `check_layout.py` computes the shift,
and the host lint build rejects the same nine `VERIFY_SIZE` assertions.

**What was unresolved.** Two readings, needing opposite corrections:

1. The offset comments are wrong. The vendor block really does start at
   +8, and the comments should be renumbered.
2. The comments are right and `EXIT_PARM_HEADER` is the wrong macro for
   these products — their real common header is 6 bytes, and these
   structs should declare their own fields instead of using the macro.

Reading 1 changes only comments. Reading 2 changes the layout the exit
actually reads. Choosing wrong silently corrupts every field access in
nine parameter blocks across DB2, IMS, System Automation and TCP/IP.

**Reading 2 is correct.** The evidence was in this repository the whole
time — in the assembler these exits were converted *from*.

`DSN3ATH.asm` addresses its parameter block by explicit
base-displacement, which needs no DSECT:

```asm
         CLI   4(R10),3                func at +4, 1 byte
         CLC   16(7,R10),=C'PAYROLL'   object name at +16
         CLC   8(8,R10),=CL8'SYSADM'   auth ID at +8, 8 bytes
         MVC   60(4,R10),=F'200'       reason code at +60, 4 bytes
```

`FTCHKCMD.asm` gives the same shape for TCP/IP (`CLI 4(R10),2`,
user ID at `8(R10)`, client IP at `16(R10)`). Both prologues document a
real 2-byte field at +6 — `Privilege requested` for DB2, `Command code`
for FTP.

So the common header is **6 bytes** (`work` +0, `func` +4, `flags` +5),
+6 belongs to the product, and the macro's `uint16_t reserved` is an
invention that collides with it. The nine structs' offset comments were
right all along.

Full derivation and the general technique:
[`asm-field-evidence.md`](asm-field-evidence.md) §6.

**Status per struct:**

| Structs | Status |
|---|---|
| `db2_ath_parm`, `tcpsec_parm`, `ipflt_parm` | Resolved — ASM in this repo pins the offsets |
| `db2_xac_parm`, `db2_sgn_parm`, `db2_edit_parm`, `db2_field_parm`, `sa_rec_parm` | Same +6 pattern and internally consistent, but **no exit here touches them**. No evidence either way — derive from the ASM of an exit that does, or from the DSECT |
| `ims_flgx_parm` | Separate problem: declares `flgxtype` at **+5**, colliding with the macro's `flags`, so it is off by 3. Needs its own resolution |

The macro is correct for the other 25 structs that use it, which
document their next field at +8. The fix is therefore per-struct — not
a single edit to `metalc_base.h`.

**Not yet applied.** Correcting the three resolved structs shifts every
field after +6 and changes two converted exits; it needs its own change,
with `make baseline` re-run afterwards to drop the entries and re-enable
the assertions.

Until then:
- These nine structs are the reason `converted/DB2/DSN3ATH.c` and
  `converted/TCPIP/FTCHKCMD.c` should not be trusted in production.
- Add a HIGH concern to each affected verification matrix.

### Finding 5 — `jct.jctjobid` is declared 2 bytes; the assembler reads 8

`metalc_jes2.h` declares:

```c
uint16_t  jctjobid;      /* +4   JES2 job number */
char      jctjname[8];   /* +6   Job name        */
```

Two exits contradict it:

```asm
* HASPEX20.asm:126
EXIT200  CLI   JCTJOBID,C'J'        byte 0 is character data

* HASPEX02.asm:135, with MSGJOBID DS CL8
         MVC   MSGJOBID,JCTJOBID    8 bytes read from JCTJOBID
```

`JCTJOBID` is `CL8`. `jctjname` therefore belongs at **+12**, not +6,
and every field after it moves by 6.

**Two live defects follow from it:**

1. `converted/JES2/HASPEX20.c` — `if (jct->jctjobid == 'J')` compares a
   16-bit field against `0x00D1`. It is true only for job number 209, so
   the exit never forces batch jobs to msgclass `E`. **The exit's only
   function does not happen.**
2. `converted/JES2/HASPEX02.c` — `memcpy_inline(work->msgjobid,
   jct->jctid, 8)` copies the **wrong field**: `jctid` is the 4-byte
   `'JCT '` eyecatcher at +0. The declared `uint16_t` could not supply 8
   bytes, so the conversion substituted a neighbour. It also over-reads
   a `char[4]` by 4 bytes. The audit WTO prints the eyecatcher instead of
   the job ID.

Neither was caught by any offline check: `make layout` compares the
comment against the declaration, and both are wrong together.
`tests/verify_structs.c` asserts `VERIFY_OFFSET(jct, jctjname, 6)` —
the harness certifies the defect.

**Origin.** `asm-to-c-conversion-guide.md` §3 used this layout as an
illustration of offset comments. It was copied into `metalc_jes2.h`,
`docs/asm-to-metalc-jes2.md`, and the assertion file as if it were a
source. The guide's example has been replaced and now carries a warning.

**Not yet applied.** The correction shifts every JCT field after +4 and
touches `HASPEX02.c`, `HASPEX20.c`, three docs, two examples and the
assertion file.

`JCTJOBID` is settled: the assembler is the specification, and it reads
the field 8 bytes wide. That correction needs no further confirmation.

What is *not* settled is the rest of the JCT. No exit here addresses
`JCTJCLAS`, `JCTPRIO` or `JCTMCLAS` by displacement — they are reached
symbolically under `USING JCT,R10` — so this repository has never had
evidence for their offsets, before or after this finding. The struct as
a whole has no provenance. That is a missing input, not a doubt about
the assembler: it needs the `$JCT` macro at your JES2 level. Correcting
`jctjobid` alone makes the struct less wrong, not sourced.

See [`asm-field-evidence.md`](asm-field-evidence.md) §5.

---

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
