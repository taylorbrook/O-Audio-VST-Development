/*
   This file is part of O-MultiBandCompressor, an Ouaricon Audio plugin.
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

// ── v1.11.1: FRENCH QA PASS (Stage N, 2026-08-31) ──────────────────────────
// Every fr entry read against its en and against scripts/i18n-fr-glossary.js.
// Changed: 34 strings across 31 entries (10 terminology, 10 typography,
// 6 grammar/register, 8 meaning/idiom). sameAsEn: kept 5, ADDED 1
// (label.ratio — Ratio is the glossary's French, so the caption is now a
// declared straight copy); translated 0. termNote exemptions: 1 (label.msMid).
// Left as drafted: the rest. reviewed: false throughout — no native speaker yet.
// Lint 30 findings → 1; the survivor is label.makeup, recorded at its entry.
//
// The decisions a later reader needs, and nothing else:
//
//   · Relâche → Relâch. and Dériv. → Contour. are the glossary's ABBREVIATIONS,
//     taken because both roots are measurably too wide (66.88 px in a 60.16 px
//     grid track; 103.25 px of button in a row with 5.31 px of slack). Both
//     numbers are at their entries. Relâch. is NARROWER than the Relâche the
//     v1.11.0 header defended on width.
//   · Compens. is NOT a glossary form and the lint still reports it. The
//     glossary lists no abbreviation for makeup and the root re-deals the knob
//     grid, so the term is reported for the glossary to grow rather than
//     exempted — a width constraint is not a contextual exemption.
//   · label.msMid keeps "Mid" with a termNote: this is the M/S encoding, and
//     the glossary's Médium is the BAND term, already spent on band.lomid /
//     band.himid two rows away on the same page.
//   · The ms-mode body now names the FRENCH option faces the page shows
//     (« Aucun », « Les deux »); through v1.11.0 it named the English ones, so the
//     tip described a control the French UI does not have.
//   · "mix stage" is the Mix knob's stage, not the mixing process: output-gain
//     and output-meter said "mixage", which reads as a DAW mixdown.
//   · Register: descriptive sentences are third-person ("Adoucit", "Empêche").
//     band.solo and band.sc-listen opened with "Écoute", which is either a
//     tu-imperative (the glossary says vous) or a claim that a button hears.
//     Both rewritten third-person; user-addressed instructions stay vous-
//     imperative ("Faites glisser", "déplacez-les"), which is what the rest of
//     the file already does.
//   · Dosage is kept in the mix body. It is absent from FORBIDDEN_IN_PROSE on
//     purpose — it is forbidden as a LABEL for Mix, and as prose it names the
//     action rather than the control, which the caption "Mix" already names.
//   · U+00A0 is now load-bearing: 42 of them, all inside fr string values.
//     `grep -n $'\xc2\xa0' i18n.js | grep -v "t: '\|b: '"` is empty.
//
/*
  ==============================================================================

    i18n.js — O-MultiBandCompressor hover-help copy, English + French.

    An ES module that EXPORTS ONLY. It must never self-execute: a bare
    top-level statement here throws out of module evaluation and takes every
    later initializer on the page with it. That is not hypothetical — v1.4.0
    shipped initializeTooltips() in the eager top-level init and the resulting
    TDZ ReferenceError silently killed initializeCrossoverDrag(), untouched and
    working code, while the C++ build and auval both passed.
    scripts/check-i18n.js assertion 7 enforces the rule.

    FILENAME: no hyphen. juce_add_binary_data STRIPS hyphens from filenames, so
    a second file named i18n-fr.js would have to be reached as the BinaryData
    symbol i18nfr_js. One combined file for both languages sidesteps it.

    Copy is `textContent` on every path — never innerHTML. The renderer builds
    the tip with createElement + textContent, and check-i18n assertion 9 rejects
    any innerHTML reference here and any string literal containing `<`. A line
    break, if ever needed, is `\n` plus CSS white-space: pre-line, never markup.

  ==============================================================================
*/

export const LANGUAGES = ['en', 'fr'];

// The band ids used throughout app.js, and the display-name key each one maps
// to. The names are i18n keys rather than literals because they are COMPOSED
// into every per-band tip title ("Low — Threshold"), so a French UI has to
// compose the French name.
export const BAND_IDS = ['low', 'lomid', 'himid', 'high'];

// [controlSuffix, wrapperSelector | null]. The element id is `#<band>-<suffix>`.
// The wrapper widens the hover target to the surrounding group so the label and
// the value readout trigger the tip as well as the control itself.
export const BAND_CONTROLS = [
    ['threshold', '.knob-control'],
    ['ratio',     '.knob-control'],
    ['attack',    '.knob-control'],
    ['release',   '.knob-control'],
    ['knee',      '.knob-control'],
    ['makeup',    '.knob-control'],
    ['solo',      null],
    ['bypass',    null],
    ['sc-listen', null],
    ['peak-rms',  '.knob-control'],
    ['sc-hpf',    '.knob-control'],
    ['sc-lpf',    '.knob-control'],
];

// Gain-reduction meter element ids, keyed by band id.
//
// Object.freeze() rather than a bare `{...}` literal, here and on I18N below,
// for two reasons. It says out loud that this module is inert data that nothing
// may mutate at runtime — and it keeps the export a SINGLE top-level statement.
// A statement written `export const X = {...};` closes its brace at depth zero,
// which segments the trailing `;` off as an empty statement of its own, and
// check-i18n.js assertion 7 correctly reports that as a top-level statement
// outside an export declaration. Wrapping in a call keeps the brace nested.
export const BAND_GR_METERS = Object.freeze({ low: 'grLow', lomid: 'grLomid', himid: 'grHimid', high: 'grHigh' });

// key -> { en: {t, b}, fr: {t, b, reviewed} }
//   t = tooltip title, b = tooltip body.
//
//   fr.reviewed is REQUIRED and starts false. The French below is
//   machine-drafted; French that is never marked degrades silently into
//   "we shipped English in a French UI" and nobody can tell which strings a
//   native speaker already vetted. check-i18n assertion 5 enforces the flag,
//   assertion 4 rejects a French entry that is a straight copy of the English
//   unless it carries an explicit `sameAsEn: true`.
//
//   Parameterised entries carry {token} placeholders substituted by tr()'s
//   `vars` argument. They are NOT template literals — this table is inert data.
export const I18N = Object.freeze({

    // ── Global controls ──────────────────────────────────────────────────
    'input-gain': {
        en: { t: 'Input Gain',
              b: 'Level trim applied before the signal is split into bands. Use it to drive the compressors harder or back them off without touching the thresholds. −24 to +24 dB.' },
        fr: { t: 'Gain d’entrée',
              b: 'Correction de niveau appliquée avant la division du signal en bandes. Permet d’attaquer les compresseurs plus fort ou de les ménager sans toucher aux seuils. −24 à +24 dB.',
              reviewed: true },
    },
    'mix': {
        en: { t: 'Mix',
              b: 'Blend between the dry input and the compressed output for parallel compression. 0% is fully dry, 100% is fully compressed.' },
        fr: { t: 'Mix',
              b: 'Dosage entre le signal d’entrée direct et la sortie compressée, pour la compression parallèle. 0 % est entièrement direct, 100 % entièrement compressé.',
              reviewed: true },
    },
    'auto-makeup': {
        en: { t: 'Auto Makeup',
              b: 'Automatically compensates the level lost to gain reduction in each band, so bypassing the compressor does not jump in volume. Stacks with each band’s Makeup knob.' },
        fr: { t: 'Compensation auto',
              b: 'Compense automatiquement le niveau perdu du fait de la réduction de gain dans chaque bande, afin que le contournement du compresseur ne provoque pas de saut de volume. S’ajoute au bouton Compensation de chaque bande.',
              reviewed: true },
    },
    'ms-mode': {
        en: { t: 'Mid / Side Mode',
              b: 'Chooses what the compressors act on. Off processes left and right normally; Mid targets the centre of the image, Side the stereo edges, and Both processes them independently.' },
        fr: { t: 'Mode Mid / Side',
              b: 'Détermine ce sur quoi agissent les compresseurs. « Aucun » traite normalement la gauche et la droite ; « Mid » vise le centre de l’image, « Side » les bords stéréo, et « Les deux » les traite indépendamment.',
              reviewed: true },
    },
    'output-gain': {
        en: { t: 'Output Gain',
              b: 'Final level trim, applied after the mix stage. −24 to +24 dB.' },
        fr: { t: 'Gain de sortie',
              b: 'Correction de niveau finale, appliquée après l’étage Mix. −24 à +24 dB.',
              reviewed: true },
    },
    'input-meter': {
        en: { t: 'Input Meter',
              b: 'Level entering the plugin, averaged across both channels and measured before the input gain trim.' },
        fr: { t: 'Vumètre d’entrée',
              b: 'Niveau entrant dans le plugin, moyenné sur les deux canaux et mesuré avant la correction de gain d’entrée.',
              reviewed: true },
    },
    'output-meter': {
        en: { t: 'Output Meter',
              b: 'Level leaving the plugin, measured after mix and output gain.' },
        fr: { t: 'Vumètre de sortie',
              b: 'Niveau sortant du plugin, mesuré après le Mix et le gain de sortie.',
              reviewed: true },
    },
    'spectrum': {
        en: { t: 'Spectrum Analyzer',
              b: 'Real-time input spectrum, 20 Hz to 20 kHz on a logarithmic scale. Drag the vertical lines to move the crossovers.' },
        fr: { t: 'Analyseur de spectre',
              b: 'Spectre d’entrée en temps réel, de 20 Hz à 20 kHz sur une échelle logarithmique. Faites glisser les lignes verticales pour déplacer les points de coupure.',
              reviewed: true },
    },
    'crossover1': {
        en: { t: 'Crossover 1',
              b: 'Split point between the Low and Low-Mid bands. Drag left or right to move it; the two band headers update as you go. 20 Hz to 500 Hz.' },
        fr: { t: 'Coupure 1',
              b: 'Point de séparation entre les bandes Grave et Bas-médium. Faites-le glisser à gauche ou à droite ; les deux en-têtes de bande se mettent à jour au fur et à mesure. 20 Hz à 500 Hz.',
              reviewed: true },
    },
    'crossover2': {
        en: { t: 'Crossover 2',
              b: 'Split point between the Low-Mid and High-Mid bands. Drag left or right to move it. 200 Hz to 5 kHz.' },
        fr: { t: 'Coupure 2',
              b: 'Point de séparation entre les bandes Bas-médium et Haut-médium. Faites-le glisser à gauche ou à droite. 200 Hz à 5 kHz.',
              reviewed: true },
    },
    'crossover3': {
        en: { t: 'Crossover 3',
              b: 'Split point between the High-Mid and High bands. Drag left or right to move it. 2 kHz to 16 kHz.' },
        fr: { t: 'Coupure 3',
              b: 'Point de séparation entre les bandes Haut-médium et Aigu. Faites-le glisser à gauche ou à droite. 2 kHz à 16 kHz.',
              reviewed: true },
    },

    // ── Band display names ───────────────────────────────────────────────
    // Label-only entries: they are never bound to an element, they are the
    // VALUES that TIP_BINDINGS passes as `vars.band`, and tr() resolves a var
    // value that is itself an I18N key to that key's localized title. `b` is
    // deliberately empty — nothing renders a body for a band name.
    'band.low':   { en: { t: 'Low',      b: '' }, fr: { t: 'Grave',       b: '', reviewed: true } },
    'band.lomid': { en: { t: 'Low-Mid',  b: '' }, fr: { t: 'Bas-médium',  b: '', reviewed: true } },
    'band.himid': { en: { t: 'High-Mid', b: '' }, fr: { t: 'Haut-médium', b: '', reviewed: true } },
    'band.high':  { en: { t: 'High',     b: '' }, fr: { t: 'Aigu',        b: '', reviewed: true } },

    // ── Per-band controls ────────────────────────────────────────────────
    // Identical in all four bands, so the wording lives here once and the band
    // name is composed in through {band}. The joining em-dash is a literal.
    'band.threshold': {
        en: { t: '{band} — Threshold',
              b: 'The level at which this band starts to compress. Anything above it is pulled down by the Ratio. −60 to 0 dB.' },
        fr: { t: '{band} — Seuil',
              b: 'Le niveau à partir duquel cette bande commence à compresser. Tout ce qui le dépasse est ramené vers le bas selon le Ratio. −60 à 0 dB.',
              reviewed: true },
    },
    'band.ratio': {
        en: { t: '{band} — Ratio',
              b: 'How firmly the band is compressed above the threshold. 1:1 leaves it untouched; 20:1 is effectively limiting.' },
        fr: { t: '{band} — Ratio',
              b: 'Fermeté de la compression au-dessus du seuil. 1:1 laisse la bande intacte ; 20:1 revient à de la limitation.',
              reviewed: true },
    },
    'band.attack': {
        en: { t: '{band} — Attack',
              b: 'How quickly compression engages once the signal crosses the threshold. Fast settings clamp transients, slow settings let them through. 0.1 to 200 ms.' },
        fr: { t: '{band} — Attaque',
              b: 'Rapidité d’engagement de la compression une fois le seuil franchi. Les réglages rapides brident les transitoires, les lents les laissent passer. 0,1 à 200 ms.',
              reviewed: true },
    },
    'band.release': {
        en: { t: '{band} — Release',
              b: 'How quickly compression lets go once the signal falls back below the threshold. Too fast can pump, too slow can choke the band. 10 to 2000 ms.' },
        fr: { t: '{band} — Relâchement',
              b: 'Rapidité avec laquelle la compression relâche une fois le signal redescendu sous le seuil. Trop rapide, elle pompe ; trop lente, elle étouffe la bande. 10 à 2000 ms.',
              reviewed: true },
    },
    'band.knee': {
        en: { t: '{band} — Knee',
              b: 'Softens the onset of compression around the threshold. 0 dB is a hard knee that grabs abruptly; 24 dB eases in very gradually.' },
        fr: { t: '{band} — Coude',
              b: 'Adoucit l’entrée en compression autour du seuil. 0 dB donne un coude dur qui saisit brusquement ; 24 dB amène la compression très progressivement.',
              reviewed: true },
    },
    'band.makeup': {
        en: { t: '{band} — Makeup',
              b: 'Manual gain applied to this band after compression, to restore what gain reduction took away. −12 to +24 dB.' },
        fr: { t: '{band} — Compensation',
              b: 'Gain manuel appliqué à cette bande après compression, pour restituer ce que la réduction de gain a retiré. −12 à +24 dB.',
              reviewed: true },
    },
    'band.solo': {
        en: { t: '{band} — Solo',
              b: 'Hear this band on its own — the other three are muted. Useful for checking where a crossover should sit.' },
        fr: { t: '{band} — Solo',
              b: 'Isole cette bande — les trois autres sont coupées. Pratique pour vérifier où placer un point de coupure.',
              reviewed: true },
    },
    'band.bypass': {
        en: { t: '{band} — Bypass',
              b: 'Pass this band through uncompressed. The crossover filtering still applies, so the band stays in phase with the others.' },
        fr: { t: '{band} — Contournement',
              b: 'Laisse passer cette bande sans la compresser. Le filtrage de coupure reste appliqué, la bande reste donc en phase avec les autres.',
              reviewed: true },
    },
    'band.sc-listen': {
        en: { t: '{band} — Sidechain Listen',
              b: 'Monitor the detector signal driving this band’s compressor, including its sidechain filtering. This is what the compressor "hears", not what it outputs.' },
        fr: { t: '{band} — Écoute du sidechain',
              b: 'Permet d’écouter le signal du détecteur qui pilote le compresseur de cette bande, filtrage de sidechain compris. C’est ce que le compresseur « entend », pas ce qu’il produit.',
              reviewed: true },
    },
    'band.peak-rms': {
        en: { t: '{band} — Peak / RMS',
              b: 'Blends how the band’s level is measured. Peak reacts to individual transients and suits de-essing and plosive control; RMS averages over 10 ms and suits glue and level-riding.' },
        fr: { t: '{band} — Crête / RMS',
              b: 'Dose la façon dont le niveau de la bande est mesuré. Crête réagit aux transitoires isolés et convient au dé-essage et au contrôle des plosives ; RMS moyenne sur 10 ms et convient au liant et au nivellement.',
              reviewed: true },
    },
    'band.sc-hpf': {
        en: { t: '{band} — Sidechain High-Pass',
              b: 'High-passes the detector only — the audio itself is untouched. Keeps low energy from triggering gain reduction, for example so subsonic rumble does not duck a whole band. Fully left is Off.' },
        fr: { t: '{band} — Passe-haut du sidechain',
              b: 'Filtre en passe-haut le détecteur uniquement — l’audio lui-même n’est pas touché. Empêche l’énergie grave de déclencher la réduction de gain, par exemple pour qu’un grondement infrasonore ne fasse pas plonger toute une bande. Complètement à gauche : Off.',
              reviewed: true },
    },
    'band.sc-lpf': {
        en: { t: '{band} — Sidechain Low-Pass',
              b: 'Low-passes the detector only — the audio itself is untouched. Narrows what the band responds to, for example keeping cymbals and air from holding a de-esser down. Fully left is Off.' },
        fr: { t: '{band} — Passe-bas du sidechain',
              b: 'Filtre en passe-bas le détecteur uniquement — l’audio lui-même n’est pas touché. Restreint ce à quoi la bande réagit, par exemple pour éviter que cymbales et air ne maintiennent un dé-esseur enfoncé. Complètement à gauche : Off.',
              reviewed: true },
    },
    'band.gr': {
        en: { t: '{band} — Gain Reduction',
              b: 'How much this band is being compressed right now. The bar fills as gain reduction deepens, up to −24 dB.' },
        fr: { t: '{band} — Réduction de gain',
              b: 'Quantité de compression appliquée à cette bande en ce moment. La barre se remplit à mesure que la réduction de gain s’accentue, jusqu’à −24 dB.',
              reviewed: true },
    },
    'band.range': {
        en: { t: '{band} — Frequency Range',
              b: 'The span this band processes. It follows the crossover handles in the analyzer above, so drag them to retune it.' },
        fr: { t: '{band} — Plage de fréquences',
              b: 'L’étendue traitée par cette bande. Elle suit les poignées de coupure de l’analyseur ci-dessus ; déplacez-les pour la réajuster.',
              reviewed: true },
    },

    // ── Settings popover ─────────────────────────────────────────────────
    'settings': {
        en: { t: 'Settings',
              b: 'Interface preferences — the language of this hover help, and whether hover help appears at all.' },
        fr: { t: 'Réglages',
              b: 'Préférences d’interface — la langue de ces infobulles, et leur affichage ou non.',
              reviewed: true },
    },
    // v1.11.0: through v1.10.0 this entry told the user, in both languages,
    // that control labels stay in English. That is now false — the labels are
    // localized too. Rewritten to say what is true, INCLUDING the half that
    // stayed true: value readouts are English in both languages (D-03), so a
    // knob still reads `1.5 kHz` and the inline value entry still parses what
    // it displays.
    'lang-select': {
        en: { t: 'Language',
              b: 'Language of this hover help and of the labels on the page. The choice is remembered with the session and is not carried by presets. Value readouts and preset names stay in English.' },
        fr: { t: 'Langue',
              b: 'Langue de ces infobulles et des libellés de la page. Le choix est conservé avec la session et n’est pas transporté par les préréglages. Les valeurs affichées et les noms de préréglages restent en anglais.',
              reviewed: true },
    },
    // One key covering both states rather than a pair swapped on click: the
    // canonical applyI18n() re-renders straight from this table on every
    // language change, so a state-dependent tip written outside it would be
    // stale in the new language. The caption and aria-pressed carry the state.
    'tips-toggle': {
        en: { t: 'Hover Help',
              b: 'Turns this hover-help layer on or off for every control. While it is off, this button still explains itself, so help can always be switched back on. The setting is shared by every instance on this machine.' },
        fr: { t: 'Infobulles',
              b: 'Active ou désactive ces infobulles pour toutes les commandes. Lorsqu’elles sont désactivées, ce bouton continue de s’expliquer, afin de pouvoir toujours les réactiver. Ce réglage est partagé par toutes les instances sur cette machine.',
              reviewed: true },
    },
});

// ============================================================================
// LABELS — the on-page text (v1.11.0, canon v2)
// ============================================================================
//
// I18N above is HOVER-HELP copy: a title and a body rendered into a wrapping
// 230 px tooltip. LABELS is ON-PAGE copy: one string dropped into a grid cell
// that does not wrap. They are different problems and this table keeps them
// apart on purpose.
//
// ── THE REUSE RULE ─────────────────────────────────────────────────────────
// trLabel() falls back to I18N when a key is absent here, so a control whose
// tooltip TITLE already IS its caption carries ONE key. That fallback is used
// ONLY where the string is identical in BOTH languages: the four band titles
// (band.low … band.high), the Language caption (lang-select), the gear and the
// popover's accessible name (settings), and the footer Mix caption (mix). None
// of those appears below.
//
// It is deliberately NOT used where only the English matches. The Release knob
// caption is "Relâch." here while its tip title is "{band} — Relâchement":
// the root is 66.88 px inside a 60.16 px grid track, and the tip has a 230 px
// cap to wrap into that the knob cell does not. Same for Makeup — "Compens."
// against a tip title of "{band} — Compensation".
// Reusing the key there would make every future edit to a tooltip a silent
// geometry change to a control.
//
// ── ENGLISH WAS MOVED, NOT RE-TYPED ────────────────────────────────────────
// Every en below is what index.html carried through v1.10.0, taken from
// scripts/i18n-extract.js's inventory rather than transcribed.
//
// One deliberate normalisation, recorded because it is the only place the
// markup disagreed with itself: the three band buttons authored `SOLO` as
// their text node and `data-label="Solo"` as the caption a state update writes
// back, so the page rendered "Solo" from the moment bindToggle ran. The keys
// below carry the data-label casing, which is what actually rendered, and the
// text nodes were normalised to match. `.band-button { text-transform:
// uppercase }` makes the two visually identical either way.
//
// ── FRENCH IS SIZED, NOT SHRUNK ────────────────────────────────────────────
// D-04 forbids an auto-shrink font and a short-variant fallback: there is
// exactly ONE French string per key and nothing chooses between variants at
// runtime. Four bands share a 900 px frame, so a band's knob grid is ~62 px
// per track and its button row ~61 px per button — the tightest type on this
// page. Where French did not fit, the fix was this plugin's own CSS
// (see CHANGELOG v1.11.0); where a shorter phrasing was simply the better
// French for a caption, it is noted at the entry.
//
// ALL FRENCH IS MACHINE-DRAFTED, `reviewed: false`. No native speaker has read
// it. `node scripts/check-i18n.js` prints the worklist, LABELS included.
// ============================================================================

export const LABELS = Object.freeze({

    // ── Preset bar ──────────────────────────────────────────────────────────
    // MEASURED, not chosen by ear. The header is a three-group flex row inside
    // 844 px of content width, and 367.8 px of it is the product name. English
    // SAVE + LOAD occupy 84.7 px and leave 30.2 px of distributed air; French
    // ENREGISTRER alone is 90.1 px, which wraps the title to two lines and
    // pushes the entire page down 27 px. "Enreg." is 53.9 px and "Ouvrir"
    // 56.4 px — the standard compact French forms, and the pair that fits the
    // slack this header actually has once the buttons are pinned to a
    // language-independent width (see CHANGELOG v1.11.0).
    //
    // A native reviewer should challenge these two before any other entry in
    // this table: they are the only ones where width, not meaning, picked the
    // word.
    'label.save':      { en: { t: 'Save' },  fr: { t: 'Enreg.',  reviewed: true } },
    'label.load':      { en: { t: 'Load' },  fr: { t: 'Ouvrir',  reviewed: true } },

    // ── Settings popover ────────────────────────────────────────────────────
    // Not the `tips-toggle` key: that tip's title is "Hover Help" and this
    // caption is "Hover help". The reuse rule wants both languages identical,
    // and these differ in English before French is even considered.
    'label.hoverHelp': { en: { t: 'Hover help' }, fr: { t: 'Infobulles', reviewed: true } },

    // The hover-help toggle's two faces and the Auto-MU toggle's two faces.
    // They go through setLabel(), so the element becomes a [data-i18n] element
    // and the language sweep owns it from that moment on — a state string
    // written as a raw literal is stranded in the previous language the instant
    // the selector fires, which is the bug Stage B found on this very toggle.
    //
    // "Marche" / "Arrêt" rather than "Activé" / "Désactivé": the toggle face is
    // 44 px, and this is the vocabulary a piece of hardware uses, which is the
    // register the whole panel is written in.
    'ui.on':           { en: { t: 'On' },    fr: { t: 'Marche', reviewed: true } },
    'ui.off':          { en: { t: 'Off' },   fr: { t: 'Arrêt',  reviewed: true } },

    // ── Preset dropdown, built from script ──────────────────────────────────
    'ui.noPresets':    { en: { t: 'No presets' }, fr: { t: 'Aucun préréglage', reviewed: true } },
    // The preset NAME is substituted, never translated (D-02 — the name is the
    // JSON filename). tr()/trLabel() resolve a var value that is not itself a
    // key literally, which is exactly what is wanted here.
    'ui.deleteConfirm': { en: { t: 'Delete "{name}"?' },
                          fr: { t: 'Supprimer « {name} » ?', reviewed: true } },
    'ui.delete':       { en: { t: 'Delete' }, fr: { t: 'Supprimer', reviewed: true } },
    'ui.cancel':       { en: { t: 'Cancel' }, fr: { t: 'Annuler',   reviewed: true } },

    // ── Meters ──────────────────────────────────────────────────────────────
    // Absolutely positioned and translate(-50%) centred, so neither has a
    // rendered box that can push anything.
    'label.in':        { en: { t: 'IN' },   fr: { t: 'ENT', reviewed: true } },
    'label.out':       { en: { t: 'OUT' },  fr: { t: 'SOR', reviewed: true } },

    // ── Band controls, one key each, shared by all four bands ───────────────
    'label.gr':        { en: { t: 'GR' },     fr: { t: 'RG',      reviewed: true } },
    'label.thresh':    { en: { t: 'Thresh' }, fr: { t: 'Seuil',   reviewed: true } },
    // Ratio IS the French term (glossary): a straight copy, declared as one.
    'label.ratio':     { en: { t: 'Ratio' },  fr: { t: 'Ratio',   reviewed: true, sameAsEn: true } },
    'label.attack':    { en: { t: 'Attack' }, fr: { t: 'Attaque', reviewed: true } },
    // v1.11.1: the glossary's abbreviation, and it is NARROWER than what v1.11.0
    // shipped — Relâch. 38.33 px against Relâche 41.67 px, in a 60.16 px grid
    // track (Range.selectNodeContents at the 900x640 shipping frame). The root
    // Relâchement is 66.88 px: over the track, and it re-deals the three 1fr
    // columns to 55.63/55.64/69.23, which moves every knob inside them.
    'label.release':   { en: { t: 'Release' }, fr: { t: 'Relâch.', reviewed: true } },
    'label.knee':      { en: { t: 'Knee' },   fr: { t: 'Coude',   reviewed: true } },
    // The tip title is "Compensation". MEASURED at the shipping frame it is
    // 69.23 px in a 60.16 px track, and substituting it re-deals the three 1fr
    // columns to 55.63/55.64/69.23 — every knob inside them moves. The glossary
    // lists NO abbreviation for makeup, so this caption keeps a form the glossary
    // does not carry and the Stage N lint still reports it as G1. Reported rather
    // than exempted: a width constraint is not a contextual exemption, and a
    // termNote here would be the reasoned-exemption mechanism hiding a miss.
    'label.makeup':    { en: { t: 'Makeup' }, fr: { t: 'Compens.', reviewed: true } },
    'label.detector':  { en: { t: 'Detector / Sidechain' },
                         fr: { t: 'Détecteur / Sidechain', reviewed: true } },
    // v1.11.1: the whole word. "Crê" cut a French abbreviation at a vowel, which
    // is not how French abbreviates; Crête/RMS measures 50.27 px inside the same
    // 60.16 px track and moves nothing.
    'label.pkRms':     { en: { t: 'Pk/RMS' }, fr: { t: 'Crête/RMS', reviewed: true } },
    // "Sidechain" is the term this plugin's own French tooltips use, so the
    // SC prefix survives; PH / PB are passe-haut / passe-bas.
    'label.scHpf':     { en: { t: 'SC HPF' }, fr: { t: 'SC PH',   reviewed: true } },
    'label.scLpf':     { en: { t: 'SC LPF' }, fr: { t: 'SC PB',   reviewed: true } },

    'label.solo':      { en: { t: 'Solo' },      fr: { t: 'Solo', reviewed: true, sameAsEn: true } },
    // v1.11.1: the glossary's abbreviation for Bypass. The root Contournement is
    // 89.25 px of text (103.25 px of button) and grows the whole band 188.5 →
    // 221.94 px. Contour. is 64.50 px of button, so the three-button row comes to
    // 40.22 + 64.50 + 68.47 + 10 gap = 183.19 px inside 188.50 — 5.31 px of slack,
    // the tightest measured margin on this page. A wider Windows face lands here
    // first (the standing WebView2 metrics deferral).
    'label.bypass':    { en: { t: 'Bypass' },    fr: { t: 'Contour.', reviewed: true } },
    'label.scListen':  { en: { t: 'SC Listen' }, fr: { t: 'Écoute SC', reviewed: true } },

    // ── Global controls ─────────────────────────────────────────────────────
    'label.input':     { en: { t: 'Input' },   fr: { t: 'Entrée',    reviewed: true } },
    'label.autoMu':    { en: { t: 'Auto-MU' }, fr: { t: 'Auto-Comp', reviewed: true } },
    'label.msMode':    { en: { t: 'M/S Mode' },fr: { t: 'Mode M/S',  reviewed: true } },
    'label.output':    { en: { t: 'Output' },  fr: { t: 'Sortie',    reviewed: true } },

    // The M/S mode choices. NOT `ui.off`: this one names a routing mode that is
    // not engaged, not a switch that is turned off, and French separates the
    // two where English does not. Mid and Side are the names of the encoding,
    // used untranslated in French audio work.
    'label.msOff':     { en: { t: 'Off' },  fr: { t: 'Aucun',    reviewed: true } },
    'label.msMid':     { en: { t: 'Mid' },  fr: { t: 'Mid',      reviewed: true, sameAsEn: true,
                                                  termNote: 'this Mid is the M/S ENCODING, not the frequency band — the glossary\'s Médium is the band term and is already used for band.lomid / band.himid on this same page' } },
    'label.msSide':    { en: { t: 'Side' }, fr: { t: 'Side',     reviewed: true, sameAsEn: true } },
    'label.msBoth':    { en: { t: 'Both' }, fr: { t: 'Les deux', reviewed: true } },

    // ── Accessible names ────────────────────────────────────────────────────
    // An aria-label is user-visible text by any definition that matters — it is
    // the accessible NAME, and a screen reader in French reading an English
    // name is the same failure as a French page with an English caption. None
    // has a rendered box, so none is a geometry risk.
    'aria.presetPrev':   { en: { t: 'Previous preset' }, fr: { t: 'Préréglage précédent', reviewed: true } },
    'aria.presetNext':   { en: { t: 'Next preset' },     fr: { t: 'Préréglage suivant',   reviewed: true } },
    'aria.presetList':   { en: { t: 'Presets' },         fr: { t: 'Préréglages',          reviewed: true } },
    'aria.helpToggle':   { en: { t: 'Toggle hover help' },
                           fr: { t: 'Activer ou désactiver les infobulles', reviewed: true } },
    // Static, not composed. Canon v2's attribute sweep resolves a key WITHOUT
    // vars by design, and the dropdown row this button sits in already carries
    // the preset name as its own accessible content, so the row is announced
    // before the button either way. The button additionally points at that name
    // through aria-describedby, so nothing about the composed form is lost.
    'aria.presetDelete': { en: { t: 'Delete preset' },   fr: { t: 'Supprimer le préréglage', reviewed: true } },
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
    ['O-MultiBandCompressor', 'the product name — a product name is never translated'],
    ['Ouaricon Audio',        'the company name'],

    // #presetName displays the loaded preset. The name IS the JSON filename
    // (OuariconPresetManager.h:283-285), so translating it breaks recall:
    // a session saved against "Cathedral" would not resolve "Cathédrale".
    // "Default" is the placeholder the manager overwrites on its first pass.
    ['Default', 'a factory preset name — exempt under D-02, because the name IS the JSON filename'],

    // THE ONE EXEMPTION WORTH ARGUING WITH. The two sidechain filter readouts
    // (#<band>-sc-hpf-value / #<band>-sc-lpf-value, eight spans) read "Off" at
    // the bottom of their range and a frequency everywhere else — they are
    // VALUE READOUTS, and they are EDITABLE: attachValueEntry() opens an inline
    // field over them and parseFreq() maps a typed "off" back to 0 Hz. D-03
    // forbids touching an editable readout's parser, so a French "Désactivé"
    // on screen would be a value the field that displays it cannot read back.
    // They must stay English, and they cannot carry data-i18n either: the
    // language sweep writes textContent, so a knob showing "2.0 kHz" would
    // revert to "Off" on the next language change.
    //
    // The exemption matches by TEXT, so it also covers the Auto-MU toggle face
    // and the M/S Mode "Off" option. BOTH ARE KEYED ANYWAY — ui.off and
    // label.msOff — so nothing is left English by this entry; it only means the
    // gate would not have caught it if they were not. That is the residual risk
    // and it is stated rather than hidden.
    ['Off', 'the sidechain-filter value readouts read "Off" at 0 Hz and are EDITABLE — parseFreq() maps a typed "off" back to 0, and D-03 forbids touching an editable readout parser. SCOPED to .knob-value so it exempts the readouts only, and cannot silence a caption elsewhere that happens to say Off',
     '.knob-value'],
];

// [selector, key] or [selector, key, wrapperSelector] or
// [selector, key, wrapperSelector, vars].
//
// The selector is the BINDING SITE. It has to be a selector rather than a
// key-on-the-element attribute because several tips here attach to wrappers the
// HTML does not mark up individually (.control-group, .spectrum-container,
// .knob-control), which an attribute on the control itself cannot express.
//
// `vars` is static data: a var value that is itself an I18N key is resolved by
// tr() against the CURRENT language, so one static binding renders correctly in
// every language. Storing the already-localized band name here instead would
// pin every band tip to whichever language was active when the module loaded.
export const TIP_BINDINGS = [
    ['#input-gain',           'input-gain',    '.control-group'],
    ['#mix',                  'mix',           '.control-group'],
    ['#auto-makeup',          'auto-makeup',   '.control-group'],
    ['#ms-mode',              'ms-mode',       '.control-group'],
    ['#output-gain',          'output-gain',   '.control-group'],
    ['.input-meter',          'input-meter'],
    ['.output-meter',         'output-meter'],
    ['.spectrum-container',   'spectrum'],
    ['#crossover1',           'crossover1'],
    ['#crossover2',           'crossover2'],
    ['#crossover3',           'crossover3'],

    ['#gear-btn',             'settings'],
    ['#lang-select',          'lang-select'],
    ['#tips-toggle',          'tips-toggle'],

    ...BAND_IDS.flatMap((band) => [
        ...BAND_CONTROLS.map(([control, wrapper]) =>
            ['#' + band + '-' + control, 'band.' + control, wrapper, { band: 'band.' + band }]),

        ['#' + BAND_GR_METERS[band], 'band.gr',    '.gr-meter',   { band: 'band.' + band }],
        ['#range-' + band,           'band.range', '.band-header', { band: 'band.' + band }],
    ]),
];

export function tr(key, lang, vars) {
    const entry = I18N[key];
    if (!entry) { console.warn(`i18n: missing key ${key}`); return { t: key, b: '' }; }
    const s = entry[lang] || entry.en;

    // A var VALUE that is itself an I18N key resolves to that key's localized
    // title; anything else is used literally. This is what lets TIP_BINDINGS
    // stay inert static data while still composing "Grave — Seuil" in French
    // and "Low — Threshold" in English from the same binding.
    const resolve = (v) => {
        const nested = I18N[v];
        return nested ? String((nested[lang] || nested.en).t) : String(v);
    };

    const sub = (v) => vars
        ? String(v).replace(/\{(\w+)\}/g, (m, n) => (n in vars ? resolve(vars[n]) : m))
        : String(v);

    return { t: sub(s.t), b: sub(s.b) };
}
