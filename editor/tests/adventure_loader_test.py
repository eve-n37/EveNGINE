import copy,json,subprocess,sys,tempfile
from pathlib import Path
binary,source=sys.argv[1:];original=json.loads(Path(source).read_text());checks=0

def npc(p):return next(o for o in p['scenes'][0]['objects'] if 'movement' in o)
def enemy(p):return next(o for o in p['scenes'][0]['objects'] if o['kind']=='enemy')
def shop(p):return next(o['shop'] for o in p['scenes'][0]['objects'] if 'shop' in o)
def reject(name,change):
 global checks
 p=copy.deepcopy(original);change(p)
 with tempfile.TemporaryDirectory() as folder:
  path=Path(folder)/'bad.eve';path.write_text(json.dumps(p));result=subprocess.run([binary,'--project',str(path),'--validate'],capture_output=True,text=True)
  assert result.returncode!=0 and result.stderr.strip(),name
 checks+=1;print('OK',name)
reject('enemy requires player combat',lambda p:p.pop('combat'))
reject('zero player health',lambda p:p['combat'].update(health=0))
reject('player attack cooldown minimum',lambda p:p['combat'].update(cooldown=0))
reject('player damage bounds',lambda p:p['combat'].update(damage=1000))
reject('enemy health bounds',lambda p:enemy(p)['combat'].update(health=-1))
reject('enemy speed bounds',lambda p:enemy(p)['combat'].update(speed=129))
reject('enemy cooldown bounds',lambda p:enemy(p)['combat'].update(cooldown=0))
reject('unknown enemy reward',lambda p:enemy(p)['combat'].update(rewardItem='missing'))
reject('reward field type',lambda p:enemy(p)['combat'].update(rewardItem=0))
reject('enemy reward quantity',lambda p:enemy(p)['combat'].update(rewardQuantity=100))
reject('NPC movement mode',lambda p:npc(p)['movement'].update(mode='fly'))
reject('empty patrol',lambda p:npc(p)['movement'].update(points=[]))
reject('negative NPC wait',lambda p:npc(p)['movement'].update(wait=-1))
reject('NPC outside map',lambda p:npc(p)['movement']['points'][0].update(x=9999))
reject('NPC blocked waypoint',lambda p:npc(p)['movement']['points'].__setitem__(0,{'x':0,'y':0}))
reject('only NPCs patrol',lambda p:npc(p).update(kind='decoration'))
reject('enemy cannot patrol',lambda p:enemy(p).update(movement=copy.deepcopy(npc(p)['movement'])))
reject('unknown currency',lambda p:shop(p).update(currency='missing'))
reject('empty shop',lambda p:shop(p).update(offers=[]))
reject('currency cannot be merchandise',lambda p:shop(p)['offers'][0].update(item=shop(p)['currency']))
reject('sell price exceeds buy price',lambda p:shop(p)['offers'][0].update(sell=100))
reject('invalid finite stock',lambda p:shop(p)['offers'][0].update(stock=-2))
reject('duplicate shop items',lambda p:shop(p)['offers'].append(copy.deepcopy(shop(p)['offers'][0])))
reject('invalid healing amount',lambda p:p['items'][2].update(heal=0))
print(checks,'adventure loader checks passed')
