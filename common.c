#include <peekpoke.h>
#include <cx16.h>

#include "common.h"

void setBank(byte bank)
{
    RAM_BANK = bank;
    POKE(0, bank);
}
