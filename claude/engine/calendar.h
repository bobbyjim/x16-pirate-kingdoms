#ifndef _CALENDAR_H_
#define _CALENDAR_H_

#include <stddef.h>
#include "common.h"

/* A tick-based calendar with one full byte per field. This keeps the time
   model simple and explicit: one day is one simulation tick, and day/month/
   year/cycle values carry naturally without bitfield packing or CPU-heavy
   bit manipulation. */
typedef struct {
    byte day;
    byte month;
    byte year;
    byte cycle;
} Calendar;

void calendar_init(Calendar *c);
void calendar_advance(Calendar *c, word ticks);
int calendar_format(const Calendar *c, char *buf, size_t size);

#endif
