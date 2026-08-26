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

    Usage:  const { I18N_CANON } = require('./i18n-canon.js');

  ==============================================================================
*/

'use strict';

const fs = require('fs');

// ─────────────────────────────────────────────────────────── BEGIN I18N CANON
/* <<<I18N_CANON_BEGIN>>>
import { LANGUAGES, I18N, TIP_BINDINGS, tr } from './i18n.js';

let uiLanguage = 'en';
let getUiLanguageNative = null;
let setUiLanguageNative = null;

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
    const sel = document.getElementById('lang-select');
    if (sel && sel.value !== uiLanguage) sel.value = uiLanguage;
}

// Exposed so a clamp gate can drive the language without teaching the ui-stub a
// promise contract: page.evaluate((l) => window.__setLanguage(l), 'fr').
window.__setLanguage = applyI18n;

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
<<<I18N_CANON_END>>> */
// ───────────────────────────────────────────────────────────── END I18N CANON

const SELF = fs.readFileSync(__filename, 'utf8');

const BEGIN = '<<<I18N_CANON_' + 'BEGIN>>>';
const END   = '<<<I18N_CANON_' + 'END>>>';

const beginAt = SELF.indexOf(BEGIN);
const endAt   = SELF.indexOf(END);

if (beginAt < 0 || endAt < 0 || endAt < beginAt) {
    throw new Error('i18n-canon.js: canon sentinels missing or out of order — the file has been edited in a way that destroys the canon.');
}

const I18N_CANON = SELF.slice(beginAt + BEGIN.length, endAt).replace(/^\n/, '');

if (I18N_CANON.trim().length === 0) {
    throw new Error('i18n-canon.js: the canon is empty. An empty canon makes the drift gate pass vacuously for every plugin.');
}

// The import line is asserted separately from the body. On plugins whose gates
// pin the shape of the module top level — O-Octagon §2 forbids any module-level
// declaration after `init();` — the hoisted import is the only new top-level
// form and does not sit adjacent to the rest of the block.
const I18N_CANON_IMPORT = "import { LANGUAGES, I18N, TIP_BINDINGS, tr } from './i18n.js';";

// The body region the drift gate compares: from the first declaration to the
// close of initI18n. A plugin is free to place the hoisted import wherever its
// own gates require.
const I18N_CANON_BODY_START = "let uiLanguage = 'en';";
const I18N_CANON_BODY_END_FN = 'initI18n';

module.exports = {
    I18N_CANON,
    I18N_CANON_IMPORT,
    I18N_CANON_BODY_START,
    I18N_CANON_BODY_END_FN,
};
