#!/usr/bin/env python
"""Fail if two function codes in the same product family share a value.

An exit dispatches on its function code:

    if (parm->func != SA_FUNC_STATCHG) { return SA_RC_CONTINUE; }

If SA_FUNC_STATCHG and SA_FUNC_TERM are both 3, that test cannot tell a
state change from a termination, and one of the two paths is silently
unreachable.  In a logon or authorization exit the unreachable path is a
security decision.

This is how the defect arises: a product numbers its own codes from 1,
then someone swaps the literals for the generic EXIT_FUNC_INIT (1) and
EXIT_FUNC_TERM (3) from metalc_base.h without checking that 1 and 3 are
already taken.  `-Werror=macro-redefined` in tools/lint_host.sh does not
catch it, because these are different names holding the same value
rather than one name defined twice - the case layout-findings.md
finding 2 covered.

Families are keyed on the text before `_FUNC_`, so SA_FUNC_* and
LY_FUNC_* are checked separately.  Deliberate aliases are declared in
ALLOWED below.

    python tools/check_func_codes.py
"""

import collections
import glob
import os
import re
import sys

INCLUDES = 'includes'

# Constants from metalc_base.h that product headers alias.
BASE_VALUES = {
    'EXIT_FUNC_INIT': 1,
    'EXIT_FUNC_TERM': 3,
}

DEFINE = re.compile(r'^#define\s+([A-Z0-9_]*_FUNC_[A-Z0-9_]+)\s+(\S+)')

# Two names that are meant to be the same value.  Key: family.
# Value: set of frozensets, each a group that may share a value.
ALLOWED = {
    # e.g. 'XYZ': [frozenset({'XYZ_FUNC_A', 'XYZ_FUNC_B'})]
}


def resolve(token):
    """Return the integer value of a #define body, or None."""
    if token in BASE_VALUES:
        return BASE_VALUES[token]
    try:
        return int(token, 0)
    except ValueError:
        return None


def scan(path):
    """Yield (family, name, value) for every *_FUNC_* constant."""
    with open(path, encoding='utf-8', errors='replace') as fh:
        for line in fh:
            m = DEFINE.match(line)
            if not m:
                continue
            name, body = m.group(1), m.group(2)
            value = resolve(body)
            if value is None:
                continue
            yield name.rsplit('_FUNC_', 1)[0], name, value


def allowed(family, names):
    for group in ALLOWED.get(family, []):
        if set(names) <= set(group):
            return True
    return False


def main():
    headers = sorted(glob.glob(os.path.join(INCLUDES, 'metalc_*.h')))
    if not headers:
        print('no headers found under %s/' % INCLUDES)
        return 1

    collisions = []
    checked = 0
    for path in headers:
        families = collections.defaultdict(lambda: collections.defaultdict(list))
        for family, name, value in scan(path):
            families[family][value].append(name)
            checked += 1
        for family in sorted(families):
            for value, names in sorted(families[family].items()):
                if len(names) > 1 and not allowed(family, names):
                    collisions.append((path, family, value, sorted(names)))

    print('== function code collisions ==')
    for path, family, value, names in collisions:
        print('  %s: %s_FUNC_* value 0x%02X shared by %s'
              % (path.replace(os.sep, '/'), family, value, ', '.join(names)))
        print('      an exit dispatching on one of these cannot reach the other')

    print()
    print('%d constant(s) in %d header(s): %d collision(s)'
          % (checked, len(headers), len(collisions)))

    if collisions:
        print()
        print('Fix the VALUE, not the dispatch.  Confirm each code against the')
        print('assembler (CLI 4(Rn),v) or the product exit documentation.  If a')
        print('code has no source, remove the constant rather than guess it -')
        print('an undefined name is a compile error, a wrong value is not.')
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main())
