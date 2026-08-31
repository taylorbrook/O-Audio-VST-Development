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
import { LANGUAGES, I18N, LABELS, TIP_BINDINGS, tr } from './i18n.js';

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

    // Initialize the interface language (v1.7.0). BEFORE the tooltip system,
    // so the help toggle's first painted face comes from the table rather than
    // from the markup fallback, and AFTER every control above exists — a
    // caption keyed by applyI18n before its element exists writes onto nothing
    // and reports only as a console warning.
    try { initI18n(); }            catch (e) { console.error('i18n init failed:', e); }
    try { initSettingsPopover(); } catch (e) { console.error('settings popover init failed:', e); }

    // Initialize tooltip system (v1.5.0, ported to measure-then-pin in v1.7.0)
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

        // Update button text. KEYS through setLabel, from an if/else — never a
        // ternary inside the call (check-i18n assertion 13), and never a
        // literal, which would strand an English "Node" the instant the
        // language selector fired.
        if (newMode === 'freehand') setLabel(toggleButton, 'ui.freehand');
        else                        setLabel(toggleButton, 'ui.node');

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

    const nameEl = document.getElementById('preset-name');
    const menuEl = document.getElementById('preset-menu');
    const prevEl = document.getElementById('preset-prev');
    const nextEl = document.getElementById('preset-next');

    // ── preset menu (v1.6.0) ────────────────────────────────────────────────
    // The module has no menu of its own, so the list lives here. It is a VIEW
    // over the module's state, not a second source of truth: selection always
    // goes through presetManager.loadPreset(), the same call the arrows make,
    // so the arrows and the menu cannot disagree about what is loaded.
    let sections = [];          // [{ category, presets: [...] }] from C++
    let menuOpen = false;

    // The ◀ ▶ arrows walk THIS order — the grouped menu order, flattened —
    // not the module's native selectNext/selectPrevious, which walk the C++
    // flat alphabetical list and would desync from the visible grouping
    // (pattern_grouping_preset_dropdown_breaks_prev_next). That is also why
    // prevButton/nextButton are NOT passed to the PresetManager constructor:
    // the module would bind them to its own native navigation.
    let presetWalkOrder = [];

    // Resolved once. A gap between this name and the C++ withNativeFunction
    // registration fails SILENTLY (pattern_webview_native_fn_bridge_gap), so
    // a throw here is logged rather than swallowed — the band still works,
    // it just loses the menu.
    let getGrouped = null;
    try {
        getGrouped = Juce.getNativeFunction('getPresetListGrouped');
    } catch (e) {
        console.error('[preset-menu] getPresetListGrouped unavailable:', e);
    }

    const markActive = () => {
        const current = app.presetManager ? app.presetManager.currentPreset : '';
        menuEl.querySelectorAll('.preset-menu-item').forEach((el) => {
            const isCurrent = el.dataset.name === current;
            el.classList.toggle('active', isCurrent);
            el.setAttribute('aria-selected', isCurrent ? 'true' : 'false');
        });
    };

    const closeMenu = () => {
        if (!menuOpen) return;
        menuOpen = false;
        menuEl.classList.remove('visible');
        nameEl.setAttribute('aria-expanded', 'false');
    };

    const openMenu = () => {
        if (menuOpen) return;
        menuOpen = true;
        markActive();
        menuEl.classList.add('visible');
        nameEl.setAttribute('aria-expanded', 'true');
        // Scroll the loaded preset into view. With 29 factory presets the
        // current one is usually below the fold, and a menu that always opens
        // at the top hides the one row the user opened it to see. offsetTop is
        // measured against .preset-menu itself (position:absolute, so it is
        // the offsetParent) and only ever moves the menu's own scrollTop —
        // scrollIntoView() could scroll the page instead.
        const active = menuEl.querySelector('.preset-menu-item.active');
        if (active) {
            menuEl.scrollTop = Math.max(
                0, active.offsetTop - (menuEl.clientHeight / 2));
        }
    };

    const buildMenu = () => {
        menuEl.replaceChildren();
        presetWalkOrder = [];
        for (const section of sections) {
            if (!section || !Array.isArray(section.presets)) continue;

            const header = document.createElement('div');
            header.className = 'preset-menu-category';
            header.textContent = section.category;
            menuEl.appendChild(header);

            for (const name of section.presets) {
                presetWalkOrder.push(name);
                const item = document.createElement('div');
                item.className = 'preset-menu-item';
                item.setAttribute('role', 'option');
                item.dataset.name = name;
                item.textContent = name;
                item.addEventListener('click', (e) => {
                    e.stopPropagation();
                    closeMenu();
                    // No local state written here: loadPreset() drives
                    // onPresetChanged, which is what repaints the readout and
                    // the highlight. Writing them here too would let the UI
                    // claim a preset loaded when the C++ side returned false.
                    app.presetManager.loadPreset(name);
                });
                menuEl.appendChild(item);
            }
        }
        markActive();
    };

    const refreshMenu = async () => {
        if (!getGrouped) return;
        try {
            const result = await getGrouped();
            sections = Array.isArray(result) ? result : [];
            buildMenu();
        } catch (e) {
            console.error('[preset-menu] getPresetListGrouped failed:', e);
        }
    };

    // ◀ ▶ step through the flattened MENU order, wrapping at the ends. A
    // preset loaded from a file (not in the list) enters at the top going
    // forward, the bottom going back.
    const stepPreset = async (delta) => {
        if (presetWalkOrder.length === 0) return;
        const index = presetWalkOrder.indexOf(app.presetManager.currentPreset);
        const base = index >= 0 ? index : (delta > 0 ? -1 : 0);
        const next = (base + delta + presetWalkOrder.length) % presetWalkOrder.length;
        await app.presetManager.loadPreset(presetWalkOrder[next]);
    };

    nameEl.addEventListener('click', (e) => {
        e.stopPropagation();      // else the document handler closes it again
        if (menuOpen) closeMenu(); else openMenu();
    });
    document.addEventListener('click', closeMenu);
    document.addEventListener('keydown', (e) => {
        if (e.key === 'Escape') closeMenu();
    });
    prevEl.addEventListener('click', () => stepPreset(-1));
    nextEl.addEventListener('click', () => stepPreset(1));

    app.presetManager = new PresetManager({
        displayElement: nameEl,   // stays childless — _updateDisplay writes textContent
        saveButton: document.getElementById('preset-save'),
        loadButton: document.getElementById('preset-load'),
        getNativeFunction: Juce.getNativeFunction,
        // Fires on every load, from the arrows OR the menu — one highlight
        // path for both.
        onPresetChanged: (name) => {
            console.log('Preset changed:', name);
            markActive();
        },
        // Fires on every refresh(), which is what save / load-from-file all
        // end in. Rebuilding from C++ rather than patching the DOM keeps the
        // User section honest about what is actually on disk.
        onPresetListUpdated: () => { refreshMenu(); }
    });

    app.presetManager.initialize();
}

// ============================================================================
// TOOLTIPS — the measure-then-pin renderer (v1.7.0)
// ============================================================================
//
// PORTED from O-ReverseDelay via O-FreqPulse / O-Lyrica, replacing this
// plugin's own second positioner ENTIRELY. There is now ONE tooltip renderer
// repo-wide.
//
// THE REFERENCE FRAME CHANGED, AND THAT IS THE POINT ON THIS PLUGIN. v1.6.2
// already measured its surface — it is the only one of the seven Stage-J
// plugins that did — but it positioned against #app:
//
//     const containerRect = container.getBoundingClientRect();
//     let left = rect.left - containerRect.left + rect.width / 2 - width / 2;
//     const maxLeft = containerRect.width - width - EDGE_MARGIN;
//
// with the surface `position: absolute` inside #app. #app is NOT the viewport
// here: it carries `padding: 12px` and sits inside a 700x500 body, so its
// content box is inset and its clamp rails were 24px narrower than the window.
// Every tip was therefore held 12px further from each edge than it needed to
// be, and the vertical rail was computed against containerRect.height rather
// than window.innerHeight. That is a SMALL error, which is exactly what makes
// it worth replacing rather than adapting: a 12px bias reads as a styling
// choice, not as a wrong reference frame, and it would have gone on being
// re-derived by hand on the next edit.
//
// The port replaces that arithmetic outright with the viewport-relative form
// and moves .tooltip to `position: fixed` in the same commit. Adapting the old
// code in place would have left `absolute` positioning being fed viewport
// coordinates, which is off by #app's origin — 12px on both axes here.
//
// What else the port brings that v1.6.2 did not have: a title/body pair built
// from data-tip-title + data-tip rather than one flat string, a dwell delay so
// a tip does not fire on every crossing, a pointerdown suppression so a tip
// cannot hang over a knob mid-drag, an arrow whose offset is recomputed AFTER
// the horizontal clamp so a clamped tip still points at its control, and
// delegated listeners on the DOCUMENT rather than on #app.
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

// v1.5.0: master on/off for the hover-help layer, persisted C++-side.
// v1.7.0 moved its control out of the wax-seal "?" and into the settings
// popover, next to the language selector.
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

    // Any press begins a click or a drag: get the tip out of the way and keep
    // it away until release, so it cannot hang over a knob or a curve canvas
    // mid-drag. Capture phase, because RotaryKnob and the curve editors call
    // preventDefault in their own mousedown handlers.
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
    let getTooltipsEnabledNative = null;

    try {
        getTooltipsEnabledNative = Juce.getNativeFunction('getTooltipsEnabled');
        setTooltipsEnabledNative = Juce.getNativeFunction('setTooltipsEnabled');
    } catch (e) {
        console.warn('Tooltip preference not available, session-only:', e);
    }

    // Paint the current (default) state first so the button is never blank
    // while the native call is in flight.
    setTooltipsEnabled(tooltipsEnabled, false);

    if (getTooltipsEnabledNative) {
        getTooltipsEnabledNative()
            .then((stored) => setTooltipsEnabled(!!stored, false))
            .catch((e) => console.warn('Could not read tooltip preference:', e));
    }
}

// `persist` is false for the start-up push, so reading the stored value does
// not immediately write it back.
function setTooltipsEnabled(enabled, persist) {
    tooltipsEnabled = !!enabled;

    if (!tooltipsEnabled) hideTooltip();

    const appEl = document.getElementById('app');
    if (appEl) appEl.classList.toggle('tooltips-enabled', tooltipsEnabled);

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

// The gear and the toggle inside the popover both carry data-tip-always: the
// two controls that reach and restore the help layer have to keep explaining
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

    // Moving between children of the same control is not a real exit. Every
    // .knob-wrapper here wraps a knob, a caption and a value readout, and
    // crossing between those children previously flickered the surface off and
    // back on.
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
    // afterwards re-wraps a 240 px tip into a narrow ribbon — and the squeezed
    // width then resolves `left` straight back against the right edge, so it
    // never recovers on later hovers. Release the width, measure from the left
    // edge, pin the result in px, and only then place.
    //
    // The pinned width is the FRACTIONAL getBoundingClientRect().width, not the
    // integer offsetWidth v1.6.2 used: 188.48 rounds to 188, and pinning that
    // makes the box 0.48 px narrower than its own shrink-to-fit, pushing the
    // last word onto a second line. Height is only stable once the width is
    // definite, so it is read after
    // (pattern_fixed_tooltip_shrink_to_fit_edge).
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

    // THE VERTICAL CLAMP. O-ReverseDelay's anchors are all knob-sized, so
    // `below` always fits there and the omission is invisible; O-FreqPulse
    // reproduced a real 15px overhang on a 376px anchor and added this line.
    //
    // ON THIS PAGE IT IS NOT INDEPENDENTLY REPRODUCIBLE, and that is said rather
    // than dressed up. Sweeping all 28 anchors in French, the SMALLEST slack
    // between a tip's bottom edge and the 500px frame is 111px, and deleting
    // this line alone leaves every one of the 56 hovers fully inside the
    // window. The two shapes that could have produced an overhang both miss:
    // the tallest anchor is .spectrogram-container at 202px, but it sits at
    // y=70 and its tip flips `below` to 386.7; the DEEPEST anchors are the
    // sustain plate's five buttons at y=397, and those all fit `above`.
    //
    // The sweep is not blind — removing the HORIZONTAL clamp instead reports 14
    // off-frame tips, out to 120px, in the same run. So the negative result
    // above is a measurement, not a probe that passes either way
    // (pattern_probe_must_target_the_branch_the_fix_changed).
    //
    // It is ported anyway because the point of this stage is ONE runtime
    // repo-wide, and a copy that silently differs from the others is the drift
    // the canon exists to stop.
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
// Interface language (v1.7.0)
// ============================================================================
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
// `grep -rn setVisible plugins/O-SpectralShaper/Source/` returns NOTHING — the
// web view is never hidden, so the hidden-completion drop cannot fire
// (critical_webview_completion_gated_on_isvisible).
//
// Declared here at module level, ABOVE every reader. The only statements
// executed at module-evaluation time are the two window.__ assignments, which
// touch hoisted function declarations and cannot enter a TDZ chain
// (pattern_module_toplevel_init_tdz). initI18n() itself is called from INSIDE
// initializeApp(), after the controls it labels exist.

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
    // <html lang> follows the selector: screen readers pick the French voice,
    // and CSS hyphens:auto / quotes resolve in the page's actual language.
    document.documentElement.lang = uiLanguage;
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
window.__reapplyI18n = () => applyI18n(uiLanguage);

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

// ============================================================================
// The settings popover (v1.7.0)
// ============================================================================
//
// The gear that carries the language selector and the hover-help switch. Two
// rows: this plugin HAS the setTooltipsEnabled bridge, so its toggle moves in
// here from the wax-seal "?" rather than sitting beside a second control for
// the same state.
//
// IT SITS EXACTLY WHERE THE "?" SAT — the same 18x18 disc in .header-right,
// same 9px gap before .version — so the new control adds ZERO geometry delta to
// a 700x500 frame that has none to spare.
//
// The panel opens DOWNWARD, because the gear is in the HEADER: there is 450px
// of frame below it and 12px above.
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
    // click, so the panel is gone before a drag on a knob or a curve canvas
    // underneath it begins — both call preventDefault in their own mousedown
    // handlers. Matches how the preset menu already behaves.
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
