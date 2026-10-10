"""Compare the editor map canvas with the native C++ reference renderer.
Set LOTA_RUNTIME to a headless lota-studio-runtime and optionally LOTA_CHROMIUM.
Only editor-only overlays/player ghost and runtime HUD are excluded.
"""
from pathlib import Path
import os,json,subprocess,base64
from playwright.sync_api import sync_playwright
from PIL import Image
import numpy as np
root=Path(__file__).resolve().parents[1]
out=Path(os.environ.get('LOTA_TEST_OUTPUT',str(root.parent/'build-ui-tests')));out.mkdir(parents=True,exist_ok=True)
runtime=os.environ['LOTA_RUNTIME']
with sync_playwright() as p:
 b=p.chromium.launch(executable_path=os.environ.get('LOTA_CHROMIUM'),headless=True,args=['--no-sandbox','--single-process','--no-zygote'])
 page=b.new_page(viewport={'width':1440,'height':960});page.goto((root/'ui/index.html').as_uri())
 project=json.loads((root/'examples/Mosslight.lota').read_text());page.evaluate('p=>studioTest.load(p)',project)
 page.evaluate('document.getElementById("grid").checked=false;studioTest.zoom(1);studioTest.view("map")')
 data=page.evaluate('document.getElementById("mapCanvas").toDataURL()');(out/'editor-world.png').write_bytes(base64.b64decode(data.split(',')[1]));b.close()
subprocess.run([runtime,'--project',str(root/'examples/Mosslight.lota'),'--capture',str(out/'runtime-world.ppm')],check=True)
editor=np.array(Image.open(out/'editor-world.png').convert('RGB').crop((56,48,456,288)));game=np.array(Image.open(out/'runtime-world.ppm').convert('RGB'))
mask=np.ones((240,400),dtype=bool);mask[:28]=False;mask[217:]=False;mask[94:126,188:212]=False
error=np.abs(editor.astype(int)-game.astype(int));bad=(error.max(axis=2)>0)&mask
assert not bad.any(),(int(bad.sum()),int(error[mask].max()))
result={'compared_pixels':int(mask.sum()),'different_pixels':int(bad.sum()),'maximum_channel_difference':int(error[mask].max())}
(out/'render-parity.json').write_text(json.dumps(result,indent=2));print(result)
