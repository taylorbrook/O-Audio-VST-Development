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

    // Attack Time (0.1-50ms, log scale)
    app.knobs.attackTime = new RotaryKnob('attack-time-knob-container', 'attack-time-value', {
        formatValue: (v) => {
            // Convert normalized to log range (0.1 to 50)
            const skew = 0.3;
            const min = 0.1;
            const max = 50.0;
            const value = min * Math.pow(max / min, Math.pow(v, 1.0 / skew));
            return value < 10 ? `${value.toFixed(1)}ms` : `${Math.round(value)}ms`;
        }
    });
    bindKnobToParameter(app.knobs.attackTime, 'ATTACK_TIME');

    // Sustain Time (10-500ms, log scale)
    app.knobs.sustainTime = new RotaryKnob('sustain-time-knob-container', 'sustain-time-value', {
        formatValue: (v) => {
            // Convert normalized to log range (10 to 500)
            const skew = 0.3;
            const min = 10.0;
            const max = 500.0;
            const value = min * Math.pow(max / min, Math.pow(v, 1.0 / skew));
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
        accentColor: '#4A90D9', // Attack blue
        numBands: 32
    });

    app.curveEditors.attack.onCurveChange = (data) => {
        sendCurveToProcessor('attack', data);
    };

    // Sustain curve editor
    app.curveEditors.sustain = new FreehandCurve('sustain-curve-canvas', {
        accentColor: '#D9944A', // Sustain orange
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

    // Load initial curve data from C++
    loadCurvesFromProcessor();
}

function setupModeToggle(curveType) {
    const toggleButton = document.getElementById(`${curveType}-mode-toggle`);
    const canvasId = `${curveType}-curve-canvas`;
    const accentColor = curveType === 'attack' ? '#4A90D9' : '#D9944A';

    toggleButton.addEventListener('click', () => {
        // Toggle mode
        const currentMode = app.curveModes[curveType];
        const newMode = currentMode === 'freehand' ? 'node' : 'freehand';
        app.curveModes[curveType] = newMode;

        // Update button text
        toggleButton.textContent = newMode === 'freehand' ? 'Freehand' : 'Node';

        // Get current curve data
        const currentData = app.curveEditors[curveType].getCurveData();

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

        // Restore curve data
        app.curveEditors[curveType].setCurveData(currentData);

        // Attach callback
        app.curveEditors[curveType].onCurveChange = (data) => {
            sendCurveToProcessor(curveType, data);
        };

        console.log(`${curveType} curve mode: ${newMode}`);
    });
}

function setupResetButton(curveType) {
    const resetButton = document.getElementById(`${curveType}-reset-btn`);

    resetButton.addEventListener('click', () => {
        app.curveEditors[curveType].resetCurve();
        console.log(`${curveType} curve reset to flat`);
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

/**
 * Load initial curve data from C++ processor
 * (C++ will call setAttackCurveFromCPP/setSustainCurveFromCPP when ready)
 */
function loadCurvesFromProcessor() {
    console.log('Waiting for initial curve data from C++...');
}

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
                    }
                    if (app.curveEditors.sustain) {
                        app.curveEditors.sustain.setTransientActivity(data.transients);
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

        // Re-render curve editors when transients are active (glow animation)
        if (app.curveEditors.attack && app.curveEditors.attack.hasActiveTransients) {
            app.curveEditors.attack.render();
        }
        if (app.curveEditors.sustain && app.curveEditors.sustain.hasActiveTransients) {
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
