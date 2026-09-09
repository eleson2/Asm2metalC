# AI Translation Rules: RACF Exit Assembler to Metal C

## Supplement to General Translation Rules

This document supplements the general assembler-to-Metal-C translation rules
with RACF-specific guidance.  Read `docs/ai-conversion-steering.md` first —
its rules override anything here that conflicts.

Header: `includes/metalc_racf.h`
SAF services: `includes/metalc_saf.h` (see §7)

> **Every rule in this document exists because a RACF exit decides who gets
> access to what.** A conversion defect here is a security defect, not a
> functional one. When something is ambiguous, choose the reading that denies.

---

## 1. RACF Exit Overview

RACF exits are installed as separate load modules with fixed names.  Unlike
JES2, there is no exit number — RACF locates the exit by module name in LPA
or the linklist, and calls it if it is there.

### Exit families

| Exit | Called during | Typical use |
|------|---------------|-------------|
| `ICHRIX01` / `ICHRIX02` | RACINIT (sign-on) pre / post | Restrict logon by terminal, time, application |
| `ICHRCX01` / `ICHRCX02` | RACROUTE AUTH pre / post | Override or audit resource access decisions |
| `ICHRDX01` / `ICHRDX02` | RACDEF (profile define) pre / post | Enforce naming standards on new profiles |
| `ICHRFX01` / `ICHRFX02` | RACF database I/O pre / post | Very rare; database-level interception |
| `ICHPWX01` | Password change / verification | Password quality rules |
| `ICHNCV00` | New password validation | Password history and syntax |
| `ICHCNX00` | RACF command processing | Restrict or audit RACF commands |

**Pre-exits (`…X01`) can change the decision. Post-exits (`…X02`) generally
cannot** — the decision has already been made and the exit is there to audit
or log it.  Converting a post-exit as though its return code steers the
outcome is a common error; check which one you have before mapping return
codes.

### Parameter block per exit

Each exit receives R1 pointing at its own parameter list.  All are mapped in
`includes/metalc_racf.h`:

| Exit | C struct |
|------|----------|
| `ICHRIX01/02` | `struct racf_rix_parm` (64 bytes) |
| `ICHRCX01/02` | `struct racf_rcx_parm` (56 bytes) |
| `ICHPWX01` | `struct racf_pwx_parm` (44 bytes) |
| `ICHNCV00` | `struct racf_ncv_parm` (40 bytes) |
| `ICHCNX00` | `struct racf_cnx_parm` (40 bytes) |

---

## 2. Return Codes — Read This Before Mapping Any

RACF return codes do **not** follow the generic `RC_OK` / `RC_WARNING` /
`RC_ERROR` intuition from `docs/ai-conversion-steering.md` §3.  The same
numeric value means opposite things across RACF exits.

### 2.1 RC=4 on ICHRIX / ICHRCX / ICHRDX means ALLOW

| RC | Constant | Meaning | Effect |
|----|----------|---------|--------|
| 0 | `RACF_RIXRC_CONTINUE` / `RACF_RCXRC_CONTINUE` | Continue normal processing | RACF makes its own decision |
| 4 | `RACF_RIXRC_BYPASS` / `RACF_RCXRC_ALLOW` | **Bypass RACF processing** | RACF skips its check — **access is granted** |
| 8 | `RACF_RIXRC_FAIL` / `RACF_RCXRC_DENY` | Fail the request | Access denied |

> **RC=4 grants access.** It tells RACF to skip its own checking entirely.
> An exit that returns 4 as a "warning" or a "soft reject" — the natural
> reading of `RC_WARNING` — silently authorizes the request. This is the
> single most dangerous mapping error in RACF conversion.

Use `RACF_RCXRC_ALLOW` only where the ASM explicitly and deliberately
bypasses RACF.  If the ASM's intent is unclear, return 0 (continue) and let
RACF decide — that is the safe default, never 4.

### 2.2 RC=4 on ICHPWX01 / ICHNCV00 means REJECT

The password exits use the opposite convention:

| RC | Constant | Meaning |
|----|----------|---------|
| 0 | `RACF_PWXRC_ACCEPT` | Accept the password |
| 4 | `RACF_PWXRC_FAIL` | Reject — RACF issues its default message |
| 8 | `RACF_PWXRC_FAILMSG` | Reject — the exit supplied a message |

`RACF_NCVRC_ACCEPT` (0) / `RACF_NCVRC_REJECT` (4) for `ICHNCV00`.

So `4` rejects on a password exit and grants on an authorization exit.
**Never carry a return-code mapping from one RACF exit to another.** Look up
the constant for the specific exit being converted.

### 2.3 Neutral RC for chaining

Initialise the return code variable to the neutral **0** (continue), per
`docs/exit-chaining.md`.  Assign 4 or 8 only where the exit reaches a
definitive decision for this invocation.

```c
int rc = RACF_RCXRC_CONTINUE;   /* neutral — RACF decides */
...
return rc;
```

---

## 3. Calling Convention

`ICHPWX01` in `asm/RACF/ICHPWX01.asm` uses the standard save-area form:

```asm
ICHPWX01 CSECT ,
ICHPWX01 AMODE 31
ICHPWX01 RMODE ANY
         SAVE  (14,12),,*          Save the registers
         LR    R12,R15             Load base register
         USING ICHPWX01,R12
         LR    R10,R1              Load PWXPL address
         USING PWXPL,R10
```

```c
#pragma prolog(ICHPWX01, "SAVE(14,12),LR(12,15)")
#pragma epilog(ICHPWX01, "RETURN(14,12)")

int ICHPWX01(struct racf_pwx_parm *parm) {
```

Detect the linkage family from the ASM before writing the pragmas — RACF
exits appear with both `SAVE (14,12)` and `BAKR R14,0`.  See
`docs/asm-linkage-conventions.md`.

### Work area

The ASM commonly does `GETMAIN R,LV=X01WKLEN`, zeroes it with `MVCL`, and
chains the save areas.  In Metal C, if the work area holds only local
variables, use stack variables instead and note the divergence — this is what
`converted/RACF/ICHPWX01.c` does.  Keep an explicit `storage_obtain` only
when the ASM passes the area to another service or keeps it across calls.

Map the storage macro faithfully — `GETMAIN` abends on failure,
`storage_obtain` returns NULL.  See `docs/system-services-catalog.md` §3.2.

---

## 4. Control Blocks

### 4.1 ACEE — Accessor Environment Element

`struct acee` in `metalc_racf.h`.  The ACEE describes the security
environment of the requesting user and is reachable from most exit parameter
lists (`rixacee`, `rcxacee`, `cnxacee`).

Attribute tests have helpers — use them rather than raw bit tests:

```c
if (racf_is_special(acee))    { /* SPECIAL    */ }
if (racf_is_operations(acee)) { /* OPERATIONS */ }
if (racf_is_auditor(acee))    { /* AUDITOR    */ }
if (racf_is_trusted(acee))    { /* TRUSTED    */ }
if (racf_is_privileged(acee)) { /* PRIVILEGED */ }
if (racf_is_protected_user(acee)) { /* no password possible */ }

char userid[8];
racf_get_userid(acee, userid);
```

`ASM: TM ACEEFLG1,ACEESPEC / BO …` maps to `if (racf_is_special(acee))`, not
to a raw `TM_ALL` on the flag byte.  The helper documents intent and survives
a field moving between flag bytes across RACF releases.

> **ACEE layout is version-sensitive.** `struct acee` in the header stops at
> +88 and is marked "additional fields vary by RACF version". Any field the
> ASM references beyond that must be verified against the installation's
> `IHAACEE` macro before the conversion is signed off. Record it as a concern
> in the verification matrix.

### 4.2 Exit parameter lists

All parameter lists follow the same shape: a pointer plus a separate length
byte for each variable-length item.

```c
/* ASM: L R4,PWXNEWPW  /  length byte at start of the field */
uint8_t *new_pw  = (uint8_t *)parm->pwxnpass + 1;
uint8_t  new_len = *(uint8_t *)parm->pwxnpass;
```

Note the two different conventions in play, and check which the ASM uses:

- a **length field in the parameter list** (`pwxnpasl` at +24), or
- a **length byte prefixing the data** at the pointer.

`converted/RACF/ICHPWX01.c` uses the length-byte-prefix form because that is
what the ASM does.  Getting this wrong reads the password one byte off and
silently changes every quality rule's behaviour.

Always null-check the pointer before dereferencing — the ASM typically tests
with `ICM Rn,B'1111',field / BZ`, which is a null check:

```c
/* ASM: ICM R0,B'1111',PWXNEWPW / BZ X01Z900 */
if (parm->pwxnpass == NULL) {
    return RACF_PWXRC_ACCEPT;
}
```

---

## 5. Common Patterns

### Pattern 1: Password quality (ICHPWX01)

The full worked conversion is `converted/RACF/ICHPWX01.c`, with the matrix at
`docs/verification-matrices/ICHPWX01_racf.md`.  The shape:

```c
#include "metalc_base.h"
#include "metalc_racf.h"

static const char PASSTAB[][8] = {
    "JAN     ", "FEB     ", /* ... */ "QWERTY  "
};
#define NUM_KEYWORDS (sizeof(PASSTAB) / sizeof(PASSTAB[0]))

#pragma prolog(ICHPWX01, "SAVE(14,12),LR(12,15)")
#pragma epilog(ICHPWX01, "RETURN(14,12)")

int ICHPWX01(struct racf_pwx_parm *parm) {
    uint8_t *new_pw;
    uint8_t  new_len;
    uint8_t *caller_flag;

    /* ASM: L R3,PWXCALLR / CLI 0(R3),PWXRINIT */
    caller_flag = (uint8_t *)parm->pwxcallr;
    if (caller_flag == NULL) return RACF_PWXRC_ACCEPT;

    /* Only police RACINIT and the PASSWORD command */
    if (*caller_flag != PWXRINIT && *caller_flag != PWXPWORD) {
        return RACF_PWXRC_ACCEPT;
    }

    if (parm->pwxnpass == NULL) return RACF_PWXRC_ACCEPT;
    new_pw  = (uint8_t *)parm->pwxnpass + 1;
    new_len = *(uint8_t *)parm->pwxnpass;

    /* ... quality rules ... */

    return RACF_PWXRC_ACCEPT;
}
```

**The caller check is not optional.**  `PWXCALLR` distinguishes RACINIT,
`PASSWORD`, and `ALTUSER`.  An exit that policices `ALTUSER` blocks
administrators from resetting a password — a support outage, not a security
improvement.  Preserve whatever set the ASM checked.

### Pattern 2: Authorization post-processing (ICHRCX02)

A post-exit audits; it does not decide.  `examples/racf_auth_logging_exit.c`
is the worked example.

```c
int ICHRCX02(struct racf_rcx_parm *parm) {
    struct acee *acee;
    char userid[8];

    if (parm == NULL || parm->rcxacee == NULL) {
        return RACF_RCXRC_CONTINUE;
    }
    acee = (struct acee *)parm->rcxacee;
    racf_get_userid(acee, userid);

    /* rcxreasn == 0 means RACF granted the request */
    if (parm->rcxreasn == 0) {
        /* log the grant */
    }

    return RACF_RCXRC_CONTINUE;   /* never 4 — that would alter the outcome */
}
```

Return `RACF_RCXRC_CONTINUE` from every path of a post-exit.  Returning 4
from `ICHRCX02` is meaningless at best and an authorization bypass at worst.

### Pattern 3: Sign-on restriction (ICHRIX01)

```c
int ICHRIX01(struct racf_rix_parm *parm) {
    int rc = RACF_RIXRC_CONTINUE;    /* neutral */

    if (parm == NULL) return RACF_RIXRC_CONTINUE;

    /* ASM: TM RIXFLG1,RIXLOGON / BNO … */
    if (!TM_ALL(parm->rixflg1, RIXFLG1_LOGON)) {
        return RACF_RIXRC_CONTINUE;  /* not a logon — not our business */
    }

    if (/* installation rule violated */ 0) {
        wto_security("ICHRIX01 Logon denied by installation policy", 42);
        rc = RACF_RIXRC_FAIL;        /* 8 — deny. NOT 4. */
    }

    return rc;
}
```

---

## 6. RACF-Specific Considerations

### 6.1 Never call RACROUTE from inside a RACF exit

`ICHRCX01` is called **by** `RACROUTE REQUEST=AUTH`.  Issuing another
RACROUTE from inside it re-enters SAF routing and calls the same exit again —
unbounded recursion, and an abend in the caller's address space.

The same applies to `ICHRIX01` and RACINIT.  If a conversion appears to need
an authorization check inside a RACF exit, that is a signal the ASM is doing
something unusual: stop and flag it rather than adding a `saf_auth()` call.

`metalc_saf.h` is for exits of **other** products that call SAF (IMS, CICS,
TSO sign-on exits).  It is not for RACF's own exits.  See
`docs/racroute-metalc-patterns.md` and `docs/exit-chaining.md` §3.2.

### 6.2 Password data is sensitive

- **Never** put a password, or any part of one, in a WTO, an SMF record, or
  a log buffer.  Message text is written to the console and the syslog in
  clear.  Log the userid and the rule that failed, never the value.
- Use `memcmp_secure()` from `metalc_base.h` for any comparison involving
  password material.  `memcmp_inline()` returns early on the first differing
  byte, which leaks length and prefix information through timing.
- Clear any local buffer that held password material before returning:

  ```c
  memset_inline(work_pw, 0, sizeof(work_pw));
  ```

- The exit sees the **new** password in clear on a change request.  That is
  the point of the exit, and it is also why the module must be in an
  APF-authorized, access-controlled library.

### 6.3 Password phrases break 8-byte assumptions

`PWXFLG1_PHRASE` (0x20) means the value is a password *phrase*, not a
password: much longer than 8 bytes, and mixed case with blanks permitted.

An ASM exit written before phrases existed will have hard-coded 8-byte logic.
Converting it faithfully preserves that limitation — which is correct — but
the conversion must **check the flag and pass phrases through untouched**
rather than applying 8-byte password rules to the first 8 bytes of a phrase:

```c
if (TM_ALL(parm->pwxflg1, PWXFLG1_PHRASE)) {
    return RACF_PWXRC_ACCEPT;   /* phrase rules are not this exit's job */
}
```

If the ASM has no phrase check, add one and record it as a divergence.  It is
a fail-safe addition: it accepts rather than rejects, and it stops the exit
from truncating a phrase into a rule it was never designed for.

### 6.4 Protected users and PassTickets have no password

`racf_is_protected_user()` (ACEEFLG3_PROT) and `ACEEFLG1_NOPW` mark users
that cannot authenticate with a password — started tasks, most commonly.
`ACEEFLG1_PASS` marks a PassTicket authentication.  Password-quality logic
must not run for these; check before applying rules.

### 6.5 Execution environment

RACF exits run in the caller's environment: frequently **supervisor state,
key 0, and possibly cross-memory or under a lock**.  Consequences for the
conversion:

- An abend in the exit abends the caller — potentially a system address
  space.  Null-check everything.
- Do not issue blocking services.  `WAIT`, `WTOR`, and unconditional `ENQ`
  are not safe here (`docs/system-services-catalog.md` §4.3).
- Keep the path short.  `ICHRCX01` runs on every authorization check on the
  system; a slow exit is a system-wide slowdown.
- Reentrancy is mandatory — no static writable data
  (`docs/reentrant-ification-policy.md`).

### 6.6 RACF release sensitivity

Control block layouts change across RACF releases.  Before sign-off, verify
against the installation's macros:

| C struct | RACF macro |
|----------|-----------|
| `struct acee` | `IHAACEE` |
| `struct racf_rix_parm` | RACINIT exit parameter list |
| `struct racf_rcx_parm` | RACROUTE AUTH exit parameter list |
| `struct racf_pwx_parm` | `ICHPWX` |

The header carries the standing warning: *"Control block layouts vary by RACF
version. Verify offsets against your installation's macros."*  Treat any
offset the conversion depends on as a matrix concern until confirmed.

---

## 7. RACF Exits vs. SAF Callers — Do Not Confuse Them

Two different things both involve RACF, and they convert differently:

| | RACF exit | SAF caller |
|---|---|---|
| Example | `ICHPWX01`, `ICHRCX02` | `DFSWHU00` (IMS), `DFHXSAD` (CICS) |
| Who calls it | RACF | The product |
| What it does | Influences or audits a RACF decision | Asks RACF for a decision |
| Header | `metalc_racf.h` | `metalc_saf.h` |
| Uses `saf_auth()`? | **No** — see §6.1 | Yes |
| Return code meaning | Per §2 — varies by exit | Product's own convention |

A module in `asm/RACF/` is the first kind.  A module in `asm/IMS/` or
`asm/CICS/` that codes `RACROUTE` is the second.

---

## 8. Build and Installation

### Compilation

```
xlc -qmetal -S -qlist -I./includes converted/RACF/ICHPWX01.c
as  -o ICHPWX01.o ICHPWX01.s
ld  -o ICHPWX01 ICHPWX01.o
```

No SAF stub is link-edited with a RACF exit (§6.1).

### Installation

RACF locates exits by module name, so the load module name must match the
exit name exactly — `ICHPWX01`, not a member alias.

- Link into `LPALIB` (or an LPA-eligible library) and refresh LPA, or IPL.
- Modules must be **reentrant and APF-authorized**.
- `ICHRIX01`/`ICHRCX01` take effect at the next IPL or LPA refresh; there is
  no dynamic activation comparable to JES2's `$T EXIT`.
- Test on a sandbox LPAR first.  A defective RACF exit can lock every user
  out of the system, including the administrators needed to remove it.

> **Have a backout plan before installing.** Know how to IPL without the exit
> — the exit module renamed, or an alternate LPA list — before the first
> install, not after the lockout.

---

## 9. Verification Checklist (RACF-Specific)

Beyond the standard checklist in `docs/verification-matrices/`:

- [ ] Exit identified as pre (`…X01`) or post (`…X02`); post-exit returns
      only the continue RC
- [ ] Return-code constants are the ones for **this** exit — RC=4 verified
      as bypass/allow (ICHRIX/ICHRCX/ICHRDX) or reject (ICHPWX/ICHNCV)
- [ ] No path returns `RACF_RIXRC_BYPASS` / `RACF_RCXRC_ALLOW` unless the
      ASM deliberately bypasses RACF at that point
- [ ] Return code variable initialised to the neutral 0
- [ ] No `saf_auth()` / RACROUTE call anywhere in the exit (§6.1)
- [ ] Every parameter-list pointer null-checked before dereference
- [ ] Password/phrase length taken from the correct source — parameter-list
      length field vs. length byte prefixing the data
- [ ] `PWXCALLR` caller check preserved exactly as the ASM had it
- [ ] Password phrase flag (`PWXFLG1_PHRASE`) handled, or the divergence
      recorded
- [ ] Protected users / PassTicket / no-password cases excluded from
      password rules
- [ ] `memcmp_secure()` used for all password comparisons
- [ ] No password material in any WTO, SMF record, or log buffer
- [ ] Local buffers holding password material cleared before return
- [ ] ACEE field offsets used by the exit verified against `IHAACEE` for the
      installed RACF release
- [ ] No static writable data; module is reentrant
- [ ] No blocking service (`WAIT`, `WTOR`, unconditional `ENQ`)
- [ ] Load module name matches the exit name exactly
- [ ] Backout plan documented before first install

---

## 10. Related Documents

- `includes/metalc_racf.h` — structs, constants, ACEE helpers
- `converted/RACF/ICHPWX01.c` — worked password-quality conversion
- `docs/verification-matrices/ICHPWX01_racf.md` — its matrix
- `examples/racf_auth_logging_exit.c` — ICHRCX02 post-exit example
- `docs/racroute-metalc-patterns.md` — for products that *call* SAF
- `docs/exit-chaining.md` §3.2 — RACF chain behaviour and the RC=4 hazard
- `docs/system-services-catalog.md` — system service macro lookup
- `docs/reentrant-ification-policy.md` — converting non-reentrant ASM
- `docs/asm-to-metalc-acf2.md` — the equivalent guide for CA ACF2
