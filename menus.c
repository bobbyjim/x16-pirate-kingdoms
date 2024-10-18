
#include <conio.h>

#include "menus.h"

char* menu1[3] = {
    "item one",
    "item two",
    "item three"
};

char* menu2[3] = {
    "item a",
    "item b",
    "item c"
};

void menus_show()
{
    unsigned char x;
    for(x=0; x<3; ++x)
    {
        cputsxy(1,4+x,menu1[x]);
        cputsxy(70,4+x,menu2[x]);
    }
}

void menus_check()
{

}