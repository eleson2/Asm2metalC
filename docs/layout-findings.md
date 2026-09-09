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

**Why this is not "fixed" here.** Two readings, and they need opposite
corrections:

1. The offset comments are wrong. The vendor block really does start at
   +8, and the comments should be renumbered.
2. The comments are right and `EXIT_PARM_HEADER` is the wrong macro for
   these products — their real common header is 6 bytes, and these
   structs should declare their own fields instead of using the macro.

Reading 1 changes only comments. Reading 2 changes the layout the exit
actually reads. Choosing wrong silently corrupts every field access in
nine parameter blocks across DB2, IMS, System Automation and TCP/IP.

**Resolving it needs the vendor documentation** for each exit's
parameter list — the DB2 access-control exit block, the IMS logger exit
block, and so on. That is a per-product question, not one this
repository can settle.

Until then:
- These nine structs are the reason `converted/DB2/DSN3ATH.c` and
  `converted/TCPIP/FTCHKCMD.c` should not be trusted in production.
- Add a HIGH concern to each affected verification matrix.
- After resolving, run `make baseline` to drop them and re-enable their
  assertions.

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
Finding 4 stays open until a product expert reads the vendor doc.
