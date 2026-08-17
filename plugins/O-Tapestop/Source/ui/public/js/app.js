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
// Native-function surface is exactly THIRTEEN and must match PluginEditor.cpp:
//   getParameterDefaults  (dblclick-reset, engineering units)
//   commitEnvelope        (mouse-up + 50 ms debounce; completes with the
//                          C++-sanitized echo, which we redraw from)
//   requestEnvelope       (page init: current envelope JSON)
// plus the 10 preset fns requested by modules/preset-manager.js (Stage 4):
//   savePreset, savePresetWithDialog, loadPreset, loadPresetFromFile,
//   getPresetList, getCurrentPreset, selectNextPreset, selectPreviousPreset,
//   deletePreset, isFactoryPreset
// An unregistered fn is a silently dead control that passes build, auval AND
// pluginval (pattern_webview_native_fn_bridge_gap) — the gate grep-diffs both
// directions at 13↔13.
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
// its data-confirm copy; a second call within the window confirms. Never
// window.confirm() — native confirm dialogs inside a JUCE WebView are
// unreliable. Button copy comes from data-label / data-confirm, never JS
// literals (pattern_js_state_updater_overwrites_html_labels).
function armedConfirmDelete() {
  const btn = document.getElementById("preset-delete");
  if (!btn) return false;

  const disarm = () => {
    if (deleteArmTimer) { clearTimeout(deleteArmTimer); deleteArmTimer = null; }
    btn.dataset.armed = "0";
    btn.textContent = btn.dataset.label || "Delete";
  };

  if (btn.dataset.armed === "1") {
    disarm();
    return true;          // second click within the window — confirmed
  }

  btn.dataset.armed = "1";
  btn.textContent = btn.dataset.confirm || "Confirm?";
  deleteArmTimer = setTimeout(disarm, 2500);
  return false;           // first click — armed only
}

// ── Themed dropdown (v1.3) ──────────────────────────────────────────────────
// Display-side grouping ONLY: getPresetList() stays a flat case-insensitive
// sort (factory + user) and the preset JSON format is untouched. This map
// must track the factory names in PluginProcessor.cpp — an unmapped factory
// preset is not lost, it just falls into the trailing "User" group.
const PRESET_THEMES = [
  ["Tape Stops", ["Classic Half-Bar Stop", "Classic 1-Bar Stop", "DJ Spinup",
                  "Tempo-Synced Short Stop", "Slow-Tape Drag", "Power Cut",
                  "Cassette Eject", "Two-Bar Dive", "Snap Back", "Half-Mix Stop"]],
  ["Scratch", ["Baby Scratch", "Chirp Flare", "Stutter-Scratch", "Transformer",
               "Tape Rewind", "Orbit", "Crab Roll"]],
  ["Wobble & Warp", ["Subtle Wobble", "Warped Record", "Drunk Tape", "Seasick",
                     "Tape Flutter", "Pitch Tide", "Loose Capstan"]],
  ["Glitch & Chaos", ["Glitch", "Total Meltdown", "Sputter", "Data Rot"]],
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

  const addGroup = (label, names) => {
    if (!names.length) return;
    const head = document.createElement("div");
    head.className = "dropdown-theme";
    head.textContent = label;
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
  for (const [label, names] of PRESET_THEMES)
    addGroup(label, names.filter((n) => all.includes(n)));
  addGroup("User", all.filter((n) => !mapped.has(n)));
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
}

// Single call, at the BOTTOM of the module — every binding above is initialised.
init();
