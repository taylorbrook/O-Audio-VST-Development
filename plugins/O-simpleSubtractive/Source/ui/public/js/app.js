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

// ── Tooltip copy (plain-language pedagogy, every control + readout) ─────────
const TIPS = {
  oscWave:        ["Oscillator Wave", "The raw tone the filter carves from. <strong>Saw</strong> is the brightest (all harmonics), <strong>Square</strong> is hollow (odd harmonics), <strong>Triangle</strong> is soft, <strong>Sine</strong> is pure — nothing for the filter to remove."],
  subLevel:       ["Sub Oscillator", "Mixes in a square wave one octave below the note. Adds body and weight underneath the main oscillator — useful for basses."],
  noiseLevel:     ["Noise", "Mixes in white noise — every frequency at once. Feed it through the filter to hear the filter's shape on its own, or add breath/air to a tone."],
  filterType:     ["Filter Type", "Which side of the cutoff is kept. <strong>Low-pass</strong> keeps lows (the classic subtractive sound), <strong>High-pass</strong> keeps highs, <strong>Band-pass</strong> keeps a band around cutoff, <strong>Notch</strong> removes a band."],
  filterSlope:    ["Filter Slope (poles)", "How sharply the filter cuts past the cutoff. <strong>6 dB/oct</strong> = 1 pole, gentle. <strong>24 dB/oct</strong> = 4 poles, steep and aggressive. Steeper = more of the spectrum removed just past the knee."],
  cutoff:         ["Cutoff Frequency", "The corner where the filter starts working. Lower it and watch the spectrum bars above the curve fall away — that's harmonics being removed, the heart of subtractive synthesis."],
  resonance:      ["Resonance", "Boosts a peak right at the cutoff. A little adds vocal emphasis; push it far and the filter rings, then <em>self-oscillates</em> into a pure sine whistle at the cutoff — the curve's peak grows into a spike."],
  filterEnvAmount:["Filter Env Amount", "How far the filter envelope sweeps the cutoff, and in which direction. Positive opens the filter on each note (bright attack); negative closes it. Bipolar: zero means the envelope does nothing."],
  keyTrack:       ["Key Tracking", "Makes the cutoff follow the note pitch — higher notes open the filter more. At 100% the filter tracks the keyboard so timbre stays consistent across the range."],
  filterAttack:   ["Filter Attack", "Time for the filter envelope to rise after note-on — how fast the filter sweep opens."],
  filterDecay:    ["Filter Decay", "Time for the filter envelope to fall from its peak to the sustain level — shapes the bright-to-dark motion of a pluck."],
  filterSustain:  ["Filter Sustain", "The cutoff-sweep level held while the key stays down."],
  filterRelease:  ["Filter Release", "Time for the filter sweep to fall back after the key is released."],
  ampAttack:      ["Amp Attack", "Time for loudness to rise after note-on. Short = a percussive start; long = a slow swell."],
  ampDecay:       ["Amp Decay", "Time for loudness to fall from its peak to the sustain level."],
  ampSustain:     ["Amp Sustain", "Loudness held while the key stays down."],
  ampRelease:     ["Amp Release", "Time for loudness to fade after the key is released — also sets how long the voice rings out."],
  voiceMode:      ["Voice Mode", "<strong>Poly</strong> plays chords. <strong>Mono</strong> plays one note, retriggering the envelopes each time. <strong>Legato</strong> plays one note but slurs — overlapping notes glide without retriggering."],
  glide:          ["Glide (portamento)", "Time to slide pitch from one note to the next. Most audible in Mono/Legato — zero is an instant jump."],
  outputLevel:    ["Output Level", "Master output trim in decibels. -60 dB is silence."],
  headline:       ["Filter Response over Spectrum", "The amber line is the filter's frequency response — the shape it imposes. The bars are the live output spectrum. Harmonics sitting above the curve's knee get pushed down: you are watching the filter remove sound."],
  scope:          ["Output Waveform", "The post-filter signal in the time domain. Watch a bright saw round off into a smooth shape as you lower the cutoff, or ring as resonance climbs."],
  filterAdsr:     ["Filter Envelope → cutoff", "The shape that sweeps the cutoff over time (Attack-Decay-Sustain-Release). The dashed marker shows the envelope's <em>live</em> output as you play — this scale drives brightness, not loudness."],
  ampAdsr:        ["Amp Envelope → level", "The shape that controls loudness over time. The dashed marker shows its live output — an independent scale from the filter envelope, so brightness and volume can move separately."],
  routing:        ["Signal Path", "Oscillator → Filter → Amplifier. The filter envelope routes up into the filter (sweeping cutoff); the amp envelope routes up into the VCA (shaping loudness). Two envelopes, two destinations."],
  // Lesson presets — wired live (FUNC-06).
  lessonSawSweep: ["Saw → LP Sweep · how it's built", "The headline move: a bright saw through a 24 dB low-pass with a slow filter envelope. Watch the upper harmonics fall away under the curve as the cutoff opens and closes — the literal subtraction the method is named for."],
  lessonPluck:    ["Pluck · how it's built", "A fast filter envelope (short decay, low sustain) snaps the cutoff bright-then-dark, while a quick amp decay makes a percussive note. Filter env does the timbral work."],
  lessonSweep:    ["Sweep Pad · how it's built", "Slow amp attack swells the level in; a long, deep filter envelope opens the cutoff gradually — you hear the spectrum brighten over seconds. Pads are about slow envelopes."],
  lessonAcid:     ["Acid Bass · how it's built", "High resonance + a snappy filter envelope on a saw through a 24 dB low-pass — the squelchy, ringing peak that defines the acid sound. Mono with a touch of glide."],
  lessonSelfOsc:  ["Self-Oscillation · how it's built", "Resonance pushed to the limit with the oscillators down: the filter rings on its own into a pure sine at the cutoff. The filter becomes the sound source."],
  lessonBrass:    ["Brass Stab · how it's built", "Positive filter-env amount so the cutoff opens with the attack and holds — brightness tracks the note like a blown brass instrument. A short, firm amp envelope gives the stab."],
  lessonSquareBass: ["Square Bass · how it's built", "A hollow square wave (odd harmonics only) plus the sub-oscillator an octave down for weight, through a 24 dB low-pass. Mono, so it plays as one solid bass voice — a polysynth is just several of these in parallel."],
  lessonNoiseWind:  ["Filtered Noise · how it's built", "Push the noise source up and band-pass it: with no harmonic source the filter sculpts pitchless air into wind. Slow envelopes swell it in and out — how subtractive synthesis makes breath and percussion, not just notes."],
};

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

// ── Concept-preset tour (UI-07) ─────────────────────────────────────────────
// Each button calls the C++ applyFactoryPreset native fn with its label; the
// processor writes the whole APVTS via setValueNotifyingHost and the relays /
// attachments sync every knob/combo back to the page (no DOM poking). The C++
// snapshot bodies land in Stage 4 (FUNC-06) — Stage 3 ships the live bridge.
const LESSONS = {
  "Saw Sweep":        "Saw → LP Sweep — a bright saw through a 24 dB low-pass with a slow filter envelope. Watch the harmonics fall away under the curve: the subtraction the method is named for.",
  "Pluck":            "Pluck — a fast filter envelope snaps bright-then-dark while the amp decays quickly; the filter envelope does the timbral work.",
  "Brass Stab":       "Brass Stab — positive filter-env amount opens the cutoff with the attack and holds it, so brightness tracks the note like brass.",
  "Sweep Pad":        "Sweep Pad — slow amp swell + a long, deep filter sweep open the spectrum gradually. Pads live in slow envelopes.",
  "Acid Bass":        "Acid Bass — high resonance and a snappy filter envelope through a 24 dB low-pass make the squelchy, ringing acid sound. Mono with a touch of glide.",
  "Square Bass":      "Square Bass — a hollow square plus the sub-oscillator for weight, played mono. The same voice as a polysynth, just one note at a time.",
  "Noise Wind":       "Noise Wind — band-passed white noise with no harmonic source: the filter sculpts pitchless air into wind. Subtractive synthesis beyond notes.",
  "Self-Oscillation": "Self-Oscillation — resonance at the limit: the filter rings into a pure sine that plays in tune across the keyboard. The filter becomes the source.",
};

let applyPresetFn = null;

async function applyLesson(name) {
  if (applyPresetFn) {
    try { await applyPresetFn(name); }
    catch (e) { console.error("[O-simpleSubtractive] applyFactoryPreset failed:", e); }
  }
  const cap = document.getElementById("tourCaption");
  if (cap) cap.textContent = LESSONS[name] || "";
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
