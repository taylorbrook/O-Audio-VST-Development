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
    soloedBand: -1,     // v1.13.0: Which band is soloed (-1 = none)
    preSoloEnables: [true, true, true, true],  // Enable states before solo was engaged
};

// Cached DOM references — populated once after renderGrid()
// cachedCells[band][step] holds direct references to all 128 step-cell elements
let cachedCells = [[], [], [], []];
let cachedGridArea = null;

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
    setupEuclideanPanel();
    initializeEuclideanListeners();
    initializeBandRateDropdowns();

    // v1.5.0: Initialize tooltip system
    initializeTooltips();

    // v1.6.0: Initialize preset manager
    initializePresetManager();

    // v1.13.0: Initialize mute/solo listeners (after grid rendered)
    initializeMuteSoloListeners();

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

    // Band label with name, M/S buttons, and frequency range
    const label = document.createElement('div');
    label.className = 'band-label';
    label.setAttribute('data-tooltip', `${band.name} band: Shows the frequency range for this band. Frequencies are set by the crossover sliders between bands.`);

    const labelTop = document.createElement('div');
    labelTop.className = 'band-label-top';

    const nameSpan = document.createElement('strong');
    nameSpan.textContent = band.name;
    labelTop.appendChild(nameSpan);

    // v1.13.0: Mute button
    const muteBtn = document.createElement('button');
    muteBtn.className = 'ms-btn mute-btn';
    muteBtn.id = `mute-${band.id}`;
    muteBtn.textContent = 'M';
    muteBtn.title = `Mute ${band.name} band`;
    muteBtn.setAttribute('data-tooltip', `Mute: Bypass the ${band.name} band sequencer (pass audio through unaffected).`);
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
    soloBtn.title = `Solo ${band.name} band`;
    soloBtn.setAttribute('data-tooltip', `Solo: Mute all other bands so only ${band.name} is sequenced. Click again to unsolo.`);
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

    // v1.7.0: Per-band rate dropdown
    const rateSelect = document.createElement('select');
    rateSelect.className = 'band-rate-dropdown';
    rateSelect.id = `band-rate-${band.id}`;
    rateSelect.setAttribute('data-tooltip', `${band.name} Rate: Override the global rate for this band. "Global" follows the main Rate knob. Set a specific division for polymetric sequencing.`);
    const rateOptions = ['Global', '1/1', '1/2', '1/4', '1/8', '1/16', '1/32', '1/8T', '1/16T', '1/4D', '1/8D'];
    rateOptions.forEach((label, idx) => {
        const opt = document.createElement('option');
        opt.value = idx;
        opt.textContent = label;
        rateSelect.appendChild(opt);
    });
    bandRow.appendChild(rateSelect);

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

    // Expand button (always visible — panel has Phase/Depth for both modes)
    const expandBtn = document.createElement('button');
    expandBtn.className = 'band-expand-btn visible';
    expandBtn.id = `expand-${band.id}`;
    expandBtn.textContent = '▶';
    expandBtn.setAttribute('data-tooltip', 'Expand: Opens the band controls panel (Phase, Depth, and Euclidean settings).');
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
        const bandCells = cachedCells[b];
        for (let s = 0; s < bandCells.length; s++) {
            bandCells[s].style.display = s < state.numSteps ? 'block' : 'none';
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

    // Expand button always visible (Phase/Depth accessible in both modes)
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

        const pattern = generateEuclidean(eucSteps, eucPulses, eucOffset);

        for (let step = 0; step < cells.length; step++) {
            const cell = cells[step];
            cell.classList.add('euclidean-mode');
            cell.classList.remove('active');

            // Wrap via modulo to match C++ getTargetGainForBand() behavior
            const wrappedStep = step % eucSteps;
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
