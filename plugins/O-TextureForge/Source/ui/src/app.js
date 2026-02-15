/*
  ==============================================================================

    O-TextureForge — Main Application (webpack entry)
    Scatter plot, knob controllers, real-time viz

  ==============================================================================
*/

import createScatterplot from 'regl-scatterplot';

// ===== Constants =====
const ANGLE_MIN = -135;
const ANGLE_RANGE = 270;
const SENSITIVITY = 0.005;

// ===== State =====
let scatterplot = null;
let corpusPoints = null;
let cursorOverlayCtx = null;
let currentCursorX = 0.5;
let currentCursorY = 0.5;
let currentVariation = 0.2;
let scatterViewTransform = null;

// ===== JUCE Bridge (loaded via module script, available globally) =====
function getSliderState(name) {
    return window.__JUCE__ ? window.getSliderState(name) : null;
}

function getComboBoxState(name) {
    return window.__JUCE__ ? window.getComboBoxState(name) : null;
}

function getNativeFunction(name) {
    if (!window.__JUCE__) return () => Promise.resolve(null);
    const f = function() {
        const promiseId = window.__JUCE__._promiseHandler
            ? window.__JUCE__._promiseHandler.createPromise()
            : [0, Promise.resolve(null)];
        window.__JUCE__.backend.emitEvent('__juce__invoke', {
            name: name,
            params: Array.prototype.slice.call(arguments),
            resultId: promiseId[0]
        });
        return promiseId[1];
    };
    return f;
}

// ===== Knob Controller =====
function setupKnob(paramId, sliderState, formatter, defaultNorm) {
    const knobEl = document.querySelector(`.knob[data-param="${paramId}"]`);
    if (!knobEl || !sliderState) return;

    const indicator = knobEl.querySelector('.knob-indicator');
    const valueEl = document.querySelector(`[data-value="${paramId}"]`);
    let isDragging = false;
    let lastY = 0;

    function updateDisplay(norm) {
        const angle = ANGLE_MIN + norm * ANGLE_RANGE;
        if (indicator) indicator.style.transform = `rotate(${angle}deg)`;
        if (valueEl) valueEl.textContent = formatter(norm);
    }

    sliderState.valueChangedEvent.addListener(() => {
        updateDisplay(sliderState.getNormalisedValue());
    });

    sliderState.propertiesChangedEvent.addListener(() => {
        updateDisplay(sliderState.getNormalisedValue());
    });

    knobEl.addEventListener('mousedown', (e) => {
        isDragging = true;
        lastY = e.clientY;
        sliderState.sliderDragStarted();
        e.preventDefault();
    });

    document.addEventListener('mousemove', (e) => {
        if (!isDragging) return;
        const deltaY = lastY - e.clientY;
        const norm = sliderState.getNormalisedValue();
        const newNorm = Math.max(0, Math.min(1, norm + deltaY * SENSITIVITY));
        sliderState.setNormalisedValue(newNorm);
        lastY = e.clientY;
    });

    document.addEventListener('mouseup', () => {
        if (isDragging) {
            isDragging = false;
            sliderState.sliderDragEnded();
        }
    });

    knobEl.addEventListener('dblclick', (e) => {
        e.preventDefault();
        sliderState.setNormalisedValue(defaultNorm);
    });

    // Initial display
    updateDisplay(sliderState.getNormalisedValue());
}

// ===== MIDI Mode Dropdown =====
function setupMidiMode(comboState) {
    const selectEl = document.getElementById('midi-mode');
    if (!selectEl || !comboState) return;

    comboState.valueChangedEvent.addListener(() => {
        selectEl.selectedIndex = comboState.getChoiceIndex();
    });

    comboState.propertiesChangedEvent.addListener(() => {
        // Populate options from backend
        selectEl.innerHTML = '';
        if (comboState.properties.choices) {
            comboState.properties.choices.forEach((choice, i) => {
                const opt = document.createElement('option');
                opt.value = i;
                opt.textContent = choice;
                selectEl.appendChild(opt);
            });
        }
        selectEl.selectedIndex = comboState.getChoiceIndex();
    });

    selectEl.addEventListener('change', () => {
        comboState.setChoiceIndex(parseInt(selectEl.value));
    });
}

// ===== Scatter Plot =====
function initScatterPlot(data) {
    const canvas = document.getElementById('scatter-canvas');
    if (!canvas) return;

    // Hide placeholder
    const placeholder = document.querySelector('.scatter-placeholder');
    if (placeholder) placeholder.style.display = 'none';

    corpusPoints = data;

    try {
        scatterplot = createScatterplot({
            canvas,
            width: canvas.clientWidth,
            height: canvas.clientHeight,
            pointSize: 4,
            opacity: 0.75,
            backgroundColor: [0, 0, 0, 0],
            showReticle: false,
            deselectOnDblClick: true,
            deselectOnEscape: true,
            mouseMode: 'panZoom',
        });

        // Earth-tone color mapping
        scatterplot.set({
            pointColor: ['#8B6914', '#C9A27B', '#A0522D', '#6B8E4E'],
            pointColorActive: '#6B8E4E',
            pointColorHover: '#C9A27B',
            pointSize: [3, 8],
            opacityInactiveMax: 0.4,
            colorBy: 'valueA',
            sizeBy: 'valueB',
        });

        // Draw points: [x, y, pitchNorm (color), energyNorm (size)]
        scatterplot.draw(data, {
            zDataType: 'continuous',
            wDataType: 'continuous',
        });

        // Click to select grain
        scatterplot.subscribe('select', ({ points }) => {
            if (points.length > 0) {
                const grainIdx = points[0];
                const selectGrain = getNativeFunction('selectGrain');
                selectGrain(grainIdx);

                // Also update scatter position from point coordinates
                if (corpusPoints && corpusPoints[grainIdx]) {
                    const pt = corpusPoints[grainIdx];
                    const setPos = getNativeFunction('setScatterPosition');
                    setPos(pt[0], pt[1]);
                }
            }
        });

        // Track view changes for cursor overlay coordinate mapping
        scatterplot.subscribe('view', ({ camera, xScale, yScale }) => {
            scatterViewTransform = { camera, xScale, yScale };
        });

    } catch (err) {
        console.error('Failed to initialize scatter plot:', err);
        if (placeholder) {
            placeholder.style.display = 'block';
            placeholder.innerHTML = '<span class="fleuron">&#10087;</span>WebGL unavailable';
        }
    }
}

// ===== Active Grain Visualization =====
function updateActiveGrains(activeIndices) {
    if (!scatterplot || !activeIndices || activeIndices.length === 0) return;
    scatterplot.select(activeIndices, { preventEvent: true });
}

// ===== Cursor Crosshair + Radius Circle =====
function initCursorOverlay() {
    const canvas = document.getElementById('cursor-overlay');
    if (!canvas) return;

    canvas.width = canvas.clientWidth;
    canvas.height = canvas.clientHeight;
    cursorOverlayCtx = canvas.getContext('2d');
}

function drawCursor(cx, cy, variation) {
    if (!cursorOverlayCtx) return;

    const ctx = cursorOverlayCtx;
    const w = ctx.canvas.width;
    const h = ctx.canvas.height;

    ctx.clearRect(0, 0, w, h);

    // Map normalized 0-1 coords to canvas pixels
    const px = cx * w;
    const py = (1 - cy) * h;  // Flip Y for canvas
    const radius = variation * Math.min(w, h) * 0.3;

    // Crosshair lines
    ctx.strokeStyle = 'rgba(139, 105, 20, 0.6)';
    ctx.lineWidth = 1;
    ctx.setLineDash([4, 4]);

    ctx.beginPath();
    ctx.moveTo(px, 0);
    ctx.lineTo(px, h);
    ctx.stroke();

    ctx.beginPath();
    ctx.moveTo(0, py);
    ctx.lineTo(w, py);
    ctx.stroke();

    // Radius circle
    ctx.setLineDash([]);
    ctx.strokeStyle = 'rgba(139, 105, 20, 0.4)';
    ctx.lineWidth = 1.5;
    ctx.beginPath();
    ctx.arc(px, py, radius, 0, Math.PI * 2);
    ctx.stroke();

    // Center dot
    ctx.fillStyle = '#8B6914';
    ctx.beginPath();
    ctx.arc(px, py, 3, 0, Math.PI * 2);
    ctx.fill();
}

// ===== UMAP Progress =====
function updateUmapProgress(progress) {
    const container = document.querySelector('.umap-progress-container');
    const fill = document.querySelector('.umap-progress-fill');
    if (!container || !fill) return;

    if (progress >= 0 && progress < 1) {
        container.classList.add('active');
        fill.style.width = (progress * 100) + '%';
    } else {
        container.classList.remove('active');
    }
}

// ===== PCA -> UMAP Transition =====
function transitionToUmap(newPoints) {
    if (!scatterplot || !newPoints) return;

    corpusPoints = newPoints;
    scatterplot.draw(newPoints, {
        transition: true,
        transitionDuration: 1500,
    });
}

// ===== Event Listeners from C++ =====
function listenForEvents() {
    if (!window.__JUCE__) return;

    // Corpus loaded event
    window.__JUCE__.backend.addEventListener('corpusLoaded', (event) => {
        const getCorpusData = getNativeFunction('getCorpusData');
        getCorpusData().then((data) => {
            if (data) {
                const points = JSON.parse(data);
                initScatterPlot(points);
            }
        });
    });

    // Real-time viz update (30Hz)
    window.__JUCE__.backend.addEventListener('vizUpdate', (event) => {
        try {
            const viz = typeof event === 'string' ? JSON.parse(event) : event;
            currentCursorX = viz.cx;
            currentCursorY = viz.cy;
            drawCursor(currentCursorX, currentCursorY, currentVariation);

            // Highlight active grains
            if (viz.g && viz.g.length > 0) {
                const indices = viz.g.map(g => g.i);
                updateActiveGrains(indices);
            }
        } catch (e) {
            // Silent fail on parse errors
        }
    });

    // UMAP progress
    window.__JUCE__.backend.addEventListener('umapProgress', (event) => {
        try {
            const data = typeof event === 'string' ? JSON.parse(event) : event;
            updateUmapProgress(data.progress);
        } catch (e) {}
    });

    // UMAP complete
    window.__JUCE__.backend.addEventListener('umapComplete', (event) => {
        try {
            const data = typeof event === 'string' ? JSON.parse(event) : event;
            updateUmapProgress(1.0);
            if (data) {
                const points = Array.isArray(data) ? data : JSON.parse(data);
                transitionToUmap(points);
            }
        } catch (e) {}
    });
}

// ===== Formatters =====
const fmtPercent = (n) => Math.round(n * 100) + '%';
const fmtDb = (n) => {
    // Params have getNormalisedValue 0-1, but display needs scaled value
    // For -60..+12 range, use scaled
    return '';  // Value comes from propertiesChanged
};

// ===== Initialization =====
document.addEventListener('DOMContentLoaded', () => {
    // Wait for JUCE bridge
    const initInterval = setInterval(() => {
        if (!window.__JUCE__) return;
        clearInterval(initInterval);

        // Import functions from JUCE bridge module (already loaded as module script)
        // The bridge auto-creates slider/combobox states from initialisationData

        // Setup macro knobs (right panel)
        setupKnob('energy', window.getSliderState('energy'), fmtPercent, 0.5);
        setupKnob('brightness', window.getSliderState('brightness'), fmtPercent, 0.5);
        setupKnob('texture', window.getSliderState('texture'), fmtPercent, 0.5);
        setupKnob('scatterX', window.getSliderState('scatterX'), fmtPercent, 0.5);
        setupKnob('scatterY', window.getSliderState('scatterY'), fmtPercent, 0.5);
        setupKnob('variation', window.getSliderState('variation'), fmtPercent, 0.2);

        // Setup bottom strip knobs
        setupKnob('position', window.getSliderState('position'), fmtPercent, 0.0);
        setupKnob('grainSize', window.getSliderState('grainSize'), (n) => '', 0.08);
        setupKnob('grainDensity', window.getSliderState('grainDensity'), (n) => '', 0.109);
        setupKnob('crossfade', window.getSliderState('crossfade'), fmtPercent, 0.5);
        setupKnob('outputGain', window.getSliderState('outputGain'), (n) => '', 0.833);

        // Setup MIDI mode dropdown
        setupMidiMode(window.getComboBoxState('midiMode'));

        // Track variation param for cursor radius
        const variationState = window.getSliderState('variation');
        if (variationState) {
            variationState.valueChangedEvent.addListener(() => {
                currentVariation = variationState.getNormalisedValue();
            });
        }

        // Initialize cursor overlay
        initCursorOverlay();

        // Listen for C++ events
        listenForEvents();

    }, 50);
});

// Export for potential external use
window.TextureForgeUI = {
    initScatterPlot,
    updateActiveGrains,
    transitionToUmap,
    updateUmapProgress,
};
