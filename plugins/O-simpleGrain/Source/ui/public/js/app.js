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
      // The picker is async on the C++ side; report truncation after a beat so the
      // decode has a chance to finish. Best-effort — the notice is informational.
      setTimeout(() => reportTruncationIfAny("Source"), 1200);
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

// Faint "awaiting signal" placeholder so the empty cells don't read as broken.
function drawPlaceholder(c, label) {
  if (!c) return;
  const { canvas, ctx } = c;
  const w = canvas.clientWidth, h = canvas.clientHeight;
  ctx.clearRect(0, 0, w, h);
  ctx.fillStyle = "rgba(210,190,150,0.28)";
  ctx.font = "11px Garamond, 'Times New Roman', serif";
  ctx.textAlign = "center";
  ctx.textBaseline = "middle";
  ctx.fillText(label, w / 2, h / 2);
}

let canvases = {};

function setupCanvases() {
  canvases = {
    cloud:    makeCanvas("cloudCanvas"),
    wave:     makeCanvas("sourceWaveCanvas"),
    scope:    makeCanvas("scopeCanvas"),
    spectrum: makeCanvas("spectrumCanvas"),
    inset:    makeCanvas("windowInsetCanvas"),
  };
  redrawPlaceholders();

  window.addEventListener("resize", () => {
    Object.values(canvases).forEach((c) => c && c.resize());
    redrawPlaceholders();
  });
}

function redrawPlaceholders() {
  drawPlaceholder(canvases.cloud, "grain cloud · plays in 3.2");
  drawPlaceholder(canvases.wave, "source waveform · plays in 3.2");
  drawPlaceholder(canvases.scope, "scope · plays in 3.2");
  drawPlaceholder(canvases.spectrum, "spectrum · plays in 3.2");
  drawPlaceholder(canvases.inset, "");
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
}

if (document.readyState === "loading")
  document.addEventListener("DOMContentLoaded", boot);
else
  boot();
