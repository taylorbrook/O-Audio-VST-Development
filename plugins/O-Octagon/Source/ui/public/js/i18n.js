/*
   This file is part of O-Octagon, an Ouaricon Audio plugin.
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
// i18n.js — O-Octagon hover-help copy, English + French (v1.6.0)
//
// An ES module that EXPORTS ONLY. It must never self-execute: a bare top-level
// statement here throws out of module evaluation and takes every later
// initializer on the page with it (pattern_module_toplevel_init_tdz). Two
// independent gates enforce that — scripts/check-i18n.js assertion 7, and
// section 2 of tests/ui_frontend_check.js, which additionally requires init()
// to remain the literal last statement of app.js. That is why app.js calls
// initI18n() from INSIDE init() rather than from a foot-of-file block.
//
// FILENAME: no hyphen. juce_add_binary_data STRIPS hyphens rather than
// converting them to underscores, so a second file named i18n-fr.js would have
// to be reached as the symbol i18nfr_js (critical_binary_data_strips_hyphens).
// One combined file for both languages sidesteps the question entirely.
//
// COPY IS textContent ON EVERY PATH — never innerHTML. showTip() in app.js
// builds the tip with createElement + textContent, and check-i18n assertion 9
// rejects any innerHTML reference here and any string literal containing `<`.
// A line break, if one is ever needed, is \n plus CSS white-space: pre-line,
// never a markup tag.
//
// THE ENGLISH WAS MOVED, NOT REWRITTEN. Every en entry below is byte-for-byte
// what index.html carried through v1.5.0, with its HTML entities decoded to the
// characters they named (&#8212; -> —, &#8202; -> \u200A, and so on) because
// setAttribute + textContent do not decode entities.
//
// ALL FRENCH IS MACHINE-DRAFTED AND FLAGGED `reviewed: false`. No native
// speaker has read it. `node scripts/check-i18n.js` prints the worklist.
// ============================================================================

export const LANGUAGES = ['en', 'fr'];

// key -> { en: {t, b}, fr: {t, b, reviewed} }
//   t = tooltip title (the small caps line), b = tooltip body.
//
// Object.freeze() rather than a bare `{...}` literal for two reasons. It says
// out loud that this module is inert data nothing may mutate at runtime — and
// it keeps the export a SINGLE top-level statement, because a statement written
// `export const X = {...};` closes its brace at depth zero and segments the
// trailing `;` off on its own.
//
// PARAMETERISED ENTRIES carry {token} placeholders substituted by tr()'s `vars`
// argument. They are NOT template literals: the table is inert data, evaluated
// once at module load, so a value interpolated here would be frozen at whatever
// the language happened to be then.
export const I18N = Object.freeze({

    // ── Header ──────────────────────────────────────────────────────────────
    'tab-room': {
        en: { t: 'Room',
              b: 'The performance view — plan, puck, weights, scenes, meters and the DBAP field.' },
        fr: { t: 'Salle',
              b: 'La vue de jeu — le plan, le curseur, les poids, les scènes, les indicateurs et le champ DBAP.',
              reviewed: false },
    },
    'tab-venue': {
        en: { t: 'Venue',
              b: 'The 42 measured values that define the room — positions, trims, rake — plus venue files, presets, output order and the verify ping.' },
        fr: { t: 'Lieu',
              b: 'Les 42 valeurs mesurées qui définissent la salle — positions, corrections, inclinaison des gradins — ainsi que les fichiers de lieu, les préréglages, l’ordre des sorties et le bip de vérification.',
              reviewed: false },
    },

    // ── The settings popover (v1.6.0) ───────────────────────────────────────
    // The gear and the language selector are new. The hover-help entry is the
    // v1.2.0 "?" toggle's copy, moved here unchanged along with the control.
    'settings': {
        en: { t: 'Settings',
              b: 'Choose the language of this hover help, and turn the hover help on or off. Both choices are remembered with the session.' },
        fr: { t: 'Réglages',
              b: 'Choisir la langue de cette aide au survol et activer ou désactiver cette aide. Les deux choix sont conservés avec la session.',
              reviewed: false },
    },
    'lang-select': {
        en: { t: 'Language',
              b: 'The language this hover help is written in. English and French are available; the labels on the page itself do not change.' },
        fr: { t: 'Langue',
              b: 'La langue dans laquelle cette aide au survol est rédigée. L’anglais et le français sont disponibles ; les libellés de la page elle-même ne changent pas.',
              reviewed: false },
    },
    // ONE key covering both states, never a state-swapped pair. applyI18n()
    // re-renders every tip straight from this table on a language change, so a
    // state-dependent string written outside it would be stranded in the
    // previous language the moment the selector fires — and window.__setLanguage(),
    // which the clamp gates drive, fires no `change` event at all, so no
    // listener workaround would cover both paths. The two buttons and their
    // aria-pressed carry the state instead.
    'tips-toggle': {
        en: { t: 'Hover help',
              b: 'Show a short description when the pointer rests on a control. The setting is remembered with the session.' },
        fr: { t: 'Aide au survol',
              b: 'Affiche une courte description lorsque le pointeur s’arrête sur une commande. Le réglage est conservé avec la session.',
              reviewed: false },
    },

    // ── Banners ─────────────────────────────────────────────────────────────
    'safe-banner': {
        en: { t: 'Safe mode',
              b: 'The host negotiated stereo, not the 8\u200A·\u200Achannel rig — what you hear is a fold-down. In Logic, insert via the slot\'s Stereo → 7.1 entry.' },
        fr: { t: 'Mode sécurisé',
              b: 'L’hôte a négocié une sortie stéréo, pas le dispositif à 8\u200A·\u200Acanaux — ce que vous entendez est un repli. Dans Logic, insérez le plugin par l’entrée Stéréo → 7.1 de l’emplacement.',
              reviewed: false },
    },
    'map-banner': {
        en: { t: 'Map invalid',
              b: 'A speaker label no longer resolves against the negotiated output set — the reason and the row to fix are printed here. Until fixed, the rig falls back to a raw stereo split.' },
        fr: { t: 'Affectation invalide',
              b: 'Un libellé de haut-parleur ne correspond plus au jeu de sorties négocié — la raison et la ligne à corriger sont indiquées ici. Tant que ce n’est pas corrigé, le dispositif revient à un simple partage stéréo.',
              reviewed: false },
    },

    // ── Room plan ───────────────────────────────────────────────────────────
    'puck': {
        en: { t: 'Source',
              b: 'Drag to move the source over the plan — writes Source X and Y together. Double-click a knob or slider elsewhere to reset it.' },
        fr: { t: 'Source',
              b: 'Faites glisser pour déplacer la source sur le plan — écrit Source X et Y ensemble. Double-cliquez ailleurs sur un bouton ou un curseur pour le réinitialiser.',
              reviewed: false },
    },
    // {n} is a speaker number, substituted literally — not an I18N key. The
    // eight weight cells shared one sentence in the markup and share one entry
    // here, which is also what stops seven of the eight going stale when the
    // wording changes.
    'weight': {
        en: { t: 'Weight {n}',
              b: 'Speaker {n}’s share of the DBAP solve, 0–1. Double-click the glyph to reassign its physical output; double-click the slider to reset.' },
        fr: { t: 'Poids {n}',
              b: 'Part du haut-parleur {n} dans la résolution DBAP, de 0 à 1. Double-cliquez sur le glyphe pour réaffecter sa sortie physique ; double-cliquez sur le curseur pour réinitialiser.',
              reviewed: false },
    },

    // ── Position ────────────────────────────────────────────────────────────
    'srcX': {
        en: { t: 'Source X',
              b: 'Left–right position of the source, resolved against the measured speaker bounding box — the metres readout below is live.' },
        fr: { t: 'Source X',
              b: 'Position gauche–droite de la source, rapportée au rectangle englobant mesuré des haut-parleurs — l’affichage en mètres ci-dessous est en temps réel.',
              reviewed: false },
    },
    'srcY': {
        en: { t: 'Source Y',
              b: 'Front–back position of the source, resolved against the measured speaker bounding box.' },
        fr: { t: 'Source Y',
              b: 'Position avant–arrière de la source, rapportée au rectangle englobant mesuré des haut-parleurs.',
              reviewed: false },
    },
    'srcZ': {
        en: { t: 'Source Z',
              b: 'Source height, −2 to 8 m. Rising toward the speaker plane gets louder and sharper; flying above the array recedes (±6 dB proximity cue). The elevation strip shows it against the rake and the speaker heights — the marker clamps, the numbers never do.' },
        fr: { t: 'Source Z',
              b: 'Hauteur de la source, de −2 à 8 m. En montant vers le plan des haut-parleurs, le son gagne en niveau et en netteté ; en s’élevant au-dessus du dispositif, il s’éloigne (indice de proximité de ±6 dB). La bande d’élévation la situe par rapport à l’inclinaison des gradins et aux hauteurs des haut-parleurs — le repère est borné, jamais les valeurs.',
              reviewed: false },
    },
    'width': {
        en: { t: 'Width',
              b: 'Spreads the source into sub-points around its position, up to 12 m apart — wider reads as a broader image across the rig. On stereo material it pulls the L and R feeds to different parts of the room; on mono material, reach for Decorrelate below.' },
        fr: { t: 'Largeur',
              b: 'Répartit la source en points secondaires autour de sa position, jusqu’à 12 m d’écart — plus la valeur est élevée, plus l’image s’élargit sur le dispositif. Sur du matériel stéréo, les voies G et D sont envoyées vers des zones différentes de la salle ; sur du mono, utilisez Décorréler ci-dessous.',
              reviewed: false },
    },
    'decorr': {
        en: { t: 'Decorrelate',
              b: 'Makes Width audible on mono material. Width alone moves two IDENTICAL copies of the signal apart in the room, and two identical copies comb rather than widen; this gives each copy its own all-pass network so they share a spectrum but not a phase. Off by default, and inert at Width 0 — where the two feeds land on the same speakers, decorrelating them would only cost you the coherent sum. Expect up to 3 dB less level where the two feeds overlap: that is the combing going away.' },
        fr: { t: 'Décorréler',
              b: 'Rend la Largeur audible sur du matériel mono. La Largeur seule éloigne deux copies IDENTIQUES du signal dans la salle, et deux copies identiques produisent un filtrage en peigne au lieu d’élargir ; ici, chaque copie reçoit son propre réseau passe-tout, de sorte qu’elles partagent le spectre mais pas la phase. Désactivé par défaut, et sans effet à Largeur 0 — là où les deux voies aboutissent aux mêmes haut-parleurs, les décorréler ne ferait que vous coûter la somme cohérente. Attendez-vous à jusqu’à 3 dB de niveau en moins là où les deux voies se recouvrent : c’est le filtrage en peigne qui disparaît.',
              reviewed: false },
    },

    // ── Solve ───────────────────────────────────────────────────────────────
    'rolloff': {
        en: { t: 'Rolloff',
              b: 'DBAP distance rolloff, 3–12 dB per distance doubling. Higher concentrates energy hard into the nearest speakers; lower spreads it across the whole array.' },
        fr: { t: 'Atténuation',
              b: 'Atténuation DBAP avec la distance, de 3 à 12 dB par doublement de distance. Une valeur élevée concentre fortement l’énergie sur les haut-parleurs les plus proches ; une valeur faible la répartit sur tout le dispositif.',
              reviewed: false },
    },
    'blur': {
        en: { t: 'Blur',
              b: 'Softens the distance differences the solve sees — full blur washes the source across the whole array. Scaled with the rig, not in metres, so a patch means the same thing in a club and a hall.' },
        fr: { t: 'Flou',
              b: 'Adoucit les écarts de distance que voit la résolution — au maximum, la source se répand sur tout le dispositif. La valeur suit l’échelle du dispositif et non des mètres, si bien qu’un réglage a le même sens dans un club et dans une grande salle.',
              reviewed: false },
    },

    // ── Space ───────────────────────────────────────────────────────────────
    'hullAtten': {
        en: { t: 'Hull attenuation',
              b: 'How strongly the source fades as it crosses outside the speaker hull.' },
        fr: { t: 'Atténuation hors enveloppe',
              b: 'Détermine la vitesse à laquelle la source s’efface lorsqu’elle sort de l’enveloppe des haut-parleurs.',
              reviewed: false },
    },
    'airAmount': {
        en: { t: 'Air',
              b: 'Distance air filter — high frequencies fall away as the source sits farther from the array, with the cutoff derived from the venue geometry.' },
        fr: { t: 'Air',
              b: 'Filtre d’air lié à la distance — les aigus s’estompent à mesure que la source s’éloigne du dispositif, la fréquence de coupure étant déduite de la géométrie du lieu.',
              reviewed: false },
    },

    // ── Output ──────────────────────────────────────────────────────────────
    'outputGain': {
        en: { t: 'Output',
              b: 'Master trim for all eight channels.' },
        fr: { t: 'Sortie',
              b: 'Réglage général de niveau pour les huit canaux.',
              reviewed: false },
    },

    // ── Scenes ──────────────────────────────────────────────────────────────
    'scene-store': {
        en: { t: 'Store',
              b: 'Arm, then click a U slot to capture the current eight weights into it. Disarms after one capture.' },
        fr: { t: 'Mémoriser',
              b: 'Armez, puis cliquez sur un emplacement U pour y enregistrer les huit poids actuels. Se désarme après une seule capture.',
              reviewed: false },
    },
    'scene-all': {
        en: { t: 'All',
              b: 'All eight speakers at full weight.' },
        fr: { t: 'Tous',
              b: 'Les huit haut-parleurs à poids maximal.',
              reviewed: false },
    },
    'scene-front': {
        en: { t: 'Front',
              b: 'The front speakers only — membership is derived from the measured geometry, not from fixed slot numbers.' },
        fr: { t: 'Avant',
              b: 'Les haut-parleurs avant uniquement — l’appartenance est déduite de la géométrie mesurée, et non de numéros d’emplacement fixes.',
              reviewed: false },
    },
    'scene-rear': {
        en: { t: 'Rear',
              b: 'The rear speakers only, derived from the measured geometry.' },
        fr: { t: 'Arrière',
              b: 'Les haut-parleurs arrière uniquement, déduits de la géométrie mesurée.',
              reviewed: false },
    },
    'scene-left': {
        en: { t: 'Left',
              b: 'The left-side speakers only, derived from the measured geometry.' },
        fr: { t: 'Gauche',
              b: 'Les haut-parleurs du côté gauche uniquement, déduits de la géométrie mesurée.',
              reviewed: false },
    },
    'scene-right': {
        en: { t: 'Right',
              b: 'The right-side speakers only, derived from the measured geometry.' },
        fr: { t: 'Droite',
              b: 'Les haut-parleurs du côté droit uniquement, déduits de la géométrie mesurée.',
              reviewed: false },
    },
    'scene-sides': {
        en: { t: 'Sides',
              b: 'The side speakers only, derived from the measured geometry.' },
        fr: { t: 'Côtés',
              b: 'Les haut-parleurs latéraux uniquement, déduits de la géométrie mesurée.',
              reviewed: false },
    },
    // {n} is a slot number, substituted literally. STORE is the on-screen
    // caption of a control, which stays English like every other label on the
    // page — CONTEXT.md scopes this work to hover help, not to page chrome.
    'scene-slot': {
        en: { t: 'User scene {n}',
              b: 'Click to recall this stored weight scene. Arm STORE first to capture the current weights into it.' },
        fr: { t: 'Scène utilisateur {n}',
              b: 'Cliquez pour rappeler cette scène de poids enregistrée. Armez d’abord STORE pour y enregistrer les poids actuels.',
              reviewed: false },
    },

    // ── Elevation strip ─────────────────────────────────────────────────────
    'elevation': {
        en: { t: 'Elevation',
              b: 'The room side-on — rake line, speaker heights and the source marker. Ear is the listener height under the source; Source is its absolute height.' },
        fr: { t: 'Élévation',
              b: 'La salle vue de côté — ligne d’inclinaison des gradins, hauteurs des haut-parleurs et repère de la source. « Ear » est la hauteur d’oreille sous la source ; « Source » est sa hauteur absolue.',
              reviewed: false },
    },

    // ── Venue screen ────────────────────────────────────────────────────────
    'rake': {
        en: { t: 'Rake',
              b: 'Audience rake — ear height at the front and rear of the seating. Heights between are interpolated along the room depth.' },
        fr: { t: 'Inclinaison',
              b: 'Inclinaison des gradins — hauteur d’oreille à l’avant et à l’arrière du public. Les hauteurs intermédiaires sont interpolées sur la profondeur de la salle.',
              reviewed: false },
    },
    'delay': {
        en: { t: 'Alignment delay',
              b: 'Per-speaker delay that time-aligns arrivals at one seat. Derive fills all eight from the measured distances; every value stays editable afterwards.' },
        fr: { t: 'Retard d’alignement',
              b: 'Retard par haut-parleur qui aligne temporellement les arrivées à une place donnée. « Derive » remplit les huit valeurs à partir des distances mesurées ; chaque valeur reste modifiable ensuite.',
              reviewed: false },
    },
    'delay-unit': {
        en: { t: 'Delay unit',
              b: 'Show the Delay column in milliseconds or in metres of path difference. Values are stored as milliseconds either way.' },
        fr: { t: 'Unité de retard',
              b: 'Affiche la colonne Delay en millisecondes ou en mètres de différence de trajet. Les valeurs sont enregistrées en millisecondes dans les deux cas.',
              reviewed: false },
    },
    'delay-derive': {
        en: { t: 'Derive delays',
              b: 'Fill all eight delays to time-align arrivals at the centre of the array, at the ear height the rake gives that depth. The farthest speaker gets zero. Every value stays editable afterwards.' },
        fr: { t: 'Calculer les retards',
              b: 'Remplit les huit retards afin d’aligner temporellement les arrivées au centre du dispositif, à la hauteur d’oreille que l’inclinaison donne à cette profondeur. Le haut-parleur le plus éloigné reçoit zéro. Chaque valeur reste modifiable ensuite.',
              reviewed: false },
    },
    'output-set': {
        en: { t: 'Output set',
              b: 'The surround format the host negotiated — speaker labels must resolve against it.' },
        fr: { t: 'Jeu de sorties',
              b: 'Le format surround négocié par l’hôte — les libellés des haut-parleurs doivent y correspondre.',
              reviewed: false },
    },
    'venue-save': {
        en: { t: 'Save venue',
              b: 'Write the 42 measured values to a .venue file — independent of musical presets.' },
        fr: { t: 'Enregistrer le lieu',
              b: 'Écrit les 42 valeurs mesurées dans un fichier .venue — indépendant des préréglages musicaux.',
              reviewed: false },
    },
    'venue-load': {
        en: { t: 'Load venue',
              b: 'Load a .venue file. Rejected if its labels do not resolve against the negotiated output set.' },
        fr: { t: 'Charger un lieu',
              b: 'Charge un fichier .venue. Refusé si ses libellés ne correspondent pas au jeu de sorties négocié.',
              reviewed: false },
    },
    'preset-list': {
        en: { t: 'Preset',
              b: 'Musical presets carry the 17 parameters and the four user scenes — never the 42 measured venue values.' },
        fr: { t: 'Préréglage',
              b: 'Les préréglages musicaux contiennent les 17 paramètres et les quatre scènes utilisateur — jamais les 42 valeurs mesurées du lieu.',
              reviewed: false },
    },
    'preset-save': {
        en: { t: 'Save preset',
              b: 'Save the current parameters and user scenes as a preset.' },
        fr: { t: 'Enregistrer le préréglage',
              b: 'Enregistre les paramètres et les scènes utilisateur actuels sous forme de préréglage.',
              reviewed: false },
    },
    'preset-load': {
        en: { t: 'Load preset',
              b: 'Load the selected preset. The venue is untouched.' },
        fr: { t: 'Charger le préréglage',
              b: 'Charge le préréglage sélectionné. Le lieu n’est pas modifié.',
              reviewed: false },
    },
    'oo-direct': {
        en: { t: 'Direct 1–8',
              b: 'Wire speaker n to physical output n under the measured CoreAudio device order — the one-click fix for a rig cabled 1–8.' },
        fr: { t: 'Direct 1–8',
              b: 'Relie le haut-parleur n à la sortie physique n selon l’ordre de périphérique CoreAudio mesuré — la correction en un clic pour un dispositif câblé de 1 à 8.',
              reviewed: false },
    },
    'oo-roles': {
        en: { t: 'Roles',
              b: 'Restore the factory surround-role labels.' },
        fr: { t: 'Rôles',
              b: 'Rétablit les libellés de rôles surround d’usine.',
              reviewed: false },
    },
    'ping-grid': {
        en: { t: 'Verify ping',
              b: 'Sound a confirmation ping from one speaker. The lit number is what the plugin reports playing — never a timer.' },
        fr: { t: 'Bip de vérification',
              b: 'Émet un bip de confirmation depuis un haut-parleur. Le numéro allumé est celui que le plugin déclare en train de jouer — jamais une minuterie.',
              reviewed: false },
    },
    'ping-auto': {
        en: { t: 'Auto ping',
              b: 'Step the ping around all eight speakers in order.' },
        fr: { t: 'Bip automatique',
              b: 'Fait passer le bip successivement sur les huit haut-parleurs, dans l’ordre.',
              reviewed: false },
    },
    'ping-stop': {
        en: { t: 'Stop',
              b: 'Stop the ping.' },
        fr: { t: 'Arrêter',
              b: 'Arrête le bip.',
              reviewed: false },
    },

    // ── Footer readouts ─────────────────────────────────────────────────────
    'readout-source': {
        en: { t: 'Source',
              b: 'The source position in metres, resolved against the live venue geometry.' },
        fr: { t: 'Source',
              b: 'La position de la source en mètres, rapportée à la géométrie du lieu en temps réel.',
              reviewed: false },
    },
    'readout-envelope': {
        en: { t: 'Envelope',
              b: 'The drawn plan extent in metres — the speaker bounding box plus its margin.' },
        fr: { t: 'Enveloppe',
              b: 'L’étendue du plan dessiné en mètres — le rectangle englobant des haut-parleurs augmenté de sa marge.',
              reviewed: false },
    },
});

// [selector, key] or [selector, key, wrapperSelector] or
// [selector, key, wrapperSelector, vars].
//
// The selector is the BINDING SITE. It is a selector rather than a
// key-on-the-element attribute because several tips here attach to elements the
// markup does not give an id — the ten `.cell` control groups, the venue rake
// and delay rows, the ping grid — and the wrapper form reaches them from the
// control they already contain. Everything with an id simply uses '#<id>'.
//
// `vars` is static data. A var value is substituted literally by tr() unless it
// is itself an I18N key, in which case it resolves against the CURRENT
// language; O-Octagon's two parameterised entries both take a plain number.
export const TIP_BINDINGS = [
    ['#tab-room',              'tab-room'],
    ['#tab-venue',             'tab-venue'],

    ['#gear-btn',              'settings'],
    ['#lang-select',           'lang-select'],
    ['#tips-toggle',           'tips-toggle'],

    ['#safe-banner',           'safe-banner'],
    ['#map-banner',            'map-banner'],

    ['#puck',                  'puck'],

    // The ten column controls. The tip belongs on the whole `.cell`, so the
    // label and the value readout raise it too, not just the slider track.
    ['#ctl-srcX',              'srcX',        '.cell'],
    ['#ctl-srcY',              'srcY',        '.cell'],
    ['#ctl-srcZ',              'srcZ',        '.cell'],
    ['#ctl-width',             'width',       '.cell'],
    ['#ctl-decorr',            'decorr',      '.cell'],
    ['#ctl-rolloff',           'rolloff',     '.cell'],
    ['#ctl-blur',              'blur',        '.cell'],
    ['#ctl-hullAtten',         'hullAtten',   '.cell'],
    ['#ctl-airAmount',         'airAmount',   '.cell'],
    ['#ctl-outputGain',        'outputGain',  '.cell'],

    ['#btn-scene-store',       'scene-store'],
    ['#scene-ALL',             'scene-all'],
    ['#scene-FRONT',           'scene-front'],
    ['#scene-REAR',            'scene-rear'],
    ['#scene-LEFT',            'scene-left'],
    ['#scene-RIGHT',           'scene-right'],
    ['#scene-SIDES',           'scene-sides'],

    ['#elev-stage',            'elevation'],

    ['.venue-rake',            'rake'],
    ['.venue-delay',           'delay'],
    ['.vunit-group',           'delay-unit'],
    ['#btn-delay-derive',      'delay-derive'],
    ['.rail-line',             'output-set'],
    ['#btn-venue-save',        'venue-save'],
    ['#btn-venue-load',        'venue-load'],
    ['#preset-list',           'preset-list'],
    ['#btn-preset-save',       'preset-save'],
    ['#btn-preset-load',       'preset-load'],
    ['#btn-oo-direct',         'oo-direct'],
    ['#btn-oo-roles',          'oo-roles'],
    ['.ping-grid',             'ping-grid'],
    ['#btn-ping-auto',         'ping-auto'],
    ['#btn-ping-stop',         'ping-stop'],

    ['#readout-label-source',   'readout-source'],
    ['#readout-label-envelope', 'readout-envelope'],

    // The eight weight cells and the four user-scene slots, expanded from one
    // parameterised entry each rather than transcribed twelve times.
    ...[1, 2, 3, 4, 5, 6, 7, 8].map(n => ['#wcell-' + n, 'weight', null, { n: String(n) }]),
    ...[1, 2, 3, 4].map(n => ['#scene-slot' + n, 'scene-slot', null, { n: String(n) }]),
];

export function tr(key, lang, vars) {
    const entry = I18N[key];
    if (!entry) { console.warn(`i18n: missing key ${key}`); return { t: key, b: '' }; }
    const s = entry[lang] || entry.en;

    // A var VALUE that is itself an I18N key resolves to that key's localized
    // title; anything else is used literally. O-Octagon only needs the literal
    // arm today, but the resolving arm is what lets a plugin compose a
    // localized name into a tip without pinning TIP_BINDINGS — which is static
    // data evaluated once — to the load-time language.
    const resolve = (v) => {
        const nested = I18N[v];
        return nested ? String((nested[lang] || nested.en).t) : String(v);
    };

    const sub = (v) => vars
        ? String(v).replace(/\{(\w+)\}/g, (m, n) => (n in vars ? resolve(vars[n]) : m))
        : String(v);

    return { t: sub(s.t), b: sub(s.b) };
}
