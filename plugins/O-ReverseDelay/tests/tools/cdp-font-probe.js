/*
   This file is part of O-ReverseDelay, an Ouaricon Audio plugin.
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
/* CDP resolved-face probe — O-ReverseDelay (v1.22.0, R5 EB Garamond pilot).

   Reports the RESOLVED platform face per text run via
   CSS.getPlatformFontsForNode, after document.fonts.ready. measure-ui.js's `ff`
   field is the DECLARED stack and cannot say which face actually painted
   (memory pattern_declared_font_stack_is_not_the_resolved_face). Boot copied
   from plugins/O-Strata/tests/tools/cdp-font-probe.js (serve-ui
   buildRoot/serve, readEditorSize frame, window.__setLanguage).

   PASS, per language:
     (a) every target node is found;
     (b) every target whose text holds a Latin letter, a digit or U+2766 (the
         fleuron) resolves to the bundled custom face "EB Garamond" with
         glyphCount > 0; any extra non-custom face on that node is allowed only
         when it is CJK, or when the text holds U+25BE / U+2699 (absent from the
         subset by design, they fall back per glyph as they always have);
     (c) under zh-Hans, every target whose text holds Han reports a
         PingFangSC-prefixed face (the family name is localised, the
         PostScript name is not);
     (d) document.fonts holds exactly three "EB Garamond" faces — 400 normal,
         400 italic, 700 normal — all `loaded`;
     (e) the served tree recorded zero unserved requests.

   options:
     --all-languages       every language in js/i18n.js (default: en only)
     --dump-rects FILE     write { lang: { key: [x,y,w,h] } } over `body *`
     --diff-rects FILE     compare against an earlier dump: vertical moves
                           (|dy| > 0.5), horizontal-only moves, resizes
     --screenshot DIR      save DIR/<lang>.png after settling

   exit: 2 any PASS rule failed (dump + shots still written); else 3 a
         vertical move was found by --diff-rects; else 0. 1 on crash, 77 when
         Playwright is unresolvable.

   usage (repo root):
     node plugins/O-ReverseDelay/tests/tools/cdp-font-probe.js --all-languages
*/
'use strict';

const fs = require('fs');
const os = require('os');
const path = require('path');
const { pathToFileURL } = require('url');

const REPO = path.resolve(__dirname, '..', '..', '..', '..');
const S = require(path.join(REPO, 'scripts', 'serve-ui.js'));
const PLUGIN = 'O-ReverseDelay';

const argv = process.argv.slice(2);
const ALL_LANGS = argv.includes('--all-languages');
function opt(name) { const i = argv.indexOf(name); return i >= 0 && argv[i + 1] ? argv[i + 1] : null; }
const DUMP = opt('--dump-rects');
const DIFF = opt('--diff-rects');
const SHOTS = opt('--screenshot');

const FACE = 'EB Garamond';
const CJK_PS = 'PingFangSC';
const CJK_FAMILIES = ['PingFang SC', 'Microsoft YaHei'];

// Nodes a before-vs-before run shows moving vertically on their own (stub
// animation). Excluded from the vertical-move count ONLY; the noise-floor
// control found none, so the list is empty.
const NOISE = [];

const TARGETS = [
    '.title', '.title-accent', '.subtitle', '.group-label', '.knob-label',
    '.preset-name', '.ab-slot', '.fleuron', '.footer-text',
];

const LATIN = /[A-Za-z0-9À-ɏ❦]/;
const HAN = /[㐀-鿿]/;
const NOT_IN_SUBSET = /[▾⚙]/;

function pad(s, n) { s = String(s); return s.length >= n ? s : s + ' '.repeat(n - s.length); }

(async () => {
    const pw = S.resolvePlaywright();
    if (!pw) { console.error('probe: Playwright unresolvable — NOTHING was measured (77)'); process.exit(77); }
    const { chromium } = pw;

    const built = S.buildRoot(PLUGIN, { repoRoot: REPO });
    const size = S.readEditorSize(PLUGIN, REPO);
    if (!size) { console.error('probe: no setSize'); process.exit(1); }

    const tmp = fs.mkdtempSync(path.join(os.tmpdir(), 'cdp-probe-'));
    const mjs = path.join(tmp, 'i18n.mjs');
    fs.copyFileSync(path.join(built.root, 'js', 'i18n.js'), mjs);
    const { LANGUAGES } = await import(pathToFileURL(mjs).href);
    const langs = ALL_LANGS ? LANGUAGES : ['en'];

    const misses = [];
    const srv = await S.serve(built.root, (rel) => misses.push(rel));
    const browser = await chromium.launch();
    const page = await browser.newPage({ viewport: { width: size.w, height: size.h } });
    const client = await page.context().newCDPSession(page);
    await client.send('DOM.enable');
    await client.send('CSS.enable');

    console.log(`# CDP CSS.getPlatformFontsForNode — ${PLUGIN}`);
    console.log(`# frame ${size.w}x${size.h}  ui=${built.uiRootLabel}  languages ${langs.join(', ')}  expected Latin face "${FACE}" (custom)`);
    if (built.placed.length) console.log(`# placed: ${built.placed.join(' | ')}`);
    if (built.unplaced.length) console.log(`# UNPLACED: ${built.unplaced.join(' | ')}`);
    if (built.missing.length) console.log(`# MISSING SOURCES: ${built.missing.join(' | ')}`);

    async function fontsFor(sel) {
        const { root } = await client.send('DOM.getDocument', { depth: -1 });
        const { nodeId } = await client.send('DOM.querySelector', { nodeId: root.nodeId, selector: sel });
        if (!nodeId) return null;
        const { fonts } = await client.send('CSS.getPlatformFontsForNode', { nodeId });
        return fonts;
    }

    const failures = [];
    const dump = {};
    if (SHOTS) fs.mkdirSync(SHOTS, { recursive: true });

    for (const lang of langs) {
        await page.goto(`http://127.0.0.1:${srv.port}/`, { waitUntil: 'networkidle' });
        await page.evaluate((l) => window.__setLanguage && window.__setLanguage(l), lang);
        await page.evaluate(() => document.fonts.ready);
        await page.waitForTimeout(250);

        console.log(`\n===== lang ${lang} =====`);
        for (const sel of TARGETS) {
            // The FIRST RENDERED match, not the first match: the first
            // .knob-label sits in the hidden Free-mode wrap, and a node that
            // paints nothing reports no platform fonts at all. The probe tags it
            // with a data attribute (no rule in styles.css selects on it) and
            // removes the tag before the rect dump.
            const info = await page.evaluate((s) => {
                const e = [...document.querySelectorAll(s)].find(n => n.getClientRects().length > 0);
                if (!e) return null;
                e.setAttribute('data-cdp-probe', '1');
                const pseudo = ['::before', '::after'].map(p => getComputedStyle(e, p).content)
                    .filter(c => c && c !== 'none' && c !== 'normal').join(' ');
                return { text: (e.textContent || '').replace(/\s+/g, ' ').trim().slice(0, 48), pseudo };
            }, sel);
            const f = info ? await fontsFor('[data-cdp-probe="1"]') : null;
            await page.evaluate(() => { for (const n of document.querySelectorAll('[data-cdp-probe]')) n.removeAttribute('data-cdp-probe'); });
            const text = info ? info.text : null;
            // Pseudo-element content (the preset menu's ::after caret) paints in
            // this node too, so it counts for the not-in-subset exemption.
            const painted = info ? `${info.text} ${info.pseudo}` : '';
            if (f === null || text === null) {
                failures.push(`${lang} ${sel}: node not found`);
                console.log(`  ${pad(sel, 14)} NODE NOT FOUND  <-- FAIL (a)`);
                continue;
            }
            const faces = f.map(x => `${x.familyName}${x.postScriptName ? '/' + x.postScriptName : ''}(${x.glyphCount}${x.isCustomFont ? ',custom' : ''})`).join('  ');
            const why = [];

            if (LATIN.test(text)) {
                const ok = f.some(x => x.familyName === FACE && x.isCustomFont && x.glyphCount > 0);
                if (!ok) why.push('(b) Latin run not on bundled EB Garamond');
                const stray = f.filter(x => !x.isCustomFont
                    && !CJK_FAMILIES.includes(x.familyName)
                    && !String(x.postScriptName || '').startsWith(CJK_PS));
                if (stray.length && !NOT_IN_SUBSET.test(painted))
                    why.push(`(b) stray system face ${stray.map(x => x.familyName).join(', ')}`);
            }
            if (lang === 'zh-Hans' && HAN.test(text)) {
                const onCjk = f.some(x => String(x.postScriptName || '').startsWith(CJK_PS));
                if (!onCjk) why.push('(c) Han run not on PingFang SC');
            }

            for (const w of why) failures.push(`${lang} ${sel}: ${w}`);
            console.log(`  ${pad(sel, 14)} ${faces || '(no platform fonts)'}   ${JSON.stringify(text)}${info.pseudo ? ' +pseudo ' + info.pseudo : ''}${why.length ? '  <-- FAIL ' + why.join('; ') : ''}`);
        }

        const faces = await page.evaluate(() => [...document.fonts].map(f => ({
            family: f.family.replace(/^["']|["']$/g, ''), weight: String(f.weight), style: f.style, status: f.status })));
        const ebg = faces.filter(f => f.family === 'EB Garamond');
        console.log(`  FontFaces: ${ebg.length ? ebg.map(f => `${f.family}/${f.weight}/${f.style}:${f.status}`).join('  ') : '(no EB Garamond FontFace)'}`);
        const want = ['400/normal', '400/italic', '700/normal'];
        const have = ebg.map(f => `${f.weight}/${f.style}`).sort();
        const facesOk = ebg.length === 3 && want.slice().sort().every((w, i) => have[i] === w) && ebg.every(f => f.status === 'loaded');
        if (!facesOk) failures.push(`${lang}: (d) expected exactly 3 loaded EB Garamond FontFaces (400/400i/700), got ${ebg.length}`);

        if (DUMP || DIFF) {
            dump[lang] = await page.evaluate(() => {
                const o = {}; let i = 0;
                for (const e of document.querySelectorAll('body *')) {
                    const r = e.getBoundingClientRect();
                    const key = (e.id ? '#' + e.id : e.tagName + '.' + [...e.classList].join('.')) + '#' + (i++);
                    o[key] = [r.x, r.y, r.width, r.height].map(n => Math.round(n * 10) / 10);
                }
                return o;
            });
        }
        if (SHOTS) await page.screenshot({ path: path.join(SHOTS, `${lang}.png`) });
    }

    if (misses.length) failures.push(`(e) unserved requests: ${misses.join(', ')}`);
    console.log(`\n# unserved requests: ${misses.length}${misses.length ? '  ' + JSON.stringify(misses) : ''}`);

    if (DUMP) { fs.writeFileSync(DUMP, JSON.stringify(dump)); console.log(`# rects written: ${DUMP}`); }

    let vertical = 0;
    if (DIFF) {
        const base = JSON.parse(fs.readFileSync(DIFF, 'utf8'));
        for (const lang of langs) {
            const a = base[lang], b = dump[lang];
            if (!a) { console.log(`# diff ${lang}: no baseline`); continue; }
            let vMoves = 0, hOnly = 0, resized = 0; const movers = [];
            for (const k of Object.keys(a)) {
                if (!b[k]) continue;
                const [x, y, w, h] = a[k], [x2, y2, w2, h2] = b[k];
                const dy = y2 - y, dx = x2 - x;
                const noisy = NOISE.some(n => k.startsWith(n));
                if (Math.abs(dy) > 0.5 && !noisy) { vMoves++; if (movers.length < 8) movers.push(`${k} dy=${dy.toFixed(1)}`); }
                else if (Math.abs(dx) > 0.5) hOnly++;
                if (Math.abs(w - w2) > 0.5 || Math.abs(h - h2) > 0.5) resized++;
            }
            const onlyA = Object.keys(a).filter(k => !b[k]).length, onlyB = Object.keys(b).filter(k => !a[k]).length;
            vertical += vMoves;
            console.log(`# diff ${lang}: elements ${Object.keys(a).length} (key-only before ${onlyA}, after ${onlyB}), vertical moves ${vMoves}, horizontal-only ${hOnly}, resized ${resized}${movers.length ? '; e.g. ' + movers.join(', ') : ''}`);
        }
    }

    await browser.close();
    await srv.close();
    fs.rmSync(tmp, { recursive: true, force: true });
    fs.rmSync(built.root, { recursive: true, force: true });

    if (failures.length) {
        console.log(`\nRESULT: FAIL (${failures.length})`);
        for (const f of failures) console.log(`  - ${f}`);
        process.exit(2);
    }
    if (vertical > 0) { console.log(`\nRESULT: PASS faces, but ${vertical} vertical move(s)`); process.exit(3); }
    console.log('\nRESULT: PASS');
    process.exit(0);
})().catch(e => { console.error('probe FAILED: ' + (e && e.stack || e)); process.exit(1); });
