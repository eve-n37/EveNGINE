'use strict';
// Game → Setup → Export: app name, bundle ID, version, window and Mac architecture.
const XP = EveExportModel;
function exportText(label, key, placeholder, max) {
  let field = storyText(
    label,
    P().export?.[key] || '',
    (v) => {
      v = v.trim();
      let errors = [];
      XP.validate({ export: { [key]: v || undefined } }, (message) => errors.push(message));
      if (v && errors.length) {
        render();
        return notice(errors[0]);
      }
      XP.set(P(), key, v);
    },
    max
  );
  field.querySelector('input').placeholder = placeholder;
  return field;
}
function renderExportSettings() {
  let body = $('setupExport'),
    current = XP.resolve(P());
  if (!body) return;
  body.replaceChildren(
    exportText('App name', 'appName', P().title || 'My Game', 80),
    exportText('Bundle ID', 'bundleId', XP.defaultBundleId(P()), 155),
    exportText('Version', 'version', '1.0.0', 14),
    storySelect(
      'Window size',
      [1, 2, 3, 4, 5, 6].map((n) => [String(n), n + '× · ' + 400 * n + ' × ' + 240 * n]),
      String(current.windowScale),
      (v) => XP.set(P(), 'windowScale', +v)
    ),
    rcCheck('Start fullscreen', current.fullscreen, (v) => XP.set(P(), 'fullscreen', v)),
    storySelect('Mac architecture', XP.architectures, current.macArchitecture, (v) =>
      XP.set(P(), 'macArchitecture', v)
    )
  );
}
// Export targets this copy of Studio can build. Each lists its formats, the first being the default.
function exportTargets() {
  let host =
    /Linux/.test(navigator.userAgent) && !/Macintosh/.test(navigator.userAgent) ? 'linux' : 'mac';
  let targets = [];
  if (host === 'mac')
    targets.push([
      'mac',
      'macOS',
      [
        ['app', 'App'],
        ['zip', 'ZIP archive'],
        ['dmg', 'Disk image (.dmg)'],
      ],
    ]);
  // A .tar.gz keeps the game's executable permissions however it is copied, so it's the default
  // on a Mac. On Linux a plain folder works as well.
  let linux = [
    ['tar.gz', 'Archive (.tar.gz)'],
    ['folder', 'Folder'],
  ];
  targets.push(['linux', 'Linux (x86_64)', host === 'linux' ? linux.reverse() : linux]);
  return targets;
}
function rememberedExport() {
  try {
    return JSON.parse(localStorage.getItem('eve-export-choice')) || {};
  } catch {
    return {};
  }
}
async function exportGame() {
  let errors = $('validate').onclick();
  if (errors.length) return;
  let targets = exportTargets(),
    last = rememberedExport(),
    target = targets.find(([id]) => id === last.target) || targets[0];
  let formatOptions = (t) =>
    options(t[2], t[2].some(([id]) => id === last.format) ? last.format : t[2][0][0]);
  let ok = await dialog(
    'Export game',
    '<label class="field"><span>Platform</span><select id="exportTarget">' +
      options(
        targets.map(([id, name]) => [id, name]),
        target[0]
      ) +
      '</select></label><label class="field"><span>Format</span><select id="exportFormat">' +
      formatOptions(target) +
      '</select></label>',
    'Export'
  );
  if (!ok) return;
  let choice = { target: $('exportTarget').value, format: $('exportFormat').value };
  try {
    localStorage.setItem('eve-export-choice', JSON.stringify(choice));
  } catch {}
  let button = $('export');
  button.disabled = true;
  try {
    let r = await native('export', { project: P(), scene: sceneId, ...choice });
    notice(r.cancelled ? 'Cancelled' : r.message || 'Exported.');
  } catch (e) {
    notice(e.message);
  } finally {
    button.disabled = false;
  }
}
$('export').onclick = exportGame;
document.addEventListener('change', (e) => {
  if (e.target.id !== 'exportTarget') return;
  let t = exportTargets().find(([id]) => id === e.target.value);
  if (t) $('exportFormat').innerHTML = options(t[2], t[2][0][0]);
});
