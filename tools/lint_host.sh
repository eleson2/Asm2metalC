#!/bin/sh
# Off-platform structural build of the Metal C header framework.
#
# There is no z/OS system in this repository, so nothing here can be
# compiled with xlc -qmetal.  This does the next best thing: it parses
# the whole header set with an ordinary C compiler targeting 32-bit
# (matching AMODE 31 pointer widths) and evaluates every struct layout
# assertion in tests/verify_structs.c.
#
# What it catches:   struct layout drift, type errors, syntax errors,
#                    macro collisions, header ordering problems
# What it cannot:    anything about the inline assembler, the z/OS
#                    macros it expands, or the real DSECT layouts
#
# METALC_HOST_LINT elides every METALC_ASM block (see metalc_svc.h).
# Warnings disabled and why:
#   unknown-pragmas       prolog/epilog/linkage are xlc-only
#   gnu-folding-constant  the VERIFY_* negative-array idiom
#   unused-variable       operands of the elided asm blocks
#
# Usage: tools/lint_host.sh [compiler]      default: clang, else gcc

set -e
cd "$(dirname "$0")/.."

CC="${1:-}"
if [ -z "$CC" ]; then
    if command -v clang >/dev/null 2>&1; then CC=clang
    elif command -v gcc >/dev/null 2>&1; then CC=gcc
    else
        echo "no clang or gcc found; skipping host lint" >&2
        exit 0
    fi
fi

# 32-bit target so pointers are 4 bytes, as in AMODE 31.
TARGET=""
case "$CC" in
    *clang*) TARGET="--target=i386-unknown-none" ;;
    *gcc*)   TARGET="-m32" ;;
esac

FLAGS="-fsyntax-only $TARGET -ffreestanding -DMETALC_HOST_LINT -I includes
       -Wall
       -Wno-unknown-pragmas
       -Wno-gnu-folding-constant
       -Wno-unused-variable
       -Werror=macro-redefined
       -ferror-limit=0"

echo "== host lint: $CC $TARGET =="
echo "-- struct layout assertions (tests/verify_structs.c)"
# shellcheck disable=SC2086
$CC $FLAGS tests/verify_structs.c

echo "-- each product header compiles standalone"
for h in includes/metalc_*.h; do
    case "$h" in
        *metalc_svc.h|*metalc_verify.h) continue ;;   # not standalone by design
    esac
    printf '#include "metalc_base.h"\n#include "%s"\nint _u;\n' \
        "$(basename "$h")" > /tmp/_hdr_probe.c
    # shellcheck disable=SC2086
    $CC $FLAGS /tmp/_hdr_probe.c || {
        echo "FAILED: $h" >&2
        exit 1
    }
done
rm -f /tmp/_hdr_probe.c

echo "-- converted exits parse"
for c in converted/*/*.c; do
    # shellcheck disable=SC2086
    $CC $FLAGS "$c" || { echo "FAILED: $c" >&2; exit 1; }
done

echo "host lint OK"
