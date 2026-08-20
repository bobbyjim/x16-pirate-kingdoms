# Pirate Kingdoms

Core Concept: Pirate Kingdoms is a simulation of emergent civilization and impermanence, where players interact with a world driven by fluid cultural dynamics, resource management, and strategic decision-making. The game emphasizes the rise and fall (churn) of settlements, with player actions influencing but not fully controlling these natural cycles.

# I. World Simulation & Dynamics:

## Settlement Attributes:
*   Population: 4 bits (0-16) a size value that influences economics and military strength. 
			1 is an encampment, 16 is a major city.
*	Wealth: 4 bits (0-16) economic resources, impacting trade and development.
*	Reserves: 4 bits (0-16) stored resources, which can be used to weather events or invest in growth.
*   Infrastructure/Resiliency: 4 bits (0-16) a settlement's ability to withstand environmental and social
			challenges, such as storms, plagues, or civil unrest.
*   Defense: 4 bits (0-16) a settlement's military strength and fortifications, affecting its ability 
			to repel attacks or maintain order.

## Cultural Vector:
*   Dynamic Cultural Vectors: Each settlement possesses a "cultural vector" defined by four primary focuses: Merchant(TRA), Aggression (AGR), Growth (GRO), and Security (SEC).
*   Focus Components: These focuses are granular numerical values (likely 8-bit, 0-255) that sum to a constant, enforcing trade-offs. A city cannot excel in all areas simultaneously.
*   Natural Evolution: A settlement's cultural vector naturally shifts over time based on its experiences, environment, and its interactions with neighboring cultures.
*   Neighbor Influence: A settlement's dominant cultural focus influences its neighbors. For example, a Merchant city might nudge neighbors towards trade, while an Aggressive city might provoke defensive or aggressive responses.

## Impermanence & Decline:
*   Cascading Events: A core mechanic involves a series of "events" that challenge settlements. These events are designed to test specific aspects of a settlement's cultural vector and infrastructure.
*   Event Types: Examples include Drought (tests reserves, impacts population), Plague (impacts population, infrastructure), Storm (tests infrastructure/resiliency), Pirates (tests Defenses, impacts wealth), Market Crash (tests reserves/merchant focus, impacts wealth), and Civil War (divides settlement, creates aggressive factions).
*   Weakness Exploitation: Events impact settlements based on their current weaknesses. A low Security focus makes a city vulnerable to Pirates or Civil War. Low Infrastructure makes it susceptible to Drought or Storms.
*   Erosion of Focus/Resources: The primary effect of events is to erode a settlement's focuses, population, wealth, or infrastructure, rather than causing instant destruction.
*   Refugee Cascades: When a settlement collapses or is severely weakened, it can generate "refugee events" on neighboring settlements, straining their resources and potentially triggering further cascades of decline.
*   Gradual Rise: All settlements rise to some degree over time, and can be a fast or slow process.
*   Eventual Fall: All settlements are subject to eventual decline and destruction, making longevity a strategic challenge rather than a given.

# II. Player Agency & Interaction:
## Strategic Decision-Making:
*   Focus/Priority Setting: Players can directly influence a settlement's cultural vector by setting its "Focus" or "Priority." This is a key decision point, forcing players to commit resources and strategic direction.
*   Resource Management: Players manage their own resources (e.g., via a player-controlled ship) to trade, gather, or transport commodities, directly impacting the game economy and potentially influencing settlement development.
## Combat:
*   In-Map Real-Time: Combat occurs directly on the main game map in real-time, providing immediate visual context.
*   Tactical Control: A "slowdown" mechanic allows players to issue commands and execute maneuvers with precision during combat.
*   Visceral Feedback: Combat includes visual and auditory cues to convey impact, damage, and destruction.
*   Consequences: Combat results can lead to appropriation of wealth, capture of settlements, or destruction of units.
## Trade & Economy:
*   Natural Market: The game features a baseline economic simulation where settlements produce and consume goods, creating inherent trade opportunities.
*   Player Trading: Players can directly engage in trade by buying low and selling high with their own assets.
*   Economic Influence: Settlement focuses (especially Merchant) and player actions influence regional wealth and trade flow.
## Territorial Control & Influence:
*   Cultural Dominance: While not explicit "empires," settlements exert influence based on their cultural vector, shaping the geopolitical landscape. Aggressive cities may seek to dominate, while Merchant cities foster interdependence.
*   Settlement IDs: Each city has a unique identifier, facilitating tracking and interaction.

# III. World State & Legacy:
*   Map Data: A 256x256 grid with 2 bytes per cell, storing detailed terrain types, resource information, and settlement IDs.
*   Settlement Attributes: Each settlement stores its coordinates, population order of magnitude, infrastructure/resiliency, defense, security, cultural vector components (TRA, AGR, GRO, SEC), a potential culture/personality modifier, owner ID, and current event status.
*   Ruins & History: The remnants of fallen settlements provide a lasting "history" within the game world, offering opportunities for exploration, resource scavenging, or even the potential to reestablish control.

