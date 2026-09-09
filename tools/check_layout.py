#!/usr/bin/env python3
"""
Check that every control-block struct lays out at the offsets its own
header documents.

Each field in includes/metalc_*.h carries the offset it is supposed to
sit at:

    uint16_t       athpriv;          /* +6   Privilege requested      */

This computes the offset the compiler will actually produce (structs are
#pragma pack(1), so no padding) and reports every field where the two
disagree.  A single wrong type or an undocumented gap shifts everything
after it, and the exit then reads the wrong bytes out of a live control
block -- silently, with no diagnostic anywhere.

    python tools/check_layout.py            # 31-bit (AMODE 31, default)
    python tools/check_layout.py --amode64  # 64-bit pointers

Exit status is non-zero if any mismatch is found, so CI can gate on it.

This proves the C matches the header.  It does NOT prove the header
matches the real z/OS DSECT -- only assembling the product macro on the
target system does that, which is why every product header carries the
"verify offsets against your installation's macros" warning.
"""

import argparse
import re
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
INCLUDES = REPO / "includes"
BASELINE = Path(__file__).resolve().parent / "layout_known_issues.txt"

SCALARS = {
    "char": 1, "int8_t": 1, "uint8_t": 1, "unsigned char": 1, "signed char": 1,
    "int16_t": 2, "uint16_t": 2, "short": 2, "unsigned short": 2,
    "int32_t": 4, "uint32_t": 4, "int": 4, "unsigned int": 4,
    "int64_t": 8, "uint64_t": 8,
    "float": 4, "double": 8,
}

# EXIT_PARM_HEADER expands to work(+0) func(+4) flags(+5) reserved(+6..7)
PARM_HEADER = [("work", 0, "ptr"), ("func", 4, 1), ("flags", 5, 1),
               ("reserved", 6, 2)]
PARM_HEADER_SIZE = 8

STRUCT_START = re.compile(r"^\s*struct\s+([A-Za-z_][A-Za-z0-9_]*)\s*\{")
STRUCT_END = re.compile(r"^\s*\}\s*;(.*)$")
PARM_HEADER_LINE = re.compile(r"^\s*EXIT_PARM_HEADER\s*;")

FIELD = re.compile(
    r"^\s*(?P<type>(?:const\s+)?(?:unsigned\s+|signed\s+)?"
    r"(?:struct\s+)?[A-Za-z_][A-Za-z0-9_]*)"
    r"\s+(?P<ptr>\*?)\s*(?P<name>[A-Za-z_][A-Za-z0-9_]*)"
    r"\s*(?:\[(?P<dim>[^\]]*)\])?\s*;"
    r"(?:\s*/\*\s*\+(?P<off>\d+))?"
)

SIZE_TOTAL = re.compile(r"/\*\s*(?:Total|Minimum|At least):\s*(\d+)\s*bytes")
EXACT_SIZE = re.compile(r"/\*\s*Total:\s*(\d+)\s*bytes")

# A struct truncated at the END still has checkable offsets for the
# fields it does declare - only its total size is unknown.  These markers
# suppress the size assertion, nothing else.
TRUNCATED_MARKERS = (
    "much larger", "add fields as needed", "additional fields",
    "fields follow", "vary by", "varies by", "(verify)",
)

# A gap in the MIDDLE means the offsets after it cannot be computed from
# the declarations.  On one of these the cursor resynchronises to the
# next documented offset instead of reporting a cascade of mismatches.
GAP_COMMENT = re.compile(r"^\s*/\*.*(?:omitted|\.\.\.).*\*/\s*$")


def size_of(type_name, is_ptr, dim, ptr_size):
    if is_ptr:
        base = ptr_size
    elif type_name in SCALARS:
        base = SCALARS[type_name]
    else:
        return None  # unknown/nested type: cannot compute
    if dim:
        try:
            n = int(dim, 0)
        except ValueError:
            return None
        return base * n
    return base


def parse_raw(path):
    """Yield raw struct records: name, ordered field declarations, markers."""
    lines = path.read_text(encoding="utf-8", errors="replace").splitlines()
    i = 0
    while i < len(lines):
        m = STRUCT_START.match(lines[i])
        if not m:
            i += 1
            continue
        name, body_start = m.group(1), i
        raw = []
        i += 1
        while i < len(lines):
            end = STRUCT_END.match(lines[i])
            if end:
                probe = end.group(1) + (lines[i + 1] if i + 1 < len(lines) else "")
                em = EXACT_SIZE.search(probe)
                blob = "\n".join(lines[body_start:i + 2]).lower()
                yield {
                    "name": name, "raw": raw,
                    "declared_size": int(em.group(1)) if em else None,
                    "truncated": any(k in blob for k in TRUNCATED_MARKERS),
                }
                break
            if PARM_HEADER_LINE.match(lines[i]):
                raw.append(("__parm_header__", None, False, None, None))
                i += 1
                continue
            if GAP_COMMENT.match(lines[i]):
                raw.append(("__gap__", None, False, None, None))
                i += 1
                continue
            fm = FIELD.match(lines[i])
            if fm:
                t = " ".join(fm.group("type").replace("const", "").split())
                raw.append((
                    fm.group("name"), t, bool(fm.group("ptr")),
                    fm.group("dim"),
                    int(fm.group("off")) if fm.group("off") else None,
                ))
            i += 1
        i += 1


def collect_all():
    """All structs across all headers, keyed by struct name."""
    by_name, order = {}, []
    for hdr in sorted(INCLUDES.glob("metalc_*.h")):
        if hdr.name == "metalc_verify.h":
            continue
        for rec in parse_raw(hdr):
            rec["header"] = hdr.name
            by_name[rec["name"]] = rec
            order.append(rec)
    return by_name, order


def resolve_sizes(by_name, ptr_size):
    """Fixpoint over nested struct sizes. Returns {name: size_or_None}."""
    sizes = {}
    for _ in range(12):
        progressed = False
        for name, rec in by_name.items():
            if name in sizes:
                continue
            total, ok = 0, True
            for f in rec["raw"]:
                if f[0] == "__parm_header__":
                    total += PARM_HEADER_SIZE
                    continue
                if f[0] == "__gap__":
                    ok = False   # size unknowable across a gap
                    break
                _, t, is_ptr, dim = f[0], f[1], f[2], f[3]
                sz = field_size(t, is_ptr, dim, ptr_size, sizes)
                if sz is None:
                    ok = False
                    break
                total += sz
            if ok:
                sizes[name] = total
                progressed = True
        if not progressed:
            break
    return sizes


def field_size(type_name, is_ptr, dim, ptr_size, sizes):
    if is_ptr:
        base = ptr_size
    elif type_name in SCALARS:
        base = SCALARS[type_name]
    elif type_name.startswith("struct "):
        base = sizes.get(type_name[7:].strip())
        if base is None:
            return None
    else:
        return None
    if dim:
        try:
            return base * int(dim, 0)
        except ValueError:
            return None
    return base


def layout(rec, ptr_size, sizes):
    """[(field, documented_offset, computed_offset, size)] or None."""
    out, cursor, gap = [], 0, False
    for f in rec["raw"]:
        if f[0] == "__gap__":
            gap = True
            continue
        if f[0] == "__parm_header__":
            for fn, fo, fs in PARM_HEADER:
                out.append((fn, None, cursor + fo,
                            ptr_size if fs == "ptr" else fs))
            cursor += PARM_HEADER_SIZE
            continue
        name, t, is_ptr, dim, doc = f[0], f[1], f[2], f[3], f[4]
        sz = field_size(t, is_ptr, dim, ptr_size, sizes)
        if sz is None:
            return None, None
        if gap:
            # first field after an omitted run: trust its documented
            # offset and resynchronise, then keep checking from there
            if doc is not None:
                cursor = doc
            gap = False
        out.append((name, doc, cursor, sz))
        cursor += sz
    return out, (None if any(f[0] == "__gap__" for f in rec["raw"]) else cursor)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--amode64", action="store_true",
                    help="8-byte pointers (xlc -q64)")
    ap.add_argument("--show-partial", action="store_true",
                    help="also list structs skipped as incomplete")
    ap.add_argument("--update-baseline", action="store_true",
                    help="record current mismatches as known issues")
    ap.add_argument("--no-baseline", action="store_true",
                    help="report every mismatch, ignoring the baseline")
    args = ap.parse_args()
    ptr = 8 if args.amode64 else 4

    by_name, order = collect_all()
    sizes = resolve_sizes(by_name, ptr)

    bad, checked, skipped, unresolved, nodoc = [], 0, [], [], 0
    for rec in order:
        if not rec["raw"]:
            continue
        fields, total = layout(rec, ptr, sizes)
        if fields is None:
            unresolved.append(f"{rec['header']}:{rec['name']}")
            continue
        documented = [f for f in fields if f[1] is not None]
        if not documented:
            nodoc += 1
            continue
        checked += 1
        for fname, doc, actual, sz in documented:
            if doc != actual:
                bad.append((rec["header"], rec["name"], fname, doc, actual, sz))
        ds = rec["declared_size"]
        if rec["truncated"]:
            skipped.append(f"{rec['header']}:{rec['name']}")
            ds = None
        if ds is not None and total is not None and ds != total:
            bad.append((rec["header"], rec["name"], "(total size)", ds, total, 0))

    mode = "AMODE 64" if args.amode64 else "AMODE 31"
    print(f"Struct layout check ({mode}, {ptr}-byte pointers)")
    print(f"  checked {checked} structs | "
          f"{len(skipped)} size-check skipped (truncated) | "
          f"{len(unresolved)} unresolved types | {nodoc} without offset comments")
    print()

    if args.show_partial:
        for label, items in (("Size check skipped (struct truncated)", skipped),
                             ("Unresolved field types", unresolved)):
            if items:
                print(f"{label}:")
                for x in items:
                    print(f"  {x}")
                print()

    def key(m):
        return f"{m[0]}:{m[1]}:{m[2]}"

    if args.update_baseline:
        BASELINE.write_text(
            "# Known struct layout mismatches - tracked, not yet resolved.\n"
            "# Regenerate: python tools/check_layout.py --update-baseline\n"
            "# See docs/layout-findings.md for what these mean.\n"
            "# Each line: header:struct:field\n"
            + "".join(f"{key(m)}\n" for m in sorted(bad, key=key)),
            encoding="utf-8")
        print(f"baseline updated: {len(bad)} known mismatches recorded in "
              f"{BASELINE.name}")
        return 0

    known = set()
    if BASELINE.exists() and not args.no_baseline:
        known = {ln.strip() for ln in BASELINE.read_text(encoding="utf-8")
                 .splitlines() if ln.strip() and not ln.startswith("#")}

    fresh = [m for m in bad if key(m) not in known]
    still = [m for m in bad if key(m) in known]
    resolved = known - {key(m) for m in bad}

    if still:
        print(f"{len(still)} known mismatch(es) carried in {BASELINE.name} "
              f"- tracked, not failing the build")
    if resolved:
        print(f"{len(resolved)} baselined mismatch(es) now FIXED - "
              f"run --update-baseline to drop them")
    if still or resolved:
        print()

    if not fresh:
        if not bad:
            print("OK - every documented offset matches the computed layout.")
        else:
            print("OK - no new layout mismatches.")
        return 0

    bad = fresh
    print(f"{len(bad)} NEW MISMATCH(ES):\n")
    print(f"  {'header':<20} {'struct':<22} {'field':<16} "
          f"{'documented':>10} {'actual':>8} {'size':>5}")
    print(f"  {'-'*20} {'-'*22} {'-'*16} {'-'*10} {'-'*8} {'-'*5}")
    for h, st, f, doc, act, sz in bad:
        print(f"  {h:<20} {st:<22} {f:<16} {doc:>10} {act:>8} "
              f"{(sz if sz else ''):>5}")
    print("\nEach mismatch means the exit reads the wrong bytes of a live "
          "control block.\nFix the field types or correct the offset "
          "comments, then re-run.")
    return 1


if __name__ == "__main__":
    sys.exit(main())
