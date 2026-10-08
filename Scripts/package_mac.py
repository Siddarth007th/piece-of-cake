#!/usr/bin/env python3
"""Build outside cloud-synced Desktop folders; deliver the artifacts beside source."""
import os
from pathlib import Path
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
STAGE = Path("/private/tmp/PieceOfCake-native-build")

def copy_file(source, destination):
    shutil.copyfile(source, destination)
    shutil.copymode(source, destination)
    return str(destination)

def copy_bundle(source, target):
    # Replacing a generated bundle must also update its loader aliases.
    for link in source.rglob('*'):
        if link.is_symlink():
            destination=target/link.relative_to(source)
            if destination.is_symlink() or destination.is_file(): destination.unlink()
    shutil.copytree(source,target,dirs_exist_ok=True,copy_function=copy_file,symlinks=True)

def main():
    configuration = sys.argv[1] if len(sys.argv) > 1 else "Shipping"
    if configuration not in ("Development", "Shipping"):
        raise SystemExit("Unknown build configuration")
    marker = STAGE / ".piece-of-cake-source"
    if STAGE.exists() and (not marker.is_file() or marker.read_text() != str(ROOT)):
        raise SystemExit(f"Build cache {STAGE} belongs to another project; choose a fresh cache directory.")
    STAGE.mkdir(parents=True, exist_ok=True)
    marker.write_text(str(ROOT))
    for name in ("Config", "Content", "Source", "Scripts", "Build"):
        if (ROOT / name).is_dir():
            shutil.copytree(ROOT / name, STAGE / name, dirs_exist_ok=True,
                            copy_function=copy_file, symlinks=True,
                            ignore=shutil.ignore_patterns("__pycache__", "*.blend1", ".DS_Store"))
    copy_file(ROOT / "PieceOfCake.uproject", STAGE / "PieceOfCake.uproject")
    environment = dict(os.environ, POC_NATIVE_STAGE="1")
    print(f"Native build cache: {STAGE}; source and delivered artifacts remain at {ROOT}", flush=True)
    subprocess.run([sys.executable, STAGE / "Scripts/ue.py", "package", "--configuration", configuration],
                   cwd=STAGE, env=environment, check=True)
    # UE 5.8 modern Xcode archive can omit Contents/UE. The staging bundle is
    # complete; validate/sign it and deliver that exact app instead of the empty archive.
    app = STAGE / "Saved/StagedBuilds/Mac/PieceOfCake.app"
    if not (app / "Contents/UE/PieceOfCake/Content/Paks").is_dir():
        raise SystemExit("Cooked content is missing from the staged Mac application")
    from finalize_mac import finalize
    finalize(app)
    source = STAGE / "Builds/Mac" / configuration
    source.mkdir(parents=True, exist_ok=True)
    copy_bundle(app,source/app.name)
    target = ROOT / "Builds/Mac" / configuration
    target.mkdir(parents=True,exist_ok=True)
    copy_bundle(source/app.name,target/app.name)
    print(f"Packaged output copied to {target}. Runtime validation is still required.", flush=True)

if __name__ == "__main__":
    try: main()
    except subprocess.CalledProcessError as error: raise SystemExit(error.returncode)
