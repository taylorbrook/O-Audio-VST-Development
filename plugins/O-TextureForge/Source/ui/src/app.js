/*
   This file is part of O-TextureForge, an Ouaricon Audio plugin.
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

// ===== i18n bridge =====
//
// THE BUNDLE CANNOT IMPORT THE LABEL TABLE. `import ... from './i18n.js'` here
// would be resolved by WEBPACK at build time and inlined into app.bundle.js,
// at which point public/js/i18n.js is embedded in the binary, served from
// getResource(), and read by nobody — the table shipped twice and editable in
// only one of the two copies.
//
// So the label runtime lives in public/js/i18n_init.js, a real ES module the
// page loads by src, and it exposes window.__setLabel for exactly this case.
// The canon block carries that export verbatim on all 43 plugins; O-Bitrot
// needs it because its controller is an inline module with nowhere to put an
// export, and this plugin needs it because its controller is a bundle.
//
// An element handed to this function becomes a [data-i18n] element from that
// moment on, so the language-change sweep owns it thereafter. There is no
// second code path and no subscription list that can go stale in the other
// language — the failure contract section 3 exists to prevent.
//
// NO ENGLISH FALLBACK IS SPELLED HERE. Repeating the English string in this
// file would be a second copy of a string that already lives in i18n.js, free
// to drift, and check-i18n assertion 12 would report it as unkeyed prose. If
// the runtime is missing the element shows its KEY, which is visible, greppable
// and unmistakable — a blank control would look like a layout bug instead.
//
// ── WHY THE CALL SITES SPELL window.__setLabel IN FULL ──────────────────────
//
// A local wrapper named setLabel() would read better here and would be WRONG.
// This file is minified into app.bundle.js, and terser mangles a local function
// name to a single letter — so `setLabel(el, 'toast.loadFailed')` becomes
// something like `Z(Q(),"toast.loadFailed")` in the shipped bundle, which is
// the only copy check-i18n can scan. The key is still in there as a literal,
// but nothing marks it as a LABEL REFERENCE, so assertion 15 reports every
// JS-only key as DEAD and assertion 13 cannot see the argument list at all.
//
// Terser does not mangle PROPERTY names, so `window.__setLabel(...)` survives
// minification verbatim — and it is the spelling i18n-extract.js's scanner
// already looks for, because the canon publishes window.__setLabel for exactly
// this "a sibling module writes a localized label" case.
//
// THIS STUB IS A SAFETY NET, NOT THE IMPLEMENTATION. index.html loads
// js/i18n_init.js as a deferred module, which runs AFTER this classic script
// and overwrites the binding with the canon's own setLabel. The stub survives
// only if that module fails to load — the embed-and-serve 404 that check-i18n
// assertion 8 exists to prevent — and then every caption shows its key rather
// than going blank.
if (typeof window.__setLabel !== 'function') {
    window.__setLabel = function (el, key) {
        if (!el) return;
        console.warn('i18n runtime not loaded; showing key for', key);
        el.textContent = key;
    };
}

// Text that arrives from C++ at runtime and cannot be keyed. Clears any key the
// element is still carrying from an earlier localized message, so the next
// language switch does not overwrite the message with a stale caption.
function setRawText(el, text) {
    if (!el) return;
    delete el.dataset.i18n;
    delete el.dataset.i18nVars;
    delete el.dataset.label;
    el.textContent = text;
}

// The placeholder is a fleuron glyph element plus a text span (v1.1.0). Only
// the span is keyed; writing the container with innerHTML, as v1.0.2 did, would
// re-author the glyph on every message and would be a markup path for a
// machine-drafted French string.
function placeholderText() {
    return document.querySelector('.scatter-placeholder .placeholder-text');
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
            window.__setLabel(placeholderText(), 'placeholder.webglUnavailable');
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

    const dpr = window.devicePixelRatio || 1;
    const w = canvas.clientWidth;
    const h = canvas.clientHeight;
    canvas.width = Math.round(w * dpr);
    canvas.height = Math.round(h * dpr);
    cursorOverlayCtx = canvas.getContext('2d');
}

function drawCursor(cx, cy, variation) {
    if (!cursorOverlayCtx) return;

    const ctx = cursorOverlayCtx;
    const canvas = ctx.canvas;
    const dpr = window.devicePixelRatio || 1;
    const w = canvas.clientWidth;
    const h = canvas.clientHeight;
    if (w === 0 || h === 0) return;

    // Sync backing store if canvas was resized
    const bw = Math.round(w * dpr);
    const bh = Math.round(h * dpr);
    if (canvas.width !== bw || canvas.height !== bh) {
        canvas.width = bw;
        canvas.height = bh;
    }

    // DPR transform: draw in CSS pixel coordinates, scale to backing store
    ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
    ctx.clearRect(0, 0, w, h);

    // Map data coordinates to CSS pixel coordinates using scatter plot's view transform
    let px, py;
    if (scatterViewTransform && typeof scatterViewTransform.xScale === 'function') {
        px = scatterViewTransform.xScale(cx);
        py = scatterViewTransform.yScale(cy);
    } else {
        // Fallback before scatter plot is initialized
        px = cx * w;
        py = (1 - cy) * h;
    }

    // Compute radius in CSS pixel space using the scale transform
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
// Two arms, deliberately. showToastNode() is the localized path: the caller
// passes a writer that names its key as a LITERAL, because a helper forwarding
// a `key` variable into window.__setLabel would be a computed key, which
// check-i18n assertion 13 rejects — a computed key cannot be checked, and a raw
// copy string there would ship English. showToast() takes text that arrived
// from C++ and cannot be keyed at all — see the loadFailed listener and the
// I18N_EXEMPT entry for 'Unsupported format: '.
function showToast(message, duration) {
    showToastNode(duration, (el) => setRawText(el, message));
}

function showToastNode(duration, write) {
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
    write(toast);
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
            'padding:8px 16px;border-radius:4px;cursor:pointer;margin-right:8px;font-size:13px;"></button>' +
            '<button id="btn-cancel-load" style="background:transparent;color:#C9A27B;border:1px solid #C9A27B;' +
            'padding:8px 16px;border-radius:4px;cursor:pointer;font-size:13px;"></button></div>';
        document.body.appendChild(overlay);

        // v1.1.0: the two captions are set through the label table rather than
        // authored into the innerHTML above. The markup is structure and inline
        // style only; no prose survives in it.
        window.__setLabel(document.getElementById('btn-load-anyway'), 'dialog.loadAnyway');
        window.__setLabel(document.getElementById('btn-cancel-load'), 'action.cancel');

        document.getElementById('btn-load-anyway').addEventListener('click', function() {
            overlay.style.display = 'none';
            var confirmLoad = getNativeFunction('confirmLargeLoad');
            confirmLoad();
        });
        document.getElementById('btn-cancel-load').addEventListener('click', function() {
            overlay.style.display = 'none';
        });
    }
    // COMPOSED, with the number as a {size} token rather than string
    // concatenation: the French word order differs and the unit symbol is Mo,
    // so the sentence has to be authored as one string per language. The value
    // itself stays a readout and is not translated (D-03).
    window.__setLabel(document.getElementById('file-size-msg'), 'dialog.largeFile',
             { size: sizeMB.toFixed(1) });
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
            window.__setLabel(cancelBtn, 'action.cancel');
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
                        // The same key the drop zone carries: one sentence, one
                        // entry, rather than two copies free to drift apart.
                        window.__setLabel(placeholderText(), 'label.dropZone');
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
        showToastNode(3000, (el) => window.__setLabel(el, 'toast.umapCancelled'));
    });

    // Load failed — invalid file format
    window.__JUCE__.backend.addEventListener('loadFailed', (event) => {
        try {
            const data = typeof event === 'string' ? JSON.parse(event) : event;
            // A reason supplied by C++ is shown VERBATIM and is not localized:
            // it is authored in CorpusLoader.cpp and reaches the page as an
            // opaque string. Only the page-side fallback is keyed.
            if (data.reason) showToast(data.reason, 5000);
            else showToastNode(5000, (el) => window.__setLabel(el, 'toast.loadFailed'));
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
                // COMPOSED, and ONE string with no branch in it. The <br> is
                // gone: the line break is \n in the table with
                // white-space: pre-line on the span, because a markup tag in a
                // localized string needs innerHTML and check-i18n assertion 9
                // rejects an angle bracket in i18n.js for exactly that reason.
                //
                // v1.1.0 first wrote `data.path || 'placeholder.unknownPath'`
                // HERE, inside the argument, carried over from v1.0.2's
                // `(data.path || 'unknown')`. That is the shape contract §6
                // forbids — copy selected by a condition rather than authored
                // around one — and the condition cannot even fire: C++ reaches
                // onCorpusMissing only inside `else if (savedPath.isNotEmpty())`
                // (PluginProcessor.cpp:294) and its emitter always writes a
                // path field (PluginEditor.cpp:345-347). So the fallback guarded
                // a case its only producer cannot produce, and its alternate
                // branch was a LABELS key nothing else referenced.
                //
                // The path is normalised to a string at the PAYLOAD BOUNDARY
                // below — JSON hygiene over a value that crossed a process
                // boundary, not a choice between two wordings — and the
                // sentence is authored so it reads correctly whether the path
                // is long, short or empty: it sits on its own line rather than
                // inside the sentence, which is also what makes a 200-character
                // path legible against the 320 px box.
                const missingPath = String(data.path || '');
                window.__setLabel(placeholderText(), 'placeholder.fileNotFound',
                         { path: missingPath });
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
