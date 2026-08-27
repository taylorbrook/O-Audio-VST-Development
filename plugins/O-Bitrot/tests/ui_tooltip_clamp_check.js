/*
  ==============================================================================

    ui_tooltip_clamp_check.js
    O-Bitrot — hover-help verification AT THE SHIPPING VIEWPORT (v1.12.0).

    Ported from O-Tapestop v1.4.0, which ported it from O-ReverseDelay v1.1.0 —
    the only version of this check that has actually caught the bug it exists
    for.

    WHY A DEDICATED FILE. A static read of index.html can prove that showTip()
    releases, measures and PINS its width before applying `left`, and that
    `left` is clamped into [MARGIN, innerWidth - width - MARGIN]. It cannot
    prove the clamp ever FIRES, because that depends entirely on the viewport.
    At Chromium's default 1280 px there is room to the right of every control,
    the clamp never engages, and "the tooltip fits" passes while the real
    900 px window overflows (pattern_tooltip_clamp_gate_viewport_sensitive).

    So this drives the REAL page — same HTML, same inline CSS, same inline
    module, only js/juce/index.js swapped for the stub — in a browser pinned to
    the exact shipping size, and measures the rendered rectangle.

    What is asserted, for EVERY control carrying a tip:
      1. The tip is at its natural width, not shrink-wrapped — a fixed-position
         box with `left` set and `width:auto` collapses into the space left to
         its right, turning a 230 px tip into a ~70 px ribbon.
      2. left >= MARGIN AND right <= innerWidth - MARGIN. Asserting width alone
         is what let the O-MultiBandCompressor v1.4.1 bug through; both edges
         must be checked.
      3. top >= 0 and bottom <= innerHeight — the above/below flip really does
         keep it on screen.
      4. The arrow still points inside the tip after clamping.

    COVERAGE. Two things hide anchors on this page, and a sweep that ignores
    either leaves tips unverified forever:

      * The global strip's Sync/Free swap slot. CLOCK_SYNC_DIV and
        CLOCK_FREE_RATE occupy the same slot and only one is ever mounted.
      * The seven family enables. Four families ship OFF, and before v1.12.0
        `.panel.off .p-body` carried `pointer-events: none`, which suppressed
        mouseover along with dragging — 18 of 53 anchors could not raise a tip
        at all. v1.12.0 narrowed that block onto the interactive elements
        themselves. The sweep runs the OFF state FIRST, precisely so a
        regression of that rule fails here rather than shipping.

    Anchors are stamped with data-tip-probe at run time rather than enumerated
    by id: most anchors on this page are .ctl containers carrying no id, and an
    id-based enumeration would skip them silently while the tally still looked
    complete.

    The dwell is AWAITED, not slept past by a fixed guess — measuring before
    .visible lands reads a zero-size rect and passes vacuously.

    Usage:  node plugins/O-Bitrot/tests/ui_tooltip_clamp_check.js
    Exit code = number of failed assertions (0 = all pass, 77 = could not run).
    Requires Playwright (`npx playwright install chromium` once).

  ==============================================================================
*/

'use strict';

const fs = require('fs');
const os = require('os');
const path = require('path');
const http = require('http');
const vm   = require('vm');   // v1.14.0 — the anchor count is derived from js/i18n.js

const pluginRoot = path.resolve(__dirname, '..');
const publicDir = path.join(pluginRoot, 'Source', 'ui', 'public');

// Kept in sync with PluginEditor.cpp's setSize and the inline CSS — and
// cross-checked against both below, so a resize that forgets this file fails
// loudly rather than measuring a stale viewport
// (pattern_test_fixture_mirrors_drift_silently).
const SHIP_W = 900;
const SHIP_H = 740;

// Inline-script / inline-CSS constants — mirrored, and cross-checked below.
const TOOLTIP_MARGIN = 8;

// .tooltip's max-width. v1.14.0 PARSES this out of the page's own CSS rather
// than hard-coding it, because the cap differs per plugin — 230 here and in
// O-ReverseDelay and O-Tapestop, 240 in O-Octagon, 220 in O-Polystutter — and a
// file that mirrors one plugin's number would mis-assert the moment it is
// pointed at another. It is the cap French has to wrap INSIDE, so it is
// load-bearing for every assertion below.
//
// The literal survives as the drift guard, not as the source of truth: the
// parsed value drives the measurements, and the assertion below fails if the
// two disagree (pattern_test_fixture_mirrors_drift_silently).

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

// ── Build the served tree: production page + the bridge stub ────────────────
// Byte-identical to Source/ui/public except js/juce/index.js, exactly as
// tests/ui-stub/serve-stub.sh does it. O-Bitrot vendors preset-manager.js
// inside Source/ui/public/modules/, which the resource provider serves at the
// same path, so the tree copy already covers it.
function buildRoot() {
    const root = fs.mkdtempSync(path.join(os.tmpdir(), 'obit-tip-'));
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

(async () => {
    console.log('== O-Bitrot ui_tooltip_clamp_check ==');
    console.log(`   viewport pinned to the SHIPPING size ${SHIP_W} x ${SHIP_H}\n`);

    // ── Guard the mirrored constants against drift ──────────────────────────
    // O-Bitrot's UI is a SINGLE file: the CSS and the module are both inline in
    // index.html, so all three greps read the same source.
    const html = fs.readFileSync(path.join(publicDir, 'index.html'), 'utf8');
    const editorCpp = fs.readFileSync(path.join(pluginRoot, 'Source', 'PluginEditor.cpp'), 'utf8');
    const processorCpp = fs.readFileSync(path.join(pluginRoot, 'Source', 'PluginProcessor.cpp'), 'utf8');

    check(new RegExp(`TOOLTIP_MARGIN\\s*=\\s*${TOOLTIP_MARGIN}\\b`).test(html),
        `TOOLTIP_MARGIN in index.html is ${TOOLTIP_MARGIN} (this file mirrors it)`);
    // Parsed from the .tooltip RULE specifically, not from the first max-width
    // on the page — .settings-popover and .preset-menu carry widths of their
    // own and a loose scan would silently measure against one of those.
    const DOCUMENTED_MAX_W = 230;
    const tipRule = html.match(/\.tooltip\s*\{[\s\S]*?\}/);
    const capMatch = tipRule && tipRule[0].match(/max-width:\s*(\d+(?:\.\d+)?)px/);
    const NATURAL_MAX_W = capMatch ? parseFloat(capMatch[1]) : NaN;

    check(Number.isFinite(NATURAL_MAX_W),
        `.tooltip max-width parsed from index.html — got ${capMatch ? capMatch[1] + 'px' : 'NOTHING'}`);
    check(NATURAL_MAX_W === DOCUMENTED_MAX_W,
        `.tooltip max-width is the documented ${DOCUMENTED_MAX_W}px — parsed ${NATURAL_MAX_W}px. `
        + `If the cap moved deliberately, move DOCUMENTED_MAX_W with it and re-read the `
        + `French sweep below: French wraps INSIDE this cap, so changing it changes every `
        + `tip height and therefore every vertical-flip decision`);
    check(new RegExp(`setSize\\s*\\(\\s*${SHIP_W}\\s*,\\s*${SHIP_H}\\s*\\)`).test(editorCpp),
        `editor setSize is ${SHIP_W} x ${SHIP_H} — the viewport measured below`);

    // The restore guard. isBool()/isInt() reads as obviously correct and is
    // wrong — the XML round-trip rebuilds every property as a var over the
    // attribute STRING, so a type test is false for every saved session and the
    // preference restores as OFF forever
    // (critical_valuetree_xml_roundtrip_loses_type). Asserted as source text
    // here AND as behaviour by the render-harness round-trip probe.
    check(/tooltipsEnabled[\s\S]{0,400}?isVoid\s*\(\s*\)/.test(processorCpp),
        'setStateInformation guards the preference on !isVoid(), not isBool()/isInt()');

    // The expected anchor count is DERIVED, never typed. A literal here fails
    // for the wrong reason every time a control gains a tip, and the hand edit
    // that follows is how a fixture starts describing the release before it
    // (pattern_test_fixture_mirrors_drift_silently).
    //
    // v1.14.0 moved the source of truth. It used to count `data-tip=` literals
    // in index.html; the copy has since left the markup, so that count is now
    // zero and the assertion would pass vacuously against nothing. TIP_BINDINGS
    // in js/i18n.js is the list the page actually binds, so it is the only
    // honest source — and if a binding is added there and the element is not on
    // the page, the coverage assertion below now fails for the RIGHT reason.
    //
    // i18n.js is an ES module outside any package.json, so node can neither
    // require() nor import() it synchronously. Evaluated in a vm sandbox with
    // the export keywords stripped, exactly as scripts/check-i18n.js does it.
    const i18nSrc = fs.readFileSync(path.join(publicDir, 'js', 'i18n.js'), 'utf8')
        .replace(/(^|\n)(\s*)export\s+(const|let|function|class)\s/g, '$1$2$3 ');
    const i18nBox = { console: { warn() {}, error() {}, log() {} } };
    vm.createContext(i18nBox);
    vm.runInContext(`${i18nSrc}\n;globalThis.__x = { I18N, TIP_BINDINGS };`,
                    i18nBox, { timeout: 5000 });
    const expectedAnchors = i18nBox.__x.TIP_BINDINGS.length;

    check(expectedAnchors > 0,
        `TIP_BINDINGS parsed from js/i18n.js — expecting ${expectedAnchors} anchors`);

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
    // Not a cosmetic slip: at 1280 every tip on this page has room to the right
    // and the clamp never engages, so every assertion passes while the real
    // 900 px window overflows. That mistake is the failure this whole file
    // exists to catch, and the viewport assertion below makes it loud.
    const page = await browser.newPage({ viewport: { width: SHIP_W, height: SHIP_H } });

    const consoleErrors = [];
    page.on('console', m => { if (m.type() === 'error') consoleErrors.push(m.text()); });
    page.on('pageerror', e => consoleErrors.push(String(e)));

    await page.goto(`http://127.0.0.1:${port}/index.html`, { waitUntil: 'networkidle' });

    // The page must actually be alive: a TDZ throw out of module evaluation
    // kills every control while leaving the HTML looking correct
    // (pattern_module_toplevel_init_tdz). The .ro readouts are born EMPTY in
    // the markup and are written only by setupKnob's render(), so a populated
    // readout proves the module ran.
    const expectedReadouts = (html.match(/<div class="ro"/g) || []).length;
    const boundReadouts = await page.$$eval('.ro',
        els => els.filter(e => e.textContent.trim() !== '').length);

    check(expectedReadouts > 0,
        `.ro readouts parsed from index.html — expecting ${expectedReadouts}`);
    check(boundReadouts === expectedReadouts,
        `the module ran and bound the knobs — ${boundReadouts}/${expectedReadouts} readouts populated`);
    check(consoleErrors.length === 0,
        'no console errors on load' + (consoleErrors.length ? ` — ${consoleErrors[0]}` : ''));

    // Real viewport, not a guess.
    const vp = await page.evaluate(() => ({ w: window.innerWidth, h: window.innerHeight }));
    check(vp.w === SHIP_W && vp.h === SHIP_H,
        `viewport really is ${SHIP_W} x ${SHIP_H} — got ${vp.w} x ${vp.h}`);

    // ── Stamp the anchors ───────────────────────────────────────────────────
    // By index, not by id. Most anchors here are .ctl containers with no id at
    // all, and an id-based enumeration would drop them while the tally still
    // read as full coverage. The label is the tip's own title, which is what a
    // failure needs to be actionable.
    const anchors = await page.evaluate(() => {
        const els = [...document.querySelectorAll('[data-tip]')];
        els.forEach((el, i) => el.setAttribute('data-tip-probe', String(i)));
        return els.map((el, i) => ({
            i,
            label: el.getAttribute('data-tip-title') || el.id || el.className || el.tagName,
        }));
    });

    check(anchors.length === expectedAnchors,
        `every data-tip in index.html is present in the DOM — `
        + `${anchors.length}/${expectedAnchors}`);

    // ── The help layer must ship OFF ────────────────────────────────────────
    // The processor defaults tooltipsEnabled to false and the page PULLS it, so
    // a fresh instance shows an unlit toggle and a silent layer. Asserting the
    // shipped state before turning it on is what keeps a default flip from
    // going unnoticed.
    const initial = await page.evaluate(() => {
        const t = document.getElementById('help-toggle');
        return { active: t.classList.contains('active'),
                 pressed: t.getAttribute('aria-pressed'),
                 caption: t.textContent.trim(),
                 key: t.dataset.i18n ?? null,
                 mirror: t.dataset.label ?? null };
    });
    check(initial.active === false && initial.pressed === 'false',
        `the help layer ships OFF — .active=${initial.active}, aria-pressed=${initial.pressed}`);

    // ── v1.15.0: this assertion was REWRITTEN A SECOND TIME, and it still
    //    guards the same rule ─────────────────────────────────────────────────
    // Through v1.13.0 the toggle was a "?" circle and this pinned the literal
    // "?". v1.14.0 moved it into the settings popover where it reads On/Off, so
    // the caption became script-written and the check moved to comparing it
    // against the data-on / data-off attributes AUTHORED ON THE ELEMENT.
    //
    // v1.15.0 removes those attributes, deliberately. An attribute holds ONE
    // string; on a two-language page the off face would have been restored in
    // ENGLISH the instant the user picked Français. The faces are keys now, and
    // setLabel() makes the element a [data-i18n] element the language sweep
    // owns.
    //
    // The rule being guarded has not moved: the caption must never be a literal
    // in the JS. Its evidence has. applyLabel() writes textContent and
    // dataset.label TOGETHER, so `dataset.label === textContent` is the
    // ownership mirror — the systemic form of
    // pattern_js_state_updater_overwrites_html_labels, and the same assertion
    // scripts/check-ui-labels.js makes repo-wide. If the caption were written
    // as a literal anywhere, the mirror and the text would disagree.
    check(initial.key === 'ui.off',
        `the unlit toggle declares its own key — data-i18n="${initial.key}", expected "ui.off"`);
    check(initial.mirror !== null && initial.caption === initial.mirror,
        `the caption IS the ownership mirror, not a JS literal — rendered `
        + `"${initial.caption}", dataset.label "${initial.mirror}"`);

    const dwell = Number((html.match(/TOOLTIP_DELAY_MS\s*=\s*(\d+)/) || [])[1] || 350);

    const tipVisible = () => page.evaluate(
        () => document.getElementById('tooltip').classList.contains('visible'));

    const hoverProbe = async (i) => {
        await page.hover(`[data-tip-probe="${i}"]`);
        try {
            await page.waitForFunction(
                () => document.getElementById('tooltip').classList.contains('visible'),
                null, { timeout: dwell + 900 });
        } catch { /* not visible — the caller decides whether that is correct */ }
    };

    const unhover = async () => {
        await page.mouse.move(2, 2);
        await page.waitForFunction(
            () => !document.getElementById('tooltip').classList.contains('visible'),
            null, { timeout: 2000 }).catch(() => {});
    };

    // ── With the layer OFF, only the exempt anchor may show ─────────────────
    // The toggle carries data-tip-always so the one control that can turn help
    // back on is never the one control unable to explain itself. Everything
    // else must stay silent, or the toggle is decoration.
    // v1.14.0: probed on the GEAR rather than the toggle. The toggle moved
    // inside the settings panel, which ships hidden, so it cannot raise a tip
    // until the panel is opened — and the control that has to keep explaining
    // itself while help is off is now the one that REACHES the panel. Both
    // carry data-tip-always; this is the one a user can reach first.
    await hoverProbe(anchors.find(a => a.label === 'Settings').i);
    check(await tipVisible(),
        'with help OFF, the gear\'s own tip still shows (data-tip-always)');
    await unhover();

    await hoverProbe(anchors.find(a => a.label === 'Mix').i);
    check((await tipVisible()) === false,
        'with help OFF, an ordinary control shows no tip');
    await unhover();

    // ── Turn it on, and confirm it persisted through the bridge ─────────────
    // Two clicks now, not one: the gear opens the panel, then the toggle inside
    // it. Driven as real clicks rather than by calling the handler, because a
    // panel that renders but is pointer-dead behind a z-index tie is a bug this
    // suite has already shipped once and no static check can see.
    await page.click('#gear-btn');
    await page.waitForTimeout(60);
    check(await page.$eval('#settings-popover', el => !el.hidden).catch(() => false),
        'the gear opens the settings popover');

    await page.click('#help-toggle');
    const afterClick = await page.evaluate(async () => {
        const juce = await import('./js/juce/index.js');
        const t = document.getElementById('help-toggle');
        return {
            active: t.classList.contains('active'),
            pressed: t.getAttribute('aria-pressed'),
            caption: t.textContent.trim(),
            key: t.dataset.i18n ?? null,
            mirror: t.dataset.label ?? null,
            persisted: await juce.getNativeFunction('getTooltipsEnabled')(),
        };
    });
    check(afterClick.active === true && afterClick.pressed === 'true',
        `clicking the toggle lights it — .active=${afterClick.active}, aria-pressed=${afterClick.pressed}`);
    check(afterClick.key === 'ui.on',
        `the lit toggle swapped to the other KEY — data-i18n="${afterClick.key}", expected "ui.on"`);
    check(afterClick.mirror !== null && afterClick.caption === afterClick.mirror,
        `the lit caption IS the ownership mirror, not a JS literal — rendered `
        + `"${afterClick.caption}", dataset.label "${afterClick.mirror}"`);
    check(afterClick.persisted === true,
        `the preference reached the native bridge — getTooltipsEnabled() = ${afterClick.persisted}`);

    // Close it again, so the sweep below starts from the page as it ships.
    await page.keyboard.press('Escape');
    await page.waitForTimeout(60);

    // ══════════════════════════════════════════════════════════════════════
    // v1.14.0 — THE SWEEP IS PARAMETERISED BY LANGUAGE
    // ══════════════════════════════════════════════════════════════════════
    //
    // Not duplicated: one process, one page load, all three passes run once per
    // language. Duplicating the file would have produced two copies free to
    // drift, which is the failure this repo has already paid for twice.
    //
    // WHY IT HAS TO BE RE-RUN AT ALL. Assertion 3 is the language-sensitive one.
    // French runs longer than English, and .tooltip's max-width is a hard cap —
    // so a longer string does not get wider, it WRAPS TO MORE LINES. `height`
    // grows, `top = anchor.top - height - MARGIN` moves further up, and the
    // above/below flip has to catch what no longer fits above. Assertion 2 is
    // capped by max-width and cannot move. 900 x 740 is a SMALLER frame than the
    // 1100 x 720 where French was first measured, which is where the risk sits.
    //
    // The language is driven through window.__setLanguage — applyI18n, exposed
    // by the canonical block for exactly this. It rewrites every anchor's two
    // attributes synchronously and fires NO `change` event, so no native
    // round-trip has to complete first.
    //
    // Every failure is LABELLED with its language. Without that a French-only
    // failure reads as a mysterious regression in a file that never mentions
    // French.
    //
    // NOTE the anchor LABELS stay English throughout. They are captured once, at
    // load, before any language switch — a label that localized with the copy
    // would make the two runs' failures impossible to line up against each
    // other, and would break the probe lookups below that address a control by
    // its English name.
    const LANGS = ['en', 'fr'];
    const tipTextByLang = new Map();
    const stats = new Map();

    for (const lang of LANGS) {
      const applied = await page.evaluate((l) => {
          if (typeof window.__setLanguage !== 'function') return '__MISSING__';
          window.__setLanguage(l);
          const sel = document.getElementById('lang-select');
          return sel ? sel.value : l;
      }, lang);

      check(applied === lang,
          `[${lang}] window.__setLanguage('${lang}') applied and synced the selector`
          + (applied === '__MISSING__' ? ' — window.__setLanguage IS NOT DEFINED' : ` — got "${applied}"`));

      const texts = await page.evaluate(() => Object.fromEntries(
          [...document.querySelectorAll('[data-tip]')].map(e => [
              e.getAttribute('data-tip-probe'),
              { t: e.getAttribute('data-tip-title') || '', b: e.getAttribute('data-tip') || '' },
          ])));
      tipTextByLang.set(lang, texts);

      const blank = Object.entries(texts).filter(([, v]) => !v.t || !v.b).map(([k]) => k);
      check(blank.length === 0,
          `[${lang}] every anchor carries a non-empty title AND body`
          + (blank.length ? ' — BLANK probes: ' + blank.join(', ') : ''));

      // An unsubstituted {token} is a tr() vars bug that renders as literal
      // braces in the shipped tip.
      const unsub = Object.entries(texts)
          .filter(([, v]) => /\{\w+\}/.test(v.t) || /\{\w+\}/.test(v.b)).map(([k]) => k);
      check(unsub.length === 0,
          `[${lang}] no unsubstituted {token} placeholder survives into a tip`
          + (unsub.length ? ' — UNSUBSTITUTED probes: ' + unsub.join(', ') : ''));

      let clampedCount = 0;
      let flippedCount = 0;
      let worstRight = -1e9, worstRightLabel = '-';
      let widest = 0, tallest = 0;
      const measured = new Set();

      const sweep = async (stateLabel) => {
        for (const a of anchors) {
            if (measured.has(a.i)) continue;

            // Hoverable, not merely present: an anchor inside a pointer-events
            // blocked subtree renders at full size but can never raise a
            // mouseover, so a visibility-only gate would mark it measured and
            // then fail it for the wrong reason.
            const shown = await page.$eval(`[data-tip-probe="${a.i}"]`, el => {
                const r = el.getBoundingClientRect();
                if (r.width <= 0 || r.height <= 0) return false;
                if (getComputedStyle(el).visibility === 'hidden') return false;
                for (let n = el; n; n = n.parentElement)
                    if (getComputedStyle(n).pointerEvents === 'none') return false;
                return true;
            }).catch(() => false);

            if (!shown) continue;   // hidden or blocked — a later pass takes it
            measured.add(a.i);

            await hoverProbe(a.i);

            if (!(await tipVisible())) {
                check(false, `[${lang}] ${a.label} [${stateLabel}]: tip became visible within ${dwell} ms dwell`);
                continue;
            }

            const m = await page.evaluate(() => {
                const t = document.getElementById('tooltip');
                const r = t.getBoundingClientRect();
                const arrow = parseFloat(getComputedStyle(t).getPropertyValue('--arrow-x')) || 0;
                return { left: r.left, right: r.right, top: r.top, bottom: r.bottom,
                         w: r.width, h: r.height, arrow,
                         placement: t.getAttribute('data-placement') || '?',
                         text: (t.textContent || '').trim().length };
            });

            // 1. Not shrink-wrapped. A collapsed tip is ~70 px against a 230 px
            //    natural width; anything under half is the failure signature.
            //    Short copy legitimately renders narrow, hence the text gate.
            const notShrunk = m.w > NATURAL_MAX_W * 0.5 || m.text < 40;

            // 2. BOTH edges inside the viewport — width alone is not enough.
            const insideX = m.left >= TOOLTIP_MARGIN - 0.5
                         && m.right <= SHIP_W - TOOLTIP_MARGIN + 0.5;

            // 3. The above/below flip really keeps it on screen. THE
            //    LANGUAGE-SENSITIVE ONE.
            const insideY = m.top >= -0.5 && m.bottom <= SHIP_H + 0.5;

            // 4. The arrow still points within the (possibly clamped) tip.
            const arrowOk = m.arrow >= 0 && m.arrow <= m.w;

            // 5. The cap is a CAP. A tip wider than max-width means the CSS is
            //    not doing the wrapping this language sweep depends on.
            const withinCap = m.w <= NATURAL_MAX_W + 24 + 0.5;   // + padding/border

            if (m.right > worstRight) { worstRight = m.right; worstRightLabel = a.label; }
            if (m.w > widest)  widest  = m.w;
            if (m.h > tallest) tallest = m.h;
            if (m.left <= TOOLTIP_MARGIN + 0.5 || m.right >= SHIP_W - TOOLTIP_MARGIN - 0.5)
                ++clampedCount;
            if (m.placement === 'below') ++flippedCount;

            check(notShrunk && insideX && insideY && arrowOk && withinCap,
                `[${lang}] ${a.label}: w=${m.w.toFixed(1)} h=${m.h.toFixed(1)} `
                + `x=[${m.left.toFixed(1)}, ${m.right.toFixed(1)}] `
                + `y=[${m.top.toFixed(1)}, ${m.bottom.toFixed(1)}] ${m.placement} `
                + `arrow=${m.arrow.toFixed(1)}`
                + (notShrunk ? '' : ' — SHRINK-WRAPPED')
                + (insideX ? '' : ' — OVERFLOWS HORIZONTALLY')
                + (insideY ? '' : ' — OVERFLOWS VERTICALLY')
                + (arrowOk ? '' : ' — ARROW OUTSIDE TIP')
                + (withinCap ? '' : ' — WIDER THAN max-width'));

            await unhover();
        }
      };

      // PASS 1 — the page exactly as it loads, with the settings panel CLOSED.
      // Packet, Codec, Crush and Rot ship OFF, so this pass is the one that
      // proves v1.12.0's narrowed `.panel.off .p-body` rule really does let a
      // disabled family's controls raise a tip. If that rule regresses to
      // blanket pointer-events:none, 18 anchors fall through to pass 2 and the
      // tally still reaches the full count — so the count is recorded here and
      // asserted separately.
      await sweep('shipped state');
      const measuredWhileOff = measured.size;

      // PASS 2 — every family enabled, and the clock slot swapped to Free.
      // CLOCK_SYNC_DIV and CLOCK_FREE_RATE share one slot and only one is ever
      // mounted, so neither pass alone reaches both.
      for (const fam of ['TAPE', 'CD', 'VINYL', 'PACKET', 'CODEC', 'CRUSH', 'ROT']) {
          const on = await page.$eval(`.en[data-param="${fam}_ENABLE"]`,
              el => el.classList.contains('on'));
          if (!on) await page.click(`.en[data-param="${fam}_ENABLE"]`);
      }
      await page.click('#clockModeSeg button[data-value="1"]');   // Free
      await page.waitForTimeout(80);
      await sweep('all families on / clock Free');

      // PASS 3 — back to Sync, for the division select in the other slot.
      await page.click('#clockModeSeg button[data-value="0"]');
      await page.waitForTimeout(80);
      await sweep('all families on / clock Sync');

      // PASS 4 (v1.14.0) — the settings popover OPEN. #lang-select and the
      // moved #help-toggle are anchors like any other, but they live in a panel
      // that ships hidden, so without this they are never hoverable and the
      // coverage assertion reports them forever.
      //
      // Excluding them instead would have been wrong twice over: they are real
      // tips a user can really raise, and they sit in the top-right corner of a
      // 900 px frame, which is precisely where the horizontal clamp and the
      // vertical flip both bite hardest. The panel is absolutely positioned and
      // changes no sibling's box, so opening it moves nothing already measured.
      // Swept LAST so the closed-state geometry above is measured against the
      // page as it loads.
      await page.click('#gear-btn');
      await page.waitForTimeout(60);
      const popoverOpen = await page.$eval('#settings-popover', el => !el.hidden).catch(() => false);
      check(popoverOpen,
          `[${lang}] the settings popover OPENS on a click — a panel that renders `
          + `but does not open would leave its controls unmeasurable, and a panel `
          + `that is merely present has already shipped pointer-dead in this suite`);
      await sweep('settings popover open');
      await page.keyboard.press('Escape');
      await page.waitForTimeout(60);

      const missed = anchors.filter(a => !measured.has(a.i)).map(a => a.label);
      check(measured.size === anchors.length,
          `[${lang}] every tip anchor was measured — ${measured.size}/${anchors.length}`
          + (missed.length ? ' — NEVER HOVERABLE: ' + missed.join(', ') : ''));

      // The disabled-family anchors must be reachable in the SHIPPED state, not
      // only after the user switches each family on. 18 of the anchors sit in
      // the four families that ship OFF; pass 1 has to have taken essentially
      // all of them. The tolerance is 4, not 2: the clock swap-slot pair, plus
      // v1.14.0's two controls inside the closed settings panel.
      check(measuredWhileOff >= anchors.length - 4,
          `[${lang}] disabled families still raise tips in the shipped state — `
          + `${measuredWhileOff}/${anchors.length} measured before any family was `
          + `switched on (only the clock swap-slot pair and the two controls `
          + `inside the closed settings panel may be deferred)`);

      // The gate has to actually FIRE, or it proved nothing. A run where no tip
      // ever reaches an edge is passing vacuously and the clamp is untested —
      // precisely the state a default-viewport run is in. Required PER
      // LANGUAGE: a French pass in which the clamp never engaged would say
      // nothing about French.
      check(clampedCount > 0,
          `[${lang}] the edge clamp actually engaged for ${clampedCount} control(s) — `
          + `a run where it never fires proves nothing about the clamp`);

      stats.set(lang, { clamped: clampedCount, flipped: flippedCount,
                        widest, tallest, worstRight, worstRightLabel,
                        measured: measured.size });
    }

    // French must actually BE French. Without this the sweep could run twice
    // over identical English text and report a confident, meaningless pass —
    // the same class of vacuity the clamp counter guards against.
    {
        const en = tipTextByLang.get('en') || {};
        const fr = tipTextByLang.get('fr') || {};
        const byProbe = new Map(anchors.map(a => [String(a.i), a.label]));
        const same = Object.keys(en).filter(k => fr[k]
            && en[k].t === fr[k].t && en[k].b === fr[k].b);
        check(same.length === 0,
            `every anchor's copy actually CHANGED between en and fr — `
            + `${Object.keys(en).length - same.length}/${Object.keys(en).length} differ`
            + (same.length ? ' — UNCHANGED: ' + same.map(k => byProbe.get(k) || k).join(', ') : ''));
    }

    // ── The Stage D deliverable, printed rather than only asserted ───────────
    console.log('\n   ── en vs fr geometry, measured at ' + SHIP_W + ' x ' + SHIP_H + ' ──');
    for (const lang of LANGS) {
        const st = stats.get(lang);
        if (!st) continue;
        console.log(`   ${lang}: ${st.measured} anchors  clamped ${st.clamped}  `
            + `flipped-below ${st.flipped}  widest ${st.widest.toFixed(1)}  `
            + `tallest ${st.tallest.toFixed(1)}  right-most ${st.worstRightLabel} @ `
            + `${st.worstRight.toFixed(1)} of ${SHIP_W} (limit ${SHIP_W - TOOLTIP_MARGIN})`);
    }
    {
        const e = stats.get('en'), f = stats.get('fr');
        if (e && f)
            console.log(`   French costs ${f.flipped - e.flipped >= 0 ? '+' : ''}`
                + `${f.flipped - e.flipped} vertical flip(s) and `
                + `${f.clamped - e.clamped >= 0 ? '+' : ''}${f.clamped - e.clamped} clamp(s), `
                + `and is ${(f.tallest - e.tallest).toFixed(1)} px taller at its tallest.`);
    }

    // Leave the page in English for the stress and layout stages below, so
    // their numbers stay comparable with every earlier release's.
    await page.evaluate(() => window.__setLanguage('en'));

    // ── Shrink-to-fit stress: the branch the measure-then-pin fix changed ────
    // The sweep above does NOT necessarily discriminate, and saying so is the
    // point. Most tips on this page are long enough to hit the 230 px
    // max-width, and the horizontal clamp's fixed point at this frame width is
    // left = 900 - 230 - 8 = 662, which leaves exactly 238 px to the right —
    // enough for the next 230 px tip. Where that holds, the collapse cannot
    // start and reverting showTip() to the naive measure-at-previous-offset
    // form leaves the assertions above still passing.
    //
    // That safety is a property of the COPY, not of the code. One short tip is
    // all it takes: a narrow tip is placed further right, and the next long tip
    // measured at that stale offset re-wraps into a ribbon that never recovers.
    // So this stage manufactures the condition — short copy on the right-most
    // control, then long copy back — and asserts the tip returns to its natural
    // width (pattern_probe_must_target_the_branch_the_fix_changed).
    // v1.14.0: the probe moved from 'Hover help' to 'Settings'. The stress needs
    // the RIGHT-MOST control on the page, and the "?" that used to be it is now
    // inside the settings panel — which ships hidden, so its rect would measure
    // 0x0 and the trap below would never be set up. The gear took over its
    // header slot and is the right-most control now.
    const stressProbe = anchors.find(a => a.label === 'Settings').i;
    const stress = await page.evaluate(async (i) => {
        const el = document.querySelector(`[data-tip-probe="${i}"]`);
        const tip = document.getElementById('tooltip');
        const original = el.getAttribute('data-tip');
        const long = 'A deliberately long description used to measure the '
                   + 'natural width of the tooltip surface at the shipping size.';

        const hover = (delay) => new Promise((resolve) => {
            el.dispatchEvent(new MouseEvent('mouseover', { bubbles: true }));
            setTimeout(() => {
                const r = tip.getBoundingClientRect();
                el.dispatchEvent(new MouseEvent('mouseout',
                    { bubbles: true, relatedTarget: document.body }));
                resolve({ w: +r.width.toFixed(1), left: +r.left.toFixed(1) });
            }, delay);
        });

        el.setAttribute('data-tip', 'Short.');
        const narrow = await hover(600);

        el.setAttribute('data-tip', long);
        const wide = await hover(600);

        el.setAttribute('data-tip', original);
        return { narrow, wide };
    }, stressProbe);

    console.log(`   stress: short copy -> w=${stress.narrow.w} at left=${stress.narrow.left}, `
        + `then long copy -> w=${stress.wide.w} at left=${stress.wide.left}`);

    check(stress.narrow.w > 0 && stress.narrow.left > SHIP_W - NATURAL_MAX_W,
        `the stress actually set up the trap — the short tip landed at left=`
        + `${stress.narrow.left}, past the ${SHIP_W - NATURAL_MAX_W} px point where `
        + `a stale offset leaves under ${NATURAL_MAX_W} px of room`);

    check(stress.wide.w >= NATURAL_MAX_W - 1,
        `after a narrow tip, a long tip still renders at its natural width — `
        + `${stress.wide.w} px of ${NATURAL_MAX_W} `
        + (stress.wide.w >= NATURAL_MAX_W - 1 ? ''
           : '— SHRINK-WRAPPED: showTip measured at the previous offset'));

    await unhover();

    // ── The layout must not have moved ──────────────────────────────────────
    // The settings cluster is a FOURTH flex child of .hdr rather than an
    // absolute overlay, which is only safe because .hdr is
    // justify-content:space-between with slack rather than a centred title.
    // Measured rather than reasoned about: the 900 x 740 frame is fixed by
    // setSize and a header that overflows or scrolls is exactly the kind of
    // change no build gate can see.
    //
    // v1.14.0 swapped the "?" for the gear IN THE SAME SLOT, at the same 22 px
    // circle geometry, so these numbers should be unchanged — which is the
    // claim being tested. #gear-btn is measured rather than #help-toggle: the
    // toggle moved inside the panel and its rect is 0x0 while that is closed.
    const layout = await page.evaluate(() => {
        const plugin = document.querySelector('.plugin').getBoundingClientRect();
        const hdr = document.querySelector('.hdr').getBoundingClientRect();
        const mark = document.querySelector('.wordmark').getBoundingClientRect();
        const band = document.querySelector('.preset-band').getBoundingClientRect();
        const right = document.querySelector('.hdr-right').getBoundingClientRect();
        const btn = document.getElementById('gear-btn').getBoundingClientRect();
        return {
            pluginW: +plugin.width.toFixed(2), pluginH: +plugin.height.toFixed(2),
            markRight: +mark.right.toFixed(2),
            bandLeft: +band.left.toFixed(2), bandRight: +band.right.toFixed(2),
            rightLeft: +right.left.toFixed(2), rightRight: +right.right.toFixed(2),
            btnLeft: +btn.left.toFixed(2), btnRight: +btn.right.toFixed(2),
            hdrRight: +hdr.right.toFixed(2),
            docScrollW: document.documentElement.scrollWidth,
            docScrollH: document.documentElement.scrollHeight,
        };
    });

    check(layout.pluginW === SHIP_W && layout.pluginH === SHIP_H,
        `.plugin still renders ${SHIP_W} x ${SHIP_H} — got ${layout.pluginW} x ${layout.pluginH}`);
    check(layout.docScrollW <= SHIP_W && layout.docScrollH <= SHIP_H,
        `the page does not scroll — ${layout.docScrollW} x ${layout.docScrollH}`);

    // The four header children must still read left-to-right with no overlap.
    // space-between redistributes when a child is added, so this is the
    // assertion that the redistribution stayed sane rather than collapsing the
    // preset band into the brand block.
    check(layout.markRight <= layout.bandLeft
        && layout.bandRight <= layout.rightLeft
        && layout.rightRight <= layout.btnLeft,
        `the header's four children do not overlap — wordmark|${layout.markRight} `
        + `band|${layout.bandLeft}..${layout.bandRight} `
        + `brand|${layout.rightLeft}..${layout.rightRight} gear|${layout.btnLeft}`);

    check(layout.btnRight <= layout.hdrRight + 0.5,
        `the gear is inside the header's content box — its right edge is `
        + `${layout.btnRight}, the header ends at ${layout.hdrRight}`);

    console.log(`   gear right edge at ${layout.btnRight} of ${SHIP_W} `
        + `— the right-most control, which is why the clamp matters here`);

    await browser.close();
    server.close();
    fs.rmSync(root, { recursive: true, force: true });

    console.log(failed === 0 ? '\n== ALL CHECKS PASSED ==' : `\n== ${failed} CHECK(S) FAILED ==`);
    process.exit(failed);
})().catch(e => { console.error(e); process.exit(1); });
