# v0.3.0 tested Mac preview — 8 October 2026

The exact final executable and cooked content were installed on the Desktop and archived only after three complete native gameplay regressions passed. The app includes the broader environment/animation pass, working developer land-here and cake shortcut, and live Supabase Free integration.

| Journey | Cake and restart | Mean FPS | p95 frame | Worst frame | Frames >50 ms |
| --- | --- | --- | --- | --- | --- |
| 1 | Pass | 59.98 | 16.76 ms | 26.48 ms | 0 |
| 2 | Pass | 59.99 | 16.77 ms | 17.73 ms | 0 |
| 3 | Pass | 59.98 | 16.77 ms | 56.81 ms | 1 |

All three runs landed on all 192 platforms without developer flight. Run 2 verified deliberate falls, empty hearts, checkpoint recovery, live bomb health loss and knockback. Run 3 verified pause/resume, Echo expiry/reactivation, rejected underfunded switches, backtracking to earn the missing shards, cake and restart. No frame exceeded 100 ms; one frame in run 3 took 56.81 ms. Its cause was not established and is retained in the results, so these measurements are not a promise of hitch-free play.

These are automated **real Unreal physics traversals**, not manual or browser-input playthroughs. Measurements apply to this Apple M4, macOS 26.7.1, native 1280×720, Medium (80% internal resolution), 60 FPS cap. They do not establish performance on every Mac or measure physical input-to-display latency.

The installed application passed developer movement, land-here, unsafe-exit protection, checkpoint rescue, normal shard gates and the unranked cake shortcut. Its title/story check observed 499 frames with zero invisible-character frames, no teleport/fall/damage and a grounded return to play. Actual rendered screenshots cover all eight districts; intentional screenshot-tour teleports are visual evidence only.

Live Supabase Auth/API/database ownership tests passed. The installed native app authenticated, saved progress and acknowledged an assisted run, then a fresh process with a fresh local cache restored its isolated QA Keychain session and downloaded the saved progress. QA data does not alter the owner's normal save. Profiles are opt-in and private per installation, with no email login or cross-device recovery yet.

Thirteen Python checks and five Unreal content/input/persistence tests passed. GitHub Actions also validates PostgreSQL ownership rules, seven optional web/signalling tests and the TypeScript production build; its status is available on the repository Actions page.

The complete arm64 ZIP is 368.7 MiB. The extracted bundle passes deep strict code-signature verification and matches the tested executable and cooked-content hashes. It contains the runtime and needs no Unreal installation or author-hosted server. It is ad-hoc signed, **not Apple notarized**; only Apple silicon Mac is supported by this download.

Archive SHA-256: `b849459d646d0910f7913b7ac6b7767dfdceb4d75cec4ffcadf9a22658c823e8`.

Current evidence: [build manifest](validation/v03-build-manifest.json), [run 1](validation/v03-journey-run-1.json), [run 2](validation/v03-journey-run-2.json), [run 3](validation/v03-journey-run-3.json), [flight](validation/v03-developer-flight-test.json), [presentation](validation/v03-presentation.json), [cloud write](validation/v03-cloud-native-write.json), [cloud reload](validation/v03-cloud-native-reload.json), [live API security checks](validation/cloud-api-validation.json).

## Historical results below

The following dated records describe earlier builds, including superseded blockers and interrupted tests. Use the v0.3.0 results above for the current download.

## 2026-10-08 v0.3 functional fixes and live cloud

- Native developer-flight checks pass: all six axes, safe landing at the explored position, unsafe exits staying in flight, explicit R checkpoint rescue, normal shard gates preserved, and E at the cake completing an assisted/unranked ending without fake shards.
- Presentation checks inspected 500 title/story frames with zero disappearing-character frames, a grounded return to play, unchanged health and no teleport.
- Supabase Free is deployed. Real HTTPS tests pass for private ownership, rejected forged writes, validated progress, idempotent run uploads, session refresh, leaderboard privacy and persisted cloud data.
- The packaged native app uploaded an assisted test run, emptied its outbox, then restored its Keychain session and downloaded cloud progress from a fresh process with a fresh local cache. Test credentials are isolated from the owner's ordinary game profile.
- Before the broader visual pass, normal run 1 completed all 192 platforms, both 180-shard seals, cake and restart without falling. Mean 59.99 FPS, p95 16.82 ms, worst 31.52 ms; zero frames over 33.34 ms.
- Baseline run 2 passed deliberate death, empty hearts, checkpoint rescue, live bomb damage/knockback, the full route and restart. Its measurement was interrupted by an externally opened pause menu, which was resumed manually. It remains functional evidence; the replacement build will receive clean performance runs.
- The updated Desktop Finder alias uses the Nori character icon and resolves to the installed application. The replacement application awaits final validation.

Evidence: `validation/v03-developer-flight-test.json`, `validation/v03-presentation.json`, `validation/cloud-api-validation.json`, `validation/v03-cloud-native-write.json`, `validation/v03-cloud-native-reload.json`.


## 2026-10-08 native app and cloud integration

- The previous complete native warm-up/polish run reached all 192 platforms, opened the cake door, completed the ending and restarted. It averaged 59.98 FPS; p95 16.75 ms, worst 34.60 ms, and no frames over 50 ms. See `validation/native-warmup-polish-run-1.json`.
- After the mascot title, thinking animation and cloud client were added, another complete native run passed all 192 platforms, 27 checkpoints, both reward switches, the ending and restart with zero falls. It averaged 59.56 FPS, p95 16.90 ms, worst 198.59 ms, with three frames over 100 ms. These spikes are retained in the evidence, not removed from the report. A stale game crash reporter was subsequently found consuming about one CPU core and stopped; the follow-up run also completed successfully. This observation alone does not prove it caused every spike.
- After stopping the stale reporter, the follow-up run passed all 192 platforms, both switches, cake and restart. It verified a deliberate fall, empty hearts, checkpoint recovery, live bomb damage and knockback. It averaged 59.53 FPS; p95 16.92 ms, p99 18.63 ms, worst 70.31 ms; 8 frames exceeded 50 ms and none exceeded 100 ms. Occasional spikes remain and are not described as flawless performance. See `validation/native-clean-session-run-2.json`.
- GitHub Actions source validation passed: https://github.com/Siddarth007th/piece-of-cake/actions/runs/37766407636.
- A subsequent CI run exposed a signalling-test race: a client close event can arrive before the server removes the streamer. The assertion now waits up to two seconds for the actual server state; 20 consecutive local integration runs passed. Updated `ws` to 8.22.0; all seven Web tests and the production build pass, and npm audit reports zero known vulnerabilities. The pushed code revision passed GitHub Actions: https://github.com/Siddarth007th/piece-of-cake/actions/runs/37769833331.
- The installed standalone app was rechecked with a fresh user profile: 498 presentation frames, zero disappearing-character frames, and all intro/health/grounded-state assertions passed (`validation/installed-app-presentation.json`). Engine initialization was 1.00 seconds with a warm file cache; this is not a cold double-click-to-interactive measurement.
- The shareable arm64 archive is 368.7 MiB (about 596 MiB installed). Its extracted app passes deep strict signature checks and matches the tested executable and cooked content. SHA-256: `96afb12b5b432a307165d91d440b4a6e6e62e995d2601c5fe98036079eae33d2`. It is ad-hoc signed, not Apple notarized; no public binary release has been published.
- Packaged presentation checks passed: no character teleport or fall, three hearts retained, a grounded return to play, and the thinking pose blends out. Screenshots caught a cropped thought bubble; the framing was corrected and checked again.
- A later code review found a genuine presentation bug: respawn invulnerability flicker could persist while the dream paused its timer. The fix suppresses that effect during the story and clears the initial spawn shield at the title/start transition. The final packaged regression checked 499 title/story frames: zero invisible frames, and all transition, health and grounded-state assertions passed. Earlier static screenshots alone did not establish this. See `validation/presentation-continuous-visibility.json`.
- A second native process successfully reloaded saved completion, best shards, sensitivity and music volume. Five Unreal automation tests pass, including persistent cloud outbox serialization.
- Python tests: 13 passed. Web/signalling tests: 7 passed. TypeScript and Vite build passed. PostgreSQL tests: 9 passed including the parent suite. They exercise the actual migration with two Auth identities under separate SQL roles.
- Cloud service is **not yet live**. The Free organization is created, but the owner still needs to set the project database password and submit project creation. Database tests are not evidence of a deployed API. No cloud save or public leaderboard availability is claimed until the real HTTPS checks pass.

These are actual packaged Unreal physics runs, not manual/browser-input playthroughs. The studio-rendered Nori icon is branding only. Results apply to the tested Apple M4/macOS 26.7.1 machine and the documented build; they do not establish bug-free operation on every Mac.


## Camera ease-of-use revision — 8 October 2026

Holding F while moving is no longer necessary: **tap F** to toggle mouse look, use the **arrow keys** to rotate directly, and press **X** (or click the browser's **Center camera**) to settle the view behind Nori. Middle-mouse drag still works; the controller uses the right stick and R3. Mouse look locks on pause. Running alone never rotates the camera. Sensitivity remains adjustable in the pause menu; normal mouse/stick vertical direction is consistent, and arrows always follow their labelled direction.

The native build and TypeScript/Vite build passed. A focused native controller-input test passed (`Artifacts/camera-control-test.json`): right/left held arrows rotated +67.83/-66.01 degrees in roughly 0.6 s; pitch clamped at +28/-65 degrees; recenter error was below 0.01 degrees; a single F tap enabled +51.65 degrees of mouse rotation; untoggled mouse movement changed yaw by zero; middle drag worked; pause cleared mouse mode; walking changed camera yaw by zero. These are actual Unreal runtime measurements with simulated native key/axis events, not browser mouse evidence.

Real Chrome testing found the underlying browser failure: pointer lock was rejected with `WrongDocumentError`, so the locked mouse controller never forwarded motion. The player now uses hovering mouse input; native F-toggle/middle-drag still decide when the camera may rotate. After reconnecting, actual mouse movement rotated the game view, a second F tap held it fixed, 45 brief Right-arrow presses changed yaw from 90 to 118.23 degrees, and a downward pointer drag reduced pitch from -20.79 to -60.39 degrees. Both the toolbar's Center camera and X returned to pitch -14/yaw 90. The final browser log had no new errors or warnings. Evidence: `Artifacts/camera-browser-check.json`, `Artifacts/camera-manual.log`, and the real engine screenshot `Artifacts/camera-controls.png`. The browser build was rerun successfully after the pointer-input fix. The live local game is left in normal manual control with the view centered and mouse look locked.

This camera-only revision does not replace the earlier full-route evidence or resolve the previously recorded streaming stutters. Release was on hold at the time of that camera check.

## Current result: easier crystals, distinct encounters and developer flight — 8 October 2026

Unreal 5.8.3 is installed and the native game runs. The crystal section that repeatedly caused falls now has 1250 cm wide landings (previously 800), 360 cm lateral shifts (previously 560), gaps capped at 340 cm, 60% lower rises and halfway checkpoints. Bombs, enemies and sweepers no longer overlap its weaving landing sequence. The safe opening has shorter gaps, a checkpoint before its first jump and forgiving short-tap jumps. New district encounters include timed presses, guarded arena gates, lifts and a six-platform crumble sprint.

### Complete gameplay regressions

Three actual Unreal physics traversals completed all 192 route beats, both 180-shard switches, the cake ending and fresh restart. Developer flight was off throughout. These are automated engine tests, **not three manual or browser-input playthroughs**, and do not establish that the game feels easy to a new player.

| Run | Traversal | Falls / respawns | Additional checks | Mean FPS / p95 frame time | Frames >100 ms |
|---|---|---|---|---|---|
| 1 | Complete, 351.24 s | 0 / 0 | Two sentry courts, three presses, three lifts, combat and Echo | 54.02 / 25.89 ms | 12 |
| 2 | Complete, 353.32 s | 1 / 1, deliberate | Fall immediately empties hearts; checkpoint restores; actual bomb damages and knocks back | 57.94 / 19.77 ms | 8 |
| 3 | Complete, 487.71 s | 2 / 2 | Pause/resume, Echo expiry/reactivation, insufficient shards rejected, backtracking to earn missing shards, return and unlock | 46.01 / 32.50 ms | 32 |

Evidence: `Artifacts/variety-final-run-1.json`, `-2.json`, `-3.json`, matching logs and `Artifacts/variety-regression.log`. Opening jump taps were 50 ms, with takeoff leads of 100, 150 and 80 cm. The final regression script fails if its gameplay acceptance fields are missing or false. **It is not a performance acceptance script: these runs still have stutters.**

Two isolated crystal checks crossed the full district plus its exit with zero damage, falls or respawns. The first averaged 57.57 FPS (p95 21.91 ms, no frames over 100 ms). After reducing Medium internal resolution from 90% to 80%, the second averaged 58.39 FPS (p95 17.29 ms, four frames over 100 ms, worst 220.70 ms). Each took 43.49 seconds. See `Artifacts/CrystalProbe/journey-run-1.json` and `Artifacts/TunedCrystalProbe/journey-run-1.json`. Both explicitly report `completed: false` and `section_probe_passed: true`: they start with a debug jump to the district and are not complete-journey proof. The three complete runs precede the render-only reduction and test-driver scan optimization; gameplay physics and layout did not change afterward.

### Developer flight and actual browser input

The native flight test passed six-direction movement, descent below the floor without death, damage immunity, safe return to checkpoint, restored collision/gravity and a normal 50 ms tap jump afterward (108.26 cm height). See `Artifacts/developer-flight-test.json`.

Actual Chrome keyboard events through Pixel Streaming then verified that typing `siddarthisgod` enables flight, Space raises Nori, and repeating the phrase disables flight and restores the checkpoint with three hearts and collision enabled. A normal jump afterward was accepted and landed after 0.681 seconds. The camera stayed at pitch -14/yaw 90, 82-degree FOV and 420 cm boom. These brief browser key presses supplement the native directional test; they are not a whole manual journey. Evidence: `Artifacts/developer-flight-browser.json`, `.log` and `Artifacts/final-manual.log`.

Flight controls are WASD horizontal, Space up, C/Ctrl down and Shift faster. Type the phrase again or R to leave flight safely. The final manual session has flight off and no automated driver. `Artifacts/easier-crystals.png` is an actual Unreal screenshot of the widened section, visited with a debug section jump solely for visual inspection.

### Performance and remaining limits

The final 1280×720 Chrome sample contains 110 one-second observations including connection/start menus, input checks, debug section jumps and native screenshot capture. Its 43.67 FPS mean counts the initial missing FPS sample as zero; it is a mixed interaction sample, not a clean traversal benchmark. It recorded ten freezes totaling 5.038 seconds, zero packet loss, 30 decoder drops and 375 browser presentation drops among 4764 frames. Audio bytes and nonzero energy prove receipt of sound; audible listening is unverified. Ten protocol responses ranged from 3–20 ms, **not motion-to-photon latency**. Raw evidence: `Artifacts/variety-browser-final-manual.json`. Earlier browser evidence at 90% resolution is preserved in `Artifacts/variety-browser-before-tuning.json`.

The lighter preset improves the isolated engine sample but **does not establish smooth browser gameplay**. Further frame/stream profiling, packaged-runtime validation, gamepad and broader browser checks remain outstanding. No public-network or downloadable release is validated. The previous Mac package failed to launch because `libtbb.12.dylib` was missing; the editor-based local game is what was tested here.

### Build checks

The final native build succeeded. All five native checks passed: OriginalMeshes, RouteAndPhysics, DeveloperPhrase, MemoryRoundTrip and ShardBudget (`Artifacts/final-native-tests.log`, `Artifacts/UnrealTests/index.json`). Thirteen Python checks passed after the crystal changes. The earlier TypeScript/Vite build passed and Web source was unchanged in this revision.

The live local handoff is Chrome at http://127.0.0.1:8080/, manually controlled at the opening. The in-app browser's Start interaction remains unreliable and is not a verified control surface. Keep the Mac game process and local server running for this preview.

---

## Historical reports below — superseded by the current results above

The following sections preserve earlier evidence and then-current pending items. Their references to “current” or “pending” apply to those earlier revisions, not this latest result.

## Ninja rabbit, hearts and manual camera — 8 October 2026

The current local build has a ninja headband, indigo outfit, wraps and animated scarf; pixel hearts; a fixed 420 cm follow camera with an 82 degree field of view; and explicit **hold F + mouse** camera rotation. Ordinary mouse motion does not rotate it. WASD remains unrestricted in both horizontal axes within the level's walkable geometry. The visible automated driver was stopped at the user's request for manual movement; the current stream runs without `-POCAutoRun`.

Real Chrome keyboard input verified W, A, S and D movement in both axes, accepted ground and double jumps, dash and spin. Pitch/yaw remained -14/90, FOV 82 and boom 420 through these checks. Recorded coordinates and evidence are in `Artifacts/free-roam-input-check.json`, `Artifacts/ninja-free-roam.log` and the actual engine screenshot `Artifacts/ninja-free-roam.png`.

Four native Unreal tests passed (content, meshes, persistence and shard budget). The browser TypeScript/Vite build also passed. A complete ninja-version run traversed all 192 platforms, paid 360 shards, opened the cake door, reached the ending and verified a fresh restart. It deliberately took a real bomb hit (two hearts lost, 1587 cm/s push and 1115 cm/s launch) and a fatal fall (zero hearts visible before checkpoint restoration). See `Artifacts/ninja-before-tuning-run-2.json` and `Artifacts/ninja-run-2.log`.

That complete run **did not pass the smoothness target**: 55.13 FPS mean, p95 24.82 ms, 12 frames over 100 ms, worst 254 ms. Heart geometry is now batched, distant/inactive flames avoid animation updates, and Medium renders at 90% internal resolution into the 1280x720 stream; High/Epic retain 100%. The next run's periodic engine timing improved, but it was stopped at platform 138 to switch to manual control. It verified pause/resume and Echo expiry/reactivation, **not** the final underfunded backtrack scenario. `Artifacts/ninja-run-3.log` is partial evidence, not a completed run; an older journey-run-3 JSON must not be mistaken for this run.

Remaining before release consideration: finish three complete regressions of the final build, validate the underfunded door/backtracking route, collect final frame/stream measurements, and finish packaged-runtime validation. No public release, upload or deployment is authorized. The old installation automation remains paused. The final handoff is the real Chrome player at http://127.0.0.1:8080/, manually controlled at the first checkpoint; the in-app browser Start interaction was unreliable and is not accepted as a verified control surface.

## Enclosed redesign: validation in progress, 8 October 2026

The new native module and procedural surface material compiled successfully. Twelve Python checks and six real signalling/server tests pass; the browser TypeScript/Vite build passes. These checks do not substitute for gameplay.

The first actual Unreal traversal completed all 192 route beats, the cake ending, and a fresh restart in 431.08 seconds. It recorded 56 double jumps, 23 dashes, 24 bomb kicks, 28 explosions, four Echo groups and one checkpoint respawn. Frame pacing averaged 59.94 FPS; p95 16.77 ms, p99 16.86 ms, worst 64.60 ms, with no frames over 100 ms during measured traversal. Source evidence: `Artifacts/redesign-initial-run-1.json` and `Artifacts/redesign-run-1.log`.

A live Chrome observation confirmed moving video, the enclosed scene and non-silent incoming audio. Its saved 353-second sample averaged 59.90 decoded FPS, zero reported WebRTC freezes or packet loss, but browser playback quality counted 701 dropped presentation frames out of 21,132 received frames. Ten protocol round trips were 3–17 ms; this is **not** motion-to-photon latency. End-of-run screenshot capture and world restart subsequently caused two stream freezes; they are outside the saved traversal sample. `Artifacts/redesign-browser-run-1.json` contains the actual measurements.

That first run exposed an invisible HUD caused by a Slate-only font without a UFont. The next build uses the actual Roboto font asset and the HUD was visually verified in the real stream. Full-resolution rendering and further interior details are now being tested. The first run is evidence for the gameplay redesign, not approval to release, and it precedes those last visual fixes.

# Runtime status — 8 October 2026

**Release blocked pending final frame-pacing, three complete regression runs,
packaging and public-network validation. No upload/deployment has occurred.**

Host: Apple M4, 16 GB RAM, macOS 26.7.1, Xcode 26.6; Unreal 5.8.3 CL58210709.

Confirmed runtime evidence:

- UBT/UHT build and actual Unreal asset bootstrap succeeded.
- Three native Unreal tests passed: Persistence.MemoryRoundTrip,
  Content.RouteAndPhysics, Content.OriginalMeshes. Report:
  `Artifacts/UnrealTests/index.json`.
- One complete automated engine-physics baseline traversed all 192 platforms,
  reached the cake, used 189 jumps / 8 Echo inputs / 35 bonks / 4 slides and took
  490.24 seconds. It used ordinary physics, not teleport or time scaling.
- That baseline **failed performance**: mean 33.844 fps, p95 40.56 ms,
  p99 56.79 ms. `Artifacts/baseline-high-stream.json` preserves it. Section PNG
  captures also introduced approximately 1.5–2 s stalls; new timing runs disable
  those captures rather than confusing instrumentation with ordinary gameplay.
- Chrome and the in-app browser decoded actual Unreal video and received nonzero
  audio energy. Audio receipt is measured; audible listening has not been verified.
- Browser Escape pause/resume and checkpoint restart were observed in native video.
  Chrome is the current control target; in-app pointer-lock showed a Chromium error.
- Native `quit` from a connected Chrome stream exited with code 0 after the
  Pixel Streaming/Slate ownership fix (`Artifacts/manual-stream.log`). Before that
  fix, one connected shutdown crashed in FMacApplication::OnWindowDestroyed.
- The intermediate 1600×900 Medium/TAA preset was still around 40–50 fps. It is
  **not accepted**. The current 1280×720 preset and frame-limit changes are testing.

Known invalid/incomplete diagnostics remain preserved separately: the early
checker-material run, high-quality performance baseline, and a paused watchdog
failure. None count toward the three final accepted runs. Original imported
meshes, material instancing, resident audio, lower-cost lighting, camera sensitivity
and clean shutdown are implemented; each final outcome must be recorded below.

## Rejected Metal offscreen experiment

`-MetalOffscreenOnly` completed the native physics run and fresh-game restart
(192 platforms, 17 checkpoint triggers, 4 Echo groups, 12 enemy defeats; 491.57 s).
However, it produced **black browser video**. Its 54.62 fps mean / 25.60 ms p95 /
30.37 ms p99 cannot validate playable streaming. The flag has been removed.
Preserved evidence: `Artifacts/metal-offscreen-rejected.json` and `.log`, plus
`Artifacts/offscreen-browser-sample-1.json`. This is not an accepted release run.

## Final regression runs

Pending. Use `Scripts/ue.py journey --run 1`, then 2 and 3. Automated physics
results must remain labeled separately from manual browser-input checks.

## Historical checks before engine installation

The following dated history describes the earlier state; its installation blockers
are superseded by the runtime status above.

# Test results — 7 October 2026

## Ran and passed on the development Mac

| Check | Result |
|---|---|
| TypeScript strict type check | PASS |
| Vite production build | PASS: JS 231.27 kB, gzip 51.75 kB; CSS 9.42 kB |
| Python journey suite | PASS: 8 tests |
| Node server/configuration suite | PASS: 6 tests |
| Python script syntax compilation | PASS |
| Bash deployment launcher syntax | PASS |
| Node service/healthcheck syntax | PASS |
| Deployment YAML parsing and macOS shortcut syntax | PASS; Compose execution still unavailable |
| Actual local HTTP server on 127.0.0.1:8080 | PASS |
| Desktop browser title page and controls dialog | PASS, manually inspected in Codex browser |
| Real Play → unavailable-server message | PASS |
| Retry and Escape dismissal | PASS |
| 390 × 844 responsive layout | PASS, manually inspected; text spacing adjusted |

The Python suite checks deterministic regeneration, run-speed jump reach with
landing margins and platform motion, safe checkpoint spacing, Echo node/bridge
links and timer budget, optional relic access, eight-section continuity, valid PCM
audio, and engine/frontend version agreement.

It caught and prompted fixes for a moving forest landing with insufficient run-speed
margin and a relic detour departing from a collapsing platform. All eight checks
then passed. These are geometry/data checks, not Unreal collision/playthrough tests.

The Node suite checks honest readiness, occupied-session status, exact WebSocket
origin checks, port validation and TURN credential construction. It launches the
actual Epic signalling server on test ports 18080/18888, connects explicitly labelled
protocol-only WebSocket fixtures, verifies streamer identification, subscribes one
client, confirms a second subscription is rejected, disconnects, and confirms
readiness returns to offline. It verifies server source files are not publicly served.
**No media was generated by these fixtures and this is not counted as game streaming.**

## Blocked or not run

| Check | Reason |
|---|---|
| UBT/UHT native compile | UE 5.8 is not installed |
| Editor asset/bootstrap execution | Requires compiled Unreal project |
| Unreal automation tests (2 authored) | Requires Unreal |
| Character/camera/movement/collision playtest | Requires actual engine execution |
| Enemy, Echo, collectibles, checkpoint and traversal playtest | Requires actual engine execution |
| Full normal playthrough and 10–15 minute duration | Not played or timed |
| Section reveal/collapse/ascent/cake visual review | Not rendered in Unreal |
| In-game pause/restart/settings/save verification | Source only; requires engine |
| Actual gamepad and accessibility testing | Requires engine and physical controller |
| CPU/GPU performance and memory profiling | No Unreal trace captured |
| Packaged Mac/Windows/Linux executable | No package produced |
| Local Unreal Pixel Streaming video/audio/data | No Unreal streamer available |
| Browser input controlling the game | Not tested |
| WebRTC tests on Chrome/Edge/Firefox/Safari | Not tested with a real game |
| Authored Playwright regression suite | Not run; current UI checks used the in-app browser |
| Docker Compose build/run | Compose subcommand unavailable; Docker daemon also absent |
| HTTPS certificate, TURN allocation, cloud health | No public host/domain configured |
| Public URL playthrough | No deployment |

The `doctor` script was run and **correctly exited with failure** for a missing
UE 5.8 installation. Free space after cleanup was about 99 GiB. It did not attempt
or claim a native build.

## Runtime acceptance checklist — all still pending

Use Development first, then repeat essential checks on Shipping. Record host,
engine patch version, GPU/driver, date, build commit and outcomes here.

- [ ] Compile the native module and generate real map/material/audio/Blueprint assets.
- [ ] Launch title menu; navigate every button with mouse, keyboard and gamepad.
- [ ] Test walk/run/sprint, variable jump, coyote/buffer, air steering, slide, bonk,
      air bonk, slam, bounce and ledge assist.
- [ ] Validate floor collisions, moving-platform carry, camera obstruction and recovery.
- [ ] Verify all enemy telegraphs, hit reactions, invulnerability and defeat behavior.
- [ ] Collect a shard/relic, die, verify no duplicate; restart and verify a fresh run.
- [ ] Activate every checkpoint; respawn safely from a fall and from damage.
- [ ] Activate all Echo groups, test expiry, reactivation and respawn reset.
- [ ] Traverse the whole route normally without debug teleport.
- [ ] Time a first playthrough and tune route/pacing toward 10–15 minutes.
- [ ] Inspect introductory cake motivation and each palette/space transition.
- [ ] Inspect the city camera reveal without losing control/readability.
- [ ] Complete collapse section; test failure/restart at each safe island.
- [ ] Reach the final ascent, interact with the cake, see eating and completion.
- [ ] Verify replay, pause/resume, graphics/window settings and save persistence.
- [ ] Check original audio, looping transitions and independent volume controls.
- [ ] Profile CPU/GPU and memory; measure 1% low frame times and worst sections.
- [ ] Produce and launch a native packaged build.
- [ ] Connect a real browser, receive actual game frames/audio and control the character.
- [ ] Verify stream volume, checkpoint button, fullscreen, leave and reconnect.
- [ ] Verify Chrome, Edge, Firefox and supported Safari; physical gamepad.
- [ ] Test the actual cloud host, HTTPS, TURN fallback and complete browser playthrough.
- [ ] Publish an actual tested URL and update README status only after success.

## Play startup correction — 7 October 2026

- Browser production build and six Node signalling tests pass again.
- Live browser inspection confirms disabled “Game not running” and explicit “Not playable yet” when `/readyz` has no Unreal stream.
- The combined Desktop launcher exits immediately with the missing-Unreal diagnostic; it does not start a misleading empty player.
- Browser regression assertions were updated for the disabled offline state.
- Combined launch after installing Unreal remains untested. No game stream or gameplay has been verified.

## Full-screen title menu redesign — 7 October 2026

- Production TypeScript/Vite build passes; all six Node HTTP/signalling tests pass.
- Live in-app browser: inspected the 1470×875 desktop layout and a 438×749 narrow layout; document dimensions matched viewport dimensions, with no page scrolling.
- Original Nori PNG loads with genuine alpha transparency; valley PNG and original WAV assets return HTTP 200 with correct content types.
- Verified Options, controls, offline explanation, keyboard activation, volume adjustment, animation toggle, persistence after reload, music toggle and fullscreen transition. No captured browser script errors.
- Restored default menu motion/volume and muted test audio afterward.
- Updated Playwright regression suite to cover offline behavior, keyboard navigation, persistent preferences and three viewport sizes. This authored suite was not executed in this session; live browser checks used the provided computer-use interface.
- Gamepad navigation is implemented but not hardware-tested. Native Unreal gameplay and stream validation remain pending engine installation.


## Gameplay readiness and visual refinement — 8 October 2026

User release constraint: test before deployment; free hosting only. No GitHub push,
public deployment, or claim of smooth/bug-free gameplay has been made.

- PASS: 11 Python tests (8 route/audio checks plus 3 packaged launcher regressions).
- PASS: all 6 real HTTP/signalling tests; these still contain no Unreal media.
- PASS: strict TypeScript and production Vite build (236.24 kB JS, 53.26 kB gzip;
  10.82 kB CSS). Menu removes the tilted oversized logo, floating gems, fireflies,
  light shafts and character caption, retaining a single Start action and two options.
- PASS: live simplified menu inspected in narrow and 1470×875 desktop views.
  Desktop document exactly matches viewport (no scroll); Options, controls and
  fullscreen activation work by keyboard; no captured browser errors.
- PASS: nine original FBX assets exported with Blender 5.2.1 and imported back.
  Bounds, manifold solids, outward normals and two ear material slots verified.
  Re-run with `Blender --background --python Scripts/verify_art.py`.
- Actual mesh preview rendered in Blender at `Artifacts/Nori-mesh-preview.png`.
  This is a studio render, **not an Unreal gameplay screenshot**. Title illustrations
  remain separate from actual game meshes.
- Code fixes awaiting native verification: character/enemy/art cook directories;
  packaged executable launch without engine installation; one air bonk per jump;
  attack contact after wind-up; head/eyes/ears moving together; paw gait, airborne
  pose and scarf refinement; retaining momentum across pause; clearing held input;
  slide braking; keeping cake-facing rotation upright; falling-only ledge assist.
- Added Unreal mesh import/unit/material-slot test, making 3 authored Unreal tests.
  None have run until the engine installation finishes.

Before publishing: compile and import successfully, run all three Unreal tests,
complete at least three unassisted full journeys, inspect each section, measure
frame pacing in both the native game and actual browser stream, and repeat key
checks on the packaged build. Include one run with deliberate deaths, checkpoint
restarts, Echo expiry, pause mid-jump, repeated aerial attack input, and leave/reconnect.
Use `python3 Scripts/ue.py profile` for an Unreal Insights trace; record hardware,
resolution, quality, frame-time percentiles and visible/input stutters. Tune quality
and expensive scenery against those measurements. Keep observed gameplay defects
and persistent stutters as release blockers. Static tests never satisfy this gate.
