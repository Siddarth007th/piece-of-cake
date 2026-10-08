#!/usr/bin/env python3
"""Run real Unreal journeys and fail on missing gameplay evidence. Never publishes."""
import argparse
import json
from pathlib import Path
import time
import ue


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--executable',type=Path,help='Test a packaged Development game instead of the editor')
    parser.add_argument('--label',default='local-regression',help='Evidence subdirectory under Artifacts')
    parser.add_argument('--runs',type=int,nargs='+',choices=[1,2,3],default=[1,2,3])
    parser.add_argument('--native',action='store_true',help='Test a windowed native app without video encoding or a browser')
    parser.add_argument('--public-session',action='store_true',help='Also disable the developer console, as in the public preview')
    args=parser.parse_args()
    if not args.label or any(c not in 'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_' for c in args.label):
        raise SystemExit('Use a simple evidence label')
    evidence=ue.ROOT/'Artifacts'/args.label;evidence.mkdir(exist_ok=True)
    if args.executable:
        if not args.executable.is_file(): raise SystemExit('Packaged game executable is missing')
        command=[args.executable.resolve()]
    else:
        root=ue.engine_root();ue.build_editor(root)
        command=[ue.tools(root)[1],ue.PROJECT,'/Game/Levels/L_LongWayToCake','-game']
    runtime_flags=['-windowed','-ForceRes','-ResX=1280','-ResY=720','-AudioMixer','-Unattended','-stdout','-FullStdOutLogOutput','-ExecCmds=stat hitches'] if args.native else ue.stream_arguments()
    summary=[]
    for run in args.runs:
        report=evidence/f'journey-run-{run}.json';started=time.time()
        with (evidence/f'journey-run-{run}.log').open('w') as log:
            ue.run([*command,f'-POCAutoRun={run}','-POCQuality=1',f'-POCArtifacts={evidence}',
                    *(['-POCPublicSession'] if args.public_session else []),*runtime_flags],stdout=log,stderr=log)
        if not report.exists() or report.stat().st_mtime<started: raise SystemExit(f'Run {run}: no fresh runtime report')
        data=json.loads(report.read_text())
        required=['completed','restart_verified','cake_door_open']
        if run==2: required += ['deliberate_fall_recovered','empty_hearts_before_checkpoint_observed','live_bomb_health_loss_verified','live_bomb_knockback_verified']
        if run==3: required += ['pause_resume_exercised','echo_expiry_reactivation_verified','insufficient_shards_rejected','backtracked_and_earned_missing_shards']
        failed=[key for key in required if not data.get(key)]
        if data.get('developer_flight_used') is not False: failed.append('developer flight excluded')
        if data.get('platforms_landed')!=192: failed.append('all 192 platforms landed')
        if data.get('shards_spent')!=360: failed.append('360 shards spent')
        item={'run':run,'native_without_streaming':args.native,'failures':failed,'respawns':data.get('respawns'),'mean_fps':data.get('mean_fps'),
              'p95_ms':data.get('p95_frame_ms'),'frames_over_100ms':data.get('frames_over_100ms')}
        summary.append(item);print(json.dumps(item),flush=True)
        (evidence/'summary.json').write_text(json.dumps(summary,indent=2)+'\n')
        if failed: raise SystemExit(f"Run {run} failed gameplay acceptance: {', '.join(failed)}")
    print('Requested engine journeys passed gameplay checks. Review frame pacing separately. Nothing was published.',flush=True)

if __name__=='__main__': main()
