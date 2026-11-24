// EveNGINE 1.2 world features: time of day, weather, puzzles, leveling, cutscenes and vehicles.
// Mirrors the native loader in engine/src/world.cpp so Studio catches the same problems.
(function (root) {
  'use strict';
  const weathers = [
      ['none', 'Clear'],
      ['rain', 'Rain'],
      ['snow', 'Snow'],
      ['fog', 'Fog'],
    ],
    stepTypes = [
      ['say', 'Show a line of dialogue'],
      ['move', 'Move a character'],
      ['face', 'Turn a character'],
      ['camera', 'Move the camera'],
      ['fade', 'Fade the screen'],
      ['wait', 'Wait'],
      ['sound', 'Play a sound'],
      ['flag', 'Set a story flag'],
    ],
    directions = ['south', 'north', 'east', 'west'],
    real = (v, lo, hi) => typeof v === 'number' && Number.isFinite(v) && v >= lo && v <= hi,
    int = (v, lo, hi) => Number.isInteger(v) && v >= lo && v <= hi,
    object = (v) => v && typeof v === 'object' && !Array.isArray(v),
    ident = (v) => typeof v === 'string' && /^[A-Za-z0-9_-]{1,64}$/.test(v),
    words = (v, max, empty = false) =>
      typeof v === 'string' && v.length <= max && (empty || v.length > 0),
    only = (v, keys) => Object.keys(v).every((k) => keys.includes(k));

  function xpForLevel(leveling, level) {
    let total = 0;
    for (let k = 1; k < level; k++)
      total += Math.round(leveling.firstLevel * Math.pow(leveling.growth, k - 1));
    return Math.min(total, 9999999);
  }
  function levelingDefaults() {
    return { maxLevel: 20, firstLevel: 10, growth: 1.5, health: 2, attack: 1, defense: 0 };
  }
  function vehicleDefaults() {
    return { speed: 1.5, terrain: 'land', acceleration: 0.5, braking: 0.4 };
  }
  function timeDefaults() {
    return { dayLength: 600, start: 8, night: '#141c40', strength: 0.55, clock: true };
  }
  function freshStep(type, p, scene) {
    let s = p.scenes.find((s) => s.id === scene),
      centre = { x: s ? s.spawn.x : 8, y: s ? s.spawn.y : 8 };
    if (type === 'say') return { type, speaker: '', text: 'Hello.' };
    if (type === 'move') return { type, actor: 'player', ...centre, speed: 48 };
    if (type === 'face') return { type, actor: 'player', direction: 'south' };
    if (type === 'camera') return { type, ...centre, seconds: 1 };
    if (type === 'fade') return { type, to: 'black', seconds: 1 };
    if (type === 'wait') return { type, seconds: 1 };
    if (type === 'sound') return { type, target: p.audio?.assets?.[0]?.id || '' };
    if (type === 'flag') return { type, target: p.flags?.[0]?.id || '', value: true };
    return { type };
  }

  function validate(p, report) {
    const flags = new Set((p.flags || []).map((f) => f.id)),
      sounds = new Set((p.audio?.assets || []).map((a) => a.id)),
      sprites = new Map(p.sprites.map((s) => [s.id, s])),
      leveling = p.leveling !== undefined;
    if (p.time !== undefined) {
      const bad = (m) => report(m, { view: 'setup' });
      let t = p.time;
      if (!object(t) || !only(t, ['dayLength', 'start', 'night', 'strength', 'clock']))
        bad('Invalid time of day settings.');
      else {
        if (t.dayLength !== undefined && !real(t.dayLength, 60, 7200))
          bad('A day lasts 60 to 7200 seconds.');
        if (t.start !== undefined && !real(t.start, 0, 23.99))
          bad('The starting hour must be 0 to 23.99.');
        if (t.night !== undefined && !/^#[0-9a-fA-F]{6}$/.test(t.night))
          bad('The night colour must use #RRGGBB.');
        if (t.strength !== undefined && !real(t.strength, 0, 0.85))
          bad('Night strength must be 0 to 0.85.');
        if (t.clock !== undefined && typeof t.clock !== 'boolean') bad('Invalid clock setting.');
      }
    }
    for (let s of p.scenes) {
      if (s.weather !== undefined && !weathers.some(([w]) => w === s.weather))
        report('Unknown weather on ' + s.title + '.', { view: 'map', scene: s.id });
      if (s.indoors !== undefined && typeof s.indoors !== 'boolean')
        report('Invalid indoors setting on ' + s.title + '.', { view: 'map', scene: s.id });
      for (let o of s.objects) {
        const bad = (m) => report(m, { view: 'map', scene: s.id, object: o.id });
        if (o.kind === 'block') {
          let c = sprites.get(o.sprite)?.collider;
          if (!c || !(c.w > 0 && c.h > 0))
            bad('Push block ' + o.id + ' needs artwork with a collider.');
        }
        if (o.kind === 'plate') {
          let pl = o.plate;
          if (pl !== undefined) {
            if (!object(pl) || !only(pl, ['flag', 'targets', 'sticky']))
              bad('Invalid pressure plate ' + o.id + '.');
            else {
              if (pl.flag !== undefined && pl.flag !== '' && !flags.has(pl.flag))
                bad('Pressure plate ' + o.id + ' uses an unknown flag.');
              if (pl.targets !== undefined) {
                if (!Array.isArray(pl.targets) || pl.targets.length > 32)
                  bad('Invalid doors on pressure plate ' + o.id + '.');
                else if (
                  new Set(pl.targets).size !== pl.targets.length ||
                  pl.targets.some((t) => s.objects.find((x) => x.id === t)?.kind !== 'door')
                )
                  bad('Pressure plate ' + o.id + ' can only open doors on its own map.');
              }
              if (pl.sticky !== undefined && typeof pl.sticky !== 'boolean')
                bad('Invalid pressure plate latch.');
            }
          }
        } else if (o.plate !== undefined) bad('Only pressure plates have plate settings.');
        if (o.kind === 'vehicle') {
          let v = o.vehicle;
          if (!object(v) || !only(v, ['speed', 'terrain', 'acceleration', 'braking']))
            bad('Vehicle ' + o.id + ' needs vehicle settings.');
          else {
            if (v.speed !== undefined && !real(v.speed, 0.25, 6))
              bad('Vehicle speed must be 0.25 to 6.');
            if (v.acceleration !== undefined && !real(v.acceleration, 0, 10))
              bad('Vehicle acceleration must be 0 to 10 seconds.');
            if (v.braking !== undefined && !real(v.braking, 0, 10))
              bad('Vehicle braking must be 0 to 10 seconds.');
            if (v.terrain !== undefined && !['land', 'water'].includes(v.terrain))
              bad('Vehicles travel on land or water.');
          }
        } else if (o.vehicle !== undefined) bad('Only vehicles have vehicle settings.');
        if (o.kind === 'enemy' && o.combat?.xp !== undefined) {
          if (!leveling) bad('Enemy XP needs leveling settings.');
          else if (!int(o.combat.xp, 0, 9999)) bad('Enemy XP must be 0 to 9999.');
        }
      }
    }
    if (leveling) {
      const bad = (m) => report(m, { view: 'setup' });
      let l = p.leveling;
      if (!p.combat) bad('Leveling needs player combat settings.');
      if (
        !object(l) ||
        !only(l, ['maxLevel', 'firstLevel', 'growth', 'health', 'attack', 'defense'])
      )
        bad('Invalid leveling settings.');
      else {
        if (l.maxLevel !== undefined && !int(l.maxLevel, 2, 99))
          bad('The top level must be 2 to 99.');
        if (l.firstLevel !== undefined && !int(l.firstLevel, 1, 9999))
          bad('XP for level 2 must be 1 to 9999.');
        if (l.growth !== undefined && !real(l.growth, 1, 3)) bad('XP growth must be 1 to 3.');
        for (let k of ['health', 'attack', 'defense'])
          if (l[k] !== undefined && !int(l[k], 0, 99)) bad('Level-up ' + k + ' must be 0 to 99.');
      }
    }
    for (let q of p.quests || [])
      if (q.rewardXp !== undefined) {
        const bad = (m) => report(m, { view: 'quests' });
        if (!leveling) bad('Quest XP needs leveling settings.');
        else if (!int(q.rewardXp, 0, 99999)) bad('Quest XP must be 0 to 99999.');
      }
    for (let e of p.events || [])
      if (!leveling && (e.actions || []).some((a) => a?.type === 'giveXp'))
        report('XP rewards need leveling settings.', { view: 'events' });
    if (p.cutscenes !== undefined) {
      if (!Array.isArray(p.cutscenes) || p.cutscenes.length > 64) {
        report('Use at most 64 cutscenes.', { view: 'cutscenes' });
        return;
      }
      let ids = new Set();
      for (let c of p.cutscenes) {
        const bad = (m) => report(m, { view: 'cutscenes', cutscene: c?.id });
        if (!object(c) || !ident(c.id) || ids.has(c.id)) {
          bad('Cutscene IDs must be unique.');
          continue;
        }
        ids.add(c.id);
        if (!words(c.name, 160)) bad('Cutscene ' + c.id + ' needs a name.');
        let s = p.scenes.find((s) => s.id === c.scene);
        if (!s) {
          bad('Cutscene ' + c.name + ' plays on an unknown map.');
          continue;
        }
        if (!Array.isArray(c.steps) || !c.steps.length || c.steps.length > 64) {
          bad('Cutscene ' + c.name + ' needs 1–64 steps.');
          continue;
        }
        const point = (st) =>
            real(st.x, 0, s.width * 16 - 0.001) && real(st.y, 0, s.height * 16 - 0.001),
          actor = (st) => st.actor === 'player' || s.objects.some((o) => o.id === st.actor);
        c.steps.forEach((st, i) => {
          let ok;
          switch (st?.type) {
            case 'wait':
              ok = real(st.seconds, 0.05, 60);
              break;
            case 'fade':
              ok = ['black', 'clear'].includes(st.to) && real(st.seconds, 0, 10);
              break;
            case 'say':
              ok =
                words(st.text, 720) && (st.speaker === undefined || words(st.speaker, 160, true));
              break;
            case 'move':
              ok = actor(st) && point(st) && (st.speed === undefined || real(st.speed, 8, 256));
              break;
            case 'face':
              ok = actor(st) && directions.includes(st.direction);
              break;
            case 'camera':
              ok = real(st.seconds, 0, 10) && (st.reset === true || point(st));
              break;
            case 'sound':
              ok = sounds.has(st.target);
              break;
            case 'flag':
              ok = flags.has(st.target) && typeof st.value === 'boolean';
              break;
            default:
              ok = false;
          }
          if (!ok) bad('Step ' + (i + 1) + ' of cutscene ' + c.name + ' is invalid.');
        });
      }
    }
  }
  const api = {
    weathers,
    stepTypes,
    directions,
    xpForLevel,
    levelingDefaults,
    timeDefaults,
    vehicleDefaults,
    freshStep,
    validate,
  };
  root.EveWorldModel = api;
  if (typeof module !== 'undefined') module.exports = api;
})(typeof window !== 'undefined' ? window : globalThis);
