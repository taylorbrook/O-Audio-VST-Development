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
// Minimal JUCE WebView bridge stub — substitutes for js/juce/index.js so
// Source/ui/public/index.html renders in a plain browser.
//
// This is the Stage-3 "mockup gate": rendering the real page catches the two
// failure modes that C++ build / auval / pluginval cannot see —
//   1. a top-level TDZ ReferenceError killing the whole UI
//      (pattern_module_toplevel_init_tdz)
//   2. a state updater overwriting HTML-authored labels
//      (pattern_js_state_updater_overwrites_html_labels)
//
// Ranges here mirror createParameterLayout() so the stub render is
// representative; they are TEST FIXTURES ONLY and are never shipped.
// ============================================================================

function listenerList() {
  const listeners = [];
  return {
    addListener: (fn) => listeners.push(fn),
    callListeners: () => listeners.forEach((fn) => fn()),
  };
}

// start / end / skew / interval mirror the C++ NormalisableRange. The skew
// values are JUCE's own (setSkewForCentre): skew = ln(0.5) / ln((c-s)/(e-s)).
function skewForCentre(start, end, centre) {
  return Math.log(0.5) / Math.log((centre - start) / (end - start));
}

const RANGES = {
  // v1.0.1 raised delayTime's max 2000 -> 4000 (A1); the stub was left at 2000
  // and so rendered a knob whose readout disagreed with the plugin's.
  delayTime: { start: 50,  end: 4000,  skew: skewForCentre(50, 4000, 316),    interval: 0.01, def: 500 },
  grainSize: { start: 50,  end: 4000,  skew: skewForCentre(50, 4000, 316),    interval: 0.01, def: 200 },
  density:   { start: 0,   end: 100,   skew: 1, interval: 0.1,  def: 60 },
  feedback:  { start: 0,   end: 100,   skew: 1, interval: 0.1,  def: 40 },
  lowCut:    { start: 20,  end: 2000,  skew: skewForCentre(20, 2000, 200),    interval: 0.01, def: 100 },
  highCut:   { start: 500, end: 20000, skew: skewForCentre(500, 20000, 3162), interval: 0.01, def: 8000 },
  width:     { start: 0,   end: 100,   skew: 1, interval: 0.1,  def: 60 },
  mix:       { start: 0,   end: 100,   skew: 1, interval: 0.1,  def: 35 },

  // v1.1.0 (B3) — RANDOM panel. All default to 0.
  jitter:       { start: 0, end: 100, skew: 1, interval: 0.1, def: 0 },
  delayScatter: { start: 0, end: 500, skew: 1, interval: 0.1, def: 0 },
  sizeRandom:   { start: 0, end: 100, skew: 1, interval: 0.1, def: 0 },
  gainRandom:   { start: 0, end: 100, skew: 1, interval: 0.1, def: 0 },

  // v1.2.0 (B1) — WINDOW panel. NOTE the default: 0.5, not 0. grainTilt is the
  // one parameter here whose neutral position is the CENTRE of its range, so a
  // stub that defaulted it to 0 would render the page with a hard peak-early
  // tilt and a "−100 %" readout, i.e. it would show a state the plugin never
  // ships in.
  grainTilt:    { start: 0, end: 1,   skew: 1, interval: 0.001, def: 0.5 },

  // v1.3.0 (B2) — COUNT panel. Same trap as grainTilt in a different disguise:
  // the neutral default is 8, the value v1.2.0 hard-coded, NOT the range minimum
  // and NOT the new maximum. A stub defaulting it to 2 or 16 would render a page
  // showing a state the plugin never ships in.
  grainCount:   { start: 2, end: 16,  skew: 1, interval: 1,     def: 8 },

  // v1.4.0 — WINDOW panel's Taper. def 0.5 is v1.2.0's frozen taper, NOT the
  // range minimum: a stub defaulting it to 0.01 would render the page showing a
  // near-rectangular window the plugin never ships.
  tukeyTaper:   { start: 0.01, end: 1, skew: 1, interval: 0.01, def: 0.5 },

  // v1.6.0 (B4 #2, #3) — MOTION panel. These two ARE plain zeros, unlike the
  // three releases before them: 0 % is all-reverse and 0 dB is unity loop gain,
  // both the engine's exact no-op. The end of regenMakeup must track
  // ReverseDelayProcessor::kRegenMakeupMaxDb — that constant is a measured
  // stability bound (probe AO), so a stub that drifts from it renders a knob
  // whose top reads a value the plugin cannot reach.
  direction:    { start: 0, end: 100, skew: 1, interval: 0.1, def: 0 },
  regenMakeup:  { start: 0, end: 6,   skew: 1, interval: 0.1, def: 0 },

  // v1.7.0 (B4 #4, #6) — row 3's DUCK and DRIFT panels. duck and driftDepth are
  // plain zeros; driftRate is the trap in this block, and it is a NEW disguise
  // of the grainTilt/grainCount/tukeyTaper one: its neutral value is 0.30 Hz,
  // the parameter's own default, because what makes it neutral is driftDepth
  // sitting at 0. A stub defaulting it to the range MINIMUM would render the
  // page showing a slower rate than the plugin ever ships — and 0 would be
  // clamped to 0.02 rather than rejected, so it would look like it worked.
  //
  // The skew mirrors setSkewForCentre(kDriftRateCentreHz) on the C++ range, and
  // the centre IS the default, so the shipped rate sits at the knob's midpoint.
  duck:         { start: 0,    end: 100, skew: 1, interval: 0.1,  def: 0 },
  driftRate:    { start: 0.02, end: 5,   skew: skewForCentre(0.02, 5, 0.30),
                                          interval: 0.01, def: 0.30 },
  driftDepth:   { start: 0,    end: 100, skew: 1, interval: 0.1,  def: 0 },

  // v1.8.0 (B4 #7, #8) — row 3's COLOUR panel. Both plain zeros, both linear,
  // and for once no trap in the block: neither is a choice index, neither is
  // neutral at a non-zero value, and neither is skewed. Worth stating rather
  // than leaving to inference, because five of the six blocks above DO carry
  // one and a reader arriving from them should not have to check.
  diffusion:    { start: 0,    end: 100, skew: 1, interval: 0.1,  def: 0 },
  drive:        { start: 0,    end: 100, skew: 1, interval: 0.1,  def: 0 },
};

// v1.6.0 — bool parameters. `freeze` is the only one, and it needs its own map
// because a ToggleState carries a boolean rather than a scaled value; feeding it
// through RANGES would give app.js a SliderState for a parameter that has none.
const TOGGLES = { freeze: false };

const CHOICES = {
  syncMode: ["Free", "Sync"],
  // v1.7.0 (B4 #5) — must match the C++ StringArray, and the ORDER is
  // load-bearing rather than cosmetic: index 0 is what an absent key in a
  // pre-v1.7.0 session or preset resolves to, so Mono Sum has to be first.
  sourceMode: ["Mono Sum", "Stereo"],
  freezeLength: ["Ring", "Delay", "1 Bar", "2 Bars"],   // v1.15.0
  grainLink: ["Free", "= Delay", "Division"],           // v1.17.0
  // v1.2.0 — must match WindowLut::Shape order and the C++ StringArray.
  grainShape: ["Hann", "Tukey", "Gaussian", "Triangular", "Expo-Decay"],
  noteDivision: [
    "1/16", "1/16D", "1/16T",
    "1/8",  "1/8D",  "1/8T",
    "1/4",  "1/4D",  "1/4T",
    "1/2",  "1/2D",  "1/2T",
    "1/1",
  ],
};
CHOICES.grainDivision = CHOICES.noteDivision;           // v1.17.0: the same table

const DEFAULT_CHOICE = { syncMode: 1, noteDivision: 6, grainShape: 0, sourceMode: 0, freezeLength: 0,
                         grainLink: 0, grainDivision: 6 };

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
  // v1.12.3: snaps to the interval exactly as juce/index.js snapToLegalValue()
  // does. Up to v1.12.2 the stub stored the raw value, so a sub-step nudge that
  // the real backend rounds straight back (grainCount's wheel and arrows) moved
  // here and no gate could see the knob was stuck.
  setNormalisedValue(n) {
    const p = this.properties;
    let v = Math.pow(n, 1 / p.skew) * (p.end - p.start) + p.start;
    if (p.interval) {
      v = Math.max(p.start, Math.min(p.end,
        p.start + p.interval * Math.floor((v - p.start) / p.interval + 0.5)));
    }
    this.scaledValue = v;
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

// v1.6.0 — mirrors juce/index.js's ToggleState: a boolean, getValue/setValue,
// and the same two listener lists. Deliberately NOT a two-choice ComboBoxState,
// because the whole point of stubbing it is to catch app.js calling the wrong
// API for a bool — which in a real host is a switch that never updates.
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

// ── Preset backend stub (Stage 4) ───────────────────────────────────────────
// preset-manager.js polls window.__JUCE__.backend before doing anything else.
// Without this shim _waitForNative() burns 100 x 50 ms = 5 s and then
// console.errors (preset-manager.js:129-142) — which both fails the
// zero-console-errors render gate and adds 5 s to every stub run.
if (typeof window !== "undefined") window.__JUCE__ = { backend: {} };

// In-memory preset library. FACTORY mirrors the eight seeded by
// initializeFactoryPresets(), in the same case-insensitive sorted order
// getPresetList() returns.
const FACTORY = ["Dark Cavern", "Guitar Swell", "Near-Infinite", "Reverse Bloom",
                 "Rhythmic Reverse", "Slow Wash", "Tight Smear", "Vocal Halo"];
const userPresets = new Set();
let currentPreset = "Default";

// A fresh instance reports "Default", which is deliberately NOT a list member.
const presetList = () => [...FACTORY, ...userPresets].sort((a, b) =>
  a.toLowerCase().localeCompare(b.toLowerCase()));

// Prev/next mirror the C++ lastListIndex behaviour closely enough for the bar:
// an out-of-list current name starts navigation at index 0.
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
    // No native dialog in a browser: synthesise a name so the Save leg of the
    // bar is still drivable at the render gate.
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

// v1.9.0 — the hover-help language, mirrored so the stub round-trips it exactly
// as the processor does. Held OUTSIDE getNativeFunction so setUiLanguage's write
// is visible to a later getUiLanguage read: the clamp gate drives the language
// through window.__setLanguage(), but the render probes assert the WRITE reached
// the (stubbed) processor, and that is only observable if the value persists.
//
// languageIndex()'s C++ clamp is mirrored here too — anything that is not "fr"
// degrades to "en" — so a stub that accepted a bogus code could not make a
// broken page look correct.
let uiLanguage = "en";

// v1.16.0 — the Mix lock, held here for the same reason as uiLanguage, and
// mirroring setMixLock's C++ rule: only a real boolean true locks.
let mixLock = false;

// v1.18.0 — A/B slots. A snapshot is every slider's normalised value and every
// combo's index the stub has created, which is what the page can see.
let abActive = 0;
const abSlots = [null, null];
const RANDOMISE_FLOAT_IDS = ["jitter", "delayScatter", "sizeRandom", "gainRandom",
                             "grainTilt", "tukeyTaper", "driftRate", "driftDepth",
                             "diffusion", "drive"];   // + grainShape (a combo)

function abCapture() {
  return {
    sliders: new Map([...sliderStates].map(([id, s]) => [id, s.getNormalisedValue()])),
    combos:  new Map([...comboStates].map(([id, s]) => [id, s.getChoiceIndex()])),
  };
}

function abRecall(snap) {
  for (const [id, v] of snap.sliders) getSliderState(id).setNormalisedValue(v);
  for (const [id, i] of snap.combos)  getComboBoxState(id).setChoiceIndex(i);
}

function abStateVar() {
  return { active: abActive, filledA: abSlots[0] !== null, filledB: abSlots[1] !== null };
}

// Mirrors the TWENTY-ONE native functions registered in PluginEditor.cpp:
// getParameterDefaults + getGrainMeter + getWindowCurve + v1.9.0's
// getUiLanguage/setUiLanguage + v1.16.0's getMixLock/setMixLock + v1.18.0's
// getAbState/abSelect/abCopy/randomise (all fetched by app.js) + the ten preset fns
// (fetched by js/preset-manager.js). Any OTHER name must still reject —
// rejecting the unknown is the whole point of this stub, and is how a bridge gap
// surfaces here instead of as a silently dead control in a DAW
// (pattern_webview_native_fn_bridge_gap). The whitelist grew
// 1 -> 11 -> 12 -> 13 -> 15 -> 17 -> 21; it did not become permissive.
//
// NOTE what is NOT here and must never be added: setTooltipsEnabled. D13 scoped
// this plugin to display-only hover help, and section 14 of ui_frontend_check.js
// asserts the absence against the real source.
export function getNativeFunction(name) {
  if (name === "getUiLanguage") {
    return () => Promise.resolve(uiLanguage);
  }

  if (name === "setUiLanguage") {
    return (code) => {
      uiLanguage = code === "fr" ? "fr" : "en";
      return Promise.resolve(uiLanguage);
    };
  }

  if (name === "getMixLock") {
    return () => Promise.resolve(mixLock);
  }

  if (name === "setMixLock") {
    return (on) => {
      mixLock = on === true;
      return Promise.resolve(mixLock);
    };
  }

  // v1.18.0 — A/B compare + Randomise, mirroring the processor's rules: leaving
  // a slot captures it, an empty target starts as a copy (no recall), copy goes
  // active -> inactive, and Randomise writes the way back into the INACTIVE slot
  // before touching only the eleven RANDOM/WINDOW/DRIFT/COLOUR parameters.
  if (name === "getAbState") return () => Promise.resolve(abStateVar());

  if (name === "abSelect") {
    return (slot) => {
      if ((slot === 0 || slot === 1) && slot !== abActive) {
        abSlots[abActive] = abCapture();
        if (abSlots[slot] === null) abSlots[slot] = abSlots[abActive];
        else abRecall(abSlots[slot]);
        abActive = slot;
      }
      return Promise.resolve(abStateVar());
    };
  }

  if (name === "abCopy") {
    return () => { abSlots[1 - abActive] = abCapture(); return Promise.resolve(abStateVar()); };
  }

  if (name === "randomise") {
    return () => {
      abSlots[1 - abActive] = abCapture();
      for (const id of RANDOMISE_FLOAT_IDS) getSliderState(id).setNormalisedValue(Math.random());
      getComboBoxState("grainShape").setChoiceIndex(Math.floor(Math.random() * 5));
      return Promise.resolve(abStateVar());
    };
  }

  if (name === "getParameterDefaults") {
    return () => Promise.resolve(
      Object.fromEntries(Object.entries(RANGES).map(([id, r]) => [id, r.def]))
    );
  }

  // v1.3.0 (B2) — the grain meter. Returns a MOVING value rather than a
  // constant: a stub that always answered the same numbers would render
  // identically whether app.js polled once or on an interval, so the render gate
  // could not tell a live readout from a dead one. The count walks the plausible
  // range and overlap is derived from the stub's own grainCount/density states,
  // which is also what makes the two readouts consistent with the knobs on screen.
  if (name === "getGrainMeter") {
    let tick = 0;
    let stubFreezeOnAt = null;
    return () => {
      const ceiling = getSliderState("grainCount").getScaledValue();
      const density = getSliderState("density").getScaledValue();
      const overlap = 2 + (density / 100) * (ceiling - 2);
      tick = (tick + 1) % 7;

      // v1.13.0 riders. A fixed 120 BPM host tempo; ?bpm=0 simulates Standalone
      // (fallback to the Delay knob) and ?bpm=40 lands 1/1 on the 4000 ms rail.
      // Freeze engages ~1 s after it is switched on, standing in for the
      // one-grain wait, so the armed pulse is visible in the stub.
      const bpmArg = new URLSearchParams(window.location.search).get("bpm");
      const bpm = bpmArg === null ? 120 : Number(bpmArg);
      const beats = [0.25, 0.375, 1 / 6, 0.5, 0.75, 1 / 3, 1, 1.5, 2 / 3, 2, 3, 4 / 3, 4];
      let delayMs = getSliderState("delayTime").getScaledValue();
      let delaySource = "free";
      if (getComboBoxState("syncMode").getChoiceIndex() === 1) {
        if (bpm > 0) {
          const ms = beats[getComboBoxState("noteDivision").getChoiceIndex()] * 60000 / bpm;
          delayMs = Math.min(4000, Math.max(50, ms));
          delaySource = ms < 50 || ms > 4000 ? "clamped" : "tempo";
        } else {
          delaySource = "fallback";
        }
      }
      // v1.17.0 — Grain Link, mirroring resolveGrainMs(): Division reads the
      // tempo in either TIME mode and falls back to the Size knob without one.
      let grainMs = getSliderState("grainSize").getScaledValue();
      let grainSource = "free";
      const link = getComboBoxState("grainLink").getChoiceIndex();
      if (link === 1) {
        grainMs = Math.min(4000, Math.max(50, delayMs));
        grainSource = "delay";
      } else if (link === 2) {
        if (bpm > 0) {
          const ms = beats[getComboBoxState("grainDivision").getChoiceIndex()] * 60000 / bpm;
          grainMs = Math.min(4000, Math.max(50, ms));
          grainSource = ms < 50 || ms > 4000 ? "clamped" : "tempo";
        } else {
          grainSource = "fallback";
        }
      }

      const frozen = getToggleState("freeze").getValue() === true;
      stubFreezeOnAt = frozen ? (stubFreezeOnAt ?? Date.now()) : null;

      return Promise.resolve({
        active: Math.max(0, Math.round(overlap) - (tick % 2)),
        overlap,
        delayMs,
        delaySource,
        grainMs,
        grainSource,
        freezeEngaged: frozen && Date.now() - stubFreezeOnAt >= 1000,
        // v1.14.0: a wobbling input at -8 dBFS, and an output lifted by Regen
        // so that Regen above ~+4 dB crosses 0 dBFS and latches the clip lamp.
        peakIn:  0.4 * (0.8 + 0.2 * Math.sin(tick)),
        peakOut: 0.6 * Math.pow(10, getSliderState("regenMakeup").getScaledValue() / 20)
                     * (0.8 + 0.2 * Math.sin(tick + 1)),
      });
    };
  }

  // v1.4.0 — the window-shape curve. The stub REIMPLEMENTS the window, which the
  // shipping page deliberately does not: app.js must never contain a second
  // definition, but the stub is a test fixture standing in for the C++ and has to
  // produce something shaped like a window or the render gate cannot see whether
  // the plot is wired. Kept in sync by eye only — the frontend check asserts the
  // real bridge, and the C++ is authoritative in the plugin.
  if (name === "getWindowCurve") {
    return () => {
      const n = 128;
      const shapeIdx = getComboBoxState("grainShape").getChoiceIndex();
      const tilt = getSliderState("grainTilt").getScaledValue();
      const alpha = getSliderState("tukeyTaper").getScaledValue();
      const t = Math.min(0.95, Math.max(0.05, 0.5 + (tilt - 0.5) * 0.9));
      const a = 0.5 / t, b = 0.5 / (1 - t);
      const out = [];

      for (let i = 0; i < n; i += 1) {
        const p = i / (n - 1);
        const q = Math.min(p, t) * a + Math.max(p - t, 0) * b;
        let w;
        if (shapeIdx === 1) {                        // Tukey — taper α
          const u = Math.min(q, 1 - q);
          const r = Math.min(u / (alpha / 2), 1) * 0.5;
          w = 0.5 * (1 - Math.cos(2 * Math.PI * r));
        } else if (shapeIdx === 2) {                 // Gaussian, pedestal removed
          const d = (q - 0.5) / 0.18;
          const g = Math.exp(-0.5 * d * d);
          const edge = Math.exp(-0.5 * (0.5 / 0.18) ** 2);
          w = Math.max(0, (g - edge) / (1 - edge));
        } else if (shapeIdx === 3) {                 // Triangular
          w = 1 - Math.abs(2 * q - 1);
        } else if (shapeIdx === 4) {                 // Expo-Decay
          const atk = 0.02;
          if (q < atk) w = 0.5 * (1 - Math.cos(Math.PI * q / atk));
          else {
            const u = (q - atk) / (1 - atk);
            const e = Math.exp(-5);
            w = Math.max(0, (Math.exp(-5 * u) - e) / (1 - e));
          }
        } else {                                     // Hann
          w = 0.5 * (1 - Math.cos(2 * Math.PI * q));
        }
        out.push(w);
      }
      return Promise.resolve(out);
    };
  }

  if (Object.prototype.hasOwnProperty.call(PRESET_FNS, name)) {
    return (...args) => Promise.resolve(PRESET_FNS[name](...args));
  }

  return () => Promise.reject(new Error(`Unregistered native function: ${name}`));
}
