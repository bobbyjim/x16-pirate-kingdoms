# Pirate Kingdoms

Core concept: Pirate Kingdoms is a simulation of emergent civilization and impermanence. Players influence a living world but do not fully control it. Settlements rise, adapt, fracture, collapse, and are reborn through systemic interactions.

# Structure

                 BUSINESS LOGIC
                       │
              ┌────────┴────────┐
              │                 │
          WORLD STATE         NOTES
              │                 │
              └────────┬────────┘
                       │
                       ▼
                 PRESENTATION
                       │
              ┌────────┴────────┐
              │                 │
           GRAPHICS           SOUND
             API                API
              │                 │
              └────────┬────────┘
                       ▼
           HAL { null, debug, or X16 }

# Canonical Design Rule

Settlements are structure-first.

Structures are the single source of truth for settlement identity, capability, and risk.

Derived outcomes from structures include:

- Settlement size
- Population support
- Wealth potential
- Reserve potential
- Infrastructure resilience
- Defense posture
- Culture vector (TRA/AGR/GRO/SEC)

Direct stat-first settlement modeling is not canonical and should not be used for new simulation logic.

# Background and Intent

The game is about endurance, adaptation, and legacy rather than permanent conquest. Failure is expected and meaningful. A collapse is not a hard game over; it is a transition that can produce migration, splintering, ruins, and recolonization.

The narrative loop is:

- Build a network of fragile settlements
- Survive shocks and cascading stress
- Lose parts of the network
- Regrow from ruins and refugee movement

# I. World Simulation and Dynamics

## Settlement Model

A settlement is a bounded set of structures in a location.

- Structure capacity is variable, based on terrain and local resource suitability.
- Capacity is intentionally constrained to preserve meaningful trade-offs and avoid feature bloat.
- A settlement can be sparse and fragile (encampment) or dense and specialized (major city), depending on how many slots are available and what is built into them.

## Structure Taxonomy

Initial structure types:

1. Dock
- Primary characteristic: Commerce
- Secondary characteristic: Defense
- Purpose: maritime access, exchange, logistics exposure

2. Warehouse
- Primary characteristic: Commerce
- Secondary characteristic: Industry
- Purpose: storage throughput, stock stabilization

3. Fort
- Primary characteristic: Defense
- Secondary characteristic: Population
- Purpose: protection, garrison influence, order pressure

4. Town Hall
- Primary characteristic: Population
- Secondary characteristic: Culture
- Purpose: coordination, civic cohesion, administrative strength

5. Monument
- Primary characteristic: Culture
- Secondary characteristic: Commerce
- Purpose: identity, prestige, attractor effects

## Structure Characteristics

Characteristics are intermediate simulation dimensions, not player-facing victory scores.

- Defense: resistance to attack and instability
- Commerce: trade throughput and exchange reliability
- Industry: transformation and material productivity
- Population: housing and social carrying potential
- Culture: cohesion, ideology, identity pressure

Derived settlement outputs are computed from characteristic composition and structure condition.

## Emergent Settlement Profiles

Different structure mixes create distinct settlement behavior:

- Dock + Warehouse + Monument tends toward trade-led influence
- Fort-heavy layouts tend toward hard security and coercive stability
- Town Hall + Monument mixes tend toward cultural cohesion and internal retention

No explicit settlement class labels are required; profile is emergent from built reality.

## Culture Vector (Derived)

Culture is also derived from structure composition and condition.

- TRA: merchant orientation from trade-heavy composition
- AGR: coercive/aggressive tendency from militarized and stress-reactive composition
- GRO: expansion and regeneration tendency from growth-supporting composition
- SEC: security orientation from protective and stabilizing composition

The vector is normalized and enforces trade-offs. A settlement cannot maximize all cultural directions simultaneously.

## Inter-Settlement Trade Links

Trade is represented explicitly by lightweight link objects. There are two canonical link types:

- Land caravan link: overland movement between two settlements
- Sea fleet link: maritime movement between two settlements with port access

These links do not simulate individual units in detail. They represent aggregate route health and throughput so they remain cheap while still being consequential.

### Link Consequence Model

Links can be disrupted, degraded, or severed by world conditions and events.

- Land caravans are sensitive to terrain friction, storms, raids, and inland conflict
- Sea fleets are sensitive to pirates, storms, blockades, and dock damage
- Low link health reduces trade throughput
- Reduced throughput lowers reserve and wealth stability at both endpoints
- Prolonged disruption increases collapse risk for specialized settlements

### Link Lifecycle

- Created when two settlements have sufficient trade compatibility and reach
- Improves gradually with stable use
- Degrades under stress, conflict, and endpoint structural failure
- Can become inactive without immediate deletion, allowing recovery
- Can be removed if both endpoints are invalid or route viability is lost long-term

### Link Capacity and Specialization

Route value emerges from endpoint structure composition.

- Dock and Warehouse bias higher maritime throughput
- Fort-heavy endpoints are harder to disrupt but may carry less total commerce
- Town Hall and Monument can improve route persistence through cohesion and trust effects

Trade links should amplify settlement specialization instead of flattening it.

## Impermanence and Decline

Events test vulnerabilities implied by the current structure profile.

- Drought pressures reserves and population support pathways
- Plague pressures population-support and civic resilience pathways
- Storm pressures infrastructure and exposed logistics pathways
- Pirates pressure coastal commerce and defensive posture
- Market Crash pressures trade dependency and stock buffering
- Civil War pressures legitimacy, cohesion, and coercive imbalance

Event effects are primarily expressed as structure degradation, dysfunction, or loss. Derived outputs drop because the underlying structure network is damaged.

Weakness amplification is required: fragile or overly specialized layouts should fail faster under the wrong stress.

## Cascades, Collapse, and Rebirth

Collapse is a consequence of sustained structural failure.

- Refugee cascades strain neighboring settlements
- Civil fracture can spawn splinter factions
- Ruins persist as historical artifacts and potential re-seed anchors
- Recolonization can occur at old or nearby sites under the right conditions

The simulation must support long-run churn without requiring a global difficulty director.

# II. Player Agency and Interaction

## Fleet-Centered Kingdom

The player's durable center is fleet mobility plus distributed outposts.

- Trade ships: high cargo, low survivability
- Warships: protection and coercive power
- Explorers: scouting and route discovery

Settlements are not static endpoints; they are nodes in a changing maritime network.

## Strategic Levers

Player agency should focus on structural and logistical decisions:

- Where to found and reinforce outposts
- Which structures to prioritize within limited capacity
- How to route cargo and reserves between vulnerable nodes
- When to evacuate, defend, or abandon

Agency should bias outcomes without eliminating uncertainty.

## Combat and Consequence

Combat happens in-map and has material consequences for structures, logistics, and political continuity.

- Raids can damage economic structures
- Sieges can degrade defense and civic cohesion
- Victories can shift control, extract resources, or force migration

## Economy and Trade

The economy emerges from settlement structure composition and inter-settlement flow.

- Specialized nodes produce surpluses and deficits
- Merchant behavior and player routing exploit imbalances
- Shocks propagate through connected logistics

Inter-settlement flow should be mediated primarily by the explicit link layer (caravan and fleet links), not by an implicit global market.

# III. World State and Legacy

## Map and Simulation Substrate

- 256x256 world grid
- Terrain and travel properties shape settlement viability
- Settlements, ruins, and routes form a persistent historical layer

## Settlement State (Conceptual)

Canonical persistent settlement state should include:

- Identity and position (id, owner, coordinates)
- Structure slot capacity and per-slot occupancy/condition
- Event status and life-state
- Any minimal metadata required for deterministic derivation and simulation continuity

Derived values (size/stats/culture) are recomputed from persistent structure state.

## Trade Link State (Conceptual)

Each trade link should be a fixed-size 16-byte record, matching the compact settlement target for memory discipline.

Suggested 16-byte shape:

1. link_id (1 byte)
2. location x, y (2 bytes: the travelling unit's current tile, derived/cached for rendering)
3. type (1 byte: caravan, fleet or pirate)
4. from_settlement_id (1 byte: canonical endpoint, never swapped)
5. to_settlement_id (1 byte: canonical endpoint, never swapped)
6. status_flags (1 byte: active, disrupted, blocked, recovering, returning)
7. health (1 byte, 0-255)
8. throughput (1 byte, 0-255)
9. risk (1 byte, 0-255)
10. path_cost (1 byte, 0-255)
11. range_or_distance (1 byte, 0-255)
12. owner_or_controller (1 byte)
13. last_event_tag (1 byte)
14. cooldown_or_recovery (1 byte: also the endpoint dwell timer)
15. progress (1 byte: distance travelled this leg, 0..path_cost)

Notes:

- Endpoint ids are a fixed canonical pair -- they are never swapped. Travel
  direction is the `returning` status flag: clear = from -> to, set = to -> from.
  On arrival the unit dwells (cooldown_or_recovery), then the flag flips and
  progress resets, so the same record shuttles back and forth without a
  separate direction field or a mutated endpoint pair.
- Directional asymmetry, if needed later, hangs off the stable from/to anchor
  plus the returning flag rather than off which id currently sits in `to`.
- Use flags + small scalar fields for deterministic, low-cost updates

## Legacy

Ruins, monuments, and abandoned infrastructure are part of the world memory.

- They record prior high points and collapse paths
- They create exploration and recolonization opportunities
- They reinforce the theme: no settlement is permanent, but nothing is fully erased

# Implementation Notes

These rules define business logic intent and should remain UI-agnostic.

- Core simulation code must be testable in isolation.
- Rendering and interaction layers should consume simulation outputs, not own simulation truth.
- Any temporary compatibility layer that still touches direct stats is transitional and should be removed as structure-first migration completes.
- Trade links should be updated in deterministic world ticks and validated by invariants (valid endpoints, bounded fields, and stable behavior under disruption).

