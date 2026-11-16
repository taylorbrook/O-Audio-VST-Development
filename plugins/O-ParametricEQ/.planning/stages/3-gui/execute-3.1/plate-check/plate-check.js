// P4 check: #plate bounding box in the served index.html vs the mockup test page
// at 960x600, 800x500, 1600x1000 (tolerance 1 px), plus idle task cost shown vs hidden.
const S = require('/Users/taylorbrook/Dev/VST-development/scripts/serve-ui.js');
const { chromium } = S.resolvePlaywright();
const TEST = 'file:///Users/taylorbrook/Dev/VST-development/plugins/O-ParametricEQ/.planning/mockups/v1-ui-test.html';
const sizes = [[960, 600], [800, 500], [1600, 1000]];
const metric = async (cdp) => Object.fromEntries((await cdp.send('Performance.getMetrics')).metrics.map(m => [m.name, m.value]));
(async () => {
  const b = S.buildRoot('O-ParametricEQ');
  const srv = await S.serve(b.root);
  const REAL = `http://127.0.0.1:${srv.port}/`;
  const browser = await chromium.launch();
  const box = async (url, w, h, force) => {
    const p = await browser.newPage({ viewport: { width: w, height: h } });
    const errs = []; p.on('pageerror', e => errs.push(String(e)));
    await p.goto(url); await p.waitForTimeout(900);
    if (force) { await p.addStyleTag({ content: '#display{width:' + force[0] + 'px!important;height:' + force[1] + 'px!important;flex:none!important;max-width:none!important;max-height:none!important}' }); await p.waitForTimeout(300); }
    const r = await p.evaluate(() => {
      const el = document.getElementById('plate'); const bb = el.getBoundingClientRect();
      const D = el.parentElement.getBoundingClientRect(); const g = el.querySelector('g'); const gb = g ? g.getBoundingClientRect() : null;
      const cs = getComputedStyle(el);
      return { parent: el.parentElement.id, disp: [D.width, D.height].map(v => +v.toFixed(2)), svg: [bb.x - D.x, bb.y - D.y, bb.width, bb.height].map(v => +v.toFixed(2)),
               ink: gb && [gb.x - D.x, gb.y - D.y, gb.width, gb.height].map(v => +v.toFixed(2)),
               opacity: cs.opacity, transform: cs.transform, pe: cs.pointerEvents, paths: el.querySelectorAll('path').length };
    });
    await p.close(); return { ...r, errs };
  };
  let worst = 0; const rows = [];
  for (const [w, h] of sizes) {
    const r = await box(REAL, w, h); const t = await box(TEST, 2400, 1600, r.disp);
    const d = Math.max(...t.svg.map((v, i) => Math.abs(v - r.svg[i])), ...(t.ink && r.ink ? t.ink.map((v, i) => Math.abs(v - r.ink[i])) : [99]));
    worst = Math.max(worst, d);
    rows.push({ size: `${w}x${h}`, test: t, real: r, maxDiffPx: +d.toFixed(3) });
  }
  const cost = [];
  for (const shown of [true, false, true, false]) {
    const p = await browser.newPage({ viewport: { width: 960, height: 600 } });
    await p.goto(REAL); await p.waitForTimeout(800);
    if (!shown) await p.addStyleTag({ content: '#plate{display:none!important}' });
    const cdp = await p.context().newCDPSession(p); await cdp.send('Performance.enable');
    await p.waitForTimeout(300);
    const a = await metric(cdp); await p.waitForTimeout(5000); const z = await metric(cdp);
    cost.push({ shown, idleTaskMsPerS: +((z.TaskDuration - a.TaskDuration) * 200).toFixed(2) });
    await p.close();
  }
  console.log(JSON.stringify({ rows, worstPx: worst, verdict: worst <= 1 ? 'PASS' : 'FAIL', cost }, null, 1));
  await browser.close(); await srv.close();
})();
