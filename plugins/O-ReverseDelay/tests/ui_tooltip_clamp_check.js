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

    v1.1.0 resized the editor 940x484 -> 940x743, v1.7.0 to 940x972, and v1.7.1
    back down to 940x768. None of them changed the WIDTH, which is what the
    horizontal clamp depends on — but the tooltip layer is position:fixed and its
    vertical flip (above -> below) is a function of viewport HEIGHT, and every
    one of those resizes moved every anchor on the page. Row 3 is the case that
    matters most for the flip: it sits lowest, so its tips have the most room
    above them and the least below, which is the opposite of row 1's situation.
    Every earlier C5 verification is therefore invalid at the new height and this
    re-measures all of them.

    v1.7.1 is the first one that made the page SHORTER, and it moved every anchor
    UP: row 1 by ~32 px, row 3 by ~102 px. A shorter page cannot overflow a tip
    horizontally, but it can leave row 1 without the clearance to place a tip
    above its knob, and it lowers the bottom edge a row-3 tip must stay inside.
    Both are assertion 3, and it is the reason this file is re-run for a resize
    that only removed empty space.

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
// v1.7.1: 972 -> 768 (the empty space rows 1 and 3 and .groups were carrying).
// Cross-checked against PluginEditor.cpp below, so a resize that forgets this
// file fails loudly rather than measuring a stale viewport
// (pattern_test_fixture_mirrors_drift_silently).
//
// This is the first resize in the plugin's history that made the page SHORTER,
// which inverts what the vertical assertions are guarding. Every earlier resize
// added room below a tip and the risk was a tip flipping down off the bottom;
// this one takes 204 px away, so the risk is the opposite — a tip on row 1 no
// longer having the clearance to sit ABOVE its knob, and a tip on row 3 pushed
// past a bottom edge that has moved up. Assertion 3 covers both directions and
// is re-run here rather than reasoned about.
const SHIP_W = 940;
const SHIP_H = 768;

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
    // Compared with === rather than >=: the loose form let this line print
    // "14/13" once grainCount landed, and a bound that can never fail upward
    // would also never notice a knob going missing while another was added.
    //
    // ── v1.6.0: the expected count is PARSED, not typed ──────────────────────
    // It was a literal through v1.5.0 — 13, then 14, then 15 — and it has now
    // failed three times for the same non-reason: a release added a knob and the
    // fixture still described the release before it. That is exactly
    // pattern_test_fixture_mirrors_drift_silently, and the third occurrence is
    // the argument for deriving it. KNOB_IDS in app.js is the list the page
    // actually binds, so it is the only honest source for how many readouts
    // should be populated — and if a knob is added there and nowhere else, this
    // check now fails for the RIGHT reason instead of needing a hand edit.
    const knobBlock = fs.readFileSync(path.join(publicDir, 'js', 'app.js'), 'utf8')
        .match(/const KNOB_IDS = \[([\s\S]*?)\];/);
    const expectedReadouts = knobBlock
        ? [...knobBlock[1].matchAll(/"([A-Za-z0-9_]+)"/g)].length
        : -1;

    check(expectedReadouts > 0,
        `KNOB_IDS parsed from app.js — expecting ${expectedReadouts} readouts`);
    check(boundReadouts === expectedReadouts,
        `app.js ran and bound the knobs — ${boundReadouts}/${expectedReadouts} readouts populated`);
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

    // ── v1.7.3 (IN-03): the WINDOW panel's height budget, MEASURED ───────────
    // styles.css carried this budget written down twice, 120 lines apart, and
    // the two disagreed. One said 214 px of content into a 213 px body ("already
    // 1 px over", itemising an 80 px knob-cell); the other said 212 of 213 ("one
    // px spare", itemising 78). Neither could fail a build, so the contradiction
    // survived three releases.
    //
    // Measured here, the 78/212 version is the correct one and the 80/214 one
    // was wrong. That direction matters more than the px: the wrong figure was
    // produced by RECOMPUTING the cell from the rules as written (gap 5 + knob
    // 46 + label + value at line-height:normal) instead of observing it, and
    // recomputation is exactly what put a second budget in the file to begin
    // with. Same lesson as pattern_flex1_container_slack_invisible_to_row_sum,
    // where five releases of "EXACTLY zero slack" sat above 204 px of real empty
    // space because the sum was computed rather than rendered.
    //
    // So the number now lives in ONE comment and ONE assertion that can fail.
    //
    // The body is display:flex/wrap with align-content:center, so the cells do
    // NOT stack one-per-row: the two knob-cells share a flex row. Grouping the
    // children by their top offset is what makes this a measurement of the real
    // layout rather than of an assumed one — a naive sum over all four children
    // double-counts the knob row and reports 290 into 213.
    //
    // WINDOW is the tightest panel on the page and it is what pins row 2 at
    // 245 px, so this is the assertion that notices if it silently gains
    // content. .group sets no overflow, so an overrun would not clip — it bleeds
    // into the panel's padding, invisible to the eye as well as to the build.
    //
    // ── The budget is measured against the PANEL, not the body ──────────────
    // The first version of this check compared the content sum with the body's
    // own clientHeight and was structurally incapable of failing. .group-body is
    // `flex: 1 1 0%` with the default min-height:auto, so it does not clip or
    // scroll when its content grows — it GROWS, and clientHeight grows with it.
    // Inflating the knob 46 -> 56 px moved content and clientHeight together
    // (213 -> 222) and the assertion happily reported "222 into 222, 0 px
    // spare". That is the same never-firing guard as
    // pattern_flex1_container_slack_invisible_to_row_sum, rebuilt by accident
    // while fixing its twin.
    //
    // What is actually fixed is the PANEL: `.group-row-2 .group { height: 245px }`.
    // Its content box — 245 − 2 − 2 border − 16 − 12 padding = 213 — is the real
    // ceiling, and the body silently overflowing it into the bottom padding is
    // exactly the failure worth catching. So the bound is derived from the
    // panel's own computed box here, not from the body and not from a literal.
    const win = await page.evaluate(() => {
        const body = document.querySelector('.group-window .group-body');
        if (!body) return null;
        const panel = document.querySelector('.group-window');
        const cs = getComputedStyle(body);
        const pcs = getComputedStyle(panel);
        const br = body.getBoundingClientRect();
        const px = (v) => parseFloat(v) || 0;

        // The panel's fixed content box: what the body is actually allowed.
        const panelBox = panel.getBoundingClientRect().height
                       - px(pcs.borderTopWidth) - px(pcs.borderBottomWidth)
                       - px(pcs.paddingTop)     - px(pcs.paddingBottom);

        // Group children into flex rows by top offset; a row's height is its
        // tallest child.
        const rows = new Map();
        for (const c of body.children) {
            const r = c.getBoundingClientRect();
            const key = Math.round(r.top - br.top);
            rows.set(key, Math.max(rows.get(key) ?? 0, r.height));
        }
        const rowHeights = [...rows.entries()].sort((a, b) => a[0] - b[0]).map(e => e[1]);
        const gap = parseFloat(cs.rowGap) || 0;
        const contentSum = rowHeights.reduce((a, b) => a + b, 0)
                         + gap * Math.max(0, rowHeights.length - 1);

        const cell = (sel) => {
            const el = document.querySelector(`.group-window ${sel}`);
            return el ? +el.getBoundingClientRect().height.toFixed(2) : -1;
        };

        return {
            clientHeight: body.clientHeight,
            scrollHeight: body.scrollHeight,
            bodyHeight:  +br.height.toFixed(2),
            panelBox:    +panelBox.toFixed(2),
            rowHeights, gap,
            contentSum:  +contentSum.toFixed(2),
            selectCell:  cell('.select-cell'),
            knobCell:    cell('.knob-cell'),
            envCell:     cell('.env-cell'),
            panelH:      panel.getBoundingClientRect().height,
        };
    });

    check(win !== null, 'the WINDOW panel body is present and measurable');

    if (win) {
        const slack = +(win.panelBox - win.contentSum).toFixed(2);

        // A CEILING, not an equality — content may shrink, it may not overrun.
        // This is the assertion the two prose budgets could never be.
        check(win.contentSum <= win.panelBox,
            `WINDOW panel content fits the panel's fixed content box — `
            + `${win.contentSum} into ${win.panelBox} px, ${slack} px spare `
            + `(rows ${win.rowHeights.join(' + ')} @ gap ${win.gap})`);

        // The body itself must not have been pushed past that box either. This
        // is the one that catches a flex:1 body silently growing into the
        // panel's bottom padding.
        check(win.bodyHeight <= win.panelBox,
            `WINDOW panel body stays inside the panel's content box — `
            + `body ${win.bodyHeight} <= ${win.panelBox} px `
            + `(panel ${win.panelH} less its border and padding)`);

        // The knob-cell is the exact figure the two dead budgets disagreed
        // about, so it is pinned rather than left to drift again.
        check(win.knobCell === 78,
            `WINDOW knob-cell renders at 78 px, matching the surviving styles.css `
            + `itemisation (the deleted budget claimed 80) — measured ${win.knobCell}`);

        console.log(`   WINDOW budget: select ${win.selectCell} + knob ${win.knobCell} `
            + `+ env ${win.envCell} + 2 gaps @ ${win.gap} = ${win.contentSum} `
            + `into body ${win.clientHeight} (panel ${win.panelH}) — ${slack} px spare`);
    }

    await browser.close();
    server.close();
    fs.rmSync(root, { recursive: true, force: true });

    console.log(failed === 0 ? '== ALL CHECKS PASSED ==' : `== ${failed} CHECK(S) FAILED ==`);
    process.exit(failed);
})().catch(e => { console.error(e); process.exit(1); });
