/* Assertion helpers for unit tests. */
#ifndef TEST_UTIL_H
#define TEST_UTIL_H

#include <stdio.h>
#include <string.h>

static int tests_run = 0;
static int tests_failed = 0;

#define CHECK(cond) do {                                                  \
        tests_run++;                                                      \
        if (!(cond)) {                                                    \
            tests_failed++;                                               \
            fprintf(stderr, "  FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        }                                                                 \
    } while (0)

#define CHECK_STR(actual, expected) do {                                  \
        tests_run++;                                                      \
        if (strcmp((actual), (expected)) != 0) {                          \
            tests_failed++;                                               \
            fprintf(stderr, "  FAIL %s:%d:\n    got:      [%s]\n    expected: [%s]\n", \
                    __FILE__, __LINE__, (actual), (expected));            \
        }                                                                 \
    } while (0)

#define TEST_SUMMARY(name) (                                              \
        printf("%-12s %d checks, %d failed\n", (name), tests_run, tests_failed), \
        tests_failed ? 1 : 0)

#endif /* TEST_UTIL_H */
