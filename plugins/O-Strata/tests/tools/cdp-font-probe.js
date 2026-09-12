/* CDP resolved-font probe — O-Strata Terrain tab (Stage 3 Round A, PLAN Task 9).
   Copied from .planning/quick/260906-uu7-…/cdp-font-probe.js (O-Chorus) with
   PLUGIN = 'O-Strata' and the Terrain-tab text runs as targets. Reports the
   RESOLVED platform font per selector under zh-Hans via
   CSS.getPlatformFontsForNode — measure-ui.js's `ff` field is the DECLARED
   stack and cannot answer this (memory
   pattern_declared_font_stack_is_not_the_resolved_face).

   Boot is copied from scripts/measure-ui.js (serve-ui buildRoot/serve,
   readEditorSize frame, window.__setLanguage). The Terrain tab is opened with
   switchTab('terrain'); #terrain-notice is display:none until a pushed
   terrainStatus sets sourceMissing, so the probe emits one through the
   generic stub's window.__stubEmit before measuring (as tests/i18n-states.json
   does).

   usage (repo root): node plugins/O-Strata/tests/tools/cdp-font-probe.js [--all-languages]
   Promotion to scripts/ is an open item.
*/
'use strict';

const fs = require('fs');
const os = require('os');
const path = require('path');
const { pathToFileURL } = require('url');

const REPO = path.resolve(__dirname, '..', '..', '..', '..');
const S = require(path.join(REPO, 'scripts', 'serve-ui.js'));
const PLUGIN = 'O-Strata';
const argv = process.argv.slice(2);
const ALL_LANGS = argv.includes('--all-languages');
const DECLARED_CJK = 'PingFang SC';
// CDP reports the LOCALISED family name ("蘋方-簡" on a zh system locale), so the
// face is matched on its PostScript name prefix, which is locale-free.
const DECLARED_CJK_PS = 'PingFangSC';

// Every text run the Terrain tab renders (readouts included — a numeral on the
// wrong face is still the wrong face).
const TARGETS = [
    '#terrain-tab .knob-label',
    '#terrain-tab .knob-value',
    '.terrain-seg-btn',
    '#seg-terQuality .terrain-seg-btn[data-idx="0"]',
    '#terrain-readout',
    '#terrain-readout span',
    '#terrain-notice',
    '.wt-osc-btn',
    '#btn-terImport',
    '.terrain-ctl-label',
    '.terrain-hint',
    '#terrain-hud',
    '#terrain-tab .section-header',
    '#terrain-tab .dropdown-label',
    '#terrain-tab .param-select',
    '.tab[data-tab="terrain"]',
];

(async () => {
    const pw = S.resolvePlaywright();
    if (!pw) { console.error('probe: Playwright unresolvable — NOTHING was measured (77)'); process.exit(77); }
    const { chromium } = pw;

    const built = S.buildRoot(PLUGIN, { repoRoot: REPO });
    const size  = S.readEditorSize(PLUGIN, REPO);
    if (!size) { console.error('probe: no setSize'); process.exit(1); }

    const tmp = fs.mkdtempSync(path.join(os.tmpdir(), 'cdp-probe-'));
    const mjs = path.join(tmp, 'i18n.mjs');
    fs.copyFileSync(path.join(built.uiRoot, 'js', 'i18n.js'), mjs);
    const { LANGUAGES } = await import(pathToFileURL(mjs).href);
    const langs = ALL_LANGS ? LANGUAGES : ['zh-Hans'];

    const srv = await S.serve(built.root);
    const browser = await chromium.launch();
    const page = await browser.newPage({ viewport: { width: size.w, height: size.h } });

    console.log(`# CDP CSS.getPlatformFontsForNode — ${PLUGIN} Terrain tab`);
    console.log(`# frame ${size.w}x${size.h}  ui=${built.uiRootLabel}  languages ${langs.join(', ')}  declared CJK face "${DECLARED_CJK}"`);
    console.log(`# ${new Date().toISOString()}`);

    const client = await page.context().newCDPSession(page);
    await client.send('DOM.enable');
    await client.send('CSS.enable');

    async function fontsFor(sel) {
        const { root } = await client.send('DOM.getDocument', { depth: -1 });
        const { nodeId } = await client.send('DOM.querySelector', { nodeId: root.nodeId, selector: sel });
        if (!nodeId) return null;
        const { fonts } = await client.send('CSS.getPlatformFontsForNode', { nodeId });
        return fonts;
    }

    let offFace = 0, measured = 0, missing = 0;
    for (const lang of langs) {
        await page.goto(`http://127.0.0.1:${srv.port}/`, { waitUntil: 'networkidle' });
        await page.evaluate((l) => window.__setLanguage && window.__setLanguage(l), lang);
        await page.waitForTimeout(220);
        await page.evaluate(() => { window.switchTab('terrain'); });
        await page.waitForTimeout(120);
        // Reveal #terrain-notice (display:none until sourceMissing) through the pushed event.
        const emitted = await page.evaluate(() => {
            if (typeof window.__stubEmit !== 'function') return false;
            window.__stubEmit('terrainStatus', { osc: 'A', sourceMissing: true, fit: 87, partials: 9 });
            return true;
        });
        await page.waitForTimeout(120);

        console.log(`\n===== lang ${lang}  (terrainStatus emitted=${emitted}) =====`);
        for (const sel of TARGETS) {
            const txt = await page.evaluate((s) => {
                const e = document.querySelector(s);
                if (!e) return null;
                if (e.tagName === 'SELECT') return 'SELECTED=' + (e.options[e.selectedIndex] ? e.options[e.selectedIndex].textContent : '?');
                const cs = getComputedStyle(e);
                return { text: (e.textContent || '').trim().slice(0, 40), display: cs.display, vis: cs.visibility };
            }, sel);
            const f = await fontsFor(sel);
            if (f === null) { missing++; console.log(`  ${pad(sel, 46)} NODE NOT FOUND`); continue; }
            if (!f.length) { console.log(`  ${pad(sel, 46)} NO PLATFORM FONTS REPORTED  ${JSON.stringify(txt)}`); continue; }
            measured++;
            const faces = f.map(x => `${x.familyName}${x.postScriptName ? '/' + x.postScriptName : ''}(${x.glyphCount}${x.isCustomFont ? ',custom' : ''})`).join('  ');
            // Han glyphs must come from the declared CJK face; Latin / digit glyphs
            // may legitimately come from Garamond / Georgia earlier in the stack.
            const hasHan = /[㐀-鿿]/.test(txt && txt.text || '');
            const onCjk = f.some(x => x.familyName === DECLARED_CJK || String(x.postScriptName || '').startsWith(DECLARED_CJK_PS));
            const verdict = lang === 'zh-Hans' && hasHan && !onCjk ? '  <-- OFF-FACE' : '';
            if (verdict) offFace++;
            console.log(`  ${pad(sel, 46)} ${faces}   ${JSON.stringify(txt)}${verdict}`);
        }
    }

    function pad(s, n) { s = String(s); return s.length >= n ? s : s + ' '.repeat(n - s.length); }

    console.log(`\n# runs measured ${measured}, nodes missing ${missing}, Han runs off the declared face ${offFace}`);
    await browser.close();
    await srv.close();
    fs.rmSync(tmp, { recursive: true, force: true });
    fs.rmSync(built.root, { recursive: true, force: true });
    process.exit(offFace === 0 && missing === 0 ? 0 : 2);
})().catch(e => { console.error('probe FAILED: ' + (e && e.stack || e)); process.exit(1); });
