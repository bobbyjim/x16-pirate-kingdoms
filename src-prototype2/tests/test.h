#ifndef _TEST_H_
#define _TEST_H_

#include <stdio.h>
#include <string.h>

// Simple unit test framework for core logic

#define ASSERT_EQ(actual, expected) \
    do { \
        if ((actual) != (expected)) { \
            printf("FAIL: %s:%d: expected %d, got %d\n", __FILE__, __LINE__, (expected), (actual)); \
            return 0; \
        } \
    } while(0)

#define ASSERT_NEQ(actual, expected) \
    do { \
        if ((actual) == (expected)) { \
            printf("FAIL: %s:%d: should not equal %d\n", __FILE__, __LINE__, (expected)); \
            return 0; \
        } \
    } while(0)

#define ASSERT_STR_EQ(actual, expected) \
    do { \
        if (strcmp((actual), (expected)) != 0) { \
            printf("FAIL: %s:%d: expected '%s', got '%s'\n", __FILE__, __LINE__, (expected), (actual)); \
            return 0; \
        } \
    } while(0)

#define TEST_START() \
    printf("Running test suite...\n")

#define TEST_END() \
    printf("All tests passed!\n")

#endif
