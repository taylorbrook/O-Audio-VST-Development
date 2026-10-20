// ryp gap-2 behaviour probe: O-Marimba Effects-tab knobs from modules/effects/* (quick 261019-ryp).
// Run from the repo root:
//   node .planning/quick/261019-ryp-value-entry-click-away-commit-fix-marimb/tools/ryp-unit-probe.js
//
// Serves O-Marimba's real UI tree (the three unit JS files are placed where
// PluginEditor::getResource() serves them) on the generic stub, seeded with the
// module C++ ranges and defaults (O-Marimba has no param dump, so without the
// seed every knob would be a neutral 0..1). Every slider state's
// sliderDragStarted / setNormalisedValue / sliderDragEnded is wrapped to a log.
//
// Per control (reverb Size + Pre-dly, compressor Thresh, EQ LF outer ring = freq,
// EQ LF inner dial = gain) it asserts:
//   A  double-click opens a focused <input> and writes NOTHING (no reset);
//   B  typed text + Enter lands on the expected C++ value, logged as exactly start/set/end;
//   C  a write from outside while the field is open (automation) leaves the field open;
//   D  Esc closes the field and writes nothing;
//   E  untouched Enter closes the field and writes nothing;
//   F  Alt-press + 30 px drag + release lands on the C++ default as exactly start/set/end
//      and the drag does not move it afterwards; no field opens.
// Click-away (grab another dial while a field is open) is covered by tools/ryp-probe.js.
// Exit: 0 PASS, 1 FAIL, 77 Playwright unresolvable (NOT a pass).
'use strict';
const path = require('path');
const S = require(path.resolve('scripts/serve-ui.js'));

// Mirrors CompressorUnit::addParameters / ReverbUnit::addParameters / AnalogEQUnit (lf band).
const OVERRIDES = { sliders: {
    fx_rev_size:     { start: 0,   end: 1,   skew: 1,   interval: 0,   def: 0.5 },
    fx_rev_damp:     { start: 0,   end: 1,   skew: 1,   interval: 0,   def: 0.5 },
    fx_rev_predelay: { start: 0,   end: 200, skew: 1,   interval: 0,   def: 20 },
    fx_rev_mix:      { start: 0,   end: 1,   skew: 1,   interval: 0,   def: 0.25 },
    fx_rev_mod:      { start: 0,   end: 1,   skew: 1,   interval: 0,   def: 0.2 },
    fx_rev_shimmer:  { start: 0,   end: 1,   skew: 1,   interval: 0,   def: 0 },
    fx_comp_threshold: { start: -60, end: 0,    skew: 1, interval: 0.1, def: -20 },
    fx_comp_ratio:     { start: 1,   end: 20,   skew: 1, interval: 0.1, def: 2 },
    fx_comp_attack:    { start: 0.1, end: 100,  skew: 1, interval: 0.1, def: 10 },
    fx_comp_release:   { start: 10,  end: 1000, skew: 1, interval: 1,   def: 100 },
    fx_eq_lf_freq:  { start: 30,  end: 500,   skew: 0.3, interval: 0.1, def: 100 },
    fx_eq_lf_gain:  { start: -12, end: 12,    skew: 1,   interval: 0.1, def: 0 },
    fx_eq_lmf_freq: { start: 100, end: 2000,  skew: 0.3, interval: 0.1, def: 500 },
    fx_eq_lmf_gain: { start: -12, end: 12,    skew: 1,   interval: 0.1, def: 0 },
    fx_eq_hmf_freq: { start: 500, end: 8000,  skew: 0.3, interval: 0.1, def: 2000 },
    fx_eq_hmf_gain: { start: -12, end: 12,    skew: 1,   interval: 0.1, def: 0 },
    fx_eq_hf_freq:  { start: 2000, end: 20000, skew: 0.3, interval: 0.1, def: 8000 },
    fx_eq_hf_gain:  { start: -12, end: 12,    skew: 1,   interval: 0.1, def: 0 },
},
// The compressor and EQ strips set pointer-events: none on their knobs while
// bypassed (by design), so both are switched on; the reverb stays at its
// shipped OFF default (its knobs stay live when bypassed).
toggles: { fx_comp_enabled: true, fx_eq_enabled: true } };

// sel = element double-clicked / Alt-pressed; rx = horizontal offset as a fraction
// of the element width (EQ outer ring needs a press outside the 60 % inner radius).
const CASES = [
    { id: 'fx_rev_size',       sel: '#fx_rev_size_knob',       rx: 0,    type: '30',     expect: 0.30, def: 0.5,  tol: 1e-6 },
    { id: 'fx_rev_predelay',   sel: '#fx_rev_predelay_knob',   rx: 0,    type: '50',     expect: 50,   def: 20,   tol: 1e-4 },
    { id: 'fx_comp_threshold', sel: '#fx_comp_threshold_knob', rx: 0,    type: '-12',    expect: -12,  def: -20,  tol: 1e-4 },
    { id: 'fx_eq_lf_freq',     sel: '#eq-panel [data-param-outer="lf_freq"]', rx: 0.42, type: '200', expect: 200, def: 100, tol: 0.6 },
    { id: 'fx_eq_lf_gain',     sel: '#eq-panel [data-param-outer="lf_freq"]', rx: 0,    type: '3',   expect: 3,   def: 0,   tol: 1e-4 },
];

(async () => {
    const pw = S.resolvePlaywright();
    if (!pw) { console.log('PROBE-UNVERIFIED: Playwright unresolvable (exit 77) - NOT a pass'); process.exit(77); }
    const built = S.buildRoot('O-Marimba');
    const size = S.readEditorSize('O-Marimba') || { w: 1200, h: 800 };
    const srv = await S.serve(built.root);
    const browser = await pw.chromium.launch();
    const ctx = await browser.newContext({ viewport: { width: size.w, height: size.h } });
    // serve-ui's preamble ASSIGNS window.__stubOverrides (its own seed) after
    // init scripts run, so a plain assignment here would be overwritten. A
    // setter merges whatever the preamble writes UNDER these values instead.
    await ctx.addInitScript((o) => {
        let cur = o;
        Object.defineProperty(window, '__stubOverrides', {
            configurable: true,
            get() { return cur; },
            set(v) {
                const m = { ...(v || {}) };
                for (const k of Object.keys(o)) m[k] = { ...((v || {})[k] || {}), ...o[k] };
                cur = m;
            },
        });
    }, OVERRIDES);
    const page = await ctx.newPage();
    const pageErrors = [];
    page.on('pageerror', (e) => pageErrors.push(String(e && e.message ? e.message : e)));
    let failed = 0;
    const check = (ok, msg) => { console.log(`${ok ? 'PROBE-OK  ' : 'PROBE-FAIL'} ${msg}`); if (!ok) failed++; };

    try {
        await page.goto(`http://127.0.0.1:${srv.port}/index.html`, { waitUntil: 'load', timeout: 20000 });
        await page.waitForTimeout(1200);
        await page.locator('#tab-effects').click();
        await page.waitForTimeout(300);
        await page.evaluate(() => {
            window.__rypLog = [];
            for (const [k, st] of window.__stubStates.sliders) {
                for (const m of ['sliderDragStarted', 'setNormalisedValue', 'sliderDragEnded']) {
                    const orig = st[m].bind(st);
                    st[m] = (...a) => { window.__rypLog.push(`${k}:${m === 'setNormalisedValue' ? 'set' : m === 'sliderDragStarted' ? 'start' : 'end'}`); return orig(...a); };
                }
            }
        });
        const val = (id) => page.evaluate((k) => window.__stubStates.sliders.get(k).getScaledValue(), id);
        const log = (id) => page.evaluate((k) => window.__rypLog.filter((e) => e.startsWith(k + ':')).map((e) => e.slice(k.length + 1)), id);
        const clear = () => page.evaluate(() => { window.__rypLog.length = 0; });
        const focusedInput = () => page.evaluate(() => { const a = document.activeElement; if (a && a.tagName === 'INPUT') { window.__rypInput = a; return true; } return false; });
        const inputGone = () => page.evaluate(() => !window.__rypInput || !window.__rypInput.isConnected);
        const point = async (c) => {
            const loc = page.locator(c.sel).first();
            await loc.scrollIntoViewIfNeeded();
            const b = await loc.boundingBox();
            if (!b) throw new Error(`no box for ${c.sel}`);
            return [b.x + b.width / 2 + c.rx * b.width, b.y + b.height / 2];
        };
        const open = async (c) => {
            const [x, y] = await point(c);
            await clear();
            await page.mouse.dblclick(x, y);
            await page.waitForTimeout(120);
            return focusedInput();
        };

        for (const c of CASES) {
            // A: dblclick opens entry, writes nothing
            const before = await val(c.id);
            const opened = await open(c);
            const lA = await log(c.id);
            check(opened, `${c.id} A: double-click opens a focused value-entry field`);
            check(!lA.includes('set') && (await val(c.id)) === before, `${c.id} A: double-click writes nothing (log: ${lA.join('/') || '-'})`);
            if (!opened) continue;

            // B: typed + Enter -> expected value, one gesture
            await clear();
            await page.evaluate(() => window.__rypInput.select());
            await page.keyboard.type(c.type);
            await page.keyboard.press('Enter');
            await page.waitForTimeout(80);
            const vB = await val(c.id), lB = await log(c.id);
            check(Math.abs(vB - c.expect) <= c.tol, `${c.id} B: typed "${c.type}" + Enter -> ${vB} (expect ${c.expect})`);
            check(lB.join('/') === 'start/set/end', `${c.id} B: commit is one gesture (log: ${lB.join('/') || '-'})`);
            check(await inputGone(), `${c.id} B: field closed after Enter`);

            // C: automation write while open keeps the field
            if (await open(c)) {
                await page.evaluate((k) => window.__stubStates.sliders.get(k).setNormalisedValue(0.9), c.id);
                await page.waitForTimeout(60);
                check(!(await inputGone()), `${c.id} C: a host write while the field is open does not wipe it`);
                // D: Esc writes nothing
                await clear();
                await page.keyboard.press('Escape');
                await page.waitForTimeout(60);
                const lD = await log(c.id);
                check((await inputGone()) && !lD.includes('set'), `${c.id} D: Esc closes and writes nothing (log: ${lD.join('/') || '-'})`);
            } else check(false, `${c.id} C/D: could not reopen the field`);

            // E: untouched Enter writes nothing
            if (await open(c)) {
                await clear();
                await page.keyboard.press('Enter');
                await page.waitForTimeout(60);
                const lE = await log(c.id);
                check((await inputGone()) && !lE.includes('set'), `${c.id} E: untouched Enter writes nothing (log: ${lE.join('/') || '-'})`);
            } else check(false, `${c.id} E: could not reopen the field`);

            // F: Alt-press resets to the default in one gesture, no drag, no field
            const [x, y] = await point(c);
            await clear();
            await page.keyboard.down('Alt');
            await page.mouse.move(x, y);
            await page.mouse.down();
            for (let i = 1; i <= 6; i++) { await page.mouse.move(x, y - i * 5); await page.waitForTimeout(16); }
            await page.mouse.up();
            await page.keyboard.up('Alt');
            await page.waitForTimeout(80);
            const vF = await val(c.id), lF = await log(c.id);
            check(Math.abs(vF - c.def) <= c.tol, `${c.id} F: Alt-click -> ${vF} (default ${c.def})`);
            check(lF.join('/') === 'start/set/end', `${c.id} F: reset is one gesture and the drag never started (log: ${lF.join('/') || '-'})`);
            check(!(await focusedInput()), `${c.id} F: Alt-click opens no field`);
        }
        check(pageErrors.length === 0, `page errors: ${pageErrors.length}${pageErrors.length ? ' - ' + pageErrors[0].slice(0, 160) : ''}`);
    } catch (e) {
        check(false, `probe threw: ${e.message}`);
    }
    console.log(`ryp-unit-probe O-Marimba effects: ${failed ? 'FAIL' : 'PASS'} (${failed} failure(s))`);
    await browser.close(); await srv.close();
    process.exit(failed ? 1 : 0);
})();
