// EveNGINE 1.1 adventure expansion model checks (equipment, ranged, factions, animations, timers).
const assert = require('node:assert/strict'),
  fs = require('node:fs'),
  M = require('../ui/model.js'),
  X = require('../ui/expansion-model.js');
const base = JSON.parse(fs.readFileSync(__dirname + '/../examples/Frontier-Watch.eve'));
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
  reject = (name, fn) => check(name, () => assert.ok(M.validate(mutate(fn)).length, name)),
  accept = (name, fn) => check(name, () => assert.deepEqual(M.validate(mutate(fn)), [])),
  item = (p, id) => p.items.find((i) => i.id === id),
  obj = (p, id) => p.scenes.flatMap((s) => s.objects).find((o) => o.id === id),
  event = (p, id) => p.events.find((e) => e.id === id);

check('expansion sample validates', () => assert.deepEqual(M.validate(base), []));
check('1.0 adventure sample still validates', () =>
  assert.deepEqual(
    M.validate(JSON.parse(fs.readFileSync(__dirname + '/../examples/Adventure-Candidate.eve'))),
    []
  )
);
// Equipment
reject('unknown equipment slot', (p) => (item(p, 'iron-sword').equip.slot = 'ring'));
reject('equipment attack bounds', (p) => (item(p, 'iron-sword').equip.attack = 100));
reject('negative defense', (p) => (item(p, 'watch-shield').equip.defense = -1));
reject('fractional health bonus', (p) => (item(p, 'watch-shield').equip.health = 1.5));
reject('healing equipment', (p) => (item(p, 'iron-sword').heal = 2));
reject('equipment needs player combat', (p) => {
  delete p.combat;
  for (let s of p.scenes) s.objects = s.objects.filter((o) => o.kind !== 'enemy');
});
reject(
  'only weapons shoot',
  (p) => (item(p, 'watch-shield').equip.ranged = { speed: 100, range: 100 })
);
reject('projectile speed bounds', (p) => (item(p, 'sling').equip.ranged.speed = 1000));
reject('projectile range bounds', (p) => (item(p, 'sling').equip.ranged.range = 5));
reject('unknown ammunition', (p) => (item(p, 'sling').equip.ranged.ammo = 'missing'));
reject(
  'weapon cannot be its own ammunition',
  (p) => (item(p, 'sling').equip.ranged.ammo = 'sling')
);
reject(
  'equipment cannot be ammunition',
  (p) => (item(p, 'sling').equip.ranged.ammo = 'iron-sword')
);
accept('ranged weapon without ammunition', (p) => delete item(p, 'sling').equip.ranged.ammo);
check('ammunition is protected from deletion', () => {
  assert.ok(X.itemUsed(base, 'sling-stone'));
  assert.ok(M.storyItemUsed(base, 'sling-stone'));
  assert.equal(X.itemUsed(base, 'iron-sword'), false);
});
// Factions
reject('duplicate faction', (p) => p.factions.push(M.clone(p.factions[0])));
reject('unknown faction attitude', (p) => (p.factions[0].player = 'friendly'));
reject('unknown hostile faction', (p) => (p.factions[0].hostile = ['missing']));
reject('self-hostile faction', (p) => (p.factions[0].hostile = ['raiders']));
reject('unknown enemy faction', (p) => (obj(p, 'watch-guard').combat.faction = 'missing'));
reject(
  'too many factions',
  (p) => (p.factions = Array.from({ length: 17 }, (_, i) => ({ id: 'f' + i, name: 'F' })))
);
accept('enemy without a faction', (p) => delete obj(p, 'watch-guard').combat.faction);
check('factions in use are protected', () => {
  assert.ok(X.factionUsed(base, 'raiders'));
  assert.ok(X.factionUsed(base, 'watch'));
  assert.equal(
    X.factionUsed(
      mutate((p) => p.factions.push({ id: 'spare', name: 'Spare' })),
      'spare'
    ),
    false
  );
});
// Ranged enemies
reject('unknown attack style', (p) => (obj(p, 'thorn-slinger').combat.style = 'magic'));
reject(
  'ranged enemy needs projectile speed',
  (p) => delete obj(p, 'thorn-slinger').combat.projectileSpeed
);
reject('ranged enemy reach bounds', (p) => (obj(p, 'thorn-slinger').combat.range = 300));
reject('melee enemy keeps short reach', (p) => (obj(p, 'watch-guard').combat.range = 96));
// Animations
reject('stationary NPC cannot animate', (p) => delete obj(p, 'scout').movement);
reject(
  'NPCs have no attack animation',
  (p) => (obj(p, 'scout').animations.attack = { south: ['eve-raider'] })
);
reject('unknown animation sprite', (p) => (obj(p, 'scout').animations.walk.south = ['missing']));
reject('unknown animation direction', (p) => (obj(p, 'scout').animations.walk.up = ['eve-raider']));
reject('empty animation direction', (p) => (obj(p, 'scout').animations.walk.south = []));
reject(
  'too many animation frames',
  (p) => (obj(p, 'scout').animations.walk.south = Array(17).fill('eve-raider'))
);
reject('animation frame duration', (p) => (obj(p, 'scout').animations.ms = 5));
reject('animations need frames', (p) => (obj(p, 'scout').animations = { ms: 140 }));
reject('unknown animation field', (p) => (obj(p, 'scout').animations.idle = {}));
accept(
  'enemy attack animation',
  (p) => (obj(p, 'thorn-slinger').animations = { attack: { east: ['eve-raider', 'eve-guard'] } })
);
check('animation frames protect their sprites', () => {
  let sprite = base.sprites.find((s) => s.id === 'eve-hero-east1');
  assert.ok(M.assetUsage(base, 'sprite', sprite) >= 1);
  assert.equal(X.spriteUses(base, 'eve-hero-east1'), 1);
  assert.throws(() => M.deleteAsset(M.clone(base), 'sprite', 'eve-hero-east1'));
});
// Timed events
reject('timer delay too short', (p) => (event(p, 'watch-horn').delay = 0.1));
reject('timer delay too long', (p) => (event(p, 'watch-horn').delay = 4000));
reject('timer needs a delay', (p) => delete event(p, 'watch-horn').delay);
accept('fractional repeating timer', (p) =>
  Object.assign(event(p, 'watch-horn'), { once: false, delay: 0.5 })
);
check('validation links point at the right editor', () => {
  let issues = M.validateDetailed(mutate((p) => (obj(p, 'scout').animations.ms = 1)));
  assert.equal(issues[0].view, 'actors');
  assert.equal(issues[0].object, 'scout');
  issues = M.validateDetailed(mutate((p) => (item(p, 'iron-sword').equip.attack = -5)));
  assert.equal(issues[0].view, 'items');
  issues = M.validateDetailed(mutate((p) => (event(p, 'watch-horn').delay = 0)));
  assert.equal(issues[0].view, 'events');
});
check('1.1 data survives serialization and undo', () => {
  let h = new M.History(base),
    before = M.clone(h.project);
  item(h.project, 'iron-sword').equip.attack = 5;
  assert.ok(h.commit(before));
  h.undo();
  assert.equal(item(h.project, 'iron-sword').equip.attack, 2);
  assert.deepEqual(JSON.parse(JSON.stringify(base)), base);
});
console.log(count + ' expansion model checks passed');
