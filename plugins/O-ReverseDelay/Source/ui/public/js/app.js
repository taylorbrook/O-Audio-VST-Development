// ============================================================================
// O-ReverseDelay — WebView UI controller (Stage 3 controls, Stage 4 bar + tips)
//
// Binds all 16 APVTS parameters two-way: 13 WebSliderRelay knobs + 3
// WebComboBoxRelay controls (syncMode as a segment pair, noteDivision and
// grainShape as selects). v1.1.0 added the four RANDOM knobs in row 2;
// v1.2.0 added the WINDOW panel's Shape select + Tilt knob.
//
// Native-function surface is 11 and must match PluginEditor.cpp exactly:
// getParameterDefaults is fetched HERE (dblclick-reset); the other ten are
// fetched by js/preset-manager.js, which this file loads dynamically. Any
// grep-diff of the bridge has to read both files.
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
  // v1.1.0 (B3) — RANDOM panel, row 2. All default to 0.
  "jitter", "delayScatter", "sizeRandom", "gainRandom",
  // v1.2.0 (B1) — WINDOW panel. grainShape is a CHOICE and is bound below as a
  // select, not here; only grainTilt is a knob.
  "grainTilt",
];

const COMBO_SYNC     = "syncMode";
const COMBO_DIVISION = "noteDivision";
const COMBO_SHAPE    = "grainShape";

// ── Display formatters — receive the SCALED value, add units only ───────────
const fmtPct = (v) => `${Math.round(v)} %`;
const fmtMs  = (v) => (v >= 1000 ? `${(v / 1000).toFixed(2)} s` : `${Math.round(v)} ms`);
const fmtHz  = (v) => (v >= 1000 ? `${(v / 1000).toFixed(1)} kHz` : `${Math.round(v)} Hz`);

// grainTilt's PARAMETER is 0..1 with 0.5 = symmetric, because that is the range
// the C++ NormalisableRange owns and 0.5 is the value whose phase warp is the
// bitwise identity. Presenting a bipolar control as "0.50" would hide that the
// centre is the neutral position, so the READOUT is signed — and derived from
// the scaled value here rather than by re-ranging the parameter, which would
// have cost the exactness at the default
// (pattern_webview_knob_readout_scaled_value: the range lives in C++, the units
// live here). U+2212 is a real minus, matching the page's en-dashes.
const fmtTilt = (v) => {
  const t = Math.round((v - 0.5) * 200);
  if (t === 0) return "Centre";
  return `${t > 0 ? "+" : "−"}${Math.abs(t)} %`;
};

const FORMAT = {
  delayTime: fmtMs,
  grainSize: (v) => `${Math.round(v)} ms`,
  density:   fmtPct,
  feedback:  fmtPct,
  lowCut:    fmtHz,
  highCut:   fmtHz,
  width:     fmtPct,
  mix:       fmtPct,
  // Scatter reads in ms and spans 0-500, so it never reaches fmtMs's second
  // branch; a plain ms formatter keeps "0 ms" from ever rendering as "0.00 s".
  jitter:       fmtPct,
  delayScatter: (v) => `${Math.round(v)} ms`,
  sizeRandom:   fmtPct,
  gainRandom:   fmtPct,
  // v1.2.0 (B1)
  grainTilt:    fmtTilt,
};

// ── Knob geometry ───────────────────────────────────────────────────────────
const KNOB_MIN_DEG   = -135;   // normalised 0.0
const KNOB_MAX_DEG   = 135;    // normalised 1.0
const DRAG_TRAVEL_PX = 220;    // vertical px for a full 0→1 sweep
const NUDGE_STEP     = 0.02;   // wheel / arrow-key increment

// ── Tooltip geometry ────────────────────────────────────────────────────────
const TOOLTIP_MARGIN   = 8;    // gap between a tip and its control / the viewport edge
const TOOLTIP_DELAY_MS = 350;  // hover dwell before a tip appears
const DELETE_ARM_MS    = 3000; // how long the delete button stays armed

// ── Mutable module state ────────────────────────────────────────────────────
// EVERY module-level binding lives in this one block — see the TDZ note above.
const sliderState = {};        // id -> Juce SliderState
let syncState     = null;      // Juce ComboBoxState (syncMode)
let divisionState = null;      // Juce ComboBoxState (noteDivision)
let shapeState    = null;      // Juce ComboBoxState (grainShape, v1.2.0)
let paramDefaults = null;      // { id: engineeringDefault } from the native fn

let presetManager = null;      // PresetManager instance (Stage 4)

let tooltipEl         = null;
let tooltipTimer      = null;
let tooltipTarget     = null;
let tooltipSuppressed = false;
let deleteArmTimer    = null;

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

// ── <select>-backed choice params (noteDivision, grainShape) ────────────────
// One implementation for both. v1.1.0 had this hard-wired to noteDivision; the
// v1.2.0 grainShape select needs identical behaviour — options built from the
// LIVE properties.choices, rebuilt when they arrive late, index refreshed on
// both events — and a second copy would be a second place for that to rot.
// Returns the state so the caller can hold it.
function bindSelectCombo(juce, paramId) {
  const st = juce.getComboBoxState(paramId);

  const sel = document.getElementById(`combo-${paramId}`);
  if (!sel) { console.error(`Missing combo element: combo-${paramId}`); return null; }

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

  return st;
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

// ═══════════════════════════════════════════════════════════════════════════
// Preset bar (Stage 4)
// ═══════════════════════════════════════════════════════════════════════════

// Two-click inline confirm. preset-manager.js otherwise falls back to the
// browser's built-in confirm dialog, which is a silent no-op or a throw in some
// JUCE WebView backends — that would make the delete leg of the bar untestable.
// Copy comes from the button's own data-label / data-confirm attributes, never
// invented here (pattern_js_state_updater_overwrites_html_labels).
function disarmDelete(btn) {
  clearTimeout(deleteArmTimer);
  btn.dataset.armed = "0";
  btn.textContent = btn.dataset.label;
}

function confirmDeleteInline(_name, _message) {
  const btn = document.getElementById("preset-delete");
  if (!btn) return false;

  if (btn.dataset.armed === "1") { disarmDelete(btn); return true; }

  btn.dataset.armed = "1";
  btn.textContent = btn.dataset.confirm;
  clearTimeout(deleteArmTimer);
  deleteArmTimer = setTimeout(() => disarmDelete(btn), DELETE_ARM_MS);
  return false;
}

// Hoisted declaration, called from inside init() — never at module top level,
// and the dynamic import() lives in here rather than at the top of the file, so
// a failure cannot escape module evaluation. The try/catch is load-bearing: it
// contains a preset-bar failure so the ten already-bound controls survive it.
async function initPresetBar() {
  try {
    const { PresetManager } = await import("./preset-manager.js");

    presetManager = new PresetManager({
      displayElement: document.getElementById("preset-name"),
      prevButton:     document.getElementById("preset-prev"),
      nextButton:     document.getElementById("preset-next"),
      saveButton:     document.getElementById("preset-save"),
      loadButton:     document.getElementById("preset-load"),
      deleteButton:   document.getElementById("preset-delete"),
      getNativeFunction: Juce.getNativeFunction,
      onConfirmDelete: confirmDeleteInline,
    });

    await presetManager.initialize();
  } catch (e) {
    console.error("[preset-bar] init failed:", e);   // the bar dies alone
  }
}

// ═══════════════════════════════════════════════════════════════════════════
// Tooltips (Stage 4) — lifted from O-MultiBandCompressor v1.4.1
// ═══════════════════════════════════════════════════════════════════════════

function handleTooltipOver(e) {
  const target = e.target.closest ? e.target.closest("[data-tip]") : null;
  if (!target || target === tooltipTarget) return;

  tooltipTarget = target;
  clearTimeout(tooltipTimer);

  if (tooltipSuppressed) return;
  tooltipTimer = setTimeout(() => showTooltip(target), TOOLTIP_DELAY_MS);
}

function handleTooltipOut(e) {
  const target = e.target.closest ? e.target.closest("[data-tip]") : null;
  if (!target) return;

  // Moving between children of the same control is not a real exit.
  if (e.relatedTarget && target.contains(e.relatedTarget)) return;

  hideTooltip();
}

function showTooltip(target) {
  // The pointer may have moved on, or gone down, during the delay.
  if (!tooltipEl || tooltipSuppressed || target !== tooltipTarget) return;

  const title = target.getAttribute("data-tip-title");
  const body  = target.getAttribute("data-tip");

  // textContent, not innerHTML — the copy stays inert.
  tooltipEl.textContent = "";

  if (title) {
    const titleEl = document.createElement("div");
    titleEl.className = "tooltip-title";
    titleEl.textContent = title;
    tooltipEl.appendChild(titleEl);
  }

  const bodyEl = document.createElement("div");
  bodyEl.className = "tooltip-body";
  bodyEl.textContent = body;
  tooltipEl.appendChild(bodyEl);

  const anchor = target.getBoundingClientRect();

  // MEASURE-THEN-PIN. A fixed-position box with `left` set and `width:auto`
  // shrinks to fit whatever space remains to its right, so measuring at the
  // PREVIOUS offset under-reports the width, and applying a near-edge `left`
  // afterwards re-wraps a 230 px tip into a ~70 px ribbon. Release the width,
  // measure from the left edge, pin the result in px, and only then place.
  // Here the exposed control is `mix` — right-most, OUTPUT panel. Invisible to
  // build, auval and pluginval (pattern_fixed_tooltip_shrink_to_fit_edge).
  tooltipEl.style.width = "";
  tooltipEl.style.left  = "0px";
  tooltipEl.style.top   = "0px";

  const width = tooltipEl.getBoundingClientRect().width;
  tooltipEl.style.width = `${width}px`;

  // Height is only stable once the width is definite.
  const height = tooltipEl.getBoundingClientRect().height;

  // Prefer above; flip below only when there is no room at the top.
  let top = anchor.top - height - TOOLTIP_MARGIN;
  let placement = "above";

  if (top < TOOLTIP_MARGIN) {
    top = anchor.bottom + TOOLTIP_MARGIN;
    placement = "below";
  }

  const anchorCentreX = anchor.left + anchor.width / 2;
  const maxLeft = window.innerWidth - width - TOOLTIP_MARGIN;
  const left = Math.max(TOOLTIP_MARGIN, Math.min(maxLeft, anchorCentreX - width / 2));

  tooltipEl.style.left = `${left}px`;
  tooltipEl.style.top  = `${top}px`;
  tooltipEl.dataset.placement = placement;

  // The tip is clamped to the viewport, but the arrow still points at the
  // control — held clear of the rounded corners.
  const arrowX = Math.max(10, Math.min(width - 10, anchorCentreX - left));
  tooltipEl.style.setProperty("--arrow-x", `${arrowX}px`);

  tooltipEl.classList.add("visible");
  tooltipEl.setAttribute("aria-hidden", "false");
}

function hideTooltip() {
  clearTimeout(tooltipTimer);
  tooltipTarget = null;

  if (!tooltipEl) return;
  tooltipEl.classList.remove("visible");
  tooltipEl.setAttribute("aria-hidden", "true");
}

function initTooltips() {
  tooltipEl = document.getElementById("tooltip");
  if (!tooltipEl) { console.warn("Tooltip element not found — tooltips disabled"); return; }

  document.addEventListener("mouseover", handleTooltipOver);
  document.addEventListener("mouseout", handleTooltipOut);

  // Any press begins a click or a drag: get the tip out of the way and keep it
  // away until release, so it cannot hang over a knob mid-drag. Capture phase,
  // because the knobs call preventDefault in their own pointerdown handlers.
  document.addEventListener("pointerdown", () => {
    tooltipSuppressed = true;
    hideTooltip();
  }, true);

  document.addEventListener("pointerup", () => { tooltipSuppressed = false; }, true);
}

// ── Entry point ─────────────────────────────────────────────────────────────
function init() {
  KNOB_IDS.forEach((id) => bindKnob(Juce, id));
  bindSyncSegments(Juce);
  divisionState = bindSelectCombo(Juce, COMBO_DIVISION);
  shapeState    = bindSelectCombo(Juce, COMBO_SHAPE);
  loadParameterDefaults(Juce);   // async; nothing else depends on it
  initTooltips();
  initPresetBar();               // async, fire-and-forget; self-contained failure
}

// Single call, at the BOTTOM of the module — every binding above is initialised.
init();
