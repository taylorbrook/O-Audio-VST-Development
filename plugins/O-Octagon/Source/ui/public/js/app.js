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
// The shell: two screens, the 17 parameter bindings, the readouts, the SAFE
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
// ── ALL 17 PARAMETERS ARE AudioParameterFloat ─────────────────────────────
// One relay family, one binding path, no Choice and no Bool (RESEARCH-3.1 F1).
// The relay-TYPE split that bit O-ReverseDelay three separate times cannot
// occur here, and neither this file nor the stub models a combo or a toggle.
//
// ── NATIVE-FUNCTION SURFACE IS EXACTLY EIGHTEEN (PLAN-3.3 P81) ────────────
//   read, from this file:
//     getParameterDefaults   once, at init      (dblclick reset)
//     getVenueGeometry       at init, then when venueGen moves — ONE call
//                            carrying all 42 venue values plus the envelope,
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

// ── Module-level bindings — ALL of them, declared here, used only inside
//    functions. Nothing below this block runs until init() is called at the
//    bottom of the file. ────────────────────────────────────────────────────

// The 17 APVTS ids, in oo::params order. ui_frontend_check.js section 16 closes
// this four ways — createParameterLayout == oo::params::id() == kSliderIds in
// PluginEditor.cpp == the `ctl-<id>` elements in index.html — so an id that
// lands in three of the four places fails loudly instead of going dead.
const PARAM_IDS = [
  "srcX", "srcY", "srcZ", "width",
  "rolloff", "blur",
  "w1", "w2", "w3", "w4", "w5", "w6", "w7", "w8",
  "hullAtten", "airAmount",
  "outputGain",
];

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
// right.
const FIELD_INPUT_IDS = [
  "w1", "w2", "w3", "w4", "w5", "w6", "w7", "w8",
  "rolloff", "blur", "hullAtten",
];

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
  return f.unit === "" ? text : `${text} ${f.unit}`;
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

  sliders.set(id, { state, input, value });
  render();
}

// ── Screens ────────────────────────────────────────────────────────────────
// The tab labels are HTML-authored and are never rewritten; only aria-selected
// and the active class move (pattern_js_state_updater_overwrites_html_labels).

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
const MAP_REASON_COPY = {
  notEightChannels: "output set is not 8 channels",
  labelNotInSet: "label not in the negotiated set",
  duplicateLabel: "duplicate label",
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

  const copy = MAP_REASON_COPY[reason] ?? reason;
  el.textContent = Number.isFinite(speaker) && speaker >= 0
    ? `${copy} — speaker ${speaker + 1}`
    : copy;
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

  const safe = status.safeMode === true;
  if (safe !== lastSafeMode) {
    lastSafeMode = safe;
    const banner = document.getElementById("safe-banner");
    if (banner !== null) banner.hidden = !safe;
  }

  renderMapBanner(status);
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
  const toggleEl = document.getElementById("help-toggle");

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

  // Class + aria only — the button's "?" glyph is HTML-authored and must never
  // be written from here (pattern_js_state_updater_overwrites_html_labels).
  const applyTipsEnabled = (enabled) => {
    tipsEnabled = enabled === true;
    if (toggleEl !== null) {
      toggleEl.classList.toggle("is-active", tipsEnabled);
      toggleEl.setAttribute("aria-pressed", tipsEnabled ? "true" : "false");
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

  if (toggleEl !== null) {
    toggleEl.addEventListener("click", () => {
      applyTipsEnabled(!tipsEnabled);
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

  for (const id of PARAM_IDS) bindSlider(id);

  // The plan is initialised INSIDE init(), hoisted into its own try/catch, so a
  // failure to draw the room cannot take the 17 bindings down with it. Above
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
  // screen or the 17 bindings down. Both are hoisted into init() and both are
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
  // sharpened: a throw out of any one constructor must not take the 17
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

  try {
    paramDefaults = await nativeFn("getParameterDefaults")();
  } catch (err) {
    console.error("getParameterDefaults failed", err);
    paramDefaults = null;
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
