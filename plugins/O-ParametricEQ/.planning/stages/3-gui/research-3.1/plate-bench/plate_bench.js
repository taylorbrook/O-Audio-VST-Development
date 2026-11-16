const path = require('path');
const S = require('/Users/taylorbrook/Dev/VST-development/scripts/serve-ui.js');
const { chromium } = S.resolvePlaywright();
const URL = 'file:///Users/taylorbrook/Dev/VST-development/plugins/O-ParametricEQ/.planning/mockups/v1-ui-test.html';
const metric = async (cdp) => Object.fromEntries((await cdp.send('Performance.getMetrics')).metrics.map(m => [m.name, m.value]));
(async () => {
  const browser = await chromium.launch();
  const out = [];
  for (const plate of [true, false, true, false]) {
    const page = await browser.newPage({ viewport: { width: 1600, height: 1000 }, deviceScaleFactor: 2 });
    await page.goto(URL); await page.waitForTimeout(800);
    if (!plate) await page.addStyleTag({ content: '#plate{display:none!important}' });
    const stats = await page.evaluate(() => { const p = document.getElementById('plate'); const ds = [...p.querySelectorAll('path')].map(e => e.getAttribute('d') || '');
      return { elems: p.querySelectorAll('*').length, dChars: ds.reduce((a, d) => a + d.length, 0), cmds: ds.reduce((a, d) => a + (d.match(/[MLQCZ]/g) || []).length, 0) }; });
    const cdp = await page.context().newCDPSession(page); await cdp.send('Performance.enable');
    await page.waitForTimeout(300);
    let a = await metric(cdp); await page.waitForTimeout(5000); let b = await metric(cdp);
    const idle = { task: (b.TaskDuration - a.TaskDuration) * 1000 / 5, script: (b.ScriptDuration - a.ScriptDuration) * 1000 / 5 };
    a = await metric(cdp);
    const t0 = Date.now();
    for (let i = 0; i < 20; i++) { await page.setViewportSize(i % 2 ? { width: 1600, height: 1000 } : { width: 800, height: 500 }); await page.evaluate(() => new Promise(r => requestAnimationFrame(() => requestAnimationFrame(r)))); }
    b = await metric(cdp);
    out.push({ plate, stats, idle_ms_per_s: idle, resize20_task_ms: (b.TaskDuration - a.TaskDuration) * 1000, resize20_layout_ms: (b.LayoutDuration - a.LayoutDuration) * 1000, resize20_wall_ms: Date.now() - t0 });
    await page.close();
  }
  console.log(JSON.stringify(out, null, 1));
  await browser.close();
})();
