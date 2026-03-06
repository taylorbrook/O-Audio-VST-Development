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
    initializeEffects(); // v1.32.0
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

// ============================================================================
// v1.32.5: Effects Tab (SVG vine-arc knobs)
// ============================================================================

// Global knob drag state for effects knobs
const fxKnobDrag = { active: false, state: null, lastY: 0, virtualNorm: 0 };

document.addEventListener('mousemove', (e) => {
    if (!fxKnobDrag.active) return;
    const deltaY = fxKnobDrag.lastY - e.clientY;
    const sensitivity = 0.005;
    fxKnobDrag.virtualNorm = Math.max(0, Math.min(1, fxKnobDrag.virtualNorm + (deltaY * sensitivity)));
    fxKnobDrag.state.setNormalisedValue(fxKnobDrag.virtualNorm);
    fxKnobDrag.lastY = e.clientY;
});

document.addEventListener('mouseup', () => {
    if (fxKnobDrag.active) {
        fxKnobDrag.state.sliderDragEnded();
        fxKnobDrag.active = false;
        fxKnobDrag.state = null;
    }
});

/**
 * Create an SVG vine-arc knob element for the effects tab
 */
function makeFxKnob(id, label) {
    const vb = 44;
    const c = 22;
    const r = 18;
    const da = (2 * Math.PI * r * 0.75).toFixed(2);

    const container = document.createElement('div');
    container.className = 'knob-container';
    container.innerHTML =
        `<div class="knob" id="${id}Knob">` +
            `<div class="knob-visual">` +
                `<svg viewBox="0 0 ${vb} ${vb}">` +
                    `<circle class="knob-track" cx="${c}" cy="${c}" r="${r}"/>` +
                    `<circle class="knob-vine" id="${id}Vine" cx="${c}" cy="${c}" r="${r}" stroke-dasharray="${da}" stroke-dashoffset="${da}"/>` +
                `</svg>` +
            `</div>` +
        `</div>` +
        `<div class="knob-label">${label}</div>` +
        `<div class="knob-value" id="${id}Value"></div>`;
    return container;
}

/**
 * Bind an SVG knob to a JUCE slider state with drag, wheel, and double-click editing
 */
function setupFxKnob(id, sliderState, displayMin, displayMax, suffix, formatter) {
    if (!sliderState) {
        console.error(`Failed to get slider state for ${id}`);
        return;
    }

    const knobEl = document.getElementById(id + 'Knob');
    const vine = document.getElementById(id + 'Vine');
    const valueDisplay = document.getElementById(id + 'Value');

    if (!knobEl || !vine || !valueDisplay) return;

    const r = parseFloat(vine.getAttribute('r'));
    const arcLength = 2 * Math.PI * r * 0.75;
    let isEditing = false;

    function updateVisual() {
        const normValue = sliderState.getNormalisedValue();
        const realValue = displayMin + (normValue * (displayMax - displayMin));

        // Update SVG vine arc
        const offset = arcLength - (normValue * arcLength);
        vine.style.strokeDashoffset = offset;

        // Update value display (skip when user is editing)
        if (!isEditing) {
            valueDisplay.textContent = formatter(realValue) + suffix;
        }
    }

    // JUCE -> UI
    sliderState.valueChangedEvent.addListener(() => updateVisual());

    // UI -> JUCE (drag on knob-visual)
    const knobVisual = knobEl.querySelector('.knob-visual');
    (knobVisual || knobEl).addEventListener('mousedown', (e) => {
        fxKnobDrag.active = true;
        fxKnobDrag.state = sliderState;
        fxKnobDrag.lastY = e.clientY;
        fxKnobDrag.virtualNorm = sliderState.getNormalisedValue();
        sliderState.sliderDragStarted();
        e.preventDefault();
    });

    // Mouse wheel support
    (knobVisual || knobEl).addEventListener('wheel', (e) => {
        e.preventDefault();
        const currentNorm = sliderState.getNormalisedValue();
        const delta = e.deltaY < 0 ? 0.02 : -0.02;
        const newNorm = Math.max(0, Math.min(1, currentNorm + delta));
        sliderState.setNormalisedValue(newNorm);
    }, { passive: false });

    updateVisual();

    // Double-click to edit value
    valueDisplay.style.cursor = 'text';
    valueDisplay.title = 'Double-click to edit';
    valueDisplay.addEventListener('dblclick', (e) => {
        e.stopPropagation();
        if (isEditing) return;
        isEditing = true;

        const normValue = sliderState.getNormalisedValue();
        const realValue = displayMin + (normValue * (displayMax - displayMin));
        const formatted = formatter(realValue);

        const input = document.createElement('input');
        input.type = 'text';
        input.value = formatted;
        input.style.cssText = `
            width: 45px; text-align: center; font-size: 9px;
            color: #5C4033; background: rgba(255,248,220,0.9);
            border: 1px solid #8B7355; border-radius: 3px;
            padding: 1px 2px; outline: none; user-select: text;
            -webkit-user-select: text;
        `;

        valueDisplay.textContent = '';
        valueDisplay.appendChild(input);
        input.focus();
        input.select();

        function commitValue() {
            if (!isEditing) return;
            const rawVal = parseFloat(input.value);
            if (!isNaN(rawVal)) {
                const clamped = Math.max(displayMin, Math.min(displayMax, rawVal));
                const norm = (clamped - displayMin) / (displayMax - displayMin);
                sliderState.sliderDragStarted();
                sliderState.setNormalisedValue(Math.max(0, Math.min(1, norm)));
                sliderState.sliderDragEnded();
            }
            isEditing = false;
            updateVisual();
        }

        input.addEventListener('keydown', (ev) => {
            if (ev.key === 'Enter') { ev.preventDefault(); commitValue(); }
            else if (ev.key === 'Escape') { isEditing = false; updateVisual(); }
        });

        input.addEventListener('blur', () => {
            if (isEditing) commitValue();
        });
    });
}

/**
 * Populate knobs into a container
 */
function populateFxKnobs(containerId, knobs) {
    const container = document.getElementById(containerId);
    if (!container) return;
    knobs.forEach(k => container.appendChild(makeFxKnob(k.id, k.label)));
}

/**
 * Setup a bypass toggle button for an effects section
 */
function setupFxBypassToggle(fxName, toggleState) {
    const btn = document.getElementById(fxName + 'BypassBtn');
    const section = document.getElementById(fxName + 'Section');
    if (!btn || !section || !toggleState) return;

    function updateVisual() {
        const bypassed = toggleState.getValue();
        btn.textContent = bypassed ? 'Off' : 'On';
        btn.classList.toggle('bypassed', bypassed);
        section.classList.toggle('bypassed', bypassed);
    }

    toggleState.valueChangedEvent.addListener(() => {
        updateVisual();
    });

    btn.addEventListener('click', () => {
        toggleState.setValue(!toggleState.getValue());
    });

    updateVisual();
}

/**
 * Initialize all effects controls (v1.32.0)
 */
function initializeEffects() {
    try {
        // --- Populate knobs ---

        // Chorus
        populateFxKnobs('chorus-knobs', [
            { id: 'chorusRate', label: 'Rate' },
            { id: 'chorusDepth', label: 'Depth' },
            { id: 'chorusMix', label: 'Mix' },
        ]);

        // Delay - knobs + mode dropdown
        const delayRow = document.getElementById('delay-knobs');
        if (delayRow) {
            delayRow.appendChild(makeFxKnob('delayTime', 'Time'));
            delayRow.appendChild(makeFxKnob('delayFeedback', 'Feedback'));

            // Mode dropdown
            const modeWrap = document.createElement('div');
            modeWrap.className = 'fx-dropdown-container';
            const modeLbl = document.createElement('div');
            modeLbl.className = 'knob-label';
            modeLbl.textContent = 'Mode';
            const modeSel = document.createElement('select');
            modeSel.className = 'fx-dropdown';
            modeSel.id = 'delayModeSelect';
            ['Normal', 'PingPong'].forEach((name, i) => {
                const opt = document.createElement('option');
                opt.value = i;
                opt.textContent = name;
                modeSel.appendChild(opt);
            });
            modeWrap.appendChild(modeLbl);
            modeWrap.appendChild(modeSel);
            delayRow.appendChild(modeWrap);

            delayRow.appendChild(makeFxKnob('delayMix', 'Mix'));
        }

        // EQ
        populateFxKnobs('eq-knobs', [
            { id: 'eqLowGain', label: 'Low' },
            { id: 'eqMidGain', label: 'Mid' },
            { id: 'eqMidFreq', label: 'Mid Freq' },
            { id: 'eqHighGain', label: 'High' },
        ]);

        // Reverb
        populateFxKnobs('reverb-knobs', [
            { id: 'reverbSize', label: 'Size' },
            { id: 'reverbDamp', label: 'Damp' },
            { id: 'reverbPredelay', label: 'Pre-dly' },
            { id: 'reverbMix', label: 'Mix' },
        ]);

        // --- Get JUCE states ---
        const chorusRateState     = Juce.getSliderState('chorusRate');
        const chorusDepthState    = Juce.getSliderState('chorusDepth');
        const chorusMixState      = Juce.getSliderState('chorusMix');
        const delayTimeState      = Juce.getSliderState('delayTime');
        const delayFeedbackState  = Juce.getSliderState('delayFeedback');
        const delayModeState      = Juce.getComboBoxState('delayMode');
        const delayMixState       = Juce.getSliderState('delayMix');
        const eqLowGainState      = Juce.getSliderState('eqLowGain');
        const eqMidGainState      = Juce.getSliderState('eqMidGain');
        const eqMidFreqState      = Juce.getSliderState('eqMidFreq');
        const eqHighGainState     = Juce.getSliderState('eqHighGain');
        const reverbSizeState     = Juce.getSliderState('reverbSize');
        const reverbDampState     = Juce.getSliderState('reverbDamp');
        const reverbPredelayState = Juce.getSliderState('reverbPredelay');
        const reverbMixState      = Juce.getSliderState('reverbMix');
        const chorusBypassState   = Juce.getToggleState('chorusBypass');
        const delayBypassState    = Juce.getToggleState('delayBypass');
        const eqBypassState       = Juce.getToggleState('eqBypass');
        const reverbBypassState   = Juce.getToggleState('reverbBypass');

        // --- Setup knobs with display ranges ---
        setupFxKnob('chorusRate',      chorusRateState,     0.1, 10, ' Hz', v => v.toFixed(2));
        setupFxKnob('chorusDepth',     chorusDepthState,    0, 100, '%', v => Math.round(v));
        setupFxKnob('chorusMix',       chorusMixState,      0, 100, '%', v => Math.round(v));
        setupFxKnob('delayTime',       delayTimeState,      1, 2000, ' ms', v => Math.round(v));
        setupFxKnob('delayFeedback',   delayFeedbackState,  0, 95, '%', v => Math.round(v));
        setupFxKnob('delayMix',        delayMixState,       0, 100, '%', v => Math.round(v));
        setupFxKnob('eqLowGain',       eqLowGainState,      -12, 12, ' dB', v => v.toFixed(1));
        setupFxKnob('eqMidGain',       eqMidGainState,      -12, 12, ' dB', v => v.toFixed(1));
        setupFxKnob('eqMidFreq',       eqMidFreqState,      200, 8000, ' Hz', v => Math.round(v));
        setupFxKnob('eqHighGain',      eqHighGainState,     -12, 12, ' dB', v => v.toFixed(1));
        setupFxKnob('reverbSize',      reverbSizeState,     0, 100, '%', v => Math.round(v));
        setupFxKnob('reverbDamp',      reverbDampState,     0, 100, '%', v => Math.round(v));
        setupFxKnob('reverbPredelay',  reverbPredelayState, 0, 200, ' ms', v => Math.round(v));
        setupFxKnob('reverbMix',       reverbMixState,      0, 100, '%', v => Math.round(v));

        // --- Delay mode dropdown ---
        const modeSelect = document.getElementById('delayModeSelect');
        if (modeSelect && delayModeState) {
            modeSelect.selectedIndex = delayModeState.getChoiceIndex();
            modeSelect.addEventListener('change', (e) => {
                delayModeState.setChoiceIndex(e.target.selectedIndex);
            });
            delayModeState.valueChangedEvent.addListener(() => {
                modeSelect.selectedIndex = delayModeState.getChoiceIndex();
            });
        }

        // --- Bypass toggles ---
        setupFxBypassToggle('chorus', chorusBypassState);
        setupFxBypassToggle('delay', delayBypassState);
        setupFxBypassToggle('eq', eqBypassState);
        setupFxBypassToggle('reverb', reverbBypassState);

        console.log('Effects tab initialized (v1.32.0)');
    } catch (error) {
        console.error('Failed to initialize effects:', error);
    }
}


