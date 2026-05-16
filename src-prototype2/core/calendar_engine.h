#ifndef _CALENDAR_ENGINE_H_
#define _CALENDAR_ENGINE_H_

// Pure calendar logic, no display dependencies

typedef struct {
    unsigned char kin;
    unsigned char winal;
    unsigned char katun;
    unsigned char baktun;
} CalendarDate;

typedef struct {
    unsigned char trecena;
    unsigned char kin;
    unsigned char haab_month;
} CalendarHaab;

// Update internal calendar state
void calendar_update_internal();

// Get long count (Mayan baktun.katun.winal.kin)
void calendar_get_long_count(CalendarDate *date);

// Get Haab date (Mayan 365-day calendar)
void calendar_get_haab(CalendarHaab *haab);

// Get calendar name strings (for any display adapter to use)
const char* calendar_get_haab_name(unsigned char month);
const char* calendar_get_tzolkin_name(unsigned char day);

#endif
