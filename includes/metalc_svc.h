/*********************************************************************
 * METALC_SVC.H - z/OS system service layer for Metal C exits
 *
 * This header is the ONLY place in the header framework that contains
 * inline assembler.  Every z/OS system service (SVC or macro) used by
 * a converted exit is wrapped here as a C function so that exit source
 * stays pure C.
 *
 *   Invariant:  grep -rl __asm includes/ converted/ examples/
 *               must return exactly this file.
 *
 * The only other assembler in the repository lives in asm/stubs/ --
 * standalone HLASM stubs for services whose macro expansion is too
 * complex to inline (see includes/metalc_saf.h).
 *
 * Included automatically by metalc_base.h; do not include directly.
 *
 * Services provided:
 *   wto_write / wto_simple / wto_security / wto_alert / wto_important
 *                            - WTO (SVC 35)
 *   get_tod_clock / get_time_hundredths
 *                            - STCK hardware instruction
 *   get_tod_clock_extended   - STCKE hardware instruction
 *   packed_to_binary / binary_to_packed
 *                            - CVB / CVD hardware instructions
 *   getmain / freemain       - GETMAIN R / FREEMAIN R
 *   storage_obtain / storage_release
 *                            - STORAGE OBTAIN/RELEASE (conditional,
 *                              LOC=ANY; preferred over GETMAIN)
 *
 * NOTE ON MACRO ASSEMBLY: the GETMAIN/FREEMAIN/STORAGE macros are
 * expanded when the compiler-generated HLASM is assembled, so
 * SYS1.MACLIB must be in the SYSLIB concatenation of the assemble
 * step.  The hardware instructions (STCK, STCKE, CVB, CVD) and the
 * raw SVC forms need no macro library.
 *********************************************************************/

#ifndef METALC_SVC_H
#define METALC_SVC_H

#ifndef METALC_BASE_H
#error "metalc_svc.h is included by metalc_base.h; include that instead"
#endif

/*-------------------------------------------------------------------
 * METALC_ASM - the single inline-assembler entry point
 *
 * On z/OS this expands to __asm volatile(...) exactly as written.
 *
 * Defining METALC_HOST_LINT elides it, so the header set can be parsed
 * and the struct layouts checked by an ordinary host C compiler that
 * knows nothing about z/Architecture.  That is what tools/lint_host.sh
 * and the CI job do: they cannot validate the assembler, but they do
 * catch struct, type and syntax regressions on every commit instead of
 * on the next mainframe build.
 *
 * METALC_HOST_LINT MUST NOT be defined for a real build.  Every service
 * becomes a no-op returning its fail-safe value, so an exit built that
 * way issues no WTO, obtains no storage, and makes no SAF call.
 *-------------------------------------------------------------------*/
#ifdef METALC_HOST_LINT
#define METALC_ASM(...)   /* elided: host lint build, no z/Architecture */
#else
#define METALC_ASM(...)   __asm volatile(__VA_ARGS__)
#endif

/*-------------------------------------------------------------------
 * z/OS System Services - WTO (Write To Operator)
 *-------------------------------------------------------------------*/

/* WTO descriptor codes */
#define WTO_DESC_SYSTEM_FAILURE      0x8000  /* Descriptor 1  */
#define WTO_DESC_IMMEDIATE_ACTION    0x4000  /* Descriptor 2  */
#define WTO_DESC_EVENTUAL_ACTION     0x2000  /* Descriptor 3  */
#define WTO_DESC_SYSTEM_STATUS       0x1000  /* Descriptor 4  */
#define WTO_DESC_IMMEDIATE_COMMAND   0x0800  /* Descriptor 5  */
#define WTO_DESC_JOB_STATUS          0x0400  /* Descriptor 6  */
#define WTO_DESC_APPLICATION         0x0200  /* Descriptor 7  */
#define WTO_DESC_OUT_OF_LINE         0x0100  /* Descriptor 8  */
#define WTO_DESC_OPERATORS_REQUEST   0x0080  /* Descriptor 9  */
#define WTO_DESC_NOT_DEFINED_10      0x0040  /* Descriptor 10 */
#define WTO_DESC_CRITICAL_ACTION     0x0020  /* Descriptor 11 */
#define WTO_DESC_IMPORTANT_INFO      0x0010  /* Descriptor 12 */

/* WTO routing codes */
#define WTO_ROUTE_MASTER_CONSOLE     0x4000  /* Route code 2  */
#define WTO_ROUTE_TAPE_POOL          0x2000  /* Route code 3  */
#define WTO_ROUTE_DIRECT_ACCESS      0x1000  /* Route code 4  */
#define WTO_ROUTE_TAPE_LIBRARY       0x0800  /* Route code 5  */
#define WTO_ROUTE_DISK_LIBRARY       0x0400  /* Route code 6  */
#define WTO_ROUTE_UNIT_RECORD        0x0200  /* Route code 7  */
#define WTO_ROUTE_TELEPROCESSING     0x0100  /* Route code 8  */
#define WTO_ROUTE_SYSTEM_SECURITY    0x0080  /* Route code 9  */
#define WTO_ROUTE_SYSTEM_ERROR       0x0040  /* Route code 10 */
#define WTO_ROUTE_PROGRAMMER_INFO    0x0020  /* Route code 11 */

#pragma pack(1)

/**
 * WTO parameter list (standard list form, MCSFLAG=0)
 */
struct wto_parm {
    uint16_t       wto_len;          /* +0  Length: text_len + 4      */
    uint16_t       wto_mcsflags;     /* +2  MCS flags (0 for simple)  */
    char           wto_text[126];    /* +4  Message text (max 126)    */
    uint16_t       wto_desc;         /* Descriptor codes              */
    uint16_t       wto_route;        /* Routing codes                 */
};

#pragma pack()

/**
 * wto_write - Issue WTO to operator console
 * @msg:   Message text (EBCDIC)
 * @len:   Length of message (max 126)
 * @route: Routing codes (0 for default)
 * @desc:  Descriptor codes (0 for default)
 *
 * Returns: Return code from WTO (0 = success)
 *
 * Note: Message text must be in EBCDIC. In production, this would
 *       typically include a message ID prefix (e.g., "ABC001I ")
 *
 * REVIEW (unverified, pre-existing behaviour): the length halfword is
 * incremented by 4 when routing/descriptor codes are appended.  IBM
 * documents the first halfword as "text length + 4", with the route
 * and descriptor halfwords following the text and NOT counted.  This
 * is carried over unchanged from metalc_base.h; confirm against the
 * WTO macro expansion on the target z/OS level before relying on
 * routed messages.
 */
static inline int wto_write(const char *msg, int len,
                            uint16_t route, uint16_t desc) {
    struct wto_parm wto;
    int rc = -1;          /* fail-safe: not issued unless the SVC sets it */

    /* Limit message length */
    if (len > 126) len = 126;
    if (len < 1) return -1;

    /* Build WTO parameter list */
    wto.wto_len = (uint16_t)(len + 4);
    wto.wto_mcsflags = 0;

    /* Copy message text */
    for (int i = 0; i < len; i++) {
        wto.wto_text[i] = msg[i];
    }

    /* Append routing and descriptor codes after the text.
     * Stored a byte at a time: wto_text[len] is not guaranteed to be
     * halfword aligned, and STH requires alignment.                 */
    wto.wto_text[len]     = (char)(desc >> 8);
    wto.wto_text[len + 1] = (char)(desc & 0xFF);
    wto.wto_text[len + 2] = (char)(route >> 8);
    wto.wto_text[len + 3] = (char)(route & 0xFF);

    /* Adjust length to include routing/descriptor codes if specified */
    if (route != 0 || desc != 0) {
        wto.wto_len += 4;
    }

    /* Issue WTO - SVC 35.
     * R0 must be zero for a single-line WTO with no connect id.     */
    METALC_ASM(
        " SR    0,0          \n"  /* No connect id                  */
        " LA    1,%1         \n"  /* Load parm list address into R1 */
        " SVC   35           \n"  /* Issue WTO                      */
        " ST    15,%0        \n"  /* Store return code              */
        : "=m"(rc)
        : "m"(wto)
        : "0", "1", "14", "15"
    );

    return rc;
}

/**
 * wto_simple - Issue simple WTO message (no routing/descriptor codes)
 * @msg: Message text (EBCDIC)
 * @len: Length of message
 */
static inline int wto_simple(const char *msg, int len) {
    return wto_write(msg, len, 0, 0);
}

/**
 * wto_security - Issue security-related WTO message
 * @msg: Message text (EBCDIC)
 * @len: Length of message
 * Routes to security console (route code 9)
 */
static inline int wto_security(const char *msg, int len) {
    return wto_write(msg, len, WTO_ROUTE_SYSTEM_SECURITY,
                     WTO_DESC_SYSTEM_STATUS);
}

/**
 * wto_alert - Issue alert WTO requiring action
 * @msg: Message text (EBCDIC)
 * @len: Length of message
 * Routes to master console with action required
 */
static inline int wto_alert(const char *msg, int len) {
    return wto_write(msg, len, WTO_ROUTE_MASTER_CONSOLE,
                     WTO_DESC_EVENTUAL_ACTION);
}

/**
 * wto_important - Issue important informational WTO
 * @msg: Message text (EBCDIC)
 * @len: Length of message
 * Routes to master console with important-info descriptor
 */
static inline int wto_important(const char *msg, int len) {
    return wto_write(msg, len, WTO_ROUTE_MASTER_CONSOLE,
                     WTO_DESC_IMPORTANT_INFO);
}

/*-------------------------------------------------------------------
 * z/OS System Services - TIME macro
 *-------------------------------------------------------------------*/

/**
 * Get current time of day
 * @tod_clock: Output - 8-byte TOD clock value
 *
 * Uses STCK instruction to get current time
 */
static inline void get_tod_clock(uint64_t *tod_clock) {
    METALC_ASM(
        " STCK  %0           \n"  /* Store TOD clock */
        : "=m"(*tod_clock)
        :
        : "cc"
    );
}

/**
 * Get current time in seconds since midnight (approximate)
 * Returns time in hundredths of a second for SMF compatibility
 */
static inline uint32_t get_time_hundredths(void) {
    uint64_t tod;
    get_tod_clock(&tod);

    /* TOD is in microseconds since 1900-01-01
     * Extract time of day: shift and mask
     * TOD bit 51 = 1 microsecond, bits 32-51 = seconds portion
     * For simplicity, extract just the time portion
     */
    uint32_t secs = (uint32_t)((tod >> 12) % 86400000000ULL / 1000000);
    return secs * 100;  /* Return in hundredths */
}

/**
 * get_tod_clock_extended - Store extended TOD clock (STCKE)
 * @tod_extended: Output - 16-byte extended TOD clock value
 *
 * STCKE stores a 16-byte value: byte 0 is the epoch index, bytes 1-8
 * are the TOD clock proper (shifted one byte right relative to STCK),
 * and the remainder is a programmable field plus the CPU id.  It is
 * NOT interchangeable with the 8-byte STCK value - do not pass the
 * result to code expecting get_tod_clock() output.
 */
static inline void get_tod_clock_extended(uint8_t tod_extended[16]) {
    METALC_ASM(
        " STCKE %0           \n"  /* Store extended TOD clock */
        : "=m"(*(uint8_t (*)[16])tod_extended)
        :
        : "cc"
    );
}

/*-------------------------------------------------------------------
 * Packed decimal conversion (CVB / CVD)
 *
 * Metal C has no packed decimal arithmetic.  The supported approach is
 * to convert to binary at the edges, compute in C, and convert back:
 *
 *     int32_t n = packed_to_binary(field);
 *     n += 1;
 *     binary_to_packed(field, n);
 *
 * This replaces AP/SP/MP/DP/CP entirely.  Do not inline those in an
 * exit - see docs/complex-asm-patterns.md section 7.
 *-------------------------------------------------------------------*/

/**
 * packed_to_binary - Convert 8-byte packed decimal to binary (CVB)
 * @packed8: 8-byte packed decimal field (need not be aligned)
 *
 * Returns: the binary value.
 *
 * The field is copied to an aligned doubleword first, because CVB
 * requires doubleword alignment and a caller's field inside a mapped
 * control block usually is not aligned.
 *
 * CAUTION: CVB raises a data exception (S0C7) if the field does not
 * contain valid packed decimal with a valid sign nibble, and a
 * fixed-point divide exception (S0C9) if the value does not fit in 31
 * bits.  Validate untrusted input before calling, or run under a
 * recovery routine.  There is no return code to check.
 */
static inline int32_t packed_to_binary(const void *packed8) {
    uint64_t aligned;
    int32_t  result = 0;

    memcpy_inline(&aligned, packed8, 8);

    METALC_ASM(
        " CVB   2,%1         \n"  /* Packed decimal -> binary in R2 */
        " ST    2,%0         \n"
        : "=m"(result)
        : "m"(aligned)
        : "2"
    );

    return result;
}

/**
 * binary_to_packed - Convert binary to 8-byte packed decimal (CVD)
 * @packed8: 8-byte output field (need not be aligned)
 * @value:   binary value to convert
 *
 * CVD cannot fail: every 32-bit value fits in 8 bytes of packed
 * decimal.  The result carries a positive (0x0C) or negative (0x0D)
 * sign nibble.
 */
static inline void binary_to_packed(void *packed8, int32_t value) {
    uint64_t aligned = 0;

    METALC_ASM(
        " L     2,%1         \n"
        " CVD   2,%0         \n"  /* Binary -> packed decimal */
        : "=m"(aligned)
        : "m"(value)
        : "2"
    );

    memcpy_inline(packed8, &aligned, 8);
}

/*-------------------------------------------------------------------
 * z/OS System Services - GETMAIN/FREEMAIN and STORAGE
 *-------------------------------------------------------------------*/

/* Storage subpool constants */
#define SUBPOOL_JOB_STEP       0    /* Job step storage             */
#define SUBPOOL_LSQA          255   /* LSQA (system use)            */
#define SUBPOOL_CSA           241   /* Common storage area          */
#define SUBPOOL_SQA           245   /* System queue area            */

/**
 * getmain - Allocate storage
 * @size:    Number of bytes to allocate
 * @subpool: Subpool number (use SUBPOOL_JOB_STEP for exits)
 *
 * Returns: Pointer to allocated storage, or NULL on failure
 *
 * GETMAIN R passes the length in bits 8-31 of R0 and the subpool in
 * bits 0-7.  Note that GETMAIN R is UNCONDITIONAL: an unsatisfiable
 * request abends (S80A/S878) rather than returning a non-zero R15.
 * Use storage_obtain() when the exit must handle a failure itself.
 */
static inline void *getmain(uint32_t size, uint8_t subpool) {
    void *addr = NULL;
    int rc = 0;

    METALC_ASM(
        " L     0,%2         \n"  /* Length into R0 bits 8-31       */
        " ICM   0,8,%3       \n"  /* Subpool into R0 bits 0-7       */
        " GETMAIN R,LV=(0)   \n"  /* Issue GETMAIN                  */
        " ST    15,%1        \n"  /* Store return code              */
        " ST    1,%0         \n"  /* Store address                  */
        : "=m"(addr), "=m"(rc)
        : "m"(size), "m"(subpool)
        : "0", "1", "14", "15"
    );

    return (rc == 0) ? addr : NULL;
}

/**
 * freemain - Release storage
 * @addr:    Address of storage to release
 * @size:    Size of storage
 * @subpool: Subpool number (must match GETMAIN)
 *
 * Returns: 0 on success, non-zero on failure
 */
static inline int freemain(void *addr, uint32_t size, uint8_t subpool) {
    int rc = 0;

    if (addr == NULL) return 0;

    METALC_ASM(
        " L     0,%2         \n"  /* Length into R0 bits 8-31       */
        " ICM   0,8,%3       \n"  /* Subpool into R0 bits 0-7       */
        " L     1,%1         \n"  /* Storage address into R1        */
        " FREEMAIN R,LV=(0),A=(1) \n"
        " ST    15,%0        \n"  /* Store return code              */
        : "=m"(rc)
        : "m"(addr), "m"(size), "m"(subpool)
        : "0", "1", "14", "15"
    );

    return rc;
}

/**
 * storage_obtain - Allocate storage conditionally (STORAGE OBTAIN)
 * @size:    Number of bytes to allocate
 * @subpool: Subpool number (use SUBPOOL_JOB_STEP for exits)
 *
 * Returns: Pointer to allocated storage, or NULL if unavailable.
 *
 * This is the modern replacement for GETMAIN and the correct target
 * when the ASM source coded STORAGE OBTAIN.  COND=YES means an
 * unsatisfiable request sets R15=4 instead of abending, so the exit
 * can take a fail-safe path.  LOC=ANY allows storage above the line.
 */
static inline void *storage_obtain(uint32_t size, uint8_t subpool) {
    void *addr = NULL;
    uint32_t sp = subpool;
    int rc = 4;

    METALC_ASM(
        " L     2,%2         \n"  /* Length into R2                 */
        " L     3,%3         \n"  /* Subpool into R3 (low byte)     */
        " STORAGE OBTAIN,LENGTH=(2),ADDR=(4),SP=(3),LOC=ANY,COND=YES \n"
        " ST    15,%1        \n"  /* Store return code              */
        " ST    4,%0         \n"  /* Store obtained address         */
        : "=m"(addr), "=m"(rc)
        : "m"(size), "m"(sp)
        : "0", "1", "2", "3", "4", "14", "15"
    );

    return (rc == 0) ? addr : NULL;
}

/**
 * storage_release - Release storage obtained by storage_obtain()
 * @addr:    Address of storage to release
 * @size:    Size of storage (must match the obtain)
 * @subpool: Subpool number (must match the obtain)
 *
 * Returns: 0 on success, non-zero on failure
 */
static inline int storage_release(void *addr, uint32_t size, uint8_t subpool) {
    uint32_t sp = subpool;
    int rc = 0;

    if (addr == NULL) return 0;

    METALC_ASM(
        " L     2,%2         \n"  /* Length into R2                 */
        " L     3,%3         \n"  /* Subpool into R3 (low byte)     */
        " L     4,%1         \n"  /* Storage address into R4        */
        " STORAGE RELEASE,LENGTH=(2),ADDR=(4),SP=(3),COND=YES \n"
        " ST    15,%0        \n"  /* Store return code              */
        : "=m"(rc)
        : "m"(addr), "m"(size), "m"(sp)
        : "0", "1", "2", "3", "4", "14", "15"
    );

    return rc;
}

#endif /* METALC_SVC_H */
