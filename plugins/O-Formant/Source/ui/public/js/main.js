import { getSliderState, getToggleState, getComboBoxState, getNativeFunction } from './juce/index.js';

// Relay states
let vowelXState, vowelYState, vowelFocusState;
let glottalRdState, breathinessState, vibratoRateState, vibratoDepthState, vibratoDelayState, jitterState, shimmerState, rdModDepthState, spectralTiltState;
let consonantLevelState, consonantToneState, sibilanceState, autoConsonantState;
let consonantAttackState, consonantHoldState, consonantDecayState;
let attackState, decayState, sustainState, releaseState;
let formantTopologyState;
let formantShiftState, formantSpreadState, pitchGlideState, transitionTimeState, singersFormantState;
let outputGainState, stereoWidthState;

// Vowel XY pad
const canvas = document.getElementById('xy-pad');
const ctx = canvas ? canvas.getContext('2d') : null;
let isDraggingXY = false;
let dpr = 1;

// Consonant XY pad
let cxyCanvas, cxyCtx, cxyDpr;
let isDraggingCXY = false;

// IPA vowel positions (normalised X, Y where Y=1 is top)
const vowelLabels = [
  { label: 'i',  x: 0.00, y: 1.00 },
  { label: 'e',  x: 0.31, y: 0.43 },
  { label: '\u0251', x: 0.83, y: 0.00 },
  { label: 'o',  x: 1.00, y: 0.35 },
  { label: 'u',  x: 0.98, y: 0.93 },
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
  initRelays();
  setupCanvas();
  setupConsonantXYCanvas();
  setupADSRCanvas();
  bindXYPad();
  bindConsonantXYPad();
  bindKnobs();
  bindToggle();
  bindTopologySelector();
  drawADSR();
  initPresetBrowser();
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
  autoConsonantState = getToggleState('autoConsonantToggle');
  consonantAttackState = getSliderState('consonantAttackSlider');
  consonantHoldState = getSliderState('consonantHoldSlider');
  consonantDecayState = getSliderState('consonantDecaySlider');
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
  outputGainState = getSliderState('outputGainSlider');
  stereoWidthState = getSliderState('stereoWidthSlider');

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
    consonantAttack: consonantAttackState,
    consonantHold: consonantHoldState,
    consonantDecay: consonantDecayState,
    formantShift: formantShiftState,
    formantSpread: formantSpreadState,
    pitchGlide: pitchGlideState,
    transitionTime: transitionTimeState,
    singersFormant: singersFormantState,
    vowelFocus: vowelFocusState,
    attack: attackState,
    decay: decayState,
    sustain: sustainState,
    release: releaseState,
    outputGain: outputGainState,
    stereoWidth: stereoWidthState,
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

  // Cursor
  const normX = vowelXState.getNormalisedValue();
  const normY = vowelYState.getNormalisedValue();
  const cx = pad + normX * (cw - pad * 2);
  const cy = pad + (1.0 - normY) * (ch - pad * 2);

  // Cursor glow
  const grad = ctx.createRadialGradient(cx, cy, 0, cx, cy, 28);
  grad.addColorStop(0, 'rgba(139, 168, 112, 0.3)');
  grad.addColorStop(1, 'rgba(139, 168, 112, 0.0)');
  ctx.fillStyle = grad;
  ctx.beginPath();
  ctx.arc(cx, cy, 28, 0, Math.PI * 2);
  ctx.fill();

  // Crosshair
  ctx.strokeStyle = 'rgba(139,163,112,0.4)';
  ctx.lineWidth = 1;
  ctx.beginPath(); ctx.moveTo(cx, 0); ctx.lineTo(cx, ch); ctx.stroke();
  ctx.beginPath(); ctx.moveTo(0, cy); ctx.lineTo(cw, cy); ctx.stroke();

  // Dot
  ctx.beginPath();
  ctx.arc(cx, cy, 6, 0, Math.PI * 2);
  ctx.fillStyle = '#8BA870';
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

  // Cursor
  const normX = consonantToneState.getNormalisedValue();
  const normY = sibilanceState.getNormalisedValue();
  const cx = pad + normX * (cw - pad * 2);
  const cy = pad + (1.0 - normY) * (ch - pad * 2);

  // Cursor glow
  const grad = cxyCtx.createRadialGradient(cx, cy, 0, cx, cy, 20);
  grad.addColorStop(0, 'rgba(139, 168, 112, 0.3)');
  grad.addColorStop(1, 'rgba(139, 168, 112, 0.0)');
  cxyCtx.fillStyle = grad;
  cxyCtx.beginPath();
  cxyCtx.arc(cx, cy, 20, 0, Math.PI * 2);
  cxyCtx.fill();

  // Crosshair
  cxyCtx.strokeStyle = 'rgba(139,163,112,0.35)';
  cxyCtx.lineWidth = 1;
  cxyCtx.beginPath(); cxyCtx.moveTo(cx, 0); cxyCtx.lineTo(cx, ch); cxyCtx.stroke();
  cxyCtx.beginPath(); cxyCtx.moveTo(0, cy); cxyCtx.lineTo(cw, cy); cxyCtx.stroke();

  // Dot
  cxyCtx.beginPath();
  cxyCtx.arc(cx, cy, 5, 0, Math.PI * 2);
  cxyCtx.fillStyle = '#8BA870';
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
  const mannerText = normY < 0.3 ? 'plosive' : normY > 0.7 ? 'fricative' : 'mixed';
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
    const name = prompt('Save preset as:');
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
  selectEl.innerHTML = '<option value="all">All</option>';
  for (const cat of Object.keys(categories).sort()) {
    const opt = document.createElement('option');
    opt.value = cat;
    opt.textContent = cat;
    selectEl.appendChild(opt);
  }
}
