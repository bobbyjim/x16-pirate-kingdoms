#include <stdio.h>
#include <time.h>

#include "calendar_engine.h"

typedef struct {
    int min:    8;
    int hour:   16;
    int kin:    5;
    int winal:  4;
    int tun:    5;
    int katun:  8;
    int baktun: 8;
} LongCount;

static LongCount today;
static time_t now;
static unsigned char kin_current;

// Mayan calendar names (data only, no display)
static const char *tzolkin_name[] = 
{
   "ha'     ",
   "ik'     ",
   "ak'ab   ",
   "ohl     ",
   "chikchan",
   "cham    ",
   "chij    ",
   "lamat   ",
   "muluk   ",
   "oc      ",
   "chuwen  ",
   "eb'     ",
   "b'en    ",
   "hish    ",
   "tz'ikin ",
   "kib'    ",
   "kab     ",
   "etz'nab ",
   "kawak   ",
   "ajaw    "
};

static const char *haab_name[] = {
   "  pop",
   "  wo'",
   "  sip",
   "sotz'",
   "  sek",
   "  xul",
   "  yax",
   "  mol",
   "ch'en",
   "  yax",
   " sak'",
   "  keh",
   "  mak",
   " k'an",
   "muwan",
   "  pax",
   "'ayab",
   "  k'u"
};

// Pure logic: calculate date components from time_t
void calendar_update_internal()
{
    unsigned char newKin;
    time(&now);
    newKin = (now / 20) % 20;
    
    if (newKin == kin_current) return; // nothing changed
    
    kin_current = newKin;
    today.winal = (now / 400) % 20;
    today.katun = (now / 8000) % 20;
    today.baktun = (now / 160000) % 20;
    today.kin = kin_current;
}

// Pure logic: format date string (no display calls)
void calendar_get_long_count(CalendarDate *date)
{
    calendar_update_internal();
    date->kin = today.kin;
    date->winal = today.winal;
    date->katun = today.katun;
    date->baktun = today.baktun;
}

// Pure logic: get Mayan Haab date
void calendar_get_haab(CalendarHaab *haab)
{
    unsigned char trecena, kin, haab_month;
    time_t t;
    
    time(&t);
    t >>= 4;
    
    trecena = t % 13;
    kin = t % 20;
    haab_month = (t / 20) % 18;
    
    haab->trecena = trecena;
    haab->kin = kin;
    haab->haab_month = haab_month;
}

// Helper: get name for haab month
const char* calendar_get_haab_name(unsigned char month)
{
    if (month < 18) return haab_name[month];
    return "unknown";
}

// Helper: get name for tzolkin day
const char* calendar_get_tzolkin_name(unsigned char day)
{
    if (day < 20) return tzolkin_name[day];
    return "unknown";
}
