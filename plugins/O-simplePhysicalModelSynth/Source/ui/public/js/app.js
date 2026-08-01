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
// NOTE: getSliderState / getComboBoxState / getNativeFunction live on the `Juce`
// ES-module namespace (used for control state + native-fn CALLS). The low-level
// backend (window.__JUCE__.backend.addEventListener) is used ONLY to RECEIVE the
// pushed spectrumUpdate / scopeUpdate / loopUpdate viz frames — never for calls.
// ============================================================================

import * as Juce from "./juce/index.js";
import { PresetManager } from "../modules/preset-manager.js";

// ── Parameter inventory (must match ParamIDs / the C++ relays exactly) ──────
const KNOB_IDS = [
  "excitationPosition", "excitationColor", "bowForce",
  "inharmonicity", "modeBrightness",
  "damping", "decay", "material",
  "coarseTune", "fineTune",
  "ampAttack", "ampRelease", "velToBrightness", "outputLevel",
];
const COMBO_IDS = ["excitationType", "resonatorType", "stringModel"];

// ── Tooltip content (UI-06) ──────────────────────────────────────────────────
// key → [title, bodyHTML]. The key MUST equal a data-tip="key" attribute in
// index.html (control cells + diagram boxes). The set of TIPS keys and the set of
// data-tip keys must match exactly — a missing key no-ops silently (no tooltip,
// no error). Plain-language teaching copy drawn from the BRIEF.
const TIPS = {
  // — Excitation —
  excitationType: ["Excitation", "How energy enters the model: <em>Pluck</em> (noise burst), <em>Strike</em> (mallet impulse), or <em>Bow</em> (sustained friction). Swap it to hear why the same string sounds different plucked vs struck vs bowed."],
  excitationPosition: ["Excite Position", "Where along the string the energy enters. Mid &#8594; rounder; near an end &#8594; thinner and brighter (a comb filter on the exciter)."],
  excitationColor: ["Excite Color", "Brightness / hardness of the exciter. Low = a soft mallet; high = a hard, bright attack."],
  bowForce: ["Bow Force", "<em>Bow only.</em> Friction pressure on the stick-slip drive &#8212; more force gives a noisier, richer attack. Greyed unless Excitation = Bow."],
  // — Resonator —
  resonatorType: ["Resonator", "The engine switch. <strong>String</strong> = Karplus-Strong (a harmonic comb). <strong>Modal</strong> = a bank of decaying sines (the inharmonic modes of bars &amp; bells)."],
  stringModel: ["String Model", "Karplus-Strong is the v1.0 engine. The two-delay <em>Waveguide</em> (which makes excitation position physical) arrives in v1.1."],
  inharmonicity: ["Inharmonicity", "<em>Modal only.</em> Stretches the mode spacing from harmonic (bar-like) toward inharmonic (bell-like): f&#8342; = f&#8320;&#183;k&#183;&#8730;(1+B&#183;k&#178;). The control that makes a bell sound like a bell, not a string."],
  modeBrightness: ["Mode Brightness", "<em>Modal only.</em> Tilts the upper modes louder and longer &#8212; how bright and metallic the struck body is."],
  // — Material / damping —
  damping: ["Damping", "The loop low-pass cutoff. It shaves a little high end on every pass, so the tone darkens as it decays &#8212; bright steel &#8596; muted nylon."],
  decay: ["Decay", "The loop feedback / ring time. Near one = long sustain; lower = a short, damped pluck. Always clamped below 1 so the loop can't run away."],
  material: ["Material", "One knob that co-moves <strong>Damping</strong> + <strong>Decay</strong> along the steel&#8596;nylon axis &#8212; watch both knobs track as you turn it."],
  // — Tuning —
  coarseTune: ["Coarse Tune", "Transpose in semitones (&#177;24)."],
  fineTune: ["Fine Tune", "Fine pitch in cents (&#177;100)."],
  // — Amp / dynamics —
  ampAttack: ["Amp Attack", "Output amplitude fade-in. Matters most for the sustained Bow &#8212; the body's own decay is intrinsic to the model."],
  ampRelease: ["Amp Release", "Output fade-out after note-off &#8212; how quickly the note is damped when you let go."],
  velToBrightness: ["Velocity &#8594; Brightness", "How much harder playing brightens and strengthens the excitation. The model's dynamic response &#8212; play harder, hear brighter."],
  outputLevel: ["Output Level", "Master output gain (&#8722;60 &#8230; 0 dB)."],
  // — Diagram boxes —
  diagExcitation: ["Excitation", "Energy is injected here &#8212; a pluck, strike, or bow. Its position and color shape the attack before it reaches the resonator."],
  diagResonator: ["Resonator loop", "Pitch comes from the loop length (fundamental = sample rate &#247; delay length). The pulse circling here dims a little each pass &#8212; that fading <em>is</em> the note decaying. In Modal mode it becomes the ringing mode stems."],
  diagMaterial: ["Material / damping", "Each pass loses a little energy: the low-pass (<em>Damping</em>) darkens it and the feedback (<em>Decay</em>) sets how long it rings. This is what turns steel into nylon."],
  diagOut: ["Output", "The summed 16-voice signal leaving the instrument, scaled by Output Level."],
};

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
  const cell = document.querySelector(`[data-tip="${id}"]`);
  if (cell) cell.classList.toggle("pm-disabled", disabled);
}

function applyEngineGating() {
  const res = comboState.resonatorType ? comboState.resonatorType.getChoiceIndex() : 0; // 0=String,1=Modal
  const exc = comboState.excitationType ? comboState.excitationType.getChoiceIndex() : 0; // 0=Pluck,1=Strike,2=Bow
  setDisabled("stringModel",    res !== 0);   // String-only
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
  const addGroup = (names, label) => {
    if (!names.length) return;
    const hdr = document.createElement("div");
    hdr.className = "preset-group-label";
    hdr.textContent = label;
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
  addGroup(list.filter((_, i) => flags[i]), "Factory");
  addGroup(list.filter((_, i) => !flags[i]), "User");
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
  const cap = document.getElementById("resonatorMode");
  if (cap) cap.textContent = modal ? "modal · stems" : "string · loop";
  if (lastLoop) drawLoop(lastLoop);   // repaint the new skin from the last frame
}

// ── Tooltips (UI-06) ─────────────────────────────────────────────────────────
// Single floating #tooltip div + data-tip="key" on every control cell + diagram
// box. Ported from O-simpleAdditive verbatim; viewport-edge flipping + a11y
// (focusin/out + Escape). Avoids native title= (slow, unstyled OS tooltips).
function setupTooltips() {
  const tip = document.getElementById("tooltip");
  if (!tip) return;
  let active = null;

  const show = (key, x, y) => {
    const entry = TIPS[key];
    if (!entry) return;
    tip.innerHTML = `<span class="tip-title">${entry[0]}</span>${entry[1]}`;
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
  const showAtEl = (key, el) => {
    const r = el.getBoundingClientRect();
    active = key;
    show(key, r.left + r.width / 2, r.bottom);
  };

  document.querySelectorAll("[data-tip]").forEach((el) => {
    const key = el.getAttribute("data-tip");
    el.addEventListener("pointerenter", (e) => { active = key; show(key, e.clientX, e.clientY); });
    el.addEventListener("pointermove", (e) => { if (active === key) position(e.clientX, e.clientY); });
    el.addEventListener("pointerleave", hide);
    el.addEventListener("pointerdown", hide);
    el.addEventListener("focusin", (e) => { e.stopPropagation(); showAtEl(key, el); });
    el.addEventListener("focusout", hide);
  });

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
  setupTooltips();

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
