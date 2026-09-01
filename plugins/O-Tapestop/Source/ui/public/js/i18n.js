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
// i18n.js — O-Tapestop hover-help copy, English + French (v1.6.2)
//
// ── v1.6.2: ENGLISH DEFECTS FOUND BY THE FRENCH (Stage O, 2026-08-31) ──────
// Two defects Stage N found by reading the French against the code, fixed
// in both languages in one commit. reviewed: false on every entry whose
// French MEANING changed (4) and on every new entry (5) — 9 of 79 to re-read.
//
//  * Item 38 — the three division <select>s (#combo-STOP_SYNC_DIV,
//    #combo-START_SYNC_DIV, #combo-ENV_SYNC_DIV) carried the accessible
//    names "Stop Time" / "Start Time" / "Env Length" while their hover-help
//    titles read "Spin-Down Time" / "Spin-Up Time" / "Pass Length" and their
//    visible caption reads "Division": three names for one control, and
//    the accessible name contained neither of the other two (WCAG 2.5.3,
//    label-in-name). The name is now the tip title plus what the control IS:
//    "Spin-Down Time division", and in French "Division de la durée de
//    ralentissement" — the tip title's settled form (Durée de ralentissement
//    / de redémarrage / du passage, all reviewed) plus the caption's own
//    word, so both the caption and the title are substrings of the name in
//    both languages. No new French form for spin-down / spin-up / pass.
//
//  * Item 39 — the preset dropdown's four theme headings (PRESET_THEMES in
//    app.js) were English literals written by textContent, so the French
//    page showed "Tape Stops" / "Wobble & Warp" as group names, and the v1.6.1
//    preset-name body had to name them in English to be truthful. They are
//    now `label.theme*` keys written through setLabel() (plus the trailing
//    "User" group, same mechanism, same defect), so a language switch
//    re-renders an OPEN dropdown, and preset-name's French body names the
//    French headings. French forms: "Arrêts de bande" (the page's own word
//    for tape), "Scratch" (as label.modeScratch), "Pleurage & Déformation"
//    (the Wobble tip title's root + the glossary's warp), "Glitch & Chaos"
//    (both French as they stand), "Utilisateur" (as the preset tips say
//    "préréglage utilisateur"). Straight copies carry sameAsEn: true — these
//    are LABELS entries, one string, so the flag disarms nothing (cf. the
//    v1.6.1 note on tooltip TITLES below, which is about I18N entries).
//
// ── v1.6.1: FRENCH QA PASS (Stage N, 2026-08-31) ───────────────────────────
// Every fr entry read against its en and against scripts/i18n-fr-glossary.js.
// Changed: 21 of 109 rows (7 terminology, 14 typography, 5 grammar/register,
// 2 meaning — 29 edits, several rows carrying more than one). sameAsEn: kept 8,
// translated 0. termNote exemptions: 1 (label.timing, below).
// Left as drafted: the rest. reviewed: false throughout — no native speaker yet.
//
// The decisions the next reader needs:
//
//  * label.toneTrack takes the glossary ROOT, "Suivi de timbre". The v1.6.0
//    comment below defended "Suivi tonal" on width and was wrong on both
//    numbers: the string measures 91.97 px, not 97, and the constraint is not
//    the 88 px .knob-cell — .knob-label is shrink-to-fit with overflow visible,
//    so the caption grows past its cell into a 148 px .group-body and lands at
//    x=[699.02, 790.98] with 28.02 px of clearance on EACH side of the
//    .group-output padding box [671, 819]. Fourth header width defence in this
//    task proven backwards by measurement (after O-Comp, O-Chorus, O-AnalogEQ).
//
//  * label.timing keeps "Cadence" under a termNote. The glossary's "décalage"
//    is the rhythmic-offset sense of Timing; this caption heads the SYNC/FREE
//    pair and names a time BASE. There is no nudge control on this page.
//
//  * label.passLength keeps the short "Passage" while the two Pass Length tip
//    titles keep the root "Durée du passage". Both are glossary-accepted for
//    'pass length', so the two forms on one page are settled, not a split.
//
//  * knob-MIX's TITLE is now "Mix" (glossary root; "Dosage" was also an F1
//    forbidden word), but its BODY keeps "mélange parallèle" for the English
//    "parallel blend" — bodies are prose and are not matched against TERMS, and
//    "un mix parallèle" is not what a French engineer writes (N2 correction 17).
//    knob-OUTPUT_GAIN's body follows the new caption: "Comme le Mix".
//
//  * THREE tooltip TITLES are straight copies and stay UNFLAGGED: "Mix",
//    "Glitch", "Chaos" — all three are French as they stand. `sameAsEn: true`
//    is NOT added, because on an I18N entry check-i18n:568 reads it as "title
//    AND body are both straight copies" and would disarm assertion 4 for those
//    entries; all three have French bodies. The lint reports them as INFO only.
//
//  * (Superseded by v1.6.2, item 39 above.) The preset dropdown's theme
//    headings were HARD-CODED ENGLISH in app.js (PRESET_THEMES) and not
//    localized, so preset-name's French body named them in English — "Tape
//    Stops, Scratch, Wobble & Warp, et Glitch & Chaos" — rather than send a
//    French user looking for headings that were not on the page. v1.6.2 keys
//    the headings, and the body names the French ones.
//
//  * Register: this page is INFINITIVE. Five sites already were (the footer,
//    both envelope hints, settings, preset-name); the two imperative bodies
//    (engage-btn, envCanvas) were converted, and envCanvas now opens on the
//    same "Glisser les points" as label.envHint1 does.
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
// ALL FRENCH WAS MACHINE-DRAFTED AND IS STILL FLAGGED `reviewed: false`.
// v1.6.1 read every entry against its English and against the suite glossary;
// that is a second machine reading, not a native speaker, so the flag stands.
// `node scripts/check-i18n.js` prints the worklist.
// ============================================================================

export const LANGUAGES = ['en', 'fr'];

export const I18N = Object.freeze({

    // ── The settings popover (v1.5.0) ───────────────────────────────────────
    // The gear is new. The hover-help entry below is the v1.4.0 "?" toggle's copy,
    // MOVED here unchanged along with the control itself — not duplicated.
    'settings': {
        en: { t: 'Settings',
              b: 'Choose the language of this plugin, and turn the hover help on or off. Both choices are remembered with the session.' },
        fr: { t: 'Réglages',
              b: 'Choisir la langue de ce plugin et activer ou désactiver l’aide au survol. Les deux choix sont conservés avec la session.',
              reviewed: true },
    },

    // v1.6.0: this entry told the user, in both languages, that the labels on
    // the page do not change. That is now false — they do. Rewritten to say
    // what is true, INCLUDING the half that stayed true: readouts are English
    // in both languages (D-03), so `250 ms` reads the same either way.
    'lang-select': {
        en: { t: 'Language',
              b: 'The language of this hover help and of the labels on the page. English and French are available; value readouts stay in English.' },
        fr: { t: 'Langue',
              b: 'La langue de cette aide au survol et des libellés de la page. L’anglais et le français sont disponibles ; les valeurs affichées restent en anglais.',
              reviewed: true },
    },

    'help-toggle': {
        en: { t: 'Hover Help',
              b: 'Turns the hover descriptions on and off for every control on this page. The setting is saved with the session.' },
        fr: { t: 'Aide au survol',
              b: 'Active ou désactive les descriptions au survol pour toutes les commandes de cette page. Le réglage est conservé avec la session.',
              reviewed: true },
    },

    'preset-prev': {
        en: { t: 'Previous',
              b: 'Steps back one entry through the preset list, factory and user alike.' },
        fr: { t: 'Précédent',
              b: 'Recule d’une entrée dans la liste des préréglages, d’usine comme utilisateur.',
              reviewed: true },
    },

    'preset-next': {
        en: { t: 'Next',
              b: 'Steps forward one entry through the preset list, factory and user alike.' },
        fr: { t: 'Suivant',
              b: 'Avance d’une entrée dans la liste des préréglages, d’usine comme utilisateur.',
              reviewed: true },
    },

    'preset-name': {
        en: { t: 'Preset',
              b: 'The loaded preset. Click to open the list, grouped into Tape Stops, Scratch, Wobble & Warp, and Glitch & Chaos.' },
        // v1.6.2: the headings are localized now (label.theme*), so the body
        // names them as the French page shows them. v1.6.1 named the English
        // ones because that is what the page rendered.
        fr: { t: 'Préréglage',
              b: 'Le préréglage chargé. Cliquer pour ouvrir la liste, groupée en Arrêts de bande, Scratch, Pleurage & Déformation, et Glitch & Chaos.',
              reviewed: false },
    },

    'preset-save': {
        en: { t: 'Save',
              b: 'Writes every control, including the drawn scratch envelope, to a user preset.' },
        fr: { t: 'Enregistrer',
              b: 'Écrit toutes les commandes, y compris l’enveloppe de scratch dessinée, dans un préréglage utilisateur.',
              reviewed: true },
    },

    'preset-load': {
        en: { t: 'Load',
              b: 'Opens a preset file from disk rather than from the list above.' },
        fr: { t: 'Charger',
              b: 'Ouvre un fichier de préréglage sur le disque plutôt que depuis la liste ci-dessus.',
              reviewed: true },
    },

    'preset-delete': {
        en: { t: 'Delete',
              b: 'Removes the loaded user preset. Click once to arm, again to confirm. Factory presets are protected.' },
        fr: { t: 'Supprimer',
              b: 'Supprime le préréglage utilisateur chargé. Un premier clic arme, un second confirme. Les préréglages d’usine sont protégés.',
              reviewed: true },
    },

    'engage-btn': {
        en: { t: 'Engage',
              b: 'The performance control. It latches: pressing it starts the gesture the current Mode describes, releasing it hands the transport back. Automate or MIDI-map this and leave the rest of the page alone.' },
        fr: { t: 'Enclencher',
              b: 'La commande de jeu. Elle se maintient : l’enfoncer démarre le geste que décrit le Mode courant, la relâcher rend la main au transport. L’automatiser ou l’assigner en MIDI, et laisser le reste de la page tranquille.',
              reviewed: true },
    },

    'seg-mode-stop': {
        en: { t: 'Stop Mode',
              b: 'The reel losing power. Engage spins the tape down to a halt; releasing spins it back up to speed. Spin Down and Spin Up shape each half separately.' },
        fr: { t: 'Mode arrêt',
              b: 'La bobine qui perd son alimentation. Enclencher fait ralentir la bande jusqu’à l’arrêt ; relâcher la fait remonter à sa vitesse. Ralentissement et Redémarrage façonnent chaque moitié séparément.',
              reviewed: true },
    },

    'seg-mode-scratch': {
        en: { t: 'Scratch Mode',
              b: 'A hand on the platter. Engage plays one pass of the drawn speed curve, reverse included, then returns to normal speed.' },
        fr: { t: 'Mode scratch',
              b: 'Une main sur le plateau. Enclencher joue un passage de la courbe de vitesse dessinée, marche arrière comprise, puis revient à la vitesse normale.',
              reviewed: true },
    },

    'seg-mode-cont': {
        en: { t: 'Motion Mode',
              b: 'Worn tape that never settles. Engage holds the speed in continuous modulation for as long as it stays lit — wobble, lurch or stutter, depending on Character.' },
        fr: { t: 'Mode mouvement',
              b: 'Une bande usée qui ne se stabilise jamais. Enclencher maintient la vitesse en modulation continue tant que le bouton reste allumé — pleurage, embardée ou bégaiement, selon le Caractère.',
              reviewed: true },
    },

    'seg-sync-sync': {
        en: { t: 'Sync',
              b: 'Every duration on this page locks to the host\'s tempo grid and is chosen as a note division. Switches all three panels at once.' },
        fr: { t: 'Synchro',
              b: 'Toutes les durées de cette page s’asservissent à la grille de tempo de l’hôte et se choisissent en divisions de note. Bascule les trois panneaux à la fois.',
              reviewed: true },
    },

    'seg-sync-free': {
        en: { t: 'Free',
              b: 'Every duration is set in milliseconds (or hertz, for Motion) and ignores the host tempo. Switches all three panels at once.' },
        fr: { t: 'Libre',
              b: 'Toutes les durées se règlent en millisecondes (ou en hertz, pour le Mouvement) et ignorent le tempo de l’hôte. Bascule les trois panneaux à la fois.',
              reviewed: true },
    },

    'ratio-fill': {
        en: { t: 'Playback Rate',
              b: 'Live readout of the transport\'s speed. 1× is normal, 0 is a dead stop, and anything left of zero is running backwards.' },
        fr: { t: 'Vitesse de lecture',
              b: 'Affichage en direct de la vitesse du transport. 1× est la vitesse normale, 0 l’arrêt complet, et tout ce qui est à gauche de zéro défile à l’envers.',
              reviewed: true },
    },

    'combo-STOP_SYNC_DIV': {
        en: { t: 'Spin-Down Time',
              b: 'How long the reel takes to reach a standstill, as a note division of the host tempo. Latched the moment Engage is pressed — moving it mid-gesture is inert.' },
        fr: { t: 'Durée de ralentissement',
              b: 'Le temps que met la bobine à atteindre l’arrêt complet, en division de note du tempo de l’hôte. Verrouillé à l’instant où Enclencher est pressé — le modifier en cours de geste est sans effet.',
              reviewed: true },
    },

    'knob-STOP_FREE_MS': {
        en: { t: 'Spin-Down Time',
              b: 'How long the reel takes to reach a standstill, in milliseconds. Latched the moment Engage is pressed.' },
        fr: { t: 'Durée de ralentissement',
              b: 'Le temps que met la bobine à atteindre l’arrêt complet, en millisecondes. Verrouillé à l’instant où Enclencher est pressé.',
              reviewed: true },
    },

    'knob-STOP_CURVE': {
        en: { t: 'Spin-Down Curve',
              b: 'Shapes the fall. At 0 the speed drops in a straight line; at 100 it plunges away at once and then crawls the last stretch. 50 is turntable physics.' },
        fr: { t: 'Courbe de ralentissement',
              b: 'Façonne la chute. À 0 la vitesse descend en ligne droite ; à 100 elle s’effondre d’un coup puis rampe sur la dernière portion. 50 correspond à la physique d’une platine.',
              reviewed: true },
    },

    'combo-START_SYNC_DIV': {
        en: { t: 'Spin-Up Time',
              b: 'How long the reel takes to regain full speed once Engage is released, as a note division of the host tempo.' },
        fr: { t: 'Durée de redémarrage',
              b: 'Le temps que met la bobine à retrouver sa pleine vitesse une fois Enclencher relâché, en division de note du tempo de l’hôte.',
              reviewed: true },
    },

    'knob-START_FREE_MS': {
        en: { t: 'Spin-Up Time',
              b: 'How long the reel takes to regain full speed once Engage is released, in milliseconds.' },
        fr: { t: 'Durée de redémarrage',
              b: 'Le temps que met la bobine à retrouver sa pleine vitesse une fois Enclencher relâché, en millisecondes.',
              reviewed: true },
    },

    'knob-START_CURVE': {
        en: { t: 'Spin-Up Curve',
              b: 'Shapes the return. At 0 the speed rises in a straight line; at 100 it hangs near a standstill and then rushes back at the end. 50 is turntable physics.' },
        fr: { t: 'Courbe de redémarrage',
              b: 'Façonne le retour. À 0 la vitesse monte en ligne droite ; à 100 elle traîne près de l’arrêt puis se précipite à la fin. 50 correspond à la physique d’une platine.',
              reviewed: true },
    },

    'envCanvas': {
        en: { t: 'Scratch Envelope',
              b: 'Speed across one pass. The 1× line is normal speed, 0 is a standstill, and everything below the 0 line plays backwards — down to −2×. Drag the points to reshape it.' },
        fr: { t: 'Enveloppe de scratch',
              b: 'La vitesse tout au long d’un passage. La ligne 1× est la vitesse normale, 0 l’immobilité, et tout ce qui passe sous la ligne 0 se lit à l’envers — jusqu’à −2×. Glisser les points pour la remodeler.',
              reviewed: true },
    },

    'combo-ENV_SYNC_DIV': {
        en: { t: 'Pass Length',
              b: 'How long one trip through the drawn envelope takes, as a note division of the host tempo.' },
        fr: { t: 'Durée du passage',
              b: 'Le temps que dure un parcours complet de l’enveloppe dessinée, en division de note du tempo de l’hôte.',
              reviewed: true },
    },

    'knob-ENV_FREE_MS': {
        en: { t: 'Pass Length',
              b: 'How long one trip through the drawn envelope takes, in milliseconds.' },
        fr: { t: 'Durée du passage',
              b: 'Le temps que dure un parcours complet de l’enveloppe dessinée, en millisecondes.',
              reviewed: true },
    },

    'seg-char-wobble': {
        en: { t: 'Wobble',
              b: 'Deterministic wow and flutter — a steady sine with a three-harmonic flutter band above it. The seasick end of worn tape.' },
        fr: { t: 'Pleurage',
              b: 'Pleurage et scintillement déterministes — une sinusoïde régulière surmontée d’une bande de scintillement à trois harmoniques. Le versant nauséeux de la bande usée.',
              reviewed: true },
    },

    'seg-char-random': {
        en: { t: 'Random',
              b: 'A drifting random walk across three time-scales at once. It never repeats and never quite settles.' },
        fr: { t: 'Aléatoire',
              b: 'Une marche aléatoire qui dérive sur trois échelles de temps à la fois. Elle ne se répète jamais et ne se pose jamais tout à fait.',
              reviewed: true },
    },

    'seg-char-glitch': {
        en: { t: 'Glitch',
              b: 'A grid scheduler firing discrete speed events — dips, half-speed stalls, reverse stabs. Chaos decides how many fire and how short they get.' },
        fr: { t: 'Glitch',
              b: 'Un séquenceur sur grille qui déclenche des événements de vitesse discrets — creux, blocages à mi-vitesse, coups en marche arrière. Le Chaos décide combien se déclenchent et à quel point ils raccourcissent.',
              reviewed: true },
    },

    'combo-CONT_RATE_SYNC_DIV': {
        en: { t: 'Motion Rate',
              b: 'How fast the motion cycles, as a note division of the host tempo.' },
        fr: { t: 'Vitesse du mouvement',
              b: 'À quelle vitesse le mouvement se répète, en division de note du tempo de l’hôte.',
              reviewed: true },
    },

    'knob-CONT_RATE_HZ': {
        en: { t: 'Motion Rate',
              b: 'How fast the motion cycles, in hertz. 0.05 Hz is one slow sweep every twenty seconds; 20 Hz is a buzz.' },
        fr: { t: 'Vitesse du mouvement',
              b: 'À quelle vitesse le mouvement se répète, en hertz. 0,05 Hz correspond à un balayage lent toutes les vingt secondes ; 20 Hz à un bourdonnement.',
              reviewed: true },
    },

    'knob-CONT_DEPTH': {
        en: { t: 'Motion Depth',
              b: 'Peak speed deviation, scaled by ear rather than linearly: 0 is barely two cents of drift, 50 is about one percent, 100 reaches roughly two semitones.' },
        fr: { t: 'Profondeur du mouvement',
              b: 'Écart de vitesse maximal, dosé à l’oreille plutôt que linéairement : 0 représente à peine deux centièmes de demi-ton de dérive, 50 environ un pour cent, et 100 atteint près de deux demi-tons.',
              reviewed: true },
    },

    'knob-CONT_CHAOS': {
        en: { t: 'Chaos',
              b: 'How irregular the motion is. At 0 each Character runs its tamest, most repeatable shape; raising it jitters Wobble, widens Random\'s drift, and unlocks faster, shorter Glitch events.' },
        fr: { t: 'Chaos',
              b: 'À quel point le mouvement est irrégulier. À 0, chaque Caractère joue sa forme la plus sage et la plus reproductible ; en montant, le Pleurage se met à trembler, la dérive de l’Aléatoire s’élargit, et les événements de Glitch deviennent plus rapides et plus courts.',
              reviewed: true },
    },

    'knob-TONE_TRACK': {
        en: { t: 'Tone Track',
              b: 'Darkens the varispeed path as the tape slows, the way tape and vinyl lose their highs off-speed. At 0 nothing is filtered; at 100 a full standstill reaches down to 150 Hz. Full speed is always open.' },
        fr: { t: 'Suivi de timbre',
              b: 'Assombrit le trajet en vitesse variable à mesure que la bande ralentit, comme la bande et le vinyle perdent leurs aigus hors vitesse. À 0 rien n’est filtré ; à 100 l’arrêt complet descend jusqu’à 150 Hz. À pleine vitesse le trajet reste toujours ouvert.',
              reviewed: true },
    },

    'knob-MIX': {
        en: { t: 'Mix',
              b: 'Balance of the dry input against the varispeed path — a straight crossfade, so it doubles as a parallel blend. Applies to the engaged chain only; disengaged output is bit-for-bit dry.' },
        fr: { t: 'Mix',
              b: 'Équilibre entre le signal direct et le trajet en vitesse variable — un simple fondu croisé, qui sert donc aussi de mélange parallèle. Ne s’applique qu’à la chaîne enclenchée ; hors enclenchement la sortie est le signal direct au bit près.',
              reviewed: true },
    },

    'knob-OUTPUT_GAIN': {
        en: { t: 'Output Gain',
              b: 'Final trim, −24 to +12 dB. Like Mix, it rides the engaged chain only and glides back to unity across the resync fade.' },
        fr: { t: 'Gain de sortie',
              b: 'Ajustement final, de −24 à +12 dB. Comme le Mix, il ne porte que sur la chaîne enclenchée et revient progressivement à l’unité pendant le fondu de resynchronisation.',
              reviewed: true },
    },
});

// ============================================================================
// LABELS — the on-page text (v1.6.0, canon v2)
// ============================================================================
//
// I18N above is HOVER-HELP copy: a title and a body, rendered into a wrapping
// 230 px tooltip. LABELS is ON-PAGE copy: one string, rendered into a fixed
// cell that does not wrap. The two are different problems and this table keeps
// them apart on purpose.
//
// ── THE REUSE RULE ─────────────────────────────────────────────────────────
// trLabel() falls back to I18N when a key is absent here, so a control whose
// tooltip TITLE already IS its label can carry ONE key. That fallback is used
// ONLY where the tooltip title is the identical string in BOTH languages —
// #preset-save, #engage-btn, the three CHARACTER segments, Mix, Chaos and the
// gear's aria-label all reuse their I18N key and appear nowhere below.
//
// It is deliberately NOT used where only the English matches. Reusing a key
// there would make every future edit to a tooltip a silent geometry change to
// a control. A label and a tip are the same string only when they are the same
// string in both languages.
//
// v1.6.1 note: #seg-sync-sync used to be this rule's worked example — its tip
// title was "Synchronisé" while its label was "Synchro", and "SYNCHRONISÉ"
// measures 81.02 px against the segment's 62 px content box. Stage N settled
// the title on the glossary's "Synchro", so the two strings now DO match in
// both languages. The keys still stay separate: collapsing them would re-couple
// a 62 px control to a 230 px tooltip, which is the coupling this rule exists
// to prevent, and the reuse rule is an allowance, never an obligation.
//
// ── ENGLISH WAS MOVED, NOT RE-TYPED ────────────────────────────────────────
// Every en below is byte-for-byte what index.html carried through v1.5.0,
// taken from scripts/i18n-extract.js's inventory rather than transcribed, with
// HTML entities decoded to the characters they named (&#183; -> ·) because
// textContent does not decode.
//
// ── FRENCH IS SIZED, NOT SHRUNK ────────────────────────────────────────────
// D-04 forbids an auto-shrink font and a short-variant fallback: there is
// exactly ONE French string per key here and nothing chooses between variants
// at runtime. Where French did not fit, the fix was the plugin's own CSS
// (see CHANGELOG v1.6.0). v1.6.1 retired the one exception this paragraph used
// to name: TONE TRACK's "Suivi tonal" was chosen over "Suivi de timbre" on a
// width claim that did not survive measurement — see the key's own comment.
//
// ALL FRENCH WAS MACHINE-DRAFTED, REVISED IN STAGE N (v1.6.1) AND IS STILL
// `reviewed: false` — a glossary-and-lint pass is not a native speaker.
// `node scripts/check-i18n.js` prints the worklist, LABELS included.
// ============================================================================

export const LABELS = Object.freeze({

    // ── Settings popover ────────────────────────────────────────────────────
    // Not 'help-toggle': that tip's title is "Hover Help", this caption is
    // "Hover help". The reuse rule wants both languages identical, and these
    // differ in English before French is even considered.
    'label.hoverHelp':   { en: { t: 'Hover help' },  fr: { t: 'Aide au survol', reviewed: true } },

    // The hover-help toggle's two faces, and the delete button's armed face.
    // These are the only three strings on this page written from script. They
    // go through setLabel(), so the element becomes a [data-i18n] element and
    // the language sweep owns it from that moment — a state string written as
    // a raw literal is stranded in the previous language the instant the
    // selector fires.
    //
    // "Marche" / "Arrêt" rather than "Activé" / "Désactivé": the toggle face is
    // 44 px, and this is the vocabulary a piece of hardware uses, which is the
    // register the whole panel is written in.
    'ui.on':             { en: { t: 'On' },          fr: { t: 'Marche',        reviewed: true } },
    'ui.off':            { en: { t: 'Off' },         fr: { t: 'Arrêt',         reviewed: true } },
    'ui.confirm':        { en: { t: 'Confirm?' },    fr: { t: 'Confirmer ?',   reviewed: true } },

    // ── Header ──────────────────────────────────────────────────────────────
    'label.subtitle':    { en: { t: 'Varispeed Transport · A Field Guide' },
                           fr: { t: 'Transport à vitesse variable · Guide de terrain', reviewed: true } },

    // ── TRIGGER panel ───────────────────────────────────────────────────────
    'label.trigger':     { en: { t: 'Trigger' },     fr: { t: 'Déclenchement', reviewed: true } },
    'label.mode':        { en: { t: 'Mode' },        fr: { t: 'Mode',          reviewed: true, sameAsEn: true } },
    'label.modeStop':    { en: { t: 'Stop' },        fr: { t: 'Arrêt',         reviewed: true } },
    'label.modeScratch': { en: { t: 'Scratch' },     fr: { t: 'Scratch',       reviewed: true, sameAsEn: true } },
    // Shared by the MODE segment, the CHARACTER pane's Motion caption and
    // #modeSegments' aria-label: one concept, one string, one key.
    'label.motion':      { en: { t: 'Motion' },      fr: { t: 'Mouvement',     reviewed: true } },
    'label.timing':      { en: { t: 'Timing' },
                           fr: { t: 'Cadence', reviewed: true,
                                 termNote: 'the caption heads the SYNC/FREE pair — it names the time BASE, '
                                         + 'not the rhythmic offset the glossary term "décalage" means; '
                                         + 'this page carries no nudge control' } },
    'label.sync':        { en: { t: 'Sync' },        fr: { t: 'Synchro',       reviewed: true } },
    'label.playback':    { en: { t: 'Playback' },    fr: { t: 'Lecture',       reviewed: true } },

    // ── CENTER panel ────────────────────────────────────────────────────────
    'label.transport':   { en: { t: 'Transport' },   fr: { t: 'Transport',     reviewed: true, sameAsEn: true } },
    'label.spinDown':    { en: { t: 'Spin Down' },   fr: { t: 'Ralentissement', reviewed: true } },
    'label.spinUp':      { en: { t: 'Spin Up' },     fr: { t: 'Redémarrage',   reviewed: true } },
    'label.division':    { en: { t: 'Division' },    fr: { t: 'Division',      reviewed: true, sameAsEn: true } },
    'label.time':        { en: { t: 'Time' },        fr: { t: 'Durée',         reviewed: true } },
    'label.curve':       { en: { t: 'Curve' },       fr: { t: 'Courbe',        reviewed: true } },
    'label.passLength':  { en: { t: 'Pass Length' }, fr: { t: 'Passage',       reviewed: true } },

    // The envelope hint is TWO text nodes around a <br> in one .env-hint div.
    // Keyed as two spans rather than one key with a \n: applyLabel writes
    // textContent, which would delete the <br> and collapse the two lines into
    // one, and the second line is what tells the user how to REMOVE a point.
    'label.envHint1':    { en: { t: 'Drag points · double-click to add' },
                           fr: { t: 'Glisser les points · double-clic pour ajouter', reviewed: true } },
    'label.envHint2':    { en: { t: 'alt-click removes · drag a diamond to bend' },
                           fr: { t: 'alt-clic pour retirer · glisser un losange pour infléchir', reviewed: true } },

    'label.character':   { en: { t: 'Character' },   fr: { t: 'Caractère',     reviewed: true } },
    'label.rate':        { en: { t: 'Rate' },        fr: { t: 'Vitesse',       reviewed: true } },
    'label.depth':       { en: { t: 'Depth' },       fr: { t: 'Profondeur',    reviewed: true } },

    // ── OUTPUT panel ────────────────────────────────────────────────────────
    'label.output':      { en: { t: 'Output' },      fr: { t: 'Sortie',        reviewed: true } },
    // v1.6.1: this key and the knob-TONE_TRACK tip title now carry the SAME
    // French, the glossary root "Suivi de timbre". v1.6.0 shipped the shorter
    // "Suivi tonal" here and defended it at "97 px in an 88 px knob cell";
    // measured at the shipping frame it is 91.97 px, and .knob-label is
    // shrink-to-fit with overflow:visible, so the cell is not the constraint —
    // the caption centres at x=[699.02, 790.98] inside .group-output's padding
    // box [671, 819], 28.02 px clear on each side. The keys still stay separate
    // (the reuse rule is an allowance, not an obligation): one is a 9.5 px
    // caption in a fixed column, the other is prose in a 230 px tip.
    'label.toneTrack':   { en: { t: 'Tone Track' },  fr: { t: 'Suivi de timbre', reviewed: true } },
    // NOT knob-OUTPUT_GAIN, whose title is "Output Gain": this caption is the
    // bare word, under an OUTPUT group heading that already says the rest.
    'label.gain':        { en: { t: 'Gain' },        fr: { t: 'Gain',          reviewed: true, sameAsEn: true } },

    'label.footer':      { en: { t: 'Drag vertically · wheel or arrows to trim · double-click to reset' },
                           fr: { t: 'Glisser verticalement · molette ou flèches pour ajuster · double-clic pour réinitialiser', reviewed: true } },

    // ── Accessible names ────────────────────────────────────────────────────
    // An aria-label is user-visible text by any definition that matters — it is
    // the accessible NAME, and a screen reader in French reading an English
    // name is the same failure as a French page with an English caption. These
    // have no rendered box, so none of them is a geometry risk.
    'aria.langSelect':   { en: { t: 'Interface language' },
                           fr: { t: 'Langue de l’interface', reviewed: true } },
    'aria.helpToggle':   { en: { t: 'Toggle hover help' },
                           fr: { t: 'Activer ou désactiver l’aide au survol', reviewed: true } },
    'aria.presetPrev':   { en: { t: 'Previous preset' },  fr: { t: 'Préréglage précédent', reviewed: true } },
    'aria.presetNext':   { en: { t: 'Next preset' },      fr: { t: 'Préréglage suivant',   reviewed: true } },
    'aria.modeCont':     { en: { t: 'Continuous motion' },fr: { t: 'Mouvement continu',    reviewed: true } },
    'aria.syncSegments': { en: { t: 'Sync Mode' },        fr: { t: 'Mode de synchro', reviewed: true } },
    // v1.6.2 (item 38): the three division selects are named after their tip
    // title plus what the control is, so the visible caption "Division" and
    // the title are both substrings of the accessible name (WCAG 2.5.3) —
    // through v1.6.1 these read "Stop Time" / "Start Time" / "Env Length",
    // a third name that matched neither. The French keeps the tip titles'
    // settled forms; no new rendering of spin-down / spin-up / pass.
    'aria.stopTime':     { en: { t: 'Spin-Down Time division' },
                           fr: { t: 'Division de la durée de ralentissement', reviewed: false } },
    'aria.startTime':    { en: { t: 'Spin-Up Time division' },
                           fr: { t: 'Division de la durée de redémarrage',    reviewed: false } },
    'aria.envCanvas':    { en: { t: 'Scratch speed envelope' },
                           fr: { t: 'Enveloppe de vitesse du scratch', reviewed: true } },
    'aria.envLength':    { en: { t: 'Pass Length division' },
                           fr: { t: 'Division de la durée du passage',        reviewed: false } },

    // ── Preset dropdown theme headings (v1.6.2, item 39) ────────────────────
    // Written by app.js's buildPresetDropdown() through setLabel(), one
    // literal key per PRESET_THEMES row plus the trailing user group. The
    // preset NAMES under them stay raw (a name IS the JSON filename, D-02).
    // "Arrêts de bande": the page's French for tape is "bande" (engage-btn,
    // seg-mode-cont, knob-TONE_TRACK). "Pleurage": the Wobble tip title's
    // word (glossary root for wow); "Déformation": the glossary root for warp.
    'label.themeTapeStops':   { en: { t: 'Tape Stops' },     fr: { t: 'Arrêts de bande',        reviewed: false } },
    'label.themeScratch':     { en: { t: 'Scratch' },        fr: { t: 'Scratch',                reviewed: false, sameAsEn: true } },
    'label.themeWobbleWarp':  { en: { t: 'Wobble & Warp' },  fr: { t: 'Pleurage & Déformation', reviewed: false } },
    'label.themeGlitchChaos': { en: { t: 'Glitch & Chaos' }, fr: { t: 'Glitch & Chaos',         reviewed: false, sameAsEn: true } },
    'label.themeUser':        { en: { t: 'User' },           fr: { t: 'Utilisateur',            reviewed: false } },
});

// ============================================================================
// I18N_EXEMPT — reasoned exclusions, never silence
// ============================================================================
//
// Every visible string the coverage scan finds must be a [data-i18n] element,
// a setLabel() call, or an entry HERE WITH A REASON. A bare skip list would let
// a missed label hide as a deliberate one.
// ============================================================================

export const I18N_EXEMPT = [
    // The product name. Split across the <h1>'s own text node and the italic
    // .title-accent span, so both halves need an entry.
    //
    // The markup authors HAIR SPACES around the en dash (&#8202;), but the
    // scanner collapses every whitespace run to one U+0020 before it
    // classifies, so this entry carries ORDINARY spaces. Exempting the
    // as-authored form instead would silently fail to match and report the
    // product name as an unlocalized label.
    ['O – Tape', 'half of the product name O–Tapestop — a product name is never translated'],
    ['stop',                    'the italic half of the product name O–Tapestop, in .title-accent'],

    // #preset-name displays the loaded preset. The name IS the JSON filename
    // (OuariconPresetManager.h:283-285), so translating it breaks recall:
    // a session saved against "Cathedral" would not resolve "Cathédrale".
    // "Default" is the placeholder the manager overwrites on its first pass.
    ['Default',                 'a factory preset name — exempt under D-02, because the name IS the JSON filename'],
];

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
