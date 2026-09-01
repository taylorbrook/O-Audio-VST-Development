/*
   This file is part of O-Formant, an Ouaricon Audio plugin.
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
import { getSliderState, getToggleState, getComboBoxState, getNativeFunction } from './juce/index.js';
// NAMESPACE import as well as the named one, and it is not decoration.
// The canon block below reaches the language bridge as `Juce.getNativeFunction`
// — that spelling is fixed, because scripts/i18n-canon.js is byte-compared by
// check-i18n assertion 6 and every other plugin in the suite spells it the same
// way. v1.26.0 shipped WITHOUT this line, so `Juce` was never bound in this
// module and initI18n's first statement threw ReferenceError. The throw landed
// in initI18n's own try/catch and degraded to the "session-only" console.warn,
// which no gate fails on (boot-all-uis counts console.error, not warn), so the
// language preference was never read from C++ at open and never written back on
// change — the C++ half (PluginEditor.cpp:167-181, PluginProcessor uiLanguage)
// was correct and complete all along. pattern_webview_native_fn_bridge_gap.
import * as Juce from './juce/index.js';

// ============================================================================
// i18n — canon v2, VERBATIM from scripts/i18n-canon.js (assertion 6 byte-
// compares this region after comment stripping and whitespace normalisation).
// Do not edit the block below; edit scripts/i18n-canon.js and re-copy.
//
// initI18n() is called from the DOMContentLoaded handler below, inside a
// try/catch: a throw here would take every later initializer on the page with
// it (pattern_module_toplevel_init_tdz).
// ============================================================================
import { LANGUAGES, I18N, LABELS, TIP_BINDINGS, tr } from './i18n.js';

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
            .then((code) => applyI18n(code))
            .catch((e) => console.warn('Could not read language preference:', e));
    }

    const sel = document.getElementById('lang-select');
    if (sel) sel.addEventListener('change', (e) => {
        applyI18n(e.target.value);
        if (setUiLanguageNative) setUiLanguageNative(uiLanguage).catch(() => {});
    });
}


// Relay states
let vowelXState, vowelYState, vowelFocusState;
let glottalRdState, breathinessState, vibratoRateState, vibratoDepthState, vibratoDelayState, jitterState, shimmerState, rdModDepthState, spectralTiltState;
let consonantLevelState, consonantToneState, sibilanceState, consonantVoicingState, autoConsonantState;
let consonantAttackState, consonantHoldState, consonantDecayState, consonantTransitionState;
let attackState, decayState, sustainState, releaseState;
let formantTopologyState;
let formantShiftState, formantSpreadState, pitchGlideState, transitionTimeState, singersFormantState;
let nasalCouplingState, nasalPlaceState;
let outputGainState, stereoWidthState;
// Effects
let chorusBypassState, chorusRateState, chorusDepthState, chorusMixState;
let delayBypassState, delayTimeState, delayFeedbackState, delayModeState, delayMixState;
let reverbBypassState, reverbSizeState, reverbDampState, reverbPredelayState, reverbMixState, reverbModState, reverbShimmerState;
let eqBypassState, eqLowGainState, eqMidGainState, eqMidFreqState, eqHighGainState;

// Vowel XY pad
const canvas = document.getElementById('xy-pad');
const ctx = canvas ? canvas.getContext('2d') : null;
let isDraggingXY = false;
let dpr = 1;

// Consonant XY pad
let cxyCanvas, cxyCtx, cxyDpr;
let isDraggingCXY = false;

// Lyrics XY animation state
let lyricsAnimating = false;
let lyricsTarget = null; // { vowelX, vowelY, consonantTone, sibilance }

// IPA vowel positions (normalised X, Y where Y=1 is top)
const vowelLabels = [
  { label: 'i',  x: 0.00, y: 1.00 },
  { label: 'e',  x: 0.31, y: 0.43 },
  { label: '\u0251', x: 0.83, y: 0.00 },
  { label: 'o',  x: 1.00, y: 0.35 },
  { label: 'u',  x: 0.98, y: 0.93 },
  { label: 'r',  x: 0.12, y: 0.72 },
  { label: 'l',  x: 0.55, y: 0.85 },
];

// Vowel formant data (matches VowelData.h exactly)
const VOWELS = [
  { name: 'A', x: 0.83, y: 0.00,
    freq: [600, 1040, 2250, 2450, 2750],
    bw: [60, 70, 110, 120, 130],
    gain: [1.0, 0.4467, 0.3548, 0.3548, 0.1000] },
  { name: 'E', x: 0.31, y: 0.43,
    freq: [400, 1620, 2400, 2800, 3100],
    bw: [40, 80, 100, 120, 120],
    gain: [1.0, 0.2512, 0.3548, 0.2512, 0.1259] },
  { name: 'I', x: 0.00, y: 1.00,
    freq: [250, 1750, 2600, 3050, 3340],
    bw: [60, 90, 100, 120, 120],
    gain: [1.0, 0.0316, 0.1585, 0.0794, 0.0398] },
  { name: 'O', x: 1.00, y: 0.35,
    freq: [400, 750, 2400, 2600, 2900],
    bw: [40, 80, 100, 120, 120],
    gain: [1.0, 0.2818, 0.0891, 0.1000, 0.0100] },
  { name: 'U', x: 0.98, y: 0.93,
    freq: [350, 600, 2400, 2675, 2950],
    bw: [40, 80, 100, 120, 120],
    gain: [1.0, 0.1000, 0.0251, 0.0398, 0.0158] },
  { name: 'R', x: 0.12, y: 0.72,
    freq: [340, 1050, 1600, 3500, 4300],
    bw: [60, 90, 130, 250, 280],
    gain: [1.0, 0.3981, 0.1995, 0.0631, 0.0316] },
  { name: 'L', x: 0.55, y: 0.85,
    freq: [400, 900, 2600, 3400, 4200],
    bw: [80, 120, 150, 250, 280],
    gain: [1.0, 0.5012, 0.1585, 0.0794, 0.0398] },
];

// Shepard IDW interpolation (matches VowelMorpher.h)
function computeFormants(cursorX, cursorY, focus) {
  const weights = [];
  let weightSum = 0;
  for (const v of VOWELS) {
    const dx = cursorX - v.x;
    const dy = cursorY - v.y;
    const dist = Math.sqrt(dx * dx + dy * dy);
    if (dist < 1e-6) {
      return { freq: [...v.freq], bw: [...v.bw], gain: [...v.gain] };
    }
    const w = 1.0 / Math.pow(dist, focus);
    weights.push(w);
    weightSum += w;
  }
  const invSum = 1.0 / weightSum;
  const freq = [0, 0, 0, 0, 0];
  const bw = [0, 0, 0, 0, 0];
  const gain = [0, 0, 0, 0, 0];
  for (let v = 0; v < 5; v++) {
    const nw = weights[v] * invSum;
    for (let f = 0; f < 5; f++) {
      freq[f] += nw * Math.log(VOWELS[v].freq[f]);
      bw[f]   += nw * VOWELS[v].bw[f];
      gain[f] += nw * VOWELS[v].gain[f];
    }
  }
  for (let f = 0; f < 5; f++) freq[f] = Math.exp(freq[f]);
  return { freq, bw, gain };
}

// Formant shift + spread (matches FormantFilterBank.h lines 42-61)
function applyShiftSpread(freq, shift, spread) {
  const shiftFactor = Math.pow(2, shift / 12);
  const shifted = freq.map(f => f * shiftFactor);
  const centerOfMass = shifted.reduce((a, b) => a + b, 0) / 5;
  return shifted.map(f => {
    const distance = f - centerOfMass;
    return Math.max(20, centerOfMass + distance * spread);
  });
}

// Knob drag state
let activeKnob = null;
let knobStartY = 0;
let knobStartNorm = 0;

document.addEventListener('DOMContentLoaded', () => {
  // i18n FIRST and inside try/catch. It paints the default language
  // synchronously (never blank, never a flash) and a throw here must not take
  // the rest of this handler with it.
  // setupTooltips() sits INSIDE the same try/catch and AFTER initI18n(),
  // because it reads data-tip attributes that applyI18n has not written yet at
  // module-evaluation time, and because a throw out of a top-level call here
  // would take every initializer below it (pattern_module_toplevel_init_tdz).
  try { initI18n(); setupTooltips(); } catch (e) { console.error('i18n init failed:', e); }
  bindSettingsPopover();
  watchLanguageForCanvasRepaint();
  initRelays();
  setupCanvas();
  setupConsonantXYCanvas();
  setupADSRCanvas();
  bindXYPad();
  bindConsonantXYPad();
  bindKnobs();
  bindToggle();
  bindTopologySelector();
  bindEffectsControls();
  drawADSR();
  initPresetBrowser();
  initLyricsTab();
});

function initRelays() {
  vowelXState = getSliderState('vowelXSlider');
  vowelYState = getSliderState('vowelYSlider');
  vowelFocusState = getSliderState('vowelFocusSlider');
  glottalRdState = getSliderState('glottalRdSlider');
  breathinessState = getSliderState('breathinessSlider');
  vibratoRateState = getSliderState('vibratoRateSlider');
  vibratoDepthState = getSliderState('vibratoDepthSlider');
  vibratoDelayState = getSliderState('vibratoDelaySlider');
  jitterState = getSliderState('jitterSlider');
  shimmerState = getSliderState('shimmerSlider');
  rdModDepthState = getSliderState('rdModDepthSlider');
  spectralTiltState = getSliderState('spectralTiltSlider');
  consonantLevelState = getSliderState('consonantLevelSlider');
  consonantToneState = getSliderState('consonantToneSlider');
  sibilanceState = getSliderState('sibilanceSlider');
  consonantVoicingState = getSliderState('consonantVoicingSlider');
  autoConsonantState = getToggleState('autoConsonantToggle');
  consonantAttackState = getSliderState('consonantAttackSlider');
  consonantHoldState = getSliderState('consonantHoldSlider');
  consonantDecayState = getSliderState('consonantDecaySlider');
  consonantTransitionState = getSliderState('consonantTransitionSlider');
  attackState = getSliderState('attackSlider');
  decayState = getSliderState('decaySlider');
  sustainState = getSliderState('sustainSlider');
  releaseState = getSliderState('releaseSlider');
  formantTopologyState = getComboBoxState('formantTopologyComboBox');
  formantShiftState = getSliderState('formantShiftSlider');
  formantSpreadState = getSliderState('formantSpreadSlider');
  pitchGlideState = getSliderState('pitchGlideSlider');
  transitionTimeState = getSliderState('transitionTimeSlider');
  singersFormantState = getSliderState('singersFormantSlider');
  nasalCouplingState = getSliderState('nasalCouplingSlider');
  nasalPlaceState = getSliderState('nasalPlaceSlider');
  outputGainState = getSliderState('outputGainSlider');
  stereoWidthState = getSliderState('stereoWidthSlider');

  // Effects relays
  chorusBypassState = getToggleState('chorusBypassToggle');
  chorusRateState = getSliderState('chorusRateSlider');
  chorusDepthState = getSliderState('chorusDepthSlider');
  chorusMixState = getSliderState('chorusMixSlider');
  delayBypassState = getToggleState('delayBypassToggle');
  delayTimeState = getSliderState('delayTimeSlider');
  delayFeedbackState = getSliderState('delayFeedbackSlider');
  delayModeState = getComboBoxState('delayModeComboBox');
  delayMixState = getSliderState('delayMixSlider');
  reverbBypassState = getToggleState('reverbBypassToggle');
  reverbSizeState = getSliderState('reverbSizeSlider');
  reverbDampState = getSliderState('reverbDampSlider');
  reverbPredelayState = getSliderState('reverbPredelaySlider');
  reverbMixState = getSliderState('reverbMixSlider');
  reverbModState = getSliderState('reverbModSlider');
  reverbShimmerState = getSliderState('reverbShimmerSlider');
  eqBypassState = getToggleState('eqBypassToggle');
  eqLowGainState = getSliderState('eqLowGainSlider');
  eqMidGainState = getSliderState('eqMidGainSlider');
  eqMidFreqState = getSliderState('eqMidFreqSlider');
  eqHighGainState = getSliderState('eqHighGainSlider');

  // Map param ID -> relay state for knob bindings
  // (consonantTone and sibilance excluded — controlled by consonant XY pad)
  const paramMap = {
    glottalRd: glottalRdState,
    breathiness: breathinessState,
    vibratoRate: vibratoRateState,
    vibratoDepth: vibratoDepthState,
    vibratoDelay: vibratoDelayState,
    jitter: jitterState,
    shimmer: shimmerState,
    rdModDepth: rdModDepthState,
    spectralTilt: spectralTiltState,
    consonantLevel: consonantLevelState,
    consonantVoicing: consonantVoicingState,
    consonantAttack: consonantAttackState,
    consonantHold: consonantHoldState,
    consonantDecay: consonantDecayState,
    consonantTransition: consonantTransitionState,
    formantShift: formantShiftState,
    formantSpread: formantSpreadState,
    pitchGlide: pitchGlideState,
    transitionTime: transitionTimeState,
    singersFormant: singersFormantState,
    nasalCoupling: nasalCouplingState,
    nasalPlace: nasalPlaceState,
    vowelFocus: vowelFocusState,
    attack: attackState,
    decay: decayState,
    sustain: sustainState,
    release: releaseState,
    outputGain: outputGainState,
    stereoWidth: stereoWidthState,
    chorusRate: chorusRateState,
    chorusDepth: chorusDepthState,
    chorusMix: chorusMixState,
    delayTime: delayTimeState,
    delayFeedback: delayFeedbackState,
    delayMix: delayMixState,
    reverbSize: reverbSizeState,
    reverbDamp: reverbDampState,
    reverbPredelay: reverbPredelayState,
    reverbMix: reverbMixState,
    reverbMod: reverbModState,
    reverbShimmer: reverbShimmerState,
    eqLowGain: eqLowGainState,
    eqMidGain: eqMidGainState,
    eqMidFreq: eqMidFreqState,
    eqHighGain: eqHighGainState,
  };

  // Listen for automation changes on all slider relays
  for (const [paramId, state] of Object.entries(paramMap)) {
    state.valueChangedEvent.addListener(() => updateKnobVisual(paramId, state));
    state.propertiesChangedEvent.addListener(() => updateKnobVisual(paramId, state));
  }

  // Vowel XY automation
  vowelXState.valueChangedEvent.addListener(() => drawXYPad());
  vowelYState.valueChangedEvent.addListener(() => drawXYPad());

  // Consonant XY automation (place + manner)
  consonantToneState.valueChangedEvent.addListener(() => drawConsonantXYPad());
  sibilanceState.valueChangedEvent.addListener(() => drawConsonantXYPad());

  // Formant-affecting params -> redraw XY pad
  formantShiftState.valueChangedEvent.addListener(() => drawXYPad());
  formantSpreadState.valueChangedEvent.addListener(() => drawXYPad());
  vowelFocusState.valueChangedEvent.addListener(() => drawXYPad());

  // ADSR automation -> redraw ADSR canvas
  attackState.valueChangedEvent.addListener(() => drawADSR());
  decayState.valueChangedEvent.addListener(() => drawADSR());
  sustainState.valueChangedEvent.addListener(() => drawADSR());
  releaseState.valueChangedEvent.addListener(() => drawADSR());

  // Toggle automation
  autoConsonantState.valueChangedEvent.addListener(() => updateToggleVisual());

  // Topology automation
  formantTopologyState.valueChangedEvent.addListener(() => updateTopologyVisual());

  // Store map globally
  window._paramMap = paramMap;
}

// ============================================================================
// Canvas setup
// ============================================================================
function setupCanvas() {
  if (!canvas) return;
  dpr = window.devicePixelRatio || 1;
  const wrap = canvas.parentElement;
  const w = wrap.clientWidth;
  const h = wrap.clientHeight;
  canvas.style.width = w + 'px';
  canvas.style.height = h + 'px';
  canvas.width = w * dpr;
  canvas.height = h * dpr;
  drawXYPad();
}

// ============================================================================
// ADSR Canvas
// ============================================================================
let adsrCanvas, adsrCtx, adsrDpr;

function setupADSRCanvas() {
  adsrCanvas = document.getElementById('adsr-canvas');
  if (!adsrCanvas) return;
  adsrDpr = window.devicePixelRatio || 1;
  const w = adsrCanvas.clientWidth;
  const h = adsrCanvas.clientHeight;
  adsrCanvas.width = w * adsrDpr;
  adsrCanvas.height = h * adsrDpr;
  adsrCtx = adsrCanvas.getContext('2d');
}

function drawADSR() {
  if (!adsrCtx) return;
  const w = adsrCanvas.clientWidth;
  const h = adsrCanvas.clientHeight;
  adsrCtx.setTransform(adsrDpr, 0, 0, adsrDpr, 0, 0);
  adsrCtx.clearRect(0, 0, w, h);

  const attack = attackState ? attackState.getScaledValue() : 10;
  const decay = decayState ? decayState.getScaledValue() : 300;
  const sustain = sustainState ? sustainState.getScaledValue() : 0.8;
  const release = releaseState ? releaseState.getScaledValue() : 500;

  const pad = 4;
  const usableW = w - pad * 2;
  const usableH = h - pad * 2;

  const total = attack + decay + release;
  const sustainWidth = usableW * 0.2;
  const timeWidth = usableW - sustainWidth;
  const scale = total > 0 ? timeWidth / total : 1;

  const x0 = pad;
  const yBot = h - pad;
  const yTop = pad;
  const ySus = yBot - sustain * usableH;

  const xA = x0 + attack * scale;
  const xD = xA + decay * scale;
  const xS = xD + sustainWidth;
  const xR = xS + release * scale;

  // Fill
  adsrCtx.beginPath();
  adsrCtx.moveTo(x0, yBot);
  adsrCtx.lineTo(xA, yTop);
  adsrCtx.lineTo(xD, ySus);
  adsrCtx.lineTo(xS, ySus);
  adsrCtx.lineTo(xR, yBot);
  adsrCtx.closePath();
  adsrCtx.fillStyle = 'rgba(139, 168, 112, 0.08)';
  adsrCtx.fill();

  // Stroke
  adsrCtx.beginPath();
  adsrCtx.moveTo(x0, yBot);
  adsrCtx.lineTo(xA, yTop);
  adsrCtx.lineTo(xD, ySus);
  adsrCtx.lineTo(xS, ySus);
  adsrCtx.lineTo(xR, yBot);
  adsrCtx.strokeStyle = '#8BA870';
  adsrCtx.lineWidth = 1.5;
  adsrCtx.stroke();
}

// ============================================================================
// XY Pad
// ============================================================================
function bindXYPad() {
  if (!canvas) return;

  canvas.addEventListener('pointerdown', (e) => {
    if (lyricsAnimating) return;
    isDraggingXY = true;
    canvas.setPointerCapture(e.pointerId);
    vowelXState.sliderDragStarted();
    vowelYState.sliderDragStarted();
    updateXYFromPointer(e);
  });

  canvas.addEventListener('pointermove', (e) => {
    if (!isDraggingXY) return;
    updateXYFromPointer(e);
  });

  canvas.addEventListener('pointerup', () => {
    if (!isDraggingXY) return;
    isDraggingXY = false;
    vowelXState.sliderDragEnded();
    vowelYState.sliderDragEnded();
  });

  canvas.addEventListener('pointercancel', () => {
    if (isDraggingXY) {
      isDraggingXY = false;
      vowelXState.sliderDragEnded();
      vowelYState.sliderDragEnded();
    }
  });
}

function updateXYFromPointer(e) {
  const rect = canvas.getBoundingClientRect();
  const normX = Math.max(0, Math.min(1, (e.clientX - rect.left) / rect.width));
  const normY = Math.max(0, Math.min(1, 1.0 - (e.clientY - rect.top) / rect.height));
  vowelXState.setNormalisedValue(normX);
  vowelYState.setNormalisedValue(normY);
  drawXYPad();
}

function drawXYPad() {
  if (!ctx) return;
  const w = canvas.width;
  const h = canvas.height;
  ctx.setTransform(dpr, 0, 0, dpr, 0, 0);

  const cw = w / dpr;
  const ch = h / dpr;

  ctx.clearRect(0, 0, cw, ch);

  // Grid lines
  ctx.strokeStyle = 'rgba(139,115,85,0.15)';
  ctx.lineWidth = 1;
  for (let i = 1; i < 5; i++) {
    const gx = (cw * i) / 5;
    const gy = (ch * i) / 5;
    ctx.beginPath(); ctx.moveTo(gx, 0); ctx.lineTo(gx, ch); ctx.stroke();
    ctx.beginPath(); ctx.moveTo(0, gy); ctx.lineTo(cw, gy); ctx.stroke();
  }

  // Vowel labels
  ctx.font = '14px Garamond, Times New Roman, serif';
  ctx.fillStyle = 'rgba(60,47,47,0.5)';
  ctx.textAlign = 'center';
  ctx.textBaseline = 'middle';
  const pad = 16;
  for (const v of vowelLabels) {
    const lx = pad + v.x * (cw - pad * 2);
    const ly = pad + (1.0 - v.y) * (ch - pad * 2);
    ctx.fillText(v.label, lx, ly);
  }

  // Cursor — use lyrics target position when animating
  const normX = lyricsAnimating && lyricsTarget ? lyricsTarget.vowelX : vowelXState.getNormalisedValue();
  const normY = lyricsAnimating && lyricsTarget ? lyricsTarget.vowelY : vowelYState.getNormalisedValue();
  const cx = pad + normX * (cw - pad * 2);
  const cy = pad + (1.0 - normY) * (ch - pad * 2);

  // Lyrics mode overlay
  if (lyricsAnimating) {
    ctx.fillStyle = 'rgba(245, 230, 211, 0.25)';
    ctx.fillRect(0, 0, cw, ch);
    ctx.font = '9px Garamond, Times New Roman, serif';
    ctx.fillStyle = 'rgba(107, 142, 78, 0.6)';
    ctx.textAlign = 'right';
    ctx.textBaseline = 'top';
    // A canvas string is invisible to BOTH gates: assertion 10 walks text
    // nodes and assertion 12 scans textContent writes, and fillText is
    // neither. Read through trLabel, repainted by
    // watchLanguageForCanvasRepaint() below.
    ctx.fillText(trLabel('canvas.lyrics', uiLanguage), cw - 6, 4);
    ctx.textAlign = 'center';
    ctx.textBaseline = 'middle';
  }

  // Cursor glow
  const glowColor = lyricsAnimating ? 'rgba(107, 142, 78, 0.4)' : 'rgba(139, 168, 112, 0.3)';
  const grad = ctx.createRadialGradient(cx, cy, 0, cx, cy, 28);
  grad.addColorStop(0, glowColor);
  grad.addColorStop(1, 'rgba(139, 168, 112, 0.0)');
  ctx.fillStyle = grad;
  ctx.beginPath();
  ctx.arc(cx, cy, 28, 0, Math.PI * 2);
  ctx.fill();

  // Crosshair
  ctx.strokeStyle = lyricsAnimating ? 'rgba(107,142,78,0.5)' : 'rgba(139,163,112,0.4)';
  ctx.lineWidth = 1;
  ctx.beginPath(); ctx.moveTo(cx, 0); ctx.lineTo(cx, ch); ctx.stroke();
  ctx.beginPath(); ctx.moveTo(0, cy); ctx.lineTo(cw, cy); ctx.stroke();

  // Dot
  ctx.beginPath();
  ctx.arc(cx, cy, 6, 0, Math.PI * 2);
  ctx.fillStyle = lyricsAnimating ? '#6B8E4E' : '#8BA870';
  ctx.fill();
  ctx.strokeStyle = '#3C2F2F';
  ctx.lineWidth = 1.5;
  ctx.stroke();

  // Inner dot
  ctx.beginPath();
  ctx.arc(cx, cy, 2, 0, Math.PI * 2);
  ctx.fillStyle = '#F5E6D3';
  ctx.fill();

  // Formant dot overlay (F1-F5)
  const shift = formantShiftState ? formantShiftState.getScaledValue() : 0;
  const spread = formantSpreadState ? formantSpreadState.getScaledValue() : 1;
  const focus = vowelFocusState ? vowelFocusState.getScaledValue() : 2.5;
  const formants = computeFormants(normX, normY, focus);
  const fFreqs = applyShiftSpread(formants.freq, shift, spread);
  const logMin = Math.log(200);
  const logMax = Math.log(5000);
  const logRange = logMax - logMin;

  ctx.font = '8px Garamond, Times New Roman, serif';
  ctx.textAlign = 'center';
  ctx.textBaseline = 'top';
  for (let i = 0; i < 5; i++) {
    const fx = pad + Math.max(0, Math.min(1, (Math.log(fFreqs[i]) - logMin) / logRange)) * (cw - pad * 2);
    const fy = pad + (1.0 - formants.gain[i] * 0.3) * (ch - pad * 2);
    ctx.beginPath();
    ctx.arc(fx, fy, 3.5, 0, Math.PI * 2);
    ctx.fillStyle = 'rgba(139, 168, 112, 0.5)';
    ctx.fill();
    ctx.strokeStyle = 'rgba(139, 168, 112, 0.8)';
    ctx.lineWidth = 1;
    ctx.stroke();
    ctx.fillStyle = 'rgba(139, 168, 112, 0.6)';
    ctx.fillText('F' + (i + 1), fx, fy + 5);
  }
}

// ============================================================================
// Consonant XY Pad
// ============================================================================
function setupConsonantXYCanvas() {
  cxyCanvas = document.getElementById('consonant-xy-pad');
  if (!cxyCanvas) return;
  cxyDpr = window.devicePixelRatio || 1;
  const wrap = cxyCanvas.parentElement;
  const w = wrap.clientWidth;
  const h = wrap.clientHeight;
  cxyCanvas.style.width = w + 'px';
  cxyCanvas.style.height = h + 'px';
  cxyCanvas.width = w * cxyDpr;
  cxyCanvas.height = h * cxyDpr;
  cxyCtx = cxyCanvas.getContext('2d');
  drawConsonantXYPad();
}

function bindConsonantXYPad() {
  if (!cxyCanvas) return;

  cxyCanvas.addEventListener('pointerdown', (e) => {
    if (lyricsAnimating) return;
    isDraggingCXY = true;
    cxyCanvas.setPointerCapture(e.pointerId);
    consonantToneState.sliderDragStarted();
    sibilanceState.sliderDragStarted();
    updateConsonantXYFromPointer(e);
  });

  cxyCanvas.addEventListener('pointermove', (e) => {
    if (!isDraggingCXY) return;
    updateConsonantXYFromPointer(e);
  });

  cxyCanvas.addEventListener('pointerup', () => {
    if (!isDraggingCXY) return;
    isDraggingCXY = false;
    consonantToneState.sliderDragEnded();
    sibilanceState.sliderDragEnded();
  });

  cxyCanvas.addEventListener('pointercancel', () => {
    if (isDraggingCXY) {
      isDraggingCXY = false;
      consonantToneState.sliderDragEnded();
      sibilanceState.sliderDragEnded();
    }
  });
}

function updateConsonantXYFromPointer(e) {
  const rect = cxyCanvas.getBoundingClientRect();
  const normX = Math.max(0, Math.min(1, (e.clientX - rect.left) / rect.width));
  const normY = Math.max(0, Math.min(1, 1.0 - (e.clientY - rect.top) / rect.height));
  consonantToneState.setNormalisedValue(normX);
  sibilanceState.setNormalisedValue(normY);
  drawConsonantXYPad();
}

// IPA consonant positions (X=place, Y=manner where 0=plosive, 1=fricative)
const consonantLabels = [
  // Plosives (bottom)
  { label: 'p',         x: 0.00, y: 0.05 },
  { label: 't',         x: 0.33, y: 0.05 },
  { label: 'k',         x: 1.00, y: 0.05 },
  // Fricatives (top)
  { label: 'f',         x: 0.08, y: 0.95 },
  { label: 's',         x: 0.33, y: 0.95 },
  { label: '\u0283',    x: 0.67, y: 0.95 },  // ʃ (palatal)
  // Nasals (mid)
  { label: 'm',         x: 0.00, y: 0.50 },
  { label: 'n',         x: 0.33, y: 0.50 },
  { label: '\u014B',    x: 1.00, y: 0.50 },  // ŋ (velar)
];

// Place frequency mapping (matches ConsonantEngine.h)
function computePlaceFreq(place) {
  if (place <= 0.33) return 500 + (place / 0.33) * 2500;
  if (place <= 0.67) return 3000 + ((place - 0.33) / 0.34) * 3000;
  return 6000 - ((place - 0.67) / 0.33) * 4000;
}

function drawConsonantXYPad() {
  if (!cxyCtx) return;
  const w = cxyCanvas.width;
  const h = cxyCanvas.height;
  cxyCtx.setTransform(cxyDpr, 0, 0, cxyDpr, 0, 0);

  const cw = w / cxyDpr;
  const ch = h / cxyDpr;
  cxyCtx.clearRect(0, 0, cw, ch);

  // Grid
  cxyCtx.strokeStyle = 'rgba(139,115,85,0.12)';
  cxyCtx.lineWidth = 1;
  for (let i = 1; i < 4; i++) {
    const gx = (cw * i) / 4;
    cxyCtx.beginPath(); cxyCtx.moveTo(gx, 0); cxyCtx.lineTo(gx, ch); cxyCtx.stroke();
  }
  const midY = ch / 2;
  cxyCtx.beginPath(); cxyCtx.moveTo(0, midY); cxyCtx.lineTo(cw, midY); cxyCtx.stroke();

  // IPA consonant labels
  const pad = 6;
  cxyCtx.font = '12px Garamond, Times New Roman, serif';
  cxyCtx.fillStyle = 'rgba(60,47,47,0.4)';
  cxyCtx.textAlign = 'center';
  cxyCtx.textBaseline = 'middle';
  for (const c of consonantLabels) {
    const lx = pad + c.x * (cw - pad * 2);
    const ly = pad + (1.0 - c.y) * (ch - pad * 2);
    cxyCtx.fillText(c.label, lx, ly);
  }

  // Cursor — use lyrics target when animating
  const normX = lyricsAnimating && lyricsTarget ? lyricsTarget.consonantTone : consonantToneState.getNormalisedValue();
  const normY = lyricsAnimating && lyricsTarget ? lyricsTarget.sibilance : sibilanceState.getNormalisedValue();
  const cx = pad + normX * (cw - pad * 2);
  const cy = pad + (1.0 - normY) * (ch - pad * 2);

  // Lyrics mode overlay
  if (lyricsAnimating) {
    cxyCtx.fillStyle = 'rgba(245, 230, 211, 0.25)';
    cxyCtx.fillRect(0, 0, cw, ch);
  }

  // Cursor glow
  const glowColor = lyricsAnimating ? 'rgba(107, 142, 78, 0.4)' : 'rgba(139, 168, 112, 0.3)';
  const grad = cxyCtx.createRadialGradient(cx, cy, 0, cx, cy, 20);
  grad.addColorStop(0, glowColor);
  grad.addColorStop(1, 'rgba(139, 168, 112, 0.0)');
  cxyCtx.fillStyle = grad;
  cxyCtx.beginPath();
  cxyCtx.arc(cx, cy, 20, 0, Math.PI * 2);
  cxyCtx.fill();

  // Crosshair
  cxyCtx.strokeStyle = lyricsAnimating ? 'rgba(107,142,78,0.5)' : 'rgba(139,163,112,0.35)';
  cxyCtx.lineWidth = 1;
  cxyCtx.beginPath(); cxyCtx.moveTo(cx, 0); cxyCtx.lineTo(cx, ch); cxyCtx.stroke();
  cxyCtx.beginPath(); cxyCtx.moveTo(0, cy); cxyCtx.lineTo(cw, cy); cxyCtx.stroke();

  // Dot
  cxyCtx.beginPath();
  cxyCtx.arc(cx, cy, 5, 0, Math.PI * 2);
  cxyCtx.fillStyle = lyricsAnimating ? '#6B8E4E' : '#8BA870';
  cxyCtx.fill();
  cxyCtx.strokeStyle = '#3C2F2F';
  cxyCtx.lineWidth = 1.5;
  cxyCtx.stroke();

  // Inner dot
  cxyCtx.beginPath();
  cxyCtx.arc(cx, cy, 1.5, 0, Math.PI * 2);
  cxyCtx.fillStyle = '#F5E6D3';
  cxyCtx.fill();

  // Frequency readout
  const freq = computePlaceFreq(normX);
  const freqText = freq >= 1000 ? (freq / 1000).toFixed(1) + 'k' : Math.round(freq) + '';
  // The number and the 'Hz' are a readout and stay (D-03). The MANNER is a
  // word, and the axis captions right above it — Fric / Plos — are
  // [data-i18n] elements, so leaving this English would put French on the
  // axis and English in the readout under it.
  let mannerKey = 'canvas.mixed';
  if (normY < 0.3) mannerKey = 'canvas.plosive';
  else if (normY > 0.7) mannerKey = 'canvas.fricative';
  const mannerText = trLabel(mannerKey, uiLanguage);
  cxyCtx.font = '8px Garamond, Times New Roman, serif';
  cxyCtx.fillStyle = 'rgba(60,47,47,0.5)';
  cxyCtx.textAlign = 'left';
  cxyCtx.textBaseline = 'top';
  cxyCtx.fillText(freqText + 'Hz ' + mannerText, 4, 2);
}

// ============================================================================
// Knobs
// ============================================================================
function bindKnobs() {
  const knobWraps = document.querySelectorAll('.knob-wrap[data-param]');

  knobWraps.forEach((wrap) => {
    const paramId = wrap.dataset.param;
    const knob = wrap.querySelector('.knob');
    if (!knob) return;

    knob.addEventListener('pointerdown', (e) => {
      const state = window._paramMap[paramId];
      if (!state) return;
      e.preventDefault();
      knob.setPointerCapture(e.pointerId);
      activeKnob = { paramId, state, knob };
      knobStartY = e.clientY;
      knobStartNorm = state.getNormalisedValue();
      state.sliderDragStarted();
    });
  });

  document.addEventListener('pointermove', (e) => {
    if (!activeKnob) return;
    const delta = (knobStartY - e.clientY) / 200;
    const newNorm = Math.max(0, Math.min(1, knobStartNorm + delta));
    activeKnob.state.setNormalisedValue(newNorm);
    updateKnobVisual(activeKnob.paramId, activeKnob.state);
  });

  document.addEventListener('pointerup', () => {
    if (!activeKnob) return;
    activeKnob.state.sliderDragEnded();
    activeKnob = null;
  });

  document.addEventListener('pointercancel', () => {
    if (!activeKnob) return;
    activeKnob.state.sliderDragEnded();
    activeKnob = null;
  });

  // Initial visuals
  setTimeout(() => {
    for (const [paramId, state] of Object.entries(window._paramMap)) {
      updateKnobVisual(paramId, state);
    }
  }, 100);
}

function updateKnobVisual(paramId, state) {
  const wrap = document.querySelector(`.knob-wrap[data-param="${paramId}"]`);
  if (!wrap) return;

  const norm = state.getNormalisedValue();
  const angle = -135 + norm * 270;
  const indicator = wrap.querySelector('.knob-indicator');
  if (indicator) {
    indicator.style.transform = `translateX(-50%) rotate(${angle}deg)`;
  }

  const valueEl = wrap.querySelector('.knob-value');
  if (valueEl) {
    const scaled = state.getScaledValue();
    const props = state.properties;
    let text;
    if (props.label) {
      text = formatValue(scaled) + ' ' + props.label;
    } else {
      text = formatValue(scaled);
    }
    valueEl.textContent = text;
  }
}

function formatValue(v) {
  if (Math.abs(v) >= 100) return Math.round(v).toString();
  if (Math.abs(v) >= 10) return v.toFixed(1);
  return v.toFixed(2);
}

// ============================================================================
// Hover-help renderer (v1.27.0)
// ============================================================================
//
// THE COPY ALONE IS INVISIBLE. applyI18n() above writes data-tip-title and
// data-tip onto the anchors named in TIP_BINDINGS and stops there. Nothing on
// this page read those attributes before v1.27.0 — there was no #tooltip node,
// no .tooltip rule and no hover handler — so authoring 57 bodies into i18n.js
// without this function would have shipped 57 unpaintable strings past three
// GREEN gates: check-i18n only counts bindings, check-ui-labels has no tooltip
// awareness at all, and boot-all-uis counts aria-label and title and never
// data-tip. tests/ui_tip_render_check.js is the gate that can see a rendered
// tip, and it exists for exactly that reason.
//
// Ported from plugins/O-simpleFM/Source/ui/public/js/app.js:384-462 and styled
// in this page's own parchment system. Every property below is load-bearing:
//
//  1. DELEGATED ON document, not querySelectorAll('[data-tip]') at setup. No
//     anchor carries data-tip until applyI18n() has run, so a setup-time query
//     binds NOTHING and fails silently.
//  2. pointerover / pointerout / focusin / focusout, because they BUBBLE.
//     pointerenter / focus do not, and delegation needs a bubbling event.
//  3. pointerout ignores a move between two descendants of the SAME anchor. A
//     .knob-wrap holds a dial, a caption and a readout; without this the tip
//     flickers off and on at every internal boundary.
//  4. createElement + textContent, NEVER innerHTML. Localized copy must not
//     reach a markup path — check-i18n assertion 9 already forbids an angle
//     bracket in an i18n.js string literal and this is the other half.
//  5. FLIP FIRST, THEN CLAMP, unconditionally, on all four edges. The clamp
//     runs AFTER the flip rather than instead of it, because a flipped tip can
//     still overflow the other way (O-Bass, 420x320). M2 finding 1 is the trap
//     here: the earlier ports wrote the re-clamp as `if (ny + r.height >
//     innerHeight - M)`, which after a flip collapses to `y - 12 > innerHeight
//     - M` and stops mentioning the tip's size at all. The Math.min/Math.max
//     pair below is what actually does the work, and the render gate drives it
//     directly rather than crediting the branch.
//  6. THE FOCUS ARM IS LATCHED TO THE KEYBOARD. A mouse click on a <button>
//     focuses it, so an unconditional focusin rule re-opens the tip that
//     pointerdown just hid and parks it over whatever the click opened —
//     measured on O-Emulator, where clicking the gear pinned the gear's own tip
//     across the settings popover. :focus-visible is deliberately NOT the
//     discriminator: Chromium reports it false for a programmatic .focus()
//     after a click, so a gate driving focus directly would measure "no tip"
//     and record that as correct.
//  7. Escape hides it; so does any pointerdown.
//
// NO SEPARATE DRAG GUARD IS NEEDED ON THIS PAGE, and that is a measurement
// rather than a default (M2 finding 2). All three drag surfaces call
// setPointerCapture(e.pointerId) on pointerdown — the knobs at bindKnobs(), the
// vowel pad and the consonant pad — so every boundary event is retargeted to
// the captured element for the duration of a drag and no neighbour's
// pointerover can arrive. The render gate drives a real cross-cell drag and
// asserts it.
function setupTooltips() {
  const tip = document.getElementById('tooltip');
  if (!tip) { console.warn('tooltip surface missing - hover-help unavailable'); return; }

  const MARGIN = 8;
  let active = null;
  let lastInputWasPointer = false;

  const position = (x, y) => {
    const r = tip.getBoundingClientRect();
    const vw = window.innerWidth;
    const vh = window.innerHeight;
    let nx = x + 14;
    let ny = y + 16;
    // Flip to the other side of the cursor when the natural side overflows.
    if (nx + r.width  > vw - MARGIN) nx = x - r.width  - 14;
    if (ny + r.height > vh - MARGIN) ny = y - r.height - 12;
    // Then clamp, unconditionally, on all four edges. Math.max on the upper
    // bound keeps the arithmetic sane for a tip taller than the frame: it lands
    // at MARGIN rather than at a negative coordinate.
    nx = Math.min(Math.max(MARGIN, nx), Math.max(MARGIN, vw - r.width  - MARGIN));
    ny = Math.min(Math.max(MARGIN, ny), Math.max(MARGIN, vh - r.height - MARGIN));
    tip.style.left = `${nx}px`;
    tip.style.top  = `${ny}px`;
  };

  const show = (el, x, y) => {
    const title = el.getAttribute('data-tip-title');
    const body  = el.getAttribute('data-tip');
    if (!title && !body) return;
    tip.textContent = '';
    if (title) {
      const t = document.createElement('span');
      t.className = 'tip-title';
      t.textContent = title;
      tip.appendChild(t);
    }
    if (body) tip.appendChild(document.createTextNode(body));
    tip.classList.add('show');
    tip.setAttribute('aria-hidden', 'false');
    position(x, y);
  };

  const hide = () => {
    tip.classList.remove('show');
    tip.setAttribute('aria-hidden', 'true');
    active = null;
  };

  const anchorOf = (t) => (t && t.closest ? t.closest('[data-tip]') : null);

  document.addEventListener('pointerover', (e) => {
    const el = anchorOf(e.target);
    if (!el || el === active) return;
    active = el;
    show(el, e.clientX, e.clientY);
  });
  document.addEventListener('pointermove', (e) => {
    if (active && anchorOf(e.target) === active) position(e.clientX, e.clientY);
  });
  document.addEventListener('pointerout', (e) => {
    if (!active) return;
    if (anchorOf(e.relatedTarget) === active) return;   // same anchor, child boundary
    hide();
  });
  // A knob or pad drag starts with a pointerdown on the anchor itself, so
  // hiding here also keeps the tip out of the way for the whole drag.
  document.addEventListener('pointerdown', () => { lastInputWasPointer = true; hide(); });

  document.addEventListener('focusin', (e) => {
    if (lastInputWasPointer) return;
    const el = anchorOf(e.target);
    if (!el) return;
    active = el;
    const r = el.getBoundingClientRect();
    show(el, r.left + r.width / 2, r.bottom);
  });
  document.addEventListener('focusout', hide);

  // One keydown listener, two jobs: any key at all means the keyboard is
  // driving again, which releases the latch above; Escape also hides.
  document.addEventListener('keydown', (e) => {
    lastInputWasPointer = false;
    if (e.key === 'Escape') hide();
  });
}

// ============================================================================
// Settings popover (v1.26.0)
// ============================================================================
function bindSettingsPopover() {
  const btn = document.getElementById('gear-btn');
  const pop = document.getElementById('settings-popover');
  if (!btn || !pop) return;

  const close = () => { pop.classList.remove('open'); btn.classList.remove('open'); };

  btn.addEventListener('click', (e) => {
    e.stopPropagation();
    const open = !pop.classList.contains('open');
    pop.classList.toggle('open', open);
    btn.classList.toggle('open', open);
  });

  // Click-away closes; a click INSIDE must not, or picking a language would
  // shut the panel out from under the pointer.
  pop.addEventListener('click', (e) => e.stopPropagation());
  document.addEventListener('click', close);
  document.addEventListener('keydown', (e) => { if (e.key === 'Escape') close(); });
}

// ============================================================================
// Canvas repaint on a language change
// ============================================================================
//
// THE PROBLEM THIS SOLVES. Two canvas strings on this page are prose and are
// read through trLabel() at PAINT time — the lyrics badge on the vowel pad and
// the manner word in the consonant pad's readout. Neither canvas repaints on
// its own: both are drawn from state changes, so after a language switch they
// keep painting the previous language until something unrelated moves.
//
// WHY AN OBSERVER RATHER THAN A LISTENER ON #lang-select. There are TWO paths
// into applyI18n and a change listener only covers one. The other is the
// one-shot getUiLanguage() pull that settles after page load and repaints the
// whole page in French with no event of its own. The canon's applyLabel writes
// `dataset.label` on every [data-i18n] element on every sweep, so observing
// that attribute on one always-present label catches BOTH paths — and any
// third one a future canon adds — without touching the canon block, which
// assertion 6 byte-compares.
function watchLanguageForCanvasRepaint() {
  const witness = document.getElementById('preset-save');
  if (!witness || typeof MutationObserver !== 'function') return;

  let last = null;
  const obs = new MutationObserver(() => {
    const now = witness.dataset.label;
    if (now === last) return;         // same language; nothing to repaint
    last = now;
    try {
      if (ctx) drawXYPad();
      if (cxyCtx) drawConsonantXYPad();
    } catch (e) { /* relays not up yet; the next state change repaints */ }
  });
  last = witness.dataset.label;
  obs.observe(witness, { attributes: true, attributeFilter: ['data-label'] });
}

// ============================================================================
// Toggle
// ============================================================================
function bindToggle() {
  const btn = document.getElementById('autoConsonant-toggle');
  if (!btn) return;

  btn.addEventListener('click', () => {
    autoConsonantState.setValue(!autoConsonantState.getValue());
  });

  updateToggleVisual();
}

function updateToggleVisual() {
  const btn = document.getElementById('autoConsonant-toggle');
  if (!btn) return;
  const isAuto = autoConsonantState.getValue();
  if (isAuto) {
    btn.classList.add('active');
  } else {
    btn.classList.remove('active');
  }
  // Dim manual envelope knobs when auto is on (timing derived from manner)
  const envControls = document.getElementById('consonant-env-controls');
  if (envControls) {
    if (isAuto) {
      envControls.classList.add('hidden');
    } else {
      envControls.classList.remove('hidden');
    }
  }
}

// ============================================================================
// Topology Selector
// ============================================================================
function bindTopologySelector() {
  const control = document.getElementById('topology-control');
  if (!control) return;

  control.addEventListener('click', (e) => {
    const btn = e.target.closest('.segmented-btn');
    if (!btn) return;
    const idx = parseInt(btn.dataset.index, 10);
    formantTopologyState.setChoiceIndex(idx);
    updateTopologyVisual();
  });

  updateTopologyVisual();
}

function updateTopologyVisual() {
  const control = document.getElementById('topology-control');
  if (!control) return;
  const idx = formantTopologyState.getChoiceIndex();
  const btns = control.querySelectorAll('.segmented-btn');
  btns.forEach((btn) => {
    if (parseInt(btn.dataset.index, 10) === idx) {
      btn.classList.add('active');
    } else {
      btn.classList.remove('active');
    }
  });
}

// ============================================================================
// Effects Controls
// ============================================================================
function bindEffectsControls() {
  // Bypass toggles
  setupFxBypass('chorusBypassBtn', chorusBypassState);
  setupFxBypass('delayBypassBtn', delayBypassState);
  setupFxBypass('reverbBypassBtn', reverbBypassState);
  setupFxBypass('eqBypassBtn', eqBypassState);

  // Delay mode dropdown
  const modeSelect = document.getElementById('delayModeSelect');
  if (modeSelect && delayModeState) {
    modeSelect.addEventListener('change', () => {
      delayModeState.setChoiceIndex(parseInt(modeSelect.value, 10));
    });
    delayModeState.valueChangedEvent.addListener(() => {
      modeSelect.value = delayModeState.getChoiceIndex().toString();
    });
    modeSelect.value = delayModeState.getChoiceIndex().toString();
  }
}

function setupFxBypass(btnId, toggleState) {
  const btn = document.getElementById(btnId);
  if (!btn || !toggleState) return;

  btn.addEventListener('click', () => {
    toggleState.setValue(!toggleState.getValue());
  });

  const update = () => {
    const bypassed = toggleState.getValue();
    // if/else, not a ternary INSIDE the setLabel argument: contract §6 forbids
    // inflection logic in a localized string and check-i18n assertion 13
    // rejects a conditional there. The element becomes a [data-i18n] element
    // from this call on, so the language sweep owns it from then on.
    if (bypassed) setLabel(btn, 'label.off');
    else          setLabel(btn, 'label.on');
    if (bypassed) {
      btn.classList.add('bypassed');
    } else {
      btn.classList.remove('bypassed');
    }
  };

  toggleState.valueChangedEvent.addListener(update);
  update();
}

// ============================================================================
// Preset Browser
// ============================================================================
let presetFns = {};

async function initPresetBrowser() {
  presetFns = {
    getPresetList: getNativeFunction('getPresetList'),
    getPresetListWithCategories: getNativeFunction('getPresetListWithCategories'),
    getCurrentPreset: getNativeFunction('getCurrentPreset'),
    loadPreset: getNativeFunction('loadPreset'),
    loadPresetFromCategory: getNativeFunction('loadPresetFromCategory'),
    savePreset: getNativeFunction('savePreset'),
    selectNextPreset: getNativeFunction('selectNextPreset'),
    selectPreviousPreset: getNativeFunction('selectPreviousPreset'),
    deletePreset: getNativeFunction('deletePreset'),
    isFactoryPreset: getNativeFunction('isFactoryPreset'),
  };

  const nameEl = document.getElementById('preset-name');
  const prevBtn = document.getElementById('preset-prev');
  const nextBtn = document.getElementById('preset-next');
  const saveBtn = document.getElementById('preset-save');
  const categorySelect = document.getElementById('preset-category');

  // Load current preset name
  const current = await presetFns.getCurrentPreset();
  if (current && nameEl) nameEl.textContent = current;

  // Populate category dropdown
  await populateCategories(categorySelect);

  // Prev / Next
  prevBtn.addEventListener('click', async () => {
    const name = await presetFns.selectPreviousPreset();
    if (name && nameEl) nameEl.textContent = name;
  });

  nextBtn.addEventListener('click', async () => {
    const name = await presetFns.selectNextPreset();
    if (name && nameEl) nameEl.textContent = name;
  });

  // Save
  saveBtn.addEventListener('click', async () => {
    const name = prompt(trLabel('js.savePresetAs', uiLanguage));
    if (!name || !name.trim()) return;
    const success = await presetFns.savePreset(name.trim());
    if (success && nameEl) {
      nameEl.textContent = name.trim();
      await populateCategories(categorySelect);
    }
  });

  // Category filter
  categorySelect.addEventListener('change', async () => {
    const cat = categorySelect.value;
    if (cat === 'all') return;
    const categories = await presetFns.getPresetListWithCategories();
    const presets = categories[cat];
    if (presets && presets.length > 0) {
      const success = await presetFns.loadPresetFromCategory(cat, presets[0]);
      if (success && nameEl) nameEl.textContent = presets[0];
    }
  });
}

async function populateCategories(selectEl) {
  if (!selectEl) return;
  const categories = await presetFns.getPresetListWithCategories();
  // The VALUE is the sentinel every comparison uses ("all"); only the text is
  // localized. setLabel() after the injection because this option is created
  // long after initI18n's sweep ran.
  selectEl.innerHTML = '<option value="all" data-i18n="label.allCategories">All</option>';
  setLabel(selectEl.firstElementChild, 'label.allCategories');
  for (const cat of Object.keys(categories).sort()) {
    const opt = document.createElement('option');
    opt.value = cat;
    opt.textContent = cat;
    selectEl.appendChild(opt);
  }
}

// ═══════════════════════════════════════════════════════════════════
//  LYRICS ENGINE
// ═══════════════════════════════════════════════════════════════════

// ARPABET vowel nuclei (used for syllabification)
const ARPABET_VOWELS = new Set([
  'AA','AE','AH','AO','AW','AY','EH','ER','EY','IH','IY','OW','OY','UH','UW'
]);

// Phoneme → parameter mapping
const PHONEME_MAP = {
  // Vowels → vowelX, vowelY (from research brief + VowelData.h positions)
  'IY':  { vowelX: 0.00, vowelY: 1.00 },
  'IH':  { vowelX: 0.10, vowelY: 0.80 },
  'EY':  { vowelX: 0.31, vowelY: 0.43 },
  'EH':  { vowelX: 0.25, vowelY: 0.30 },
  'AE':  { vowelX: 0.50, vowelY: 0.05 },
  'AA':  { vowelX: 0.83, vowelY: 0.00 },
  'AH':  { vowelX: 0.60, vowelY: 0.15 },
  'AO':  { vowelX: 0.92, vowelY: 0.20 },
  'OW':  { vowelX: 1.00, vowelY: 0.35 },
  'UH':  { vowelX: 0.85, vowelY: 0.78 },
  'UW':  { vowelX: 0.98, vowelY: 0.93 },
  'ER':  { vowelX: 0.55, vowelY: 0.50 },
  'AW':  { vowelX: 0.55, vowelY: 0.08 },
  'AY':  { vowelX: 0.65, vowelY: 0.10 },
  'OY':  { vowelX: 0.90, vowelY: 0.28 },

  // Consonants → place, manner, voicing, level, nasal
  'P':  { consonantTone: 0.00, sibilance: 0.00, consonantVoicing: 0.0, consonantLevel: 0.6 },
  'B':  { consonantTone: 0.00, sibilance: 0.00, consonantVoicing: 1.0, consonantLevel: 0.6 },
  'M':  { nasalCoupling: 1.0, nasalPlace: 0.0, consonantLevel: 0.0 },
  'F':  { consonantTone: 0.08, sibilance: 1.00, consonantVoicing: 0.0, consonantLevel: 0.5 },
  'V':  { consonantTone: 0.08, sibilance: 1.00, consonantVoicing: 1.0, consonantLevel: 0.5 },
  'W':  { vowelX: 0.98, vowelY: 0.93, consonantLevel: 0.0 },  // Glide from U-region
  'TH': { consonantTone: 0.15, sibilance: 1.00, consonantVoicing: 0.0, consonantLevel: 0.4 },
  'DH': { consonantTone: 0.15, sibilance: 1.00, consonantVoicing: 1.0, consonantLevel: 0.4 },
  'T':  { consonantTone: 0.33, sibilance: 0.00, consonantVoicing: 0.0, consonantLevel: 0.6 },
  'D':  { consonantTone: 0.33, sibilance: 0.00, consonantVoicing: 1.0, consonantLevel: 0.6 },
  'N':  { nasalCoupling: 1.0, nasalPlace: 0.5, consonantLevel: 0.0 },
  'S':  { consonantTone: 0.33, sibilance: 1.00, consonantVoicing: 0.0, consonantLevel: 0.5 },
  'Z':  { consonantTone: 0.33, sibilance: 1.00, consonantVoicing: 1.0, consonantLevel: 0.5 },
  'L':  { vowelX: 0.55, vowelY: 0.85, consonantLevel: 0.0 },  // Existing morph point
  'R':  { vowelX: 0.12, vowelY: 0.72, consonantLevel: 0.0 },  // Existing morph point
  'CH': { consonantTone: 0.55, sibilance: 0.15, consonantVoicing: 0.0, consonantLevel: 0.6 },
  'JH': { consonantTone: 0.55, sibilance: 0.15, consonantVoicing: 1.0, consonantLevel: 0.6 },
  'SH': { consonantTone: 0.55, sibilance: 1.00, consonantVoicing: 0.0, consonantLevel: 0.5 },
  'ZH': { consonantTone: 0.55, sibilance: 1.00, consonantVoicing: 1.0, consonantLevel: 0.5 },
  'Y':  { vowelX: 0.00, vowelY: 1.00, consonantLevel: 0.0 },  // Glide from I-region
  'K':  { consonantTone: 1.00, sibilance: 0.00, consonantVoicing: 0.0, consonantLevel: 0.6 },
  'G':  { consonantTone: 1.00, sibilance: 0.00, consonantVoicing: 1.0, consonantLevel: 0.6 },
  'NG': { nasalCoupling: 1.0, nasalPlace: 1.0, consonantLevel: 0.0 },
  'HH': { consonantTone: 0.50, sibilance: 0.85, consonantVoicing: 0.0, consonantLevel: 0.3 },
};

// Legal 2-consonant onsets for MOP syllabification
const LEGAL_ONSETS_2 = new Set([
  'P L','P R','B L','B R','T R','D R','K L','K R','G L','G R',
  'F L','F R','TH R','SH R','S K','S P','S T','S M','S N','S L','S W'
]);
const LEGAL_ONSETS_3 = new Set(['S P L','S P R','S T R','S K R','S K W']);

// Syllabify ARPABET phoneme array using Maximum Onset Principle
function syllabify(phonemes) {
  if (phonemes.length === 0) return [];

  // Find vowel positions
  const vowelIndices = [];
  for (let i = 0; i < phonemes.length; i++) {
    if (ARPABET_VOWELS.has(phonemes[i])) vowelIndices.push(i);
  }

  if (vowelIndices.length === 0) {
    // No vowels — treat entire sequence as one syllable
    return [phonemes];
  }

  const syllables = [];
  let syllStart = 0;

  for (let vi = 0; vi < vowelIndices.length; vi++) {
    const vowelIdx = vowelIndices[vi];
    const nextVowelIdx = vi + 1 < vowelIndices.length ? vowelIndices[vi + 1] : phonemes.length;

    if (vi + 1 < vowelIndices.length) {
      // Consonants between this vowel and next vowel
      const gapStart = vowelIdx + 1;
      const gapEnd = nextVowelIdx;
      const gap = phonemes.slice(gapStart, gapEnd);

      // Find maximum legal onset for the next syllable
      let onsetLen = 0;
      for (let tryLen = Math.min(gap.length, 3); tryLen >= 1; tryLen--) {
        const candidate = gap.slice(gap.length - tryLen);
        const key = candidate.join(' ');
        if (tryLen === 1) {
          // Single consonants are always legal onsets (except NG)
          if (candidate[0] !== 'NG') { onsetLen = 1; break; }
        } else if (tryLen === 2 && LEGAL_ONSETS_2.has(key)) {
          onsetLen = 2; break;
        } else if (tryLen === 3 && LEGAL_ONSETS_3.has(key)) {
          onsetLen = 3; break;
        }
      }

      const splitPoint = gapEnd - onsetLen;
      syllables.push(phonemes.slice(syllStart, splitPoint));
      syllStart = splitPoint;
    } else {
      // Last vowel — take everything remaining
      syllables.push(phonemes.slice(syllStart));
    }
  }

  return syllables.filter(s => s.length > 0);
}

// Convert a syllable (array of ARPABET phonemes) to a SyllableTarget
function syllableToTarget(phonemes) {
  const target = {
    vowelX: 0.5, vowelY: 0.5,
    consonantTone: 0.5, sibilance: 0.5,
    consonantVoicing: 0.5, consonantLevel: 0.0,
    nasalCoupling: 0.0, nasalPlace: 0.5,
    hasConsonant: false
  };

  // Find the vowel nucleus
  let vowelFound = false;
  for (const ph of phonemes) {
    if (ARPABET_VOWELS.has(ph) && PHONEME_MAP[ph]) {
      target.vowelX = PHONEME_MAP[ph].vowelX;
      target.vowelY = PHONEME_MAP[ph].vowelY;
      vowelFound = true;
      break;
    }
  }

  // Find the first onset consonant (before the vowel)
  for (const ph of phonemes) {
    if (ARPABET_VOWELS.has(ph)) break;  // Stop at vowel
    const map = PHONEME_MAP[ph];
    if (!map) continue;

    // Nasal consonants
    if (map.nasalCoupling !== undefined && map.nasalCoupling > 0) {
      target.nasalCoupling = map.nasalCoupling;
      target.nasalPlace = map.nasalPlace;
      target.hasConsonant = true;
      break;
    }
    // Approximants (L, R, W, Y) — override vowel position for coloring
    if (map.vowelX !== undefined && (map.consonantLevel === undefined || map.consonantLevel === 0)) {
      target.vowelX = map.vowelX;
      target.vowelY = map.vowelY;
      continue;  // Don't count as consonant onset
    }
    // Plosive/fricative consonants
    if (map.consonantLevel !== undefined && map.consonantLevel > 0) {
      target.consonantTone = map.consonantTone;
      target.sibilance = map.sibilance;
      target.consonantVoicing = map.consonantVoicing;
      target.consonantLevel = map.consonantLevel;
      target.hasConsonant = true;
      break;
    }
  }

  // Check coda for nasals (e.g., NG at end of "SING")
  if (!target.hasConsonant || target.nasalCoupling === 0) {
    for (let i = phonemes.length - 1; i >= 0; i--) {
      if (ARPABET_VOWELS.has(phonemes[i])) break;
      const map = PHONEME_MAP[phonemes[i]];
      if (map && map.nasalCoupling > 0) {
        target.nasalCoupling = map.nasalCoupling * 0.5;  // Coda nasals softer
        target.nasalPlace = map.nasalPlace;
        break;
      }
    }
  }

  return target;
}

// Parse raw ARPABET text → syllable targets
function parseArpabet(text) {
  const cleaned = text.trim().toUpperCase().replace(/[^A-Z0-9\s]/g, '');
  if (!cleaned) return [];

  const tokens = cleaned.split(/\s+/);

  // Merge digraphs: CH, SH, ZH, TH, DH, JH, HH, NG, etc.
  const phonemes = [];
  for (let i = 0; i < tokens.length; i++) {
    const pair = tokens[i] + (i + 1 < tokens.length ? tokens[i + 1] : '');
    if (['CH','SH','ZH','TH','DH','JH','HH','NG','AW','AY','EY','OW','OY',
         'AA','AE','AH','AO','EH','ER','IH','IY','UH','UW'].includes(pair)
        && pair.length === 2 && tokens[i].length === 1) {
      phonemes.push(pair);
      i++;  // Skip next token
    } else if (PHONEME_MAP[tokens[i]] || ARPABET_VOWELS.has(tokens[i])) {
      phonemes.push(tokens[i]);
    }
    // Skip unrecognized tokens
  }

  const syllables = syllabify(phonemes);
  return syllables.map(syl => ({
    phonemes: syl,
    label: syl.join(' '),
    target: syllableToTarget(syl)
  }));
}

let lyricsEnabledState;
let lyricsFns = {};
let lyricsPollingId = null;

function initLyricsTab() {
  const input = document.getElementById('lyrics-input');
  const syllablesEl = document.getElementById('lyrics-syllables');
  const counterEl = document.getElementById('lyrics-counter');
  const loopBtn = document.getElementById('lyrics-loop-btn');
  const resetBtn = document.getElementById('lyrics-reset-btn');

  if (!input) return;

  // Get native functions
  lyricsFns.setLyrics = getNativeFunction('setLyrics');
  lyricsFns.setLyricsText = getNativeFunction('setLyricsText');
  lyricsFns.getLyricsText = getNativeFunction('getLyricsText');
  lyricsFns.getLyricsPosition = getNativeFunction('getLyricsPosition');
  lyricsFns.resetLyrics = getNativeFunction('resetLyrics');
  lyricsFns.setLyricsLooping = getNativeFunction('setLyricsLooping');
  lyricsFns.getLyricsLooping = getNativeFunction('getLyricsLooping');

  // Lyrics enabled toggle
  lyricsEnabledState = getToggleState('lyricsEnabledToggle');
  const toggleBtn = document.getElementById('lyricsEnabled-toggle');
  if (toggleBtn && lyricsEnabledState) {
    const updateToggleUI = () => {
      const isOn = lyricsEnabledState.getValue();
      toggleBtn.classList.toggle('active', isOn);
    };
    lyricsEnabledState.valueChangedEvent.addListener(() => {
      updateToggleUI();
      updateLyricsAnimatingState();
    });
    updateToggleUI();
    toggleBtn.addEventListener('click', () => {
      lyricsEnabledState.setValue(!lyricsEnabledState.getValue());
    });
  }

  let parsedSyllables = [];

  // Parse and send on input change
  let debounceTimer = null;
  input.addEventListener('input', () => {
    clearTimeout(debounceTimer);
    debounceTimer = setTimeout(() => {
      parsedSyllables = parseArpabet(input.value);
      renderSyllables(parsedSyllables, syllablesEl, counterEl, -1);
      sendToEngine(parsedSyllables, input.value);
    }, 300);
  });

  // Loop button
  let looping = true;
  loopBtn.classList.add('active');
  loopBtn.addEventListener('click', async () => {
    looping = !looping;
    loopBtn.classList.toggle('active', looping);
    await lyricsFns.setLyricsLooping(looping);
  });

  // Reset button
  resetBtn.addEventListener('click', async () => {
    await lyricsFns.resetLyrics();
  });

  // Restore saved lyrics text on load
  (async () => {
    try {
      const savedText = await lyricsFns.getLyricsText();
      if (savedText && savedText.length > 0) {
        input.value = savedText;
        parsedSyllables = parseArpabet(savedText);
        renderSyllables(parsedSyllables, syllablesEl, counterEl, -1);
        sendToEngine(parsedSyllables, savedText);
      }
      const savedLoop = await lyricsFns.getLyricsLooping();
      looping = !!savedLoop;
      loopBtn.classList.toggle('active', looping);
    } catch (e) { /* ignore on first load */ }
  })();

  // Position polling (50ms when lyrics tab is visible)
  startPositionPolling(parsedSyllables, syllablesEl, counterEl);
}

async function sendToEngine(parsedSyllables, rawText) {
  if (!lyricsFns.setLyrics) return;
  const targets = parsedSyllables.map(s => s.target);
  const json = JSON.stringify(targets);
  await lyricsFns.setLyrics(json);
  await lyricsFns.setLyricsText(rawText);
}

function renderSyllables(parsed, container, counterEl, currentIdx) {
  if (!container) return;
  container.innerHTML = '';
  parsed.forEach((syl, i) => {
    const chip = document.createElement('div');
    chip.className = 'syl-chip';
    if (i === currentIdx) chip.classList.add('current');
    else if (currentIdx >= 0 && i < currentIdx) chip.classList.add('past');
    chip.textContent = syl.label;
    // v1.26.0: the native title= is DELETED per contract §4 — it rendered a
    // second, untranslated OS tooltip on every chip.
    //
    // It is deleted rather than MOVED to data-i18n-aria, and that is the
    // narrow reading of §4 rather than a shortcut. §4 moves a title's text
    // when the title is the ONLY help an element has. Here it was
    // "Syllable 3: HH-AH-L" over a chip whose own text already reads
    // "HH AH L", beside a #lyrics-counter that already reads "3 / 7" — so the
    // title carried nothing the page was not already saying, and an
    // aria-label would OVERRIDE the phonemes a screen reader otherwise reads
    // straight off the chip. No new prose is invented, and none is lost.
    container.appendChild(chip);
  });
  if (counterEl) {
    const idx = currentIdx >= 0 ? currentIdx + 1 : 0;
    counterEl.textContent = `${idx} / ${parsed.length}`;
  }
}

function updateLyricsAnimatingState() {
  const wasAnimating = lyricsAnimating;
  const enabled = lyricsEnabledState && lyricsEnabledState.getValue();
  lyricsAnimating = !!(enabled && lyricsTarget);

  // Update cursor style on pads
  if (canvas) canvas.style.cursor = lyricsAnimating ? 'default' : 'crosshair';
  if (cxyCanvas) cxyCanvas.style.cursor = lyricsAnimating ? 'default' : 'crosshair';

  // When transitioning out of lyrics mode, clear target and redraw pads at APVTS values
  if (wasAnimating && !lyricsAnimating) {
    lyricsTarget = null;
    drawXYPad();
    drawConsonantXYPad();
  }
}

function startPositionPolling(initialParsed, container, counterEl) {
  let lastIdx = -1;
  let cachedParsed = initialParsed;

  const input = document.getElementById('lyrics-input');

  setInterval(async () => {
    if (!lyricsFns.getLyricsPosition) return;

    const enabled = lyricsEnabledState && lyricsEnabledState.getValue();
    const lyricsTab = document.getElementById('lyrics-tab');
    const synthTab = document.getElementById('synth-tab');
    const lyricsTabActive = lyricsTab && lyricsTab.classList.contains('active');
    const synthTabActive = synthTab && synthTab.classList.contains('active');

    // Nothing to poll if neither tab is active or lyrics disabled
    if (!enabled || (!lyricsTabActive && !synthTabActive)) {
      if (lyricsAnimating) {
        lyricsAnimating = false;
        lyricsTarget = null;
        if (synthTabActive) { drawXYPad(); drawConsonantXYPad(); }
      }
      return;
    }

    try {
      const pos = await lyricsFns.getLyricsPosition();
      if (!pos || pos.total === 0) {
        if (lyricsAnimating) {
          lyricsAnimating = false;
          lyricsTarget = null;
          if (synthTabActive) { drawXYPad(); drawConsonantXYPad(); }
        }
        return;
      }

      // Update XY pad animation target
      lyricsTarget = {
        vowelX: pos.vowelX,
        vowelY: pos.vowelY,
        consonantTone: pos.consonantTone,
        sibilance: pos.sibilance
      };
      lyricsAnimating = true;

      // Redraw XY pads on synth tab
      if (synthTabActive) {
        drawXYPad();
        drawConsonantXYPad();
      }

      // Update syllable chips on lyrics tab
      const idx = pos.index;
      if (lyricsTabActive && idx !== lastIdx) {
        lastIdx = idx;
        if (input) cachedParsed = parseArpabet(input.value);
        renderSyllables(cachedParsed, container, counterEl, idx);
      }
    } catch (e) { /* ignore polling errors */ }
  }, 80);
}
