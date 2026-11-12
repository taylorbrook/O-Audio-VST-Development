const S = require('/Users/taylorbrook/Dev/VST-development/scripts/serve-ui.js');
(async () => {
  const pw = S.resolvePlaywright();
  const built = S.buildRoot('O-GranulatorInstrument');
  const srv = await S.serve(built.root, () => {});
  const browser = await pw.chromium.launch();
  const page = await browser.newPage({ viewport: { width: 1200, height: 800 } });
  await page.goto(`http://127.0.0.1:${srv.port}/`);
  await page.waitForFunction(() => { const v = document.getElementById('val-polyphony'); return v && v.textContent !== '—'; }, null, { timeout: 15000 });
  await page.evaluate(() => document.fonts.ready);
  await page.click('#tab-mod'); await page.waitForTimeout(200);
  const i18n = await import('./i18n.mjs');
  const newKeys = ['feedback','fb_low_cut','fb_high_cut','fb_saturation','fb_diffusion','reverb_mix','reverb_size','reverb_damp'];
  for (const lang of ['en','fr','zh-Hans']) {
    await page.evaluate((l) => window.__setLanguage(l), lang); await page.waitForTimeout(150);
    const cands = newKeys.map(k => 'IV · ' + i18n.tr(k, lang).t);
    const r = await page.evaluate((cands) => {
      const sel = document.getElementById('combo-mod1_dest'); const cs = getComputedStyle(sel); const r = sel.getBoundingClientRect();
      const c = document.createElement('canvas').getContext('2d'); c.font = `${cs.fontStyle} ${cs.fontWeight} ${cs.fontSize} ${cs.fontFamily}`;
      const opts = [...sel.options].map((o, i) => [i, o.textContent, +c.measureText(o.textContent).width.toFixed(1)]);
      opts.sort((a, b) => b[2] - a[2]);
      const pad = parseFloat(cs.paddingLeft) + parseFloat(cs.paddingRight) + parseFloat(cs.borderLeftWidth) + parseFloat(cs.borderRightWidth);
      return { selW: +r.width.toFixed(1), inner: +(r.width - pad).toFixed(1), font: c.font.slice(0, 40), n: sel.options.length, widest: opts.slice(0, 5), cands: cands.map(t => [t, +c.measureText(t).width.toFixed(1)]) };
    }, cands);
    console.log(lang, JSON.stringify(r));
  }
  // toggle/latch/pill vocabulary
  await page.evaluate(() => window.__setLanguage('en'));
  const voc = await page.evaluate(() => [...document.querySelectorAll('.tog, .gbtn, .pill, .seg, .icon-btn')].filter(e => e.getBoundingClientRect().width > 0 || true).map(e => { const r = e.getBoundingClientRect(); return `${e.id} [${e.getAttribute('class')}] ${r.width.toFixed(0)}x${r.height.toFixed(0)}`; }).slice(0, 60));
  console.log(voc.join('\n'));
  await browser.close(); await srv.close(); require('fs').rmSync(built.root, { recursive: true, force: true });
})().catch(e => { console.error(e); process.exit(70); });
