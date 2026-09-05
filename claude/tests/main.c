#include <stdio.h>
#include "test.h"

int g_tests_run = 0;
int g_tests_failed = 0;

void run_settlement_tests(void);
void run_events_tests(void);
void run_map_tests(void);
void run_world_tests(void);
void run_note_tests(void);
void run_storage_tests(void);
void run_calendar_tests(void);

int main(void)
{
    run_settlement_tests();
    run_events_tests();
    run_map_tests();
    run_world_tests();
    run_note_tests();
    run_storage_tests();
    run_calendar_tests();

    printf("%d/%d checks passed\n", g_tests_run - g_tests_failed, g_tests_run);
    return g_tests_failed ? 1 : 0;
}
