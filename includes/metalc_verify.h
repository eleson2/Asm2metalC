/*********************************************************************
 * METALC_VERIFY.H - Compile-time struct layout verification
 *
 * Purpose:
 *   Provides macros that assert struct sizes and field offsets at
 *   compile time using the negative-array-size trick.  A compile
 *   error in this header means a real layout mismatch between the
 *   C struct definition and the underlying z/OS DSECT.
 *
 *   Compatible with xlc -qmetal (no _Static_assert required).
 *
 * Usage:
 *   #include "metalc_verify.h"
 *   VERIFY_SIZE(my_struct, 64);
 *   VERIFY_OFFSET(my_struct, my_field, 12);
 *
 * Invoked by: tests/verify_structs.c (compile-only; no main())
 *
 * Build:
 *   xlc -qmetal -S -qlist -I./includes tests/verify_structs.c
 *
 *   A clean compile = all sizes and offsets match the plan.
 *   Any error of the form "size of array ... is negative" pinpoints
 *   the failing assertion.
 *********************************************************************/

#ifndef METALC_VERIFY_H
#define METALC_VERIFY_H

#include <stddef.h>   /* offsetof */

/*-------------------------------------------------------------------
 * VERIFY_SIZE(type, expected_bytes)
 *   Fails to compile if sizeof(struct type) != expected_bytes.
 *   The typedef name encodes both the struct name and the expected
 *   size so the compiler error message is self-describing.
 *-------------------------------------------------------------------*/
#define VERIFY_SIZE(type, expected) \
    typedef char _chk_##type##_sz \
        [(sizeof(struct type) == (size_t)(expected)) ? 1 : -1]

/*-------------------------------------------------------------------
 * VERIFY_OFFSET(type, field, expected_offset)
 *   Fails to compile if offsetof(struct type, field) != expected_offset.
 *-------------------------------------------------------------------*/
#define VERIFY_OFFSET(type, field, expected) \
    typedef char _chk_##type##_##field \
        [(offsetof(struct type, field) == (size_t)(expected)) ? 1 : -1]

#endif /* METALC_VERIFY_H */
