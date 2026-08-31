/*
   This file is part of O-TextureForge, an Ouaricon Audio plugin.
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
// i18n_init.js — O-TextureForge language runtime, hover-help renderer and
//                settings popover (v1.2.0)
//
// ── WHY THIS FILE EXISTS AT ALL, WHEN NO OTHER PLUGIN HAS ONE ───────────────
//
// O-TextureForge is the first WEBPACK-BUNDLED page in this rollout. Its
// authored controller is Source/ui/src/app.js, which webpack compiles into
// public/js/app.bundle.js and which index.html loads as a CLASSIC script. That
// is a fourth controller shape: not O-Tapestop's js/app.js module, not
// O-AnalogSaturation's inline block, not O-Texture's js/main.js loaded by src.
//
// The canon block CANNOT go into the bundle. It imports './i18n.js' at module
// scope, and webpack would resolve that import at BUILD time and inline the
// table into app.bundle.js — at which point js/i18n.js is embedded in the
// binary, served from getResource(), and read by nobody. The label table would
// be shipped twice and editable in only one of the two places.
//
// So the canon lives here, in a real ES module the page loads by src. That
// keeps js/i18n.js a genuine runtime import over the wire, keeps assertion 8's
// embed-and-serve pair meaningful, and gives check-i18n a module to resolve as
// the controller by exactly the rule it already uses for O-Texture: whatever
// index.html loads as type="module" IS the controller.
//
// FILENAME UNDERSCORE, NOT HYPHEN. juce_add_binary_data strips hyphens rather
// than converting them, so i18n-init.js would embed as the unreadable symbol
// i18ninit_js (critical_binary_data_strips_hyphens). i18n_init.js embeds as
// i18n_init_js.
//
// ── HOW THE BUNDLE REACHES THIS ─────────────────────────────────────────────
//
// It does not import it. Two modules on one page cannot share a binding when
// one of them is a webpack output, so the canon exposes window.__setLabel —
// which it already does, verbatim, for exactly this case (O-Bitrot's inline
// controller has the same problem for a different reason). src/app.js calls
// window.__setLabel(el, key, vars) for the eight prose strings it writes, and
// each element becomes a [data-i18n] element from that moment on, so the
// language sweep owns it thereafter with no second code path.
//
// ── ORDERING ────────────────────────────────────────────────────────────────
//
// A type="module" script is deferred: it runs after the document is parsed and
// AFTER the classic app.bundle.js has executed. The bundle only registers a
// DOMContentLoaded handler at that point, so nothing races. initI18n() is
// called at the very END of this file, never at the top — a top-level call
// reaching a lower let/const is a TDZ throw that kills every later initializer
// (pattern_module_toplevel_init_tdz).
// ============================================================================

// The Juce ES-MODULE NAMESPACE, not window.__JUCE__. getNativeFunction is an
// export of this module; the raw backend object underneath it has no such
// method (critical_juce_webview_namespace_vs_postmessage). The page also loads
// this module in <head> and the harness overlays it with a stub, so this import
// resolves to the same module instance either way.
import * as Juce from './juce/index.js';

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

// ============================================================================
// The settings popover. ONE row. v1.2.0 adds hover-help but NOT a switch for
// it: two plugins in the suite (O-Tapestop, O-Bitrot) carry an on/off toggle
// and the other forty-one do not, so making this the forty-second would be a
// uniformity decision taken sideways inside a copy commit. Tips are always on.
// ============================================================================

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

    // pointerdown rather than click, so the panel is gone before a drag begins
    // underneath it: every knob on this page starts its drag on mousedown, and
    // the panel hangs over the scatter canvas, which also starts a pan there.
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
// setupTooltips — the hover-help RENDERER (v1.2.0)
//
// ── WHY THIS IS HERE AND NOT IN src/app.js ──────────────────────────────────
//
// Same reason the canon is here. src/app.js is webpack INPUT; the page loads
// its output, js/app.bundle.js, as a classic script. Putting the renderer there
// means a webpack rebuild inside a copy commit and a 220 KB diff nobody can
// review. It belongs beside the canon, in the module that already owns every
// [data-tip] attribute on the page.
//
// ── WHAT IT IS ──────────────────────────────────────────────────────────────
//
// A behavioural port of O-simpleFM's setupTooltips (js/app.js:384-462): one
// delegated, cursor-following surface, no per-element listener, no
// measure-then-pin placement engine and no help-toggle state. It looks up
// NOTHING: applyI18n() has already written data-tip-title and data-tip onto
// each anchor in the current language, and rewrites both on every language
// change. This function only positions and shows what the anchor carries.
//
// Every property below is load-bearing and each has a scar behind it:
//
//  1. DELEGATED ON document, not querySelectorAll('[data-tip]') at setup time.
//     No anchor carries data-tip until applyI18n() has run, so a setup-time
//     query binds nothing at all and fails silently.
//  2. pointerover / pointerout / focusin / focusout, because they BUBBLE.
//     pointerenter / focus do not.
//  3. pointerout ignores a move between two descendants of the SAME anchor, or
//     the tip flickers off and on at every child boundary — and every anchor
//     here is a wrapper cell holding a knob, a caption and a readout, so those
//     boundaries are crossed constantly.
//  4. createElement + textContent, NEVER innerHTML. Localized copy must never
//     reach a markup path.
//  5. The clamp runs AFTER the flip, not before it, and on all four edges.
//     Measured on O-Bass at 420x320 in M1: every anchor placed by flipping to
//     the opposite side of the cursor, and two of the flipped results then hit
//     the 8 px floor. A renderer that clamps before it flips passes every
//     containment check while placing tips outside the frame.
//  6. pointer-events: none on the surface, or it steals the hover keeping it
//     open. (In the stylesheet.)
//  7. position: fixed, visibility: hidden until shown. (In the stylesheet.)
//  8. Escape hides. pointerdown hides.
//  9. THE FOCUS ARM IS LATCHED TO THE KEYBOARD. See the comment on
//     lastInputWasPointer below — this is the defect the M1 batch found on nine
//     plugins at once.
// 10. A DRAG GUARD, which the reference family does not have and this page
//     needs. Every knob here starts its drag on mousedown and tracks
//     document.mousemove, and the scatter canvas pans the same way. Without the
//     guard a vertical knob drag that strays into a neighbouring .knob-row
//     fires pointerover and opens THAT row's tip mid-gesture, over the control
//     being turned. pointerdown alone cannot cover it, because pointerover
//     arrives after it (O-Chorus reported this shape in M1).
// ============================================================================

function setupTooltips() {
    const tip = document.getElementById('tooltip');
    if (!tip) { console.warn('hover-help surface #tooltip missing'); return; }

    // The clamp margin, in px, from every edge of the frame. Mirrored by
    // tests/ui_tip_render_check.js, which reads it back out of this file.
    const MARGIN = 8;

    let active = null;

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

    const position = (x, y) => {
        const r = tip.getBoundingClientRect();
        const W = window.innerWidth;
        const H = window.innerHeight;

        // Preferred placement: below and to the right of the cursor.
        let nx = x + 14;
        let ny = y + 16;

        // FLIP to the opposite side of the cursor where the preferred side
        // would overflow.
        if (nx + r.width  > W - MARGIN) nx = x - r.width  - 14;
        if (ny + r.height > H - MARGIN) ny = y - r.height - 12;

        // THEN CLAMP THE FLIPPED RESULT, both rails, both axes. The flip can
        // itself leave the frame on the far edge. Math.max on the upper rail
        // keeps a surface taller than the frame pinned at MARGIN rather than
        // driven negative, so the overflow it causes is REPORTED by the render
        // gate instead of being hidden by the clamp.
        nx = Math.min(Math.max(MARGIN, nx), Math.max(MARGIN, W - r.width  - MARGIN));
        ny = Math.min(Math.max(MARGIN, ny), Math.max(MARGIN, H - r.height - MARGIN));

        tip.style.left = `${nx}px`;
        tip.style.top  = `${ny}px`;
    };

    const hide = () => {
        tip.classList.remove('show');
        tip.setAttribute('aria-hidden', 'true');
        active = null;
    };

    const anchorOf = (t) => (t && t.closest ? t.closest('[data-tip]') : null);

    // Property 10, the drag guard. Set on any pointerdown, cleared on the
    // matching up/cancel; while it is set no hover opens a tip.
    let pointerHeld = false;

    // Property 9. A mouse click on a <button> FOCUSES it, so the reference
    // renderer's unconditional focusin rule leaves a tip parked on screen after
    // every click — measured across nine M1 plugins as 3800 to 5280 px2 of the
    // gear's own tip lying across the settings popover the click had just
    // opened. :focus-visible is deliberately NOT the discriminator: Chromium
    // reports it false for a programmatic .focus() following a click, so a gate
    // driving focus directly would measure "no tip" and record that as correct.
    // An explicit last-input-device latch is the same rule and is drivable with
    // real events; any keydown clears it.
    let lastInputWasPointer = false;

    document.addEventListener('pointerover', (e) => {
        if (pointerHeld) return;
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

    document.addEventListener('pointerdown', () => {
        lastInputWasPointer = true;
        pointerHeld = true;
        hide();
    });
    document.addEventListener('pointerup',     () => { pointerHeld = false; });
    document.addEventListener('pointercancel', () => { pointerHeld = false; });

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
    document.addEventListener('keydown', (e) => {
        lastInputWasPointer = false;
        if (e.key === 'Escape') hide();
    });
}

// ── deferred init, LAST in the file ─────────────────────────────────────────
// Inside try/catch so that a failure here cannot take the page down with it:
// this module runs after app.bundle.js, and an uncaught throw at module scope
// would leave the page in English rather than leaving it broken, which is the
// correct degradation for a language runtime.
//
// setupTooltips() sits AFTER initI18n() and inside the SAME try/catch, on
// purpose on both counts: no anchor carries data-tip until applyI18n() has run,
// and a top-level call reaching a lower let/const is a TDZ throw that kills
// every later initializer (pattern_module_toplevel_init_tdz).
initializeSettingsPopover();
try { initI18n(); setupTooltips(); } catch (e) { console.error('i18n init failed:', e); }
