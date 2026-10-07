# The Long Way to Cake

Nori is a small cream-colored creature with teal tapered ears, large dark eyes,
a red scarf and a worn adventurer's pack. Nori's ambition is completely ordinary:
eat the slice of vanilla cake at the terrace above the meadow.

## Journey

| Section | Palette / scale | Play focus |
|---|---|---|
| Meadow | Warm green, broad platforms, distant basin | Walk/run/jump, bonk, bounce tutorial |
| Forgotten ruins | Blue stone, amber lanterns | Moving platforms, wandering enemies, pulse traps |
| Ancient forest | Giant teal canopies, roots and falls | Ascending jumps, moving landings, bounce chains |
| Listening temple | Huge pillars, warm inlays | Echo bridges, guardians, slide gates, pulse traps |
| Underground caverns | Deep blue roof ribs and cyan crystals | Narrower atmosphere, moving landings, hazards |
| Sunken city | Monumental towers, blue/gold | Camera widening, flight nuisances, Echo traversal |
| Long way down | Crumbling causeway | Sustained movement, sliding, bouncing, pulse timing |
| One last climb | Quiet warm stone | No enemies, final ascent, ordinary cake |

The route is about 3 km in Unreal world units. Its 192 platforms run continuously
around a basin, ending on a higher terrace near the start. There are 24 safe
checkpoints, 576 common shards, 8 optional relic detours and 18 enemies. No secret
is required for completion. A complete first run is **targeted** at 10–15 minutes;
this has not been timed and must not be presented as a measured duration.

## Movement tuning

- Run 620 cm/s; sprint 840 cm/s; analog gamepad input supplies walking speed.
- Acceleration 3200 cm/s²; walking brake 2800 cm/s²; air control 0.7.
- Jump impulse 650 cm/s; gravity 1.6 × 980 cm/s².
- Coyote window 0.12 s; input buffer 0.14 s. Release cuts upward velocity for short hops.
- Slide lasts 0.65 s and lowers collision height. In air the same button slams downward.
- Bonk has a short cooldown and forward reach; aerial bonk redirects momentum.
- Landing on an enemy from above defeats/staggers it and bounces the player.
- Ledge assist checks surface normal and capsule clearance before a short step-up.
- Three hits cause respawn; individual hits grant temporary invulnerability.
- Falls respawn at the latest checkpoint. Collected shards and defeated enemies
  remain accounted for during the same run.

Geometry checks reserve a 45 cm takeoff/landing margin at each edge, account for
height changes, and allow a conservative 300 cm lateral moving-platform offset.
This only establishes mathematical feasibility; controller feel, actual collisions
and frame-rate behavior need engine validation.

## Echo

Approach a cyan Echo stone and press E/Y. Its two linked platforms become solid for
18 seconds. They brighten/pulse near expiry, and a bridge won't remove collision
directly beneath the player. The player can reactivate a stone; no inventory key or
long puzzle is required. Respawn resets all temporary platforms.

## Danger and rewards

Moss wanderers patrol a fixed platform. Stone guardians take two bonks or one slam.
Flying nuisances bob above a platform and can be hit in the air. Enemies telegraph
for 0.65 seconds before their attack. Pulse traps also visibly warn before activation.
Defeat is a squash and particle pop, with no gore.

Collapse platforms shake, then fall after 2.4 seconds of contact. Safe landings
and checkpoints break the sequence into forgiving stretches. The relic departure
in this section is a permanent platform. The final ascent is deliberately calmer.

Press E/Y near the final table to begin the brief eating animation; the cake
disappears after the bite, completion is recorded, and the actual elapsed time and
collected counts appear. Restart creates a fresh journey.

## Presentation limits

The character/scenery animation is a procedural first implementation, not a finished
skeletal animation pipeline. The generated music consists of short original loops.
The opening cake reveal, visual transitions, camera readability, enemy telegraphs,
collapse pacing and eating animation all require visual validation and refinement
inside Unreal before this meets the intended commercial-prototype quality bar.
