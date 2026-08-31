/*
   This file is part of O-Polystutter, an Ouaricon Audio plugin.
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
// app.js — O-Polystutter page controller (v1.14.0)
//
// NEW FILE, AND THE FILE IS THE POINT. Through v1.13.0 this plugin's controller
// was four <script> blocks inline in index.html: the JUCE namespace shim, the
// preset manager, a context-menu suppressor, and a 92-line tooltip positioner.
// Nothing was wrong with that layout in itself — but it is the ONLY plugin
// layout in the repo that puts the tooltip runtime inside the markup, and that
// is how the second renderer stayed invisible long enough to spread to seven
// plugins. Extracting it here gives the served tree the same shape as every
// other canon-v2 plugin: index.html is markup, js/app.js is the controller,
// js/i18n.js is the copy.
//
// js/parameter-bindings.js is UNTOUCHED and still loaded on its own. It is the
// APVTS<->DOM layer, not page chrome, and moving it would have put a working
// 1043-line binding file into a commit about language.
//
// THE SECOND TOOLTIP RENDERER IS DELETED, NOT DISABLED. v1.13.0's positioner
// never measured: it carried `const tooltipHeight = 60; // Approximate max
// height`, `const tooltipWidth = 220; // max-width from CSS` and the two
// viewport literals 660 and 1000, hard-coded against a frame that is 690 tall.
// It was already wrong before French made any string taller. What replaces it
// is the measure-then-pin runtime from O-ReverseDelay / O-MultiBandCompressor /
// O-FreqPulse, and after this commit six of the seven second-renderer plugins
// remain.
// ============================================================================

import * as Juce from './juce/index.js';
import { PresetManager } from '../modules/preset-manager.js';
import { LANGUAGES, I18N, LABELS, TIP_BINDINGS, tr } from './i18n.js';

// Kept from v1.13.0's shim block: the namespace was exposed for console
// debugging inside the WebView, where there is no module scope to reach into.
window.Juce = Juce;


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
// initI18n() is CALLED from the DOMContentLoaded handler at the bottom of this
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

// ═══════════════════════════════════════════════════════════════════════════
// The settings popover (v1.14.0)
// ═══════════════════════════════════════════════════════════════════════════
//
// The gear takes the exact place the floating "?" occupied through v1.13.0 —
// bottom-right of the frame, over the tape section's empty corner — so nothing
// else on a fully-packed 1000 x 690 layout had to move for it. The hover-help
// switch moves inside: one place for the two things that decide what the hover
// help says and whether it says it at all.

let gearBtnEl = null;
let settingsPopoverEl = null;

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
    // click, so the panel is gone before a drag on a knob underneath it begins.
    // Matches how the preset dropdown already behaves.
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
// Tooltips — the measure-then-pin renderer (v1.14.0)
// ═══════════════════════════════════════════════════════════════════════════
//
// PORTED from O-ReverseDelay / O-MultiBandCompressor / O-FreqPulse, replacing
// this plugin's own positioner ENTIRELY. This plugin is where the deleted one
// was WRITTEN, so nothing of it survives anywhere: the grep for tooltipHeight,
// tooltipWidth and data-tooltip over the served tree returns nothing.
//
// What the port brings that v1.13.0's positioner did not have: a title/body pair
// built from data-tip-title + data-tip rather than one flat string, a dwell
// delay so a tip does not fire on every crossing, a width RELEASED, MEASURED
// and PINNED before `left` is applied, an arrow whose offset is recomputed
// AFTER the horizontal clamp so a clamped tip still points at its control,
// delegated listeners on the DOCUMENT rather than on .plugin-frame, and
// viewport-relative arithmetic that matches the fixed-position box the browser
// actually lays out instead of four hard-coded numbers.
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

// The hover-help layer's master switch. SESSION-ONLY, exactly as in v1.13.0:
// this plugin has never had a getTooltipsEnabled / setTooltipsEnabled native
// pair, and adding one is a processor-state change that does not belong in a
// commit about language. Starts false, which is v1.13.0's behaviour unchanged.
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
    // because the knob handlers in js/parameter-bindings.js call preventDefault
    // in their own mousedown listeners.
    document.addEventListener('pointerdown', () => {
        tooltipSuppressed = true;
        hideTooltip();
    }, true);

    document.addEventListener('pointerup', () => { tooltipSuppressed = false; }, true);

    console.log('[v1.14.0] Tooltips initialized');
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

    const frame = document.getElementById('plugin-frame');
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
    // .knob-container wraps a label, a knob and a value readout, and crossing
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

    // THE VERTICAL CLAMP. The renderer as written in O-ReverseDelay prefers
    // above, flips below, and stops — correct on a page whose anchors are all
    // knob-sized, wrong the moment an anchor is tall enough that NEITHER
    // placement fits. O-FreqPulse's 376 px #grid-area is where that surfaced.
    //
    // IT IS NOT INDEPENDENTLY REPRODUCIBLE ON THIS PAGE, and that is said rather
    // than dressed up: reverting these two lines alone and re-hovering all 105
    // anchors in both languages leaves the sweep GREEN. The clamp fires only
    // when anchor.top < h + 16 AND anchor.bottom > 674 - h; at this page's
    // tallest French tip (102 px, the language selector's) that needs an anchor
    // over 454 px tall, and the tallest anchor here is #sequencer-section at
    // 185 px with 369 px of clear room above it. Unlike O-FreqPulse, v1.13.0's
    // positioner had NO vertical clamp either — only a flip, decided against a
    // hard-coded 60 px height it never measured — so nothing that used to work
    // is being restored here.
    //
    // Carried in anyway, as PART of the renderer rather than a per-plugin
    // patch: it is one line of the one repo-wide runtime, it costs nothing when
    // it does not fire, and this page's own tip heights are a font-metric
    // measurement that no gate makes on Windows.
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
// Preset bar — modules/preset-manager.js
// ═══════════════════════════════════════════════════════════════════════════
//
// Moved verbatim in behaviour from the inline <script type="module"> v1.13.0
// carried, with ONE change: the dropdown rows are built with createElement and
// setLabel instead of an innerHTML template. That template held two localizable
// strings — the Factory badge and the delete button's native title= — and no
// I18N_EXEMPT entry could have covered them, because an exemption lives in
// js/i18n.js where assertion 9 forbids the opening angle bracket outright.

let presetManager = null;

function initializePresetManager() {
    const presetNameText = document.getElementById('preset-name-text');
    const presetNameDisplay = document.getElementById('preset-name-display');
    const presetDropdownMenu = document.getElementById('preset-dropdown-menu');

    if (!presetNameText || !presetNameDisplay || !presetDropdownMenu) {
        console.warn('preset bar markup missing — preset manager not started');
        return;
    }

    presetManager = new PresetManager({
        displayElement: presetNameText,
        prevButton: document.getElementById('preset-prev'),
        nextButton: document.getElementById('preset-next'),
        saveButton: document.getElementById('preset-save'),
        loadButton: document.getElementById('preset-load'),
        getNativeFunction: Juce.getNativeFunction,
        onPresetChanged: (name) => {
            console.log('[PresetManager] Loaded preset:', name);
            updateDropdownMenu();
        },
        onPresetListUpdated: (list) => {
            console.log('[PresetManager] Preset list updated:', list.length, 'presets');
            updateDropdownMenu();
        }
    });

    presetNameDisplay.addEventListener('click', (e) => {
        e.stopPropagation();
        presetDropdownMenu.classList.toggle('open');
    });

    document.addEventListener('click', () => {
        presetDropdownMenu.classList.remove('open');
    });

    presetManager.initialize().then(() => updateDropdownMenu());

    // Kept from v1.13.0 for console debugging inside the WebView.
    window.presetManager = presetManager;
}

// Rebuilt from scratch on every open — the list is small, and this sidesteps
// stale-list bugs after a save or a delete without extra refresh plumbing.
async function updateDropdownMenu() {
    const menu = document.getElementById('preset-dropdown-menu');
    if (!menu || !presetManager) return;

    const list = presetManager.getPresetList();
    const currentName = presetManager.getCurrentPreset();

    menu.textContent = '';

    for (const name of list) {
        const isFactory = await presetManager.isFactoryPreset(name);

        const item = document.createElement('div');
        item.className = 'preset-dropdown-item' + (name === currentName ? ' selected' : '');

        // The preset NAME is not localized and never can be: it IS the JSON
        // filename (OuariconPresetManager.h:283-285), so a translated name
        // would not resolve on recall. D-02.
        const nameEl = document.createElement('span');
        nameEl.className = 'preset-item-name';
        nameEl.textContent = name;
        item.appendChild(nameEl);

        if (isFactory) {
            const badge = document.createElement('span');
            badge.className = 'factory-badge';
            setLabel(badge, 'label.factory');
            item.appendChild(badge);
        } else {
            const del = document.createElement('span');
            del.className = 'delete-btn';
            // The glyph is a multiplication sign, not a letter: it is not copy
            // and has no language. Its accessible NAME is the copy, and it is
            // keyed by assigning dataset.i18nAria — setLabel() writes
            // textContent and so cannot key an attribute, and an element built
            // at runtime cannot carry the attribute in the markup.
            del.textContent = '×';
            del.dataset.i18nAria = 'aria.deletePreset';
            applyI18nAttributes(del);
            item.appendChild(del);
        }

        item.addEventListener('click', async (e) => {
            if (e.target.classList.contains('delete-btn')) {
                e.stopPropagation();
                // tr(), not trLabel(): this sentence has no element and no
                // tooltip, so it is housed in I18N with an empty body — the one
                // shape that satisfies assertions 13 and 15 together. Authored
                // around the inflection per contract §6: no count, no plural.
                if (confirm(tr('msg-delete-preset', uiLanguage, { name }).t)) {
                    await presetManager.deletePreset(name);
                }
                return;
            }
            await presetManager.loadPreset(name);
            menu.classList.remove('open');
        });

        menu.appendChild(item);
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// Page init
// ═══════════════════════════════════════════════════════════════════════════

// Kept from v1.13.0: a right-click context menu inside a plugin WebView is not
// something a DAW user ever wants.
document.addEventListener('contextmenu', (e) => {
    e.preventDefault();
    return false;
});

document.addEventListener('DOMContentLoaded', () => {
    console.log('O-Polystutter UI v1.14.0 initializing...');

    // ORDER IS LOAD-BEARING, in the ordinary way rather than the TDZ way.
    //
    // initSettingsPopover() first: initI18n() reads #lang-select and attaches
    // the change listener that drives the sweep, so the panel it lives in has to
    // be wired before the sweep can be driven from it.
    //
    // initI18n() before initializeTooltips(): applyI18n() is what puts
    // data-tip / data-tip-title on all 105 anchors in the first place. The
    // renderer resolves e.target.closest('[data-tip]') at hover time, so a first
    // hover landing in the window between the two would find no anchor at all.
    //
    // Each inside its own try/catch: a translation-table typo must not take the
    // 64 bound step buttons down with it, which is exactly what the MBC v1.4.0
    // TDZ throw did to unrelated working controls while build, auval and every
    // static check still passed.
    try { initSettingsPopover(); } catch (e) { console.error('settings popover init failed:', e); }
    try { initI18n(); }           catch (e) { console.error('i18n init failed:', e); }

    initializeTooltips();
    initializePresetManager();
});
