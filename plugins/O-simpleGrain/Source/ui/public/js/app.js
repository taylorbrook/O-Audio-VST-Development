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

// ── Parameter inventory (must match OSimpleGrain::ParamIDs exactly) ─────────
const KNOB_IDS = [
  "grainSize", "density", "position", "scan",
  "pitchSpray", "positionSpray", "scatter", "grainPitch", "panSpray", "velToDensity",
  "ampAttack", "ampDecay", "ampSustain", "ampRelease",
  "outputLevel",
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

// ── Tooltip copy (FUNC-07) — plain-language, class-grounded hover copy for EVERY
// control + the load action, the drop zone, the four visualizations, the readout,
// and the eight concept presets. Keyed by the data-tip attribute on each element;
// each entry is [title, bodyHTML]. Tone = field-guide, not jargon: every tip names
// the concept AND a concrete consequence you can hear/see.
const TIPS = {
  // ── Source ────────────────────────────────────────────────────────────────
  sourceSample: ["Source Sample",
    "The short sound the synth chops into grains. Granular synthesis never makes tone from scratch — it sprinkles tiny slices of <em>this</em> recording. Pick fire, voice, water, or piano; or drop your own below."],
  loadSource: ["Load your own",
    "Open a file picker to granulate any short <code>.wav</code> / <code>.aif</code> (capped at 10&nbsp;s). The same grain controls then operate on your sound — the engine doesn't care what the source is."],
  dropZone: ["Drop a source",
    "Drag a <code>.wav</code> / <code>.aif</code> here to granulate your own sound. Files over 10&nbsp;s are trimmed (you'll see a notice). Try a spoken word or a field recording — granular makes textures out of anything."],

  // ── Grain ─────────────────────────────────────────────────────────────────
  grainSize: ["Grain Size",
    "How long each slice is, 2–200&nbsp;ms. This is the buzz&nbsp;&harr;&nbsp;fragments axis: very short grains (a few&nbsp;ms) lose the source and turn to tone; long grains (&gt;60&nbsp;ms) keep recognisable chunks. With Density it sets how deeply grains <strong>overlap</strong> (overlap = size&nbsp;&times;&nbsp;density)."],
  density: ["Density",
    "Grains fired per second, 1–200. Sparse = you hear separated grains; dense = they fuse into a continuous cloud. Overlap-add only sounds smooth when many grains overlap, so Density and Size work together (watch the Overlap readout)."],
  position: ["Position",
    "Where in the source the read head rests, 0–100&nbsp;%. It's the point grains are sliced from — sweep it to scrub through the recording. Pairs with Scan (which moves the head) and Freeze (which pins it)."],
  scan: ["Scan / Time-Stretch",
    "How fast the read head travels, &minus;200…+200&nbsp;%. 0&nbsp;% holds on one instant; below 100&nbsp;% stretches the source in time without changing pitch; negative scans <em>backwards</em>. This is granular time-stretch."],
  freeze: ["Freeze",
    "Pins the read head on the current instant and sustains it forever — the grain stream keeps flowing but never advances through the source. Add Pitch Spray for a shimmering frozen pad. The pin crossfades in, so no click."],

  // ── Window ────────────────────────────────────────────────────────────────
  windowShape: ["Window Shape",
    "The fade envelope on each grain. Hann/Gauss fade in and out smoothly so overlapping grains crossfade cleanly. <strong>Rectangular has no fade</strong> — every grain edge is a hard step, which clicks. Try Rect to hear <em>why</em> windows exist (it's a lesson, not a bug)."],

  // ── Spray & Scatter ───────────────────────────────────────────────────────
  pitchSpray: ["Pitch Spray",
    "Random per-grain transposition, 0–12&nbsp;st. Each grain is nudged up/down by a random amount, so a frozen or static texture starts to shimmer and thicken instead of sitting on one dead pitch."],
  positionSpray: ["Position Spray",
    "Random per-grain read position, 0–100&nbsp;%. Scatters where each grain is sliced from around the Position point — turns a tight read into a wash drawn from a whole region of the source (shown as the green band on the waveform)."],
  scatter: ["Scatter",
    "Randomises the <em>timing</em> of grains, 0–100&nbsp;%. This is the synchronous&nbsp;&harr;&nbsp;asynchronous axis: at 0&nbsp;% grains fire on a perfect clock (a pitched comb, discrete sidebands); high scatter dissolves the comb into broadband noise. Watch the Spectrum."],
  grainPitch: ["Grain Pitch",
    "Global transposition of every grain, &minus;24…+24&nbsp;st. Stacks on top of MIDI key-tracking and Pitch Spray — shift the whole cloud up an octave without touching the keyboard."],
  panSpray: ["Pan Spray",
    "Per-grain stereo spread, 0–100&nbsp;%. At 0 every grain is centred; raise it and grains scatter left/right (equal-power), widening a mono source into an immersive stereo cloud."],
  velToDensity: ["Velocity → Density",
    "How much your playing velocity drives Density, 0–100&nbsp;%. At 0 density is fixed; raise it and harder keys spawn thicker clouds (loudness already follows velocity through the amp envelope — this adds <em>thickness</em> on top)."],

  // ── Amplitude envelope ─────────────────────────────────────────────────────
  adsrEnabled: ["ADSR On / Off",
    "Switches the per-voice amplitude envelope on or off. <strong>Off</strong> bypasses Attack/Decay/Sustain/Release — each note plays at a flat level while held and, on release, simply stops launching new grains so the cloud fades out over one grain length through the <em>Window</em> envelopes (no click). Turn it on for shaped swells and pads; off for a raw, immediate gate."],
  ampAttack: ["Amp Attack",
    "How quickly a note fades in, 0–5&nbsp;s. This is the per-<em>voice</em> envelope over the whole grain stream — short for percussive, long for a pad swell. (Each grain has its own tiny Window envelope; this is the bigger one.)"],
  ampDecay: ["Amp Decay",
    "How the note falls from its attack peak toward the sustain level, 0–5&nbsp;s. Together with Sustain it shapes the body of a held note."],
  ampSustain: ["Amp Sustain",
    "The level a held note settles at after the attack/decay, 0–100&nbsp;%. 100&nbsp;% holds full volume while a key is down; lower it for notes that bloom then back off."],
  ampRelease: ["Amp Release",
    "How long the note fades out after you let go, 0–5&nbsp;s. Long release lets clouds ring on and overlap into the next note — granular pads love a generous release."],

  // ── Output ─────────────────────────────────────────────────────────────────
  outputLevel: ["Output Level",
    "Master volume trim, &minus;&infin;…0&nbsp;dB. Dense overlapping clouds can pile up energy, so trim here if a thick patch peaks. (Headroom normalisation upstream already tames the worst of it.)"],

  // ── Visualizations ─────────────────────────────────────────────────────────
  vizCloud: ["Grain Cloud",
    "Every grain that spawns drops a sepia dot — horizontal = where in the source it was read, vertical = its pitch, dot size = grain length. Raise Density and the cloud thickens; raise the sprays and it spreads out."],
  vizWave: ["Source Waveform",
    "The loaded source drawn as a waveform. The brown line is the live read head (Position + Scan), the green band is the Position-Spray range grains are drawn from, and a snowflake pins the head when Freeze is on."],
  vizScope: ["Output Scope",
    "The actual audio coming out, plotted as a waveform. Useful for spotting the hard edges of a rectangular window (the clicks) versus the smooth crossfades of Hann."],
  vizSpectrum: ["Spectrum",
    "The frequency content of the output. At Scatter&nbsp;0 you'll see discrete spikes (the synchronous grain comb — a pitched sound); push Scatter up and the spikes smear into a continuous noise floor. The sync&nbsp;&rarr;&nbsp;async lesson, made visible."],
  readout: ["Grain Readout",
    "Live cost meter. <strong>Grains</strong> = active grains out of the 192 global cap. <strong>Overlap</strong> = grain size &times; density (how many grains sound at once — over ~2&times; they fuse). The <strong>CPU</strong> bar tracks the grain load: density &times; size &times; polyphony is what makes granular expensive."],

  // ── Concept presets (the 8-stop tour) ──────────────────────────────────────
  lessonSingleGrain: ["Single Grain",
    "One long grain fired slowly — density at the floor so grains stay separated. Hear a single slice on its own: the atom of granular synthesis."],
  lessonPitchedBuzz: ["Pitched Buzz",
    "Tiny grains fired fast and perfectly in sync. The grain <em>rate itself</em> becomes an audible pitch (a comb) — granular can make tone, not just texture."],
  lessonFragments: ["Fragments",
    "Medium grains, sparse. You still recognise chunks of the source — the middle ground between one grain and a smooth cloud."],
  lessonSmoothCloud: ["Smooth Cloud",
    "Many overlapping Hann grains fuse into one continuous, glassy texture. Overlap-add doing its job: size &times; density well above 1."],
  lessonFrozenPad: ["Frozen Pad",
    "Freeze pins the read head; Pitch Spray shimmers the frozen instant into a sustained, evolving pad that never moves through the source."],
  lessonAsyncCloud: ["Asynchronous Cloud",
    "High Scatter randomises the grain timing — the pitched comb dissolves and the spectrum smears into broadband noise. The async end of the axis."],
  lessonGranularFire: ["Granular Fire",
    "The worked example on the crackling-fire recording: a lively grain/spray set that turns a field recording into a moving granular bed."],
  lessonRectClick: ["Rect Click",
    "The intentional artifact: a rectangular window has no fade, so every grain edge clicks. Sparse grains let each click stand alone — this is <em>why</em> windows matter."],
};

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
let toastTimer = null;
function showToast(msg) {
  const t = document.getElementById("toast");
  if (!t) return;
  t.textContent = msg;
  t.classList.add("show");
  if (toastTimer) clearTimeout(toastTimer);
  toastTimer = setTimeout(() => t.classList.remove("show"), 2600);
}

function setSourceStatus(text, truncated) {
  const el = document.getElementById("sourceStatus");
  if (!el) return;
  el.textContent = text || "";
  el.classList.toggle("truncated", !!truncated);
}

// Label for the next user-initiated load, consumed by the "sourceChanged"
// backend event (IN-08). Set BEFORE invoking the load so the event handler —
// which fires as soon as the decode publishes — can't beat it. Built-in
// switches leave it null (no status line, matching the old behaviour).
let pendingSourceLabel = null;

// ── After a load completes, surface the 10 s truncation notice ──────────────
async function reportTruncationIfAny(label) {
  try {
    const wasTrunc = await Juce.getNativeFunction("wasLastLoadTruncated")();
    if (wasTrunc) setSourceStatus(`${label} — truncated to 10 s`, true);
    else setSourceStatus(`${label} loaded`, false);
  } catch (e) {
    setSourceStatus(`${label} loaded`, false);
  }
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
  showToast(`Loading ${fileEntry.name}…`);
  pendingSourceLabel = fileEntry.name;   // consumed by the sourceChanged event (IN-08)
  try {
    const startOk = await Juce.getNativeFunction("dropSessionStart")(sessionId, "");
    if (!startOk) { pendingSourceLabel = null; showToast("Drop session start failed"); return; }

    const base64 = await readFileEntryAsBase64(fileEntry);

    const addOk = await Juce.getNativeFunction("dropSessionAddFile")(sessionId, fileEntry.name, base64);
    if (!addOk) { pendingSourceLabel = null; showToast("File transfer failed"); return; }

    const commitOk = await Juce.getNativeFunction("dropSessionCommitFile")(sessionId, fileEntry.name, base64);
    if (!commitOk) { pendingSourceLabel = null; showToast("File load failed at commit"); return; }

    // Thumbnail + truncation status are driven by the "sourceChanged" backend
    // event, which fires only after the decode has actually published (IN-08).
  } catch (e) {
    pendingSourceLabel = null;
    console.error("[O-simpleGrain] drop failed:", e);
    showToast(`Drop failed: ${e && e.message ? e.message : e}`);
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

    if (entry.isDirectory) { showToast("Drop a single audio file, not a folder"); return; }
    if (!audioRe.test(entry.name)) { showToast("Drop a .wav / .aif / .aiff file"); return; }
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
      pendingSourceLabel = "Source";
      await Juce.getNativeFunction("loadSourceFromFileChooser")();
    } catch (e) {
      pendingSourceLabel = null;
      console.error("[O-simpleGrain] Load… failed:", e);
      showToast("Load failed");
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
    if (pendingSourceLabel) {
      const label = pendingSourceLabel;
      pendingSourceLabel = null;
      reportTruncationIfAny(label);
    }
  });
}

// ── UI-03: window-envelope inset (recomputed in JS, redrawn on change only) ──
// The 5 closed-form windows match the DSP LUTs (WindowLuts.h: 0=rect, 1=tri,
// 2=Welch, 3=Gauss σ≈0.18, 4=Hann). Drawn for one grain's envelope; recomputed
// only when the windowShape combo changes (+ once at boot) — NOT per frame
// (Pitfall 4). No C++ change (Open Q2 default = JS recompute).
// CONTRACT (IN-05): these formulas + GAUSS_SIGMA re-implement WindowLuts.h's
// build() — any change THERE must be mirrored HERE (and vice versa).
const GAUSS_SIGMA = 0.18;
function windowValue(shape, phi) {
  switch (shape) {
    case 0: return 1.0;                                   // rectangular
    case 1: return 1.0 - Math.abs(2 * phi - 1);           // triangular
    case 2: { const u = 2 * phi - 1; return 1.0 - u * u; }// Welch
    case 3: { const d = (phi - 0.5) / GAUSS_SIGMA; return Math.exp(-0.5 * d * d); } // Gaussian (centre=1)
    case 4: return 0.5 * (1.0 - Math.cos(2 * Math.PI * phi));   // Hann
    default: return 0.5 * (1.0 - Math.cos(2 * Math.PI * phi));
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

  // baseline
  ctx.strokeStyle = "rgba(139,115,85,0.30)";
  ctx.lineWidth = 1;
  ctx.beginPath(); ctx.moveTo(0, h - 1.5); ctx.lineTo(w, h - 1.5); ctx.stroke();

  // envelope curve
  const N = 128;
  ctx.strokeStyle = "#6B8E4E";
  ctx.lineWidth = 1.6;
  ctx.lineJoin = "round";
  ctx.beginPath();
  for (let i = 0; i < N; i++) {
    const phi = i / (N - 1);
    const v = Math.max(0, Math.min(1, windowValue(shape, phi)));
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
const LESSONS = {
  "Single Grain":       "Single Grain — one long grain fired slowly. Density at the floor keeps grains separated: hear a single slice on its own, the atom of granular synthesis.",
  "Pitched Buzz":       "Pitched Buzz — tiny grains fired fast and perfectly in sync. The grain rate itself becomes an audible pitch (a comb). Granular can make tone, not just texture.",
  "Fragments":          "Fragments — medium grains, sparse. You still recognise chunks of the source: the middle ground between one grain and a smooth cloud.",
  "Smooth Cloud":       "Smooth Cloud — many overlapping Hann grains fuse into one continuous, glassy texture. Overlap-add at work: size × density well above 1.",
  "Frozen Pad":         "Frozen Pad — Freeze pins the read head; Pitch Spray shimmers the frozen instant into a sustained, evolving pad that never moves through the source.",
  "Asynchronous Cloud": "Asynchronous Cloud — high Scatter randomises grain timing. The pitched comb dissolves and the spectrum smears into broadband noise: the async end of the axis.",
  "Granular Fire":      "Granular Fire — the worked example on the crackling-fire recording. A lively grain/spray set turns a field recording into a moving granular bed.",
  "Rect Click":         "Rect Click — the intentional artifact: a rectangular window has no fade, so every grain edge clicks. Sparse grains let each click stand alone. This is why windows matter.",
};

let applyPresetFn = null;

async function applyLesson(name) {
  if (applyPresetFn) {
    try { await applyPresetFn(name); }
    catch (e) { console.error("[O-simpleGrain] applyFactoryPreset failed:", e); }
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

// ── Tooltips ─────────────────────────────────────────────────────────────────
function setupTooltips() {
  const tip = document.getElementById("tooltip");
  if (!tip) return;
  let active = null;

  const show = (key, x, y) => {
    const entry = TIPS[key];
    if (!entry) return;   // 3.1: most keys have no copy yet (3.3 fills TIPS)
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

  // Strip HTML tags/entities to a plain string for the native title= fallback.
  const plain = (html) => {
    const d = document.createElement("div");
    d.innerHTML = html;
    return (d.textContent || d.innerText || "").replace(/\s+/g, " ").trim();
  };

  document.querySelectorAll("[data-tip]").forEach((el) => {
    const key = el.getAttribute("data-tip");
    const entry = TIPS[key];
    // Native title= as a robust fallback (accessibility + hosts where the custom
    // floating tooltip might be missed). The floating tooltip is the rich version.
    if (entry && !el.hasAttribute("title"))
      el.setAttribute("title", `${entry[0]} — ${plain(entry[1])}`);
    el.addEventListener("pointerenter", (e) => { active = key; show(key, e.clientX, e.clientY); });
    el.addEventListener("pointermove", (e) => { if (active === key) position(e.clientX, e.clientY); });
    el.addEventListener("pointerleave", hide);
    el.addEventListener("pointerdown", hide);
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
  setupTooltips();
  setupKeyboard();

  // Viz: subscribe to the C++ push events, fetch the freq-axis sample rate, draw
  // the initial source thumbnail + window inset.
  setupVizEvents();
  fetchSampleRate();
  fetchSourceThumbnail();
  drawWindowInset();

  // The window inset is recomputed only on a windowShape change (Pitfall 4).
  // The overlap/CPU readout floor is refreshed alongside (grainSize/density may
  // have changed). The grain count itself is driven by grainMeterUpdate.
  if (comboState.windowShape)
    comboState.windowShape.valueChangedEvent.addListener(drawWindowInset);

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
