# Distribution and optional free hosting

The owner authorized publication after testing on 8 October 2026, then chose the
standalone Mac application as the priority. **No public stream has been deployed.**

## Native Mac release

Package with `python3 Scripts/package_mac.py Development`, run the actual packaged
journeys, validate the dependency closure and signature, then archive the complete
`.app`. Publish the tested archive as a GitHub release; do not commit engine binaries.
The Development configuration keeps the engine regression harness available.

The app is ad-hoc signed and not Apple notarized. Disclose this with the download.
Never remove download quarantine or disable Gatekeeper in a launcher. Apple silicon
macOS is the only tested native target; Windows and Linux require separate builds.

The development Mac keeps its installed app under `~/Applications`, with a Desktop
shortcut. This avoids running the bundle from a cloud-synchronized project directory,
where Finder metadata can invalidate bundle signature checks.

## Why Vercel is not the whole host

Vercel may serve the frontend. This Unreal project uses Pixel Streaming, which runs
the native game on a GPU-equipped machine and streams frames/audio to the browser.
A static deployment alone cannot provide gameplay. Vercel's functions are bounded
request runtimes, not a substitute for that native renderer.

Sources: [Vercel function limits](https://vercel.com/docs/functions/limitations),
[Epic Pixel Streaming](https://dev.epicgames.com/documentation/unreal-engine/pixel-streaming-in-unreal-engine).

## Prepared temporary preview (not publicly validated)

`Host Online Preview.command` starts only this game's tunnel, loopback browser server,
packaged renderer and keep-awake process. Control-C stops them. It refuses occupied
ports. The current temporary address is saved in ignored `Deploy/runtime/preview.json`
and a Desktop-project `Play Online.webloc` shortcut. The address changes each session.

The launcher uses Cloudflare Quick Tunnel for HTTPS/WSS and an exact origin allowlist.
Streamer port 8888 stays on loopback. The tunnel does not relay WebRTC media: direct
connections still depend on NAT/firewalls. The optional `--free-relay` flag uses the
Open Relay Project's published static-auth service; `--relay-only` requires relayed
media during QA. This configuration has unit tests, but public media delivery is
not verified. No account, billing, subscription or paid trial has been enabled.

Before sharing a preview: verify real moving video, audio data, input, pause/restart,
reconnection and performance through the public URL. Same-Mac testing alone does not
prove operation from another network. Keep logs private; never publish credentials.

The Mac must remain powered, awake and online. There is no uptime guarantee. One game
instance supports one player; progress is shared on that host. GitHub's downloadable
app avoids these streaming dependencies.

References: [Cloudflare Quick Tunnels](https://developers.cloudflare.com/tunnel/get-started/quick-tunnels/),
[Open Relay documentation](https://www.metered.ca/tools/openrelay/).

Existing Linux Docker/Caddy/coturn files remain unprovisioned tooling. They have not
been tested on a Linux GPU host and must not trigger spending.
