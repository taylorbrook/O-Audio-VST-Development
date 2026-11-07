/*
   This file is part of O-Granulator, an Ouaricon Audio plugin.
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
    O-Granulator — two-way page binding probe, all 220 parameters
    (Stage 3 / Phase 3.1, PLAN-3.1 P110, RESEARCH-3.1 R113 layer 2).

    Serves the shipping page through scripts/serve-ui.js with the generic JUCE
    stub seeded from .planning/params.tsv + tests/ui-stub/generic-overrides.json
    (C++-generated, P106), drives it in headless Chromium at 1200 × 800 and,
    for every parameter id in params.tsv:

      host -> UI   what a relay does on automation / preset recall: write the
                   stub state's value and fire its valueChangedEvent, then
                   assert the control's DOM moved and is consistent —
                     knob        val-<id> text changed, == aria-valuetext, stem
                                 rotated; restoring the value restores the text
                     fader       val-<id> text changed, input.value == normalised
                     choice knob val-<id> text == choices[idx], aria-valuenow == idx
                     segment     exactly the button data-index == idx pressed
                     select      selectedIndex == idx
                     scale       trigger label changed; restoring restores it
                     toggle      aria-pressed == value
      UI -> host   a real input event on the control — ArrowUp (knobs, choice
                   knobs), ArrowRight (faders), click (toggles, segments, the
                   scale list), selectOption (selects) — and assert the stub
                   state's value changed; for continuous controls
                   sliderDragStarted / sliderDragEnded fired and are paired.

    Hidden tabs are reached by clicking #tab-<page> first; Tempo-only and
    Free-only cells by setting sync_mode through its own state first. Every
    value is restored after each probe.

    LIVENESS: the run passes only with 220 / 220 assertions in EACH direction
    (an id the page never binds has no state → its row fails, it is not skipped).

    Usage:
      node plugins/O-Granulator/tests/ui_binding_probe.js
      node plugins/O-Granulator/tests/ui_binding_probe.js --nc
           negative control: the served page has ONE bindKnob removed
           (s2_drive); the run must FAIL (exit != 0) on exactly that id.
    Exit: 0 = 220 / 220 both ways; n = failures; 77 = Playwright missing —
    NOTHING was verified, which is NOT a pass.

  ==============================================================================
*/

'use strict';

const fs = require('fs');
const path = require('path');

const pluginRoot = path.resolve(__dirname, '..');
const repoRoot   = path.resolve(pluginRoot, '..', '..');
const S = require(path.join(repoRoot, 'scripts', 'serve-ui.js'));

const NC = process.argv.includes('--nc');
const NC_ID = 's2_drive';

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

(async () => {
    const pw = S.resolvePlaywright();
    if (!pw) {
        console.log('ui_binding_probe: Playwright could not be resolved — NOTHING verified (exit 77 is not a pass).');
        process.exit(77);
    }

    const params = readTypes();
    console.log(`== O-Granulator ui_binding_probe${NC ? ' (NEGATIVE CONTROL: bindKnob(' + NC_ID + ') removed)' : ''} ==`);
    console.log(`   params.tsv: ${params.length} ids`);

    const built = S.buildRoot('O-Granulator');
    console.log(`   stub=${built.stubKind} seed=${built.seedFrom}${built.seedError ? ' ERROR ' + built.seedError : ''}`);
    let failures = 0;
    const fail = (msg) => { ++failures; console.log(`  FAIL: ${msg}`); };
    if (!/\+ overrides$/.test(built.seedFrom || '')) fail('stub seed is not "param-dump (220 rows) + overrides" — the probe would run under-seeded');

    if (NC) {
        const p = path.join(built.root, 'index.html');
        const src = fs.readFileSync(p, 'utf8');
        const call = 'KNOB_IDS.forEach((id) => bindKnob(Juce, id));';
        if (!src.includes(call)) { console.log('  NC mutation did not apply'); process.exit(3); }
        fs.writeFileSync(p, src.replace(call, `KNOB_IDS.filter((id) => id !== '${NC_ID}').forEach((id) => bindKnob(Juce, id));`));
    }

    const srv = await S.serve(built.root);
    const browser = await pw.chromium.launch();
    const page = await browser.newPage({ viewport: { width: 1200, height: 800 } });
    const errors = [];
    page.on('pageerror', (e) => errors.push(`pageerror: ${e.message}`));
    page.on('console', (m) => { if (m.type() === 'error') errors.push(`console.error: ${m.text()}`); });

    await page.goto(`http://127.0.0.1:${srv.port}/`);
    // Quiesce: init() ran (readouts written) and the first telemetry tick landed.
    await page.waitForFunction(() => window.__stubStates && Array.isArray(window.__stubNativeCalls)
        && window.__stubNativeCalls.some((c) => c.name === 'getTelemetry')
        && document.getElementById('val-input_gain') && document.getElementById('val-input_gain').textContent !== '—',
        null, { timeout: 15000 });
    await page.waitForTimeout(300);

    // In-page helpers (installed once).
    await page.evaluate(() => {
        const S = window.__stubStates;
        const H = window.__probe = {};
        H.kind = (id, type) => {
            const has = (x) => !!document.getElementById(x);
            if (type === 'bool') return has(`toggle-${id}`) ? 'toggle' : null;
            if (type === 'float') return has(`fader-${id}`) ? 'fader' : has(`knob-${id}`) ? 'knob' : null;
            if (/^s\d_scale$/.test(id)) return has(`scale-trig-${id.slice(0, 2)}`) ? 'scale' : null;
            if (has(`seg-${id}`)) return 'segment';
            if (has(`combo-${id}`)) return 'select';
            if (has(`knob-${id}`)) return 'choiceKnob';
            return null;
        };
        H.state = (id, type) => (type === 'float' ? S.sliders : type === 'bool' ? S.toggles : S.combos).get(id) || null;
        H.el = (id, kind) => document.getElementById(
            kind === 'toggle' ? `toggle-${id}` : kind === 'fader' ? `fader-${id}` : kind === 'segment' ? `seg-${id}`
          : kind === 'select' ? `combo-${id}` : kind === 'scale' ? `scale-trig-${id.slice(0, 2)}` : `knob-${id}`);
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
        H.gesture = (st) => {
            st.__g = { started: 0, ended: 0 };
            st.sliderDragStarted = () => { st.__g.started++; };
            st.sliderDragEnded = () => { st.__g.ended++; };
        };
    });

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
        if (!info.kind) { fail(`${id}: no control element for a ${type}`); continue; }
        if (!info.bound) { fail(`${id}: the page never asked for its ${type} state (not bound) — host->UI`); fail(`${id}: not bound — UI->host`); continue; }

        // ── host -> UI ──────────────────────────────────────────────────────
        const h = await page.evaluate(({ id, type, kind }) => {
            const H = window.__probe;
            const st = H.state(id, type);
            const v0 = H.value(st, type);
            const el = H.el(id, kind);
            let ok = false, why = '';
            if (kind === 'knob' || kind === 'fader') {
                const n0 = H.norm(st), t0 = H.text(id), s0 = kind === 'knob' ? H.stem(id) : null;
                const n1 = n0 < 0.5 ? 0.83 : 0.17;
                H.push(st, type, H.toScaled(st, n1));
                const t1 = H.text(id);
                if (kind === 'knob') {
                    const s1 = H.stem(id), aria = el.getAttribute('aria-valuetext');
                    ok = t1 !== t0 && s1 !== s0 && aria === t1;
                    why = `text ${t0} -> ${t1}, stem ${s1 !== s0 ? 'moved' : 'STILL'}, aria ${aria}`;
                } else {
                    ok = t1 !== t0 && Math.abs(Number(el.value) - n1) < 1e-6;
                    why = `text ${t0} -> ${t1}, input ${el.value} vs ${n1}`;
                }
                H.push(st, type, v0);
                ok = ok && H.text(id) === t0;
                if (H.text(id) !== t0) why += ` (restore gave ${H.text(id)})`;
            } else if (kind === 'toggle') {
                H.push(st, type, !v0);
                const a1 = el.getAttribute('aria-pressed');
                H.push(st, type, v0);
                const a0 = el.getAttribute('aria-pressed');
                ok = a1 === String(!v0) && a0 === String(!!v0);
                why = `aria-pressed ${a1} / ${a0}`;
            } else {
                const choices = (st.properties && st.properties.choices) || [];
                const i1 = (v0 + 1) % Math.max(1, choices.length);
                const before = kind === 'scale' ? document.getElementById(`scale-name-${id.slice(0, 2)}`).textContent : null;
                H.push(st, type, i1);
                if (kind === 'choiceKnob') {
                    ok = H.text(id) === String(choices[i1]) && el.getAttribute('aria-valuenow') === String(i1);
                    why = `val ${H.text(id)} vs ${choices[i1]}`;
                } else if (kind === 'segment') {
                    const pressed = [...el.querySelectorAll('button[data-index]')].filter((b) => b.getAttribute('aria-pressed') === 'true').map((b) => b.dataset.index);
                    ok = pressed.length === 1 && pressed[0] === String(i1);
                    why = `pressed [${pressed}] vs ${i1}`;
                } else if (kind === 'select') {
                    ok = el.selectedIndex === i1 && el.options.length === choices.length;
                    why = `selectedIndex ${el.selectedIndex} vs ${i1} (${el.options.length} options / ${choices.length})`;
                } else if (kind === 'scale') {
                    const after = document.getElementById(`scale-name-${id.slice(0, 2)}`).textContent;
                    H.push(st, type, v0);
                    const back = document.getElementById(`scale-name-${id.slice(0, 2)}`).textContent;
                    ok = after !== before && back === before;
                    why = `label ${before} -> ${after} -> ${back}`;
                }
                H.push(st, type, v0);
            }
            return { ok, why, choices: (st.properties && st.properties.choices || []).length };
        }, { id, type, kind: info.kind });
        if (h.ok) ++hostOk; else fail(`${id} host->UI (${info.kind}): ${h.why}`);

        // ── UI -> host ──────────────────────────────────────────────────────
        if (info.page) await page.click(`#tab-${info.page}`);
        const prevSync = await page.evaluate((want) => {
            const st = window.__stubStates.combos.get('sync_mode');
            const prev = st.getChoiceIndex();
            if (want) { const idx = want === 'tempo' ? 1 : 0; if (prev !== idx) st.setChoiceIndex(idx); }
            return prev;
        }, info.sync);

        const setup = await page.evaluate(({ id, type, kind }) => {
            const H = window.__probe;
            const st = H.state(id, type);
            const v0 = H.value(st, type);
            if (kind === 'knob' || kind === 'fader') { H.push(st, type, H.toScaled(st, 0.5)); H.gesture(st); }
            if (kind === 'choiceKnob') H.push(st, type, 0);
            return { v0, before: H.value(st, type) };
        }, { id, type, kind: info.kind });

        let acted = true, actErr = '';
        try {
            const sel = info.kind === 'toggle' ? `#toggle-${id}` : info.kind === 'fader' ? `#fader-${id}`
                      : info.kind === 'select' ? `#combo-${id}` : info.kind === 'scale' ? `#scale-trig-${id.slice(0, 2)}` : `#knob-${id}`;
            if (info.kind === 'knob' || info.kind === 'choiceKnob') { await page.focus(sel); await page.keyboard.press('ArrowUp'); }
            else if (info.kind === 'fader') { await page.focus(sel); await page.keyboard.press('ArrowRight'); }
            else if (info.kind === 'toggle') await page.click(sel, { timeout: 2000 });
            else if (info.kind === 'segment') {
                const cur = setup.before;
                const target = (cur + 1) % Math.max(1, info.count);
                await page.click(`#seg-${id} button[data-index="${target}"]`, { timeout: 2000 });
            } else if (info.kind === 'select') {
                await page.selectOption(sel, String((setup.before + 1) % Math.max(1, info.count)));
            } else if (info.kind === 'scale') {
                await page.click(sel, { timeout: 2000 });
                const target = setup.before === 5 ? 6 : 5;
                await page.click(`#scale-list .it[data-idx="${target}"]`, { timeout: 2000 });
            }
        } catch (e) { acted = false; actErr = e.message.split('\n')[0]; }

        const u = await page.evaluate(({ id, type, kind, setup }) => {
            const H = window.__probe;
            const st = H.state(id, type);
            const after = H.value(st, type);
            const g = st.__g || null;
            let ok = after !== setup.before;
            let why = `value ${setup.before} -> ${after}`;
            if (kind === 'knob' || kind === 'fader') {
                ok = ok && g && g.started >= 1 && g.started === g.ended;
                why += `, gesture ${g ? g.started + '/' + g.ended : 'none'}`;
            }
            // restore (value + the stub's no-op gesture fns)
            if (g) { delete st.__g; st.sliderDragStarted = () => {}; st.sliderDragEnded = () => {}; }
            H.push(st, type, setup.v0);
            return { ok, why };
        }, { id, type, kind: info.kind, setup });
        await page.evaluate((prev) => {
            const st = window.__stubStates.combos.get('sync_mode');
            if (st.getChoiceIndex() !== prev) st.setChoiceIndex(prev);
        }, prevSync);
        await page.keyboard.press('Escape').catch(() => {});
        if (acted && u.ok) ++uiOk; else fail(`${id} UI->host (${info.kind}): ${acted ? u.why : 'action failed: ' + actErr}`);
    }

    console.log(`   kinds: ${Object.entries(kinds).map(([k, n]) => `${k} ${n}`).join(', ')}`);
    console.log(`   page errors: ${errors.length}${errors.length ? ' — ' + errors.slice(0, 3).join(' | ') : ''}`);
    if (errors.length) fail(`${errors.length} page / console error(s)`);
    console.log(`\n   host -> UI ${hostOk} / ${params.length}, UI -> host ${uiOk} / ${params.length}`);
    if (params.length !== 220) fail(`params.tsv has ${params.length} ids, expected 220`);
    if (hostOk !== 220 || uiOk !== 220) { if (failures === 0) fail('liveness: not 220 / 220 both ways'); }

    await browser.close();
    await srv.close();
    fs.rmSync(built.root, { recursive: true, force: true });
    console.log(failures === 0 ? 'ALL PASS (220 / 220 both ways)' : `${failures} FAILED`);
    process.exit(Math.min(failures, 76));
})().catch((e) => { console.error(e); process.exit(70); });
