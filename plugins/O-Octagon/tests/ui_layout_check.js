/*
   This file is part of O-Octagon, an Ouaricon Audio plugin.
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

    ui_layout_check.js
    O-Octagon — both screens, MEASURED on the rendered page at 1100 x 720.

    TWENTY-EIGHT sections. 1-10 are the Room plan (3.1); 11-18 are the Venue
    screen (3.2); 19-27 are scenes, meters, the field and the elevation (3.3);
    28 is Q5's in-flight guard deadline (4.2).
    This file is the SOLE evidence for UI-02 criteria 1, 3, 4 and 5 and
    for UI-01 criteria 1 and 2, and it runs TWICE per phase: once against the
    ui-stub before that phase's C++ exists, and again afterwards, to prove the
    resource provider serves the same tree the stub did.

    ── IT FAILS WHEN PLAYWRIGHT IS MISSING. IT MUST NOT SKIP (PLAN-3.1 P49) ──
    The O-ReverseDelay precedent prints SKIP and exits 77 when it cannot resolve
    Playwright, and that was right THERE: the browser run was supplementary, and
    a static sibling already covered the mechanism. Here it is the only evidence
    four criteria have. A SKIP that reads green would let Phase 3.1 close UI-02
    against nothing, which is the exact class of vacuity this project has caught
    five times (the width-wired-to-nothing sweep, the zipper probe with no
    liveness gate, the worktree with no tracked files, NC3 at 2.2, NC6 at 2.3).
    So: print the install command, and exit non-zero.

    ── WHY THINGS ARE MEASURED AND NOT COMPUTED ─────────────────────────────
    Every assertion about size reads a rendered box. Row arithmetic that fits on
    paper is what carried 93.5 px of phantom slack through five releases of
    O-ReverseDelay (pattern_flex1_container_slack_invisible_to_row_sum), and a
    flex:1 container's slack is invisible to arithmetic on the row sum.

    ── WHY THINGS ARE COMPARED TO THE PAYLOAD AND NOT TO LITERALS ───────────
    Sections 2, 3 and 7 assert against the values the stub RETURNED, re-read
    from the stub at assertion time. A mirrored table of expected numbers is
    pattern_test_fixture_mirrors_drift_silently, which the precedent's own
    fixtures hit five separate times.

    Usage:  node plugins/O-Octagon/tests/ui_layout_check.js
    Exit code = number of failed assertions (0 = all pass).
    Requires Playwright — `npx playwright install chromium` once.

  ==============================================================================
*/

'use strict';

const fs   = require('fs');
const os   = require('os');
const path = require('path');
const http = require('http');

const pluginRoot = path.resolve(__dirname, '..');
const publicDir  = path.join(pluginRoot, 'Source', 'ui', 'public');

// The shipping frame. Cross-checked below against styles.css always, and
// against PluginEditor.cpp's setSize once that file exists — so a resize that
// forgets this gate fails loudly instead of measuring a stale viewport.
const SHIP_W = 1100;
const SHIP_H = 720;

// app.js's status poll. Mirrored here only to derive a wait budget, and
// cross-checked against the source below.
const STATUS_POLL_MS = 500;
const SETTLE_MS = STATUS_POLL_MS * 6;

const MIME = {
    '.html': 'text/html; charset=utf-8',
    '.css':  'text/css; charset=utf-8',
    '.js':   'application/javascript; charset=utf-8',
};

let failed = 0;
let section = '';

function head(n, title) {
    section = `${n}`;
    console.log(`\n-- section ${n}: ${title}`);
}

function check(cond, desc) {
    console.log(`  ${cond ? 'PASS' : 'FAIL'}: [${section}] ${desc}`);
    if (!cond) ++failed;
}

const near = (a, b, tol) => Math.abs(a - b) <= tol;

// ── Build the served tree: production page + the bridge stub ────────────────
// Byte-identical to Source/ui/public except js/juce/index.js, exactly as
// tests/ui-stub/serve-stub.sh does it. What renders here is what renders in the
// WebView — modulo WKWebView itself, which no automated gate in this repo can
// stand in for and which is why Task 12's gate 13 is a human launch-and-look.
function buildRoot() {
    const root = fs.mkdtempSync(path.join(os.tmpdir(), 'ooct-layout-'));
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

// Resolve Playwright from wherever it already lives. Deliberately installs
// NOTHING — a verification script that mutates the machine to make itself pass
// is not a verification script.
function resolvePlaywright() {
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
}

// Drive one control with a REAL key event, so the page's own `input` listener
// runs and the gesture brackets open and close exactly as they do under a
// finger.
//
// TWO TRAPS, BOTH FOUND BY THIS PROBE FAILING RATHER THAN BY INSPECTION:
//
//   1. A control already sitting at its MAXIMUM is stepped DOWN instead. The
//      eight weights default to 1.0, and ArrowRight on a control at its maximum
//      emits nothing at all — RESEARCH-3.1 F3's compare-before-notify trap,
//      arriving from the DOM side rather than from JUCE's.
//
//   2. PageUp/PageDown, NOT ArrowRight/ArrowLeft. One arrow step is one `step`
//      attribute — 0.0001 normalised for the ten column controls — which is
//      BELOW the display resolution of every readout on the page: 0.0001 of
//      srcZ's 10 m range is 0.001 m, and the readout carries two decimals. A
//      probe built on arrow keys therefore reports sixteen "frozen" readouts on
//      a page whose echo is working perfectly. That is a vacuous FAILURE rather
//      than a vacuous pass, but it is the same defect class: the probe was not
//      driving the thing it claimed to measure. Chromium steps a range input by
//      max(step, range/10) on PageUp/PageDown, which is a visible move for all
//      eighteen.
// v1.8.0: a control on a hidden group panel (Position | Motion) is revealed by clicking its
// panel's tab first; a checkbox is toggled with Space and a <select> stepped with ArrowDown
// (ArrowUp on its last option). Every other control is still a range and takes PageUp/PageDown.
async function nudge(page, id) {
    const el = await page.$(`#ctl-${id}`);
    if (el === null) return false;
    await page.evaluate(id => {
        const e = document.getElementById(`ctl-${id}`);
        const panel = e.closest('[data-tab-panel]');
        if (panel !== null && panel.hidden) document.getElementById(panel.dataset.tabButton).click();
    }, id);
    // Seed's cell is hidden unless Path is Drift (it replaces Phase there). Reveal it through the
    // page's own rule — select Drift — and put Orbit back afterwards.
    const cellHidden = await el.evaluate(e => e.closest('.cell')?.hidden === true);
    const priorPath = cellHidden ? await page.$eval('#ctl-motionPath', e => String(e.selectedIndex)) : null;
    if (cellHidden) await page.selectOption('#ctl-motionPath', { label: 'Drift' });
    await el.focus();
    const kind = await el.evaluate(e => e.tagName === 'SELECT' ? 'select' : e.type);
    if (kind === 'checkbox') {
        await page.keyboard.press('Space');
    } else if (kind === 'select') {
        // selectOption dispatches `change` in every engine; a keyboard step on a native <select>
        // is popup-driven on macOS and fires nothing headless.
        const next = await el.evaluate(e => String(e.selectedIndex >= e.options.length - 1 ? e.selectedIndex - 1 : e.selectedIndex + 1));
        await page.selectOption(`#ctl-${id}`, { value: next });
    } else {
        const v = await el.evaluate(e => Number(e.value));
        await page.keyboard.press(v >= 0.999 ? 'PageDown' : 'PageUp');
    }
    // Back to what Path WAS (its own nudge may have just moved it) — not to a fixed choice.
    if (cellHidden) await page.selectOption('#ctl-motionPath', { value: priorPath });
    return true;
}

(async () => {
    console.log('== O-Octagon ui_layout_check ==');
    console.log(`   viewport pinned to the SHIPPING size ${SHIP_W} x ${SHIP_H}`);

    const appJs = fs.readFileSync(path.join(publicDir, 'js', 'app.js'), 'utf8');
    const css   = fs.readFileSync(path.join(publicDir, 'css', 'styles.css'), 'utf8');

    head(0, 'this file has not drifted from the page it measures');

    // ── P84 / D27 — THE STAMP IS MACHINE-PRODUCED, AND THAT IS THE POINT ──
    // Gate 4's claim across Stage 3 is an ORDERING claim: the stub render ran
    // BEFORE the integration C++ existed, which is what makes the pre-
    // integration half of every layout assertion evidence rather than
    // decoration. At 3.2 verify that claim was found to rest on a stamp
    // TRANSCRIBED from console output — a repo-wide search for the recorded
    // time hits only planning prose and never a machine-produced artifact.
    //
    // One line fixes it for every phase after this one. It cannot repair 3.2's
    // record and does not pretend to.
    console.log(`  STAMP: [0] ui_layout_check run at ${new Date().toISOString()}`);

    check(new RegExp(`STATUS_POLL_MS\\s*=\\s*${STATUS_POLL_MS}\\b`).test(appJs),
        `app.js STATUS_POLL_MS is ${STATUS_POLL_MS} (this file mirrors it)`);
    check(new RegExp(`width:\\s*${SHIP_W}px`).test(css) && new RegExp(`height:\\s*${SHIP_H}px`).test(css),
        `styles.css html/body is ${SHIP_W} x ${SHIP_H}`);

    // PluginEditor.cpp does not exist on the PRE-INTEGRATION run — Tasks 1-5
    // complete before Task 6 writes a line of C++, and that ordering IS UI-02
    // criterion 6. On the post-integration re-run (Task 10) it exists and the
    // setSize pair is diffed against this file's constants.
    const editorCpp = path.join(pluginRoot, 'Source', 'PluginEditor.cpp');
    if (fs.existsSync(editorCpp)) {
        const src = fs.readFileSync(editorCpp, 'utf8');
        check(new RegExp(`setSize\\s*\\(\\s*${SHIP_W}\\s*,\\s*${SHIP_H}\\s*\\)`).test(src),
            `PluginEditor.cpp setSize is ${SHIP_W} x ${SHIP_H} — the viewport measured below`);
    } else {
        console.log('  NOTE: [0] PluginEditor.cpp absent — this is the PRE-INTEGRATION run (UI-02/6)');
    }

    const pw = resolvePlaywright();
    if (pw === null) {
        console.log('\n  FAIL: playwright is not resolvable, and this gate does not skip.');
        console.log('        Install it with:  npx playwright install chromium');
        console.log('        UI-02 criteria 1, 3, 4 and 5 have NO other evidence (PLAN-3.1 P49).');
        process.exit(1);
    }
    const { chromium } = pw;

    const root = buildRoot();
    const { server, port } = await serve(root);
    const browser = await chromium.launch();

    // `viewport`, NOT `viewportSize` — the latter is the getter's name and is
    // silently ignored as an option, leaving Chromium's 1280 x 720 default. At
    // 1280 the overflow assertions in section 8 pass while the real 1100 px
    // window overflows (pattern_tooltip_clamp_gate_viewport_sensitive).
    const context = await browser.newContext({ viewport: { width: SHIP_W, height: SHIP_H }, deviceScaleFactor: 1 });
    const page = await context.newPage();

    const consoleErrors = [];
    page.on('console', m => { if (m.type() === 'error') consoleErrors.push(m.text()); });
    page.on('pageerror', e => consoleErrors.push(String(e)));

    await page.goto(`http://127.0.0.1:${port}/index.html`, { waitUntil: 'networkidle' });
    await page.waitForFunction(() => document.getElementById('readout-envelope').textContent !== '—',
        null, { timeout: 10000 }).catch(() => {});

    // ── 1 ────────────────────────────────────────────────────────────────────
    head(1, 'the page is ALIVE — zero console errors, all 28 controls bound');

    check(consoleErrors.length === 0,
        'no console errors on load' + (consoleErrors.length ? ` — ${consoleErrors[0]}` : ''));

    // The expected id list is PARSED from app.js rather than typed here: a
    // literal 17 is a fixture that stops describing the page the moment a
    // parameter is added, and this repo has watched exactly that happen three
    // times (pattern_test_fixture_mirrors_drift_silently).
    const idBlock = appJs.match(/const PARAM_IDS = \[([\s\S]*?)\];/);
    const paramIds = idBlock ? [...idBlock[1].matchAll(/"([A-Za-z0-9_]+)"/g)].map(m => m[1]) : [];
    check(paramIds.length === 28, `PARAM_IDS parsed from app.js — ${paramIds.length} ids (expect 28)`);

    const present = await page.evaluate(
        ids => ids.filter(id => document.getElementById(`ctl-${id}`) !== null).length, paramIds);
    check(present === paramIds.length, `all ${paramIds.length} ctl-<id> elements present — got ${present}`);

    // A populated readout is proof the module RAN. A TDZ throw out of module
    // evaluation leaves the HTML looking correct and every readout at the
    // em-dash the HTML authored (pattern_module_toplevel_init_tdz).
    const bound = await page.evaluate(
        ids => ids.filter(id => {
            const el = document.getElementById(`val-${id}`);
            return el !== null && el.textContent.trim() !== '—' && el.textContent.trim() !== '';
        }).length, paramIds);
    check(bound === paramIds.length, `app.js ran and bound every readout — ${bound}/${paramIds.length}`);

    // ── 2 ────────────────────────────────────────────────────────────────────
    head(2, 'plan proportions follow the RETURNED envelope (UI-02/1)');

    const planBox = () => page.evaluate(() => {
        const r = document.getElementById('plan-layers').getBoundingClientRect();
        return { w: r.width, h: r.height };
    });
    const stubEnvelopeAspect = () => page.evaluate(() => {
        const e = window.__OCTAGON_STUB__.geometry().envelope;
        return (e.maxX - e.minX) / (e.maxY - e.minY);
    });

    const box0 = await planBox();
    const asp0 = await stubEnvelopeAspect();
    check(box0.w > 0 && box0.h > 0, `plan box rendered — ${box0.w.toFixed(1)} x ${box0.h.toFixed(1)} px`);
    check(near(box0.w / box0.h, asp0, 0.02),
        `rendered aspect ${(box0.w / box0.h).toFixed(4)} matches returned envelope ${asp0.toFixed(4)}`);

    // Mutate the venue to a LANDSCAPE bbox and let the venueGen poll pick it up.
    // If the aspect were hardcoded anywhere, the box would not move.
    await page.evaluate(() => window.__OCTAGON_STUB__.setVenueBox(0, 30, 0, 10));
    await page.waitForFunction(w => document.getElementById('plan-layers').getBoundingClientRect().width !== w,
        box0.w, { timeout: SETTLE_MS }).catch(() => {});

    const box1 = await planBox();
    const asp1 = await stubEnvelopeAspect();
    check(near(box1.w / box1.h, asp1, 0.02),
        `after a stub bbox mutation the aspect FOLLOWED — ${(box1.w / box1.h).toFixed(4)} vs ${asp1.toFixed(4)}`);
    check(!near(box1.w / box1.h, box0.w / box0.h, 0.05),
        'the box actually changed shape (a hardcoded aspect would not have moved)');

    await page.evaluate(() => window.__OCTAGON_STUB__.resetVenue());
    await page.waitForFunction(w => Math.abs(document.getElementById('plan-layers').getBoundingClientRect().width - w) < 1.5,
        box0.w, { timeout: SETTLE_MS }).catch(() => {});

    // ── 3 ────────────────────────────────────────────────────────────────────
    head(3, 'hull overlay: hullCount vertices, speakers 3 and 8 ON_EDGE (UI-02/2)');

    const hullFacts = await page.evaluate(() => ({
        rendered: document.getElementById('hull-poly').points.numberOfItems,
        returned: window.__OCTAGON_STUB__.geometry().hullCount,
        classes: window.__OCTAGON_STUB__.geometry().speakers.map(s => s.class),
        onEdgeDom: [3, 8].map(n => document.getElementById(`glyph-${n}`).classList.contains('is-onedge')),
        vertexDom: [1, 2, 4, 5, 6, 7].map(n => document.getElementById(`glyph-${n}`).classList.contains('is-vertex')),
    }));

    check(hullFacts.rendered === hullFacts.returned,
        `polygon.points.numberOfItems == hullCount — ${hullFacts.rendered} vs ${hullFacts.returned}`);
    check(hullFacts.returned === 6, `the default venue's hull is a HEXAGON — hullCount ${hullFacts.returned}`);
    check(hullFacts.onEdgeDom.every(Boolean),
        'glyphs 3 and 8 carry the ON_EDGE treatment, not the vertex one');
    check(hullFacts.vertexDom.every(Boolean),
        'the other six glyphs carry the VERTEX treatment');
    check(hullFacts.classes[2] === 'ON_EDGE' && hullFacts.classes[7] === 'ON_EDGE',
        'the DOM classes came from the payload classification, not from a JS guess');

    // ── 4 ────────────────────────────────────────────────────────────────────
    head(4, 'puck drag is RELATIVE-DELTA, grabbed OFF-CENTRE (UI-02/3)');

    const puckPos = () => page.evaluate(() => {
        const r = document.getElementById('puck').getBoundingClientRect();
        return { x: r.x + r.width / 2, y: r.y + r.height / 2 };
    });

    const p0 = await puckPos();
    // Off-centre by 8 px. A CENTRED grab passes under absolute cursor tracking
    // too, so a centred probe would be vacuous.
    const grabX = p0.x - 8;
    const grabY = p0.y - 8;

    await page.mouse.move(grabX, grabY);
    await page.mouse.down();
    const pDown = await puckPos();
    check(near(pDown.x, p0.x, 0.75) && near(pDown.y, p0.y, 0.75),
        `no jump to the cursor on pointerdown — moved ${(pDown.x - p0.x).toFixed(2)} px`);

    const DX = 40;
    await page.mouse.move(grabX + DX, grabY, { steps: 4 });
    const pMoved = await puckPos();
    await page.mouse.up();

    check(near(pMoved.x - p0.x, DX, 2.0),
        `puck tracked the DELTA — moved ${(pMoved.x - p0.x).toFixed(2)} px for a ${DX} px pointer move`);

    // Under absolute tracking the puck would land ON the cursor, i.e. 8 px
    // further than the delta. Stated as its own assertion so the failure names
    // the bug rather than a tolerance.
    check(!near(pMoved.x - p0.x, DX + 8, 1.0),
        'the puck did NOT land on the cursor (that would be absolute tracking)');

    // ── 5 ────────────────────────────────────────────────────────────────────
    head(5, 'the accumulator is clamped AT THE ACCUMULATOR — edge reversal (N5/P41)');

    const pStart = await puckPos();
    await page.mouse.move(pStart.x, pStart.y);
    await page.mouse.down();
    // Well past the right edge: a surplus-carrying accumulator banks ~1.2
    // normalised units of overshoot here.
    await page.mouse.move(pStart.x + 420, pStart.y, { steps: 8 });
    const pEdge = await puckPos();

    await page.mouse.move(pStart.x + 416, pStart.y, { steps: 1 });
    const pBack = await puckPos();
    await page.mouse.up();

    check(pEdge.x > pStart.x + 100, `the drag reached the edge — puck at +${(pEdge.x - pStart.x).toFixed(1)} px`);
    check(pBack.x < pEdge.x - 1.0,
        `one 4 px reversal moved the puck IMMEDIATELY — ${(pEdge.x - pBack.x).toFixed(2)} px back`);

    // ── 6 ────────────────────────────────────────────────────────────────────
    head(6, 'canvas DPR backing store, at DPR 1 AND DPR 2 (UI-02/4)');

    const canvasFacts = p => p.evaluate(() => {
        const c = document.getElementById('plan-backdrop');
        const r = c.getBoundingClientRect();
        return { w: c.width, h: c.height, rw: r.width, rh: r.height, dpr: window.devicePixelRatio };
    });

    const c1 = await canvasFacts(page);
    check(c1.rw > 0 && c1.rh > 0, `canvas has a rendered box — ${c1.rw.toFixed(1)} x ${c1.rh.toFixed(1)} css px`);
    check(c1.w === Math.round(c1.rw * c1.dpr) && c1.h === Math.round(c1.rh * c1.dpr),
        `DPR ${c1.dpr}: backing store ${c1.w} x ${c1.h} == round(rect x dpr)`);

    // The bug is INVISIBLE at exactly one DPR — a canvas with no backing-store
    // scaling looks correct at DPR 1 and blurry at DPR 2, and nothing at DPR 1
    // can tell the difference (o-textureforge-cursor-bug).
    const ctx2 = await browser.newContext({ viewport: { width: SHIP_W, height: SHIP_H }, deviceScaleFactor: 2 });
    const page2 = await ctx2.newPage();
    const err2 = [];
    page2.on('pageerror', e => err2.push(String(e)));
    await page2.goto(`http://127.0.0.1:${port}/index.html`, { waitUntil: 'networkidle' });
    await page2.waitForFunction(() => document.getElementById('readout-envelope').textContent !== '—',
        null, { timeout: 10000 }).catch(() => {});

    const c2 = await canvasFacts(page2);
    check(c2.dpr === 2, `second context really is DPR 2 — got ${c2.dpr}`);
    check(c2.w === Math.round(c2.rw * c2.dpr) && c2.h === Math.round(c2.rh * c2.dpr),
        `DPR ${c2.dpr}: backing store ${c2.w} x ${c2.h} == round(rect x dpr)`);
    check(c2.w !== c1.w,
        'the backing store actually differs between DPR 1 and 2 (a CSS-only size would not)');
    check(err2.length === 0, 'no page errors at DPR 2' + (err2.length ? ` — ${err2[0]}` : ''));
    await ctx2.close();

    // ── 7 ────────────────────────────────────────────────────────────────────
    head(7, 'metres resolve against the LIVE venue, puck stationary (UI-02/5a)');

    const readout = () => page.evaluate(() => document.getElementById('readout-metres').textContent);
    const srcNorm = () => page.evaluate(() => [
        Number(document.getElementById('ctl-srcX').value),
        Number(document.getElementById('ctl-srcY').value),
    ]);

    const before = await readout();
    const normBefore = await srcNorm();

    await page.evaluate(() => window.__OCTAGON_STUB__.setVenueBox(100, 130, 200, 240));
    await page.waitForFunction(t => document.getElementById('readout-metres').textContent !== t,
        before, { timeout: SETTLE_MS }).catch(() => {});

    const after = await readout();
    const normAfter = await srcNorm();

    check(before !== '—' && before !== '', `readout populated before the edit — "${before}"`);
    check(after !== before, `editing the venue MOVED the readout — "${before}" -> "${after}"`);
    check(near(normBefore[0], normAfter[0], 1e-6) && near(normBefore[1], normAfter[1], 1e-6),
        'srcX / srcY did not move — the readout changed because the VENUE did');

    // A JS min/max map would have produced the same string both times. State it
    // as its own assertion so the log records what was actually excluded.
    check(!/^\s*6\.50/.test(after),
        'the readout is not pinned to the default venue arithmetic');

    await page.evaluate(() => window.__OCTAGON_STUB__.resetVenue());
    await page.waitForFunction(t => document.getElementById('readout-metres').textContent !== t,
        after, { timeout: SETTLE_MS }).catch(() => {});

    // ── 8 ────────────────────────────────────────────────────────────────────
    head(8, 'the page FITS 1100 x 720 — measured, not computed (P47)');

    const overflow = await page.evaluate(() => ({
        sw: document.documentElement.scrollWidth,
        sh: document.documentElement.scrollHeight,
        iw: window.innerWidth,
        ih: window.innerHeight,
    }));

    check(overflow.iw === SHIP_W && overflow.ih === SHIP_H,
        `viewport really is ${SHIP_W} x ${SHIP_H} — got ${overflow.iw} x ${overflow.ih}`);
    check(overflow.sw <= SHIP_W, `scrollWidth ${overflow.sw} <= ${SHIP_W}`);
    check(overflow.sh <= SHIP_H, `scrollHeight ${overflow.sh} <= ${SHIP_H}`);

    // The Venue screen is a second layout and can overflow on its own.
    //
    // force: true ON THE TAB CLICKS, and NC3 is why. A mini-plan that overflows its rail badly
    // enough physically covers #tab-room, so Playwright's actionability check retries for 30 s and
    // the whole gate dies with a TimeoutError — 200 lines before section 11 could name the actual
    // fault. A layout overflow must produce a NAMED failure, not a click timeout somewhere else.
    // The tabs' own visibility is not what these sections measure; sections 1 and 12 cover that.
    await page.click('#tab-venue', { force: true });
    const venueOverflow = await page.evaluate(() => ({
        sw: document.documentElement.scrollWidth,
        sh: document.documentElement.scrollHeight,
    }));
    check(venueOverflow.sw <= SHIP_W && venueOverflow.sh <= SHIP_H,
        `the Venue screen also fits — ${venueOverflow.sw} x ${venueOverflow.sh}`);
    await page.click('#tab-room', { force: true });
    await page.waitForFunction(() => document.getElementById('plan-layers').getBoundingClientRect().width > 0,
        null, { timeout: SETTLE_MS }).catch(() => {});

    // ── 9 ────────────────────────────────────────────────────────────────────
    head(9, 'the SAFE banner tracks getStatus().safeMode (ROADMAP)');

    const bannerShown = () => page.evaluate(() => {
        const b = document.getElementById('safe-banner');
        return b !== null && !b.hidden && b.getBoundingClientRect().height > 0;
    });

    check((await bannerShown()) === false, 'banner absent while safeMode is false');

    await page.evaluate(() => window.__OCTAGON_STUB__.setStatus({ safeMode: true }));
    await page.waitForFunction(() => !document.getElementById('safe-banner').hidden,
        null, { timeout: SETTLE_MS }).catch(() => {});
    check((await bannerShown()) === true, 'banner APPEARS when safeMode goes true');

    await page.evaluate(() => window.__OCTAGON_STUB__.setStatus({ safeMode: false }));
    await page.waitForFunction(() => document.getElementById('safe-banner').hidden,
        null, { timeout: SETTLE_MS }).catch(() => {});
    check((await bannerShown()) === false, 'banner DISAPPEARS when safeMode goes false again');

    // ── 10 ───────────────────────────────────────────────────────────────────
    head(10, 'every control moves its parameter, and the echo moves the control (ROADMAP)');

    // Both directions from one gesture: the stub records the write, and the
    // stub's echo repaints the readout. A control wired in only one direction
    // fails exactly one half of this.
    const wroteBefore = await page.evaluate(() => ({ ...window.__OCTAGON_STUB__.writes }));
    const readBefore = await page.evaluate(
        ids => Object.fromEntries(ids.map(id => [id, document.getElementById(`val-${id}`).textContent])),
        paramIds);

    let drivable = 0;
    for (const id of paramIds) if (await nudge(page, id)) ++drivable;
    check(drivable === paramIds.length, `all ${paramIds.length} controls were reachable — ${drivable}`);

    const wroteAfter = await page.evaluate(() => ({ ...window.__OCTAGON_STUB__.writes }));
    const readAfter = await page.evaluate(
        ids => Object.fromEntries(ids.map(id => [id, document.getElementById(`val-${id}`).textContent])),
        paramIds);

    const noWrite = paramIds.filter(id => (wroteAfter[id] || 0) <= (wroteBefore[id] || 0));
    check(noWrite.length === 0,
        `every control WROTE its parameter${noWrite.length ? ` — silent: ${noWrite.join(', ')}` : ''}`);

    const noEcho = paramIds.filter(id => readAfter[id] === readBefore[id]);
    check(noEcho.length === 0,
        `every readout MOVED on the echo${noEcho.length ? ` — frozen: ${noEcho.join(', ')}` : ''}`);

    // The gesture brackets, observed rather than only asserted statically. Every
    // parameter that was written must have opened and closed a gesture, and the
    // counts must balance — an unclosed bracket leaves a host in an open
    // automation-write region (RESEARCH-3.1 N1 / PLAN-3.1 P39).
    const gestures = await page.evaluate(() => window.__OCTAGON_STUB__.gestures.slice());
    const opens  = gestures.filter(g => g.phase === 'start').length;
    const closes = gestures.filter(g => g.phase === 'end').length;
    check(opens > 0 && opens === closes,
        `gesture brackets balance — ${opens} opened, ${closes} closed`);

    const bracketed = new Set(gestures.filter(g => g.phase === 'start').map(g => g.id));
    const unbracketed = paramIds.filter(id => !bracketed.has(id));
    check(unbracketed.length === 0,
        `every written parameter opened a gesture${unbracketed.length ? ` — bare: ${unbracketed.join(', ')}` : ''}`);

    // The puck's two-parameter bracket, which is the one JUCE supplies nothing
    // for and the one section 12 of the static gate asserts in source form.
    const puckPairs = gestures.filter(g => g.id === 'srcX' || g.id === 'srcY');
    check(puckPairs.filter(g => g.phase === 'start').length
          === puckPairs.filter(g => g.phase === 'end').length,
        'srcX and srcY brackets balance across the puck drags in sections 4 and 5');

    check(consoleErrors.length === 0,
        'still no console errors after driving every control'
        + (consoleErrors.length ? ` — ${consoleErrors[0]}` : ''));

    // ════════════════════════════════════════════════════════════════════════
    // PHASE 3.2 — the Venue screen. Sections 11-18.
    //
    // Every one of these ran against the ui-stub BEFORE a line of 3.2's C++
    // existed, which is the ordering D4 bought when it removed the
    // browser-iteration safety net for the whole stage.
    // ════════════════════════════════════════════════════════════════════════

    // force: true — see the note in section 8. A screen that overflows must be REPORTED by the
    // section that measures it, not swallowed as a click timeout in a helper.
    const showVenue = async () => {
        await page.click('#tab-venue', { force: true });
        await page.waitForFunction(
            () => document.getElementById('miniplan').getBoundingClientRect().height > 0,
            null, { timeout: SETTLE_MS }).catch(() => {});
    };
    const showRoom = async () => {
        await page.click('#tab-room', { force: true });
        await page.waitForFunction(
            () => document.getElementById('plan-layers').getBoundingClientRect().width > 0,
            null, { timeout: SETTLE_MS }).catch(() => {});
    };

    // Commit the way a finger does: focus, replace, blur. Never el.value = x,
    // which fires no `input` event and would make every assertion below measure
    // a page whose own listeners never ran.
    const typeInto = async (id, text) => {
        const el = await page.$(`#${id}`);
        if (el === null) return false;
        await el.click({ clickCount: 3 });
        await el.press('Backspace');
        await el.type(text);
        return true;
    };
    const blurActive = () => page.evaluate(() => document.activeElement.blur());
    const venueWriteCount = () => page.evaluate(() => window.__OCTAGON_STUB__.venueWrites.length);

    await showVenue();

    // ── 11 ───────────────────────────────────────────────────────────────────
    head(11, 'the RAIL does not overflow, and the Venue screen fits 1100 x 720 (P62/Q11)');

    // THE ASSERTION SECTION 8 DOES NOT MAKE. Q11 measured a width-bound mini-plan pushing the rail
    // stack to 601 px inside 592 — WHILE document.scrollHeight stayed 720 the whole time. Section
    // 8 measures the DOCUMENT and would have passed that. NC3's evidence is that ASYMMETRY:
    // section 11 fires while section 8 passes.
    //
    // ── WHICH OF THE ASSERTIONS BELOW ACTUALLY FIRES, MEASURED BY NC3 AT EXECUTE ──────────────
    // PLAN-3.2 P62 named `railScrollHeight <= railClientHeight` as the guard. IT IS NOT ONE, in
    // this layout: NC3 width-bound the mini-plan to 300 x 375 inside a 300 x 213 stage — a 162 px
    // overflow — and that assertion still reported 592 <= 592 and PASSED.
    //
    // The reason is the shape of the box tree. Q11's mock made the plan a direct flex CHILD of the
    // rail, sized to the fitted box, so an oversized plan grew the rail's own content. Here
    // .miniplan is a `flex: 1 1 auto` STAGE that absorbs the rail's residual height, and the <svg>
    // inside it overflows the STAGE — which Chromium does not propagate up into the rail's
    // scrollHeight.
    //
    // So the LOAD-BEARING assertion is the fitted box against the stage that was measured to
    // produce it. The rail check is kept because it covers a DIFFERENT shape — a rail GROUP
    // growing past the stack — but it must not be read as the guard against a mis-fitted plan.
    // This is the same class of error Q11 itself caught, one layer further down: an assertion
    // measuring the wrong box.
    const railFacts = await page.evaluate(() => {
        const rail = document.getElementById('venue-rail');
        const mini = document.getElementById('miniplan');
        const svg = document.getElementById('mini-geometry');
        const r = (el) => { const b = el.getBoundingClientRect(); return { w: +b.width.toFixed(1), h: +b.height.toFixed(1) }; };
        return {
            railScrollH: rail.scrollHeight,
            railClientH: rail.clientHeight,
            rail: r(rail),
            mini: r(mini),
            svg: r(svg),
            docW: document.documentElement.scrollWidth,
            docH: document.documentElement.scrollHeight,
        };
    });

    // Coarse: catches a rail GROUP growing past the stack. Does NOT catch a mis-fitted plan.
    check(railFacts.railScrollH <= railFacts.railClientH,
        `[coarse] railScrollHeight ${railFacts.railScrollH} <= railClientHeight ${railFacts.railClientH}`);

    // THE GUARD. The fitted box against the stage it was fitted into — height-bound means it fits,
    // width-bound at a portrait aspect does not, and this is the assertion NC3 makes fire.
    check(railFacts.svg.h <= railFacts.mini.h + 0.5,
        `[guard] the fitted plan is inside its stage — ${railFacts.svg.h} <= ${railFacts.mini.h} `
        + `(plan ${railFacts.svg.w} x ${railFacts.svg.h}, stage ${railFacts.mini.w} x ${railFacts.mini.h})`);
    check(railFacts.svg.w <= railFacts.mini.w + 0.5,
        `[guard] and inside it horizontally — ${railFacts.svg.w} <= ${railFacts.mini.w}`);
    check(railFacts.docW <= SHIP_W, `Venue screen scrollWidth ${railFacts.docW} <= ${SHIP_W}`);
    check(railFacts.docH <= SHIP_H, `Venue screen scrollHeight ${railFacts.docH} <= ${SHIP_H}`);

    // ── 12 ───────────────────────────────────────────────────────────────────
    head(12, 'all 50 venue values are viewable and editable at 1100 x 720 (UI-01/1)');

    // The id list is DERIVED here the same way PARAM_IDS is parsed in section 1
    // — 8 x (label + x + y + z + trim + delay) + 2 rake. A literal 50 typed into
    // this file would stop describing the page the moment a column moved.
    //
    // 42 -> 50 at v1.4.0. The COUNT below still has to move by hand, and that is
    // the point of having it at all: it is the assertion that the delay column
    // actually reached the DOM, as opposed to the derivation quietly agreeing
    // with itself about a page that never got the eighth column.
    const VENUE_IDS = [];
    for (let n = 1; n <= 8; ++n) {
        VENUE_IDS.push(`vf-label-${n}`);
        for (const k of ['x', 'y', 'z', 'trim', 'delay']) VENUE_IDS.push(`vf-${n}-${k}`);
    }
    VENUE_IDS.push('vf-rake-front', 'vf-rake-rear');
    check(VENUE_IDS.length === 50, `50 venue field ids derived — ${VENUE_IDS.length}`);

    const fieldFacts = await page.evaluate(ids => {
        const out = { present: 0, editable: 0, inViewport: 0, populated: 0, numberTyped: 0, missing: [] };
        for (const id of ids) {
            const el = document.getElementById(id);
            if (el === null) { out.missing.push(id); continue; }
            ++out.present;
            if (!el.disabled && !el.readOnly) ++out.editable;
            if (el.getAttribute('type') === 'number') ++out.numberTyped;
            const b = el.getBoundingClientRect();
            if (b.width > 0 && b.height > 0 && b.left >= 0 && b.top >= 0
                && b.right <= window.innerWidth && b.bottom <= window.innerHeight) ++out.inViewport;
            if (String(el.value).trim() !== '') ++out.populated;
        }
        return out;
    }, VENUE_IDS);

    check(fieldFacts.present === 50, `all 50 fields present — ${fieldFacts.present}${fieldFacts.missing.length ? ` (missing ${fieldFacts.missing.join(', ')})` : ''}`);
    check(fieldFacts.editable === 50, `all 50 are editable — ${fieldFacts.editable}`);
    check(fieldFacts.inViewport === 50, `all 50 are fully inside ${SHIP_W} x ${SHIP_H} — ${fieldFacts.inViewport}`);
    check(fieldFacts.populated === 50, `all 50 were POPULATED from the payload — ${fieldFacts.populated}`);
    check(fieldFacts.numberTyped === 0, `no type="number" among them — ${fieldFacts.numberTyped} (D12)`);

    // The mini-plan's proportions follow the RETURNED envelope, exactly as the
    // Room plan's do. Compared against what the stub returned, re-read now.
    const miniFacts = await page.evaluate(() => {
        const b = document.getElementById('mini-geometry').getBoundingClientRect();
        const e = window.__OCTAGON_STUB__.geometry().envelope;
        return { w: b.width, h: b.height, aspect: (e.maxX - e.minX) / (e.maxY - e.minY) };
    });
    check(miniFacts.w > 0 && miniFacts.h > 0,
        `mini-plan rendered — ${miniFacts.w.toFixed(1)} x ${miniFacts.h.toFixed(1)} px`);
    check(near(miniFacts.w / miniFacts.h, miniFacts.aspect, 0.02),
        `mini-plan aspect ${(miniFacts.w / miniFacts.h).toFixed(4)} follows the returned envelope ${miniFacts.aspect.toFixed(4)}`);

    // ── 13 ───────────────────────────────────────────────────────────────────
    head(13, 'invalid input MARKS and REVERTS; a metre commits ONE setVenue of 50 (FUNC-02/1)');

    const beforeBad = await venueWriteCount();
    const goodValue = await page.evaluate(() => document.getElementById('vf-3-y').value);

    await typeInto('vf-3-y', 'abc');
    await blurActive();

    const badFacts = await page.evaluate(() => {
        const el = document.getElementById('vf-3-y');
        return { marked: el.classList.contains('is-invalid'), value: el.value };
    });
    const afterBad = await venueWriteCount();

    check(badFacts.marked, 'typing `abc` MARKED the field');
    check(badFacts.value === goodValue,
        `and it REVERTED on blur to the last committed value — "${badFacts.value}" vs "${goodValue}"`);
    check(afterBad === beforeBad,
        `an invalid edit made NO setVenue call — ${beforeBad} -> ${afterBad}`);

    await typeInto('vf-3-y', '7.25');
    await blurActive();
    await page.waitForFunction(n => window.__OCTAGON_STUB__.venueWrites.length > n,
        afterBad, { timeout: SETTLE_MS }).catch(() => {});

    const goodFacts = await page.evaluate(n => {
        const w = window.__OCTAGON_STUB__.venueWrites;
        const last = w[w.length - 1];
        const speakers = last && Array.isArray(last.speakers) ? last.speakers : [];
        const count = speakers.reduce((acc, s) =>
            acc + ['label', 'x', 'y', 'z', 'trimDb', 'delayMs'].filter(k => s[k] !== undefined).length, 0)
            + (last && last.rake ? ['front', 'rear'].filter(k => last.rake[k] !== undefined).length : 0);
        return { calls: w.length - n, values: count, y3: speakers.length > 2 ? speakers[2].y : null,
                 marked: document.getElementById('vf-3-y').classList.contains('is-invalid') };
    }, afterBad);

    check(goodFacts.calls === 1, `a valid metre committed EXACTLY ONE setVenue call — ${goodFacts.calls}`);
    check(goodFacts.values === 50, `and it carried all 50 values in one payload — ${goodFacts.values}`);
    check(goodFacts.y3 === 7.25, `the typed value reached the payload — speakers[2].y = ${goodFacts.y3}`);
    check(!goodFacts.marked, 'the invalid mark cleared once the field parsed');

    // ── 14 ───────────────────────────────────────────────────────────────────
    head(14, 'UI-02/5 END-TO-END: a Venue edit moves the ROOM readout (P45, inherited)');

    // The half of UI-02 criterion 5 that 3.1 could not close, declared at 3.1's
    // plan as a 3.2 gate. Section 7 proved the readout follows a MUTATED STUB;
    // this proves it follows a value the OPERATOR TYPED on the other screen.
    await showRoom();

    // ── THE SOURCE HAS TO BE OFF THE RAIL FOR THE METRES CLAUSE TO MEAN
    //    ANYTHING (found by the WR-04 fix, v1.3.2) ──────────────────────────
    //
    // Until v1.3.2 the metres clause below read `metBefore` from a footer that
    // WR-04 had left STALE: renderMetres() had exactly one live subscriber (the
    // puck's pointermove), so the venue edit here was the first thing to
    // recompute it since the source had last moved. "The readout changed" was
    // measuring the accumulated staleness, not the edit — a probe reporting on
    // the bug rather than on the behaviour.
    //
    // With the footer subscribed to the srcX / srcY echo the clause failed, and
    // it was RIGHT to: the source sits at normalised 1.0, where metres.x is the
    // bbox MAX rail, and the edit below moves speaker 1's x — a MIN rail. The
    // mapping is metres = min + n*(max - min), so a min-rail move of Δ shifts
    // the readout by (1 - n)*Δ, which at n = 1 is exactly zero. Nothing was
    // broken; the stimulus could not reach the readout.
    //
    // Park the source in the interior, where the mapping is sensitive to both
    // rails, and read `metBefore` AFTER that settles — so what the clause
    // compares is the venue edit alone. Driven through the control's own `input`
    // event, which is the same path a finger takes.
    const srcParked = await page.evaluate(() => ({
        x: document.getElementById('ctl-srcX').value,
        y: document.getElementById('ctl-srcY').value,
    }));

    const driveSource = async (id, norm) => {
        const el = await page.$(`#ctl-${id}`);
        if (el === null) return;
        await el.evaluate((e, v) => {
            e.value = String(v);
            e.dispatchEvent(new Event('input', { bubbles: true }));
        }, norm);
    };

    await driveSource('srcX', 0.5);
    await driveSource('srcY', 0.5);
    await page.waitForTimeout(120);

    const envBefore = await page.evaluate(() => document.getElementById('readout-envelope').textContent);
    const metBefore = await page.evaluate(() => document.getElementById('readout-metres').textContent);

    await showVenue();
    await typeInto('vf-1-x', '-4.00');
    await blurActive();
    await showRoom();

    await page.waitForFunction(t => document.getElementById('readout-envelope').textContent !== t,
        envBefore, { timeout: SETTLE_MS }).catch(() => {});

    const envAfter = await page.evaluate(() => document.getElementById('readout-envelope').textContent);
    const metAfter = await page.evaluate(() => document.getElementById('readout-metres').textContent);

    check(envBefore !== '—' && envBefore !== '', `Room envelope readout populated before the edit — "${envBefore}"`);
    check(envAfter !== envBefore,
        `typing a coordinate on the VENUE screen moved the ROOM envelope readout — "${envBefore}" -> "${envAfter}"`);
    check(metAfter !== metBefore,
        `and the metres readout too — "${metBefore}" -> "${metAfter}"`);

    await page.evaluate(() => window.__OCTAGON_STUB__.resetVenue());
    await page.waitForFunction(t => document.getElementById('readout-envelope').textContent !== t,
        envAfter, { timeout: SETTLE_MS }).catch(() => {});

    // Put the source back where the sections after this one found it, so this
    // section's fixture does not become their hidden precondition.
    await driveSource('srcX', srcParked.x);
    await driveSource('srcY', srcParked.y);
    await page.waitForTimeout(120);

    await showVenue();

    // ── 15 ───────────────────────────────────────────────────────────────────
    head(15, 'a duplicate label MARKS BOTH ROWS and BLOCKS the commit (N8/P53)');

    // THE REACHABILITY ARGUMENT, DRIVEN. Every route from (L, R) to (R, L)
    // passes through a duplicate, so a label column that REVERTED like the
    // numeric ones would make the swap impossible. It holds and marks instead,
    // and while the set is not a permutation the page does not call setVenue AT
    // ALL — which matters because an invalid map is AUDIBLE, not a quiet
    // retention (N8: speaker 1 gets L, speakers 2-8 all get R at unity).
    const l1 = await page.evaluate(() => document.getElementById('vf-label-1').value);
    const l2 = await page.evaluate(() => document.getElementById('vf-label-2').value);
    const beforeSwap = await venueWriteCount();

    await typeInto('vf-label-1', l2);
    await blurActive();

    const collided = await page.evaluate(() => ({
        one: document.getElementById('vf-label-1').classList.contains('is-colliding'),
        two: document.getElementById('vf-label-2').classList.contains('is-colliding'),
        held: document.getElementById('vf-label-1').value,
    }));
    const midSwap = await venueWriteCount();

    check(collided.one && collided.two, 'BOTH colliding rows are marked, not just the one that was typed');
    check(collided.held === l2, `the pending label is HELD, not reverted — "${collided.held}" (reverting makes L <-> R unreachable)`);
    check(midSwap === beforeSwap, `setVenue was NOT called while the set was not a permutation — ${beforeSwap} -> ${midSwap}`);

    await typeInto('vf-label-2', l1);
    await blurActive();
    await page.waitForFunction(n => window.__OCTAGON_STUB__.venueWrites.length > n,
        midSwap, { timeout: SETTLE_MS }).catch(() => {});

    const swapped = await page.evaluate(n => {
        const w = window.__OCTAGON_STUB__.venueWrites;
        return {
            calls: w.length - n,
            labels: w.length ? w[w.length - 1].speakers.map(s => s.label) : [],
            marks: [1, 2].map(i => document.getElementById(`vf-label-${i}`).classList.contains('is-colliding')),
        };
    }, midSwap);

    check(swapped.calls === 1, `completing the swap committed exactly once — ${swapped.calls}`);
    check(swapped.labels[0] === l2 && swapped.labels[1] === l1,
        `and the swap reached the payload — row1 "${swapped.labels[0]}", row2 "${swapped.labels[1]}"`);
    check(!swapped.marks[0] && !swapped.marks[1], 'both marks cleared once the set was a permutation again');

    await page.evaluate(() => window.__OCTAGON_STUB__.resetVenue());
    await page.waitForFunction(l => document.getElementById('vf-label-1').value === l,
        l1, { timeout: SETTLE_MS }).catch(() => {});

    // ── 16 ───────────────────────────────────────────────────────────────────
    head(16, 'the MAP INVALID banner carries reason AND row, on BOTH screens (D13/P54)');

    const bannerFacts = () => page.evaluate(() => {
        const b = document.getElementById('map-banner');
        const c = document.getElementById('map-invalid-copy');
        return {
            shown: b !== null && !b.hidden && b.getBoundingClientRect().height > 0,
            copy: c === null ? '' : c.textContent,
        };
    });

    check((await bannerFacts()).shown === false, 'banner absent while the map is valid');

    await page.evaluate(() => window.__OCTAGON_STUB__.setMapInvalid('duplicateLabel', 2));
    await page.waitForFunction(() => !document.getElementById('map-banner').hidden,
        null, { timeout: SETTLE_MS }).catch(() => {});

    const onVenue = await bannerFacts();
    check(onVenue.shown, 'banner APPEARS on the Venue screen');
    check(/duplicate/i.test(onVenue.copy), `and names the REASON — "${onVenue.copy}"`);
    check(/\b3\b/.test(onVenue.copy), `and names the ROW, 1-based — "${onVenue.copy}"`);

    await showRoom();
    const onRoom = await bannerFacts();
    check(onRoom.shown, 'the SAME banner is visible on the ROOM screen — it is frame-level (D13)');
    check(onRoom.copy === onVenue.copy, 'with the same reason and row');

    await page.evaluate(() => window.__OCTAGON_STUB__.setMapInvalid('none', -1));
    await page.waitForFunction(() => document.getElementById('map-banner').hidden,
        null, { timeout: SETTLE_MS }).catch(() => {});
    check((await bannerFacts()).shown === false, 'banner DISAPPEARS when the map resolves again');

    await showVenue();

    // ── 17 ───────────────────────────────────────────────────────────────────
    head(17, 'negotiated set name and per-speaker hull CLASS are on screen (orphans 6, 7)');

    // Compared against what the stub RETURNED, re-read now — never against a
    // "7.1 Surround" typed into this file
    // (pattern_test_fixture_mirrors_drift_silently).
    const readFacts = await page.evaluate(() => ({
        setName: document.getElementById('vset-name').textContent.trim(),
        returnedSetName: String(window.__OCTAGON_STUB__.status().outputSetName),
        classes: [1, 2, 3, 4, 5, 6, 7, 8].map(n => document.getElementById(`vclass-${n}`).textContent.trim()),
        returned: window.__OCTAGON_STUB__.geometry().speakers.map(s => s.class),
    }));

    check(readFacts.setName !== '' && readFacts.setName !== '—',
        `the negotiated output-set name is rendered — "${readFacts.setName}"`);
    check(readFacts.setName === readFacts.returnedSetName,
        `and it is the name getStatus RETURNED — "${readFacts.setName}" vs "${readFacts.returnedSetName}"`);
    check(readFacts.classes.join(',') === readFacts.returned.join(','),
        `the CLASS column is the PAYLOAD's classification, not a JS guess — [${readFacts.classes.join(', ')}]`);
    check(readFacts.returned.filter(c => c === 'ON_EDGE').length === 2,
        'and it distinguishes ON_EDGE from VERTEX — the default venue has two of them');

    // ── 18 ───────────────────────────────────────────────────────────────────
    head(18, 'the ping indicator follows the RETURNED getPingState().speaker (UI-01/2, D14)');

    const litRows = () => page.evaluate(() =>
        [1, 2, 3, 4, 5, 6, 7, 8].filter(n => document.getElementById(`btn-ping-${n}`).classList.contains('is-pinging')));
    const stubSpeaker = () => page.evaluate(() => window.__OCTAGON_STUB__.ping().speaker);

    check((await litRows()).length === 0, 'nothing is lit before the ping starts');

    await page.click('#btn-ping-3');
    await page.waitForFunction(() => document.getElementById('btn-ping-3').classList.contains('is-pinging'),
        null, { timeout: SETTLE_MS }).catch(() => {});

    const lit1 = await litRows();
    check(lit1.length === 1 && lit1[0] === await stubSpeaker(),
        `exactly one row is lit and it is the RETURNED speaker — lit [${lit1.join(',')}], returned ${await stubSpeaker()}`);

    // Advance the cycle in the STUB and let the 100 ms poll pick it up. The
    // page never computes the next index: a drifted setInterval would name
    // speaker 5 while 6 sounds, during the one procedure whose entire purpose is
    // confirming that speaker N is speaker N (D14).
    await page.evaluate(() => window.__OCTAGON_STUB__.stepPing());
    const stepped = await stubSpeaker();
    await page.waitForFunction(n => document.getElementById(`btn-ping-${n}`).classList.contains('is-pinging'),
        stepped, { timeout: SETTLE_MS }).catch(() => {});

    const lit2 = await litRows();
    check(lit2.length === 1 && lit2[0] === stepped,
        `the indicator FOLLOWED a stub-driven step — lit [${lit2.join(',')}], returned ${stepped}`);
    check(lit2[0] !== lit1[0], 'and it actually moved (a frozen indicator would have passed the first check)');

    await page.click('#btn-ping-stop');
    await page.waitForFunction(() =>
        [1, 2, 3, 4, 5, 6, 7, 8].every(n => !document.getElementById(`btn-ping-${n}`).classList.contains('is-pinging')),
        null, { timeout: SETTLE_MS }).catch(() => {});
    check((await litRows()).length === 0, 'Stop clears the indicator');

    // A REFUSAL RENDERS ITS REASON. Pinging "speaker 5" on a stereo fold names a
    // speaker that does not exist during the one procedure whose purpose is the
    // opposite — R1 reproduced inside its own diagnostic tool (Q5/P60).
    await page.evaluate(() => window.__OCTAGON_STUB__.setMapInvalid('duplicateLabel', 0));
    await page.click('#btn-ping-1');
    await page.waitForFunction(() => document.getElementById('vping-state').textContent.trim() !== '—',
        null, { timeout: SETTLE_MS }).catch(() => {});

    const refusal = await page.evaluate(() => ({
        copy: document.getElementById('vping-state').textContent.trim(),
        lit: [1, 2, 3, 4, 5, 6, 7, 8].filter(n => document.getElementById(`btn-ping-${n}`).classList.contains('is-pinging')).length,
    }));
    check(refusal.lit === 0, 'the ping REFUSED to start on an invalid map — nothing is lit');
    check(/mapInvalid/i.test(refusal.copy), `and the refusal reason is on screen — "${refusal.copy}"`);

    await page.evaluate(() => window.__OCTAGON_STUB__.setMapInvalid('none', -1));

    check(consoleErrors.length === 0,
        'still no console errors after driving the whole Venue screen'
        + (consoleErrors.length ? ` — ${consoleErrors[0]}` : ''));

    // ══════════════════════════════════════════════════════════════════════
    // PHASE 3.3 — sections 19-27. Scenes, meters, the field, the elevation.
    //
    // §28/§29/§30 of PLAN-3.3's requirement-staging table — UI-05's three
    // construction assertions — are folded in below as sub-checks of the
    // elevation sections rather than as three further top-level sections. That
    // kept 3.3's gating section count at 27.
    //
    // 4.2 ADDS ONE TOP-LEVEL SECTION (28, Q5's guard deadline), so THE SECTION
    // COUNT THAT GATES IS NOW 28. PLAN-4.2 P110 states the move rather than
    // leaving it to be discovered: frontend 42 + layout 28 = 70. A re-run
    // against the old 27 would report a failure that is actually the plan
    // working.
    // ══════════════════════════════════════════════════════════════════════

    await page.click('#tab-room', { force: true });
    await page.waitForFunction(() => document.getElementById('plan-layers').getBoundingClientRect().width > 0,
        null, { timeout: SETTLE_MS }).catch(() => {});

    // ── 19 ───────────────────────────────────────────────────────────────────
    head(19, 'ten scene buttons in ONE row, and every label\'s ink fits its box (P72)');

    // V2 — one row of ELEVEN — misses by 0.2 px and clips SILENTLY, and only on
    // FRONT / RIGHT / SIDES / STORE. That is the kind of defect that ships, so
    // the fit is MEASURED per label rather than reasoned about: scrollWidth
    // exceeds clientWidth exactly when nowrap content is being clipped by the
    // overflow:hidden that keeps the row at ten columns.
    const sceneFacts = await page.evaluate(() => {
        const row = document.getElementById('scene-row');
        const btns = Array.from(row.querySelectorAll('.scene-btn'));
        const tops = new Set(btns.map(b => Math.round(b.getBoundingClientRect().top)));
        return {
            count: btns.length,
            rows: tops.size,
            clipped: btns.filter(b => b.scrollWidth > b.clientWidth).map(b => b.id),
            widths: btns.map(b => +b.getBoundingClientRect().width.toFixed(2)),
            labels: btns.map(b => b.textContent.trim()),
            rowW: +row.getBoundingClientRect().width.toFixed(1),
        };
    });

    check(sceneFacts.count === 10, `ten scene controls — got ${sceneFacts.count}`);
    check(sceneFacts.rows === 1,
        `all ten share ONE row — ${sceneFacts.rows} distinct top edge(s)`);
    check(sceneFacts.clipped.length === 0,
        `no label is clipped — scrollWidth <= clientWidth on all ten`
        + (sceneFacts.clipped.length ? ` — CLIPPED: ${sceneFacts.clipped.join(', ')}` : '')
        + ` (buttons ${Math.min(...sceneFacts.widths).toFixed(1)} px in a ${sceneFacts.rowW} px row)`);

    // THE LABELS ARE HTML-AUTHORED AND JS NEVER WRITES THEM. A shared state
    // updater writing textContent erases them silently and passes every build
    // gate — ROADMAP names this a 3.3 criterion in its own right.
    check(sceneFacts.labels.join(',') === 'ALL,FRONT,REAR,LEFT,RIGHT,SIDES,U1,U2,U3,U4',
        `the authored labels survived the page running — [${sceneFacts.labels.join(', ')}]`);

    // ── 20 ───────────────────────────────────────────────────────────────────
    head(20, 'STORE is an ARM toggle in the group TITLE row, and it auto-disarms (D22/P72)');

    const storeFacts = await page.evaluate(() => {
        const btn = document.getElementById('btn-scene-store');
        const head = btn.closest('.group-head');
        const title = head === null ? null : head.querySelector('.group-title');
        const rowTop = document.getElementById('scene-row').getBoundingClientRect().top;
        return {
            inTitleRow: title !== null,
            aboveTheRow: btn.getBoundingClientRect().bottom <= rowTop + 1,
            pressed: btn.getAttribute('aria-pressed'),
            label: btn.textContent.trim(),
        };
    });

    check(storeFacts.inTitleRow, 'STORE shares the group head with the Scenes heading');
    check(storeFacts.aboveTheRow, 'and it is NOT an eleventh button in the row');
    check(storeFacts.pressed === 'false', `it starts disarmed — aria-pressed="${storeFacts.pressed}"`);

    // ARM -> CAPTURE -> AUTO-DISARM. Recalling a scene is reversible;
    // overwriting a slot is not, which is why this one gesture is armed and
    // D21's preview is not.
    await page.click('#btn-scene-store');
    const armed = await page.getAttribute('#btn-scene-store', 'aria-pressed');
    check(armed === 'true', `clicking STORE arms it — aria-pressed="${armed}"`);

    // The slot is EMPTY, so it is disabled for RECALL — but it must be
    // clickable for CAPTURE, or the arm gesture could never fill a slot.
    const armedSlotEnabled = await page.evaluate(() => !document.getElementById('scene-slot1').disabled);
    check(armedSlotEnabled, 'an empty slot becomes clickable while armed — capture is the point');

    await page.click('#scene-slot1');
    await page.waitForFunction(() => document.getElementById('btn-scene-store').getAttribute('aria-pressed') === 'false',
        null, { timeout: SETTLE_MS }).catch(() => {});

    const afterStore = await page.evaluate(() => ({
        pressed: document.getElementById('btn-scene-store').getAttribute('aria-pressed'),
        occupied: window.__OCTAGON_STUB__.slots()[0].occupied,
        stored: window.__OCTAGON_STUB__.slots()[0].w,
        label: document.getElementById('btn-scene-store').textContent.trim(),
    }));

    check(afterStore.pressed === 'false', 'and it AUTO-DISARMS after one capture');
    check(afterStore.occupied === true,
        `the slot captured the live weights — [${afterStore.stored.map(v => v.toFixed(2)).join(', ')}]`);
    check(afterStore.label === 'STORE' && storeFacts.label === 'STORE',
        `the STORE label is HTML-authored and was never rewritten — "${afterStore.label}"`);

    // ── 21 ───────────────────────────────────────────────────────────────────
    head(21, 'the elevation strip fits its STAGE, at DPR 1 and DPR 2 (D25/P75/N11)');

    // ── WHY THIS ASSERTION AND NOT scrollHeight <= clientHeight ────────────
    // RESEARCH-3.3 N11 corrected D25's premise and left its conclusion
    // standing. The premise was that `controls.scrollHeight === clientHeight
    // === 592` on an UNMODIFIED tree proved the vacuous shape was present. An
    // unmodified tree has NOTHING TO OVERFLOW, so both numbers read equal
    // either way. Measured with a real 120 px overflow, the COLUMN-LEVEL
    // assertion FIRES (699<=592) in all three candidate stage constructions;
    // the genuinely vacuous one is the DOCUMENT-LEVEL section 8, which passed
    // at 720<=720 in every run at both DPRs.
    //
    // The structural fact behind that: a flex container's scrollHeight grows
    // only for overflow past its LAST child's margin edge. .miniplan is child
    // 2 of 5 in the venue rail — which is why section 11's coarse check is
    // vacuous there — and #group-elevation IS this column's last child.
    //
    // SO THE COLUMN CHECK IS KEPT AND IS NOT THE GUARD. It is non-vacuous only
    // WHILE the elevation group remains last; insert anything after it and it
    // silently stops meaning anything. Section 22 asserts that ordering fact.
    // NC1's asymmetry is [section 8 passes] while [this fires].
    const elevBox = async (p) => p.evaluate(() => {
        const col = document.querySelector('.controls-column');
        const stage = document.getElementById('elev-stage');
        const strip = document.getElementById('elev-strip');
        const canvas = document.getElementById('plan-backdrop');
        const r = (el) => { const b = el.getBoundingClientRect(); return { w: +b.width.toFixed(1), h: +b.height.toFixed(1) }; };
        return {
            colScrollH: col.scrollHeight,
            colClientH: col.clientHeight,
            stage: { w: stage.clientWidth, h: stage.clientHeight },
            strip: r(strip),
            docH: document.documentElement.scrollHeight,
            dpr: window.devicePixelRatio,
            backing: { w: canvas.width, h: canvas.height },
        };
    });

    const e1 = await elevBox(page);

    check(e1.colScrollH <= e1.colClientH,
        `[coarse] controls column scrollHeight ${e1.colScrollH} <= clientHeight ${e1.colClientH}`);

    check(e1.strip.h <= e1.stage.h + 0.5,
        `[guard] DPR 1: the strip is inside its stage — ${e1.strip.h} <= ${e1.stage.h} `
        + `(strip ${e1.strip.w} x ${e1.strip.h}, stage ${e1.stage.w} x ${e1.stage.h})`);
    check(e1.strip.w <= e1.stage.w + 0.5,
        `[guard] and inside it horizontally — ${e1.strip.w} <= ${e1.stage.w}`);

    // DPR 2. The whole point of measuring twice is that a raster bug is
    // INVISIBLE at exactly one DPR, and the backing-store comparison below is
    // the control proving the second context really is what it claims.
    const ctxE = await browser.newContext({ viewport: { width: SHIP_W, height: SHIP_H }, deviceScaleFactor: 2 });
    const pageE = await ctxE.newPage();
    const errE = [];
    pageE.on('pageerror', ev => errE.push(String(ev)));
    await pageE.goto(`http://127.0.0.1:${port}/index.html`, { waitUntil: 'networkidle' });
    await pageE.waitForFunction(() => document.getElementById('readout-envelope').textContent !== '—',
        null, { timeout: 10000 }).catch(() => {});

    const e2 = await elevBox(pageE);

    check(e2.dpr === 2, `second context really is DPR 2 — got ${e2.dpr}`);
    check(e2.backing.w === e1.backing.w * 2,
        `and the plan backing store DOUBLED — ${e1.backing.w} -> ${e2.backing.w} device px `
        + '(a CSS-only size would have reported the same number twice)');
    check(e2.strip.h <= e2.stage.h + 0.5 && e2.strip.w <= e2.stage.w + 0.5,
        `[guard] DPR 2: the strip is inside its stage — ${e2.strip.w} x ${e2.strip.h} `
        + `in ${e2.stage.w} x ${e2.stage.h}`);
    check(e2.colScrollH <= e2.colClientH,
        `[coarse] DPR 2: column scrollHeight ${e2.colScrollH} <= clientHeight ${e2.colClientH}`);
    check(errE.length === 0, 'no page errors at DPR 2' + (errE.length ? ` — ${errE[0]}` : ''));
    await ctxE.close();

    // ── 22 ───────────────────────────────────────────────────────────────────
    head(22, 'the ordering fact section 21 depends on, and the strip\'s construction (UI-05)');

    // THE ORDERING FACT. Without this the coarse column assertion above goes
    // vacuous the first time a later phase appends a group, and NOTHING would
    // report it — the check would keep printing PASS over an arbitrary
    // overflow. NC8 inserts a node after #group-elevation and fires this while
    // section 21's guard still passes.
    const ordering = await page.evaluate(() => {
        const col = document.querySelector('.controls-column');
        return {
            last: col.lastElementChild.id,
            count: col.children.length,
            ids: Array.from(col.children).map(c => c.id),
        };
    });

    check(ordering.last === 'group-elevation',
        `#group-elevation is the controls column's LAST child — last is "${ordering.last}" `
        + `of [${ordering.ids.join(', ')}]`);

    // ── UI-05/1 — AND ITS NEGATIVE HALF IS THE COVERAGE ────────────────────
    // earHeight EXTRAPOLATES linearly outside bbMinY..bbMaxY, so a rake line
    // drawn across the whole ENVELOPE has BOTH ends move when rakeRear moves.
    // Asserting only that the rear moved would pass over exactly that defect,
    // and so would a height axis that RESCALED instead of the line moving.
    // Both halves, together, are the criterion. NC7 draws the whole-envelope
    // line and watches the negative half fail.
    const rakeLine = () => page.evaluate(() => {
        const l = document.getElementById('elev-rake');
        return {
            x1: +Number(l.getAttribute('x1')).toFixed(2), y1: +Number(l.getAttribute('y1')).toFixed(2),
            x2: +Number(l.getAttribute('x2')).toFixed(2), y2: +Number(l.getAttribute('y2')).toFixed(2),
        };
    });

    const rakeBefore = await rakeLine();
    const axisBefore = await page.evaluate(() =>
        document.querySelectorAll('#elev-axis .elev-axis-label').length);

    await page.evaluate(() => window.__OCTAGON_STUB__.setRake(0, 2.0));
    await page.waitForFunction(y => Number(document.getElementById('elev-rake').getAttribute('y2')) !== y,
        rakeBefore.y2, { timeout: SETTLE_MS }).catch(() => {});

    const rakeAfter = await rakeLine();
    const axisAfter = await page.evaluate(() =>
        document.querySelectorAll('#elev-axis .elev-axis-label').length);

    check(Math.abs(rakeAfter.y2 - rakeBefore.y2) > 4,
        `[positive] rakeRear 0 -> 2.0 m MOVED the rear endpoint — y2 ${rakeBefore.y2} -> ${rakeAfter.y2} px`);
    check(near(rakeAfter.y1, rakeBefore.y1, 0.01) && near(rakeAfter.x1, rakeBefore.x1, 0.01),
        `[negative] and the FRONT endpoint did NOT move — (${rakeBefore.x1}, ${rakeBefore.y1}) `
        + `-> (${rakeAfter.x1}, ${rakeAfter.y1})`);
    check(axisAfter === axisBefore,
        `[rule 2] the QUANTISED height axis did not rescale — ${axisBefore} -> ${axisAfter} ticks`);

    await page.evaluate(() => window.__OCTAGON_STUB__.setRake(0, 0));
    await page.waitForFunction(y => Number(document.getElementById('elev-rake').getAttribute('y2')) !== y,
        rakeAfter.y2, { timeout: SETTLE_MS }).catch(() => {});

    // ── UI-05/2 — BOTH READINGS, AND NEITHER IS CLAMPED ────────────────────
    const readings = await page.evaluate(() => ({
        ear: document.getElementById('elev-ear').textContent.trim(),
        src: document.getElementById('elev-src').textContent.trim(),
    }));
    check(/^-?\d+\.\d\d m$/.test(readings.ear) && /^-?\d+\.\d\d m$/.test(readings.src),
        `both readings are shown — ear "${readings.ear}", source "${readings.src}"`);

    // srcZ to its maximum. The absolute source height leaves a 7 m axis, so the
    // MARKER clamps with a chevron and the NUMBER does not (P76 rule 3).
    await page.evaluate(() => {
        const el = document.getElementById('ctl-srcZ');
        el.value = '1';
        el.dispatchEvent(new Event('input', { bubbles: true }));
    });
    await page.waitForFunction(t => document.getElementById('elev-src').textContent.trim() !== t,
        readings.src, { timeout: SETTLE_MS }).catch(() => {});

    const clampFacts = await page.evaluate(() => ({
        src: document.getElementById('elev-src').textContent.trim(),
        chevron: !document.getElementById('elev-marker-chevron').hasAttribute('hidden'),
        dotY: Number(document.getElementById('elev-marker-dot').getAttribute('cy')),
        stripH: document.getElementById('elev-strip').getBoundingClientRect().height,
    }));

    check(parseFloat(clampFacts.src) > 7,
        `the NUMBER is exact past the axis top — source "${clampFacts.src}" against a 7 m axis`);
    check(clampFacts.chevron, 'and the MARKER clamps, with a chevron');
    check(clampFacts.dotY >= 0 && clampFacts.dotY <= clampFacts.stripH,
        `the clamped dot stays inside the strip — cy ${clampFacts.dotY.toFixed(1)} in ${clampFacts.stripH} px`);

    await page.evaluate(() => {
        const el = document.getElementById('ctl-srcZ');
        el.value = '0.2';
        el.dispatchEvent(new Event('input', { bubbles: true }));
    });

    // ── UI-05/3 — THE §OQ4 GRADING IS VISIBLE ──────────────────────────────
    // The default rig is graded 4.50 -> 5.40 m, and that is not decoration: a
    // dropped (z_i − z_s)^2 term in the solve is INVISIBLE on a flat rig, which
    // is why the defaults are graded at all (DSP-01/2). A strip that drew all
    // eight at one height would hide the very thing it exists to show.
    const heights = await page.evaluate(() => {
        const dots = Array.from(document.querySelectorAll('#elev-speakers .elev-spk'));
        return {
            count: dots.length,
            ys: dots.map(d => +Number(d.getAttribute('cy')).toFixed(2)),
            returned: window.__OCTAGON_STUB__.geometry().speakers.map(s => s.z),
        };
    });

    const distinctY = new Set(heights.ys).size;
    const distinctZ = new Set(heights.returned).size;

    check(heights.count === 8, `eight speakers on the strip — got ${heights.count}`);
    check(distinctY === distinctZ,
        `the drawn heights are as distinct as the RETURNED z values — ${distinctY} vs ${distinctZ} `
        + `(z = [${heights.returned.join(', ')}])`);
    check(distinctY >= 3,
        `and the §OQ4 grading is visible rather than flat — ${distinctY} distinct y positions`);

    // ── 23 ───────────────────────────────────────────────────────────────────
    // ── 32 (v1.8.0) ─────────────────────────────────────────────────────────
    head(32, 'the Motion panel costs the column NOTHING, and the LIVE PUCK sits ON THE TRACE (D6/R6)');
    {
        // (a) THE LAYOUT CLAIM OF PLAN TASK 8 STEP 0, MEASURED: the Motion panel is three rows like
        // Position's, so the group is the same height whichever panel shows and the column below it
        // does not move. Section 21 already holds the column; this holds the equal-height premise.
        const groupH = () => page.evaluate(() => Math.round(document.getElementById('group-position').getBoundingClientRect().height));
        await page.click('#gtab-position');
        const hPos = await groupH();
        await page.click('#gtab-motion');
        const hMot = await groupH();
        check(hPos === hMot, `#group-position is the same height on both panels — Position ${hPos}, Motion ${hMot}`);
        check(await page.evaluate(() => document.querySelector('.controls-column').scrollHeight
                                     <= document.querySelector('.controls-column').clientHeight),
            'the controls column still does not overflow with the Motion panel showing');
        const seedRule = await page.evaluate(() => ({
            seedHidden: document.getElementById('cell-motionSeed').hidden,
            phaseHidden: document.getElementById('cell-motionPhase').hidden,
        }));
        check(seedRule.seedHidden && !seedRule.phaseHidden, 'Seed is hidden and Phase shown while Path is not Drift');

        // (b) THE PUCK IS ON THE TRACE — GEOMETRY, NOT ATTRIBUTES. Motion on through the page's own
        // checkbox (the stub's toggle state flips, getMeters starts returning an orbiting offset and
        // motionOn: true, and getMotionTrace returns the fixture's 128 points). At eight polled ticks
        // the rendered centre of #puck-live must lie within 1.5 px of the rendered #motion-trace
        // polyline — measured from getBoundingClientRect and the path's own d attribute, in the same
        // box (pattern_ui_gate_asserts_attributes_never_rendered_geometry).
        // Section 10's nudge may already have flipped the checkbox; set it ON explicitly.
        await page.evaluate(() => { const e = document.getElementById('ctl-motionOn'); if (!e.checked) e.click(); });
        await page.waitForFunction(() => document.getElementById('motion-trace').getAttribute('d').length > 20
                                       && !document.getElementById('puck-live').hidden,
            null, { timeout: SETTLE_MS }).catch(() => {});

        const ghost = await page.evaluate(() => document.getElementById('puck').classList.contains('puck--ghost'));
        check(ghost, 'the anchor puck is drawn as a GHOST while motion runs');

        const samples = [];
        for (let i = 0; i < 8; ++i) {
            await page.waitForTimeout(45);
            samples.push(await page.evaluate(() => {
                const box = document.getElementById('plan-layers').getBoundingClientRect();
                const live = document.getElementById('puck-live').getBoundingClientRect();
                const cx = live.left + live.width / 2 - box.left;
                const cy = live.top + live.height / 2 - box.top;
                const d = document.getElementById('motion-trace').getAttribute('d');
                const pts = [...d.matchAll(/[ML]\s*(-?[\d.]+)\s+(-?[\d.]+)/g)].map(m => [Number(m[1]), Number(m[2])]);
                if (pts.length > 1 && /Z\s*$/.test(d)) pts.push(pts[0]);
                let best = Infinity;
                for (let k = 1; k < pts.length; ++k) {
                    const [ax, ay] = pts[k - 1], [bx, by] = pts[k];
                    const dx = bx - ax, dy = by - ay;
                    const len2 = dx * dx + dy * dy;
                    const t = len2 > 0 ? Math.max(0, Math.min(1, ((cx - ax) * dx + (cy - ay) * dy) / len2)) : 0;
                    best = Math.min(best, Math.hypot(cx - (ax + t * dx), cy - (ay + t * dy)));
                }
                return { cx: +cx.toFixed(2), cy: +cy.toFixed(2), dist: +best.toFixed(3), n: pts.length };
            }));
        }
        const worst = Math.max(...samples.map(s => s.dist));
        const moved = new Set(samples.map(s => `${s.cx},${s.cy}`)).size > 1;
        check(samples[0].n >= 128, `the trace has >= 128 rendered points — ${samples[0].n}`);
        check(worst <= 1.5, `live puck centre within 1.5 px of the trace at 8 ticks — worst ${worst} px`);
        check(moved, 'and the live puck MOVED across the ticks (not parked on one vertex)');

        // (c) THE TRACE FOLLOWS THE SHAPE: a Size step re-fetches and the rendered extent grows.
        const extent = () => page.evaluate(() => {
            const d = document.getElementById('motion-trace').getAttribute('d');
            const xs = [...d.matchAll(/[ML]\s*(-?[\d.]+)\s+/g)].map(m => Number(m[1]));
            return +(Math.max(...xs) - Math.min(...xs)).toFixed(1);
        });
        const before = await extent();
        await nudge(page, 'motionSize');
        await page.waitForTimeout(120);
        const after = await extent();
        check(after > before, `the trace re-renders on a Size change — extent ${before} -> ${after} px`);

        // (d) Drift: no trace, a tail.
        await page.selectOption('#ctl-motionPath', { label: 'Drift' });
        await page.waitForTimeout(250);
        const drift = await page.evaluate(() => ({
            d: document.getElementById('motion-trace').getAttribute('d'),
            tail: document.getElementById('motion-tail').getAttribute('points').split(' ').filter(Boolean).length,
            seedShown: !document.getElementById('cell-motionSeed').hidden,
            phaseHidden: document.getElementById('cell-motionPhase').hidden,
        }));
        check(drift.d === '' && drift.tail >= 3, `Drift draws no trace and a tail — trace "${drift.d}", tail ${drift.tail} points`);
        check(drift.seedShown && drift.phaseHidden, 'Seed replaces Phase while Path is Drift');

        // Restore: motion off, Orbit, Position panel — later sections measure the resting page.
        await page.selectOption('#ctl-motionPath', { label: 'Orbit' });
        await page.evaluate(() => { const e = document.getElementById('ctl-motionOn'); if (e.checked) e.click(); });
        await page.click('#gtab-position');
        await page.waitForTimeout(80);
        check(await page.evaluate(() => document.getElementById('puck-live').hidden
                                     && !document.getElementById('puck').classList.contains('puck--ghost')),
            'motion off hides the live puck and restores the solid anchor');
    }

    head(23, 'eight meter arcs, MEASURED at their glyph positions, OUTSIDE the glyph stroke (UI-03/1, D23)');

    const meterGeom = await page.evaluate(() => {
        const out = [];
        for (let n = 1; n <= 8; ++n) {
            const g = document.getElementById(`glyph-${n}`);
            const dot = g.querySelector('.glyph-dot');
            const arc = document.getElementById(`meter-${n}`);
            const tick = document.getElementById(`mpeak-${n}`);
            out.push({
                n,
                inGlyph: arc !== null && arc.closest(`#glyph-${n}`) !== null
                         && tick !== null && tick.closest(`#glyph-${n}`) !== null,
                dotR: Number(dot.getAttribute('r')),
                dotStroke: parseFloat(getComputedStyle(dot).strokeWidth),
                arcR: Number(arc.getAttribute('r')),
                arcStroke: parseFloat(getComputedStyle(arc).strokeWidth),
            });
        }
        return out;
    });

    check(meterGeom.every(m => m.inGlyph), 'each arc and peak tick is a CHILD of its glyph group');
    check(meterGeom.every(m => m.arcR - m.arcStroke / 2 >= m.dotR + m.dotStroke / 2),
        'every arc clears the glyph stroke — '
        + `inner edge ${(meterGeom[0].arcR - meterGeom[0].arcStroke / 2).toFixed(2)} `
        + `>= dot outer ${(meterGeom[0].dotR + meterGeom[0].dotStroke / 2).toFixed(2)} px`);

    // ── UI-03/2 — THE SPEAKER THAT LIGHTS IS THE SPEAKER THAT SOUNDS ───────
    // Driven one-hot, the way a verify ping drives it. A meter wired to the
    // wrong index looks entirely plausible on a symmetric rig, which is why
    // this walks all eight rather than checking one.
    const litOnly = async (n) => {
        await page.evaluate(k => window.__OCTAGON_STUB__.pingMeter(k), n);
        await page.waitForFunction(k => {
            const a = document.getElementById(`meter-${k}`).getAttribute('stroke-dasharray');
            return a !== null && parseFloat(a) > 1;
        }, n, { timeout: SETTLE_MS }).catch(() => {});
        return page.evaluate(() => {
            const filled = [];
            for (let i = 1; i <= 8; ++i) {
                const a = document.getElementById(`meter-${i}`).getAttribute('stroke-dasharray');
                if (a !== null && parseFloat(a) > 1) filled.push(i);
            }
            return filled;
        });
    };

    let wrongIndex = [];
    for (let n = 1; n <= 8; ++n) {
        const filled = await litOnly(n);
        if (!(filled.length === 1 && filled[0] === n)) wrongIndex.push(`${n}->[${filled.join(',')}]`);
        // Let the decay run the previous one back down before the next step.
        await page.evaluate(() => window.__OCTAGON_STUB__.setMeters([0, 0, 0, 0, 0, 0, 0, 0]));
        await page.waitForFunction(() => {
            for (let i = 1; i <= 8; ++i) {
                const a = document.getElementById(`meter-${i}`).getAttribute('stroke-dasharray');
                if (a !== null && parseFloat(a) > 1) return false;
            }
            return true;
        }, null, { timeout: SETTLE_MS }).catch(() => {});
    }

    check(wrongIndex.length === 0,
        'a ping stepping 1 -> 8 lights the MATCHING arc and the other seven read zero'
        + (wrongIndex.length ? ` — MISMATCHES: ${wrongIndex.join(' ')}` : ''));

    // ── UI-03/1 — RENDERED GEOMETRY. IS THE ARC ACTUALLY *ON* ITS GLYPH? ───
    //
    // Until v1.3.1 this section asserted DOM parentage, the `r` attributes and
    // the `stroke-dasharray` attribute — and passed, green, through a shipped
    // release in which all eight arcs rendered 507 px from their speakers in
    // BOTH engines (CODE_REVIEW CR-01). Every attribute it read was correct;
    // the CSS `transform-origin: center` on an SVG element resolved against the
    // viewBox and threw the rotation origin half a plan away, which no
    // attribute can show. A probe that passes whether or not the bug is present
    // is decoration (pattern_probe_must_target_the_branch_the_fix_changed), so
    // every clause below reads a RENDERED position: getScreenCTM for where the
    // rotation origin landed, getBoundingClientRect for where the ink landed.
    //
    // NEGATIVE CONTROL, run before this block was accepted: with
    // `transform-origin: center` put back in styles.css, clauses 2-6 fail 8/8
    // (arc origin AND ink 507.1 px off, tick radius 28-696 px instead of 15) —
    // in Chromium 141 and, re-run by hand, in WebKit 26.5. This is the failure
    // the gate owed the shipped build and did not raise.
    //
    // Eight DISTINCT peaks, so the eight ticks sit at eight different angles: a
    // tick resting at 0 deg is at its authored position, where a wrong origin
    // cannot displace it, and would satisfy clause 6 no matter what. Clause 7
    // refuses to let a flat set stand in for a swept one.
    await page.evaluate(() =>
        window.__OCTAGON_STUB__.setMeters([0.9, 0.7, 0.55, 0.42, 0.3, 0.2, 0.12, 0.06]));
    await page.waitForFunction(() => {
        for (let i = 1; i <= 8; ++i) {
            const t = document.getElementById(`mpeak-${i}`).getAttribute('transform');
            if (t === null || !/rotate\(\s*[\d.]/.test(t)) return false;
        }
        return true;
    }, null, { timeout: SETTLE_MS }).catch(() => {});

    // The angle and the position are read in the SAME evaluate, so the tick's
    // transform attribute and the tick's rendered point cannot skew across a
    // ballistics frame and manufacture a failure out of timing.
    const rendered = await page.evaluate(() => {
        const at = (el, x, y) => {
            const q = new DOMPoint(x, y).matrixTransform(el.getScreenCTM());
            return { x: q.x, y: q.y };
        };
        const inkCentre = (el) => {
            const b = el.getBoundingClientRect();
            return { x: b.x + b.width / 2, y: b.y + b.height / 2 };
        };
        const out = [];
        for (let n = 1; n <= 8; ++n) {
            const g    = document.getElementById(`glyph-${n}`);
            const dot  = g.querySelector('.glyph-dot');
            const arc  = document.getElementById(`meter-${n}`);
            const tick = document.getElementById(`mpeak-${n}`);

            // The glyph centre, MEASURED: .glyph-dot carries no cx/cy and no
            // transform of its own, so its local origin is the speaker itself.
            const centre  = at(dot, 0, 0);
            const arcAt   = at(arc, 0, 0);

            // The tick's radius comes from the tick's OWN y1/y2, never from a
            // literal 15 (pattern_test_fixture_mirrors_drift_silently), and its
            // angle from the attribute js/roomplan.js actually wrote.
            const mid    = (Number(tick.getAttribute('y1')) + Number(tick.getAttribute('y2'))) / 2;
            const deg    = parseFloat(String(tick.getAttribute('transform')).replace(/^\D*/, ''));
            const rad    = (deg * Math.PI) / 180;
            const tickAt = at(tick, 0, mid);

            out.push({
                n,
                arcBox:     getComputedStyle(arc).transformBox,
                arcOrigin:  getComputedStyle(arc).transformOrigin,
                tickBox:    getComputedStyle(tick).transformBox,
                tickOrigin: getComputedStyle(tick).transformOrigin,

                // Where the arc's rotation origin landed, and where its ink
                // landed. Both have to be the glyph centre, and they fail
                // independently — the first is the transform, the second is
                // the paint.
                arcOriginOff: Math.hypot(arcAt.x - centre.x, arcAt.y - centre.y),
                arcInkOff:    Math.hypot(inkCentre(arc).x - inkCentre(dot).x,
                                         inkCentre(arc).y - inkCentre(dot).y),

                deg,
                arcR:       Number(arc.getAttribute('r')),
                wantRadius: Math.abs(mid),
                tickRadius: Math.hypot(tickAt.x - centre.x, tickAt.y - centre.y),

                // SVG rotate(a) sends (0, mid) to (-mid*sin a, mid*cos a) about
                // the origin it is given. Predicted from the tick's own angle,
                // compared against the MEASURED point.
                tickErr: Math.hypot(tickAt.x - (centre.x - mid * Math.sin(rad)),
                                    tickAt.y - (centre.y + mid * Math.cos(rad))),
            });
        }
        return out;
    });

    // The defect this catches is 507 px. Residual error with the fix in place
    // is under 0.05 px in both engines, so half a pixel is a wide moat.
    const GEOM_TOL = 0.5;
    const worst = (k) => Math.max(...rendered.map(m => m[k])).toFixed(3);
    const bad   = (k) => rendered.filter(m => !near(m[k], 0, GEOM_TOL));
    const list  = (ms, k) => ms.map(m => `spk${m.n} ${m[k].toFixed(1)}px`).join(', ');

    check(rendered.every(m => m.arcBox === 'view-box' && m.tickBox === 'view-box'),
        'transform-box is left at the SVG default on arc and tick — '
        + `got ${rendered[0].arcBox} / ${rendered[0].tickBox}`);

    check(rendered.every(m => m.arcOrigin === '0px 0px' && m.tickOrigin === '0px 0px'),
        'so transform-origin must name the glyph-local origin outright, and computes to 0px 0px — '
        + `got ${rendered[0].arcOrigin} / ${rendered[0].tickOrigin} `
        + '(224px 280px is the viewBox centre, i.e. CR-01 is back)');

    const originBad = bad('arcOriginOff');
    check(originBad.length === 0,
        'every arc ROTATES ABOUT its own glyph centre — getScreenCTM(0,0) lands on the dot'
        + (originBad.length ? ` — OFF BY: ${list(originBad, 'arcOriginOff')}`
                            : ` (worst ${worst('arcOriginOff')} px of 8)`));

    const inkBad = bad('arcInkOff');
    check(inkBad.length === 0,
        'and every arc PAINTS on its own glyph — rendered box centre lands on the dot\'s'
        + (inkBad.length ? ` — OFF BY: ${list(inkBad, 'arcInkOff')}`
                         : ` (worst ${worst('arcInkOff')} px of 8)`));

    check(rendered.every(m => near(m.wantRadius, m.arcR, GEOM_TOL)),
        'the peak tick straddles the arc it marks — tick |y1+y2|/2 = '
        + `${rendered[0].wantRadius} matches the arc r = ${rendered[0].arcR}`);

    const radiusBad = rendered.filter(m => !near(m.tickRadius, m.wantRadius, GEOM_TOL));
    check(radiusBad.length === 0,
        'and every peak tick ORBITS at that radius instead of flying off-plan'
        + (radiusBad.length
            ? ` — MEASURED: ${radiusBad.map(m => `spk${m.n} ${m.tickRadius.toFixed(1)}px want ${m.wantRadius}`).join(', ')}`
            : ` (all eight ${rendered[0].tickRadius.toFixed(2)} px)`));

    const angleBad = bad('tickErr');
    check(angleBad.length === 0,
        'landing at the angle its own transform attribute asked for'
        + (angleBad.length
            ? ` — OFF BY: ${angleBad.map(m => `spk${m.n} rotate(${m.deg.toFixed(1)}) ${m.tickErr.toFixed(1)}px`).join(', ')}`
            : ` (worst ${worst('tickErr')} px of 8)`));

    const degs   = rendered.map(m => m.deg);
    const spread = Math.max(...degs) - Math.min(...degs);
    const fromRest = Math.max(...degs.map(d => Math.abs(((d + 180) % 360) - 180)));
    check(spread > 90 && fromRest > 30,
        `[non-vacuity] the eight ticks were measured across ${spread.toFixed(0)} deg of sweep, the `
        + `furthest ${fromRest.toFixed(0)} deg from rest — an unrotated tick sits where it was `
        + 'authored and CANNOT be displaced by a wrong origin, so a flat set would prove nothing');

    // Leave the meters where the next section expects them.
    await page.evaluate(() => window.__OCTAGON_STUB__.setMeters([0, 0, 0, 0, 0, 0, 0, 0]));

    // ── 24 ───────────────────────────────────────────────────────────────────
    head(24, 'the scene preview responds to HOVER *and* KEYBOARD FOCUS (D21/FUNC-06/3)');

    const previewed = () => page.evaluate(() =>
        [1, 2, 3, 4, 5, 6, 7, 8].filter(n => document.getElementById(`glyph-${n}`).classList.contains('is-preview')));
    const returnedSet = (id) => page.evaluate(
        k => window.__OCTAGON_STUB__.scenes().find(s => s.id === k).indices, id);

    check((await previewed()).length === 0, 'nothing is previewed at rest');

    await page.hover('#scene-FRONT');
    await page.waitForFunction(() => document.querySelectorAll('.glyph.is-preview').length > 0,
        null, { timeout: SETTLE_MS }).catch(() => {});

    const hoverSet = await previewed();
    const frontSet = await returnedSet('FRONT');
    check(hoverSet.join(',') === frontSet.join(','),
        `HOVER lights the set the plugin RETURNED — [${hoverSet.join(',')}] vs [${frontSet.join(',')}]`);

    await page.hover('#tab-room');
    await page.waitForFunction(() => document.querySelectorAll('.glyph.is-preview').length === 0,
        null, { timeout: SETTLE_MS }).catch(() => {});
    check((await previewed()).length === 0, 'and it clears on pointer leave');

    // KEYBOARD FOCUS GIVES THE IDENTICAL PREVIEW. A pointer-only preview
    // satisfies FUNC-06/3 for a user who happens to hover and for nobody else.
    await page.focus('#scene-SIDES');
    await page.waitForFunction(() => document.querySelectorAll('.glyph.is-preview').length > 0,
        null, { timeout: SETTLE_MS }).catch(() => {});

    const focusSet = await previewed();
    const sidesSet = await returnedSet('SIDES');
    check(focusSet.join(',') === sidesSet.join(','),
        `FOCUS lights the same way — [${focusSet.join(',')}] vs the returned [${sidesSet.join(',')}]`);
    check(focusSet.join(',') !== frontSet.join(','),
        'and it is a DIFFERENT set from the hover case, so neither check is passing on a constant');

    await page.evaluate(() => document.getElementById('scene-SIDES').blur());
    await page.waitForFunction(() => document.querySelectorAll('.glyph.is-preview').length === 0,
        null, { timeout: SETTLE_MS }).catch(() => {});

    // ── 25 ───────────────────────────────────────────────────────────────────
    head(25, 'an EMPTY scene is legible and NOT writable (D20)');

    // All-zero weights are DSP-05's silence path, and reaching it by a
    // mis-derived scene click mid-concert is unrecoverable. The disabled
    // control is the AFFORDANCE; applyScene's { ok:false, reason:"emptyScene" }
    // in C++ is the GUARANTEE, and BOTH are asserted — here and in frontend §3.
    const emptyFacts = await page.evaluate(() => {
        const b = document.getElementById('scene-slot4');
        return { disabled: b.disabled, empty: b.dataset.empty, occupied: b.classList.contains('is-occupied') };
    });

    check(emptyFacts.disabled === true && emptyFacts.empty === 'true',
        `an uncaptured slot is disabled and marked empty — disabled=${emptyFacts.disabled}, `
        + `data-empty="${emptyFacts.empty}"`);
    check(emptyFacts.occupied === false, 'and it is not styled as occupied');

    const writesBefore = await page.evaluate(() => JSON.stringify(window.__OCTAGON_STUB__.writes));
    await page.click('#scene-slot4', { force: true });
    await page.waitForTimeout(120);
    const writesAfter = await page.evaluate(() => JSON.stringify(window.__OCTAGON_STUB__.writes));

    // "THE PAGE DID NOT ASK" IS A DIFFERENT CLAIM FROM "THE PLUGIN SAID NO",
    // and only the first one keeps eight zeros off the PA. This is the same
    // assertion shape section 15 makes for the colliding label set.
    check(writesBefore === writesAfter,
        'clicking it writes NOTHING — not one of the eight weights moved');

    const applyCalls = await page.evaluate(() => Number(window.__OCTAGON_STUB__.calls.applyScene || 0));
    await page.click('#scene-slot4', { force: true });
    await page.waitForTimeout(120);
    const applyCallsAfter = await page.evaluate(() => Number(window.__OCTAGON_STUB__.calls.applyScene || 0));
    check(applyCalls === applyCallsAfter,
        `and applyScene was never invoked — ${applyCalls} == ${applyCallsAfter} calls`);

    // The POSITIVE control: a non-empty scene DOES commit, so the check above
    // is not passing because the whole surface is inert.
    const before8 = await page.evaluate(() =>
        [1, 2, 3, 4, 5, 6, 7, 8].map(n => Number(document.getElementById(`ctl-w${n}`).value)));
    await page.click('#scene-REAR');
    await page.waitForFunction(() => Number(document.getElementById('ctl-w1').value) === 0,
        null, { timeout: SETTLE_MS }).catch(() => {});
    const after8 = await page.evaluate(() =>
        [1, 2, 3, 4, 5, 6, 7, 8].map(n => Number(document.getElementById(`ctl-w${n}`).value)));
    const rearSet = await returnedSet('REAR');

    check(after8.join(',') !== before8.join(','),
        `[control] a NON-empty scene does commit — [${after8.map(v => v.toFixed(2)).join(',')}]`);
    check(after8.map((v, i) => (v > 0 ? i + 1 : null)).filter(v => v !== null).join(',') === rearSet.join(','),
        `and it wrote exactly the RETURNED set — [${rearSet.join(',')}]`);

    // The eight gesture brackets D18 requires, observed rather than argued.
    const sceneGestures = await page.evaluate(() => {
        const g = window.__OCTAGON_STUB__.gestures;
        const w = g.filter(e => /^w[1-8]$/.test(e.id));
        const open = {};
        let unclosed = 0;
        for (const e of w) {
            if (e.phase === 'start') open[e.id] = (open[e.id] || 0) + 1;
            else open[e.id] = (open[e.id] || 0) - 1;
        }
        for (const k of Object.keys(open)) if (open[k] !== 0) ++unclosed;
        return { total: w.length, unclosed };
    });
    check(sceneGestures.unclosed === 0,
        `every weight gesture opened by a scene commit was CLOSED — ${sceneGestures.total} bracket events, `
        + `${sceneGestures.unclosed} unclosed`);

    await page.click('#scene-ALL');
    await page.waitForTimeout(120);

    // ── 26 ───────────────────────────────────────────────────────────────────
    head(26, '#plan-backdrop carries the field with an explicit DPR backing store (UI-04/3)');

    // Canvas is a CSS REPLACED ELEMENT: left + right does NOT stretch it, and
    // the collapse is silent (o-textureforge-cursor-bug). Section 6 already
    // measures the backing store at both DPRs; this section is about the FIELD
    // being on that surface at all, and about the legend that makes it mean
    // something.
    const backdrop = await page.evaluate(() => {
        const c = document.getElementById('plan-backdrop');
        const cs = getComputedStyle(c);
        const r = c.getBoundingClientRect();
        const ctx = c.getContext('2d');
        // Sample the interior. A field that decoded to nothing leaves the flat
        // #241E1A envelope fill, so the test is whether ANY interior pixel
        // differs from that constant.
        const d = ctx.getImageData(Math.round(r.width * 0.35), Math.round(r.height * 0.5), 40, 40).data;
        let varied = 0;
        for (let i = 0; i < d.length; i += 4)
            if (d[i] !== 0x24 || d[i + 1] !== 0x1E || d[i + 2] !== 0x1A) ++varied;
        return {
            widthCss: cs.width, heightCss: cs.height,
            backing: { w: c.width, h: c.height },
            rect: { w: +r.width.toFixed(1), h: +r.height.toFixed(1) },
            dpr: window.devicePixelRatio,
            varied,
            legend: document.getElementById('field-legend').textContent.trim(),
            grid: window.__OCTAGON_STUB__.field(),
        };
    });

    check(backdrop.widthCss !== 'auto' && backdrop.heightCss !== 'auto',
        `the canvas has an explicit CSS box — ${backdrop.widthCss} x ${backdrop.heightCss}`);
    check(backdrop.backing.w === Math.round(backdrop.rect.w * backdrop.dpr)
          && backdrop.backing.h === Math.round(backdrop.rect.h * backdrop.dpr),
        `backing store ${backdrop.backing.w} x ${backdrop.backing.h} == round(rect x dpr ${backdrop.dpr})`);
    check(backdrop.varied > 0,
        `the field is actually PAINTED — ${backdrop.varied} of 1600 sampled subpixels differ from the flat fill`);

    // THE LEGEND IS THE DIFFERENCE BETWEEN A GRADIENT AND A DECORATION. The
    // field over a raked audience plane is genuinely flat, so a normalised ramp
    // with no printed span looks informative and is not.
    check(/\d+\.\d\s+–\s+\d+\.\d\s+dB/.test(backdrop.legend),
        `and the dB span is printed beside it — "${backdrop.legend}"`);
    check(backdrop.legend.startsWith(Number(backdrop.grid.minDb).toFixed(1)),
        `the legend is the span the plugin RETURNED — ${Number(backdrop.grid.minDb).toFixed(1)} dB`);

    // ── 27 ───────────────────────────────────────────────────────────────────
    head(27, 'a PUCK DRAG does not recompute the field — counted, not eyeballed (UI-04/2)');

    // UI-04 criterion 2 requires a COUNTER. The field depends on speaker
    // positions, the eight weights, rolloff, blur and hullAtten — FIVE inputs,
    // three of them automatable at audio rate (N12) — and on NONE of
    // srcX / srcY / srcZ / width. Dragging the puck across many frames must
    // therefore leave the invocation count exactly where it was.
    //
    // COUNTING INVOCATIONS, NOT WRITES: a recompute that never happened leaves
    // no parameter trace at all, which is why the stub gained CALLS at 3.3.
    const fieldCalls = () => page.evaluate(() => Number(window.__OCTAGON_STUB__.calls.getFieldGrid || 0));

    const puck = await page.$('#puck');
    const pb = await puck.boundingBox();

    // ── QUIESCE BEFORE SAMPLING THE BASELINE (added 4.2) ─────────────────────
    // This section was FLAKY at roughly 1 run in 5, on the pristine 3.3 file as
    // well as with 4.2's section 28 present (measured interleaved, 1/5 and 1/5).
    // It failed reading "11 -> 12": one recompute inside the drag window that
    // the drag did not cause.
    //
    // field.js:161 — refresh() runs AT MOST ONCE PER STATUS TICK, and the tick
    // is 2 Hz. So a field input marked dirty by an EARLIER section is owed a
    // recompute that lands whenever the next tick fires — including part-way
    // through this section's 24-frame drag, where it reads as a drag-caused
    // recompute. The measurement window overlapped the poll period
    // (pattern_metric_window_vs_modulation_period).
    //
    // The assertion is unchanged. Only its starting state is: wait until the
    // count has been STABLE across three consecutive ticks before sampling, so
    // the window opens with nothing owed.
    let quiesced = -1;
    let stableTicks = 0;
    for (let i = 0; i < 12 && stableTicks < 3; ++i) {
        await page.waitForTimeout(STATUS_POLL_MS);
        const n = await fieldCalls();
        stableTicks = (n === quiesced) ? stableTicks + 1 : 0;
        quiesced = n;
    }

    // If it never settles, that is a FINDING about the page, not a reason to
    // measure anyway — an unsettled baseline is what made this section flaky.
    check(stableTicks >= 3,
        `[precondition] the recompute count QUIESCED before the drag window opened `
        + `— stable at ${quiesced} across ${stableTicks} consecutive ${STATUS_POLL_MS} ms ticks`);

    const callsBeforeDrag = await fieldCalls();

    // OFF-CENTRE, for the reason section 4 grabs off-centre: a centred grab
    // passes under absolute tracking too.
    await page.mouse.move(pb.x + pb.width * 0.75, pb.y + pb.height * 0.75);
    await page.mouse.down();
    for (let i = 0; i < 24; ++i) {
        await page.mouse.move(pb.x + pb.width * 0.75 + i * 3, pb.y + pb.height * 0.75 + i * 2);
    }
    await page.mouse.up();
    await page.waitForTimeout(120);

    const callsAfterDrag = await fieldCalls();
    const dragWrites = await page.evaluate(() => ({
        x: Number(window.__OCTAGON_STUB__.writes.srcX || 0),
        y: Number(window.__OCTAGON_STUB__.writes.srcY || 0),
    }));

    check(dragWrites.x > 5 && dragWrites.y > 5,
        `[non-vacuity] the drag really happened — ${dragWrites.x} srcX / ${dragWrites.y} srcY writes`);
    check(callsAfterDrag === callsBeforeDrag,
        `24 frames of puck drag left the recompute count UNCHANGED — ${callsBeforeDrag} -> ${callsAfterDrag}`);

    // THE POSITIVE CONTROL. One of the five real inputs must move it, or the
    // assertion above is passing because the field never recomputes at all.
    await page.evaluate(() => {
        const el = document.getElementById('ctl-blur');
        el.value = '0.8';
        el.dispatchEvent(new Event('input', { bubbles: true }));
    });
    await page.waitForFunction(n => Number(window.__OCTAGON_STUB__.calls.getFieldGrid || 0) > n,
        callsAfterDrag, { timeout: SETTLE_MS * 2 }).catch(() => {});

    const callsAfterBlur = await fieldCalls();
    check(callsAfterBlur > callsAfterDrag,
        `[control] moving BLUR — one of the five real inputs — DID move it: `
        + `${callsAfterDrag} -> ${callsAfterBlur}`);

    check(consoleErrors.length === 0,
        'no console errors after driving scenes, meters, the field and the strip'
        + (consoleErrors.length ? ` — ${consoleErrors[0]}` : ''));

    // ══════════════════════════════════════════════════════════════════════
    // PHASE 4.2 — section 28. Q5's MECHANISM, executed.
    //
    // Five phases have described this mechanism and none has run it. The host
    // half of Gate 13 — minimise / ⌘H and watch the meters recover — CANNOT
    // reach it: Component::isVisible() is the component's own flag, set once at
    // PluginEditor.cpp:1139 and never cleared, and it stays true under minimise,
    // ⌘H, occlusion and Spaces (RESEARCH-4.2 §3.3). So that half never drops a
    // completion, and what it actually observes is WebKit throttling. It is kept
    // and RELABELLED as a throttling-recovery smoke check (P101).
    //
    // Frontend §33 is a STATIC section: it greps meters.js for performance.now()
    // and for a deadline-shaped comparison. It proves the deadline is WRITTEN.
    // It cannot prove it RELEASES. That gap is the four-phase-old premise and it
    // closes here.
    //
    // THE READOUT PROBLEM (N9), AND WHY THIS CONSTRUCTS ITS OWN INSTANCE.
    // The app's live `meters` binding is a module-scope `let` in app.js:639 with
    // no window handle, so page.evaluate cannot reach it either. meters.js:116
    // exports createMeters(deps), a factory taking deps.nativeFn — so the
    // section builds a FRESH instance whose getMeters it controls and whose
    // diagnostics() it holds directly. No source change, no window handle, and
    // the module under test is the SHIPPED FILE BYTE-FOR-BYTE.
    // Rejected: exposing meters.diagnostics on window. It would ride P110's
    // re-freeze to buy a convenience the dynamic import already supplies.
    // ══════════════════════════════════════════════════════════════════════

    // ── 28 ───────────────────────────────────────────────────────────────────
    head(28, 'the in-flight guard RELEASES ON ITS DEADLINE and the poll survives a dropped completion (Q5)');

    // Measured constants, re-read from the shipped module rather than mirrored
    // here (pattern_test_fixture_mirrors_drift_silently): the deadline is
    // METER_POLL_MS * GUARD_DEADLINE_TICKS = 33 * 5 = 165 ms. The section waits
    // ~480 ms — more than two deadlines, with margin for a loaded machine.
    const metersSrc = fs.readFileSync(path.join(publicDir, 'js', 'meters.js'), 'utf8');
    const pollMs  = Number((metersSrc.match(/METER_POLL_MS\s*=\s*(\d+)/)       || [])[1]);
    const ticks   = Number((metersSrc.match(/GUARD_DEADLINE_TICKS\s*=\s*(\d+)/) || [])[1]);
    const deadlineMs = pollMs * ticks;

    check(Number.isFinite(deadlineMs) && deadlineMs > 0,
        `the deadline is READ from the shipped module — ${pollMs} ms x ${ticks} ticks = ${deadlineMs} ms`);

    // One constant, used for the wait AND for every message about it, so a
    // report cannot quote a duration the run did not use.
    const GUARD_WAIT_MS = 480;

    const guard = await page.evaluate(async ({ origin, waitMs }) => {
        const { createMeters } = await import(`${origin}/js/meters.js`);

        let calls = 0;
        const getMeters = () => {
            ++calls;
            // THE FIRST REQUEST NEVER SETTLES. This is the completion the
            // WebView drops — the one a settlement-only guard waits for
            // forever (pattern_webview_completion_gated_on_isvisible).
            if (calls === 1) return new Promise(() => {});
            return Promise.resolve({ peaks: new Array(8).fill(0.25) });
        };

        const m = createMeters({
            nativeFn: (name) => (name === 'getMeters' ? getMeters : () => Promise.resolve(null)),
            onLevels: () => {},
        });

        m.start();

        // Sample so the ORDERING is observed, not inferred: record how many
        // calls had been made at the moment `dropped` first went non-zero.
        let callsWhenDropSeen = null;
        let droppedSeen = 0;
        for (let i = 0; i < Math.ceil(waitMs / 20); ++i) {
            await new Promise(r => setTimeout(r, 20));
            const d = m.diagnostics();
            if (callsWhenDropSeen === null && d.dropped >= 1) {
                callsWhenDropSeen = calls;
                droppedSeen = d.dropped;
            }
        }

        const final = m.diagnostics();
        m.stop();
        return { dropped: final.dropped, calls, callsWhenDropSeen, droppedSeen };
    }, { origin: `http://127.0.0.1:${port}`, waitMs: GUARD_WAIT_MS });

    // Clause 1 — the guard released on the DEADLINE. A settlement-only guard
    // would sit at 0 here forever, because request 1 never settles.
    check(guard.dropped >= 1,
        `the guard RELEASED on its ${deadlineMs} ms deadline against a never-settling request `
        + `— dropped = ${guard.dropped}`);

    // Clause 2 — and the poll CONTINUED. This is the clause that matters:
    // `dropped` rising only proves a counter moved; the call count rising AFTER
    // the drop proves the poll is not latched, which is the property the whole
    // pattern is about.
    check(guard.callsWhenDropSeen !== null && guard.calls > guard.callsWhenDropSeen,
        `and the poll CONTINUED past it — getMeters had been called ${guard.callsWhenDropSeen} time(s) `
        + `when the drop was first observed, ${guard.calls} by the end`);

    // [non-vacuity] If this reads 1, the stimulus never reached the module and
    // both clauses above are passing on nothing.
    //
    // NOTE THE CLAIM THIS DOES *NOT* MAKE. On the SHIPPED guard, request 1 never
    // settles, so call 2 is reachable only through the deadline — and it would
    // be tempting to write that here as if the count alone proved the mechanism.
    // It does not: NC1 deletes the guard outright, and the count then rises for
    // the opposite reason (nothing is holding the poll back at all). NC1 was run
    // and this line was corrected because of what it showed. The mechanism is
    // proved by clauses 1 and 2; this clause only proves the stimulus arrived.
    check(guard.calls >= 2,
        `[non-vacuity] the stimulus REACHED the module — ${guard.calls} getMeters calls in `
        + `${GUARD_WAIT_MS} ms (a latched poll would read exactly 1)`);

    // ── 29 ───────────────────────────────────────────────────────────────────
    head(29, 'a DPR change with NO resize event rebuilds the plan backing store (WR-02)');

    // WHAT THIS REPLACES. Section 21 measures the elevation strip at DPR 1 and
    // DPR 2 in two SEPARATE browser contexts — construction-time DPR only. It
    // therefore cannot see a LIVE transition, which is the case roomplan.js's
    // own comment describes ("a window dragged between a Retina and a
    // non-Retina display changes DPR without changing any CSS size"): until
    // v1.3.2 that comment sat above `window.addEventListener("resize", ...)`,
    // an event that cannot fire for it. The editor is a fixed 1100 x 720
    // non-resizable surface, so the CSS viewport never changes, and a
    // backing-scale change with an unchanged CSS viewport dispatches no resize.
    //
    // Playwright cannot change deviceScaleFactor on a live page, so the harness
    // below substitutes devicePixelRatio and matchMedia BEFORE the page's own
    // scripts run and fires the media query the way a UA does. That is a
    // fixture for the ENVIRONMENT, not for the module: roomplan.js is the
    // shipped file, unmodified, and every assertion reads a rendered
    // backing-store size.
    //
    // The clause that makes it non-vacuous is `resizes === 0`. If a resize
    // event had fired, the OLD code would pass this section too.
    const dprPage = await context.newPage();

    await dprPage.addInitScript(() => {
        let ratio = 1;
        let resizes = 0;
        const armed = [];

        Object.defineProperty(window, 'devicePixelRatio', { configurable: true, get: () => ratio });

        window.matchMedia = (query) => {
            const mq = {
                media: query,
                matches: true,
                listeners: [],
                addEventListener(type, fn) { if (type === 'change') mq.listeners.push(fn); },
                removeEventListener(type, fn) {
                    const i = mq.listeners.indexOf(fn);
                    if (i >= 0) mq.listeners.splice(i, 1);
                },
                addListener(fn) { mq.listeners.push(fn); },
                removeListener(fn) {
                    const i = mq.listeners.indexOf(fn);
                    if (i >= 0) mq.listeners.splice(i, 1);
                },
            };
            armed.push(mq);
            return mq;
        };

        // Registered before the page's own listeners, so it counts every resize
        // the page could possibly have seen.
        window.addEventListener('resize', () => { ++resizes; });

        window.__DPR_HARNESS__ = {
            resizes: () => resizes,
            // Queries that still hold a live listener — a one-shot hook that
            // was never re-armed reads 0 here after the first fire.
            armedQueries: () => armed.filter(m => m.listeners.length > 0).map(m => m.media),
            setRatio(next) {
                ratio = next;
                // Fire exactly what the UA fires: every armed query whose stated
                // resolution no longer matches. No resize event.
                for (const mq of armed.slice()) {
                    const m = /\(resolution:\s*([0-9.]+)dppx\)/.exec(mq.media);
                    if (m === null || Number(m[1]) === next) continue;
                    mq.matches = false;
                    for (const fn of mq.listeners.slice()) fn.call(mq, { matches: false, media: mq.media });
                }
            },
        };
    });

    await dprPage.goto(`http://127.0.0.1:${port}/index.html`, { waitUntil: 'networkidle' });
    await dprPage.waitForFunction(
        () => document.getElementById('readout-envelope').textContent !== '—',
        null, { timeout: 10000 }).catch(() => {});

    const readBacking = () => dprPage.evaluate(() => {
        const c = document.getElementById('plan-backdrop');
        const r = c.getBoundingClientRect();
        return { store: c.width, cssW: r.width, armed: window.__DPR_HARNESS__.armedQueries() };
    });

    const dprBefore = await readBacking();

    check(dprBefore.cssW > 0 && dprBefore.store > 0,
        `plan backdrop rendered at DPR 1 — ${dprBefore.cssW.toFixed(1)} css px, `
        + `${dprBefore.store} px backing store`);
    check(dprBefore.armed.some(q => /resolution:\s*1dppx/.test(q)),
        `[non-vacuity] the module ARMED a resolution watch at construction — [${dprBefore.armed.join(', ')}]`);

    await dprPage.evaluate(() => window.__DPR_HARNESS__.setRatio(2));
    await dprPage.waitForFunction(
        w => document.getElementById('plan-backdrop').width !== w,
        dprBefore.store, { timeout: SETTLE_MS }).catch(() => {});

    const dprAfter = await readBacking();
    const dprResizes = await dprPage.evaluate(() => window.__DPR_HARNESS__.resizes());

    // Clause 1 — the backing store followed the ratio. The CSS box is unchanged
    // (that is the whole premise), so the store must have DOUBLED.
    check(near(dprAfter.store / Math.max(dprBefore.store, 1), 2, 0.02),
        `the backing store REBUILT at the new ratio — ${dprBefore.store} -> ${dprAfter.store} px `
        + `(css width unchanged, ${dprBefore.cssW.toFixed(1)} -> ${dprAfter.cssW.toFixed(1)})`);

    // Clause 2 — and it did NOT arrive through a resize. Without this the old
    // resize-only code passes the section.
    check(dprResizes === 0,
        `and NOT through a resize event — ${dprResizes} resize events dispatched in the whole run`);

    // Clause 3 — the one-shot trap. A `(resolution: Ndppx)` query stops matching
    // the moment the ratio moves and never fires again, so a hook armed once at
    // construction is good for exactly one transition. This asserts the re-arm.
    check(dprAfter.armed.some(q => /resolution:\s*2dppx/.test(q)),
        `and the watch RE-ARMED against the new ratio — [${dprAfter.armed.join(', ')}]`);

    await dprPage.close();

    // ── 30 ───────────────────────────────────────────────────────────────────
    head(30, 'the footer METRES readout follows srcX from the CONTROLS COLUMN, not just the puck (WR-04)');

    // Section 14 moves the metres readout by editing the VENUE — that path runs
    // through refreshGeometry(), which calls renderMetres() itself. It says
    // nothing about the source moving. Until v1.3.2 renderMetres() had exactly
    // one live-update subscriber: roomplan's onSourceMoved, called from ONE
    // place — the puck's pointermove handler. Dragging ctl-srcX, stepping it
    // from the keyboard, its dblclick reset and host automation all moved the
    // puck and updated val-srcX while the footer went on showing a plausible
    // WRONG position, which is worse than a blank one on a live-hall
    // calibration tool. index.html's own Source X tooltip promises "the metres
    // readout below is live".
    //
    // Driven with a KEY on the slider, never the puck: a pointer gesture on the
    // puck is the one path that always worked.
    await showRoom();

    const metresBefore = await page.evaluate(() => document.getElementById('readout-metres').textContent);
    const valXBefore   = await page.evaluate(() => document.getElementById('val-srcX').textContent);

    const nudged = await nudge(page, 'srcX');
    await page.waitForFunction(t => document.getElementById('readout-metres').textContent !== t,
        metresBefore, { timeout: SETTLE_MS }).catch(() => {});

    const metresAfter = await page.evaluate(() => document.getElementById('readout-metres').textContent);
    const valXAfter   = await page.evaluate(() => document.getElementById('val-srcX').textContent);

    // [non-vacuity] The stimulus has to have reached the ECHO, or both clauses
    // below are passing on nothing. val-srcX is written by bindSlider's own
    // render on the same valueChangedEvent the fix subscribes to, so if this
    // moved, the echo fired.
    check(nudged && valXAfter !== valXBefore,
        `[non-vacuity] a key on ctl-srcX reached the parameter echo — val-srcX "${valXBefore}" -> "${valXAfter}"`);

    check(metresBefore !== '—' && metresBefore !== '',
        `metres readout populated before the nudge — "${metresBefore}"`);
    check(metresAfter !== metresBefore,
        `and the footer FOLLOWED a controls-column move — "${metresBefore}" -> "${metresAfter}"`);

    // ── 31 ───────────────────────────────────────────────────────────────────
    head(31, 'a C++-REJECTED venue commit repaints the table from the MODEL (WR-05)');

    // THE CASE SECTION 15 DOES NOT COVER. Section 15 is the JS-detected
    // collision: the page refuses to call setVenue at all, and HOLDS the typed
    // label on purpose (reverting would make an L <-> R swap unreachable, P53).
    // This is the other branch — the page asked and the PLUGIN said no.
    // applyVenueEditChecked() returns false before applyVenueEdit() runs, so
    // nothing publishes, venueGen never moves, and the venueGen-gated
    // refreshGeometry -> setGeometry -> paintFields chain never runs. Until
    // v1.3.2 pending.clear() had already dropped the typed value, so the input
    // went on displaying a number the model does not hold and the NEXT commit
    // would silently send the old one.
    //
    // Armed with the SAFE-mode shape (`speaker: -1`), which is the reachable
    // unbounded case: on a mono or stereo output — supported AU configurations —
    // buildSpeakerToBuffer fails with notEightChannels on EVERY commit, the
    // venue inputs are not disabled, and a -1 row index means not even the
    // is-colliding mark appears. No commit can ever succeed there, so nothing
    // ever heals the desync.
    await showVenue();
    await page.evaluate(() => window.__OCTAGON_STUB__.resetVenue());

    // ── [precondition] QUIESCE, AND WHY THIS LINE EXISTS ─────────────────────
    //
    // resetVenue() above bumps venueGen, and the refresh it triggers arrives on
    // the 2 Hz status poll — up to a poll period later. If that refresh lands
    // AFTER the rejected commit below, its own paintFields() repaints the table
    // for entirely the wrong reason and the section passes whether or not the
    // fix is present. It did exactly that: NC3 (revert venue.js, re-run) came
    // back ALL SECTIONS PASS, and this section was decoration until the wait
    // below was added (pattern_probe_must_target_the_branch_the_fix_changed).
    // With it, NC3 fails on the display clause.
    const quiesce = [];
    for (let i = 0; i < 6; ++i) {
        await page.waitForTimeout(STATUS_POLL_MS);
        quiesce.push(await page.evaluate(() =>
            `${document.getElementById('readout-envelope').textContent}|`
            + `${document.getElementById('vf-1-x').value}`));
        if (quiesce.length >= 3 && quiesce.slice(-3).every(v => v === quiesce[quiesce.length - 1])) break;
    }
    const settled = quiesce.length >= 3
        && quiesce.slice(-3).every(v => v === quiesce[quiesce.length - 1]);

    check(settled,
        `[precondition] the venue refresh QUIESCED before anything was typed — `
        + `stable at "${quiesce[quiesce.length - 1]}" across 3 consecutive ${STATUS_POLL_MS} ms ticks`);

    const rejCommitted = await page.evaluate(() => document.getElementById('vf-1-x').value);
    const rejEnvBefore = await page.evaluate(() => document.getElementById('readout-envelope').textContent);
    const rejWritesBefore = await venueWriteCount();

    await page.evaluate(() => window.__OCTAGON_STUB__.rejectVenueWrites('notEightChannels', -1));

    const REJECTED_TEXT = '-7.25';
    await typeInto('vf-1-x', REJECTED_TEXT);
    await blurActive();

    await page.waitForFunction(n => window.__OCTAGON_STUB__.venueWrites.length > n,
        rejWritesBefore, { timeout: SETTLE_MS }).catch(() => {});
    await page.waitForFunction(v => document.getElementById('vf-1-x').value === v,
        rejCommitted, { timeout: SETTLE_MS }).catch(() => {});

    const rej = await page.evaluate(n => {
        const w = window.__OCTAGON_STUB__.venueWrites;
        return {
            calls: w.length - n,
            sentX: w.length ? String(w[w.length - 1].speakers[0].x) : null,
            shown: document.getElementById('vf-1-x').value,
            marked: document.getElementById('vf-label-1').classList.contains('is-colliding'),
            env: document.getElementById('readout-envelope').textContent,
        };
    }, rejWritesBefore);

    // [non-vacuity] The page ASKED — a section where setVenue was never called
    // would satisfy the display clause trivially, because nothing was ever typed
    // through to a refusal.
    check(rej.calls === 1 && Number(rej.sentX) === Number(REJECTED_TEXT),
        `[non-vacuity] the page committed the typed value exactly once — ${rej.calls} call(s), x = ${rej.sentX}`);

    // And the refusal really did leave the model where it was: no publish, no
    // venueGen bump, so no geometry refresh could have done the repaint for us.
    check(rej.env === rejEnvBefore,
        `the refusal moved NOTHING downstream — envelope readout unchanged, "${rej.env}"`);

    // THE CLAIM.
    check(rej.shown === rejCommitted,
        `the field went back to the value the plugin HOLDS — showed "${REJECTED_TEXT}", `
        + `now "${rej.shown}" (committed "${rejCommitted}")`);

    // Why the repaint is the only feedback in this shape.
    check(!rej.marked,
        'and with speaker = -1 there is no is-colliding mark to fall back on — the repaint IS the feedback');

    await page.evaluate(() => {
        window.__OCTAGON_STUB__.rejectVenueWrites(null);
        window.__OCTAGON_STUB__.resetVenue();
    });

    // ── done ─────────────────────────────────────────────────────────────────
    await context.close();
    await browser.close();
    server.close();
    fs.rmSync(root, { recursive: true, force: true });

    console.log(`\n${failed === 0 ? 'ALL SECTIONS PASS' : `${failed} FAILED`} — 31 sections`);
    process.exit(failed);
})().catch(err => {
    console.error('\nui_layout_check crashed:', err);
    process.exit(99);
});
