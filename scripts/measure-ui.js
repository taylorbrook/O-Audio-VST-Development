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

    ── THE FOUR SCREENS ────────────────────────────────────────

    Promoting the MEASUREMENT alone does not close wave 4b's complaint: that
    wave rebuilt the ANALYSIS by hand too, and the next wave would rebuild it
    wrong the same way. So the four screens it ran by hand are here, as
    --report filters over the same rows.

    Every screen is a PURE FUNCTION over the row array. Nothing in a screen
    touches Playwright, the filesystem or the network. That is what makes
    --from work:

        node scripts/measure-ui.js --report undeclared-font --from /tmp/rows.json

    re-runs a screen against a saved measurement with no browser at all, so a
    screen can be re-run, diffed and reviewed long after the run that produced
    the rows. With --from, --plugin is not required.

    EVERY SCREEN PRINTS ITS COUNT, INCLUDING WHEN IT IS 0. A screen that prints
    nothing on a clean plugin is indistinguishable from a screen that did not
    run, and three of these four read 0 on every plugin wave 4b already
    repaired. The count line is fixed in format, one per screen, on stderr:

        <screen-name>: <count> finding(s)

    A screen whose fields are absent from the rows prints

        <screen-name>: SKIPPED — needs --mode box (field <name> not present)

    and NO finding count, because a screen that could not run must not be
    mistaken for a screen that ran and found nothing. That distinction is the
    whole point of printing a count at 0.

    undeclared-font   Visible nodes carrying Han in any carrier whose computed
                      font-family names NO CJK face. This is FORM 4 of the
                      font-carrier census (wave 4b, C2): <button>, <select> and
                      <input> do NOT inherit font-family — the UA stylesheet
                      gives them one, on this build Arial — so a sweep that
                      reads every CSS declaration AND every SVG attribute in
                      both files still misses them. No grep can see this class.
                      The face list is a named constant and is PRINTED on every
                      run, so a clean result is distinguishable from a result
                      produced by an empty list. --cjk-faces extends it.

    line-height-normal  Visible Han-carrying LEAF nodes whose computed
                      line-height resolves to `normal`. Restricted to leaves for
                      the reason check-ui-labels restricts its clip check to
                      them: a container's box does not report a text metric.

    wrap-count        Per node, lines = round((h - pt - pb - bt - bb) /
                      lineHeightPx), reported where the count DIFFERS between
                      English and any non-English language. lineHeightPx is the
                      computed value, or fsn * 1.2 when it resolves to `normal`
                      — an APPROXIMATION of the UA's normal line box, not a
                      measurement. So a node the line-height-normal screen also
                      catches is a node whose wrap count is ESTIMATED rather
                      than measured, and the two screens should be read
                      together. Needs --mode box.

    svg-font-attr     Two counts, both printed:
                        (a) every node carrying a font-family PRESENTATION
                            ATTRIBUTE — the raw count, which is this screen's
                            own liveness signal;
                        (b) the subset whose attribute DIVERGES from the
                            computed stack.
                      (b) is the useful half, and the reason this is a
                      divergence report rather than a presence report: a
                      presentation attribute has the LOWEST specificity of any
                      font-family source, so a CSS rule overrides it silently.
                      On O-AnalogEQ the 24 attributes read
                      `Garamond, 'Times New Roman', serif` while the computed
                      stack carries the CJK tail. Someone reading only the
                      markup concludes the tail is missing; someone reading only
                      the computed style never learns the markup disagrees.
                      Those 24 are numeric tick labels and carry no Han, so a
                      divergence there is a finding to READ, not a defect.

    A screen with findings still exits 0. --report with an unknown name exits 2.

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

    The reasoning behind every note above, the measured evidence for each, the
    positive controls that make a screen's 0 readable, the known limitations and
    the record of what was deliberately NOT promoted alongside this file:

        scripts/measure-ui-README.md

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

const SCREEN_ORDER = ['undeclared-font', 'line-height-normal', 'wrap-count', 'svg-font-attr'];

const USAGE = [
    'usage: node scripts/measure-ui.js --plugin <Name> [--mode fonts|box] [--select <css>]',
    '                                  [--report <screen>] [--root DIR] [--verbose]',
    '       node scripts/measure-ui.js --report <screen> --from <rows.json>',
    '',
    '  --plugin <Name>   the plugin to measure (required unless --from is given)',
    '  --mode fonts|box  fonts: computed font stack per node (default)',
    '                    box:   fonts PLUS the full geometry a wrap count needs',
    '  --select <css>    restrict the sweep to nodes matching this selector',
    '  --report <screen> run a defect screen over the rows; counts go to stderr',
    '                    ' + SCREEN_ORDER.join(' | ') + ' | all',
    '  --from <file>     read rows from a saved JSON array instead of measuring',
    '  --cjk-faces <a,b> extend the CJK face list undeclared-font checks against',
    '  --root DIR        repo-root override (fixture trees)',
    '  --verbose         diagnostics to stderr, including skipped states',
    '',
    'exit: 0 run completed  1 harness could not run  2 usage  77 no Playwright',
    '      a screen finding defects is 0 \u2014 this is a report, not a gate',
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

// ── the defect screens ─────────────────────────────────────────────────────
//
// Every screen is a PURE FUNCTION (rows, opts) -> { name, needs, count,
// findings, notes }. No I/O, no browser, no filesystem — which is exactly what
// lets --from re-run one against a saved measurement.

// The CJK faces actually shipped by this repo today, counted over
// plugins/*/Source/ui/public: PingFang SC 147, Microsoft YaHei 137,
// Songti SC 3. It is a CONSTANT, and that is a disclosed limitation: a face a
// future plugin ships and nobody adds here reads as an undeclared font. The
// list is PRINTED on every run so a clean result cannot be confused with a
// result produced by an empty list. --cjk-faces extends it.
const CJK_FACES = ['PingFang SC', 'Microsoft YaHei', 'Songti SC'];

// The UA's `normal` line box is not exposed anywhere. 1.2 is the conventional
// approximation, and it is an APPROXIMATION — see the wrap-count note above.
const NORMAL_LINE_BOX = 1.2;

function esc(x) { return x.replace(/[.*+?^${}()|[\]\\]/g, '\\$&'); }

function normStack(v) {
    return String(v || '').replace(/["']/g, '').split(',')
        .map((t) => t.trim().toLowerCase()).filter(Boolean).join(', ');
}

function lineBoxPx(r) {
    if (!r.lh || r.lh === 'normal') return (r.fsn || 0) * NORMAL_LINE_BOX;
    const v = parseFloat(r.lh);
    return isFinite(v) ? v : 0;
}

function lineCount(r) {
    const lb = lineBoxPx(r);
    if (!(lb > 0)) return null;
    const content = r.h - r.pt - r.pb - r.bt - r.bb;
    if (!(content > 0)) return null;
    return Math.round(content / lb);
}

const SCREENS = {
    'undeclared-font': {
        needs: ['ff'],
        run(rows, opts) {
            const faces = opts.cjkFaces;
            const re = new RegExp(faces.map(esc).join('|'), 'i');
            const findings = rows
                .filter((r) => r.vis && r.han && !re.test(r.ff || ''))
                .map((r) => `${r.lang}  ${r.key}  (${r.id})  "${r.own}"  ff=${r.ff}`);
            return { count: findings.length, findings,
                     notes: [`faces checked against: ${faces.join(', ')}`] };
        },
    },

    'line-height-normal': {
        needs: ['lh', 'kids'],
        run(rows) {
            const findings = rows
                .filter((r) => r.vis && r.han && r.kids === 0 && r.lh === 'normal')
                .map((r) => `${r.lang}  ${r.key}  (${r.id})  "${r.own}"  font-size=${r.fs}`);
            return { count: findings.length, findings,
                     notes: ['leaf nodes only — a container box does not report a text metric'] };
        },
    },

    'wrap-count': {
        needs: ['h', 'pt', 'pb', 'bt', 'bb', 'lh', 'fsn'],
        run(rows) {
            const byKey = new Map();
            for (const r of rows) {
                if (!(r.vis && r.kids === 0 && r.own)) continue;
                if (!byKey.has(r.key)) byKey.set(r.key, new Map());
                byKey.get(r.key).set(r.lang, r);
            }
            const findings = [];
            let estimated = 0;
            for (const [key, langs] of byKey) {
                const en = langs.get('en');
                if (!en) continue;
                const nEn = lineCount(en);
                if (nEn === null) continue;
                if (!en.lh || en.lh === 'normal') ++estimated;
                for (const [lang, r] of langs) {
                    if (lang === 'en') continue;
                    const n = lineCount(r);
                    if (n === null || n === nEn) continue;
                    findings.push(`${key}  (${en.id})  "${en.own}"  en=${nEn} line(s), ${lang}=${n} line(s)`);
                }
            }
            return { count: findings.length, findings,
                     notes: [`${byKey.size} text-bearing leaf node(s) compared; `
                           + `${estimated} of them ESTIMATED at fsn * ${NORMAL_LINE_BOX} `
                           + '(line-height: normal) rather than measured'] };
        },
    },

    'svg-font-attr': {
        needs: ['ffAttr', 'ff'],
        run(rows) {
            // Counted per DOM node, not per row: a presentation attribute is a
            // property of the markup and does not vary by language, so counting
            // rows would simply multiply by the language count.
            const carriers = new Map();
            const diverging = new Map();
            for (const r of rows) {
                if (!r.ffAttr) continue;
                if (!carriers.has(r.key)) carriers.set(r.key, r);
                if (normStack(r.ffAttr) !== normStack(r.ff) && !diverging.has(r.key)) diverging.set(r.key, r);
            }
            const findings = Array.from(diverging.values())
                .map((r) => `${r.key}  (${r.id})  attr=[${r.ffAttr}]  computed=[${r.ff}]`);
            return { count: findings.length, findings, carriers: carriers.size,
                     notes: [`${carriers.size} node(s) carry a font-family presentation attribute`] };
        },
    },
};

// Fixed line format, because a paraphrase would make the verify blocks that
// match on it lie. See the header.
function runScreen(name, rows, opts) {
    const sc = SCREENS[name];
    const probe = rows.length ? rows[0] : null;
    const missing = probe ? sc.needs.find((f) => !(f in probe)) : sc.needs[0];

    if (missing) {
        console.error(`${name}: SKIPPED — needs --mode box (field ${missing} not present)`);
        return;
    }

    const res = sc.run(rows, opts);

    if (name === 'svg-font-attr')
        console.error(`${name}: ${res.carriers} attribute carrier(s), ${res.count} finding(s)`);
    else
        console.error(`${name}: ${res.count} finding(s)`);

    for (const n of res.notes || []) console.error(`  ${n}`);
    for (const f of res.findings) console.error(`    ${f}`);
}

// ── CLI ────────────────────────────────────────────────────────────────────

if (require.main === module) {
    const report   = val('--report');
    const from     = val('--from');
    const cjkExtra = val('--cjk-faces');
    const cjkFaces = CJK_FACES.concat((cjkExtra || '').split(',').map((x) => x.trim()).filter(Boolean));

    if (report && report !== 'all' && !SCREENS[report]) {
        console.log(USAGE);
        console.log(`\n--report: no such screen '${report}'. Valid: ${SCREEN_ORDER.join(', ')}, all`);
        process.exit(2);
    }
    if (!plugin && !from) { console.log(USAGE); process.exit(2); }
    if (mode !== 'fonts' && mode !== 'box') {
        console.log(USAGE);
        console.log(`\n--mode must be 'fonts' or 'box' (got '${mode}')`);
        process.exit(2);
    }

    const emit = (rows) => {
        process.stdout.write(JSON.stringify(rows));
        process.stdout.write('\n');
        if (report) for (const name of (report === 'all' ? SCREEN_ORDER : [report]))
            runScreen(name, rows, { cjkFaces });
    };

    if (from) {
        let rows;
        try { rows = JSON.parse(fs.readFileSync(from, 'utf8')); }
        catch (e) { die(1, `--from ${from}: ${e.message}`); }
        if (!Array.isArray(rows)) die(1, `--from ${from}: not a JSON array of rows`);
        if (verbose) console.error(`measure-ui: ${rows.length} row(s) read from ${from} — no browser launched`);
        emit(rows);
    } else {
        measure().then(emit).catch((e) => {
            console.error('measure-ui: ' + (e && e.stack ? e.stack : e));
            process.exit(1);
        });
    }
}

module.exports = { HAN_SRC, pageProbe, SCREENS, SCREEN_ORDER, CJK_FACES, NORMAL_LINE_BOX };
