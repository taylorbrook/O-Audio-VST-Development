/**
 * OuariconLyrica - WebView UI Controller
 * Binds all 19 APVTS parameters to HTML controls
 */

// CRITICAL: Import JUCE functions from the embedded bridge
import * as Juce from './juce/index.js';

// Wait for DOM to be fully loaded
document.addEventListener('DOMContentLoaded', () => {
    console.log('OuariconLyrica UI initializing...');
    console.log('JUCE backend available:', typeof window.__JUCE__ !== 'undefined');

    initializeParameters();
    initializeMeters();
});

/**
 * Initialize all parameter bindings
 */
function initializeParameters() {
    // Float parameters (sliders)
    bindSlider('masterVolume', (value) => {
        const db = -60 + (value * 66); // -60 to +6 dB
        document.getElementById('masterVolumeValue').textContent = `${db.toFixed(1)} dB`;
    });

    bindSlider('brightness', (value) => {
        document.getElementById('brightnessValue').textContent = `${Math.round(value * 100)}%`;
    });

    bindSlider('sustain', (value) => {
        document.getElementById('sustainValue').textContent = `${Math.round(value * 100)}%`;
    });

    bindSlider('bodySize', (value) => {
        document.getElementById('bodySizeValue').textContent = `${Math.round(value * 100)}%`;
    });

    bindSlider('bodyResonance', (value) => {
        document.getElementById('bodyResonanceValue').textContent = `${Math.round(value * 100)}%`;
    });

    bindSlider('sympatheticAmount', (value) => {
        document.getElementById('sympatheticAmountValue').textContent = `${Math.round(value * 100)}%`;
    });

    bindSlider('pluckPosition', (value) => {
        document.getElementById('pluckPositionValue').textContent = `${Math.round(value * 100)}%`;
    });

    bindSlider('fingerHardness', (value) => {
        document.getElementById('fingerHardnessValue').textContent = `${Math.round(value * 100)}%`;
    });

    bindSlider('stringTension', (value) => {
        document.getElementById('stringTensionValue').textContent = `${Math.round(value * 100)}%`;
    });

    bindSlider('stringGauge', (value) => {
        document.getElementById('stringGaugeValue').textContent = `${Math.round(value * 100)}%`;
    });

    bindSlider('stringLength', (value) => {
        document.getElementById('stringLengthValue').textContent = `${Math.round(value * 100)}%`;
    });

    bindSlider('stringStiffness', (value) => {
        document.getElementById('stringStiffnessValue').textContent = `${Math.round(value * 100)}%`;
    });

    bindSlider('masterTune', (value) => {
        const hz = 400 + (value * 80); // 400-480 Hz
        document.getElementById('masterTuneValue').textContent = `${hz.toFixed(1)} Hz`;
    });

    bindSlider('pitchBendRange', (value) => {
        const semitones = Math.round(1 + (value * 47)); // 1-48 st
        document.getElementById('pitchBendRangeValue').textContent = `±${semitones} st`;
    });

    // Choice parameters (dropdowns)
    bindChoice('stringMaterial');
    bindChoice('woodType');
    bindChoice('technique');
    bindChoice('glissandoMode');
    bindChoice('glissandoScale');
}

/**
 * Bind a slider parameter (float/int)
 * @param {string} paramId - Parameter ID matching APVTS
 * @param {function} updateDisplay - Callback to update value display
 */
function bindSlider(paramId, updateDisplay) {
    const element = document.getElementById(paramId);
    if (!element) {
        console.error(`Slider element not found: ${paramId}`);
        return;
    }

    try {
        // Get slider state from JUCE bridge
        const sliderState = Juce.getSliderState(paramId);

        // Initialize element with current value
        const initialValue = sliderState.getNormalisedValue();
        element.value = initialValue;
        if (updateDisplay) updateDisplay(initialValue);

        // Update JUCE when element changes (UI → C++)
        element.addEventListener('input', (e) => {
            const value = parseFloat(e.target.value);
            sliderState.setNormalisedValue(value);
            if (updateDisplay) updateDisplay(value);
        });

        // Update element when JUCE changes (C++ → UI, automation, preset load)
        // CRITICAL: valueChangedEvent doesn't pass parameters in JUCE 8
        sliderState.valueChangedEvent.addListener(() => {
            const value = sliderState.getNormalisedValue();
            element.value = value;
            if (updateDisplay) updateDisplay(value);
        });

        console.log(`Slider bound: ${paramId}`);
    } catch (error) {
        console.error(`Failed to bind slider ${paramId}:`, error);
    }
}

/**
 * Bind a choice parameter (dropdown/select)
 * @param {string} paramId - Parameter ID matching APVTS
 */
function bindChoice(paramId) {
    const element = document.getElementById(paramId);
    if (!element) {
        console.error(`Choice element not found: ${paramId}`);
        return;
    }

    try {
        // Get combo box state from JUCE bridge
        const comboBoxState = Juce.getComboBoxState(paramId);

        // Initialize element with current value
        const initialIndex = comboBoxState.getSelectedItemIndex();
        element.selectedIndex = initialIndex;

        // Update JUCE when element changes (UI → C++)
        element.addEventListener('change', (e) => {
            const index = e.target.selectedIndex;
            comboBoxState.setSelectedItemIndex(index);
        });

        // Update element when JUCE changes (C++ → UI, automation, preset load)
        comboBoxState.valueChangedEvent.addListener(() => {
            const index = comboBoxState.getSelectedItemIndex();
            element.selectedIndex = index;
        });

        console.log(`Choice bound: ${paramId}`);
    } catch (error) {
        console.error(`Failed to bind choice ${paramId}:`, error);
    }
}

/**
 * Initialize real-time meters (voice count, CPU usage)
 * These are updated via custom JUCE events, not APVTS parameters
 */
function initializeMeters() {
    // Voice count meter
    const voiceCountElement = document.getElementById('voiceCount');

    // CPU usage meter
    const cpuUsageElement = document.getElementById('cpuUsage');

    // Update meters periodically (will be driven by C++ timer in future)
    setInterval(() => {
        // Placeholder values - will be replaced with actual JUCE callbacks
        // voiceCountElement.textContent = `${voiceCount}/16`;
        // cpuUsageElement.textContent = `${cpuUsage}%`;
    }, 100);

    console.log('Meters initialized (placeholder)');
}
