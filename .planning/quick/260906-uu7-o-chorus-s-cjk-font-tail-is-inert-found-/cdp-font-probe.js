/* Throwaway CDP probe for quick task 260906-uu7.
   Reports the RESOLVED platform font per selector per language via
   CSS.getPlatformFontsForNode. measure-ui.js's `ff` field is the DECLARED
   stack and cannot answer this.

   Boot is copied from scripts/measure-ui.js (serve-ui buildRoot/serve,
   readEditorSize frame, window.__setLanguage) and the tip-open gesture is
   copied from plugins/O-Chorus/tests/ui_tip_render_check.js hoverAnchor().

   usage: node cdp-font-probe.js [--label BEFORE|AFTER]
*/
'use strict';

const fs = require('fs');
const os = require('os');
const path = require('path');
const { pathToFileURL } = require('url');

const REPO = '/Users/taylorbrook/Dev/VST-development';
const S = require(path.join(REPO, 'scripts', 'serve-ui.js'));
const PLUGIN = 'O-Chorus';
const argv = process.argv.slice(2);
const LABEL = (argv.indexOf('--label') >= 0 ? argv[argv.indexOf('--label') + 1] : 'RUN');

// selector -> { state: 'closed' | 'tip' | 'open' }
const TARGETS = [
    { sel: '.knob-label',      state: 'closed' },
    { sel: '.preset-action',   state: 'closed' },
    { sel: '.settings-label',  state: 'open'   },
    { sel: '.settings-select', state: 'open'   },
    { sel: '.settings-toggle', state: 'open'   },
    { sel: '.tooltip',         state: 'tip'    },
    { sel: '.tip-title',       state: 'tip'    },
    // The THREE TAIL-LESS declarations, probed for the wave-4a sibling defect
    // (qrc SUMMARY s.2): a stack whose only surviving family is the bare
    // generic renders its glyphs through a LANGUAGE-RESOLVED face too.
    { sel: '#preset-prev',     state: 'closed' },
    { sel: '#preset-next',     state: 'closed' },
    { sel: '#preset-load',     state: 'closed' },
    { sel: '#gear-btn',        state: 'closed' },
    { sel: '.preset-dropdown-item', state: 'dropdown' },
];

(async () => {
    const pw = S.resolvePlaywright();
    if (!pw) { console.error('probe: Playwright unresolvable — NOTHING was measured (77)'); process.exit(77); }
    const { chromium } = pw;

    const built = S.buildRoot(PLUGIN, { repoRoot: REPO });
    const size  = S.readEditorSize(PLUGIN, REPO);
    if (!size) { console.error('probe: no setSize'); process.exit(1); }

    // LANGUAGES off the RESOLVED ui root, exactly as measure-ui does.
    const tmp = fs.mkdtempSync(path.join(os.tmpdir(), 'cdp-probe-'));
    const mjs = path.join(tmp, 'i18n.mjs');
    fs.copyFileSync(path.join(built.uiRoot, 'js', 'i18n.js'), mjs);
    const { LANGUAGES } = await import(pathToFileURL(mjs).href);

    const srv = await S.serve(built.root);
    const browser = await chromium.launch();
    const page = await browser.newPage({ viewport: { width: size.w, height: size.h } });

    console.log(`# CDP CSS.getPlatformFontsForNode — ${PLUGIN} — ${LABEL}`);
    console.log(`# frame ${size.w}x${size.h}  ui=${built.uiRootLabel}  languages ${LANGUAGES.join(', ')}`);
    console.log(`# ${new Date().toISOString()}`);

    const client = await page.context().newCDPSession(page);
    await client.send('DOM.enable');
    await client.send('CSS.enable');

    async function fontsFor(sel) {
        // Fresh document each resolve — node ids are per-getDocument.
        const { root } = await client.send('DOM.getDocument', { depth: -1 });
        const { nodeId } = await client.send('DOM.querySelector', { nodeId: root.nodeId, selector: sel });
        if (!nodeId) return null;
        const { fonts } = await client.send('CSS.getPlatformFontsForNode', { nodeId });
        return fonts;
    }

    async function hoverAnchor(sel, wrapper) {
        await page.mouse.move(size.w - 2, 2);
        await page.waitForTimeout(60);
        const box = await page.evaluate(({ sel, wrapper }) => {
            const el = document.querySelector(sel);
            if (!el) return null;
            const target = wrapper ? (el.closest(wrapper) || el) : el;
            const r = target.getBoundingClientRect();
            return { x: r.left + r.width / 2, y: r.top + r.height / 2 };
        }, { sel, wrapper: wrapper || null });
        if (!box) return false;
        await page.mouse.move(box.x, box.y);
        await page.waitForTimeout(220);
        return true;
    }

    for (const lang of LANGUAGES) {
        await page.goto(`http://127.0.0.1:${srv.port}/`, { waitUntil: 'networkidle' });
        await page.evaluate((l) => window.__setLanguage && window.__setLanguage(l), lang);
        await page.waitForTimeout(220);

        console.log(`\n===== lang ${lang} =====`);

        // --- CLOSED state ---
        for (const t of TARGETS.filter(t => t.state === 'closed')) {
            const txt = await page.evaluate((s) => {
                const e = document.querySelector(s);
                if (!e) return null;
                const own = (e.textContent || '').trim().slice(0, 40);
                const al = e.getAttribute('aria-label');
                return al ? own + '  [aria-label=' + al + ']' : own;
            }, t.sel);
            const f = await fontsFor(t.sel);
            print(lang, t.sel, 'closed', txt, f);
        }

        // --- TIP state: hover a knob so the tip renders, tip-title included ---
        const hovered = await hoverAnchor('.knob[data-param="drive"]', '.knob-container');
        const tipTxt = await page.evaluate(() => {
            const t = document.getElementById('tooltip');
            return t ? { shown: t.classList.contains('show'), text: (t.textContent || '').trim().slice(0, 60) } : null;
        });
        console.log(`  [tip gesture] hovered=${hovered} shown=${tipTxt && tipTxt.shown} text=${JSON.stringify(tipTxt && tipTxt.text)}`);
        for (const t of TARGETS.filter(t => t.state === 'tip')) {
            const f = await fontsFor(t.sel);
            print(lang, t.sel, 'tip-open', tipTxt && tipTxt.text, f);
        }
        // RENDERED tooltip geometry. measure-ui.js NEVER opens the tip
        // (tests/i18n-states.json only clicks the gear), so its #tooltip and
        // .tip-title rows are the at-rest hidden box. These are the only
        // BEFORE/AFTER numbers for the 384 px wrap ceiling.
        const tipGeo = await page.evaluate(() => {
            const t = document.getElementById('tooltip');
            if (!t) return null;
            const cs = getComputedStyle(t);
            const r = t.getBoundingClientRect();
            const tt = t.querySelector('.tip-title');
            const tr = tt ? tt.getBoundingClientRect() : null;
            const tcs = tt ? getComputedStyle(tt) : null;
            const lh = parseFloat(cs.lineHeight) || 0;
            const inner = r.height - parseFloat(cs.paddingTop) - parseFloat(cs.paddingBottom)
                        - parseFloat(cs.borderTopWidth) - parseFloat(cs.borderBottomWidth)
                        - (tr ? tr.height + parseFloat(tcs.marginBottom) : 0);
            return {
                show: t.classList.contains('show'), vis: cs.visibility,
                tip: { w: +r.width.toFixed(2), h: +r.height.toFixed(2), lh: cs.lineHeight, fs: cs.fontSize },
                title: tr ? { w: +tr.width.toFixed(2), h: +tr.height.toFixed(2), lh: tcs.lineHeight, fs: tcs.fontSize } : null,
                bodyLines: lh ? Math.round(inner / lh) : null,
            };
        });
        console.log('  [tip geometry RENDERED] ' + JSON.stringify(tipGeo));

        // --- DROPDOWN state: open the preset list so its items are rendered ---
        await page.mouse.move(size.w - 2, 2);
        try { await page.click('#preset-menu-btn, #preset-dropdown-btn, .preset-name, #preset-display', { timeout: 1200, force: true }); }
        catch (e) { console.log('  [dropdown gesture] no dropdown opener matched — ' + String(e.message).split('\n')[0]); }
        await page.waitForTimeout(180);
        for (const t of TARGETS.filter(t => t.state === 'dropdown')) {
            const txt = await page.evaluate((s) => { const e = document.querySelector(s); return e ? (e.textContent||'').trim().slice(0,40) : null; }, t.sel);
            const f = await fontsFor(t.sel);
            print(lang, t.sel, 'dropdown', txt, f);
        }

        // --- OPEN state: click #gear-btn, as tests/i18n-states.json does ---
        await page.mouse.move(size.w - 2, 2);
        await page.click('#gear-btn', { timeout: 1500, force: true });
        await page.waitForTimeout(200);
        const popOpen = await page.evaluate(() => {
            const p = document.getElementById('settings-popover');
            return p ? !p.hidden : null;
        });
        console.log(`  [gear gesture] settings-popover open=${popOpen}`);
        // ── THE COUNTERFACTUALS THE PIN COMMENTS QUOTE ────────────────────
        // Every line-height pin comment on this page quotes a number measured
        // WITH THE PIN REMOVED (EN 10.00 px / ZH 13.00 px "at line-height:
        // normal"), and the width pin quotes the select's INTRINSIC width. The
        // pins override both, so measure-ui.js rows can never reproduce either.
        // Measured here by temporarily overriding the declaration in-page; the
        // override is reverted immediately and nothing is written to disk.
        const cf = await page.evaluate(() => {
            const box = (el) => {
                const cs = getComputedStyle(el), r = el.getBoundingClientRect();
                return +(r.height - parseFloat(cs.paddingTop) - parseFloat(cs.paddingBottom)
                       - parseFloat(cs.borderTopWidth) - parseFloat(cs.borderBottomWidth)).toFixed(3);
            };
            const out = {};
            for (const sel of ['.knob-label', '.preset-action', '.settings-label', '.settings-toggle']) {
                const el = document.querySelector(sel);
                if (!el) { out[sel] = null; continue; }
                const prev = el.style.lineHeight;
                const pinned = box(el);
                el.style.setProperty('line-height', 'normal', 'important');
                void el.offsetHeight;
                const normal = box(el);
                el.style.lineHeight = prev;
                out[sel] = { pinnedLineBox: pinned, normalLineBox: normal, text: (el.textContent||'').trim().slice(0,20) };
            }
            const sel = document.querySelector('.settings-select');
            if (sel) {
                const prevW = sel.style.width;
                const pinnedW = +sel.getBoundingClientRect().width.toFixed(3);
                sel.style.setProperty('width', 'auto', 'important');
                void sel.offsetWidth;
                const intrinsicW = +sel.getBoundingClientRect().width.toFixed(3);
                sel.style.width = prevW;
                // The three endonym option widths, measured in a detached span
                // that inherits the select's own stack.
                const probe = document.createElement('span');
                const scs = getComputedStyle(sel);
                probe.style.cssText = 'position:absolute;visibility:hidden;white-space:pre;'
                    + 'font-family:' + scs.fontFamily + ';font-size:' + scs.fontSize
                    + ';letter-spacing:' + scs.letterSpacing;
                document.body.appendChild(probe);
                const opts = {};
                for (const o of sel.options) { probe.textContent = o.textContent; opts[o.textContent] = +probe.getBoundingClientRect().width.toFixed(3); }
                probe.remove();
                out['.settings-select'] = { pinnedWidth: pinnedW, intrinsicWidth: intrinsicW,
                                            selected: sel.options[sel.selectedIndex].textContent,
                                            computedLineHeight: scs.lineHeight, optionWidths: opts };
            }
            return out;
        });
        console.log('  [counterfactual pins] ' + JSON.stringify(cf, null, 0));

        for (const t of TARGETS.filter(t => t.state === 'open')) {
            const txt = await page.evaluate((s) => {
                const e = document.querySelector(s);
                if (!e) return null;
                if (e.tagName === 'SELECT') return 'SELECTED=' + (e.options[e.selectedIndex] ? e.options[e.selectedIndex].textContent : '?');
                return (e.textContent || '').trim().slice(0, 40);
            }, t.sel);
            const f = await fontsFor(t.sel);
            print(lang, t.sel, 'popover-open', txt, f);
        }
    }

    function print(lang, sel, state, txt, fonts) {
        if (fonts === null) { console.log(`  ${pad(sel)} ${pad(state, 14)} NODE NOT FOUND`); return; }
        if (!fonts.length)  { console.log(`  ${pad(sel)} ${pad(state, 14)} NO PLATFORM FONTS REPORTED  text=${JSON.stringify(txt)}`); return; }
        const faces = fonts.map(f => `${f.familyName}${f.postScriptName ? '/' + f.postScriptName : ''}(${f.glyphCount}${f.isCustomFont ? ',custom' : ''})`).join('  ');
        console.log(`  ${pad(sel)} ${pad(state, 14)} ${faces}   text=${JSON.stringify(txt)}`);
    }
    function pad(s, n) { s = String(s); n = n || 18; return s.length >= n ? s : s + ' '.repeat(n - s.length); }

    await browser.close();
    await srv.close();
    fs.rmSync(tmp, { recursive: true, force: true });
    fs.rmSync(built.root, { recursive: true, force: true });
})().catch(e => { console.error('probe FAILED: ' + (e && e.stack || e)); process.exit(1); });
