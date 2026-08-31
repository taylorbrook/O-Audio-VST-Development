/*
   This file is part of O-Lyrica, an Ouaricon Audio plugin.
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
/**
 * OuariconLyrica - WebView UI Controller
 * Binds all 19 APVTS parameters to HTML controls
 */

// CRITICAL: Import JUCE functions from the embedded bridge
import * as Juce from './juce/index.js';
import { LANGUAGES, I18N, LABELS, TIP_BINDINGS, tr } from './i18n.js';

// Wait for DOM to be fully loaded
document.addEventListener('DOMContentLoaded', () => {
    console.log('OuariconLyrica effects module loaded');

    // v2.4.0: the effects tab BUILDS its sixteen knobs, so it runs first — a
    // caption keyed by setLabel() before its element exists writes onto
    // nothing, and the language sweep below only sees elements that are in the
    // document when it runs.
    initializeEffects();

    // v2.4.0: the settings popover and the language sweep, in that order and
    // BOTH before initializeTooltips(). The gear and the hover-help switch are
    // themselves data-tip anchors, and applyI18n() is what puts the copy on
    // them: binding the renderer first would leave the two controls that reach
    // and restore the help layer with nothing to say on the first hover.
    try { initSettingsPopover(); } catch (e) { console.error('settings popover init failed:', e); }
    try { initI18n(); }           catch (e) { console.error('i18n init failed:', e); }

    initializeTooltips();
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

        // KEYS through setLabel, from an if/else — never a ternary inside the
        // call (check-i18n assertion 13), and never a literal, which would be
        // stranded in the previous language the instant the selector fired.
        //
        // THIS FUNCTION IS NOT REACHED. See the CHANGELOG: app.js's
        // initializeParameters / initializeMeters / bindSlider / bindChoice /
        // bindToggle / bindKeyswitchChoice / setupCustomSemitonesVisibility
        // have been dead since v1.35.1 moved the effects tab in here and left
        // the inline module owning every other binding. It is localized rather
        // than deleted because deleting ~350 lines of a physical-model synth's
        // controller is not a language commit's business, and localizing it
        // costs no new string: ui.onCaps / ui.offCaps are the same two keys the
        // LIVE copy in the inline module uses.
        const updateButton = (isOn) => {
            if (isOn) setLabel(button, 'ui.onCaps');
            else      setLabel(button, 'ui.offCaps');
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
// v2.4.0: NO innerHTML, and NO caption argument.
//
// Through v2.3.3 this built the whole knob — the vine-arc SVG and the caption —
// from one interpolated innerHTML string, so the caption was a raw prose write
// no language sweep could ever own. It is createElement / createElementNS
// throughout now, which also removes the last markup string in this file.
//
// THE CAPTION IS APPLIED BY THE CALLER, always with a PLAIN STRING LITERAL.
// A {id, labelKey} table passed through a loop reads better and fails twice:
// check-i18n assertion 13 rejects a computed setLabel key, and assertion 15
// counts only a literal as a live reference, so all sixteen captions would have
// reported DEAD while the gate simultaneously said the key was uncheckable.
const SVG_NS = 'http://www.w3.org/2000/svg';

function makeFxKnob(id) {
    const vb = 44;
    const c = 22;
    const r = 18;
    const da = (2 * Math.PI * r * 0.75).toFixed(2);

    const container = document.createElement('div');
    container.className = 'knob-container';

    const knob = document.createElement('div');
    knob.className = 'knob';
    knob.id = id + 'Knob';

    const visual = document.createElement('div');
    visual.className = 'knob-visual';

    const svg = document.createElementNS(SVG_NS, 'svg');
    svg.setAttribute('viewBox', `0 0 ${vb} ${vb}`);

    const track = document.createElementNS(SVG_NS, 'circle');
    track.setAttribute('class', 'knob-track');
    track.setAttribute('cx', c);
    track.setAttribute('cy', c);
    track.setAttribute('r', r);

    const vine = document.createElementNS(SVG_NS, 'circle');
    vine.setAttribute('class', 'knob-vine');
    vine.setAttribute('id', id + 'Vine');
    vine.setAttribute('cx', c);
    vine.setAttribute('cy', c);
    vine.setAttribute('r', r);
    vine.setAttribute('stroke-dasharray', da);
    vine.setAttribute('stroke-dashoffset', da);

    svg.appendChild(track);
    svg.appendChild(vine);
    visual.appendChild(svg);
    knob.appendChild(visual);
    container.appendChild(knob);

    const labelEl = document.createElement('div');
    labelEl.className = 'knob-label';
    container.appendChild(labelEl);

    const valueEl = document.createElement('div');
    valueEl.className = 'knob-value';
    valueEl.id = id + 'Value';
    container.appendChild(valueEl);

    return container;
}

// The caption element of a knob built above, so a call site can write its
// literal key onto it in one line.
function fxCaption(container) {
    return container.querySelector('.knob-label');
}

// Create a knob, append it, and hand its CAPTION back so the call site can
// key it: `setLabel(addFxKnob('chorus-knobs', 'chorusRate'), 'label.knRate')`.
// The key is then a plain string literal in setLabel's second argument, which
// is the only shape assertion 13 accepts and the only one assertion 15 counts
// as a live reference. Returns null when the row is missing; setLabel's own
// first line already guards that.
function addFxKnob(containerId, id) {
    const row = document.getElementById(containerId);
    if (!row) return null;
    const knob = makeFxKnob(id);
    row.appendChild(knob);
    return fxCaption(knob);
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
    // v2.4.1: was `valueDisplay.title = 'Double-click to edit'`. A native title
    // renders a second, untranslated OS tooltip beside the measure-then-pin
    // renderer, and §4 DELETES it rather than localizing it. Where the title is
    // an element's only help its text moves to data-i18n-aria. A literal
    // dataset.i18nAria write is also what assertion 15 collects as a reference.
    valueDisplay.dataset.i18nAria = 'aria.valueEdit';
    applyI18nAttributes(valueDisplay);
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

// v2.4.0: populateFxKnobs() is GONE. It took a table of {id, label} pairs and
// applied them in a loop, which is precisely the shape that cannot carry a
// literal i18n key — see makeFxKnob's note. Each knob is now created and
// captioned at its own call site in initializeEffects().

/**
 * Setup a bypass toggle button for an effects section
 */
function setupFxBypassToggle(fxName, toggleState) {
    const btn = document.getElementById(fxName + 'BypassBtn');
    const section = document.getElementById(fxName + 'Section');
    if (!btn || !section || !toggleState) return;

    function updateVisual() {
        const bypassed = toggleState.getValue();
        // Two literal keys from an if/else, per check-i18n assertion 13. NOT a
        // value mirror under D-01: chorusBypass / delayBypass / eqBypass /
        // reverbBypass are AudioParameterBool, so no automation lane ever
        // prints either word and translating them cannot make the page and the
        // host disagree.
        if (bypassed) setLabel(btn, 'ui.off');
        else          setLabel(btn, 'ui.on');
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
        // Every caption below is a PLAIN STRING LITERAL at its call site. See
        // makeFxKnob: a loop over a {id, labelKey} table fails assertion 13 on
        // the computed key and reports all sixteen keys dead in assertion 15.
        setLabel(addFxKnob('chorus-knobs', 'chorusRate'),  'label.knRate');
        setLabel(addFxKnob('chorus-knobs', 'chorusDepth'), 'label.knDepth');
        setLabel(addFxKnob('chorus-knobs', 'chorusMix'),   'label.knMix');

        // Delay - knobs + mode dropdown
        const delayRow = document.getElementById('delay-knobs');
        if (delayRow) {
            setLabel(addFxKnob('delay-knobs', 'delayTime'),     'label.time');
            setLabel(addFxKnob('delay-knobs', 'delayFeedback'), 'label.knFeedback');

            // Mode dropdown
            const modeWrap = document.createElement('div');
            modeWrap.className = 'fx-dropdown-container';
            const modeLbl = document.createElement('div');
            modeLbl.className = 'knob-label';
            setLabel(modeLbl, 'label.mode');
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

            setLabel(addFxKnob('delay-knobs', 'delayMix'), 'label.knMix');
        }

        // EQ
        setLabel(addFxKnob('eq-knobs', 'eqLowGain'),  'label.knLow');
        setLabel(addFxKnob('eq-knobs', 'eqMidGain'),  'label.knMid');
        setLabel(addFxKnob('eq-knobs', 'eqMidFreq'),  'label.knMidFreq');
        setLabel(addFxKnob('eq-knobs', 'eqHighGain'), 'label.knHigh');

        // Reverb (v2.1.0: FDN plate reverb with mod + shimmer)
        setLabel(addFxKnob('reverb-knobs', 'reverbSize'),     'label.size');
        setLabel(addFxKnob('reverb-knobs', 'reverbDamp'),     'label.knDamp');
        setLabel(addFxKnob('reverb-knobs', 'reverbPredelay'), 'label.knPredelay');
        setLabel(addFxKnob('reverb-knobs', 'reverbMod'),      'label.knMod');
        setLabel(addFxKnob('reverb-knobs', 'reverbShimmer'),  'label.knShimmer');
        setLabel(addFxKnob('reverb-knobs', 'reverbMix'),      'label.knMix');

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
        const reverbModState      = Juce.getSliderState('reverbMod');
        const reverbShimmerState  = Juce.getSliderState('reverbShimmer');
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
        setupFxKnob('reverbMod',       reverbModState,      0, 100, '%', v => Math.round(v));
        setupFxKnob('reverbShimmer',   reverbShimmerState,  0, 100, '%', v => Math.round(v));
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



// ═══════════════════════════════════════════════════════════════════════════
// Interface language (v2.4.0)
// ═══════════════════════════════════════════════════════════════════════════
//
// THIS BLOCK IS REPLICATED VERBATIM ACROSS EVERY LOCALIZED PLUGIN and is
// byte-compared (comments stripped, whitespace collapsed) against
// scripts/i18n-canon.js by scripts/check-i18n.js assertion 6. This repo has no
// shared UI module and deliberately does not gain one, so 43 hand-copies are
// only safe because a drifted copy fails a gate. Do not "tidy" it.
//
// One PULL at page init, no push, no timer, no poll().then(poll), no revision
// counter. The language is not preset content: OuariconPresetManager::loadPreset
// walks preset["parameters"] and never touches a state-tree property, so no
// preset path can change it. The pull is safe here because
// `grep -rn setVisible plugins/O-Lyrica/Source/` returns NOTHING — the web view
// is never hidden, so the hidden-completion drop cannot fire
// (critical_webview_completion_gated_on_isvisible).
//
// IT ALSO REPLACES A PUSH THAT WAS ALREADY HERE. v2.3.3 restored the tooltip
// preference by having the editor's timerCallback fire ONE evaluateJavascript
// at `window.restoreTooltipState` and never retry — the racy shape WR-01
// documented on O-FreqPulse, which loses the stored value outright on a cold
// WebView start where the page has not evaluated yet. Both preferences are
// PULLED now, and that hook and its timer branch are deleted.
//
// Declared here at module level, ABOVE every reader. The only statements
// executed at module-evaluation time are the two window.__ assignments, which
// touch hoisted function declarations and cannot enter a TDZ chain
// (pattern_module_toplevel_init_tdz). initI18n() itself is called from INSIDE
// the DOMContentLoaded handler, after initializeEffects().

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
            .then((code) => applyI18n(code === 'fr' ? 'fr' : 'en'))
            .catch((e) => console.warn('Could not read language preference:', e));
    }

    const sel = document.getElementById('lang-select');
    if (sel) sel.addEventListener('change', (e) => {
        applyI18n(e.target.value);
        if (setUiLanguageNative) setUiLanguageNative(uiLanguage).catch(() => {});
    });
}

// ═══════════════════════════════════════════════════════════════════════════
// The settings popover (v2.4.0)
// ═══════════════════════════════════════════════════════════════════════════
//
// The gear that carries the language selector and the hover-help switch. Two
// rows: this plugin HAS the setTooltipsEnabled bridge, so its toggle moves in
// here from the floating "?" rather than sitting beside a second control for the
// same state.
//
// IT SITS EXACTLY WHERE THE "?" SAT — bottom: 50px, right: 15px inside
// .plugin-container, the same absolute slot, so the new control adds ZERO
// geometry delta to a 700x450 frame that has none to spare. The header was the
// obvious alternative and was rejected for the opposite reason: .header is a
// justify-content: space-between row of three items, and a fourth would have
// moved the title, the preset bar and the voice readout for a control that
// belongs in a corner.
//
// The panel opens UPWARD (bottom-anchored), because 50px from the bottom edge
// is not enough room to drop a two-row panel downward.
//
// All state lives in this closure, so nothing here can join a TDZ chain.

let settingsPopoverEl = null;
let gearBtnEl = null;

function setSettingsPopoverOpen(open) {
    if (!settingsPopoverEl || !gearBtnEl) return;

    settingsPopoverEl.hidden = !open;
    gearBtnEl.setAttribute('aria-expanded', open ? 'true' : 'false');
}

function initSettingsPopover() {
    gearBtnEl = document.getElementById('gear-btn');
    settingsPopoverEl = document.getElementById('settings-popover');

    if (!gearBtnEl || !settingsPopoverEl) {
        console.warn('settings popover missing — language selector unavailable');
        return;
    }

    gearBtnEl.addEventListener('click', (e) => {
        e.stopPropagation();
        setSettingsPopoverOpen(settingsPopoverEl.hidden);
    });

    // Dismiss on a press anywhere else, and on Escape. mousedown rather than
    // click, so the panel is gone before a drag on a slider or a knob
    // underneath it begins — the effects knobs call preventDefault in their own
    // mousedown handlers. Matches how the preset dropdown already behaves.
    document.addEventListener('mousedown', (e) => {
        if (settingsPopoverEl.hidden) return;
        if (settingsPopoverEl.contains(e.target) || gearBtnEl.contains(e.target)) return;
        setSettingsPopoverOpen(false);
    });

    document.addEventListener('keydown', (e) => {
        if (e.key === 'Escape' && !settingsPopoverEl.hidden) {
            setSettingsPopoverOpen(false);
            gearBtnEl.focus();
        }
    });
}

// ═══════════════════════════════════════════════════════════════════════════
// Tooltips — the measure-then-pin renderer (v2.4.0)
// ═══════════════════════════════════════════════════════════════════════════
//
// PORTED from O-ReverseDelay / O-FreqPulse, replacing this plugin's own second
// positioner ENTIRELY. There is now ONE tooltip renderer repo-wide.
//
// What the port brings that v2.3.3's positioner did not have: a title/body pair
// built from data-tip-title + data-tip rather than one flat string, a dwell
// delay so a tip does not fire on every crossing, a width that is MEASURED and
// PINNED before `left` is applied rather than a hard-coded 200px fallback, an
// arrow whose offset is recomputed AFTER the horizontal clamp so a clamped tip
// still points at its control, delegated listeners on the DOCUMENT rather than
// on .plugin-container, and viewport-relative arithmetic that matches the
// fixed-position box the browser actually lays out.
//
// The four literals it deletes were: `tooltip.offsetWidth || 200`,
// `tooltip.offsetHeight || 40`, and the two containerRect-relative 10px
// margins. The fallbacks fired whenever the box had not been laid out yet,
// which on a first hover is exactly when they were read.
//
// The renderer never sees a KEY. applyI18n() writes both attributes from
// js/i18n.js and rewrites them on every language change; this function reads
// only what is on the anchor.

const TOOLTIP_DELAY_MS = 120;
const TOOLTIP_MARGIN = 8;   // gap between a tip and its control / the viewport edge

let tooltipEl = null;
let tooltipTimer = null;
let tooltipTarget = null;
let tooltipSuppressed = false;

// v1.18.0: master on/off for the hover-help layer, persisted C++-side.
// v2.4.0 moved its control out of the floating "?" and into the settings
// popover, next to the language selector.
//
// Starts FALSE, matching the C++ default (PluginProcessor.h: tooltipsEnabled),
// so the very first hover behaves the same whether or not the stored value has
// arrived yet — the native call below is a promise.
let tooltipsEnabled = false;
let helpToggleEl = null;
let setTooltipsEnabledNative = null;

function initializeTooltips() {
    tooltipEl = document.getElementById('tooltip');
    if (!tooltipEl) { console.warn('Tooltip element not found — tooltips disabled'); return; }

    initializeHelpToggle();

    document.addEventListener('mouseover', handleTooltipOver);
    document.addEventListener('mouseout', handleTooltipOut);

    // Any press begins a click or a drag: get the tip out of the way and keep it
    // away until release, so it cannot hang over a slider or a knob mid-drag.
    // Capture phase, because the effects knobs call preventDefault in their own
    // mousedown handlers.
    document.addEventListener('pointerdown', () => {
        tooltipSuppressed = true;
        hideTooltip();
    }, true);

    document.addEventListener('pointerup', () => { tooltipSuppressed = false; }, true);

    console.log('Tooltips initialized');
}

function initializeHelpToggle() {
    helpToggleEl = document.getElementById('tips-toggle');
    if (!helpToggleEl) { console.warn('Help toggle not found — hover help stays off'); return; }

    helpToggleEl.addEventListener('click', () => setTooltipsEnabled(!tooltipsEnabled, true));

    // Bridge to the processor. Guarded because the same page is opened in a
    // plain browser for UI checks, where native integration does not exist —
    // there the toggle still works, it just does not persist.
    //
    // getTooltipsEnabled was REMOVED from PluginEditor.cpp in v2.2.0 as
    // finding IN-14, on the grounds that the JS never called it: the state was
    // pushed instead, by one evaluateJavascript from timerCallback. That push
    // is the racy shape, so the getter is back and the push is gone.
    let getTooltipsEnabledNative = null;

    try {
        getTooltipsEnabledNative = Juce.getNativeFunction('getTooltipsEnabled');
        setTooltipsEnabledNative = Juce.getNativeFunction('setTooltipsEnabled');
    } catch (e) {
        console.warn('Tooltip preference not available, session-only:', e);
    }

    // Paint the current (default) state first so the button is never blank while
    // the native call is in flight.
    setTooltipsEnabled(tooltipsEnabled, false);

    if (getTooltipsEnabledNative) {
        getTooltipsEnabledNative()
            .then((stored) => setTooltipsEnabled(!!stored, false))
            .catch((e) => console.warn('Could not read tooltip preference:', e));
    }
}

// `persist` is false for the start-up push, so reading the stored value does not
// immediately write it back.
function setTooltipsEnabled(enabled, persist) {
    tooltipsEnabled = !!enabled;

    if (!tooltipsEnabled) hideTooltip();

    const pluginContainer = document.querySelector('.plugin-container');
    if (pluginContainer) pluginContainer.classList.toggle('tooltips-enabled', tooltipsEnabled);

    if (helpToggleEl) {
        // The two faces are KEYS through setLabel(), not literals. A literal
        // holds one string, so switching to French mid-session would have
        // restored an English "On". if/else, not a ternary inside the call —
        // check-i18n assertion 13.
        helpToggleEl.setAttribute('aria-pressed', tooltipsEnabled ? 'true' : 'false');
        if (tooltipsEnabled) setLabel(helpToggleEl, 'ui.on');
        else                 setLabel(helpToggleEl, 'ui.off');
    }

    if (persist && setTooltipsEnabledNative) {
        setTooltipsEnabledNative(tooltipsEnabled)
            .catch((e) => console.warn('Could not save tooltip preference:', e));
    }
}

// The gear and the toggle inside the popover both carry data-tip-always: the two
// controls that reach and restore the help layer have to keep explaining
// themselves while help is off.
function tipAllowed(target) {
    return tooltipsEnabled || target.hasAttribute('data-tip-always');
}

function handleTooltipOver(e) {
    const target = e.target.closest ? e.target.closest('[data-tip]') : null;
    if (!target || target === tooltipTarget) return;
    if (!tipAllowed(target)) return;

    tooltipTarget = target;
    clearTimeout(tooltipTimer);

    if (tooltipSuppressed) return;
    tooltipTimer = setTimeout(() => showTooltip(target), TOOLTIP_DELAY_MS);
}

function handleTooltipOut(e) {
    const target = e.target.closest ? e.target.closest('[data-tip]') : null;
    if (!target) return;

    // Moving between children of the same control is not a real exit. Every
    // .slider-group here wraps a caption, a slider and a value readout, and
    // crossing between those children previously flickered the surface off and
    // back on.
    if (e.relatedTarget && target.contains(e.relatedTarget)) return;

    hideTooltip();
}

function showTooltip(target) {
    // The pointer may have moved on or gone down during the delay, and help may
    // have been switched off between the hover and the timer firing.
    if (!tooltipEl || tooltipSuppressed || target !== tooltipTarget) return;
    if (!tipAllowed(target)) return;

    const title = target.getAttribute('data-tip-title');
    const body  = target.getAttribute('data-tip');

    // textContent, not innerHTML — the copy stays inert.
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

    const anchor = target.getBoundingClientRect();

    // MEASURE-THEN-PIN. A fixed-position box with `left` set and `width:auto`
    // shrinks to fit whatever space remains to its right, so measuring at the
    // PREVIOUS offset under-reports the width, and applying a near-edge `left`
    // afterwards re-wraps a 200 px tip into a narrow ribbon — and the squeezed
    // width then resolves `left` straight back against the right edge, so it
    // never recovers on later hovers. Release the width, measure from the left
    // edge, pin the result in px, and only then place.
    //
    // The pinned width is the FRACTIONAL getBoundingClientRect().width, not the
    // integer offsetWidth: 188.48 rounds to 188, and pinning that makes the box
    // 0.48 px narrower than its own shrink-to-fit, pushing the last word onto a
    // second line. Height is only stable once the width is definite, so it is
    // read after (pattern_fixed_tooltip_shrink_to_fit_edge).
    tooltipEl.style.width = '';
    tooltipEl.style.left  = '0px';
    tooltipEl.style.top   = '0px';

    const width = tooltipEl.getBoundingClientRect().width;
    tooltipEl.style.width = `${width}px`;

    const height = tooltipEl.getBoundingClientRect().height;

    // Prefer above; flip below only when there is no room at the top.
    let top = anchor.top - height - TOOLTIP_MARGIN;
    let placement = 'above';

    if (top < TOOLTIP_MARGIN) {
        top = anchor.bottom + TOOLTIP_MARGIN;
        placement = 'below';
    }

    // THE VERTICAL CLAMP the ported renderer does not carry. O-ReverseDelay's
    // anchors are all knob-sized, so `below` always fits there and the omission
    // is invisible; O-FreqPulse reproduced a real 15px overhang on a 376px
    // anchor and added this line.
    //
    // ON THIS PAGE IT IS NOT INDEPENDENTLY REPRODUCIBLE, and that is said rather
    // than dressed up: the tallest tip anchor here is .master-volume at 24px,
    // every one of the 46 fits `above` or `below` inside 450px, and reverting
    // this line alone leaves every gate green in both languages. It is ported
    // anyway because the point of this stage is ONE runtime repo-wide, and a
    // copy that silently differs from the others is the drift the canon exists
    // to stop.
    const maxTop = window.innerHeight - height - TOOLTIP_MARGIN;
    if (top > maxTop) top = Math.max(TOOLTIP_MARGIN, maxTop);

    const anchorCentreX = anchor.left + anchor.width / 2;
    const maxLeft = window.innerWidth - width - TOOLTIP_MARGIN;
    const left = Math.max(TOOLTIP_MARGIN, Math.min(maxLeft, anchorCentreX - width / 2));

    tooltipEl.style.left = `${left}px`;
    tooltipEl.style.top  = `${top}px`;
    tooltipEl.dataset.placement = placement;

    // The tip is clamped to the viewport, but the arrow still points at the
    // control — held clear of the rounded corners. Recomputed AFTER the clamp,
    // which is the whole reason the clamp can be this aggressive.
    const arrowX = Math.max(10, Math.min(width - 10, anchorCentreX - left));
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
