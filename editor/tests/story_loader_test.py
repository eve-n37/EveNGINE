"""Verify native story validation rejects malformed authored projects."""
import copy
import json
import subprocess
import sys
import tempfile
from pathlib import Path

binary, source = sys.argv[1:]
original = json.loads(Path(source).read_text())
checks = 0

def keeper(project):
    return next(o for o in project['scenes'][0]['objects'] if o['id'] == 'keeper')

def reject(name, change):
    global checks
    project = copy.deepcopy(original)
    change(project)
    with tempfile.TemporaryDirectory() as folder:
        path = Path(folder) / 'invalid.eve'
        path.write_text(json.dumps(project))
        result = subprocess.run([binary, '--project', str(path), '--validate'], capture_output=True, text=True)
        assert result.returncode != 0, name
        assert result.stderr.strip(), name
    checks += 1
    print('OK', name)

reject('unknown quest action', lambda p: keeper(p)['dialogue']['nodes'][0]['choices'][0].update(quest='missing'))
reject('unknown dialogue destination', lambda p: keeper(p)['dialogue']['nodes'][0]['choices'][0].update(next='missing'))
reject('invalid quest condition', lambda p: keeper(p)['dialogue']['nodes'][0]['choices'][0].update(conditionState='invalid'))
reject('unreachable node', lambda p: keeper(p)['dialogue']['nodes'].append({'id': 'unused', 'text': 'Unused', 'choices': []}))
reject('duplicate node', lambda p: keeper(p)['dialogue']['nodes'].append(copy.deepcopy(keeper(p)['dialogue']['nodes'][0])))
reject('too many choices', lambda p: keeper(p)['dialogue']['nodes'][0]['choices'].append({'text': 'Extra', 'next': ''}))
reject('unknown objective item', lambda p: p['quests'][0]['objectives'][3].update(target='missing'))
reject('invalid talk target', lambda p: p['quests'][0]['objectives'][2].update(target='meadow:key-door'))
reject('unknown visit map', lambda p: p['quests'][0]['objectives'][1].update(target='missing'))
reject('duplicate objective', lambda p: p['quests'][0]['objectives'].append(copy.deepcopy(p['quests'][0]['objectives'][0])))
reject('invalid objective quantity', lambda p: p['quests'][0]['objectives'][3].update(quantity=1.5))
reject('unknown reward', lambda p: p['quests'][0].update(rewardItem='missing'))
reject('duplicate quest', lambda p: p['quests'].append(copy.deepcopy(p['quests'][0])))
print(checks, 'native story loader checks passed')
