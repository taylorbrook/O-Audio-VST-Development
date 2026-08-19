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
// Minimal JUCE WebView bridge stub — substitutes for js/juce/index.js so
// Source/ui/public/index.html renders in a plain browser.
//
// This is the "mockup gate": rendering the real page catches the failure modes
// that C++ build / auval / pluginval cannot see —
//   1. a top-level TDZ ReferenceError killing the whole UI
//      (pattern_module_toplevel_init_tdz)
//   2. a state updater overwriting HTML-authored labels
//      (pattern_js_state_updater_overwrites_html_labels)
//   3. tooltip geometry, which is pure WebView layout
//      (pattern_fixed_tooltip_shrink_to_fit_edge)
//
// Ranges here mirror createParameterLayout() so the stub render is
// representative; they are TEST FIXTURES ONLY and are never shipped. A fixture
// that silently drifts from the plugin keeps passing while it measures the
// wrong page (pattern_test_fixture_mirrors_drift_silently), so every entry
// below names the C++ line it mirrors.
// ============================================================================

function listenerList() {
  const listeners = [];
  return {
    addListener: (fn) => listeners.push(fn),
    callListeners: () => listeners.forEach((fn) => fn()),
  };
}

// freeMsRange(): 10 ms .. kMaxGestureSeconds*1000, skew 0.35. The 8000 ms
// ceiling is DERIVED in C++ from kMaxGestureSeconds so the ring-sizing
// static_assert and the range can never drift apart; spelled out here.
const FREE_MS = { start: 10, end: 8000, skew: 0.35, interval: 0.01 };
// percentRange(): 0..100 linear.
const PCT = { start: 0, end: 100, skew: 1, interval: 0.1 };

const RANGES = {
  STOP_FREE_MS:  { ...FREE_MS, def: 500 },
  STOP_CURVE:    { ...PCT,     def: 50 },
  START_FREE_MS: { ...FREE_MS, def: 250 },
  START_CURVE:   { ...PCT,     def: 50 },
  ENV_FREE_MS:   { ...FREE_MS, def: 1000 },

  // CONT_RATE_HZ is the one skewed non-ms range: 0.05..20 Hz, skew 0.3, and
  // its default 1.2 Hz is NOT the range minimum — a stub defaulting to 0.05
  // would render the page showing a state the plugin never ships in.
  CONT_RATE_HZ:  { start: 0.05, end: 20, skew: 0.3, interval: 0.01, def: 1.2 },
  CONT_DEPTH:    { ...PCT, def: 35 },
  CONT_CHAOS:    { ...PCT, def: 20 },

  TONE_TRACK:    { ...PCT, def: 60 },
  MIX:           { ...PCT, def: 100 },
  // The only bipolar range on the page: -24..+12 dB, linear, neutral at 0.
  OUTPUT_GAIN:   { start: -24, end: 12, skew: 1, interval: 0.1, def: 0 },
};

// ENGAGE is the plugin's only bool. It needs its own map because a ToggleState
// carries a boolean, not a scaled value; routing it through RANGES would hand
// app.js a SliderState for a parameter that has none — which in a real host is
// a switch that never updates.
const TOGGLES = { ENGAGE: false };

// Must match the C++ StringArrays exactly. ORDER is load-bearing, not
// cosmetic: MODE's "Continuous" was APPENDED in v1.1 precisely so indices 0
// and 1 kept their meaning for existing sessions.
const CHOICES = {
  MODE:      ["Stop", "Scratch", "Continuous"],
  SYNC_MODE: ["Sync", "Free"],
  CHARACTER: ["Wobble", "Random", "Glitch"],
  STOP_SYNC_DIV:      ["1/16", "1/8", "1/4", "1/2", "1 bar", "2 bars", "4 bars"],
  START_SYNC_DIV:     ["1/16", "1/8", "1/4", "1/2", "1 bar", "2 bars", "4 bars"],
  ENV_SYNC_DIV:       ["1/16", "1/8", "1/4", "1/2", "1 bar", "2 bars", "4 bars"],
  CONT_RATE_SYNC_DIV: ["1/16", "1/8", "1/4", "1/2", "1 bar", "2 bars", "4 bars"],
};

// Defaults are INDICES, and three of the four divisions differ — copying one
// value across all of them would render a page the plugin never ships.
const DEFAULT_CHOICE = {
  MODE: 0, SYNC_MODE: 0, CHARACTER: 0,
  STOP_SYNC_DIV: 3,        // 1/2
  START_SYNC_DIV: 2,       // 1/4
  ENV_SYNC_DIV: 4,         // 1 bar
  CONT_RATE_SYNC_DIV: 2,   // 1/4
};

class StubSliderState {
  constructor(name) {
    const r = RANGES[name] || { start: 0, end: 1, skew: 1, interval: 0, def: 0 };
    this.name = name;
    this.properties = {
      start: r.start, end: r.end, skew: r.skew,
      name, label: "", numSteps: 100, interval: r.interval, parameterIndex: 0,
    };
    this.scaledValue = r.def;
    this.valueChangedEvent = listenerList();
    this.propertiesChangedEvent = listenerList();
  }
  getScaledValue() { return this.scaledValue; }
  getNormalisedValue() {
    const p = this.properties;
    return Math.pow((this.scaledValue - p.start) / (p.end - p.start), p.skew);
  }
  setNormalisedValue(n) {
    const p = this.properties;
    this.scaledValue = Math.pow(n, 1 / p.skew) * (p.end - p.start) + p.start;
    this.valueChangedEvent.callListeners();
  }
  sliderDragStarted() {}
  sliderDragEnded() {}
}

class StubComboBoxState {
  constructor(name) {
    this.name = name;
    this.properties = { name, parameterIndex: 0, choices: CHOICES[name] || [] };
    this.choiceIndex = DEFAULT_CHOICE[name] ?? 0;
    this.valueChangedEvent = listenerList();
    this.propertiesChangedEvent = listenerList();
  }
  getChoiceIndex() { return this.choiceIndex; }
  setChoiceIndex(i) { this.choiceIndex = i; this.valueChangedEvent.callListeners(); }
}

class StubToggleState {
  constructor(name) {
    this.name = name;
    this.properties = { name, parameterIndex: 0 };
    this.value = TOGGLES[name] ?? false;
    this.valueChangedEvent = listenerList();
    this.propertiesChangedEvent = listenerList();
  }
  getValue() { return this.value; }
  setValue(v) { this.value = v; this.valueChangedEvent.callListeners(); }
}

const sliderStates = new Map();
const comboStates  = new Map();
const toggleStates = new Map();

export function getSliderState(name) {
  if (!sliderStates.has(name)) sliderStates.set(name, new StubSliderState(name));
  return sliderStates.get(name);
}

export function getComboBoxState(name) {
  if (!comboStates.has(name)) comboStates.set(name, new StubComboBoxState(name));
  return comboStates.get(name);
}

export function getToggleState(name) {
  if (!toggleStates.has(name)) toggleStates.set(name, new StubToggleState(name));
  return toggleStates.get(name);
}

// ── Backend event stub ──────────────────────────────────────────────────────
// app.js subscribes to "transportFrame" and "envelopeState" on
// window.__JUCE__.backend — the sanctioned window.__JUCE__ use (backend events
// only; parameter state goes through the ES-module namespace above,
// critical_juce_webview_namespace_vs_postmessage). preset-manager.js also
// polls for window.__JUCE__.backend before doing anything, and without this
// shim its _waitForNative() burns 100 x 50 ms and then console.errors, which
// both fails the zero-console-errors gate and adds 5 s to every run.
// emit() is exposed so a harness can drive the ratio bar and playhead.
if (typeof window !== "undefined") {
  const handlers = new Map();
  window.__JUCE__ = {
    backend: {
      addEventListener: (name, fn) => {
        if (!handlers.has(name)) handlers.set(name, []);
        handlers.get(name).push(fn);
      },
      removeEventListener: (name, fn) => {
        const list = handlers.get(name) || [];
        const i = list.indexOf(fn);
        if (i >= 0) list.splice(i, 1);
      },
      emitEvent: (name, payload) => {
        (handlers.get(name) || []).forEach((fn) => fn(payload));
      },
    },
  };
  window.__stubEmit = (name, payload) => window.__JUCE__.backend.emitEvent(name, payload);
}

// ── Envelope backend stub ───────────────────────────────────────────────────
// The default wobble, verbatim from kDefaultWobbleEnv. commitEnvelope echoes
// back what it was handed, standing in for the C++ sanitize pass; the harness
// asserts round-trip wiring, not sanitizer behaviour (the C++ owns that).
let envelopeJson =
  '{"v":1,"points":[{"x":0.0,"y":0.5,"curve":0.0},{"x":0.25,"y":0.65,"curve":0.0},' +
  '{"x":0.5,"y":0.35,"curve":0.0},{"x":0.75,"y":0.6,"curve":0.0},' +
  '{"x":1.0,"y":0.5,"curve":0.0}]}';

// ── Hover-help preference stub (v1.4.0) ─────────────────────────────────────
// Seeded FALSE, matching the processor's default: the page must render with
// the "?" unlit and the layer silent, or the gate would be measuring a state
// a fresh instance never shows. A harness can pre-seed a different value
// through window.__stubTooltipsEnabled before the page's init() runs.
let stubTooltipsEnabled =
  (typeof window !== "undefined" && window.__stubTooltipsEnabled) || false;

// ── Preset backend stub (Stage 4) ───────────────────────────────────────────
// The 28 names seeded by initializeFactoryPresets(), in the case-insensitive
// sorted order getPresetList() returns.
const FACTORY = [
  "Baby Scratch", "Cassette Eject", "Chirp Flare",
  "Classic 1-Bar Stop", "Classic Half-Bar Stop", "Crab Roll",
  "Data Rot", "DJ Spinup", "Drunk Tape",
  "Glitch", "Half-Mix Stop", "Loose Capstan",
  "Orbit", "Pitch Tide", "Power Cut",
  "Seasick", "Slow-Tape Drag", "Snap Back",
  "Sputter", "Stutter-Scratch", "Subtle Wobble",
  "Tape Flutter", "Tape Rewind", "Tempo-Synced Short Stop",
  "Total Meltdown", "Transformer", "Two-Bar Dive",
  "Warped Record",
];
const userPresets = new Set();
let currentPreset = "Default";

// A fresh instance reports "Default", deliberately NOT a list member.
const presetList = () => [...FACTORY, ...userPresets].sort((a, b) =>
  a.toLowerCase().localeCompare(b.toLowerCase()));

function neighbour(step) {
  const list = presetList();
  if (list.length === 0) return currentPreset;
  const i = list.indexOf(currentPreset);
  if (i < 0) return list[0];
  return list[(i + step + list.length) % list.length];
}

// Both dialog fns MUST resolve {success, name} — preset-manager.js checks
// `result && result.success`, so a bare bool silently reports failure.
const dialogResult = (ok, name) => ({ success: ok, name });

const PRESET_FNS = {
  savePreset: (name) => {
    if (!name || FACTORY.includes(name)) return false;
    userPresets.add(name); currentPreset = name; return true;
  },
  savePresetWithDialog: () => {
    const name = `Stub Preset ${userPresets.size + 1}`;
    userPresets.add(name); currentPreset = name;
    return dialogResult(true, name);
  },
  loadPreset: (name) => {
    if (!presetList().includes(name)) return false;
    currentPreset = name; return true;
  },
  loadPresetFromFile: () => {
    const name = presetList()[0];
    if (!name) return dialogResult(false, "");
    currentPreset = name; return dialogResult(true, name);
  },
  getPresetList: () => presetList(),
  getCurrentPreset: () => currentPreset,
  selectNextPreset: () => neighbour(1),
  selectPreviousPreset: () => neighbour(-1),
  deletePreset: (name) => {
    if (FACTORY.includes(name) || !userPresets.has(name)) return false;
    userPresets.delete(name);
    if (currentPreset === name) currentPreset = "Default";
    return true;
  },
  isFactoryPreset: (name) => FACTORY.includes(name),
};

const CORE_FNS = {
  getParameterDefaults: () =>
    Object.fromEntries(Object.entries(RANGES).map(([id, r]) => [id, r.def])),

  commitEnvelope: (json) => {
    if (typeof json === "string" && json.length > 0) envelopeJson = json;
    return envelopeJson;
  },
  requestEnvelope: () => envelopeJson,

  setTooltipsEnabled: (enabled) => {
    stubTooltipsEnabled = !!enabled;
    return stubTooltipsEnabled;
  },
  getTooltipsEnabled: () => stubTooltipsEnabled,
};

// Mirrors the FIFTEEN native functions registered in PluginEditor.cpp:
// getParameterDefaults + commitEnvelope + requestEnvelope (Stage 3) +
// setTooltipsEnabled + getTooltipsEnabled (v1.4.0) + the ten preset fns
// (fetched by modules/preset-manager.js). Any OTHER name must still REJECT —
// rejecting the unknown is the whole point of this stub, and is how a bridge
// gap surfaces here instead of as a silently dead control in a DAW
// (pattern_webview_native_fn_bridge_gap). The whitelist grew 13 -> 15; it did
// not become permissive.
export function getNativeFunction(name) {
  if (Object.prototype.hasOwnProperty.call(CORE_FNS, name)) {
    return (...args) => Promise.resolve(CORE_FNS[name](...args));
  }

  if (Object.prototype.hasOwnProperty.call(PRESET_FNS, name)) {
    return (...args) => Promise.resolve(PRESET_FNS[name](...args));
  }

  return () => Promise.reject(new Error(`Unregistered native function: ${name}`));
}
