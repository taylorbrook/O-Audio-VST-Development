/*
   This file is part of the Ouaricon Audio plugin suite.
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
// generic-juce-stub.js — one bridge stub that boots ANY of the 43 plugin pages.
//
// Substituted for js/juce/index.js by scripts/serve-ui.js. Only 5 plugins have a
// hand-written tests/ui-stub/juce-stub.js; the other 38 could not be rendered at
// all before this file, and a French-label overflow gate that cannot load the
// page is not a gate.
//
// ── How it DIFFERS from the five hand-written stubs, deliberately ────────────
//
// A hand-written stub REJECTS an unregistered native-function name. That is the
// whole point of those files: an unlisted name is a bridge gap that would
// present as a silently dead control in a DAW
// (pattern_webview_native_fn_bridge_gap), and the rejection is what turns it
// into a visible failure. Those stubs are running a set-equality census against
// their own plugin's PluginEditor.cpp.
//
// This file is NOT running that census, and pretending to would produce a
// gate that fails on 38 plugins for a reason that has nothing to do with i18n.
// So an unknown name resolves a benign value instead — and every unknown name
// is RECORDED on window.__stubUnknownNativeFns, so a caller can report the gap
// rather than lose it. Reporting is not the same as rejecting, and the README
// says so plainly: this stub cannot discharge a bridge-parity claim.
//
// ── The parameter states are FIXTURES, not the plugin ────────────────────────
//
// No plugin's createParameterLayout() is parsed here. A regex over that function
// provably undercounts in this repo (O-Prism: 0 usable IDs found statically,
// 173 at runtime), so guessing would be worse than admitting ignorance. Every
// slider gets a neutral 0..1 range unless a param-dump TSV or a per-plugin
// override says otherwise. What that costs: readouts render at fixture values,
// so this stub proves LAYOUT and TEXT, never a value.
//
// ── Seeding from a param-dump TSV ────────────────────────────────────────────
//
// If plugins/<Name>/.planning/params.tsv exists, serve-ui.js copies it in as
// /js/juce/params.tsv and this file seeds ranges from it. None exists today —
// the tool is wired for the day one is generated, and reports which mode it is
// in on window.__stubReport.
//
// TEST FIXTURE ONLY. Never shipped, never embedded.
// ============================================================================

// ── window.__JUCE__ ─────────────────────────────────────────────────────────
// Installed here as well as in stub-preamble.js. Both are idempotent, and the
// preamble is skipped for the five plugins that bring their own stub, so this
// module cannot rely on it having run.
function installGlobal() {
  if (typeof window === 'undefined') return null;
  if (window.__JUCE__ && window.__JUCE__.backend) return window.__JUCE__;

  const handlers = new Map();
  const backend = {
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
      (handlers.get(name) || []).forEach((fn) => {
        try { fn(payload); } catch (e) { console.warn(`stub: listener threw for ${name}`, e); }
      });
    },
    emitByBackend: (name, payload) => {
      let v = payload;
      if (typeof v === 'string') { try { v = JSON.parse(v); } catch (e) { /* a plain string is legitimate */ } }
      backend.emitEvent(name, v);
    },
  };

  window.__JUCE__ = {
    __ouariconStub: true,
    backend,
    initialisationData: {
      __juce__platform: ['ouaricon-stub'],
      __juce__functions: [],
      __juce__sliders: [],
      __juce__toggles: [],
      __juce__comboBoxes: [],
    },
    postMessage: () => {},
    getAndroidUserScripts: () => [],
  };
  window.__stubEmit = (name, payload) => backend.emitEvent(name, payload);
  return window.__JUCE__;
}

installGlobal();

// ── report surface, for boot-all-uis.js and the label gate ──────────────────
const report = {
  mode: 'generic',
  rangesFrom: 'neutral-defaults',
  sliders: [],
  toggles: [],
  combos: [],
  nativeKnown: [],
  nativeUnknown: [],
};
if (typeof window !== 'undefined') {
  window.__stubReport = report;
  window.__stubUnknownNativeFns = report.nativeUnknown;
}

// ── overrides, optional, per plugin ─────────────────────────────────────────
// serve-ui.js copies plugins/<Name>/tests/ui-stub/generic-overrides.json (when
// one exists) to /js/juce/generic-overrides.json. It is fetched SYNCHRONOUSLY
// through the global the harness pre-seeds rather than awaited: a module that
// awaits before exporting getSliderState would hand the page an undefined
// binding at import time.
const OVERRIDES = (typeof window !== 'undefined' && window.__stubOverrides) || {};

// ── slider / toggle / combo state ───────────────────────────────────────────

function listenerList() {
  const listeners = [];
  return {
    addListener: (fn) => listeners.push(fn),
    removeListener: (fn) => { const i = listeners.indexOf(fn); if (i >= 0) listeners.splice(i, 1); },
    callListeners: (...args) => listeners.forEach((fn) => { try { fn(...args); } catch (e) { console.warn('stub: state listener threw', e); } }),
  };
}

// Neutral, and neutral on purpose. `def` sits at the MIDDLE of the range rather
// than at `start`: a page that renders every knob pinned at minimum is a page
// in a state the plugin never ships in, and several UIs hide or grey a section
// when its depth parameter reads zero — which would remove elements the label
// gate is supposed to measure.
const NEUTRAL_RANGE = { start: 0, end: 1, skew: 1, interval: 0, def: 0.5 };

// A choice list has to be non-empty: `state.properties.choices[i]` and
// `.map(...)` over it appear unguarded in several app.js files, and an empty
// array renders an empty <select> that collapses to a different width than the
// shipping one. Single characters keep the fixture's own text from dominating
// any geometry measurement.
const NEUTRAL_CHOICES = ['1', '2', '3', '4'];

function rangeFor(name) {
  const o = OVERRIDES.sliders && OVERRIDES.sliders[name];
  return { ...NEUTRAL_RANGE, ...(o || {}) };
}

class StubSliderState {
  constructor(name) {
    const r = rangeFor(name);
    this.name = name;
    this.properties = {
      start: r.start, end: r.end, skew: r.skew, name,
      label: r.label || '', numSteps: r.numSteps ?? 100,
      interval: r.interval, parameterIndex: 0,
    };
    this.scaledValue = r.def;
    this.valueChangedEvent = listenerList();
    this.propertiesChangedEvent = listenerList();
    report.sliders.push(name);
  }
  getScaledValue() { return this.scaledValue; }
  getNormalisedValue() {
    const p = this.properties;
    if (p.end === p.start) return 0;
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
    this.value = (OVERRIDES.toggles && name in OVERRIDES.toggles) ? !!OVERRIDES.toggles[name] : false;
    this.valueChangedEvent = listenerList();
    this.propertiesChangedEvent = listenerList();
    report.toggles.push(name);
  }
  getValue() { return this.value; }
  setValue(v) { this.value = !!v; this.valueChangedEvent.callListeners(); }
}

class StubComboBoxState {
  constructor(name) {
    const o = (OVERRIDES.combos && OVERRIDES.combos[name]) || {};
    this.name = name;
    this.properties = { name, parameterIndex: 0, choices: o.choices || NEUTRAL_CHOICES };
    this.choiceIndex = o.def ?? 0;
    this.valueChangedEvent = listenerList();
    this.propertiesChangedEvent = listenerList();
    report.combos.push(name);
  }
  getChoiceIndex() { return this.choiceIndex; }
  setChoiceIndex(i) { this.choiceIndex = i; this.valueChangedEvent.callListeners(); }
}

const sliderStates = new Map();
const toggleStates = new Map();
const comboStates  = new Map();

// Exposed so a gate can run a real STATE-UPDATE PASS: driving
// setNormalisedValue fires the page's own valueChangedEvent listeners, which is
// what makes `dataset.label === textContent` a meaningful assertion rather than
// a restatement of what applyLabel just wrote. Without a way to reach the
// states, a gate can only prove the invariant holds at the moment nothing has
// happened — and the failure it guards
// (pattern_js_state_updater_overwrites_html_labels) happens precisely when
// something has.
if (typeof window !== 'undefined')
  window.__stubStates = { sliders: sliderStates, toggles: toggleStates, combos: comboStates };

export function getSliderState(name) {
  if (!sliderStates.has(name)) sliderStates.set(name, new StubSliderState(name));
  return sliderStates.get(name);
}
export function getToggleState(name) {
  if (!toggleStates.has(name)) toggleStates.set(name, new StubToggleState(name));
  return toggleStates.get(name);
}
export function getComboBoxState(name) {
  if (!comboStates.has(name)) comboStates.set(name, new StubComboBoxState(name));
  return comboStates.get(name);
}

// ── native functions ────────────────────────────────────────────────────────
//
// The override map covers the names whose RETURN SHAPE is load-bearing: a
// caller that does `(await getPresetList()).map(...)` breaks on anything but an
// array, and preset-manager.js checks `result && result.success` so a bare bool
// from a dialog fn reads as failure.
//
// Names are grouped by the module that requests them so a future addition lands
// beside its siblings rather than at the bottom of one flat list.

let uiLanguage = 'en';
let tooltipsEnabled = false;

// Deliberately English and deliberately BLAND. These strings render on the
// page, so anything long or French-looking would contaminate the very geometry
// the label gate measures. Two entries rather than one so a prev/next control
// has somewhere to go.
const PRESETS = ['Default', 'Preset A', 'Preset B'];
let currentPreset = 'Default';

const dialogResult = (ok, name) => ({ success: ok, name });

const PRESET_FNS = {
  getPresetList:              () => PRESETS.slice(),
  // Shape DIVERGES between plugins and there is no default that satisfies both:
  // O-Bells returns a juce::var OBJECT (PluginEditor.cpp:233-245) and its page
  // calls Object.keys on it; O-Prism returns a JSON STRING (:809-821) and its
  // page JSON.parses it. The object is the default because a page that
  // JSON.parses an object catches and degrades to an empty list, while a page
  // that Object.keys a string renders one bogus category per character. O-Prism
  // carries a `natives` override for its own shape.
  getPresetListGrouped:       () => ({ Factory: PRESETS.slice() }),
  getPresetListWithCategories:() => ({ Factory: PRESETS.slice() }),
  getPresetCategories:        () => ['Factory'],
  getCurrentPreset:           () => currentPreset,
  getCurrentPresetName:       () => currentPreset,
  isFactoryPreset:            (n) => PRESETS.includes(n),
  loadPreset:                 (n) => { if (!PRESETS.includes(n)) return false; currentPreset = n; return true; },
  loadPresetByName:           (n) => { if (!PRESETS.includes(n)) return false; currentPreset = n; return true; },
  loadPresetFromCategory:     (_c, n) => { currentPreset = n || currentPreset; return true; },
  applyFactoryPreset:         (n) => { currentPreset = n || currentPreset; return true; },
  applyPreset:                (n) => { currentPreset = n || currentPreset; return true; },
  savePreset:                 () => true,
  saveCurrentPreset:          () => true,
  savePresetWithDialog:       () => dialogResult(true, 'Stub Preset'),
  loadPresetFromFile:         () => dialogResult(true, PRESETS[0]),
  deletePreset:               () => false,   // a factory-only stub list has nothing deletable
  renamePreset:               () => false,
  selectNextPreset:           () => { const i = PRESETS.indexOf(currentPreset); currentPreset = PRESETS[(i + 1 + PRESETS.length) % PRESETS.length]; return currentPreset; },
  selectPreviousPreset:       () => { const i = PRESETS.indexOf(currentPreset); currentPreset = PRESETS[(i - 1 + PRESETS.length) % PRESETS.length]; return currentPreset; },
  loadNextPreset:             () => PRESET_FNS.selectNextPreset(),
  loadPrevPreset:             () => PRESET_FNS.selectPreviousPreset(),
};

const I18N_FNS = {
  // The C++ clamp is mirrored — anything that is not "fr" degrades to "en" — so
  // a stub that accepted a bogus code could not make a broken page look right.
  setUiLanguage: (code) => { uiLanguage = code === 'fr' ? 'fr' : 'en'; return uiLanguage; },
  getUiLanguage: () => uiLanguage,
  setTooltipsEnabled: (v) => { tooltipsEnabled = !!v; return tooltipsEnabled; },
  getTooltipsEnabled: () => tooltipsEnabled,
  setTipsEnabled: (v) => { tooltipsEnabled = !!v; return tooltipsEnabled; },
  getTipsEnabled: () => tooltipsEnabled,
};

// A 12-EDO identity table. Every tuning-panel consumer reads intervals as an
// array of cents and a tonic as a MIDI note; a null there throws inside the
// shared module before the page finishes painting.
// Three of these are JSON STRINGS, not arrays. The shared tuning-panel module
// does JSON.parse on what getTuningIntervals and getEmbeddedTuningList return
// (modules/tuning/scala-tuning-engine/js/tuning-panel.js:258, :709) and so does
// O-IntonationPad's own copy — an array there parses to "0,100,200" and throws
// on the second character. Nine plugins load that module, so getting this wrong
// is nine console errors, which is what the first boot run reported.
const TUNING_FNS = {
  getTuningIntervals:   () => JSON.stringify([0, 100, 200, 300, 400, 500, 600, 700, 800, 900, 1000, 1100]),
  setTuningIntervals:   () => true,
  getTuningName:        () => '12-EDO',
  getTonicNote:         () => 60,
  setTonicNote:         () => true,
  getScaleDegreeCount:  () => 12,
  getEnabledIntervals:  () => JSON.stringify([]),
  resetEnabledIntervals:() => true,
  setIntervalEnabled:   () => true,
  setSingleInterval:    () => true,
  setSingleIntervalEncoded: () => true,
  getOctaveStretch:     () => 0,
  setOctaveStretch:     () => true,
  getMasterTune:        () => 440,
  setMasterTune:        () => true,
  getEmbeddedTuningList:() => JSON.stringify([]),
  loadEmbeddedTuning:   () => false,
  loadScalaFile:        () => dialogResult(false, ''),
  loadKBMFile:          () => dialogResult(false, ''),
  saveScalaFile:        () => dialogResult(false, ''),
  saveKBMFile:          () => dialogResult(false, ''),
  openTuningFilePicker: () => dialogResult(false, ''),
  exportTuningHTML:     () => '',
  generateEDO:          () => true,
  generateRank2:        () => true,
  generateHarmonicSeries: () => true,
  applyGeneratedScale:  () => true,
  setTemperamentPreset: () => true,
  getGlissCustomDegrees:() => [],
  setGlissCustomDegrees:() => true,
};

const MISC_FNS = {
  getParameterDefaults: () => ({}),
  getSampleRate:        () => 48000,
  getPluginVersion:     () => '0.0.0-stub',
  getVoiceCount:        () => 0,
  getHeldNotesJson:     () => '[]',
  getSpeakerLayout:     () => '{}',
  getDownmixStatus:     () => '{}',
  getVisualizationState:() => '{}',
  getTriggerState:      () => '{}',
  getTechniqueState:    () => '{}',
  getSampleMap:         () => '{}',
  getGrid:              () => '[]',
  getGrainMeter:        () => 0,
  getModSourceNames:    () => [],
  getModDestNames:      () => [],
  getLayoutList:        () => [],
  getUserWavetableList: () => [],
  getWaveformData:      () => [],
  getWaveformPeaks:     () => [],
  getWindowCurve:       () => [],
  getSourceThumbnail:   () => '',
  getActiveOscInfo:     () => '{}',
  getActiveOscFrame:    () => [],
  getWavetableFrameForPosition: () => [],
  getPendingMissingFolder: () => '',
  wasLastLoadTruncated: () => false,
  getLyricsText:        () => '',
  getLyricsPosition:    () => 0,
  getLyricsLooping:     () => false,
  requestEnvelope:      () => '{"v":1,"points":[{"x":0,"y":0.5,"curve":0},{"x":1,"y":0.5,"curve":0}]}',
  commitEnvelope:       (json) => json,
};

const KNOWN = Object.assign({}, PRESET_FNS, I18N_FNS, TUNING_FNS, MISC_FNS);

// ── the benign default for an unmodelled name ───────────────────────────────
//
// There is no single value that is safe for every caller, so the shape is
// inferred from the name and the guess is REPORTED rather than hidden. The
// alternative — rejecting, as the hand-written stubs do — turns every
// unmodelled name into a console error and would make the boot report measure
// this file's coverage instead of the page's health.
function benignDefault(name) {
  if (/^(is|has|was|can|should)[A-Z]/.test(name)) return false;
  if (/(List|Names|Categories|Intervals|Degrees|Peaks|Curve|Data|Frame|Notes)$/.test(name)) return [];
  if (/Json$/.test(name)) return '{}';
  if (/^(set|apply|commit|reset|clear|remove|add|move|save|delete|import|export|select|trigger|send|report|randomize|toggle|drop|locate|override|pick|load|open)/.test(name)) return true;
  if (/^get/.test(name)) return null;
  return null;
}

// The per-plugin escape hatch, for the names whose shape genuinely differs
// between plugins. A value here wins over KNOWN and over the benign default.
const NATIVE_OVERRIDES = OVERRIDES.natives || {};

export function getNativeFunction(name) {
  if (Object.prototype.hasOwnProperty.call(NATIVE_OVERRIDES, name)) {
    if (!report.nativeKnown.includes(name)) report.nativeKnown.push(name);
    const v = NATIVE_OVERRIDES[name];
    return () => Promise.resolve(v);
  }

  if (Object.prototype.hasOwnProperty.call(KNOWN, name)) {
    if (!report.nativeKnown.includes(name)) report.nativeKnown.push(name);
    return (...args) => Promise.resolve(KNOWN[name](...args));
  }

  if (!report.nativeUnknown.includes(name)) report.nativeUnknown.push(name);
  const value = benignDefault(name);
  return () => Promise.resolve(value);
}

// ── the rest of the real module's export surface ────────────────────────────
// js/juce/index.js exports six names. A page doing `import * as Juce` and then
// `new Juce.ControlParameterIndexUpdater(...)` gets an undefined constructor
// unless all six are here, and that throws at module scope — which kills the
// whole UI while leaving the HTML looking correct
// (pattern_module_toplevel_init_tdz).
export function getBackendResourceAddress(path) { return path; }

export class ControlParameterIndexUpdater {
  constructor(annotation) { this.controlParameterIndexAnnotation = annotation; }
  handleMouseMove() {}
  handleMouseDown() {}
  handleMouseUp() {}
}
