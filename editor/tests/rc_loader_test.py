"""Reject malformed release feature data through the production native loader."""
import copy,json,subprocess,sys,tempfile
from pathlib import Path
binary,source=sys.argv[1:]
original=json.loads(Path(source).read_text());checks=0

def reject(name,fn):
 global checks
 p=copy.deepcopy(original);fn(p)
 with tempfile.TemporaryDirectory() as folder:
  path=Path(folder)/'bad.eve';path.write_text(json.dumps(p));r=subprocess.run([binary,'--project',str(path),'--validate'],capture_output=True,text=True)
  assert r.returncode!=0 and r.stderr.strip(),name
 checks+=1;print('OK',name)
reject('unknown input action',lambda p:p.update(input={'keys':{'unknown':'T'}}))
reject('duplicate keyboard binding',lambda p:p.update(input={'keys':{'interact':'Q'}}))
reject('Escape stays reserved',lambda p:p.update(input={'keys':{'interact':'Escape'}}))
reject('duplicate gamepad binding',lambda p:p.update(input={'buttons':{'attack':'south'}}))
reject('missing title artwork',lambda p:p['menus'].update(artwork='missing'))
reject('invalid typewriter speed',lambda p:p['dialogueSettings'].update(speed=121))
reject('missing portrait',lambda p:p['scenes'][0]['objects'][0].update(portrait='missing'))
reject('invalid speaker color',lambda p:p['scenes'][0]['objects'][0].update(speakerColor='#GGHHII'))
reject('duplicate flags',lambda p:p['flags'].append(copy.deepcopy(p['flags'][0])))
reject('nonboolean initial flag',lambda p:p['flags'][0].update(initial=1))
reject('unknown trigger',lambda p:p['events'][0].update(trigger='tick'))
reject('unknown event map',lambda p:p['events'][0].update(scene='missing'))
reject('duplicate event IDs',lambda p:p['events'][1].update(id=p['events'][0]['id']))
reject('unknown interaction object',lambda p:p['events'][0].update(trigger='interact',object='missing'))
reject('trigger outside map',lambda p:p['events'][2]['area'].update(x=10000))
reject('fractional trigger coordinates',lambda p:p['events'][2]['area'].update(x=1.5))
reject('empty action sequence',lambda p:p['events'][0].update(actions=[]))
reject('unknown action',lambda p:p['events'][0]['actions'][0].update(type='script'))
reject('unknown flag condition',lambda p:p['events'][1]['conditions'][0].update(target='missing'))
reject('unknown item condition',lambda p:p['events'][0].update(conditions=[{'kind':'item','target':'missing','quantity':1}]))
reject('unknown quest condition',lambda p:p['events'][0].update(conditions=[{'kind':'quest','target':'missing','state':'active'}]))
reject('unknown reward item',lambda p:p['events'][1]['actions'][1].update(target='missing'))
reject('invalid reward quantity',lambda p:p['events'][1]['actions'][1].update(quantity=100))
reject('invalid dialogue target type',lambda p:p['events'][0].update(actions=[{'type':'dialogue','target':'meadow:key-door'}]))
reject('blocked map change arrival',lambda p:p['events'][0].update(actions=[{'type':'changeMap','target':'meadow','x':0,'y':0}]))
reject('unknown event sound',lambda p:p['events'][0].update(actions=[{'type':'sound','target':'missing'}]))
reject('autotile needs all masks',lambda p:p['autotiles'][0]['tiles'].pop())
reject('unknown autotile tile',lambda p:p['autotiles'][0]['tiles'].__setitem__(0,'missing'))
reject('unknown autotile annotation',lambda p:p['scenes'][0]['autotileCells'].__setitem__(0,'missing'))
reject('truncated autotile grid',lambda p:p['scenes'][0]['autotileCells'].pop())
reject('duplicate object group',lambda p:p['scenes'][0]['groups'].append(copy.deepcopy(p['scenes'][0]['groups'][0])))
reject('unknown object group',lambda p:p['scenes'][0]['objects'][0].update(group='missing'))
reject('negative world layout position',lambda p:p['worldLayout']['meadow'].update(x=-1))
reject('unknown world layout map',lambda p:p['worldLayout'].update(missing={'x':0,'y':0}))
print(checks,'native RC loader checks passed')
