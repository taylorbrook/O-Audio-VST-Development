/*
   This file is part of O-Texture, an Ouaricon Audio plugin.
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
// i18n.js — O-Texture page labels, English + French (v0.2.0)
//
// An ES module that EXPORTS ONLY. It must never self-execute: a bare top-level
// statement here throws out of module evaluation and takes every later
// initializer on the page with it (pattern_module_toplevel_init_tdz).
// scripts/check-i18n.js assertion 7 enforces it.
//
// FILENAME: no hyphen. juce_add_binary_data STRIPS hyphens rather than
// converting them to underscores, so a second file named i18n-fr.js would have
// to be reached as the symbol i18nfr_js (critical_binary_data_strips_hyphens).
// One combined file for both languages sidesteps the question entirely.
//
// ── THIS PLUGIN HAS NO HOVER-HELP, AND THIS COMMIT DOES NOT GIVE IT ANY ─────
//
// v0.1.2 carried no data-tip and no data-tooltip anywhere on the page — only
// six stray native title="Coming soon" attributes, all six of them deleted
// here per contract section 4. So there is no tooltip copy to MOVE and none is
// INVENTED: authoring hover-help prose is Stage M's job. I18N is therefore
// empty and TIP_BINDINGS is empty, which is this plugin's correct state rather
// than a gap. check-i18n assertion 2 reports it as "0 tip(s) bound" instead of
// passing silently, and the emptiness is only admissible BECAUSE no I18N entry
// carries a body — an emptied TIP_BINDINGS over a bodied table would be
// orphaned copy and would fail.
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
// I18N — hover-help copy. EMPTY, deliberately. See the header.
//
// Exported all the same because the canonical import line names it and
// trLabel() falls back through it — a control whose tooltip title already IS
// its caption is meant to carry ONE key, and that fallback must exist even on a
// plugin that has no tooltips today, so Stage M can add bodies here without
// touching the label keys below.
// ============================================================================

export const I18N = Object.freeze({});

// ============================================================================
// LABELS — the visible text of the page. {en:{t}, fr:{t, reviewed}}.
//
// One string per entry, no body: a label is not a tooltip.
//
// ── WHAT IS *NOT* HERE, AND WHY (the D-01 test) ─────────────────────────────
//
// NINE of this page's fifteen visible strings are EXEMPT, and every one is an
// I18N_EXEMPT entry with its reason rather than a silent skip. Eight of the
// nine are AudioParameterChoice option strings verbatim — see I18N_EXEMPT.
// That is an unusually high exempt fraction and it is a property of this
// plugin: the source row and the mode row are its two largest caption groups
// and BOTH are choice-backed.
//
// Arm 3 of D-01 (a readout node is never a [data-i18n] element) is live here
// but needed no judgement: .value and .knob-value are already separate sibling
// nodes from .label and .knob-label, written only by main.js as
// scaledValue.toFixed(2). No node on this page carries a caption and a number
// at once, so contract section 5 required no split.
//
// ── GEOMETRY — MEASURED at the shipping 800 x 600, not reasoned ─────────────
//
// The three vertical-slider captions are the tight ones. .vertical-slider is a
// hard `width: 50px` COLUMN flex whose .slider-track takes `height: 100%`, and
// each caption is a shrink-wrapped flex item inside it. So a caption that grows
// past 50 px does one of two things, both bad and both invisible to a clip
// check: a MULTI-WORD one wraps to a second line and SHORTENS THE TRACK by
// 12 px, moving the thumb and the value readout under it; a SINGLE-WORD one
// cannot wrap at all and simply overhangs the 8 px gap into the slider beside
// it. The first is a rect change on a non-label element — assertion 7 — and the
// second is assertion 5.
//
// Rendered text width against the box it sits in, both languages, both states:
//
//     .label charA    Char A      36.9 -> Car. A     33.4   SHRANK   (50 px col)
//     .label charB    Char B      36.7 -> Car. B     33.2   SHRANK   (50 px col)
//     .label evolve   Evolve      35.8 -> Évol.      27.5   SHRANK   (50 px col)
//     .knob-label     Brightness  57.5 -> Brillance  48.5   SHRANK   (64 px box)
//     .knob-label     Mix         20.6 -> Mélange    45.5   grew     (64 px box)
//     .freeze-label   FREEZE      45.8 -> GEL        23.5   SHRANK   (64 px box)
//     .settings-label Language    51.5 -> Langue     39.2   SHRANK   (196 px row)
//
// SIX OF SEVEN SHRINK. This page is the shrink case, not the growth case, and a
// clip-only check would have certified it while seeing nothing. The one that
// grows, Mélange at 45.5, sits in a .knob-container that shrink-wraps to the
// 64 px knob above it, so it does not reach the floor and .bottom-strip's
// space-around distribution stays language-invariant by construction — measured
// identical at [117.3, 477, 64, 99] in both languages.
//
// No pin was needed and none was added, so there is no decorative pin in this
// change. The one geometry declaration that IS here — min-height: 0 on
// .main-area — is not a French pin at all; it fixes an English layout that grew
// without bound, and its negative control fires six assertions. Its reasoning
// is in the stylesheet beside it.
// ============================================================================

export const LABELS = Object.freeze({

    // ── The three vertical sliders ──────────────────────────────────────────
    // "Char A" is already an ABBREVIATION of the parameter's display name,
    // "Character A" (PluginProcessor.cpp:350), and the French keeps that shape
    // because the 50 px column leaves no room for the word. MEASURED in that
    // column: "Caractère A" is 51.5 px and WRAPS to two lines, shortening the
    // slider track from 271 px to 259; "Timbre A" is 39 px of text but also
    // wraps, because the flex item clamps to 50 and the space is a break
    // opportunity; "Caract. A" does fit on one line at 49.4 px, but with 0.6 px
    // of margin — thinner than any margin this rollout has accepted, and the
    // Windows/WebView2 font metrics that would decide it are the named
    // hardware-blocked deferral. "Car. A" is 33.4 px with 16.6 px to spare.
    'label.charA':  { en: { t: 'Char A' }, fr: { t: 'Car. A', reviewed: false } },
    'label.charB':  { en: { t: 'Char B' }, fr: { t: 'Car. B', reviewed: false } },

    // Same 50 px budget, and the single-word case. MEASURED: "Évolution" is
    // 52.5 px and is ONE WORD, so it cannot wrap — it overhangs the 50 px
    // column into the 8 px gap beside it. "Évolue" fits at 36.7 px but is a
    // conjugated verb where the two neighbours are noun abbreviations.
    // "Évol." is 27.5 px and matches their shape.
    'label.evolve': { en: { t: 'Evolve' }, fr: { t: 'Évol.', reviewed: false } },

    // ── The two knobs ───────────────────────────────────────────────────────
    // Captions, NOT readouts: .knob-value is a separate sibling node and is the
    // only thing that ever holds a number here (D-01 arm 3, contract 5).
    'label.brightness': { en: { t: 'Brightness' }, fr: { t: 'Brillance', reviewed: false } },

    // MIX is the APVTS parameter ID and "Mix" its display name, not a choice
    // option — an AudioParameterFloat has no option strings for a French
    // caption to disagree with in the automation lane, so arm 1 does not apply
    // and this localizes.
    'label.mix': { en: { t: 'Mix' }, fr: { t: 'Mélange', reviewed: false } },

    // FREEZE is an AudioParameterBool. Same reasoning as MIX: no option
    // strings, so nothing in the host is spelled "Freeze" for this to contradict.
    // .freeze-label is text-transform: uppercase, so the table holds the
    // authored case and the page renders GEL.
    'label.freeze': { en: { t: 'Freeze' }, fr: { t: 'Gel', reviewed: false } },

    // ── The settings popover (v0.2.0) ───────────────────────────────────────
    'label.language': { en: { t: 'Language' }, fr: { t: 'Langue', reviewed: false } },

    // ── Accessible names ────────────────────────────────────────────────────
    // Resolved through the same sweep via data-i18n-aria, so a screen reader
    // hears the same language the page is showing.
    'aria.settings':   { en: { t: 'Settings' },           fr: { t: 'Réglages',              reviewed: false } },
    'aria.langSelect': { en: { t: 'Interface language' }, fr: { t: 'Langue de l’interface', reviewed: false } },

    // ── The six not-yet-implemented controls ────────────────────────────────
    //
    // Each of these replaces a native title="Coming soon", DELETED per contract
    // section 4. The name is the button's OWN caption plus the status text the
    // title already carried — nothing is authored that was not already on the
    // page. A single shared 'Coming soon' key was the obvious shape and is
    // WRONG: aria-label REPLACES an accessible name, so it would have erased
    // "Metal" from the button whose visible caption is Metal, breaking the
    // label-in-name match a screen-reader user relies on.
    //
    // The identifier half stays byte-identical in French for the same reason
    // the visible caption does — it is a SOURCE / MODE choice option (D-01
    // arm 1). Only the status half is translated.
    'aria.soon.transform': { en: { t: 'Transform — coming soon' }, fr: { t: 'Transform — bientôt disponible', reviewed: false } },
    'aria.soon.metal':     { en: { t: 'Metal — coming soon' },     fr: { t: 'Metal — bientôt disponible',     reviewed: false } },
    'aria.soon.wind':      { en: { t: 'Wind — coming soon' },      fr: { t: 'Wind — bientôt disponible',      reviewed: false } },
    'aria.soon.crowd':     { en: { t: 'Crowd — coming soon' },     fr: { t: 'Crowd — bientôt disponible',     reviewed: false } },
    'aria.soon.synth':     { en: { t: 'Synth — coming soon' },     fr: { t: 'Synth — bientôt disponible',     reviewed: false } },
    'aria.soon.organic':   { en: { t: 'Organic — coming soon' },   fr: { t: 'Organic — bientôt disponible',   reviewed: false } },
});

// ============================================================================
// I18N_EXEMPT — reasoned exclusions, never silence
//
// Every visible string the coverage scan finds must be a [data-i18n] element, a
// setLabel() call, or an entry HERE WITH A REASON. A bare skip list would let a
// missed label hide as a deliberate one.
// ============================================================================

export const I18N_EXEMPT = [
    ['O-TEXTURE',
     'the product display name in the h1 — a product name is never translated, and this is the uppercase form of the plugin\'s registered PRODUCT_NAME "O-Texture" in CMakeLists.txt:41'],

    // ── D-01 arm 1: the captions that ARE the option strings ────────────────
    //
    // The six source buttons and the two mode buttons carry the SOURCE and MODE
    // AudioParameterChoice option strings BYTE FOR BYTE. Translating the
    // caption alone would make the page and the host automation lane disagree
    // about the same setting: a DAW showing SOURCE = "Rain" beside a page
    // reading "Pluie" is a bug report, not a localization.
    //
    // Byte-identity is the test. Note that four of the six — Metal, Crowd,
    // Synth, Organic — have perfectly good French words that are NOT used here
    // for exactly that reason.
    ['Rain',
     'a SOURCE AudioParameterChoice option string VERBATIM (PluginProcessor.cpp:335, StringArray {"Rain","Metal","Wind","Crowd","Synth","Organic"}) — D-01 arm 1'],
    ['Metal',    'a SOURCE option string VERBATIM (PluginProcessor.cpp:335) — D-01 arm 1'],
    ['Wind',     'a SOURCE option string VERBATIM (PluginProcessor.cpp:335) — D-01 arm 1'],
    ['Crowd',    'a SOURCE option string VERBATIM (PluginProcessor.cpp:335) — D-01 arm 1'],
    ['Synth',    'a SOURCE option string VERBATIM (PluginProcessor.cpp:335) — D-01 arm 1'],
    ['Organic',  'a SOURCE option string VERBATIM (PluginProcessor.cpp:335) — D-01 arm 1'],

    ['Generate',
     'a MODE AudioParameterChoice option string VERBATIM (PluginProcessor.cpp:339, StringArray {"Generate","Transform"}) — D-01 arm 1'],
    ['Transform',
     'a MODE option string VERBATIM (PluginProcessor.cpp:339) — D-01 arm 1. Spelled identically in French in any case'],

    // ── Endonyms ────────────────────────────────────────────────────────────
    ['English',  'endonym — a language name is never translated'],
    ['Français', 'endonym — a language name is never translated'],

    // ── The gear glyph ──────────────────────────────────────────────────────
    ['⚙', 'the GEAR SYMBOL U+2699 is the settings button\'s only content — a pictograph, not prose. Its meaning is carried by data-i18n-aria="aria.settings", which IS localized'],
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
