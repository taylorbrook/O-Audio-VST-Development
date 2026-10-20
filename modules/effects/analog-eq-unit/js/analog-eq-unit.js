/*
   This file is part of the Ouaricon Audio analog-eq-unit module.
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
 * Analog EQ Unit - Compact WebView UI Component with Dual-Ring Knobs
 * Ouaricon Module System v1.4.0
 *
 * v1.4.0 Changes:
 * - Double-click opens inline value entry for the ring under the pointer, in
 *   that ring's half of the value tooltip (outer ring = frequency, `1.2k` =
 *   1200 Hz, a bare number is Hz; inner dial = gain in dB). Enter or clicking
 *   away applies, Esc cancels, untouched text is not written
 * - Alt/Option-click resets the ring under the pointer to its default inside
 *   one host gesture (was double-click, which wrote without a gesture)
 * - Value tooltip split into .vt-freq / .vt-gain spans
 *
 * v1.3.0 Changes (shared strip contract with compressor-unit 1.3.0 / reverb-unit 1.0.0):
 * - Host face: var(--font-serif) with an EB Garamond fallback; buttons inherit it
 * - Fixed 505px strip, 21px side padding, 76px bypass button, fixed line-heights
 *   (no height change between Latin and CJK document languages)
 * - Bands on a 64px column pitch starting 10px after the bypass button, so band
 *   centres line up with the compressor/reverb knob columns; ANALOG pinned right
 * - 9px text floor (band labels, W/M/T, value tooltip were 7-8px)
 * - AA fills: on #4E6839 / #FFF (6.2:1), off #6B5040 / #E8D5B7 (5.1:1)
 * - Off states are colour, not opacity: only knob graphics fade (0.4); a disabled
 *   band's label/Q and a bypassed strip's text controls take a muted fill
 *   (#3A2E24 / #B8A08C, 5.3:1). At opacity they read 1.4-2.1:1, and a disabled
 *   band's own on/off label (the control that re-enables it) sat at 0.14
 *
 * v1.2.0 Changes:
 * - Module dimensions: 10px shorter vertically, 100px wider
 * - Padding adjusted from 8px 10px to 3px 60px
 *
 * v1.1.0 Changes:
 * - Compact single-row layout (half height)
 * - EQ button as title/bypass toggle (left side)
 * - ANALOG toggle inline with bands (right side)
 * - Removed output knob and footer
 *
 * Features the signature dual-ring knob design:
 * - Outer ring: Frequency control with notches
 * - Inner dial: Gain control with seed pattern
 *
 * Usage:
 *   import { AnalogEQUnitUI } from './modules/analog-eq-unit.js';
 *
 *   const eq = new AnalogEQUnitUI({
 *     container: document.getElementById('eq-panel'),
 *     paramPrefix: 'eq_',
 *     getSliderState: getSliderState,
 *     getToggleState: getToggleState,
 *     getComboBoxState: getComboBoxState
 *   });
 *   eq.initialize();
 */

// ====================================================================
// Value entry (module-private; the suite's O-Prism attachValueEntry semantics).
// Self-contained on purpose: the unit imports nothing from its host page and
// uses its own input class, so the host's .knob-value-input rule never crosses.
// ====================================================================

// Scaled (C++ units) -> normalised [0,1], reading the live start/end/skew the
// WebSliderRelay pushes from C++ (never a JS range table). Clamps BEFORE the
// pow(): a value below `start` gives a negative base, and Math.pow(negative,
// fractional skew) is NaN, which would stick in the parameter.
function scaledToNorm(v, props) {
    const start = props.start, end = props.end;
    const skew = props.skew > 0 ? props.skew : 1;
    if (!(end > start)) return 0;
    const t = Math.min(1, Math.max(0, (v - start) / (end - start)));
    return skew === 1 ? t : Math.pow(t, skew);
}

// Opens an inline editor over a readout. formatFn(norm) renders the readout;
// formatFn.parse(text, currentText) returns a SCALED value or null to reject.
// Enter / blur / clicking away commits, Esc cancels, untouched text is never
// written back, and "-inf" / "off" land on the range start.
function attachValueEntry(valueEl, state, formatFn) {
    if (!valueEl || !state) return;
    if (typeof formatFn !== 'function' || typeof formatFn.parse !== 'function') return;
    if (valueEl.dataset.editing === '1') return;   // already open

    const currentText = valueEl.textContent;
    const prevPosition = valueEl.style.position;
    const prevColor = valueEl.style.color;
    // Read the ink before the readout goes transparent.
    const inkColor = getComputedStyle(valueEl).color;

    const input = document.createElement('input');
    input.type = 'text';
    input.className = 'eq-value-input';
    input.value = currentText;
    input.spellcheck = false;
    input.autocomplete = 'off';
    input.style.color = inkColor;

    // The readout keeps its text (transparent) so its box does not change size.
    valueEl.dataset.editing = '1';
    valueEl.style.position = 'relative';
    valueEl.style.color = 'transparent';
    valueEl.appendChild(input);
    input.focus();
    input.select();

    let closed = false;

    // Clicking away commits even where a dial's drag handler preventDefault()s
    // its mousedown: that suppresses the focus change that would blur this
    // field, so grabbing another dial used to leave it open with its readout
    // frozen. Capture phase on document, so it commits before any drag starts.
    const onOutsidePointer = (e) => { if (e.target !== input) input.blur(); };
    document.addEventListener('pointerdown', onOutsidePointer, true);

    const close = () => {
        if (closed) return;
        closed = true;
        document.removeEventListener('pointerdown', onOutsidePointer, true);
        delete valueEl.dataset.editing;
        input.remove();
        valueEl.style.position = prevPosition;
        valueEl.style.color = prevColor;
        // Re-render from live state (automation / preset may have moved it).
        valueEl.textContent = formatFn(state.getNormalisedValue());
    };

    const commit = () => {
        if (closed) return;
        if (input.value === currentText) { close(); return; }
        const props = state.properties || {};
        const scaled = /^\s*([-−]?\s*inf(inity)?|off)\s*(db)?\s*$/i.test(input.value)
            ? props.start
            : formatFn.parse(input.value, currentText);
        if (scaled !== null && scaled !== undefined && isFinite(scaled)) {
            // setNormalisedValue() snaps to the interval and clamps.
            state.sliderDragStarted();
            state.setNormalisedValue(scaledToNorm(scaled, props));
            state.sliderDragEnded();
        }
        close();
    };

    input.addEventListener('keydown', (e) => {
        e.stopPropagation();
        if (e.key === 'Enter') { e.preventDefault(); commit(); }
        else if (e.key === 'Escape') { e.preventDefault(); close(); }
    });
    input.addEventListener('blur', commit);
    // Never start a drag (or reach a document handler) from inside the field.
    input.addEventListener('pointerdown', (e) => e.stopPropagation());
    input.addEventListener('mousedown', (e) => e.stopPropagation());
    input.addEventListener('dblclick', (e) => e.stopPropagation());
    input.addEventListener('wheel', (e) => e.stopPropagation());
}

// First signed decimal in the text; U+2212 (typographic minus) counts as '-'.
const numOf = (t) => {
    const m = String(t).replace(/−/g, '-').match(/-?\d*\.?\d+/);
    if (!m) return null;
    const v = parseFloat(m[0]);
    return isFinite(v) ? v : null;
};
// Gain readouts: the display already IS the scaled value ("3", "-4.5 dB").
const parseDirect = (t) => numOf(t);
// Frequency readouts that switch to kHz: "k" multiplies by 1000 ("1.2k" =
// 1200 Hz), a bare number is Hz.
const parseFreq = (t) => {
    const v = numOf(t);
    if (v === null) return null;
    return /k/i.test(String(t)) ? v * 1000 : v;
};

export class AnalogEQUnitUI {
    constructor(options = {}) {
        this.container = options.container;
        this.paramPrefix = options.paramPrefix || 'eq_';
        this.getSliderState = options.getSliderState;
        this.getToggleState = options.getToggleState;
        this.getComboBoxState = options.getComboBoxState;
        this.showMeter = options.showMeter ?? false;
        this.onReady = options.onReady || (() => {});

        // Internal state
        this.sliders = {};
        this.toggles = {};
        this.combos = {};

        // Drag state
        this.isDragging = false;
        this.currentState = null;
        this.currentParamName = null;
        this.lastY = 0;

        // Bound event handlers for cleanup
        this.boundOnDrag = (e) => this.onDrag(e);
        this.boundEndDrag = () => this.endDrag();

        // Knob geometry
        this.KNOB_RADIUS = 30;
        this.INNER_THRESHOLD = 0.60;

        // WR-09: Alt/Option-click reset target for each control, as a NORMALISED (0..1) position.
        // Gains are linear ([-12,12] dB, default 0 dB -> 0.5). The freq params are skewed
        // 0.3 in C++, so their default Hz maps to a NON-0.5 normalised position; resetting to
        // a hardcoded 0.5 landed them well off the shipped default. Each value below is
        // convertTo0to1(defaultHz) = ((defaultHz-start)/(end-start))^0.3 for the C++ range:
        //   lf  100 Hz in [30,500]     -> 0.5647
        //   lmf 500 Hz in [100,2000]   -> 0.6267
        //   hmf 2000 Hz in [500,8000]  -> 0.6171
        //   hf  8000 Hz in [2000,20000]-> 0.7192
        // (If a getParameterDefaults native fn is ever added across host plugins, prefer that.)
        this.DEFAULT_VALUES = {
            lf_freq: 0.5647, lf_gain: 0.5,
            lmf_freq: 0.6267, lmf_gain: 0.5,
            hmf_freq: 0.6171, hmf_gain: 0.5,
            hf_freq: 0.7192, hf_gain: 0.5
        };

        // WR-09: format the ACTUAL (scaled) engineering value, not a linear map of the
        // normalised value. The C++ freq params are skewed 0.3, so the old linear formula
        // (e.g. lf_freq: 30 + v*470 at norm 0.5 -> 265 Hz) displayed ~3.5x the real filter
        // frequency (~77 Hz). Callers pass state.getScaledValue(), which is the true Hz/dB.
        this.paramRanges = {
            lf_freq: { format: hz => Math.round(hz) + ' Hz' },
            lf_gain: { format: db => db.toFixed(1) + ' dB' },
            lmf_freq: { format: hz => Math.round(hz) + ' Hz' },
            lmf_gain: { format: db => db.toFixed(1) + ' dB' },
            hmf_freq: { format: hz => Math.round(hz) + ' Hz' },
            hmf_gain: { format: db => db.toFixed(1) + ' dB' },
            hf_freq: { format: hz => (hz >= 10000 ? (hz/1000).toFixed(1) + 'k' : Math.round(hz) + ' Hz') },
            hf_gain: { format: db => db.toFixed(1) + ' dB' }
        };
    }

    initialize() {
        if (!this.container) {
            console.error('AnalogEQUnitUI: No container element provided');
            return;
        }

        this.render();
        this.injectStyles();
        this.bindParameters();
        this.attachEventListeners();
        this.onReady();
    }

    render() {
        // v1.1.0: Compact single-row layout
        // [EQ toggle] | [LF] [LMF] [HMF] [HF] | [ANALOG toggle]
        this.container.innerHTML = `
            <div class="eq-unit-compact">
                <div class="eq-row">
                    <button class="eq-bypass-toggle" data-param="enabled">EQ</button>
                    <div class="eq-bands-compact">
                        ${this.renderBand('lf', 'LF', false)}
                        ${this.renderBand('lmf', 'LMF', true)}
                        ${this.renderBand('hmf', 'HMF', true)}
                        ${this.renderBand('hf', 'HF', false)}
                    </div>
                    <button class="eq-toggle analog-toggle" data-param="analog">ANALOG</button>
                </div>
            </div>
        `;
    }

    renderBand(id, label, hasQ) {
        return `
            <div class="eq-band-compact" data-band="${id}">
                <button class="band-label-compact" data-param="${id}_on">${label}</button>
                <div class="dual-knob-wrapper-compact">
                    ${this.renderFreqNotches()}
                    <div class="dual-knob-container" data-param-outer="${id}_freq" data-param-inner="${id}_gain">
                        <div class="knob-outer" id="${id}_freq_knob">
                            <div class="knob-outer-indicator"></div>
                        </div>
                        <div class="knob-inner" id="${id}_gain_knob">
                            <div class="knob-pointer"></div>
                        </div>
                        <div class="value-tooltip" id="${id}_tooltip"><span class="vt-freq">--</span> / <span class="vt-gain">--</span></div>
                    </div>
                </div>
                ${hasQ ? `
                <div class="three-way-toggle-compact" data-param="${id}_q">
                    <div class="three-way-option" data-value="0">W</div>
                    <div class="three-way-option active" data-value="1">M</div>
                    <div class="three-way-option" data-value="2">T</div>
                </div>
                ` : '<div class="q-spacer-compact"></div>'}
            </div>
        `;
    }

    renderFreqNotches() {
        return `
            <svg class="freq-notches-compact" viewBox="0 0 60 60">
                <g stroke="#5C4033" stroke-width="1" fill="none">
                    <line x1="30" y1="4" x2="30" y2="7" transform="rotate(-135, 30, 30)"/>
                    <line x1="30" y1="4" x2="30" y2="6" transform="rotate(-108, 30, 30)"/>
                    <line x1="30" y1="4" x2="30" y2="6" transform="rotate(-81, 30, 30)"/>
                    <line x1="30" y1="4" x2="30" y2="6" transform="rotate(-54, 30, 30)"/>
                    <line x1="30" y1="4" x2="30" y2="6" transform="rotate(-27, 30, 30)"/>
                    <line x1="30" y1="4" x2="30" y2="7" transform="rotate(0, 30, 30)"/>
                    <line x1="30" y1="4" x2="30" y2="6" transform="rotate(27, 30, 30)"/>
                    <line x1="30" y1="4" x2="30" y2="6" transform="rotate(54, 30, 30)"/>
                    <line x1="30" y1="4" x2="30" y2="6" transform="rotate(81, 30, 30)"/>
                    <line x1="30" y1="4" x2="30" y2="6" transform="rotate(108, 30, 30)"/>
                    <line x1="30" y1="4" x2="30" y2="7" transform="rotate(135, 30, 30)"/>
                </g>
            </svg>
        `;
    }

    injectStyles() {
        if (document.getElementById('eq-unit-styles')) return;

        const style = document.createElement('style');
        style.id = 'eq-unit-styles';
        style.textContent = `
            /* v1.3.0: shared strip contract (see header) */
            .eq-unit-compact {
                box-sizing: border-box;
                width: 505px;
                background: #221C14 linear-gradient(135deg, #2a2318 0%, #1a1510 100%);
                border: 1px solid #5C4033;
                border-radius: 6px;
                padding: 3px 21px;
                font-family: var(--font-serif, 'EB Garamond', Garamond, 'Times New Roman', serif);
                color: #E8D5B7;
                user-select: none;
            }

            .eq-row {
                display: flex;
                align-items: center;
                gap: 10px;
            }

            /* EQ bypass toggle (title + on/off) */
            .eq-bypass-toggle {
                box-sizing: border-box;
                width: 76px;
                flex: none;
                background: #6B5040;
                border: 1px solid #4A3728;
                border-radius: 3px;
                padding: 4px 0;
                font-family: inherit;
                font-size: 11px;
                line-height: 13px;
                font-weight: 600;
                letter-spacing: 1.5px;
                color: #E8D5B7;
                cursor: pointer;
                transition: all 0.1s ease;
                white-space: nowrap;
            }

            .eq-bypass-toggle.active {
                background: #4E6839;
                border-color: #3C5C1A;
                color: #FFF;
            }

            .eq-bypass-toggle:hover,
            .analog-toggle:hover,
            .band-label-compact:hover { filter: brightness(1.1); }

            /* Analog toggle (pinned to the strip's right edge) */
            .analog-toggle {
                flex: none;
                margin-left: auto;
                background: #6B5040;
                border: 1px solid #4A3728;
                border-radius: 3px;
                padding: 4px 8px;
                font-family: inherit;
                font-size: 9px;
                line-height: 11px;
                font-weight: 600;
                letter-spacing: 1px;
                color: #E8D5B7;
                cursor: pointer;
                transition: all 0.1s ease;
            }

            .analog-toggle.active {
                background: #4E6839;
                border-color: #3C5C1A;
                color: #FFF;
            }

            /* Bands: 60px columns, 4px gap = 64px pitch, left-aligned */
            .eq-bands-compact {
                display: flex;
                gap: 4px;
                justify-content: flex-start;
            }

            .eq-band-compact {
                width: 60px;
                display: flex;
                flex-direction: column;
                align-items: center;
                gap: 2px;
            }

            /* Fade the knob graphics only: the value tooltip lives inside the
               wrapper and must stay opaque over the Q row while hovered. */
            .eq-band-compact.disabled .freq-notches-compact,
            .eq-band-compact.disabled .knob-outer,
            .eq-band-compact.disabled .knob-inner {
                opacity: 0.4;
            }

            .eq-band-compact.disabled .three-way-option,
            .eq-band-compact.disabled .three-way-option.active {
                background: #3A2E24; border-color: #4A3728; color: #B8A08C;
            }

            .band-label-compact {
                background: #4E6839;
                border: 1px solid #3C5C1A;
                border-radius: 2px;
                padding: 1px 5px;
                font-family: inherit;
                font-size: 9px;
                line-height: 11px;
                font-weight: 600;
                letter-spacing: 0.5px;
                color: #FFF;
                cursor: pointer;
                transition: all 0.1s ease;
                white-space: nowrap;
            }

            .band-label-compact.inactive {
                background: #6B5040;
                border-color: #4A3728;
                color: #E8D5B7;
            }

            /* Compact dual-ring knob wrapper */
            .dual-knob-wrapper-compact {
                position: relative;
                width: 60px;
                height: 60px;
                display: flex;
                align-items: center;
                justify-content: center;
            }

            .freq-notches-compact {
                position: absolute;
                width: 60px;
                height: 60px;
                pointer-events: none;
            }

            .dual-knob-container {
                position: absolute;
                width: 46px;
                height: 46px;
                cursor: pointer;
            }

            /* Outer ring - frequency */
            .knob-outer {
                position: absolute;
                width: 46px;
                height: 46px;
                border-radius: 50%;
                background: radial-gradient(circle,
                    transparent 62%,
                    rgba(107, 142, 78, 0.3) 62%,
                    rgba(107, 142, 78, 0.5) 72%,
                    rgba(139, 115, 85, 0.8) 82%,
                    #8B7355 88%
                );
                border: 2px solid #6B8E4E;
                transform-origin: center center;
                pointer-events: none;
            }

            .knob-outer-indicator {
                position: absolute;
                top: 2px;
                left: 50%;
                transform: translateX(-50%);
                width: 3px;
                height: 7px;
                background: #3C5C1A;
                border-radius: 1.5px;
                pointer-events: none;
            }

            /* Inner dial - gain (seed pattern) */
            .knob-inner {
                position: absolute;
                top: 9px;
                left: 9px;
                width: 28px;
                height: 28px;
                border-radius: 50%;
                background:
                    radial-gradient(circle, #FFF8DC 0%, #FFF8DC 18%, transparent 18%),
                    conic-gradient(from 0deg,
                        #F5DEB3 0deg, #F5DEB3 35deg, #8B7355 35deg, #8B7355 36deg,
                        #E8D5B7 36deg, #E8D5B7 71deg, #8B7355 71deg, #8B7355 72deg,
                        #F5DEB3 72deg, #F5DEB3 107deg, #8B7355 107deg, #8B7355 108deg,
                        #E8D5B7 108deg, #E8D5B7 143deg, #8B7355 143deg, #8B7355 144deg,
                        #F5DEB3 144deg, #F5DEB3 179deg, #8B7355 179deg, #8B7355 180deg,
                        #E8D5B7 180deg, #E8D5B7 215deg, #8B7355 215deg, #8B7355 216deg,
                        #F5DEB3 216deg, #F5DEB3 251deg, #8B7355 251deg, #8B7355 252deg,
                        #E8D5B7 252deg, #E8D5B7 287deg, #8B7355 287deg, #8B7355 288deg,
                        #F5DEB3 288deg, #F5DEB3 323deg, #8B7355 323deg, #8B7355 324deg,
                        #E8D5B7 324deg, #E8D5B7 359deg, #8B7355 359deg, #8B7355 360deg
                    );
                border: 2px solid #8B7355;
                transform-origin: center center;
                pointer-events: none;
            }

            .knob-pointer {
                position: absolute;
                top: 2px;
                left: 50%;
                transform: translateX(-50%);
                width: 2px;
                height: 7px;
                background: #3C2F2F;
                pointer-events: none;
            }

            /* Value tooltip */
            .value-tooltip {
                position: absolute;
                bottom: -16px;
                left: 50%;
                transform: translateX(-50%);
                background: rgba(60, 47, 47, 0.95);
                color: #FFF;
                padding: 2px 4px;
                border-radius: 2px;
                font-size: 9px;
                line-height: 11px;
                white-space: nowrap;
                pointer-events: none;
                opacity: 0;
                transition: opacity 0.15s;
                z-index: 10;
            }

            .dual-knob-container:hover .value-tooltip,
            .dual-knob-container.dragging .value-tooltip {
                opacity: 1;
            }

            /* Value-entry field, overlaid on its half of the tooltip (which keeps
               its text, transparent), so nothing shifts. Form controls do not
               inherit fonts on their own, hence the inherits. The tooltip is
               pointer-events: none decoration, which the field would inherit, so
               it takes clicks back; and the tooltip stays up while a field is
               open, or it fades the moment the pointer leaves the knob. */
            .eq-value-input {
                position: absolute; left: 0; top: 0; width: 100%; height: 100%;
                box-sizing: border-box; padding: 0; margin: 0;
                border: none; border-bottom: 1px solid currentColor; border-radius: 0;
                background: transparent;
                font-family: inherit; font-size: inherit; font-weight: inherit;
                line-height: normal; text-align: inherit; outline: none;
                -moz-appearance: textfield;
                pointer-events: auto;
            }
            .eq-value-input::-webkit-outer-spin-button,
            .eq-value-input::-webkit-inner-spin-button { -webkit-appearance: none; margin: 0; }
            .value-tooltip:has(.eq-value-input) { opacity: 1; background: rgb(60, 47, 47); }

            /* Compact Q selector */
            .three-way-toggle-compact {
                display: flex;
                gap: 1px;
                background: #5C4033;
                border: 1px solid #5C4033;
                border-radius: 2px;
                overflow: hidden;
            }

            .three-way-toggle-compact .three-way-option {
                padding: 0 4px;
                font-size: 9px;
                line-height: 11px;
                font-weight: 600;
                letter-spacing: 0.3px;
                color: #E8D5B7;
                cursor: pointer;
                background: #6B5040;
                transition: background 0.1s;
            }

            .three-way-toggle-compact .three-way-option.active {
                background: #4E6839;
                color: #FFF;
            }

            .q-spacer-compact {
                height: 13px;
            }

            /* Bypassed: knob graphics fade, text controls go muted (still AA) */
            .eq-unit-compact.bypassed .eq-bands-compact,
            .eq-unit-compact.bypassed .analog-toggle {
                pointer-events: none;
            }

            .eq-unit-compact.bypassed .freq-notches-compact,
            .eq-unit-compact.bypassed .knob-outer,
            .eq-unit-compact.bypassed .knob-inner {
                opacity: 0.4;
            }

            .eq-unit-compact.bypassed .band-label-compact,
            .eq-unit-compact.bypassed .analog-toggle,
            .eq-unit-compact.bypassed .three-way-option,
            .eq-unit-compact.bypassed .three-way-option.active {
                background: #3A2E24; border-color: #4A3728; color: #B8A08C;
            }
        `;
        document.head.appendChild(style);
    }

    bindParameters() {
        const prefix = this.paramPrefix;

        // Slider parameters (removed output_gain)
        const sliderParams = [
            'lf_freq', 'lf_gain',
            'lmf_freq', 'lmf_gain',
            'hmf_freq', 'hmf_gain',
            'hf_freq', 'hf_gain'
        ];

        sliderParams.forEach(param => {
            if (this.getSliderState) {
                const state = this.getSliderState(prefix + param);
                if (state) {
                    this.sliders[param] = state;
                }
            }
        });

        // Toggle parameters (added 'enabled')
        const toggleParams = ['lf_on', 'lmf_on', 'hmf_on', 'hf_on', 'analog', 'enabled'];

        toggleParams.forEach(param => {
            if (this.getToggleState) {
                const state = this.getToggleState(prefix + param);
                if (state) {
                    this.toggles[param] = state;
                }
            }
        });

        // ComboBox parameters
        const comboParams = ['lmf_q', 'hmf_q'];

        comboParams.forEach(param => {
            if (this.getComboBoxState) {
                const state = this.getComboBoxState(prefix + param);
                if (state) {
                    this.combos[param] = state;
                }
            }
        });
    }

    attachEventListeners() {
        // Setup dual-ring knobs for each band
        ['lf', 'lmf', 'hmf', 'hf'].forEach(band => {
            this.setupDualKnob(band);
        });

        // Setup band toggles
        this.container.querySelectorAll('.band-label-compact').forEach(btn => {
            const param = btn.dataset.param;
            btn.addEventListener('click', () => this.toggleBand(param));

            const state = this.toggles[param];
            if (state) {
                state.valueChangedEvent.addListener(() => this.updateBandToggle(param));
                this.updateBandToggle(param);
            }
        });

        // Setup EQ bypass toggle (new in v1.1.0)
        const eqBypassBtn = this.container.querySelector('.eq-bypass-toggle');
        if (eqBypassBtn) {
            eqBypassBtn.addEventListener('click', () => this.toggleEnabled());
            const state = this.toggles['enabled'];
            if (state) {
                state.valueChangedEvent.addListener(() => this.updateEnabledToggle());
                this.updateEnabledToggle();
            }
        }

        // Setup analog toggle
        const analogBtn = this.container.querySelector('.analog-toggle');
        if (analogBtn) {
            analogBtn.addEventListener('click', () => this.toggleAnalog());
            const state = this.toggles['analog'];
            if (state) {
                state.valueChangedEvent.addListener(() => this.updateAnalogToggle());
                this.updateAnalogToggle();
            }
        }

        // Setup Q selectors
        this.container.querySelectorAll('.three-way-toggle-compact').forEach(toggle => {
            this.setupQToggle(toggle);
        });

        // Global mouse events for drag
        document.addEventListener('mousemove', this.boundOnDrag);
        document.addEventListener('mouseup', this.boundEndDrag);
    }

    setupDualKnob(band) {
        const container = this.container.querySelector(`[data-param-outer="${band}_freq"]`);
        if (!container) return;

        const outerKnob = container.querySelector('.knob-outer');
        const innerKnob = container.querySelector('.knob-inner');
        const tooltip = this.container.querySelector(`#${band}_tooltip`);
        const freqText = tooltip.querySelector('.vt-freq');   // frequency half of the tooltip
        const gainText = tooltip.querySelector('.vt-gain');

        const freqState = this.sliders[`${band}_freq`];
        const gainState = this.sliders[`${band}_gain`];

        if (!freqState || !gainState) return;

        // WR-09: format the scaled (engineering) value so skewed freq params read
        // correctly. Value entry calls these with the state's own normalised value,
        // so they read getScaledValue() straight from the state.
        const freqFormat = () => this.paramRanges[`${band}_freq`].format(freqState.getScaledValue());
        freqFormat.parse = parseFreq;
        const gainFormat = () => this.paramRanges[`${band}_gain`].format(gainState.getScaledValue());
        gainFormat.parse = parseDirect;

        // Each half is written on its own, so an open value field in one half is
        // never wiped by an update to either.
        const updateTooltip = () => {
            if (freqText.dataset.editing !== '1') freqText.textContent = freqFormat();
            if (gainText.dataset.editing !== '1') gainText.textContent = gainFormat();
        };

        // ONE ring hit test, shared by the drag, the value entry and the reset, so
        // all three act on the same ring: outer (> 60 % from centre) = frequency.
        const isOuterRing = (e) => {
            const rect = container.getBoundingClientRect();
            const centerX = rect.left + rect.width / 2;
            const centerY = rect.top + rect.height / 2;

            const dx = e.clientX - centerX;
            const dy = e.clientY - centerY;
            const distance = Math.sqrt(dx * dx + dy * dy);
            const normalizedDistance = distance / (rect.width / 2);
            return normalizedDistance > this.INNER_THRESHOLD;
        };

        const updateKnobRotation = (element, normalizedValue) => {
            const degrees = -135 + (normalizedValue * 270);
            element.style.transform = `rotate(${degrees}deg)`;
        };

        // Alt/Option-click resets the ring under the pointer to its default as one
        // host gesture. CAPTURE phase, registered before the drag listener below,
        // so it runs first and cancels the drag outright (stopImmediatePropagation);
        // it never touches the drag state.
        container.addEventListener('mousedown', (e) => {
            if (!e.altKey) return;
            if (e.target.closest('.eq-value-input')) return;   // presses inside an open field
            e.preventDefault();
            e.stopImmediatePropagation();
            const outer = isOuterRing(e);
            const state = outer ? freqState : gainState;
            const def = this.DEFAULT_VALUES[outer ? `${band}_freq` : `${band}_gain`];
            if (typeof def !== 'number') return;
            state.sliderDragStarted();
            state.setNormalisedValue(def);
            state.sliderDragEnded();
        }, true);

        // Mouse down - detect which layer
        container.addEventListener('mousedown', (e) => {
            // Outer ring (freq) if > 60% from center, otherwise inner (gain)
            if (isOuterRing(e)) {
                this.currentState = freqState;
                this.currentParamName = `${band}_freq`;
            } else {
                this.currentState = gainState;
                this.currentParamName = `${band}_gain`;
            }

            this.isDragging = true;
            this.lastY = e.clientY;
            this.activeContainer = container;
            container.classList.add('dragging');

            if (this.currentState.sliderDragStarted) {
                this.currentState.sliderDragStarted();
            }

            e.preventDefault();
        });

        // Double-click opens inline value entry for the ring under the pointer
        // (Alt/Option-click resets instead), in that ring's half of the tooltip.
        container.addEventListener('dblclick', (e) => {
            if (e.altKey) return;
            e.preventDefault();
            if (isOuterRing(e)) attachValueEntry(freqText, freqState, freqFormat);
            else attachValueEntry(gainText, gainState, gainFormat);
        });

        // Listen for value changes
        freqState.valueChangedEvent.addListener(() => {
            updateKnobRotation(outerKnob, freqState.getNormalisedValue());
            updateTooltip();
        });

        gainState.valueChangedEvent.addListener(() => {
            updateKnobRotation(innerKnob, gainState.getNormalisedValue());
            updateTooltip();
        });

        // Initial state
        updateKnobRotation(outerKnob, freqState.getNormalisedValue());
        updateKnobRotation(innerKnob, gainState.getNormalisedValue());
        updateTooltip();
    }

    onDrag(e) {
        if (!this.isDragging || !this.currentState) return;

        const deltaY = this.lastY - e.clientY;
        const currentValue = this.currentState.getNormalisedValue();
        const newValue = Math.max(0, Math.min(1, currentValue + (deltaY * 0.005)));
        this.currentState.setNormalisedValue(newValue);
        this.lastY = e.clientY;
    }

    endDrag() {
        if (this.isDragging && this.currentState) {
            if (this.currentState.sliderDragEnded) {
                this.currentState.sliderDragEnded();
            }
            if (this.activeContainer) {
                this.activeContainer.classList.remove('dragging');
            }
        }
        this.isDragging = false;
        this.currentState = null;
        this.currentParamName = null;
        this.activeContainer = null;
    }

    toggleBand(param) {
        const state = this.toggles[param];
        if (state) {
            state.setValue(!state.getValue());
        }
    }

    updateBandToggle(param) {
        const state = this.toggles[param];
        if (!state) return;

        const isOn = state.getValue();
        const btn = this.container.querySelector(`[data-param="${param}"]`);
        const bandId = param.replace('_on', '');
        const band = this.container.querySelector(`[data-band="${bandId}"]`);

        if (btn) {
            btn.classList.toggle('inactive', !isOn);
        }
        if (band) {
            band.classList.toggle('disabled', !isOn);
        }
    }

    // v1.1.0: EQ bypass toggle
    toggleEnabled() {
        const state = this.toggles['enabled'];
        if (state) {
            state.setValue(!state.getValue());
        }
    }

    updateEnabledToggle() {
        const state = this.toggles['enabled'];
        if (!state) return;

        const isEnabled = state.getValue();
        const btn = this.container.querySelector('.eq-bypass-toggle');
        const unit = this.container.querySelector('.eq-unit-compact');

        if (btn) {
            btn.classList.toggle('active', isEnabled);
        }
        if (unit) {
            unit.classList.toggle('bypassed', !isEnabled);
        }
    }

    toggleAnalog() {
        const state = this.toggles['analog'];
        if (state) {
            state.setValue(!state.getValue());
        }
    }

    updateAnalogToggle() {
        const state = this.toggles['analog'];
        if (!state) return;

        const btn = this.container.querySelector('.analog-toggle');
        if (btn) {
            btn.classList.toggle('active', state.getValue());
        }
    }

    setupQToggle(container) {
        const param = container.dataset.param;
        const state = this.combos[param];
        if (!state) return;

        const options = container.querySelectorAll('.three-way-option');

        options.forEach((option) => {
            option.addEventListener('click', () => {
                const value = parseInt(option.dataset.value, 10);
                state.setChoiceIndex(value);
            });
        });

        const updateOptions = () => {
            const index = state.getChoiceIndex();
            options.forEach((opt) => {
                opt.classList.toggle('active', parseInt(opt.dataset.value, 10) === index);
            });
        };

        state.valueChangedEvent.addListener(updateOptions);
        updateOptions();
    }

    /**
     * Update VU meter (call from timer callback)
     * Note: VU meter removed in v1.1.0 compact layout
     */
    updateMeter(levelDB) {
        // No-op in compact layout (meter removed)
    }

    destroy() {
        document.removeEventListener('mousemove', this.boundOnDrag);
        document.removeEventListener('mouseup', this.boundEndDrag);
    }
}

export default AnalogEQUnitUI;
