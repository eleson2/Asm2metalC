# Pre-Conversion Triage Template

Use this template to assess an HLASM source file **before** handing it to
the conversion agent or beginning a manual conversion.  A completed triage
form should accompany every conversion request.

Triage is a two-step process:
1. **Analyst** fills in Sections 1–5 by reading the ASM source.
2. **Conversion lead** reviews the triage and signs off on scope before work starts.

Alternatively, run the **`asm-pre-analyzer`** agent to auto-populate most fields.

---

## Module Identification

| Field | Value |
|-------|-------|
| Module name | |
| Source path | `asm/<PRODUCT>/<MODULE>.asm` |
| Product | |
| Exit name / number | |
| Date of triage | |
| Analyst | |

---

## Section 1 — Entry Point Inventory

List every `CSECT`, `ENTRY`, `$ENTRY`, or `START` label.

| Label | Type | Parameter block / register at entry | Notes |
|-------|------|-------------------------------------|-------|
| | Main entry | | |
| | Subroutine | | |

---

## Section 2 — DSECT / Mapping Inventory

List every DSECT referenced (via `USING DSECT,Rn` or macro expansion).

| DSECT name | IBM macro source | Register | Mapped C struct | Status |
|------------|-----------------|----------|-----------------|--------|
| | | | `struct tbd` | Mapped / Unknown |

**Status values:**
- **Mapped** — struct exists in `includes/metalc_<product>.h`
- **Partial** — some fields missing from header; need to add before converting
- **Unknown** — DSECT not found in any header; must be added to product header
  or flagged for manual review

---

## Section 3 — Macro Inventory

List every macro invocation and classify it against
`docs/system-services-catalog.md` §3.

| Macro | Category | Conversion approach | Status |
|-------|----------|---------------------|--------|
| WTO | Console | `wto_simple()` / `wto_write()` | IMPLEMENTED |
| GETMAIN / FREEMAIN | Storage | `getmain()` / `freemain()` | IMPLEMENTED |
| STORAGE OBTAIN / RELEASE | Storage | `storage_obtain()` / `storage_release()` | IMPLEMENTED |
| STCK | Time | `get_tod_clock()` | IMPLEMENTED |
| RACROUTE REQUEST=AUTH | SAF | `saf_auth()` / `saf_auth_appl()` + `asm/stubs/SAFAUTH.asm` | IMPLEMENTED |
| $SAVE / $RETURN / $ENTRY | JES2 linkage | `#pragma prolog/epilog` | IMPLEMENTED |
| SPLEVEL / SYSSTATE / TITLE / EJECT | Assembly-time | emit nothing | NO-OP |
| | | | |

**Status values** (same vocabulary as the catalog):

- **IMPLEMENTED** — wrapper or stub exists; call it
- **NO-OP** — assembly-time directive; generates no code, emit nothing
- **ADD WRAPPER** — needs a new wrapper in `includes/metalc_svc.h` (catalog §4)
- **ADD STUB** — needs a new stub in `asm/stubs/` (catalog §5)
- **DO NOT CONVERT** — flag for manual review

Any macro not in the catalog at all is **ADD WRAPPER** or **ADD STUB** by
the test in catalog §4.1 — never inline `__asm` in the exit.

### 3.1 New services required

Fill this in for every macro marked ADD WRAPPER or ADD STUB. These are
build tasks that must complete before or alongside the conversion; they are
not optional cleanup.

| Macro (with keyword forms used) | Wrapper or stub | Target file | Owner | Done |
|---|---|---|---|---|
| | | | | |

If this table is non-empty and the work is not done, the conversion is
**BLOCKED**, not partial. Record it as such in Section 6 — the
partial-scope policy covers dropped *function*, not missing services.

---

## Section 4 — Complexity Flag Assessment

Answer YES or NO.  All YES answers must be resolved before conversion proceeds.

| Flag | Answer | Line(s) | Resolution / Notes |
|------|--------|---------|--------------------|
| Self-modifying code (`STC`/`MVI` into executable storage) | | | |
| `EXECUTE` with variable target (`EX Rn,label` where Rn is not constant) | | | |
| Channel programs (CCW / EXCP / STARTIO) | | | |
| Cross-memory services (PC / PT / SAC / SSAR instructions) | | | |
| AR-mode (LAM / EAR / SAR instructions; ALET usage) | | | |
| SVC calls (list all SVCs used) | | | |
| ESTAE / SETFRR recovery blocks | | | |
| Conditional assembly (`AIF`, `AGO`, `&variable` with significant branches) | | | |
| Multiple CSECTs or pseudo-reentrant self-copy pattern | | | |
| 64-bit instructions (`LG`, `STG`, `AG`, etc.) with AMODE=64 | | | |
| Inter-module calls to unnamed internal subroutines | | | |

---

## Section 5 — Return Code Analysis

List every exit path (every `BR R14`, `$RETURN`, or equivalent).

| Label | Condition | R15 value at exit | Meaning | C constant |
|-------|-----------|-------------------|---------|------------|
| | Normal | 0 | Continue | `RC_OK` / product RC |
| | Error | 8 | Reject | `RC_ERROR` / product RC |

---

## Section 6 — Scope Decision

Based on Sections 1–5, determine the conversion scope.

### Recommended scope

- [ ] **Full conversion** — all ASM functions will be converted to Metal C
- [ ] **Partial conversion** — see excluded sections below
- [ ] **Do not convert** — reasons: ________________________________

### Excluded sections (if partial)

| ASM function / label | Reason for exclusion | Follow-on action |
|---------------------|---------------------|------------------|
| | DSECT not mapped | Add to product header first |
| | Self-modifying code | Requires manual rewrite |
| | Cross-memory service (PC/PT/SSAR) | DO NOT CONVERT — flag for manual review |

A partial conversion **must** follow the `docs/partial-scope-policy.md` before
the converted file can be placed in service.

---

## Section 7 — Pre-Work Checklist

Complete all checked items before handing to the conversion agent or beginning
manual conversion.

- [ ] All DSECTs have a mapped C struct (or are listed as excluded)
- [ ] All macros have a known conversion approach (or are listed as Unknown)
- [ ] All YES items in Section 4 are resolved (or the affected sections are excluded)
- [ ] Return code semantics are understood and C constants are identified
- [ ] Product guide exists: `docs/asm-to-metalc-<product>.md`
- [ ] Product header exists: `includes/metalc_<product>.h`
- [ ] If product guide / header does not exist, run `product-onboarder` agent first
- [ ] Scope decision made and signed off (Section 6 complete)

---

## Section 8 — Conversion Agent Handoff

When handing to the `asm-to-metalc-converter` agent, include:

1. This completed triage form (or the output of `asm-pre-analyzer`)
2. Source file path: `asm/<PRODUCT>/<MODULE>.asm`
3. Confirmed scope (full or partial, with excluded section list)
4. Any special instructions (e.g., "preserve the exact WTO message format")

Suggested agent invocation:

```
Convert: asm/<PRODUCT>/<MODULE>.asm
Scope: [Full | Partial — exclude: <label list>]
Triage notes: [paste Section 4 and Section 6 here]
```

---

## Section 9 — Sign-off

| Role | Name | Date | Signature |
|------|------|------|-----------|
| Analyst (triage) | | | |
| Conversion lead (scope approval) | | | |

> A triage form without both signatures should not proceed to conversion.
