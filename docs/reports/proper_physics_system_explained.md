You are completely right to call this out. Your instincts are spot on: the current implementation is essentially a "brute force" approach.

Right now, your `FindClosestCollision` function takes a moving entity and loops through every single static entity in the entire level. If you have 10 enemies moving, and 1,000 rocks/trees on the map, your engine is performing 10,000 collision checks every single frame. It works for a small prototype, but it is mathematically unoptimized and will tank your framerate in a real level.

Here is a breakdown of how professional 2.5D physics engines are traditionally architected, what they handle, and how they decouple from game logic.

### 1. The Traditional Physics Pipeline

A proper physics system is divided into strict phases to handle performance and accuracy.

* **Integration (The Intent):** The game applies velocity to an entity.
* **The Broad Phase (The Filter):** This is what your engine is currently missing. Instead of checking every rock, the engine queries a spatial data structure (like a Spatial Hash Grid or a QuadTree). It asks, *"Give me only the static hitboxes that are within 50 pixels of this moving entity."* This reduces the checks from 1,000 down to maybe 4 or 5.
* **The Narrow Phase (The Math):** This is your `SweptAABB`. Much like a real-time target tracking system, it takes the velocity vector and predicts the exact future point of impact.
* **The Resolution (The Response):** If an impact is predicted, the engine resolves it. Swept AABB acts a bit like a mini time loop inside the frame—it moves the entity forward to the exact moment of impact, kills the perpendicular velocity, resets the remaining time, and runs the simulation again to slide the entity smoothly along the wall.



### 2. What the Physics System is Supposed to Handle

In a 2.5D game, the physics engine is typically responsible for three distinct types of interactions:

* **Kinematic vs. Static (Sliding):** Moving entities (Player, Enemies) colliding with non-moving environment (Rocks, Walls). This is what your code currently does.


* **Kinematic vs. Kinematic (Pushing/Blocking):** Moving entities colliding with *other* moving entities. For example, the player bumping into a Goblin, or two Goblins bumping into each other so they don't overlap into a single blob.
* **Triggers / Sensors (Overlapping):** Invisible hitboxes that do not stop movement but need to register overlap. For example, a damage zone, a transition door to a new map, or the aggro-radius of an enemy.

### 3. How the Game Interacts with It

The golden rule of engine architecture is that **the physics system does not know what a game is.** It only knows about rectangles and velocity.

* **How Gameplay Moves Things:** Your `character_movement` system or player controller never touches `TransformComponent.pos`. They only read input and set `VelocityComponent.velocity`. The physics system takes over from there, ensuring the transform only updates to valid, non-colliding positions.
* **How Gameplay Reacts:** Instead of hardcoding game logic into the physics loop, the physics system uses an Event Dispatcher (which EnTT has built-in). When a sword hitbox overlaps a goblin hitbox, the physics engine simply broadcasts an event: `CollisionEvent(EntityA, EntityB)`.
* **The Handoff:** Your game has a system (e.g., `CombatSystem`) listening for that event. It checks if Entity A is a sword and Entity B is an enemy, and if so, applies damage. The physics engine remains completely ignorant of health, swords, or stats.

---

To refine your `UpdateKinematics` system and get it up to industry standards, we need to replace that brute-force loop. Which optimization step do you want to tackle first: implementing a Spatial Grid for the Broad Phase to fix the performance, or adding Dynamic-vs-Dynamic collisions so entities can bump into each other?