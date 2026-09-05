#include <string.h>
#include "test.h"
#include "../engine/calendar.h"

void run_calendar_tests(void)
{
    Calendar c = {0, 0, 0, 0};

    calendar_advance(&c, 1);
    CHECK(c.day == 1);
    CHECK(c.month == 0);
    CHECK(c.year == 0);
    CHECK(c.cycle == 0);

    c = (Calendar){0, 0, 0, 0};
    calendar_advance(&c, 20);
    CHECK(c.day == 0);
    CHECK(c.month == 1);
    CHECK(c.year == 0);
    CHECK(c.cycle == 0);

    c = (Calendar){19, 19, 19, 0};
    calendar_advance(&c, 1);
    CHECK(c.day == 0);
    CHECK(c.month == 0);
    CHECK(c.year == 0);
    CHECK(c.cycle == 1);

    c = (Calendar){12, 7, 2, 3};
    {
        char buf[32];
        CHECK(calendar_format(&c, buf, sizeof(buf)) == 0);
        CHECK(strcmp(buf, "3.2.7.12") == 0);
    }
}
