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

    i18n-zh-glossary.js — the settled Simplified Chinese for terms that recur
    across plugins.

    ── REPORT TODAY, GATE LATER ──────────────────────────────────────────────

    This file is data; the tool that reads it — scripts/i18n-zh-lint.js — ships
    as a REPORT and exits 0 no matter what it finds. It is promoted to a GATE
    (exit 2 on any finding) only once the Stage 2 pilot, O-Chorus, is at zero
    findings. That is the exact lifecycle scripts/i18n-fr-lint.js went through:
    it began report-only on the day 43 of 43 plugins failed it, and became a
    gate on 2026-08-31 when the rollout had taken every plugin to 0. Doing it in
    the other order would let a half-built lint block Stage 2.

    ── WHY THIS EXISTS ───────────────────────────────────────────────────────

    The French rollout scanned every French entry in the suite and found 267
    English label strings carrying MORE THAN ONE French rendering — "Off" had
    nine. Each was a defensible choice made by one author looking at one plugin.
    Forty-three reviewers working without a shared list make that WORSE, not
    better. The French glossary was written AFTER the divergence; this one is
    written BEFORE any translator is dispatched, which is the entire reason
    Stage 1 precedes Stage 2.

    The shared-string set is measured, not guessed: an ESM-import walk over all
    43 plugins on 2026-09-01 found 3789 entries needing a zh value, 1802 unique
    short strings, and 551 strings appearing at MORE THAN ONE site — covering
    2538 of 3789 occurrences (67.0%). Those 551 are what TERMS holds.

    ── THE REVIEW BAR, STATED PLAINLY ────────────────────────────────────────

    This project has NO native Chinese reader. The `reviewed: true` flip the
    French tables got meant literally "the developer read it", and that lane is
    closed here. So the zh rollout ships at `reviewed: 'bt'` — an INDEPENDENT
    back-translation (zh -> en', produced by a separate pass that never saw the
    English) which the developer read against the English source. `'native'`
    stays open as a later upgrade and is a blocker for nothing. `'mt'` — a raw
    machine draft nobody checked — NEVER ships. See scripts/i18n-zh-lint.js
    rule R1, which is what makes that bar mechanical rather than aspirational.

    ── How TERMS is read ─────────────────────────────────────────────────────

    Key:   the English label or tooltip TITLE, lower-cased, trimmed, with a
           trailing period dropped (the lint normalises live entries the same
           way, so the keys must be normalised too).
    Value: an ARRAY of every ACCEPTED zh-Hans rendering, ROOT FIRST.

    The value type is deliberately IDENTICAL to the French glossary's — English
    key to an array of renderings — so Stage-2 tooling can consume the fr and zh
    glossaries through one code path. Budgets and the stay-English set are
    SEPARATE exports (BUDGETS, SAME_AS_EN) rather than a richer value type, for
    the same reason.

    The first value is the ROOT term — what a reviewer reaches for when the
    frame has room. The rest are accepted alternates for a caption whose width
    was pinned. Chinese has no abbreviations, so the only lever a tight cell
    leaves is a SHORTER RENDERING: a 2-character term instead of a 3- or
    4-character one. Where an alternate is listed, the trailing comment names
    the cell that forced it.

    Tooltip BODIES are not matched against TERMS — prose is a person's job.
    Bodies ARE scanned for FORBIDDEN_IN_PROSE.

    ── Exempting one entry ───────────────────────────────────────────────────

    A term can be right in one plugin and wrong in another. The entry carries
    the reason, never silence — same mechanism as the French glossary:

        'label.delay': { en: { t: 'Delay' },
                         'zh-Hans': { t: '延时补偿', reviewed: 'bt',
                                      termNote: 'alignment delay, not the effect' } }

    A termNote exempts the entry from BOTH term rules — Z5 and F1.

  ==============================================================================
*/

'use strict';

// ── EN (lower-cased) -> accepted zh-Hans renderings. ROOT FIRST. ────────────
// Seeded in Task 1 with the chrome every plugin shares; the full measured
// shared-string set is filled in Task 3.
const TERMS = {
    // ── chrome, shared by all 43 ────────────────────────────────────────────
    'settings':                 ['设置'],
    'interface language':       ['界面语言'],
    'hover help':               ['悬停帮助'],
    'save':                     ['保存'],
    'load':                     ['载入', '加载'],
    'delete':                   ['删除'],
    'reset':                    ['重置'],
    'on':                       ['开', '开启'],
    'off':                      ['关', '关闭'],
    // ── the shared knob captions ────────────────────────────────────────────
    'mix':                      ['混合', '干湿'],
    'depth':                    ['深度'],
    'spread':                   ['扩散', '展宽'],
    'rate':                     ['速率'],
    'size':                     ['尺寸', '大小'],
    'level':                    ['电平'],
    'input':                    ['输入'],
    'output':                   ['输出'],
};

// ── Character budgets. MEASURED CELLS ONLY. ─────────────────────────────────
// maxChars = floor(cellWidthPx / fontSizePx). A key ABSENT from this table is
// UNBUDGETED and lint rule Z6 is inert on it, by design — inventing a budget
// would be a number with no measurement behind it, which is worse than none.
// Stages 2-4 fill these in from the check-ui-labels zh arm as each cell is
// measured. The lint PRINTS the unbudgeted count so the inert coverage is
// disclosed rather than silent.
const CHORUS_CITE = 'plugins/O-Chorus/Source/ui/public/js/i18n.js:39-42 (62 px wrap cliff, '
    + '50 px gate cliff) at the 10 px caption size given in 260901-akh-IMPLEMENTATION-PLAN.md '
    + 'Stage 2 item 6';
const BUDGETS = {
    'depth':  { maxChars: 6, cellWidthPx: 62, fontSizePx: 10, source: CHORUS_CITE },
    'save':   { maxChars: 6, cellWidthPx: 62, fontSizePx: 10, source: CHORUS_CITE },
    'spread': { maxChars: 5, cellWidthPx: 50, fontSizePx: 10, source: CHORUS_CITE },
};

// ── Tokens that STAY ENGLISH in a zh table. ─────────────────────────────────
// These are keyed `sameAsEn: true` in a plugin's table rather than exempted, so
// a human still has to agree with each one. A token here must NOT also carry a
// TERMS rendering — the module-load assertion below refuses that overlap.
const SAME_AS_EN = ['LFO', 'MIDI', 'dB', 'Hz', 'kHz', 'ms', 'BPM'];

// ── Renderings that are wrong wherever they appear. ─────────────────────────
// Same mechanism as the French glossary: a wrong rendering mapping to what
// should have been written instead. Empty in Task 1; filled in Task 2 with
// the content the corpus warrants.
const FORBIDDEN_IN_LABELS = {};
const FORBIDDEN_IN_PROSE = {};

// Code-point iteration, NOT `.length`. CJK extension characters live above the
// BMP and are surrogate pairs, so `.length` double-counts them — a 3-character
// caption would measure 4 and Z6 would fire on a caption that fits.
function charCount(s) {
    return [...String(s)].length;
}

// ── Derived, never stored. ──────────────────────────────────────────────────
// charCount is COMPUTED at require time from the root rendering rather than
// written beside it. A stored count is a mirrored constant, and a mirrored
// constant drifts silently the first time someone edits the rendering and not
// the number — the failure mode this repo has already been bitten by in test
// fixtures that mirror plugin constants.
const TERM_META = {};
for (const [en, renderings] of Object.entries(TERMS)) {
    const zh = renderings[0];
    const b = BUDGETS[en];
    TERM_META[en] = {
        zh,
        charCount: charCount(zh),
        maxChars: b && typeof b.maxChars === 'number' ? b.maxChars : null,
    };
}

// A token cannot both stay English and carry a Chinese rendering. Fail loudly
// at load rather than let the lint quietly enforce a contradiction.
{
    const same = SAME_AS_EN.map((s) => String(s).trim().toLowerCase());
    const overlap = Object.keys(TERMS).filter((t) => same.includes(t));
    if (overlap.length)
        throw new Error(`i18n-zh-glossary: SAME_AS_EN token also carries a TERMS rendering: ${overlap.join(', ')}`);
    const orphanBudgets = Object.keys(BUDGETS).filter((k) => !TERMS[k]);
    if (orphanBudgets.length)
        throw new Error(`i18n-zh-glossary: BUDGETS key with no TERMS entry: ${orphanBudgets.join(', ')}`);
}

module.exports = {
    TERMS,
    BUDGETS,
    SAME_AS_EN,
    FORBIDDEN_IN_LABELS,
    FORBIDDEN_IN_PROSE,
    charCount,
    TERM_META,
};
