// Export settings: app name, bundle ID, version, window and Mac architecture.
// The engine reads appName, windowScale and fullscreen (engine/src/release.cpp); the exporters
// read the rest.
(function (root) {
  'use strict';
  const architectures = [
      ['universal', 'Universal (Apple Silicon and Intel)'],
      ['arm64', 'Apple Silicon only'],
      ['x86_64', 'Intel only'],
    ],
    object = (v) => v && typeof v === 'object' && !Array.isArray(v),
    bundlePattern = /^[A-Za-z][A-Za-z0-9-]*(\.[A-Za-z0-9-]+)+$/,
    versionPattern = /^[0-9]{1,4}(\.[0-9]{1,4}){0,2}$/;
  function defaultBundleId(p) {
    return 'dev.lota.game.' + String(p.id || 'game').replace(/[^A-Za-z0-9-]/g, '-');
  }
  // Settings with every default filled in, as the exporters use them.
  function resolve(p) {
    let e = object(p.export) ? p.export : {};
    return {
      appName: e.appName || p.title || 'My Game',
      bundleId: e.bundleId || defaultBundleId(p),
      version: e.version || '1.0.0',
      windowScale: e.windowScale ?? 3,
      fullscreen: !!e.fullscreen,
      macArchitecture: e.macArchitecture || 'universal',
    };
  }
  // Stores one setting, dropping it (and the block) when it goes back to the default.
  function set(p, key, value) {
    let defaults = { windowScale: 3, fullscreen: false, macArchitecture: 'universal' };
    let e = object(p.export) ? p.export : {};
    if (value === '' || value === undefined || value === defaults[key]) delete e[key];
    else e[key] = value;
    if (Object.keys(e).length) p.export = e;
    else delete p.export;
  }
  function validate(p, report) {
    if (p.export === undefined) return;
    const bad = (message) => report(message, { view: 'setup' });
    let e = p.export;
    if (!object(e)) return bad('Invalid export settings.');
    for (let key of Object.keys(e))
      if (
        ![
          'appName',
          'bundleId',
          'version',
          'windowScale',
          'fullscreen',
          'macArchitecture',
        ].includes(key)
      )
        bad('Unknown export setting ' + key + '.');
    if (
      e.appName !== undefined &&
      (typeof e.appName !== 'string' ||
        !e.appName.trim() ||
        e.appName.length > 80 ||
        /[/\\:\u0000-\u001f]/.test(e.appName))
    )
      bad('App names need 1–80 characters and no slashes or colons.');
    if (
      e.bundleId !== undefined &&
      (typeof e.bundleId !== 'string' || e.bundleId.length > 155 || !bundlePattern.test(e.bundleId))
    )
      bad('Bundle IDs look like com.yourname.game: letters, digits and hyphens between dots.');
    if (
      e.version !== undefined &&
      (typeof e.version !== 'string' || !versionPattern.test(e.version))
    )
      bad('Versions are one to three numbers separated by dots, like 1.0.2.');
    if (
      e.windowScale !== undefined &&
      !(Number.isInteger(e.windowScale) && e.windowScale >= 1 && e.windowScale <= 6)
    )
      bad('Window size must be 1× to 6×.');
    if (e.fullscreen !== undefined && typeof e.fullscreen !== 'boolean')
      bad('Invalid fullscreen setting.');
    if (e.macArchitecture !== undefined && !architectures.some(([id]) => id === e.macArchitecture))
      bad('Unknown Mac architecture.');
  }
  const api = { architectures, defaultBundleId, resolve, set, validate };
  root.EveExportModel = api;
  if (typeof module !== 'undefined') module.exports = api;
})(typeof window !== 'undefined' ? window : globalThis);
