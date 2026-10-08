#!/usr/bin/env python3
"""Host the tested Mac package through a free temporary HTTPS tunnel; Ctrl-C stops it."""
import argparse
import json
import os
from pathlib import Path
import re
import plistlib
import shutil
import signal
import socket
import subprocess
import time
import urllib.request
import ue

ROOT=Path(__file__).resolve().parents[1]
RUNTIME=ROOT/'Deploy/runtime'
APP=ROOT/'Builds/Mac/Development/PieceOfCake.app/Contents/MacOS/PieceOfCake'


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--free-relay', action='store_true', help='Use the public Open Relay static-auth TURN service (best effort)')
    parser.add_argument('--relay-only', action='store_true', help='QA: require relayed media; no direct connection fallback')
    args=parser.parse_args()
    if args.relay_only and not args.free_relay: parser.error('--relay-only requires --free-relay')
    if not APP.is_file() or not (ROOT/'Web/dist/index.html').is_file():
        raise SystemExit('Build and test the Mac package and web player first.')
    for name in ('node','cloudflared','caffeinate'):
        if not shutil.which(name): raise SystemExit(f'Missing {name}')
    for port in (8080,8888):
        with socket.socket() as check:
            try: check.bind(('127.0.0.1',port))
            except OSError: raise SystemExit(f'Port {port} is busy. Stop the existing Piece of Cake session first; nothing was killed.')
    RUNTIME.mkdir(parents=True,exist_ok=True)
    processes=[]; logs=[]
    def launch(name, command, environment=None):
        log=(RUNTIME/f'{name}.log').open('w');logs.append(log)
        proc=subprocess.Popen([str(x) for x in command],cwd=ROOT,env=environment,stdout=log,stderr=subprocess.STDOUT,start_new_session=True)
        processes.append(proc);return proc
    def stop(_signum,_frame): raise KeyboardInterrupt
    signal.signal(signal.SIGTERM,stop)
    try:
        tunnel=launch('tunnel',['cloudflared','tunnel','--url','http://127.0.0.1:8080','--no-autoupdate'])
        deadline=time.monotonic()+90;url=None
        while time.monotonic()<deadline:
            if tunnel.poll() is not None: raise SystemExit('Tunnel stopped. Read Deploy/runtime/tunnel.log.')
            found=re.search(r'https://[a-z0-9-]+\.trycloudflare\.com',(RUNTIME/'tunnel.log').read_text())
            if found: url=found.group();break
            time.sleep(1)
        if not url: raise SystemExit('No temporary URL returned. Read Deploy/runtime/tunnel.log.')
        env=dict(os.environ,NODE_ENV='production',BIND_HOST='127.0.0.1',PUBLIC_ORIGIN=f'{url},http://127.0.0.1:8080,http://localhost:8080',STUN_URL='stun:stun.cloudflare.com:3478')
        if args.free_relay: env['FREE_RELAY']='1'
        if args.relay_only: env['ICE_RELAY_ONLY']='1'
        server=launch('server',['node',ROOT/'Web/server.mjs'],env)
        deadline=time.monotonic()+20
        while True:
            if server.poll() is not None: raise SystemExit('Browser server stopped. Read Deploy/runtime/server.log.')
            try:
                with urllib.request.urlopen('http://127.0.0.1:8080/healthz',timeout=2): break
            except OSError:
                if time.monotonic()>deadline: raise SystemExit('Browser server did not become healthy.')
                time.sleep(1)
        game=launch('game',[APP,'-POCPublicSession','-POCQuality=1',*ue.stream_arguments()])
        launch('awake',['caffeinate','-di','-w',str(game.pid)])
        state={'url':url,'host_pid':os.getpid(),'game_pid':game.pid,'server_pid':server.pid,'tunnel_pid':tunnel.pid,'relay_only':args.relay_only,'free_relay':args.free_relay,'started':time.strftime('%Y-%m-%dT%H:%M:%SZ',time.gmtime())}
        (RUNTIME/'preview.json').write_text(json.dumps(state,indent=2)+'\n')
        (ROOT/'Play Online.webloc').write_bytes(plistlib.dumps({'URL':url}))
        print(f'Preview address: {url}\nKeep this Mac awake and online. One player at a time. Control-C stops hosting.',flush=True)
        while all(proc.poll() is None for proc in processes): time.sleep(1)
        raise SystemExit('A hosting process stopped. Read Deploy/runtime logs.')
    finally:
        for proc in reversed(processes):
            try: os.killpg(proc.pid,signal.SIGTERM)
            except ProcessLookupError: pass
        for proc in processes:
            try: proc.wait(timeout=5)
            except subprocess.TimeoutExpired:
                try: os.killpg(proc.pid,signal.SIGKILL)
                except ProcessLookupError: pass
        for log in logs: log.close()
        if (RUNTIME/'preview.json').exists():
            state=json.loads((RUNTIME/'preview.json').read_text());state['stopped']=True
            (RUNTIME/'preview.json').write_text(json.dumps(state,indent=2)+'\n')

if __name__=='__main__':
    try: main()
    except KeyboardInterrupt: print('Preview hosting stopped.')
