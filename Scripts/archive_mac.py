#!/usr/bin/env python3
"""Archive a tested Mac application, preserve symlinks, and verify the extracted bundle."""
import argparse
import hashlib
from pathlib import Path
import shutil
import subprocess
import tempfile
from package_mac import ROOT, copy_file


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('app',type=Path)
    args=parser.parse_args()
    app=args.app.expanduser().resolve()
    subprocess.run(['codesign','--verify','--deep','--strict',str(app)],check=True)
    output=ROOT/'Builds/Releases';output.mkdir(parents=True,exist_ok=True)
    name='PieceOfCake-0.3.0-mac-arm64'
    archive=output/(name+'.zip')
    with tempfile.TemporaryDirectory(prefix='piece-of-cake-release-') as temp:
        release=Path(temp)/name;release.mkdir()
        shutil.copytree(app,release/'PieceOfCake.app',copy_function=copy_file,symlinks=True)
        (release/'READ ME.txt').write_text('''PIECE OF CAKE 0.3 — MAC PREVIEW

Play: move PieceOfCake.app to Applications, then double-click it.
No Unreal Editor, browser, Node.js, account or internet connection is needed.
Apple silicon Mac only (M1 or newer). Tested on M4 / macOS 26.7.1.
The bundle targets macOS 14+, but other versions have not been tested.

This is an ad-hoc signed, non-notarized preview. A Mac downloading it may
require its owner's explicit approval. Do not disable macOS security.

CONTROLS
WASD: move      Space: jump / press again for double jump
Shift: sprint  Q/right mouse: dash   J/left mouse: twist attack
C/Ctrl: slide or slam   E: Echo / cake switch
Arrows: camera   F: toggle mouse look   X: center camera
Esc: pause / settings   R: checkpoint restart

Choose Start adventure; Enter/Space skips the short cake dream.
Type siddarthisgod while playing to enable developer flight. WASD moves,
Space rises, C descends, Shift flies faster. Type it again to land where
you are above safe ground. R is the explicit checkpoint rescue. E at the
cake previews the ending; developer-assisted runs are unranked.

CLOUD SAVES
Optional Supabase Free cloud saves are available in the Cloud saves menu.
Connect creates a private guest profile for this installation; the session
is stored in macOS Keychain. No email is needed. There is no cross-device
account recovery yet. Offline play still works; saves retry on reconnection.

Medium graphics is the tested 60 FPS target. Three hearts; falling sends
you to your checkpoint. Each cake switch costs 180 shards. You may return
for more shards. Gamepad is mapped but not hardware tested.

SHARING
Share this complete ZIP, not only the executable inside the app.
The app is a local single-player game. It does not depend on the author's Mac.
Source and measured test results: https://github.com/Siddarth007th/piece-of-cake
No claim of flawless performance on every Mac is made.
''')
        subprocess.run(['ditto','-c','-k','--norsrc','--keepParent','--zlibCompressionLevel','9',str(release),str(archive)],check=True)
        extracted=Path(temp)/'verify';extracted.mkdir()
        subprocess.run(['ditto','-x','-k',str(archive),str(extracted)],check=True)
        unpacked=extracted/name/'PieceOfCake.app'
        subprocess.run(['codesign','--verify','--deep','--strict',str(unpacked)],check=True)
        for relative in ['Contents/MacOS/PieceOfCake','Contents/UE/PieceOfCake/Content/Paks/PieceOfCake-Mac.ucas']:
            if hashlib.sha256((app/relative).read_bytes()).digest()!=hashlib.sha256((unpacked/relative).read_bytes()).digest():
                raise SystemExit('Archive changed the tested executable or cooked content')
    digest=hashlib.sha256(archive.read_bytes()).hexdigest()
    (output/'SHA256SUMS.txt').write_text(f'{digest}  {archive.name}\n')
    print(f'{archive}\nDownload size: {archive.stat().st_size/2**20:.1f} MiB\nSHA-256: {digest}\nExtracted signature, executable and cooked-content checks passed.')

if __name__=='__main__': main()
