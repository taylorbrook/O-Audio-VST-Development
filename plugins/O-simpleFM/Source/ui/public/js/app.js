// ============================================================================
// O-simpleFM — WebView UI controller
// Binds all 17 APVTS params two-way, draws the live spectrum + scope, and runs
// the pedagogical layer (routing diagram, tooltips, preset tour).
//
// NOTE: getSliderState / getToggleState / getNativeFunction live on the `Juce`
// ES-module namespace. window.__JUCE__ has the low-level backend (used here only
// for backend.addEventListener on the spectrum/scope push events).
// ============================================================================

import * as Juce from "./juce/index.js";
import { PresetManager } from "../modules/preset-manager.js";

// ── Parameter inventory (must match OSimpleFM::ParamIDs exactly) ───────────
const KNOB_IDS = [
  "ratio", "modIndex", "feedback", "modFixedHz", "modEnvToIndex", "velToIndex",
  "modAttack", "modDecay", "modSustain", "modRelease",
  "ampAttack", "ampDecay", "ampSustain", "ampRelease", "outputLevel",
];
const TOGGLE_IDS = ["ratioSnap", "modFixedMode"];

// ── Display formatters (keyed by param id) ─────────────────────────────────
// Each receives the *scaled* value (NormalisableRange::convertFrom0to1 output).
const fmtMs = (s) => (s < 1 ? `${Math.round(s * 1000)} ms` : `${s.toFixed(2)} s`);
const fmtPct = (v) => `${Math.round(v * 100)}%`;
const FORMAT = {
  ratio: (v) => `${v.toFixed(2)} : 1`,
  modIndex: (v) => v.toFixed(2),
  feedback: (v) => fmtPct(v),
  modFixedHz: (v) => (v >= 1000 ? `${(v / 1000).toFixed(2)} kHz` : `${Math.round(v)} Hz`),
  modEnvToIndex: fmtPct,
  velToIndex: fmtPct,
  modAttack: fmtMs, modDecay: fmtMs, modSustain: fmtPct, modRelease: fmtMs,
  ampAttack: fmtMs, ampDecay: fmtMs, ampSustain: fmtPct, ampRelease: fmtMs,
  outputLevel: (v) => `${v.toFixed(1)} dB`,
};

// ── Tooltip copy (plain-language, every parameter) ─────────────────────────
const TIPS = {
  ratio:        ["Ratio (C : M)", "Frequency of the modulator relative to the carrier. Whole-number ratios (1, 2, 3...) give harmonic, pitched timbres; irrational ratios (1.41, 2.76) give inharmonic, bell-like tones."],
  modIndex:     ["Modulation Index", "How hard the modulator bends the carrier's phase. Zero = pure sine. Raising it grows more and louder sidebands — the core of FM brightness."],
  feedback:     ["Feedback", "Routes the modulator back into itself. Pushes the modulator's shape from sine toward sawtooth, then toward noise — smearing the spectrum."],
  modFixedHz:   ["Fixed Modulator Hz", "When Fixed Mode is on, the modulator runs at this absolute frequency instead of tracking the played note — producing formant-like, key-independent colour."],
  modEnvToIndex:["Env → Index Depth", "How much the modulator envelope drives the index over time. This makes the timbre evolve after the key is struck (bright attack → mellow tail)."],
  velToIndex:   ["Velocity → Index", "Lets how hard you play add to the modulation index — harder strikes become brighter, like an acoustic instrument."],
  modAttack:    ["Mod Attack", "Time for the modulator (brightness) envelope to rise after note-on."],
  modDecay:     ["Mod Decay", "Time for the modulator envelope to fall from peak to its sustain level."],
  modSustain:   ["Mod Sustain", "Held brightness level while the key stays down."],
  modRelease:   ["Mod Release", "Time for brightness to fade after the key is released."],
  ampAttack:    ["Amp Attack", "Time for loudness to rise after note-on."],
  ampDecay:     ["Amp Decay", "Time for loudness to fall from peak to its sustain level."],
  ampSustain:   ["Amp Sustain", "Held loudness while the key stays down."],
  ampRelease:   ["Amp Release", "Time for loudness to fade after the key is released — also sets how long the voice rings out."],
  outputLevel:  ["Output Level", "Master output trim in decibels."],
  ratioSnap:    ["Ratio Snap", "Quantises the C:M ratio to whole numbers — instantly snaps an inharmonic tone to a harmonic one."],
  modFixedMode: ["Fixed Mode", "Switches the modulator from tracking the note (Ratio) to a fixed frequency in Hz (set by Fixed Hz)."],
  routing:      ["Signal Path", "MOD modulates the phase of CAR; MOD's self-loop is Feedback. Arrow thickness reflects Mod Index and Feedback amount."],
  readout:      ["Live FM Readout", "The two numbers that define the tone, updating as you play. Left — the C : M ratio: the modulator's frequency relative to the played note, which sets <em>which</em> harmonics appear (whole numbers = pitched, irrational = bell-like). Right — I, the modulation index: how hard the modulator bends the carrier, which sets the <em>brightness</em> (more index = more sidebands)."],
};

// ── Knob geometry ──────────────────────────────────────────────────────────
const KNOB_MIN_DEG = -135;   // 0.0 normalised
const KNOB_MAX_DEG = 135;    // 1.0 normalised
const DRAG_TRAVEL_PX = 220;  // vertical px for full 0..1 sweep

const sliderState = {};   // id -> Juce SliderState
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
    const n = Math.max(0, Math.min(1, st.getNormalisedValue() + delta));
    st.sliderDragStarted();
    st.setNormalisedValue(n);
    st.sliderDragEnded();
    updateKnobVisual(id);
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

  // Double-click resets to a sensible default via mouse wheel-free nudge:
  knob.addEventListener("dblclick", () => {
    // No backend default query; nudge to mid for index/feedback feel only when 0.
    // (Left intentionally minimal — host has full automation control.)
  });

  // Mouse wheel fine adjust.
  knob.addEventListener("wheel", (e) => {
    let n = st.getNormalisedValue() + (e.deltaY < 0 ? 0.02 : -0.02);
    n = Math.max(0, Math.min(1, n));
    st.sliderDragStarted();
    st.setNormalisedValue(n);
    st.sliderDragEnded();
    updateKnobVisual(id);
    e.preventDefault();
  }, { passive: false });
}

// ── Toggle binding ─────────────────────────────────────────────────────────
function bindToggle(id) {
  const st = Juce.getToggleState(id);
  toggleState[id] = st;
  const el = document.getElementById(`toggle-${id}`);
  if (!el) { console.error(`Missing toggle element: toggle-${id}`); return; }

  const refresh = () => {
    el.classList.toggle("active", st.getValue());
    if (id === "modFixedMode") updateFixedModeVisibility(st.getValue());
  };
  st.valueChangedEvent.addListener(refresh);
  st.propertiesChangedEvent.addListener(refresh);
  refresh();

  el.addEventListener("click", () => { st.setValue(!st.getValue()); refresh(); });
}

function updateFixedModeVisibility(fixed) {
  const cell = document.querySelector('.knob-cell[data-tip="modFixedHz"]');
  if (cell) cell.style.opacity = fixed ? "1" : "0.4";
}

// ── Routing diagram (reflects ratio / modIndex / feedback) ─────────────────
function updateRouting() {
  const idx = sliderState.modIndex;
  const fb = sliderState.feedback;
  const rt = sliderState.ratio;
  if (!idx || !fb || !rt) return;

  const idxNorm = idx.getNormalisedValue();      // 0..1
  const fbNorm = fb.getNormalisedValue();        // 0..1
  const ratioVal = rt.getScaledValue();

  const line = document.getElementById("modToCar");
  const head = document.getElementById("modToCarHead");
  if (line) line.style.strokeWidth = (1.5 + idxNorm * 6).toFixed(2);
  if (head) head.style.opacity = (0.35 + idxNorm * 0.65).toFixed(2);

  const arc = document.getElementById("fbArc");
  const fbHead = document.getElementById("fbHead");
  if (arc) { arc.style.strokeWidth = (0.5 + fbNorm * 5).toFixed(2); arc.style.opacity = (0.15 + fbNorm * 0.85).toFixed(2); }
  if (fbHead) fbHead.style.opacity = (0.15 + fbNorm * 0.85).toFixed(2);

  const mod = document.getElementById("opMod");
  if (mod) mod.style.filter = `brightness(${(1 + fbNorm * 0.25).toFixed(2)})`;

  const rRead = document.getElementById("routeRatioRead");
  if (rRead) rRead.textContent = `${ratioVal.toFixed(2)} : 1`;
  const iRead = document.getElementById("routeIndexRead");
  if (iRead) iRead.textContent = idx.getScaledValue().toFixed(1);
}

// ── Tooltips ────────────────────────────────────────────────────────────────
function setupTooltips() {
  const tip = document.getElementById("tooltip");
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
  // Keyboard path: anchor the tooltip under the focused control's box.
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
    // focusin/out bubbles from a focusable child knob up to its [data-tip] cell.
    // stopPropagation so a nested [data-tip] (e.g. the readout inside the routing
    // panel) isn't overridden by its ancestor's focusin handler on the bubble.
    el.addEventListener("focusin", (e) => { e.stopPropagation(); showAtEl(key, el); });
    el.addEventListener("focusout", hide);
  });

  // The routing panel has no focusable child — make it itself focusable.
  const routing = document.querySelector(".routing-panel[data-tip]");
  if (routing && !routing.hasAttribute("tabindex")) routing.setAttribute("tabindex", "0");

  document.addEventListener("keydown", (e) => { if (e.key === "Escape") hide(); });
}

// ── Preset tour ─────────────────────────────────────────────────────────────
// Each preset is a snapshot of *scaled* values per param id + toggle booleans.
// concept = one-line caption shown after applying.
const PRESETS = {
  epiano: {
    caption: "E-Piano — ratio 1:1 + a fast mod-envelope makes a bright pluck that mellows to a sine.",
    s: { ratio: 1, modIndex: 5.5, feedback: 0, modFixedHz: 220, modEnvToIndex: 1, velToIndex: 0.6,
         modAttack: 0.001, modDecay: 0.45, modSustain: 0, modRelease: 0.3,
         ampAttack: 0.002, ampDecay: 1.2, ampSustain: 0, ampRelease: 0.4, outputLevel: -3 },
    t: { ratioSnap: true, modFixedMode: false },
  },
  tubular: {
    caption: "Tubular Bell — an inharmonic ratio (1.41) sprays non-integer sidebands → metallic ring.",
    s: { ratio: 1.41, modIndex: 8.0, feedback: 0, modFixedHz: 220, modEnvToIndex: 1, velToIndex: 0,
         modAttack: 0.001, modDecay: 2.5, modSustain: 0, modRelease: 2.0,
         ampAttack: 0.001, ampDecay: 3.0, ampSustain: 0, ampRelease: 3.0, outputLevel: -4 },
    t: { ratioSnap: false, modFixedMode: false },
  },
  brass: {
    caption: "Brass — ratio 1:1, index rises with the amp envelope; sustained, vowel-bright.",
    s: { ratio: 1, modIndex: 4.0, feedback: 0, modFixedHz: 220, modEnvToIndex: 0.8, velToIndex: 0.4,
         modAttack: 0.08, modDecay: 0.2, modSustain: 0.7, modRelease: 0.15,
         ampAttack: 0.06, ampDecay: 0.1, ampSustain: 0.85, ampRelease: 0.2, outputLevel: -4 },
    t: { ratioSnap: true, modFixedMode: false },
  },
  clarinet: {
    caption: "Clarinet — ratio 2:1 + low index emphasises odd harmonics → hollow, woody tone.",
    s: { ratio: 2, modIndex: 2.2, feedback: 0, modFixedHz: 220, modEnvToIndex: 0.3, velToIndex: 0.2,
         modAttack: 0.04, modDecay: 0.1, modSustain: 0.9, modRelease: 0.1,
         ampAttack: 0.03, ampDecay: 0.1, ampSustain: 0.9, ampRelease: 0.12, outputLevel: -3 },
    t: { ratioSnap: true, modFixedMode: false },
  },
  clang: {
    caption: "Clang Bell — high index + feedback smears the spectrum into a dense, noisy strike.",
    s: { ratio: 3.46, modIndex: 14.0, feedback: 0.6, modFixedHz: 220, modEnvToIndex: 1, velToIndex: 0,
         modAttack: 0.001, modDecay: 1.8, modSustain: 0.1, modRelease: 1.5,
         ampAttack: 0.001, ampDecay: 2.2, ampSustain: 0, ampRelease: 2.2, outputLevel: -6 },
    t: { ratioSnap: false, modFixedMode: false },
  },
};

// scaled -> normalised inversion using the live properties of the slider state.
function scaledToNorm(st, scaled) {
  const p = st.properties;
  const span = (p.end - p.start) || 1;
  let lin = (scaled - p.start) / span;
  lin = Math.max(0, Math.min(1, lin));
  return Math.pow(lin, p.skew || 1);
}

function applyPreset(name) {
  const preset = PRESETS[name];
  if (!preset) return;
  for (const [id, scaled] of Object.entries(preset.s)) {
    const st = sliderState[id];
    if (!st) continue;
    st.sliderDragStarted();
    st.setNormalisedValue(scaledToNorm(st, scaled));
    st.sliderDragEnded();
    updateKnobVisual(id);
  }
  for (const [id, val] of Object.entries(preset.t)) {
    const st = toggleState[id];
    if (!st) continue;
    st.setValue(val);
    const el = document.getElementById(`toggle-${id}`);
    if (el) el.classList.toggle("active", val);
  }
  updateFixedModeVisibility(toggleState.modFixedMode ? toggleState.modFixedMode.getValue() : false);
  updateRouting();

  const cap = document.getElementById("tourCaption");
  if (cap) cap.textContent = preset.caption;
  document.querySelectorAll(".tour-btn").forEach((b) =>
    b.classList.toggle("active", b.getAttribute("data-preset") === name));
}

function setupPresets() {
  document.querySelectorAll(".tour-btn").forEach((btn) => {
    btn.addEventListener("click", () => applyPreset(btn.getAttribute("data-preset")));
  });
}

// ── Preset manager (persistent factory/user JSON presets) ────────────────────
// Backed by the suite OuariconPresetManager via 10 JUCE native functions. The
// in-UI "Lesson Presets" tour above is the pedagogical layer; this bar is the
// persistent, save/load-able layer that also survives DAW session reloads.
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
  // Pull a fresh list+current from C++ first, so the very first open can't race
  // the async init cache and render empty.
  await presetManager.refresh();
  const list = presetManager.getPresetList();
  const current = presetManager.getCurrentPreset();
  // Classify factory vs user (async) so the list groups cleanly.
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
    getNativeFunction: Juce.getNativeFunction,           // ES-module namespace, NOT window.__JUCE__
    onPresetChanged: () => { updateRouting(); updateDeleteButtonState(); closeDropdown(); },
  });
  presetManager.initialize().then(updateDeleteButtonState);   // disable Delete on the initial (factory) preset

  if (nameBtn) nameBtn.addEventListener("click", toggleDropdown);

  // Delete current preset (deletePreset is a no-op on factory presets).
  const delBtn = document.getElementById("presetDelete");
  if (delBtn) delBtn.addEventListener("click", async () => {
    const ok = await presetManager.deletePreset(presetManager.getCurrentPreset());
    if (ok) closeDropdown();
  });

  // Dismiss the dropdown on outside click / Escape.
  document.addEventListener("pointerdown", (e) => {
    const bar = document.getElementById("presetBar");
    if (bar && !bar.contains(e.target)) closeDropdown();
  });
  document.addEventListener("keydown", (e) => { if (e.key === "Escape") closeDropdown(); });
}

// ── Canvases (DPR-aware) ─────────────────────────────────────────────────────
function makeCanvas(id) {
  const canvas = document.getElementById(id);
  const ctx = canvas.getContext("2d");
  const resize = () => {
    const dpr = window.devicePixelRatio || 1;
    const w = canvas.clientWidth, h = canvas.clientHeight;
    canvas.width = Math.max(1, Math.round(w * dpr));
    canvas.height = Math.max(1, Math.round(h * dpr));
    ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
  };
  resize();
  window.addEventListener("resize", resize);
  return { canvas, ctx, resize };
}

let spec = null, scope = null;
let lastSpectrum = null, lastScope = null;

function drawSpectrum(arr) {
  lastSpectrum = arr;
  if (!spec) return;
  const { canvas, ctx } = spec;
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
}

function drawScope(arr) {
  lastScope = arr;
  if (!scope) return;
  const { canvas, ctx } = scope;
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

function setupVizEvents() {
  // The spectrum/scope are pushed from C++ via emitEventIfBrowserIsVisible.
  // These arrive on the low-level backend (window.__JUCE__.backend), NOT Juce.*.
  if (window.__JUCE__ && window.__JUCE__.backend) {
    window.__JUCE__.backend.addEventListener("spectrumUpdate", (arr) => drawSpectrum(arr));
    window.__JUCE__.backend.addEventListener("scopeUpdate", (arr) => drawScope(arr));
  } else {
    console.error("window.__JUCE__.backend unavailable — viz events will not arrive.");
  }
}

// redraw on canvas resize (preserve last frame)
function rewireResize() {
  window.addEventListener("resize", () => {
    if (lastSpectrum) drawSpectrum(lastSpectrum);
    if (lastScope) drawScope(lastScope);
  });
}

// ── Boot ────────────────────────────────────────────────────────────────────
function boot() {
  spec = makeCanvas("spectrumCanvas");
  scope = makeCanvas("scopeCanvas");

  KNOB_IDS.forEach(bindKnob);
  TOGGLE_IDS.forEach(bindToggle);

  // Routing diagram tracks the three relevant params.
  ["ratio", "modIndex", "feedback"].forEach((id) => {
    const st = sliderState[id];
    if (st) { st.valueChangedEvent.addListener(updateRouting); st.propertiesChangedEvent.addListener(updateRouting); }
  });
  updateRouting();

  setupTooltips();
  setupPresets();
  setupPresetManager();
  setupVizEvents();
  rewireResize();

  // initial empty frames so the canvases aren't black
  drawSpectrum(new Array(256).fill(-100));
  drawScope(new Array(128).fill(0));
}

if (document.readyState === "loading")
  document.addEventListener("DOMContentLoaded", boot);
else
  boot();
