ARCHIPELAGO MAP GENERATOR - REQUIREMENTS
=========================================

PURPOSE
-------
Generate a procedural archipelago game world with settlements, roads, and points
of interest for the Commander X16 platform (6502-based system with ~40K RAM).

OUTPUT FILES
------------
1. archipelago.map (binary, 72KB total)
   - Object list (8KB) - comes first in file
   - Map data (64KB) - follows object list

2. archipelago.txt (text visualization)
   - Shows terrain types and objects with ASCII characters
   - No roads displayed

3. archipelago-roads.txt (text visualization with roads)
   - Shows terrain, objects, and road network

BINARY MAP FORMAT
-----------------
Map Dimensions: 256 x 256 tiles = 65,536 bytes (64KB)

Each tile is 1 byte with bit fields:
  Bits 0-3: Terrain type (4 bits)
    0 = water
    1 = grassland
    2 = forest
    3 = hills
    4 = mountains
    5 = desert
    6 = swamp
    7-15 = reserved
  
  Bits 4-5: Travel ease (2 bits)
    0 = difficult
    1 = standard
    2 = path
    3 = road
  
  Bit 6: Special zone flag (1 bit)
    Indicates tile is part of special zone (settlement, hazard, etc)
  
  Bit 7: Object flag (1 bit)
    Indicates tile contains an object

OBJECT LIST FORMAT
------------------
Size: 8,192 bytes (8KB)
Max objects: 1024
Each object: 8 bytes

Structure per object:
  Byte 0: Object type
  Byte 1: X position (0-255)
  Byte 2: Y position (0-255)
  Bytes 3-7: Additional data (5 bytes, type-specific)

Object Types:
  0x00: Empty/null
  0x01: Settlement (Byte 3 = size: 20-139 based on terrain)
  0x02: Ship (Byte 3 = subtype)
  0x03: Group
  0x04: Tower
  0x05: Shrine
  0x06: Ruins
  0x07: Mine
  0x08: Stela
  0x09: Portal
  0x0A: Lighthouse
  0x0B: Bridge
  0x0C: Cave
  0x0D: Monument

GENERATION REQUIREMENTS
------------------------

Archipelago Generation:
- Generate 12 islands using blob growth algorithm
- Island sizes range from 20x20 to 50x50 tiles
- Islands positioned with 40-tile margin from map edges
- Terrain distribution based on distance from island center:
  * Center (0-30%): Hills, mountains, some forest and grassland
  * Middle (30-70%): Forest, grassland, some hills
  * Edge (70%+): Grassland, forest, hills

Settlement Placement:
- Place 20 settlements on land
- Avoid mountains and swamps
- Minimum 15-tile spacing between settlements
- Settlement size based on terrain:
  * Grassland: 60-139 (most favorable)
  * Forest: 40-99
  * Hills: 30-79
  * Other: 20-59
- Mark tile as special zone (bit 6 set)

Road Network:
- Connect all settlements using minimum spanning tree algorithm
- Roads marked with travel ease = 3 (ROAD)
- Roads avoid water but may cross other terrain
- Roads do not cross mountains

Points of Interest:
- 8 towers
- 12 shrines
- 10 ruins
- 6 mines
- 7 stelae
- 3 portals
- 8 caves
- 4 monuments
- Placed on land only
- Cannot overlap with existing special zones
- Each POI marks its tile as special zone (bit 6 set)

TEXT MAP LEGEND
---------------
Terrain:
  . = water
  g = grassland
  f = forest
  h = hills
  m = mountains
  d = desert
  w = swamp

Roads (archipelago-roads.txt only):
  = = road

Objects:
  X = settlement
  T = tower
  S = shrine
  R = ruins
  M = mine
  A = stela
  P = portal
  C = cave
  O = monument

TECHNICAL REQUIREMENTS
-----------------------
- Uses random walk algorithm for organic island shapes
- Weighted random terrain selection based on distance zones
- Minimum spanning tree for efficient road connectivity
- Binary file written in correct byte order (object list first, map second)
- All positions 0-indexed within 256x256 coordinate space

DESIGN CONSTRAINTS
------------------
- Total binary size: exactly 73,728 bytes (72KB)
- Object list always 8,192 bytes (padded with null objects if needed)
- Map data always 65,536 bytes
- Single-byte tile encoding for minimal memory footprint
- Optimized for 6502 platform with limited RAM

