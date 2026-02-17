/*
  ==============================================================================

    O-TextureForge — Main Application (webpack entry)
    Scatter plot, knob controllers, real-time viz

  ==============================================================================
*/

import createScatterplot from 'regl-scatterplot';

// ===== Minimal D3-compatible linear scale (avoids full D3 dependency) =====
function linearScale() {
    let _domain = [0, 1];
    let _range = [0, 1];

    function scale(value) {
        const t = (value - _domain[0]) / (_domain[1] - _domain[0]);
        return _range[0] + t * (_range[1] - _range[0]);
    }

    scale.domain = function(d) {
        if (!arguments.length) return _domain.slice();
        _domain = [d[0], d[1]];
        return scale;
    };

    scale.range = function(r) {
        if (!arguments.length) return _range.slice();
        _range = [r[0], r[1]];
        return scale;
    };

    scale.copy = function() {
        return linearScale().domain(_domain).range(_range);
    };

    return scale;
}

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
    return window.getNativeFunction(name);
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
            xScale: linearScale().domain([0, 1]).range([0, canvas.clientWidth]),
            yScale: linearScale().domain([0, 1]).range([canvas.clientHeight, 0]),
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

    // Map data coordinates to pixel coordinates using scatter plot's view transform
    let px, py;
    if (scatterViewTransform && typeof scatterViewTransform.xScale === 'function') {
        px = scatterViewTransform.xScale(cx);
        py = scatterViewTransform.yScale(cy);
    } else {
        // Fallback before scatter plot is initialized
        px = cx * w;
        py = (1 - cy) * h;
    }

    // Compute radius in pixel space using the scale transform
    let radius;
    if (scatterViewTransform && typeof scatterViewTransform.xScale === 'function') {
        const edge = scatterViewTransform.xScale(cx + variation * 0.5);
        radius = Math.abs(edge - px);
    } else {
        radius = variation * Math.min(w, h) * 0.3;
    }

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

// ===== Toast / Notification =====
function showToast(message, duration) {
    duration = duration || 4000;
    let toast = document.getElementById('toast-notification');
    if (!toast) {
        toast = document.createElement('div');
        toast.id = 'toast-notification';
        toast.style.cssText = 'position:fixed;bottom:20px;left:50%;transform:translateX(-50%);' +
            'background:rgba(43,43,43,0.95);color:#C9A27B;padding:10px 20px;border-radius:6px;' +
            'font-family:inherit;font-size:13px;z-index:1000;transition:opacity 0.3s;border:1px solid #8B6914;';
        document.body.appendChild(toast);
    }
    toast.textContent = message;
    toast.style.opacity = '1';
    toast.style.display = 'block';
    clearTimeout(toast._timer);
    toast._timer = setTimeout(function() {
        toast.style.opacity = '0';
        setTimeout(function() { toast.style.display = 'none'; }, 300);
    }, duration);
}

// ===== File Size Warning Overlay =====
function showFileSizeWarning(sizeMB) {
    let overlay = document.getElementById('file-size-overlay');
    if (!overlay) {
        overlay = document.createElement('div');
        overlay.id = 'file-size-overlay';
        overlay.style.cssText = 'position:fixed;top:0;left:0;width:100%;height:100%;' +
            'background:rgba(0,0,0,0.7);display:flex;align-items:center;justify-content:center;z-index:999;';
        overlay.innerHTML = '<div style="background:#2B2B2B;border:1px solid #8B6914;border-radius:8px;' +
            'padding:24px;text-align:center;max-width:320px;color:#C9A27B;font-family:inherit;">' +
            '<p id="file-size-msg" style="margin:0 0 16px;font-size:14px;"></p>' +
            '<button id="btn-load-anyway" style="background:#8B6914;color:#F5E6D3;border:none;' +
            'padding:8px 16px;border-radius:4px;cursor:pointer;margin-right:8px;font-size:13px;">Load Anyway</button>' +
            '<button id="btn-cancel-load" style="background:transparent;color:#C9A27B;border:1px solid #C9A27B;' +
            'padding:8px 16px;border-radius:4px;cursor:pointer;font-size:13px;">Cancel</button></div>';
        document.body.appendChild(overlay);

        document.getElementById('btn-load-anyway').addEventListener('click', function() {
            overlay.style.display = 'none';
            var confirmLoad = getNativeFunction('confirmLargeLoad');
            confirmLoad();
        });
        document.getElementById('btn-cancel-load').addEventListener('click', function() {
            overlay.style.display = 'none';
        });
    }
    document.getElementById('file-size-msg').textContent =
        'Large file: ' + sizeMB.toFixed(1) + ' MB. This may use significant memory.';
    overlay.style.display = 'flex';
}

// ===== UMAP Progress =====
function updateUmapProgress(progress) {
    const container = document.querySelector('.umap-progress-container');
    const fill = document.querySelector('.umap-progress-fill');
    if (!container || !fill) return;

    if (progress >= 0 && progress < 1) {
        container.classList.add('active');
        fill.style.width = (progress * 100) + '%';

        // Show cancel button
        let cancelBtn = document.getElementById('umap-cancel-btn');
        if (!cancelBtn) {
            cancelBtn = document.createElement('button');
            cancelBtn.id = 'umap-cancel-btn';
            cancelBtn.textContent = 'Cancel';
            cancelBtn.style.cssText = 'position:absolute;right:8px;top:50%;transform:translateY(-50%);' +
                'background:transparent;color:#C9A27B;border:1px solid #C9A27B;border-radius:3px;' +
                'padding:2px 8px;font-size:11px;cursor:pointer;';
            cancelBtn.addEventListener('click', function() {
                var cancelUmap = getNativeFunction('cancelUmap');
                cancelUmap();
            });
            container.style.position = 'relative';
            container.appendChild(cancelBtn);
        }
        cancelBtn.style.display = 'block';
    } else {
        container.classList.remove('active');
        var cancelBtn2 = document.getElementById('umap-cancel-btn');
        if (cancelBtn2) cancelBtn2.style.display = 'none';
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
                if (points.length === 0) {
                    // Empty corpus — keep placeholder visible
                    const placeholder = document.querySelector('.scatter-placeholder');
                    if (placeholder) {
                        placeholder.style.display = 'block';
                        placeholder.innerHTML = '<span class="fleuron">&#10087;</span>Drop audio file here';
                    }
                    return;
                }
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

    // UMAP cancelled — PCA layout preserved
    window.__JUCE__.backend.addEventListener('umapCancelled', () => {
        updateUmapProgress(1.0);
        showToast('UMAP cancelled \u2014 using PCA layout', 3000);
    });

    // Load failed — invalid file format
    window.__JUCE__.backend.addEventListener('loadFailed', (event) => {
        try {
            const data = typeof event === 'string' ? JSON.parse(event) : event;
            showToast(data.reason || 'Failed to load file', 5000);
        } catch (e) {}
    });

    // File size warning
    window.__JUCE__.backend.addEventListener('fileSizeWarning', (event) => {
        try {
            const data = typeof event === 'string' ? JSON.parse(event) : event;
            showFileSizeWarning(data.sizeMB);
        } catch (e) {}
    });

    // Corpus missing on session restore
    window.__JUCE__.backend.addEventListener('corpusMissing', (event) => {
        try {
            const data = typeof event === 'string' ? JSON.parse(event) : event;
            const placeholder = document.querySelector('.scatter-placeholder');
            if (placeholder) {
                placeholder.style.display = 'block';
                placeholder.innerHTML = '<span class="fleuron">&#10087;</span>File not found: ' +
                    (data.path || 'unknown') + '<br>Drop a new file to continue.';
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

// ===== File Browse =====
function setupFileBrowse() {
    const browseForFile = getNativeFunction('browseForFile');

    // Click on drop zone opens file browser
    const dropZone = document.getElementById('drop-zone');
    if (dropZone) {
        dropZone.style.cursor = 'pointer';
        dropZone.addEventListener('click', () => browseForFile());
    }

    // Click on scatter placeholder opens file browser
    const placeholder = document.querySelector('.scatter-placeholder');
    if (placeholder) {
        placeholder.style.cursor = 'pointer';
        placeholder.addEventListener('click', () => browseForFile());
    }
}

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

        // Setup file browse click handlers
        setupFileBrowse();

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
