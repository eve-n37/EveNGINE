from pathlib import Path
import json,os,wave,struct,math
from PIL import Image
from playwright.sync_api import sync_playwright
root=Path(__file__).resolve().parents[1];output=Path(os.environ.get('LOTA_TEST_OUTPUT',str(root.parent/'build-feature-tests')));output.mkdir(exist_ok=True);checks=[]
with sync_playwright() as pw:
 browser=pw.chromium.launch(executable_path=os.environ.get('LOTA_CHROMIUM'),headless=True,args=['--no-sandbox','--disable-dev-shm-usage','--no-zygote','--single-process'])
 page=browser.new_page(viewport={'width':1440,'height':960},device_scale_factor=1);errors=[];page.on('pageerror',lambda e:errors.append(str(e)));page.goto((root/'ui/index.html').as_uri());page.wait_for_selector('#mapCanvas')
 def check(name,expr):
  value=page.evaluate(expr)
  if not value:
   (output/'feature-ui-failure.json').write_text(json.dumps(page.evaluate('({project:studioTest.project(),features:studioTest.features(),notice:document.getElementById("notice").textContent,dialogValue:document.getElementById("dialog").returnValue,body:document.getElementById("dialogBody").innerHTML})'),indent=2));page.screenshot(path=str(output/'feature-ui-failure.png'))
  assert value,name;checks.append(name)
 def modal():page.wait_for_selector('#dialog[open]')
 def apply():page.locator('#dialogOk').click();page.wait_for_selector('#dialog[open]',state='hidden');page.wait_for_timeout(80)
 page.evaluate('studioTest.load('+ (root/'examples/Mosslight.eve').read_text()+')')
 page.locator('[data-view=assets]').click();page.locator('#assetSearch').fill('south');assert page.locator('.assetCard').count()==4;checks.append('asset browser searches names')
 page.locator('#assetSearch').fill('');page.locator('#assetKind').select_option('tile');assert page.locator('.assetCard').count()==5;checks.append('browser filters tiles')
 page.locator('.assetCard').first.click();page.locator('[data-field=group]').fill('Terrain kit');page.locator('[data-field=group]').press('Tab');check('asset group editable','studioTest.project().tiles[0].group==="Terrain kit"')
 page.locator('#assetGroup').select_option('Terrain kit');assert page.locator('.assetCard').count()==1;checks.append('browser filters groups');page.locator('#assetGroup').select_option('');page.locator('.assetCard').first.dblclick();check('browser double click opens artwork','document.getElementById("pixelWorkspace").hidden===false');page.locator('[data-view=assets]').click()
 # Sheet import: two opaque tiles, alpha preserved, exact grid.
 image=output/'tilesheet.png';im=Image.new('RGBA',(32,16),(66,110,77,255));im.paste((202,164,85,255),(16,0,32,16));im.putpixel((1,1),(0,0,0,0));im.save(image)
 page.locator('#sheetInput').set_input_files(image);modal();assert '2 tile assets' in page.locator('#sheetInfo').inner_text();checks.append('sheet importer previews grid count');page.screenshot(path=str(output/'EveNGINE-Studio-v0.6.0-import.png'));apply()
 check('sheet import commits all assets and alpha','studioTest.project().tiles.length===7&&studioTest.project().tiles[5].pixels.slice(34,36)==="00"')
 check('sheet import is a single undo action','studioTest.history().undoStack.length===2')
 page.locator('#undo').click();check('undo removes sheet and palette','studioTest.project().tiles.length===5');page.locator('#redo').click();check('redo restores sheet','studioTest.project().tiles.length===7')
 # Invalid sheet disables apply and leaves the project intact.
 bad=output/'bad-sheet.png';Image.new('RGBA',(33,16),(255,0,0,255)).save(bad);page.locator('#sheetInput').set_input_files(bad);modal();assert page.locator('#dialogOk').is_disabled();checks.append('incomplete grid blocks import');page.locator('#dialogButtons button').first.click();check('invalid sheet leaves project intact','studioTest.project().tiles.length===7')
 # Sprite sheet, animation assignment.
 frames=output/'sprite-sheet.png';p=page.evaluate('studioTest.project()');im=Image.new('RGBA',(64,24),(0,0,0,0))
 for frame in range(4):
  a=p['sprites'][frame];pix=bytes.fromhex(a['pixels']);tileim=Image.new('RGBA',(a['width'],a['height']));tileim.putdata([tuple(bytes.fromhex((p['palette'][v][1:]+('ff' if len(p['palette'][v])==7 else '')))) for v in pix]);im.paste(tileim,(frame*16,0))
 im.save(frames);page.locator('#sheetInput').set_input_files(frames);modal();page.locator('#sheetKind').select_option('sprite');page.locator('#dialogBody input').nth(1).fill('24');page.locator('#sheetClip').select_option('0');apply();check('sprite sheet assigns ordered animation frames','studioTest.project().sprites.length===22&&studioTest.project().animations[0].frames.every(f=>f.sprite.startsWith("sheet-"))')
 # Unused deletion and reference protection.
 page.locator('#assetKind').select_option('tile');page.locator('#assetSearch').fill('tilesheet 1');page.locator('.assetCard').first.click();page.locator('#deleteAsset').click();modal();apply();check('unused artwork can be deleted','studioTest.project().tiles.length===6')
 page.locator('#assetSearch').fill('Meadow');page.locator('.assetCard').first.click();page.locator('#deleteAsset').click();assert not page.locator('#dialog').evaluate('(d)=>d.open');assert 'still used' in page.locator('#notice').inner_text();checks.append('used artwork cannot be deleted')
 page.locator('#assetSearch').fill('');page.locator('#assetKind').select_option('all');page.screenshot(path=str(output/'EveNGINE-Studio-v0.6.0-assets.png'))
 # Reverse drag map selection and four-layer copying.
 page.locator('[data-view=map]').click();page.locator('#maps button').first.click();page.evaluate('studioTest.zoom(1)');page.locator('[data-tool=region]').click();box=page.locator('#mapCanvas').bounding_box()
 def point(x,y):return (box['x']+x*16+8,box['y']+y*16+8)
 page.mouse.move(*point(24,5));page.mouse.down();page.mouse.move(*point(22,3),steps=5);page.mouse.up();check('region drag works in either direction','studioTest.features().region.x===22&&studioTest.features().region.y===3&&studioTest.features().region.w===3&&studioTest.features().region.h===3')
 page.locator('#copyRegion').click();check('region copies four layers','Object.keys(studioTest.features().mapClipboard.layers).length===4')
 page.locator('#saveStamp').click();modal();page.locator('#dialogBody input').fill('Small pond');apply();check('saved stamp uses stable tile IDs','studioTest.project().stamps.length===2&&studioTest.project().stamps[1].layers.base.every(v=>v==="water")')
 page.locator('#cutRegion').click();check('cut clears terrain and water','studioTest.project().scenes[0].base[3*32+22]===0&&studioTest.project().scenes[0].water[3*32+22]===0');page.locator('#undo').click();check('undo restores all cut layers','studioTest.project().scenes[0].base[3*32+22]===3&&studioTest.project().scenes[0].water[3*32+22]===1')
 page.locator('#maps button').nth(1).click();check('map switch clears old selection','studioTest.features().region===null');page.locator('#pasteRegion').click();box=page.locator('#mapCanvas').bounding_box();page.mouse.click(*point(6,3));check('stamp pastes across maps','studioTest.project().scenes[1].base[3*32+6]===3&&studioTest.project().scenes[1].water[3*32+6]===1')
 page.locator('#undo').click();check('undo reverses whole stamp','studioTest.project().scenes[1].water[3*32+6]===0');page.locator('#stamps button').filter(has_text='Small pond').click();page.mouse.click(*point(6,3));page.screenshot(path=str(output/'EveNGINE-Studio-v0.6.0-stamps.png'));page.locator('#stamps button').filter(has_text='Small pond').dblclick();modal();page.locator('#dialogBody input').fill('Pond stamp');apply();check('saved stamps can be renamed','studioTest.project().stamps[1].name==="Pond stamp"')
 # Import a real WAV and preview via WebAudio, then assign map and event cues.
 sound=output/'test-chime.wav'
 with wave.open(str(sound),'wb') as w:
  w.setnchannels(1);w.setsampwidth(2);w.setframerate(16000);w.writeframes(b''.join(struct.pack('<h',int(4000*math.sin(i/16000*660*2*math.pi))) for i in range(4000)))
 page.locator('[data-view=audio]').click();page.locator('#audioInput').set_input_files(sound);page.wait_for_function('()=>studioTest.project().audio.assets.length===3');check('WAV is embedded and validates','LotaModel.validate(studioTest.project()).length===0')
 page.locator('.audioRow').last.locator('button').nth(1).click();modal();page.locator('#dialogBody input').fill('Test chime');apply();check('audio clips can be renamed','studioTest.project().audio.assets[2].name==="Test chime"');sounds=page.evaluate('studioTest.project().audio.assets');new=sounds[-1]['id'];page.locator('#sceneMusic').select_option(new);page.locator('[data-cue=attack]').select_option(new);page.locator('#musicVolume').fill('0.25');page.locator('#musicVolume').dispatch_event('change');page.locator('#sfxVolume').fill('0.35');page.locator('#sfxVolume').dispatch_event('change');check('audio assignments and gain persist','studioTest.project().scenes[1].music==="'+new+'"&&studioTest.project().audio.musicVolume===.25&&studioTest.project().audio.sfxVolume===.35&&studioTest.project().audio.cues.attack==="'+new+'"')
 page.locator('.audioRow').last.locator('button').first.click();page.wait_for_function('()=>document.getElementById("audioStatus").textContent.startsWith("Previewing")');checks.append('WebAudio previews imported PCM');page.locator('#stopAudio').click();check('preview stop clears source','audioPreview===null')
 page.locator('.audioRow').last.locator('button').last.click();assert 'before deleting' in page.locator('#notice').inner_text();checks.append('assigned sound deletion is protected')
 page.screenshot(path=str(output/'EveNGINE-Studio-v0.6.0-audio.png'))
 broken=output/'broken.wav';broken.write_bytes(b'RIFF\x00\x00\x00\x00WAVE');page.locator('#audioInput').set_input_files(broken);page.wait_for_timeout(100);check('bad WAV cannot partially import','studioTest.project().audio.assets.length===3')
 with page.expect_download() as d:page.locator('#saveProject').click()
 saved=output/'v06-ui-edited.eve';d.value.save_as(saved);page.locator('#fileInput').set_input_files(saved);page.wait_for_timeout(100);check('save and reopen preserve stamps and embedded sounds','studioTest.project().stamps.length===2&&studioTest.project().audio.assets.length===3&&LotaModel.validate(studioTest.project()).length===0')
 assert not errors,errors;browser.close()
report={'count':len(checks),'checks':checks,'page_errors':errors};print(json.dumps(report,indent=2));(output/'feature-ui-checks.json').write_text(json.dumps(report,indent=2))
