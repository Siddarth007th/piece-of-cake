# The Long Way to Cake — local redesign

**The owner authorized distribution after testing; each updated binary must pass runtime validation before publication.** This version replaces the open-air route with enclosed districts and a more demanding movement/combat loop. Nori is an original ninja rabbit with cream ears and face, an indigo cloth outfit, light wraps, a red headband and flowing scarf, looking for an ordinary slice of vanilla cake.

| District | Interior and mood | Main challenges |
|---|---|---|
| Sunroot Vault | Warm stone, roots, ferns and lime lamps | Learn double jump, dash and vent timing |
| Bellows Foundry | Copper pipes, orange vents, heavy masonry | Timed heavy presses, charging guards and kickable bombs |
| Fern Archives | Timber surfaces, bookshelves and hanging foliage | Two sentry courts with guard-locked gates, moving landings |
| Echo Sanctum | Violet stone, banners and glowing inlays | Timed Echo bridges and guardians |
| Prism Grotto | Blue rock strata, crystals and hanging formations | Wider weaving islands, shorter gaps and halfway checkpoints |
| Tidal Gallery | Teal ceramic, canals and wall panels | Three vertical lifts, bomb throwers and Echo crossings |
| Clockwork Descent | Brass pipes and amber machinery | Crumbling floors, sweepers and chained movement |
| The Cake Chamber | Warm tiles, red banners and gold trim | A final mixed gauntlet, then the cake |

Each district has three encounter rooms with checkpoints at their entrances. The 192 route beats include continuous walking surfaces, combat spaces, ordinary gaps, 200 cm rises and wide dash crossings. Checkpoints never share a bomb, vent or sweeper. Eight relic detours remain optional. Grounded platforms have visible borders; every gap has an enclosing ceiling and walls. Shadows, contact shadows and ambient occlusion are disabled at every graphics setting.

## Controls and movement

| Action | Keyboard/mouse | Controller |
|---|---|---|
| Move | WASD | Left stick |
| Rotate camera | Arrow keys, or tap F then move mouse; F again locks | Right stick |
| Center behind Nori | X | R3 |
| Jump, then double jump | Space; press again in the air | A / Cross |
| Dash | Q or right click | RB / R1 |
| Sprint | Shift | LB / L1 |
| Spin attack / kick a bomb | J or left click | X / Square |
| Slide / air slam | C or Ctrl | B / Circle |
| Echo / interact / eat cake | E | Y / Triangle |
| Pause / checkpoint | Esc / R | Menu / Options |

Run speed is 820 cm/s; sprint speed 1100 cm/s. Jump impulse is 650 cm/s and gravity 1568 cm/s². One additional jump resets vertical velocity in the air; landing or a bounce restores it. Coyote time is 0.16 seconds and the jump buffer 0.18 seconds. Short taps retain at least 0.12 seconds of lift before a gentler upward-speed cut; held jumps retain full height.

The gameplay camera holds its angle with a fixed 420 cm boom and 82 degree field of view. Arrow keys rotate it directly. Tap F once to enable mouse look and tap it again to lock; middle-drag remains available. X (or R3) smoothly centers behind Nori and locks mouse look. Right-stick look remains supported. The browser uses pointer-lock-free mouse input, and its Center camera button is equivalent to X. Mouse/stick sensitivity and invert Y are adjustable in Pause → Settings; arrow up always looks up. Pausing clears mouse-look mode. Running and dashing never auto-rotate or zoom the camera. Wall collision may shorten the boom to avoid clipping.

Three pixel hearts show health. Bombs and enemies remove hearts and launch Nori away. Every fatal fall immediately empties all hearts, shows a brief death cue, then restores three hearts at the checkpoint. There is no limited life counter.

Body lean follows turns, the torso stretches during jumps and squashes on landing, arms swing through the run, and the spin twists the full body twice. There is no grapple or hanging-ring move; the user clarified that “swing” meant body animation.

Dash lasts 0.20 seconds at 1550 cm/s, with a 0.9-second cooldown and one dash per airborne stretch. It briefly suspends gravity and hits nearby enemies or kicks bombs, but grants no blanket damage immunity. The HUD shows when dash is available. Slide lasts 0.65 seconds; the same input becomes a downward slam while airborne.

## Encounters

Vents use a shared, pause-aware room clock: 1.05 seconds of amber warning, 1 second of red danger, then a teal crossing window. Rotating orange arms require jumping over or timing a passage around them. Crumbling platforms shake for 2.4 seconds before falling and reform after five seconds so players can return for missed shards.

Floor bombs arm on approach and show their 360 cm blast footprint during a 1.6-second fuse. Some are triggered by reusable pressure plates. Close blasts deal two health points; outer blasts deal one. They launch Nori up and away with distance-scaled force, and ignite nearby bombs with a 0.4-second chain fuse. A spin launches a bomb forward with a short remaining fuse; its explosion can defeat enemies. Bomb throwers mark a player's position with a bomb whose fuse allows time to move away. A fixed pool limits spawned bombs.

Charging guards lock their direction during a 0.7-second warning and cannot chase across gaps. Armoured guardians need two bonks or one slam and punish close approaches with a ground burst. Flying bomb throwers pressure stationary players. Enemy defeat uses a squash and particle pop. Checkpoint recovery restores enemies, temporary platforms, bombs, movement resources and three hearts; collected shards remain collected.

Echo stones activate their linked bridges for 18 seconds. Expiring bridges pulse and never remove support directly beneath the player. The final approach has actual 25 cm stair treads and additional enemies that can knock Nori back down. At the cake room, each of two side switches consumes 180 shards. Insufficient funds consume nothing; players can go back for more, with return Echo controls available. Both switches raise a physical door, then E at the cake starts the eating animation and completion menu. Paid switches and collected shards survive checkpoint deaths. Restart clears the run and shard spending.

A short, skippable opening shows Nori thinking about a slice of cake, with a visible thought bubble and a blended return to movement.

## Validation

The Python route checks establish geometric reachability and safe checkpoint placement only. Actual movement, encounter interactions, collision, ending, restart and frame pacing are tested in Unreal. See TEST_RESULTS.md for current runtime evidence and outstanding issues. The target first-play duration remains 10–15 minutes; an automated route traversal is not a measure of a new player's experience.


## Developer flight and the opening practice sequence

Type `siddarthisgod` during gameplay or while paused to toggle developer flight. No console is needed. The phrase is case insensitive and allows three seconds between letters. Flight controls: WASD moves on the horizontal plane, Space rises, C or Ctrl descends, and Shift increases speed. Tap F to enable mouse look, or use the arrow keys. X centers behind Nori. The green developer banner includes coordinates. Collision and damage are disabled in this mode; ordinary collectibles and checkpoint triggers are suspended. Type the phrase again to land at the current position when there is clear, walkable ground below; an unsafe exit stays in flight. R explicitly returns to the checkpoint. E beside the cake previews the ending without buying switches or fabricating shards. The run stays developer-assisted and excluded from the leaderboard even after flight is turned off. This mode starts off on every new journey and is never used to certify beatability.

The first practice room now has no enemy or timed vent, a checkpoint immediately before its first gap, a lower 80 cm introductory rise and a shorter 260 cm practice gap. Quick taps retain at least 120 ms of lift, with a gentler jump cut; holding Space still gives full height. Coyote time is 160 ms and the jump buffer is 180 ms. Later encounters retain their harder double jump, dash, bomb and enemy demands.


## Distinct district encounters

- **Sunroot Vault:** a safe movement practice room, teal takeoff edges and a checkpoint before the introductory jumps. Later rooms introduce bombs and timing.
- **Bellows Foundry:** three heavy presses with a four-second warning/slam/open cycle. Amber announces the drop; teal marks the crossing window. The press physically blocks the route while lowered.
- **Fern Archives:** two wider sentry courts. Each gate belongs to three guards and opens only after those guards are defeated. Spin attacks and jumping create space during the fight.
- **Echo Sanctum:** temporary bridges activated from either end, so the route supports returning for shards.
- **Prism Grotto:** 1250 cm wide islands alternate 360 cm from side to side. The player steers across the room, with gaps capped at 340 cm, lower rises and an extra checkpoint halfway through each room.
- **Tidal Gallery:** three lifts travel 220 cm vertically. Wait for a low deck, board it, then choose when to leave. Lit guide rails identify lift travel.
- **Clockwork Descent:** a six-platform collapsing-floor sprint after a safe checkpoint, in addition to the earlier sweepers.
- **Cake Chamber:** guarded stairs, bombs and the two shard-funded switches before the cake.

Ceiling heights now vary from a low 950 cm industrial passage to a 2100 cm crystal chamber. All districts remain enclosed and free of cast shadows. Arena enemies reset on checkpoint recovery, along with their gate.

Medium now uses 80% internal rendering into a 1280×720 stream, while HUD text remains at stream resolution. High/Epic retain 100%. The current performance evidence still includes stutters; see TEST_RESULTS.md.


## v0.3 visual direction

Large structural arches and distinct room fixtures break up the corridor shells: warm garden lanterns and planters, foundry grilles, tall library shelves, purple sanctum windows, two-tone crystal clusters, tidal portholes, gearwork and velvet/gold chamber details. Surface courses are larger and mortar is softened. All new scenery is non-colliding and uses instanced meshes without cast shadows.

Nori has blended strides and airborne poses, responsive ears, a visual somersault on the second jump and short spin ribbons. Guards have clearer silhouettes and a visible wind-up marker. The cake slice has icing detail and a dressed plinth; the completed-game portrait frames Nori beside the result menu. Normal gameplay retains the manually controlled camera.
