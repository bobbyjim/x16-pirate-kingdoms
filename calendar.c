#include "calendar.h"

char *months[] = 
{
    "wendros 1",
    "wendros 2",
    "wendros 3",
    "hewsreh 1",
    "hewsreh 2",
    "hewsreh 3",
    "samreh 1",
    "samreh 2",
    "samreh 3",
    "kerpos 1",
    "kerpos 2",
    "kerpos 3"
};

char *getMonth(unsigned char num)
{
    return months[num];
}
