/*********************************************************************
 * METALC_BASE.H - Base definitions for Metal C exit programming
 * 
 * This header provides common utilities, type definitions, and
 * system control block mappings for IBM Metal C exit development.
 * 
 * Usage: #include "metalc_base.h"
 * 
 * Note: All structures use #pragma pack(1) for byte alignment
 *       matching mainframe DSECT layouts.
 *********************************************************************/

#ifndef METALC_BASE_H
#define METALC_BASE_H

/*-------------------------------------------------------------------
 * Standard type definitions for z/OS
 *-------------------------------------------------------------------*/

typedef unsigned char      uint8_t;
typedef signed char        int8_t;
typedef unsigned short     uint16_t;
typedef signed short       int16_t;
typedef unsigned int       uint32_t;
typedef signed int         int32_t;
typedef unsigned long long uint64_t;
typedef signed long long   int64_t;
typedef unsigned int       size_t;
typedef unsigned int       uintptr_t;

/* Pointer types for 31-bit and 64-bit modes */
#ifdef __LP64__
typedef uint64_t           ptr_t;
#else
typedef uint32_t           ptr_t;
#endif

/*-------------------------------------------------------------------
 * Common constants
 *-------------------------------------------------------------------*/

#define NULL               ((void *)0)
#define TRUE               1
#define FALSE              0

/*-------------------------------------------------------------------
 * Inline utility functions (no C library available in Metal C)
 *-------------------------------------------------------------------*/

/**
 * memcpy_inline - Copy memory
 * @dest: Destination pointer
 * @src:  Source pointer
 * @n:    Number of bytes to copy
 */
static inline void *memcpy_inline(void *dest, const void *src, size_t n) {
    unsigned char *d = (unsigned char *)dest;
    const unsigned char *s = (const unsigned char *)src;
    while (n--) {
        *d++ = *s++;
    }
    return dest;
}

/**
 * memset_inline - Fill memory with a constant byte
 * @s: Memory pointer
 * @c: Byte value to set
 * @n: Number of bytes to set
 */
static inline void *memset_inline(void *s, int c, size_t n) {
    unsigned char *p = (unsigned char *)s;
    while (n--) {
        *p++ = (unsigned char)c;
    }
    return s;
}

/**
 * memcmp_inline - Compare memory areas
 * @s1: First memory area
 * @s2: Second memory area
 * @n:  Number of bytes to compare
 * Returns: <0 if s1<s2, 0 if equal, >0 if s1>s2
 */
static inline int memcmp_inline(const void *s1, const void *s2, size_t n) {
    const unsigned char *p1 = (const unsigned char *)s1;
    const unsigned char *p2 = (const unsigned char *)s2;
    while (n--) {
        if (*p1 != *p2) {
            return *p1 - *p2;
        }
        p1++;
        p2++;
    }
    return 0;
}

/**
 * memcmp_secure - Constant-time memory comparison (timing-attack safe)
 * @s1: First memory area
 * @s2: Second memory area
 * @n:  Number of bytes to compare
 * Returns: 0 if equal, non-zero if different
 */
static inline int memcmp_secure(const void *s1, const void *s2, size_t n) {
    const unsigned char *p1 = (const unsigned char *)s1;
    const unsigned char *p2 = (const unsigned char *)s2;
    unsigned char result = 0;
    while (n--) {
        result |= *p1++ ^ *p2++;
    }
    return result;
}

/**
 * strlen_inline - Calculate string length
 * @s: Null-terminated string
 * Returns: Length not including null terminator
 */
static inline size_t strlen_inline(const char *s) {
    const char *p = s;
    while (*p) p++;
    return (size_t)(p - s);
}

/**
 * strncmp_inline - Compare strings up to n characters
 * @s1: First string
 * @s2: Second string
 * @n:  Maximum characters to compare
 * Returns: <0 if s1<s2, 0 if equal, >0 if s1>s2
 */
static inline int strncmp_inline(const char *s1, const char *s2, size_t n) {
    while (n && *s1 && (*s1 == *s2)) {
        s1++;
        s2++;
        n--;
    }
    if (n == 0) return 0;
    return *(unsigned char *)s1 - *(unsigned char *)s2;
}

/**
 * is_blank - Check if character array is all blanks
 * @s: Character array (not null-terminated)
 * @n: Length to check
 * Returns: 1 if all blanks, 0 otherwise
 */
static inline int is_blank(const char *s, size_t n) {
    while (n--) {
        if (*s++ != ' ') return 0;
    }
    return 1;
}

/**
 * pad_blank - Pad character array with blanks
 * @s:   Character array
 * @len: Current content length
 * @max: Total field size
 */
static inline void pad_blank(char *s, size_t len, size_t max) {
    while (len < max) {
        s[len++] = ' ';
    }
}

/**
 * match_field - Compare two fixed-width fields
 * @a: First field
 * @b: Second field
 * @n: Field width in bytes
 * Returns: 1 if equal, 0 otherwise
 */
static inline int match_field(const char *a, const char *b, size_t n) {
    return memcmp_inline(a, b, n) == 0;
}

/**
 * match_prefix - Check if field starts with a given prefix
 * @s:   Field to check
 * @pfx: Prefix to match
 * @len: Prefix length
 * Returns: 1 if matches, 0 otherwise
 */
static inline int match_prefix(const char *s, const char *pfx, size_t len) {
    return memcmp_inline(s, pfx, len) == 0;
}

/**
 * set_fixed_string - Copy string into fixed-width blank-padded field
 * @dest: Destination field
 * @src:  Source string (null-terminated)
 * @len:  Field width
 */
static inline void set_fixed_string(char *dest, const char *src, size_t len) {
    size_t i = 0;
    while (i < len && src[i]) { dest[i] = src[i]; i++; }
    while (i < len) { dest[i++] = ' '; }
}

/*-------------------------------------------------------------------
 * Password validation utilities
 *-------------------------------------------------------------------*/

/**
 * pwd_has_triple_repeat - Check for 3 consecutive identical characters
 * @pwd: Password field
 * @len: Password length
 * Returns: 1 if triple repeat found, 0 otherwise
 */
static inline int pwd_has_triple_repeat(const char *pwd, int len) {
    for (int i = 0; i < len - 2; i++) {
        if (pwd[i] == pwd[i + 1] && pwd[i] == pwd[i + 2]) {
            return 1;
        }
    }
    return 0;
}

/**
 * pwd_in_table - Check if password matches any entry in a bad-word table
 * @pwd:     Password field
 * @table:   Array of fixed-width entries
 * @width:   Entry width in bytes
 * @count:   Number of entries in table
 * Returns: 1 if match found, 0 otherwise
 */
static inline int pwd_in_table(const char *pwd, const char *table,
                                int width, int count) {
    for (int i = 0; i < count; i++) {
        if (memcmp_inline(pwd, table + (i * width), width) == 0) {
            return 1;
        }
    }
    return 0;
}

/*-------------------------------------------------------------------
 * Native C Exit Development Macros
 *-------------------------------------------------------------------*/

/* Entry/Exit Abstraction */
#define METALC_ENTRY(name)  _Pragma(linkage_string(name, "SAVE(14,12),LR(12,15)"))
#define METALC_EXIT(name)   _Pragma(linkage_string(name, "RETURN(14,12)"))

/* Return helpers */
#define RETURN_OK           return RC_OK
#define RETURN_REJECT       return RC_ERROR

/* Simplified WTO for literals */
#define WTO_LITERAL(msg)    wto_simple(msg, sizeof(msg)-1)

/*-------------------------------------------------------------------
 * Bit manipulation macros (Mapping to Assembler TM/OI/NI/XI)
 *-------------------------------------------------------------------*/

#define TM_ALL(val, bits)      (((val) & (bits)) == (bits))
#define TM_ANY(val, bits)      (((val) & (bits)) != 0)
#define TM_NONE(val, bits)     (((val) & (bits)) == 0)
#define TM_MIXED(val, bits)    (!TM_ALL(val, bits) && !TM_NONE(val, bits))

#define OI(val, bits)          ((val) |= (bits))
#define NI(val, bits)          ((val) &= ~(bits))
#define XI(val, bits)          ((val) ^= (bits))

/* Suppress unused parameter warnings */
#define UNUSED(x)              ((void)(x))

/* High-order bit for parameter list end marker (VL=1 pattern) */
#define IS_LAST_PARM(ptr)      ((uintptr_t)(ptr) & PARM_LAST_MASK)
#define GET_PARM_ADDR(ptr)     ((void *)((uintptr_t)(ptr) & ~PARM_LAST_MASK))
#define PARM_LAST_MASK         0x80000000

/*-------------------------------------------------------------------
 * Pointer/Offset Helpers (Mapping to Assembler base+offset)
 *-------------------------------------------------------------------*/

#define ADDR_AT_OFFSET(base, offset) ((void *)((char *)(base) + (offset)))
#define PTR_AT_OFFSET(base, offset)  (*(void **)ADDR_AT_OFFSET(base, offset))

/*-------------------------------------------------------------------
 * z/OS System Control Blocks
 *-------------------------------------------------------------------*/

#pragma pack(1)

/* PSA - Prefixed Save Area (low memory) */
#define PSA_BASE               0
#define FLCCVT                 0x10        /* Offset to CVT pointer */
#define PSATOLD                0x21C       /* Current TCB pointer   */
#define PSAAOLD                0x224       /* Current ASCB pointer  */

/* CVT - Communication Vector Table */
struct cvt {
    char           cvtfix[128];      /* +0   Fixed portion            */
    void          *cvttcbp;          /* +128 TCB/ASCB words pointer   */
    /* ... many fields omitted ... */
    char           cvtprodn[8];      /* +404 Product name             */
    char           cvtprodo[8];      /* +412 Product owner            */
    char           cvtprodv[8];      /* +420 Product version          */
    /* Note: Actual CVT is much larger; add fields as needed */
};

/* ASCB - Address Space Control Block (partial) */
struct ascb {
    char           ascbid[4];        /* +0   'ASCB'                   */
    void          *ascbfwdp;         /* +4   Forward chain            */
    void          *ascbbwdp;         /* +8   Backward chain           */
    uint16_t       ascbasid;         /* +36  ASID                     */
    char           ascbjbni[8];      /* +172 Job name (initiator)     */
    char           ascbjbns[8];      /* +180 Job name (started)       */
    /* Add fields as needed */
};

/* TCB - Task Control Block (partial) */
struct tcb {
    void          *tcbrbp;           /* +0   RB pointer               */
    void          *tcbpie;           /* +4   PIE pointer              */
    void          *tcbdeb;           /* +8   DEB pointer              */
    void          *tcbtio;           /* +12  TIOT pointer             */
    uint32_t       tcbcmp;           /* +16  Completion code          */
    /* Add fields as needed */
};

/* TIOT - Task I/O Table (partial) */
struct tiot {
    char           tiocnjob[8];      /* +0   Job name                 */
    char           tiocstep[8];      /* +8   Step name                */
    /* DD entries follow */
};

#pragma pack()

/*-------------------------------------------------------------------
 * System access macros
 *-------------------------------------------------------------------*/

/* Get CVT pointer */
#define GET_CVT()              (*(struct cvt **)FLCCVT)

/* Get current TCB pointer */
#define GET_TCB()              (*(struct tcb **)PSATOLD)

/* Get current ASCB pointer */
#define GET_ASCB()             (*(struct ascb **)PSAAOLD)

/* Get TCB/ASCB words from CVT */
static inline void **get_tcb_ascb_words(void) {
    struct cvt *cvt = GET_CVT();
    return (void **)((char *)cvt + 0);  /* CVTTCBP offset */
}

/*-------------------------------------------------------------------
 * Time and date utilities
 *-------------------------------------------------------------------*/

/* SMF time is in 0.01 second units since midnight */
#define SMF_TIME_HOUR(t)       ((t) / 360000)
#define SMF_TIME_MINUTE(t)     (((t) / 6000) % 60)
#define SMF_TIME_SECOND(t)     (((t) / 100) % 60)
#define SMF_TIME_HUNDREDTH(t)  ((t) % 100)

/* Build SMF time from components */
#define MAKE_SMF_TIME(h,m,s)   (((h)*360000) + ((m)*6000) + ((s)*100))

/**
 * parse_smf_date - Extract year and day from SMF packed date
 * @smfdte: 4-byte SMF date field (0cyydddF format)
 * @year:   Output year (1900-2099)
 * @day:    Output day of year (1-366)
 */
static inline void parse_smf_date(const uint8_t smfdte[4], 
                                   int *year, int *day) {
    int century = (smfdte[0] & 0x0F);
    int yy = ((smfdte[1] >> 4) * 10) + (smfdte[1] & 0x0F);
    int ddd = ((smfdte[2] >> 4) * 100) + 
              ((smfdte[2] & 0x0F) * 10) + 
              (smfdte[3] >> 4);
    
    *year = (century == 0 ? 1900 : 2000) + yy;
    *day = ddd;
}

/*-------------------------------------------------------------------
 * Return code constants (generic)
 *-------------------------------------------------------------------*/

#define RC_OK                  0
#define RC_WARNING             4
#define RC_ERROR               8
#define RC_SEVERE              12
#define RC_CRITICAL            16

/*-------------------------------------------------------------------
 * Common exit function codes (products using standard values)
 * Products with non-standard values keep their own definitions.
 *-------------------------------------------------------------------*/

#define EXIT_FUNC_INIT         0x01      /* Exit initialization       */
#define EXIT_FUNC_TERM         0x03      /* Exit termination          */

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
 * WTO Parameter List structure for single-line WTO
 * Build this on the stack and pass address to SVC 35
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
 */
static inline int wto_write(const char *msg, int len,
                            uint16_t route, uint16_t desc) {
    struct wto_parm wto;
    int rc;

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

    /* Add routing and descriptor codes after the text */
    /* They go immediately after the message text */
    *(uint16_t *)&wto.wto_text[len] = desc;
    *(uint16_t *)&wto.wto_text[len + 2] = route;

    /* Adjust length to include routing/descriptor codes if specified */
    if (route != 0 || desc != 0) {
        wto.wto_len += 4;
    }

    /* Issue WTO - SVC 35 */
    __asm(
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
    __asm(
        " STCK  %0           \n"  /* Store TOD clock */
        : "=m"(*tod_clock)
        :
        :
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

/*-------------------------------------------------------------------
 * z/OS System Services - GETMAIN/FREEMAIN
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
 * Note: For exit routines, storage should typically be obtained
 *       from the caller's subpool or a specific system subpool.
 */
static inline void *getmain(uint32_t size, uint8_t subpool) {
    void *addr = NULL;
    int rc;

    __asm(
        " LA    0,%2         \n"  /* Length in R0                   */
        " ICM   0,8,%3       \n"  /* Subpool in high byte of R0     */
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
    int rc;

    __asm(
        " LA    0,%2         \n"  /* Length in R0                   */
        " ICM   0,8,%3       \n"  /* Subpool in high byte of R0     */
        " LA    1,%1         \n"  /* Address in R1                  */
        " FREEMAIN R,LV=(0),A=(1) \n"
        " ST    15,%0        \n"  /* Store return code              */
        : "=m"(rc)
        : "m"(addr), "m"(size), "m"(subpool)
        : "0", "1", "14", "15"
    );

    return rc;
}

/*-------------------------------------------------------------------
 * Message formatting helpers
 *-------------------------------------------------------------------*/

/**
 * msg_append_str - Append null-terminated string to message buffer
 * @buf: Message buffer
 * @pos: Current position (updated on return)
 * @str: Null-terminated string to append
 */
static inline void msg_append_str(char *buf, int *pos, const char *str) {
    while (*str) { buf[(*pos)++] = *str++; }
}

/**
 * msg_append_field - Append fixed-width field to message buffer
 * @buf:   Message buffer
 * @pos:   Current position (updated on return)
 * @field: Fixed-width field
 * @len:   Field width
 */
static inline void msg_append_field(char *buf, int *pos,
                                     const char *field, int len) {
    for (int i = 0; i < len; i++) { buf[(*pos)++] = field[i]; }
}

/**
 * format_int - Format integer into character buffer (EBCDIC)
 * @buf:   Output buffer
 * @value: Integer value to format
 * @width: Field width (right-justified, space-filled)
 *
 * Returns: Number of characters written
 */
static inline int format_int(char *buf, int value, int width) {
    char temp[12];
    int i = 0;
    int neg = 0;
    unsigned int uval;

    if (value < 0) {
        neg = 1;
        uval = (unsigned int)(-value);
    } else {
        uval = (unsigned int)value;
    }

    /* Build digits in reverse */
    do {
        temp[i++] = (char)('0' + (uval % 10));  /* EBCDIC '0' = 0xF0 */
        uval /= 10;
    } while (uval > 0);

    if (neg) temp[i++] = '-';

    /* Pad with spaces */
    int j = 0;
    while (j < width - i) {
        buf[j++] = ' ';
    }

    /* Copy digits in correct order */
    while (i > 0) {
        buf[j++] = temp[--i];
    }

    return j;
}

/**
 * format_hex - Format integer as hex into buffer (EBCDIC)
 * @buf:   Output buffer
 * @value: Value to format
 * @width: Number of hex digits
 */
static inline void format_hex(char *buf, uint32_t value, int width) {
    static const char hexchars[] = "0123456789ABCDEF";
    for (int i = width - 1; i >= 0; i--) {
        buf[i] = hexchars[value & 0x0F];
        value >>= 4;
    }
}

/*-------------------------------------------------------------------
 * Debugging support
 *-------------------------------------------------------------------*/

#ifdef DEBUG_EXITS

#define DEBUG_WTO(msg) wto_simple(msg, sizeof(msg)-1)

#else

#define DEBUG_WTO(msg)

#endif /* DEBUG_EXITS */

/*-------------------------------------------------------------------
 * Common exit parameter header macro
 * Many exit parameter blocks share: work area, function code,
 * flags, and reserved bytes as their first fields.
 *-------------------------------------------------------------------*/

#define EXIT_PARM_HEADER       \
    void          *work;       /* +0  Work area pointer         */ \
    uint8_t        func;       /* +4  Function code             */ \
    uint8_t        flags;      /* +5  Flags                     */ \
    uint16_t       reserved    /* +6  Reserved                  */

/*-------------------------------------------------------------------
 * Save Area Layout
 *-------------------------------------------------------------------*/

/* Standard save area offsets */
#define SAVEAREA_BACK          4         /* +4  Back chain           */
#define SAVEAREA_FWD           8         /* +8  Forward chain        */
#define SAVEAREA_R14           12        /* +12 R14                  */
#define SAVEAREA_R15           16        /* +16 R15                  */
#define SAVEAREA_R0            20        /* +20 R0                   */
#define SAVEAREA_R1            24        /* +24 R1                   */
/* R2-R12 at +28 through +68 */

#endif /* METALC_BASE_H */
