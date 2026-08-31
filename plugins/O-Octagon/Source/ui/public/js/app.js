/*
   This file is part of O-Octagon, an Ouaricon Audio plugin.
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
// O-Octagon — WebView UI controller (Stage 3 Phase 3.1)
//
// The shell: two screens, the 18 parameter bindings, the readouts, the SAFE
// banner, the 2 Hz status poll and the venue-geometry cache. The Room plan
// itself lives in js/roomplan.js, which this file initialises from inside
// init().
//
// ── STRUCTURE IS LOAD-BEARING ─────────────────────────────────────────────
// Every module-level const/let is declared in the top block below, and the
// SINGLE init() call is the LAST statement in the file. A top-level statement
// that reaches a not-yet-initialised binding throws a ReferenceError out of
// module evaluation and silently kills the ENTIRE UI — not the one control that
// touched it (pattern_module_toplevel_init_tdz). Section 2 of
// ui_frontend_check.js asserts the form; the ui-stub render is what would catch
// it happening.
//
// ── The Juce ES-MODULE NAMESPACE, not window.__JUCE__ ─────────────────────
// The import below is the supported surface. Reaching for window.__JUCE__ and
// hand-rolling postMessage produces panels that go silently dead
// (critical_juce_webview_namespace_vs_postmessage).
//
// ── ALL 18 PARAMETERS ARE AudioParameterFloat ─────────────────────────────
// One relay family, one binding path, no Choice and no Bool (RESEARCH-3.1 F1).
// The relay-TYPE split that bit O-ReverseDelay three separate times cannot
// occur here, and neither this file nor the stub models a combo or a toggle.
//
// ── NATIVE-FUNCTION SURFACE IS EXACTLY EIGHTEEN (PLAN-3.3 P81) ────────────
//   read, from this file:
//     getParameterDefaults   once, at init      (dblclick reset)
//     getVenueGeometry       at init, then when venueGen moves — ONE call
//                            carrying all 50 venue values plus the envelope,
//                            bbox, centroid, hull, NAMED-SCENE MEMBERSHIP and
//                            the generation
//     getStatus              polled at 2 Hz     (SAFE + MAP banners, venueGen,
//                                                scenesGen)
//     getFieldGrid           at most once per status tick, when a field input
//                            moved (UI-04)
//   read, from js/meters.js:
//     getMeters              polled at ~30 Hz   (UI-03)
//   read and write, from js/scenes.js:
//     getScenes · applyScene · storeScene
//   write and read, from js/venue.js:
//     setVenue · saveVenue · loadVenue
//     savePreset · loadPreset · getPresetList · getCurrentPreset
//     startPing · stopPing · getPingState
// The count is grep-diffed against PluginEditor.cpp in BOTH directions by
// ui_frontend_check.js section 3, whose count literal moved 3 -> 13 at 3.2 and
// 13 -> 18 here, and FAILED LOUDLY until every one of the eighteen existed in
// all three places. An unregistered fn is a silently dead control that passes
// build, auval AND pluginval (pattern_webview_native_fn_bridge_gap) — and worse
// in JUCE 8 than that name suggests: an invocation naming an unregistered
// function hits jassertfalse; return, with NO completion, so in Release the
// promise never settles at all (RESEARCH-3.2 N4).
//
// UI-05 NEEDS NO NEW FUNCTION. getVenueGeometry already carries per-speaker z,
// rake.front / rake.rear, the bbox and the centroid — all landed by 3.2's P55 —
// so the elevation strip is a rendering job over a payload that already exists.
//
// ── NO NATIVE CALL IN A POINTER HANDLER ───────────────────────────────────
// getNativeFunction is an ASYNC ROUND TRIP whose promises can resolve out of
// order (juce_gui_extra/native/javascript/index.js:73-92). Calling it per
// pointermove would show a STALE metre value while the puck is current, at
// 60-120 JSON round trips a second, for a quantity that only changes when the
// venue changes. So: pull the geometry once, cache it, and refresh when
// venueGen moves on the poll that already exists for the SAFE banner
// (PLAN-3.1 P42). Section 14 of the static gate asserts the absence.
//
// ── READOUTS COME FROM THE SLIDER STATE ───────────────────────────────────
// getScaledValue() only. The FORMAT table below carries UNITS AND DECIMALS
// ONLY — no start, no end, no skew. A JS min/max map drifts from the C++
// NormalisableRange and the drift is invisible
// (pattern_webview_knob_readout_scaled_value). Metres come from the plugin's
// own live geometry, fetched from the plugin.
// ============================================================================

import * as Juce from "./juce/index.js";
import { createRoomPlan, normToMetres } from "./roomplan.js";
import { createVenueScreen } from "./venue.js";
import { createScenes } from "./scenes.js";
import { createMeters } from "./meters.js";
import { createField } from "./field.js";
import { createElevation } from "./elevation.js";

// v1.6.0 — the hover-help copy table. SINGLE QUOTES and this exact spelling:
// scripts/check-i18n.js assertion 6 requires the canonical import line verbatim
// so that 43 hand-copies of the i18n runtime cannot drift apart silently. The
// module EXPORTS ONLY and never self-executes, which is what lets a hoisted
// import be the only new top-level form here — section 2 of
// tests/ui_frontend_check.js requires init() to stay the last statement of this
// file and forbids any module-level declaration after it.
import { LANGUAGES, I18N, LABELS, TIP_BINDINGS, tr } from './i18n.js';

// ── Module-level bindings — ALL of them, declared here, used only inside
//    functions. Nothing below this block runs until init() is called at the
//    bottom of the file. ────────────────────────────────────────────────────

// The 18 APVTS ids, in oo::params order. ui_frontend_check.js section 16 closes
// this four ways — createParameterLayout == oo::params::id() == kSliderIds in
// PluginEditor.cpp == the `ctl-<id>` elements in index.html — so an id that
// lands in three of the four places fails loudly instead of going dead.
const PARAM_IDS = [
  "srcX", "srcY", "srcZ", "width", "decorr",
  "rolloff", "blur",
  "w1", "w2", "w3", "w4", "w5", "w6", "w7", "w8",
  "hullAtten", "airAmount",
  "outputGain",
  "motionOn", "motionPath", "motionSync", "motionRate", "motionSize",
  "motionRatio", "motionAngle", "motionHeight", "motionPhase", "motionSeed",
];

// v1.8.0 — THE THREE NON-FLOAT PARAMETERS, and the only place the page knows
// which they are. Everything else in PARAM_IDS is a WebSliderRelay; these three
// are a WebToggleButtonRelay and two WebComboBoxRelays (a host lane must read
// "Figure-8", not 0.2), reached through Juce.getToggleState / getComboBoxState.
// The split mirrors PluginEditor.cpp's relay loop and ui_frontend_check §16
// holds the two against each other.
const CONTROL_KIND = { motionOn: "toggle", motionPath: "combo", motionSync: "combo" };

// The six SHAPE parameters. A change to any of them re-fetches the trace;
// the anchor and the rate do not change the trace's shape (RESEARCH Q6).
const TRACE_SHAPE_IDS = ["motionPath", "motionSize", "motionRatio", "motionAngle", "motionHeight", "motionPhase"];

// The eight weight ids, which roomplan.js sites at their speakers rather than
// laying out in the controls column.
const WEIGHT_IDS = ["w1", "w2", "w3", "w4", "w5", "w6", "w7", "w8"];

// UNITS AND DECIMALS ONLY. If a range constant ever appears in this table it is
// a defect, and section 4 of the static gate fails on it.
const FORMAT = {
  srcX:       { unit: "",      dp: 3 },
  srcY:       { unit: "",      dp: 3 },
  srcZ:       { unit: "m",     dp: 2 },
  width:      { unit: "m",     dp: 2 },
  decorr:     { unit: "",      dp: 2 },
  rolloff:    { unit: "dB/2x", dp: 2 },
  blur:       { unit: "",      dp: 2 },
  w1:         { unit: "",      dp: 2 },
  w2:         { unit: "",      dp: 2 },
  w3:         { unit: "",      dp: 2 },
  w4:         { unit: "",      dp: 2 },
  w5:         { unit: "",      dp: 2 },
  w6:         { unit: "",      dp: 2 },
  w7:         { unit: "",      dp: 2 },
  w8:         { unit: "",      dp: 2 },
  hullAtten:  { unit: "dB/m",  dp: 2 },
  airAmount:  { unit: "",      dp: 2 },
  outputGain: { unit: "dB",    dp: 1 },
  motionRate:   { unit: "Hz",  dp: 2 },
  motionSize:   { unit: "m",   dp: 1 },
  motionRatio:  { unit: "",    dp: 2 },
  motionAngle:  { unit: "\u00B0", dp: 0 },
  motionHeight: { unit: "m",   dp: 1 },
  motionPhase:  { unit: "\u00B0", dp: 0 },
  motionSeed:   { unit: "",    dp: 0 },
};

// 2 Hz. A JS interval, deliberately NOT a juce::Timer: keeping the pull on this
// side is what lets the ui-stub render the page without modelling
// backend.addEventListener, and it keeps the editor free of a Timer.
const STATUS_POLL_MS = 500;

// ── EVERY IN-FLIGHT GUARD ON THIS PAGE RELEASES ON A DEADLINE (P71 / N9) ───
// A native completion is DROPPED, not rejected, when the browser is hidden
// (RESEARCH-3.2 N4). A dropped completion is neither an exception nor a
// rejection, so NEITHER `catch` NOR `finally` RUNS — the await never resumes
// and a flag cleared only in a `finally` stays true for the life of the page.
//
// RESEARCH-3.3 N9 MEASURED THAT IN SHIPPED 3.2 CODE, and it is worse than "the
// meters freeze": hiding the editor once during a venue change left the Room
// plan, the Venue table, the envelope readout and the metres readout
// permanently frozen on a stale venue, with no error anywhere. The measured
// signature was an envelope readout stuck at 15.60 x 19.50 m against a real
// 39.00 x 52.00 m, unrecovered through five poll ticks with the transport
// restored.
//
// So the guard carries a TIMESTAMP and is released when the outstanding request
// is older than this, whether or not it ever settles. Section 33 asserts the
// shape for every guard on the page; NC5 removes the deadline and watches the
// latch reproduce.
const GEOMETRY_GUARD_DEADLINE_MS = STATUS_POLL_MS * 6;

// The five inputs the DBAP field actually depends on (RESEARCH-3.3 N12).
// UI-04/2 names two — geometry and weights — but GainStage::updateControl shows
// three more, and ALL THREE ARE AUTOMATABLE AT AUDIO RATE, so a literal
// "recompute on change" would make a blur ramp recompute every block. Marking
// dirty here and spending it on the existing 2 Hz tick coalesces to at most one
// recompute per tick.
//
// srcX / srcY / srcZ / width are DELIBERATELY ABSENT: they move the source, not
// the field. That is why UI-04/2's assertion is the puck one, and it is exactly
// right. v1.5.0's decorr is absent for a STRONGER reason than those four: it
// does not touch the solve at all — it is a filter on the two feeds, downstream
// of every gain the field draws — so a redraw on its account would be a redraw
// of an identical field.
const FIELD_INPUT_IDS = [...WEIGHT_IDS, "rolloff", "blur", "hullAtten"];

const sliders = new Map();   // paramId -> { state, input, value }
const nativeFns = new Map(); // name    -> callable

let roomPlan = null;
let venueScreen = null;
let scenes = null;
let meters = null;
let field = null;
let elevation = null;
let geometry = null;
let cachedVenueGen = -1;
let cachedScenesGen = -1;
let geometryFetchInFlight = false;
let geometryFetchSince = 0;
let fieldDirty = true;
let paramDefaults = null;
let statusTimer = null;
let lastSafeMode = null;
let lastMapKey = null;

// ── Small helpers ──────────────────────────────────────────────────────────

function nativeFn(name) {
  if (!nativeFns.has(name)) nativeFns.set(name, Juce.getNativeFunction(name));
  return nativeFns.get(name);
}

function formatValue(id, scaled) {
  const f = FORMAT[id];
  if (f === undefined) return String(scaled);
  const text = Number(scaled).toFixed(f.dp);
  if (f.unit === "") return text;
  // The degree sign sits on the digit ("360°"), every other unit takes a space ("6.0 m").
  // Keyed on the unit rather than a FORMAT flag: section 4 of ui_frontend_check pins the table
  // to {unit, dp} so nothing range-shaped can creep in. v1.10.1 (IN-19).
  return f.unit === "\u00B0" ? `${text}${f.unit}` : `${text} ${f.unit}`;
}

// ── Parameter binding ──────────────────────────────────────────────────────
//
// The <input type="range"> elements are NORMALISED 0..1 and carry no range of
// their own — the C++ NormalisableRange is the only source of range and skew,
// reached through the SliderState. That is also why the readout below is
// getScaledValue() and never an arithmetic reconstruction.
//
// GESTURE BRACKETS ARE MANDATORY AND JUCE DOES NOT SUPPLY THEM.
// WebSliderParameterAttachment::sliderValueChanged calls setValueAsPartOfGesture
// — setValueNotifyingHost with NO beginChangeGesture / endChangeGesture
// (juce_ParameterAttachments.cpp:324 -> :76). The brackets come from
// sliderDragStarted / sliderDragEnded, which this side must send. Without them
// a host's Touch / Latch automation mode may MOVE THE SOUND AND NOT RECORD IT,
// and nothing in build, auval or pluginval can see the omission
// (RESEARCH-3.1 N1 / PLAN-3.1 P39).
//
// The rule, generalised so Phase 3.3's scenes inherit it: every interaction
// that writes a parameter OPENS a gesture on every parameter it will write and
// CLOSES every one of them — including on pointercancel and lostpointercapture.
// An interrupted drag that never closes leaves the host in an open write region.

function bindSlider(id) {
  const state = Juce.getSliderState(id);
  const input = document.getElementById(`ctl-${id}`);
  const value = document.getElementById(`val-${id}`);

  if (input === null) throw new Error(`missing control element ctl-${id}`);

  // `open` is the gesture flag AND the echo guard, and it is declared before
  // render() reads it. While a gesture is open the echo paints the READOUT but
  // does not rewrite input.value: a parameter that snaps would otherwise fight
  // the thumb under the user's own finger. closeGesture() re-syncs.
  let open = false;

  const render = () => {
    if (value !== null) value.textContent = formatValue(id, state.getScaledValue());
    if (!open) input.value = String(state.getNormalisedValue());
  };

  // RENDER ON ECHO; NEVER WRITE ON ECHO. WebSliderParameterAttachment::setValue
  // sets ignoreCallbacks and sliderValueChanged carries a jassertfalse on that
  // path (juce_ParameterAttachments.cpp:326); a listener that re-wrote a rounded
  // value could ping-pong. This listener only paints.
  state.valueChangedEvent.addListener(render);
  state.propertiesChangedEvent.addListener(render);

  // The write direction. `input` fires per pointer step and per key step.
  input.addEventListener("input", () => {
    state.setNormalisedValue(Number(input.value));
  });

  // ── The bracket pair ──
  const openGesture = () => {
    if (open) return;
    open = true;
    state.sliderDragStarted();
  };
  const closeGesture = () => {
    if (!open) return;
    open = false;
    state.sliderDragEnded();
    render();
  };

  input.addEventListener("pointerdown", openGesture);
  input.addEventListener("pointerup", closeGesture);
  input.addEventListener("pointercancel", closeGesture);
  input.addEventListener("lostpointercapture", closeGesture);
  input.addEventListener("keydown", openGesture);
  input.addEventListener("keyup", closeGesture);
  input.addEventListener("blur", closeGesture);

  // Dblclick reset. The default comes from getParameterDefaults in ENGINEERING
  // units and is converted back through the SAME live properties, so the round
  // trip is exact. A JS default table would be a second copy free to drift
  // (pattern_webview_knob_readout_scaled_value), and w1..w8 default to 1.0
  // rather than to their range minimum, which such a table gets wrong quietly.
  input.addEventListener("dblclick", () => {
    if (paramDefaults === null) return;
    const def = paramDefaults[id];
    if (typeof def !== "number") return;

    const p = state.properties;
    const span = p.end - p.start;
    if (!(Math.abs(span) > 0)) return;

    openGesture();
    state.setNormalisedValue(Math.pow((def - p.start) / span, p.skew));
    closeGesture();
  });

  sliders.set(id, { state });
  render();
}

// ── Screens ────────────────────────────────────────────────────────────────
// The tab labels are HTML-authored and are never rewritten; only aria-selected
// and the active class move (pattern_js_state_updater_overwrites_html_labels).

// ── v1.8.0 — the toggle and the combo ─────────────────────────────────────
//
// JUCE's WebToggleButtonParameterAttachment and WebComboBoxParameterAttachment
// route a JS write through setValueAsCompleteGesture, which DOES carry its own
// begin/end brackets (unlike the slider's setValueAsPartOfGesture) — so there
// is nothing for this side to open or close, and the stub records the pair on
// the write to keep section 10's bracket audit honest about that.
//
// The readout is the same dedicated `value` node the sliders use, so section 6
// sees no new textContent receiver. The <option>s are built from the state's
// OWN choices with the Option constructor — the C++ StringArray is the single
// list and nothing here transcribes it.

function bindToggle(id) {
  const state = Juce.getToggleState(id);
  const input = document.getElementById(`ctl-${id}`);
  const value = document.getElementById(`val-${id}`);

  if (input === null) throw new Error(`missing control element ctl-${id}`);

  const render = () => {
    const on = state.getValue() === true;
    // TWO CALLS BEHIND AN if/else, NEVER ONE WITH A TERNARY IN ITS ARGUMENT.
    // check-i18n assertion 13 rejects the ternary shape, because a conditional
    // inside a localized string is where a plural or a gender inflection gets
    // engineered instead of authored around (contract §6). setLabel() makes the
    // node a [data-i18n] element, so the language sweep re-renders this face on
    // a switch — as a bare literal it would have been stranded in English the
    // instant the selector fired while the toggle was on.
    if (value !== null) {
      if (on) setLabel(value, "label.on");
      else    setLabel(value, "label.off");
    }
    input.checked = on;
  };

  state.valueChangedEvent.addListener(render);
  state.propertiesChangedEvent.addListener(render);

  input.addEventListener("change", () => {
    state.setValue(input.checked);
  });

  sliders.set(id, { state, input, value, kind: "toggle" });
  render();
}

function bindCombo(id) {
  const state = Juce.getComboBoxState(id);
  const input = document.getElementById(`ctl-${id}`);
  const value = document.getElementById(`val-${id}`);

  if (input === null) throw new Error(`missing control element ctl-${id}`);

  let builtFrom = -1;

  const rebuildOptions = () => {
    const choices = Array.isArray(state.properties.choices) ? state.properties.choices : [];
    if (choices.length === builtFrom) return;
    builtFrom = choices.length;
    while (input.options.length > 0) input.remove(0);
    choices.forEach((name, i) => input.add(new Option(String(name), String(i))));
  };

  const render = () => {
    rebuildOptions();
    const i = state.getChoiceIndex();
    const name = state.properties.choices?.[i];
    if (value !== null) value.textContent = name === undefined ? "" : String(name);
    if (input.options.length > i) input.selectedIndex = i;
  };

  state.valueChangedEvent.addListener(render);
  state.propertiesChangedEvent.addListener(render);

  input.addEventListener("change", () => {
    state.setChoiceIndex(Number(input.value));
  });

  sliders.set(id, { state, input, value, kind: "combo" });
  render();
}

function bindControl(id) {
  const kind = CONTROL_KIND[id];
  if (kind === "toggle") bindToggle(id);
  else if (kind === "combo") bindCombo(id);
  else bindSlider(id);
}

// ── v1.8.0 — the Position | Motion tab pair ───────────────────────────────
// Pure view state. Both captions are authored; the switch is class + aria and
// the `hidden` attribute on the two panels. Seed shows only while Path is Drift
// and takes Phase's cell so the panel stays at three rows (index.html).
function bindGroupTabs() {
  const tabs = Array.from(document.querySelectorAll(".group-tab[data-panel]"));

  const select = (tab) => {
    for (const t of tabs) {
      const active = t === tab;
      t.classList.toggle("is-active", active);
      t.setAttribute("aria-selected", active ? "true" : "false");
      const panel = document.getElementById(t.dataset.panel);
      if (panel !== null) panel.hidden = !active;
    }
  };

  for (const t of tabs) t.addEventListener("click", () => select(t));
}

const DRIFT_PATH_INDEX = 3;

function bindSeedVisibility() {
  const path = sliders.get("motionPath");
  const seedCell = document.getElementById("cell-motionSeed");
  const phaseCell = document.getElementById("cell-motionPhase");
  if (path === undefined || seedCell === null || phaseCell === null) return;

  const render = () => {
    // Bound by INDEX, not by display name (v1.10.1, IN-16): the choice strings
    // arrive verbatim from the C++ StringArray and a localised label must never
    // be the lookup key. Drift is index 3 in PluginProcessor.cpp's pathChoices
    // ({Orbit, Figure-8, Sweep, Drift, Pendulum, Spiral}) and MotionPath.h's enum.
    const isDrift = path.state.getChoiceIndex() === DRIFT_PATH_INDEX;
    seedCell.hidden = !isDrift;
    phaseCell.hidden = isDrift;
  };

  path.state.valueChangedEvent.addListener(render);
  path.state.propertiesChangedEvent.addListener(render);
  render();
}

// ── v1.8.0 — the trace ────────────────────────────────────────────────────
// Points come WHOLE from the C++ generator (getMotionTrace, anchor-relative
// metres). This page never computes a path: roomplan.js adds the anchor's
// metres and projects through metresToPx. Coalesced to one fetch per task
// turn (a zero timeout, deliberately not an animation-frame callback —
// section 14 forbids a native call inside one, and a burst of six echoes on
// a preset load must become one fetch), and a SEQUENCE NUMBER drops an out-of-order
// completion — there is no in-flight flag to latch, so a dropped completion
// costs one stale trace, repaired by the next echo.
let traceSeq = 0;
let traceQueued = false;

function refreshTrace() {
  if (traceQueued) return;
  traceQueued = true;

  window.setTimeout(() => {
    traceQueued = false;
    const seq = ++traceSeq;

    Promise.resolve(nativeFn("getMotionTrace")())
      .then((payload) => {
        if (seq !== traceSeq) return;
        if (roomPlan !== null) roomPlan.setTrace(payload);
      })
      .catch((err) => console.error("getMotionTrace failed", err));
  });
}

function bindMotionView() {
  for (const id of TRACE_SHAPE_IDS) {
    const s = sliders.get(id);
    if (s !== undefined) s.state.valueChangedEvent.addListener(refreshTrace);
  }

  const on = sliders.get("motionOn");
  const dot = document.getElementById("motion-state");

  const renderOn = () => {
    const running = on !== undefined && on.state.getValue() === true;
    if (roomPlan !== null) roomPlan.setMotionOn(running);
    if (dot !== null) dot.classList.toggle("is-running", running);
    // v1.10.1 (IN-15): re-fetch on every motion-on. A completion dropped while the
    // editor was hidden (a Path echo landing on a closed window) left the previous
    // shape's trace with nothing but another shape echo to repair it; now the next
    // motion-on repairs it too. Coalesced and sequence-guarded like every other call.
    if (running) refreshTrace();
  };

  if (on !== undefined) on.state.valueChangedEvent.addListener(renderOn);
  renderOn();
  refreshTrace();
}

function bindScreens() {
  const tabs = Array.from(document.querySelectorAll(".screen-tab"));

  const show = (name) => {
    tabs.forEach((tab) => {
      const on = tab.dataset.screen === name;
      tab.classList.toggle("is-active", on);
      tab.setAttribute("aria-selected", on ? "true" : "false");
    });

    document.querySelectorAll(".screen").forEach((section) => {
      const on = section.id === `screen-${name}`;
      section.classList.toggle("is-active", on);
      section.hidden = !on;
    });

    // A plan box is fitted to a MEASURED container rect, and a hidden section
    // measures zero. Both plans — and, from 3.3, the elevation strip — re-lay
    // out on the way back to their screen.
    if (name === "room" && roomPlan !== null) roomPlan.relayout();
    if (name === "room" && elevation !== null) elevation.relayout();
    if (name === "venue" && venueScreen !== null) venueScreen.relayout();
  };

  tabs.forEach((tab) => tab.addEventListener("click", () => show(tab.dataset.screen)));
}

// ── Readouts resolved against the LIVE venue ───────────────────────────────
//
// UI-02 criterion 5. The metres are the plugin's own geometry, fetched from the
// plugin and cached; the denormalisation is normToMetres() in roomplan.js,
// which is the JS twin of oo::plane::normToMetres and takes its degenerate-axis
// decision from the payload FLAG rather than from a transcribed threshold. A
// naive min + n*(max-min) diverges from the C++ on exactly the degenerate
// venues Phase 2.1 spent a whole matrix on.

function renderMetres() {
  const el = document.getElementById("readout-metres");
  if (el === null) return;

  if (geometry === null) {
    el.textContent = "—";
    return;
  }

  const sx = sliders.get("srcX");
  const sy = sliders.get("srcY");
  if (sx === undefined || sy === undefined) return;

  const m = normToMetres(sx.state.getNormalisedValue(), sy.state.getNormalisedValue(), geometry);
  el.textContent = `${m.x.toFixed(2)} × ${m.y.toFixed(2)} m`;
}

function renderEnvelope() {
  const el = document.getElementById("readout-envelope");
  if (el === null || geometry === null) return;

  const e = geometry.envelope;
  el.textContent = `${(e.maxX - e.minX).toFixed(2)} × ${(e.maxY - e.minY).toFixed(2)} m`;
}

function renderVenueName() {
  const el = document.getElementById("venue-name");
  if (el === null || geometry === null) return;
  el.textContent = String(geometry.venueName ?? "");
}

// ── The negotiated output-set name (ROADMAP orphan 6) ──────────────────────
// It lives on the Venue rail but it is rendered from HERE, because getStatus is
// this file's poll and the name is a STATUS value, not a venue value. Stage 4's
// R2 reads it off this screen.
function renderSetName(status) {
  const el = document.getElementById("vset-name");
  if (el === null) return;
  el.textContent = String(status.outputSetName ?? "");
}

// ── The MAP INVALID banner, WITH its reason and its row (D13 / P54) ────────
//
// FRAME-LEVEL, so it is visible on the Room screen too. This is not a layout
// preference. An invalid map is AUDIBLE: mappedOutputAvailable() false sends
// GainStage to its else arm, which writes out[ch][n] = ch == 0 ? sL : sR with
// numWrite 8 — speaker 1 gets the left input and speakers 2 through 8 all get
// the right one, at unity (RESEARCH-3.2 N8). R1's defining property is silence;
// putting its only warning behind a tab the operator is not looking at
// reproduces exactly the failure it exists to break.
//
// The reason is built in C++ from the MapFailure enum, on the message thread —
// buildSpeakerToBuffer already separated three failure modes and threw the
// distinction away in a bool. In a hall, WHICH ROW is the actionable half.
// C++ SENDS A REASON CODE, NEVER PROSE, and that is what makes this banner
// localizable at all: the page maps the code to a KEY, and i18n.js owns the
// sentence in both languages. Through v1.8.0 this table held the English
// sentences directly, which would have left an invalid map explaining itself
// in English on a French page.
//
// SIX KEYS FOR THREE REASONS, not three keys with a sometimes-absent {n}. The
// row number is part of the sentence when there is one — "the row to fix" is
// the actionable half in a hall (PLAN-3.2 P54) — and a key that is sometimes
// parameterised needs a ternary at its call site, which assertion 13 rejects
// for the reason contract §6 gives.
const MAP_REASON_KEY = {
  notEightChannels: "map.notEightChannels",
  labelNotInSet: "map.labelNotInSet",
  duplicateLabel: "map.duplicateLabel",
};

function renderMapBanner(status) {
  const bad = status.mapInvalid === true;
  const reason = String(status.mapInvalidReason ?? "none");
  const speaker = Number(status.mapInvalidSpeaker);
  const key = `${bad}|${reason}|${speaker}`;

  if (key === lastMapKey) return;
  lastMapKey = key;

  const banner = document.getElementById("map-banner");
  if (banner !== null) banner.hidden = !bad;
  if (!bad) return;

  const el = document.getElementById("map-invalid-copy");
  if (el === null) return;

  // EVERY setLabel KEY IS A PLAIN STRING LITERAL. A computed key — the obvious
  // `setLabel(el, MAP_REASON_KEY[reason])` — cannot be checked by assertion 15
  // for danglingness, which is the same rule assertion 13 already applies. The
  // if/else ladder is the cost of a checkable key, and it is three branches.
  const hasRow = Number.isFinite(speaker) && speaker >= 0;
  const vars = { n: String(speaker + 1) };

  if (reason === "notEightChannels") {
    if (hasRow) setLabel(el, "map.notEightChannels.spk", vars);
    else        setLabel(el, "map.notEightChannels");
  } else if (reason === "labelNotInSet") {
    if (hasRow) setLabel(el, "map.labelNotInSet.spk", vars);
    else        setLabel(el, "map.labelNotInSet");
  } else if (reason === "duplicateLabel") {
    if (hasRow) setLabel(el, "map.duplicateLabel.spk", vars);
    else        setLabel(el, "map.duplicateLabel");
  } else {
    // A reason code this build does not know. Printing the CODE is right: it
    // is a diagnostic, not prose, and inventing a sentence for it would be
    // inventing a claim about a state the page cannot describe. It is not a
    // [data-i18n] element, so delete any stale mirror rather than leave one
    // disagreeing with the text — the invariant §6 and check-ui-labels
    // assertion 3 both assert is dataset.label === textContent.
    delete el.dataset.i18n;
    delete el.dataset.label;
    el.textContent = reason;
  }
}

// ── Monitor fold-down (v1.7.0) ─────────────────────────────────────────────
//
// THE PAGE NEVER DECIDES WHETHER THE MONITOR IS ARMED. It renders
// getStatus().monitorArmed and nothing else, exactly as the SAFE banner renders
// safeMode and never re-derives which sets count as SAFE (P43).
//
// That is not style here, it is correctness: the processor clears the arm on
// its own in four situations the page cannot see — a bypass, a SAFE-mode flip,
// a verify ping starting, and this editor closing — and suppresses the fold in
// a fifth, an offline render. A page that tracked its own boolean would keep
// showing MONITOR over a rig that had gone back to playing normally, which is
// the banner lying in the more dangerous direction.
let lastMonitorKey = "";

function renderMonitor(status) {
  const armed = status.monitorArmed === true;
  const suppressed = status.monitorSuppressed === true;
  const available = status.monitorAvailable !== false;

  // v1.11.0 — the stereo-bus binaural arm, two more facts from the SAME poll.
  // stereoBus makes the Headphones button live on a stereo insert; binaural is
  // what the audio thread DID last block, never the preference — a mono bus or
  // an F3 3-7 channel buffer keeps the preference and takes the dry fold.
  const stereoBus = status.stereoBus === true;
  const binaural = status.binauralActive === true;

  const key = `${armed}|${suppressed}|${available}|${stereoBus}|${binaural}`;
  if (key === lastMonitorKey) return;
  lastMonitorKey = key;

  // The banner is up whenever the arm is up — INCLUDING while suppressed. An
  // operator mid-bounce needs to be told the bounce is clean, and hiding the
  // banner exactly when the render is running would remove the reassurance at
  // the only moment it is wanted.
  //
  // The binaural arm lights the SAME banner: on a stereo bus it is the one
  // thing telling the operator the pair they hear is a fold of eight solved
  // feeds and not the rig — which is what the SAFE banner said, and SAFE is
  // hidden while this is up (applyStatus) so the header carries one claim.
  const banner = document.getElementById("monitor-banner");
  if (banner !== null) banner.hidden = !(armed || binaural);

  const monitorCopy = document.getElementById("monitor-copy");
  if (monitorCopy !== null) {
    if (binaural)        setLabel(monitorCopy, "monitor.binaural");
    else if (suppressed) setLabel(monitorCopy, "monitor.suppressed");
    else                 setLabel(monitorCopy, "monitor.folding");
  }

  // THE BUTTON'S CAPTION IS AUTHORED AND IS NEVER REWRITTEN — the armed state
  // is carried by aria-pressed plus the CSS fill, and the words live on the
  // advisory line below. That is this page's standing idiom (the tips toggle
  // and the ms/m unit toggle both refuse to relabel themselves), and it is what
  // keeps a shared state updater from ever erasing an authored label
  // (pattern_js_state_updater_overwrites_html_labels).
  //
  // On a stereo bus the button is ALWAYS live and toggles the binaural
  // preference instead of the monitor arm. The click handler in venue.js reads
  // data-stereo-bus, written here by the poll, to choose which native call to
  // make — never a local boolean (the same rule as aria-pressed).
  const btn = document.getElementById("monitor-toggle");
  if (btn !== null) {
    btn.setAttribute("aria-pressed", armed || binaural ? "true" : "false");
    btn.dataset.stereoBus = stereoBus ? "true" : "false";
    btn.disabled = stereoBus ? false : (!available && !armed);
  }

  const monitorNode = document.getElementById("vmonitor-state");
  if (monitorNode !== null) {
    if (binaural)        setLabel(monitorNode, "monitor.binaural.rail");
    else if (stereoBus)  setLabel(monitorNode, "monitor.off");
    else if (!available) setLabel(monitorNode, "monitor.unavailable");
    else if (!armed)     setLabel(monitorNode, "monitor.off");
    else if (suppressed) setLabel(monitorNode, "monitor.armed");
    else                 setLabel(monitorNode, "monitor.folding.rail");
  }
}

// ── Geometry cache ─────────────────────────────────────────────────────────
//
// ══ THIS FUNCTION IS REPAIRED AT 3.3, AND THE DEFECT WAS LIVE IN 3.2 ══════
//
// The 3.2 shape was `if (inFlight) return;` with the flag cleared in a
// `finally`. RESEARCH-3.3 N9 measured what that does when ONE completion is
// dropped: the finally never runs, the flag stays true forever, and every
// subsequent venueGen change is refused at the first line. The Room plan, the
// Venue table, the envelope readout and the metres readout all freeze on a
// stale venue with NO ERROR ANYWHERE. Measured signature: envelope stuck at
// 15.60 x 19.50 m against a real 39.00 x 52.00 m, not recovered across five
// poll ticks after the transport was restored.
//
// It is not caused by 3.3 and it is repaired by 3.3, because 3.3 is what makes
// the same shape load-bearing at 30 Hz.
//
// THE `finally` IS THE FAST PATH, NOT THE GUARANTEE. It releases a completion
// that arrives; the deadline releases one that never does. Both are required.
//
// AND DO NOT ADD A GUARD TO pollStatus (P71 rule 3). It has none, which is
// precisely why it is the one poll already safe: setInterval fires the next
// tick regardless, so a dropped tick leaks one pending promise and the poll
// SELF-HEALS. That leak is bounded and acceptable; the latch is not. "Tidying"
// pollStatus would convert the safe path into the broken one.

async function refreshGeometry() {
  const now = performance.now();

  if (geometryFetchInFlight && now - geometryFetchSince < GEOMETRY_GUARD_DEADLINE_MS)
    return;

  geometryFetchInFlight = true;
  geometryFetchSince = now;

  try {
    const payload = await nativeFn("getVenueGeometry")();
    if (payload === null || typeof payload !== "object") return;

    geometry = payload;
    cachedVenueGen = Number(payload.generation ?? cachedVenueGen);

    if (roomPlan !== null) roomPlan.setGeometry(geometry);
    if (venueScreen !== null) venueScreen.setGeometry(geometry);

    // Named-scene membership rides this payload (Q10 / P79): it is a pure
    // function of the venue, so it cannot go stale independently of the room
    // it describes, and it costs no nineteenth native function.
    if (scenes !== null) scenes.setGeometry(geometry);
    if (elevation !== null) elevation.setGeometry(geometry);

    // A venue change moves the field — it is input one of five (N12).
    fieldDirty = true;

    renderEnvelope();
    renderVenueName();
    renderMetres();
  } catch (err) {
    console.error("getVenueGeometry failed", err);
  } finally {
    geometryFetchInFlight = false;
  }
}

// ── The field legend ───────────────────────────────────────────────────────
// The dB span the plugin RETURNED, never re-derived from the decoded bytes.
// Without it the gradient is a picture that looks as though it carries
// information over a field that is genuinely flat (P69).
function renderFieldLegend() {
  const el = document.getElementById("field-legend");
  if (el === null || field === null) return;
  el.textContent = field.legendText();
}

// ── Status poll ────────────────────────────────────────────────────────────
// Two jobs on one round trip: the SAFE banner, and the venueGen comparison that
// invalidates the geometry cache when the venue changes from ANY source — a
// session restore, a .venue load, a host preset. That is the staleness hole
// P42 closes, and it costs nothing extra: the poll already exists for the
// banner and the counter already exists for the solver's dirty check.

function applyStatus(status) {
  if (status === null || typeof status !== "object") return;

  // v1.11.0: SAFE yields to the binaural arm. Both are true on a stereo bus
  // with the fold up; the MONITOR banner then carries the claim, and two
  // banners saying "not the rig" in different words would be noise.
  const safe = status.safeMode === true && status.binauralActive !== true;
  if (safe !== lastSafeMode) {
    lastSafeMode = safe;
    const banner = document.getElementById("safe-banner");
    if (banner !== null) banner.hidden = !safe;
  }

  renderMapBanner(status);
  renderMonitor(status);
  renderSetName(status);

  const gen = Number(status.venueGen);
  if (Number.isFinite(gen) && gen !== cachedVenueGen) refreshGeometry();

  // The USER slots are NOT a function of the venue, so they need their own
  // generation. scenesGen mirrors venueGen exactly, on the poll that already
  // exists, which is what keeps the four slots off a nineteenth native call
  // AND off a staleness hole (P79 / Q10).
  const sgen = Number(status.scenesGen);
  if (Number.isFinite(sgen) && sgen !== cachedScenesGen) {
    cachedScenesGen = sgen;
    if (scenes !== null) scenes.refreshSlots();
  }

  // ── THE FIELD RECOMPUTE IS SPENT HERE, AT MOST ONCE PER TICK (N12/P73) ──
  // Three of the field's five inputs are automatable at audio rate. Marking
  // dirty from the parameter echo and spending it on this existing 2 Hz tick
  // is what stops a blur ramp recomputing every block, and it is why UI-04/2's
  // assertion can stay on the puck: srcX/srcY/srcZ/width never set this flag.
  if (fieldDirty && field !== null) {
    fieldDirty = false;
    field.refresh();
  }
}

async function pollStatus() {
  try {
    applyStatus(await nativeFn("getStatus")());
  } catch (err) {
    console.error("getStatus failed", err);
  }
}

// ── hover-help language (v1.6.0) ───────────────────────────────────────────
//
// THIS BLOCK IS REPLICATED VERBATIM ACROSS EVERY LOCALIZED PLUGIN and is
// byte-compared (comments stripped, whitespace collapsed) against
// scripts/i18n-canon.js by scripts/check-i18n.js assertion 6. This repo has no
// shared UI module and deliberately does not gain one, so 43 hand-copies are
// only safe because a drifted copy fails a gate. Do not "tidy" it.
//
// One PULL at page init, no push, no timer, no poll().then(poll), no revision
// counter. The language is not preset content: OuariconPresetManager::loadPreset
// walks preset["parameters"] and never touches a state-tree property or a root
// XML attribute, so no preset path can change it. The pull is safe here for the
// reason RESEARCH B2 establishes and this session re-confirmed —
// `grep -rn setVisible plugins/O-Octagon/Source/` returns NOTHING, and the web
// view is addAndMakeVisible'd once at PluginEditor.cpp:1406 and never hidden,
// so the hidden-completion drop cannot fire.
//
// Declared here at module level, ABOVE every reader and BELOW nothing that runs
// — the only statement executed at module-evaluation time is the
// window.__setLanguage assignment, which touches a hoisted function declaration
// and cannot enter a TDZ chain. initI18n() itself is called from INSIDE init(),
// which is section 2's requirement.

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
            .then((code) => applyI18n(code === 'fr' ? 'fr' : 'en'))
            .catch((e) => console.warn('Could not read language preference:', e));
    }

    const sel = document.getElementById('lang-select');
    if (sel) sel.addEventListener('change', (e) => {
        applyI18n(e.target.value);
        if (setUiLanguageNative) setUiLanguageNative(uiLanguage).catch(() => {});
    });
}

// ── the settings popover (v1.6.0) ──────────────────────────────────────────
//
// The gear that carries the language selector and the hover-help toggle. All
// state lives in this closure, so nothing here can join a TDZ chain.

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
  // click, so the panel is gone before a drag on a control underneath it
  // begins — the puck calls preventDefault in its own pointerdown.
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

// ── hover help (v1.2.0) ────────────────────────────────────────────────────
//
// Ported from O-Contrabass v1.7.0, which carries the VERIFIED measure-then-pin
// placement rather than the earlier shrink-to-fit variant
// (pattern_fixed_tooltip_shrink_to_fit_edge).
//
// Every show is gated on `tipsEnabled`, the "?" toggle's state, which
// round-trips through the processor so it survives a session reload. The
// toggle's OWN tip carries data-tip-always and bypasses the gate — otherwise
// the one control that could turn help back on would be the one control unable
// to explain itself.
//
// All state lives in this closure — no module-level bindings, so nothing here
// can join a TDZ chain (pattern_module_toplevel_init_tdz).

function initHoverHelp() {
  // TOOLTIP_MARGIN is both the gap between a tip and its control AND the gap
  // it keeps from the viewport edge.
  const TOOLTIP_MARGIN = 8;
  const TOOLTIP_DELAY_MS = 350; // hover dwell before a tip appears

  const tipEl = document.getElementById("tooltip");

  // v1.6.0 — the toggle MOVED into the settings popover and became a segmented
  // On/Off pair, the same idiom the Venue screen's ms/m unit toggle uses. TWO
  // BUTTONS, NOT ONE THAT RELABELS ITSELF: both captions are HTML-authored and
  // neither is ever rewritten, so this module still makes no textContent write
  // (pattern_js_state_updater_overwrites_html_labels, section 6 of the static
  // gate). The button's own tip is now ONE key in js/i18n.js covering both
  // states rather than a pair of hard-coded sentences swapped on click —
  // applyI18n() re-renders every tip from the table on a language change, and
  // would have overwritten a state-dependent string written here, stranding it
  // in the previous language.
  const tipsButtons = {
    on: document.getElementById("btn-tips-on"),
    off: document.getElementById("btn-tips-off"),
  };

  let tipTimer = null;
  let tipTarget = null;
  let tipSuppressed = false; // true between pointerdown and pointerup
  let tipsEnabled = false;   // the "?" state; PULLED from C++ at init

  const tipAllowed = (t) => tipsEnabled || t.hasAttribute("data-tip-always");

  const hideTip = () => {
    clearTimeout(tipTimer);
    tipTarget = null;
    if (tipEl === null) return;
    tipEl.classList.remove("visible");
    tipEl.setAttribute("aria-hidden", "true");
  };

  const showTip = (target) => {
    // The pointer may have moved on, or gone down, during the dwell.
    if (tipEl === null || tipSuppressed || target !== tipTarget) return;
    if (!tipAllowed(target)) return;

    const title = target.getAttribute("data-tip-title");
    const body = target.getAttribute("data-tip");
    if (body === null || body === "") return;

    // textContent, not innerHTML — the copy stays inert. The tooltip's OWN
    // nodes are the one place this module writes text, and they are never a
    // label anything else authored (pattern_js_state_updater_overwrites_html_labels).
    tipEl.textContent = "";
    if (title !== null && title !== "") {
      const t = document.createElement("div");
      t.className = "tooltip-title";
      t.textContent = title;
      tipEl.appendChild(t);
    }
    const b = document.createElement("div");
    b.className = "tooltip-body";
    b.textContent = body;
    tipEl.appendChild(b);

    const anchor = target.getBoundingClientRect();

    // MEASURE-THEN-PIN. A fixed-position box with `left` set and width:auto
    // shrink-to-fits whatever space remains to its right, so measuring at the
    // PREVIOUS offset under-reports the width, and a near-edge `left` re-wraps
    // the tip into a narrow ribbon that never recovers. Release the width,
    // measure from the left edge, pin the result in px, and only then place.
    tipEl.style.width = "";
    tipEl.style.left = "0px";
    tipEl.style.top = "0px";

    // getBoundingClientRect, NOT offsetWidth: offsetWidth rounds, and a pinned
    // width narrower than the box's own shrink-to-fit pushes the last word
    // onto a second line after the height was already read.
    const width = tipEl.getBoundingClientRect().width;
    tipEl.style.width = `${width}px`;

    // Height is only stable once the width is definite.
    const height = tipEl.getBoundingClientRect().height;

    // Prefer above; flip below only when there is no room at the top.
    let top = anchor.top - height - TOOLTIP_MARGIN;
    let placement = "above";
    if (top < TOOLTIP_MARGIN) {
      top = anchor.bottom + TOOLTIP_MARGIN;
      placement = "below";
    }

    const centreX = anchor.left + anchor.width / 2;
    const maxLeft = window.innerWidth - width - TOOLTIP_MARGIN;
    const left = Math.max(TOOLTIP_MARGIN, Math.min(maxLeft, centreX - width / 2));

    tipEl.style.left = `${left}px`;
    tipEl.style.top = `${top}px`;
    tipEl.dataset.placement = placement;

    // The tip is clamped to the viewport, but the arrow still points at the
    // control — held clear of the rounded corners.
    const arrowX = Math.max(10, Math.min(width - 10, centreX - left));
    tipEl.style.setProperty("--arrow-x", `${arrowX}px`);

    tipEl.classList.add("visible");
    tipEl.setAttribute("aria-hidden", "false");
  };

  // Class + aria only — both captions are HTML-authored and must never be
  // written from here (pattern_js_state_updater_overwrites_html_labels).
  const applyTipsEnabled = (enabled) => {
    tipsEnabled = enabled === true;
    for (const key of ["on", "off"]) {
      const btn = tipsButtons[key];
      if (btn === null) continue;
      const active = (key === "on") === tipsEnabled;
      btn.classList.toggle("is-active", active);
      btn.setAttribute("aria-pressed", active ? "true" : "false");
    }
    if (!tipsEnabled) hideTip();
  };

  if (tipEl === null) {
    console.warn("tooltip surface missing — hover help disabled");
    return;
  }

  document.addEventListener("mouseover", (e) => {
    const target = e.target.closest !== undefined ? e.target.closest("[data-tip]") : null;
    if (target === null || target === tipTarget) return;
    tipTarget = target;
    clearTimeout(tipTimer);
    if (tipSuppressed || !tipAllowed(target)) return;
    tipTimer = setTimeout(() => showTip(target), TOOLTIP_DELAY_MS);
  });

  document.addEventListener("mouseout", (e) => {
    const target = e.target.closest !== undefined ? e.target.closest("[data-tip]") : null;
    if (target === null) return;
    // Moving between children of the same control is not a real exit — every
    // cell wraps a label, a range and a value readout, so without this the
    // surface flickers as the pointer crosses them.
    if (e.relatedTarget !== null && target.contains(e.relatedTarget)) return;
    hideTip();
  });

  // Any press begins a click or a drag: get the tip out of the way and keep it
  // away until release, so it cannot hang over the puck mid-drag. CAPTURE
  // phase, because the puck calls preventDefault in its own pointerdown.
  document.addEventListener("pointerdown", () => {
    tipSuppressed = true;
    hideTip();
  }, true);
  document.addEventListener("pointerup", () => { tipSuppressed = false; }, true);

  // Each button sets its OWN state rather than inverting the current one, so a
  // second click on the already-active half is a no-op instead of a flip.
  for (const key of ["on", "off"]) {
    const btn = tipsButtons[key];
    if (btn === null) continue;
    btn.addEventListener("click", () => {
      const wanted = key === "on";
      if (wanted === tipsEnabled) return;
      applyTipsEnabled(wanted);
      // Fire-and-forget: the page is already in the new state, and a failed
      // persist must not leave the toggle disagreeing with what it shows.
      nativeFn("setTooltipsEnabled")(tipsEnabled).catch(() => {});
    });
  }

  // PULL the persisted preference. The C++ side deliberately never pushes it:
  // a push from the editor constructor or a poll tick fires before this module
  // has evaluated, so it would silently never arrive and the toggle would read
  // OFF on every reopen (pattern_webview_one_shot_state_push_stale_on_preset_load).
  nativeFn("getTooltipsEnabled")()
    .then((enabled) => applyTipsEnabled(enabled === true))
    .catch((err) => console.error("getTooltipsEnabled failed", err));
}

// ── init ───────────────────────────────────────────────────────────────────

async function init() {
  bindScreens();

  for (const id of PARAM_IDS) bindControl(id);

  // v1.8.0. View-only wiring, hoisted under the same try/catch discipline as
  // the plan: a throw here must not take the 28 bindings down.
  try {
    bindGroupTabs();
    bindSeedVisibility();
  } catch (err) {
    console.error("motion panel failed to initialise", err);
  }

  // The plan is initialised INSIDE init(), hoisted into its own try/catch, so a
  // failure to draw the room cannot take the 18 bindings down with it. Above
  // this line every control is already live.
  try {
    roomPlan = createRoomPlan({
      sliders,
      weightIds: WEIGHT_IDS,
      onSourceMoved: renderMetres,

      // v1.1.0 — the popover's write path. Two integers to C++, which owns the
      // device-order table and the swap (D19). No state is established here:
      // badges and the venue table converge on the venueGen poll (P64), and a
      // dropped completion costs only this log line.
      assignOutput: (speakerN, outputK) =>
        nativeFn("assignSpeakerOutput")(speakerN, outputK)
          .catch((err) => console.error("assignSpeakerOutput failed", err)),
    });
  } catch (err) {
    console.error("room plan failed to initialise", err);
    roomPlan = null;
  }

  // Same discipline, one screen along: a Venue failure must not take the Room
  // screen or the 18 bindings down. Both are hoisted into init() and both are
  // wrapped, because D4 removed the browser-iteration safety net for the whole
  // stage — a throw out of either constructor would otherwise first surface
  // inside a plugin.
  try {
    venueScreen = createVenueScreen({ nativeFn });
  } catch (err) {
    console.error("venue screen failed to initialise", err);
    venueScreen = null;
  }

  // ── Phase 3.3's four modules ────────────────────────────────────────────
  // Each in its OWN try/catch, for the reason 3.1 and 3.2 established and D4
  // sharpened: a throw out of any one constructor must not take the 18
  // bindings, the Room plan or the Venue screen down with it. Above this line
  // every control is already live, and each block below leaves its own module
  // null on failure rather than aborting init().

  try {
    scenes = createScenes({
      nativeFn,
      // The plan RENDERS the resolved set; it never computes one (D19 / P79).
      onPreview: (indices) => { if (roomPlan !== null) roomPlan.setPreview(indices); },
    });
  } catch (err) {
    console.error("scenes failed to initialise", err);
    scenes = null;
  }

  try {
    field = createField({
      nativeFn,
      onUpdated: () => {
        if (roomPlan !== null) roomPlan.redrawBackdrop();
        renderFieldLegend();
      },
    });

    // UI-04 criterion 4 IS THIS LINE. The backdrop is a separate canvas layer
    // that has existed since 3.1 for exactly this, and descoping the field
    // means not installing the painter — no other component is touched.
    if (roomPlan !== null) roomPlan.setFieldPainter(field.drawInto);
  } catch (err) {
    console.error("field failed to initialise", err);
    field = null;
  }

  try {
    elevation = createElevation({ sliders });
  } catch (err) {
    console.error("elevation failed to initialise", err);
    elevation = null;
  }

  // v1.6.0 — the settings popover, which carries the language selector and the
  // hover-help toggle. Same discipline: its own try/catch, so a throw out of
  // the panel wiring cannot take the 18 bindings down.
  try {
    initSettingsPopover();
  } catch (err) {
    console.error("settings popover failed to initialise", err);
  }

  // v1.6.0 — the hover-help copy, English or French, written onto the page as
  // data-tip / data-tip-title attributes. Called from HERE and not from module
  // top level: section 2 requires init() to remain the last statement of this
  // file with no module-level declaration after it, and the v1.4.0 lesson from
  // O-MultiBandCompressor is that a table typo in an eager top-level init
  // throws a TDZ ReferenceError that silently kills unrelated working code.
  // English is painted synchronously inside initI18n() before the native pull
  // resolves, so a tip is never blank and never flashes the wrong language.
  //
  // BEFORE initHoverHelp(), so the attributes exist before the first hover can
  // read them. (The order is not load-bearing — the renderer reads the
  // attributes at show time — but it keeps the DOM consistent from the first
  // frame.)
  try {
    initI18n();
  } catch (err) {
    console.error("hover-help copy failed to initialise", err);
  }

  // v1.2.0 — hover help, in its own try/catch under the same discipline: a
  // throw out of the tooltip wiring must not take the bindings down.
  try {
    initHoverHelp();
  } catch (err) {
    console.error("hover help failed to initialise", err);
  }

  try {
    meters = createMeters({
      nativeFn,
      onLevels: (levels, peaks, hot) => {
        if (roomPlan !== null) roomPlan.setMeters(levels, peaks, hot);
      },

      // v1.8.0 — the live puck rides THIS poll, not getStatus: a puck at 2 Hz
      // stutters (RESEARCH Q6). Three floats and a flag, straight through.
      onMotion: (offset, running) => {
        if (roomPlan !== null) roomPlan.setMotion(offset, running);
        if (elevation !== null) elevation.setMotion(offset, running);
      },
    });
  } catch (err) {
    console.error("meters failed to initialise", err);
    meters = null;
  }

  // The field's five inputs (N12). Marking dirty on the ECHO rather than on the
  // write catches host automation and preset loads too, which a listener on the
  // DOM `input` event would miss entirely.
  for (const id of FIELD_INPUT_IDS) {
    const s = sliders.get(id);
    if (s !== undefined) s.state.valueChangedEvent.addListener(() => { fieldDirty = true; });
  }

  // ── THE FOOTER METRES READOUT RIDES THE SAME ECHO (CODE_REVIEW WR-04) ────
  //
  // renderMetres() is a pure function of srcX and srcY (it reads both slider
  // states and runs normToMetres), so it has to re-run whenever either moves.
  // Until v1.3.2 its only live-update wiring was roomPlan's onSourceMoved
  // callback, and roomplan.js calls that from ONE place: the puck's pointermove
  // handler. Every other way the source moves — dragging ctl-srcX / ctl-srcY,
  // stepping them from the keyboard, their dblclick reset, or host automation —
  // moved the puck (renderPuck is on this same echo) and updated val-srcX, while
  // the footer went on showing a plausible WRONG position in metres until the
  // next venue change or editor reopen.
  //
  // index.html's Source X tooltip says "the metres readout below is live".
  // Dragging that exact slider was the case where it was not, and UI-02
  // criterion 5 is the criterion it broke. elevation.js:343 already subscribes
  // its marker to this echo — the footer was the one module out of step with its
  // siblings.
  //
  // Same idiom as the FIELD_INPUT_IDS loop above: render on the echo, never
  // write on it.
  for (const id of ["srcX", "srcY"]) {
    const s = sliders.get(id);
    if (s !== undefined) s.state.valueChangedEvent.addListener(renderMetres);
  }

  try {
    paramDefaults = await nativeFn("getParameterDefaults")();
  } catch (err) {
    console.error("getParameterDefaults failed", err);
    paramDefaults = null;
  }

  // v1.8.0. After the plan and the elevation strip exist and before the first
  // geometry arrives — the trace re-renders on the geometry it is handed.
  try {
    bindMotionView();
  } catch (err) {
    console.error("motion view failed to initialise", err);
  }

  await refreshGeometry();
  await pollStatus();

  if (scenes !== null) await scenes.refreshSlots();

  statusTimer = window.setInterval(pollStatus, STATUS_POLL_MS);

  // TWO POLLS, TWO RATES, AND THE SPLIT IS MEASURED (Q4 / P77). getStatus stays
  // at 2 Hz because it builds a juce::String from
  // getBus(false,0)->getCurrentLayout().getDescription() on EVERY call
  // (PluginEditor.cpp) — at 30 Hz that is thirty string constructions a second
  // on the message thread for a value that changes only on renegotiation.
  // getMeters carries eight floats and nothing else.
  if (meters !== null) meters.start();

  window.addEventListener("pagehide", () => {
    window.clearInterval(statusTimer);
    if (meters !== null) meters.stop();
  });

  window.addEventListener("resize", () => {
    if (roomPlan !== null) roomPlan.relayout();
    if (venueScreen !== null) venueScreen.relayout();
    if (elevation !== null) elevation.relayout();
  });

  renderMetres();
}

// THE LAST STATEMENT IN THE FILE, and the only top-level call. Nothing above
// this line executes at module-evaluation time except the declarations
// themselves (pattern_module_toplevel_init_tdz).
init();
