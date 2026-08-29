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
// i18n_init.js — O-TextureForge language runtime and settings popover (v1.1.0)
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
// The settings popover. ONE row — this plugin has no hover-help to switch on
// or off, and authoring that copy is Stage M's job.
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

// ── deferred init, LAST in the file ─────────────────────────────────────────
// Inside try/catch so that a failure here cannot take the page down with it:
// this module runs after app.bundle.js, and an uncaught throw at module scope
// would leave the page in English rather than leaving it broken, which is the
// correct degradation for a language runtime.
initializeSettingsPopover();
try { initI18n(); } catch (e) { console.error('i18n init failed:', e); }
