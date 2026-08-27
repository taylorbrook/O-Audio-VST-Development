/*
   This file is part of O-Tapestop, an Ouaricon Audio plugin.
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
// i18n.js — O-Tapestop hover-help copy, English + French (v1.5.0)
//
// An ES module that EXPORTS ONLY. It must never self-execute: a bare top-level
// statement here throws out of module evaluation and takes every later
// initializer on the page with it (pattern_module_toplevel_init_tdz).
// scripts/check-i18n.js assertion 7 enforces it.
//
// FILENAME: no hyphen. juce_add_binary_data STRIPS hyphens rather than
// converting them to underscores, so a second file named i18n-fr.js would have
// to be reached as the symbol i18nfr_js (critical_binary_data_strips_hyphens).
// One combined file for both languages sidesteps the question entirely.
//
// COPY IS textContent ON EVERY PATH — never innerHTML. showTooltip() in app.js
// builds the tip with createElement + textContent, and check-i18n assertion 9
// rejects any innerHTML reference here and any string literal containing `<`.
// A line break, if one is ever needed, is \n plus CSS white-space: pre-line,
// never a markup tag.
//
// THE ENGLISH WAS MOVED, NOT REWRITTEN. Every en entry below is byte-for-byte
// what index.html carried through v1.4.0, extracted mechanically rather than
// re-typed, with its HTML entities decoded to the characters they named
// (&#8212; -> —, &#215; -> ×) because setAttribute + textContent do not decode.
//
// KEYS ARE THE ANCHOR'S OWN ID, or its first id'd DESCENDANT where the anchor
// itself carries none. 17 of the 33 anchors here are `.knob-cell`,
// `.select-cell`, `.ratio-cell` and `.env-plate` wrappers with no id of their
// own, so the canonical [selector, key, wrapper] triple addresses them: the
// selector finds the id'd child and closest(wrapper) walks back up to the cell
// the tip belongs on.
//
// Note WHY the child rather than the wrapper's class. The sync/free swap slots
// put a `.select-cell` and a `.knob-cell` back to back under the SAME tip title
// — "Spin-Down Time" appears twice, once as a note division and once in
// milliseconds — so a class-based key would collide. The id'd children
// (#combo-STOP_SYNC_DIV, #knob-STOP_FREE_MS) do not.
//
// ALL FRENCH IS MACHINE-DRAFTED AND FLAGGED `reviewed: false`. No native
// speaker has read it. `node scripts/check-i18n.js` prints the worklist.
// ============================================================================

export const LANGUAGES = ['en', 'fr'];

export const I18N = Object.freeze({

    // ── The settings popover (v1.5.0) ───────────────────────────────────────
    // The gear is new. The hover-help entry below is the v1.4.0 "?" toggle's copy,
    // MOVED here unchanged along with the control itself — not duplicated.
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

    'help-toggle': {
        en: { t: 'Hover Help',
              b: 'Turns the hover descriptions on and off for every control on this page. The setting is saved with the session.' },
        fr: { t: 'Aide au survol',
              b: 'Active ou désactive les descriptions au survol pour toutes les commandes de cette page. Le réglage est conservé avec la session.',
              reviewed: false },
    },

    'preset-prev': {
        en: { t: 'Previous',
              b: 'Steps back one entry through the preset list, factory and user alike.' },
        fr: { t: 'Précédent',
              b: 'Recule d’une entrée dans la liste des préréglages, d’usine comme utilisateur.',
              reviewed: false },
    },

    'preset-next': {
        en: { t: 'Next',
              b: 'Steps forward one entry through the preset list, factory and user alike.' },
        fr: { t: 'Suivant',
              b: 'Avance d’une entrée dans la liste des préréglages, d’usine comme utilisateur.',
              reviewed: false },
    },

    'preset-name': {
        en: { t: 'Preset',
              b: 'The loaded preset. Click to open the list, grouped into Tape Stops, Scratch, Wobble & Warp, and Glitch & Chaos.' },
        fr: { t: 'Préréglage',
              b: 'Le préréglage chargé. Cliquer pour ouvrir la liste, groupée en Arrêts de bande, Scratch, Pleurage et voile, et Glitch et chaos.',
              reviewed: false },
    },

    'preset-save': {
        en: { t: 'Save',
              b: 'Writes every control, including the drawn scratch envelope, to a user preset.' },
        fr: { t: 'Enregistrer',
              b: 'Écrit toutes les commandes, y compris l’enveloppe de scratch dessinée, dans un préréglage utilisateur.',
              reviewed: false },
    },

    'preset-load': {
        en: { t: 'Load',
              b: 'Opens a preset file from disk rather than from the list above.' },
        fr: { t: 'Charger',
              b: 'Ouvre un fichier de préréglage sur le disque plutôt que depuis la liste ci-dessus.',
              reviewed: false },
    },

    'preset-delete': {
        en: { t: 'Delete',
              b: 'Removes the loaded user preset. Click once to arm, again to confirm. Factory presets are protected.' },
        fr: { t: 'Supprimer',
              b: 'Supprime le préréglage utilisateur chargé. Un premier clic arme, un second confirme. Les préréglages d’usine sont protégés.',
              reviewed: false },
    },

    'engage-btn': {
        en: { t: 'Engage',
              b: 'The performance control. It latches: pressing it starts the gesture the current Mode describes, releasing it hands the transport back. Automate or MIDI-map this and leave the rest of the page alone.' },
        fr: { t: 'Enclencher',
              b: 'La commande de jeu. Elle se maintient : l’enfoncer démarre le geste que décrit le Mode courant, la relâcher rend la main au transport. Automatisez ou assignez-la en MIDI et laissez le reste de la page tranquille.',
              reviewed: false },
    },

    'seg-mode-stop': {
        en: { t: 'Stop Mode',
              b: 'The reel losing power. Engage spins the tape down to a halt; releasing spins it back up to speed. Spin Down and Spin Up shape each half separately.' },
        fr: { t: 'Mode arrêt',
              b: 'La bobine qui perd son alimentation. Enclencher fait ralentir la bande jusqu’à l’arrêt ; relâcher la fait remonter à sa vitesse. Ralentissement et Redémarrage façonnent chaque moitié séparément.',
              reviewed: false },
    },

    'seg-mode-scratch': {
        en: { t: 'Scratch Mode',
              b: 'A hand on the platter. Engage plays one pass of the drawn speed curve, reverse included, then returns to normal speed.' },
        fr: { t: 'Mode scratch',
              b: 'Une main sur le plateau. Enclencher joue un passage de la courbe de vitesse dessinée, marche arrière comprise, puis revient à la vitesse normale.',
              reviewed: false },
    },

    'seg-mode-cont': {
        en: { t: 'Motion Mode',
              b: 'Worn tape that never settles. Engage holds the speed in continuous modulation for as long as it stays lit — wobble, lurch or stutter, depending on Character.' },
        fr: { t: 'Mode mouvement',
              b: 'Une bande usée qui ne se stabilise jamais. Enclencher maintient la vitesse en modulation continue tant que le bouton reste allumé — pleurage, embardée ou bégaiement, selon le Caractère.',
              reviewed: false },
    },

    'seg-sync-sync': {
        en: { t: 'Sync',
              b: 'Every duration on this page locks to the host\'s tempo grid and is chosen as a note division. Switches all three panels at once.' },
        fr: { t: 'Synchronisé',
              b: 'Toutes les durées de cette page s’asservissent à la grille de tempo de l’hôte et se choisissent en divisions de note. Bascule les trois panneaux à la fois.',
              reviewed: false },
    },

    'seg-sync-free': {
        en: { t: 'Free',
              b: 'Every duration is set in milliseconds (or hertz, for Motion) and ignores the host tempo. Switches all three panels at once.' },
        fr: { t: 'Libre',
              b: 'Toutes les durées se règlent en millisecondes (ou en hertz, pour le Mouvement) et ignorent le tempo de l’hôte. Bascule les trois panneaux à la fois.',
              reviewed: false },
    },

    'ratio-fill': {
        en: { t: 'Playback Rate',
              b: 'Live readout of the transport\'s speed. 1× is normal, 0 is a dead stop, and anything left of zero is running backwards.' },
        fr: { t: 'Vitesse de lecture',
              b: 'Affichage en direct de la vitesse du transport. 1× est la vitesse normale, 0 l’arrêt complet, et tout ce qui est à gauche de zéro défile à l’envers.',
              reviewed: false },
    },

    'combo-STOP_SYNC_DIV': {
        en: { t: 'Spin-Down Time',
              b: 'How long the reel takes to reach a standstill, as a note division of the host tempo. Latched the moment Engage is pressed — moving it mid-gesture is inert.' },
        fr: { t: 'Durée de ralentissement',
              b: 'Le temps que met la bobine à atteindre l’arrêt complet, en division de note du tempo de l’hôte. Verrouillée à l’instant où Enclencher est pressé — la modifier en cours de geste est sans effet.',
              reviewed: false },
    },

    'knob-STOP_FREE_MS': {
        en: { t: 'Spin-Down Time',
              b: 'How long the reel takes to reach a standstill, in milliseconds. Latched the moment Engage is pressed.' },
        fr: { t: 'Durée de ralentissement',
              b: 'Le temps que met la bobine à atteindre l’arrêt complet, en millisecondes. Verrouillée à l’instant où Enclencher est pressé.',
              reviewed: false },
    },

    'knob-STOP_CURVE': {
        en: { t: 'Spin-Down Curve',
              b: 'Shapes the fall. At 0 the speed drops in a straight line; at 100 it plunges away at once and then crawls the last stretch. 50 is turntable physics.' },
        fr: { t: 'Courbe de ralentissement',
              b: 'Façonne la chute. À 0 la vitesse descend en ligne droite ; à 100 elle s’effondre d’un coup puis rampe sur la dernière portion. 50 correspond à la physique d’une platine.',
              reviewed: false },
    },

    'combo-START_SYNC_DIV': {
        en: { t: 'Spin-Up Time',
              b: 'How long the reel takes to regain full speed once Engage is released, as a note division of the host tempo.' },
        fr: { t: 'Durée de redémarrage',
              b: 'Le temps que met la bobine à retrouver sa pleine vitesse une fois Enclencher relâché, en division de note du tempo de l’hôte.',
              reviewed: false },
    },

    'knob-START_FREE_MS': {
        en: { t: 'Spin-Up Time',
              b: 'How long the reel takes to regain full speed once Engage is released, in milliseconds.' },
        fr: { t: 'Durée de redémarrage',
              b: 'Le temps que met la bobine à retrouver sa pleine vitesse une fois Enclencher relâché, en millisecondes.',
              reviewed: false },
    },

    'knob-START_CURVE': {
        en: { t: 'Spin-Up Curve',
              b: 'Shapes the return. At 0 the speed rises in a straight line; at 100 it hangs near a standstill and then rushes back at the end. 50 is turntable physics.' },
        fr: { t: 'Courbe de redémarrage',
              b: 'Façonne le retour. À 0 la vitesse monte en ligne droite ; à 100 elle traîne près de l’arrêt puis se précipite à la fin. 50 correspond à la physique d’une platine.',
              reviewed: false },
    },

    'envCanvas': {
        en: { t: 'Scratch Envelope',
              b: 'Speed across one pass. The 1× line is normal speed, 0 is a standstill, and everything below the 0 line plays backwards — down to −2×. Drag the points to reshape it.' },
        fr: { t: 'Enveloppe de scratch',
              b: 'La vitesse au long d’un passage. La ligne 1× est la vitesse normale, 0 l’immobilité, et tout ce qui passe sous la ligne 0 se lit à l’envers — jusqu’à −2×. Faites glisser les points pour la remodeler.',
              reviewed: false },
    },

    'combo-ENV_SYNC_DIV': {
        en: { t: 'Pass Length',
              b: 'How long one trip through the drawn envelope takes, as a note division of the host tempo.' },
        fr: { t: 'Durée du passage',
              b: 'Le temps que dure un parcours complet de l’enveloppe dessinée, en division de note du tempo de l’hôte.',
              reviewed: false },
    },

    'knob-ENV_FREE_MS': {
        en: { t: 'Pass Length',
              b: 'How long one trip through the drawn envelope takes, in milliseconds.' },
        fr: { t: 'Durée du passage',
              b: 'Le temps que dure un parcours complet de l’enveloppe dessinée, en millisecondes.',
              reviewed: false },
    },

    'seg-char-wobble': {
        en: { t: 'Wobble',
              b: 'Deterministic wow and flutter — a steady sine with a three-harmonic flutter band above it. The seasick end of worn tape.' },
        fr: { t: 'Pleurage',
              b: 'Pleurage et scintillement déterministes — une sinusoïde régulière surmontée d’une bande de scintillement à trois harmoniques. Le versant nauséeux de la bande usée.',
              reviewed: false },
    },

    'seg-char-random': {
        en: { t: 'Random',
              b: 'A drifting random walk across three time-scales at once. It never repeats and never quite settles.' },
        fr: { t: 'Aléatoire',
              b: 'Une marche aléatoire qui dérive sur trois échelles de temps à la fois. Elle ne se répète jamais et ne se pose jamais tout à fait.',
              reviewed: false },
    },

    'seg-char-glitch': {
        en: { t: 'Glitch',
              b: 'A grid scheduler firing discrete speed events — dips, half-speed stalls, reverse stabs. Chaos decides how many fire and how short they get.' },
        fr: { t: 'Glitch',
              b: 'Un séquenceur sur grille qui déclenche des événements de vitesse discrets — creux, blocages à mi-vitesse, coups en marche arrière. Le Chaos décide combien se déclenchent et à quel point ils raccourcissent.',
              reviewed: false },
    },

    'combo-CONT_RATE_SYNC_DIV': {
        en: { t: 'Motion Rate',
              b: 'How fast the motion cycles, as a note division of the host tempo.' },
        fr: { t: 'Vitesse du mouvement',
              b: 'À quelle cadence le mouvement se répète, en division de note du tempo de l’hôte.',
              reviewed: false },
    },

    'knob-CONT_RATE_HZ': {
        en: { t: 'Motion Rate',
              b: 'How fast the motion cycles, in hertz. 0.05 Hz is one slow sweep every twenty seconds; 20 Hz is a buzz.' },
        fr: { t: 'Vitesse du mouvement',
              b: 'À quelle cadence le mouvement se répète, en hertz. 0,05 Hz correspond à un balayage lent toutes les vingt secondes ; 20 Hz à un bourdonnement.',
              reviewed: false },
    },

    'knob-CONT_DEPTH': {
        en: { t: 'Motion Depth',
              b: 'Peak speed deviation, scaled by ear rather than linearly: 0 is barely two cents of drift, 50 is about one percent, 100 reaches roughly two semitones.' },
        fr: { t: 'Profondeur du mouvement',
              b: 'Écart de vitesse maximal, dosé à l’oreille plutôt que linéairement : 0 représente à peine deux centièmes de dérive, 50 environ un pour cent, et 100 atteint près de deux demi-tons.',
              reviewed: false },
    },

    'knob-CONT_CHAOS': {
        en: { t: 'Chaos',
              b: 'How irregular the motion is. At 0 each Character runs its tamest, most repeatable shape; raising it jitters Wobble, widens Random\'s drift, and unlocks faster, shorter Glitch events.' },
        fr: { t: 'Chaos',
              b: 'À quel point le mouvement est irrégulier. À 0, chaque Caractère joue sa forme la plus sage et la plus reproductible ; en montant, le Pleurage se met à trembler, la dérive de l’Aléatoire s’élargit, et les événements de Glitch deviennent plus rapides et plus courts.',
              reviewed: false },
    },

    'knob-TONE_TRACK': {
        en: { t: 'Tone Track',
              b: 'Darkens the varispeed path as the tape slows, the way tape and vinyl lose their highs off-speed. At 0 nothing is filtered; at 100 a full standstill reaches down to 150 Hz. Full speed is always open.' },
        fr: { t: 'Suivi de timbre',
              b: 'Assombrit le trajet en vitesse variable à mesure que la bande ralentit, comme la bande et le vinyle perdent leurs aigus hors vitesse. À 0 rien n’est filtré ; à 100 l’arrêt complet descend jusqu’à 150 Hz. À pleine vitesse le trajet reste toujours ouvert.',
              reviewed: false },
    },

    'knob-MIX': {
        en: { t: 'Mix',
              b: 'Balance of the dry input against the varispeed path — a straight crossfade, so it doubles as a parallel blend. Applies to the engaged chain only; disengaged output is bit-for-bit dry.' },
        fr: { t: 'Dosage',
              b: 'Équilibre entre le signal direct et le trajet en vitesse variable — un simple fondu croisé, qui sert donc aussi de mélange parallèle. Ne s’applique qu’à la chaîne enclenchée ; hors enclenchement la sortie est le signal direct au bit près.',
              reviewed: false },
    },

    'knob-OUTPUT_GAIN': {
        en: { t: 'Output Gain',
              b: 'Final trim, −24 to +12 dB. Like Mix, it rides the engaged chain only and glides back to unity across the resync fade.' },
        fr: { t: 'Gain de sortie',
              b: 'Ajustement final, de −24 à +12 dB. Comme le Dosage, il ne porte que sur la chaîne enclenchée et revient progressivement à l’unité pendant le fondu de resynchronisation.',
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

    ['#help-toggle',                     'help-toggle'],
    ['#preset-prev',                     'preset-prev'],
    ['#preset-next',                     'preset-next'],
    ['#preset-name',                     'preset-name'],
    ['#preset-save',                     'preset-save'],
    ['#preset-load',                     'preset-load'],
    ['#preset-delete',                   'preset-delete'],
    ['#engage-btn',                      'engage-btn'],
    ['#seg-mode-stop',                   'seg-mode-stop'],
    ['#seg-mode-scratch',                'seg-mode-scratch'],
    ['#seg-mode-cont',                   'seg-mode-cont'],
    ['#seg-sync-sync',                   'seg-sync-sync'],
    ['#seg-sync-free',                   'seg-sync-free'],
    ['#ratio-fill',                      'ratio-fill',         '.ratio-cell'],
    ['#combo-STOP_SYNC_DIV',             'combo-STOP_SYNC_DIV', '.select-cell'],
    ['#knob-STOP_FREE_MS',               'knob-STOP_FREE_MS',  '.knob-cell'],
    ['#knob-STOP_CURVE',                 'knob-STOP_CURVE',    '.knob-cell'],
    ['#combo-START_SYNC_DIV',            'combo-START_SYNC_DIV', '.select-cell'],
    ['#knob-START_FREE_MS',              'knob-START_FREE_MS', '.knob-cell'],
    ['#knob-START_CURVE',                'knob-START_CURVE',   '.knob-cell'],
    ['#envCanvas',                       'envCanvas',          '.env-plate'],
    ['#combo-ENV_SYNC_DIV',              'combo-ENV_SYNC_DIV', '.select-cell'],
    ['#knob-ENV_FREE_MS',                'knob-ENV_FREE_MS',   '.knob-cell'],
    ['#seg-char-wobble',                 'seg-char-wobble'],
    ['#seg-char-random',                 'seg-char-random'],
    ['#seg-char-glitch',                 'seg-char-glitch'],
    ['#combo-CONT_RATE_SYNC_DIV',        'combo-CONT_RATE_SYNC_DIV', '.select-cell'],
    ['#knob-CONT_RATE_HZ',               'knob-CONT_RATE_HZ',  '.knob-cell'],
    ['#knob-CONT_DEPTH',                 'knob-CONT_DEPTH',    '.knob-cell'],
    ['#knob-CONT_CHAOS',                 'knob-CONT_CHAOS',    '.knob-cell'],
    ['#knob-TONE_TRACK',                 'knob-TONE_TRACK',    '.knob-cell'],
    ['#knob-MIX',                        'knob-MIX',           '.knob-cell'],
    ['#knob-OUTPUT_GAIN',                'knob-OUTPUT_GAIN',   '.knob-cell'],
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
