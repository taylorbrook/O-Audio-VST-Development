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

    Usage (from anywhere):
      node plugins/O-GranulatorInstrument/tests/ui_binding_probe.js                    liveness + VE on the tree's page
      node plugins/O-GranulatorInstrument/tests/ui_binding_probe.js --liveness-only    261 x 2 only
      node plugins/O-GranulatorInstrument/tests/ui_binding_probe.js --page <file>      serve <file> as index.html
           (env OGI_UI_PAGE=<file> is the same). The served ROOT is still the
           one serve-ui.js assembles — module JS, eb-garamond css + woff2, the
           stub and its preamble all resolve; only index.html is replaced.
      node plugins/O-GranulatorInstrument/tests/ui_binding_probe.js --nc [--page <file>]
           negative control: re-runs itself (liveness only) on a page copy with
           ONE bindKnob removed (s2_drive); exits 0 only if that run FAILS at
           260 / 261 both ways on exactly that id.
      --mutate-bindknob <id>   the raw mutated run --nc drives (exits non-zero).
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
    process.exit(ok ? 0 : 1);
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
    console.log(`== ${PLUGIN} ui_binding_probe${MUTATE ? ' (MUTATED: bindKnob(' + MUTATE + ') removed)' : ''}${LIVENESS_ONLY ? ' (liveness only)' : ''} ==`);
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

    for (const { id, type } of params) {
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

    console.log(`   binders: ${Object.keys(KIND_COUNTS).map((k) => `${k} ${kinds[k] || 0}`).join(', ')}${kinds.none ? ', NONE ' + kinds.none : ''}`
              + ` = ${Object.values(kinds).reduce((a, b) => a + b, 0)}`);
    for (const [k, n] of Object.entries(KIND_COUNTS))
        if ((kinds[k] || 0) !== n) fail(`binder census: ${k} ${kinds[k] || 0}, expected ${n}`);
    console.log(`\n   host -> UI ${hostOk} / ${params.length}, UI -> host ${uiOk} / ${params.length}`);
    if (params.length !== N_PARAMS) fail(`params.tsv has ${params.length} ids, expected ${N_PARAMS}`);
    const liveFailuresBefore = failures;
    if ((hostOk !== N_PARAMS || uiOk !== N_PARAMS) && failures === 0) fail(`liveness: not ${N_PARAMS} / ${N_PARAMS} both ways`);

    // ════════════════════════════════════════════════════════ value entry ══
    const VE_NAMES = ['VE1 Enter applies', 'VE2 Esc cancels', 'VE3 outside click applies',
        'VE4 outside press on a choice knob (preventDefault) applies', 'VE5 unchanged text writes nothing',
        'VE6 exactly one gesture pair per value', 'VE7 clamp above end', 'VE8 -inf on a gain', 'VE9 fCount',
        'VE10 fBend with and without ± / st', 'VE11 a tempo division',
        'VE12a scan mode typed in French, page in fr', 'VE12b language switch closes an open field', 'VE12c Sync twin hidden closes an open field'];
    let veRun = 0, vePass = 0;

    if (LIVENESS_ONLY) {
        console.log('\n   value entry: NOT RUN (--liveness-only)');
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

    console.log(`   page errors: ${errors.length}${errors.length ? ' — ' + errors.slice(0, 3).join(' | ') : ''}`);
    if (errors.length) fail(`${errors.length} page / console error(s)`);
    void liveFailuresBefore;

    const liveOk = hostOk === N_PARAMS && uiOk === N_PARAMS;
    const veText = LIVENESS_ONLY ? 'value entry NOT RUN (--liveness-only)' : !hasVE ? 'value entry NOT RUN — page has no value-entry code' : `value entry ${vePass} / ${veRun}`;
    console.log(failures === 0 ? `ALL PASS (${N_PARAMS} / ${N_PARAMS} both ways; ${veText})`
                               : `${failures} FAILED (liveness ${liveOk ? 'OK ' : ''}${hostOk} / ${uiOk} of ${N_PARAMS}; ${veText})`);
    await finish(Math.min(failures, 76));
})().catch((e) => { console.error(e); process.exit(70); });
