# Piece of Cake

**A tiny adventurer. An enormous journey. One perfectly ordinary dessert.**

Project location: `~/Desktop/PieceOfCake`.

## Current status — 7 October 2026

**This is a source implementation, not a validated playable release.**

The Unreal C++ project, deterministic eight-section route, asset generator, original
audio, browser player, real signalling server, and deployment configuration are here.
The browser frontend builds and the route/configuration/signalling tests pass.

**Unreal Engine is not installed on the development Mac.** Consequently the native
module has not been compiled, the `.uasset`/`.umap` files have not been generated,
and no packaged game or browser game stream has been tested. Gameplay, duration,
performance and graphics quality remain unverified. **No cloud deployment exists.**

| Deliverable | Evidence |
|---|---|
| Unreal 5.8 project and gameplay source | Written; engine compilation pending |
| 192-platform continuous route, 24 checkpoints, 8 secrets | Generated; geometric checks pass |
| Character, movement, enemies, Echo, cake, menus, saves | Implemented in source; runtime testing pending |
| Original audio and title illustration | Generated; asset format checks pass |
| Browser player | Production build passes; desktop/mobile UI inspected |
| Signalling server | Local HTTP/WebSocket protocol tests pass |
| Unreal video/audio/input through WebRTC | **NOT TESTED** — requires running Unreal |
| Packaged native build | **NOT CREATED** |
| Cloud deployment / HTTPS game URL | **NOT DEPLOYED** |

See [test results](docs/TEST_RESULTS.md) for the exact distinction between checks that
ran and the acceptance work still pending. The landing-page illustration is original
vector art; it is not a screenshot of the game.

## Start here on this Mac

1. Install **Unreal Engine 5.8** with the [official Epic Games Launcher](https://www.unrealengine.com/download).
   Sign in and complete Epic's license prompts yourself. Xcode is already installed.
2. If installed somewhere other than `/Users/Shared/Epic Games/UE_5.8`, set `UE_ROOT`
   in your terminal to the directory containing `Engine/`.
3. Double-click **Check Setup.command**. It checks for the actual engine and exits
   unsuccessfully if it is missing.
4. Double-click **Open Unreal.command**. The first run compiles the native module,
   generates real materials/audio/Blueprint assets and creates `L_LongWayToCake`.
   This step may take time and disk space; build errors must be fixed, not ignored.
5. In Unreal, open `/Game/Levels/L_LongWayToCake` and press Play.
6. For the browser: double-click **Play Piece of Cake.command**. It checks for Unreal,
   builds the project, starts both Unreal and the signalling server, and opens the browser
   only after Unreal registers. Keep its terminal open; Control-C stops both processes.
   The separate **Start Browser Player.command** and **Stream Unreal.command** remain
   available for debugging. Close their terminals before using the combined launcher.

The browser page alone is not the game. It disables Play and reports **Not playable yet**
until Unreal connects. Availability refreshes automatically every five seconds. A normal end user's browser needs neither Unreal nor a game
download once a working GPU host is deployed.

## Command-line workflow

```sh
cd ~/Desktop/PieceOfCake
python3 Scripts/ue.py doctor
python3 Scripts/ue.py prepare       # build C++; generate actual Unreal assets/map
python3 Scripts/ue.py editor
python3 Scripts/ue.py play
python3 Scripts/ue.py test          # Unreal automation, requires engine
python3 Scripts/ue.py profile       # writes Artifacts/Gameplay.utrace
```

For a custom installation, first use:

```sh
export UE_ROOT="/path/to/UE_5.8"
```

`bootstrap_unreal.py` preserves existing Unreal assets, so artists can edit the
generated assets without the next bootstrap overwriting them. `generate_journey.py`
regenerates the JSON; edit its source to make permanent layout changes.

## Controls

| Action | Keyboard / mouse | Xbox-style gamepad |
|---|---|---|
| Move / walk | WASD | Left stick; analog walk |
| Camera | Mouse | Right stick |
| Jump / variable height | Hold/release Space | A |
| Sprint | Hold Shift | Hold LB |
| Bonk / aerial bonk | J or left mouse | X |
| Slide / crouch; slam in air | C or Ctrl | B |
| Echo / cake interaction | E | Y |
| Pause / settings | Escape | Menu |
| Restart checkpoint | R | Pause → Restart checkpoint |
| Navigate menus | Arrows + Enter; mouse | D-pad + A |
| Adjust settings | Left/right; click/right-click | D-pad left/right |

PlayStation equivalents: Cross, Square, Circle, Triangle, L1, Options.
Bindings are editable in `Config/DefaultInput.ini`; no in-game rebinding UI yet.
Press Escape to release browser pointer lock; click the video to capture it again.

Settings include music/effects volume, sensitivity, inversion, captions, graphics
quality and window mode. Browser volume controls received audio, separately from
the in-game mix. Desktop keyboard/gamepad is the target; touch gameplay is not implemented.

## Stack and structure

- Unreal Engine 5.8, C++, Blueprint subclasses for designer tuning.
- Engine static primitives, instanced modular scenery, generated materials,
  procedural character animation, synthesized PCM audio.
- Pixel Streaming 2; matching Epic UE5.8 frontend `0.1.2` and signalling `0.2.0`.
- TypeScript, Vite, Node 22.12+ (tested with Node 24.14), native WebRTC media/input.
- Linux native GPU game process, Dockerized web/signalling, Caddy HTTPS, coturn relay.

```text
PieceOfCake.uproject
Config/                         engine, input, packaging settings
Source/PieceOfCake/
  Public/                       gameplay interfaces and tuning properties
  Private/                      character, world, props, enemies, HUD, persistence
  Private/Tests/                Unreal automation tests (not yet run)
Content/
  Data/journey.json             deterministic continuous layout
  Audio/Source/                 15 effects and 8 original music loops
  Characters/ Environments/ Materials/ VFX/ UI/ Gameplay/
  Levels/ Pickups/ Enemies/ Cinematics/  bootstrap asset destinations
Scripts/                        engine wrapper and asset/data generators
Web/                            browser UI, Epic frontend, real signalling server
Deploy/                         Docker, HTTPS, TURN, native game service
Tests/                          route geometry and configuration checks
docs/                           architecture, gameplay, deployment, test status
Artifacts/                      ignored local validation outputs
```

## Packaging

On a machine with the matching native target toolchain:

```sh
python3 Scripts/ue.py package --target Mac --configuration Shipping
# On a Linux Unreal host:
python3 Scripts/ue.py package --target Linux --configuration Shipping
# On Windows with Visual Studio's UE C++ prerequisites:
python Scripts/ue.py package --target Win64 --configuration Shipping
```

Archives are written under `Builds/<platform>/`. The wrapper rejects unsupported
cross-packaging requests. A Mac package cannot be run on a Linux cloud GPU host.
Run the packaged executable and complete the manual acceptance list before shipping.

## Local Pixel Streaming

Terminal one:

```sh
cd Web
npm ci
npm run build
npm run signal
```

Terminal two, from project root:

```sh
python3 Scripts/ue.py stream
```

To stream a packaged build, set `GAME_EXECUTABLE` to its native executable/launcher
before running `ue.py stream`. The Unreal streamer ID must be `piece-of-cake`.
Signalling listens locally on 8888; the browser page and `/signal` WebSocket share
8080. Only one player may subscribe to the single Unreal process.

```sh
node Deploy/healthcheck.mjs http://127.0.0.1:8080
```

`/healthz` means Node is alive. `/readyz` returns 503 without an identified Unreal
streamer, 409 when occupied, and 200 when a seat is advertised. **Even a readiness
pass does not prove video, audio, controls, or the game works.**

## Tests

```sh
python3 -m unittest discover -s Tests -v
npm --prefix Web test
npm --prefix Web run build
```

The protocol integration test briefly opens localhost ports 18080 and 18888 and
uses an explicitly labelled test WebSocket peer. It sends no game media and is
not counted as a Pixel Streaming playthrough. Production always requires Unreal.

## Deployment and server requirements

Follow [docs/DEPLOYMENT.md](docs/DEPLOYMENT.md). The configuration targets a single
Linux x86_64 GPU host. A reasonable **starting budget to profile**, not a measured
requirement, is 8 CPU threads, 32 GB RAM, 150 GB SSD, and an NVIDIA GPU with NVENC
and 12+ GB VRAM. Start at 1600×900, 60 fps, High, then tune using real measurements.
The game must be packaged for Linux on a compatible Unreal build host.

Required values: `GAME_DOMAIN`, `PUBLIC_ORIGIN`, `ACME_EMAIL`, `PUBLIC_IP`,
`TURN_URL`, `TURN_SECRET`, `GAME_EXECUTABLE`. No real secrets are included.
Local optional values: `UE_ROOT`, `PLAYER_PORT`, `STREAMER_PORT`, `WEB_ROOT`.

Cloud URL: **none**. `https://cake.example.com` in examples is a placeholder.

## Troubleshooting and limitations

- **Engine not found:** install UE 5.8 and set `UE_ROOT`. Free disk is not an engine installation.
- **Missing map/material:** run `python3 Scripts/ue.py prepare`; inspect `Saved/Logs`.
- **Module won't compile:** keep build output, fix UE/API errors, rerun `prepare`. Source has not yet passed UBT/UHT.
- **Page loads, offline message:** launch the Unreal process with the correct streamer ID and URL.
- **Connected but no video:** inspect Unreal encoder logs, browser codec support and GPU driver. Try H.264.
- **Works locally but not externally:** verify HTTPS/WSS, TURN, public IP and media/firewall ports.
- **Second browser can't play:** intentional single seat. Independent sessions need one game process per player.
- **No sound:** use the player volume, in-game effects/music settings and browser playback permission.
- **macOS won't open `.command`:** from Terminal use `zsh "Open Unreal.command"`; do not disable Gatekeeper globally.
- **Controls stop:** click the stream to regain focus; Escape releases pointer lock. Gamepad requires a focused browser.
- **Performance:** not profiled. Procedural geometry/animation are a starting implementation, not final commercial art.
- **Duration:** 10–15 minutes is the design target. Route distance is checked; no first-playthrough timing exists yet.
- **Save isolation:** settings/best counts are local to the game host. A public single-instance server shares its save.
- **Browsers:** Chrome, Edge, Firefox and Safari are targets, but real WebRTC cross-browser testing remains pending.

See [architecture](docs/ARCHITECTURE.md), [gameplay](docs/GAMEPLAY.md),
[development](docs/DEVELOPMENT.md), and [asset provenance](docs/ASSETS.md).
