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
  grainSize: { start: 50,  end: 500,   skew: skewForCentre(50, 500, 158),     interval: 0.01, def: 200 },
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
};

const CHOICES = {
  syncMode: ["Free", "Sync"],
  noteDivision: [
    "1/16", "1/16D", "1/16T",
    "1/8",  "1/8D",  "1/8T",
    "1/4",  "1/4D",  "1/4T",
    "1/2",  "1/2D",  "1/2T",
    "1/1",
  ],
};

const DEFAULT_CHOICE = { syncMode: 1, noteDivision: 6 };

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

const sliderStates = new Map();
const comboStates = new Map();

export function getSliderState(name) {
  if (!sliderStates.has(name)) sliderStates.set(name, new StubSliderState(name));
  return sliderStates.get(name);
}

export function getComboBoxState(name) {
  if (!comboStates.has(name)) comboStates.set(name, new StubComboBoxState(name));
  return comboStates.get(name);
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

// Mirrors the ELEVEN native functions registered in PluginEditor.cpp:
// getParameterDefaults (fetched by app.js) + the ten preset fns (fetched by
// js/preset-manager.js). Any OTHER name must still reject — rejecting the
// unknown is the whole point of this stub, and is how a bridge gap surfaces
// here instead of as a silently dead control in a DAW
// (pattern_webview_native_fn_bridge_gap). The whitelist grew 1 -> 11; it did
// not become permissive.
export function getNativeFunction(name) {
  if (name === "getParameterDefaults") {
    return () => Promise.resolve(
      Object.fromEntries(Object.entries(RANGES).map(([id, r]) => [id, r.def]))
    );
  }

  if (Object.prototype.hasOwnProperty.call(PRESET_FNS, name)) {
    return (...args) => Promise.resolve(PRESET_FNS[name](...args));
  }

  return () => Promise.reject(new Error(`Unregistered native function: ${name}`));
}
