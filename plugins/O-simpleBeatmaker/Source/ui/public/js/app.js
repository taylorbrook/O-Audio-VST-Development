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
// ============================================================================

import * as Juce from "./juce/index.js";

// ── Voice roster — row order MUST match BeatmakerIDs kVoicePrefix / kGmNotes ──
const VOICES = [
  { prefix: "kick",      name: "Kick",       note: 36, hue: "#c0532b" },
  { prefix: "snare",     name: "Snare",      note: 38, hue: "#b5862e" },
  { prefix: "clap",      name: "Clap",       note: 39, hue: "#9a6b3f" },
  { prefix: "closedHat", name: "Closed Hat", note: 42, hue: "#5f9e57" },
  { prefix: "openHat",   name: "Open Hat",   note: 46, hue: "#3f8e93" },
  { prefix: "tom",       name: "Tom",        note: 45, hue: "#7a5ba6" },
];
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

// ── Tooltip copy (plain-language pedagogy on every control + the visuals) ─────
const TIPS = {
  grid: ["The Step Grid", "Each row is a drum voice, each column a sixteenth-note step. <strong>Click</strong> a cell to light a step; <strong>click again</strong> to cycle <em>normal → accent → ghost</em> (cell height = velocity); <strong>right-click</strong> to clear. The amber bar sweeping across is the playhead — when it crosses a lit cell, that voice fires."],
  lane: ["Timing / Groove Lane", "The standout view: each dot is a hit, placed at its <em>actual</em> moment relative to its grid line. A line to the left of the tick = early, right = late. This is the exact Δt applied to the audio — turn <strong>Swing</strong> and watch off-beats drift right together; <strong>Humanize</strong> scatters them; <strong>Quantize</strong> pulls the scatter back toward the grid while leaving swing."],
  midi: ["Live MIDI Readout", "Every note-on as it fires, from the sequencer (<strong>SEQ</strong>) and from notes you play in (<strong>MIDI</strong>). The grid and this list are two views of one MIDI stream — the sequencer literally emits these messages into the same buffer your playing does."],
  clearGrid: ["Clear All", "Erases every step in the pattern across all six voices — a blank grid to start a fresh beat from."],
  swing: ["Swing", "Delays every off-beat sixteenth, turning a stiff grid into a shuffle. 0% is dead-straight; 75% is the maximum MPC-style swing. Crucially, swing is <em>not</em> removed by Quantize — it is a deliberate, musical lateness, not an error."],
  humanize: ["Humanize", "Adds a small random timing and velocity wobble to every hit, like a human drummer who never lands exactly on the grid. A little brings a flat pattern to life; too much sounds sloppy. Watch the lane scatter as you raise it."],
  quantizeStrength: ["Quantize Strength", "How hard hits are pulled back onto the grid. At 100% the random humanize is fully removed (dead tight); at 0% the full wobble is kept. The exact tradeoff the craft names: quantize enough that the part is solid, without over-quantizing the life out of it. <em>Swing survives quantize</em> — only the random part is pulled in."],
  tempo: ["Tempo", "Playback speed in beats per minute — used when there is no host transport (the standalone app, or a stopped DAW). When a DAW is playing, the grid locks to the host's tempo instead."],
  patternLength: ["Pattern Length", "How many steps the loop is before it repeats: 8, 16, or 32. Shrinking then re-growing keeps the cells you drew — they are remembered, just not played while the loop is short."],
  outputLevel: ["Output Level", "Master output trim in decibels. −60 dB is silence."],
  voiceTune: ["Tune", "Shifts this voice's pitch up or down by up to an octave (±12 semitones). Tune the kick down for weight, the toms across a fill."],
  voiceDecay: ["Decay", "How long the voice rings out. Short snaps it into a tight tick; long lets it boom or sizzle. The musical range differs per voice (a kick boom vs. a closed-hat tick)."],
  voiceTone: ["Tone", "The voice's character knob — snap/brightness/body-vs-noise, depending on the drum. Sweep it to hear the timbre shift from dark to bright (or body to noise)."],
  voiceLevel: ["Level", "This voice's volume in the mix, in decibels. −60 dB silences it."],
  voiceMute: ["Mute", "Silences this voice without erasing its pattern — solo a part by muting the rest, or drop a voice out and back in."],
  voiceSolo: ["Solo", "Plays only the soloed voice(s), muting everything else. Great for hearing exactly what one drum is doing in the groove."],
  presets: ["Lesson Presets", "A guided tour where each preset isolates one idea — straight vs. swung, accents, ghost notes, humanize, quantize. Click one to load it, then tweak a knob to hear the concept."],
  lessonStraight:  ["Straight", "A flat, no-feel pattern — every hit dead on the grid at one velocity. The baseline that everything else departs from."],
  lessonAccents:   ["Backbeat + Accents", "Snare on 2 and 4 with hard accents, quieter hits between — how velocity alone turns a march into a groove."],
  lessonGhost:     ["Ghost Notes", "Quiet snare hits tucked between the backbeats — the secret to a pattern that breathes."],
  lessonSwing:     ["Triplet Swing", "The same pattern with swing pushed up — feel the off-beats slide late into a shuffle."],
  lessonHumanized: ["Humanized", "A tight pattern loosened with humanize — watch the lane scatter off the grid lines."],
  lessonQuantize:  ["Quantize Demo", "Humanize up, then sweep quantize strength to pull the scatter back — the tradeoff made audible and visible."],
};

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
  const lbl = knob.parentElement && knob.parentElement.querySelector(".knob-label");
  if (lbl) knob.setAttribute("aria-label", lbl.textContent);
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
    sel.innerHTML = "";
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
  if (mark) mark.textContent = tier === "accent" ? "▲" : tier === "ghost" ? "·" : "";
  // a11y: announce the cell's voice, step and current velocity state
  el.setAttribute("aria-pressed", vel > 0 ? "true" : "false");
  el.setAttribute("aria-label",
    `${VOICES[v].name} step ${s + 1}: ${vel === 0 ? "off" : `${tier} (velocity ${vel})`}`);
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
  rows.innerHTML = "";
  lastPhaseCol = -1;   // cells rebuilt — force the playhead class to re-apply
  for (let v = 0; v < NUM_VOICES; v++) {
    cellEls[v].fill(null);
    const row = document.createElement("div");
    row.className = "grid-row";
    row.style.setProperty("--voice-hue", VOICES[v].hue);

    const label = document.createElement("div");
    label.className = "row-label";
    label.textContent = VOICES[v].name;
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
      cell.innerHTML = '<div class="cell-fill"></div><div class="cell-mark"></div>';
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
  const srcCls = h.src === 1 ? "src-midi" : "src-seq";
  const srcTxt = h.src === 1 ? "MIDI" : "SEQ ";
  row.innerHTML = `<span class="${srcCls}">${srcTxt}</span> ` +
    `<span class="mr-note">note ${note}</span> ${voice ? voice.name : ""} ` +
    `<span class="mr-vel">vel ${h.vel}</span>`;
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

  const tEl = document.getElementById("readTransport");
  if (tEl) {
    const synced = !!frame.sync;
    tEl.textContent = synced ? "● synced" : "● free-run";
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
  // poll the authoritative grid ~4×/s so host preset/state restores show up
  gridPollAccum++;
  if (gridPollAccum >= 15) { gridPollAccum = 0; refreshGridFromBackend(); }
  requestAnimationFrame(raf);
}

// ── Tooltips ─────────────────────────────────────────────────────────────────
function initTooltips() {
  const tip = document.getElementById("tooltip");
  if (!tip) return;
  let active = null;
  const show = (key, x, y) => {
    const t = TIPS[key];
    if (!t) return;
    tip.innerHTML = `<span class="tt-title">${t[0]}</span>${t[1]}`;
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
  const showAtEl = (key, el) => {
    const r = el.getBoundingClientRect();
    show(key, r.left + r.width / 2, r.bottom);
  };
  document.querySelectorAll("[data-tip]").forEach((el) => {
    const key = el.getAttribute("data-tip");
    el.addEventListener("pointerenter", (e) => { active = key; show(key, e.clientX, e.clientY); });
    el.addEventListener("pointermove", (e) => { if (active === key) position(e.clientX, e.clientY); });
    el.addEventListener("pointerleave", hide);
    el.addEventListener("pointerdown", hide);
    // keyboard / focus users get the same teaching copy
    el.addEventListener("focusin", (e) => { e.stopPropagation(); active = key; showAtEl(key, el); });
    el.addEventListener("focusout", hide);
  });
  document.addEventListener("keydown", (e) => { if (e.key === "Escape") hide(); });
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
      if (caption) caption.textContent = `“${btn.getAttribute("data-preset")}” loaded — tweak a knob to hear the concept.`;
    });
  });
}

// ── Per-voice strips (built here so IDs stay in lock-step) ────────────────────
function buildVoiceStrips() {
  const host = document.getElementById("voiceStrips");
  if (!host) return;
  host.innerHTML = "";
  for (const v of VOICES) {
    const strip = document.createElement("div");
    strip.className = "voice-strip";
    strip.style.setProperty("--voice-hue", v.hue);
    const knobs = VOICE_SLIDER_SUFFIX.map((s) => {
      const id = v.prefix + s;
      const tipKey = "voice" + s;
      return `<div class="knob-cell cell-sm" data-tip="${tipKey}">
                <div class="knob knob-sm" id="knob-${id}"><div class="knob-stem"></div></div>
                <div class="knob-label">${s}</div>
                <div class="knob-value" id="val-${id}">—</div>
              </div>`;
    }).join("");
    strip.innerHTML = `<div class="vs-name">${v.name}</div>
      <div class="vs-knobs">${knobs}</div>
      <div class="vs-toggles">
        <button class="toggle-btn mute" id="toggle-${v.prefix}Mute" data-tip="voiceMute">Mute</button>
        <button class="toggle-btn solo" id="toggle-${v.prefix}Solo" data-tip="voiceSolo">Solo</button>
      </div>`;
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
