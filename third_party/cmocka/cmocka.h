/**
 * @file cmocka.h
 * @brief Minimal CMocka compatibility layer for native builds
 *
 * Provides CMocka-compatible assertion macros and test runner.
 * Uses longjmp-based failure handling (compatible with both int-return
 * and void-return test function signatures).
 *
 * (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
 */
#ifndef CMOCKA_H
#define CMOCKA_H

#include <stddef.h>
#include <stdint.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ─── State for longjmp-based assertion failures ────────────────────── */
static jmp_buf cmocka_assert_jmp_buf;
static int     cmocka_assert_jmp_ready = 0;

/* ─── State type for test runner ────────────────────────────────────── */
struct CMUnitTest {
    const char *name;
    int (*test_func)(void **state);
    int (*setup_func)(void **state);
    int (*teardown_func)(void **state);
};

/* ─── Mock value tracking (stubs) ───────────────────────────────────── */
#define will_return(func, value)       ((void)(func), (void)(value))
#define mock()                         0
#define mock_type(type)                ((type)0)
#define mock_ptr_type(type)            ((type)(intptr_t)0)

/* ─── Internal: longjmp-based assertion helper ──────────────────────── */
#define CMOCKA_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            fprintf(stderr, "FAIL: %s:%d: %s\n", __FILE__, __LINE__, (msg)); \
            if (cmocka_assert_jmp_ready) { \
                longjmp(cmocka_assert_jmp_buf, 1); \
            } \
            exit(1); \
        } \
    } while(0)

/* ─── Mock expectation tracking (stubs) ─────────────────────────────── */
#define expect_string(call, param, value)   ((void)0)
#define expect_value(call, param, value)    ((void)0)
#define expect_any(call, param)             ((void)0)
#define expect_not_count(call, param, count) ((void)0)
#define will_return_count(func, value, count) ((void)(func), (void)(value), (void)(count))

/* ─── Assert macros ─────────────────────────────────────────────────── */
#define assert_true(c)                    CMOCKA_ASSERT((c), "assert_true failed")
#define assert_false(c)                   CMOCKA_ASSERT(!(c), "assert_false failed")
#define assert_int_equal(a, b)            CMOCKA_ASSERT((a) == (b), "assert_int_equal failed")
#define assert_int_not_equal(a, b)        CMOCKA_ASSERT((a) != (b), "assert_int_not_equal failed")
#define assert_uint_equal(a, b)           CMOCKA_ASSERT((a) == (b), "assert_uint_equal failed")
#define assert_uint_not_equal(a, b)       CMOCKA_ASSERT((a) != (b), "assert_uint_not_equal failed")
#define assert_ptr_equal(a, b)            CMOCKA_ASSERT((void*)(uintptr_t)(a) == (void*)(uintptr_t)(b), "assert_ptr_equal failed")
#define assert_ptr_not_equal(a, b)        CMOCKA_ASSERT((void*)(uintptr_t)(a) != (void*)(uintptr_t)(b), "assert_ptr_not_equal failed")
#define assert_string_equal(a, b)         CMOCKA_ASSERT((a) != NULL && (b) != NULL && strcmp((a), (b)) == 0, "assert_string_equal failed")
#define assert_string_not_equal(a, b)     CMOCKA_ASSERT((a) != NULL && (b) != NULL && strcmp((a), (b)) != 0, "assert_string_not_equal failed")
#define assert_null(ptr)                  CMOCKA_ASSERT((ptr) == NULL, "assert_null failed")
#define assert_non_null(ptr)              CMOCKA_ASSERT((ptr) != NULL, "assert_non_null failed")
#define assert_return_code(c, r)          CMOCKA_ASSERT((c) >= (r), "assert_return_code failed")
#define assert_in_range(v, min, max)      CMOCKA_ASSERT((v) >= (min) && (v) <= (max), "assert_in_range failed")
#define assert_not_in_range(v, min, max)  CMOCKA_ASSERT((v) < (min) || (v) > (max), "assert_not_in_range failed")

#define assert_memory_equal(a, b, size)   CMOCKA_ASSERT(memcmp((a), (b), (size)) == 0, "assert_memory_equal failed")
#define assert_memory_not_equal(a, b, size) CMOCKA_ASSERT(memcmp((a), (b), (size)) != 0, "assert_memory_not_equal failed")

/* ─── Failure / Skip ────────────────────────────────────────────────── */
#define fail()                            CMOCKA_ASSERT(0, "explicit fail")
#define fail_msg(msg)                     CMOCKA_ASSERT(0, (msg))
#define skip()                            do { fprintf(stderr, "SKIP: %s:%d\n", __FILE__, __LINE__); return; } while(0)

/* ─── Test runner macros ────────────────────────────────────────────── */
#define cmocka_unit_test(fn)              { #fn, (int (*)(void**))fn, NULL, NULL }
#define cmocka_unit_test_setup_teardown(fn, setup, teardown) \
    { #fn, (int (*)(void**))fn, (int (*)(void**))setup, (int (*)(void**))teardown }
#define cmocka_run_group_tests(tests, setup, teardown) \
    cmocka_run_group_tests_impl(tests, sizeof(tests)/sizeof(tests[0]))

/* ─── Runner ────────────────────────────────────────────────────────── */
static inline int cmocka_run_group_tests_impl(
    const struct CMUnitTest tests[], size_t count)
{
    int result = 0;
    for (size_t i = 0; i < count; i++) {
        void *state = NULL;
        cmocka_assert_jmp_ready = 0;

        if (setjmp(cmocka_assert_jmp_buf) == 0) {
            cmocka_assert_jmp_ready = 1;
            if (tests[i].setup_func) {
                if (tests[i].setup_func(&state) != 0) { result = 1; continue; }
            }
            if (tests[i].test_func(&state) != 0) { result = 1; continue; }
            if (tests[i].teardown_func) {
                if (tests[i].teardown_func(&state) != 0) { result = 1; continue; }
            }
        } else {
            /* Assertion failure — test already aborted via longjmp */
            result = 1;
            if (tests[i].teardown_func) {
                tests[i].teardown_func(&state);
            }
        }
        cmocka_assert_jmp_ready = 0;
    }
    return result;
}

#ifdef __cplusplus
}
#endif

#endif /* CMOCKA_H */
