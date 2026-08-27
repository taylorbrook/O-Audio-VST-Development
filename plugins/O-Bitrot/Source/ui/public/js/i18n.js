/*
   This file is part of O-Bitrot, an Ouaricon Audio plugin.
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
// i18n.js — O-Bitrot hover-help copy, English + French (v1.14.0)
//
// An ES module that EXPORTS ONLY. It must never self-execute: a bare top-level
// statement here throws out of module evaluation and takes every later
// initializer on the page with it (pattern_module_toplevel_init_tdz), which on
// this plugin means the ENTIRE UI — O-Bitrot's controller is one inline
// <script type="module"> in index.html, evaluated top to bottom, with no init()
// function to isolate a failure. scripts/check-i18n.js assertion 7 enforces it.
//
// FILENAME: no hyphen. juce_add_binary_data STRIPS hyphens rather than
// converting them to underscores, so a second file named i18n-fr.js would have
// to be reached as the symbol i18nfr_js (critical_binary_data_strips_hyphens).
// One combined file for both languages sidesteps the question entirely.
//
// COPY IS textContent ON EVERY PATH — never innerHTML. showTip() builds the tip
// with createElement + textContent, and check-i18n assertion 9 rejects any
// innerHTML reference here and any string literal containing `<`. A line break,
// if one is ever needed, is \n plus CSS white-space: pre-line, never a tag.
//
// THE ENGLISH WAS MOVED, NOT REWRITTEN. Every en entry below is byte-for-byte
// what index.html carried through v1.13.0, extracted mechanically rather than
// re-typed, with its HTML entities decoded to the characters they named
// (&amp; -> &) because setAttribute + textContent do not decode entities.
//
// KEYS ARE THE PARAMETER ID where the anchor has one, and the element id
// otherwise. 41 of the 53 anchors on this page carry NO id — they are `.ctl`
// and `.g-group` wrappers around a `[data-param]` knob — so binding by id was
// never an option here. The canonical [selector, key, wrapper] triple is what
// addresses them: the selector finds the knob, and closest(wrapper) walks back
// up to the cell the tip actually belongs on.
//
// ALL FRENCH IS MACHINE-DRAFTED AND FLAGGED `reviewed: false`. No native
// speaker has read it. `node scripts/check-i18n.js` prints the worklist.
// ============================================================================

export const LANGUAGES = ['en', 'fr'];

export const I18N = Object.freeze({

    // ── The settings popover (v1.14.0) ──────────────────────────────────────
    // The gear is new. The hover-help entry below is the v1.12.0 "?" toggle's
    // copy, MOVED here unchanged along with the control itself — not duplicated.
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

    'preset-prev': {
        en: { t: 'Previous preset',
              b: 'Step back through the preset list.' },
        fr: { t: 'Préréglage précédent',
              b: 'Revenir en arrière dans la liste des préréglages.',
              reviewed: false },
    },

    'preset-next': {
        en: { t: 'Next preset',
              b: 'Step forward through the preset list.' },
        fr: { t: 'Préréglage suivant',
              b: 'Avancer dans la liste des préréglages.',
              reviewed: false },
    },

    'preset-select': {
        en: { t: 'Preset',
              b: 'The preset currently loaded — click to browse all of them by category. The 28 factory presets are read-only; saving under the same name writes a user copy instead.' },
        fr: { t: 'Préréglage',
              b: 'Le préréglage actuellement chargé — cliquer pour les parcourir tous par catégorie. Les 28 préréglages d’usine sont en lecture seule ; enregistrer sous le même nom crée une copie utilisateur à la place.',
              reviewed: false },
    },

    'preset-save': {
        en: { t: 'Save',
              b: 'Save the current settings as a user preset.' },
        fr: { t: 'Enregistrer',
              b: 'Enregistrer les réglages actuels comme préréglage utilisateur.',
              reviewed: false },
    },

    'preset-load': {
        en: { t: 'Load',
              b: 'Load a preset from a file.' },
        fr: { t: 'Charger',
              b: 'Charger un préréglage depuis un fichier.',
              reviewed: false },
    },

    'preset-delete': {
        en: { t: 'Delete',
              b: 'Delete the current user preset. Click once to arm it, again to confirm.' },
        fr: { t: 'Supprimer',
              b: 'Supprimer le préréglage utilisateur actuel. Un premier clic arme, un second confirme.',
              reviewed: false },
    },

    'help-toggle': {
        en: { t: 'Hover help',
              b: 'Show a short description when the pointer rests on a control. The setting is remembered with the session.' },
        fr: { t: 'Aide au survol',
              b: 'Affiche une courte description lorsque le pointeur s’arrête sur une commande. Le réglage est conservé avec la session.',
              reviewed: false },
    },

    'TAPE_ENABLE': {
        en: { t: 'Tape',
              b: 'Enable the tape family — stop gestures, oxide dropouts, wow and flutter, and hiss.' },
        fr: { t: 'Bande',
              b: 'Active la famille bande — arrêts de défilement, pertes d’oxyde, pleurage et scintillement, et souffle.',
              reviewed: false },
    },

    'TAPE_PROB': {
        en: { t: 'Probability',
              b: 'How often the tape family fires an event, per clock tick. At 0 the family is silent even while enabled.' },
        fr: { t: 'Probabilité',
              b: 'À quelle fréquence la famille bande déclenche un événement, par top d’horloge. À 0 la famille reste muette même si elle est activée.',
              reviewed: false },
    },

    'TAPE_STOP_PROB': {
        en: { t: 'Stop share',
              b: 'The share of tape events that become a full stop-and-restart gesture rather than a dropout.' },
        fr: { t: 'Part d’arrêts',
              b: 'La part des événements de bande qui deviennent un arrêt-redémarrage complet plutôt qu’une perte de niveau.',
              reviewed: false },
    },

    'TAPE_DROP': {
        en: { t: 'Dropout share',
              b: 'The share of tape events that become an oxide dropout — a dip to 10–70% of level with a filter blended in. Real dropouts almost never mute; a full mute reads as an edit.' },
        fr: { t: 'Part de pertes',
              b: 'La part des événements de bande qui deviennent une perte d’oxyde — une chute à 10–70 % du niveau avec un filtre mélangé. Les vraies pertes ne coupent presque jamais ; une coupure franche s’entend comme un montage.',
              reviewed: false },
    },

    'TAPE_WOW': {
        en: { t: 'Wow & flutter',
              b: 'Depth of the slow speed drift and its faster flutter. This modulates the read rate, so its slope is pitch.' },
        fr: { t: 'Pleurage et scintillement',
              b: 'Profondeur de la dérive lente de vitesse et de son scintillement plus rapide. Ceci module la vitesse de lecture : sa pente est donc une hauteur.',
              reviewed: false },
    },

    'TAPE_HISS': {
        en: { t: 'Hiss',
              b: 'Level of the tape hiss bed. Runs whenever the family is enabled, independent of events.' },
        fr: { t: 'Souffle',
              b: 'Niveau du lit de souffle de bande. Il joue dès que la famille est activée, indépendamment des événements.',
              reviewed: false },
    },

    'TAPE_RAMP': {
        en: { t: 'Stop ramp',
              b: 'How long a stop gesture takes to spin down and back up again. 20–500 ms.' },
        fr: { t: 'Rampe d’arrêt',
              b: 'Le temps que met un arrêt à ralentir puis à repartir. 20–500 ms.',
              reviewed: false },
    },

    'CD_ENABLE': {
        en: { t: 'CD Skip',
              b: 'Enable the CD family — anti-shock loop stutters, sector-quantised buzz, and servo seeks.' },
        fr: { t: 'Saut de CD',
              b: 'Active la famille CD — bégaiements de boucle anti-choc, bourdonnement quantifié au secteur, et recherches de servo.',
              reviewed: false },
    },

    'CD_PROB': {
        en: { t: 'Probability',
              b: 'How often the CD family fires a skip, per clock tick.' },
        fr: { t: 'Probabilité',
              b: 'À quelle fréquence la famille CD déclenche un saut, par top d’horloge.',
              reviewed: false },
    },

    'CD_SEVERITY': {
        en: { t: 'Severity',
              b: 'How broken the disc is. Past the upper thresholds, loop windows quantise to the sector quantum — the 75 Hz-family buzz of a real anti-shock loop — and releases go through a servo seek instead of recovering instantly.' },
        fr: { t: 'Gravité',
              b: 'À quel point le disque est abîmé. Au-delà des seuils hauts, les fenêtres de boucle se quantifient sur le pas de secteur — le bourdonnement de la famille des 75 Hz d’une vraie boucle anti-choc — et les relâchements passent par une recherche de servo au lieu de se rétablir instantanément.',
              reviewed: false },
    },

    'CD_SEGMENT': {
        en: { t: 'Loop length',
              b: 'Length of the segment an anti-shock stutter repeats. 10–400 ms.' },
        fr: { t: 'Longueur de boucle',
              b: 'Longueur du segment que répète un bégaiement anti-choc. 10–400 ms.',
              reviewed: false },
    },

    'VINYL_ENABLE': {
        en: { t: 'Vinyl',
              b: 'Enable the vinyl family — groove jumps and locked grooves, surface pops, wear and warp.' },
        fr: { t: 'Vinyle',
              b: 'Active la famille vinyle — sauts de sillon et sillons fermés, craquements de surface, usure et voile.',
              reviewed: false },
    },

    'VINYL_PROB': {
        en: { t: 'Probability',
              b: 'How often the vinyl family fires a groove jump or a locked groove, per clock tick.' },
        fr: { t: 'Probabilité',
              b: 'À quelle fréquence la famille vinyle déclenche un saut de sillon ou un sillon fermé, par top d’horloge.',
              reviewed: false },
    },

    'VINYL_RPM': {
        en: { t: 'Speed',
              b: 'Disc speed. Sets the revolution period, which both the groove-jump distance and the warp wobble are locked to.' },
        fr: { t: 'Vitesse',
              b: 'Vitesse du disque. Fixe la période de révolution, à laquelle sont asservis la distance du saut de sillon et l’ondulation du voile.',
              reviewed: false },
    },

    'VINYL_POP': {
        en: { t: 'Pops',
              b: 'Density of surface crackle and pops. Runs whenever the family is enabled, independent of events.' },
        fr: { t: 'Craquements',
              b: 'Densité des craquements et crépitements de surface. Ils jouent dès que la famille est activée, indépendamment des événements.',
              reviewed: false },
    },

    'VINYL_WEAR': {
        en: { t: 'Wear',
              b: 'Level of the worn-groove noise bed — the dull roar under a played-out record.' },
        fr: { t: 'Usure',
              b: 'Niveau du lit de bruit de sillon usé — le grondement sourd sous un disque trop joué.',
              reviewed: false },
    },

    'VINYL_WARP': {
        en: { t: 'Warp',
              b: 'Depth of the once-per-revolution pitch wobble of a warped disc. At 100% the read rate deviates 0.6%, which is the far end of what a visibly warped LP does.' },
        fr: { t: 'Voile',
              b: 'Profondeur de l’ondulation de hauteur d’un disque voilé, une fois par tour. À 100 % la vitesse de lecture dévie de 0,6 %, ce qui correspond à l’extrême d’un microsillon visiblement voilé.',
              reviewed: false },
    },

    'PACKET_ENABLE': {
        en: { t: 'Packet',
              b: 'Enable the packet family — dropped 20 ms packets in bursts, with a concealment strategy.' },
        fr: { t: 'Paquets',
              b: 'Active la famille paquets — pertes de paquets de 20 ms en rafales, avec une stratégie de dissimulation.',
              reviewed: false },
    },

    'PACKET_LOSS': {
        en: { t: 'Loss rate',
              b: 'Share of 20 ms packets that fail to arrive.' },
        fr: { t: 'Taux de perte',
              b: 'Part des paquets de 20 ms qui n’arrivent pas.',
              reviewed: false },
    },

    'PACKET_BURST': {
        en: { t: 'Burstiness',
              b: 'How much losses clump. At 0 they are independent; higher values hold the chain in its bad state, so packets drop in runs rather than singly.' },
        fr: { t: 'Groupement',
              b: 'À quel point les pertes se groupent. À 0 elles sont indépendantes ; plus haut, la chaîne reste dans son mauvais état et les paquets tombent par séries plutôt qu’un par un.',
              reviewed: false },
    },

    'PACKET_CONCEAL': {
        en: { t: 'Concealment',
              b: 'What the decoder does with a missing packet — go Silent, Repeat the last one, let it Decay, or Substitute new material.' },
        fr: { t: 'Dissimulation',
              b: 'Ce que fait le décodeur d’un paquet manquant — se taire, répéter le précédent, le laisser décroître, ou lui substituer de la matière nouvelle.',
              reviewed: false },
    },

    'PACKET_COMFORT': {
        en: { t: 'Comfort noise',
              b: 'Level of the comfort noise injected under concealed packets, as a real codec does to keep the line from sounding dead.' },
        fr: { t: 'Bruit de confort',
              b: 'Niveau du bruit de confort injecté sous les paquets dissimulés, comme le fait un vrai codec pour éviter que la ligne ne semble morte.',
              reviewed: false },
    },

    'CODEC_ENABLE': {
        en: { t: 'Codec',
              b: 'Enable the codec family — a telephone chain: band-limit, μ-law or GSM coding, AGC and line noise.' },
        fr: { t: 'Codec',
              b: 'Active la famille codec — une chaîne téléphonique : limitation de bande, codage μ-law ou GSM, correction automatique de gain et bruit de ligne.',
              reviewed: false },
    },

    'CODEC_MODE': {
        en: { t: 'Line coding',
              b: 'μ-law companding or GSM full-rate. GSM adds frame structure, so a lost packet takes its whole frame with it.' },
        fr: { t: 'Codage de ligne',
              b: 'Compression μ-law ou GSM plein débit. Le GSM ajoute une structure de trames : un paquet perdu emporte donc toute sa trame.',
              reviewed: false },
    },

    'CODEC_MIX': {
        en: { t: 'Blend',
              b: 'How much of the coded signal replaces the dry one through this stage.' },
        fr: { t: 'Dosage',
              b: 'Quelle part du signal codé remplace le signal direct à travers cet étage.',
              reviewed: false },
    },

    'CODEC_AGC': {
        en: { t: 'AGC',
              b: 'Depth of the fast automatic gain control after the codec — a large part of why a phone sounds like a phone. At 0 the gain is exactly unity.' },
        fr: { t: 'Gain automatique',
              b: 'Profondeur de la correction automatique de gain rapide après le codec — une grande part de ce qui fait qu’un téléphone sonne comme un téléphone. À 0 le gain est exactement unitaire.',
              reviewed: false },
    },

    'CODEC_MAINS': {
        en: { t: 'Mains',
              b: 'Hum frequency and its harmonics in the line-noise bed. Inert while Noise is 0.' },
        fr: { t: 'Secteur',
              b: 'Fréquence du ronflement secteur et de ses harmoniques dans le lit de bruit de ligne. Sans effet tant que Bruit de ligne est à 0.',
              reviewed: false },
    },

    'CODEC_NOISE': {
        en: { t: 'Line noise',
              b: 'Level of the line-noise bed — mains hum plus the hiss of a bad connection.' },
        fr: { t: 'Bruit de ligne',
              b: 'Niveau du lit de bruit de ligne — ronflement secteur et souffle d’une mauvaise connexion.',
              reviewed: false },
    },

    'CRUSH_ENABLE': {
        en: { t: 'Crush',
              b: 'Enable the crush family — bit-depth reduction, sample-rate decimation with jitter, and dither.' },
        fr: { t: 'Écrasement',
              b: 'Active la famille écrasement — réduction de résolution, décimation de fréquence d’échantillonnage avec gigue, et dithering.',
              reviewed: false },
    },

    'CRUSH_BITS': {
        en: { t: 'Bit depth',
              b: 'Quantisation depth, 1–16 bits. At 16 the stage is bit-transparent.' },
        fr: { t: 'Résolution',
              b: 'Profondeur de quantification, 1–16 bits. À 16 l’étage est transparent au bit près.',
              reviewed: false },
    },

    'CRUSH_RATE': {
        en: { t: 'Sample rate',
              b: 'Decimation rate — the grid the signal is re-sampled onto. 500 Hz to 20 kHz.' },
        fr: { t: 'Fréquence d’échantillonnage',
              b: 'Fréquence de décimation — la grille sur laquelle le signal est ré-échantillonné. De 500 Hz à 20 kHz.',
              reviewed: false },
    },

    'CRUSH_JITTER': {
        en: { t: 'Jitter',
              b: 'Random timing error on the decimation grid, so crossings land off the clock.' },
        fr: { t: 'Gigue',
              b: 'Erreur temporelle aléatoire sur la grille de décimation, si bien que les passages tombent à côté de l’horloge.',
              reviewed: false },
    },

    'CRUSH_ENV_AMT': {
        en: { t: 'Envelope',
              b: 'Bipolar: how much the input envelope pushes bit depth around. Positive cleans up loud passages, negative dirties them.' },
        fr: { t: 'Enveloppe',
              b: 'Bipolaire : dans quelle mesure l’enveloppe d’entrée fait varier la résolution. En positif, les passages forts se nettoient ; en négatif, ils se salissent.',
              reviewed: false },
    },

    'CRUSH_DITHER': {
        en: { t: 'Dither',
              b: 'Noise added before quantisation, in LSBs — trades quantisation distortion for a steady noise floor.' },
        fr: { t: 'Dithering',
              b: 'Bruit ajouté avant la quantification, en LSB — il échange la distorsion de quantification contre un plancher de bruit stable.',
              reviewed: false },
    },

    'ROT_ENABLE': {
        en: { t: 'Rot',
              b: 'Enable the rot family — bit flips, sticky decode holds, and wrong-decode garble stretches. While off it takes no random draws at all, so a pre-1.10 session renders bit-identically.' },
        fr: { t: 'Corruption',
              b: 'Active la famille corruption — inversions de bits, blocages de décodage, et plages de décodage erroné. Désactivée, elle ne tire aucun nombre aléatoire : une session antérieure à la 1.10 rend donc un résultat identique au bit près.',
              reviewed: false },
    },

    'ROT_PROB': {
        en: { t: 'Probability',
              b: 'How often the rot family fires an event, per clock tick.' },
        fr: { t: 'Probabilité',
              b: 'À quelle fréquence la famille corruption déclenche un événement, par top d’horloge.',
              reviewed: false },
    },

    'ROT_DEPTH': {
        en: { t: 'Flip severity',
              b: 'Sweeps the bit-flip rate from an occasional tick to a dense digital hash, and opens the reachable bit field from bit 3 up to bit 14. At most one sample in four is ever touched.' },
        fr: { t: 'Gravité d’inversion',
              b: 'Fait passer le taux d’inversion de bits d’un tic occasionnel à un hachis numérique dense, et ouvre le champ de bits atteignable du bit 3 jusqu’au bit 14. Au plus un échantillon sur quatre est jamais touché.',
              reviewed: false },
    },

    'ROT_STICK': {
        en: { t: 'Sticky share',
              b: 'The share of rot events that become a sticky decode hold — the decoder hangs on one value.' },
        fr: { t: 'Part de blocages',
              b: 'La part des événements de corruption qui deviennent un blocage de décodage — le décodeur reste accroché à une seule valeur.',
              reviewed: false },
    },

    'ROT_GARBLE': {
        en: { t: 'Garble share',
              b: 'The share of the remaining rot events that become a wrong-decode stretch. Whatever survives both shares is a bit-flip window.' },
        fr: { t: 'Part de brouillage',
              b: 'La part des événements de corruption restants qui deviennent une plage de décodage erroné. Ce qui survit aux deux parts est une fenêtre d’inversion de bits.',
              reviewed: false },
    },

    'clockModeSeg': {
        en: { t: 'Clock',
              b: 'Whether events are scheduled against the host tempo or a free-running rate.' },
        fr: { t: 'Horloge',
              b: 'Si les événements sont cadencés sur le tempo de l’hôte ou sur une fréquence libre.',
              reviewed: false },
    },

    'CLOCK_SYNC_DIV': {
        en: { t: 'Division',
              b: 'Musical division the event clock ticks on, locked to host tempo.' },
        fr: { t: 'Division',
              b: 'Division musicale sur laquelle bat l’horloge d’événements, asservie au tempo de l’hôte.',
              reviewed: false },
    },

    'viewFree': {
        en: { t: 'Free rate',
              b: 'Free-running event clock rate, 0.1–20 Hz. Ignores host tempo.' },
        fr: { t: 'Fréquence libre',
              b: 'Fréquence de l’horloge d’événements en marche libre, 0,1–20 Hz. Ignore le tempo de l’hôte.',
              reviewed: false },
    },

    'seedRo': {
        en: { t: 'Seed',
              b: 'The seed every random stream is derived from. The same seed at the same transport position gives the same events on every render.' },
        fr: { t: 'Germe',
              b: 'Le germe dont dérive chaque flux aléatoire. Le même germe à la même position de transport donne les mêmes événements à chaque rendu.',
              reviewed: false },
    },

    'diceBtn': {
        en: { t: 'Reseed',
              b: 'Draw a new seed. Everything stochastic re-rolls, so the take becomes a different one.' },
        fr: { t: 'Retirer un germe',
              b: 'Tirer un nouveau germe. Tout ce qui est stochastique est relancé : la prise devient donc une autre prise.',
              reviewed: false },
    },

    'edgeBtn': {
        en: { t: 'Hard edges',
              b: 'Bypass the short crossfades at event boundaries, so entries and exits become true steps. Lit means bypassed.' },
        fr: { t: 'Fronts francs',
              b: 'Contourne les courts fondus aux limites des événements, si bien que les entrées et les sorties deviennent de vraies marches. Allumé signifie contourné.',
              reviewed: false },
    },

    'MIX': {
        en: { t: 'Mix',
              b: 'Dry/wet blend of the whole processed chain.' },
        fr: { t: 'Dosage',
              b: 'Équilibre direct/traité de toute la chaîne de traitement.',
              reviewed: false },
    },
});

// [selector, key] or [selector, key, wrapperSelector]. The selector is the
// BINDING SITE, and on this page it usually is NOT the element carrying the
// key: most controls here are an idless wrapper around a [data-param] knob, so
// the tip binds through the knob and `closest(wrapper)` walks back up to the
// cell the tip belongs on. That is exactly what the wrapper slot is for.
export const TIP_BINDINGS = [
    ['#gear-btn',                        'settings'],
    ['#lang-select',                     'lang-select'],

    ['#preset-prev',                     'preset-prev'],
    ['#preset-next',                     'preset-next'],
    ['#preset-select',                   'preset-select'],
    ['#preset-save',                     'preset-save'],
    ['#preset-load',                     'preset-load'],
    ['#preset-delete',                   'preset-delete'],
    ['#help-toggle',                     'help-toggle'],
    ['[data-param="TAPE_ENABLE"]',       'TAPE_ENABLE'],
    ['[data-param="TAPE_PROB"]',         'TAPE_PROB',          '.ctl'],
    ['[data-param="TAPE_STOP_PROB"]',    'TAPE_STOP_PROB',     '.ctl'],
    ['[data-param="TAPE_DROP"]',         'TAPE_DROP',          '.ctl'],
    ['[data-param="TAPE_WOW"]',          'TAPE_WOW',           '.ctl'],
    ['[data-param="TAPE_HISS"]',         'TAPE_HISS',          '.ctl'],
    ['[data-param="TAPE_RAMP"]',         'TAPE_RAMP',          '.ctl'],
    ['[data-param="CD_ENABLE"]',         'CD_ENABLE'],
    ['[data-param="CD_PROB"]',           'CD_PROB',            '.ctl'],
    ['[data-param="CD_SEVERITY"]',       'CD_SEVERITY',        '.ctl'],
    ['[data-param="CD_SEGMENT"]',        'CD_SEGMENT',         '.ctl'],
    ['[data-param="VINYL_ENABLE"]',      'VINYL_ENABLE'],
    ['[data-param="VINYL_PROB"]',        'VINYL_PROB',         '.ctl'],
    ['[data-param="VINYL_RPM"]',         'VINYL_RPM',          '.ctl'],
    ['[data-param="VINYL_POP"]',         'VINYL_POP',          '.ctl'],
    ['[data-param="VINYL_WEAR"]',        'VINYL_WEAR',         '.ctl'],
    ['[data-param="VINYL_WARP"]',        'VINYL_WARP',         '.ctl'],
    ['[data-param="PACKET_ENABLE"]',     'PACKET_ENABLE'],
    ['[data-param="PACKET_LOSS"]',       'PACKET_LOSS',        '.ctl'],
    ['[data-param="PACKET_BURST"]',      'PACKET_BURST',       '.ctl'],
    ['[data-param="PACKET_CONCEAL"]',    'PACKET_CONCEAL',     '.ctl'],
    ['[data-param="PACKET_COMFORT"]',    'PACKET_COMFORT',     '.ctl'],
    ['[data-param="CODEC_ENABLE"]',      'CODEC_ENABLE'],
    ['[data-param="CODEC_MODE"]',        'CODEC_MODE',         '.ctl'],
    ['[data-param="CODEC_MIX"]',         'CODEC_MIX',          '.ctl'],
    ['[data-param="CODEC_AGC"]',         'CODEC_AGC',          '.ctl'],
    ['[data-param="CODEC_MAINS"]',       'CODEC_MAINS',        '.ctl'],
    ['[data-param="CODEC_NOISE"]',       'CODEC_NOISE',        '.ctl'],
    ['[data-param="CRUSH_ENABLE"]',      'CRUSH_ENABLE'],
    ['[data-param="CRUSH_BITS"]',        'CRUSH_BITS',         '.ctl'],
    ['[data-param="CRUSH_RATE"]',        'CRUSH_RATE',         '.ctl'],
    ['[data-param="CRUSH_JITTER"]',      'CRUSH_JITTER',       '.ctl'],
    ['[data-param="CRUSH_ENV_AMT"]',     'CRUSH_ENV_AMT',      '.ctl'],
    ['[data-param="CRUSH_DITHER"]',      'CRUSH_DITHER',       '.ctl'],
    ['[data-param="ROT_ENABLE"]',        'ROT_ENABLE'],
    ['[data-param="ROT_PROB"]',          'ROT_PROB',           '.g-group'],
    ['[data-param="ROT_DEPTH"]',         'ROT_DEPTH',          '.g-group'],
    ['[data-param="ROT_STICK"]',         'ROT_STICK',          '.g-group'],
    ['[data-param="ROT_GARBLE"]',        'ROT_GARBLE',         '.g-group'],
    ['#clockModeSeg',                    'clockModeSeg'],
    ['[data-param="CLOCK_SYNC_DIV"]',    'CLOCK_SYNC_DIV'],
    ['#viewFree',                        'viewFree'],
    ['#seedRo',                          'seedRo'],
    ['#diceBtn',                         'diceBtn'],
    ['#edgeBtn',                         'edgeBtn'],
    ['[data-param="MIX"]',               'MIX',                '.g-group'],
];

export function tr(key, lang, vars) {
    const entry = I18N[key];
    if (!entry) { console.warn(`i18n: missing key ${key}`); return { t: key, b: '' }; }
    const s = entry[lang] || entry.en;

    // A var VALUE that is itself an I18N key resolves to that key's localized
    // title; anything else is used literally. This plugin needs neither arm
    // today, but the resolving arm is what lets a plugin compose a localized
    // name into a tip without pinning TIP_BINDINGS — which is static data
    // evaluated once — to the load-time language. The canon is one shape across
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
