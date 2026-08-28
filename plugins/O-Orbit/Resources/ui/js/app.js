/*
   This file is part of O-Orbit, an Ouaricon Audio plugin.
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
// O-Orbit App.js - Parameter Binding + Orbital Visualizer + Speaker Editor
// Phase 3.1: WebView UI + 17 Parameter Controls
// Phase 3.2: Canvas Orbital Visualizer
// Phase 3.3: Speaker Layout Editor + File I/O

import { getSliderState, getComboBoxState, getToggleState, getNativeFunction } from './juce/index.js';
// The canon block below calls Juce.getNativeFunction by NAMESPACE, verbatim
// across all 43 plugins, so the namespace form is imported alongside the named
// bindings this module already used. Two imports of one module is one fetch.
import * as Juce from './juce/index.js';
import { PresetManager } from './modules/preset-manager.js';

// ═══════════════════════════════════════════════════════════════════════════
// INTERFACE LANGUAGE (v1.2.0) — canon v2, verbatim from scripts/i18n-canon.js.
//
// It sits HERE, directly under the imports and above every other declaration in
// this module, because `uiLanguage` is a module-level `let`: anything above it
// that reached applyLabel() would touch it inside its temporal dead zone and
// throw out of module evaluation, taking the knobs, the visualizer, the preset
// band and the hover help with it (pattern_module_toplevel_init_tdz). Nothing
// in this file executes at top level today — every initializer runs from the
// DOMContentLoaded handler — so the ordering is not load-bearing at v1.2.0. It
// is placed defensively anyway: the next eager top-level line anyone adds is
// the one that would find the bug, and `node scripts/boot-all-uis.js` is the
// ONLY gate in the repo that sees this class of failure.
//
// Do NOT edit the region between the import line and the close of initI18n():
// check-i18n assertion 6 byte-compares it against scripts/i18n-canon.js.
// ═══════════════════════════════════════════════════════════════════════════
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

// ─── Settings popover (v1.2.0) ──────────────────────────────────
// The gear panel holding the language selector and the relocated hover-help
// toggle. All state lives in this closure, so nothing here can join a TDZ
// chain. Styled in O-Orbit's own paper-and-sage vocabulary — the same
// #F5E6D3 / #8B7355 plate the preset menu uses, not a widget pasted in from
// another plugin.

function initializeSettingsPopover() {
    const gearBtn = document.getElementById('gear-btn');
    const popover = document.getElementById('settings-popover');

    if (gearBtn === null || popover === null) {
        console.warn('Settings popover missing — language selector unavailable');
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

    // Dismiss on a press anywhere else, and on Escape. mousedown rather than
    // click, so the panel is gone before a drag on a knob underneath it begins
    // — the knobs call preventDefault in their own mousedown handlers.
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

// ─── Motion State ───────────────────────────────────────────────

let motionState = { azL: 0, elL: 0, azR: 0, elR: 0, dist: 1, split: false };
let speakers = [];  // Current speaker positions from backend
let trailL = [];    // Circular buffer of L source positions
let trailR = [];    // Circular buffer of R source positions
const TRAIL_LENGTH = 120; // 2 seconds at 60fps

// ─── View Mode & Editor State ───────────────────────────────────

let viewMode = 'motion'; // 'motion' or 'editor'
let hoveredSpeakerIndex = -1;
let draggingSpeakerIndex = -1;
let canvasRef = null;
let canvasCtxRef = null;

// ─── Initialization ─────────────────────────────────────────────

document.addEventListener('DOMContentLoaded', () => {
    // v1.2.0 — the popover and the language sweep go FIRST, each in its own
    // try/catch. A translation-table typo must not be allowed to take the
    // eighteen parameter bindings, the canvas and the preset band down with it,
    // which is the v1.4.0 TDZ failure this repo has already paid for once.
    // Running applyI18n() before the bindings also means bindToggle() writes
    // its first face into an already-swept page rather than being swept a
    // second time immediately afterwards.
    try { initializeSettingsPopover(); } catch (e) { console.error('settings popover init failed:', e); }
    try { initI18n(); }                  catch (e) { console.error('i18n init failed:', e); }

    initializeParameters();
    initializeVisualizer();
    initializePresetBand();   // B1 (v1.1.0)
    initializeHoverHelp();    // B2 (v1.1.0)
});

function initializeParameters() {
    // Slider parameters (11 knobs)
    bindKnob('speed', 'Hz');
    bindKnob('width', '\u00B0');
    bindKnob('depth', '%');
    bindKnob('tilt', '\u00B0');
    bindKnob('phase', '\u00B0');
    bindKnob('elevation_range', '\u00B0');
    bindKnob('distance', 'm');
    bindKnob('air_absorption', '%');
    bindKnob('center_diverge', '%');
    bindKnob('lr_offset', '\u00B0');
    bindKnob('mix', '%');

    // Choice parameters (5 dropdowns)
    bindDropdown('path');
    bindDropdown('tempo_sync');
    bindDropdown('speaker_layout');
    bindDropdown('attenuation_curve');
    bindDropdown('source_mode');

    // Bool parameter (1 toggle)
    bindToggle('elevation_enable');
}

// ─── Knob Binding ───────────────────────────────────────────────

function bindKnob(paramId, unit) {
    const knobElement = document.getElementById(paramId);
    const valueDisplay = document.getElementById(paramId + '-value');

    if (!knobElement) return;

    const state = getSliderState(paramId);

    function updateFromState() {
        const norm = state.getNormalisedValue();
        updateKnobRotation(knobElement, norm);
        if (valueDisplay) {
            const scaled = state.getScaledValue();
            valueDisplay.textContent = formatValue(scaled, unit);
        }
    }

    updateFromState();
    state.valueChangedEvent.addListener(updateFromState);
    state.propertiesChangedEvent.addListener(updateFromState);

    let isDragging = false;
    let lastY = 0;

    knobElement.addEventListener('mousedown', (e) => {
        e.preventDefault();
        isDragging = true;
        lastY = e.clientY;
        knobElement.style.cursor = 'grabbing';
        state.sliderDragStarted();
    });

    document.addEventListener('mousemove', (e) => {
        if (!isDragging) return;
        const delta = lastY - e.clientY;
        lastY = e.clientY;
        const normalizedDelta = delta / 150.0;
        let newNorm = Math.max(0, Math.min(1, state.getNormalisedValue() + normalizedDelta));
        state.setNormalisedValue(newNorm);
    });

    document.addEventListener('mouseup', () => {
        if (isDragging) {
            isDragging = false;
            knobElement.style.cursor = 'pointer';
            state.sliderDragEnded();
        }
    });

    knobElement.addEventListener('dblclick', () => {
        const def = parseFloat(knobElement.dataset.default);
        state.sliderDragStarted();
        state.setNormalisedValue(isNaN(def) ? 0.5 : scaledToNormalised(state, def));
        state.sliderDragEnded();
    });
}

// Skew-aware inverse of the parameter's NormalisableRange — a raw normalized
// value would land skewed knobs (Speed, Distance) away from their default.
function scaledToNormalised(state, scaled) {
    const p = state.properties;
    if (!p || p.end === p.start) return 0;
    const norm = Math.pow((scaled - p.start) / (p.end - p.start), p.skew);
    return Math.max(0, Math.min(1, norm));
}

function updateKnobRotation(knobElement, normalized) {
    const rotation = -135 + (normalized * 270);
    knobElement.style.transform = 'rotate(' + rotation + 'deg)';
}

function formatValue(scaled, unit) {
    let formatted;
    const abs = Math.abs(scaled);
    if (abs < 10) formatted = scaled.toFixed(2);
    else if (abs < 100) formatted = scaled.toFixed(1);
    else formatted = Math.round(scaled).toString();
    return formatted + ' ' + unit;
}

// ─── Dropdown Binding ───────────────────────────────────────────

function bindDropdown(paramId) {
    const dropdown = document.getElementById(paramId);
    if (!dropdown) return;

    const state = getComboBoxState(paramId);

    function updateFromState() {
        dropdown.selectedIndex = state.getChoiceIndex();
    }

    updateFromState();
    state.valueChangedEvent.addListener(updateFromState);
    state.propertiesChangedEvent.addListener(updateFromState);

    dropdown.addEventListener('change', () => {
        state.setChoiceIndex(dropdown.selectedIndex);
    });
}

// ─── Toggle Binding ─────────────────────────────────────────────

function bindToggle(paramId) {
    const checkbox = document.getElementById(paramId);
    if (!checkbox) return;

    const label = checkbox.nextElementSibling;
    const state = getToggleState(paramId);

    // v1.2.0: the two faces are KEYS through setLabel(), not JS literals. Two
    // calls behind an if/else and never one call with a ternary in its
    // argument — check-i18n assertion 13 rejects that shape outright, because a
    // conditional inside a localized string is where an English plural rule
    // gets smuggled into French copy.
    const setFace = (on) => {
        if (!label) return;
        if (on) setLabel(label, 'ui.on');
        else    setLabel(label, 'ui.off');
    };

    function updateFromState() {
        const val = state.getValue();
        checkbox.checked = val;
        setFace(val);
    }

    updateFromState();
    state.valueChangedEvent.addListener(updateFromState);

    checkbox.addEventListener('change', () => {
        state.setValue(checkbox.checked);
        setFace(checkbox.checked);
    });
}

// ─── Orbital Visualizer (Phase 3.2) ────────────────────────────

function initializeVisualizer() {
    const canvas = document.getElementById('visualizer-canvas');
    if (!canvas) return;

    const ctx = canvas.getContext('2d');
    canvasRef = canvas;
    canvasCtxRef = ctx;

    // High-DPI setup. setTransform, NOT scale — scale() compounds on every
    // resize call, and v1.1.0's resizable editor (D4) re-rasterizes the canvas
    // on each window resize (a canvas is a CSS replaced element: its bitmap
    // does not follow CSS size on its own).
    function resizeCanvas() {
        const dpr = window.devicePixelRatio || 1;
        const rect = canvas.parentElement.getBoundingClientRect();
        canvas.width = rect.width * dpr;
        canvas.height = rect.height * dpr;
        ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
    }
    resizeCanvas();
    window.addEventListener('resize', resizeCanvas);

    // Listen for motion updates from C++ timer (30Hz)
    window.__JUCE__.backend.addEventListener('motionUpdate', (data) => {
        if (typeof data === 'string') {
            try { motionState = JSON.parse(data); } catch (e) {}
        } else if (typeof data === 'object') {
            motionState = data;
        }
    });

    // Fetch speaker layout (and refresh when speaker_layout param changes)
    const getSpeakerLayout = getNativeFunction('getSpeakerLayout');

    window._refreshSpeakers = function() {
        getSpeakerLayout().then((result) => {
            if (Array.isArray(result)) {
                speakers = result;
            }
        });
    };

    window._refreshSpeakers();

    // Re-fetch speakers when layout parameter changes
    const layoutState = getComboBoxState('speaker_layout');
    layoutState.valueChangedEvent.addListener(() => {
        setTimeout(window._refreshSpeakers, 100);
    });

    // ─── View Toggle ────────────────────────────────────────────
    initializeViewToggle();

    // ─── Speaker Editor Interactions ────────────────────────────
    initializeEditorInteractions(canvas);

    // ─── Preset & File Buttons ──────────────────────────────────
    initializeEditorButtons();

    // ─── Downmix Badge Polling ──────────────────────────────────
    initializeDownmixBadge();

    // 60fps render loop
    function renderLoop() {
        if (viewMode === 'motion') {
            drawMotionFrame(canvas, ctx);
        } else {
            drawEditorFrame(canvas, ctx);
        }
        requestAnimationFrame(renderLoop);
    }
    requestAnimationFrame(renderLoop);
}

function drawMotionFrame(canvas, ctx) {
    const dpr = window.devicePixelRatio || 1;
    const w = canvas.width / dpr;
    const h = canvas.height / dpr;
    const centerX = w / 2;
    const centerY = h / 2;
    const radius = Math.min(w, h) * 0.42;

    // Clear
    ctx.clearRect(0, 0, w, h);

    // Background plate
    ctx.fillStyle = 'rgba(139, 115, 85, 0.06)';
    ctx.fillRect(0, 0, w, h);

    // Draw grid circles
    ctx.strokeStyle = 'rgba(139, 115, 85, 0.15)';
    ctx.lineWidth = 0.5;
    for (let i = 1; i <= 3; i++) {
        ctx.beginPath();
        ctx.arc(centerX, centerY, radius * (i / 3), 0, Math.PI * 2);
        ctx.stroke();
    }

    // Draw crosshairs
    ctx.beginPath();
    ctx.moveTo(centerX, centerY - radius);
    ctx.lineTo(centerX, centerY + radius);
    ctx.moveTo(centerX - radius, centerY);
    ctx.lineTo(centerX + radius, centerY);
    ctx.stroke();

    // "Front" label
    ctx.fillStyle = 'rgba(60, 47, 47, 0.4)';
    ctx.font = '9px Garamond, serif';
    ctx.textAlign = 'center';
    ctx.fillText('FRONT', centerX, centerY - radius - 5);

    // Draw speaker icons
    drawSpeakers(ctx, centerX, centerY, radius);

    // Convert azimuth to canvas coordinates
    // Convention: azimuth 0=front (top), positive=clockwise on screen
    // SpeakerLayout.h: 0=front, +90=left (counter-clockwise)
    // So we negate azimuth for canvas (screen clockwise = positive angle)
    function azToCanvas(azDeg, dist) {
        const azRad = (-azDeg) * Math.PI / 180;
        const r = Math.min(dist / 3, 1) * radius; // Normalize distance to radius
        return {
            x: centerX + r * Math.sin(azRad),
            y: centerY - r * Math.cos(azRad)
        };
    }

    // Update trail buffers
    const posL = azToCanvas(motionState.azL, motionState.dist);
    trailL.push({ x: posL.x, y: posL.y });
    if (trailL.length > TRAIL_LENGTH) trailL.shift();

    if (motionState.split) {
        const posR = azToCanvas(motionState.azR, motionState.dist);
        trailR.push({ x: posR.x, y: posR.y });
        if (trailR.length > TRAIL_LENGTH) trailR.shift();
    } else {
        trailR = [];
    }

    // Draw trails
    drawTrail(ctx, trailL, '#8B7355', 0.6);  // Warm brown trail for L
    if (motionState.split) {
        drawTrail(ctx, trailR, '#C9A27B', 0.5);  // Amber trail for R
    }

    // Draw source dots — scaled and brightened by elevation (D3)
    drawSourceDot(ctx, posL.x, posL.y, '#8BA870', 'L', motionState.elL);  // Green for L

    if (motionState.split) {
        const posR = azToCanvas(motionState.azR, motionState.dist);
        drawSourceDot(ctx, posR.x, posR.y, '#C9A27B', 'R', motionState.elR);  // Amber for R
    }

    // Side elevation gauge (D3)
    drawElevationMeter(ctx, 14, centerY, radius);
}

// Distance → radial display factor (shared by drawing and hit-testing so a
// speaker is grabbed exactly where it is drawn). sqrt compresses the 0.1-30 m
// range; clamps keep everything on the plate. 1.0 m maps to 1.0 (unchanged).
function distToRadial(d) {
    return Math.max(0.25, Math.min(1.15, Math.sqrt(d > 0 ? d : 1)));
}

// Height-layer cue (D3): elevated speakers get a second, dashed ring.
function strokeElevationRing(ctx, x, y, baseRadius, elevation) {
    if (Math.abs(elevation) < 0.5) return;
    ctx.save();
    ctx.beginPath();
    ctx.arc(x, y, baseRadius + 3.5, 0, Math.PI * 2);
    ctx.setLineDash([2.5, 2.5]);
    ctx.strokeStyle = elevation > 0 ? '#8BA870' : '#B0713B';
    ctx.lineWidth = 1.2;
    ctx.stroke();
    ctx.restore();
}

function drawSpeakers(ctx, cx, cy, radius) {
    for (const spk of speakers) {
        if (spk.isLFE) continue; // Don't draw LFE speakers

        const azRad = (-spk.azimuth) * Math.PI / 180;
        const r = radius * 0.95;
        const x = cx + r * Math.sin(azRad);
        const y = cy - r * Math.cos(azRad);

        // Speaker icon: small circle (+ dashed ring for height-layer speakers)
        ctx.beginPath();
        ctx.arc(x, y, 6, 0, Math.PI * 2);
        ctx.fillStyle = '#EBD9C7';
        ctx.fill();
        ctx.strokeStyle = '#8B7355';
        ctx.lineWidth = 1.5;
        ctx.stroke();

        strokeElevationRing(ctx, x, y, 6, spk.elevation);

        // Label
        ctx.fillStyle = '#5C4033';
        ctx.font = '8px Garamond, serif';
        ctx.textAlign = 'center';
        ctx.textBaseline = 'middle';
        ctx.fillText(spk.label, x, y);
    }
}

// Side elevation meter (D3): slim vertical gauge, -90° at the bottom, +90° at
// the top, with a marker per source.
function drawElevationMeter(ctx, x, cy, radius) {
    const h = radius * 1.4;
    const top = cy - h / 2;

    ctx.strokeStyle = 'rgba(139, 115, 85, 0.35)';
    ctx.lineWidth = 1;
    ctx.strokeRect(x, top, 5, h);

    // Horizon tick
    ctx.beginPath();
    ctx.moveTo(x - 2, cy);
    ctx.lineTo(x + 7, cy);
    ctx.stroke();

    ctx.fillStyle = 'rgba(60, 47, 47, 0.4)';
    ctx.font = '8px Garamond, serif';
    ctx.textAlign = 'center';
    ctx.textBaseline = 'alphabetic';
    ctx.fillText('ELEV', x + 3, top - 4);

    const markerY = (el) => cy - (Math.max(-90, Math.min(90, el)) / 90) * (h / 2);

    ctx.fillStyle = '#8BA870';
    ctx.fillRect(x - 1, markerY(motionState.elL) - 1.5, 7, 3);

    if (motionState.split) {
        ctx.fillStyle = '#C9A27B';
        ctx.fillRect(x - 1, markerY(motionState.elR) - 1.5, 7, 3);
    }
}

function drawTrail(ctx, trail, color, maxOpacity) {
    if (trail.length < 2) return;

    for (let i = 1; i < trail.length; i++) {
        const opacity = (i / trail.length) * maxOpacity;
        const lineWidth = (i / trail.length) * 2.5 + 0.5;

        ctx.beginPath();
        ctx.moveTo(trail[i - 1].x, trail[i - 1].y);
        ctx.lineTo(trail[i].x, trail[i].y);
        ctx.strokeStyle = color;
        ctx.globalAlpha = opacity;
        ctx.lineWidth = lineWidth;
        ctx.lineCap = 'round';
        ctx.stroke();
    }
    ctx.globalAlpha = 1.0;
}

function drawSourceDot(ctx, x, y, color, label, elevation) {
    // Elevation cue (D3): the dot grows and brightens as the source rises.
    const el = Math.max(-90, Math.min(90, elevation || 0));
    const scale = 1 + (el / 90) * 0.5;
    const glowR = 12 * Math.max(0.6, scale);
    const coreR = 4 * Math.max(0.6, scale);

    // Radial gradient glow
    const gradient = ctx.createRadialGradient(x, y, 0, x, y, glowR);
    gradient.addColorStop(0, color);
    gradient.addColorStop(0.5, color + '80');
    gradient.addColorStop(1, color + '00');

    ctx.beginPath();
    ctx.arc(x, y, glowR, 0, Math.PI * 2);
    ctx.fillStyle = gradient;
    ctx.fill();

    // Core dot
    ctx.beginPath();
    ctx.arc(x, y, coreR, 0, Math.PI * 2);
    ctx.fillStyle = color;
    ctx.fill();
    ctx.strokeStyle = '#FFF8DC';
    ctx.lineWidth = 1;
    ctx.stroke();

    // Brighten with height
    if (el > 2) {
        ctx.beginPath();
        ctx.arc(x, y, coreR, 0, Math.PI * 2);
        ctx.fillStyle = 'rgba(255, 248, 220, ' + (el / 90 * 0.4).toFixed(3) + ')';
        ctx.fill();
    }

    // Label below dot (only in split mode)
    if (label) {
        ctx.fillStyle = color;
        ctx.font = 'bold 8px Garamond, serif';
        ctx.textAlign = 'center';
        ctx.fillText(label, x, y + 18);
    }
}

// ─── View Toggle (Phase 3.3) ────────────────────────────────────

function initializeViewToggle() {
    const toggleBtn = document.getElementById('view-toggle');
    const toolbar = document.getElementById('editor-toolbar');
    if (!toggleBtn) return;

    // v1.2.0: both faces are KEYS through setLabel(). The button becomes a
    // [data-i18n] element on the first click and the language sweep owns it
    // from then on, so switching language while the editor is open re-renders
    // the OPEN face rather than restoring the English "Motion View".
    toggleBtn.addEventListener('click', () => {
        if (viewMode === 'motion') {
            viewMode = 'editor';
            setLabel(toggleBtn, 'label.viewEditor');
            if (toolbar) toolbar.classList.add('visible');
            // Clear motion trails when switching
            trailL = [];
            trailR = [];
        } else {
            viewMode = 'motion';
            setLabel(toggleBtn, 'label.viewMotion');
            if (toolbar) toolbar.classList.remove('visible');
        }
    });
}

// ─── Speaker Editor Drawing ─────────────────────────────────────

function drawEditorFrame(canvas, ctx) {
    const dpr = window.devicePixelRatio || 1;
    const w = canvas.width / dpr;
    const h = canvas.height / dpr;
    const centerX = w / 2;
    const centerY = (h - 32) / 2; // Account for toolbar
    const radius = Math.min(w, h - 32) * 0.40;

    ctx.clearRect(0, 0, w, h);

    // Background plate
    ctx.fillStyle = 'rgba(139, 115, 85, 0.06)';
    ctx.fillRect(0, 0, w, h);

    // Draw grid circles
    ctx.strokeStyle = 'rgba(139, 115, 85, 0.15)';
    ctx.lineWidth = 0.5;
    for (let i = 1; i <= 3; i++) {
        ctx.beginPath();
        ctx.arc(centerX, centerY, radius * (i / 3), 0, Math.PI * 2);
        ctx.stroke();
    }

    // Draw crosshairs
    ctx.beginPath();
    ctx.moveTo(centerX, centerY - radius);
    ctx.lineTo(centerX, centerY + radius);
    ctx.moveTo(centerX - radius, centerY);
    ctx.lineTo(centerX + radius, centerY);
    ctx.stroke();

    // Labels
    ctx.fillStyle = 'rgba(60, 47, 47, 0.4)';
    ctx.font = '9px Garamond, serif';
    ctx.textAlign = 'center';
    ctx.textBaseline = 'alphabetic';
    ctx.fillText('FRONT', centerX, centerY - radius - 5);
    ctx.fillText('REAR', centerX, centerY + radius + 12);

    // Draw interactive speaker icons. Radial position follows the speaker's
    // DISTANCE (D1) via the same distToRadial() the hit-test uses.
    for (let i = 0; i < speakers.length; i++) {
        const spk = speakers[i];
        if (spk.isLFE) continue;

        const azRad = (-spk.azimuth) * Math.PI / 180;
        const r = radius * 0.85 * distToRadial(spk.distance);
        const x = centerX + r * Math.sin(azRad);
        const y = centerY - r * Math.cos(azRad);

        const isHovered = (i === hoveredSpeakerIndex);
        const isDragging = (i === draggingSpeakerIndex);
        const spkRadius = isHovered || isDragging ? 10 : 8;

        // Speaker icon
        ctx.beginPath();
        ctx.arc(x, y, spkRadius, 0, Math.PI * 2);
        ctx.fillStyle = isDragging ? '#8BA870' : (isHovered ? '#D4C5A9' : '#EBD9C7');
        ctx.fill();
        ctx.strokeStyle = isDragging ? '#5C8A3E' : '#8B7355';
        ctx.lineWidth = isDragging ? 2.5 : (isHovered ? 2 : 1.5);
        ctx.stroke();

        strokeElevationRing(ctx, x, y, spkRadius, spk.elevation);

        // Elevation badge (D1/D3)
        if (Math.abs(spk.elevation) >= 0.5) {
            ctx.fillStyle = spk.elevation > 0 ? '#5C6E3E' : '#8A5A2B';
            ctx.font = '8px Garamond, serif';
            ctx.textAlign = 'center';
            ctx.textBaseline = 'alphabetic';
            ctx.fillText((spk.elevation > 0 ? '+' : '') + Math.round(spk.elevation) + '\u00b0',
                         x, y - spkRadius - 7);
        }

        // Label
        ctx.fillStyle = '#5C4033';
        ctx.font = (isHovered ? 'bold ' : '') + '9px Garamond, serif';
        ctx.textAlign = 'center';
        ctx.textBaseline = 'middle';
        ctx.fillText(spk.label, x, y);
    }

    // Per-speaker readout while hovering or dragging (D1)
    const readoutIndex = draggingSpeakerIndex >= 0 ? draggingSpeakerIndex : hoveredSpeakerIndex;
    if (readoutIndex >= 0 && readoutIndex < speakers.length) {
        const spk = speakers[readoutIndex];
        const text = spk.label + '   az ' + Math.round(spk.azimuth) + '\u00b0   el '
                   + Math.round(spk.elevation) + '\u00b0   ' + Number(spk.distance).toFixed(1) + ' m';
        ctx.font = '11px Garamond, serif';
        const tw = ctx.measureText(text).width;

        ctx.fillStyle = 'rgba(245, 230, 211, 0.9)';
        ctx.fillRect(8, 8, tw + 16, 20);
        ctx.strokeStyle = '#8B7355';
        ctx.lineWidth = 1;
        ctx.strokeRect(8, 8, tw + 16, 20);

        ctx.fillStyle = '#3C2F2F';
        ctx.textAlign = 'left';
        ctx.textBaseline = 'middle';
        ctx.fillText(text, 16, 18);
    }

    // Instructions
    ctx.fillStyle = 'rgba(60, 47, 47, 0.3)';
    ctx.font = '9px Garamond, serif';
    ctx.textAlign = 'center';
    ctx.textBaseline = 'alphabetic';
    ctx.fillText('Drag azimuth \u2022 Shift-drag elevation \u2022 Alt-drag / scroll distance \u2022 Click to add \u2022 Right-click to remove', centerX, h - 38);
}

// ─── Speaker Editor Interactions ────────────────────────────────

function initializeEditorInteractions(canvas) {
    const addSpeaker = getNativeFunction('addSpeaker');
    const removeSpeaker = getNativeFunction('removeSpeaker');
    const moveSpeaker = getNativeFunction('moveSpeaker');

    // D1 drag modes: 'az' (plain), 'el' (shift), 'dist' (alt). Chosen at
    // mousedown, held for the whole gesture.
    let dragMode = 'az';
    let dragLastY = 0;

    function getCanvasGeometry() {
        const dpr = window.devicePixelRatio || 1;
        const w = canvas.width / dpr;
        const h = canvas.height / dpr;
        const centerX = w / 2;
        const centerY = (h - 32) / 2;
        const radius = Math.min(w, h - 32) * 0.40;
        return { centerX, centerY, radius };
    }

    function canvasToAzimuth(mouseX, mouseY) {
        const { centerX, centerY } = getCanvasGeometry();
        const dx = mouseX - centerX;
        const dy = centerY - mouseY;
        // atan2 gives angle from positive Y axis, negate for our convention
        let az = -Math.atan2(dx, dy) * 180 / Math.PI;
        return az;
    }

    function findSpeakerAt(mouseX, mouseY) {
        const { centerX, centerY, radius } = getCanvasGeometry();
        let closest = -1;
        let closestDist = 15; // Max pixel distance to register hit

        for (let i = 0; i < speakers.length; i++) {
            if (speakers[i].isLFE) continue;
            const azRad = (-speakers[i].azimuth) * Math.PI / 180;
            // Same distance→radial mapping the draw uses, so a speaker is
            // grabbed exactly where it is drawn (D1)
            const r = radius * 0.85 * distToRadial(speakers[i].distance);
            const x = centerX + r * Math.sin(azRad);
            const y = centerY - r * Math.cos(azRad);
            const dist = Math.sqrt((mouseX - x) ** 2 + (mouseY - y) ** 2);
            if (dist < closestDist) {
                closestDist = dist;
                closest = i;
            }
        }
        return closest;
    }

    function getMousePos(e) {
        const rect = canvas.getBoundingClientRect();
        return { x: e.clientX - rect.left, y: e.clientY - rect.top };
    }

    // Push a speaker's full position to C++ and locally mirror it so the
    // 60fps draw doesn't lag behind the 50ms refresh round-trip.
    function pushSpeaker(index, az, el, dist) {
        const spk = speakers[index];
        spk.azimuth = az;
        spk.elevation = el;
        spk.distance = dist;
        moveSpeaker(index, az, el, dist).then(() => {
            setTimeout(window._refreshSpeakers, 50);
        });
    }

    canvas.addEventListener('mousemove', (e) => {
        if (viewMode !== 'editor') return;
        const pos = getMousePos(e);

        if (draggingSpeakerIndex >= 0) {
            const spk = speakers[draggingSpeakerIndex];
            const dy = dragLastY - e.clientY;
            dragLastY = e.clientY;

            if (dragMode === 'el') {
                // Shift-drag: 2px per degree of elevation
                const el = Math.max(-90, Math.min(90, spk.elevation + dy * 0.5));
                pushSpeaker(draggingSpeakerIndex, spk.azimuth, el, spk.distance);
            } else if (dragMode === 'dist') {
                // Alt-drag: multiplicative, up = farther
                const dist = Math.max(0.1, Math.min(30, spk.distance * Math.exp(dy * 0.01)));
                pushSpeaker(draggingSpeakerIndex, spk.azimuth, spk.elevation, dist);
            } else {
                const az = canvasToAzimuth(pos.x, pos.y);
                pushSpeaker(draggingSpeakerIndex, az, spk.elevation, spk.distance);
            }
            return;
        }

        hoveredSpeakerIndex = findSpeakerAt(pos.x, pos.y);
        canvas.style.cursor = hoveredSpeakerIndex >= 0 ? 'grab' : 'crosshair';
    });

    canvas.addEventListener('mousedown', (e) => {
        if (viewMode !== 'editor') return;
        if (e.button !== 0) return; // Left click only
        const pos = getMousePos(e);
        const hit = findSpeakerAt(pos.x, pos.y);

        if (hit >= 0) {
            draggingSpeakerIndex = hit;
            dragMode = e.shiftKey ? 'el' : (e.altKey ? 'dist' : 'az');
            dragLastY = e.clientY;
            canvas.style.cursor = 'grabbing';
            e.preventDefault();
        }
    });

    // Scroll over a speaker: edit its distance (D1)
    canvas.addEventListener('wheel', (e) => {
        if (viewMode !== 'editor') return;
        const pos = getMousePos(e);
        const hit = draggingSpeakerIndex >= 0 ? draggingSpeakerIndex : findSpeakerAt(pos.x, pos.y);
        if (hit < 0) return;
        e.preventDefault();

        const spk = speakers[hit];
        const dist = Math.max(0.1, Math.min(30, spk.distance * Math.exp(-e.deltaY * 0.002)));
        pushSpeaker(hit, spk.azimuth, spk.elevation, dist);
    }, { passive: false });

    canvas.addEventListener('mouseup', (e) => {
        if (viewMode !== 'editor') return;
        if (e.button !== 0) return;

        if (draggingSpeakerIndex >= 0) {
            draggingSpeakerIndex = -1;
            canvas.style.cursor = hoveredSpeakerIndex >= 0 ? 'grab' : 'crosshair';
            return;
        }

        // Click on empty space: add speaker
        const pos = getMousePos(e);
        const hit = findSpeakerAt(pos.x, pos.y);
        if (hit < 0) {
            const az = canvasToAzimuth(pos.x, pos.y);
            const label = 'S' + (speakers.length + 1);
            addSpeaker(az, 0, 1, label).then(() => {
                setTimeout(window._refreshSpeakers, 100);
            });
        }
    });

    canvas.addEventListener('contextmenu', (e) => {
        if (viewMode !== 'editor') return;
        e.preventDefault();
        const pos = getMousePos(e);
        const hit = findSpeakerAt(pos.x, pos.y);
        if (hit >= 0) {
            removeSpeaker(hit).then(() => {
                setTimeout(window._refreshSpeakers, 100);
            });
        }
    });

    canvas.addEventListener('mouseleave', () => {
        hoveredSpeakerIndex = -1;
        draggingSpeakerIndex = -1;
    });
}

// ─── Preset & File Buttons ──────────────────────────────────────

function initializeEditorButtons() {
    const layoutState = getComboBoxState('speaker_layout');
    const exportLayout = getNativeFunction('exportLayout');
    const importLayout = getNativeFunction('importLayout');

    // Preset buttons
    document.querySelectorAll('.preset-btn').forEach(btn => {
        btn.addEventListener('click', () => {
            const preset = parseInt(btn.dataset.preset, 10);
            layoutState.setChoiceIndex(preset);
            setTimeout(window._refreshSpeakers, 150);
        });
    });

    // Export button
    const exportBtn = document.getElementById('export-btn');
    if (exportBtn) {
        exportBtn.addEventListener('click', () => {
            exportLayout();
        });
    }

    // Import button
    const importBtn = document.getElementById('import-btn');
    if (importBtn) {
        importBtn.addEventListener('click', () => {
            importLayout().then((result) => {
                if (Array.isArray(result)) {
                    speakers = result;
                } else {
                    setTimeout(window._refreshSpeakers, 100);
                }
            });
        });
    }

    // ─── Named layout library (D2, v1.1.0) ──────────────────────
    const layoutSelect = document.getElementById('layout-select');
    const layoutNameInput = document.getElementById('layout-name');
    const layoutSaveBtn = document.getElementById('layout-save-btn');
    const layoutDeleteBtn = document.getElementById('layout-delete-btn');

    if (layoutSelect && layoutNameInput && layoutSaveBtn && layoutDeleteBtn) {
        const getLayoutList = getNativeFunction('getLayoutList');
        const saveLayoutNamed = getNativeFunction('saveLayoutNamed');
        const loadLayoutNamed = getNativeFunction('loadLayoutNamed');
        const deleteLayoutNamed = getNativeFunction('deleteLayoutNamed');

        function refreshLayoutList(selectName) {
            getLayoutList().then((list) => {
                if (!Array.isArray(list)) return;
                // First option is the HTML-authored placeholder — keep it.
                while (layoutSelect.options.length > 1) layoutSelect.remove(1);
                for (const name of list) {
                    const opt = document.createElement('option');
                    opt.value = name;
                    opt.textContent = name;
                    layoutSelect.appendChild(opt);
                }
                layoutSelect.value = selectName && list.includes(selectName) ? selectName : '';
            });
        }
        refreshLayoutList();

        layoutSelect.addEventListener('change', () => {
            const name = layoutSelect.value;
            if (!name) return;
            loadLayoutNamed(name).then((result) => {
                if (Array.isArray(result)) {
                    speakers = result;
                } else {
                    setTimeout(window._refreshSpeakers, 100);
                }
            });
        });

        layoutSaveBtn.addEventListener('click', () => {
            const name = layoutNameInput.value.trim();
            if (!name) { layoutNameInput.focus(); return; }
            saveLayoutNamed(name).then((ok) => {
                if (ok) {
                    layoutNameInput.value = '';
                    refreshLayoutList(name.replace(/[/\\:]/g, '_'));
                }
            });
        });

        // Two-click armed delete. v1.2.0: the two faces are KEYS through
        // setLabel(), not the data-label / data-confirm ATTRIBUTES they were
        // through v1.1.1. Those attributes were the right answer while the page
        // was English-only — they kept the copy out of this file, which is what
        // pattern_js_state_updater_overwrites_html_labels asks for. They are the
        // wrong answer once the page has two languages: an attribute holds ONE
        // string, so a language switch while the button was armed would have
        // restored the ENGLISH armed face. A key re-renders with the sweep.
        let layoutDeleteTimer = null;
        layoutDeleteBtn.addEventListener('click', () => {
            const name = layoutSelect.value;
            if (!name) return;

            const disarm = () => {
                if (layoutDeleteTimer) { clearTimeout(layoutDeleteTimer); layoutDeleteTimer = null; }
                layoutDeleteBtn.dataset.armed = '0';
                setLabel(layoutDeleteBtn, 'label.delete');
            };

            if (layoutDeleteBtn.dataset.armed === '1') {
                disarm();
                deleteLayoutNamed(name).then(() => refreshLayoutList());
                return;
            }
            layoutDeleteBtn.dataset.armed = '1';
            setLabel(layoutDeleteBtn, 'ui.confirm');
            layoutDeleteTimer = setTimeout(disarm, 2500);
        });
    }
}

// ─── Downmix Badge ──────────────────────────────────────────────

function initializeDownmixBadge() {
    const getDownmixStatus = getNativeFunction('getDownmixStatus');
    const badge = document.getElementById('downmix-badge');
    if (!badge) return;

    const layoutNames = ['Stereo', 'Quad', '5.1', '7.1', '5.1.4', '7.1.4', 'Hex', 'Oct'];

    // v1.2.0: a COMPOSED entry, and the only one on this page. The two channel
    // counts stay NUMBERS (D-03 \u2014 no readout is localized); "ch" is not a unit
    // symbol like Hz or dB, it is an abbreviation of the WORD "channels", so it
    // localizes. Passing the counts as vars rather than baking them into the
    // string is what lets the language sweep re-render this badge with the SAME
    // counts instead of a stale English face: setLabel() writes them onto the
    // element as data-i18n-vars and applyLabel() reads them back every pass.
    function updateBadge() {
        getDownmixStatus().then((status) => {
            if (status && status.active) {
                setLabel(badge, 'ui.downmix',
                         { from: status.sourceChannels, to: status.targetChannels });
                badge.classList.add('active');
            } else {
                badge.classList.remove('active');
            }
        });
    }

    // Poll every 2 seconds
    updateBadge();
    setInterval(updateBadge, 2000);
}

// ─── Preset Band (B1, v1.1.0) — modules/preset-manager.js ───────
// Constructor + explicit DOM refs (never createPresetBar(), which
// innerHTML-wipes and has no delete button). The categorized MENU is a view
// over the module's state: selection always goes through
// presetManager.loadPreset(), so the arrows and the menu cannot disagree.

function initializePresetBand() {
    let deleteArmTimer = null;

    // Two-click armed delete — window.confirm() is unreliable (silent no-op
    // or throw) in some JUCE WebView backends. v1.2.0: both faces are KEYS
    // through setLabel(), for the same reason the layout-library delete button
    // above changed — an attribute holds one string and cannot follow a
    // language switch that happens while the button is armed.
    const armedConfirmDelete = () => {
        const btn = document.getElementById('preset-delete');
        if (!btn) return false;

        const disarm = () => {
            if (deleteArmTimer) { clearTimeout(deleteArmTimer); deleteArmTimer = null; }
            btn.dataset.armed = '0';
            setLabel(btn, 'label.delete');
        };

        if (btn.dataset.armed === '1') {
            disarm();
            return true;          // second click within the window — confirmed
        }

        btn.dataset.armed = '1';
        setLabel(btn, 'ui.confirm');
        deleteArmTimer = setTimeout(disarm, 2500);
        return false;             // first click — armed only
    };

    const byId = (id) => document.getElementById(id);
    const els = ['preset-prev', 'preset-next', 'preset-name',
                 'preset-save', 'preset-load', 'preset-delete',
                 'preset-select', 'preset-menu'].map(byId);

    if (els.some((el) => !el)) {
        console.error('Missing preset-band elements — preset band disabled');
        return;
    }

    const [prevEl, nextEl, nameEl, saveEl, loadEl, deleteEl,
           selectEl, menuEl] = els;

    let sections = [];          // [{ category, presets: [...] }] from C++
    let presetWalkOrder = [];   // the menu flattened — what ◀ / ▶ step
    let menuOpen = false;

    // A gap between this name and the C++ registration fails SILENTLY
    // (pattern_webview_native_fn_bridge_gap) — a throw here is logged, and
    // the band still works without the menu.
    let getGrouped = null;
    try {
        getGrouped = getNativeFunction('getPresetListGrouped');
    } catch (e) {
        console.error('[preset-menu] getPresetListGrouped unavailable:', e);
    }

    const markActive = () => {
        const current = presetManager.getCurrentPreset();
        menuEl.querySelectorAll('.preset-menu-item').forEach((el) => {
            const isCurrent = el.dataset.name === current;
            el.classList.toggle('active', isCurrent);
            el.setAttribute('aria-selected', isCurrent ? 'true' : 'false');
        });
    };

    const buildMenu = () => {
        menuEl.replaceChildren();
        for (const section of sections) {
            if (!section || !Array.isArray(section.presets)) continue;

            const header = document.createElement('div');
            header.className = 'preset-menu-category';
            header.textContent = section.category;
            menuEl.appendChild(header);

            const grid = document.createElement('div');
            grid.className = 'preset-menu-grid';
            for (const name of section.presets) {
                const item = document.createElement('div');
                item.className = 'preset-menu-item';
                item.setAttribute('role', 'option');
                item.dataset.name = name;
                item.textContent = name;
                item.addEventListener('click', (e) => {
                    e.stopPropagation();
                    closeMenu();
                    // No local state written here: loadPreset() drives
                    // onPresetChanged, which repaints readout and highlight.
                    presetManager.loadPreset(name);
                });
                grid.appendChild(item);
            }
            menuEl.appendChild(grid);
        }
        markActive();
    };

    const refreshMenu = async () => {
        if (!getGrouped) return;
        try {
            const result = await getGrouped();
            sections = Array.isArray(result) ? result : [];
            // The walk order IS the menu, flattened — derived, never fetched
            // separately, so the two cannot describe different orders.
            presetWalkOrder = sections.flatMap(
                (s) => (s && Array.isArray(s.presets) ? s.presets : []));
            buildMenu();
        } catch (e) {
            console.error('[preset-menu] getPresetListGrouped failed:', e);
        }
    };

    // ◀ / ▶ step through the MENU order, not the alphabet. The module's
    // prevButton/nextButton options are deliberately NOT passed below: they
    // walk the flat alphabetical getPresetList(), which disagrees with a
    // grouped menu (pattern_grouping_preset_dropdown_breaks_prev_next).
    const stepPreset = async (delta) => {
        if (presetWalkOrder.length === 0) return;
        const index = presetWalkOrder.indexOf(presetManager.getCurrentPreset());
        // Out-of-list current preset ("Default", or file-loaded) has no
        // neighbour — enter at the top going forward, bottom going back.
        const base = index >= 0 ? index : (delta > 0 ? -1 : 0);
        const next = (base + delta + presetWalkOrder.length) % presetWalkOrder.length;
        await presetManager.loadPreset(presetWalkOrder[next]);
    };

    prevEl.addEventListener('click', () => { stepPreset(-1); });
    nextEl.addEventListener('click', () => { stepPreset(1); });

    const openMenu = () => {
        if (menuOpen) return;
        menuOpen = true;
        markActive();
        menuEl.classList.add('visible');
        selectEl.setAttribute('aria-expanded', 'true');
        const active = menuEl.querySelector('.preset-menu-item.active');
        if (active) {
            menuEl.scrollTop = Math.max(
                0, active.offsetTop - (menuEl.clientHeight / 2));
        }
    };

    const closeMenu = () => {
        if (!menuOpen) return;
        menuOpen = false;
        menuEl.classList.remove('visible');
        selectEl.setAttribute('aria-expanded', 'false');
    };

    selectEl.addEventListener('click', (e) => {
        e.stopPropagation();    // else the document handler closes it again
        if (menuOpen) closeMenu(); else openMenu();
    });
    document.addEventListener('click', closeMenu);
    document.addEventListener('keydown', (e) => {
        if (e.key === 'Escape') closeMenu();
    });

    const presetManager = new PresetManager({
        displayElement: nameEl,  // stays childless — _updateDisplay writes textContent
        // prevButton / nextButton intentionally omitted — see stepPreset.
        saveButton: saveEl,
        loadButton: loadEl,
        deleteButton: deleteEl,
        getNativeFunction: getNativeFunction,
        onConfirmDelete: () => armedConfirmDelete(),
        onPresetChanged: () => markActive(),
        onPresetListUpdated: () => { refreshMenu(); },
    });

    // The band ships disabled in the markup; un-disable only after
    // initialize() resolves — if the bridge never comes up, the buttons stay
    // honestly disabled instead of enabled-but-inert.
    presetManager.initialize().then(() => {
        [prevEl, nextEl, saveEl, loadEl, deleteEl, selectEl]
            .forEach((el) => { el.disabled = false; });
    });
}

// ─── Hover Help (B2, v1.1.0) ────────────────────────────────────
// Measure-then-pin tooltip placement (pattern_fixed_tooltip_shrink_to_fit_edge):
// a fixed-position box with `left` set and width:auto shrink-to-fits the space
// to its right, so measure at left:0, pin the width in px, THEN place.
// Every show is gated on the "?" toggle's state, which round-trips through the
// processor. The toggle's own tip carries data-tip-always.

function initializeHoverHelp() {
    const TOOLTIP_MARGIN = 8;       // gap to control AND to the viewport edge
    const TOOLTIP_DELAY_MS = 350;   // hover dwell before a tip appears

    const tipEl = document.getElementById('tooltip');
    const toggleEl = document.getElementById('help-toggle');

    let tipTimer = null;
    let tipTarget = null;
    let tipSuppressed = false;      // true between pointerdown and pointerup
    let tipsEnabled = false;        // the "?" state; PULLED from C++ at init
    let setTipsFn = null;

    const tipAllowed = (t) => tipsEnabled || t.hasAttribute('data-tip-always');

    const hideTip = () => {
        clearTimeout(tipTimer);
        tipTarget = null;
        if (!tipEl) return;
        tipEl.classList.remove('visible');
        tipEl.setAttribute('aria-hidden', 'true');
    };

    const showTip = (target) => {
        if (!tipEl || tipSuppressed || target !== tipTarget) return;
        if (!tipAllowed(target)) return;

        const title = target.getAttribute('data-tip-title');
        const body = target.getAttribute('data-tip');
        if (!body) return;

        // textContent, not innerHTML — the copy stays inert.
        tipEl.textContent = '';
        if (title) {
            const t = document.createElement('div');
            t.className = 'tooltip-title';
            t.textContent = title;
            tipEl.appendChild(t);
        }
        const b = document.createElement('div');
        b.className = 'tooltip-body';
        b.textContent = body;
        tipEl.appendChild(b);

        const anchor = target.getBoundingClientRect();

        // MEASURE-THEN-PIN: release width, measure from the left edge, pin in
        // px, only then place. getBoundingClientRect, not offsetWidth —
        // offsetWidth rounds down and re-wraps the last word.
        tipEl.style.width = '';
        tipEl.style.left = '0px';
        tipEl.style.top = '0px';

        const width = tipEl.getBoundingClientRect().width;
        tipEl.style.width = width + 'px';

        // Height is only stable once the width is definite.
        const height = tipEl.getBoundingClientRect().height;

        // Prefer above; flip below only when there is no room at the top.
        let top = anchor.top - height - TOOLTIP_MARGIN;
        let placement = 'above';
        if (top < TOOLTIP_MARGIN) {
            top = anchor.bottom + TOOLTIP_MARGIN;
            placement = 'below';
        }

        const centreX = anchor.left + anchor.width / 2;
        const maxLeft = window.innerWidth - width - TOOLTIP_MARGIN;
        const left = Math.max(TOOLTIP_MARGIN, Math.min(maxLeft, centreX - width / 2));

        tipEl.style.left = left + 'px';
        tipEl.style.top = top + 'px';
        tipEl.dataset.placement = placement;

        // The tip is clamped to the viewport, but the arrow still points at
        // the control — held clear of the rounded corners.
        const arrowX = Math.max(10, Math.min(width - 10, centreX - left));
        tipEl.style.setProperty('--arrow-x', arrowX + 'px');

        tipEl.classList.add('visible');
        tipEl.setAttribute('aria-hidden', 'false');
    };

    // v1.2.0: the toggle MOVED into the settings popover and its face is now a
    // WORD rather than the "?" glyph it wore in the header, so this function
    // writes the caption as well as the class and the aria state. Through
    // v1.1.1 it deliberately wrote neither, because the glyph was HTML-authored
    // and a JS write would have erased it
    // (pattern_js_state_updater_overwrites_html_labels). It goes through
    // setLabel(), which is the sanctioned way to write a caption: the element
    // becomes a [data-i18n] element and the language sweep owns it, so
    // switching to French while the help is ON re-renders the ON face.
    //
    // Two calls behind an if/else, never a ternary in the argument
    // (check-i18n assertion 13).
    const applyTipsEnabled = (enabled) => {
        tipsEnabled = !!enabled;
        if (toggleEl) {
            toggleEl.classList.toggle('active', tipsEnabled);
            toggleEl.setAttribute('aria-pressed', tipsEnabled ? 'true' : 'false');
            if (tipsEnabled) setLabel(toggleEl, 'ui.on');
            else             setLabel(toggleEl, 'ui.off');
        }
        if (!tipsEnabled) hideTip();
    };

    if (!tipEl) {
        console.warn('Tooltip surface missing — hover help disabled');
        return;
    }

    document.addEventListener('mouseover', (e) => {
        const target = e.target.closest ? e.target.closest('[data-tip]') : null;
        if (!target || target === tipTarget) return;
        tipTarget = target;
        clearTimeout(tipTimer);
        if (tipSuppressed || !tipAllowed(target)) return;
        tipTimer = setTimeout(() => showTip(target), TOOLTIP_DELAY_MS);
    });

    document.addEventListener('mouseout', (e) => {
        const target = e.target.closest ? e.target.closest('[data-tip]') : null;
        if (!target) return;
        // Moving between children of the same control is not a real exit.
        if (e.relatedTarget && target.contains(e.relatedTarget)) return;
        hideTip();
    });

    // Any press begins a click or a drag: hide the tip and keep it away until
    // release. CAPTURE phase — the knobs preventDefault in their own handlers.
    document.addEventListener('pointerdown', () => {
        tipSuppressed = true;
        hideTip();
    }, true);
    document.addEventListener('pointerup', () => { tipSuppressed = false; }, true);

    if (toggleEl) {
        toggleEl.addEventListener('click', () => {
            applyTipsEnabled(!tipsEnabled);
            // Fire-and-forget: the page is already in the new state.
            if (setTipsFn) setTipsFn(tipsEnabled).catch(() => {});
        });
    }

    // PULL the persisted preference — the C++ side deliberately never pushes
    // it (a push would fire before this module evaluates and silently never
    // arrive: the O-FreqPulse WR-01 bug).
    try {
        setTipsFn = getNativeFunction('setTooltipsEnabled');
        getNativeFunction('getTooltipsEnabled')()
            .then((enabled) => applyTipsEnabled(!!enabled))
            .catch((e) => console.error('[help] getTooltipsEnabled failed:', e));
    } catch (e) {
        console.error('[help] native bridge unavailable:', e);
    }
}
