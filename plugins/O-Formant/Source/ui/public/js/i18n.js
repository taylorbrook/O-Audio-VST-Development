/*
   This file is part of O-Formant, an Ouaricon Audio plugin.
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
// i18n.js — O-Formant on-page copy, English + French (v1.26.0, canon v2)
//
// An ES module that EXPORTS ONLY. It must never self-execute: a bare top-level
// statement here throws out of module evaluation and takes every later
// initializer on the page with it (pattern_module_toplevel_init_tdz).
// scripts/check-i18n.js assertion 7 enforces it.
//
// FILENAME: no hyphen. juce_add_binary_data STRIPS hyphens rather than
// converting them to underscores (critical_binary_data_strips_hyphens), so one
// combined file for both languages sidesteps the question entirely.
//
// COPY IS textContent ON EVERY PATH — never innerHTML. check-i18n assertion 9
// rejects any innerHTML reference here and any string literal containing `<`.
//
// THE ENGLISH WAS MOVED, NOT REWRITTEN. Every `en` below is byte-for-byte what
// index.html / main.js / tuning-panel.js carried through v1.25.4, taken from
// scripts/i18n-extract.js's inventory rather than re-typed.
//
// ── WHY I18N CARRIES ONLY EMPTY BODIES ─────────────────────────────────────
// This plugin has NO hover-help copy: TIP_BINDINGS is [] and Stage K does not
// author tooltip prose (that is Stage M). check-i18n assertion 2 accepts zero
// tips only while no I18N entry carries a non-empty body, so every entry below
// is `b: ''`.
//
// I18N is used here for the strings that are NOT written into a DOM element:
// canvas ctx.fillText prose and one window.prompt caption. Those are read
// through trLabel(), and a trLabel() call
// is invisible to assertion 15's `referenced` set (which collects markup
// attributes, literal setLabel keys, literal .dataset.i18n* writes and
// innerHTML-injected keys, and nothing else). Housing them in LABELS would
// therefore report every one of them as a DEAD KEY. Housing them in I18N is
// legal under the contract as written — assertion 15's dead sweep runs over
// LABELS only — and is the shape adopted repo-wide after Stage K batch K3.
//
// NEITHER GATE CAN SEE A CANVAS STRING. Assertion 10 walks TEXT NODES,
// assertion 12 scans textContent / innerText writes, and ctx.fillText is
// neither. Leaving them in English passes green. They are verified here by a
// fillText-recording probe, en -> fr -> en, with its own negative control.
//
// ALL FRENCH IS MACHINE-DRAFTED AND FLAGGED `reviewed: false`. No native
// speaker has read it. `node scripts/check-i18n.js` prints the worklist.
// ============================================================================

export const LANGUAGES = ['en', 'fr'];

export const I18N = Object.freeze({

    // ── Canvas prose (js/main.js) ───────────────────────────────────────────
    // Five fillText/strokeText sites on this page. Three of them paint
    // notation rather than prose and are exempt under D-01 arm 2 — see
    // I18N_EXEMPT below for the IPA glyph tables and the F1..F5 formant
    // markers. The two that paint WORDS are here.

    // The lyrics-mode badge stamped into the top-right of the vowel XY pad
    // while the lyrics engine is driving the cursor (main.js drawXYPad).
    'canvas.lyrics': {
        en: { t: 'LYRICS', b: '' },
        fr: { t: 'PAROLES', b: '', reviewed: false },
    },

    // The manner-of-articulation word in the consonant pad's live readout,
    // "3.0kHz fricative". The number and the unit stay (D-03); the WORD does
    // not, because the axis captions directly above it — Fric / Plos — are
    // [data-i18n] elements and reading French on the axis with English in the
    // readout under it is the exact split this stage exists to close.
    'canvas.plosive':   { en: { t: 'plosive',   b: '' }, fr: { t: 'occlusive', b: '', reviewed: false } },
    'canvas.fricative': { en: { t: 'fricative', b: '' }, fr: { t: 'fricative', b: '', reviewed: false, sameAsEn: true } },
    'canvas.mixed':     { en: { t: 'mixed',     b: '' }, fr: { t: 'mixte',     b: '', reviewed: false } },

    // ── Runtime-composed strings that are not element text ──────────────────

    // window.prompt caption for Save. Not a DOM node, so no [data-i18n]
    // element can own it. See the CHANGELOG note about prompt() itself.
    'js.savePresetAs': {
        en: { t: 'Save preset as:', b: '' },
        fr: { t: 'Enregistrer le préréglage sous :', b: '', reviewed: false },
    },
});

// ============================================================================
// LABELS — the on-page text (v1.26.0, canon v2)
// ============================================================================
//
// One string per key, rendered into a fixed cell that mostly does not wrap.
// `.knob-label` is `white-space: nowrap` inside a 55 px (42 px on the
// consonant envelope, 50 px in the effects rack) `.knob-wrap`, so a French
// caption is measured against ~3.84 px per character at 9 px Garamond and the
// collision threshold with the neighbouring cell is width + gap, not width.
//
// FRENCH IS SIZED, NOT SHRUNK. D-04 forbids an auto-shrink font and a
// short-variant fallback: there is exactly ONE French string per key here and
// nothing chooses between variants at runtime. Where the natural French did
// not fit, the shorter phrasing was chosen once, here, and it is the phrasing
// the plugin ships in.
// ============================================================================

export const LABELS = Object.freeze({

    // ── Header ──────────────────────────────────────────────────────────────
    'label.subtitle':      { en: { t: 'vocal synthesizer' }, fr: { t: 'synthétiseur vocal', reviewed: false } },

    // ── Settings popover (v1.26.0) ──────────────────────────────────────────
    'label.settings':      { en: { t: 'Settings' },   fr: { t: 'Réglages',  reviewed: false } },
    'label.language':      { en: { t: 'Language' },   fr: { t: 'Langue',    reviewed: false } },

    // ── Preset bar ──────────────────────────────────────────────────────────
    // "Sauver", not "Enregistrer": the button is the width driver for the
    // whole centred preset bar (see the .preset-save-btn pin in index.html),
    // and 11 characters against 4 moved four siblings. The prompt this button
    // opens says "Enregistrer le préréglage sous :" in full — it has no box.
    'label.save':          { en: { t: 'Save' },       fr: { t: 'Sauver',    reviewed: false } },
    // The "no filter" sentinel of #preset-category. Its VALUE is the string
    // "all" and that is what populateCategories() and the change handler
    // compare, so the visible text is free to change.
    'label.allCategories': { en: { t: 'All' },        fr: { t: 'Toutes',    reviewed: false } },

    // ── Tabs ────────────────────────────────────────────────────────────────
    'label.tabSynth':      { en: { t: 'Synth' },      fr: { t: 'Synthé',    reviewed: false } },
    'label.tabLyrics':     { en: { t: 'Lyrics' },     fr: { t: 'Paroles',   reviewed: false } },
    'label.tabTuning':     { en: { t: 'Tuning' },     fr: { t: 'Accord',    reviewed: false } },
    'label.tabEffects':    { en: { t: 'Effects' },    fr: { t: 'Effets',    reviewed: false } },

    // ── Synth tab: vowel pad and glottal source ─────────────────────────────
    'label.vowelMorph':    { en: { t: 'Vowel Morph' },    fr: { t: 'Morphose vocalique', reviewed: false } },
    'label.glottalSource': { en: { t: 'Glottal Source' }, fr: { t: 'Source glottique',   reviewed: false } },
    // glottalRd drives the LF model's Rd shape, which IS the voice quality.
    'label.voiceQ':        { en: { t: 'Voice Q' },    fr: { t: 'Qualité',   reviewed: false } },
    'label.breath':        { en: { t: 'Breath' },     fr: { t: 'Souffle',   reviewed: false } },
    'label.vibRate':       { en: { t: 'Vib Rate' },   fr: { t: 'Vib Vitesse', reviewed: false } },
    // "Ampleur" rather than "Profondeur": the cell is 55 px and the standalone
    // Depth knob in the effects rack already owns "Profondeur".
    'label.vibDepth':      { en: { t: 'Vib Depth' },  fr: { t: 'Vib Ampleur', reviewed: false } },
    'label.vibDelay':      { en: { t: 'Vib Delay' },  fr: { t: 'Vib Retard',  reviewed: false } },
    // jitter and shimmer are the terms French voice science uses untranslated.
    'label.jitter':        { en: { t: 'Jitter' },     fr: { t: 'Jitter',    reviewed: false, sameAsEn: true } },
    'label.shimmer':       { en: { t: 'Shimmer' },    fr: { t: 'Shimmer',   reviewed: false, sameAsEn: true } },
    'label.rdMod':         { en: { t: 'Rd Mod' },     fr: { t: 'Mod Rd',    reviewed: false } },
    'label.tilt':          { en: { t: 'Tilt' },       fr: { t: 'Pente',     reviewed: false } },

    // ── Synth tab: consonant ────────────────────────────────────────────────
    'label.consonant':     { en: { t: 'Consonant' },  fr: { t: 'Consonne',  reviewed: false } },
    'label.level':         { en: { t: 'Level' },      fr: { t: 'Niveau',    reviewed: false } },
    'label.voicing':       { en: { t: 'Voicing' },    fr: { t: 'Voisement', reviewed: false } },
    'label.auto':          { en: { t: 'Auto' },       fr: { t: 'Auto',      reviewed: false, sameAsEn: true } },
    // Place and manner of articulation, abbreviated to fit the 8 px overlay on
    // the consonant pad. The French terms are labial / alvéolaire / palatal /
    // vélaire and fricative / occlusive, so four of the six abbreviate the
    // same way and two do not.
    'label.placeLabial':     { en: { t: 'Lab' },  fr: { t: 'Lab',  reviewed: false, sameAsEn: true } },
    'label.placeAlveolar':   { en: { t: 'Alv' },  fr: { t: 'Alv',  reviewed: false, sameAsEn: true } },
    'label.placePalatal':    { en: { t: 'Pal' },  fr: { t: 'Pal',  reviewed: false, sameAsEn: true } },
    'label.placeVelar':      { en: { t: 'Vel' },  fr: { t: 'Vél',  reviewed: false } },
    'label.mannerFricative': { en: { t: 'Fric' }, fr: { t: 'Fric', reviewed: false, sameAsEn: true } },
    'label.mannerPlosive':   { en: { t: 'Plos' }, fr: { t: 'Occl', reviewed: false } },
    // The consonant envelope column: 42 px cells, the tightest on the page.
    'label.attackShort':   { en: { t: 'Atk' },    fr: { t: 'Att',   reviewed: false } },
    'label.hold':          { en: { t: 'Hold' },   fr: { t: 'Tenue', reviewed: false } },
    'label.transShort':    { en: { t: 'Trans' },  fr: { t: 'Trans', reviewed: false, sameAsEn: true } },

    // ── Synth tab: character ────────────────────────────────────────────────
    'label.character':     { en: { t: 'Character' },  fr: { t: 'Caractère', reviewed: false } },
    'label.topology':      { en: { t: 'Topology' },   fr: { t: 'Topologie', reviewed: false } },
    'label.shift':         { en: { t: 'Shift' },      fr: { t: 'Décalage',  reviewed: false } },
    'label.spread':        { en: { t: 'Spread' },     fr: { t: 'Écart',     reviewed: false } },
    'label.glide':         { en: { t: 'Glide' },      fr: { t: 'Glissé',    reviewed: false } },
    'label.transition':    { en: { t: 'Transition' }, fr: { t: 'Transition', reviewed: false, sameAsEn: true } },
    'label.focus':         { en: { t: 'Focus' },      fr: { t: 'Focale',    reviewed: false } },
    // "Formant du chanteur" is 19 characters in a 55 px cell. The abbreviated
    // form is what a French singing-synthesis UI uses.
    'label.singersFormant':{ en: { t: "Singer's F" }, fr: { t: 'F. chanteur', reviewed: false } },
    'label.nasality':      { en: { t: 'Nasality' },   fr: { t: 'Nasalité',  reviewed: false } },
    'label.nasalPlace':    { en: { t: 'Nasal Place' }, fr: { t: 'Lieu nasal', reviewed: false } },

    // ── Synth tab: envelope and output ──────────────────────────────────────
    'label.envelope':      { en: { t: 'Envelope' },   fr: { t: 'Enveloppe', reviewed: false } },
    'label.attack':        { en: { t: 'Attack' },     fr: { t: 'Attaque',   reviewed: false } },
    // Shared by the ADSR decay (55 px) and the consonant decay (42 px):
    // "Déclin" is 6 characters and fits both, so one key, one string.
    'label.decay':         { en: { t: 'Decay' },      fr: { t: 'Déclin',    reviewed: false } },
    'label.sustain':       { en: { t: 'Sustain' },    fr: { t: 'Maintien',  reviewed: false } },
    'label.release':       { en: { t: 'Release' },    fr: { t: 'Relâche',   reviewed: false } },
    'label.output':        { en: { t: 'Output' },     fr: { t: 'Sortie',    reviewed: false } },
    'label.gain':          { en: { t: 'Gain' },       fr: { t: 'Gain',      reviewed: false, sameAsEn: true } },
    'label.width':         { en: { t: 'Width' },      fr: { t: 'Largeur',   reviewed: false } },

    // ── Effects tab ─────────────────────────────────────────────────────────
    'label.chorus':        { en: { t: 'Chorus' },     fr: { t: 'Chorus',    reviewed: false, sameAsEn: true } },
    'label.delay':         { en: { t: 'Delay' },      fr: { t: 'Délai',     reviewed: false } },
    'label.reverb':        { en: { t: 'Reverb' },     fr: { t: 'Réverb',    reviewed: false } },
    'label.eq':            { en: { t: 'EQ' },         fr: { t: 'EQ',        reviewed: false, sameAsEn: true } },
    // The four bypass buttons' two faces. Written from script, so they go
    // through setLabel() and the element becomes a [data-i18n] element from
    // that moment on — a raw literal there is stranded in the previous
    // language the instant the selector fires.
    'label.on':            { en: { t: 'On' },         fr: { t: 'Marche',    reviewed: false } },
    'label.off':           { en: { t: 'Off' },        fr: { t: 'Arrêt',     reviewed: false } },
    'label.rate':          { en: { t: 'Rate' },       fr: { t: 'Vitesse',   reviewed: false } },
    'label.depth':         { en: { t: 'Depth' },      fr: { t: 'Profondeur', reviewed: false } },
    'label.mix':           { en: { t: 'Mix' },        fr: { t: 'Mix',       reviewed: false, sameAsEn: true } },
    'label.time':          { en: { t: 'Time' },       fr: { t: 'Durée',     reviewed: false } },
    'label.feedback':      { en: { t: 'Feedback' },   fr: { t: 'Rétroaction', reviewed: false } },
    // Shared by the delay-mode caption and the tuning panel's rotation-table
    // column header: one word, identical in both languages, one key.
    'label.mode':          { en: { t: 'Mode' },       fr: { t: 'Mode',      reviewed: false, sameAsEn: true } },
    'label.size':          { en: { t: 'Size' },       fr: { t: 'Taille',    reviewed: false } },
    'label.damp':          { en: { t: 'Damp' },       fr: { t: 'Amortis.',  reviewed: false } },
    'label.preDelay':      { en: { t: 'Pre-dly' },    fr: { t: 'Pré-délai', reviewed: false } },
    'label.mod':           { en: { t: 'Mod' },        fr: { t: 'Mod',       reviewed: false, sameAsEn: true } },
    'label.low':           { en: { t: 'Low' },        fr: { t: 'Grave',     reviewed: false } },
    'label.mid':           { en: { t: 'Mid' },        fr: { t: 'Médium',    reviewed: false } },
    'label.midFreq':       { en: { t: 'Mid Freq' },   fr: { t: 'Fréq. méd.', reviewed: false } },
    'label.high':          { en: { t: 'High' },       fr: { t: 'Aigu',      reviewed: false } },

    // ── Lyrics tab ──────────────────────────────────────────────────────────
    'label.arpabetInput':  { en: { t: 'ARPABET Input' }, fr: { t: 'Saisie ARPABET', reviewed: false } },
    'label.enable':        { en: { t: 'Enable' },     fr: { t: 'Activer',   reviewed: false } },
    'label.loop':          { en: { t: 'Loop' },       fr: { t: 'Boucle',    reviewed: false } },
    // "Réinit.", not "Réinitialiser": 13 characters against 5 grew
    // .lyrics-controls by 45 px and dragged four elements left. The full
    // sentence survives on the button's accessible name (aria.resetLyrics),
    // which has no box to fit.
    'label.reset':         { en: { t: 'Reset' },      fr: { t: 'Réinit.',   reviewed: false } },
    // The help line under the ARPABET box is ONE text node in v1.25.4 holding
    // two captions around two runs of phoneme codes. Split into two keyed
    // spans so applyLabel cannot delete the codes with them; the code runs
    // themselves are I18N_EXEMPT notation.
    'label.vowels':        { en: { t: 'Vowels:' },     fr: { t: 'Voyelles :', reviewed: false } },
    'label.consonants':    { en: { t: 'Consonants:' }, fr: { t: 'Consonnes :', reviewed: false } },
    'label.syllables':     { en: { t: 'Syllables' },   fr: { t: 'Syllabes',  reviewed: false } },
    'label.tuningPanelFailed': {
        en: { t: 'Tuning panel failed to load.' },
        fr: { t: 'Le panneau d’accord n’a pas pu se charger.', reviewed: false },
    },

    // ── Tuning tab (js/tuning-panel.js) ─────────────────────────────────────
    //
    // scripts/i18n-extract.js:442 drops `tuning-panel.js` from the WORKLIST by
    // filename with no ownership test, so none of the keys below appears in
    // this plugin's inventory. The file is nevertheless O-Formant's own copy —
    // its header says so, it is 45 lines diverged from
    // modules/tuning/scala-tuning-engine/js/tuning-panel.js, and O-Formant has
    // no dependencies.json listing that module — so localizing it here does
    // not reach another plugin and /module-upgrade will not revert it.
    //
    // check-i18n DOES reach it: its pageModules set is derived from the js
    // directory, so assertions 12, 13 and 15 scan this file like any other.
    'tuning.intervals':    { en: { t: 'Intervals ({n} notes)' },
                             fr: { t: 'Intervalles ({n} notes)', reviewed: false } },
    'tuning.tonic':        { en: { t: 'Tonic' },      fr: { t: 'Tonique',   reviewed: false } },
    // The note count under each library row. "notes" is the same word in
    // French, so the entry exists to KEY the node rather than to change it:
    // an unkeyed node here is indistinguishable from one somebody forgot, and
    // this template is an `html +=` accumulator, which assertion 12 cannot
    // read at all.
    'tuning.noteCount':    { en: { t: '{n} notes' },  fr: { t: '{n} notes', reviewed: false, sameAsEn: true } },
    'tuning.vizCircle':    { en: { t: 'Circle' },     fr: { t: 'Cercle',    reviewed: false } },
    'tuning.vizPolar':     { en: { t: 'Polar' },      fr: { t: 'Polaire',   reviewed: false } },
    'tuning.vizMatrix':    { en: { t: 'Matrix' },     fr: { t: 'Matrice',   reviewed: false } },
    'tuning.vizTrueKeys':  { en: { t: 'True Keys' },  fr: { t: 'Touches',   reviewed: false } },
    'tuning.vizRotation':  { en: { t: 'Rotation' },   fr: { t: 'Rotation',  reviewed: false, sameAsEn: true } },
    'tuning.scaleIntervals': { en: { t: 'Scale Intervals' },
                               fr: { t: 'Intervalles de la gamme', reviewed: false } },
    'tuning.tkHint':       { en: { t: 'Hold 2+ notes to see intervals' },
                             fr: { t: 'Tenir 2 notes ou plus pour voir les intervalles', reviewed: false } },
    'tuning.library':      { en: { t: 'Tuning Library' }, fr: { t: 'Bibliothèque', reviewed: false } },
    // The library filter <option>s. Their VALUE is matched against the
    // `category` field of getEmbeddedTuningList(), so only the text moves.
    'tuning.catAll':       { en: { t: 'All Categories' },   fr: { t: 'Toutes catégories', reviewed: false } },
    'tuning.catHistorical':{ en: { t: 'Historical' },       fr: { t: 'Historiques',   reviewed: false } },
    'tuning.catJust':      { en: { t: 'Just Intonation' },  fr: { t: 'Intonation juste', reviewed: false } },
    'tuning.catEqual':     { en: { t: 'Equal Divisions' },  fr: { t: 'Divisions égales', reviewed: false } },
    'tuning.catNonOctave': { en: { t: 'Non-Octave' },       fr: { t: 'Non octaviantes', reviewed: false } },
    'tuning.catWorld':     { en: { t: 'World' },            fr: { t: 'Du monde',      reviewed: false } },
    // A4 stays: it is scientific pitch notation, and the French octave
    // numbering for the same pitch is La3, which would silently rename the
    // reference the .scl / .kbm files are written against.
    'tuning.a4Ref':        { en: { t: 'A4 REF' },      fr: { t: 'RÉF. A4',   reviewed: false } },
    'tuning.stretch':      { en: { t: 'Stretch' },     fr: { t: 'Étirement', reviewed: false } },
    'tuning.loadScl':      { en: { t: 'Load .SCL' },   fr: { t: 'Ouvrir .SCL', reviewed: false } },
    'tuning.loadKbm':      { en: { t: 'Load .KBM' },   fr: { t: 'Ouvrir .KBM', reviewed: false } },
    'tuning.saveScl':      { en: { t: 'Save .SCL' },   fr: { t: 'Sauver .SCL', reviewed: false } },
    'tuning.saveKbm':      { en: { t: 'Save .KBM' },   fr: { t: 'Sauver .KBM', reviewed: false } },
    'tuning.exportHtml':   { en: { t: 'Export HTML' }, fr: { t: 'Exporter HTML', reviewed: false } },
    'tuning.generateScale':{ en: { t: 'Generate Scale' }, fr: { t: 'Générer une gamme', reviewed: false } },
    'tuning.genEdo':       { en: { t: 'EDO (Equal Division)' }, fr: { t: 'EDO (division égale)', reviewed: false } },
    'tuning.genHarmonic':  { en: { t: 'Harmonic Series' },      fr: { t: 'Série harmonique', reviewed: false } },
    'tuning.genRank2':     { en: { t: 'Rank-2 Temperament' },   fr: { t: 'Tempérament de rang 2', reviewed: false } },
    'tuning.divisions':    { en: { t: 'Divisions' },      fr: { t: 'Divisions', reviewed: false, sameAsEn: true } },
    'tuning.period':       { en: { t: 'Period (c)' },     fr: { t: 'Période (c)', reviewed: false } },
    'tuning.startHarmonic':{ en: { t: 'Start Harmonic' }, fr: { t: 'Harm. initiale', reviewed: false } },
    'tuning.endHarmonic':  { en: { t: 'End Harmonic' },   fr: { t: 'Harm. finale', reviewed: false } },
    'tuning.generator':    { en: { t: 'Generator (c)' },  fr: { t: 'Générateur (c)', reviewed: false } },
    'tuning.notes':        { en: { t: 'Notes' },          fr: { t: 'Notes',    reviewed: false, sameAsEn: true } },
    'tuning.generate':     { en: { t: 'Generate' },       fr: { t: 'Générer',  reviewed: false } },

    // ── Accessible names ────────────────────────────────────────────────────
    // An aria-label is the accessible NAME. A screen reader in French reading
    // an English name is the same failure as a French page with an English
    // caption. None of these has a rendered box, so none is a geometry risk.
    // The four below replace the four native title= attributes v1.25.4
    // carried: their text is MOVED, not re-authored (contract §4).
    'aria.presetPrev':     { en: { t: 'Previous preset' }, fr: { t: 'Préréglage précédent', reviewed: false } },
    'aria.presetNext':     { en: { t: 'Next preset' },     fr: { t: 'Préréglage suivant',   reviewed: false } },
    'aria.loopToggle':     { en: { t: 'Toggle loop' },     fr: { t: 'Activer ou désactiver la boucle', reviewed: false } },
    'aria.resetLyrics':    { en: { t: 'Reset to first syllable' },
                             fr: { t: 'Revenir à la première syllabe', reviewed: false } },
    'aria.langSelect':     { en: { t: 'Interface language' },
                             fr: { t: 'Langue de l’interface', reviewed: false } },
    'placeholder.lyrics':  { en: { t: 'Type ARPABET phonemes separated by spaces (e.g. HH AH L OW W ER L D)' },
                             fr: { t: 'Saisir des phonèmes ARPABET séparés par des espaces (ex. HH AH L OW W ER L D)',
                                   reviewed: false } },
});

// ============================================================================
// I18N_EXEMPT — reasoned exclusions, never silence
// ============================================================================
//
// Every visible string the coverage scan finds must be a [data-i18n] element,
// a setLabel() call, or an entry HERE WITH A REASON. A bare skip list would
// let a missed label hide as a deliberate one.
//
// An entry is [text, reason] or [text, reason, scope]. An exemption is matched
// by TEXT, so an unscoped one silences EVERY node carrying that string. A
// scope is REQUIRED where the same string is also keyed on this page —
// assertion 14 enforces it — and is written here for two entries that are not
// strictly ambiguous but sit beside keyed siblings.
// ============================================================================

export const I18N_EXEMPT = [
    // ── Product identity ────────────────────────────────────────────────────
    ['O-Formant', 'the product name — a product name is never translated'],

    // ── Preset names (D-02) ─────────────────────────────────────────────────
    // #preset-name displays the loaded preset. The name IS the JSON filename
    // (OuariconPresetManager.h), so translating it breaks recall: a session
    // saved against "Cathedral" would not resolve "Cathédrale". "Default" is
    // the placeholder the manager overwrites on its first pass.
    ['Default', 'a factory preset name — the name IS the JSON filename, so translating it breaks recall (D-02)'],

    // ── AudioParameterChoice options, byte-identical (D-01 arm 1) ───────────
    // The page and the host automation lane must agree. Verified verbatim
    // against PluginProcessor.cpp rather than assumed.
    ['Cascade',  'a formantTopology AudioParameterChoice option VERBATIM (PluginProcessor.cpp:218) — D-01 arm 1'],
    ['Parallel', 'a formantTopology AudioParameterChoice option VERBATIM (PluginProcessor.cpp:218) — D-01 arm 1'],
    ['Hybrid',   'a formantTopology AudioParameterChoice option VERBATIM (PluginProcessor.cpp:218) — D-01 arm 1'],
    ['Normal',   'a delayMode AudioParameterChoice option VERBATIM (PluginProcessor.cpp:369) — D-01 arm 1'],
    ['PingPong', 'a delayMode AudioParameterChoice option VERBATIM (PluginProcessor.cpp:369) — D-01 arm 1'],

    // ── Phonetic and scientific notation (D-01 arm 2) ───────────────────────
    // The ARPABET code runs in the lyrics help line. Scoped to .lyrics-help
    // because the two captions that bracket them ARE keyed, and an unscoped
    // entry over a run this long would be the one place a forgotten caption
    // could hide.
    ['AA AE AH AO AW AY EH ER EY IH IY OW OY UH UW |',
     'ARPABET vowel codes — phonetic notation, identical in every language (D-01 arm 2)',
     '.lyrics-help'],
    ['B CH D DH F G HH JH K L M N NG P R S SH T TH V W Y Z ZH',
     'ARPABET consonant codes — phonetic notation, identical in every language (D-01 arm 2)',
     '.lyrics-help'],

    // ── Canvas glyph tables and markers, painted by ctx.fillText ────────────
    // Not reachable by assertion 10 (it walks text nodes) or assertion 12 (it
    // scans textContent writes), so these entries are documentation of a
    // deliberate decision rather than something a gate would otherwise fire
    // on. Written down anyway: the two canvas strings that ARE prose are in
    // I18N above, and an undocumented split between them is exactly how the
    // next reader concludes the rest were forgotten.
    ['i e ɑ o u r l',
     'IPA vowel glyphs painted into the vowel XY pad (main.js vowelLabels) — the International Phonetic Alphabet is notation, not language (D-01 arm 2)'],
    ['p t k f s ʃ m n ŋ',
     'IPA consonant glyphs painted into the consonant XY pad (main.js consonantLabels) — same reason (D-01 arm 2)'],
    ['F1 F2 F3 F4 F5',
     'formant-index markers painted onto the vowel XY pad (main.js drawXYPad) — a letter and a number, language-neutral (D-01 arm 2)'],
    ['Hz',
     'unit symbol in the consonant pad readout, language-neutral (D-03)'],

    // ── Tuning-panel data from the engine ───────────────────────────────────
    // The scale-name display and the library rows render whatever
    // getTuningName() / getEmbeddedTuningList() return. "12-TET Standard" is
    // the authored English fallback that sits in the markup until the first
    // native pull answers; it is an engine tuning NAME, not a caption.
    ['12-TET Standard',
     'the tuning engine\'s own scale name, rendered from getTuningName() — a scale name is data, not a caption'],
];

// ============================================================================
// TIP_BINDINGS — empty by design
// ============================================================================
//
// O-Formant has NO hover-help copy. Stage K localizes a plugin's existing
// visible text and does NOT author tooltip prose; that is Stage M. Nothing
// here can regress a tooltip because there are none, and check-i18n
// assertion 2 accepts zero bindings while no I18N entry carries a non-empty
// body — which is why every I18N entry above is `b: ''`.
// ============================================================================

export const TIP_BINDINGS = [];

export function tr(key, lang, vars) {
    const entry = I18N[key];
    if (!entry) { console.warn(`i18n: missing key ${key}`); return { t: key, b: '' }; }
    const s = entry[lang] || entry.en;

    // A var VALUE that is itself an I18N key resolves to that key's localized
    // title; anything else is used literally. The canon is one shape across
    // all 43 plugins; this function is not trimmed per plugin.
    const resolve = (v) => {
        const nested = I18N[v];
        return nested ? String((nested[lang] || nested.en).t) : String(v);
    };

    const sub = (v) => vars
        ? String(v).replace(/\{(\w+)\}/g, (m, n) => (n in vars ? resolve(vars[n]) : m))
        : String(v);

    return { t: sub(s.t), b: sub(s.b) };
}
