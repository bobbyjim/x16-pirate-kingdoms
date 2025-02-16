

#include "ship.h"


// 0 SHIP_ADDR_DRAKKAR;
// 1 SHIP_ADDR_DROMON;
// 2 SHIP_ADDR_GENOESE;
// 3 SHIP_ADDR_PENTEKONTOR;
// 4 SHIP_ADDR_SAMBUK;
// 5 SHIP_ADDR_TARTANE;
// 6 SHIP_ADDR_TRIREME;
// 7 SHIP_ADDR_XEBEC;

// L/M/H     Light Medium Heavy
// T/W/G/C   War Trader General-Purpose Corsair
// G/S       Galley Sailboat

ShipData ships[8] = { //      addr      speed       accel     hull      people     goods      ballista    GkFire 
	{ "dromon",      "hwg",   0x4400,      5,         1,        32,       150,        40,        10,        10 },  
	{ "pentekontor", "mtg",   0x4c00,      6,         1,        24,        50,        60,         2,         0 },  
	{ "trireme",     "mgg",   0x5800,      7,         1,        24,       200,        20,         5,        10 },  
	{ "drakkar",     "lwg",   0x4000,      8,         2,        24,        30,        10,         0,         0 },  
	{ "xebec",       "mcs",   0x5c00,      9,         2,        24,        16,        20,         1,         0 },
	{ "tartane",     "lts",   0x5400,     10,         2,        16,        16,        25,         0,         0 },  
	{ "sambuk",      "lts",   0x5000,     11,         3,        16,        30,        25,         0,         0 },  
	{ "genoese",     "lwg",   0x4800,     12,         4,        20,        50,        25,         2,         0 },  
};

ShipData* getShipData( unsigned char index )
{
	return &ships[index];
}