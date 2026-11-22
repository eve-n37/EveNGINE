// Export settings model checks.
const assert = require('node:assert/strict'),
  fs = require('node:fs'),
  M = require('../ui/model.js'),
  XP = require('../ui/export-model.js');
const base = JSON.parse(fs.readFileSync(__dirname + '/../examples/Frontier-Watch.eve'));
let count = 0;
const check = (name, fn) => {
    fn();
    count++;
    console.log('OK ' + name);
  },
  withExport = (settings) => Object.assign(M.clone(base), { export: settings }),
  accept = (name, settings) =>
    check(name, () => assert.deepEqual(M.validate(withExport(settings)), [])),
  reject = (name, settings) =>
    check(name, () => assert.ok(M.validate(withExport(settings)).length, name));

check('defaults come from the project', () => {
  let r = XP.resolve(base);
  assert.equal(r.appName, base.title);
  assert.equal(r.windowScale, 3);
  assert.equal(r.fullscreen, false);
  assert.equal(r.macArchitecture, 'universal');
  assert.match(r.bundleId, /^dev\.lota\.game\.[A-Za-z0-9-]+$/);
});
check('default bundle IDs replace characters Apple rejects', () =>
  assert.equal(XP.defaultBundleId({ id: 'my_game' }), 'dev.lota.game.my-game')
);
accept('full settings validate', {
  appName: 'Frontier Watch',
  bundleId: 'com.example.frontier-watch',
  version: '2.1.0',
  windowScale: 4,
  fullscreen: true,
  macArchitecture: 'arm64',
});
reject('empty app name', { appName: ' ' });
reject('app name with a slash', { appName: 'A/B' });
reject('bundle ID without a dot', { bundleId: 'frontier' });
reject('bundle ID with an underscore', { bundleId: 'com.example.my_game' });
reject('four-part version', { version: '1.2.3.4' });
reject('window scale out of range', { windowScale: 7 });
reject('fractional window scale', { windowScale: 2.5 });
reject('fullscreen as a number', { fullscreen: 1 });
reject('unknown architecture', { macArchitecture: 'ppc' });
reject('unknown key', { colour: 'red' });
check('setting a default value removes it, and an empty block goes away', () => {
  let p = M.clone(base);
  XP.set(p, 'windowScale', 2);
  assert.deepEqual(p.export, { windowScale: 2 });
  XP.set(p, 'windowScale', 3);
  assert.equal(p.export, undefined);
  XP.set(p, 'appName', '');
  assert.equal(p.export, undefined);
});
console.log(count + ' export model checks passed');
