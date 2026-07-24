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
  delayTime: { start: 50,  end: 2000,  skew: skewForCentre(50, 2000, 316),    interval: 0.01, def: 500 },
  grainSize: { start: 50,  end: 500,   skew: skewForCentre(50, 500, 158),     interval: 0.01, def: 200 },
  density:   { start: 0,   end: 100,   skew: 1, interval: 0.1,  def: 60 },
  feedback:  { start: 0,   end: 100,   skew: 1, interval: 0.1,  def: 40 },
  lowCut:    { start: 20,  end: 2000,  skew: skewForCentre(20, 2000, 200),    interval: 0.01, def: 100 },
  highCut:   { start: 500, end: 20000, skew: skewForCentre(500, 20000, 3162), interval: 0.01, def: 8000 },
  width:     { start: 0,   end: 100,   skew: 1, interval: 0.1,  def: 60 },
  mix:       { start: 0,   end: 100,   skew: 1, interval: 0.1,  def: 35 },
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

// Mirrors the ONE native function registered in PluginEditor.cpp. Any other
// name must reject — that is how the stub surfaces a bridge gap
// (pattern_webview_native_fn_bridge_gap).
export function getNativeFunction(name) {
  if (name !== "getParameterDefaults") {
    return () => Promise.reject(new Error(`Unregistered native function: ${name}`));
  }
  return () => Promise.resolve(
    Object.fromEntries(Object.entries(RANGES).map(([id, r]) => [id, r.def]))
  );
}
