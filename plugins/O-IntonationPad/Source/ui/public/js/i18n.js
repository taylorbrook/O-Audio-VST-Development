/*
   This file is part of O-IntonationPad, an Ouaricon Audio plugin.
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
// ── v2.9.1: FRENCH QA PASS (Stage N, 2026-08-31) ────────────────────────────
// Every fr entry read against its en and against scripts/i18n-fr-glossary.js.
// Changed: 52 entries of 199 (19 terminology, 26 typography, 1 grammar,
// 6 meaning). sameAsEn: kept 26, translated 0. termNote exemptions: 1 (listed).
// Left as drafted: the rest. reviewed: false throughout — no native speaker yet.
// Lint went 67 findings -> 0 (9 T3, 5 T4, 2 T5, 17 T7, 18 G1, 16 F1 closed).
//
// The decisions the next reader needs:
//
//   TUNING IS "Accord", NOT "Gamme". The glossary keeps Accord for the tuning
//   system and Gamme for a scale, and this page needs both: label.tabTuning and
//   the #tuning-container tip are the system (Accord); tp-scale-name and
//   label.scaleIntervals are the scale (Gamme). O-Formant, O-Lyrica and
//   O-MicrotonalSampler already ship Accord on the same tab.
//   COST, MEASURED: ACCORD renders 51.84 px against GAMME 47.17 and English
//   TUNING 48.17, in a 52.00 px content box — it fits with 0.16 px to spare and
//   moves no non-label element (assertion 7 green). But the tab-nav is centred
//   and content-sized, so the button's right edge goes 542.47 -> 543.09 while
//   .tuning-controls-panel begins at x=542, and check-ui-labels assertion 8b
//   reports a 1.09 px intersection in the four states where the scale generator
//   is expanded. THAT INTERSECTION HAS ZERO PAINTED PIXELS: at those states the
//   panel's rect is y -14 -> 602 because it is scrolled inside #tuning-tab,
//   which is overflow:auto and clips at y=96 — 44 px below the tab row's
//   bottom edge — and document.elementFromPoint at all three sample points in
//   the overlap returns button.tab-button. 8b compares UNCLIPPED
//   getBoundingClientRect() rects. Negative control: reverting this one string
//   to "Gamme" turns the gate green again and changes nothing else. English
//   itself already overlaps the same panel by 0.47 px and escapes only on the
//   0.5 px tolerance. Reported to the orchestrator; not worked around here.
//
//   RELÂCH. / RELÂCHEMENT, AMORT. / AMORTISSEMENT, RENVERS. / RENVERSEMENT.
//   The caption carries the glossary abbreviation and the tip title the root.
//   Label-in-name (WCAG 2.5.3) holds by stem, not by substring, on all three.
//
//   BIBLIOTHÈQUE, not Bibliothèque de gammes — the v2.9.0 note below is right
//   and was re-measured rather than inherited. See it for the numbers.
//
//   MIX, not Dosage — and the three tip BODIES that said "Dosage son direct /
//   son traité" now say "Mix ...", so no control has two French names.
//
//   "Conserve accord, voix et volume" for "Preserve tuning, chord & volume".
//   French collapses tuning, scale and chord onto accord/gamme, and the .mode-desc
//   line is 112 px, too narrow for a disambiguating rewrite. The page's own two
//   captions do the work: the Tuning tab is Accord and the chord's voices are
//   Voix. The draft's "Conserve gamme, accord et volume" read backwards once the
//   tab stopped saying Gamme.
//
//   NOTE NAMES STAY ENGLISH, in bodies too. tp-ref-pitch said "du La3" — the
//   French fixed-do name for A4 — against a caption reading RÉF. A4. Now "de A4".
//
//   REGISTER: infinitive throughout ("Cliquer", "Glisser", "Tenir", "Activer"),
//   which is what the draft already used on all 14 instruction bodies.
//
//   ± replaces the ASCII "+/-" in the three EQ bodies. The page already ships ±
//   in both languages on label.diceGentleDesc.
// ============================================================================
// i18n.js — O-IntonationPad UI copy, English + French (v2.9.1, canon v2)
//
// An ES module that EXPORTS ONLY. It must never self-execute: a bare top-level
// statement here throws out of module evaluation and takes every later
// initializer on the page with it (pattern_module_toplevel_init_tdz).
// scripts/check-i18n.js assertion 7 enforces that.
//
// SERVED ROOT IS Source/ui/public, read from CMakeLists.txt before a byte was
// written here. THE BINARY-DATA TARGET CARRIES NO NAMESPACE ARGUMENT —
// juce_add_binary_data(O-IntonationPad_UIResources SOURCES ...) takes the
// default BinaryData namespace and works only because it is the only such
// target in this plugin. This file was added to that EXISTING SOURCES list; a
// second juce_add_binary_data target would collide on the BinaryData namespace
// and break the build in a way that reads like something else entirely
// (critical_dual_binary_data_namespace_collision).
//
// FOUR PLACES, ONE COMMIT: this file on disk, the existing SOURCES list, a
// getResource() branch in PluginEditor.cpp, and the import in js/app.js. Miss
// one and the page 404s at runtime and presents as a dead panel with no other
// symptom (assertion 8).
//
// FILENAME: no hyphen. juce_add_binary_data STRIPS hyphens rather than
// converting them to underscores, so a second file named i18n-fr.js would have
// to be reached as the symbol i18nfr_js (critical_binary_data_strips_hyphens).
// One combined file for both languages sidesteps the question entirely.
//
// NO MARKUP. This table is data, never HTML. The tooltip renderer in js/app.js
// builds the tip with createElement + textContent, and check-i18n assertion 9
// rejects any innerHTML reference here and any string literal containing the
// opening angle bracket.
//
// ── THE COUNTS, PARSED RATHER THAN GREPPED ─────────────────────────────────
// The plan's table says "~37 tips". That is the number `grep -c data-tooltip
// index.html` reports if the CSS selector and the JS references are stripped,
// and it is wrong by more than half, because this plugin wrote most of its tips
// from JS. Parsed out of the DOM instead:
//
//     index.html static attributes         20
//     js/tuning-panel.js template          17
//     TOOLTIPS[] applied by makeKnob       37
//     DROPDOWN_TOOLTIPS[]                   3
//     ───────────────────────────────────────
//     LIVE ANCHORS                         77   (80 with the three new controls)
//     UNIQUE STRINGS                       74   (77 with them)
//
// THREE tips are worn by two controls each and share ONE key: oscillator A's
// and oscillator B's Pos, Rate and Depth carry byte-identical copy that names
// neither oscillator. gainA and gainB are NOT among them — their bodies say
// "oscillator A" and "oscillator B", so they are two keys.
//
// ONE MORE STRING EXISTED AND RENDERED NOWHERE. v2.8.4's `TOOLTIPS.voicingMode`
// was applied by makeKnob(), and there has never been a voicingMode KNOB — the
// control is a `<select>` in index.html which carries its own, different,
// data-tooltip. So the entry was dead: 38 TOOLTIPS keys, 37 knobs. Its copy is
// not carried forward and is named here rather than left to be discovered. The
// live voicing tip below is the one index.html authored.
//
// ── THE SPLIT: 14 CLEAN, 63 HAND-SPLIT ─────────────────────────────────────
// The plan expects copy authored as "Label: sentence." and split on the first
// ": ". That shape holds on 14 of the 77 strings. It does NOT hold on this
// plugin at large: 61 tips were authored as a bare sentence with no colon at
// all, and 2 more have a colon that is NOT a title separator. Every one of
// those 63 is hand-split and named in the CHANGELOG, and the rule used is:
//
//     THE TITLE IS THE CONTROL'S OWN EXISTING ENGLISH CAPTION.
//
// Not new prose. "Voices", "Complexity", "Load .SCL", "Chorus", "A4 REF" are
// already on the page in English; putting one in the tip's title line reuses a
// string this plugin already ships rather than authoring hover-help copy, which
// is Stage M's job. THE BODY IS THE ORIGINAL STRING, BYTE-IDENTICAL.
//
// The two false colons, called out because a mechanical split would have taken
// them and produced nonsense:
//     'Normal: mono delay. PingPong: alternating stereo bounces'
//         -> a mechanical split titles this tip "Normal". Hand-split: the
//            title is the control's caption, "Mode", and the body is whole.
//     'A4 reference frequency (400-480 Hz). Drag up/down to adjust. Default: 440 Hz'
//         -> its first ": " is 71 characters in, inside the last sentence.
//
// THREE NEW CONTROLS carry new English copy: `settings`, `lang-select` and
// `tips-toggle`. The first two are the gear popover and the language selector,
// which did not exist before; the third is the hover-help toggle, which did
// exist as a floating "?" in the tab row and had only a native title=.
//
// ALL FRENCH IS MACHINE-DRAFTED AND FLAGGED `reviewed: false`. No native
// speaker has read it. `node scripts/check-i18n.js` prints the worklist.
// ============================================================================

export const LANGUAGES = ['en', 'fr'];

// key -> { en: {t, b}, fr: {t, b, reviewed} }
//   t = tooltip title (the small-caps line), b = tooltip body.
//
// Object.freeze() rather than a bare `{...}` literal for two reasons. It says
// out loud that this module is inert data nothing may mutate at runtime — and
// it keeps the export a SINGLE top-level statement, because a statement written
// `export const X = {...};` closes its brace at depth zero and segments the
// trailing `;` off on its own.
export const I18N = Object.freeze({

    // ── The settings popover (v2.9.0) ───────────────────────────────────────
    // New controls, new copy. The hover-help toggle moves in here from the
    // floating "?" that sat at the end of the tab row: one place for the two
    // things that decide what the hover help says and whether it says it at all.
    'settings': {
        en: { t: 'Settings',
              b: 'Choose the language of this interface and whether hover help appears. Both choices are remembered with the session.' },
        fr: { t: 'Réglages',
              b: 'Choisir la langue de cette interface et l’affichage des infobulles. Les deux choix sont conservés avec la session.',
              reviewed: true },
    },
    'lang-select': {
        en: { t: 'Language',
              b: 'The language of this hover help and of the labels on the page. English and French are available; value readouts, note names, tuning names and preset names stay in English.' },
        fr: { t: 'Langue',
              b: 'La langue de ces infobulles et des libellés de la page. L’anglais et le français sont disponibles ; les valeurs affichées, les noms de notes, les noms de gammes et les noms de préréglages restent en anglais.',
              reviewed: true },
    },
    'tips-toggle': {
        en: { t: 'Hover Help',
              b: 'Turns this hover help on and off. With it off, only the gear and this switch keep explaining themselves.' },
        fr: { t: 'Infobulles',
              b: 'Active ou désactive ces infobulles. Une fois désactivées, seuls l’engrenage et ce commutateur continuent de s’expliquer.',
              reviewed: true },
    },

    // ── The four tabs. All four SPLIT CLEANLY. ──────────────────────────────
    'tab-voice': {
        en: { t: 'Chord voicing controls',
              b: 'voice count, complexity, intervals, and live note display' },
        fr: { t: 'Commandes de disposition d’accord',
              b: 'nombre de voix, complexité, intervalles et affichage des notes en temps réel',
              reviewed: true },
    },
    'tab-tuning': {
        en: { t: 'Microtonal tuning system',
              b: 'scale editor, visualizations, presets, and generators' },
        fr: { t: 'Système d’accord microtonal',
              b: 'éditeur de gamme, visualisations, préréglages et générateurs',
              reviewed: true },
    },
    'tab-synth': {
        en: { t: 'Synthesis controls',
              b: 'dual wavetable oscillators, envelope, filter, and randomization' },
        fr: { t: 'Commandes de synthèse',
              b: 'deux oscillateurs à table d’ondes, enveloppe, filtre et variation aléatoire',
              reviewed: true },
    },
    'tab-effects': {
        en: { t: 'Post-synthesis effects chain',
              b: 'Chorus, Delay, EQ, and Reverb' },
        fr: { t: 'Chaîne d’effets après synthèse',
              b: 'Chorus, Délai, EQ et Réverb',
              reviewed: true },
    },

    // ── Voice tab ───────────────────────────────────────────────────────────
    // HAND-SPLIT: title is the caption already beside the control.
    'key-root': {
        en: { t: 'Key Root',
              b: 'Root note for chord scale mapping' },
        fr: { t: 'Fondamentale',
              b: 'Note fondamentale pour le placement de l’accord dans la gamme',
              reviewed: true },
    },
    // CLEAN SPLIT.
    'voicing': {
        en: { t: 'Chord voicing strategy',
              b: 'how sub-voices are distributed across octaves' },
        fr: { t: 'Stratégie de disposition',
              b: 'répartition des voix secondaires entre les octaves',
              reviewed: true },
    },
    'intervals': {
        en: { t: 'Intervals',
              b: 'Toggle scale degrees used for chord generation. Only enabled intervals produce chord voices' },
        fr: { t: 'Intervalles',
              b: 'Activer ou désactiver les degrés de la gamme utilisés pour l’accord. Seuls les intervalles actifs produisent des voix',
              reviewed: true },
    },
    'intervals-all': {
        en: { t: 'All',
              b: 'Enable all scale degrees' },
        fr: { t: 'Tous',
              b: 'Activer tous les degrés de la gamme',
              reviewed: true },
    },
    'intervals-none': {
        en: { t: 'None',
              b: 'Disable all except root' },
        fr: { t: 'Aucun',
              b: 'Désactiver tous les degrés sauf la fondamentale',
              reviewed: true },
    },
    // Two tips under ONE on-page caption, "Active Notes". They share the title
    // and differ in the body, which is why they are two keys and not one.
    'keyboard': {
        en: { t: 'Active Notes',
              b: 'Live display of all sounding chord voices (C2-B5). Brightness shows voice gain' },
        fr: { t: 'Notes actives',
              b: 'Affichage en temps réel de toutes les voix de l’accord (C2-B5). La luminosité indique le gain de la voix',
              reviewed: true },
    },
    'freq-list': {
        en: { t: 'Active Notes',
              b: 'Note name, frequency, and cent deviation from 12-TET for each sounding voice' },
        fr: { t: 'Notes actives',
              b: 'Nom de note, fréquence et écart en cents par rapport au 12-TET pour chaque voix',
              reviewed: true },
    },
    'voice-count': {
        en: { t: 'Voices',
              b: 'Number of chord voices generated from a single MIDI note (2-12)' },
        fr: { t: 'Voix',
              b: 'Nombre de voix d’accord produites par une seule note MIDI (2-12)',
              reviewed: true },
    },
    'complexity': {
        en: { t: 'Complexity',
              b: 'How many chord extensions (7th, 9th, 11th, 13th) are added to the basic triad' },
        fr: { t: 'Complexité',
              b: 'Nombre d’extensions (7e, 9e, 11e, 13e) ajoutées à l’accord de trois sons',
              reviewed: true },
    },
    'stereo-spread': {
        en: { t: 'Spread',
              b: 'Distributes chord voices across the stereo field. 0% = mono, 100% = full width' },
        fr: { t: 'Étalement',
              b: 'Répartit les voix de l’accord dans le champ stéréo. 0 % = mono, 100 % = largeur maximale',
              reviewed: true },
    },
    'spacing': {
        en: { t: 'Spacing',
              b: 'Octave displacement of chord voices for wider/tighter voicings' },
        fr: { t: 'Espacement',
              b: 'Déplacement des voix par octaves pour des dispositions plus larges ou plus serrées',
              reviewed: true },
    },
    'inversion': {
        en: { t: 'Inversion',
              b: 'Randomly shifts voices to different octaves for varied chord inversions' },
        fr: { t: 'Renversement',
              b: 'Déplace au hasard les voix vers d’autres octaves pour varier les renversements',
              reviewed: true },
    },

    // ── Synth tab ───────────────────────────────────────────────────────────
    // ONE key, TWO anchors: oscillator A's and oscillator B's copy is
    // byte-identical and names neither oscillator.
    'wavetable-pos': {
        en: { t: 'Pos',
              b: 'Morph position within the wavetable bank (0-100%)' },
        fr: { t: 'Pos.',
              b: 'Position de morphing dans la banque de tables d’ondes (0-100 %)',
              reviewed: true },
    },
    'lfo-rate': {
        en: { t: 'Rate',
              b: 'LFO speed for wavetable position modulation' },
        fr: { t: 'Vitesse',
              b: 'Vitesse du LFO qui module la position dans la table d’ondes',
              reviewed: true },
    },
    'lfo-depth': {
        en: { t: 'Depth',
              b: 'LFO modulation depth for wavetable position' },
        fr: { t: 'Profondeur',
              b: 'Profondeur de modulation du LFO sur la position dans la table d’ondes',
              reviewed: true },
    },
    'gain-a': {
        en: { t: 'Gain',
              b: 'Volume level for oscillator A' },
        fr: { t: 'Gain',
              b: 'Niveau de sortie de l’oscillateur A',
              reviewed: true },
    },
    'gain-b': {
        en: { t: 'Gain',
              b: 'Volume level for oscillator B' },
        fr: { t: 'Gain',
              b: 'Niveau de sortie de l’oscillateur B',
              reviewed: true },
    },
    'bank-a': {
        en: { t: 'OSC A',
              b: 'Wavetable bank for oscillator A — 20 banks from analog to spectral' },
        fr: { t: 'OSC A',
              b: 'Banque de tables d’ondes de l’oscillateur A — 20 banques, de l’analogique au spectral',
              reviewed: true },
    },
    'bank-b': {
        en: { t: 'OSC B',
              b: 'Wavetable bank for oscillator B — 20 banks from analog to spectral' },
        fr: { t: 'OSC B',
              b: 'Banque de tables d’ondes de l’oscillateur B — 20 banques, de l’analogique au spectral',
              reviewed: true },
    },
    'attack': {
        en: { t: 'Attack',
              b: 'Envelope attack time — how quickly the sound fades in (1-5000 ms)' },
        fr: { t: 'Attaque',
              b: 'Temps d’attaque de l’enveloppe — rapidité d’apparition du son (1-5000 ms)',
              reviewed: true },
    },
    'decay': {
        en: { t: 'Decay',
              b: 'Envelope decay time — how quickly the sound drops from peak to sustain level (10-5000 ms)' },
        fr: { t: 'Déclin',
              b: 'Temps de déclin de l’enveloppe — rapidité de descente du sommet au niveau de maintien (10-5000 ms)',
              reviewed: true },
    },
    'sustain': {
        en: { t: 'Sustain',
              b: 'Envelope sustain level — volume held while note is on after decay (0-100%)' },
        fr: { t: 'Maintien',
              b: 'Niveau de maintien de l’enveloppe — volume conservé après le déclin tant que la note dure (0-100 %)',
              reviewed: true },
    },
    'release': {
        en: { t: 'Release',
              b: 'Envelope release time — how long the sound fades out after note off (10-10000 ms)' },
        fr: { t: 'Relâchement',
              b: 'Temps de relâchement de l’enveloppe — durée de disparition du son une fois la note relâchée (10-10000 ms)',
              reviewed: true },
    },
    'filter-cutoff': {
        en: { t: 'Filter',
              b: 'Low-pass filter cutoff frequency (20-20000 Hz)' },
        fr: { t: 'Filtre',
              b: 'Fréquence de coupure du filtre passe-bas (20-20000 Hz)',
              reviewed: true },
    },
    'filter-lfo-depth': {
        en: { t: 'Flt LFO',
              b: 'Filter LFO depth — modulates cutoff using LFO A phase for sweeping filter effects (0-100%)' },
        fr: { t: 'LFO filtre',
              b: 'Profondeur du LFO sur le filtre — module la coupure avec la phase du LFO A pour des balayages (0-100 %)',
              reviewed: true },
    },
    'velocity-to-filter': {
        en: { t: 'Vel>Flt',
              b: 'Velocity to filter — MIDI velocity modulates filter cutoff. Low velocity = darker, high velocity = brighter (0-100%)' },
        fr: { t: 'Vél>Filt',
              b: 'Vélocité vers filtre — la vélocité MIDI module la coupure. Vélocité faible = plus sombre, vélocité forte = plus clair (0-100 %)',
              reviewed: true },
    },
    'master-volume': {
        en: { t: 'Volume',
              b: 'Master output volume in dB' },
        fr: { t: 'Volume',
              b: 'Volume général de sortie, en dB',
              reviewed: true },
    },
    'timing-random': {
        en: { t: 'Timing',
              b: 'Random delay offset per chord voice — adds organic strum feel (0-100 ms)' },
        fr: { t: 'Décalage',
              b: 'Retard aléatoire pour chaque voix de l’accord — donne un égrenage naturel (0-100 ms)',
              reviewed: true },
    },
    'detune-random': {
        en: { t: 'Detune',
              b: 'Random pitch deviation per chord voice — creates ensemble/unison width (0-50 cents)' },
        fr: { t: 'Désaccord',
              b: 'Désaccord aléatoire pour chaque voix de l’accord — crée une largeur d’ensemble ou d’unisson (0-50 cents)',
              reviewed: true },
    },

    // ── Effects tab ─────────────────────────────────────────────────────────
    // Each section and its own bypass button share the section's caption as the
    // tip title and differ in the body.
    'fx-chorus': {
        en: { t: 'Chorus',
              b: 'Chorus effect — adds width and movement via modulated delay copies' },
        fr: { t: 'Chorus',
              b: 'Effet de chorus — ajoute largeur et mouvement par des copies retardées et modulées',
              reviewed: true },
    },
    'fx-chorus-bypass': {
        en: { t: 'Chorus',
              b: 'Enable/disable chorus processing' },
        fr: { t: 'Chorus',
              b: 'Activer ou désactiver le traitement de chorus',
              reviewed: true },
    },
    'fx-delay': {
        en: { t: 'Delay',
              b: 'Stereo delay with Normal and PingPong modes' },
        fr: { t: 'Délai',
              b: 'Délai stéréo avec les modes Normal et PingPong',
              reviewed: true },
    },
    'fx-delay-bypass': {
        en: { t: 'Delay',
              b: 'Enable/disable delay processing' },
        fr: { t: 'Délai',
              b: 'Activer ou désactiver le traitement de délai',
              reviewed: true },
    },
    // CLEAN SPLIT.
    'fx-eq': {
        en: { t: '3-band EQ',
              b: 'low shelf (200 Hz), mid peak (variable), high shelf (8 kHz)' },
        fr: { t: 'EQ 3 bandes',
              b: 'plateau grave (200 Hz), cloche médium (variable), plateau aigu (8 kHz)',
              reviewed: true },
    },
    'fx-eq-bypass': {
        en: { t: 'EQ',
              b: 'Enable/disable EQ processing' },
        fr: { t: 'EQ',
              b: 'Activer ou désactiver le traitement d’égalisation',
              reviewed: true },
    },
    'fx-reverb': {
        en: { t: 'Reverb',
              b: 'Schroeder reverb with adjustable pre-delay' },
        fr: { t: 'Réverb',
              b: 'Réverbération de Schroeder avec pré-délai réglable',
              reviewed: true },
    },
    'fx-reverb-bypass': {
        en: { t: 'Reverb',
              b: 'Enable/disable reverb processing' },
        fr: { t: 'Réverb',
              b: 'Activer ou désactiver le traitement de réverbération',
              reviewed: true },
    },
    'chorus-rate': {
        en: { t: 'Rate',
              b: 'Chorus LFO rate (0.1-10 Hz)' },
        fr: { t: 'Vitesse',
              b: 'Vitesse du LFO de chorus (0,1-10 Hz)',
              reviewed: true },
    },
    'chorus-depth': {
        en: { t: 'Depth',
              b: 'Chorus modulation depth' },
        fr: { t: 'Profondeur',
              b: 'Profondeur de modulation du chorus',
              reviewed: true },
    },
    'chorus-mix': {
        en: { t: 'Mix',
              b: 'Chorus dry/wet mix' },
        fr: { t: 'Mix',
              b: 'Mix son direct / son traité du chorus',
              reviewed: true },
    },
    'delay-time': {
        en: { t: 'Time',
              b: 'Delay time (1-2000 ms)' },
        fr: { t: 'Durée',
              b: 'Durée du délai (1-2000 ms)',
              reviewed: true },
    },
    'delay-feedback': {
        en: { t: 'Feedback',
              b: 'Delay feedback amount (0-95%)' },
        fr: { t: 'Réinjection',
              b: 'Taux de réinjection du délai (0-95 %)',
              reviewed: true },
    },
    // HAND-SPLIT, and the reason is spelled out at the head of this file: a
    // mechanical split on the first ": " would have titled this tip "Normal".
    'delay-mode': {
        en: { t: 'Mode',
              b: 'Normal: mono delay. PingPong: alternating stereo bounces' },
        fr: { t: 'Mode',
              b: 'Normal : délai mono. PingPong : rebonds stéréo alternés',
              reviewed: true },
    },
    'delay-mix': {
        en: { t: 'Mix',
              b: 'Delay dry/wet mix' },
        fr: { t: 'Mix',
              b: 'Mix son direct / son traité du délai',
              reviewed: true },
    },
    'eq-low': {
        en: { t: 'Low',
              b: 'Low shelf gain at 200 Hz (+/-12 dB)' },
        fr: { t: 'Grave',
              b: 'Gain du plateau grave à 200 Hz (±12 dB)',
              reviewed: true },
    },
    'eq-mid': {
        en: { t: 'Mid',
              b: 'Mid peak gain (+/-12 dB)' },
        fr: { t: 'Médium',
              b: 'Gain de la cloche médium (±12 dB)',
              reviewed: true },
    },
    'eq-mid-freq': {
        en: { t: 'Mid Freq',
              b: 'Mid peak center frequency (200-8000 Hz)' },
        fr: { t: 'Fréq. méd.',
              b: 'Fréquence centrale de la cloche médium (200-8000 Hz)',
              reviewed: true },
    },
    'eq-high': {
        en: { t: 'High',
              b: 'High shelf gain at 8 kHz (+/-12 dB)' },
        fr: { t: 'Aigu',
              b: 'Gain du plateau aigu à 8 kHz (±12 dB)',
              reviewed: true },
    },
    'reverb-size': {
        en: { t: 'Size',
              b: 'Reverb room size (0-100%)' },
        fr: { t: 'Taille',
              b: 'Taille de la salle de réverbération (0-100 %)',
              reviewed: true },
    },
    'reverb-damp': {
        en: { t: 'Damp',
              b: 'Reverb high-frequency damping (0-100%)' },
        fr: { t: 'Amort.',
              b: 'Amortissement des aigus de la réverbération (0-100 %)',
              reviewed: true },
    },
    'reverb-predelay': {
        en: { t: 'Pre-dly',
              b: 'Time before reverb onset (0-200 ms)' },
        fr: { t: 'Pré-délai',
              b: 'Temps avant le début de la réverbération (0-200 ms)',
              reviewed: true },
    },
    'reverb-mix': {
        en: { t: 'Mix',
              b: 'Reverb dry/wet mix' },
        fr: { t: 'Mix',
              b: 'Mix son direct / son traité de la réverbération',
              reviewed: true },
    },

    // ── Tuning tab: the container, and the seventeen tips inside the panel ──
    // js/tuning-panel.js authored these seventeen as data-tooltip attributes in
    // its own template. This version DELETES the attributes and binds the same
    // copy through TIP_BINDINGS, so the panel's markup carries no copy at all
    // and the module is not forked further than the attribute deletion.
    'tuning-container': {
        en: { t: 'Tuning',
              b: 'Microtonal tuning system — edit intervals, load presets, or generate custom scales' },
        fr: { t: 'Accord',
              b: 'Système d’accord microtonal — modifier les intervalles, charger des préréglages ou générer des gammes',
              reviewed: true },
    },
    'tp-interval-list': {
        en: { t: 'Intervals',
              b: 'Scale intervals in cents. Click any value to edit. Last row is the period (usually 1200c for octave)' },
        fr: { t: 'Intervalles',
              b: 'Intervalles de la gamme en cents. Cliquer sur une valeur pour la modifier. La dernière ligne est la période (1200c pour l’octave, en général)',
              reviewed: true },
    },
    'tp-viz-circle': {
        en: { t: 'Pitch circle',
              b: 'intervals as spokes around a circle. Active notes highlight in red' },
        fr: { t: 'Cercle des hauteurs',
              b: 'intervalles en rayons autour d’un cercle. Les notes actives apparaissent en rouge',
              reviewed: true },
    },
    'tp-viz-polar': {
        en: { t: 'Polar plot',
              b: 'intervals mapped by both angle and distance' },
        fr: { t: 'Tracé polaire',
              b: 'intervalles placés selon l’angle et la distance',
              reviewed: true },
    },
    'tp-viz-matrix': {
        en: { t: 'Interval matrix',
              b: 'cent distance between every pair of scale degrees' },
        fr: { t: 'Matrice d’intervalles',
              b: 'écart en cents entre chaque paire de degrés de la gamme',
              reviewed: true },
    },
    'tp-viz-truekeys': {
        en: { t: 'True Keys',
              b: 'hold 2+ notes to see real cent intervals between them' },
        fr: { t: 'Touches',
              b: 'tenir 2 notes ou plus pour voir les intervalles réels en cents',
              reviewed: true },
    },
    'tp-viz-rotation': {
        en: { t: 'Rotation table',
              b: 'all modes of the current scale' },
        fr: { t: 'Table de rotation',
              b: 'tous les modes de la gamme actuelle',
              reviewed: true },
    },
    'tp-library': {
        en: { t: 'Tuning Library',
              b: 'Browse 24+ built-in tuning presets across Historical, World, and Experimental categories' },
        fr: { t: 'Bibliothèque de gammes',
              b: 'Parcourir plus de 24 gammes intégrées, réparties en catégories Historiques, Du monde et Expérimentales',
              reviewed: true },
    },
    'tp-ref-pitch': {
        en: { t: 'A4 REF',
              b: 'A4 reference frequency (400-480 Hz). Drag up/down to adjust. Default: 440 Hz' },
        fr: { t: 'RÉF. A4',
              b: 'Fréquence de référence de A4 (400-480 Hz). Glisser vers le haut ou le bas pour régler. Par défaut : 440 Hz',
              reviewed: true },
    },
    'tp-scale-name': {
        en: { t: 'Scale',
              b: 'Name of the currently loaded tuning/scale' },
        fr: { t: 'Gamme',
              b: 'Nom de la gamme actuellement chargée',
              reviewed: true },
    },
    'tp-stretch': {
        en: { t: 'Stretch',
              b: 'Stretch or compress the octave ratio. 1.00 = pure octave (1200 cents)' },
        fr: { t: 'Étirement',
              b: 'Étirer ou comprimer le rapport d’octave. 1,00 = octave juste (1200 cents)',
              reviewed: true },
    },
    'tp-pb-range': {
        en: { t: 'PB Range',
              b: 'Pitch bend range in semitones (1-48 st). Controls how far the pitch wheel bends notes' },
        fr: { t: 'Plage PB',
              b: 'Plage du pitch bend en demi-tons (1-48 st). Détermine l’amplitude de la molette de hauteur',
              reviewed: true },
    },
    'tp-load-scl': {
        en: { t: 'Load .SCL',
              b: 'Import a Scala (.scl) tuning file' },
        fr: { t: 'Ouvrir .SCL',
              b: 'Importer un fichier de gamme Scala (.scl)',
              reviewed: true },
    },
    'tp-load-kbm': {
        en: { t: 'Load .KBM',
              b: 'Import a keyboard mapping (.kbm) file' },
        fr: { t: 'Ouvrir .KBM',
              b: 'Importer un fichier de mappage clavier (.kbm)',
              reviewed: true },
    },
    'tp-save-scl': {
        en: { t: 'Save .SCL',
              b: 'Export current tuning as Scala (.scl) file' },
        fr: { t: 'Enreg. .SCL',
              b: 'Exporter la gamme actuelle en fichier Scala (.scl)',
              reviewed: true },
    },
    'tp-save-kbm': {
        en: { t: 'Save .KBM',
              b: 'Export keyboard mapping as .kbm file' },
        fr: { t: 'Enreg. .KBM',
              b: 'Exporter le mappage clavier en fichier .kbm',
              reviewed: true },
    },
    'tp-export-html': {
        en: { t: 'Export HTML',
              b: 'Export tuning documentation as a standalone HTML page' },
        fr: { t: 'Exporter HTML',
              b: 'Exporter la documentation de la gamme en page HTML autonome',
              reviewed: true },
    },
    // CLEAN SPLIT.
    'tp-generator': {
        en: { t: 'Generate custom scales',
              b: 'Equal Division (EDO), Harmonic Series, or Rank-2 Temperament' },
        fr: { t: 'Générer des gammes personnalisées',
              b: 'division égale (EDO), série harmonique ou tempérament de rang 2',
              reviewed: true },
    },

    // ── Native dialogs ──────────────────────────────────────────────────────
    // prompt() and confirm() take a STRING, not an element, so neither can be a
    // [data-i18n] node and neither can go through setLabel(). They live here
    // with an empty body rather than in LABELS, because assertion 15's dead-key
    // sweep only recognises a LABELS key that an element or a setLabel call
    // names — a key reached only through trLabel() would report as dead while
    // being read on every rename and every delete.
    'dialog.renamePrompt': {
        en: { t: 'Rename preset:', b: '' },
        fr: { t: 'Renommer le préréglage :', b: '', reviewed: true },
    },
    'dialog.deleteConfirm': {
        en: { t: 'Delete preset "{name}"?', b: '' },
        fr: { t: 'Supprimer le préréglage « {name} » ?', b: '', reviewed: true },
    },
});

// ============================================================================
// LABELS — the on-page text (v2.9.1, canon v2)
// ============================================================================
//
// I18N above is HOVER-HELP copy: a title and a body rendered into a wrapping
// 220 px tooltip. LABELS is ON-PAGE copy: one string dropped into a cell that
// mostly does not wrap. They are different problems and this table keeps them
// apart on purpose.
//
// ── THE REUSE RULE, AND WHERE IT APPLIES HERE ──────────────────────────────
// trLabel() falls back to I18N when a key is absent here, so a control whose
// tooltip TITLE already IS its caption could carry ONE key. That is deliberately
// NOT done: the hand-split above set every tooltip title FROM the caption, so a
// fallback would make the tooltip table the owner of the page's captions and a
// later tooltip rewrite would silently move a knob label. The two tables are
// kept disjoint and the duplication is on purpose.
//
// ── ENGLISH WAS MOVED, NOT RE-TYPED ────────────────────────────────────────
// Every `en` below is what index.html, js/app.js's knob configuration or
// js/tuning-panel.js carried through v2.8.4, taken from
// scripts/i18n-extract.js's inventory rather than transcribed — with TWO
// exceptions, both contract §6 rewrites, both named here:
//
//   'Intervals (12 notes)'  ->  'Intervals: 12'
//   '12 notes'              ->  'Notes: 12'
//
// Both were composed with a count and an inflected noun. French pluralises zero
// as singular and English does not, and `count` here is `intervals.length - 1`,
// which a degenerate one-line .scl file makes 0. Contract §6 says copy that
// needs a count is authored AROUND the inflection rather than through a plural
// engine, so the noun moved in front of the number where it is invariant in both
// languages. The English loses the word "notes" from the interval header; that
// is the visible cost and it is recorded rather than hidden.
//
// ── FRENCH IS SIZED, NOT SHRUNK ────────────────────────────────────────────
// D-04 forbids an auto-shrink font and a short-variant fallback: exactly ONE
// French string per key, and nothing chooses between variants at runtime. This
// is an 800 x 500 frame whose knob captions live in 44 px and 52 px columns, so
// every French caption below was MEASURED AS RENDERED inside its own element —
// text-transform and letter-spacing are not in getComputedStyle().font, so a
// font probe reads short and a pin lands under the French.
//
// ALL FRENCH IS MACHINE-DRAFTED, `reviewed: false`. No native speaker has read
// it. `node scripts/check-i18n.js` prints the worklist, LABELS included.
// ============================================================================

export const LABELS = Object.freeze({

    // ── Tab row ─────────────────────────────────────────────────────────────
    'label.tabVoice':    { en: { t: 'Voice' },   fr: { t: 'Voix',   reviewed: true } },
    'label.tabTuning':   { en: { t: 'Tuning' },  fr: { t: 'Accord', reviewed: true } },
    'label.tabSynth':    { en: { t: 'Synth' },   fr: { t: 'Synthé', reviewed: true } },
    'label.tabEffects':  { en: { t: 'Effects' }, fr: { t: 'Effets', reviewed: true } },

    // ── The dice menu. Each row is a caption AND a description, and the two
    //    are separate keyed FRAGMENTS inside the same button rather than one
    //    keyed wrapper: applyLabel writes textContent, so keying the button
    //    would delete the .mode-desc span on the first sweep, and measuring the
    //    wrapper's box would measure the whole row forever (assertion 7).
    'label.diceGentle':      { en: { t: 'Gentle' },     fr: { t: 'Léger',       reviewed: true } },
    // "du réglage actuel", not the fuller "autour des réglages actuels": the
    // .mode-desc line is 112 px and the longer form wraps to a second line,
    // making the whole dice menu 10 px taller in French and moving the two rows
    // below it. Measured as rendered, both ways.
    'label.diceGentleDesc':  { en: { t: '±15% variation from current' },
                               fr: { t: '±15 % du réglage actuel', reviewed: true } },
    'label.diceWild':        { en: { t: 'Wild' },       fr: { t: 'Extrême',     reviewed: true } },
    'label.diceWildDesc':    { en: { t: 'Full range randomization' },
                               fr: { t: 'Aléatoire sur toute la plage', reviewed: true } },
    'label.diceSoundOnly':   { en: { t: 'Sound Only' }, fr: { t: 'Timbre seul', reviewed: true } },
    'label.diceSoundOnlyDesc': { en: { t: 'Preserve tuning, chord & volume' },
                               fr: { t: 'Conserve accord, voix et volume', reviewed: true } },

    // ── Preset bar, browser and save dialog ─────────────────────────────────
    // "Enreg.", not "Enregistrer": this caption is worn by the preset bar's SAVE
    // button, whose width is pinned in the CSS so its French face cannot squeeze
    // the preset-name display beside it. The full word is 91.28 px against
    // SAVE's 44.83; the abbreviation is the same one the tuning panel's file
    // buttons already use.
    'label.save':            { en: { t: 'Save' },   fr: { t: 'Enreg.', reviewed: true } },
    'label.cancel':          { en: { t: 'Cancel' }, fr: { t: 'Annuler',     reviewed: true } },
    'label.presetBrowser':   { en: { t: 'Preset Browser' }, fr: { t: 'Navigateur de préréglages', reviewed: true } },
    'label.savePresetTitle': { en: { t: 'Save Preset' },    fr: { t: 'Enregistrer le préréglage', reviewed: true } },
    'label.noPresetsInCategory': { en: { t: 'No presets in this category' },
                               fr: { t: 'Aucun préréglage dans cette catégorie', reviewed: true } },

    // The six preset CATEGORIES. Only the caption is localized — the English
    // string stays the value that reaches C++ and comes back on preset.category.
    // 'label.all' is shared with the interval selector's All button: one string,
    // one key, two anchors.
    'label.all':             { en: { t: 'All' },          fr: { t: 'Tous',      reviewed: true } },
    'label.catAmbient':      { en: { t: 'Ambient' },      fr: { t: 'Ambient',   reviewed: true, sameAsEn: true } },
    'label.catCinematic':    { en: { t: 'Cinematic' },    fr: { t: 'Cinéma',    reviewed: true } },
    'label.catClassicPads':  { en: { t: 'Classic Pads' }, fr: { t: 'Nappes classiques', reviewed: true } },
    'label.catDrones':       { en: { t: 'Drones' },       fr: { t: 'Bourdons',  reviewed: true } },
    'label.catExperimental': { en: { t: 'Experimental' }, fr: { t: 'Expérimental', reviewed: true } },

    // ── Voice tab ───────────────────────────────────────────────────────────
    'label.keyRoot':     { en: { t: 'Key Root' },  fr: { t: 'Fondamentale', reviewed: true } },
    'label.voicing':     { en: { t: 'Voicing' },   fr: { t: 'Disposition',  reviewed: true,
                       termNote: 'chord voicing — how the chord’s notes are laid out across octaves — not O-Formant’s phonetic voicing (Voisement). Disposition is the French harmony term for exactly this' } },
    'label.intervals':   { en: { t: 'Intervals' }, fr: { t: 'Intervalles',  reviewed: true } },
    'label.none':        { en: { t: 'None' },      fr: { t: 'Aucun',        reviewed: true } },
    'label.loading':     { en: { t: 'Loading...' }, fr: { t: 'Chargement…',  reviewed: true } },
    'label.activeNotes': { en: { t: 'Active Notes' }, fr: { t: 'Notes actives', reviewed: true } },
    'label.playANote':   { en: { t: 'Play a note to see chord voicing' },
                           fr: { t: 'Jouer une note pour voir la disposition', reviewed: true } },

    // The five Voice-tab knob captions, written by makeKnob() from a key.
    'label.voices':      { en: { t: 'Voices' },     fr: { t: 'Voix',        reviewed: true } },
    'label.complexity':  { en: { t: 'Complexity' }, fr: { t: 'Complexité',  reviewed: true } },
    'label.spread':      { en: { t: 'Spread' },     fr: { t: 'Étalement',   reviewed: true } },
    'label.spacing':     { en: { t: 'Spacing' },    fr: { t: 'Espacement',  reviewed: true } },
    'label.inversion':   { en: { t: 'Inversion' },  fr: { t: 'Renvers.',    reviewed: true } },

    // ── Synth tab ───────────────────────────────────────────────────────────
    // "OSC A" and "OSC B" are oscillator designations, not words: the French is
    // the same and says so with sameAsEn rather than by silence.
    'label.oscA':        { en: { t: 'OSC A' },   fr: { t: 'OSC A',   reviewed: true, sameAsEn: true } },
    'label.oscB':        { en: { t: 'OSC B' },   fr: { t: 'OSC B',   reviewed: true, sameAsEn: true } },
    'label.pos':         { en: { t: 'Pos' },     fr: { t: 'Pos.',    reviewed: true, sameAsEn: true } },
    'label.rate':        { en: { t: 'Rate' },    fr: { t: 'Vitesse', reviewed: true } },
    'label.depth':       { en: { t: 'Depth' },   fr: { t: 'Prof.',   reviewed: true } },
    'label.gain':        { en: { t: 'Gain' },    fr: { t: 'Gain',    reviewed: true, sameAsEn: true } },
    'label.attack':      { en: { t: 'Attack' },  fr: { t: 'Attaque', reviewed: true } },
    'label.decay':       { en: { t: 'Decay' },   fr: { t: 'Déclin',  reviewed: true } },
    'label.sustain':     { en: { t: 'Sustain' }, fr: { t: 'Maintien', reviewed: true } },
    'label.release':     { en: { t: 'Release' }, fr: { t: 'Relâch.', reviewed: true } },
    'label.filter':      { en: { t: 'Filter' },  fr: { t: 'Filtre',  reviewed: true } },
    'label.fltLfo':      { en: { t: 'Flt LFO' }, fr: { t: 'LFO filt.', reviewed: true } },
    'label.velFlt':      { en: { t: 'Vel>Flt' }, fr: { t: 'Vél>Filt', reviewed: true } },
    'label.volume':      { en: { t: 'Volume' },  fr: { t: 'Volume',  reviewed: true, sameAsEn: true } },
    'label.timing':      { en: { t: 'Timing' },  fr: { t: 'Décalage', reviewed: true } },
    'label.detune':      { en: { t: 'Detune' },  fr: { t: 'Désacc.', reviewed: true } },

    // ── Effects tab ─────────────────────────────────────────────────────────
    'label.fxChorus':    { en: { t: 'Chorus' },   fr: { t: 'Chorus',  reviewed: true, sameAsEn: true } },
    'label.fxDelay':     { en: { t: 'Delay' },    fr: { t: 'Délai',   reviewed: true } },
    'label.fxEq':        { en: { t: 'EQ' },       fr: { t: 'EQ',      reviewed: true, sameAsEn: true } },
    'label.fxReverb':    { en: { t: 'Reverb' },   fr: { t: 'Réverb', reviewed: true } },
    'label.mix':         { en: { t: 'Mix' },      fr: { t: 'Mix',     reviewed: true, sameAsEn: true } },
    'label.time':        { en: { t: 'Time' },     fr: { t: 'Durée',   reviewed: true } },
    'label.feedback':    { en: { t: 'Feedback' }, fr: { t: 'Réinj.',  reviewed: true } },
    'label.low':         { en: { t: 'Low' },      fr: { t: 'Grave',   reviewed: true } },
    'label.mid':         { en: { t: 'Mid' },      fr: { t: 'Médium',  reviewed: true } },
    'label.midFreq':     { en: { t: 'Mid Freq' }, fr: { t: 'Fréq. méd.', reviewed: true } },
    'label.high':        { en: { t: 'High' },     fr: { t: 'Aigu',    reviewed: true } },
    'label.size':        { en: { t: 'Size' },     fr: { t: 'Taille',  reviewed: true } },
    'label.damp':        { en: { t: 'Damp' },     fr: { t: 'Amort.',  reviewed: true } },
    'label.preDly':      { en: { t: 'Pre-dly' },  fr: { t: 'Pré-dél.', reviewed: true } },
    // Shared by the delay-mode dropdown caption and the rotation table's first
    // column header. One string, one key, two anchors.
    'label.mode':        { en: { t: 'Mode' },     fr: { t: 'Mode',    reviewed: true, sameAsEn: true } },

    // ── Tuning panel (js/tuning-panel.js) ───────────────────────────────────
    // A PARAMETERISED entry, written by __setLabel with vars = { n }. See the
    // §6 note at the head of this table for why the noun sits in front of the
    // number instead of after it.
    'label.intervalsHeader': { en: { t: 'Intervals: {n}' },
                               fr: { t: 'Intervalles : {n}', reviewed: true } },
    'label.noteCount':       { en: { t: 'Notes: {n}' },
                               fr: { t: 'Notes : {n}', reviewed: true } },
    'label.tonic':           { en: { t: 'Tonic' },  fr: { t: 'Tonique', reviewed: true } },
    'label.scaleIntervals':  { en: { t: 'Scale Intervals' }, fr: { t: 'Intervalles de la gamme', reviewed: true } },
    'label.vizCircle':       { en: { t: 'Circle' },    fr: { t: 'Cercle',   reviewed: true } },
    'label.vizPolar':        { en: { t: 'Polar' },     fr: { t: 'Polaire',  reviewed: true } },
    'label.vizMatrix':       { en: { t: 'Matrix' },    fr: { t: 'Matrice',  reviewed: true } },
    'label.vizTrueKeys':     { en: { t: 'True Keys' }, fr: { t: 'Touches',  reviewed: true } },
    'label.vizRotation':     { en: { t: 'Rotation' },  fr: { t: 'Rotation', reviewed: true, sameAsEn: true } },
    'label.tkHint':          { en: { t: 'Hold 2+ notes to see intervals' },
                               fr: { t: 'Tenir 2 notes ou plus pour voir les intervalles', reviewed: true } },
    'label.totalSpan':       { en: { t: 'Total span' }, fr: { t: 'Écart total', reviewed: true } },
    // "Bibliothèque", not the fuller "Bibliothèque de gammes": the tuning
    // panel's 220 px controls column already sits PARTLY OUTSIDE the 800 px
    // frame in English — a pre-existing horizontal overflow this commit does not
    // introduce and does not fix — so the English caption at 94.22 px is the
    // budget. "Bibliothèque de gammes" is 152.5 px and crossed the frame edge by
    // 17.4 px; "Bibliothèque" is 82.28 px and clears it.
    //
    // v2.9.1 RE-MEASURED AND CONFIRMED TO THE TENTH. Stage N re-applied the
    // glossary root and re-ran the gate: assertion 6 reported label.tuningLibrary
    // 0.0px -> 17.4px @665,120 153x12, and assertion 5 reported the same caption
    // spilling its offsetParent's padding box 0.0px -> 20.4px. The measured
    // 152.50 px is right and so is the 17.4 px overflow. The abbreviation stays;
    // the tip TITLE below carries the full "Bibliothèque de gammes", where a
    // 220 px wrapping tooltip has the room the caption does not.
    'label.tuningLibrary':   { en: { t: 'Tuning Library' }, fr: { t: 'Bibliothèque', reviewed: true } },
    // The library filter is a PLAIN select over the strings all / Historical /
    // ... — it is not an AudioParameterChoice, no host ever shows these six
    // strings, and translating them cannot make the page and an automation lane
    // disagree. That is the discriminator; the five dropdowns that ARE choice
    // parameters are exempt below.
    'label.catAllCategories':  { en: { t: 'All Categories' },  fr: { t: 'Toutes catégories', reviewed: true } },
    'label.catHistorical':     { en: { t: 'Historical' },      fr: { t: 'Historiques',   reviewed: true } },
    'label.catJustIntonation': { en: { t: 'Just Intonation' }, fr: { t: 'Intonation juste', reviewed: true } },
    'label.catEqualDivisions': { en: { t: 'Equal Divisions' }, fr: { t: 'Divisions égales', reviewed: true } },
    'label.catNonOctave':      { en: { t: 'Non-Octave' },      fr: { t: 'Non octaviantes', reviewed: true } },
    'label.catWorld':          { en: { t: 'World' },           fr: { t: 'Du monde',      reviewed: true } },
    // "A4" is a pitch identifier and stays; only the abbreviation "REF" moves.
    'label.a4Ref':       { en: { t: 'A4 REF' },   fr: { t: 'RÉF. A4',   reviewed: true } },
    'label.stretch':     { en: { t: 'Stretch' },  fr: { t: 'Étirement', reviewed: true } },
    'label.pbRange':     { en: { t: 'PB Range' }, fr: { t: 'Plage PB',  reviewed: true } },
    // The four file buttons keep their EXTENSIONS, which are file-format
    // identifiers, and translate only the verb.
    'label.loadScl':     { en: { t: 'Load .SCL' }, fr: { t: 'Ouvrir .SCL', reviewed: true } },
    'label.loadKbm':     { en: { t: 'Load .KBM' }, fr: { t: 'Ouvrir .KBM', reviewed: true } },
    'label.saveScl':     { en: { t: 'Save .SCL' }, fr: { t: 'Enreg. .SCL', reviewed: true } },
    'label.saveKbm':     { en: { t: 'Save .KBM' }, fr: { t: 'Enreg. .KBM', reviewed: true } },
    'label.exportHtml':  { en: { t: 'Export HTML' }, fr: { t: 'Exporter HTML', reviewed: true } },
    'label.generateScale': { en: { t: 'Generate Scale' }, fr: { t: 'Générer une gamme', reviewed: true } },
    'label.genEdo':      { en: { t: 'EDO (Equal Division)' }, fr: { t: 'EDO (division égale)', reviewed: true } },
    'label.genHarmonic': { en: { t: 'Harmonic Series' },      fr: { t: 'Série harmonique',     reviewed: true } },
    'label.genRank2':    { en: { t: 'Rank-2 Temperament' },   fr: { t: 'Tempérament de rang 2', reviewed: true } },
    'label.genDivisions': { en: { t: 'Divisions' },     fr: { t: 'Divisions', reviewed: true, sameAsEn: true } },
    'label.genPeriod':   { en: { t: 'Period (c)' },     fr: { t: 'Période (c)', reviewed: true } },
    'label.genStart':    { en: { t: 'Start Harmonic' }, fr: { t: 'Harmonique de départ', reviewed: true } },
    'label.genEnd':      { en: { t: 'End Harmonic' },   fr: { t: 'Harmonique de fin',    reviewed: true } },
    'label.genGenerator': { en: { t: 'Generator (c)' }, fr: { t: 'Générateur (c)', reviewed: true } },
    'label.genCount':    { en: { t: 'Notes' },          fr: { t: 'Notes', reviewed: true, sameAsEn: true } },
    'label.generate':    { en: { t: 'Generate' },       fr: { t: 'Générer', reviewed: true } },

    // ── The settings popover, and the two faces of every toggle ─────────────
    'label.language':    { en: { t: 'Language' },   fr: { t: 'Langue', reviewed: true } },
    'label.hoverHelp':   { en: { t: 'Hover help' }, fr: { t: 'Infobulles', reviewed: true } },
    // Worn by the four FX bypass buttons AND by the hover-help switch. Written
    // only by setLabel, from an if/else and never a ternary — assertion 13.
    'ui.on':             { en: { t: 'On' },  fr: { t: 'Marche', reviewed: true } },
    'ui.off':            { en: { t: 'Off' }, fr: { t: 'Arrêt',  reviewed: true } },

    // ── Accessible names and the one placeholder ────────────────────────────
    // Every one of these was a native title= or an alt= in v2.8.4. Contract §4
    // DELETES native title= rather than localizing it: on a page with a
    // measure-then-pin renderer a native title renders a second, untranslated OS
    // tooltip competing with the tip. Where the title was an element's only
    // help, its text became the accessible name — no new prose was invented.
    'alt.background':      { en: { t: 'Background' }, fr: { t: 'Arrière-plan', reviewed: true } },
    'alt.botanical':       { en: { t: 'Botanical' },  fr: { t: 'Motif botanique', reviewed: true } },
    'aria.randomize':      { en: { t: 'Randomize parameters' }, fr: { t: 'Rendre les paramètres aléatoires', reviewed: true } },
    'aria.prevPreset':     { en: { t: 'Previous preset' }, fr: { t: 'Préréglage précédent', reviewed: true } },
    'aria.browsePresets':  { en: { t: 'Click to browse presets' }, fr: { t: 'Cliquer pour parcourir les préréglages', reviewed: true } },
    'aria.nextPreset':     { en: { t: 'Next preset' }, fr: { t: 'Préréglage suivant', reviewed: true } },
    'aria.savePreset':     { en: { t: 'Save current settings as preset' },
                             fr: { t: 'Enregistrer les réglages actuels comme préréglage', reviewed: true } },
    'placeholder.presetName': { en: { t: 'Preset name...' }, fr: { t: 'Nom du préréglage…', reviewed: true } },
    'aria.factoryPreset':  { en: { t: 'Factory preset' }, fr: { t: 'Préréglage d’usine', reviewed: true } },
    'aria.rename':         { en: { t: 'Rename' }, fr: { t: 'Renommer', reviewed: true } },
    'aria.delete':         { en: { t: 'Delete' }, fr: { t: 'Supprimer', reviewed: true } },
    'aria.doubleClickEdit': { en: { t: 'Double-click to edit' }, fr: { t: 'Double-cliquer pour modifier', reviewed: true } },
    'aria.settings':       { en: { t: 'Settings' }, fr: { t: 'Réglages', reviewed: true } },
    'aria.langSelect':     { en: { t: 'Interface language' }, fr: { t: 'Langue de l’interface', reviewed: true } },
    'aria.helpToggle':     { en: { t: 'Toggle hover help' }, fr: { t: 'Activer ou désactiver les infobulles', reviewed: true } },
});

// ============================================================================
// I18N_EXEMPT — reasoned exclusions, never silence
// ============================================================================
//
// Every visible string the coverage scan finds must be a [data-i18n] element, a
// setLabel() call, or an entry HERE WITH A REASON. A bare skip list would let a
// missed label hide as a deliberate one.
// ============================================================================

export const I18N_EXEMPT = [
    ['Ouaricon Intonation Pad',
     'the product name in .title — a product name is never translated, and this one is the display form of the plugin\'s registered PRODUCT_NAME in CMakeLists.txt'],

    // #presetNameDisplay shows the loaded preset. The name IS the JSON filename
    // (OuariconPresetManager.h:283-285), so translating it breaks recall
    // outright. "Init" is the placeholder the manager overwrites on its first
    // pass.
    ['Init', 'a factory preset name — exempt under D-02, because the name IS the JSON filename'],

    // ── The five AudioParameterChoice dropdowns (D-01) ──────────────────────
    // keyRoot, voicingMode, wavetableBank, wavetableBank2 and delayMode are all
    // WebComboBoxRelay controls over choice parameters. Their option strings are
    // the parameter's own choice list, which the host shows in its automation
    // lane and which some hosts cache. Translating the option text would make
    // the page and the automation lane disagree about the same parameter. The
    // library filter and the generator type ARE localized, and the difference is
    // exactly this: those two are plain selects no host ever sees.
    ['Free',     'voicingMode choice-parameter value — host automation contract (D-01)'],
    ['Close',    'voicingMode choice-parameter value — host automation contract (D-01)'],
    ['Open',     'voicingMode choice-parameter value — host automation contract (D-01)'],
    ['Drop-2',   'voicingMode choice-parameter value — host automation contract (D-01)'],
    ['Thirds',   'voicingMode choice-parameter value — host automation contract (D-01)'],
    ['Quartal',  'voicingMode choice-parameter value — host automation contract (D-01)'],
    ['Quintal',  'voicingMode choice-parameter value — host automation contract (D-01)'],
    ['Normal',   'delayMode choice-parameter value — host automation contract (D-01)'],
    ['PingPong', 'delayMode choice-parameter value — host automation contract (D-01)'],

    // ── Identifiers, not words ──────────────────────────────────────────────
    ['12-TET Standard',
     'a tuning IDENTIFIER, not a caption — it is the name the tuning engine reports for the loaded scale and is matched against Scala file names'],
    ['English',  'endonym — a language name is never translated'],
    ['Français', 'endonym — a language name is never translated'],
];

// [selector, key] or [selector, key, wrapperSelector] or
// [selector, key, wrapperSelector, vars].
//
// The selector is the BINDING SITE and the wrapper climbs to the box the tip
// should hang off. applyI18n() uses document.querySelector, which returns the
// FIRST match in document order, so every row below names an element that
// carries a UNIQUE id — the knob, the select, the section — and never a bare
// class that repeats.
//
// NOT EVERY ANCHOR EXISTS AT PARSE TIME, and that is the one structural
// difference from every plugin shipped before this one. The 37 knob containers
// are built by makeKnob() inside the DOMContentLoaded handler, and the 17
// tuning-panel anchors are built by an async import that resolves later still.
// js/app.js therefore calls initI18n() at the END of DOMContentLoaded rather
// than the start, and calls applyI18n() again once the tuning panel reports
// ready. The first of those two sweeps warns about the 17 tuning selectors it
// cannot yet see; that is the canon's own "tip target not found" console.warn,
// left alone rather than worked around, because the canon block is byte-compared
// against scripts/i18n-canon.js and a local edit to silence it would fail
// assertion 6 on every plugin at once.
// EXPORTED, though js/app.js never imports it: assertion 7 requires this module
// to hold nothing but export declarations, because a bare top-level statement
// here throws out of module evaluation and takes every later initializer on the
// page with it. Same shape as O-Polystutter's exported LANES.
export const KNOB_TIPS = [
    ['voiceCount',       'voice-count'],
    ['complexity',       'complexity'],
    ['stereoSpread',     'stereo-spread'],
    ['spacing',          'spacing'],
    ['inversion',        'inversion'],
    // ONE key, TWO anchors — oscillator A and oscillator B.
    ['wavetablePos',     'wavetable-pos'],
    ['wavetablePos2',    'wavetable-pos'],
    ['lfoRate',          'lfo-rate'],
    ['lfoRate2',         'lfo-rate'],
    ['lfoDepth',         'lfo-depth'],
    ['lfoDepth2',        'lfo-depth'],
    ['gainA',            'gain-a'],
    ['gainB',            'gain-b'],
    ['attackTime',       'attack'],
    ['decayTime',        'decay'],
    ['sustainLevel',     'sustain'],
    ['releaseTime',      'release'],
    ['filterCutoff',     'filter-cutoff'],
    ['filterLfoDepth',   'filter-lfo-depth'],
    ['velocityToFilter', 'velocity-to-filter'],
    ['masterVolume',     'master-volume'],
    ['timingRandom',     'timing-random'],
    ['detuneRandom',     'detune-random'],
    ['chorusRate',       'chorus-rate'],
    ['chorusDepth',      'chorus-depth'],
    ['chorusMix',        'chorus-mix'],
    ['delayTime',        'delay-time'],
    ['delayFeedback',    'delay-feedback'],
    ['delayMix',         'delay-mix'],
    ['eqLowGain',        'eq-low'],
    ['eqMidGain',        'eq-mid'],
    ['eqMidFreq',        'eq-mid-freq'],
    ['eqHighGain',       'eq-high'],
    ['reverbSize',       'reverb-size'],
    ['reverbDamp',       'reverb-damp'],
    ['reverbPredelay',   'reverb-predelay'],
    ['reverbMix',        'reverb-mix'],
];

export const TIP_BINDINGS = [
    // The three new controls.
    ['#gear-btn',     'settings'],
    ['#lang-select',  'lang-select'],
    ['#tips-toggle',  'tips-toggle'],

    // The tab row.
    ['.tab-button[data-tab="voice"]',   'tab-voice'],
    ['.tab-button[data-tab="tuning"]',  'tab-tuning'],
    ['.tab-button[data-tab="synth"]',   'tab-synth'],
    ['.tab-button[data-tab="effects"]', 'tab-effects'],

    // Voice tab.
    ['#keyRootSelect',      'key-root',       '.dropdown-container'],
    ['#voicingModeSelect',  'voicing',        '.dropdown-container'],
    ['.interval-selector',  'intervals'],
    ['#intervalAllBtn',     'intervals-all'],
    ['#intervalNoneBtn',    'intervals-none'],
    ['#miniKeyboard',       'keyboard'],
    ['#freqList',           'freq-list'],

    // Synth tab — the two wavetable bank selects.
    ['#wavetableBankSelect',  'bank-a', '.dropdown-container'],
    ['#wavetableBank2Select', 'bank-b', '.dropdown-container'],
    ['#delayModeSelect',      'delay-mode', '.dropdown-container'],

    // Effects tab — a section and its own bypass button each carry a tip. The
    // button is INSIDE the section, and closest('[data-tip]') from the pointer
    // resolves to the button, which is the behaviour v2.8.4 already had.
    ['#chorusSection',    'fx-chorus'],
    ['#chorusBypassBtn',  'fx-chorus-bypass'],
    ['#delaySection',     'fx-delay'],
    ['#delayBypassBtn',   'fx-delay-bypass'],
    ['#eqSection',        'fx-eq'],
    ['#eqBypassBtn',      'fx-eq-bypass'],
    ['#reverbSection',    'fx-reverb'],
    ['#reverbBypassBtn',  'fx-reverb-bypass'],

    // Tuning tab — the pane itself, then the seventeen anchors js/tuning-panel.js
    // builds. Their copy used to live in that module's own template as
    // data-tooltip attributes; the attributes are deleted and the copy is here.
    ['#tuning-container',  'tuning-container'],
    ['#interval-list',     'tp-interval-list'],
    ['.viz-btn[data-mode="circle"]',   'tp-viz-circle'],
    ['.viz-btn[data-mode="polar"]',    'tp-viz-polar'],
    ['.viz-btn[data-mode="matrix"]',   'tp-viz-matrix'],
    ['.viz-btn[data-mode="truekeys"]', 'tp-viz-truekeys'],
    ['.viz-btn[data-mode="rotation"]', 'tp-viz-rotation'],
    ['#library-section',   'tp-library'],
    ['#ref-pitch-knob',    'tp-ref-pitch',  '.tuning-ref-section'],
    ['#scale-name-display', 'tp-scale-name'],
    ['#octave-stretch',    'tp-stretch',    '.octave-stretch-section'],
    ['#pitch-bend-range',  'tp-pb-range',   '.octave-stretch-section'],
    ['#btn-load-scl',      'tp-load-scl'],
    ['#btn-load-kbm',      'tp-load-kbm'],
    ['#btn-save-scl',      'tp-save-scl'],
    ['#btn-save-kbm',      'tp-save-kbm'],
    ['#btn-export-html',   'tp-export-html'],
    ['#generator-section', 'tp-generator'],

    // The 37 knobs. Generated from KNOB_TIPS so a knob cannot be given a tip
    // here and forgotten in the configuration js/app.js builds it from.
    ...KNOB_TIPS.map(([id, key]) => ['#' + id + 'Knob', key, '.knob-container']),
];

export function tr(key, lang, vars) {
    const entry = I18N[key];
    if (!entry) { console.warn(`i18n: missing key ${key}`); return { t: key, b: '' }; }
    const s = entry[lang] || entry.en;

    // A var VALUE that is itself an I18N key resolves to that key's localized
    // title; anything else is used literally. TIP_BINDINGS is evaluated once at
    // module load, so a localized string stored there would be frozen at the
    // load-time language.
    const resolve = (v) => {
        const nested = I18N[v];
        return nested ? String((nested[lang] || nested.en).t) : String(v);
    };

    const sub = (v) => vars
        ? String(v).replace(/\{(\w+)\}/g, (m, n) => (n in vars ? resolve(vars[n]) : m))
        : String(v);

    return { t: sub(s.t), b: sub(s.b) };
}
