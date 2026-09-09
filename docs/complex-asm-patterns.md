# Complex HLASM Patterns — Translation Reference

This document covers HLASM instruction sequences that have no obvious one-to-one C
equivalent.  Each section gives the pattern, its semantics, whether it is translatable
or must be flagged for manual review, and the canonical Metal C idiom.

---

## 1. EX (Execute) — Fixed-Target vs. Variable-Target

`EX Rn,label` executes the instruction at `label` with the length or mask field
modified by `Rn`.  The critical distinction is:

### 1.1 Fixed-Target EX — **Translatable**

The target address is a constant label in the source:

```asm
BCTR R1,0           ← R1 = length - 1 (for EX)
EX   R1,MVCINSTR    ← execute MVC with variable length
...
MVCINSTR MVC  DST(0),SRC   ← length 0 = placeholder, overridden by EX
```

This is a **sized memory copy with variable length**.  The pattern translates directly:

```c
memcpy_inline(dst, src, len);    /* len = original R1 value */
```

Similarly, `EX` over `TRT`, `CLC`, `MVC` with a fixed label is always translatable
to a C sized-operation.

**Detection**: The `EX` operand is a plain label (symbol) defined elsewhere in the
same source file, not a register or computed address.

### 1.2 Variable-Target EX — **Flag for manual review**

The target address is computed at runtime:

```asm
L    Rm,FUNCTAB(Rn)     ← load address from a jump table
EX   Rn,(Rm)            ← EX to computed address
```

or:

```asm
EX   Rn,0(Rm)           ← base+0 with Rm computed at runtime
```

This is a computed dispatch and cannot be mechanically translated.  Write:

```c
/* TODO: Manual review required — variable-target EX (computed dispatch) */
/* Source: EX Rn,(Rm) at label XXXXX */
/* Original: Rm loaded from table; dispatches to runtime-determined instruction */
```

**Detection**: The `EX` second operand is a register in parentheses `(Rm)` or a
base+offset where the base register is set by a prior `L Rm,...` from a table.

---

## 2. TRT (Translate and Test)

`TRT src(len),table` scans `src` for up to `len` bytes, using a 256-byte
translate table.  It stops at the first byte whose table entry is non-zero.
On return: R1 = address of stopping byte, R2 = table entry value, CC = 0 (all zero),
1 (partial — not all bytes processed), or 2 (all bytes processed).

### 2.1 Common usage: scan for delimiter / non-alpha

```asm
         TRT  FIELD(8),TRTABLE
         BZ   ALLALPHA           ← CC=0: all bytes mapped to 0 (all alphabetic)
         BC   8,NOTALL           ← CC=1: stopped early (found non-alpha)
```

The translate table `TRTABLE` has 0 for valid characters and non-zero for
delimiters or invalid characters.

### 2.2 C translation

There is no direct C equivalent for TRT over 256-byte tables on IBM platforms.
Use a simple scan loop:

```asm
TRTABLE  DC   256X'01'          ← default: flag everything
         ORG  TRTABLE+C'A'
         DC   26X'00'           ← A-Z: allow
         ORG  TRTABLE+C'a'
         DC   26X'00'           ← a-z: allow
         ORG                    ← reset ORG
```

```c
/* Equivalent of TRT FIELD(8),TRTABLE — scan for first non-alpha */
static const uint8_t trtable[256] = {
    /* populate: 0 for A-Z/a-z in EBCDIC, 1 elsewhere */
    /* ... (see metalc_base.h trt_alpha_table if defined) */
};

static int trt_scan(const uint8_t *src, int len, const uint8_t *table,
                    int *stop_offset) {
    int i;
    for (i = 0; i < len; i++) {
        if (table[src[i]] != 0) {
            if (stop_offset) *stop_offset = i;
            return 1;   /* CC=1: stopped early */
        }
    }
    return 0;           /* CC=0: all zero */
}
```

**Important**: In EBCDIC, 'A'=0xC1 and 'Z'=0xC9 / 'J'=0xD1 / 'S'=0xE2 etc.
EBCDIC alphabetic characters are not contiguous.  The literal `'A'` in a Metal C
source compiled for z/OS will be EBCDIC 0xC1, so this is handled automatically
when the translate table is defined as a `static const uint8_t` array with
character literals as indices.

The EX over TRT pattern is also fixed-target:
```asm
BCTR R1,0
EX   R1,TRTINST
...
TRTINST  TRT  0(*-*,R4),TRTABLE   ← length 0 overridden by EX
```

Translate as the scan loop above with `len = original R1`.

---

## 3. ICM (Insert Characters under Mask)

`ICM Rx,mask,field` inserts 1–4 bytes from `field` into specified byte positions
of register `Rx` and sets CC based on the result.

Common usage: load a 3-byte address and test for null:

```asm
ICM  R3,B'0111',PTRFIELD    ← load bytes 1,2,3 of R3 from PTRFIELD (3 bytes)
BZ   NULLPOINTER            ← CC=0: all inserted bytes were zero
```

C translation:
```c
uint32_t r3;
memcpy_inline(&r3, ptrfield, 3);   /* load 3 bytes into low 3 bytes */
r3 &= 0x00FFFFFF;                  /* mask high byte */
if (r3 == 0) goto null_pointer;
```

Or more idiomatically for a null pointer check:
```c
void *ptr = PTR_AT_OFFSET(base, offset);   /* read the 4-byte field as pointer */
if (ptr == NULL) ...
```

---

## 4. MVCL (Move Long)

`MVCL R1,R2` moves a variable-length block.  Register conventions:
- R1 = destination address
- R1+1 = destination length (31-bit) + padding byte (high byte)
- R2 = source address
- R2+1 = source length (31-bit)

If source length < destination length, the destination is padded with the pad byte
(from high byte of R1+1).

```asm
LA   R1,DEST
LA   R0,DESTLEN
STM  R0,R1,MVCLDEST   ← store pair
LA   R3,SRC
LA   R2,SRCLEN
STM  R2,R3,MVCLSRC    ← store pair
MVCL R0,R2            ← move with pad
```

C translation (no padding):
```c
memcpy_inline(dest, src, len);
```

C translation (with zero-padding):
```c
memcpy_inline(dest, src, src_len);
memset_inline((uint8_t *)dest + src_len, pad_byte, dest_len - src_len);
```

---

## 5. BCT / BCTR (Branch on Count)

`BCT Rx,label` decrements `Rx` and branches to `label` if the result is non-zero.
`BCTR Rx,0` decrements `Rx` without branching (used to compute `length - 1` before `EX`).

### 5.1 BCT as a loop counter

```asm
LA   R4,8           ← count = 8
LOOP EQU *
     ... body ...
     BCT  R4,LOOP
```

C:
```c
for (int i = 8; i > 0; i--) {
    /* body */
}
```

### 5.2 BCTR before EX

```asm
LA   R1,8           ← length = 8
BCTR R1,0           ← R1 = 7 (length - 1 for EX)
EX   R1,MVCINST
```

C: just use the full length:
```c
memcpy_inline(dst, src, 8);
```

---

## 6. BAS / BASR (Branch and Save Register) — Internal Subroutine

`BAS R14,label` branches to `label` and saves the return address in R14.
Used for calling internal subroutines within the same CSECT.

```asm
         BAS  R14,SUBRTN
         ...
SUBRTN   EQU  *
         ... subroutine body ...
         BR   R14            ← return
```

C translation: extract the subroutine into a `static` C function:

```c
static void subrtn(struct my_parm *parm) {
    /* subroutine body */
}
...
subrtn(parm);
```

**Important**: `BAS` does NOT push a linkage stack entry.  It is purely a
register-save branch.  Do not use `#pragma linkage` for internal subroutines —
just use a plain C function call.

---

## 7. Packed Decimal Instructions

Packed decimal (BCD) operations appear in exits that compute dates, amounts, or
check JES2 job-class codes.  There is no packed decimal arithmetic in Metal C.

**Convert at the edges, compute in binary.**  `packed_to_binary()` and
`binary_to_packed()` in `includes/metalc_svc.h` wrap CVB and CVD; everything
between them is ordinary C.  This replaces the whole AP/SP/MP/DP/CP family.

```asm
         AP    COUNTER,=P'1'          increment a packed counter
```

```c
int32_t n = packed_to_binary(counter);
n += 1;
binary_to_packed(counter, n);
```

| Instruction | Operation | C approach |
|---|---|---|
| `CVB Rx,field` | Packed decimal → binary | `packed_to_binary(field)` |
| `CVD Rx,field` | Binary → packed decimal | `binary_to_packed(field, value)` |
| `AP`, `SP`, `MP`, `DP` | Packed arithmetic | Convert to binary, compute in C, convert back |
| `CP` | Packed compare | Compare the binary values |
| `ZAP dst,src` | Zero-and-add (copy) | `binary_to_packed(dst, packed_to_binary(src))`, or `memcpy_inline` if the field is opaque |
| `PACK` / `UNPK` | Zoned ↔ packed | No wrapper yet — ADD WRAPPER per the services catalog |
| `ED` / `EDMK` | Edit for display | No wrapper yet; usually replaceable with `format_int()` |

Do **not** inline `__asm` for these in an exit.  A wrapper that is missing gets
added to `metalc_svc.h` following `docs/system-services-catalog.md` §4.

**CVB raises exceptions.**  A field that is not valid packed decimal with a
valid sign nibble causes S0C7, and a value too large for 31 bits causes S0C9.
There is no return code.  Validate untrusted input before converting, or run
under a recovery routine.  This is a real risk in exits that read packed fields
out of user-supplied JCL or control blocks.

---

## 8. STCK / STCKE (Store Clock)

`STCK field` stores the 8-byte TOD clock value to a doubleword-aligned field.

```asm
STCK TODFIELD
```

C:
```c
uint64_t tod;
get_tod_clock(&tod);
```

`STCKE` stores an extended 16-byte TOD clock:
```c
uint8_t tod_extended[16];
get_tod_clock_extended(tod_extended);
```

The two are not interchangeable: STCKE's byte 0 is an epoch index and its
clock field is shifted one byte right relative to STCK.  Match whichever the
ASM used.

Neither is the same as the `TIME` macro, which returns formatted date and
time — see `docs/system-services-catalog.md` §3.3.

---

## 9. Multiple USING (Multiple Base Registers)

Some exits establish multiple USINGs for different DSECTs simultaneously:

```asm
USING MYEXIT,R12    ← CSECT base
USING JCT,R10       ← JCT DSECT base
USING EXIT02W,R9    ← work area DSECT base
```

In Metal C this is handled by typed pointers:
```c
struct jct       *jct    = (struct jct *)r10;
struct exit02_work *work = (struct exit02_work *)r9;
```

All field accesses that the assembler resolves via the DSECT are replaced by
`ptr->field` accesses in C.

---

## 10. DROP / ORG

`DROP Rx` cancels a USING.  In C, simply stop using that pointer.

`ORG label` repositions the assembler location counter — used to define overlapping
or alternative views of storage.  In C, use a `union` or cast to the alternative
struct type.

---

## 11. Summary Table — Translatability

| Pattern | Translatable? | C approach |
|---------|---------------|------------|
| `EX Rn,fixed_label` | Yes | Sized memcpy / memset / scan |
| `EX Rn,(Rm)` | No — flag | Manual review comment |
| `TRT src(n),table` | Yes | Inline scan loop + static table |
| `ICM Rx,mask,field` | Yes | memcpy + mask, or typed pointer |
| `MVCL R1,R2` | Yes | memcpy + optional memset pad |
| `BCT Rx,label` | Yes | for loop |
| `BCTR Rx,0` | Yes | length-1 optimization (usually drop) |
| `BAS R14,sub` | Yes | static C function |
| `BAKR R14,0` + `PR` | Yes | `#pragma prolog "BAKR 14,0"` |
| `STCK field` | Yes | `get_tod_clock(&tod)` |
| `STCKE field` | Yes | `get_tod_clock_extended(buf)` |
| `CVB`/`CVD` | Yes | `packed_to_binary()` / `binary_to_packed()` |
| `AP`/`SP`/`MP`/`DP` | Yes | convert to binary, compute in C, convert back |
| Self-modifying code | No — flag | Manual review |
| PC / PT instructions | No — flag | Manual review |
| AR-mode (LAM/EAR/SAR) | No — flag | Manual review |

---

## 12. Related Documents

- `docs/hlasm-structured-programming.md` — IF/DO/SELECT macros
- `docs/asm-linkage-conventions.md` — BAKR/PR pragma handling
- `docs/asm-to-metalc-general.md` — basic instruction mapping
- `docs/asm-to-c-conversion-guide.md` — DSECT-to-struct mapping
