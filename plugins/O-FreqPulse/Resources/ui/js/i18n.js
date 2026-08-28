/*
   This file is part of O-FreqPulse, an Ouaricon Audio plugin.
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
// i18n.js — O-FreqPulse UI copy, English + French (v1.18.0, canon v2)
//
// An ES module that EXPORTS ONLY. It must never self-execute: a bare top-level
// statement here throws out of module evaluation and takes every later
// initializer on the page with it (pattern_module_toplevel_init_tdz).
// scripts/check-i18n.js assertion 7 enforces that.
//
// SERVED ROOT IS Resources/ui, NOT Source/ui/public. O-FreqPulse has only the
// one UI tree and CMakeLists.txt embeds it directly; this file has to be listed
// in that juce_add_binary_data SOURCES block, reached by a getResource() branch
// and imported by js/app.js, all in the same commit, or the page 404s at
// runtime and presents as a dead panel with no other symptom (assertion 8).
//
// FILENAME: no hyphen. juce_add_binary_data STRIPS hyphens rather than
// converting them to underscores, so a second file named i18n-fr.js would have
// to be reached as the symbol i18nfr_js (critical_binary_data_strips_hyphens).
// One combined file for both languages sidesteps the question entirely.
//
// COPY IS textContent ON EVERY PATH — never innerHTML. showTooltip() in app.js
// builds the tip with createElement + textContent, and check-i18n assertion 9
// rejects any innerHTML reference here and any string literal containing the
// opening angle bracket. A line break, if one is ever needed, is \n plus CSS
// white-space: pre-line, never a markup tag.
//
// THE ENGLISH WAS MOVED, NOT REWRITTEN. v1.17.0 authored its hover help as a
// SINGLE data-tooltip string in the shape "Label: sentence.", read by a second
// tooltip renderer this version deletes. Every entry below is that string split
// on its FIRST ": " into the t/b pair the measure-then-pin renderer wants, with
// both halves byte-identical to v1.17.0 either side of the separator.
//
// ONE tip did not split cleanly and was HAND-SPLIT: `grid` (the step-sequencer
// grid) was authored with no colon at all, so its title "Step Grid" is the one
// new English string in this table's tooltip half. It is named in the commit
// message rather than left to be discovered.
//
// THREE NEW CONTROLS carry new English copy: `settings`, `lang-select` and
// `tips-toggle`. The first two are the gear popover and the language selector,
// which did not exist before; the third is the hover-help toggle, which did
// exist as a floating "?" and had only a native title=. Authoring hover-help
// prose for controls that have none is Stage M's job and is NOT done here —
// these three are the controls this version itself adds or moves.
//
// PARAMETERISED ENTRIES carry {token} placeholders substituted by tr()'s `vars`
// argument. They are NOT template literals: the table is inert data, evaluated
// once at module load, so a value interpolated here would be frozen at whatever
// the language happened to be then. A var VALUE that is itself a key resolves to
// that key's localized title, which is what lets ONE static TIP_BINDINGS row per
// band render the band's name in the current language.
//
// ALL FRENCH IS MACHINE-DRAFTED AND FLAGGED `reviewed: false`. No native speaker
// has read it. `node scripts/check-i18n.js` prints the worklist.
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

    // ── The settings popover (v1.18.0) ──────────────────────────────────────
    // New controls, new copy. The hover-help toggle moves in here from the
    // floating "?" that sat over the grid's bottom-right corner: one place for
    // the two things that decide what the hover help says and whether it says
    // it at all.
    'settings': {
        en: { t: 'Settings',
              b: 'Choose the language of this interface and whether hover help appears. Both choices are remembered with the session.' },
        fr: { t: 'Réglages',
              b: 'Choisir la langue de cette interface et l’affichage de l’aide au survol. Les deux choix sont conservés avec la session.',
              reviewed: false },
    },
    'lang-select': {
        en: { t: 'Language',
              b: 'The language of this hover help and of the labels on the page. English and French are available; value readouts, note divisions and preset names stay in English.' },
        fr: { t: 'Langue',
              b: 'La langue de cette aide au survol et des libellés de la page. L’anglais et le français sont disponibles ; les valeurs affichées, les divisions rythmiques et les noms de préréglages restent en anglais.',
              reviewed: false },
    },
    'tips-toggle': {
        en: { t: 'Hover Help',
              b: 'Turns this hover help on and off. With it off, only the gear and this switch keep explaining themselves.' },
        fr: { t: 'Aide au survol',
              b: 'Active ou désactive cette aide au survol. Une fois désactivée, seuls l’engrenage et ce commutateur continuent de s’expliquer.',
              reviewed: false },
    },

    // ── The grid ────────────────────────────────────────────────────────────
    // THE ONE HAND-SPLIT. v1.17.0 authored this as a single sentence with no
    // ": " to split on, so the title is new English and the body is the whole
    // original string, unchanged.
    'grid': {
        en: { t: 'Step Grid',
              b: 'Step sequencer grid. Each row is a frequency band; click cells to toggle steps on/off. Active steps highlight green during playback.' },
        fr: { t: 'Grille de pas',
              b: 'Grille du séquenceur à pas. Chaque ligne est une bande de fréquences ; cliquez sur les cases pour activer ou désactiver les pas. Les pas actifs s’allument en vert pendant la lecture.',
              reviewed: false },
    },

    // ── Band controls panel ─────────────────────────────────────────────────
    'euc-steps': {
        en: { t: 'Euclidean Steps',
              b: 'The total length of the Euclidean pattern. Steps beyond this wrap around.' },
        fr: { t: 'Pas euclidiens',
              b: 'La longueur totale du motif euclidien. Les pas au-delà repartent au début.',
              reviewed: false },
    },
    'euc-pulses': {
        en: { t: 'Euclidean Pulses',
              b: 'The number of active beats distributed as evenly as possible across the pattern length.' },
        fr: { t: 'Impulsions euclidiennes',
              b: 'Le nombre de temps actifs répartis aussi régulièrement que possible sur la longueur du motif.',
              reviewed: false },
    },
    'euc-offset': {
        en: { t: 'Euclidean Offset',
              b: 'Rotates the generated pattern by this many steps, shifting where the rhythm starts.' },
        fr: { t: 'Décalage euclidien',
              b: 'Fait tourner le motif généré de ce nombre de pas, déplaçant l’endroit où le rythme commence.',
              reviewed: false },
    },
    'euc-phase': {
        en: { t: 'Phase',
              b: 'Shifts this band\'s pattern read position by N steps. Creates phase-shifted polyrhythmic patterns between bands. Works in both Manual and Euclidean modes.' },
        fr: { t: 'Phase',
              b: 'Décale de N pas la position de lecture du motif de cette bande. Crée des motifs polyrythmiques déphasés entre les bandes. Fonctionne aussi bien en mode manuel qu’euclidien.',
              reviewed: false },
    },
    'euc-band-steps': {
        en: { t: 'Band Steps',
              b: 'Override the global step count for this band (0 = follow global). Creates polymetric loops when bands have different step counts.' },
        fr: { t: 'Pas de la bande',
              b: 'Remplace le nombre de pas global pour cette bande (0 = suivre le réglage global). Crée des boucles polymétriques lorsque les bandes ont des nombres de pas différents.',
              reviewed: false },
    },

    // ── Footer: the global controls ─────────────────────────────────────────
    'mix': {
        en: { t: 'Mix',
              b: 'Blends between the dry (unprocessed) and wet (gated) signal. At 0% you hear only dry; at 100% only the gated output.' },
        fr: { t: 'Mixage',
              b: 'Fait le fondu entre le signal direct (non traité) et le signal traité (découpé). À 0 %, on n’entend que le direct ; à 100 %, uniquement la sortie découpée.',
              reviewed: false },
    },
    'steps': {
        en: { t: 'Steps',
              b: 'Sets the number of active steps in the sequence (2-32). Higher values create longer, more complex patterns.' },
        fr: { t: 'Pas',
              b: 'Règle le nombre de pas actifs dans la séquence (2-32). Les valeurs élevées donnent des motifs plus longs et plus complexes.',
              reviewed: false },
    },
    'rate': {
        en: { t: 'Rate',
              b: 'Sets the tempo-synced note division for each step. Smaller values (1/32) create faster patterns; larger values (1/1) create slower ones. T = triplet, D = dotted.' },
        fr: { t: 'Vitesse',
              b: 'Règle la division rythmique synchronisée au tempo pour chaque pas. Les petites valeurs (1/32) donnent des motifs rapides ; les grandes (1/1), des motifs lents. T = triolet, D = pointée.',
              reviewed: false },
    },
    'swing': {
        en: { t: 'Swing',
              b: 'Delays every other step to create a shuffle/groove feel. At 0% timing is straight; higher values push odd steps later.' },
        fr: { t: 'Swing',
              b: 'Retarde un pas sur deux pour donner un balancement de type shuffle. À 0 %, la mise en place est droite ; en montant, les pas impairs sont repoussés.',
              reviewed: false },
    },
    'attack': {
        en: { t: 'Attack',
              b: 'Fade-in time when a step turns ON (0-500ms). Lower values create sharper onsets; higher values create gentle swells.' },
        fr: { t: 'Attaque',
              b: 'Durée du fondu d’entrée quand un pas s’active (0-500 ms). Les valeurs basses donnent des attaques franches ; les valeurs hautes, des montées douces.',
              reviewed: false },
    },
    'release': {
        en: { t: 'Release',
              b: 'Fade-out time when a step turns OFF (0-500ms). Lower values create sharp cuts; higher values create gentle tails.' },
        fr: { t: 'Relâchement',
              b: 'Durée du fondu de sortie quand un pas se désactive (0-500 ms). Les valeurs basses donnent des coupures nettes ; les valeurs hautes, des extinctions douces.',
              reviewed: false },
    },

    // ── Band display names ──────────────────────────────────────────────────
    // Label-only entries with an EMPTY body, exactly the shape
    // O-MultiBandCompressor uses. They serve two jobs at once: they are the
    // caption inside each band's label cell (bound by assigning dataset.i18n in
    // createBandRow, because the cell is built at runtime and cannot carry the
    // attribute in the markup), AND they are the VALUES that TIP_BINDINGS passes
    // as `vars.band` — tr() resolves a var value that is itself a key to that
    // key's localized title, so ONE static binding per band renders correctly in
    // both languages. Storing the already-localized name in TIP_BINDINGS instead
    // would pin every band tip to whichever language loaded the module.
    //
    // SUB is `sameAsEn` on purpose: it is the word a French control room uses.
    'bandName.sub':  { en: { t: 'SUB',  b: '' }, fr: { t: 'SUB',    b: '', reviewed: false, sameAsEn: true } },
    'bandName.low':  { en: { t: 'LOW',  b: '' }, fr: { t: 'GRAVE',  b: '', reviewed: false } },
    'bandName.mid':  { en: { t: 'MID',  b: '' }, fr: { t: 'MÉDIUM', b: '', reviewed: false } },
    'bandName.high': { en: { t: 'HIGH', b: '' }, fr: { t: 'AIGU',   b: '', reviewed: false } },

    // ── Per-band controls ───────────────────────────────────────────────────
    // Identical in all four bands, so the wording lives here once and the band
    // name is composed in through {band}. Four of the nine have no {band} at all
    // — v1.17.0 authored them without one and the English is moved, not
    // rewritten.
    'band.label': {
        en: { t: '{band} Band',
              b: 'Shows the frequency range for this band. Frequencies are set by the crossover sliders between bands.' },
        fr: { t: 'Bande {band}',
              b: 'Affiche la plage de fréquences de cette bande. Les fréquences sont fixées par les curseurs de coupure entre les bandes.',
              reviewed: false },
    },
    'band.mute': {
        en: { t: 'Mute',
              b: 'Bypass the {band} band sequencer (pass audio through unaffected).' },
        fr: { t: 'Muet',
              b: 'Contourne le séquenceur de la bande {band} (le signal passe sans être traité).',
              reviewed: false },
    },
    'band.solo': {
        en: { t: 'Solo',
              b: 'Mute all other bands so only {band} is sequenced. Click again to unsolo.' },
        fr: { t: 'Solo',
              b: 'Rend muettes toutes les autres bandes pour ne séquencer que {band}. Cliquez à nouveau pour annuler.',
              reviewed: false },
    },
    'band.clear': {
        en: { t: 'Clear',
              b: 'Resets all steps in this band to OFF.' },
        fr: { t: 'Effacer',
              b: 'Remet tous les pas de cette bande à l’arrêt.',
              reviewed: false },
    },
    'band.random': {
        en: { t: 'Random',
              b: 'Fills steps with a random pattern (50% probability per step).' },
        fr: { t: 'Aléatoire',
              b: 'Remplit les pas avec un motif aléatoire (50 % de probabilité par pas).',
              reviewed: false },
    },
    'band.rate': {
        en: { t: '{band} Rate',
              b: 'Override the global rate for this band. "Global" follows the main Rate knob. Set a specific division for polymetric sequencing.' },
        fr: { t: 'Vitesse {band}',
              b: 'Remplace la vitesse globale pour cette bande. « Global » suit le réglage Vitesse principal. Choisissez une division précise pour un séquençage polymétrique.',
              reviewed: false },
    },
    'band.mix': {
        en: { t: '{band} Mix',
              b: 'Controls how much the volume drops on OFF steps. At 100%, OFF steps are fully silent. At 0%, no gating occurs.' },
        fr: { t: 'Mixage {band}',
              b: 'Détermine de combien le volume baisse sur les pas inactifs. À 100 %, les pas inactifs sont totalement silencieux. À 0 %, aucun découpage n’a lieu.',
              reviewed: false },
    },
    'band.mode': {
        en: { t: 'Mode',
              b: 'Click to toggle between Manual (draw your own pattern) and Euclidean (algorithmically generated rhythm).' },
        fr: { t: 'Mode',
              b: 'Cliquez pour basculer entre Manuel (vous dessinez le motif) et Euclidien (rythme généré par algorithme).',
              reviewed: false },
    },
    'band.expand': {
        en: { t: 'Expand',
              b: 'Opens the band controls panel (Phase, Steps, and Euclidean settings).' },
        fr: { t: 'Déplier',
              b: 'Ouvre le panneau de réglages de la bande (Phase, Pas et réglages euclidiens).',
              reviewed: false },
    },

    // ── The crossover and boundary sliders ──────────────────────────────────
    'freq-low': {
        en: { t: 'Low Boundary',
              b: 'Sets the lowest frequency included in processing. Frequencies below this are unaffected.' },
        fr: { t: 'Limite grave',
              b: 'Fixe la fréquence la plus basse incluse dans le traitement. Les fréquences en dessous ne sont pas affectées.',
              reviewed: false },
    },
    'crossover-1': {
        en: { t: 'Crossover 1',
              b: 'Split point between Sub and Low bands. Drag to adjust where sub frequencies end and low frequencies begin.' },
        fr: { t: 'Coupure 1',
              b: 'Point de séparation entre les bandes Sub et Grave. Faites glisser pour ajuster où finissent les subgraves et où commencent les graves.',
              reviewed: false },
    },
    'crossover-2': {
        en: { t: 'Crossover 2',
              b: 'Split point between Low and Mid bands. Drag to adjust the frequency boundary.' },
        fr: { t: 'Coupure 2',
              b: 'Point de séparation entre les bandes Grave et Médium. Faites glisser pour ajuster la limite de fréquence.',
              reviewed: false },
    },
    'crossover-3': {
        en: { t: 'Crossover 3',
              b: 'Split point between Mid and High bands. Drag to adjust where mid frequencies end and highs begin.' },
        fr: { t: 'Coupure 3',
              b: 'Point de séparation entre les bandes Médium et Aigu. Faites glisser pour ajuster où finissent les médiums et où commencent les aigus.',
              reviewed: false },
    },
    'freq-high': {
        en: { t: 'High Boundary',
              b: 'Sets the highest frequency included in processing. Frequencies above this are unaffected.' },
        fr: { t: 'Limite aiguë',
              b: 'Fixe la fréquence la plus haute incluse dans le traitement. Les fréquences au-dessus ne sont pas affectées.',
              reviewed: false },
    },
});

// ============================================================================
// LABELS — the on-page text (v1.18.0, canon v2)
// ============================================================================
//
// I18N above is HOVER-HELP copy: a title and a body rendered into a wrapping
// 220 px tooltip. LABELS is ON-PAGE copy: one string dropped into a cell that
// mostly does not wrap. They are different problems and this table keeps them
// apart on purpose.
//
// ── THE REUSE RULE ─────────────────────────────────────────────────────────
// trLabel() falls back to I18N when a key is absent here, so a control whose
// tooltip TITLE already IS its caption carries ONE key. Five do on this page —
// `settings` names the gear, and the four `bandName.*` entries are both the
// band captions and the tip {band} tokens. None of those appears below.
//
// It is deliberately NOT used where only the English matches. `mix` and `steps`
// and `rate` are keyed BOTH here and in I18N, because a tooltip title is allowed
// to grow into a phrase while a 60 px footer caption is not: reusing there would
// make the next tooltip copy edit a silent change to a control's caption.
//
// ── ENGLISH WAS MOVED, NOT RE-TYPED ────────────────────────────────────────
// Every en below is what index.html or js/app.js carried through v1.17.0, taken
// from scripts/i18n-extract.js's inventory rather than transcribed.
//
// ── FRENCH IS SIZED, NOT SHRUNK ────────────────────────────────────────────
// D-04 forbids an auto-shrink font and a short-variant fallback: exactly ONE
// French string per key, and nothing chooses between variants at runtime.
// 850 x 550 is a mid-size frame, but the band label cell is a fixed 80 px grid
// column sharing its top row with two 16 px buttons — see CHANGELOG v1.18.0 for
// what had to move.
//
// ALL FRENCH IS MACHINE-DRAFTED, `reviewed: false`. No native speaker has read
// it. `node scripts/check-i18n.js` prints the worklist, LABELS included.
// ============================================================================

export const LABELS = Object.freeze({

    // ── Header ──────────────────────────────────────────────────────────────
    'label.tagline':      { en: { t: 'Frequency Band Step Sequencer' },
                            fr: { t: 'Séquenceur à pas par bandes de fréquences', reviewed: false } },

    // ── Preset bar ──────────────────────────────────────────────────────────
    'label.load':         { en: { t: 'Load' }, fr: { t: 'Ouvrir', reviewed: false } },
    'label.save':         { en: { t: 'Save' }, fr: { t: 'Enreg.', reviewed: false } },
    // Written by setLabel() from showPresetDropdown(), which through v1.17.0
    // built this row with innerHTML and a markup fragment. It is createElement +
    // setLabel now: assertion 12 reports a raw prose write inside a template
    // string, and no I18N_EXEMPT entry could cover it, because an exemption
    // lives in this file where assertion 9 forbids that character.
    'label.noPresets':    { en: { t: 'No presets' }, fr: { t: 'Aucun préréglage', reviewed: false } },

    // ── The settings popover ────────────────────────────────────────────────
    'label.language':     { en: { t: 'Language' },   fr: { t: 'Langue',        reviewed: false } },
    'label.hoverHelp':    { en: { t: 'Hover help' }, fr: { t: 'Aide',          reviewed: false } },
    // The two faces of the hover-help switch. KEYS through setLabel(), not
    // literals: a literal holds one string, so switching to French mid-session
    // would restore an English "On". Written from an if/else with two literal
    // keys, never a ternary inside the call — check-i18n assertion 13.
    'ui.on':              { en: { t: 'On' },  fr: { t: 'Oui', reviewed: false } },
    'ui.off':             { en: { t: 'Off' }, fr: { t: 'Non', reviewed: false } },

    // ── Band controls panel ─────────────────────────────────────────────────
    // ONE key with a {token}, not two. The panel is display:none at rest, and
    // openEuclideanPanel() re-keys it per band through setLabel with
    // vars = { band: 'bandName.<n>' } every time it opens — so the markup's
    // data-i18n-vars only ever paints the never-seen resting state, and is
    // present so the resting markup carries no surviving {band} placeholder.
    'label.bandControls': { en: { t: '{band} Band Controls' },
                            fr: { t: 'Réglages de la bande {band}', reviewed: false } },
    'label.pulses':       { en: { t: 'Pulses' }, fr: { t: 'Impulsions', reviewed: false } },
    'label.offset':       { en: { t: 'Offset' }, fr: { t: 'Décalage',   reviewed: false } },
    'label.phase':        { en: { t: 'Phase' },  fr: { t: 'Phase',      reviewed: false, sameAsEn: true } },

    // ── Captions shared by the footer row and the panel ─────────────────────
    // `label.steps` is used FIVE times: the footer caption, the panel's
    // Euclidean-length caption and the panel's band-step caption. Same English
    // word, same French word, one key — not three copies that could drift.
    'label.steps':        { en: { t: 'Steps' },   fr: { t: 'Pas',     reviewed: false } },
    'label.mix':          { en: { t: 'Mix' },     fr: { t: 'Mixage',  reviewed: false } },
    'label.rate':         { en: { t: 'Rate' },    fr: { t: 'Vitesse', reviewed: false } },
    'label.swing':        { en: { t: 'Swing' },   fr: { t: 'Swing',   reviewed: false, sameAsEn: true } },
    'label.attack':       { en: { t: 'Attack' },  fr: { t: 'Attaque', reviewed: false } },
    // The full word, not an abbreviation: .control is `flex: 1`, i.e. flex-basis
    // ZERO, so all six footer cells are 121.7px wide whatever their caption says
    // and the caption cannot move its neighbours. Measured, not assumed — the
    // trap in the other direction (a flex:1 1 auto row where the basis IS the
    // content) is what makes a "fixed" diff come back byte-identical.
    'label.release':      { en: { t: 'Release' }, fr: { t: 'Relâchement', reviewed: false } },

    // ── The band mode caption ───────────────────────────────────────────────
    // Both faces of a clickable caption a state updater rewrites, which is
    // exactly where the dataset.label mirror earns its keep. They are NOT value
    // mirrors under D-01: the per-band euclidean switch is an
    // AudioParameterBool, so no automation lane ever shows either of these two
    // words, and translating them cannot make the page and the host disagree.
    'label.manual':       { en: { t: 'Manual' },    fr: { t: 'Manuel',    reviewed: false } },
    'label.euclidean':    { en: { t: 'Euclidean' }, fr: { t: 'Euclidien', reviewed: false } },

    // ── Accessible names ────────────────────────────────────────────────────
    // An aria-label is user-visible text by any definition that matters — it is
    // the accessible NAME, and a screen reader in French reading an English name
    // is the same failure as a French page with an English caption. None has a
    // rendered box, so none is a geometry risk.
    //
    // Seven of these replace a native title= that v1.17.0 carried. Contract §4
    // DELETES a native title rather than localizing it: on an element that also
    // has a data-tip it renders a second, untranslated OS tooltip competing with
    // the measure-then-pin renderer, and check-i18n assertion 11 now fails on
    // any that survive.
    'aria.presetPrev':    { en: { t: 'Previous preset' },       fr: { t: 'Préréglage précédent',          reviewed: false } },
    'aria.presetNext':    { en: { t: 'Next preset' },           fr: { t: 'Préréglage suivant',            reviewed: false } },
    'aria.presetName':    { en: { t: 'Click to see all presets' },
                            fr: { t: 'Cliquer pour voir tous les préréglages', reviewed: false } },
    'aria.presetLoad':    { en: { t: 'Load preset from file' }, fr: { t: 'Ouvrir un préréglage depuis un fichier', reviewed: false } },
    'aria.presetSave':    { en: { t: 'Save current settings' }, fr: { t: 'Enregistrer les réglages actuels',       reviewed: false } },
    'aria.langSelect':    { en: { t: 'Interface language' },    fr: { t: 'Langue de l’interface',         reviewed: false } },
    'aria.helpToggle':    { en: { t: 'Toggle hover help' },     fr: { t: 'Activer ou désactiver l’aide au survol', reviewed: false } },
    'aria.closePanel':    { en: { t: 'Close band controls' },   fr: { t: 'Fermer les réglages de la bande', reviewed: false } },
    // The five accessible names on runtime-built controls whose only visible
    // text is a glyph — M, S, the empty-set sign, the die face and the
    // disclosure triangle. Bound by assigning dataset.i18nAria with a plain
    // string literal in createBandRow, which check-i18n assertion 15 counts as
    // a reference for exactly this case: an element the controller creates
    // cannot carry the attribute in the markup, and setLabel() writes
    // textContent and so cannot key an ATTRIBUTE.
    //
    // They carry no {band}: applyI18nAttributes() resolves with vars = null by
    // design, so a token here would render literally. The band name is the
    // adjacent caption in the same cell and the tip says it in full.
    'aria.mute':          { en: { t: 'Mute this band' },        fr: { t: 'Rendre cette bande muette',     reviewed: false } },
    'aria.solo':          { en: { t: 'Solo this band' },        fr: { t: 'Isoler cette bande',            reviewed: false } },
    'aria.clear':         { en: { t: 'Clear this band' },       fr: { t: 'Effacer cette bande',           reviewed: false } },
    'aria.random':        { en: { t: 'Randomize this band' },   fr: { t: 'Rendre cette bande aléatoire',  reviewed: false } },
    'aria.expand':        { en: { t: 'Open band controls' },    fr: { t: 'Ouvrir les réglages de la bande', reviewed: false } },
    'aria.bandRate':      { en: { t: 'Band rate override' },    fr: { t: 'Vitesse propre à la bande',     reviewed: false } },
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
    ['Ouaricon Frequency Pulse',
     'the product name in the h1 — a product name is never translated, and this one is also the plugin\'s registered PRODUCT_NAME in CMakeLists.txt'],

    // #presetName displays the loaded preset. The name IS the JSON filename
    // (OuariconPresetManager.h:283-285), so translating it breaks recall.
    // "Init" is the placeholder the manager overwrites on its first pass.
    ['Init', 'a factory preset name — exempt under D-02, because the name IS the JSON filename'],

    // The value mirror. #euc-band-steps-value shows the per-band steps
    // parameter, and its only non-numeric face is the word the ADJACENT
    // per-band rate AudioParameterChoice already spells "Global" in the host's
    // automation lane. Translating the readout alone would make the two Globals
    // on this page disagree about what following the global setting is called,
    // and D-01 keeps the choice entry itself English. Written from a ternary,
    // which assertion 13 would reject inside a setLabel argument anyway —
    // carried item 8's discriminator: it tracks a PARAMETER, so it is a value
    // mirror. (Parameter ids are spelled without angle brackets here on
    // purpose: assertion 9 forbids one in any string literal in this file.)
    ['Global',
     'the per-band steps value mirror AND the per-band rate AudioParameterChoice entry — English under D-01, so the readout and the automation lane agree'],
];

// [selector, key] or [selector, key, wrapperSelector] or
// [selector, key, wrapperSelector, vars].
//
// The selector is the BINDING SITE. Every anchor on this page is named
// INDIVIDUALLY, because document.querySelector returns the FIRST match in
// document order — which is precisely how O-Octagon's .vunit-group tip nearly
// landed on the wrong control in Stage C, and this page has four structurally
// identical band rows.
//
// 56 anchors, 29 tip keys. The 36 per-band rows are generated from BAND_IDS so
// the four bands cannot drift apart, and each passes its own band-name KEY as
// vars.band rather than a localized string. The four bandName.* entries are the
// remaining I18N keys and are deliberately NOT bound to an anchor — they are
// captions and tip tokens, never a tip of their own.
//
// EVERY ONE OF THESE ELEMENTS EXISTS BY THE TIME applyI18n RUNS: renderGrid()
// builds the four band rows and the five divider sliders, and app.js calls it
// before initI18n(). Binding earlier would silently write onto nothing.
export const BAND_IDS = ['sub', 'low', 'mid', 'high'];

export const TIP_BINDINGS = [
    ['#gear-btn',                 'settings'],
    ['#lang-select',              'lang-select'],
    ['#tips-toggle',              'tips-toggle'],

    ['#grid-area',                'grid'],

    ['#euc-steps',                'euc-steps',      '.control-group'],
    ['#euc-pulses',               'euc-pulses',     '.control-group'],
    ['#euc-offset',               'euc-offset',     '.control-group'],
    ['#euc-phase',                'euc-phase',      '.control-group'],
    ['#euc-band-steps',           'euc-band-steps', '.control-group'],

    ['#mix',                      'mix',            '.control'],
    ['#steps',                    'steps',          '.control'],
    ['#rate',                     'rate',           '.control'],
    ['#swing',                    'swing',          '.control'],
    ['#attack',                   'attack',         '.control'],
    ['#release',                  'release',        '.control'],

    ['#divider-freq_low',         'freq-low'],
    ['#divider-crossover_1',      'crossover-1'],
    ['#divider-crossover_2',      'crossover-2'],
    ['#divider-crossover_3',      'crossover-3'],
    ['#divider-freq_high',        'freq-high'],

    ...BAND_IDS.flatMap((band, i) => [
        ['#band-label-' + i, 'band.label',  null, { band: 'bandName.' + band }],
        ['#mute-' + i,       'band.mute',   null, { band: 'bandName.' + band }],
        ['#solo-' + i,       'band.solo',   null, { band: 'bandName.' + band }],
        ['#clear-' + i,      'band.clear'],
        ['#random-' + i,     'band.random'],
        ['#band-rate-' + i,  'band.rate',   null, { band: 'bandName.' + band }],
        ['#band-mix-cell-' + i, 'band.mix', null, { band: 'bandName.' + band }],
        ['#mode-' + i,       'band.mode'],
        ['#expand-' + i,     'band.expand'],
    ]),
];

export function tr(key, lang, vars) {
    const entry = I18N[key];
    if (!entry) { console.warn(`i18n: missing key ${key}`); return { t: key, b: '' }; }
    const s = entry[lang] || entry.en;

    // A var VALUE that is itself an I18N key resolves to that key's localized
    // title; anything else is used literally. That is what lets the four band
    // rows above be STATIC data and still render the band name in the current
    // language — TIP_BINDINGS is evaluated once at module load, so a localized
    // string stored there would be frozen at the load-time language.
    const resolve = (v) => {
        const nested = I18N[v];
        return nested ? String((nested[lang] || nested.en).t) : String(v);
    };

    const sub = (v) => vars
        ? String(v).replace(/\{(\w+)\}/g, (m, n) => (n in vars ? resolve(vars[n]) : m))
        : String(v);

    return { t: sub(s.t), b: sub(s.b) };
}
