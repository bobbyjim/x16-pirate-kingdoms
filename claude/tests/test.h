#ifndef _TEST_H_
#define _TEST_H_

#include <stdio.h>

/* Minimal assert-style test framework, no external dependency (matches
   DEV-PROCESS.md's requirement that the engine be testable off-hardware
   with plain tools). Each test_*.c exposes a run_*_tests() function that
   uses CHECK(); tests/main.c calls them all and prints a summary. */

extern int g_tests_run;
extern int g_tests_failed;

#define CHECK(cond) do { \
    g_tests_run++; \
    if (!(cond)) { \
        g_tests_failed++; \
        printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
    } \
} while (0)

#endif
