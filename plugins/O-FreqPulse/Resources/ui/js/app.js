/* O-FreqPulse - Main Application */

import * as Juce from './juce/index.js';
import { PresetManager } from '../modules/preset-manager.js';

// State
const state = {
    currentBand: null,  // Expanded Euclidean panel band
    numSteps: 16,       // Current step count (4, 8, 16, or 32)
    stepStates: {},     // SliderState/ToggleState objects by parameter ID
    euclideanActive: [false, false, false, false],  // Per-band euclidean mode state
    tooltipsEnabled: false,  // v1.5.0: Tooltip toggle state
};

// Band configuration
const bands = [
    { id: 0, name: 'SUB' },
    { id: 1, name: 'LOW' },
    { id: 2, name: 'MID' },
    { id: 3, name: 'HIGH' },
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
    setupGlobalControls();
    setupEuclideanPanel();
    initializeEuclideanListeners();

    // v1.5.0: Initialize tooltip system
    initializeTooltips();

    // v1.6.0: Initialize preset manager
    initializePresetManager();

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

    // Smoothing slider
    const smoothingState = Juce.getSliderState('smoothing');
    const smoothingSlider = document.getElementById('smoothing');
    const smoothingValue = document.getElementById('smoothing-value');

    smoothingSlider.value = smoothingState.getScaledValue();
    smoothingValue.textContent = `${Math.round(smoothingSlider.value)}ms`;

    smoothingSlider.addEventListener('input', (e) => {
        const normalized = e.target.value / 100;
        smoothingState.setNormalisedValue(normalized);
        smoothingValue.textContent = `${Math.round(smoothingState.getScaledValue())}ms`;
    });

    smoothingState.valueChangedEvent.addListener(() => {
        smoothingSlider.value = smoothingState.getScaledValue();
        smoothingValue.textContent = `${Math.round(smoothingSlider.value)}ms`;
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
    }
}

// ============================================================================
// Step Grid Initialization
// ============================================================================

function initializeStepGrid() {
    for (let band = 0; band < 4; band++) {
        for (let step = 0; step < 32; step++) {
            const paramId = `step_b${band}_s${step}`;
            const toggleState = Juce.getToggleState(paramId);

            state.stepStates[paramId] = toggleState;

            // Listen for changes from automation/preset load
            toggleState.valueChangedEvent.addListener(() => {
                updateStepVisual(band, step, toggleState.getValue());
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

    // Band label with derived frequency range (read-only)
    const label = document.createElement('div');
    label.className = 'band-label';
    label.innerHTML = `<strong>${band.name}</strong><br><span class="freq-range" id="freq-${band.id}"></span>`;
    label.setAttribute('data-tooltip', `${band.name} band: Shows the frequency range for this band. Frequencies are set by the crossover sliders between bands.`);
    bandRow.appendChild(label);

    // Steps container
    const stepsContainer = document.createElement('div');
    stepsContainer.className = 'steps-container';

    for (let step = 0; step < 32; step++) {
        const cell = document.createElement('div');
        cell.className = 'step-cell';
        cell.dataset.band = band.id;
        cell.dataset.step = step;

        cell.addEventListener('click', () => {
            toggleStep(band.id, step);
        });

        stepsContainer.appendChild(cell);
    }

    bandRow.appendChild(stepsContainer);

    // Lane action buttons (Clear/Random)
    const laneActions = document.createElement('div');
    laneActions.className = 'lane-actions';

    const clearBtn = document.createElement('button');
    clearBtn.className = 'lane-btn clear-btn';
    clearBtn.textContent = '⌀';
    clearBtn.title = 'Clear all steps';
    clearBtn.setAttribute('data-tooltip', 'Clear: Resets all steps in this band to OFF.');
    clearBtn.addEventListener('click', () => {
        clearBand(band.id);
    });
    laneActions.appendChild(clearBtn);

    const randomBtn = document.createElement('button');
    randomBtn.className = 'lane-btn random-btn';
    randomBtn.textContent = '⚄';
    randomBtn.title = 'Randomize steps';
    randomBtn.setAttribute('data-tooltip', 'Random: Fills steps with a random pattern (50% probability per step).');
    randomBtn.addEventListener('click', () => {
        randomizeBand(band.id);
    });
    laneActions.appendChild(randomBtn);

    bandRow.appendChild(laneActions);

    // Band mode toggle (clickable Manual/Euclidean)
    const modeIndicator = document.createElement('div');
    modeIndicator.className = 'band-mode';
    modeIndicator.textContent = 'Manual';
    modeIndicator.id = `mode-${band.id}`;
    modeIndicator.setAttribute('data-tooltip', 'Mode: Click to toggle between Manual (draw your own pattern) and Euclidean (algorithmically generated rhythm).');
    modeIndicator.addEventListener('click', () => {
        const eucOnState = state.stepStates[`band${band.id}_euc_on`];
        if (eucOnState) {
            eucOnState.setValue(!eucOnState.getValue());
        }
    });
    bandRow.appendChild(modeIndicator);

    // Expand button (visible only in euclidean mode)
    const expandBtn = document.createElement('button');
    expandBtn.className = 'band-expand-btn';
    expandBtn.id = `expand-${band.id}`;
    expandBtn.textContent = '▶';
    expandBtn.setAttribute('data-tooltip', 'Expand: Opens the Euclidean controls panel for this band (Steps, Pulses, Offset, Depth).');
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

function createDividerSlider(paramId, cssClass, tooltipText) {
    const divider = document.createElement('div');
    divider.className = cssClass;
    if (tooltipText) divider.setAttribute('data-tooltip', tooltipText);

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

    container.appendChild(createDividerSlider('freq_low', 'freq-boundary', 'Low Boundary: Sets the lowest frequency included in processing. Frequencies below this are unaffected.'));
    container.appendChild(createBandRow(bands[0]));  // SUB
    container.appendChild(createDividerSlider('crossover_1', 'crossover-divider', 'Crossover 1: Split point between Sub and Low bands. Drag to adjust where sub frequencies end and low frequencies begin.'));
    container.appendChild(createBandRow(bands[1]));  // LOW
    container.appendChild(createDividerSlider('crossover_2', 'crossover-divider', 'Crossover 2: Split point between Low and Mid bands. Drag to adjust the frequency boundary.'));
    container.appendChild(createBandRow(bands[2]));  // MID
    container.appendChild(createDividerSlider('crossover_3', 'crossover-divider', 'Crossover 3: Split point between Mid and High bands. Drag to adjust where mid frequencies end and highs begin.'));
    container.appendChild(createBandRow(bands[3]));  // HIGH
    container.appendChild(createDividerSlider('freq_high', 'freq-boundary', 'High Boundary: Sets the highest frequency included in processing. Frequencies above this are unaffected.'));

    // Update step visibility based on current step count
    updateStepVisibility();

    // Sync initial step states from JUCE
    for (let band = 0; band < 4; band++) {
        for (let step = 0; step < 32; step++) {
            const paramId = `step_b${band}_s${step}`;
            const toggleState = state.stepStates[paramId];
            updateStepVisual(band, step, toggleState.getValue());
        }
    }
}

function toggleStep(band, step) {
    // Ignore clicks when euclidean mode is active for this band
    if (state.euclideanActive[band]) return;

    const paramId = `step_b${band}_s${step}`;
    const toggleState = state.stepStates[paramId];
    const newValue = !toggleState.getValue();

    toggleState.setValue(newValue);
    updateStepVisual(band, step, newValue);
}

function updateStepVisual(band, step, active) {
    // Skip manual step visual updates when euclidean mode controls the display
    if (state.euclideanActive[band]) return;

    const cell = document.querySelector(`.step-cell[data-band="${band}"][data-step="${step}"]`);
    if (cell) {
        if (active) {
            cell.classList.add('active');
        } else {
            cell.classList.remove('active');
        }
    }
}

function updateStepCount(count) {
    state.numSteps = Math.max(2, Math.min(32, count || 16));
    updateStepVisibility();
}

function updateStepVisibility() {
    const allCells = document.querySelectorAll('.step-cell');
    allCells.forEach((cell) => {
        const step = parseInt(cell.dataset.step);
        if (step < state.numSteps) {
            cell.style.display = 'block';
        } else {
            cell.style.display = 'none';
        }
    });
}

// ============================================================================
// Playhead Update (called from C++ timer)
// ============================================================================

window.updatePlayhead = function (step, hasSignal) {
    const playhead = document.getElementById('playhead');
    const gridArea = document.querySelector('.grid-area');

    if (!playhead || !gridArea) return;

    // Hide playhead when no audio signal is present
    if (!hasSignal) {
        playhead.style.opacity = '0';
        return;
    }

    playhead.style.opacity = '1';

    // Find the actual cell at this step position (use band 0, all bands align)
    const cell = document.querySelector(`.step-cell[data-band="0"][data-step="${step}"]`);
    if (!cell || cell.style.display === 'none') return;

    // Get cell's actual position relative to grid area
    const cellRect = cell.getBoundingClientRect();
    const gridRect = gridArea.getBoundingClientRect();

    // Position playhead at the center of this cell
    const offset = cellRect.left - gridRect.left + (cellRect.width / 2);

    playhead.style.transform = `translateX(${offset}px)`;
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

    const band = bands.find((b) => b.id === bandId);
    title.textContent = `${band.name} Band Controls`;

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

    // Depth
    const depthState = state.stepStates[`band${bandId}_depth`];
    const depthSlider = document.getElementById('euc-depth');
    const depthValue = document.getElementById('euc-depth-value');

    depthSlider.value = depthState.getNormalisedValue() * 100;
    depthValue.textContent = `${Math.round(depthSlider.value)}%`;

    depthSlider.oninput = (e) => {
        const normalized = e.target.value / 100;
        depthState.setNormalisedValue(normalized);
        depthValue.textContent = `${Math.round(e.target.value)}%`;
    };
}

function updateBandModeIndicator(bandId, isEuclidean) {
    const indicator = document.getElementById(`mode-${bandId}`);
    if (indicator) {
        indicator.textContent = isEuclidean ? 'Euclidean' : 'Manual';
        indicator.classList.toggle('euclidean', isEuclidean);
    }

    const expandBtn = document.getElementById(`expand-${bandId}`);
    if (expandBtn) {
        expandBtn.classList.toggle('visible', isEuclidean);
    }
}

// ============================================================================
// Lane Actions (Clear/Random)
// ============================================================================

function clearBand(bandId) {
    if (state.euclideanActive[bandId]) return;

    for (let step = 0; step < 32; step++) {
        const paramId = `step_b${bandId}_s${step}`;
        const toggleState = state.stepStates[paramId];
        if (toggleState) {
            toggleState.setValue(false);
            updateStepVisual(bandId, step, false);
        }
    }
}

function randomizeBand(bandId) {
    if (state.euclideanActive[bandId]) return;

    for (let step = 0; step < 32; step++) {
        const paramId = `step_b${bandId}_s${step}`;
        const toggleState = state.stepStates[paramId];
        if (toggleState) {
            // 50% chance each step is enabled
            const active = Math.random() < 0.5;
            toggleState.setValue(active);
            updateStepVisual(bandId, step, active);
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

    // Close euclidean panel if switching this band to manual while panel is open
    if (!isEuclidean && state.currentBand === bandId) {
        const panel = document.getElementById('euclidean-panel');
        panel.classList.remove('visible');
        state.currentBand = null;
    }

    const cells = document.querySelectorAll(`.step-cell[data-band="${bandId}"]`);

    if (isEuclidean) {
        // Compute euclidean pattern
        const eucSteps = Math.round(state.stepStates[`band${bandId}_euc_steps`].getScaledValue());
        const eucPulses = Math.round(state.stepStates[`band${bandId}_euc_pulses`].getScaledValue());
        const eucOffset = Math.round(state.stepStates[`band${bandId}_euc_offset`].getScaledValue());

        const pattern = generateEuclidean(eucSteps, eucPulses, eucOffset);

        cells.forEach((cell) => {
            const step = parseInt(cell.dataset.step);
            cell.classList.add('euclidean-mode');
            cell.classList.remove('active');

            // Wrap via modulo to match C++ getTargetGainForBand() behavior
            const wrappedStep = step % eucSteps;
            if (pattern[wrappedStep]) {
                cell.classList.add('euclidean-active');
            } else {
                cell.classList.remove('euclidean-active');
            }
        });
    } else {
        // Restore manual step display
        cells.forEach((cell) => {
            const step = parseInt(cell.dataset.step);
            cell.classList.remove('euclidean-mode', 'euclidean-active');

            const paramId = `step_b${bandId}_s${step}`;
            const toggleState = state.stepStates[paramId];
            if (toggleState && toggleState.getValue()) {
                cell.classList.add('active');
            } else {
                cell.classList.remove('active');
            }
        });
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
// Setup Global Controls
// ============================================================================

function setupGlobalControls() {
    // Already initialized in initializeGlobalParameters()
    console.log('Global controls bound to JUCE parameters');
}

// ============================================================================
// v1.5.0: Tooltip System
// ============================================================================

function initializeTooltips() {
    const tooltipToggle = document.getElementById('tooltip-toggle');
    const tooltip = document.getElementById('tooltip');
    const pluginContainer = document.getElementById('plugin-container');

    if (!tooltipToggle || !tooltip || !pluginContainer) return;

    // Toggle button click handler
    tooltipToggle.addEventListener('click', function() {
        state.tooltipsEnabled = !state.tooltipsEnabled;
        tooltipToggle.classList.toggle('active', state.tooltipsEnabled);
        pluginContainer.classList.toggle('tooltips-enabled', state.tooltipsEnabled);

        // Hide tooltip when disabling
        if (!state.tooltipsEnabled) {
            tooltip.classList.remove('visible');
        }

        // Persist state to C++ (if native function available)
        if (window.__JUCE__ && window.__JUCE__.backend && window.__JUCE__.backend.getNativeFunction) {
            try {
                window.__JUCE__.backend.getNativeFunction('setTooltipsEnabled')(state.tooltipsEnabled);
            } catch (e) {
                // Silently ignore if not available
            }
        }
    });

    // Show tooltip on hover over elements with data-tooltip
    pluginContainer.addEventListener('mouseover', function(e) {
        if (!state.tooltipsEnabled) return;

        const target = e.target.closest('[data-tooltip]');
        if (!target) return;

        const text = target.getAttribute('data-tooltip');
        if (!text) return;

        tooltip.textContent = text;
        tooltip.classList.add('visible');

        // Position tooltip above the element
        const rect = target.getBoundingClientRect();
        const containerRect = pluginContainer.getBoundingClientRect();

        let left = rect.left - containerRect.left + rect.width / 2;
        let top = rect.top - containerRect.top - 8;

        // Constrain within container bounds
        const tooltipWidth = tooltip.offsetWidth || 200;
        const tooltipHeight = tooltip.offsetHeight || 40;

        // Keep tooltip within horizontal bounds
        if (left - tooltipWidth / 2 < 10) {
            left = tooltipWidth / 2 + 10;
        } else if (left + tooltipWidth / 2 > containerRect.width - 10) {
            left = containerRect.width - tooltipWidth / 2 - 10;
        }

        // If tooltip would go above container, show below instead
        if (top - tooltipHeight < 0) {
            top = rect.bottom - containerRect.top + 8;
        }

        tooltip.style.left = left + 'px';
        tooltip.style.top = top + 'px';
        tooltip.style.transform = 'translateX(-50%)';
    });

    // Hide tooltip on mouseout
    pluginContainer.addEventListener('mouseout', function(e) {
        const target = e.target.closest('[data-tooltip]');
        if (target) {
            tooltip.classList.remove('visible');
        }
    });
}

// v1.5.0: Restore tooltip state from C++ (called via evaluateJavascript)
window.restoreTooltipState = function(enabled) {
    state.tooltipsEnabled = !!enabled;
    const tooltipToggle = document.getElementById('tooltip-toggle');
    const pluginContainer = document.getElementById('plugin-container');
    if (tooltipToggle) tooltipToggle.classList.toggle('active', state.tooltipsEnabled);
    if (pluginContainer) pluginContainer.classList.toggle('tooltips-enabled', state.tooltipsEnabled);
};

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
            presetDropdown.innerHTML = '<div class="preset-dropdown-item" style="opacity:0.5">No presets</div>';
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
