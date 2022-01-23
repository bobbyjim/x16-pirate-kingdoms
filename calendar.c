#include <stdio.h>
#include <stddef.h>
#include <time.h>

#include "calendar.h"

typedef struct {
    int min:    8;
    int hour:   16;
    int kin:    5;
    int winal:  4;
    int tun:    5;
    int katun:  8;
} LongCount;

LongCount today;
char todaysDate[20];

char *tzolkin_name[] = 
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

char *haab_name[] = {
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

time_t now;  // Unsigned Long

char *theDate()
{
   unsigned char kin, winal, katun, baktun;
   time(&now);
   kin   = (now/20) % 20;
   winal = (now/400) % 20; 
   katun = (now/8000) % 20;
   baktun = (now/160000) % 20;

   sprintf(todaysDate, "%02d.%02d.%02d.%02d", 
      kin,
      winal,
      katun,
      baktun);

   return todaysDate;
}

char *thaHaab()
{
    unsigned char trecena, kin, haab;
    time(&now);
    now >>= 4;

   trecena = now % 13;
   kin     = now % 20;
   haab    = (now/20) % 18;

   sprintf(todaysDate, "%s %02d %s", 
        haab_name[haab],
        trecena+1,
        tzolkin_name[kin]
        );

   return todaysDate;
}
