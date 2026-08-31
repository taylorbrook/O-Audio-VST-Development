/*
   This file is part of O-Marimba, an Ouaricon Audio plugin.
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
// ============================================================================
// app.js — O-Marimba page controller (v1.13.0)
//
// EXTRACTED from index.html's inline <script type="module">, which through
// v1.12.1 was 1,247 lines ending at index.html:2496. The behaviour below the
// tooltip section is that script MOVED, not rewritten: the same listeners in
// the same order, with every visible string turned into a key.
//
// THE IMPORT SPECIFIERS CHANGED WITH THE MODULE'S DEPTH, and only they. The
// page module was at the UI ROOT and reached the bridge as './js/juce/index.js'
// and the shared units as './modules/...'. This file is served from /js/app.js,
// so those become './juce/index.js' and '../modules/...'. Module specifiers
// resolve against the importing module's URL, and the URL is what
// PluginEditor::getResource() answers — not the filesystem.
//
// THE OLD POSITIONER IS GONE, NOT DISABLED. v1.12.1's initTooltipSystem() read
// a flat data-tooltip attribute, positioned against .plugin-container rather
// than the viewport, and fell back to two hard-coded literals
// (`tooltip.offsetWidth || 200`, `tooltip.offsetHeight || 40`) whenever the
// surface had not been laid out yet — which is every first hover. It is deleted
// in full and replaced by the measure-then-pin renderer below. After this
// commit:
//
//     grep -rn 'tooltipHeight\|tooltipWidth\|data-tooltip' \
//          plugins/O-Marimba/Source/ui/public/
//
// returns nothing outside comments. Leaving both is how two renderers came to
// exist repo-wide in the first place.
// ============================================================================

import * as Juce from './juce/index.js';
import { AnalogEQUnitUI } from '../modules/analog-eq-unit.js';
import { CompressorUnit } from '../modules/compressor-unit.js';
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

// ═══════════════════════════════════════════════════════════════════════════
// The settings popover (v1.13.0)
// ═══════════════════════════════════════════════════════════════════════════
//
// The gear takes the EXACT absolute slot the floating "?" occupied through
// v1.12.1 — `position: absolute; bottom: 50px; right: 15px` — so nothing on a
// packed 600 x 400 layout had to move to make room for it. The hover-help
// toggle moves inside, beside the language selector: one place for the two
// things that decide what the hover help says and whether it says it at all.
//
// The panel opens UPWARDS from the gear, unlike O-IntonationPad's, because this
// gear sits near the bottom edge rather than in the header.

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
        console.warn('settings popover missing — language selector unavailable');
        return;
    }

    gearBtnEl.addEventListener('click', (e) => {
        e.stopPropagation();
        setSettingsPopoverOpen(settingsPopoverEl.hidden);
    });

    // Dismiss on a press anywhere else, and on Escape. mousedown rather than
    // click, so the panel is gone before a drag on a knob underneath it begins.
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


// ═══════════════════════════════════════════════════════════════════════════
// Tooltips — the measure-then-pin renderer (v1.13.0)
// ═══════════════════════════════════════════════════════════════════════════
//
// PORTED from O-ReverseDelay via O-IntonationPad, replacing v1.12.1's
// positioner ENTIRELY.
//
// What the port brings that the old one did not have: a title/body pair built
// from data-tip-title + data-tip rather than one flat string, a dwell delay so
// a tip does not fire on every crossing, a width RELEASED, MEASURED and PINNED
// before `left` is applied, an arrow whose offset is recomputed AFTER the
// horizontal clamp so a clamped tip still points at its control, and
// viewport-relative arithmetic on a `position: fixed` box instead of
// container-relative arithmetic with two hard-coded fallback numbers.
//
// The container-vs-viewport change is not cosmetic here. .plugin-container is
// 600 x 400 with `overflow: hidden`, and the old positioner clamped against
// ITS box — so a tip that fitted the container was correct only while the
// container filled the window. It does in the plugin frame and it does NOT in
// the Standalone build, where the window can be larger and the container is
// centred inside it. No ancestor of #tooltip establishes a containing block
// (no transform, filter or will-change on .plugin-container), so `fixed`
// resolves against the viewport as intended and is not clipped by the
// container's overflow.
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

// The hover-help layer's master switch. SESSION-ONLY.
//
// THE PLAN SAYS THIS PLUGIN "HAS THE TOOLTIPS BRIDGE". IT DOES NOT. v1.12.1
// called `window.JuceAPI.getNativeFunction('setTooltipsEnabled')(...)` inside a
// try/catch — and `window.JuceAPI` has never existed on this page (the bridge
// namespace is the imported `Juce` module, and the global is `window.__JUCE__`).
// The call threw on every toggle and the catch swallowed it, so the preference
// was never persisted and no symptom ever surfaced. Grepped against
// PluginEditor.cpp: there is no setTooltipsEnabled native function to reach.
//
// The dead call is REMOVED rather than repaired. Adding a real tooltips bridge
// is a processor-state change that does not belong in a commit about language,
// and repairing it in place would have shipped a new persisted preference under
// cover of a rename. Starts false, which is v1.12.1's observable behaviour
// unchanged.
let tooltipsEnabled = false;
let helpToggleEl = null;

function initializeTooltips() {
    tooltipEl = document.getElementById('tooltip');
    if (!tooltipEl) { console.warn('Tooltip element not found — tooltips disabled'); return; }

    initializeHelpToggle();

    document.addEventListener('mouseover', handleTooltipOver);
    document.addEventListener('mouseout', handleTooltipOut);

    // Any press begins a click or a drag: get the tip out of the way and keep it
    // away until release, so it cannot hang over a knob mid-drag. Capture phase,
    // because makeKnobDraggable calls preventDefault in its own mousedown
    // listener.
    document.addEventListener('pointerdown', () => {
        tooltipSuppressed = true;
        hideTooltip();
    }, true);

    document.addEventListener('pointerup', () => { tooltipSuppressed = false; }, true);

    console.log('[v1.13.0] Tooltips initialized');
}

function initializeHelpToggle() {
    helpToggleEl = document.getElementById('tips-toggle');
    if (!helpToggleEl) { console.warn('Help toggle not found — hover help stays off'); return; }

    helpToggleEl.addEventListener('click', () => setTooltipsEnabled(!tooltipsEnabled));

    setTooltipsEnabled(tooltipsEnabled);
}

function setTooltipsEnabled(enabled) {
    tooltipsEnabled = !!enabled;

    if (!tooltipsEnabled) hideTooltip();

    const frame = document.querySelector('.plugin-container');
    if (frame) frame.classList.toggle('tooltips-enabled', tooltipsEnabled);

    if (helpToggleEl) {
        // The two faces are KEYS through setLabel(), not literals. A literal
        // holds one string, so switching to French mid-session would restore an
        // English "On". if/else, not a ternary inside the call — check-i18n
        // assertion 13.
        helpToggleEl.setAttribute('aria-pressed', tooltipsEnabled ? 'true' : 'false');
        if (tooltipsEnabled) setLabel(helpToggleEl, 'ui.on');
        else                 setLabel(helpToggleEl, 'ui.off');
    }
}

// The gear and the toggle inside the popover both carry data-tip-always: the two
// controls that reach and restore the help layer have to keep explaining
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
    // .knob-container wraps a knob, a caption and a value readout, and crossing
    // between those three previously flickered the surface off and back on.
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
    // afterwards re-wraps a 200 px tip into a narrow ribbon — and the squeezed
    // width then resolves `left` straight back against the right edge, so it
    // never recovers on later hovers. Release the width, measure from the left
    // edge, pin the result in px, and only then place.
    //
    // The pinned width is the FRACTIONAL getBoundingClientRect().width, not the
    // integer offsetWidth: 189.34 rounds to 189, and pinning that makes the box
    // 0.34 px narrower than its own shrink-to-fit, pushing the last word onto a
    // second line. Height is only stable once the width is definite, so it is
    // read after (pattern_fixed_tooltip_shrink_to_fit_edge).
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

    // THE VERTICAL CLAMP, carried in from O-FreqPulse. The renderer as written
    // in O-ReverseDelay prefers above, flips below, and stops — correct on a
    // page whose anchors are all knob-sized, wrong the moment an anchor is tall
    // enough that NEITHER placement fits.
    //
    // IT IS **NOT** INDEPENDENTLY REPRODUCIBLE ON THIS PAGE, and it is carried
    // in anyway. MEASURED, not assumed: every visible [data-tip] anchor was
    // hovered across both tabs and both languages — 26 rendered tips — with
    // these two lines deleted, and NOT ONE left the 600 x 400 frame. Same case
    // as O-Polystutter, O-Lyrica and O-SpectralShaper; unlike O-FreqPulse and
    // O-IntonationPad, where deleting them puts real tips off-screen.
    //
    // The reason is the anchor geometry, not luck. This page's tallest tip
    // anchor is #interval-list at 231 px in a 400 px frame, and it sits 8 px
    // from the top of the tuning tab, so the "below" flip always has room. An
    // anchor tall enough that NEITHER placement fits does not exist here — yet.
    // The clamp costs two lines and there is ONE renderer repo-wide; a per-page
    // variant of it is how two renderers came to exist in the first place.
    //
    // THE SWEEP THAT REPORTED "not reproducible" IS NOT BLIND, which is the
    // claim that actually needed proving. Re-run with the HORIZONTAL clamp
    // deleted instead and the same sweep reports ten off-frame tips in both
    // languages, the worst of them:
    //
    //     [en/fr] #gear-btn        72.0 px past the right edge
    //     [en/fr] #strike-knob     29.5 px past the left edge
    //     [en/fr] #mallet-knob     26.5 px past the left edge
    //
    // So the probe does detect a tip leaving the frame; the vertical clamp
    // simply has nothing to catch on this page.
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


// ═══════════════════════════════════════════════════════════════════════════
// The page controller — moved verbatim in behaviour from index.html's inline
// <script type="module">, minus the tooltip block above and with every visible
// string turned into a key.
// ═══════════════════════════════════════════════════════════════════════════

// Disable right-click context menu
document.addEventListener("contextmenu", (e) => {
    e.preventDefault();
    return false;
});

// Verify JUCE backend
if (!window.__JUCE__) {
    console.error("JUCE backend not available");
    throw new Error("JUCE backend not connected");
}

console.log("JUCE backend connected:", window.__JUCE__.backend);

// ====================================================================
// TAB SWITCHING
// ====================================================================
let currentTab = 'main';

function switchTab(tabName) {
    const tabs = ['main', 'tuning', 'effects'];
    const tabButtons = ['tab-sound', 'tab-tuning', 'tab-effects'];
    const botanicalPositions = { tuning: 'tuning-position', effects: 'effects-position' };

    tabs.forEach((t, i) => {
        document.getElementById(t + '-tab').classList.toggle('active', t === tabName);
        document.getElementById(tabButtons[i]).classList.toggle('active', t === tabName);
    });

    const botanical = document.getElementById('botanical');
    botanical.classList.remove('tuning-position', 'effects-position');
    if (botanicalPositions[tabName]) botanical.classList.add(botanicalPositions[tabName]);
    currentTab = tabName;
}

document.getElementById('tab-sound').addEventListener('click', () => switchTab('main'));
document.getElementById('tab-tuning').addEventListener('click', () => switchTab('tuning'));
document.getElementById('tab-effects').addEventListener('click', () => switchTab('effects'));

// ====================================================================
// HELPER FUNCTIONS
// ====================================================================

// Update knob indicator rotation (fixed rotation)
function updateKnobIndicator(indicator, normalizedValue) {
    // Map 0-1 to -135deg to +135deg (270 degree range)
    const angle = -135 + (normalizedValue * 270);
    indicator.style.transform = `rotate(${angle}deg)`;
}

// Create draggable knob behavior
function makeKnobDraggable(knobElement, state, onUpdate) {
    let isDragging = false;

    knobElement.addEventListener('mousedown', (e) => {
        isDragging = true;
        state.sliderDragStarted();
        e.preventDefault();
    });

    document.addEventListener('mouseup', () => {
        if (isDragging) {
            isDragging = false;
            state.sliderDragEnded();
        }
    });

    document.addEventListener('mousemove', (e) => {
        if (isDragging) {
            const currentValue = state.getNormalisedValue();
            const newValue = Math.max(0, Math.min(1, currentValue - e.movementY * 0.005));
            state.setNormalisedValue(newValue);
        }
    });

    // Listen for value changes (from backend OR user interaction)
    state.valueChangedEvent.addListener(() => {
        const newValue = state.getNormalisedValue();
        onUpdate(newValue);
    });

    // Initialize with current value
    onUpdate(state.getNormalisedValue());
}

// ====================================================================
// PARAMETER BINDINGS
// ====================================================================

// MALLET_HARDNESS
const malletState = Juce.getSliderState("MALLET_HARDNESS");
const malletIndicator = document.getElementById("mallet-indicator");
const malletValue = document.getElementById("mallet-value");

makeKnobDraggable(document.getElementById("mallet-knob"), malletState, (value) => {
    updateKnobIndicator(malletIndicator, value);
    malletValue.textContent = Math.round(value * 100) + "%";
});

// BAR_MATERIAL
const materialState = Juce.getSliderState("BAR_MATERIAL");
const materialIndicator = document.getElementById("material-indicator");
const materialValue = document.getElementById("material-value");

makeKnobDraggable(document.getElementById("material-knob"), materialState, (value) => {
    updateKnobIndicator(materialIndicator, value);
    materialValue.textContent = Math.round(value * 100) + "%";
});

// RESONANCE
const resonanceState = Juce.getSliderState("RESONANCE");
const resonanceIndicator = document.getElementById("resonance-indicator");
const resonanceValue = document.getElementById("resonance-value");

makeKnobDraggable(document.getElementById("resonance-knob"), resonanceState, (value) => {
    updateKnobIndicator(resonanceIndicator, value);
    resonanceValue.textContent = Math.round(value * 100) + "%";
});

// v1.6.0: STRIKE_POSITION
const strikeState = Juce.getSliderState("STRIKE_POSITION");
const strikeIndicator = document.getElementById("strike-indicator");
const strikeValue = document.getElementById("strike-value");

makeKnobDraggable(document.getElementById("strike-knob"), strikeState, (value) => {
    updateKnobIndicator(strikeIndicator, value);
    // Show as "Edge" at 0/100%, "Center" at 50%
    if (value < 0.15 || value > 0.85) {
        strikeValue.textContent = "Edge";
    } else if (value > 0.4 && value < 0.6) {
        strikeValue.textContent = "Center";
    } else {
        strikeValue.textContent = Math.round(value * 100) + "%";
    }
});

// v1.6.0: OVERTONE_DAMPING
const dampingState = Juce.getSliderState("OVERTONE_DAMPING");
const dampingIndicator = document.getElementById("damping-indicator");
const dampingValue = document.getElementById("damping-value");

makeKnobDraggable(document.getElementById("damping-knob"), dampingState, (value) => {
    updateKnobIndicator(dampingIndicator, value);
    // Show descriptive labels at extremes
    if (value < 0.15) {
        dampingValue.textContent = "Shimmer";
    } else if (value > 0.85) {
        dampingValue.textContent = "Focused";
    } else {
        dampingValue.textContent = Math.round(value * 100) + "%";
    }
});

// v1.6.0: TONE
const toneState = Juce.getSliderState("TONE");
const toneIndicator = document.getElementById("tone-indicator");
const toneValueEl = document.getElementById("tone-value");

makeKnobDraggable(document.getElementById("tone-knob"), toneState, (value) => {
    updateKnobIndicator(toneIndicator, value);
    // Show descriptive labels at extremes
    if (value < 0.15) {
        toneValueEl.textContent = "Warm";
    } else if (value > 0.85) {
        toneValueEl.textContent = "Bright";
    } else {
        toneValueEl.textContent = Math.round(value * 100) + "%";
    }
});

// VEL_CURVE
const velCurveState = Juce.getSliderState("VEL_CURVE");
const velocityIndicator = document.getElementById("velocity-indicator");
const velocityValue = document.getElementById("velocity-value");
const velocityCurve = document.getElementById("velocity-curve");
const velocityFill = document.getElementById("velocity-fill");

function updateVelocityCurve(value) {
    const cpX = 70;
    const cpY = 85 - (70 * (1 - value * 0.8));
    const pathD = `M15,85 Q${cpX},${cpY} 125,15`;
    const fillD = `M15,85 Q${cpX},${cpY} 125,15 L125,85 Z`;
    velocityCurve.setAttribute('d', pathD);
    velocityFill.setAttribute('d', fillD);
}

makeKnobDraggable(document.getElementById("velocity-knob"), velCurveState, (value) => {
    updateKnobIndicator(velocityIndicator, value);
    velocityValue.textContent = Math.round(value * 100) + "%";
    updateVelocityCurve(value);
});

// OUTPUT_GAIN
const outputGainState = Juce.getSliderState("OUTPUT_GAIN");
const outputIndicator = document.getElementById("output-indicator");
const outputValue = document.getElementById("output-value");

makeKnobDraggable(document.getElementById("output-knob"), outputGainState, (value) => {
    updateKnobIndicator(outputIndicator, value);
    const dbValue = -24.0 + (value * 36.0);
    outputValue.textContent = dbValue.toFixed(1) + " dB";
});

// TUNING_MODE (Choice parameter - button group)
const tuningModeState = Juce.getSliderState("TUNING_MODE");
const btn12tet = document.getElementById("btn-12tet");
const btnScala = document.getElementById("btn-scala");
const btnMts = document.getElementById("btn-mts");
const scaleName = document.getElementById("scale-name");
const scalaButtons = document.getElementById("scala-buttons");
const mtsStatus = document.getElementById("mts-status");

function updateTuningModeUI(normalizedValue) {
    const index = Math.round(normalizedValue * 2);
    currentTuningMode = index; // Track current mode for table editing
    btn12tet.classList.toggle('active', index === 0);
    btnScala.classList.toggle('active', index === 1);
    btnMts.classList.toggle('active', index === 2);

    // Show/hide mode-specific controls
    scalaButtons.style.display = index === 1 ? 'flex' : 'none';
    mtsStatus.style.display = index === 2 ? 'flex' : 'none';

    if (index === 0) {
        // Load 12-TET preset
        loadScalePreset('12tet');
    } else if (index === 1) {
        // Load Just Intonation as default Custom preset
        loadScalePreset('just');
    } else if (index === 2) {
        scaleName.textContent = "MTS-ESP (stub)";
        // MTS-ESP would override tuning from external source
    }

    // Update keyboard visualization
    updateKeyboardViz(index);

    // Refresh interval list to update editable state
    updateIntervalListUI();
}

btn12tet.addEventListener('click', () => tuningModeState.setNormalisedValue(0.0));
btnScala.addEventListener('click', () => tuningModeState.setNormalisedValue(0.5));
btnMts.addEventListener('click', () => tuningModeState.setNormalisedValue(1.0));

tuningModeState.valueChangedEvent.addListener(() => {
    updateTuningModeUI(tuningModeState.getNormalisedValue());
});

// Scala file loading
const loadScalaFileNative = Juce.getNativeFunction("loadScalaFile");
const loadKBMFileNative = Juce.getNativeFunction("loadKBMFile");

// v1.4.0: Scala file saving
const saveScalaFileNative = Juce.getNativeFunction("saveScalaFile");
const saveKBMFileNative = Juce.getNativeFunction("saveKBMFile");

document.getElementById('btn-load-scl').addEventListener('click', () => {
    loadScalaFileNative();
});

document.getElementById('btn-load-kbm').addEventListener('click', () => {
    loadKBMFileNative();
});

// v1.4.0: Save .scl file
document.getElementById('btn-save-scl').addEventListener('click', () => {
    saveScalaFileNative();
});

// v1.4.0: Save .kbm file
document.getElementById('btn-save-kbm').addEventListener('click', () => {
    saveKBMFileNative();
});

// Get native function to fetch intervals
const getTuningIntervalsNative = Juce.getNativeFunction("getTuningIntervals");

// Callback when Scala file is loaded (called from C++)
window.onScalaLoaded = async function(scaleName) {
    document.getElementById('scale-name').textContent = scaleName;
    currentScaleName = scaleName;

    // Fetch the loaded intervals from the TuningEngine
    try {
        const result = await getTuningIntervalsNative();
        if (result && result.intervals) {
            // Update currentIntervals with ALL loaded intervals (any scale size)
            // The intervals array includes unison (0) at index 0
            currentIntervals = [];
            for (let i = 0; i < result.intervals.length; i++) {
                currentIntervals.push(result.intervals[i]);
            }

            // Update UI with dynamic scale size
            updateIntervalListUI();
            drawPitchCircle();
            console.log('Loaded scale with', currentIntervals.length, 'notes:', currentIntervals);
        }
    } catch (e) {
        console.error('Failed to get tuning intervals:', e);
    }
};

// REFERENCE_PITCH
const refPitchState = Juce.getSliderState("REFERENCE_PITCH");
const refPitchIndicator = document.getElementById("ref-pitch-indicator");
const refPitchValue = document.getElementById("ref-pitch-value");
const refPitchKnob = document.getElementById("ref-pitch-knob");

makeKnobDraggable(refPitchKnob, refPitchState, (value) => {
    updateKnobIndicator(refPitchIndicator, value);
    const hzValue = 400.0 + (value * 80.0);
    refPitchValue.textContent = hzValue.toFixed(1) + " Hz";
});

// Double-click to reset A4 reference to 440 Hz
refPitchKnob.addEventListener('dblclick', () => {
    // 440 Hz = 400 + (0.5 * 80), so normalized value is 0.5
    refPitchState.setNormalisedValue(0.5);
});

// ====================================================================
// TUNING VISUALIZATIONS
// ====================================================================

// Get native function for setting tuning
const setTuningIntervals = Juce.getNativeFunction("setTuningIntervals");

// Get native function for setting tonic (transposition)
const setTonicNote = Juce.getNativeFunction("setTonicNote");

// Current editable intervals (dynamic size based on loaded scale)
let currentIntervals = [0, 100, 200, 300, 400, 500, 600, 700, 800, 900, 1000, 1100];
let currentScaleName = '12-TET Standard';
let currentTonic = 0; // 0 = C, 1 = C#, 2 = D, etc.
let currentTuningMode = 0; // 0 = 12-TET, 1 = Custom, 2 = MTS-ESP

// Note names for tonic display
const noteNames = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B'];

// Scale presets (cents from root, including unison at 0)
const scalePresets = {
    '12tet': {
        name: '12-TET Standard',
        intervals: [0, 100, 200, 300, 400, 500, 600, 700, 800, 900, 1000, 1100]
    },
    'just': {
        name: 'Just Intonation',
        intervals: [0, 112, 204, 316, 386, 498, 590, 702, 814, 884, 996, 1088]
    },
    'pythagorean': {
        name: 'Pythagorean',
        intervals: [0, 90, 204, 294, 408, 498, 612, 702, 792, 906, 996, 1110]
    }
};

// Generate degree label for any position (with tonic offset)
function getDegreeLabel(index, total) {
    if (total === 12) {
        // Standard chromatic labels with tonic offset
        const noteIndex = (index + currentTonic) % 12;
        return noteNames[noteIndex];
    }
    // For other scales, use numbers
    return String(index + 1);
}

// Parse user input as cents or ratio
function parseIntervalInput(value) {
    value = value.trim();
    if (!value) return null;

    // Check if it's a ratio (contains /)
    if (value.includes('/')) {
        const parts = value.split('/');
        if (parts.length === 2) {
            const num = parseFloat(parts[0]);
            const den = parseFloat(parts[1]);
            if (!isNaN(num) && !isNaN(den) && den !== 0) {
                // Convert ratio to cents: 1200 * log2(ratio)
                return 1200 * Math.log2(num / den);
            }
        }
        return null;
    }

    // Otherwise treat as cents
    const cents = parseFloat(value);
    return isNaN(cents) ? null : cents;
}

// Send current intervals to backend
function applyTuning() {
    // Send intervals (excluding unison at index 0)
    const intervalsToSend = currentIntervals.slice(1);
    setTuningIntervals(intervalsToSend, currentScaleName);
    console.log('Applied tuning:', currentScaleName, intervalsToSend);
}

// Load a preset scale
function loadScalePreset(presetKey) {
    const preset = scalePresets[presetKey];
    if (!preset) return;

    currentScaleName = preset.name;
    currentIntervals = [...preset.intervals]; // Copy the full array

    updateIntervalListUI();
    drawPitchCircle();
    applyTuning();

    document.getElementById('scale-name').textContent = currentScaleName;
}

function drawPitchCircle() {
    const linesGroup = document.getElementById('interval-lines');
    const labelsGroup = document.getElementById('degree-labels');

    // Clear previous
    linesGroup.innerHTML = '';
    labelsGroup.innerHTML = '';

    const cx = 75, cy = 75;
    const innerR = 8;
    const outerR = 55;
    const labelR = 65;
    const total = currentIntervals.length;

    // Adjust font size based on number of notes
    const fontSize = total > 12 ? 5 : 7;

    currentIntervals.forEach((cents, i) => {
        // Convert cents to angle (0 cents = top, clockwise)
        const angle = (cents / 1200) * 360 - 90;
        const rad = angle * (Math.PI / 180);

        // Calculate line endpoints
        const x1 = cx + innerR * Math.cos(rad);
        const y1 = cy + innerR * Math.sin(rad);
        const x2 = cx + outerR * Math.cos(rad);
        const y2 = cy + outerR * Math.sin(rad);

        // Draw line
        const line = document.createElementNS('http://www.w3.org/2000/svg', 'line');
        line.setAttribute('x1', x1);
        line.setAttribute('y1', y1);
        line.setAttribute('x2', x2);
        line.setAttribute('y2', y2);
        line.setAttribute('stroke', '#6B8E4E');
        line.setAttribute('stroke-width', total > 19 ? '1' : '2');
        line.setAttribute('stroke-linecap', 'round');
        linesGroup.appendChild(line);

        // Draw label (skip some labels if too many notes)
        if (total <= 24 || i % 2 === 0) {
            const labelX = cx + labelR * Math.cos(rad);
            const labelY = cy + labelR * Math.sin(rad);
            const text = document.createElementNS('http://www.w3.org/2000/svg', 'text');
            text.setAttribute('x', labelX);
            text.setAttribute('y', labelY + 2);
            text.setAttribute('text-anchor', 'middle');
            text.setAttribute('font-size', fontSize);
            text.setAttribute('fill', '#3C2F2F');
            text.textContent = getDegreeLabel(i, total);
            labelsGroup.appendChild(text);
        }
    });
}

function updateIntervalListUI() {
    const list = document.getElementById('interval-list');
    const total = currentIntervals.length;

    // Show tonic selector only for 12-tone scales
    let tonicHtml = '';
    if (total === 12) {
        tonicHtml = `
            <div class="tonic-selector" data-i18n-aria="aria.tonicSelector">
                <span class="tonic-arrow" id="tonic-down" data-i18n-aria="aria.tonicPrev">◀</span>
                <span class="tonic-label" data-i18n="label.tonic">Tonic:</span>
                <span class="tonic-value" id="tonic-display">${noteNames[currentTonic]}</span>
                <span class="tonic-arrow" id="tonic-up" data-i18n-aria="aria.tonicNext">▶</span>
            </div>
        `;
    }

    // CONTRACT §6: `Intervals (${total} notes)` inflected a noun on a count, and
    // French pluralises 0 as singular where English does not. The noun is dropped
    // and the count moved behind a colon, where it is invariant in both languages.
    // data-i18n-vars is JSON, so the number goes in unquoted and applyLabel
    // JSON.parses it back out.
    let html = `<div class="interval-list-header" data-i18n="label.intervalHeader" `
             + `data-i18n-vars='{"n":${total}}'>Intervals: ${total}</div>${tonicHtml}`;

    // Show rows for actual number of intervals
    // In 12-TET mode (index 0), all inputs are disabled (non-editable)
    const isEditable = (currentTuningMode === 1); // Only editable in Custom mode
    for (let i = 0; i < total; i++) {
        const cents = currentIntervals[i] || 0;
        const isUnison = (i === 0);
        const displayValue = isUnison ? '0' : cents.toFixed(1);
        const label = getDegreeLabel(i, total);
        const isDisabled = isUnison || !isEditable;

        html += `
            <div class="interval-item">
                <span class="interval-degree">${label}</span>
                <input type="text"
                       class="interval-input"
                       data-index="${i}"
                       value="${displayValue}"
                       ${isDisabled ? 'disabled' : ''}
                       placeholder="${Math.round(i * 1200 / total)}">
            </div>
        `;
    }

    list.innerHTML = html;

    // The header, the tonic caption and the three tonic accessible names are
    // BRAND-NEW nodes the last applyI18n() sweep never saw, and this function runs
    // on every tuning-mode change, every tonic step and every preset load.
    // Re-sweep at the CURRENT language: guessing 'en' here would silently reset a
    // French page every time a user nudged the tonic.
    if (typeof window.__reapplyI18n === 'function') window.__reapplyI18n();

    // Add event listeners to inputs
    list.querySelectorAll('.interval-input:not([disabled])').forEach(input => {
        input.addEventListener('change', (e) => {
            const index = parseInt(e.target.dataset.index);
            const parsed = parseIntervalInput(e.target.value);

            if (parsed !== null) {
                currentIntervals[index] = parsed;
                e.target.value = parsed.toFixed(1);
                currentScaleName = 'Custom';
                document.getElementById('scale-name').textContent = currentScaleName;
                drawPitchCircle();
                applyTuning();
            } else {
                // Invalid input, revert
                e.target.value = (currentIntervals[index] || 0).toFixed(1);
            }
        });

        // Apply on Enter key
        input.addEventListener('keydown', (e) => {
            if (e.key === 'Enter') {
                e.target.blur();
            }
        });
    });

    // Add tonic arrow click handlers (only present for 12-tone scales)
    const tonicUp = document.getElementById('tonic-up');
    const tonicDown = document.getElementById('tonic-down');

    if (tonicUp) {
        tonicUp.addEventListener('click', (e) => {
            e.stopPropagation();
            // Move up: C -> C# -> D -> ... -> B -> C
            currentTonic = (currentTonic + 1) % 12;
            applyTonicChange();
        });
    }

    if (tonicDown) {
        tonicDown.addEventListener('click', (e) => {
            e.stopPropagation();
            // Move down: C -> B -> A# -> ... -> C# -> C
            currentTonic = (currentTonic - 1 + 12) % 12;
            applyTonicChange();
        });
    }
}

// Apply tonic change: update display, interval labels, pitch circle, and send to backend
function applyTonicChange() {
    // Update display
    document.getElementById('tonic-display').textContent = noteNames[currentTonic];

    // Redraw interval list labels and pitch circle
    updateIntervalListUI();
    drawPitchCircle();

    // Send tonic to backend for transposition
    setTonicNote(currentTonic);

    console.log('Tonic changed to:', noteNames[currentTonic], '(transposition:', currentTonic, 'semitones)');
}

function updateKeyboardViz(modeIndex) {
    const allKeys = document.querySelectorAll('.white-key, .black-key');

    // All keys mapped (single octave shows all 12 notes)
    allKeys.forEach(key => key.classList.add('mapped'));
}

// v1.2.6: Polyphonic note tracking for interval line highlighting
// Map: scaleIndex -> { count: number, maxVelocity: number }
// count tracks how many notes map to this scale degree (for octave stacking)
// maxVelocity tracks the highest velocity for intensity
const activeNotes = new Map();

// Map MIDI note to scale index
function midiNoteToScaleIndex(midiNote) {
    const noteInOctave = midiNote % 12;
    const scaleSize = currentIntervals.length;

    if (scaleSize === 12) {
        return noteInOctave;
    } else {
        // Proportional mapping for non-12-tone scales
        return Math.round((noteInOctave / 12) * scaleSize) % scaleSize;
    }
}

// Calculate red color intensity based on velocity (0-1)
// Low velocity: darker red (rgb(120, 40, 40))
// High velocity: bright red (rgb(220, 0, 0))
function velocityToColor(velocity) {
    const minR = 120, maxR = 220;
    const minG = 40, maxG = 0;
    const minB = 40, maxB = 0;

    const r = Math.round(minR + velocity * (maxR - minR));
    const g = Math.round(minG + velocity * (maxG - minG));
    const b = Math.round(minB + velocity * (maxB - minB));

    return `rgb(${r}, ${g}, ${b})`;
}

// Activate interval line for a note (note-on)
function setNoteActive(midiNote, velocity) {
    const scaleIndex = midiNoteToScaleIndex(midiNote);

    // Track this note
    if (activeNotes.has(scaleIndex)) {
        const entry = activeNotes.get(scaleIndex);
        entry.count++;
        entry.maxVelocity = Math.max(entry.maxVelocity, velocity);
    } else {
        activeNotes.set(scaleIndex, { count: 1, maxVelocity: velocity });
    }

    // Update visual
    updateIntervalLineVisual(scaleIndex);
}

// Deactivate interval line for a note (note-off)
function setNoteInactive(midiNote) {
    const scaleIndex = midiNoteToScaleIndex(midiNote);

    if (activeNotes.has(scaleIndex)) {
        const entry = activeNotes.get(scaleIndex);
        entry.count--;

        if (entry.count <= 0) {
            activeNotes.delete(scaleIndex);
        }
    }

    // Update visual
    updateIntervalLineVisual(scaleIndex);
}

// Update the visual appearance of an interval line
function updateIntervalLineVisual(scaleIndex) {
    const linesGroup = document.getElementById('interval-lines');
    const lines = linesGroup.querySelectorAll('line');

    if (scaleIndex >= 0 && scaleIndex < lines.length) {
        const line = lines[scaleIndex];

        if (activeNotes.has(scaleIndex)) {
            // Active: red with velocity-based intensity
            const entry = activeNotes.get(scaleIndex);
            line.setAttribute('stroke', velocityToColor(entry.maxVelocity));
            line.setAttribute('stroke-width', '4');
        } else {
            // Inactive: restore original green
            const scaleSize = currentIntervals.length;
            line.setAttribute('stroke', '#6B8E4E');
            line.setAttribute('stroke-width', scaleSize > 19 ? '1' : '2');
        }
    }
}

// Export to window for C++ evaluateJavascript calls
window.setNoteActive = setNoteActive;
window.setNoteInactive = setNoteInactive;

// Legacy support: flashIntervalLine now maps to setNoteActive with brief timeout
function flashIntervalLine(midiNote) {
    setNoteActive(midiNote, 0.8);
    setTimeout(() => setNoteInactive(midiNote), 150);
}
window.flashIntervalLine = flashIntervalLine;

// ====================================================================
// AUDIO VISUALIZATIONS - v1.2.3: Live oscilloscope
// ====================================================================

// Get native function for waveform data
const getWaveformDataNative = Juce.getNativeFunction("getWaveformData");

// Oscilloscope display elements
const waveformPath = document.getElementById('waveform-path');
const waveformFill = document.getElementById('waveform-fill');

// Draw waveform from sample data
function drawWaveform(samples) {
    if (!samples || samples.length === 0) return;

    const width = 260;
    const height = 120;
    const centerY = height / 2;
    const amplitude = height / 2 - 5; // Leave margin

    // Build SVG path from samples
    let pathD = '';
    const numPoints = samples.length;
    const xStep = width / (numPoints - 1);

    for (let i = 0; i < numPoints; i++) {
        const x = i * xStep;
        const y = centerY - (samples[i] * amplitude);
        if (i === 0) {
            pathD += `M${x.toFixed(1)},${y.toFixed(1)}`;
        } else {
            pathD += ` L${x.toFixed(1)},${y.toFixed(1)}`;
        }
    }

    waveformPath.setAttribute('d', pathD);

    // Fill path (close to bottom)
    const fillD = pathD + ` L${width},${height} L0,${height} Z`;
    waveformFill.setAttribute('d', fillD);
}

// Initialize waveform as flat line
function initWaveform() {
    waveformPath.setAttribute('d', 'M0,60 L260,60');
    waveformFill.setAttribute('d', 'M0,60 L260,60 L260,120 L0,120 Z');
}

// Poll for waveform data (30fps)
let waveformAnimationId = null;
async function updateWaveform() {
    try {
        const data = await getWaveformDataNative();
        if (data && Array.isArray(data) && data.length > 0) {
            drawWaveform(data);
        }
    } catch (e) {
        // Silently ignore errors (plugin may not be playing)
    }
    waveformAnimationId = requestAnimationFrame(updateWaveform);
}

// Start waveform animation
updateWaveform();

// ===== VU METER ANIMATION =====
// v1.2.5: Live VU meter driven by C++ backend events
const vuNeedle = document.getElementById('vu-needle');
let currentVUAngle = -90;
let targetVUAngle = -90;

const VU_ATTACK_SPEED = 0.5;   // Fast attack
const VU_DECAY_SPEED = 0.08;   // Smooth decay
const VU_MIN_ANGLE = -90;      // -60dB position (fully left)
const VU_MAX_ANGLE = 90;       // +3dB position (fully right)
const VU_SNAP_THRESHOLD = 2.0; // Snap to rest when within 2 degrees

// Map dB to needle angle
// Full sweep: -60dB at -90° to +3dB at +90°
function mapDBToAngle(dbLevel) {
    const minDB = -60;
    const maxDB = 3;

    // Clamp to range
    const clampedDB = Math.max(minDB, Math.min(maxDB, dbLevel));

    // Linear mapping: -60dB → -90°, +3dB → +90°
    const normalized = (clampedDB - minDB) / (maxDB - minDB);
    return VU_MIN_ANGLE + (normalized * (VU_MAX_ANGLE - VU_MIN_ANGLE));
}

// Update needle color: dark green (quiet) to bright red (loud)
function updateNeedleColor(angle) {
    // Normalize angle from -90..+90 to 0..1
    const normalized = (angle - VU_MIN_ANGLE) / (VU_MAX_ANGLE - VU_MIN_ANGLE);

    // Interpolate from dark green to bright red
    // Dark green: rgb(34, 139, 34)
    // Bright red: rgb(255, 50, 50)
    const r = Math.round(34 + normalized * (255 - 34));
    const g = Math.round(139 - normalized * (139 - 50));
    const b = Math.round(34 + normalized * (50 - 34));

    const color = `rgb(${r}, ${g}, ${b})`;
    const colorDark = `rgb(${Math.round(r * 0.8)}, ${Math.round(g * 0.8)}, ${Math.round(b * 0.8)})`;

    vuNeedle.style.background = `linear-gradient(180deg, ${color} 0%, ${colorDark} 100%)`;
}

// Listen for output level from C++ via JUCE backend events
if (window.__JUCE__ && window.__JUCE__.backend) {
    window.__JUCE__.backend.addEventListener("outputLevel", (dbLevel) => {
        targetVUAngle = mapDBToAngle(dbLevel);
    });
}

// Animate VU meter with ballistic motion (fast attack, slow decay)
function animateVUMeter() {
    const vuSpeed = currentVUAngle < targetVUAngle ? VU_ATTACK_SPEED : VU_DECAY_SPEED;
    currentVUAngle += (targetVUAngle - currentVUAngle) * vuSpeed;

    // Snap to rest position when close (prevents endless asymptotic approach)
    if (targetVUAngle <= VU_MIN_ANGLE && Math.abs(currentVUAngle - VU_MIN_ANGLE) < VU_SNAP_THRESHOLD) {
        currentVUAngle = VU_MIN_ANGLE;
    }

    vuNeedle.style.transform = `translateX(-50%) rotate(${currentVUAngle}deg)`;
    updateNeedleColor(currentVUAngle);

    requestAnimationFrame(animateVUMeter);
}

// Initialize displays
initWaveform();
animateVUMeter();

// ====================================================================
// PLAYABLE KEYBOARD
// ====================================================================

// Map keyboard key IDs to MIDI note numbers (C4 = 60 = middle C)
const keyToMidi = {
    // Single Octave (C4-B4)
    'key-c1': 60,  'key-cs1': 61, 'key-d1': 62,  'key-ds1': 63,
    'key-e1': 64,  'key-f1': 65,  'key-fs1': 66, 'key-g1': 67,
    'key-gs1': 68, 'key-a1': 69,  'key-as1': 70, 'key-b1': 71
};

// Get native function for sending MIDI
const sendMidiNote = Juce.getNativeFunction("sendMidiNote");

// Track currently playing keys (for proper note-off handling)
const playingKeys = new Set();

// Handle key press (note on)
function handleKeyDown(keyId) {
    const midiNote = keyToMidi[keyId];
    if (midiNote === undefined) return;

    // Avoid retriggering if already playing
    if (playingKeys.has(keyId)) return;

    playingKeys.add(keyId);
    const keyEl = document.getElementById(keyId);
    if (keyEl) keyEl.classList.add('playing');

    // NOTE: Don't call flashIntervalLine here - the C++ timer will handle it
    // via popLastPlayedNote() to avoid race condition causing permanent red lines

    // Send note on (velocity = 0.8)
    sendMidiNote(midiNote, 0.8, true);
}

// Handle key release (note off)
function handleKeyUp(keyId) {
    const midiNote = keyToMidi[keyId];
    if (midiNote === undefined) return;

    playingKeys.delete(keyId);
    const keyEl = document.getElementById(keyId);
    if (keyEl) keyEl.classList.remove('playing');

    // Send note off
    sendMidiNote(midiNote, 0.0, false);
}

// Attach handlers to all keyboard keys
// Black keys are now siblings of white keys, so no parent/child checks needed
Object.keys(keyToMidi).forEach(keyId => {
    const keyEl = document.getElementById(keyId);
    if (!keyEl) return;

    keyEl.addEventListener('mousedown', (e) => {
        e.preventDefault();
        handleKeyDown(keyId);
    });

    keyEl.addEventListener('mouseup', () => {
        handleKeyUp(keyId);
    });

    keyEl.addEventListener('mouseleave', () => {
        if (playingKeys.has(keyId)) {
            handleKeyUp(keyId);
        }
    });

    keyEl.addEventListener('touchstart', (e) => {
        e.preventDefault();
        handleKeyDown(keyId);
    });

    keyEl.addEventListener('touchend', (e) => {
        e.preventDefault();
        handleKeyUp(keyId);
    });
});

// Global mouseup handler to catch releases outside keys
document.addEventListener('mouseup', () => {
    playingKeys.forEach(keyId => handleKeyUp(keyId));
});

console.log("Keyboard MIDI playback enabled");

// ====================================================================
// v1.3.0: PRESET SYSTEM (v1.3.1: Added LOAD button + dropdown)
// ====================================================================

// Get native functions for preset operations
const savePresetNative = Juce.getNativeFunction("savePreset");
const loadPresetNative = Juce.getNativeFunction("loadPreset");
const loadPresetFromFileNative = Juce.getNativeFunction("loadPresetFromFile");  // v1.3.1
const getPresetListNative = Juce.getNativeFunction("getPresetList");
const getCurrentPresetNative = Juce.getNativeFunction("getCurrentPreset");
const selectNextPresetNative = Juce.getNativeFunction("selectNextPreset");
const selectPreviousPresetNative = Juce.getNativeFunction("selectPreviousPreset");

// Preset display elements
const presetNameDisplay = document.getElementById('preset-name-display');
const presetDropdown = document.getElementById('preset-dropdown');

// Current preset list (cached for navigation)
let presetList = [];
let currentPresetIndex = 0;

// Update preset name display
function updatePresetDisplay(presetName) {
    presetNameDisplay.textContent = presetName || 'Default Marimba';
}

// Load preset list from backend
async function refreshPresetList() {
    try {
        const list = await getPresetListNative();
        if (Array.isArray(list)) {
            presetList = list;
            console.log('Preset list loaded:', presetList.length, 'presets');
        }
    } catch (e) {
        console.error('Failed to load preset list:', e);
    }
}

// v1.3.1: Populate and show dropdown menu
function showPresetDropdown() {
    presetDropdown.innerHTML = '';

    // Returns the section's header element, or null when the section is empty,
    // so the CALLER can key it with a plain string literal.
    //
    // v1.12.1 wrote `header.textContent = label`, where `label` was the string
    // 'Factory' or 'User'. A literal captured there holds ONE language, so a
    // dropdown opened before a language change and reopened after it would still
    // say "Factory". setLabel() makes the header a [data-i18n] element from the
    // moment it is created, so the sweep owns it from then on — ONE re-render
    // path, no second copy to go stale.
    //
    // The key is passed at the CALL SITE rather than through this function,
    // because check-i18n assertion 13 rejects a computed setLabel key: a key
    // read from a parameter cannot be checked against the table, and a raw copy
    // string in that position would ship English with nothing to catch it.
    function addPresetSection(presets, isFactory) {
        if (presets.length === 0) return null;

        const header = document.createElement('div');
        header.className = 'preset-dropdown-header';
        presetDropdown.appendChild(header);

        presets.forEach(preset => {
            const item = document.createElement('div');
            item.className = 'preset-dropdown-item' + (isFactory ? ' factory' : '');
            if (preset.name === presetNameDisplay.textContent) item.classList.add('active');
            item.textContent = preset.name;
            item.addEventListener('click', (e) => {
                e.stopPropagation();
                selectPresetFromDropdown(preset.name);
            });
            presetDropdown.appendChild(item);
        });

        return header;
    }

    const factoryHeader = addPresetSection(presetList.filter(p => p.isFactory), true);
    if (factoryHeader) setLabel(factoryHeader, 'label.presetFactory');

    const userHeader = addPresetSection(presetList.filter(p => !p.isFactory), false);
    if (userHeader) setLabel(userHeader, 'label.presetUser');

    presetDropdown.classList.add('show');
}

// v1.3.1: Hide dropdown
function hidePresetDropdown() {
    presetDropdown.classList.remove('show');
}

// v1.3.1: Select preset from dropdown
async function selectPresetFromDropdown(presetName) {
    hidePresetDropdown();
    try {
        await loadPresetNative(presetName);
        console.log('Selected preset:', presetName);
    } catch (e) {
        console.error('Failed to load preset:', e);
    }
}

// Initialize preset display
async function initializePresets() {
    await refreshPresetList();

    // Get current preset name
    try {
        const currentName = await getCurrentPresetNative();
        updatePresetDisplay(currentName);

        // Find index in list
        currentPresetIndex = presetList.findIndex(p => p.name === currentName);
        if (currentPresetIndex < 0) currentPresetIndex = 0;
    } catch (e) {
        console.error('Failed to get current preset:', e);
    }
}

// v1.3.1: Preset name click - toggle dropdown
presetNameDisplay.addEventListener('click', async (e) => {
    e.stopPropagation();
    if (presetDropdown.classList.contains('show')) {
        hidePresetDropdown();
    } else {
        await refreshPresetList();  // Refresh list before showing
        showPresetDropdown();
    }
});

// v1.3.1: Close dropdown when clicking outside
document.addEventListener('click', (e) => {
    if (!presetDropdown.contains(e.target) && e.target !== presetNameDisplay) {
        hidePresetDropdown();
    }
});

// v1.3.1: LOAD button click handler - opens file dialog
document.getElementById('preset-load-btn').addEventListener('click', async () => {
    console.log('Load preset clicked');
    loadPresetFromFileNative();
});

// Save button click handler
document.getElementById('preset-save-btn').addEventListener('click', async () => {
    console.log('Save preset clicked');
    savePresetNative();
});

// Previous preset button
document.getElementById('preset-prev').addEventListener('click', async () => {
    try {
        const prevName = await selectPreviousPresetNative();
        console.log('Previous preset:', prevName);
    } catch (e) {
        console.error('Failed to select previous preset:', e);
    }
});

// Next preset button
document.getElementById('preset-next').addEventListener('click', async () => {
    try {
        const nextName = await selectNextPresetNative();
        console.log('Next preset:', nextName);
    } catch (e) {
        console.error('Failed to select next preset:', e);
    }
});

// Callback when preset is saved (called from C++)
window.onPresetSaved = function(presetName) {
    console.log('Preset saved:', presetName);
    updatePresetDisplay(presetName);
    refreshPresetList();  // Refresh list to include new preset
};

// Callback when preset is loaded (called from C++)
// Updates tuning visualization with loaded preset data
window.onPresetLoaded = function(presetName, scaleName, intervals, tonic) {
    console.log('Preset loaded:', presetName, 'Scale:', scaleName);

    // Update preset display
    updatePresetDisplay(presetName);

    // Close dropdown if open
    hidePresetDropdown();

    // Update tuning state
    currentScaleName = scaleName;
    currentIntervals = intervals || [0, 100, 200, 300, 400, 500, 600, 700, 800, 900, 1000, 1100];
    currentTonic = tonic || 0;

    // Update scale name display
    document.getElementById('scale-name').textContent = scaleName;

    // Refresh tuning visualizations
    updateIntervalListUI();
    drawPitchCircle();

    // Update preset index in list
    currentPresetIndex = presetList.findIndex(p => p.name === presetName);
    if (currentPresetIndex < 0) currentPresetIndex = 0;
};

// ====================================================================
// INITIALIZATION
// ====================================================================

// Initialize interval list UI first
updateIntervalListUI();
drawPitchCircle();

// Initialize tuning mode UI (this will load appropriate preset)
updateTuningModeUI(tuningModeState.getNormalisedValue());

// Apply initial tuning to backend
applyTuning();

// v1.3.0: Initialize preset system
initializePresets();

// ====================================================================
// v1.8.0: ANALOG EQ UNIT (Effects Tab)
// ====================================================================

const eqUI = new AnalogEQUnitUI({
    container: document.getElementById('eq-panel'),
    paramPrefix: 'fx_eq_',
    getSliderState: Juce.getSliderState,
    getToggleState: Juce.getToggleState,
    getComboBoxState: Juce.getComboBoxState,
    showMeter: false,
    onReady: () => console.log('Analog EQ Unit initialized')
});
eqUI.initialize();

// ====================================================================
// v1.9.0: COMPRESSOR UNIT (Effects Tab, below EQ)
// ====================================================================

const compUI = new CompressorUnit({
    container: document.getElementById('comp-panel'),
    paramPrefix: 'fx_comp_',
    getSliderState: Juce.getSliderState,
    getToggleState: Juce.getToggleState
});
compUI.initialize();

// Store reference for GR meter updates
window.compressorUnitInstance = compUI;

// Listen for GR meter updates from C++
window.__JUCE__.backend.addEventListener("compressorGR", (grDB) => {
    if (compUI) {
        compUI.updateGainReduction(grDB);
    }
});

// ════════════════════════════════════════════════════════════════════
// INITIALIZATION — i18n, the popover and the ported tooltip renderer
// ════════════════════════════════════════════════════════════════════
//
// ORDER MATTERS. initI18n() runs LAST of the three so that every anchor it
// sweeps already exists: initializeSettingsPopover() binds #gear-btn and
// #settings-popover, initializeTooltips() binds #tooltip and #tips-toggle, and
// applyI18n()'s first pass then writes data-tip-title / data-tip onto all 18
// TIP_BINDINGS targets and textContent onto every [data-i18n] element in one go.
//
// setTooltipsEnabled() inside initializeTooltips() calls setLabel() before
// initI18n() has run. That is safe and deliberate: setLabel writes through
// trLabel, which reads the module-scope `uiLanguage` — 'en' until initI18n
// resolves the stored preference — and the element it keys is swept again a few
// lines later. Initialising i18n first would instead leave #tips-toggle unkeyed
// at the moment of the first sweep.
initializeSettingsPopover();
initializeTooltips();
initI18n();

console.log("O-Marimba UI initialized (v1.13.0 — EN/FR, measure-then-pin tooltips)");