/*
   This file is part of O-FreqPulse, an Ouaricon Audio plugin.
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
/* O-FreqPulse - Main Application */

import * as Juce from './juce/index.js';
import { PresetManager } from '../modules/preset-manager.js';
import { LANGUAGES, I18N, LABELS, TIP_BINDINGS, tr } from './i18n.js';

// State
const state = {
    currentBand: null,  // Expanded Euclidean panel band
    numSteps: 16,       // Current step count (4, 8, 16, or 32)
    stepStates: {},     // SliderState/ToggleState objects by parameter ID
    euclideanActive: [false, false, false, false],  // Per-band euclidean mode state
    soloedBand: -1,     // v1.13.0: Which band is soloed (-1 = none)
    preSoloEnables: [true, true, true, true],  // Enable states before solo was engaged
    bandSteps: [0, 0, 0, 0],  // v1.15.0: Per-band step count (0=follow global)
};

// Cached DOM references — populated once after renderGrid()
// cachedCells[band][step] holds direct references to all 128 step-cell elements
let cachedCells = [[], [], [], []];
let cachedGridArea = null;

// Band configuration.
//
// v1.18.0: `key` is the i18n key for the band's display name. It does double
// duty — createBandRow assigns it to the caption's dataset.i18n so the language
// sweep owns the caption, and js/i18n.js passes the same key to every one of
// that band's tips as vars.band, where tr() resolves it against the CURRENT
// language. `name` stays as the English fallback painted before applyI18n's
// first pass, so a table typo cannot leave the four captions blank.
const bands = [
    { id: 0, name: 'SUB',  key: 'bandName.sub'  },
    { id: 1, name: 'LOW',  key: 'bandName.low'  },
    { id: 2, name: 'MID',  key: 'bandName.mid'  },
    { id: 3, name: 'HIGH', key: 'bandName.high' },
];

// Format Hz value to readable string (e.g. 120 -> "120 Hz", 4000 -> "4.0 kHz")
function formatFreq(hz) {
    if (hz >= 1000) {
        const khz = hz / 1000;
        return khz % 1 === 0 ? `${khz} kHz` : `${khz.toFixed(1)} kHz`;
    }
    return `${Math.round(hz)} Hz`;
}

// Update all band frequency labels derived from crossover values
function updateAllBandFreqDisplays() {
    const c1 = state.stepStates['crossover_1'];
    const c2 = state.stepStates['crossover_2'];
    const c3 = state.stepStates['crossover_3'];
    if (!c1 || !c2 || !c3) return;

    const v1 = c1.getScaledValue();
    const v2 = c2.getScaledValue();
    const v3 = c3.getScaledValue();

    const freqLow = state.stepStates['freq_low']?.getScaledValue() ?? 20;
    const freqHigh = state.stepStates['freq_high']?.getScaledValue() ?? 20000;

    // Derive band ranges (sorted)
    const sorted = [v1, v2, v3].sort((a, b) => a - b);
    const ranges = [
        { low: freqLow, high: sorted[0] },
        { low: sorted[0], high: sorted[1] },
        { low: sorted[1], high: sorted[2] },
        { low: sorted[2], high: freqHigh },
    ];

    for (let i = 0; i < 4; i++) {
        const labelEl = document.getElementById(`freq-${i}`);
        if (labelEl) {
            labelEl.textContent = `${formatFreq(ranges[i].low)} - ${formatFreq(ranges[i].high)}`;
        }
    }
}

// ============================================================================
// Initialization
// ============================================================================

document.addEventListener('DOMContentLoaded', () => {
    console.log('O-FreqPulse UI initializing...');
    console.log('JUCE backend:', window.__JUCE__?.backend);

    // Initialize parameter bindings
    initializeGlobalParameters();
    initializeBandParameters();
    initializeStepGrid();

    // Render UI
    renderGrid();
    setupEuclideanPanel();
    initializeEuclideanListeners();
    initializeBandRateDropdowns();

    // v1.18.0: the settings popover and the language sweep, in that order and
    // BOTH before initializeTooltips().
    //
    // AFTER renderGrid(): applyI18n() is what puts data-tip on the anchors in
    // the first place, and 36 of the 56 anchors — every per-band control and
    // every crossover divider — do not exist until renderGrid has run. Binding
    // earlier would write onto nothing and report 36 warnings.
    //
    // BEFORE initializeTooltips(): the renderer's delegated listener resolves
    // e.target.closest('[data-tip]') at hover time, but a first hover landing in
    // the window between the two would find no anchor at all. Ordering here is
    // load-bearing in the ordinary way, not the TDZ way.
    //
    // Each inside its own try/catch: a translation-table typo must not take the
    // 128 bound step cells down with it, which is exactly what the MBC v1.4.0
    // TDZ throw did to unrelated working controls while build, auval and every
    // static check still passed.
    try { initSettingsPopover(); } catch (e) { console.error('settings popover init failed:', e); }
    try { initI18n(); }           catch (e) { console.error('i18n init failed:', e); }

    // v1.5.0: Initialize tooltip system
    initializeTooltips();

    // v1.6.0: Initialize preset manager
    initializePresetManager();

    // v1.15.0: Initialize per-band step count listeners
    initializeBandStepsListeners();

    // v1.13.0: Initialize mute/solo listeners (after grid rendered)
    initializeMuteSoloListeners();

    // v1.16.1: Display plugin version
    try {
        const getVersion = Juce.getNativeFunction('getPluginVersion');
        getVersion().then(v => { document.getElementById('version-label').textContent = 'v' + v; });
    } catch (e) {}

    console.log('O-FreqPulse UI initialized');
});

// ============================================================================
// Global Parameter Bindings
// ============================================================================

function initializeGlobalParameters() {
    // Mix slider
    const mixState = Juce.getSliderState('mix');
    const mixSlider = document.getElementById('mix');
    const mixValue = document.getElementById('mix-value');

    mixSlider.value = mixState.getNormalisedValue() * 100;
    mixValue.textContent = `${Math.round(mixSlider.value)}%`;

    mixSlider.addEventListener('input', (e) => {
        const normalized = e.target.value / 100;
        mixState.setNormalisedValue(normalized);
        mixValue.textContent = `${Math.round(e.target.value)}%`;
    });

    mixState.valueChangedEvent.addListener(() => {
        mixSlider.value = mixState.getNormalisedValue() * 100;
        mixValue.textContent = `${Math.round(mixSlider.value)}%`;
    });

    // Steps slider (2-32)
    const stepsState = Juce.getSliderState('steps');
    const stepsSlider = document.getElementById('steps');
    const stepsValue = document.getElementById('steps-value');

    stepsSlider.value = Math.round(stepsState.getScaledValue());
    stepsValue.textContent = stepsSlider.value;
    updateStepCount(parseInt(stepsSlider.value));

    stepsSlider.addEventListener('input', (e) => {
        const val = parseInt(e.target.value);
        const normalized = (val - 2) / 30;  // 2-32 → 0-1
        stepsState.setNormalisedValue(normalized);
        stepsValue.textContent = val;
        updateStepCount(val);
    });

    stepsState.valueChangedEvent.addListener(() => {
        const val = Math.round(stepsState.getScaledValue());
        stepsSlider.value = val;
        stepsValue.textContent = val;
        updateStepCount(val);
    });

    // Rate dropdown
    const rateState = Juce.getComboBoxState('rate');
    const rateDropdown = document.getElementById('rate');

    rateDropdown.selectedIndex = rateState.getChoiceIndex();

    rateDropdown.addEventListener('change', (e) => {
        rateState.setChoiceIndex(e.target.selectedIndex);
    });

    rateState.valueChangedEvent.addListener(() => {
        rateDropdown.selectedIndex = rateState.getChoiceIndex();
    });

    // Swing slider
    const swingState = Juce.getSliderState('swing');
    const swingSlider = document.getElementById('swing');
    const swingValue = document.getElementById('swing-value');

    swingSlider.value = swingState.getNormalisedValue() * 100;
    swingValue.textContent = `${Math.round(swingSlider.value)}%`;

    swingSlider.addEventListener('input', (e) => {
        const normalized = e.target.value / 100;
        swingState.setNormalisedValue(normalized);
        swingValue.textContent = `${Math.round(e.target.value)}%`;
    });

    swingState.valueChangedEvent.addListener(() => {
        swingSlider.value = swingState.getNormalisedValue() * 100;
        swingValue.textContent = `${Math.round(swingSlider.value)}%`;
    });

    // Attack slider (0-500ms, skewed range — use normalized values for slider position)
    const attackState = Juce.getSliderState('attack');
    const attackSlider = document.getElementById('attack');
    const attackValue = document.getElementById('attack-value');

    attackSlider.value = attackState.getNormalisedValue() * 1000;
    attackValue.textContent = `${Math.round(attackState.getScaledValue())}ms`;

    attackSlider.addEventListener('input', (e) => {
        const normalized = e.target.value / 1000;
        attackState.setNormalisedValue(normalized);
        attackValue.textContent = `${Math.round(attackState.getScaledValue())}ms`;
    });

    attackState.valueChangedEvent.addListener(() => {
        attackSlider.value = attackState.getNormalisedValue() * 1000;
        attackValue.textContent = `${Math.round(attackState.getScaledValue())}ms`;
    });

    // Release slider (0-500ms, skewed range — use normalized values for slider position)
    const releaseState = Juce.getSliderState('release');
    const releaseSlider = document.getElementById('release');
    const releaseValue = document.getElementById('release-value');

    releaseSlider.value = releaseState.getNormalisedValue() * 1000;
    releaseValue.textContent = `${Math.round(releaseState.getScaledValue())}ms`;

    releaseSlider.addEventListener('input', (e) => {
        const normalized = e.target.value / 1000;
        releaseState.setNormalisedValue(normalized);
        releaseValue.textContent = `${Math.round(releaseState.getScaledValue())}ms`;
    });

    releaseState.valueChangedEvent.addListener(() => {
        releaseSlider.value = releaseState.getNormalisedValue() * 1000;
        releaseValue.textContent = `${Math.round(releaseState.getScaledValue())}ms`;
    });
}

// ============================================================================
// Band Parameter Bindings
// ============================================================================

function initializeBandParameters() {
    // Crossover parameters (3 global crossover points)
    for (const id of ['crossover_1', 'crossover_2', 'crossover_3']) {
        const crossoverState = Juce.getSliderState(id);
        state.stepStates[id] = crossoverState;
    }

    // Frequency boundary parameters
    state.stepStates['freq_low'] = Juce.getSliderState('freq_low');
    state.stepStates['freq_high'] = Juce.getSliderState('freq_high');

    for (const band of bands) {
        const bandId = band.id;

        // Band enable toggle (not displayed in v1.0, but bound for automation)
        const enableState = Juce.getToggleState(`band${bandId}_enable`);
        state.stepStates[`band${bandId}_enable`] = enableState;

        // v1.7.0: Per-band rate override
        const rateState = Juce.getComboBoxState(`band${bandId}_rate`);
        state.stepStates[`band${bandId}_rate`] = rateState;

        // Euclidean mode toggle
        const eucOnState = Juce.getToggleState(`band${bandId}_euc_on`);
        state.stepStates[`band${bandId}_euc_on`] = eucOnState;

        // Depth slider
        const depthState = Juce.getSliderState(`band${bandId}_depth`);
        state.stepStates[`band${bandId}_depth`] = depthState;

        // Euclidean parameters
        const eucStepsState = Juce.getSliderState(`band${bandId}_euc_steps`);
        const eucPulsesState = Juce.getSliderState(`band${bandId}_euc_pulses`);
        const eucOffsetState = Juce.getSliderState(`band${bandId}_euc_offset`);

        state.stepStates[`band${bandId}_euc_steps`] = eucStepsState;
        state.stepStates[`band${bandId}_euc_pulses`] = eucPulsesState;
        state.stepStates[`band${bandId}_euc_offset`] = eucOffsetState;

        // v1.14.0: Per-band phase offset
        const phaseOffsetState = Juce.getSliderState(`band${bandId}_phase_offset`);
        state.stepStates[`band${bandId}_phase_offset`] = phaseOffsetState;

        // v1.15.0: Per-band step count
        const bandStepsState = Juce.getSliderState(`band${bandId}_steps`);
        state.stepStates[`band${bandId}_steps`] = bandStepsState;
    }
}

// ============================================================================
// Step Grid Initialization
// ============================================================================

function initializeStepGrid() {
    for (let band = 0; band < 4; band++) {
        for (let step = 0; step < 32; step++) {
            const paramId = `step_b${band}_s${step}`;
            const sliderState = Juce.getSliderState(paramId);

            state.stepStates[paramId] = sliderState;

            // Listen for changes from automation/preset load
            sliderState.valueChangedEvent.addListener(() => {
                updateStepVisual(band, step, sliderState.getNormalisedValue());
            });
        }
    }
}

// ============================================================================
// Grid Rendering
// ============================================================================

function createBandRow(band) {
    const bandRow = document.createElement('div');
    bandRow.className = 'band-row';
    bandRow.dataset.band = band.id;

    // Band label with name, M/S buttons, and frequency range.
    //
    // v1.18.0: no copy is written here any more. Every data-tip / data-tip-title
    // below is applied by applyI18n() from js/i18n.js, which is why each of these
    // runtime-built controls now carries an id — TIP_BINDINGS names its anchors
    // individually, because document.querySelector returns the FIRST match and
    // this page has four structurally identical band rows.
    const label = document.createElement('div');
    label.className = 'band-label';
    label.id = `band-label-${band.id}`;

    const labelTop = document.createElement('div');
    labelTop.className = 'band-label-top';

    // The band caption. dataset.i18n rather than setLabel(): assertion 13
    // requires a plain string literal key, and this key is per-band data. The
    // language sweep owns any [data-i18n] element however the attribute got
    // there, and assertion 15 does not report bandName.* dead because they live
    // in I18N, not LABELS. The English is painted first so a table failure
    // degrades to the old caption rather than to four empty cells.
    const nameSpan = document.createElement('strong');
    nameSpan.textContent = band.name;
    nameSpan.dataset.i18n = band.key;
    labelTop.appendChild(nameSpan);

    // v1.13.0: Mute button
    const muteBtn = document.createElement('button');
    muteBtn.className = 'ms-btn mute-btn';
    muteBtn.id = `mute-${band.id}`;
    muteBtn.textContent = 'M';
    muteBtn.dataset.i18nAria = 'aria.mute';
    muteBtn.addEventListener('click', (e) => {
        e.stopPropagation();
        toggleMute(band.id);
    });
    labelTop.appendChild(muteBtn);

    // v1.13.0: Solo button
    const soloBtn = document.createElement('button');
    soloBtn.className = 'ms-btn solo-btn';
    soloBtn.id = `solo-${band.id}`;
    soloBtn.textContent = 'S';
    soloBtn.dataset.i18nAria = 'aria.solo';
    soloBtn.addEventListener('click', (e) => {
        e.stopPropagation();
        toggleSolo(band.id);
    });
    labelTop.appendChild(soloBtn);

    label.appendChild(labelTop);

    const freqSpan = document.createElement('span');
    freqSpan.className = 'freq-range';
    freqSpan.id = `freq-${band.id}`;
    label.appendChild(freqSpan);

    bandRow.appendChild(label);

    // Steps container
    const stepsContainer = document.createElement('div');
    stepsContainer.className = 'steps-container';

    for (let step = 0; step < 32; step++) {
        const cell = document.createElement('div');
        cell.className = 'step-cell';
        cell.dataset.band = band.id;
        cell.dataset.step = step;

        // Left-click: toggle on/off (no drag) or drag vertically to set velocity
        // Shift+click: cycle velocity levels
        cell.addEventListener('mousedown', (e) => {
            if (e.button !== 0) return;
            e.preventDefault();
            startStepInteraction(band.id, step, cell, e);
        });

        cell.addEventListener('contextmenu', (e) => e.preventDefault());

        stepsContainer.appendChild(cell);
    }

    bandRow.appendChild(stepsContainer);

    // Lane action buttons (Clear/Random)
    const laneActions = document.createElement('div');
    laneActions.className = 'lane-actions';

    const clearBtn = document.createElement('button');
    clearBtn.className = 'lane-btn clear-btn';
    clearBtn.id = `clear-${band.id}`;
    clearBtn.textContent = '⌀';
    clearBtn.dataset.i18nAria = 'aria.clear';
    clearBtn.addEventListener('click', () => {
        clearBand(band.id);
    });
    laneActions.appendChild(clearBtn);

    const randomBtn = document.createElement('button');
    randomBtn.className = 'lane-btn random-btn';
    randomBtn.id = `random-${band.id}`;
    randomBtn.textContent = '⚄';
    randomBtn.dataset.i18nAria = 'aria.random';
    randomBtn.addEventListener('click', () => {
        randomizeBand(band.id);
    });
    laneActions.appendChild(randomBtn);

    bandRow.appendChild(laneActions);

    // v1.7.0: Per-band rate dropdown
    const rateSelect = document.createElement('select');
    rateSelect.className = 'band-rate-dropdown';
    rateSelect.id = `band-rate-${band.id}`;
    rateSelect.dataset.i18nAria = 'aria.bandRate';
    // The eleven option texts are the band<n>_rate AudioParameterChoice entries
    // verbatim and stay English under D-01 — they are the host automation
    // contract, and translating "Global" here alone would make this menu and the
    // automation lane disagree about what following the global setting is called.
    const rateOptions = ['Global', '1/1', '1/2', '1/4', '1/8', '1/16', '1/32', '1/8T', '1/16T', '1/4D', '1/8D'];
    rateOptions.forEach((label, idx) => {
        const opt = document.createElement('option');
        opt.value = idx;
        opt.textContent = label;
        rateSelect.appendChild(opt);
    });
    bandRow.appendChild(rateSelect);

    // v1.16.0: Inline per-band mix (depth) slider
    const mixContainer = document.createElement('div');
    mixContainer.className = 'band-mix';
    // -cell-, not -mix-: the SLIDER inside already owns `band-mix-<n>`, and the
    // tip belongs on the cell so hovering the caption shows it too.
    mixContainer.id = `band-mix-cell-${band.id}`;

    const mixLabel = document.createElement('label');
    setLabel(mixLabel, 'label.mix');
    mixContainer.appendChild(mixLabel);

    const mixSlider = document.createElement('input');
    mixSlider.type = 'range';
    mixSlider.className = 'band-mix-slider';
    mixSlider.id = `band-mix-${band.id}`;
    mixSlider.min = '0';
    mixSlider.max = '100';
    mixSlider.value = '100';

    const depthState = state.stepStates[`band${band.id}_depth`];
    if (depthState) {
        mixSlider.value = Math.round(depthState.getNormalisedValue() * 100);
        depthState.valueChangedEvent.addListener(() => {
            mixSlider.value = Math.round(depthState.getNormalisedValue() * 100);
        });
    }

    mixSlider.addEventListener('input', (e) => {
        const normalized = e.target.value / 100;
        if (depthState) depthState.setNormalisedValue(normalized);
    });

    mixContainer.appendChild(mixSlider);
    bandRow.appendChild(mixContainer);

    // Band mode toggle (clickable Manual/Euclidean)
    const modeIndicator = document.createElement('div');
    modeIndicator.className = 'band-mode';
    modeIndicator.id = `mode-${band.id}`;
    setLabel(modeIndicator, 'label.manual');
    modeIndicator.addEventListener('click', () => {
        const eucOnState = state.stepStates[`band${band.id}_euc_on`];
        if (eucOnState) {
            eucOnState.setValue(!eucOnState.getValue());
        }
    });
    bandRow.appendChild(modeIndicator);

    // Expand button (always visible — panel has Phase/Depth for both modes)
    const expandBtn = document.createElement('button');
    expandBtn.className = 'band-expand-btn visible';
    expandBtn.id = `expand-${band.id}`;
    expandBtn.textContent = '▶';
    expandBtn.dataset.i18nAria = 'aria.expand';
    expandBtn.addEventListener('click', () => {
        openEuclideanPanel(band.id);
    });
    bandRow.appendChild(expandBtn);

    return bandRow;
}

// Ordered list of frequency parameters from low to high for clamping
const freqParamOrder = ['freq_low', 'crossover_1', 'crossover_2', 'crossover_3', 'freq_high'];

function clampFreqSlider(paramId, newNorm) {
    const idx = freqParamOrder.indexOf(paramId);
    if (idx < 0) return newNorm;

    // Get neighbour normalised values
    let minNorm = 0;
    let maxNorm = 1;

    if (idx > 0) {
        const lowerState = state.stepStates[freqParamOrder[idx - 1]];
        if (lowerState) minNorm = lowerState.getNormalisedValue();
    }
    if (idx < freqParamOrder.length - 1) {
        const upperState = state.stepStates[freqParamOrder[idx + 1]];
        if (upperState) maxNorm = upperState.getNormalisedValue();
    }

    return Math.max(minNorm, Math.min(maxNorm, newNorm));
}

// v1.18.0: the tooltipText parameter is GONE. The five crossover / boundary
// tips are keyed in js/i18n.js and applied by applyI18n() onto the id below.
function createDividerSlider(paramId, cssClass) {
    const divider = document.createElement('div');
    divider.className = cssClass;
    divider.id = `divider-${paramId}`;

    const slider = document.createElement('input');
    slider.type = 'range';
    slider.min = '0';
    slider.max = '1000';
    slider.id = `divider-slider-${paramId}`;

    const paramState = state.stepStates[paramId];
    if (paramState) {
        slider.value = paramState.getNormalisedValue() * 1000;

        slider.addEventListener('input', (e) => {
            const rawNorm = e.target.value / 1000;
            const clamped = clampFreqSlider(paramId, rawNorm);
            slider.value = clamped * 1000;
            paramState.setNormalisedValue(clamped);
            updateAllBandFreqDisplays();
        });

        paramState.valueChangedEvent.addListener(() => {
            slider.value = paramState.getNormalisedValue() * 1000;
        });
    }

    divider.appendChild(slider);
    return divider;
}

function renderGrid() {
    const container = document.getElementById('grid-container');
    container.innerHTML = '';

    // DOM order (column-reverse means first child = bottom visually):
    // 1. freq_low boundary (bottom)
    // 2. SUB band row
    // 3. Crossover 1 divider (Sub|Low)
    // 4. LOW band row
    // 5. Crossover 2 divider (Low|Mid)
    // 6. MID band row
    // 7. Crossover 3 divider (Mid|High)
    // 8. HIGH band row
    // 9. freq_high boundary (top)

    container.appendChild(createDividerSlider('freq_low', 'freq-boundary'));
    container.appendChild(createBandRow(bands[0]));  // SUB
    container.appendChild(createDividerSlider('crossover_1', 'crossover-divider'));
    container.appendChild(createBandRow(bands[1]));  // LOW
    container.appendChild(createDividerSlider('crossover_2', 'crossover-divider'));
    container.appendChild(createBandRow(bands[2]));  // MID
    container.appendChild(createDividerSlider('crossover_3', 'crossover-divider'));
    container.appendChild(createBandRow(bands[3]));  // HIGH
    container.appendChild(createDividerSlider('freq_high', 'freq-boundary'));

    // Update step visibility based on current step count
    updateStepVisibility();

    // Sync initial step states from JUCE (velocity values)
    for (let band = 0; band < 4; band++) {
        for (let step = 0; step < 32; step++) {
            const paramId = `step_b${band}_s${step}`;
            const sliderState = state.stepStates[paramId];
            updateStepVisual(band, step, sliderState.getNormalisedValue());
        }
    }

    // Cache all cell references in a 2D array for O(1) lookups
    for (let b = 0; b < 4; b++) {
        cachedCells[b] = [...document.querySelectorAll(`.step-cell[data-band="${b}"]`)];
    }
    cachedGridArea = document.querySelector('.grid-area');
}

function toggleStep(band, step) {
    // Ignore clicks when euclidean mode is active for this band
    if (state.euclideanActive[band]) return;

    const paramId = `step_b${band}_s${step}`;
    const sliderState = state.stepStates[paramId];
    // Toggle: if any velocity > 0 → set to 0, otherwise set to 1
    const currentVel = sliderState.getNormalisedValue();
    const newVel = currentVel > 0.001 ? 0.0 : 1.0;

    sliderState.setNormalisedValue(newVel);
    updateStepVisual(band, step, newVel);
}

function cycleVelocity(band, step) {
    if (state.euclideanActive[band]) return;

    const paramId = `step_b${band}_s${step}`;
    const sliderState = state.stepStates[paramId];
    const current = sliderState.getNormalisedValue();

    // Cycle: 0 → 0.25 → 0.5 → 0.75 → 1.0 → 0
    const levels = [0, 0.25, 0.5, 0.75, 1.0];
    let nextIdx = 0;
    for (let i = 0; i < levels.length; i++) {
        if (current < levels[i] + 0.01) {
            nextIdx = (i + 1) % levels.length;
            break;
        }
    }
    // If current > 1.0 (shouldn't happen), wrap to 0
    if (current > 0.99) nextIdx = 0;

    const newVel = levels[nextIdx];
    sliderState.setNormalisedValue(newVel);
    updateStepVisual(band, step, newVel);
}

function startStepInteraction(band, step, cell, startEvent) {
    if (state.euclideanActive[band]) return;

    const paramId = `step_b${band}_s${step}`;
    const sliderState = state.stepStates[paramId];
    const startY = startEvent.clientY;
    const startVel = sliderState.getNormalisedValue();
    let isDragging = false;
    const DRAG_THRESHOLD = 3;

    function onMouseMove(e) {
        const deltaY = startY - e.clientY; // positive = up = increase velocity

        if (!isDragging) {
            if (Math.abs(deltaY) > DRAG_THRESHOLD) {
                isDragging = true;
            } else {
                return;
            }
        }

        // 100px vertical movement = full 0-to-1 range
        const newVel = Math.max(0, Math.min(1, startVel + deltaY / 100));
        const quantized = Math.round(newVel * 100) / 100;
        sliderState.setNormalisedValue(quantized);
        updateStepVisual(band, step, quantized);
    }

    function onMouseUp() {
        document.removeEventListener('mousemove', onMouseMove);
        document.removeEventListener('mouseup', onMouseUp);

        if (!isDragging) {
            if (startEvent.shiftKey) {
                cycleVelocity(band, step);
            } else {
                toggleStep(band, step);
            }
        }
    }

    document.addEventListener('mousemove', onMouseMove);
    document.addEventListener('mouseup', onMouseUp);
}

function updateStepVisual(band, step, velocity) {
    // Skip manual step visual updates when euclidean mode controls the display
    if (state.euclideanActive[band]) return;

    const cell = cachedCells[band]?.[step];
    if (!cell) return;

    const isActive = velocity > 0.001;
    cell.classList.toggle('active', isActive);

    // Show velocity as fill height (bottom-up) via CSS custom property
    if (isActive) {
        cell.style.setProperty('--vel', (velocity * 100) + '%');
    } else {
        cell.style.removeProperty('--vel');
    }
    cell.style.opacity = '';
}

function updateStepCount(count) {
    state.numSteps = Math.max(2, Math.min(32, count || 16));
    updateStepVisibility();
}

function updateStepVisibility() {
    for (let b = 0; b < 4; b++) {
        // v1.15.0: Per-band step count (0 = follow global)
        const effSteps = (state.bandSteps[b] >= 2) ? state.bandSteps[b] : state.numSteps;
        const bandCells = cachedCells[b];
        for (let s = 0; s < bandCells.length; s++) {
            bandCells[s].style.display = s < effSteps ? 'block' : 'none';
        }
    }
}

// ============================================================================
// Per-Band Playhead Highlighting (called from C++ timer)
// ============================================================================

// Track previous playing step per band for efficient class toggling
const lastPlayingStep = [-1, -1, -1, -1];

window.updatePlayhead = function (b0, b1, b2, b3, hasSignal) {
    const bandSteps = [b0, b1, b2, b3];

    for (let band = 0; band < 4; band++) {
        const prevStep = lastPlayingStep[band];
        const newStep = bandSteps[band];

        // Remove previous highlight
        if (prevStep >= 0 && prevStep !== newStep) {
            const prevCell = cachedCells[band]?.[prevStep];
            if (prevCell) prevCell.classList.remove('playing');
        }

        if (hasSignal && newStep >= 0) {
            const cell = cachedCells[band]?.[newStep];
            if (cell && cell.style.display !== 'none') {
                cell.classList.add('playing');
            }
            lastPlayingStep[band] = newStep;
        } else {
            // Clear highlight when no signal
            if (prevStep >= 0) {
                const prevCell = cachedCells[band]?.[prevStep];
                if (prevCell) prevCell.classList.remove('playing');
            }
            lastPlayingStep[band] = -1;
        }
    }
};

// ============================================================================
// Euclidean Panel
// ============================================================================

function setupEuclideanPanel() {
    const panel = document.getElementById('euclidean-panel');
    const closeBtn = document.getElementById('euc-close');

    closeBtn.addEventListener('click', () => {
        panel.classList.remove('visible');
        state.currentBand = null;
    });
}

function openEuclideanPanel(bandId) {
    const panel = document.getElementById('euclidean-panel');
    const title = document.getElementById('euc-band-title');

    state.currentBand = bandId;

    // ONE key with a {band} token and a literal key, re-applied per band. A
    // template literal here would have shipped an English heading in a French
    // panel the moment the selector fired; setLabel makes the element a
    // [data-i18n] element from this call on, so the sweep owns it too.
    const band = bands.find((b) => b.id === bandId);
    setLabel(title, 'label.bandControls', { band: band.key });

    // Sync controls with current parameter values
    syncEuclideanControls(bandId);

    panel.classList.add('visible');
}

function syncEuclideanControls(bandId) {
    // Euclidean steps
    const eucStepsState = state.stepStates[`band${bandId}_euc_steps`];
    const eucStepsSlider = document.getElementById('euc-steps');
    const eucStepsValue = document.getElementById('euc-steps-value');

    eucStepsSlider.value = eucStepsState.getScaledValue();
    eucStepsValue.textContent = Math.round(eucStepsSlider.value);

    eucStepsSlider.oninput = (e) => {
        const normalized = (e.target.value - 1) / 31;  // 1-32 → 0-1
        eucStepsState.setNormalisedValue(normalized);
        eucStepsValue.textContent = Math.round(eucStepsState.getScaledValue());
    };

    // Euclidean pulses
    const eucPulsesState = state.stepStates[`band${bandId}_euc_pulses`];
    const eucPulsesSlider = document.getElementById('euc-pulses');
    const eucPulsesValue = document.getElementById('euc-pulses-value');

    eucPulsesSlider.value = eucPulsesState.getScaledValue();
    eucPulsesValue.textContent = Math.round(eucPulsesSlider.value);

    eucPulsesSlider.oninput = (e) => {
        const normalized = (e.target.value - 1) / 31;
        eucPulsesState.setNormalisedValue(normalized);
        eucPulsesValue.textContent = Math.round(eucPulsesState.getScaledValue());
    };

    // Euclidean offset
    const eucOffsetState = state.stepStates[`band${bandId}_euc_offset`];
    const eucOffsetSlider = document.getElementById('euc-offset');
    const eucOffsetValue = document.getElementById('euc-offset-value');

    eucOffsetSlider.value = eucOffsetState.getScaledValue();
    eucOffsetValue.textContent = Math.round(eucOffsetSlider.value);

    eucOffsetSlider.oninput = (e) => {
        const normalized = e.target.value / 31;  // 0-31 → 0-1
        eucOffsetState.setNormalisedValue(normalized);
        eucOffsetValue.textContent = Math.round(eucOffsetState.getScaledValue());
    };

    // v1.14.0: Phase offset
    const phaseState = state.stepStates[`band${bandId}_phase_offset`];
    const phaseSlider = document.getElementById('euc-phase');
    const phaseValue = document.getElementById('euc-phase-value');

    phaseSlider.value = phaseState.getScaledValue();
    phaseValue.textContent = Math.round(phaseSlider.value);

    phaseSlider.oninput = (e) => {
        const normalized = e.target.value / 31;  // 0-31 → 0-1
        phaseState.setNormalisedValue(normalized);
        phaseValue.textContent = Math.round(phaseState.getScaledValue());
    };

    // v1.15.0: Per-band step count
    const bandStepsState = state.stepStates[`band${bandId}_steps`];
    const bandStepsSlider = document.getElementById('euc-band-steps');
    const bandStepsValue = document.getElementById('euc-band-steps-value');

    const bStepsVal = Math.round(bandStepsState.getScaledValue());
    bandStepsSlider.value = bStepsVal;
    bandStepsValue.textContent = bStepsVal === 0 ? 'Global' : bStepsVal;

    bandStepsSlider.oninput = (e) => {
        const val = parseInt(e.target.value);
        // Skip value 1 (invalid) — snap to 0 or 2
        const clamped = (val === 1) ? 0 : val;
        bandStepsSlider.value = clamped;
        const normalized = clamped / 32;
        bandStepsState.setNormalisedValue(normalized);
        bandStepsValue.textContent = clamped === 0 ? 'Global' : clamped;
    };

}

function updateBandModeIndicator(bandId, isEuclidean) {
    const indicator = document.getElementById(`mode-${bandId}`);
    if (indicator) {
        // Two literal-keyed calls from an if/else, never a ternary inside the
        // call — check-i18n assertion 13. A literal caption here would have been
        // stranded in the previous language the instant the selector fired,
        // which is the generalised form of the bug Stage B found on MBC's
        // hover-help toggle.
        if (isEuclidean) setLabel(indicator, 'label.euclidean');
        else             setLabel(indicator, 'label.manual');
        indicator.classList.toggle('euclidean', isEuclidean);
    }

    // Expand button always visible (Phase/Steps accessible in both modes)
}

// ============================================================================
// Lane Actions (Clear/Random)
// ============================================================================

function clearBand(bandId) {
    if (state.euclideanActive[bandId]) return;

    for (let step = 0; step < 32; step++) {
        const paramId = `step_b${bandId}_s${step}`;
        const sliderState = state.stepStates[paramId];
        if (sliderState) {
            sliderState.setNormalisedValue(0.0);
            updateStepVisual(bandId, step, 0.0);
        }
    }
}

function randomizeBand(bandId) {
    if (state.euclideanActive[bandId]) return;

    for (let step = 0; step < 32; step++) {
        const paramId = `step_b${bandId}_s${step}`;
        const sliderState = state.stepStates[paramId];
        if (sliderState) {
            // 50% chance each step is active, with random velocity 0.5–1.0
            const active = Math.random() < 0.5;
            const velocity = active ? (0.5 + Math.random() * 0.5) : 0.0;
            sliderState.setNormalisedValue(velocity);
            updateStepVisual(bandId, step, velocity);
        }
    }
}

// ============================================================================
// Euclidean Pattern Generation (mirrors C++ Bresenham algorithm)
// ============================================================================

function generateEuclidean(steps, pulses, offset) {
    const pattern = new Array(32).fill(false);

    if (pulses > steps) pulses = steps;
    if (pulses <= 0 || steps <= 0) return pattern;

    // Bresenham bucket-fill
    let bucket = 0;
    for (let i = 0; i < steps; i++) {
        bucket += pulses;
        if (bucket >= steps) {
            bucket -= steps;
            pattern[i] = true;
        }
    }

    // Apply rotation offset
    if (offset > 0 && offset < steps) {
        const head = pattern.slice(0, steps);
        const rotated = head.slice(offset).concat(head.slice(0, offset));
        for (let i = 0; i < steps; i++) {
            pattern[i] = rotated[i];
        }
    }

    return pattern;
}

function updateEuclideanGrid(bandId) {
    const eucOnState = state.stepStates[`band${bandId}_euc_on`];
    const isEuclidean = eucOnState && eucOnState.getValue();

    state.euclideanActive[bandId] = isEuclidean;
    updateBandModeIndicator(bandId, isEuclidean);

    // Panel stays open in both modes (Phase/Depth work in manual too)

    const cells = cachedCells[bandId];

    if (isEuclidean) {
        // Compute euclidean pattern
        const eucSteps = Math.round(state.stepStates[`band${bandId}_euc_steps`].getScaledValue());
        const eucPulses = Math.round(state.stepStates[`band${bandId}_euc_pulses`].getScaledValue());
        const eucOffset = Math.round(state.stepStates[`band${bandId}_euc_offset`].getScaledValue());

        // v1.15.0: Per-band step count overrides euc_steps when set
        const bSteps = state.bandSteps[bandId];
        const effectiveEucSteps = (bSteps >= 2) ? bSteps : eucSteps;

        const pattern = generateEuclidean(effectiveEucSteps, eucPulses, eucOffset);

        for (let step = 0; step < cells.length; step++) {
            const cell = cells[step];
            cell.classList.add('euclidean-mode');
            cell.classList.remove('active');

            // Wrap via modulo to match C++ getTargetGainForBand() behavior
            const wrappedStep = step % effectiveEucSteps;
            cell.classList.toggle('euclidean-active', pattern[wrappedStep]);
        }
    } else {
        // Restore manual step display with velocity fill bar
        for (let step = 0; step < cells.length; step++) {
            const cell = cells[step];
            cell.classList.remove('euclidean-mode', 'euclidean-active');

            const paramId = `step_b${bandId}_s${step}`;
            const sliderState = state.stepStates[paramId];
            if (sliderState) {
                const vel = sliderState.getNormalisedValue();
                const isActive = vel > 0.001;
                cell.classList.toggle('active', isActive);
                if (isActive) {
                    cell.style.setProperty('--vel', (vel * 100) + '%');
                } else {
                    cell.style.removeProperty('--vel');
                }
                cell.style.opacity = '';
            }
        }
    }
}

function initializeEuclideanListeners() {
    for (let bandId = 0; bandId < 4; bandId++) {
        const bid = bandId;  // Capture for closure

        // Listen for euclidean on/off changes
        const eucOnState = state.stepStates[`band${bid}_euc_on`];
        if (eucOnState) {
            eucOnState.valueChangedEvent.addListener(() => {
                updateEuclideanGrid(bid);
            });
        }

        // Listen for euclidean parameter changes
        for (const param of ['euc_steps', 'euc_pulses', 'euc_offset']) {
            const paramState = state.stepStates[`band${bid}_${param}`];
            if (paramState) {
                paramState.valueChangedEvent.addListener(() => {
                    if (state.euclideanActive[bid]) {
                        updateEuclideanGrid(bid);
                    }
                });
            }
        }

        // Sync initial euclidean state
        updateEuclideanGrid(bid);
    }

    // Listen for crossover and boundary parameter changes to update all band labels
    for (const id of ['crossover_1', 'crossover_2', 'crossover_3', 'freq_low', 'freq_high']) {
        const paramState = state.stepStates[id];
        if (paramState) {
            paramState.valueChangedEvent.addListener(() => {
                updateAllBandFreqDisplays();
            });
        }
    }

    // Initial frequency label update
    updateAllBandFreqDisplays();
}

// ============================================================================
// v1.7.0: Per-Band Rate Dropdowns
// ============================================================================

function initializeBandRateDropdowns() {
    for (let bandId = 0; bandId < 4; bandId++) {
        const rateState = state.stepStates[`band${bandId}_rate`];
        const dropdown = document.getElementById(`band-rate-${bandId}`);
        if (!rateState || !dropdown) continue;

        // Sync initial value
        dropdown.selectedIndex = rateState.getChoiceIndex();

        // UI → JUCE
        dropdown.addEventListener('change', (e) => {
            rateState.setChoiceIndex(e.target.selectedIndex);
        });

        // JUCE → UI (automation/preset changes)
        rateState.valueChangedEvent.addListener(() => {
            dropdown.selectedIndex = rateState.getChoiceIndex();
        });
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// Interface language (v1.18.0)
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
// `grep -rn setVisible plugins/O-FreqPulse/Source/` returns NOTHING — the web
// view is never hidden, so the hidden-completion drop cannot fire
// (critical_webview_completion_gated_on_isvisible).
//
// Declared here at module level, ABOVE every reader. The only statements
// executed at module-evaluation time are the two window.__ assignments, which
// touch hoisted function declarations and cannot enter a TDZ chain
// (pattern_module_toplevel_init_tdz). initI18n() itself is called from INSIDE
// the DOMContentLoaded handler, after renderGrid().

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
// The settings popover (v1.18.0)
// ═══════════════════════════════════════════════════════════════════════════
//
// The gear that carries the language selector and the hover-help switch. Two
// rows: this plugin HAS the setTooltipsEnabled bridge, so its toggle moves in
// here from the floating "?" rather than sitting beside a second control for the
// same state.
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
    // click, so the panel is gone before a drag on a slider underneath it
    // begins — the step cells call preventDefault in their own mousedown
    // handlers. Matches how the preset dropdown already behaves.
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
// Tooltips — the measure-then-pin renderer (v1.18.0)
// ═══════════════════════════════════════════════════════════════════════════
//
// PORTED from O-ReverseDelay / O-MultiBandCompressor, replacing this plugin's
// own second positioner ENTIRELY. There is now ONE tooltip renderer repo-wide.
//
// What the port brings that v1.17.0's positioner did not have: a title/body pair
// built from data-tip-title + data-tip rather than one flat string, a dwell
// delay so a tip does not fire on every crossing, an arrow whose offset is
// recomputed AFTER the horizontal clamp so a clamped tip still points at its
// control, delegated listeners on the DOCUMENT so runtime-built anchors need no
// re-binding, and viewport-relative arithmetic that matches the fixed-position
// box the browser actually lays out.
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

// v1.5.0: master on/off for the hover-help layer, persisted C++-side on the
// APVTS state tree. v1.18.0 moved its control out of the floating "?" and into
// the settings popover, next to the language selector.
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
    // away until release, so it cannot hang over a step cell mid-drag. Capture
    // phase, because the step cells call preventDefault in their own mousedown
    // handlers.
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
    // WR-01 (v1.16.x) still applies: getNativeFunction lives on the `Juce`
    // ES-module namespace, NOT on window.__JUCE__.backend, whose Backend object
    // only has addEventListener/removeEventListener/emitEvent.
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

    const pluginContainer = document.getElementById('plugin-container');
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

    // Moving between children of the same control is not a real exit. Several
    // control-groups wrap a label, a slider and a value-display, and crossing
    // between those children previously flickered the surface off and back on.
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
    // afterwards re-wraps a 220 px tip into a narrow ribbon — and the squeezed
    // width then resolves `left` straight back against the right edge, so it
    // never recovers on later hovers. Release the width, measure from the left
    // edge, pin the result in px, and only then place.
    //
    // The pinned width is the FRACTIONAL getBoundingClientRect().width, not the
    // integer offsetWidth: 208.48 rounds to 208, and pinning that makes the box
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

    // ONE LINE THE PORTED RENDERER DID NOT HAVE, and it is not a tidy-up.
    // O-ReverseDelay's anchors are all knob-sized, so `below` always fits there
    // and the omission is invisible; this page has #grid-area, 376px tall, where
    // NEITHER above nor below has room. Measured before this line: the French
    // grid tip is 97px and landed at top 468 in a 550px frame — 15px off the
    // bottom of the window. v1.17.0's own positioner clamped here, so porting
    // without it would have re-broken something this plugin had already fixed.
    // The same gap is latent in every other copy of this renderer; it is
    // REPORTED rather than swept, because this commit is scoped to one plugin.
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

// ============================================================================
// v1.6.0: Preset Manager
// ============================================================================

function initializePresetManager() {
    const presetNameDisplay = document.getElementById('presetName');
    const presetDropdown = document.getElementById('presetDropdown');

    if (!presetNameDisplay) return;

    const presetManager = new PresetManager({
        displayElement: presetNameDisplay,
        prevButton: document.getElementById('prevPreset'),
        nextButton: document.getElementById('nextPreset'),
        saveButton: document.getElementById('savePreset'),
        loadButton: document.getElementById('loadPreset'),
        getNativeFunction: Juce.getNativeFunction,
        onPresetChanged: (name) => {
            console.log('Preset loaded:', name);
            hidePresetDropdown();
        },
        onPresetListUpdated: (list) => {
            console.log('Preset list updated:', list.length, 'presets');
        }
    });
    presetManager.initialize();

    // Preset dropdown (click preset name to show list)
    function showPresetDropdown() {
        const presets = presetManager.getPresetList();
        presetDropdown.innerHTML = '';

        if (presets.length === 0) {
            // createElement + setLabel, not innerHTML. check-i18n assertion 12
            // reports the prose inside a markup template as a raw English write,
            // and no I18N_EXEMPT entry can cover it, because an exemption lives
            // in i18n.js where assertion 9 forbids the angle bracket the
            // template is made of.
            const empty = document.createElement('div');
            empty.className = 'preset-dropdown-item';
            empty.style.opacity = '0.5';
            setLabel(empty, 'label.noPresets');
            presetDropdown.appendChild(empty);
            presetDropdown.classList.add('show');
            return;
        }

        const currentName = presetManager.getCurrentPreset();
        presets.forEach((name) => {
            const item = document.createElement('div');
            item.className = 'preset-dropdown-item';
            if (name === currentName) item.classList.add('active');
            item.textContent = name;
            item.addEventListener('click', (e) => {
                e.stopPropagation();
                presetManager.loadPreset(name);
                hidePresetDropdown();
            });
            presetDropdown.appendChild(item);
        });

        presetDropdown.classList.add('show');
    }

    function hidePresetDropdown() {
        presetDropdown.classList.remove('show');
    }

    presetNameDisplay.addEventListener('click', (e) => {
        e.stopPropagation();
        if (presetDropdown.classList.contains('show')) {
            hidePresetDropdown();
        } else {
            showPresetDropdown();
        }
    });

    // Close dropdown when clicking elsewhere
    document.addEventListener('click', () => {
        hidePresetDropdown();
    });
}

// ============================================================================
// v1.13.0: Mute/Solo System
// ============================================================================

function toggleMute(bandId) {
    const enableState = state.stepStates[`band${bandId}_enable`];
    if (!enableState) return;

    const currentlyEnabled = enableState.getValue();
    enableState.setValue(!currentlyEnabled);

    // If we mute a soloed band, clear solo
    if (state.soloedBand === bandId && currentlyEnabled) {
        state.soloedBand = -1;
        updateAllSoloVisuals();
    }

    // Explicit visual update (event may not fire if value unchanged)
    updateMuteVisual(bandId);
}

function toggleSolo(bandId) {
    if (state.soloedBand === bandId) {
        // Unsolo: restore pre-solo enable states
        for (let i = 0; i < 4; i++) {
            const enableState = state.stepStates[`band${i}_enable`];
            if (enableState) {
                enableState.setValue(state.preSoloEnables[i]);
            }
        }
        state.soloedBand = -1;
    } else {
        // Save current enable states before soloing
        for (let i = 0; i < 4; i++) {
            const enableState = state.stepStates[`band${i}_enable`];
            state.preSoloEnables[i] = enableState ? enableState.getValue() : true;
        }

        // Solo: enable this band, disable all others
        for (let i = 0; i < 4; i++) {
            const enableState = state.stepStates[`band${i}_enable`];
            if (enableState) {
                enableState.setValue(i === bandId);
            }
        }
        state.soloedBand = bandId;
    }

    // Explicit visual update for all bands (events may not fire for unchanged values)
    for (let i = 0; i < 4; i++) updateMuteVisual(i);
    updateAllSoloVisuals();
}

function updateMuteVisual(bandId) {
    const enableState = state.stepStates[`band${bandId}_enable`];
    if (!enableState) return;

    const enabled = enableState.getValue();
    const muteBtn = document.getElementById(`mute-${bandId}`);
    const bandRow = document.querySelector(`.band-row[data-band="${bandId}"]`);

    if (muteBtn) muteBtn.classList.toggle('active', !enabled);
    if (bandRow) bandRow.classList.toggle('muted', !enabled);
}

function updateAllSoloVisuals() {
    for (let i = 0; i < 4; i++) {
        const soloBtn = document.getElementById(`solo-${i}`);
        if (soloBtn) soloBtn.classList.toggle('active', state.soloedBand === i);
    }
}

// ============================================================================
// v1.15.0: Per-Band Step Count Listeners
// ============================================================================

function initializeBandStepsListeners() {
    for (let bandId = 0; bandId < 4; bandId++) {
        const bid = bandId;
        const bStepsState = state.stepStates[`band${bid}_steps`];
        if (!bStepsState) continue;

        // Sync initial value
        state.bandSteps[bid] = Math.round(bStepsState.getScaledValue());

        // Listen for changes
        bStepsState.valueChangedEvent.addListener(() => {
            state.bandSteps[bid] = Math.round(bStepsState.getScaledValue());
            updateStepVisibility();
            // Refresh euclidean grid if active (band steps overrides euc_steps)
            if (state.euclideanActive[bid]) {
                updateEuclideanGrid(bid);
            }
        });
    }
}

function initializeMuteSoloListeners() {
    for (let bandId = 0; bandId < 4; bandId++) {
        const bid = bandId;
        const enableState = state.stepStates[`band${bid}_enable`];
        if (!enableState) continue;

        // Sync visual on automation/preset changes
        enableState.valueChangedEvent.addListener(() => {
            updateMuteVisual(bid);

            // If automation changes enable state, clear stale solo
            if (state.soloedBand >= 0) {
                const soloedEnable = state.stepStates[`band${state.soloedBand}_enable`];
                if (soloedEnable && !soloedEnable.getValue()) {
                    state.soloedBand = -1;
                    updateAllSoloVisuals();
                }
            }
        });

        // Sync initial state
        updateMuteVisual(bid);
    }
}
