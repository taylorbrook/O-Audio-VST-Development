/*
   This file is part of O-Tapestop, an Ouaricon Audio plugin.
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
// O-Tapestop — WebView UI controller (Stage 3)
//
// Binds all 19 APVTS parameters two-way: 11 WebSliderRelay knobs + 7
// WebComboBoxRelay controls (MODE, SYNC_MODE and CHARACTER as segment
// groups; the four SYNC_DIV divisions as selects) + 1 WebToggleButtonRelay
// (ENGAGE, the large latching performance control — UI-02).
//
// Native-function surface is exactly FIFTEEN and must match PluginEditor.cpp:
//   getParameterDefaults  (dblclick-reset, engineering units)
//   commitEnvelope        (mouse-up + 50 ms debounce; completes with the
//                          C++-sanitized echo, which we redraw from)
//   requestEnvelope       (page init: current envelope JSON)
//   setTooltipsEnabled    (v1.4.0: the "?" toggle's state → processor)
//   getTooltipsEnabled    (v1.4.0: page init PULLS the saved preference)
//   getUiLanguage         (v1.5.0: page init PULLS the saved hover-help language)
//   setUiLanguage         (v1.5.0: the selector's choice → processor)
// plus the 10 preset fns requested by modules/preset-manager.js (Stage 4):
//   savePreset, savePresetWithDialog, loadPreset, loadPresetFromFile,
//   getPresetList, getCurrentPreset, selectNextPreset, selectPreviousPreset,
//   deletePreset, isFactoryPreset
// An unregistered fn is a silently dead control that passes build, auval AND
// pluginval (pattern_webview_native_fn_bridge_gap) — the gate grep-diffs both
// directions at 15↔15.
//
// Backend events (the sanctioned window.__JUCE__ use — backend events only;
// parameter state comes through the Juce ES-module namespace,
// critical_juce_webview_namespace_vs_postmessage):
//   transportFrame  { state, ratio, phase }  30 Hz  → ratio bar + playhead
//   envelopeState   sanitized envelope JSON on generation change (host
//                   session reload / harness commit while the editor is open)
//
// STRUCTURE IS LOAD-BEARING: every module-level const/let is declared in the
// top block and the single init() call sits at the BOTTOM of the file. A
// top-level call reaching a not-yet-initialised binding throws out of module
// evaluation and silently kills the ENTIRE UI
// (pattern_module_toplevel_init_tdz).
//
// Readouts and knob angles come exclusively from the SliderState
// (getScaledValue / getNormalisedValue) — the C++ NormalisableRange is the
// only source of range and skew. The FORMAT table carries units and decimals
// ONLY (pattern_webview_knob_readout_scaled_value; the three FREE_MS knobs
// are skewed 0.35).
// ============================================================================

import * as Juce from "./juce/index.js";
import { EnvelopeEditor } from "./envelope_editor.js";
// v1.6.0 — hover-help copy AND on-page labels, English + French. A HOISTED
// import; i18n.js exports
// only and never self-executes, so it cannot enter a TDZ chain
// (pattern_module_toplevel_init_tdz).
//
// scripts/check-i18n.js assertion 6 requires this line VERBATIM, single quotes
// included — it is one of the two anchors the repo-wide drift gate matches on.
import { LANGUAGES, I18N, LABELS, TIP_BINDINGS, tr } from './i18n.js';
// Shared preset band (Stage 4). Constructor + explicit DOM refs ONLY — the
// module's createPresetBar() innerHTML-wipes the styled band and never
// creates a delete button.
import { PresetManager } from "../modules/preset-manager.js";

// ── Parameter inventory (must match createParameterLayout() exactly) ────────
const KNOB_IDS = [
  "STOP_FREE_MS", "STOP_CURVE",
  "START_FREE_MS", "START_CURVE",
  "ENV_FREE_MS",
  "CONT_RATE_HZ", "CONT_DEPTH", "CONT_CHAOS",
  "TONE_TRACK", "MIX", "OUTPUT_GAIN",
];

// Segment-group combos (named modes, not lists).
const COMBO_MODE      = "MODE";      // { Stop, Scratch, Continuous }, default 0
const COMBO_SYNC      = "SYNC_MODE"; // { Sync, Free },                default 0
const COMBO_CHARACTER = "CHARACTER"; // { Wobble, Random, Glitch },    default 0

// Select-backed division combos — options built at runtime from
// properties.choices (the C++ StringArray is the single source of truth).
const DIVISION_IDS = ["STOP_SYNC_DIV", "START_SYNC_DIV", "ENV_SYNC_DIV",
                      "CONT_RATE_SYNC_DIV"];

// The plugin's only bool parameter → its only ToggleState. A bool bound
// through a slider or combo relay attaches without error and produces a
// control whose state never updates.
const TOGGLE_ENGAGE = "ENGAGE";

// TapestopTransport::State enum, mirrored by value (Bypassed…ScratchPass).
const STATE_SCRATCH_PASS = 6;

// ── Display formatters — receive the SCALED value, add units only ───────────
const fmtPct = (v) => `${Math.round(v)} %`;
const fmtMs  = (v) => (v >= 1000 ? `${(v / 1000).toFixed(2)} s` : `${Math.round(v)} ms`);
// One decimal (0.1 dB legibility); explicit sign so the trim reads as a trim.
const fmtDb  = (v) => {
  const r = Math.round(v * 10) / 10;
  if (r === 0) return "0.0 dB";
  return `${r > 0 ? "+" : "−"}${Math.abs(r).toFixed(1)} dB`;
};
// Rate range is 0.05–20 Hz (skew 0.3) — two decimals below 10 Hz keep the
// low end legible.
const fmtHz  = (v) => `${v >= 10 ? v.toFixed(1) : v.toFixed(2)} Hz`;

const FORMAT = {
  STOP_FREE_MS:  fmtMs,
  STOP_CURVE:    fmtPct,
  START_FREE_MS: fmtMs,
  START_CURVE:   fmtPct,
  ENV_FREE_MS:   fmtMs,
  CONT_RATE_HZ:  fmtHz,
  CONT_DEPTH:    fmtPct,
  CONT_CHAOS:    fmtPct,
  TONE_TRACK:    fmtPct,
  MIX:           fmtPct,
  OUTPUT_GAIN:   fmtDb,
};

// ── Knob geometry ───────────────────────────────────────────────────────────
const KNOB_MIN_DEG   = -135;   // normalised 0.0
const KNOB_MAX_DEG   = 135;    // normalised 1.0
const DRAG_TRAVEL_PX = 220;    // vertical px for a full 0→1 sweep
const NUDGE_STEP     = 0.02;   // wheel / arrow-key increment

// ── Hover help (v1.4.0) ─────────────────────────────────────────────────────
// TOOLTIP_MARGIN is both the gap between a tip and its control AND the gap it
// keeps from the viewport edge — the same number on purpose, so the clamp
// assertion in tests/ui_tooltip_clamp_check.js has one constant to check.
const TOOLTIP_MARGIN   = 8;
const TOOLTIP_DELAY_MS = 350;  // hover dwell before a tip appears

// ── Mutable module state ────────────────────────────────────────────────────
// EVERY module-level binding lives in this one block — see the TDZ note above.
const sliderState = {};        // id -> Juce SliderState
let modeState     = null;      // Juce ComboBoxState (MODE)
let syncState     = null;      // Juce ComboBoxState (SYNC_MODE)
let characterState = null;     // Juce ComboBoxState (CHARACTER, v1.1)
const divisionState = {};      // id -> Juce ComboBoxState
let engageState   = null;      // Juce ToggleState (ENGAGE)
let paramDefaults = null;      // { id: engineeringDefault } from the native fn

let envEditor     = null;      // EnvelopeEditor instance
let commitEnvFn   = null;      // native fn, resolved once
let requestEnvFn  = null;      // native fn, resolved once

let ratioFillEl    = null;     // #ratio-fill
let ratioReadoutEl = null;     // #ratio-readout
let lastRatioText  = "";       // change gate for the readout writes
let lastFillKey    = "";       // change gate for the fill style writes

let presetManager   = null;    // PresetManager instance (Stage 4)
let deleteArmTimer  = null;    // two-click delete disarm timeout

let tooltipEl         = null;  // #tooltip — the one shared surface
let tooltipToggleEl   = null;  // #help-toggle
let tooltipTimer      = null;  // dwell timer
let tooltipTarget     = null;  // the [data-tip] currently hovered
let tooltipSuppressed = false; // true between pointerdown and pointerup
let tooltipsEnabled   = false; // the "?" state; PULLED from C++ at init
let setTooltipsFn     = null;  // native fn, resolved once

// ═══════════════════════════════════════════════════════════════════════════
// Function declarations (hoisted — safe to reference from init() below)
// ═══════════════════════════════════════════════════════════════════════════

function normToDeg(n) {
  return KNOB_MIN_DEG + n * (KNOB_MAX_DEG - KNOB_MIN_DEG);
}

// Inverse of SliderState.getNormalisedValue(), using the LIVE properties
// pushed from the C++ NormalisableRange (start/end/skew) — never hardcoded.
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

// ── Knob binding (relative vertical drag, pointer capture — ORD WR-05) ──────
function bindKnob(juce, id) {
  const st = juce.getSliderState(id);
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

  // Terminates on cancel and lost-capture as well as on up, and is idempotent
  // — all four paths can fire (ORD v1.7.2 WR-05).
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
    // Capture on the KNOB, never window listeners: guarantees a terminating
    // event even when the drag leaves the plugin window mid-gesture.
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

  knob.addEventListener("dblclick", (e) => {
    resetToDefault(st, id);
    e.preventDefault();
  });
}

// ── <select>-backed division combos ─────────────────────────────────────────
// Options built from the LIVE properties.choices, rebuilt if they arrive
// late, index refreshed on both events (ORD bindSelectCombo verbatim).
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
    buildOptions();
    const idx = st.getChoiceIndex();
    if (idx >= 0 && idx < sel.options.length) sel.selectedIndex = idx;
  };

  st.propertiesChangedEvent.addListener(refresh);
  st.valueChangedEvent.addListener(refresh);
  refresh();

  sel.addEventListener("change", () => st.setChoiceIndex(sel.selectedIndex));

  return st;
}

// ── MODE segment group + centre-pane swap ───────────────────────────────────
// Stop/Scratch/Motion copy is authored in index.html and never rewritten here
// — classes + aria only (pattern_js_state_updater_overwrites_html_labels).
// The pane swap is an instant .hidden toggle of absolutely-positioned panes
// in a fixed box: zero layout shift (locked decision). Three-way since v1.1.
function bindModeSegments(juce) {
  const st = juce.getComboBoxState(COMBO_MODE);
  modeState = st;

  const segs = [
    document.getElementById("seg-mode-stop"),
    document.getElementById("seg-mode-scratch"),
    document.getElementById("seg-mode-cont"),
  ];
  const panes = [
    document.getElementById("pane-stop"),
    document.getElementById("pane-scratch"),
    document.getElementById("pane-continuous"),
  ];

  if (segs.some((el) => !el) || panes.some((el) => !el)) {
    console.error("Missing MODE segment / pane elements");
    return;
  }

  const refresh = () => {
    const idx = Math.min(2, Math.max(0, st.getChoiceIndex()));

    segs.forEach((seg, i) => {
      seg.classList.toggle("active", i === idx);
      seg.setAttribute("aria-pressed", String(i === idx));
    });
    panes.forEach((pane, i) => pane.classList.toggle("hidden", i !== idx));
  };

  st.valueChangedEvent.addListener(refresh);
  st.propertiesChangedEvent.addListener(refresh);
  refresh();

  segs.forEach((seg, i) => seg.addEventListener("click", () => st.setChoiceIndex(i)));
}

// ── CHARACTER segment stack (Continuous pane, v1.1) ─────────────────────────
// Same segment-group discipline as MODE: HTML-authored copy, classes + aria
// only, choice index straight from the ComboBoxState.
function bindCharacterSegments(juce) {
  const st = juce.getComboBoxState(COMBO_CHARACTER);
  characterState = st;

  const segs = [
    document.getElementById("seg-char-wobble"),
    document.getElementById("seg-char-random"),
    document.getElementById("seg-char-glitch"),
  ];

  if (segs.some((el) => !el)) {
    console.error("Missing CHARACTER segment elements");
    return;
  }

  const refresh = () => {
    const idx = Math.min(2, Math.max(0, st.getChoiceIndex()));
    segs.forEach((seg, i) => {
      seg.classList.toggle("active", i === idx);
      seg.setAttribute("aria-pressed", String(i === idx));
    });
  };

  st.valueChangedEvent.addListener(refresh);
  st.propertiesChangedEvent.addListener(refresh);
  refresh();

  segs.forEach((seg, i) => seg.addEventListener("click", () => st.setChoiceIndex(i)));
}

// ── SYNC_MODE segment pair + triple time-slot swap ──────────────────────────
// Toggles the select-wrap / knob-wrap visibility in ALL THREE time slots
// (stop, start, env). The env slot lives inside the Scratch pane, so its
// on-screen visibility is the AND of MODE and SYNC_MODE — expressed as two
// INDEPENDENT class toggles on nested wrappers, never one computed flag
// (locked decision). Both controls in every slot stay relay-bound at all
// times; only visibility moves.
function bindSyncSegments(juce) {
  const st = juce.getComboBoxState(COMBO_SYNC);
  syncState = st;

  const segSync = document.getElementById("seg-sync-sync");
  const segFree = document.getElementById("seg-sync-free");

  const wraps = [
    ["wrap-STOP_SYNC_DIV",  "wrap-STOP_FREE_MS"],
    ["wrap-START_SYNC_DIV", "wrap-START_FREE_MS"],
    ["wrap-ENV_SYNC_DIV",   "wrap-ENV_FREE_MS"],
    ["wrap-CONT_RATE_SYNC_DIV", "wrap-CONT_RATE_HZ"],   // Continuous pane (v1.1)
  ].map(([divId, msId]) => [document.getElementById(divId), document.getElementById(msId)]);

  if (!segSync || !segFree || wraps.some(([a, b]) => !a || !b)) {
    console.error("Missing SYNC_MODE segment / slot-wrap elements");
    return;
  }

  const refresh = () => {
    const isFree = st.getChoiceIndex() === 1;   // { Sync, Free }, default 0 = Sync

    segSync.classList.toggle("active", !isFree);
    segFree.classList.toggle("active", isFree);
    segSync.setAttribute("aria-pressed", String(!isFree));
    segFree.setAttribute("aria-pressed", String(isFree));

    for (const [divWrap, msWrap] of wraps) {
      divWrap.classList.toggle("hidden", isFree);
      msWrap.classList.toggle("hidden", !isFree);
    }
  };

  st.valueChangedEvent.addListener(refresh);
  st.propertiesChangedEvent.addListener(refresh);
  refresh();

  segSync.addEventListener("click", () => st.setChoiceIndex(0));
  segFree.addEventListener("click", () => st.setChoiceIndex(1));
}

// ── ENGAGE — large latching toggle (UI-02) ──────────────────────────────────
// A ToggleState, NOT a combo/slider: the relay TYPE has to match the
// parameter type or the control attaches cleanly and never updates. The
// attachment path is setValueNotifyingHost with gesture brackets, so a UI
// click is indistinguishable from host automation by construction. The
// ENGAGE copy is authored in index.html; classes + aria only here.
function bindEngage(juce) {
  const st = juce.getToggleState(TOGGLE_ENGAGE);
  engageState = st;

  const btn = document.getElementById("engage-btn");
  if (!btn) { console.error("Missing engage button"); return; }

  const refresh = () => {
    const on = st.getValue() === true;
    btn.classList.toggle("active", on);
    btn.setAttribute("aria-pressed", String(on));
  };

  st.valueChangedEvent.addListener(refresh);
  st.propertiesChangedEvent.addListener(refresh);
  refresh();

  btn.addEventListener("click", () => { st.setValue(!(st.getValue() === true)); refresh(); });
}

// ── Defaults for dblclick-reset ─────────────────────────────────────────────
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
// Envelope editor bridge (UI-01)
// ═══════════════════════════════════════════════════════════════════════════

// Commit → redraw from the C++-sanitized echo, so any JS/C++ disagreement
// (clamps, sort, endpoint pins) resolves to the C++ truth.
async function commitEnvelope(json) {
  if (!commitEnvFn) return;
  try {
    const echo = await commitEnvFn(json);
    if (envEditor && typeof echo === "string" && echo.length > 0) {
      envEditor.setFromJson(echo);
    }
  } catch (e) {
    console.error("commitEnvelope failed:", e);
  }
}

async function initEnvelope(juce) {
  const canvas = document.getElementById("envCanvas");
  if (!canvas || typeof canvas.getContext !== "function") {
    console.warn("Envelope canvas not found — editor disabled");
    return;
  }

  try {
    commitEnvFn = juce.getNativeFunction("commitEnvelope");
    requestEnvFn = juce.getNativeFunction("requestEnvelope");
  } catch (e) {
    console.error("Envelope native fns unavailable:", e);
    // The editor still renders its local default; edits just cannot commit.
  }

  envEditor = new EnvelopeEditor(canvas, (json) => { commitEnvelope(json); });

  // Page init: pull the processor's current envelope (session restore).
  if (requestEnvFn) {
    try {
      const json = await requestEnvFn();
      if (typeof json === "string" && json.length > 0) envEditor.setFromJson(json);
    } catch (e) {
      console.error("requestEnvelope failed:", e);
    }
  }
}

// ═══════════════════════════════════════════════════════════════════════════
// Preset band (Stage 4) — modules/preset-manager.js
// ═══════════════════════════════════════════════════════════════════════════

// Two-click armed delete — the pattern #preset-delete's CSS anticipates
// (data-armed="1" restyles the button). First call arms the button and shows
// its confirm copy; a second call within the window confirms. Never
// window.confirm() — native confirm dialogs inside a JUCE WebView are
// unreliable.
//
// v1.6.0: the two faces are KEYS, not the data-label / data-confirm attributes
// they were through v1.5.0. Under canon v2 the element carries its current
// key, so a language switch mid-arm re-renders the ARMED face in the new
// language — an attribute pair would have restored the English one. Two
// separate setLabel() calls behind an if/else, never one call with a ternary
// in its argument: check-i18n assertion 13 rejects that shape outright,
// because inflection inside a localized string is the thing contract §6
// authors around rather than engineers.
function armedConfirmDelete() {
  const btn = document.getElementById("preset-delete");
  if (!btn) return false;

  const disarm = () => {
    if (deleteArmTimer) { clearTimeout(deleteArmTimer); deleteArmTimer = null; }
    btn.dataset.armed = "0";
    setLabel(btn, "preset-delete");
  };

  if (btn.dataset.armed === "1") {
    disarm();
    return true;          // second click within the window — confirmed
  }

  btn.dataset.armed = "1";
  setLabel(btn, "ui.confirm");
  deleteArmTimer = setTimeout(disarm, 2500);
  return false;           // first click — armed only
}

// ── Themed dropdown (v1.3) ──────────────────────────────────────────────────
// Display-side grouping ONLY: getPresetList() stays a flat case-insensitive
// sort (factory + user) and the preset JSON format is untouched. This map
// must track the factory names in PluginProcessor.cpp — an unmapped factory
// preset is not lost, it just falls into the trailing "User" group.
//
// v1.6.2 (Stage O item 39): each theme's HEADING is a `label.theme*` key in
// i18n.js, written through setLabel() so the heading becomes a [data-i18n]
// element the language sweep owns — through v1.6.1 the headings were English
// literals written by textContent, so the French page showed English group
// names its own preset-name tip could only describe in English. The
// heading is a one-line arrow per row rather than a key string, because
// check-i18n assertion 13 accepts only a LITERAL key in a setLabel() call:
// `setLabel(head, row.key)` would be a computed key the gate cannot check.
// The preset NAMES stay raw: a name IS the JSON filename (I18N_EXEMPT, D-02).
const PRESET_THEMES = [
  [(el) => setLabel(el, "label.themeTapeStops"),
   ["Classic Half-Bar Stop", "Classic 1-Bar Stop", "DJ Spinup",
    "Tempo-Synced Short Stop", "Slow-Tape Drag", "Power Cut",
    "Cassette Eject", "Two-Bar Dive", "Snap Back", "Half-Mix Stop"]],
  [(el) => setLabel(el, "label.themeScratch"),
   ["Baby Scratch", "Chirp Flare", "Stutter-Scratch", "Transformer",
    "Tape Rewind", "Orbit", "Crab Roll"]],
  [(el) => setLabel(el, "label.themeWobbleWarp"),
   ["Subtle Wobble", "Warped Record", "Drunk Tape", "Seasick",
    "Tape Flutter", "Pitch Tide", "Loose Capstan"]],
  [(el) => setLabel(el, "label.themeGlitchChaos"),
   ["Glitch", "Total Meltdown", "Sputter", "Data Rot"]],
];

// Rebuilt from scratch on every open — the list is small and this sidesteps
// stale-list bugs after saves/deletes without extra refresh plumbing. Items
// are JS-owned nodes, so textContent here is safe (the label-overwrite trap
// only bites HTML-authored children of #preset-name itself).
function buildPresetDropdown(panel) {
  panel.textContent = "";
  const all = presetManager.getPresetList();
  const current = presetManager.getCurrentPreset();
  const mapped = new Set(PRESET_THEMES.flatMap(([, names]) => names));

  const addGroup = (labelHeading, names) => {
    if (!names.length) return;
    const head = document.createElement("div");
    head.className = "dropdown-theme";
    labelHeading(head);
    panel.appendChild(head);
    for (const name of names) {
      const item = document.createElement("div");
      item.className = "dropdown-item" + (name === current ? " current" : "");
      item.setAttribute("role", "option");
      item.setAttribute("aria-selected", name === current ? "true" : "false");
      item.textContent = name;
      item.addEventListener("click", async () => {
        closePresetDropdown();
        await presetManager.loadPreset(name);
      });
      panel.appendChild(item);
    }
  };

  // Curated order within factory themes; only names the C++ side actually
  // reported survive the filter (a renamed factory preset can't ghost-list).
  for (const [labelHeading, names] of PRESET_THEMES)
    addGroup(labelHeading, names.filter((n) => all.includes(n)));
  addGroup((el) => setLabel(el, "label.themeUser"), all.filter((n) => !mapped.has(n)));
}

function openPresetDropdown() {
  const panel = document.getElementById("preset-dropdown");
  const nameEl = document.getElementById("preset-name");
  if (!panel || !nameEl) return;
  buildPresetDropdown(panel);
  panel.hidden = false;
  nameEl.setAttribute("aria-expanded", "true");
}

function closePresetDropdown() {
  const panel = document.getElementById("preset-dropdown");
  const nameEl = document.getElementById("preset-name");
  if (!panel || panel.hidden) return;
  panel.hidden = true;
  if (nameEl) nameEl.setAttribute("aria-expanded", "false");
}

function initPresetDropdown(selectEl, nameEl, panelEl) {
  const toggle = () => {
    if (!selectEl.classList.contains("ready")) return;
    if (panelEl.hidden) openPresetDropdown();
    else closePresetDropdown();
  };

  nameEl.addEventListener("click", toggle);
  nameEl.addEventListener("keydown", (e) => {
    if (e.key === "Enter" || e.key === " ") { e.preventDefault(); toggle(); }
    else if (e.key === "Escape") closePresetDropdown();
  });

  // Outside click / Escape close. pointerdown (not click) so a drag that
  // starts outside also dismisses; listeners are page-lifetime, matching the
  // band itself.
  document.addEventListener("pointerdown", (e) => {
    if (!panelEl.hidden && !selectEl.contains(e.target)) closePresetDropdown();
  });
  document.addEventListener("keydown", (e) => {
    if (e.key === "Escape") closePresetDropdown();
  });
}

// Constructor + EXPLICIT DOM refs for all six band elements, plus
// getNativeFunction — omitting it leaves the bar rendered but silently inert.
// initialize() is called from init() below (post-bridge, view visible — a
// hidden view drops native-fn completions and initialize() would hang before
// setting isInitialized). Un-disabling the buttons here keeps index.html the
// single source of the disabled ship state.
function initPresets() {
  const byId = (id) => document.getElementById(id);
  const els = ["preset-prev", "preset-next", "preset-name",
               "preset-save", "preset-load", "preset-delete"].map(byId);

  if (els.some((el) => !el)) {
    console.error("Missing preset-bar elements — preset band disabled");
    return;
  }

  const [prevEl, nextEl, nameEl, saveEl, loadEl, deleteEl] = els;

  presetManager = new PresetManager({
    displayElement: nameEl,       // stays childless — _updateDisplay writes textContent
    prevButton: prevEl,
    nextButton: nextEl,
    saveButton: saveEl,
    loadButton: loadEl,
    deleteButton: deleteEl,
    getNativeFunction: Juce.getNativeFunction,
    onConfirmDelete: () => armedConfirmDelete(),
  });

  // Dropdown trigger (v1.3): wired before initialize() but gated on the
  // wrapper's "ready" class, so a dead bridge leaves the name inert too.
  const selectEl = document.getElementById("preset-select");
  const panelEl = document.getElementById("preset-dropdown");
  if (selectEl && panelEl) initPresetDropdown(selectEl, nameEl, panelEl);

  // The band shipped disabled in Stage 3's markup. Un-disable only after
  // initialize() resolves (native fns bound, first refresh done) — if the
  // bridge never comes up, the buttons stay honestly disabled instead of
  // enabled-but-inert. (#preset-name is a div — its enabled state is the
  // wrapper's "ready" class + tabindex, granted on the same promise.)
  presetManager.initialize().then(() => {
    [prevEl, nextEl, saveEl, loadEl, deleteEl].forEach((el) => { el.disabled = false; });
    if (selectEl && panelEl) {
      selectEl.classList.add("ready");
      nameEl.setAttribute("tabindex", "0");
    }
  });
}

// ═══════════════════════════════════════════════════════════════════════════
// Live readback: transportFrame (30 Hz) + envelopeState (on change)
// ═══════════════════════════════════════════════════════════════════════════

// Bar geometry mirrors styles.css: zero mark at 40 %; the negative half
// compresses −2..0 into 40 %, the positive spreads 0..+2 over 60 %.
function ratioToPct(r) {
  if (r >= 0) return 40 + (Math.min(r, 2) / 2) * 60;
  return 40 + (Math.max(r, -2) / 2) * 40;
}

function renderRatio(ratio) {
  if (!ratioFillEl || !ratioReadoutEl) return;

  // Change-gated DOM writes: steady 1.00× costs nothing.
  const text = `${ratio < 0 ? "−" : ""}${Math.abs(ratio).toFixed(2)}×`;
  if (text !== lastRatioText) {
    lastRatioText = text;
    ratioReadoutEl.textContent = text;
  }

  const pct = ratioToPct(ratio);
  const left = Math.min(40, pct);
  const width = Math.abs(pct - 40);
  const key = `${left.toFixed(1)}|${width.toFixed(1)}|${ratio < 0 ? "r" : "f"}`;
  if (key !== lastFillKey) {
    lastFillKey = key;
    ratioFillEl.style.left = `${left}%`;
    ratioFillEl.style.width = `${width}%`;
    ratioFillEl.classList.toggle("reverse", ratio < 0);
  }
}

function initReadback() {
  ratioFillEl = document.getElementById("ratio-fill");
  ratioReadoutEl = document.getElementById("ratio-readout");

  if (!window.__JUCE__ || !window.__JUCE__.backend) {
    console.warn("JUCE backend not available — live readback disabled");
    return;
  }

  // The sanctioned __JUCE__ use: backend events (Polystutter precedent).
  window.__JUCE__.backend.addEventListener("transportFrame", (payload) => {
    try {
      const data = typeof payload === "string" ? JSON.parse(payload) : payload;
      const ratio = Number(data.ratio) || 0;
      const state = Number(data.state) || 0;
      const phase = Number(data.phase) || 0;

      renderRatio(ratio);

      // Canvas playhead only while a scratch pass runs in Scratch mode;
      // setPlayhead() itself skips the redraw when the phase is unchanged.
      if (envEditor) {
        const scratchVisible = modeState && modeState.getChoiceIndex() === 1;
        if (scratchVisible && state === STATE_SCRATCH_PASS) {
          envEditor.setPlayhead(phase);
        } else {
          envEditor.setPlayhead(null);
        }
      }
    } catch (e) {
      console.error("transportFrame handler failed:", e);
    }
  });

  // Envelope changed under the editor (host session reload, preset load,
  // another editor): redraw from the sanitized JSON. Push model — no JS
  // promise that could be dropped while hidden
  // (critical_webview_completion_gated_on_isvisible).
  window.__JUCE__.backend.addEventListener("envelopeState", (payload) => {
    if (envEditor && payload != null) envEditor.setFromJson(payload);
  });
}

// ═══════════════════════════════════════════════════════════════════════════
// Hover-help language (v1.5.0)
// ═══════════════════════════════════════════════════════════════════════════
//
// THIS BLOCK IS REPLICATED VERBATIM ACROSS EVERY LOCALIZED PLUGIN and is
// byte-compared (comments stripped, whitespace collapsed) against
// scripts/i18n-canon.js by scripts/check-i18n.js assertion 6. This repo has no
// shared UI module and deliberately does not gain one, so 43 hand-copies are
// only safe because a drifted copy fails a gate. Do not "tidy" it.
//
// v1.6.0 — CANON V2. v1 localized tooltip ATTRIBUTES only. v2 adds the
// [data-i18n] label sweep, the aria/placeholder/alt attribute sweep, and
// setLabel(). Three things follow that matter more than they look:
//
//   applyLabel() writes textContent AND dataset.label TOGETHER. Making every
//   label JS-written would otherwise put this page into
//   pattern_js_state_updater_overwrites_html_labels wholesale; writing both in
//   one place instead makes the invariant checkable at render time, and
//   scripts/check-ui-labels.js asserts dataset.label === textContent after
//   init, after a language switch AND after a state-update pass.
//
//   setLabel() gives a JS-written label its OWN KEY, so the element becomes a
//   [data-i18n] element and the language sweep owns it from then on. There is
//   ONE re-render path. A state string written as a raw literal — "Confirm?",
//   "On" — is stranded in the previous language the instant the selector
//   fires, and window.__setLanguage(), which the gates drive, fires no change
//   event at all.
//
//   trLabel() falls back to I18N so a control whose tooltip title IS its label
//   carries one key. See the REUSE RULE in i18n.js: the fallback is used only
//   where the string is identical in BOTH languages.
//
// One PULL at page init, no push, no timer, no poll().then(poll), no revision
// counter — the same discipline initTooltips() already uses for the v1.4.0
// preference, and for the same reason. The language is not preset content
// either: OuariconPresetManager::loadPreset walks preset["parameters"] and
// never touches a state-tree property.
//
// `grep -rn setVisible plugins/O-Tapestop/Source/` returns NOTHING, so the web
// view is never hidden and the hidden-completion drop cannot fire
// (critical_webview_completion_gated_on_isvisible).
//
// Declared at module level, ABOVE every reader. The only statement executed at
// module-evaluation time is the window.__setLanguage assignment, which touches
// a hoisted function declaration and cannot enter a TDZ chain.

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

// ── The settings popover (v1.5.0) ──────────────────────────────────────────
//
// The gear that carries the language selector and the moved hover-help toggle.
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
  // click, so the panel is gone before a drag on a knob or on the envelope
  // editor underneath it begins — both call preventDefault in their own
  // pointerdown handlers.
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
// Hover help (v1.4.0)
//
// Ported from O-ReverseDelay v1.1.0 — the version carrying the VERIFIED
// measure-then-pin placement, not the earlier shrink-to-fit variant
// (pattern_fixed_tooltip_shrink_to_fit_edge). Two things differ here:
//
//   1. Every show is gated on `tooltipsEnabled`, the "?" toggle's state,
//      which round-trips through the processor so it survives a session
//      reload. The toggle's OWN tip carries data-tip-always and bypasses the
//      gate — otherwise the one control that could turn help back on would be
//      the one control unable to explain itself.
//   2. The centre panel is three mode-switched panes. A tip on a hidden pane
//      is unreachable, so tests/ui_tooltip_clamp_check.js sweeps all three —
//      a single-pane sweep leaves a third of the anchors unverified forever.
// ═══════════════════════════════════════════════════════════════════════════

function tooltipAllowed(target) {
  return tooltipsEnabled || target.hasAttribute("data-tip-always");
}

function handleTooltipOver(e) {
  const target = e.target.closest ? e.target.closest("[data-tip]") : null;
  if (!target || target === tooltipTarget) return;

  tooltipTarget = target;
  clearTimeout(tooltipTimer);

  if (tooltipSuppressed || !tooltipAllowed(target)) return;
  tooltipTimer = setTimeout(() => showTooltip(target), TOOLTIP_DELAY_MS);
}

function handleTooltipOut(e) {
  const target = e.target.closest ? e.target.closest("[data-tip]") : null;
  if (!target) return;

  // Moving between children of the same control is not a real exit. Every
  // knob cell wraps a knob, a label and a value readout, so without this the
  // surface flickers off and back on as the pointer crosses them.
  if (e.relatedTarget && target.contains(e.relatedTarget)) return;

  hideTooltip();
}

function showTooltip(target) {
  // The pointer may have moved on, or gone down, during the dwell.
  if (!tooltipEl || tooltipSuppressed || target !== tooltipTarget) return;
  if (!tooltipAllowed(target)) return;

  const title = target.getAttribute("data-tip-title");
  const body  = target.getAttribute("data-tip");
  if (!body) return;

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
  // shrink-to-fits whatever space remains to its right, so measuring at the
  // PREVIOUS offset under-reports the width, and applying a near-edge `left`
  // afterwards re-wraps a 230 px tip into a ~70 px ribbon that never recovers
  // on later hovers. Release the width, measure from the left edge, pin the
  // result in px, and only then place.
  //
  // On this page the exposed control is the "?" toggle itself — right-most on
  // the frame, 16 px from the edge. Build, auval and pluginval are all blind
  // to it (pattern_fixed_tooltip_shrink_to_fit_edge).
  tooltipEl.style.width = "";
  tooltipEl.style.left  = "0px";
  tooltipEl.style.top   = "0px";

  // getBoundingClientRect, NOT offsetWidth: offsetWidth rounds, and a natural
  // width of 208.48 px pinned at 208 px is narrower than the box's own
  // shrink-to-fit — enough to push the last word onto a second line, after
  // the height has already been read.
  const width = tooltipEl.getBoundingClientRect().width;
  tooltipEl.style.width = `${width}px`;

  // Height is only stable once the width is definite. Read before the pin and
  // the tip is PLACED at 28 px while it RENDERS at 42 px, landing on top of
  // the control it describes.
  const height = tooltipEl.getBoundingClientRect().height;

  // Prefer above; flip below only when there is no room at the top. Computed
  // as a real top edge rather than a centre plus translateY(-100%), so the
  // clamp arithmetic and the box the browser lays out are the same geometry.
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

// v1.5.0: the toggle moved into the settings popover and shows On/Off rather
// than the old static "?" glyph. v1.6.0: those two faces are KEYS through
// setLabel(), not the data-on / data-off attributes they were through v1.5.0.
//
// The attributes were the right answer while the page was English-only — they
// kept the copy out of this file, which is what
// pattern_js_state_updater_overwrites_html_labels asks for. They are the wrong
// answer once the page has two languages: an attribute holds ONE string, so
// switching to French mid-session would have restored an English "On". A key
// re-renders. aria-pressed carries the state for a screen reader, which is why
// replacing a literal caption with a localized one costs no accessibility.
//
// if/else, not a ternary inside the call — check-i18n assertion 13.
function applyTooltipsEnabled(enabled) {
  tooltipsEnabled = !!enabled;

  if (tooltipToggleEl) {
    tooltipToggleEl.classList.toggle("active", tooltipsEnabled);
    tooltipToggleEl.setAttribute("aria-pressed", tooltipsEnabled ? "true" : "false");
    if (tooltipsEnabled) setLabel(tooltipToggleEl, "ui.on");
    else                 setLabel(tooltipToggleEl, "ui.off");
  }

  if (!tooltipsEnabled) hideTooltip();
}

function initTooltips() {
  tooltipEl       = document.getElementById("tooltip");
  tooltipToggleEl = document.getElementById("help-toggle");

  if (!tooltipEl) { console.warn("Tooltip surface missing — hover help disabled"); return; }

  document.addEventListener("mouseover", handleTooltipOver);
  document.addEventListener("mouseout", handleTooltipOut);

  // Any press begins a click or a drag: get the tip out of the way and keep it
  // away until release, so it cannot hang over a knob mid-drag or over the
  // envelope canvas mid-edit. CAPTURE phase, because the knobs and the canvas
  // call preventDefault in their own pointerdown handlers.
  document.addEventListener("pointerdown", () => {
    tooltipSuppressed = true;
    hideTooltip();
  }, true);

  document.addEventListener("pointerup", () => { tooltipSuppressed = false; }, true);

  if (tooltipToggleEl) {
    tooltipToggleEl.addEventListener("click", () => {
      applyTooltipsEnabled(!tooltipsEnabled);
      // Fire-and-forget: the page is already in the new state, and a failed
      // persist must not leave the toggle disagreeing with what it shows.
      if (setTooltipsFn) setTooltipsFn(tooltipsEnabled).catch(() => {});
    });
  }

  // PULL the persisted preference. The C++ side deliberately never pushes it:
  // a push from the editor constructor or the first 30 Hz tick fires before
  // this module has evaluated, so it would silently never arrive and the
  // toggle would read OFF on every reopen (the O-FreqPulse WR-01 bug).
  try {
    setTooltipsFn = Juce.getNativeFunction("setTooltipsEnabled");
    Juce.getNativeFunction("getTooltipsEnabled")()
      .then((enabled) => applyTooltipsEnabled(!!enabled))
      .catch((e) => console.error("[help] getTooltipsEnabled failed:", e));
  } catch (e) {
    console.error("[help] native bridge unavailable:", e);
  }
}

// ── Entry point ─────────────────────────────────────────────────────────────
function init() {
  KNOB_IDS.forEach((id) => bindKnob(Juce, id));
  DIVISION_IDS.forEach((id) => { divisionState[id] = bindSelectCombo(Juce, id); });
  bindModeSegments(Juce);
  bindCharacterSegments(Juce);
  bindSyncSegments(Juce);
  bindEngage(Juce);

  loadParameterDefaults(Juce);   // async; nothing else depends on it
  initEnvelope(Juce);            // async; self-contained failure
  initReadback();
  initPresets();                 // preset band (Stage 4); async init inside

  // v1.5.0. BEFORE initTooltips(): applyI18n() is what puts data-tip on the
  // anchors in the first place, and the renderer's delegated listener resolves
  // e.target.closest("[data-tip]") at hover time — a first hover landing in the
  // window between the two would find no anchor at all. Ordering here is
  // load-bearing in the ordinary way, not the TDZ way.
  //
  // Each inside its own try/catch: a translation-table typo must not take the
  // drawn envelope editor down with it.
  try { initSettingsPopover(); } catch (e) { console.error("settings popover init failed:", e); }
  try { initI18n(); }           catch (e) { console.error("i18n init failed:", e); }

  initTooltips();                // hover help (v1.4.0); async PULL inside
}

// Single call, at the BOTTOM of the module — every binding above is initialised.
init();
