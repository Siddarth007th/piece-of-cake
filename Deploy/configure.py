#!/usr/bin/env python3
"""Render relay config; never print secrets. Run on the intended Linux GPU host."""
import ipaddress
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parent
env_file = ROOT / ".env"
if not env_file.exists():
    raise SystemExit("Copy Deploy/.env.example to Deploy/.env and fill in real host settings first.")
values = {}
for line in env_file.read_text().splitlines():
    if not line.strip() or line.lstrip().startswith("#"):
        continue
    key, value = line.split("=", 1)
    values[key.strip()] = value.strip().strip('"').strip("'")
domain = values.get("GAME_DOMAIN", "")
if not re.fullmatch(r"[a-zA-Z0-9.-]+", domain) or domain.endswith("example.com"):
    raise SystemExit("Set a real GAME_DOMAIN.")
if values.get("PUBLIC_ORIGIN") != "https://" + domain:
    raise SystemExit("PUBLIC_ORIGIN must be https:// followed by GAME_DOMAIN.")
address = ipaddress.ip_address(values.get("PUBLIC_IP", ""))
if not address.is_global:
    raise SystemExit("PUBLIC_IP must be the public IP of the GPU/relay host.")
secret = values.get("TURN_SECRET", "")
if not re.fullmatch(r"[a-zA-Z0-9]{32,128}", secret) or secret.startswith("replace"):
    raise SystemExit("Generate TURN_SECRET with openssl rand -hex 32.")
if not values.get("TURN_URL", "").startswith("turn:" + domain + ":"):
    raise SystemExit("TURN_URL must point at the configured host.")
runtime = ROOT / "runtime"
runtime.mkdir(mode=0o700, exist_ok=True)
config = runtime / "turnserver.conf"
config.write_text(f"""listening-port=3478
listening-ip=0.0.0.0
external-ip={address}
realm={domain}
fingerprint
use-auth-secret
static-auth-secret={secret}
min-port=49160
max-port=49200
no-cli
no-tls
no-dtls
no-multicast-peers
no-loopback-peers
denied-peer-ip=0.0.0.0-0.255.255.255
denied-peer-ip=127.0.0.0-127.255.255.255
denied-peer-ip=169.254.0.0-169.254.255.255
denied-peer-ip=224.0.0.0-255.255.255.255
user-quota=4
total-quota=32
log-file=stdout
simple-log
""")
# coturn's container user must be able to read this bind-mounted file; the parent
# stays private to the host owner. The shared secret is never sent to browsers.
config.chmod(0o644)
env_file.chmod(0o600)
print("Relay configuration written. Secrets omitted from output. No services started.")
