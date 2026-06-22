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
  // Lesson presets — how each factory voice is built (values mirror FactoryPresets.cpp).
  lessonEpiano:   ["E-Piano · how it's built", "Carrier:modulator <strong>1:1</strong> (harmonic). A fast mod-envelope (decay 0.45 s &rarr; zero sustain) sweeps the index down from 5.5, so a bright pluck attack collapses to a near-pure sine as it rings. Velocity adds index — strike harder, sound brighter."],
  lessonTubular:  ["Tubular Bell · how it's built", "<strong>Inharmonic</strong> ratio <strong>1.41</strong> (&asymp;&radic;2, snap off) puts sidebands at non-integer multiples, so the partials never fuse into a pitch — that's the metallic ring. High index (8) plus long &asymp;3 s decays let it shimmer out."],
  lessonBrass:    ["Brass · how it's built", "Carrier:modulator <strong>1:1</strong> (harmonic). The index (4) swells in with the attack and <em>holds</em> at sustain — brightness tracks loudness, the way a blown brass note brightens as it gets louder."],
  lessonClarinet: ["Clarinet · how it's built", "Carrier:modulator <strong>2:1</strong>. A <strong>low index</strong> (2.2) keeps the spectrum sparse, and the 2:1 ratio emphasises <strong>odd harmonics</strong> &rarr; the hollow, stopped-pipe woody tone. High sustain, so it speaks steadily like a reed."],
  lessonClang:    ["Clang Bell · how it's built", "<strong>Inharmonic</strong> ratio 3.46 plus a <strong>very high index</strong> (14) throw a dense thicket of non-integer sidebands; 60% <strong>feedback</strong> bends the modulator toward noise. The spectrum smears into an atonal clang rather than a pitch."],
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

  // Teaching meta: harmonic when the ratio is (near) a whole number; Carson's
  // rule puts ~I+1 significant sideband pairs either side of the carrier.
  const meta = document.getElementById("routeMeta");
  if (meta) {
    const harmonic = Math.abs(ratioVal - Math.round(ratioVal)) < 0.02 && Math.round(ratioVal) >= 1;
    const nSb = Math.max(1, Math.round(idx.getScaledValue()) + 1);
    meta.textContent = `${harmonic ? "harmonic" : "inharmonic"} · ≈ ${nSb} sideband${nSb === 1 ? "" : "s"}`;
  }

  // Carrier-null teaching badge: the carrier (Bessel J₀) collapses to zero when
  // the EFFECTIVE radian index reaches the first J₀ zero, β ≈ 2.405. The readout's
  // I is the raw control value; the DSP applies a perceptual taper
  // (FMVoice::setParams → baseIndex = 20·(I/20)^1.7), and the render-harness
  // "carrier-null@2.405" test sets I so baseIndex == 2.405. Match that here so the
  // badge lights exactly when the carrier marker sits on a nulled peak (≈ I 5.75).
  const badge = document.getElementById("carrierNullBadge");
  if (badge) {
    const effIndex = 20 * Math.pow(idx.getScaledValue() / 20, 1.7);
    badge.classList.toggle("show", Math.abs(effIndex - 2.405) <= 0.15);
  }
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
// The five lessons ARE the factory presets — single source of truth lives in
// FactoryPresets.cpp. The tour buttons load them by name through the preset
// manager (which pushes every param via setValueNotifyingHost, so the knobs,
// toggles, and routing diagram all sync automatically). Only the teaching
// captions live here.
const LESSONS = {
  epiano:   { name: "E-Piano",      caption: "E-Piano — ratio 1:1 + a fast mod-envelope makes a bright pluck that mellows to a sine." },
  tubular:  { name: "Tubular Bell", caption: "Tubular Bell — an inharmonic ratio (1.41) sprays non-integer sidebands → metallic ring." },
  brass:    { name: "Brass",        caption: "Brass — ratio 1:1, index rises with the amp envelope; sustained, vowel-bright." },
  clarinet: { name: "Clarinet",     caption: "Clarinet — ratio 2:1 + low index emphasises odd harmonics → hollow, woody tone." },
  clang:    { name: "Clang Bell",   caption: "Clang Bell — high index + feedback smears the spectrum into a dense, noisy strike." },
};

async function applyLesson(key) {
  const lesson = LESSONS[key];
  if (!lesson) return;
  if (presetManager) await presetManager.loadPreset(lesson.name);
  const cap = document.getElementById("tourCaption");
  if (cap) cap.textContent = lesson.caption;
  document.querySelectorAll(".tour-btn").forEach((b) =>
    b.classList.toggle("active", b.getAttribute("data-preset") === key));
}

function setupPresets() {
  document.querySelectorAll(".tour-btn").forEach((btn) => {
    btn.addEventListener("click", () => applyLesson(btn.getAttribute("data-preset")));
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
  resize();   // single window 'resize' handler lives in rewireResize() (resizes + redraws)
  return { canvas, ctx, resize };
}

let spec = null, scope = null;
let lastSpectrum = null, lastScope = null;

// Spectrum x-axis is log-frequency 20 Hz → Nyquist (matches FmVizAnalyzer).
// nyquistHz comes from C++ (getSampleRate); 22.05 kHz is a safe pre-fetch default.
let nyquistHz = 22050;
const FREQ_TICKS = [100, 1000, 10000];
const fmtTickHz = (f) => (f >= 1000 ? `${f / 1000}k` : `${f}`);

// Carrier frequency of the most-recently-started note, pushed from C++ on the
// `carrierUpdate` event (0 = nothing sounding). Drives the FM sideband markers.
let carrierHz = 0;

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

  drawSidebandMarkers(ctx, w, h, logRange);
}

// ── FM sideband markers (predicted f_c ± k·f_m) ────────────────────────────
// Teaching overlay: the discrete FM spectrum lands at the carrier and at
// f_c ± k·f_m. Drawing the predicted positions lets the eye confirm that the
// live peaks sit exactly where Chowning's math says. f_m mirrors the DSP
// (FMVoice): the fixed Hz in Fixed Mode, else f_c·ratio (snap-rounded). Carrier
// (k = 0) is coloured distinctly from the sidebands. Skipped when no note sounds.
function drawSidebandMarkers(ctx, w, h, logRange) {
  if (carrierHz <= 20) return;

  let ratioVal = sliderState.ratio ? sliderState.ratio.getScaledValue() : 1;
  if (toggleState.ratioSnap && toggleState.ratioSnap.getValue()) ratioVal = Math.round(ratioVal);
  ratioVal = Math.max(0.001, ratioVal);

  const fixedOn = toggleState.modFixedMode && toggleState.modFixedMode.getValue();
  const fm = fixedOn
    ? (sliderState.modFixedHz ? sliderState.modFixedHz.getScaledValue() : 0)
    : carrierHz * ratioVal;

  const freqToX = (f) => (Math.log(f / 20) / logRange) * w;
  const top = 0, bottom = h - 11;

  // Sidebands first (under the carrier): faint dashed sage ticks, k = 1..8.
  if (fm > 0) {
    ctx.strokeStyle = "rgba(158, 196, 111, 0.42)";
    ctx.lineWidth = 1;
    ctx.setLineDash([3, 3]);
    ctx.beginPath();
    for (let k = 1; k <= 8; k++) {
      for (const f of [carrierHz - k * fm, carrierHz + k * fm]) {
        if (f <= 20 || f >= nyquistHz) continue;
        const x = freqToX(f);
        ctx.moveTo(x, top); ctx.lineTo(x, bottom);
      }
    }
    ctx.stroke();
    ctx.setLineDash([]);
  }

  // Carrier (k = 0): solid amber, labelled — distinct from the sidebands.
  if (carrierHz < nyquistHz) {
    const xc = freqToX(carrierHz);
    ctx.strokeStyle = "rgba(232, 176, 74, 0.9)";
    ctx.lineWidth = 1.5;
    ctx.beginPath(); ctx.moveTo(xc, top); ctx.lineTo(xc, bottom); ctx.stroke();
    ctx.fillStyle = "rgba(236, 188, 104, 0.95)";
    ctx.font = "9px Garamond, 'Times New Roman', serif";
    ctx.textAlign = "center";
    ctx.fillText("fc", xc, 9);   // carrier (played note); sidebands sit at fc ± k·fm
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
    // carrierUpdate is emitted just BEFORE spectrumUpdate each frame, so storing
    // it here lands in time for the same frame's sideband markers.
    window.__JUCE__.backend.addEventListener("carrierUpdate", (hz) => {
      carrierHz = (typeof hz === "number" && hz > 0) ? hz : 0;
    });
    window.__JUCE__.backend.addEventListener("spectrumUpdate", (arr) => drawSpectrum(arr));
    window.__JUCE__.backend.addEventListener("scopeUpdate", (arr) => drawScope(arr));
  } else {
    console.error("window.__JUCE__.backend unavailable — viz events will not arrive.");
  }
}

// Single resize handler: re-fit both canvas backing stores, then redraw the
// last frame (preserves the visible image across an editor resize).
function rewireResize() {
  window.addEventListener("resize", () => {
    if (spec) spec.resize();
    if (scope) scope.resize();
    if (lastSpectrum) drawSpectrum(lastSpectrum);
    if (lastScope) drawScope(lastScope);
  });
}

// ── On-screen keyboard (play without external MIDI) ──────────────────────────
// Notes are injected into the synth via the C++ `uiMidi` native function (queued
// through a MidiMessageCollector, merged into processBlock). Mouse is monophonic
// with glide; the computer keyboard is polyphonic over one octave (C4–C5).
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

  // Position each black key straddling the boundary of the white key below it.
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

// Pull the host sample rate for the spectrum frequency-axis labels.
async function fetchSampleRate() {
  try {
    const sr = await Juce.getNativeFunction("getSampleRate")();
    if (sr > 0) nyquistHz = sr / 2;
    if (lastSpectrum) drawSpectrum(lastSpectrum);   // relabel once we know the real rate
  } catch (e) { /* keep the default */ }
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
  setupKeyboard();
  setupVizEvents();
  rewireResize();
  fetchSampleRate();

  // initial empty frames so the canvases aren't black
  drawSpectrum(new Array(256).fill(-100));
  drawScope(new Array(128).fill(0));
}

if (document.readyState === "loading")
  document.addEventListener("DOMContentLoaded", boot);
else
  boot();
