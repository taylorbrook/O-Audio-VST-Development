/*
   This file is part of O-SpectralShaper, an Ouaricon Audio plugin.
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
 * O-SpectralShaper Main Application
 *
 * Phase 3.1: Parameter binding with JUCE relays ✓
 * Phase 3.2: Curve editors ✓
 * Phase 3.3: Spectrogram visualization ✓
 */

import * as Juce from './juce/index.js';
import { RotaryKnob } from './components/RotaryKnob.js';
import { FreehandCurve } from './components/FreehandCurve.js';
import { NodeCurve } from './components/NodeCurve.js';
import { Spectrogram } from './components/Spectrogram.js';
import { PresetManager } from '../modules/preset-manager.js';

// ============================================================================
// APPLICATION STATE
// ============================================================================

// Curve accents, Ouaricon Naturalist palette. Earth tones chosen to stay
// legible against the dark specimen plate the curve editors are drawn on.
// Mirrors --accent-attack / --accent-sustain in css/styles.css.
const ACCENT_COLORS = {
    attack: '#9BB877',  // moss
    sustain: '#D4A257'  // ochre
};

const app = {
    knobs: {},
    curves: {},
    curveEditors: {
        attack: null,
        sustain: null
    },
    curveModes: {
        attack: 'freehand', // 'freehand' or 'node'
        sustain: 'freehand'
    },
    spectrogram: null,
    presetManager: null,
    animationFrameId: null,
    tooltipsEnabled: false,  // v1.5.0: armed by the header "?" toggle, persisted in session state
    initialized: false
};

// ============================================================================
// INITIALIZATION
// ============================================================================

function initializeApp() {
    console.log('O-SpectralShaper - Initializing...');

    // Verify JUCE backend
    if (typeof window.__JUCE__ === 'undefined') {
        console.error('JUCE backend not available');
        return;
    }

    console.log('JUCE backend:', window.__JUCE__);

    // Initialize knobs and parameter bindings
    initializeKnobs();

    // Initialize toggle
    initializeToggle();

    // Initialize curve editors (Phase 3.2)
    initializeCurveEditors();

    // Initialize spectrogram (Phase 3.3)
    initializeSpectrogram();

    // Initialize preset manager
    initializePresetManager();

    // Initialize tooltip system (v1.5.0)
    initializeTooltips();

    // Mark as initialized
    app.initialized = true;
    console.log('O-SpectralShaper - Initialization complete');
}

// ============================================================================
// KNOB INITIALIZATION
// ============================================================================

function initializeKnobs() {
    // Mix (0-100%)
    app.knobs.mix = new RotaryKnob('mix-knob-container', 'mix-value', {
        formatValue: (v) => `${Math.round(v * 100)}%`
    });
    bindKnobToParameter(app.knobs.mix, 'MIX');

    // Attack Time (0.1-50ms, skewed range)
    // WR-01: read the real engineering value JUCE already computed via getScaledValue()
    // (== NormalisableRange::convertFrom0to1). Re-deriving the skewed range in JS drifts
    // from the C++ range and ignores the skew — see pattern_webview_knob_readout_scaled_value.
    app.knobs.attackTime = new RotaryKnob('attack-time-knob-container', 'attack-time-value', {
        formatValue: () => {
            const value = Juce.getSliderState('ATTACK_TIME').getScaledValue();
            return value < 10 ? `${value.toFixed(1)}ms` : `${Math.round(value)}ms`;
        }
    });
    bindKnobToParameter(app.knobs.attackTime, 'ATTACK_TIME');

    // Sustain Time (10-500ms, skewed range) — WR-01: use getScaledValue(), see above
    app.knobs.sustainTime = new RotaryKnob('sustain-time-knob-container', 'sustain-time-value', {
        formatValue: () => {
            const value = Juce.getSliderState('SUSTAIN_TIME').getScaledValue();
            return `${Math.round(value)}ms`;
        }
    });
    bindKnobToParameter(app.knobs.sustainTime, 'SUSTAIN_TIME');

    // Sensitivity (0-100%)
    app.knobs.sensitivity = new RotaryKnob('sensitivity-knob-container', 'sensitivity-value', {
        formatValue: (v) => `${Math.round(v * 100)}%`
    });
    bindKnobToParameter(app.knobs.sensitivity, 'SENSITIVITY');

    // Lookahead Time (0.1-10ms)
    app.knobs.lookaheadTime = new RotaryKnob('lookahead-time-knob-container', 'lookahead-time-value', {
        formatValue: (v) => {
            const min = 0.1;
            const max = 10.0;
            const value = min + (v * (max - min));
            return `${value.toFixed(1)}ms`;
        }
    });
    bindKnobToParameter(app.knobs.lookaheadTime, 'LOOKAHEAD_TIME');

    // Output Gain (-12 to +12 dB)
    app.knobs.outputGain = new RotaryKnob('output-gain-knob-container', 'output-gain-value', {
        formatValue: (v) => {
            const min = -12.0;
            const max = 12.0;
            const value = min + (v * (max - min));
            return `${value >= 0 ? '+' : ''}${value.toFixed(1)}dB`;
        }
    });
    bindKnobToParameter(app.knobs.outputGain, 'OUTPUT_GAIN');
}

/**
 * Bind RotaryKnob to JUCE parameter via WebSliderRelay
 *
 * CRITICAL: JUCE 8 valueChangedEvent pattern
 * - Callback receives NO parameters
 * - Must call getNormalisedValue() inside callback
 */
function bindKnobToParameter(knob, parameterId) {
    try {
        // Get slider state from JUCE
        const sliderState = Juce.getSliderState(parameterId);

        // Initialize knob with current parameter value
        const initialValue = sliderState.getNormalisedValue();
        knob.setValue(initialValue);

        // Knob changes → JUCE parameter
        knob.onValueChange = (value) => {
            sliderState.setNormalisedValue(value);
        };

        // JUCE parameter changes → Knob (automation, preset load)
        // CRITICAL: No callback parameters in JUCE 8!
        sliderState.valueChangedEvent.addListener(() => {
            const newValue = sliderState.getNormalisedValue();
            knob.setValue(newValue);
        });

        console.log(`Bound knob to parameter: ${parameterId}`);
    } catch (error) {
        console.error(`Failed to bind knob to ${parameterId}:`, error);
    }
}

// ============================================================================
// TOGGLE INITIALIZATION
// ============================================================================

function initializeToggle() {
    const toggleElement = document.getElementById('lookahead-toggle');

    try {
        // Get toggle state from JUCE
        const toggleState = Juce.getToggleState('LOOKAHEAD_ENABLED');

        // Initialize toggle with current value
        const initialValue = toggleState.getValue();
        updateToggleVisual(toggleElement, initialValue);

        // Toggle click → JUCE parameter
        toggleElement.addEventListener('click', () => {
            const newValue = !toggleState.getValue();
            toggleState.setValue(newValue);
            updateToggleVisual(toggleElement, newValue);
        });

        // JUCE parameter changes → Toggle (automation, preset load)
        toggleState.valueChangedEvent.addListener(() => {
            const newValue = toggleState.getValue();
            updateToggleVisual(toggleElement, newValue);
        });

        console.log('Bound toggle to parameter: LOOKAHEAD_ENABLED');
    } catch (error) {
        console.error('Failed to bind toggle to LOOKAHEAD_ENABLED:', error);
    }
}

function updateToggleVisual(element, value) {
    if (value) {
        element.classList.add('active');
    } else {
        element.classList.remove('active');
    }
}

// ============================================================================
// CURVE EDITOR INITIALIZATION (Phase 3.2)
// ============================================================================

function initializeCurveEditors() {
    console.log('Initializing curve editors...');

    // Attack curve editor
    app.curveEditors.attack = new FreehandCurve('attack-curve-canvas', {
        accentColor: ACCENT_COLORS.attack,
        numBands: 32
    });

    app.curveEditors.attack.onCurveChange = (data) => {
        sendCurveToProcessor('attack', data);
    };

    // Sustain curve editor
    app.curveEditors.sustain = new FreehandCurve('sustain-curve-canvas', {
        accentColor: ACCENT_COLORS.sustain,
        numBands: 32
    });

    app.curveEditors.sustain.onCurveChange = (data) => {
        sendCurveToProcessor('sustain', data);
    };

    // Mode toggle buttons
    setupModeToggle('attack');
    setupModeToggle('sustain');

    // Reset buttons
    setupResetButton('attack');
    setupResetButton('sustain');

    // Undo/redo buttons
    setupUndoRedoButtons('attack');
    setupUndoRedoButtons('sustain');

    // Spectrum overlay toggles
    setupSpectrumToggle('attack');
    setupSpectrumToggle('sustain');
}

function setupModeToggle(curveType) {
    const toggleButton = document.getElementById(`${curveType}-mode-toggle`);
    const canvasId = `${curveType}-curve-canvas`;
    const accentColor = ACCENT_COLORS[curveType];

    toggleButton.addEventListener('click', () => {
        // Toggle mode
        const currentMode = app.curveModes[curveType];
        const newMode = currentMode === 'freehand' ? 'node' : 'freehand';
        app.curveModes[curveType] = newMode;

        // Update button text
        toggleButton.textContent = newMode === 'freehand' ? 'Freehand' : 'Node';

        // Get current curve data and spectrum state
        const currentData = app.curveEditors[curveType].getCurveData();
        const spectrumVisible = app.curveEditors[curveType].showSpectrum;

        // Replace editor
        if (newMode === 'freehand') {
            app.curveEditors[curveType] = new FreehandCurve(canvasId, {
                accentColor,
                numBands: 32
            });
        } else {
            app.curveEditors[curveType] = new NodeCurve(canvasId, {
                accentColor,
                numBands: 32
            });
        }

        // Restore curve data and spectrum state
        app.curveEditors[curveType].setCurveData(currentData);
        app.curveEditors[curveType].showSpectrum = spectrumVisible;

        // Attach callbacks
        app.curveEditors[curveType].onCurveChange = (data) => {
            sendCurveToProcessor(curveType, data);
        };

        // Rewire undo/redo state callback for buttons
        const undoBtn = document.getElementById(`${curveType}-undo-btn`);
        const redoBtn = document.getElementById(`${curveType}-redo-btn`);
        undoBtn.disabled = true;
        redoBtn.disabled = true;
        app.curveEditors[curveType].onUndoStateChange = (canUndo, canRedo) => {
            undoBtn.disabled = !canUndo;
            redoBtn.disabled = !canRedo;
        };

        console.log(`${curveType} curve mode: ${newMode}`);
    });
}

function setupUndoRedoButtons(curveType) {
    const undoBtn = document.getElementById(`${curveType}-undo-btn`);
    const redoBtn = document.getElementById(`${curveType}-redo-btn`);

    undoBtn.addEventListener('click', () => {
        app.curveEditors[curveType].undo();
    });

    redoBtn.addEventListener('click', () => {
        app.curveEditors[curveType].redo();
    });

    // Bind state change callback to enable/disable buttons
    app.curveEditors[curveType].onUndoStateChange = (canUndo, canRedo) => {
        undoBtn.disabled = !canUndo;
        redoBtn.disabled = !canRedo;
    };
}

function setupResetButton(curveType) {
    const resetButton = document.getElementById(`${curveType}-reset-btn`);

    resetButton.addEventListener('click', () => {
        app.curveEditors[curveType].resetCurve();
        console.log(`${curveType} curve reset to flat`);
    });
}

function setupSpectrumToggle(curveType) {
    const btn = document.getElementById(`${curveType}-spectrum-btn`);
    if (!btn) return;

    btn.addEventListener('click', () => {
        const editor = app.curveEditors[curveType];
        editor.showSpectrum = !editor.showSpectrum;
        btn.classList.toggle('active', editor.showSpectrum);
        // Re-render immediately to show/hide
        editor.render();
    });
}

/**
 * Native function references (obtained once via Juce.getNativeFunction)
 */
const nativeFunctions = {
    setAttackCurve: Juce.getNativeFunction('setAttackCurve'),
    setSustainCurve: Juce.getNativeFunction('setSustainCurve')
};

/**
 * Send curve data to C++ processor via JUCE native function bridge
 */
function sendCurveToProcessor(curveType, data) {
    try {
        const fn = curveType === 'attack'
            ? nativeFunctions.setAttackCurve
            : nativeFunctions.setSustainCurve;
        fn(...data);
    } catch (error) {
        console.error(`Failed to send ${curveType} curve to C++:`, error);
    }
}

/**
 * C++ → JavaScript: Set attack curve data
 */
window.setAttackCurveFromCPP = function(data) {
    if (app.curveEditors && app.curveEditors.attack && data.length === 32) {
        app.curveEditors.attack.setCurveData(data);
        console.log('Loaded attack curve from C++');
    }
};

/**
 * C++ → JavaScript: Set sustain curve data
 */
window.setSustainCurveFromCPP = function(data) {
    if (app.curveEditors && app.curveEditors.sustain && data.length === 32) {
        app.curveEditors.sustain.setCurveData(data);
        console.log('Loaded sustain curve from C++');
    }
};

// ============================================================================
// PRESET MANAGER INITIALIZATION
// ============================================================================

function initializePresetManager() {
    console.log('Initializing preset manager...');

    app.presetManager = new PresetManager({
        displayElement: document.getElementById('preset-name'),
        prevButton: document.getElementById('preset-prev'),
        nextButton: document.getElementById('preset-next'),
        saveButton: document.getElementById('preset-save'),
        loadButton: document.getElementById('preset-load'),
        getNativeFunction: Juce.getNativeFunction,
        onPresetChanged: (name) => {
            console.log('Preset changed:', name);
        }
    });

    app.presetManager.initialize();
}

// ============================================================================
// TOOLTIP SYSTEM (v1.5.0)
// ============================================================================

/**
 * Hover tooltips for every [data-tooltip] element, armed by the header "?" toggle.
 *
 * One reused .tooltip element positioned against #app rather than per-element
 * popups: the controls live in a fixed 700x500 grid, so a single absolutely
 * positioned surface avoids 25 extra nodes and lets the edge clamping live in
 * one place.
 */
function initializeTooltips() {
    const toggle = document.getElementById('tooltip-toggle');
    const tooltip = document.getElementById('tooltip');
    const container = document.getElementById('app');

    if (!toggle || !tooltip || !container) {
        console.warn('Tooltip system: required elements missing, skipping');
        return;
    }

    const EDGE_MARGIN = 8;   // keep the surface clear of the window edge
    const GAP = 8;           // vertical gap between control and tooltip

    function applyEnabledState(enabled) {
        app.tooltipsEnabled = !!enabled;
        toggle.classList.toggle('active', app.tooltipsEnabled);
        toggle.setAttribute('aria-pressed', String(app.tooltipsEnabled));
        container.classList.toggle('tooltips-enabled', app.tooltipsEnabled);

        if (!app.tooltipsEnabled) {
            hideTooltip();
        }
    }

    function hideTooltip() {
        tooltip.classList.remove('visible');
        tooltip.setAttribute('aria-hidden', 'true');
    }

    function showTooltipFor(target) {
        const text = target.getAttribute('data-tooltip');
        if (!text) return;

        tooltip.textContent = text;

        // Measure at a neutral origin BEFORE placing. An absolutely positioned
        // element's shrink-to-fit width is computed against (containing block
        // width - left), so measuring while it still sits near the right edge
        // reports a narrow, wrapped box and the clamp below then mispositions
        // it. Reset to 0,0 with width:auto, measure, then pin the width in px.
        // See pattern_fixed_tooltip_shrink_to_fit_edge.
        tooltip.style.width = 'auto';
        tooltip.style.left = '0px';
        tooltip.style.top = '0px';

        const width = tooltip.offsetWidth;
        const height = tooltip.offsetHeight;
        tooltip.style.width = width + 'px';

        const rect = target.getBoundingClientRect();
        const containerRect = container.getBoundingClientRect();

        // Horizontal: centre on the control, then clamp both edges.
        let left = rect.left - containerRect.left + rect.width / 2 - width / 2;
        const maxLeft = containerRect.width - width - EDGE_MARGIN;
        if (left > maxLeft) left = maxLeft;
        if (left < EDGE_MARGIN) left = EDGE_MARGIN;

        // Vertical: prefer above the control, flip below if it would clip the top.
        let top = rect.top - containerRect.top - height - GAP;
        if (top < EDGE_MARGIN) {
            top = rect.bottom - containerRect.top + GAP;
        }
        // If flipping below would clip the bottom, clamp back inside.
        const maxTop = containerRect.height - height - EDGE_MARGIN;
        if (top > maxTop) top = maxTop;

        tooltip.style.left = left + 'px';
        tooltip.style.top = top + 'px';
        tooltip.classList.add('visible');
        tooltip.setAttribute('aria-hidden', 'false');
    }

    toggle.addEventListener('click', () => {
        applyEnabledState(!app.tooltipsEnabled);

        // Persist to C++. getNativeFunction lives on the `Juce` ES-module
        // namespace, NOT window.__JUCE__.backend (that object only carries
        // addEventListener/removeEventListener/emitEvent).
        try {
            Juce.getNativeFunction('setTooltipsEnabled')(app.tooltipsEnabled);
        } catch (error) {
            console.warn('Could not persist tooltip preference:', error);
        }
    });

    // Delegated hover — covers controls created after init too.
    container.addEventListener('mouseover', (e) => {
        if (!app.tooltipsEnabled) return;
        if (e.target.closest('#tooltip-toggle')) return;  // don't cover the toggle itself

        const target = e.target.closest('[data-tooltip]');
        if (!target) return;

        showTooltipFor(target);
    });

    container.addEventListener('mouseout', (e) => {
        const target = e.target.closest('[data-tooltip]');
        if (!target) return;

        // Ignore moves that stay inside the same tooltipped control.
        if (e.relatedTarget && target.contains(e.relatedTarget)) return;

        hideTooltip();
    });

    // Pull the persisted preference now that the bridge is live. Doing this here
    // rather than having C++ push on open avoids racing the WebView load.
    try {
        Juce.getNativeFunction('getTooltipsEnabled')().then((enabled) => {
            applyEnabledState(!!enabled);
        });
    } catch (error) {
        console.warn('Could not read tooltip preference:', error);
    }
}

// ============================================================================
// SPECTROGRAM INITIALIZATION (Phase 3.3)
// ============================================================================

function initializeSpectrogram() {
    console.log('Initializing spectrogram...');

    // Create WebGL spectrogram renderer
    app.spectrogram = new Spectrogram('spectrogram-canvas', {
        width: 512,
        height: 257,
        heatIntensity: 0.5
    });

    // Listen for visualization events from C++
    if (window.__JUCE__ && window.__JUCE__.backend) {
        window.__JUCE__.backend.addEventListener('visualizationUpdate', (event) => {
            try {
                const data = JSON.parse(event);

                if (data.fft && data.transients) {
                    // Add frame to spectrogram
                    app.spectrogram.addFrame(data.fft, data.transients);

                    // Route transient data to curve editors for glow animation
                    if (app.curveEditors.attack) {
                        app.curveEditors.attack.setTransientActivity(data.transients);
                        app.curveEditors.attack.setSpectrumData(data.fft);
                    }
                    if (app.curveEditors.sustain) {
                        app.curveEditors.sustain.setTransientActivity(data.transients);
                        app.curveEditors.sustain.setSpectrumData(data.fft);
                    }
                }
            } catch (error) {
                console.error('Failed to parse visualization data:', error);
            }
        });

        console.log('Listening for visualizationUpdate events');
    }

    // Start render loop
    startRenderLoop();
}

function startRenderLoop() {
    function render() {
        if (app.spectrogram) {
            app.spectrogram.draw();
        }

        // Re-render curve editors when transients are active or spectrum overlay is shown
        if (app.curveEditors.attack && (app.curveEditors.attack.hasActiveTransients || app.curveEditors.attack.showSpectrum)) {
            app.curveEditors.attack.render();
        }
        if (app.curveEditors.sustain && (app.curveEditors.sustain.hasActiveTransients || app.curveEditors.sustain.showSpectrum)) {
            app.curveEditors.sustain.render();
        }

        app.animationFrameId = requestAnimationFrame(render);
    }

    // Start loop
    render();
    console.log('Render loop started');
}

function stopRenderLoop() {
    if (app.animationFrameId) {
        cancelAnimationFrame(app.animationFrameId);
        app.animationFrameId = null;
    }
}

// ============================================================================
// ENTRY POINT
// ============================================================================

// Wait for DOM to load
if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', initializeApp);
} else {
    initializeApp();
}

// Cleanup on unload
window.addEventListener('beforeunload', () => {
    stopRenderLoop();
    if (app.spectrogram) {
        app.spectrogram.destroy();
    }
});
