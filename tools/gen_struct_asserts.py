#!/usr/bin/env python3
"""
Generate compile-time struct layout assertions from the offset comments
already carried in includes/metalc_*.h.

Every control-block struct in the framework documents its field offsets
inline:

    struct jct {
        char           jctid[4];      /* +0   Control block ID 'JCT ' */
        uint16_t       jctjobid;      /* +4   JES2 job ID             */
    };                                /* Total: 64 bytes              */

Those comments are the conversion's record of the underlying DSECT, but
nothing checks that the C struct actually lays out that way.  A wrong
type, a missing #pragma pack(1), or an undocumented gap silently shifts
every field after it -- and an exit then reads the wrong bytes out of a
live control block.

This script turns the comments into VERIFY_OFFSET/VERIFY_SIZE assertions
that fail the compile when the struct and its documentation disagree.

    python tools/gen_struct_asserts.py --check     # CI: is the file current?
    python tools/gen_struct_asserts.py --write     # regenerate it

Note what this does and does not prove.  It proves the C struct matches
what the header says.  It does NOT prove the header matches the real
z/OS DSECT -- only assembling the product macro on the target system
does that.  See docs/system-services-catalog.md and the "verify offsets
against your installation's macros" warning in each product header.
"""

import argparse
import re
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
INCLUDES = REPO / "includes"
OUT = REPO / "tests" / "verify_structs.c"
BASELINE = Path(__file__).resolve().parent / "layout_known_issues.txt"


def known_issues():
    """Structs with a tracked, unresolved layout mismatch.

    Shared with tools/check_layout.py so the two tools never disagree.
    Their assertions are emitted commented out; when the underlying
    issue is fixed, check_layout reports it and regenerating re-enables
    them.
    """
    if not BASELINE.exists():
        return set()
    out = set()
    for ln in BASELINE.read_text(encoding="utf-8").splitlines():
        ln = ln.strip()
        if ln and not ln.startswith("#") and ":" in ln:
            out.add(ln.split(":")[1])
    return out

# Headers with no control blocks of their own.
SKIP_HEADERS = {"metalc_verify.h"}

STRUCT_START = re.compile(r"^\s*struct\s+([A-Za-z_][A-Za-z0-9_]*)\s*\{")
STRUCT_END = re.compile(r"^\s*\}\s*;(.*)$")

# uint16_t  name;  /* +4 ... */      |  void *name;  /* +8 ... */
# char      name[8];  /* +12 ... */  |  const char *name;  /* +0 ... */
FIELD = re.compile(
    r"^\s*(?:const\s+)?(?:struct\s+)?[A-Za-z_][A-Za-z0-9_]*\s+"
    r"\*?\s*([A-Za-z_][A-Za-z0-9_]*)\s*(?:\[[^\]]*\])?\s*;"
    r"\s*/\*\s*\+(\d+)"
)

SIZE_TOTAL = re.compile(r"/\*\s*Total:\s*(\d+)\s*bytes")
SIZE_MIN = re.compile(r"/\*\s*(?:Minimum|At least):\s*(\d+)\s*bytes")

# A macro expands to several fields, so offsets after it cannot be
# derived from the visible declarations.
MACRO_FIELD = re.compile(r"^\s*EXIT_PARM_HEADER\s*;")

# A comment marking omitted fields: the C struct is truncated there, so
# every offset after it belongs to the real control block and not to
# this struct.  Stop emitting at that point.
GAP_COMMENT = re.compile(r"^\s*/\*.*(?:omitted|\.\.\.).*\*/\s*$")


def parse_header(path):
    """Yield (struct_name, [(field, offset)], exact_size_or_None)."""
    lines = path.read_text(encoding="utf-8", errors="replace").splitlines()
    i = 0
    while i < len(lines):
        m = STRUCT_START.match(lines[i])
        if not m:
            i += 1
            continue
        name = m.group(1)
        fields, opaque, gapped = [], False, False
        i += 1
        while i < len(lines):
            end = STRUCT_END.match(lines[i])
            if end:
                size = None
                tail = end.group(1)
                # the size comment may sit on the closing line or just after
                probe = tail + (lines[i + 1] if i + 1 < len(lines) else "")
                sm = SIZE_TOTAL.search(probe)
                if sm and not gapped:
                    size = int(sm.group(1))
                elif SIZE_MIN.search(probe):
                    size = None  # a floor, not an exact size
                yield name, ([] if opaque else fields), size
                break
            if GAP_COMMENT.match(lines[i]):
                gapped = True
            if MACRO_FIELD.match(lines[i]):
                opaque = True
            fm = FIELD.match(lines[i])
            if fm and not gapped:
                fields.append((fm.group(1), int(fm.group(2))))
            i += 1
        i += 1


def collect():
    out = []
    for hdr in sorted(INCLUDES.glob("metalc_*.h")):
        if hdr.name in SKIP_HEADERS:
            continue
        items = [x for x in parse_header(hdr) if x[1] or x[2]]
        if items:
            out.append((hdr.name, items))
    return out


def render(data):
    L = []
    a = L.append
    a("/*********************************************************************")
    a(" * verify_structs.c - Compile-time struct layout verification")
    a(" *")
    a(" * GENERATED FILE - do not edit by hand.")
    a(" *   Regenerate:  python tools/gen_struct_asserts.py --write")
    a(" *   Check:       python tools/gen_struct_asserts.py --check")
    a(" *")
    a(" * Assertions are derived from the +N offset comments and the")
    a(" * Total-bytes markers in includes/metalc_*.h.  A compile")
    a(" * error here means a struct does not lay out the way its own header")
    a(" * documents - usually a wrong field type, a missing #pragma pack(1),")
    a(" * or an offset comment that was not updated with the field.")
    a(" *")
    a(" * This file has no executable body - a clean compile is the pass.")
    a(" *")
    a(" * On z/OS:")
    a(" *   xlc -qmetal -S -qlist -I../includes verify_structs.c")
    a(" * Off-platform (structure only, no assembler):")
    a(" *   tools/lint_host.sh")
    a(" *")
    a(" * NOTE: this proves the C matches the header. It does NOT prove the")
    a(" * header matches the real z/OS DSECT - only assembling the product")
    a(" * macro on the target system does that.")
    a(" *********************************************************************/")
    a("")
    a('#include "metalc_base.h"')
    for hdr, _ in data:
        if hdr != "metalc_base.h":
            a(f'#include "{hdr}"')
    a('#include "metalc_verify.h"')
    a("")
    known = known_issues()
    total_o = total_s = total_k = 0
    for hdr, items in data:
        a("/*===================================================================")
        a(f" * {hdr}")
        a(" *===================================================================*/")
        a("")
        for name, fields, size in items:
            skip = name in known
            a(f"/* struct {name} */")
            if skip:
                a(f"/* KNOWN ISSUE - assertions disabled for struct {name}.")
                a( "   Tracked in tools/layout_known_issues.txt; see")
                a( "   docs/layout-findings.md. Re-enable by regenerating")
                a( "   once the mismatch is resolved. */")
            pre = "/* " if skip else ""
            post = " */" if skip else ""
            if size is not None:
                a(f"{pre}VERIFY_SIZE({name}, {size});{post}")
                total_s += 0 if skip else 1
            for fld, off in fields:
                a(f"{pre}VERIFY_OFFSET({name}, {fld}, {off});{post}")
                total_o += 0 if skip else 1
            if skip:
                total_k += 1
            a("")
    a("/* Generated: "
      f"{total_s} size assertions, {total_o} offset assertions "
      f"across {sum(len(i) for _, i in data)} structs */")
    return "\n".join(L) + "\n", total_s, total_o


def main():
    ap = argparse.ArgumentParser()
    g = ap.add_mutually_exclusive_group(required=True)
    g.add_argument("--write", action="store_true")
    g.add_argument("--check", action="store_true")
    args = ap.parse_args()

    data = collect()
    text, ns, no = render(data)
    nstructs = sum(len(i) for _, i in data)

    if args.write:
        OUT.write_text(text, encoding="utf-8")
        print(f"wrote {OUT.relative_to(REPO)}: "
              f"{nstructs} structs, {ns} size + {no} offset assertions")
        return 0

    if not OUT.exists():
        print(f"MISSING: {OUT.relative_to(REPO)} -- run with --write",
              file=sys.stderr)
        return 1
    if OUT.read_text(encoding="utf-8") != text:
        print(f"STALE: {OUT.relative_to(REPO)} does not match the headers.\n"
              f"       Run: python tools/gen_struct_asserts.py --write",
              file=sys.stderr)
        return 1
    print(f"up to date: {nstructs} structs, {ns} size + {no} offset assertions")
    return 0


if __name__ == "__main__":
    sys.exit(main())
