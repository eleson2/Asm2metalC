# Exit Chaining — Writing Chain-Friendly Metal C Exits

When a large organization installs an exit, it rarely installs only one routine
at that exit point.  Two or more exit routines are **chained** at the same exit
point and called in sequence.  A converted Metal C exit that ignores chaining
will either absorb invocations that should reach subsequent exits, or return
codes that confuse the product's exit dispatch mechanism.

This document covers: what chaining is, how each product implements it, the
return code conventions that control chain flow, and the rules for writing a
chain-aware Metal C exit.

---

## 1. What Exit Chaining Is

Most z/OS products allow multiple exit routines to be registered at the same
exit point.  The product calls them in the order they are registered.  Each
routine receives the same parameter block and can:

- **Pass** (do nothing meaningful): return a neutral "continue" RC so the next
  routine in the chain gets called.
- **Modify**: update fields in the parameter block and pass, so subsequent
  exits see the modified values.
- **Absorb**: return a definitive RC (accept or reject) that ends the chain
  immediately — no further routines are called.

The critical rule is: **a converted exit must not inadvertently absorb the
chain** by returning a definitive RC when it intends to pass.

---

## 2. Chain Flow by Return Code

The exact RC that "passes" vs. "absorbs" is product-specific, but the general
pattern is:

| RC meaning | Effect on chain |
|---|---|
| Continue / pass-through (usually 0) | Product calls next exit in chain |
| Accept (affirmative decision, e.g. 0 with decision flag set) | Chain may end; depends on product |
| Reject / deny (definitive, e.g. 4 or 8) | Chain ends immediately |
| Skip / bypass (e.g. 12) | Chain ends; product skips its own processing too |

The Metal C exit must return the **neutral RC** when it has no opinion on the
current invocation, so that other exits in the chain get a chance to decide.

---

## 3. Product-Specific Chaining Rules

### 3.1 JES2

JES2 supports multiple routines per exit point via `$EXIT` or dynamic exit
registration.  JES2 calls them in registration order.

**Chain-control RCs:**

| RC | Meaning | Chain effect |
|---|---|---|
| 0 | Continue | Next routine called |
| 4 | Skip this function | Next routine called (function skipped, chain continues) |
| 8 | Fail job | Chain ends; subsequent routines NOT called |
| 12 | Bypass exit point | Chain ends; JES2 skips its own processing |

**Rule**: Return 0 (continue) unless you have a definitive accept or reject
decision.  Never return 8 (fail) unless you specifically intend to end the chain
and fail the job.

**Registration order**: Dynamic exits are called in the order listed in
`JES2PARM`.  Legacy `$EXIT`-based exits depend on the link-edit order.  The
site controls registration order — the exit writer cannot assume position.

### 3.2 RACF

RACF calls multiple exits at each ICHRIX01, ICHRCX01, ICHPWX01 etc. point.

**Chain-control RCs:**

| RC | Meaning | Chain effect |
|---|---|---|
| 0 | Continue normal processing | Next routine called |
| 4 | Bypass RACF processing | Chain ends; RACF skips its checking |
| 8 | Fail the request | Chain ends; request denied |

**Rule**: Return 0 unless you are making a definitive decision.
RACF exits that return 4 (bypass) cause RACF to *skip its own checking*,
which is a security hole if the exit was not intended to grant full bypass.
Use RC=4 only when you specifically intend to bypass RACF for this request.

**Important**: RACF exit chaining is managed by RACF itself.  When using
`RACROUTE` from within an exit (rare but possible), the recursive call goes
through SAF routing again and may trigger the same exit chain, causing
infinite recursion.  Guard against this with a re-entry flag if needed.

### 3.3 IMS

IMS exit chaining depends on the exit type:

**Sign-on exits (DFSWHU00)**: IMS calls multiple exits only if the first
returns "continue" (RC=0).  A non-zero RC (deny or allow) ends the chain.

**MSC routing exits (DFSMSCE0)**: Only one routine is typically active
(IMS registers one MSCP exit).  Chaining is achieved through the exit itself
calling a chain of internal routines, not through IMS dispatch.

**Rule**: Return `IMS_SIGN_NOTAUTH` (non-zero) only when you have a definitive
denial.  Return `IMS_SIGN_CONTINUE` (0) to pass to the next exit.

### 3.4 CICS

CICS DFHPEP (Program Error Program) chains via the `DFHPEP` mechanism.
CICS calls only one PEP, but the PEP can internally chain to a previous PEP
by saving and calling the previous exit address.

**Chaining pattern for CICS**:

```c
/* Save pointer to previous PEP at activation time (stored by CICS) */
static void (*prev_pep)(struct dfhpeppar *) = NULL;

int DFHPEP(struct dfhpeppar *parm) {
    /* Our logic first */
    if (parm->pep_type == PEP_TYPE_ASRA) {
        /* handle ASRA */
    }

    /* Then call previous PEP in chain (if any) */
    if (prev_pep != NULL) {
        prev_pep(parm);
    }

    return CICS_PEP_CONTINUE;
}
```

**Rule**: Always chain to the previous PEP unless you intend to replace it
entirely.  The site may have a vendor PEP already installed.

### 3.5 DB2

DB2 exit chaining: DB2 supports a single exit routine per exit point (no
built-in chaining).  If a site has multiple exits, they must be manually
chained within the exit source by the installer, using the same CICS PEP
pattern above.

### 3.6 SMF (IEFU83)

SMF calls multiple exits in the order listed in `SMFPRMxx`.  All registered
exits are called regardless of what earlier exits return — SMF does not
short-circuit the chain based on RC.

**Rule**: There is no absorb risk in SMF exits.  Return 0 (continue) or
4 (suppress record) freely; all other exits will still be called.

### 3.7 VTAM (ISTEXCLY)

VTAM calls only one logon verify exit (configured in `ATCSTRxx`).
No built-in chaining.  Internal chaining must be implemented manually.

### 3.8 TCP/IP (FTCHKCMD)

A single exit routine per FTP event.  No built-in chaining.

---

## 4. Writing a Chain-Aware Exit

### Rule C1 — Default to the neutral RC

At the top of every decision branch, initialize the return code to the neutral
"continue" value for that product:

```c
int HASPEX02(struct jes2_exit_parm *parm) {
    int rc = JES2_RC_CONTINUE;   /* default: pass to next exit in chain */

    /* Only change rc if we have a definitive decision */
    if (should_reject(parm)) {
        rc = JES2_RC_FAIL;
    }

    return rc;
}
```

Never initialize to a reject RC and then "undo" it on success — that pattern
risks returning a reject when an unexpected code path is reached.

### Rule C2 — Do not return a definitive RC for conditions you don't own

If your exit is responsible for PAYROLL jobs and sees a BATCH job it doesn't
recognize, return the neutral RC.  Let a subsequent exit in the chain handle it.

```c
/* Wrong: returns reject for jobs this exit doesn't own */
if (!is_payroll_job(parm)) {
    return JES2_RC_FAIL;   /* BAD: absorbs the chain for all non-PAYROLL jobs */
}

/* Correct: pass through jobs this exit doesn't own */
if (!is_payroll_job(parm)) {
    return JES2_RC_CONTINUE;   /* Good: next exit in chain decides */
}
```

### Rule C3 — Document the chain position assumption

In the module header comment, document where in the chain this exit is expected
to be placed:

```c
/* Chain Position:
 * This exit is designed to run FIRST in the HASPEX02 chain.
 * It rejects non-PAYROLL batch jobs; all others pass through.
 * A subsequent exit handles PAYROLL class validation.
 * If installed AFTER a general-purpose reject exit, payroll jobs
 * will never reach this exit.
 */
```

### Rule C4 — Test with a pass-through stub exit in the chain

Before production, test with a dummy exit registered both before and after
your exit in the chain.  The dummy exits log their invocation to the system
console so you can verify that:
- Exits before yours are called before yours
- Exits after yours are called when you return the neutral RC
- Exits after yours are NOT called when you return a reject RC

### Rule C5 — Avoid parameter block modifications that surprise later exits

If your exit modifies a field in the parameter block (e.g., changes a job
class, modifies a destination), later exits in the chain will see the modified
value.  Document every field your exit modifies:

```c
/* Parameter Block Modifications:
 * This exit modifies parm->jctjclas (job class) from 'A' to 'P' for
 * PAYROLL jobs.  Exits later in the chain will see class 'P', not 'A'.
 */
```

---

## 5. Chaining in the Verification Matrix

Add a row in §2 (Entry Conditions) documenting chain awareness:

```
| Chain position | Expected: first in HASPEX02 chain; subsequent exits handle class validation |
```

Add an entry in §6 (Flagged Concerns) if chaining behavior is uncertain:

```
Cx — Chain position not confirmed with site
Assessment: Open — confirm with JES2 administrator which exits are registered
at EXIT02 and in what order before enabling this exit
```

Add to the sign-off checklist:

```
- [ ] Chain position confirmed with JES2 / product administrator
- [ ] Neutral RC verified: non-owned invocations return JES2_RC_CONTINUE (0)
- [ ] Test with dummy exit in chain (before and after) confirmed correct flow
```

---

## 6. Quick Reference — Neutral RC by Product

| Product | Neutral "pass" RC | Constant |
|---------|-------------------|----------|
| JES2 | 0 | `JES2_RC_CONTINUE` |
| RACF exits | 0 | `RACF_RIXRC_CONTINUE` / `RACF_RCXRC_CONTINUE` |
| IMS sign-on | 0 | `IMS_SIGN_CONTINUE` |
| CICS PEP | 0 | `CICS_PEP_CONTINUE` |
| DB2 auth | 0 | `DB2_ATH_CONTINUE` |
| DFSMS alloc | 0 | `ALLOC_RC_CONTINUE` |
| SMF | 0 | `SMF_RC_CONTINUE` |
| MQ channel | 0 | `MQXCC_OK` (exitResponse field) |
| SA z/OS | 0 | `SA_RES_CONTINUE` |
| OPC/TWS | 0 | `OPC_UX007_CONTINUE` |
| VTAM | 8 (defer) | `VTAM_LY_DEFER` — neutral is DEFER, not 0 |
| TCP/IP FTP | 0 | `FTP_RC_CONTINUE` |
| ACF2 | 0 | `ACF2_RC_CONTINUE` |
| NetView | 0 | `NV_RC_CONTINUE` |

> **VTAM note**: For ISTEXCLY, DEFER (RC=8) is the neutral/pass-through RC.
> REJECT (RC=4) is the definitive denial.  This is inverted from most products.
> A VTAM chain-aware exit must return DEFER, not 0, when it has no opinion.

---

## 7. Related Documents

- `docs/asm-to-metalc-jes2.md` — JES2 RC semantics and registration
- `docs/asm-to-metalc-racf.md` — RACF exit RC conventions, including why
  RC=4 grants access on ICHRIX/ICHRCX/ICHRDX
- `docs/asm-to-metalc-vtam.md` — VTAM DEFER vs REJECT distinction
- `docs/verification-matrices/README.md` — production deployment block table
- `docs/exit-chaining.md` (this document)
