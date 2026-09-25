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
// Binds all 27 APVTS parameters two-way: 22 WebSliderRelay knobs + 4
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
// Native-function surface is 15 (+ A/B 4 + categories 1 = 20) and must match
// PluginEditor.cpp exactly: getParameterDefaults, getGrainMeter, getWindowCurve
// and v1.9.0's getUiLanguage/setUiLanguage are fetched HERE (v1.21.0 removed
// v1.16.0's getMixLock/setMixLock with the padlock); the other ten are fetched by
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
const COMBO_FREEZE_LENGTH = "freezeLength";   // v1.15.0: select in MOTION
// v1.17.0: two params, ONE control — the chain glyph beside GRAIN's Size caption.
const COMBO_GRAIN_LINK     = "grainLink";
const COMBO_GRAIN_DIVISION = "grainDivision";

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

// duck, driftDepth, diffusion and drive all read "Off" at 0 rather than "0 %" —
// the same reasoning fmtTilt uses for "Centre" and fmtDirection for "Reverse".
// In all four the word is literally true of the engine rather than a rounding
// of something small:
//
//   * duck 0: the wet multiply is exactly 1.0f, not a very small attenuation.
//   * driftDepth 0: no LFO is evaluated at all.
//   * diffusion 0 (v1.8.0) leaves the dry term at exactly 1.0, so the allpass
//     chain's output is multiplied by exactly zero — it runs, but nothing it
//     computes reaches the loop.
//   * drive 0 (v1.8.0) maps to a pre-gain of exactly 1.0, and driveShape()
//     branches on that to call plain std::tanh — the same call the loop has
//     made since v1.0.0, not tanh(1.0·x)/1.0 evaluated the long way.
const fmtPctOrOff = (v) => (v <= 0 ? "Off" : `${Math.round(v)} %`);

// grainSize and delayScatter: plain integer ms. Scatter spans 0-500, so it never
// reaches fmtMs's second branch; this keeps "0 ms" from ever rendering "0.00 s".
const fmtMsInt = (v) => `${Math.round(v)} ms`;

// driftRate spans 0.02–5 Hz, so a single format would print "0 Hz" at the bottom
// and "5.00 Hz" at the top. Two decimals below 1 Hz keeps the slow end legible —
// where the 0.01 parameter step actually resolves distinct settings — and one
// above it, where it does not. Derived from the scaled value, never from a JS
// range map (pattern_webview_knob_readout_scaled_value).
const fmtDriftRate = (v) => (v < 1 ? `${v.toFixed(2)} Hz` : `${v.toFixed(1)} Hz`);

const FORMAT = {
  delayTime: fmtMs,
  grainSize: fmtMsInt,
  density:   fmtPct,
  feedback:  fmtPct,
  lowCut:    fmtHz,
  highCut:   fmtHz,
  width:     fmtPct,
  mix:       fmtPct,
  jitter:       fmtPct,
  delayScatter: fmtMsInt,
  sizeRandom:   fmtPct,
  gainRandom:   fmtPct,
  // v1.2.0 (B1)
  grainTilt:    fmtTilt,
  // v1.3.0 (B2). The parameter is a float with step 1, so the scaled value
  // arrives as 8 rather than 8.0 — Math.round guards against a host that
  // reports it a hair off the grid, which would render "7.999999".
  // v1.14.0: "8×", not "8" — it sits beside the Overlap readout ("5.6×") and
  // is the ceiling that readout is measured against, so they share a unit.
  grainCount:   (v) => `${Math.round(v)}×`,
  // v1.4.0. Two decimals, matching the 0.01 parameter step exactly: one decimal
  // would make adjacent steps read identically and the knob would look stuck.
  tukeyTaper:   (v) => v.toFixed(2),
  // v1.6.0 (B4 #2, #3) — MOTION panel.
  direction:    fmtDirection,
  regenMakeup:  fmtRegen,
  // v1.7.0 (B4 #4, #6) — DUCK and DRIFT panels.
  duck:         fmtPctOrOff,
  driftRate:    fmtDriftRate,
  driftDepth:   fmtPctOrOff,
  // v1.8.0 (B4 #7, #8) — COLOUR panel.
  diffusion:    fmtPctOrOff,
  drive:        fmtPctOrOff,
};

// ── Knob geometry ───────────────────────────────────────────────────────────
const KNOB_MIN_DEG   = -135;   // normalised 0.0
const KNOB_MAX_DEG   = 135;    // normalised 1.0
const DRAG_TRAVEL_PX = 220;    // vertical px for a full 0→1 sweep
const FINE_DRAG_RATE = 0.2;    // v1.14.0: Shift-drag speed (a full sweep = 1100 px)
const NUDGE_STEP     = 0.02;   // wheel / arrow-key increment (floored at one param step)

// v1.12.3: wheel scaling and gesture hold.
//
// One wheel event is AT LEAST one nudge — the behaviour every backend had
// before — and a larger delta (a fast flick, or a backend reporting lines or
// pages) scales up to WHEEL_MAX_NUDGES. The floor is deliberate: WebKit's
// per-notch pixel delta varies with acceleration, and a pure proportional map
// would make a slow notch move a continuous knob less than it did in v1.12.2.
//
// A burst of ticks is ONE automation gesture, closed WHEEL_GESTURE_MS after the
// last tick, rather than a begin/end pair per tick: Logic and Live write one
// automation touch per gesture, so the old shape left a comb of tiny touches.
const WHEEL_PX_PER_NUDGE = 100;  // pixel delta worth one nudge
const WHEEL_LINE_PX      = 33;   // deltaMode 1 (lines) → px
const WHEEL_PAGE_PX      = 400;  // deltaMode 2 (pages) → px
const WHEEL_MAX_NUDGES   = 4;    // per-event clamp
const WHEEL_GESTURE_MS   = 250;

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

// ── Output level meter (v1.14.0) ────────────────────────────────────────────
// Linear peaks arrive from getGrainMeter (peak since the previous poll). The bar
// is drawn in dBFS over LEVEL_FLOOR_DB..LEVEL_CEIL_DB so the region above 0 dB —
// where Regen can put the output — is visible rather than pinned at full.
const LEVEL_FLOOR_DB     = -48;
const LEVEL_CEIL_DB      = 6;
const LEVEL_FALL_DB      = 1.5;    // bar release per poll (~23 dB/s at 66 ms)
const LEVEL_HOLD_MS      = 1500;   // peak-hold tick dwell before it falls
const LEVEL_CLIP_LINEAR  = 1.0;    // lamp latches above 0 dBFS

// ── Envelope display (v1.4.0) ───────────────────────────────────────────────
// Drawing constants. The curve is fetched from C++, so nothing here describes the
// window's SHAPE — only how it is painted.
const ENV_PAD       = 5;    // px inset so the curve's 0 and 1 are not on the frame
const ENV_LINE_W    = 1.6;
const ENV_REDRAW_MS = 40;   // coalescing delay while a knob is being dragged

// ── Grain view (v1.21.0) ────────────────────────────────────────────────────
// The OUTPUT panel's live grain cloud. Data arrives with the 15 Hz meter poll;
// between polls each grain is advanced analytically at requestAnimationFrame
// (phase grows 1/length per ms; a reverse grain's read point ages 2 ms per ms,
// a forward grain's stays put), so motion is smooth without a faster bridge.
const GV_PAD          = 5;      // px inset inside the canvas border
const GV_MIN_AXIS_MS  = 200;    // shortest time axis, so a tiny delay still spreads out
const GV_AXIS_HEAD    = 1.08;   // axis headroom over the furthest grain's end of life
const GV_AXIS_GROW    = 0.25;   // per-frame easing when the axis must grow (never clip)
const GV_AXIS_SHRINK  = 0.03;   // ... and when it may shrink (slow, so it doesn't pump)
const GV_BAR_H        = 3;      // pill height, px
const GV_DOT_R_MIN    = 1.8;    // playhead radius at the quietest level
const GV_DOT_R_MAX    = 3.6;    // ... and the loudest
const GV_STALE_MS     = 400;    // a snapshot this old with no new block = host idle
// v1.21.1: darkness follows the grain's AMPLITUDE — the level of the material it
// is reading × where it is in its window × its Gain RND level — on a dB scale.
// At or below the floor a grain is drawn as a hollow ring (it is running, but
// reading silence); at the ceiling it is solid dark green.
const GV_FLOOR_DB     = -54;
const GV_CEIL_DB      = -6;

// ── Mutable module state ────────────────────────────────────────────────────
// EVERY module-level binding lives in this one block — see the TDZ note above.
// The syncMode, sourceMode, freeze and noteDivision states are held only by
// their binders' closures — nothing outside them reads those states.
const sliderState = {};        // id -> Juce SliderState
let shapeState    = null;      // Juce ComboBoxState (grainShape, v1.2.0)
let paramDefaults = null;      // { id: engineeringDefault } from the native fn

let presetManager = null;      // PresetManager instance (Stage 4)

let tooltipEl         = null;
let tooltipTimer      = null;
let tooltipTarget     = null;
let tooltipSuppressed = false;
let deleteArmTimer    = null;
let deleteGateToken   = 0;       // v1.12.3: discards stale isFactoryPreset answers
let tipsEnabled       = true;    // v1.11.0 hover-help switch — see applyTipsEnabled()

let meterTimer      = null;    // setInterval handle for the grain meter poll
let meterActiveEl   = null;    // #meter-active  span
let meterOverlapEl  = null;    // #meter-overlap span
let meterFn         = null;    // the getGrainMeter native fn, resolved once
let meterInFlight   = false;   // drop a tick rather than queue behind a slow one
let meterDelayEl    = null;    // #effective-delay (v1.13.0)
let freezeEngaged   = null;    // v1.13.0: the latch, from the meter; null until the first poll
let repaintFreeze   = null;    // bindFreezeSegments' refresh, re-run when freezeEngaged moves

let levelOutEl   = null;       // #level-out  (v1.14.0)
let levelInEl    = null;       // #level-in
let levelHoldEl  = null;       // #level-hold
let clipLampEl   = null;       // #clip-lamp
let levelOutDb   = LEVEL_FLOOR_DB;   // displayed bar, after release ballistics
let levelInDb    = LEVEL_FLOOR_DB;
let levelHoldDb  = LEVEL_FLOOR_DB;
let levelHoldAt  = 0;          // performance.now() when the hold tick was set

let envCanvas    = null;       // #envelopeCanvas
let envCtx       = null;       // its 2D context
let envCurveFn   = null;       // the getWindowCurve native fn, resolved once
let envRedrawTid = null;       // coalescing timer
let envInFlight  = false;      // as meterInFlight — never queue fetches

let gvCanvas     = null;       // #grainCanvas (v1.21.0)
let gvCtx        = null;
let gvGrains     = [];         // last snapshot: { age, len, phase, pan, level, fwd, peak }
let gvAnchorAt   = 0;          // performance.now() when gvGrains was taken
let gvSeq        = -1;         // grainSeq of gvGrains; unchanged = no new block
let gvDelayMs    = 0;          // from the meter, for the delay guide
let gvAxisMs     = 0;          // eased time-axis length
let gvCurve      = null;       // the window curve (shared with the envelope plot)
let gvColors     = null;       // palette, read once from the CSS tokens
let gvDrewEmpty  = false;      // skip redrawing an already-empty frame

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

// FORMAT[id] is called without a fallback: section 15 of ui_frontend_check.js
// requires a FORMAT entry for every KNOB_IDS member, so a missing one fails the
// gate rather than rendering a unitless number.
function updateKnobVisual(id) {
  const st = sliderState[id];
  if (!st) return;

  const text = FORMAT[id](st.getScaledValue());   // scaled value — never a JS range map

  const knob = document.getElementById(`knob-${id}`);
  if (knob) {
    const stem = knob.querySelector(".knob-stem");
    if (stem) {
      stem.style.transform =
        `translate(-50%, -100%) rotate(${normToDeg(st.getNormalisedValue())}deg)`;
    }
    knob.setAttribute("aria-valuetext", text);
  }

  const valEl = document.getElementById(`val-${id}`);
  if (valEl) valEl.textContent = text;
}

// v1.12.3: the normalised size of one nudge — NUDGE_STEP, floored at one
// parameter step. setNormalisedValue() snaps to the range's interval, so a move
// smaller than half a step rounds straight back: on grainCount (2–16, step 1)
// 0.02 is 0.28 of a step and the wheel and arrows did nothing at all. Every
// other knob's step is under 0.001 normalised, so the floor only bites where
// the knob was stuck. interval / span is exact for grainCount because it is
// linear; the skewed ranges all have 0.01 steps over spans of 100+, where the
// floor never engages.
function nudgeStep(st) {
  const p = st.properties;
  const span = p.end - p.start;
  if (!(p.interval > 0) || !isFinite(span) || span <= 0) return NUDGE_STEP;
  return Math.max(NUDGE_STEP, p.interval / span);
}

// Move by `nudges` nudge-steps (signed). Unbracketed — the caller owns the gesture.
function stepBy(st, nudges, id) {
  const n = Math.min(1, Math.max(0, st.getNormalisedValue() + nudges * nudgeStep(st)));
  st.setNormalisedValue(n);
  updateKnobVisual(id);
}

// One-shot fine adjust for the arrow keys (a full bracketed gesture).
function nudge(st, dir, id) {
  st.sliderDragStarted();
  stepBy(st, dir, id);
  st.sliderDragEnded();
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
    let dir = 0;
    if (e.key === "ArrowUp" || e.key === "ArrowRight") dir = 1;
    else if (e.key === "ArrowDown" || e.key === "ArrowLeft") dir = -1;
    else return;
    endWheelGesture();
    nudge(st, dir, id);
    e.preventDefault();
  });

  let dragging  = false;
  let startY    = 0;
  let startNorm = 0;
  let lastY     = 0;       // v1.14.0: previous move's Y, the re-base point
  let fine      = false;   // v1.14.0: Shift held for the current drag segment

  // v1.12.3: the open wheel gesture, if any. Every other interaction on this
  // knob closes it first, so a key, drag or double-click never nests its own
  // begin/end inside it.
  let wheelTimer = null;
  const endWheelGesture = () => {
    if (wheelTimer === null) return;
    clearTimeout(wheelTimer);
    wheelTimer = null;
    st.sliderDragEnded();
  };

  // v1.14.0: Shift drags at FINE_DRAG_RATE. The drag is absolute from an origin
  // (startY, startNorm), so a bare rate switch would jump the knob by the whole
  // distance travelled so far times the rate change. Instead the origin is
  // re-based to where the knob IS whenever the modifier flips — at the previous
  // move's Y, so this event's motion is applied at the new rate, not dropped.
  const onMove = (e) => {
    if (!dragging) return;
    if (e.shiftKey !== fine) {
      fine      = e.shiftKey;
      startY    = lastY;
      startNorm = st.getNormalisedValue();
    }
    lastY = e.clientY;
    const travel = fine ? DRAG_TRAVEL_PX / FINE_DRAG_RATE : DRAG_TRAVEL_PX;
    const dy = startY - e.clientY;
    const n = Math.min(1, Math.max(0, startNorm + dy / travel));
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
    endWheelGesture();
    dragging  = true;
    startY    = e.clientY;
    lastY     = e.clientY;
    fine      = e.shiftKey;
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

  // v1.12.3: horizontal-dominant events are not ours — up to v1.12.2 a sideways
  // trackpad swipe (deltaY 0) read as "down" and walked the knob to its floor.
  // Not preventDefault()ed, so they reach whatever else wants them. Ignored
  // mid-drag too: the drag already holds the gesture, and a tick there used to
  // close it early.
  knob.addEventListener("wheel", (e) => {
    if (dragging) { e.preventDefault(); return; }
    if (Math.abs(e.deltaX) >= Math.abs(e.deltaY)) return;
    e.preventDefault();

    const px = Math.abs(e.deltaY) *
      (e.deltaMode === 1 ? WHEEL_LINE_PX : e.deltaMode === 2 ? WHEEL_PAGE_PX : 1);
    const nudges = Math.min(WHEEL_MAX_NUDGES, Math.max(1, px / WHEEL_PX_PER_NUDGE));

    if (wheelTimer === null) st.sliderDragStarted();
    else clearTimeout(wheelTimer);
    wheelTimer = setTimeout(endWheelGesture, WHEEL_GESTURE_MS);

    stepBy(st, e.deltaY < 0 ? nudges : -nudges, id);
  }, { passive: false });

  // Dblclick-reset uses the engineering default fetched from C++ — the
  // properties payload carries no default field, so a JS default table would
  // be the only alternative (and would drift).
  knob.addEventListener("dblclick", (e) => {
    endWheelGesture();
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
//
// v1.15.0: optional `labelers` — one function per option, for choices that are
// WORDS (freezeLength) rather than proper names (grainShape, divisions). Each
// calls setLabel with a LITERAL key (check-i18n [13] rejects a computed one),
// so the option carries data-i18n and applyI18n() relabels it on a language
// change like any other caption; the C++ string is only the pre-i18n fallback.
function bindSelectCombo(juce, paramId, labelers) {
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
      if (labelers && labelers[i]) labelers[i](opt);
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

// ── Segment pairs and inapplicable cells: the shared painters ───────────────
// Classes and aria ONLY. Every segment caption (FREE / SYNC, OFF / FREEZE,
// MONO / STEREO) and every knob label is authored in index.html, and nothing
// here may write textContent (pattern_js_state_updater_overwrites_html_labels).
function paintSegmentPair(offSeg, onSeg, on) {
  offSeg.classList.toggle("active", !on);
  onSeg.classList.toggle("active", on);
  offSeg.setAttribute("aria-pressed", String(!on));
  onSeg.setAttribute("aria-pressed", String(on));
}

// Dims a cell whose control does not apply in the current state. The relay stays
// bound and pointer-events stay on — see .knob-cell-inert in styles.css.
function setCellApplicable(cell, applicable) {
  cell.classList.toggle("knob-cell-inert", !applicable);
  cell.setAttribute("aria-disabled", String(!applicable));
}

// A var round-trip delivers a native function's result as either a JSON string
// or an already-parsed value, depending on backend.
function parseNativeResult(raw) {
  return typeof raw === "string" ? JSON.parse(raw) : raw;
}

// ── syncMode segment pair + UI-02 time-slot swap ────────────────────────────
function bindSyncSegments(juce) {
  const st = juce.getComboBoxState(COMBO_SYNC);

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
    paintSegmentPair(segFree, segSync, isSync);

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

  const segOff = document.getElementById("seg-freeze-off");
  const segOn  = document.getElementById("seg-freeze-on");

  if (!segOff || !segOn) { console.error("Missing freeze segment elements"); return; }

  // v1.13.0: ON but not yet engaged paints as armed. null (no poll yet) is
  // not false, so an editor opening on an engaged hold never pulses first.
  const refresh = () => {
    const on = st.getValue() === true;
    paintSegmentPair(segOff, segOn, on);
    segOn.classList.toggle("armed", on && freezeEngaged === false);
  };
  repaintFreeze = refresh;

  st.valueChangedEvent.addListener(refresh);
  st.propertiesChangedEvent.addListener(refresh);
  refresh();

  segOff.addEventListener("click", () => { st.setValue(false); refresh(); });
  segOn.addEventListener("click", () => { st.setValue(true); refresh(); });
}

// ── A/B compare + Randomise (v1.18.0) ───────────────────────────────────────
// Four native functions, all answering { active, filledA, filledB }. The slots
// live in the processor (they outlive this page), so the page PULLS once at
// init and repaints from every answer — it never holds its own copy of the
// state. Recall and Randomise move parameters through setValueNotifyingHost,
// so the knobs follow through their relays; only the preset NAME needs a
// nudge, and presetManager.refresh() is the module's own way to re-read it.
//
// #ab-copy's face is the one piece of text written here. It is a glyph pair
// (A→B / B→A), carries no data-i18n, and so is outside applyLabel()'s sweep
// (pattern_js_state_updater_overwrites_html_labels).
function initAbCompare(juce) {
  const btnA    = document.getElementById("ab-a");
  const btnB    = document.getElementById("ab-b");
  const btnCopy = document.getElementById("ab-copy");
  const btnRnd  = document.getElementById("ab-random");
  if (!btnA || !btnB || !btnCopy || !btnRnd) { console.error("Missing A/B elements"); return; }

  let getFn = null, selectFn = null, copyFn = null, randomFn = null;
  try {
    getFn    = juce.getNativeFunction("getAbState");
    selectFn = juce.getNativeFunction("abSelect");
    copyFn   = juce.getNativeFunction("abCopy");
    randomFn = juce.getNativeFunction("randomise");
  } catch (e) {
    console.warn("A/B not available:", e);
    return;
  }

  let busy = false;   // one bridge round-trip at a time: a double click must not interleave two recalls

  const paint = (raw) => {
    const s = parseNativeResult(raw);
    if (!s || typeof s !== "object") return;
    const onB = s.active === 1;
    btnA.classList.toggle("active", !onB);
    btnB.classList.toggle("active", onB);
    btnA.setAttribute("aria-pressed", String(!onB));
    btnB.setAttribute("aria-pressed", String(onB));
    btnCopy.textContent = onB ? "B→A" : "A→B";
  };

  const run = (fn, refreshName, ...args) => {
    if (busy) return;
    busy = true;
    fn(...args)
      .then((raw) => {
        paint(raw);
        if (refreshName && presetManager) return presetManager.refresh();
      })
      .catch((e) => console.warn("A/B call failed:", e))
      .finally(() => { busy = false; });
  };

  getFn().then(paint).catch((e) => console.warn("Could not read A/B state:", e));

  btnA.addEventListener("click", () => run(selectFn, true, 0));
  btnB.addEventListener("click", () => run(selectFn, true, 1));
  btnCopy.addEventListener("click", () => run(copyFn, false));
  btnRnd.addEventListener("click", () => run(randomFn, false));
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

  const segMono   = document.getElementById("seg-source-mono");
  const segStereo = document.getElementById("seg-source-stereo");

  if (!segMono || !segStereo) { console.error("Missing sourceMode segment elements"); return; }

  // { Mono Sum, Stereo }, default 0
  const refresh = () => paintSegmentPair(segMono, segStereo, st.getChoiceIndex() === 1);

  st.valueChangedEvent.addListener(refresh);
  st.propertiesChangedEvent.addListener(refresh);
  refresh();

  segMono.addEventListener("click", () => st.setChoiceIndex(0));
  segStereo.addEventListener("click", () => st.setChoiceIndex(1));
}

// ── Grain Link (v1.17.0) ────────────────────────────────────────────────────
// grainLink (Free / = Delay / Division) and grainDivision share one native
// <select> laid over the chain glyph: "free", "delay", a disabled "Division"
// heading, then "div-N" for every kNoteDivisions entry. A heading OPTION rather
// than an <optgroup>: its caption is text, so setLabel/applyI18n relabel it like
// any other, where an optgroup's label attribute would need its own i18n path. Picking a division writes
// grainDivision first and then grainLink, so the engine never sees Division
// with the previous note value for longer than one block.
//
// Both relays stay bound at all times and the Size knob stays adjustable while
// linked: it is the Free value, and Division's no-tempo fallback. Classes, aria
// and the readout span only — the SIZE caption is authored in index.html.
let grainLinkState   = null;
let grainDivState    = null;
let grainLinkBox     = null;
let grainKnobEl      = null;
let grainValEl       = null;
let grainLinkedEl    = null;
let grainMeterMs     = 0;
let grainMeterSource = "free";

function bindGrainLink(juce) {
  grainLinkState = juce.getComboBoxState(COMBO_GRAIN_LINK);
  grainDivState  = juce.getComboBoxState(COMBO_GRAIN_DIVISION);

  const sel     = document.getElementById("grain-link-menu");
  grainLinkBox  = document.getElementById("grain-link");
  grainKnobEl   = document.getElementById("knob-grainSize");
  grainValEl    = document.getElementById("val-grainSize");
  grainLinkedEl = document.getElementById("grain-linked");

  if (!sel || !grainLinkBox || !grainKnobEl || !grainValEl || !grainLinkedEl) {
    console.error("Missing Grain Link elements");
    grainLinkState = null;
    return;
  }

  // Built from grainDivision's LIVE choices, like bindSelectCombo — rebuilt if
  // they arrive late. Free / = Delay / Division are words, so each carries a
  // literal LABELS key; the note names are proper names and are not localized.
  const buildOptions = () => {
    const divs = (grainDivState.properties && grainDivState.properties.choices) || [];
    if (divs.length === 0 || sel.options.length === divs.length + 3) return;
    sel.innerHTML = "";
    const add = (parent, value, text) => {
      const opt = document.createElement("option");
      opt.value = value;
      opt.textContent = text;
      parent.appendChild(opt);
      return opt;
    };
    setLabel(add(sel, "free", "Free"), "opt.grainLink.free");
    setLabel(add(sel, "delay", "= Delay"), "opt.grainLink.delay");
    const heading = add(sel, "", "Division");
    heading.disabled = true;
    setLabel(heading, "opt.grainLink.division");
    divs.forEach((d, i) => add(sel, `div-${i}`, d));
  };

  const refresh = () => {
    buildOptions();
    const link = grainLinkState.getChoiceIndex();
    sel.value = link === 1 ? "delay"
              : link === 2 ? `div-${grainDivState.getChoiceIndex()}`
              : "free";
    grainLinkBox.classList.toggle("active", link !== 0);
    paintGrainReadout();
  };

  for (const st of [grainLinkState, grainDivState]) {
    st.propertiesChangedEvent.addListener(refresh);
    st.valueChangedEvent.addListener(refresh);
  }

  sel.addEventListener("change", () => {
    const v = sel.value;
    if (v.startsWith("div-")) {
      grainDivState.setChoiceIndex(Number(v.slice(4)));
      grainLinkState.setChoiceIndex(2);
    } else {
      grainLinkState.setChoiceIndex(v === "delay" ? 1 : 0);
    }
  });

  refresh();
}

// Linked: the dial dims and the readout shows the engine's G. Division with no
// host tempo is playing the knob, so the dial is live again and the readout
// warns — as does a division pinned at 50 or 4000 ms. The number comes from
// getGrainMeter; until a linked value arrives the readout keeps what it had.
function paintGrainReadout() {
  if (!grainLinkState) return;
  const linked   = grainLinkState.getChoiceIndex() !== 0;
  const knobLive = !linked || grainMeterSource === "fallback";

  grainValEl.classList.toggle("hidden", linked);
  grainLinkedEl.classList.toggle("hidden", !linked);
  grainKnobEl.classList.toggle("knob-dial-inert", !knobLive);
  grainKnobEl.setAttribute("aria-disabled", String(!knobLive));

  if (linked && grainMeterSource !== "free" && grainMeterMs > 0)
    grainLinkedEl.textContent = `= ${fmtMs(grainMeterMs)}`;
  grainLinkedEl.classList.toggle("warn",
    linked && (grainMeterSource === "fallback" || grainMeterSource === "clamped"));
}

function renderGrainLink(ms, source) {
  grainMeterMs     = ms;
  grainMeterSource = source;
  paintGrainReadout();
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

  setCellApplicable(cell, st.getScaledValue() > 0);
}

// ── Defaults for dblclick-reset (getParameterDefaults) ──────────────────────
async function loadParameterDefaults(juce) {
  try {
    const raw = await juce.getNativeFunction("getParameterDefaults")();
    paramDefaults = parseNativeResult(raw);
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
  setCellApplicable(cell, shapeState.getChoiceIndex() === 1);
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
    const curve = parseNativeResult(raw);

    if (Array.isArray(curve) && curve.length >= 2) {
      gvCurve = curve.map(Number);   // v1.21.0: the grain view fades by the same window
      drawEnvelope(gvCurve);
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
    const onShapeChanged = () => {
      refreshTaperEnabled();
      scheduleEnvelopeRedraw();
    };
    shapeState.valueChangedEvent.addListener(onShapeChanged);
    shapeState.propertiesChangedEvent.addListener(onShapeChanged);
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

// v1.13.0: the delay the engine is playing. Same fmtMs as the Delay knob, so a
// Sync readout and a Free readout of the same time are the same string. Only
// the value span is written; the Division caption is authored in index.html.
function renderEffectiveDelay(ms, source) {
  if (!meterDelayEl || ms <= 0) return;
  meterDelayEl.textContent = `= ${fmtMs(ms)}`;
  meterDelayEl.classList.toggle("warn", source === "fallback" || source === "clamped");
}

// v1.14.0: the OUTPUT level meter. Instant attack, LEVEL_FALL_DB per poll
// release, a hold tick, and a clip lamp that stays lit until the meter is
// clicked — a 66 ms flash would be missed exactly when it matters.
function linToDb(v) {
  return v > 0 ? 20 * Math.log10(v) : -Infinity;
}

function dbToPct(db) {
  const c = Math.min(LEVEL_CEIL_DB, Math.max(LEVEL_FLOOR_DB, db));
  return (c - LEVEL_FLOOR_DB) / (LEVEL_CEIL_DB - LEVEL_FLOOR_DB) * 100;
}

function renderLevelMeter(peakIn, peakOut) {
  if (!levelOutEl) return;
  const now = performance.now();

  levelOutDb = Math.max(linToDb(peakOut), levelOutDb - LEVEL_FALL_DB, LEVEL_FLOOR_DB);
  levelInDb  = Math.max(linToDb(peakIn),  levelInDb  - LEVEL_FALL_DB, LEVEL_FLOOR_DB);

  if (levelOutDb >= levelHoldDb || now - levelHoldAt > LEVEL_HOLD_MS) {
    levelHoldDb = levelOutDb;
    levelHoldAt = now;
  }

  levelOutEl.style.clipPath = `inset(0 ${100 - dbToPct(levelOutDb)}% 0 0)`;
  levelInEl.style.width  = `${dbToPct(levelInDb)}%`;
  levelHoldEl.style.left = `${dbToPct(levelHoldDb)}%`;
  levelHoldEl.classList.toggle("off", levelHoldDb <= LEVEL_FLOOR_DB);

  if (peakOut > LEVEL_CLIP_LINEAR) clipLampEl.classList.add("lit");
}

function initLevelMeter() {
  levelOutEl  = document.getElementById("level-out");
  levelInEl   = document.getElementById("level-in");
  levelHoldEl = document.getElementById("level-hold");
  clipLampEl  = document.getElementById("clip-lamp");
  const meter = document.getElementById("levelMeter");

  if (!levelOutEl || !levelInEl || !levelHoldEl || !clipLampEl || !meter) {
    console.warn("Level meter elements not found — level meter disabled");
    levelOutEl = null;
    return;
  }
  meter.addEventListener("click", () => clipLampEl.classList.remove("lit"));
}

// ── Grain view (v1.21.0) ────────────────────────────────────────────────────
// Geometry, per grain, in "ms behind now" (the x axis, 0 at the left edge):
//   reverse: reads [spawn − D − G, spawn − D] from its young end to its old end,
//            so its read point ages 2 ms per ms and the stretch ages 1 ms per ms.
//   forward: reads the same stretch old end to young end, in step with the
//            write head, so its read point stays at D while the stretch ages.
// From a snapshot (age = read point, p = phase, L = length) that gives:
//   reverse  young = age − pL        old = young + L
//   forward  young = age + pL − L    old = age + pL
function gvAdvance(g, dt) {
  const phase = g.phase + dt / g.len;
  if (phase >= 1) return null;
  const age = g.fwd ? g.age : g.age + 2 * dt;
  const pl = phase * g.len;
  const young = g.fwd ? age + pl - g.len : age - pl;
  return { age, phase, young, old: young + g.len };
}

// End-of-life extent of a grain's stretch, for the axis target.
function gvOldestAt(g) {
  return g.fwd ? g.age + g.len : g.age - 2 * g.phase * g.len + 2 * g.len;
}

function gvWindow(phase) {
  const c = gvCurve;
  if (c && c.length >= 2) {
    const x = Math.min(1, Math.max(0, phase)) * (c.length - 1);
    const i = Math.floor(x);
    const f = x - i;
    return i >= c.length - 1 ? c[c.length - 1] : c[i] + (c[i + 1] - c[i]) * f;
  }
  const s = Math.sin(Math.PI * phase);   // Hann until the real curve arrives
  return s * s;
}

function ingestGrainView(grains, seq, delayMs) {
  if (!gvCanvas) return;
  if (delayMs !== gvDelayMs) gvDrewEmpty = false;   // move the delay guide even when idle
  gvDelayMs = delayMs;
  if (!Array.isArray(grains)) return;   // torn read: keep animating the last frame

  // Same block as last time = the host stopped calling processBlock. Keep the
  // OLD anchor so the grains play out and fade instead of replaying 66 ms.
  const s = Number(seq);
  if (s === gvSeq) return;
  gvSeq = s;

  gvGrains = grains.map((t) => ({
    age:   Number(t[0]) || 0,
    len:   Math.max(1, Number(t[1]) || 1),
    phase: Number(t[2]) || 0,
    pan:   Math.min(1, Math.max(0, Number(t[3]))),
    level: Math.max(0, Number(t[4]) || 0),
    fwd:   Number(t[5]) === 1,
    peak:  Math.max(0, Number(t[6]) || 0),   // v1.21.1: source level under the grain
  }));
  gvAnchorAt = performance.now();
}

function gvResize() {
  const dpr = window.devicePixelRatio || 1;
  const w = gvCanvas.clientWidth  || 330;
  const h = gvCanvas.clientHeight || 62;
  const bw = Math.round(w * dpr);
  const bh = Math.round(h * dpr);
  if (gvCanvas.width !== bw || gvCanvas.height !== bh) {   // assigning clears
    gvCanvas.width = bw;
    gvCanvas.height = bh;
  }
  gvCtx.setTransform(dpr, 0, 0, dpr, 0, 0);
  return { w, h };
}

function drawGrainView(now) {
  requestAnimationFrame(drawGrainView);
  if (!gvCtx) return;

  const dt = Math.max(0, now - gvAnchorAt);
  const live = [];
  let target = Math.max(GV_MIN_AXIS_MS, gvDelayMs * 1.5);

  // A snapshot with no successor for GV_STALE_MS is let run out, never renewed.
  for (const g of gvGrains) {
    const a = gvAdvance(g, dt);
    if (!a) continue;
    live.push([g, a]);
    target = Math.max(target, gvOldestAt(g));
  }
  if (dt > GV_STALE_MS && live.length === 0) gvGrains = [];

  target *= GV_AXIS_HEAD;
  if (gvAxisMs <= 0) gvAxisMs = target;
  gvAxisMs += (target - gvAxisMs) * (target > gvAxisMs ? GV_AXIS_GROW : GV_AXIS_SHRINK);

  if (live.length === 0 && gvDrewEmpty) return;

  const { w, h } = gvResize();
  const c = gvColors;
  gvCtx.clearRect(0, 0, w, h);

  const x0 = GV_PAD, x1 = w - GV_PAD, span = x1 - x0;
  const yTop = GV_PAD + GV_DOT_R_MAX, yBot = h - GV_PAD - GV_DOT_R_MAX;
  const xAt = (ms) => x0 + span * Math.min(1, Math.max(0, ms / gvAxisMs));
  const yAt = (pan) => yTop + (yBot - yTop) * pan;   // top = left, bottom = right

  // Guides: the centre pan line, and the delay time — the age every grain is
  // born at, so the cloud visibly launches from it.
  gvCtx.save();
  gvCtx.strokeStyle = c.rule;
  gvCtx.lineWidth = 1;
  gvCtx.globalAlpha = 0.35;
  gvCtx.setLineDash([2, 3]);
  gvCtx.beginPath();
  gvCtx.moveTo(x0, Math.round(h / 2) + 0.5);
  gvCtx.lineTo(x1, Math.round(h / 2) + 0.5);
  if (gvDelayMs > 0) {
    const xd = Math.round(xAt(gvDelayMs)) + 0.5;
    gvCtx.moveTo(xd, GV_PAD);
    gvCtx.lineTo(xd, h - GV_PAD);
  }
  gvCtx.stroke();
  gvCtx.restore();

  // "Now" edge: a solid tick on the left, so the direction of time is readable.
  gvCtx.fillStyle = c.rule;
  gvCtx.globalAlpha = 0.6;
  gvCtx.fillRect(x0, GV_PAD, 1.5, h - 2 * GV_PAD);

  // Amplitude 0..1 on the dB scale — the material's level through the window
  // at the grain's CURRENT phase, so a loud grain still fades in and out.
  const loud = (g, a) => {
    const amp = g.peak * gvWindow(a.phase) * g.level;
    if (amp <= 0) return 0;
    const db = 20 * Math.log10(amp);
    return Math.min(1, Math.max(0, (db - GV_FLOOR_DB) / (GV_CEIL_DB - GV_FLOOR_DB)));
  };
  const drawn = live.map(([g, a]) => [g, a, loud(g, a)]);

  // Stretches first, then every playhead on top, so no dot hides under a bar.
  for (const [g, a, t] of drawn) {
    const xa = xAt(a.young), xb = xAt(a.old);
    const y = yAt(g.pan);
    gvCtx.globalAlpha = 0.05 + 0.30 * t;
    gvCtx.fillStyle = g.fwd ? c.fwd : c.ink;
    const bw = Math.max(1, xb - xa);
    if (typeof gvCtx.roundRect === "function") {   // WKWebView < Safari 16 lacks it
      gvCtx.beginPath();
      gvCtx.roundRect(xa, y - GV_BAR_H / 2, bw, GV_BAR_H, GV_BAR_H / 2);
      gvCtx.fill();
    } else {
      gvCtx.fillRect(xa, y - GV_BAR_H / 2, bw, GV_BAR_H);
    }
  }
  for (const [g, a, t] of drawn) {
    const r = GV_DOT_R_MIN + (GV_DOT_R_MAX - GV_DOT_R_MIN) * t;
    const col = g.fwd ? c.fwd : c.ink;
    gvCtx.beginPath();
    gvCtx.arc(xAt(a.age), yAt(g.pan), r, 0, Math.PI * 2);
    if (t <= 0) {
      // Silent: a faint hollow ring — the grain is running, there is just
      // nothing under it.
      gvCtx.globalAlpha = 0.35;
      gvCtx.strokeStyle = col;
      gvCtx.lineWidth = 1;
      gvCtx.stroke();
    } else {
      gvCtx.globalAlpha = 0.15 + 0.85 * t;
      gvCtx.fillStyle = col;
      gvCtx.fill();
    }
  }
  gvCtx.globalAlpha = 1;
  gvDrewEmpty = live.length === 0;
}

function initGrainView() {
  gvCanvas = document.getElementById("grainCanvas");
  if (!gvCanvas || typeof gvCanvas.getContext !== "function") {
    console.warn("Grain canvas not found — grain view disabled");
    gvCanvas = null;
    return;
  }
  gvCtx = gvCanvas.getContext("2d");
  if (!gvCtx) { gvCanvas = null; return; }

  // Read the palette once: a per-frame getComputedStyle would force style
  // recalculation 60 times a second for values that never change.
  const css = getComputedStyle(document.documentElement);
  gvColors = {
    ink:  css.getPropertyValue("--green-dark").trim()   || "#3C5C1A",
    fwd:  css.getPropertyValue("--brown-frame").trim()  || "#5C4033",
    rule: css.getPropertyValue("--brown-border").trim() || "#8B7355",
  };
  requestAnimationFrame(drawGrainView);
}

async function pollGrainMeter() {
  // Never let ticks stack: at 15 Hz a round trip that stalls would otherwise
  // queue, and the queue would drain as a burst of stale values.
  if (!meterFn || meterInFlight) return;

  meterInFlight = true;
  try {
    const raw = await meterFn();
    const m = parseNativeResult(raw);

    // Number() rather than trusting the payload: a var round-trip can deliver
    // these as strings depending on backend, and `overlap.toFixed` on a string
    // throws — which inside an interval callback would be a silent dead readout.
    renderGrainMeter(Number(m.active) || 0, Number(m.overlap) || 0);
    renderEffectiveDelay(Number(m.delayMs) || 0, String(m.delaySource));
    renderGrainLink(Number(m.grainMs) || 0, String(m.grainSource));   // v1.17.0
    renderLevelMeter(Number(m.peakIn) || 0, Number(m.peakOut) || 0);
    ingestGrainView(m.grains, m.grainSeq, Number(m.delayMs) || 0);   // v1.21.0

    const engaged = m.freezeEngaged === true || m.freezeEngaged === "true";
    if (engaged !== freezeEngaged) {
      freezeEngaged = engaged;
      if (repaintFreeze) repaintFreeze();
    }
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
  meterDelayEl   = document.getElementById("effective-delay");   // optional: its absence only blanks it
  initLevelMeter();                                               // v1.14.0: optional in the same way
  initGrainView();                                                // v1.21.0: optional in the same way

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

// v1.12.3: Delete is disabled on a factory preset. Up to v1.12.2 it armed,
// took the confirming click, and then did nothing — preset-manager's
// deletePreset() refuses factory presets with only a console.warn. The shared
// module is left alone; this asks the same isFactoryPreset question up front
// whenever the current preset may have changed (load, prev/next, save, file
// load via onPresetChanged; initial refresh and post-delete refresh via
// onPresetListUpdated). A token discards an answer that arrives after a newer
// change. On a failed lookup the button stays enabled — the manager's own
// guard still stops a factory delete, so failing open costs nothing.
async function updateDeleteAvailability() {
  const btn = document.getElementById("preset-delete");
  if (!btn || !presetManager) return;
  const token = ++deleteGateToken;
  const name = presetManager.getCurrentPreset();
  let factory = false;
  try { factory = (await presetManager.isFactoryPreset(name)) === true; } catch (_) { /* fail open */ }
  if (token !== deleteGateToken) return;
  if (factory && btn.dataset.armed === "1") disarmDelete(btn);
  btn.disabled = factory;
  btn.setAttribute("aria-disabled", factory ? "true" : "false");
}

// ── v1.20.0: grouped preset dropdown ────────────────────────────────────────
// Clicking the name cartouche opens the bank grouped by category, in the order
// C++ reports (getPresetCategories). Built after O-MultiBandCompressor v1.7.0.
//
// ◀ / ▶ are deliberately NOT handed to PresetManager any more: the module binds
// them to selectNextPreset/selectPreviousPreset, which walk getPresetList()'s
// flat alphabetical order. With the list grouped, those two orders disagree and
// ▶ from the last Swells preset would land mid-Vocals
// (pattern_grouping_preset_dropdown_breaks_prev_next). stepPreset() walks
// presetWalkOrder — the dropdown flattened — instead.
const USER_CATEGORY = "User";

let presetGroups       = [];     // [{ category, names }], display order, non-empty only
let presetWalkOrder    = [];     // presetGroups flattened — the ◀ / ▶ sequence
let presetCategoriesFn = null;   // the getPresetCategories native fn
let presetDropdownEl   = null;   // #preset-dropdown
let presetNameEl       = null;   // #preset-name — the dropdown's trigger

// One LITERAL key per heading: check-i18n [13] rejects a computed setLabel key.
// A category C++ adds without a line here falls back to its raw name.
const PRESET_CATEGORY_LABELERS = {
  "Swells":           (el) => setLabel(el, "presetCat.swells"),
  "Vocals":           (el) => setLabel(el, "presetCat.vocals"),
  "Rhythmic":         (el) => setLabel(el, "presetCat.rhythmic"),
  "Ambient":          (el) => setLabel(el, "presetCat.ambient"),
  "Dark":             (el) => setLabel(el, "presetCat.dark"),
  "Glitch & Texture": (el) => setLabel(el, "presetCat.glitch"),
  "Lo-Fi & Drive":    (el) => setLabel(el, "presetCat.lofi"),
  "Motion & Width":   (el) => setLabel(el, "presetCat.motion"),
  "User":             (el) => setLabel(el, "presetCat.user"),
};

async function refreshPresetCategories() {
  if (!presetManager) return;

  const list = presetManager.getPresetList();
  let order = [];
  let map   = {};

  try {
    const result = await presetCategoriesFn();
    order = (result && result.order) || [];
    map   = (result && result.categories) || {};
  } catch (e) {
    // One ungrouped list beats losing the browser.
    console.warn("[preset-bar] categories unavailable, flat list:", e);
    order = [USER_CATEGORY];
    map   = {};
  }

  // Own-property lookup: a user preset saved as "constructor" or "toString"
  // would otherwise read an Object.prototype member and fall out of every group.
  const categoryOf = (name) =>
    (Object.prototype.hasOwnProperty.call(map, name) ? map[name] : null) || USER_CATEGORY;

  presetGroups = order
    .map((category) => ({ category, names: list.filter((n) => categoryOf(n) === category) }))
    .filter((g) => g.names.length > 0);

  // A category C++ did not list (only if the two tables drift) is appended
  // rather than hidden.
  const grouped = new Set(presetGroups.flatMap((g) => g.names));
  const orphans = list.filter((n) => !grouped.has(n));
  if (orphans.length > 0) presetGroups.push({ category: "Other", names: orphans });

  presetWalkOrder = presetGroups.flatMap((g) => g.names);
}

async function stepPreset(delta) {
  if (!presetManager || presetWalkOrder.length === 0) return;
  const index = presetWalkOrder.indexOf(presetManager.getCurrentPreset());
  // Out of the list ("Default", or a file loaded from disk): enter at the top
  // going forward, at the bottom going back.
  const base = index >= 0 ? index : (delta > 0 ? -1 : 0);
  const next = (base + delta + presetWalkOrder.length) % presetWalkOrder.length;
  await presetManager.loadPreset(presetWalkOrder[next]);
}

function isPresetDropdownOpen() {
  return !!presetDropdownEl && !presetDropdownEl.hidden;
}

function showPresetDropdown() {
  if (!presetDropdownEl) return;
  renderPresetDropdown();
  presetDropdownEl.hidden = false;
  presetNameEl.setAttribute("aria-expanded", "true");
  // Land on the current preset, centred, and focus it so arrows work at once.
  const active = presetDropdownEl.querySelector(".preset-option.active")
              || presetDropdownEl.querySelector(".preset-option");
  if (active) {
    active.scrollIntoView({ block: "center" });
    active.focus({ preventScroll: true });
  }
}

function hidePresetDropdown(refocus) {
  if (!isPresetDropdownOpen()) return;
  presetDropdownEl.hidden = true;
  presetNameEl.setAttribute("aria-expanded", "false");
  if (refocus) presetNameEl.focus();
}

function renderPresetDropdown() {
  presetDropdownEl.textContent = "";
  const current = presetManager ? presetManager.getCurrentPreset() : "";

  for (const group of presetGroups) {
    const groupEl = document.createElement("div");
    groupEl.className = "preset-group";
    groupEl.setAttribute("role", "group");

    const heading = document.createElement("div");
    heading.className = "preset-group-heading";
    heading.id = `preset-group-${presetGroups.indexOf(group)}`;
    const labeler = PRESET_CATEGORY_LABELERS[group.category];
    if (labeler) labeler(heading); else heading.textContent = group.category;
    groupEl.setAttribute("aria-labelledby", heading.id);
    groupEl.appendChild(heading);

    for (const name of group.names) {
      const item = document.createElement("div");
      const isActive = name === current;
      item.className = "preset-option" + (isActive ? " active" : "");
      item.setAttribute("role", "option");
      item.setAttribute("aria-selected", isActive ? "true" : "false");
      item.tabIndex = -1;
      item.textContent = name;   // a preset name IS its filename — never translated
      item.addEventListener("click", async () => {
        hidePresetDropdown(true);
        await presetManager.loadPreset(name);
      });
      groupEl.appendChild(item);
    }

    presetDropdownEl.appendChild(groupEl);
  }
}

function onPresetDropdownKey(e) {
  const options = [...presetDropdownEl.querySelectorAll(".preset-option")];
  const i = options.indexOf(document.activeElement);
  const focusAt = (j) => {
    const el = options[Math.max(0, Math.min(options.length - 1, j))];
    if (el) { el.focus({ preventScroll: true }); el.scrollIntoView({ block: "nearest" }); }
  };

  switch (e.key) {
    case "ArrowDown": e.preventDefault(); focusAt(i + 1); break;
    case "ArrowUp":   e.preventDefault(); focusAt(i - 1); break;
    case "Home":      e.preventDefault(); focusAt(0); break;
    case "End":       e.preventDefault(); focusAt(options.length - 1); break;
    case "Enter":
    case " ":
      e.preventDefault();
      if (i >= 0) options[i].click();
      break;
    case "Escape":
    case "Tab":
      if (e.key === "Escape") e.preventDefault();
      hidePresetDropdown(e.key === "Escape");
      break;
  }
}

// Hoisted declaration, called from inside init() — never at module top level,
// and the dynamic import() lives in here rather than at the top of the file, so
// a failure cannot escape module evaluation. The try/catch is load-bearing: it
// contains a preset-bar failure so every already-bound control survives it.
async function initPresetBar() {
  try {
    const { PresetManager } = await import("./preset-manager.js");

    presetNameEl       = document.getElementById("preset-name");
    presetDropdownEl   = document.getElementById("preset-dropdown");
    presetCategoriesFn = Juce.getNativeFunction("getPresetCategories");

    presetManager = new PresetManager({
      displayElement: presetNameEl,
      // prevButton / nextButton deliberately omitted — see stepPreset().
      saveButton:     document.getElementById("preset-save"),
      loadButton:     document.getElementById("preset-load"),
      deleteButton:   document.getElementById("preset-delete"),
      getNativeFunction: Juce.getNativeFunction,
      onConfirmDelete: confirmDeleteInline,
      onPresetChanged:     () => { updateDeleteAvailability(); },
      onPresetListUpdated: () => {
        updateDeleteAvailability();
        refreshPresetCategories().then(() => {
          if (isPresetDropdownOpen()) renderPresetDropdown();
        });
      },
    });

    await presetManager.initialize();
    await refreshPresetCategories();

    document.getElementById("preset-prev").addEventListener("click", () => stepPreset(-1));
    document.getElementById("preset-next").addEventListener("click", () => stepPreset(1));

    presetNameEl.addEventListener("click", (e) => {
      e.stopPropagation();
      if (isPresetDropdownOpen()) hidePresetDropdown(false); else showPresetDropdown();
    });
    presetNameEl.addEventListener("keydown", (e) => {
      if (e.key === "Enter" || e.key === " " || e.key === "ArrowDown") {
        e.preventDefault();
        showPresetDropdown();
      }
    });
    presetDropdownEl.addEventListener("keydown", onPresetDropdownKey);

    // Any press outside the name + list closes it.
    document.addEventListener("mousedown", (e) => {
      if (!isPresetDropdownOpen()) return;
      if (presetDropdownEl.contains(e.target) || presetNameEl.contains(e.target)) return;
      hidePresetDropdown(false);
    });
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
    // <html lang> follows the selector: screen readers pick the French voice,
    // and CSS hyphens:auto / quotes resolve in the page's actual language.
    document.documentElement.lang = uiLanguage;
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
            .then((code) => applyI18n(code))
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
// The gear that carries the language selector and, since v1.11.0, the hover-help
// switch (#tips-toggle, initTipsToggle() below). The switch persists through
// localStorage only: D13 scoped this plugin to display-only hover help, so there
// is still no setTooltipsEnabled native function and no processor state, which
// section 14 of ui_frontend_check.js asserts by name.
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

  // v1.11.0 THE SWITCH, and it is a SHOW gate rather than a bind gate: this
  // renderer is delegated on document, so there is nothing to unbind.
  // data-tip-always survives it — see the markup comment on #tips-toggle in
  // index.html for which two controls carry it and why.
  if (!tipsEnabled && !target.hasAttribute("data-tip-always")) return;

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

// ════════════════════════════════════════════════════════════════════════════
// The hover-help switch (v1.11.0)
// ════════════════════════════════════════════════════════════════════════════
//
// OUTSIDE the applyI18n/initI18n region, deliberately: check-i18n assertion [6]
// byte-compares that region, comment-stripped and whitespace-normalised,
// against scripts/i18n-canon.js. A character added inside it fails the drift
// gate on all 43 plugins' behalf.
//
// DEFAULT IS ON. v1.10.1 showed hover help unconditionally, so ON is the setting
// that leaves an existing user's plugin behaving exactly as it did. Default OFF
// would additionally make boot-all-uis --strict-tips measure an empty tip
// surface and call it correct.
//
// hideTooltip() is a module-level function on this page rather than a closure,
// so unlike the sibling plugins this one needs no published reference. The
// tipsEnabled binding itself lives in the module-state block at the top.

function applyTipsEnabled(on) {
  tipsEnabled = !!on;
  if (!tipsEnabled) hideTooltip();
  const btn = document.getElementById("tips-toggle");
  if (!btn) return;
  btn.setAttribute("aria-pressed", tipsEnabled ? "true" : "false");
  // TWO CALLS IN TWO BRANCHES, never one call with a ternary — the same rule
  // the delete/confirm pair above already follows, and the one check-i18n
  // assertion [13] enforces: an inflection decided in JS is a string no
  // translator can see.
  if (tipsEnabled) setLabel(btn, "ui.on");
  else             setLabel(btn, "ui.off");
}

function initTipsToggle() {
  const btn = document.getElementById("tips-toggle");
  if (!btn) { console.error("Missing tips-toggle element"); return; }
  let stored = null;
  try { stored = localStorage.getItem("ord.tipsEnabled"); } catch (e) { stored = null; }
  // Anything that is not the literal string "false" — including a first run with
  // nothing stored, and a private-mode throw — resolves to ON.
  applyTipsEnabled(stored !== "false");
  btn.addEventListener("click", () => {
    applyTipsEnabled(!tipsEnabled);
    try { localStorage.setItem("ord.tipsEnabled", String(tipsEnabled)); }
    catch (e) { /* private mode: the switch still works, it just forgets */ }
  });
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
  bindSelectCombo(Juce, COMBO_DIVISION);
  shapeState = bindSelectCombo(Juce, COMBO_SHAPE);
  bindGrainLink(Juce);           // v1.17.0
  bindSelectCombo(Juce, COMBO_FREEZE_LENGTH, [   // v1.15.0, in choice order
    (o) => setLabel(o, "opt.freezeLength.ring"),
    (o) => setLabel(o, "opt.freezeLength.delay"),
    (o) => setLabel(o, "opt.freezeLength.oneBar"),
    (o) => setLabel(o, "opt.freezeLength.twoBars"),
  ]);

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
  // 22 bound knobs down with it, which is exactly what the MBC v1.4.0 TDZ throw
  // did to unrelated working controls while build, auval and every static check
  // still passed.
  try { initSettingsPopover(); } catch (e) { console.error("settings popover init failed:", e); }
  try { initI18n(); }           catch (e) { console.error("i18n init failed:", e); }

  initTooltips();
  // AFTER initI18n(), which has already written the button's English face; this
  // reads setLabel() to rewrite it from the stored state. Its own try/catch,
  // matching the two above: a missing switch must not take the renderer down.
  try { initTipsToggle(); } catch (e) { console.error("tips toggle init failed:", e); }
  try { initAbCompare(Juce); }  catch (e) { console.error("A/B init failed:", e); }        // v1.18.0
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
