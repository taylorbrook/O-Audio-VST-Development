/*
   This file is part of O-IntonationPad, an Ouaricon Audio plugin.
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
// app.js — O-IntonationPad page controller (v2.9.0)
//
// NEW FILE, AND THE FILE IS THE POINT. Through v2.8.4 this plugin's controller
// was a single 1,435-line <script type="module"> inline in index.html, and the
// tooltip runtime was 60 of those lines. That is the same shape O-Polystutter
// had before Stage J, and it is how the second tooltip renderer stayed
// invisible long enough to spread to seven plugins. Extracting it here gives
// the served tree the shape every other canon-v2 plugin has: index.html is
// markup, js/app.js is the controller, js/i18n.js is the copy.
//
// js/tuning-panel.js and js/constants.js are UNCHANGED IN SHAPE and still
// loaded on their own. tuning-panel.js is a plugin-local copy of the
// scala-tuning-engine module's panel; this commit deletes its `data-tooltip`
// attributes and keys its captions, and does NOT restructure it or re-vendor it.
//
// THE SECOND TOOLTIP RENDERER IS DELETED, NOT DISABLED. v2.8.4's positioner
// never measured — it read `if (top + 60 > window.innerHeight)` against a
// tooltip height it never took, and `if (left + 220 > window.innerWidth)`
// against a width it never took, and centred on `rect.left + rect.width / 2 -
// 110` with the same 220 baked in a third time. It was already wrong before
// French made any string taller. What replaces it is the measure-then-pin
// runtime from O-ReverseDelay, carried in verbatim including the vertical
// clamp added on O-FreqPulse.
// ============================================================================

import * as Juce from './juce/index.js';
import { NOTE_NAMES } from './constants.js';
import { LANGUAGES, I18N, LABELS, TIP_BINDINGS, tr } from './i18n.js';


// ═══════════════════════════════════════════════════════════════════════════
// The i18n runtime — canon v2, verbatim from scripts/i18n-canon.js
// ═══════════════════════════════════════════════════════════════════════════
//
// CONTEXT.md accepts 43 hand-copies of this block as a deliberate cost, matching
// the repo's existing no-shared-UI-module convention. check-i18n.js assertion 6
// pulls this region out, strips comments, normalises whitespace and byte-
// compares it against the canon, so a copy that drifts fails a gate instead of
// drifting quietly. Do not "improve" anything between here and the close of
// initI18n(): change the canon, then every copy.
//
// initI18n() is CALLED from the DOMContentLoaded handler further down this
// file, never at module top level — a top-level statement touching a lower
// `let` throws a TDZ error out of module evaluation and takes every later
// initializer on the page with it (pattern_module_toplevel_init_tdz).

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
// The settings popover (v2.9.0)
// ═══════════════════════════════════════════════════════════════════════════
//
// The gear takes the exact place the floating "?" occupied through v2.8.4 — the
// last slot of the tab row — so nothing on a packed 800 x 500 layout had to move
// to make room for it. The hover-help toggle moves inside, beside the language
// selector: one place for the two things that decide what the hover help says
// and whether it says it at all.

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
// Tooltips — the measure-then-pin renderer (v2.9.0)
// ═══════════════════════════════════════════════════════════════════════════
//
// PORTED from O-ReverseDelay via O-Polystutter, replacing this plugin's own
// positioner ENTIRELY. The grep for tooltipHeight, tooltipWidth and
// data-tooltip over the served tree returns nothing outside comments.
//
// What the port brings that v2.8.4's positioner did not have: a title/body pair
// built from data-tip-title + data-tip rather than one flat string, a dwell
// delay so a tip does not fire on every crossing, a width RELEASED, MEASURED
// and PINNED before `left` is applied, an arrow whose offset is recomputed
// AFTER the horizontal clamp so a clamped tip still points at its control, and
// viewport-relative arithmetic that matches the fixed-position box the browser
// actually lays out instead of the three hard-coded numbers it replaces.
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

// The hover-help layer's master switch. SESSION-ONLY, exactly as in v2.8.4:
// this plugin has never had a getTooltipsEnabled / setTooltipsEnabled native
// pair, and adding one is a processor-state change that does not belong in a
// commit about language. Starts false, which is v2.8.4's behaviour unchanged.
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
    // because setupKnob() calls preventDefault in its own mousedown listener.
    document.addEventListener('pointerdown', () => {
        tooltipSuppressed = true;
        hideTooltip();
    }, true);

    document.addEventListener('pointerup', () => { tooltipSuppressed = false; }, true);

    console.log('[v2.9.0] Tooltips initialized');
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
    // afterwards re-wraps a 220 px tip into a narrow ribbon — and the squeezed
    // width then resolves `left` straight back against the right edge, so it
    // never recovers on later hovers. Release the width, measure from the left
    // edge, pin the result in px, and only then place.
    //
    // The pinned width is the FRACTIONAL getBoundingClientRect().width, not the
    // integer offsetWidth: 208.48 rounds to 208, and pinning that makes the box
    // 0.48 px narrower than its own shrink-to-fit, pushing the last word onto a
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
    // IT IS INDEPENDENTLY REPRODUCIBLE ON THIS PAGE, unlike on O-Polystutter,
    // O-Lyrica and O-SpectralShaper where it was carried in on principle. Delete
    // these two lines, re-hover all 184 tips this page renders across four tabs
    // and two languages, and two of them leave the 500 px frame:
    //
    //     [fr] #tuning-container  "Gamme"        53 px below the bottom edge
    //     [fr] #interval-list     "Intervalles"  67 px below the bottom edge
    //
    // Both are tall tuning-tab anchors — a full-height tab pane and the interval
    // column inside it — so neither above nor below leaves room, and both fire
    // only in FRENCH, where the body is a line taller. English is clean either
    // way, which is exactly how this would have shipped unnoticed.
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

// JUCE Parameter States
let voiceCountState, complexityState, keyRootState, voicingModeState, stereoSpreadState, spacingState, inversionState;
let wavetableBankState, wavetablePosState, lfoRateState, lfoDepthState, timingRandomState, detuneRandomState;
let wavetableBank2State, wavetablePos2State, lfoRate2State, lfoDepth2State, gainAState, gainBState;
let attackTimeState, decayTimeState, sustainLevelState, releaseTimeState, filterCutoffState, filterLfoDepthState, velocityToFilterState, masterVolumeState;
let chorusRateState, chorusDepthState, chorusMixState;
let delayTimeState, delayFeedbackState, delayModeState, delayMixState;
let eqLowGainState, eqMidGainState, eqMidFreqState, eqHighGainState;
let reverbSizeState, reverbDampState, reverbPredelayState, reverbMixState;
let chorusBypassState, delayBypassState, eqBypassState, reverbBypassState;

// ====================================================================
// KNOB FACTORY + WAVETABLE BANK DATA
// ====================================================================

const WAVETABLE_BANKS = [
    'JI Harmonic', 'Warm Analog', 'Choir', 'Strings', 'Glass', 'Evolving',
    'Organ', 'Ethereal', 'Dark Matter', 'Sine', 'Square', 'Triangle',
    'Spectral Cloud', 'Metallic Res.', 'Formant Vowel', 'Warm Sub',
    'Soft Flute', 'Velvet Pad', 'Whisper', 'Deep Haze'
];

// size: 'small' = 44px (r=18), default = 52px (r=22)
// THE CAPTION IS APPLIED BY THE CALLER, always with a PLAIN STRING LITERAL.
// A { id, labelKey } table passed through a loop reads better and fails twice:
// check-i18n assertion 13 rejects a computed setLabel key, and assertion 15
// counts only a literal as a live reference, so all thirty-seven captions would
// have reported DEAD while the gate simultaneously said the key was uncheckable.
//
// createElement / createElementNS throughout, not innerHTML — a caption keyed by
// a sweep must be an ELEMENT the sweep can find, and building the knob from a
// markup string is how the old caption ended up inside a template no language
// sweep could own.
const SVG_NS = 'http://www.w3.org/2000/svg';

function makeKnob(id, size, labelStyle) {
    const isSmall = size === 'small';
    const vb = isSmall ? 44 : 52;
    const c = isSmall ? 22 : 26;
    const r = isSmall ? 18 : 22;
    const da = (2 * Math.PI * r * 0.75).toFixed(2);

    const container = document.createElement('div');
    // The size class rides the CONTAINER as well as the visual: the container is
    // width-pinned per size class so a French caption cannot move the row, and
    // the pin has to be selectable from the container itself.
    container.className = 'knob-container' + (isSmall ? ' small' : '');

    const knob = document.createElement('div');
    knob.className = 'knob';
    knob.id = id + 'Knob';

    const visual = document.createElement('div');
    visual.className = 'knob-visual' + (isSmall ? ' small' : '');

    const svg = document.createElementNS(SVG_NS, 'svg');
    svg.setAttribute('viewBox', `0 0 ${vb} ${vb}`);

    const track = document.createElementNS(SVG_NS, 'circle');
    track.setAttribute('class', 'knob-track');
    track.setAttribute('cx', c);
    track.setAttribute('cy', c);
    track.setAttribute('r', r);

    const vine = document.createElementNS(SVG_NS, 'circle');
    vine.setAttribute('class', 'knob-vine');
    vine.setAttribute('id', id + 'Vine');
    vine.setAttribute('cx', c);
    vine.setAttribute('cy', c);
    vine.setAttribute('r', r);
    vine.setAttribute('stroke-dasharray', da);
    vine.setAttribute('stroke-dashoffset', da);

    svg.appendChild(track);
    svg.appendChild(vine);
    visual.appendChild(svg);
    knob.appendChild(visual);
    container.appendChild(knob);

    const labelEl = document.createElement('div');
    labelEl.className = 'knob-label';
    if (labelStyle) labelEl.style.cssText = labelStyle;
    container.appendChild(labelEl);

    const valueEl = document.createElement('div');
    valueEl.className = 'knob-value';
    valueEl.id = id + 'Value';
    container.appendChild(valueEl);

    return container;
}

// Create a knob, append it to a row, and hand its CAPTION back so the call site
// can key it: `setLabel(addKnob('voice-knobs', 'voiceCount'), 'label.voices')`.
// Returns null when the row is missing; setLabel's own first line guards that.
function addKnob(row, id, size, labelStyle) {
    const parent = typeof row === 'string' ? document.getElementById(row) : row;
    if (!parent) return null;
    const knob = makeKnob(id, size, labelStyle);
    parent.appendChild(knob);
    return knob.querySelector('.knob-label');
}

function makeWavetableDropdown(selectId, canvasId) {
    const wrapper = document.createElement('div');
    wrapper.className = 'dropdown-container';
    const lbl = document.createElement('div');
    lbl.className = 'knob-label';
    lbl.style.cssText = 'font-weight: bold; color: #2C3E10;';
    const sel = document.createElement('select');
    sel.className = 'dropdown';
    sel.id = selectId;
    sel.style.width = '120px';
    WAVETABLE_BANKS.forEach((name, i) => {
        const opt = document.createElement('option');
        opt.value = i;
        opt.textContent = name;
        sel.appendChild(opt);
    });
    const canvasWrap = document.createElement('div');
    canvasWrap.className = 'wt-canvas-wrap';
    const canvas = document.createElement('canvas');
    canvas.className = 'wt-canvas';
    canvas.id = canvasId;
    canvas.width = 240;
    canvas.height = 136;
    canvasWrap.appendChild(canvas);
    wrapper.appendChild(lbl);
    wrapper.appendChild(sel);
    wrapper.appendChild(canvasWrap);
    return wrapper;
}

// Same contract as addKnob: append the dropdown, hand back its caption so the
// call site keys it with a literal.
function addWavetableDropdown(row, selectId, canvasId) {
    const wrapper = makeWavetableDropdown(selectId, canvasId);
    row.appendChild(wrapper);
    return wrapper.querySelector('.knob-label');
}

// ====================================================================
// Wavetable Waveform Display
// ====================================================================

class WavetableDisplay {
    constructor(canvasId) {
        this.canvas = document.getElementById(canvasId);
        this.ctx = this.canvas.getContext('2d');
        this.samples = [];
        this.w = 0;
        this.h = 0;
        this.getFrame = Juce.getNativeFunction('getWavetableFrameForPosition');
        this._fetching = false;
        this._pendingFetch = null;
        // Use ResizeObserver to size canvas when layout is computed
        this._resizeObserver = new ResizeObserver(() => {
            this.resizeCanvas();
            if (this.samples.length) this.draw();
        });
        this._resizeObserver.observe(this.canvas);
    }

    resizeCanvas() {
        const dpr = window.devicePixelRatio || 1;
        const w = this.canvas.clientWidth;
        const h = this.canvas.clientHeight;
        if (w === 0 || h === 0) return;
        this.canvas.width = w * dpr;
        this.canvas.height = h * dpr;
        this.ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
        this.w = w;
        this.h = h;
    }

    async fetchAndDraw(bankIndex, position) {
        if (typeof this.getFrame !== 'function') {
            this.getFrame = Juce.getNativeFunction('getWavetableFrameForPosition');
        }
        if (typeof this.getFrame !== 'function') return;
        // Coalesce overlapping calls — keep only the latest request
        if (this._fetching) {
            this._pendingFetch = { bankIndex, position };
            return;
        }
        this._fetching = true;
        try {
            const result = await this.getFrame(bankIndex, position);
            if (result) {
                this.samples = typeof result === 'string' ? JSON.parse(result) : result;
                this.draw();
            }
        } catch(e) { console.error('Wavetable fetch error:', e); }
        this._fetching = false;
        // Process most recent pending request (drops intermediate ones)
        if (this._pendingFetch) {
            const pend = this._pendingFetch;
            this._pendingFetch = null;
            this.fetchAndDraw(pend.bankIndex, pend.position);
        }
    }

    draw() {
        if (this.w === 0 || this.h === 0) this.resizeCanvas();
        const { ctx, w, h, samples } = this;
        if (!samples.length || w === 0 || h === 0) return;
        const mid = h / 2;

        ctx.clearRect(0, 0, w, h);

        // Dark background
        ctx.fillStyle = '#1a1a1a';
        ctx.fillRect(0, 0, w, h);

        // Center line
        ctx.strokeStyle = 'rgba(107, 142, 35, 0.2)';
        ctx.lineWidth = 1;
        ctx.beginPath();
        ctx.moveTo(0, mid);
        ctx.lineTo(w, mid);
        ctx.stroke();

        // Waveform fill (green gradient matching theme)
        const grad = ctx.createLinearGradient(0, 0, 0, h);
        grad.addColorStop(0, 'rgba(107, 142, 35, 0.3)');
        grad.addColorStop(0.5, 'rgba(107, 142, 35, 0.08)');
        grad.addColorStop(1, 'rgba(107, 142, 35, 0.0)');

        ctx.beginPath();
        for (let i = 0; i < samples.length; i++) {
            const x = (i / (samples.length - 1)) * w;
            const y = mid - samples[i] * mid * 0.85;
            i === 0 ? ctx.moveTo(x, y) : ctx.lineTo(x, y);
        }
        ctx.lineTo(w, mid);
        ctx.lineTo(0, mid);
        ctx.closePath();
        ctx.fillStyle = grad;
        ctx.fill();

        // Waveform stroke (green)
        ctx.beginPath();
        for (let i = 0; i < samples.length; i++) {
            const x = (i / (samples.length - 1)) * w;
            const y = mid - samples[i] * mid * 0.85;
            i === 0 ? ctx.moveTo(x, y) : ctx.lineTo(x, y);
        }
        ctx.strokeStyle = '#8BC34A';
        ctx.lineWidth = 1.5;
        ctx.stroke();

        // Border
        ctx.strokeStyle = '#5C4033';
        ctx.lineWidth = 1;
        ctx.strokeRect(0, 0, w, h);
    }
}

function wireWavetableDisplay(display, posState, bankState) {
    function refresh() {
        const bankIdx = Math.round(bankState.getChoiceIndex());
        const pos = posState.getNormalisedValue();
        display.fetchAndDraw(bankIdx, pos);
    }

    posState.valueChangedEvent.addListener(refresh);
    bankState.valueChangedEvent.addListener(refresh);

    // Initial draw with retry
    let retries = 0;
    async function tryInitialDraw() {
        const bankIdx = Math.round(bankState.getChoiceIndex());
        const pos = posState.getNormalisedValue();
        await display.fetchAndDraw(bankIdx, pos);
        retries++;
        if (display.samples.length === 0 && retries < 8) {
            setTimeout(tryInitialDraw, 500);
        }
    }
    setTimeout(tryInitialDraw, 500);
}

document.addEventListener('DOMContentLoaded', () => {
    console.log('JUCE backend:', window.__JUCE__?.backend);

    initializeSettingsPopover();

    // v2.6.0: Dice randomization button
    {
        const diceBtn = document.getElementById('diceBtn');
        const diceMenu = document.getElementById('diceMenu');
        let menuOpen = false;

        diceBtn.addEventListener('click', (e) => {
            e.stopPropagation();
            menuOpen = !menuOpen;
            diceMenu.classList.toggle('open', menuOpen);
        });

        document.addEventListener('click', () => {
            if (menuOpen) {
                menuOpen = false;
                diceMenu.classList.remove('open');
            }
        });

        diceMenu.addEventListener('click', async (e) => {
            const item = e.target.closest('.dice-menu-item');
            if (!item) return;
            e.stopPropagation();
            const mode = item.dataset.mode;

            // Roll animation
            diceBtn.classList.add('rolling');
            setTimeout(() => diceBtn.classList.remove('rolling'), 400);

            // Close menu
            menuOpen = false;
            diceMenu.classList.remove('open');

            // Call native function
            if (window.__JUCE__?.backend) {
                await Juce.getNativeFunction('randomizeParameters')(mode);
            }
        });
    }

    // Build knob DOM. THIRTY-SEVEN EXPLICIT LINES, not a loop over a
    // { id, labelKey } table: a computed setLabel key fails check-i18n
    // assertion 13, and assertion 15 counts only a literal as a live reference,
    // so a table would have reported every caption below as a dead translation
    // while the gate said in the same breath that the key was uncheckable.
    setLabel(addKnob('voice-knobs', 'voiceCount'),   'label.voices');
    setLabel(addKnob('voice-knobs', 'complexity'),   'label.complexity');
    setLabel(addKnob('voice-knobs', 'stereoSpread'), 'label.spread');
    setLabel(addKnob('voice-knobs', 'spacing'),      'label.spacing');
    setLabel(addKnob('voice-knobs', 'inversion'),    'label.inversion');

    // OSC A: dropdown + waveform canvas + knobs
    const oscARow = document.getElementById('oscA-row');
    setLabel(addWavetableDropdown(oscARow, 'wavetableBankSelect', 'canvasOscA'), 'label.oscA');
    setLabel(addKnob(oscARow, 'wavetablePos', 'small'), 'label.pos');
    setLabel(addKnob(oscARow, 'lfoRate',      'small'), 'label.rate');
    setLabel(addKnob(oscARow, 'lfoDepth',     'small'), 'label.depth');
    setLabel(addKnob(oscARow, 'gainA',        'small', 'color: #6B8E23;'), 'label.gain');

    // OSC B: dropdown + waveform canvas + knobs
    const oscBRow = document.getElementById('oscB-row');
    setLabel(addWavetableDropdown(oscBRow, 'wavetableBank2Select', 'canvasOscB'), 'label.oscB');
    setLabel(addKnob(oscBRow, 'wavetablePos2', 'small'), 'label.pos');
    setLabel(addKnob(oscBRow, 'lfoRate2',      'small'), 'label.rate');
    setLabel(addKnob(oscBRow, 'lfoDepth2',     'small'), 'label.depth');
    setLabel(addKnob(oscBRow, 'gainB',         'small', 'color: #6B8E23;'), 'label.gain');

    setLabel(addKnob('synth-env-knobs', 'attackTime',       'small'), 'label.attack');
    setLabel(addKnob('synth-env-knobs', 'decayTime',        'small'), 'label.decay');
    setLabel(addKnob('synth-env-knobs', 'sustainLevel',     'small'), 'label.sustain');
    setLabel(addKnob('synth-env-knobs', 'releaseTime',      'small'), 'label.release');
    setLabel(addKnob('synth-env-knobs', 'filterCutoff',     'small'), 'label.filter');
    setLabel(addKnob('synth-env-knobs', 'filterLfoDepth',   'small'), 'label.fltLfo');
    setLabel(addKnob('synth-env-knobs', 'velocityToFilter', 'small'), 'label.velFlt');
    setLabel(addKnob('synth-env-knobs', 'masterVolume',     'small'), 'label.volume');

    setLabel(addKnob('synth-random-knobs', 'timingRandom', 'small'), 'label.timing');
    setLabel(addKnob('synth-random-knobs', 'detuneRandom', 'small'), 'label.detune');

    // Effects knobs
    setLabel(addKnob('chorus-knobs', 'chorusRate',  'small'), 'label.rate');
    setLabel(addKnob('chorus-knobs', 'chorusDepth', 'small'), 'label.depth');
    setLabel(addKnob('chorus-knobs', 'chorusMix',   'small'), 'label.mix');
    {
        const delayRow = document.getElementById('delay-knobs');
        setLabel(addKnob(delayRow, 'delayTime',     'small'), 'label.time');
        setLabel(addKnob(delayRow, 'delayFeedback', 'small'), 'label.feedback');
        // delayMode: discrete choice → dropdown (not a knob)
        const modeWrap = document.createElement('div');
        modeWrap.className = 'dropdown-container';
        modeWrap.id = 'delayModeContainer';
        const modeLbl = document.createElement('div');
        modeLbl.className = 'knob-label';
        setLabel(modeLbl, 'label.mode');
        const modeSel = document.createElement('select');
        modeSel.className = 'dropdown';
        modeSel.id = 'delayModeSelect';
        modeSel.style.width = '90px';
        ['Normal', 'PingPong'].forEach((name, i) => {
            const opt = document.createElement('option');
            opt.value = i;
            opt.textContent = name;
            modeSel.appendChild(opt);
        });
        modeWrap.appendChild(modeLbl);
        modeWrap.appendChild(modeSel);
        delayRow.appendChild(modeWrap);
        setLabel(addKnob(delayRow, 'delayMix', 'small'), 'label.mix');
    }
    setLabel(addKnob('eq-knobs', 'eqLowGain',  'small'), 'label.low');
    setLabel(addKnob('eq-knobs', 'eqMidGain',  'small'), 'label.mid');
    setLabel(addKnob('eq-knobs', 'eqMidFreq',  'small'), 'label.midFreq');
    setLabel(addKnob('eq-knobs', 'eqHighGain', 'small'), 'label.high');

    setLabel(addKnob('reverb-knobs', 'reverbSize',     'small'), 'label.size');
    setLabel(addKnob('reverb-knobs', 'reverbDamp',     'small'), 'label.damp');
    setLabel(addKnob('reverb-knobs', 'reverbPredelay', 'small'), 'label.preDly');
    setLabel(addKnob('reverb-knobs', 'reverbMix',      'small'), 'label.mix');

    // Initialize parameter states
    voiceCountState = Juce.getSliderState('voiceCount');
    complexityState = Juce.getSliderState('complexity');
    keyRootState = Juce.getComboBoxState('keyRoot');
    voicingModeState = Juce.getComboBoxState('voicingMode');
    stereoSpreadState = Juce.getSliderState('stereoSpread');
    spacingState = Juce.getSliderState('spacing');
    inversionState = Juce.getSliderState('inversion');
    wavetableBankState = Juce.getComboBoxState('wavetableBank');
    wavetablePosState = Juce.getSliderState('wavetablePos');
    wavetableBank2State = Juce.getComboBoxState('wavetableBank2');
    wavetablePos2State = Juce.getSliderState('wavetablePos2');
    gainAState = Juce.getSliderState('gainA');
    gainBState = Juce.getSliderState('gainB');
    lfoRateState = Juce.getSliderState('lfoRate');
    lfoRate2State = Juce.getSliderState('lfoRate2');
    lfoDepthState = Juce.getSliderState('lfoDepth');
    lfoDepth2State = Juce.getSliderState('lfoDepth2');
    timingRandomState = Juce.getSliderState('timingRandom');
    detuneRandomState = Juce.getSliderState('detuneRandom');
    attackTimeState = Juce.getSliderState('attackTime');
    decayTimeState = Juce.getSliderState('decayTime');
    sustainLevelState = Juce.getSliderState('sustainLevel');
    releaseTimeState = Juce.getSliderState('releaseTime');
    filterCutoffState = Juce.getSliderState('filterCutoff');
    filterLfoDepthState = Juce.getSliderState('filterLfoDepth');
    velocityToFilterState = Juce.getSliderState('velocityToFilter');
    masterVolumeState = Juce.getSliderState('masterVolume');

    chorusRateState = Juce.getSliderState('chorusRate');
    chorusDepthState = Juce.getSliderState('chorusDepth');
    chorusMixState = Juce.getSliderState('chorusMix');
    delayTimeState = Juce.getSliderState('delayTime');
    delayFeedbackState = Juce.getSliderState('delayFeedback');
    delayModeState = Juce.getComboBoxState('delayMode');
    delayMixState = Juce.getSliderState('delayMix');
    eqLowGainState = Juce.getSliderState('eqLowGain');
    eqMidGainState = Juce.getSliderState('eqMidGain');
    eqMidFreqState = Juce.getSliderState('eqMidFreq');
    eqHighGainState = Juce.getSliderState('eqHighGain');
    reverbSizeState = Juce.getSliderState('reverbSize');
    reverbDampState = Juce.getSliderState('reverbDamp');
    reverbPredelayState = Juce.getSliderState('reverbPredelay');
    reverbMixState = Juce.getSliderState('reverbMix');
    chorusBypassState = Juce.getToggleState('chorusBypass');
    delayBypassState = Juce.getToggleState('delayBypass');
    eqBypassState = Juce.getToggleState('eqBypass');
    reverbBypassState = Juce.getToggleState('reverbBypass');

    // Setup tab switching
    setupTabs();

    // v2.7.0: Setup preset system
    setupPresetSystem();

    // Setup all knobs
    setupKnob('voiceCount', voiceCountState, 2, 12, '', v => Math.round(v));
    setupKnob('complexity', complexityState, 0, 100, '%', v => Math.round(v));
    setupKnob('stereoSpread', stereoSpreadState, 0, 100, '%', v => Math.round(v));
    setupKnob('spacing', spacingState, 0, 100, '%', v => Math.round(v));
    setupKnob('inversion', inversionState, 0, 100, '%', v => Math.round(v));
    setupKnob('wavetablePos', wavetablePosState, 0, 100, '%', v => Math.round(v));
    setupKnob('wavetablePos2', wavetablePos2State, 0, 100, '%', v => Math.round(v));
    setupKnob('gainA', gainAState, 0, 100, '%', v => Math.round(v));
    setupKnob('gainB', gainBState, 0, 100, '%', v => Math.round(v));
    setupKnob('lfoRate', lfoRateState, 0.01, 20, ' Hz', v => v.toFixed(2));
    setupKnob('lfoRate2', lfoRate2State, 0.01, 20, ' Hz', v => v.toFixed(2));
    setupKnob('lfoDepth', lfoDepthState, 0, 100, '%', v => Math.round(v));
    setupKnob('lfoDepth2', lfoDepth2State, 0, 100, '%', v => Math.round(v));
    setupKnob('timingRandom', timingRandomState, 0, 100, ' ms', v => Math.round(v));
    setupKnob('detuneRandom', detuneRandomState, 0, 50, '¢', v => Math.round(v));
    setupKnob('attackTime', attackTimeState, 1, 5000, ' ms', v => Math.round(v));
    setupKnob('decayTime', decayTimeState, 10, 5000, ' ms', v => Math.round(v));
    setupKnob('sustainLevel', sustainLevelState, 0, 100, '%', v => Math.round(v));
    setupKnob('releaseTime', releaseTimeState, 10, 10000, ' ms', v => Math.round(v));
    setupKnob('filterCutoff', filterCutoffState, 20, 20000, ' Hz', v => Math.round(v));
    setupKnob('filterLfoDepth', filterLfoDepthState, 0, 100, '%', v => Math.round(v));
    setupKnob('velocityToFilter', velocityToFilterState, 0, 100, '%', v => Math.round(v));
    // masterVolume is a linear gain 0.0–1.26; display as dB = 20*log10(gain)
    setupKnob('masterVolume', masterVolumeState, -60, 6, ' dB', v => v.toFixed(1), { db: true });

    // Setup dropdowns
    // WR-02: keyRoot is an AudioParameterChoice → WebComboBoxRelay/getComboBoxState (was WebSliderRelay by luck)
    setupComboBox('keyRoot', keyRootState);
    setupComboBox('voicingMode', voicingModeState);

    setupComboBox('wavetableBank', wavetableBankState);
    setupComboBox('wavetableBank2', wavetableBank2State);

    // Wavetable waveform displays
    const wtDisplayA = new WavetableDisplay('canvasOscA');
    const wtDisplayB = new WavetableDisplay('canvasOscB');
    wireWavetableDisplay(wtDisplayA, wavetablePosState, wavetableBankState);
    wireWavetableDisplay(wtDisplayB, wavetablePos2State, wavetableBank2State);

    // Refresh wavetable canvases when Synth tab becomes visible
    document.querySelector('[data-tab="synth"]').addEventListener('click', () => {
        requestAnimationFrame(() => {
            wtDisplayA.resizeCanvas();
            wtDisplayB.resizeCanvas();
            const bankA = Math.round(wavetableBankState.getChoiceIndex());
            const bankB = Math.round(wavetableBank2State.getChoiceIndex());
            wtDisplayA.fetchAndDraw(bankA, wavetablePosState.getNormalisedValue());
            wtDisplayB.fetchAndDraw(bankB, wavetablePos2State.getNormalisedValue());
        });
    });

    // v2.4.1: Animate waveform displays with LFO-modulated positions (30 Hz from C++)
    if (window.__JUCE__ && window.__JUCE__.backend) {
        window.__JUCE__.backend.addEventListener("wavetableModPos", (raw) => {
            try {
                const data = JSON.parse(raw);
                const bankA = Math.round(wavetableBankState.getChoiceIndex());
                const bankB = Math.round(wavetableBank2State.getChoiceIndex());
                wtDisplayA.fetchAndDraw(bankA, data.posA);
                wtDisplayB.fetchAndDraw(bankB, data.posB);
            } catch(e) { /* ignore parse errors */ }
        });
    }

    setupKnob('chorusRate', chorusRateState, 0.1, 10, ' Hz', v => v.toFixed(2));
    setupKnob('chorusDepth', chorusDepthState, 0, 100, '%', v => Math.round(v));
    setupKnob('chorusMix', chorusMixState, 0, 100, '%', v => Math.round(v));
    setupKnob('delayTime', delayTimeState, 1, 2000, ' ms', v => Math.round(v));
    setupKnob('delayFeedback', delayFeedbackState, 0, 95, '%', v => Math.round(v));
    setupComboBox('delayMode', delayModeState);
    setupKnob('delayMix', delayMixState, 0, 100, '%', v => Math.round(v));
    setupKnob('eqLowGain', eqLowGainState, -12, 12, ' dB', v => v.toFixed(1));
    setupKnob('eqMidGain', eqMidGainState, -12, 12, ' dB', v => v.toFixed(1));
    setupKnob('eqMidFreq', eqMidFreqState, 200, 8000, ' Hz', v => Math.round(v));
    setupKnob('eqHighGain', eqHighGainState, -12, 12, ' dB', v => v.toFixed(1));
    setupKnob('reverbSize', reverbSizeState, 0, 100, '%', v => Math.round(v));
    setupKnob('reverbDamp', reverbDampState, 0, 100, '%', v => Math.round(v));
    setupKnob('reverbPredelay', reverbPredelayState, 0, 200, ' ms', v => Math.round(v));
    setupKnob('reverbMix', reverbMixState, 0, 100, '%', v => Math.round(v));

    setupBypassToggle('chorus', chorusBypassState);
    setupBypassToggle('delay', delayBypassState);
    setupBypassToggle('eq', eqBypassState);
    setupBypassToggle('reverb', reverbBypassState);

    initIntervalSelector();

    // Build mini keyboard
    buildKeyboard();

    // Listen for active notes from C++
    if (window.__JUCE__ && window.__JUCE__.backend) {
        window.__JUCE__.backend.addEventListener("activeNotes", (data) => {
            try {
                const notes = JSON.parse(data);
                updateActiveNotes(notes);
            } catch (e) {
                console.error('Failed to parse activeNotes:', e);
            }
        });
    }

    // LAST, not first. Thirty-seven of this page's eighty tip anchors are the
    // .knob-container elements makeKnob() builds a few lines above, and
    // applyI18n() resolves every selector in TIP_BINDINGS with
    // document.querySelector at the moment it runs. Calling initI18n() at the
    // TOP of this handler would sweep a page whose knobs do not exist yet: it
    // would warn about all 37 and bind none of them, and the failure would look
    // like the tips not working rather than like an ordering bug. The captions
    // makeKnob() writes are already painted through setLabel() by then, in
    // English, which is the same synchronous-then-corrected paint every other
    // plugin does.
    initI18n();
    initializeTooltips();
});

// ====================================================================
// TUNING PANEL INITIALIZATION
// ====================================================================

(async () => {
    try {
        const { TuningPanel } = await import('./tuning-panel.js');
        const container = document.getElementById('tuning-container');
        const tuningPanel = new TuningPanel(container, Juce);
        await tuningPanel.init();

        // Expose note highlighting for C++ evaluateJavascript calls
        window.tuningNoteOn = (midiNote) => tuningPanel.noteOn(midiNote);
        window.tuningNoteOff = (midiNote) => tuningPanel.noteOff(midiNote);
        window.updateHeldNotes = (notes, freqs) => tuningPanel.updateHeldNotes(notes, freqs);

        // The tuning panel builds its whole subtree asynchronously,
        // AFTER initI18n() has already swept the page. Re-running the
        // sweep is what binds its 17 tips and its captions; without it
        // the panel is the one region of the page still in English.
        applyI18n(uiLanguage);

        console.log('[Tuning] Panel initialized successfully');
    } catch (err) {
        console.error('[Tuning] Failed to initialize panel:', err);
    }
})();

// Tab switching logic
function setupTabs() {
    const tabButtons = document.querySelectorAll('.tab-button');
    const tabContents = document.querySelectorAll('.tab-content');

    tabButtons.forEach(button => {
        button.addEventListener('click', () => {
            const targetTab = button.dataset.tab;

            tabButtons.forEach(btn => btn.classList.remove('active'));
            tabContents.forEach(content => content.classList.remove('active'));

            button.classList.add('active');
            document.getElementById(`${targetTab}-tab`).classList.add('active');
        });
    });
}

// ═══ v2.7.0: Preset System ═══
function setupPresetSystem() {
    const nameDisplay = document.getElementById('presetNameDisplay');
    const prevBtn = document.getElementById('presetPrevBtn');
    const nextBtn = document.getElementById('presetNextBtn');
    const saveBtn = document.getElementById('presetSaveBtn');
    const browser = document.getElementById('presetBrowser');
    const browserClose = document.getElementById('presetBrowserClose');
    const categoriesEl = document.getElementById('presetCategories');
    const listEl = document.getElementById('presetList');
    const saveDialog = document.getElementById('presetSaveDialog');
    const saveBackdrop = document.getElementById('presetSaveBackdrop');
    const saveNameInput = document.getElementById('presetSaveNameInput');
    const saveCategorySelect = document.getElementById('presetSaveCategorySelect');
    const saveCancelBtn = document.getElementById('presetSaveCancelBtn');
    const saveConfirmBtn = document.getElementById('presetSaveConfirmBtn');

    let allPresets = [];
    let activeCategory = 'All';

    // Load initial preset name
    (async () => {
        try {
            const name = await Juce.getNativeFunction('getCurrentPresetName')();
            if (name) nameDisplay.textContent = name;
        } catch (e) { console.warn('[Presets] Could not get current name:', e); }
    })();

    // Navigation
    prevBtn.addEventListener('click', async () => {
        try {
            const name = await Juce.getNativeFunction('loadPrevPreset')();
            if (name) nameDisplay.textContent = name;
        } catch (e) { console.error('[Presets] Prev failed:', e); }
    });

    nextBtn.addEventListener('click', async () => {
        try {
            const name = await Juce.getNativeFunction('loadNextPreset')();
            if (name) nameDisplay.textContent = name;
        } catch (e) { console.error('[Presets] Next failed:', e); }
    });

    // Open browser
    nameDisplay.addEventListener('click', async () => {
        await refreshBrowser();
        browser.classList.add('open');
    });

    // Close browser
    browserClose.addEventListener('click', () => {
        browser.classList.remove('open');
    });

    // Save button → open dialog
    saveBtn.addEventListener('click', () => {
        saveNameInput.value = nameDisplay.textContent || '';
        saveBackdrop.classList.add('open');
        saveDialog.classList.add('open');
        saveNameInput.focus();
        saveNameInput.select();
    });

    // Save dialog cancel
    saveCancelBtn.addEventListener('click', closeSaveDialog);
    saveBackdrop.addEventListener('click', closeSaveDialog);

    function closeSaveDialog() {
        saveDialog.classList.remove('open');
        saveBackdrop.classList.remove('open');
    }

    // Save dialog confirm
    saveConfirmBtn.addEventListener('click', async () => {
        const name = saveNameInput.value.trim();
        const category = saveCategorySelect.value;
        if (!name) return;

        try {
            const success = await Juce.getNativeFunction('savePreset')(name, category);
            if (success) {
                nameDisplay.textContent = name;
                closeSaveDialog();
            }
        } catch (e) { console.error('[Presets] Save failed:', e); }
    });

    // Enter key in save dialog
    saveNameInput.addEventListener('keydown', (e) => {
        if (e.key === 'Enter') saveConfirmBtn.click();
        if (e.key === 'Escape') closeSaveDialog();
    });

    // Refresh browser data
    async function refreshBrowser() {
        try {
            const raw = await Juce.getNativeFunction('getPresetList')();
            allPresets = typeof raw === 'string' ? JSON.parse(raw) : raw;
        } catch (e) {
            console.error('[Presets] Failed to load list:', e);
            allPresets = [];
        }
        renderCategories();
        renderList();
    }

    // Render category buttons
    // The English category string stays the VALUE — it is what savePreset()
    // sends to C++ and what preset.category is compared against on the way
    // back — and only the caption is localized. The button is created here and
    // keyed by the caller, for the same reason the knob captions are: a lookup
    // through a { value: key } table is a computed setLabel key.
    function addCategoryBtn(cat) {
        const btn = document.createElement('button');
        btn.className = 'preset-category-btn' + (cat === activeCategory ? ' active' : '');
        btn.addEventListener('click', () => {
            activeCategory = cat;
            categoriesEl.querySelectorAll('.preset-category-btn').forEach(b => b.classList.remove('active'));
            btn.classList.add('active');
            renderList();
        });
        categoriesEl.appendChild(btn);
        return btn;
    }

    function renderCategories() {
        categoriesEl.innerHTML = '';
        setLabel(addCategoryBtn('All'),          'label.all');
        setLabel(addCategoryBtn('Ambient'),      'label.catAmbient');
        setLabel(addCategoryBtn('Cinematic'),    'label.catCinematic');
        setLabel(addCategoryBtn('Classic Pads'), 'label.catClassicPads');
        setLabel(addCategoryBtn('Drones'),       'label.catDrones');
        setLabel(addCategoryBtn('Experimental'), 'label.catExperimental');
    }

    // Render preset list
    function renderList() {
        listEl.innerHTML = '';
        const currentName = nameDisplay.textContent;
        const filtered = activeCategory === 'All'
            ? allPresets
            : allPresets.filter(p => p.category === activeCategory);

        for (const preset of filtered) {
            const item = document.createElement('div');
            item.className = 'preset-item' + (preset.name === currentName ? ' active' : '');

            const nameSpan = document.createElement('span');
            nameSpan.className = 'preset-item-name';
            nameSpan.textContent = preset.name;
            item.appendChild(nameSpan);

            const catSpan = document.createElement('span');
            catSpan.className = 'preset-item-category';
            catSpan.textContent = preset.category;
            item.appendChild(catSpan);

            if (preset.isFactory) {
                const badge = document.createElement('span');
                badge.className = 'preset-item-factory';
                badge.textContent = 'F';
                badge.dataset.i18nAria = 'aria.factoryPreset';
                applyI18nAttributes(badge);
                item.appendChild(badge);
            } else {
                // User preset actions: rename, delete
                const actions = document.createElement('div');
                actions.className = 'preset-item-actions';

                const renameBtn = document.createElement('button');
                renameBtn.className = 'preset-item-action-btn';
                renameBtn.textContent = '\u270E'; // pencil
                renameBtn.dataset.i18nAria = 'aria.rename';
                applyI18nAttributes(renameBtn);
                renameBtn.addEventListener('click', async (e) => {
                    e.stopPropagation();
                    const newName = prompt(trLabel('label.renamePrompt', uiLanguage), preset.name);
                    if (newName && newName !== preset.name) {
                        const ok = await Juce.getNativeFunction('renamePreset')(preset.name, newName);
                        if (ok) {
                            if (nameDisplay.textContent === preset.name) nameDisplay.textContent = newName;
                            await refreshBrowser();
                        }
                    }
                });
                actions.appendChild(renameBtn);

                const delBtn = document.createElement('button');
                delBtn.className = 'preset-item-action-btn delete';
                delBtn.textContent = '\u2715'; // x
                delBtn.dataset.i18nAria = 'aria.delete';
                applyI18nAttributes(delBtn);
                delBtn.addEventListener('click', async (e) => {
                    e.stopPropagation();
                    if (confirm(trLabel('label.deleteConfirm', uiLanguage, { name: preset.name }))) {
                        await Juce.getNativeFunction('deletePreset')(preset.name);
                        await refreshBrowser();
                    }
                });
                actions.appendChild(delBtn);

                item.appendChild(actions);
            }

            // Click to load
            item.addEventListener('click', async () => {
                const isFactory = preset.isFactory ? 1 : 0;
                const success = await Juce.getNativeFunction('loadPreset')(preset.name, isFactory);
                if (success) {
                    nameDisplay.textContent = preset.name;
                    browser.classList.remove('open');
                }
            });

            listEl.appendChild(item);
        }

        if (filtered.length === 0) {
            const empty = document.createElement('div');
            empty.style.cssText = 'grid-column: 1/-1; text-align: center; color: rgba(212,201,176,0.4); font-family: Garamond, serif; font-size: 12px; padding: 20px;';
            setLabel(empty, 'label.noPresetsInCategory');
            listEl.appendChild(empty);
        }
    }
}

// Global knob drag state — one pair of document-level listeners for all knobs
const knobDrag = { active: false, state: null, lastY: 0, virtualNorm: 0 };

document.addEventListener('mousemove', (e) => {
    if (!knobDrag.active) return;
    const deltaY = knobDrag.lastY - e.clientY;
    const sensitivity = 0.005;
    knobDrag.virtualNorm = Math.max(0, Math.min(1, knobDrag.virtualNorm + (deltaY * sensitivity)));
    knobDrag.state.setNormalisedValue(knobDrag.virtualNorm);
    knobDrag.lastY = e.clientY;
});

document.addEventListener('mouseup', () => {
    if (knobDrag.active) {
        knobDrag.state.sliderDragEnded();
        knobDrag.active = false;
        knobDrag.state = null;
    }
});

// Generic knob setup (SVG vine-arc style)
//
// CR-02: The readout is driven by state.getScaledValue() — the true C++
// engineering value from the parameter's NormalisableRange (skew-aware) —
// NOT a linear `min + norm*(max-min)` map, which was ~8x wrong for skewed
// knobs (filterCutoff, lfoRate, time knobs, etc.). `dispMin`/`dispMax` are
// the DISPLAY endpoints matching the range endpoints; the ratio against
// the live properties.start/end handles unit conversion (s→ms, gain→%).
// opts.db=true routes the display through 20*log10(gain) for Master Volume.
function setupKnob(paramId, state, dispMin, dispMax, unit, formatter, opts) {
    if (!state) {
        console.error(`Failed to get slider state for ${paramId}`);
        return;
    }
    const isDb = !!(opts && opts.db);

    const knobEl = document.getElementById(`${paramId}Knob`);
    const vine = document.getElementById(`${paramId}Vine`);
    const valueDisplay = document.getElementById(`${paramId}Value`);

    if (!knobEl || !vine || !valueDisplay) return;

    // Calculate arc length from SVG circle radius
    // Arc spans 270 degrees (0.75 of circumference)
    const r = parseFloat(vine.getAttribute('r'));
    const arcLength = 2 * Math.PI * r * 0.75;

    let isEditing = false;

    // Scaled engineering value (C++ range) -> displayed number
    function scaledToDisplay(scaled) {
        if (isDb) return scaled > 1e-4 ? (20 * Math.log10(scaled)) : -Infinity;
        const { start, end } = state.properties;
        if (end === start) return dispMin;
        return dispMin + ((scaled - start) / (end - start)) * (dispMax - dispMin);
    }
    // Displayed number -> scaled engineering value (C++ range)
    function displayToScaled(disp) {
        if (isDb) return Math.pow(10, disp / 20);
        const { start, end } = state.properties;
        if (dispMax === dispMin) return start;
        return start + ((disp - dispMin) / (dispMax - dispMin)) * (end - start);
    }
    // Scaled engineering value -> normalised [0,1] (skew-aware, matches JUCE)
    function scaledToNorm(scaled) {
        const { start, end, skew } = state.properties;
        if (end === start) return 0;
        const frac = Math.max(0, Math.min(1, (scaled - start) / (end - start)));
        return Math.pow(frac, skew);
    }
    function formatDisplay(scaled) {
        const disp = scaledToDisplay(scaled);
        if (!isFinite(disp)) return '-∞' + unit;  // e.g. 0 gain -> -∞ dB
        const formatted = formatter ? formatter(disp) : disp.toFixed(1);
        return formatted + unit;
    }

    function updateVisual() {
        const normValue = state.getNormalisedValue();

        // Update SVG vine arc (getNormalisedValue is already skew-correct)
        const offset = arcLength - (normValue * arcLength);
        vine.style.strokeDashoffset = offset;

        // Update value display (skip when user is editing)
        if (!isEditing) {
            valueDisplay.textContent = formatDisplay(state.getScaledValue());
        }
    }

    // JUCE -> UI
    state.valueChangedEvent.addListener(() => updateVisual());
    // Re-render once the backend pushes the real range (start/end/skew)
    state.propertiesChangedEvent.addListener(() => updateVisual());

    // UI -> JUCE (drag on knob-visual)
    const knobVisual = knobEl.querySelector('.knob-visual');
    (knobVisual || knobEl).addEventListener('mousedown', (e) => {
        knobDrag.active = true;
        knobDrag.state = state;
        knobDrag.lastY = e.clientY;
        knobDrag.virtualNorm = state.getNormalisedValue();
        state.sliderDragStarted();
        e.preventDefault();
    });

    // Mouse wheel support
    (knobVisual || knobEl).addEventListener('wheel', (e) => {
        e.preventDefault();
        const currentNorm = state.getNormalisedValue();
        const delta = e.deltaY < 0 ? 0.02 : -0.02;
        const newNorm = Math.max(0, Math.min(1, currentNorm + delta));
        state.setNormalisedValue(newNorm);
    }, { passive: false });

    updateVisual();

    // Double-click to edit knob value
    valueDisplay.style.cursor = 'text';
    // NOT a native title=. On a page with a measure-then-pin renderer a native
    // title renders a second, untranslated OS tooltip over the tip (contract
    // section 4), so the hint becomes the element's accessible name instead.
    valueDisplay.dataset.i18nAria = 'aria.doubleClickEdit';
    applyI18nAttributes(valueDisplay);
    valueDisplay.addEventListener('dblclick', (e) => {
        e.stopPropagation();
        if (isEditing) return;
        isEditing = true;

        const initDisp = scaledToDisplay(state.getScaledValue());
        const formatted = isFinite(initDisp)
            ? (formatter ? formatter(initDisp) : initDisp.toFixed(1))
            : '';

        const input = document.createElement('input');
        input.type = 'text';
        input.value = formatted;
        input.style.cssText = `
            width: 50px; text-align: center; font-size: 10px;
            font-family: 'Garamond', serif; font-weight: bold;
            color: #5C4033; background: rgba(255,248,220,0.9);
            border: 1px solid #8B7355; border-radius: 3px;
            padding: 1px 2px; outline: none; user-select: text;
            -webkit-user-select: text;
        `;

        valueDisplay.textContent = '';
        valueDisplay.appendChild(input);
        input.focus();
        input.select();

        function commitValue() {
            if (!isEditing) return;
            const rawVal = parseFloat(input.value);
            if (!isNaN(rawVal)) {
                // display -> scaled -> normalised (skew-aware inverse)
                let scaled = displayToScaled(rawVal);
                const { start, end } = state.properties;
                const lo = Math.min(start, end), hi = Math.max(start, end);
                scaled = Math.max(lo, Math.min(hi, scaled));
                const norm = scaledToNorm(scaled);
                state.sliderDragStarted();
                state.setNormalisedValue(Math.max(0, Math.min(1, norm)));
                state.sliderDragEnded();
            }
            isEditing = false;
            updateVisual();
        }

        input.addEventListener('keydown', (ev) => {
            if (ev.key === 'Enter') { ev.preventDefault(); commitValue(); }
            else if (ev.key === 'Escape') { isEditing = false; updateVisual(); }
        });

        input.addEventListener('blur', () => {
            if (isEditing) commitValue();
        });
    });
}

// Dropdown setup
function setupDropdown(paramId, state, maxIndex) {
    if (!state) {
        console.error(`Failed to get slider state for ${paramId}`);
        return;
    }

    const select = document.getElementById(`${paramId}Select`);

    function updateVisual() {
        const normValue = state.getNormalisedValue();
        const index = Math.round(normValue * (maxIndex - 1));
        select.value = index.toString();
    }

    state.valueChangedEvent.addListener(() => {
        updateVisual();
    });

    select.addEventListener('change', () => {
        const index = parseInt(select.value);
        const normValue = index / (maxIndex - 1);
        state.setNormalisedValue(normValue);
    });

    updateVisual();
}

// ComboBox setup (for WebComboBoxRelay parameters)
function setupComboBox(paramId, state) {
    if (!state) {
        console.error(`Failed to get combobox state for ${paramId}`);
        return;
    }

    const select = document.getElementById(`${paramId}Select`);
    if (!select) return;

    function updateVisual() {
        select.value = state.getChoiceIndex().toString();
    }

    state.valueChangedEvent.addListener(() => {
        updateVisual();
    });

    select.addEventListener('change', () => {
        state.setChoiceIndex(parseInt(select.value));
    });

    updateVisual();
}

// ====================================================================
// BYPASS TOGGLE SETUP
// ====================================================================

function setupBypassToggle(fxName, toggleState) {
    const btn = document.getElementById(`${fxName}BypassBtn`);
    const section = document.getElementById(`${fxName}Section`);
    if (!btn || !section || !toggleState) return;

    function updateVisual() {
        const bypassed = toggleState.getValue();
        // Two KEYS through setLabel, not a literal and not a ternary inside the
        // call: a literal holds one string, so switching to French mid-session
        // would restore an English "On" (check-i18n assertion 13).
        if (bypassed) setLabel(btn, 'ui.off');
        else          setLabel(btn, 'ui.on');
        btn.classList.toggle('bypassed', bypassed);
        section.classList.toggle('bypassed', bypassed);
    }

    toggleState.valueChangedEvent.addListener(() => {
        updateVisual();
    });

    btn.addEventListener('click', () => {
        toggleState.setValue(!toggleState.getValue());
    });

    updateVisual();
}

// ====================================================================
// INTERVAL SELECTOR
// ====================================================================

let intervalData = [];      // Array of { cents, enabled }

async function initIntervalSelector() {
    // Load intervals from tuning engine (don't block on initial load —
    // DAW hosts may delay native function responses during init)
    refreshIntervalSelector();

    // Quick buttons
    document.getElementById('intervalAllBtn')?.addEventListener('click', async () => {
        if (window.__JUCE__?.backend) {
            await Juce.getNativeFunction('resetEnabledIntervals')();
            await refreshIntervalSelector();
        }
    });

    document.getElementById('intervalNoneBtn')?.addEventListener('click', async () => {
        // Disable all except root (degree 0)
        for (let i = 0; i < intervalData.length; i++) {
            if (i > 0) {
                await Juce.getNativeFunction('setIntervalEnabled')(i, false);
            }
        }
        await refreshIntervalSelector();
    });
}

async function refreshIntervalSelector() {
    if (!window.__JUCE__?.backend) {
        // Fallback for standalone testing
        renderIntervalToggles([0, 100, 200, 300, 400, 500, 600, 700, 800, 900, 1000, 1100, 1200],
                              Array(13).fill(true));
        return;
    }

    try {
        // Get tuning intervals (cents)
        const intervalsJson = await Juce.getNativeFunction('getTuningIntervals')();
        const intervals = JSON.parse(intervalsJson);

        // Get enabled state
        const enabledJson = await Juce.getNativeFunction('getEnabledIntervals')();
        const enabled = JSON.parse(enabledJson);

        renderIntervalToggles(intervals, enabled);
    } catch (e) {
        console.error('[IntervalSelector] Failed to refresh:', e);
    }
}

function renderIntervalToggles(intervals, enabled) {
    const container = document.getElementById('intervalToggleList');
    if (!container) return;

    // intervals includes period as last element; exclude it from toggles
    const count = intervals.length - 1;

    intervalData = [];
    let html = '';

    for (let i = 0; i < count; i++) {
        const cents = intervals[i];
        const isEnabled = enabled[i] !== false;
        const isRoot = i === 0;
        intervalData.push({ cents, enabled: isEnabled });

        const label = getIntervalLabel(i, count, cents);
        const centsStr = cents.toFixed(1);

        const classes = ['interval-toggle-item'];
        if (isRoot) classes.push('root');
        else if (isEnabled) classes.push('enabled');

        html += `<div class="${classes.join(' ')}" data-index="${i}" ${isRoot ? '' : 'data-toggleable="true"'}>
            <span class="interval-toggle-dot"></span>
            <span>${label}</span>
            <span class="interval-toggle-cents">${centsStr}c</span>
        </div>`;
    }

    container.innerHTML = html;

    // Attach toggle handlers
    container.querySelectorAll('[data-toggleable]').forEach(item => {
        item.addEventListener('click', async () => {
            const idx = parseInt(item.dataset.index);
            const currentlyEnabled = item.classList.contains('enabled');
            const newState = !currentlyEnabled;

            // Update C++
            if (window.__JUCE__?.backend) {
                await Juce.getNativeFunction('setIntervalEnabled')(idx, newState);
            }

            // Update UI immediately
            item.classList.toggle('enabled', newState);
            intervalData[idx].enabled = newState;
        });
    });
}

function getIntervalLabel(index, scaleSize, cents) {
    if (index === 0) return 'R';  // Root

    if (scaleSize === 12) {
        // For 12-note scales, use semitone names
        const names = ['R', 'm2', 'M2', 'm3', 'M3', 'P4', 'TT', 'P5', 'm6', 'M6', 'm7', 'M7'];
        return names[index] || index.toString();
    }

    // For other scale sizes, use degree number
    return index.toString();
}

// Refresh interval selector when any scale change occurs
// (covers preset changes, .SCL loads, generators, embedded tunings, tonic rotation)
window.addEventListener('tuningScaleChanged', () => {
    setTimeout(() => refreshIntervalSelector(), 100);
});

// Also refresh when temperament preset changes via APVTS (direct parameter change)
const tempState = Juce.getComboBoxState('tuning_temperamentPreset');
if (tempState) {
    tempState.valueChangedEvent.addListener(() => {
        setTimeout(() => refreshIntervalSelector(), 100);
    });
}

// ====================================================================
// MINI KEYBOARD + ACTIVE NOTES VISUALIZATION
// ====================================================================

// Keyboard range: C2 (36) to B5 (83) = 4 octaves
const KB_LOW = 36;
const KB_HIGH = 83;
const BLACK_KEYS = new Set([1, 3, 6, 8, 10]); // pitch classes that are black keys

// Map of MIDI note -> key DOM element
const keyElements = new Map();

// Previous active MIDI notes (for diffing) — Map<midi, gain>
let prevActiveMidi = new Map();

function buildKeyboard() {
    const container = document.getElementById('miniKeyboard');
    if (!container) return;

    // Count white keys in range to calculate widths
    let whiteCount = 0;
    for (let m = KB_LOW; m <= KB_HIGH; m++) {
        if (!BLACK_KEYS.has(m % 12)) whiteCount++;
    }

    // Available width (Voice tab is ~760px)
    const availWidth = 720;
    const whiteW = Math.floor(availWidth / whiteCount);
    const blackW = Math.floor(whiteW * 0.6);
    container.style.width = (whiteCount * whiteW) + 'px';

    let whiteIndex = 0;
    for (let m = KB_LOW; m <= KB_HIGH; m++) {
        const pc = m % 12;
        const isBlack = BLACK_KEYS.has(pc);
        const key = document.createElement('div');
        key.classList.add('key', isBlack ? 'black' : 'white');
        key.dataset.midi = m;

        if (isBlack) {
            // Position black key overlapping previous white key
            const leftPos = (whiteIndex * whiteW) - Math.floor(blackW / 2);
            key.style.left = leftPos + 'px';
            key.style.width = blackW + 'px';
        } else {
            key.style.left = (whiteIndex * whiteW) + 'px';
            key.style.width = whiteW + 'px';
            whiteIndex++;
        }

        // Add C note labels on white keys
        if (pc === 0) {
            const oct = Math.floor(m / 12) - 1;
            const label = document.createElement('span');
            label.className = 'key-label';
            label.textContent = 'C' + oct;
            key.appendChild(label);
        }

        container.appendChild(key);
        keyElements.set(m, key);
    }
}

function updateActiveNotes(notes) {
    const activeMidi = new Map();   // midi -> max gain
    const uniqueNotes = new Map();  // midi -> freq (first seen)
    const groups = new Map();       // midi -> { note, count, maxGain }
    const freqList = document.getElementById('freqList');

    // Single pass: build activeMidi, uniqueNotes, and groups together
    for (const n of notes) {
        const gain = n.gain !== undefined ? n.gain : 1.0;
        activeMidi.set(n.midi, Math.max(activeMidi.get(n.midi) || 0, gain));

        if (!uniqueNotes.has(n.midi)) {
            uniqueNotes.set(n.midi, n.hz);
            groups.set(n.midi, { note: n, count: 1, maxGain: gain });
        } else {
            const g = groups.get(n.midi);
            g.count++;
            g.maxGain = Math.max(g.maxGain, gain);
        }
    }

    // Update keyboard highlighting with gain-based opacity
    for (const midi of prevActiveMidi.keys()) {
        if (!activeMidi.has(midi)) {
            const el = keyElements.get(midi);
            if (el) {
                el.classList.remove('active');
                el.style.removeProperty('--key-opacity');
            }
            if (window.tuningNoteOff) window.tuningNoteOff(midi);
        }
    }
    for (const [midi, gain] of activeMidi) {
        const el = keyElements.get(midi);
        if (el) {
            el.classList.add('active');
            el.style.setProperty('--key-opacity', gain.toFixed(3));
        }
        if (!prevActiveMidi.has(midi) && window.tuningNoteOn) {
            window.tuningNoteOn(midi);
        }
    }
    prevActiveMidi = activeMidi;

    // Feed held notes + frequencies to tuning panel for TrueKeys
    if (window.updateHeldNotes) {
        const midiArr = [...uniqueNotes.keys()];
        const freqArr = midiArr.map(m => uniqueNotes.get(m));
        window.updateHeldNotes(midiArr, freqArr);
    }

    // Update frequency list
    if (notes.length === 0) {
        // createElement + setLabel rather than an innerHTML template:
        // the string is a KEY from here on, so a language change while
        // no note sounds re-renders it through the same sweep.
        freqList.textContent = '';
        const placeholder = document.createElement('span');
        placeholder.className = 'freq-placeholder';
        setLabel(placeholder, 'label.playANote');
        freqList.appendChild(placeholder);
        return;
    }

    // Sort groups by MIDI note for display
    const sorted = [...groups.entries()].sort((a, b) => a[0] - b[0]);

    let html = '';
    for (const [, { note: n, count, maxGain }] of sorted) {
        const noteName = NOTE_NAMES[n.pc] + n.oct;
        const centsAbs = Math.abs(n.cents);
        let centsStr, centsClass;
        const outOfRange = n.midi < KB_LOW || n.midi > KB_HIGH;

        if (centsAbs < 0.5) {
            centsStr = '0¢';
            centsClass = 'zero';
        } else if (n.cents > 0) {
            centsStr = '+' + n.cents.toFixed(1) + '¢';
            centsClass = 'sharp';
        } else {
            centsStr = n.cents.toFixed(1) + '¢';
            centsClass = 'flat';
        }

        const rangeClass = outOfRange ? ' out-of-range' : '';
        const rangeBadge = outOfRange
            ? `<span class="freq-range-badge">${n.midi > KB_HIGH ? '↑' : '↓'}</span>`
            : '';
        const countBadge = count > 1 ? `<span class="freq-count">×${count}</span>` : '';
        // Opacity reflects complexity gain (min 0.3 so fading notes are still visible)
        const opacity = (0.3 + maxGain * 0.7).toFixed(3);

        html += `<div class="freq-item${rangeClass}" style="opacity: ${opacity}; transition: opacity 0.15s;">
            <span class="freq-note">${noteName}${rangeBadge}</span>
            ${countBadge}
            <span class="freq-hz">${n.hz.toFixed(1)} Hz</span>
            <span class="freq-cents ${centsClass}">${centsStr}</span>
        </div>`;
    }
    freqList.innerHTML = html;
}
