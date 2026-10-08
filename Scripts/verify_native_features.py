#!/usr/bin/env python3
"""Exercise real packaged features; cloud cases use isolated, unranked QA profiles."""
import argparse
import json
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--executable',type=Path,required=True)
    p.add_argument('--label',default='native-features')
    p.add_argument('--cloud',action='store_true',help='Also contact the configured live cloud using the separate QA Keychain account')
    args=p.parse_args()
    if not args.label or any(c not in 'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_' for c in args.label):
        raise SystemExit('Use a simple evidence label')
    evidence=ROOT/'Artifacts'/args.label;evidence.mkdir(exist_ok=True)
    cases=[('flight','-POCFlightTest','developer-flight-test.json'),('presentation','-POCPresentationTest','presentation.json')]
    if args.cloud: cases += [('cloud-write','-POCCloudSmoke','cloud-native-write.json'),('cloud-reload','-POCCloudSmokeVerify','cloud-native-reload.json')]
    for name,flag,report in cases:
        target=evidence/report
        if target.exists(): target.rename(evidence/(report+'.previous'))
        with tempfile.TemporaryDirectory(prefix='poc-'+name+'-') as profile, (evidence/(name+'.log')).open('w') as log:
            subprocess.run([str(args.executable.resolve()),flag,'-POCPublicSession','-windowed','-ResX=1280','-ResY=720','-ForceRes',
                            '-Unattended','-stdout','-FullStdOutLogOutput','-UserDir='+profile,'-POCArtifacts='+str(evidence)],
                           stdout=log,stderr=log,timeout=95,check=True)
        result=json.loads(target.read_text())
        passed=result.get('passed') if name!='presentation' else all(v for v in result.values() if isinstance(v,bool))
        print(json.dumps({'case':name,**result}),flush=True)
        if not passed: raise SystemExit(name+' failed')
if __name__=='__main__':main()
