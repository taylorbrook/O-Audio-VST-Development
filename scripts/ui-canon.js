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

    ui-canon.js — the UI component canon, held as DATA.

    Every plugin hand-copies its UI JS. The i18n runtime is hand-copied too, but
    it has a canon and a gate (i18n-canon.js + check-i18n.js), and the review
    measured the difference: 1 variant across 44 copies WITH a gate, 13–22
    variants per component WITHOUT one (260924-nho-UI-DESIGN-REVIEW.md, R3).

    This file is the canon half of R3 for the ungated components: the tooltip
    renderer, the hover-help switch, knob binding and the knob visual.
    scripts/check-ui-canon.js reads it and reports, per plugin, whether each
    component matches.

    ── REPORT-ONLY ─────────────────────────────────────────────────────────
    Nothing fails on a mismatch. Converging a plugin onto this canon changes its
    behaviour, so it is done one plugin at a time through /improve with a
    version bump — never as a sweep across the suite. The report is the
    burn-down list for that work, not a gate.

    ── Why DATA and not a live read of O-ReverseDelay ──────────────────────
    The canon is machine-copied from one recorded commit of O-ReverseDelay's
    app.js (UI_CANON_SOURCE below). Reading the live file instead would move
    the target every time O-ReverseDelay itself is improved, and silently
    redefine "canon" for the other 43 plugins with no diff anyone reviews.
    Held here, a re-anchor is a deliberate, reviewed change to this file, and
    O-ReverseDelay is measured against it like every other plugin.

    ── Why //| line comments and not one block comment ─────────────────────
    i18n-canon.js stores its canon inside a block comment. That cannot work
    here: the canon functions themselves contain the block-comment terminator
    (bindKnob has an "already released" note and initTipsToggle a "private
    mode" note, both written as block comments). Either would close an
    enclosing block comment mid-canon. Every canon line is instead prefixed
    with "//|", which makes backticks, ${...} and comment terminators inert
    with zero escaping — the canon stays verbatim.

    ── Re-anchoring (deliberate, reviewed) ──────────────────────────────────
      1. node scripts/check-ui-canon.js --emit-canon <rev>  > /tmp/canon.txt
         (stderr prints the full SHA and that revision's CMake VERSION)
      2. Replace this file's block — from the line holding the BEGIN sentinel
         through the line holding the END sentinel, inclusive — with the
         stdout of step 1. Never hand-edit canon text.
      3. Update UI_CANON_SOURCE.commit and .version to what step 1 printed.
      4. node scripts/check-ui-canon.js --self-test   (N4 compares the block
         against a fresh emit of UI_CANON_SOURCE.commit)

    The sentinel strings are assembled at runtime and never spelled whole in
    any comment: readCanonText() takes the FIRST line containing each one, so
    a literal sentinel anywhere above the block would hijack the extraction.

  ==============================================================================
*/

'use strict';

const fs = require('fs');

// Provenance of the canon block below. `commit` is the last commit that touched
// `file` when the canon was anchored; `version` is that revision's CMake VERSION.
const UI_CANON_SOURCE = Object.freeze({
    plugin:  'O-ReverseDelay',
    file:    'plugins/O-ReverseDelay/Source/ui/public/js/app.js',
    commit:  'ed1fc465e67973c3df2e365a7a1f70341b3aecac',
    version: '1.20.0',
});

// One entry per component. `canon` names are O-ReverseDelay's functions, in the
// order the report joins them. `aliases` are the names other plugins use for
// the same job; an alias hit is always reported as a variant, and it is listed
// so the report NAMES what a plugin actually defines.
const COMPONENTS = Object.freeze([
    { id: 'knob-visual', label: 'knob visual', canon: ['updateKnobVisual'], aliases: [] },
]);

const CANON_BEGIN = '<<<UI_CANON_' + 'BEGIN>>>';
const CANON_END   = '<<<UI_CANON_' + 'END>>>';

// ─────────────────────────────────────────────────────────────── canon block
// <<<UI_CANON_BEGIN>>>
//| function updateKnobVisual(id) {
//|   const st = sliderState[id];
//|   if (!st) return;
//|
//|   const text = FORMAT[id](st.getScaledValue());   // scaled value — never a JS range map
//|
//|   const knob = document.getElementById(`knob-${id}`);
//|   if (knob) {
//|     const stem = knob.querySelector(".knob-stem");
//|     if (stem) {
//|       stem.style.transform =
//|         `translate(-50%, -100%) rotate(${normToDeg(st.getNormalisedValue())}deg)`;
//|     }
//|     knob.setAttribute("aria-valuetext", text);
//|   }
//|
//|   const valEl = document.getElementById(`val-${id}`);
//|   if (valEl) valEl.textContent = text;
//| }
// <<<UI_CANON_END>>>
// ───────────────────────────────────────────────────────────────────────────

// Lazy on purpose: the file must load while the block is empty, because
// --emit-canon (which produces the block) requires this module.
function readCanonText() {
    const lines = fs.readFileSync(__filename, 'utf8').replace(/\r\n?/g, '\n').split('\n');

    const beginAt = lines.findIndex((l) => l.includes(CANON_BEGIN));
    const endAt   = lines.findIndex((l) => l.includes(CANON_END));

    if (beginAt < 0 || endAt < 0)
        throw new Error('ui-canon.js: canon sentinel missing — the file has been edited in a way that destroys the canon.');
    if (endAt <= beginAt)
        throw new Error('ui-canon.js: canon sentinels out of order.');

    const out = [];
    for (let i = beginAt + 1; i < endAt; ++i) {
        const l = lines[i];
        if (!l.startsWith('//|'))
            throw new Error(`ui-canon.js: line ${i + 1} inside the canon block lacks the //| prefix.`);
        out.push(l.startsWith('//| ') ? l.slice(4) : l.slice(3));
    }

    const text = out.join('\n');
    if (text.trim().length === 0)
        throw new Error('ui-canon.js: the canon block is empty — an empty canon makes every comparison vacuous.');

    return text;
}

module.exports = {
    UI_CANON_SOURCE,
    COMPONENTS,
    CANON_BEGIN,
    CANON_END,
    readCanonText,
};
