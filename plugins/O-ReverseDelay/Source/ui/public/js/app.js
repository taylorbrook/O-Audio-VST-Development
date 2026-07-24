// ============================================================================
// O-ReverseDelay — WebView UI controller (Stage 3)
//
// Binds all 10 APVTS parameters two-way: 8 WebSliderRelay knobs + 2
// WebComboBoxRelay controls (syncMode as a segment pair, noteDivision as a
// select). One native function only: getParameterDefaults (dblclick-reset).
//
// STRUCTURE IS LOAD-BEARING: every module-level `const`/`let` is declared in
// this top block, and the single init() call sits at the BOTTOM of the file.
// A top-level call that reaches a not-yet-initialised binding throws a
// ReferenceError out of module evaluation and silently kills the ENTIRE UI
// (pattern_module_toplevel_init_tdz).
//
// Readouts and knob angles come exclusively from the SliderState (getScaledValue
// / getNormalisedValue) — the C++ NormalisableRange is the only source of range
// and skew. The FORMAT table below carries units and decimals ONLY; a JS min/max
// map would drift from the four skewed params
// (pattern_webview_knob_readout_scaled_value).
// ============================================================================

import * as Juce from "./juce/index.js";

// ── Parameter inventory (must match createParameterLayout() exactly) ────────
const KNOB_IDS = [
  "delayTime",
  "grainSize", "density",
  "feedback", "lowCut", "highCut",
  "width", "mix",
];

const COMBO_SYNC     = "syncMode";
const COMBO_DIVISION = "noteDivision";

// ── Display formatters — receive the SCALED value, add units only ───────────
const fmtPct = (v) => `${Math.round(v)} %`;
const fmtMs  = (v) => (v >= 1000 ? `${(v / 1000).toFixed(2)} s` : `${Math.round(v)} ms`);
const fmtHz  = (v) => (v >= 1000 ? `${(v / 1000).toFixed(1)} kHz` : `${Math.round(v)} Hz`);

const FORMAT = {
  delayTime: fmtMs,
  grainSize: (v) => `${Math.round(v)} ms`,
  density:   fmtPct,
  feedback:  fmtPct,
  lowCut:    fmtHz,
  highCut:   fmtHz,
  width:     fmtPct,
  mix:       fmtPct,
};

// ── Knob geometry ───────────────────────────────────────────────────────────
const KNOB_MIN_DEG   = -135;   // normalised 0.0
const KNOB_MAX_DEG   = 135;    // normalised 1.0
const DRAG_TRAVEL_PX = 220;    // vertical px for a full 0→1 sweep
const NUDGE_STEP     = 0.02;   // wheel / arrow-key increment

// ── Mutable module state ────────────────────────────────────────────────────
const sliderState = {};        // id -> Juce SliderState
let syncState     = null;      // Juce ComboBoxState (syncMode)
let divisionState = null;      // Juce ComboBoxState (noteDivision)
let paramDefaults = null;      // { id: engineeringDefault } from the native fn

// ═══════════════════════════════════════════════════════════════════════════
// Function declarations (hoisted — safe to reference from init() below)
// ═══════════════════════════════════════════════════════════════════════════

function normToDeg(n) {
  return KNOB_MIN_DEG + n * (KNOB_MAX_DEG - KNOB_MIN_DEG);
}

// Inverse of SliderState.getNormalisedValue(), using the LIVE properties pushed
// from the C++ NormalisableRange (start/end/skew) — never hardcoded ranges.
// Being JS's own inverse guarantees an exact round-trip through
// setNormalisedValue() whatever skew convention the backend uses.
function scaledToNorm(st, scaled) {
  const p = st.properties;
  const span = p.end - p.start;
  if (!isFinite(span) || span === 0) return 0;
  const proportion = Math.min(1, Math.max(0, (scaled - p.start) / span));
  return Math.pow(proportion, p.skew);
}

function updateKnobVisual(id) {
  const st = sliderState[id];
  if (!st) return;

  const knob = document.getElementById(`knob-${id}`);
  if (knob) {
    const stem = knob.querySelector(".knob-stem");
    if (stem) {
      stem.style.transform =
        `translate(-50%, -100%) rotate(${normToDeg(st.getNormalisedValue())}deg)`;
    }
    knob.setAttribute("aria-valuetext", (FORMAT[id] || String)(st.getScaledValue()));
  }

  const valEl = document.getElementById(`val-${id}`);
  if (valEl) {
    const fmt = FORMAT[id] || ((v) => v.toFixed(2));
    valEl.textContent = fmt(st.getScaledValue());   // scaled value — never a JS range map
  }
}

// One-shot fine adjust shared by wheel + arrow keys (a full bracketed gesture).
function nudge(st, delta, id) {
  const n = Math.min(1, Math.max(0, st.getNormalisedValue() + delta));
  st.sliderDragStarted();
  st.setNormalisedValue(n);
  st.sliderDragEnded();
  updateKnobVisual(id);
}

function resetToDefault(st, id) {
  if (!paramDefaults || !(id in paramDefaults)) return;
  const norm = scaledToNorm(st, Number(paramDefaults[id]));
  st.sliderDragStarted();
  st.setNormalisedValue(norm);
  st.sliderDragEnded();
  updateKnobVisual(id);
}

// ── Knob binding (relative vertical drag) ───────────────────────────────────
function bindKnob(juce, id) {
  const st = juce.getSliderState(id);
  sliderState[id] = st;

  st.valueChangedEvent.addListener(() => updateKnobVisual(id));
  st.propertiesChangedEvent.addListener(() => updateKnobVisual(id));
  updateKnobVisual(id);

  const knob = document.getElementById(`knob-${id}`);
  if (!knob) { console.error(`Missing knob element: knob-${id}`); return; }

  // Accessibility: focusable + arrow-key fine adjust.
  knob.setAttribute("tabindex", "0");
  knob.setAttribute("role", "slider");
  knob.addEventListener("keydown", (e) => {
    let delta = 0;
    if (e.key === "ArrowUp" || e.key === "ArrowRight") delta = NUDGE_STEP;
    else if (e.key === "ArrowDown" || e.key === "ArrowLeft") delta = -NUDGE_STEP;
    else return;
    nudge(st, delta, id);
    e.preventDefault();
  });

  let dragging  = false;
  let startY    = 0;
  let startNorm = 0;

  const onMove = (e) => {
    if (!dragging) return;
    const dy = startY - e.clientY;
    const n = Math.min(1, Math.max(0, startNorm + dy / DRAG_TRAVEL_PX));
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
    dragging  = true;
    startY    = e.clientY;
    startNorm = st.getNormalisedValue();
    st.sliderDragStarted();
    window.addEventListener("pointermove", onMove);
    window.addEventListener("pointerup", onUp);
    e.preventDefault();
  });

  knob.addEventListener("wheel", (e) => {
    nudge(st, e.deltaY < 0 ? NUDGE_STEP : -NUDGE_STEP, id);
    e.preventDefault();
  }, { passive: false });

  // Dblclick-reset uses the engineering default fetched from C++ — the
  // properties payload carries no default field, so a JS default table would
  // be the only alternative (and would drift).
  knob.addEventListener("dblclick", (e) => {
    resetToDefault(st, id);
    e.preventDefault();
  });
}

// ── noteDivision select ─────────────────────────────────────────────────────
function bindDivisionCombo(juce) {
  const st = juce.getComboBoxState(COMBO_DIVISION);
  divisionState = st;

  const sel = document.getElementById(`combo-${COMBO_DIVISION}`);
  if (!sel) { console.error(`Missing combo element: combo-${COMBO_DIVISION}`); return; }

  const buildOptions = () => {
    const choices = (st.properties && st.properties.choices) || [];
    if (choices.length === 0) return;
    if (sel.options.length === choices.length) return;   // already built
    sel.innerHTML = "";
    choices.forEach((c, i) => {
      const opt = document.createElement("option");
      opt.value = String(i);
      opt.textContent = c;
      sel.appendChild(opt);
    });
  };

  const refresh = () => {
    buildOptions();                        // choices may arrive after first load
    const idx = st.getChoiceIndex();
    if (idx >= 0 && idx < sel.options.length) sel.selectedIndex = idx;
  };

  st.propertiesChangedEvent.addListener(refresh);
  st.valueChangedEvent.addListener(refresh);
  refresh();

  sel.addEventListener("change", () => st.setChoiceIndex(sel.selectedIndex));
}

// ── syncMode segment pair + UI-02 time-slot swap ────────────────────────────
function bindSyncSegments(juce) {
  const st = juce.getComboBoxState(COMBO_SYNC);
  syncState = st;

  const segFree     = document.getElementById("seg-free");
  const segSync     = document.getElementById("seg-sync");
  const delayWrap   = document.getElementById("wrap-delayTime");
  const divisionWrap = document.getElementById("wrap-noteDivision");

  if (!segFree || !segSync || !delayWrap || !divisionWrap) {
    console.error("Missing syncMode / time-slot elements");
    return;
  }

  // UI-02: both controls stay relay-bound at all times — only visibility moves,
  // so neither is ever a dead control. Identical slot boxes → zero layout shift.
  const refresh = () => {
    const isSync = st.getChoiceIndex() === 1;   // { Free, Sync }, default 1 = Sync

    // Classes + aria only. The FREE / SYNC text is authored in index.html and is
    // never touched here (pattern_js_state_updater_overwrites_html_labels).
    segFree.classList.toggle("active", !isSync);
    segSync.classList.toggle("active", isSync);
    segFree.setAttribute("aria-pressed", String(!isSync));
    segSync.setAttribute("aria-pressed", String(isSync));

    divisionWrap.classList.toggle("hidden", !isSync);
    delayWrap.classList.toggle("hidden", isSync);
  };

  st.valueChangedEvent.addListener(refresh);
  st.propertiesChangedEvent.addListener(refresh);
  refresh();

  segFree.addEventListener("click", () => st.setChoiceIndex(0));
  segSync.addEventListener("click", () => st.setChoiceIndex(1));
}

// ── Defaults for dblclick-reset (the only native function) ──────────────────
async function loadParameterDefaults(juce) {
  try {
    const raw = await juce.getNativeFunction("getParameterDefaults")();
    paramDefaults = typeof raw === "string" ? JSON.parse(raw) : raw;
  } catch (e) {
    console.error("getParameterDefaults failed:", e);
    paramDefaults = null;   // dblclick becomes a no-op; every other control is unaffected
  }
}

// ── Entry point ─────────────────────────────────────────────────────────────
function init() {
  KNOB_IDS.forEach((id) => bindKnob(Juce, id));
  bindSyncSegments(Juce);
  bindDivisionCombo(Juce);
  loadParameterDefaults(Juce);   // async; nothing else depends on it
}

// Single call, at the BOTTOM of the module — every binding above is initialised.
init();
