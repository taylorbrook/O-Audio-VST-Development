/*
  ==============================================================================

    O-MultiBandCompressor - UI Application Logic
    Phase 5.2: Full parameter binding implementation (56 parameters)

  ==============================================================================
*/

console.log('O-MultiBandCompressor UI initializing (Phase 5.2)...');

// Parameter binding state
const parameterStates = {};

// Wait for JUCE backend to connect
if (window.__JUCE__?.backend) {
    window.__JUCE__.backend.addEventListener('backendConnected', initializeUI);
} else {
    // Fallback for testing outside JUCE
    document.addEventListener('DOMContentLoaded', () => {
        console.warn('Running outside JUCE - parameter binding disabled');
        initializeSpectrumPlaceholder();
    });
}

function initializeUI() {
    console.log('JUCE backend connected - initializing parameter bindings');

    // Initialize all parameter bindings
    bindGlobalParameters();
    bindBandParameters('LOW', 'low');
    bindBandParameters('LOMID', 'lomid');
    bindBandParameters('HIMID', 'himid');
    bindBandParameters('HIGH', 'high');

    // Initialize spectrum placeholder
    initializeSpectrumPlaceholder();

    console.log('Phase 5.2 UI initialized - 56 parameters bound');
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

    // Crossover frequencies (not visible in UI yet - Phase 5.3)
    // Still need to bind for APVTS sync
    if (window.__JUCE__?.initialisers?.getSliderState) {
        parameterStates['XOVER1'] = window.__JUCE__.initialisers.getSliderState('XOVER1');
        parameterStates['XOVER2'] = window.__JUCE__.initialisers.getSliderState('XOVER2');
        parameterStates['XOVER3'] = window.__JUCE__.initialisers.getSliderState('XOVER3');
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

    // Attack: 0.1 to 200 ms (logarithmic)
    bindSlider(`${bandId}-attack`, `${bandPrefix}_ATTACK`, (norm) => {
        const ms = Math.pow(10, norm * (Math.log10(200) - Math.log10(0.1)) + Math.log10(0.1));
        return ms.toFixed(1) + ' ms';
    });

    // Release: 10 to 2000 ms (logarithmic)
    bindSlider(`${bandId}-release`, `${bandPrefix}_RELEASE`, (norm) => {
        const ms = Math.pow(10, norm * (Math.log10(2000) - Math.log10(10)) + Math.log10(10));
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

    // Peak/RMS: 0-100% (not visible in UI yet - simplified)
    if (window.__JUCE__?.initialisers?.getSliderState) {
        parameterStates[`${bandPrefix}_PEAK_RMS`] = window.__JUCE__.initialisers.getSliderState(`${bandPrefix}_PEAK_RMS`);
    }

    // Solo, Bypass, SC Listen: bool
    bindToggle(`${bandId}-solo`, `${bandPrefix}_SOLO`);
    bindToggle(`${bandId}-bypass`, `${bandPrefix}_BYPASS`);
    bindToggle(`${bandId}-sc-listen`, `${bandPrefix}_SC_LISTEN`);

    // SC HPF/LPF (not visible in UI yet - Phase 5.3)
    if (window.__JUCE__?.initialisers?.getSliderState) {
        parameterStates[`${bandPrefix}_SC_HPF`] = window.__JUCE__.initialisers.getSliderState(`${bandPrefix}_SC_HPF`);
        parameterStates[`${bandPrefix}_SC_LPF`] = window.__JUCE__.initialisers.getSliderState(`${bandPrefix}_SC_LPF`);
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

    if (!window.__JUCE__?.initialisers?.getSliderState) {
        console.warn('JUCE slider state API not available');
        return;
    }

    // Get slider state from JUCE
    const sliderState = window.__JUCE__.initialisers.getSliderState(parameterId);
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

    // Update UI when parameter changes (automation, preset load)
    sliderState.valueChangedEvent.addListener(() => {
        const norm = sliderState.getNormalisedValue();
        element.value = norm;

        if (valueDisplay && formatValue) {
            valueDisplay.textContent = formatValue(norm);
        }
    });

    console.log(`Bound slider: ${parameterId} → #${elementId}`);
}

function bindToggle(elementId, parameterId) {
    const element = document.getElementById(elementId);

    if (!element) {
        console.warn(`Toggle element not found: ${elementId}`);
        return;
    }

    if (!window.__JUCE__?.initialisers?.getToggleState) {
        console.warn('JUCE toggle state API not available');
        return;
    }

    // Get toggle state from JUCE
    const toggleState = window.__JUCE__.initialisers.getToggleState(parameterId);
    parameterStates[parameterId] = toggleState;

    // Initialize element with current state
    const initialValue = toggleState.getValue();
    if (initialValue) {
        element.classList.add('active');
        element.textContent = 'On';
    } else {
        element.classList.remove('active');
        element.textContent = 'Off';
    }

    // Update parameter when button clicked
    element.addEventListener('click', () => {
        const newValue = !toggleState.getValue();
        toggleState.setValue(newValue);

        if (newValue) {
            element.classList.add('active');
            element.textContent = 'On';
        } else {
            element.classList.remove('active');
            element.textContent = 'Off';
        }
    });

    // Update UI when parameter changes (automation, preset load)
    toggleState.valueChangedEvent.addListener(() => {
        const value = toggleState.getValue();
        if (value) {
            element.classList.add('active');
            element.textContent = 'On';
        } else {
            element.classList.remove('active');
            element.textContent = 'Off';
        }
    });

    console.log(`Bound toggle: ${parameterId} → #${elementId}`);
}

function bindComboBox(elementId, parameterId) {
    const element = document.getElementById(elementId);

    if (!element) {
        console.warn(`ComboBox element not found: ${elementId}`);
        return;
    }

    if (!window.__JUCE__?.initialisers?.getComboBoxState) {
        console.warn('JUCE combobox state API not available');
        return;
    }

    // Get combobox state from JUCE
    const comboState = window.__JUCE__.initialisers.getComboBoxState(parameterId);
    parameterStates[parameterId] = comboState;

    // Initialize element with current value
    const initialId = comboState.getSelectedId();
    element.selectedIndex = initialId;

    // Update parameter when selection changes
    element.addEventListener('change', (e) => {
        const selectedId = parseInt(e.target.selectedIndex);
        comboState.setSelectedId(selectedId);
    });

    // Update UI when parameter changes (automation, preset load)
    comboState.valueChangedEvent.addListener(() => {
        const selectedId = comboState.getSelectedId();
        element.selectedIndex = selectedId;
    });

    console.log(`Bound combobox: ${parameterId} → #${elementId}`);
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
        return (freq / 1000).toFixed(1) + ' kHz';
    } else {
        return freq.toFixed(0) + ' Hz';
    }
}
