/*********************************************************************
 * test_harness_template.c — Metal C Unit Test Harness Template
 *
 * PURPOSE:
 *   Template for writing per-exit unit tests in Metal C.
 *   Copy this file to test_<MODULE>.c and fill in the test cases.
 *
 * DESIGN:
 *   - No Language Environment (LE) — runs on the bare metal z/OS kernel
 *   - No malloc/printf/stdlib; uses wto_simple/wto_important for output
 *   - Tracks pass/fail counts with static counters (single-threaded)
 *   - format_int() from metalc_base.h produces all numeric output
 *   - test_main() is the ENTRY point, driven by JCL
 *   - Returns 0 if all pass, 8 if any fail
 *
 * BUILD (on z/OS):
 *   xlc -qmetal -S -qlist -I../includes test_<MODULE>.c
 *   xlc -qmetal -S -qlist -I../includes ../converted/<PROD>/<MODULE>.c
 *   # Assemble both .s files with HLASM
 *   # Link-edit with ENTRY test_main
 *   # Run via batch JCL; check SYSLOG for PASS:/FAIL: messages
 *   # JCL step RC=0 → all pass; RC=8 → at least one failure
 *
 * HOW TO USE THIS TEMPLATE:
 *   1. Copy to tests/test_MYMODULE.c
 *   2. Replace <MODULE> placeholders throughout
 *   3. Implement mock_build_<type>() helpers for your test records
 *   4. Add CHECK() calls in test_main()
 *
 * HARNESS OVERHEAD: < 100 bytes of stack.  Safe for any z/OS TCB.
 *********************************************************************/

#include "metalc_base.h"
/* #include "metalc_<product>.h" */   /* Add product header here */

/*===================================================================
 * Test counters — static (read-only at program start; written only
 * within this single-threaded test program)
 *===================================================================*/
static int g_tests_run    = 0;
static int g_tests_passed = 0;
static int g_tests_failed = 0;

/*===================================================================
 * CHECK macro
 *
 * Usage:
 *   CHECK("type40 suppress", SMF_RC_SUPPRESS, actual_rc);
 *
 * Emits:
 *   PASS: type40 suppress
 *   FAIL: type40 suppress  EXP=4 GOT=0
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

/*-------------------------------------------------------------------
 * Internal helpers — not called directly by test code
 *-------------------------------------------------------------------*/
static void _check_pass(const char *name) {
    /* Build: "PASS: <name>" — fixed prefix + name */
    char buf[80];
    int pos = 0;
    /* PASS: */
    buf[pos++] = 'P'; buf[pos++] = 'A'; buf[pos++] = 'S';
    buf[pos++] = 'S'; buf[pos++] = ':'; buf[pos++] = ' ';
    /* append test name */
    const char *p = name;
    while (*p && pos < 78) buf[pos++] = *p++;
    wto_simple(buf, pos);
}

static void _check_fail(const char *name, int expected, int actual) {
    /* Build: "FAIL: <name>  EXP=<n> GOT=<n>" */
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

/*===================================================================
 * report_summary — emit final test counts via WTO
 *
 * Format: "TESTS: nnn P: nnn F: nnn"
 *===================================================================*/
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
 * test_main — ENTRY POINT
 *
 * Add CHECK() calls below.  Return 0 = all pass; 8 = any fail.
 *===================================================================*/
#pragma prolog(test_main, "SAVE(14,12),LR(12,15)")
#pragma epilog(test_main, "RETURN(14,12)")

int test_main(void) {

    /* -------------------------------------------------------------- */
    /* INSERT TEST CASES HERE                                          */
    /*   Example:                                                      */
    /*   int rc = MY_EXIT(&my_parm);                                   */
    /*   CHECK("case description", EXPECTED_RC, rc);                  */
    /* -------------------------------------------------------------- */

    report_summary();
    return (g_tests_failed == 0) ? 0 : 8;
}
