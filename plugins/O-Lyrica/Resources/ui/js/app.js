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

    // v1.3.0: New advanced physical modeling parameters
    bindSlider('attackNoise', (value) => {
        document.getElementById('attackNoiseValue').textContent = `${Math.round(value * 100)}%`;
    });

    bindSlider('sympatheticQ', (value) => {
        // Skewed range: 0.1-20.0, skew factor 0.5
        const skew = 0.5;
        const minVal = 0.1;
        const maxVal = 20.0;
        const actual = minVal + Math.pow(value, 1/skew) * (maxVal - minVal);
        document.getElementById('sympatheticQValue').textContent = `Q ${actual.toFixed(1)}`;
    });

    bindSlider('bodyModeSpread', (value) => {
        // Range: -1.0 to +1.0, centered at 0
        const spread = (value * 2) - 1;  // Convert 0-1 to -1 to +1
        const sign = spread >= 0 ? '+' : '';
        document.getElementById('bodyModeSpreadValue').textContent = `${sign}${Math.round(spread * 100)}%`;
    });

    bindSlider('bridgeBrightness', (value) => {
        document.getElementById('bridgeBrightnessValue').textContent = `${Math.round(value * 100)}%`;
    });

    bindSlider('masterTune', (value) => {
        const hz = 400 + (value * 80); // 400-480 Hz
        document.getElementById('masterTuneValue').textContent = `${hz.toFixed(1)} Hz`;
    });

    bindSlider('pitchBendRange', (value) => {
        const semitones = Math.round(1 + (value * 47)); // 1-48 st
        document.getElementById('pitchBendRangeValue').textContent = `±${semitones} st`;
    });

    // v1.21.0: Glissando speed (4-30 n/s, skew 0.5)
    bindSlider('glissandoSpeed', (value) => {
        const skew = 0.5;
        const minVal = 4.0;
        const maxVal = 30.0;
        const actual = minVal + Math.pow(value, 1/skew) * (maxVal - minVal);
        document.getElementById('glissandoSpeedValue').textContent = `${actual.toFixed(1)} n/s`;
    });

    // v1.23.0: Custom semitones slider (1-48 st)
    bindSlider('glissandoCustomSemitones', (value) => {
        const st = Math.round(1 + value * 47);
        document.getElementById('glissandoCustomSemitonesValue').textContent = `${st} st`;
    });

    // v1.26.0: Glissando excitation softness (0.0-1.0)
    bindSlider('glissandoExcitation', (value) => {
        document.getElementById('glissandoExcitationValue').textContent = `${Math.round(value * 100)}%`;
    });

    // v1.27.0: Glissando velocity profile (0.0-1.0)
    bindSlider('glissandoVelStart', (value) => {
        document.getElementById('glissandoVelStartValue').textContent = `${Math.round(value * 100)}%`;
    });
    bindSlider('glissandoVelEnd', (value) => {
        document.getElementById('glissandoVelEndValue').textContent = `${Math.round(value * 100)}%`;
    });

    // v1.25.0: Glissando time (0.01-0.5s, skew 0.5)
    bindSlider('glissandoTime', (value) => {
        const skew = 0.5;
        const minVal = 0.01;
        const maxVal = 0.5;
        const actual = minVal + Math.pow(value, 1/skew) * (maxVal - minVal);
        const ms = Math.round(actual * 1000);
        document.getElementById('glissandoTimeValue').textContent = `${ms} ms`;
    });

    // Choice parameters (dropdowns)
    bindChoice('stringMaterial');
    bindChoice('woodType');
    bindChoice('technique');
    bindChoice('glissandoScale');
    bindChoice('glissandoTonic');
    // v1.23.0: Interval and direction dropdowns (Scale-Locked)
    bindChoice('glissandoInterval');
    bindChoice('glissandoDirection');
    // v1.30.0: Free mode's own shape/interval/direction
    bindChoice('freeShape');
    bindChoice('freeInterval');
    bindChoice('freeDirection');
    // v1.30.0: Keyswitch note dropdowns
    bindKeyswitchChoice('freeKeyswitchNote');
    bindKeyswitchChoice('scaleKeyswitchNote');

    // v1.30.0: Free custom semitones slider
    bindSlider('freeCustomSemitones', (value) => {
        const st = Math.round(1 + value * 47);
        document.getElementById('freeCustomSemitonesValue').textContent = `${st} st`;
    });

    // v1.30.0: Glissando toggle buttons
    bindToggle('freeToggle', 'freeToggleBtn');
    bindToggle('scaleToggle', 'scaleToggleBtn');

    // v1.30.0: Custom semitones visibility per section
    setupCustomSemitonesVisibility('freeInterval', 'freeCustomSemitonesGroup');
    setupCustomSemitonesVisibility('glissandoInterval', 'glissandoCustomSemitonesGroup');

    // v1.31.0: Tempo sync dropdowns and visibility
    bindChoice('freeTempoSync');
    bindChoice('scaleTempoSync');
    setupTempoSyncVisibility('freeTempoSync', 'freeTimeGroup');
    setupTempoSyncVisibility('scaleTempoSync', 'scaleSpeedGroup');
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

/**
 * v1.30.0: Bind a bool toggle parameter to a button element
 * Uses getToggleState for JUCE 8 bool param bridge, falls back to getSliderState
 */
function bindToggle(paramId, buttonId) {
    const button = document.getElementById(buttonId);
    if (!button) {
        console.error(`Toggle button not found: ${buttonId}`);
        return;
    }

    try {
        // Try JUCE 8 toggle state first, fall back to slider state with 0/1 threshold
        let state;
        let getValue, setValue;

        if (typeof Juce.getToggleState === 'function') {
            state = Juce.getToggleState(paramId);
            getValue = () => state.getValue();
            setValue = (v) => state.setValue(v);
        } else {
            state = Juce.getSliderState(paramId);
            getValue = () => state.getNormalisedValue() >= 0.5;
            setValue = (v) => state.setNormalisedValue(v ? 1.0 : 0.0);
        }

        const updateButton = (isOn) => {
            button.textContent = isOn ? 'ON' : 'OFF';
            button.classList.toggle('active', isOn);
        };

        // Initialize
        updateButton(getValue());

        // UI → C++
        button.addEventListener('click', () => {
            const newVal = !getValue();
            setValue(newVal);
            updateButton(newVal);
        });

        // C++ → UI (automation, preset load)
        state.valueChangedEvent.addListener(() => {
            updateButton(getValue());
        });

        console.log(`Toggle bound: ${paramId} → ${buttonId}`);
    } catch (error) {
        console.error(`Failed to bind toggle ${paramId}:`, error);
    }
}

/**
 * v1.30.0: Bind keyswitch choice dropdown (populates 48 MIDI note options)
 */
function bindKeyswitchChoice(paramId) {
    const element = document.getElementById(paramId);
    if (!element) {
        console.error(`Keyswitch element not found: ${paramId}`);
        return;
    }

    // Populate options: MIDI notes 0-47 (C-1 through B2)
    const noteNames = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B'];
    element.innerHTML = '';
    for (let i = 0; i < 48; i++) {
        const noteName = noteNames[i % 12];
        const octave = Math.floor(i / 12) - 1; // MIDI 0 = C-1 (octave offset 4 in JUCE)
        const option = document.createElement('option');
        option.value = i;
        option.textContent = `${noteName}${octave}`;
        element.appendChild(option);
    }

    // Bind to APVTS via standard choice binding
    bindChoice(paramId);
}

/**
 * v1.30.0: Show/hide custom semitones group when interval dropdown = Custom (index 16)
 */
function setupCustomSemitonesVisibility(intervalId, customGroupId) {
    const intervalSelect = document.getElementById(intervalId);
    const customGroup = document.getElementById(customGroupId);

    if (!intervalSelect || !customGroup) return;

    const update = () => {
        customGroup.style.display = (intervalSelect.selectedIndex === 16) ? '' : 'none';
    };

    intervalSelect.addEventListener('change', update);

    // React to JUCE-side changes
    try {
        const intervalState = Juce.getComboBoxState(intervalId);
        if (intervalState && intervalState.valueChangedEvent) {
            intervalState.valueChangedEvent.addListener(() => {
                intervalSelect.selectedIndex = intervalState.getChoiceIndex();
                update();
            });
        }
    } catch (e) {
        console.error(`Failed to bind custom semitones visibility for ${intervalId}:`, e);
    }

    update();
}

/**
 * v1.31.0: Show/hide a manual control group when tempo sync is active.
 * When sync dropdown is not "Off" (index 0), hides the manual slider group.
 */
function setupTempoSyncVisibility(syncId, manualGroupId) {
    const syncSelect = document.getElementById(syncId);
    const manualGroup = document.getElementById(manualGroupId);
    if (!syncSelect || !manualGroup) return;

    const update = () => {
        manualGroup.style.display = (syncSelect.selectedIndex === 0) ? '' : 'none';
    };

    syncSelect.addEventListener('change', update);

    try {
        const syncState = Juce.getComboBoxState(syncId);
        if (syncState && syncState.valueChangedEvent) {
            syncState.valueChangedEvent.addListener(() => {
                syncSelect.selectedIndex = syncState.getChoiceIndex();
                update();
            });
        }
    } catch (e) {
        console.error(`Failed to bind tempo sync visibility for ${syncId}:`, e);
    }

    update();
}
