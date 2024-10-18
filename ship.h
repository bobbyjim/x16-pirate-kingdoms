#ifndef _SHIP_H_
#define _SHIP_H_

typedef struct {

	char name[15];
	char mission[3];
	unsigned address;
	unsigned char speed;
	unsigned char acceleration;
	unsigned char hull;
	unsigned char people_capacity;
	unsigned char goods_capacity;
	unsigned char ballista_capacity;
	unsigned char greek_fire_capability;

} ShipData;


ShipData* getShipData( unsigned char index );


#endif