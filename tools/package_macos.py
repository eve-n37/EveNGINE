#!/usr/bin/env python3
"""Package a starter-derived desktop executable and its own content."""
import argparse,plistlib,shutil
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('--binary',required=True,type=Path);p.add_argument('--content',required=True,type=Path);p.add_argument('--framework',required=True,type=Path);p.add_argument('--output',required=True,type=Path);p.add_argument('--name',default='My Game');p.add_argument('--identifier',default='dev.lota.mygame');p.add_argument('--version',default='0.1.0');a=p.parse_args()
if any(c in a.name for c in '/\\') or a.name in ('','.','..'):raise ValueError('Invalid app name')
app=a.output/(a.name+'.app');contents=app/'Contents'
if app.exists():shutil.rmtree(app)
for folder in ['MacOS','Frameworks','Resources/starter/content']:(contents/folder).mkdir(parents=True,exist_ok=True)
shutil.copy2(a.binary,contents/'MacOS/Game');(contents/'MacOS/Game').chmod(0o755)
shutil.copytree(a.content,contents/'Resources/starter/content',dirs_exist_ok=True)
shutil.copytree(a.framework,contents/'Frameworks/SDL3.framework',symlinks=True)
info={'CFBundleName':a.name,'CFBundleDisplayName':a.name,'CFBundleExecutable':'Game','CFBundleIdentifier':a.identifier,'CFBundlePackageType':'APPL','CFBundleVersion':a.version,'CFBundleShortVersionString':a.version,'LSMinimumSystemVersion':'12.0','NSHighResolutionCapable':True,'SDL_FILESYSTEM_BASE_DIR_TYPE':'resource'}
with (contents/'Info.plist').open('wb') as f:plistlib.dump(info,f)
(contents/'PkgInfo').write_bytes(b'APPL????');print(app)
