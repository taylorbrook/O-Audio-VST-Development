/*
   This file is part of O-SpectralShaper, an Ouaricon Audio plugin.
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
// i18n.js — O-SpectralShaper UI copy, English + French (v1.7.2, canon v2)
//
// ── v1.7.2: STAGE O, ITEM 40 (2026-08-31) — no copy changed in this file ──
// The .settings-toggle min-width in css/styles.css is pinned 40px -> 64px so
// the hover-help switch no longer resizes between Activée (49.09px) and
// Désactivée (61.88px). The block below records the widths as measured in
// v1.7.1; the CSS comment above .settings-toggle carries the v1.7.2 numbers.
//
// ── v1.7.1: FRENCH QA PASS (Stage N, 2026-08-31) ──────────────────────────
// Every fr entry read against its en and against scripts/i18n-fr-glossary.js.
// Changed: 21 entries (13 terminology, 5 typography, 0 grammar/agreement,
// 3 meaning). sameAsEn: kept 2, translated 0. termNote exemptions: 1 (listed).
// Left as drafted: the rest. reviewed: false throughout — no native speaker yet.
// Lint 24 -> 0, --strict exit 0.
//
// Decisions the next reader needs:
//
//   RESET KEEPS "Init.", AND THE HEADER'S OLD DEFENCE HOLDS. Three of the nine
//   plugins reviewed before this one had a width defence that measured
//   BACKWARDS, so this one was re-measured with the gate's own method
//   (Range.selectNodeContents on the real node at the shipping 700x500 frame).
//   It stands: the glossary's Réinit. renders .curve-reset-btn at 58.69px and
//   Réinit at 56.30px against a 53.52px min-width pin, and the row is an
//   absolutely positioned right-pinned flex, so the overflow pushes
//   #attack-undo-btn / #attack-redo-btn 5.17px and 2.78px LEFT. Those two are
//   NOT [data-i18n] elements, so that is a check-ui-labels assertion-7
//   geometry regression, not a cosmetic one. Réinitialiser is 96.39px.
//   Recorded as the one termNote on this file.
//
//   SUSTAIN IS "Maintien", NOT "Tenue" — the textbook ADSR term, applied to the
//   caption, both tip titles and every body that named the curve. Measured at
//   57.44px in an 80px .knob-label, no wrap, .knob-wrapper height unchanged
//   at 87px.
//
//   "déflexion" BECAME "débattement". The English is "Full deflection is
//   ±12 dB"; déflexion is a calque, and a control's full travel is a
//   débattement in French. Both curve bodies carried the sentence.
//
//   THREE BODIES NAMED A CONTROL BY ITS ENGLISH CAPTION. The preset body said
//   "utiliser Save" where the French button reads Enr.; the draw-mode body said
//   "Freehand" and "Node" where the buttons read Libre and Points; the
//   lookahead-time body said "voir le commutateur Lookahead" where the toggle
//   reads Anticipation. A tip that points at a caption the page does not show
//   is a tip that lies. "Le mode Libre / Le mode Points" also repairs "Points
//   place", which read as a plural subject on a singular verb.
//
//   ON / OFF ARE A FEATURE, NOT A POWER STATE, so the glossary's Activé(e) /
//   Désactivé(e) pair applies rather than Marche / Arrêt, feminine to agree
//   with "l'aide au survol". See the note above .settings-toggle in
//   css/styles.css: 40px no longer covers the widest face (Désactivée renders
//   the button at 61.88px). The ROW still cannot resize — the popover holds at
//   168px and #lang-select and the row's own left edge do not move — but the
//   button does, and the comment that claimed otherwise was corrected.
//   (v1.7.2 pins it to 64px; the button no longer resizes.)
//
//   "Aide" BECAME "Aide au survol". The tip title and the aria-label on this
//   very control already said "aide au survol"; the popover row alone said
//   "Aide". Measured at 65.89px in a nowrap .settings-label — the popover
//   holds at its 168px min-width and nothing inside it moves.
//
//   THE TRAILING PERIODS ON "Enr." AND "Ouv." STAY. The label-in-name rule
//   (WCAG 2.5.3) that dropped them on two other plugins does not apply here:
//   #preset-save and #preset-load carry no aria-label, so the accessible name
//   IS the visible caption and the criterion is satisfied by identity rather
//   than by stem.
//
//   REGISTER: the French follows the ENGLISH MOOD, entry by entry — infinitive
//   where the English is imperative ("Superposer", "Ramener"), indicative where
//   the English describes ("Dose", "Active ou désactive"). That is the pattern
//   the v1.7.0 draft already held to, and it was kept.
//
//   NO COMMITTED TIP GATE. plugins/O-SpectralShaper/tests/ holds
//   ui_preset_menu_check.js, i18n-states.json and ui-stub/ — no
//   ui_tip_render_check.js. Hover help was driven from a scratchpad probe
//   instead: 28/28 anchors opened in both languages, all inside the frame, all
//   28 changed text en -> fr. The gap is reported, not filled here.
//
// An ES module that EXPORTS ONLY. It must never self-execute: a bare top-level
// statement here throws out of module evaluation and takes every later
// initializer on the page with it (pattern_module_toplevel_init_tdz).
// scripts/check-i18n.js assertion 7 enforces that.
//
// SERVED ROOT IS Resources/ui. This plugin has only the one UI tree and
// CMakeLists.txt embeds it directly. THE BINARY-DATA TARGET CARRIES NO
// NAMESPACE ARGUMENT — juce_add_binary_data(O-SpectralShaper_UIResources
// SOURCES ...) takes the default BinaryData namespace and works only because it
// is the only such target in this plugin. This file was added to that EXISTING
// SOURCES list; a second juce_add_binary_data target would collide on the
// BinaryData namespace and break the build in a way that reads like something
// else entirely (critical_dual_binary_data_namespace_collision).
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
// THE ENGLISH WAS MOVED, NOT REWRITTEN. v1.6.2 authored its hover help as a
// SINGLE data-tooltip string in the shape "Label: sentence.", read by a second
// tooltip renderer this version deletes. Every tooltip entry below is that
// string split on its FIRST ": " into the t/b pair the measure-then-pin
// renderer wants, with both halves byte-identical to v1.6.2 either side of the
// separator. ALL TWENTY-ONE SPLIT CLEANLY — there is no hand-split on this
// plugin, and that is measured rather than assumed: the longest surviving title
// is "Previous Preset" at 15 characters, and no body reaches its own colon
// before the separator.
//
// TWENTY-FIVE ANCHORS, TWENTY-ONE STRINGS. Four tips are worn by two controls
// each — Spectrum, Undo, Redo and Draw Mode are the same copy on the attack
// plate and on the sustain plate. Reset is NOT one of them: its two bodies name
// different curves, so it is two keys. Those counts are attributes and unique
// strings parsed out of the DOM, not grep hits for the token `data-tooltip`,
// which also matches the CSS selector and every JS reference.
//
// THREE NEW CONTROLS carry new English copy: `settings`, `lang-select` and
// `tips-toggle`. The first two are the gear popover and the language selector,
// which did not exist before; the third is the hover-help toggle, which did
// exist as a wax-seal "?" in the same header slot and had only a native
// aria-label. Authoring hover-help prose for controls that have none is
// Stage M's job and is NOT done here.
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

    // ── The settings popover (v1.7.0) ───────────────────────────────────────
    // New controls, new copy. The hover-help toggle moves in here from the
    // wax-seal "?" that sat in this exact header slot: one place for the two
    // things that decide what the hover help says and whether it says it at all.
    'settings': {
        en: { t: 'Settings',
              b: 'Choose the language of this interface and whether hover help appears. Both choices are remembered with the session.' },
        fr: { t: 'Réglages',
              b: 'Choisir la langue de cette interface et l’affichage de l’aide au survol. Les deux choix sont conservés avec la session.',
              reviewed: true },
    },
    'lang-select': {
        en: { t: 'Language',
              b: 'The language of this hover help and of the labels on the page. English and French are available; value readouts, preset names and preset category headings stay in English.' },
        fr: { t: 'Langue',
              b: 'La langue de cette aide au survol et des libellés de la page. L’anglais et le français sont disponibles ; les valeurs affichées, les noms de préréglages et les intitulés de catégories restent en anglais.',
              reviewed: true },
    },
    'tips-toggle': {
        en: { t: 'Hover Help',
              b: 'Turns this hover help on and off. With it off, only the gear and this switch keep explaining themselves.' },
        fr: { t: 'Aide au survol',
              b: 'Active ou désactive cette aide au survol. Une fois désactivée, seuls l’engrenage et ce commutateur continuent de s’expliquer.',
              reviewed: true },
    },

    // ── Preset bar ──────────────────────────────────────────────────────────
    'preset-prev': {
        en: { t: 'Previous Preset',
              b: 'Step back through the preset list.' },
        fr: { t: 'Préréglage précédent',
              b: 'Reculer d’un préréglage dans la liste.',
              reviewed: true },
    },
    'preset-name': {
        en: { t: 'Preset Name',
              b: 'The currently loaded preset — click to open the preset menu, grouped by category. Factory presets cannot be overwritten — use Save to store your own.' },
        fr: { t: 'Nom du préréglage',
              b: 'Le préréglage actuellement chargé — cliquer pour ouvrir le menu des préréglages, groupé par catégorie. Les préréglages d’usine ne peuvent pas être écrasés — utiliser le bouton Enr. pour enregistrer les vôtres.',
              reviewed: true },
    },
    'preset-next': {
        en: { t: 'Next Preset',
              b: 'Step forward through the preset list.' },
        fr: { t: 'Préréglage suivant',
              b: 'Avancer d’un préréglage dans la liste.',
              reviewed: true },
    },
    'preset-save': {
        en: { t: 'Save Preset',
              b: 'Store the current settings — including both shaping curves — as a user preset.' },
        fr: { t: 'Enregistrer le préréglage',
              b: 'Enregistrer les réglages actuels — les deux courbes de modelage comprises — comme préréglage utilisateur.',
              reviewed: true },
    },
    'preset-load': {
        en: { t: 'Load Preset',
              b: 'Open a preset file from disk.' },
        fr: { t: 'Ouvrir un préréglage',
              b: 'Ouvrir un fichier de préréglage depuis le disque.',
              reviewed: true },
    },

    // ── The specimen plates ─────────────────────────────────────────────────
    'spectrogram': {
        en: { t: 'Spectrogram',
              b: 'Live frequency analysis, scrolling right to left. Brighter areas are louder; the hotter red-orange tones mark energy the transient detector has flagged as attack.' },
        fr: { t: 'Spectrogramme',
              b: 'Analyse fréquentielle en direct, défilant de droite à gauche. Les zones claires sont plus fortes ; les teintes rouge-orangé les plus chaudes marquent l’énergie que le détecteur de transitoires a signalée comme attaque.',
              reviewed: true },
    },
    'attackCurve': {
        en: { t: 'Attack Curve',
              b: 'Per-band gain applied to detected transients, across 32 logarithmic bands from 20 Hz to Nyquist. Drag up to sharpen a band’s attack, down to soften it. Full deflection is ±12 dB.' },
        fr: { t: 'Courbe d’attaque',
              b: 'Gain par bande appliqué aux transitoires détectés, sur 32 bandes logarithmiques de 20 Hz à Nyquist. Glisser vers le haut pour aiguiser l’attaque d’une bande, vers le bas pour l’adoucir. Le débattement total est de ±12 dB.',
              reviewed: true },
    },
    'sustainCurve': {
        en: { t: 'Sustain Curve',
              b: 'Per-band gain applied to the sustained body of the signal — everything the detector does not flag as transient — across the same 32 bands. Drag up to thicken a band’s tail, down to tighten it. Full deflection is ±12 dB.' },
        fr: { t: 'Courbe de maintien',
              b: 'Gain par bande appliqué au corps tenu du signal — tout ce que le détecteur ne signale pas comme transitoire — sur les mêmes 32 bandes. Glisser vers le haut pour épaissir la traîne d’une bande, vers le bas pour la resserrer. Le débattement total est de ±12 dB.',
              reviewed: true },
    },

    // ── Plate controls ──────────────────────────────────────────────────────
    // Spectrum, Undo, Redo and Draw Mode are one key each, worn by BOTH plates:
    // the copy is identical on the attack plate and on the sustain plate, and it
    // was identical in v1.6.2 too. Reset is not — its two bodies name different
    // curves — so it stays two keys.
    'spectrum': {
        en: { t: 'Spectrum',
              b: 'Overlay the live input spectrum behind the curve, so you can see which bands are actually active while drawing.' },
        fr: { t: 'Spectre',
              b: 'Superposer le spectre d’entrée en direct derrière la courbe, pour voir quelles bandes sont réellement actives pendant le tracé.',
              reviewed: true },
    },
    'undo': {
        en: { t: 'Undo',
              b: 'Step back through curve edits (Ctrl+Z).' },
        fr: { t: 'Annuler',
              b: 'Revenir en arrière dans les modifications de la courbe (Ctrl+Z).',
              reviewed: true },
    },
    'redo': {
        en: { t: 'Redo',
              b: 'Step forward through curve edits (Ctrl+Shift+Z).' },
        fr: { t: 'Rétablir',
              b: 'Avancer dans les modifications de la courbe (Ctrl+Shift+Z).',
              reviewed: true },
    },
    'attackReset': {
        en: { t: 'Reset',
              b: 'Return every band to 0 dB — flat, with no attack shaping.' },
        fr: { t: 'Réinitialiser',
              b: 'Ramener toutes les bandes à 0 dB — plat, sans modelage d’attaque.',
              reviewed: true },
    },
    'sustainReset': {
        en: { t: 'Reset',
              b: 'Return every band to 0 dB — flat, with no sustain shaping.' },
        fr: { t: 'Réinitialiser',
              b: 'Ramener toutes les bandes à 0 dB — plat, sans modelage du maintien.',
              reviewed: true },
    },
    'drawMode': {
        en: { t: 'Draw Mode',
              b: 'Freehand draws a continuous curve as you drag. Node places draggable control points with smooth interpolation between them.' },
        fr: { t: 'Mode de tracé',
              b: 'Le mode Libre trace une courbe continue au glissement. Le mode Points place des points de contrôle déplaçables, avec une interpolation lisse entre eux.',
              reviewed: true },
    },

    // ── The knob sidebar ────────────────────────────────────────────────────
    // Unit SYMBOLS and ranges are carried across unchanged: Hz, dB, ms and %
    // are language-neutral.
    //
    // v1.7.1 CORRECTS THE OTHER HALF OF THIS NOTE. It used to read "English
    // number formatting is retained in both languages (D-03)", and that
    // over-read D-03: the exemption is for the READOUT NODE, which formats a
    // live value, not for prose in a tooltip body. French prose takes the
    // decimal COMMA ("0,1-50 ms") and a U+00A0 between a number and its unit
    // and before a % sign. The readouts in index.html are untouched and still
    // print 10ms, 100%, 0.0dB in both languages.
    'mix': {
        en: { t: 'Mix',
              b: 'Blends the dry input against the spectrally shaped output. 0% is fully dry, 100% fully processed. Default 100%.' },
        fr: { t: 'Mix', sameAsEn: true,
              b: 'Dose l’entrée directe face à la sortie modelée spectralement. À 0 % le signal est entièrement direct, à 100 % entièrement traité. Défaut 100 %.',
              reviewed: true },
    },
    'attackTime': {
        en: { t: 'Attack Time',
              b: 'How quickly the transient detector responds, 0.1–50 ms. Short values catch sharp percussive hits; longer values treat more of each note as attack. Default 10 ms.' },
        fr: { t: 'Temps d’attaque',
              b: 'Vitesse de réaction du détecteur de transitoires, 0,1–50 ms. Les valeurs courtes captent les frappes percussives nettes ; les valeurs longues traitent une plus grande part de chaque note comme attaque. Défaut 10 ms.',
              reviewed: true },
    },
    'sustainTime': {
        en: { t: 'Sustain Time',
              b: 'How long a band keeps being treated as sustain after a transient passes, 10–500 ms. Longer values extend the region the Sustain curve acts on. Default 100 ms.' },
        fr: { t: 'Temps de maintien',
              b: 'Durée pendant laquelle une bande reste traitée comme du maintien après le passage d’un transitoire, 10–500 ms. Les valeurs longues étendent la zone sur laquelle agit la courbe de maintien. Défaut 100 ms.',
              reviewed: true },
    },
    'sensitivity': {
        en: { t: 'Sensitivity',
              b: 'Threshold for transient detection, 0–100%. Higher values flag more material as transient, shifting the balance from the Sustain curve toward the Attack curve. Default 50%.' },
        fr: { t: 'Sensibilité',
              b: 'Seuil de détection des transitoires, 0–100 %. Les valeurs élevées signalent davantage de matière comme transitoire, déplaçant l’équilibre de la courbe de maintien vers la courbe d’attaque. Défaut 50 %.',
              reviewed: true },
    },
    'outputGain': {
        en: { t: 'Output Gain',
              b: 'Final level trim after shaping, −12 to +12 dB. Use it to compensate when heavy boosting or cutting has changed the overall loudness. Default 0 dB.' },
        fr: { t: 'Gain de sortie',
              b: 'Ajustement de niveau final après modelage, −12 à +12 dB. À utiliser pour compenser lorsqu’un fort renforcement ou une forte atténuation a changé le niveau global. Défaut 0 dB.',
              reviewed: true },
    },
    'lookaheadTime': {
        en: { t: 'Lookahead Time',
              b: 'Amount of latency reported to the host when Lookahead is on, 0.1–10 ms. Note that Lookahead does not yet alter the sound — see the Lookahead toggle.' },
        fr: { t: 'Temps d’anticipation',
              b: 'Quantité de latence signalée à l’hôte lorsque l’anticipation est active, 0,1–10 ms. Noter que l’anticipation ne modifie pas encore le son — voir le commutateur Anticipation.',
              reviewed: true },
    },
    'lookahead': {
        en: { t: 'Lookahead',
              b: 'Currently inert. Detection and the shaped signal are delayed by the same amount, so they stay time-aligned and nothing audible changes — it only reports added latency to the host. True lookahead is planned for a future release.' },
        fr: { t: 'Anticipation',
              b: 'Sans effet pour l’instant. La détection et le signal modelé sont retardés de la même durée, ils restent donc alignés dans le temps et rien d’audible ne change — cela signale seulement une latence supplémentaire à l’hôte. La véritable anticipation est prévue pour une version future.',
              reviewed: true },
    },
});

// ============================================================================
// LABELS — the on-page text (v1.7.1, canon v2)
// ============================================================================
//
// I18N above is HOVER-HELP copy: a title and a body rendered into a wrapping
// 240 px tooltip. LABELS is ON-PAGE copy: one string dropped into a cell that
// mostly does not wrap. They are different problems and this table keeps them
// apart on purpose.
//
// ── THE REUSE RULE IS USED SPARINGLY ────────────────────────────────────────
// trLabel() falls back to I18N when a key is absent from this table. Here that
// fallback is taken for NOTHING. The reason is the sidebar: .knob-sidebar is a
// 180 px two-column grid, so a caption cell is ~86 px and a caption that
// outruns it WRAPS, raising its whole grid row and pushing the rows below it
// down. A tooltip title is allowed to grow into a phrase — "Temps d’attaque",
// "Gain de sortie" — and a caption in that cell is not. Reusing would make the
// next tooltip copy edit a silent layout change. The English already reflects
// that judgement: the tip title is "Attack Time" and the caption is "Attack",
// the tip title is "Lookahead Time" and the caption is "LA Time".
//
// ── ENGLISH WAS MOVED, NOT RE-TYPED ────────────────────────────────────────
// Every en below is what index.html carried through v1.6.2, taken from
// scripts/i18n-extract.js's inventory rather than transcribed.
//
// ── FRENCH IS SIZED, NOT SHRUNK ────────────────────────────────────────────
// D-04 forbids an auto-shrink font and a short-variant fallback: exactly ONE
// French string per key, and nothing chooses between variants at runtime.
// Where French could not fit a cell, the CELL was changed — see CHANGELOG
// v1.7.0 for the two geometry rules and what each was measured against.
//
// ALL FRENCH IS MACHINE-DRAFTED, `reviewed: false`.
// ============================================================================

export const LABELS = Object.freeze({

    // ── The settings popover ────────────────────────────────────────────────
    'label.language':  { en: { t: 'Language' },   fr: { t: 'Langue',        reviewed: true } },
    'label.hoverHelp': { en: { t: 'Hover help' }, fr: { t: 'Aide au survol', reviewed: true } },

    // The two faces of the hover-help switch, chosen by an if/else over two
    // literal keys in setTooltipsEnabled — never a ternary inside the setLabel
    // call (check-i18n assertion 13), and never a literal, which would strand
    // an English "On" the moment the language selector fired.
    'ui.on':           { en: { t: 'On' },         fr: { t: 'Activée',       reviewed: true } },
    'ui.off':          { en: { t: 'Off' },        fr: { t: 'Désactivée',    reviewed: true } },

    // ── Preset bar ──────────────────────────────────────────────────────────
    // .preset-action-btn is 9 px uppercase with 0.8 px letter-spacing inside a
    // `justify-content: space-between` header. The two buttons sit at the right
    // of the preset cluster, so a wider caption pushes the cluster LEFT rather
    // than growing the frame — but it still moves .preset-menu's anchor and the
    // header's middle group, so both French forms were measured as rendered
    // rather than read off a font probe (text-transform and letter-spacing are
    // not in getComputedStyle().font).
    // "Enreg." (55.06px) and "Ouvrir" (57.31px) both overran their pins; the
    // standard French UI abbreviations fit at 41.45px and 41.80px.
    'label.save':      { en: { t: 'Save' },       fr: { t: 'Enr.',          reviewed: true } },
    'label.load':      { en: { t: 'Load' },       fr: { t: 'Ouv.',          reviewed: true } },

    // ── The specimen plates ─────────────────────────────────────────────────
    // .spectrogram-label and .curve-label are absolutely positioned overlays
    // with `pointer-events: none`, so they are out of flow: a longer French
    // caption cannot push anything. They are free to be phrases.
    'label.spectrogram':  { en: { t: 'Spectrogram' },   fr: { t: 'Spectrogramme',      reviewed: true } },
    'label.attackCurve':  { en: { t: 'Attack Curve' },  fr: { t: 'Courbe d’attaque',   reviewed: true } },
    'label.sustainCurve': { en: { t: 'Sustain Curve' }, fr: { t: 'Courbe de maintien', reviewed: true } },

    // ── Plate controls ──────────────────────────────────────────────────────
    // .curve-controls is an absolutely positioned flex row pinned `right: 8px`
    // over a 100 px-tall plate, so it grows LEFTWARD across the plate rather
    // than pushing anything — but the row is 5 buttons wide over a plate that
    // is only ~490 px, so its French width was measured, not assumed.
    //
    // "Freehand" and "Node" are the two faces of the draw-mode button. They are
    // MODE NAMES, not value mirrors: `app.curveModes` is UI-local state and
    // there is no draw-mode parameter in createParameterLayout — grep
    // PluginProcessor.cpp for CURVE_MODE and nothing comes back. So they are
    // copy, and they localize.
    'label.spectrum':  { en: { t: 'Spectrum' },   fr: { t: 'Spectre',       reviewed: true } },
    // The width case for keeping "Init." is the termNote on the entry itself,
    // re-measured in v1.7.1 rather than inherited. Short form: Réinit. renders
    // the button at 58.69px against a 53.52px pin and moves two NON-label
    // siblings.
    'label.reset':     { en: { t: 'Reset' },
                         fr: { t: 'Init.', reviewed: true,
                               termNote: 'width: the glossary’s Réinit. renders the button at 58.69px and Réinit at 56.30px against .curve-reset-btn’s 53.52px pin, pushing the NON-label #attack-undo-btn / #attack-redo-btn 5.17px and 2.78px left — a check-ui-labels assertion-7 geometry regression. Réinitialiser is 96.39px. Init. holds the pin at 53.52px.' } },
    // "Main levée" is 84.75px against a 76.41px pin; "Libre" is 52px and reads
    // as the clear opposite of "Points", which is what the pair has to do.
    'ui.freehand':     { en: { t: 'Freehand' },   fr: { t: 'Libre',         reviewed: true } },
    'ui.node':         { en: { t: 'Node' },       fr: { t: 'Points',        reviewed: true } },

    // ── The knob sidebar ────────────────────────────────────────────────────
    // Each caption sits in a ~86 px column of a 180 px two-column grid at 10 px
    // uppercase with 1 px letter-spacing, under a 56 px knob. `Sensitivity`
    // already wraps to two lines in ENGLISH at that width, which is why the
    // wrapping behaviour is authored rather than avoided — see the CHANGELOG
    // for the `.knob-label` height reservation that keeps a French caption from
    // moving the row below it.
    'label.mix':         { en: { t: 'Mix' },         fr: { t: 'Mix',        reviewed: true, sameAsEn: true } },
    'label.attack':      { en: { t: 'Attack' },      fr: { t: 'Attaque',    reviewed: true } },
    'label.sustain':     { en: { t: 'Sustain' },     fr: { t: 'Maintien',   reviewed: true } },
    'label.sensitivity': { en: { t: 'Sensitivity' }, fr: { t: 'Sensibilité', reviewed: true } },
    'label.output':      { en: { t: 'Output' },      fr: { t: 'Sortie',     reviewed: true } },
    // "Tps antic." rather than "Anticip.": the toggle two rows down is
    // "Anticipation", and two adjacent controls reading the same abbreviated
    // word is worse than a slightly clipped-looking one. English draws the same
    // distinction with "LA Time" against "Lookahead".
    'label.laTime':      { en: { t: 'LA Time' },     fr: { t: 'Tps antic.', reviewed: true } },
    'label.lookahead':   { en: { t: 'Lookahead' },   fr: { t: 'Anticipation', reviewed: true } },

    // ── Accessible names ────────────────────────────────────────────────────
    'aria.presets':      { en: { t: 'Presets' },              fr: { t: 'Préréglages',           reviewed: true } },
    'aria.langSelect':   { en: { t: 'Interface language' },   fr: { t: 'Langue de l’interface', reviewed: true } },
    'aria.helpToggle':   { en: { t: 'Toggle hover help' },
                           fr: { t: 'Activer ou désactiver l’aide au survol', reviewed: true } },
});

// ============================================================================
// I18N_EXEMPT — reasoned exclusions, never silence
// ============================================================================
//
// Every visible string the coverage scan finds must be a [data-i18n] element, a
// setLabel() call, or an entry HERE WITH A REASON. A bare skip list would let a
// missed label hide as a deliberate one.
//
// This plugin's list is SHORT, and that is a real finding rather than an
// oversight: it has no AudioParameterChoice dropdown at all — seven parameters,
// six continuous and one bool — so the D-01 choice-mirror class that dominates
// O-Lyrica's list is empty here.
//
// NOT LISTED, and deliberately: the preset-menu CATEGORY HEADINGS (Essentials,
// Drums & Percussion, …) and the preset NAMES inside them. Both are written
// from a variable — `header.textContent = section.category`, `item.textContent
// = name` — and reach the page from C++ getPresetListGrouped(), so the scan
// never sees them as literals and an entry here would be a claim about strings
// this file cannot see. They stay English for the reason O-Orbit's do:
// tests/ui_preset_menu_check.js derives the grouping from PluginProcessor.cpp
// and holds the rendered DOM to it, and a preset name IS its JSON filename
// (OuariconPresetManager.h:283-285), so translating either breaks recall or the
// gate.
// ============================================================================

export const I18N_EXEMPT = [
    ['O-SpectralShaper',
     'the product name in the header — a product name is never translated, and this one is also the plugin’s registered PRODUCT_NAME in CMakeLists.txt'],

    // #preset-name shows the loaded preset. The name IS the JSON filename
    // (OuariconPresetManager.h:283-285), so translating it breaks recall.
    // "Default" is the placeholder the manager overwrites on its first pass,
    // and it is also a factory preset in its own right.
    ['Default', 'a factory preset name — exempt under D-02, because the name IS the JSON filename'],
];

// [selector, key] or [selector, key, wrapperSelector] or
// [selector, key, wrapperSelector, vars].
//
// The selector is the BINDING SITE. The tip ANCHOR is frequently a wrapper —
// .knob-wrapper, .curve-container, .toggle-container — and this page has SIX
// structurally identical .knob-wrapper elements and TWO .curve-container ones.
// A bare class selector would bind all six knob tips to the first knob, because
// document.querySelector returns the FIRST match in document order — precisely
// how O-Octagon's .vunit-group tip nearly landed on the wrong control in
// Stage C. So every anchor is named by the id of the one control it wraps and
// resolved back up with .closest().
//
// 28 bindings over 24 keys: `spectrum`, `undo`, `redo` and `drawMode` are each
// worn by two controls, which is the same reuse v1.6.2 authored by hand.
//
// EVERY ONE OF THESE ELEMENTS EXISTS IN THE MARKUP before applyI18n runs; none
// is built at runtime, so no binding here can silently write onto nothing.
export const TIP_BINDINGS = [
    ['#gear-btn',      'settings'],
    ['#lang-select',   'lang-select'],
    ['#tips-toggle',   'tips-toggle'],

    // Preset bar
    ['#preset-prev',   'preset-prev'],
    ['#preset-name',   'preset-name'],
    ['#preset-next',   'preset-next'],
    ['#preset-save',   'preset-save'],
    ['#preset-load',   'preset-load'],

    // The plates. The anchor is the container; the id'd child is the canvas.
    ['#spectrogram-canvas',  'spectrogram',  '.spectrogram-container'],
    ['#attack-curve-canvas',  'attackCurve',  '.curve-container'],
    ['#sustain-curve-canvas', 'sustainCurve', '.curve-container'],

    // Plate controls — the buttons carry their own tips directly.
    ['#attack-spectrum-btn',  'spectrum'],
    ['#attack-undo-btn',      'undo'],
    ['#attack-redo-btn',      'redo'],
    ['#attack-reset-btn',     'attackReset'],
    ['#attack-mode-toggle',   'drawMode'],
    ['#sustain-spectrum-btn', 'spectrum'],
    ['#sustain-undo-btn',     'undo'],
    ['#sustain-redo-btn',     'redo'],
    ['#sustain-reset-btn',    'sustainReset'],
    ['#sustain-mode-toggle',  'drawMode'],

    // The knob sidebar. Each anchor is the .knob-wrapper reached from the one
    // knob container it holds.
    ['#mix-knob-container',            'mix',           '.knob-wrapper'],
    ['#attack-time-knob-container',    'attackTime',    '.knob-wrapper'],
    ['#sustain-time-knob-container',   'sustainTime',   '.knob-wrapper'],
    ['#sensitivity-knob-container',    'sensitivity',   '.knob-wrapper'],
    ['#output-gain-knob-container',    'outputGain',    '.knob-wrapper'],
    ['#lookahead-time-knob-container', 'lookaheadTime', '.knob-wrapper'],
    ['#lookahead-toggle',              'lookahead',     '.toggle-container'],
];

export function tr(key, lang, vars) {
    const entry = I18N[key];
    if (!entry) { console.warn(`i18n: missing key ${key}`); return { t: key, b: '' }; }
    const s = entry[lang] || entry.en;

    // A var VALUE that is itself an I18N key resolves to that key's localized
    // title; anything else is used literally.
    const resolve = (v) => {
        const nested = I18N[v];
        return nested ? String((nested[lang] || nested.en).t) : String(v);
    };

    const sub = (v) => vars
        ? String(v).replace(/\{(\w+)\}/g, (m, n) => (n in vars ? resolve(vars[n]) : m))
        : String(v);

    return { t: sub(s.t), b: sub(s.b) };
}
