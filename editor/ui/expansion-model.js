// EveNGINE 1.1 adventure expansion: equipment, ranged combat, factions, actor animations.
// Mirrors the native loader in engine/src/expansion.cpp so Studio catches the same problems.
(function (root) {
  'use strict';
  const number = (v, a, b) => typeof v === 'number' && Number.isFinite(v) && v >= a && v <= b,
    integer = (v, a, b) => Number.isInteger(v) && number(v, a, b),
    object = (v) => v && typeof v === 'object' && !Array.isArray(v),
    ident = (v) => typeof v === 'string' && /^[A-Za-z0-9_-]{1,64}$/.test(v),
    directions = ['south', 'north', 'east', 'west'],
    slots = ['weapon', 'armor', 'charm'];
  function validate(p, report) {
    let items = new Map((p.items || []).map((i) => [i.id, i])),
      sprites = new Set(p.sprites.map((s) => s.id)),
      factions = new Set();
    if (p.factions !== undefined) {
      const bad = (message) => report(message, { view: 'actors' });
      if (!Array.isArray(p.factions) || p.factions.length > 16) bad('Use at most 16 factions.');
      else {
        for (let f of p.factions) {
          if (
            !object(f) ||
            !ident(f.id) ||
            factions.has(f.id) ||
            typeof f.name !== 'string' ||
            !f.name ||
            f.name.length > 40 ||
            (f.player !== undefined && !['hostile', 'neutral'].includes(f.player))
          )
            bad('Invalid faction ' + (f?.name || f?.id || '') + '.');
          if (object(f)) factions.add(f.id);
        }
        for (let f of p.factions)
          if (
            object(f) &&
            f.hostile !== undefined &&
            (!Array.isArray(f.hostile) ||
              f.hostile.length > 16 ||
              f.hostile.some((h) => !factions.has(h) || h === f.id))
          )
            bad('Invalid hostile factions for ' + f.name + '.');
      }
    }
    for (let item of p.items || []) {
      if (item.equip === undefined) continue;
      const bad = (message) => report(message, { view: 'items' });
      let e = item.equip;
      if (!object(e) || !slots.includes(e.slot)) {
        bad('Invalid equipment in ' + item.name + '.');
        continue;
      }
      if (!p.combat) bad('Equipment needs player combat (Game → Setup → Player combat).');
      if (item.heal !== undefined) bad(item.name + ' cannot be both healing and equipment.');
      for (let k of ['attack', 'defense', 'health'])
        if (e[k] !== undefined && !integer(e[k], 0, 99))
          bad('Invalid ' + k + ' bonus in ' + item.name + '.');
      if (e.ranged !== undefined) {
        let r = e.ranged;
        if (
          e.slot !== 'weapon' ||
          !object(r) ||
          !number(r.speed, 40, 400) ||
          !number(r.range, 16, 320) ||
          (r.ammo !== undefined &&
            r.ammo !== '' &&
            (!items.has(r.ammo) || r.ammo === item.id || items.get(r.ammo).equip !== undefined))
        )
          bad('Invalid ranged weapon ' + item.name + '.');
      }
    }
    for (let s of p.scenes)
      for (let o of s.objects) {
        const bad = (message) => report(message, { view: 'actors', scene: s.id, object: o.id });
        let label = s.title + ' / ' + o.id;
        if (o.kind === 'enemy' && object(o.combat)) {
          let c = o.combat;
          if (c.style !== undefined && !['melee', 'ranged'].includes(c.style))
            bad('Unknown attack style in ' + label + '.');
          if (c.style === 'ranged' && !number(c.projectileSpeed, 40, 400))
            bad('Ranged enemy ' + label + ' needs a projectile speed from 40 to 400.');
          if (c.faction !== undefined && c.faction !== '' && !factions.has(c.faction))
            bad('Unknown faction in ' + label + '.');
        }
        if (o.animations === undefined) continue;
        let a = o.animations;
        if (o.kind !== 'enemy' && !(o.kind === 'npc' && o.movement)) {
          bad('Only moving NPCs and enemies can animate (' + label + ').');
          continue;
        }
        if (
          !object(a) ||
          Object.keys(a).some((k) => !['ms', 'walk', 'attack'].includes(k)) ||
          (a.ms !== undefined && !integer(a.ms, 16, 5000))
        ) {
          bad('Invalid animations in ' + label + '.');
          continue;
        }
        if (a.attack !== undefined && o.kind !== 'enemy')
          bad('Only enemies have attack animations (' + label + ').');
        let frames = 0;
        for (let state of ['walk', 'attack']) {
          let set = a[state];
          if (set === undefined) continue;
          if (!object(set) || Object.keys(set).some((d) => !directions.includes(d))) {
            bad('Invalid ' + state + ' animation in ' + label + '.');
            continue;
          }
          for (let d of directions) {
            if (set[d] === undefined) continue;
            if (
              !Array.isArray(set[d]) ||
              !set[d].length ||
              set[d].length > 16 ||
              set[d].some((f) => !sprites.has(f))
            )
              bad('Animations need 1–16 existing sprites per direction (' + label + ').');
            else frames += set[d].length;
          }
        }
        if (!frames) bad('Add animation frames or remove animations from ' + label + '.');
      }
  }
  function itemUsed(p, id) {
    return (p.items || []).some((i) => i.equip?.ranged?.ammo === id);
  }
  function spriteUses(p, id) {
    let n = 0;
    for (let s of p.scenes)
      for (let o of s.objects)
        for (let state of ['walk', 'attack'])
          for (let d of directions)
            n += (o.animations?.[state]?.[d] || []).filter((f) => f === id).length;
    return n;
  }
  function factionUsed(p, id) {
    return (
      p.scenes.some((s) => s.objects.some((o) => o.combat?.faction === id)) ||
      (p.factions || []).some((f) => f.id !== id && (f.hostile || []).includes(id))
    );
  }
  const api = { validate, itemUsed, spriteUses, factionUsed, directions, slots };
  root.EveExpansionModel = api;
  if (typeof module !== 'undefined') module.exports = api;
})(typeof window !== 'undefined' ? window : globalThis);
