/*
   This file is part of O-GranulatorInstrument, an Ouaricon Audio plugin.
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

    ui_binding_probe.js
    O-GranulatorInstrument — two-way page binding probe, all 261 parameters,
    plus the value-entry rows VE1–VE12 (Stage 3 / Phase 3.1, PLAN-3.1 PD19,
    PD16; RESEARCH-3.1 §7).

    Copied from plugins/O-Granulator/tests/ui_binding_probe.js and adapted.
    Serves the page through scripts/serve-ui.js with the generic JUCE stub
    seeded from .planning/params.tsv + tests/ui-stub/generic-overrides.json
    (C++-generated, PD12), drives it in headless Chromium at 1200 x 800 and,
    for every parameter id in params.tsv:

      host -> UI   what a relay does on automation / preset recall: write the
                   stub state's value and fire its valueChangedEvent, at TWO
                   distinct values, and assert the control's DOM followed —
                     knob        val-<id> text changed, == aria-valuetext, stem
                                 rotated; restoring the value restores the text
                     fader       val-<id> text changed, input.value == normalised
                     handle      style.left and aria-valuetext changed; restored
                     field       text changed, == aria-valuetext; restored
                     choice knob val-<id> text == choices[idx] (or its localized
                                 face changed), aria-valuenow == idx
                     segment     exactly the button data-index == idx pressed
                     select      selectedIndex == idx, one option per choice
                     radio       exactly the button data-index == idx checked
                     scale       trigger label changed; restoring restores it
                     toggle      aria-pressed == value, both values
      UI -> host   a real input event on the control — ArrowUp (knobs, choice
                   knobs, fields), ArrowRight (faders, handles), click (toggles,
                   segments, the radio pips, the scale list), selectOption
                   (selects) — and assert the stub state's value changed; for
                   continuous controls the call sequence on the state is exactly
                   sliderDragStarted, setNormalisedValue, sliderDragEnded.

    Hidden tabs are reached by clicking #tab-<page> first; Tempo-only and
    Free-only cells by setting sync_mode through its own state first. Every
    value is restored after each probe.

    LIVENESS: passes only with 261 / 261 assertions in EACH direction (an id
    the page never binds has no state -> its row fails, it is not skipped).

    VALUE ENTRY (PD16), after liveness, each with real clicks and keys:
      VE1  Enter applies                    VE7  clamp above `end`
      VE2  Esc cancels                      VE8  -inf on a gain
      VE3  outside click applies            VE9  fCount (polyphony)
      VE4  outside press on a choice knob   VE10 fBend with and without ± / st
           (its pointerdown preventDefaults) VE11 a tempo division
      VE5  unchanged text writes nothing    VE12 a) scan mode typed in French, page in fr
      VE6  exactly one gesture pair/value        b) language switch closes an open field
                                                 c) Sync twin hidden closes an open field
    A served page WITHOUT value-entry code makes every VE row a loud FAIL
    ("NOT RUN") — a run on the wrong page cannot look green. Only
    --liveness-only leaves them out, and says so in the verdict line.

    LANES (PLAN-3.2 PD27), after value entry. Every row first proves its
    stimulus reached the page (the stub answer was pulled after the edit),
    then asserts the result; the stub hands the page its fixture objects by
    reference, so a row edits them in place and puts them back.
      LN1  lane "recording" class on the target lane only
      LN2  each #rec-status class + label key, run 0–5 and bus 0, in the
           page's precedence: bus → 2 → 3 → 1 → 5 → !signal → 4 → 0
      LN3  Clear / Save / Relink call their natives with the 1-based slot;
           Save is absent on a file: lane
      LN4  toast.loadFailed when slots[i].error turns non-empty, not again
           while it is unchanged, not for an error present at the first pull
      LN5  toast.saveFailed on `timeout`, none on `cancelled`
      LN6  fetchOverview refetches when ovRev moves and stores the ANSWER's rev
      LN7  scan-mode aria-valuetext is French after a switch to fr
      PX1  playhead            PX4  record head
      PX2  grain dot, stream 1 and stream 4 (the 0-based / 1-based class)
      PX3  feedback head       PX5  dirty tick
    A PX row compares two frames of a lane canvas read inside one animation
    frame — the feature present and the feature removed from the stub answer —
    and wants the differing pixels at the expected x within ± 2 px and none
    outside the feature's footprint (canvas backing pixels are mapped through
    canvas.width / clientWidth, so the row holds at any devicePixelRatio).

    Usage (from anywhere):
      node plugins/O-GranulatorInstrument/tests/ui_binding_probe.js                    liveness + VE + lanes on the tree's page
      node plugins/O-GranulatorInstrument/tests/ui_binding_probe.js --liveness-only    261 x 2 only
      node plugins/O-GranulatorInstrument/tests/ui_binding_probe.js --page <file>      serve <file> as index.html
           (env OGI_UI_PAGE=<file> is the same). The served ROOT is still the
           one serve-ui.js assembles — module JS, eb-garamond css + woff2, the
           stub and its preamble all resolve; only index.html is replaced.
      node plugins/O-GranulatorInstrument/tests/ui_binding_probe.js --nc [--page <file>]
           negative control: re-runs itself (liveness only) on a page copy with
           ONE bindKnob removed (s2_drive); exits 0 only if that run FAILS at
           260 / 261 both ways on exactly that id. THEN a second control
           (PD27): re-runs itself (lanes only) with the stub's grain rows sent
           0-based; passes only if that run FAILS on exactly the PX2 row.
      --mutate-bindknob <id>   the raw mutated run --nc drives (exits non-zero).
      --lanes-only             skip liveness and value entry (the verdict line says so).
      --mutate-grains-0based   the stub's grain rows carry stream 0…3 (raw run of the second control).
    Exit: 0 = all pass; n = failures (max 76); 78 = refused (no overrides
    banner / page not served as asked); 70 = crashed; 77 = Playwright missing —
    NOTHING was verified, which is NOT a pass.

  ==============================================================================
*/

'use strict';

const fs = require('fs');
const path = require('path');
const { spawnSync } = require('child_process');

const pluginRoot = path.resolve(__dirname, '..');
const repoRoot   = path.resolve(pluginRoot, '..', '..');
const PLUGIN     = 'O-GranulatorInstrument';
const S = require(path.join(repoRoot, 'scripts', 'serve-ui.js'));

const argv = process.argv.slice(2);
const argVal = (f) => { const i = argv.indexOf(f); return i >= 0 && i + 1 < argv.length ? argv[i + 1] : null; };
const PAGE = argVal('--page') || process.env.OGI_UI_PAGE || null;
const LIVENESS_ONLY = argv.includes('--liveness-only');
const MUTATE = argVal('--mutate-bindknob');
const LANES_ONLY = argv.includes('--lanes-only');
const GRAINS_0BASED = argv.includes('--mutate-grains-0based');
const PX_TOL = 2;          // px: expected x of a canvas feature (PD27)
const PX_DIFF = 6;         // 0…255: a pixel "differs" between two frames at or above this
const PX_STRONG = 90;      // 0…255: the record head against its own 8 % tint and the dimmed waveform
const NC_ID = 's2_drive';
const N_PARAMS = 261;
const KIND_COUNTS = { knob: 144, fader: 16, handle: 8, field: 8, choiceKnob: 17, segment: 15, select: 33, radio: 1, scale: 4, toggle: 15 };
const PRESET_FNS = ['savePreset', 'loadPreset', 'getPresetList', 'getCurrentPreset', 'selectNextPreset', 'selectPreviousPreset',
                    'deletePreset', 'isFactoryPreset', 'savePresetWithDialog', 'loadPresetFromFile'];

// ───────────────────────────────────────────── negative control (PD19) ──
if (argv.includes('--nc')) {
    console.log(`== ${PLUGIN} ui_binding_probe — negative control (bindKnob(${NC_ID}) removed) ==`);
    const args = [__filename, '--mutate-bindknob', NC_ID, '--liveness-only', ...(PAGE ? ['--page', PAGE] : [])];
    const r = spawnSync(process.execPath, args, { encoding: 'utf8', maxBuffer: 1 << 26 });
    if (r.status === 77) { console.log('  Playwright could not be resolved — NOTHING verified (exit 77 is not a pass).'); process.exit(77); }
    const fails = r.stdout.match(/^ {2}FAIL: .*$/gm) || [];
    const tally = /host -> UI 260 \/ 261, UI -> host 260 \/ 261/.test(r.stdout);
    const onlyThatId = fails.length === 2 && fails.every((f) => f.includes(`FAIL: ${NC_ID}:`));
    for (const f of fails.slice(0, 6)) console.log(`      ${f.trim().slice(0, 170)}`);
    const tallyLine = (r.stdout.match(/^ {3}host -> UI .*$/m) || ['   (no tally line)'])[0].trim();
    const ok = r.status === 2 && tally && onlyThatId;
    console.log(`  ${ok ? 'PASS' : 'FAIL'}: mutated page -> exit ${r.status}; ${tallyLine}; ${fails.length} FAIL line(s), ${onlyThatId ? 'all on ' + NC_ID : 'NOT confined to ' + NC_ID}`);
    if (!ok) console.log(r.stdout.split('\n').slice(-12).join('\n') + (r.stderr ? '\n' + r.stderr.slice(-600) : ''));
    console.log(ok ? 'negative control: the mutated page FAILS at 260 — the gate is live' : 'negative control: the mutated page did NOT fail as required');

    // Second control (PLAN-3.2 PD27): grain rows sent 0-based -> the PX stream row fails, and only it.
    console.log(`== ${PLUGIN} ui_binding_probe — negative control (stub grain rows sent 0-based) ==`);
    const r2 = spawnSync(process.execPath, [__filename, '--mutate-grains-0based', '--lanes-only', ...(PAGE ? ['--page', PAGE] : [])], { encoding: 'utf8', maxBuffer: 1 << 26 });
    if (r2.status === 77) { console.log('  Playwright could not be resolved — NOTHING verified (exit 77 is not a pass).'); process.exit(77); }
    const fails2 = r2.stdout.match(/^ {2}FAIL: .*$/gm) || [];
    const onlyPx2 = fails2.length === 1 && /FAIL: PX2 /.test(fails2[0]);
    const lanesLine = (r2.stdout.match(/^ {3}lanes \d+ \/ \d+ rows$/m) || ['   (no lanes tally line)'])[0].trim();
    for (const f of fails2.slice(0, 4)) console.log(`      ${f.trim().slice(0, 600)}`);
    const ok2 = r2.status === 1 && onlyPx2;
    console.log(`  ${ok2 ? 'PASS' : 'FAIL'}: 0-based grain rows -> exit ${r2.status}; ${lanesLine}; ${fails2.length} FAIL line(s), ${onlyPx2 ? 'confined to PX2' : 'NOT confined to PX2'}`);
    if (!ok2) console.log(r2.stdout.split('\n').slice(-14).join('\n') + (r2.stderr ? '\n' + r2.stderr.slice(-600) : ''));
    console.log(ok2 ? 'negative control: 0-based grain rows FAIL the stream pixel row — the gate is live' : 'negative control: 0-based grain rows did NOT fail as required');
    process.exit(ok && ok2 ? 0 : 1);
}

function readTypes() {
    const out = [];
    let cols = null;
    for (const line of fs.readFileSync(path.join(pluginRoot, '.planning', 'params.tsv'), 'utf8').split('\n')) {
        if (!line.trim()) continue;
        if (line.startsWith('#')) {
            const maybe = line.replace(/^#\s*/, '').split('\t').map((c) => c.trim());
            if (maybe.includes('id') && maybe.includes('flags')) cols = maybe;
            continue;
        }
        const c = line.split('\t');
        const flags = c[cols.indexOf('flags')];
        out.push({ id: c[cols.indexOf('id')], type: /\bboolean\b/.test(flags) ? 'bool' : /\bdiscrete\b/.test(flags) ? 'choice' : 'float' });
    }
    return out;
}

const norm = (s) => String(s == null ? '' : s).replace(/\s+/g, ' ').trim();   // \s covers the thin space
const near = (a, b, rel = 1e-6) => Number.isFinite(a) && Math.abs(a - b) <= rel * Math.max(1, Math.abs(b));

(async () => {
    const pw = S.resolvePlaywright();
    if (!pw) {
        console.log('ui_binding_probe: Playwright could not be resolved — NOTHING verified (exit 77 is not a pass).');
        process.exit(77);
    }

    const params = readTypes();
    console.log(`== ${PLUGIN} ui_binding_probe${MUTATE ? ' (MUTATED: bindKnob(' + MUTATE + ') removed)' : ''}${GRAINS_0BASED ? ' (MUTATED: stub grain rows 0-based)' : ''}${LIVENESS_ONLY ? ' (liveness only)' : ''}${LANES_ONLY ? ' (lanes only)' : ''} ==`);
    console.log(`   params.tsv: ${params.length} ids`);

    const built = S.buildRoot(PLUGIN);
    const cleanup = () => fs.rmSync(built.root, { recursive: true, force: true });
    console.log(`   stub=${built.stubKind} seed=${built.seedFrom}${built.seedError ? ' ERROR ' + built.seedError : ''}`);
    if (built.unplaced.length || built.missing.length)
        console.log(`   served tree: unplaced [${built.unplaced.join(', ')}] missing [${built.missing.join(', ')}]`);

    // REFUSE without the overrides banner: neutral 0..1 fixtures and '1'..'4'
    // choice lists would make most rows pass or fail for the wrong reason.
    const wantSeed = `param-dump (${N_PARAMS} rows) + overrides`;
    if (built.stubKind !== 'generic' || built.seedFrom !== wantSeed || built.seedError || !built.preamble) {
        console.log(`  REFUSED: the stub seed is "${built.seedFrom}"${built.seedError ? ' (' + built.seedError + ')' : ''}, expected "${wantSeed}" `
                  + '(tests/ui-stub/generic-overrides.json missing or unparsable, or params.tsv stale). NOTHING verified.');
        cleanup();
        process.exit(78);
    }

    // The page to serve: the tree's (already copied by buildRoot) or --page.
    const indexPath = path.join(built.root, 'index.html');
    const treePage = path.join(built.uiRoot, 'index.html');
    if (PAGE) {
        if (!fs.existsSync(PAGE)) { console.log(`  REFUSED: --page ${PAGE} does not exist`); cleanup(); process.exit(78); }
        // Same preamble tag, same place serve-ui.js puts it (first child of <head>).
        const src = fs.readFileSync(PAGE, 'utf8');
        const tag = '\n<script src="/js/juce/stub-preamble.js"></script>';
        const headAt = src.search(/<head\b[^>]*>/i);
        if (headAt < 0) { console.log('  REFUSED: --page has no <head>'); cleanup(); process.exit(78); }
        const at = src.indexOf('>', headAt) + 1;
        fs.writeFileSync(indexPath, src.slice(0, at) + tag + src.slice(at));
    }
    const pageFile = PAGE ? path.resolve(PAGE) : treePage;
    const sameAsTree = fs.readFileSync(pageFile).equals(fs.readFileSync(treePage));
    let served = fs.readFileSync(indexPath, 'utf8');
    const hasVE = served.includes('attachValueEntry') && served.includes('knob-value-input') && served.includes('bindLabelEntry');
    console.log(`   page: ${PAGE ? pageFile : path.relative(repoRoot, treePage)} (${fs.statSync(pageFile).size} bytes${PAGE ? (sameAsTree ? ', byte-identical to the tree page' : ', DIFFERS from the tree page') : ''}); value-entry code: ${hasVE ? 'present' : 'ABSENT'}`);

    if (MUTATE) {
        const call = 'KNOB_IDS.forEach((id) => bindKnob(Juce, id));';
        if (!served.includes(call)) { console.log('  REFUSED: the mutation did not apply'); cleanup(); process.exit(78); }
        served = served.replace(call, `KNOB_IDS.filter((id) => id !== '${MUTATE}').forEach((id) => bindKnob(Juce, id));`);
        fs.writeFileSync(indexPath, served);
    }

    let failures = 0;
    const fail = (msg) => { ++failures; console.log(`  FAIL: ${msg}`); };
    const pass = (msg) => console.log(`  PASS: ${msg}`);

    const misses = [];
    const srv = await S.serve(built.root, (rel) => misses.push(rel));
    const browser = await pw.chromium.launch();
    const page = await browser.newPage({ viewport: { width: 1200, height: 800 } });
    const errors = [];
    page.on('pageerror', (e) => errors.push(`pageerror: ${e.message}`));
    page.on('console', (m) => { if (m.type() === 'error') errors.push(`console.error: ${m.text()}`); });
    const finish = async (code) => { await browser.close(); await srv.close(); cleanup(); process.exit(code); };

    // Every label the page puts in #toast, from before its first script runs
    // (LN4 needs the boot: an error already there at the first pull is not news).
    await page.addInitScript(() => {
        window.__toastLog = [];
        new MutationObserver((recs) => {
            for (const r of recs) {
                if (r.target.id !== 'toast') continue;
                window.__toastLog.push({ key: r.target.getAttribute('data-i18n'), vars: r.target.getAttribute('data-i18n-vars'), text: r.target.textContent });
            }
        }).observe(document, { subtree: true, attributes: true, attributeFilter: ['data-i18n'] });
    });
    await page.goto(`http://127.0.0.1:${srv.port}/`);
    // Quiesce: init() ran (readouts written), the first telemetry tick landed,
    // slot state + a lane overview were pulled, the preset bar initialised.
    try {
        await page.waitForFunction((presetFns) => {
            const calls = window.__stubNativeCalls;
            if (!window.__stubStates || !Array.isArray(calls)) return false;
            const called = (n) => calls.some((c) => c.name === n);
            const v = document.getElementById('val-polyphony');
            return called('getTelemetry') && called('getSlotState') && called('getSlotOverview') && called('getScaleState')
                && presetFns.every((n) => window.__stubReport.nativeKnown.includes(n))
                && v && v.textContent !== '—';
        }, PRESET_FNS, { timeout: 15000 });
    } catch (e) {
        const st = await page.evaluate(() => ({
            calls: [...new Set((window.__stubNativeCalls || []).map((c) => c.name))],
            known: window.__stubReport ? window.__stubReport.nativeKnown : null,
        })).catch(() => null);
        fail(`the page did not boot to quiescence in 15 s (natives called: ${st ? st.calls.join(', ') : '?'}); errors: ${errors.slice(0, 3).join(' | ')}`);
        console.log(`${failures} FAILED`);
        await finish(failures);
    }
    await page.waitForTimeout(400);

    // ── boot facts ──────────────────────────────────────────────────────────
    const boot = await page.evaluate((presetFns) => ({
        banner: !!window.__stubOverrides && window.__stubOverrides.__from,
        combos: window.__stubOverrides ? Object.keys(window.__stubOverrides.combos || {}).length : 0,
        natives: window.__stubOverrides ? Object.keys(window.__stubOverrides.natives || {}) : [],
        unknown: window.__stubReport.nativeUnknown.slice(),
        presetKnown: presetFns.filter((n) => window.__stubReport.nativeKnown.includes(n)),
        presetAnswered: presetFns.filter((n) => window.__stubNativeCalls.some((c) => c.name === n)),
        presetText: (document.getElementById('preset-name-text') || {}).textContent,
        states: { s: window.__stubStates.sliders.size, c: window.__stubStates.combos.size, t: window.__stubStates.toggles.size },
        font: document.fonts.check('11px "EB Garamond"'),
    }), PRESET_FNS);
    if (boot.banner !== wantSeed || boot.combos !== 70) fail(`in-page overrides banner is "${boot.banner}" with ${boot.combos} combos (expected "${wantSeed}", 70)`);
    else pass(`overrides banner in the page: "${boot.banner}" (70 combos, natives: ${boot.natives.join(', ')})`);
    if (boot.presetKnown.length === 10 && boot.unknown.filter((n) => PRESET_FNS.includes(n)).length === 0 && norm(boot.presetText) === 'Default')
        pass(`the stub answers the ten preset functions (preset bar shows "${norm(boot.presetText)}"; called at boot: ${boot.presetAnswered.join(', ')})`);
    else fail(`preset functions: ${boot.presetKnown.length} / 10 resolved by the stub, preset bar "${norm(boot.presetText)}", unknown [${boot.unknown.join(', ')}]`);
    console.log(`   states requested at boot: sliders ${boot.states.s}, combos ${boot.states.c}, toggles ${boot.states.t}; `
              + `natives the stub had to guess: [${boot.unknown.join(', ')}]; EB Garamond loaded: ${boot.font}; 404s: [${misses.join(', ')}]`);
    if (misses.length) fail(`the served tree 404'd: ${misses.join(', ')}`);
    // LN4 boot fact, taken before anything edits the stub.
    const bootToast = await page.evaluate(() => ({
        log: window.__toastLog.map((t) => t.key),
        pulls: window.__stubNativeCalls.filter((c) => c.name === 'getSlotState').length,
        errs: window.__stubOverrides.natives.getSlotState.slots.map((s) => String((s && s.error) || '')),
    }));

    // In-page helpers (installed once).
    await page.evaluate(() => {
        const S = window.__stubStates;
        const H = window.__probe = {};
        const has = (x) => !!document.getElementById(x);
        H.kind = (id, type) => {
            if (type === 'bool') return has(`toggle-${id}`) ? 'toggle' : null;
            if (type === 'float') return has(`fader-${id}`) ? 'fader' : has(`handle-${id}`) ? 'handle' : has(`field-${id}`) ? 'field' : has(`knob-${id}`) ? 'knob' : null;
            if (/^s\d_scale$/.test(id)) return has(`scale-trig-${id.slice(0, 2)}`) ? 'scale' : null;
            if (has(`seg-${id}`)) return 'segment';
            if (has(`combo-${id}`)) return 'select';
            if (has(`radio-${id}`)) return 'radio';
            if (has(`knob-${id}`)) return 'choiceKnob';
            return null;
        };
        H.state = (id, type) => (type === 'float' ? S.sliders : type === 'bool' ? S.toggles : S.combos).get(id) || null;
        H.sel = (id, kind) => (kind === 'toggle' ? `toggle-${id}` : kind === 'fader' ? `fader-${id}` : kind === 'handle' ? `handle-${id}`
            : kind === 'field' ? `field-${id}` : kind === 'segment' ? `seg-${id}` : kind === 'select' ? `combo-${id}`
            : kind === 'radio' ? `radio-${id}` : kind === 'scale' ? `scale-trig-${id.slice(0, 2)}` : `knob-${id}`);
        H.el = (id, kind) => document.getElementById(H.sel(id, kind));
        H.norm = (st) => st.getNormalisedValue();
        H.toScaled = (st, n) => { const p = st.properties; return Math.pow(n, 1 / p.skew) * (p.end - p.start) + p.start; };
        // Relay emulation: the value changes behind the page, then valueChangedEvent fires.
        H.push = (st, type, v) => {
            if (type === 'float') st.scaledValue = v;
            else if (type === 'bool') st.value = !!v;
            else st.choiceIndex = v;
            st.valueChangedEvent.callListeners();
        };
        H.value = (st, type) => (type === 'float' ? st.getScaledValue() : type === 'bool' ? st.getValue() : st.getChoiceIndex());
        H.text = (id) => { const e = document.getElementById(`val-${id}`); return e ? e.textContent : null; };
        H.stem = (id) => { const k = document.getElementById(`knob-${id}`); const s = k && k.querySelector('.knob-stem'); return s ? s.style.transform : null; };
        H.pageOf = (el) => { const p = el && el.closest('.page'); return p ? p.id.replace('page-', '') : null; };
        H.syncOf = (el) => { const s = el && el.closest('[data-sync]'); return s ? s.dataset.sync : null; };
        // Every write the page makes to a state, in order: S = sliderDragStarted,
        // V = setNormalisedValue, E = sliderDragEnded, C = setChoiceIndex, T = setValue.
        H.track = (st) => {
            const proto = Object.getPrototypeOf(st);
            st.__log = [];
            if (proto.sliderDragStarted) {
                st.sliderDragStarted = () => { st.__log.push('S'); };
                st.sliderDragEnded = () => { st.__log.push('E'); };
                st.setNormalisedValue = (n) => { st.__log.push('V'); return proto.setNormalisedValue.call(st, n); };
            } else if (proto.setChoiceIndex) {
                st.setChoiceIndex = (i) => { st.__log.push('C'); return proto.setChoiceIndex.call(st, i); };
            } else {
                st.setValue = (v) => { st.__log.push('T'); return proto.setValue.call(st, v); };
            }
        };
        H.log = (st) => (st.__log ? st.__log.join('') : null);
        H.untrack = (st) => {
            for (const k of ['__log', 'sliderDragStarted', 'sliderDragEnded', 'setNormalisedValue', 'setChoiceIndex', 'setValue'])
                if (Object.prototype.hasOwnProperty.call(st, k)) delete st[k];
        };
        // Value-entry snapshot of one cell.
        H.ve = (id, type) => {
            const st = H.state(id, type), v = document.getElementById(`val-${id}`), c = document.getElementById(`cell-${id}`);
            const input = v ? v.querySelector('input.knob-value-input') : null;
            const own = v ? [...v.childNodes].filter((n) => n.nodeType === 3).map((n) => n.textContent).join('') : null;
            return {
                value: st ? H.value(st, type) : null, log: st ? H.log(st) : null,
                open: !!input && v.dataset.editing === '1' && c.classList.contains('editing'),
                focused: !!input && document.activeElement === input,
                inputText: input ? input.value : null,
                anyInput: document.querySelectorAll('.knob-value-input').length,
                anyEditing: document.querySelectorAll('[data-editing]').length + document.querySelectorAll('.knob-cell.editing').length,
                readout: own, aria: (document.getElementById(`knob-${id}`) || { getAttribute: () => null }).getAttribute('aria-valuetext'),
            };
        };
    });

    let currentTab = 'streams';
    const gotoTab = async (key) => { if (key && key !== currentTab) { await page.click(`#tab-${key}`); currentTab = key; } };

    // Host-side slot change: the stub hands the page the fixture objects by
    // reference, so editing them in place and bumping telemetry.slotRev is a
    // new getSlotState answer the page pulls on its next telemetry tick.
    const setLaneLoaded = async (n, on) => {
        await page.evaluate(({ n, on }) => {
            const nat = window.__stubOverrides.natives, slots = nat.getSlotState.slots;
            window.__probeSlots = window.__probeSlots || {};
            if (on) {
                window.__probeSlots[n] = slots[n - 1];
                const loaded = slots.find((s) => s && s.scheme === 'file' && !s.missing);
                slots[n - 1] = Object.assign({}, loaded, { name: `probe-slot-${n}.wav` });
            } else {
                slots[n - 1] = window.__probeSlots[n];
            }
            nat.getSlotState.revision += 1;
            nat.getTelemetry.slotRev += 1;
        }, { n, on });
        await page.waitForFunction(({ n, on }) =>
            /\b(empty|missing)\b/.test(document.getElementById(`lane-${n}`).className) !== on, { n, on }, { timeout: 5000 });
    };

    // ════════════════════════════════════════════════════════════ liveness ══
    let hostOk = 0, uiOk = 0;
    const kinds = {};

    for (const { id, type } of (LANES_ONLY ? [] : params)) {
        const info = await page.evaluate(({ id, type }) => {
            const H = window.__probe;
            const kind = H.kind(id, type);
            const st = H.state(id, type);
            const el = kind ? H.el(id, kind) : null;
            return { kind, bound: !!st, page: H.pageOf(el), sync: H.syncOf(el),
                     count: st && st.properties && st.properties.choices ? st.properties.choices.length : 0 };
        }, { id, type });
        kinds[info.kind || 'none'] = (kinds[info.kind || 'none'] || 0) + 1;
        if (!info.kind) { fail(`${id}: no control element for a ${type} — host->UI`); fail(`${id}: no control element — UI->host`); continue; }
        if (!info.bound) { fail(`${id}: the page never asked for its ${type} state (not bound) — host->UI`); fail(`${id}: not bound — UI->host`); continue; }

        // ── host -> UI ──────────────────────────────────────────────────────
        const h = await page.evaluate(({ id, type, kind }) => {
            const H = window.__probe;
            const st = H.state(id, type);
            const v0 = H.value(st, type);
            const el = H.el(id, kind);
            let ok = false, why = '';
            if (kind === 'knob' || kind === 'fader' || kind === 'handle' || kind === 'field') {
                const n0 = H.norm(st);
                const n1 = n0 < 0.5 ? 0.83 : 0.17;
                const read = () => (kind === 'knob' ? { t: H.text(id), x: H.stem(id), a: el.getAttribute('aria-valuetext') }
                    : kind === 'fader' ? { t: H.text(id), x: el.value, a: el.getAttribute('aria-valuetext') }
                    : kind === 'handle' ? { t: el.getAttribute('aria-valuetext'), x: el.style.left, a: el.getAttribute('aria-valuetext') }
                    : { t: el.textContent, x: el.textContent, a: el.getAttribute('aria-valuetext') });
                const b = read();
                H.push(st, type, H.toScaled(st, n1));
                const a = read();
                ok = a.t !== b.t && a.x !== b.x && a.a === a.t && !!a.t;
                if (kind === 'fader') ok = ok && Math.abs(Number(el.value) - n1) < 1e-6;
                why = `text ${b.t} -> ${a.t}, ${kind === 'knob' ? 'stem' : kind === 'handle' ? 'left' : 'x'} ${b.x} -> ${a.x}, aria ${a.a}`;
                H.push(st, type, v0);
                const r = read();
                if (r.t !== b.t || r.x !== b.x) { ok = false; why += ` (restore gave ${r.t} / ${r.x})`; }
            } else if (kind === 'toggle') {
                const a0 = el.getAttribute('aria-pressed');
                H.push(st, type, !v0);
                const a1 = el.getAttribute('aria-pressed');
                H.push(st, type, v0);
                const a2 = el.getAttribute('aria-pressed');
                ok = a0 === String(!!v0) && a1 === String(!v0) && a2 === String(!!v0);
                why = `aria-pressed ${a0} -> ${a1} -> ${a2}`;
            } else {
                const choices = (st.properties && st.properties.choices) || [];
                const i1 = (v0 + 1) % Math.max(1, choices.length);
                const pressed = (attr) => [...el.querySelectorAll('button[data-index]')].filter((b) => b.getAttribute(attr) === 'true').map((b) => b.dataset.index);
                if (kind === 'choiceKnob') {
                    const t0 = H.text(id), s0 = H.stem(id);
                    H.push(st, type, i1);
                    const t1 = H.text(id), face = !!document.getElementById(`val-${id}`).dataset.i18n;
                    ok = t1 !== t0 && H.stem(id) !== s0 && el.getAttribute('aria-valuenow') === String(i1) && el.getAttribute('aria-valuetext') === t1
                      && (face || t1 === String(choices[i1]));
                    why = `val ${t0} -> ${t1} (choice ${choices[i1]}${face ? ', localized face' : ''}), aria-valuenow ${el.getAttribute('aria-valuenow')} vs ${i1}`;
                } else if (kind === 'segment') {
                    const p0 = pressed('aria-pressed');
                    H.push(st, type, i1);
                    const p1 = pressed('aria-pressed');
                    ok = p0.length === 1 && p0[0] === String(v0) && p1.length === 1 && p1[0] === String(i1);
                    why = `pressed [${p0}] -> [${p1}] vs ${v0} -> ${i1}`;
                } else if (kind === 'radio') {
                    const p0 = pressed('aria-checked');
                    H.push(st, type, i1);
                    const p1 = pressed('aria-checked');
                    const on = [...el.querySelectorAll('button.on')].map((b) => b.dataset.index);
                    ok = p0.length === 1 && p0[0] === String(v0) && p1.length === 1 && p1[0] === String(i1) && on.length === 1 && on[0] === String(i1);
                    why = `checked [${p0}] -> [${p1}] vs ${v0} -> ${i1}, .on [${on}]`;
                } else if (kind === 'select') {
                    const s0 = el.selectedIndex;
                    H.push(st, type, i1);
                    ok = s0 === v0 && el.selectedIndex === i1 && el.options.length === choices.length && choices.length > 1;
                    why = `selectedIndex ${s0} -> ${el.selectedIndex} vs ${v0} -> ${i1} (${el.options.length} options / ${choices.length} choices)`;
                } else if (kind === 'scale') {
                    const name = () => document.getElementById(`scale-name-${id.slice(0, 2)}`).textContent;
                    const before = name();
                    H.push(st, type, i1);
                    const after = name();
                    H.push(st, type, v0);
                    const back = name();
                    ok = after !== before && back === before && !!after;
                    why = `label ${before} -> ${after} -> ${back}`;
                }
                H.push(st, type, v0);
            }
            return { ok, why };
        }, { id, type, kind: info.kind });
        if (h.ok) ++hostOk; else fail(`${id}: host->UI (${info.kind}): ${h.why}`);

        // ── UI -> host ──────────────────────────────────────────────────────
        await gotoTab(info.page);
        const prevSync = await page.evaluate((want) => {
            const st = window.__stubStates.combos.get('sync_mode');
            const prev = st.getChoiceIndex();
            if (want) { const idx = want === 'tempo' ? 1 : 0; if (prev !== idx) st.setChoiceIndex(idx); }
            return prev;
        }, info.sync);

        // A lane handle is display:none while its lane is empty or missing (by
        // design). The fixture has slot 3 missing and slot 4 empty, so load a
        // file there the way the host would — new slot state behind a slotRev
        // bump the page re-pulls on — and put the fixture back afterwards.
        let laneSwap = null;
        if (info.kind === 'handle') {
            const n = Number(id[1]);
            const shown = await page.evaluate((n) => !/\b(empty|missing)\b/.test(document.getElementById(`lane-${n}`).className), n);
            if (!shown) { laneSwap = n; await setLaneLoaded(n, true); }
        }

        const continuous = ['knob', 'fader', 'handle', 'field'].includes(info.kind);
        const setup = await page.evaluate(({ id, type, kind, continuous }) => {
            const H = window.__probe;
            const st = H.state(id, type);
            const v0 = H.value(st, type);
            if (continuous) H.push(st, type, H.toScaled(st, 0.5));
            if (kind === 'choiceKnob') H.push(st, type, 0);
            H.track(st);
            return { v0, before: H.value(st, type) };
        }, { id, type, kind: info.kind, continuous });

        let acted = true, actErr = '';
        try {
            const sel = '#' + await page.evaluate(({ id, kind }) => window.__probe.sel(id, kind), { id, kind: info.kind });
            if (info.kind === 'knob' || info.kind === 'choiceKnob' || info.kind === 'field') { await page.focus(sel); await page.keyboard.press('ArrowUp'); }
            else if (info.kind === 'fader' || info.kind === 'handle') { await page.focus(sel); await page.keyboard.press('ArrowRight'); }
            else if (info.kind === 'toggle') await page.click(sel, { timeout: 2000 });
            else if (info.kind === 'segment' || info.kind === 'radio') {
                const target = (setup.before + 1) % Math.max(1, info.count);
                await page.click(`${sel} button[data-index="${target}"]`, { timeout: 2000 });
            } else if (info.kind === 'select') {
                await page.selectOption(sel, String((setup.before + 1) % Math.max(1, info.count)), { timeout: 2000 });
            } else if (info.kind === 'scale') {
                await page.click(sel, { timeout: 2000 });
                const target = setup.before === 5 ? 6 : 5;
                await page.click(`#scale-list .it[data-idx="${target}"]`, { timeout: 2000 });
            }
        } catch (e) { acted = false; actErr = e.message.split('\n')[0]; }

        const u = await page.evaluate(({ id, type, setup, continuous }) => {
            const H = window.__probe;
            const st = H.state(id, type);
            const after = H.value(st, type);
            const log = H.log(st);
            let ok = after !== setup.before;
            let why = `value ${setup.before} -> ${after}, calls "${log}"`;
            if (continuous) ok = ok && log === 'SVE';                 // one gesture pair around one write
            else ok = ok && (log === 'C' || log === 'T');             // exactly one write
            H.untrack(st);
            H.push(st, type, setup.v0);
            return { ok, why };
        }, { id, type, setup, continuous });
        await page.evaluate((prev) => {
            const st = window.__stubStates.combos.get('sync_mode');
            if (st.getChoiceIndex() !== prev) st.setChoiceIndex(prev);
        }, prevSync);
        await page.keyboard.press('Escape').catch(() => {});
        if (laneSwap) await setLaneLoaded(laneSwap, false);
        if (acted && u.ok) ++uiOk; else fail(`${id}: UI->host (${info.kind}): ${acted ? u.why : 'action failed: ' + actErr}`);
    }

    const liveFailuresBefore = failures;
    if (LANES_ONLY) {
        console.log('\n   liveness: NOT RUN (--lanes-only)');
    } else {
        console.log(`   binders: ${Object.keys(KIND_COUNTS).map((k) => `${k} ${kinds[k] || 0}`).join(', ')}${kinds.none ? ', NONE ' + kinds.none : ''}`
                  + ` = ${Object.values(kinds).reduce((a, b) => a + b, 0)}`);
        for (const [k, n] of Object.entries(KIND_COUNTS))
            if ((kinds[k] || 0) !== n) fail(`binder census: ${k} ${kinds[k] || 0}, expected ${n}`);
        console.log(`\n   host -> UI ${hostOk} / ${params.length}, UI -> host ${uiOk} / ${params.length}`);
        if (params.length !== N_PARAMS) fail(`params.tsv has ${params.length} ids, expected ${N_PARAMS}`);
        if ((hostOk !== N_PARAMS || uiOk !== N_PARAMS) && failures === 0) fail(`liveness: not ${N_PARAMS} / ${N_PARAMS} both ways`);
    }

    // ════════════════════════════════════════════════════════ value entry ══
    const VE_NAMES = ['VE1 Enter applies', 'VE2 Esc cancels', 'VE3 outside click applies',
        'VE4 outside press on a choice knob (preventDefault) applies', 'VE5 unchanged text writes nothing',
        'VE6 exactly one gesture pair per value', 'VE7 clamp above end', 'VE8 -inf on a gain', 'VE9 fCount',
        'VE10 fBend with and without ± / st', 'VE11 a tempo division',
        'VE12a scan mode typed in French, page in fr', 'VE12b language switch closes an open field', 'VE12c Sync twin hidden closes an open field'];
    let veRun = 0, vePass = 0;

    if (LIVENESS_ONLY || LANES_ONLY) {
        console.log(`\n   value entry: NOT RUN (${LANES_ONLY ? '--lanes-only' : '--liveness-only'})`);
    } else if (!hasVE) {
        console.log('\n-- value entry (VE1–VE12)');
        for (const n of VE_NAMES) fail(`${n} — NOT RUN: the served page has no value-entry code (attachValueEntry / bindLabelEntry / .knob-value-input)`);
    } else {
        console.log('\n-- value entry (VE1–VE12)');
        const ov = JSON.parse(fs.readFileSync(path.join(pluginRoot, 'tests', 'ui-stub', 'generic-overrides.json'), 'utf8'));
        const rng = (id) => ov.sliders[id];
        const choices = (id) => ov.combos[id].choices;

        const snap = (id, type = 'float') => page.evaluate(({ id, type }) => window.__probe.ve(id, type), { id, type });
        const prep = (id, type, value) => page.evaluate(({ id, type, value }) => {
            const H = window.__probe, st = H.state(id, type);
            const v0 = H.value(st, type);
            H.untrack(st);
            if (value !== null) H.push(st, type, value);
            H.track(st);
            return v0;
        }, { id, type, value });
        const restore = (id, type, v0) => page.evaluate(({ id, type, v0 }) => {
            const H = window.__probe, st = H.state(id, type);
            H.untrack(st); H.push(st, type, v0);
        }, { id, type, v0 });
        const tabOf = (id) => page.evaluate((id) => window.__probe.pageOf(document.getElementById(`cell-${id}`)), id);
        const open = async (id, type = 'float') => {
            await gotoTab(await tabOf(id));
            await page.click(`#cell-${id} .knob-label`, { timeout: 2000 });
            const s = await snap(id, type);
            if (!s.open || !s.focused) throw new Error(`clicking the ${id} label did not open a focused field (open ${s.open}, focused ${s.focused}, inputs ${s.anyInput})`);
            return s;
        };
        // The field opens with its text selected, so typing replaces it.
        const type_ = (text) => page.keyboard.type(text);
        const closedClean = (s) => !s.open && s.anyInput === 0 && s.anyEditing === 0;
        const outsideClick = async () => { const b = await page.locator('#title').boundingBox(); await page.mouse.click(b.x + b.width / 2, b.y + b.height / 2); };
        const setSyncByUi = async (idx) => { await page.click(`#seg-sync_mode button[data-index="${idx}"]`, { timeout: 2000 }); };
        const setLangByUi = async (code) => {
            await page.click('#gear-btn', { timeout: 2000 });
            await page.selectOption('#lang-select', code, { timeout: 2000 });
            await page.keyboard.press('Escape');
            const lang = await page.evaluate(() => document.documentElement.lang);
            if (lang !== code) throw new Error(`language select did not switch the page to ${code} (html lang = ${lang})`);
        };

        const row = async (name, fn) => {
            ++veRun;
            const notes = [];
            const need = (cond, msg) => { if (!cond) notes.push(msg); };
            try { await fn(need); } catch (e) { notes.push(`threw: ${e.message.split('\n')[0]}`); }
            // Leave nothing open for the next row.
            await page.keyboard.press('Escape').catch(() => {});
            const left = await page.evaluate(() => document.querySelectorAll('.knob-value-input').length);
            if (left) { notes.push(`${left} field(s) still open after the row`); await page.evaluate(() => document.querySelectorAll('.knob-value-input').forEach((i) => i.blur())); }
            if (notes.length === 0) { ++vePass; pass(name); } else fail(`${name} — ${notes.join('; ')}`);
        };

        // Float row: type `text` into `id` (baseline `base`), finish with `how`,
        // expect scaled value `want` (null = unchanged), readout `readout`, calls `log`.
        const floatRow = async (need, { id, base, text, how, want, readout, log }) => {
            const v0 = await prep(id, 'float', base);
            try {
                const o = await open(id);
                need(norm(o.inputText) === norm(o.readout), `${id}: the field opened with "${o.inputText}", the readout says "${o.readout}"`);
                if (text !== null) await type_(text);
                if (how === 'enter') await page.keyboard.press('Enter');
                else if (how === 'escape') await page.keyboard.press('Escape');
                else await how();
                const s = await snap(id);
                need(closedClean(s), `${id}: the field did not close cleanly (open ${s.open}, inputs ${s.anyInput}, editing marks ${s.anyEditing})`);
                if (want === null) need(s.value === base, `${id}: typed "${text}" -> value ${s.value}, expected UNCHANGED ${base}`);
                else need(near(s.value, want), `${id}: typed "${text}" -> value ${s.value}, expected ${want}`);
                need(s.log === log, `${id}: typed "${text}" -> state calls "${s.log}", expected "${log}"`);
                if (readout !== undefined) need(norm(s.readout) === norm(readout), `${id}: typed "${text}" -> readout "${norm(s.readout)}", expected "${norm(readout)}"`);
                need(norm(s.aria) === norm(s.readout), `${id}: aria-valuetext "${norm(s.aria)}" != readout "${norm(s.readout)}"`);
                return s;
            } finally { await restore(id, 'float', v0); }
        };

        await row(VE_NAMES[0], async (need) => {
            await floatRow(need, { id: 's1_filter_cutoff', base: 5000, text: '1.2k', how: 'enter', want: 1200, readout: '1.20 kHz', log: 'SVE' });
        });

        await row(VE_NAMES[1], async (need) => {
            const before = await snap('s1_filter_cutoff');
            const s = await floatRow(need, { id: 's1_filter_cutoff', base: 5000, text: '800', how: 'escape', want: null, readout: '5.00 kHz', log: '' });
            need(before.anyInput === 0 && s.anyInput === 0, 'a field was open before / after the row');
        });

        await row(VE_NAMES[2], async (need) => {
            await floatRow(need, { id: 'glide', base: 100, text: '250', how: outsideClick, want: 250, readout: '250 ms', log: 'SVE' });
        });

        await row(VE_NAMES[3], async (need) => {
            const choiceId = 's1_scan_mode';
            const [tA, tB] = [await tabOf('glide'), await tabOf(choiceId)];
            if (tA !== tB) throw new Error(`glide (${tA}) and ${choiceId} (${tB}) are not on the same page`);
            const c0 = await prep(choiceId, 'choice', null);
            await page.evaluate(() => {
                window.__probeDown = [];
                window.__probeDownFn = (e) => window.__probeDown.push({ id: e.target.id, prevented: e.defaultPrevented });
                document.addEventListener('pointerdown', window.__probeDownFn);        // bubble phase: after the knob's own handler
            });
            try {
                const press = async () => {
                    const b = await page.locator(`#knob-${choiceId}`).boundingBox();
                    await page.mouse.move(b.x + b.width / 2, b.y + b.height / 2);
                    await page.evaluate(() => { window.__probeDown = []; });     // drop the label click that opened the field
                    await page.mouse.down();
                    await page.mouse.up();
                };
                await floatRow(need, { id: 'glide', base: 100, text: '400', how: press, want: 400, readout: '400 ms', log: 'SVE' });
                const d = await page.evaluate(() => window.__probeDown);
                need(d.length === 1 && d[0].id === `knob-${choiceId}` && d[0].prevented === true,
                    `the press was not a preventDefault'ed pointerdown on knob-${choiceId} (${JSON.stringify(d)})`);
                const c = await snap(choiceId, 'choice');
                need(c.value === c0 && c.log === '', `${choiceId} moved under the press (index ${c0} -> ${c.value}, calls "${c.log}")`);
            } finally {
                await page.evaluate(() => { document.removeEventListener('pointerdown', window.__probeDownFn); });
                await restore(choiceId, 'choice', c0);
            }
        });

        await row(VE_NAMES[4], async (need) => {
            // 123.456 reads "123 ms": re-writing the readout would quantise the value.
            await floatRow(need, { id: 's1_size', base: 123.456, text: null, how: 'enter', want: null, readout: '123 ms', log: '' });
            await floatRow(need, { id: 's1_size', base: 123.456, text: null, how: outsideClick, want: null, readout: '123 ms', log: '' });
            await floatRow(need, { id: 's1_size', base: 123.456, text: '123 ms', how: 'enter', want: null, readout: '123 ms', log: '' });
        });

        await row(VE_NAMES[5], async (need) => {
            const id = 's1_pitch', r = rng(id);
            const a = Math.min(r.end, 7), b = Math.max(r.start, -3.5);
            const v0 = await prep(id, 'float', 0);
            try {
                await open(id); await type_(String(a)); await page.keyboard.press('Enter');
                const s1 = await snap(id);
                need(near(s1.value, a) && s1.log === 'SVE', `${id}: "${a}" + Enter -> value ${s1.value}, calls "${s1.log}" (expected ${a}, "SVE")`);
                await open(id); await type_(String(b)); await outsideClick();
                const s2 = await snap(id);
                need(near(s2.value, b) && s2.log === 'SVESVE', `${id}: "${b}" + outside click -> value ${s2.value}, calls "${s2.log}" (expected ${b}, "SVESVE")`);
                // A second blur / a stray Enter after the close must not write again.
                await page.keyboard.press('Enter'); await outsideClick();
                const s3 = await snap(id);
                need(s3.log === 'SVESVE' && closedClean(s3), `${id}: calls after the field closed: "${s3.log}"`);
            } finally { await restore(id, 'float', v0); }
        });

        await row(VE_NAMES[6], async (need) => {
            const r = rng('s1_size');
            await floatRow(need, { id: 's1_size', base: 100, text: '99999', how: 'enter', want: r.end, readout: '2.00 s', log: 'SVE' });
            await floatRow(need, { id: 's1_size', base: 100, text: '0.001', how: 'enter', want: r.start, readout: '5.0 ms', log: 'SVE' });
        });

        await row(VE_NAMES[7], async (need) => {
            const r = rng('s1_level');
            await floatRow(need, { id: 's1_level', base: 0, text: '-inf', how: 'enter', want: r.start, readout: '−inf dB', log: 'SVE' });
            await floatRow(need, { id: 's1_level', base: 0, text: '−inf dB', how: 'enter', want: r.start, readout: '−inf dB', log: 'SVE' });
            await floatRow(need, { id: 's1_level', base: 0, text: '-12', how: 'enter', want: -12, readout: '−12.0 dB', log: 'SVE' });
        });

        await row(VE_NAMES[8], async (need) => {
            await floatRow(need, { id: 'polyphony', base: 8, text: '12', how: 'enter', want: 12, readout: '12', log: 'SVE' });
            await floatRow(need, { id: 'polyphony', base: 8, text: '5.6', how: 'enter', want: 6, readout: '6', log: 'SVE' });
            await floatRow(need, { id: 'polyphony', base: 8, text: '99', how: 'enter', want: rng('polyphony').end, readout: '16', log: 'SVE' });
            await floatRow(need, { id: 'polyphony', base: 8, text: 'abc', how: 'enter', want: null, readout: '8', log: '' });
        });

        await row(VE_NAMES[9], async (need) => {
            const id = 'pitch_bend_range';
            await floatRow(need, { id, base: 2, text: '±7 st', how: 'enter', want: 7, readout: '±7 st', log: 'SVE' });
            await floatRow(need, { id, base: 2, text: '12', how: 'enter', want: 12, readout: '±12 st', log: 'SVE' });
            await floatRow(need, { id, base: 2, text: '9 st', how: 'enter', want: 9, readout: '±9 st', log: 'SVE' });
            await floatRow(need, { id, base: 2, text: '±5', how: 'enter', want: 5, readout: '±5 st', log: 'SVE' });
        });

        // Choice row: type `text`, expect index `want` (null = unchanged) and readout.
        const choiceRow = async (need, { id, base, text, how = 'enter', want, readout }) => {
            const v0 = await prep(id, 'choice', base);
            try {
                const o = await open(id, 'choice');
                need(norm(o.inputText) === norm(o.readout), `${id}: the field opened with "${o.inputText}", the readout says "${o.readout}"`);
                await type_(text);
                if (how === 'enter') await page.keyboard.press('Enter'); else await how();
                const s = await snap(id, 'choice');
                need(closedClean(s), `${id}: the field did not close cleanly (open ${s.open}, inputs ${s.anyInput}, editing marks ${s.anyEditing})`);
                need(s.value === (want === null ? base : want), `${id}: typed "${text}" -> index ${s.value}, expected ${want === null ? 'UNCHANGED ' + base : want}`);
                need(s.log === (want === null || want === base ? '' : 'C'), `${id}: typed "${text}" -> state calls "${s.log}"`);
                if (readout !== undefined) need(norm(s.readout) === norm(readout), `${id}: typed "${text}" -> readout "${norm(s.readout)}", expected "${norm(readout)}"`);
                need(norm(s.aria) === norm(s.readout), `${id}: aria-valuetext "${norm(s.aria)}" != readout "${norm(s.readout)}"`);
                return s;
            } finally { await restore(id, 'choice', v0); }
        };

        await row(VE_NAMES[10], async (need) => {
            const id = 's1_size_sync', ch = choices(id);
            const i8 = ch.indexOf('1/8'), i16t = ch.indexOf('1/16 T'), ibar = ch.indexOf('2 bars'), base = ch.indexOf('1/16');
            if ([i8, i16t, ibar, base].some((i) => i < 0)) throw new Error(`fixture choices lack a division: ${ch.join(' | ')}`);
            await gotoTab(await tabOf(id));
            await setSyncByUi(1);
            try {
                const vis = await page.evaluate((id) => !document.getElementById(`cell-${id}`).hidden, id);
                need(vis, 'the Tempo cell is not shown after clicking Sync = Tempo');
                await choiceRow(need, { id, base, text: '1/8', want: i8, readout: '1/8' });
                await choiceRow(need, { id, base, text: '16t', want: i16t, readout: '1/16 T' });
                await choiceRow(need, { id, base, text: '2 bars', how: outsideClick, want: ibar, readout: '2 bars' });
                await choiceRow(need, { id, base, text: '1/7', want: null, readout: '1/16' });
            } finally { await setSyncByUi(0); }
        });

        // The French faces come from the served i18n table, not from this file.
        const i18n = await page.evaluate(async () => {
            const m = await import('/js/i18n.js');
            const t = (k, l) => m.LABELS[k] && m.LABELS[k][l] ? m.LABELS[k][l].t : null;
            return ['opt.scan.free', 'opt.scan.retrigger', 'opt.scan.pingpong'].map((k) => ({ en: t(k, 'en'), fr: t(k, 'fr') }));
        });

        await row(VE_NAMES[11], async (need) => {
            const id = 's1_scan_mode', ch = choices(id);
            const k = i18n.findIndex((f, i) => i > 0 && f.fr && f.fr !== f.en && f.fr.toLowerCase() !== String(ch[i]).toLowerCase());
            if (k < 0 || !i18n[0].fr) throw new Error(`no scan-mode face differs in French: ${JSON.stringify(i18n)}`);
            await setLangByUi('fr');
            try {
                const first = await choiceRow(need, { id, base: 0, text: i18n[k].fr, want: k, readout: i18n[k].fr });
                need(norm(first.readout) !== norm(ch[k]), `the readout is the English choice "${ch[k]}" — the page is not in French`);
                await choiceRow(need, { id, base: 0, text: `  ${i18n[k].fr.toUpperCase()} `, want: k, readout: i18n[k].fr });
                await choiceRow(need, { id, base: k, text: i18n[0].fr, how: outsideClick, want: 0, readout: i18n[0].fr });
                await choiceRow(need, { id, base: 0, text: ch[k], want: k, readout: i18n[k].fr });          // the English choice string still matches
                await choiceRow(need, { id, base: 0, text: 'zzz', want: null, readout: i18n[0].fr });
            } finally { await setLangByUi('en'); }
        });

        await row(VE_NAMES[12], async (need) => {
            const id = 's1_scan_mode', ch = choices(id);
            await setLangByUi('fr');
            const v0 = await prep(id, 'choice', 0);
            try {
                await open(id, 'choice');
                await type_(ch[2]);                                              // a VALID, different value: a commit would write it
                await page.evaluate(() => window.__setLanguage('en'));
                const s = await snap(id, 'choice');
                need(closedClean(s), `the field survived the language switch (open ${s.open}, inputs ${s.anyInput}, editing marks ${s.anyEditing})`);
                need(s.value === 0 && s.log === '', `the switch wrote the typed text (index ${s.value}, calls "${s.log}") — it must CANCEL`);
                need(norm(s.readout) === norm(i18n[0].en), `readout after the switch "${norm(s.readout)}", expected the English face "${i18n[0].en}"`);
                // NOT a value-entry criterion: v1 never re-localizes a scan-mode knob's
                // aria-valuetext on a language switch (same without any field open).
                if (norm(s.aria) !== norm(s.readout))
                    console.log(`   NOTE (pre-existing v1, not counted): knob-${id} aria-valuetext "${norm(s.aria)}" stays in the old language; readout "${norm(s.readout)}"`);
                // Same on a plain float readout, and the page really is English again.
                const g0 = await prep('glide', 'float', 100);
                try {
                    await open('glide'); await type_('300');
                    await page.evaluate(() => window.__setLanguage('fr'));
                    const g = await snap('glide');
                    need(closedClean(g) && g.value === 100 && g.log === '', `glide: field open ${g.open}, value ${g.value}, calls "${g.log}" after a language switch`);
                } finally { await restore('glide', 'float', g0); }
            } finally {
                await restore(id, 'choice', v0);
                await page.evaluate(() => window.__setLanguage('en'));
            }
        });

        await row(VE_NAMES[13], async (need) => {
            const id = 's1_size';
            await gotoTab(await tabOf(id));
            const v0 = await prep(id, 'float', 100);
            const sync = await page.evaluate(() => window.__stubStates.combos.get('sync_mode'));
            void sync;
            try {
                await open(id); await type_('500');
                // Host automation flips Sync to Tempo: the Free cell is hidden under the field.
                await page.evaluate(() => { const st = window.__stubStates.combos.get('sync_mode'); st.choiceIndex = 1; st.valueChangedEvent.callListeners(); });
                await page.waitForTimeout(100);                                   // let any late blur land
                const s = await snap(id);
                const hidden = await page.evaluate((id) => document.getElementById(`cell-${id}`).hidden, id);
                need(hidden, 'the Free cell was not hidden by Sync = Tempo');
                need(closedClean(s), `the field survived its cell being hidden (open ${s.open}, inputs ${s.anyInput}, editing marks ${s.anyEditing})`);
                need(s.value === 100 && s.log === '', `hiding the cell wrote the typed text (value ${s.value}, calls "${s.log}") — it must CANCEL`);
            } finally {
                await page.evaluate(() => { const st = window.__stubStates.combos.get('sync_mode'); st.choiceIndex = 0; st.valueChangedEvent.callListeners(); });
                await restore(id, 'float', v0);
            }
        });

        console.log(`\n   value entry ${vePass} / ${veRun} rows`);
    }

    // ═════════════════════════════════════════════ lanes (PLAN-3.2 PD27) ══
    const LN_NAMES = ['LN1 recording class on the target lane only',
        'LN2 rec-status class + label for run 0–5 and bus 0, in the page\'s precedence',
        'LN3 Clear / Save / Relink call their natives with the 1-based slot; Save absent on a file: lane',
        'LN4 toast.loadFailed when error turns non-empty, not again while unchanged, not at the first pull',
        'LN5 toast.saveFailed on timeout, none on cancelled',
        'LN6 fetchOverview refetches when ovRev moves and stores the answer\'s rev',
        'LN7 scan-mode aria-valuetext is French after a switch to fr',
        'PX1 playhead', 'PX2 grain dot of stream 1 and of stream 4', 'PX3 feedback head', 'PX4 record head', 'PX5 dirty tick'];
    let lnRun = 0, lnPass = 0;

    if (LIVENESS_ONLY) {
        console.log('\n   lanes: NOT RUN (--liveness-only)');
    } else {
        console.log('\n-- lanes (LN1–LN7, PX1–PX5)');
        // The fixture as the C++ encoders dumped it (expectations come from DISK;
        // the page gets the in-page copy, which the 0-based control alters).
        const disk = JSON.parse(fs.readFileSync(path.join(pluginRoot, 'tests', 'ui-stub', 'generic-overrides.json'), 'utf8')).natives;

        await page.evaluate((zeroBased) => {
            const nat = window.__stubOverrides.natives, calls = window.__stubNativeCalls;
            const L = window.__ln = { nat };
            if (zeroBased) nat.getTelemetry.grains.forEach((r) => { r[0] -= 1; });      // the negative control's whole mutation
            L.saved = { tel: JSON.stringify(nat.getTelemetry), slots: JSON.stringify(nat.getSlotState.slots),
                        ovDirty: nat.getSlotOverview.dirty.slice(), ovRev: nat.getSlotOverview.rev };
            L.count = (name, slot) => { let k = 0; for (const c of calls) if (c.name === name && (slot === undefined || (c.args[0] && c.args[0].slot === slot))) ++k; return k; };
            const raf = () => new Promise((r) => requestAnimationFrame(r));
            L.until = async (pred, what) => { const t0 = performance.now(); while (!pred()) { if (performance.now() - t0 > 5000) throw new Error(`timed out waiting for ${what}`); await raf(); } };
            // The page pulled telemetry at least once AFTER the edit and ingested it; then a frame was drawn from it.
            L.settle = async () => { const b = L.count('getTelemetry'); await L.until(() => L.count('getTelemetry') >= b + 2, 'two telemetry pulls'); await raf(); await raf(); return L.count('getTelemetry') - b; };
            L.set = (root, ops) => { for (const [pathArr, value] of ops) { let o = root; for (const k of pathArr.slice(0, -1)) o = o[k]; o[pathArr[pathArr.length - 1]] = value; } };
            L.restoreTel = () => { const keep = nat.getTelemetry.slotRev; Object.assign(nat.getTelemetry, JSON.parse(L.saved.tel)); nat.getTelemetry.slotRev = keep; };
            // New slot state behind a slotRev bump; resolves once the page has pulled it.
            L.pushSlots = async () => {
                const b = L.count('getSlotState');
                nat.getSlotState.revision += 1; nat.getTelemetry.slotRev += 1;
                await L.until(() => L.count('getSlotState') > b, 'a slot-state pull'); await raf(); await raf();
                return L.count('getSlotState') - b;
            };
            L.restoreSlots = async () => { nat.getSlotState.slots.splice(0, 4, ...JSON.parse(L.saved.slots)); return L.pushSlots(); };
            // ── pixel frames of the four lane canvases ──
            L.frames = [];
            L.shot = async () => {
                await L.settle();
                const f = [1, 2, 3, 4].map((n) => { const c = document.getElementById(`lane-canvas-${n}`); return c.getContext('2d').getImageData(0, 0, c.width, c.height); });
                L.frames.push(f);
                return L.frames.length - 1;
            };
            L.geom = (n) => { const c = document.getElementById(`lane-canvas-${n}`); return { W: c.clientWidth, H: c.clientHeight, bw: c.width, bh: c.height, dpr: window.devicePixelRatio || 1 }; };
            // Pixels of lane n that differ by >= thr (any channel) between two frames,
            // binned to CSS pixels: columns, the column / row holding the most of them.
            L.diff = (a, b, n, thr) => {
                const A = L.frames[a][n - 1], B = L.frames[b][n - 1], g = L.geom(n);
                const sx = A.width / g.W, sy = A.height / g.H;
                const col = new Array(g.W).fill(0), row = new Array(g.H).fill(0);
                let total = 0, max = 0;
                for (let y = 0; y < A.height; y++) for (let x = 0; x < A.width; x++) {
                    const i = (y * A.width + x) * 4;
                    const d = Math.max(Math.abs(A.data[i] - B.data[i]), Math.abs(A.data[i + 1] - B.data[i + 1]), Math.abs(A.data[i + 2] - B.data[i + 2]), Math.abs(A.data[i + 3] - B.data[i + 3]));
                    if (d > max) max = d;
                    if (d >= thr) { ++total; ++col[Math.min(g.W - 1, Math.floor(x / sx))]; ++row[Math.min(g.H - 1, Math.floor(y / sy))]; }
                }
                const peak = (arr) => { const m = Math.max(...arr); if (m === 0) return null; const at = arr.map((v, i) => (v === m ? i : -1)).filter((i) => i >= 0); return (at[0] + at[at.length - 1]) / 2; };
                const span = (arr) => { const at = arr.map((v, i) => (v > 0 ? i : -1)).filter((i) => i >= 0); return at.length ? [at[0], at[at.length - 1]] : null; };
                return { total, max, cols: span(col), rows: span(row), peakCol: peak(col), peakRow: peak(row), nCols: col.filter((v) => v > 0).length };
            };
        }, GRAINS_0BASED);

        const lnRow = async (name, fn) => {
            ++lnRun;
            const notes = [], facts = [];
            const need = (cond, msg) => { if (!cond) notes.push(msg); };
            const say = (msg) => facts.push(msg);
            try { await fn(need, say); } catch (e) { notes.push(`threw: ${e.message.split('\n')[0]}`); }
            // Whatever a row did, the next one starts from the fixture.
            try {
                await page.evaluate(async () => {
                    const L = window.__ln;
                    L.restoreTel(); delete L.nat.saveRecording;
                    L.nat.getSlotOverview.dirty = L.saved.ovDirty.slice(); L.nat.getSlotOverview.rev = L.saved.ovRev;
                    if (JSON.stringify(L.nat.getSlotState.slots) !== L.saved.slots) await L.restoreSlots();
                    await L.settle();
                });
            } catch (e) { notes.push(`restore threw: ${e.message.split('\n')[0]}`); }
            if (notes.length === 0) { ++lnPass; pass(`${name}${facts.length ? ' — ' + facts.join('; ') : ''}`); }
            else fail(`${name} — ${notes.join('; ')}${facts.length ? ' [measured: ' + facts.join('; ') + ']' : ''}`);
        };
        const tel = (ops) => page.evaluate(async (ops) => { const L = window.__ln; L.set(L.nat.getTelemetry, ops); return L.settle(); }, ops);
        const lang = (code) => page.evaluate((code) => { window.__setLanguage(code); return document.documentElement.lang; }, code);
        const toastLen = () => page.evaluate(() => window.__toastLog.length);
        const toastSince = (from) => page.evaluate((from) => window.__toastLog.slice(from), from);
        const laneTab = async (id) => gotoTab(await page.evaluate((id) => window.__probe.pageOf(document.getElementById(id)), id));
        const clickNative = async (sel, name) => {
            const base = await page.evaluate(() => window.__stubNativeCalls.length);
            await page.click(sel, { timeout: 2000 });
            // The native was called and slotAction ran to its end (it pulls slot state last).
            await page.waitForFunction(({ base, name }) => {
                const c = window.__stubNativeCalls.slice(base); const i = c.findIndex((x) => x.name === name);
                return i >= 0 && c.slice(i + 1).some((x) => x.name === 'getSlotState');
            }, { base, name }, { timeout: 5000 });
            await page.waitForTimeout(60);
            return page.evaluate(({ base, name }) => window.__stubNativeCalls.slice(base).filter((x) => x.name === name).map((x) => JSON.stringify(x.args)), { base, name });
        };

        // ── LN1 ────────────────────────────────────────────────────────────
        await lnRow(LN_NAMES[0], async (need, say) => {
            const recLanes = () => page.evaluate(() => [1, 2, 3, 4].filter((n) => document.getElementById(`lane-${n}`).classList.contains('recording')));
            const seen = [];
            for (const [run, slot, want] of [[2, 2, [2]], [2, 3, [3]], [2, 1, [1]], [2, 4, [4]], [1, 2, []], [3, 2, []], [0, 0, []], [2, 2, [2]]]) {
                const pulls = await tel([[['rec', 'run'], run], [['rec', 'slot'], slot], [['rec', 'bus'], 1]]);
                const got = await recLanes();
                need(pulls >= 2, `run ${run} slot ${slot}: the page did not pull the edited telemetry`);
                need(JSON.stringify(got) === JSON.stringify(want), `run ${run} slot ${slot}: recording lanes [${got}], expected [${want}]`);
                seen.push(`run ${run} slot ${slot} -> [${got}]`);
            }
            say(seen.join(', '));
        });

        // ── LN2 ────────────────────────────────────────────────────────────
        await lnRow(LN_NAMES[1], async (need, say) => {
            const trig0 = await page.evaluate(() => window.__stubStates.combos.get('rec_trigger').getChoiceIndex());
            const setTrig = (i) => page.evaluate((i) => { const st = window.__stubStates.combos.get('rec_trigger'); window.__probe.push(st, 'choice', i); return st.getChoiceIndex(); }, i);
            // [bus, run, signal, trigger] -> [class, label key]; each pair of neighbours is one precedence step.
            const cases = [
                [0, 2, true, 0, 'nosc', 'rec.noSidechain'],        // bus beats 2
                [0, 0, true, 0, 'nosc', 'rec.noSidechain'],        // bus 0
                [1, 2, false, 0, 'recording', 'rec.recording'],    // 2 beats !signal
                [1, 3, false, 0, 'recording', 'rec.committing'],   // 3 beats !signal
                [1, 1, false, 0, 'armed', 'rec.armed'],            // 1 beats !signal
                [1, 1, true, 1, 'armed', 'rec.waitLevel'],
                [1, 1, true, 2, 'armed', 'rec.waitNote'],
                [1, 5, true, 0, 'nosignal', 'rec.discarded'],      // 5 with a live signal
                [1, 5, false, 0, 'nosignal', 'rec.discarded'],     // 5 beats !signal (the label tells them apart)
                [1, 4, false, 0, 'nosignal', 'rec.noSignal'],      // !signal beats 4
                [1, 0, false, 0, 'nosignal', 'rec.noSignal'],
                [1, 4, true, 0, 'ready', 'rec.done'],              // 4
                [1, 0, true, 0, 'ready', 'rec.ready'],             // 0
            ];
            const seen = [];
            try {
                for (const [bus, run, signal, trig, cls, key] of cases) {
                    const t = await setTrig(trig);
                    const pulls = await tel([[['rec', 'bus'], bus], [['rec', 'run'], run], [['rec', 'signal'], signal], [['rec', 'slot'], 2]]);
                    const got = await page.evaluate(() => ({ cls: document.getElementById('rec-status').className, key: document.getElementById('rec-status-t').dataset.i18n,
                        nosc: document.getElementById('plugin').classList.contains('rec-nosc') }));
                    const tag = `bus ${bus} run ${run} signal ${signal ? 1 : 0} trig ${trig}`;
                    need(pulls >= 2 && t === trig, `${tag}: the stimulus did not reach the page (pulls ${pulls}, trigger ${t})`);
                    need(got.cls === cls && got.key === key, `${tag}: class "${got.cls}" label ${got.key}, expected "${cls}" ${key}`);
                    need(got.nosc === (bus === 0), `${tag}: #plugin.rec-nosc is ${got.nosc}`);
                    seen.push(`${bus}/${run}/${signal ? 1 : 0}/${trig} ${got.cls}:${String(got.key).replace('rec.', '')}`);
                }
            } finally { await setTrig(trig0); }
            say(`${cases.length} states (bus/run/signal/trig class:label) ${seen.join(', ')}`);
        });

        // ── LN3 ────────────────────────────────────────────────────────────
        await lnRow(LN_NAMES[2], async (need, say) => {
            const schemes = await page.evaluate(() => window.__ln.nat.getSlotState.slots.map((s) => `${s.scheme || 'empty'}${s.missing ? '+missing' : ''}`));
            need(schemes.join(',') === 'factory,file,rec+missing,empty', `the fixture lanes are [${schemes}], expected factory, file, rec+missing, empty`);
            await laneTab('lane-1');
            const shown = (id) => page.evaluate((id) => { const e = document.getElementById(id); return !!e && !e.hidden && !e.disabled && e.getClientRects().length > 0; }, id);
            const seen = [];
            for (const [sel, name, slot] of [['#lane-clear-1', 'clearSlot', 1], ['#lane-clear-2', 'clearSlot', 2],
                                             ['#lane-badge-save-3', 'saveRecording', 3], ['#lane-badge-relink-3', 'relinkSlot', 3]]) {
                const got = await clickNative(sel, name);
                need(got.length === 1 && got[0] === `[{"slot":${slot}}]`, `${sel}: ${name} called with ${got.join(' ') || 'nothing'}, expected [{"slot":${slot}}]`);
                seen.push(`${sel.slice(1)} -> ${name}${got[0] || '()'}`);
            }
            for (const id of ['lane-badge-save-2', 'lane-badge-save-1', 'lane-badge-save-4', 'lane-badge-relink-1', 'lane-badge-relink-2'])
                need(!(await shown(id)), `${id} is shown (Save is for rec: lanes, Relink for missing ones)`);
            need(await page.evaluate(() => document.getElementById('lane-clear-4').disabled), 'Clear is enabled on the empty lane 4');
            // Per-row answer: lane 2 becomes a recording -> Save appears there and carries slot 2.
            const pulled = await page.evaluate(async () => { const L = window.__ln; Object.assign(L.nat.getSlotState.slots[1], { scheme: 'rec', name: 'probe-take.wav' }); return L.pushSlots(); });
            need(pulled >= 1 && await shown('lane-badge-save-2'), 'Save did not appear on lane 2 after its slot became rec:');
            const got = await clickNative('#lane-badge-save-2', 'saveRecording');
            need(got.length === 1 && got[0] === '[{"slot":2}]', `#lane-badge-save-2: saveRecording called with ${got.join(' ') || 'nothing'}`);
            seen.push(`lane-badge-save-2 (slot 2 as rec:) -> saveRecording${got[0] || '()'}`);
            say(`${seen.join(', ')}; Save hidden on file: lane 2, factory lane 1, empty lane 4`);
        });

        // ── LN4 ────────────────────────────────────────────────────────────
        await lnRow(LN_NAMES[3], async (need, say) => {
            // Boot: the fixture's slot 4 carries an error at the FIRST pull -> no toast.
            need(bootToast.errs[3] !== '' && bootToast.pulls >= 1, `boot stimulus missing: slot 4 error "${bootToast.errs[3]}", ${bootToast.pulls} slot-state pull(s)`);
            need(!bootToast.log.includes('toast.loadFailed'), `the page toasted at boot for an error already present (${bootToast.log.join(', ')})`);
            const step = async (error) => {
                const from = await toastLen();
                const pulls = await page.evaluate(async (error) => { const L = window.__ln; if (error !== null) L.nat.getSlotState.slots[1].error = error; return L.pushSlots(); }, error);
                await page.waitForTimeout(80);
                return { pulls, toasts: (await toastSince(from)).filter((t) => t.key === 'toast.loadFailed') };
            };
            const seen = [];
            const want = async (label, error, n) => {
                const r = await step(error);
                need(r.pulls >= 1, `${label}: the page did not pull the new slot state`);
                need(r.toasts.length === n, `${label}: ${r.toasts.length} toast.loadFailed, expected ${n}`);
                if (n === 1 && r.toasts.length === 1) {
                    need(r.toasts[0].vars === '{"n":"2"}', `${label}: toast vars ${r.toasts[0].vars}, expected {"n":"2"}`);
                    need(/2/.test(r.toasts[0].text) && r.toasts[0].text.length > 8, `${label}: toast text "${r.toasts[0].text}"`);
                }
                seen.push(`${label} ${r.toasts.length}${r.toasts[0] ? ' "' + r.toasts[0].text + '"' : ''}`);
            };
            await want('error "" -> "decode failed: a"', 'decode failed: a', 1);
            need(await page.evaluate(() => document.getElementById('toast').classList.contains('show')), 'the toast is not shown (#toast.show)');
            await want('unchanged, pulled again', null, 0);
            await want('unchanged, pulled a third time', null, 0);
            await want('-> "decode failed: b" (changed)', 'decode failed: b', 1);
            await want('-> "" (cleared)', '', 0);
            await want('-> "decode failed: b" (re-armed)', 'decode failed: b', 1);
            say(seen.join(', ') + '; boot: slot 4 error present, 0 toasts');
        });

        // ── LN5 ────────────────────────────────────────────────────────────
        await lnRow(LN_NAMES[4], async (need, say) => {
            await laneTab('lane-3');
            const seen = [];
            for (const [answer, n] of [[{ ok: false, error: 'timeout' }, 1], [{ ok: false, error: 'cancelled' }, 0],
                                       [{ ok: false, error: 'writeFailed' }, 1], [{ ok: true, path: 'x.wav' }, 0]]) {
                // The stub must really answer with this object before the click counts.
                const echoed = await page.evaluate(async (answer) => {
                    window.__ln.nat.saveRecording = answer;
                    const m = await import('/js/juce/index.js');
                    return JSON.stringify(await m.getNativeFunction('saveRecording')({ probe: true }));
                }, answer);
                need(echoed === JSON.stringify(answer), `the stub answers saveRecording with ${echoed}, not ${JSON.stringify(answer)}`);
                const from = await toastLen();
                const got = await clickNative('#lane-badge-save-3', 'saveRecording');
                const toasts = (await toastSince(from)).filter((t) => t.key === 'toast.saveFailed');
                const tag = answer.ok ? 'ok' : answer.error;
                need(got.includes('[{"slot":3}]'), `${tag}: saveRecording was not called with slot 3 (${got.join(' ')})`);
                need(toasts.length === n, `${tag}: ${toasts.length} toast.saveFailed, expected ${n}`);
                if (n === 1 && toasts.length === 1) need(toasts[0].vars === '{"n":"3"}', `${tag}: toast vars ${toasts[0].vars}, expected {"n":"3"}`);
                seen.push(`${tag} -> ${toasts.length}${toasts[0] ? ' "' + toasts[0].text + '"' : ''}`);
            }
            say(seen.join(', '));
        });

        // ── LN6 ────────────────────────────────────────────────────────────
        await lnRow(LN_NAMES[5], async (need, say) => {
            // Lane 1 only: its telemetry ovRev equals the fixture answer's rev, so it is quiet at rest.
            const fetches = (ms) => page.evaluate(async (ms) => {
                const L = window.__ln; const b = L.count('getSlotOverview', 1), t = L.count('getTelemetry');
                await new Promise((r) => setTimeout(r, ms));
                return { n: L.count('getSlotOverview', 1) - b, pulls: L.count('getTelemetry') - t };
            }, ms);
            const set = (ovRev, rev) => page.evaluate(async ({ ovRev, rev }) => {
                const L = window.__ln;
                if (ovRev !== null) L.nat.getTelemetry.lanes[0].ovRev = ovRev;
                if (rev === 'none') delete L.nat.getSlotOverview.rev; else if (rev !== null) L.nat.getSlotOverview.rev = rev;
                await L.settle(); await new Promise((r) => setTimeout(r, 250));
            }, { ovRev, rev });
            const r0 = disk.getTelemetry.lanes[0].ovRev;
            need(r0 === disk.getSlotOverview.rev, `fixture: lanes[0].ovRev ${r0} != overview rev ${disk.getSlotOverview.rev}`);
            const rest = await fetches(500);
            need(rest.pulls >= 5 && rest.n === 0, `at rest (ovRev == stored rev): ${rest.n} fetch(es) of lane 1 over ${rest.pulls} telemetry pulls, expected 0`);
            // ovRev moves, the answer still says the old rev: a page that stores the
            // ANSWER's rev keeps asking; one that stored the asked rev would stop after one.
            await set(r0 + 96, null);
            const stale = await fetches(700);
            need(stale.pulls >= 5 && stale.n >= 3, `ovRev moved, answer rev stale: ${stale.n} fetch(es) in 700 ms, expected >= 3 (refetch while the stored rev is the answer's old one)`);
            // The answer catches up: stored == telemetry, the lane goes quiet.
            await set(null, r0 + 96);
            const caught = await fetches(500);
            need(caught.pulls >= 5 && caught.n === 0, `answer rev == ovRev: ${caught.n} fetch(es), expected 0`);
            // An answer without a rev: the asked one is stored (one fetch, then quiet).
            await set(r0 + 200, 'none');
            const bare = await fetches(500);
            need(bare.pulls >= 5 && bare.n === 0, `answer without rev: ${bare.n} fetch(es) after the first, expected 0 (the asked rev is stored)`);
            say(`lane 1 fetches: at rest ${rest.n} / ${rest.pulls} pulls; ovRev ${r0} -> ${r0 + 96} with answer rev ${r0}: ${stale.n} in 700 ms; answer rev ${r0 + 96}: ${caught.n}; no rev in the answer: ${bare.n}`);
        });

        // ── LN7 ────────────────────────────────────────────────────────────
        await lnRow(LN_NAMES[6], async (need, say) => {
            const faces = await page.evaluate(async () => {
                const m = await import('/js/i18n.js');
                return ['opt.scan.free', 'opt.scan.retrigger', 'opt.scan.pingpong'].map((k) => ({ en: m.LABELS[k].en.t, fr: m.LABELS[k].fr.t }));
            });
            const k = faces.findIndex((f) => f.fr && f.fr !== f.en);
            if (k < 0) throw new Error(`no scan-mode face differs in French: ${JSON.stringify(faces)}`);
            const ids = [1, 2, 3, 4].map((n) => `s${n}_scan_mode`);
            const v0 = await page.evaluate(({ ids, k }) => ids.map((id) => { const st = window.__stubStates.combos.get(id); const v = st.getChoiceIndex(); window.__probe.push(st, 'choice', k); return v; }), { ids, k });
            const read = () => page.evaluate((ids) => ids.map((id) => ({ aria: document.getElementById(`knob-${id}`).getAttribute('aria-valuetext'), val: document.getElementById(`val-${id}`).textContent })), ids);
            try {
                const en = await read();
                need(en.every((r) => norm(r.aria) === norm(faces[k].en)), `before the switch: aria-valuetext [${en.map((r) => r.aria)}], expected "${faces[k].en}"`);
                need(await lang('fr') === 'fr', 'the page did not switch to fr (html lang)');
                const fr = await read();
                need(fr.every((r) => norm(r.val) === norm(faces[k].fr)), `stimulus: the readouts are [${fr.map((r) => r.val)}] after the switch, expected "${faces[k].fr}"`);
                need(fr.every((r) => norm(r.aria) === norm(faces[k].fr)), `aria-valuetext after the switch to fr: [${fr.map((r) => r.aria)}], expected "${faces[k].fr}"`);
                need(await lang('en') === 'en', 'the page did not switch back to en');
                const back = await read();
                need(back.every((r) => norm(r.aria) === norm(faces[k].en)), `aria-valuetext back in en: [${back.map((r) => r.aria)}], expected "${faces[k].en}"`);
                say(`4 scan-mode knobs at choice ${k}: en "${en[0].aria}" -> fr "${fr[0].aria}" -> en "${back[0].aria}"`);
            } finally {
                await lang('en');
                await page.evaluate(({ ids, v0 }) => ids.forEach((id, i) => window.__probe.push(window.__stubStates.combos.get(id), 'choice', v0[i])), { ids, v0 });
            }
        });

        // ── pixel rows ─────────────────────────────────────────────────────
        const geom = await page.evaluate(() => [1, 2, 3, 4].map((n) => window.__ln.geom(n)));
        const { W, H } = geom[0];
        const mid = H / 2 + 2;                                    // drawLane's centre line
        console.log(`   lane canvas ${W} x ${H} CSS px, backing ${geom[0].bw} x ${geom[0].bh}, devicePixelRatio ${geom[0].dpr}; `
                  + `a pixel differs at >= ${PX_DIFF} / 255 (record head: >= ${PX_STRONG}); x tolerance ± ${PX_TOL} px`);
        const shot = (ops) => page.evaluate(async (ops) => { const L = window.__ln; if (ops) L.set(L.nat.getTelemetry, ops); return L.shot(); }, ops || null);
        const diff = (a, b, n, thr = PX_DIFF) => page.evaluate(({ a, b, n, thr }) => window.__ln.diff(a, b, n, thr), { a, b, n, thr });
        const fmt = (d) => (d.total ? `${d.total} px, cols ${d.cols[0]}–${d.cols[1]}, peak col ${d.peakCol}, rows ${d.rows[0]}–${d.rows[1]}, peak row ${d.peakRow}` : '0 px');
        // The feature: differing pixels exist, the densest column is at x ± 2, none outside x ± reach.
        const featureAt = (need, d, what, x, reach, y) => {
            if (!d.total) { need(false, `${what}: NO pixel differs from the frame without it (expected at x ${x})`); return; }
            need(Math.abs(d.peakCol - x) <= PX_TOL, `${what}: densest column ${d.peakCol}, expected ${x} ± ${PX_TOL}`);
            need(d.cols[0] >= x - reach && d.cols[1] <= x + reach, `${what}: pixels differ in cols ${d.cols[0]}–${d.cols[1]}, outside ${x} ± ${reach}`);
            if (y !== undefined) need(Math.abs(d.peakRow - y) <= PX_TOL, `${what}: densest row ${d.peakRow}, expected ${y} ± ${PX_TOL}`);
        };
        const blank = async (need, a, b, lanesToCheck, what) => {
            for (const n of lanesToCheck) { const d = await diff(a, b, n, 1); need(d.total === 0, `${what}: lane ${n} changed where the feature is absent (${fmt(d)})`); }
        };
        // Two frames of an unchanged stub answer are identical: any later difference is the feature.
        const s0 = await shot(), s1 = await shot();
        const still = [];
        for (const n of [1, 2, 3, 4]) still.push((await diff(s0, s1, n, 1)).total);
        const pxStable = still.every((v) => v === 0);
        console.log(`   two frames of the unchanged fixture differ in [${still.join(', ')}] pixels per lane (must be 0)`);

        // ── PX1 playhead ──
        await lnRow(LN_NAMES[7], async (need, say) => {
            need(pxStable, `frames of an unchanged answer are not identical ([${still}])`);
            const v = disk.getTelemetry.voices[0];
            need(v[0] === 1 && v[3] >= 0, `fixture voice 0 is not a held voice with a stream-1 scan (${JSON.stringify(v)})`);
            const on = await shot();
            const off = await shot([[['voices', 0, 0], 0]]);
            const x1 = Math.floor(Math.round(v[3] * W) + 0.5);
            const d1 = await diff(on, off, 1);
            featureAt(need, d1, `playhead of the fixture voice (scan ${v[3]})`, x1, 3);
            need(d1.total > 0 && d1.rows[0] <= 4 && d1.rows[1] >= H - 5, `playhead: differing rows ${d1.rows}, expected the lane's height (3 … ${H - 3})`);
            // A second position (per-row answer), and scan −1 = no playhead.
            const moved = await shot([[['voices', 0, 0], 1], [['voices', 0, 3], 0.7]]);
            const x2 = Math.floor(Math.round(0.7 * W) + 0.5);
            const d2 = await diff(moved, off, 1);
            featureAt(need, d2, 'playhead at scan 0.7', x2, 3);
            const none = await shot([[['voices', 0, 3], -1]]);
            const d3 = await diff(none, off, 1, 1);
            need(d3.total === 0, `scan −1 still draws on lane 1 (${fmt(d3)})`);
            await blank(need, on, off, [3, 4], 'playhead');
            say(`scan ${v[3]} -> expected col ${x1}: ${fmt(d1)}; scan 0.7 -> expected col ${x2}: ${fmt(d2)}; scan −1: ${d3.total} px; lanes 3 / 4 unchanged`);
        });

        // ── PX2 grain dots, stream 1 and stream 4 ──
        await lnRow(LN_NAMES[8], async (need, say) => {
            need(pxStable, `frames of an unchanged answer are not identical ([${still}])`);
            const rows = disk.getTelemetry.grains;
            const streams = [...new Set(rows.map((r) => r[0]))].sort();
            need(streams.join(',') === '1,2,3,4', `the C++-dumped fixture's grain streams are [${streams}], expected 1…4 (1-based)`);
            // One fixture row per stream, away from the playhead column; expectations from DISK, pixels from the page's copy.
            const pick = (st) => rows.findIndex((r, i) => r[0] === st && Math.abs(r[1] - disk.getTelemetry.voices[0][3]) > 0.02 && i >= 0);
            const i1 = pick(1), i4 = pick(4);
            if (i1 < 0 || i4 < 0) throw new Error('the fixture has no usable grain row for stream 1 / stream 4');
            const sent = await page.evaluate(({ i1, i4 }) => { const g = JSON.parse(window.__ln.saved.tel).grains; return [g[i1], g[i4]]; }, { i1, i4 });
            need(sent[0][1] === rows[i1][1] && sent[1][1] === rows[i4][1], 'the stub does not carry the fixture rows');
            await setLaneLoaded(4, true);                           // lane 4 is empty in the fixture: an empty lane draws nothing
            try {
                const none = await shot([[['grains'], []]]);
                const seen = [];
                for (const [st, i] of [[1, i1], [4, i4]]) {
                    const r = rows[i];
                    const on = await shot([[['grains'], [sent[st === 1 ? 0 : 1]]]]);
                    const x = Math.floor(r[1] * W), y = Math.floor(mid - r[2] * (H / 2 - 9));
                    const rad = 3 * (1.1 + Math.min(2.4, Math.sqrt((r[5] || 0.1) * 1000) / 12));
                    const d = await diff(on, none, st);
                    featureAt(need, d, `stream-${st} grain (row ${JSON.stringify(r)}) on lane ${st}`, x, Math.ceil(rad) + 2, Math.min(H - 1, y));
                    const others = [1, 2, 3, 4].filter((n) => n !== st);
                    const stray = [];
                    for (const n of others) { const o = await diff(on, none, n, 1); if (o.total) stray.push(`lane ${n}: ${fmt(o)}`); }
                    need(stray.length === 0, `stream-${st} grain drew on another lane (${stray.join(' | ')})`);
                    seen.push(`stream ${st} row sent as ${JSON.stringify(sent[st === 1 ? 0 : 1])} -> lane ${st} expected (${x}, ${y}): ${fmt(d)}; other lanes ${stray.length ? stray.join(' | ') : 'unchanged'}`);
                }
                say(seen.join(' || '));
            } finally { await setLaneLoaded(4, false); }
        });

        // ── PX3 feedback head ──
        await lnRow(LN_NAMES[9], async (need, say) => {
            need(pxStable, `frames of an unchanged answer are not identical ([${still}])`);
            const fb = disk.getTelemetry.lanes[0].fb;
            need(typeof fb === 'number' && disk.getTelemetry.lanes[0].rec === null, `fixture lane 1: fb ${fb}, rec ${disk.getTelemetry.lanes[0].rec}`);
            const frozen = await page.evaluate(() => window.__stubStates.toggles.get('freeze').getValue());
            need(frozen === false, 'freeze is on: the head would be dashed');
            const on = await shot();
            const off = await shot([[['lanes', 0, 'fb'], null]]);
            const x1 = Math.floor(Math.round(fb * W) + 0.5);
            const d1 = await diff(on, off, 1);
            featureAt(need, d1, `feedback head (fb ${fb})`, x1, 5);
            need(d1.total > 0 && d1.rows[0] >= H - 16 && d1.rows[1] === H - 1, `feedback head: rows ${d1.rows}, expected the lane's bottom (${H - 15} … ${H - 1})`);
            const moved = await shot([[['lanes', 0, 'fb'], 0.2]]);
            const x2 = Math.floor(Math.round(0.2 * W) + 0.5);
            const d2 = await diff(moved, off, 1);
            featureAt(need, d2, 'feedback head at fb 0.2', x2, 5);
            await blank(need, on, off, [2, 3, 4], 'feedback head');
            say(`fb ${fb} -> expected col ${x1}: ${fmt(d1)}; fb 0.2 -> expected col ${x2}: ${fmt(d2)}; lanes 2–4 unchanged`);
        });

        // ── PX4 record head ──
        await lnRow(LN_NAMES[10], async (need, say) => {
            need(pxStable, `frames of an unchanged answer are not identical ([${still}])`);
            const rec = disk.getTelemetry.lanes[1].rec;
            need(typeof rec === 'number', `fixture lane 2 rec is ${rec}`);
            const on = await shot();
            const off = await shot([[['lanes', 1, 'rec'], null]]);
            const x1 = Math.floor(Math.round(rec * W) + 0.5);
            const d1 = await diff(on, off, 2, PX_STRONG);
            featureAt(need, d1, `record head (rec ${rec})`, x1, 6);
            need(d1.total > 0 && d1.rows[0] === 0 && d1.rows[1] === H - 1, `record head: rows ${d1.rows}, expected the full height (0 … ${H - 1})`);
            // The tint: the lane left of the head changes too (weakly), the lane right of the head + 6 px only where the waveform dims.
            const weak = await diff(on, off, 2, PX_DIFF);
            need(weak.total > d1.total && weak.cols[0] <= 2, `record tint: weak differences start at col ${weak.cols ? weak.cols[0] : '—'}, expected the lane's left edge`);
            const moved = await shot([[['lanes', 1, 'rec'], 0.7]]);
            const x2 = Math.floor(Math.round(0.7 * W) + 0.5);
            const d2 = await diff(moved, off, 2, PX_STRONG);
            featureAt(need, d2, 'record head at rec 0.7', x2, 6);
            await blank(need, on, off, [1, 3, 4], 'record head');
            say(`rec ${rec} -> expected col ${x1}: ${fmt(d1)} (>= ${PX_STRONG}); tint + dimmed waveform (>= ${PX_DIFF}): ${fmt(weak)}; rec 0.7 -> expected col ${x2}: ${fmt(d2)}; lanes 1 / 3 / 4 unchanged`);
        });

        // ── PX5 dirty tick ──
        await lnRow(LN_NAMES[11], async (need, say) => {
            need(pxStable, `frames of an unchanged answer are not identical ([${still}])`);
            const ov = disk.getSlotOverview, nb = ov.peaks.length;
            const lit = (k) => ov.dirty[k] / 255 > 0.05 && Math.min(1, ov.peaks[k]) * (H / 2 - 4) >= 0.3;      // drawLane's two conditions
            const colOf = (k) => { const xs = []; for (let x = 0; x < W; x++) if (Math.min(nb - 1, Math.floor(x / W * nb)) === k) xs.push(x); return xs; };
            const litBins = ov.dirty.map((_, k) => k).filter(lit);
            need(litBins.length > 0, 'the fixture overview has no dirty bin above the page\'s 5 % threshold');
            // A new overview answer reaches the page through a revision move (the LN6 path).
            const overview = (dirty) => page.evaluate(async (dirty) => {
                const L = window.__ln, o = L.nat.getSlotOverview, b = L.count('getSlotOverview', 1);
                o.dirty = dirty; o.rev += 1; L.nat.getTelemetry.lanes[0].ovRev = o.rev;
                await L.until(() => L.count('getSlotOverview', 1) > b, 'an overview refetch of lane 1');
                const id = await L.shot();
                return { id, fetched: L.count('getSlotOverview', 1) - b };
            }, dirty);
            const zeros = new Array(nb).fill(0);
            const clean = await overview(zeros);
            const fx = await overview(ov.dirty.slice());
            need(clean.fetched >= 1 && fx.fetched >= 1, 'the page did not refetch the edited overview');
            // (a) the fixture's own dirty bins (the C++ dump): ticks from the first lit column to the last, none beyond.
            const first = colOf(litBins[0])[0], last = colOf(litBins[litBins.length - 1]).pop();
            const d1 = await diff(fx.id, clean.id, 1);
            need(d1.total > 0, 'fixture dirty bins: no tick drawn');
            if (d1.total) {
                need(Math.abs(d1.cols[0] - first) <= PX_TOL && Math.abs(d1.cols[1] - last) <= PX_TOL, `fixture dirty bins: ticks in cols ${d1.cols[0]}–${d1.cols[1]}, expected ${first}–${last} ± ${PX_TOL}`);
                need(d1.rows[1] <= mid, `dirty ticks reach row ${d1.rows[1]}, below the centre line ${mid} (a tick hangs from the top of the upper half)`);
            }
            // (b) one bin alone, in the fixture's clean part: one tick at its column, nothing else.
            const k0 = ov.dirty.findIndex((v, k) => k > litBins[litBins.length - 1] + 40 && v === 0 && Math.min(1, ov.peaks[k]) * (H / 2 - 4) >= 3);
            if (k0 < 0) throw new Error('no clean, loud bin in the fixture to light alone');
            const one = zeros.slice(); one[k0] = 255;
            const single = await overview(one);
            const xk = colOf(k0)[0];
            const d2 = await diff(single.id, clean.id, 1);
            featureAt(need, d2, `single dirty bin ${k0}`, xk, PX_TOL);
            const amp = Math.min(1, ov.peaks[k0]) * (H / 2 - 4);
            if (d2.total) need(d2.rows[0] >= Math.floor(mid - amp) - 1 && d2.rows[1] <= Math.ceil(mid - amp + Math.min(amp, 6)) + 1,
                `single dirty bin: rows ${d2.rows}, expected the top of the peak (${(mid - amp).toFixed(1)} … ${(mid - amp + Math.min(amp, 6)).toFixed(1)})`);
            await blank(need, single.id, clean.id, [3], 'dirty tick');
            say(`fixture dirty bins ${litBins[0]}–${litBins[litBins.length - 1]} -> expected cols ${first}–${last}: ${fmt(d1)}; bin ${k0} alone -> expected col ${xk}: ${fmt(d2)}; refetches ${clean.fetched} / ${fx.fetched} / ${single.fetched}`);
        });

        console.log(`\n   lanes ${lnPass} / ${lnRun} rows`);
        if (lnRun !== LN_NAMES.length) fail(`lanes: ${lnRun} rows ran, expected ${LN_NAMES.length}`);
    }

    console.log(`   page errors: ${errors.length}${errors.length ? ' — ' + errors.slice(0, 3).join(' | ') : ''}`);
    if (errors.length) fail(`${errors.length} page / console error(s)`);
    void liveFailuresBefore;

    const liveOk = hostOk === N_PARAMS && uiOk === N_PARAMS;
    const veText = LANES_ONLY ? 'value entry NOT RUN (--lanes-only)' : LIVENESS_ONLY ? 'value entry NOT RUN (--liveness-only)'
        : !hasVE ? 'value entry NOT RUN — page has no value-entry code' : `value entry ${vePass} / ${veRun}`;
    const lnText = LIVENESS_ONLY ? 'lanes NOT RUN (--liveness-only)' : `lanes ${lnPass} / ${lnRun}`;
    const liveText = LANES_ONLY ? 'liveness NOT RUN (--lanes-only)' : `${N_PARAMS} / ${N_PARAMS} both ways`;
    console.log(failures === 0 ? `ALL PASS (${liveText}; ${veText}; ${lnText})`
                               : `${failures} FAILED (liveness ${LANES_ONLY ? 'NOT RUN' : `${liveOk ? 'OK ' : ''}${hostOk} / ${uiOk} of ${N_PARAMS}`}; ${veText}; ${lnText})`);
    await finish(Math.min(failures, 76));
})().catch((e) => { console.error(e); process.exit(70); });
