/*********************************************************************
 * test_iefu83.c — Unit tests for IEFU83 (SMF Record Filtering Exit)
 *
 * Tests four cases from the verification matrix:
 *   1. Type 40 record          → SMF_RC_SUPPRESS (4)
 *   2. Type 42 record          → SMF_RC_SUPPRESS (4)
 *   3. Type 4, SYS* job name   → SMF_RC_SUPPRESS (4)
 *   4. Type 4, non-SYS job name→ SMF_RC_WRITE    (0)
 *
 * BUILD (on z/OS):
 *   xlc -qmetal -S -qlist -I../includes test_iefu83.c
 *   xlc -qmetal -S -qlist -I../includes ../converted/SMF/IEFU83.c
 *   # Assemble both generated .s files with HLASM
 *   # Link-edit with ENTRY test_main, include both object modules
 *   # Run via batch JCL
 *   # RC=0 → all four tests pass; RC=8 → at least one failure
 *   # Check SYSLOG for PASS:/FAIL: WTO messages
 *
 * SYSLOG expected output (all passing):
 *   PASS: type40 suppress
 *   PASS: type42 suppress
 *   PASS: type4 SYS-prefix suppress
 *   PASS: type4 non-SYS write
 *   TESTS:    4 P:    4 F:    0
 *********************************************************************/

#include "metalc_base.h"
#include "metalc_smf.h"

/*===================================================================
 * Forward declaration of the exit under test
 * (Defined in converted/SMF/IEFU83.c, linked in)
 *===================================================================*/
extern int IEFU83(void **parmlist);

/*===================================================================
 * Test counters
 *===================================================================*/
static int g_tests_run    = 0;
static int g_tests_passed = 0;
static int g_tests_failed = 0;

/*===================================================================
 * CHECK macro
 *===================================================================*/
#define CHECK(testname, expected_rc, actual_rc) \
    do { \
        g_tests_run++; \
        if ((actual_rc) == (expected_rc)) { \
            g_tests_passed++; \
            _check_pass(testname); \
        } else { \
            g_tests_failed++; \
            _check_fail(testname, (expected_rc), (actual_rc)); \
        } \
    } while (0)

static void _check_pass(const char *name) {
    char buf[80];
    int pos = 0;
    buf[pos++] = 'P'; buf[pos++] = 'A'; buf[pos++] = 'S';
    buf[pos++] = 'S'; buf[pos++] = ':'; buf[pos++] = ' ';
    const char *p = name;
    while (*p && pos < 78) buf[pos++] = *p++;
    wto_simple(buf, pos);
}

static void _check_fail(const char *name, int expected, int actual) {
    char buf[80];
    int pos = 0;
    buf[pos++] = 'F'; buf[pos++] = 'A'; buf[pos++] = 'I';
    buf[pos++] = 'L'; buf[pos++] = ':'; buf[pos++] = ' ';
    const char *p = name;
    while (*p && pos < 60) buf[pos++] = *p++;
    buf[pos++] = ' '; buf[pos++] = ' ';
    buf[pos++] = 'E'; buf[pos++] = 'X'; buf[pos++] = 'P'; buf[pos++] = '=';
    pos += format_int(buf + pos, expected, 4);
    buf[pos++] = ' ';
    buf[pos++] = 'G'; buf[pos++] = 'O'; buf[pos++] = 'T'; buf[pos++] = '=';
    pos += format_int(buf + pos, actual, 4);
    wto_important(buf, pos);
}

static void report_summary(void) {
    char buf[40];
    int pos = 0;
    buf[pos++] = 'T'; buf[pos++] = 'E'; buf[pos++] = 'S';
    buf[pos++] = 'T'; buf[pos++] = 'S'; buf[pos++] = ':'; buf[pos++] = ' ';
    pos += format_int(buf + pos, g_tests_run, 4);
    buf[pos++] = ' '; buf[pos++] = 'P'; buf[pos++] = ':'; buf[pos++] = ' ';
    pos += format_int(buf + pos, g_tests_passed, 4);
    buf[pos++] = ' '; buf[pos++] = 'F'; buf[pos++] = ':'; buf[pos++] = ' ';
    pos += format_int(buf + pos, g_tests_failed, 4);
    if (g_tests_failed == 0) {
        wto_simple(buf, pos);
    } else {
        wto_important(buf, pos);
    }
}

/*===================================================================
 * mock_build_smf_header — Construct a minimal SMF record header
 *
 * @buf:  Caller-supplied buffer (must be >= 26 bytes for type 4/5)
 * @type: SMF record type (smfrty at +5)
 *
 * Zeros the buffer first, then sets:
 *   smflen (+0) = buffer size estimate
 *   smfrty (+5) = type
 *   smfsid (+14)= "TEST" (4 bytes)
 *===================================================================*/
static void mock_build_smf_header(char *buf, int buflen, uint8_t type) {
    memset_inline(buf, 0, (size_t)buflen);
    /* smflen at +0 (uint16_t) */
    *(uint16_t *)&buf[0] = (uint16_t)buflen;
    /* smfrty at +5 */
    buf[5] = (char)type;
    /* smfsid at +14 — "TEST" */
    buf[14] = 'T'; buf[15] = 'E'; buf[16] = 'S'; buf[17] = 'T';
}

/*===================================================================
 * mock_build_smf4_with_jobname — Type 4 record with job name at +18
 *
 * @buf:     Buffer (must be >= 26 bytes)
 * @buflen:  Buffer length
 * @jobname: 8-byte job name (blank-padded)
 *===================================================================*/
static void mock_build_smf4_with_jobname(char *buf, int buflen,
                                          const char *jobname) {
    mock_build_smf_header(buf, buflen, SMF_TYPE_4);
    /* Job name at +18 (immediately after 18-byte smf_header) */
    for (int i = 0; i < 8; i++) {
        buf[18 + i] = jobname[i];
    }
}

/*===================================================================
 * test_main — ENTRY POINT
 *===================================================================*/
#pragma prolog(test_main, "SAVE(14,12),LR(12,15)")
#pragma epilog(test_main, "RETURN(14,12)")

int test_main(void) {
    /* Buffers large enough for each fake SMF record */
    char  rec40[32];
    char  rec42[32];
    char  rec4sys[32];
    char  rec4bat[32];

    void *parm40[1];
    void *parm42[1];
    void *parm4sys[1];
    void *parm4bat[1];

    int rc;

    /*------------------------------------------------------------------
     * Test 1: Type 40 record — must be suppressed
     *   SMF type 40 = Storage class memory; always suppress per IEFU83
     *------------------------------------------------------------------*/
    mock_build_smf_header(rec40, (int)sizeof(rec40), 40);
    parm40[0] = rec40;
    rc = IEFU83((void **)parm40);
    CHECK("type40 suppress", SMF_RC_SUPPRESS, rc);

    /*------------------------------------------------------------------
     * Test 2: Type 42 record — must be suppressed
     *   SMF type 42 = DFSMS statistics; always suppress per IEFU83
     *------------------------------------------------------------------*/
    mock_build_smf_header(rec42, (int)sizeof(rec42), 42);
    parm42[0] = rec42;
    rc = IEFU83((void **)parm42);
    CHECK("type42 suppress", SMF_RC_SUPPRESS, rc);

    /*------------------------------------------------------------------
     * Test 3: Type 4 with SYS-prefix job name — must be suppressed
     *   Job name "SYSPRINT" starts with "SYS" → suppress
     *------------------------------------------------------------------*/
    mock_build_smf4_with_jobname(rec4sys, (int)sizeof(rec4sys),
                                  "SYSPRINT");
    parm4sys[0] = rec4sys;
    rc = IEFU83((void **)parm4sys);
    CHECK("type4 SYS-prefix suppress", SMF_RC_SUPPRESS, rc);

    /*------------------------------------------------------------------
     * Test 4: Type 4 with non-SYS job name — must be written
     *   Job name "PAYROLL " does not start with "SYS" → write
     *------------------------------------------------------------------*/
    mock_build_smf4_with_jobname(rec4bat, (int)sizeof(rec4bat),
                                  "PAYROLL ");
    parm4bat[0] = rec4bat;
    rc = IEFU83((void **)parm4bat);
    CHECK("type4 non-SYS write", SMF_RC_WRITE, rc);

    report_summary();
    return (g_tests_failed == 0) ? 0 : 8;
}
