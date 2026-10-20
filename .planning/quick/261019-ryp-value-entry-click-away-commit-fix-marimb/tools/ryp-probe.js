// ryp click-away headless probe (quick 261019-ryp). Run from the repo root.
//
//   node .planning/quick/261019-ryp-value-entry-click-away-commit-fix-marimb/tools/ryp-probe.js \
//        --plugin <Name> --a <dial A css> --b <dial B css> --type <text A should take> \
//        [--ra <readout A css>] [--rb <readout B css>] [--pre <css to click first>]... \
//        [--a-offset dx,dy] [--b-offset dx,dy]
//
// Serves the plugin's real UI tree on the repo stub (scripts/serve-ui.js), then:
//   1. double-clicks dial A (centre + optional offset) -> a focused <input> must appear;
//   2. replaces its text with --type, WITHOUT pressing Enter;
//   3. presses the mouse on dial B -> the field must be gone (closed) and A's value
//      must have been written by that press (the commit);
//   4. drags up 40 px and releases -> B's value must move (B's drag still works).
// Evidence: slider-state diffs when the generic stub exposes window.__stubStates;
// otherwise --ra and --rb are required and readout text is compared.
// --pre clicks run first (e.g. switch to the tab the dials live on).
// Exit: 0 PASS, 1 FAIL, 2 usage, 77 Playwright unresolvable (NOT a pass).
'use strict';
const path = require('path');
const S = require(path.resolve('scripts/serve-ui.js'));

const argv = process.argv.slice(2);
const opt = { pre: [] };
for (let i = 0; i < argv.length; i++) {
    const k = argv[i].replace(/^--/, '');
    const v = argv[i + 1];
    if (k === 'pre') { opt.pre.push(v); i++; } else { opt[k] = v; i++; }
}
if (!opt.plugin || !opt.a || !opt.b || opt.type === undefined) {
    console.log('usage: ryp-probe.js --plugin <Name> --a <css> --b <css> --type <text> [--ra <css>] [--rb <css>] [--pre <css>]... [--a-offset dx,dy] [--b-offset dx,dy]');
    process.exit(2);
}
const offset = (s) => (s ? s.split(',').map(Number) : [0, 0]);
// Echo the exact invocation so a tee'd log is self-describing and re-runnable.
console.log(`ryp-probe args: ${argv.map((a) => (/^[\w.,:/=-]+$/.test(a) ? a : `'${a.replace(/'/g, `'\\''`)}'`)).join(' ')}`);

(async () => {
    const pw = S.resolvePlaywright();
    if (!pw) { console.log('PROBE-UNVERIFIED: Playwright unresolvable (exit 77) - NOT a pass'); process.exit(77); }
    const built = S.buildRoot(opt.plugin);
    const size = S.readEditorSize(opt.plugin) || { w: 1200, h: 800 };
    const srv = await S.serve(built.root);
    const browser = await pw.chromium.launch();
    const page = await (await browser.newContext({ viewport: { width: size.w, height: size.h } })).newPage();
    const pageErrors = [];
    page.on('pageerror', (e) => pageErrors.push(String(e && e.message ? e.message : e)));
    let failed = 0;
    const check = (ok, msg) => { console.log(`${ok ? 'PROBE-OK  ' : 'PROBE-FAIL'} ${msg}`); if (!ok) failed++; };
    const finish = async (code) => { await browser.close(); await srv.close(); process.exit(code); };

    try {
        await page.goto(`http://127.0.0.1:${srv.port}/index.html`, { waitUntil: 'load', timeout: 20000 });
        await page.waitForTimeout(1200);
        for (const sel of opt.pre) { await page.locator(sel).first().click(); await page.waitForTimeout(300); }

        const centre = async (sel, off) => {
            const loc = page.locator(sel).first();
            try { await loc.waitFor({ state: 'visible', timeout: 4000 }); }
            catch { throw new Error(`${sel} not found or not visible (wrong selector? on another tab - add --pre?)`); }
            await loc.scrollIntoViewIfNeeded();
            const b = await loc.boundingBox();
            if (!b) throw new Error(`no box for ${sel} (hidden? add a --pre click)`);
            return [b.x + b.width / 2 + off[0], b.y + b.height / 2 + off[1]];
        };
        const snap = () => page.evaluate(() => {
            const s = window.__stubStates && window.__stubStates.sliders;
            if (!s) return null;
            const o = {};
            for (const [k, st] of s) o[k] = st.getScaledValue();
            return o;
        });
        const text = (sel) => (sel ? page.evaluate((q) => { const e = document.querySelector(q); return e ? e.textContent.trim() : null; }, sel) : null);
        const diff = (x, y) => Object.keys(y).filter((k) => x[k] !== y[k]);

        const [ax, ay] = await centre(opt.a, offset(opt['a-offset']));
        const [bx, by] = await centre(opt.b, offset(opt['b-offset']));
        const s0 = await snap();
        if (!s0 && (!opt.ra || !opt.rb)) { console.log('PROBE-USAGE: this stub exposes no __stubStates - pass --ra and --rb'); return finish(2); }
        const ra0 = await text(opt.ra);

        await page.mouse.dblclick(ax, ay);
        await page.waitForTimeout(150);
        const opened = await page.evaluate(() => {
            const el = document.activeElement;
            if (!el || el.tagName !== 'INPUT') return false;
            window.__rypInput = el; el.select(); return true;
        });
        check(opened, `double-click on A (${opt.a}) opened a focused value-entry field`);
        if (!opened) return finish(1);
        await page.keyboard.type(String(opt.type));
        const s1 = await snap();
        if (s0 && diff(s0, s1).length) console.log(`NOTE written while typing (before any commit): ${diff(s0, s1).join(', ')}`);

        await page.mouse.move(bx, by);
        await page.mouse.down();
        await page.waitForTimeout(120);
        const closed = await page.evaluate(() => !window.__rypInput.isConnected);
        check(closed, `pointerdown on B (${opt.b}) closed the field on A`);
        const s2 = await snap();
        const ra2 = await text(opt.ra);
        const rb2 = await text(opt.rb);
        if (s0) {
            const committed = diff(s1, s2);
            check(committed.length >= 1, `the press on B committed A's typed value (changed: ${committed.join(', ') || 'nothing'})`);
        }
        if (opt.ra) check(ra2 !== ra0, `readout A changed on the press: "${ra0}" -> "${ra2}"`);

        for (let i = 1; i <= 8; i++) { await page.mouse.move(bx, by - i * 5); await page.waitForTimeout(16); }
        await page.mouse.up();
        await page.waitForTimeout(150);
        const s3 = await snap();
        const rb3 = await text(opt.rb);
        if (s0) {
            const dragged = diff(s2, s3);
            check(dragged.length >= 1, `dragging B still moves it (changed: ${dragged.join(', ') || 'nothing'})`);
        }
        if (opt.rb) check(rb3 !== rb2, `readout B changed during the drag: "${rb2}" -> "${rb3}"`);
        const stray = await page.evaluate(() => document.activeElement && document.activeElement.tagName === 'INPUT');
        check(!stray, 'no value-entry field left focused after the drag');
        check(pageErrors.length === 0, `page errors: ${pageErrors.length}${pageErrors.length ? ' - ' + pageErrors[0].slice(0, 160) : ''}`);
    } catch (e) {
        check(false, `probe threw: ${e.message}`);
    }
    console.log(`ryp-probe ${opt.plugin}: ${failed ? 'FAIL' : 'PASS'} (${failed} failure(s))`);
    return finish(failed ? 1 : 0);
})();
