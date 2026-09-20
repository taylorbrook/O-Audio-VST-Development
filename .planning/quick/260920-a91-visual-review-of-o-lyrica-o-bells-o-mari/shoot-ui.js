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

    shoot-ui.js — capture a plugin's WebView page as JPEGs, one per
    (language x state), at the frame the plugin actually ships.

    It is the VISUAL twin of scripts/measure-ui.js: same assembled tree
    (scripts/serve-ui.js buildRoot), same parsed frame (readEditorSize), same
    cumulative state walk over tests/i18n-states.json, same three step forms.
    measure-ui.js answers "what does the computed style say"; this answers
    "what does the page look like". A review needs both — a hex or a px value
    must come from measure-ui, a composition or hierarchy judgement from here.

    It lives in the quick-task directory rather than scripts/ on purpose: it is
    the reproducibility record for ONE review batch, committed beside the review
    files it produced. Promote it to scripts/ only if a second batch needs it.

    ── Why a duplicate-frame detector ─────────────────────────────────────────

    serve-ui/measure-ui click with `force: true`, so a control sitting under a
    popover is clicked THROUGH the popover and the step neither throws nor logs
    (pattern_forced_click_under_popover_silent_coverage_hole). The state then
    reports as walked while nothing happened. Hashing each frame per language
    and naming any byte-identical repeat is what converts that silent hole into
    a visible one — a duplicate is a COVERAGE HOLE, never a pass.

    ── Exit codes ─────────────────────────────────────────────────────────────

      0   frames were captured; read the manifest
      1   the harness could not run (no such plugin, no setSize, no LANGUAGES)
      2   usage error
     77   Playwright unresolvable — NOTHING was captured. Never a pass.

    Usage:
        node shoot-ui.js --plugin O-Marimba --list
        node shoot-ui.js --plugin O-Marimba --out shots/O-Marimba
        node shoot-ui.js --plugin O-Bells --langs fr,zh-Hans --states 0,3,13

  ==============================================================================
*/

'use strict';

const fs      = require('fs');
const os      = require('os');
const path    = require('path');
const crypto  = require('crypto');
const { pathToFileURL } = require('url');

const REPO_ROOT = path.resolve(__dirname, '..', '..', '..');
const S = require(path.join(REPO_ROOT, 'scripts', 'serve-ui.js'));

// ─────────────────────────────────────────────────────────────────── argv ──

const argv = process.argv.slice(2);
const val  = (f) => { const i = argv.indexOf(f); return i >= 0 && i + 1 < argv.length ? argv[i + 1] : null; };
const has  = (f) => argv.includes(f);

const plugin  = val('--plugin');
const listOnly = has('--list');
const outArg  = val('--out');
const langsArg = val('--langs');
const statesArg = val('--states');
const quality = parseInt(val('--quality') || '92', 10);
const settle  = parseInt(val('--settle') || '160', 10);

function die(code, msg) { console.error(`shoot-ui: ${msg}`); process.exit(code); }

if (!plugin) {
    console.error('usage: node shoot-ui.js --plugin <Name> [--list] [--out DIR] [--langs a,b] [--states 0,3,7]');
    process.exit(2);
}

// ───────────────────────────────────────────────────────────── the states ──

function readStates(name) {
    const p = path.join(S.pluginRoot(name, REPO_ROOT), 'tests', 'i18n-states.json');
    if (!fs.existsSync(p)) return [];
    try {
        const parsed = JSON.parse(fs.readFileSync(p, 'utf8'));
        return Array.isArray(parsed) ? parsed : (parsed && parsed.states) || [];
    } catch (e) {
        die(1, `tests/i18n-states.json did not parse: ${e.message}`);
    }
    return [];
}

// A state name in this repo can be a 300-character paragraph (O-Lyrica's are
// design notes, not identifiers), so the slug is truncated hard. The index
// prefix, not the slug, is what makes the filename unique.
function slug(s) {
    return String(s || 'state')
        .toLowerCase()
        .replace(/[^a-z0-9]+/g, '-')
        .replace(/^-+|-+$/g, '')
        .slice(0, 48) || 'state';
}

// The three step forms check-ui-labels documents and the 43 shipped state
// files actually use: click, dblclick, eval. Read from st.steps or the bare
// key. `select` appears in no state file in this repo; handling it would look
// correct and measure nothing.
function stepsOf(st) {
    if (Array.isArray(st.steps)) return st.steps;
    if (st.click)    return [{ click: st.click }];
    if (st.dblclick) return [{ dblclick: st.dblclick }];
    if (st.eval)     return [{ eval: st.eval }];
    return [];
}

const stateList = readStates(plugin);
// The synthetic `default` is index 0 so --states indices line up with --list.
const walk = [{ name: 'default', __synthetic: true }].concat(stateList);

if (listOnly) {
    console.log(`${plugin}: ${walk.length} state(s) (index 0 is the synthetic default)`);
    walk.forEach((st, i) => {
        const steps = stepsOf(st);
        const kind = st.__synthetic ? 'load' : (steps.length ? Object.keys(steps[0])[0] : 'NO-STEP');
        console.log(`  ${String(i).padStart(2)}  ${kind.padEnd(8)}  ${String(st.name || '').slice(0, 110)}`);
    });
    process.exit(0);
}

// ──────────────────────────────────────────────────────────────── capture ──

(async function main() {
    const pw = S.resolvePlaywright();
    if (!pw) {
        console.error('shoot-ui: Playwright unresolvable — NOTHING was captured.');
        console.error('          This is exit 77 and it is NOT a pass. Do not npm install:');
        console.error('          resolvePlaywright() reads an existing ~/.npm/_npx cache by design.');
        process.exit(77);
    }
    const { chromium } = pw;

    let built;
    try { built = S.buildRoot(plugin, { repoRoot: REPO_ROOT }); }
    catch (e) { die(1, e.message); }

    // THE SHIPPING FRAME — parsed from PluginEditor.cpp, never chosen.
    const size = S.readEditorSize(plugin, REPO_ROOT);
    if (!size) die(1, `no setSize(W,H) in plugins/${plugin}/Source/PluginEditor.cpp — `
                    + 'the frame is parsed, never chosen, so there is nothing to capture at');

    // LANGUAGES off the RESOLVED ui root. Two plugins carry both a
    // Source/ui/public and a Resources/ui and only one is embedded; a guessed
    // path renders a stale page that looks perfectly fine.
    const langsFile = path.join(built.uiRoot, 'js', 'i18n.js');
    if (!fs.existsSync(langsFile)) die(1, `no js/i18n.js under ${built.uiRootLabel} for ${plugin}`);

    const tmp = fs.mkdtempSync(path.join(os.tmpdir(), 'shoot-ui-'));
    const mjs = path.join(tmp, 'i18n.mjs');
    fs.copyFileSync(langsFile, mjs);

    let LANGUAGES = null;
    try { ({ LANGUAGES } = await import(pathToFileURL(mjs).href)); }
    catch (e) { die(1, `js/i18n.js did not import as an ES module: ${e.message}`); }
    if (!Array.isArray(LANGUAGES) || LANGUAGES.length === 0)
        die(1, 'js/i18n.js exports no usable LANGUAGES array — refusing to fall back to a guessed '
             + 'pair, because frames of a language set the plugin does not ship are frames of a '
             + 'page that does not exist');

    let langs = LANGUAGES;
    if (langsArg) {
        const want = langsArg.split(',').map(s => s.trim()).filter(Boolean);
        const bad = want.filter(l => !LANGUAGES.includes(l));
        if (bad.length) die(2, `--langs names ${bad.join(', ')}, which ${plugin} does not ship `
                              + `(it ships ${LANGUAGES.join(', ')})`);
        langs = want;
    }

    let wantIdx = null;
    if (statesArg) {
        wantIdx = new Set(statesArg.split(',').map(s => parseInt(s.trim(), 10)).filter(Number.isFinite));
        for (const i of wantIdx)
            if (i < 0 || i >= walk.length) die(2, `--states index ${i} is out of range (0..${walk.length - 1})`);
    }

    const out = path.resolve(outArg || path.join(__dirname, 'shots', plugin));
    fs.mkdirSync(out, { recursive: true });

    const srv = await S.serve(built.root);
    const browser = await chromium.launch();
    const page = await browser.newPage({ viewport: { width: size.w, height: size.h } });

    // `viewport`, NOT `viewportSize` — the plural is the getter's name and a
    // silent no-op as a launch option, leaving Chromium's 1280x720. One wrong
    // identifier and every frame is of a page nobody ships.

    const pageErrors = [];
    page.on('pageerror', (e) => pageErrors.push(String(e && e.message || e).split('\n')[0]));

    const shots = [];
    const shaSeen = new Map();   // per-language: sha -> first file carrying it

    for (const lang of langs) {
        // ONE load per language; the states below apply CUMULATIVELY with no
        // reset, because a state can require an earlier one (O-Lyrica's
        // generator forms only exist once the generator is expanded).
        await page.goto(`http://127.0.0.1:${srv.port}/`, { waitUntil: 'networkidle' });
        await page.evaluate((l) => window.__setLanguage && window.__setLanguage(l), lang);
        await page.waitForTimeout(220);

        for (let i = 0; i < walk.length; ++i) {
            const st = walk[i];
            const steps = stepsOf(st);
            let stepSkipped = null;

            for (const step of steps) {
                try {
                    if (step.click)         await page.click(step.click, { timeout: 1500, force: true });
                    else if (step.dblclick) await page.dblclick(step.dblclick, { timeout: 1500, force: true });
                    else if (step.eval)     await page.evaluate((src) => { (0, eval)(src); }, step.eval);
                } catch (e) {
                    // A step that will not run is a state this page does not
                    // have. The walk continues — but it is RECORDED, because a
                    // silently skipped state is a coverage hole, not a pass.
                    stepSkipped = `${Object.keys(step)[0]}: ${String(step.click || step.dblclick || step.eval).slice(0, 80)} — ${String(e.message).split('\n')[0]}`;
                    console.error(`shoot-ui: [${lang}] state ${i} step skipped — ${stepSkipped}`);
                }
            }
            await page.waitForTimeout(settle);

            // States the walk must still APPLY (cumulative) but not capture.
            if (wantIdx && !wantIdx.has(i)) continue;

            const file = `${lang}__${String(i).padStart(2, '0')}-${slug(st.name)}.jpg`;
            const buf = await page.screenshot({ type: 'jpeg', quality });
            fs.writeFileSync(path.join(out, file), buf);

            const sha = crypto.createHash('sha256').update(buf).digest('hex');
            const key = lang + ' ' + sha;
            const dup = shaSeen.has(key) ? shaSeen.get(key) : null;
            if (!dup) shaSeen.set(key, file);

            shots.push({
                file,
                lang,
                index: i,
                stateName: String(st.name || ''),
                sha: sha.slice(0, 16),
                duplicateOf: dup,
                stepSkipped,
                bytes: buf.length,
            });
        }
    }

    await browser.close();
    await srv.close();
    fs.rmSync(tmp, { recursive: true, force: true });
    fs.rmSync(built.root, { recursive: true, force: true });

    const manifest = {
        plugin,
        uiRootLabel: built.uiRootLabel,
        uiRootFrom: built.uiRootFrom,
        stubKind: built.stubKind,
        seedFrom: built.seedFrom,
        frame: { w: size.w, h: size.h },
        languages: langs,
        allLanguages: LANGUAGES,
        stateCount: walk.length,
        capturedAt: new Date().toISOString(),
        quality,
        shots,
        pageErrors,
        unplaced: built.unplaced,
        missing: built.missing,
    };

    // A --states/--langs subset run must EXTEND the manifest, not clobber the
    // rows an earlier full run wrote — otherwise the review's own screenshot
    // table loses the frames it cites.
    const manifestPath = path.join(out, 'manifest.json');
    if (fs.existsSync(manifestPath)) {
        try {
            const prev = JSON.parse(fs.readFileSync(manifestPath, 'utf8'));
            const fresh = new Set(shots.map(s => s.file));
            const kept = (prev.shots || []).filter(s => !fresh.has(s.file));
            manifest.shots = kept.concat(shots).sort((a, b) =>
                a.lang === b.lang ? a.index - b.index : a.lang.localeCompare(b.lang));
            manifest.languages = [...new Set((prev.languages || []).concat(langs))];
            manifest.pageErrors = [...new Set((prev.pageErrors || []).concat(pageErrors))];
        } catch { /* an unreadable previous manifest is simply replaced */ }
    }

    fs.writeFileSync(manifestPath, JSON.stringify(manifest, null, 2) + '\n');

    const dups = shots.filter(s => s.duplicateOf).length;
    const skips = shots.filter(s => s.stepSkipped).length;
    console.error(`shoot-ui: ${plugin} ${size.w}x${size.h} ui=${built.uiRootLabel} seed=${built.seedFrom}`);
    console.error(`shoot-ui: coverage ${shots.length - dups} distinct / ${shots.length} shots`
                + `  duplicates=${dups}  skipped-steps=${skips}  page-errors=${pageErrors.length}`);
    if (dups) for (const s of shots.filter(x => x.duplicateOf))
        console.error(`shoot-ui:   DUPLICATE ${s.file} == ${s.duplicateOf}`);
    console.error(`shoot-ui: manifest ${path.relative(process.cwd(), manifestPath)}`);
})().catch((e) => { console.error('shoot-ui: ' + (e && e.stack || e)); process.exit(1); });
