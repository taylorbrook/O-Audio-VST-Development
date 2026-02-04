/**
 * O-SpectralShaper Main Application
 *
 * Phase 3.1: Parameter binding with JUCE relays
 * Phase 3.2: Curve editors (placeholder)
 * Phase 3.3: Spectrogram visualization (placeholder)
 */

import * as Juce from './juce/index.js';
import { RotaryKnob } from './components/RotaryKnob.js';

// ============================================================================
// APPLICATION STATE
// ============================================================================

const app = {
    knobs: {},
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
// ENTRY POINT
// ============================================================================

// Wait for DOM to load
if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', initializeApp);
} else {
    initializeApp();
}
