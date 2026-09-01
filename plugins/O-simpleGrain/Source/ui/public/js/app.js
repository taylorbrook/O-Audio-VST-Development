/*
   This file is part of O-simpleGrain, an Ouaricon Audio plugin.
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
// O-simpleGrain — WebView UI controller (Phase 3.1)
// Binds all 19 APVTS params two-way (15 sliders + 2 combos + 2 toggles), wires the
// source drag-drop + "Load…" picker, and sets up DPR-aware canvas placeholders for
// the four visualizations (real rendering + viz-event subscriptions land in 3.2).
//
// NOTE: getSliderState / getComboBoxState / getToggleState / getNativeFunction live
// on the `Juce` ES-module namespace. The low-level backend (window.__JUCE__.backend)
// is reserved for the viz push-event listeners added in Phase 3.2.
// ============================================================================

import * as Juce from "./juce/index.js";
import { readFileEntryAsBase64 } from "./modules/webview-drop-streaming.js";

// ════════════════════════════════════════════════════════════════════════════
// INTERFACE LANGUAGE (v1.3.0) — canon v2, verbatim from scripts/i18n-canon.js.
//
// It sits HERE, directly under the imports and above every other declaration in
// this module, because `uiLanguage` is a module-level `let`: anything above it
// that reached applyLabel() would touch it inside its temporal dead zone and
// throw out of module evaluation, taking the nineteen parameter bindings, the
// four canvases, the drop handlers and the keyboard with it
// (pattern_module_toplevel_init_tdz). This file DOES have eager top-level work
// below — KNOB_IDS, FORMAT, GLOBAL_GRAIN_CAP — so the ordering is load-bearing
// here, not merely defensive. `node scripts/boot-all-uis.js` is the ONLY gate
// in the repo that sees this class of failure.
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


// ── Parameter inventory (must match OSimpleGrain::ParamIDs exactly) ─────────
const KNOB_IDS = [
  "grainSize", "density", "position", "scan",
  "pitchSpray", "positionSpray", "scatter", "grainPitch", "panSpray", "velToDensity",
  "ampAttack", "ampDecay", "ampSustain", "ampRelease",
  "outputLevel", "windowTaper",
];
const COMBO_IDS = ["sourceSample", "windowShape"];
const TOGGLE_IDS = ["freeze", "adsrEnabled"];

// ── Display formatters (keyed by param id) — receive the *scaled* value ─────
const fmtSec = (s) => (s < 1 ? `${Math.round(s * 1000)} ms` : `${s.toFixed(2)} s`);
const fmtPct = (v) => `${Math.round(v)}%`;            // value already 0..100
const fmtPct01 = (v) => `${Math.round(v * 100)}%`;    // value 0..1
const fmtSt = (v) => `${v >= 0 ? "+" : ""}${v.toFixed(1)} st`;
const FORMAT = {
  grainSize: (v) => `${Math.round(v)} ms`,
  windowTaper: fmtPct,
  density: (v) => `${Math.round(v)}/s`,
  position: fmtPct,
  scan: (v) => `${v >= 0 ? "+" : ""}${Math.round(v)}%`,
  pitchSpray: (v) => `${v.toFixed(1)} st`,
  positionSpray: fmtPct,
  scatter: fmtPct,
  grainPitch: fmtSt,
  panSpray: fmtPct,
  velToDensity: fmtPct,
  ampAttack: fmtSec, ampDecay: fmtSec, ampRelease: fmtSec,
  ampSustain: fmtPct01,
  outputLevel: (v) => `${v.toFixed(1)} dB`,
};

// ── Tooltip copy ────────────────────────────────────────────────────────────
// MOVED to js/i18n.js at v1.3.0. Through v1.2.1 the copy lived in a `TIPS`
// object here (34 entries, each [title, bodyHTML]) and each anchor carried the
// KEY in its own data-tip attribute. applyI18n now WRITES data-tip (the body)
// and data-tip-title on every anchor named by TIP_BINDINGS, so the renderer
// below reads the attributes rather than a table — one code path, and no way
// for a tip to be stranded in the previous language after the selector fires.
//
// The bodies lost their strong/em/code emphasis tags on the way. The WORDS are
// unchanged, and were compared back to v1.2.1 with entities decoded rather than
// re-typed. check-i18n assertion 9 forbids an angle bracket in an i18n.js
// string literal, and it is right to: the renderer writes textContent now, so a
// tag would render as literal characters rather than as emphasis.

// ── Knob geometry ────────────────────────────────────────────────────────────
const KNOB_MIN_DEG = -135;   // 0.0 normalised
const KNOB_MAX_DEG = 135;    // 1.0 normalised
const DRAG_TRAVEL_PX = 220;  // vertical px for a full 0..1 sweep

const sliderState = {};   // id -> Juce SliderState
const comboState  = {};   // id -> Juce ComboBoxState
const toggleState = {};   // id -> Juce ToggleState

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

// One-shot fine adjust shared by wheel + arrow-key (a full bracketed gesture).
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

// ── Combo box binding (sourceSample, windowShape) ───────────────────────────
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
  // ComboBox choices may not be present on first load — listen for both.
  st.propertiesChangedEvent.addListener(refresh);
  st.valueChangedEvent.addListener(refresh);
  refresh();

  sel.addEventListener("change", () => st.setChoiceIndex(sel.selectedIndex));

  // Switching the built-in source rebuilds currentSource on the C++ side (async);
  // the "sourceChanged" backend event (IN-08) drives the thumbnail refetch once
  // the decode has actually published — no fixed-delay race.
}

// ── Toggle binding (freeze) ──────────────────────────────────────────────────
function bindToggle(id) {
  const st = Juce.getToggleState(id);
  toggleState[id] = st;
  const el = document.getElementById(`toggle-${id}`);
  if (!el) { console.error(`Missing toggle element: toggle-${id}`); return; }

  const refresh = () => {
    const on = st.getValue();
    el.classList.toggle("active", on);
    // ADSR off → dim + lock the A/D/S/R knobs so the bypass is visually obvious.
    if (id === "adsrEnabled") {
      const knobs = document.getElementById("env-knobs");
      if (knobs) knobs.classList.toggle("env-bypassed", !on);
    }
  };
  st.valueChangedEvent.addListener(refresh);
  st.propertiesChangedEvent.addListener(refresh);
  refresh();

  el.addEventListener("click", () => { st.setValue(!st.getValue()); refresh(); });
}

// ── Toast ────────────────────────────────────────────────────────────────────
// v1.3.0: the COPY and the SHOWING are separate now. Through v1.2.1 showToast()
// took a finished English string and wrote it with `t.textContent = msg`, which
// is exactly the raw-write canon §3 forbids — a toast raised in English and
// still on screen when the selector fires would have been stranded there. Each
// call site names its own key through setLabel() instead, so the toast element
// is a [data-i18n] element from that moment on and the language sweep owns it.
const toastEl = () => document.getElementById("toast");

let toastTimer = null;
function flashToast() {
  const t = toastEl();
  if (!t) return;
  t.classList.add("show");
  if (toastTimer) clearTimeout(toastTimer);
  toastTimer = setTimeout(() => t.classList.remove("show"), 2600);
}

const statusEl = () => document.getElementById("sourceStatus");
function markStatusTruncated(truncated) {
  const el = statusEl();
  if (el) el.classList.toggle("truncated", !!truncated);
}

// Label for the next user-initiated load, consumed by the "sourceChanged"
// backend event (IN-08). Set BEFORE invoking the load so the event handler —
// which fires as soon as the decode publishes — can't beat it. Built-in
// switches leave it null (no status line, matching the old behaviour); the file
// picker parks the EMPTY STRING, which is falsy and selects the un-named pair
// of status writers above.
let pendingSourceLabel = null;

// ── After a load completes, surface the 10 s truncation notice ──────────────
// Four one-line writers, each naming a plain string literal. A single writer
// taking `name ? "label.sourceLoaded" : "label.sourceLoadedGeneric"` reads more
// naturally and is what this was first written as; check-i18n assertion 13
// rejects a conditional ANYWHERE inside a setLabel call, including the vars
// object, and it is right to — a reviewer cannot tell a message-selection
// ternary from a plural one by reading the call.
function writeSourceStatus(name, truncated) {
  const el = statusEl();
  if (!el) return;
  markStatusTruncated(truncated);
  if (name) {
    if (truncated) setLabel(el, "label.sourceTruncated", { name: name });
    else           setLabel(el, "label.sourceLoaded",    { name: name });
  } else {
    // The file picker knows no name until the decode publishes; v1.2.1 filled
    // the same two templates with the literal word "Source", and these two
    // entries are that result, authored rather than interpolated.
    if (truncated) setLabel(el, "label.sourceTruncatedGeneric");
    else           setLabel(el, "label.sourceLoadedGeneric");
  }
}

async function reportTruncationIfAny(name) {
  let wasTrunc = false;
  try { wasTrunc = await Juce.getNativeFunction("wasLastLoadTruncated")(); }
  catch (e) { wasTrunc = false; }
  writeSourceStatus(name, wasTrunc);
}

// ── Single-source drag-drop (custom: calls the GRAIN drop native fns) ───────
// The grain commit signature is (sessionId, filename, base64) — NOT the sampler's
// (sessionId, name, midi, vel). So we drive the 3 native fns directly here and
// reuse only readFileEntryAsBase64 from the shared module. All getNativeFunction
// calls go through the `Juce` ES-module namespace.
function newSessionId() {
  return `s${Date.now()}-${Math.random().toString(36).slice(2, 8)}`;
}

async function commitDroppedFile(fileEntry) {
  const sessionId = newSessionId();
  setLabel(toastEl(), "toast.loading", { name: fileEntry.name });
  flashToast();
  pendingSourceLabel = fileEntry.name;   // consumed by the sourceChanged event (IN-08)
  try {
    const startOk = await Juce.getNativeFunction("dropSessionStart")(sessionId, "");
    if (!startOk) { pendingSourceLabel = null; setLabel(toastEl(), "toast.dropStartFailed"); flashToast(); return; }

    const base64 = await readFileEntryAsBase64(fileEntry);

    const addOk = await Juce.getNativeFunction("dropSessionAddFile")(sessionId, fileEntry.name, base64);
    if (!addOk) { pendingSourceLabel = null; setLabel(toastEl(), "toast.transferFailed"); flashToast(); return; }

    const commitOk = await Juce.getNativeFunction("dropSessionCommitFile")(sessionId, fileEntry.name, base64);
    if (!commitOk) { pendingSourceLabel = null; setLabel(toastEl(), "toast.commitFailed"); flashToast(); return; }

    // Thumbnail + truncation status are driven by the "sourceChanged" backend
    // event, which fires only after the decode has actually published (IN-08).
  } catch (e) {
    pendingSourceLabel = null;
    console.error("[O-simpleGrain] drop failed:", e);
    // HOISTED. check-i18n assertion 13 rejects a conditional anywhere inside a
    // setLabel call — including in the vars object, which is where this one
    // used to sit as a template interpolation.
    const detail = (e && e.message) ? e.message : String(e);
    setLabel(toastEl(), "toast.dropFailed", { error: detail });
    flashToast();
  }
}

function bindSourceDrop() {
  const zone = document.getElementById("source-drop-zone");
  const audioRe = /\.(wav|aif|aiff)$/i;

  const hover = (on) => { if (zone) zone.classList.toggle("drop-hover", on); };

  document.addEventListener("dragenter", (e) => { e.preventDefault(); hover(true); });
  document.addEventListener("dragover", (e) => {
    e.preventDefault();
    try { e.dataTransfer.dropEffect = "copy"; } catch (_) { /* read-only in some hosts */ }
  });
  document.addEventListener("dragleave", (e) => { if (e.relatedTarget === null) hover(false); });

  document.addEventListener("drop", async (e) => {
    e.preventDefault();
    hover(false);
    if (!window.__JUCE__) return;

    const items = Array.from(e.dataTransfer?.items || []);
    const entry = items.length > 0 && typeof items[0].webkitGetAsEntry === "function"
      ? items[0].webkitGetAsEntry()
      : null;
    if (!entry) return;

    if (entry.isDirectory) { setLabel(toastEl(), "toast.dropFolder"); flashToast(); return; }
    if (!audioRe.test(entry.name)) { setLabel(toastEl(), "toast.dropFileType"); flashToast(); return; }
    await commitDroppedFile(entry);
  });
}

// ── "Load…" picker (always works where drag-drop doesn't) ───────────────────
function bindLoadButton() {
  const btn = document.getElementById("btnLoad");
  if (!btn) return;
  btn.addEventListener("click", async () => {
    try {
      // The picker is async on the C++ side; the "sourceChanged" backend event
      // reports truncation + refreshes the waveform once the decode publishes
      // (IN-08 — the old 1.2 s fixed delay went stale on a longer browse).
      // The empty string, not the word "Source": the status line's copy is a
      // table entry now, so the placeholder only has to be truthy-or-not.
      pendingSourceLabel = "";
      await Juce.getNativeFunction("loadSourceFromFileChooser")();
    } catch (e) {
      pendingSourceLabel = null;
      console.error("[O-simpleGrain] Load… failed:", e);
      setLabel(toastEl(), "toast.loadFailed");
      flashToast();
    }
  });
}

// ── Canvases (DPR-aware backing store — invariant 5) ────────────────────────
// Even the empty 3.1 placeholders get DPR-correct backing stores so the 3.2
// renderers draw crisp on Retina without a re-fit. CSS sizes the canvas via the
// positioned .canvas-wrap (calc()-equivalent), NOT right/bottom (replaced-element).
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

let canvases = {};

// Last frames retained so a resize (or a late sample-rate fetch) can redraw the
// current image rather than flashing blank.
let lastCloud = null;       // grainCloudUpdate payload (also drives the waveform)
let lastScope = null;       // 128-pt array
let lastSpectrum = null;    // 256-bin array
let sourceThumb = null;     // flat [min,max,…] envelope of the loaded source
let nyquistHz = 22050;      // updated from getSampleRate

function setupCanvases() {
  canvases = {
    cloud:    makeCanvas("cloudCanvas"),
    wave:     makeCanvas("sourceWaveCanvas"),
    scope:    makeCanvas("scopeCanvas"),
    spectrum: makeCanvas("spectrumCanvas"),
    inset:    makeCanvas("windowInsetCanvas"),
  };

  // Single resize handler: re-fit every backing store, then redraw the last frame
  // of each viz (preserves the visible image across an editor resize — invariant 5).
  window.addEventListener("resize", () => {
    Object.values(canvases).forEach((c) => c && c.resize());
    if (lastCloud) { drawCloud(lastCloud); drawSourceWaveform(lastCloud); }
    if (lastScope) drawScope(lastScope);
    if (lastSpectrum) drawSpectrum(lastSpectrum);
    drawWindowInset();
  });
}

// ── UI-01: grain-cloud scatter ──────────────────────────────────────────────
// Each grain is a sepia dot on aged paper. X = read position in the source,
// Y = grain pitch (relative semitones), radius ∝ grain size, a small lateral
// nudge from pan. The cloud accumulates as grains spawn (density thickens it;
// position/pitch spray widens it).
const CLOUD_PITCH_RANGE = 36;   // ± semitones mapped to the full canvas height

function drawCloud(f) {
  lastCloud = f;
  const c = canvases.cloud;
  if (!c || !f) return;
  const { canvas, ctx } = c;
  const w = canvas.clientWidth, h = canvas.clientHeight;
  ctx.clearRect(0, 0, w, h);

  // Faint pitch grid: centre (0 st) + ±octave guides.
  ctx.strokeStyle = "rgba(139,115,85,0.16)";
  ctx.lineWidth = 1;
  for (const semis of [-24, -12, 0, 12, 24]) {
    const y = h / 2 - (semis / CLOUD_PITCH_RANGE) * (h / 2);
    ctx.globalAlpha = semis === 0 ? 0.6 : 0.3;
    ctx.beginPath(); ctx.moveTo(0, y); ctx.lineTo(w, y); ctx.stroke();
  }
  ctx.globalAlpha = 1;

  const grains = f.grains || [];
  for (let i = 0; i < grains.length; i++) {
    const g = grains[i];                 // [readPosNorm, sizeMs, pitchSemis, pan, spawn]
    const readPos = g[0], sizeMs = g[1], pitch = g[2], pan = g[3];

    const lateral = (pan - 0.5) * 0.10 * w;   // small horizontal nudge from pan
    const x = Math.max(2, Math.min(w - 2, readPos * w + lateral));
    const y = Math.max(2, Math.min(h - 2,
      h / 2 - (Math.max(-CLOUD_PITCH_RANGE, Math.min(CLOUD_PITCH_RANGE, pitch)) / CLOUD_PITCH_RANGE) * (h / 2)));
    const r = Math.max(1.2, Math.min(7, 1.0 + sizeMs * 0.04));   // bigger grain = bigger dot

    // Sepia fill; warmer/brighter for higher pitch so the cloud reads as a
    // pitch field, not a flat dot soup.
    const warmth = (pitch + CLOUD_PITCH_RANGE) / (2 * CLOUD_PITCH_RANGE);   // 0..1
    const hue = 28 + warmth * 18;          // brown → amber
    const light = 32 + warmth * 22;
    ctx.fillStyle = `hsla(${hue}, 45%, ${light}%, 0.5)`;
    ctx.beginPath();
    ctx.arc(x, y, r, 0, Math.PI * 2);
    ctx.fill();
  }

  // Axis annotations (Task 3.3-4) — name what the scatter means, faintly.
  ctx.fillStyle = "rgba(139,115,85,0.5)";
  ctx.font = "8px Garamond, 'Times New Roman', serif";
  ctx.textBaseline = "bottom";
  ctx.textAlign = "left";
  ctx.fillText("← read position →", 4, h - 3);
  ctx.save();
  ctx.translate(9, h / 2);
  ctx.rotate(-Math.PI / 2);
  ctx.textAlign = "center";
  ctx.fillText("pitch", 0, 0);
  ctx.restore();
}

// ── UI-02: source waveform + playhead / freeze pin / spray range ─────────────
// Background = the static min/max thumbnail of the loaded source (fetched on load
// + at boot). Overlay = a brown vertical playhead at playheadNorm, a translucent
// shaded band [positionNorm ± positionSprayNorm], and a freeze-pin glyph when frozen.
function drawSourceWaveform(f) {
  lastCloud = f || lastCloud;
  const c = canvases.wave;
  if (!c) return;
  const { canvas, ctx } = c;
  const w = canvas.clientWidth, h = canvas.clientHeight;
  const mid = h / 2;
  ctx.clearRect(0, 0, w, h);

  // Static thumbnail (min/max pairs → filled brown waveform).
  if (sourceThumb && sourceThumb.length >= 2) {
    const pairs = sourceThumb.length / 2;
    ctx.fillStyle = "rgba(92,64,51,0.40)";
    ctx.beginPath();
    // top edge (max) left→right
    for (let p = 0; p < pairs; p++) {
      const x = (p / (pairs - 1)) * w;
      const mx = sourceThumb[p * 2 + 1];
      const y = mid - mx * (mid - 2);
      if (p === 0) ctx.moveTo(x, y); else ctx.lineTo(x, y);
    }
    // bottom edge (min) right→left
    for (let p = pairs - 1; p >= 0; p--) {
      const x = (p / (pairs - 1)) * w;
      const mn = sourceThumb[p * 2];
      const y = mid - mn * (mid - 2);
      ctx.lineTo(x, y);
    }
    ctx.closePath();
    ctx.fill();
  } else {
    ctx.fillStyle = "rgba(210,190,150,0.35)";
    ctx.font = "11px Garamond, 'Times New Roman', serif";
    ctx.textAlign = "center";
    ctx.textBaseline = "middle";
    ctx.fillText("drop or load a source to see its waveform", w / 2, mid);
  }

  if (!f) return;

  // Shaded spray band [position ± positionSpray].
  const pos = f.positionNorm ?? 0;
  const spray = f.positionSprayNorm ?? 0;
  if (spray > 0.0005) {
    const x0 = Math.max(0, (pos - spray)) * w;
    const x1 = Math.min(1, (pos + spray)) * w;
    ctx.fillStyle = "rgba(107,142,78,0.18)";   // green wash = the spawn-from range
    ctx.fillRect(x0, 0, Math.max(1, x1 - x0), h);
    ctx.strokeStyle = "rgba(60,92,26,0.35)";
    ctx.lineWidth = 1;
    ctx.beginPath(); ctx.moveTo(x0, 0); ctx.lineTo(x0, h);
    ctx.moveTo(x1, 0); ctx.lineTo(x1, h); ctx.stroke();
  }

  // Small annotation for the spray band (Task 3.3-4 — labels the spray range).
  if (spray > 0.02) {
    const cx = Math.max(0, Math.min(1, pos)) * w;
    ctx.fillStyle = "rgba(60,92,26,0.6)";
    ctx.font = "8px Garamond, 'Times New Roman', serif";
    ctx.textAlign = "center";
    ctx.textBaseline = "top";
    ctx.fillText("spray", cx, 2);
  }

  // Playhead (brown vertical line).
  const ph = Math.max(0, Math.min(1, f.playheadNorm ?? 0));
  const phx = ph * w;
  ctx.strokeStyle = f.frozen ? "rgba(180,120,40,0.95)" : "rgba(92,64,51,0.9)";
  ctx.lineWidth = f.frozen ? 2.5 : 1.5;
  ctx.beginPath(); ctx.moveTo(phx, 0); ctx.lineTo(phx, h); ctx.stroke();

  // Playhead annotation (only when NOT frozen — frozen gets the snowflake pin).
  if (!f.frozen) {
    ctx.fillStyle = "rgba(92,64,51,0.7)";
    ctx.font = "8px Garamond, 'Times New Roman', serif";
    ctx.textAlign = (phx > w - 36) ? "right" : "left";
    ctx.textBaseline = "bottom";
    ctx.fillText("playhead", (phx > w - 36) ? phx - 3 : phx + 3, h - 2);
  }

  // Freeze-pin glyph at the top of the playhead.
  if (f.frozen) {
    ctx.fillStyle = "rgba(180,120,40,0.95)";
    ctx.beginPath();
    ctx.arc(phx, 8, 4.5, 0, Math.PI * 2);
    ctx.fill();
    ctx.fillStyle = "rgba(245,230,211,0.95)";
    ctx.font = "8px Garamond, 'Times New Roman', serif";
    ctx.textAlign = "center";
    ctx.textBaseline = "middle";
    ctx.fillText("❄", phx, 8);
  }
}

// Fetch the source min/max envelope (on load + at boot) and repaint the waveform.
async function fetchSourceThumbnail() {
  try {
    const env = await Juce.getNativeFunction("getSourceThumbnail")(512);
    sourceThumb = Array.isArray(env) ? env : (env && env.length ? Array.from(env) : null);
  } catch (e) {
    sourceThumb = null;
  }
  drawSourceWaveform(lastCloud);
}

// ── UI-04: output scope (128 pts) — adapted verbatim from O-simpleFM ─────────
function drawScope(arr) {
  lastScope = arr;
  const c = canvases.scope;
  if (!c || !arr) return;
  const { canvas, ctx } = c;
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

// ── UI-04: spectrum (256 log-freq bins) — adapted from O-simpleFM ────────────
// NB: the FM sideband-marker overlay (carrier f_c ± k·f_m) is intentionally
// DROPPED here — granular synthesis has no carrier (Pitfall 2). The teaching
// point is read straight off the bars: discrete sidebands at scatter 0 (the
// pitched grain comb) smearing toward broadband noise as scatter rises.
const FREQ_TICKS = [100, 1000, 10000];
const fmtTickHz = (f) => (f >= 1000 ? `${f / 1000}k` : `${f}`);

function drawSpectrum(arr) {
  lastSpectrum = arr;
  const c = canvases.spectrum;
  if (!c || !arr) return;
  const { canvas, ctx } = c;
  const w = canvas.clientWidth, h = canvas.clientHeight;
  ctx.clearRect(0, 0, w, h);

  // dB grid lines.
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
    const norm = Math.max(0, Math.min(1, (db + 100) / 100));   // 0..1
    const barH = norm * (h - 2);
    const x = i * bw;
    const hue = 90 - norm * 40;    // green → yellow-green with intensity
    ctx.fillStyle = `hsl(${hue}, 55%, ${35 + norm * 30}%)`;
    ctx.fillRect(x, h - barH, Math.max(1, bw - 0.5), barH);
  }

  // Log-frequency axis ticks (20 Hz → Nyquist, matching the analyzer's map).
  const logRange = Math.log(nyquistHz / 20);
  ctx.strokeStyle = "rgba(139,115,85,0.22)";
  ctx.fillStyle = "rgba(210,190,150,0.7)";
  ctx.font = "9px Garamond, 'Times New Roman', serif";
  ctx.textAlign = "center";
  for (const fr of FREQ_TICKS) {
    if (fr >= nyquistHz) continue;
    const x = (Math.log(fr / 20) / logRange) * w;
    ctx.beginPath(); ctx.moveTo(x, 0); ctx.lineTo(x, h - 11); ctx.stroke();
    ctx.fillText(fmtTickHz(fr), x, h - 2);
  }
}

// Pull the host sample rate for the spectrum's frequency-axis labels.
async function fetchSampleRate() {
  try {
    const sr = await Juce.getNativeFunction("getSampleRate")();
    if (sr > 0) nyquistHz = sr / 2;
    if (lastSpectrum) drawSpectrum(lastSpectrum);   // relabel once the real rate is known
  } catch (e) { /* keep the default */ }
}

// ── Viz event subscriptions (low-level backend, NOT Juce.*) ─────────────────
// The five viz events arrive on window.__JUCE__.backend (the C++ side emits via
// emitEventIfBrowserIsVisible). Param state still goes through the Juce namespace
// (invariant 4). Event names are the C++↔JS contract — they must match exactly.
function setupVizEvents() {
  const be = window.__JUCE__ && window.__JUCE__.backend;
  if (!be) { console.error("window.__JUCE__.backend unavailable — viz events will not arrive."); return; }

  be.addEventListener("scopeUpdate",      (a) => drawScope(a));
  be.addEventListener("spectrumUpdate",   (a) => drawSpectrum(a));
  be.addEventListener("grainCloudUpdate", (f) => { drawCloud(f); drawSourceWaveform(f); });
  be.addEventListener("grainMeterUpdate", (n) => drawGrainReadout(n));

  // A new source published on the C++ side (IN-08): refetch the waveform, and
  // report load status only when a user-initiated load is pending (built-in
  // switches refresh silently, matching the old behaviour).
  be.addEventListener("sourceChanged", () => {
    fetchSourceThumbnail();
    // `!== null`, not truthiness: the file picker parks the EMPTY STRING here
    // (it has no name to show until the decode publishes), and a truthiness
    // gate would silently drop the picker's own status line.
    if (pendingSourceLabel !== null) {
      const name = pendingSourceLabel;
      pendingSourceLabel = null;
      reportTruncationIfAny(name);
    }
  });
}

// ── UI-03: window-envelope inset (recomputed in JS, redrawn on change only) ──
// The closed-form windows match the DSP LUTs (WindowLuts.h: 0=rect, 1=tri,
// 2=Welch, 3=Gauss σ≈0.18, 4=Hann, 5=Tukey). Drawn for one grain's envelope;
// recomputed only when windowShape / windowTaper / grainSize change (+ once at
// boot) — NOT per frame (Pitfall 4). No C++ change (Open Q2 default = JS recompute).
// CONTRACT (IN-05): these formulas + GAUSS_SIGMA + RECT_GUARD_MS + taperEndFor()
// re-implement WindowLuts.h — any change THERE must be mirrored HERE (and vice versa).
const GAUSS_SIGMA = 0.18;
const RECT_GUARD_MS = 1.0;      // WindowLuts::kRectGuardMs
const SHAPE_RECT = 0, SHAPE_TUKEY = 5;
const HANN = (x) => 0.5 * (1.0 - Math.cos(2 * Math.PI * x));
// Mirrors WindowLuts::taperEndFor(): phase-unit taper end for rect (fixed 1 ms
// guard) and Tukey (α/2, floored at the guard). Needs the grain length in ms
// because the guard is a TIME, not a phase fraction.
function taperEndFor(shape, alpha, grainMs) {
  const guard = RECT_GUARD_MS / Math.max(RECT_GUARD_MS, grainMs);
  let te = guard;
  if (shape === SHAPE_TUKEY) te = Math.max(guard, 0.5 * Math.min(1, Math.max(0, alpha)));
  return Math.min(0.5, Math.max(1e-4, te));
}
function windowValue(shape, phi, taperEnd) {
  switch (shape) {
    case 0:                                               // rectangular (guard-faded)
    case 5: {                                             // Tukey — one remap into Hann
      const u = Math.min(phi, 1 - phi);
      return HANN(Math.min(u / Math.max(1e-4, taperEnd), 1) * 0.5);
    }
    case 1: return 1.0 - Math.abs(2 * phi - 1);           // triangular
    case 2: { const u = 2 * phi - 1; return 1.0 - u * u; }// Welch
    case 3: { const d = (phi - 0.5) / GAUSS_SIGMA; return Math.exp(-0.5 * d * d); } // Gaussian (centre=1)
    case 4: return HANN(phi);                             // Hann
    default: return HANN(phi);
  }
}

function drawWindowInset() {
  const c = canvases.inset;
  if (!c) return;
  const { canvas, ctx } = c;
  const w = canvas.clientWidth, h = canvas.clientHeight;
  ctx.clearRect(0, 0, w, h);

  const st = comboState.windowShape;
  const shape = st ? st.getChoiceIndex() : 4;   // default Hann
  const alpha = sliderState.windowTaper ? sliderState.windowTaper.getScaledValue() * 0.01 : 0.5;
  const grainMs = sliderState.grainSize ? sliderState.grainSize.getScaledValue() : 30;
  const taperEnd = taperEndFor(shape, alpha, grainMs);

  // Taper knob only means something for Tukey — dim + lock it otherwise.
  const taperCell = document.getElementById("taper-cell");
  if (taperCell) taperCell.classList.toggle("taper-inactive", shape !== SHAPE_TUKEY);

  // baseline
  ctx.strokeStyle = "rgba(139,115,85,0.30)";
  ctx.lineWidth = 1;
  ctx.beginPath(); ctx.moveTo(0, h - 1.5); ctx.lineTo(w, h - 1.5); ctx.stroke();

  // envelope curve
  const N = 256;   // enough x-resolution for a 1 ms guard on a 500 ms grain
  ctx.strokeStyle = "#6B8E4E";
  ctx.lineWidth = 1.6;
  ctx.lineJoin = "round";
  ctx.beginPath();
  for (let i = 0; i < N; i++) {
    const phi = i / (N - 1);
    const v = Math.max(0, Math.min(1, windowValue(shape, phi, taperEnd)));
    const x = phi * w;
    const y = (h - 2) - v * (h - 4);
    if (i === 0) ctx.moveTo(x, y); else ctx.lineTo(x, y);
  }
  ctx.stroke();
}

// ── UI-05: grain-count / overlap / CPU readout ──────────────────────────────
// Grains N/192 (live, from grainMeterUpdate). Overlap ×Y = grainSizeSec × density
// (display-only, read from the Juce slider states — no extra tap). CPU bar is a
// coarse N/192 fill (Assumption A2 — no real CPU tap).
// Pushed from C++ via withInitialisationData (IN-05) — kGlobalGrainCap is the
// single source of truth; 192 is only the fallback for a missing bridge.
const GLOBAL_GRAIN_CAP =
  (window.__JUCE__ && window.__JUCE__.initialisationData
    && Array.isArray(window.__JUCE__.initialisationData.grainCap)
    && window.__JUCE__.initialisationData.grainCap[0]) || 192;

function drawGrainReadout(n) {
  const count = (typeof n === "number") ? n : 0;

  const grainsEl = document.getElementById("readoutGrains");
  if (grainsEl) grainsEl.textContent = `${count}/${GLOBAL_GRAIN_CAP}`;

  // Overlap ×Y from the live param states (grainSize ms → s, × density grains/s).
  const gs = sliderState.grainSize ? sliderState.grainSize.getScaledValue() : 0;   // ms
  const dn = sliderState.density   ? sliderState.density.getScaledValue()   : 0;   // grains/s
  const overlap = (gs / 1000) * dn;
  const overlapEl = document.getElementById("readoutOverlap");
  if (overlapEl) overlapEl.textContent = `×${overlap.toFixed(1)}`;

  // Coarse CPU bar from the active-grain load.
  const fill = document.getElementById("cpuFill");
  if (fill) {
    const frac = Math.max(0, Math.min(1, count / GLOBAL_GRAIN_CAP));
    fill.style.width = `${Math.round(frac * 100)}%`;
    // hint at cost climbing: green → amber → warm as the load rises
    const hue = 95 - frac * 70;
    fill.style.background = `hsl(${hue}, 55%, 45%)`;
  }
}

// ── Concept-preset tour (FUNC-06) ───────────────────────────────────────────
// Each button is one snapshot applied in C++ (applyFactoryPreset native fn) as a
// full APVTS write; the relays/attachments sync every knob/combo/toggle back to
// the page and the viz reacts on the next timer tick. We only set the caption +
// the active-button highlight here. data-preset MUST match the C++ preset names.
// Through v1.2.1 a LESSONS table here held each caption and applyLesson wrote
// it with a raw `cap.textContent = LESSONS[name] || ""`. Both the copy and the
// write have moved: the eight captions are authored table entries in
// js/i18n.js, and the write below goes through setLabel, which makes the
// caption a [data-i18n] element from that moment on so the language sweep owns
// it. Written raw it would be stranded in the language it was picked in the
// instant the selector fires — and it is the one string on this page that is
// chosen by a click.
//
// What is left here is a dispatch from the C++ preset name (which is also the
// button's data-preset, and is never localized) to a WRITER that names its
// caption key as a plain string literal.
//
// A map of name -> key with `setLabel(cap, KEYS[name] || "label.tourCaption")`
// reads more naturally and is what this was first written as. check-i18n
// assertion 13 rejects it twice over, correctly: a computed key cannot be
// checked against the table, and the `||` is the conditional-inside-a-localized-
// string shape contract §6 forbids. Eight call sites, eight literals, and
// assertion 15 can see that all eight keys are live.
const LESSON_CAPTION_WRITERS = {
  "Single Grain":       (el) => setLabel(el, "label.captionSingleGrain"),
  "Pitched Buzz":       (el) => setLabel(el, "label.captionPitchedBuzz"),
  "Fragments":          (el) => setLabel(el, "label.captionFragments"),
  "Smooth Cloud":       (el) => setLabel(el, "label.captionSmoothCloud"),
  "Frozen Pad":         (el) => setLabel(el, "label.captionFrozenPad"),
  "Asynchronous Cloud": (el) => setLabel(el, "label.captionAsyncCloud"),
  "Granular Fire":      (el) => setLabel(el, "label.captionGranularFire"),
  "Rect Click":         (el) => setLabel(el, "label.captionRectClick"),
};

let applyPresetFn = null;

async function applyLesson(name) {
  if (applyPresetFn) {
    try { await applyPresetFn(name); }
    catch (e) { console.error("[O-simpleGrain] applyFactoryPreset failed:", e); }
  }
  // An unknown name falls back to the resting caption rather than to "", so a
  // C++/JS name drift shows as the default text instead of a blank row.
  const cap = document.getElementById("tourCaption");
  if (cap) {
    const write = LESSON_CAPTION_WRITERS[name];
    if (write) write(cap);
    else setLabel(cap, "label.tourCaption");
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

// ── Tooltips ─────────────────────────────────────────────────────────────────
// v1.3.0: the copy is read from the ANCHOR'S OWN attributes, which applyI18n
// rewrote in the current language. v1.2.1 looked it up in a TIPS object keyed
// by the anchor's data-tip; that table is gone, and with it the second code
// path that would have gone stale on a language switch.
//
// The hover-help switch lives in the settings popover now (it was a "?" chip in
// the preset bar through v1.2.1). Its storage is unchanged: localStorage under
// "osg.tipsEnabled", so a preference set before this version survives the move.
// It is a BROWSER-side preference, not session state — this plugin has no
// tooltips bridge and never had one, and adding C++ state for it would be a new
// persistence surface rather than a localization change.
let tipsEnabled = true;                 // shipped default; localStorage wins at boot
let hideTooltip = () => {};             // published by setupTooltips (used by the toggle)

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
  try { stored = localStorage.getItem("osg.tipsEnabled"); } catch (e) { stored = null; }
  applyTipsEnabled(stored !== "false");

  btn.addEventListener("click", () => {
    applyTipsEnabled(!tipsEnabled);
    try { localStorage.setItem("osg.tipsEnabled", String(tipsEnabled)); } catch (e) { /* private mode */ }
  });
}

function setupTooltips() {
  const tip = document.getElementById("tooltip");
  if (!tip) return;
  let active = null;

  // Built with createElement + textContent, not innerHTML. The tip text is
  // table-sourced and localized now rather than a fixed literal, and localized
  // copy must never reach a markup path.
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
  // v1.4.2 (Stage O item 58) — the focus latch, ported from O-Comp v1.7.0
  // setupTooltips. Before it, a pointer click on any anchor that TAKES focus
  // (the eight .tour-btn lesson buttons, #gear-btn, #lang-select) hid the
  // hover tip on pointerdown and then re-opened it from focusin — the tip
  // came back under the pointer the instant the click landed. A latch on the
  // last input device is the rule: pointerdown latches, ANY keydown releases,
  // focusin opens only while released (keyboard focus still gets its tip —
  // the accessibility half of the feature), focusout hides, Escape hides.
  // :focus-visible is deliberately NOT the discriminator: Chromium reports it
  // false for a programmatic .focus() after a click, so a gate driving focus
  // directly would measure "no tip" and record a false pass. The one
  // programmatic .focus() on this page (initializeSettingsPopover: Escape ->
  // gearBtn.focus()) follows a KEYDOWN, so the latch is already released and
  // the gear's tip opens — keyboard-driven, correct. Knobs take no focus from
  // a click at all (bindKnob preventDefaults its pointerdown), so they were
  // never on this path.
  let lastInputWasPointer = false;

  document.addEventListener("pointerdown", () => { lastInputWasPointer = true; hide(); });

  document.addEventListener("focusin", (e) => {
    if (lastInputWasPointer) return;
    const el = anchorOf(e.target);
    if (!el) return;
    active = el;
    const r = el.getBoundingClientRect();
    show(el, r.left + r.width / 2, r.bottom);
  });
  document.addEventListener("focusout", hide);

  // One keydown listener, two jobs: any key at all means the keyboard is
  // driving again, which releases the latch above; Escape additionally hides.
  // Independent of initializeSettingsPopover()'s own Escape handler — both run.
  document.addEventListener("keydown", (e) => {
    lastInputWasPointer = false;
    if (e.key === "Escape") hide();
  });
}

// ── Settings popover (v1.3.0) ───────────────────────────────────────────────
// The gear panel holding the language selector and the hover-help switch. All
// state lives in this closure, so nothing here can join a TDZ chain.
//
// Styled in O-simpleGrain's own aged-paper vocabulary in css/styles.css: the
// panel wears the .group plate, the selector wears select.combo's border,
// radius and inset shadow at panel scale, and the switch wears the .tip-toggle
// chip's own lit-state greens — the chip it replaces. It is not a widget pasted
// in unchanged from another plugin.
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
  // uiMidi bridges keys → the processor's MidiMessageCollector (merged into
  // processBlock). The try/catch keeps the keyboard's highlight working even if
  // the binding is ever absent. (External MIDI works regardless.)
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
    else kb.appendChild(el);
  }
  blacks.forEach((el) => kb.appendChild(el));

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
  setupCanvases();

  KNOB_IDS.forEach(bindKnob);
  COMBO_IDS.forEach(bindCombo);
  TOGGLE_IDS.forEach(bindToggle);

  bindSourceDrop();
  bindLoadButton();
  setupPresets();

  // The popover, the language sweep and the hover-help switch go here, EACH IN
  // ITS OWN try/catch. applyI18n is what writes data-tip onto every anchor, and
  // the delegated tooltip listeners below read that attribute — but delegation
  // means the order between them is free, and a translation-table typo must not
  // be allowed to take the nineteen parameter bindings, the four canvases and
  // the keyboard down with it. That is the v1.4.0 TDZ failure this repo has
  // already paid for once.
  //
  // setupTipsToggle runs AFTER initI18n because it writes the switch's own face
  // through setLabel and would otherwise write it in the default language a
  // moment before the sweep, which is harmless but reads as a race.
  try { setupSettingsPopover(); } catch (e) { console.error("settings popover init failed:", e); }
  try { initI18n(); }             catch (e) { console.error("i18n init failed:", e); }
  try { setupTipsToggle(); }      catch (e) { console.error("tips toggle init failed:", e); }

  setupTooltips();
  setupKeyboard();

  // Viz: subscribe to the C++ push events, fetch the freq-axis sample rate, draw
  // the initial source thumbnail + window inset.
  setupVizEvents();
  fetchSampleRate();
  fetchSourceThumbnail();
  drawWindowInset();

  // The window inset is recomputed only on a windowShape / windowTaper /
  // grainSize change (Pitfall 4) — grainSize because the rect guard is a fixed
  // 1 ms, so its phase footprint depends on the grain length.
  if (comboState.windowShape)
    comboState.windowShape.valueChangedEvent.addListener(drawWindowInset);
  if (sliderState.windowTaper)
    sliderState.windowTaper.valueChangedEvent.addListener(drawWindowInset);
  if (sliderState.grainSize)
    sliderState.grainSize.valueChangedEvent.addListener(drawWindowInset);

  // Keep the Overlap readout honest when grainSize/density move (no extra tap —
  // recompute from the live slider states; count stays from the last meter push).
  let lastGrainCount = 0;
  if (sliderState.grainSize)
    sliderState.grainSize.valueChangedEvent.addListener(() => drawGrainReadout(lastGrainCount));
  if (sliderState.density)
    sliderState.density.valueChangedEvent.addListener(() => drawGrainReadout(lastGrainCount));
  // Track the latest count so the size/density-driven recompute keeps it.
  const be = window.__JUCE__ && window.__JUCE__.backend;
  if (be) be.addEventListener("grainMeterUpdate", (n) => { lastGrainCount = (typeof n === "number") ? n : 0; });
}

if (document.readyState === "loading")
  document.addEventListener("DOMContentLoaded", boot);
else
  boot();
