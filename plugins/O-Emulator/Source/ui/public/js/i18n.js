/*
   This file is part of O-Emulator, an Ouaricon Audio plugin.
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
// i18n.js — O-Emulator page labels, English + French (v1.1.0)
//
// An ES module that EXPORTS ONLY. It must never self-execute: a bare top-level
// statement here throws out of module evaluation and takes every later
// initializer on the page with it (pattern_module_toplevel_init_tdz). This
// plugin's controller is a single inline <script type="module"> in index.html,
// so that failure mode would take the WHOLE UI — knobs, console selector,
// preset band — not a panel of it. scripts/check-i18n.js assertion 7 enforces
// the export-only shape.
//
// FILENAME: no hyphen. juce_add_binary_data STRIPS hyphens rather than
// converting them to underscores, so a second file named i18n-fr.js would have
// to be reached as the symbol i18nfr_js (critical_binary_data_strips_hyphens).
// One combined file for both languages sidesteps the question entirely.
//
// ── THIS PLUGIN HAS NO HOVER-HELP, AND THIS COMMIT DOES NOT GIVE IT ANY ─────
//
// v1.0.1 carried no data-tip, no data-tooltip and no native title= anywhere on
// the page. So there is no tooltip copy to MOVE here and none is INVENTED:
// authoring hover-help prose is Stage M's job. I18N is therefore empty and
// TIP_BINDINGS is empty, which is this plugin's correct state rather than a
// gap. check-i18n assertion 2 reports it as "0 tip(s) bound" instead of passing
// silently, and the emptiness is only admissible BECAUSE no I18N entry carries
// a body — an emptied TIP_BINDINGS over a bodied table would be orphaned copy
// and would fail.
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
// ── THE PAGE'S OWN GEOMETRY IS THE CONSTRAINT HERE, AND IT IS TIGHT ─────────
//
// Measured at the shipping 620 x 430 frame, rendered, not guessed. Two boxes
// on this page have NO slack at all, and both of them decide what the French
// could be rather than the other way round:
//
//   .hdr is 570 px of content holding three flex children whose max-content
//   widths total 732.28 px. It is 162 px OVER-FULL in ENGLISH. .preset-band
//   cannot shrink (every child is nowrap or a fixed width, so its min-content
//   IS its max-content), so the whole 162 px lands on .wordmark — which is
//   already at its min-content and renders "❦O-" / "EMULATOR" on TWO LINES —
//   and on .hdr-right, which takes whatever is left and wraps .plate to three
//   lines. Both overflow the 48 px header upward, in English, at v1.0.1.
//   That is a pre-existing layout defect, reported and NOT fixed here; what
//   matters for this file is the consequence: .hdr-right's width is exactly
//   `570 - 159.66 - bandWidth`, so ANY change to the preset band's width moves
//   .brand and .hdr-right, and both are non-label elements the geometry diff
//   measures. The three button captions are therefore PINNED (see index.html),
//   to a total of 128 px against English's natural 128.14 px.
//
//   .ctl is shrink-to-fit around a 60 px knob in a `space-evenly` row, so the
//   column's width is `max(60, captionWidth)`. A caption wider than 60 px
//   widens its column and slides all four. Every French caption below is
//   under 60 px, so no pin was needed and none was added.
//
// MEASURED, rendered text width, English -> French:
//
//     .ctl-label   Crush   42.69 -> Broyage 59.17   in a 60.00 column
//                  Age     27.39 -> Âge     27.39   IDENTICAL
//                  Reverb  50.17 -> Réverb  50.17   IDENTICAL
//                  Mix     27.06 -> Dosage  51.39   in a 60.00 column
//     .preset-btn  Save    22.25 -> Enreg.  32.47   in a 36.00 content box
//                  Load    24.48 -> Ouvrir  33.81   in a 37.00 content box
//                  Delete  33.41 -> Suppr.  30.38   in a 49.00 content box, and
//                  Confirm? 44.75 -> Sûr ?  26.23   in the same box, ARMED
//     .plate       widest line 91.91 -> 94.45, THREE lines in both languages,
//                  so .hdr-right stays 58.23 px tall and .brand does not move
//
// Broyage's 0.83 px of clearance is the tightest margin in this plugin and is
// named here so it is never a surprise: it is a margin against the 60 px KNOB,
// not against a clip, so overrunning it slides the columns by under a pixel
// rather than truncating anything. Windows/WebView2 font metrics remain the
// repo's standing hardware-blocked deferral.
//
// "Sûr ?" is 26.23 and NOT the 17.69 a first pass recorded. 17.69 was the
// widest LINE of the string after it WRAPPED in the unpinned 33 px box — a
// space before a French "?" is a break opportunity, so measuring a caption in
// the wrong box reports a confidently wrong number that is too SMALL. In the
// pinned 49 px box it is one line in both languages, verified rather than
// assumed.
//
// Two of the eight visible-text French strings render at EXACTLY the English
// width (Âge, Réverb), which is the half a clip check is blind to in the other
// direction — nothing to pin, nothing to fix, and worth stating rather than
// leaving as an unexplained zero in the diff.
// ============================================================================

export const LABELS = Object.freeze({

    // ── The four macro knob captions ────────────────────────────────────────
    // Captions, NOT readouts: the value lives in the .ro span below each knob
    // and keeps its "50 %" form untouched (D-03). None of these four is an
    // AudioParameterChoice option — they are the DISPLAY NAMES of four
    // AudioParameterFloats (PluginProcessor.cpp:71-74) — so arm 1 of D-01 does
    // not apply and there is no automation-lane string for a French caption to
    // disagree with.
    'label.crush':  { en: { t: 'Crush' },  fr: { t: 'Broyage', reviewed: false } },
    'label.age':    { en: { t: 'Age' },    fr: { t: 'Âge',     reviewed: false } },
    'label.reverb': { en: { t: 'Reverb' }, fr: { t: 'Réverb',  reviewed: false } },
    'label.mix':    { en: { t: 'Mix' },    fr: { t: 'Dosage',  reviewed: false } },

    // ── The preset band ─────────────────────────────────────────────────────
    // The repo-standard trio, matching O-Bitrot v1.15.0, O-ReverseDelay and
    // O-MultiBandCompressor verbatim: the same three buttons in the same band
    // should not be spelled three different ways across the suite. Abbreviated
    // rather than "Enregistrer" / "Charger" / "Supprimer" because this header
    // is 162 px over-full in English before French is asked for anything.
    'label.save':   { en: { t: 'Save' },   fr: { t: 'Enreg.', reviewed: false } },
    'label.load':   { en: { t: 'Load' },   fr: { t: 'Ouvrir', reviewed: false } },
    'label.delete': { en: { t: 'Delete' }, fr: { t: 'Suppr.', reviewed: false } },

    // The ARMED face of the delete button — the only string on this page
    // written from script. It goes through setLabel(), so the button becomes a
    // [data-i18n] element and the language sweep owns it from that moment on.
    // Through v1.0.1 it was a data-confirm ATTRIBUTE, which was the right
    // answer while the page was English-only and is the wrong one the moment
    // it has two languages: an attribute holds ONE string, so switching to
    // French mid-arm would have restored the English "Confirm?".
    //
    // "Sûr ?" and not "Confirmer ?": the latter renders 50.05 px in a 48.00 px
    // content box. Widening the button is not available — the band's total
    // width is what keeps .brand and .hdr-right still (see the header note
    // above) — and "Sûr ?" carries the same terse register as "Confirm?".
    'ui.confirm':   { en: { t: 'Confirm?' }, fr: { t: 'Sûr ?', reviewed: false } },

    // ── The imprint line ────────────────────────────────────────────────────
    // The naturalist-plate conceit the whole page is built on. Its box is
    // .hdr-right, whose WIDTH is leftover header space and therefore
    // language-invariant; only its LINE COUNT could move anything, so the
    // French was chosen to wrap to three lines exactly as the English does.
    // "Planche" is the French term of art for a plate in an illustrated
    // natural-history volume, which is what this line is imitating. The Roman
    // numeral is a numeral (D-03) and is carried across unchanged.
    'label.plate': {
        en: { t: 'A Survey of Extinct Consoles · Plate CDLXXXVII' },
        fr: { t: 'Relevé des consoles disparues · Planche CDLXXXVII', reviewed: false },
    },

    // ── The settings popover (v1.1.0) ───────────────────────────────────────
    'label.language': { en: { t: 'Language' }, fr: { t: 'Langue', reviewed: false } },

    // ── Accessible names ────────────────────────────────────────────────────
    // Resolved through the same sweep via data-i18n-aria, so a screen reader
    // hears the same language the page is showing.
    'aria.presetPrev': { en: { t: 'Previous preset' },   fr: { t: 'Préréglage précédent', reviewed: false } },
    'aria.presetNext': { en: { t: 'Next preset' },       fr: { t: 'Préréglage suivant',   reviewed: false } },
    'aria.settings':   { en: { t: 'Settings' },          fr: { t: 'Réglages',             reviewed: false } },
    'aria.langSelect': { en: { t: 'Interface language' }, fr: { t: 'Langue de l’interface', reviewed: false } },

    // "Console" is spelled identically in French — it is the same Latin root
    // and the same word for the same object. sameAsEn declares that on
    // purpose, because check-i18n assertion 4 otherwise reads an untranslated
    // entry and a deliberately identical one as the same thing.
    //
    // It localizes at all, rather than being exempt under D-01 arm 1, because
    // "Console" is the AudioParameterChoice's DISPLAY NAME
    // (PluginProcessor.cpp:55) and not one of its option strings. Nothing in a
    // host automation lane is spelled "Console" as a VALUE.
    'aria.console': { en: { t: 'Console' }, fr: { t: 'Console', sameAsEn: true, reviewed: false } },
});

// ============================================================================
// I18N_EXEMPT — reasoned exclusions, never silence
//
// Every visible string the coverage scan finds must be a [data-i18n] element, a
// setLabel() call, or an entry HERE WITH A REASON. A bare skip list would let a
// missed label hide as a deliberate one.
// ============================================================================

export const I18N_EXEMPT = [

    // ── Names ───────────────────────────────────────────────────────────────
    ['O-EMULATOR',
     'the product wordmark — a product name is never translated. It is also the '
     + 'only text node in an element that carries a ❦ span sibling, so keying it '
     + 'would make applyLabel delete the fleuron'],
    ['Ouaricon Audio',
     'the company name in .hdr-right .brand — a brand is never translated'],

    // ── D-01 arm 1: the captions that ARE the option strings ────────────────
    //
    // The console selector's five segments carry the `console`
    // AudioParameterChoice option strings, StringArray {"SNES", "PS1", "NES",
    // "Game Boy", "Genesis"} (PluginProcessor.cpp:53-57). Four of the five are
    // byte-identical to their option and are exempt on arm 1 outright: a DAW
    // showing CONSOLE = "Genesis" beside a page reading "Mega Drive" — which is
    // what that console was actually called in France — is a bug report, not a
    // localization.
    //
    // The fifth, "GB", is NOT byte-identical to its option "Game Boy", so arm 1
    // does not reach it on its own. It is exempted with the extra reason
    // recorded on its line: it is the abbreviated form of the same option, and
    // keying it alone would leave one of five segments switching language while
    // its four siblings are pinned by arm 1. The caption/option divergence
    // itself predates this commit and is reported rather than changed —
    // widening the segment to "Game Boy" is a visible layout change to a
    // shipped control.
    ['SNES',
     'a `console` AudioParameterChoice option string VERBATIM '
     + '(PluginProcessor.cpp:56) — D-01 arm 1'],
    ['PS1',
     'a `console` option string VERBATIM (PluginProcessor.cpp:56) — D-01 arm 1'],
    ['NES',
     'a `console` option string VERBATIM (PluginProcessor.cpp:56) — D-01 arm 1'],
    ['Genesis',
     'a `console` option string VERBATIM (PluginProcessor.cpp:56) — D-01 arm 1. '
     + 'The console was sold in France as the Mega Drive, and the page still says '
     + 'Genesis, because the host automation lane says Genesis'],
    ['GB',
     'the abbreviated form of the `console` option "Game Boy" (PluginProcessor.cpp:56). '
     + 'NOT byte-identical, so arm 1 does not reach it alone — exempted because '
     + 'keying one segment of five would leave it switching language while its four '
     + 'arm-1 siblings stay pinned, and because "Game Boy" is a hardware product '
     + 'name in French too. The caption/option divergence is pre-existing and is '
     + 'reported, not changed'],

    // ── D-01 arms 2 and 3: the per-console spec readout ─────────────────────
    //
    // #consoleInfo is written ONLY from the console listener, as
    // `c.name + " — " + c.spec` over a static table locked to ARCHITECTURE.md.
    //
    // The name half is arm 1 again — all five are the option strings verbatim,
    // including "Game Boy" this time.
    //
    // The spec half is exempt on TWO independent arms. Arm 2: every one of the
    // five carries a number and a unit (32 kHz, 22.05 kHz, 33.144 kHz,
    // 16.384 kHz, 26.32 kHz), which D-03 exempts. Arm 3: #consoleInfo is a
    // READOUT node — it is never a [data-i18n] element regardless of what is
    // behind it. O-Gain's LOW/MED/HIGH overrules arm 3 precisely because that
    // node never holds a number; this one always does, so the overrule is not
    // available and arm 3 stands.
    //
    // What that costs a French reader is "Gaussian" and "wave", and it is
    // stated here rather than left as an unexplained gap: translating them
    // means splitting a centred one-line readout into four keyed spans under
    // contract §5, which is a markup change to a readout in a stage whose scope
    // is labels.
    ['BRR 4-bit · 32 kHz · Gaussian',
     'the #consoleInfo spec readout — D-01 arm 2 (a number and a unit) and arm 3 '
     + '(a readout node is never a [data-i18n] element). Codec/rate/interpolation '
     + 'identifiers locked to ARCHITECTURE.md'],
    ['SPU-ADPCM · 22.05 kHz · Gaussian',
     'the #consoleInfo spec readout — D-01 arms 2 and 3, as above'],
    ['DPCM · 33.144 kHz · ZOH',
     'the #consoleInfo spec readout — D-01 arms 2 and 3, as above'],
    ['4-bit wave · 16.384 kHz · ZOH',
     'the #consoleInfo spec readout — D-01 arms 2 and 3, as above'],
    ['8-bit DAC · 26.32 kHz · ZOH',
     'the #consoleInfo spec readout — D-01 arms 2 and 3, as above'],
    ['Game Boy',
     'the #consoleInfo name half — a `console` option string VERBATIM '
     + '(PluginProcessor.cpp:56), D-01 arm 1'],

    // ── The preset name plate ───────────────────────────────────────────────
    ['Default',
     'the placeholder in #preset-name, which DISPLAYS a preset name — exempt under '
     + 'D-02, because the name IS the JSON filename '
     + '(modules/persistence/preset-manager, OuariconPresetManager.h). The element '
     + 'is written by the shared module\'s _updateDisplay(), never by this page'],

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
