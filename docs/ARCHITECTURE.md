# Architecture

## Runtime ownership

`APOCGameMode` selects `BP_Nori` when generated and spawns `APOCWorld` after play
starts. The native character remains the fallback. `APOCWorld` reads staged JSON,
creates the entire lightweight route and modular scenery, and owns its props,
enemies, atmosphere, sound and bounded particle pool.

`APOCCharacter` uses Unreal CharacterMovement for collision/floor/base movement,
acceleration and air control. It adds jump buffering/coyote time, short-hop release,
bonk/air bonk, crouched sliding, slam, enemy bounce and a small ledge assist.
SpringArm collision and camera lag handle follow framing. The character consists of
original proportions assembled from engine primitives, with procedural gait,
blinking, breathing, scarf motion, squash and attack/eating motion.

`APOCProp` is a small explicit behavior enum. It handles collectible pickup,
checkpoints, Echo nodes/bridges, moving platforms, crumble timing, hazards, bounce
pads, slide gates and the cake. `APOCEnemy` handles three telegraphed enemy behaviors.
Neither relies on a NavMesh or uncontrolled physics simulation.

`APOCController` owns the title, play, pause, settings, controls and completion
states. `APOCHUD` draws their real interactive controls. The controller continues
ticking while the game is paused. Mouse, keyboard and gamepad menu actions enter
the same action handlers.

`UPOCGameInstance` owns a run's collectible IDs, checkpoint-independent scores and
progress. `UPOCSaveGame` serializes settings, completion and best counts. A fresh
journey clears run state while retaining these persistent values.

## Level data and asset pipeline

`Scripts/generate_journey.py` is the authoritative route source. It builds an
eight-sector basin route, including explicit flags for enemies, hazards, Echo
groups, secrets and safe checkpoints. `Content/Data/journey.json` is staged as UFS
data during packaging and read with Unreal file APIs.

`Scripts/bootstrap_unreal.py`, run inside the compiled Editor, creates actual
Unreal material/audio/Blueprint assets and the map. It is idempotent and preserves
existing assets. These binary assets do not exist until a successful editor run.
`BP_Nori`, `BP_EchoObject` and `BP_Guardian` inherit working native behavior and are
loaded by the runtime for subsequent designer edits.

Scenery and ordinary platforms use hierarchical instancing, grouped by material
and shape. Moving/interactive objects remain actors. Their tick is disabled beyond
6–8k cm and restored as the player approaches. A pool of 72 mesh particles bounds
effect allocation. Collision is on the traversal surfaces; decorative geometry is
nonblocking. This avoids hidden blockers, though visual/collision matching still
needs an engine playthrough.

This compact implementation keeps the route in one map. It does not claim to use
World Partition/streamed sublevels. Culling and tick activation limit the small
procedural scene; profile before adding high-resolution production assets.

## Browser and host

```text
Browser: HTML/CSS + Epic UE5.8 frontend
  │ HTTPS document / WSS signalling
  ▼
Caddy TLS ──► Node HTTP + Epic SignallingServer
                    │ loopback ws://127.0.0.1:8888
                    ▼
             UE5.8 packaged native GPU process
                    ║ encrypted WebRTC video/audio/data
                    ╚════════════════════► Browser
                         direct or via authenticated coturn
```

The page has no local game renderer, fallback gameplay or fake progress meter.
It switches to play state only on the frontend's actual `playStream` event.
Keyboard, mouse and gamepad are forwarded by Epic's library. The checkpoint button
uses the library's registered KeyDown/KeyUp protocol handlers for the game's R key.

`/readyz` requires the `piece-of-cake` streamer to identify itself and advertise a
free seat. This is registration evidence, not a media health test. The stream port
is loopback-only; the browser WebSocket enforces exact allowed origins. No arbitrary
console command channel is implemented. TURN clients receive HMAC credentials,
never the shared relay secret.

## Version policy

Engine 5.8 is pinned together with `lib-pixelstreamingfrontend-ue5.8@0.1.2` and
`lib-pixelstreamingsignalling-ue5.8@0.2.0`, with an npm lockfile.
[Epic's infrastructure repository](https://github.com/EpicGames/PixelStreamingInfrastructure)
requires matching engine/infrastructure branches. Its
[Pixel Streaming 2 migration guide](https://github.com/EpicGames/PixelStreamingInfrastructure/blob/UE5.8/Docs/pixel-streaming-2-migration-guide.md)
documents the public API changes and `PixelStreamingSignallingURL` launch flag.

An engine upgrade is a deliberate project + npm + bootstrap + packaging + browser
verification change. The wrapper fails if a different minor engine is selected.
