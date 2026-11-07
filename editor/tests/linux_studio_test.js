// Runs inside EveNGINE Studio for Linux (EVE_STUDIO_TEST_SCRIPT) and drives every native bridge
// action. File dialogs are answered by EVE_STUDIO_TEST_CHOOSE; see linux_studio_test.py.
(async () => {
  const out = [];
  const ok = (name, value) => {
    if (!value) throw Error(name);
    out.push('OK ' + name);
  };
  const fails = async (promise) => {
    try {
      await promise;
      return '';
    } catch (e) {
      return e.message || 'failed';
    }
  };
  try {
    const folder = document.documentElement.dataset.testFolder || window.EVE_TEST_FOLDER;
    ok('native bridge is available', !!window.webkit?.messageHandlers?.studio);
    let opened = await native('open');
    ok('open reads the chosen project', opened.project.id === 'frontier-watch');
    ok('open returns the project path', opened.path.endsWith('Frontier-Watch.eve'));
    await importProjectData(opened.project, opened.path);
    ok('opened project loads into the editor', P().title === 'Frontier Watch');
    let saved = await native('save', { project: P(), path: folder + '/direct.eve' });
    ok('save writes to a known path', saved.path === folder + '/direct.eve');
    saved = await native('save', { project: P() });
    ok('save as adds the .eve extension', saved.path === folder + '/chosen.eve');
    ok(
      'save rejects non-projects',
      /Invalid project/.test(
        await fails(native('save', { project: { format: 'x' }, path: folder + '/x.eve' }))
      )
    );
    await native('autosave', { project: P() });
    let recovered = await native('recover');
    ok('autosave can be recovered', recovered.project.id === 'frontier-watch');
    let template = await native('template', {
      name: 'scout.eve-object',
      template: { sprite: 'x' },
    });
    ok('object templates save', /saved/.test(template.message));
    let png = await native('png', { name: 'pixel.png', data: 'iVBORw0KGgo=' });
    ok('PNG export saves', /PNG saved/.test(png.message));
    let play = await native('play', { project: P(), scene: 'grove' });
    ok('playtest launches the native runtime', /playtest launched/.test(play.message));
    let broken = M.clone(P());
    broken.start = 'missing-map';
    ok('playtest validates first', (await fails(native('play', { project: broken }))).length > 0);
    let exported = await native('export', { project: P() });
    ok('export builds a game folder', /Exported game/.test(exported.message));
    let cancelled = await native('open');
    ok('cancelled dialogs report cancellation', cancelled.cancelled === true);
    ok('unknown actions fail', /Unknown editor action/.test(await fails(native('dance'))));
    window.webkit.messageHandlers.studioTestDone.postMessage(out.join('\n') + '\nPASS');
  } catch (e) {
    window.webkit.messageHandlers.studioTestDone.postMessage(
      out.join('\n') + '\nFAIL ' + e.message
    );
  }
})();
