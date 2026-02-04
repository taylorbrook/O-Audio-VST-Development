/* O-FreqPulse - Main Application */

import * as Juce from './juce/index.js';

// State
const state = {
    currentBand: null,  // Expanded Euclidean panel band
    numSteps: 16,       // Current step count (4, 8, 16, or 32)
    stepStates: {},     // SliderState/ToggleState objects by parameter ID
};

// Band configuration
const bands = [
    { id: 0, name: 'SUB', freq: '20Hz-120Hz' },
    { id: 1, name: 'LOW', freq: '120Hz-500Hz' },
    { id: 2, name: 'MID', freq: '500Hz-4kHz' },
    { id: 3, name: 'HIGH', freq: '4kHz-20kHz' },
];

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

    // Steps dropdown
    const stepsState = Juce.getComboBoxState('steps');
    const stepsDropdown = document.getElementById('steps');

    stepsDropdown.selectedIndex = stepsState.getChoiceIndex();
    updateStepCount(stepsState.getChoiceIndex());

    stepsDropdown.addEventListener('change', (e) => {
        stepsState.setChoiceIndex(e.target.selectedIndex);
        updateStepCount(e.target.selectedIndex);
    });

    stepsState.valueChangedEvent.addListener(() => {
        stepsDropdown.selectedIndex = stepsState.getChoiceIndex();
        updateStepCount(stepsState.getChoiceIndex());
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

        // Frequency bands (bound but not editable in v1.0)
        const lowState = Juce.getSliderState(`band${bandId}_low`);
        const highState = Juce.getSliderState(`band${bandId}_high`);
        state.stepStates[`band${bandId}_low`] = lowState;
        state.stepStates[`band${bandId}_high`] = highState;
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

function renderGrid() {
    const container = document.getElementById('grid-container');
    container.innerHTML = '';

    for (const band of bands) {
        const bandRow = document.createElement('div');
        bandRow.className = 'band-row';
        bandRow.dataset.band = band.id;

        // Band label
        const label = document.createElement('div');
        label.className = 'band-label';
        label.innerHTML = `<strong>${band.name}</strong><br>${band.freq}`;
        bandRow.appendChild(label);

        // Steps container
        const stepsContainer = document.createElement('div');
        stepsContainer.className = 'steps-container';

        for (let step = 0; step < 32; step++) {
            const cell = document.createElement('div');
            cell.className = 'step-cell';
            cell.dataset.band = band.id;
            cell.dataset.step = step;

            // Click handler for step toggle
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
        clearBtn.addEventListener('click', () => {
            clearBand(band.id);
        });
        laneActions.appendChild(clearBtn);

        const randomBtn = document.createElement('button');
        randomBtn.className = 'lane-btn random-btn';
        randomBtn.textContent = '⚄';
        randomBtn.title = 'Randomize steps';
        randomBtn.addEventListener('click', () => {
            randomizeBand(band.id);
        });
        laneActions.appendChild(randomBtn);

        bandRow.appendChild(laneActions);

        // Band mode indicator
        const modeIndicator = document.createElement('div');
        modeIndicator.className = 'band-mode';
        modeIndicator.textContent = 'Manual';
        modeIndicator.id = `mode-${band.id}`;
        bandRow.appendChild(modeIndicator);

        // Expand button
        const expandBtn = document.createElement('button');
        expandBtn.className = 'band-expand-btn';
        expandBtn.textContent = '▶';
        expandBtn.addEventListener('click', () => {
            openEuclideanPanel(band.id);
        });
        bandRow.appendChild(expandBtn);

        container.appendChild(bandRow);
    }

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
    const paramId = `step_b${band}_s${step}`;
    const toggleState = state.stepStates[paramId];
    const newValue = !toggleState.getValue();

    toggleState.setValue(newValue);
    updateStepVisual(band, step, newValue);
}

function updateStepVisual(band, step, active) {
    const cell = document.querySelector(`.step-cell[data-band="${band}"][data-step="${step}"]`);
    if (cell) {
        if (active) {
            cell.classList.add('active');
        } else {
            cell.classList.remove('active');
        }
    }
}

function updateStepCount(stepsIndex) {
    const stepCounts = [4, 8, 16, 32];
    state.numSteps = stepCounts[stepsIndex] || 16;
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

window.updatePlayhead = function (step) {
    const playhead = document.getElementById('playhead');
    const gridArea = document.querySelector('.grid-area');

    if (!playhead || !gridArea) return;

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
    // Mode toggle (euclidean on/off)
    const eucOnState = state.stepStates[`band${bandId}_euc_on`];
    const modeToggle = document.getElementById('euc-mode-toggle');
    const modeLabels = modeToggle.querySelectorAll('.mode-label');

    modeLabels.forEach((label, index) => {
        if (index === (eucOnState.getValue() ? 1 : 0)) {
            label.classList.add('active');
        } else {
            label.classList.remove('active');
        }
    });

    modeLabels.forEach((label, index) => {
        label.onclick = () => {
            eucOnState.setValue(index === 1);
            syncEuclideanControls(bandId);
            updateBandModeIndicator(bandId, index === 1);
        };
    });

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
    }
}

// ============================================================================
// Lane Actions (Clear/Random)
// ============================================================================

function clearBand(bandId) {
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
// Setup Global Controls
// ============================================================================

function setupGlobalControls() {
    // Already initialized in initializeGlobalParameters()
    console.log('Global controls bound to JUCE parameters');
}
