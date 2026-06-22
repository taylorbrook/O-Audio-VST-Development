// ============================================================================
// O-simpleAdditive — WebView UI controller
// Binds all 33 APVTS params two-way (16 drawbars + 15 knobs + 2 combo boxes),
// draws the oscilloscope, lights the drawbars as the live spectrum, and runs the
// pedagogical layer (tooltips, lesson preset tour, on-screen keyboard).
//
// NOTE: getSliderState / getComboBoxState / getNativeFunction live on the `Juce`
// ES-module namespace. window.__JUCE__ has the low-level backend (used here only
// for backend.addEventListener on the scope / drawbar-spectrum push events).
// ============================================================================

import * as Juce from "./juce/index.js";

// ── Parameter inventory (must match OSimpleAdditive::ParamIDs exactly) ───────
const NUM_PARTIALS = 16;
const DRAWBAR_IDS = Array.from({ length: NUM_PARTIALS }, (_, i) => `partial${i + 1}`);
const KNOB_IDS = [
  "scanPosition", "scanLfoRate", "scanLfoDepth", "scanEnvAmount",
  "spectralDecay", "velToDecay",
  "ampAttack", "ampDecay", "ampSustain", "ampRelease",
  "modAttack", "modDecay", "modSustain", "modRelease",
  "outputLevel",
];
const COMBO_IDS = ["frameBSource", "bitDepth"];

// ── Display formatters (keyed by param id) ──────────────────────────────────
// Each receives the *scaled* value (NormalisableRange::convertFrom0to1 output).
const fmtMs = (s) => (s < 1 ? `${Math.round(s * 1000)} ms` : `${s.toFixed(2)} s`);
const fmtPct = (v) => `${Math.round(v * 100)}%`;
const fmtPctSigned = (v) => `${v >= 0 ? "+" : ""}${Math.round(v * 100)}%`;
const FORMAT = {
  scanPosition: fmtPct,
  scanLfoRate: (v) => `${v.toFixed(2)} Hz`,
  scanLfoDepth: fmtPct,
  scanEnvAmount: fmtPctSigned,
  spectralDecay: fmtPct,
  velToDecay: fmtPct,
  ampAttack: fmtMs, ampDecay: fmtMs, ampSustain: fmtPct, ampRelease: fmtMs,
  modAttack: fmtMs, modDecay: fmtMs, modSustain: fmtPct, modRelease: fmtMs,
  outputLevel: (v) => `${v.toFixed(1)} dB`,
};

// ── Tooltip copy (plain-language, every control) ────────────────────────────
const TIPS = {
  drawbars:     ["Harmonic Drawbars", "Each bar sets one overtone's level — together they ARE the additive spectrum. The brass shows what you set; the green glow shows what's actually sounding (after morph &amp; spectral decay)."],
  frameBSource: ["Frame B Source", "The target spectrum the Scan morphs toward (Sine, Saw, Square, or Odd). Scan at 0% = your drawbars; 100% = this shape."],
  scanPosition: ["Scan", "Morphs the spectrum from your drawbars (0%) toward Frame B (100%). Watch the waveform change shape as you scan."],
  scanLfoRate:  ["Scan LFO Rate", "Speed of the sine LFO that sweeps Scan automatically. One global LFO — all held notes morph together, in phase."],
  scanLfoDepth: ["Scan LFO Depth", "How far the LFO sweeps Scan around its set position. 0% = no automatic morph."],
  scanEnvAmount:["Env → Scan", "How much the Modulation Envelope pushes Scan over each note (bipolar −/+). Makes the timbre evolve after the key is struck."],
  spectralDecay:["Spectral Decay", "Over each note, makes higher partials fade faster than lower ones — the tone darkens as it rings, like a plucked string. 0% = steady balance."],
  bitDepth:     ["Bit Depth", "Quantizes the output to N bits of amplitude resolution for early-digital grit. 'Off' = clean; fewer bits = more crunch."],
  velToDecay:   ["Velocity → Decay", "Lets how hard you play add to Spectral Decay. (Velocity always sets loudness; this adds timbral response — harder = darker decay.)"],
  ampAttack:    ["Amp Attack", "Time for loudness to rise after note-on."],
  ampDecay:     ["Amp Decay", "Time for loudness to fall from peak to the sustain level."],
  ampSustain:   ["Amp Sustain", "Held loudness while the key stays down."],
  ampRelease:   ["Amp Release", "Time for loudness to fade after the key is released — also how long the voice rings out."],
  modAttack:    ["Mod Attack", "Attack of the modulation envelope, which drives Scan via Env → Scan."],
  modDecay:     ["Mod Decay", "Decay of the modulation envelope toward its sustain level."],
  modSustain:   ["Mod Sustain", "Held level of the modulation envelope while the key is down."],
  modRelease:   ["Mod Release", "Release of the modulation envelope after key-up."],
  outputLevel:  ["Output Level", "Master output trim in decibels."],
  // Lesson presets.
  lessonSine:   ["Pure Sine", "Only the fundamental (H1) — one drawbar, one sine: the atom of additive synthesis."],
  lessonSaw:    ["Sawtooth", "Every harmonic present at 1/k — all overtones falling by 1/k give a bright, buzzy ramp."],
  lessonSquare: ["Square", "Odd harmonics only, at 1/k. Dropping the even partials gives the hollow, reedy tone."],
  lessonOrgan:  ["Organ", "A Hammond-style drawbar registration — a few low harmonics, instant attack, full sustain."],
  lessonMorph:  ["Morph Pad", "The scan LFO slowly morphs your drawbars toward a square; long envelopes make it breathe."],
  lessonLofi:   ["Lo-Fi Bells", "A spread bell spectrum + spectral-decay tilt + 8-bit quantization for digital grit."],
};

// Per-partial harmonic tooltips (generated).
function harmonicTip(k) {
  if (k === 1) return ["Partial 1 · Fundamental", "The fundamental — the pitch you hear. On its own it's a pure sine; it's the 1st harmonic of the overtone series."];
  const parity = (k % 2 === 1) ? "odd" : "even";
  const note = (k % 2 === 1)
    ? "Odd harmonics give hollow, reedy color — a square wave is built from these alone."
    : "Even harmonics reinforce octave-ish color and add body/warmth.";
  return [`Partial ${k} · ${parity} harmonic`, `The ${k}th harmonic — ${k}× the fundamental frequency. ${note}`];
}
for (let k = 1; k <= NUM_PARTIALS; k++) TIPS[`partial${k}`] = harmonicTip(k);

// ── Knob geometry ────────────────────────────────────────────────────────────
const KNOB_MIN_DEG = -135;   // 0.0 normalised
const KNOB_MAX_DEG = 135;    // 1.0 normalised
const DRAG_TRAVEL_PX = 220;  // vertical px for full 0..1 sweep

const sliderState = {};   // id -> Juce SliderState (knobs + drawbars)
const comboState = {};    // id -> Juce ComboBoxState
const drawbarCells = {};  // id -> .drawbar-cell element

function normToDeg(n) { return KNOB_MIN_DEG + n * (KNOB_MAX_DEG - KNOB_MIN_DEG); }

// One-shot fine adjust shared by wheel + arrow keys (a full drag gesture).
function nudge(st, delta, after) {
  const n = Math.max(0, Math.min(1, st.getNormalisedValue() + delta));
  st.sliderDragStarted();
  st.setNormalisedValue(n);
  st.sliderDragEnded();
  if (after) after();
}

// ── Knob binding (relative vertical drag) ───────────────────────────────────
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
  knob.addEventListener("keydown", (e) => {
    let delta = 0;
    if (e.key === "ArrowUp" || e.key === "ArrowRight") delta = 0.02;
    else if (e.key === "ArrowDown" || e.key === "ArrowLeft") delta = -0.02;
    else return;
    nudge(st, delta, () => updateKnobVisual(id));
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
    nudge(st, e.deltaY < 0 ? 0.02 : -0.02, () => updateKnobVisual(id));
    e.preventDefault();
  }, { passive: false });
}

// ── Drawbar bay (16 vertical faders that double as the live spectrum) ───────
function buildDrawbars() {
  const bay = document.getElementById("drawbarBay");
  if (!bay) return;
  for (let k = 1; k <= NUM_PARTIALS; k++) {
    const id = `partial${k}`;
    const cell = document.createElement("div");
    cell.className = "drawbar-cell " + (k === 1 ? "db-fundamental" : (k % 2 === 1 ? "db-odd" : "db-even"));
    cell.setAttribute("data-tip", id);
    cell.innerHTML =
      `<div class="drawbar" id="drawbar-${id}" role="slider" tabindex="0" aria-label="Partial ${k} level">` +
        `<div class="drawbar-fill" id="fill-${id}"></div>` +
        `<div class="drawbar-live" id="live-${id}"></div>` +
      `</div>` +
      `<div class="drawbar-num">${k}</div>` +
      `<div class="drawbar-val" id="val-${id}">0</div>`;
    bay.appendChild(cell);
    drawbarCells[id] = cell;
  }
}

function bindDrawbar(id) {
  const st = Juce.getSliderState(id);
  sliderState[id] = st;
  const track = document.getElementById(`drawbar-${id}`);
  const fill = document.getElementById(`fill-${id}`);
  const valEl = document.getElementById(`val-${id}`);
  if (!track) { console.error(`Missing drawbar element: drawbar-${id}`); return; }

  const update = () => {
    const n = st.getNormalisedValue();
    if (fill) fill.style.height = `${n * 100}%`;
    if (valEl) valEl.textContent = `${Math.round(st.getScaledValue() * 100)}`;
  };
  st.valueChangedEvent.addListener(update);
  st.propertiesChangedEvent.addListener(update);
  update();

  // Absolute fader drag: the bar follows the pointer (natural drawbar feel).
  const setFromY = (clientY) => {
    const r = track.getBoundingClientRect();
    let n = (r.bottom - clientY) / r.height;
    n = Math.max(0, Math.min(1, n));
    st.setNormalisedValue(n);
    update();
  };
  let dragging = false;
  const onMove = (e) => { if (dragging) { setFromY(e.clientY); e.preventDefault(); } };
  const onUp = () => {
    if (!dragging) return;
    dragging = false;
    st.sliderDragEnded();
    window.removeEventListener("pointermove", onMove);
    window.removeEventListener("pointerup", onUp);
  };
  track.addEventListener("pointerdown", (e) => {
    dragging = true;
    st.sliderDragStarted();
    setFromY(e.clientY);
    window.addEventListener("pointermove", onMove);
    window.addEventListener("pointerup", onUp);
    e.preventDefault();
  });
  track.addEventListener("wheel", (e) => {
    nudge(st, e.deltaY < 0 ? 0.04 : -0.04, update);
    e.preventDefault();
  }, { passive: false });
  track.addEventListener("keydown", (e) => {
    let delta = 0;
    if (e.key === "ArrowUp" || e.key === "ArrowRight") delta = 0.04;
    else if (e.key === "ArrowDown" || e.key === "ArrowLeft") delta = -0.04;
    else return;
    nudge(st, delta, update);
    e.preventDefault();
  });
}

// ── Combo box binding (Frame B source, Bit depth) ───────────────────────────
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

// ── Live drawbar spectrum (morphed + decayed levels of the sounding voice) ──
function updateDrawbarSpectrum(sounding, levels) {
  for (let k = 0; k < NUM_PARTIALS; k++) {
    const id = `partial${k + 1}`;
    const cell = drawbarCells[id];
    const live = document.getElementById(`live-${id}`);
    if (!cell || !live) continue;
    if (sounding && levels) {
      cell.classList.add("sounding");
      const lvl = Math.max(0, Math.min(1, levels[k]));
      live.style.height = `${lvl * 100}%`;
    } else {
      cell.classList.remove("sounding");   // idle → only the brass (set level) shows
    }
  }
}

// ── Oscilloscope (DPR-aware canvas) ─────────────────────────────────────────
function makeCanvas(id) {
  const canvas = document.getElementById(id);
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

let scope = null, lastScope = null;

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

function setupVizEvents() {
  if (window.__JUCE__ && window.__JUCE__.backend) {
    window.__JUCE__.backend.addEventListener("drawbarSpectrumUpdate", (d) => {
      if (d) updateDrawbarSpectrum(!!d.sounding, d.levels);
    });
    window.__JUCE__.backend.addEventListener("scopeUpdate", (arr) => drawScope(arr));
  } else {
    console.error("window.__JUCE__.backend unavailable — viz events will not arrive.");
  }
}

function rewireResize() {
  window.addEventListener("resize", () => {
    if (scope) scope.resize();
    if (lastScope) drawScope(lastScope);
  });
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

// ── Lesson preset tour ──────────────────────────────────────────────────────
// The lessons are applied in C++ (applyFactoryPreset native fn) as full APVTS
// snapshots; the relays/attachments sync every knob, drawbar, and combo here.
const LESSONS = {
  "Pure Sine":   "Pure Sine — only the fundamental (H1). One drawbar, one sine: the atom of additive synthesis.",
  "Sawtooth":    "Sawtooth — every harmonic at 1/k. All overtones falling by 1/k → a bright, buzzy ramp.",
  "Square":      "Square — odd harmonics only, at 1/k. Dropping the even partials gives the hollow, reedy tone.",
  "Organ":       "Organ — a Hammond-style drawbar registration: a few low harmonics, instant attack, full sustain.",
  "Morph Pad":   "Morph Pad — the scan LFO slowly morphs your drawbars toward a square; long envelopes make it breathe.",
  "Lo-Fi Bells": "Lo-Fi Bells — a spread bell spectrum + spectral-decay tilt + 8-bit quantization for digital grit.",
};
let applyPresetFn = null;

async function applyLesson(name) {
  if (applyPresetFn) { try { await applyPresetFn(name); } catch (e) { /* ignore */ } }
  const cap = document.getElementById("tourCaption");
  if (cap) cap.textContent = LESSONS[name] || "";
  document.querySelectorAll(".tour-btn").forEach((b) =>
    b.classList.toggle("active", b.getAttribute("data-preset") === name));
}

function setupPresets() {
  try { applyPresetFn = Juce.getNativeFunction("applyFactoryPreset"); } catch (e) { applyPresetFn = null; }
  document.querySelectorAll(".tour-btn").forEach((btn) => {
    btn.addEventListener("click", () => applyLesson(btn.getAttribute("data-preset")));
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
  scope = makeCanvas("scopeCanvas");

  buildDrawbars();
  DRAWBAR_IDS.forEach(bindDrawbar);
  KNOB_IDS.forEach(bindKnob);
  COMBO_IDS.forEach(bindCombo);

  setupTooltips();
  setupPresets();
  setupKeyboard();
  setupVizEvents();
  rewireResize();

  // initial empty scope so the canvas isn't black
  drawScope(new Array(128).fill(0));
}

if (document.readyState === "loading")
  document.addEventListener("DOMContentLoaded", boot);
else
  boot();
