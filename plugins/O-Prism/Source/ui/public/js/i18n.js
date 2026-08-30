/*
   This file is part of O-Prism, an Ouaricon Audio plugin.
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
// i18n.js — O-Prism UI labels, English + French (v1.21.0)
//
// An ES module that EXPORTS ONLY. A bare top-level statement here throws out of
// module evaluation and takes every later initializer on the page with it
// (pattern_module_toplevel_init_tdz). check-i18n assertion 7 enforces it.
//
// O-Prism HAS NO HOVER-HELP COPY. I18N is empty and TIP_BINDINGS is []; every
// string below is a LABEL. Authoring hover-help prose is Stage M and is not
// approved, so nothing here invents copy that was not already on the page.
//
// THE ENGLISH WAS MOVED, NOT REWRITTEN. Every `en` entry is byte-for-byte what
// index.html carried through v1.20.0, with two deliberate exceptions, both
// recorded in CHANGELOG.md:
//   - `label.intervalCount` drops the word "notes" from "Intervals (12 notes)".
//     Contract section 6: French pluralizes zero as singular where English does
//     not, and the existing English already reads "1 notes" on a one-degree
//     scale. The count in a panel captioned "Intervals" needs no noun.
//   - `label.a4Ref` keeps "A4" untranslated inside "Réf. A4". French note
//     naming would make it "La3", and a pitch designation printed beside a
//     440.0 Hz readout is a technical reference, not prose.
//
// FILENAME: no hyphen. juce_add_binary_data STRIPS hyphens rather than
// converting them to underscores (critical_binary_data_strips_hyphens), so one
// combined file for both languages sidesteps the question entirely.
//
// COPY IS textContent ON EVERY PATH — never innerHTML. check-i18n assertion 9
// rejects any innerHTML reference here and any string literal containing `<`.
//
// GEOMETRY BUDGET. 64 of the labels below are knob captions inside
// `.knob-container`, an inline-flex box that SHRINK-WRAPS: it is 52 px wide
// because `.knob-visual` is, and a caption wider than that widens the container
// and pushes every knob to its right. Several French words were shortened for
// that reason and only that reason — `Prof.`, `Réso`, `Étir.`, `Amort.` — and
// the caption-fit probe measured every one against its own content box. A
// native speaker reviewing this file should read those as layout constraints,
// not as preferred vocabulary.
//
// ALL FRENCH IS MACHINE-DRAFTED AND FLAGGED `reviewed: false`. No native
// speaker has read it. `node scripts/check-i18n.js` prints the worklist.
// ============================================================================

export const LANGUAGES = ['en', 'fr'];

// O-Prism has no hover-help. Assertion 2 accepts an empty TIP_BINDINGS only
// while no I18N entry carries a body, which is exactly this state: an emptied
// binding list over a bodied table would be orphaned copy and would fail.
export const I18N = Object.freeze({});

export const TIP_BINDINGS = [];

export const LABELS = Object.freeze({

    // ── Header ──────────────────────────────────────────────────────────────
    // MEASURED CONSTRAINT, not a style preference: `.header-bar` is
    // `justify-content: space-between`, so this caption's width decides where
    // the preset browser sits. `Synthétiseur microtonal à tables d’ondes` is
    // 242.13 px against the English 202.78 and drags the browser 13.1 px left.
    // Anything at or under 202.78 px is free.
    'label.subtitle':        { en: { t: 'Microtonal Wavetable Synthesizer' },
                               fr: { t: 'Synthé microtonal à tables d’onde', reviewed: false } },
    'label.language':        { en: { t: 'Language' },      fr: { t: 'Langue',        reviewed: false } },
    'aria.settings':         { en: { t: 'Settings' },      fr: { t: 'Réglages',      reviewed: false } },
    'aria.presetPrev':       { en: { t: 'Previous Preset' },
                               fr: { t: 'Préréglage précédent', reviewed: false } },
    'aria.presetNext':       { en: { t: 'Next Preset' },   fr: { t: 'Préréglage suivant', reviewed: false } },
    'aria.presetSave':       { en: { t: 'Save Preset' },   fr: { t: 'Enregistrer le préréglage', reviewed: false } },
    'aria.presetBrowse':     { en: { t: 'Click to browse presets' },
                               fr: { t: 'Cliquer pour parcourir les préréglages', reviewed: false } },

    // ── Tabs ────────────────────────────────────────────────────────────────
    // `.tab` is `flex: 1` across the full 1200 px frame, so these five have the
    // most room on the page and are the only captions written out in full.
    'tab.synth':             { en: { t: 'Synth' },         fr: { t: 'Synthé',        reviewed: false } },
    'tab.mod':               { en: { t: 'Mod' },           fr: { t: 'Modul.',        reviewed: false } },
    'tab.tuning':            { en: { t: 'Tuning' },        fr: { t: 'Accord',        reviewed: false } },
    'tab.effects':           { en: { t: 'Effects' },       fr: { t: 'Effets',        reviewed: false } },
    'tab.wavetable':         { en: { t: 'Wavetable' },     fr: { t: 'Table d’onde',  reviewed: false } },

    // ── Synth tab: section headers ──────────────────────────────────────────
    'label.oscA':            { en: { t: 'Oscillator A' },  fr: { t: 'Oscillateur A', reviewed: false } },
    'label.oscB':            { en: { t: 'Oscillator B' },  fr: { t: 'Oscillateur B', reviewed: false } },
    'label.subOsc':          { en: { t: 'Sub Oscillator' }, fr: { t: 'Sous-oscillateur', reviewed: false } },
    'label.noise':           { en: { t: 'Noise' },         fr: { t: 'Bruit',         reviewed: false } },
    'label.performance':     { en: { t: 'Performance' },   fr: { t: 'Performance',   reviewed: false, sameAsEn: true } },
    'label.filterA':         { en: { t: 'Filter A' },      fr: { t: 'Filtre A',      reviewed: false } },
    'label.filterB':         { en: { t: 'Filter B' },      fr: { t: 'Filtre B',      reviewed: false } },
    'label.ampEnv':          { en: { t: 'Amp Envelope' },  fr: { t: 'Enveloppe ampli', reviewed: false } },
    'label.filtEnv':         { en: { t: 'Filter Envelope' }, fr: { t: 'Enveloppe filtre', reviewed: false } },
    // The four LFO headers hold BUTTON CHILDREN as well as this text, so each
    // one is split into its own <span> (contract section 5). Writing textContent
    // on the header itself would delete the Free / Retrig buttons beside it.
    'label.lfo1':            { en: { t: 'LFO 1' },         fr: { t: 'OBF 1',         reviewed: false } },
    'label.lfo2':            { en: { t: 'LFO 2' },         fr: { t: 'OBF 2',         reviewed: false } },
    'label.lfo3':            { en: { t: 'LFO 3' },         fr: { t: 'OBF 3',         reviewed: false } },
    'label.lfo4':            { en: { t: 'LFO 4' },         fr: { t: 'OBF 4',         reviewed: false } },

    // ── Dropdown captions ───────────────────────────────────────────────────
    // `.dropdown-group` is inline-flex and shrink-wraps around the WIDER of its
    // caption and its <select>, so a caption longer than the select widens the
    // group and pushes every control to its right.
    'label.shape':           { en: { t: 'Shape' },         fr: { t: 'Forme',         reviewed: false } },
    'label.warp':            { en: { t: 'Warp' },          fr: { t: 'Déform.',       reviewed: false } },
    'label.octave':          { en: { t: 'Octave' },        fr: { t: 'Octave',        reviewed: false, sameAsEn: true } },
    'label.routing':         { en: { t: 'Routing' },       fr: { t: 'Routage',       reviewed: false } },
    'label.type':            { en: { t: 'Type' },          fr: { t: 'Type',          reviewed: false, sameAsEn: true } },
    'label.glideMode':       { en: { t: 'Glide Mode' },    fr: { t: 'Mode glissé',   reviewed: false } },
    'label.filterRouting':   { en: { t: 'Filter Routing' }, fr: { t: 'Routage filt.', reviewed: false } },
    'label.division':        { en: { t: 'Division' },      fr: { t: 'Division',      reviewed: false, sameAsEn: true } },
    'label.mode':            { en: { t: 'Mode' },          fr: { t: 'Mode',          reviewed: false, sameAsEn: true } },
    'label.dropWav':         { en: { t: 'Drop WAV' },      fr: { t: 'Déposer WAV',   reviewed: false } },

    // ── The 64 knob captions ────────────────────────────────────────────────
    // Keyed on the STATIC `.knob-container[data-i18n]`, moved onto the generated
    // `.knob-label` span by expandKnobMarkup(). 64 attributes, 35 distinct
    // strings, 35 keys — one per string, shared wherever the caption repeats.
    'label.position':        { en: { t: 'Position' },      fr: { t: 'Position',      reviewed: false, sameAsEn: true } },
    'label.level':           { en: { t: 'Level' },         fr: { t: 'Niv.',          reviewed: false } },
    'label.pan':             { en: { t: 'Pan' },           fr: { t: 'Pano',          reviewed: false } },
    // `Grossier` is 51.75 px and clears neither test: the knob column is 52 px, and
    // the neighbouring knob's ROTATED svg puts its own bounding box 5.1 px into
    // this column, which caps a caption here at 49.8 px.
    'label.coarse':          { en: { t: 'Coarse' },        fr: { t: 'Gross.',        reviewed: false } },
    'label.fine':            { en: { t: 'Fine' },          fr: { t: 'Fin',           reviewed: false } },
    'label.phase':           { en: { t: 'Phase' },         fr: { t: 'Phase',         reviewed: false, sameAsEn: true } },
    'label.unison':          { en: { t: 'Unison' },        fr: { t: 'Unisson',       reviewed: false } },
    'label.detune':          { en: { t: 'Detune' },        fr: { t: 'Désacc.',       reviewed: false } },
    'label.width':           { en: { t: 'Width' },         fr: { t: 'Larg.',         reviewed: false } },
    'label.warpAmt':         { en: { t: 'Warp Amt' },      fr: { t: 'Qté déf.',      reviewed: false } },
    'label.pbRange':         { en: { t: 'PB Range' },      fr: { t: 'Ampl. PB',      reviewed: false } },
    'label.glide':           { en: { t: 'Glide' },         fr: { t: 'Glissé',        reviewed: false } },
    'label.cutoff':          { en: { t: 'Cutoff' },        fr: { t: 'Coupure',       reviewed: false } },
    'label.reso':            { en: { t: 'Reso' },          fr: { t: 'Réso',          reviewed: false } },
    'label.drive':           { en: { t: 'Drive' },         fr: { t: 'Satur.',        reviewed: false } },
    'label.keyTrk':          { en: { t: 'Key Trk' },       fr: { t: 'Suivi',         reviewed: false } },
    'label.attack':          { en: { t: 'Attack' },        fr: { t: 'Attaque',       reviewed: false } },
    'label.decay':           { en: { t: 'Decay' },         fr: { t: 'Chute',         reviewed: false } },
    'label.sustain':         { en: { t: 'Sustain' },       fr: { t: 'Tenue',         reviewed: false } },
    'label.release':         { en: { t: 'Release' },       fr: { t: 'Relâche',       reviewed: false } },
    'label.depA':            { en: { t: 'Dep A' },         fr: { t: 'Prof A',        reviewed: false } },
    'label.depB':            { en: { t: 'Dep B' },         fr: { t: 'Prof B',        reviewed: false } },
    'label.rate':            { en: { t: 'Rate' },          fr: { t: 'Vit.',          reviewed: false } },
    'label.time':            { en: { t: 'Time' },          fr: { t: 'Temps',         reviewed: false } },
    'label.feedback':        { en: { t: 'Feedback' },      fr: { t: 'Réaction',      reviewed: false } },
    // `Mix` is the word French audio software uses; `Dosage` is more correct and
    // 21 px wider, which is 3 px past the 38.45 px this column can hold.
    'label.mix':             { en: { t: 'Mix' },           fr: { t: 'Mix',           reviewed: false, sameAsEn: true } },
    'label.depth':           { en: { t: 'Depth' },         fr: { t: 'Prof.',         reviewed: false } },
    'label.size':            { en: { t: 'Size' },          fr: { t: 'Taille',        reviewed: false } },
    'label.damp':            { en: { t: 'Damp' },          fr: { t: 'Amor.',         reviewed: false } },
    'label.preDly':          { en: { t: 'Pre-Dly' },       fr: { t: 'Pré-dél.',      reviewed: false } },
    'label.modAmt':          { en: { t: 'Mod' },           fr: { t: 'Mod.',          reviewed: false } },
    'label.low':             { en: { t: 'Low' },           fr: { t: 'Grave',         reviewed: false } },
    'label.mid':             { en: { t: 'Mid' },           fr: { t: 'Méd.',          reviewed: false } },
    'label.midFreq':         { en: { t: 'Mid Freq' },      fr: { t: 'Fq. méd',       reviewed: false } },
    'label.high':            { en: { t: 'High' },          fr: { t: 'Aigu',          reviewed: false } },

    // ── Mod matrix ──────────────────────────────────────────────────────────
    'label.modMatrix':       { en: { t: 'Modulation Matrix' },
                               fr: { t: 'Matrice de modulation', reviewed: false } },
    'label.modMatrixInfo':   { en: { t: 'Route any source to any destination. 16 slots available.' },
                               fr: { t: 'Acheminer n’importe quelle source vers n’importe quelle destination. 16 emplacements.', reviewed: false } },
    // `.mod-col-on` is a fixed 36 px column, which is what decides this against
    // the fuller `Activé`.
    'label.colOn':           { en: { t: 'On' },            fr: { t: 'Act.',          reviewed: false } },
    'label.colSource':       { en: { t: 'Source' },        fr: { t: 'Source',        reviewed: false, sameAsEn: true } },
    'label.colDest':         { en: { t: 'Destination' },   fr: { t: 'Destination',   reviewed: false, sameAsEn: true } },
    'label.colAmount':       { en: { t: 'Amount' },        fr: { t: 'Quantité',      reviewed: false } },

    // ── Tuning tab ──────────────────────────────────────────────────────────
    // The count is a {token} and the noun is gone — see the header note.
    'label.intervalCount':   { en: { t: 'Intervals ({n})' },
                               fr: { t: 'Intervalles ({n})', reviewed: false } },
    'label.tonic':           { en: { t: 'Tonic:' },        fr: { t: 'Ton. :',        reviewed: false } },
    'label.vizCircle':       { en: { t: 'Circle' },        fr: { t: 'Cercle',        reviewed: false } },
    'label.vizPolar':        { en: { t: 'Polar' },         fr: { t: 'Polaire',       reviewed: false } },
    'label.vizMatrix':       { en: { t: 'Matrix' },        fr: { t: 'Matrice',       reviewed: false } },
    'label.vizTrueKeys':     { en: { t: 'True Keys' },     fr: { t: 'Touches',       reviewed: false } },
    'label.vizRotation':     { en: { t: 'Rotation' },      fr: { t: 'Rotation',      reviewed: false, sameAsEn: true } },
    'label.scaleIntervals':  { en: { t: 'Scale Intervals' }, fr: { t: 'Intervalles de la gamme', reviewed: false } },
    'label.tkHint':          { en: { t: 'Hold 2+ notes to see intervals' },
                               fr: { t: 'Tenir 2 notes ou plus pour voir les intervalles', reviewed: false } },
    'label.heldIntervals':   { en: { t: 'Intervals:' },    fr: { t: 'Interv. :',     reviewed: false } },
    'label.span':            { en: { t: 'Span' },          fr: { t: 'Écart',         reviewed: false } },
    'label.totalSpan':       { en: { t: 'Total span' },    fr: { t: 'Écart total',   reviewed: false } },
    'label.rotationMode':    { en: { t: 'Mode' },          fr: { t: 'Mode',          reviewed: false, sameAsEn: true } },
    'label.tuningLibrary':   { en: { t: 'Tuning Library' }, fr: { t: 'Bibliothèque',  reviewed: false } },
    // The five category captions are keyed HERE, on the filter <select>'s
    // options, and the same five keys are reused by the library list's own
    // category span. The <option value="..."> attributes stay English because
    // they are matched against the C++ category strings.
    'label.catAll':          { en: { t: 'All Categories' }, fr: { t: 'Toutes catégories', reviewed: false } },
    'label.catHistorical':   { en: { t: 'Historical' },    fr: { t: 'Historique',    reviewed: false } },
    'label.catJust':         { en: { t: 'Just Intonation' }, fr: { t: 'Intonation juste', reviewed: false } },
    'label.catEqual':        { en: { t: 'Equal Divisions' }, fr: { t: 'Divisions égales', reviewed: false } },
    'label.catNonOctave':    { en: { t: 'Non-Octave' },    fr: { t: 'Non-octaviant', reviewed: false } },
    'label.catWorld':        { en: { t: 'World' },         fr: { t: 'Monde',         reviewed: false } },
    'label.libNotes':        { en: { t: 'notes' },         fr: { t: 'notes',         reviewed: false, sameAsEn: true } },
    'label.libPeriod':       { en: { t: 'period' },        fr: { t: 'période',       reviewed: false } },
    // `.knob-label` inside the bespoke A4 knob — same 52 px column as the 64.
    'label.a4Ref':           { en: { t: 'A4 Ref' },        fr: { t: 'Réf. A4',       reviewed: false } },
    // `.octave-stretch-label` is `min-width: 40px` in a 210 px panel and the
    // slider beside it takes the remainder, so `Étirement` would shrink the
    // slider by 8 px. That is what decides this abbreviation.
    'label.stretch':         { en: { t: 'Stretch' },       fr: { t: 'Étir.',         reviewed: false } },
    'label.loadScl':         { en: { t: 'Load .SCL' },     fr: { t: 'Charger .SCL',  reviewed: false } },
    'label.loadKbm':         { en: { t: 'Load .KBM' },     fr: { t: 'Charger .KBM',  reviewed: false } },
    'label.saveScl':         { en: { t: 'Save .SCL' },     fr: { t: 'Enreg. .SCL',   reviewed: false } },
    'label.saveKbm':         { en: { t: 'Save .KBM' },     fr: { t: 'Enreg. .KBM',   reviewed: false } },
    'label.exportHtml':      { en: { t: 'Export HTML' },   fr: { t: 'Exporter HTML', reviewed: false } },
    'label.generateScale':   { en: { t: 'Generate Scale' }, fr: { t: 'Générer gamme', reviewed: false } },
    'label.genEdo':          { en: { t: 'EDO (Equal Division)' },
                               fr: { t: 'EDO (division égale)', reviewed: false } },
    'label.genHarmonic':     { en: { t: 'Harmonic Series' }, fr: { t: 'Série harmonique', reviewed: false } },
    'label.genRank2':        { en: { t: 'Rank-2 Temperament' },
                               fr: { t: 'Tempérament de rang 2', reviewed: false } },
    'label.genDivisions':    { en: { t: 'Divisions' },     fr: { t: 'Divisions',     reviewed: false, sameAsEn: true } },
    'label.genPeriod':       { en: { t: 'Period (cents)' }, fr: { t: 'Période (cents)', reviewed: false } },
    'label.genStartHarm':    { en: { t: 'Start Harmonic' }, fr: { t: 'Harmonique de départ', reviewed: false } },
    'label.genEndHarm':      { en: { t: 'End Harmonic' },  fr: { t: 'Harmonique de fin', reviewed: false } },
    'label.genGenerator':    { en: { t: 'Generator (cents)' },
                               fr: { t: 'Générateur (cents)', reviewed: false } },
    'label.genNotes':        { en: { t: 'Notes' },         fr: { t: 'Notes',         reviewed: false, sameAsEn: true } },
    'label.generate':        { en: { t: 'Generate' },      fr: { t: 'Générer',       reviewed: false } },

    // ── Effects tab ─────────────────────────────────────────────────────────
    'label.delay':           { en: { t: 'Delay' },         fr: { t: 'Délai',         reviewed: false } },
    'label.chorus':          { en: { t: 'Chorus' },        fr: { t: 'Chorus',        reviewed: false, sameAsEn: true } },
    'label.distortion':      { en: { t: 'Distortion' },    fr: { t: 'Distorsion',    reviewed: false } },
    'label.reverb':          { en: { t: 'Reverb' },        fr: { t: 'Réverb.',       reviewed: false } },
    'label.eq3':             { en: { t: '3-Band EQ' },     fr: { t: 'ÉG 3 bandes',   reviewed: false } },
    // The caption above the delay Sync toggle. `Synchro` is 21 px wider and the
    // cell it sits in is 44.44 px, so it pushed the whole delay row 16 px.
    // `Sync` is current usage in French DAWs; the LFO button keeps `Synchro`,
    // where a section header gives it room.
    'label.sync':            { en: { t: 'Sync' },          fr: { t: 'Sync',          reviewed: false, sameAsEn: true } },

    // ── Wavetable tab ───────────────────────────────────────────────────────
    // `Osc A` is already the French abbreviation; the added period cost 4.2 px in
    // a shrink-wrapping toggle that pushes the harmonic toolbar behind it.
    'label.oscAShort':       { en: { t: 'Osc A' },         fr: { t: 'Osc A',         reviewed: false, sameAsEn: true } },
    'label.oscBShort':       { en: { t: 'Osc B' },         fr: { t: 'Osc B',         reviewed: false, sameAsEn: true } },
    // The seven ops-bar captions below are pinned to their English boxes in CSS,
    // so the row's three separators and its undo/redo pair hold still. Each
    // French string is the longest form that FITS its own English button:
    // `Normaliser` is 60.06 px in a 55.19 px box, `Fondre bords` 71.48 in
    // 60.61, `Inverser` 44.66 in 41.91, `Inverser ordre` 76.89 in 76.39, and
    // `Enregistrer` 60.72 in 24.50. The alternative was pinning to the FRENCH
    // and moving the English row 38 px, which is a visible change to a shipped
    // English UI for no English benefit.
    'label.harmonics':       { en: { t: 'Harmonics' },     fr: { t: 'Harmon.',       reviewed: false } },
    'label.waveform':        { en: { t: 'Waveform' },      fr: { t: 'Forme d’onde',  reviewed: false } },
    'label.normalize':       { en: { t: 'Normalize' },     fr: { t: 'Norm.',         reviewed: false } },
    'label.normalizeGlobal': { en: { t: 'Normalize Global' }, fr: { t: 'Normaliser tout', reviewed: false } },
    'label.fadeEdges':       { en: { t: 'Fade Edges' },    fr: { t: 'Fondre',        reviewed: false } },
    'label.reverse':         { en: { t: 'Reverse' },       fr: { t: 'Invers.',       reviewed: false } },
    'label.reverseOrder':    { en: { t: 'Reverse Order' }, fr: { t: 'Ordre inv.',    reviewed: false } },
    'label.smooth':          { en: { t: 'Smooth' },        fr: { t: 'Lisser',        reviewed: false } },
    // TWO keys for one English word. The two modal buttons have room for
    // `Enregistrer`; the ops-bar button is a 24.50 px box.
    'label.save':            { en: { t: 'Save' },          fr: { t: 'Enregistrer',   reviewed: false } },
    'label.saveShort':       { en: { t: 'Save' },          fr: { t: 'Enr.',          reviewed: false } },
    'label.cancel':          { en: { t: 'Cancel' },        fr: { t: 'Annuler',       reviewed: false } },
    'label.saveWavetable':   { en: { t: 'Save Wavetable' }, fr: { t: 'Enregistrer la table', reviewed: false } },
    'label.savePreset':      { en: { t: 'Save Preset' },   fr: { t: 'Enregistrer le préréglage', reviewed: false } },
    'label.userWavetables':  { en: { t: 'User Wavetables' }, fr: { t: 'Tables utilisateur', reviewed: false } },
    'label.close':           { en: { t: 'Close' },         fr: { t: 'Fermer',        reviewed: false } },
    'label.delete':          { en: { t: 'Delete' },        fr: { t: 'Supprimer',     reviewed: false } },
    'label.importWav':       { en: { t: 'Import WAV...' }, fr: { t: 'Importer WAV…', reviewed: false } },
    'label.manage':          { en: { t: 'Manage...' },     fr: { t: 'Gérer…',        reviewed: false } },
    'label.noUserWavetables': { en: { t: 'No user wavetables imported yet.' },
                               fr: { t: 'Aucune table utilisateur importée.', reviewed: false } },
    'aria.wavetableName':    { en: { t: 'Wavetable name...' }, fr: { t: 'Nom de la table…', reviewed: false } },
    'aria.presetName':       { en: { t: 'Preset name...' }, fr: { t: 'Nom du préréglage…', reviewed: false } },
    'aria.undo':             { en: { t: 'Undo (Ctrl+Z)' }, fr: { t: 'Annuler (Ctrl+Z)', reviewed: false } },
    'aria.redo':             { en: { t: 'Redo (Ctrl+Shift+Z)' },
                               fr: { t: 'Rétablir (Ctrl+Maj+Z)', reviewed: false } },

    // ── Footer ──────────────────────────────────────────────────────────────
    'label.master':          { en: { t: 'Master' },        fr: { t: 'Maître',        reviewed: false } },
    'label.oscMix':          { en: { t: 'Osc Mix' },       fr: { t: 'Mix osc',       reviewed: false } },

    // ── State faces written from script ─────────────────────────────────────
    // Every one goes through setLabel(), so the element becomes a [data-i18n]
    // element from that moment on and the language sweep owns it. A state string
    // written as a raw literal is stranded in the previous language the instant
    // the selector fires.
    //
    // These are the SIX TOGGLE BUTTON FACES, and they are localized rather than
    // exempted even though `delaySync`, `lfoNSync`, `lfoNFreeRun` and the five
    // bypass flags are all parameters. D-01 arm 1 exempts an
    // `AudioParameterChoice` OPTION that the page reproduces byte-identically;
    // every one of these is an `AudioParameterBool`, whose host text is JUCE's
    // generic Off/On boilerplate and not an authored choice name. `Free`,
    // `Retrig`, `Free Run` and `Sync` are not byte-identical to anything in the
    // automation lane in the first place.
    'ui.free':               { en: { t: 'Free' },          fr: { t: 'Libre',         reviewed: false } },
    'ui.sync':               { en: { t: 'Sync' },          fr: { t: 'Synchro',       reviewed: false } },
    // "Free Run" and "Free" are different concepts on adjacent buttons — one is
    // "not tempo-synced", the other is "phase runs across notes" — so they get
    // different French rather than colliding on `Libre`.
    'ui.freeRun':            { en: { t: 'Free Run' },      fr: { t: 'Continu',       reviewed: false } },
    'ui.retrig':             { en: { t: 'Retrig' },        fr: { t: 'Redécl.',       reviewed: false } },
    // The five bypass buttons carry the same two words in the markup's own
    // upper case. Separate keys, because the ownership mirror asserts
    // dataset.label === textContent and CSS text-transform is not textContent.
    'ui.bypassOn':           { en: { t: 'ON' },            fr: { t: 'MARCHE',        reviewed: false } },
    'ui.bypassOff':          { en: { t: 'OFF' },           fr: { t: 'ARRÊT',         reviewed: false } },
});

// ============================================================================
// I18N_EXEMPT — reasoned exclusions, never silence
// ============================================================================
//
// Every visible string the coverage scan finds must be a [data-i18n] element, a
// setLabel() call, or an entry HERE WITH A REASON. A bare skip list would let a
// missed label hide as a deliberate one.
//
// AN EXEMPTION IS MATCHED BY TEXT, so an unscoped one silences EVERY node with
// that string. On this page that hazard is real and not theoretical: `Sine`,
// `Square`, `Triangle`, `Saw`, `Harmonic Series`, `Off`, `Sync`, `Wind` and
// `Digital` each appear BOTH as a parameter dropdown option (exempt) and as a
// caption or a non-parameter option that must translate. So every option
// exemption below is SCOPED to `.param-select`, the class the 23 parameter
// dropdowns carry and the library filter and the scale generator do not.
// ============================================================================

export const I18N_EXEMPT = [
    // ── The delay Sync toggle face — D-01 arm 1, and the geometry that decided it
    // #toggle-delaySync shows `On` / `Off`, which is BYTE-IDENTICAL to what the
    // `delaySync` parameter reports to the host (measured in the runtime dump:
    // textAtMin "Off", textAtMax "On"). The five BYPASS buttons are keyed rather
    // than exempted because their faces are `ON` / `OFF` in the markup's own
    // upper case, which is not byte-identical to anything.
    //
    // Geometry made the call unambiguous: this button and its caption share a
    // 44.44 px cell in the middle of the delay row, and `Arrêt` / `Marche` push
    // every control to their right by 16 px. Scoped, because `Off` is also a
    // parameter dropdown option and `On` is also the mod-matrix column caption
    // `label.colOn`, which IS keyed.
    ['On',  'the #toggle-delaySync face, byte-identical to the delaySync parameter\'s '
          + 'host text ("Off"/"On", runtime param dump) — D-01 arm 1', '#toggle-delaySync'],
    ['Off', 'the #toggle-delaySync face, byte-identical to the delaySync parameter\'s '
          + 'host text ("Off"/"On", runtime param dump) — D-01 arm 1', '#toggle-delaySync'],

    // ── The product name ───────────────────────────────────────────────────
    ['O-PRISM', 'the product name — a product name is never translated'],

    // ── D-02: a name that IS an identifier ─────────────────────────────────
    ['— Init —',
     'the placeholder in #preset-current-name, which DISPLAYS a preset name. '
     + 'The name IS the JSON filename (OuariconPresetManager.h), so translating it '
     + 'breaks recall: a session saved against "Cathedral" would not resolve '
     + '"Cathédrale". Confirmed to be the name span and not a caption beside it — '
     + '#preset-current-category is its sibling and carries the category',
     '#preset-current-name'],

    // ── D-01 arm 3: a node that otherwise holds backend-supplied data ──────
    ['12-TET Standard',
     'the fallback written into #scale-name-display, whose every other value comes '
     + 'from TuningEngine::getScaleName() — an .scl filename, a library entry, or a '
     + 'generated name. D-01 arm 3: keying a node that holds data would make it '
     + 'enter and leave the language sweep as the scale changes',
     '#scale-name-display'],

    // ── D-01 arm 1: AudioParameterChoice options, byte-identical ───────────
    // Verified against the RUNTIME parameter dump (173 parameters,
    // .planning/params.tsv) and the StringArrays in PluginProcessor.cpp.
    // Localizing one would make the page and the host automation lane disagree
    // about what the user just selected.
    ...[
        ['Off',        'oscA/BWarpType + glideMode option (PluginProcessor.cpp:109,247)'],
        ['Sync',       'oscA/BWarpType option (PluginProcessor.cpp:109)'],
        ['Bend',       'oscA/BWarpType option (PluginProcessor.cpp:109)'],
        ['FM',         'oscA/BWarpType option (PluginProcessor.cpp:109)'],
        ['Window',     'oscA/BWarpType option (PluginProcessor.cpp:109)'],
        ['-1 Oct',     'subOctave option (PluginProcessor.cpp:126)'],
        ['-2 Oct',     'subOctave option (PluginProcessor.cpp:126)'],
        ['-3 Oct',     'subOctave option (PluginProcessor.cpp:126)'],
        ['-4 Oct',     'subOctave option (PluginProcessor.cpp:126)'],
        ['Post-Filter', 'subRouting option (PluginProcessor.cpp:138)'],
        ['Pre-Filter', 'subRouting option (PluginProcessor.cpp:138)'],
        ['White',      'noiseType option (PluginProcessor.cpp:132)'],
        ['Pink',       'noiseType option (PluginProcessor.cpp:132)'],
        ['Brown',      'noiseType option (PluginProcessor.cpp:132)'],
        ['Digital',    'noiseType option (PluginProcessor.cpp:132)'],
        ['Vinyl',      'noiseType option (PluginProcessor.cpp:132)'],
        ['Legato',     'glideMode option (PluginProcessor.cpp:247)'],
        ['Always',     'glideMode option (PluginProcessor.cpp:247)'],
        ['LP12',       'filtA/BType option (PluginProcessor.cpp:197)'],
        ['LP24',       'filtA/BType option (PluginProcessor.cpp:197)'],
        ['HP12',       'filtA/BType option (PluginProcessor.cpp:197)'],
        ['HP24',       'filtA/BType option (PluginProcessor.cpp:197)'],
        ['BP12',       'filtA/BType option (PluginProcessor.cpp:197)'],
        ['BP24',       'filtA/BType option (PluginProcessor.cpp:197)'],
        ['Notch',      'filtA/BType option (PluginProcessor.cpp:197)'],
        ['Serial',     'filtRouting option (PluginProcessor.cpp:220)'],
        ['Parallel',   'filtRouting option (PluginProcessor.cpp:220)'],
        ['Normal',     'delayMode option (PluginProcessor.cpp:305)'],
        ['PingPong',   'delayMode option (PluginProcessor.cpp:305)'],
        ['SoftClip',   'distType option (PluginProcessor.cpp:340)'],
        ['HardClip',   'distType option (PluginProcessor.cpp:340)'],
        ['Tube',       'distType option (PluginProcessor.cpp:340)'],
        ['Fold',       'distType option (PluginProcessor.cpp:340)'],
    ].map(([t, where]) => [t,
        'a parameter dropdown option reproduced BYTE-IDENTICALLY from ' + where
        + ' — D-01 arm 1: the page and the host automation lane must agree',
        '.param-select']),

    // ── The wavetable catalogue: 28 option texts + their optgroup labels ────
    // Not arm 1 — `oscATable` is an AudioParameterInt (0..27, host text "0".."27"),
    // so there is no choice option to match. They are exempt for two other
    // reasons, and either alone would be enough:
    //   1. They are a hand-mirrored copy of WavetableFactory::getTableInfoList()
    //      (WavetableFactory.cpp:93-130), a C++-owned catalogue of factory
    //      content NAMES. The optgroup labels mirror its category column.
    //   2. Four of them — Saw, Square, Triangle, Sine — are byte-identical to
    //      subShape and lfoNShape options that ARE arm 1. Translating the osc
    //      list would make the same word French in one dropdown on the page and
    //      English in the next.
    ...[
        'Saw', 'Square', 'Triangle', 'Sine', 'PWM Sweep', 'Supersaw', 'Sync Sweep',
        'FM E.Piano', 'FM Bell', 'FM Metallic', 'Wavefold', 'Bitcrush',
        'Vowel Morph', 'Choir Pad', 'Vocal Lead', 'Formant Filter',
        'Harmonic Series', 'Spectral Tilt', 'Odd Harmonics', 'Harmonic Stretch',
        'Comb Sweep', 'Prism Spectrum',
        'Breath', 'Plucked String', 'Church Bell', 'Organ Sweep', 'Wind', 'Filtered Noise',
    ].map((t) => [t,
        'a factory wavetable NAME, mirrored from WavetableFactory::getTableInfoList() '
        + '(WavetableFactory.cpp:93-130). Its parameter is an AudioParameterInt, so '
        + 'arm 1 does not reach it; it is exempt as C++-owned content, and four of the '
        + '28 are byte-identical to arm-1 subShape/lfoNShape options on the same page',
        '.param-select']),

    // ── The 37 modulation matrix names ─────────────────────────────────────
    // getModSourceNames() / getModDestNames() (dsp/ModulationMatrix.h:86-101)
    // feed BOTH the page (over the getModSourceNames / getModDestNames native
    // fns, PluginEditor.cpp) and the mod-slot AudioParameterChoice options
    // (PluginProcessor.cpp:415-427). Byte-identical BY CONSTRUCTION — D-01 arm 1.
    //
    // Scoped even though they are injected rather than authored, because `Osc Mix`
    // is ALSO the footer caption `label.oscMix`: unscoped, this entry would
    // silence a live keyed node and assertion 14 could not tell a deliberate
    // skip from a forgotten label.
    ...[
        'None', 'LFO1', 'LFO2', 'LFO3', 'LFO4', 'AmpEnv', 'FilterEnv',
        'Velocity', 'NoteNum', 'ModWheel', 'Aftertouch',
        'OscA Pos', 'OscB Pos', 'FiltA Cut', 'FiltB Cut', 'FiltA Res', 'FiltB Res',
        'Osc Mix', 'Sub Level', 'Noise Level',
        'LFO1 Rate', 'LFO2 Rate', 'LFO3 Rate', 'LFO4 Rate',
        'OscA Detune', 'OscB Detune', 'OscA Pan', 'OscB Pan',
        'Reverb Mix', 'Delay Mix', 'Chorus Mix', 'Dist Mix', 'Master Vol', 'Pitch',
        'OscA Warp', 'OscB Warp',
    ].map((t) => [t,
        'a modulation source/destination name from dsp/ModulationMatrix.h:86-101, which '
        + 'builds the mod-slot AudioParameterChoice options in PluginProcessor.cpp:415-427 '
        + 'and is pushed to the page over the same two functions — byte-identical by '
        + 'construction, D-01 arm 1',
        '#mod-matrix-rows']),

    // ── D-01 arm 1, written from script ────────────────────────────────────
    ['Custom',
     'written into #scale-name-display when an interval is hand-edited, and '
     + 'byte-identical to the last tuningPreset choice option '
     + '(PluginProcessor.cpp:231-233) — D-01 arm 1. It is also arm 3: the node it '
     + 'lands in holds backend-supplied scale names'],

    // ── Language endonyms ──────────────────────────────────────────────────
    ['English',  'an endonym — a language name is never translated'],
    ['Français', 'an endonym — a language name is never translated'],
];

// The canon imports tr() alongside the tables. O-Prism binds no tooltips, so
// nothing on this page calls it today — but the canon block is ONE shape across
// all 43 plugins and is not trimmed per plugin, and an import of a name this
// module does not export throws at module evaluation and takes the whole UI
// down (pattern_module_toplevel_init_tdz).
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

    const sub = (v) => vars
        ? String(v).replace(/\{(\w+)\}/g, (m, n) => (n in vars ? resolve(vars[n]) : m))
        : String(v);

    return { t: sub(s.t), b: sub(s.b) };
}
