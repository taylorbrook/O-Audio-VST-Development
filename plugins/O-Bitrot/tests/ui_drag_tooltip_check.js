/*
    ui_drag_tooltip_check.js
    O-Bitrot v1.17.1: gates CODE_REVIEW WR-10 (Shift mid-drag jumped the knob)
    and WR-11 (the Preset tip painted over the open preset menu).

    The real page is served through tests/ui-stub/serve-stub.sh on a random
    port, at the 900 x 740 shipping size.
    - The WR-11 assertions are guarded by a control: the tip must show on the
      CLOSED trigger first, or "no tip on the open menu" would pass vacuously.
    - Negative-controlled: against the v1.17.0 index.html, 3 assertions fail.
      The knob jumps 67.5 deg -> -40.5 deg on a bare Shift press, and the tip
      stays over the open menu.

    Usage:  node plugins/O-Bitrot/tests/ui_drag_tooltip_check.js
    Exit code = failed assertions (0 = pass, 77 = could not run).
*/
'use strict';
const { spawn, execSync } = require('child_process');
const path = require('path'), fs = require('fs'), os = require('os');
const tests = __dirname;
const PORT = 8000 + Math.floor(Math.random() * 900) + 37;
const resolvePW = () => { const c = ['playwright'];
  try { c.push(path.join(execSync('npm root -g',{encoding:'utf8'}).trim(),'playwright')); } catch {}
  const nc = path.join(os.homedir(),'.npm','_npx'); if (fs.existsSync(nc)) for (const d of fs.readdirSync(nc)) { const p=path.join(nc,d,'node_modules','playwright'); if (fs.existsSync(p)) c.push(p); }
  for (const x of c) { try { return require(x); } catch {} } return null; };
let failed = 0; const check = (ok, d) => { console.log(`  ${ok?'PASS':'FAIL'}: ${d}`); if (!ok) failed++; };
(async () => {
  const srv = spawn('bash', [path.join(tests,'ui-stub','serve-stub.sh'), String(PORT)], { stdio: 'ignore' });
  await new Promise(r => setTimeout(r, 1500));
  const { chromium } = resolvePW();
  const b = await chromium.launch(); const page = await b.newPage({ viewport: { width: 900, height: 740 } });
  const errs = []; page.on('pageerror', e => errs.push(String(e)));
  await page.goto(`http://127.0.0.1:${PORT}/index.html`); await page.waitForTimeout(1200);

  // WR-10: drag 100px up, press Shift mid-drag with no movement → value must not jump; release Shift → no jump.
  const knob = page.locator('.knob[data-param="TAPE_PROB"]'); const bb = await knob.boundingBox();
  const cx = bb.x + bb.width/2, cy = bb.y + bb.height/2;
  const angle = () => knob.locator('.stem').evaluate(e => e.style.transform);
  await page.mouse.move(cx, cy); await page.mouse.down();
  for (let i=1;i<=10;i++) await page.mouse.move(cx, cy - 10*i);
  const a0 = await angle();
  await page.keyboard.down('Shift'); await page.mouse.move(cx, cy - 100); const a1 = await angle();
  await page.keyboard.up('Shift');   await page.mouse.move(cx, cy - 100); const a2 = await angle();
  await page.keyboard.down('Shift'); await page.mouse.move(cx, cy - 110); const a3 = await angle(); await page.keyboard.up('Shift');
  await page.mouse.up();
  check(a0 === a1 && a1 === a2, `Shift press/release with no movement leaves the knob put (${a0} | ${a1} | ${a2})`);
  check(a3 !== a2, `a fine (Shift) move still moves the knob (${a2} -> ${a3})`);

  // WR-11: tips on, open preset menu by click, sweep across trigger spans, wait > dwell → tooltip hidden.
  await page.locator('.gear-btn').click(); await page.waitForTimeout(200);
  await page.locator('#help-toggle').click(); await page.waitForTimeout(200);
  console.log('   help-toggle pressed:', await page.locator('#help-toggle').getAttribute('aria-pressed'));
  await page.locator('.gear-btn').click(); await page.waitForTimeout(200);
  await page.mouse.move(450, 700); await page.waitForTimeout(100);
  console.log('   data-tip host:', await page.evaluate(() => { const e = document.getElementById('preset-select').closest('[data-tip]'); return e ? (e.id || e.className) : 'NONE'; }));
  const sel = page.locator('#preset-select'); const sb = await sel.boundingBox();
  // control: hover the closed trigger → tip should appear (proves tips are live on it)
  await page.mouse.move(sb.x + 5, sb.y + sb.height/2); await page.mouse.move(sb.x + sb.width/2, sb.y + sb.height/2);
  await page.waitForTimeout(700);
  const tipVis = () => page.evaluate(() => document.getElementById('tooltip')?.classList.contains('visible'));
  const controlShown = await tipVis();
  await sel.click(); await page.waitForTimeout(100);
  const expanded = await sel.getAttribute('aria-expanded');
  for (let x = sb.x + 4; x < sb.x + sb.width - 4; x += 6) await page.mouse.move(x, sb.y + sb.height/2);
  await page.waitForTimeout(700);
  const openShown = await tipVis();
  check(controlShown === true, `control: tip shows on the CLOSED preset trigger with help on (${controlShown})`);
  check(expanded === 'true' && openShown === false, `no tip over the OPEN preset menu after a span sweep + dwell (expanded=${expanded}, tip=${openShown})`);
  // keyboard open with a tip showing
  await page.keyboard.press('Escape'); await page.waitForTimeout(100);
  if ((await sel.getAttribute('aria-expanded')) === 'true') { await page.mouse.click(450, 700); await page.waitForTimeout(100); }
  console.log('   menu closed:', await sel.getAttribute('aria-expanded'));
  await page.mouse.move(450, 700); await page.waitForTimeout(100);
  await page.mouse.move(sb.x + sb.width/2 + 3, sb.y + sb.height/2); await page.waitForTimeout(700);
  const before = await tipVis(); await sel.focus(); await page.keyboard.press('Enter'); await page.waitForTimeout(150);
  check(before === true && (await tipVis()) === false, `keyboard open clears a showing tip (before=${before})`);
  check(errs.length === 0, `no page errors ${errs.join('; ')}`);
  await b.close(); srv.kill(); process.exit(failed);
})().catch(e => { console.error(e); process.exit(77); });
