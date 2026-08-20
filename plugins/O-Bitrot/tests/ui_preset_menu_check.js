/*
  ==============================================================================

    ui_preset_menu_check.js
    O-Bitrot — preset menu verification AT THE SHIPPING VIEWPORT (v1.13.0).

    Companion to ui_tooltip_clamp_check.js, and built on the same scaffolding:
    the REAL page, served from a copy of Source/ui/public with only
    js/juce/index.js swapped for tests/ui-stub/juce-stub.js, in a browser pinned
    to the exact 900 x 740 shipping size.

    WHY A DEDICATED FILE. The menu's correctness is not a property of the JS
    alone. Its section order and its contents come from C++
    (getPresetListGrouped, over the categorySpans table in PluginProcessor.cpp),
    are re-implemented by the browser stub so the page can be driven headlessly,
    and are finally rendered by index.html. That is THREE descriptions of one
    grouping, and any two of them can agree while the third drifts.

    So this file derives the grouping from the C++ ONCE and holds the other two
    to it:

      1. C++ -> expected. Factory preset names are parsed in DECLARATION order
         out of PluginProcessor.cpp, and the categorySpans table is parsed for
         the [first, last] index ranges. Nothing about the grouping is typed
         into this file, so a span edit that forgets a preset fails here rather
         than silently dropping it into "User" beside the user's own saves
         (pattern_test_fixture_mirrors_drift_silently).
      2. stub == expected. The stub's FACTORY_CATEGORIES is the browser's data
         source; if it drifts from the C++ every DOM assertion below still
         passes while the shipped plugin shows something else.
      3. DOM == expected. The rendered menu, opened by a real click.

    Geometry is measured, not assumed: .plugin sets overflow:hidden, so a menu
    that opens past the frame is CLIPPED rather than scrolled, and a 28-item
    list is tall enough for that to be a live risk.

    The behavioural assertions are written to fail if the menu is inert. In
    particular the load probe drives a click on a NON-current item and requires
    the readout to change to that name — an assertion that a menu which only
    opens and closes cannot pass, and which failed as written when the item
    handler was first stubbed out to confirm it discriminates.

    Usage:  node plugins/O-Bitrot/tests/ui_preset_menu_check.js
    Exit code = number of failed assertions (0 = all pass, 77 = could not run).
    Requires Playwright (`npx playwright install chromium` once).

  ==============================================================================
*/

'use strict';

const fs = require('fs');
const os = require('os');
const path = require('path');
const http = require('http');

const pluginRoot = path.resolve(__dirname, '..');
const publicDir = path.join(pluginRoot, 'Source', 'ui', 'public');

// Kept in sync with PluginEditor.cpp's setSize — and cross-checked against it
// below, so a resize that forgets this file fails loudly rather than measuring
// a stale viewport.
const SHIP_W = 900;
const SHIP_H = 740;

const MIME = {
    '.html': 'text/html; charset=utf-8',
    '.css': 'text/css; charset=utf-8',
    '.js': 'application/javascript; charset=utf-8',
    '.png': 'image/png',
    '.jpg': 'image/jpeg',
    '.webp': 'image/webp',
};

let failed = 0;
function check(cond, desc) {
    console.log(`  ${cond ? 'PASS' : 'FAIL'}: ${desc}`);
    if (!cond) ++failed;
}

// A wait that TIMES OUT is the most likely way this file reports a real
// regression — an inert menu never satisfies the condition it is waiting on.
// Playwright rejects on timeout, which would crash the run with a stack trace
// and an exit code that is no longer the failure count, so every wait goes
// through here and resolves to false instead. Verified by stubbing out the
// item click handler: the load probe then reports one clean FAIL rather than
// an uncaught TimeoutError.
async function settles(promise) {
    try { await promise; return true; } catch { return false; }
}

function buildRoot() {
    const root = fs.mkdtempSync(path.join(os.tmpdir(), 'obit-menu-'));
    fs.cpSync(publicDir, root, { recursive: true });
    fs.copyFileSync(path.join(pluginRoot, 'tests', 'ui-stub', 'juce-stub.js'),
                    path.join(root, 'js', 'juce', 'index.js'));
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

// ── C++ is the source of truth for the grouping ─────────────────────────────
// Factory names in DECLARATION order: the entries of the factoryPresets vector
// are the only `{ "Name",` openers at that indentation, and declaration order
// is exactly what the spans index into.
function parseFactoryOrder(processorCpp) {
    const vector = processorCpp.split('factoryPresets {')[1];
    if (!vector) return [];
    const body = vector.split(/\n\s*};/)[0];
    return [...body.matchAll(/^\s{8}\{\s*"([^"]+)",/gm)].map(m => m[1]);
}

function parseCategorySpans(processorCpp) {
    const table = processorCpp.split('categorySpans[] {')[1];
    if (!table) return [];
    const body = table.split('};')[0];
    return [...body.matchAll(/\{\s*"([^"]+)"\s*,\s*(\d+)\s*,\s*(\d+)\s*\}/g)]
        .map(m => ({ label: m[1], first: Number(m[2]), last: Number(m[3]) }));
}

// ── The stub's copy, which is what the browser actually renders from ────────
function parseStubCategories(stubJs) {
    const block = stubJs.split('FACTORY_CATEGORIES = [')[1];
    if (!block) return [];
    const body = block.split(/\n\];/)[0];
    // Split on each `{ category: "..."` opener, then take every quoted string
    // after the `presets:` marker within that chunk.
    const chunks = body.split(/\{\s*category:\s*"/).slice(1);
    return chunks.map(chunk => {
        const label = chunk.slice(0, chunk.indexOf('"'));
        const presetsPart = chunk.slice(chunk.indexOf('presets:'));
        const presets = [...presetsPart.matchAll(/"([^"]+)"/g)].map(m => m[1]);
        return { category: label, presets };
    });
}

const sameGrouping = (a, b) =>
    a.length === b.length && a.every((sec, i) =>
        sec.category === b[i].category &&
        sec.presets.length === b[i].presets.length &&
        sec.presets.every((n, j) => n === b[i].presets[j]));

const fmt = g => g.map(s => `${s.category}(${s.presets.length})`).join(' ');

(async () => {
    console.log('== O-Bitrot ui_preset_menu_check ==');
    console.log(`   viewport pinned to the SHIPPING size ${SHIP_W} x ${SHIP_H}\n`);

    const html = fs.readFileSync(path.join(publicDir, 'index.html'), 'utf8');
    const editorCpp = fs.readFileSync(path.join(pluginRoot, 'Source', 'PluginEditor.cpp'), 'utf8');
    const processorCpp = fs.readFileSync(path.join(pluginRoot, 'Source', 'PluginProcessor.cpp'), 'utf8');
    const stubJs = fs.readFileSync(path.join(pluginRoot, 'tests', 'ui-stub', 'juce-stub.js'), 'utf8');

    check(new RegExp(`setSize\\s*\\(\\s*${SHIP_W}\\s*,\\s*${SHIP_H}\\s*\\)`).test(editorCpp),
        `editor setSize is ${SHIP_W} x ${SHIP_H} — the viewport measured below`);

    // ── 1. Derive the expected grouping from the C++ ────────────────────────
    const factoryOrder = parseFactoryOrder(processorCpp);
    const spans = parseCategorySpans(processorCpp);

    check(factoryOrder.length > 0, `parsed the factory bank from PluginProcessor.cpp — ${factoryOrder.length} presets`);
    check(spans.length > 0, `parsed the categorySpans table — ${spans.length} categories`);

    // The spans must tile the bank exactly. This is the same invariant the C++
    // jasserts, restated where a Release build can still catch it: a preset
    // appended to the vector without extending a span would show up under
    // "User" in the shipped plugin, and no assert would fire.
    let tiled = true, cursor = 0;
    for (const s of spans) {
        if (s.first !== cursor) tiled = false;
        cursor = s.last + 1;
    }
    check(tiled && cursor === factoryOrder.length,
        `categorySpans tile the bank exactly — [0, ${cursor}) over ${factoryOrder.length} presets`);

    const expected = spans.map(s => ({
        category: s.label,
        presets: factoryOrder.slice(s.first, s.last + 1),
    }));
    console.log(`   expected grouping: ${fmt(expected)}\n`);

    // ── 2. The stub must agree with the C++ ─────────────────────────────────
    const stubGrouping = parseStubCategories(stubJs);
    check(sameGrouping(stubGrouping, expected),
        `the browser stub's FACTORY_CATEGORIES matches the C++ spans — ${fmt(stubGrouping)}`);

    // The bridge is only as good as its narrowest side: a name registered in
    // C++ and never requested in JS (or the reverse) fails silently at runtime
    // (pattern_webview_native_fn_bridge_gap).
    check(/withNativeFunction\("getPresetListGrouped"/.test(editorCpp),
        'C++ registers getPresetListGrouped');
    check(/getNativeFunction\("getPresetListGrouped"\)/.test(html),
        'the page requests getPresetListGrouped');
    check(/getPresetListGrouped:/.test(stubJs),
        'the stub implements getPresetListGrouped');

    const playwright = resolvePlaywright();
    if (!playwright) {
        console.error('\nCould not load Playwright. Run: npx playwright install chromium');
        process.exit(77);
    }

    const root = buildRoot();
    const { server, port } = await serve(root);
    const browser = await playwright.chromium.launch();
    const page = await browser.newPage({ viewport: { width: SHIP_W, height: SHIP_H } });

    const consoleErrors = [];
    page.on('console', m => { if (m.type() === 'error') consoleErrors.push(m.text()); });
    page.on('pageerror', e => consoleErrors.push(String(e)));

    try {
        await page.goto(`http://127.0.0.1:${port}/index.html`);
        // The band ships disabled and is un-disabled only once initialize()
        // resolves, so this waits on the real readiness signal rather than a
        // fixed sleep that could measure a half-built page.
        check(await settles(page.waitForFunction(
            () => !document.getElementById('preset-select').disabled,
            null, { timeout: 10000 })),
            'the preset band came up — initialize() resolved and un-disabled the trigger');

        const sel = '#preset-select';
        const menu = '#preset-menu';

        // ── 3. Closed at load ───────────────────────────────────────────────
        check(!(await page.locator(menu).isVisible()), 'the menu is closed at load');
        check(await page.getAttribute(sel, 'aria-expanded') === 'false',
            'aria-expanded is "false" while closed');

        // ── 4. Opens on click ───────────────────────────────────────────────
        await page.click(sel);
        await settles(page.waitForSelector(`${menu}.visible`, { timeout: 3000 }));
        check(await page.locator(menu).isVisible(), 'clicking the readout opens the menu');
        check(await page.getAttribute(sel, 'aria-expanded') === 'true',
            'aria-expanded flips to "true" when open');

        // ── 5. The DOM must agree with the C++ ──────────────────────────────
        const domGrouping = await page.evaluate(() => {
            const out = [];
            const menuEl = document.getElementById('preset-menu');
            for (const header of menuEl.querySelectorAll('.preset-menu-category')) {
                const grid = header.nextElementSibling;
                out.push({
                    category: header.textContent,
                    presets: [...grid.querySelectorAll('.preset-menu-item')]
                        .map(el => el.dataset.name),
                });
            }
            return out;
        });
        check(sameGrouping(domGrouping, expected),
            `the rendered menu matches the C++ grouping — ${fmt(domGrouping)}`);

        // No user presets exist in a fresh stub, so "User" must be ABSENT
        // rather than rendered empty.
        check(!domGrouping.some(s => s.category === 'User'),
            'no empty "User" section on a fresh instance');

        const itemCount = await page.locator('.preset-menu-item').count();
        check(itemCount === factoryOrder.length,
            `every factory preset is reachable — ${itemCount} items for ${factoryOrder.length} presets`);

        // ── 6. Geometry: .plugin is overflow:hidden, so a tall menu CLIPS ───
        const box = await page.locator(menu).boundingBox();
        check(box.x >= 0 && box.x + box.width <= SHIP_W,
            `the open menu is inside the frame horizontally — x=[${box.x.toFixed(1)}, ${(box.x + box.width).toFixed(1)}] of ${SHIP_W}`);
        check(box.y >= 0 && box.y + box.height <= SHIP_H,
            `the open menu is inside the frame vertically — y=[${box.y.toFixed(1)}, ${(box.y + box.height).toFixed(1)}] of ${SHIP_H}`);

        // Whether the list needed scrolling is not the point; that it is
        // REACHABLE if it did is. A clipped overflow would report scrollHeight
        // > clientHeight with no way to reach the tail.
        const scrollable = await page.evaluate(() => {
            const m = document.getElementById('preset-menu');
            return { scrollH: m.scrollHeight, clientH: m.clientHeight,
                     overflowY: getComputedStyle(m).overflowY };
        });
        check(scrollable.scrollH <= scrollable.clientH || scrollable.overflowY === 'auto',
            `the tail of the list is reachable — scrollHeight ${scrollable.scrollH} vs clientHeight ${scrollable.clientH}, overflow-y ${scrollable.overflowY}`);

        // ── 7. The highlight tracks the loaded preset ───────────────────────
        // A fresh instance reports "Default", which is deliberately NOT a list
        // member, so nothing may be highlighted yet.
        const readout = async () => (await page.textContent('#preset-name')).trim();
        check(await page.locator('.preset-menu-item.active').count() === 0,
            'nothing is highlighted while the readout is "Default" (not a list member)');

        // ── 8. Clicking an item LOADS it ────────────────────────────────────
        // Deliberately not the first item, and asserted on the readout rather
        // than on the click landing: a menu that opens, closes and changes
        // nothing passes every assertion above.
        const target = expected[2].presets[0];      // first of the third category
        await page.click(`.preset-menu-item[data-name="${target}"]`);
        await settles(page.waitForFunction(
            (name) => document.getElementById('preset-name').textContent.trim() === name,
            target, { timeout: 3000 }));
        check(await readout() === target, `clicking "${target}" loads it — the readout follows`);
        check(!(await page.locator(menu).isVisible()), 'selecting an item closes the menu');

        // ── 9. Reopening highlights what is loaded ──────────────────────────
        await page.click(sel);
        await settles(page.waitForSelector(`${menu}.visible`, { timeout: 3000 }));
        const activeNames = await page.$$eval('.preset-menu-item.active',
            els => els.map(e => e.dataset.name));
        check(activeNames.length === 1 && activeNames[0] === target,
            `exactly one item is highlighted, and it is the loaded one — ${JSON.stringify(activeNames)}`);

        // ── 10. Escape closes ───────────────────────────────────────────────
        await page.keyboard.press('Escape');
        check(!(await page.locator(menu).isVisible()), 'Escape closes the menu');

        // ── 11. A click outside closes ──────────────────────────────────────
        await page.click(sel);
        await settles(page.waitForSelector(`${menu}.visible`, { timeout: 3000 }));
        await page.click('.wordmark');
        check(!(await page.locator(menu).isVisible()), 'a click outside closes the menu');

        // ── 12. The ARROWS step through the MENU order, not the alphabet ────
        // The weak version of this check — "the highlight moved" — passes even
        // when the arrows walk a completely different order, and it did: the
        // module's prevButton/nextButton bind to the native selectNextPreset,
        // which walks the flat case-insensitive getPresetList(). That agreed
        // with a flat dropdown by coincidence, and grouping broke it silently
        // (pattern_grouping_preset_dropdown_breaks_prev_next).
        //
        // So the assertion is positional: from item i, ▶ must land on item
        // i+1 OF THE RENDERED LIST. Under the alphabetical walk the same click
        // moves from "Thrift-Store Turntable" (Vinyl, index 15) backwards to
        // "Total Media Failure" (Showcases, index 6), which this rejects.
        const walkOrder = expected.flatMap(s => s.presets);
        const fromIndex = walkOrder.indexOf(target);
        const expectNext = walkOrder[(fromIndex + 1) % walkOrder.length];

        await page.click('#preset-next');
        await settles(page.waitForFunction(
            (was) => document.getElementById('preset-name').textContent.trim() !== was,
            target, { timeout: 3000 }));
        const afterNext = await readout();
        await page.click(sel);
        await settles(page.waitForSelector(`${menu}.visible`, { timeout: 3000 }));
        const activeAfterArrow = await page.$$eval('.preset-menu-item.active',
            els => els.map(e => e.dataset.name));
        check(afterNext === expectNext,
            `▶ from "${target}" (index ${fromIndex}) lands on the NEXT item in the rendered list — expected "${expectNext}", got "${afterNext}"`);
        check(activeAfterArrow.length === 1 && activeAfterArrow[0] === afterNext,
            `the highlight follows the arrow — highlighted ${JSON.stringify(activeAfterArrow)}`);
        await page.keyboard.press('Escape');

        // ◀ must be the exact inverse: back to where we started.
        await page.click('#preset-prev');
        await settles(page.waitForFunction(
            (was) => document.getElementById('preset-name').textContent.trim() !== was,
            afterNext, { timeout: 3000 }));
        check(await readout() === target,
            `◀ is the inverse of ▶ — back to "${target}", got "${await readout()}"`);

        // Wrap-around at the ends of the list, where an off-by-one hides.
        const last = walkOrder[walkOrder.length - 1];
        await page.click(sel);
        await settles(page.waitForSelector(`${menu}.visible`, { timeout: 3000 }));
        await page.click(`.preset-menu-item[data-name="${last}"]`);
        await settles(page.waitForFunction(
            (name) => document.getElementById('preset-name').textContent.trim() === name,
            last, { timeout: 3000 }));
        await page.click('#preset-next');
        await settles(page.waitForFunction(
            (was) => document.getElementById('preset-name').textContent.trim() !== was,
            last, { timeout: 3000 }));
        check(await readout() === walkOrder[0],
            `▶ from the last item "${last}" wraps to the first "${walkOrder[0]}" — got "${await readout()}"`);

        // ── 13. The readout stayed childless ────────────────────────────────
        // _updateDisplay() assigns textContent, so any markup placed inside
        // #preset-name is erased on the first preset change. The caret has to
        // be a SIBLING (pattern_js_state_updater_overwrites_html_labels), and
        // it must still be there after the loads above.
        const caret = await page.evaluate(() => {
            const name = document.getElementById('preset-name');
            const c = document.querySelector('.preset-caret');
            return { nameChildren: name.children.length,
                     caretPresent: !!c,
                     caretText: c ? c.textContent.trim() : '',
                     caretInsideName: c ? name.contains(c) : false };
        });
        check(caret.nameChildren === 0, '#preset-name is still childless after several preset changes');
        check(caret.caretPresent && !caret.caretInsideName,
            `the caret survived as a sibling of the readout — "${caret.caretText}"`);

        // ── 14. The readout is centred on the PLATE, not on the leftovers ───
        // The caret is a flex child, so without a matching spacer opposite it
        // the name centres on (plate - caret - gap) and sits visibly left of
        // centre beside the symmetrical Save/Load/Delete row. Measured rather
        // than eyeballed: the offset is only ~7px and reads as sloppy rather
        // than as a bug.
        const centring = await page.evaluate(() => {
            const plate = document.getElementById('preset-select').getBoundingClientRect();
            const name = document.getElementById('preset-name').getBoundingClientRect();
            return { plateMid: plate.left + plate.width / 2,
                     nameMid: name.left + name.width / 2 };
        });
        const skew = Math.abs(centring.plateMid - centring.nameMid);
        check(skew <= 1.0,
            `the readout is centred in its plate — off by ${skew.toFixed(2)}px (limit 1.0)`);

        check(consoleErrors.length === 0,
            `no console errors — ${consoleErrors.length ? JSON.stringify(consoleErrors.slice(0, 3)) : 'clean'}`);

    } finally {
        await browser.close();
        server.close();
        fs.rmSync(root, { recursive: true, force: true });
    }

    console.log(failed === 0 ? '\n== ALL CHECKS PASSED ==' : `\n== ${failed} CHECK(S) FAILED ==`);
    process.exit(failed);
})();
