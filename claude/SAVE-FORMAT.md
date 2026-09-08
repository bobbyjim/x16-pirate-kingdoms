# Save Format Specification

This document defines the intended persisted state split for the Commander X16 port.

The guiding principle is to keep the static world separate from the dynamic game state, and to keep player state separate from world state so that multiple player records can exist without forcing a single active player into the same file as the world.

## Resource split

The game should be loaded as a bundle of resource files keyed by a common basename:

- `<base>.MAP` : static terrain and seeded map object layout
- `<base>.WLD` : dynamic world state (settlements, links, tick, world events)
- `<base>.PLY` : player list / player state catalog
- `<base>.PRG` : frontend program binary

Legacy note:

- `PKGAME.SAV` is a useful working name for a single dynamic-state file, but the design below prefers `WLD` + `PLY` because the state is genuinely two different concerns.
- If we later decide to keep one file for convenience, it can still be a small wrapper around the same logical sections below.

## Why a separate player file?

We want to support multiple player instances in the save set without forcing a game to commit to only one player state.

This gives us:

- multiple historical or alternate player records
- simple switching between player instances
- a stable list of players distinct from the current world snapshot
- a cleaner mapping from `Settlement.owner` or `PlayerID` references to player metadata

Only one player needs to be active at a time, but the data model should allow more than one player record to exist in storage.

## Core model

The world save should be authoritative for mutable world facts.

The player save should be authoritative for mutable player facts.

The static map should be authoritative for fixed geographic facts and seeded objects, and should not depend on either runtime world or player data.

## File responsibilities

### 1. MAP file

The map file stores:

- terrain grid
- special-zone flags
- static object list / world object table
- seeded settlement object locations
- world boundaries and static geography

This file is effectively read-only for a campaign and is not intended to contain settlement, player, or trade-link runtime mutations.

### 2. WLD file

The world file stores all mutable world state derived from the loaded map.

This includes:

- world tick and date/calendar state
- world seed / rng state, if needed for reproducibility
- world-level event chance settings
- `settlement_count`
- settlement records
- trade-link records
- any dynamic world-level data that is not player-specific
- note buffer / recent events if we want replay or logging

This should reflect the actual live world state after the map has been bootstrapped.

### 3. PLY file

The player file stores the player catalog and the active player selection.

This includes:

- `player_count`
- `active_player_id`
- per-player records
- each player record includes:
  - `player_id`
  - `name`
  - `color` or banner seed
  - `supplies`
  - `reputation`
  - `status` or campaign flags
  - `owner`-level resource data
  - any player-specific progression not tied to a specific settlement

This allows the game to keep a list of player instances, even if only one is currently chosen.

## Object identity and references

To keep serialization simple and stable across save/load cycles:

- settlement IDs are world-unique IDs
- player IDs are globally unique across the player list
- settlement `owner` values are player IDs, not raw indices
- trade links reference settlement IDs, not in-memory pointers
- the player file can contain a player record for any player referenced by world data, even if that player is not the active one

This makes the data model robust even as the world evolves and active-player switching happens.

## Proposed file-level layout

### MAP layout

The map file should remain a direct binary representation of the fixed world substrate.

The current project already treats this as a flat, mostly static file:

- terrain data
- object list
- object count metadata
- no ephemeral settlement table

This is the best candidate for the `MAP` file.

### WLD layout

The world file begins with a header tag and version, then a series of tables.

Suggested structure:

1. Header
   - magic: `PKWL`
   - version: `1`
   - reserved bytes
2. World metadata
   - tick
   - calendar date / day / year
   - world seed
   - event chance
   - initial settlement count
3. Settlement table
   - count
   - for each settlement:
     - id
     - x
     - y
     - owner player id
     - alive flag
     - population
     - wealth / supply reserves if applicable
     - structure bitmask
     - focus / culture values
     - event status
     - any settlement-specific state relevant for gameplay
4. Trade-link table
   - count
   - for each link:
     - link id
     - type
     - from_settlement_id
     - to_settlement_id
     - owner player id or world owner id
     - status / disruption state
     - distance/cost fields if needed
5. Optional world notes / event log
   - note count
   - list of recent notes if needed for UI continuity

The exact settlement record may later be compressed, but the schema should stay conceptually like this.

### PLY layout

The player file begins with a header tag and version, then a list of player records.

Suggested structure:

1. Header
   - magic: `PKPL`
   - version: `1`
2. Player metadata
   - `active_player_id`
   - `player_count`
3. Player records
   - for each player:
     - `player_id`
     - `name`
     - `color`
     - `human_or_ai`
     - `supplies`
     - `reputation`
     - `status_flags`
     - `campaign_progress` or objective state
     - any persistent player-level economic data

This file represents the player list, not just one player. The active player is simply the one selected when the game resumes.

## Minimal save policy

For the first implementation, the save format can be intentionally small but clear:

- world file contains all world-level mutable data
- player file contains the player list and active player selection
- the code can ignore optional fields until needed

We should avoid serializing raw pointers or transient runtime-only data structures.

## Example conceptual bundle

A campaign can be persisted as this bundle:

- `PKGAME.MAP`
- `PKGAME.WLD`
- `PKGAME.PLY`
- `PKGAME.PRG`

This matches the architecture cleanly:

- map is static geography
- world is mutable simulation data
- player list is the independent player catalog
- program is the frontend runtime

## Implementation guidance

The first save implementation should stick to these rules:

1. Use explicit versioned headers.
2. Use fixed-width numeric fields where possible.
3. Keep all references by stable IDs, never by pointer addresses.
4. Keep the player list separate from the world table.
5. Keep optional future fields at the end of each record so older saves remain easier to extend.

## Open design decisions

These remain to be finalized:

- whether `WLD` and `PLY` are separate files in all builds or whether we later consolidate them into a single `SAV` wrapper
- whether we need to save a note ring buffer or only the latest notes
- how much of the `Settlement` record is world-owned vs player-owned
- whether `PlayerList` should also include AI or neutral factions, not just human players

But the split itself is the right direction: static map, dynamic world, dynamic players.
