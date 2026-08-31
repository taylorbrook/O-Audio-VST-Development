/*
   This file is part of the Ouaricon Audio plugin suite.
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

    i18n-canon.js — the ONE i18n runtime block, held as DATA.

    This repo has no shared UI module and deliberately does not gain one for
    this work: every plugin hand-copies its UI JS. That decision was taken with
    its cost understood — 43 independent copies of the same runtime, which prior
    module-extraction work here shows diverge silently.

    This file is the only mitigation available under that rule. It holds the
    canonical applyI18n/initI18n block verbatim. scripts/check-i18n.js pulls the
    same region out of every plugin's app.js, normalises both (comments
    stripped, whitespace collapsed) and compares. A copy that drifts fails a
    gate instead of drifting quietly.

    It is DATA, not an import target. Nothing imports the block from here at
    runtime — that would BE the shared module CONTEXT.md rules out.

    ── Why the canon lives in a comment ─────────────────────────────────────
    The block contains backticks and ${...}. Held in a template literal every
    one of those would need escaping, and an escaped canon is no longer a
    verbatim canon — it is a second, subtly different copy of the thing whose
    job is to be the only copy. So the canon is stored between two sentinels
    inside a comment, where backticks and ${...} are inert, and the module
    reads its own source to recover it. Zero escaping, zero duplication.

    Usage:  const { I18N_CANON_V2 } = require('./i18n-canon.js');

    ── There used to be two canons ──────────────────────────────────────────
    v1 localized tooltip ATTRIBUTES; v2 adds labels, visible-text attributes
    and JS-written strings. Both were carried through the rollout and assertion
    6 accepted either, reporting the split, because changing the canon in place
    would have turned the gate red the moment v2 was committed and kept it red
    for every plugin that had not migrated yet.

    That is over. All 43 plugins are on v2 as of Stage L, and v1 is deleted
    here rather than left as a canon nothing is on — a second canonical block
    that no plugin matches is not a safety net, it is a thing that can be
    matched by accident. The rollout's own record lives in the SUMMARY, not in
    a dead sentinel pair.

  ==============================================================================
*/

'use strict';

const fs = require('fs');


// ───────────────────────────────────────────────────────── BEGIN I18N CANON V2
//
// The retired canon v1 localized tooltip ATTRIBUTES. v2 adds LABELS,
// visible-text attributes and JS-written strings, per the CONTRACT V2 section of
// the plan.
//
// BOTH canons are carried, and check-i18n.js assertion 6 accepts either and
// reports the split as a migration worklist. Changing the canon in place would
// turn the gate red the moment this file is committed and keep it red for the
// whole rollout — which is how a team learns to ignore a gate.
//
// What v2 adds, and why each piece is shaped the way it is:
//
//   trLabel()   — a lookup over LABELS with a fallback to I18N. The fallback is
//                 what lets a control whose tooltip TITLE already IS its label
//                 carry ONE key instead of two copies of the same string in two
//                 tables, drifting apart.
//
//   applyLabel()— writes textContent AND dataset.label together. Making every
//                 label JS-written puts all 43 plugins into
//                 pattern_js_state_updater_overwrites_html_labels at once; the
//                 repo's documented fix is that the element OWNS its label and
//                 an updater reads `el.dataset.label ?? fallback`. Writing both
//                 in one place makes the invariant checkable at render time:
//                 dataset.label === textContent for every [data-i18n].
//
//   setLabel()  — a JS-written label DECLARES ITS OWN KEY and becomes a
//                 [data-i18n] element from that moment on, so the language
//                 sweep owns it. No custom event, no subscription list, no
//                 second code path that can go stale in the other language.
//                 A state-dependent string written outside the table is
//                 stranded in the previous language the instant the selector
//                 fires — the bug Stage B found on MBC's hover-help toggle.
//
//   the attribute sweep — data-i18n-aria / -placeholder / -alt resolve through
//                 the same pass. Native title= is DELETED rather than
//                 localized (contract §4): on an element that has a data-tip it
//                 renders a second, untranslated OS tooltip competing with the
//                 measure-then-pin renderer.
//
// I18N_EXEMPT is deliberately NOT imported here. It is read statically by
// check-i18n.js; importing a binding the runtime never touches is noise.
//
/* <<<I18N_CANON_V2_BEGIN>>>
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
// Re-runs the sweep at the CURRENT language, for a subtree built AFTER the last
// one. It cannot be window.__setLanguage, which needs a language argument the
// caller does not have: uiLanguage lives in this module's scope and nothing
// outside can read it, so guessing 'en' would silently reset a French page every
// time a panel remounted.
//
// This is in the canon because three plugins wrote it independently, OUTSIDE the
// byte-compared region, each with a comment explaining that the region "may not
// gain a line" — O-Bells (index.html:1992), O-Marimba and O-IntonationPad
// (js/app.js). Three authors hitting the same wall and working around it the
// same way is the canon's job, not theirs. Declaring it here changes no
// behaviour on a page that never calls it; it only means a page that needs it no
// longer has to reach around the gate to get it.
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
<<<I18N_CANON_V2_END>>> */
// ─────────────────────────────────────────────────────────── END I18N CANON V2


const SELF = fs.readFileSync(__filename, 'utf8');

// The sentinels are assembled at runtime so that the literals below cannot
// themselves be found by the search — the file would otherwise locate its own
// extraction code instead of the canon.
function extractBetween(beginTag, endTag, label) {
    const beginAt = SELF.indexOf(beginTag);
    const endAt   = SELF.indexOf(endTag);

    if (beginAt < 0 || endAt < 0 || endAt < beginAt)
        throw new Error(`i18n-canon.js: ${label} sentinels missing or out of order — the file has been edited in a way that destroys the canon.`);

    const body = SELF.slice(beginAt + beginTag.length, endAt).replace(/^\n/, '');

    if (body.trim().length === 0)
        throw new Error(`i18n-canon.js: ${label} is empty. An empty canon makes the drift gate pass vacuously for every plugin.`);

    return body;
}

const I18N_CANON_V2 = extractBetween('<<<I18N_CANON_V2_' + 'BEGIN>>>', '<<<I18N_CANON_V2_' + 'END>>>', 'canon v2');

// The import line is asserted separately from the body. On plugins whose gates
// pin the shape of the module top level — O-Octagon §2 forbids any module-level
// declaration after `init();` — the hoisted import is the only new top-level
// form and does not sit adjacent to the rest of the block.
const I18N_CANON_V2_IMPORT = "import { LANGUAGES, I18N, LABELS, TIP_BINDINGS, tr } from './i18n.js';";

// The body region the drift gate compares: from the first declaration to the
// close of initI18n. A plugin is free to place the hoisted import wherever its
// own gates require.
const I18N_CANON_BODY_START = "let uiLanguage = 'en';";
const I18N_CANON_BODY_END_FN = 'initI18n';

module.exports = {
    I18N_CANON_V2,
    I18N_CANON_V2_IMPORT,
    I18N_CANON_BODY_START,
    I18N_CANON_BODY_END_FN,
};
