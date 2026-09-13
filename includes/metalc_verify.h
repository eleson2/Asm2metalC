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
 *   #include "metalc_base.h"      (required first)
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

/*
 * Include metalc_base.h first - this header uses its size_t.
 *
 * <stddef.h> is deliberately NOT included.  It is a freestanding
 * header and so legal in Metal C, but it defines its own size_t,
 * which collides with the one in metalc_base.h.  In 31-bit mode the
 * two happen to agree; under -q64 the compiler's is 8 bytes and the
 * collision is a hard error - which would break this file on exactly
 * the AMODE 64 modules docs/amode64-exits.md tells you to build that
 * way.  offsetof is defined below instead.
 */
#ifndef METALC_BASE_H
#error "include metalc_base.h before metalc_verify.h"
#endif

#ifndef offsetof
#define offsetof(type, member)  ((size_t)&(((type *)0)->member))
#endif

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

/*-------------------------------------------------------------------
 * VERIFY_FIELD_SIZE(type, field, expected_bytes)
 *   Fails to compile if sizeof the field != expected_bytes.
 *
 *   Offsets alone do not catch a field declared the wrong WIDTH, and
 *   width is what layout-findings.md finding 5 turned on: jct.jctjobid
 *   was uint16_t where the assembler reads 8 bytes
 *   (HASPEX02.asm: MVC MSGJOBID,JCTJOBID with MSGJOBID DS CL8).  A
 *   wrong width also silently shifts every later offset, so assert it
 *   wherever an instruction pins a length:
 *
 *     CLC  8(8,R10),...   ->  VERIFY_FIELD_SIZE(x, authid, 8)
 *     CLI  4(R10),...     ->  VERIFY_FIELD_SIZE(x, func,   1)
 *     MVC  60(4,R10),...  ->  VERIFY_FIELD_SIZE(x, reasn,  4)
 *-------------------------------------------------------------------*/
#define VERIFY_FIELD_SIZE(type, field, expected) \
    typedef char _chk_##type##_##field##_w \
        [(sizeof(((struct type *)0)->field) == (size_t)(expected)) ? 1 : -1]

#endif /* METALC_VERIFY_H */
