/*
   This file is part of O-simpleSubtractive, an Ouaricon Audio plugin.
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
// O-simpleSubtractive — WebView UI controller
// Binds all 20 APVTS params two-way (16 knobs + 4 combos), draws the headline
// filter-curve-over-spectrum, the oscilloscope, and the dual-ADSR display, and
// runs the pedagogical layer (signal-path diagram, tooltips, preset tour).
//
// NOTE: getSliderState / getComboBoxState / getNativeFunction live on the `Juce`
// ES-module namespace. window.__JUCE__ has the low-level backend, used here ONLY
// for backend.addEventListener on the four C++-pushed viz events.
// ============================================================================

import * as Juce from "./juce/index.js";

// ════════════════════════════════════════════════════════════════════════════
// INTERFACE LANGUAGE (v1.3.0) — canon v2, verbatim from scripts/i18n-canon.js.
//
// It sits HERE, directly under the import and above every other declaration in
// this module, because `uiLanguage` is a module-level `let`: anything above it
// that reached applyLabel() would touch it inside its temporal dead zone and
// throw out of module evaluation, taking the twenty parameter bindings, the
// four canvases and the keyboard with it (pattern_module_toplevel_init_tdz).
// This file DOES have eager top-level work below — SLIDER_IDS, FORMAT, the
// choice-name tables — so the ordering is load-bearing here, not merely
// defensive. `node scripts/boot-all-uis.js` is the ONLY gate in the repo that
// sees this class of failure.
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


// ── Parameter inventory (must match OSimpleSubtractive::ParamIDs exactly) ───
const SLIDER_IDS = [
  "subLevel", "noiseLevel",
  "cutoff", "resonance", "filterEnvAmount", "keyTrack",
  "filterAttack", "filterDecay", "filterSustain", "filterRelease",
  "ampAttack", "ampDecay", "ampSustain", "ampRelease",
  "glide", "outputLevel",
];
const COMBO_IDS = ["oscWave", "filterType", "filterSlope", "voiceMode"];

// ── Display formatters (keyed by param id; receive the *scaled* value) ──────
const fmtMs = (s) => (s < 1 ? `${Math.round(s * 1000)} ms` : `${s.toFixed(2)} s`);
const fmtPct = (v) => `${Math.round(v * 100)}%`;
const fmtHz = (v) => (v >= 1000 ? `${(v / 1000).toFixed(2)} kHz` : `${Math.round(v)} Hz`);
const fmtSignedPct = (v) => `${v >= 0 ? "+" : ""}${Math.round(v * 100)}%`;
const FORMAT = {
  subLevel: fmtPct, noiseLevel: fmtPct,
  cutoff: fmtHz, resonance: fmtPct, filterEnvAmount: fmtSignedPct, keyTrack: fmtPct,
  filterAttack: fmtMs, filterDecay: fmtMs, filterSustain: fmtPct, filterRelease: fmtMs,
  ampAttack: fmtMs, ampDecay: fmtMs, ampSustain: fmtPct, ampRelease: fmtMs,
  glide: fmtMs, outputLevel: (v) => `${v.toFixed(1)} dB`,
};

// ── Choice labels (for the signal-path diagram) ────────────────────────────
const WAVE_NAMES = ["Saw", "Square", "Triangle", "Sine"];
const TYPE_SHORT = ["LP", "HP", "BP", "Notch"];
const TYPE_LONG = ["low-pass", "high-pass", "band-pass", "notch"];
const SLOPE_SHORT = ["6 dB", "12 dB", "24 dB"];
const SLOPE_DBOCT = ["6", "12", "24"];

// ── Tooltip copy ───────────────────────────────────────────────────────────────
// MOVED to js/i18n.js at v1.3.0. Through v1.2.5 the copy lived in a `TIPS`
// object here and each anchor carried the KEY in its own data-tip attribute.
// applyI18n now WRITES data-tip (the body) and data-tip-title on every anchor
// named by TIP_BINDINGS, so the renderer below reads the attributes rather than
// a table — one code path, and no way for a tip to be stranded in the previous
// language after the selector fires.
//
// The bodies lost their strong/em emphasis tags on the way. The WORDS are
// unchanged, and were compared back to v1.2.5 with entities decoded rather than
// re-typed. check-i18n assertion 9 forbids an angle bracket in an i18n.js
// string literal, and it is right to: the renderer writes textContent now, so a
// tag would render as literal characters rather than as emphasis.

// ── Knob geometry ──────────────────────────────────────────────────────────
const KNOB_MIN_DEG = -135;   // 0.0 normalised
const KNOB_MAX_DEG = 135;    // 1.0 normalised
const DRAG_TRAVEL_PX = 220;  // vertical px for a full 0..1 sweep

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

  let dragging = false;
  let startY = 0;
  let startNorm = 0;

  const onMove = (e) => {
    if (!dragging) return;
    const dy = startY - e.clientY;
    let n = startNorm + dy / DRAG_TRAVEL_PX;
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

  // Mouse wheel fine adjust.
  knob.addEventListener("wheel", (e) => {
    nudge(st, e.deltaY < 0 ? 0.02 : -0.02, id);
    e.preventDefault();
  }, { passive: false });
}

// ── Combo box binding (oscWave / filterType / filterSlope / voiceMode) ──────
// ComboBox choices may not be present on first load — listen to BOTH
// propertiesChangedEvent and valueChangedEvent or the <select> renders empty.
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
    updateDiagram();
  };
  st.propertiesChangedEvent.addListener(refresh);
  st.valueChangedEvent.addListener(refresh);
  refresh();

  sel.addEventListener("change", () => st.setChoiceIndex(sel.selectedIndex));
}

// ── Canvases (DPR-aware backing store — canvas is a CSS replaced element) ────
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
  resize();   // single window 'resize' handler lives in rewireResize()
  return { canvas, ctx, resize };
}

let headline = null, scope = null, filterAdsr = null, ampAdsr = null;
let lastSpectrum = null, lastCurve = null, lastScope = null;
let lastEnv = { filterEnv: 0, ampEnv: 0 };

// Shared dB window. Curve peaks exceed 0 dB at resonance/self-osc, so DB_MAX is
// well above 0 to keep the peak on-canvas (Risk R2).
const DB_MIN = -90, DB_MAX = 18;
let nyquistHz = 22050;                  // replaced by getSampleRate() at boot
const FREQ_TICKS = [100, 1000, 10000];
const fmtTickHz = (f) => (f >= 1000 ? `${f / 1000}k` : `${f}`);

// ── Headline: filter curve OVER the live spectrum (UI-01) ──────────────────
// Both arrays are 256 bins, both log-f 20 Hz → Nyquist with identical axis math
// (SubVizAnalyzer.h) → bin index is the SAME frequency in both. Plot at the same
// x = (i/(n-1))*w, share the same dB→y window. No remapping.
function drawHeadline() {
  if (!headline) return;
  const { canvas, ctx } = headline;
  const w = canvas.clientWidth, h = canvas.clientHeight;
  ctx.clearRect(0, 0, w, h);

  const yFor = (db) => {
    const n = (db - DB_MIN) / (DB_MAX - DB_MIN);
    return h - Math.max(0, Math.min(1, n)) * h;
  };

  // dB grid lines
  ctx.strokeStyle = "rgba(139,115,85,0.16)";
  ctx.lineWidth = 1;
  for (let g = 1; g < 5; g++) {
    const y = (h * g) / 5;
    ctx.beginPath(); ctx.moveTo(0, y); ctx.lineTo(w, y); ctx.stroke();
  }

  // 1) spectrum bars (what the filter lets through)
  if (lastSpectrum && lastSpectrum.length) {
    const n = lastSpectrum.length;          // 256, dB in ~[-100, 0]
    const bw = w / (n - 1);
    const y0 = yFor(DB_MIN);
    for (let i = 0; i < n; i++) {
      const db = lastSpectrum[i];
      const y = yFor(db);
      const barH = Math.max(0, y0 - y);
      if (barH <= 0) continue;
      const norm = Math.max(0, Math.min(1, (db + 100) / 100));
      const hue = 90 - norm * 40;           // green → yellow-green with intensity
      ctx.fillStyle = `hsl(${hue}, 55%, ${32 + norm * 28}%)`;
      ctx.fillRect(i * bw, y, Math.max(1, bw - 0.5), barH);
    }
  }

  // 0 dB reference (unity gain) — faint amber dashed
  const yUnity = yFor(0);
  ctx.strokeStyle = "rgba(232,176,74,0.25)";
  ctx.setLineDash([2, 4]); ctx.lineWidth = 1;
  ctx.beginPath(); ctx.moveTo(0, yUnity); ctx.lineTo(w, yUnity); ctx.stroke();
  ctx.setLineDash([]);

  // 2) filter response curve, stroked on top
  if (lastCurve && lastCurve.length) {
    const n = lastCurve.length;             // 256, dB (can exceed 0 at resonance)
    ctx.beginPath();
    for (let i = 0; i < n; i++) {
      const x = (i / (n - 1)) * w;
      const y = yFor(lastCurve[i]);
      if (i === 0) ctx.moveTo(x, y); else ctx.lineTo(x, y);
    }
    ctx.strokeStyle = "#e8b04a";
    ctx.lineWidth = 2;
    ctx.lineJoin = "round";
    ctx.stroke();
  }

  // Frequency-axis ticks (log scale; same map the bins use).
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

// ── Oscilloscope (UI-04) ───────────────────────────────────────────────────
function drawScope(arr) {
  lastScope = arr;
  if (!scope) return;
  const { canvas, ctx } = scope;
  const w = canvas.clientWidth, h = canvas.clientHeight;
  ctx.clearRect(0, 0, w, h);

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

// ── Dual-ADSR display (UI-02) ──────────────────────────────────────────────
// Two independent canvases (filter env → cutoff, amp env → level). Each shape
// is computed from its 4 ADSR scaled values; the dashed marker shows the LIVE
// env output (0..1) on the y-axis — a vertical-only playhead (Open Question 1).
const ADSR_COLORS = {
  filter: { fill: "rgba(232,176,74,0.16)", line: "#e8b04a", head: "#f3cd86" },
  amp:    { fill: "rgba(158,196,111,0.16)", line: "#9ec46f", head: "#c4e0a0" },
};

function envShapePoints(a, d, s, r, w, h) {
  const pad = 4;
  const innerW = w - pad * 2, innerH = h - pad * 2;
  const f = (t) => Math.sqrt(Math.max(0, t) / 5);   // compress 0..5 s → 0..1
  const aW = f(a), dW = f(d), rW = f(r), sW = 0.7;   // fixed sustain plateau weight
  const total = aW + dW + sW + rW || 1;
  const sx = innerW / total;
  const yFor = (lvl) => pad + innerH * (1 - Math.max(0, Math.min(1, lvl)));
  const pts = [];
  let x = pad;
  pts.push([x, yFor(0)]);
  x += aW * sx; pts.push([x, yFor(1)]);
  x += dW * sx; pts.push([x, yFor(s)]);
  x += sW * sx; pts.push([x, yFor(s)]);
  x += rW * sx; pts.push([x, yFor(0)]);
  return pts;
}

function drawOneAdsr(cv, prefix, live, color) {
  if (!cv) return;
  const { canvas, ctx } = cv;
  const w = canvas.clientWidth, h = canvas.clientHeight;
  ctx.clearRect(0, 0, w, h);

  const get = (id) => (sliderState[id] ? sliderState[id].getScaledValue() : 0);
  const a = get(`${prefix}Attack`), d = get(`${prefix}Decay`);
  const s = get(`${prefix}Sustain`), r = get(`${prefix}Release`);
  const pts = envShapePoints(a, d, s, r, w, h);
  const base = h - 4;

  // filled body
  ctx.beginPath();
  ctx.moveTo(pts[0][0], base);
  for (const p of pts) ctx.lineTo(p[0], p[1]);
  ctx.lineTo(pts[pts.length - 1][0], base);
  ctx.closePath();
  ctx.fillStyle = color.fill;
  ctx.fill();

  // outline
  ctx.beginPath();
  pts.forEach((p, i) => (i ? ctx.lineTo(p[0], p[1]) : ctx.moveTo(p[0], p[1])));
  ctx.strokeStyle = color.line;
  ctx.lineWidth = 2;
  ctx.lineJoin = "round";
  ctx.stroke();

  // live playhead: dashed horizontal line at the env's current output + a dot
  const pad = 4, innerH = h - pad * 2;
  const y = pad + innerH * (1 - Math.max(0, Math.min(1, live)));
  ctx.strokeStyle = color.head;
  ctx.lineWidth = 1;
  ctx.setLineDash([3, 3]);
  ctx.beginPath(); ctx.moveTo(0, y); ctx.lineTo(w, y); ctx.stroke();
  ctx.setLineDash([]);
  ctx.fillStyle = color.head;
  ctx.beginPath(); ctx.arc(w - 6, y, 3.2, 0, Math.PI * 2); ctx.fill();
}

function drawDualAdsr() {
  drawOneAdsr(filterAdsr, "filter", lastEnv.filterEnv, ADSR_COLORS.filter);
  drawOneAdsr(ampAdsr, "amp", lastEnv.ampEnv, ADSR_COLORS.amp);
}

// ── Signal-path diagram (UI-03) ────────────────────────────────────────────
function updateDiagram() {
  // OSC node reflects the wave choice.
  const oscEl = document.getElementById("oscModeText");
  if (oscEl && comboState.oscWave) {
    const i = comboState.oscWave.getChoiceIndex();
    oscEl.textContent = WAVE_NAMES[i] || "";
  }

  // FILTER node reflects slope + type.
  const tSt = comboState.filterType, slSt = comboState.filterSlope;
  const fEl = document.getElementById("filterModeText");
  if (fEl && tSt && slSt) {
    const ti = tSt.getChoiceIndex(), si = slSt.getChoiceIndex();
    fEl.textContent = `${SLOPE_SHORT[si] || ""} ${TYPE_SHORT[ti] || ""}`.trim();
  }

  // Filter-env route arrow ∝ |filterEnvAmount|; colour the bipolar sign.
  const envSt = sliderState.filterEnvAmount;
  const arrow = document.getElementById("arrowFiltEnv");
  const head = document.getElementById("arrowFiltEnvHead");
  if (envSt) {
    const v = envSt.getScaledValue();           // -1..1
    const mag = Math.min(1, Math.abs(v));
    const col = v >= 0 ? "#e8b04a" : "#7fa6d4";  // + opens (amber) / − closes (cool blue)
    if (arrow) { arrow.style.strokeWidth = (1 + mag * 4).toFixed(2); arrow.style.opacity = (0.2 + mag * 0.8).toFixed(2); arrow.style.stroke = col; }
    if (head) { head.style.opacity = (0.2 + mag * 0.8).toFixed(2); head.style.fill = col; }
  }

  // VCA brightens with the live amp env; FILTER brightens with the live filter env.
  const vca = document.getElementById("nodeVca");
  if (vca) vca.style.filter = `brightness(${(0.85 + lastEnv.ampEnv * 0.6).toFixed(2)})`;
  const filt = document.getElementById("nodeFilter");
  if (filt) filt.style.filter = `brightness(${(0.9 + lastEnv.filterEnv * 0.5).toFixed(2)})`;

  // Readout: cutoff + resonance.
  const cSt = sliderState.cutoff, rSt = sliderState.resonance;
  const rc = document.getElementById("readCutoff");
  if (rc && cSt) rc.textContent = fmtHz(cSt.getScaledValue());
  const rr = document.getElementById("readRes");
  if (rr && rSt) rr.textContent = rSt.getScaledValue().toFixed(2);

  const meta = document.getElementById("routeMeta");
  if (meta && tSt && slSt) {
    meta.textContent = `${TYPE_LONG[tSt.getChoiceIndex()] || ""} · ${SLOPE_DBOCT[slSt.getChoiceIndex()] || ""} dB/oct`;
  }
}

// ── Viz events (the four C++ pushes arrive on window.__JUCE__.backend) ──────
function setupVizEvents() {
  if (window.__JUCE__ && window.__JUCE__.backend) {
    // curve before/with spectrum each frame — store both, redraw the headline.
    window.__JUCE__.backend.addEventListener("filterCurveUpdate", (arr) => { lastCurve = arr; drawHeadline(); });
    window.__JUCE__.backend.addEventListener("spectrumUpdate", (arr) => { lastSpectrum = arr; drawHeadline(); });
    window.__JUCE__.backend.addEventListener("scopeUpdate", (arr) => drawScope(arr));
    window.__JUCE__.backend.addEventListener("envUpdate", (obj) => {
      if (obj) {
        lastEnv.filterEnv = Number(obj.filterEnv) || 0;
        lastEnv.ampEnv = Number(obj.ampEnv) || 0;
      }
      drawDualAdsr();
      updateDiagram();   // VCA/FILTER pulse with the live envelopes
    });
  } else {
    console.error("window.__JUCE__.backend unavailable — viz events will not arrive.");
  }
}

// Single resize handler: re-fit every canvas backing store, then redraw.
function rewireResize() {
  window.addEventListener("resize", () => {
    if (headline) headline.resize();
    if (scope) scope.resize();
    if (filterAdsr) filterAdsr.resize();
    if (ampAdsr) ampAdsr.resize();
    drawHeadline();
    drawDualAdsr();
    if (lastScope) drawScope(lastScope);
  });
}

// ── Tooltips ────────────────────────────────────────────────────────────────
function setupTooltips() {
  const tip = document.getElementById("tooltip");
  if (!tip) return;
  let active = null;

  // The copy is read from the anchor's OWN attributes, which applyI18n rewrote
  // in the current language. v1.2.5 looked it up in a TIPS object keyed by the
  // anchor's data-tip; that table is gone, and with it the second code path
  // that would have gone stale on a language switch.
  //
  // Built with createElement + textContent, not innerHTML. The tip text is now
  // table-sourced rather than a fixed literal, and localized copy must never
  // reach a markup path.
  const show = (el, x, y) => {
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

  // DELEGATED on the document rather than attached per element. No anchor
  // carries data-tip until applyI18n has run, so a querySelectorAll at setup
  // time would bind nothing at all. Delegation has no ordering to get wrong.
  // pointerover/pointerout and focusin/focusout are used because — unlike
  // pointerenter/pointerleave and focus/blur — they bubble.
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

// ── Concept-preset tour (UI-07) ─────────────────────────────────────────────
// Each button calls the C++ applyFactoryPreset native fn with its label; the
// processor writes the whole APVTS via setValueNotifyingHost and the relays /
// attachments sync every knob/combo back to the page (no DOM poking). The C++
// snapshot bodies land in Stage 4 (FUNC-06) — Stage 3 ships the live bridge.
// Through v1.2.5 a LESSONS table here held each caption and applyLesson wrote
// it with a raw `cap.textContent = LESSONS[name]`. Both the copy and the write
// have moved: the eight captions are authored table entries in js/i18n.js, and
// the write below goes through setLabel, which makes the caption a [data-i18n]
// element from that moment on so the language sweep owns it. Written raw it
// would be stranded in the language it was picked in the instant the selector
// fires — and it is the one string on this page that is chosen by a click.
//
// What is left here is a dispatch from the C++ preset name (which is also the
// button's data-preset, and is never localized) to a WRITER that names its
// caption key as a plain string literal.
//
// A map of name -> key with `setLabel(cap, KEYS[name] || "label.captionDefault")`
// reads more naturally and is what this was first written as. check-i18n
// assertion 13 rejects it twice over, correctly: a computed key cannot be
// checked against the table, and the `||` is the conditional-inside-a-localized-
// string shape contract §6 forbids. Nine call sites, nine literals, and
// assertion 15 can see that all nine keys are live.
const LESSON_CAPTION_WRITERS = {
  "Saw Sweep":        (el) => setLabel(el, "label.captionSawSweep"),
  "Pluck":            (el) => setLabel(el, "label.captionPluck"),
  "Brass Stab":       (el) => setLabel(el, "label.captionBrass"),
  "Sweep Pad":        (el) => setLabel(el, "label.captionSweep"),
  "Acid Bass":        (el) => setLabel(el, "label.captionAcid"),
  "Square Bass":      (el) => setLabel(el, "label.captionSquareBass"),
  "Noise Wind":       (el) => setLabel(el, "label.captionNoiseWind"),
  "Self-Oscillation": (el) => setLabel(el, "label.captionSelfOsc"),
};

let applyPresetFn = null;

async function applyLesson(name) {
  if (applyPresetFn) {
    try { await applyPresetFn(name); }
    catch (e) { console.error("[O-simpleSubtractive] applyFactoryPreset failed:", e); }
  }
  // An unknown name falls back to the resting caption rather than to "", so a
  // C++/JS name drift shows as the default text instead of a blank row.
  const cap = document.getElementById("tourCaption");
  if (cap) {
    const write = LESSON_CAPTION_WRITERS[name];
    if (write) write(cap);
    else setLabel(cap, "label.captionDefault");
  }
  document.querySelectorAll(".tour-btn").forEach((b) =>
    b.classList.toggle("active", b.getAttribute("data-preset") === name));
}

function setupPresets() {
  try { applyPresetFn = Juce.getNativeFunction("applyFactoryPreset"); }
  catch (e) { applyPresetFn = null; }
  document.querySelectorAll(".tour-btn").forEach((btn) => {
    btn.addEventListener("click", () => applyLesson(btn.getAttribute("data-preset")));
  });
}

// ── On-screen keyboard (play without external MIDI) ──────────────────────────
// Notes are injected via the C++ `uiMidi` native function (queued through a
// MidiMessageCollector, merged into processBlock — identical path to host MIDI).
const KB_LOW = 48, KB_HIGH = 72;              // C3 … C5 inclusive
const BLACK_OFFSETS = new Set([1, 3, 6, 8, 10]);
const QWERTY = { a: 60, w: 61, s: 62, e: 63, d: 64, f: 65, t: 66, g: 67, y: 68, h: 69, u: 70, j: 71, k: 72 };

let uiMidiFn = null;
const keyEls = {};                  // midi note -> key element
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

  // Computer keyboard — polyphonic, auto-repeat suppressed. Skip while a form
  // control (e.g. a filter/voice <select>) is focused so type-ahead and option
  // selection aren't hijacked into note triggers.
  const typingInControl = () =>
    ["SELECT", "INPUT", "TEXTAREA"].includes(document.activeElement?.tagName);
  window.addEventListener("keydown", (e) => {
    if (e.repeat || e.metaKey || e.ctrlKey || e.altKey) return;
    if (typingInControl()) return;
    const n = QWERTY[e.key.toLowerCase()];
    if (n == null) return;
    noteOn(n);
    e.preventDefault();
  });
  window.addEventListener("keyup", (e) => {
    if (typingInControl()) return;
    const n = QWERTY[e.key.toLowerCase()];
    if (n != null) noteOff(n);
  });
}

// ── Settings popover (v1.3.0) ───────────────────────────────────────────────
// The gear panel holding the language selector. All state lives in this
// closure, so nothing here can join a TDZ chain.
//
// The panel holds the selector ALONE: this plugin has no tooltips bridge and
// never had a hover-help toggle — its help layer is always on — so a toggle row
// would be a control for a preference that does not exist.
//
// Styled in O-simpleSubtractive's own aged-paper vocabulary in css/styles.css:
// the panel wears the .group plate, the selector wears select.combo's border,
// radius and inset shadow at panel scale, and the green is the same --green-mid
// the .tour-btn:hover and the knob stem already use for a lit state. It is not
// a widget pasted in unchanged from another plugin.
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

// Pull the host sample rate for the headline's frequency-axis labels.
async function fetchSampleRate() {
  try {
    const sr = await Juce.getNativeFunction("getSampleRate")();
    if (sr > 0) nyquistHz = sr / 2;
    drawHeadline();   // relabel once we know the real rate
  } catch (e) { /* keep the default */ }
}

// ── Boot ────────────────────────────────────────────────────────────────────
function boot() {
  headline = makeCanvas("headlineCanvas");
  scope = makeCanvas("scopeCanvas");
  filterAdsr = makeCanvas("filterAdsrCanvas");
  ampAdsr = makeCanvas("ampAdsrCanvas");

  SLIDER_IDS.forEach(bindKnob);
  COMBO_IDS.forEach(bindCombo);

  // The signal-path readout/arrow track cutoff / resonance / filterEnvAmount.
  ["cutoff", "resonance", "filterEnvAmount"].forEach((id) => {
    const st = sliderState[id];
    if (st) { st.valueChangedEvent.addListener(updateDiagram); st.propertiesChangedEvent.addListener(updateDiagram); }
  });

  // The dual-ADSR shapes redraw when any of the 8 ADSR knobs move.
  ["filterAttack", "filterDecay", "filterSustain", "filterRelease",
   "ampAttack", "ampDecay", "ampSustain", "ampRelease"].forEach((id) => {
    const st = sliderState[id];
    if (st) { st.valueChangedEvent.addListener(drawDualAdsr); st.propertiesChangedEvent.addListener(drawDualAdsr); }
  });

  // The popover and the language sweep go FIRST, EACH IN ITS OWN try/catch.
  // applyI18n is what writes data-tip onto every anchor, and the delegated
  // tooltip listeners below read that attribute — but delegation means the
  // order between these two is free, and a translation-table typo must not be
  // allowed to take the twenty parameter bindings, the four canvases and the
  // keyboard down with it. That is the v1.4.0 TDZ failure this repo has already
  // paid for once.
  try { setupSettingsPopover(); } catch (e) { console.error("settings popover init failed:", e); }
  try { initI18n(); }             catch (e) { console.error("i18n init failed:", e); }

  setupTooltips();
  setupPresets();
  setupKeyboard();
  setupVizEvents();
  rewireResize();
  fetchSampleRate();

  updateDiagram();
  drawDualAdsr();

  // initial empty frames so the canvases aren't black
  drawHeadline();
  drawScope(new Array(128).fill(0));
}

if (document.readyState === "loading")
  document.addEventListener("DOMContentLoaded", boot);
else
  boot();
