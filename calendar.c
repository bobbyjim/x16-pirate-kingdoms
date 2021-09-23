#include <stdio.h>

#include "calendar.h"

//
// The calendar is 
//    32 kins per winal (a month)
//    16 winals per tun (a long year)
//    32 tuns per katun (42 years)
//    16 katuns per baktun (682 years)
//    32 baktuns
//
// The calendar rolls over every 21,000 years or so.
//

typedef struct {
    int hour:   8;
    int kin:    5;
    int winal:  4;
    int tun:    5;
    int katun:  4;
    int baktun: 5;
} LongCount;

LongCount today;
char todaysDate[20];

char *winal[] = 
{
    "nahakku",
    "wendros chi",
    "wendros bit",
    "wendros taw",
    "hashni",
    "hewsreh chi",
    "hewsreh bit",
    "hewsreh taw",
    "nahati",
    "samreh chi",
    "samreh bit",
    "samreh taw",
    "nakabash",
    "kerpos chi",
    "kerpos bit",
    "kerpos taw"
};

void nextHour()
{
    // we can use the nature of bitfields
    // to very very easily manipulate the calendar

    if (today.katun == 0) ++today.baktun;
    if (today.tun   == 0) ++today.katun;
    if (today.winal == 0) ++today.tun;
    if (today.kin   == 0) ++today.winal;
    if (today.hour  == 0) ++today.kin;
    ++today.hour;
}

unsigned char kin() { return today.kin; }
unsigned char tun() { return today.tun; }
unsigned char katun() { return today.katun; }
unsigned char baktun() { return today.baktun; }

char *theDate()
{
   sprintf(todaysDate, "%02d.%02d.%02d.%02d.%02d", 
        today.kin,
        today.winal,
        today.tun,
        today.katun,
        today.baktun);

   return todaysDate;
}
