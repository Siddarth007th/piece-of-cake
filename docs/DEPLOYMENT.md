# Deployment runbook

**Current deployment status: NOT DEPLOYED.** No packaged Unreal build exists yet,
no GPU cloud host or domain has been supplied, and no cloud credentials were used.
Local signalling is tested; local Unreal media streaming is not tested.

## Prerequisites and topology

Use a Linux x86_64 host with a compatible NVIDIA driver and NVENC-capable GPU.
Start sizing at 8 CPU threads, 32 GB RAM, 12+ GB VRAM and 150 GB disk, then profile;
these are planning estimates, not measured performance requirements. This setup
uses one native Unreal process and permits one connected player at a time.

Required host tools: the packaged **Linux** game, NVIDIA driver, Docker Engine with
Compose, systemd, Python 3, curl and openssl. Unreal Editor belongs on the build
machine; it is not needed on the public player's device or the runtime host.

Build the Linux package on a UE 5.8 Linux build machine using:

```sh
python3 Scripts/ue.py package --target Linux --configuration Shipping
```

Do not upload a Mac build to a Linux GPU server. Do not run the engine as root.
Epic's [Pixel Streaming reference](https://dev.epicgames.com/documentation/en-us/unreal-engine/unreal-engine-pixel-streaming-reference)
describes the GPU encoders and networking involved; the
[UE5.8 migration guide](https://github.com/EpicGames/PixelStreamingInfrastructure/blob/UE5.8/Docs/pixel-streaming-2-migration-guide.md)
provides the Pixel Streaming 2 launch setting changes used here.

## 1. Prepare and verify the build

1. Install UE 5.8 and complete `Scripts/ue.py prepare` without errors.
2. Run the Unreal automation tests and manual acceptance checklist.
3. Complete a normal playthrough and record its duration.
4. Capture an Unreal Insights CPU/GPU trace with `Scripts/ue.py profile`.
5. Package Linux and run that packaged build on the target GPU class.
6. Connect the browser locally and verify actual video, sound, keyboard and gamepad.

Do not move a failed build into production or treat the browser poster as validation.

## 2. Stage files on your chosen GPU host

Choose/rent the host using your own provider account and spending decision; this
repository does not provision billable infrastructure. Point a DNS A record for
the game domain at the host's public IPv4 address. Add AAAA only if IPv6 is configured.

On that host, an administrator creates the dedicated runtime account and directory:

```sh
sudo useradd --system --create-home --home-dir /var/lib/pieceofcake --shell /usr/sbin/nologin pieceofcake
sudo install -d -o pieceofcake -g pieceofcake /opt/pieceofcake
```

Upload the repository and the Linux packaged archive to `/opt/pieceofcake`. Preserve
executable permissions on `Deploy/launch-game.sh` and the packaged launcher/binary.
Set ownership of the game directory to `pieceofcake`. Ensure this user can access
the GPU devices under the host's normal NVIDIA driver permissions.

## 3. Configure domain, relay and process

```sh
cd /opt/pieceofcake
cp Deploy/.env.example Deploy/.env
openssl rand -hex 32
```

Edit `.env` locally. Put the generated value in `TURN_SECRET` without copying it
into source control or logs. Set all fields to the real host values:

| Variable | Meaning |
|---|---|
| `GAME_DOMAIN` | Your DNS hostname, without protocol |
| `PUBLIC_ORIGIN` | Exactly `https://` + the hostname |
| `ACME_EMAIL` | Email for the TLS certificate account |
| `PUBLIC_IP` | Routable public IPv4 of this host |
| `TURN_URL` | UDP and TCP `turn:` URLs on this host, comma-separated |
| `TURN_SECRET` | Shared coturn HMAC secret; at least 32 alphanumeric characters |
| `GAME_EXECUTABLE` | Absolute path to the packaged Linux launcher |

```sh
python3 Deploy/configure.py
docker compose --env-file Deploy/.env -f Deploy/compose.yaml config --quiet
```

The generator rejects example domains, nonpublic IPs and weak/example secrets. It
writes `Deploy/runtime/turnserver.conf` without printing its content. `.env` and
`runtime/` are ignored by Git. Browser peers receive 24-hour temporary credentials;
the trusted local streamer receives a longer-lived credential so it can keep
serving new sessions. Restart the services when rotating the shared secret.

This baseline uses authenticated TURN on UDP/TCP 3478; WebRTC media stays encrypted.
TURN-over-TLS on 443 is **not configured**. Networks blocking both UDP and outbound
3478 may need a separate TLS TURN endpoint and a corresponding `turns:` URL.

## 4. Network rules

Open inbound:

- TCP 80 and 443: Caddy certificate issuance and HTTPS/WSS.
- UDP and TCP 3478: authenticated TURN.
- UDP 49160–49200: relay/game media port range.
- SSH only as required for your administration policy.

Do not expose ports 8080 or 8888 publicly. Both bind to loopback. There is no SFU or
matchmaker in this single-player deployment. Keep the same-host media port range
under observation under load; coturn and Unreal must be able to allocate free ports.

## 5. Start the web services and game

```sh
docker compose --env-file Deploy/.env -f Deploy/compose.yaml up -d --build
sudo install -m 644 Deploy/piece-of-cake.service /etc/systemd/system/piece-of-cake.service
sudo systemctl daemon-reload
sudo systemctl enable --now piece-of-cake
```

Compose uses Linux host networking so the native GPU process can reach the
signalling container at `127.0.0.1:8888`. Caddy handles HTTPS and WebSocket upgrade
on the same origin. The game runs natively under systemd; the web container does
not claim to contain an Unreal build or GPU runtime.

The shared `.env` must be readable by the service user (`pieceofcake`); keep mode
600 and the correct ownership. Caddy certificate volumes persist across restarts.

## 6. Verify externally

```sh
node Deploy/healthcheck.mjs https://YOUR-ACTUAL-GAME-DOMAIN
docker compose --env-file Deploy/.env -f Deploy/compose.yaml logs --tail=100
sudo journalctl -u piece-of-cake -n 100 --no-pager
```

On a separate network/device, open the real URL and click Play. Verify all of:

1. A live rendered game frame, not merely a connected signalling socket.
2. WASD changes the character position and mouse changes the camera.
3. Audio is audible and both browser/in-game volume controls work.
4. Gamepad movement, actions and menus work after focusing the page.
5. An actual full journey through all eight sections, ending with eating the cake.
6. Pause, checkpoint recovery and restart.
7. TURN fallback on a network where direct media cannot connect.
8. A second client gets an occupied-state response, not shared accidental control.

Only after these checks should `README.md` gain a real **deployed URL** and a dated
cloud validation entry. `/healthz` alone is never a deployment acceptance test.

## Restart, logs and recovery

```sh
sudo systemctl restart piece-of-cake
docker compose --env-file Deploy/.env -f Deploy/compose.yaml restart signalling
sudo journalctl -u piece-of-cake -f
```

Restarting Unreal resets the active play session; notify any current player first.
Saves are under the runtime user's Unreal Saved directory; back them up before
replacing a deployment. Keep the previous packaged release for rollback. Rotate
TURN secrets by updating `.env`, regenerating the relay configuration and restarting
TURN, signalling and Unreal together.

The Node service enforces one seat, same-origin input WebSockets, bounded message
sizes, no camera/microphone permission, and no exposed server files. It does not
provide accounts, per-user saves, a queue, elastic GPU allocation or DDoS protection.
Add those only after this single-session experience is measured and accepted.
