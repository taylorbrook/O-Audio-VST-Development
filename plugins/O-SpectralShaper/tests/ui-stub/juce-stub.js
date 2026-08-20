/*
   This file is part of O-SpectralShaper, an Ouaricon Audio plugin.
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
// Resources/ui/index.html renders in a plain browser.
//
// Ranges here mirror createParameterLayout() so the stub render is
// representative; they are TEST FIXTURES ONLY and are never shipped. A fixture
// that silently drifts from the plugin keeps passing while it measures the
// wrong page (pattern_test_fixture_mirrors_drift_silently), so every value
// below is the one the C++ layout states.
// ============================================================================

function listenerList() {
  const listeners = [];
  return {
    addListener: (fn) => listeners.push(fn),
    callListeners: () => listeners.forEach((fn) => fn()),
  };
}

// Mirrors createParameterLayout() in PluginProcessor.cpp: the two time
// parameters carry an explicit 0.3 skew in their NormalisableRange.
const RANGES = {
  MIX:            { start: 0,    end: 1,   skew: 1,   interval: 0.01, def: 1.0 },
  ATTACK_TIME:    { start: 0.1,  end: 50,  skew: 0.3, interval: 0.1,  def: 10 },
  SUSTAIN_TIME:   { start: 10,   end: 500, skew: 0.3, interval: 1,    def: 100 },
  SENSITIVITY:    { start: 0,    end: 1,   skew: 1,   interval: 0.01, def: 0.5 },
  LOOKAHEAD_TIME: { start: 0.1,  end: 10,  skew: 1,   interval: 0.1,  def: 2.0 },
  OUTPUT_GAIN:    { start: -12,  end: 12,  skew: 1,   interval: 0.1,  def: 0 },
};

const TOGGLES = {
  LOOKAHEAD_ENABLED: false,
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
const toggleStates = new Map();

export function getSliderState(name) {
  if (!sliderStates.has(name)) sliderStates.set(name, new StubSliderState(name));
  return sliderStates.get(name);
}

export function getToggleState(name) {
  if (!toggleStates.has(name)) toggleStates.set(name, new StubToggleState(name));
  return toggleStates.get(name);
}

// ── Backend event stub ──────────────────────────────────────────────────────
// app.js gates initialization on window.__JUCE__ existing, and subscribes to
// "visualizationUpdate" for the spectrogram. preset-manager.js also polls for
// window.__JUCE__.backend before doing anything, and without this shim its
// _waitForNative() burns 100 x 50 ms and then console.errors, which both fails
// the zero-console-errors gate and adds 5 s to every run.
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

// ── Hover-help preference stub (v1.5.0) ─────────────────────────────────────
// Seeded FALSE, matching the processor's default.
let stubTooltipsEnabled =
  (typeof window !== "undefined" && window.__stubTooltipsEnabled) || false;

// ── Preset backend stub ─────────────────────────────────────────────────────
// The 29 names seeded by the v1.6.0 factory bank, in the NARRATIVE order and
// grouping the constructor declares them in — which is what
// getPresetListGrouped() returns. Held this way round, and FACTORY derived
// from it below, so the stub carries ONE list of names: a second flat copy in
// sorted order is exactly the mirror that drifts silently
// (pattern_test_fixture_mirrors_drift_silently).
const FACTORY_CATEGORIES = [
  { category: "Essentials", presets: [
    "Default", "Gentle Shaping", "Extra Snap", "Transient Tamer"] },
  { category: "Drums & Percussion", presets: [
    "Punch Enhancer", "Kick Tightener", "Snare Crack", "Tom Focus",
    "Room Tamer", "Percussion Sparkle"] },
  { category: "Cymbals & Air", presets: [
    "Cymbal Control", "Hat De-Harsh", "Shimmer Sustain"] },
  { category: "Vocals & Speech", presets: [
    "De-Esser", "Plosive Guard", "Vocal Presence", "Breath & Air"] },
  { category: "Instruments", presets: [
    "Strum Snap", "Piano Hammer", "Bass Definition", "String Swell",
    "Pick Bite"] },
  { category: "Mix & Master", presets: [
    "Warm Sustain", "Sustain Lift", "Low-End Tightener", "Master Polish"] },
  { category: "Creative", presets: [
    "Aggressive Bite", "Attack Eraser", "Infinite Bloom"] },
];
const FACTORY = FACTORY_CATEGORIES.flatMap((c) => c.presets);
const userPresets = new Set();
let currentPreset = "Default";

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
  // Mirrors the C++ shape: an ARRAY of {category, presets}, factory sections in
  // narrative order, then a "User" section that is OMITTED when empty. Only
  // names actually in presetList() are emitted, as the C++ side cross-checks.
  getPresetListGrouped: () => {
    const live = presetList();
    const out = FACTORY_CATEGORIES
      .map((c) => ({ category: c.category,
                     presets: c.presets.filter((n) => live.includes(n)) }))
      .filter((c) => c.presets.length > 0);
    const user = live.filter((n) => !FACTORY.includes(n));
    if (user.length > 0) out.push({ category: "User", presets: user });
    return out;
  },
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
  setAttackCurve: () => true,
  setSustainCurve: () => true,
  setTooltipsEnabled: (enabled) => {
    stubTooltipsEnabled = !!enabled;
    return stubTooltipsEnabled;
  },
  getTooltipsEnabled: () => stubTooltipsEnabled,
};

// Mirrors the FIFTEEN native functions registered in PluginEditor.cpp:
// setAttackCurve + setSustainCurve, setTooltipsEnabled + getTooltipsEnabled,
// and the eleven preset fns (ten fetched by modules/preset-manager.js plus
// getPresetListGrouped, fetched by app.js for the menu).
//
// Any OTHER name must still REJECT — rejecting the unknown is the whole point
// of this stub, and is how a bridge gap surfaces here instead of as a silently
// dead control in a DAW (pattern_webview_native_fn_bridge_gap).
export function getNativeFunction(name) {
  if (Object.prototype.hasOwnProperty.call(CORE_FNS, name)) {
    return (...args) => Promise.resolve(CORE_FNS[name](...args));
  }

  if (Object.prototype.hasOwnProperty.call(PRESET_FNS, name)) {
    return (...args) => Promise.resolve(PRESET_FNS[name](...args));
  }

  return () => Promise.reject(new Error(`Unregistered native function: ${name}`));
}
