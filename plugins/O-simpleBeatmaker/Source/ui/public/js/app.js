/*
   This file is part of O-simpleBeatmaker, an Ouaricon Audio plugin.
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
// ============================================================================
// O-simpleBeatmaker — WebView UI controller
// Binds all 42 APVTS params two-way (29 knobs + 1 selector + 12 toggles), builds
// the 6×N step grid with the velocity cycle (off → normal → accent → ghost), runs
// the live playhead, draws the applied-Δt timing lane (QUAL-02: the Δt baked into
// the audio, drained from VizAnalyzer — never a UI recompute), prints the live
// MIDI readout, and runs the pedagogical tooltip layer.
//
// Two namespaces (project memory): getSliderState / getComboBoxState /
// getToggleState / getNativeFunction live on the `Juce` ES-module namespace.
// window.__JUCE__.backend.addEventListener is the low-level channel for the one
// C++-pushed "frame" event (playhead + drained hits + transport).
//
// INTERFACE COPY (v1.1.0): every label, heading, button face, hint, legend key,
// MIDI row and tooltip on this page comes from js/i18n.js in English or French.
// applyI18n() writes each tooltip onto its anchor as data-tip-title + data-tip
// and each caption into its [data-i18n] element; the renderer below only
// positions what the anchor already carries. The value readouts, the six lesson
// preset names and the MIDI note numbers stay English (D-01/D-02/D-03).
// ============================================================================

import * as Juce from "./juce/index.js";

// ════════════════════════════════════════════════════════════════════════════
// INTERFACE LANGUAGE (v1.1.0) — canon v2, verbatim from scripts/i18n-canon.js.
//
// It sits HERE, directly under the imports and above every other declaration in
// this module, because `uiLanguage` is a module-level `let`: anything above it
// that reached applyLabel() would touch it inside its temporal dead zone and
// throw out of module evaluation, taking the 42 parameter bindings, the step
// grid, the timing lane and the MIDI readout with it
// (pattern_module_toplevel_init_tdz). This file DOES have eager top-level work
// below — VOICES, SLIDER_IDS, the FORMAT table — so the ordering is load-bearing
// here, not merely defensive. `node scripts/boot-all-uis.js` is the ONLY gate in
// the repo that sees this class of failure.
//
// Do NOT edit the region between the import line and the close of initI18n():
// check-i18n assertion 6 byte-compares it against scripts/i18n-canon.js.
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
            .then((code) => applyI18n(code === 'fr' ? 'fr' : 'en'))
            .catch((e) => console.warn('Could not read language preference:', e));
    }

    const sel = document.getElementById('lang-select');
    if (sel) sel.addEventListener('change', (e) => {
        applyI18n(e.target.value);
        if (setUiLanguageNative) setUiLanguageNative(uiLanguage).catch(() => {});
    });
}

// ── Voice roster — row order MUST match BeatmakerIDs kVoicePrefix / kGmNotes ──
// `name` is the v1.0.3 English face, kept as the no-i18n fallback and as the
// {voice} token's last resort; `key` is what the six one-line writers below
// actually use. VOICE_NAME_WRITERS exists because check-i18n assertion 13
// requires every setLabel key to be a plain string literal and assertion 15
// counts only literal keys as references — VOICES[v].key is neither, so a
// key-driven call would fail one gate and report all six keys DEAD to the other.
const VOICES = [
  { prefix: "kick",      name: "Kick",       key: "label.voiceKick",      note: 36, hue: "#c0532b" },
  { prefix: "snare",     name: "Snare",      key: "label.voiceSnare",     note: 38, hue: "#b5862e" },
  { prefix: "clap",      name: "Clap",       key: "label.voiceClap",      note: 39, hue: "#9a6b3f" },
  { prefix: "closedHat", name: "Closed Hat", key: "label.voiceClosedHat", note: 42, hue: "#5f9e57" },
  { prefix: "openHat",   name: "Open Hat",   key: "label.voiceOpenHat",   note: 46, hue: "#3f8e93" },
  { prefix: "tom",       name: "Tom",        key: "label.voiceTom",       note: 45, hue: "#7a5ba6" },
];
const VOICE_NAME_WRITERS = {
  kick:      (el) => setLabel(el, "label.voiceKick"),
  snare:     (el) => setLabel(el, "label.voiceSnare"),
  clap:      (el) => setLabel(el, "label.voiceClap"),
  closedHat: (el) => setLabel(el, "label.voiceClosedHat"),
  openHat:   (el) => setLabel(el, "label.voiceOpenHat"),
  tom:       (el) => setLabel(el, "label.voiceTom"),
};
// The four repeating knob captions and the two toggle faces, keyed ONCE each and
// applied to all six strips — not six copies of one key.
const VOICE_KNOB_WRITERS = {
  Tune:  (el) => setLabel(el, "label.knobTune"),
  Decay: (el) => setLabel(el, "label.knobDecay"),
  Tone:  (el) => setLabel(el, "label.knobTone"),
  Level: (el) => setLabel(el, "label.knobLevel"),
};
const VOICE_TOGGLE_WRITERS = {
  Mute: (el) => setLabel(el, "label.mute"),
  Solo: (el) => setLabel(el, "label.solo"),
};
// The accessible-name key each generated knob carries. A TABLE, not
// `"voice" + suffix`: check-i18n assertion 15 reads a dataset.i18nAria
// assignment with a plain string literal on the right as a key REFERENCE, and
// the concatenated form handed it the prefix `voice` as if that were the key —
// a dangling reference to a key that does not exist and never could.
const VOICE_KNOB_ARIA = {
  Tune:  "voiceTune",
  Decay: "voiceDecay",
  Tone:  "voiceTone",
  Level: "voiceLevel",
};
const NUM_VOICES = VOICES.length;   // 6
const MAX_STEPS = 32;

// Velocity quick-states (the "click-again-to-accent, generalised to velocity").
const VEL_GHOST = 40, VEL_NORMAL = 100, VEL_ACCENT = 127;
function nextVelocity(v) {
  if (v === 0)   return VEL_NORMAL;   // off    → normal
  if (v <= 55)   return 0;            // ghost  → off
  if (v <= 112)  return VEL_ACCENT;   // normal → accent
  return VEL_GHOST;                   // accent → ghost
}
function velTier(v) { return v === 0 ? "off" : v <= 55 ? "ghost" : v <= 112 ? "normal" : "accent"; }
// Velocity glyphs, and the per-tier accessible-name key. Both are LOOKUP TABLES
// rather than the v1.0.3 chained ternary in the textContent assignment itself:
// the extractor collects every string literal on the right-hand side of a
// textContent write, including a comparison operand, so `tier === "accent"`
// there reported "accent" and "ghost" as raw English prose writes and failed
// check-i18n assertion 12 over two words the page never renders.
const VEL_MARK = { off: "", ghost: "·", normal: "", accent: "▲" };
const VEL_ARIA_KEY = {
  off:    "aria.cellOff",
  ghost:  "aria.cellGhost",
  normal: "aria.cellNormal",
  accent: "aria.cellAccent",
};

// ── Parameter inventory (must match OSimpleBeatmaker::ParamIDs exactly) ──────
const GLOBAL_SLIDERS = ["swing", "humanize", "quantizeStrength", "tempo", "outputLevel"];
const VOICE_SLIDER_SUFFIX = ["Tune", "Decay", "Tone", "Level"];
const VOICE_TOGGLE_SUFFIX = ["Mute", "Solo"];
const SLIDER_IDS = [
  ...GLOBAL_SLIDERS,
  ...VOICES.flatMap((v) => VOICE_SLIDER_SUFFIX.map((s) => v.prefix + s)),
];
const COMBO_IDS = ["patternLength"];
const TOGGLE_IDS = VOICES.flatMap((v) => VOICE_TOGGLE_SUFFIX.map((s) => v.prefix + s));

// ── Display formatters (receive the *scaled* param value) ────────────────────
const fmtPct75 = (v) => `${Math.round(v * 75)}%`;        // swing 0–1 → 0–75%
const fmtPct = (v) => `${Math.round(v * 100)}%`;          // 0–1 → 0–100%
const fmtBpm = (v) => `${Math.round(v)} BPM`;
const fmtDb = (v) => (v <= -59.95 ? "−∞ dB" : `${v.toFixed(1)} dB`);
const fmtSt = (v) => `${v >= 0 ? "+" : "−"}${Math.abs(v).toFixed(1)} st`;
const FORMAT = {
  swing: fmtPct75, humanize: fmtPct, quantizeStrength: fmtPct, tempo: fmtBpm, outputLevel: fmtDb,
};
function voiceFmt(suffix) {
  if (suffix === "Tune") return fmtSt;
  if (suffix === "Level") return fmtDb;
  return fmtPct;   // Decay, Tone
}

// ── Tooltip copy ────────────────────────────────────────────────────────────
// It lives in js/i18n.js now, in BOTH languages, and applyI18n writes it onto
// each anchor as data-tip-title + data-tip. Through v1.0.3 a TIPS object here
// held [title, bodyHtml] pairs keyed by the anchor's OWN data-tip attribute —
// which canon v2 overwrites with the tip BODY, so the key and the copy would
// have fought over one attribute, and check-i18n assertion 3 requires index.html
// to carry zero data-tip literals. The six parameter cells in the markup and the
// 24 generated ones carry a data-param attribute from v1.1.0; the three panels
// and the lesson row carry an id; the twelve mute/solo buttons already had one;
// the six lesson chips are addressed by the data-preset they already carried.

// ── Knob geometry (relative vertical drag, matches the simple family) ────────
const KNOB_MIN_DEG = -135, KNOB_MAX_DEG = 135, DRAG_TRAVEL_PX = 220;
const sliderState = {};   // id -> Juce SliderState
const comboState = {};    // id -> Juce ComboBoxState
const toggleState = {};   // id -> Juce ToggleState
let paramDefaults = null; // id -> normalised default (getParameterDefaults native fn)
function normToDeg(n) { return KNOB_MIN_DEG + n * (KNOB_MAX_DEG - KNOB_MIN_DEG); }

function fmtFor(id) {
  if (FORMAT[id]) return FORMAT[id];
  for (const v of VOICES)
    for (const s of VOICE_SLIDER_SUFFIX)
      if (id === v.prefix + s) return voiceFmt(s);
  return (x) => x.toFixed(2);
}

function updateKnobVisual(id) {
  const st = sliderState[id];
  if (!st) return;
  const norm = st.getNormalisedValue();
  const knob = document.getElementById(`knob-${id}`);
  const valText = fmtFor(id)(st.getScaledValue());
  if (knob) {
    const stem = knob.querySelector(".knob-stem");
    if (stem) stem.style.transform = `translate(-50%, -100%) rotate(${normToDeg(norm)}deg)`;
    knob.setAttribute("aria-valuenow", norm.toFixed(3));
    knob.setAttribute("aria-valuetext", valText);
  }
  const valEl = document.getElementById(`val-${id}`);
  if (valEl) valEl.textContent = valText;
}

function nudge(st, delta, id) {
  const n = Math.max(0, Math.min(1, st.getNormalisedValue() + delta));
  st.sliderDragStarted(); st.setNormalisedValue(n); st.sliderDragEnded();
  updateKnobVisual(id);
}

function bindKnob(id) {
  const st = Juce.getSliderState(id);
  sliderState[id] = st;
  st.valueChangedEvent.addListener(() => updateKnobVisual(id));
  st.propertiesChangedEvent.addListener(() => updateKnobVisual(id));
  updateKnobVisual(id);

  const knob = document.getElementById(`knob-${id}`);
  if (!knob) { console.error(`Missing knob element: knob-${id}`); return; }
  knob.setAttribute("tabindex", "0");
  knob.setAttribute("role", "slider");
  knob.setAttribute("aria-valuemin", "0");
  knob.setAttribute("aria-valuemax", "1");
  // v1.0.3 named the knob by copying its caption's textContent HERE, once, at
  // bind time — which runs before initI18n(), so the accessible name was the
  // English fallback forever and never followed a language switch. The name now
  // comes from data-i18n-aria (in the markup for the five global knobs, set by
  // buildVoiceStrips for the 24 generated ones) and is rewritten by every sweep.
  knob.addEventListener("keydown", (e) => {
    let d = 0;
    if (e.key === "ArrowUp" || e.key === "ArrowRight") d = 0.02;
    else if (e.key === "ArrowDown" || e.key === "ArrowLeft") d = -0.02;
    else return;
    nudge(st, d, id); e.preventDefault();
  });

  let dragging = false, startY = 0, startNorm = 0;
  const onMove = (e) => {
    if (!dragging) return;
    let n = startNorm + (startY - e.clientY) / DRAG_TRAVEL_PX;
    n = Math.max(0, Math.min(1, n));
    st.setNormalisedValue(n); updateKnobVisual(id); e.preventDefault();
  };
  // pointercancel (pen/touch, OS gesture interruption) must also end the drag,
  // or the host automation gesture (sliderDragStarted) is left open.
  const onUp = () => {
    if (!dragging) return;
    dragging = false; st.sliderDragEnded();
    window.removeEventListener("pointermove", onMove);
    window.removeEventListener("pointerup", onUp);
    window.removeEventListener("pointercancel", onUp);
  };
  knob.addEventListener("pointerdown", (e) => {
    dragging = true; startY = e.clientY; startNorm = st.getNormalisedValue();
    st.sliderDragStarted();
    try { knob.setPointerCapture(e.pointerId); } catch (_) { /* mouse w/o capture support */ }
    window.addEventListener("pointermove", onMove);
    window.addEventListener("pointerup", onUp);
    window.addEventListener("pointercancel", onUp);
    e.preventDefault();
  });
  knob.addEventListener("dblclick", () => {
    const d = paramDefaults ? paramDefaults[id] : undefined;
    if (typeof d !== "number") return;
    st.sliderDragStarted(); st.setNormalisedValue(d); st.sliderDragEnded();
    updateKnobVisual(id);
  });
  knob.addEventListener("wheel", (e) => { nudge(st, e.deltaY < 0 ? 0.02 : -0.02, id); e.preventDefault(); }, { passive: false });
}

// ── Pattern-length selector (also re-renders grid columns) ───────────────────
let patternLen = 16;
function bindCombo(id) {
  const st = Juce.getComboBoxState(id);
  comboState[id] = st;
  const sel = document.getElementById(`combo-${id}`);
  if (!sel) { console.error(`Missing combo element: combo-${id}`); return; }
  const buildOptions = () => {
    const choices = (st.properties && st.properties.choices) || [];
    if (choices.length === 0) return false;
    if (sel.options.length === choices.length) return true;
    sel.textContent = "";
    choices.forEach((c, i) => { const o = document.createElement("option"); o.value = String(i); o.textContent = c; sel.appendChild(o); });
    return true;
  };
  const refresh = () => {
    buildOptions();
    const idx = st.getChoiceIndex();
    if (idx >= 0 && idx < sel.options.length) sel.selectedIndex = idx;
    const lens = [8, 16, 32];
    patternLen = lens[idx] || 16;
    const lenEl = document.getElementById("readLength");
    if (lenEl) lenEl.textContent = String(patternLen);
    renderGridColumns();
  };
  st.propertiesChangedEvent.addListener(refresh);
  st.valueChangedEvent.addListener(refresh);
  refresh();
  sel.addEventListener("change", () => st.setChoiceIndex(sel.selectedIndex));
}

// ── Toggle (mute / solo) binding ─────────────────────────────────────────────
function bindToggle(id) {
  const st = Juce.getToggleState(id);
  toggleState[id] = st;
  const el = document.getElementById(`toggle-${id}`);
  if (!el) { console.error(`Missing toggle element: toggle-${id}`); return; }
  const refresh = () => {
    const on = st.getValue();
    el.classList.toggle("active", on);
    el.setAttribute("aria-pressed", on ? "true" : "false");
  };
  st.valueChangedEvent.addListener(refresh);
  refresh();
  el.addEventListener("click", () => { st.setValue(!st.getValue()); refresh(); });
}

// ── Step grid ────────────────────────────────────────────────────────────────
// Full 6×32 mirror; only the active patternLen columns are rendered.
const gridState = Array.from({ length: NUM_VOICES }, () => new Array(MAX_STEPS).fill(0));
let setStepFn = null;
const cellEls = Array.from({ length: NUM_VOICES }, () => new Array(MAX_STEPS).fill(null));

function paintCell(v, s) {
  const el = cellEls[v][s];
  if (!el) return;
  const vel = gridState[v][s];
  const tier = velTier(vel);
  el.classList.toggle("on", vel > 0);
  el.classList.remove("vel-ghost", "vel-normal", "vel-accent");
  if (vel > 0) el.classList.add(`vel-${tier}`);
  const fill = el.querySelector(".cell-fill");
  if (fill) fill.style.height = vel > 0 ? `${Math.max(20, Math.round((vel / 127) * 100))}%` : "0%";
  const mark = el.querySelector(".cell-mark");
  if (mark) mark.textContent = VEL_MARK[tier] || "";
  // a11y: announce the cell's voice, step and current velocity state, in the
  // current language. NOT a [data-i18n] element and not a setLabel call: there
  // are 96 to 192 of these and every one has a different name, so it is composed
  // per cell through trLabel() with vars. {voice} is itself a LABELS key and
  // resolves to the localized voice name; {step} and {vel} are numbers and
  // substitute verbatim (D-03). No prose literal appears in this call — the four
  // sentence shapes are in the table, which is what keeps assertion 12 green.
  el.setAttribute("aria-pressed", vel > 0 ? "true" : "false");
  el.setAttribute("aria-label", trLabel(VEL_ARIA_KEY[tier], uiLanguage,
                                        { voice: VOICES[v].key, step: s + 1, vel }));
}

// The cells are repainted on edit, not on a language change, so the sweep never
// reaches their accessible names. One equality check per animation frame does:
// it covers the selector, the asynchronous boot-time pull from C++ and any
// future caller, because it watches the RESULT rather than a particular path.
let lastAriaLang = null;
function refreshGridAria() {
  if (lastAriaLang === uiLanguage) return;
  lastAriaLang = uiLanguage;
  for (let v = 0; v < NUM_VOICES; v++)
    for (let s = 0; s < patternLen; s++) paintCell(v, s);
}

function applyStep(v, s, vel) {
  gridState[v][s] = vel;
  lastLocalEditTime = performance.now();   // guard the 4 Hz poll (see refreshGridFromBackend)
  paintCell(v, s);
  if (setStepFn) setStepFn(v, s, vel);   // push to C++ atomics
}

function renderGridColumns() {
  const rows = document.getElementById("gridRows");
  if (!rows) return;
  rows.textContent = "";
  lastPhaseCol = -1;   // cells rebuilt — force the playhead class to re-apply
  for (let v = 0; v < NUM_VOICES; v++) {
    cellEls[v].fill(null);
    const row = document.createElement("div");
    row.className = "grid-row";
    row.style.setProperty("--voice-hue", VOICES[v].hue);

    const label = document.createElement("div");
    label.className = "row-label";
    VOICE_NAME_WRITERS[VOICES[v].prefix](label);
    row.appendChild(label);

    const cells = document.createElement("div");
    cells.className = "row-cells";
    cells.style.gridTemplateColumns = `repeat(${patternLen}, 1fr)`;
    for (let s = 0; s < patternLen; s++) {
      const cell = document.createElement("div");
      cell.className = "cell" + (s % 4 === 0 ? " beat-start" : "");
      cell.dataset.voice = String(v);
      cell.dataset.step = String(s);
      cell.setAttribute("tabindex", "0");
      cell.setAttribute("role", "button");
      // createElement, not innerHTML: the copy on this page is table-sourced now
      // and localized copy must never reach a markup path (check-i18n assertion
      // 9 forbids an angle bracket in an i18n.js literal for the same reason).
      const fillEl = document.createElement("div");
      fillEl.className = "cell-fill";
      const markEl = document.createElement("div");
      markEl.className = "cell-mark";
      cell.appendChild(fillEl);
      cell.appendChild(markEl);
      cell.addEventListener("click", (e) => { e.preventDefault(); applyStep(v, s, nextVelocity(gridState[v][s])); });
      cell.addEventListener("contextmenu", (e) => { e.preventDefault(); applyStep(v, s, 0); });
      // keyboard: Enter/Space cycles velocity, Delete/Backspace clears the cell
      cell.addEventListener("keydown", (e) => {
        if (e.key === "Enter" || e.key === " ") { e.preventDefault(); applyStep(v, s, nextVelocity(gridState[v][s])); }
        else if (e.key === "Delete" || e.key === "Backspace") { e.preventDefault(); applyStep(v, s, 0); }
      });
      cells.appendChild(cell);
      cellEls[v][s] = cell;
      paintCell(v, s);
    }
    row.appendChild(cells);
    rows.appendChild(row);
  }
}

// Pull the authoritative grid from C++ (boot + after any host state restore).
let getGridFn = null, clearGridFn = null, gridPollBusy = false;
let applyPresetFn = null;
let lastLocalEditTime = -1;
const LOCAL_EDIT_HOLDOFF_MS = 300;   // a poll snapshot older than an in-flight click must not win

// Clear every cell (UI + C++ atomics) via the clearGrid native fn.
function clearAllSteps() {
  for (let v = 0; v < NUM_VOICES; v++)
    for (let s = 0; s < MAX_STEPS; s++) { gridState[v][s] = 0; if (s < patternLen) paintCell(v, s); }
  lastLocalEditTime = performance.now();
  if (clearGridFn) clearGridFn();
  else if (setStepFn) for (let v = 0; v < NUM_VOICES; v++) for (let s = 0; s < MAX_STEPS; s++) setStepFn(v, s, 0);
}
// `force` bypasses the local-edit holdoff (boot, preset load — authoritative pulls).
async function refreshGridFromBackend(force = false) {
  if (!getGridFn || gridPollBusy) return;
  const editedRecently = () =>
    !force && lastLocalEditTime >= 0 && performance.now() - lastLocalEditTime < LOCAL_EDIT_HOLDOFF_MS;
  if (editedRecently()) return;   // a cell click may still be in flight to C++
  gridPollBusy = true;
  try {
    const flat = await getGridFn();   // length 6*32, row-major
    // Re-check after the await: a click during the round-trip means this
    // snapshot is stale for that cell — drop it, the next poll is ≤250 ms away.
    if (!editedRecently() && Array.isArray(flat) && flat.length >= NUM_VOICES * MAX_STEPS) {
      for (let v = 0; v < NUM_VOICES; v++)
        for (let s = 0; s < MAX_STEPS; s++) {
          const vel = flat[v * MAX_STEPS + s] | 0;
          if (vel !== gridState[v][s]) { gridState[v][s] = vel; if (s < patternLen) paintCell(v, s); }
        }
    }
  } catch (e) { /* backend not ready */ }
  gridPollBusy = false;
}

// ── Timing / groove lane ─────────────────────────────────────────────────────
function makeCanvas(id) {
  const canvas = document.getElementById(id);
  if (!canvas) { console.error(`Missing canvas: ${id}`); return null; }
  const ctx = canvas.getContext("2d");
  const resize = () => {
    const dpr = window.devicePixelRatio || 1;
    canvas.width = Math.max(1, Math.round(canvas.clientWidth * dpr));
    canvas.height = Math.max(1, Math.round(canvas.clientHeight * dpr));
    ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
  };
  resize();
  return { canvas, ctx, resize };
}

let lane = null;
let sampleRate = 44100, currentBpm = 120;
let playheadPhase = 0;
const laneHits = [];                 // { v, step, dSteps, vel, src, life }
const LANE_LIFE = 95;                // frames a dot persists (~1.6 s @ 60 Hz)

function samplesPer16th() { return (60 / Math.max(1, currentBpm)) * sampleRate * 0.25; }

function pushLaneHit(h) {
  const sp16 = samplesPer16th();
  const dSteps = sp16 > 0 ? h.d / sp16 : 0;     // Δt as a fraction of a 16th step
  laneHits.push({ v: h.v, step: h.step, dSteps, vel: h.vel, src: h.src, life: LANE_LIFE });
  if (laneHits.length > 256) laneHits.splice(0, laneHits.length - 256);
}

function drawLane() {
  if (!lane) return;
  const { canvas, ctx } = lane;
  const w = canvas.clientWidth, h = canvas.clientHeight;
  ctx.clearRect(0, 0, w, h);
  if (patternLen <= 0) return;
  const stepW = w / patternLen;
  const bandH = h / NUM_VOICES;
  const xNominal = (k) => (k + 0.5) * stepW;

  // nominal grid lines (every step; heavier on beat starts)
  for (let k = 0; k < patternLen; k++) {
    const x = xNominal(k);
    ctx.strokeStyle = k % 4 === 0 ? "rgba(92,64,51,0.4)" : "rgba(139,115,85,0.18)";
    ctx.lineWidth = k % 4 === 0 ? 1.5 : 1;
    ctx.beginPath(); ctx.moveTo(x, 0); ctx.lineTo(x, h); ctx.stroke();
  }
  // faint voice-band separators
  ctx.strokeStyle = "rgba(139,115,85,0.12)";
  ctx.lineWidth = 1;
  for (let v = 1; v < NUM_VOICES; v++) { const y = v * bandH; ctx.beginPath(); ctx.moveTo(0, y); ctx.lineTo(w, y); ctx.stroke(); }

  // playhead sweep
  const xph = ((playheadPhase % patternLen + patternLen) % patternLen + 0.5) * stepW;
  ctx.strokeStyle = "rgba(232,176,74,0.85)";
  ctx.lineWidth = 2;
  ctx.beginPath(); ctx.moveTo(xph, 0); ctx.lineTo(xph, h); ctx.stroke();

  // hits — connector from grid line to actual position, dot at actual
  for (const hit of laneHits) {
    const a = Math.max(0, hit.life / LANE_LIFE);
    const yc = (hit.v + 0.5) * bandH;
    const k = hit.step < 0 ? Math.round(((playheadPhase % patternLen) + patternLen) % patternLen) : hit.step;
    const xn = xNominal(k);
    const xa = xn + hit.dSteps * stepW;
    const hue = VOICES[hit.v] ? VOICES[hit.v].hue : "#6b8e4e";
    // connector
    ctx.strokeStyle = `rgba(92,64,51,${0.3 * a})`;
    ctx.lineWidth = 1;
    ctx.beginPath(); ctx.moveTo(xn, yc); ctx.lineTo(xa, yc); ctx.stroke();
    // dot (radius scales with velocity)
    const r = 2.5 + (hit.vel / 127) * 4;
    ctx.globalAlpha = a;
    ctx.fillStyle = hue;
    ctx.beginPath(); ctx.arc(xa, yc, r, 0, Math.PI * 2); ctx.fill();
    if (hit.src === 1) { ctx.strokeStyle = "rgba(255,248,220,0.9)"; ctx.lineWidth = 1.2; ctx.stroke(); }
    ctx.globalAlpha = 1;
  }
}

function ageLaneHits() {
  for (let i = laneHits.length - 1; i >= 0; i--) { if (--laneHits[i].life <= 0) laneHits.splice(i, 1); }
}

// ── Live MIDI readout ────────────────────────────────────────────────────────
const NOTE_NAMES = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"];
function noteName(n) { return `${NOTE_NAMES[n % 12]}${Math.floor(n / 12) - 1}`; }
let midiReadout = null;
function pushMidiRow(h) {
  if (!midiReadout) return;
  const voice = VOICES[h.v];
  const note = voice ? voice.note : 0;
  const row = document.createElement("div");
  row.className = "midi-row fresh";

  // Four localized fields, built with createElement + setLabel. Two calls behind
  // an if/else for the source tag rather than a ternary in the argument:
  // check-i18n assertion 13 rejects a conditional anywhere inside a setLabel
  // call, and it is right to — a reviewer cannot tell a message-selection
  // ternary from a plural one by reading it. The v1.0.3 "SEQ " padded itself to
  // MIDI's width with a trailing space inside the string; the width is a CSS
  // min-width now, so neither face carries invisible layout.
  const srcEl = document.createElement("span");
  if (h.src === 1) { srcEl.className = "src-midi"; setLabel(srcEl, "label.srcMidi"); }
  else             { srcEl.className = "src-seq";  setLabel(srcEl, "label.srcSeq"); }

  const noteEl = document.createElement("span");
  noteEl.className = "mr-note";
  setLabel(noteEl, "label.midiNote", { n: note });

  const nameEl = document.createElement("span");
  nameEl.className = "mr-voice";
  if (voice) VOICE_NAME_WRITERS[voice.prefix](nameEl);

  const velEl = document.createElement("span");
  velEl.className = "mr-vel";
  setLabel(velEl, "label.midiVel", { v: h.vel });

  row.appendChild(srcEl);
  row.appendChild(document.createTextNode(" "));
  row.appendChild(noteEl);
  row.appendChild(document.createTextNode(" "));
  row.appendChild(nameEl);
  row.appendChild(document.createTextNode(" "));
  row.appendChild(velEl);
  midiReadout.insertBefore(row, midiReadout.firstChild);   // newest at bottom (column-reverse)
  while (midiReadout.childElementCount > 40) midiReadout.removeChild(midiReadout.lastChild);
}

// ── The one C++→JS push: a per-frame snapshot from the editor Timer ──────────
function onFrame(frame) {
  if (!frame) return;
  if (typeof frame.bpm === "number" && frame.bpm > 0) {
    currentBpm = frame.bpm;
    const tEl = document.getElementById("readTempo");
    if (tEl) tEl.textContent = String(Math.round(currentBpm));
  }
  if (typeof frame.ph === "number") playheadPhase = frame.ph;
  // live SR (host can switch rates after boot; boot's getSampleRate is just a seed)
  if (typeof frame.sr === "number" && frame.sr > 0) sampleRate = frame.sr;

  // THE TRANSPORT STATE LINE IS COPY, NOT A VALUE MIRROR. There is no sync
  // parameter in this plugin's APVTS — frame.sync is host transport state pushed
  // from the editor Timer — so translating it cannot make the page and a host
  // automation lane disagree, which is the D-01 test that keeps O-simpleSubtractive's
  // and O-simplePhysicalModelSynth's diagram state lines in English. Two
  // literal-keyed setLabel calls behind an if/else, never a ternary in the
  // argument (assertion 13). The bullet is inside BOTH strings in BOTH
  // languages, so no writer ever has to re-attach the glyph.
  const tEl = document.getElementById("readTransport");
  if (tEl) {
    const synced = !!frame.sync;
    if (synced) setLabel(tEl, "label.synced");
    else        setLabel(tEl, "label.freeRun");
    tEl.classList.toggle("synced", synced);
  }

  const hits = frame.hits || [];
  for (const h of hits) {
    // h = { v, s, vel, src, d, n }
    const hit = { v: h.v | 0, step: (h.s === undefined ? -1 : h.s) | 0, vel: h.vel | 0, src: h.src | 0, d: h.d || 0 };
    pushLaneHit(hit);
    pushMidiRow(hit);
    if (hit.src === 0 && hit.step >= 0 && hit.step < patternLen) flashCell(hit.v, hit.step);
  }
}

let lastPhaseCol = -1;
function updatePlayheadColumn() {
  const col = ((Math.floor(playheadPhase) % patternLen) + patternLen) % patternLen;
  if (col === lastPhaseCol) return;
  for (let v = 0; v < NUM_VOICES; v++) {
    if (lastPhaseCol >= 0 && cellEls[v][lastPhaseCol]) cellEls[v][lastPhaseCol].classList.remove("playhead");
    if (cellEls[v][col]) cellEls[v][col].classList.add("playhead");
  }
  lastPhaseCol = col;
}

const flashTimers = {};
function flashCell(v, s) {
  const el = cellEls[v][s];
  if (!el) return;
  el.classList.add("flash");
  const key = `${v}:${s}`;
  if (flashTimers[key]) clearTimeout(flashTimers[key]);
  flashTimers[key] = setTimeout(() => el.classList.remove("flash"), 110);
}

// ── Animation loop (drives the lane + playhead column at display rate) ───────
let gridPollAccum = 0;
function raf(ts) {
  ageLaneHits();
  drawLane();
  updatePlayheadColumn();
  refreshGridAria();
  // poll the authoritative grid ~4×/s so host preset/state restores show up
  gridPollAccum++;
  if (gridPollAccum >= 15) { gridPollAccum = 0; refreshGridFromBackend(); }
  requestAnimationFrame(raf);
}

// ── Hover-help switch (v1.2.0) ──────────────────────────────────────────────
// The toggle O-simpleGrain carries, ported. It is a BROWSER-side preference
// under localStorage "osbm.tipsEnabled", not session state: this plugin has no
// tooltips bridge and never had one, and C++ state for it would be a new
// persistence surface rather than a settings-panel change. The tooltip engine
// below reads tipsEnabled at show time, so the switch takes effect on the next
// hover with nothing to rebind.
let tipsEnabled = true;                 // shipped default; localStorage wins at boot
let hideTooltip = () => {};             // published by the tooltip setup (used by the toggle)

function applyTipsEnabled(on) {
  tipsEnabled = !!on;
  if (!tipsEnabled) hideTooltip();

  const btn = document.getElementById("help-toggle");
  if (!btn) return;
  btn.setAttribute("aria-pressed", tipsEnabled ? "true" : "false");
  // Two calls behind an if/else, never a ternary in the setLabel argument:
  // check-i18n assertion 13 rejects a conditional anywhere inside the call, and
  // it is right to — a reviewer cannot tell a message-selection ternary from a
  // hand-inflection one.
  if (tipsEnabled) setLabel(btn, "ui.on");
  else             setLabel(btn, "ui.off");
}

function setupTipsToggle() {
  const btn = document.getElementById("help-toggle");
  if (!btn) { console.error("Missing help-toggle element"); return; }

  let stored = null;
  try { stored = localStorage.getItem("osbm.tipsEnabled"); } catch (e) { stored = null; }
  applyTipsEnabled(stored !== "false");

  btn.addEventListener("click", () => {
    applyTipsEnabled(!tipsEnabled);
    try { localStorage.setItem("osbm.tipsEnabled", String(tipsEnabled)); } catch (e) { /* private mode */ }
  });
}

// ── Tooltips ─────────────────────────────────────────────────────────────────
// Single floating #tooltip div. The copy is no longer looked up here: applyI18n
// has already written it onto each anchor as data-tip-title + data-tip, in the
// current language, and it rewrites both on every language change. This function
// only positions and shows what the anchor carries. Avoids native title= (slow,
// unstyled OS tooltips, and untranslated — contract §4; this page never carried
// one, and index.html still carries zero).
function initTooltips() {
  const tip = document.getElementById("tooltip");
  if (!tip) return;
  let active = null;

  // Built with createElement + textContent, not innerHTML. The tip text is
  // table-sourced and localized now rather than a fixed literal, and localized
  // copy must never reach a markup path. v1.0.3's tip bodies carried strong/em
  // tags; the words are unchanged and the tags are gone (check-i18n assertion 9
  // forbids an angle bracket in an i18n.js string literal).
  const show = (el, x, y) => {
    if (!tipsEnabled) return;
    const title = el.getAttribute("data-tip-title");
    const body = el.getAttribute("data-tip");
    if (!title && !body) return;
    tip.textContent = "";
    if (title) {
      const t = document.createElement("span");
      t.className = "tt-title";
      t.textContent = title;
      tip.appendChild(t);
    }
    if (body) tip.appendChild(document.createTextNode(body));
    tip.classList.add("show"); tip.setAttribute("aria-hidden", "false");
    position(x, y);
  };
  const position = (x, y) => {
    const r = tip.getBoundingClientRect();
    let nx = x + 14, ny = y + 16;
    if (nx + r.width > window.innerWidth - 8) nx = x - r.width - 14;
    if (ny + r.height > window.innerHeight - 8) ny = y - r.height - 14;
    tip.style.left = `${Math.max(6, nx)}px`; tip.style.top = `${Math.max(6, ny)}px`;
  };
  const hide = () => { active = null; tip.classList.remove("show"); tip.setAttribute("aria-hidden", "true"); };
  hideTooltip = hide;

  // DELEGATED on the document rather than attached per element. No anchor
  // carries data-tip until applyI18n has run, so the v1.0.3
  // querySelectorAll("[data-tip]") at setup time would bind NOTHING at all — and
  // 36 of the 55 anchors are built by buildVoiceStrips and do not exist when the
  // markup is parsed. Delegation has no ordering to get wrong.
  // pointerover/pointerout and focusin/focusout are used because — unlike
  // pointerenter/pointerleave and focus/blur — they bubble. closest() also gets
  // the nesting right for free: a knob cell sits INSIDE the panel anchor and a
  // lesson chip inside the lesson-row anchor, and the innermost wins with no
  // stopPropagation.
  const anchorOf = (t) => (t && t.closest ? t.closest("[data-tip]") : null);

  document.addEventListener("pointerover", (e) => {
    const el = anchorOf(e.target);
    if (!el || el === active) return;
    active = el;
    show(el, e.clientX, e.clientY);
  });
  document.addEventListener("pointermove", (e) => {
    if (active && anchorOf(e.target) === active) position(e.clientX, e.clientY);
  });
  document.addEventListener("pointerout", (e) => {
    if (!active) return;
    // Ignore a move between two descendants of the SAME anchor: pointerout fires
    // on every child boundary and would flicker the tip off and on.
    if (anchorOf(e.relatedTarget) === active) return;
    hide();
  });
  document.addEventListener("pointerdown", hide);

  document.addEventListener("focusin", (e) => {
    const el = anchorOf(e.target);
    if (!el) return;
    active = el;
    const r = el.getBoundingClientRect();
    show(el, r.left + r.width / 2, r.bottom);
  });
  document.addEventListener("focusout", hide);

  document.addEventListener("keydown", (e) => { if (e.key === "Escape") hide(); });
}

// ── Settings popover (v1.1.0) ───────────────────────────────────────────────
// The gear panel holding the language selector and, since v1.2.0, the
// hover-help switch (wired by setupTipsToggle above). All state lives in this
// closure, so nothing here can join a TDZ chain.
//
// Styled in this plugin's own aged-paper field-guide vocabulary in
// css/styles.css: the panel wears the .group plate's paper fill and brown rule,
// the gear wears the .tour-btn chip's border and paper fill rounded to a circle
// (and lights the same green when open), and the selector wears select.combo's
// border, radius and caret at panel scale. It is not a widget pasted in
// unchanged from another plugin.
function setupSettingsPopover() {
  const gearBtn = document.getElementById("gear-btn");
  const popover = document.getElementById("settings-popover");

  if (gearBtn === null || popover === null) {
    console.warn("Settings popover missing — language selector unavailable");
    return;
  }

  const setOpen = (open) => {
    popover.hidden = !open;
    gearBtn.setAttribute("aria-expanded", open ? "true" : "false");
    gearBtn.classList.toggle("open", open);
  };

  gearBtn.addEventListener("click", (e) => {
    e.stopPropagation();
    setOpen(popover.hidden);
  });

  // Dismiss on a press anywhere else, and on Escape. pointerdown rather than
  // click, so the panel is gone before a drag on a knob underneath it begins —
  // bindKnob calls preventDefault in its own pointerdown handler.
  document.addEventListener("pointerdown", (e) => {
    if (popover.hidden) return;
    if (popover.contains(e.target) || gearBtn.contains(e.target)) return;
    setOpen(false);
  });

  document.addEventListener("keydown", (e) => {
    if (e.key === "Escape" && !popover.hidden) {
      setOpen(false);
      gearBtn.focus();
    }
  });
}

// ── Preset tour (concept-isolating factory presets — FUNC-05) ────────────────
// DOM button order matches the C++ kBeatPresets[] index order (0 Straight … 5
// Quantize Demo), so the forEach index IS the preset index. applyPreset sets the
// timing-feel params host-notifying (knobs + length combo auto-update via their
// attachments); we only need to repaint the grid velocities afterwards.
function initPresetTour() {
  const caption = document.getElementById("tourCaption");
  document.querySelectorAll(".tour-btn").forEach((btn, index) => {
    btn.addEventListener("click", async () => {
      document.querySelectorAll(".tour-btn").forEach((b) => b.classList.remove("armed"));
      btn.classList.add("armed");
      if (applyPresetFn) {
        try { await applyPresetFn(index); } catch (e) { console.error("applyPreset failed", e); }
        await refreshGridFromBackend(true);   // authoritative pull — bypass the edit holdoff
      }
      // The NAME substitutes verbatim (D-02): it is the factory preset name the
      // chip carries and the one C++ knows. Hoisted to a const because
      // assertion 13 rejects a conditional ANYWHERE inside a setLabel call,
      // including inside the vars object.
      const presetName = btn.getAttribute("data-preset") || "";
      if (caption) setLabel(caption, "label.tourLoaded", { name: presetName });
    });
  });
}

// ── Per-voice strips (built here so IDs stay in lock-step) ────────────────────
// createElement throughout, not the v1.0.3 innerHTML template. Two reasons, and
// both are load-bearing: localized copy must never reach a markup path, and a
// template string carrying prose is reported by check-i18n assertion 12 as a raw
// English write that no I18N_EXEMPT entry can cover — an exemption lives in
// i18n.js, where assertion 9 forbids the angle bracket the template is made of.
//
// The four knob captions and the two toggle faces repeat identically down all
// six strips, so they are ONE key each applied six times by the writers above.
// The tip ANCHOR is data-param (the parameter ID) on the knob cell and the id
// bindToggle already needs on the buttons; TIP_BINDINGS names all 36 of them
// individually, because document.querySelector returns the FIRST match.
function buildVoiceStrips() {
  const host = document.getElementById("voiceStrips");
  if (!host) return;
  host.textContent = "";
  for (const v of VOICES) {
    const strip = document.createElement("div");
    strip.className = "voice-strip";
    strip.style.setProperty("--voice-hue", v.hue);

    const nameEl = document.createElement("div");
    nameEl.className = "vs-name";
    VOICE_NAME_WRITERS[v.prefix](nameEl);
    strip.appendChild(nameEl);

    const knobs = document.createElement("div");
    knobs.className = "vs-knobs";
    for (const suffix of VOICE_SLIDER_SUFFIX) {
      const id = v.prefix + suffix;

      const cell = document.createElement("div");
      cell.className = "knob-cell cell-sm";
      cell.dataset.param = id;

      const knob = document.createElement("div");
      knob.className = "knob knob-sm";
      knob.id = `knob-${id}`;
      // The accessible name reads the control's own tooltip title through
      // trLabel's I18N fallback — an accessible name IS the control's name. A
      // computed key is fine HERE (assertion 13 governs setLabel, and the four
      // tooltip keys are I18N entries, which assertion 15 does not dead-check).
      knob.dataset.i18nAria = VOICE_KNOB_ARIA[suffix];
      const stem = document.createElement("div");
      stem.className = "knob-stem";
      knob.appendChild(stem);

      const cap = document.createElement("div");
      cap.className = "knob-label";
      VOICE_KNOB_WRITERS[suffix](cap);

      const val = document.createElement("div");
      val.className = "knob-value";
      val.id = `val-${id}`;
      val.textContent = "—";   // a readout placeholder, replaced on the first bind (D-03)

      cell.appendChild(knob);
      cell.appendChild(cap);
      cell.appendChild(val);
      knobs.appendChild(cell);
    }
    strip.appendChild(knobs);

    const toggles = document.createElement("div");
    toggles.className = "vs-toggles";
    for (const suffix of VOICE_TOGGLE_SUFFIX) {
      const btn = document.createElement("button");
      btn.className = `toggle-btn ${suffix.toLowerCase()}`;
      btn.id = `toggle-${v.prefix}${suffix}`;
      btn.type = "button";
      VOICE_TOGGLE_WRITERS[suffix](btn);
      toggles.appendChild(btn);
    }
    strip.appendChild(toggles);

    host.appendChild(strip);
  }
}

// ── Boot ─────────────────────────────────────────────────────────────────────
async function boot() {
  buildVoiceStrips();

  // native functions (must match PluginEditor withNativeFunction registrations)
  try { setStepFn = Juce.getNativeFunction("setStep"); } catch (e) { setStepFn = null; console.error("setStep native fn unavailable", e); }
  try { getGridFn = Juce.getNativeFunction("getGrid"); } catch (e) { getGridFn = null; }
  try { clearGridFn = Juce.getNativeFunction("clearGrid"); } catch (e) { clearGridFn = null; }
  try { applyPresetFn = Juce.getNativeFunction("applyPreset"); } catch (e) { applyPresetFn = null; console.error("applyPreset native fn unavailable", e); }
  try { sampleRate = await Juce.getNativeFunction("getSampleRate")(); } catch (e) { sampleRate = 44100; }
  try { paramDefaults = await Juce.getNativeFunction("getParameterDefaults")(); } catch (e) { paramDefaults = null; }

  // bind all params
  for (const id of SLIDER_IDS) bindKnob(id);
  for (const id of COMBO_IDS) bindCombo(id);   // sets patternLen + renders columns
  for (const id of TOGGLE_IDS) bindToggle(id);

  // grid (renderGridColumns already ran via bindCombo refresh; ensure + paint)
  if (!document.querySelector(".cell")) renderGridColumns();
  await refreshGridFromBackend(true);

  // canvases + visuals
  lane = makeCanvas("laneCanvas");
  midiReadout = document.getElementById("midiReadout");

  const clearBtn = document.getElementById("clearGridBtn");
  if (clearBtn) clearBtn.addEventListener("click", clearAllSteps);

  initTooltips();
  initPresetTour();

  // The popover and the language sweep go here, EACH IN ITS OWN try/catch, and
  // AFTER buildVoiceStrips() and the grid render above — the sweep must see the
  // finished DOM or the 36 generated anchors get no tip and no accessible name
  // (batch I2 lesson 1.3). applyI18n is what writes data-tip onto every anchor;
  // initTooltips above reads that attribute, but delegation means the order
  // between the two is free. A translation-table typo must not be allowed to
  // take the 42 parameter bindings, the grid and the lane down with it — that is
  // the TDZ failure this repo has already paid for once.
  try { setupSettingsPopover(); } catch (e) { console.error("settings popover init failed:", e); }
  try { initI18n(); }             catch (e) { console.error("i18n init failed:", e); }
  try { setupTipsToggle(); }      catch (e) { console.error("tips toggle init failed:", e); }

  // C++ → JS per-frame push (playhead + drained hits + transport)
  if (window.__JUCE__ && window.__JUCE__.backend) {
    window.__JUCE__.backend.addEventListener("frame", onFrame);
  } else {
    console.error("window.__JUCE__.backend unavailable — playhead/lane/MIDI will not animate.");
  }

  window.addEventListener("resize", () => { if (lane) lane.resize(); });
  requestAnimationFrame(raf);
}

if (document.readyState === "loading") document.addEventListener("DOMContentLoaded", boot);
else boot();
