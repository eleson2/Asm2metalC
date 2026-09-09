# COPY Members and Macro Dependency Handling

Real-world z/OS exits commonly pull in definitions through `COPY` statements and
product-supplied macro libraries.  This document explains what COPY and macro
dependencies are, how to detect them, and how to handle them during the
HLASM-to-Metal-C conversion.

---

## 1. What Is a COPY Member?

A `COPY membername` statement in HLASM inserts the named member from a library
(usually `SYS1.MACLIB`, `SYS1.MODGEN`, or a site-local library) verbatim at that
point in the source.  It is similar to a C `#include`.

Common uses:
- Pull in common equates (`COPY $HASPGBL` in JES2 — defines hundreds of symbols)
- Pull in DSECT mappings (`COPY DFHCSADS` in CICS)
- Pull in standard saves/work-area layouts

**Effect on conversion**: If the source file contains `COPY` statements, the
file as seen by the converter is **incomplete**.  The converter cannot see the
symbols defined in those members.  This is the COPY dependency problem.

---

## 2. Detection

The asm-pre-analyzer will flag:

```
COPY MEMBER         ← verbatim text in source
```

The member name appears as the operand.  Common members:

| Member | Product | Defines |
|---|---|---|
| `$HASPGBL` | JES2 | All JES2 global equates, DSECT starts |
| `$HASPEQU` | JES2 | JES2 field equates |
| `DFHCSADS` | CICS | CICS CSA DSECT |
| `IHAECB` | z/OS | ECB format |
| `IHAASCB` | z/OS | ASCB format |
| `IHAPSA` | z/OS | PSA format |
| `CVT` | z/OS | Communications Vector Table |
| `IEECUCM` | z/OS | UCM (Unit Control Module) |
| `DSAGETDS` | DFSMS | SMS data set allocation DSECT |

---

## 3. Resolution Strategies

### Strategy A — Use the Expanded Listing

The most reliable approach: compile the ASM source on z/OS with `PRINT GEN` to
get a fully expanded listing.  The `.list` output contains all COPY/macro expansions
inline.

```
//STEP1 EXEC PGM=ASMA90,PARM='NODECK,OBJECT,LIST(133),USING(WARN)'
//SYSLIB DD DSN=SYS1.MACLIB,DISP=SHR
//       DD DSN=SYS1.MODGEN,DISP=SHR
//       DD DSN=site.MACLIB,DISP=SHR
//SYSIN  DD DSN=myexit.asm,DISP=SHR
//SYSPRINT DD SYSOUT=*
```

The `SYSPRINT` output can be fed to the asm-pre-analyzer as the source file.  All
COPY and macro expansions will be visible.

### Strategy B — Obtain the COPY Member

Request the COPY member source from the site:
- `SYS1.MACLIB(membername)` — IBM-supplied
- `SYS1.MODGEN(membername)` — IBM model definitions

For IBM-supplied members (`IHAASCB`, `IHAPSA`, etc.), the definitions are documented
in the z/OS MVS Data Areas manuals and have already been mapped to C structs in
`includes/metalc_base.h` for the most common ones.

### Strategy C — Manual Symbol Resolution

If neither A nor B is possible, document each undefined symbol in the analysis report:

```
## Unresolved COPY Dependencies
| Symbol | COPY member | Likely value | Confidence |
|--------|-------------|--------------|------------|
| $JCBAT | $HASPGBL | bit mask | Low |
| EXIT02W | $HASPGBL | DSECT label | Medium |
```

The converter agent then treats unresolved symbols as `/* UNRESOLVED: $JCBAT */`
placeholders and the conversion scope is marked Partial.

---

## 4. Product Macro Libraries

Beyond COPY, exits use product macros from library concatenations:

| Product | Library | Common macros |
|---|---|---|
| JES2 | `SYS1.HASPSRC` or site-local | `$MODULE`, `$ENTRY`, `$SAVE`, `$RETURN`, `$WTO`, `$QSUSE` |
| IMS | `IMS.SDFSMAC` | `IHB*`, `DFSMSCR`, `DFSMSCE` |
| CICS | `CICSTS.CICS.SDFHMAC` | `DFH*` |
| RACF | `SYS1.MACLIB` | `RACROUTE`, `RACLIST`, `RACINIT` |
| DB2 | `DB2.SDSNMACS` | `DSN*`, `DSNZPARM` |

If the converted source references macro names from these libraries, those macros
expand to register manipulations, SVC calls, or data declarations that must be
manually traced.

---

## 5. Handling JES2 `$HASPGBL` / `$HASPEQU`

JES2 exits almost always begin with:

```asm
         COPY  $HASPGBL
```

This defines hundreds of equates, bit flags, and DSECT starters.  The critical
ones for exit conversion are already documented in `docs/asm-to-metalc-jes2.md`
and reflected in `includes/metalc_jes2.h`.

For any JES2 exit, assume `$HASPGBL` is present and use the equate values from the
JES2 product header rather than trying to resolve the COPY member.

If a JES2 symbol appears in the source that is NOT covered by `metalc_jes2.h`,
add it to the pre-conversion checklist and open a task to add it to the header.

---

## 6. IBM z/OS Data Area COPY Members

IBM ships these COPY members as part of z/OS:

| COPY member | C equivalent | Covered in |
|---|---|---|
| `IHAASCB` | `struct ascb` | `metalc_base.h` (partial) |
| `IHAPSA` | `struct psa` | not yet in framework |
| `CVT` | `struct cvt` | not yet in framework |
| `IHAECB` | `uint32_t ecb` | simple word, no struct needed |
| `IHAASCB` | `struct ascb` | not yet in framework |

For unlisted members, consult the z/OS MVS Data Areas manual (SA23-1352 and related)
and add a minimal struct to `includes/metalc_base.h` or the product header as needed.

---

## 7. Reporting Unresolved Dependencies in the Matrix

When a conversion proceeds with unresolved COPY dependencies:

1. Add a concern in the verification matrix §6:
   ```
   Cx — Unresolved COPY member: $HASPGBL symbol XXXXX
   Assessment: Open — value assumed from JES2 documentation; verify against site MACLIB
   ```
2. Mark the conversion scope as Partial in the matrix §4 if entire sections depend
   on the unresolved member.
3. Add the COPY resolution to the sign-off checklist.

---

## 8. Related Documents

- `docs/asm-to-metalc-jes2.md` — JES2-specific equates and macros
- `docs/partial-scope-policy.md` — how to document partial conversions
- `docs/pre-conversion-triage.md` — checklist includes COPY dependency check
- `includes/metalc_base.h` — common z/OS data area structs
