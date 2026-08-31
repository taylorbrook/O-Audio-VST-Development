/*
   This file is part of O-Gain, an Ouaricon Audio plugin.
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
// app.js — O-Gain page controller (v1.3.0)
//
// EXTRACTED from index.html's inline <script type="module">, which through
// v1.2.1 was 447 lines ending at index.html:1150. The behaviour below the
// tooltip section is that script MOVED, not rewritten: the same listeners in
// the same order, with every visible string turned into a key.
//
// THE IMPORT SPECIFIER CHANGED WITH THE MODULE'S DEPTH, and only it. The page
// module was at the UI ROOT and reached the bridge as './js/juce/index.js'.
// This file is served from /js/app.js, so that becomes './juce/index.js'.
// Module specifiers resolve against the importing module's URL, and the URL is
// what PluginEditor::getResource() answers — not the filesystem.
//
// THE OLD TOOLTIP LAYER IS GONE, NOT DISABLED — and it was not a positioner.
//
//   THE PLAN SAYS THESE SEVEN PLUGINS CARRY "the never-measures positioner"
//   with a hard-coded tooltipHeight, tooltipWidth and two viewport literals.
//   O-Gain DOES NOT. Grepped and then rendered: it had NO tooltip JavaScript AT
//   ALL. Its hover help was a pure-CSS `[data-tooltip]::after` pseudo-element
//   with `content: attr(data-tooltip)`, `position: absolute`, `left: 50%`,
//   `transform: translateX(-50%)` and three static override classes
//   (data-tooltip-pos = bottom / left / right) that an author had to pick BY
//   HAND per anchor. So it did not measure wrong — it never positioned at all;
//   it hung a fixed-offset box off each anchor and relied on a human having
//   chosen the right one of four directions.
//
//   That is worse than the positioner the plan describes, not better. A
//   pseudo-element cannot be measured, cannot be flipped, cannot be clamped,
//   and cannot have its arrow re-aimed — and because it is a rendered box in
//   the layout, all twenty-three of them inflated the page's own scroll extent
//   to 435 x 540 inside a 350 x 500 frame, in English, at rest. Deleting them
//   is what brings the document back inside its own frame.
//
// After this commit:
//
//     grep -rn 'tooltipHeight\|tooltipWidth\|data-tooltip' \
//          plugins/O-Gain/Source/ui/public/
//
// returns nothing outside comments. Leaving both is how two renderers came to
// exist repo-wide in the first place.
// ============================================================================

import * as Juce from './juce/index.js';
import { getSliderState, getToggleState, getComboBoxState, getNativeFunction } from './juce/index.js';

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
// The settings popover (v1.3.0)
// ═══════════════════════════════════════════════════════════════════════════
//
// The gear takes the EXACT absolute slot the "?" help button occupied through
// v1.2.1 — `position: absolute; right: 8px; top: 0` inside the 22 px header,
// which is `position: relative` — so nothing on a 350 x 500 layout had to move
// to make room for it. The panel opens DOWNWARDS from the gear, unlike
// O-Marimba's, because this gear sits in the header rather than near the
// bottom edge.
//
// THIS PLUGIN HAS NO TOOLTIPS BRIDGE and this commit does not give it one.
// v1.2.1's "?" toggled a `tooltips-active` class on <body> and persisted
// nothing; the switch below is the same session-only preference wearing a
// caption. Adding a persisted hover-help preference is a processor-state change
// that does not belong in a commit about language.

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
// Tooltips — the measure-then-pin renderer (v1.3.0)
// ═══════════════════════════════════════════════════════════════════════════
//
// PORTED from O-ReverseDelay via O-IntonationPad and O-Marimba, replacing
// v1.2.1's pure-CSS `[data-tooltip]::after` layer ENTIRELY.
//
// What the port brings that a pseudo-element could not have: a title/body pair
// built from data-tip-title + data-tip rather than one flat string, a dwell
// delay so a tip does not fire on every crossing, a width RELEASED, MEASURED
// and PINNED before `left` is applied, an automatic vertical flip in place of
// four hand-picked data-tooltip-pos classes, a horizontal clamp, an arrow whose
// offset is recomputed AFTER the clamp so a clamped tip still points at its
// control, and — the reason this matters most at 350 px — a `position: fixed`
// surface that is OUT OF FLOW, so twenty-six tips no longer inflate the page's
// own scroll extent the way twenty-three `::after` boxes did.
//
// No ancestor of #tooltip establishes a containing block (no transform, filter
// or will-change on .container), so `fixed` resolves against the viewport as
// intended and is not clipped by body's `overflow: hidden`.
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

// The hover-help layer's master switch. SESSION-ONLY, exactly as v1.2.1's "?"
// was. Starts false, which is v1.2.1's observable behaviour unchanged.
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
    // because setupSliderKnob calls preventDefault in its own mousedown listener.
    document.addEventListener('pointerdown', () => {
        tooltipSuppressed = true;
        hideTooltip();
    }, true);

    document.addEventListener('pointerup', () => { tooltipSuppressed = false; }, true);

    console.log('[v1.3.0] Tooltips initialized');
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

    const frame = document.querySelector('.container');
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

    // Moving between children of the same control is not a real exit. Each meter
    // column wraps a caption, two bars and a dB readout, and each Learn-panel
    // cell wraps a caption and a value; crossing between those previously
    // flickered the surface off and back on.
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
    // afterwards re-wraps a 220 px tip into a narrow ribbon — and the squeezed
    // width then resolves `left` straight back against the right edge, so it
    // never recovers on later hovers. Release the width, measure from the left
    // edge, pin the result in px, and only then place. This bites HARDER at
    // 350 px than anywhere else in the suite: a 220 px cap in a 350 px viewport
    // leaves only 130 px of slack, so most anchors are "near an edge".
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
    // in anyway. MEASURED, not reasoned — and the measurement contradicted the
    // guess that was written here first. Every [data-tip] anchor was hovered in
    // both languages with these two lines deleted, twice: 52 tips with the
    // Learn panel open and 40 with it closed (which is when the meter columns
    // are at their full 383 px). NOT ONE left the 350 x 500 frame.
    //
    // The reason is the anchor geometry, not luck. The two tall anchors are the
    // meter columns, 44 x 383 at y = 36. "Above" has no room (36 < height + 8)
    // so both flip "below" to y = 427, and the tallest tip either of them
    // carries is 54.1 px — 481.1 px, still 18.9 px inside the frame. The
    // anchor tall enough that NEITHER placement fits does not exist here — yet.
    // The clamp costs two lines and there is ONE renderer repo-wide; a per-page
    // variant of it is how two renderers came to exist in the first place.
    //
    // Same case as O-Polystutter, O-Lyrica, O-SpectralShaper and O-Marimba;
    // unlike O-FreqPulse and O-IntonationPad, where deleting these two lines
    // puts real tips off-screen.
    //
    // THE SWEEP THAT REPORTED "not reproducible" IS NOT BLIND, which is the
    // claim that actually needed proving. Re-run with the HORIZONTAL clamp
    // deleted instead and the SAME sweep reports 26 off-frame tips — every
    // anchor that shows one, in both languages — the worst of them:
    //
    //     [fr] #phase-l-btn        89.3 px past the LEFT edge
    //     [fr] #ms-dec-btn         86.0 px past the RIGHT edge
    //     [en] #gear-btn           85.0 px past the RIGHT edge
    //     [en] #input-meter-group  80.0 px past the LEFT edge
    //
    // So the probe does detect a tip leaving the frame; the vertical clamp
    // simply has nothing to catch on this page. A 220 px cap in a 350 px
    // viewport is why the HORIZONTAL clamp is the load-bearing one here.
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
// <script type="module">, minus the "?" handler above and with every visible
// string turned into a key.
// ═══════════════════════════════════════════════════════════════════════════


// Disable context menu
document.addEventListener('contextmenu', (e) => { e.preventDefault(); return false; });

// =========================================================================
// SVG Knob Drawing
// =========================================================================
const KNOB_CX = 24, KNOB_CY = 24, KNOB_R = 18;
const START_ANGLE = 135;   // degrees (7 o'clock)
const END_ANGLE = 405;     // degrees (5 o'clock) = 135 + 270
const SWEEP = 270;

function degToRad(deg) { return deg * Math.PI / 180; }

function polarToXY(cx, cy, r, angleDeg) {
  const rad = degToRad(angleDeg);
  return { x: cx + r * Math.cos(rad), y: cy + r * Math.sin(rad) };
}

function describeArc(cx, cy, r, startAngle, endAngle) {
  const start = polarToXY(cx, cy, r, startAngle);
  const end = polarToXY(cx, cy, r, endAngle);
  const sweep = endAngle - startAngle;
  const largeArc = sweep > 180 ? 1 : 0;
  return `M ${start.x} ${start.y} A ${r} ${r} 0 ${largeArc} 1 ${end.x} ${end.y}`;
}

function drawKnob(trackId, fillId, dotId, normalizedValue) {
  const track = document.getElementById(trackId);
  const fill = document.getElementById(fillId);
  const dot = document.getElementById(dotId);
  if (!track || !fill || !dot) return;

  // Full arc (track)
  track.setAttribute('d', describeArc(KNOB_CX, KNOB_CY, KNOB_R, START_ANGLE, END_ANGLE));

  // Value arc
  const valueAngle = START_ANGLE + normalizedValue * SWEEP;
  if (normalizedValue > 0.005) {
    fill.setAttribute('d', describeArc(KNOB_CX, KNOB_CY, KNOB_R, START_ANGLE, valueAngle));
  } else {
    fill.setAttribute('d', '');
  }

  // Dot position
  const dotPos = polarToXY(KNOB_CX, KNOB_CY, KNOB_R, valueAngle);
  dot.setAttribute('cx', dotPos.x);
  dot.setAttribute('cy', dotPos.y);
}

// =========================================================================
// Parameter Definitions
// =========================================================================
const paramDefs = {
  gain_offset:  { min: -40.0, max: 40.0, unit: ' dB', format: v => (v >= 0 ? '+' : '') + v.toFixed(1) },
  trim:         { min: -6.0,  max: 6.0,  unit: ' dB', format: v => (v >= 0 ? '+' : '') + v.toFixed(1) },
  target_level: { min: -36.0, max: 0.0,  unit: ' dB', format: v => v.toFixed(1) }
};

function normToScaled(paramId, norm) {
  const p = paramDefs[paramId];
  return p.min + norm * (p.max - p.min);
}

function formatParam(paramId, norm) {
  const p = paramDefs[paramId];
  const val = normToScaled(paramId, norm);
  return p.format(val) + p.unit;
}

// =========================================================================
// Slider Knob Binding
// =========================================================================
const knobConfigs = {
  gain_offset:  { trackId: 'gain-track',   fillId: 'gain-fill',   dotId: 'gain-dot',   svgId: 'gain-knob' },
  trim:         { trackId: 'trim-track',   fillId: 'trim-fill',   dotId: 'trim-dot',   svgId: 'trim-knob' },
  target_level: { trackId: 'target-track', fillId: 'target-fill', dotId: 'target-dot', svgId: 'target-knob' }
};

// Shared drag state
let activeDrag = null;
document.addEventListener('mousemove', (e) => {
  if (!activeDrag) return;
  const delta = activeDrag.lastY - e.clientY;
  const current = activeDrag.state.getNormalisedValue();
  const sensitivity = e.shiftKey ? 600 : 200;
  activeDrag.state.setNormalisedValue(Math.max(0, Math.min(1, current + delta / sensitivity)));
  activeDrag.lastY = e.clientY;
});
document.addEventListener('mouseup', () => {
  if (activeDrag) {
    activeDrag.state.sliderDragEnded();
    activeDrag = null;
  }
});

function setupSliderKnob(paramId) {
  const config = knobConfigs[paramId];
  const state = getSliderState(paramId);
  if (!state) { console.error('No slider state for', paramId); return; }

  // Initialize
  const initNorm = state.getNormalisedValue();
  drawKnob(config.trackId, config.fillId, config.dotId, initNorm);
  updateSliderDisplay(paramId, initNorm);

  // Listen to C++ changes
  state.valueChangedEvent.addListener(() => {
    const norm = state.getNormalisedValue();
    drawKnob(config.trackId, config.fillId, config.dotId, norm);
    updateSliderDisplay(paramId, norm);
  });

  // Mouse drag
  const svg = document.getElementById(config.svgId);
  svg.addEventListener('mousedown', (e) => {
    state.sliderDragStarted();
    activeDrag = { state, lastY: e.clientY };
    e.preventDefault();
  });

  // Double-click to reset
  svg.addEventListener('dblclick', () => {
    const defaults = { gain_offset: 0.5, trim: 0.5, target_level: 0.5 };
    state.setNormalisedValue(defaults[paramId] || 0.5);
  });

  // Mouse wheel
  svg.addEventListener('wheel', (e) => {
    e.preventDefault();
    const step = e.shiftKey ? 0.005 : 0.02;
    const delta = e.deltaY < 0 ? step : -step;
    const current = state.getNormalisedValue();
    state.setNormalisedValue(Math.max(0, Math.min(1, current + delta)));
  }, { passive: false });
}

// All three writes below are READOUTS — untouched by D-03, no key, no
// setLabel. They pass `paramId` rather than re-typing its value, which is both
// the plainer spelling and the one that does not trip a gate shape worth
// naming:
//
//   check-i18n assertion 12 reads the WHOLE right-hand side of a
//   `.textContent =` and reports EVERY string literal in it that contains a
//   run of two letters. That is right for `on ? "On" : "Off"` and for
//   `el.dataset.label || "Delete"`, where the literal IS the value. It is
//   wrong for a literal in CALL-ARGUMENT position: `formatParam('trim', norm)`
//   renders "+0.0 dB", and 'trim' is a parameter ID that selects the format
//   and never reaches the screen. v1.3.0's first draft wrote it that way and
//   assertion 12 reported 'trim' and 'target_level' as unkeyed copy.
//
//   The assertion is NOT narrowed here. Grepped repo-wide, this shape occurs
//   at exactly TWO sites and both are these, so narrowing a COVERAGE gate on
//   n=1 evidence buys nothing and risks hiding a real unkeyed label later — an
//   over-report costs one glance, an under-report ships English. Named here so
//   the next author who writes `el.textContent = fmt('gain', v)` and sees
//   assertion 12 fire knows it is the gate's shape and not their code.
function updateSliderDisplay(paramId, norm) {
  if (paramId === 'gain_offset') {
    const val = normToScaled(paramId, norm);
    document.getElementById('gain-value').textContent = (val >= 0 ? '+' : '') + val.toFixed(1);
  } else if (paramId === 'trim') {
    document.getElementById('trim-value').textContent = formatParam(paramId, norm);
  } else if (paramId === 'target_level') {
    document.getElementById('target-value').textContent = formatParam(paramId, norm);
  }
}

// =========================================================================
// Toggle Button Binding
// =========================================================================
function setupToggle(btnId, paramId) {
  const btn = document.getElementById(btnId);
  const state = getToggleState(paramId);
  if (!state) { console.error('No toggle state for', paramId); return; }

  const updateVisual = () => {
    const val = state.getValue();
    btn.classList.toggle('active', val);
  };

  state.valueChangedEvent.addListener(updateVisual);
  btn.addEventListener('click', () => {
    state.setValue(!state.getValue());
  });
  updateVisual();
}

// =========================================================================
// ComboBox Binding (Mode Selectors)
// =========================================================================
function setupComboBox(selectorId, paramId) {
  const container = document.getElementById(selectorId);
  const state = getComboBoxState(paramId);
  if (!state) { console.error('No combobox state for', paramId); return; }

  const options = container.querySelectorAll('.mode-option');

  const updateVisual = () => {
    const idx = state.getChoiceIndex();
    options.forEach((opt, i) => {
      opt.classList.toggle('active', i === idx);
    });
  };

  state.valueChangedEvent.addListener(updateVisual);

  // Wait for properties to be available, then update
  state.propertiesChangedEvent.addListener(updateVisual);

  options.forEach((opt) => {
    opt.addEventListener('click', () => {
      const idx = parseInt(opt.dataset.index, 10);
      state.setChoiceIndex(idx);
    });
  });

  updateVisual();
}

// =========================================================================
// M/S Mode (ComboBox with separate buttons)
// =========================================================================
function setupMsMode() {
  const state = getComboBoxState('ms_mode');
  if (!state) { console.error('No combobox state for ms_mode'); return; }

  const buttons = [
    document.getElementById('ms-off-btn'),
    document.getElementById('ms-enc-btn'),
    document.getElementById('ms-dec-btn')
  ];

  const updateVisual = () => {
    const idx = state.getChoiceIndex();
    buttons.forEach((btn, i) => {
      btn.classList.toggle('ms-active', i === idx && i > 0);
      btn.classList.toggle('active', i === idx && i === 0);
    });
  };

  state.valueChangedEvent.addListener(updateVisual);
  state.propertiesChangedEvent.addListener(updateVisual);

  buttons.forEach((btn, i) => {
    btn.addEventListener('click', () => {
      state.setChoiceIndex(i);
    });
  });

  updateVisual();
}

// =========================================================================
// Learn Button (Native Function)
// =========================================================================
const toggleLearn = getNativeFunction('toggleLearn');

function setupLearnButton() {
  const btn = document.getElementById('learn-btn');
  btn.addEventListener('click', async () => {
    try {
      const result = await toggleLearn();
      // State will be updated via metering timer
    } catch (e) {
      console.error('toggleLearn error:', e);
    }
  });
}

// =========================================================================
// Metering (called from C++ via evaluateJavascript)
// =========================================================================
let currentMeterMode = 2; // default VU
let lastLearnState = 0;

// Metering scale constants (IN-06)
const METER_DB_MIN = -60;        // meter bottom (dB)
const METER_DB_MAX = 0;          // meter top / full scale (dB)
const CLIP_THRESHOLD_DB = -0.5;  // clip indicator lights when peak exceeds this (dBFS)

// Convert linear amplitude to dB
function ampToDb(amp) {
  if (amp <= 0.00001) return -100;
  return 20 * Math.log10(amp);
}

// Convert dB to meter percentage (0-100) across the METER_DB_MIN..MAX range
function dbToPercent(db) {
  return Math.max(0, Math.min(100, ((db - METER_DB_MIN) / (METER_DB_MAX - METER_DB_MIN)) * 100));
}

window.updateMeters = function(data) {
  // Determine which meter values to display based on meter_mode
  let inL, inR, outL, outR;

  switch (currentMeterMode) {
    case 0: // Peak
      inL = ampToDb(data.inputPeakL);
      inR = ampToDb(data.inputPeakR);
      outL = ampToDb(data.outputPeakL);
      outR = ampToDb(data.outputPeakR);
      break;
    case 1: // RMS
      inL = ampToDb(data.inputRmsL);
      inR = ampToDb(data.inputRmsR);
      outL = ampToDb(data.outputRmsL);
      outR = ampToDb(data.outputRmsR);
      break;
    case 2: // VU
      inL = ampToDb(data.vuLevelL);
      inR = ampToDb(data.vuLevelR);
      outL = ampToDb(data.outputRmsL);
      outR = ampToDb(data.outputRmsR);
      break;
    case 3: // LUFS — K-weighted momentary loudness (input, during Learn)
      // Momentary LUFS is only computed while Learn runs; when it is live,
      // drive both input meters from it (loudness is not per-channel). Outside
      // Learn there is no loudness value, so fall back to RMS (IN-03).
      if (data.momentaryLUFS > -99) {
        inL = inR = data.momentaryLUFS;
      } else {
        inL = ampToDb(data.inputRmsL);
        inR = ampToDb(data.inputRmsR);
      }
      outL = ampToDb(data.outputRmsL);
      outR = ampToDb(data.outputRmsR);
      break;
    default:
      inL = inR = outL = outR = -100;
  }

  // Update meter bars
  document.getElementById('input-meter-l').style.height = dbToPercent(inL) + '%';
  document.getElementById('input-meter-r').style.height = dbToPercent(inR) + '%';
  document.getElementById('output-meter-l').style.height = dbToPercent(outL) + '%';
  document.getElementById('output-meter-r').style.height = dbToPercent(outR) + '%';

  // Clip indicators (peak above CLIP_THRESHOLD_DB)
  const inPeakL = ampToDb(data.inputPeakL);
  const inPeakR = ampToDb(data.inputPeakR);
  const outPeakL = ampToDb(data.outputPeakL);
  const outPeakR = ampToDb(data.outputPeakR);

  document.getElementById('input-clip-l').classList.toggle('clipping', inPeakL > CLIP_THRESHOLD_DB);
  document.getElementById('input-clip-r').classList.toggle('clipping', inPeakR > CLIP_THRESHOLD_DB);
  document.getElementById('output-clip-l').classList.toggle('clipping', outPeakL > CLIP_THRESHOLD_DB);
  document.getElementById('output-clip-r').classList.toggle('clipping', outPeakR > CLIP_THRESHOLD_DB);

  // dB readouts under meters
  const maxInDb = Math.max(inL, inR);
  const maxOutDb = Math.max(outL, outR);
  document.getElementById('input-db-label').textContent = maxInDb > -99 ? maxInDb.toFixed(0) : '-inf';
  document.getElementById('output-db-label').textContent = maxOutDb > -99 ? maxOutDb.toFixed(0) : '-inf';

  // Apply meter bar color gradient based on level
  applyMeterColor('input-meter-l', inL);
  applyMeterColor('input-meter-r', inR);
  applyMeterColor('output-meter-l', outL);
  applyMeterColor('output-meter-r', outR);

  // Learn state
  const learnBtn = document.getElementById('learn-btn');
  const learnInfo = document.getElementById('learn-info');

  if (data.learnState !== lastLearnState) {
    lastLearnState = data.learnState;
    learnBtn.classList.remove('learning', 'complete');
    if (data.learnState === 1) {
      learnBtn.classList.add('learning');
      setLabel(learnBtn, 'ui.learning');
      learnInfo.classList.add('visible');
    } else if (data.learnState === 2) {
      learnBtn.classList.add('complete');
      // confidence 0 at completion = Learn refused to write a gain (silence / too
      // quiet — see kMaxLearnBoostDB). Show that instead of a misleading "DONE":
      // no gain was applied, so the user should re-run Learn over a louder section.
      //
      // if/else rather than v1.2.1's ternary. check-i18n assertion 13 rejects a
      // conditional inside a setLabel argument, and the reason it does is that a
      // ternary is where an inflection hides: each arm has to be lifted out so it
      // is a plain string literal a gate can resolve against LABELS.
      if (data.learnConfidence === 0) setLabel(learnBtn, 'ui.tooQuiet');
      else                            setLabel(learnBtn, 'ui.done');
      learnInfo.classList.add('visible');
    } else {
      setLabel(learnBtn, 'ui.learn');
      learnInfo.classList.remove('visible');
    }
  }

  // Learn info panel updates
  if (data.learnState >= 1) {
    const momLufs = data.momentaryLUFS > -99 ? data.momentaryLUFS.toFixed(1) + ' LUFS' : '-- LUFS';
    const stLufs = data.shortTermLUFS > -99 ? data.shortTermLUFS.toFixed(1) + ' LUFS' : '-- LUFS';
    const intLufs = data.integratedLUFS > -99 ? data.integratedLUFS.toFixed(1) + ' LUFS' : '-- LUFS';
    const sp = data.samplePeakDBFS > -99 ? data.samplePeakDBFS.toFixed(1) + ' dBFS' : '-- dBFS';

    document.getElementById('lufs-momentary').textContent = momLufs;
    document.getElementById('lufs-short-term').textContent = stLufs;
    document.getElementById('lufs-integrated').textContent = intLufs;
    document.getElementById('lufs-true-peak').textContent = sp;
    document.getElementById('learn-elapsed').textContent = data.learnElapsedSeconds.toFixed(1) + 's';

    // The confidence VERDICT is copy, not a readout — see the long note in
    // js/i18n.js's I18N_EXEMPT block for why this call goes the opposite way
    // from O-Marimba's six timbre words. Three branches, three plain-literal
    // keys, no ternary and no computed key (assertion 13). The 0 branch drops
    // the key entirely: "--" is language-neutral, and owning it would leave a
    // key on the element for a French pass to repaint over.
    const confEl = document.getElementById('learn-confidence');
    const confClasses = ['', 'confidence-low', 'confidence-medium', 'confidence-high'];
    if (data.learnConfidence === 1)      setLabel(confEl, 'ui.confLow');
    else if (data.learnConfidence === 2) setLabel(confEl, 'ui.confMed');
    else if (data.learnConfidence === 3) setLabel(confEl, 'ui.confHigh');
    else {
      delete confEl.dataset.i18n;
      confEl.dataset.label = '--';
      confEl.textContent   = '--';
    }
    // className is rewritten AFTER setLabel deliberately: setLabel writes
    // data-i18n and textContent and never touches class, so the two do not race.
    confEl.className = 'learn-info-value ' + (confClasses[data.learnConfidence] || '');
  }
};

function applyMeterColor(elementId, db) {
  const el = document.getElementById(elementId);
  if (db > -6) {
    el.style.background = 'linear-gradient(to top, #5a7a4e, #6B8E4E, #C9A27B, #a04030)';
  } else if (db > -18) {
    el.style.background = 'linear-gradient(to top, #5a7a4e, #6B8E4E, #C9A27B)';
  } else {
    el.style.background = 'linear-gradient(to top, #5a7a4e, #6B8E4E, #8BA870)';
  }
}

// Track meter mode changes to update display
function watchMeterMode() {
  const state = getComboBoxState('meter_mode');
  if (!state) return;

  state.valueChangedEvent.addListener(() => {
    currentMeterMode = state.getChoiceIndex();
  });
  state.propertiesChangedEvent.addListener(() => {
    currentMeterMode = state.getChoiceIndex();
  });
}

// =========================================================================
// Initialize
// =========================================================================
window.addEventListener('DOMContentLoaded', () => {
  // Slider knobs
  setupSliderKnob('gain_offset');
  setupSliderKnob('trim');
  setupSliderKnob('target_level');

  // Toggle buttons
  setupToggle('phase-l-btn', 'phase_invert_l');
  setupToggle('phase-r-btn', 'phase_invert_r');
  setupToggle('swap-btn', 'channel_swap');
  setupToggle('mono-btn', 'mono_sum');

  // ComboBox mode selectors
  setupComboBox('measurement-mode-selector', 'measurement_mode');
  setupComboBox('meter-mode-selector', 'meter_mode');
  setupMsMode();

  // Watch meter mode for display
  watchMeterMode();

  // Learn button
  setupLearnButton();

  // v1.3.0: the settings popover, the language pair, and the ported tooltip
  // renderer, replacing v1.2.1's three-line "?" handler. initI18n() runs BEFORE
  // initializeTooltips() so applyI18n()'s first pass has already written
  // data-tip-title / data-tip onto all twenty-six anchors before the first
  // hover can read them.
  initializeSettingsPopover();
  initI18n();
  initializeTooltips();

  console.log('O-Gain v1.3.0 UI loaded');
});
