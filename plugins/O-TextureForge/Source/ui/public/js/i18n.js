/*
   This file is part of O-TextureForge, an Ouaricon Audio plugin.
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
// i18n.js — O-TextureForge page labels, English + French (v1.1.0)
//
// An ES module that EXPORTS ONLY. It must never self-execute: a bare top-level
// statement here throws out of module evaluation and takes every later
// initializer on the page with it (pattern_module_toplevel_init_tdz).
// scripts/check-i18n.js assertion 7 enforces it.
//
// FILENAME: no hyphen. juce_add_binary_data STRIPS hyphens rather than
// converting them to underscores, so a second file named i18n-fr.js would have
// to be reached as the symbol i18nfr_js (critical_binary_data_strips_hyphens).
// One combined file for both languages sidesteps the question entirely. The
// companion module beside this one is js/i18n_init.js — an UNDERSCORE, for the
// same reason: it embeds as i18n_init_js, where i18n-init.js would embed as
// the unreadable i18ninit_js.
//
// ── THIS PLUGIN HAS NO HOVER-HELP, AND THIS COMMIT DOES NOT GIVE IT ANY ─────
//
// v1.0.2 carried no data-tip, no data-tooltip and — unusually for this stage —
// not even a stray native title=. So there is no tooltip copy to MOVE and none
// is INVENTED: authoring hover-help prose is Stage M's job. I18N is therefore
// empty and TIP_BINDINGS is empty, which is this plugin's correct state rather
// than a gap. check-i18n assertion 2 reports it as "0 tip(s) bound" instead of
// passing silently, and the emptiness is only admissible BECAUSE no I18N entry
// carries a body — an emptied TIP_BINDINGS over a bodied table would be
// orphaned copy and would fail.
//
// COPY IS textContent ON EVERY PATH — never innerHTML. check-i18n assertion 9
// rejects any innerHTML reference here and any string literal containing an
// angle bracket, so machine-drafted French cannot open a markup path. Three of
// the JS strings converted in this version USED to be written with innerHTML,
// into an element that also held a fleuron glyph; the markup is now authored
// once in index.html and only the text node is keyed. See LABELS below.
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
// ── WHERE THE STRINGS CAME FROM ─────────────────────────────────────────────
//
// 21 of them are HTML text nodes, which is the orchestrator's measured LABEL
// count exactly. EIGHT MORE ARE JAVASCRIPT PROSE THAT NO SCANNER IN THIS REPO
// CAN SEE. The extractor reports this plugin at 0 js-prose, and that number is
// an artifact of two rules meeting: i18n-extract.js skips *.bundle.js
// (i18n-extract.js:444, correctly — a minified vendor bundle is not authored
// page code), and the AUTHORED controller lives at Source/ui/src/app.js,
// OUTSIDE the served UI root, so no scan reaches it either. The eight were
// found by reading the file. They are: two placeholder writes, a WebGL-failure
// message, two toast messages, a composed large-file warning and its two
// dialog buttons, and the UMAP cancel button. Every one is user-visible.
//
// ── THE D-01 TEST, ALL THREE ARMS ───────────────────────────────────────────
//
// ARM 1 exempts the three MIDI-mode options: they are the MIDI_MODE
// AudioParameterChoice option strings byte for byte. They carry a SECOND,
// independent reason recorded in I18N_EXEMPT — src/app.js rebuilds that option
// list from the backend on every propertiesChanged, so a data-i18n on those
// <option>s would be destroyed the first time C++ pushed its properties. A key
// there would be dead markup that LOOKS localized.
//
// ARM 2 exempts the two readout nodes, 50ms and 0 dB.
//
// ARM 3 needed one judgement and made one SPLIT. .scatter-placeholder held a
// fleuron glyph element AND a bare text node as siblings; applyLabel writes
// textContent, which would have deleted the glyph. The text now lives in its
// own .placeholder-text span and only that span is keyed (contract section 5).
// Every .knob-value is already a separate sibling of its .knob-label, so no
// other node on this page carries a caption and a number at once.
//
// ── GEOMETRY — MEASURED at the shipping 900 x 600, not reasoned ─────────────
//
// The tight place on this page is .bottom-knobs, and the reason is
// justify-content: space-around over five shrink-wrapped .bottom-knob-group
// columns. Under space-around the free space is divided around items of
// whatever width they happen to have, so ANY change to any caption's width
// moves ALL FIVE GROUPS — and a group is not a label and not inside one, so
// every one of them is an assertion-7 element. Both directions are fatal
// there: a French caption that GROWS past the 38 px knob widens its group, and
// one that SHRINKS below the widest English caption narrows it. Four of this
// page's twelve French captions are shorter than their English source, so the
// shrink case is not hypothetical here.
//
// The pin is in the stylesheet with its reasoning and its negative control.
// The per-caption measurements are in the commit message.
// ============================================================================

export const LABELS = Object.freeze({

    // ── Header ──────────────────────────────────────────────────────────────
    // The h1 is NOT here: a product name is never translated. It is an
    // I18N_EXEMPT entry with that reason, so a genuinely missed label cannot
    // hide as a deliberate one.
    'label.tagline': {
        en: { t: 'Concatenative Texture Engine' },
        fr: { t: 'Moteur de texture concaténative', reviewed: false },
    },

    // ── The scatter placeholder ─────────────────────────────────────────────
    // Three states of ONE element, written by src/app.js. Before v1.1.0 all
    // three were innerHTML writes that re-authored the fleuron glyph each time;
    // the glyph is now a permanent sibling span in the markup and these key
    // only the text.
    'placeholder.dropToBegin': {
        en: { t: 'Drop an audio file to begin' },
        fr: { t: 'Déposez un fichier audio pour commencer', reviewed: false },
    },
    'placeholder.webglUnavailable': {
        en: { t: 'WebGL unavailable' },
        fr: { t: 'WebGL indisponible', reviewed: false },
    },
    // Composed. {path} is the saved corpus path, substituted literally — it is
    // a filesystem path and resolves through trLabel's `resolve` arm to itself
    // because no LABELS key is spelled like a path.
    //
    // NO BRANCH AND NO INFLECTION (contract section 6). The path sits on its
    // OWN LINE rather than inside the sentence, and both sentences around it
    // read correctly whether that line is a 200-character path, a short one or
    // empty — so there is no count, no ternary and no second wording to choose
    // between. src/app.js first wrote `data.path || 'placeholder.unknownPath'`
    // inside the setLabel argument and carried a second entry here for the
    // fallback word; assertion 13 was right to reject it, and the condition it
    // guarded cannot fire in any case, because C++ reaches onCorpusMissing only
    // inside `else if (savedPath.isNotEmpty())` (PluginProcessor.cpp:294). Both
    // the branch and the fallback entry are gone.
    //
    // The line break is \n with white-space: pre-line on the span — never a
    // <br>, which assertion 9 would reject and which would need innerHTML to
    // render. Its own line is also what makes a long path legible inside the
    // 320 px box, with overflow-wrap: anywhere to break it.
    'placeholder.fileNotFound': {
        en: { t: 'File not found:\n{path}\nDrop a new file to continue.' },
        fr: { t: 'Fichier introuvable :\n{path}\nDéposez un nouveau fichier pour continuer.', reviewed: false },
    },

    // ── Macro panel section captions ────────────────────────────────────────
    'section.timbralMacros': {
        en: { t: 'Timbral Macros' },
        fr: { t: 'Macros timbrales', reviewed: false },
    },
    'section.scatterPosition': {
        en: { t: 'Scatter Position' },
        fr: { t: 'Position de dispersion', reviewed: false },
    },

    // ── The six macro knobs ─────────────────────────────────────────────────
    // Every one is an AudioParameterFloat display name, not a choice option, so
    // D-01 arm 1 does not apply and they localize. Their .knob-value siblings
    // are the readouts and are untouched.
    'knob.energy':     { en: { t: 'Energy' },     fr: { t: 'Énergie',   reviewed: false } },
    'knob.brightness': { en: { t: 'Brightness' }, fr: { t: 'Brillance', reviewed: false } },
    // Identical in French. sameAsEn is REQUIRED here: check-i18n assertion 4
    // rejects a French entry that merely repeats the English unless the repeat
    // is declared deliberate, so an untranslated string cannot hide as a
    // coincidence.
    'knob.texture':    { en: { t: 'Texture' },    fr: { t: 'Texture',   reviewed: false, sameAsEn: true } },
    // MEASURED, not guessed. .knob-row .knob-label is a hard `width: 72px`
    // right-aligned box at 9 px with 1 px of letter-spacing. "Dispersion X" is
    // 65.14 px of text and still WRAPS TO TWO LINES in that box — the space
    // before the axis letter is a break opportunity and "Dispersion" alone
    // fills 65 of the 72 — taking the label's rect from 10 px tall to 20 and
    // its top from 296 to 291. The row is `height: 50px` with align-items
    // center, so nothing else moved, but a two-line caption beside a one-line
    // one is a defect the clip check cannot see.
    //
    // "Disp. X" keeps the section caption's root ("Position de dispersion")
    // and is the same abbreviation shape the two axes already have in English.
    'knob.scatterX':   { en: { t: 'Scatter X' },  fr: { t: 'Disp. X', reviewed: false } },
    'knob.scatterY':   { en: { t: 'Scatter Y' },  fr: { t: 'Disp. Y', reviewed: false } },
    'knob.variation':  { en: { t: 'Variation' },  fr: { t: 'Variation', reviewed: false, sameAsEn: true } },

    // ── The five bottom-strip knobs ─────────────────────────────────────────
    // The tight row. See the GEOMETRY note above and the pin in the stylesheet.
    'knob.position':     { en: { t: 'Position' },   fr: { t: 'Position', reviewed: false, sameAsEn: true } },
    // "Grain Size" is GRAIN_SIZE's display name. "Taille grain" drops the
    // preposition rather than reading "Taille de grain" because this caption
    // sits in a 8 px uppercase row where the extra word is 18 px it does not
    // have; the shortened form is the same shape as the English compound.
    'knob.grainSize':    { en: { t: 'Grain Size' }, fr: { t: 'Taille grain', reviewed: false } },
    // The page caption is "Density"; the parameter is "Grain Density". The
    // caption is what is localized, because the caption is what is rendered.
    'knob.grainDensity': { en: { t: 'Density' },    fr: { t: 'Densité',  reviewed: false } },
    'knob.crossfade':    { en: { t: 'Crossfade' },  fr: { t: 'Fondu',    reviewed: false } },
    // The page caption is "Gain"; the parameter is "Output Gain". Spelled
    // identically in French, hence sameAsEn.
    'knob.gain':         { en: { t: 'Gain' },       fr: { t: 'Gain',     reviewed: false, sameAsEn: true } },

    // ── Bottom controls ─────────────────────────────────────────────────────
    // "MIDI Mode" is the MIDI_MODE parameter's DISPLAY NAME, not one of its
    // option strings, so arm 1 does not reach it — only the three options it
    // offers are exempt. MIDI stays uppercase and untranslated: it is a
    // protocol name.
    'label.midiMode': {
        en: { t: 'MIDI Mode' },
        fr: { t: 'Mode MIDI', reviewed: false },
    },
    // Used TWICE — on the drop zone in the markup, and by src/app.js when an
    // empty corpus arrives and the placeholder falls back to the same
    // invitation. One key rather than two copies of one sentence drifting
    // apart in two files.
    'label.dropZone': {
        en: { t: 'Drop audio file here' },
        fr: { t: 'Déposez un fichier audio ici', reviewed: false },
    },

    // ── Toasts and the large-file dialog, all written by src/app.js ─────────
    // "UMAP" and "PCA" are untranslated: both are algorithm names used in
    // French technical writing in their English acronym form, and UMAP is
    // already the visible caption of the progress row above.
    'toast.umapCancelled': {
        en: { t: 'UMAP cancelled — using PCA layout' },
        fr: { t: 'UMAP annulé — disposition PCA conservée', reviewed: false },
    },
    // The FALLBACK only. When C++ supplies a reason it is shown verbatim and is
    // NOT localized — see the note in I18N_EXEMPT.
    'toast.loadFailed': {
        en: { t: 'Failed to load file' },
        fr: { t: 'Échec du chargement du fichier', reviewed: false },
    },
    // Composed. {size} is a number the caller has already formatted to one
    // decimal; it is a readout and is not translated (D-03). "MB" becomes "Mo",
    // which is the French unit symbol for megabyte and is a genuine
    // localization rather than a translation of prose.
    //
    // NO INFLECTION: the sentence reads correctly at any size, so there is no
    // plural to engineer (contract section 6).
    'dialog.largeFile': {
        en: { t: 'Large file: {size} MB. This may use significant memory.' },
        fr: { t: 'Fichier volumineux : {size} Mo. Cela peut consommer beaucoup de mémoire.', reviewed: false },
    },
    'dialog.loadAnyway': {
        en: { t: 'Load Anyway' },
        fr: { t: 'Charger quand même', reviewed: false },
    },
    // Used twice: the large-file dialog's dismiss button and the UMAP progress
    // row's cancel button. Same word, same meaning, one key.
    'action.cancel': {
        en: { t: 'Cancel' },
        fr: { t: 'Annuler', reviewed: false },
    },

    // ── The settings popover (v1.1.0) ───────────────────────────────────────
    'label.language': { en: { t: 'Language' }, fr: { t: 'Langue', reviewed: false } },

    // ── Accessible names ────────────────────────────────────────────────────
    // Resolved through the same sweep via data-i18n-aria, so a screen reader
    // hears the language the page is showing.
    'aria.settings':   { en: { t: 'Settings' },           fr: { t: 'Réglages',              reviewed: false } },
    'aria.langSelect': { en: { t: 'Interface language' }, fr: { t: 'Langue de l’interface', reviewed: false } },
});

// ============================================================================
// I18N_EXEMPT — reasoned exclusions, never silence
//
// Every visible string the coverage scan finds must be a [data-i18n] element, a
// setLabel() call, or an entry HERE WITH A REASON. A bare skip list would let a
// missed label hide as a deliberate one.
// ============================================================================

export const I18N_EXEMPT = [
    ['O-TextureForge',
     'the product display name in the h1 and in the document title — a product name is never translated. It is the plugin\'s registered PRODUCT_NAME in CMakeLists.txt:29'],

    // ── D-01 arm 1: the captions that ARE the option strings ────────────────
    //
    // TWO independent reasons, and the second is the stronger one. Byte-identity
    // with the MIDI_MODE options means a French caption would make the page and
    // the host automation lane disagree about the same setting. But even
    // setting that aside, these three <option> elements are DESTROYED AND
    // REBUILT from comboState.properties.choices by setupMidiMode() in
    // src/app.js the first time C++ pushes its properties — a data-i18n on them
    // would be dead markup that looks localized in the source and renders
    // English at runtime, which is worse than an honest exemption.
    ['Pitch-Mapped',
     'a MIDI_MODE AudioParameterChoice option string VERBATIM (PluginProcessor.cpp:137, StringArray {"Pitch-Mapped","Trigger + Modulate","Generative Drone"}) — D-01 arm 1; and the option element is rebuilt from the backend by setupMidiMode(), so a key on it could not survive'],
    ['Trigger + Modulate',
     'a MIDI_MODE option string VERBATIM (PluginProcessor.cpp:137) — D-01 arm 1; rebuilt from the backend by setupMidiMode()'],
    ['Generative Drone',
     'a MIDI_MODE option string VERBATIM (PluginProcessor.cpp:137) — D-01 arm 1; rebuilt from the backend by setupMidiMode()'],

    // ── D-01 arm 2 / arm 3: the readouts ────────────────────────────────────
    // Listed rather than left silent even though the extractor already classes
    // them READOUT and assertion 10 does not ask about them: an explicit entry
    // is what distinguishes a decision from an oversight when the next stage
    // reads this file.
    ['50ms',
     'a .knob-value READOUT node written by src/app.js from GRAIN_SIZE — a number and its unit (D-03, arm 2), and a readout node is never a [data-i18n] element (arm 3)'],
    ['0 dB',
     'a .knob-value READOUT node written by src/app.js from OUTPUT_GAIN — a number and its unit (D-03, arm 2), and a readout node is never a [data-i18n] element (arm 3)'],

    // ── Acronyms and proper nouns ───────────────────────────────────────────
    ['UMAP',
     'the algorithm name Uniform Manifold Approximation and Projection, used untranslated in French technical writing exactly as in English. It is also the .umap-progress-label caption, so translating it would rename a published algorithm on screen'],
    ['MIDI',
     'a protocol name — never translated, and it appears only inside label.midiMode, whose French keeps it verbatim'],
    ['PCA',
     'the algorithm name Principal Component Analysis, kept in its English acronym form for the same reason as UMAP and to match the UMAP caption beside it. It appears only inside toast.umapCancelled'],

    // ── C++-authored prose that reaches the page over the event bus ─────────
    ['Unsupported format: ',
     'authored in C++ at Source/dsp/CorpusLoader.cpp:102 and delivered to the page as an opaque `reason` string in the loadFailed event. The page cannot key a string it receives at runtime; localizing it means C++ emitting a KEY instead of a sentence, which changes the event payload contract. REPORTED, not fixed, in the v1.1.0 commit message. The page-side fallback for a missing reason IS localized (toast.loadFailed)'],

    // ── Endonyms ────────────────────────────────────────────────────────────
    ['English',  'endonym — a language name is never translated'],
    ['Français', 'endonym — a language name is never translated'],

    // ── Pictographs ─────────────────────────────────────────────────────────
    ['⚙', 'the GEAR SYMBOL U+2699 is the settings button\'s only content — a pictograph, not prose. Its meaning is carried by data-i18n-aria="aria.settings", which IS localized'],
    ['❧', 'the ROTATED FLORAL HEART BULLET U+2767, this plugin\'s fleuron. A decorative glyph used as the scatter placeholder\'s ornament and as the panel divider — not prose, and language-neutral'],
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
