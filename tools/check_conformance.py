#!/usr/bin/env python3
"""
Check converted exits against the mandatory rules in CLAUDE.md.

Every verification matrix carries a sign-off checklist, and most of its
items are mechanical: does the file have a pack pragma, does it return
named constants, does it contain inline assembler.  Those do not need a
human, and at 50-200 exits the human checks are the bottleneck.  This
tool clears the mechanical ones so review effort goes to the items that
actually need judgement -- semantic equivalence and security decisions.

    python tools/check_conformance.py                 # all converted exits
    python tools/check_conformance.py converted/IMS/DFSWHU00.c
    python tools/check_conformance.py --list-rules

Exit status is non-zero if any ERROR-level rule fails.

What this cannot check: whether the C means the same thing as the ASM.
That stays with the reviewer and the verification matrix.
"""

import argparse
import re
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent

# (id, severity, description) -- severity ERROR fails the build, WARN does not
RULES = [
    ("no-inline-asm", "ERROR",
     "no __asm in a converted exit (CLAUDE.md rule 8)"),
    ("prolog-epilog", "ERROR",
     "#pragma prolog and #pragma epilog present for the entry point"),
    ("pack-pragma", "ERROR",
     "every struct definition is inside #pragma pack(1)"),
    ("no-literal-rc", "ERROR",
     "return uses a named constant, never a literal integer"),
    ("no-libc", "ERROR",
     "no standard C library header or function"),
    ("base-header", "ERROR",
     "includes metalc_base.h"),
    ("saf-stub-linked", "ERROR",
     "an exit calling saf_auth* documents the SAFAUTH link edit"),
    ("fixed-width-types", "WARN",
     "struct fields use fixed-width types, not raw int/long/short"),
    ("register-map", "WARN",
     "header documents the register-to-variable mapping"),
    ("no-static-writable", "ERROR",
     "no static writable data (reentrancy)"),
    ("matrix-exists", "WARN",
     "a verification matrix exists for the module"),
]

LIBC_HEADERS = re.compile(r'#\s*include\s*<(?!stdarg\.h)')
LIBC_FUNCS = re.compile(
    r"\b(?:printf|sprintf|malloc|free|strcpy|strcat|strcmp|strlen|"
    r"memcpy|memset|memcmp|exit|atoi)\s*\(")
STATIC_WRITABLE = re.compile(r"^\s*static\s+(?!const\b|inline\b)[A-Za-z_]")
STRUCT_DEF = re.compile(r"^\s*struct\s+[A-Za-z_][A-Za-z0-9_]*\s*\{")
PACK_ON = re.compile(r"^\s*#\s*pragma\s+pack\s*\(\s*1\s*\)")
PACK_OFF = re.compile(r"^\s*#\s*pragma\s+pack\s*\(\s*(?:\)|reset\s*\))")
LITERAL_RC = re.compile(r"^\s*return\s+(-?\d+)\s*;")


def strip_comments(text):
    text = re.sub(r"/\*.*?\*/", " ", text, flags=re.S)
    return re.sub(r"//[^\n]*", " ", text)


def check(path):
    """Return list of (rule_id, severity, line_no, detail)."""
    raw = path.read_text(encoding="utf-8", errors="replace")
    lines = raw.splitlines()
    code = strip_comments(raw)
    code_lines = strip_comments("\n".join(lines)).splitlines()
    out = []

    def fail(rid, line=0, detail=""):
        sev = next(s for i, s, _ in RULES if i == rid)
        out.append((rid, sev, line, detail))

    # no-inline-asm
    for n, ln in enumerate(code_lines, 1):
        if "__asm" in ln:
            fail("no-inline-asm", n, ln.strip()[:60])

    # prolog / epilog
    if "#pragma prolog" not in code:
        fail("prolog-epilog", 0, "no #pragma prolog")
    if "#pragma epilog" not in code:
        fail("prolog-epilog", 0, "no #pragma epilog")

    # pack pragma around every struct definition
    packed = False
    for n, ln in enumerate(code_lines, 1):
        if PACK_ON.match(ln):
            packed = True
        elif PACK_OFF.match(ln):
            packed = False
        elif STRUCT_DEF.match(ln) and not packed:
            fail("pack-pragma", n, ln.strip()[:60])

    # literal return codes (0 is allowed only in a non-exit helper; flag all
    # in the exit body and let the reviewer justify with a named constant)
    for n, ln in enumerate(code_lines, 1):
        m = LITERAL_RC.match(ln)
        if m:
            fail("no-literal-rc", n, f"return {m.group(1)};")

    # standard library
    for n, ln in enumerate(code_lines, 1):
        if LIBC_HEADERS.search(ln):
            fail("no-libc", n, ln.strip()[:60])
    for m in LIBC_FUNCS.finditer(code):
        n = code[:m.start()].count("\n") + 1
        fail("no-libc", n, m.group(0))

    # required include
    if '#include "metalc_base.h"' not in code:
        fail("base-header", 0, "metalc_base.h not included")

    # SAF callers must document the stub link edit
    if re.search(r"\bsaf_auth\w*\s*\(", code):
        if "SAFAUTH" not in raw:
            fail("saf-stub-linked", 0,
                 "calls saf_auth* but no SAFAUTH link-edit note in the header")

    # fixed-width types inside structs
    packed = False
    for n, ln in enumerate(code_lines, 1):
        if PACK_ON.match(ln):
            packed = True
        elif PACK_OFF.match(ln):
            packed = False
        elif packed and re.match(
                r"^\s*(?:unsigned\s+|signed\s+)?(?:int|long|short)\s+"
                r"\*?[A-Za-z_]", ln):
            fail("fixed-width-types", n, ln.strip()[:60])

    # register map comment (comments are stripped from `code`, search raw)
    if "Register mapping" not in raw and "Register Mapping" not in raw:
        fail("register-map", 0, "no register map in the module header")

    # static writable data
    for n, ln in enumerate(code_lines, 1):
        if STATIC_WRITABLE.match(ln):
            fail("no-static-writable", n, ln.strip()[:60])

    # verification matrix
    product = path.parent.name.lower()
    stem = path.stem
    matrices = list((REPO / "docs" / "verification-matrices")
                    .glob(f"{stem}_*.md"))
    if not matrices:
        fail("matrix-exists", 0,
             f"no docs/verification-matrices/{stem}_{product}.md")

    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("files", nargs="*", help="default: converted/*/*.c")
    ap.add_argument("--list-rules", action="store_true")
    ap.add_argument("--warnings-as-errors", action="store_true")
    args = ap.parse_args()

    if args.list_rules:
        w = max(len(r[0]) for r in RULES)
        for rid, sev, desc in RULES:
            print(f"  {rid:<{w}}  {sev:<5}  {desc}")
        return 0

    files = ([Path(f) for f in args.files] if args.files
             else sorted((REPO / "converted").glob("*/*.c")))
    if not files:
        print("no files to check", file=sys.stderr)
        return 1

    n_err = n_warn = 0
    for f in files:
        issues = check(f)
        try:
            rel = f.relative_to(REPO)
        except ValueError:
            rel = f
        errs = [i for i in issues if i[1] == "ERROR"]
        warns = [i for i in issues if i[1] == "WARN"]
        n_err += len(errs)
        n_warn += len(warns)
        if not issues:
            print(f"PASS  {rel}")
            continue
        print(f"{'FAIL' if errs else 'WARN'}  {rel}")
        for rid, sev, line, detail in issues:
            loc = f":{line}" if line else ""
            print(f"        {sev:<5} {rid}{loc}"
                  + (f"  {detail}" if detail else ""))

    print(f"\n{len(files)} file(s): {n_err} error(s), {n_warn} warning(s)")
    if n_err:
        print("Mechanical rules failed. See CLAUDE.md 'Conversion Rules'.")
    return 1 if (n_err or (args.warnings_as_errors and n_warn)) else 0


if __name__ == "__main__":
    sys.exit(main())
