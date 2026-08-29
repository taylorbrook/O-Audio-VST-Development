/*
   This file is part of O-AnalogEQ, an Ouaricon Audio plugin.
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
// i18n.js — O-AnalogEQ page labels, English + French (v1.2.0)
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
// v1.1.11 carried no data-tip and no data-tooltip anywhere on the page — only
// five native title= attributes on the preset bar, which contract §4 DELETES
// rather than localizes, moving their existing text into data-i18n-aria. No
// hover-help prose is INVENTED here: authoring it is Stage M's job. I18N is
// therefore empty and TIP_BINDINGS is empty, which is this plugin's correct
// state rather than a gap. check-i18n assertion 2 reports it as "0 tip(s)
// bound", and the emptiness is only admissible BECAUSE no I18N entry carries a
// body — an emptied TIP_BINDINGS over a bodied table would be orphaned copy.
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
// ── THE FRAME IS 920 x 220. WIDE, BUT ONLY 220 TALL ─────────────────────────
//
// Every French string below was chosen against a MEASURED width, rendered in
// the real node with its own letter-spacing, font-weight and text-transform
// (none of which appears in getComputedStyle().font).
//
// ── THIS PAGE HAS THREE CLIFFS, NOT TWO, AND THE THIRD IS INVISIBLE ─────────
//
// O-Chorus (700x125) has a shrink-to-fit caption inside a fixed container, so
// its two cliffs are 50.00 (the rect widens) and 62.00 (the caption wraps).
// O-DigiDelay (700x196) has a hard-pinned .knob-label, so both of its cliffs
// are 60.00 and the mechanism turns on whether the string has a break
// opportunity. THIS PAGE IS NEITHER SHAPE and it carries a mechanism the other
// two do not:
//
//   A. SPILL — #analog, 57.00 px. `.toggle` is `width: 75px; height: 22px`, a
//      hard pin in BOTH axes, so its rectangle is language-invariant and
//      assertion 7 is structurally blind. A caption past 57.00 is painted
//      outside the button onto bare paper. Caught by check-ui-labels
//      assertion 4, the text-wider-than-its-content-box half. ANALOGIQUE
//      (66.70) fails it by 9.70 px and moves NOTHING.
//
//   B. PUSH — #savePreset / #loadPreset. `.preset-bar` is a right-anchored
//      flex row and these two buttons shrink-to-fit, so a wider caption widens
//      the button and shoves the three controls to its LEFT. Caught by
//      assertion 7. Assertion 4 is blind to it: the caption still fits its own
//      grown box. Unpinned, SAUVER + CHARGER move four elements by dx=-34.37.
//      THE PIN BELOW IS WHAT CLOSES IT.
//
//   C. WRAP INSIDE A PINNED, ABSOLUTELY POSITIONED, AUTO-HEIGHT BOX —
//      .band-label, 67.00 px. THIS ONE IS INVISIBLE TO BOTH ASSERTIONS AND IT
//      IS THIS PAGE'S REAL RISK.
//
//      Each band caption is `position: absolute; width: 85px` set inline, with
//      no fixed height. A two-word French caption past 67.00 px does not spill
//      and does not push:
//        - it WRAPS, so no line exceeds the content box  → assertion 4's
//          horizontal half sees nothing;
//        - the box GROWS to hold its own second line, 21 -> 34 px, so the text
//          never exceeds its own content height → assertion 4's vertical twin
//          (fbdb6930) sees nothing;
//        - the caption IS the [data-i18n] element and it is absolutely
//          positioned, so it moves no sibling → assertion 7 sees nothing.
//      And the visible result is a caption 13 px taller reaching y=86 into the
//      knob ring that starts at y=75. PLATEAU BF renders exactly that, with
//      every assertion green.
//
//      `.band-label { white-space: nowrap }` in index.html converts C into A.
//      It is NOT a geometry pin and is not claimed as one — see the note there.
//
// ── THE FOUR BAND CAPTIONS ──────────────────────────────────────────────────
//
// MEASURED at 11 px Garamond, letter-spacing 0.12em, weight 600, against the
// 67.00 px content box of an 85 px band-label (85 - 2 border - 16 padding):
//
//     LF SHELF  63.03 -> LF PLAT.  57.44    9.56 spare
//     LMF       28.41 -> LMF       28.41    sameAsEn
//     HMF       29.63 -> HMF       29.63    sameAsEn
//     HF SHELF  64.25 -> HF PLAT.  58.66    8.34 spare
//
// TWO OF FOUR SHRINK, and the two that do not change at all. A clip-only check
// would have certified this page — and so, here, would a push check.
//
// NOTE HOW LITTLE ROOM THE ENGLISH HAS: LF SHELF has 3.97 px spare and
// HF SHELF 2.75 px, in ENGLISH, at v1.1.11. There was never room for a longer
// French caption on this control, in any language, and that is a property of
// the authored layout rather than of the translation.
// ============================================================================

export const LABELS = Object.freeze({

    // ── The four band captions ──────────────────────────────────────────────
    //
    // These four divs are BOTH the caption and the band on/off switch: each is
    // an AudioParameterBool (lf_on / lmf_on / hmf_on / hf_on) whose only face
    // is a CSS class, never a rewritten string. So the node holds one fixed
    // caption and never a number — D-01 arm 3 does not apply — and a Bool has
    // no option strings for a French caption to disagree with in a host
    // automation lane, so arm 1 does not apply either.
    //
    // LF / LMF / HMF / HF ARE KEPT VERBATIM, and that is a decision, not an
    // oversight. They are the band abbreviations silk-screened on the French
    // market's own consoles (SSL, API, Neve are sold in France with exactly
    // these four), so translating them would make the plugin LESS legible to
    // its French user, not more. Only SHELF — the filter TYPE — is a word, and
    // it becomes PLAT., the standard abbreviation of "filtre en plateau".
    //
    // LMF and HMF are keyed with sameAsEn rather than dropped into
    // I18N_EXEMPT, deliberately and for the reason O-DigiDelay keyed MOD: an
    // exemption would hide a translation JUDGEMENT from the native-speaker
    // worklist forever, whereas a sameAsEn key is one more `reviewed: false`
    // line somebody has to agree with.
    //
    // Rejected on measurement, not on taste. Every fuller French form is past
    // the 67.00 px wrap cliff and would land in mechanism C, silently:
    //     PLATEAU BF   81.77   two lines, box 21 -> 34 px
    //     PLATEAU HF   82.98   two lines
    //     BAS MEDIUM   85.63   two lines
    //     HAUT MEDIUM  97.14   two lines
    // MED.HAUT (70.77) stays on one line only because it has no space in it,
    // and then SPILLS 3.77 px past the box.
    'label.band.lf':  { en: { t: 'LF SHELF' }, fr: { t: 'LF PLAT.', reviewed: false } },
    'label.band.lmf': { en: { t: 'LMF' },      fr: { t: 'LMF', reviewed: false, sameAsEn: true } },
    'label.band.hmf': { en: { t: 'HMF' },      fr: { t: 'HMF', reviewed: false, sameAsEn: true } },
    'label.band.hf':  { en: { t: 'HF SHELF' }, fr: { t: 'HF PLAT.', reviewed: false } },

    // ── The analog-saturation switch ────────────────────────────────────────
    //
    // `analog` is an AudioParameterBool gating the WaveShaper stage
    // (PluginProcessor.cpp), and like the band labels its face is a CSS class
    // rather than a rewritten string — the caption is fixed and never holds a
    // number, so neither arm 1 nor arm 3 of D-01 touches it.
    //
    // ANALOGIQUE is the word a French user would rather read and it does not
    // fit: 66.70 px against a 57.00 px content box in a button pinned to
    // 75 x 22, so it SPILLS 4.85 px past each edge onto bare paper. Widening
    // the button to 85 px would hold it — there is 63 px of empty paper to its
    // right before the VU meter — but that is a layout change caused by
    // French, and this page did not need one. ANALOG. is the standard French
    // abbreviation OF that word (45.30, 11.70 px spare), which is the same
    // trade O-DigiDelay made for REINJ. and O-Chorus for PROF.
    //
    // SATURATION (62.70) also spills. CHALEUR (47.55) fits and names a
    // different claim — warmth rather than the analogue path — so it was not
    // taken. A reviewer who prefers ANALOGIQUE must widen the button with it.
    'label.analog': { en: { t: 'ANALOG' }, fr: { t: 'ANALOG.', reviewed: false } },

    // ── The VU meter caption ────────────────────────────────────────────────
    //
    // `text-transform: uppercase` on `.vu-meter-label`, so this renders LEVEL /
    // NIVEAU. The box is the meter's full 108 px content width and the text is
    // centred in it, so NIVEAU (41.33) has 66.67 px spare — the roomiest string
    // on the page by a wide margin.
    'label.level': { en: { t: 'Level' }, fr: { t: 'Niveau', reviewed: false } },

    // ── The two preset buttons ──────────────────────────────────────────────
    //
    // `#savePreset, #loadPreset` are PINNED to 62 px in index.html. Rendered
    // text against that pin's 48 px content box — 62 border-box less 2 px
    // border and 12 px padding:
    //
    //     SAVE  24.52 -> SAUVER   39.25    8.75 px spare
    //     LOAD  27.16 -> CHARGER  46.80    1.20 px spare   TIGHTEST ON THE PAGE
    //
    // 62 px is O-Chorus's and O-DigiDelay's number, kept so the suite's preset
    // bar is one shape across the batch. CHARGER's 1.20 px is the tightest
    // French margin shipped here and it is thinner than O-DigiDelay's 1.48 px
    // on the identical string, because this page's 9 px type carries 0.06em
    // letter-spacing. OUVRIR (37.75, 10.25 px spare) is the reviewer's lever if
    // 1.20 px is judged too thin on Windows metrics — it is what O-Detune
    // already ships for this control — and taking it would require moving
    // aria.loadPreset's French to match, so that label-in-name still holds.
    // ENREGISTRER (66.95) would need the pin raised to 82 px.
    'label.save': { en: { t: 'SAVE' }, fr: { t: 'SAUVER',  reviewed: false } },
    'label.load': { en: { t: 'LOAD' }, fr: { t: 'CHARGER', reviewed: false } },

    // ── The preset dropdown's empty line, written through setLabel() ────────
    //
    // CONTRACT §6 — PLURALS ARE AVOIDED, NOT ENGINEERED. This is the one string
    // on the page that could have carried a count, and it is authored so that
    // it never does: "Aucun préréglage" is categorical, correct at exactly
    // zero, and needs no inflection — French treats zero as singular and
    // English does not, and a plural engine for one string on one plugin is not
    // a trade worth making. check-i18n assertion 13 rejects a ternary inside a
    // setLabel argument so a count cannot creep back in later.
    //
    // The English is byte-identical to what v1.1.11 wrote at index.html:1017.
    // No prose was invented; it was moved into the table.
    'label.noPresets': { en: { t: 'No presets' }, fr: { t: 'Aucun préréglage', reviewed: false } },

    // ── The settings popover (v1.2.0) ───────────────────────────────────────
    'label.language': { en: { t: 'Language' }, fr: { t: 'Langue', reviewed: false } },

    // ── Accessible names ────────────────────────────────────────────────────
    //
    // Resolved through the same sweep via data-i18n-aria, so a screen reader
    // hears the language the page is showing.
    //
    // THE FIVE PRESET-BAR NAMES ARE THE DELETED title= TEXT, MOVED, NOT
    // AUTHORED. v1.1.11 carried title="Previous preset", "Click to browse
    // presets", "Next preset", "Save preset to file" and "Load preset from
    // file"; contract §4 deletes the native attribute — on an element that also
    // has a data-tip it renders a second, untranslated OS tooltip, and leaving
    // it on an element that has none is still an untranslated string — and
    // moves its existing English into the accessible name. Every English string
    // below is byte-identical to what v1.1.11 shipped.
    //
    // LABEL IN NAME. #savePreset and #loadPreset carry BOTH a visible caption
    // and an aria-label, and an aria-label REPLACES the accessible name rather
    // than extending it. Each of those two names therefore CONTAINS its own
    // visible caption — "Save" in "Save preset to file", "SAUVER" in "Sauver un
    // préréglage dans un fichier" — so a voice-control user saying the caption
    // still hits the button (WCAG 2.5.3, which matches case-insensitively).
    //
    // That is why aria.loadPreset's French says "Charger" and not "Ouvrir":
    // the visible French caption is CHARGER. O-DigiDelay v1.3.0 ships
    // label.load CHARGER against aria.loadPreset "Ouvrir un préréglage depuis
    // un fichier", which breaks the rule in French only — reported to the
    // orchestrator, not edited from here.
    //
    // #presetName is the one place the rule cannot be honoured, and the
    // divergence is deliberate rather than overlooked: its visible text is the
    // PRESET NAME, which changes at runtime and is exempt under D-02, so no
    // fixed accessible name can contain it. The same trade was made on
    // O-Detune, O-DigiDelay, O-FreqPulse and O-Lyrica for the identical
    // control.
    'aria.prevPreset': { en: { t: 'Previous preset' },
                         fr: { t: 'Préréglage précédent', reviewed: false } },
    'aria.nextPreset': { en: { t: 'Next preset' },
                         fr: { t: 'Préréglage suivant',   reviewed: false } },
    'aria.presetList': { en: { t: 'Click to browse presets' },
                         fr: { t: 'Cliquez pour parcourir les préréglages', reviewed: false } },
    'aria.savePreset': { en: { t: 'Save preset to file' },
                         fr: { t: 'Sauver un préréglage dans un fichier', reviewed: false } },
    'aria.loadPreset': { en: { t: 'Load preset from file' },
                         fr: { t: 'Charger un préréglage depuis un fichier', reviewed: false } },

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
// is the one state in which the gate cannot tell a deliberate skip from a
// forgotten label (check-i18n assertion 14).
//
// NONE of the entries below is in that state — no key in LABELS above resolves
// to any of these strings — so all of them are correctly unscoped and assertion
// 14 passes without one. Checked rather than assumed: the closest call is MED,
// which is a Q option here and appears in no French caption; the band captions
// use PLAT., not MED.
// ============================================================================

export const I18N_EXEMPT = [
    // ── D-01 arm 1: byte-identical AudioParameterChoice option strings ──────
    //
    // WIDE / MED / TIGHT are the three options of BOTH lmf_q and hmf_q,
    // declared verbatim as juce::StringArray { "WIDE", "MED", "TIGHT" } at
    // PluginProcessor.cpp:56 and :69. Six nodes, three strings. The page and
    // the host automation lane must agree, so these do not translate: a French
    // user reading "MOYEN" on the face while the DAW's automation lane offers
    // "MED" is the divergence arm 1 exists to prevent.
    //
    // Unscoped is correct AND is the strictly stronger choice here: both
    // occurrences of each string are the same parameter case, and there is no
    // fourth node anywhere on the page carrying any of the three words, so
    // there is nothing for the exemption to over-silence.
    //
    // Their consequence for this stage is that `.three-way-option`'s
    // `white-space: nowrap` — one of the three the plan flags as this plugin's
    // clip risk — is NEVER REACHED BY FRENCH. See the report.
    ['WIDE',  'an lmf_q and hmf_q AudioParameterChoice option string VERBATIM '
            + '(PluginProcessor.cpp:56, :69) — D-01 arm 1: the page and the host automation '
            + 'lane must agree'],
    ['MED',   'an lmf_q and hmf_q AudioParameterChoice option string VERBATIM '
            + '(PluginProcessor.cpp:56, :69) — D-01 arm 1'],
    ['TIGHT', 'an lmf_q and hmf_q AudioParameterChoice option string VERBATIM '
            + '(PluginProcessor.cpp:56, :69) — D-01 arm 1'],

    // ── The product display name ────────────────────────────────────────────
    ['OUARICON ANALOG EQUALIZER',
     'the product display name in .title — a product name is never translated, and this is '
     + 'the brand-plus-product form of the plugin\'s registered PRODUCT_NAME "O-AnalogEQ" in '
     + 'CMakeLists.txt'],

    // ── D-02: the preset name IS the filename ───────────────────────────────
    ['Default',
     'the PRESET NAME shown in #presetName, not a caption — D-02. The name is the JSON '
     + 'filename on disk (OuariconPresetManager sanitizes it into getUserPresetsDirectory()), '
     + 'and it is written into this node at runtime by the VENDORED '
     + 'modules/preset-manager.js, which is shared across plugins: localizing it here would '
     + 'rename presets in one language and orphan the files'],

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
