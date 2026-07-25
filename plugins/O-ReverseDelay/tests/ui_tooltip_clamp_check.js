/*
  ==============================================================================

    ui_tooltip_clamp_check.js
    O-ReverseDelay — tooltip edge-clamp verification AT THE SHIPPING VIEWPORT.

    Why this file exists separately from ui_frontend_check.js:

    ui_frontend_check.js is STATIC. It can prove that showTooltip() releases,
    measures and PINS its width before applying `left`, and that `left` is
    clamped into [MARGIN, innerWidth - width - MARGIN] — but it cannot prove the
    clamp actually fires, because whether it fires depends entirely on the
    viewport width. At a default 1280-wide browser there is room to the right of
    every control, the clamp never engages, and an assertion that "the tooltip
    fits" passes while the real 940 px window would overflow
    (pattern_tooltip_clamp_gate_viewport_sensitive).

    So this drives the REAL page — same HTML, same CSS, same app.js, with only
    js/juce/index.js swapped for the stub — in a browser resized to the exact
    shipping size, and measures the rendered tooltip rectangle.

    v1.1.0 resized the editor 940x484 -> 940x743. The width did not change, but
    the tooltip layer is position:fixed and its vertical flip (above -> below)
    is a function of viewport HEIGHT, and the new RANDOM panel put four controls
    in a second row where the "prefer above" placement has different clearance.
    The v1.0.1 C5 verification is therefore invalid and this re-measures it.

    What is asserted, for EVERY control carrying a tooltip:
      1. The tip is at its natural width, not shrink-wrapped — a fixed-position
         box with `left` set and `width:auto` collapses to the space remaining
         to its right, turning a 230 px tip into a ~70 px ribbon
         (pattern_fixed_tooltip_shrink_to_fit_edge).
      2. left >= MARGIN  AND  right <= innerWidth - MARGIN. Asserting width
         alone is what let the O-MBC bug through; both edges must be checked.
      3. top >= 0 and bottom <= innerHeight — the flip actually keeps it on
         screen at the new height.
      4. The arrow still points inside the tip after clamping.

    The dwell delay is AWAITED rather than slept past by a fixed guess: the tip
    only renders after TOOLTIP_DELAY_MS, and polling for the .visible class is
    what keeps this from silently measuring an invisible element.

    Usage:  node plugins/O-ReverseDelay/tests/ui_tooltip_clamp_check.js
    Exit code = number of failed assertions (0 = all pass).
    Requires Playwright (`npx playwright install chromium` once).

  ==============================================================================
*/

'use strict';

const fs   = require('fs');
const os   = require('os');
const path = require('path');
const http = require('http');

const pluginRoot = path.resolve(__dirname, '..');
const repoRoot   = path.resolve(pluginRoot, '..', '..');
const publicDir  = path.join(pluginRoot, 'Source', 'ui', 'public');

// Kept in sync with PluginEditor.cpp's setSize and styles.css.
const SHIP_W = 940;
const SHIP_H = 743;

// app.js constants — mirrored here, and cross-checked against the source below
// so this file cannot drift from the page it is measuring.
const TOOLTIP_MARGIN = 8;
const NATURAL_MAX_W  = 230;   // .tooltip max-width

const MIME = {
    '.html': 'text/html; charset=utf-8',
    '.css':  'text/css; charset=utf-8',
    '.js':   'application/javascript; charset=utf-8',
    '.png':  'image/png',
};

let failed = 0;
function check(cond, desc) {
    console.log(`  ${cond ? 'PASS' : 'FAIL'}: ${desc}`);
    if (!cond) ++failed;
}

// ── Build the served tree: production page + the bridge stub ────────────────
// Byte-identical to Source/ui/public except js/juce/index.js, exactly as
// tests/ui-stub/serve-stub.sh does it — what renders here is what renders in
// the WebView.
function buildRoot() {
    const root = fs.mkdtempSync(path.join(os.tmpdir(), 'orvd-tip-'));
    fs.cpSync(publicDir, root, { recursive: true });
    fs.copyFileSync(path.join(pluginRoot, 'tests', 'ui-stub', 'juce-stub.js'),
                    path.join(root, 'js', 'juce', 'index.js'));
    // Not under Source/ui/public — CMake embeds it from the module tree and
    // getResource() serves it at /js/preset-manager.js.
    fs.copyFileSync(
        path.join(repoRoot, 'modules', 'persistence', 'preset-manager', 'js', 'preset-manager.js'),
        path.join(root, 'js', 'preset-manager.js'));
    return root;
}

function serve(root) {
    const server = http.createServer((req, res) => {
        const rel = decodeURIComponent(req.url.split('?')[0]);
        const file = path.join(root, rel === '/' ? 'index.html' : rel);
        if (!file.startsWith(root) || !fs.existsSync(file) || fs.statSync(file).isDirectory()) {
            res.writeHead(404); res.end('not found'); return;
        }
        res.writeHead(200, { 'Content-Type': MIME[path.extname(file)] || 'application/octet-stream' });
        fs.createReadStream(file).pipe(res);
    });
    return new Promise(resolve => server.listen(0, '127.0.0.1',
        () => resolve({ server, port: server.address().port })));
}

(async () => {
    console.log('== O-ReverseDelay ui_tooltip_clamp_check ==');
    console.log(`   viewport pinned to the SHIPPING size ${SHIP_W} x ${SHIP_H}\n`);

    // Guard against this file's mirrored constants drifting from the page.
    const appJs = fs.readFileSync(path.join(publicDir, 'js', 'app.js'), 'utf8');
    const css   = fs.readFileSync(path.join(publicDir, 'css', 'styles.css'), 'utf8');
    const editorCpp = fs.readFileSync(path.join(pluginRoot, 'Source', 'PluginEditor.cpp'), 'utf8');

    check(new RegExp(`TOOLTIP_MARGIN\\s*=\\s*${TOOLTIP_MARGIN}\\b`).test(appJs),
        `TOOLTIP_MARGIN in app.js is ${TOOLTIP_MARGIN} (this file mirrors it)`);
    check(new RegExp(`max-width:\\s*${NATURAL_MAX_W}px`).test(css),
        `.tooltip max-width is ${NATURAL_MAX_W}px (this file mirrors it)`);
    check(new RegExp(`setSize\\s*\\(\\s*${SHIP_W}\\s*,\\s*${SHIP_H}\\s*\\)`).test(editorCpp),
        `editor setSize is ${SHIP_W} x ${SHIP_H} — the viewport measured below`);

    // Resolve playwright from wherever it already lives: a local/global install,
    // or npx's cache (this repo carries no package.json, and `npx playwright`
    // leaves a usable copy under ~/.npm/_npx). Deliberately does NOT install
    // anything — a verification script that mutates the machine to make itself
    // pass is not a verification script.
    const resolvePlaywright = () => {
        const { execSync } = require('child_process');
        const candidates = ['playwright'];

        try { candidates.push(path.join(execSync('npm root -g', { encoding: 'utf8' }).trim(), 'playwright')); }
        catch { /* npm not on PATH — the other candidates still stand */ }

        const npxCache = path.join(os.homedir(), '.npm', '_npx');
        if (fs.existsSync(npxCache)) {
            for (const dir of fs.readdirSync(npxCache)) {
                const p = path.join(npxCache, dir, 'node_modules', 'playwright');
                if (fs.existsSync(p)) candidates.push(p);
            }
        }

        for (const c of candidates) {
            try { return require(c); } catch { /* next */ }
        }
        return null;
    };

    const pw = resolvePlaywright();
    if (pw == null) {
        console.log('\n  SKIP: playwright not resolvable. Install with');
        console.log('        npx playwright install chromium');
        console.log('  The tooltip edge clamp is NOT verified without it.');
        process.exit(77);   // distinct from 0: "could not verify" is not "passed"
    }
    const { chromium } = pw;

    const root = buildRoot();
    const { server, port } = await serve(root);
    const browser = await chromium.launch();
    // `viewport`, NOT `viewportSize` — the latter is the getter's name and is
    // silently IGNORED as a launch option, leaving Chromium's 1280x720 default.
    // That is not a cosmetic slip: at 1280 the `mix` tooltip lands at x=744..974
    // with the clamp never engaging, so every assertion passes while the real
    // 940 px window overflows by 42 px. This exact mistake is the failure the
    // whole file exists to catch (pattern_tooltip_clamp_gate_viewport_sensitive),
    // and the viewport assertion below is what makes it loud instead of silent.
    const page = await browser.newPage({ viewport: { width: SHIP_W, height: SHIP_H } });

    const consoleErrors = [];
    page.on('console', m => { if (m.type() === 'error') consoleErrors.push(m.text()); });
    page.on('pageerror', e => consoleErrors.push(String(e)));

    await page.goto(`http://127.0.0.1:${port}/index.html`, { waitUntil: 'networkidle' });

    // The page must actually be alive: a TDZ throw out of module evaluation
    // kills every control while leaving the HTML looking correct
    // (pattern_module_toplevel_init_tdz). Readouts are written by JS, so a
    // populated readout is proof the module ran.
    const boundReadouts = await page.$$eval('.knob-value',
        els => els.filter(e => e.textContent.trim() !== '—' && e.textContent.trim() !== '').length);
    // v1.3.0: 14 knobs (grainCount joined). Compared with === rather than >=:
    // the loose form let this line print "14/13" once grainCount landed, and a
    // bound that can never fail upward would also never notice a knob going
    // missing while another was added.
    check(boundReadouts === 14,
        `app.js ran and bound the knobs — ${boundReadouts}/14 readouts populated`);
    check(consoleErrors.length === 0,
        'no console errors on load' + (consoleErrors.length ? ` — ${consoleErrors[0]}` : ''));

    // Real viewport, not a guess.
    const vp = await page.evaluate(() => ({ w: window.innerWidth, h: window.innerHeight }));
    check(vp.w === SHIP_W && vp.h === SHIP_H,
        `viewport really is ${SHIP_W} x ${SHIP_H} — got ${vp.w} x ${vp.h}`);

    // Every [data-tip] element must carry an id, because this enumeration is
    // BY id — an anchor without one is silently skipped and its tip is never
    // measured. v1.3.0's grain-meter tip was added without an id and vanished
    // from this run without changing the 17/17 tally, which is exactly the kind
    // of coverage hole a filter(Boolean) hides.
    const idless = await page.$$eval('[data-tip]',
        els => els.filter(e => !e.id).map(e => e.className || e.tagName));
    check(idless.length === 0,
        'every [data-tip] anchor carries an id, so none is skipped below'
        + (idless.length ? ' — ID-LESS: ' + idless.join(', ') : ''));

    const anchors = await page.$$eval('[data-tip]', els => els.map(e => e.id).filter(Boolean));
    // v1.2.0 added two: the WINDOW panel's grainShape select and grainTilt knob.
    check(anchors.length >= 16, `found ${anchors.length} tooltip anchors (expected >= 16)`);

    const dwell = Number((appJs.match(/TOOLTIP_DELAY_MS\s*=\s*(\d+)/) || [])[1] || 350);

    let clampedCount = 0;
    let worstRight = -1e9, worstRightId = '-';

    // UI-02 hides one of the two TIME controls at any moment: the delayTime knob
    // in Sync, the division select in Free. Measuring only the default mode
    // would leave one control's tooltip unverified forever, so both modes are
    // swept and coverage is asserted at the end.
    const measured = new Set();

    const sweep = async (modeLabel) => {
      for (const id of anchors) {
        if (measured.has(id)) continue;

        const shown = await page.$eval(`#${id}`, el => {
            const r = el.getBoundingClientRect();
            return r.width > 0 && r.height > 0 && getComputedStyle(el).visibility !== 'hidden';
        }).catch(() => false);

        if (!shown) continue;   // hidden by the mode swap — the other pass takes it
        measured.add(id);

        await page.hover(`#${id}`);

        // AWAIT the dwell rather than sleeping a fixed guess past it: measuring
        // before .visible lands reads a zero-size rect and passes vacuously.
        let visible = true;
        try {
            await page.waitForFunction(
                () => document.getElementById('tooltip').classList.contains('visible'),
                null, { timeout: dwell + 1500 });
        } catch { visible = false; }

        if (!visible) {
            check(false, `#${id} [${modeLabel}]: tooltip became visible within ${dwell} ms dwell`);
            continue;
        }

        const m = await page.evaluate(() => {
            const t = document.getElementById('tooltip');
            const r = t.getBoundingClientRect();
            const arrow = parseFloat(getComputedStyle(t).getPropertyValue('--arrow-x')) || 0;
            return { left: r.left, right: r.right, top: r.top, bottom: r.bottom,
                     w: r.width, h: r.height, arrow,
                     text: (t.textContent || '').trim().length };
        });

        // 1. Not shrink-wrapped. A collapsed tip is ~70 px against a 230 px
        //    natural width; anything under half is the failure signature.
        const notShrunk = m.w > NATURAL_MAX_W * 0.5 || m.text < 40;

        // 2. BOTH edges inside the viewport — width alone is not enough.
        const insideX = m.left >= TOOLTIP_MARGIN - 0.5
                     && m.right <= SHIP_W - TOOLTIP_MARGIN + 0.5;

        // 3. The vertical flip keeps it on screen at the NEW height.
        const insideY = m.top >= -0.5 && m.bottom <= SHIP_H + 0.5;

        // 4. The arrow still points within the (possibly clamped) tip.
        const arrowOk = m.arrow >= 0 && m.arrow <= m.w;

        if (m.right > worstRight) { worstRight = m.right; worstRightId = id; }
        if (m.left <= TOOLTIP_MARGIN + 0.5 || m.right >= SHIP_W - TOOLTIP_MARGIN - 0.5)
            ++clampedCount;

        check(notShrunk && insideX && insideY && arrowOk,
            `#${id}: w=${m.w.toFixed(1)} x=[${m.left.toFixed(1)}, ${m.right.toFixed(1)}] `
            + `y=[${m.top.toFixed(1)}, ${m.bottom.toFixed(1)}] arrow=${m.arrow.toFixed(1)}`
            + (notShrunk ? '' : ' — SHRINK-WRAPPED')
            + (insideX ? '' : ' — OVERFLOWS HORIZONTALLY')
            + (insideY ? '' : ' — OVERFLOWS VERTICALLY')
            + (arrowOk ? '' : ' — ARROW OUTSIDE TIP'));

        await page.mouse.move(2, 2);
        await page.waitForFunction(
            () => !document.getElementById('tooltip').classList.contains('visible'),
            null, { timeout: 2000 }).catch(() => {});
      }
    };

    await sweep('sync');                     // default mode: division select shown
    await page.click('#seg-free');           // UI-02 swap: delayTime knob shown
    await page.waitForTimeout(50);
    await sweep('free');
    await page.click('#seg-sync');           // leave the page as it loaded

    check(measured.size === anchors.length,
        `every tooltip anchor was measured across both TIME modes — `
        + `${measured.size}/${anchors.length}`
        + (measured.size === anchors.length ? ''
           : ' — NEVER VISIBLE: ' + anchors.filter(a => !measured.has(a)).join(', ')));

    // The gate has to actually FIRE, or it proved nothing. If no control ever
    // reaches an edge, this test is passing vacuously and the clamp is untested
    // — which is precisely the state the v1.0.1 verification was in.
    check(clampedCount > 0,
        `the edge clamp actually engaged for ${clampedCount} control(s) — `
        + `a run where it never fires proves nothing about the clamp`);

    console.log(`\n   right-most tip: #${worstRightId} ends at ${worstRight.toFixed(1)} `
        + `of ${SHIP_W} (limit ${SHIP_W - TOOLTIP_MARGIN})`);

    await browser.close();
    server.close();
    fs.rmSync(root, { recursive: true, force: true });

    console.log(failed === 0 ? '== ALL CHECKS PASSED ==' : `== ${failed} CHECK(S) FAILED ==`);
    process.exit(failed);
})().catch(e => { console.error(e); process.exit(1); });
