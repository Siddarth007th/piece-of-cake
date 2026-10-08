#!/usr/bin/env python3
"""Validate a self-contained staged Mac app, add vendor version aliases, then ad-hoc sign."""
from pathlib import Path
import subprocess
import re
import plistlib
import shutil
import hashlib

def output(*args):
    return subprocess.check_output([str(x) for x in args], text=True)

def finalize(app):
    app=Path(app).resolve()
    info_file=app/'Contents/Info.plist'
    info=plistlib.loads(info_file.read_bytes())
    info.update(CFBundleIdentifier='com.siddarth007th.pieceofcake', CFBundleDisplayName='Piece of Cake', CFBundleName='Piece of Cake', CFBundleShortVersionString='0.3.0', CFBundleVersion='3')
    info.pop('CFBundleIconName',None)
    info['CFBundleIconFile']='AppIcon.icns'
    icon=Path(__file__).resolve().parents[1]/'Build/Mac/Resources/Application.icns'
    if icon.is_file(): shutil.copyfile(icon,app/'Contents/Resources/AppIcon.icns')
    info_file.write_bytes(plistlib.dumps(info))
    # A new Mac should start at the measured 720p window size, independent of the
    # developer's existing Saved/GameUserSettings.ini and desktop resolution.
    defaults=Path(__file__).resolve().parents[1]/'Config/DefaultGameUserSettings.ini'
    if defaults.is_file():
        config=app/'Contents/UE/PieceOfCake/Config'
        config.mkdir(parents=True,exist_ok=True)
        shutil.copyfile(defaults,config/defaults.name)
    executable=app/'Contents/MacOS/PieceOfCake'
    if not executable.is_file(): raise RuntimeError('Missing Mac game executable')
    libraries=list(app.rglob('*.dylib'))
    # The executable is Apple silicon only. Remove unused Intel slices and local
    # debug symbols from the delivered runtime, retaining exported/dynamic symbols.
    for binary in [executable,*[p for p in libraries if not p.is_symlink()]]:
        arches=output('lipo','-archs',binary).split()
        if 'arm64' not in arches: raise RuntimeError(f'Not an Apple silicon runtime: {binary}')
        if len(arches)>1:
            slim=binary.with_name(binary.name+'.arm64-tmp')
            subprocess.run(['lipo',str(binary),'-thin','arm64','-output',str(slim)],check=True)
            slim.replace(binary)
        subprocess.run(['codesign','--remove-signature',str(binary)],stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL,check=False)
        subprocess.run(['strip','-S','-x',str(binary)],check=True)
    # UAT stages multiple byte-identical copies under version aliases. Use links
    # inside the same directory, preserving every requested loader filename.
    seen={}
    for library in sorted(libraries):
        if library.is_symlink(): continue
        key=(library.parent,hashlib.sha256(library.read_bytes()).digest())
        if key in seen:
            library.unlink();library.symlink_to(seen[key].name)
        else: seen[key]=library
    # Some engine runtimes stage a shortened filename despite a versioned Mach-O ID.
    for library in libraries:
        ids=output('otool','-arch','arm64','-D',library).splitlines()[1:]
        for identity in ids:
            if not identity.startswith('@rpath/'): continue
            alias=library.parent/Path(identity).name
            if not alias.exists(): alias.symlink_to(library.name)
    libraries=[p for p in app.rglob('*.dylib') if not p.is_symlink()]
    unresolved=[]
    for binary in [executable,*libraries]:
        load=output('otool','-arch','arm64','-l',binary)
        paths=re.findall(r'cmd LC_RPATH\s+cmdsize \d+\s+path (.*?) \(offset',load)
        def expand(value):
            return Path(value.replace('@loader_path',str(binary.parent)).replace('@executable_path',str(executable.parent)))
        rpaths=[expand(value) for value in paths]
        # Libraries may inherit executable rpaths.
        if binary!=executable:
            main=output('otool','-arch','arm64','-l',executable)
            rpaths += [Path(v.replace('@loader_path',str(executable.parent)).replace('@executable_path',str(executable.parent))) for v in re.findall(r'cmd LC_RPATH\s+cmdsize \d+\s+path (.*?) \(offset',main)]
        identities=set(output('otool','-arch','arm64','-D',binary).splitlines()[1:]) if binary != executable else set()
        for line in output('otool','-arch','arm64','-L',binary).splitlines()[1:]:
            if ' (compatibility' not in line: continue
            dep=line.strip().split(' (compatibility')[0]
            if dep in identities: continue
            if dep.startswith(('/usr/lib/','/System/Library/')): continue
            candidates=[p/dep[7:] for p in rpaths] if dep.startswith('@rpath/') else [expand(dep)]
            # Absolute references to the installed engine must not mask missing bundled files.
            if not any(p.exists() and p.resolve().is_relative_to(app) for p in candidates):
                unresolved.append(f'{binary.relative_to(app)}: {dep}')
    if unresolved: raise RuntimeError('Unresolved application dependencies:\n'+'\n'.join(unresolved))
    for binary in libraries:
        subprocess.run(['codesign','--force','--sign','-',str(binary)],check=True)
    subprocess.run(['codesign','--force','--deep','--sign','-',str(app)],check=True)
    subprocess.run(['codesign','--verify','--deep','--strict',str(app)],check=True)
    print(f'Validated dependencies and ad-hoc signature: {app}',flush=True)

if __name__=='__main__':
    import sys
    finalize(sys.argv[1])
