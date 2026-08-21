/*
   This file is part of O-MultiBandCompressor, an Ouaricon Audio plugin.
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

// ========== v1.8.0: SEED-DIAL KNOB INFRASTRUCTURE ==========
//
// Everything in this block is declared ABOVE the initializeUI() call below on
// purpose. That call sits at module top level, so anything it reaches during
// binding must already be initialised — a `const` declared further down would
// still be in its temporal dead zone and would throw, taking every later
// initializer on the module with it.

// The stem sweeps 270 degrees, the house range across the suite.
const KNOB_MIN_DEG = -135;      // normalised 0.0
const KNOB_MAX_DEG = 135;       // normalised 1.0

// Vertical pixels for a full 0 -> 1 sweep. Drag is absolute from the press
// point rather than accumulated from movementY: an absolute mapping cannot
// drift away from the pointer over a long drag, and it survives the pointer
// briefly leaving the element.
const DRAG_TRAVEL_PX = 220;
const FINE_TRAVEL_PX = 1100;    // shift-drag, 5x finer
const NUDGE_STEP = 0.02;        // wheel / arrow-key increment

// Normalised defaults keyed by parameter id, fetched from the C++ APVTS at
// start-up. A hardcoded JS table would drift from the NormalisableRange the
// moment a range moved, and silently: only skewed parameters would land wrong.
// Alt-click is simply inert until this arrives rather than guessing a value.
const parameterDefaults = {};

// ---------- value-entry parsers ----------
// Inverse of the formatters further down: display text -> the parameter's
// SCALED value, in the units its NormalisableRange is declared in, or null to
// reject. Rejected text is discarded without ever writing the parameter.
//
// Display units and scaled units line up more closely here than in most of the
// suite: MIX and PEAK_RMS are declared 0..100 in C++, not 0..1, so a percent
// readout parses as a plain number and must NOT be divided by 100.

// First signed decimal in the string, or null if there isn't one.
const numOf = (text) => {
    const m = String(text).match(/-?\d*\.?\d+/);
    if (!m) return null;
    const v = parseFloat(m[0]);
    return isFinite(v) ? v : null;
};

// dB, ratios, ms and the 0..100 percent params: the number on screen already IS
// the scaled value. "4.0:1" yields 4.0, "-20 dB" yields -20.
const parseDirect = (t) => numOf(t);

// Hz readouts that switch to kHz above 1000 (see formatHz). A bare number is
// always Hz — the whole range is expressible in Hz, and on a knob reading
// "1.2 kHz" someone typing "440" means 440 Hz, not 440 kHz. Write "1.2k" for
// kHz. Both sidechain filters read "Off" at the bottom of their range.
const parseFreq = (t) => {
    const s = String(t);
    if (/off/i.test(s)) return 0;
    const v = numOf(s);
    if (v === null) return null;
    return /k/i.test(s) ? v * 1000 : v;
};

// Peak/RMS blend, declared 0..100 in C++, whose two rails read as words.
const parseBlend = (t) => {
    const s = String(t);
    if (/peak/i.test(s)) return 0;
    if (/rms/i.test(s) && numOf(s) === null) return 100;
    return numOf(s);
};

// Scaled (C++ units) -> normalised [0,1]. Mirrors JUCE's NormalisableRange but
// reads start/end/skew from the live properties the WebSliderRelay pushes up
// from C++, so the entry path cannot drift from the ranges even if one of the
// hand-written formatters below does.
//
// Clamping before the pow() is not cosmetic: a scaled value below `start` gives
// a negative base, and Math.pow(negative, fractional) is NaN. That NaN would
// ride straight through setNormalisedValue() into the parameter and stick.
function scaledToNorm(scaled, props) {
    const start = props.start;
    const end = props.end;
    const skew = props.skew > 0 ? props.skew : 1;
    if (!(end > start)) return 0;
    const t = Math.min(1, Math.max(0, (scaled - start) / (end - start)));
    return skew === 1 ? t : Math.pow(t, skew);
}

// Initialize immediately (JUCE module handles backend connection internally)
initializeUI();

function initializeUI() {
    console.log('Initializing parameter bindings');

    // Before the bindings: Alt-click reset reads this map. It is a promise, so
    // it lands after the first render either way — the knobs just have no reset
    // target for the first moment, which is why the handler tolerates a miss.
    initializeParameterDefaults();

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
    }, parseDirect);

    // Output Gain: -24 to +24 dB
    bindSlider('output-gain', 'OUTPUT_GAIN', (norm) => {
        const db = norm * 48.0 - 24.0;
        return db.toFixed(1) + ' dB';
    }, parseDirect);

    // Mix: 0-100%. The C++ range is 0..100, not 0..1, so the percent on screen
    // is already the scaled value and parseDirect is correct here.
    bindSlider('mix', 'MIX', (norm) => {
        const pct = norm * 100.0;
        return pct.toFixed(0) + '%';
    }, parseDirect);

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
    }, parseDirect);

    // Ratio: 1:1 to 20:1
    bindSlider(`${bandId}-ratio`, `${bandPrefix}_RATIO`, (norm) => {
        const ratio = 1.0 + norm * 19.0;
        return ratio.toFixed(1) + ':1';
    }, parseDirect);

    // Attack: 0.1 to 200 ms (WR-04: APVTS skew 0.3 → value = min + (max−min)·norm^(1/skew))
    bindSlider(`${bandId}-attack`, `${bandPrefix}_ATTACK`, (norm) => {
        const ms = 0.1 + (200 - 0.1) * Math.pow(norm, 1 / 0.3);
        return ms.toFixed(1) + ' ms';
    }, parseDirect);

    // Release: 10 to 2000 ms (WR-04: APVTS skew 0.3 → value = min + (max−min)·norm^(1/skew))
    bindSlider(`${bandId}-release`, `${bandPrefix}_RELEASE`, (norm) => {
        const ms = 10 + (2000 - 10) * Math.pow(norm, 1 / 0.3);
        return ms.toFixed(0) + ' ms';
    }, parseDirect);

    // Knee: 0 to 24 dB
    bindSlider(`${bandId}-knee`, `${bandPrefix}_KNEE`, (norm) => {
        const db = norm * 24.0;
        return db.toFixed(1) + ' dB';
    }, parseDirect);

    // Makeup: -12 to +24 dB
    bindSlider(`${bandId}-makeup`, `${bandPrefix}_MAKEUP`, (norm) => {
        const db = norm * 36.0 - 12.0;
        return db.toFixed(1) + ' dB';
    }, parseDirect);

    // Solo, Bypass, SC Listen: bool
    bindToggle(`${bandId}-solo`, `${bandPrefix}_SOLO`);
    bindToggle(`${bandId}-bypass`, `${bandPrefix}_BYPASS`);
    bindToggle(`${bandId}-sc-listen`, `${bandPrefix}_SC_LISTEN`);

    // v1.5.0: the detector-path controls. Until now these three had no on-screen
    // control and were only bound so automation and presets stayed in sync.
    //
    // They use bindScaledSlider rather than the hand-rolled formulas above: it reads
    // the real range and skew that C++ pushes over propertiesChanged, so nothing here
    // duplicates the NormalisableRange. (The six knobs above keep their existing
    // formulas — those readouts are correct and working, and rewriting them is a
    // separate change with its own regression surface.)

    // Peak/RMS: 0% = pure peak detection, 100% = pure RMS over a 10 ms window
    bindScaledSlider(`${bandId}-peak-rms`, `${bandPrefix}_PEAK_RMS`, (pct) => {
        if (pct <= 0.5) return 'Peak';
        if (pct >= 99.5) return 'RMS';
        return `${pct.toFixed(0)}% RMS`;
    }, parseBlend);

    // SC HPF: 0 Hz means the filter is off (see Compressor::updateSidechainFilters)
    bindScaledSlider(`${bandId}-sc-hpf`, `${bandPrefix}_SC_HPF`, (hz) =>
        hz < 1.0 ? 'Off' : formatHz(hz), parseFreq);

    // SC LPF: the DSP treats 0 and anything at or above 20 kHz as off.
    // parseFreq maps a typed "off" to 0, the bottom of the range — matching the
    // tooltip's "fully left is Off" rather than the equally-off top end.
    bindScaledSlider(`${bandId}-sc-lpf`, `${bandPrefix}_SC_LPF`, (hz) =>
        (hz < 1.0 || hz >= 20000.0) ? 'Off' : formatHz(hz), parseFreq);
}

function formatHz(hz) {
    return hz >= 1000 ? `${(hz / 1000).toFixed(hz >= 10000 ? 0 : 1)} kHz`
                      : `${hz.toFixed(0)} Hz`;
}

// ========== BINDING HELPERS ==========

// v1.8.0: the dials were <input type="range"> circles through v1.7.0. Both
// engines' slider thumbs were sized to 0x0, so the control itself never showed
// its own position and dragging was horizontal, which is not how any other
// plugin in the suite behaves. They are now the house seed-cross-section knob:
// a <div class="knob"> whose child .knob-stem is rotated to the value.

// Points the stem at `norm`. Kept in one place because the three sizes differ
// only in stem LENGTH (set in CSS), never in the angle.
function setKnobStem(element, norm) {
    const stem = element.querySelector('.knob-stem');
    if (!stem) return;
    const deg = KNOB_MIN_DEG + norm * (KNOB_MAX_DEG - KNOB_MIN_DEG);
    stem.style.transform = `translate(-50%, -100%) rotate(${deg}deg)`;
}

// A bare <div role="slider"> has no accessible name of its own, so it borrows
// the caption beside it: band knobs sit in a .knob-control next to .knob-label,
// the three footer knobs in a .control-group under .control-label.
function labelKnob(element) {
    if (element.hasAttribute('aria-label')) return;
    const cell = element.closest('.knob-control') || element.closest('.control-group');
    const label = cell ? cell.querySelector('.knob-label, .control-label') : null;
    const text = label && label.textContent ? label.textContent.trim() : '';
    if (text) element.setAttribute('aria-label', text);
    element.setAttribute('aria-valuemin', '0');
    element.setAttribute('aria-valuemax', '1');
}

// Drag / wheel / keyboard / Alt-click / double-click, shared by both binders.
// `render` redraws stem, ARIA and readout from live state; `parseScaled` may be
// undefined, which simply leaves that knob without value entry.
function attachKnobBehaviour(element, sliderState, render, valueDisplay, parseScaled, parameterId) {
    let dragging = false;
    let startY = 0;
    let startNorm = 0;

    const setNorm = (n) => {
        sliderState.setNormalisedValue(Math.min(1, Math.max(0, n)));
        render();
    };

    // Wheel and arrow keys are discrete edits, not drags, but the host still
    // wants a gesture around each one or the change is not recorded as
    // automation.
    const nudge = (delta) => {
        sliderState.sliderDragStarted();
        setNorm(sliderState.getNormalisedValue() + delta);
        sliderState.sliderDragEnded();
    };

    const onMove = (e) => {
        if (!dragging) return;
        const travel = e.shiftKey ? FINE_TRAVEL_PX : DRAG_TRAVEL_PX;
        setNorm(startNorm + (startY - e.clientY) / travel);
        e.preventDefault();
    };

    // Idempotent, and bound to all four termination paths. A drag released
    // outside the window, or interrupted by the host stealing the pointer,
    // must still close the gesture — otherwise the parameter stays latched in
    // a "being dragged" state and the host keeps writing automation for it.
    const onUp = (e) => {
        if (!dragging) return;
        dragging = false;
        sliderState.sliderDragEnded();
        element.removeEventListener('pointermove', onMove);
        element.removeEventListener('pointerup', onUp);
        element.removeEventListener('pointercancel', onUp);
        element.removeEventListener('lostpointercapture', onUp);
        if (e && e.pointerId !== undefined) {
            try { element.releasePointerCapture(e.pointerId); } catch (_) { /* already released */ }
        }
    };

    element.addEventListener('pointerdown', (e) => {
        // Alt/Option-click resets to the C++ default instead of starting a drag.
        if (e.altKey) {
            const def = parameterDefaults[parameterId];
            if (def !== undefined) {
                sliderState.sliderDragStarted();
                sliderState.setNormalisedValue(def);
                sliderState.sliderDragEnded();
                render();
            }
            e.preventDefault();
            return;
        }

        dragging = true;
        startY = e.clientY;
        startNorm = sliderState.getNormalisedValue();
        sliderState.sliderDragStarted();
        try { element.setPointerCapture(e.pointerId); } catch (_) { /* older backends */ }
        element.addEventListener('pointermove', onMove);
        element.addEventListener('pointerup', onUp);
        element.addEventListener('pointercancel', onUp);
        element.addEventListener('lostpointercapture', onUp);
        e.preventDefault();
    });

    element.addEventListener('wheel', (e) => {
        nudge(e.deltaY < 0 ? NUDGE_STEP : -NUDGE_STEP);
        e.preventDefault();
    }, { passive: false });

    element.addEventListener('keydown', (e) => {
        let delta = 0;
        if (e.key === 'ArrowUp' || e.key === 'ArrowRight') delta = NUDGE_STEP;
        else if (e.key === 'ArrowDown' || e.key === 'ArrowLeft') delta = -NUDGE_STEP;
        else return;
        nudge(e.shiftKey ? delta / 5 : delta);
        e.preventDefault();
    });

    // Double-click opens inline numeric entry; Alt-click already reset above,
    // so an Alt-double-click must not also open the field.
    element.addEventListener('dblclick', (e) => {
        if (e.altKey) return;
        attachValueEntry(valueDisplay, sliderState, render, parseScaled);
    });
}

// Opens an inline editor over a value readout. Enter or blur commits, Esc
// cancels, and unparseable input is discarded without writing the parameter.
function attachValueEntry(valueEl, sliderState, render, parseScaled) {
    if (!valueEl || !sliderState || typeof parseScaled !== 'function') return;
    if (valueEl.dataset.editing === '1') return;   // already open

    const currentText = valueEl.textContent;
    const prevPosition = valueEl.style.position;
    const prevColor = valueEl.style.color;

    // Read the readout's ink BEFORE hiding it. The band readouts and the footer
    // ones sit on different backgrounds, so the field takes its colour from
    // whatever it replaces; `inherit` would resolve to transparent once the
    // span's own colour is cleared below.
    const inkColor = getComputedStyle(valueEl).color;

    const input = document.createElement('input');
    input.type = 'text';
    input.className = 'knob-value-input';
    input.value = currentText;
    input.spellcheck = false;
    input.autocomplete = 'off';
    input.style.color = inkColor;

    // The span keeps its own text, rendered transparent, so its box stays
    // exactly the size it was and the knob column does not shift. Clearing the
    // text instead would collapse it to zero height.
    valueEl.dataset.editing = '1';
    valueEl.style.position = 'relative';
    valueEl.style.color = 'transparent';
    valueEl.appendChild(input);
    input.focus();
    input.select();

    let closed = false;

    const close = () => {
        if (closed) return;
        closed = true;
        delete valueEl.dataset.editing;
        input.remove();
        valueEl.style.position = prevPosition;
        valueEl.style.color = prevColor;
        // Re-render from live state: the parameter may have moved under host
        // automation or a preset load while readout updates were suppressed.
        render();
    };

    const commit = () => {
        if (closed) return;
        // Untouched text is never written back. Readouts are rounded for
        // display — Release shows whole ms, so 104.6 reads "105" — and without
        // this guard, merely opening the field and pressing Enter would
        // quantise the parameter to whatever the readout had rounded to.
        if (input.value === currentText) { close(); return; }
        const scaled = parseScaled(input.value, currentText);
        if (scaled !== null && isFinite(scaled)) {
            // setNormalisedValue() clamps and snaps to `interval` itself, so
            // only the scaled -> norm conversion needs guarding here.
            sliderState.sliderDragStarted();
            sliderState.setNormalisedValue(scaledToNorm(scaled, sliderState.properties || {}));
            sliderState.sliderDragEnded();
        }
        close();
    };

    input.addEventListener('keydown', (e) => {
        // Keep digits and arrows away from the knob's own key handler while the
        // field is open.
        e.stopPropagation();
        if (e.key === 'Enter') { e.preventDefault(); commit(); }
        else if (e.key === 'Escape') { e.preventDefault(); close(); }
    });
    input.addEventListener('blur', commit);
    // The readout is a sibling of the knob, not a child, so these are belt and
    // braces — but a stray pointerdown reaching the knob would start a drag
    // underneath the open field.
    input.addEventListener('pointerdown', (e) => e.stopPropagation());
    input.addEventListener('mousedown', (e) => e.stopPropagation());
    input.addEventListener('dblclick', (e) => e.stopPropagation());
    input.addEventListener('wheel', (e) => e.stopPropagation());
}

// Pulls every parameter's default, normalised, from the C++ APVTS. Guarded
// because the same page is opened in a plain browser for UI checks, where
// native integration does not exist — there Alt-click is simply inert.
function initializeParameterDefaults() {
    let getDefaults = null;

    try {
        getDefaults = Juce.getNativeFunction('getParameterDefaults');
    } catch (e) {
        console.warn('Parameter defaults not available, Alt-click reset disabled:', e);
        return;
    }

    getDefaults()
        .then((defaults) => {
            if (!defaults || typeof defaults !== 'object') return;
            Object.keys(defaults).forEach((id) => {
                const v = Number(defaults[id]);
                if (isFinite(v)) parameterDefaults[id] = Math.min(1, Math.max(0, v));
            });
            console.log(`Parameter defaults loaded: ${Object.keys(parameterDefaults).length}`);
        })
        .catch((e) => console.warn('Could not read parameter defaults:', e));
}

// `formatValue` receives the NORMALISED value. `parseScaled` is the inverse for
// double-click entry and converts display text to the parameter's scaled units.
function bindSlider(elementId, parameterId, formatValue, parseScaled) {
    const element = document.getElementById(elementId);
    const valueDisplay = document.getElementById(`${elementId}-value`);

    if (!element) {
        console.warn(`Knob element not found: ${elementId}`);
        return;
    }

    try {
        // Get slider state from JUCE module
        const sliderState = Juce.getSliderState(parameterId);
        parameterStates[parameterId] = sliderState;

        const render = () => {
            const norm = sliderState.getNormalisedValue();
            setKnobStem(element, norm);
            element.setAttribute('aria-valuenow', norm.toFixed(3));

            if (!formatValue) return;
            const text = formatValue(norm);
            element.setAttribute('aria-valuetext', text);
            // Suppressed while an entry field is open — writing textContent
            // would wipe out the <input> living inside this span.
            if (valueDisplay && valueDisplay.dataset.editing !== '1') {
                valueDisplay.textContent = text;
            }
        };

        labelKnob(element);
        render();
        attachKnobBehaviour(element, sliderState, render, valueDisplay, parseScaled, parameterId);

        // Update UI when parameter changes (automation, preset load)
        sliderState.valueChangedEvent.addListener(render);

        console.log(`Bound knob: ${parameterId} → #${elementId}`);
    } catch (e) {
        console.error(`Failed to bind knob ${parameterId}:`, e);
    }
}

// v1.5.0: like bindSlider, but the readout is formatted from the parameter's real
// engineering value (SliderState.getScaledValue()) instead of a range restated in JS.
// The range and skew arrive from C++ via propertiesChanged, which lands *after* the
// first render — hence the propertiesChangedEvent listener, without which every
// readout would sit at its start-of-range value until the knob is first touched.
function bindScaledSlider(elementId, parameterId, formatScaled, parseScaled) {
    const element = document.getElementById(elementId);
    const valueDisplay = document.getElementById(`${elementId}-value`);

    if (!element) {
        console.warn(`Knob element not found: ${elementId}`);
        return;
    }

    try {
        const sliderState = Juce.getSliderState(parameterId);
        parameterStates[parameterId] = sliderState;

        const render = () => {
            const norm = sliderState.getNormalisedValue();
            setKnobStem(element, norm);
            element.setAttribute('aria-valuenow', norm.toFixed(3));

            if (!formatScaled) return;
            // getScaledValue() reflects the value actually held by the
            // parameter, so read it back rather than converting norm here.
            const text = formatScaled(sliderState.getScaledValue());
            element.setAttribute('aria-valuetext', text);
            if (valueDisplay && valueDisplay.dataset.editing !== '1') {
                valueDisplay.textContent = text;
            }
        };

        labelKnob(element);
        render();
        attachKnobBehaviour(element, sliderState, render, valueDisplay, parseScaled, parameterId);

        // Automation and preset loads
        sliderState.valueChangedEvent.addListener(render);

        // First arrival of the real range/skew from C++
        sliderState.propertiesChangedEvent.addListener(render);

        console.log(`Bound scaled knob: ${parameterId} → #${elementId}`);
    } catch (e) {
        console.error(`Failed to bind knob ${parameterId}:`, e);
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

// v1.4.2: a toggle that carries its own name in data-label keeps that name and
// shows its state through the .active fill alone. Before this, every band button
// was relabelled "On"/"Off" on bind, so a band's three buttons all read the same
// word and nothing on screen said which was solo, bypass or sidechain listen.
// The global Auto-MU toggle has no data-label — it sits beside its own caption,
// so On/Off is the informative thing to show there and it is left as it was.
// aria-pressed carries the state for screen readers now that colour alone does.
function updateToggleUI(element, value) {
    element.classList.toggle('active', Boolean(value));
    element.setAttribute('aria-pressed', value ? 'true' : 'false');

    const label = element.dataset.label;
    element.textContent = label ? label : (value ? 'On' : 'Off');
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
     'Monitor the detector signal driving this band’s compressor, including its sidechain filtering. This is what the compressor "hears", not what it outputs.'],

    // v1.5.0: detector-path controls
    ['peak-rms', '.knob-control', 'Peak / RMS',
     'Blends how the band’s level is measured. Peak reacts to individual transients and suits de-essing and plosive control; RMS averages over 10 ms and suits glue and level-riding.'],

    ['sc-hpf', '.knob-control', 'Sidechain High-Pass',
     'High-passes the detector only — the audio itself is untouched. Keeps low energy from triggering gain reduction, for example so subsonic rumble does not duck a whole band. Fully left is Off.'],

    ['sc-lpf', '.knob-control', 'Sidechain Low-Pass',
     'Low-passes the detector only — the audio itself is untouched. Narrows what the band responds to, for example keeping cymbals and air from holding a de-esser down. Fully left is Off.']
];

let tooltipEl = null;
let tooltipTimer = null;
let tooltipTarget = null;
let tooltipSuppressed = false;

// v1.4.1: master on/off for the hover-help layer, driven by the "?" button in
// the header and persisted C++-side in a machine-wide preference file. Starts
// true so the very first hover behaves like v1.4.0 even if the stored value
// arrives a moment later (the native call below is a promise).
let tooltipsEnabled = true;
let helpToggleEl = null;
let setTooltipsEnabledNative = null;

function initializeTooltips() {
    tooltipEl = document.getElementById('tooltip');
    if (!tooltipEl) {
        console.warn('Tooltip element not found - tooltips disabled');
        return;
    }

    GLOBAL_TOOLTIPS.forEach(([selector, title, body, wrapper]) =>
        applyTooltip(selector, title, body, wrapper));

    Object.keys(BAND_LABELS).forEach(applyBandTooltips);

    initializeHelpToggle();

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

// ---------- v1.4.1: the "?" toggle ----------

function initializeHelpToggle() {
    helpToggleEl = document.getElementById('help-toggle');
    if (!helpToggleEl) {
        console.warn('Help toggle not found - tooltips stay permanently on');
        return;
    }

    helpToggleEl.addEventListener('click', () => setTooltipsEnabled(!tooltipsEnabled, true));

    // Bridge to the preference file. Guarded because the same page is opened in
    // a plain browser for UI checks, where native integration does not exist —
    // there the toggle still works, it just does not persist.
    let getTooltipsEnabledNative = null;

    try {
        getTooltipsEnabledNative = Juce.getNativeFunction('getTooltipsEnabled');
        setTooltipsEnabledNative = Juce.getNativeFunction('setTooltipsEnabled');
    } catch (e) {
        console.warn('Tooltip preference not available, using session-only state:', e);
    }

    // Paint the current (default) state first so the button is never blank
    // while the native call is in flight.
    setTooltipsEnabled(tooltipsEnabled, false);

    if (getTooltipsEnabledNative) {
        getTooltipsEnabledNative()
            .then((stored) => setTooltipsEnabled(stored !== false, false))
            .catch((e) => console.warn('Could not read tooltip preference:', e));
    }
}

// `persist` is false for the start-up push, so reading the stored value does
// not immediately write it back.
function setTooltipsEnabled(enabled, persist) {
    tooltipsEnabled = !!enabled;

    if (!tooltipsEnabled) hideTooltip();

    if (helpToggleEl) {
        helpToggleEl.setAttribute('aria-pressed', tooltipsEnabled ? 'true' : 'false');
        helpToggleEl.setAttribute('data-tip-title', 'Hover Help');
        helpToggleEl.setAttribute('data-tip', tooltipsEnabled
            ? 'Hover help is on — every control describes itself and states its range. Click to turn the tips off.'
            : 'Hover help is off. Click to turn the tips back on.');
    }

    if (persist && setTooltipsEnabledNative) {
        setTooltipsEnabledNative(tooltipsEnabled)
            .catch((e) => console.warn('Could not save tooltip preference:', e));
    }

    // Re-show the button's own tip straight after a click so the new state is
    // confirmed in place. mousedown suppression has already lifted by now, and
    // the pointer is still over the button, so no fresh mouseover would fire.
    if (persist && helpToggleEl) {
        tooltipTarget = helpToggleEl;
        showTooltip(helpToggleEl);
    }
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

// The "?" button carries data-tip-always: the control that turns help back on
// has to keep explaining itself while help is off.
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

    // Moving between children of the same control is not a real exit
    if (e.relatedTarget && target.contains(e.relatedTarget)) return;

    hideTooltip();
}

function showTooltip(target) {
    // The pointer may have moved on or gone down during the delay, and help may
    // have been switched off between the hover and the timer firing.
    if (!tooltipEl || tooltipSuppressed || target !== tooltipTarget) return;
    if (!tipAllowed(target)) return;

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

    // v1.4.1: measure from the left edge with the width released, then pin the
    // measured width in px before placing. A fixed-position box with `left` set
    // and `width:auto` shrinks to fit whatever space is left to its right, so
    // measuring at the previous offset under-reports the width, and applying a
    // near-the-edge `left` afterwards re-wraps the copy into a narrow ribbon.
    // The header "?" button, as the right-most control, is what exposes this.
    tooltipEl.style.width = '';
    tooltipEl.style.left = '0px';
    tooltipEl.style.top = '0px';

    const width = tooltipEl.getBoundingClientRect().width;
    tooltipEl.style.width = `${width}px`;

    // Height is only stable once the width is definite
    const height = tooltipEl.getBoundingClientRect().height;

    // Prefer above; flip below only when there is no room at the top
    let top = anchor.top - height - TOOLTIP_MARGIN;
    let placement = 'above';

    if (top < TOOLTIP_MARGIN) {
        top = anchor.bottom + TOOLTIP_MARGIN;
        placement = 'below';
    }

    const anchorCentreX = anchor.left + anchor.width / 2;
    const maxLeft = window.innerWidth - width - TOOLTIP_MARGIN;
    const left = Math.max(TOOLTIP_MARGIN, Math.min(maxLeft, anchorCentreX - width / 2));

    tooltipEl.style.left = `${left}px`;
    tooltipEl.style.top = `${top}px`;
    tooltipEl.dataset.placement = placement;

    // The tip is clamped to the viewport but the arrow still points at the control,
    // held clear of the rounded corners.
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

// ========== v1.5.0: PRESET BAR ==========

let presetMgr = null;
let presetDropdownEl = null;

// v1.7.0: category name -> its presets, in the order the browser shows them, plus the
// flat walk of the same thing that the prev/next buttons step through. Both are rebuilt
// from one getPresetCategories() native call whenever the preset list changes.
//
// Presets in the "User" category are the deletable ones. Before v1.7.0 that answer came
// from isFactoryPreset(), one native round trip per name on every list change; the
// category map carries the same information in a single call, because a name absent
// from the C++ factory table is by definition a user preset.
const USER_CATEGORY = 'User';

let presetGroups = [];              // [{ category, names: [...] }], display order, non-empty only
let presetWalkOrder = [];           // presetGroups flattened — the ◀ / ▶ sequence

let getPresetCategoriesFn = null;

async function refreshPresetCategories() {
    if (!presetMgr) return;

    const list = presetMgr.getPresetList();
    let order = [];
    let map = {};

    try {
        const result = await getPresetCategoriesFn();
        order = (result && result.order) || [];
        map = (result && result.categories) || {};
    } catch (e) {
        // Fall back to one ungrouped list rather than losing the browser entirely. The
        // fallback category is "User" so every preset keeps its delete button: the
        // alternative — assuming factory — would strip the user's ability to remove
        // their own presets, which is worse than offering a delete that then fails.
        console.warn('Could not load preset categories, falling back to a flat list:', e);
        order = [USER_CATEGORY];
        map = {};
    }

    // Own-property lookup, not map[name]: `categories` arrives as a plain object whose
    // keys are preset names, and a user is free to save one called "constructor" or
    // "toString" — a bare index would hand back the Object.prototype member instead of
    // a category string and drop that preset out of every group.
    const categoryOf = (name) =>
        (Object.prototype.hasOwnProperty.call(map, name) ? map[name] : null) || USER_CATEGORY;

    const presetCategories = new Map(list.map((name) => [name, categoryOf(name)]));

    // The C++ order array is authoritative — do not re-sort it. Within a category the
    // names keep getPresetList()'s alphabetical order. Empty categories are dropped so
    // "User" does not show as a bare heading before anything has been saved.
    presetGroups = order
        .map((category) => ({
            category,
            names: list.filter((name) => presetCategories.get(name) === category)
        }))
        .filter((group) => group.names.length > 0);

    // Any category C++ did not list (only reachable if the two tables drift) would
    // otherwise vanish from the browser. Append the leftovers rather than hide them.
    const grouped = new Set(presetGroups.flatMap((g) => g.names));
    const orphans = list.filter((name) => !grouped.has(name));
    if (orphans.length > 0) {
        console.warn('Presets with an unlisted category:', orphans);
        presetGroups.push({ category: 'Other', names: orphans });
    }

    presetWalkOrder = presetGroups.flatMap((group) => group.names);
}

// v1.7.0: prev/next are driven from presetWalkOrder rather than from the C++
// selectNextPreset/selectPreviousPreset, which walk getPresetList()'s flat alphabetical
// order. With the dropdown grouped by category those two orders no longer agree, and
// the buttons have to follow what the user can see — otherwise ▶ from the last
// Mastering preset lands somewhere in the middle of Instruments.
async function stepPreset(delta) {
    if (!presetMgr || presetWalkOrder.length === 0) return;

    const current = presetMgr.getCurrentPreset();
    const index = presetWalkOrder.indexOf(current);

    // A preset loaded from an arbitrary file is not in the list; start from the top
    // going forward, and from the bottom going backward.
    const base = index >= 0 ? index : (delta > 0 ? -1 : 0);
    const next = (base + delta + presetWalkOrder.length) % presetWalkOrder.length;

    await presetMgr.loadPreset(presetWalkOrder[next]);
}

async function initializePresets() {
    const nameDisplay = document.getElementById('presetName');
    presetDropdownEl = document.getElementById('presetDropdown');

    if (!nameDisplay || !presetDropdownEl) {
        console.warn('Preset bar elements not found - presets disabled');
        return;
    }

    const { PresetManager } = await import('../modules/preset-manager.js');

    getPresetCategoriesFn = Juce.getNativeFunction('getPresetCategories');

    presetMgr = new PresetManager({
        displayElement: nameDisplay,
        // prevButton / nextButton are deliberately NOT handed to the module: its
        // selectPrevious/selectNext call through to the C++ walk of the flat
        // alphabetical list, and v1.7.0 needs the grouped order instead. They are
        // bound to stepPreset() below.
        saveButton: document.getElementById('savePreset'),
        loadButton: document.getElementById('loadPreset'),
        getNativeFunction: Juce.getNativeFunction,
        // window.confirm() is a silent no-op in some JUCE WebView backends, so the
        // module is handed an in-DOM confirmation instead of falling back to it.
        onConfirmDelete: confirmDeletePreset,
        onPresetListUpdated: () => {
            refreshPresetCategories().then(() => {
                if (presetDropdownEl.classList.contains('show')) renderPresetDropdown();
            });
        }
    });

    await presetMgr.initialize();
    await refreshPresetCategories();

    const prevButton = document.getElementById('prevPreset');
    const nextButton = document.getElementById('nextPreset');
    if (prevButton) prevButton.addEventListener('click', () => stepPreset(-1));
    if (nextButton) nextButton.addEventListener('click', () => stepPreset(1));

    nameDisplay.addEventListener('click', (e) => {
        e.stopPropagation();
        if (presetDropdownEl.classList.contains('show')) hidePresetDropdown();
        else showPresetDropdown();
    });

    nameDisplay.addEventListener('keydown', (e) => {
        if (e.key === 'Enter' || e.key === ' ') {
            e.preventDefault();
            nameDisplay.click();
        } else if (e.key === 'Escape') {
            hidePresetDropdown();
        }
    });

    // Any click outside the bar closes the list.
    document.addEventListener('click', (e) => {
        const bar = document.getElementById('presetBar');
        if (bar && !bar.contains(e.target)) hidePresetDropdown();
    });

    console.log('Preset bar initialized');
}

function showPresetDropdown() {
    renderPresetDropdown();
    presetDropdownEl.classList.add('show');
    const nameDisplay = document.getElementById('presetName');
    if (nameDisplay) nameDisplay.setAttribute('aria-expanded', 'true');
}

function hidePresetDropdown() {
    if (!presetDropdownEl) return;
    presetDropdownEl.classList.remove('show');
    const nameDisplay = document.getElementById('presetName');
    if (nameDisplay) nameDisplay.setAttribute('aria-expanded', 'false');
}

function renderPresetDropdown() {
    presetDropdownEl.innerHTML = '';

    const current = presetMgr ? presetMgr.getCurrentPreset() : '';

    if (presetGroups.length === 0) {
        const empty = document.createElement('div');
        empty.className = 'preset-dropdown-item empty';
        empty.textContent = 'No presets';
        presetDropdownEl.appendChild(empty);
        return;
    }

    // v1.7.0: grouped under category headings, in the order C++ reports, with the
    // ◀ / ▶ buttons walking the same sequence (see stepPreset). Within a group the
    // names stay in getPresetList()'s alphabetical order.
    //
    // The listbox owns the options; the headings are presentational only. Marking them
    // role="presentation" keeps a screen reader from counting them as selectable
    // entries, and each group is wrapped so the heading and its items stay associated.
    presetGroups.forEach((group) => {
        const groupEl = document.createElement('div');
        groupEl.className = 'preset-group';
        groupEl.setAttribute('role', 'group');
        groupEl.setAttribute('aria-label', group.category);

        const heading = document.createElement('div');
        heading.className = 'preset-group-heading';
        heading.setAttribute('role', 'presentation');
        heading.textContent = group.category;
        groupEl.appendChild(heading);

        group.names.forEach((name) => {
            const item = document.createElement('div');
            item.className = 'preset-dropdown-item' + (name === current ? ' active' : '');
            item.setAttribute('role', 'option');
            item.setAttribute('aria-selected', name === current ? 'true' : 'false');

            const label = document.createElement('span');
            label.textContent = name;
            item.appendChild(label);

            // Factory presets are read-only, so they get no delete affordance. The
            // "User" category IS the set of user presets — anything the C++ factory
            // table does not name comes back under it.
            if (group.category === USER_CATEGORY) {
                const del = document.createElement('button');
                del.type = 'button';
                del.className = 'preset-delete';
                del.textContent = '×';
                del.setAttribute('aria-label', `Delete preset ${name}`);
                // deletePreset() removes the file immediately — only promptDelete() runs
                // the confirmation, and that one is hard-wired to the *current* preset.
                // Confirm here so deleting any listed preset asks first.
                del.addEventListener('click', async (e) => {
                    e.stopPropagation();
                    if (!await confirmDeletePreset(name)) return;
                    await presetMgr.deletePreset(name);
                    await refreshPresetCategories();
                    renderPresetDropdown();
                });
                item.appendChild(del);
            }

            item.addEventListener('click', async () => {
                await presetMgr.loadPreset(name);
                hidePresetDropdown();
            });

            groupEl.appendChild(item);
        });

        presetDropdownEl.appendChild(groupEl);
    });
}

// Renders a confirmation strip at the top of the open dropdown and resolves once the
// user picks. Resolves false if the dropdown has gone away, so a delete never
// proceeds on an unanswered prompt.
function confirmDeletePreset(presetName) {
    return new Promise((resolve) => {
        if (!presetDropdownEl) {
            resolve(false);
            return;
        }

        const strip = document.createElement('div');
        strip.className = 'preset-confirm';
        strip.textContent = `Delete "${presetName}"?`;

        const buttons = document.createElement('div');
        buttons.className = 'preset-confirm-buttons';

        const finish = (answer) => {
            strip.remove();
            resolve(answer);
        };

        const yes = document.createElement('button');
        yes.type = 'button';
        yes.className = 'preset-btn preset-btn-text';
        yes.textContent = 'Delete';
        yes.addEventListener('click', (e) => { e.stopPropagation(); finish(true); });

        const no = document.createElement('button');
        no.type = 'button';
        no.className = 'preset-btn preset-btn-text';
        no.textContent = 'Cancel';
        no.addEventListener('click', (e) => { e.stopPropagation(); finish(false); });

        buttons.append(yes, no);
        strip.appendChild(buttons);

        presetDropdownEl.classList.add('show');
        presetDropdownEl.prepend(strip);
    });
}

// Initialize tooltips once their state above has been evaluated. This must stay at
// the foot of the file: initializeUI() runs at module top level, where these
// `let`/`const` bindings are still in the temporal dead zone, and the ReferenceError
// would escape module evaluation and take initializeCrossoverDrag() down with it.
// The preset bar is started here for the same reason — and separately from tooltips,
// so a failure in one cannot prevent the other from running.
function initializeDeferredUI() {
    try {
        initializeTooltips();
    } catch (e) {
        console.error('Tooltip initialization failed:', e);
    }

    initializePresets().catch((e) => console.error('Preset initialization failed:', e));
}

if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', initializeDeferredUI);
} else {
    initializeDeferredUI();
}
