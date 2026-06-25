// ============================================================================
// O-simpleGrain — WebView UI controller (Phase 3.1)
// Binds all 18 APVTS params two-way (15 sliders + 2 combos + 1 toggle), wires the
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
const TOGGLE_IDS = ["freeze"];

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

// ── Tooltip copy — minimal stubs this phase; full pedagogical copy lands in 3.3
const TIPS = {};

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

  // Switching the built-in source rebuilds currentSource on the C++ side (async).
  // Refetch the thumbnail after a beat so the UI-02 waveform tracks the new source.
  if (id === "sourceSample")
    st.valueChangedEvent.addListener(() => setTimeout(fetchSourceThumbnail, 300));
}

// ── Toggle binding (freeze) ──────────────────────────────────────────────────
function bindToggle(id) {
  const st = Juce.getToggleState(id);
  toggleState[id] = st;
  const el = document.getElementById(`toggle-${id}`);
  if (!el) { console.error(`Missing toggle element: toggle-${id}`); return; }

  const refresh = () => el.classList.toggle("active", st.getValue());
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
  try {
    const startOk = await Juce.getNativeFunction("dropSessionStart")(sessionId, "");
    if (!startOk) { showToast("Drop session start failed"); return; }

    const base64 = await readFileEntryAsBase64(fileEntry);

    const addOk = await Juce.getNativeFunction("dropSessionAddFile")(sessionId, fileEntry.name, base64);
    if (!addOk) { showToast("File transfer failed"); return; }

    const commitOk = await Juce.getNativeFunction("dropSessionCommitFile")(sessionId, fileEntry.name, base64);
    if (!commitOk) { showToast("File load failed at commit"); return; }

    await reportTruncationIfAny(fileEntry.name);
    fetchSourceThumbnail();   // refresh the UI-02 waveform background
  } catch (e) {
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
      await Juce.getNativeFunction("loadSourceFromFileChooser")();
      // The picker is async on the C++ side; report truncation + refresh the
      // waveform after a beat so the decode has a chance to finish. Best-effort.
      setTimeout(() => { reportTruncationIfAny("Source"); fetchSourceThumbnail(); }, 1200);
    } catch (e) {
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

  // Playhead (brown vertical line).
  const ph = Math.max(0, Math.min(1, f.playheadNorm ?? 0));
  const phx = ph * w;
  ctx.strokeStyle = f.frozen ? "rgba(180,120,40,0.95)" : "rgba(92,64,51,0.9)";
  ctx.lineWidth = f.frozen ? 2.5 : 1.5;
  ctx.beginPath(); ctx.moveTo(phx, 0); ctx.lineTo(phx, h); ctx.stroke();

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
}

// ── UI-03: window-envelope inset (recomputed in JS, redrawn on change only) ──
// The 5 closed-form windows match the DSP LUTs (WindowLuts.h: 0=rect, 1=tri,
// 2=Welch, 3=Gauss σ≈0.18, 4=Hann). Drawn for one grain's envelope; recomputed
// only when the windowShape combo changes (+ once at boot) — NOT per frame
// (Pitfall 4). No C++ change (Open Q2 default = JS recompute).
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
const GLOBAL_GRAIN_CAP = 192;

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

// ── Tooltips (mechanism wired now; copy filled in 3.3) ──────────────────────
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

  document.querySelectorAll("[data-tip]").forEach((el) => {
    const key = el.getAttribute("data-tip");
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
  // uiMidi is optional this phase — the processor may not expose it yet; the
  // keyboard still highlights so the layout reads correctly. (External MIDI works.)
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
