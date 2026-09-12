# Building on z/OS

Everything in this repository is verified off-platform (`make check`) and
**none of it has ever been compiled**. There is no z/OS system here. This
directory holds what a systems programmer needs to run the first real
build, and what to look at when it fails.

---

## What the offline checks do and do not prove

`make check` runs on any machine with Python and a C compiler.

| Verified offline | Only a z/OS build shows |
|---|---|
| Struct offsets match their header comments | Headers match the real DSECTs |
| Headers parse; no type or macro collisions | `xlc -qmetal` accepts the pragmas |
| Layout assertions hold at 4-byte pointers | Inline assembler is correct |
| Converted exits obey the CLAUDE.md rules | Service macros expand |
| No inline assembler outside the service layer | Prolog/epilog match the ASM linkage |

A clean `make check` means the code is structurally sound. It says
nothing about whether it runs.

---

## First build — do this one first

Pick **`IEFU83`** (SMF record filter) as the pathfinder:

- simplest converted exit in the repository
- no SAF dependency, so no stub to assemble
- SMF exits are straightforward to test and low blast radius
- it exercises the whole toolchain: compile, assemble, bind

`ASMCBLD.jcl` builds one exit. Expect the first attempt to fail — the
point is to find out how, on a real compiler, with a real assembler.

### Datasets to create

| Dataset | Contents | From |
|---|---|---|
| `&HLQ..INCLUDE` | all `metalc_*.h` | `includes/` |
| `&HLQ..CONVERTD` | the exit source | `converted/<PRODUCT>/` |
| `&HLQ..ASMSTUBS` | HLASM stubs | `asm/stubs/` |
| `&HLQ..TESTS` | `verify_structs.c` | `tests/` |

Upload as text so ASCII→EBCDIC translation happens; the framework is
EBCDIC-sensitive throughout (see the EBCDIC helpers in `metalc_base.h`).

---

## Order of work

**1. Compile `verify_structs.c` first, before any exit.**

It is compile-only, has no executable body, and asserts 831 field
offsets and 77 struct sizes. If it compiles clean, every struct in the
framework lays out the way its header claims — under the real compiler,
with real pointer widths. If it fails, the error names the exact struct
and field:

```
error: size of array '_chk_jct_jctjclas' is negative
```

That is a far cheaper way to find layout problems than debugging an exit
that reads the wrong field of a live control block.

```
xlc -qmetal -S -qlist -I//'&HLQ..INCLUDE' //'&HLQ..TESTS(VERIFYST)'
```

**2. Build `IEFU83`** with `ASMCBLD.jcl`.

**3. Read the listings.** A successful bind is not the same as a correct
exit. The checklist at the bottom of `ASMCBLD.jcl` says what to look at:
the generated prolog/epilog, macro expansion, unresolved externals,
module attributes.

**4. Assemble `SAFAUTH.asm`.** It blocks every SAF-calling exit and its
`RACROUTE` operand forms are the least verified part of the framework.
Its header lists the three things to confirm against the RACF Macro
Reference for your release.

---

## Expect these to bite

Ranked by how likely they are to be wrong, given none of it has been
compiled:

**Inline assembler constraints.** `includes/metalc_svc.h` is the only
file with `__asm`. The `"m"` operand handling, the clobber lists, and
the register conventions are written to IBM's documented behaviour but
have never been through the compiler. Three bugs of exactly this kind
were already found by inspection (`LA` where `L` was meant, a missing
`SR 0,0` before `SVC 35`, missing `volatile`). Assume more.

**`SYS1.MACLIB` in the assemble step.** `WTO`, `GETMAIN`, `FREEMAIN` and
`STORAGE` are macros that expand at assembly, not compile. Without the
macro library in `SYSLIB` every service wrapper fails.

**The 64-bit macro.** `metalc_base.h` sets `METALC_64` from
`__LP64__ || __64BIT__`. If your compiler defines neither under `-q64`,
every width silently stays 31-bit and the struct offsets are wrong with
no diagnostic. Settle it before any `-q64` build:

```c
#if !defined(__LP64__) && !defined(__64BIT__)
#error "-q64 build but no 64-bit macro defined; adjust METALC_64"
#endif
```

**`#pragma prolog` / `epilog` syntax.** The strings are written per
`docs/asm-linkage-conventions.md`. Verify the generated prolog in the
`-qlist` listing actually matches the ASM module's linkage — especially
for `BAKR`/`PR` exits, where getting it wrong abends rather than
returning.

**The WTO length field.** `wto_write()` adds 4 to the length halfword
when routing and descriptor codes are present. IBM documents the field
as text length + 4 with the codes *not* counted. Flagged in the function
comment; confirm against the macro expansion.

---

## Feeding results back

When the first build finds something:

1. Fix it in the header or the exit.
2. Re-run `make check` off-platform — the layout and conformance tools
   catch the same class of problem for free from then on.
3. If it was a struct offset, correct the header comment too. The
   offset comments are the input to `tools/gen_struct_asserts.py`, so a
   corrected comment becomes a permanent assertion.
4. If a whole class of problem is mechanically detectable, add a rule to
   `tools/check_conformance.py` so it cannot recur.

That loop is the point of the offline harness: every mainframe finding
should become an offline check, so the next 50 exits do not repeat it.

---

## Related

- `docs/layout-findings.md` — known layout issues and what they mean
- `docs/system-services-catalog.md` — which services exist and which need building
- `docs/amode64-exits.md` — `-q64` rules and affected products
- `docs/verification-matrices/README.md` — per-exit sign-off status
