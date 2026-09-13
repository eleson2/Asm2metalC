/*********************************************************************
 * test_haspex20.c — Unit tests for HASPEX20 / EXIT20 (JES2 Exit 20)
 *
 * Every test asserts a numbered statement from the behavioural spec,
 * docs/specs/HASPEX20_jes2.md — NOT a line of the C conversion.  The
 * spec is derived from the ASM prologue and instruction stream; this
 * file is derived from the spec.
 *
 *   T1  S1,S3  batch job JOB00123      → jctmclas == 'E'
 *   T2  S2,S3  XJOB0001 (J not byte 0) → jctmclas unchanged
 *   T3  S2,S3  started task STC00042   → jctmclas unchanged
 *   T4  S2,S3  TSO user TSU00007       → jctmclas unchanged
 *   T5  S4     no JCT field but jctmclas is modified
 *   T6  S5     input code 4 behaves as input code 0
 *
 * WHAT T1 CATCHES.  The original conversion declared jctjobid as
 * uint16_t and tested `jct->jctjobid == 'J'`, i.e. halfword == 0x00D1.
 * Against real storage the first two bytes of a CL8 job ID are letters
 * — 'JO' is 0xD1D6, 'ST' is 0xE2E3, 'TS' is 0xE3E2 — so the halfword
 * was NEVER 0x00D1 and the branch was taken for no job at all.  T1
 * fails outright on the old code.  (docs/layout-findings.md finding 5
 * described this as "true only for job number 209", which holds only
 * under the old header's fiction that the field was a 2-byte binary
 * job number.  Against the real CL8 field it was never true.)
 *
 * WHAT T2 CATCHES.  `CLI` compares exactly ONE byte, so the test is on
 * byte 0 and nowhere else.  'XJOB0001' contains 'J' but not in byte 0,
 * and must be left alone.  This fails any "does the job ID contain J"
 * or whole-field comparison.  It is the test that keeps a future fix
 * honest about the width of the compare.
 *
 * BUILD (on z/OS):
 *   xlc -qmetal -S -qlist -I../includes test_haspex20.c
 *   xlc -qmetal -S -qlist -I../includes ../converted/JES2/HASPEX20.c
 *   # Assemble both generated .s files with HLASM
 *   # Link-edit with ENTRY test_main, include both object modules
 *   # Run via batch JCL
 *   # RC=0 → all tests pass; RC=8 → at least one failure
 *   # Check SYSLOG for PASS:/FAIL: WTO messages
 *
 * SYSLOG expected output (all passing):
 *   PASS: S1 batch JOB00123 forced to E
 *   PASS: S2 XJOB0001 untouched (J not in byte 0)
 *   PASS: S2 started task STC00042 untouched
 *   PASS: S2 TSO user TSU00007 untouched
 *   PASS: S4 only jctmclas modified
 *   PASS: S5 JECL error code behaves as normal
 *   TESTS:    6 P:    6 F:    0
 *********************************************************************/

#include "metalc_base.h"
#include "metalc_jes2.h"
#include "metalc_verify.h"

/*===================================================================
 * Layout precondition
 *
 * Every statement below assumes JCTJOBID is the CL8 field the
 * assembler proves it is (HASPEX02.asm: MVC MSGJOBID,JCTJOBID with
 * MSGJOBID DS CL8).  If it is ever narrowed again, this fails at
 * compile time rather than letting the tests quietly re-pass against a
 * byte-0 cast of a 2-byte field.
 *===================================================================*/
VERIFY_FIELD_SIZE(jct, jctjobid, 8);

/*===================================================================
 * Exit under test (defined in converted/JES2/HASPEX20.c, linked in)
 *
 * EXIT20(input_code, reserved, jct):
 *   input_code = R0  (0 = normal end of input, 4 = JECL error)
 *   reserved         (R1-R9 are N/A at this exit point)
 *   jct        = R10
 *===================================================================*/
extern int EXIT20(int input_code, void *reserved, struct jct *jct);

/* Input codes from the ASM prologue */
#define EX20_CODE_NORMAL     0
#define EX20_CODE_JECL_ERR   4

/* The message class the exit must install, and a distinct value to
   pre-load so "unchanged" is distinguishable from "set to E". */
#define EXPECTED_MSGCLASS    'E'
#define SENTINEL_MSGCLASS    'A'

/*===================================================================
 * Test counters
 *===================================================================*/
static int g_tests_run    = 0;
static int g_tests_passed = 0;
static int g_tests_failed = 0;

static void report_pass(const char *name) {
    char buf[80];
    int  pos = 0;
    msg_append_str(buf, &pos, "PASS: ");
    msg_append_str(buf, &pos, name);
    wto_simple(buf, pos);
}

static void report_fail(const char *name) {
    char buf[80];
    int  pos = 0;
    msg_append_str(buf, &pos, "FAIL: ");
    msg_append_str(buf, &pos, name);
    wto_simple(buf, pos);
}

/*===================================================================
 * CHECK — assert a boolean condition holds
 *===================================================================*/
#define CHECK(testname, condition) \
    do { \
        g_tests_run++; \
        if (condition) { \
            g_tests_passed++; \
            report_pass(testname); \
        } else { \
            g_tests_failed++; \
            report_fail(testname); \
        } \
    } while (0)

/*===================================================================
 * mock_build_jct — Minimal JCT with a known job ID
 *
 * @jct:   Caller-supplied struct, zeroed here
 * @jobid: 8 characters, e.g. "JOB00123" (no terminator)
 *
 * Sets the 'JCT ' eyecatcher, the CL8 job ID, a recognisable job name,
 * and a sentinel message class so "left unchanged" is observable.
 *===================================================================*/
static void mock_build_jct(struct jct *jct, const char *jobid) {
    int i;

    memset_inline(jct, 0, sizeof(struct jct));

    jct->jctid[0] = 'J';
    jct->jctid[1] = 'C';
    jct->jctid[2] = 'T';
    jct->jctid[3] = ' ';

    for (i = 0; i < 8; i++) {
        jct->jctjobid[i] = jobid[i];
    }
    for (i = 0; i < 8; i++) {
        jct->jctjname[i] = "TESTJOB "[i];
    }

    jct->jctmclas = SENTINEL_MSGCLASS;
    jct->jctjclas = 'A';
    jct->jctprio  = 9;
}

/*===================================================================
 * jct_differs_outside_msgclass — S4 support
 *
 * Byte-compares two JCTs, skipping the one byte jctmclas occupies.
 * Returns 1 if any other byte differs.
 *===================================================================*/
static int jct_differs_outside_msgclass(const struct jct *a,
                                        const struct jct *b) {
    const unsigned char *pa = (const unsigned char *)a;
    const unsigned char *pb = (const unsigned char *)b;
    size_t mclas = (size_t)((const char *)&a->jctmclas - (const char *)a);
    size_t i;

    for (i = 0; i < sizeof(struct jct); i++) {
        if (i == mclas) {
            continue;
        }
        if (pa[i] != pb[i]) {
            return 1;
        }
    }
    return 0;
}

static void report_summary(void) {
    char buf[80];
    int  pos = 0;
    msg_append_str(buf, &pos, "TESTS: ");
    pos += format_int(buf + pos, g_tests_run, 5);
    msg_append_str(buf, &pos, " P: ");
    pos += format_int(buf + pos, g_tests_passed, 5);
    msg_append_str(buf, &pos, " F: ");
    pos += format_int(buf + pos, g_tests_failed, 5);
    wto_simple(buf, pos);
}

/*===================================================================
 * test_main — ENTRY POINT
 *===================================================================*/
#pragma prolog(test_main, "SAVE(14,12),LR(12,15)")
#pragma epilog(test_main, "RETURN(14,12)")

int test_main(void) {
    struct jct jct;
    struct jct before;
    int rc;

    /*------------------------------------------------------------------
     * T1 — S1, S3.  Ordinary batch job.
     *   'JOB00123' begins with 'J', so the message class must become E.
     *------------------------------------------------------------------*/
    mock_build_jct(&jct, "JOB00123");
    rc = EXIT20(EX20_CODE_NORMAL, NULL, &jct);
    CHECK("S1 batch JOB00123 forced to E",
          jct.jctmclas == EXPECTED_MSGCLASS && rc == JES2_RC_CONTINUE);

    /*------------------------------------------------------------------
     * T2 — S2, S3.  'J' present but not in byte 0: must be left alone.
     *   CLI tests one byte.  Catches a comparison widened to the whole
     *   field, or a substring search.
     *------------------------------------------------------------------*/
    mock_build_jct(&jct, "XJOB0001");
    rc = EXIT20(EX20_CODE_NORMAL, NULL, &jct);
    CHECK("S2 XJOB0001 untouched (J not in byte 0)",
          jct.jctmclas == SENTINEL_MSGCLASS && rc == JES2_RC_CONTINUE);

    /*------------------------------------------------------------------
     * T3 — S2, S3.  Started task: must be left alone.
     *------------------------------------------------------------------*/
    mock_build_jct(&jct, "STC00042");
    rc = EXIT20(EX20_CODE_NORMAL, NULL, &jct);
    CHECK("S2 started task STC00042 untouched",
          jct.jctmclas == SENTINEL_MSGCLASS && rc == JES2_RC_CONTINUE);

    /*------------------------------------------------------------------
     * T4 — S2, S3.  TSO user: must be left alone.
     *------------------------------------------------------------------*/
    mock_build_jct(&jct, "TSU00007");
    rc = EXIT20(EX20_CODE_NORMAL, NULL, &jct);
    CHECK("S2 TSO user TSU00007 untouched",
          jct.jctmclas == SENTINEL_MSGCLASS && rc == JES2_RC_CONTINUE);

    /*------------------------------------------------------------------
     * T5 — S4.  Only jctmclas may change.
     *   Catches a misplaced struct field writing 'E' somewhere else,
     *   which S1 alone could miss.
     *------------------------------------------------------------------*/
    mock_build_jct(&jct, "JOB00123");
    memcpy_inline(&before, &jct, sizeof(struct jct));
    rc = EXIT20(EX20_CODE_NORMAL, NULL, &jct);
    CHECK("S4 only jctmclas modified",
          jct.jctmclas == EXPECTED_MSGCLASS
          && !jct_differs_outside_msgclass(&before, &jct)
          && rc == JES2_RC_CONTINUE);

    /*------------------------------------------------------------------
     * T6 — S5.  A JECL error must not change behaviour.
     *   The ASM never tests R0; adding such a test would be a
     *   behaviour change, so assert the absence.
     *------------------------------------------------------------------*/
    mock_build_jct(&jct, "JOB00123");
    rc = EXIT20(EX20_CODE_JECL_ERR, NULL, &jct);
    CHECK("S5 JECL error code behaves as normal",
          jct.jctmclas == EXPECTED_MSGCLASS && rc == JES2_RC_CONTINUE);

    report_summary();
    return (g_tests_failed == 0) ? 0 : 8;
}
