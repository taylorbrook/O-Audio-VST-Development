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
              b: 'Correction de niveau appliquée avant la division du signal en bandes. Permet d’attaquer les compresseurs plus fort ou de les retenir sans toucher aux seuils. −24 à +24 dB.',
              reviewed: false },
    },
    'mix': {
        en: { t: 'Mix',
              b: 'Blend between the dry input and the compressed output for parallel compression. 0% is fully dry, 100% is fully compressed.' },
        fr: { t: 'Mix',
              b: 'Dosage entre le signal d’entrée non traité et la sortie compressée, pour la compression parallèle. 0 % est entièrement sec, 100 % entièrement compressé.',
              reviewed: false },
    },
    'auto-makeup': {
        en: { t: 'Auto Makeup',
              b: 'Automatically compensates the level lost to gain reduction in each band, so bypassing the compressor does not jump in volume. Stacks with each band’s Makeup knob.' },
        fr: { t: 'Compensation auto',
              b: 'Compense automatiquement le niveau perdu par la réduction de gain dans chaque bande, afin que la mise en dérivation ne provoque pas de saut de volume. S’ajoute au bouton Compensation de chaque bande.',
              reviewed: false },
    },
    'ms-mode': {
        en: { t: 'Mid / Side Mode',
              b: 'Chooses what the compressors act on. Off processes left and right normally; Mid targets the centre of the image, Side the stereo edges, and Both processes them independently.' },
        fr: { t: 'Mode Mid / Side',
              b: 'Détermine ce sur quoi agissent les compresseurs. Off traite normalement la gauche et la droite ; Mid vise le centre de l’image, Side les bords stéréo, et Both les traite indépendamment.',
              reviewed: false },
    },
    'output-gain': {
        en: { t: 'Output Gain',
              b: 'Final level trim, applied after the mix stage. −24 to +24 dB.' },
        fr: { t: 'Gain de sortie',
              b: 'Correction de niveau finale, appliquée après l’étage de mixage. −24 à +24 dB.',
              reviewed: false },
    },
    'input-meter': {
        en: { t: 'Input Meter',
              b: 'Level entering the plugin, averaged across both channels and measured before the input gain trim.' },
        fr: { t: 'Vumètre d’entrée',
              b: 'Niveau entrant dans le plug-in, moyenné sur les deux canaux et mesuré avant la correction de gain d’entrée.',
              reviewed: false },
    },
    'output-meter': {
        en: { t: 'Output Meter',
              b: 'Level leaving the plugin, measured after mix and output gain.' },
        fr: { t: 'Vumètre de sortie',
              b: 'Niveau sortant du plug-in, mesuré après le mixage et le gain de sortie.',
              reviewed: false },
    },
    'spectrum': {
        en: { t: 'Spectrum Analyzer',
              b: 'Real-time input spectrum, 20 Hz to 20 kHz on a logarithmic scale. Drag the vertical lines to move the crossovers.' },
        fr: { t: 'Analyseur de spectre',
              b: 'Spectre d’entrée en temps réel, de 20 Hz à 20 kHz sur une échelle logarithmique. Faites glisser les lignes verticales pour déplacer les points de coupure.',
              reviewed: false },
    },
    'crossover1': {
        en: { t: 'Crossover 1',
              b: 'Split point between the Low and Low-Mid bands. Drag left or right to move it; the two band headers update as you go. 20 Hz to 500 Hz.' },
        fr: { t: 'Coupure 1',
              b: 'Point de séparation entre les bandes Grave et Bas-médium. Faites-la glisser à gauche ou à droite ; les deux en-têtes de bande se mettent à jour au fur et à mesure. 20 Hz à 500 Hz.',
              reviewed: false },
    },
    'crossover2': {
        en: { t: 'Crossover 2',
              b: 'Split point between the Low-Mid and High-Mid bands. Drag left or right to move it. 200 Hz to 5 kHz.' },
        fr: { t: 'Coupure 2',
              b: 'Point de séparation entre les bandes Bas-médium et Haut-médium. Faites-la glisser à gauche ou à droite. 200 Hz à 5 kHz.',
              reviewed: false },
    },
    'crossover3': {
        en: { t: 'Crossover 3',
              b: 'Split point between the High-Mid and High bands. Drag left or right to move it. 2 kHz to 16 kHz.' },
        fr: { t: 'Coupure 3',
              b: 'Point de séparation entre les bandes Haut-médium et Aigu. Faites-la glisser à gauche ou à droite. 2 kHz à 16 kHz.',
              reviewed: false },
    },

    // ── Band display names ───────────────────────────────────────────────
    // Label-only entries: they are never bound to an element, they are the
    // VALUES that TIP_BINDINGS passes as `vars.band`, and tr() resolves a var
    // value that is itself an I18N key to that key's localized title. `b` is
    // deliberately empty — nothing renders a body for a band name.
    'band.low':   { en: { t: 'Low',      b: '' }, fr: { t: 'Grave',       b: '', reviewed: false } },
    'band.lomid': { en: { t: 'Low-Mid',  b: '' }, fr: { t: 'Bas-médium',  b: '', reviewed: false } },
    'band.himid': { en: { t: 'High-Mid', b: '' }, fr: { t: 'Haut-médium', b: '', reviewed: false } },
    'band.high':  { en: { t: 'High',     b: '' }, fr: { t: 'Aigu',        b: '', reviewed: false } },

    // ── Per-band controls ────────────────────────────────────────────────
    // Identical in all four bands, so the wording lives here once and the band
    // name is composed in through {band}. The joining em-dash is a literal.
    'band.threshold': {
        en: { t: '{band} — Threshold',
              b: 'The level at which this band starts to compress. Anything above it is pulled down by the Ratio. −60 to 0 dB.' },
        fr: { t: '{band} — Seuil',
              b: 'Le niveau à partir duquel cette bande commence à compresser. Tout ce qui le dépasse est ramené vers le bas selon le Taux. −60 à 0 dB.',
              reviewed: false },
    },
    'band.ratio': {
        en: { t: '{band} — Ratio',
              b: 'How firmly the band is compressed above the threshold. 1:1 leaves it untouched; 20:1 is effectively limiting.' },
        fr: { t: '{band} — Taux',
              b: 'Fermeté de la compression au-dessus du seuil. 1:1 laisse la bande intacte ; 20:1 revient à de la limitation.',
              reviewed: false },
    },
    'band.attack': {
        en: { t: '{band} — Attack',
              b: 'How quickly compression engages once the signal crosses the threshold. Fast settings clamp transients, slow settings let them through. 0.1 to 200 ms.' },
        fr: { t: '{band} — Attaque',
              b: 'Rapidité d’engagement de la compression une fois le seuil franchi. Les réglages rapides écrêtent les transitoires, les lents les laissent passer. 0,1 à 200 ms.',
              reviewed: false },
    },
    'band.release': {
        en: { t: '{band} — Release',
              b: 'How quickly compression lets go once the signal falls back below the threshold. Too fast can pump, too slow can choke the band. 10 to 2000 ms.' },
        fr: { t: '{band} — Rétablissement',
              b: 'Rapidité avec laquelle la compression relâche une fois le signal redescendu sous le seuil. Trop rapide, elle pompe ; trop lent, elle étouffe la bande. 10 à 2000 ms.',
              reviewed: false },
    },
    'band.knee': {
        en: { t: '{band} — Knee',
              b: 'Softens the onset of compression around the threshold. 0 dB is a hard knee that grabs abruptly; 24 dB eases in very gradually.' },
        fr: { t: '{band} — Coude',
              b: 'Adoucit l’entrée en compression autour du seuil. 0 dB donne un coude dur qui saisit brusquement ; 24 dB amène la compression très progressivement.',
              reviewed: false },
    },
    'band.makeup': {
        en: { t: '{band} — Makeup',
              b: 'Manual gain applied to this band after compression, to restore what gain reduction took away. −12 to +24 dB.' },
        fr: { t: '{band} — Compensation',
              b: 'Gain manuel appliqué à cette bande après compression, pour restituer ce que la réduction de gain a retiré. −12 à +24 dB.',
              reviewed: false },
    },
    'band.solo': {
        en: { t: '{band} — Solo',
              b: 'Hear this band on its own — the other three are muted. Useful for checking where a crossover should sit.' },
        fr: { t: '{band} — Solo',
              b: 'Écoute cette bande seule — les trois autres sont coupées. Pratique pour vérifier où placer un point de coupure.',
              reviewed: false },
    },
    'band.bypass': {
        en: { t: '{band} — Bypass',
              b: 'Pass this band through uncompressed. The crossover filtering still applies, so the band stays in phase with the others.' },
        fr: { t: '{band} — Dérivation',
              b: 'Laisse passer cette bande sans compression. Le filtrage de coupure reste appliqué, la bande reste donc en phase avec les autres.',
              reviewed: false },
    },
    'band.sc-listen': {
        en: { t: '{band} — Sidechain Listen',
              b: 'Monitor the detector signal driving this band’s compressor, including its sidechain filtering. This is what the compressor "hears", not what it outputs.' },
        fr: { t: '{band} — Écoute du sidechain',
              b: 'Écoute le signal du détecteur qui pilote le compresseur de cette bande, filtrage de sidechain compris. C’est ce que le compresseur « entend », pas ce qu’il produit.',
              reviewed: false },
    },
    'band.peak-rms': {
        en: { t: '{band} — Peak / RMS',
              b: 'Blends how the band’s level is measured. Peak reacts to individual transients and suits de-essing and plosive control; RMS averages over 10 ms and suits glue and level-riding.' },
        fr: { t: '{band} — Crête / RMS',
              b: 'Dose la façon dont le niveau de la bande est mesuré. Crête réagit aux transitoires isolés et convient au dé-essage et au contrôle des plosives ; RMS moyenne sur 10 ms et convient au liant et au nivellement.',
              reviewed: false },
    },
    'band.sc-hpf': {
        en: { t: '{band} — Sidechain High-Pass',
              b: 'High-passes the detector only — the audio itself is untouched. Keeps low energy from triggering gain reduction, for example so subsonic rumble does not duck a whole band. Fully left is Off.' },
        fr: { t: '{band} — Passe-haut du sidechain',
              b: 'Filtre en passe-haut le détecteur uniquement — l’audio lui-même n’est pas touché. Empêche l’énergie grave de déclencher la réduction de gain, par exemple pour qu’un grondement infrasonore ne fasse pas plonger toute une bande. Complètement à gauche : Off.',
              reviewed: false },
    },
    'band.sc-lpf': {
        en: { t: '{band} — Sidechain Low-Pass',
              b: 'Low-passes the detector only — the audio itself is untouched. Narrows what the band responds to, for example keeping cymbals and air from holding a de-esser down. Fully left is Off.' },
        fr: { t: '{band} — Passe-bas du sidechain',
              b: 'Filtre en passe-bas le détecteur uniquement — l’audio lui-même n’est pas touché. Restreint ce à quoi la bande réagit, par exemple pour éviter que cymbales et air ne maintiennent un dé-esseur enfoncé. Complètement à gauche : Off.',
              reviewed: false },
    },
    'band.gr': {
        en: { t: '{band} — Gain Reduction',
              b: 'How much this band is being compressed right now. The bar fills as gain reduction deepens, up to −24 dB.' },
        fr: { t: '{band} — Réduction de gain',
              b: 'Quantité de compression appliquée à cette bande en ce moment. La barre se remplit à mesure que la réduction de gain s’accentue, jusqu’à −24 dB.',
              reviewed: false },
    },
    'band.range': {
        en: { t: '{band} — Frequency Range',
              b: 'The span this band processes. It follows the crossover handles in the analyzer above, so drag them to retune it.' },
        fr: { t: '{band} — Plage de fréquences',
              b: 'L’étendue traitée par cette bande. Elle suit les poignées de coupure de l’analyseur ci-dessus ; déplacez-les pour la réajuster.',
              reviewed: false },
    },

    // ── Settings popover ─────────────────────────────────────────────────
    'settings': {
        en: { t: 'Settings',
              b: 'Interface preferences — the language of this hover help, and whether hover help appears at all.' },
        fr: { t: 'Réglages',
              b: 'Préférences d’interface — la langue de cette aide au survol, et son affichage ou non.',
              reviewed: false },
    },
    'lang-select': {
        en: { t: 'Language',
              b: 'Language of the hover help. The choice is remembered with the session and is not carried by presets. Control labels and value readouts stay in English.' },
        fr: { t: 'Langue',
              b: 'Langue de l’aide au survol. Le choix est conservé avec la session et n’est pas transporté par les préréglages. Les libellés des commandes et les afficheurs restent en anglais.',
              reviewed: false },
    },
    // One key covering both states rather than a pair swapped on click: the
    // canonical applyI18n() re-renders straight from this table on every
    // language change, so a state-dependent tip written outside it would be
    // stale in the new language. The caption and aria-pressed carry the state.
    'tips-toggle': {
        en: { t: 'Hover Help',
              b: 'Turns this hover-help layer on or off for every control. While it is off, this button still explains itself, so help can always be switched back on. The setting is shared by every instance on this machine.' },
        fr: { t: 'Aide au survol',
              b: 'Active ou désactive cette aide au survol pour toutes les commandes. Lorsqu’elle est désactivée, ce bouton continue de s’expliquer, afin de pouvoir toujours la réactiver. Ce réglage est partagé par toutes les instances de cette machine.',
              reviewed: false },
    },
});

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
