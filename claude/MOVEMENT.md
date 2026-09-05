# How Movement Works

Movement in the simulation is based on "ticks," which represent discrete units of time. Each entity in the simulation can move a certain distance per tick, and the rules for movement are applied at each tick. In this context, a "tick" is the smallest unit of time in the simulation, similar to how a "day" is used in the calendar system.

Typical land movement over regular terrain is 4 squares per tick. Movement on roads is 6 squares per tick.  In both cases, land movement is cardinal directions only.

Sea movement depends on the vessel type and its capabilities. Different types of ships have different movement rates per tick, and sea movement can occur in any direction, not just cardinal directions.  Movement typically ranges from 3 (barges) to 6 or more (fast ships) squares per tick.
