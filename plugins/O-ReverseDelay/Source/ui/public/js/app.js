/*
   This file is part of O-ReverseDelay, an Ouaricon Audio plugin.
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
// O-ReverseDelay — WebView UI controller (Stage 3 controls, Stage 4 bar + tips)
//
// Binds all 25 APVTS parameters two-way: 20 WebSliderRelay knobs + 4
// WebComboBoxRelay controls (syncMode and sourceMode as segment pairs,
// noteDivision and grainShape as selects) + 1 WebToggleButtonRelay (freeze, as
// a segment pair).
// v1.1.0 added the four RANDOM knobs in row 2;
// v1.2.0 added the WINDOW panel's Shape select + Tilt knob; v1.3.0 added the
// COUNT panel's Count knob and the live grain meter; v1.4.0 added WINDOW's
// Taper knob and the ENVELOPE panel's window-shape display; v1.6.0 filled the
// last reserved panel with MOTION (Freeze, Direction, Regen); v1.7.0 added
// row 3 — SOURCE (segments), DUCK (one knob), DRIFT (Rate + Depth) and a
// framed, empty COLOUR reserve.
//
// THREE relay families now, and the third is the one that is easy to get wrong:
// `freeze` is an AudioParameterBool, so it has a ToggleState and NOT a
// SliderState or a ComboBoxState. getSliderState("freeze") does not fail loudly
// — it builds a state the backend never updates.
//
// Native-function surface is 15 and must match PluginEditor.cpp exactly:
// getParameterDefaults, getGrainMeter, getWindowCurve and — since v1.9.0 —
// getUiLanguage/setUiLanguage are fetched HERE; the other ten are fetched by
// js/preset-manager.js, which this file loads dynamically. Any grep-diff of the
// bridge has to read both files.
//
// NOTE what is NOT in that 15: there is no setTooltipsEnabled. D13 scoped this
// plugin's hover help to display only, and section 14 of ui_frontend_check.js
// asserts the absence by name. v1.9.0 added the language pair and nothing else.
//
// The envelope display deliberately does NOT compute the window in JS. The curve
// is fetched from C++ so there is exactly one definition of the window; a JS copy
// would be free to drift and a graph has no units to reveal it when it does.
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
// v1.9.0 — hover-help copy, English + French. A HOISTED import, which is the
// only new top-level form this file gains: section 2 of ui_frontend_check.js
// forbids any module-level declaration after the init() call at the bottom, so
// initI18n() is invoked from INSIDE init() rather than from a foot-of-file
// block. i18n.js exports only and never self-executes.
//
// scripts/check-i18n.js assertion 6 requires this line VERBATIM, single quotes
// included — it is one of the two anchors the repo-wide drift gate matches on.
import { LANGUAGES, I18N, LABELS, TIP_BINDINGS, tr } from './i18n.js';

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
  // v1.3.0 (B2) — COUNT panel. Default 8, which is v1.2.0's hard-coded overlap
  // ceiling, so an existing session opened here is unchanged.
  "grainCount",
  // v1.4.0 — WINDOW panel. Default 0.5, v1.2.0's frozen Tukey taper. Applies to
  // the Tukey shape only; bound unconditionally regardless (see the inert-cell
  // note on refreshTaperEnabled).
  "tukeyTaper",
  // v1.6.0 (B4 #2, #3) — MOTION panel. Both default to 0, which for once is
  // genuinely the no-op: all-reverse and unity loop gain. `freeze`, the third
  // control in that panel, is a BOOL and is bound through getToggleState below —
  // not here, because a bool has no SliderState and asking for one throws.
  "direction", "regenMakeup",
  // v1.7.0 (B4 #4, #6) — row 3's DUCK and DRIFT panels. All three default to
  // their no-op; driftRate's no-op is its own DEFAULT rate (0.30 Hz) rather
  // than zero, because what makes it neutral is driftDepth being 0 — writing
  // it as 0 anywhere would be clamped up to the range minimum instead.
  // `sourceMode`, the fourth new parameter, is a CHOICE and is bound below
  // through getComboBoxState — not here.
  "duck", "driftRate", "driftDepth",
  // v1.8.0 (B4 #7, #8) — row 3's COLOUR panel, the last of the v1.0.0 review's
  // section B4. Both default to 0 and both zeroes are exact no-ops: diffusion 0
  // leaves the loop's dry path at exactly 1.0, drive 0 maps to a tanh pre-gain
  // of exactly 1.0 and early-outs to the plain std::tanh the loop has always
  // called. Neither is a CHOICE or a BOOL, so neither has a counterpart below.
  "diffusion", "drive",
];

const COMBO_SYNC     = "syncMode";
const COMBO_DIVISION = "noteDivision";
const COMBO_SHAPE    = "grainShape";

// v1.7.0 (B4 #5) — rendered as a SEGMENT PAIR like syncMode, not as a select:
// it names two modes rather than picking from a list. Same ComboBoxState either
// way; only the control drawn on top of it differs.
const COMBO_SOURCE   = "sourceMode";

// v1.6.0 — the plugin's only bool parameter, and so its only ToggleState.
const TOGGLE_FREEZE  = "freeze";

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

// direction's PARAMETER is a plain 0–100 %, but the two ENDS of it are named
// states rather than amounts — 0 is "every grain reversed", 100 is "every grain
// forward" — and a readout of "0 %" hides which of those zero means. The
// endpoints therefore read as words and the interior as a percentage, which is
// the same reasoning fmtTilt uses for "Centre". Derived from the scaled value,
// never from a JS range map (pattern_webview_knob_readout_scaled_value).
const fmtDirection = (v) => {
  const p = Math.round(v);
  if (p === 0)   return "Reverse";
  if (p === 100) return "Forward";
  return `${p} %`;
};

// One decimal, matching the 0.1 dB parameter step, and an explicit "+" so the
// knob reads as makeup rather than as a level that might be cutting. 0 dB is
// written plainly: it is the shipped no-op and "+0.0 dB" would suggest the loop
// is being touched when it is not.
const fmtRegen = (v) => (v <= 0 ? "0.0 dB" : `+${v.toFixed(1)} dB`);

// duck's 0 is the shipped no-op, so it reads as a word rather than as "0 %" —
// the same reasoning fmtTilt uses for "Centre" and fmtDirection for "Reverse".
// "Off" and not "0 %" because the control is genuinely bypassed there: the wet
// multiply is exactly 1.0f, not a very small attenuation.
const fmtDuck = (v) => (v <= 0 ? "Off" : `${Math.round(v)} %`);

// driftDepth reads the same way and for the same reason — at 0 no LFO is
// evaluated at all, so "Off" is the literal truth about the engine.
const fmtDriftDepth = (v) => (v <= 0 ? "Off" : `${Math.round(v)} %`);

// driftRate spans 0.02–5 Hz, so a single format would print "0 Hz" at the bottom
// and "5.00 Hz" at the top. Two decimals below 1 Hz keeps the slow end legible —
// where the 0.01 parameter step actually resolves distinct settings — and one
// above it, where it does not. Derived from the scaled value, never from a JS
// range map (pattern_webview_knob_readout_scaled_value).
const fmtDriftRate = (v) => (v < 1 ? `${v.toFixed(2)} Hz` : `${v.toFixed(1)} Hz`);

// v1.8.0 (B4 #7, #8). Both read "Off" at 0 on the same grounds fmtDuck and
// fmtDriftDepth do, and in both cases the word is literally true of the engine
// rather than a rounding of something small:
//
//   * diffusion 0 leaves the dry term at exactly 1.0, so the allpass chain's
//     output is multiplied by exactly zero — it runs, but nothing it computes
//     reaches the loop.
//   * drive 0 maps to a pre-gain of exactly 1.0, and driveShape() branches on
//     that to call plain std::tanh — the same call the loop has made since
//     v1.0.0, not tanh(1.0·x)/1.0 evaluated the long way.
const fmtDiffusion = (v) => (v <= 0 ? "Off" : `${Math.round(v)} %`);
const fmtDrive     = (v) => (v <= 0 ? "Off" : `${Math.round(v)} %`);

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
  // v1.3.0 (B2). The parameter is a float with step 1, so the scaled value
  // arrives as 8 rather than 8.0 — Math.round guards against a host that
  // reports it a hair off the grid, which would render "7.999999".
  grainCount:   (v) => `${Math.round(v)}`,
  // v1.4.0. Two decimals, matching the 0.01 parameter step exactly: one decimal
  // would make adjacent steps read identically and the knob would look stuck.
  tukeyTaper:   (v) => v.toFixed(2),
  // v1.6.0 (B4 #2, #3) — MOTION panel.
  direction:    fmtDirection,
  regenMakeup:  fmtRegen,
  // v1.7.0 (B4 #4, #6) — DUCK and DRIFT panels.
  duck:         fmtDuck,
  driftRate:    fmtDriftRate,
  driftDepth:   fmtDriftDepth,
  // v1.8.0 (B4 #7, #8) — COLOUR panel.
  diffusion:    fmtDiffusion,
  drive:        fmtDrive,
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

// ── Grain meter poll (v1.3.0, B2) ───────────────────────────────────────────
// ~15 Hz. Fast enough that the count reads as live, slow enough that the
// message-thread round trip is nothing: the native fn does two relaxed atomic
// loads and builds a two-property object.
//
// A PULL on an interval rather than a push from a juce::Timer, because the page
// is the only thing that knows whether it is visible and it keeps the whole
// bridge inside the getNativeFunction surface the ui-stub already models.
const METER_POLL_MS = 66;

// ── Envelope display (v1.4.0) ───────────────────────────────────────────────
// Drawing constants. The curve is fetched from C++, so nothing here describes the
// window's SHAPE — only how it is painted.
const ENV_PAD       = 5;    // px inset so the curve's 0 and 1 are not on the frame
const ENV_LINE_W    = 1.6;
const ENV_REDRAW_MS = 40;   // coalescing delay while a knob is being dragged

// ── Mutable module state ────────────────────────────────────────────────────
// EVERY module-level binding lives in this one block — see the TDZ note above.
const sliderState = {};        // id -> Juce SliderState
let syncState     = null;      // Juce ComboBoxState (syncMode)
let sourceState   = null;      // Juce ComboBoxState (sourceMode, v1.7.0)
let freezeState   = null;      // Juce ToggleState (freeze, v1.6.0)
let divisionState = null;      // Juce ComboBoxState (noteDivision)
let shapeState    = null;      // Juce ComboBoxState (grainShape, v1.2.0)
let paramDefaults = null;      // { id: engineeringDefault } from the native fn

let presetManager = null;      // PresetManager instance (Stage 4)

let tooltipEl         = null;
let tooltipTimer      = null;
let tooltipTarget     = null;
let tooltipSuppressed = false;
let deleteArmTimer    = null;

let meterTimer      = null;    // setInterval handle for the grain meter poll
let meterActiveEl   = null;    // #meter-active  span
let meterOverlapEl  = null;    // #meter-overlap span
let meterFn         = null;    // the getGrainMeter native fn, resolved once
let meterInFlight   = false;   // drop a tick rather than queue behind a slow one

let envCanvas    = null;       // #envelopeCanvas
let envCtx       = null;       // its 2D context
let envCurveFn   = null;       // the getWindowCurve native fn, resolved once
let envRedrawTid = null;       // coalescing timer
let envInFlight  = false;      // as meterInFlight — never queue fetches

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

  // v1.7.2 (WR-05): terminates on cancel and lost-capture as well as on up, and
  // is idempotent (the !dragging early-return), because all four paths can fire.
  const onUp = (e) => {
    if (!dragging) return;
    dragging = false;
    st.sliderDragEnded();
    knob.removeEventListener("pointermove", onMove);
    knob.removeEventListener("pointerup", onUp);
    knob.removeEventListener("pointercancel", onUp);
    knob.removeEventListener("lostpointercapture", onUp);
    if (e && e.pointerId !== undefined) {
      try { knob.releasePointerCapture(e.pointerId); } catch (_) { /* already released */ }
    }
  };

  knob.addEventListener("pointerdown", (e) => {
    dragging  = true;
    startY    = e.clientY;
    startNorm = st.getNormalisedValue();
    st.sliderDragStarted();
    // v1.7.2 (WR-05): capture on the KNOB rather than listening on window.
    //
    // With window listeners and only a `pointerup` to end the drag, any path that
    // does not deliver that event leaves `dragging` true and both listeners
    // attached: drag out of the plugin window and release over the DAW, let the
    // host take a modal grab, or let the WebView lose focus mid-drag and the OS
    // synthesise a pointercancel instead. Two silent consequences followed —
    // every later mouse move over the page kept writing setNormalisedValue()
    // with no button held (the knob follows the cursor until the next click), and
    // sliderDragStarted() was left unmatched, so the host's parameter gesture
    // stayed open, which latches automation write in Logic and Live.
    //
    // Capturing routes every subsequent pointer event for this pointerId to the
    // knob even outside the WebView, and guarantees a terminating
    // pointerup/pointercancel/lostpointercapture. try/catch covers older
    // backends; the window path is not kept as a fallback because a partial
    // failure there is what produced the bug.
    try { knob.setPointerCapture(e.pointerId); } catch (_) { /* older backends */ }
    knob.addEventListener("pointermove", onMove);
    knob.addEventListener("pointerup", onUp);
    knob.addEventListener("pointercancel", onUp);
    knob.addEventListener("lostpointercapture", onUp);
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

// ── freeze segment pair (v1.6.0, B4 #1) ─────────────────────────────────────
// Deliberately NOT a copy of bindSyncSegments: that one drives a ComboBoxState
// through setChoiceIndex, and freeze is a ToggleState whose value is a boolean.
// The two APIs are not interchangeable — ToggleState has no getChoiceIndex — and
// a bool bound as a combo is the same class of silently dead control as an
// unregistered native function (pattern_webview_native_fn_bridge_gap).
//
// The OFF / FREEZE text is authored in index.html and is never touched here.
// Only classes and aria-pressed move (pattern_js_state_updater_overwrites_html_labels).
function bindFreezeSegments(juce) {
  const st = juce.getToggleState(TOGGLE_FREEZE);
  freezeState = st;

  const segOff = document.getElementById("seg-freeze-off");
  const segOn  = document.getElementById("seg-freeze-on");

  if (!segOff || !segOn) { console.error("Missing freeze segment elements"); return; }

  const refresh = () => {
    const frozen = st.getValue() === true;
    segOff.classList.toggle("active", !frozen);
    segOn.classList.toggle("active", frozen);
    segOff.setAttribute("aria-pressed", String(!frozen));
    segOn.setAttribute("aria-pressed", String(frozen));
  };

  st.valueChangedEvent.addListener(refresh);
  st.propertiesChangedEvent.addListener(refresh);
  refresh();

  segOff.addEventListener("click", () => { st.setValue(false); refresh(); });
  segOn.addEventListener("click", () => { st.setValue(true); refresh(); });
}

// ── sourceMode segment pair (v1.7.0, B4 #5) ─────────────────────────────────
// A ComboBoxState like syncMode's, drawn as segments rather than as a select.
// Deliberately not folded into bindSyncSegments: that one also owns the UI-02
// time-slot swap, and a shared "bind a two-segment combo" helper would either
// carry that swap as a special case or lose it.
//
// The MONO / STEREO text is authored in index.html and is never touched here —
// classes and aria-pressed only (pattern_js_state_updater_overwrites_html_labels).
function bindSourceSegments(juce) {
  const st = juce.getComboBoxState(COMBO_SOURCE);
  sourceState = st;

  const segMono   = document.getElementById("seg-source-mono");
  const segStereo = document.getElementById("seg-source-stereo");

  if (!segMono || !segStereo) { console.error("Missing sourceMode segment elements"); return; }

  const refresh = () => {
    const isStereo = st.getChoiceIndex() === 1;   // { Mono Sum, Stereo }, default 0
    segMono.classList.toggle("active", !isStereo);
    segStereo.classList.toggle("active", isStereo);
    segMono.setAttribute("aria-pressed", String(!isStereo));
    segStereo.setAttribute("aria-pressed", String(isStereo));
  };

  st.valueChangedEvent.addListener(refresh);
  st.propertiesChangedEvent.addListener(refresh);
  refresh();

  segMono.addEventListener("click", () => st.setChoiceIndex(0));
  segStereo.addEventListener("click", () => st.setChoiceIndex(1));
}

// ── Drift Rate is inert while Depth is 0 (v1.7.0, B4 #6) ────────────────────
// Same shape as refreshTaperEnabled, and for the same reason: the knob is not
// dead, it is inapplicable, and the page should say which. Class + aria only —
// the relay stays bound at all times, so a host automating driftRate while
// depth is 0 still round-trips correctly and the control lights up the moment
// depth is raised.
function refreshDriftRateEnabled() {
  const cell = document.getElementById("cell-driftRate");
  const st   = sliderState.driftDepth;
  if (!cell || !st) return;

  const live = st.getScaledValue() > 0;
  cell.classList.toggle("knob-cell-inert", !live);
  cell.setAttribute("aria-disabled", String(!live));
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
// Envelope display (v1.4.0)
// ═══════════════════════════════════════════════════════════════════════════

// Taper applies to the Tukey shape only. Dim the cell when it does not, so an
// inapplicable knob says so instead of looking live. Class + aria only — the text
// in .knob-label / .knob-value is authored in HTML and written by the shared knob
// updater respectively, and neither is touched here
// (pattern_js_state_updater_overwrites_html_labels).
function refreshTaperEnabled() {
  const cell = document.getElementById("cell-tukeyTaper");
  if (!cell || !shapeState) return;

  // 1 == WindowLut::tukey. The index comes from the C++ StringArray via the
  // relay's own choices, so it cannot drift from the enum.
  const isTukey = shapeState.getChoiceIndex() === 1;
  cell.classList.toggle("knob-cell-inert", !isTukey);
  cell.setAttribute("aria-disabled", String(!isTukey));
}

// Sizes the backing store for the device pixel ratio and returns the CSS-px box.
// Called from drawEnvelope() on every draw, so a devicePixelRatio that changed
// since the last draw is picked up without a separate resize path.
//
// This is NOT a retina-migration handler: drawEnvelope() is only reachable from
// fetchEnvelope(), which runs on a parameter change or at init. Dragging the
// window between a retina and a non-retina display without touching Shape, Tilt
// or Taper leaves the canvas at its old backing-store resolution until the next
// parameter move. v1.7.3 (IN-03/IN-04) removed the dead `envLastCurve` binding
// that made this look handled; wiring a matchMedia listener would be the fix, and
// is deliberately not shipped — it is new runtime behaviour, not a comment fix.
function envResize() {
  const dpr = window.devicePixelRatio || 1;
  const w = envCanvas.clientWidth  || 158;
  const h = envCanvas.clientHeight || 82;

  const bw = Math.round(w * dpr);
  const bh = Math.round(h * dpr);

  // Only touch width/height when they actually change — assigning to either
  // CLEARS the canvas, so an unconditional write would blank the curve on every
  // redraw and leave a flicker.
  if (envCanvas.width !== bw || envCanvas.height !== bh) {
    envCanvas.width = bw;
    envCanvas.height = bh;
  }

  envCtx.setTransform(dpr, 0, 0, dpr, 0, 0);   // draw in CSS px from here on
  return { w, h };
}

function drawEnvelope(curve) {
  if (!envCanvas || !envCtx || !curve || curve.length < 2) return;

  const { w, h } = envResize();
  envCtx.clearRect(0, 0, w, h);

  const x0 = ENV_PAD;
  const x1 = w - ENV_PAD;
  const y0 = h - ENV_PAD;          // amplitude 0
  const y1 = ENV_PAD;              // amplitude 1
  const span = x1 - x0;

  // Read the page's own palette rather than hardcoding hexes, so the plot follows
  // the aesthetic if it is ever retuned.
  const css = getComputedStyle(document.documentElement);
  const ink = css.getPropertyValue("--green-dark").trim() || "#3C5C1A";
  const rule = css.getPropertyValue("--brown-border").trim() || "#8B7355";

  // Midpoint guide — the reference that makes TILT legible. Without it a tilted
  // window just looks like a differently-shaped bump.
  envCtx.save();
  envCtx.strokeStyle = rule;
  envCtx.globalAlpha = 0.45;
  envCtx.setLineDash([2, 3]);
  envCtx.lineWidth = 1;
  envCtx.beginPath();
  envCtx.moveTo(Math.round(x0 + span / 2) + 0.5, y1);
  envCtx.lineTo(Math.round(x0 + span / 2) + 0.5, y0);
  envCtx.stroke();

  // Baseline, so amplitude 0 is a place rather than an absence.
  envCtx.setLineDash([]);
  envCtx.globalAlpha = 0.5;
  envCtx.beginPath();
  envCtx.moveTo(x0, y0 + 0.5);
  envCtx.lineTo(x1, y0 + 0.5);
  envCtx.stroke();
  envCtx.restore();

  const xAt = (i) => x0 + (span * i) / (curve.length - 1);
  const yAt = (v) => y0 + (y1 - y0) * Math.min(1, Math.max(0, v));

  // Soft fill under the curve, then the curve itself — the fill is what makes a
  // near-rectangular taper read as "more window" at a glance.
  envCtx.beginPath();
  envCtx.moveTo(x0, y0);
  for (let i = 0; i < curve.length; i += 1) envCtx.lineTo(xAt(i), yAt(curve[i]));
  envCtx.lineTo(x1, y0);
  envCtx.closePath();
  envCtx.globalAlpha = 0.16;
  envCtx.fillStyle = ink;
  envCtx.fill();

  envCtx.globalAlpha = 1;
  envCtx.beginPath();
  for (let i = 0; i < curve.length; i += 1) {
    const x = xAt(i);
    const y = yAt(curve[i]);
    if (i === 0) envCtx.moveTo(x, y);
    else envCtx.lineTo(x, y);
  }
  envCtx.strokeStyle = ink;
  envCtx.lineWidth = ENV_LINE_W;
  envCtx.lineJoin = "round";
  envCtx.stroke();
}

async function fetchEnvelope() {
  if (!envCurveFn || envInFlight) return;

  envInFlight = true;
  try {
    const raw = await envCurveFn();
    const curve = typeof raw === "string" ? JSON.parse(raw) : raw;

    if (Array.isArray(curve) && curve.length >= 2) {
      drawEnvelope(curve.map(Number));
    }
  } catch (e) {
    console.error("getWindowCurve failed:", e);   // the plot stays on its last curve
  } finally {
    envInFlight = false;
  }
}

// Coalesced: a knob drag fires valueChangedEvent per pointermove, and each fetch
// is a message-thread round trip. One redraw per ~40 ms keeps the plot feeling
// live while bounding the traffic to something a drag cannot flood.
function scheduleEnvelopeRedraw() {
  clearTimeout(envRedrawTid);
  envRedrawTid = setTimeout(fetchEnvelope, ENV_REDRAW_MS);
}

function initEnvelope(juce) {
  envCanvas = document.getElementById("envelopeCanvas");

  if (!envCanvas || typeof envCanvas.getContext !== "function") {
    console.warn("Envelope canvas not found — display disabled");
    return;
  }

  envCtx = envCanvas.getContext("2d");
  if (!envCtx) { console.warn("2D context unavailable — envelope disabled"); return; }

  try {
    envCurveFn = juce.getNativeFunction("getWindowCurve");
  } catch (e) {
    console.error("getWindowCurve unavailable:", e);
    return;   // the panel stays empty; every control is unaffected
  }

  // Redraw when any of the three parameters the window depends on moves. Bound to
  // the STATES rather than to pointer events on the knobs, so a change arriving
  // from the host — automation, a preset load, another editor — repaints too.
  ["grainTilt", "tukeyTaper"].forEach((id) => {
    const st = sliderState[id];
    if (st) {
      st.valueChangedEvent.addListener(scheduleEnvelopeRedraw);
      st.propertiesChangedEvent.addListener(scheduleEnvelopeRedraw);
    }
  });

  if (shapeState) {
    shapeState.valueChangedEvent.addListener(() => {
      refreshTaperEnabled();
      scheduleEnvelopeRedraw();
    });
    shapeState.propertiesChangedEvent.addListener(() => {
      refreshTaperEnabled();
      scheduleEnvelopeRedraw();
    });
  }

  refreshTaperEnabled();
  fetchEnvelope();   // paint immediately rather than on the first parameter move
}

// ═══════════════════════════════════════════════════════════════════════════
// Grain meter (v1.3.0, B2)
// ═══════════════════════════════════════════════════════════════════════════

// Writes the two live values. textContent on the VALUE spans only — the
// "Active" / "Overlap" captions are authored in index.html and must never be
// rewritten from here (pattern_js_state_updater_overwrites_html_labels: a
// shared updater that writes textContent on the wrong node erased HTML-authored
// labels in every DAW since launch, and reading the source did not catch it).
function renderGrainMeter(active, overlap) {
  if (meterActiveEl) {
    meterActiveEl.textContent = `${active}`;
  }
  if (meterOverlapEl) {
    // One decimal: density is a continuous knob, so overlap is genuinely
    // fractional between the integer ceilings — "5.6×" is the honest reading of
    // density 60 at count 8, and rounding it to "6×" would hide that Density
    // does anything at all between two Count settings.
    meterOverlapEl.textContent = `${overlap.toFixed(1)}×`;
  }
}

async function pollGrainMeter() {
  // Never let ticks stack: at 15 Hz a round trip that stalls would otherwise
  // queue, and the queue would drain as a burst of stale values.
  if (!meterFn || meterInFlight) return;

  meterInFlight = true;
  try {
    const raw = await meterFn();
    const m = typeof raw === "string" ? JSON.parse(raw) : raw;

    // Number() rather than trusting the payload: a var round-trip can deliver
    // these as strings depending on backend, and `overlap.toFixed` on a string
    // throws — which inside an interval callback would be a silent dead readout.
    renderGrainMeter(Number(m.active) || 0, Number(m.overlap) || 0);
  } catch (e) {
    // Stop after the first failure rather than logging 15 times a second. The
    // readout freezes on its last value; every other control is unaffected.
    console.error("getGrainMeter failed, meter stopped:", e);
    clearInterval(meterTimer);
    meterTimer = null;
  } finally {
    meterInFlight = false;
  }
}

function initGrainMeter(juce) {
  meterActiveEl  = document.getElementById("meter-active");
  meterOverlapEl = document.getElementById("meter-overlap");

  if (!meterActiveEl || !meterOverlapEl) {
    console.warn("Grain meter elements not found — meter disabled");
    return;
  }

  try {
    meterFn = juce.getNativeFunction("getGrainMeter");
  } catch (e) {
    console.error("getGrainMeter unavailable:", e);
    return;   // the readout keeps its em-dash placeholders; nothing else breaks
  }

  meterTimer = setInterval(pollGrainMeter, METER_POLL_MS);
  pollGrainMeter();   // paint once immediately rather than after the first tick
}

// ═══════════════════════════════════════════════════════════════════════════
// Preset bar (Stage 4)
// ═══════════════════════════════════════════════════════════════════════════

// Two-click inline confirm. preset-manager.js otherwise falls back to the
// browser's built-in confirm dialog, which is a silent no-op or a throw in some
// JUCE WebView backends — that would make the delete leg of the bar untestable.
// v1.10.0: the two faces are KEYS through setLabel(), not the data-label /
// data-confirm attributes they were through v1.9.0. Those attributes were the
// right answer while the page was English-only — they kept the copy out of this
// file, which is what pattern_js_state_updater_overwrites_html_labels asks for.
// They are the wrong answer once the page has two languages: an attribute holds
// ONE string, so a language switch while the button was armed would have
// restored the ENGLISH armed face. A key re-renders with the sweep.
//
// Two separate setLabel() calls in two functions, never one call with a ternary
// in its argument: check-i18n assertion 13 rejects that shape outright.
function disarmDelete(btn) {
  clearTimeout(deleteArmTimer);
  btn.dataset.armed = "0";
  setLabel(btn, "label.delete");
}

function confirmDeleteInline(_name, _message) {
  const btn = document.getElementById("preset-delete");
  if (!btn) return false;

  if (btn.dataset.armed === "1") { disarmDelete(btn); return true; }

  btn.dataset.armed = "1";
  setLabel(btn, "ui.confirm");
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
// Hover-help language (v1.9.0)
// ═══════════════════════════════════════════════════════════════════════════
//
// THIS BLOCK IS REPLICATED VERBATIM ACROSS EVERY LOCALIZED PLUGIN and is
// byte-compared (comments stripped, whitespace collapsed) against
// scripts/i18n-canon.js by scripts/check-i18n.js assertion 6. This repo has no
// shared UI module and deliberately does not gain one, so 43 hand-copies are
// only safe because a drifted copy fails a gate. Do not "tidy" it.
//
// One PULL at page init, no push, no timer, no poll().then(poll), no revision
// counter. The language is not preset content: OuariconPresetManager::loadPreset
// walks preset["parameters"] and never touches a state-tree property, so no
// preset path can change it. The pull is safe here for the reason RESEARCH B2
// establishes and this session re-confirmed — `grep -rn setVisible
// plugins/O-ReverseDelay/Source/` returns NOTHING, so the web view is never
// hidden and the hidden-completion drop cannot fire
// (critical_webview_completion_gated_on_isvisible).
//
// Declared here at module level, ABOVE every reader. The only statement
// executed at module-evaluation time is the window.__setLanguage assignment,
// which touches a hoisted function declaration and cannot enter a TDZ chain
// (pattern_module_toplevel_init_tdz). initI18n() itself is called from INSIDE
// init(), which is section 2's requirement.

let uiLanguage = 'en';
let getUiLanguageNative = null;
let setUiLanguageNative = null;

// LABELS first, I18N as the fallback: a control whose tooltip title already IS
// its label carries one key, not two copies of the same string.
function trLabel(key, lang, vars) {
    const entry = (typeof LABELS === 'object' && LABELS && LABELS[key]) || I18N[key];
    if (!entry) { console.warn(`i18n: missing label key ${key}`); return key; }
    const s = entry[lang] || entry.en;
    const resolve = (v) => {
        const nested = (typeof LABELS === 'object' && LABELS && LABELS[v]) || I18N[v];
        return nested ? String((nested[lang] || nested.en).t) : String(v);
    };
    return vars
        ? String(s.t).replace(/\{(\w+)\}/g, (m, n) => (n in vars ? resolve(vars[n]) : m))
        : String(s.t);
}

function applyLabel(el) {
    const key = el.dataset.i18n;
    if (!key) return;
    let vars = null;
    try { vars = el.dataset.i18nVars ? JSON.parse(el.dataset.i18nVars) : null; }
    catch (e) { console.warn(`i18n: bad vars on ${key}`); }
    const s = trLabel(key, uiLanguage, vars);
    el.dataset.label = s;
    el.textContent   = s;
}

function applyI18nAttributes(el) {
    const pairs = [['i18nAria', 'aria-label'], ['i18nPlaceholder', 'placeholder'], ['i18nAlt', 'alt']];
    for (const [prop, attr] of pairs) {
        const key = el.dataset[prop];
        if (key) el.setAttribute(attr, trLabel(key, uiLanguage, null));
    }
}

function setLabel(el, key, vars) {
    if (!el) return;
    el.dataset.i18n = key;
    if (vars) el.dataset.i18nVars = JSON.stringify(vars); else delete el.dataset.i18nVars;
    applyLabel(el);
}

function applyI18n(lang) {
    uiLanguage = LANGUAGES.includes(lang) ? lang : 'en';
    for (const [selector, key, wrapper, vars] of TIP_BINDINGS) {
        const el = document.querySelector(selector);
        if (!el) { console.warn(`i18n: tip target not found: ${selector}`); continue; }
        const target = wrapper ? (el.closest(wrapper) || el) : el;
        const s = tr(key, uiLanguage, vars);
        target.setAttribute('data-tip-title', s.t);
        target.setAttribute('data-tip', s.b);
    }
    for (const el of document.querySelectorAll('[data-i18n]')) applyLabel(el);
    for (const el of document.querySelectorAll('[data-i18n-aria],[data-i18n-placeholder],[data-i18n-alt]'))
        applyI18nAttributes(el);
    const sel = document.getElementById('lang-select');
    if (sel && sel.value !== uiLanguage) sel.value = uiLanguage;
}

// Exposed so a clamp gate can drive the language without teaching the ui-stub a
// promise contract: page.evaluate((l) => window.__setLanguage(l), 'fr').
window.__setLanguage = applyI18n;
// Exposed for the same reason, and so a sibling module can write a localized
// label without app.js having to export anything — O-Bitrot's controller is an
// inline <script type="module">, where an export declaration has nowhere to go.
window.__setLabel = setLabel;
window.__reapplyI18n = () => applyI18n(uiLanguage);

function initI18n() {
    try {
        getUiLanguageNative = Juce.getNativeFunction('getUiLanguage');
        setUiLanguageNative = Juce.getNativeFunction('setUiLanguage');
    } catch (e) {
        console.warn('Language preference not available, session-only:', e);
    }

    // Paint the default SYNCHRONOUSLY first. Never blank, never a flash.
    try { applyI18n('en'); } catch (e) { console.error('i18n init failed:', e); }

    if (getUiLanguageNative) {
        getUiLanguageNative()
            .then((code) => applyI18n(code === 'fr' ? 'fr' : 'en'))
            .catch((e) => console.warn('Could not read language preference:', e));
    }

    const sel = document.getElementById('lang-select');
    if (sel) sel.addEventListener('change', (e) => {
        applyI18n(e.target.value);
        if (setUiLanguageNative) setUiLanguageNative(uiLanguage).catch(() => {});
    });
}

// ── The settings popover (v1.9.0) ──────────────────────────────────────────
//
// The gear that carries the language selector. ONE ROW — D13 scoped this plugin
// to display-only hover help, so there is deliberately no on/off toggle and no
// setTooltipsEnabled native function, which section 14 of ui_frontend_check.js
// asserts by name.
//
// All state lives in this closure, so nothing here can join a TDZ chain.

function initSettingsPopover() {
  const gearBtn = document.getElementById("gear-btn");
  const popover = document.getElementById("settings-popover");

  if (gearBtn === null || popover === null) {
    console.warn("settings popover missing — language selector unavailable");
    return;
  }

  const setOpen = (open) => {
    popover.hidden = !open;
    gearBtn.setAttribute("aria-expanded", open ? "true" : "false");
  };

  gearBtn.addEventListener("click", (e) => {
    e.stopPropagation();
    setOpen(popover.hidden);
  });

  // Dismiss on a press anywhere else, and on Escape. mousedown rather than
  // click, so the panel is gone before a drag on a knob underneath it begins —
  // the knobs call preventDefault in their own pointerdown handlers.
  document.addEventListener("mousedown", (e) => {
    if (popover.hidden) return;
    if (popover.contains(e.target) || gearBtn.contains(e.target)) return;
    setOpen(false);
  });

  document.addEventListener("keydown", (e) => {
    if (e.key === "Escape" && !popover.hidden) {
      setOpen(false);
      gearBtn.focus();
    }
  });
}

// ═══════════════════════════════════════════════════════════════════════════
// Tooltips (Stage 4) — lifted from O-MultiBandCompressor v1.4.1
//
// v1.9.0: the renderer is UNCHANGED. It still reads data-tip-title / data-tip
// off the anchor; those two attributes are simply written by applyI18n() above
// rather than authored in index.html. There is no data-tip-key attribute and
// this function never sees a key.
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
  bindFreezeSegments(Juce);      // v1.6.0 (B4 #1); the only ToggleState
  bindSourceSegments(Juce);      // v1.7.0 (B4 #5)
  divisionState = bindSelectCombo(Juce, COMBO_DIVISION);
  shapeState    = bindSelectCombo(Juce, COMBO_SHAPE);

  // v1.7.0: Drift Rate dims while Depth is 0. AFTER the bindKnob loop above,
  // which is what creates sliderState.driftDepth — ordinary ordering, not the
  // TDZ kind: the binding exists from the top of the file, it is just empty
  // until the binder runs.
  {
    const st = sliderState.driftDepth;
    if (st) {
      st.valueChangedEvent.addListener(refreshDriftRateEnabled);
      st.propertiesChangedEvent.addListener(refreshDriftRateEnabled);
    }
    refreshDriftRateEnabled();
  }

  loadParameterDefaults(Juce);   // async; nothing else depends on it

  // v1.9.0. BEFORE initTooltips(): applyI18n() is what puts data-tip on the
  // anchors in the first place, and the renderer's delegated listener resolves
  // e.target.closest("[data-tip]") at hover time — but a first hover landing in
  // the window between the two would find no anchor at all. Ordering here is
  // load-bearing in the ordinary way, not the TDZ way.
  //
  // Each inside its own try/catch: a translation-table typo must not take the
  // 20 bound knobs down with it, which is exactly what the MBC v1.4.0 TDZ throw
  // did to unrelated working controls while build, auval and every static check
  // still passed.
  try { initSettingsPopover(); } catch (e) { console.error("settings popover init failed:", e); }
  try { initI18n(); }           catch (e) { console.error("i18n init failed:", e); }

  initTooltips();
  initGrainMeter(Juce);          // v1.3.0 (B2); self-contained failure
  // AFTER bindKnob/bindSelectCombo above: it subscribes to sliderState[...] and
  // shapeState, which those calls create. Ordering here is load-bearing in the
  // ordinary way, not the TDZ way — the bindings exist, they are just empty until
  // the binders run.
  initEnvelope(Juce);            // v1.4.0; self-contained failure
  initPresetBar();               // async, fire-and-forget; self-contained failure
}

// Single call, at the BOTTOM of the module — every binding above is initialised.
init();
