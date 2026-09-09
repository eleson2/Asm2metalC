# Verification Matrices — Overview

This directory contains per-exit logic equivalence matrices for the
assembler-to-Metal-C conversions in `converted/`.

## Purpose

Each matrix provides a structured audit trail that answers: *does the
converted C code produce the same outcomes as the original assembler
for every reachable input combination?*

The matrices are not auto-generated. They are maintained by a human
reviewer who cross-checks the assembler listing against the C source
and documents every point of correspondence or deliberate divergence.

## Matrix Files

| File | Exit | Product | Status |
|------|------|---------|--------|
| [ACF2PWX_acf2.md](ACF2PWX_acf2.md) | ACF2PWX | ACF2 | Draft |
| [DFHPEP_cics.md](DFHPEP_cics.md) | DFHPEP | CICS | Draft |
| [DSN3ATH_db2.md](DSN3ATH_db2.md) | DSN3ATH | DB2 | Draft |
| [IEFDB401_dfsms.md](IEFDB401_dfsms.md) | IEFDB401 | DFSMS | Draft |
| [DFSMSCE0_ims.md](DFSMSCE0_ims.md) | DFSMSCE0 | IMS | Draft |
| [DFSWHU00_ims.md](DFSWHU00_ims.md) | DFSWHU00 | IMS | Draft |
| [IEFU83_smf.md](IEFU83_smf.md) | IEFU83 | SMF | Draft |
| [ICHPWX01_racf.md](ICHPWX01_racf.md) | ICHPWX01 | RACF | Draft |
| [HASPEX02_jes2.md](HASPEX02_jes2.md) | EXIT02/HASPEX02 | JES2 | Draft |
| [HASPEX20_jes2.md](HASPEX20_jes2.md) | EXIT20/HASPEX20 | JES2 | Draft |
| [CSQXLIB_mq.md](CSQXLIB_mq.md) | CSQXLIB | MQ | Draft |
| [DSIEX01_netview.md](DSIEX01_netview.md) | DSIEX01 | NetView | Draft |
| [EQQUX007_opc.md](EQQUX007_opc.md) | EQQUX007 | OPC/TWS | Draft |
| [AOFEXC02_sa.md](AOFEXC02_sa.md) | AOFEXC02 | SA z/OS | Draft |
| [FTCHKCMD_tcpip.md](FTCHKCMD_tcpip.md) | FTCHKCMD | TCP/IP | Draft |
| [ISTEXCLY_vtam.md](ISTEXCLY_vtam.md) | ISTEXCLY | VTAM | Draft |

### Production Deployment Blocks

The following matrices have HIGH severity concerns that must be resolved
before the converted exit can replace the ASM module in production:

| Exit | Concern | Description |
|------|---------|-------------|
| HASPEX20 | C1 | JCTJOBID type mismatch may cause incorrect job-type detection |
| EQQUX007 | C2 | Critical path flag offset `TM 71(R3)` may not match struct field |

### Blocked on Struct Layout Resolution

HIGH severity: the exit's parameter block uses `EXIT_PARM_HEADER`, whose
documented field offsets are 2 bytes off the layout the compiler
produces.  Detected by `make layout`; see `docs/layout-findings.md`
finding 4.  Resolving it needs the vendor documentation for that exit's
parameter list.

| Exit | Concern | Struct |
|------|---------|--------|
| DSN3ATH | CX | `db2_ath_parm` |
| FTCHKCMD | CX | `tcpsec_parm`, `ipflt_parm` |

Seven further structs are affected but have no converted exit yet:
`db2_xac_parm`, `db2_sgn_parm`, `db2_edit_parm`, `db2_field_parm`,
`ims_flgx_parm`, `sa_rec_parm`.

### Blocked on Assembler Stub Validation

MEDIUM severity: the exit calls a stub in `asm/stubs/` that has never been
assembled, because there is no z/OS system in this repository.  The stub
header lists what a systems programmer must confirm before it is built.

| Exit | Concern | Stub | Description |
|------|---------|------|-------------|
| DFSWHU00 | C2 | `SAFAUTH.asm` | RACROUTE macro operands and `RELEASE=` level need review against the installed RACF level; stub must assemble clean and be link-edited with the exit |

> DFSWHU00's earlier HIGH concern — an inline `XR 15,15` that allowed
> every sign-on regardless of RACF — is resolved.  The exit now calls
> `saf_auth_appl()` and denies on SAF RC=4 (no decision) and RC=8 (denied).

## How to Read a Matrix

Each matrix has six sections:

1. **Module Identity** — names, sources, products, verifier.
2. **Entry Conditions** — register-to-variable mapping at function entry.
3. **Logic Equivalence Table** — row-per-significant-ASM-label with:
   - ASM label, ASM instruction(s), C equivalent, Verified flag, Concern notes.
4. **Return Code Path Inventory** — every exit path with expected RC and match status.
5. **Flagged Concerns** — numbered list of issues requiring follow-up.
6. **Sign-off Checklist** — reviewer checklist before marking a matrix "Verified".

## Verification Lifecycle

```
Draft  →  In Review  →  Verified
```

A matrix moves to **In Review** when all Logic Equivalence rows and RC
paths are filled in.  It moves to **Verified** when a second reviewer
has confirmed all concerns are either resolved or accepted as
intentional divergences.

## Related Files

- `includes/metalc_verify.h` — compile-time struct layout assertions
- `tests/verify_structs.c`  — compile-only struct size/offset checks
- `tests/test_iefu83.c`     — runtime test harness for IEFU83
