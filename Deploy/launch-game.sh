#!/usr/bin/env bash
set -euo pipefail
: "${GAME_EXECUTABLE:?Set GAME_EXECUTABLE to the packaged native Linux launcher}"
[[ -x "$GAME_EXECUTABLE" ]] || { echo 'Packaged game executable is missing or not executable.' >&2; exit 1; }
for attempt in {1..30}; do
  if curl -fsS http://127.0.0.1:8080/healthz >/dev/null; then break; fi
  sleep 2
done
curl -fsS http://127.0.0.1:8080/healthz >/dev/null
exec "$GAME_EXECUTABLE" \
  -PixelStreamingSignallingURL=ws://127.0.0.1:8888 \
  -PixelStreamingID=piece-of-cake \
  -PixelStreamingEncoderCodec=H264 \
  -PixelStreamingWebRTCMinPort=49160 -PixelStreamingWebRTCMaxPort=49200 \
  -RenderOffscreen -ForceRes -ResX=1600 -ResY=900 \
  -AudioMixer -Unattended -stdout -FullStdOutLogOutput
