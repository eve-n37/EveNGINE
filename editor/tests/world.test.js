// EveNGINE 1.2 world model checks; mirrors editor/tests/world_loader_test.py.
const assert = require('node:assert/strict'),
  fs = require('node:fs'),
  M = require('../ui/model.js'),
  W = require('../ui/world-model.js');
const base = JSON.parse(fs.readFileSync(__dirname + '/../examples/Lantern-Isles.eve'));
let count = 0;
const check = (name, fn) => {
    fn();
    count++;
    console.log('OK ' + name);
  },
  mutate = (fn) => {
    let p = M.clone(base);
    fn(p);
    return p;
  },
  reject = (name, fn, message) =>
    check(name, () => {
      let errors = M.validate(mutate(fn));
      assert.ok(
        errors.some((e) => e.toLowerCase().includes(message.toLowerCase())),
        name + ': ' + errors.join('; ')
      );
    }),
  accept = (name, fn) => check(name, () => assert.deepEqual(M.validate(mutate(fn)), [])),
  scene = (p, id) => p.scenes.find((s) => s.id === id),
  obj = (p, id) => p.scenes.flatMap((s) => s.objects).find((o) => o.id === id),
  event = (p, id) => p.events.find((e) => e.id === id);

check('Lantern Isles validates', () => assert.deepEqual(M.validate(base), []));
check('older samples still validate', () => {
  for (let name of ['Frontier-Watch', 'Adventure-Candidate', 'Keys-and-Gates'])
    assert.deepEqual(
      M.validate(JSON.parse(fs.readFileSync(__dirname + '/../examples/' + name + '.eve'))),
      [],
      name
    );
});
check('XP table matches the engine', () => {
  let l = base.leveling;
  assert.equal(W.xpForLevel(l, 2), 10);
  assert.equal(W.xpForLevel(l, 3), 25);
});
reject('short days', (p) => (p.time.dayLength = 10), 'a day lasts');
reject('night too dark', (p) => (p.time.strength = 0.9), 'night strength');
reject('bad night colour', (p) => (p.time.night = 'navy'), 'night colour');
reject('unknown weather', (p) => (scene(p, 'isle').weather = 'hail'), 'unknown weather');
reject('time conditions need time', (p) => delete p.time, 'invalid event condition');
reject(
  'time conditions need two hours',
  (p) => (event(p, 'isle-night').conditions[0].to = 19),
  'invalid event condition'
);
reject(
  'weather actions name real weather',
  (p) => (event(p, 'fog-lifts').actions[0].weather = 'hail'),
  'setWeather'
);
reject('blocks need a collider', (p) => (obj(p, 'stone').sprite = 'eve-plate'), 'collider');
reject(
  'plates open doors only',
  (p) => (obj(p, 'plate').plate.targets = ['stone']),
  'only open doors'
);
reject('plates use known flags', (p) => (obj(p, 'plate').plate.flag = 'nope'), 'unknown flag');
reject('plate settings only on plates', (p) => (obj(p, 'stone').plate = {}), 'only pressure');
reject('vehicles need settings', (p) => delete obj(p, 'boat').vehicle, 'vehicle settings');
reject('vehicle terrain', (p) => (obj(p, 'boat').vehicle.terrain = 'air'), 'land or water');
reject('vehicle speed', (p) => (obj(p, 'cart').vehicle.speed = 5), 'vehicle speed');
reject('leveling top level', (p) => (p.leveling.maxLevel = 1), 'top level');
reject('leveling needs combat', (p) => delete p.combat, 'leveling needs player combat');
reject('enemy XP needs leveling', (p) => delete p.leveling, 'enemy xp needs leveling');
reject('enemy XP bounds', (p) => (obj(p, 'thorn-raider').combat.xp = 10000), 'enemy xp');
reject(
  'XP actions need leveling',
  (p) => {
    delete p.leveling;
    for (let o of p.scenes.flatMap((s) => s.objects)) delete o.combat?.xp;
    for (let q of p.quests) delete q.rewardXp;
    event(p, 'intro').actions.push({ type: 'giveXp', quantity: 5 });
  },
  'xp rewards need leveling'
);
reject(
  'cutscene actors are on its map',
  (p) => (p.cutscenes[0].steps[4].actor = 'watch-guard'),
  'step 5'
);
reject('cutscene moves stay on the map', (p) => (p.cutscenes[0].steps[4].x = 9999), 'step 5');
reject('unknown step', (p) => p.cutscenes[0].steps.push({ type: 'dance' }), 'step 9');
reject(
  'cutscenes play on the event map',
  (p) => event(p, 'isle-night').actions.push({ type: 'cutscene', target: 'keeper-intro' }),
  'cutscene event action'
);
reject(
  'duplicate cutscenes',
  (p) => p.cutscenes.push(M.clone(p.cutscenes[0])),
  'cutscene ids must be unique'
);
accept('new steps of every type validate', (p) => {
  for (let [type] of W.stepTypes) p.cutscenes[0].steps.push(W.freshStep(type, p, 'meadow'));
});
console.log(count + ' world model checks passed');
