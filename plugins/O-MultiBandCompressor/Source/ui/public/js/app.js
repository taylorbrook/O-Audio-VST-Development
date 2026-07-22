/*
  ==============================================================================

    O-MultiBandCompressor - UI Application Logic
    Phase 5.2: Full parameter binding implementation (56 parameters)

    Fixed: Import JUCE module and use correct API

  ==============================================================================
*/

import * as Juce from './juce/index.js';

console.log('O-MultiBandCompressor UI initializing...');

// Parameter binding state
const parameterStates = {};

// Initialize immediately (JUCE module handles backend connection internally)
initializeUI();

function initializeUI() {
    console.log('Initializing parameter bindings');

    // Initialize all parameter bindings
    bindGlobalParameters();
    bindBandParameters('LOW', 'low');
    bindBandParameters('LOMID', 'lomid');
    bindBandParameters('HIMID', 'himid');
    bindBandParameters('HIGH', 'high');

    // Initialize spectrum placeholder
    initializeSpectrumPlaceholder();

    // Note: tooltips are initialized at the foot of this file, not here.
    // initializeUI() runs at module top level, above the `let`/`const` tooltip
    // state, which would still be in the temporal dead zone at this point.

    console.log('UI initialized - 56 parameters bound');
}

// ========== GLOBAL PARAMETERS (8) ==========

function bindGlobalParameters() {
    // Input Gain: -24 to +24 dB
    bindSlider('input-gain', 'INPUT_GAIN', (norm) => {
        const db = norm * 48.0 - 24.0;
        return db.toFixed(1) + ' dB';
    });

    // Output Gain: -24 to +24 dB
    bindSlider('output-gain', 'OUTPUT_GAIN', (norm) => {
        const db = norm * 48.0 - 24.0;
        return db.toFixed(1) + ' dB';
    });

    // Mix: 0-100%
    bindSlider('mix', 'MIX', (norm) => {
        const pct = norm * 100.0;
        return pct.toFixed(0) + '%';
    });

    // Auto-Makeup: bool
    bindToggle('auto-makeup', 'AUTO_MAKEUP');

    // M/S Mode: choice
    bindComboBox('ms-mode', 'MS_MODE');

    // Crossover frequencies (bind for APVTS sync even if not visible)
    try {
        parameterStates['XOVER1'] = Juce.getSliderState('XOVER1');
        parameterStates['XOVER2'] = Juce.getSliderState('XOVER2');
        parameterStates['XOVER3'] = Juce.getSliderState('XOVER3');
    } catch (e) {
        console.warn('Could not bind crossover parameters:', e);
    }
}

// ========== PER-BAND PARAMETERS (12 × 4 = 48) ==========

function bindBandParameters(bandPrefix, bandId) {
    // Threshold: -60 to 0 dB
    bindSlider(`${bandId}-threshold`, `${bandPrefix}_THRESHOLD`, (norm) => {
        const db = norm * 60.0 - 60.0;
        return db.toFixed(1) + ' dB';
    });

    // Ratio: 1:1 to 20:1
    bindSlider(`${bandId}-ratio`, `${bandPrefix}_RATIO`, (norm) => {
        const ratio = 1.0 + norm * 19.0;
        return ratio.toFixed(1) + ':1';
    });

    // Attack: 0.1 to 200 ms (WR-04: APVTS skew 0.3 → value = min + (max−min)·norm^(1/skew))
    bindSlider(`${bandId}-attack`, `${bandPrefix}_ATTACK`, (norm) => {
        const ms = 0.1 + (200 - 0.1) * Math.pow(norm, 1 / 0.3);
        return ms.toFixed(1) + ' ms';
    });

    // Release: 10 to 2000 ms (WR-04: APVTS skew 0.3 → value = min + (max−min)·norm^(1/skew))
    bindSlider(`${bandId}-release`, `${bandPrefix}_RELEASE`, (norm) => {
        const ms = 10 + (2000 - 10) * Math.pow(norm, 1 / 0.3);
        return ms.toFixed(0) + ' ms';
    });

    // Knee: 0 to 24 dB
    bindSlider(`${bandId}-knee`, `${bandPrefix}_KNEE`, (norm) => {
        const db = norm * 24.0;
        return db.toFixed(1) + ' dB';
    });

    // Makeup: -12 to +24 dB
    bindSlider(`${bandId}-makeup`, `${bandPrefix}_MAKEUP`, (norm) => {
        const db = norm * 36.0 - 12.0;
        return db.toFixed(1) + ' dB';
    });

    // Peak/RMS: bind for APVTS sync
    try {
        parameterStates[`${bandPrefix}_PEAK_RMS`] = Juce.getSliderState(`${bandPrefix}_PEAK_RMS`);
    } catch (e) {
        console.warn(`Could not bind ${bandPrefix}_PEAK_RMS:`, e);
    }

    // Solo, Bypass, SC Listen: bool
    bindToggle(`${bandId}-solo`, `${bandPrefix}_SOLO`);
    bindToggle(`${bandId}-bypass`, `${bandPrefix}_BYPASS`);
    bindToggle(`${bandId}-sc-listen`, `${bandPrefix}_SC_LISTEN`);

    // SC HPF/LPF: bind for APVTS sync
    try {
        parameterStates[`${bandPrefix}_SC_HPF`] = Juce.getSliderState(`${bandPrefix}_SC_HPF`);
        parameterStates[`${bandPrefix}_SC_LPF`] = Juce.getSliderState(`${bandPrefix}_SC_LPF`);
    } catch (e) {
        console.warn(`Could not bind ${bandPrefix} sidechain filters:`, e);
    }
}

// ========== BINDING HELPERS ==========

function bindSlider(elementId, parameterId, formatValue) {
    const element = document.getElementById(elementId);
    const valueDisplay = document.getElementById(`${elementId}-value`);

    if (!element) {
        console.warn(`Slider element not found: ${elementId}`);
        return;
    }

    try {
        // Get slider state from JUCE module
        const sliderState = Juce.getSliderState(parameterId);
        parameterStates[parameterId] = sliderState;

        // Initialize element with current value
        const initialNorm = sliderState.getNormalisedValue();
        element.value = initialNorm;
        if (valueDisplay && formatValue) {
            valueDisplay.textContent = formatValue(initialNorm);
        }

        // Update parameter when UI changes
        element.addEventListener('input', (e) => {
            const norm = parseFloat(e.target.value);
            sliderState.setNormalisedValue(norm);

            if (valueDisplay && formatValue) {
                valueDisplay.textContent = formatValue(norm);
            }
        });

        // Notify JUCE when drag starts/ends
        element.addEventListener('mousedown', () => {
            sliderState.sliderDragStarted();
        });

        element.addEventListener('mouseup', () => {
            sliderState.sliderDragEnded();
        });

        // Update UI when parameter changes (automation, preset load)
        sliderState.valueChangedEvent.addListener(() => {
            const norm = sliderState.getNormalisedValue();
            element.value = norm;

            if (valueDisplay && formatValue) {
                valueDisplay.textContent = formatValue(norm);
            }
        });

        console.log(`Bound slider: ${parameterId} → #${elementId}`);
    } catch (e) {
        console.error(`Failed to bind slider ${parameterId}:`, e);
    }
}

function bindToggle(elementId, parameterId) {
    const element = document.getElementById(elementId);

    if (!element) {
        console.warn(`Toggle element not found: ${elementId}`);
        return;
    }

    try {
        // Get toggle state from JUCE module
        const toggleState = Juce.getToggleState(parameterId);
        parameterStates[parameterId] = toggleState;

        // Initialize element with current state
        const initialValue = toggleState.getValue();
        updateToggleUI(element, initialValue);

        // Update parameter when button clicked
        element.addEventListener('click', () => {
            const newValue = !toggleState.getValue();
            toggleState.setValue(newValue);
            updateToggleUI(element, newValue);
        });

        // Update UI when parameter changes (automation, preset load)
        toggleState.valueChangedEvent.addListener(() => {
            const value = toggleState.getValue();
            updateToggleUI(element, value);
        });

        console.log(`Bound toggle: ${parameterId} → #${elementId}`);
    } catch (e) {
        console.error(`Failed to bind toggle ${parameterId}:`, e);
    }
}

function updateToggleUI(element, value) {
    if (value) {
        element.classList.add('active');
        element.textContent = 'On';
    } else {
        element.classList.remove('active');
        element.textContent = 'Off';
    }
}

function bindComboBox(elementId, parameterId) {
    const element = document.getElementById(elementId);

    if (!element) {
        console.warn(`ComboBox element not found: ${elementId}`);
        return;
    }

    try {
        // Get combobox state from JUCE module
        const comboState = Juce.getComboBoxState(parameterId);
        parameterStates[parameterId] = comboState;

        // Initialize element with current value
        const initialIndex = comboState.getChoiceIndex();
        element.selectedIndex = initialIndex;

        // Update parameter when selection changes
        element.addEventListener('change', (e) => {
            const selectedIndex = e.target.selectedIndex;
            comboState.setChoiceIndex(selectedIndex);
        });

        // Update UI when parameter changes (automation, preset load)
        comboState.valueChangedEvent.addListener(() => {
            const selectedIndex = comboState.getChoiceIndex();
            element.selectedIndex = selectedIndex;
        });

        console.log(`Bound combobox: ${parameterId} → #${elementId}`);
    } catch (e) {
        console.error(`Failed to bind combobox ${parameterId}:`, e);
    }
}

// ========== SPECTRUM ANALYZER PLACEHOLDER ==========

function initializeSpectrumPlaceholder() {
    const canvas = document.getElementById('spectrum-canvas');
    if (!canvas) return;

    const ctx = canvas.getContext('2d');
    canvas.width = canvas.offsetWidth * 2; // Retina resolution
    canvas.height = canvas.offsetHeight * 2;
    ctx.scale(2, 2);

    const width = canvas.offsetWidth;
    const height = canvas.offsetHeight;

    // Draw grid
    ctx.strokeStyle = 'rgba(139, 115, 85, 0.2)';
    ctx.lineWidth = 1;

    // Horizontal lines (dB grid)
    for (let i = 0; i <= 4; i++) {
        const y = (height / 4) * i;
        ctx.beginPath();
        ctx.moveTo(0, y);
        ctx.lineTo(width, y);
        ctx.stroke();
    }

    // Vertical lines (frequency grid)
    for (let i = 0; i <= 10; i++) {
        const x = (width / 10) * i;
        ctx.beginPath();
        ctx.moveTo(x, 0);
        ctx.lineTo(x, height);
        ctx.stroke();
    }

    // Draw placeholder spectrum curve
    ctx.strokeStyle = 'rgba(107, 142, 35, 0.5)';
    ctx.lineWidth = 2;
    ctx.beginPath();

    for (let x = 0; x < width; x++) {
        // Simulate frequency response curve
        const freq = Math.pow(x / width, 1.5);
        const lowBump = Math.sin(freq * Math.PI * 2) * 0.2;
        const midBump = Math.sin(freq * Math.PI * 4) * 0.3;
        const y = height * (0.5 + lowBump + midBump);

        if (x === 0) {
            ctx.moveTo(x, y);
        } else {
            ctx.lineTo(x, y);
        }
    }
    ctx.stroke();

    // Add text
    ctx.fillStyle = 'rgba(60, 47, 47, 0.4)';
    ctx.font = '14px Garamond';
    ctx.textAlign = 'center';
    ctx.fillText('Spectrum Analyzer', width / 2, height / 2 - 10);
    ctx.fillText('(Phase 5.3)', width / 2, height / 2 + 10);
}

// ========== v1.2.0: REAL-TIME SPECTRUM ANALYZER ==========

// Spectrum state (initialized lazily when first data arrives)
let spectrumCtx = null;
let spectrumWidth = 0;
let spectrumHeight = 0;
let smoothedSpectrum = null;
const SMOOTHING = 0.7;

// Called by C++ with FFT magnitude data (64 bins, normalized 0-1)
window.updateSpectrumData = function(magnitudes) {
    try {
        if (!Array.isArray(magnitudes) || magnitudes.length === 0) return;

        const canvas = document.getElementById('spectrum-canvas');
        if (!canvas) return;

        // Lazy initialization
        if (!spectrumCtx) {
            spectrumCtx = canvas.getContext('2d');
            const dpr = window.devicePixelRatio || 1;
            spectrumWidth = canvas.offsetWidth;
            spectrumHeight = canvas.offsetHeight;
            canvas.width = spectrumWidth * dpr;
            canvas.height = spectrumHeight * dpr;
            spectrumCtx.scale(dpr, dpr);
            smoothedSpectrum = new Array(magnitudes.length).fill(0);
        }

        // Apply smoothing
        for (let i = 0; i < magnitudes.length; i++) {
            smoothedSpectrum[i] = smoothedSpectrum[i] * SMOOTHING + magnitudes[i] * (1 - SMOOTHING);
        }

        // Clear and draw grid
        spectrumCtx.clearRect(0, 0, spectrumWidth, spectrumHeight);
        spectrumCtx.strokeStyle = 'rgba(139, 115, 85, 0.15)';
        spectrumCtx.lineWidth = 1;

        // Horizontal grid lines
        for (let i = 0; i <= 4; i++) {
            const y = (spectrumHeight / 4) * i;
            spectrumCtx.beginPath();
            spectrumCtx.moveTo(0, y);
            spectrumCtx.lineTo(spectrumWidth, y);
            spectrumCtx.stroke();
        }

        // Vertical grid lines
        for (let i = 0; i <= 8; i++) {
            const x = (spectrumWidth / 8) * i;
            spectrumCtx.beginPath();
            spectrumCtx.moveTo(x, 0);
            spectrumCtx.lineTo(x, spectrumHeight);
            spectrumCtx.stroke();
        }

        // Draw spectrum fill
        const gradient = spectrumCtx.createLinearGradient(0, spectrumHeight, 0, 0);
        gradient.addColorStop(0, 'rgba(107, 142, 35, 0.1)');
        gradient.addColorStop(0.5, 'rgba(107, 142, 35, 0.3)');
        gradient.addColorStop(1, 'rgba(139, 115, 85, 0.5)');

        spectrumCtx.beginPath();
        spectrumCtx.moveTo(0, spectrumHeight);

        const numBins = smoothedSpectrum.length;
        for (let i = 0; i < numBins; i++) {
            const x = (i / (numBins - 1)) * spectrumWidth;
            const y = spectrumHeight * (1 - smoothedSpectrum[i]);
            spectrumCtx.lineTo(x, y);
        }

        spectrumCtx.lineTo(spectrumWidth, spectrumHeight);
        spectrumCtx.closePath();
        spectrumCtx.fillStyle = gradient;
        spectrumCtx.fill();

        // Draw spectrum line
        spectrumCtx.beginPath();
        spectrumCtx.strokeStyle = 'rgba(107, 142, 35, 0.8)';
        spectrumCtx.lineWidth = 1.5;

        for (let i = 0; i < numBins; i++) {
            const x = (i / (numBins - 1)) * spectrumWidth;
            const y = spectrumHeight * (1 - smoothedSpectrum[i]);
            if (i === 0) spectrumCtx.moveTo(x, y);
            else spectrumCtx.lineTo(x, y);
        }
        spectrumCtx.stroke();

    } catch (e) {
        console.error('Spectrum update error:', e);
    }
};

// ========== PHASE 5.3: METERING FUNCTIONS ==========

// Called by C++ to update gain reduction meters
window.updateGainReductionMeters = function(lowNorm, lomidNorm, himidNorm, highNorm) {
    // Update each band's GR meter (normalized 0-1, where 1 = full -24 dB GR)
    updateMeterFill('grLow', lowNorm);
    updateMeterFill('grLomid', lomidNorm);
    updateMeterFill('grHimid', himidNorm);
    updateMeterFill('grHigh', highNorm);
};

// Called by C++ to update input/output level meters
window.updateInputOutputMeters = function(inputLevel, outputLevel) {
    // Update meters (normalized 0-1 linear)
    updateMeterFill('inputMeterFill', inputLevel);
    updateMeterFill('outputMeterFill', outputLevel);
};

// Called by C++ to update crossover line positions
window.updateCrossoverPositions = function(xover1Hz, xover2Hz, xover3Hz) {
    // Convert Hz to X position (log scale: 20 Hz = 0%, 20000 Hz = 100%)
    const xover1Pos = freqToX(xover1Hz);
    const xover2Pos = freqToX(xover2Hz);
    const xover3Pos = freqToX(xover3Hz);

    // Update line positions
    const crossover1 = document.getElementById('crossover1');
    const crossover2 = document.getElementById('crossover2');
    const crossover3 = document.getElementById('crossover3');

    if (crossover1) {
        crossover1.style.left = `${xover1Pos}%`;
        const label = crossover1.querySelector('.crossover-label');
        if (label) label.textContent = formatFrequency(xover1Hz);
    }

    if (crossover2) {
        crossover2.style.left = `${xover2Pos}%`;
        const label = crossover2.querySelector('.crossover-label');
        if (label) label.textContent = formatFrequency(xover2Hz);
    }

    if (crossover3) {
        crossover3.style.left = `${xover3Pos}%`;
        const label = crossover3.querySelector('.crossover-label');
        if (label) label.textContent = formatFrequency(xover3Hz);
    }

    // v1.4.0: this is the only path that sees host automation and preset loads,
    // so the band headers have to follow it too — not just the drag handler.
    updateBandRanges(xover1Hz, xover2Hz, xover3Hz);
};

// Helper: Update meter fill height/width
function updateMeterFill(elementId, normalizedValue) {
    const element = document.getElementById(elementId);
    if (!element) return;

    // Clamp to valid range
    const clamped = Math.max(0, Math.min(1, normalizedValue));

    // Check if it's a vertical meter (input/output) or horizontal (GR)
    if (elementId === 'inputMeterFill' || elementId === 'outputMeterFill') {
        // Vertical meter - height from bottom
        element.style.height = `${clamped * 100}%`;
    } else {
        // Horizontal GR meter - width from left
        element.style.width = `${clamped * 100}%`;
    }
}

// Helper: Convert frequency to X position (log scale)
function freqToX(freq) {
    const minLog = Math.log10(20);
    const maxLog = Math.log10(20000);
    const freqLog = Math.log10(Math.max(20, Math.min(20000, freq)));
    return ((freqLog - minLog) / (maxLog - minLog)) * 100;
}

// Helper: Format frequency for display
function formatFrequency(freq) {
    if (freq >= 1000) {
        // Round to one decimal but drop a trailing ".0", so 2000 Hz reads
        // "2 kHz" (matching the shipped defaults) rather than "2.0 kHz".
        return (Math.round(freq / 100) / 10) + ' kHz';
    }
    return Math.round(freq) + ' Hz';
}

// ========== v1.4.0: LIVE BAND FREQUENCY RANGES ==========

// The analyzer's fixed display span; also the outer edges of the LOW and HIGH bands.
const SPECTRUM_MIN_HZ = 20;
const SPECTRUM_MAX_HZ = 20000;

// Rewrite the four band-header readouts from the current crossover frequencies.
// The markup ships static strings for the defaults; without this they would keep
// advertising 20/200/2k/8k/20k no matter where the crossovers actually sit.
function updateBandRanges(xover1Hz, xover2Hz, xover3Hz) {
    setBandRange('range-low',   SPECTRUM_MIN_HZ, xover1Hz);
    setBandRange('range-lomid', xover1Hz,        xover2Hz);
    setBandRange('range-himid', xover2Hz,        xover3Hz);
    setBandRange('range-high',  xover3Hz,        SPECTRUM_MAX_HZ);
}

function setBandRange(elementId, lowHz, highHz) {
    const element = document.getElementById(elementId);
    if (!element) return;
    element.textContent = `${formatFrequency(lowHz)} - ${formatFrequency(highHz)}`;
}

// ========== CROSSOVER DRAG INTERACTION ==========

// Parameter ranges (must match PluginProcessor.cpp)
const XOVER_RANGES = {
    XOVER1: { min: 20, max: 500 },
    XOVER2: { min: 200, max: 5000 },
    XOVER3: { min: 2000, max: 16000 }
};

// Minimum gap between crossovers (Hz) to prevent overlap
const MIN_CROSSOVER_GAP = 100;

// Active drag state
let activeDrag = null;

// Initialize crossover drag handlers
function initializeCrossoverDrag() {
    const crossovers = [
        { element: document.getElementById('crossover1'), param: 'XOVER1', index: 1 },
        { element: document.getElementById('crossover2'), param: 'XOVER2', index: 2 },
        { element: document.getElementById('crossover3'), param: 'XOVER3', index: 3 }
    ];

    crossovers.forEach(xover => {
        if (!xover.element) return;

        // Mouse down - start drag
        xover.element.addEventListener('mousedown', (e) => {
            e.preventDefault();
            startCrossoverDrag(xover.param, xover.index, xover.element);
        });

        // Touch support for mobile/tablet
        xover.element.addEventListener('touchstart', (e) => {
            e.preventDefault();
            startCrossoverDrag(xover.param, xover.index, xover.element);
        }, { passive: false });
    });

    // Global mouse/touch move and up handlers
    document.addEventListener('mousemove', handleCrossoverDrag);
    document.addEventListener('mouseup', endCrossoverDrag);
    document.addEventListener('touchmove', handleCrossoverDrag, { passive: false });
    document.addEventListener('touchend', endCrossoverDrag);

    console.log('Crossover drag handlers initialized');
}

function startCrossoverDrag(paramId, index, element) {
    const sliderState = parameterStates[paramId];
    if (!sliderState) {
        console.warn(`No slider state for ${paramId}`);
        return;
    }

    // Notify JUCE that drag is starting (for undo/redo grouping)
    sliderState.sliderDragStarted();

    activeDrag = {
        paramId: paramId,
        index: index,
        element: element,
        sliderState: sliderState
    };

    // Visual feedback
    element.classList.add('dragging');
    document.body.style.cursor = 'ew-resize';

    console.log(`Started dragging ${paramId}`);
}

function handleCrossoverDrag(e) {
    if (!activeDrag) return;

    e.preventDefault();

    // Get the spectrum container for position calculation
    const container = document.querySelector('.spectrum-container');
    if (!container) return;

    const rect = container.getBoundingClientRect();

    // Get X position from mouse or touch
    let clientX;
    if (e.type === 'touchmove') {
        clientX = e.touches[0].clientX;
    } else {
        clientX = e.clientX;
    }

    // Calculate position as percentage (0-100)
    let xPercent = ((clientX - rect.left) / rect.width) * 100;
    xPercent = Math.max(0, Math.min(100, xPercent));

    // Convert X position to frequency (inverse of freqToX)
    let freq = xToFreq(xPercent);

    // Apply range constraints for this crossover
    const range = XOVER_RANGES[activeDrag.paramId];
    freq = Math.max(range.min, Math.min(range.max, freq));

    // Apply ordering constraints to prevent overlap
    freq = applyOrderingConstraints(activeDrag.paramId, freq);

    // Convert frequency to normalized value for JUCE parameter
    const normValue = freqToNormalized(freq, range.min, range.max);

    // Update the JUCE parameter
    activeDrag.sliderState.setNormalisedValue(normValue);

    // Update visual position immediately (don't wait for C++ callback)
    const newXPercent = freqToX(freq);
    activeDrag.element.style.left = `${newXPercent}%`;

    // Update label
    const label = activeDrag.element.querySelector('.crossover-label');
    if (label) {
        label.textContent = formatFrequency(freq);
    }

    // v1.4.0: update the band headers from the live drag value rather than waiting
    // for the 30 Hz C++ push, which would otherwise trail the handle visibly.
    const freqs = getCrossoverFreqs();
    freqs[activeDrag.paramId] = freq;
    updateBandRanges(freqs.XOVER1, freqs.XOVER2, freqs.XOVER3);
}

function endCrossoverDrag() {
    if (!activeDrag) return;

    // Notify JUCE that drag ended
    activeDrag.sliderState.sliderDragEnded();

    // Remove visual feedback
    activeDrag.element.classList.remove('dragging');
    document.body.style.cursor = '';

    console.log(`Ended dragging ${activeDrag.paramId}`);
    activeDrag = null;
}

// Convert X position (0-100%) back to frequency (log scale)
function xToFreq(xPercent) {
    const minLog = Math.log10(20);
    const maxLog = Math.log10(20000);
    const freqLog = minLog + (xPercent / 100) * (maxLog - minLog);
    return Math.pow(10, freqLog);
}

// Convert frequency to normalized value (0-1) for a given range
// Uses logarithmic mapping to match JUCE's skew factor of 0.3
function freqToNormalized(freq, minFreq, maxFreq) {
    // Skew factor 0.3 means: normalized = pow(linear, 0.3)
    // So: linear = pow(normalized, 1/0.3)
    // And: normalized = pow(linear, 0.3)
    const linear = (freq - minFreq) / (maxFreq - minFreq);
    const skew = 0.3;
    return Math.pow(Math.max(0, Math.min(1, linear)), skew);
}

// Read the three crossover frequencies from the bound parameters.
// Note the ?? rather than ||: a normalised value of exactly 0 is legitimate (it
// means the crossover is parked at the bottom of its range), and || would have
// silently substituted the 0.5 fallback for it.
function getCrossoverFreqs() {
    const read = (paramId) => {
        const range = XOVER_RANGES[paramId];
        const norm = parameterStates[paramId]?.getNormalisedValue() ?? 0.5;
        return normalizedToFreq(norm, range.min, range.max);
    };

    return {
        XOVER1: read('XOVER1'),
        XOVER2: read('XOVER2'),
        XOVER3: read('XOVER3')
    };
}

// Apply ordering constraints: XOVER1 < XOVER2 < XOVER3
function applyOrderingConstraints(paramId, freq) {
    const {
        XOVER1: xover1Freq,
        XOVER2: xover2Freq,
        XOVER3: xover3Freq
    } = getCrossoverFreqs();

    if (paramId === 'XOVER1') {
        // XOVER1 must be at least MIN_GAP below XOVER2
        const maxAllowed = xover2Freq - MIN_CROSSOVER_GAP;
        return Math.min(freq, maxAllowed);
    } else if (paramId === 'XOVER2') {
        // XOVER2 must be at least MIN_GAP above XOVER1 and below XOVER3
        const minAllowed = xover1Freq + MIN_CROSSOVER_GAP;
        const maxAllowed = xover3Freq - MIN_CROSSOVER_GAP;
        return Math.max(minAllowed, Math.min(freq, maxAllowed));
    } else if (paramId === 'XOVER3') {
        // XOVER3 must be at least MIN_GAP above XOVER2
        const minAllowed = xover2Freq + MIN_CROSSOVER_GAP;
        return Math.max(freq, minAllowed);
    }

    return freq;
}

// Convert normalized value (0-1) back to frequency
function normalizedToFreq(norm, minFreq, maxFreq) {
    const skew = 0.3;
    const linear = Math.pow(norm, 1.0 / skew);
    return minFreq + linear * (maxFreq - minFreq);
}

// Initialize drag handlers after DOM is ready
if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', initializeCrossoverDrag);
} else {
    initializeCrossoverDrag();
}

// ========== v1.4.0: TOOLTIPS ==========

const TOOLTIP_DELAY_MS = 120;
const TOOLTIP_MARGIN = 8;   // gap between a tip and its control / the viewport edge

// Controls that exist exactly once. Entries are [selector, title, body, wrapper?] —
// the optional wrapper widens the hover target to a surrounding element, so the
// label and value readout trigger the tip as well as the control itself.
const GLOBAL_TOOLTIPS = [
    ['#input-gain', 'Input Gain',
     'Level trim applied before the signal is split into bands. Use it to drive the compressors harder or back them off without touching the thresholds. −24 to +24 dB.',
     '.control-group'],

    ['#mix', 'Mix',
     'Blend between the dry input and the compressed output for parallel compression. 0% is fully dry, 100% is fully compressed.',
     '.control-group'],

    ['#auto-makeup', 'Auto Makeup',
     'Automatically compensates the level lost to gain reduction in each band, so bypassing the compressor does not jump in volume. Stacks with each band’s Makeup knob.',
     '.control-group'],

    ['#ms-mode', 'Mid / Side Mode',
     'Chooses what the compressors act on. Off processes left and right normally; Mid targets the centre of the image, Side the stereo edges, and Both processes them independently.',
     '.control-group'],

    ['#output-gain', 'Output Gain',
     'Final level trim, applied after the mix stage. −24 to +24 dB.',
     '.control-group'],

    ['.input-meter', 'Input Meter',
     'Level entering the plugin, averaged across both channels and measured before the input gain trim.'],

    ['.output-meter', 'Output Meter',
     'Level leaving the plugin, measured after mix and output gain.'],

    ['.spectrum-container', 'Spectrum Analyzer',
     'Real-time input spectrum, 20 Hz to 20 kHz on a logarithmic scale. Drag the vertical lines to move the crossovers.'],

    ['#crossover1', 'Crossover 1',
     'Split point between the Low and Low-Mid bands. Drag left or right to move it; the two band headers update as you go. 20 Hz to 500 Hz.'],

    ['#crossover2', 'Crossover 2',
     'Split point between the Low-Mid and High-Mid bands. Drag left or right to move it. 200 Hz to 5 kHz.'],

    ['#crossover3', 'Crossover 3',
     'Split point between the High-Mid and High bands. Drag left or right to move it. 2 kHz to 16 kHz.']
];

// Display names and GR-meter element ids, keyed by the band id used throughout app.js.
const BAND_LABELS = { low: 'Low', lomid: 'Low-Mid', himid: 'High-Mid', high: 'High' };
const BAND_GR_METERS = { low: 'grLow', lomid: 'grLomid', himid: 'grHimid', high: 'grHigh' };

// Per-band controls. Identical in all four bands, so the wording lives here once.
// Entries are [controlSuffix, wrapper, title, body].
const BAND_TOOLTIPS = [
    ['threshold', '.knob-control', 'Threshold',
     'The level at which this band starts to compress. Anything above it is pulled down by the Ratio. −60 to 0 dB.'],

    ['ratio', '.knob-control', 'Ratio',
     'How firmly the band is compressed above the threshold. 1:1 leaves it untouched; 20:1 is effectively limiting.'],

    ['attack', '.knob-control', 'Attack',
     'How quickly compression engages once the signal crosses the threshold. Fast settings clamp transients, slow settings let them through. 0.1 to 200 ms.'],

    ['release', '.knob-control', 'Release',
     'How quickly compression lets go once the signal falls back below the threshold. Too fast can pump, too slow can choke the band. 10 to 2000 ms.'],

    ['knee', '.knob-control', 'Knee',
     'Softens the onset of compression around the threshold. 0 dB is a hard knee that grabs abruptly; 24 dB eases in very gradually.'],

    ['makeup', '.knob-control', 'Makeup',
     'Manual gain applied to this band after compression, to restore what gain reduction took away. −12 to +24 dB.'],

    ['solo', null, 'Solo',
     'Hear this band on its own — the other three are muted. Useful for checking where a crossover should sit.'],

    ['bypass', null, 'Bypass',
     'Pass this band through uncompressed. The crossover filtering still applies, so the band stays in phase with the others.'],

    ['sc-listen', null, 'Sidechain Listen',
     'Monitor the detector signal driving this band’s compressor, including its sidechain filtering. This is what the compressor "hears", not what it outputs.']
];

let tooltipEl = null;
let tooltipTimer = null;
let tooltipTarget = null;
let tooltipSuppressed = false;

function initializeTooltips() {
    tooltipEl = document.getElementById('tooltip');
    if (!tooltipEl) {
        console.warn('Tooltip element not found - tooltips disabled');
        return;
    }

    GLOBAL_TOOLTIPS.forEach(([selector, title, body, wrapper]) =>
        applyTooltip(selector, title, body, wrapper));

    Object.keys(BAND_LABELS).forEach(applyBandTooltips);

    document.addEventListener('mouseover', handleTooltipOver);
    document.addEventListener('mouseout', handleTooltipOut);

    // Any press begins a click or a drag, so get the tip out of the way and keep
    // it away until the pointer is released. Capture phase, because the crossover
    // lines and knobs call preventDefault on their own mousedown handlers.
    document.addEventListener('mousedown', () => {
        tooltipSuppressed = true;
        hideTooltip();
    }, true);

    document.addEventListener('mouseup', () => { tooltipSuppressed = false; }, true);

    console.log('Tooltips initialized');
}

function applyBandTooltips(bandId) {
    const bandName = BAND_LABELS[bandId];

    BAND_TOOLTIPS.forEach(([control, wrapper, title, body]) =>
        applyTooltip(`#${bandId}-${control}`, `${bandName} — ${title}`, body, wrapper));

    applyTooltip(`#${BAND_GR_METERS[bandId]}`, `${bandName} — Gain Reduction`,
        'How much this band is being compressed right now. The bar fills as gain reduction deepens, up to −24 dB.',
        '.gr-meter');

    applyTooltip(`#range-${bandId}`, `${bandName} — Frequency Range`,
        'The span this band processes. It follows the crossover handles in the analyzer above, so drag them to retune it.',
        '.band-header');
}

function applyTooltip(selector, title, body, wrapper) {
    const element = document.querySelector(selector);
    if (!element) {
        console.warn(`Tooltip target not found: ${selector}`);
        return;
    }

    const target = wrapper ? (element.closest(wrapper) || element) : element;
    target.setAttribute('data-tip-title', title);
    target.setAttribute('data-tip', body);
}

function handleTooltipOver(e) {
    const target = e.target.closest ? e.target.closest('[data-tip]') : null;
    if (!target || target === tooltipTarget) return;

    tooltipTarget = target;
    clearTimeout(tooltipTimer);

    if (tooltipSuppressed) return;
    tooltipTimer = setTimeout(() => showTooltip(target), TOOLTIP_DELAY_MS);
}

function handleTooltipOut(e) {
    const target = e.target.closest ? e.target.closest('[data-tip]') : null;
    if (!target) return;

    // Moving between children of the same control is not a real exit
    if (e.relatedTarget && target.contains(e.relatedTarget)) return;

    hideTooltip();
}

function showTooltip(target) {
    // The pointer may have moved on or gone down during the delay
    if (!tooltipEl || tooltipSuppressed || target !== tooltipTarget) return;

    const title = target.getAttribute('data-tip-title');
    const body = target.getAttribute('data-tip');

    // Built with textContent rather than innerHTML so the copy stays inert
    tooltipEl.textContent = '';

    if (title) {
        const titleEl = document.createElement('div');
        titleEl.className = 'tooltip-title';
        titleEl.textContent = title;
        tooltipEl.appendChild(titleEl);
    }

    const bodyEl = document.createElement('div');
    bodyEl.className = 'tooltip-body';
    bodyEl.textContent = body;
    tooltipEl.appendChild(bodyEl);

    // Measure and place while still transparent, so the tip never flashes at its
    // previous position before settling on the new one.
    const anchor = target.getBoundingClientRect();
    const tip = tooltipEl.getBoundingClientRect();

    // Prefer above; flip below only when there is no room at the top
    let top = anchor.top - tip.height - TOOLTIP_MARGIN;
    let placement = 'above';

    if (top < TOOLTIP_MARGIN) {
        top = anchor.bottom + TOOLTIP_MARGIN;
        placement = 'below';
    }

    const anchorCentreX = anchor.left + anchor.width / 2;
    const maxLeft = window.innerWidth - tip.width - TOOLTIP_MARGIN;
    const left = Math.max(TOOLTIP_MARGIN, Math.min(maxLeft, anchorCentreX - tip.width / 2));

    tooltipEl.style.left = `${left}px`;
    tooltipEl.style.top = `${top}px`;
    tooltipEl.dataset.placement = placement;

    // The tip is clamped to the viewport but the arrow still points at the control,
    // held clear of the rounded corners.
    const arrowX = Math.max(10, Math.min(tip.width - 10, anchorCentreX - left));
    tooltipEl.style.setProperty('--arrow-x', `${arrowX}px`);

    tooltipEl.classList.add('visible');
    tooltipEl.setAttribute('aria-hidden', 'false');
}

function hideTooltip() {
    clearTimeout(tooltipTimer);
    tooltipTarget = null;

    if (!tooltipEl) return;
    tooltipEl.classList.remove('visible');
    tooltipEl.setAttribute('aria-hidden', 'true');
}

// Initialize tooltips once their state above has been evaluated. This must stay at
// the foot of the file: initializeUI() runs at module top level, where these
// `let`/`const` bindings are still in the temporal dead zone, and the ReferenceError
// would escape module evaluation and take initializeCrossoverDrag() down with it.
if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', initializeTooltips);
} else {
    initializeTooltips();
}
