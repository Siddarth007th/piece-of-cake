#!/usr/bin/env python3
"""Build the local game and start both halves of Pixel Streaming together."""
import json
import os
from pathlib import Path
import signal
import shutil
import socket
import subprocess
import sys
import time
import urllib.request
import webbrowser
import ue

ROOT = Path(__file__).resolve().parents[1]

def main():
    # Fail before starting a tempting but unplayable browser page.
    ue.engine_root()
    for program in ("node", "npm"):
        if not shutil.which(program):
            raise SystemExit(f"Missing {program}. Install Node.js before continuing.")
    for port in (8080, 8888):
        with socket.socket() as probe:
            probe.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            try: probe.bind(("127.0.0.1", port))
            except OSError:
                raise SystemExit(f"Port {port} is already in use. Close the existing browser-player/server terminal, then retry. No existing process was stopped.")
    subprocess.run([sys.executable, ROOT / "Scripts/ue.py", "prepare"], cwd=ROOT, check=True)
    if not (ROOT / "Web/node_modules").is_dir():
        subprocess.run(["npm", "ci"], cwd=ROOT / "Web", check=True)
    subprocess.run(["npm", "run", "build"], cwd=ROOT / "Web", check=True)
    logs = ROOT / "Artifacts"
    logs.mkdir(exist_ok=True)
    processes = []
    try:
        with (logs / "browser-server.log").open("w") as server_log, (logs / "unreal-stream.log").open("w") as game_log:
            processes.append(subprocess.Popen(["node", "server.mjs"], cwd=ROOT / "Web", stdout=server_log, stderr=subprocess.STDOUT, start_new_session=True))
            processes.append(subprocess.Popen([sys.executable, ROOT / "Scripts/ue.py", "stream"], cwd=ROOT, stdout=game_log, stderr=subprocess.STDOUT, start_new_session=True))
            print("Starting Unreal. Logs are in Desktop/PieceOfCake/Artifacts. Keep this window open.", flush=True)
            # Shader compilation on a first launch can take several minutes.
            deadline = time.monotonic() + 900
            ready = False
            while time.monotonic() < deadline:
                if any(proc.poll() is not None for proc in processes):
                    raise SystemExit("A game/server process stopped. Read Artifacts/unreal-stream.log and browser-server.log.")
                try:
                    with urllib.request.urlopen("http://127.0.0.1:8080/readyz", timeout=2) as response:
                        ready = json.load(response).get("ready") is True
                except (OSError, ValueError): pass
                if ready: break
                time.sleep(2)
            if not ready: raise SystemExit("Unreal did not connect in 15 minutes. Read Artifacts/unreal-stream.log. No playable stream was confirmed.")
            print("Unreal is connected. Opening http://127.0.0.1:8080 — click Play. Press Control-C here to stop.", flush=True)
            webbrowser.open("http://127.0.0.1:8080/")
            while all(proc.poll() is None for proc in processes): time.sleep(1)
            raise SystemExit("A game/server process stopped. See the logs in Artifacts.")
    finally:
        for proc in processes:
            try: os.killpg(proc.pid, signal.SIGTERM)
            except ProcessLookupError: pass
        for proc in processes:
            try: proc.wait(timeout=5)
            except subprocess.TimeoutExpired:
                try: os.killpg(proc.pid, signal.SIGKILL)
                except ProcessLookupError: pass
                proc.wait()

if __name__ == "__main__":
    try: main()
    except KeyboardInterrupt: print("Game and browser server stopped.")
    except subprocess.CalledProcessError as error: raise SystemExit(error.returncode)
