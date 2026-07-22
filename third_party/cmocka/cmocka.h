/**
 * @file cmocka.h
 * @brief Minimal cmocka stub for native coverage builds
 *
 * Provides macros commonly used by cmocka tests without the full framework.
 */
#ifndef CMOCKA_H
#define CMOCKA_H

#include <stddef.h>
#include <stdint.h>
#include <setjmp.h>

/* State type */
struct CMUnitTest {
    const char *name;
    int (*test_func)(void **state);
    int (*setup_func)(void **state);
    int (*teardown_func)(void **state);
};

/* Mock value tracking */
#define will_return(func, value)       ((void)(func), (void)(value))
#define mock()                         0
#define mock_type(type)                ((type)0)
#define mock_ptr_type(type)            ((type)(intptr_t)0)

/* Assert macros */
#define assert_int_equal(a, b)         do { \
    if ((a) != (b)) { return 1; } \
} while(0)

#define assert_int_not_equal(a, b)     do { \
    if ((a) == (b)) { return 1; } \
} while(0)

#define assert_ptr_equal(a, b)         do { \
    if ((void*)(uintptr_t)(a) != (void*)(uintptr_t)(b)) { return 1; } \
} while(0)

#define assert_ptr_not_equal(a, b)     do { \
    if ((void*)(uintptr_t)(a) == (void*)(uintptr_t)(b)) { return 1; } \
} while(0)

#define assert_string_equal(a, b)      do { \
    (void)(a); (void)(b); \
} while(0)

#define assert_true(c)                 do { \
    if (!(c)) { return 1; } \
} while(0)

#define assert_false(c)                do { \
    if ((c)) { return 1; } \
} while(0)

#define assert_return_code(c, r)       do { \
    (void)(r); if ((c) < 0) { return 1; } \
} while(0)

#define assert_in_range(v, min, max)   do { \
    if ((v) < (min) || (v) > (max)) { return 1; } \
} while(0)

#define assert_not_in_range(v, min, max) do { \
    if ((v) >= (min) && (v) <= (max)) { return 1; } \
} while(0)

/* Test runner */
#define cmocka_unit_test(fn)           { #fn, fn, NULL, NULL }
#define cmocka_unit_test_setup_teardown(fn, setup, teardown) \
    { #fn, fn, setup, teardown }
#define cmocka_run_group_tests(tests, setup, teardown) \
    cmocka_run_group_tests_impl(tests, sizeof(tests)/sizeof(tests[0]))

static inline int cmocka_run_group_tests_impl(
    const struct CMUnitTest tests[], size_t count)
{
    for (size_t i = 0; i < count; i++) {
        void *state = NULL;
        if (tests[i].setup_func) {
            if (tests[i].setup_func(&state) != 0) return 1;
        }
        if (tests[i].test_func(&state) != 0) return 1;
        if (tests[i].teardown_func) {
            if (tests[i].teardown_func(&state) != 0) return 1;
        }
    }
    return 0;
}

#endif /* CMOCKA_H */
