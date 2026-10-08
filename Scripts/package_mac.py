#!/usr/bin/env python3
"""Build outside cloud-synced Desktop folders; deliver the artifacts beside source."""
import os
import filecmp
from pathlib import Path
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
STAGE = Path("/private/tmp/PieceOfCake-native-build")

def copy_file(source, destination):
    # Preserve timestamps for unchanged source so UBT can reuse compiled objects.
    # Do not copy Finder resource forks/xattrs into signed bundles.
    if Path(destination).is_file() and not Path(destination).is_symlink() and filecmp.cmp(source,destination,shallow=False):
        return str(destination)
    shutil.copyfile(source, destination)
    shutil.copymode(source, destination)
    source_stat=Path(source).stat()
    os.utime(destination,ns=(source_stat.st_atime_ns,source_stat.st_mtime_ns))
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
    # Cook public connection settings through Unreal's normal Game config layer.
    # Loose custom INIs are intentionally unavailable in cooked runtimes.
    cloud = ROOT / "Config/Cloud.ini"
    if cloud.is_file():
        import configparser
        parsed = configparser.ConfigParser(); parsed.read(cloud)
        url = parsed.get("PieceOfCake.Cloud", "URL").strip('"')
        key = parsed.get("PieceOfCake.Cloud", "PublishableKey")
        if not url.startswith("https://") or not url.endswith(".supabase.co") or not key.startswith("sb_publishable_"):
            raise SystemExit("Cloud packaging requires a Supabase HTTPS URL and public publishable key")
        with (STAGE / "Config/DefaultGame.ini").open("a") as game_config:
            game_config.write('\n[PieceOfCake.Cloud]\nURL="'+url+'"\nPublishableKey='+key+'\n')
    copy_file(ROOT / "PieceOfCake.uproject", STAGE / "PieceOfCake.uproject")
    environment = dict(os.environ, POC_NATIVE_STAGE="1")
    print(f"Native build cache: {STAGE}; source and delivered artifacts remain at {ROOT}", flush=True)
    subprocess.run([sys.executable, STAGE / "Scripts/ue.py", "package", "--configuration", configuration],
                   cwd=STAGE, env=environment, check=True)
    # Keep editor-authored source assets beside the complete source project too.
    # Cook outputs remain ignored; these .uasset/.umap files are editable inputs.
    for asset in (STAGE / "Content").rglob("*"):
        if asset.is_file() and asset.suffix in (".uasset", ".umap"):
            destination=ROOT / asset.relative_to(STAGE)
            destination.parent.mkdir(parents=True,exist_ok=True)
            copy_file(asset,destination)
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
