/*
   This file is part of O-Tapestop, an Ouaricon Audio plugin.
   Copyright (C) 2026  Ouaricon Audio

   SPDX-License-Identifier: AGPL-3.0-or-later

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
/*
  ==============================================================================

    ui_tooltip_clamp_check.js
    O-Tapestop — hover-help verification AT THE SHIPPING VIEWPORT (v1.4.0).

    Ported from O-ReverseDelay v1.1.0, which is the only version of this check
    that has actually caught the bug it exists for.

    WHY A DEDICATED FILE. A static read of app.js can prove that showTooltip()
    releases, measures and PINS its width before applying `left`, and that
    `left` is clamped into [MARGIN, innerWidth - width - MARGIN]. It cannot
    prove the clamp ever FIRES, because that depends entirely on the viewport.
    At Chromium's default 1280 px there is room to the right of every control,
    the clamp never engages, and "the tooltip fits" passes while the real
    860 px window overflows (pattern_tooltip_clamp_gate_viewport_sensitive).

    So this drives the REAL page — same HTML, same CSS, same app.js, only
    js/juce/index.js swapped for the stub — in a browser pinned to the exact
    shipping size, and measures the rendered rectangle.

    WHY IT MATTERS PARTICULARLY HERE. v1.4.0 put the "?" toggle 16 px from the
    right edge of an 860 px frame, making it the right-most control on the
    page. That is precisely the geometry that surfaced
    pattern_fixed_tooltip_shrink_to_fit_edge in O-MultiBandCompressor v1.4.1:
    every other control sat inboard enough that the squeeze was too small to
    notice, so it shipped unseen. Build, auval and pluginval are all blind to
    it — it is pure WebView layout.

    What is asserted, for EVERY control carrying a tip:
      1. The tip is at its natural width, not shrink-wrapped — a fixed-position
         box with `left` set and `width:auto` collapses into the space left to
         its right, turning a 230 px tip into a ~70 px ribbon.
      2. left >= MARGIN AND right <= innerWidth - MARGIN. Asserting width alone
         is what let the O-MBC bug through; both edges must be checked.
      3. top >= 0 and bottom <= innerHeight — the above/below flip really does
         keep it on screen.
      4. The arrow still points inside the tip after clamping.

    COVERAGE. The centre panel is three mode-switched panes and every duration
    control is a Sync/Free time-slot, so a control hidden by either swap is not
    hoverable and its tip would stay unverified forever. All six MODE x
    SYNC_MODE combinations are swept and total anchor coverage is asserted at
    the end.

    Anchors are stamped with data-tip-probe at run time rather than enumerated
    by id: several legitimate anchors (the ratio cell, the envelope plate, the
    four division select-cells) carry no id, and an id-based enumeration would
    skip them silently while the tally still looked complete.

    The dwell is AWAITED, not slept past by a fixed guess — measuring before
    .visible lands reads a zero-size rect and passes vacuously.

    Usage:  node plugins/O-Tapestop/tests/ui_tooltip_clamp_check.js
    Exit code = number of failed assertions (0 = all pass, 77 = could not run).
    Requires Playwright (`npx playwright install chromium` once).

  ==============================================================================
*/

'use strict';

const fs   = require('fs');
const os   = require('os');
const path = require('path');
const http = require('http');
const vm   = require('vm');   // v1.5.0 — the anchor count is derived from js/i18n.js

const pluginRoot = path.resolve(__dirname, '..');
const publicDir  = path.join(pluginRoot, 'Source', 'ui', 'public');

// Kept in sync with PluginEditor.cpp's setSize and styles.css — and
// cross-checked against both below, so a resize that forgets this file fails
// loudly rather than measuring a stale viewport
// (pattern_test_fixture_mirrors_drift_silently). The PLAN Locked Decisions
// say never resize, which makes a drift here a bug in its own right.
const SHIP_W = 860;
const SHIP_H = 580;

// app.js / styles.css constants — mirrored, and cross-checked below.
const TOOLTIP_MARGIN = 8;
// .tooltip's max-width. v1.5.0 PARSES this out of the plugin's own CSS rather
// than hard-coding it, because the cap differs per plugin — 230 here and in
// O-ReverseDelay and O-Bitrot, 240 in O-Octagon, 220 in O-Polystutter — and a
// file that mirrors one plugin's number would mis-assert the moment it is
// pointed at another. It is the cap French has to wrap INSIDE, so it is
// load-bearing for every assertion below.
//
// The literal survives as the drift guard, not as the source of truth: the
// parsed value drives the measurements, and the assertion below fails if the
// two disagree (pattern_test_fixture_mirrors_drift_silently).
const DOCUMENTED_MAX_W = 230;

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
// tests/ui-stub/serve-stub.sh does it. Unlike O-ReverseDelay there is no
// separate preset-manager copy step — O-Tapestop vendors that module inside
// Source/ui/public/modules/, which the resource provider serves at the same
// path, so the tree copy already covers it.
function buildRoot() {
    const root = fs.mkdtempSync(path.join(os.tmpdir(), 'otap-tip-'));
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
    console.log('== O-Tapestop ui_tooltip_clamp_check ==');
    console.log(`   viewport pinned to the SHIPPING size ${SHIP_W} x ${SHIP_H}\n`);

    // ── Guard the mirrored constants against drift ──────────────────────────
    const appJs     = fs.readFileSync(path.join(publicDir, 'js', 'app.js'), 'utf8');
    const css       = fs.readFileSync(path.join(publicDir, 'css', 'styles.css'), 'utf8');
    const html      = fs.readFileSync(path.join(publicDir, 'index.html'), 'utf8');
    const editorCpp = fs.readFileSync(path.join(pluginRoot, 'Source', 'PluginEditor.cpp'), 'utf8');

    check(new RegExp(`TOOLTIP_MARGIN\\s*=\\s*${TOOLTIP_MARGIN}\\b`).test(appJs),
        `TOOLTIP_MARGIN in app.js is ${TOOLTIP_MARGIN} (this file mirrors it)`);
    // Parsed from the .tooltip RULE specifically, not from the first max-width
    // in the file — .settings-popover and .preset-name carry widths of their own
    // and a loose scan would silently measure against one of those.
    const tipRule = css.match(/\.tooltip\s*\{[\s\S]*?\}/);
    const capMatch = tipRule && tipRule[0].match(/max-width:\s*(\d+(?:\.\d+)?)px/);
    const NATURAL_MAX_W = capMatch ? parseFloat(capMatch[1]) : NaN;

    check(Number.isFinite(NATURAL_MAX_W),
        `.tooltip max-width parsed from styles.css — got ${capMatch ? capMatch[1] + 'px' : 'NOTHING'}`);
    check(NATURAL_MAX_W === DOCUMENTED_MAX_W,
        `.tooltip max-width is the documented ${DOCUMENTED_MAX_W}px — parsed ${NATURAL_MAX_W}px. `
        + `If the cap moved deliberately, move DOCUMENTED_MAX_W with it and re-read the `
        + `French sweep below: French wraps INSIDE this cap, so changing it changes every `
        + `tip height and therefore every vertical-flip decision`);
    check(new RegExp(`setSize\\s*\\(\\s*${SHIP_W}\\s*,\\s*${SHIP_H}\\s*\\)`).test(editorCpp),
        `editor setSize is ${SHIP_W} x ${SHIP_H} — the viewport measured below`);

    // The expected anchor count is PARSED from the page, never typed. A literal
    // here fails for the wrong reason every time a control gains a tip, and the
    // hand edit that follows is how a fixture starts describing the release
    // before it (pattern_test_fixture_mirrors_drift_silently).
    // v1.5.0 moved the source of truth. This used to count `data-tip=` literals
    // in index.html; the copy has since left the markup, so that count is now
    // ZERO and the assertion would pass vacuously against nothing. TIP_BINDINGS
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
    // 860 px window overflows. That mistake is the failure this whole file
    // exists to catch, and the viewport assertion below makes it loud.
    const page = await browser.newPage({ viewport: { width: SHIP_W, height: SHIP_H } });

    const consoleErrors = [];
    page.on('console', m => { if (m.type() === 'error') consoleErrors.push(m.text()); });
    page.on('pageerror', e => consoleErrors.push(String(e)));

    await page.goto(`http://127.0.0.1:${port}/index.html`, { waitUntil: 'networkidle' });

    // The page must actually be alive: a TDZ throw out of module evaluation
    // kills every control while leaving the HTML looking correct
    // (pattern_module_toplevel_init_tdz). Readouts are born as em-dashes and
    // are JS-owned, so a populated readout proves the module ran.
    const knobBlock = appJs.match(/const KNOB_IDS = \[([\s\S]*?)\];/);
    const expectedReadouts = knobBlock
        ? [...knobBlock[1].matchAll(/"([A-Za-z0-9_]+)"/g)].length
        : -1;
    const boundReadouts = await page.$$eval('.knob-value',
        els => els.filter(e => e.textContent.trim() !== '—' && e.textContent.trim() !== '').length);

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

    // ── Stamp the anchors ───────────────────────────────────────────────────
    // By index, not by id. Several legitimate anchors carry no id — the ratio
    // cell, the envelope plate, the four division select-cells — and an
    // id-based enumeration would drop them while the tally still read as full
    // coverage. The label is the tip's own title, which is what a failure needs
    // to be actionable.
    const anchors = await page.evaluate(() => {
        const els = [...document.querySelectorAll('[data-tip]')];
        els.forEach((el, i) => el.setAttribute('data-tip-probe', String(i)));
        return els.map((el, i) => ({
            i,
            label: el.id || el.getAttribute('data-tip-title') || el.className || el.tagName,
        }));
    });

    check(anchors.length === expectedAnchors,
        `every data-tip in index.html is present in the DOM — `
        + `${anchors.length}/${expectedAnchors}`);

    // ── The help layer must ship OFF ────────────────────────────────────────
    // The processor defaults tooltipsEnabled to false and the page PULLS it, so
    // a fresh instance shows an unlit "?" and a silent layer. Asserting the
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

    // ── v1.6.0: this assertion was REWRITTEN A SECOND TIME, and it still
    //    guards the same rule ─────────────────────────────────────────────────
    // Through v1.4.0 the toggle was a "?" circle and this pinned the literal
    // "?". v1.5.0 moved it into the settings popover, where it reads On/Off, so
    // the caption became script-written and the check moved to the data-on /
    // data-off attributes AUTHORED ON THE ELEMENT.
    //
    // v1.6.0 retires those attributes. An attribute holds ONE string, so on a
    // page with two languages the off face would have been restored in English
    // the moment the user picked Français. The caption now comes from a KEY —
    // ui.on / ui.off in LABELS — written through setLabel(), which makes the
    // element a [data-i18n] element that the language sweep owns.
    //
    // What actually matters is unchanged across all three rewrites: the COPY
    // must not come from a literal in the JS. That is the rule
    // pattern_js_state_updater_overwrites_html_labels exists for, and it is how
    // O-MBC's band glyphs became "Off Off Off". Under canon v2 the guard is
    // STRONGER than the attribute pair was, because applyLabel() writes
    // textContent and dataset.label together: the mirror is what a state
    // updater reads, so a caption that disagrees with it means someone wrote
    // the element behind i18n's back.
    check(initial.key !== null,
        `the toggle's caption is a KEY, not a JS literal — data-i18n="${initial.key}"`);
    check(initial.key === 'ui.off',
        `the shipped-OFF toggle carries the OFF key — data-i18n="${initial.key}"`);
    check(initial.mirror === initial.caption,
        `dataset.label mirrors the rendered caption — mirror "${initial.mirror}", `
        + `rendered "${initial.caption}"`);

    const dwell = Number((appJs.match(/TOOLTIP_DELAY_MS\s*=\s*(\d+)/) || [])[1] || 350);

    // ── With the layer OFF, only the exempt anchor may show ─────────────────
    // The toggle carries data-tip-always so the one control that can turn help
    // back on is never the one control unable to explain itself. Everything
    // else must stay silent, or the toggle is decoration.
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

    // v1.5.0: probed on the GEAR rather than the toggle. The toggle moved inside
    // the settings panel, which ships hidden, so it cannot raise a tip until the
    // panel is opened — and the control that has to keep explaining itself while
    // help is off is now the one that REACHES the panel. Both carry
    // data-tip-always; this is the one a user can reach first.
    await hoverProbe(anchors.find(a => a.label === 'gear-btn').i);
    check(await tipVisible(),
        'with help OFF, the gear\'s own tip still shows (data-tip-always)');
    await unhover();

    // NOTE the label: this page's anchor labels prefer the element id, but the
    // Mix anchor is a .knob-cell WRAPPER with no id of its own, so its label
    // falls back to the tip title. Captured before the language loop below, so
    // it is the English one.
    const mixProbe = anchors.find(a => a.label === 'Mix');
    await hoverProbe(mixProbe.i);
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
    // The key SWAPPED with the state. That is the v2 contract's whole point:
    // the element now carries the ON key, so the next language switch
    // re-renders the ON face in the new language rather than restoring a
    // stale one.
    check(afterClick.key === 'ui.on',
        `clicking swaps the caption's KEY to the ON entry — data-i18n="${afterClick.key}"`);
    check(afterClick.mirror === afterClick.caption,
        `dataset.label still mirrors the rendered caption after the state change — `
        + `mirror "${afterClick.mirror}", rendered "${afterClick.caption}"`);
    check(afterClick.persisted === true,
        `the preference reached the native bridge — getTooltipsEnabled() = ${afterClick.persisted}`);

    // Close it again, so the sweep below starts from the page as it ships.
    await page.keyboard.press('Escape');
    await page.waitForTimeout(60);

    // ══════════════════════════════════════════════════════════════════════
    // v1.5.0 — THE SWEEP IS PARAMETERISED BY LANGUAGE
    // ══════════════════════════════════════════════════════════════════════
    //
    // Not duplicated: one process, one page load, all six MODE x SYNC passes run
    // once per language. Duplicating the file would have produced two copies
    // free to drift, which is the failure this repo has already paid for twice.
    //
    // WHY IT HAS TO BE RE-RUN AT ALL. Assertion 3 is the language-sensitive one.
    // French runs longer than English, and .tooltip's max-width is a hard cap —
    // so a longer string does not get wider, it WRAPS TO MORE LINES. `height`
    // grows, `top = anchor.top - height - MARGIN` moves further up, and the
    // above/below flip has to catch what no longer fits above. Assertion 2 is
    // capped by max-width and cannot move.
    //
    // 860 x 580 is the SMALLEST frame in the suite, so if French geometry is
    // going to break anywhere it is here.
    //
    // The language is driven through window.__setLanguage — applyI18n, exposed
    // by the canonical block for exactly this. It rewrites every anchor's two
    // attributes synchronously and fires NO `change` event.
    //
    // Every failure is LABELLED with its language. Without that a French-only
    // failure reads as a mysterious regression in a file that never mentions
    // French. The anchor LABELS stay stable: this page's labels prefer the
    // element id, so they do not localize with the copy.
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

      const sweep = async (modeLabel) => {
        for (const a of anchors) {
            if (measured.has(a.i)) continue;

            const shown = await page.$eval(`[data-tip-probe="${a.i}"]`, el => {
                const r = el.getBoundingClientRect();
                return r.width > 0 && r.height > 0
                    && getComputedStyle(el).visibility !== 'hidden'
                    && !el.disabled;
            }).catch(() => false);

            if (!shown) continue;   // hidden by a pane or slot swap — a later pass takes it
            measured.add(a.i);

            await hoverProbe(a.i);

            if (!(await tipVisible())) {
                check(false, `[${lang}] ${a.label} [${modeLabel}]: tip became visible within ${dwell} ms dwell`);
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

      // All six MODE x SYNC_MODE combinations. MODE swaps the centre pane
      // (Stop / Scratch / Motion) and SYNC_MODE swaps every time-slot between
      // its division select and its free knob — two independent nested toggles,
      // so neither sweep alone reaches every anchor.
      const MODES = [['#seg-mode-stop', 'stop'],
                     ['#seg-mode-scratch', 'scratch'],
                     ['#seg-mode-cont', 'motion']];
      const SYNCS = [['#seg-sync-sync', 'sync'], ['#seg-sync-free', 'free']];

      for (const [modeSel, modeName] of MODES) {
          await page.click(modeSel);
          for (const [syncSel, syncName] of SYNCS) {
              await page.click(syncSel);
              await page.waitForTimeout(50);
              await sweep(`${modeName}/${syncName}`);
          }
      }

      // Leave the page as it loaded.
      await page.click('#seg-sync-sync');
      await page.click('#seg-mode-stop');
      await page.waitForTimeout(50);

      // PASS 7 (v1.5.0) — the settings popover OPEN. #lang-select and the moved
      // #help-toggle are anchors like any other, but they live in a panel that
      // ships hidden, so without this they are never hoverable and the coverage
      // assertion reports them forever.
      //
      // Excluding them instead would have been wrong twice over: they are real
      // tips a user can really raise, and they sit in the top-right corner of an
      // 860 px frame — the smallest in the suite, and precisely where the
      // horizontal clamp and the vertical flip both bite hardest. The panel is
      // absolutely positioned and changes no sibling's box, so opening it moves
      // nothing already measured. Swept LAST so the closed-state geometry above
      // is measured against the page as it loads.
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
          `[${lang}] every tip anchor was measured across all six MODE x SYNC `
          + `combinations and the open settings popover — `
          + `${measured.size}/${anchors.length}`
          + (missed.length ? ' — NEVER VISIBLE: ' + missed.join(', ') : ''));

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

    const worstRight = (stats.get('en') || {}).worstRight ?? -1;
    const worstRightLabel = (stats.get('en') || {}).worstRightLabel ?? '-';

    console.log(`\n   right-most tip: ${worstRightLabel} ends at ${worstRight.toFixed(1)} `
        + `of ${SHIP_W} (limit ${SHIP_W - TOOLTIP_MARGIN})`);

    // ── Shrink-to-fit stress: the branch the measure-then-pin fix changed ────
    // The sweep above does NOT discriminate, and saying so is the point.
    //
    // Every tip on this page happens to be long enough to hit the 230 px
    // max-width, and the horizontal clamp's fixed point at this frame width is
    // left = 860 - 230 - 8 = 622, which leaves exactly 238 px to the right —
    // enough for the next 230 px tip. So the collapse cannot start, and
    // reverting showTooltip() to the naive measure-at-previous-offset form
    // leaves every assertion above still passing. Verified by doing exactly
    // that: 0 failures either way (pattern_probe_must_target_the_branch_the
    // _fix_changed).
    //
    // That safety is a property of the COPY, not of the code. One short tip is
    // all it takes: a narrow tip is placed further right, and the next long tip
    // measured at that stale offset re-wraps into a ribbon that never recovers.
    // So this stage manufactures the condition — short copy on the right-most
    // control, then long copy back — and asserts the tip returns to its natural
    // width. With the fix it does; with the naive version it renders ~70 px.
    // v1.5.0: the probe moved from 'help-toggle' to 'gear-btn'. The stress needs
    // the RIGHT-MOST control on the page, and the "?" that used to be it is now
    // inside the settings panel — which ships hidden, so its rect would measure
    // 0x0 and the trap below would never be set up. The gear took over its
    // absolute box and is the right-most control now.
    const stressProbe = anchors.find(a => a.label === 'gear-btn').i;
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
           : '— SHRINK-WRAPPED: showTooltip measured at the previous offset'));

    await unhover();

    // ── The layout must not have moved ──────────────────────────────────────
    // The settings cluster is position:absolute precisely so it costs the centred
    // header nothing. Measured rather than reasoned about: the frame is a PLAN
    // Locked Decision and a header shifted by a few px is exactly the kind of
    // change no build gate can see.
    //
    // v1.5.0 swapped the "?" for the gear IN THE SAME absolute box, at the same
    // 22 px circle geometry, so these numbers should be unchanged — which is the
    // claim being tested. #gear-btn is measured rather than #help-toggle: the
    // toggle moved inside the panel and its rect is 0x0 while that is closed.
    const layout = await page.evaluate(() => {
        const frame = document.querySelector('.frame').getBoundingClientRect();
        const title = document.querySelector('.title').getBoundingClientRect();
        const btn   = document.getElementById('gear-btn').getBoundingClientRect();
        return {
            frameW: +frame.width.toFixed(2), frameH: +frame.height.toFixed(2),
            titleCentre: +(title.left + title.width / 2).toFixed(2),
            frameCentre: +(frame.left + frame.width / 2).toFixed(2),
            btnRight: +btn.right.toFixed(2),
            docScrollW: document.documentElement.scrollWidth,
            docScrollH: document.documentElement.scrollHeight,
        };
    });

    check(layout.frameW === SHIP_W && layout.frameH === SHIP_H,
        `.frame still renders ${SHIP_W} x ${SHIP_H} — got ${layout.frameW} x ${layout.frameH}`);
    check(Math.abs(layout.titleCentre - layout.frameCentre) < 1.0,
        `the title is still centred in the frame — title ${layout.titleCentre} `
        + `vs frame ${layout.frameCentre}`);
    check(layout.docScrollW <= SHIP_W && layout.docScrollH <= SHIP_H,
        `the page does not scroll — ${layout.docScrollW} x ${layout.docScrollH}`);
    check(layout.btnRight <= SHIP_W,
        `the gear is inside the frame — its right edge is ${layout.btnRight}`);

    console.log(`   gear right edge at ${layout.btnRight} of ${SHIP_W} `
        + `— the right-most control, which is why the clamp matters here`);

    await browser.close();
    server.close();
    fs.rmSync(root, { recursive: true, force: true });

    console.log(failed === 0 ? '\n== ALL CHECKS PASSED ==' : `\n== ${failed} CHECK(S) FAILED ==`);
    process.exit(failed);
})().catch(e => { console.error(e); process.exit(1); });
