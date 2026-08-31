/*
   This file is part of O-GrainScatter, an Ouaricon Audio plugin.
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
// O-GrainScatter — Parameter binding, grain visualization, Euclidean circle
// JUCE 8 WebView integration

import * as Juce from './juce/index.js';

// ════════════════════════════════════════════════════════════════════════════
// i18n — CANON V2, verbatim. scripts/check-i18n.js assertion 6 strips comments,
// normalises whitespace and byte-compares this region against
// scripts/i18n-canon.js. Do not edit it here; edit the canon.
//
// PLACED AT THE TOP, ABOVE EVERY OTHER DECLARATION, and placed by where the
// page first READS trLabel() rather than by convention. `let uiLanguage` is a
// lexical binding: an initializer that ran ABOVE this block and reached into it
// would be a TDZ ReferenceError that takes the whole UI with it, and every
// later initializer on the page with it (pattern_module_toplevel_init_tdz).
// This module's only reader is init(), which is still called LAST.
// ════════════════════════════════════════════════════════════════════════════
import { LANGUAGES, I18N, LABELS, TIP_BINDINGS, tr } from './i18n.js';

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

    // WR-11: skew-correct reset defaults come from C++ (the real NormalisableRange), fetched
    // async at startup and ready long before any double-click. No hand-coded JS defaults.
    const getParameterDefaults = Juce.getNativeFunction('getParameterDefaults');
    let paramDefaults = {};

    // ════════════════════════════════════════════════════════════════════
    // Knob drag system
    // ════════════════════════════════════════════════════════════════════

    const SENSITIVITY = 0.005;
    const ANGLE_MIN = -140;
    const ANGLE_MAX = 140;
    const ANGLE_RANGE = ANGLE_MAX - ANGLE_MIN;

    function setupKnob(paramId, state, formatter) {
        const knobEl = document.querySelector(`.knob[data-param="${paramId}"]`);
        const indicator = knobEl ? knobEl.querySelector('.knob-indicator') : null;
        const valueEl = document.querySelector(`[data-value="${paramId}"]`);
        if (!knobEl || !indicator) return;

        let isDragging = false;
        let lastY = 0;

        function updateDisplay() {
            const norm = state.getNormalisedValue();
            const angle = ANGLE_MIN + norm * ANGLE_RANGE;
            indicator.style.transform = `translateX(-50%) rotate(${angle}deg)`;
            // WR-11: format the engineering value JUCE reports via getScaledValue() — the real
            // range/skew pushed from C++ — instead of re-deriving it from a hardcoded JS range.
            if (valueEl && formatter) valueEl.textContent = formatter(state.getScaledValue());
        }

        // Sync from JUCE
        state.valueChangedEvent.addListener(updateDisplay);

        // Initial sync
        updateDisplay();

        knobEl.addEventListener('mousedown', (e) => {
            isDragging = true;
            lastY = e.clientY;
            state.sliderDragStarted();
            e.preventDefault();
        });

        document.addEventListener('mousemove', (e) => {
            if (!isDragging) return;
            const deltaY = lastY - e.clientY;
            const norm = state.getNormalisedValue();
            const newNorm = Math.max(0, Math.min(1, norm + deltaY * SENSITIVITY));
            state.setNormalisedValue(newNorm);
            lastY = e.clientY;
        });

        document.addEventListener('mouseup', () => {
            if (isDragging) {
                state.sliderDragEnded();
                isDragging = false;
            }
        });

        // Double-click to reset to the C++-provided (skew-correct) default
        knobEl.addEventListener('dblclick', (e) => {
            e.preventDefault();
            const def = paramDefaults[paramId];
            if (def === undefined) return;   // defaults not loaded yet — no-op
            state.sliderDragStarted();
            state.setNormalisedValue(def);
            state.sliderDragEnded();
        });
    }

    // ════════════════════════════════════════════════════════════════════
    // ComboBox binding
    // ════════════════════════════════════════════════════════════════════

    function setupComboBox(paramId, state) {
        const selectEl = document.querySelector(`select[data-param="${paramId}"]`);
        if (!selectEl) return;

        state.valueChangedEvent.addListener(() => {
            selectEl.selectedIndex = state.getChoiceIndex();
        });

        selectEl.selectedIndex = state.getChoiceIndex();

        selectEl.addEventListener('change', () => {
            state.setChoiceIndex(selectEl.selectedIndex);
        });
    }

    // ════════════════════════════════════════════════════════════════════
    // Toggle binding
    // ════════════════════════════════════════════════════════════════════

    function setupToggle(paramId, state) {
        const btnEl = document.querySelector(`[data-param="${paramId}"].toggle`);
        if (!btnEl) return;

        state.valueChangedEvent.addListener(() => {
            btnEl.classList.toggle('active', state.getValue());
        });

        btnEl.classList.toggle('active', state.getValue());

        btnEl.addEventListener('click', () => {
            state.setValue(!state.getValue());
        });
    }

    // ════════════════════════════════════════════════════════════════════
    // Value formatters — receive the SCALED (engineering-unit) value from
    // state.getScaledValue(); they add units/precision only, and never
    // re-implement the C++ NormalisableRange or its skew (WR-11).
    // ════════════════════════════════════════════════════════════════════

    const pctFormatter          = (v) => Math.round(v) + '%';   // any 0-N range shown as N%
    const grainSizeFormatter    = (v) => Math.round(v) + ' ms';
    const repeatsFormatter      = (v) => String(Math.round(v));
    const eucPulsesFormatter    = repeatsFormatter;
    const eucStepsFormatter     = repeatsFormatter;
    const eucRotationFormatter  = repeatsFormatter;
    const swingFormatter        = (v) => Math.round(v) + '%';

    // Spatial — getScaledValue() already yields real degrees (incl. negative elevation);
    // 176 is the degree-sign code point.
    const degreeFormatter = (v) => Math.round(v) + String.fromCharCode(176);

    const azimuthFormatter       = degreeFormatter;
    const elevationFormatter     = degreeFormatter;
    const azSpreadFormatter      = degreeFormatter;
    const elSpreadFormatter      = degreeFormatter;
    const trajSpeedFormatter     = (v) => Math.round(v) + '%';
    const spatialSmoothFormatter = (v) => Math.round(v) + ' ms';

    // ════════════════════════════════════════════════════════════════════
    // Initialize all parameter bindings
    // ════════════════════════════════════════════════════════════════════

    function initParams() {
        // Sliders — double-click reset defaults come from C++ via getParameterDefaults (WR-11)
        setupKnob('grain_size',       Juce.getSliderState('grain_size'),       grainSizeFormatter);
        setupKnob('density',          Juce.getSliderState('density'),          pctFormatter);
        setupKnob('scan_position',    Juce.getSliderState('scan_position'),    pctFormatter);
        setupKnob('spread',           Juce.getSliderState('spread'),           pctFormatter);
        setupKnob('reverse',          Juce.getSliderState('reverse'),          pctFormatter);
        setupKnob('feedback',         Juce.getSliderState('feedback'),         pctFormatter);
        setupKnob('dry_wet',          Juce.getSliderState('dry_wet'),          pctFormatter);
        setupKnob('pitch_random',     Juce.getSliderState('pitch_random'),     pctFormatter);
        setupKnob('pan_random',       Juce.getSliderState('pan_random'),       pctFormatter);
        setupKnob('size_random',      Juce.getSliderState('size_random'),      pctFormatter);
        setupKnob('amp_random',       Juce.getSliderState('amp_random'),       pctFormatter);
        setupKnob('probability',      Juce.getSliderState('probability'),      pctFormatter);
        setupKnob('repeats',          Juce.getSliderState('repeats'),          repeatsFormatter);
        setupKnob('euclidean_pulses',   Juce.getSliderState('euclidean_pulses'),   eucPulsesFormatter);
        setupKnob('euclidean_steps',    Juce.getSliderState('euclidean_steps'),    eucStepsFormatter);
        setupKnob('euclidean_rotation', Juce.getSliderState('euclidean_rotation'), eucRotationFormatter);
        setupKnob('euclidean_swing',    Juce.getSliderState('euclidean_swing'),    swingFormatter);

        // ComboBoxes (5)
        setupComboBox('scale',       Juce.getComboBoxState('scale'));
        setupComboBox('root_note',   Juce.getComboBoxState('root_note'));
        setupComboBox('pitch_mode',  Juce.getComboBoxState('pitch_mode'));
        setupComboBox('sync_mode',   Juce.getComboBoxState('sync_mode'));
        setupComboBox('grain_shape', Juce.getComboBoxState('grain_shape'));

        // Toggles (2)
        setupToggle('freeze',       Juce.getToggleState('freeze'));
        setupToggle('stutter_gate', Juce.getToggleState('stutter_gate'));

        // Spatial knobs (6)
        setupKnob('azimuth',        Juce.getSliderState('azimuth'),        azimuthFormatter);
        setupKnob('elevation',      Juce.getSliderState('elevation'),      elevationFormatter);
        setupKnob('az_spread',      Juce.getSliderState('az_spread'),      azSpreadFormatter);
        setupKnob('el_spread',      Juce.getSliderState('el_spread'),      elSpreadFormatter);
        setupKnob('distance',       Juce.getSliderState('distance'),       pctFormatter);
        setupKnob('spatial_width',  Juce.getSliderState('spatial_width'),  pctFormatter);

        // Spatial extended (4)
        setupKnob('traj_speed',      Juce.getSliderState('traj_speed'),      trajSpeedFormatter);
        setupKnob('dist_lpf',        Juce.getSliderState('dist_lpf'),        pctFormatter);
        setupKnob('doppler',         Juce.getSliderState('doppler'),         pctFormatter);
        setupKnob('spatial_smooth',  Juce.getSliderState('spatial_smooth'),  spatialSmoothFormatter);

        // Spatial comboboxes (2)
        setupComboBox('spatial_mode', Juce.getComboBoxState('spatial_mode'));
        setupComboBox('trajectory',   Juce.getComboBoxState('trajectory'));
    }

    // ════════════════════════════════════════════════════════════════════
    // Pitch gate: dim Scale / Root Note / Pitch Mode when Pitch Rnd = 0
    // ════════════════════════════════════════════════════════════════════

    function setupPitchGate() {
        const pitchRandomState = Juce.getSliderState('pitch_random');
        const scaleContainer = document.querySelector('select[data-param="scale"]')?.closest('.dropdown-container');
        const rootNoteContainer = document.querySelector('select[data-param="root_note"]')?.closest('.dropdown-container');
        const pitchModeContainer = document.querySelector('select[data-param="pitch_mode"]')?.closest('.dropdown-container');
        const pitchHint = document.getElementById('pitch-hint');

        if (!scaleContainer || !rootNoteContainer || !pitchModeContainer) return;

        function update() {
            const isDimmed = pitchRandomState.getNormalisedValue() < 0.01;
            scaleContainer.classList.toggle('dimmed', isDimmed);
            rootNoteContainer.classList.toggle('dimmed', isDimmed);
            pitchModeContainer.classList.toggle('dimmed', isDimmed);
            if (pitchHint) pitchHint.classList.toggle('visible', isDimmed);
        }

        pitchRandomState.valueChangedEvent.addListener(update);
        update();
    }

    // ════════════════════════════════════════════════════════════════════
    // Spatial gate: dim controls when spatial_mode = Off
    // ════════════════════════════════════════════════════════════════════

    function setupSpatialGate() {
        const spatialModeState = Juce.getComboBoxState('spatial_mode');
        const spatialGroup = document.getElementById('spatial-group');
        const spatialHint = document.getElementById('spatial-hint');

        if (!spatialGroup) return;

        // All interactive children except the mode dropdown itself
        const knobs = spatialGroup.querySelectorAll('.knob-container');
        const trajContainer = spatialGroup.querySelector('select[data-param="trajectory"]')?.closest('.dropdown-container');

        function update() {
            const isOff = spatialModeState.getChoiceIndex() === 0;
            knobs.forEach(k => k.style.opacity = isOff ? '0.25' : '1');
            knobs.forEach(k => k.style.pointerEvents = isOff ? 'none' : 'auto');
            if (trajContainer) {
                trajContainer.style.opacity = isOff ? '0.25' : '1';
                trajContainer.style.pointerEvents = isOff ? 'none' : 'auto';
            }
            if (spatialHint) spatialHint.classList.toggle('visible', isOff);
        }

        spatialModeState.valueChangedEvent.addListener(update);
        update();
    }

    // ════════════════════════════════════════════════════════════════════
    // Shared canvas resize (DPR-aware backing store)
    // ════════════════════════════════════════════════════════════════════

    function resizeCanvas(viz) {
        const dpr = window.devicePixelRatio || 1;
        const rect = viz.canvas.parentElement.getBoundingClientRect();
        viz.w = rect.width;
        viz.h = rect.height;
        viz.canvas.width = viz.w * dpr;
        viz.canvas.height = viz.h * dpr;
        viz.canvas.style.width = viz.w + 'px';
        viz.canvas.style.height = viz.h + 'px';
        viz.ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
    }

    // ════════════════════════════════════════════════════════════════════
    // Grain Scatter Visualization (Canvas 2D)
    // ════════════════════════════════════════════════════════════════════

    class GrainScatterViz {
        constructor(canvas) {
            this.canvas = canvas;
            this.ctx = canvas.getContext('2d');
            this.grains = [];
            this.activeCount = 0;
            this.resize();
        }

        resize() {
            resizeCanvas(this);
        }

        update(data) {
            this.grains = data.g || [];
            this.activeCount = data.ac || 0;
        }

        draw() {
            const ctx = this.ctx;
            const w = this.w;
            const h = this.h;
            ctx.clearRect(0, 0, w, h);

            // Grid lines
            ctx.strokeStyle = 'rgba(139,115,85,0.12)';
            ctx.lineWidth = 0.5;
            // Horizontal (pitch axis)
            for (let i = 1; i < 5; i++) {
                const y = (h / 5) * i;
                ctx.beginPath(); ctx.moveTo(0, y); ctx.lineTo(w, y); ctx.stroke();
            }
            // Vertical (time axis)
            for (let i = 1; i < 5; i++) {
                const x = (w / 5) * i;
                ctx.beginPath(); ctx.moveTo(x, 0); ctx.lineTo(x, h); ctx.stroke();
            }

            // Axis labels
            ctx.fillStyle = 'rgba(139,115,85,0.3)';
            ctx.font = '8px Georgia';
            ctx.fillText('0s', 4, h - 4);
            ctx.fillText('2s', w - 16, h - 4);
            ctx.fillText('+24st', 4, 14);
            ctx.fillText('-24st', 4, h - 14);

            // Center line (0 semitones)
            ctx.strokeStyle = 'rgba(139,115,85,0.2)';
            ctx.lineWidth = 0.5;
            ctx.setLineDash([3, 3]);
            ctx.beginPath(); ctx.moveTo(0, h / 2); ctx.lineTo(w, h / 2); ctx.stroke();
            ctx.setLineDash([]);

            // Draw grains
            for (const g of this.grains) {
                const x = g.p * w;
                // Map pitch: -24 to +24 semitones → bottom to top
                const y = h / 2 - (g.s / 24) * (h / 2);
                const radius = 3 + g.e * 6;
                const alpha = g.e * 0.85 + 0.15;

                if (g.f) {
                    // Frozen: green tint
                    ctx.fillStyle = `rgba(107,142,78,${alpha})`;
                } else {
                    // Live: warm brown
                    ctx.fillStyle = `rgba(139,115,85,${alpha})`;
                }

                ctx.beginPath();
                ctx.arc(x, y, radius, 0, Math.PI * 2);
                ctx.fill();

                // Reverse indicator: small arrow
                if (g.r) {
                    ctx.strokeStyle = `rgba(92,64,51,${alpha * 0.6})`;
                    ctx.lineWidth = 1;
                    ctx.beginPath();
                    ctx.moveTo(x - 3, y);
                    ctx.lineTo(x + 2, y - 3);
                    ctx.moveTo(x - 3, y);
                    ctx.lineTo(x + 2, y + 3);
                    ctx.stroke();
                }
            }

            // Active count
            ctx.fillStyle = 'rgba(139,115,85,0.4)';
            ctx.font = '9px Georgia';
            ctx.textAlign = 'right';
            ctx.fillText(this.activeCount + ' grains', w - 8, 14);
            ctx.textAlign = 'left';
        }
    }

    // ════════════════════════════════════════════════════════════════════
    // Euclidean Circle Visualization (Canvas 2D)
    // ════════════════════════════════════════════════════════════════════

    class EuclideanCircleViz {
        constructor(canvas) {
            this.canvas = canvas;
            this.ctx = canvas.getContext('2d');
            this.pattern = [];
            this.currentStep = 0;
            this.steps = 8;
            this.rotation = 0;
            this.resize();
        }

        resize() {
            resizeCanvas(this);
        }

        update(data) {
            this.pattern = data.p || [];
            this.currentStep = data.s || 0;
            this.steps = data.n || 8;
            this.rotation = data.r || 0;
        }

        draw() {
            const ctx = this.ctx;
            const w = this.w;
            const h = this.h;
            const cx = w / 2;
            const cy = h / 2;
            const radius = Math.min(cx, cy) - 20;

            ctx.clearRect(0, 0, w, h);

            // Rotation offset in radians (rotate entire pattern display)
            const rotOffset = this.steps > 0
                ? (this.rotation / this.steps) * Math.PI * 2
                : 0;

            // Outer circle
            ctx.strokeStyle = 'rgba(139,115,85,0.25)';
            ctx.lineWidth = 1;
            ctx.beginPath();
            ctx.arc(cx, cy, radius, 0, Math.PI * 2);
            ctx.stroke();

            // Inner circle
            ctx.strokeStyle = 'rgba(139,115,85,0.1)';
            ctx.beginPath();
            ctx.arc(cx, cy, radius * 0.4, 0, Math.PI * 2);
            ctx.stroke();

            if (this.steps === 0) return;

            // Rotation indicator line (from center to rotation start position)
            if (this.rotation > 0) {
                const rotAngle = -Math.PI / 2 + rotOffset;
                ctx.strokeStyle = 'rgba(107,142,78,0.35)';
                ctx.lineWidth = 1;
                ctx.setLineDash([3, 3]);
                ctx.beginPath();
                ctx.moveTo(cx, cy);
                ctx.lineTo(cx + radius * 0.95 * Math.cos(rotAngle),
                           cy + radius * 0.95 * Math.sin(rotAngle));
                ctx.stroke();
                ctx.setLineDash([]);
            }

            // Connect active steps with polygon (using rotated positions)
            const activePositions = [];
            for (let i = 0; i < this.steps; i++) {
                const rotIdx = (i + this.rotation) % this.steps;
                if (this.pattern[rotIdx]) {
                    const angle = (i / this.steps) * Math.PI * 2 - Math.PI / 2;
                    activePositions.push({
                        x: cx + radius * Math.cos(angle),
                        y: cy + radius * Math.sin(angle)
                    });
                }
            }
            if (activePositions.length > 1) {
                ctx.strokeStyle = 'rgba(107,142,78,0.2)';
                ctx.lineWidth = 1;
                ctx.beginPath();
                ctx.moveTo(activePositions[0].x, activePositions[0].y);
                for (let i = 1; i < activePositions.length; i++) {
                    ctx.lineTo(activePositions[i].x, activePositions[i].y);
                }
                ctx.closePath();
                ctx.stroke();
            }

            // Draw step dots (rotated pattern readout)
            for (let i = 0; i < this.steps; i++) {
                const angle = (i / this.steps) * Math.PI * 2 - Math.PI / 2;
                const x = cx + radius * Math.cos(angle);
                const y = cy + radius * Math.sin(angle);

                const isCurrent = i === this.currentStep;
                const rotIdx = (i + this.rotation) % this.steps;
                const isActive = !!this.pattern[rotIdx];

                const dotRadius = isCurrent ? 8 : 5;

                ctx.beginPath();
                ctx.arc(x, y, dotRadius, 0, Math.PI * 2);

                if (isCurrent && isActive) {
                    ctx.fillStyle = '#6B8E4E';
                } else if (isCurrent) {
                    ctx.fillStyle = 'rgba(107,142,78,0.5)';
                } else if (isActive) {
                    ctx.fillStyle = '#8B7355';
                } else {
                    ctx.fillStyle = 'rgba(139,115,85,0.15)';
                }
                ctx.fill();

                // Border on current step
                if (isCurrent) {
                    ctx.strokeStyle = '#3C5C1A';
                    ctx.lineWidth = 2;
                    ctx.stroke();
                }
            }

            // Step counter in center (show rotation if non-zero)
            ctx.fillStyle = 'rgba(139,115,85,0.4)';
            ctx.font = '10px Georgia';
            ctx.textAlign = 'center';
            ctx.textBaseline = 'middle';
            const activeCount = this.pattern.filter(Boolean).length;
            let label = activeCount + '/' + this.steps;
            if (this.rotation > 0) label += ' r' + this.rotation;
            ctx.fillText(label, cx, cy);
            ctx.textBaseline = 'alphabetic';
            ctx.textAlign = 'left';
        }
    }

    // ════════════════════════════════════════════════════════════════════
    // Settings popover — the language selector (v2.5.0)
    // ════════════════════════════════════════════════════════════════════

    function initSettingsPopover() {
        const gearBtn  = document.getElementById('gear-btn');
        const popover  = document.getElementById('settings-popover');
        if (!gearBtn || !popover) {
            console.warn('settings popover missing \u2014 language selector unavailable');
            return;
        }

        const setOpen = (open) => {
            popover.hidden = !open;
            gearBtn.setAttribute('aria-expanded', open ? 'true' : 'false');
        };

        gearBtn.addEventListener('click', (e) => {
            e.stopPropagation();
            setOpen(popover.hidden);
        });

        // Dismiss on a press anywhere else, and on Escape. mousedown rather
        // than click, so the panel is gone before a drag on a knob underneath
        // it begins \u2014 setupKnob starts a drag on mousedown.
        document.addEventListener('mousedown', (e) => {
            if (popover.hidden) return;
            if (popover.contains(e.target) || gearBtn.contains(e.target)) return;
            setOpen(false);
        });

        document.addEventListener('keydown', (e) => {
            if (e.key === 'Escape' && !popover.hidden) {
                setOpen(false);
                gearBtn.focus();
            }
        });
    }

    // ════════════════════════════════════════════════════════════════════
    // Hover-help renderer (v2.6.0)
    // ════════════════════════════════════════════════════════════════════
    //
    // THE COPY ALONE IS INVISIBLE. applyI18n() above writes data-tip-title and
    // data-tip onto the anchors named in TIP_BINDINGS and stops there. Nothing
    // on this page read those attributes before v2.6.0, so authoring 38 bodies
    // into i18n.js without this function would have shipped 38 unpaintable
    // strings past three GREEN gates: check-i18n only counts bindings,
    // check-ui-labels has no tooltip awareness at all, and boot-all-uis counts
    // aria-label and title and never data-tip. tests/ui_tip_render_check.js is
    // the gate that can see a rendered tip, and it exists for that reason.
    //
    // Ported from plugins/O-simpleFM/Source/ui/public/js/app.js:384-462 and
    // styled in this page's own parchment system. Every property below is
    // load-bearing:
    //
    //  1. DELEGATED ON document, not querySelectorAll('[data-tip]') at setup.
    //     No anchor carries data-tip until applyI18n() has run, so a setup-time
    //     query binds NOTHING and fails silently.
    //  2. pointerover / pointerout / focusin / focusout, because they BUBBLE.
    //     pointerenter / focus do not, and delegation needs a bubbling event.
    //  3. pointerout ignores a move between two descendants of the SAME anchor.
    //     A .knob-container holds a caption, a 48 px circle and a readout;
    //     without this the tip flickers off and on at every internal boundary.
    //  4. createElement + textContent, NEVER innerHTML. Localized copy must not
    //     reach a markup path — check-i18n assertion 9 already forbids an angle
    //     bracket in an i18n.js string literal and this is the other half.
    //  5. FLIP FIRST, THEN CLAMP, on all four edges with an 8 px margin. The
    //     clamp runs unconditionally AFTER the flip rather than instead of it,
    //     because a flipped tip can still overflow the other way (O-Bass, at
    //     420x320). This frame is 900x800 and roomy, so the flip is the normal
    //     path only along the bottom row and the right-hand spatial knobs — but
    //     a body long enough to need both is exactly what the render gate's
    //     negative control plants.
    //  6. THE FOCUS ARM IS LATCHED TO THE KEYBOARD. A mouse click on a <button>
    //     focuses it, so an unconditional focusin rule re-opens the tip that
    //     pointerdown just hid and parks it on top of whatever the click opened
    //     — measured on O-Emulator, where clicking the gear pinned the gear's
    //     own tip across the settings popover. :focus-visible is deliberately
    //     NOT the discriminator: Chromium reports it false for a programmatic
    //     .focus() after a click, so a gate driving focus directly would measure
    //     "no tip" and record that as correct. An explicit last-input-device
    //     latch is the same rule and is drivable with real events.
    //  7. Escape hides it; so does any pointerdown.
    //
    // Called AFTER initI18n(), inside the same deferred init and the same
    // try/catch (pattern_module_toplevel_init_tdz).
    function setupTooltips() {
        const tip = document.getElementById('tooltip');
        if (!tip) { console.warn('tooltip surface missing - hover-help unavailable'); return; }

        const MARGIN = 8;
        let active = null;
        let lastInputWasPointer = false;

        const position = (x, y) => {
            const r = tip.getBoundingClientRect();
            const vw = window.innerWidth;
            const vh = window.innerHeight;
            let nx = x + 14;
            let ny = y + 16;
            // Flip to the other side of the cursor when the natural side overflows.
            if (nx + r.width  > vw - MARGIN) nx = x - r.width  - 14;
            if (ny + r.height > vh - MARGIN) ny = y - r.height - 12;
            // Then clamp, unconditionally, on all four edges. Math.max on the
            // upper bound keeps the arithmetic sane for a tip larger than the
            // frame: it lands at MARGIN rather than at a negative coordinate.
            nx = Math.min(Math.max(MARGIN, nx), Math.max(MARGIN, vw - r.width  - MARGIN));
            ny = Math.min(Math.max(MARGIN, ny), Math.max(MARGIN, vh - r.height - MARGIN));
            tip.style.left = `${nx}px`;
            tip.style.top  = `${ny}px`;
        };

        const show = (el, x, y) => {
            const title = el.getAttribute('data-tip-title');
            const body  = el.getAttribute('data-tip');
            if (!title && !body) return;
            tip.textContent = '';
            if (title) {
                const t = document.createElement('span');
                t.className = 'tip-title';
                t.textContent = title;
                tip.appendChild(t);
            }
            if (body) tip.appendChild(document.createTextNode(body));
            tip.classList.add('show');
            tip.setAttribute('aria-hidden', 'false');
            position(x, y);
        };

        const hide = () => {
            tip.classList.remove('show');
            tip.setAttribute('aria-hidden', 'true');
            active = null;
        };

        const anchorOf = (t) => (t && t.closest ? t.closest('[data-tip]') : null);

        document.addEventListener('pointerover', (e) => {
            const el = anchorOf(e.target);
            if (!el || el === active) return;
            active = el;
            show(el, e.clientX, e.clientY);
        });
        document.addEventListener('pointermove', (e) => {
            if (active && anchorOf(e.target) === active) position(e.clientX, e.clientY);
        });
        document.addEventListener('pointerout', (e) => {
            if (!active) return;
            if (anchorOf(e.relatedTarget) === active) return;   // same anchor, child boundary
            hide();
        });
        // A knob drag starts with a pointerdown on the anchor itself, so hiding
        // here also keeps the tip out of the way for the whole drag: active is
        // cleared and no further pointerover arrives while the button is held.
        document.addEventListener('pointerdown', () => { lastInputWasPointer = true; hide(); });

        document.addEventListener('focusin', (e) => {
            if (lastInputWasPointer) return;
            const el = anchorOf(e.target);
            if (!el) return;
            active = el;
            const r = el.getBoundingClientRect();
            show(el, r.left + r.width / 2, r.bottom);
        });
        document.addEventListener('focusout', hide);

        // One keydown listener, two jobs: any key at all means the keyboard is
        // driving again, which releases the latch above; Escape also hides.
        document.addEventListener('keydown', (e) => {
            lastInputWasPointer = false;
            if (e.key === 'Escape') hide();
        });
    }

    // ════════════════════════════════════════════════════════════════════
    // Initialization
    // ════════════════════════════════════════════════════════════════════

    let grainViz, euclideanViz;

    function init() {
        initParams();
        setupPitchGate();
        setupSpatialGate();

        // WR-11: fetch the C++ (skew-correct) reset defaults; resolves well before any
        // double-click, so the dblclick handler in setupKnob reads a populated map.
        getParameterDefaults()
            .then((defs) => { paramDefaults = defs || {}; })
            .catch(() => { /* leave defaults empty — dblclick becomes a no-op */ });

        // Visualizations
        const grainCanvas = document.getElementById('grain-canvas');
        const eucCanvas = document.getElementById('euclidean-canvas');

        if (grainCanvas) grainViz = new GrainScatterViz(grainCanvas);
        if (eucCanvas) euclideanViz = new EuclideanCircleViz(eucCanvas);

        // Event listeners for C++ data push
        window.__JUCE__.backend.addEventListener('grainUpdate', (event) => {
            try {
                const data = JSON.parse(event);
                if (grainViz) grainViz.update(data);
            } catch (e) { /* ignore parse errors */ }
        });

        window.__JUCE__.backend.addEventListener('euclideanUpdate', (event) => {
            try {
                const data = JSON.parse(event);
                if (euclideanViz) euclideanViz.update(data);
            } catch (e) { /* ignore parse errors */ }
        });

        // Render loop
        function renderLoop() {
            if (grainViz) grainViz.draw();
            if (euclideanViz) euclideanViz.draw();
            requestAnimationFrame(renderLoop);
        }
        requestAnimationFrame(renderLoop);

        // Handle resize
        window.addEventListener('resize', () => {
            if (grainViz) grainViz.resize();
            if (euclideanViz) euclideanViz.resize();
        });

        // ── i18n, GUARDED AND LAST ──────────────────────────────────────
        // Every binding above has already run, so a throw in here cannot cost
        // the page a knob, a dropdown or a visualization. The page then reads
        // English on its own authored markup, which is the correct degradation.
        initSettingsPopover();
        // setupTooltips() sits INSIDE the same try/catch and AFTER initI18n(),
        // deliberately on both counts: no anchor carries data-tip until
        // applyI18n() has written it, and a throw in either must not cost the
        // page a knob.
        try { initI18n(); setupTooltips(); } catch (e) { console.error('i18n init failed:', e); }
    }

// ES modules are deferred — DOM is ready when this runs
init();
