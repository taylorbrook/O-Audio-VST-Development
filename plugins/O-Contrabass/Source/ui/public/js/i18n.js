/*
   This file is part of O-Contrabass, an Ouaricon Audio plugin.
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
// i18n.js — O-Contrabass interface copy, English + French (v1.8.2)
//
// ── v1.8.2: ENGLISH DEFECT FROM THE FRENCH READING (Stage O, 2026-08-31) ────
//
// Item 61: the note-expression-toggle tip TITLE read "Note expression" while
// the caption (label.noteExpression), the VST3 parameter name
// (PluginProcessor.cpp "Note Expression") and NOTES.md all say "Note
// Expression" — the feature is a proper noun in the VST3 SDK. Title
// capitalised. The French title and caption already agreed (both "Note
// Expression"), so the French is byte-untouched and reviewed: true stands.
// The en title now equals the fr title over a translated body: NO sameAsEn
// flag (Stage N correction 26 — the lint counts it as covered). No aria.*
// entry names this control (the toggle is a div with no aria-label; reported,
// not in scope). English changed: 1 entry. French changed: 0.
//
// ── v1.8.1: FRENCH QA PASS (Stage N, 2026-08-31) ────────────────────────────
//
// Every fr entry read against its en and against scripts/i18n-fr-glossary.js.
// Changed: 47 entries of 103 (17 terminology, 22 typography, 4 grammar/
// agreement, 3 meaning, 1 flag). sameAsEn: kept 9, translated 0, added 1
// (label.mix, now the glossary root "Mix" and therefore a straight copy),
// removed 1 (note-expression-toggle — its BODY is translated, so the entry is
// not a straight copy and the flag was disarming check-i18n assertion 4 for
// it). termNote exemptions: 2 (both listed by
// `node scripts/i18n-fr-lint.js --plugin O-Contrabass --strict`, which exits 0
// against a baseline of 70 findings: 33 T1, 8 T5, 8 T7, 11 G1, 10 F1).
// Left as drafted: the other 56. reviewed: false throughout — the flag means a
// NATIVE SPEAKER read it, and none has; this was a second machine reading.
//
// Decisions the next reader needs:
//
//   * THE KNOB CELL IS 62 px, not the 55 px the LABELS block claimed through
//     v1.8.0, and .knob-label truncates with an ellipsis rather than pushing a
//     neighbour. Three of this file's width claims were wrong when measured
//     (Saturation 55.00 not 66, Amortissement 74.27 not 84, the cell itself);
//     each is corrected at its entry. "Saturation" now fits and ships.
//   * Release: "Extinction" / "Chute" were two French names for one control,
//     and the glossary forbids both. The tip carries the root "Relâchement",
//     the caption the listed abbreviation "Relâch." (the root wraps at 64.95).
//   * Rate/Depth are Vitesse/Profondeur here, NOT Fréquence/Ampleur — even
//     though both LFOs are specified in Hz. O-Bowed's Fréq. exemption was for a
//     knob sitting directly under a column already captioned Vitesse; nothing
//     on this page collides that way, and the body of VIBRATO_RATE already
//     said "Vitesse" while its title said "Fréquence".
//   * "Tenue infinie" / "Tenue inf." KEPT against the glossary's Maintien, with
//     a termNote on each half. This page carries no envelope, only a Release,
//     so there is no ADSR sustain segment for Maintien to name. MEANING, not
//     width: Maint. inf. measures 50.58 px and would have fit. Converges with
//     O-Bowed v1.6.1, which reasoned the same exemption on the same control.
//   * "Corps" is KEPT for Body. O-Bowed says "Caisse" and argues it is the
//     organology term; "body" is not a glossary row, so neither is settled.
//     Reported for the glossary rather than changed on one page — two plugins
//     disagreeing is exactly what the list exists to decide.
//   * "Dosage" stays in two BODIES (INFINITE_SUSTAIN, MASTER_SAT_AMOUNT) where
//     it means an amount, and is gone from both places it was a NAME
//     (label.mix, BODY_MIX). Bodies are not matched against TERMS.
//   * Register: bodies are INFINITIVE throughout (Choisir, Reculer, Cliquer,
//     Double-cliquer, Afficher) and address the user as vous. One register per
//     plugin — keep it.
//   * U+00A0 landed 21 times: before every ';' and between every number and its
//     unit, including m/s, N, dBFS and VU, which the lint's UNITS list does not
//     all carry. The cents sign stays glued (±1200¢) because the English glues
//     it and no gate asks otherwise. Nothing outside a French string VALUE was
//     touched: both revisions were imported and compared field by field — en
//     values changed 0, key sets changed 0, TIP_BINDINGS / I18N_EXEMPT /
//     LANGUAGES byte-identical, and no U+00A0 sits outside a t:/b: value.
//
// An ES module that EXPORTS ONLY. It must never self-execute: a bare top-level
// statement here throws out of module evaluation and takes every later
// initializer on the page with it (pattern_module_toplevel_init_tdz), which on
// this plugin means the ENTIRE UI — O-Contrabass's controller is one inline
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
// innerHTML reference here and any string literal containing `<`.
//
// THE ENGLISH WAS MOVED, NOT REWRITTEN. Every `en` entry below was extracted
// mechanically from index.html at v1.7.2 rather than re-typed, with its HTML
// entities decoded to the characters they named (&beta; -> β, &mdash; -> —)
// because setAttribute + textContent do not decode entities. Two deliberate
// normalisations are recorded at their entries.
//
// KEYS ARE THE PARAMETER ID where the anchor has one, and the element id
// otherwise. Five anchors have neither — the two tab buttons, the Active
// Strings block, the Fine Tuners title and the Tuning System field — and are
// addressed by their authored selector in TIP_BINDINGS with a stable key.
//
// ALL FRENCH IS MACHINE-DRAFTED AND FLAGGED `reviewed: false`. No native
// speaker has read it. `node scripts/check-i18n.js` prints the worklist.
// ============================================================================

export const LANGUAGES = ['en', 'fr'];

export const I18N = Object.freeze({

    // ── The settings popover (v1.8.0) ───────────────────────────────────────
    // The gear is new. The `help-toggle` entry below is the v1.7.0 "?" toggle's
    // copy, MOVED here unchanged along with the control itself — not
    // duplicated. One place for the two things that decide what the hover help
    // says and whether it says it.
    'gear-btn': {
        en: { t: 'Settings',
              b: 'Choose the language of the interface, and turn the hover help on or off. Both choices are remembered with the session.' },
        fr: { t: 'Réglages',
              b: "Choisir la langue de l’interface et activer ou désactiver l’aide au survol. Les deux choix sont conservés avec la session.",
              reviewed: true },
    },

    // Written to say what is TRUE of canon v2, in both languages: the labels DO
    // change, and the halves that stay English are named rather than left to be
    // discovered — value readouts (D-03), note names and preset names (D-02,
    // the name IS the JSON filename).
    'lang-select': {
        en: { t: 'Language',
              b: 'The language of the labels on this page and of this hover help. English and French are available; value readouts, note names and preset names stay in English.' },
        fr: { t: 'Langue',
              b: "La langue des libellés de cette page et de cette aide au survol. L’anglais et le français sont disponibles ; les valeurs affichées, les noms de notes et les noms de préréglages restent en anglais.",
              reviewed: true },
    },

    // ── Header / preset bar ─────────────────────────────────────────────────
    'preset-prev': {
        en: { t: 'Previous preset',
              b: 'Step back through the preset list.' },
        fr: { t: 'Préréglage précédent',
              b: 'Reculer dans la liste des préréglages.',
              reviewed: true },
    },
    'preset-name': {
        en: { t: 'Preset',
              b: 'The preset currently loaded — click to browse the full list. Factory presets are read-only; saving under the same name writes a user copy.' },
        fr: { t: 'Préréglage',
              b: "Le préréglage actuellement chargé — cliquer pour parcourir la liste complète. Les préréglages d’usine sont en lecture seule ; enregistrer sous le même nom écrit une copie utilisateur.",
              reviewed: true },
    },
    'preset-next': {
        en: { t: 'Next preset',
              b: 'Step forward through the preset list.' },
        fr: { t: 'Préréglage suivant',
              b: 'Avancer dans la liste des préréglages.',
              reviewed: true },
    },
    'preset-save': {
        en: { t: 'Save',
              b: 'Save the current settings as a user preset.' },
        fr: { t: 'Enregistrer',
              b: 'Enregistrer les réglages actuels comme préréglage utilisateur.',
              reviewed: true },
    },
    'tab-main': {
        en: { t: 'Main',
              b: 'The playing surface — bow, body, strings, expression, drone and output.' },
        fr: { t: 'Principal',
              b: "La surface de jeu — archet, corps, cordes, expression, bourdon et sortie.",
              reviewed: true },
    },
    'tab-tuning': {
        en: { t: 'Tuning',
              b: 'The full tuning panel — intervals table, scale library, generators, .scl/.kbm files.' },
        fr: { t: 'Accord',
              b: "Le panneau d’accord complet — table des intervalles, bibliothèque de gammes, générateurs, fichiers .scl/.kbm.",
              reviewed: true },
    },
    'help-toggle': {
        en: { t: 'Hover help',
              b: 'Show a short description when the pointer rests on a control. The setting is remembered with the session.' },
        fr: { t: 'Aide au survol',
              b: "Afficher une brève description lorsque le pointeur s’arrête sur une commande. Le réglage est conservé avec la session.",
              reviewed: true },
    },

    // ── I · Bow ─────────────────────────────────────────────────────────────
    'BOW_SPEED': {
        en: { t: 'Bow speed',
              b: 'How fast the bow travels, 0.02–1.5 m/s. Slow bows choke and darken; fast bows loosen and sing.' },
        fr: { t: "Vitesse d’archet",
              b: "Vitesse de déplacement de l’archet, 0,02–1,5 m/s. Un archet lent étouffe et assombrit ; un archet rapide libère et fait chanter.",
              reviewed: true },
    },
    'BOW_PRESSURE': {
        en: { t: 'Bow pressure',
              b: 'Bow force on the string, 0.05–8 N. Too light skates into surface sound, too heavy drives into raucous scratch — the Schelleng diagram shows where you are.' },
        fr: { t: "Pression d’archet",
              b: "Force de l’archet sur la corde, 0,05–8 N. Trop légère, elle fait glisser le son en surface ; trop lourde, elle verse dans le grattement rauque — le diagramme de Schelleng indique où vous êtes.",
              reviewed: true },
    },
    'BOW_POSITION': {
        en: { t: 'Bow position',
              b: 'Where the bow crosses the string, as fraction β of its length. Near the bridge (low β) is brighter but harder to speak; toward the fingerboard is warmer.' },
        fr: { t: "Position d’archet",
              b: "Point de contact de l’archet sur la corde, en fraction β de sa longueur. Près du chevalet (β faible), le son est plus brillant mais parle plus difficilement ; vers la touche, il est plus chaud.",
              reviewed: true },
    },
    'ROSIN': {
        en: { t: 'Rosin',
              b: 'Bow grip — the strength of the stick-slip friction. More rosin bites harder and speaks faster.' },
        fr: { t: 'Colophane',
              b: "Adhérence de l’archet — la force du frottement adhérence-glissement. Plus de colophane mord davantage et fait parler plus vite.",
              reviewed: true },
    },
    'BOW_NOISE': {
        en: { t: 'Bow noise',
              b: 'Level of the bow-hair noise bed — the breathy scratch riding under the tone.' },
        fr: { t: "Bruit d’archet",
              b: "Niveau du lit de bruit de crin — le grattement soufflé qui court sous le son.",
              reviewed: true },
    },
    'RELEASE': {
        en: { t: 'Release',
              b: 'How long the string keeps ringing once the bow lifts, 0.05–20 s.' },
        fr: { t: 'Relâchement',
              b: "Durée de résonance de la corde après le retrait de l’archet, 0,05–20 s.",
              reviewed: true },
    },
    'canvas-schelleng': {
        en: { t: 'Schelleng diagram',
              b: 'Where the current bow speed × pressure sits. The green band is the Helmholtz regime — the singing zone between surface sound and raucous; its wedge shifts with bow position β.' },
        fr: { t: 'Diagramme de Schelleng',
              b: "Où se situe le couple vitesse × pression actuel. La bande verte est le régime de Helmholtz — la zone chantante entre le son de surface et le rauque ; son coin se déplace avec la position d’archet β.",
              reviewed: true },
    },

    // ── II · Body ───────────────────────────────────────────────────────────
    'BODY_SIZE': {
        en: { t: 'Body size',
              b: 'Size of the modeled body resonator. Larger bodies shift the eight modes down — more chest, less bark.' },
        fr: { t: 'Taille du corps',
              b: 'Taille du résonateur de corps modélisé. Un corps plus grand abaisse les huit modes — plus de coffre, moins de mordant.',
              reviewed: true },
    },
    'BODY_DAMPING': {
        en: { t: 'Body damping',
              b: 'How quickly the body modes ring out. High damping dries and tightens the resonance.' },
        fr: { t: 'Amortissement du corps',
              b: "Vitesse d’extinction des modes du corps. Un amortissement élevé assèche et resserre la résonance.",
              reviewed: true },
    },
    'BODY_MIX': {
        en: { t: 'Body mix',
              b: 'Balance of body resonance against the raw string signal.' },
        fr: { t: 'Mix du corps',
              b: 'Équilibre entre la résonance du corps et le signal brut de la corde.',
              reviewed: true },
    },
    'BRIGHTNESS': {
        en: { t: 'Brightness',
              b: 'Master tone low-pass, 80 Hz–12 kHz. Below it the instrument darkens toward felt.' },
        fr: { t: 'Brillance',
              b: "Passe-bas de tonalité générale, 80 Hz–12 kHz. En dessous, l’instrument s’assombrit vers le feutre.",
              reviewed: true },
    },
    'canvas-spectrum': {
        en: { t: 'Body response',
              b: "The resonator's frequency response — the eight modal peaks the string drives. Moves with Size, Damping and Brightness." },
        fr: { t: 'Réponse du corps',
              b: 'La réponse en fréquence du résonateur — les huit pics modaux que la corde excite. Suit Taille, Amortissement et Brillance.',
              reviewed: true },
    },

    // ── III · Strings ───────────────────────────────────────────────────────
    'STRING_TENSION': {
        en: { t: 'Tension',
              b: 'String tension. Tighter strings respond faster and sound more focused; slack strings growl and flex.' },
        fr: { t: 'Tension',
              b: 'Tension des cordes. Des cordes plus tendues répondent plus vite et donnent un son plus focalisé ; des cordes lâches grondent et fléchissent.',
              reviewed: true },
    },
    'STRING_STIFFNESS': {
        en: { t: 'Stiffness',
              b: 'String inharmonicity. Stiffer strings sharpen the upper partials — wound steel versus gut.' },
        fr: { t: 'Raideur',
              b: 'Inharmonicité des cordes. Des cordes plus raides haussent les partiels aigus — acier filé contre boyau.',
              reviewed: true },
    },
    'active-strings': {
        en: { t: 'Active strings',
              b: 'How many of the four strings (E–A–D–G, low to high) are on the instrument. Notes land on the string that can reach them.' },
        fr: { t: 'Cordes actives',
              b: "Combien des quatre cordes (E–A–D–G, de la grave à l’aiguë) sont montées sur l’instrument. Chaque note se place sur la corde capable de l’atteindre.",
              reviewed: true },
    },
    'fine-tuners': {
        en: { t: 'Fine tuners',
              b: 'Per-string detune in cents, ±1200¢. Double-click a fader to return it to 0.' },
        fr: { t: 'Tendeurs fins',
              b: 'Désaccord par corde en cents, ±1200¢. Double-cliquer sur un curseur pour le remettre à 0.',
              reviewed: true },
    },
    'DETUNE_E': {
        en: { t: 'E fine tuner',
              b: 'Detune of the low E string in cents, ±1200¢.' },
        fr: { t: 'Tendeur fin E',
              b: 'Désaccord de la corde grave E en cents, ±1200¢.',
              reviewed: true },
    },
    'DETUNE_A': {
        en: { t: 'A fine tuner',
              b: 'Detune of the A string in cents, ±1200¢.' },
        fr: { t: 'Tendeur fin A',
              b: 'Désaccord de la corde A en cents, ±1200¢.',
              reviewed: true },
    },
    'DETUNE_D': {
        en: { t: 'D fine tuner',
              b: 'Detune of the D string in cents, ±1200¢.' },
        fr: { t: 'Tendeur fin D',
              b: 'Désaccord de la corde D en cents, ±1200¢.',
              reviewed: true },
    },
    'DETUNE_G': {
        en: { t: 'G fine tuner',
              b: 'Detune of the high G string in cents, ±1200¢.' },
        fr: { t: 'Tendeur fin G',
              b: "Désaccord de la corde aiguë G en cents, ±1200¢.",
              reviewed: true },
    },

    // ── IV · Expression ─────────────────────────────────────────────────────
    'EXPRESSION_MACRO': {
        en: { t: 'Expression',
              b: 'One-knob intensity macro — leans bow speed, pressure and vibrato together from restful to fervent.' },
        fr: { t: 'Expression',
              b: "Macro d’intensité à un seul bouton — oriente ensemble vitesse d’archet, pression et vibrato, du calme à la ferveur.",
              reviewed: true },
    },
    'VIBRATO_RATE': {
        en: { t: 'Vibrato rate',
              b: 'Vibrato speed, 0.1–12 Hz.' },
        fr: { t: 'Vitesse du vibrato',
              b: 'Vitesse du vibrato, 0,1–12 Hz.',
              reviewed: true },
    },
    'VIBRATO_DEPTH': {
        en: { t: 'Vibrato depth',
              b: 'Vibrato width in cents, 0–50¢. At 0 the left hand holds still.' },
        fr: { t: 'Profondeur du vibrato',
              b: 'Largeur du vibrato en cents, 0–50¢. À 0, la main gauche reste immobile.',
              reviewed: true },
    },
    'VIBRATO_ONSET': {
        en: { t: 'Vibrato onset',
              b: 'Delay before vibrato blooms after a note starts, 0–3000 ms — the straight-tone attack of a real player.' },
        fr: { t: 'Entrée du vibrato',
              b: "Délai avant l’épanouissement du vibrato après le début d’une note, 0–3000 ms — l’attaque en son droit d’un vrai instrumentiste.",
              reviewed: true },
    },
    'SLOW_LFO_RATE': {
        en: { t: 'LFO rate',
              b: 'Rate of the slow bow drift, 0.05–2 Hz — a once-per-phrase wander of the bowing.' },
        fr: { t: 'Vitesse du LFO',
              b: "Vitesse de la dérive lente d’archet, 0,05–2 Hz — une errance du coup d’archet, une fois par phrase.",
              reviewed: true },
    },
    'SLOW_LFO_DEPTH': {
        en: { t: 'LFO depth',
              b: 'Depth of the slow bow drift. At 0 the bow holds perfectly steady.' },
        fr: { t: 'Profondeur du LFO',
              b: "Profondeur de la dérive lente d’archet. À 0, l’archet reste parfaitement stable.",
              reviewed: true },
    },

    // ── V · Drone ───────────────────────────────────────────────────────────
    'INFINITE_SUSTAIN': {
        en: { t: 'Infinite sustain',
              b: 'Bow-forever amount. Raise it and held notes stop decaying — the drone wakes.' },
        fr: { t: 'Tenue infinie',
              termNote: 'not the ADSR sustain segment — this page carries no envelope at all, '
                      + 'only a Release. The control stops the string losing energy so the note '
                      + 'keeps sounding, and "une tenue" is the French musical term for a held '
                      + 'note. MEANING, not width: a tip title is unconstrained, and the caption '
                      + 'half fits too (Maint. inf. 50.58 px in a 62 px cell). Converges with '
                      + 'O-Bowed v1.6.1, which reasoned the same exemption on the same control.',
              b: "Dosage de l’archet perpétuel. En le montant, les notes tenues cessent de décroître — le bourdon s’éveille.",
              reviewed: true },
    },
    'SUB_HARMONICS': {
        en: { t: 'Sub-harmonics',
              b: 'Level of the sub-harmonic layer beneath the played note — tectonic weight under the drone.' },
        fr: { t: 'Sous-harmoniques',
              b: 'Niveau de la couche sous-harmonique sous la note jouée — un poids tectonique sous le bourdon.',
              reviewed: true },
    },

    // ── VI · Output ─────────────────────────────────────────────────────────
    'OUTPUT_GAIN': {
        en: { t: 'Level',
              b: 'Output level, −60 to +12 dB.' },
        fr: { t: 'Niveau',
              b: 'Niveau de sortie, −60 à +12 dB.',
              reviewed: true },
    },
    'WIDTH': {
        en: { t: 'Width',
              b: 'Stereo width. 1.00× is the natural body image; 0 collapses to mono, 2× exaggerates.' },
        fr: { t: 'Largeur',
              b: "Largeur stéréo. 1,00× est l’image naturelle du corps ; 0 replie l’image en mono, 2× exagère.",
              reviewed: true },
    },
    'MASTER_SAT_AMOUNT': {
        en: { t: 'Saturate',
              b: 'Master saturator blend — gentle harmonic warmth on the summed output.' },
        fr: { t: 'Saturation',
              b: 'Dosage du saturateur général — une chaleur harmonique douce sur la sortie sommée.',
              reviewed: true },
    },
    'LIMITER_CEILING_DB': {
        en: { t: 'Ceiling',
              b: 'Limiter ceiling, −6 to 0 dBFS. The output never crosses this.' },
        fr: { t: 'Plafond',
              b: 'Plafond du limiteur, −6 à 0 dBFS. La sortie ne le franchit jamais.',
              reviewed: true },
    },
    'canvas-vu': {
        en: { t: 'VU meter',
              b: 'Output level after the limiter, VU-ballistic. 0 VU = −18 dBFS RMS.' },
        fr: { t: 'VU-mètre',
              b: 'Niveau de sortie après le limiteur, balistique VU. 0 VU = −18 dBFS RMS.',
              reviewed: true },
    },

    // ── VII · Microtonal ────────────────────────────────────────────────────
    'REFERENCE_PITCH': {
        en: { t: 'Reference pitch',
              b: 'Concert pitch for A4, 220–880 Hz. Everything tunes around it.' },
        fr: { t: 'Diapason',
              b: "Hauteur de référence du A4, 220–880 Hz. Tout s’accorde autour d’elle.",
              reviewed: true },
    },
    'tuning-system': {
        en: { t: 'Tuning system',
              b: 'Where the tuning comes from — a loaded Scala scale, an MTS-ESP master, or plain 12-TET.' },
        fr: { t: "Système d’accord",
              b: "D’où vient l’accord — une gamme Scala chargée, une source maître MTS-ESP ou le 12-TET simple.",
              reviewed: true },
    },
    'scl-load-btn': {
        en: { t: 'Load .scl',
              b: 'Load a Scala tuning file (.scl) and switch the tuning system to it.' },
        fr: { t: 'Charger .scl',
              b: "Charger un fichier d’accord Scala (.scl) et y basculer le système d’accord.",
              reviewed: true },
    },
    'note-expression-toggle': {
        en: { t: 'Note Expression',
              b: 'Per-note VST3 pitch (Note Expression) for hosts like Cubase and Dorico — microtonal scores play in tune.' },
        fr: { t: 'Note Expression',
              b: "Hauteur VST3 par note (Note Expression) pour les hôtes comme Cubase et Dorico — les partitions microtonales sonnent juste.",
              reviewed: true },
    },
});

// ============================================================================
// LABELS — the page's own captions, v1.8.1
//
// Separate from I18N because a tooltip entry is a {title, body} PAIR and a
// label is one string. trLabel() falls back to I18N, so a control whose tooltip
// TITLE already IS its caption carries ONE key rather than two copies of the
// same string in two tables, drifting apart.
//
// THE REUSE RULE (settled in Stage F, O-Tapestop): a label reuses a tooltip key
// ONLY where the string is identical in BOTH languages. An English-only match
// is not enough — reusing there would make every future tooltip copy edit a
// silent geometry change to a control.
//
// ALL FRENCH IS MACHINE-DRAFTED, `reviewed: false`.
// ============================================================================

export const LABELS = Object.freeze({

    // ── The header plate line ───────────────────────────────────────────────
    // The Latin binomial is a scientific name and is identical in French; only
    // the plate numbering is translated. Rendered inside an SVG <text>, which
    // is text-anchor="middle" in a fixed viewBox, so a wider string grows
    // symmetrically about its own centre and moves no sibling.
    'label.plate': {
        en: { t: 'Physeter macrocephalus — Plate VII' },
        fr: { t: 'Physeter macrocephalus — Planche VII', reviewed: true },
    },

    // ── Preset bar and tab strip ────────────────────────────────────────────
    // NOT the `preset-save` tooltip key: that title is "Enregistrer", and this
    // button sits in a header band that already carries the wordmark, a preset
    // readout, two nav arrows, the tab pair, the gear and the brand line.
    // v1.8.1 — this defence was TESTED rather than inherited (Stage N found
    // five headers wrong about the string they defended). Planting the root
    // "Enregistrer" (65.73 px) grows the button 66 -> 77.61 px and MOVES 12
    // elements: the nav arrows compress 21.81 -> 20.84, #preset-name shifts
    // left 0.96, and the whole tab strip plus the gear slide right 9.69 px.
    // check-ui-labels assertion 7 would fail on it. "Enreg." (38.00 px) and
    // "Enreg" (34.27) both move nothing; the glossary lists both. The period
    // stays because this button carries no aria-label, so its accessible name
    // IS its caption and there is no label-in-name substring to close.
    'label.save':       { en: { t: 'Save' }, fr: { t: 'Enreg.', reviewed: true } },
    // The tab captions DO reuse their tooltip titles: "Main"/"Principal" and
    // "Tuning"/"Accord" are identical strings in both tables, so a single key
    // is correct under the reuse rule. Declared here anyway rather than pointed
    // at I18N, because the tab strip is a fixed two-button row where a caption
    // width change is a geometry change, and a tooltip copy edit must not be
    // able to move it.
    'label.tabMain':    { en: { t: 'Main' },   fr: { t: 'Principal', reviewed: true } },
    'label.tabTuning':  { en: { t: 'Tuning' }, fr: { t: 'Accord',    reviewed: true } },

    // ── The two faces of the hover-help toggle ──────────────────────────────
    // The ONLY strings on this page written from script. They go through
    // setLabel(), so the element becomes a [data-i18n] element and the language
    // sweep owns it from that moment on. Written as two calls behind an
    // if/else, never one call with a ternary — check-i18n assertion 13.
    //
    // "Marche"/"Arrêt" rather than "Activé"/"Désactivé": the button is 44 px,
    // and this is the vocabulary a piece of hardware uses, which is the
    // register this instrument panel is written in.
    'ui.on':            { en: { t: 'On' },  fr: { t: 'Marche', reviewed: true } },
    'ui.off':           { en: { t: 'Off' }, fr: { t: 'Arrêt',  reviewed: true } },

    // ── Panel captions (the Roman numeral is a sibling, never inside these) ──
    // The interpunct belongs to the caption, not to the numeral beside it —
    // the same shape as O-Bitrot's em-dashed plate captions.
    'label.secBow':        { en: { t: ' · Bow' },        fr: { t: ' · Archet',     reviewed: true } },
    'label.secBody':       { en: { t: ' · Body' },       fr: { t: ' · Corps',      reviewed: true } },
    'label.secStrings':    { en: { t: ' · Strings' },    fr: { t: ' · Cordes',     reviewed: true } },
    'label.secExpression': { en: { t: ' · Expression' }, fr: { t: ' · Expression', reviewed: true, sameAsEn: true } },
    'label.secDrone':      { en: { t: ' · Drone' },      fr: { t: ' · Bourdon',    reviewed: true } },
    'label.secOutput':     { en: { t: ' · Output' },     fr: { t: ' · Sortie',     reviewed: true } },
    'label.secMicrotonal': { en: { t: ' · Microtonal' }, fr: { t: ' · Microtonal', reviewed: true, sameAsEn: true } },
    'label.secSchelleng':  { en: { t: 'Schelleng Diagram' },
                             fr: { t: 'Diagramme de Schelleng', reviewed: true } },

    // ── Panel sublabels, in the small italic hand ───────────────────────────
    'label.subResonator':  { en: { t: 'resonator · 8 modes' },
                             fr: { t: 'résonateur · 8 modes', reviewed: true } },
    'label.subVibrato':    { en: { t: 'vibrato · bow drift' },
                             fr: { t: "vibrato · dérive d’archet", reviewed: true } },
    // SIZED, and recorded here with the measurement that forced it. The full
    // form "point de fonctionnement de l'archet" is 156.3 px, and this panel
    // head is 314 px carrying a 167.4 px title beside it, so the pair wrapped
    // to two lines and pushed the Schelleng canvas down 11 px. The panel is
    // already titled "Diagramme de Schelleng", so "de l'archet" was saying
    // twice what the title says once.
    'label.subOperating':  { en: { t: 'bow operating point' },
                             fr: { t: 'point de fonctionnement', reviewed: true } },
    'label.vizCaption':    { en: { t: 'speed × pressure, log–log · wedge shifts with β' },
                             fr: { t: 'vitesse × pression, log–log · le coin suit β', reviewed: true } },

    // ── Knob captions ───────────────────────────────────────────────────────
    // v1.8.1 — RE-MEASURED. The cell is 62 px, not the 55 px this block said
    // through v1.8.0: .knob-control is width:62px and .knob-label is
    // max-width:62px / nowrap / overflow:hidden + text-overflow:ellipsis
    // (index.html:544, :570). A caption over 62 px does not push a neighbour —
    // it is ELLIPSIS-TRUNCATED in place, which check-ui-labels assertion 4
    // catches. Every French caption below was measured with
    // Range.selectNodeContents on its own node at the shipping 1000x650 frame.
    // Where the tooltip title is longer than the cell the caption gets its own
    // shorter key, per the reuse rule.
    'label.speed':      { en: { t: 'Speed' },      fr: { t: 'Vitesse',   reviewed: true } },
    'label.pressure':   { en: { t: 'Pressure' },   fr: { t: 'Pression',  reviewed: true } },
    'label.position':   { en: { t: 'Position' },   fr: { t: 'Position',  reviewed: true, sameAsEn: true } },
    'label.rosin':      { en: { t: 'Rosin' },      fr: { t: 'Colophane', reviewed: true } },
    'label.noise':      { en: { t: 'Noise' },      fr: { t: 'Bruit',     reviewed: true } },
    // v1.8.1 — "Chute" was the caption for a title that read "Extinction"; the
    // glossary settles Release on Relâchement, and forbids both words. The root
    // measures 64.95 px and wraps the 62 px cell, so the caption takes the
    // listed abbreviation Relâch. (36.95 px) while the tip carries the root.
    'label.release':    { en: { t: 'Release' },    fr: { t: 'Relâch.',   reviewed: true } },
    'label.size':       { en: { t: 'Size' },       fr: { t: 'Taille',    reviewed: true } },
    // "Amortissement" measures 74.27 px — not the 84 px this line claimed —
    // and still wraps the 62 px cell, so "Amort." (33.27 px) stands. The
    // glossary lists it. Re-measured at v1.8.1; the verdict did not change.
    'label.damping':    { en: { t: 'Damping' },    fr: { t: 'Amort.',    reviewed: true } },
    'label.mix':        { en: { t: 'Mix' },        fr: { t: 'Mix',       reviewed: true, sameAsEn: true } },
    'label.brightness': { en: { t: 'Brightness' }, fr: { t: 'Brillance', reviewed: true } },
    'label.tension':    { en: { t: 'Tension' },    fr: { t: 'Tension',   reviewed: true, sameAsEn: true } },
    'label.stiffness':  { en: { t: 'Stiffness' },  fr: { t: 'Raideur',   reviewed: true } },
    'label.expression': { en: { t: 'Expression' }, fr: { t: 'Expression', reviewed: true, sameAsEn: true } },
    // v1.8.1 — the three vibrato captions and the two LFO captions now carry
    // the glossary's own abbreviations, head first the way French inverts them:
    // Vit. vibr. 41.94 px, Prof. vibr. 50.08, Entrée vibr. 59.36, Vit. LFO
    // 35.47, Prof. LFO 43.61 — all one line in the 62 px cell. The roots do not
    // all fit (Vit. vibrato 56.66 does, Prof. vibrato 64.80 wraps), and a
    // matched pair is worth more here than one root beside one abbreviation.
    'label.vibRate':    { en: { t: 'Vib Rate' },   fr: { t: 'Vit. vibr.', reviewed: true } },
    'label.vibDepth':   { en: { t: 'Vib Depth' },  fr: { t: 'Prof. vibr.', reviewed: true } },
    'label.vibOnset':   { en: { t: 'Vib Onset' },  fr: { t: 'Entrée vibr.', reviewed: true } },
    'label.lfoRate':    { en: { t: 'LFO Rate' },   fr: { t: 'Vit. LFO',   reviewed: true } },
    'label.lfoDepth':   { en: { t: 'LFO Depth' },  fr: { t: 'Prof. LFO',  reviewed: true } },
    // The caption half of INFINITE_SUSTAIN’s exemption. Measured in this page’s
    // own 62 px .knob-label: Tenue inf. 48.73 px, Maint. inf. 50.58 — both fit,
    // so the exemption is MEANING, not width. Maintien inf. is 63.52 and wraps.
    'label.infSustain': { en: { t: 'Inf. Sustain' },
                          fr: { t: 'Tenue inf.', reviewed: true,
                                termNote: 'the caption half of INFINITE_SUSTAIN’s exemption — '
                                        + 'sustain here is a note that keeps sounding, not the '
                                        + 'ADSR segment; this page has no envelope' } },
    'label.subHarm':    { en: { t: 'Sub-Harm.' },  fr: { t: 'Sous-harm.', reviewed: true } },
    'label.level':      { en: { t: 'Level' },      fr: { t: 'Niveau',    reviewed: true } },
    'label.width':      { en: { t: 'Width' },      fr: { t: 'Largeur',   reviewed: true } },
    // v1.8.1 — "Saturation" measures 55.00 px, NOT the 66 px this line claimed,
    // and fits the 62 px cell with 7 px to spare. The English caption is not
    // abbreviated either, so the French stops being: one control, one name.
    'label.saturate':   { en: { t: 'Saturate' },   fr: { t: 'Saturation', reviewed: true } },
    'label.ceiling':    { en: { t: 'Ceiling' },    fr: { t: 'Plafond',   reviewed: true } },

    // ── Strings panel ───────────────────────────────────────────────────────
    'label.activeStrings': { en: { t: 'Active Strings' }, fr: { t: 'Cordes actives', reviewed: true } },
    'label.fineTuners':    { en: { t: 'Fine Tuners · cents' },
                             fr: { t: 'Tendeurs fins · cents', reviewed: true } },
    // The Active Strings readout. NOT a bare number with a unit symbol, so it
    // is not covered by D-03: "of" is a connective word and reads as English
    // prose on a French page. The NUMBER stays a number and is substituted in
    // as {n}; only the connective is localized. Written through setLabel() with
    // vars, so the language sweep re-renders it in place.
    'readout.activeStrings': { en: { t: '{n} of 4' }, fr: { t: '{n} sur 4', reviewed: true } },

    // ── Drone panel caption (two spans either side of a <br>) ───────────────
    'label.silentAtRest':  { en: { t: 'silent at rest —' },
                             fr: { t: 'silencieux au repos —', reviewed: true } },
    'label.raiseToWake':   { en: { t: 'raise to wake' },
                             fr: { t: 'monter pour éveiller', reviewed: true } },

    // ── Microtonal strip ────────────────────────────────────────────────────
    // .strip-field-label is text-transform: uppercase, so this renders SYSTÈME
    // D’ACCORD on screen; the table keeps the English caption's mixed case,
    // which is what the lint and the accessible name read. The typographic
    // apostrophe costs 0.09 px (97.52 -> 97.61) inside the 98 px pin.
    'label.tuningSystem':  { en: { t: 'Tuning System' }, fr: { t: "Système d’accord", reviewed: true } },
    'label.loadScl':       { en: { t: 'Load .scl…' },    fr: { t: 'Charger .scl…',    reviewed: true } },
    'label.noteExpression': { en: { t: 'Note Expression' },
                              fr: { t: 'Note Expression', reviewed: true, sameAsEn: true } },
    // "monophonic" is the only translated half; the pitch range is scientific
    // pitch notation and is exempt for the reason given in I18N_EXEMPT.
    'label.rangeCaption':  { en: { t: 'E1–G3 · monophonic' },
                             fr: { t: 'E1–G3 · monophonique', reviewed: true } },

    // ── The Tuning tab's load-failure notice ────────────────────────────────
    // Through v1.7.2 this was an innerHTML string literal in the catch arm. It
    // is the one prose string this page writes from script that is not a
    // toggle face, and it went through untranslated on a French page.
    // v1.8.1 — the glossary's settled form, which O-Bassoon, O-Bowed, O-Reed
    // and O-Wind already ship. The copies converging is the point; the panel
    // is shared code. (The glossary's TERMS key for it ends in a period, which
    // the lint's own norm() strips before the lookup, so G1 can never reach
    // this row — reported to the orchestrator, not worked around.)
    'label.tuningLoadFailed': { en: { t: 'Tuning panel failed to load.' },
                                fr: { t: 'Échec du chargement du panneau d’accord.', reviewed: true } },

    // ── Accessible names ────────────────────────────────────────────────────
    // An aria-label is user-visible text by any definition that matters — it is
    // the accessible NAME, and a screen reader in French reading an English
    // name is the same failure as a French page with an English caption. None
    // has a rendered box, so none is a geometry risk.
    'aria.presetPrev':  { en: { t: 'Previous preset' }, fr: { t: 'Préréglage précédent', reviewed: true } },
    'aria.presetNext':  { en: { t: 'Next preset' },     fr: { t: 'Préréglage suivant',   reviewed: true } },
    'aria.helpToggle':  { en: { t: 'Toggle hover help' },
                          fr: { t: "Activer ou désactiver l’aide au survol", reviewed: true } },
    'aria.langSelect':  { en: { t: 'Interface language' },
                          fr: { t: "Langue de l’interface", reviewed: true } },
    'aria.settings':    { en: { t: 'Settings' }, fr: { t: 'Réglages', reviewed: true } },
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
    ['O-Contrabass',   'the product name — a product name is never translated'],
    ['Contrabass',     'the second half of the wordmark, split around a styled hyphen span'],
    ['Ouaricon · Naturalist Series', 'the company name and the product-line name'],

    // #preset-name displays the loaded preset. The name IS the JSON filename
    // (OuariconPresetManager.h:283-285), so translating it breaks recall: a
    // session saved against "Arco Foundation" would not resolve its French.
    ['Default — Arco Foundation',
     'a factory preset name — exempt under D-02, because the name IS the JSON filename'],

    // The Roman numerals of the seven panels. A Roman numeral is a Roman
    // numeral. Listed individually rather than as a pattern: an exemption
    // matching /^[IVX]+$/ would silently swallow a future caption that happened
    // to be spelled the same way.
    ['I',   'panel numbering — a Roman numeral is identical in French'],
    ['II',  'panel numbering — identical in French'],
    ['III', 'panel numbering — identical in French'],
    ['IV',  'panel numbering — identical in French'],
    ['V',   'panel numbering — identical in French'],
    ['VI',  'panel numbering — identical in French'],
    ['VII', 'panel numbering — identical in French'],

    // The four open strings, and the two range captions that name them. This
    // instrument's strings are addressed as E-A-D-G everywhere it matters: the
    // four DETUNE_* parameter IDs, the four data-string attributes, the DAW's
    // own note display and every score the plugin will be played from. French
    // solfege (Mi-La-Re-Sol) would break that correspondence in the one place a
    // player checks it against the host. Scientific pitch notation for the
    // range is the same decision.
    ['E', 'open-string name — the four strings are E-A-D-G in the parameter IDs, in the host and on the page'],
    ['A', 'open-string name — see E'],
    ['D', 'open-string name — see E'],
    ['G', 'open-string name — see E'],
    ['A4', 'scientific pitch notation for the reference pitch — see E'],
    ['E1–G3', 'the playable range in scientific pitch notation — see E'],

    // The bowing direction, printed in the small italic hand under the Bow
    // panel. An Italian performance term, used unchanged in French scores.
    ['arco', 'an Italian performance direction — used unchanged in French'],

    // The three tuning sources. A file format, a protocol and a notation.
    ['Scala',   'the name of a tuning file format — a format name is not translated'],
    ['MTS-ESP', 'the name of a tuning protocol (ODDSound MTS-ESP)'],
    ['12-TET',  'the standard notation for twelve-tone equal temperament'],

    // The two endonyms in the language selector. A language name is never
    // translated: a French speaker looking for their language looks for
    // "Français".
    ['English',  'endonym — a language name is never translated'],
    ['Français', 'endonym — a language name is never translated'],

    // The face of the gear and the tick marks of the VU scale are glyphs and
    // numbers, not prose.
    ['⚙', 'the gear glyph'],
    ['◀', 'the previous-preset glyph'],
    ['▶', 'the next-preset glyph'],
];

// ============================================================================
// TIP_BINDINGS — [selector, key, wrapper?, vars?]
//
// The tip anchor IS the element the selector finds on every row here: unlike
// O-Bitrot, this page authors its data-tip on the .knob-control / .finetuner
// cell itself rather than on the knob inside it, so no closest(wrapper) walk is
// needed anywhere.
//
// Five anchors carry neither an id nor a data-param — the two tab buttons, the
// Active Strings block, the Fine Tuners title and the Tuning System field. Each
// is addressed by a selector that matches exactly one element on the page.
// ============================================================================

export const TIP_BINDINGS = [
    ['#gear-btn',                            'gear-btn'],
    ['#lang-select',                         'lang-select'],
    ['#help-toggle',                         'help-toggle'],

    ['#preset-prev',                         'preset-prev'],
    ['#preset-name',                         'preset-name'],
    ['#preset-next',                         'preset-next'],
    ['#preset-save',                         'preset-save'],
    ['[data-tab="main"]',                    'tab-main'],
    ['[data-tab="tuning"]',                  'tab-tuning'],

    ['[data-param="BOW_SPEED"]',             'BOW_SPEED'],
    ['[data-param="BOW_PRESSURE"]',          'BOW_PRESSURE'],
    ['[data-param="BOW_POSITION"]',          'BOW_POSITION'],
    ['[data-param="ROSIN"]',                 'ROSIN'],
    ['[data-param="BOW_NOISE"]',             'BOW_NOISE'],
    ['[data-param="RELEASE"]',               'RELEASE'],
    ['#canvas-schelleng',                    'canvas-schelleng'],

    ['[data-param="BODY_SIZE"]',             'BODY_SIZE'],
    ['[data-param="BODY_DAMPING"]',          'BODY_DAMPING'],
    ['[data-param="BODY_MIX"]',              'BODY_MIX'],
    ['[data-param="BRIGHTNESS"]',            'BRIGHTNESS'],
    ['#canvas-spectrum',                     'canvas-spectrum'],

    ['[data-param="STRING_TENSION"]',        'STRING_TENSION'],
    ['[data-param="STRING_STIFFNESS"]',      'STRING_STIFFNESS'],
    ['.stepper-block',                       'active-strings'],
    ['.finetuner-title',                     'fine-tuners'],
    ['[data-param="DETUNE_E"]',              'DETUNE_E'],
    ['[data-param="DETUNE_A"]',              'DETUNE_A'],
    ['[data-param="DETUNE_D"]',              'DETUNE_D'],
    ['[data-param="DETUNE_G"]',              'DETUNE_G'],

    ['[data-param="EXPRESSION_MACRO"]',      'EXPRESSION_MACRO'],
    ['[data-param="VIBRATO_RATE"]',          'VIBRATO_RATE'],
    ['[data-param="VIBRATO_DEPTH"]',         'VIBRATO_DEPTH'],
    ['[data-param="VIBRATO_ONSET"]',         'VIBRATO_ONSET'],
    ['[data-param="SLOW_LFO_RATE"]',         'SLOW_LFO_RATE'],
    ['[data-param="SLOW_LFO_DEPTH"]',        'SLOW_LFO_DEPTH'],

    ['[data-param="INFINITE_SUSTAIN"]',      'INFINITE_SUSTAIN'],
    ['[data-param="SUB_HARMONICS"]',         'SUB_HARMONICS'],

    ['[data-param="OUTPUT_GAIN"]',           'OUTPUT_GAIN'],
    ['[data-param="WIDTH"]',                 'WIDTH'],
    ['[data-param="MASTER_SAT_AMOUNT"]',     'MASTER_SAT_AMOUNT'],
    ['[data-param="LIMITER_CEILING_DB"]',    'LIMITER_CEILING_DB'],
    ['#canvas-vu',                           'canvas-vu'],

    ['[data-param="REFERENCE_PITCH"]',       'REFERENCE_PITCH'],
    ['.strip-field',                         'tuning-system'],
    ['#scl-load-btn',                        'scl-load-btn'],
    ['#note-expression-toggle',              'note-expression-toggle'],
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
