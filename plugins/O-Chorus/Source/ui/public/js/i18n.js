/*
   This file is part of O-Chorus, an Ouaricon Audio plugin.
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
// i18n.js — O-Chorus page labels, English + French (v1.3.0)
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
// v1.2.3 carried no data-tip and no data-tooltip anywhere on the page — only
// four native title= attributes on the preset bar, which contract §4 DELETES
// rather than localizes, moving their existing text into data-i18n-aria. No
// hover-help prose is INVENTED here: authoring it is Stage M's job. I18N is
// therefore empty and TIP_BINDINGS is empty, which is this plugin's correct
// state rather than a gap. check-i18n assertion 2 reports it as "0 tip(s)
// bound" instead of passing silently, and the emptiness is only admissible
// BECAUSE no I18N entry carries a body — an emptied TIP_BINDINGS over a bodied
// table would be orphaned copy and would fail.
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
// fallback must exist even on a plugin with no tooltips today, so Stage M can
// add bodies here without touching the label keys below.
// ============================================================================

export const I18N = Object.freeze({});

// ============================================================================
// LABELS — the visible text of the page. {en:{t}, fr:{t, reviewed}}.
//
// One string per entry, no body: a label is not a tooltip.
//
// ── THE FRAME IS 700 x 125, THE SHORTEST IN THE REPO ────────────────────────
//
// 125 px of vertical space. A caption that gains a line has nowhere to go, so
// every French string below was chosen against a MEASURED width, rendered in
// the real node with its own text-transform and letter-spacing (neither of
// which appears in getComputedStyle().font).
//
// ── THE TWO CLIFFS UNDER A KNOB CAPTION, AND THEY ARE NOT THE SAME NUMBER ───
//
// .knob-label is a flex item of .knob, which is itself a flex item of the
// fixed 62 px .knob-container with align-items: center. So .knob's width is
// fit-content clamped to 62:  max(48 visual, 50 .knob-value min-width, caption).
//
//   50.00 px — THE GATE CLIFF. Above it .knob's own rectangle widens with the
//              language. .knob is not a [data-i18n] element, so check-ui-labels
//              assertion 7 reports it as moved. Nothing a user can see: the
//              visual and the value stay centred on the identical absolute
//              coordinates either way (verified: at .knob w=55 the visual is
//              still x=42.5 and the value still x=41.5).
//   62.00 px — THE WRAP CLIFF, and the one that matters in this frame. Past
//              the container width the caption wraps to two lines, .knob grows
//              from 73 to 83 px tall and pushes .knob-value down 10 px.
//
// MEASURED, at 700 x 125, rendered text width against the 50 px gate cliff:
//
//     Rate   25.70 -> VITESSE 41.61   8.39 spare
//     Depth  33.00 -> PROF.   28.05  21.95 spare   SHRANK
//     Voices 37.31 -> VOIX    25.70  24.30 spare   SHRANK
//     Spread 39.31 -> ECART   32.97  17.03 spare   SHRANK
//     Width  34.00 -> LARGEUR 48.11   1.89 spare   <- the tightest on the page
//     Tone   27.05 -> TIMBRE  38.81  11.19 spare
//     Mix    19.91 -> DOSAGE  41.31   8.69 spare
//     Drive  31.50 -> SATUR.  35.56  14.44 spare
//     LFO    18.72 -> LFO     18.72   sameAsEn
//
// THREE OF EIGHT SHRINK. A clip-only check would have certified this page.
//
// ── GEOMETRY ────────────────────────────────────────────────────────────────
//
// One pin ships, on .preset-action, and it is load-bearing rather than
// decorative — see the note there in index.html. Nothing else needed one: the
// eight knob captions all land under the 50 px gate cliff, so .knob stays
// exactly 50 px wide in both languages and no knob element moves at all.
// ============================================================================

export const LABELS = Object.freeze({

    // ── The eight knob captions ─────────────────────────────────────────────
    //
    // D-01 arm 1 does not apply anywhere on this page: O-Chorus has NO
    // AudioParameterChoice at all. Its eight parameters are seven
    // AudioParameterFloat and one AudioParameterInt (PluginProcessor.cpp:37-61),
    // and neither type has option strings for a French caption to disagree
    // with in the host automation lane. Arm 3 does not apply either — every
    // caption below is a .knob-label span that never holds a number; the number
    // lives in its own .knob-value sibling, so contract §5's split already
    // exists in the authored markup and nothing had to be split here.

    // "Vitesse" rather than "Taux": this is the LFO's rate in Hz, and a French
    // modulation section calls that its speed.
    'label.rate': { en: { t: 'Rate' }, fr: { t: 'VITESSE', reviewed: false } },

    // PROFONDEUR is the word a French user expects and it does not fit: 68.02
    // px against a 62 px wrap cliff, so it would render on two lines and push
    // the value readout down inside a 125 px frame. AMPLEUR fits at 48.61 but
    // leaves 1.39 px against the gate cliff and means "breadth" rather than
    // "depth". PROF. is the standard French abbreviation OF the expected word,
    // and it is the only option that is both recognisable and comfortable.
    'label.depth': { en: { t: 'Depth' }, fr: { t: 'PROF.', reviewed: false } },

    'label.voices': { en: { t: 'Voices' }, fr: { t: 'VOIX', reviewed: false } },

    // Spread offsets the voices' LFO phases and delay times from one another,
    // so the quantity is the gap between them. ETALEMENT (60.47) and
    // DISPERSION (60.02) are both nearer the wrap cliff than the gate cliff.
    'label.spread': { en: { t: 'Spread' }, fr: { t: 'ÉCART', reviewed: false } },

    // THE TIGHTEST STRING ON THE PAGE, 1.89 px under the gate cliff. Crossing
    // it widens .knob by fractions of a pixel and nothing else; the wrap cliff
    // is 13.89 px further out. STÉRÉO measures 38.81 and is the obvious lever
    // if a reviewer wants margin rather than the literal translation.
    'label.width': { en: { t: 'Width' }, fr: { t: 'LARGEUR', reviewed: false } },

    // A tilt control, dark to bright. "Timbre" is the French word for that
    // quality; TONALITÉ measures 50.73 and would cross the gate cliff.
    'label.tone': { en: { t: 'Tone' }, fr: { t: 'TIMBRE', reviewed: false } },

    // The wet/dry blend. "Dosage" is what a French plugin calls it.
    'label.mix': { en: { t: 'Mix' }, fr: { t: 'DOSAGE', reviewed: false } },

    // SATURATION measures 63.52 — past the WRAP cliff, not merely the gate one,
    // so the full word would put a second line under this knob. SATUR. is the
    // abbreviation of the actual DSP (a tanh drive stage), which is why it is
    // preferred over CHALEUR (48.11, "warmth" — a marketing word for the same
    // thing, and 1.89 px from the gate cliff).
    'label.drive': { en: { t: 'Drive' }, fr: { t: 'SATUR.', reviewed: false } },

    // ── The LFO ring heading ────────────────────────────────────────────────
    //
    // Keyed with sameAsEn rather than exempted, deliberately. LFO is spelled
    // LFO in French audio software, but that is a TRANSLATION JUDGEMENT and an
    // I18N_EXEMPT entry would hide it from the native-speaker worklist forever.
    // Keyed, it is one more `reviewed: false` line somebody has to agree with.
    'label.lfo': { en: { t: 'LFO' }, fr: { t: 'LFO', reviewed: false, sameAsEn: true } },

    // ── The two preset buttons ──────────────────────────────────────────────
    //
    // .preset-action is PINNED to 62 px for these two (index.html). Rendered
    // border-box widths against that pin — text + 10 px padding + 2 px border:
    //
    //     Load 39.00 -> CHARGER 58.52   3.48 px spare
    //     Save 36.34 -> SAUVER  51.02  10.98 px spare
    //
    // ENREGISTRER is the word a French user would rather see and it needs an
    // 78.52 px box — a 26 px widening of BOTH buttons over what ships here,
    // which moves the preset arrows and the preset name a further 32 px left in
    // ENGLISH. A reviewer who upgrades SAUVER to ENREGISTRER must raise the
    // .preset-action pin with it; leaving the pin at 62 would wrap an
    // 11-character caption inside a 14 px-high button, which is the failure
    // shape check-ui-labels gained a vertical assertion for in fbdb6930.
    'label.load': { en: { t: 'Load' }, fr: { t: 'CHARGER', reviewed: false } },
    'label.save': { en: { t: 'Save' }, fr: { t: 'SAUVER', reviewed: false } },

    // ── The settings popover (v1.3.0) ───────────────────────────────────────
    'label.language': { en: { t: 'Language' }, fr: { t: 'Langue', reviewed: false } },

    // ── Accessible names ────────────────────────────────────────────────────
    //
    // Resolved through the same sweep via data-i18n-aria, so a screen reader
    // hears the language the page is showing.
    //
    // THE FOUR PRESET-BAR NAMES ARE THE DELETED title= TEXT, MOVED, NOT
    // AUTHORED. v1.2.3 carried title="Previous preset", "Next preset", "Load
    // preset from file" and "Save preset"; contract §4 deletes the native
    // attribute (it renders a second, untranslated OS tooltip) and moves its
    // existing English into the accessible name. Every English string below is
    // byte-identical to what v1.2.3 shipped. Nothing new was invented.
    //
    // LABEL IN NAME. #preset-load and #preset-save carry BOTH a visible caption
    // and an aria-label, and an aria-label REPLACES the accessible name rather
    // than extending it. Each accessible name therefore CONTAINS its own
    // visible caption as a prefix — "Load" in "Load preset from file",
    // "CHARGER" in "Charger un préréglage depuis un fichier" — so a voice
    // control user saying the caption still hits the button (WCAG 2.5.3). This
    // is the constraint O-Texture's "Metal — coming soon" landed on from the
    // other direction.
    'aria.prevPreset': { en: { t: 'Previous preset' },        fr: { t: 'Préréglage précédent',                 reviewed: false } },
    'aria.nextPreset': { en: { t: 'Next preset' },            fr: { t: 'Préréglage suivant',                   reviewed: false } },
    'aria.loadPreset': { en: { t: 'Load preset from file' },  fr: { t: 'Charger un préréglage depuis un fichier', reviewed: false } },
    'aria.savePreset': { en: { t: 'Save preset' },            fr: { t: 'Sauver le préréglage',                 reviewed: false } },

    'aria.settings':   { en: { t: 'Settings' },           fr: { t: 'Réglages',              reviewed: false } },
    'aria.langSelect': { en: { t: 'Interface language' }, fr: { t: 'Langue de l’interface', reviewed: false } },
});

// ============================================================================
// I18N_EXEMPT — reasoned exclusions, never silence
//
// Every visible string the coverage scan finds must be a [data-i18n] element, a
// setLabel() call, or an entry HERE WITH A REASON. A bare skip list would let a
// missed label hide as a deliberate one.
//
// AN ENTRY IS [text, reason] OR [text, reason, scope]. An exemption is matched
// by TEXT, so an unscoped one silences EVERY node carrying that string. A scope
// is REQUIRED exactly when a string is exempt AND keyed on the same page, which
// is the one state the gate cannot tell a deliberate skip from a forgotten
// label (check-i18n assertion 14). NONE of the four below is in that state:
// no key on this page resolves to any of these strings, so all four are
// correctly unscoped and assertion 14 passes without one.
// ============================================================================

export const I18N_EXEMPT = [
    ['Ouaricon Chorus',
     'the product display name in .title — a product name is never translated, and this is the brand-plus-product form of the plugin\'s registered PRODUCT_NAME "O-Chorus" in CMakeLists.txt'],

    // ── D-02: the preset name IS the filename ───────────────────────────────
    ['Default',
     'the PRESET NAME shown in #preset-display, not a caption — D-02. The name is the JSON filename on disk (OuariconPresetManager sanitizes it into getUserPresetsDirectory()), and it is written into this node at runtime by the VENDORED modules/preset-manager.js, which is shared across plugins: localizing it here would rename presets in one language and orphan the files'],

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
