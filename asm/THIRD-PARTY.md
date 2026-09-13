# Third-Party Assembler Sources

Everything else under `asm/` is this project's own sample material. The
sources listed here were **fetched unmodified from public repositories**
and are included as large, real-world conversion challenges. Their
licences apply to them, not to the rest of this repository.

Nothing here has been edited. If you need to re-fetch or diff against
upstream, the commit each file came from is recorded below.

---

## 1. `asm/DB2/IRR@XACS.asm` — RACF/DB2 External Security Module

| | |
|---|---|
| Upstream | https://github.com/IBM/zopeneditor-sample |
| Path | `ASM/IRR@XACS.asm` |
| Commit | `8f9835308de6` (2026-09-11) |
| Licence | Apache-2.0 (`LICENSE` at repo root) |
| Copyright | `5647-A01 (C) COPYRIGHT IBM CORP. 1997, 2000` (in-file) |
| Size | 6,432 lines / 353 KB |
| Service level | `OA12836` (in-file `&SERVICELEVEL`) |

IBM's shipped `SYS1.SAMPLIB` module that implements the DB2 **access
control authorization exit**, installed as `DSNX@XAC`. Real production
code carrying APAR change flags (`@01C`, `@08A`, `@L3A`, `@09A`), not a
teaching sample.

Placed under `asm/DB2/` because the *exit point* is DB2's, even though
the module name is RACF's (`IRR` prefix). It spans both products.

Triage: [`docs/triage/IRR@XACS_db2.md`](../docs/triage/IRR@XACS_db2.md)

## 2. `asm/challenges/mxe/` — z/OS synchronous cross-memory server

| | |
|---|---|
| Upstream | https://github.com/rscott-rocket/mxe |
| Commit | `0e0d2eec0ab8` (2020-03-03) |
| Licence | Apache-2.0 (`asm/challenges/mxe/LICENSE`) |
| Size | 12 `.asm` (3,071 lines) + 19 `.mac` + 8 samplib members |

A PC-based cross-memory server: System LX reservation, entry table
build, space-switch PC routine, SRB scheduling, and an end-of-task
resource manager. Upstream describes it as for test systems only.

This one is here as a **negative** example. Cross-memory PC/PT is on
CLAUDE.md's do-not-convert list, so the correct outcome is a BLOCKED
triage — it exists to prove the pipeline refuses rather than emitting a
placeholder that links.

Triage: [`docs/triage/mxe_xmem.md`](../docs/triage/mxe_xmem.md)

---

## Why these are not in `converted/`

Neither has been converted, and `tools/check_conformance.py` only scans
`converted/`, so adding them changes no check. `asm/challenges/` is
outside the per-product exit tree on purpose: the material there is not
a z/OS exit and should not be mistaken for one.
