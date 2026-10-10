"""Validate universal Mac app/resource packages without executing macOS code."""
from pathlib import Path
import plistlib,struct,json,subprocess,argparse
p=argparse.ArgumentParser();p.add_argument('sdk',type=Path);a=p.parse_args();checks=[]
def check(ok,name):
 if not ok:raise AssertionError(name)
 checks.append(name)
def slices(path):
 b=path.read_bytes();magic=struct.unpack_from('>I',b)[0];check(magic==0xcafebabe,str(path.name)+' universal Mach-O');n=struct.unpack_from('>I',b,4)[0];out={}
 for i in range(n):
  cpu,sub,off,size,align=struct.unpack_from('>IIIII',b,8+i*20);out[cpu]=b[off:off+size]
 check(set(out)=={0x100000c,0x1000007},str(path.name)+' ARM64 + x86_64');return out
def dependencies(path):
 out=[]
 for cpu,b in slices(path).items():
  check(struct.unpack_from('<I',b)[0]==0xfeedfacf,'64-bit slice header');n=struct.unpack_from('<I',b,16)[0];offset=32;links=[];rpaths=[]
  for i in range(n):
   cmd,size=struct.unpack_from('<II',b,offset)
   if cmd in [0xc,0x80000018,0x8000001f,0x8000001c]:
    at=struct.unpack_from('<I',b,offset+8)[0];s=b[offset+at:offset+size].split(b'\0')[0].decode();(rpaths if cmd==0x8000001c else links).append(s)
   offset+=size
  out.append({'cpu':hex(cpu),'links':links,'rpaths':rpaths})
 return out
editor=a.sdk/'macOS/EveNGINE-Studio.app/Contents';example=a.sdk/'macOS/Mosslight-Example.app/Contents'
demo=a.sdk/'macOS/Keys-and-Gates.app/Contents'
for app in [editor,example,demo]:
 info=plistlib.loads((app/'Info.plist').read_bytes());binary=app/'MacOS'/info['CFBundleExecutable'];check(binary.is_file() and bool(binary.stat().st_mode&0o111),'bundle executable present/executable');dependencies(binary);check(info['LSMinimumSystemVersion']=='12.0','Mac 12 deployment target');f=app/'Frameworks/SDL3.framework';check((f/'SDL3').is_file(),'SDL framework resolves');check((f/'Versions/Current').is_symlink(),'framework version symlink preserved')
for name in ['model.js','editor.js','features.js','sample-audio.js','builtins-data.js','demo-project.js','interactions-editor.js','style.css','index.html']:check((editor/'Resources/ui'/name).read_bytes()==(a.sdk/'editor/ui'/name).read_bytes(),'packaged '+name+' matches tested source')
for app in [editor,example,demo]:check((app/'Resources/font.pam').is_file(),'font resource present')
links=dependencies(editor/'Resources/Runtime');check(all('@executable_path/../Frameworks' in s['rpaths'] for s in links),'runtime relocatable framework path');check(all('/usr/lib/libc++.1.dylib' in s['links'] for s in links),'runtime uses system C++ library')
check((example/'Resources/project.lota').read_bytes()==(a.sdk/'editor/examples/Mosslight.lota').read_bytes(),'sample export contains exact project')
check((demo/'Resources/project.lota').read_bytes()==(a.sdk/'editor/examples/Keys-and-Gates.eve').read_bytes(),'interaction export contains exact project')
for name in ['core','platform','studio']:slices(a.sdk/('macOS/lib/liblota_'+name+'.a'))
check((a.sdk/'macOS/include/lota/audio.hpp').read_bytes()==(a.sdk/'engine/include/lota/audio.hpp').read_bytes(),'installed audio header matches source')
check((a.sdk/'macOS/include/lota/platform.hpp').read_bytes()==(a.sdk/'engine/include/lota/platform.hpp').read_bytes(),'installed platform header matches source')
check((a.sdk/'macOS/include/lota/studio.hpp').read_bytes()==(a.sdk/'engine/include/lota/studio.hpp').read_bytes(),'installed Studio header matches source')
check('0.6.0' in (a.sdk/'macOS/lib/cmake/EveNGINE/EveNGINEConfigVersion.cmake').read_text(),'installed CMake version is 0.6.0')
print(json.dumps({'count':len(checks),'checks':checks,'runtime_dependencies':links},indent=2))
