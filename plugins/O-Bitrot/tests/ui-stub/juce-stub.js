/*
   This file is part of O-Bitrot, an Ouaricon Audio plugin.
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
// that the C++ build, auval and pluginval cannot see —
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
// wrong page (pattern_test_fixture_mirrors_drift_silently), so every value
// below is the one the C++ layout states, and the two skews are DERIVED from
// the same setSkewForCentre() call rather than eyeballed.
// ============================================================================

function listenerList() {
  const listeners = [];
  return {
    addListener: (fn) => listeners.push(fn),
    callListeners: () => listeners.forEach((fn) => fn()),
  };
}

// JUCE's NormalisableRange::setSkewForCentre(c) solves for the exponent that
// puts `c` at normalised 0.5: skew = ln(0.5) / ln((c-start)/(end-start)).
// Computed rather than transcribed — a hand-typed 0.25 would be a fixture that
// drifts from the plugin the moment the centre moves.
const skewForCentre = (start, end, centre) =>
  Math.log(0.5) / Math.log((centre - start) / (end - start));

// percentRange(): 0..100 linear, interval 0.1 — 27 of the 45 parameters.
const PCT = { start: 0, end: 100, skew: 1, interval: 0.1 };

const RANGES = {
  // ── Global ────────────────────────────────────────────────────────────────
  // CLOCK_FREE_RATE: 0.1..20 Hz, setSkewForCentre(1.414) = sqrt(0.1*20).
  CLOCK_FREE_RATE: { start: 0.1, end: 20, skew: skewForCentre(0.1, 20, 1.414), interval: 0.01, def: 2.0 },
  // SEED is an AudioParameterInt 0..9999 — a slider state, not a choice.
  SEED:            { start: 0, end: 9999, skew: 1, interval: 1, def: 0 },
  MIX:             { ...PCT, def: 100 },

  // ── Tape ──────────────────────────────────────────────────────────────────
  TAPE_PROB:       { ...PCT, def: 25 },
  TAPE_STOP_PROB:  { ...PCT, def: 10 },
  TAPE_RAMP:       { start: 20, end: 500, skew: 1, interval: 0.1, def: 150 },
  TAPE_DROP:       { ...PCT, def: 0 },
  TAPE_WOW:        { ...PCT, def: 0 },
  TAPE_HISS:       { ...PCT, def: 0 },

  // ── CD Skip ───────────────────────────────────────────────────────────────
  CD_PROB:         { ...PCT, def: 25 },
  // The only 0..1 range on the page — its readout carries 2 decimals.
  CD_SEVERITY:     { start: 0, end: 1, skew: 1, interval: 0.01, def: 0.5 },
  CD_SEGMENT:      { start: 10, end: 400, skew: 1, interval: 0.1, def: 100 },

  // ── Vinyl ─────────────────────────────────────────────────────────────────
  VINYL_PROB:      { ...PCT, def: 25 },
  VINYL_POP:       { ...PCT, def: 50 },
  VINYL_WEAR:      { ...PCT, def: 0 },
  VINYL_WARP:      { ...PCT, def: 0 },

  // ── Packet ────────────────────────────────────────────────────────────────
  PACKET_LOSS:     { ...PCT, def: 20 },
  PACKET_BURST:    { ...PCT, def: 30 },
  PACKET_COMFORT:  { ...PCT, def: 0 },

  // ── Codec ─────────────────────────────────────────────────────────────────
  CODEC_MIX:       { ...PCT, def: 100 },
  CODEC_NOISE:     { ...PCT, def: 0 },
  // NOT transparent at its default — v1.8.0 shipped the AGC on with the
  // section at 100, deliberately (see the layout comment in C++).
  CODEC_AGC:       { ...PCT, def: 100 },

  // ── Crush ─────────────────────────────────────────────────────────────────
  CRUSH_BITS:      { start: 1, end: 16, skew: 1, interval: 0.01, def: 16 },
  // CRUSH_RATE: 500..20000 Hz, setSkewForCentre(3162) = sqrt(500*20000).
  CRUSH_RATE:      { start: 500, end: 20000, skew: skewForCentre(500, 20000, 3162), interval: 1, def: 20000 },
  CRUSH_JITTER:    { ...PCT, def: 0 },
  // The only bipolar range on the page — its readout is data-signed.
  CRUSH_ENV_AMT:   { start: -100, end: 100, skew: 1, interval: 0.1, def: 0 },
  CRUSH_DITHER:    { start: 0, end: 2, skew: 1, interval: 0.01, def: 0 },

  // ── Rot (v1.10.0) ─────────────────────────────────────────────────────────
  ROT_PROB:        { ...PCT, def: 25 },
  ROT_DEPTH:       { ...PCT, def: 50 },
  ROT_STICK:       { ...PCT, def: 25 },
  ROT_GARBLE:      { ...PCT, def: 25 },
};

// The eight bools. Each needs its own entry because a ToggleState carries a
// boolean, not a scaled value; routing one through RANGES would hand the page
// a SliderState for a parameter that has none — in a real host, a switch that
// never updates.
//
// The three/four split is load-bearing for what the page RENDERS: Tape, CD and
// Vinyl ship enabled and their panels ship .on; Packet, Codec, Crush and Rot
// ship disabled and their panels ship .off. A stub that defaulted all eight to
// one value would measure a page the plugin never opens in.
const TOGGLES = {
  TAPE_ENABLE: true,
  CD_ENABLE: true,
  VINYL_ENABLE: true,
  PACKET_ENABLE: false,
  CODEC_ENABLE: false,
  CRUSH_ENABLE: false,
  ROT_ENABLE: false,
  HARD_EDGES: false,
};

// Must match the C++ StringArrays exactly. ORDER is load-bearing, not
// cosmetic: VINYL_RPM's "78" was APPENDED in v1.7.0 precisely so indices 0 and
// 1 kept their meaning for existing sessions — and appending is why the
// preset-manager migration hook exists.
const CHOICES = {
  CLOCK_MODE:     ["Sync", "Free"],
  CLOCK_SYNC_DIV: ["1/16", "1/8T", "1/8", "1/4T", "1/4", "1/2", "1 bar"],
  VINYL_RPM:      ["33 1/3", "45", "78"],   // page renders the 33⅓ glyph
  PACKET_CONCEAL: ["Silence", "Repeat", "Decay", "Substitute"],
  CODEC_MODE:     ["Mu-law", "GSM"],        // page renders the μ glyph
  CODEC_MAINS:    ["50 Hz", "60 Hz"],
};

// Defaults are INDICES. CLOCK_SYNC_DIV (1/8) and PACKET_CONCEAL (Decay) are
// NOT zero — copying 0 across all six would render a page the plugin never
// ships, and would leave the Sync/Free swap slot showing the wrong division.
const DEFAULT_CHOICE = {
  CLOCK_MODE: 0,        // Sync
  CLOCK_SYNC_DIV: 2,    // 1/8
  VINYL_RPM: 0,         // 33 1/3
  PACKET_CONCEAL: 2,    // Decay
  CODEC_MODE: 0,        // Mu-law
  CODEC_MAINS: 0,       // 50 Hz
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
const comboStates = new Map();
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
// The page subscribes to "ledUpdate" on window.__JUCE__.backend — the
// sanctioned window.__JUCE__ use (backend events only; parameter state goes
// through the ES-module namespace above,
// critical_juce_webview_namespace_vs_postmessage). preset-manager.js also
// polls for window.__JUCE__.backend before doing anything, and without this
// shim its _waitForNative() burns 100 x 50 ms and then console.errors, which
// both fails the zero-console-errors gate and adds 5 s to every run.
// __stubEmit is exposed so a harness can drive the spore-print LEDs.
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

// ── Hover-help preference stub (v1.12.0) ────────────────────────────────────
// Seeded FALSE, matching the processor's default: the page must render with
// the "?" unlit and the layer silent, or the gate would be measuring a state a
// fresh instance never shows. A harness can pre-seed a different value through
// window.__stubTooltipsEnabled before the page's module evaluates.
let stubTooltipsEnabled =
  (typeof window !== "undefined" && window.__stubTooltipsEnabled) || false;

// ── Preset backend stub (Stage 4) ───────────────────────────────────────────
// The 28 names seeded by the v1.11.0 factory bank, in the NARRATIVE order and
// grouping the constructor declares them in — which is what
// getPresetListGrouped() returns (v1.13.0). Held this way round, and FACTORY
// derived from it below, so the stub carries ONE list of names: a second flat
// copy in sorted order is exactly the mirror that drifts silently
// (pattern_test_fixture_mirrors_drift_silently).
const FACTORY_CATEGORIES = [
  { category: "Showcases", presets: [
    "Worn Cassette", "Skipping Disc", "Locked Groove", "Dropped Call",
    "Cellphone 1998", "Eight-Bit Ruin", "Total Media Failure", "Gentle Rot",
    "Corrupt Archive"] },
  { category: "Tape", presets: [
    "Warped C-90", "Basement Reel", "Chewed Tape", "Dictaphone Memo"] },
  { category: "Vinyl", presets: [
    "Thrift-Store Turntable", "Dusty 45", "Shellac 78"] },
  { category: "CD", presets: [
    "Scratched CD-R", "Stuck Disc"] },
  { category: "Phone & Network", presets: [
    "Last Voicemail", "Answering Machine", "Transatlantic Line"] },
  { category: "Rot", presets: [
    "Bad Blocks", "Wrong Byte Offset", "Frozen Decoder"] },
  { category: "Disintegration", presets: [
    "Disintegration Loop I", "Disintegration Loop II",
    "Disintegration Loop III", "Disintegration Loop IV"] },
];
// getPresetList() returns every preset in case-insensitive sorted order; the
// grouping above is authored order, so this flattens rather than mirrors.
const FACTORY = FACTORY_CATEGORIES.flatMap((c) => c.presets);
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

// v1.14.0 — the hover-help language, mirrored so the stub round-trips it
// exactly as the processor does. languageIndex()'s C++ clamp is mirrored too —
// anything that is not "fr" degrades to "en" — so a stub that accepted a bogus
// code could not make a broken page look correct.
let stubUiLanguage = "en";

const CORE_FNS = {
  setTooltipsEnabled: (enabled) => {
    stubTooltipsEnabled = !!enabled;
    return stubTooltipsEnabled;
  },
  getTooltipsEnabled: () => stubTooltipsEnabled,

  setUiLanguage: (code) => {
    stubUiLanguage = code === "fr" ? "fr" : "en";
    return stubUiLanguage;
  },
  getUiLanguage: () => stubUiLanguage,
};

// Mirrors the FOURTEEN native functions registered in PluginEditor.cpp:
// setTooltipsEnabled + getTooltipsEnabled (v1.12.0) + v1.14.0's
// getUiLanguage/setUiLanguage + the ten preset fns (fetched by
// modules/preset-manager.js). O-Bitrot has no getParameterDefaults — its
// dblclick-reset reads data-default from the markup instead.
//
// (PluginEditor.cpp registers 15: getPresetListGrouped is O-Bitrot's own and is
// consumed by the preset MENU in index.html, not by the shared module, and it
// is handled in PRESET_FNS below.)
//
// Any OTHER name must still REJECT — rejecting the unknown is the whole point
// of this stub, and is how a bridge gap surfaces here instead of as a silently
// dead control in a DAW (pattern_webview_native_fn_bridge_gap). The whitelist
// grew 10 -> 12 -> 14; it did not become permissive.
export function getNativeFunction(name) {
  if (Object.prototype.hasOwnProperty.call(CORE_FNS, name)) {
    return (...args) => Promise.resolve(CORE_FNS[name](...args));
  }

  if (Object.prototype.hasOwnProperty.call(PRESET_FNS, name)) {
    return (...args) => Promise.resolve(PRESET_FNS[name](...args));
  }

  return () => Promise.reject(new Error(`Unregistered native function: ${name}`));
}
