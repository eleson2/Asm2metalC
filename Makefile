# Asm2C - offline verification harness
#
# There is no z/OS system here, so nothing in this repository can be
# compiled with xlc -qmetal.  These targets do everything that CAN be
# checked off-platform, so a mainframe build starts from code that is
# already known to be structurally sound.
#
#   make            run every offline check (what CI runs)
#   make layout     struct offsets match their header comments
#   make conform    converted exits obey the CLAUDE.md rules
#   make lint       host compiler parses the framework, assertions hold
#   make generate   regenerate tests/verify_structs.c from the headers
#   make baseline   re-record known layout mismatches
#
# What none of this proves: that the inline assembler is correct, that
# the z/OS macros expand, or that the headers match the real DSECTs.
# Only a build on the target system shows that -- see docs/zos-build/.

PYTHON ?= python
TOOLS   = tools

.PHONY: all check layout layout64 conform lint generate baseline clean help

all: check

check: generate-check layout conform lint
	@echo
	@echo "All offline checks passed."
	@echo "NOT verified off-platform: inline assembler, z/OS macro"
	@echo "expansion, real DSECT layouts. See docs/zos-build/README.md."

## struct offsets match the offsets their headers document
layout:
	@echo "== struct layout (AMODE 31) =="
	@$(PYTHON) $(TOOLS)/check_layout.py

## same check with 64-bit pointers - informational, not a gate
##
## Every header documents 31-bit offsets, and none yet carries the
## #ifdef pointer guards docs/amode64-exits.md requires, so every
## struct containing a pointer mismatches here by design.  This target
## measures how much work AMODE 64 support needs; it is not run by
## `make check`.  See docs/layout-findings.md finding 3.
layout64:
	@echo "== struct layout (AMODE 64, informational) =="
	@$(PYTHON) $(TOOLS)/check_layout.py --amode64 || true

## converted exits obey the mandatory rules in CLAUDE.md
conform:
	@echo
	@echo "== conversion rules =="
	@$(PYTHON) $(TOOLS)/check_conformance.py

## host compiler parses everything and evaluates the layout assertions
lint:
	@echo
	@sh $(TOOLS)/lint_host.sh

## regenerate the assertion file from the headers
generate:
	@$(PYTHON) $(TOOLS)/gen_struct_asserts.py --write

## fail if the generated file is stale relative to the headers
generate-check:
	@echo "== generated files current =="
	@$(PYTHON) $(TOOLS)/gen_struct_asserts.py --check

## re-record known layout mismatches after resolving some
baseline:
	@$(PYTHON) $(TOOLS)/check_layout.py --update-baseline
	@$(PYTHON) $(TOOLS)/gen_struct_asserts.py --write

clean:
	@rm -f /tmp/_hdr_probe.c

help:
	@grep -B1 -E '^[a-z-]+:' $(MAKEFILE_LIST) \
		| grep -A1 '^##' | sed 's/^## /  /;s/:.*//' | grep -v '^--'
