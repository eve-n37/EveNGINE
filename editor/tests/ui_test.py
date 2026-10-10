from pathlib import Path
import json,subprocess,os
from playwright.sync_api import sync_playwright
root=Path(__file__).resolve().parents[1]
output=Path(os.environ.get('LOTA_TEST_OUTPUT',str(root.parent/'build-ui-tests')));output.mkdir(exist_ok=True)
checks=[]
with sync_playwright() as pw:
 browser=pw.chromium.launch(executable_path=os.environ.get('LOTA_CHROMIUM'),headless=True,args=['--no-sandbox','--disable-dev-shm-usage','--no-zygote','--single-process'])
 page=browser.new_page(viewport={'width':1440,'height':960},device_scale_factor=1)
 errors=[];page.on('pageerror',lambda e:errors.append(str(e)))
 page.goto((root/'ui/index.html').as_uri());page.wait_for_selector('#mapCanvas')
 def check(name,expr):
  assert page.evaluate(expr),name;checks.append(name)
 check('template loaded','LotaModel.validate(studioTest.project()).length===0')
 page.screenshot(path=str(output/'EveNGINE-Studio-v0.6.0-map.png'))
 # Paint a tile through actual pointer input, then undo/redo.
 box=page.locator('#mapCanvas').bounding_box();z=page.evaluate('document.getElementById("mapCanvas").width/512');x=box['x']+(6*16+8)*z;y=box['y']+(4*16+8)*z
 before=page.evaluate('studioTest.project().scenes[0].base[4*32+6]')
 page.mouse.click(x,y)
 check('map brush paints terrain','studioTest.project().scenes[0].base[4*32+6]===1')
 page.locator('#undo').click();assert page.evaluate('studioTest.project().scenes[0].base[4*32+6]')==before;checks.append('undo restores terrain')
 page.locator('#redo').click();check('redo restores edit','studioTest.project().scenes[0].base[4*32+6]===1')
 page.locator('#layers button').filter(has_text='Collision').click();page.mouse.click(x,y)
 check('collision painted','studioTest.project().scenes[0].collision[4*32+6]===1')
 page.locator('#undo').click()
 # Inspector changes persist in saved JSON.
 page.locator('#maps button').first.click();page.locator('[data-field=title]').fill('Edited Meadow');page.locator('[data-field=title]').press('Tab')
 check('map inspector applies','studioTest.project().scenes[0].title==="Edited Meadow"')
 page.locator('[data-view=pixels]').click();page.wait_for_selector('#pixelCanvas')
 page.locator('#palette button').nth(20).click();box=page.locator('#pixelCanvas').bounding_box();page.mouse.click(box['x']+8,box['y']+8)
 check('pixel pencil changes sprite','studioTest.project().sprites[0].pixels.startsWith("14")')
 page.locator('[data-transform=flipX]').click()
 check('sprite flip works','studioTest.project().sprites[0].pixels.slice(30,32)==="14"')
 page.locator('#copyPixels').click();page.locator('#pastePixels').click()
 # Native-free pixel import/export exercises the actual canvas/file controls.
 from PIL import Image
 png=output/'import.png';im=Image.new('RGBA',(16,24),(0,0,0,0));im.putpixel((2,2),(250,12,32,255));im.save(png)
 page.locator('#imageInput').set_input_files(png);page.wait_for_timeout(150)
 check('PNG import preserves a new colour','studioTest.project().palette.includes("#fa0c20ff")')
 with page.expect_download() as pd:page.locator('#exportPixels').click()
 exported=output/'sprite-export.png';pd.value.save_as(exported)
 assert Image.open(exported).getpixel((2,2))==(250,12,32,255);checks.append('PNG export preserves pixels')
 page.screenshot(path=str(output/'EveNGINE-Studio-v0.6.0-sprite.png'))
 page.locator('[data-view=animation]').click();page.locator('#frameMs').fill('220');page.locator('#frameMs').press('Tab')
 check('animation duration editable','studioTest.project().animations[0].frames[0].ms===220')
 page.locator('#duplicateFrame').click();check('timeline duplicates frame','studioTest.project().animations[0].frames.length===5')
 page.locator('#removeFrame').click();check('timeline removes frame','studioTest.project().animations[0].frames.length===4')
 page.screenshot(path=str(output/'EveNGINE-Studio-v0.6.0-animation.png'))
 # Add map through modal.
 page.locator('#addMap').click();page.locator('#dialogBody input').nth(0).fill('Tiny Room');page.locator('#dialogBody input').nth(1).fill('12');page.locator('#dialogBody input').nth(2).fill('10');page.locator('#dialogOk').click();page.wait_for_timeout(150)
 check('GUI creates map','studioTest.project().scenes.length===3&&studioTest.project().scenes[2].width===12')
 page.locator('#validate').click();check('edited project validates','LotaModel.validate(studioTest.project()).length===0')
 check('autosave includes edits','JSON.parse(localStorage.getItem("lota-studio-recovery")).project.scenes.length===3')
 # Save portable JSON through browser fallback and reopen it.
 with page.expect_download() as d:page.locator('#saveProject').click()
 saved=output/'ui-edited.lota';d.value.save_as(saved)
 assert json.loads(saved.read_text())['scenes'][2]['title']=='Tiny Room';checks.append('save preserves GUI changes')
 page.locator('#fileInput').set_input_files(saved);page.wait_for_timeout(200)
 check('open preserves data','studioTest.project().scenes.length===3&&studioTest.project().animations[0].frames[0].ms===220')
 page.evaluate('studioTest.load(LotaModel.createProject())')
 page.locator('#recover').click();page.locator('#dialogOk').click();page.wait_for_timeout(150)
 check('recovery restores edited project','studioTest.project().scenes.length===3')
 page.locator('[data-view=map]').click();page.locator('#maps button').first.click();page.locator('#mapTools button[data-tool=select]').click()
 page.locator('#validate').click();page.screenshot(path=str(output/'EveNGINE-Studio-v0.6.0-inspector.png'))
 page.evaluate('studioTest.load(LotaModel.createProject())');page.locator('[data-view=map]').click();page.screenshot(path=str(output/'EveNGINE-Studio-v0.6.0-map.png'));page.locator('[data-view=pixels]').click();page.screenshot(path=str(output/'EveNGINE-Studio-v0.6.0-sprite.png'));page.locator('[data-view=animation]').click();page.screenshot(path=str(output/'EveNGINE-Studio-v0.6.0-animation.png'));assert not errors,errors
 browser.close()
print(json.dumps({'checks':checks,'count':len(checks),'page_errors':errors},indent=2));(output/'ui-checks.json').write_text(json.dumps({'checks':checks,'count':len(checks),'page_errors':errors},indent=2))
