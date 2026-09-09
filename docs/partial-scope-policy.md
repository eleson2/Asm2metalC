# Partial Scope Conversion Policy

## Purpose

This policy defines when a **partial scope conversion** is acceptable, how it
must be documented, and what sign-off is required before a partial-scope Metal C
exit can replace its assembler predecessor in production.

A partial scope conversion is one where the converted Metal C file does **not**
implement all functions present in the original HLASM source.

---

## 1. When Partial Scope Is Acceptable

A partial conversion is acceptable when **all** of the following are true:

1. **The excluded functions are explicitly identified** and documented in both the
   converted source file and the verification matrix.

2. **The excluded functions have a documented reason** from the list below:

   | Reason code | Description |
   |-------------|-------------|
   | `DSECT-UNMAPPED` | Required DSECT fields are not yet mapped in the product header |
   | `COMPLEX-ASM` | Function uses techniques that cannot be safely translated (self-modifying code, AR-mode, channel programs, cross-memory services) |
   | `OUT-OF-SCOPE` | Function is outside the agreed delivery scope for this iteration |
   | `DEFERRED` | Function will be implemented in a follow-on change (tracked in change management) |
   | `SUPERSEDED` | Function is no longer needed in the target environment |

3. **The excluded functions are non-critical for the initial deployment**, meaning:
   - The included functions provide standalone value, AND
   - The excluded functions' absence does not cause data loss, system damage,
     or security exposure in normal operation.

4. **A follow-on work item is created** in change management for each `DEFERRED`
   or `DSECT-UNMAPPED` exclusion.

---

## 2. Prohibited Partial Conversions

A partial conversion is **NOT acceptable** when:

- The excluded functions are on the **critical path** for the exit's primary purpose
  (e.g., excluding the main authorization check from an auth exit).
- Excluding the functions would cause the exit to return an incorrect default that
  could expose a security vulnerability or cause data corruption.
- The excluded functions interact with the included functions in ways that leave
  the combined module in an inconsistent state.

In these cases, the conversion must either:
- Expand scope to include the problematic functions, OR
- Remain as HLASM and be deferred entirely.

---

## 3. Required Documentation

### 3.1 In the Converted Source File

Every partial-scope conversion must include a `SCOPE REDUCTION` comment block
immediately after the file header comment and before the `#include` directives:

```c
/*********************************************************************
 * SCOPE REDUCTION NOTICE
 *
 * This file is a PARTIAL SCOPE conversion.  The following functions
 * are present in the original ASM source but are NOT implemented here.
 * This exit CANNOT fully replace the ASM module until these functions
 * are implemented.
 *
 * Excluded function 1: <ASM label/name>
 *   Reason: <reason code from policy>
 *   Description: <what the function does>
 *   Follow-on item: <change management reference>
 *
 * Excluded function 2: <ASM label/name>
 *   Reason: <reason code>
 *   Description: <what the function does>
 *   Follow-on item: <change management reference or N/A>
 *********************************************************************/
```

### 3.2 In the Verification Matrix

The verification matrix must include a **Section 4: Scope Reduction** table
(this section already exists in the standard matrix format):

```markdown
## 4. Scope Reduction — Functions Present in ASM but Absent in C

| ASM Function | Description | Reason Excluded |
|-------------|-------------|-----------------|
| <label> | <description> | <reason code> |
```

A Flagged Concern entry must also be created for each excluded function:

```markdown
**Cn — <function name> absent**
> This function exists in the ASM source and is absent in the C port.
> The exit CANNOT fully replace the ASM module until it is implemented.
> **Assessment:** Open work item <reference>. Track in change management.
```

### 3.3 In the Sign-off Checklist

The verification matrix sign-off checklist must include:

```markdown
- [ ] Scope reduction documented and accepted by <product> owner
- [ ] All excluded functions tracked in change management
- [ ] Partial-scope deployment decision reviewed by security team (if applicable)
```

---

## 4. Deployment Gate

A partial-scope exit **must not replace the ASM module in production** unless
all of the following are satisfied:

| Gate | Who approves |
|------|-------------|
| Verification matrix at "In Review" or "Verified" status | Verification lead |
| Scope reduction accepted in writing by the product/application owner | Product owner |
| All excluded functions tracked in change management | Change manager |
| Security review completed (if any excluded function has a security role) | Security architect |
| Rollback plan documented (ASM version available to restore) | Operations team |

---

## 5. Scope Expansion

When a follow-on change implements a previously excluded function:

1. Remove or update the `SCOPE REDUCTION NOTICE` in the source file.
2. Add the new function's ASM labels to the Logic Equivalence Table in the matrix.
3. Update the matrix status to reflect the expanded scope.
4. Close the change management item for the excluded function.
5. Obtain fresh sign-off before re-deploying.

---

## 6. Examples

### Acceptable partial scope

**HASPEX02** (JES2 Job Statement Scan):
- Included: Function 2 — CLASS= keyword scanning (the primary delivery objective).
- Excluded: Function 1 — SWBT extension initialization (`DSECT-UNMAPPED`).
- Excluded: Function 3 — MSGCLASS= scanning (`OUT-OF-SCOPE` for initial delivery).
- Rationale: Functions 1 and 3 are independent of Function 2.  Function 2 works
  correctly without them.  The ASM version handles all three; the C version
  handles only Function 2 in the initial release.

### Unacceptable partial scope

**Hypothetical auth exit** where the main SAF call is excluded:
- Included: Logging function only.
- Excluded: RACROUTE authorization check (`COMPLEX-ASM`).
- **REJECTED**: Without the authorization check, the exit always permits access.
  This is a security exposure.  Scope must be expanded or the exit deferred.

---

## 7. Policy Owner and Review

This policy is owned by the **conversion lead**.  It should be reviewed whenever:

- A new product is onboarded that has complex exclusion scenarios.
- A security incident is traced to an incomplete partial conversion.
- The project's quality standards are updated.
