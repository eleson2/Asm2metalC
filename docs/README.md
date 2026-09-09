# Documentation Index

Guidance for converting IBM z/OS HLASM exits to IBM Metal C.

`CLAUDE.md` in the repository root is the entry point for the project as a
whole. This index maps the `docs/` tree.

---

## Start here

| If you are… | Read |
|---|---|
| Converting a module for the first time | §1 Core rules, in order |
| Converting a module for a product | §1, then that product's guide in §3 |
| Stuck on a macro or instruction | §2 Reference |
| Reviewing a finished conversion | §4 Process, plus `verification-matrices/` |
| Adding a product the framework does not cover | `product-onboarder` agent, then §3 |

---

## 1. Core rules — read in this order

| Document | Covers |
|---|---|
| [`ai-conversion-steering.md`](ai-conversion-steering.md) | **Authoritative.** Headers, `EXIT_PARM_HEADER`, return codes, bit macros, field comparison, system services. Overrides every other guide where they conflict. |
| [`asm-to-metalc-general.md`](asm-to-metalc-general.md) | Entry points, register mapping, data types, control flow, what not to translate |
| [`asm-to-c-conversion-guide.md`](asm-to-c-conversion-guide.md) | DSECT→struct mapping, macro expansion, condition codes, common mistakes |
| [`asm-linkage-conventions.md`](asm-linkage-conventions.md) | BAKR/PR vs SAVE/RETURN vs JES2 `$SAVE`; the right `#pragma prolog/epilog` for each |
| [`system-services-catalog.md`](system-services-catalog.md) | Every system-service macro → C call, and how to add a wrapper or stub for one that is missing |
| [`exit-chaining.md`](exit-chaining.md) | Chain-safe RC initialisation; the neutral RC per product |

## 2. Reference — consult as needed

| Document | Covers |
|---|---|
| [`complex-asm-patterns.md`](complex-asm-patterns.md) | `EX`, `TRT`, `ICM`, `MVCL`, `BCT`, `BAS`, packed decimal, `STCK` |
| [`hlasm-structured-programming.md`](hlasm-structured-programming.md) | `IF`/`ELSE`/`ENDIF`, `DO`/`ENDDO`, `SELECT`/`WHEN` macros and their C equivalents |
| [`racroute-metalc-patterns.md`](racroute-metalc-patterns.md) | SAF/RACROUTE for products that *call* RACF; three-way RC and default-deny |
| [`copy-macro-dependency.md`](copy-macro-dependency.md) | Resolving `COPY` members and macro library dependencies |
| [`amode64-exits.md`](amode64-exits.md) | AMODE 64 detection, `-q64`, pointer widths, affected products |
| [`reentrant-ification-policy.md`](reentrant-ification-policy.md) | Converting non-reentrant ASM (static `DS` fields) to reentrant Metal C |
| [`native-c-exit-development.md`](native-c-exit-development.md) | Writing a new Metal C exit rather than converting one |

## 3. Product guides

Each supplements the core rules with that product's control blocks, return
code conventions, calling convention, and worked patterns.

| Product | Guide | Header |
|---|---|---|
| CA ACF2 | [`asm-to-metalc-acf2.md`](asm-to-metalc-acf2.md) | `metalc_acf2.h` |
| CICS TS | [`asm-to-metalc-cics.md`](asm-to-metalc-cics.md) | `metalc_cics.h` |
| DB2 | [`asm-to-metalc-db2.md`](asm-to-metalc-db2.md) | `metalc_db2.h` |
| DFSMS | [`asm-to-metalc-dfsms.md`](asm-to-metalc-dfsms.md) | `metalc_dfsms.h` |
| IMS | [`asm-to-metalc-ims.md`](asm-to-metalc-ims.md) | `metalc_ims.h` |
| JES2 | [`asm-to-metalc-jes2.md`](asm-to-metalc-jes2.md) | `metalc_jes2.h` |
| IBM MQ | [`asm-to-metalc-mq.md`](asm-to-metalc-mq.md) | `metalc_mq.h` |
| NetView | [`asm-to-metalc-netview.md`](asm-to-metalc-netview.md) | `metalc_netview.h` |
| OPC/TWS | [`asm-to-metalc-opc.md`](asm-to-metalc-opc.md) | `metalc_opc.h` |
| RACF | [`asm-to-metalc-racf.md`](asm-to-metalc-racf.md) | `metalc_racf.h` |
| System Automation | [`asm-to-metalc-sa.md`](asm-to-metalc-sa.md) | `metalc_sa.h` |
| SMF | [`asm-to-metalc-smf.md`](asm-to-metalc-smf.md) | `metalc_smf.h` |
| TCP/IP | [`asm-to-metalc-tcpip.md`](asm-to-metalc-tcpip.md) | `metalc_tcpip.h` |
| VTAM / SNA | [`asm-to-metalc-vtam.md`](asm-to-metalc-vtam.md) | `metalc_vtam.h` |

## 4. Process and policy

| Document | Covers |
|---|---|
| [`pre-conversion-triage.md`](pre-conversion-triage.md) | Assessment template to complete **before** any conversion |
| [`partial-scope-policy.md`](partial-scope-policy.md) | When a partial conversion is allowed, what to document, deployment gates |
| [`verification-matrices/README.md`](verification-matrices/README.md) | Matrix index, status values, production deployment blocks |

---

## The framework in one page

```
asm/<PRODUCT>/<MODULE>.asm          source exit
        │
        ├─ triage ──────────────►   pre-conversion-triage.md
        │                           (asm-pre-analyzer agent)
        ├─ convert ─────────────►   converted/<PRODUCT>/<MODULE>.c
        │                           (asm-to-metalc-converter agent)
        └─ verify ──────────────►   docs/verification-matrices/<MODULE>_<product>.md
                                    (metalc-verifier agent, then a human reviewer)

includes/metalc_base.h              types, bit macros, RC constants, field utilities
        └─ metalc_svc.h             system services — the ONLY inline assembler
includes/metalc_<product>.h         control blocks and RC constants per product
includes/metalc_saf.h               SAF/RACROUTE, for products that call RACF
asm/stubs/                          HLASM stubs, link-edited with the exit
```

## Rules that hold everywhere

These come from `CLAUDE.md` and `ai-conversion-steering.md`. A guide that
appears to contradict one of them is wrong — follow these.

1. Fixed-width types only (`int32_t`, `uint16_t`); never raw `int`/`long`.
2. `#pragma pack(1)` on every control block struct, with offset comments.
3. Named return-code constants; never a literal integer.
4. `#pragma prolog`/`epilog` matching the ASM's actual linkage family.
5. No static writable data — reentrant exits only. Static `const` is fine.
6. No `__asm` in a converted exit. Services come from `metalc_svc.h` or an
   `asm/stubs/` stub.
7. A security decision defaults to deny. Only the explicit "authorized"
   code allows.
8. Document every divergence from the ASM in the module header and the
   verification matrix.
