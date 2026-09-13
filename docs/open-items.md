# Open Items and How Each One Closes

Finding a defect is only worth the effort if someone can act on it. This
register exists so no item sits here indefinitely as a well-documented
problem nobody can close.

**Every item must be in class A, B or C.** An item that fits none of
them does not belong in this repository as an open finding — either the
risk is accepted (class C) or the speculative code is deleted.

| Class | Meaning | Rule |
|---|---|---|
| **A** | Closable here, no external input | Should be scheduled, not tracked. If it lingers, it was really class C |
| **B** | Blocked on **one named input** | The input must be named exactly — which macro, which manual, which library. "Needs more research" is not class B |
| **C** | Accepted, will not be fixed | State the consequence. A class C item is closed, not open |

---

## Class A — closable here

| Item | Where | Effort | If never done |
|---|---|---|---|
| AMODE 64 pointer guards | every `metalc_*.h` | Medium, mechanical | No exit can be built `-q64` with correct layouts. Blocks IMS 15+, MQ 9.3+, WLM. `layout-findings.md` finding 3 |
| `RACROUTE REQUEST=FASTAUTH` stub | `asm/stubs/SAFFAST.asm` | Small | Performance-path SAF checks unavailable |
| `RACROUTE REQUEST=STAT` stub | `asm/stubs/SAFSTAT.asm` | Small | Cannot test whether RACF is active |
| `RACROUTE REQUEST=VERIFY` stub | `asm/stubs/SAFVERIF.asm` | Small | No ACEE create/delete |
| `RACROUTE REQUEST=LIST` stub | `asm/stubs/SAFLIST.asm` | Small | No in-storage profile management |
| `CPOOL`, `MODESET`, `SETFRR` wrappers/stubs | `metalc_svc.h` / `asm/stubs/` | Small each | Authorized and recovery code cannot be expressed |

The four SAF stubs are worth doing regardless of `IRR@XACS`:
`metalc_saf.h` currently covers `REQUEST=AUTH` alone, which is one of at
least five request types real exits use. Follow
`docs/racroute-metalc-patterns.md`; **Strategy A (inline SVC 119) is
banned.**

### Closed recently

| Item | How it closed |
|---|---|
| 117 layout baseline mismatches | Findings 4 and 5 applied; baseline is now **empty** |
| `ascb` struct, last 3 mismatches | It is a truncated mapping and was missing the `/* ... fields omitted ... */` markers the `cvt` struct already used |
| `format_int()` wrote past `width` | Bounded; fills with `'*'` on overflow. Also fixed UB negating `INT32_MIN` |
| 5 of 17 examples did not compile | `examples/` was never in the lint build. Now it is, and all 17 build |
| 3 function-code collisions | `tools/check_func_codes.py`, wired into `make funcs` |
| `UNPK` "needs a wrapper" | It never did — `format_int()`/`format_hex()` already covered it. `complex-asm-patterns.md` §7.1 |

---

## Class B — blocked on one named input

Each row names the exact artefact needed. Obtain it and the item closes;
until then the affected code must not be trusted.

| Item | Needs | Where to get it |
|---|---|---|
| `db2_xac_parm` has the wrong **shape** | `DSNDXAPL` + `DSNDEXPL` | DB2 `SDSNMACS`, or *DB2 Administration Guide* → "Access control authorization exit" |
| `IRR@XACS` conversion | same two macros | as above. `docs/triage/IRR@XACS_db2.md` |
| `jct` fields after `jctjname` | `$JCT` macro | Your JES2 level's macro library |
| `db2_sgn_parm`, `db2_edit_parm`, `db2_field_parm` | the DSECT for each exit | DB2 macro library |
| `sa_rec_parm` | AOFEXC30 parameter mapping | System Automation exit documentation |
| `ipflt_parm` | `EZBIPMXT` mapping | Comms Server exit documentation |
| `tcpsec_parm` | `EZACSEC`/`EZASSEC` mapping | Comms Server exit documentation |
| `ims_flgx_parm` | `DFSFLGX0` parameter mapping | IMS exit documentation |
| `SA_FUNC_TERM` value | SA function code list | System Automation exit documentation |
| `LY_FUNC_INIT` / `LY_FUNC_TERM` values | VTAM function code list | VTAM exit documentation |

> **The eight struct rows share one property: no code in this repository
> uses them.** They are speculative scaffolding. If a product's DSECT is
> not going to be obtained, the honest disposition is to **delete the
> struct**, not to carry it as a permanent open item — an unused struct
> with unsourced offsets is a trap for the next person, and
> `db2_xac_parm` has already proven it can be wrong in *shape* and not
> merely in offsets.
>
> They are kept for now because they document intent for products this
> framework means to support. **Revisit at the next review**: any struct
> still unsourced and still unused should go.

`SA_FUNC_TERM`, `LY_FUNC_INIT` and `LY_FUNC_TERM` were **removed** rather
than left with colliding values, so referencing one is now a compile
error. That is deliberate: an undefined name fails loudly, a wrong value
does not.

---

## Class C — accepted, will not be fixed

| Item | Consequence accepted |
|---|---|
| `asm/challenges/mxe/` will not be converted | Cross-memory PC/PT and SRB scheduling cannot be expressed in Metal C. Keeping that layer in HLASM is the correct engineering answer, not a deferral. `docs/triage/mxe_xmem.md` |
| `IRR@XACS` FRR routine stays in HLASM | A recovery routine runs in key 0, disabled, on the FRR stack. Metal C is the wrong tool; a stub is the right boundary |
| Offline checks cannot prove z/OS behaviour | Structural only. Inline assembler, macro expansion and real DSECT layouts are verified by a build on the target — `docs/zos-build/README.md` |

---

## Per-exit concerns

Concerns scoped to a single converted exit live in that exit's
verification matrix, not here. See
[`verification-matrices/README.md`](verification-matrices/README.md) for
the ones still gating production deployment.

## Reviewing this register

When an item closes, move it to "Closed recently" with one line on *how*
— the mechanism matters more than the fact. When a class B item has sat
unclosed through two reviews, its input is not actually coming: convert
it to class C, or delete the code it protects.
