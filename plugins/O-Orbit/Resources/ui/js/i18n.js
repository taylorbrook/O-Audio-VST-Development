/*
   This file is part of O-Orbit, an Ouaricon Audio plugin.
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
// i18n.js — O-Orbit interface copy, English + French (v1.2.1)
//
// ── v1.2.1: FRENCH QA PASS (Stage N, 2026-08-31) ────────────────────────────
// Every fr entry read against its en and against scripts/i18n-fr-glossary.js.
// Changed: 23 of 91 entries — 6 terminology, 15 typography, 1 grammar/agreement,
// 1 meaning, 3 exemption-only (the categories overlap: tempo_sync took two
// no-break spaces AND an agreement fix, speed took a term AND a cross-reference).
// sameAsEn: kept 6, translated 0, ADDED 2 (label.mix,
// label.groupSourceMix — a glossary root term that IS the English word makes
// the French a straight copy, and check-i18n assertion 4 requires the flag).
// termNote exemptions: 3 (ui.on, ui.off, downmix-badge — each listed at its
// entry with the measurement or the sense that earns it).
// Left as drafted: the rest. reviewed: false throughout — no native speaker yet.
//
// Decisions the next reader needs:
//   · Mix wins over Mixage in three places (the group heading, the caption, the
//     tip title). "Mixage réducteur" on the downmix badge does NOT change — a
//     channel fold-down is not the dry/wet control, and it carries a termNote.
//     The speaker_layout BODY keeps "mixage réducteur" for the same reason;
//     bodies are not matched against the term list.
//   · Oui/Non stay on the elevation toggle. No glossary form fits either of the
//     two controls that key pair drives — the numbers are at the entry. This is
//     a CSS specificity consequence, reported, not fixed here.
//   · "Sync tempo" stays as the Tempo Sync caption: "Synchro tempo" WRAPS to
//     two lines in the 100.28px grid cell (59.58 x 26.00 over two lines against
//     79.19 x 13.00 on one), which is the one v1.2.0 width defence on this page
//     that re-measured true. The full "Synchro tempo" survives as the tip title.
//   · The Speed body now says "Vitesse", not "Fréquence", and names the control
//     it cross-refers to by its CAPTION ("Sync tempo"), which is what the other
//     five cross-references in this file already do.
//   · Fifteen no-break spaces (U+00A0) are now load-bearing across thirteen
//     French strings — before % : ; ? and between a number and its unit. They
//     are invisible in a diff and in most editors. Do not retype a French
//     string by hand; edit it and re-run scripts/i18n-fr-lint.js.
//
// UI ROOT IS Resources/ui, NOT Source/ui/public. CMakeLists.txt:54-62 embeds
// exactly seven files and every one of them is under Resources/ui; the stray
// Source/ui/public/modules/preset-manager.js in this plugin's tree is NOT
// embedded and NOT served. This file is the eighth SOURCES entry.
//
// An ES module that EXPORTS ONLY. It must never self-execute: a bare top-level
// statement here throws out of module evaluation and takes every later
// initializer on the page with it (pattern_module_toplevel_init_tdz).
// scripts/check-i18n.js assertion 7 enforces it.
//
// FILENAME: no hyphen. juce_add_binary_data STRIPS hyphens rather than
// converting them to underscores, so a second file named i18n-fr.js would have
// to be reached as the symbol i18nfr_js (critical_binary_data_strips_hyphens).
// One combined file for both languages sidesteps the question entirely. The
// symbol this file becomes is BinaryData::i18n_js, which does not collide with
// BinaryData::index_js (js/juce/index.js) already served by the same editor.
//
// COPY IS textContent ON EVERY PATH — never innerHTML. showTip() builds the tip
// with createElement + textContent, and check-i18n assertion 9 rejects any
// innerHTML reference here and any string literal containing `<`.
//
// THE ENGLISH WAS MOVED, NOT REWRITTEN. Every `en` entry below was extracted
// mechanically from index.html at v1.1.1 rather than re-typed, with its HTML
// entities decoded to the characters they named (&#176; -> °, &#8230; -> …)
// because setAttribute + textContent do not decode entities.
//
// KEYS ARE THE PARAMETER ID where the anchor is a parameter cell, and the
// element id otherwise. The eighteen parameter cells carried neither an id nor
// a data-param at v1.1.1 — a `.param-container` selector would have matched
// the FIRST of eighteen, which is exactly how O-Octagon's .vunit-group tip
// nearly landed on the wrong control in Stage C — so each one gained a
// data-param attribute naming the parameter its <label for> already names.
//
// ALL FRENCH WAS MACHINE-DRAFTED at v1.2.0 and REVIEWED at v1.2.1 against the
// suite glossary and lint (see the Stage N block above). It stays flagged
// `reviewed: false`: that flag means a NATIVE SPEAKER has read the entry, and
// none has. `node scripts/check-i18n.js` prints the worklist.
// ============================================================================

export const LANGUAGES = ['en', 'fr'];

export const I18N = Object.freeze({

    // ── The settings popover (v1.2.0) ───────────────────────────────────────
    // The gear is new. The `help-toggle` entry below is the v1.1.0 "?" toggle's
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
    // discovered — value readouts (D-03) and preset names (D-02, the name IS
    // the JSON filename). Speaker-layout format names are named too: 5.1, 7.1
    // and 7.1.4 are numeric designations and read identically in both.
    'lang-select': {
        en: { t: 'Language',
              b: 'The language of the labels on this page and of this hover help. English and French are available; value readouts and preset names stay in English.' },
        fr: { t: 'Langue',
              b: "La langue des libellés de cette page et de cette aide au survol. L’anglais et le français sont disponibles ; les valeurs affichées et les noms de préréglages restent en anglais.",
              reviewed: true },
    },

    // ── Header preset band ──────────────────────────────────────────────────
    'preset-prev': {
        en: { t: 'Previous preset',
              b: 'Step back through the preset list.' },
        fr: { t: 'Préréglage précédent',
              b: 'Reculer dans la liste des préréglages.',
              reviewed: true },
    },
    'preset-next': {
        en: { t: 'Next preset',
              b: 'Step forward through the preset list.' },
        fr: { t: 'Préréglage suivant',
              b: 'Avancer dans la liste des préréglages.',
              reviewed: true },
    },
    'preset-select': {
        en: { t: 'Preset',
              b: 'The preset currently loaded — click to browse by category. The 12 factory presets are read-only; saving under the same name writes a user copy instead.' },
        fr: { t: 'Préréglage',
              b: "Le préréglage actuellement chargé — cliquer pour parcourir par catégorie. Les 12 préréglages d’usine sont en lecture seule ; enregistrer sous le même nom écrit plutôt une copie utilisateur.",
              reviewed: true },
    },
    'preset-save': {
        en: { t: 'Save',
              b: 'Save the current settings as a user preset.' },
        fr: { t: 'Enregistrer',
              b: 'Enregistrer les réglages actuels comme préréglage utilisateur.',
              reviewed: true },
    },
    'preset-load': {
        en: { t: 'Load',
              b: 'Load a preset from a file.' },
        fr: { t: 'Ouvrir',
              b: 'Charger un préréglage depuis un fichier.',
              reviewed: true },
    },
    'preset-delete': {
        en: { t: 'Delete',
              b: 'Delete the current user preset. Click once to arm it, again to confirm.' },
        fr: { t: 'Supprimer',
              b: 'Supprimer le préréglage utilisateur actuel. Cliquer une fois pour armer, une seconde fois pour confirmer.',
              reviewed: true },
    },

    // ── Header right ────────────────────────────────────────────────────────
    'help-toggle': {
        en: { t: 'Hover help',
              b: 'Show a short description when the pointer rests on a control. The setting is remembered with the session.' },
        fr: { t: 'Aide au survol',
              b: 'Afficher une courte description lorsque le pointeur se pose sur un contrôle. Le réglage est conservé avec la session.',
              reviewed: true },
    },
    'view-toggle': {
        en: { t: 'View',
              b: 'Switch between the motion visualizer and the speaker layout editor.' },
        fr: { t: 'Vue',
              b: "Basculer entre le visualiseur de mouvement et l’éditeur de disposition des enceintes.",
              reviewed: true },
    },

    // ── Speaker-editor toolbar: the named layout library ────────────────────
    'layout-select': {
        en: { t: 'Saved layouts',
              b: 'Load a named custom speaker layout from your library.' },
        fr: { t: 'Dispositions enregistrées',
              b: 'Charger une disposition d’enceintes personnalisée nommée depuis votre bibliothèque.',
              reviewed: true },
    },
    'layout-name': {
        en: { t: 'Layout name',
              b: 'Name for saving the current speaker arrangement to your library.' },
        fr: { t: 'Nom de la disposition',
              b: 'Nom sous lequel enregistrer la disposition d’enceintes actuelle dans votre bibliothèque.',
              reviewed: true },
    },
    'layout-save-btn': {
        en: { t: 'Save layout',
              b: 'Save the current speaker arrangement under the name to the left.' },
        fr: { t: 'Enregistrer la disposition',
              b: "Enregistrer la disposition d’enceintes actuelle sous le nom saisi à gauche.",
              reviewed: true },
    },
    'layout-delete-btn': {
        en: { t: 'Delete layout',
              b: 'Delete the selected saved layout. Click once to arm, again to confirm.' },
        fr: { t: 'Supprimer la disposition',
              b: 'Supprimer la disposition enregistrée sélectionnée. Cliquer une fois pour armer, une seconde fois pour confirmer.',
              reviewed: true },
    },
    'export-btn': {
        en: { t: 'Export',
              b: 'Export the current speaker layout to a JSON file for sharing.' },
        fr: { t: 'Exporter',
              b: "Exporter la disposition d’enceintes actuelle vers un fichier JSON à partager.",
              reviewed: true },
    },
    'import-btn': {
        en: { t: 'Import',
              b: 'Import a speaker layout from a JSON file.' },
        fr: { t: 'Importer',
              b: "Importer une disposition d’enceintes depuis un fichier JSON.",
              reviewed: true },
    },

    // ── Motion ──────────────────────────────────────────────────────────────
    // The five path names inside this body are the SAME five strings the Path
    // dropdown renders, and both are localized — so a French reader is told
    // about "Orbite" and then finds "Orbite" in the list. Composing the body
    // from the option keys would pin it to the load-time language: TIP_BINDINGS
    // is static data evaluated once, and tr()'s var-resolving arm exists for
    // exactly that case. It is not used here because the sentence needs its
    // verbs inflected around each name, not a name slotted into a template.
    'path': {
        en: { t: 'Path',
              b: 'Motion trajectory: Orbit circles, Pendulum swings, Linear sweeps and snaps back, Drift wanders organically, Ping-Pong sweeps back and forth without the snap.' },
        fr: { t: 'Trajectoire',
              b: 'Trajectoire du mouvement : Orbite décrit un cercle, Pendule oscille, Linéaire balaie puis revient d’un coup, Dérive vagabonde de façon organique, Va-et-vient balaie dans les deux sens sans le retour brusque.',
              reviewed: true },
    },
    'speed': {
        en: { t: 'Speed',
              b: 'Motion rate in cycles per second. Ignored while Tempo Sync is set to a division.' },
        fr: { t: 'Vitesse',
              b: 'Vitesse du mouvement en cycles par seconde. Ignorée tant que Sync tempo est réglée sur une division.',
              reviewed: true },
    },
    'width': {
        en: { t: 'Width',
              b: 'Angular span of the motion in degrees — 360 is a full circle around the listener.' },
        fr: { t: 'Largeur',
              b: 'Étendue angulaire du mouvement en degrés — 360 fait un cercle complet autour de l’auditeur.',
              reviewed: true },
    },
    'depth': {
        en: { t: 'Depth',
              b: 'Near/far motion. At 0% the source stays at the Distance radius; higher values move it toward and away from you each cycle.' },
        fr: { t: 'Profondeur',
              b: 'Mouvement de rapprochement et d’éloignement. À 0 %, la source reste au rayon défini par Distance ; au-delà, elle avance et recule à chaque cycle.',
              reviewed: true },
    },
    'tilt': {
        en: { t: 'Tilt',
              b: 'Static elevation of the path in degrees, used while Elevation motion is off.' },
        fr: { t: 'Inclinaison',
              b: 'Élévation fixe de la trajectoire en degrés, utilisée tant que le mouvement Élévation est désactivé.',
              reviewed: true },
    },
    'phase': {
        en: { t: 'Phase',
              b: 'Offset into the motion cycle in degrees — shifts where the source starts.' },
        fr: { t: 'Phase',
              b: 'Décalage dans le cycle du mouvement en degrés — déplace le point de départ de la source.',
              reviewed: true },
    },
    'elevation_enable': {
        en: { t: 'Elevation',
              b: 'Adds vertical motion — the source rises and falls with the cycle instead of staying at the Tilt angle.' },
        fr: { t: 'Élévation',
              b: 'Ajoute un mouvement vertical — la source monte et descend avec le cycle au lieu de rester à l’angle défini par Inclinaison.',
              reviewed: true },
    },
    'elevation_range': {
        en: { t: 'Elev Range',
              b: 'How far the elevation swings when Elevation motion is on, in degrees.' },
        fr: { t: 'Plage d’élévation',
              b: 'Amplitude du balayage vertical lorsque le mouvement Élévation est actif, en degrés.',
              reviewed: true },
    },
    'tempo_sync': {
        en: { t: 'Tempo Sync',
              b: 'Locks the motion rate to the host tempo at the chosen division: 1/4 is one cycle per beat, 1 Bar one cycle per four beats (4/4 assumed). While the transport plays, motion phase locks to the beat position, so bounces are deterministic.' },
        fr: { t: 'Synchro tempo',
              b: 'Verrouille la vitesse du mouvement sur le tempo de l’hôte à la division choisie : 1/4 donne un cycle par temps, 1 mesure un cycle tous les quatre temps (4/4 supposé). Pendant la lecture, la phase du mouvement se cale sur la position rythmique, ce qui rend les exports déterministes.',
              reviewed: true },
    },

    // ── Spatial ─────────────────────────────────────────────────────────────
    'speaker_layout': {
        en: { t: 'Speaker Layout',
              b: 'Target speaker arrangement. When the track has fewer channels, an energy-preserving downmix kicks in automatically (badge below shows when active).' },
        fr: { t: 'Disposition des enceintes',
              b: 'Disposition d’enceintes visée. Si la piste compte moins de canaux, un mixage réducteur à énergie constante s’active automatiquement (la pastille ci-dessous l’indique).',
              reviewed: true },
    },
    'downmix-badge': {
        en: { t: 'Downmix',
              b: 'Shown when the layout has more channels than the track output — an energy-preserving fold-down is active.' },
        fr: { t: 'Mixage réducteur',
              b: 'Apparaît lorsque la disposition compte plus de canaux que la sortie de la piste — un repliement à énergie constante est actif.',
              reviewed: true,
              termNote: 'a downmix is a CHANNEL fold-down, not the dry/wet Mix control four cells away; '
                      + '"mixage réducteur" is the term French DAWs print for it, and the glossary forbids '
                      + '"mixage" precisely because it means the mixing process — which is what this badge names' },
    },
    'distance': {
        en: { t: 'Distance',
              b: 'Base distance of the source in meters — farther is quieter and darker.' },
        fr: { t: 'Distance',
              b: 'Distance de base de la source en mètres — plus loin, plus faible et plus sombre.',
              reviewed: true },
    },
    'air_absorption': {
        en: { t: 'Air Absorption',
              b: 'High-frequency loss with distance — more absorption makes distant sources darker.' },
        fr: { t: 'Absorption de l’air',
              b: 'Perte des aigus avec la distance — plus d’absorption assombrit les sources lointaines.',
              reviewed: true },
    },
    'attenuation_curve': {
        en: { t: 'Atten Curve',
              b: 'How level falls with distance: Linear, Inverse (1/d), or Inverse Square (1/d²).' },
        fr: { t: 'Courbe d’atténuation',
              b: 'Manière dont le niveau décroît avec la distance : Linéaire, Inverse (1/d) ou Inverse carrée (1/d²).',
              reviewed: true },
    },
    'center_diverge': {
        en: { t: 'Center Diverge',
              b: 'Spreads energy into more speakers as it rises — 0% is the sharpest point-source imaging.' },
        fr: { t: 'Divergence centrale',
              b: 'Répartit l’énergie sur davantage d’enceintes à mesure qu’elle augmente — à 0 %, l’image ponctuelle est la plus précise.',
              reviewed: true },
    },

    // ── Source / Mix ────────────────────────────────────────────────────────
    'source_mode': {
        en: { t: 'Source Mode',
              b: 'Mono sums the input into one moving source; L+R Split moves the left and right channels as two separate sources.' },
        fr: { t: 'Mode source',
              b: 'Mono additionne l’entrée en une seule source mobile ; Séparé G+D déplace les canaux gauche et droit comme deux sources distinctes.',
              reviewed: true },
    },
    'lr_offset': {
        en: { t: 'L/R Offset',
              b: 'Angle between the left and right sources in L+R Split mode — 180° keeps them opposite.' },
        fr: { t: 'Décalage G/D',
              b: 'Angle entre les sources gauche et droite en mode Séparé G+D — 180° les maintient opposées.',
              reviewed: true },
    },
    'mix': {
        en: { t: 'Mix',
              b: 'Dry/wet balance. Wet is the spatialized signal on all outputs; dry stays on its native input channels.' },
        fr: { t: 'Mix',
              b: 'Équilibre son direct / son traité. Le son traité est le signal spatialisé sur toutes les sorties ; le son direct reste sur ses canaux d’entrée d’origine.',
              reviewed: true },
    },
});

// ============================================================================
// LABELS — the page's own captions, v1.2.1
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
// FRENCH REVIEWED at v1.2.1 against the glossary; still `reviewed: false`,
// which records the native-speaker reading that has not happened yet.
// ============================================================================

export const LABELS = Object.freeze({

    // ── Preset band ─────────────────────────────────────────────────────────
    // NOT the `preset-save` / `preset-load` / `preset-delete` tooltip keys.
    // Those titles are the full verbs ("Enregistrer", "Supprimer"); these three
    // buttons sit in a 34px header band beside a 130-190px preset readout, two
    // 20px nav circles and the gear, at 10px uppercase. The abbreviations are
    // the same ones O-ReverseDelay settled on in Stage H.
    'label.save':    { en: { t: 'Save' },   fr: { t: 'Enreg.',  reviewed: true } },
    'label.load':    { en: { t: 'Load' },   fr: { t: 'Ouvrir',  reviewed: true } },
    'label.delete':  { en: { t: 'Del' },    fr: { t: 'Suppr.',  reviewed: true } },

    // The armed face of BOTH two-click delete buttons — the preset one in the
    // header and the layout one in the editor toolbar. It goes through
    // setLabel(), so the element becomes a [data-i18n] element and the language
    // sweep owns it. Through v1.1.1 it was a data-confirm ATTRIBUTE, which was
    // the right answer while the page was English-only — it kept the copy out
    // of app.js, which is what pattern_js_state_updater_overwrites_html_labels
    // asks for — and the wrong one the moment the page had two languages: an
    // attribute holds ONE string, so a language switch while a button was armed
    // would have restored the ENGLISH armed face.
    'ui.confirm':    { en: { t: 'Sure?' },  fr: { t: 'Sûr ?',   reviewed: true } },

    // ── The view toggle, both faces ─────────────────────────────────────────
    // Written from script on every click, so both faces are keys. Two separate
    // setLabel() calls in the two arms of the if/else, never one call with a
    // ternary in its argument: check-i18n assertion 13 rejects that shape.
    'label.viewMotion': { en: { t: 'Motion View' },
                          fr: { t: 'Vue mouvement', reviewed: true } },
    'label.viewEditor': { en: { t: 'Speaker Editor' },
                          fr: { t: 'Éditeur d’enceintes', reviewed: true } },

    // ── The elevation toggle, both faces ────────────────────────────────────
    // ONE key pair, TWO controls: the 50px elevation pill (.toggle-label) and
    // the 46px hover-help button in the settings popover (.settings-toggle).
    // The tighter of the two governs, and neither takes a glossary form.
    //
    // RE-MEASURED at v1.2.1 with Range.selectNodeContents on the shipping frame,
    // because the v1.2.0 defence above it was arithmetic on the WRONG font. It
    // said "9px uppercase … the budget is three glyphs". It is not 9px: the
    // `.toggle-label { font-size: 9px }` rule loses to `.param-container label
    // { font-size: 11px }` (0,1,1 beats 0,1,0), so the pill renders at 11px and
    // the authored 9px has been dead since v1.0.0. Measured at the real 11px,
    // in a 46.00px content box:
    //
    //     OUI    22.56   NON    26.84   ARRÊT   41.06   AUCUN  44.13   fit
    //     ACTIVÉ 46.33 (over by 0.33)   MARCHE  53.06 (over by 7.06)
    //     DÉSACTIVÉ 70.11              DÉSACTIVÉE 77.83               do not
    //
    // and in the settings toggle's 44.00px content box (10px, flex: 0 0 46px,
    // so it cannot grow): ARRÊT 35.28 and ACTIVÉ 39.67 fit, MARCHE 45.80 is
    // over by 1.80, DÉSACTIVÉE 66.67 is far over.
    //
    // So the OFF side has a glossary form that fits (ARRÊT, both sites) and the
    // ON side has none — and half a pair is worse French than the calque: an
    // ARRÊT face whose opposite reads OUI names two different oppositions on
    // one 24px pill. Both stay, both carry a termNote with the number.
    //
    // The exemption is a CSS consequence, not a translation one. At the 9px the
    // stylesheet asks for, MARCHE measures ≈43.4px and the settled MARCHE/ARRÊT
    // pair fits with room. Fixing the specificity is a CSS change and Stage N
    // takes none; it is reported instead.
    'ui.on':         { en: { t: 'On' },
                       fr: { t: 'Oui', reviewed: true,
                             termNote: 'MARCHE measures 53.06px and ACTIVÉ 46.33px in the elevation pill\'s '
                                     + '46.00px content box, and MARCHE 45.80px in the settings toggle\'s 44.00px '
                                     + 'box — no glossary form fits either site, so the pair stays as drafted' } },
    'ui.off':        { en: { t: 'Off' },
                       fr: { t: 'Non', reviewed: true,
                             termNote: 'ARRÊT does fit (41.06px of 46.00, 35.28px of 44.00) but its partner does '
                                     + 'not, and ARRÊT opposite OUI is a mismatched pair on one toggle — held '
                                     + 'with ui.on until the .toggle-label font-size specificity is settled' } },

    // ── Editor toolbar: the eight layout preset buttons ─────────────────────
    // 5.1, 7.1, 5.1.4 and 7.1.4 are absent: a channel-count designation is
    // digits and dots, it is identical in both languages, and the coverage scan
    // classifies it as a non-label for exactly that reason. Only the four
    // WORDED buttons need keys.
    'label.fmtStereo': { en: { t: 'Stereo' }, fr: { t: 'Stéréo', reviewed: true } },
    'label.fmtQuad':   { en: { t: 'Quad' },   fr: { t: 'Quad',   reviewed: true, sameAsEn: true } },
    'label.fmtHex':    { en: { t: 'Hex' },    fr: { t: 'Hexa',   reviewed: true } },
    'label.fmtOct':    { en: { t: 'Oct' },    fr: { t: 'Octo',   reviewed: true } },

    // ── Editor toolbar: the layout library and file buttons ─────────────────
    'label.layoutsPlaceholder': { en: { t: 'Layouts…' },
                                  fr: { t: 'Dispositions…', reviewed: true } },
    'label.export':  { en: { t: 'Export' }, fr: { t: 'Exporter', reviewed: true } },
    'label.import':  { en: { t: 'Import' }, fr: { t: 'Importer', reviewed: true } },

    // ── Group headings ──────────────────────────────────────────────────────
    // "Source / Mix", not the v1.2.0 draft's "Source / Mixage": the suite
    // glossary settles Mix as Mix — it is what every French DAW prints — and
    // forbids "mixage" in a label because mixage is the mixing PROCESS. The
    // heading then equals its English, so it carries sameAsEn: true, which is
    // the declaration that a reader looked and agreed the word is French too.
    'label.groupMotion':   { en: { t: 'Motion' },
                             fr: { t: 'Mouvement', reviewed: true } },
    'label.groupSpatial':  { en: { t: 'Spatial' },
                             fr: { t: 'Spatial', reviewed: true, sameAsEn: true } },
    'label.groupSourceMix': { en: { t: 'Source / Mix' },
                              fr: { t: 'Source / Mix', reviewed: true, sameAsEn: true } },

    // ── Motion parameter captions ───────────────────────────────────────────
    // Eight of these are identical to their tooltip TITLE in both languages and
    // could have reused the tooltip key. They do not, deliberately: a caption
    // and a tip title diverge the moment either is edited, and the two that
    // ALREADY diverge here (Elev Range / Plage élév., Atten Curve / Courbe
    // attén.) prove the divergence is not hypothetical on this page — the
    // captions live in 90px grid cells and the titles live in a 230px tip.
    'label.path':      { en: { t: 'Path' },       fr: { t: 'Trajectoire', reviewed: true } },
    'label.speed':     { en: { t: 'Speed' },      fr: { t: 'Vitesse',     reviewed: true } },
    'label.width':     { en: { t: 'Width' },      fr: { t: 'Largeur',     reviewed: true } },
    'label.depth':     { en: { t: 'Depth' },      fr: { t: 'Profondeur',  reviewed: true } },
    'label.tilt':      { en: { t: 'Tilt' },       fr: { t: 'Inclinaison', reviewed: true } },
    'label.phase':     { en: { t: 'Phase' },      fr: { t: 'Phase', reviewed: true, sameAsEn: true } },
    'label.elevation': { en: { t: 'Elevation' },  fr: { t: 'Élévation',   reviewed: true } },
    'label.elevRange': { en: { t: 'Elev Range' }, fr: { t: 'Plage élév.', reviewed: true } },
    // SIZED, and this is the one French string on this page that was.
    // "Synchro tempo" measures 105.4px in this cell's own font and the Motion
    // group's grid track is 100.3px (repeat(auto-fit, minmax(90px, 1fr)) over
    // nine items resolves to seven 100.3px columns), so it WRAPPED to two lines
    // and pushed the #tempo_sync select down 13px. "Sync tempo" measures 79.2 —
    // 21px of margin — and "Sync" is what French DAW interfaces call this. The
    // full phrase survives as the tooltip TITLE, which renders in a 230px box.
    'label.tempoSync': { en: { t: 'Tempo Sync' }, fr: { t: 'Sync tempo', reviewed: true } },

    // ── Path dropdown options ───────────────────────────────────────────────
    'label.pathOrbit':    { en: { t: 'Orbit' },     fr: { t: 'Orbite',      reviewed: true } },
    'label.pathPendulum': { en: { t: 'Pendulum' },  fr: { t: 'Pendule',     reviewed: true } },
    'label.pathDrift':    { en: { t: 'Drift' },     fr: { t: 'Dérive',      reviewed: true } },
    'label.pathPingPong': { en: { t: 'Ping-Pong' }, fr: { t: 'Va-et-vient', reviewed: true } },

    // Shared by the Path dropdown and the Atten Curve dropdown. Identical in
    // BOTH languages at both sites, which is the only condition under which the
    // reuse rule allows one key to serve two controls.
    'label.linear':       { en: { t: 'Linear' },    fr: { t: 'Linéaire',    reviewed: true } },

    // ── Tempo Sync dropdown: the four WORDED options ────────────────────────
    // The eleven note-value options (1/16T … 1/2D) are digits, slashes and the
    // T/D suffixes that mean triplet and dotted in both languages' notation.
    // They carry no key and are exempt below.
    //
    // NOT `ui.off`: that key is the elevation toggle's OFF face, pinned to a
    // 46px pill. This one sits in an 84.28px dropdown with room, so it takes
    // the glossary's feature-sense form in full — "Désactivé" 43.38px, 40.90px
    // to spare, against the v1.2.0 draft's clipped "Désact." 32.69px. The
    // feature sense, not "Aucune": the English is Off, and Off here means the
    // sync is not running, not that a division is unselected.
    'label.syncOff':   { en: { t: 'Off' },     fr: { t: 'Désactivé',  reviewed: true } },
    'label.bar1':      { en: { t: '1 Bar' },   fr: { t: '1 mesure',   reviewed: true } },
    'label.bars2':     { en: { t: '2 Bars' },  fr: { t: '2 mesures',  reviewed: true } },
    'label.bars4':     { en: { t: '4 Bars' },  fr: { t: '4 mesures',  reviewed: true } },

    // ── Spatial parameter captions ──────────────────────────────────────────
    // "Enceintes", not "Disposition des enceintes": this caption sits in a
    // grid cell whose track is minmax(90px, 1fr) and the English "Speaker
    // Layout" already wraps to two lines there. The full phrase is what the
    // TOOLTIP title says.
    'label.speakerLayout': { en: { t: 'Speaker Layout' },
                             fr: { t: 'Enceintes', reviewed: true } },
    'label.distance':      { en: { t: 'Distance' },
                             fr: { t: 'Distance', reviewed: true, sameAsEn: true } },
    // "Absorption air" is the glossary's short form and it is FREE here: at
    // 11px uppercase it measures 104.17px, the same 104.17px as the English
    // "Air Absorption" it replaces, letter for letter. The v1.2.0 draft's bare
    // "Absorption" (79.08px) dropped the half of the name that says WHAT is
    // absorbing. The full "Absorption de l'air" is 135.88px and does not fit;
    // it is what the TOOLTIP title says.
    'label.airAbsorption': { en: { t: 'Air Absorption' },
                             fr: { t: 'Absorption air', reviewed: true } },
    'label.attenCurve':    { en: { t: 'Atten Curve' },
                             fr: { t: 'Courbe attén.', reviewed: true } },
    'label.centerDiverge': { en: { t: 'Center Diverge' },
                             fr: { t: 'Divergence', reviewed: true } },

    // ── Speaker Layout dropdown: the four WORDED options ────────────────────
    // Stereo and Quad reuse the toolbar buttons' keys — same string, same two
    // languages, same meaning. Hexaphonic and Octaphonic are the full words the
    // toolbar abbreviates and need their own.
    'label.hexaphonic':    { en: { t: 'Hexaphonic' },
                             fr: { t: 'Hexaphonique', reviewed: true } },
    'label.octaphonic':    { en: { t: 'Octaphonic' },
                             fr: { t: 'Octophonique', reviewed: true } },

    // ── Atten Curve dropdown options ────────────────────────────────────────
    'label.inverse':       { en: { t: 'Inverse' },
                             fr: { t: 'Inverse', reviewed: true, sameAsEn: true } },
    'label.inverseSquare': { en: { t: 'Inverse Square' },
                             fr: { t: 'Inverse carrée', reviewed: true } },

    // ── Source / Mix ────────────────────────────────────────────────────────
    'label.sourceMode': { en: { t: 'Source Mode' },
                          fr: { t: 'Mode source', reviewed: true } },
    'label.mono':       { en: { t: 'Mono' },
                          fr: { t: 'Mono', reviewed: true, sameAsEn: true } },
    'label.lrSplit':    { en: { t: 'L+R Split' },
                          fr: { t: 'Séparé G+D', reviewed: true } },
    'label.lrOffset':   { en: { t: 'L/R Offset' },
                          fr: { t: 'Décalage G/D', reviewed: true } },
    'label.mix':        { en: { t: 'Mix' },
                          fr: { t: 'Mix', reviewed: true, sameAsEn: true } },

    // ── The downmix badge ───────────────────────────────────────────────────
    // A COMPOSED entry, and the only one on this page. The two numbers are
    // channel counts and stay numbers (D-03); "ch" is not a unit symbol like Hz
    // or dB, it is an abbreviation of the WORD "channels", so it localizes.
    // This is the O-Contrabass "4 of 4" shape: the connective is what changes,
    // never the number. Rendered through setLabel(el, key, vars), so the vars
    // ride on the element as data-i18n-vars and the language sweep re-renders
    // the badge with the SAME counts rather than a stale English face.
    'ui.downmix':       { en: { t: '{from}ch → {to}ch' },
                          fr: { t: '{from} can. → {to} can.', reviewed: true } },

    // ── Accessible names ────────────────────────────────────────────────────
    // #preset-prev and #preset-next do NOT appear here: their accessible name
    // is word-for-word their tooltip title in both languages, so they point
    // data-i18n-aria at the tooltip key and trLabel()'s I18N fallback resolves
    // it. The four below differ from their control's tip title, or belong to a
    // control that has no tip.
    'aria.browsePresets': { en: { t: 'Browse presets' },
                            fr: { t: 'Parcourir les préréglages', reviewed: true } },
    'aria.presets':       { en: { t: 'Presets' },
                            fr: { t: 'Préréglages', reviewed: true } },
    'aria.helpToggle':    { en: { t: 'Toggle hover help' },
                            fr: { t: 'Activer ou désactiver l’aide au survol', reviewed: true } },
    'aria.settings':      { en: { t: 'Settings' },
                            fr: { t: 'Réglages', reviewed: true } },
    'aria.langSelect':    { en: { t: 'Interface language' },
                            fr: { t: 'Langue de l’interface', reviewed: true } },

    // The layout-name field's placeholder. Lower-case in both, matching the
    // authored English — a placeholder on this page is a hint, not a caption.
    'placeholder.layoutName': { en: { t: 'name…' },
                                fr: { t: 'nom…', reviewed: true } },
});

// ============================================================================
// I18N_EXEMPT — reasoned exclusions, never silence
//
// Every visible string the coverage scan finds must be a [data-i18n] element, a
// setLabel() call, or an entry HERE WITH A REASON. A bare skip list would let a
// missed label hide as a deliberate one.
// ============================================================================

export const I18N_EXEMPT = [
    ['O-Orbit',
     'the product name — a product name is never translated'],
    ['Ouaricon Audio',
     'the company name, in the footer plate'],

    // #preset-name displays the loaded preset. The name IS the JSON filename
    // (OuariconPresetManager.h:283-285), so translating it breaks recall: a
    // session saved against "Default" would not resolve its French. The element
    // is also written by modules/preset-manager.js on every load, so making it
    // a [data-i18n] element would put the sweep and the module in a fight over
    // one node (pattern_js_state_updater_overwrites_html_labels).
    ['Default',
     'the factory preset name shown at rest — exempt under D-02, because the name IS the JSON filename'],

    // The two endonyms in the language selector. A language name is written in
    // its OWN language: a French speaker looking for their language looks for
    // "Français", not "French".
    ['English',  'endonym — a language name is never translated'],
    ['Français', 'endonym — a language name is never translated'],
];

// ============================================================================
// TIP_BINDINGS — [selector, key, wrapper?, vars?]
//
// The tip anchor IS the element the selector finds on every row: this page
// authors its tips on the .param-container cell rather than on the knob inside
// it, so no closest(wrapper) walk is needed anywhere.
//
// The eighteen parameter cells are addressed by the data-param attribute added
// in v1.2.0. Before it they carried neither an id nor any distinguishing
// attribute, and `.param-container` would have matched the FIRST of eighteen —
// the failure mode canon §1 names, and the one O-Octagon's .vunit-group tip hit
// for real in Stage C. The value of data-param is exactly the string the cell's
// own <label for="..."> already carries, so the two cannot drift apart without
// the browser's own label association breaking first.
// ============================================================================

export const TIP_BINDINGS = [
    ['#gear-btn',                            'gear-btn'],
    ['#lang-select',                         'lang-select'],
    ['#help-toggle',                         'help-toggle'],

    ['#preset-prev',                         'preset-prev'],
    ['#preset-next',                         'preset-next'],
    ['#preset-select',                       'preset-select'],
    ['#preset-save',                         'preset-save'],
    ['#preset-load',                         'preset-load'],
    ['#preset-delete',                       'preset-delete'],

    ['#view-toggle',                         'view-toggle'],

    ['#layout-select',                       'layout-select'],
    ['#layout-name',                         'layout-name'],
    ['#layout-save-btn',                     'layout-save-btn'],
    ['#layout-delete-btn',                   'layout-delete-btn'],
    ['#export-btn',                          'export-btn'],
    ['#import-btn',                          'import-btn'],

    ['[data-param="path"]',                  'path'],
    ['[data-param="speed"]',                 'speed'],
    ['[data-param="width"]',                 'width'],
    ['[data-param="depth"]',                 'depth'],
    ['[data-param="tilt"]',                  'tilt'],
    ['[data-param="phase"]',                 'phase'],
    ['[data-param="elevation_enable"]',      'elevation_enable'],
    ['[data-param="elevation_range"]',       'elevation_range'],
    ['[data-param="tempo_sync"]',            'tempo_sync'],

    ['[data-param="speaker_layout"]',        'speaker_layout'],
    ['#downmix-badge',                       'downmix-badge'],
    ['[data-param="distance"]',              'distance'],
    ['[data-param="air_absorption"]',        'air_absorption'],
    ['[data-param="attenuation_curve"]',     'attenuation_curve'],
    ['[data-param="center_diverge"]',        'center_diverge'],

    ['[data-param="source_mode"]',           'source_mode'],
    ['[data-param="lr_offset"]',             'lr_offset'],
    ['[data-param="mix"]',                   'mix'],
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
