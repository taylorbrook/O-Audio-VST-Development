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

    bindSlider('timbre', (value) => {
        document.getElementById('timbreValue').textContent = `${Math.round(value * 100)}%`;
    });

    // v1.1.0: New decay time parameter (0.1-20s, skewed range)
    bindSlider('decayTime', (value) => {
        // Skewed range: value^0.4 maps 0-1 to 0.1-20s
        // Inverse: normalized = ((actual - 0.1) / 19.9)^(1/0.4)
        // To display: apply skew to normalized value
        const skew = 0.4;
        const minVal = 0.1;
        const maxVal = 20.0;
        const actual = minVal + Math.pow(value, 1/skew) * (maxVal - minVal);
        document.getElementById('decayTimeValue').textContent = `${actual.toFixed(1)} s`;
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
        // v1.0.3 FIX: Use getChoiceIndex() not getSelectedItemIndex()
        const initialIndex = comboBoxState.getChoiceIndex();
        element.selectedIndex = initialIndex;

        // Update JUCE when element changes (UI → C++)
        element.addEventListener('change', (e) => {
            const index = e.target.selectedIndex;
            // v1.0.3 FIX: Use setChoiceIndex() not setSelectedItemIndex()
            comboBoxState.setChoiceIndex(index);
        });

        // Update element when JUCE changes (C++ → UI, automation, preset load)
        comboBoxState.valueChangedEvent.addListener(() => {
            const index = comboBoxState.getChoiceIndex();
            element.selectedIndex = index;
        });

        console.log(`Choice bound: ${paramId}`);
    } catch (error) {
        console.error(`Failed to bind choice ${paramId}:`, error);
    }
}

/**
 * Initialize real-time meters (voice count, CPU usage)
 * Uses native function calls to get real-time data from C++
 */
function initializeMeters() {
    const voiceCountElement = document.getElementById('voiceCount');
    const cpuUsageElement = document.getElementById('cpuUsage');

    // Get native function for voice count (Phase 3.3)
    const getVoiceCount = Juce.getNativeFunction('getVoiceCount');

    // Update voice count periodically
    const updateVoiceCount = async () => {
        try {
            const count = await getVoiceCount();
            voiceCountElement.textContent = `${count}/16`;
        } catch (error) {
            console.error('Failed to get voice count:', error);
        }
    };

    // Initial update
    updateVoiceCount();

    // Poll every 100ms for voice count updates
    setInterval(updateVoiceCount, 100);

    // CPU usage placeholder (would need additional native function)
    // For now, estimate based on voice count (rough approximation)
    setInterval(() => {
        const voiceText = voiceCountElement.textContent;
        const voiceMatch = voiceText.match(/(\d+)\/16/);
        if (voiceMatch) {
            const voices = parseInt(voiceMatch[1], 10);
            // Rough estimate: ~1% CPU per voice at high quality
            const cpuEstimate = Math.min(100, voices * 1.0);
            cpuUsageElement.textContent = `~${cpuEstimate.toFixed(0)}%`;
        }
    }, 500);

    console.log('Meters initialized with native function');
}
