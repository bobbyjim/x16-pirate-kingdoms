#include <stdio.h>
#include "calendar.h"

void calendar_init(Calendar *c)
{
    if (!c) return;
    c->day = 0;
    c->month = 0;
    c->year = 0;
    c->cycle = 0;
}

void calendar_advance(Calendar *c, word ticks)
{
    word i;

    if (!c) return;
    for (i = 0; i < ticks; i++) {
        if (++c->day >= 20) {
            c->day = 0;
            if (++c->month >= 20) {
                c->month = 0;
                if (++c->year >= 20) {
                    c->year = 0;
                    c->cycle++;
                }
            }
        }
    }
}

int calendar_format(const Calendar *c, char *buf, size_t size)
{
    if (!buf || size == 0) return 0;
    if (size < 32) return -1;
    if (!c) {
        snprintf(buf, size, "0.0.0.0");
        return 0;
    }

    snprintf(buf, size, "%u.%u.%u.%u", (unsigned)c->cycle, (unsigned)c->year,
             (unsigned)c->month, (unsigned)c->day);
    return 0;
}
