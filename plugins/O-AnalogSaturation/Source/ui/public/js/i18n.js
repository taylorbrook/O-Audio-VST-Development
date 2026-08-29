/*
   This file is part of O-AnalogSaturation, an Ouaricon Audio plugin.
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
// i18n.js — O-AnalogSaturation page labels, English + French (v1.2.0)
//
// An ES module that EXPORTS ONLY. It must never self-execute: a bare top-level
// statement here throws out of module evaluation and takes every later
// initializer on the page with it (pattern_module_toplevel_init_tdz). This
// plugin's controller is an inline <script type="module"> in index.html, so
// that failure mode would take the WHOLE UI, not a panel of it.
// scripts/check-i18n.js assertion 7 enforces it.
//
// FILENAME: no hyphen. juce_add_binary_data STRIPS hyphens rather than
// converting them to underscores, so a second file named i18n-fr.js would have
// to be reached as the symbol i18nfr_js (critical_binary_data_strips_hyphens).
// One combined file for both languages sidesteps the question entirely.
//
// ── THIS PLUGIN HAS NO HOVER-HELP, AND THIS COMMIT DOES NOT GIVE IT ANY ─────
//
// v1.1.6 carried no data-tip, no data-tooltip and no native title= anywhere on
// the page — the boot report reads `title= 0`. So there is no tooltip copy to
// MOVE here and none is INVENTED: authoring hover-help prose is Stage M's job.
// I18N is therefore empty and TIP_BINDINGS is empty, which is this plugin's
// correct state rather than a gap. check-i18n assertion 2 reports it as
// "0 tip(s) bound" instead of passing silently, and the emptiness is only
// admissible BECAUSE no I18N entry carries a body — an emptied TIP_BINDINGS
// over a bodied table would be orphaned copy and would fail.
//
// COPY IS textContent ON EVERY PATH — never innerHTML. check-i18n assertion 9
// rejects any innerHTML reference here and any string literal containing an
// angle bracket, so machine-drafted French cannot open a markup path.
//
// ALL FRENCH IS MACHINE-DRAFTED AND FLAGGED `reviewed: false`. No native
// speaker has read it. `node scripts/check-i18n.js` prints the worklist.
// ============================================================================

export const LANGUAGES = ['en', 'fr'];

// ============================================================================
// I18N — hover-help copy. EMPTY, deliberately.
//
// A tooltip entry is {t, b}: a title and a body. This page has neither, so the
// table has no entries. It is exported all the same because the canonical
// import line names it and trLabel() falls back through it — a control whose
// tooltip title already IS its caption is meant to carry ONE key, and that
// fallback must exist even on a plugin that has no tooltips today, so Stage M
// can add bodies here without touching the label keys below.
// ============================================================================

export const I18N = Object.freeze({});

// ============================================================================
// LABELS — the visible text of the page. {en:{t}, fr:{t, reviewed}}.
//
// One string per entry, no body: a label is not a tooltip.
//
// ── WHAT IS *NOT* HERE, AND WHY (the D-01 test) ─────────────────────────────
//
// Seven of the thirteen visible strings on this page are EXEMPT, and every one
// of them is an I18N_EXEMPT entry with its reason rather than a silent skip.
// The four model captions and the three quality captions are the
// AudioParameterChoice option strings VERBATIM — see the note in I18N_EXEMPT.
//
// ── GEOMETRY ────────────────────────────────────────────────────────────────
//
// Every label on this page lives in an ABSOLUTELY POSITIONED box or a
// fixed-width one, so no French string can push a sibling: .knob-label is
// `left: 175px; width: 90px; text-align: center`, .vu-label is
// `left: 50%; transform: translateX(-50%)`, .quality-label is a bare absolute
// box with no following sibling in flow, and .autogain-toggle is a hard
// `width: 100px`. The French was still chosen to fit rather than to rely on
// that. MEASURED at 600 x 450, rendered text width against the box it sits in:
//
//     .autogain-toggle   AUTOGAIN  57.4 -> GAIN AUTO 59.9  in an 84.0 content
//                        box (100 px border-box, 2 px borders, the UA button's
//                        own 6 px side padding) — 24.1 px spare
//     .knob-label        INTENSITY 68.2 -> INTENSITÉ 67.0  in 90 px — SHRANK
//     .quality-label     QUALITY   47.0 -> QUALITÉ   46.0  — SHRANK
//     .vu-label in       IN         8.9 -> ENTRÉE    32.9  centred by transform
//                        in a 90 px face: 83.5..116.4, well inside 55..145
//     .vu-label out      OUT       17.3 -> SORTIE    30.0  — same, inside
//
// Two of the five SHRINK rather than grow, which is the half a clip check is
// blind to and the half Stage J found four times in twelve.
//
// The check-ui-labels assertion-7 diff reports zero moved elements, and that
// verdict is NOT vacuous: a negative control that lengthened label.language
// until it pushed #lang-select inside the popover row made assertion 7 report
// the move by name, so the sweep can see this page.
// ============================================================================

export const LABELS = Object.freeze({

    // ── The two VU meters ───────────────────────────────────────────────────
    // Full words rather than the ENT/SORT abbreviations: .vu-label is centred
    // by transform inside a 90 px face with nothing beside it, so the extra
    // characters cost nothing and the meaning is not left to be guessed.
    'label.in':  { en: { t: 'IN' },  fr: { t: 'ENTRÉE', reviewed: false } },
    'label.out': { en: { t: 'OUT' }, fr: { t: 'SORTIE', reviewed: false } },

    // ── The intensity knob ──────────────────────────────────────────────────
    // The caption under the knob, NOT a readout: this node never holds a
    // number. The knob has no numeric readout at all on this page — the value
    // is shown by the indicator dot and by the snake's opacity — so there is no
    // readout/label node to split (contract §5).
    'label.intensity': { en: { t: 'INTENSITY' }, fr: { t: 'INTENSITÉ', reviewed: false } },

    // ── The quality section heading ─────────────────────────────────────────
    // The heading localizes; the three BUTTONS under it do not. The heading is
    // this page's own caption for the group, and "Quality" is the
    // AudioParameterChoice's DISPLAY NAME rather than one of its option
    // strings, so nothing in the host automation lane is spelled "QUALITY".
    'label.quality': { en: { t: 'QUALITY' }, fr: { t: 'QUALITÉ', reviewed: false } },

    // ── The auto-gain toggle ────────────────────────────────────────────────
    // AUTOGAIN is the APVTS parameter ID, not a choice option — the parameter
    // is an AudioParameterBool whose display name is "Auto Gain"
    // (PluginProcessor.cpp:63-67). A bool has no option strings, so there is no
    // automation-lane string for a French caption to disagree with, and arm 1
    // of D-01 does not apply. GAIN AUTO is the standard French word order.
    'label.autogain': { en: { t: 'AUTOGAIN' }, fr: { t: 'GAIN AUTO', reviewed: false } },

    // ── The settings popover (v1.2.0) ───────────────────────────────────────
    'label.language': { en: { t: 'Language' }, fr: { t: 'Langue', reviewed: false } },

    // ── Accessible names ────────────────────────────────────────────────────
    // Resolved through the same sweep via data-i18n-aria / data-i18n-alt, so a
    // screen reader hears the same language the page is showing.
    'aria.settings':   { en: { t: 'Settings' },           fr: { t: 'Réglages',              reviewed: false } },
    'aria.langSelect': { en: { t: 'Interface language' }, fr: { t: 'Langue de l’interface', reviewed: false } },

    // The decorative plate behind the knob. Its alt text was the page's only
    // prose-bearing attribute at v1.1.6 and was unkeyed; it is keyed here
    // rather than emptied, because the illustration changes with the model and
    // a blind user is entitled to know something is there.
    'alt.snake': { en: { t: 'Snake illustration' }, fr: { t: 'Illustration de serpent', reviewed: false } },
});

// ============================================================================
// I18N_EXEMPT — reasoned exclusions, never silence
//
// Every visible string the coverage scan finds must be a [data-i18n] element, a
// setLabel() call, or an entry HERE WITH A REASON. A bare skip list would let a
// missed label hide as a deliberate one.
// ============================================================================

export const I18N_EXEMPT = [
    ['OUARICON SATURATION',
     'the product display name in .title — a product name is never translated, and this is the brand-plus-product form of the plugin\'s registered PRODUCT_NAME "O-AnalogSaturation" in CMakeLists.txt'],

    // ── D-01 arm 1: the captions that ARE the option strings ────────────────
    //
    // The four model buttons and the three quality buttons carry the
    // AudioParameterChoice option strings BYTE FOR BYTE
    // (PluginProcessor.cpp:47-61). Translating the caption alone would make the
    // page and the host automation lane disagree about the same setting: a DAW
    // showing MODEL = "TRANSFORMER" beside a page reading "TRANSFO" is a bug
    // report, not a localization.
    //
    // Byte-identity is the test, and it is the reason these seven differ from
    // O-Gain's LOW/MED/HIGH confidence verdict, which localizes: that node is
    // not backed by a parameter at all.
    ['MAGNETIC',
     'a MODEL AudioParameterChoice option string VERBATIM (PluginProcessor.cpp:50, StringArray {"MAGNETIC","TUBE","TRANSFORMER","DIODE"}) — D-01 arm 1'],
    ['TUBE',
     'a MODEL option string VERBATIM (PluginProcessor.cpp:50) — D-01 arm 1'],
    ['TRANSFORMER',
     'a MODEL option string VERBATIM (PluginProcessor.cpp:50) — D-01 arm 1'],
    ['DIODE',
     'a MODEL option string VERBATIM (PluginProcessor.cpp:50) — D-01 arm 1. Also spelled identically in French'],
    ['LOW',
     'a QUALITY AudioParameterChoice option string VERBATIM (PluginProcessor.cpp:58, StringArray {"LOW","MID","HIGH"}) — D-01 arm 1'],
    ['MID',
     'a QUALITY option string VERBATIM (PluginProcessor.cpp:58) — D-01 arm 1'],
    ['HIGH',
     'a QUALITY option string VERBATIM (PluginProcessor.cpp:58) — D-01 arm 1'],

    // ── Endonyms ────────────────────────────────────────────────────────────
    ['English',  'endonym — a language name is never translated'],
    ['Français', 'endonym — a language name is never translated'],
];

// ============================================================================
// TIP_BINDINGS — EMPTY. See the header: this plugin has no hover-help.
//
// Exported because the canonical import line names it and applyI18n() iterates
// it. A zero-length loop is the correct no-op; the alternative — omitting the
// export and editing the canon block to match — would put this plugin's copy of
// the runtime out of step with the other forty-two, which is the whole drift
// the canon gate exists to prevent.
// ============================================================================

export const TIP_BINDINGS = [];

// The tooltip lookup. Returns {t, b} — never null, never a bare key without a
// console.warn saying so, because a silently-missing tip renders as an empty
// surface that looks like a positioning bug rather than a missing entry.
//
// Unreferenced at runtime today: applyI18n() calls it only from the
// TIP_BINDINGS loop, which is empty. It is exported verbatim all the same, so
// that the canon block is byte-identical to the other forty-two copies and
// Stage M can add bodies to I18N without touching this file's shape.
export function tr(key, lang, vars) {
    const entry = I18N[key];
    if (!entry) { console.warn(`i18n: missing key ${key}`); return { t: key, b: '' }; }
    const s = entry[lang] || entry.en;

    // A var VALUE that is itself an I18N key resolves to that key's localized
    // title; anything else is used literally.
    const resolve = (v) => {
        const nested = I18N[v];
        return nested ? String((nested[lang] || nested.en).t) : String(v);
    };

    const fill = (str) => (vars
        ? String(str).replace(/\{(\w+)\}/g, (m, n) => (n in vars ? resolve(vars[n]) : m))
        : String(str));

    return { t: fill(s.t), b: fill(s.b) };
}
