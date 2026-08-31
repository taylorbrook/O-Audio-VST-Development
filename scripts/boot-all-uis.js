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

    boot-all-uis.js — load every plugin's WebView page headless and REPORT.

    This answers a question nobody had asked before Stage E: do all 43 pages
    even boot outside their own plugin? Five had a stub and were known to; the
    other 38 had never been rendered at all.

    ── It REPORTS. It does not fix, and it does not gate. ──────────────────────

    A page that will not boot is a FINDING, and a finding may change the plan for
    the stage that was going to localize it. Turning that into a red gate on the
    same commit that discovers it would mean either fixing 38 plugins in a tool
    commit or shipping a gate the repo is expected to ignore — and this repo
    already knows what a permanently-red gate does to the habit of reading gates.
    So the exit code is 0 whenever the RUN completed, and non-zero only when the
    run itself could not be trusted.

    Exit codes:
        0   the run completed; read the table for per-plugin verdicts
        2   --strict-tips was passed AND at least one DEAD tip binding was found
        77  Playwright unresolvable — NOTHING was verified (never a pass)
        1   the harness itself failed (no plugins discovered, etc.)

    Per plugin it records:
      - page errors (an uncaught throw during module evaluation, which is what a
        TDZ failure looks like — pattern_module_toplevel_init_tdz)
      - console.error calls
      - failed subresource requests, and 404s the server itself saw, which is how
        an embed-but-do-not-serve mistake surfaces
      - unknown native-function names the stub had to invent a value for
      - the count of elements carrying visible text, which is the number the
        i18n inventory has to account for
      - DEAD TIP BINDINGS. The canon warns `i18n: tip target not found: <sel>`
        at WARN level (i18n-canon.js), and until Stage M this handler dropped
        every non-`error` message before looking at it — so a TIP_BINDINGS row
        resolving to nothing was invisible here, invisible to check-i18n (which
        is static and only proves the KEY exists), and invisible to
        check-ui-labels (no tooltip awareness at all). No gate in the repo saw
        it. This now captures those warnings and, crucially, RE-QUERIES each
        missed selector after the settle, so a selector that misses the first
        sweep because its panel mounts late is reported as LATE, not as dead.
        Only a selector that still does not resolve at settle is DEAD.

    Usage:
        node scripts/boot-all-uis.js
        node scripts/boot-all-uis.js --plugin O-Tapestop
        node scripts/boot-all-uis.js --json /tmp/boot.json
        node scripts/boot-all-uis.js --verbose
        node scripts/boot-all-uis.js --strict-tips     # exit 2 on a DEAD binding

  ==============================================================================
*/

'use strict';

const fs   = require('fs');
const path = require('path');
const S    = require(path.join(__dirname, 'serve-ui.js'));

const argv    = process.argv.slice(2);
const val     = (f) => { const i = argv.indexOf(f); return i >= 0 && i + 1 < argv.length ? argv[i + 1] : null; };
const only    = val('--plugin');
const jsonOut = val('--json');
const verbose = argv.includes('--verbose');
// Opt-in ONLY. The default stays exit-0 for the reason in the header: this file
// reports, and a permanently-red gate teaches people to stop reading gates. But
// the dead-binding census is a genuine pass/fail question, so a caller that
// wants it enforced (CI, or a stage that has just finished authoring tips) can
// ask for it without every other caller inheriting a new failure mode.
const strictTips = argv.includes('--strict-tips');

// A module that never finishes evaluating leaves the page alive but inert, so
// waiting on `networkidle` alone can hang. Every page gets the same budget and
// a timeout is recorded as a finding rather than thrown.
const NAV_TIMEOUT_MS   = 20000;
const SETTLE_MS        = 900;

// Noise that is a property of running headless rather than of the page.
const IGNORABLE_CONSOLE = [
    /Failed to load resource.*favicon/i,
    /net::ERR_ABORTED.*favicon/i,
];

// The canon's runtime diagnostics, all emitted at WARN level by
// scripts/i18n-canon.js. The broad prefix is deliberate: a canon that grows a
// fourth diagnostic should land in i18nWarnings without an edit here, and only
// the two that carry a NAME need an extractor.
//
// These are matched against the literal text the canon writes, so they drift
// silently if that wording changes. The standing control: O-Bells has exactly
// two lazily-mounted tuning-panel anchors (#ref-pitch-knob, #octave-stretch)
// and MUST report 2 late / 0 dead. A run that reports 0 for O-Bells means
// these patterns have drifted, not that the page got better. (The first draft
// of this block failed exactly that way — it required a space where the canon
// writes a colon.)
const I18N_DIAGNOSTIC = /^i18n: /;
const TIP_MISS        = /^i18n: tip target not found: (.+)$/;
const LABEL_MISS      = /^i18n: missing label key (.+)$/;

(async () => {
    const pw = S.resolvePlaywright();
    if (pw == null) {
        console.log('SKIP: playwright not resolvable. Install with');
        console.log('      npx playwright install chromium');
        console.log('NOTHING was booted. This is not a pass.');
        process.exit(77);
    }
    const { chromium } = pw;

    const plugins = S.listPlugins().filter((n) => !only || n === only);
    if (plugins.length === 0) {
        console.error(`boot-all-uis: no plugins discovered${only ? ` matching --plugin ${only}` : ''}`);
        process.exit(1);
    }

    console.log('boot-all-uis — headless render of every plugin WebView page');
    console.log(`  plugins: ${plugins.length}\n`);

    const browser = await chromium.launch();
    const results = [];

    for (const name of plugins) {
        const r = {
            plugin: name, ok: false, stub: null, uiRoot: null, size: null,
            pageErrors: [], consoleErrors: [], requestFailures: [], serverMisses: [],
            unknownNativeFns: [], textElements: 0, dataI18n: 0,
            titleAttrs: 0, ariaAttrs: 0, note: null,
            i18nWarnings: [], missedTipSelectors: [],
            deadTipBindings: [], lateTipBindings: [], missingLabelKeys: [],
        };

        let built = null;
        let srv   = null;
        let page  = null;
        let ctx   = null;

        try {
            built = S.buildRoot(name);
            r.stub    = built.stubKind;
            r.uiRoot  = built.uiRootLabel;
            r.seed    = built.seedFrom;
            if (built.missing.length)  r.note = `CMake SOURCES not on disk: ${built.missing.join(', ')}`;
            if (built.unplaced.length) r.note = [r.note, `unmapped embed: ${built.unplaced.join(', ')}`].filter(Boolean).join(' | ');

            const size = S.readEditorSize(name);
            if (!size) { r.pageErrors.push('no setSize(W,H) parsed from PluginEditor.cpp'); throw new Error('no size'); }
            r.size = `${size.w}x${size.h}`;

            srv = await S.serve(built.root, (rel) => r.serverMisses.push(rel));

            ctx  = await browser.newContext({ viewport: { width: size.w, height: size.h } });
            page = await ctx.newPage();

            page.on('pageerror', (e) => r.pageErrors.push(String(e && e.message ? e.message : e)));
            page.on('console', (m) => {
                const t = m.text();
                // The canon's own diagnostics are WARN level. They must be read
                // before the error filter, not after it — that ordering is the
                // whole bug this block fixes.
                if (I18N_DIAGNOSTIC.test(t)) {
                    r.i18nWarnings.push(t);
                    const miss = t.match(TIP_MISS);
                    if (miss) r.missedTipSelectors.push(miss[1]);
                    const key = t.match(LABEL_MISS);
                    if (key) r.missingLabelKeys.push(key[1]);
                }
                if (m.type() !== 'error') return;
                if (IGNORABLE_CONSOLE.some((re) => re.test(t))) return;
                r.consoleErrors.push(t);
            });
            page.on('requestfailed', (req) => {
                const u = req.url();
                if (/favicon/.test(u)) return;
                r.requestFailures.push(`${u} (${(req.failure() || {}).errorText || 'failed'})`);
            });

            await page.goto(`http://127.0.0.1:${srv.port}/index.html`,
                            { waitUntil: 'load', timeout: NAV_TIMEOUT_MS });
            await page.waitForTimeout(SETTLE_MS);

            const counts = await page.evaluate(() => {
                // "Carrying visible text" means a LEAF element whose own text has
                // two consecutive letters — the same predicate the static scan
                // used, so the two numbers are comparable. Counting every
                // ancestor would report the same string once per nesting level.
                const isVisible = (el) => {
                    const cs = getComputedStyle(el);
                    if (cs.display === 'none' || cs.visibility === 'hidden') return false;
                    return true;
                };
                let text = 0;
                for (const el of document.body ? document.body.querySelectorAll('*') : []) {
                    if (el.tagName === 'SCRIPT' || el.tagName === 'STYLE') continue;
                    const own = [...el.childNodes]
                        .filter((n) => n.nodeType === 3)
                        .map((n) => n.textContent)
                        .join('');
                    if (!/[A-Za-zÀ-ÿ]{2}/.test(own)) continue;
                    if (!isVisible(el)) continue;
                    ++text;
                }
                return {
                    text,
                    dataI18n: document.querySelectorAll('[data-i18n]').length,
                    titleAttrs: document.querySelectorAll('[title]').length,
                    ariaAttrs: document.querySelectorAll('[aria-label]').length,
                    unknown: (window.__stubUnknownNativeFns || []).slice(),
                };
            });

            r.textElements     = counts.text;
            r.dataI18n         = counts.dataI18n;
            r.titleAttrs       = counts.titleAttrs;
            r.ariaAttrs        = counts.ariaAttrs;
            r.unknownNativeFns = counts.unknown;

            // A selector that missed the FIRST sweep may simply belong to a
            // panel that had not mounted yet. Re-querying it after the settle
            // is what separates a late anchor from a dead binding, and it is
            // the only reason this census is worth reading: without it every
            // lazily-mounted panel in the suite would be reported as a defect.
            const missed = [...new Set(r.missedTipSelectors)];
            if (missed.length) {
                const now = await page.evaluate((sels) => sels.map((sel) => {
                    try { return [sel, document.querySelector(sel) !== null]; }
                    catch (e) { return [sel, null]; }   // invalid selector: neither
                }), missed);
                for (const [sel, resolves] of now) {
                    if (resolves === true) r.lateTipBindings.push(sel);
                    else                   r.deadTipBindings.push(sel);
                }
            }

            r.ok = r.pageErrors.length === 0
                && r.consoleErrors.length === 0
                && r.requestFailures.length === 0;
        } catch (e) {
            r.pageErrors.push(String(e && e.message ? e.message : e));
        } finally {
            if (page) await page.close().catch(() => {});
            if (ctx)  await ctx.close().catch(() => {});
            if (srv)  await srv.close().catch(() => {});
            if (built) fs.rmSync(built.root, { recursive: true, force: true });
        }

        results.push(r);

        const flag = r.ok ? 'BOOT' : (r.pageErrors.length ? 'FAIL' : 'WARN');
        console.log(`  ${flag}  ${r.plugin.padEnd(30)} ${String(r.size || '-').padEnd(10)}`
            + ` stub=${String(r.stub || '-').padEnd(8)} text=${String(r.textElements).padStart(4)}`
            + ` aria=${String(r.ariaAttrs).padStart(3)} title=${String(r.titleAttrs).padStart(3)}`
            + ` i18n=${String(r.dataI18n).padStart(3)}`
            + (r.deadTipBindings.length ? `  DEAD-TIPS=${r.deadTipBindings.length}` : '')
            + (r.lateTipBindings.length ? `  late-tips=${r.lateTipBindings.length}` : '')
            + (r.missingLabelKeys.length ? `  missing-keys=${new Set(r.missingLabelKeys).size}` : ''));

        for (const sel of r.deadTipBindings)
            console.log(`        DEAD TIP:  ${sel}  (TIP_BINDINGS row resolves to nothing)`);
        if (verbose) {
            for (const sel of r.lateTipBindings)
                console.log(`        late tip:  ${sel}  (missed first sweep, resolves at settle)`);
            for (const k of [...new Set(r.missingLabelKeys)])
                console.log(`        no key:    ${k}`);
        }

        if (!r.ok || verbose) {
            for (const e of r.pageErrors)      console.log(`        pageerror: ${e.slice(0, 200)}`);
            for (const e of r.consoleErrors)   console.log(`        console:   ${e.slice(0, 200)}`);
            for (const e of r.requestFailures) console.log(`        request:   ${e.slice(0, 200)}`);
            for (const e of [...new Set(r.serverMisses)]) console.log(`        404:       ${e}`);
            if (r.note) console.log(`        note:      ${r.note}`);
        }
        if (verbose && r.unknownNativeFns.length)
            console.log(`        stub invented: ${r.unknownNativeFns.join(', ')}`);
    }

    await browser.close();

    // ── the report ──
    const booted = results.filter((r) => r.ok);
    const warned = results.filter((r) => !r.ok && r.pageErrors.length === 0);
    const failed = results.filter((r) => r.pageErrors.length > 0);

    console.log('\n-- boot verdict');
    console.log(`  clean:  ${booted.length} / ${results.length}`);
    console.log(`  warn:   ${warned.length}  (rendered, but console errors or failed subresources)`);
    console.log(`  failed: ${failed.length}  (uncaught throw, or could not be assembled)`);

    if (warned.length) { console.log('\n  WARN:'); for (const r of warned) console.log(`    ${r.plugin}`); }
    if (failed.length) { console.log('\n  FAIL:'); for (const r of failed) console.log(`    ${r.plugin}: ${r.pageErrors[0].slice(0, 160)}`); }

    // ── tip-binding census ──────────────────────────────────────────────
    // Deliberately its OWN verdict, not folded into `clean: N/43`. The boot
    // verdict answers "does the page load"; a dead tip binding loads fine. One
    // number that meant both would be the same conflation that let this go
    // unseen in the first place (pattern_repo_wide_gate_count_is_not_a_batch_verdict).
    const dead = results.filter((r) => r.deadTipBindings.length);
    const late = results.filter((r) => r.lateTipBindings.length);
    const keys = results.filter((r) => r.missingLabelKeys.length);
    const totDead = results.reduce((a, r) => a + r.deadTipBindings.length, 0);
    const totLate = results.reduce((a, r) => a + r.lateTipBindings.length, 0);

    console.log('\n-- tip-binding census   (separate from the boot verdict above)');
    console.log(`  DEAD bindings:  ${totDead}  across ${dead.length} plugin(s)`
        + '   — TIP_BINDINGS rows whose selector resolves to nothing, at settle');
    console.log(`  late bindings:  ${totLate}  across ${late.length} plugin(s)`
        + '   — missed the first sweep, resolve later; need a re-sweep to carry a tip');
    if (dead.length) {
        console.log('\n  DEAD:');
        for (const r of dead)
            console.log(`    ${r.plugin.padEnd(30)} ${r.deadTipBindings.join(', ')}`);
    }
    if (late.length) {
        console.log('\n  late (informational):');
        for (const r of late)
            console.log(`    ${r.plugin.padEnd(30)} ${r.lateTipBindings.join(', ')}`);
    }
    if (keys.length) {
        const totKeys = results.reduce((a, r) => a + new Set(r.missingLabelKeys).size, 0);
        console.log(`\n  missing label keys: ${totKeys} across ${keys.length} plugin(s)`);
        for (const r of keys)
            console.log(`    ${r.plugin.padEnd(30)} ${[...new Set(r.missingLabelKeys)].join(', ')}`);
    }
    if (!totDead && !totLate && !keys.length)
        console.log('  no i18n runtime diagnostics on any page.');

    const totText  = results.reduce((a, r) => a + r.textElements, 0);
    const totAria  = results.reduce((a, r) => a + r.ariaAttrs, 0);
    const totTitle = results.reduce((a, r) => a + r.titleAttrs, 0);
    console.log(`\n  rendered text-bearing elements: ${totText}   aria-label: ${totAria}   title: ${totTitle}`);

    const unknown = new Map();
    for (const r of results) for (const n of r.unknownNativeFns) unknown.set(n, (unknown.get(n) || 0) + 1);
    if (unknown.size) {
        console.log(`\n  native fns the stub INVENTED a value for (${unknown.size} distinct) —`);
        console.log('  this stub reports rather than rejects, so these are NOT a bridge-parity claim:');
        const top = [...unknown.entries()].sort((a, b) => b[1] - a[1]).slice(0, 25);
        console.log('    ' + top.map(([n, c]) => `${n}(${c})`).join(', '));
    }

    if (jsonOut) {
        fs.writeFileSync(jsonOut, JSON.stringify(results, null, 2));
        console.log(`\n  json: ${jsonOut}`);
    }

    console.log('\nThis is a REPORT, not a gate. Exit 0 means the run completed —'
              + ' it does NOT mean the tip-binding census above is empty.');
    if (strictTips && totDead) {
        console.log(`--strict-tips: FAIL — ${totDead} dead tip binding(s). Exit 2.`);
        process.exit(2);
    }
    process.exit(0);
})().catch((e) => {
    console.error('boot-all-uis: harness failure —', e);
    process.exit(1);
});
