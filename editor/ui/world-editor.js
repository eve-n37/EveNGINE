'use strict';
// EveNGINE 1.2 world editors: time & weather and leveling in Game → Setup, map weather, push
// blocks, pressure plates, vehicles and enemy XP in the inspector, and Story → Cutscenes.
const WM = EveWorldModel;
workspaceGroups.find((g) => g.id === 'story').views.push(['cutscenes', 'Cutscenes']);

function worldSection(title) {
  let t = document.createElement('div');
  t.className = 'sectionTitle';
  t.textContent = title;
  return t;
}
function worldNote(text) {
  let p = document.createElement('p');
  p.className = 'hint';
  p.textContent = text;
  return p;
}
function worldHeading(parent, title, ...buttons) {
  let h = document.createElement('div');
  h.className = 'workspaceHeading';
  let name = document.createElement('h2');
  name.textContent = title;
  let left = document.createElement('div');
  left.append(name);
  h.append(left, ...buttons);
  parent.append(h);
  return h;
}
function timeRange(condition) {
  let row = document.createElement('div');
  row.className = 'worldRow';
  row.append(
    rc3Number('From hour', condition.from, (v) => (condition.from = v), 0, 24, 0.25),
    rc3Number('To hour', condition.to, (v) => (condition.to = v), 0, 24, 0.25)
  );
  return row;
}
// Called by the behaviour menu when an object's kind changes.
function worldKindDefaults(o) {
  if (o.kind !== 'plate') delete o.plate;
  if (o.kind !== 'vehicle') delete o.vehicle;
  if (o.kind === 'plate') {
    o.plate ||= { targets: [] };
    if (!o.openSprite && P().sprites.some((s) => s.id === 'eve-plate-down'))
      o.openSprite = 'eve-plate-down';
  }
  if (o.kind === 'vehicle') o.vehicle ||= WM.vehicleDefaults();
}

// ---- Game → Setup ---------------------------------------------------------------------------

function renderWorldSettings() {
  let time = $('setupTime'),
    lv = $('setupLeveling');
  if (!time || !lv) return;
  time.replaceChildren(
    rcCheck('Day and night', !!P().time, (v) => {
      if (v) P().time = WM.timeDefaults();
      else {
        if ((P().events || []).some((e) => e.conditions?.some((c) => c.kind === 'time'))) {
          render();
          return notice('Remove time-of-day conditions from events first.');
        }
        if ((P().events || []).some((e) => e.actions.some((a) => a.type === 'setTime'))) {
          render();
          return notice('Remove "Set the time" actions from events first.');
        }
        delete P().time;
      }
    })
  );
  let t = P().time;
  if (t) {
    let colour = document.createElement('label');
    colour.className = 'field';
    colour.innerHTML = '<span>Night colour</span><input type="color">';
    let input = colour.querySelector('input');
    input.value = t.night || '#141c40';
    input.onchange = () => edit(() => (t.night = input.value));
    time.append(
      rc3Number(
        'Length of a day (seconds)',
        t.dayLength ?? 600,
        (v) => (t.dayLength = v),
        60,
        7200
      ),
      rc3Number('Starting hour', t.start ?? 8, (v) => (t.start = v), 0, 23.99, 0.25),
      colour,
      rc3Number(
        'Night strength (0–0.85)',
        t.strength ?? 0.55,
        (v) => (t.strength = v),
        0,
        0.85,
        0.05
      ),
      rcCheck('Show a clock', !!t.clock, (v) => (t.clock = v))
    );
  }
  time.append(worldNote('Weather and indoor lighting are set per map in the map inspector.'));

  lv.replaceChildren();
  if (!P().combat) {
    lv.append(worldNote('Turn on player combat to use leveling.'));
    return;
  }
  lv.append(
    rcCheck('Leveling', !!P().leveling, (v) => {
      if (v) P().leveling = WM.levelingDefaults();
      else {
        delete P().leveling;
        for (let o of P().scenes.flatMap((s) => s.objects)) if (o.combat) delete o.combat.xp;
        for (let q of P().quests || []) delete q.rewardXp;
        for (let e of P().events || []) e.actions = e.actions.filter((a) => a.type !== 'giveXp');
        P().events = (P().events || []).filter((e) => e.actions.length);
      }
    })
  );
  let l = P().leveling;
  if (!l) return;
  l = { ...WM.levelingDefaults(), ...l };
  let set = (k) => (v) => (P().leveling[k] = v);
  lv.append(
    rcNumber('Top level', l.maxLevel, set('maxLevel'), 2, 99),
    rcNumber('XP to reach level 2', l.firstLevel, set('firstLevel'), 1, 9999),
    rc3Number('XP growth per level', l.growth, set('growth'), 1, 3, 0.05),
    rcNumber('Health per level', l.health, set('health'), 0, 99),
    rcNumber('Attack per level', l.attack, set('attack'), 0, 99),
    rcNumber('Defence per level', l.defense, set('defense'), 0, 99)
  );
  let table = [2, 3, 5, 10, l.maxLevel]
    .filter((n, i, all) => n <= l.maxLevel && all.indexOf(n) === i)
    .map((n) => 'Level ' + n + ': ' + WM.xpForLevel(l, n) + ' XP');
  lv.append(worldNote(table.join(' · ')));
}

// ---- Gameplay styles ------------------------------------------------------------------------

function setStyle(s, style) {
  if (style === 'topdown') delete s.style;
  else s.style = style;
  if (style !== 'side') {
    delete s.side;
    // Ledges, ladders and hazards only exist on sidescroller maps.
    s.collision = s.collision.map((v) => (v === 1 ? 1 : 0));
  } else s.side ||= WM.sideDefaults();
  if (style !== 'raycast') {
    delete s.raycast;
    if (s.movement) delete s.movement.turnSpeed;
  } else {
    s.raycast ||= WM.raycastDefaults();
    if (s.movement) s.movement.turnSpeed ??= 140;
  }
  if (style !== 'side' && collisionKind !== 1) collisionKind = 1;
}
function colourField(title, value, set) {
  let field = storyText(title, value, set, 7);
  field.querySelector('input').type = 'color';
  return field;
}
function styleFields(s) {
  let fields = [
    storySelect('Plays as', WM.styles, s.style || 'topdown', (v) => {
      setStyle(s, v);
      render();
    }),
  ];
  if (s.style === 'side') {
    let v = { ...WM.sideDefaults(), ...s.side };
    fields.push(
      rc3Number('Gravity', v.gravity, (x) => (s.side.gravity = x), 100, 4000, 50),
      rc3Number('Jump height (tiles)', v.jump / 16, (x) => (s.side.jump = x * 16), 0.25, 10, 0.25)
    );
  }
  if (s.style === 'raycast') {
    let v = { ...WM.raycastDefaults(), ...s.raycast };
    fields.push(
      colourField('Floor colour', v.floor, (x) => (s.raycast.floor = x)),
      colourField('Ceiling colour', v.ceiling, (x) => (s.raycast.ceiling = x)),
      rc3Number(
        'View distance (tiles)',
        v.viewDistance / 16,
        (x) => (s.raycast.viewDistance = x * 16),
        2,
        64,
        0.5
      )
    );
  }
  return fields;
}
function movementFields(s) {
  let fields = [
    rcCheck('Custom movement on this map', !!s.movement, (v) => {
      if (v) s.movement = WM.movementDefaults(P(), s);
      else delete s.movement;
      render();
    }),
  ];
  if (!s.movement) return fields;
  let v = { ...WM.movementDefaults(P(), s), ...s.movement },
    set = (k) => (x) => (s.movement[k] = x);
  fields.push(
    rc3Number('Walking speed (pixels a second)', v.speed, set('speed'), 8, 512, 1),
    rc3Number('Seconds to top speed', v.acceleration, set('acceleration'), 0, 10, 0.05),
    rc3Number('Seconds to stop', v.braking, set('braking'), 0, 10, 0.05)
  );
  if (s.style === 'raycast')
    fields.push(
      rc3Number('Turning speed (degrees a second)', v.turnSpeed, set('turnSpeed'), 30, 720, 5)
    );
  return fields;
}

// ---- Inspector ------------------------------------------------------------------------------

const worldInspect = inspect;
inspect = function () {
  worldInspect();
  let box = $('inspector');
  if (box.querySelector('#makeStart')) {
    let s = S();
    box.append(worldSection('GAMEPLAY STYLE'), ...styleFields(s));
    box.append(worldSection('MOVEMENT'), ...movementFields(s));
    box.append(
      worldSection('WEATHER & LIGHT'),
      storySelect('Weather', WM.weathers, s.weather || 'none', (v) => {
        if (v === 'none') delete s.weather;
        else s.weather = v;
      }),
      rcCheck('Indoors (no night darkness)', !!s.indoors, (v) => {
        if (v) s.indoors = true;
        else delete s.indoors;
      })
    );
    return;
  }
  if (selected?.type !== 'object') return;
  let o = S().objects.find((o) => o.id === selected.id);
  if (!o) return;
  if (o.kind === 'block') {
    let c = P().sprites.find((s) => s.id === o.sprite)?.collider;
    box.append(
      worldSection('PUSH BLOCK'),
      worldNote(
        c?.w > 0 && c?.h > 0
          ? 'The player pushes it one tile at a time.'
          : 'This sprite has no collider, so it can’t be pushed. Add one in the pixel editor.'
      )
    );
  }
  if (o.kind === 'plate') {
    let plate = o.plate || {},
      doors = S().objects.filter((x) => x.kind === 'door'),
      setPlate = (fn) => {
        o.plate ||= {};
        fn(o.plate);
      };
    box.append(
      worldSection('PRESSURE PLATE'),
      storySelect(
        'Pressed sprite',
        [['', 'Keep current artwork'], ...P().sprites.map((s) => [s.id, s.name])],
        o.openSprite || '',
        (v) => {
          if (v) o.openSprite = v;
          else delete o.openSprite;
        }
      ),
      storySelect(
        'Story flag while pressed',
        [['', 'None'], ...(P().flags || []).map((f) => [f.id, f.name])],
        plate.flag || '',
        (v) => setPlate((p) => (v ? (p.flag = v) : delete p.flag))
      ),
      rcCheck('Stays pressed once triggered', !!plate.sticky, (v) =>
        setPlate((p) => (v ? (p.sticky = true) : delete p.sticky))
      )
    );
    box.append(worldSection('DOORS IT OPENS'));
    if (!doors.length) box.append(worldNote('Place a door on this map to link it.'));
    for (let d of doors)
      box.append(
        rcCheck(d.id, (plate.targets || []).includes(d.id), (v) =>
          setPlate((p) => {
            let t = new Set(p.targets || []);
            if (v) t.add(d.id);
            else t.delete(d.id);
            p.targets = [...t];
          })
        )
      );
  }
  if (o.kind === 'vehicle') {
    let v = (o.vehicle ||= WM.vehicleDefaults());
    box.append(
      worldSection('VEHICLE'),
      storySelect(
        'Travels on',
        [
          ['land', 'Land (carts, horses)'],
          ['water', 'Water (boats)'],
        ],
        v.terrain || 'land',
        (x) => (v.terrain = x)
      ),
      rc3Number('Top speed (× walking)', v.speed ?? 1.5, (x) => (v.speed = x), 0.25, 6, 0.05),
      rc3Number(
        'Seconds to top speed',
        v.acceleration ?? 0,
        (x) => (v.acceleration = x),
        0,
        10,
        0.1
      ),
      rc3Number('Seconds to stop', v.braking ?? 0, (x) => (v.braking = x), 0, 10, 0.1)
    );
  }
  if (o.kind === 'enemy' && P().leveling && o.combat)
    box.append(
      worldSection('LEVELING'),
      rcNumber(
        'XP for defeating it',
        o.combat.xp || 0,
        (v) => {
          if (v) o.combat.xp = v;
          else delete o.combat.xp;
        },
        0,
        9999
      )
    );
};

// ---- Story → Cutscenes ----------------------------------------------------------------------

function cutsceneActors(scene) {
  let s = P().scenes.find((s) => s.id === scene);
  return [['player', 'Player'], ...(s?.objects || []).map((o) => [o.id, o.speaker || o.id])];
}
function renderCutscenes() {
  let b = $('cutscenesWorkspace');
  b.replaceChildren();
  worldHeading(
    b,
    'Cutscenes',
    storyButton('New cutscene', () => {
      if ((P().cutscenes || []).length >= 64) return notice('Cutscene limit reached: 64.');
      edit(() =>
        (P().cutscenes ||= []).push({
          id: storyId('cutscene'),
          name: 'New cutscene',
          scene: S().id,
          steps: [WM.freshStep('say', P(), S().id)],
        })
      );
    })
  );
  if (!(P().cutscenes || []).length)
    b.append(worldNote('Play a cutscene from an event with the "Play cutscene" action.'));
  for (let c of P().cutscenes || []) {
    let card = rcCard(b, c.name);
    card.dataset.cutscene = c.id;
    let users = (P().events || []).filter((e) =>
      e.actions.some((a) => a.type === 'cutscene' && a.target === c.id)
    );
    card.append(
      storyText('Cutscene name', c.name, (v) => (c.name = v), 60),
      storySelect(
        'Plays on map',
        P().scenes.map((s) => [s.id, s.title]),
        c.scene,
        (v) => {
          if (users.length) return notice('Change the map of the events that play it first.');
          c.scene = v;
          for (let st of c.steps) if (st.actor && st.actor !== 'player') st.actor = 'player';
        }
      ),
      worldNote(
        users.length
          ? 'Played by: ' + users.map((e) => e.name).join(', ')
          : 'No event plays this yet. Add a "Play cutscene" action in Events & flags.'
      )
    );
    let s = P().scenes.find((s) => s.id === c.scene);
    c.steps.forEach((st, i) => {
      let row = document.createElement('div');
      row.className = 'storyObjective';
      row.append(
        storySelect('Step ' + (i + 1), WM.stepTypes, st.type, (v) => {
          let next = WM.freshStep(v, P(), c.scene);
          Object.keys(st).forEach((k) => delete st[k]);
          Object.assign(st, next);
        })
      );
      let maxX = (s?.width || 1) * 16 - 1,
        maxY = (s?.height || 1) * 16 - 1;
      if (st.type === 'say')
        row.append(
          storyText('Speaker', st.speaker || '', (v) => (st.speaker = v), 60),
          storyText('Line', st.text, (v) => (st.text = v), 240, true)
        );
      if (st.type === 'move' || st.type === 'face')
        row.append(
          storySelect('Character', cutsceneActors(c.scene), st.actor, (v) => (st.actor = v))
        );
      if (st.type === 'move')
        row.append(
          rcNumber(
            st.actor === 'player' ? 'To X (feet)' : 'To X (top left)',
            Math.round(st.x),
            (v) => (st.x = v),
            0,
            maxX
          ),
          rcNumber(
            st.actor === 'player' ? 'To Y (feet)' : 'To Y (top left)',
            Math.round(st.y),
            (v) => (st.y = v),
            0,
            maxY
          ),
          rcNumber(
            'Speed (pixels / second)',
            Math.round(st.speed ?? 48),
            (v) => (st.speed = v),
            8,
            256
          )
        );
      if (st.type === 'face')
        row.append(
          storySelect(
            'Facing',
            WM.directions.map((d) => [d, d]),
            st.direction,
            (v) => (st.direction = v)
          )
        );
      if (st.type === 'camera') {
        row.append(
          rcCheck('Back to the player', !!st.reset, (v) => {
            if (v) {
              st.reset = true;
              delete st.x;
              delete st.y;
            } else {
              delete st.reset;
              st.x = s?.spawn.x || 8;
              st.y = s?.spawn.y || 8;
            }
          })
        );
        if (!st.reset)
          row.append(
            rcNumber('Look at X', Math.round(st.x), (v) => (st.x = v), 0, maxX),
            rcNumber('Look at Y', Math.round(st.y), (v) => (st.y = v), 0, maxY)
          );
      }
      if (st.type === 'fade')
        row.append(
          storySelect(
            'Fade to',
            [
              ['black', 'Black'],
              ['clear', 'Clear'],
            ],
            st.to,
            (v) => (st.to = v)
          )
        );
      if (['camera', 'fade'].includes(st.type))
        row.append(rc3Number('Seconds', st.seconds, (v) => (st.seconds = v), 0, 10, 0.1));
      if (st.type === 'wait')
        row.append(rc3Number('Seconds', st.seconds, (v) => (st.seconds = v), 0.05, 60, 0.05));
      if (st.type === 'sound')
        row.append(
          storySelect(
            'Sound',
            (P().audio?.assets || []).map((a) => [a.id, a.name]),
            st.target,
            (v) => (st.target = v)
          )
        );
      if (st.type === 'flag')
        row.append(
          storySelect(
            'Flag',
            (P().flags || []).map((f) => [f.id, f.name]),
            st.target,
            (v) => (st.target = v)
          ),
          rcCheck('Enabled', st.value, (v) => (st.value = v))
        );
      if (i)
        row.append(
          storyButton('Move earlier', () =>
            edit(() => ([c.steps[i - 1], c.steps[i]] = [c.steps[i], c.steps[i - 1]]))
          )
        );
      if (c.steps.length > 1)
        row.append(storyButton('Remove step', () => edit(() => c.steps.splice(i, 1))));
      card.append(row);
    });
    if (c.steps.length < 64)
      card.append(
        storyButton('Add step', () => edit(() => c.steps.push(WM.freshStep('say', P(), c.scene))))
      );
    card.append(
      storyButton('Delete cutscene', () => {
        if (users.length) return notice('Remove the "Play cutscene" actions that use it first.');
        edit(() => (P().cutscenes = P().cutscenes.filter((x) => x !== c)));
      })
    );
  }
}
const worldRender = render;
render = function () {
  worldRender();
  $('cutscenesWorkspace').hidden = view !== 'cutscenes';
  if (view === 'cutscenes') {
    renderCutscenes();
    $('inspector').innerHTML =
      '<h3>Cutscenes</h3><p class="hint">Steps play in order. Players can skip with the pause key.</p>';
  }
};
render();
Object.assign(studioTest, {
  renderCutscenes,
  selectMap: (id) => {
    sceneId = id;
    selected = null;
    view = 'map';
    render();
  },
});
