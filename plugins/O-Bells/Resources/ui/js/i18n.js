/*
   This file is part of O-Bells, an Ouaricon Audio plugin.
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
// i18n.js — O-Bells UI copy, English + French (v4.2.0, canon v2)
//
// LABELS ONLY. This plugin ships no hover-help: it had no data-tip renderer and
// no tooltip copy at v4.1.5, only one native alt= and one native title= written
// from JS. Authoring hover-help prose is Stage M's job, not this file's, so
// I18N is EMPTY and TIP_BINDINGS is []. check-i18n assertion 2 accepts zero
// bindings only when no I18N entry carries a body, which is exactly this state.
//
// An ES module that EXPORTS ONLY. It must never self-execute: a bare top-level
// statement here throws out of module evaluation and takes every later
// initializer on the page with it (pattern_module_toplevel_init_tdz).
// check-i18n assertion 7 enforces it.
//
// SERVED ROOT IS Resources/ui, read from CMakeLists.txt before a byte was
// written here — NOT Source/ui/public, which this plugin does not have. The
// binary-data target O-Bells_UIResources carries no namespace argument, so it
// takes the default BinaryData namespace and works only because it is the only
// such target in this plugin; this file was added to that EXISTING SOURCES list
// rather than to a second target, which would collide on the BinaryData
// namespace (critical_dual_binary_data_namespace_collision).
//
// FOUR PLACES, ONE COMMIT: this file on disk, the SOURCES list in
// CMakeLists.txt, a getResource() branch in PluginEditor.cpp, and the import in
// the inline module in index.html. Miss one and the page 404s at runtime and
// presents as a dead panel with no other symptom (assertion 8).
//
// FILENAME: no hyphen. juce_add_binary_data STRIPS hyphens rather than
// converting them to underscores, so a second file named i18n-fr.js would have
// to be reached as the symbol i18nfr_js (critical_binary_data_strips_hyphens).
// One combined file for both languages sidesteps the question entirely.
//
// NO MARKUP. This table is data, never HTML. check-i18n assertion 9 rejects any
// innerHTML reference here and any string literal containing an opening angle
// bracket.
//
// ── WHERE THE COPY LIVES, AND WHY THE COUNT IS NOT THE EXTRACTOR'S ─────────
//
// scripts/i18n-extract.js skips js/tuning-panel.js BY FILENAME
// (i18n-extract.js:442), with no ownership test. That skip is correct for
// O-Wind, which consumes the MODULE file by reference from
// ${CMAKE_SOURCE_DIR}/modules/. It is wrong here: this plugin owns its copy —
// the header says "part of O-Bells", it is 279 lines diverged from
// modules/tuning/scala-tuning-engine/js/tuning-panel.js, and O-Bells has no
// dependencies.json listing the module, so /module-upgrade will not revert an
// edit to it. So the Tuning tab IS in scope and its ~34 strings are keyed here,
// on top of the 79 the extractor found in index.html.
//
// Half of that file IS reachable by the gates and half is not, which is worth
// stating precisely rather than repeating the dispatch's blanket claim that
// neither gate sees it:
//   - check-i18n assertions 12/13/15 DO scan js/tuning-panel.js — pageModules
//     is built from every top-level .js in the served js/ directory.
//   - assertion 10 does NOT, because it walks index.html's text nodes only.
//   - assertion 12's innerHTML arm reads only a literal sitting on the RHS of
//     the assignment, so the panel's `html += ...` accumulators (the interval
//     list, the matrix, the rotation table, the library list) are invisible to
//     it. They are keyed here regardless, and driven in the browser to prove it.
//
// ALL FRENCH IS MACHINE-DRAFTED AND FLAGGED `reviewed: false`. No native
// speaker has read it. `node scripts/check-i18n.js` prints the worklist.
//
// The tuning-panel and effects-chain French below is taken VERBATIM from
// O-IntonationPad v2.9.0, which localized the same panel and the same
// Chorus/Delay/EQ/Reverb chain. Two hand-copies of one panel disagreeing about
// the French would be a worse outcome than either translation alone.
// ============================================================================

export const LANGUAGES = ['en', 'fr'];

// EMPTY, and deliberately so. I18N holds hover-help — a title AND a body per
// key. This plugin has none. trLabel() falls back to I18N when a key is absent
// from LABELS, so the canon block is unchanged; there is simply nothing here to
// fall back to.
export const I18N = Object.freeze({});

export const LABELS = Object.freeze({

    // ── Header: preset browser ──────────────────────────────────────────────
    // The two arrows and the name display are NOT keyed: the arrows are glyphs
    // and the name display shows a preset name, which is the JSON filename
    // (D-02, and I18N_EXEMPT below).
    'label.presetSave':  { en: { t: 'Save' }, fr: { t: 'Enreg.', reviewed: false } },
    'label.presetLoad':  { en: { t: 'Load' }, fr: { t: 'Ouvrir', reviewed: false } },

    // ── Tab row ─────────────────────────────────────────────────────────────
    'label.tabInstrument': { en: { t: 'Instrument' }, fr: { t: 'Instrument', reviewed: false, sameAsEn: true } },
    'label.tabTuning':     { en: { t: 'Tuning' },     fr: { t: 'Gamme',      reviewed: false } },
    'label.tabEffects':    { en: { t: 'Effects' },    fr: { t: 'Effets',     reviewed: false } },

    // ── The CPU warning banner, SPLIT — and the readout moved to the FRONT ──
    // v4.1.5 wrote this as one .cpu-warning-text span holding two text nodes
    // with the live estimate element BETWEEN them: "Estimated bell decay: ~18s.
    // Long decays...". Keying that span whole would make applyLabel's
    // `el.textContent = s` delete the estimate — contract §5's split rule, and
    // the extractor flagged both halves as UNSURE for exactly this reason.
    //
    // Splitting it into prefix + readout + suffix was the first attempt and it
    // FAILED the geometry diff: the French prefix is 54.41px wider than the
    // English one, so the readout moved by exactly that, and no word choice can
    // land two translations within the gate's 0.5px tolerance. Contract §6
    // authors the copy around a constraint rather than engineering around it, so
    // the readout now LEADS — its x is the start of the text block and is
    // language-invariant — and everything after it is this one keyed span, which
    // the diff excludes because it is a label. The number keeps its own bold
    // .cpu-warning-estimate styling, which folding the whole banner into a single
    // composed {s} label would have thrown away.
    'label.cpuWarning': { en: { t: 'estimated bell decay. Long decays with polyphony may cause high CPU usage, distortion, or stuttering.' },
                          fr: { t: 'de décroissance estimée. Les décroissances longues avec polyphonie peuvent provoquer une charge CPU élevée, de la distorsion ou des coupures.', reviewed: false } },

    // ── Instrument tab: Synthesis ───────────────────────────────────────────
    'label.secSynthesis':       { en: { t: 'Synthesis' },           fr: { t: 'Synthèse', reviewed: false } },
    'label.damping':            { en: { t: 'Damping' },             fr: { t: 'Amortissement', reviewed: false } },
    'label.overtoneBrightness': { en: { t: 'Overtone Brightness' }, fr: { t: 'Brillance harmonique', reviewed: false } },
    'label.acousticBrightness': { en: { t: 'Acoustic Brightness' }, fr: { t: 'Brillance acoustique', reviewed: false } },
    'label.material':           { en: { t: 'Material' },            fr: { t: 'Matériau', reviewed: false } },
    'label.inharmonicity':      { en: { t: 'Inharmonicity' },       fr: { t: 'Inharmonicité', reviewed: false } },
    'label.airAbsorption':      { en: { t: 'Air Absorption' },      fr: { t: 'Absorption air', reviewed: false } },
    'label.airTime':            { en: { t: 'Air Time' },            fr: { t: 'Durée air', reviewed: false } },

    // "Bloom" is the bell-acoustics term for the swell that follows the strike.
    // "Éclosion" is the French rendering; it is NOT kept in English the way
    // "Shimmer" is, because "shimmer" is a shipped effect NAME across this suite
    // and "bloom" here is a described behaviour.
    'label.bloomSpeed':     { en: { t: 'Bloom Speed' },  fr: { t: 'Vitesse éclosion', reviewed: false } },
    'label.bloomAmount':    { en: { t: 'Bloom Amount' }, fr: { t: 'Dosage éclosion', reviewed: false } },
    'label.shimmer':        { en: { t: 'Shimmer' },      fr: { t: 'Shimmer', reviewed: false, sameAsEn: true } },
    'label.bloomFineToggle': { en: { t: 'Bloom Fine Controls (Override Mode)' },
                               fr: { t: 'Réglages fins d’éclosion (mode manuel)', reviewed: false } },
    'label.bloomFineHint':  { en: { t: 'Per-band control - main sliders disabled when active' },
                              fr: { t: 'Réglage par bande - curseurs principaux désactivés', reviewed: false } },
    'label.speedLow':       { en: { t: 'Speed Low' },   fr: { t: 'Vitesse grave', reviewed: false } },
    'label.speedMid':       { en: { t: 'Speed Mid' },   fr: { t: 'Vitesse médium', reviewed: false } },
    'label.speedHigh':      { en: { t: 'Speed High' },  fr: { t: 'Vitesse aigu', reviewed: false } },
    'label.amountLow':      { en: { t: 'Amount Low' },  fr: { t: 'Dosage grave', reviewed: false } },
    'label.amountMid':      { en: { t: 'Amount Mid' },  fr: { t: 'Dosage médium', reviewed: false } },
    'label.amountHigh':     { en: { t: 'Amount High' }, fr: { t: 'Dosage aigu', reviewed: false } },

    // ── Instrument tab: Ensemble ────────────────────────────────────────────
    'label.secEnsemble': { en: { t: 'Ensemble' }, fr: { t: 'Ensemble', reviewed: false, sameAsEn: true } },
    'label.unison':      { en: { t: 'Unison' },   fr: { t: 'Unisson',  reviewed: false } },
    'label.detune':      { en: { t: 'Detune' },   fr: { t: 'Désacc.',  reviewed: false } },
    // "Sub" and "Oct" are the octave-below and octave-above blend amounts. Both
    // are the same truncation in French (sub-octave, octave), so both carry
    // sameAsEn rather than a fabricated difference.
    'label.sub':         { en: { t: 'Sub' },      fr: { t: 'Sub',      reviewed: false, sameAsEn: true } },
    'label.oct':         { en: { t: 'Oct' },      fr: { t: 'Oct',      reviewed: false, sameAsEn: true } },
    'label.spread':      { en: { t: 'Spread' },   fr: { t: 'Étalement', reviewed: false } },

    // ── Instrument tab: Onsets ──────────────────────────────────────────────
    'label.secOnsets':    { en: { t: 'Onsets' },        fr: { t: 'Attaques', reviewed: false } },
    'label.strike':       { en: { t: 'Strike' },        fr: { t: 'Frappe', reviewed: false } },
    'label.mallet':       { en: { t: 'Mallet' },        fr: { t: 'Mailloche', reviewed: false } },
    'label.attackAmount': { en: { t: 'Attack Amount' }, fr: { t: 'Dosage attaque', reviewed: false } },
    'label.noise':        { en: { t: 'Noise' },         fr: { t: 'Bruit', reviewed: false } },
    'label.velocity':     { en: { t: 'Velocity' },      fr: { t: 'Vélocité', reviewed: false } },
    // The velocityCurve group is three buttons over ONE AudioParameterChoice
    // whose options are Linear / Exponential / Logarithmic. Only the first
    // caption is byte-identical to its option, so only that one is exempt under
    // D-01 arm 1; "Exp" and "Log" are the plugin's own abbreviations and are
    // plain captions that localize. French takes a point on a truncated
    // abbreviation, which is what makes these genuinely different strings and
    // not a sameAsEn pair.
    'label.velExp':       { en: { t: 'Exp' }, fr: { t: 'Exp.', reviewed: false } },
    'label.velLog':       { en: { t: 'Log' }, fr: { t: 'Log.', reviewed: false } },

    // ── Instrument tab: Advanced ────────────────────────────────────────────
    'label.secAdvanced': { en: { t: 'Advanced' },     fr: { t: 'Avancé', reviewed: false } },
    'label.partialTune': { en: { t: 'Partial Tune' }, fr: { t: 'Accord partiels', reviewed: false } },
    'label.pitchEnv':    { en: { t: 'Pitch Env' },    fr: { t: 'Env. hauteur', reviewed: false } },
    // The English is ALREADY abbreviated to fit a quarter-width cell, so the
    // French is held to the same budget rather than spelled out.
    'label.pEnvTime':    { en: { t: 'P.Env Time' },   fr: { t: 'Durée env.', reviewed: false } },
    'label.nonlinear':   { en: { t: 'Nonlinear' },    fr: { t: 'Non linéaire', reviewed: false } },

    // ── Instrument tab: Multi-Stage Envelope ────────────────────────────────
    'label.secEnvelope':  { en: { t: 'Multi-Stage Envelope' }, fr: { t: 'Enveloppe multi-étages', reviewed: false } },
    'label.envelopeHint': { en: { t: 'Controls how different frequencies decay over time' },
                            fr: { t: 'Règle la décroissance des fréquences dans le temps', reviewed: false } },
    'label.strikeTime':   { en: { t: 'Strike Time' }, fr: { t: 'Durée frappe', reviewed: false } },
    'label.brilliance':   { en: { t: 'Brilliance' },  fr: { t: 'Brillance', reviewed: false } },
    'label.bodyTime':     { en: { t: 'Body Time' },   fr: { t: 'Durée corps', reviewed: false } },
    'label.humSustain':   { en: { t: 'Hum Sustain' }, fr: { t: 'Tenue bourdon', reviewed: false } },

    // ── Instrument tab: Filter / Performance / Output ───────────────────────
    'label.secFilter':      { en: { t: 'Filter' },    fr: { t: 'Filtre', reviewed: false } },
    'label.lpFilter':       { en: { t: 'LP Filter' }, fr: { t: 'Filtre PB', reviewed: false } },
    'label.cutoff':         { en: { t: 'Cutoff' },    fr: { t: 'Coupure', reviewed: false } },
    'label.secPerformance': { en: { t: 'Performance' }, fr: { t: 'Performance', reviewed: false, sameAsEn: true } },
    'label.highFidelity':   { en: { t: 'High Fidelity' }, fr: { t: 'Haute fidélité', reviewed: false } },
    'label.hiFiNote':       { en: { t: 'Disables voice culling for maximum sustain fidelity. May cause CPU overload with long-decay presets and dense polyphony.' },
                              fr: { t: 'Désactive l’élagage des voix pour une tenue maximale. Peut surcharger le processeur avec des préréglages à longue décroissance et une polyphonie dense.', reviewed: false } },
    'label.secOutput':      { en: { t: 'Output' },   fr: { t: 'Sortie', reviewed: false } },
    'label.humanize':       { en: { t: 'Humanize' }, fr: { t: 'Humanisation', reviewed: false } },
    'label.level':          { en: { t: 'Level' },    fr: { t: 'Niveau', reviewed: false } },

    // ── Effects tab ─────────────────────────────────────────────────────────
    // Same chain, same French as O-IntonationPad v2.9.0.
    'label.fxChorus': { en: { t: 'Chorus' }, fr: { t: 'Chorus',  reviewed: false, sameAsEn: true } },
    'label.fxDelay':  { en: { t: 'Delay' },  fr: { t: 'Délai',   reviewed: false } },
    'label.fxEq':     { en: { t: 'EQ' },     fr: { t: 'EQ',      reviewed: false, sameAsEn: true } },
    'label.fxReverb': { en: { t: 'Reverb' }, fr: { t: 'Réverbe', reviewed: false } },
    'label.rate':     { en: { t: 'Rate' },     fr: { t: 'Vitesse', reviewed: false } },
    'label.depth':    { en: { t: 'Depth' },    fr: { t: 'Prof.',   reviewed: false } },
    'label.mix':      { en: { t: 'Mix' },      fr: { t: 'Dosage',  reviewed: false } },
    'label.time':     { en: { t: 'Time' },     fr: { t: 'Durée',   reviewed: false } },
    'label.feedback': { en: { t: 'Feedback' }, fr: { t: 'Réinj.',  reviewed: false } },
    'label.low':      { en: { t: 'Low' },      fr: { t: 'Grave',   reviewed: false } },
    'label.mid':      { en: { t: 'Mid' },      fr: { t: 'Médium',  reviewed: false } },
    'label.midFreq':  { en: { t: 'Mid Freq' }, fr: { t: 'Fréq. méd.', reviewed: false } },
    'label.high':     { en: { t: 'High' },     fr: { t: 'Aigu',    reviewed: false } },
    'label.size':     { en: { t: 'Size' },     fr: { t: 'Taille',  reviewed: false } },
    'label.damp':     { en: { t: 'Damp' },     fr: { t: 'Amortis.', reviewed: false } },
    'label.preDly':   { en: { t: 'Pre-dly' },  fr: { t: 'Pré-dél.', reviewed: false } },
    'label.mod':      { en: { t: 'Mod' },      fr: { t: 'Mod',     reviewed: false, sameAsEn: true } },
    // Worn by the delay-mode dropdown caption AND by the rotation table's first
    // column header in the tuning panel. One string, one key, two anchors.
    'label.mode':     { en: { t: 'Mode' },     fr: { t: 'Mode',    reviewed: false, sameAsEn: true } },
    // The two faces of the four FX bypass buttons. Written ONLY by setLabel,
    // from an if/else and never a ternary — check-i18n assertion 13 rejects a
    // conditional inside a setLabel argument (contract §6).
    'ui.on':          { en: { t: 'On' },  fr: { t: 'Marche', reviewed: false } },
    'ui.off':         { en: { t: 'Off' }, fr: { t: 'Arrêt',  reviewed: false } },

    // ── Footer ──────────────────────────────────────────────────────────────
    'label.gain':         { en: { t: 'Gain' }, fr: { t: 'Gain', reviewed: false, sameAsEn: true } },
    // The key NAMES stay: Z-M and Q-P are physical QWERTY positions the page
    // maps by e.key, not words. Only the sentence around them moves.
    'label.keyboardHelp': { en: { t: 'Click or use Z-M, Q-P keys' },
                            fr: { t: 'Cliquer ou utiliser les touches Z-M, Q-P', reviewed: false } },

    // ── The settings popover (new in v4.2.0) ────────────────────────────────
    'label.language': { en: { t: 'Language' }, fr: { t: 'Langue', reviewed: false } },

    // ── Tuning panel (js/tuning-panel.js) ───────────────────────────────────
    // PARAMETERISED entries, written through data-i18n-vars. Contract §6: the
    // inflection is authored AROUND, not engineered. English pluralizes zero as
    // "0 notes" and French as "0 note", so the noun moved in front of the
    // number and the count now stands alone — correct at 0, 1 and n in both
    // languages with no plural engine anywhere.
    'label.intervalsHeader': { en: { t: 'Intervals: {n}' }, fr: { t: 'Intervalles : {n}', reviewed: false } },
    'label.noteCount':       { en: { t: 'Notes: {n}' },     fr: { t: 'Notes : {n}',       reviewed: false } },
    'label.tonic':           { en: { t: 'Tonic' },          fr: { t: 'Tonique', reviewed: false } },
    'label.scaleIntervals':  { en: { t: 'Scale Intervals' }, fr: { t: 'Intervalles de la gamme', reviewed: false } },
    'label.vizCircle':       { en: { t: 'Circle' },    fr: { t: 'Cercle',   reviewed: false } },
    'label.vizPolar':        { en: { t: 'Polar' },     fr: { t: 'Polaire',  reviewed: false } },
    'label.vizMatrix':       { en: { t: 'Matrix' },    fr: { t: 'Matrice',  reviewed: false } },
    'label.vizTrueKeys':     { en: { t: 'True Keys' }, fr: { t: 'Touches',  reviewed: false } },
    'label.vizRotation':     { en: { t: 'Rotation' },  fr: { t: 'Rotation', reviewed: false, sameAsEn: true } },
    'label.tkHint':          { en: { t: 'Hold 2+ notes to see intervals' },
                               fr: { t: 'Tenir 2 notes ou plus pour voir les intervalles', reviewed: false } },
    'label.totalSpan':       { en: { t: 'Total span' }, fr: { t: 'Écart total', reviewed: false } },
    'label.tuningLibrary':   { en: { t: 'Tuning Library' }, fr: { t: 'Bibliothèque', reviewed: false } },
    // The library filter is a PLAIN select over the strings all / Historical /
    // ... — it is not an AudioParameterChoice, no host ever shows these six
    // strings, and translating them cannot make the page and an automation lane
    // disagree. That is the discriminator; the choice-parameter option strings
    // are exempt below for exactly the opposite reason. The option VALUES are
    // untouched, so filtering still matches the categories the C++ side reports.
    'label.catAllCategories':  { en: { t: 'All Categories' },  fr: { t: 'Toutes catégories', reviewed: false } },
    'label.catHistorical':     { en: { t: 'Historical' },      fr: { t: 'Historiques',   reviewed: false } },
    'label.catJustIntonation': { en: { t: 'Just Intonation' }, fr: { t: 'Intonation juste', reviewed: false } },
    'label.catEqualDivisions': { en: { t: 'Equal Divisions' }, fr: { t: 'Divisions égales', reviewed: false } },
    'label.catNonOctave':      { en: { t: 'Non-Octave' },      fr: { t: 'Non octaviantes', reviewed: false } },
    'label.catWorld':          { en: { t: 'World' },           fr: { t: 'Du monde',      reviewed: false } },
    // "A4" is a pitch identifier and stays; only the abbreviation "REF" moves.
    'label.a4Ref':   { en: { t: 'A4 REF' },  fr: { t: 'RÉF. A4',   reviewed: false } },
    'label.stretch': { en: { t: 'Stretch' }, fr: { t: 'Étirement', reviewed: false } },
    // The four file buttons keep their EXTENSIONS, which are file-format
    // identifiers, and translate only the verb.
    'label.loadScl':    { en: { t: 'Load .SCL' },   fr: { t: 'Ouvrir .SCL', reviewed: false } },
    'label.loadKbm':    { en: { t: 'Load .KBM' },   fr: { t: 'Ouvrir .KBM', reviewed: false } },
    'label.saveScl':    { en: { t: 'Save .SCL' },   fr: { t: 'Enreg. .SCL', reviewed: false } },
    'label.saveKbm':    { en: { t: 'Save .KBM' },   fr: { t: 'Enreg. .KBM', reviewed: false } },
    'label.exportHtml': { en: { t: 'Export HTML' }, fr: { t: 'Exporter HTML', reviewed: false } },
    'label.generateScale': { en: { t: 'Generate Scale' }, fr: { t: 'Générer une gamme', reviewed: false } },
    'label.genEdo':      { en: { t: 'EDO (Equal Division)' }, fr: { t: 'EDO (division égale)', reviewed: false } },
    'label.genHarmonic': { en: { t: 'Harmonic Series' },      fr: { t: 'Série harmonique',     reviewed: false } },
    'label.genRank2':    { en: { t: 'Rank-2 Temperament' },   fr: { t: 'Tempérament de rang 2', reviewed: false } },
    'label.genDivisions': { en: { t: 'Divisions' },     fr: { t: 'Divisions', reviewed: false, sameAsEn: true } },
    'label.genPeriod':   { en: { t: 'Period (c)' },     fr: { t: 'Période (c)', reviewed: false } },
    'label.genStart':    { en: { t: 'Start Harmonic' }, fr: { t: 'Harmonique de départ', reviewed: false } },
    'label.genEnd':      { en: { t: 'End Harmonic' },   fr: { t: 'Harmonique de fin',    reviewed: false } },
    'label.genGenerator': { en: { t: 'Generator (c)' }, fr: { t: 'Générateur (c)', reviewed: false } },
    'label.genCount':    { en: { t: 'Notes' },          fr: { t: 'Notes', reviewed: false, sameAsEn: true } },
    'label.generate':    { en: { t: 'Generate' },       fr: { t: 'Générer', reviewed: false } },

    // ── Accessible names ────────────────────────────────────────────────────
    // alt.snail was a native alt=; aria.doubleClickEdit was the ONE native
    // title= this plugin had, written from JS onto all sixteen effects-knob
    // readouts (`valueDisplay.title = 'Double-click to edit'`). Contract §4
    // DELETES a native title= rather than localizing it, and where the title was
    // an element's only help its text becomes the accessible name. NO NEW PROSE
    // WAS INVENTED: both strings are the plugin's own v4.1.5 wording.
    'alt.snail':            { en: { t: 'Snail' }, fr: { t: 'Escargot', reviewed: false } },
    'aria.settings':        { en: { t: 'Settings' }, fr: { t: 'Réglages', reviewed: false } },
    'aria.langSelect':      { en: { t: 'Interface language' }, fr: { t: 'Langue de l’interface', reviewed: false } },
    'aria.doubleClickEdit': { en: { t: 'Double-click to edit' }, fr: { t: 'Double-cliquer pour modifier', reviewed: false } },
});

// ============================================================================
// I18N_EXEMPT — reasoned exclusions, never silence
// ============================================================================
//
// Every visible string the coverage scan finds must be a [data-i18n] element, a
// setLabel() call, or an entry HERE WITH A REASON. A bare skip list would let a
// missed label hide as a deliberate one.
//
// An entry is [text, reason] or [text, reason, SCOPE]. An exemption is matched
// by TEXT, so an unscoped one silences EVERY node with that string. A scope is
// REQUIRED where the same string is also keyed on this page — the one state in
// which the gate cannot tell a deliberate skip from a forgotten label
// (assertion 14). None of the strings below is keyed on this page; "Default"
// carries a scope anyway because it is the one string a future caption could
// plausibly collide with.
//
// ── NOT LISTED HERE, AND WHY ───────────────────────────────────────────────
//
// Runtime DATA has no fixed literal to exempt, so listing it would be an inert
// entry that reads like a rule. Named instead:
//   - the preset-dropdown CATEGORY headers and preset ITEM names
//     (`header.textContent = category`, `item.textContent = preset`) come from
//     getPresetListWithCategories() on the C++ side. The name IS the JSON
//     filename and the category IS its folder, so both are D-02.
//   - the tuning library item names and the #scale-name-display value come from
//     getEmbeddedTuningList() / getTuningName(). Same rule.
//   - the scale names the generator BUILDS (`19-EDO`, `Harmonics 8-16`,
//     `Rank-2 (696.6c, 12 notes)`) are passed to applyGeneratedScale() and are
//     written into .scl files and the exported HTML. Localizing them would put
//     French inside a Scala file.
//
// LETTER PITCH NOTATION IS DELIBERATELY NOT LOCALIZED, across the whole page:
// the footer keyboard's C/D/E/F/G/A/B and C3/C4, the tuning panel's twelve
// noteNames, and the interval-quality abbreviations m2/M2/…/P8 that sit beside
// them in True Keys. French solfège would be Do/Ré/Mi and 2m/2M/…/8J, and the
// C++ TuningEngine, the .scl and .kbm file formats and the exported HTML all
// speak the letter system. Translating the page alone would desync it from the
// files it reads and writes; translating both is a data-format change, not a
// caption change. Only "TT" survives the readout classifier as prose, so it is
// the only one of the twelve that needs an entry.
// ============================================================================

export const I18N_EXEMPT = [
    ['Ouaricon Bells',
     'the product name in the h1 — a product name is never translated, and this is the display form of the plugin’s registered PRODUCT_NAME in CMakeLists.txt'],
    ['Ouaricon Audio',
     'the company name in the footer — a company name is never translated'],

    // #preset-name-display shows the loaded preset. The name IS the JSON
    // filename (OuariconPresetManager.h), so translating it breaks recall.
    // "Default" is both the authored markup fallback and the `name || 'Default'`
    // fallback in updatePresetDisplay().
    ['Default', 'the preset-name display’s fallback — exempt under D-02, because a preset name IS the JSON filename',
                '#preset-name-display'],

    // ── AudioParameterChoice option strings (D-01 arm 1) ────────────────────
    // Byte-identical to the parameter's own choice list, which the host shows in
    // its automation lane and which some hosts cache. Translating the option
    // text would make the page and the automation lane disagree about the same
    // parameter. The library filter and the generator type ARE localized above,
    // and the difference is exactly this: those two are plain selects no host
    // ever sees.
    ['Click',  'strikeNoiseChar choice-parameter value, byte-identical — host automation contract (D-01 arm 1)'],
    ['Thud',   'strikeNoiseChar choice-parameter value, byte-identical — host automation contract (D-01 arm 1)'],
    ['Ping',   'strikeNoiseChar choice-parameter value, byte-identical — host automation contract (D-01 arm 1)'],
    ['Linear', 'velocityCurve choice-parameter value, byte-identical — host automation contract (D-01 arm 1). Its two siblings Exp/Log are NOT byte-identical to "Exponential"/"Logarithmic" and are keyed above'],
    ['Normal',   'delayMode choice-parameter value, byte-identical — host automation contract (D-01 arm 1)'],
    ['PingPong', 'delayMode choice-parameter value, byte-identical — host automation contract (D-01 arm 1)'],

    // ── material: exempt on TWO arms at once ────────────────────────────────
    // Arm 1: byte-identical to the material AudioParameterChoice options.
    // Arm 3: the receiving element is `.param-value[data-value="material"]`, a
    // READOUT node that holds a percentage for every other slider on the page.
    // Keying one would make the element enter and leave the sweep as the knob
    // turns — the O-Marimba case, here with arm 1 agreeing.
    ['Bronze',    'material choice-parameter value written into a .param-value READOUT node — D-01 arms 1 and 3'],
    ['Brass',     'material choice-parameter value written into a .param-value READOUT node — D-01 arms 1 and 3'],
    ['Steel',     'material choice-parameter value written into a .param-value READOUT node — D-01 arms 1 and 3'],
    ['Aluminum',  'material choice-parameter value written into a .param-value READOUT node — D-01 arms 1 and 3'],
    ['Cast Iron', 'material choice-parameter value written into a .param-value READOUT node — D-01 arms 1 and 3'],

    // ── Identifiers, not words ──────────────────────────────────────────────
    ['12-TET Standard',
     'a tuning IDENTIFIER, not a caption — it is the name the tuning engine reports for the loaded scale and is matched against Scala file names'],
    ['TT',
     'the tritone’s interval-quality abbreviation in the True Keys view, beside letter pitch names the C++ engine and the .scl/.kbm formats also use — see the note above I18N_EXEMPT. The other eleven (m2, M2, m3, M3, P4, P5, m6, M6, m7, M7, P8) classify as READOUT and need no entry'],
    ['English',  'endonym — a language name is never translated'],
    ['Français', 'endonym — a language name is never translated'],
];

// This plugin ships no hover-help, so nothing is bound. check-i18n assertion 2
// accepts an empty list only when no I18N entry carries a body — which is the
// state above, not an oversight. Authoring the copy is Stage M.
export const TIP_BINDINGS = [];

export function tr(key, lang, vars) {
    const entry = I18N[key];
    if (!entry) { console.warn(`i18n: missing key ${key}`); return { t: key, b: '' }; }
    const s = entry[lang] || entry.en;

    // A var VALUE that is itself an I18N key resolves to that key's localized
    // title; anything else is used literally. This plugin needs neither arm
    // today, but the canon is one shape across all 43 plugins and this function
    // is not trimmed per plugin.
    const resolve = (v) => {
        const nested = I18N[v];
        return nested ? String((nested[lang] || nested.en).t) : String(v);
    };

    const sub = (v) => vars
        ? String(v).replace(/\{(\w+)\}/g, (m, n) => (n in vars ? resolve(vars[n]) : m))
        : String(v);

    return { t: sub(s.t), b: sub(s.b) };
}
