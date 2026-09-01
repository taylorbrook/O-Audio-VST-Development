/*
   This file is part of O-simplePhysicalModelSynth, an Ouaricon Audio plugin.
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
// O-simplePhysicalModelSynth — WebView UI controller (Phases 3.1 + 3.2 + 3.3)
// Binds all 17 APVTS params two-way (14 knobs + 3 combos), wires the
// resonator-/exciter-aware grey-out, the persistent preset bar, the on-screen
// keyboard, the live spectrum + oscilloscope, the SVG loop/flow diagram (String
// loop ↔ Modal stems, driven by loopEnergy), and the on-hover tooltips.
//
// INTERFACE COPY (v1.2.0): every label, heading, button face, hint and tooltip
// on this page comes from js/i18n.js in English or French. applyI18n() writes
// each tooltip onto its anchor as data-tip-title + data-tip and each caption
// into its [data-i18n] element; the renderer below only positions what the
// anchor already carries. Value readouts, the factory preset names and the three
// AudioParameterChoice drop-downs stay English (D-01/D-02/D-03).
//
// NOTE: getSliderState / getComboBoxState / getNativeFunction live on the `Juce`
// ES-module namespace (used for control state + native-fn CALLS). The low-level
// backend (window.__JUCE__.backend.addEventListener) is used ONLY to RECEIVE the
// pushed spectrumUpdate / scopeUpdate / loopUpdate viz frames — never for calls.
// ============================================================================

import * as Juce from "./juce/index.js";
import { PresetManager } from "../modules/preset-manager.js";

// ════════════════════════════════════════════════════════════════════════════
// INTERFACE LANGUAGE (v1.2.0) — canon v2, verbatim from scripts/i18n-canon.js.
//
// It sits HERE, directly under the imports and above every other declaration in
// this module, because `uiLanguage` is a module-level `let`: anything above it
// that reached applyLabel() would touch it inside its temporal dead zone and
// throw out of module evaluation, taking the seventeen parameter bindings, the
// two canvases, the loop diagram and the on-screen keyboard with it
// (pattern_module_toplevel_init_tdz). This file DOES have eager top-level work
// below — KNOB_IDS, FORMAT, the stem constants — so the ordering is load-bearing
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


// ── Parameter inventory (must match ParamIDs / the C++ relays exactly) ──────
const KNOB_IDS = [
  "excitationPosition", "excitationColor", "bowForce",
  "inharmonicity", "modeBrightness",
  "damping", "decay", "material",
  "coarseTune", "fineTune",
  "ampAttack", "ampRelease", "velToBrightness", "outputLevel",
];
const COMBO_IDS = ["excitationType", "resonatorType", "stringModel"];

// ── Tooltip copy ────────────────────────────────────────────────────────────
// It lives in js/i18n.js now, in BOTH languages, and applyI18n writes it onto
// each anchor as data-tip-title + data-tip. Through v1.1.0 a TIPS object here
// held [title, bodyHtml] pairs keyed by the anchor's OWN data-tip attribute —
// which canon v2 overwrites with the tip BODY, so the key and the copy would
// have fought over one attribute, and check-i18n assertion 3 requires index.html
// to carry zero data-tip literals. The seventeen control cells carry a data-param
// attribute from v1.2.0; the four diagram boxes carry an id.

// ── Display formatters (keyed by param id) ──────────────────────────────────
// Each receives the *scaled* value (NormalisableRange::convertFrom0to1 output).
const fmtPct = (v) => `${Math.round(v)}%`;
const fmtSec = (s) => (s < 1 ? `${Math.round(s * 1000)} ms` : `${s.toFixed(2)} s`);
const fmtSigned = (v, unit) => `${v >= 0 ? "+" : ""}${Math.round(v)} ${unit}`;
const FORMAT = {
  excitationPosition: fmtPct,
  excitationColor: fmtPct,
  bowForce: fmtPct,
  inharmonicity: fmtPct,
  modeBrightness: fmtPct,
  damping: fmtPct,
  decay: fmtPct,
  material: fmtPct,
  coarseTune: (v) => fmtSigned(v, "st"),
  fineTune: (v) => fmtSigned(v, "c"),
  ampAttack: fmtSec,
  ampRelease: fmtSec,
  velToBrightness: fmtPct,
  outputLevel: (v) => `${v.toFixed(1)} dB`,
};

// ── Knob geometry ────────────────────────────────────────────────────────────
const KNOB_MIN_DEG = -135;   // 0.0 normalised
const KNOB_MAX_DEG = 135;    // 1.0 normalised
const DRAG_TRAVEL_PX = 220;  // vertical px for full 0..1 sweep

const sliderState = {};   // id -> Juce SliderState
const comboState = {};    // id -> Juce ComboBoxState

function normToDeg(n) { return KNOB_MIN_DEG + n * (KNOB_MAX_DEG - KNOB_MIN_DEG); }

function updateKnobVisual(id) {
  const st = sliderState[id];
  if (!st) return;
  const norm = st.getNormalisedValue();
  const knob = document.getElementById(`knob-${id}`);
  if (knob) {
    const stem = knob.querySelector(".knob-stem");
    if (stem) stem.style.transform = `translate(-50%, -100%) rotate(${normToDeg(norm)}deg)`;
  }
  const valEl = document.getElementById(`val-${id}`);
  if (valEl) {
    const fmt = FORMAT[id] || ((v) => v.toFixed(2));
    valEl.textContent = fmt(st.getScaledValue());
  }
}

// One-shot fine adjust shared by the wheel + arrow-key paths (full drag gesture).
function nudge(st, delta, id) {
  const n = Math.max(0, Math.min(1, st.getNormalisedValue() + delta));
  st.sliderDragStarted();
  st.setNormalisedValue(n);
  st.sliderDragEnded();
  updateKnobVisual(id);
}

// ── Knob binding (relative vertical drag) ──────────────────────────────────
function bindKnob(id) {
  const st = Juce.getSliderState(id);
  sliderState[id] = st;

  st.valueChangedEvent.addListener(() => updateKnobVisual(id));
  st.propertiesChangedEvent.addListener(() => updateKnobVisual(id));
  updateKnobVisual(id);

  const knob = document.getElementById(`knob-${id}`);
  if (!knob) { console.error(`Missing knob element: knob-${id}`); return; }

  // Keyboard operable (accessibility): focusable + arrow-key fine adjust.
  knob.setAttribute("tabindex", "0");
  knob.setAttribute("role", "slider");
  knob.addEventListener("keydown", (e) => {
    let delta = 0;
    if (e.key === "ArrowUp" || e.key === "ArrowRight") delta = 0.02;
    else if (e.key === "ArrowDown" || e.key === "ArrowLeft") delta = -0.02;
    else return;
    nudge(st, delta, id);
    e.preventDefault();
  });

  let dragging = false, startY = 0, startNorm = 0;

  const onMove = (e) => {
    if (!dragging) return;
    let n = startNorm + (startY - e.clientY) / DRAG_TRAVEL_PX;
    n = Math.max(0, Math.min(1, n));
    st.setNormalisedValue(n);
    updateKnobVisual(id);
    e.preventDefault();
  };
  const onUp = () => {
    if (!dragging) return;
    dragging = false;
    st.sliderDragEnded();
    window.removeEventListener("pointermove", onMove);
    window.removeEventListener("pointerup", onUp);
  };

  knob.addEventListener("pointerdown", (e) => {
    dragging = true;
    startY = e.clientY;
    startNorm = st.getNormalisedValue();
    st.sliderDragStarted();
    window.addEventListener("pointermove", onMove);
    window.addEventListener("pointerup", onUp);
    e.preventDefault();
  });

  knob.addEventListener("wheel", (e) => {
    nudge(st, e.deltaY < 0 ? 0.02 : -0.02, id);
    e.preventDefault();
  }, { passive: false });
}

// ── Combo box binding (excitation type, resonator type, string model) ───────
function bindCombo(id) {
  const st = Juce.getComboBoxState(id);
  comboState[id] = st;
  const sel = document.getElementById(`combo-${id}`);
  if (!sel) { console.error(`Missing combo element: combo-${id}`); return; }

  const buildOptions = () => {
    const choices = (st.properties && st.properties.choices) || [];
    if (choices.length === 0) return false;
    if (sel.options.length === choices.length) return true;   // already built
    sel.innerHTML = "";
    choices.forEach((c, i) => {
      const opt = document.createElement("option");
      opt.value = String(i);
      opt.textContent = c;
      sel.appendChild(opt);
    });
    return true;
  };
  const refresh = () => {
    buildOptions();
    const idx = st.getChoiceIndex();
    if (idx >= 0 && idx < sel.options.length) sel.selectedIndex = idx;
  };
  st.propertiesChangedEvent.addListener(refresh);
  st.valueChangedEvent.addListener(refresh);
  refresh();

  sel.addEventListener("change", () => st.setChoiceIndex(sel.selectedIndex));
}

// ── Resonator-/exciter-aware grey-out (D5) ──────────────────────────────────
// Disabled-but-visible (.pm-disabled). The resonatorType / excitationType
// selectors are NEVER gated, so the pointer-events escape hatch always holds.
function setDisabled(id, disabled) {
  // v1.2.0: was `[data-tip="${id}"]`. The tip ANCHOR moved to data-param when
  // applyI18n took ownership of data-tip, and this selector would have matched
  // nothing from that moment on — no error, no warning, the four gated controls
  // simply stop dimming. It is built from a TEMPLATE LITERAL, so a grep for the
  // static form `[data-tip="` does not find it.
  const cell = document.querySelector(`[data-param="${id}"]`);
  if (cell) cell.classList.toggle("pm-disabled", disabled);
}

function applyEngineGating() {
  const res = comboState.resonatorType ? comboState.resonatorType.getChoiceIndex() : 0; // 0=String,1=Modal
  const exc = comboState.excitationType ? comboState.excitationType.getChoiceIndex() : 0; // 0=Pluck,1=Strike,2=Bow
  setDisabled("stringModel",    res !== 0);   // String-only (cell is hidden since v1.2.2 — harmless)
  setDisabled("inharmonicity",  res !== 1);   // Modal-only
  setDisabled("modeBrightness", res !== 1);   // Modal-only
  setDisabled("bowForce",       exc !== 2);   // Bow-only
}

// ── Preset bar (persistent factory/user JSON presets) ───────────────────────
let presetManager = null;

function closeDropdown() {
  const dd = document.getElementById("presetDropdown");
  const nameBtn = document.getElementById("presetName");
  if (dd) dd.classList.remove("show");
  if (nameBtn) nameBtn.setAttribute("aria-expanded", "false");
}

async function buildPresetDropdown() {
  const dd = document.getElementById("presetDropdown");
  if (!dd || !presetManager) return;
  await presetManager.refresh();
  const list = presetManager.getPresetList();
  const current = presetManager.getCurrentPreset();
  const flags = await Promise.all(list.map((n) => presetManager.isFactoryPreset(n)));

  dd.innerHTML = "";
  // writeLabel, not a label STRING: the heading is localized and its key has to
  // be a plain literal at the call site (check-i18n assertion 13), so each caller
  // brings its own one-line writer. The preset NAMES below stay exactly as C++
  // reports them — a preset name is its JSON filename (D-02).
  const addGroup = (names, writeLabel) => {
    if (!names.length) return;
    const hdr = document.createElement("div");
    hdr.className = "preset-group-label";
    writeLabel(hdr);
    dd.appendChild(hdr);
    names.forEach((n) => {
      const item = document.createElement("div");
      item.className = "preset-dropdown-item" + (n === current ? " active" : "");
      item.setAttribute("role", "option");
      item.tabIndex = 0;
      item.textContent = n;
      const choose = () => { presetManager.loadPreset(n); closeDropdown(); };
      item.addEventListener("click", choose);
      item.addEventListener("keydown", (e) => {
        if (e.key === "Enter" || e.key === " ") { e.preventDefault(); choose(); }
      });
      dd.appendChild(item);
    });
  };
  addGroup(list.filter((_, i) => flags[i]),  (el) => setLabel(el, "label.presetFactory"));
  addGroup(list.filter((_, i) => !flags[i]), (el) => setLabel(el, "label.presetUser"));
}

function toggleDropdown() {
  const dd = document.getElementById("presetDropdown");
  const nameBtn = document.getElementById("presetName");
  if (!dd) return;
  if (dd.classList.contains("show")) { closeDropdown(); return; }
  buildPresetDropdown();
  dd.classList.add("show");
  if (nameBtn) nameBtn.setAttribute("aria-expanded", "true");
}

// Factory presets can't be deleted — disable the button so the click isn't a silent no-op.
async function updateDeleteButtonState() {
  const delBtn = document.getElementById("presetDelete");
  if (!delBtn || !presetManager) return;
  delBtn.disabled = await presetManager.isFactoryPreset(presetManager.getCurrentPreset());
}

function setupPresetManager() {
  const nameBtn = document.getElementById("presetName");

  presetManager = new PresetManager({
    displayElement: nameBtn,
    prevButton: document.getElementById("presetPrev"),
    nextButton: document.getElementById("presetNext"),
    saveButton: document.getElementById("presetSave"),  // → native save dialog
    getNativeFunction: Juce.getNativeFunction,           // ES-module namespace (has getNativeFunction)
    onPresetChanged: () => { applyEngineGating(); applyDiagramSkin(); updateDeleteButtonState(); closeDropdown(); },
  });
  presetManager.initialize().then(updateDeleteButtonState);

  if (nameBtn) nameBtn.addEventListener("click", toggleDropdown);

  const delBtn = document.getElementById("presetDelete");
  if (delBtn) delBtn.addEventListener("click", async () => {
    const ok = await presetManager.deletePreset(presetManager.getCurrentPreset());
    if (ok) closeDropdown();
  });

  document.addEventListener("pointerdown", (e) => {
    const bar = document.getElementById("presetBar");
    if (bar && !bar.contains(e.target)) closeDropdown();
  });
  document.addEventListener("keydown", (e) => { if (e.key === "Escape") closeDropdown(); });
}

// ── Spectrum + oscilloscope (Phase 3.2) ─────────────────────────────────────
// DPR-aware canvas: backing store sized to clientWidth*dpr (crisp on Retina); the
// canvas stretches to 100% inside a positioned overflow:hidden .canvas-wrap (the
// CSS replaced-element gotcha — never rely on right/bottom to stretch a <canvas>).
function makeCanvas(id) {
  const canvas = document.getElementById(id);
  if (!canvas) { console.error(`Missing canvas: ${id}`); return null; }
  const ctx = canvas.getContext("2d");
  const resize = () => {
    const dpr = window.devicePixelRatio || 1;
    const w = canvas.clientWidth, h = canvas.clientHeight;
    canvas.width = Math.max(1, Math.round(w * dpr));
    canvas.height = Math.max(1, Math.round(h * dpr));
    ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
  };
  resize();
  return { canvas, ctx, resize };
}

let specCanvas = null, scopeCanvas = null;
let lastSpectrum = null, lastScope = null;

// Spectrum x-axis is log-frequency 20 Hz → Nyquist (matches PmVizAnalyzer). The
// log axis is what makes the String harmonic comb read as an even ladder and the
// Modal modes (f_k = f0·k·√(1+B·k²)) read as uneven spacing. nyquistHz is fetched
// from C++ (getSampleRate); 22.05 kHz is a safe pre-fetch default.
let nyquistHz = 22050;
const FREQ_TICKS = [100, 1000, 10000];
const fmtTickHz = (f) => (f >= 1000 ? `${f / 1000}k` : `${f}`);

function drawSpectrum(arr) {
  lastSpectrum = arr;
  if (!specCanvas) return;
  const { canvas, ctx } = specCanvas;
  const w = canvas.clientWidth, h = canvas.clientHeight;
  ctx.clearRect(0, 0, w, h);

  // subtle horizontal grid lines (dB)
  ctx.strokeStyle = "rgba(139,115,85,0.18)";
  ctx.lineWidth = 1;
  for (let g = 1; g < 4; g++) {
    const y = (h * g) / 4;
    ctx.beginPath(); ctx.moveTo(0, y); ctx.lineTo(w, y); ctx.stroke();
  }

  const n = arr.length;          // 256 bins, dB in ~[-100, 0]
  const bw = w / n;
  for (let i = 0; i < n; i++) {
    const db = arr[i];
    const norm = Math.max(0, Math.min(1, (db + 100) / 100)); // 0..1
    const barH = norm * (h - 2);
    const x = i * bw;
    const hue = 90 - norm * 40;   // green -> yellow-green with intensity
    ctx.fillStyle = `hsl(${hue}, 55%, ${35 + norm * 30}%)`;
    ctx.fillRect(x, h - barH, Math.max(1, bw - 0.5), barH);
  }

  // Frequency-axis ticks (log scale, matching the analyzer's 20 Hz → Nyquist map).
  const logRange = Math.log(nyquistHz / 20);
  ctx.strokeStyle = "rgba(139,115,85,0.22)";
  ctx.fillStyle = "rgba(210,190,150,0.7)";
  ctx.font = "9px Garamond, 'Times New Roman', serif";
  ctx.textAlign = "center";
  for (const f of FREQ_TICKS) {
    if (f >= nyquistHz) continue;
    const x = (Math.log(f / 20) / logRange) * w;
    ctx.beginPath(); ctx.moveTo(x, 0); ctx.lineTo(x, h - 11); ctx.stroke();
    ctx.fillText(fmtTickHz(f), x, h - 2);
  }
}

function drawScope(arr) {
  lastScope = arr;
  if (!scopeCanvas) return;
  const { canvas, ctx } = scopeCanvas;
  const w = canvas.clientWidth, h = canvas.clientHeight;
  ctx.clearRect(0, 0, w, h);

  // centre line
  ctx.strokeStyle = "rgba(139,115,85,0.25)";
  ctx.lineWidth = 1;
  ctx.beginPath(); ctx.moveTo(0, h / 2); ctx.lineTo(w, h / 2); ctx.stroke();

  const n = arr.length;          // 128 pts in [-1, 1]
  ctx.strokeStyle = "#9ec46f";
  ctx.lineWidth = 2;
  ctx.lineJoin = "round";
  ctx.beginPath();
  for (let i = 0; i < n; i++) {
    const x = (i / (n - 1)) * w;
    const y = h / 2 - arr[i] * (h / 2 - 3);
    if (i === 0) ctx.moveTo(x, y); else ctx.lineTo(x, y);
  }
  ctx.stroke();
}

// Spectrum/scope frames are PUSHED from C++ via emitEventIfBrowserIsVisible — they
// arrive on the low-level backend (window.__JUCE__.backend.addEventListener), NOT
// on the Juce.* namespace (which is only for native-fn calls + control state).
function setupVizEvents() {
  if (window.__JUCE__ && window.__JUCE__.backend) {
    window.__JUCE__.backend.addEventListener("spectrumUpdate", (arr) => drawSpectrum(arr));
    window.__JUCE__.backend.addEventListener("scopeUpdate", (arr) => drawScope(arr));
    window.__JUCE__.backend.addEventListener("loopUpdate", (data) => drawLoop(data));
  } else {
    console.error("Native backend unavailable — spectrum/scope/loop frames will not arrive.");
  }
}

// Single resize handler: re-fit both canvas backing stores, then redraw the last
// frame (preserves the visible image across an editor resize).
function rewireResize() {
  window.addEventListener("resize", () => {
    if (specCanvas) specCanvas.resize();
    if (scopeCanvas) scopeCanvas.resize();
    if (lastSpectrum) drawSpectrum(lastSpectrum);
    if (lastScope) drawScope(lastScope);
  });
}

// Pull the host sample rate for the spectrum frequency-axis labels.
async function fetchSampleRate() {
  try {
    const sr = await Juce.getNativeFunction("getSampleRate")();
    if (sr > 0) nyquistHz = sr / 2;
    if (lastSpectrum) drawSpectrum(lastSpectrum);   // relabel once we know the real rate
  } catch (e) { /* keep the default */ }
}

// ── Loop / flow diagram (Phase 3.3) ─────────────────────────────────────────
// Inline SVG (no canvas → sidesteps the replaced-element gotcha). Two skins,
// toggled on resonatorType:
//   • String: a pulse circulates the KS delay loop; its opacity = loopEnergy, so
//     it visibly dims each pass as the note decays (the headline mental-model viz).
//   • Modal: 8 vertical stems, x = log-map(freq), height = amp — the live mode bank.
// Driven by the C++ loopUpdate event { energy, stemFreqs[8], stemAmps[8] } at 30 Hz.
const STEM_COUNT = 8;
const STEM_BASE_Y = 150, STEM_TOP_Y = 64;      // baseline + tallest-stem top (SVG units)
const STEM_X_L = 170, STEM_X_R = 284;          // interior x range inside the RESONATOR box
const STEM_F_MIN = 40, STEM_F_MAX = 12000;     // log-frequency axis bounds
const stemLogRange = Math.log(STEM_F_MAX / STEM_F_MIN);
const SVG_NS = "http://www.w3.org/2000/svg";

const stemEls = [];   // <line> per mode
const stemCaps = [];  // <circle> lollipop cap per mode
let lastLoop = null;

function stemX(f) {
  if (f <= STEM_F_MIN) return STEM_X_L;
  const t = Math.min(1, Math.log(f / STEM_F_MIN) / stemLogRange);
  return STEM_X_L + t * (STEM_X_R - STEM_X_L);
}

// Build the 8 stem elements once (cleaner than 16 hand-written SVG nodes).
function buildStems() {
  const g = document.getElementById("modalStems");
  if (!g) return;
  for (let k = 0; k < STEM_COUNT; k++) {
    const line = document.createElementNS(SVG_NS, "line");
    line.setAttribute("class", "stem");
    line.setAttribute("x1", STEM_X_L); line.setAttribute("x2", STEM_X_L);
    line.setAttribute("y1", STEM_BASE_Y); line.setAttribute("y2", STEM_BASE_Y);
    line.style.opacity = "0";
    g.appendChild(line);
    stemEls[k] = line;

    const cap = document.createElementNS(SVG_NS, "circle");
    cap.setAttribute("class", "stem-cap");
    cap.setAttribute("r", "3");
    cap.setAttribute("cx", STEM_X_L); cap.setAttribute("cy", STEM_BASE_Y);
    cap.style.opacity = "0";
    g.appendChild(cap);
    stemCaps[k] = cap;
  }
}

// String skin — drive the circulating pulse's opacity/size from loopEnergy.
function drawStringEnergy(energy) {
  const e = Math.max(0, Math.min(1, energy || 0));
  const pulse = document.getElementById("loopPulse");
  if (pulse) {
    pulse.style.opacity = String(0.12 + 0.88 * e);
    pulse.setAttribute("r", String(3 + 4 * e));
  }
  const ring = document.getElementById("loopRing");
  if (ring) ring.style.opacity = String(0.35 + 0.5 * e);
}

// Modal skin — position/scale the 8 stems from the live (freq, amp) bank.
function drawStems(freqs, amps) {
  const maxH = STEM_BASE_Y - STEM_TOP_Y;
  for (let k = 0; k < STEM_COUNT; k++) {
    const line = stemEls[k], cap = stemCaps[k];
    if (!line) continue;
    const f = (freqs && freqs[k]) || 0;
    const a = Math.max(0, Math.min(1, (amps && amps[k]) || 0));
    if (f <= 0 || a <= 0.0008) {            // silent / unset → hide
      line.style.opacity = "0";
      if (cap) cap.style.opacity = "0";
      continue;
    }
    const x = stemX(f);
    const top = STEM_BASE_Y - a * maxH;
    line.setAttribute("x1", x); line.setAttribute("x2", x);
    line.setAttribute("y1", STEM_BASE_Y); line.setAttribute("y2", top);
    line.style.opacity = "1";
    const hue = 90 - a * 40;                 // green → yellow-green with amplitude
    line.style.stroke = `hsl(${hue}, 55%, ${38 + a * 22}%)`;
    if (cap) {
      cap.setAttribute("cx", x); cap.setAttribute("cy", top);
      cap.style.opacity = "1";
      cap.style.fill = `hsl(${hue}, 60%, ${30 + a * 20}%)`;
    }
  }
}

// One loopUpdate frame → drive whichever skin is currently shown.
function drawLoop(data) {
  if (!data) return;
  lastLoop = data;
  const modal = comboState.resonatorType && comboState.resonatorType.getChoiceIndex() === 1;
  if (modal) drawStems(data.stemFreqs, data.stemAmps);
  else drawStringEnergy(data.energy);
}

// Toggle the diagram skin (String loop ↔ Modal stems) from resonatorType.
function applyDiagramSkin() {
  const modal = comboState.resonatorType && comboState.resonatorType.getChoiceIndex() === 1;
  const ss = document.getElementById("stringSkin");
  const ms = document.getElementById("modalSkin");
  if (ss) ss.style.display = modal ? "none" : "";
  if (ms) ms.style.display = modal ? "" : "none";
  // NOT a setLabel call, and deliberately so. This line mirrors the resonatorType
  // AudioParameterChoice value (String / Modal), which the Engine combo one row
  // below displays in English under D-01 — translating it here would make the
  // page and the host automation lane disagree about what the plugin is SET to.
  // Both faces are I18N_EXEMPT with that reason, which is what keeps assertion 12
  // green over this raw textContent write. Same rule as the EXCITE node's
  // "pluck·strike·bow" sub-caption; the node CAPTIONS above them are keyed,
  // because a caption names the box and these name what is selected inside it.
  const cap = document.getElementById("resonatorMode");
  if (cap) cap.textContent = modal ? "modal · stems" : "string · loop";
  if (lastLoop) drawLoop(lastLoop);   // repaint the new skin from the last frame
}

// ── Settings popover (v1.2.0) ───────────────────────────────────────────────
// The gear panel holding the language selector and the hover-help switch. All
// state lives in this closure, so nothing here can join a TDZ chain.
//
// Styled in this plugin's own aged-paper field-guide vocabulary in
// css/styles.css: the panel wears the .group plate's paper fill and dotted rule,
// the gear wears the .tip-toggle circle it replaces (same 22px, same lit green
// when open), and the selector wears the preset-bar button's border, radius and
// paper fill at panel scale. It is not a widget pasted in unchanged from another
// plugin.
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

// ── Hover-help switch ───────────────────────────────────────────────────────
// It was the "?" chip at the end of the preset bar through v1.1.0 and it lives
// in the settings popover now. Its storage is unchanged: localStorage under
// "opms.tipsEnabled", so a preference set before this version survives the move.
// It is a BROWSER-side preference, not session state — this plugin has no
// tooltips bridge and never had one, and adding C++ state for it would be a new
// persistence surface rather than a localization change (O-ReverseDelay D13).
let tipsEnabled = true;                 // shipped default; localStorage wins at boot
let hideTooltip = () => {};             // published by setupTooltips (used by the switch)

function applyTipsEnabled(on) {
  tipsEnabled = !!on;
  if (!tipsEnabled) hideTooltip();

  const btn = document.getElementById("help-toggle");
  if (!btn) return;
  btn.setAttribute("aria-pressed", tipsEnabled ? "true" : "false");
  // Two calls behind an if/else, never a ternary in the setLabel argument:
  // check-i18n assertion 13 rejects a conditional anywhere inside the call, and
  // it is right to — a reviewer cannot tell a message-selection ternary from a
  // plural one by reading it.
  if (tipsEnabled) setLabel(btn, "ui.on");
  else             setLabel(btn, "ui.off");
}

function setupTipsToggle() {
  const btn = document.getElementById("help-toggle");
  if (!btn) { console.error("Missing help-toggle element"); return; }

  let stored = null;
  try { stored = localStorage.getItem("opms.tipsEnabled"); } catch (e) { stored = null; }
  applyTipsEnabled(stored !== "false");

  btn.addEventListener("click", () => {
    applyTipsEnabled(!tipsEnabled);
    try { localStorage.setItem("opms.tipsEnabled", String(tipsEnabled)); } catch (e) { /* private mode */ }
  });
}

// ── Tooltips (UI-06) ─────────────────────────────────────────────────────────
// Single floating #tooltip div. The copy is no longer looked up here: applyI18n
// has already written it onto each anchor as data-tip-title + data-tip, in the
// current language, and it rewrites both on every language change. This function
// only positions and shows what the anchor carries. Avoids native title= (slow,
// unstyled OS tooltips, and untranslated — contract §4 deletes all five this
// page carried).
function setupTooltips() {
  const tip = document.getElementById("tooltip");
  if (!tip) return;
  let active = null;

  // Built with createElement + textContent, not innerHTML. The tip text is
  // table-sourced and localized now rather than a fixed literal, and localized
  // copy must never reach a markup path. v1.1.0's tip bodies carried strong/em
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
      t.className = "tip-title";
      t.textContent = title;
      tip.appendChild(t);
    }
    if (body) tip.appendChild(document.createTextNode(body));
    tip.classList.add("show");
    tip.setAttribute("aria-hidden", "false");
    position(x, y);
  };
  const position = (x, y) => {
    const r = tip.getBoundingClientRect();
    let nx = x + 14, ny = y + 16;
    if (nx + r.width > window.innerWidth - 8) nx = x - r.width - 14;
    if (ny + r.height > window.innerHeight - 8) ny = y - r.height - 12;
    tip.style.left = `${Math.max(8, nx)}px`;
    tip.style.top = `${Math.max(8, ny)}px`;
  };
  const hide = () => { tip.classList.remove("show"); tip.setAttribute("aria-hidden", "true"); active = null; };
  hideTooltip = hide;

  // DELEGATED on the document rather than attached per element. No anchor
  // carries data-tip until applyI18n has run, so the v1.1.0
  // querySelectorAll("[data-tip]") at setup time would bind NOTHING at all.
  // Delegation has no ordering to get wrong. pointerover/pointerout and
  // focusin/focusout are used because — unlike pointerenter/pointerleave and
  // focus/blur — they bubble. closest() also gets the nesting right for free:
  // the four diagram boxes are siblings, but a control cell's focusable knob is
  // a CHILD of the anchor, and the innermost anchor wins without a
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
    // Ignore a move between two descendants of the SAME anchor: pointerout
    // fires on every child boundary and would flicker the tip off and on.
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

// ── On-screen keyboard (play without external MIDI) ──────────────────────────
const KB_LOW = 48, KB_HIGH = 72;              // C3 … C5 inclusive
const BLACK_OFFSETS = new Set([1, 3, 6, 8, 10]);
const QWERTY = { a: 60, w: 61, s: 62, e: 63, d: 64, f: 65, t: 66, g: 67, y: 68, h: 69, u: 70, j: 71, k: 72 };

let uiMidiFn = null;
const keyEls = {};
const heldNotes = new Set();

function noteOn(note, vel = 0.8) {
  if (note < 0 || note > 127 || heldNotes.has(note)) return;
  heldNotes.add(note);
  if (uiMidiFn) uiMidiFn(note, true, vel);
  const el = keyEls[note];
  if (el) el.classList.add("active");
}
function noteOff(note) {
  if (!heldNotes.has(note)) return;
  heldNotes.delete(note);
  if (uiMidiFn) uiMidiFn(note, false, 0);
  const el = keyEls[note];
  if (el) el.classList.remove("active");
}

function setupKeyboard() {
  const kb = document.getElementById("keyboard");
  if (!kb) return;
  try { uiMidiFn = Juce.getNativeFunction("uiMidi"); } catch (e) { uiMidiFn = null; }

  const qFor = (n) => Object.keys(QWERTY).find((k) => QWERTY[k] === n);
  const blacks = [];
  for (let n = KB_LOW; n <= KB_HIGH; n++) {
    const isBlack = BLACK_OFFSETS.has(((n % 12) + 12) % 12);
    const el = document.createElement("div");
    el.className = "key " + (isBlack ? "key-black" : "key-white");
    el.dataset.note = String(n);
    const q = qFor(n);
    if (q) {
      const lab = document.createElement("span");
      lab.className = "key-label";
      lab.textContent = q.toUpperCase();
      el.appendChild(lab);
    }
    keyEls[n] = el;
    if (isBlack) blacks.push(el);
    else kb.appendChild(el);          // white keys flow in the flex row
  }
  blacks.forEach((el) => kb.appendChild(el));   // black keys overlay (absolute)

  const positionBlacks = () => {
    for (const el of blacks) {
      const n = +el.dataset.note;
      const leftWhite = keyEls[n - 1];
      if (!leftWhite) continue;
      el.style.left = (leftWhite.offsetLeft + leftWhite.offsetWidth - el.offsetWidth / 2) + "px";
    }
  };
  requestAnimationFrame(positionBlacks);
  window.addEventListener("resize", positionBlacks);

  // Mouse / touch — monophonic with glide while the button is held.
  let pointerNote = null;
  const noteAt = (target) => {
    const k = target.closest ? target.closest(".key") : null;
    return k ? +k.dataset.note : null;
  };
  kb.addEventListener("pointerdown", (e) => {
    const n = noteAt(e.target);
    if (n == null) return;
    pointerNote = n;
    noteOn(n);
    e.preventDefault();
  });
  kb.addEventListener("pointerover", (e) => {
    if (pointerNote == null) return;
    const n = noteAt(e.target);
    if (n != null && n !== pointerNote) { noteOff(pointerNote); noteOn(n); pointerNote = n; }
  });
  window.addEventListener("pointerup", () => {
    if (pointerNote != null) { noteOff(pointerNote); pointerNote = null; }
  });

  // Computer keyboard — polyphonic, auto-repeat suppressed.
  window.addEventListener("keydown", (e) => {
    if (e.repeat || e.metaKey || e.ctrlKey || e.altKey) return;
    const n = QWERTY[e.key.toLowerCase()];
    if (n == null) return;
    noteOn(n);
    e.preventDefault();
  });
  window.addEventListener("keyup", (e) => {
    const n = QWERTY[e.key.toLowerCase()];
    if (n != null) noteOff(n);
  });
}

// ── Boot ────────────────────────────────────────────────────────────────────
function boot() {
  specCanvas = makeCanvas("spectrumCanvas");
  scopeCanvas = makeCanvas("scopeCanvas");
  buildStems();

  KNOB_IDS.forEach(bindKnob);
  COMBO_IDS.forEach(bindCombo);

  // Grey-out tracks both engine selectors; the diagram skin tracks resonatorType.
  // Both re-apply after a preset load (see setupPresetManager onPresetChanged).
  ["resonatorType", "excitationType"].forEach((id) => {
    const st = comboState[id];
    if (st) {
      st.valueChangedEvent.addListener(applyEngineGating);
      st.propertiesChangedEvent.addListener(applyEngineGating);
    }
  });
  const resSt = comboState.resonatorType;
  if (resSt) {
    resSt.valueChangedEvent.addListener(applyDiagramSkin);
    resSt.propertiesChangedEvent.addListener(applyDiagramSkin);
  }
  applyEngineGating();
  applyDiagramSkin();

  setupPresetManager();
  setupKeyboard();

  // The popover and the language sweep go here, EACH IN ITS OWN try/catch, and
  // AFTER every builder that creates cells — buildStems() and setupKeyboard()
  // above — so the sweep sees the finished DOM. applyI18n is what writes
  // data-tip onto every anchor and what names the fourteen knobs; setupTooltips
  // below reads that attribute, but delegation means the order between the two
  // is free. setupTipsToggle() runs LAST of the three: it calls setLabel, which
  // needs the table loaded, and it captures hideTooltip, which setupTooltips
  // publishes. A translation-table typo must not be allowed to take the
  // seventeen parameter bindings, the two canvases and the keyboard down with it
  // — that is the TDZ failure this repo has already paid for once.
  try { setupSettingsPopover(); } catch (e) { console.error("settings popover init failed:", e); }
  try { initI18n(); }             catch (e) { console.error("i18n init failed:", e); }

  setupTooltips();
  try { setupTipsToggle(); }      catch (e) { console.error("hover-help switch init failed:", e); }

  setupVizEvents();
  rewireResize();
  fetchSampleRate();

  // Initial empty frames so the panels aren't blank before the first tick.
  drawSpectrum(new Array(256).fill(-100));
  drawScope(new Array(128).fill(0));
  drawStringEnergy(0);
}

if (document.readyState === "loading")
  document.addEventListener("DOMContentLoaded", boot);
else
  boot();
