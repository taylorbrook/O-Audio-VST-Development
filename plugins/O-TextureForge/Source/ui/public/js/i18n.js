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
// i18n.js — O-TextureForge page labels and hover-help, English + French (v1.2.0)
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
// ── v1.2.0: HOVER-HELP ARRIVES, AND IT NEEDED A RENDERER AS WELL AS COPY ────
//
// Through v1.1.0 I18N and TIP_BINDINGS were both empty, which was this
// plugin's correct state rather than a gap: v1.0.2 carried no data-tip, no
// data-tooltip and not even a stray native title=, so there was no tooltip copy
// to MOVE and none was invented.
//
// v1.2.0 authors it: 12 parameter tips — one per dumped parameter, and every
// one of the twelve has a control on this page — plus 2 chrome tips, 14 in all.
//
// AUTHORING THE COPY WAS NOT THE WHOLE JOB. Canon v2's applyI18n() writes
// data-tip-title and data-tip ATTRIBUTES onto the anchors named in
// TIP_BINDINGS and stops there. The code that reads those attributes and paints
// a surface is per-plugin, and on this page it did not exist: id="tooltip" 0,
// .tooltip { 0, closest("[data-tip]") 0. Binding 14 entries with no renderer
// ships 14 invisible strings past three green gates — check-i18n reads the
// table statically, check-ui-labels has no tooltip awareness at all, and
// boot-all-uis counts aria-label and title, never data-tip. The renderer lives
// in js/i18n_init.js beside the canon (NOT in src/app.js, which webpack
// bundles), its surface is in index.html and its styling is in the stylesheet.
// tests/ui_tip_render_check.js is the gate that can see it paint.
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
// I18N — hover-help copy (v1.2.0). {en:{t, b}, fr:{t, b, reviewed}}.
//
// `t` is the TITLE — the control's own caption as the page shows it, so the tip
// names the thing the pointer is on rather than the automation lane's spelling.
// Three of the twelve differ from the parameter's display name and the CAPTION
// wins each time: GRAIN_DENSITY is "Grain Density" in the host and "Density" on
// the page, OUTPUT_GAIN is "Output Gain" and "Gain", and the French Scatter X/Y
// captions were shortened to "Disp. X" / "Disp. Y" in v1.1.0 against a measured
// 72 px box (see LABELS below).
//
// `b` is the BODY: what the control does, when to reach for it, ending with the
// range and its unit. Three sentences at most — this is a tooltip, not a manual.
//
// ── WHERE EVERY RANGE CAME FROM, AND THE THREE THAT COULD NOT COME FROM THE
//    PAGE ───────────────────────────────────────────────────────────────────
//
// The rule for this stage is that where the dump's `label` column is empty the
// range is phrased from the PAGE'S OWN formatter, never invented. Eight of the
// twelve resolve that way: fmtPercent at Source/ui/src/app.js:659 is
// `(n) => Math.round(n * 100) + '%'`, so ENERGY, BRIGHTNESS, TEXTURE, POSITION,
// SCATTER_X, SCATTER_Y, VARIATION and CROSSFADE all read 0-100% on screen
// whatever their C++ range is. CROSSFADE's own dump label is "%" and agrees.
//
// THREE OF THEM HAVE NO PAGE FORMATTER TO READ, because their formatter returns
// the EMPTY STRING: src/app.js:705, :706 and :708 pass `(n) => ''` for
// grainSize, grainDensity and outputGain, so those three .knob-value nodes
// render BLANK the moment setupKnob's initial updateDisplay() runs — measured
// in the harness, not reasoned, and present since v1.0.0. The static markup
// fallbacks in index.html (50ms, 8, 0 dB) are erased by that first write and no
// user ever sees them. Their ranges here therefore come from the PARAMETER
// DUMP, which is authoritative and which carries real units for two of the
// three: GRAIN_SIZE 10..500 with label "ms", OUTPUT_GAIN -60..+12 with label
// "dB", GRAIN_DENSITY an AudioParameterInt 1..64 with no label (the word
// "grains" is the page's own noun for what it counts, not an invented unit).
//
// REPORTED, NOT FIXED. Repairing those three formatters means editing
// Source/ui/src/app.js and rebuilding app.bundle.js, which is a webpack rebuild
// inside a hover-help commit — out of this stage's scope and a 220 KB
// unreviewable diff. It also makes the two I18N_EXEMPT entries below ("50ms",
// "0 dB") describe something real for the first time; their v1.1.0 reasons say
// those nodes are "written by src/app.js", and they are not written at all.
//
// ── FRENCH CONVENTION ───────────────────────────────────────────────────────
//
// A tooltip body is PROSE and takes French convention: a space before %, U+2212
// for the minus sign in -60 dB, and a decimal COMMA — settled repo-wide on
// 2026-08-30. No entry below happens to carry a decimal, so the comma does not
// appear; the rule is recorded so the next editor does not reopen it. The
// READOUT nodes keep their point, because D-03 exempts the readout NODE and
// that has not moved: a body and its readout spell a number differently on
// purpose, one being prose and the other a machine-formatted value.
//
// ALL FRENCH IS MACHINE-DRAFTED, every entry `reviewed: false`.
// ============================================================================

export const I18N = Object.freeze({

    // ── The six macro knobs, right panel ────────────────────────────────────
    // Each biases the KD-tree query in descriptor space. The three timbral ones
    // name the descriptor they steer, because that is what makes the difference
    // between them legible: GrainMetadata.h:45-47 maps BRIGHTNESS_DIM to
    // spectral centroid, TEXTURE_DIM to spectral flatness and ENERGY_DIM to RMS.
    'tip.energy': {
        en: { t: 'Energy',
              b: 'Biases the grain search toward louder or quieter material, matching each grain’s RMS energy. Turn it down for a hushed bed, up for the most forceful moments in the corpus. 0 to 100%.' },
        fr: { t: 'Énergie',
              b: 'Oriente la recherche de grains vers un matériau plus fort ou plus doux, selon l’énergie RMS de chaque grain. Baissez-le pour un lit feutré, montez-le pour les moments les plus puissants du corpus. 0 à 100 %.',
              reviewed: false },
    },
    'tip.brightness': {
        en: { t: 'Brightness',
              b: 'Biases the search toward darker or brighter grains, matching each grain’s spectral centroid. Low settings favour muffled, body-heavy material; high settings favour air and hiss. 0 to 100%.' },
        fr: { t: 'Brillance',
              b: 'Oriente la recherche vers des grains plus sombres ou plus clairs, selon le centroïde spectral de chaque grain. Les valeurs basses privilégient un matériau étouffé et corpulent ; les valeurs hautes, l’air et le souffle. 0 à 100 %.',
              reviewed: false },
    },
    'tip.texture': {
        en: { t: 'Texture',
              b: 'Biases the search between tonal and noisy grains, matching each grain’s spectral flatness. Low settings pick pitched, steady material; high settings pick breath, grit and noise. 0 to 100%.' },
        fr: { t: 'Texture',
              b: 'Oriente la recherche entre des grains tonals et des grains bruités, selon la planéité spectrale de chaque grain. Les valeurs basses choisissent un matériau tenu et intonné ; les valeurs hautes, le souffle et le bruit. 0 à 100 %.',
              reviewed: false },
    },
    // The two scatter axes have their OWN entries rather than one shared one,
    // because they have their own knobs: the XY control here is the scatter map
    // itself, which is not a bound anchor (it is a canvas whose whole surface is
    // a click-to-select target, and a tip following the pointer across it would
    // sit on top of the thing being aimed at).
    'tip.scatterX': {
        en: { t: 'Scatter X',
              b: 'Moves the cursor horizontally across the scatter map and pulls grains from that region of the corpus. Drag on the map itself to move both axes at once. 0 to 100%.' },
        fr: { t: 'Disp. X',
              b: 'Déplace le curseur horizontalement sur la carte de dispersion et puise les grains dans cette zone du corpus. Faites glisser directement sur la carte pour déplacer les deux axes à la fois. 0 à 100 %.',
              reviewed: false },
    },
    'tip.scatterY': {
        en: { t: 'Scatter Y',
              b: 'Moves the cursor vertically across the scatter map and pulls grains from that region of the corpus. Drag on the map itself to move both axes at once. 0 to 100%.' },
        fr: { t: 'Disp. Y',
              b: 'Déplace le curseur verticalement sur la carte de dispersion et puise les grains dans cette zone du corpus. Faites glisser directement sur la carte pour déplacer les deux axes à la fois. 0 à 100 %.',
              reviewed: false },
    },
    'tip.variation': {
        en: { t: 'Variation',
              b: 'Randomises the search around the macro settings, so repeated grains are never quite the same. At 0 the same grain returns every time; raise it for a looser, more alive texture. 0 to 100%.' },
        fr: { t: 'Variation',
              b: 'Randomise la recherche autour des réglages de macros, afin que les grains répétés ne soient jamais tout à fait identiques. À 0, le même grain revient à chaque fois ; montez-le pour une texture plus lâche et plus vivante. 0 à 100 %.',
              reviewed: false },
    },

    // ── The five bottom-strip knobs ─────────────────────────────────────────
    'tip.position': {
        en: { t: 'Position',
              b: 'Biases the search toward grains taken from a given point in the source recording. At 0 the texture draws on the opening; at the top of the range it draws on the end. 0 to 100%.' },
        fr: { t: 'Position',
              b: 'Oriente la recherche vers les grains issus d’un point donné de l’enregistrement source. À 0, la texture puise dans le début ; en haut de la plage, dans la fin. 0 à 100 %.',
              reviewed: false },
    },
    // Range from the DUMP: this knob's readout is blank at runtime. See the
    // header. The dump's label column carries "ms" for GRAIN_SIZE.
    'tip.grainSize': {
        en: { t: 'Grain Size',
              b: 'Sets how long each grain plays. Short values granulate the source into a fine cloud; long values let recognisable fragments of the recording through. 10 to 500 ms.' },
        fr: { t: 'Taille grain',
              b: 'Définit la durée de lecture de chaque grain. Les valeurs courtes granulent la source en un nuage fin ; les valeurs longues laissent passer des fragments reconnaissables de l’enregistrement. 10 à 500 ms.',
              reviewed: false },
    },
    // Range from the DUMP (AudioParameterInt 1..64, no label). "grains" is the
    // page's own noun for what this counts, not an invented unit.
    'tip.grainDensity': {
        en: { t: 'Density',
              b: 'Sets how often new grains are fired in the drone mode, thinning the texture out or thickening it. Low values leave audible gaps; high values overlap into a continuous bed. 1 to 64 grains.' },
        fr: { t: 'Densité',
              b: 'Définit la fréquence de déclenchement des nouveaux grains en mode drone, ce qui éclaircit ou épaissit la texture. Les valeurs basses laissent des trous audibles ; les valeurs hautes se recouvrent en un lit continu. 1 à 64 grains.',
              reviewed: false },
    },
    'tip.crossfade': {
        en: { t: 'Crossfade',
              b: 'Shapes each grain’s envelope, from a narrow peak to a wide flat top that overlaps its neighbours. Raise it to smooth a grainy texture, lower it for rhythm and attack. 0 to 100%.' },
        fr: { t: 'Fondu',
              b: 'Façonne l’enveloppe de chaque grain, d’un pic étroit à un plateau large qui recouvre ses voisins. Montez-le pour lisser une texture granuleuse, baissez-le pour le rythme et l’attaque. 0 à 100 %.',
              reviewed: false },
    },
    // Range from the DUMP (label "dB"); this readout is blank at runtime too.
    // U+2212 MINUS SIGN, not a hyphen, in both languages.
    'tip.gain': {
        en: { t: 'Gain',
              b: 'Trims the plugin’s output level after the grain cloud has been summed. Dense settings stack many grains at once, so pull it down if the output clips. −60 to +12 dB.' },
        fr: { t: 'Gain',
              b: 'Ajuste le niveau de sortie du plugin après la somme du nuage de grains. Les réglages denses empilent de nombreux grains à la fois : baissez-le si la sortie écrête. −60 à +12 dB.',
              reviewed: false },
    },

    // ── The MIDI mode select ────────────────────────────────────────────────
    // The three option words stay ENGLISH inside the French body, deliberately
    // and against the general rule that prose naming an option is localized.
    // They are I18N_EXEMPT under D-01 arm 1 — byte-identical MIDI_MODE choice
    // strings — so they stay English ON THE PAGE in both languages. A French
    // body naming French option words would be pointing at captions the reader
    // cannot find in the dropdown two centimetres below the tip.
    'tip.midiMode': {
        en: { t: 'MIDI Mode',
              b: 'Chooses how the plugin answers MIDI. Pitch-Mapped transposes grains from the note played, Trigger + Modulate fires a grain per note and takes CC control, and Generative Drone runs on its own with no notes at all. Three settings.' },
        fr: { t: 'Mode MIDI',
              b: 'Choisit la façon dont le plugin répond au MIDI. Pitch-Mapped transpose les grains selon la note jouée, Trigger + Modulate déclenche un grain par note et accepte le contrôle CC, et Generative Drone tourne seul, sans aucune note. Trois réglages.',
              reviewed: false },
    },

    // ── Chrome ──────────────────────────────────────────────────────────────
    // The gear tip is what tells a user hover-help exists at all, so it must
    // describe ONLY what this popover actually contains: one row, the language
    // selector. This plugin has no hover-help on/off toggle, and a tip that
    // promised one would be a tip that lies.
    'tip.settings': {
        en: { t: 'Settings',
              b: 'Opens the settings panel. It holds one control, the interface language. Nothing in it changes the sound or any saved parameter.' },
        fr: { t: 'Réglages',
              b: 'Ouvre le panneau de réglages. Il contient un seul contrôle, la langue de l’interface. Rien ici ne modifie le son ni un paramètre enregistré.',
              reviewed: false },
    },
    // The endonyms are named verbatim in both bodies: they are what the two
    // options in this select actually say, and they are I18N_EXEMPT for that
    // reason (a language name is never translated).
    'tip.language': {
        en: { t: 'Language',
              b: 'Chooses the language of the captions and of this hover help. Value readouts stay in English. The choice is saved with the session, not with a preset. English or Français.' },
        fr: { t: 'Langue',
              b: 'Choisit la langue des libellés et de cette aide au survol. Les valeurs affichées restent en anglais. Le choix est enregistré avec la session, pas avec un préréglage. English ou Français.',
              reviewed: false },
    },
});

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
    // CORRECTED in v1.2.0. Both reasons said these were "written by src/app.js";
    // they are not written at all. setupKnob is passed `(n) => ''` for grainSize,
    // grainDensity and outputGain (src/app.js:705, :706, :708), so the first
    // updateDisplay() ERASES the authored markup and all three readouts render
    // blank for the rest of the session — measured in the harness, and true
    // since v1.0.0. The exemption still stands on the same D-03 grounds: they
    // are a number and its unit in a readout node either way.
    ['50ms',
     'the GRAIN_SIZE .knob-value READOUT node — a number and its unit (D-03, arm 2), and a readout node is never a [data-i18n] element (arm 3). Authored in index.html and erased at init by an empty formatter (src/app.js:705); reported, not fixed, in the v1.2.0 commit message'],
    ['0 dB',
     'the OUTPUT_GAIN .knob-value READOUT node — a number and its unit (D-03, arm 2), and a readout node is never a [data-i18n] element (arm 3). Authored in index.html and erased at init by an empty formatter (src/app.js:708); reported, not fixed, in the v1.2.0 commit message'],

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
// TIP_BINDINGS — [selector, key] or [selector, key, wrapper] (v1.2.0)
//
// applyI18n() runs document.querySelector(selector), walks closest(wrapper)
// when a third element is given, and writes data-tip-title + data-tip onto
// whatever it lands on.
//
// ── "BIND TO THE IDS THE UI ALREADY USES" IS FALSE HERE, ON BOTH HALVES ─────
//
// T17 says to bind the ids the page already has. Eleven of these fourteen
// anchors carry NO id: the knobs are .knob[data-param="..."] divs, and the only
// ids on the whole page are #scatter-canvas, #cursor-overlay, #midi-mode,
// #drop-zone, #gear-btn, #settings-popover and #lang-select. So the selector
// half fails eleven times.
//
// The TARGET half fails on the same eleven, independently and for a different
// reason: a .knob is a 44 px circle (28 px in the bottom strip), and its label
// and its readout are SIBLINGS of it, not children. A tip bound to the circle
// alone would vanish the moment the pointer crossed onto the caption the user
// is reading. The wrapper walk puts the tip on the whole cell — .knob-row in
// the macro panel, .bottom-knob-group in the bottom strip — which is the area a
// pointer actually occupies.
//
// ── THE THREE THAT BIND BARE, AND WHY ───────────────────────────────────────
//
// #gear-btn and #lang-select bind BARE even though both sit inside
// .settings-cluster. A wrapper walk to that ancestor would make hovering the
// language selector resolve to the GEAR's tip, because .settings-cluster
// contains the gear AND the popover the selector lives in — the O-Comp trap.
//
// #midi-mode binds bare for the mirror-image reason: its only useful ancestor
// is .bottom-controls, which also holds #drop-zone, so a wrapper walk would put
// the MIDI tip over the drop zone as well.
//
// Every binding must land on a DISTINCT element; the render gate asserts that
// by node identity. applyI18n writes onto whatever a selector resolves to, so
// two bindings on one node mean the second silently overwrites the first while
// check-i18n reports both as bound.
// ============================================================================

export const TIP_BINDINGS = [
    // Macro panel — the cell is the label + knob + readout row.
    ['.knob[data-param="energy"]',       'tip.energy',       '.knob-row'],
    ['.knob[data-param="brightness"]',   'tip.brightness',   '.knob-row'],
    ['.knob[data-param="texture"]',      'tip.texture',      '.knob-row'],
    ['.knob[data-param="scatterX"]',     'tip.scatterX',     '.knob-row'],
    ['.knob[data-param="scatterY"]',     'tip.scatterY',     '.knob-row'],
    ['.knob[data-param="variation"]',    'tip.variation',    '.knob-row'],

    // Bottom strip — the cell is the knob + caption + readout column.
    ['.knob[data-param="position"]',     'tip.position',     '.bottom-knob-group'],
    ['.knob[data-param="grainSize"]',    'tip.grainSize',    '.bottom-knob-group'],
    ['.knob[data-param="grainDensity"]', 'tip.grainDensity', '.bottom-knob-group'],
    ['.knob[data-param="crossfade"]',    'tip.crossfade',    '.bottom-knob-group'],
    ['.knob[data-param="outputGain"]',   'tip.gain',         '.bottom-knob-group'],

    // Bare — see the header.
    ['#midi-mode',                       'tip.midiMode'],
    ['#gear-btn',                        'tip.settings'],
    ['#lang-select',                     'tip.language'],
];

// The tooltip lookup. Returns {t, b} — never null, never a bare key without a
// console.warn saying so, because a silently-missing tip renders as an empty
// surface that looks like a positioning bug rather than a missing entry.
//
// Live as of v1.2.0: applyI18n() calls it once per TIP_BINDINGS row, on every
// language change. Through v1.1.0 the loop was empty and this function was
// exported unreferenced, so that the canon block stayed byte-identical to the
// other forty-two copies; nothing about its shape changed when the bodies
// arrived, which is what that arrangement was for.
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
