/*
   This file is part of O-MicrotonalSampler, an Ouaricon Audio plugin.
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
/* ==============================================================================
 * sampler-app.js
 * O-MicrotonalSampler — Phase 3.1 entry point.
 *
 * Wires:
 *   - 9 APVTS sliders ↔ DOM range inputs via Juce.getSliderState() (relay
 *     identifiers must match the C++ WebSliderRelay names exactly).
 *     (v1.7.0 added 'expression' — CC 11 dynamics; v1.22.0 added
 *     'dynamic_range' — CC-crossfade pp→ff dB span.)
 *   - Tab activation (Sample Map / Tuning / About).
 *   - Lazy mount of the TuningPanel on first Tuning-tab activation, plus a
 *     read-only interval-input → span swap shim per RESEARCH §RQ3-1.
 *   - sampleMapUpdated event listener (initial pull on load + push events
 *     from C++ on every map atomic-store).
 *   - Tuning-state readout poll on editor open + Tuning-tab activation
 *     (RP3-3 — no background interval).
 *
 * Phases 3.2–3.5 will extend this with grid rendering, drag-drop, loop
 * editor, knob styling. The 3.1 surface is intentionally minimal.
 * ============================================================================== */

// The check_native_interop.js script (carried verbatim from O-Bells, loaded
// in <head>) populates window.__JUCE__ before this module runs.
import * as Juce from './juce/index.js';

// v1.13.0 (ARCH-02): WKWebView drag-drop content-streaming — shared module.
// PluginEditor::getResource serves this at /js/modules/webview-drop-streaming.js.
import { bindWebViewFileDrop } from './modules/webview-drop-streaming.js';

// ════════════════════════════════════════════════════════════════════════════
// v1.24.0 — CANON V2 i18n BLOCK, verbatim from scripts/i18n-canon.js.
//
// check-i18n assertion 6 byte-compares the region from `let uiLanguage` to the
// close of initI18n() against that file after comments are stripped and
// whitespace collapsed. DO NOT EDIT INSIDE IT — every plugin in this repo
// hand-copies its UI runtime, and this gate is the only thing that stops 43
// copies drifting. Plugin-specific helpers go BELOW the block, never inside it.
//
// initI18n() is called LAST, at the end of the DOMContentLoaded handler at the
// bottom of this file, so the first sweep sees the control strip, the technique
// bar and the trim panel — none of which exist in index.html.
// ════════════════════════════════════════════════════════════════════════════
import { LANGUAGES, I18N, LABELS, TIP_BINDINGS, tr } from './i18n.js';

let uiLanguage = 'en';
let getUiLanguageNative = null;
let setUiLanguageNative = null;

// LABELS first, I18N as the fallback: a control whose tooltip title already IS
// its label carries one key, not two copies of the same string.
function trLabel(key, lang, vars) {
    const entry = (typeof LABELS === 'object' && LABELS && LABELS[key]) || I18N[key];
    if (!entry) { console.warn(`i18n: missing label key ${key}`); return key; }
    const s = entry[lang] || entry.en;
    const resolve = (v) => {
        const nested = (typeof LABELS === 'object' && LABELS && LABELS[v]) || I18N[v];
        return nested ? String((nested[lang] || nested.en).t) : String(v);
    };
    return vars
        ? String(s.t).replace(/\{(\w+)\}/g, (m, n) => (n in vars ? resolve(vars[n]) : m))
        : String(s.t);
}

function applyLabel(el) {
    const key = el.dataset.i18n;
    if (!key) return;
    let vars = null;
    try { vars = el.dataset.i18nVars ? JSON.parse(el.dataset.i18nVars) : null; }
    catch (e) { console.warn(`i18n: bad vars on ${key}`); }
    const s = trLabel(key, uiLanguage, vars);
    el.dataset.label = s;
    el.textContent   = s;
}

function applyI18nAttributes(el) {
    const pairs = [['i18nAria', 'aria-label'], ['i18nPlaceholder', 'placeholder'], ['i18nAlt', 'alt']];
    for (const [prop, attr] of pairs) {
        const key = el.dataset[prop];
        if (key) el.setAttribute(attr, trLabel(key, uiLanguage, null));
    }
}

function setLabel(el, key, vars) {
    if (!el) return;
    el.dataset.i18n = key;
    if (vars) el.dataset.i18nVars = JSON.stringify(vars); else delete el.dataset.i18nVars;
    applyLabel(el);
}

function applyI18n(lang) {
    uiLanguage = LANGUAGES.includes(lang) ? lang : 'en';
    // <html lang> follows the selector: screen readers pick the French voice,
    // and CSS hyphens:auto / quotes resolve in the page's actual language.
    document.documentElement.lang = uiLanguage;
    for (const [selector, key, wrapper, vars] of TIP_BINDINGS) {
        const el = document.querySelector(selector);
        if (!el) { console.warn(`i18n: tip target not found: ${selector}`); continue; }
        const target = wrapper ? (el.closest(wrapper) || el) : el;
        const s = tr(key, uiLanguage, vars);
        target.setAttribute('data-tip-title', s.t);
        target.setAttribute('data-tip', s.b);
    }
    for (const el of document.querySelectorAll('[data-i18n]')) applyLabel(el);
    for (const el of document.querySelectorAll('[data-i18n-aria],[data-i18n-placeholder],[data-i18n-alt]'))
        applyI18nAttributes(el);
    const sel = document.getElementById('lang-select');
    if (sel && sel.value !== uiLanguage) sel.value = uiLanguage;
}

// Exposed so a clamp gate can drive the language without teaching the ui-stub a
// promise contract: page.evaluate((l) => window.__setLanguage(l), 'fr').
window.__setLanguage = applyI18n;
// Exposed for the same reason, and so a sibling module can write a localized
// label without app.js having to export anything — O-Bitrot's controller is an
// inline <script type="module">, where an export declaration has nowhere to go.
window.__setLabel = setLabel;
window.__reapplyI18n = () => applyI18n(uiLanguage);

function initI18n() {
    try {
        getUiLanguageNative = Juce.getNativeFunction('getUiLanguage');
        setUiLanguageNative = Juce.getNativeFunction('setUiLanguage');
    } catch (e) {
        console.warn('Language preference not available, session-only:', e);
    }

    // Paint the default SYNCHRONOUSLY first. Never blank, never a flash.
    try { applyI18n('en'); } catch (e) { console.error('i18n init failed:', e); }

    if (getUiLanguageNative) {
        getUiLanguageNative()
            .then((code) => applyI18n(code))
            .catch((e) => console.warn('Could not read language preference:', e));
    }

    const sel = document.getElementById('lang-select');
    if (sel) sel.addEventListener('change', (e) => {
        applyI18n(e.target.value);
        if (setUiLanguageNative) setUiLanguageNative(uiLanguage).catch(() => {});
    });
}

// ── plugin-side i18n helpers (OUTSIDE the canon region) ────────────────────
//
// A composed or JS-written string that cannot declare its key in markup lives
// in I18N with an EMPTY body and is resolved through trLabel(). A trLabel()
// call is none of the four reference shapes check-i18n assertion 15 collects
// (markup attribute, literal setLabel key, literal `.dataset.i18n* =` write,
// innerHTML-injected key), so a LABELS key reached only this way would report
// DEAD. I18N keys sit outside that sweep, and assertion 2 accepts a body-less
// I18N entry with no TIP_BINDINGS. Same shape O-Comp adopted in Stage K3 for
// its canvas strings, adopted as the standard by the K4 addendum.
function trComposed(key, vars) {
    return trLabel(key, uiLanguage, vars || null);
}

// Every string this page composes at RENDER time — the grid's accessible cell
// names, the technique tab strip, the trim chip's accessible name — is written
// by its own renderer, not by applyI18n's [data-i18n] sweep, so the sweep alone
// leaves them stranded in the previous language the instant the selector fires
// (the Stage B bug, generalised). Re-running the renderers is the one
// re-render path; there is no second copy of the copy.
function refreshComposedUi() {
    try {
        if (lastSampleMapSnapshot) renderGrid(lastSampleMapSnapshot);
        renderTechniqueBar();
        renderTrimPanel();
        renderVariantTabStrip();
        updateResetButtonState(editorState.snap);
    } catch (e) {
        console.warn('[sampler-app] refreshComposedUi failed', e);
    }
}

// The canon publishes window.__setLanguage = applyI18n and the label gate drives
// the page through it. Wrapping it here — rather than editing the canon — keeps
// assertion 6 byte-exact while still refreshing the composed strings. The
// selector's own change event is covered separately, in initSettingsPopover().
{
    const canonApply = window.__setLanguage;
    window.__setLanguage = (lang) => { canonApply(lang); refreshComposedUi(); };
}

// js/tuning-panel.js mounts LAZILY, on the first Tuning-tab activation — long
// after initI18n() has swept the page — and rebuilds its interval list, matrix,
// rotation table, library list and generator rows from markup templates whose
// captions carry data-i18n keys. Re-running the sweep is how those keys are
// applied; the panel calls this after every injection. Published rather than
// imported because tuning-panel.js is a plugin-owned copy of a module file and
// must keep working when app.js exports nothing (the same reason the canon
// publishes window.__setLabel).
window.__omsRefreshI18n = () => {
    try { applyI18n(uiLanguage); }
    catch (e) { console.warn('[sampler-app] i18n sweep failed', e); }
};

// ════════════════════════════════════════════════════════════════════════════
// v1.25.0 — HOVER-HELP RENDERER (Stage M)
// ════════════════════════════════════════════════════════════════════════════
//
// WHY THIS EXISTS AT ALL. Canon v2's applyI18n() writes data-tip-title and
// data-tip ATTRIBUTES onto the anchors named in TIP_BINDINGS and stops there.
// The code that reads those attributes and paints a surface is per-plugin and
// this page had none of it at v1.24.0 — no #tooltip element, no tooltip rule,
// no hover handler. Authoring 21 bodies and binding them WITHOUT this function
// ships 21 invisible strings past three green gates: check-i18n reads the table
// statically, check-ui-labels has no tooltip awareness whatsoever, and
// boot-all-uis counts aria-label and title and never data-tip. That is why
// tests/ui_tip_render_check.js exists beside this.
//
// Ported from plugins/O-simpleFM/Source/ui/public/js/app.js:384-462 — delegated
// and cursor-following, ~90 lines against O-Tapestop's ~180 measure-then-pin
// engine, which exists to serve a flip-above/below design with an arrow that
// this page does not have.
//
// EIGHT PROPERTIES, each load-bearing and each with a scar behind it:
//
//   1. DELEGATED on document, never querySelectorAll('[data-tip]') at setup.
//      No anchor carries data-tip until applyI18n() has run, so a setup-time
//      query binds NOTHING and fails silently.
//   2. pointerover / pointerout / focusin / focusout — they BUBBLE.
//      pointerenter / pointerleave and focus / blur do not.
//   3. pointerout ignores a move between two descendants of the SAME anchor.
//      Every knob cell here holds an SVG, a caption, a readout and a hidden
//      range input; without this the tip flickers off and on at each boundary.
//   4. createElement + textContent, NEVER innerHTML. Localized copy must not
//      reach a markup path — check-i18n assertion 9 already forbids an angle
//      bracket in an i18n.js string literal and this is the other half of it.
//   5. Clamped on ALL FOUR edges with an 8px margin, AFTER the flip. The clamp
//      is applied after rather than instead of the flip so a tip that fits on
//      neither side of the cursor still lands fully on screen; O-Bass measured
//      at 420x320 that a flipped placement needs clamping again.
//   6. pointer-events: none on the surface (CSS), or it steals the hover.
//   7. Escape hides it, and so does any pointerdown.
//   8. THE FOCUS ARM IS LATCHED TO THE KEYBOARD. A mouse click on a <button>
//      focuses it, so the O-simpleFM reference's unconditional focusin rule
//      leaves a tip parked on screen after every click — measured on
//      O-Emulator at 4669-5280 px2 of overlap across nine M1 plugins, sitting
//      on top of the very popover the click had just opened.
//      :focus-visible is deliberately NOT the discriminator: Chromium reports
//      it false for a programmatic .focus() following a click, so a gate
//      driving focus directly would measure "no tip" and record that as
//      correct — a false pass built into the fix. An explicit last-input-device
//      latch is the same rule and is drivable with real events.
//
// A ninth, specific to this page: a knob drag is started on pointerdown and
// takes a pointer capture, and a drag that travels out of its own cell must not
// open the neighbour's tip mid-gesture. knobDrag.active is the guard. It is
// declared BELOW this function; that is safe because nothing here runs until
// setupTooltips() is called from the DOMContentLoaded handler at the foot of
// the file, long after every top-level binding has been evaluated
// (pattern_module_toplevel_init_tdz is about the CALL site, not the closure).
function setupTooltips() {
    const tip = document.getElementById('tooltip');
    if (!tip) {
        console.warn('[sampler-app] #tooltip surface missing — hover-help unavailable');
        return;
    }

    const MARGIN = 8;
    let active = null;
    let lastInputWasPointer = false;

    const position = (x, y) => {
        const r  = tip.getBoundingClientRect();
        const vw = window.innerWidth;
        const vh = window.innerHeight;
        let nx = x + 14;
        let ny = y + 16;
        // Flip to the other side of the cursor when the natural side overflows.
        if (nx + r.width  > vw - MARGIN) nx = x - r.width  - 14;
        if (ny + r.height > vh - MARGIN) ny = y - r.height - 12;
        // Then clamp, unconditionally, on all four edges. Math.max on the upper
        // bound keeps the arithmetic sane if a tip ever exceeds the frame.
        nx = Math.min(Math.max(MARGIN, nx), Math.max(MARGIN, vw - r.width  - MARGIN));
        ny = Math.min(Math.max(MARGIN, ny), Math.max(MARGIN, vh - r.height - MARGIN));
        tip.style.left = `${nx}px`;
        tip.style.top  = `${ny}px`;
    };

    const show = (el, x, y) => {
        const title = el.getAttribute('data-tip-title');
        const body  = el.getAttribute('data-tip');
        if (!title && !body) return;
        tip.textContent = '';
        if (title) {
            const t = document.createElement('span');
            t.className = 'tip-title';
            t.textContent = title;
            tip.appendChild(t);
        }
        if (body) tip.appendChild(document.createTextNode(body));
        tip.classList.add('show');
        tip.setAttribute('aria-hidden', 'false');
        position(x, y);
    };

    const hide = () => {
        tip.classList.remove('show');
        tip.setAttribute('aria-hidden', 'true');
        active = null;
    };

    const anchorOf = (t) => (t && t.closest ? t.closest('[data-tip]') : null);

    document.addEventListener('pointerover', (e) => {
        if (knobDrag.active) return;            // a drag is in flight — see above
        const el = anchorOf(e.target);
        if (!el || el === active) return;
        active = el;
        show(el, e.clientX, e.clientY);
    });
    document.addEventListener('pointermove', (e) => {
        if (active && anchorOf(e.target) === active) position(e.clientX, e.clientY);
    });
    document.addEventListener('pointerout', (e) => {
        if (!active) return;
        if (anchorOf(e.relatedTarget) === active) return;   // same anchor, child boundary
        hide();
    });
    document.addEventListener('pointerdown', () => { lastInputWasPointer = true; hide(); });

    document.addEventListener('focusin', (e) => {
        if (lastInputWasPointer) return;        // property 8
        const el = anchorOf(e.target);
        if (!el) return;
        active = el;
        const r = el.getBoundingClientRect();
        show(el, r.left + r.width / 2, r.bottom);
    });
    document.addEventListener('focusout', hide);

    // One keydown listener, two jobs: any key at all means the keyboard is
    // driving again, which releases the latch; Escape additionally hides.
    document.addEventListener('keydown', (e) => {
        lastInputWasPointer = false;
        if (e.key === 'Escape') hide();
    });
}

// v1.16.10 (MEDIUM-05): guarded-number coercion for parsed JSON snapshots.
// Replaces the `Number.isFinite(x) ? x : default` ternary pattern. Sites that
// use Number.isFinite as a control-flow guard (e.g. `if (!Number.isFinite(x)) return`)
// keep the bare check — different intent.
const num = (v, fallback = 0) => (Number.isFinite(v) ? v : fallback);

// v1.16.10 (MEDIUM-06): standardised native-fn invoker. Returns the resolved
// value on success, undefined when the host is missing or the call throws
// (matching the prior silent-catch behaviour). Sites that need a user-visible
// failure path (e.g. presets — toast on catch) keep the explicit try/catch.
// Sites that subscribe to backend events (window.__JUCE__.backend.addEventListener)
// stay direct — those are not native-fn invocations.
async function invokeNative(name, ...args) {
    if (!window.__JUCE__) return undefined;
    try { return await Juce.getNativeFunction(name)(...args); }
    catch (err) { console.warn(`[sampler-app] ${name} failed`, err); return undefined; }
}

// v1.16.11 (MEDIUM-04): unified modal lifecycle. Centralises the
//   addEventListener / cleanup / removeEventListener / Esc-Enter
// boilerplate that used to recur across 7 dialogs (folder-load-options,
// embed-size-confirm, per-cell-merge, confirm, diagnostic, missing-folder,
// rr-confirm).
//
// Usage:
//   const result = await bindModal(dialog, [
//       [yesBtn, () => 'yes'],
//       [noBtn,  () => 'no'],
//   ], { keys: { Escape: noBtn, Enter: yesBtn }, focus: yesBtn });
//
// `buttons` is an array of [HTMLElement, handler] tuples. Clicking any bound
// button (or pressing its key alias) resolves the promise with the handler's
// return value. handler may be sync or async; exceptions are swallowed and
// logged with the same `[sampler-app]` tag invokeNative uses.
//
// `opts.keys` maps key names to a button reference. Each entry may be either
// a static HTMLElement or a `() => HTMLElement` callback (for cases like
// per-cell-merge where Enter's target depends on dialog state). Missing keys
// (e.g. diagnostic-dialog has no Enter binding) are simply ignored. The
// keydown listener is attached on the capture phase so the host's own
// Esc/Enter handlers don't intercept first — preserves prior behaviour.
//
// `opts.focus` is the element to focus after dialog.hidden = false.
//
// `opts.onClose` runs once after all bindModal-owned listeners are removed
// and before the promise resolves. Used by callers that registered their
// own non-dismissing listeners (folder-load-options live-preview handlers,
// diagnostic-dialog copy button) so all teardown happens in one path.
function bindModal(dialog, buttons, opts = {}) {
    return new Promise((resolve) => {
        let closed = false;
        const cleanups = [];

        const close = (result) => {
            if (closed) return;
            closed = true;
            dialog.hidden = true;
            for (const c of cleanups) c();
            if (typeof opts.onClose === 'function') {
                try { opts.onClose(); }
                catch (err) { console.warn('[sampler-app] modal onClose failed', err); }
            }
            resolve(result);
        };

        for (const [btn, handler] of buttons) {
            const wrapped = async () => {
                let result;
                try { result = await handler(); }
                catch (err) {
                    console.warn('[sampler-app] modal button handler failed', err);
                    result = undefined;
                }
                close(result);
            };
            btn.addEventListener('click', wrapped);
            cleanups.push(() => btn.removeEventListener('click', wrapped));
        }

        const resolveKeyTarget = (entry) =>
            (typeof entry === 'function') ? entry() : entry;

        // v1.23.7 (IN-04): stopPropagation once a key is claimed — this
        // listener runs in the capture phase, so without it document-level
        // bubble handlers (e.g. the loop editor's Esc-to-close) also fire
        // and dismiss UI behind the modal.
        const onKey = (e) => {
            if (e.key === 'Escape') {
                const target = resolveKeyTarget(opts.keys?.Escape);
                if (target) { e.preventDefault(); e.stopPropagation(); target.click(); }
            } else if (e.key === 'Enter') {
                const target = resolveKeyTarget(opts.keys?.Enter);
                if (target) { e.preventDefault(); e.stopPropagation(); target.click(); }
            }
        };
        document.addEventListener('keydown', onKey, true);
        cleanups.push(() => document.removeEventListener('keydown', onKey, true));

        dialog.hidden = false;
        if (opts.focus) opts.focus.focus();
    });
}

// ============================================================================
// Slider relay binding
// ============================================================================
//
// Each entry maps the DOM element id to its WebSliderRelay/APVTS parameter
// id. The relay handle (Juce.getSliderState) is bidirectional: setting
// .setNormalisedValue() pushes to the C++ APVTS; valueChangedEvent fires
// when automation / preset / DAW changes the parameter so we update the
// DOM control to match.
// v1.16.8 (HIGH-06): SLIDER_BINDINGS is the single source of truth for the
// 9 control-strip knobs — order, label, and optional tooltip. Knob DOM is
// rendered from this array by renderControlStrip() before bindOneKnob runs.
const SLIDER_BINDINGS = [
    { domId: 'ctrl-attack',              relayId: 'attack',              label: 'Attack' },
    { domId: 'ctrl-decay',               relayId: 'decay',               label: 'Decay' },
    { domId: 'ctrl-sustain',             relayId: 'sustain',             label: 'Sustain' },
    { domId: 'ctrl-release',             relayId: 'release',             label: 'Release' },
    { domId: 'ctrl-polyphony',           relayId: 'polyphony',           label: 'Poly' },
    { domId: 'ctrl-velocity-crossfade',  relayId: 'velocity_crossfade',  label: 'Vel-XF' },
    // v1.24.0: the `tooltip` field is gone. It was interpolated into a native
    // title= inside renderControlStrip's innerHTML template — invisible to
    // check-i18n assertion 11 (which reads index.html and `.title =` writes,
    // and this was neither) and to boot-all-uis as anything but a count.
    // Contract §4 deletes a native title rather than localizing it; the two
    // strings that carried real help move VERBATIM to data-i18n-aria, applied
    // by literal dataset writes in labelControlStrip() below.
    { domId: 'ctrl-expression',          relayId: 'expression',          label: 'Expr' },      // v1.7.0
    { domId: 'ctrl-dynamic-range',       relayId: 'dynamic_range',       label: 'Dyn Rng' },   // v1.22.0
    { domId: 'ctrl-output-gain',         relayId: 'output_gain',         label: 'Out Gain' }
];

// Phase 3.5 — display formatting per parameter. Maps relayId → {suffix,
// format(scaledValue)}.
//
// v1.23.7 (WR-01): the readout is computed from the relay's SCALED value
// (SliderState.getScaledValue() — JUCE pushes the real NormalisableRange
// start/end/skew via propertiesChanged), so the JS side no longer hard-codes
// parameter ranges. The old linear min/max map here claimed attack/decay/
// release were 0.001–5 s linear while the C++ range is 0–10 s skew 0.5 (label
// read up to 2× low), and claimed polyphony 1–32 vs the real 1–16.
const KNOB_FORMATS = {
    'attack':              { suffix: ' s',
                             format: v => v < 0.1 ? v.toFixed(3) : v.toFixed(2) },
    'decay':               { suffix: ' s',
                             format: v => v < 0.1 ? v.toFixed(3) : v.toFixed(2) },
    'sustain':             { suffix: '',
                             format: v => v.toFixed(2) },
    'release':             { suffix: ' s',
                             format: v => v < 0.1 ? v.toFixed(3) : v.toFixed(2) },
    'polyphony':           { suffix: '',
                             format: v => Math.round(v).toString() },
    'velocity_crossfade':  { suffix: '',
                             format: v => v.toFixed(2) },
    // v1.7.0: expression displays as 0-100 % (scaled param is 0..1; squared
    // curve handled C++ side).
    'expression':          { suffix: ' %',
                             format: v => Math.round(v * 100).toString() },
    // v1.22.0: dynamic range in dB (linear NormalisableRange 0..40 C++ side).
    'dynamic_range':       { suffix: ' dB',
                             format: v => v.toFixed(1) },
    'output_gain':         { suffix: ' dB',
                             format: v => (v >= 0 ? '+' : '') + v.toFixed(1) },
};

// v1.23.7 (WR-04): relayId → default normalised value, pulled once at boot
// from the C++ side (param->getDefaultValue()) via getParameterDefaults.
// Used by the knob double-click reset so it lands on the real APVTS default
// instead of a blanket mid-range 0.5 (which, for the skewed ADSR ranges,
// denormalised to ~2.5 s).
let paramDefaults = {};

async function pullParameterDefaults() {
    if (!window.__JUCE__ || !window.__JUCE__.backend) return;
    try {
        const fn = Juce.getNativeFunction('getParameterDefaults');
        const raw = await fn();
        const obj = (typeof raw === 'string') ? JSON.parse(raw) : raw;
        if (obj && typeof obj === 'object') paramDefaults = obj;
    } catch (err) {
        console.warn('[sampler-app] pullParameterDefaults failed', err);
    }
}

// SVG arc geometry (matches O-Bells: 270 degree sweep over r=18). The arc
// is drawn via stroke-dasharray + stroke-dashoffset on a circle; the
// transform: rotate(-135deg) on the SVG places the gap at the bottom.
const KNOB_RADIUS = 18;
const KNOB_ARC_LENGTH = 2 * Math.PI * KNOB_RADIUS * 0.75;

// Global drag state for the SVG knobs (shared across all 7 — only one drag
// at a time, matches O-Bells convention so we don't fight pointer-capture).
const knobDrag = {
    active: false,
    knob: null,            // .ouaricon-knob root element
    state: null,           // Juce slider state
    input: null,           // hidden <input type="range">
    fmt: null,             // KNOB_FORMATS entry
    valueEl: null,         // .ouaricon-knob-value span
    vineEl: null,          // .knob-vine SVG circle
    startY: 0,
    startNorm: 0,
    pointerId: -1,
};

function knobUpdateVisual(vineEl, valueEl, fmt, norm, state) {
    // Arc sweep
    if (vineEl) {
        const offset = KNOB_ARC_LENGTH * (1 - Math.max(0, Math.min(1, norm)));
        vineEl.style.strokeDasharray  = KNOB_ARC_LENGTH.toFixed(2);
        vineEl.style.strokeDashoffset = offset.toFixed(2);
    }
    // Numeric readout — the real (scaled) value comes from the relay itself,
    // which applies the C++ NormalisableRange incl. skew (WR-01). See the
    // KNOB_FORMATS note.
    if (valueEl && fmt && state) {
        valueEl.textContent = fmt.format(state.getScaledValue()) + fmt.suffix;
    }
}

function bindOneKnob({ domId, relayId }) {
    const input = document.getElementById(domId);
    if (!input) {
        console.warn(`[sampler-app] DOM element #${domId} not found`);
        return;
    }

    const knob = input.closest('.ouaricon-knob');
    if (!knob) {
        console.warn(`[sampler-app] no .ouaricon-knob wrapper for #${domId}`);
        return;
    }

    const vineEl  = knob.querySelector('.knob-vine');
    const valueEl = knob.querySelector('.ouaricon-knob-value');
    const fmt = KNOB_FORMATS[relayId] || null;

    let state = null;
    try {
        state = Juce.getSliderState(relayId);
    } catch (e) {
        console.error(`[sampler-app] Failed to bind slider ${relayId}:`, e);
        return;
    }

    // Initial pull
    const init = state.getNormalisedValue();
    if (typeof init === 'number') {
        input.value = init;
        knobUpdateVisual(vineEl, valueEl, fmt, init, state);
    }

    // C++ -> DOM (automation, preset load, DAW change)
    state.valueChangedEvent.addListener(() => {
        const v = state.getNormalisedValue();
        if (typeof v === 'number') {
            input.value = v;
            knobUpdateVisual(vineEl, valueEl, fmt, v, state);
        }
    });

    // The real range (start/end/skew) arrives asynchronously from C++ via
    // propertiesChanged — refresh the visual when it lands, otherwise the
    // boot-time readout is computed against the default 0..1 linear
    // properties (WR-01).
    state.propertiesChangedEvent.addListener(() => {
        const v = state.getNormalisedValue();
        if (typeof v === 'number' && Number.isFinite(v)) {
            input.value = v;
            knobUpdateVisual(vineEl, valueEl, fmt, v, state);
        }
    });

    // DOM -> C++ (defensive — input only changes if external code sets
    // .value and dispatches event; the SVG drag updates state directly).
    input.addEventListener('input', () => {
        const v = parseFloat(input.value);
        if (!Number.isNaN(v)) {
            state.setNormalisedValue(v);
            knobUpdateVisual(vineEl, valueEl, fmt, v, state);
        }
    });

    // Pointer drag — relative-vertical 200 px = full sweep (matches the
    // O-Reed sensitivity from agent memory; feels natural for 44 px knobs).
    knob.addEventListener('pointerdown', (e) => {
        if (knobDrag.active) return;
        knobDrag.active     = true;
        knobDrag.knob       = knob;
        knobDrag.state      = state;
        knobDrag.input      = input;
        knobDrag.fmt        = fmt;
        knobDrag.valueEl    = valueEl;
        knobDrag.vineEl     = vineEl;
        knobDrag.startY     = e.clientY;
        knobDrag.startNorm  = state.getNormalisedValue();
        knobDrag.pointerId  = e.pointerId;
        knob.classList.add('dragging');
        try { knob.setPointerCapture(e.pointerId); } catch (_) {}
        state.sliderDragStarted();
        e.preventDefault();
    });

    // Wheel — 2 % per tick (matches O-Bells fxKnob convention).
    // v1.23.7 (WR-05): wrapped in a sliderDragStarted/Ended gesture — opened
    // on the first tick, closed after a short idle — so hosts in
    // automation-write mode record wheel tweaks the same way as drags.
    let wheelGestureTimer = null;
    knob.addEventListener('wheel', (e) => {
        e.preventDefault();
        if (wheelGestureTimer === null) state.sliderDragStarted();
        else clearTimeout(wheelGestureTimer);
        wheelGestureTimer = setTimeout(() => {
            wheelGestureTimer = null;
            state.sliderDragEnded();
        }, 250);
        const cur = state.getNormalisedValue();
        const delta = e.deltaY < 0 ? 0.02 : -0.02;
        const next = Math.max(0, Math.min(1, cur + delta));
        state.setNormalisedValue(next);
        input.value = next;
        knobUpdateVisual(vineEl, valueEl, fmt, next, state);
    }, { passive: false });

    // Double-click resets to the parameter's APVTS default (WR-04). Defaults
    // are pulled once at boot via getParameterDefaults (normalised 0..1,
    // straight from param->getDefaultValue()); mid-range only as a fallback
    // if that pull failed or hasn't landed yet.
    knob.addEventListener('dblclick', (e) => {
        e.preventDefault();
        const def = (typeof paramDefaults[relayId] === 'number')
            ? Math.max(0, Math.min(1, paramDefaults[relayId]))
            : 0.5;
        state.sliderDragStarted();
        state.setNormalisedValue(def);
        state.sliderDragEnded();
        input.value = def;
        knobUpdateVisual(vineEl, valueEl, fmt, def, state);
    });
}

function bindKnobGlobalDrag() {
    document.addEventListener('pointermove', (e) => {
        if (!knobDrag.active || e.pointerId !== knobDrag.pointerId) return;
        const deltaY = knobDrag.startY - e.clientY;          // up = increase
        const sensitivity = 1 / 200;                          // 200 px = full sweep
        const next = Math.max(0, Math.min(1, knobDrag.startNorm + deltaY * sensitivity));
        knobDrag.state.setNormalisedValue(next);
        if (knobDrag.input) knobDrag.input.value = next;
        knobUpdateVisual(knobDrag.vineEl, knobDrag.valueEl, knobDrag.fmt, next, knobDrag.state);
    });

    const endDrag = (e) => {
        if (!knobDrag.active) return;
        if (e && e.pointerId !== knobDrag.pointerId) return;
        try {
            if (knobDrag.knob && knobDrag.knob.hasPointerCapture && knobDrag.knob.hasPointerCapture(knobDrag.pointerId)) {
                knobDrag.knob.releasePointerCapture(knobDrag.pointerId);
            }
        } catch (_) {}
        if (knobDrag.state) knobDrag.state.sliderDragEnded();
        if (knobDrag.knob) knobDrag.knob.classList.remove('dragging');
        knobDrag.active = false;
        knobDrag.knob = null;
        knobDrag.state = null;
        knobDrag.input = null;
        knobDrag.fmt = null;
        knobDrag.valueEl = null;
        knobDrag.vineEl = null;
        knobDrag.pointerId = -1;
    };
    document.addEventListener('pointerup',     endDrag);
    document.addEventListener('pointercancel', endDrag);
}

// v1.16.8 (HIGH-06): render the 8 knob blocks from SLIDER_BINDINGS into
// <footer id="control-strip">. Replaces 8 nearly-identical static HTML
// blocks. Run unconditionally at boot — the DOM should exist even when
// the plugin host isn't available (matches pre-refactor behaviour where
// the static knobs always rendered).
function renderControlStrip() {
    const strip = document.getElementById('control-strip');
    if (!strip) {
        console.warn('[sampler-app] #control-strip not found — knobs cannot render');
        return;
    }
    const knobsHtml = SLIDER_BINDINGS.map(b => {
        return `
      <div class="ouaricon-knob" data-knob-id="${b.domId}">
        <div class="ouaricon-knob-visual">
          <svg viewBox="0 0 44 44">
            <circle class="knob-track" cx="22" cy="22" r="18"/>
            <circle class="knob-vine"  cx="22" cy="22" r="18"/>
          </svg>
        </div>
        <label class="ouaricon-knob-label" for="${b.domId}">${b.label}</label>
        <span class="ouaricon-knob-value"></span>
        <input type="range" id="${b.domId}" min="0" max="1" step="0.001" />
      </div>`;
    }).join('');

    // v1.21.0: Dynamics Mode segmented toggle (choice param "dynamics_mode").
    // Sits in the control strip next to Expression. Bound to the WebComboBox
    // relay in bindDynamicsMode(). CC Crossfade is the default — see
    // PluginProcessor::createParameterLayout.
    // v1.24.0: the native title= is DELETED (contract §4) and its own v1.23.10
    // wording moved verbatim to a keyed accessible name. The keys here are
    // LITERAL inside the template — check-i18n assertion 15 reads keys out of
    // markup a module injects, so a literal `data-i18n-aria="..."` is a live
    // reference while `data-i18n="${b.key}"` from the mapped template above
    // would be a computed one and report DEAD. That is why the nine knob
    // captions are keyed by literal setLabel() calls instead (see below).
    //
    // Velocity / CC Crossfade are NOT keyed: both are byte-identical to the
    // dynamics_mode AudioParameterChoice option strings, so the page and the
    // host automation lane must keep saying the same word (D-01 arm 1).
    const dynamicsHtml = `
      <div class="dynamics-mode-control"
           aria-label="Dynamics Mode — how MIDI CC 11 shapes dynamics. Velocity: note-on velocity picks the layer, CC 11 is a post-mix volume trim (v1.20 behaviour). CC Crossfade: CC 11 morphs across all velocity layers mid-note (timbre + loudness, like pro sustain patches)."
           data-i18n-aria="aria.dynamicsMode">
        <span class="dynamics-mode-title" data-i18n="label.dynamics">Dynamics</span>
        <div class="seg-toggle" id="dynamics-mode-seg" role="group"
             aria-label="Dynamics Mode" data-i18n-aria="aria.dynamicsModeShort">
          <button type="button" class="seg-btn" data-idx="0">Velocity</button>
          <button type="button" class="seg-btn" data-idx="1">CC&nbsp;Crossfade</button>
        </div>
      </div>`;

    strip.innerHTML = knobsHtml + dynamicsHtml;
    labelControlStrip();
}

// v1.24.0: the nine knob captions and the two knob accessible names.
//
// NINE LITERAL CALL SITES, deliberately verbose. The captions live in
// SLIDER_BINDINGS' array literal and are interpolated into an innerHTML
// template one frame from the write, which is the shape neither the extractor's
// js-prose scan nor assertion 12 can see — it is the O-Tremolo near-miss, and
// on O-Wind and O-Bells it was sixteen strings each. A key read from the data
// table (`b.key`) fails assertion 13's plain-literal rule and is invisible to
// assertion 15's dead-key sweep, so one literal call per knob is the only shape
// BOTH gates can read. The authored English stays in the markup above as
// contract §1's fallback.
function labelControlStrip() {
    const cap = (id) => document.querySelector(`label[for="${id}"]`);
    setLabel(cap('ctrl-attack'),              'label.knobAttack');
    setLabel(cap('ctrl-decay'),               'label.knobDecay');
    setLabel(cap('ctrl-sustain'),             'label.knobSustain');
    setLabel(cap('ctrl-release'),             'label.knobRelease');
    setLabel(cap('ctrl-polyphony'),           'label.knobPoly');
    setLabel(cap('ctrl-velocity-crossfade'),  'label.knobVelXf');
    setLabel(cap('ctrl-expression'),          'label.knobExpr');
    setLabel(cap('ctrl-dynamic-range'),       'label.knobDynRng');
    setLabel(cap('ctrl-output-gain'),         'label.knobOutGain');

    // The two deleted native titles, verbatim, as keyed accessible names.
    // Literal `.dataset.i18nAria =` writes — the shape assertion 15 collects,
    // so these read LIVE rather than dead.
    const expr = document.querySelector('[data-knob-id="ctrl-expression"]');
    if (expr) expr.dataset.i18nAria = 'aria.knobExpr';
    const dyn = document.querySelector('[data-knob-id="ctrl-dynamic-range"]');
    if (dyn) dyn.dataset.i18nAria = 'aria.knobDynRng';
}

// v1.21.0: bind the Dynamics Mode segmented toggle to the "dynamics_mode"
// WebComboBoxRelay. getChoiceIndex()/setChoiceIndex() mirror the choice param;
// valueChangedEvent/propertiesChangedEvent keep the UI in sync with host
// automation / preset loads (and with the choices arriving asynchronously
// after the initial-update request).
function bindDynamicsMode() {
    const seg = document.getElementById('dynamics-mode-seg');
    if (!seg) {
        console.warn('[sampler-app] #dynamics-mode-seg not found — dynamics toggle cannot bind');
        return;
    }
    const buttons = Array.from(seg.querySelectorAll('.seg-btn'));

    let state = null;
    try {
        state = Juce.getComboBoxState('dynamics_mode');
    } catch (e) {
        console.error('[sampler-app] Failed to bind dynamics_mode:', e);
        return;
    }

    const refresh = () => {
        const idx = Math.max(0, Math.min(buttons.length - 1, state.getChoiceIndex()));
        buttons.forEach(b => b.classList.toggle('active', Number(b.dataset.idx) === idx));
    };
    refresh();
    state.valueChangedEvent.addListener(refresh);
    if (state.propertiesChangedEvent) state.propertiesChangedEvent.addListener(refresh);

    buttons.forEach(b => {
        b.addEventListener('click', () => {
            state.setChoiceIndex(Number(b.dataset.idx));
            refresh();
        });
    });
}

function bindSliders() {
    renderControlStrip();
    if (!window.__JUCE__) {
        console.warn('[sampler-app] __JUCE__ not available — running outside plugin host');
        return;
    }
    SLIDER_BINDINGS.forEach(bindOneKnob);
    bindKnobGlobalDrag();
    bindDynamicsMode();   // v1.21.0
}

// ============================================================================
// Tab activation
// ============================================================================
const tabButtons = () => document.querySelectorAll('.tab-btn');
const tabBodies  = () => document.querySelectorAll('.tab-body');

function activateTab(tabName) {
    tabButtons().forEach(btn => {
        btn.classList.toggle('active', btn.dataset.tab === tabName);
    });
    tabBodies().forEach(body => {
        body.classList.toggle('active', body.id === `tab-${tabName}`);
    });

    // v1.10.0: anatomy overlay parallax — swap position class per tab so the
    // engraving slides subtly between tabs (samplemap = peeking right edge,
    // tuning = retreats further right, about = swings into view as a feature).
    const overlay = document.getElementById('anatomyOverlay');
    if (overlay) {
        overlay.classList.remove('samplemap-position', 'tuning-position', 'about-position');
        overlay.classList.add(`${tabName}-position`);
    }

    if (tabName === 'tuning') {
        ensureTuningPanelMounted();
        refreshTuningReadout();
    }
    // v1.1.0: re-sync vel-labels height when returning to Sample Map
    // (getBoundingClientRect returns 0 on hidden tab; values may be stale).
    if (tabName === 'samplemap') {
        requestAnimationFrame(syncVelLabelsHeight);
    }
}

function bindTabs() {
    tabButtons().forEach(btn => {
        btn.addEventListener('click', () => activateTab(btn.dataset.tab));
    });
}

// ============================================================================
// TuningPanel — lazy mount (v1.2.0: editable authoring surface)
//
// v1.0–v1.1 mounted the panel in display-only mode (per Stage 3 RESEARCH
// §RQ3-1) by layering tuning-panel-readonly.css on top of the editable
// HTML and span-swapping each interval-input into a read-only label.
// v1.2.0 reverses that decision: the panel is now fully editable —
// users can select factory tunings from the library, load .scl/.kbm
// files, generate scales, and edit individual intervals. The shared
// scala-tuning-engine remains the single source of truth that VST3
// Note Expression overrides at note-on time, so Dorico microtonal
// playback is preserved.
// ============================================================================
let tuningPanelMounted = false;
let tuningPanelInstance = null;

// v1.7.1: subscribe to C++ tuning-note events ONCE, at module init time, NOT
// inside ensureTuningPanelMounted (which is gated on first Tuning-tab click).
// Events that arrive before the panel is mounted are simply dropped; the
// catch-up pull inside ensureTuningPanelMounted brings the panel up to date
// with whatever is held at that moment, and subsequent events flow live.
function bindTuningNoteEvents() {
    if (!window.__JUCE__ || !window.__JUCE__.backend) return;

    const backend = window.__JUCE__.backend;

    backend.addEventListener('tuningNoteOn', (midiVar) => {
        if (!tuningPanelInstance) return;
        const midi = typeof midiVar === 'number' ? midiVar : parseInt(midiVar, 10);
        if (Number.isFinite(midi)) {
            try { tuningPanelInstance.noteOn(midi); } catch (_) { /* ignore */ }
        }
    });

    backend.addEventListener('tuningNoteOff', (midiVar) => {
        if (!tuningPanelInstance) return;
        const midi = typeof midiVar === 'number' ? midiVar : parseInt(midiVar, 10);
        if (Number.isFinite(midi)) {
            try { tuningPanelInstance.noteOff(midi); } catch (_) { /* ignore */ }
        }
    });

    backend.addEventListener('tuningHeldNotes', (payloadVar) => {
        if (!tuningPanelInstance) return;
        try {
            const payload = typeof payloadVar === 'string'
                ? JSON.parse(payloadVar) : payloadVar;
            if (payload && Array.isArray(payload.notes) && Array.isArray(payload.freqs)) {
                tuningPanelInstance.updateHeldNotes(payload.notes, payload.freqs);
            }
        } catch (_) { /* ignore — malformed payload is non-fatal */ }
    });
}
// Run once on module load.
bindTuningNoteEvents();

async function ensureTuningPanelMounted() {
    if (tuningPanelMounted) return;
    tuningPanelMounted = true;

    const container = document.getElementById('tuning-container');
    if (!container) {
        console.error('[sampler-app] #tuning-container missing');
        return;
    }

    try {
        const mod = await import('./tuning-panel.js');
        const TuningPanel = mod.TuningPanel || mod.default;
        if (!TuningPanel) {
            console.error('[sampler-app] tuning-panel.js exports unrecognized');
            return;
        }

        // tuning-panel.js calls juceApi.getNativeFunction(name) — that method
        // lives on the ES-module namespace `Juce`, NOT on window.__JUCE__
        // (which is the low-level postMessage handler). Passing the wrong
        // object is a pre-existing v1.0–v1.1 latent bug that silently
        // swallowed every backend call inside tuning-panel.js's try/catch
        // blocks (intervals never loaded, library never populated, etc.).
        tuningPanelInstance = new TuningPanel(container, Juce);
        await tuningPanelInstance.init();

        // v1.7.1: catch-up pull. The 30 Hz timer in C++ only emits
        // tuningHeldNotes on bitmask change, so a panel that mounts while
        // notes are already held would otherwise sit empty until the next
        // change. Pulling once at mount fixes that.
        try {
            const json = await Juce.getNativeFunction('getHeldNotesJson')();
            const payload = JSON.parse(json);
            if (Array.isArray(payload.notes) && Array.isArray(payload.freqs)) {
                tuningPanelInstance.updateHeldNotes(payload.notes, payload.freqs);
                // Replay note-ons so Circle / Polar highlights catch up too
                // (updateHeldNotes only feeds TrueKeys; activeScaleDegrees is
                // populated through noteOn).
                for (const midi of payload.notes) {
                    try { tuningPanelInstance.noteOn(midi); } catch (_) { /* ignore */ }
                }
            }
        } catch (_) { /* ignore — empty held set or backend unavailable */ }

        // v1.2.0: auto-expand the Tuning Library and pull the factory
        // preset list so the right column shows selectable tunings on
        // first open (instead of just the category dropdown). Same goes
        // for the Scale Generator section, which the panel renders
        // collapsed by default.
        const libContent = container.querySelector('#library-content');
        const libToggle  = container.querySelector('#library-toggle');
        libContent?.classList.add('expanded');
        libToggle?.classList.add('expanded');
        await tuningPanelInstance.loadEmbeddedTunings();

        const genContent = container.querySelector('#generator-content');
        const genToggle  = container.querySelector('#generator-toggle');
        genContent?.classList.add('expanded');
        genToggle?.classList.add('expanded');
    } catch (e) {
        console.error('[sampler-app] TuningPanel mount failed:', e);
        // v1.24.0: built with createElement + setLabel rather than an
        // innerHTML string, so the caption declares a LITERAL key that
        // assertion 15 can see and the language sweep owns it.
        container.innerHTML = '';
        const notice = document.createElement('div');
        notice.className = 'tuning-panel-unavailable';
        setLabel(notice, 'label.tuningPanelUnavailable');
        container.appendChild(notice);
    }
}

// ============================================================================
// Tuning-state readout (RP3-3 — poll on Tuning-tab activate + editor open)
// ============================================================================
async function refreshTuningReadout() {
    const el = document.getElementById('tuning-readout');
    if (!el || !window.__JUCE__) return;

    try {
        const getName = Juce.getNativeFunction('getTuningName');
        const name = await getName();
        if (typeof name === 'string') el.textContent = name;
    } catch (e) {
        // Silent — fail-safe if native function not registered yet.
    }
}

async function refreshAboutVersion() {
    const el = document.getElementById('about-version');
    if (!el || !window.__JUCE__) return;

    try {
        const getVersion = Juce.getNativeFunction('getPluginVersion');
        const value = await getVersion();
        if (typeof value === 'string' && value.length > 0) {
            el.textContent = 'v' + value;
        }
    } catch (e) {
        // Silent — leave pill empty if native function unavailable.
    }
}

// ============================================================================
// Sample-map snapshot — initial pull + push subscription
// ============================================================================
async function pullInitialSampleMap() {
    if (!window.__JUCE__) return;
    try {
        const getMap = Juce.getNativeFunction('getSampleMap');
        const json = await getMap();
        handleSampleMapSnapshot(json);
    } catch (e) {
        console.warn('[sampler-app] getSampleMap failed:', e);
    }
}

function subscribeSampleMapUpdates() {
    if (!window.__JUCE__ || !window.__JUCE__.backend) return;
    try {
        window.__JUCE__.backend.addEventListener('sampleMapUpdated', (payload) => {
            handleSampleMapSnapshot(payload);
        });
    } catch (e) {
        console.warn('[sampler-app] sampleMapUpdated subscription failed:', e);
    }
}

// Track previous skipped-files count so we only toast on transitions
// (e.g. 0 → N or M → N where the set changed). Toasts are emitted on every
// fresh load that produces skips; not on idle re-renders.
let lastSkippedSignature = '';

// v1.14.0: cache the most recent snapshot so technique-tab switches can
// re-render the grid without a fresh pull from C++.
let lastSampleMapSnapshot = null;

function handleSampleMapSnapshot(payloadOrJson) {
    let snap;
    try {
        // Push events arrive parsed; pull returns a JSON string.
        snap = (typeof payloadOrJson === 'string')
            ? JSON.parse(payloadOrJson)
            : payloadOrJson;
    } catch (e) {
        console.warn('[sampler-app] sampleMap snapshot parse failed:', e);
        return;
    }

    if (!snap) return;
    lastSampleMapSnapshot = snap;     // v1.14.0

    // v1.14.0: refresh the technique tab strip's per-slot cell counts and
    // empty-slot dimming whenever the map changes (folder load, single-cell
    // load, clear). Render is cheap — one DOM rebuild for at most 8 buttons.
    renderTechniqueBar();

    // v1.23.0: refresh the trim panel — its visibility (loaded vs empty) and
    // per-layer dimming both derive from the current map.
    renderTrimPanel();

    // ---- Issues disclosure + toast (Phase 3.3 Task 21) ----
    //
    // The disclosure is always reflected (open or closed depending on the
    // current map's skip set). The toast is emitted only on a *transition* —
    // i.e. the new skip-set differs from the last one we showed — to avoid
    // re-toasting on every push event when the set is stable.
    const issues = document.getElementById('issues-disclosure');
    const issuesList = document.getElementById('issues-list');
    const skipped = Array.isArray(snap.skippedFiles) ? snap.skippedFiles : [];
    const sig = skipped.join('');  // 0x01 separator — never appears in filenames

    if (issues && issuesList) {
        if (skipped.length > 0) {
            issuesList.innerHTML = '';
            skipped.forEach(s => {
                const li = document.createElement('li');
                // Each entry is "filename: reason" or just "filename" — render
                // the raw string; the processor formats consistently.
                li.textContent = s;
                issuesList.appendChild(li);
            });
            // v1.24.0 (contract §6): the English ternary is GONE, not ported.
            // French pluralizes 0 as singular and English does not, so a
            // mechanical port is wrong at n=0 before it is wrong anywhere else.
            // The copy is authored around the inflection instead: the count sits
            // after a colon, beside an invariant plural noun phrase that reads
            // as a category header at 0, 1 and n in both languages.
            setLabel(issues.querySelector('summary'), 'label.issuesSummary',
                     { n: skipped.length });
            issues.hidden = false;
        } else {
            issues.hidden = true;
        }
    }

    // Only toast on transition. (Initial pull on editor open: lastSkippedSignature
    // is '' — if there were skips from a prior session they will toast once.
    // Subsequent renders with the same set are silent.)
    if (sig !== lastSkippedSignature) {
        lastSkippedSignature = sig;
        if (skipped.length > 0) {
            showToastKey('toast.filesSkipped', { n: skipped.length });   // §6: no inflection
        }
    }

    // ---- Grid render (Phase 3.2) ----
    renderGrid(snap);

    // ---- Clear-samples button enable/disable (v1.0.2) ----
    // Only meaningful when the map has at least one loaded slot — otherwise
    // there is nothing to clear and the button stays disabled to avoid
    // accidental clicks (and a no-op confirmation roundtrip).
    const clearBtn = document.getElementById('clear-samples-btn');
    const hasSlots = Array.isArray(snap?.slots) && snap.slots.length > 0;
    if (clearBtn) {
        clearBtn.disabled = !hasSlots;
    }
    // v1.18.0: batch loop button follows the same has-samples gate.
    const batchLoopBtn = document.getElementById('batch-loop-btn');
    if (batchLoopBtn) {
        batchLoopBtn.disabled = !hasSlots;
    }

    // Re-publish the cell-layout shadow now that DOM has settled. Defer one
    // frame so the browser has computed final geometry.
    requestAnimationFrame(() => publishCellLayout());

    // ---- Loop editor sync (Phase 3.4) ----
    // If the editor is open and the active cell's loop fields changed (e.g.
    // user clicked Apply, or external automation altered the map), refresh
    // the editor's snapshot so the loop-mode label / reset-button enabled
    // state stay consistent. Marker positions are NOT clobbered if the user
    // is mid-drag (dragMarker !== null).
    if (editorState.open && editorState.midi >= 0) {
        const cellSlot = Array.isArray(snap?.slots)
            ? snap.slots.find(s => s.midiNote === editorState.midi
                                && s.velocityLayer === editorState.vel)
            : null;
        if (cellSlot && editorState.snap && editorState.dragMarker === null) {
            editorState.snap.loopMode = cellSlot.loopMode;
            editorState.snap.loopStart = cellSlot.loopStart;
            editorState.snap.loopEnd   = cellSlot.loopEnd;
            editorState.snap.filename  = cellSlot.filename;
            editorState.loopStart = cellSlot.loopStart;
            editorState.loopEnd   = cellSlot.loopEnd;
            populateLoopEditorHeader(editorState.snap);
            redrawLoopEditor();
        }
    }
}

// ============================================================================
// Sample-mapping grid (Phase 3.2 — Tasks 15–17)
// ============================================================================
//
// Layout per PLAN Task 15:
//   - 88 columns (MIDI 21..108), 4 rows (velocity layers).
//   - Rows top→bottom = layer 3..0 so the loudest layer reads at the top.
//   - data-note + data-layer attributes for hit-testing.
//   - cell-loaded / cell-empty / cell-loading classes.
//   - title attribute = filename for loaded cells (tooltip).
//   - octave-separator marker (data-octave-start) every 12 cells.
//
// The grid is rebuilt on every snapshot. With 352 cells (88 × 4), full
// rebuild is < 5 ms and avoids stateful diff bookkeeping.

const MIDI_LOW  = 21;   // labelled "A-1" under the C3=60 convention (A0 in C4=60 terms)
const MIDI_HIGH = 108;  // labelled "C7" under the C3=60 convention (C8 in C4=60 terms)
const NUM_LAYERS = 4;

// v1.1.0: Velocity-layer ranges (matches MicrotonalSamplerVoice.cpp:
// layerWidth = 128/4 = 32, layerIdx = (vel-1)/32 with vel ∈ [1,127]).
//   L0 → 1..32, L1 → 33..64, L2 → 65..96, L3 → 97..127
// v1.18.1: dynamic markings replace the numeric layer labels in the UI.
//   L0 → p (softest) · L1 → mp · L2 → mf · L3 → f (loudest).
// The numeric `label` is retained for tooltips; `mark` is the visible label.
const VELOCITY_MARKS = ['p', 'mp', 'mf', 'f'];

function velocityLayerToRange(layer) {
    const lo = layer === 0 ? 1 : layer * 32 + 1;
    const hi = layer === 3 ? 127 : (layer + 1) * 32;
    const mark = VELOCITY_MARKS[layer] || `L${layer}`;
    return { lo, hi, mark, label: `${lo}–${hi}` };
}

const NOTE_NAMES = ['C', 'C♯', 'D', 'D♯', 'E', 'F', 'F♯', 'G', 'G♯', 'A', 'A♯', 'B'];

// MIDI 0 = C-2, MIDI 12 = C-1, MIDI 60 = C3 (middle C), … This is the
// Ableton/Cubase/FL/Logic/Pro Tools/Reaper convention. v1.11.1 switched
// from C4=60 (Yamaha/JUCE-native) to C3=60 to match the FilenameParser
// (Source/FilenameParser.cpp) and the dominant DAW labelling. Keep these
// two formulas in lockstep — drift causes parsed cell midiNotes and UI
// labels to diverge by 12 semitones.
function midiToNoteName(midi) {
    const pitchClass = ((midi % 12) + 12) % 12;
    const octave = Math.floor(midi / 12) - 2;
    return `${NOTE_NAMES[pitchClass]}${octave}`;
}

function renderGrid(snap) {
    const container = document.getElementById('sample-map-grid');
    if (!container) return;

    // v1.12.2 (FE-03): cancel any in-flight single-click defer before we
    // tear down the cell DOM. The 250 ms double-click discriminator (see
    // bindGridInteractions) closes over a `cell` reference; if a folder
    // load / sample-map snapshot fires a renderGrid mid-defer, the timer
    // would later read `dataset.note`/`dataset.layer` off a detached node
    // and silently no-op (or worse, fire on a re-bound stale cell at the
    // same grid position with different MIDI/layer values). Clearing the
    // pending timer is the simplest fix and matches the cleanup the
    // dblclick branch already performs.
    if (pendingClickTimer) {
        clearTimeout(pendingClickTimer);
        pendingClickTimer = null;
    }

    // Build fast — clear via innerHTML then assemble in a fragment.
    container.innerHTML = '';
    const inner = document.createElement('div');
    inner.id = 'sample-grid-inner';

    // v1.8.0: prefer cells (multi-variant aware) but accept legacy slots
    // for back-compat in case any consumer is still feeding the old shape.
    // Cells emit `{midiNote, velocityLayer, technique, variants: [...]}`.
    // Slots emit `{midiNote, velocityLayer, filename, variantCount?, ...}`
    // with `variantCount` carrying the cell's variant count (v1.8.0+).
    //
    // v1.14.0: filter cells by the active technique tab. Cells in other
    // technique slots exist in the snapshot but are not visible in the
    // grid until the user switches tabs. Falls back gracefully when the
    // snapshot has no `technique` field (v1.13.0 in-memory contract).
    const activeTech = techniqueState.active;
    const cellMap = new Map();
    if (Array.isArray(snap?.cells)) {
        for (const c of snap.cells) {
            const t = num(c.technique);
            if (t !== activeTech) continue;
            cellMap.set(`${c.midiNote}_${c.velocityLayer}`, c);
        }
    }
    // The legacy `slots` array isn't technique-aware. We only fall through
    // to it when the cells map is empty AND the active technique is 0 —
    // that way single-technique back-compat libraries still render via the
    // legacy path while multi-technique sessions never see ghost slots
    // bleeding from another technique.
    const slotMap = new Map();
    if (cellMap.size === 0 && activeTech === 0 && Array.isArray(snap?.slots)) {
        for (const s of snap.slots) {
            slotMap.set(`${s.midiNote}_${s.velocityLayer}`, s);
        }
    }

    const frag = document.createDocumentFragment();

    // Rows are layer 3 (top) → layer 0 (bottom) so loudest-on-top.
    for (let row = 0; row < NUM_LAYERS; ++row) {
        const layer = (NUM_LAYERS - 1) - row;
        const velRange = velocityLayerToRange(layer);
        for (let midi = MIDI_LOW; midi <= MIDI_HIGH; ++midi) {
            const cell = document.createElement('div');
            cell.className = 'grid-cell';
            cell.dataset.note = String(midi);
            cell.dataset.layer = String(layer);

            // Mark octave starts (C notes — midi % 12 === 0) for the octave
            // separator border. Always also include MIDI 21 (lowest in range).
            if (midi % 12 === 0 || midi === MIDI_LOW) {
                cell.dataset.octaveStart = '1';
            }

            // ── v1.24.0: 352 NATIVE title= DELETED (contract §4) ──────────
            //
            // This loop wrote a native title on EVERY cell — 88 notes x 4
            // layers — so v1.23.10 rendered 356 of them here alone against the
            // five index.html declares. A source grep counts WRITES (five in
            // this function); the page renders INSTANCES.
            //
            // §4 deletes a native title rather than localizing it, and moves
            // its text to a keyed accessible name where it is the element's
            // only help. A grid cell is an empty <div>: it has no text of its
            // own, so the title WAS its only help and deleting it outright
            // would take the filename with it. But the string is COMPOSED from
            // per-cell data, and applyI18nAttributes() resolves a
            // data-i18n-aria key with no vars — so a keyed attribute cannot
            // express it. The accessible name is therefore composed here from
            // trComposed() parts and re-composed by refreshComposedUi() on
            // every language change, which is the same one-re-render-path rule
            // §3 states for a JS-written label.
            //
            // AN EMPTY CELL GETS NO ACCESSIBLE NAME AT ALL. "Empty · C3 (60) ·
            // Vel mf (65–96)" is pure grid position, and the position is
            // already on the page twice — #sample-grid-col-labels names every
            // C and #sample-grid-vel-labels names every layer. 348 of the 352
            // attributes in the default state carried nothing else.
            //
            // WHAT THE USER LOSES, stated rather than hidden: the HOVER
            // tooltip is gone. Every Stage K plugin pays that, because these
            // 21 plugins have no data-tip renderer and authoring hover-help
            // copy is Stage M's job, not this rule's.
            const noteLabel = `${midiToNoteName(midi)} (${midi})`;
            const velLabel  = trComposed('aria.cellVel',
                                         { mark: velRange.mark, range: velRange.label });
            const techName  = techniqueState.names[activeTech]
                              || trComposed('aria.slotN', { n: activeTech + 1 });
            const techLabel = (techniqueState.count > 1)
                                ? ' · ' + trComposed('aria.cellTech', { name: techName })
                                : '';
            const cellEntry = cellMap.get(`${midi}_${layer}`);
            const slot = slotMap.get(`${midi}_${layer}`);

            if (cellEntry && Array.isArray(cellEntry.variants) && cellEntry.variants.length > 0) {
                cell.classList.add('cell-loaded');
                const variantCount = cellEntry.variants.length;
                if (variantCount > 1) {
                    cell.classList.add('cell-multivariant');
                    const head = trComposed('aria.cellVariants', { n: variantCount });
                    const unnamed = trComposed('aria.unnamed');
                    const list = cellEntry.variants
                        .map((v, i) => `  ${i + 1}. ${v.filename || unnamed}`)
                        .join('\n');
                    cell.setAttribute('aria-label',
                        `${head}\n${list}\n${noteLabel} · ${velLabel}${techLabel}`);
                    cell.dataset.variantCount = String(variantCount);
                } else {
                    const loadedWord = trComposed('aria.loaded');
                    const head = cellEntry.variants[0].filename || loadedWord;
                    cell.setAttribute('aria-label',
                        `${head} · ${noteLabel} · ${velLabel}${techLabel}`);
                }
            } else if (slot) {
                cell.classList.add('cell-loaded');
                const loadedWord2 = trComposed('aria.loaded');
                const head = slot.filename ? slot.filename : loadedWord2;
                if (slot.variantCount && slot.variantCount > 1) {
                    cell.classList.add('cell-multivariant');
                    const slotVariants = trComposed('aria.cellVariants', { n: slot.variantCount });
                    cell.setAttribute('aria-label',
                        `${head} · ${slotVariants} · ${noteLabel} · ${velLabel}`);
                    cell.dataset.variantCount = String(slot.variantCount);
                } else {
                    cell.setAttribute('aria-label', `${head} · ${noteLabel} · ${velLabel}`);
                }
            } else {
                cell.classList.add('cell-empty');
            }

            frag.appendChild(cell);
        }
    }

    inner.appendChild(frag);
    container.appendChild(inner);

    // v1.1.0: Render C-note column labels below the grid (inside scroll
    // container, so they pan horizontally with it).
    const colLabels = document.createElement('div');
    colLabels.id = 'sample-grid-col-labels';
    const colFrag = document.createDocumentFragment();
    for (let midi = MIDI_LOW; midi <= MIDI_HIGH; ++midi) {
        const lbl = document.createElement('span');
        lbl.className = 'col-label';
        if (midi % 12 === 0) {
            lbl.dataset.c = '1';
            lbl.textContent = midiToNoteName(midi);   // "C0" … "C7" (C3=60 convention)
        }
        colFrag.appendChild(lbl);
    }
    colLabels.appendChild(colFrag);
    container.appendChild(colLabels);

    // v1.1.0: Render velocity row labels in the sidebar wrapper (outside
    // scroll container so they stay visible while user scrolls horizontally).
    const velLabels = document.getElementById('sample-grid-vel-labels');
    if (velLabels) {
        velLabels.innerHTML = '';
        const vFrag = document.createDocumentFragment();
        for (let row = 0; row < NUM_LAYERS; ++row) {
            const layer = (NUM_LAYERS - 1) - row;
            const v = velocityLayerToRange(layer);
            const el = document.createElement('div');
            el.className = 'vel-label';
            el.dataset.layer = String(layer);
            el.textContent = v.mark;
            // v1.18.0: right-click clears the whole layer (across techniques).
            // v1.24.0: native title= DELETED (§4). The label's own text is the
            // dynamic mark alone, so the right-click affordance IS its only
            // help — it moves to a composed accessible name that keeps the mark
            // in front, so a screen reader still reads the mark first.
            const velAria = trComposed('aria.velLabel',
                                       { mark: v.mark, layer, range: v.label });
            el.setAttribute('aria-label', velAria);
            vFrag.appendChild(el);
        }
        velLabels.appendChild(vFrag);
    }

    // Wire interactions on the inner container (single delegated listener).
    bindGridInteractions(inner);

    // v1.1.0: Sync vel-labels' top-padding + height to the inner grid's
    // actual rendered position. Run on next frame so layout is settled.
    requestAnimationFrame(syncVelLabelsHeight);
}

// v1.1.0: Align vel-labels rows to the rendered inner grid.
//   topOffset = inner.top - wrapper.top  → matches scroll container's
//                                          top-padding offset.
//   height    = topOffset + inner.height → bottom edge meets inner-grid bottom.
// The vel-labels' grid-template-rows: repeat(4, 1fr) then fills the area
// between (topOffset, topOffset+innerHeight), aligning row-for-row with
// the cells.
function syncVelLabelsHeight() {
    const inner   = document.getElementById('sample-grid-inner');
    const vel     = document.getElementById('sample-grid-vel-labels');
    const wrapper = document.getElementById('sample-map-grid-wrapper');
    if (!inner || !vel || !wrapper) return;

    const innerRect   = inner.getBoundingClientRect();
    const wrapperRect = wrapper.getBoundingClientRect();
    const topOffset   = Math.max(0, innerRect.top - wrapperRect.top);

    vel.style.paddingTop    = topOffset + 'px';
    vel.style.paddingBottom = '0px';
    vel.style.height        = (topOffset + innerRect.height) + 'px';
    vel.style.boxSizing     = 'border-box';
}

// ============================================================================
// Cell interactions (Phase 3.2 Task 16 — RP3-1 resolution)
// ============================================================================
//
//   - Single-click EMPTY  → loadSingleSampleDialog(midi, vel)
//   - Single-click LOADED → openLoopEditor(midi, vel)  (3.4 placeholder; logs)
//   - Double-click LOADED → loadSingleSampleDialog(midi, vel)  (replace)
//   - Right-click any     → context menu (Replace / Open Loop Editor / Clear)
//
// The 250 ms double-click discrimination is implemented via setTimeout — when
// a single-click fires we delay the action by 250 ms; if a second click
// arrives in that window we cancel the timer and fire the dblclick action
// instead. The browser's native `dblclick` event fires on the second click
// so we use it directly and rely on the timer to defer the single-click.

let pendingClickTimer = null;

function bindGridInteractions(innerEl) {
    if (!innerEl || innerEl.dataset.bound === '1') return;
    innerEl.dataset.bound = '1';

    innerEl.addEventListener('click', (e) => {
        const cell = e.target.closest('.grid-cell');
        if (!cell) return;
        const midi  = parseInt(cell.dataset.note, 10);
        const layer = parseInt(cell.dataset.layer, 10);
        if (!Number.isFinite(midi) || !Number.isFinite(layer)) return;

        // Defer single-click so a follow-up dblclick can cancel it.
        if (pendingClickTimer) clearTimeout(pendingClickTimer);
        pendingClickTimer = setTimeout(() => {
            pendingClickTimer = null;
            handleCellSingleClick(cell, midi, layer);
        }, 250);
    });

    innerEl.addEventListener('dblclick', (e) => {
        const cell = e.target.closest('.grid-cell');
        if (!cell) return;
        // Cancel the pending single-click action.
        if (pendingClickTimer) {
            clearTimeout(pendingClickTimer);
            pendingClickTimer = null;
        }
        const midi  = parseInt(cell.dataset.note, 10);
        const layer = parseInt(cell.dataset.layer, 10);
        if (!Number.isFinite(midi) || !Number.isFinite(layer)) return;

        // Double-click on a loaded cell → replace path. Empty cells route
        // to the same FileChooser as single-click; behaviour is consistent.
        replaceCellSample(cell, midi, layer);
    });

    innerEl.addEventListener('contextmenu', (e) => {
        const cell = e.target.closest('.grid-cell');
        if (!cell) return;
        e.preventDefault();
        showContextMenu(cell, e.clientX, e.clientY);
    });
}

function handleCellSingleClick(cell, midi, layer) {
    const isLoaded = cell.classList.contains('cell-loaded');
    if (isLoaded) {
        // Phase 3.4 — open loop editor side panel.
        openLoopEditor(midi, layer);
    } else {
        replaceCellSample(cell, midi, layer);
    }
}

async function replaceCellSample(cell, midi, layer) {
    if (!window.__JUCE__) return;

    // v1.9.0: when the target cell is non-empty, prompt the user to choose
    // between merging the new sample as a round-robin variant or replacing
    // the cell. Empty cells skip straight to the file picker (v1.8.0 path).
    let mergeAsRr = false;
    const isLoaded = cell.classList.contains('cell-loaded');
    if (isLoaded) {
        const existingCount = parseInt(cell.dataset.variantCount, 10);
        const count = Number.isFinite(existingCount) && existingCount > 0
            ? existingCount : 1;
        const choice = await showPerCellMergeDialog(count, midi, layer);
        if (choice === null) return;
        mergeAsRr = (choice === 'merge');
    }

    cell.classList.add('cell-loading');

    try {
        const fn = Juce.getNativeFunction('loadSingleSampleDialog');
        const ok = await fn(midi, layer, mergeAsRr);
        if (!ok) {
            // User cancelled or selection invalid — drop the loading shimmer.
            cell.classList.remove('cell-loading');
            return;
        }
        // The sampleMapUpdated push event will trigger renderGrid which
        // rebuilds the cell.
    } catch (e) {
        console.error('[sampler-app] replaceCellSample failed:', e);
        cell.classList.remove('cell-loading');
    }
}

// ---- Context menu ----
let contextMenuCell = null;

function showContextMenu(cell, clientX, clientY) {
    const menu = document.getElementById('cell-context-menu');
    if (!menu) return;

    const isLoaded = cell.classList.contains('cell-loaded');
    contextMenuCell = cell;

    // Disable "Open Loop Editor" and "Delete sample" on empty cells (v1.18.0).
    const openBtn = menu.querySelector('button[data-action="open-loop-editor"]');
    if (openBtn) openBtn.disabled = !isLoaded;
    const deleteBtn = menu.querySelector('button[data-action="delete-cell"]');
    if (deleteBtn) deleteBtn.disabled = !isLoaded;

    menu.style.left = `${clientX}px`;
    menu.style.top  = `${clientY}px`;
    menu.hidden = false;

    // Wire actions once.
    if (menu.dataset.bound !== '1') {
        menu.dataset.bound = '1';
        menu.addEventListener('click', (e) => {
            const btn = e.target.closest('button');
            if (!btn || btn.disabled) return;
            const action = btn.dataset.action;
            const cellEl = contextMenuCell;
            if (cellEl) {
                const midi  = parseInt(cellEl.dataset.note, 10);
                const layer = parseInt(cellEl.dataset.layer, 10);
                if (action === 'replace') {
                    replaceCellSample(cellEl, midi, layer);
                } else if (action === 'open-loop-editor') {
                    openLoopEditor(midi, layer);
                } else if (action === 'delete-cell') {
                    deleteCellWithConfirm(midi, layer);   // v1.18.0
                }
            }
            hideContextMenu();
        });
    }
}

function hideContextMenu() {
    const menu = document.getElementById('cell-context-menu');
    if (menu) menu.hidden = true;
    contextMenuCell = null;
}

document.addEventListener('click', (e) => {
    const menu = document.getElementById('cell-context-menu');
    if (!menu || menu.hidden) return;
    if (!menu.contains(e.target)) hideContextMenu();
});

// ============================================================================
// Single-cell delete + clear-layer (v1.18.0)
// ============================================================================
//
// Granular counterparts to "Clear samples" (which wipes the whole map):
//   - Right-click a loaded cell → "Delete sample" removes that (note, layer)
//     cell on the ACTIVE technique only.
//   - Right-click a velocity-row label → clears that velocity layer across
//     ALL techniques (matches the ReplaceLayer load mode).
//
// Both go through the shared confirm dialog. The grid refreshes via the
// sampleMapUpdated push event fired by the processor's atomic-store.

function deleteCellWithConfirm(midi, layer) {
    const noteName = (typeof midiToNoteName === 'function')
        ? midiToNoteName(midi) : `MIDI ${midi}`;
    const techName = techniqueState.names[techniqueState.active];
    const techStr = (techniqueState.count > 1 && techName)
        ? ` (technique “${techName}”)` : '';
    const mark = velocityLayerToRange(layer).mark;
    showConfirmDialog({
        title: trComposed('msg.deleteSampleTitle'),
        message: trComposed('msg.deleteSampleBody',
                            { note: noteName, mark, tech: techStr }),
        confirmLabel: trComposed('msg.deleteBtn'),
        destructive: true,
        onConfirm: async () => {
            const ok = await invokeNative('deleteSampleCell', midi, layer, techniqueState.active);
            if (!ok) showToastKey('toast.nothingToDelete');
            // On success the sampleMapUpdated push triggers renderGrid.
        },
    });
}

function clearLayerWithConfirm(layer) {
    const mark = velocityLayerToRange(layer).mark;
    showConfirmDialog({
        title: trComposed('msg.clearLayerTitle', { mark }),
        message: trComposed('msg.clearLayerBody', { mark }),
        confirmLabel: trComposed('msg.clearLayerBtn'),
        destructive: true,
        onConfirm: async () => {
            const removed = await invokeNative('clearVelocityLayer', layer);
            const n = Number.isFinite(removed) ? removed : 0;
            // §6: no ternary suffix. Two separately keyed faces, and the
            // count sits after a colon beside an invariant noun phrase.
            if (n > 0) showToastKey('toast.layerCleared', { mark, n });
            else       showToastKey('toast.layerAlreadyEmpty', { mark });
        },
    });
}

// Delegated contextmenu listener on the persistent vel-labels container.
// renderGrid rebuilds the child labels but the container element survives, so
// binding once here is safe.
function bindVelLabelInteractions() {
    const velLabels = document.getElementById('sample-grid-vel-labels');
    if (!velLabels || velLabels.dataset.bound === '1') return;
    velLabels.dataset.bound = '1';
    velLabels.addEventListener('contextmenu', (e) => {
        const label = e.target.closest('.vel-label');
        if (!label) return;
        e.preventDefault();
        const layer = parseInt(label.dataset.layer, 10);
        if (!Number.isFinite(layer)) return;
        clearLayerWithConfirm(layer);
    });
}

// ============================================================================
// Batch loop-point modal (v1.18.0)
// ============================================================================
//
// Applies one loop region to EVERY loaded variant at once. Two unit modes:
//   mode 0 — Proportional: inputs are 0–100 %, sent to C++ as fractions 0..1.
//   mode 1 — Milliseconds: inputs are ms-from-start, sent as-is.
// The Apply button is disabled while inputs are invalid (end must exceed
// start; % capped at 100) so bindModal's apply handler can assume validity.

function bindBatchLoopButton() {
    const btn = document.getElementById('batch-loop-btn');
    if (!btn) return;
    btn.addEventListener('click', () => { openBatchLoopDialog(); });
}

function openBatchLoopDialog() {
    const dialog     = document.getElementById('batch-loop-dialog');
    if (!dialog) return;
    const startEl    = document.getElementById('bl-start');
    const endEl      = document.getElementById('bl-end');
    const startUnit  = document.getElementById('bl-start-unit');
    const endUnit    = document.getElementById('bl-end-unit');
    const errEl      = document.getElementById('bl-error');
    const applyBtn   = document.getElementById('bl-apply-btn');
    const cancelBtn  = document.getElementById('bl-cancel-btn');
    const modeRadios = Array.from(dialog.querySelectorAll('input[name="bl-mode"]'));
    if (!startEl || !endEl || !startUnit || !endUnit || !applyBtn
            || !cancelBtn || !modeRadios.length) return;

    const getMode = () => {
        const checked = modeRadios.find(r => r.checked);
        return checked ? parseInt(checked.value, 10) : 0;
    };

    const applyModeUI = (mode) => {
        if (mode === 1) {
            startUnit.textContent = 'ms';
            endUnit.textContent   = 'ms';
            startEl.min = '0'; startEl.removeAttribute('max'); startEl.step = '1';
            endEl.min   = '0'; endEl.removeAttribute('max');   endEl.step = '1';
            startEl.value = '0';
            endEl.value   = '1000';
        } else {
            startUnit.textContent = '%';
            endUnit.textContent   = '%';
            startEl.min = '0'; startEl.max = '100'; startEl.step = '1';
            endEl.min   = '0'; endEl.max   = '100'; endEl.step = '1';
            startEl.value = '0';
            endEl.value   = '100';
        }
    };

    const validate = () => {
        const mode  = getMode();
        const start = parseFloat(startEl.value);
        const end   = parseFloat(endEl.value);
        const ok = Number.isFinite(start) && Number.isFinite(end)
                && start >= 0 && end > start
                && (mode !== 0 || end <= 100);
        applyBtn.disabled = !ok;
        if (errEl) {
            if (ok || startEl.value === '' || endEl.value === '') {
                errEl.hidden = true;
            } else {
                // Two literal keys, not one ternary: assertion 13 rejects a
                // conditional inside a setLabel argument on purpose, because a
                // conditional is where an inflection creeps back in (§6).
                if (mode === 0) setLabel(errEl, 'label.blErrPercent');
                else            setLabel(errEl, 'label.blErrMs');
                errEl.hidden = false;
            }
        }
        return ok;
    };

    // Reset to proportional defaults each open.
    modeRadios.forEach(r => { r.checked = (r.value === '0'); });
    applyModeUI(0);
    validate();

    const onModeChange = (e) => { applyModeUI(parseInt(e.target.value, 10)); validate(); };
    const onInput = () => validate();
    modeRadios.forEach(r => r.addEventListener('change', onModeChange));
    startEl.addEventListener('input', onInput);
    endEl.addEventListener('input', onInput);

    bindModal(dialog, [
        [applyBtn, async () => {
            const mode  = getMode();
            const start = parseFloat(startEl.value);
            const end   = parseFloat(endEl.value);
            const sVal  = (mode === 0) ? start / 100.0 : start;
            const eVal  = (mode === 0) ? end   / 100.0 : end;
            const updated = await invokeNative('applyLoopPointsToAll', mode, sVal, eVal, 8);
            const n = Number.isFinite(updated) ? updated : 0;
            // §6 again: the count moved behind a colon so no suffix inflects.
            if (n > 0) showToastKey('toast.loopPointsApplied', { n });
            else       showToastKey('toast.noLoopableSamples');
            return true;
        }],
        [cancelBtn, () => false],
    ], {
        keys:    { Escape: cancelBtn, Enter: applyBtn },
        focus:   startEl,
        onClose: () => {
            modeRadios.forEach(r => r.removeEventListener('change', onModeChange));
            startEl.removeEventListener('input', onInput);
            endEl.removeEventListener('input', onInput);
        },
    });
}

// ============================================================================
// Layout shadow publish (Phase 3.2 Task 17 — reportCellLayout)
// ============================================================================
//
// The C++ side hit-tests file drops against a JS-published cell rectangle map.
// We re-publish on:
//   - every sampleMapUpdated event (after the grid renders),
//   - ResizeObserver callbacks on document.body (rAF-throttled — RESEARCH §9
//     risk register: too-frequent calls cause WebView main-thread thrash).
//
// The shadow is JSON: {cells: [{midiNote, velocityLayer, x, y, w, h}], folderZone: {...}}.
// Coordinates are in WebView client space (page-relative), which is what the
// C++ FileDragAndDropTarget expects.

let layoutPublishScheduled = false;

function publishCellLayout() {
    if (layoutPublishScheduled || !window.__JUCE__) return;
    layoutPublishScheduled = true;

    requestAnimationFrame(() => {
        layoutPublishScheduled = false;

        try {
            const cells = [];
            document.querySelectorAll('.grid-cell').forEach(el => {
                const r = el.getBoundingClientRect();
                cells.push({
                    midiNote:      parseInt(el.dataset.note, 10),
                    velocityLayer: parseInt(el.dataset.layer, 10),
                    x: Math.round(r.left),
                    y: Math.round(r.top),
                    w: Math.round(r.width),
                    h: Math.round(r.height)
                });
            });

            let folderZone = { x: 0, y: 0, w: 0, h: 0 };
            const fz = document.getElementById('folder-drop-zone');
            if (fz) {
                const r = fz.getBoundingClientRect();
                folderZone = {
                    x: Math.round(r.left),
                    y: Math.round(r.top),
                    w: Math.round(r.width),
                    h: Math.round(r.height)
                };
            }

            const payload = JSON.stringify({ cells, folderZone });
            const fn = Juce.getNativeFunction('reportCellLayout');
            fn(payload);
        } catch (e) {
            console.warn('[sampler-app] publishCellLayout failed:', e);
        }
    });
}

// Phase 3.5 Task 33 — auto-close loop editor + toast when window narrows
// below 900 px while it's open (panel + grid would otherwise overlap).
const NARROW_BREAKPOINT_PX = 900;
const NARROW_TOAST_KEY = 'toast.resizeWider';
let lastIsNarrow = null;  // null = not yet measured; otherwise boolean

function checkNarrowWindowGuard() {
    const w = window.innerWidth || document.documentElement.clientWidth || 0;
    const isNarrow = w < NARROW_BREAKPOINT_PX;
    if (isNarrow === lastIsNarrow) return;
    lastIsNarrow = isNarrow;

    if (isNarrow && editorState.open) {
        closeLoopEditor();
        showToastKey(NARROW_TOAST_KEY);
    }
}

function bindResizeObserver() {
    const onResize = () => {
        publishCellLayout();
        checkNarrowWindowGuard();
        syncVelLabelsHeight();   // v1.1.0: keep vel-labels aligned on resize
    };
    if (typeof ResizeObserver !== 'function') {
        // Fallback to window resize listener.
        window.addEventListener('resize', onResize);
        return;
    }
    const obs = new ResizeObserver(onResize);
    obs.observe(document.body);
    // Seed initial bucket so the very first transition fires correctly.
    checkNarrowWindowGuard();
}

// ============================================================================
// Folder drop-zone — Load Folder… button (Phase 3.3 Task 20)
// ============================================================================

function bindFolderDropZone() {
    const button = document.getElementById('load-folder-btn');
    if (!button) return;

    button.addEventListener('click', async () => {
        if (!window.__JUCE__) return;
        try {
            // v1.6.0: ask for layer / mode / override BEFORE the native file
            // picker so the user has the controls in front of them while the
            // OS modal is still latent. Cancel here = silent return.
            //
            // v1.12.0: dialog flow doesn't know the folder size at modal
            // time, so we pass null for sizeBytes — the embed checkbox
            // surfaces a "size confirmed after selection" hint, and a
            // post-pick confirmation modal shows the real size before
            // the load commits.
            const opts = await showFolderLoadOptionsModal(null);
            if (!opts) return;

            // v1.12.0: pickSampleFolder returns just the folder path
            // (replaces v1.6.0's combined loadSampleFolderDialog). This
            // separation lets us interject the embed-size confirmation
            // between selection and load.
            const pick = await Juce.getNativeFunction('pickSampleFolder')();
            if (!pick || pick.cancelled || !pick.path) {
                return;
            }

            // v1.12.0: when embed is on, surface the actual byte cost as a
            // confirmation gate. Cancel = abort the entire load (no half-
            // committed state on the C++ side, since we haven't called load
            // yet).
            if (opts.embedAudio) {
                let sizeBytes = 0;
                try {
                    sizeBytes = await Juce.getNativeFunction(
                        'estimateFolderAudioSize')(pick.path);
                } catch (sizeErr) {
                    console.warn('[sampler-app] estimateFolderAudioSize failed:', sizeErr);
                }
                const proceed = await showEmbedSizeConfirmModal(
                    Number(sizeBytes) || 0, pick.name || '');
                if (!proceed) return;
            }

            const ok = await Juce.getNativeFunction('loadSampleFolderByPath')(
                pick.path,
                opts.layer,
                opts.mode,
                opts.override         ? 1 : 0,
                opts.embedAudio       ? 1 : 0,
                opts.technique         ?? 0,
                opts.overrideTechnique ? 1 : 0);
            if (!ok) {
                showToastKey('toast.folderLoadFailed');
            }
            // sampleMapUpdated push event drives the rest of the UI.
        } catch (e) {
            console.error('[sampler-app] folder load failed:', e);
        }
    });
}

// ============================================================================
// WebView file drag-drop — delegated to shared module (v1.13.0, ARCH-02)
// ============================================================================
//
// The full pattern lives in modules/core/webview-drop-streaming. The
// historical context (why C++ FileDragAndDropTarget overrides do not fire,
// why -unregisterDraggedTypes failed in v1.0.1, why the JUCE Component
// overlay failed in v1.0.2, and the v1.0.4 base64 content-streaming
// workaround) is documented in that module's README and source.
//
// We bind the document-level drop listener via the module API in the
// DOMContentLoaded handler at the bottom of this file. The plugin only
// owns the glue: drop-zone selectors, modal/toast/hover callbacks, and
// the cell midi/vel extractor.

// Plugin-specific UI helper passed to the module as opts.setDropZoneHover.
// Toggles the .drag-over class on the folder-drop-zone element so the CSS
// hover treatment fires while the cursor is over the zone during a drag.
// Also called via bindHostDragEvents (host C++→JS channel) for hosts that
// expose drag move/exit events — defence in depth for non-WKWebView paths.
function setFolderDropZoneHover (over) {
    const z = document.getElementById('folder-drop-zone');
    if (z) z.classList.toggle('drag-over', !!over);
}

// ============================================================================
// Clear-samples button + in-WebView confirmation modal (v1.0.2)
// ============================================================================
//
// JUCE does not wire the JavaScript window.confirm() through WKWebView's
// UIDelegate, so we render our own dialog (CSS in sampler-shell.css, markup
// at the end of #tab-samplemap). showConfirmDialog returns nothing — it
// invokes onConfirm on the Confirm button click, hides the dialog on Cancel
// or Escape, and supports Enter as a confirm shortcut. One-shot listeners
// are detached on every dialog close so subsequent opens do not double-fire.

// v1.12.0: format byte counts for the embed size label.
//   formatBytes(0)        → '0 B'
//   formatBytes(1024)     → '1.0 KB'
//   formatBytes(1500000)  → '1.4 MB'
//   formatBytes(2.5e9)    → '2.3 GB'
function formatBytes (n) {
    if (!Number.isFinite(n) || n <= 0) return '0 B';
    const units = ['B', 'KB', 'MB', 'GB', 'TB'];
    let i = 0;
    let v = n;
    while (v >= 1024 && i < units.length - 1) {
        v /= 1024;
        i++;
    }
    const decimals = (i === 0 || v >= 100) ? 0 : 1;
    return `${v.toFixed(decimals)} ${units[i]}`;
}

// v1.6.0: folder-load options modal. Resolves to {layer, mode, override,
// embedAudio} on Load, or null on Cancel. Surfaced before BOTH the file
// picker (button) and macOS drag-drop streaming (so cancel doesn't waste 5s
// of base64 work).
//
//   layer:      0..3 — target velocity layer
//   mode:       'append' | 'replace_layer' | 'replace_all' | 'merge_rr' (v1.9.0)
//   override:   boolean — true = ignore filename velocity tokens (v1, ff, …)
//   embedAudio: boolean — v1.12.0; true = serialise audio inline into the
//               saved project state so reload doesn't depend on the source
//               folder existing on disk
//
// v1.9.0: 'merge_rr' adds the new samples as round-robin variants on top of
// any existing cells they collide with (per (note, layer)) — useful for
// layering multiple takes/recordings as RR alternates.
//
// v1.12.0: optional sizeBytes — when known at modal time (drag-drop has
// already walked the entry tree for size), the embed-size label updates
// live as the user toggles the embed checkbox. For dialog flows (folder
// not picked yet) pass null; the size is shown in a follow-up confirmation
// modal after the file picker resolves.
async function showFolderLoadOptionsModal (sizeBytes) {
    // v1.17.0: technique dropdown options are populated from the user's
    // current technique names (renameable via the technique strip) and the
    // active tab is pre-selected. Fetched once at modal open; if names
    // change while the modal is closed, the next open re-fetches.
    let techniqueNames = ['ord', 'sp', 'st', 'sv', 'cs', 'pizz', 'harm', 'mart'];
    let activeTechnique = 0;
    try {
        const state = await invokeNative('getTechniqueState');
        if (state && Array.isArray(state.names) && state.names.length > 0) {
            techniqueNames = state.names;
        }
        if (state && Number.isFinite(state.active)) {
            activeTechnique = Math.max(0, Math.min(techniqueNames.length - 1, state.active | 0));
        }
    } catch (err) {
        console.warn('[sampler-app] getTechniqueState failed; using default technique names', err);
    }

    return new Promise((resolve) => {
        const dialog     = document.getElementById('folder-load-options-dialog');
        if (!dialog) { resolve(null); return; }

        const layerSelect      = document.getElementById('flo-layer-select');
        const techniqueSelect  = document.getElementById('flo-technique-select');
        const modeRadios       = dialog.querySelectorAll('input[name="flo-mode"]');
        const overrideEl       = document.getElementById('flo-override-checkbox');
        const overrideTechEl   = document.getElementById('flo-override-technique-checkbox');
        const embedEl          = document.getElementById('flo-embed-checkbox');
        const sizeEl           = document.getElementById('flo-embed-size');
        const explainEl        = document.getElementById('flo-explain');
        const confirmBtn       = document.getElementById('flo-confirm-btn');
        const cancelBtn        = document.getElementById('flo-cancel-btn');
        if (!layerSelect || !techniqueSelect || !modeRadios.length
                || !overrideEl || !overrideTechEl
                || !embedEl || !sizeEl
                || !explainEl || !confirmBtn || !cancelBtn) {
            resolve(null);
            return;
        }

        // Repopulate technique dropdown from the user's current names.
        // Cleared+rebuilt each open so renames are reflected immediately.
        techniqueSelect.innerHTML = '';
        techniqueNames.forEach((name, i) => {
            const opt = document.createElement('option');
            opt.value       = String(i);
            opt.textContent = name;
            techniqueSelect.appendChild(opt);
        });

        // Reset to defaults each invocation. Technique defaults to the
        // currently-active tab so workflows like "select pizz tab → drop
        // pizz folder" land where the user is already looking.
        let layer     = 0;
        let mode      = 'append';
        let technique = activeTechnique;
        layerSelect.value     = '0';
        techniqueSelect.value = String(technique);
        modeRadios.forEach((r) => { r.checked = (r.value === 'append'); });
        overrideEl.checked     = false;
        overrideTechEl.checked = false;
        embedEl.checked        = false;

        const updateSizeLabel = () => {
            // Always-on embed-size telemetry per the v1.12.0 spec — the label
            // is visible whenever the embed checkbox is on, regardless of
            // estimated size, so the user always sees the size impact.
            if (!embedEl.checked) {
                sizeEl.textContent = '';
                sizeEl.classList.remove('has-warning');
                return;
            }
            if (typeof sizeBytes === 'number' && sizeBytes > 0) {
                setLabel(sizeEl, 'label.floEmbedSize', { size: formatBytes(sizeBytes) });
                sizeEl.classList.add('has-warning');
            } else {
                // Dialog flow — folder not selected yet. The post-pick
                // confirmation modal will surface the actual size before
                // the load commits.
                setLabel(sizeEl, 'label.floEmbedSizePending');
                sizeEl.classList.remove('has-warning');
            }
        };

        const updateExplain = () => {
            const overrideOn     = overrideEl.checked;
            const overrideTechOn = overrideTechEl.checked;
            const layerStr       = velocityLayerToRange(layer).mark;
            const techName       = techniqueNames[technique]
                                   || trComposed('aria.slotN', { n: technique });
            // v1.24.0: eight separately keyed faces rather than four ternaries.
            // The key IS the branch, so nothing inflects inside a string and
            // check-i18n assertion 13 has nothing to reject.
            const EXPLAIN = {
                'append':        ['msg.floAppendForced',      'msg.floAppendTokens'],
                'replace_layer': ['msg.floReplaceLayerForced','msg.floReplaceLayerTokens'],
                'replace_all':   ['msg.floReplaceAllForced',  'msg.floReplaceAllTokens'],
                'merge_rr':      ['msg.floMergeRrForced',     'msg.floMergeRrTokens'],
            };
            const pair = EXPLAIN[mode];
            let txt = pair ? trComposed(pair[overrideOn ? 0 : 1], { layer: layerStr }) : '';
            // Technique line — appended only when the user is forcing a
            // technique. Filename-token routing is the default path and
            // doesn't need a callout.
            if (overrideTechOn) {
                txt += ' ' + trComposed('msg.floTechniqueForced', { name: techName });
            }
            explainEl.textContent = txt;
        };
        updateExplain();
        updateSizeLabel();

        const layerHandler = (e) => {
            const v = parseInt(e.target.value, 10);
            if (!Number.isFinite(v)) return;
            layer = v;
            updateExplain();
        };
        const techniqueHandler = (e) => {
            const v = parseInt(e.target.value, 10);
            if (!Number.isFinite(v)) return;
            technique = v;
            updateExplain();
        };
        const modeHandler = (e) => {
            mode = e.target.value;
            updateExplain();
        };
        const overrideHandler     = () => updateExplain();
        const overrideTechHandler = () => updateExplain();
        const embedHandler        = () => updateSizeLabel();

        // Live-preview handlers (layer / technique / mode / overrides /
        // embed) don't dismiss the dialog — they update the explanation
        // text and embed-size label as the user toggles them. Register
        // them outside bindModal and tear them down in onClose so all
        // listener removal still happens in one path.
        layerSelect.addEventListener('change', layerHandler);
        techniqueSelect.addEventListener('change', techniqueHandler);
        modeRadios.forEach((r) => r.addEventListener('change', modeHandler));
        overrideEl.addEventListener('change', overrideHandler);
        overrideTechEl.addEventListener('change', overrideTechHandler);
        embedEl.addEventListener('change', embedHandler);

        bindModal(dialog, [
            [confirmBtn, () => ({
                layer,
                mode,
                override:           overrideEl.checked,
                technique,
                overrideTechnique:  overrideTechEl.checked,
                embedAudio:         embedEl.checked,
            })],
            [cancelBtn,  () => null],
        ], {
            keys:    { Escape: cancelBtn, Enter: confirmBtn },
            focus:   confirmBtn,
            onClose: () => {
                layerSelect.removeEventListener('change', layerHandler);
                techniqueSelect.removeEventListener('change', techniqueHandler);
                modeRadios.forEach((r) => r.removeEventListener('change', modeHandler));
                overrideEl.removeEventListener('change', overrideHandler);
                overrideTechEl.removeEventListener('change', overrideTechHandler);
                embedEl.removeEventListener('change', embedHandler);
            },
        }).then(resolve);
    });
}

// v1.12.0: post-pick embed-size confirmation modal (dialog flow only). Used
// when the user opted in to embed in the options modal, and the folder
// they then picked is large enough that they may want to back out. Resolves
// true on Confirm, false on Cancel/Escape.
function showEmbedSizeConfirmModal (sizeBytes, displayName) {
    return new Promise((resolve) => {
        const dialog     = document.getElementById('embed-size-confirm-dialog');
        const messageEl  = document.getElementById('embed-size-confirm-message');
        const confirmBtn = document.getElementById('embed-size-confirm-btn');
        const cancelBtn  = document.getElementById('embed-size-cancel-btn');
        if (!dialog || !messageEl || !confirmBtn || !cancelBtn) {
            // v1.23.7 (IN-05): modal DOM missing = markup regression.
            // window.confirm is NOT wired through WKWebView's UIDelegate
            // (returns undefined silently), so the old confirm() fallback
            // could never actually ask — fail safe: cancel the embed and
            // surface the failure instead.
            console.error('[sampler-app] embed-size-confirm modal DOM missing — cancelling embed');
            showToastKey('toast.embedDialogMissing');
            resolve(false);
            return;
        }

        // Two keyed faces rather than an interpolated fragment: the named and
        // unnamed forms are different SENTENCES in French, not one sentence
        // with a hole in it.
        const embedVars = { name: displayName, size: formatBytes(sizeBytes) };
        if (displayName) setLabel(messageEl, 'label.embedMsgNamed',   embedVars);
        else             setLabel(messageEl, 'label.embedMsgUnnamed', embedVars);

        bindModal(dialog, [
            [confirmBtn, () => true],
            [cancelBtn,  () => false],
        ], { keys: { Escape: cancelBtn, Enter: confirmBtn }, focus: confirmBtn })
            .then(resolve);
    });
}


// v1.9.0: per-cell merge prompt. Surfaced when the user attempts a per-cell
// single-file load on a non-empty cell. Resolves to:
//   'merge'   — append the new sample as a round-robin variant.
//   'replace' — drop the cell's variants and replace with this single sample
//               (v1.8.0 behaviour, current default for explicit replace).
//   null      — user cancelled.
function showPerCellMergeDialog (existingCount, midi, layer) {
    return new Promise((resolve) => {
        const dialog    = document.getElementById('per-cell-merge-dialog');
        const messageEl = document.getElementById('per-cell-merge-message');
        const mergeBtn  = document.getElementById('per-cell-merge-merge-btn');
        const replBtn   = document.getElementById('per-cell-merge-replace-btn');
        const cancelBtn = document.getElementById('per-cell-merge-cancel-btn');
        if (!dialog || !messageEl || !mergeBtn || !replBtn || !cancelBtn) {
            resolve(null);
            return;
        }

        const noteName = (typeof midiToNoteName === 'function')
            ? midiToNoteName(midi) : trComposed('aria.midiN', { n: midi });
        const capHit = existingCount >= 64;
        const mark = velocityLayerToRange(layer).mark;
        // v1.24.0 (§6): `existingCount === 1 ? '1 variant' : '${n} variants'`
        // is gone. It was the repo's only inline English pluralization outside
        // the four above, and porting the ternary would be wrong in French at
        // 0 and inconsistent at 1. The count now sits after a colon beside an
        // invariant plural noun phrase, in both faces.
        const mergeVars = { note: noteName, mark, n: existingCount, next: existingCount + 1 };
        if (capHit) setLabel(messageEl, 'label.mergeMsgCapped', mergeVars);
        else        setLabel(messageEl, 'label.mergeMsgAdd',    mergeVars);
        mergeBtn.disabled = capHit;
        mergeBtn.style.opacity = capHit ? '0.4' : '';
        mergeBtn.style.cursor  = capHit ? 'not-allowed' : '';

        // mergeBtn is disabled when capHit, so its bound handler will not fire
        // from a mouse click. The dynamic Enter-target callback below routes
        // Enter to replBtn in that state instead.
        bindModal(dialog, [
            [mergeBtn,  () => 'merge'],
            [replBtn,   () => 'replace'],
            [cancelBtn, () => null],
        ], {
            keys:  {
                Escape: cancelBtn,
                Enter:  () => (capHit ? replBtn : mergeBtn),
            },
            focus: capHit ? replBtn : mergeBtn,
        }).then(resolve);
    });
}

function showConfirmDialog ({ title, message, confirmLabel, destructive, onConfirm }) {
    const dialog    = document.getElementById('confirm-dialog');
    const titleEl   = document.getElementById('confirm-dialog-title');
    const messageEl = document.getElementById('confirm-dialog-message');
    const confirmEl = document.getElementById('confirm-confirm-btn');
    const cancelEl  = document.getElementById('confirm-cancel-btn');
    if (!dialog || !titleEl || !messageEl || !confirmEl || !cancelEl) return;

    // The three defaults are the ones index.html already authors on these
    // elements, so they are keyed there and re-applied here through the same
    // keys rather than re-spelled as raw English.
    if (title) titleEl.textContent = title; else setLabel(titleEl, 'label.areYouSure');
    messageEl.textContent = message || '';
    if (confirmLabel) confirmEl.textContent = confirmLabel;
    else setLabel(confirmEl, 'label.confirm');
    confirmEl.classList.toggle('destructive', !!destructive);

    bindModal(dialog, [
        [confirmEl, () => true],
        [cancelEl,  () => false],
    ], { keys: { Escape: cancelEl, Enter: confirmEl }, focus: cancelEl })
        .then(async (yes) => { if (yes && onConfirm) await onConfirm(); });
}

// v1.0.3: diagnostic dialog used by the failed-drop path. Auto-copies the
// dump to the clipboard on open so the user doesn't have to chase a toast,
// and exposes a selectable <pre> fallback for hosts where the clipboard API
// is gated.
async function showDiagnosticDialog (title, text) {
    const dialog   = document.getElementById('diagnostic-dialog');
    const titleEl  = document.getElementById('diagnostic-dialog-title');
    const hintEl   = document.getElementById('diagnostic-dialog-hint');
    const textEl   = document.getElementById('diagnostic-dialog-text');
    const copyBtn  = document.getElementById('diagnostic-copy-btn');
    const closeBtn = document.getElementById('diagnostic-close-btn');
    if (!dialog || !titleEl || !textEl || !copyBtn || !closeBtn) return;

    if (title) titleEl.textContent = title; else setLabel(titleEl, 'label.diagnostic');
    textEl.textContent  = text  || '';
    setLabel(copyBtn, 'label.copyAgain');

    const writeClipboard = async () => {
        // Primary: async Clipboard API (requires user activation, which the
        // drop event provides).
        if (navigator?.clipboard?.writeText) {
            try { await navigator.clipboard.writeText(text); return true; }
            catch (_) { /* fall through to execCommand fallback */ }
        }
        // Fallback: select the <pre> contents and execCommand('copy').
        try {
            const range = document.createRange();
            range.selectNodeContents(textEl);
            const sel = window.getSelection();
            sel.removeAllRanges();
            sel.addRange(range);
            const ok = document.execCommand('copy');
            sel.removeAllRanges();
            return ok;
        } catch (_) { return false; }
    };

    const autoCopied = await writeClipboard();
    if (hintEl) {
        if (autoCopied) setLabel(hintEl, 'label.diagCopied');
        else            setLabel(hintEl, 'label.diagCopyBlocked');
    }

    // Copy button doesn't dismiss — register externally; bindModal's onClose
    // hook removes it when the user finally closes the dialog.
    const onCopy = async () => {
        const ok = await writeClipboard();
        if (ok) setLabel(copyBtn, 'label.copied');
        else    setLabel(copyBtn, 'label.copyFailed');
        setTimeout(() => setLabel(copyBtn, 'label.copyAgain'), 1400);
    };
    copyBtn.addEventListener('click', onCopy);

    bindModal(dialog, [
        [closeBtn, () => undefined],
    ], {
        keys:    { Escape: closeBtn },   // no Enter binding — Enter shouldn't dismiss
        focus:   closeBtn,
        onClose: () => copyBtn.removeEventListener('click', onCopy),
    });
}

function bindClearSamplesButton() {
    const btn = document.getElementById('clear-samples-btn');
    if (!btn) return;

    btn.addEventListener('click', () => {
        if (btn.disabled) return;
        showConfirmDialog({
            title:        trComposed('msg.clearAllTitle'),
            message:      trComposed('msg.clearAllBody'),
            confirmLabel: trComposed('msg.clearBtn'),
            destructive:  true,
            onConfirm:    async () => {
                // sampleMapUpdated push event drives grid + button state
                // refresh — no further work needed here.
                await invokeNative('clearSampleMap');
            },
        });
    });
}

function bindHostDragEvents() {
    if (!window.__JUCE__ || !window.__JUCE__.backend) return;

    const zone = () => document.getElementById('folder-drop-zone');

    const setHover = (over) => {
        const z = zone();
        if (!z) return;
        z.classList.toggle('drag-over', !!over);
    };

    try {
        window.__JUCE__.backend.addEventListener('hostFileDragMove', (payload) => {
            // Payload arrives parsed: {x, y}.
            const z = zone();
            if (!z) return;
            const r = z.getBoundingClientRect();
            const inside = payload && typeof payload.x === 'number' && typeof payload.y === 'number'
                ? (payload.x >= r.left && payload.x < r.right
                   && payload.y >= r.top && payload.y < r.bottom)
                : false;
            setHover(inside);
        });

        window.__JUCE__.backend.addEventListener('hostFileDragExit', () => {
            setHover(false);
        });
    } catch (e) {
        console.warn('[sampler-app] host drag event subscription failed:', e);
    }
}

// ============================================================================
// Toast queue (Phase 3.3 Task 21)
// ============================================================================
//
// Single-element queue: each toast displays for 3 s, fades in/out via CSS.
// New toasts replace the previous one (re-arming the timer). Subscribed to
// the C++ "toast" event so file-drop routing (filesDropped) can surface
// invalid-target messages without a custom JS round-trip.

let toastTimer = null;

// v1.24.0: the KEYED toast. showToast() below still takes a raw string,
// because the C++ side and the shared webview-drop-streaming module both push
// finished text through it (see I18N_EXEMPT for why those stay English); every
// toast this file authors goes through showToastKey instead.
function showToastKey(key, vars) {
    showToast(trComposed(key, vars));
}

function showToast(message) {
    const region = document.getElementById('toast-region');
    if (!region) return;

    // Replace any existing toast element to keep the queue single-element.
    region.innerHTML = '';

    const el = document.createElement('div');
    el.className = 'toast';
    el.textContent = message;
    region.appendChild(el);

    // Trigger entrance transition on next frame.
    requestAnimationFrame(() => el.classList.add('show'));

    if (toastTimer) clearTimeout(toastTimer);
    toastTimer = setTimeout(() => {
        el.classList.remove('show');
        // Remove from DOM after the fade-out completes (250 ms — matches
        // the CSS transition duration plus a small safety margin).
        setTimeout(() => {
            if (el.parentNode === region) region.removeChild(el);
        }, 300);
        toastTimer = null;
    }, 3000);
}

function bindToastEventListener() {
    if (!window.__JUCE__ || !window.__JUCE__.backend) return;
    try {
        window.__JUCE__.backend.addEventListener('toast', (payload) => {
            // Payload may be a parsed string (single arg) or an object; for
            // file-drop emit we send a bare String.
            const msg = (typeof payload === 'string') ? payload
                       : (payload && typeof payload === 'object' && 'message' in payload)
                            ? payload.message
                            : String(payload || '');
            if (msg) showToast(msg);
        });
    } catch (e) {
        console.warn('[sampler-app] toast event subscription failed:', e);
    }
}

// ============================================================================
// Loop-point editor (Phase 3.4 — DSP-06 + UI-02)
// ============================================================================
//
// On openLoopEditor(midi, vel):
//   - await Juce.getNativeFunction('getWaveformPeaks')(midi, vel, 512)
//   - parse JSON snapshot per RESEARCH §RQ3-5 schema
//   - render min/max envelope on DPR-aware canvas
//   - draw two draggable vertical markers (start, end) at sample positions
//   - Apply → Juce.getNativeFunction('overrideLoopPoints')(midi, vel, start, end, 8)
//   - Reset → Juce.getNativeFunction('resetLoopToAutoDetect')(midi, vel) → re-fetch
//   - Cancel / X / Esc → close panel, no writeback
//   - Reset disabled when loopMode === "one-shot" (EC3-7)
//
// Markers are pure JS overlay — drag does NOT re-call getWaveformPeaks; the
// peak data is cached in `editorState.snap` for the duration of the panel
// session. Apply persists; Cancel discards.

const LOOP_EDITOR_BINS = 512;
const MARKER_HIT_PX = 8;       // Within this many CSS px of marker → grab handle
const APPLY_TOAST_KEY = 'toast.loopApplyNote';
// v1.24.0: the one-shot string is no longer a native title (contract §4) — it
// is a keyed accessible name applied by a LITERAL dataset write in
// updateResetButtonState(). It is not hoisted into a const here, because
// check-i18n assertion 15 only counts a PLAIN STRING LITERAL on the right of
// `.dataset.i18nAria =` as a reference — a const reads as a computed key and
// reports the entry dead, which is the gate describing a violation of a rule
// the code is obeying.

const editorState = {
    open: false,
    midi: -1,
    vel: -1,
    snap: null,           // Last fetched JSON snapshot (peaks + meta).
    loopStart: 0,         // Current marker positions (samples).
    loopEnd: 0,
    dragMarker: null,     // 'start' | 'end' | null
    pointerId: -1,
    variantIndex: 0,      // v1.8.0: which variant we're currently editing.
    variantCount: 1,      // v1.8.0: total variants in the cell.
};

function isOneShot(snap) {
    if (!snap || typeof snap.loopMode !== 'string') return false;
    const m = snap.loopMode.toLowerCase();
    return m === 'one-shot' || m === 'oneshot';
}

async function openLoopEditor(midi, vel, variantIndex = 0) {
    if (!window.__JUCE__) return;

    try {
        const fn = Juce.getNativeFunction('getWaveformPeaks');
        const json = await fn(midi, vel, LOOP_EDITOR_BINS, variantIndex);
        const snap = (typeof json === 'string') ? JSON.parse(json) : json;
        if (!snap || !Array.isArray(snap.peaks) || snap.peaks.length === 0) {
            console.warn('[sampler-app] openLoopEditor: empty peaks snapshot', snap);
            showToastKey('toast.waveformUnavailable');
            return;
        }
        editorState.open = true;
        editorState.midi = midi;
        editorState.vel = vel;
        editorState.snap = snap;
        editorState.loopStart = num(snap.loopStart);
        editorState.loopEnd   = num(snap.loopEnd);

        // v1.8.0: variant tab strip surfaces when the cell has > 1 variant.
        editorState.variantIndex = num(snap.variantIndex, variantIndex);
        editorState.variantCount = num(snap.variantCount, 1);

        populateLoopEditorHeader(snap);
        renderVariantTabStrip();
        showLoopEditorPanel();

        // Defer canvas draw one frame so the panel transition has applied
        // and clientWidth/clientHeight reflect the open state.
        requestAnimationFrame(() => {
            redrawLoopEditor();
        });
    } catch (e) {
        console.error('[sampler-app] openLoopEditor failed:', e);
    }
}

// v1.8.0: variant tab strip — one tab per variant. Shown only when cell
// has > 1 variant. Click switches the active variant; the snapshot reload
// happens via openLoopEditor with the selected index. Loop points are
// per-variant in v1.8.0, so each tab carries its own state on the C++ side.
function renderVariantTabStrip() {
    const wrap = document.getElementById('le-variant-tabs');
    if (!wrap) return;

    wrap.innerHTML = '';
    if (editorState.variantCount <= 1) {
        wrap.style.display = 'none';
        return;
    }
    wrap.style.display = '';

    const label = document.createElement('span');
    label.className = 'le-variant-label';
    setLabel(label, 'label.variantOf',
             { i: editorState.variantIndex + 1, n: editorState.variantCount });
    wrap.appendChild(label);

    for (let i = 0; i < editorState.variantCount; ++i) {
        const tab = document.createElement('button');
        tab.type = 'button';
        tab.className = 'le-variant-tab';
        if (i === editorState.variantIndex) tab.classList.add('active');
        tab.textContent = String(i + 1);
        // v1.24.0: native title= DELETED (§4). The tab's own text is a bare
        // ordinal, so the affordance is its only help: composed accessible
        // name, re-composed by refreshComposedUi() on a language change.
        const tabAria = trComposed('aria.switchToVariant', { n: i + 1 });
        tab.setAttribute('aria-label', tabAria);
        tab.addEventListener('click', () => {
            if (i === editorState.variantIndex) return;
            // Re-open with the chosen variant — fresh peaks + loop points.
            openLoopEditor(editorState.midi, editorState.vel, i);
        });
        wrap.appendChild(tab);
    }
}

function populateLoopEditorHeader(snap) {
    const fnEl    = document.getElementById('le-filename');
    const midiEl  = document.getElementById('le-midi');
    const velEl   = document.getElementById('le-vel');
    const modeEl  = document.getElementById('le-loop-mode');
    const unknownName = trComposed('aria.unknown');
    const midiText    = trComposed('aria.midiN', { n: snap.midiNote });
    if (fnEl)   fnEl.textContent   = snap.filename || unknownName;
    if (midiEl) midiEl.textContent = midiText;
    if (velEl)  velEl.textContent  = velocityLayerToRange(snap.velocityLayer).mark;
    if (modeEl) modeEl.textContent = snap.loopMode || '—';
    updateLoopMetaLabels();
    updateResetButtonState(snap);
}

function updateLoopMetaLabels() {
    const snap = editorState.snap;
    if (!snap) return;
    const sr = snap.sourceSampleRate > 0 ? snap.sourceSampleRate : 48000;
    const startMs = (editorState.loopStart / sr) * 1000.0;
    const endMs   = (editorState.loopEnd   / sr) * 1000.0;
    const sEl = document.getElementById('le-loop-start-ms');
    const eEl = document.getElementById('le-loop-end-ms');
    if (sEl) sEl.textContent = startMs.toFixed(1);
    if (eEl) eEl.textContent = endMs.toFixed(1);
}

function updateResetButtonState(snap) {
    const btn = document.getElementById('loop-reset');
    if (!btn) return;
    if (isOneShot(snap)) {
        btn.disabled = true;
        // Literal `.dataset.i18nAria =` — the shape assertion 15 collects, so
        // the key reads LIVE. The wording is v1.23.10's own, verbatim.
        btn.dataset.i18nAria = 'aria.loopResetOneShot';
        applyI18nAttributes(btn);
    } else {
        btn.disabled = false;
        delete btn.dataset.i18nAria;
        btn.removeAttribute('aria-label');
    }
}

// v1.5.0 — panel is inline + always visible. show/close just toggle the
// .no-selection class which swaps between placeholder and full editor.
function showLoopEditorPanel() {
    const panel = document.getElementById('loop-editor-panel');
    if (!panel) return;
    panel.classList.remove('no-selection');
    // No grid-width change anymore (panel doesn't overlap the grid), but
    // republish layout in case the panel's appearance shifts cell rects.
    requestAnimationFrame(() => publishCellLayout());
}

function closeLoopEditor() {
    const panel = document.getElementById('loop-editor-panel');
    if (panel) panel.classList.add('no-selection');
    editorState.open = false;
    editorState.snap = null;
    editorState.dragMarker = null;
    editorState.pointerId = -1;
    requestAnimationFrame(() => publishCellLayout());
}

function redrawLoopEditor() {
    const canvas = document.getElementById('waveform-canvas');
    if (!canvas || !editorState.snap) return;

    // DPR-aware backing store (memory pitfall #6).
    const dpr = window.devicePixelRatio || 1;
    const cssW = Math.max(1, Math.floor(canvas.clientWidth));
    const cssH = Math.max(1, Math.floor(canvas.clientHeight));
    const targetW = Math.round(cssW * dpr);
    const targetH = Math.round(cssH * dpr);
    if (canvas.width !== targetW)  canvas.width  = targetW;
    if (canvas.height !== targetH) canvas.height = targetH;

    const ctx = canvas.getContext('2d');
    if (!ctx) return;
    ctx.setTransform(dpr, 0, 0, dpr, 0, 0);

    // Background
    ctx.clearRect(0, 0, cssW, cssH);
    const bgGrad = ctx.createLinearGradient(0, 0, 0, cssH);
    bgGrad.addColorStop(0, 'rgba(245, 230, 211, 0.3)');
    bgGrad.addColorStop(1, 'rgba(235, 217, 199, 0.5)');
    ctx.fillStyle = bgGrad;
    ctx.fillRect(0, 0, cssW, cssH);

    // Centerline
    ctx.strokeStyle = 'rgba(139, 115, 85, 0.35)';
    ctx.lineWidth = 1;
    ctx.beginPath();
    ctx.moveTo(0, cssH / 2);
    ctx.lineTo(cssW, cssH / 2);
    ctx.stroke();

    // Min/max envelope
    const peaks = editorState.snap.peaks;
    const bins = peaks.length;
    if (bins === 0) return;
    const binW = cssW / bins;

    // Filled area
    ctx.fillStyle = 'rgba(184, 134, 11, 0.35)';  // antique-gold @ 35 %
    ctx.beginPath();
    for (let i = 0; i < bins; ++i) {
        const px = i * binW;
        const max = peaks[i][1];
        const y = cssH * 0.5 - max * (cssH * 0.48);
        if (i === 0) ctx.moveTo(px, y);
        else ctx.lineTo(px, y);
    }
    for (let i = bins - 1; i >= 0; --i) {
        const px = i * binW;
        const min = peaks[i][0];
        const y = cssH * 0.5 - min * (cssH * 0.48);
        ctx.lineTo(px, y);
    }
    ctx.closePath();
    ctx.fill();

    // Stroke outline (warm-brown)
    ctx.strokeStyle = '#5C4033';
    ctx.lineWidth = 1;
    ctx.beginPath();
    for (let i = 0; i < bins; ++i) {
        const px = i * binW;
        const max = peaks[i][1];
        const y = cssH * 0.5 - max * (cssH * 0.48);
        if (i === 0) ctx.moveTo(px, y);
        else ctx.lineTo(px, y);
    }
    ctx.stroke();
    ctx.beginPath();
    for (let i = 0; i < bins; ++i) {
        const px = i * binW;
        const min = peaks[i][0];
        const y = cssH * 0.5 - min * (cssH * 0.48);
        if (i === 0) ctx.moveTo(px, y);
        else ctx.lineTo(px, y);
    }
    ctx.stroke();

    // Markers — skipped for one-shot cells (IN-03: no loop region; both
    // markers would otherwise render stacked at x=0).
    if (!isOneShot(editorState.snap)) {
        drawMarker(ctx, cssW, cssH, sampleToX(editorState.loopStart, cssW), 'start');
        drawMarker(ctx, cssW, cssH, sampleToX(editorState.loopEnd,   cssW), 'end');
    }
}

function drawMarker(ctx, cssW, cssH, x, which) {
    if (!Number.isFinite(x)) return;
    const colour = which === 'start' ? '#B8860B' : '#C0392B';
    ctx.strokeStyle = colour;
    ctx.fillStyle = colour;
    ctx.lineWidth = 2;
    ctx.beginPath();
    ctx.moveTo(x, 0);
    ctx.lineTo(x, cssH);
    ctx.stroke();

    // Handle triangle at top
    const handleSize = 6;
    ctx.beginPath();
    ctx.moveTo(x - handleSize, 0);
    ctx.lineTo(x + handleSize, 0);
    ctx.lineTo(x, handleSize);
    ctx.closePath();
    ctx.fill();
}

function sampleToX(sample, cssW) {
    const snap = editorState.snap;
    if (!snap || !snap.lengthSamples) return 0;
    return (sample / snap.lengthSamples) * cssW;
}

function xToSample(x, cssW) {
    const snap = editorState.snap;
    if (!snap || !snap.lengthSamples) return 0;
    const s = Math.round((x / cssW) * snap.lengthSamples);
    return Math.max(0, Math.min(snap.lengthSamples - 1, s));
}

function bindLoopEditorEvents() {
    const closeBtn  = document.getElementById('loop-close');
    const resetBtn  = document.getElementById('loop-reset');
    const cancelBtn = document.getElementById('loop-cancel');
    const applyBtn  = document.getElementById('loop-apply');
    const canvas    = document.getElementById('waveform-canvas');

    if (closeBtn)  closeBtn.addEventListener('click', closeLoopEditor);
    if (cancelBtn) cancelBtn.addEventListener('click', closeLoopEditor);

    if (resetBtn) {
        resetBtn.addEventListener('click', async () => {
            if (resetBtn.disabled || !editorState.open) return;
            try {
                const fn = Juce.getNativeFunction('resetLoopToAutoDetect');
                await fn(editorState.midi, editorState.vel, editorState.variantIndex);
                // Re-fetch peaks to get fresh marker positions + mode.
                const get = Juce.getNativeFunction('getWaveformPeaks');
                const json = await get(editorState.midi, editorState.vel,
                                       LOOP_EDITOR_BINS, editorState.variantIndex);
                const snap = (typeof json === 'string') ? JSON.parse(json) : json;
                if (snap && Array.isArray(snap.peaks)) {
                    editorState.snap = snap;
                    editorState.loopStart = num(snap.loopStart);
                    editorState.loopEnd   = num(snap.loopEnd);
                    populateLoopEditorHeader(snap);
                    redrawLoopEditor();
                }
            } catch (e) {
                console.error('[sampler-app] resetLoopToAutoDetect failed:', e);
            }
        });
    }

    if (applyBtn) {
        applyBtn.addEventListener('click', async () => {
            if (!editorState.open) return;
            try {
                const fn = Juce.getNativeFunction('overrideLoopPoints');
                await fn(editorState.midi, editorState.vel,
                         editorState.loopStart, editorState.loopEnd, 8,
                         editorState.variantIndex);
                showToastKey(APPLY_TOAST_KEY);
                // Don't auto-close — user may want to keep iterating.
                // The sampleMapUpdated push event will refresh the grid;
                // editor stays open with current values reflected.
            } catch (e) {
                console.error('[sampler-app] overrideLoopPoints failed:', e);
            }
        });
    }

    if (canvas) {
        canvas.addEventListener('pointerdown', onCanvasPointerDown);
        canvas.addEventListener('pointermove', onCanvasPointerMove);
        canvas.addEventListener('pointerup',   onCanvasPointerUp);
        canvas.addEventListener('pointercancel', onCanvasPointerUp);
    }

    // Esc closes the editor.
    document.addEventListener('keydown', (e) => {
        if (e.key === 'Escape' && editorState.open) {
            e.preventDefault();
            closeLoopEditor();
        }
    });
}

function onCanvasPointerDown(e) {
    if (!editorState.open || !editorState.snap) return;
    const canvas = e.currentTarget;
    const rect = canvas.getBoundingClientRect();
    const x = e.clientX - rect.left;
    const cssW = rect.width;

    // Hit-test against the two markers; prefer the closer one.
    const startX = sampleToX(editorState.loopStart, cssW);
    const endX   = sampleToX(editorState.loopEnd,   cssW);
    const dStart = Math.abs(x - startX);
    const dEnd   = Math.abs(x - endX);
    const HIT = MARKER_HIT_PX;

    let target = null;
    if (dStart <= HIT && dEnd <= HIT) {
        target = (dStart <= dEnd) ? 'start' : 'end';
    } else if (dStart <= HIT) {
        target = 'start';
    } else if (dEnd <= HIT) {
        target = 'end';
    }

    if (target) {
        editorState.dragMarker = target;
        editorState.pointerId = e.pointerId;
        canvas.setPointerCapture(e.pointerId);
        e.preventDefault();
    }
}

function onCanvasPointerMove(e) {
    if (!editorState.dragMarker || e.pointerId !== editorState.pointerId) return;
    const canvas = e.currentTarget;
    const rect = canvas.getBoundingClientRect();
    const x = e.clientX - rect.left;
    const cssW = rect.width;
    let s = xToSample(x, cssW);

    // Maintain ordering: start < end with a 16-sample minimum gap (matches
    // the LoopDetector's defensive guard so we don't allow degenerate loops).
    const MIN_GAP = 16;
    const lengthSamples = editorState.snap.lengthSamples;
    if (editorState.dragMarker === 'start') {
        s = Math.max(0, Math.min(s, editorState.loopEnd - MIN_GAP));
        editorState.loopStart = s;
    } else {
        s = Math.max(editorState.loopStart + MIN_GAP, Math.min(s, lengthSamples));
        editorState.loopEnd = s;
    }

    updateLoopMetaLabels();
    redrawLoopEditor();
}

function onCanvasPointerUp(e) {
    if (e.pointerId !== editorState.pointerId) return;
    const canvas = e.currentTarget;
    if (canvas.hasPointerCapture && canvas.hasPointerCapture(e.pointerId)) {
        try { canvas.releasePointerCapture(e.pointerId); } catch (_) { /* ignore */ }
    }
    editorState.dragMarker = null;
    editorState.pointerId = -1;
}

// Re-publish cell layout AND redraw the loop editor on resize. The canvas
// CSS sizing is fluid (calc(100% - 0px)) so its clientWidth changes with
// the panel; we need to update the backing store + rerender the envelope.
function bindLoopEditorResize() {
    if (typeof ResizeObserver !== 'function') return;
    const canvas = document.getElementById('waveform-canvas');
    if (!canvas) return;
    const obs = new ResizeObserver(() => {
        if (editorState.open) requestAnimationFrame(() => redrawLoopEditor());
    });
    obs.observe(canvas);
}

// ============================================================================
// v1.3.0 — Save/Load preset buttons + missing-folder modal
// ============================================================================
//
// The plugin already round-trips full state (params + sample folder + tuning)
// through DAW project save/load via PluginProcessor::get/setStateInformation.
// The Save/Load preset buttons surface that same ValueTree as a portable
// .omspreset file so users can share configurations across projects on the
// same machine (path-only — sample data is referenced, not embedded).
//
// Missing-folder modal: when DAW project reopen tries to restore a folder
// path that no longer exists, the C++ side fires a `folderMissing` WebView
// event AND parks the path in the processor for the boot-time race case
// (state restore happens before the WebView attaches its listener). On boot
// we register the listener first, then pull any pending state — covers both.

function bindPresetButtons () {
    const saveBtn = document.getElementById('save-preset-btn');
    const loadBtn = document.getElementById('load-preset-btn');

    if (saveBtn) {
        saveBtn.addEventListener('click', async () => {
            if (!window.__JUCE__) return;
            try {
                const fn = Juce.getNativeFunction('saveCurrentPreset');
                const ok = await fn();
                if (ok === true) showToastKey('toast.presetSaved');
                // Cancel resolves false — silent (user dismissed the picker).
            } catch (e) {
                console.error('[sampler-app] saveCurrentPreset failed:', e);
                showToastKey('toast.presetSaveFailed');
            }
        });
    }

    if (loadBtn) {
        loadBtn.addEventListener('click', async () => {
            if (!window.__JUCE__) return;
            try {
                const fn = Juce.getNativeFunction('loadPreset');
                const ok = await fn();
                if (ok === true) {
                    showToastKey('toast.presetLoaded');
                    // The processor's sampleMapUpdated push covers the grid;
                    // refresh the tuning readout (tab-tuning lazy-mounts the
                    // panel, but the header readout polls on demand).
                    refreshTuningReadout();
                    if (tuningPanelInstance && typeof tuningPanelInstance.refresh === 'function') {
                        try { await tuningPanelInstance.refresh(); } catch (_) { /* ignore */ }
                    }
                } else if (ok === false) {
                    // false from a non-cancel path means parse/restore failed;
                    // cancel also resolves false, so don't toast on that.
                }
            } catch (e) {
                console.error('[sampler-app] loadPreset failed:', e);
                showToastKey('toast.presetLoadFailed');
            }
        });
    }
}

// v1.12.0: payload is now an object {path, kind, name} (was a bare string
// in v1.3.0–v1.11.x). String-form payloads are normalised in
// `subscribeFolderMissingEvent` and `pullPendingMissingFolder` before
// reaching this function.
//
//   kind="filesystem" — original behaviour: saved disk path is gone. The
//                       modal shows the path and offers a Locate folder
//                       picker that delegates to the existing reload flow.
//   kind="drag-drop"  — v1.12.0: the load was a WebView drag-drop with no
//                       embed. The temp dir is reaped at next-session-
//                       start, so we never had a stable path to surface.
//                       Show the original folder name (lifted from
//                       FileSystemEntry::name at drop time) and direct the
//                       user to re-drag or browse.
async function showMissingFolderDialog (info) {
    const dialog    = document.getElementById('missing-folder-dialog');
    const titleEl   = document.getElementById('missing-folder-dialog-title');
    const messageEl = document.getElementById('missing-folder-dialog-message');
    const pathEl    = document.getElementById('missing-folder-saved-path');
    const skipBtn   = document.getElementById('missing-folder-skip-btn');
    const locateBtn = document.getElementById('missing-folder-locate-btn');
    if (!dialog || !titleEl || !messageEl || !pathEl || !skipBtn || !locateBtn) return;

    const safe      = (info && typeof info === 'object') ? info : {};
    const path      = typeof safe.path === 'string' ? safe.path : '';
    const kind      = typeof safe.kind === 'string' ? safe.kind : 'filesystem';
    const givenName = typeof safe.name === 'string' ? safe.name : '';

    // These two are the highest-value strings on the page: they are what a
    // French user reads when their session comes back broken. Both faces of
    // each are keyed separately rather than interpolated, so neither has to
    // read as a sentence with a hole in it.
    if (kind === 'drag-drop') {
        setLabel(titleEl, 'label.dragDropNotEmbedded');
        if (givenName) setLabel(messageEl, 'label.dragDropMsgNamed', { name: givenName });
        else           setLabel(messageEl, 'label.dragDropMsgUnnamed');
        pathEl.textContent = '';   // no path to show; clear the legacy slot
        pathEl.style.display = 'none';
        setLabel(locateBtn, 'label.browseForFolder');
    } else {
        setLabel(titleEl, 'label.folderNotFound');
        // Derive a friendly folder name from the saved path for the message
        // (or use the explicit name when provided).
        let folderName = givenName;
        if (!folderName && path) {
            const sep = path.lastIndexOf('/') >= 0 ? '/' : '\\';
            const idx = path.lastIndexOf(sep);
            folderName = idx >= 0 ? path.substring(idx + 1) : path;
        }
        if (folderName) setLabel(messageEl, 'label.folderNotFoundMsgNamed', { name: folderName });
        else            setLabel(messageEl, 'label.folderNotFoundMsgUnnamed');
        const emptyPathText = trComposed('aria.emptyPath');
        pathEl.textContent = path || emptyPathText;
        pathEl.style.display = '';
        setLabel(locateBtn, 'label.locateFolder');
    }

    // Dialog dismisses synchronously on click; the chosen action runs after
    // bindModal resolves so the modal isn't visually held while the native
    // OS picker (locate path) is in flight.
    const action = await bindModal(dialog, [
        [skipBtn,   () => 'skip'],
        [locateBtn, () => 'locate'],
    ], { keys: { Escape: skipBtn, Enter: locateBtn }, focus: locateBtn });

    if (action === 'skip') {
        await invokeNative('dismissMissingFolder');
    } else if (action === 'locate') {
        if (!window.__JUCE__) return;
        try {
            const fn = Juce.getNativeFunction('locateMissingFolder');
            const ok = await fn();
            // Cancel resolves false — leave the pending state intact so the
            // user can try again from the next setStateInformation event or
            // by manually using "Load Folder…".
            if (ok === true) showToastKey('toast.folderLocated');
        } catch (e) {
            console.error('[sampler-app] locateMissingFolder failed:', e);
            showToastKey('toast.locateFolderFailed');
        }
    }
}

function subscribeFolderMissingEvent () {
    if (!window.__JUCE__ || !window.__JUCE__.backend) return;
    try {
        window.__JUCE__.backend.addEventListener('folderMissing', (payload) => {
            // v1.12.0: payload is now an object {path, kind, name}. Older
            // string-form payloads are no longer emitted by the plugin, but
            // we accept them defensively (host could replay an old event).
            let info;
            if (payload && typeof payload === 'object') {
                info = payload;
            } else if (typeof payload === 'string') {
                info = { path: payload, kind: 'filesystem', name: '' };
            } else {
                info = { path: '', kind: 'filesystem', name: '' };
            }
            showMissingFolderDialog(info);
        });
    } catch (e) {
        console.warn('[sampler-app] folderMissing subscription failed:', e);
    }
}

// v1.8.0: ambiguous-duplicates modal. Fired when a folder load contained
// multiple files for the same (midi, layer) without rr/take/tk tokens.
// User decides: treat as round-robin variants OR cancel the load.
function showAmbiguousDuplicatesDialog (dups) {
    const dialog  = document.getElementById('rr-confirm-dialog');
    const listEl  = document.getElementById('rr-confirm-list');
    const okBtn   = document.getElementById('rr-confirm-accept');
    const noBtn   = document.getElementById('rr-confirm-cancel');
    if (!dialog || !listEl || !okBtn || !noBtn) {
        // v1.23.7 (IN-05): modal DOM missing = markup regression.
        // window.confirm is dead in WKWebView (no UIDelegate wiring — it
        // returns undefined), so the old fallback silently sent a "cancel"
        // the user never saw. Cancel explicitly and surface the failure.
        console.error('[sampler-app] rr-confirm modal DOM missing — cancelling ambiguous-duplicate load');
        showToastKey('toast.rrDialogMissing');
        sendRrConfirmation(false);
        return;
    }

    listEl.innerHTML = '';
    for (const d of dups) {
        const item = document.createElement('li');
        const head = document.createElement('div');
        head.className = 'rr-confirm-cell-head';
        setLabel(head, 'label.rrCellHead',
                 { n: d.midiNote, mark: velocityLayerToRange(d.velocityLayer).mark });
        item.appendChild(head);
        const fns = document.createElement('ul');
        fns.className = 'rr-confirm-filename-list';
        for (const fn of (d.filenames || [])) {
            const li = document.createElement('li');
            li.textContent = fn;
            fns.appendChild(li);
        }
        item.appendChild(fns);
        listEl.appendChild(item);
    }

    bindModal(dialog, [
        [okBtn, () => true],
        [noBtn, () => false],
    ], { keys: { Escape: noBtn, Enter: okBtn }, focus: okBtn })
        .then((accept) => sendRrConfirmation(accept));
}

async function sendRrConfirmation (accept) {
    await invokeNative('confirmRoundRobinLoad', !!accept);
}

function subscribeAmbiguousDuplicatesEvent () {
    if (!window.__JUCE__ || !window.__JUCE__.backend) return;
    try {
        window.__JUCE__.backend.addEventListener('ambiguousDuplicates', (payload) => {
            let dups = [];
            if (typeof payload === 'string') {
                try { dups = JSON.parse(payload); } catch (_) { dups = []; }
            } else if (Array.isArray(payload)) {
                dups = payload;
            }
            if (Array.isArray(dups) && dups.length > 0) {
                showAmbiguousDuplicatesDialog(dups);
            }
        });
    } catch (e) {
        console.warn('[sampler-app] ambiguousDuplicates subscription failed:', e);
    }
}

async function pullPendingMissingFolder () {
    // Boot-time race: setStateInformation may have run before this listener
    // was registered. Pull the parked path (if any) and surface the modal.
    //
    // v1.12.0: returns an object {path, kind, name}. Old v1.11.x C++ would
    // return a bare string — accept both forms defensively in case a stale
    // host bundle ever round-trips the value.
    if (!window.__JUCE__) return;
    try {
        const fn = Juce.getNativeFunction('getPendingMissingFolder');
        const result = await fn();
        let info = null;
        if (result && typeof result === 'object') {
            const hasContent = (typeof result.path === 'string' && result.path.length > 0)
                            || (typeof result.name === 'string' && result.name.length > 0);
            if (hasContent) info = result;
        } else if (typeof result === 'string' && result.length > 0) {
            info = { path: result, kind: 'filesystem', name: '' };
        }
        if (info) showMissingFolderDialog(info);
    } catch (e) {
        // Silent — older builds may lack the function (defence-in-depth).
    }
}

// ============================================================================
// Boot
// ============================================================================
// ============================================================================
// v1.14.0 — Playing Techniques tab strip + KS picker
// ============================================================================
//
// Single-file feature scope: rendering, native-fn bridging, KS edit, rename
// modal. State is mirrored from the C++ side — every render reads the
// authoritative `getTechniqueState()` snapshot and never trusts a stale local
// copy. The panel hides itself when the user has not opted in (count==1 AND
// ks_enabled==false) so the v1.13.0 visual contract survives untouched.

const techniqueState = {
    names: [],
    active: 0,
    count: 1,
    ksEnabled: false,
    ksLow: 0,
    ksHigh: 9,
    // v1.23.0: { technique: [8 dB], layers: [[4 dB]×8], maxDb: 12 }
    trims: null,
};

async function pullTechniqueState() {
    if (!window.__JUCE__) return;
    try {
        const fn = Juce.getNativeFunction('getTechniqueState');
        const result = await fn();
        const obj = (typeof result === 'string') ? JSON.parse(result) : result;
        if (!obj) return;

        const prevActive = techniqueState.active;

        techniqueState.names     = Array.isArray(obj.names) ? obj.names.slice() : [];
        techniqueState.active    = num(obj.active);
        techniqueState.count     = num(obj.count, 1);
        techniqueState.ksEnabled = !!obj.ksEnabled;
        techniqueState.ksLow     = num(obj.ksLow);
        techniqueState.ksHigh    = num(obj.ksHigh, 9);
        techniqueState.trims     = obj.trims || null;   // v1.23.0

        renderTechniqueBar();
        renderTrimPanel();   // v1.23.0

        // v1.15.0: technique count drives the trigger-panel visibility
        // (single-technique libraries hide it) and slot dimming. Refresh
        // the panel against the cached trigger snapshot whenever the
        // technique state changes — cheap, no native fn call.
        renderTriggerPanel();

        // Active-technique change → re-render the grid against the cached
        // snapshot so cells from the new technique slot appear immediately.
        if (prevActive !== techniqueState.active && lastSampleMapSnapshot) {
            renderGrid(lastSampleMapSnapshot);
        }
    } catch (err) {
        console.warn('[sampler-app] pullTechniqueState failed', err);
    }
}

function subscribeTechniqueStateUpdates() {
    if (!window.__JUCE__ || !window.__JUCE__.backend) return;   // WR-02: match the guard every other subscriber uses
    window.__JUCE__.backend.addEventListener('techniqueStateUpdated', () => {
        pullTechniqueState();
    });
}

function renderTechniqueBar() {
    const bar = document.getElementById('technique-bar');
    if (!bar) return;

    // Visibility: stay collapsed until the user has either grown beyond a
    // single technique slot OR enabled keyswitches. Matches the back-compat
    // contract.
    const expanded = techniqueState.count > 1 || techniqueState.ksEnabled;
    bar.classList.toggle('hidden', !expanded);

    // Per-slot cell counts derived from the cached snapshot. Used to
    // surface "where did my files go?" feedback on the tab strip and to
    // dim empty slots.
    const cellCountsByTech = new Array(8).fill(0);
    if (lastSampleMapSnapshot && Array.isArray(lastSampleMapSnapshot.cells)) {
        for (const c of lastSampleMapSnapshot.cells) {
            const t = num(c.technique);
            if (t >= 0 && t < 8) cellCountsByTech[t]++;
        }
    }

    // Tabs
    const tabs = document.getElementById('technique-tabs');
    if (tabs) {
        tabs.innerHTML = '';
        for (let i = 0; i < techniqueState.count; ++i) {
            const name  = techniqueState.names[i] || trComposed('aria.slotN', { n: i + 1 });
            const count = cellCountsByTech[i];
            const btn = document.createElement('button');
            btn.type = 'button';
            btn.className = 'tech-tab';
            if (i === techniqueState.active) btn.classList.add('active');
            if (count === 0) btn.classList.add('empty');
            btn.dataset.tech = String(i);
            btn.setAttribute('role', 'tab');
            btn.setAttribute('aria-selected', i === techniqueState.active ? 'true' : 'false');
            // v1.24.0: native title= DELETED (§4). check-i18n assertion 11's
            // JS scan reads a QUOTED literal immediately after `.title =`, so
            // this ternary form was invisible to it — reported to the
            // orchestrator, and deleted here regardless because the page
            // rendered it. Its text was also the tab's only help beyond the
            // slot name, so it becomes a composed accessible name.
            //
            // §6: `cell${count === 1 ? '' : 's'}` is gone; the count sits
            // after a colon beside an invariant plural noun phrase.
            const tabAriaText = count > 0
                ? trComposed('aria.techTabLoaded', { i: i + 1, name, n: count })
                : trComposed('aria.techTabEmpty',  { i: i + 1, name });
            btn.setAttribute('aria-label', tabAriaText);
            // The tab's visible face is a technique NAME plus a cell count —
            // runtime data, not copy. D-01 arm 3: a node that carries a number
            // is never a [data-i18n] element, so the parenthesised count stays
            // exactly as it is.
            btn.textContent = count > 0 ? `${name} (${count})` : name;
            tabs.appendChild(btn);
        }
    }

    // Add / remove buttons
    const addBtn = document.getElementById('technique-add');
    const remBtn = document.getElementById('technique-remove');
    if (addBtn) addBtn.disabled = techniqueState.count >= 8;
    if (remBtn) remBtn.disabled = techniqueState.count <= 1;

    // KS controls
    const ksToggle = document.getElementById('technique-ks-enabled');
    const ksLow    = document.getElementById('technique-ks-low');
    const ksHigh   = document.getElementById('technique-ks-high');
    const ksWrap   = document.getElementById('technique-ks-controls');
    if (ksToggle) ksToggle.checked = techniqueState.ksEnabled;
    // v1.23.7 (IN-01): skip the write while the user is typing in the field —
    // an echoed techniqueStateUpdated would otherwise overwrite the edit.
    setInputValueUnlessFocused(ksLow,  techniqueState.ksLow);
    setInputValueUnlessFocused(ksHigh, techniqueState.ksHigh);
    if (ksWrap)   ksWrap.classList.toggle('disabled', !techniqueState.ksEnabled);
}

// ============================================================================
// v1.23.0 — Articulation & layer loudness trims.
//
// One compact panel of 1 master + 4 layer sliders that always targets the
// ACTIVE technique. Switching technique tabs retargets it (renderTrimPanel runs
// on every techniqueStateUpdated). The DOM is STATIC — renderTrimPanel only
// writes `.value`/readouts in place and never rebuilds the inputs, so an echoed
// techniqueStateUpdated mid-drag cannot interrupt a drag. As an extra guard we
// skip writing the value of a slider the user is actively focused on.
// ============================================================================

const TRIM_MAX_DB_FALLBACK = 12;

function formatTrimDb(db) {
    const v = num(db);
    const s = (v > 0 ? '+' : '') + v.toFixed(1);
    return `${s} dB`;
}

// Per-(active technique) layer population: which of layers 0..3 hold cells, so
// empty layer rows can be dimmed. Pure read of the cached snapshot.
function activeTechniqueLayerCounts() {
    const counts = [0, 0, 0, 0];
    const t = num(techniqueState.active);
    if (lastSampleMapSnapshot && Array.isArray(lastSampleMapSnapshot.cells)) {
        for (const c of lastSampleMapSnapshot.cells) {
            if (num(c.technique) !== t) continue;
            const l = num(c.velocityLayer);
            if (l >= 0 && l < 4) counts[l]++;
        }
    }
    return counts;
}

function renderTrimPanel() {
    const panel = document.getElementById('trim-panel');
    if (!panel) return;

    // Visibility: reveal once a library is loaded; otherwise there is nothing
    // to balance. Matches the "trims are library metadata" model.
    const hasCells = !!(lastSampleMapSnapshot
        && Array.isArray(lastSampleMapSnapshot.cells)
        && lastSampleMapSnapshot.cells.length > 0);
    panel.hidden = !hasCells;
    if (!hasCells) return;

    const t = Math.max(0, Math.min(7, num(techniqueState.active)));
    const name = techniqueState.names[t] || trComposed('aria.slotN', { n: t + 1 });

    const chip = document.getElementById('trim-active-tech');
    if (chip) chip.textContent = name;
    const techLabel = document.getElementById('trim-tech-label');
    // v1.24.0: native title= DELETED (§4). The caption's own word is
    // "Technique"; WHICH technique is what the title added, so it becomes a
    // composed accessible name rather than being dropped.
    const trimAria = trComposed('aria.trimWholeTechnique', { name });
    if (techLabel) techLabel.setAttribute('aria-label', trimAria);

    const trims   = techniqueState.trims || {};
    const techArr = Array.isArray(trims.technique) ? trims.technique : [];
    const layRows = Array.isArray(trims.layers) ? trims.layers : [];
    const techDb  = num(techArr[t]);
    const rowForT = Array.isArray(layRows[t]) ? layRows[t] : [];

    // Helper: write a slider's value + readout unless the user is dragging it.
    const apply = (slider, readout, db) => {
        if (slider && document.activeElement !== slider) slider.value = String(db);
        if (readout) readout.textContent = formatTrimDb(db);
    };

    apply(document.getElementById('trim-tech-slider'),
          document.getElementById('trim-tech-readout'), techDb);

    const counts = activeTechniqueLayerCounts();
    for (let l = 0; l < 4; ++l) {
        apply(document.getElementById(`trim-layer-slider-${l}`),
              document.getElementById(`trim-layer-readout-${l}`),
              num(rowForT[l]));
        // Dim layers with no samples in the active technique (still adjustable,
        // just visually de-emphasised so the user knows where the audio is).
        const row = document.querySelector(`.trim-row-layer[data-layer="${l}"]`);
        if (row) row.classList.toggle('empty', counts[l] === 0);
    }
}

// One-time wiring (called from init). Live-updates the readout on input for
// responsiveness, pushes to C++ via the native bridge, and supports
// double-click-to-zero on every slider plus a reset-all button.
function setupTrimPanel() {
    const clampDb = (v) => {
        const max = num(techniqueState.trims && techniqueState.trims.maxDb,
                        TRIM_MAX_DB_FALLBACK) || TRIM_MAX_DB_FALLBACK;
        return Math.max(-max, Math.min(max, num(v)));
    };

    const techSlider  = document.getElementById('trim-tech-slider');
    const techReadout = document.getElementById('trim-tech-readout');
    if (techSlider) {
        techSlider.addEventListener('input', () => {
            const db = clampDb(parseFloat(techSlider.value));
            if (techReadout) techReadout.textContent = formatTrimDb(db);
            invokeNative('setTechniqueTrim',
                         Math.max(0, Math.min(7, num(techniqueState.active))), db);
        });
        techSlider.addEventListener('dblclick', () => {
            techSlider.value = '0';
            if (techReadout) techReadout.textContent = formatTrimDb(0);
            invokeNative('setTechniqueTrim',
                         Math.max(0, Math.min(7, num(techniqueState.active))), 0);
        });
    }

    for (let l = 0; l < 4; ++l) {
        const slider  = document.getElementById(`trim-layer-slider-${l}`);
        const readout = document.getElementById(`trim-layer-readout-${l}`);
        if (!slider) continue;
        slider.addEventListener('input', () => {
            const db = clampDb(parseFloat(slider.value));
            if (readout) readout.textContent = formatTrimDb(db);
            invokeNative('setLayerTrim',
                         Math.max(0, Math.min(7, num(techniqueState.active))), l, db);
        });
        slider.addEventListener('dblclick', () => {
            slider.value = '0';
            if (readout) readout.textContent = formatTrimDb(0);
            invokeNative('setLayerTrim',
                         Math.max(0, Math.min(7, num(techniqueState.active))), l, 0);
        });
    }

    const resetBtn = document.getElementById('trim-reset');
    if (resetBtn) {
        resetBtn.addEventListener('click', () => {
            invokeNative('resetTrims');
            // The echoed techniqueStateUpdated re-pulls + re-renders the sliders.
        });
    }
}

let techniqueRenameTargetIndex = -1;

function openTechniqueRenameDialog(index) {
    if (index < 0 || index >= techniqueState.count) return;
    techniqueRenameTargetIndex = index;
    const dialog = document.getElementById('technique-rename-dialog');
    const idxEl  = document.getElementById('technique-rename-index');
    const input  = document.getElementById('technique-rename-input');
    if (!dialog || !input) return;
    if (idxEl) idxEl.textContent = String(index + 1);
    input.value = techniqueState.names[index] || '';
    dialog.hidden = false;
    setTimeout(() => input.focus(), 0);
}

function closeTechniqueRenameDialog() {
    const dialog = document.getElementById('technique-rename-dialog');
    if (dialog) dialog.hidden = true;
    techniqueRenameTargetIndex = -1;
}

async function commitTechniqueRename() {
    const input = document.getElementById('technique-rename-input');
    if (!input || techniqueRenameTargetIndex < 0) return;
    const trimmed = input.value.trim();
    if (!trimmed) {
        closeTechniqueRenameDialog();
        return;
    }
    await invokeNative('setTechniqueName', techniqueRenameTargetIndex, trimmed);
    closeTechniqueRenameDialog();
}

// ============================================================================
// v1.20.0 — Technique naming presets
// ============================================================================
//
// Each preset maps a Dorico instrument family to the 8 technique slot names in
// keyswitch (baseSwitchID) order, so sampler slot N lines up with keyswitch N
// in the matching O-MicrotonalSampler expression map (see Resources/dorico/…/
// playbacktemplatedeps.doricolib). Applying a preset is non-destructive: it
// only renames slots and grows technique_count to 8 — loaded samples are keyed
// by slot index and stay put. Names are cosmetic; filename auto-routing is
// fixed to the string layout in C++ (FilenameParser) and is unaffected.
//
// Labels reuse the terse vocabulary of the string defaults.
const TECHNIQUE_PRESETS = {
    // ord, sul pont, sul tasto, staccato, con sord (muted), pizz, nat. harmonic, tremolo
    strings: ['ord', 'sp', 'st', 'stacc', 'cs', 'pizz', 'harm', 'trem'],
    // ord, flutter-tongue, whisper, multiphonic, key click, slap-tongue, nat. harmonic, staccato
    winds:   ['ord', 'flz', 'whis', 'mult', 'key', 'slap', 'harm', 'stacc'],
    // ord, muted, cuivré, flutter-tongue, (spare — unbound in Dorico), stopped, growl, fall/drop
    brass:   ['ord', 'mute', 'cuiv', 'flz', 'spare', 'stop', 'growl', 'fall'],
    // ord + 7 open slots — percussion / unknown families; a customization seed
    generic: ['ord', 't2', 't3', 't4', 't5', 't6', 't7', 't8'],
};

// v1.24.0: the four family names are captions, not data — the <option>
// VALUES ('strings' … 'generic') are what the code keys on, and they are
// untouched. The visible faces are keyed in index.html; this map now names
// their keys so the toast reads the same four words the selector shows.
const TECHNIQUE_PRESET_LABEL_KEYS = {
    strings: 'label.familyStrings', winds: 'label.familyWinds',
    brass:   'label.familyBrass',   generic: 'label.familyGeneric',
};

async function applyTechniquePreset(key) {
    const names = TECHNIQUE_PRESETS[key];
    if (!names) return;
    // Variadic native fn: each name is a positional arg. C++ sets
    // technique_count = arg count and writes each slot, firing a single
    // techniqueStateUpdated that re-pulls + re-renders the strip (and reveals
    // the technique bar, which stays hidden at count==1).
    await invokeNative('applyTechniqueNames', ...names);
    showToastKey('toast.techniquePresetApplied',
                 { family: TECHNIQUE_PRESET_LABEL_KEYS[key] || key });
}

function bindTechniquePresets() {
    const select = document.getElementById('technique-preset-select');
    if (!select) return;
    select.addEventListener('change', async () => {
        const key = select.value;
        // Snap back to the placeholder so the same family can be re-applied
        // and the control never implies a sticky "current preset" (slots are
        // freely renameable afterwards, which would make a held selection lie).
        select.value = '';
        if (key) await applyTechniquePreset(key);
    });
}

function bindTechniqueBar() {
    const tabs = document.getElementById('technique-tabs');
    if (tabs) {
        tabs.addEventListener('click', async (e) => {
            const btn = e.target.closest('.tech-tab');
            if (!btn) return;
            const idx = parseInt(btn.dataset.tech, 10);
            if (!Number.isFinite(idx)) return;
            if (window.__JUCE__) {
                const fn = Juce.getNativeFunction('setActiveTechnique');
                await fn(idx);
            } else {
                techniqueState.active = idx;
                renderTechniqueBar();
            }
        });
        tabs.addEventListener('contextmenu', (e) => {
            const btn = e.target.closest('.tech-tab');
            if (!btn) return;
            e.preventDefault();
            const idx = parseInt(btn.dataset.tech, 10);
            if (Number.isFinite(idx)) openTechniqueRenameDialog(idx);
        });
    }

    const addBtn = document.getElementById('technique-add');
    if (addBtn) {
        addBtn.addEventListener('click', async () => {
            await invokeNative('addTechniqueSlot');
        });
    }

    const remBtn = document.getElementById('technique-remove');
    if (remBtn) {
        remBtn.addEventListener('click', async () => {
            if (techniqueState.count > 1)
                await invokeNative('removeTechniqueSlot', techniqueState.count - 1);
        });
    }

    const ksToggle = document.getElementById('technique-ks-enabled');
    if (ksToggle) {
        ksToggle.addEventListener('change', async () => {
            await invokeNative('setKeyswitchEnabled', !!ksToggle.checked);
        });
    }

    const commitKsRange = async () => {
        const ksLow  = document.getElementById('technique-ks-low');
        const ksHigh = document.getElementById('technique-ks-high');
        if (!ksLow || !ksHigh) return;
        let lo = parseInt(ksLow.value,  10);
        let hi = parseInt(ksHigh.value, 10);
        if (!Number.isFinite(lo)) lo = 0;
        if (!Number.isFinite(hi)) hi = 9;
        lo = Math.max(0, Math.min(127, lo));
        hi = Math.max(0, Math.min(127, hi));
        if (hi < lo) hi = lo;
        await invokeNative('setKeyswitchRange', lo, hi);
    };

    const ksLow  = document.getElementById('technique-ks-low');
    const ksHigh = document.getElementById('technique-ks-high');
    if (ksLow)  ksLow.addEventListener('change',  commitKsRange);
    if (ksHigh) ksHigh.addEventListener('change', commitKsRange);

    // Rename modal — Save / Cancel + Enter / Escape
    const saveBtn   = document.getElementById('technique-rename-save');
    const cancelBtn = document.getElementById('technique-rename-cancel');
    const input     = document.getElementById('technique-rename-input');
    if (saveBtn)   saveBtn.addEventListener('click',   commitTechniqueRename);
    if (cancelBtn) cancelBtn.addEventListener('click', closeTechniqueRenameDialog);
    if (input) {
        input.addEventListener('keydown', (e) => {
            if (e.key === 'Enter')      { e.preventDefault(); commitTechniqueRename(); }
            else if (e.key === 'Escape'){ e.preventDefault(); closeTechniqueRenameDialog(); }
        });
    }
}

// ============================================================================
// v1.15.0 — CC + PC trigger panel
// ============================================================================
//
// The C++ side owns two 8-slot tables (CC value-range → tech, PC# → tech)
// plus three APVTS gates (cc_select_enabled, cc_number, pc_enabled). The
// audio thread reads these to drive technique routing in processBlock with
// KS > CC > PC > history precedence. The JS layer holds a cached snapshot
// for rendering — single source of truth always lives in C++.

const triggerState = {
    ccEnabled: false,
    ccNumber:  32,
    ccMapping: [],   // 8 entries: { rangeLow, rangeHigh, tech }
    pcEnabled: false,
    pcMapping: [],   // 8 entries: { pc, tech }
};

async function pullTriggerState() {
    if (!window.__JUCE__) return;
    try {
        const fn = Juce.getNativeFunction('getTriggerState');
        const raw = await fn();
        // The native fn returns a juce::DynamicObject which arrives as a
        // plain JS object (matching the getTechniqueState pattern).
        const obj = (typeof raw === 'string') ? JSON.parse(raw) : raw;

        triggerState.ccEnabled = !!obj.ccEnabled;
        triggerState.ccNumber  = num(obj.ccNumber, 32);
        triggerState.pcEnabled = !!obj.pcEnabled;

        triggerState.ccMapping = Array.isArray(obj.ccMapping)
            ? obj.ccMapping.slice(0, 8).map(s => ({
                rangeLow:  num(s.rangeLow),
                rangeHigh: num(s.rangeHigh, 127),
                tech:      num(s.tech),
            }))
            : [];
        triggerState.pcMapping = Array.isArray(obj.pcMapping)
            ? obj.pcMapping.slice(0, 8).map(s => ({
                pc:   num(s.pc),
                tech: num(s.tech),
            }))
            : [];

        renderTriggerPanel();
    } catch (err) {
        console.warn('[sampler-app] pullTriggerState failed', err);
    }
}

function subscribeTriggerStateUpdates() {
    if (!window.__JUCE__ || !window.__JUCE__.backend) return;   // WR-02: match the guard every other subscriber uses
    window.__JUCE__.backend.addEventListener('triggerStateUpdated', () => {
        pullTriggerState();
    });
}

// v1.23.7 (WR-03): the CC/PC table rows are built ONCE and then only updated
// in place. renderTriggerPanel used to `innerHTML = ''`-rebuild all 8 rows on
// every technique/trigger state echo, discarding any in-progress edit and
// dropping focus — the exact bug class the trim panel solved in v1.23.0 with
// a static DOM + activeElement guard. The update pass below skips whichever
// input the user is currently focused in.
function ensureTriggerRows(tbody, fields) {
    if (tbody.childElementCount === 8) return;
    tbody.innerHTML = '';
    for (let i = 0; i < 8; ++i) {
        const tr = document.createElement('tr');
        tr.dataset.slot = String(i);

        const idTd = document.createElement('td');
        idTd.textContent = String(i + 1);
        tr.appendChild(idTd);

        for (const f of fields) {
            const td = document.createElement('td');
            const input = document.createElement('input');
            input.type = 'number';
            input.min = String(f.min);
            input.max = String(f.max);
            input.step = '1';
            input.dataset.field = f.field;
            td.appendChild(input);
            tr.appendChild(td);
        }
        tbody.appendChild(tr);
    }
}

// Write an input's value in place unless the user is editing it right now.
function setInputValueUnlessFocused(input, value) {
    if (input && document.activeElement !== input) input.value = String(value);
}

function renderTriggerPanel() {
    const panel = document.getElementById('trigger-panel');
    if (!panel) return;

    // Mirror the technique-bar visibility contract: stay hidden until the
    // user has more than one technique slot, since CC/PC routing is
    // pointless on a single-technique library.
    const expanded = techniqueState.count > 1;
    panel.hidden = !expanded;
    if (!expanded) return;

    // Top-level fields
    const ccToggle = document.getElementById('cc-trigger-enabled');
    const ccNumber = document.getElementById('cc-trigger-number');
    const pcToggle = document.getElementById('pc-trigger-enabled');
    if (ccToggle) ccToggle.checked = triggerState.ccEnabled;
    if (ccNumber) setInputValueUnlessFocused(ccNumber, triggerState.ccNumber);
    if (pcToggle) pcToggle.checked = triggerState.pcEnabled;

    const techCount = Math.max(1, Math.min(8, techniqueState.count || 1));

    // CC table — 8 rows. Slots beyond techCount are dimmed (kept editable
    // but greyed so the user can still pre-stage values for a future grow).
    const ccTbody = document.querySelector('#cc-trigger-table tbody');
    if (ccTbody) {
        ensureTriggerRows(ccTbody, [
            { field: 'lo',   min: 0, max: 127 },
            { field: 'hi',   min: 0, max: 127 },
            { field: 'tech', min: 1, max: 8   },
        ]);
        for (let i = 0; i < 8; ++i) {
            const tr = ccTbody.children[i];
            const slot = triggerState.ccMapping[i] || { rangeLow: 0, rangeHigh: 127, tech: 0 };
            tr.style.opacity = (i >= techCount) ? '0.45' : '';
            setInputValueUnlessFocused(tr.querySelector('input[data-field="lo"]'),   slot.rangeLow);
            setInputValueUnlessFocused(tr.querySelector('input[data-field="hi"]'),   slot.rangeHigh);
            setInputValueUnlessFocused(tr.querySelector('input[data-field="tech"]'), slot.tech + 1);  // user-facing 1..8
        }
    }

    // PC table
    const pcTbody = document.querySelector('#pc-trigger-table tbody');
    if (pcTbody) {
        ensureTriggerRows(pcTbody, [
            { field: 'pc',   min: 0, max: 127 },
            { field: 'tech', min: 1, max: 8   },
        ]);
        for (let i = 0; i < 8; ++i) {
            const tr = pcTbody.children[i];
            const slot = triggerState.pcMapping[i] || { pc: i, tech: i };
            tr.style.opacity = (i >= techCount) ? '0.45' : '';
            setInputValueUnlessFocused(tr.querySelector('input[data-field="pc"]'),   slot.pc);
            setInputValueUnlessFocused(tr.querySelector('input[data-field="tech"]'), slot.tech + 1);
        }
    }

    // Sub-panel disabled visuals (mirror the KS-controls disabled pattern)
    const ccSubpanel = document.querySelector('#cc-trigger-table')?.closest('.trigger-subpanel');
    const pcSubpanel = document.querySelector('#pc-trigger-table')?.closest('.trigger-subpanel');
    if (ccSubpanel) ccSubpanel.classList.toggle('disabled', !triggerState.ccEnabled);
    if (pcSubpanel) pcSubpanel.classList.toggle('disabled', !triggerState.pcEnabled);
}

function bindTriggerPanel() {
    const ccToggle = document.getElementById('cc-trigger-enabled');
    if (ccToggle) {
        ccToggle.addEventListener('change', async () => {
            await invokeNative('setCcEnabled', !!ccToggle.checked);
        });
    }

    const ccNumber = document.getElementById('cc-trigger-number');
    if (ccNumber) {
        ccNumber.addEventListener('change', async () => {
            let v = parseInt(ccNumber.value, 10);
            if (!Number.isFinite(v)) v = 32;
            v = Math.max(0, Math.min(119, v));
            await invokeNative('setCcNumber', v);
        });
    }

    const pcToggle = document.getElementById('pc-trigger-enabled');
    if (pcToggle) {
        pcToggle.addEventListener('change', async () => {
            await invokeNative('setPcEnabled', !!pcToggle.checked);
        });
    }

    const ccTable = document.getElementById('cc-trigger-table');
    if (ccTable) {
        ccTable.addEventListener('change', async (e) => {
            const input = e.target;
            if (!input || input.tagName !== 'INPUT') return;
            const tr = input.closest('tr');
            if (!tr) return;
            const slot = parseInt(tr.dataset.slot, 10);
            if (!Number.isFinite(slot)) return;

            const loInput   = tr.querySelector('input[data-field="lo"]');
            const hiInput   = tr.querySelector('input[data-field="hi"]');
            const techInput = tr.querySelector('input[data-field="tech"]');
            if (!loInput || !hiInput || !techInput) return;

            let lo   = Math.max(0, Math.min(127, parseInt(loInput.value,   10) || 0));
            let hi   = Math.max(0, Math.min(127, parseInt(hiInput.value,   10) || 0));
            let tech = Math.max(1, Math.min(8,   parseInt(techInput.value, 10) || 1)) - 1;
            if (hi < lo) hi = lo;

            await invokeNative('setCcMapping', slot, lo, hi, tech);
        });
    }

    const pcTable = document.getElementById('pc-trigger-table');
    if (pcTable) {
        pcTable.addEventListener('change', async (e) => {
            const input = e.target;
            if (!input || input.tagName !== 'INPUT') return;
            const tr = input.closest('tr');
            if (!tr) return;
            const slot = parseInt(tr.dataset.slot, 10);
            if (!Number.isFinite(slot)) return;

            const pcInput   = tr.querySelector('input[data-field="pc"]');
            const techInput = tr.querySelector('input[data-field="tech"]');
            if (!pcInput || !techInput) return;

            const pc   = Math.max(0, Math.min(127, parseInt(pcInput.value,   10) || 0));
            const tech = Math.max(1, Math.min(8,   parseInt(techInput.value, 10) || 1)) - 1;

            await invokeNative('setPcMapping', slot, pc, tech);
        });
    }

    const resetBtn = document.getElementById('trigger-reset');
    if (resetBtn) {
        resetBtn.addEventListener('click', async () => {
            await invokeNative('resetTriggerMappings');
        });
    }
}

// ════════════════════════════════════════════════════════════════════════════
// v1.24.0 — THE SETTINGS POPOVER
//
// ONE row: this plugin has no hover-help to switch on or off, so the popover
// holds the language selector alone.
// ════════════════════════════════════════════════════════════════════════════

function initSettingsPopover() {
    const gearBtn = document.getElementById('gear-btn');
    const popover = document.getElementById('settings-popover');
    if (!gearBtn || !popover) {
        console.warn('[sampler-app] settings popover missing — language selector unavailable');
        return;
    }

    const setOpen = (open) => {
        popover.hidden = !open;
        gearBtn.setAttribute('aria-expanded', open ? 'true' : 'false');
    };

    gearBtn.addEventListener('click', (e) => {
        e.stopPropagation();
        setOpen(popover.hidden);
    });

    // mousedown, not click, so the panel is gone before a knob drag underneath
    // it begins — the SVG knobs start their drag on pointerdown.
    document.addEventListener('mousedown', (e) => {
        if (popover.hidden) return;
        if (popover.contains(e.target) || gearBtn.contains(e.target)) return;
        setOpen(false);
    });

    document.addEventListener('keydown', (e) => {
        if (e.key === 'Escape' && !popover.hidden) { setOpen(false); gearBtn.focus(); }
    });

    // The canon's own change listener (registered inside initI18n) runs first
    // and repaints every [data-i18n] element; this one repaints the strings the
    // renderers compose. Registration order is the firing order, so the
    // composed pass always sees the language the sweep has just set.
    const sel = document.getElementById('lang-select');
    if (sel) sel.addEventListener('change', () => refreshComposedUi());
}

document.addEventListener('DOMContentLoaded', () => {
    bindTabs();
    bindSliders();
    pullParameterDefaults();               // v1.23.7 (WR-04) — dblclick reset targets

    subscribeSampleMapUpdates();
    subscribeFolderMissingEvent();   // v1.3.0 — register before pull
    subscribeAmbiguousDuplicatesEvent();   // v1.8.0
    subscribeTechniqueStateUpdates();      // v1.14.0
    subscribeTriggerStateUpdates();        // v1.15.0
    bindHostDragEvents();
    bindToastEventListener();
    bindFolderDropZone();
    // v1.13.0 (ARCH-02): document-level drop listener now lives in the
    // shared webview-drop-streaming module. We supply the plugin-specific
    // glue (selectors + UI callbacks + cell midi/vel extraction).
    bindWebViewFileDrop({
        juce: Juce,                              // Juce ES-module namespace, NOT window.__JUCE__
        dropZoneSelector: '#folder-drop-zone',
        cellSelector: '[data-note]',
        setDropZoneHover: setFolderDropZoneHover,
        showFolderLoadOptionsModal,              // (totalBytes) -> {layer,mode,override,embedAudio} | null
        cellMidiVelExtractor: (cellEl) => ({
            midi: parseInt(cellEl.dataset.note,  10),
            vel:  parseInt(cellEl.dataset.layer, 10),
        }),
        showToast,
        showDiagnosticDialog,
        // Fast-path for hosts that DO expose absolute paths (Linux/Win/some
        // WebKit). Forward to the existing handleWebViewFileDrop native fn
        // so cell hit-test + folder-zone hit-test routing run unchanged.
        onPathFastPath: async (paths, x, y) => {
            const fn = Juce.getNativeFunction('handleWebViewFileDrop');
            await fn(paths, x, y);
        },
    });
    bindClearSamplesButton();
    bindBatchLoopButton();           // v1.18.0
    bindVelLabelInteractions();      // v1.18.0
    bindPresetButtons();             // v1.3.0
    pullInitialSampleMap();
    pullPendingMissingFolder();      // v1.3.0 — covers boot-time race
    refreshTuningReadout();
    refreshAboutVersion();
    bindResizeObserver();
    bindLoopEditorEvents();
    bindLoopEditorResize();
    bindTechniqueBar();              // v1.14.0
    bindTechniquePresets();          // v1.20.0
    setupTrimPanel();                // v1.23.0
    pullTechniqueState();            // v1.14.0
    bindTriggerPanel();              // v1.15.0
    pullTriggerState();              // v1.15.0

    // ── i18n LAST, and guarded ──────────────────────────────────────────
    // Every binding above has already run, so an i18n failure leaves an
    // English plugin that still works rather than a dead page. Running it here
    // is also what lets the first sweep see the control strip, which does not
    // exist in index.html until renderControlStrip() has run.
    initSettingsPopover();           // v1.24.0
    // setupTooltips() sits INSIDE the same try/catch and strictly AFTER
    // initI18n(), deliberately on both counts: no anchor carries data-tip until
    // initI18n() has painted the attributes, and a throw out of either must not
    // reach module scope (pattern_module_toplevel_init_tdz).
    try { initI18n(); setupTooltips(); }
    catch (e) { console.error('i18n init failed:', e); }
});
