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
// should have been written instead. The content is different in one structural
// way — Chinese has no word delimiter, so the lint tests CONTAINMENT, not a
// stem with a word-boundary lookahead. Two consequences the next editor must
// keep in mind:
//
//   1. A forbidden rendering that is a SUBSTRING of a correct one will fire on
//      the correct one. 混音 is forbidden for a wet/dry Mix knob and is also the
//      first half of 混音器, the correct rendering of "Mixer". That case is
//      covered because F1 never fires on a rendering the glossary ACCEPTS for
//      the same English — so 'mixer' MUST carry 混音器 in TERMS. Check for this
//      before adding an entry.
//   2. A word that is genuinely correct somewhere in the suite does not belong
//      here at all. 轨道 ("orbit / track") was considered and REJECTED: it is
//      the wrong sense of "Track" but the right word for O-Orbit. A termNote
//      would be needed on every O-Orbit entry to buy one catch elsewhere, which
//      is a bad trade. 发布 stays out of the PROSE table for the same reason —
//      "v1.2 发布" is a correct sentence about a software release.
//
// Every entry below is a machine-translation TELL: the general-purpose sense of
// an English homograph, rendered into Chinese with no audio context.
const FORBIDDEN_IN_LABELS = {
    '\u6df7\u97f3':    '\u6df7\u5408 (\u6216 \u5e72\u6e7f) — \u6df7\u97f3 is the mixing PROCESS, not a wet/dry blend',
    '\u653b\u51fb':    '\u8d77\u97f3 — \u653b\u51fb is a military attack',
    '\u53d1\u5e03':    '\u91ca\u97f3 (\u91ca\u653e) — \u53d1\u5e03 is publishing a product',
    '\u8870\u53d8':    '\u8870\u51cf — \u8870\u53d8 is radioactive decay',
    '\u83b7\u5f97':    '\u589e\u76ca — \u83b7\u5f97 is to obtain something',
    '\u8282\u7701':    '\u4fdd\u5b58 — \u8282\u7701 is to economise',
    '\u94a5\u5319':    '\u8c03 — \u94a5\u5319 is a door key',
    '\u5e73\u5e95\u9505': '\u58f0\u50cf — a pan pot, not a frying pan',
    '\u7b14\u8bb0':    '\u97f3\u7b26 — \u7b14\u8bb0 is a written note',
    '\u89c4\u6a21':    '\u97f3\u9636 — \u89c4\u6a21 is scale in the sense of magnitude',
    '\u7403\u573a':    '\u97f3\u9ad8 — \u7403\u573a is a sports pitch',
    '\u9152\u5427':    '\u5c0f\u8282 — \u9152\u5427 is a drinking bar',
    '\u626c\u673a':    '\u89e6\u53d1 — \u626c\u673a is a gun trigger',
    '\u9a7e\u9a76':    '\u9a71\u52a8 (\u8fc7\u8f7d) — \u9a7e\u9a76 is to drive a vehicle',
    '\u8fc7\u6ee4\u5668': '\u6ee4\u6ce2\u5668 — \u8fc7\u6ee4\u5668 is a water or air filter',
    '\u6837\u54c1':    '\u6837\u672c (\u91c7\u6837) — \u6837\u54c1 is a merchandise sample',
};

// Renderings that are wrong in tooltip PROSE too — the small, unambiguous set.
// Deliberately shorter than the label table: prose has room for a word that
// would be wrong as a caption, so only the unarguable tells are listed.
const FORBIDDEN_IN_PROSE = {
    '\u63d2\u5934':    '\u63d2\u4ef6 — \u63d2\u5934 is an electrical plug',
    '\u8fc7\u6ee4\u5668': '\u6ee4\u6ce2\u5668',
    '\u6837\u54c1':    '\u6837\u672c / \u91c7\u6837',
    '\u653b\u51fb':    '\u8d77\u97f3',
    '\u5e73\u5e95\u9505': '\u58f0\u50cf',
};

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
