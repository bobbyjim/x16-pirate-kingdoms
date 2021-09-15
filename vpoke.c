
#include <peekpoke.h>

#include "vpoke.h"

/*
void vpoke(uint16_t address, uint8_t value)
{
    POKE( 0x9f20, address & 0xff); 
    address >>= 8;
    POKE( 0x9f21, 16 + address & 0x0f); // 4 low bits of the high byte
                                        // but let's set auto-increment anyway
    POKE( 0x9f22, 1);
    POKE( 0x9f23, value);
}
*/
void vinc(uint8_t value)
{
    // okay everything's already set up, including auto-increment,
    // so all we have to do is:
    POKE(0x9f23, value);
}
