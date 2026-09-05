/*
   This file is part of the Ouaricon Audio plugin suite.
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

    measure-ui.js — a per-node COMPUTED-STYLE CENSUS of any plugin's WebView
    page, taken in every language the plugin declares, in every state its
    tests/i18n-states.json can reach, at the frame the plugin actually ships.

    Promoted from the throwaway harness written for wave 4b (quick task
    260905-acr), which found four defect classes no grep in this repo can see.
    That harness lived in a session scratchpad; every wave that needs it has to
    rebuild it, and wave 4b proved a rebuild can be WRONG in ways that hide real
    defects rather than announce themselves. The four notes below are the whole
    reason this file is committed instead of retyped.

    ── 1. THE SHIPPING FRAME IS PARSED, NEVER CHOSEN ──────────────────────────

    The viewport comes from readEditorSize(), which reads setSize(W, H) out of
    that plugin's own Source/PluginEditor.cpp — the same source check-ui-labels
    parses. A wider frame measures a page nobody ships: a caption that wraps to
    three lines at the shipped frame fits on one at 1200px, and the whole class
    of wrap-count defects simply is not there to be found. Wave 4b measured
    O-Emulator at 1200px wide and its engraved plate read clean; at the parsed
    frame it did not.

    What makes this fragile is a silent fallback, so it is worth naming the
    mechanism: Playwright accepts `viewport`, and SILENTLY IGNORES the
    misspelled plural `viewportSize` (that is the getter's name, not an option),
    leaving Chromium's 1280x720 default. One wrong identifier turns every
    measurement in this file into a measurement of a page that does not exist,
    and nothing anywhere reports it.

    ── 2. THE STATE FILE IS A CUMULATIVE WALK, NOT A SET OF ONE-SHOT CLICKS ───

    The page loads ONCE per language and the states apply in file order with no
    reset between them, because a state can REQUIRE an earlier one:
    O-IntonationPad's rotation view is reachable only through the tuning tab.
    Resetting between states measures pages those states never produce, and the
    panels behind the second click are never measured at all.

    ── 3. THE VISIBILITY AND HAN FLAGS ARE OR-ED ACROSS STATES (W2) ───────────

    Keying a node on first sighting while walking cumulatively makes every node
    read as invisible for any panel the walk later navigates away from. On
    O-Prism that was 19 visible Han-bearing nodes found where 48 existed. So
    `vis` and `han` are OR-ed over the whole walk, and the rest of the fields
    are taken from the last state in which the node was actually visible.

    ── 4. NODE IDENTITY IS A DOM PATH, NOT A TAG-AND-CLASS STRING ─────────────

    New to this promotion, and the largest defect found in the scratch original.
    It keyed its accumulator on `lang + ' ' + (#id | tag.class.class)`, so every
    node sharing a shape collapsed into ONE row and the last visible one won.
    Measured: O-AnalogEQ's 29 sibling SVG <text> nodes reported as 1 row, and
    O-Emulator's entire page as 47. Every per-node count wave 4b produced was
    therefore a LOWER BOUND, and any screen run over such a key is structurally
    blind to siblings.

    Here the accumulator is keyed on a structural nth-of-type chain walked up to
    documentElement, emitted as `key`. The friendly string survives as `id`, for
    reading only. Every run prints the ratio between the two counts to stderr,
    so a reader comparing this run against a wave-4b number can see why it moved.

    ── Exit codes ─────────────────────────────────────────────────────────────

        0    the run completed — read stdout
        1    the harness itself could not run: no such plugin, no setSize in
             PluginEditor.cpp, no js/i18n.js, no LANGUAGES export
        2    usage error
        77   Playwright unresolvable — NOTHING was measured, never a pass

    THIS IS A REPORT, NOT A GATE, and the name says so: `check-` in this repo
    means a gate (check-ui-labels exits n>0 on n failures); `measure-` does not.
    A screen finding defects still exits 0. The reasoning is boot-all-uis.js's:
    a permanently-red gate on findings a wave has not budgeted to fix destroys
    the habit of reading gates, and these findings are read by a person deciding
    what a localization wave should repair, not by CI.

    ── Output ─────────────────────────────────────────────────────────────────

    ONE JSON array on stdout and nothing else, so this composes with jq and node
    one-liners. Every human-readable line — diagnostics, the identity
    disclosure, per-screen counts — goes to stderr.

    Usage:
        node scripts/measure-ui.js --plugin O-Emulator
        node scripts/measure-ui.js --plugin O-Emulator --mode box
        node scripts/measure-ui.js --plugin O-AnalogEQ --select text
        node scripts/measure-ui.js --plugin O-Fixture --root /tmp/fix --verbose

  ==============================================================================
*/

'use strict';

const fs   = require('fs');
const os   = require('os');
const path = require('path');
const { pathToFileURL } = require('url');
const S    = require(path.join(__dirname, 'serve-ui.js'));

// The repo root is DERIVED, never spelled. serve-ui.js already takes it from
// __dirname; a literal path here would name one machine's home directory and
// the file would work nowhere else.
const REPO_ROOT = S.REPO_ROOT;

const argv     = process.argv.slice(2);
const val      = (f) => { const i = argv.indexOf(f); return i >= 0 && i + 1 < argv.length ? argv[i + 1] : null; };
const plugin   = val('--plugin');
const mode     = val('--mode') || 'fonts';
const select   = val('--select') || '';
const repoRoot = val('--root') || REPO_ROOT;
const verbose  = argv.includes('--verbose');

const USAGE = [
    'usage: node scripts/measure-ui.js --plugin <Name> [--mode fonts|box] [--select <css>]',
    '                                  [--root DIR] [--verbose]',
    '',
    '  --plugin <Name>   the plugin to measure (required)',
    '  --mode fonts|box  fonts: computed font stack per node (default)',
    '                    box:   fonts PLUS the full geometry a wrap count needs',
    '  --select <css>    restrict the sweep to nodes matching this selector',
    '  --root DIR        repo-root override (fixture trees)',
    '  --verbose         diagnostics to stderr, including skipped states',
    '',
    'exit: 0 run completed  1 harness could not run  2 usage  77 no Playwright',
].join('\n');

// Han + CJK Ext-A, written as escaped code points rather than literal glyphs so
// this file stays ASCII and survives any shell that mangles multibyte input.
const HAN_SRC = '[\\u3400-\\u4DBF\\u4E00-\\u9FFF]';

function die(code, msg) { console.error('measure-ui: ' + msg); process.exit(code); }

// ── the page-side probe ────────────────────────────────────────────────────
// Everything measured in ONE evaluate per state, so nothing can shift between
// two round trips.
function pageProbe({ mode, sel, hanSrc }) {
    const han = new RegExp(hanSrc);

    // A structural key: an nth-of-type chain walked to documentElement. This is
    // what makes siblings distinct rows. See design note 4.
    const domKey = (n) => {
        const parts = [];
        let cur = n;
        while (cur && cur.nodeType === 1) {
            const tag = cur.tagName.toLowerCase();
            const parent = cur.parentNode;
            if (!parent || parent.nodeType !== 1) { parts.unshift(tag); break; }
            let i = 1;
            for (const sib of parent.children) {
                if (sib === cur) break;
                if (sib.tagName === cur.tagName) ++i;
            }
            parts.unshift(tag + '[' + i + ']');
            cur = parent;
        }
        return parts.join('/');
    };

    const out = [];
    let nodes;
    try { nodes = sel ? document.querySelectorAll(sel) : document.querySelectorAll('*'); }
    catch (e) { return { error: 'selector did not parse: ' + e.message }; }

    for (const n of nodes) {
        const cs = getComputedStyle(n);
        const bb = n.getBoundingClientRect();
        const vis = bb.width > 0 && bb.height > 0 && cs.visibility !== 'hidden' && cs.display !== 'none';
        const own = Array.from(n.childNodes).filter((c) => c.nodeType === 3)
            .map((c) => c.nodeValue).join('').trim();

        // EVERY surface that HOLDS OR CAN RECEIVE a Han codepoint, not just own
        // text: data-tip and data-tip-title are written by applyI18n and painted
        // through this node's inherited stack.
        const carriers = [own,
            n.getAttribute && n.getAttribute('data-tip'),
            n.getAttribute && n.getAttribute('data-tip-title'),
            n.getAttribute && n.getAttribute('aria-label')].filter(Boolean).join(' ');

        const id = n.id ? '#' + n.id
            : n.tagName.toLowerCase() + (n.className && typeof n.className === 'string'
                ? '.' + n.className.trim().split(/\s+/).join('.') : '');

        const kids = Array.from(n.children).length;

        // The node's OWN font-family PRESENTATION ATTRIBUTE, which has the
        // lowest specificity of any font-family source and is therefore
        // overridden silently by any CSS rule. One field, in the same pass, is
        // what makes the svg-font-attr screen possible without a second sweep.
        const ffAttr = (n.getAttribute && n.getAttribute('font-family')) || null;

        const base = { key: domKey(n), id, vis, han: han.test(carriers), own: own.slice(0, 40), kids, ffAttr };

        if (mode === 'fonts') out.push(Object.assign({}, base, { ff: cs.fontFamily, ls: cs.letterSpacing }));
        else out.push(Object.assign({}, base, {
            x: +bb.x.toFixed(2), y: +bb.y.toFixed(2),
            w: +bb.width.toFixed(2), h: +bb.height.toFixed(2),
            ls: cs.letterSpacing, lh: cs.lineHeight, fs: cs.fontSize,
            pad: cs.padding, ff: cs.fontFamily,
            pt: parseFloat(cs.paddingTop) || 0, pb: parseFloat(cs.paddingBottom) || 0,
            bt: parseFloat(cs.borderTopWidth) || 0, bb: parseFloat(cs.borderBottomWidth) || 0,
            fsn: parseFloat(cs.fontSize) || 0,
        }));
    }
    return { rows: out };
}

// ── the measurement run ────────────────────────────────────────────────────

async function measure() {
    const pw = S.resolvePlaywright();
    if (!pw) {
        console.error('measure-ui: Playwright unresolvable — NOTHING was measured.');
        console.error('            This is exit 77 and it is NOT a pass.');
        process.exit(77);
    }
    const { chromium } = pw;

    let built;
    try { built = S.buildRoot(plugin, { repoRoot }); }
    catch (e) { die(1, e.message); }

    // THE SHIPPING FRAME — parsed, never chosen. See design note 1.
    const size = S.readEditorSize(plugin, repoRoot);
    if (!size) die(1, `no setSize(W,H) in plugins/${plugin}/Source/PluginEditor.cpp — `
                    + 'the frame is parsed, never chosen, so there is nothing to measure at');

    // LANGUAGES is read off the RESOLVED ui root, not a guessed
    // Source/ui/public: two plugins carry both that and a Resources/ui, and the
    // one that is not embedded renders a stale page (serve-ui.js header).
    const langsFile = path.join(built.uiRoot, 'js', 'i18n.js');
    if (!fs.existsSync(langsFile)) die(1, `no js/i18n.js under ${built.uiRootLabel} for ${plugin}`);

    const tmp = fs.mkdtempSync(path.join(os.tmpdir(), 'measure-ui-'));
    const mjs = path.join(tmp, 'i18n.mjs');
    fs.copyFileSync(langsFile, mjs);

    let LANGUAGES = null;
    try { ({ LANGUAGES } = await import(pathToFileURL(mjs).href)); }
    catch (e) { die(1, `js/i18n.js did not import as an ES module: ${e.message}`); }
    if (!Array.isArray(LANGUAGES) || LANGUAGES.length === 0)
        die(1, 'js/i18n.js exports no usable LANGUAGES array — refusing to fall back to a '
             + 'default pair, because a run over a language set the plugin does not ship '
             + 'reports numbers about a page that does not exist');

    const statesPath = path.join(S.pluginRoot(plugin, repoRoot), 'tests', 'i18n-states.json');
    let stateList = [];
    if (fs.existsSync(statesPath)) {
        try {
            const parsed = JSON.parse(fs.readFileSync(statesPath, 'utf8'));
            stateList = Array.isArray(parsed) ? parsed : (parsed && parsed.states) || [];
        } catch (e) { console.error(`measure-ui: tests/i18n-states.json did not parse: ${e.message}`); }
    }

    if (verbose) {
        console.error(`measure-ui: ${plugin}  ui=${built.uiRootLabel}  frame=${size.w}x${size.h}`);
        console.error(`measure-ui: languages ${LANGUAGES.join(', ')}  states default + ${stateList.length}`);
        if (select) console.error(`measure-ui: selector ${select}`);
    }

    const srv = await S.serve(built.root);
    const browser = await chromium.launch();

    // `viewport`, NOT `viewportSize`. See design note 1 — the misspelling is a
    // silent no-op and leaves 1280x720.
    const page = await browser.newPage({ viewport: { width: size.w, height: size.h } });

    const acc = new Map();
    let probeError = null;

    for (const lang of LANGUAGES) {
        // ONE load per language. The states below then apply CUMULATIVELY on
        // this same page — see design note 2.
        await page.goto(`http://127.0.0.1:${srv.port}/`, { waitUntil: 'networkidle' });
        await page.evaluate((l) => window.__setLanguage && window.__setLanguage(l), lang);
        await page.waitForTimeout(220);

        for (const st of [{ name: 'default' }].concat(stateList)) {
            // THREE STEP FORMS, the three check-ui-labels.js documents and the
            // three the 43 shipped state files use: click, dblclick, eval. The
            // scratch original handled click / select / eval — `select` appears
            // in no state file in the repo, and `dblclick` appears in exactly
            // one (O-Octagon), so the wrong three looked correct.
            const steps = st.steps
                || (st.click ? [{ click: st.click }]
                  : st.dblclick ? [{ dblclick: st.dblclick }]
                  : st.eval ? [{ eval: st.eval }] : []);

            for (const step of steps) {
                try {
                    if (step.click) await page.click(step.click, { timeout: 1500, force: true });
                    else if (step.dblclick) await page.dblclick(step.dblclick, { timeout: 1500, force: true });
                    else if (step.eval) await page.evaluate((src) => { (0, eval)(src); }, step.eval);
                } catch (e) {
                    // A step that will not run is a state this page does not
                    // have, and the walk must continue — but say so under
                    // --verbose, because a silently skipped state is exactly how
                    // a dblclick entry passed for a measured state before.
                    if (verbose)
                        console.error(`measure-ui: [${lang}] state "${st.name}" step skipped `
                                    + `(${Object.keys(step)[0]}: ${String(step.click || step.dblclick || step.eval).slice(0, 60)}) `
                                    + `— ${String(e.message).split('\n')[0]}`);
                }
            }
            await page.waitForTimeout(140);

            const res = await page.evaluate(pageProbe, { mode, sel: select, hanSrc: HAN_SRC });
            if (res.error) { probeError = res.error; break; }

            for (const row of res.rows) {
                const k = lang + ' ' + row.key;                       // design note 4
                if (!acc.has(k)) acc.set(k, Object.assign({}, row, { lang, vis: false, han: false }));
                const cur = acc.get(k);
                cur.vis = cur.vis || row.vis;                          // W2, design note 3
                cur.han = cur.han || row.han;                          // W2, design note 3
                if (row.vis) {
                    const copy = Object.assign({}, row);
                    delete copy.vis; delete copy.han;
                    Object.assign(cur, copy);
                    cur.vis = true;
                }
            }
        }
        if (probeError) break;
    }

    await browser.close();
    await srv.close();
    fs.rmSync(tmp, { recursive: true, force: true });
    fs.rmSync(built.root, { recursive: true, force: true });

    if (probeError) die(2, probeError);

    const rows = Array.from(acc.values());

    // THE IDENTITY DISCLOSURE. A reader comparing this run against a wave-4b
    // count needs the ratio to understand why the numbers moved: everything
    // measured before this file existed was keyed on the display id.
    const keys = new Set(rows.map((r) => r.key));
    const ids  = new Set(rows.map((r) => r.id));
    console.error(`identity: ${rows.length} nodes / ${keys.size} distinct DOM keys / `
                + `${ids.size} distinct display ids — keyed by DOM path`);

    return rows;
}

// ── CLI ────────────────────────────────────────────────────────────────────

if (require.main === module) {
    if (!plugin) { console.log(USAGE); process.exit(2); }
    if (mode !== 'fonts' && mode !== 'box') {
        console.log(USAGE);
        console.log(`\n--mode must be 'fonts' or 'box' (got '${mode}')`);
        process.exit(2);
    }

    measure().then((rows) => {
        process.stdout.write(JSON.stringify(rows));
        process.stdout.write('\n');
    }).catch((e) => {
        console.error('measure-ui: ' + (e && e.stack ? e.stack : e));
        process.exit(1);
    });
}

module.exports = { HAN_SRC, pageProbe };
