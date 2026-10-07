#!/usr/bin/env python3
"""Cross-platform, fail-fast UE build entry point. Never reports a missing build as success."""
import argparse
import json
import os
from pathlib import Path
import platform
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
PROJECT = ROOT / "PieceOfCake.uproject"
PLATFORM = {"Darwin": "Mac", "Windows": "Win64", "Linux": "Linux"}[platform.system()]


def engine_root():
    configured = os.environ.get("UE_ROOT")
    candidates = [Path(configured)] if configured else []
    candidates += [Path("/Users/Shared/Epic Games/UE_5.8"), Path("/Users/Shared/Epic Games/UE_5.8_SI"),
                   Path("C:/Program Files/Epic Games/UE_5.8"), Path.home() / "UnrealEngine"]
    for candidate in candidates:
        if (candidate / "Engine/Build/Build.version").exists():
            version = json.loads((candidate / "Engine/Build/Build.version").read_text())
            if (version["MajorVersion"], version["MinorVersion"]) != (5, 8):
                raise SystemExit(f"This project pins UE 5.8; found {version['MajorVersion']}.{version['MinorVersion']} at {candidate}. Match engine and streaming libraries before changing versions.")
            return candidate
    raise SystemExit("Unreal Engine 5.8 was not found. Install it with Epic Games Launcher, then set UE_ROOT to the folder containing Engine/. No Unreal build was attempted.")


def run(command, **kwargs):
    print("Running:", " ".join(str(x) for x in command), flush=True)
    subprocess.run([str(x) for x in command], check=True, cwd=ROOT, **kwargs)


def tools(root):
    engine = root / "Engine"
    if PLATFORM == "Mac":
        return (engine / "Build/BatchFiles/Mac/Build.sh", engine / "Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor", engine / "Build/BatchFiles/RunUAT.sh")
    if PLATFORM == "Win64":
        return (engine / "Build/BatchFiles/Build.bat", engine / "Binaries/Win64/UnrealEditor.exe", engine / "Build/BatchFiles/RunUAT.bat")
    return (engine / "Build/BatchFiles/Linux/Build.sh", engine / "Binaries/Linux/UnrealEditor", engine / "Build/BatchFiles/RunUAT.sh")


def build_editor(root):
    build, _, _ = tools(root)
    run([build, "PieceOfCakeEditor", PLATFORM, "Development", PROJECT, "-WaitMutex", "-NoHotReload"])


def prepare(root):
    build, editor, _ = tools(root)
    run([sys.executable, ROOT / "Scripts/generate_journey.py"])
    run([sys.executable, ROOT / "Scripts/generate_audio.py"])
    build_editor(root)
    run([editor, PROJECT, "-unattended", "-nosplash", "-run=pythonscript", f"-script={ROOT / 'Scripts/bootstrap_unreal.py'}", "-stdout", "-FullStdOutLogOutput"])
    if not (ROOT / "Content/Levels/L_LongWayToCake.umap").exists():
        raise SystemExit("UE asset generation did not produce the map. Inspect Saved/Logs before continuing.")


def stream_arguments():
    return ["-PixelStreamingSignallingURL=ws://127.0.0.1:8888", "-PixelStreamingID=piece-of-cake", "-PixelStreamingEncoderCodec=H264",
                  "-RenderOffscreen", "-ForceRes", "-ResX=1600", "-ResY=900", "-AudioMixer", "-Unattended", "-log", "-stdout", "-FullStdOutLogOutput"]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=["doctor", "prepare", "editor", "play", "package", "stream", "profile", "test"])
    parser.add_argument("--target", choices=["Mac", "Win64", "Linux"], default=PLATFORM)
    parser.add_argument("--configuration", choices=["Development", "Shipping"], default="Shipping")
    args = parser.parse_args()
    if args.action == "doctor":
        print(json.dumps({"platform":PLATFORM,"project":str(PROJECT),"disk_free_gib":round(shutil.disk_usage(ROOT).free / 2**30, 1),"python":platform.python_version(),"node":shutil.which("node")}, indent=2))
    configured = os.environ.get("GAME_EXECUTABLE")
    if args.action == "stream" and configured:
        executable = Path(configured).expanduser().resolve()
        if not executable.is_file(): raise SystemExit("GAME_EXECUTABLE does not exist")
        run([executable, *stream_arguments()])
        return
    root = engine_root()
    build, editor, uat = tools(root)
    if args.action == "doctor":
        print(f"UE 5.8 found at {root}")
    elif args.action == "prepare":
        prepare(root)
    elif args.action in ("editor", "play", "test", "profile"):
        if not (ROOT / "Content/Levels/L_LongWayToCake.umap").exists(): prepare(root)
        else: build_editor(root)
        command = [editor, PROJECT, "/Game/Levels/L_LongWayToCake", "-log"]
        if args.action in ("play", "profile"): command += ["-game", "-windowed", "-ResX=1600", "-ResY=900"]
        if args.action == "profile": command += ["-trace=cpu,gpu,frame,bookmark", f"-tracefile={ROOT / 'Artifacts/Gameplay.utrace'}", "-statnamedevents"]
        if args.action == "test": command += ["-unattended", "-ExecCmds=Automation RunTests PieceOfCake", "-TestExit=Automation Test Queue Empty", f"-ReportExportPath={ROOT / 'Artifacts/UnrealTests'}"]
        run(command)
    elif args.action == "package":
        if args.target != PLATFORM:
            raise SystemExit(f"Build {args.target} on a {args.target} UE host. This wrapper does not pretend {PLATFORM} can cross-package that platform.")
        prepare(root)
        output = ROOT / "Builds" / args.target
        run([uat, "BuildCookRun", f"-project={PROJECT}", "-noP4", f"-platform={args.target}", f"-clientconfig={args.configuration}",
             "-build", "-cook", "-stage", "-pak", "-archive", f"-archivedirectory={output}", "-utf8output"])
        print(f"Packaging command succeeded. Launch and validate the build in {output} before distribution.")
    elif args.action == "stream":
        if not (ROOT / "Content/Levels/L_LongWayToCake.umap").exists(): prepare(root)
        else: build_editor(root)
        run([editor, PROJECT, "/Game/Levels/L_LongWayToCake", "-game", *stream_arguments()])


if __name__ == "__main__":
    try:
        main()
    except subprocess.CalledProcessError as error:
        raise SystemExit(error.returncode)
