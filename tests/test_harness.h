#ifndef TEST_HARNESS_H
#define TEST_HARNESS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int tests_passed = 0;
static int tests_failed = 0;

#define ASSERT_EQ(actual, expected) \
    do { \
        if ((actual) != (expected)) { \
            fprintf(stderr, "FAIL: %s:%d - Expected %d, got %d\n", \
                    __FILE__, __LINE__, (int)(expected), (int)(actual)); \
            tests_failed++; \
        } else { \
            tests_passed++; \
        } \
    } while (0)

#define ASSERT_STR_EQ(actual, expected) \
    do { \
        if (strcmp((actual), (expected)) != 0) { \
            fprintf(stderr, "FAIL: %s:%d - Expected '%s', got '%s'\n", \
                    __FILE__, __LINE__, (expected), (actual)); \
            tests_failed++; \
        } else { \
            tests_passed++; \
        } \
    } while (0)

#define ASSERT_TRUE(condition) \
    do { \
        if (!(condition)) { \
            fprintf(stderr, "FAIL: %s:%d - Condition was false\n", __FILE__, __LINE__); \
            tests_failed++; \
        } else { \
            tests_passed++; \
        } \
    } while (0)

#define ASSERT_FALSE(condition) \
    do { \
        if ((condition)) { \
            fprintf(stderr, "FAIL: %s:%d - Condition was true\n", __FILE__, __LINE__); \
            tests_failed++; \
        } else { \
            tests_passed++; \
        } \
    } while (0)

#define RUN_TEST(test_func) \
    do { \
        printf("Running %s...\n", #test_func); \
        test_func(); \
    } while (0)

#define PRINT_SUMMARY() \
    do { \
        printf("\n========================================\n"); \
        printf("Tests passed: %d\n", tests_passed); \
        printf("Tests failed: %d\n", tests_failed); \
        printf("========================================\n"); \
        return (tests_failed == 0) ? 0 : 1; \
    } while (0)

#endif /* TEST_HARNESS_H */
