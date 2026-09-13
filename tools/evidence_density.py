#!/usr/bin/env python
"""Measure how much offset evidence an assembler source actually carries.

docs/asm-field-evidence.md accepts exactly one thing as proof of a field's
offset: an explicit base-displacement reference, d(Rn) or d(len,Rn).  A
symbolic reference resolved through USING against a DSECT proves the
field's *width* and *type* but says nothing about where it sits.

So the ledger technique only works on source that addresses by
displacement.  This script reports how much of that a file contains, to
answer "is there anything here to build a struct from?" before anyone
starts.

    python tools/evidence_density.py                # every asm/ source
    python tools/evidence_density.py path/to/x.asm  # one file

A low score is not a defect in the source.  It usually means the module
does the right thing and uses a mapping macro - in which case go and get
the macro (docs/copy-macro-dependency.md) instead of reading the
instruction stream.
"""

import glob
import os
import re
import sys

# Operand forms that pin an offset: 16(R10), 16(7,R10), 8(8,R10)
BASE_DISP = re.compile(r'\b\d+\((?:\d+,)?(?:R\d+|\d+)\)')
USING = re.compile(r'^\s+USING\s', re.I)
COMMENT = re.compile(r'^[*.]')

# Below this, the instruction stream will not support a struct on its own.
THIN = 3.0


def scan(path):
    with open(path, encoding='utf-8', errors='replace') as fh:
        lines = fh.read().splitlines()
    code = [l for l in lines if l.strip() and not COMMENT.match(l)]
    bd = sum(len(BASE_DISP.findall(l)) for l in code)
    using = sum(1 for l in lines if USING.match(l))
    density = 100.0 * bd / len(code) if code else 0.0
    return len(lines), len(code), bd, using, density


def main(argv):
    targets = argv[1:]
    if not targets:
        targets = sorted(glob.glob(os.path.join('asm', '*', '*.asm')))
        targets += sorted(glob.glob(os.path.join('asm', 'challenges', '*',
                                                 'asm', '*.asm')))
    if not targets:
        print('no assembler sources found')
        return 1

    print('%-40s %6s %7s %6s %9s' %
          ('source', 'lines', 'base-d', 'USING', 'per 100'))
    print('-' * 74)
    thin = []
    for path in targets:
        n, ncode, bd, using, density = scan(path)
        mark = '  <- thin' if density < THIN else ''
        if density < THIN:
            thin.append(path)
        print('%-40s %6d %7d %6d %9.1f%s' %
              (path.replace(os.sep, '/'), n, bd, using, density, mark))

    print()
    print('"per 100" = base-displacement references per 100 lines of code.')
    print('Repo sample exits run 7-28.  Real production modules run 0-2,')
    print('because they address through DSECTs supplied by mapping macros.')
    if thin:
        print()
        print('%d source(s) below %.1f carry too little offset evidence to'
              % (len(thin), THIN))
        print('build a struct from the instruction stream. Obtain the DSECT')
        print('or mapping macro first - see docs/copy-macro-dependency.md.')
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
