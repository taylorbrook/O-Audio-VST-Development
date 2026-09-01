/*
   This file is part of O-Texture, an Ouaricon Audio plugin.
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
    O-Texture - Main UI Logic
    Parameter binding, XY pad, animations, controls
  ==============================================================================
*/

import { getSliderState, getToggleState, getComboBoxState } from './juce/index.js';
// The JUCE 8 frontend NAMESPACE, imported alongside the three named bindings
// this file already used. Juce.getNativeFunction is the only correct way to
// reach a withNativeFunction registration — window.__JUCE__ is the raw
// postMessage transport and does not carry the promise contract
// (critical_juce_webview_namespace_vs_postmessage). Two import statements from
// one specifier is legal ES and leaves the existing call sites untouched.
import * as Juce from './juce/index.js';

// v0.2.0: the label table. The CANONICAL import line verbatim — this
// controller is a real js/app.js-shaped module living beside i18n.js, so the
// specifier needs no re-rooting. I18N and TIP_BINDINGS were both EMPTY at
// v0.2.0 and were imported anyway so the canon block at the foot of this file
// stayed byte-identical to the other forty-two copies; v0.3.0 fills both, and
// that foresight is why adding hover-help changed no line of the canon.
import { LANGUAGES, I18N, LABELS, TIP_BINDINGS, tr } from './i18n.js';

// ============================================================================
// State Management
// ============================================================================
let xState, yState, characterAState, characterBState, evolveState;
let brightnessState, mixState, sourceState, modeState, freezeState;

// XY pad animation
const canvas = document.getElementById('xy-pad');
const ctx = canvas ? canvas.getContext('2d') : null;
let canvasW = 0, canvasH = 0; // logical (CSS pixel) drawing size
const trailPoints = [];
const MAX_TRAIL_LENGTH = 60; // ~2 seconds at 30fps
let lastFrameTime = 0;
const FRAME_INTERVAL = 1000 / 30; // 30fps cap
let isDraggingXY = false;

// ============================================================================
// Initialize after DOM loads
// ============================================================================
document.addEventListener('DOMContentLoaded', () => {
    // Setup canvas — a ResizeObserver keeps the backing store in sync with
    // layout (covers the stylesheet settling after DOMContentLoaded in a cold
    // WebView, and any future resizable-editor work).
    if (canvas) {
        resizeCanvas();
        new ResizeObserver(resizeCanvas).observe(canvas);
    }

    // Initialize all relay states
    initializeRelays();

    // Bind UI controls
    bindModeToggle();
    bindXYPad();
    bindVerticalSliders();
    bindSourceSelector();
    bindKnobs();
    bindFreezeToggle();

    // Start animation loop
    if (canvas) {
        requestAnimationFrame(animationLoop);
    }

    // v0.2.0: i18n LAST, and guarded. Every relay and every control binding
    // above has already run, so a failure here leaves an English plugin that
    // still works rather than a dead page. The canon block lives at the FOOT of
    // this module: its `let uiLanguage` is a top-level binding, and module
    // evaluation completes before DOMContentLoaded fires on a deferred module
    // script, so there is no TDZ window for this call to fall into
    // (pattern_module_toplevel_init_tdz).
    //
    // v0.3.0: setupTooltips() joins it INSIDE the same try/catch and AFTER
    // initI18n(), deliberately on both counts. Before initI18n() every anchor
    // is bare — applyI18n() is what writes data-tip-title and data-tip onto
    // them — so a renderer wired up first would open an empty box on every
    // hover. And a throw out of either must not reach module scope.
    initializeSettingsPopover();
    try { initI18n(); setupTooltips(); } catch (e) { console.error('i18n init failed:', e); }
});

// ============================================================================
// Relay Initialization
// ============================================================================
function initializeRelays() {
    // 7 WebSliderRelays
    xState = getSliderState('xSlider');
    yState = getSliderState('ySlider');
    characterAState = getSliderState('characterASlider');
    characterBState = getSliderState('characterBSlider');
    evolveState = getSliderState('evolveSlider');
    brightnessState = getSliderState('brightnessSlider');
    mixState = getSliderState('mixSlider');

    // 2 WebComboBoxRelays
    sourceState = getComboBoxState('sourceCombo');
    modeState = getComboBoxState('modeCombo');

    // 1 WebToggleButtonRelay
    freezeState = getToggleState('freezeToggle');

    // Listen for backend property updates (initial state)
    sourceState.propertiesChangedEvent.addListener(() => updateSourceButtons());
    modeState.propertiesChangedEvent.addListener(() => updateModeButtons());

    // Listen for backend value changes (automation, presets)
    xState.valueChangedEvent.addListener(() => updateXYPadVisual());
    yState.valueChangedEvent.addListener(() => updateXYPadVisual());
    characterAState.valueChangedEvent.addListener(() => updateVerticalSlider('charA'));
    characterBState.valueChangedEvent.addListener(() => updateVerticalSlider('charB'));
    evolveState.valueChangedEvent.addListener(() => updateVerticalSlider('evolve'));
    brightnessState.valueChangedEvent.addListener(() => updateKnob('brightness'));
    mixState.valueChangedEvent.addListener(() => updateKnob('mix'));
    sourceState.valueChangedEvent.addListener(() => updateSourceButtons());
    modeState.valueChangedEvent.addListener(() => updateModeButtons());
    freezeState.valueChangedEvent.addListener(() => updateFreezeVisual());
}

// ============================================================================
// Mode Toggle (Generate | Transform)
// ============================================================================
function bindModeToggle() {
    const buttons = document.querySelectorAll('.mode-toggle button');
    buttons.forEach((btn, index) => {
        btn.addEventListener('click', () => {
            modeState.setChoiceIndex(index);
        });
    });
}

function updateModeButtons() {
    const buttons = document.querySelectorAll('.mode-toggle button');
    const currentIndex = modeState.getChoiceIndex();
    buttons.forEach((btn, index) => {
        if (index === currentIndex) {
            btn.classList.add('active');
        } else {
            btn.classList.remove('active');
        }
    });
}

// ============================================================================
// XY Pad with Orbital Trails
// ============================================================================
function resizeCanvas() {
    // Canvas is a CSS replaced element: CSS sets its display size, but the
    // backing store must be sized explicitly (DPR-aware for crisp Retina).
    const w = canvas.clientWidth;
    const h = canvas.clientHeight;
    if (w <= 0 || h <= 0) return;

    const dpr = window.devicePixelRatio || 1;
    canvasW = w;
    canvasH = h;
    canvas.width = Math.round(w * dpr);
    canvas.height = Math.round(h * dpr);
    ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
}

function bindXYPad() {
    if (!canvas) return;

    canvas.addEventListener('pointerdown', (e) => {
        isDraggingXY = true;
        canvas.setPointerCapture(e.pointerId);
        xState.sliderDragStarted();
        yState.sliderDragStarted();
        updateXYFromPointer(e);
    });

    canvas.addEventListener('pointermove', (e) => {
        if (!isDraggingXY) return;
        updateXYFromPointer(e);
    });

    canvas.addEventListener('pointerup', (e) => {
        if (!isDraggingXY) return;
        isDraggingXY = false;
        xState.sliderDragEnded();
        yState.sliderDragEnded();
    });

    canvas.addEventListener('pointercancel', () => {
        if (isDraggingXY) {
            isDraggingXY = false;
            xState.sliderDragEnded();
            yState.sliderDragEnded();
        }
    });
}

function updateXYFromPointer(e) {
    const rect = canvas.getBoundingClientRect();
    const normX = Math.max(0, Math.min(1, (e.clientX - rect.left) / rect.width));
    const normY = Math.max(0, Math.min(1, 1.0 - (e.clientY - rect.top) / rect.height)); // Y inverted

    xState.setNormalisedValue(normX);
    yState.setNormalisedValue(normY);
}

function updateXYPadVisual() {
    // Visual update happens in animation loop
}

function animationLoop(timestamp) {
    if (timestamp - lastFrameTime >= FRAME_INTERVAL) {
        lastFrameTime = timestamp;

        const normX = xState.getNormalisedValue();
        const normY = yState.getNormalisedValue();

        // Add trail point if not frozen
        if (!freezeState.getValue()) {
            trailPoints.push({ x: normX, y: normY, age: 0 });
            if (trailPoints.length > MAX_TRAIL_LENGTH) {
                trailPoints.shift();
            }

            // Age all points
            for (const p of trailPoints) {
                p.age++;
            }
        }

        drawXYPad(normX, normY);
    }
    requestAnimationFrame(animationLoop);
}

function drawXYPad(normX, normY) {
    if (!ctx) return;

    const w = canvasW;
    const h = canvasH;

    // Clear
    ctx.clearRect(0, 0, w, h);

    // Draw trail
    for (let i = 0; i < trailPoints.length; i++) {
        const p = trailPoints[i];
        const alpha = 1.0 - (p.age / MAX_TRAIL_LENGTH);
        const radius = 2 + alpha * 3;

        ctx.beginPath();
        ctx.arc(p.x * w, (1 - p.y) * h, radius, 0, Math.PI * 2);
        ctx.fillStyle = `rgba(107, 142, 78, ${alpha * 0.8})`; // botanical green
        ctx.fill();
    }

    // Draw current cursor
    ctx.beginPath();
    ctx.arc(normX * w, (1 - normY) * h, 7, 0, Math.PI * 2);
    ctx.fillStyle = '#6B8E4E';
    ctx.fill();
    ctx.strokeStyle = '#4A6B35';
    ctx.lineWidth = 2;
    ctx.stroke();
}

// ============================================================================
// Vertical Sliders (Character A, B, Evolve)
// ============================================================================
function bindVerticalSliders() {
    // Normalized defaults must match createParameterLayout() in PluginProcessor.cpp
    bindVerticalSlider('charA', characterAState, 0.5);
    bindVerticalSlider('charB', characterBState, 0.5);
    bindVerticalSlider('evolve', evolveState, 0.3);
}

function bindVerticalSlider(id, sliderState, defaultNorm) {
    const slider = document.getElementById(`slider-${id}`);
    if (!slider) return;

    const track = slider.querySelector('.slider-track');
    let isDragging = false;
    let startY, startValue;

    track.addEventListener('pointerdown', (e) => {
        isDragging = true;
        startY = e.clientY;
        startValue = sliderState.getNormalisedValue();
        track.setPointerCapture(e.pointerId);
        sliderState.sliderDragStarted();
        e.preventDefault();
    });

    track.addEventListener('pointermove', (e) => {
        if (!isDragging) return;
        const deltaY = startY - e.clientY; // Up = positive
        const sensitivity = 1.0 / track.clientHeight;
        const newValue = Math.max(0, Math.min(1, startValue + deltaY * sensitivity));
        sliderState.setNormalisedValue(newValue);
    });

    track.addEventListener('pointerup', () => {
        if (!isDragging) return;
        isDragging = false;
        sliderState.sliderDragEnded();
    });

    track.addEventListener('pointercancel', () => {
        if (isDragging) {
            isDragging = false;
            sliderState.sliderDragEnded();
        }
    });

    // Double-click to reset to the parameter's default
    track.addEventListener('dblclick', () => {
        sliderState.sliderDragStarted();
        sliderState.setNormalisedValue(defaultNorm);
        sliderState.sliderDragEnded();
    });

    // Initial position
    updateVerticalSlider(id);
}

function updateVerticalSlider(id) {
    const slider = document.getElementById(`slider-${id}`);
    if (!slider) return;

    const thumb = slider.querySelector('.slider-thumb');
    const valueDisplay = slider.querySelector('.value');
    let sliderState;

    if (id === 'charA') sliderState = characterAState;
    else if (id === 'charB') sliderState = characterBState;
    else if (id === 'evolve') sliderState = evolveState;

    if (!sliderState) return;

    const normValue = sliderState.getNormalisedValue();
    const track = slider.querySelector('.slider-track');
    const thumbHeight = thumb.offsetHeight;
    const trackHeight = track.clientHeight;
    const thumbY = (1 - normValue) * (trackHeight - thumbHeight);

    thumb.style.top = `${thumbY}px`;

    if (valueDisplay) {
        const scaledValue = sliderState.getScaledValue();
        valueDisplay.textContent = scaledValue.toFixed(2);
    }
}

// ============================================================================
// Source Selector (6 icon buttons)
// ============================================================================
function bindSourceSelector() {
    const buttons = document.querySelectorAll('.source-button');
    buttons.forEach((btn, index) => {
        btn.addEventListener('click', () => {
            sourceState.setChoiceIndex(index);
        });
    });
}

function updateSourceButtons() {
    const buttons = document.querySelectorAll('.source-button');
    const currentIndex = sourceState.getChoiceIndex();
    buttons.forEach((btn, index) => {
        if (index === currentIndex) {
            btn.classList.add('active');
        } else {
            btn.classList.remove('active');
        }
    });
}

// ============================================================================
// Rotary Knobs (Brightness, Mix)
// ============================================================================
function bindKnobs() {
    // Normalized defaults must match createParameterLayout() in PluginProcessor.cpp
    bindKnob('brightness', brightnessState, 0.5); // 0.0 in a -1..1 range
    bindKnob('mix', mixState, 1.0);
}

function bindKnob(id, sliderState, defaultNorm) {
    const knob = document.getElementById(`knob-${id}`);
    if (!knob) return;

    let isDragging = false;
    let startY, startValue;

    knob.addEventListener('pointerdown', (e) => {
        isDragging = true;
        startY = e.clientY;
        startValue = sliderState.getNormalisedValue();
        knob.setPointerCapture(e.pointerId);
        sliderState.sliderDragStarted();
        e.preventDefault();
    });

    knob.addEventListener('pointermove', (e) => {
        if (!isDragging) return;
        const deltaY = startY - e.clientY;
        const sensitivity = 0.005; // Slower rotation
        const newValue = Math.max(0, Math.min(1, startValue + deltaY * sensitivity));
        sliderState.setNormalisedValue(newValue);
    });

    knob.addEventListener('pointerup', () => {
        if (!isDragging) return;
        isDragging = false;
        sliderState.sliderDragEnded();
    });

    knob.addEventListener('pointercancel', () => {
        if (isDragging) {
            isDragging = false;
            sliderState.sliderDragEnded();
        }
    });

    // Double-click to reset to the parameter's default
    knob.addEventListener('dblclick', () => {
        sliderState.sliderDragStarted();
        sliderState.setNormalisedValue(defaultNorm);
        sliderState.sliderDragEnded();
    });

    // Initial rotation
    updateKnob(id);
}

function updateKnob(id) {
    const knob = document.getElementById(`knob-${id}`);
    if (!knob) return;

    const indicator = knob.querySelector('.knob-indicator');
    const valueDisplay = knob.parentElement.querySelector('.knob-value');
    let sliderState;

    if (id === 'brightness') sliderState = brightnessState;
    else if (id === 'mix') sliderState = mixState;

    if (!sliderState) return;

    const normValue = sliderState.getNormalisedValue();
    // Rotation: 0 = -135deg, 1 = +135deg
    const angle = -135 + normValue * 270;
    indicator.style.transform = `translate(-50%, -100%) rotate(${angle}deg)`;

    if (valueDisplay) {
        const scaledValue = sliderState.getScaledValue();
        valueDisplay.textContent = scaledValue.toFixed(2);
    }
}

// ============================================================================
// Freeze Toggle
// ============================================================================
function bindFreezeToggle() {
    const button = document.getElementById('freeze-button');
    if (!button) return;

    button.addEventListener('click', () => {
        freezeState.setValue(!freezeState.getValue());
    });
}

function updateFreezeVisual() {
    const button = document.getElementById('freeze-button');
    if (!button) return;

    const isActive = freezeState.getValue();

    if (isActive) {
        button.classList.add('active');
        canvas?.classList.add('frozen');
    } else {
        button.classList.remove('active');
        canvas?.classList.remove('frozen');
    }
}

// ============================================================================
// i18n — the canon v2 block, VERBATIM from scripts/i18n-canon.js
// ============================================================================
//
// Byte-compared by check-i18n assertion 6 after comment stripping and
// whitespace normalisation. Do not edit it here; edit the canon and every copy
// together, which is the whole point of holding it as data.
//
// It sits at the END of the module, below every existing initializer, and is
// REACHED only through the guarded initI18n() call inside the DOMContentLoaded
// handler above. Nothing here executes at module-evaluation time.

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
            .then((code) => applyI18n(code))
            .catch((e) => console.warn('Could not read language preference:', e));
    }

    const sel = document.getElementById('lang-select');
    if (sel) sel.addEventListener('change', (e) => {
        applyI18n(e.target.value);
        if (setUiLanguageNative) setUiLanguageNative(uiLanguage).catch(() => {});
    });
}

// ============================================================================
// Settings popover (v0.2.0)
// ============================================================================
//
// ONE row, and STILL one row at v0.3.0. Hover-help arrived in that version but
// it is always on: a toggle would mean a second control in this panel, a
// preference persisted through C++, and a data-tip-always bypass so the
// toggle's own tip still works when tips are off. Two of the suite's 43 plugins
// have such a toggle and 41 do not; making them agree is a decision across all
// 43, not a side effect here. So the popover still holds the language selector
// alone, and tip.gearBtn says exactly that rather than promising a control this
// plugin does not have.
//
// It opens DOWNWARDS: the gear sits 21 px from the top of a 600 px frame, so a
// panel above it would leave the frame entirely.

let settingsPopoverEl = null;
let gearBtnEl = null;

function setSettingsPopoverOpen(open) {
    if (!settingsPopoverEl || !gearBtnEl) return;
    settingsPopoverEl.hidden = !open;
    gearBtnEl.setAttribute('aria-expanded', open ? 'true' : 'false');
}

function initializeSettingsPopover() {
    gearBtnEl = document.getElementById('gear-btn');
    settingsPopoverEl = document.getElementById('settings-popover');

    if (!gearBtnEl || !settingsPopoverEl) {
        console.warn('settings popover missing - language selector unavailable');
        return;
    }

    gearBtnEl.addEventListener('click', (e) => {
        e.stopPropagation();
        setSettingsPopoverOpen(settingsPopoverEl.hidden);
    });

    // Dismiss on a press anywhere else, and on Escape. pointerdown rather than
    // click, so the panel is gone before a drag begins underneath it — every
    // knob and slider on this page starts its drag on pointerdown, and the
    // panel overhangs the XY pad.
    document.addEventListener('pointerdown', (e) => {
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
// Hover-help renderer (v0.3.0)
// ============================================================================
//
// THE COPY IS NOT LOOKED UP HERE. applyI18n() has already written it onto every
// anchor named in TIP_BINDINGS as data-tip-title + data-tip, in the current
// language, and it rewrites both on every language change. This function only
// positions and paints what the anchor is already carrying — which is why it is
// called AFTER initI18n() and not before: before, every anchor is bare and
// every hover would open an empty box.
//
// Ported from O-simpleFM's delegated, cursor-following family
// (js/app.js:384-462) rather than O-Tapestop's measure-then-pin engine: this
// page has no flip-above/below design and no clamp gates to serve, and 80 lines
// beat 180 for the same behaviour. Styled in this page's own aged-paper
// vocabulary in css/ouaricon-naturalist.css.
//
// Every property below is load-bearing, and each has a scar behind it:
//
//   1. DELEGATED ON document, never querySelectorAll('[data-tip]') at setup.
//      No anchor carries data-tip until applyI18n() has run, so a setup-time
//      query binds NOTHING and fails silently.
//   2. pointerover / pointerout / focusin / focusout, because those BUBBLE.
//      pointerenter / pointerleave / focus / blur do not, and a delegated
//      listener on document never sees them.
//   3. pointerout IGNORES a move between two descendants of the SAME anchor.
//      Every anchor here holds children — .vertical-slider has a caption, a
//      track, a thumb and a readout — and without this the tip would flicker
//      off and on at each internal boundary.
//   4. createElement + textContent, NEVER innerHTML. Localized copy must not
//      reach a markup path; check-i18n assertion 9 already forbids an angle
//      bracket in an i18n.js string literal and this is the other half of it.
//   5. Clamped on ALL FOUR edges with an 8 px margin, AFTER the flip. 800 x 600
//      is roomy and the flip is still the normal path, MEASURED at the shipping
//      frame: 5 of the 11 anchors flip horizontally (the mode row, the three
//      vertical sliders whose column ends 12 px from the right edge, and the
//      freeze button) and 3 flip vertically (both knobs and the freeze button,
//      all of which sit in the bottom strip) — the freeze button flips BOTH
//      ways. Clamping after the flip rather than instead of it means a tip that
//      fits on neither side still lands fully on screen.
//   6. pointer-events: none on the surface (CSS), or it steals the hover that
//      is keeping it open.
//   7. Escape hides it, and so does any pointerdown.
//   8. THE FOCUS ARM IS LATCHED TO THE KEYBOARD. See the comment at the latch.
//
// No tabindex is added to the pointer-only controls — see the note above
// TIP_BINDINGS in js/i18n.js. #gear-btn, #lang-select and the two enabled
// buttons carry the keyboard half.
function setupTooltips() {
    const tip = document.getElementById('tooltip');
    if (!tip) { console.warn('tooltip surface missing - hover-help unavailable'); return; }

    const MARGIN = 8;
    let active = null;

    const position = (x, y) => {
        const r = tip.getBoundingClientRect();
        const vw = window.innerWidth;
        const vh = window.innerHeight;
        let nx = x + 14;
        let ny = y + 16;
        // Flip to the other side of the cursor when the natural side overflows.
        if (nx + r.width  > vw - MARGIN) nx = x - r.width  - 14;
        if (ny + r.height > vh - MARGIN) ny = y - r.height - 12;
        // Then clamp, unconditionally, on all four edges. Math.max on the upper
        // bound keeps the arithmetic sane if a tip ever exceeds the frame.
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

    // Focus opens a tip as well — that is the keyboard half of this feature —
    // but ONLY when the focus arrived from the keyboard.
    //
    // A mouse click on a button FOCUSES it, so the reference implementation's
    // unconditional focusin rule re-opens the tip that pointerdown has just
    // hidden, with the pointer still on the anchor and no further pointerover
    // coming. Measured on two sibling plugins before the fix: the gear's own
    // tip stayed pinned across the settings popover the click had opened,
    // covering it by 146 x 35 px on O-Bass and 161 x 29 px on
    // O-AnalogSaturation, and it stayed until focus moved. On this page the
    // popover opens DOWNWARDS from a gear 21 px below the top edge, so the
    // gear's tip would land on top of the language selector itself.
    //
    // :focus-visible is deliberately NOT the discriminator. Chromium reports it
    // false for a programmatic .focus() that follows a click, so a gate driving
    // focus directly would measure "no tip" and record that as correct — a
    // false pass built into the fix. An explicit last-input-device latch is the
    // same rule and is drivable from a test with real events.
    let lastInputWasPointer = false;

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
    // driving again, which releases the latch above; Escape additionally hides.
    // Registered AFTER initializeSettingsPopover()'s own keydown listener, whose
    // Escape branch calls gearBtnEl.focus() — that focus is keyboard-driven by
    // definition, so the gear's tip correctly opens as focus returns to it.
    document.addEventListener('keydown', (e) => {
        lastInputWasPointer = false;
        if (e.key === 'Escape') hide();
    });
}
