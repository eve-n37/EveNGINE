'use strict';
// EveNGINE 1.1 adventure expansion editors: equipment on items, factions, ranged enemies and
// NPC/enemy animations in Story → NPCs & trading.
const X = EveExpansionModel;
const directionNames = { south: 'South', north: 'North', east: 'East', west: 'West' };
function xHint(text) {
  let p = document.createElement('p');
  p.className = 'hint';
  p.textContent = text;
  return p;
}
function xRow(...children) {
  let row = document.createElement('div');
  row.className = 'storyObjective';
  row.append(...children);
  return row;
}
function xSubheading(text) {
  let h = document.createElement('h4');
  h.textContent = text;
  return h;
}

// Factions
function renderFactions(parent) {
  let card = rcCard(parent, 'Factions');
  card.dataset.factions = '';
  card.append(
    xHint(
      'Give enemies a faction to control who they fight. Hostile factions attack the player on sight; neutral ones only fight back when attacked. Enemies attack members of hostile factions; hostility always works both ways.'
    )
  );
  let list = P().factions || [];
  for (let f of list) {
    let row = xRow(
      storyText('Faction name', f.name, (v) => (f.name = v || f.name), 40),
      storySelect(
        'Toward the player',
        [
          ['hostile', 'Hostile: attacks on sight'],
          ['neutral', 'Neutral: fights back when attacked'],
        ],
        f.player || 'hostile',
        (v) => (f.player = v)
      )
    );
    row.dataset.faction = f.id;
    for (let other of list)
      if (other.id !== f.id)
        row.append(
          rcCheck(
            'Hostile to ' + other.name,
            (f.hostile || []).includes(other.id) || (other.hostile || []).includes(f.id),
            (v) => {
              // Hostility works both ways, so keep a single entry for each pair.
              f.hostile = (f.hostile || []).filter((h) => h !== other.id);
              other.hostile = (other.hostile || []).filter((h) => h !== f.id);
              if (v) f.hostile.push(other.id);
            }
          )
        );
    row.append(
      storyButton('Remove faction', () => {
        if (X.factionUsed(P(), f.id))
          return notice('Remove this faction from enemies and other factions first.');
        edit(() => {
          P().factions = P().factions.filter((x) => x.id !== f.id);
          if (!P().factions.length) delete P().factions;
        });
      })
    );
    card.append(row);
  }
  if (list.length < 16)
    card.append(
      storyButton('New faction', () =>
        edit(() =>
          (P().factions ||= []).push({
            id: storyId('faction'),
            name: 'Faction ' + (list.length + 1),
            player: 'hostile',
            hostile: [],
          })
        )
      )
    );
}

// Animations
function cleanAnimations(o) {
  let a = o.animations;
  if (!a) return;
  for (let state of ['walk', 'attack']) {
    if (!a[state]) continue;
    for (let d of X.directions) if (a[state][d] && !a[state][d].length) delete a[state][d];
    if (!Object.keys(a[state]).length) delete a[state];
  }
  if (!a.walk && !a.attack) delete o.animations;
}
function renderAnimations(card, o) {
  card.append(
    xSubheading('Animations'),
    xHint(
      'Frames play in the direction the character faces. Directions without frames use the placed sprite. The first walk frame is shown while standing still.'
    )
  );
  let a = o.animations;
  card.append(
    rc3Number(
      'Frame duration (ms)',
      a?.ms || 140,
      (v) => {
        if (!o.animations) return notice('Add an animation frame first.');
        o.animations.ms = v;
      },
      16,
      5000,
      1
    )
  );
  let sprites = P().sprites.map((s) => [s.id, s.id]);
  for (let state of o.kind === 'enemy' ? ['walk', 'attack'] : ['walk'])
    for (let d of X.directions) {
      let frames = a?.[state]?.[d] || [];
      let row = xRow(
        document.createTextNode((state === 'walk' ? 'Walk' : 'Attack') + ' · ' + directionNames[d])
      );
      row.dataset.animation = state + '-' + d;
      frames.forEach((sprite, i) => {
        let chip = storyButton(sprite + ' ×', () =>
          edit(() => {
            o.animations[state][d].splice(i, 1);
            cleanAnimations(o);
          })
        );
        chip.title = 'Remove frame ' + (i + 1);
        chip.className = 'frameChip';
        row.append(chip);
      });
      if (frames.length < 16)
        row.append(
          storySelect('Add frame', [['', 'Choose sprite…'], ...sprites], '', (v) => {
            if (!v) return;
            let set = ((o.animations ||= { ms: 140 })[state] ||= {});
            (set[d] ||= []).push(v);
          })
        );
      card.append(row);
    }
  let walk = P().animations.filter((c) => c.state === 'walk');
  card.append(
    storyButton('Copy player walk frames', () =>
      edit(() => {
        let set = ((o.animations ||= { ms: 140 }).walk = {});
        for (let c of walk) set[c.direction] = c.frames.slice(0, 16).map((f) => f.sprite);
      })
    )
  );
  if (o.animations)
    card.append(storyButton('Remove animations', () => edit(() => delete o.animations)));
}

// Enemy combat additions
function renderEnemyExtras(card, o) {
  let c = o.combat;
  if (!c) return;
  card.append(
    xSubheading('Ranged attacks & faction'),
    storySelect(
      'Attack style',
      [
        ['melee', 'Melee: attacks up close'],
        ['ranged', 'Ranged: shoots projectiles'],
      ],
      c.style || 'melee',
      (v) => {
        if (v === 'ranged') {
          c.style = 'ranged';
          c.projectileSpeed ||= 120;
          c.range = Math.max(c.range, 64);
        } else {
          delete c.style;
          delete c.projectileSpeed;
          c.range = Math.min(c.range, 64);
        }
      }
    )
  );
  if (c.style === 'ranged')
    card.append(
      rc3Number(
        'Projectile speed (pixels / second)',
        c.projectileSpeed,
        (v) => (c.projectileSpeed = v),
        40,
        400,
        1
      )
    );
  card.append(
    storySelect(
      'Faction',
      [['', 'No faction (always hostile)'], ...(P().factions || []).map((f) => [f.id, f.name])],
      c.faction || '',
      (v) => {
        if (v) c.faction = v;
        else delete c.faction;
      }
    )
  );
}

const expansionActors = renderActors;
renderActors = function () {
  expansionActors();
  let b = $('actorWorkspace'),
    entry = actorsList().find((a) => a.key === actorKey);
  if (entry) {
    let card = b.querySelector('[data-actor="' + CSS.escape(entry.o.id) + '"]');
    if (card) {
      if (entry.o.kind === 'enemy') renderEnemyExtras(card, entry.o);
      if (entry.o.kind === 'enemy' || entry.o.movement) renderAnimations(card, entry.o);
      else if (entry.o.animations)
        card.append(
          xHint('Animations only play for moving NPCs. Choose a movement style above.'),
          storyButton('Remove animations', () => edit(() => delete entry.o.animations))
        );
    }
  }
  renderFactions(b);
};

// Equipment on inventory items
const expansionItems = renderItems;
renderItems = function () {
  expansionItems();
  for (let [i, item] of (P().items || []).entries()) {
    let fields = $('itemList').children[i]?.querySelector('.itemFields');
    if (!fields) continue;
    let e = item.equip;
    if (e)
      for (let input of fields.querySelectorAll('input'))
        if (input.closest('label')?.textContent.startsWith('Healing amount')) {
          input.disabled = true;
          input.title = 'Equipment cannot also heal.';
        }
    fields.append(
      storySelect(
        'Equipment slot',
        [
          ['', 'Not equipment'],
          ['weapon', 'Weapon'],
          ['armor', 'Armor'],
          ['charm', 'Charm'],
        ],
        e?.slot || '',
        (v) => {
          if (!v) return delete item.equip;
          if (!P().combat) {
            render();
            return notice('Turn on player combat in Game → Setup before adding equipment.');
          }
          if ((P().items || []).some((x) => x.equip?.ranged?.ammo === item.id)) {
            render();
            return notice('This item is ammunition for a ranged weapon.');
          }
          delete item.heal;
          item.equip = { ...(item.equip || {}), slot: v };
          if (v !== 'weapon') delete item.equip.ranged;
        }
      )
    );
    if (!e) continue;
    for (let [key, label] of [
      ['attack', 'Attack bonus'],
      ['defense', 'Defense (reduces damage taken)'],
      ['health', 'Maximum health bonus'],
    ])
      fields.append(
        rc3Number(
          label,
          e[key] || 0,
          (v) => {
            if (v) e[key] = v;
            else delete e[key];
          },
          0,
          99,
          1
        )
      );
    if (e.slot !== 'weapon') continue;
    fields.append(
      rcCheck('Ranged weapon (fires projectiles)', !!e.ranged, (v) => {
        if (v) e.ranged = { speed: 160, range: 128 };
        else delete e.ranged;
      })
    );
    if (!e.ranged) continue;
    let r = e.ranged;
    fields.append(
      rc3Number('Projectile speed (pixels / second)', r.speed, (v) => (r.speed = v), 40, 400, 1),
      rc3Number('Projectile range (pixels)', r.range, (v) => (r.range = v), 16, 320, 1),
      storySelect(
        'Ammunition',
        [
          ['', 'No ammunition needed'],
          ...(P().items || [])
            .filter((x) => x.id !== item.id && !x.equip)
            .map((x) => [x.id, x.name]),
        ],
        r.ammo || '',
        (v) => {
          if (v) r.ammo = v;
          else delete r.ammo;
        }
      )
    );
  }
};
Object.assign(studioTest, { renderActors, renderItems, renderFactions });
render();
