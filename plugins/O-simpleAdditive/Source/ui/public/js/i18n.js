/*
   This file is part of O-simpleAdditive, an Ouaricon Audio plugin.
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
// i18n.js — O-simpleAdditive interface copy, English + French (v1.2.0)
//
// ── v1.1.2: FOCUS LATCH (Stage O item 58, 2026-08-31) ───────────────────────
// No entry in this table changed. The fix is in js/app.js setupTooltips(): a
// pointer click no longer opens hover help (26 of the 41 focusable anchors did
// — 10 through focusin, 16 drawbars through a hover re-open under the pointer),
// keyboard focus still does. This page has no hover-help toggle and therefore
// no aria.helpToggle entry — the item 54 rename does not apply here.
//
// ── v1.1.1: FRENCH QA PASS (Stage N, 2026-08-31) ────────────────────────────
// Every fr entry read against its en and against scripts/i18n-fr-glossary.js.
// Changed: 52 of 84 entries — 33 typography only, 19 with a wording change
// (13 terminology, 5 meaning, 2 grammar/register; scanLfoDepth is in two).
// i18n-fr-lint: 58 findings -> 0 (--strict exit 0): 31 T1, 4 T3, 4 T4, 8 T5,
// 10 G1, 1 F1. sameAsEn: kept 0, translated 0 — this page had no straight copy
// in either direction. termNote exemptions: 0 — every glossary term on this
// page fits, so nothing needed exempting.
// Left as drafted: the rest. reviewed: false throughout — no native speaker yet.
//
// The decisions a later reader needs:
//   - ROOT TERMS FIT HERE, and two of the three widths the v1.1.0 header and
//     styles.css defended were measured again at the shipping frame rather
//     than inherited. VITESSE DU LFO wraps to two lines at 42.20 px — the same
//     max line width as the shipped VITESSE LFO, in a .knob-label that already
//     reserves two lines (min-height 2.2em), so the cell is byte-identical in
//     height. PROFONDEUR DU LFO is 69.36 px in a 58 px cell (shrink-to-fit,
//     overflow visible, symmetric — 5.68 px each side) and RELÂCHEMENT is
//     77.33 px in a 56 px cell (10.67 px each side): both clear their
//     neighbours, and check-ui-labels [5]/[7]/[8]/[8b] are unchanged from the
//     v1.1.0 baseline, still 0 non-label elements moved. The one abbreviation
//     kept for width is DÉCROISS. SPECTRALE (58.72 px on two lines; the root
//     DÉCROISSANCE SPECTRALE is 79.00 px, 21 px wider, next to a VÉL→DÉCROISS.
//     cell that already overhangs 13 px a side) — and the glossary accepts it,
//     so it draws no finding.
//   - THE GLOSSARY CLOSED FOUR LABEL-IN-NAME RELATIONS (WCAG 2.5.3). The knob
//     and combo accessible names are their tooltip TITLES (data-i18n-aria), and
//     the caption is now a substring of the name it labels for LFO Rate
//     (Vitesse du LFO / Vitesse du LFO de balayage), LFO Depth (Profondeur du
//     LFO / Profondeur du LFO de balayage), Bit Depth (Résolution / Résolution)
//     and Release (Relâchement / Relâchement d'amplitude). Three still hold by
//     STEM only, and all three are captions the page pinned as abbreviations
//     before this pass: ENV→BALAY., VÉL→DÉCROISS. and DÉCROISS. SPECTRALE.
//   - DECAY IS TWO WORDS ON THIS PAGE, deliberately. The envelope stage is
//     Déclin (glossary 'decay'); the spectral tilt is Décroissance (glossary
//     'spectral decay'). They are different controls and must not converge.
//   - ONE CONTROL, ONE FRENCH NAME. Scan is Balayage everywhere — the caption,
//     the tip title, the group heading (· → Balayage, was lowercase) and the
//     LFO tip titles. Scan LFO Depth's title said Amplitude du LFO while its
//     caption said Ampleur LFO and the glossary says Profondeur; all three are
//     Profondeur now.
//   - THE FAMILY SENTENCES USE THE FORM O-simpleFM SETTLED at v1.3.1, so the
//     six O-simple* copies converge: "cliquez SUR les touches …" (the draft
//     dropped the preposition) and "… pour entendre une notion." The lesson
//     row caption is Leçons (46.09 px in the 99 px .tour-label pin), which the
//     glossary accepts for "Lesson Presets"; the root Préréglages de leçon
//     needs two lines in that box and the shipped Préréglages was not a
//     glossary rendering at all.
//   - THE LOANWORDS THAT STAY: morpher / morphage (the English verb is "morph"
//     and the French audio press writes it), and the four Frame B faces plus
//     Off, which are AudioParameterChoice strings and English on screen in
//     both languages (I18N_EXEMPT, D-01) — so the bodies name them in English.
//   - REGISTER: vous, imperative. The gear body was the page's only infinitive
//     ("Choisir la langue") and is now "Choisissez la langue".
//
// UI ROOT IS Source/ui/public. There is no second UI root in this plugin and no
// Resources/ui staging directory. This file is the seventh SOURCES entry in
// juce_add_binary_data(O-simpleAdditive_UIResources).
//
// An ES module that EXPORTS ONLY. It must never self-execute: a bare top-level
// statement here throws out of module evaluation and takes every later
// initializer on the page with it (pattern_module_toplevel_init_tdz).
// scripts/check-i18n.js assertion 7 enforces it — which is also why the sixteen
// per-partial entries below are WRITTEN OUT rather than generated by a loop.
// A generator would have to be a top-level function, and assertion 7 rejects
// one; wrapping it in an IIFE inside the export initializer would hide sixteen
// pieces of reviewable prose behind a template. A translator reads sentences.
//
// FILENAME: no hyphen. juce_add_binary_data STRIPS hyphens rather than
// converting them to underscores, so a second file named i18n-fr.js would have
// to be reached as the symbol i18nfr_js (critical_binary_data_strips_hyphens).
// One combined file for both languages sidesteps the question entirely. The
// symbol this file becomes is BinaryData::i18n_js, which does not collide with
// BinaryData::index_js (js/juce/index.js) already served by the same editor.
//
// COPY IS textContent ON EVERY PATH — never innerHTML. v1.0.7's tooltip
// renderer built its tip with `tip.innerHTML = '<span class="tip-title">' + ...`;
// v1.1.0 builds it with createElement + textContent, because the tip text is now
// table-sourced and localized rather than a fixed literal. check-i18n
// assertion 9 rejects any innerHTML reference here and any string literal
// containing `<`.
//
// THE ENGLISH WAS MOVED, NOT REWRITTEN. Every `en` entry below was extracted
// mechanically from v1.0.7's `TIPS` / `LESSONS` tables in js/app.js and from
// index.html, and compared back to the source byte-for-byte, rather than
// re-typed. HTML entities are decoded to the characters they named
// (&#183; -> ·, &#8594; -> →, &#8202; -> a \u200a escape, NOT a plain space)
// because setAttribute and textContent do not decode entities.
//
// TWO DELIBERATE ENGLISH CHANGES, both recorded in the CHANGELOG:
//   1. `partial2` / `partial3` read "The 2nd harmonic" / "The 3rd harmonic".
//      v1.0.7's generator built the ordinal as `${k}th` unconditionally and so
//      shipped "The 2th harmonic" and "The 3th harmonic". That is a bug in the
//      shipped English, fixed here rather than translated faithfully.
//   2. The six `lesson*` tips and the six `label.caption*` captions are now two
//      separate table entries. v1.0.7 derived the tip from the caption by
//      splitting on " — " at load time. A derived string cannot be reviewed by
//      a translator, and a table IS the single source of truth once there are
//      two languages, so the derivation is gone and both halves are authored.
//
// KEYS ARE THE PARAMETER ID where the anchor is a parameter cell, and a
// `label.*` / `aria.*` slug otherwise. The parameter cells are addressed by the
// data-param attribute added in v1.1.0; through v1.0.7 they carried the tip KEY
// in their own data-tip attribute, and a `.knob-cell` selector would have
// matched the FIRST of fifteen — the failure canon §1 names and the one
// O-Octagon's .vunit-group tip hit for real in Stage C.
//
// LABELS NEVER REUSE A TOOLTIP KEY HERE. trLabel() falls back to I18N and three
// of this page's captions do happen to equal their tip title today ("Scan",
// "Spectral Decay", "Bit Depth") — but ten others do not ("LFO Rate" vs "Scan
// LFO Rate", "Level" vs "Output Level"), and a rule that holds for three of
// thirteen is a rule nobody can apply. A caption and a tip title also diverge
// the moment either is edited, which would make a tooltip copy edit a silent
// geometry change to a control. The ONE place the fallback is used on purpose
// is `data-i18n-aria` on the knobs and combos: an accessible name IS the
// control's name, so it reads the tooltip title by design.
//
// ALL FRENCH IS MACHINE-DRAFTED AND FLAGGED `reviewed: false`. No native
// speaker has read it. `node scripts/check-i18n.js` prints the worklist.
// ============================================================================

export const LANGUAGES = ['en', 'fr'];

export const I18N = Object.freeze({

    // ── The settings popover (v1.1.0) ───────────────────────────────────────
    // The gear holds the language selector and, since v1.2.0, the hover-help
    // switch. Through v1.1.2 this plugin's help layer was always on and the
    // panel held the selector alone; the switch is the one O-simpleGrain
    // carries, and its copy is that plugin's, verbatim.
    'gear-btn': {
        en: { t: 'Settings',
              b: 'Choose the language of the interface and switch this hover help off or on. The language is remembered with the session; the help switch is remembered on this computer.' },
        fr: { t: 'Réglages',
              b: "Choisissez la langue de l’interface et activez ou désactivez cette aide au survol. La langue est conservée avec la session ; le réglage de l’aide est conservé sur cet ordinateur.",
              reviewed: true },
    },

    // Written to say what is TRUE of canon v2, in both languages: the labels DO
    // change, and the halves that stay English are named rather than left to be
    // discovered — value readouts (D-03) and the Frame B / Bit Depth menu
    // entries, which come from the C++ parameter and are the host automation
    // contract (D-01).
    'lang-select': {
        en: { t: 'Language',
              b: 'The language of the labels on this page and of this hover help. English and French are available; value readouts and the two drop-down menus stay in English.' },
        fr: { t: 'Langue',
              b: "La langue des libellés de cette page et de cette aide au survol. L’anglais et le français sont disponibles ; les valeurs affichées et les deux menus déroulants restent en anglais.",
              reviewed: true },
    },
    'help-toggle': {
        en: { t: 'Hover help',
              b: 'Turns these hover explanations off or back on. The switch is remembered on this computer rather than in the session, so it follows you from one project to the next.' },
        fr: { t: "Aide au survol",
              b: "Active ou désactive ces explications au survol. Le réglage est conservé sur cet ordinateur et non dans la session : il vous suit d’un projet à l’autre.",
              reviewed: true },
    },

    // ── The drawbar bay ─────────────────────────────────────────────────────
    drawbars: {
        en: { t: 'Harmonic Drawbars',
              b: "Each bar sets one overtone's level — together they ARE the additive spectrum. The brass shows what you set; the green glow shows what's actually sounding (after morph & spectral decay)." },
        fr: { t: 'Tirettes harmoniques',
              b: "Chaque tirette règle le niveau d’un harmonique — ensemble, elles SONT le spectre additif. Le laiton montre ce que vous réglez ; la lueur verte montre ce qui sonne réellement (après morphage et décroissance spectrale).",
              reviewed: true },
    },

    // ── Morph · Wavetable ───────────────────────────────────────────────────
    frameBSource: {
        en: { t: 'Frame B Source',
              b: 'The target spectrum the Scan morphs toward (Sine, Saw, Square, or Odd). Scan at 0% = your drawbars; 100% = this shape.' },
        fr: { t: 'Source de la trame B',
              b: "Le spectre cible vers lequel le balayage morphe (Sine, Saw, Square ou Odd). Balayage à 0 % = vos tirettes ; à 100 % = cette forme.",
              reviewed: true },
    },
    scanPosition: {
        en: { t: 'Scan',
              b: 'Morphs the spectrum from your drawbars (0%) toward Frame B (100%). Watch the waveform change shape as you scan.' },
        fr: { t: 'Balayage',
              b: "Morphe le spectre depuis vos tirettes (0 %) vers la trame B (100 %). Regardez la forme d’onde changer pendant le balayage.",
              reviewed: true },
    },
    scanLfoRate: {
        en: { t: 'Scan LFO Rate',
              b: 'Speed of the sine LFO that sweeps Scan automatically. One global LFO — all held notes morph together, in phase.' },
        fr: { t: 'Vitesse du LFO de balayage',
              b: 'Vitesse du LFO sinusoïdal qui fait varier automatiquement le balayage. Un seul LFO global — toutes les notes tenues morphent ensemble, en phase.',
              reviewed: true },
    },
    scanLfoDepth: {
        en: { t: 'Scan LFO Depth',
              b: 'How far the LFO sweeps Scan around its set position. 0% = no automatic morph.' },
        fr: { t: 'Profondeur du LFO de balayage',
              b: "De combien le LFO fait varier le balayage autour de la position réglée. 0 % = aucun morphage automatique.",
              reviewed: true },
    },
    scanEnvAmount: {
        en: { t: 'Env → Scan',
              b: 'How much the Modulation Envelope pushes Scan over each note (bipolar −/+). Makes the timbre evolve after the key is struck.' },
        fr: { t: 'Env → Balayage',
              b: "De combien l’enveloppe de modulation pousse le balayage sur chaque note (bipolaire −/+). Fait évoluer le timbre après la frappe.",
              reviewed: true },
    },

    // ── Spectral shaping ────────────────────────────────────────────────────
    spectralDecay: {
        en: { t: 'Spectral Decay',
              b: 'Over each note, makes higher partials fade faster than lower ones — the tone darkens as it rings, like a plucked string. 0% = steady balance.' },
        fr: { t: 'Décroissance spectrale',
              b: "Sur chaque note, fait décroître les partiels aigus plus vite que les graves — le timbre s’assombrit en résonnant, comme une corde pincée. 0 % = équilibre stable.",
              reviewed: true },
    },
    bitDepth: {
        en: { t: 'Bit Depth',
              b: "Quantizes the output to N bits of amplitude resolution for early-digital grit. 'Off' = clean; fewer bits = more crunch." },
        fr: { t: 'Résolution',
              b: "Quantifie la sortie sur N bits de résolution d’amplitude, pour un grain numérique d’époque. « Off » = propre ; moins de bits = plus de grain.",
              reviewed: true },
    },
    velToDecay: {
        en: { t: 'Velocity → Decay',
              b: 'Lets how hard you play add to Spectral Decay. (Velocity always sets loudness; this adds timbral response — harder = darker decay.)' },
        fr: { t: 'Vélocité → Décroissance',
              b: "Laisse la force de jeu s’ajouter à la décroissance spectrale. (La vélocité règle toujours le volume ; ceci ajoute une réponse de timbre — plus fort = décroissance plus sombre.)",
              reviewed: true },
    },

    // ── Amplitude envelope ──────────────────────────────────────────────────
    ampAttack: {
        en: { t: 'Amp Attack',  b: 'Time for loudness to rise after note-on.' },
        fr: { t: 'Attaque d’amplitude', b: "Temps de montée du volume après l’enfoncement de la touche.",
              reviewed: true },
    },
    ampDecay: {
        en: { t: 'Amp Decay',  b: 'Time for loudness to fall from peak to the sustain level.' },
        fr: { t: 'Déclin d’amplitude', b: 'Temps de chute du volume, du sommet vers le niveau de maintien.',
              reviewed: true },
    },
    ampSustain: {
        en: { t: 'Amp Sustain',  b: 'Held loudness while the key stays down.' },
        fr: { t: 'Maintien d’amplitude', b: 'Volume tenu tant que la touche reste enfoncée.',
              reviewed: true },
    },
    ampRelease: {
        en: { t: 'Amp Release',
              b: 'Time for loudness to fade after the key is released — also how long the voice rings out.' },
        fr: { t: 'Relâchement d’amplitude',
              b: 'Temps de disparition du volume après le relâchement de la touche — et donc durée de résonance de la voix.',
              reviewed: true },
    },

    // ── Modulation envelope ─────────────────────────────────────────────────
    modAttack: {
        en: { t: 'Mod Attack',
              b: 'Attack of the modulation envelope, which drives Scan via Env → Scan.' },
        fr: { t: 'Attaque mod.',
              b: "Attaque de l’enveloppe de modulation, qui pilote le balayage via Env → Balayage.",
              reviewed: true },
    },
    modDecay: {
        en: { t: 'Mod Decay',  b: 'Decay of the modulation envelope toward its sustain level.' },
        fr: { t: 'Déclin mod.', b: "Déclin de l’enveloppe de modulation vers son niveau de maintien.",
              reviewed: true },
    },
    modSustain: {
        en: { t: 'Mod Sustain',  b: 'Held level of the modulation envelope while the key is down.' },
        fr: { t: 'Maintien mod.', b: "Niveau tenu de l’enveloppe de modulation tant que la touche est enfoncée.",
              reviewed: true },
    },
    modRelease: {
        en: { t: 'Mod Release',  b: 'Release of the modulation envelope after key-up.' },
        fr: { t: 'Relâchement mod.', b: "Relâchement de l’enveloppe de modulation après le relâchement de la touche.",
              reviewed: true },
    },

    // ── Output ──────────────────────────────────────────────────────────────
    outputLevel: {
        en: { t: 'Output Level',  b: 'Master output trim in decibels.' },
        fr: { t: 'Niveau de sortie', b: 'Réglage général de la sortie, en décibels.',
              reviewed: true },
    },

    // ── Lesson-preset tooltips ──────────────────────────────────────────────
    // Through v1.0.7 these six were DERIVED at load time by splitting the tour
    // caption on its first " — ". They are authored now: a derived string is
    // invisible to the translator reviewing this file, and once there are two
    // languages the table is the single source of truth rather than the
    // caption. The captions themselves are LABELS entries below.
    lessonSine: {
        en: { t: 'Pure Sine',
              b: 'only the fundamental (H1). One drawbar, one sine: the atom of additive synthesis.' },
        fr: { t: 'Sinus pur',
              b: "seulement la fondamentale (H1). Une tirette, un sinus : l’atome de la synthèse additive.",
              reviewed: true },
    },
    lessonSaw: {
        en: { t: 'Sawtooth',
              b: 'every harmonic at 1/k. All overtones falling by 1/k → a bright, buzzy ramp.' },
        fr: { t: 'Dent de scie',
              b: 'tous les harmoniques à 1/k. Tous les partiels décroissant en 1/k → une rampe brillante et bourdonnante.',
              reviewed: true },
    },
    lessonSquare: {
        en: { t: 'Square',
              b: 'odd harmonics only, at 1/k. Dropping the even partials gives the hollow, reedy tone.' },
        fr: { t: 'Carrée',
              b: 'harmoniques impairs seulement, à 1/k. Retirer les partiels pairs donne le timbre creux et anché.',
              reviewed: true },
    },
    lessonOrgan: {
        en: { t: 'Organ',
              b: 'a Hammond-style drawbar registration: a few low harmonics, instant attack, full sustain.' },
        fr: { t: 'Orgue',
              b: "un registre de tirettes à la Hammond : quelques harmoniques graves, attaque immédiate, maintien plein.",
              reviewed: true },
    },
    lessonMorph: {
        en: { t: 'Morph Pad',
              b: 'the scan LFO slowly morphs your drawbars toward a square; long envelopes make it breathe.' },
        fr: { t: 'Nappe morphée',
              b: 'le LFO de balayage morphe lentement vos tirettes vers une carrée ; de longues enveloppes la font respirer.',
              reviewed: true },
    },
    lessonLofi: {
        en: { t: 'Lo-Fi Bells',
              b: 'a spread bell spectrum + spectral-decay tilt + 8-bit quantization for digital grit.' },
        fr: { t: 'Cloches lo-fi',
              b: 'un spectre de cloche étalé + une pente de décroissance spectrale + une quantification 8 bits pour le grain numérique.',
              reviewed: true },
    },

    // ── The sixteen partials ────────────────────────────────────────────────
    // Written out, not generated: assertion 7 forbids a top-level generator in
    // this file, and hiding sixteen pieces of reviewable prose inside a template
    // would defeat the review this table exists for.
    //
    // The English is v1.0.7's harmonicTip() output verbatim, EXCEPT that
    // partial2 and partial3 read "2nd" and "3rd". v1.0.7 built the ordinal as
    // `${k}th` with no special-casing and shipped "The 2th harmonic" and "The
    // 3th harmonic". That is a bug in the English, fixed rather than translated.
    //
    // French avoids the ordinal entirely — "le 2e harmonique" is correct but the
    // elided "l'harmonique de rang 2" reads better at every k and needs no
    // inflection, which is what contract §6 asks for.
    partial1: {
        en: { t: 'Partial 1 · Fundamental',
              b: "The fundamental — the pitch you hear. On its own it's a pure sine; it's the 1st harmonic of the overtone series." },
        fr: { t: 'Partiel 1 · Fondamentale',
              b: "La fondamentale — la hauteur que vous entendez. Seule, c’est un sinus pur ; c’est le premier harmonique de la série.",
              reviewed: true },
    },
    partial2: {
        en: { t: 'Partial 2 · even harmonic',
              b: 'The 2nd harmonic — 2× the fundamental frequency. Even harmonics reinforce octave-ish color and add body/warmth.' },
        fr: { t: 'Partiel 2 · harmonique pair',
              b: "L’harmonique de rang 2 — 2× la fréquence fondamentale. Les harmoniques pairs renforcent la couleur d’octave et ajoutent du corps et de la chaleur.",
              reviewed: true },
    },
    partial3: {
        en: { t: 'Partial 3 · odd harmonic',
              b: 'The 3rd harmonic — 3× the fundamental frequency. Odd harmonics give hollow, reedy color — a square wave is built from these alone.' },
        fr: { t: 'Partiel 3 · harmonique impair',
              b: "L’harmonique de rang 3 — 3× la fréquence fondamentale. Les harmoniques impairs donnent une couleur creuse et anchée — une onde carrée n’est faite que de ceux-là.",
              reviewed: true },
    },
    partial4: {
        en: { t: 'Partial 4 · even harmonic',
              b: 'The 4th harmonic — 4× the fundamental frequency. Even harmonics reinforce octave-ish color and add body/warmth.' },
        fr: { t: 'Partiel 4 · harmonique pair',
              b: "L’harmonique de rang 4 — 4× la fréquence fondamentale. Les harmoniques pairs renforcent la couleur d’octave et ajoutent du corps et de la chaleur.",
              reviewed: true },
    },
    partial5: {
        en: { t: 'Partial 5 · odd harmonic',
              b: 'The 5th harmonic — 5× the fundamental frequency. Odd harmonics give hollow, reedy color — a square wave is built from these alone.' },
        fr: { t: 'Partiel 5 · harmonique impair',
              b: "L’harmonique de rang 5 — 5× la fréquence fondamentale. Les harmoniques impairs donnent une couleur creuse et anchée — une onde carrée n’est faite que de ceux-là.",
              reviewed: true },
    },
    partial6: {
        en: { t: 'Partial 6 · even harmonic',
              b: 'The 6th harmonic — 6× the fundamental frequency. Even harmonics reinforce octave-ish color and add body/warmth.' },
        fr: { t: 'Partiel 6 · harmonique pair',
              b: "L’harmonique de rang 6 — 6× la fréquence fondamentale. Les harmoniques pairs renforcent la couleur d’octave et ajoutent du corps et de la chaleur.",
              reviewed: true },
    },
    partial7: {
        en: { t: 'Partial 7 · odd harmonic',
              b: 'The 7th harmonic — 7× the fundamental frequency. Odd harmonics give hollow, reedy color — a square wave is built from these alone.' },
        fr: { t: 'Partiel 7 · harmonique impair',
              b: "L’harmonique de rang 7 — 7× la fréquence fondamentale. Les harmoniques impairs donnent une couleur creuse et anchée — une onde carrée n’est faite que de ceux-là.",
              reviewed: true },
    },
    partial8: {
        en: { t: 'Partial 8 · even harmonic',
              b: 'The 8th harmonic — 8× the fundamental frequency. Even harmonics reinforce octave-ish color and add body/warmth.' },
        fr: { t: 'Partiel 8 · harmonique pair',
              b: "L’harmonique de rang 8 — 8× la fréquence fondamentale. Les harmoniques pairs renforcent la couleur d’octave et ajoutent du corps et de la chaleur.",
              reviewed: true },
    },
    partial9: {
        en: { t: 'Partial 9 · odd harmonic',
              b: 'The 9th harmonic — 9× the fundamental frequency. Odd harmonics give hollow, reedy color — a square wave is built from these alone.' },
        fr: { t: 'Partiel 9 · harmonique impair',
              b: "L’harmonique de rang 9 — 9× la fréquence fondamentale. Les harmoniques impairs donnent une couleur creuse et anchée — une onde carrée n’est faite que de ceux-là.",
              reviewed: true },
    },
    partial10: {
        en: { t: 'Partial 10 · even harmonic',
              b: 'The 10th harmonic — 10× the fundamental frequency. Even harmonics reinforce octave-ish color and add body/warmth.' },
        fr: { t: 'Partiel 10 · harmonique pair',
              b: "L’harmonique de rang 10 — 10× la fréquence fondamentale. Les harmoniques pairs renforcent la couleur d’octave et ajoutent du corps et de la chaleur.",
              reviewed: true },
    },
    partial11: {
        en: { t: 'Partial 11 · odd harmonic',
              b: 'The 11th harmonic — 11× the fundamental frequency. Odd harmonics give hollow, reedy color — a square wave is built from these alone.' },
        fr: { t: 'Partiel 11 · harmonique impair',
              b: "L’harmonique de rang 11 — 11× la fréquence fondamentale. Les harmoniques impairs donnent une couleur creuse et anchée — une onde carrée n’est faite que de ceux-là.",
              reviewed: true },
    },
    partial12: {
        en: { t: 'Partial 12 · even harmonic',
              b: 'The 12th harmonic — 12× the fundamental frequency. Even harmonics reinforce octave-ish color and add body/warmth.' },
        fr: { t: 'Partiel 12 · harmonique pair',
              b: "L’harmonique de rang 12 — 12× la fréquence fondamentale. Les harmoniques pairs renforcent la couleur d’octave et ajoutent du corps et de la chaleur.",
              reviewed: true },
    },
    partial13: {
        en: { t: 'Partial 13 · odd harmonic',
              b: 'The 13th harmonic — 13× the fundamental frequency. Odd harmonics give hollow, reedy color — a square wave is built from these alone.' },
        fr: { t: 'Partiel 13 · harmonique impair',
              b: "L’harmonique de rang 13 — 13× la fréquence fondamentale. Les harmoniques impairs donnent une couleur creuse et anchée — une onde carrée n’est faite que de ceux-là.",
              reviewed: true },
    },
    partial14: {
        en: { t: 'Partial 14 · even harmonic',
              b: 'The 14th harmonic — 14× the fundamental frequency. Even harmonics reinforce octave-ish color and add body/warmth.' },
        fr: { t: 'Partiel 14 · harmonique pair',
              b: "L’harmonique de rang 14 — 14× la fréquence fondamentale. Les harmoniques pairs renforcent la couleur d’octave et ajoutent du corps et de la chaleur.",
              reviewed: true },
    },
    partial15: {
        en: { t: 'Partial 15 · odd harmonic',
              b: 'The 15th harmonic — 15× the fundamental frequency. Odd harmonics give hollow, reedy color — a square wave is built from these alone.' },
        fr: { t: 'Partiel 15 · harmonique impair',
              b: "L’harmonique de rang 15 — 15× la fréquence fondamentale. Les harmoniques impairs donnent une couleur creuse et anchée — une onde carrée n’est faite que de ceux-là.",
              reviewed: true },
    },
    partial16: {
        en: { t: 'Partial 16 · even harmonic',
              b: 'The 16th harmonic — 16× the fundamental frequency. Even harmonics reinforce octave-ish color and add body/warmth.' },
        fr: { t: 'Partiel 16 · harmonique pair',
              b: "L’harmonique de rang 16 — 16× la fréquence fondamentale. Les harmoniques pairs renforcent la couleur d’octave et ajoutent du corps et de la chaleur.",
              reviewed: true },
    },
});

// ============================================================================
// LABELS — the page's own captions, v1.1.0
//
// Separate from I18N because a tooltip entry is a {title, body} PAIR and a
// label is one string.
//
// THE REUSE RULE, as applied on this page: NO caption reuses a tooltip key.
// The header comment above records why — three of thirteen captions match their
// tip title and ten do not, so reuse would be a rule nobody could apply, and a
// tooltip copy edit would become a silent geometry change to a control.
//
// The `aria.*` keys are the exception the fallback exists for: they are read by
// data-i18n-aria on the knobs and combos, where the accessible name IS the
// control's name and reading the tooltip title is the point.
//
// ALL FRENCH IS MACHINE-DRAFTED, `reviewed: false`.
// ============================================================================

export const LABELS = Object.freeze({

    // ── Header ──────────────────────────────────────────────────────────────
    // The product name itself is NOT here — see I18N_EXEMPT. Only the strapline
    // under it is copy.
    'label.subtitle': {
        en: { t: 'Additive · Wavetable-Scan Synthesizer · A Field Guide' },
        fr: { t: 'Synthétiseur additif · balayage de table d’ondes · un guide de terrain',
              reviewed: true },
    },

    // ── Drawbar bay ─────────────────────────────────────────────────────────
    'label.drawbarTitle': {
        en: { t: 'Harmonic Drawbars' },
        fr: { t: 'Tirettes harmoniques', reviewed: true },
    },
    'label.drawbarHint': {
        en: { t: "each bar is one overtone · the row reads as the live spectrum (glow = what's sounding)" },
        fr: { t: 'chaque tirette est un harmonique · la rangée se lit comme le spectre en direct (la lueur = ce qui sonne)',
              reviewed: true },
    },

    // ── Oscilloscope ────────────────────────────────────────────────────────
    // "Waveform ·" keeps its trailing separator: the fleuron belongs to the
    // caption, not to the hint span beside it, and moving it would change where
    // the two boxes meet.
    'label.waveform': {
        en: { t: 'Waveform ·' },
        fr: { t: 'Forme d’onde ·', reviewed: true },
    },
    'label.waveformHint': {
        en: { t: 'the summed single-cycle shape (morphs as you scan)' },
        fr: { t: 'la forme sommée d’un seul cycle (elle morphe pendant le balayage)', reviewed: true },
    },

    // ── Group headings ──────────────────────────────────────────────────────
    'label.groupMorph': {
        en: { t: 'Morph · Wavetable' },
        fr: { t: 'Morphage · table d’ondes', reviewed: true },
    },
    'label.groupSpectral': {
        en: { t: 'Spectral Shaping' },
        fr: { t: 'Façonnage spectral', reviewed: true },
    },
    'label.groupAmpEnv': {
        en: { t: 'Amplitude Envelope' },
        fr: { t: 'Enveloppe d’amplitude', reviewed: true },
    },
    'label.groupModEnv': {
        en: { t: 'Modulation Envelope · → Scan' },
        fr: { t: 'Enveloppe de modulation · → Balayage', reviewed: true },
    },
    'label.groupOutput': {
        en: { t: 'Output' },
        fr: { t: 'Sortie', reviewed: true },
    },

    // ── Knob and combo captions ─────────────────────────────────────────────
    // These sit in a 56-58px cell at 9.5px uppercase and wrap freely; the
    // French is chosen to stay within the two lines the English already needs
    // for "SPECTRAL DECAY", and .knob-label reserves that height in BOTH
    // languages so a one-line/two-line difference cannot move the readout below
    // it. See the v1.1.0 CHANGELOG for the measured before/after.
    //
    // v1.1.1 (Stage N), measured again at the shipping frame: VITESSE DU LFO
    // wraps VITESSE / DU LFO at the same 42.20px max line width the shipped
    // VITESSE LFO had, so that cell is unchanged; PROFONDEUR DU LFO is 69.36px
    // and RELÂCHEMENT 77.33px, both shrink-to-fit with overflow visible and
    // both clearing their neighbours (check-ui-labels [5]/[8]/[8b] unchanged,
    // 0 non-label elements moved). DÉCROISS. SPECTRALE stays abbreviated: the
    // root is 79.00px, 21px wider, beside a VÉL→DÉCROISS. cell that already
    // overhangs 13px a side — and the glossary accepts the abbreviation.
    'label.frameB': {
        en: { t: 'Frame B' },  fr: { t: 'Trame B', reviewed: true },
    },
    'label.scan': {
        en: { t: 'Scan' },     fr: { t: 'Balayage', reviewed: true },
    },
    'label.lfoRate': {
        en: { t: 'LFO Rate' }, fr: { t: 'Vitesse du LFO', reviewed: true },
    },
    'label.lfoDepth': {
        en: { t: 'LFO Depth' }, fr: { t: 'Profondeur du LFO', reviewed: true },
    },
    // The arrow is a glyph, not copy, and it is kept on both sides so the two
    // strings have the same shape.
    'label.envScan': {
        en: { t: 'Env→Scan' }, fr: { t: 'Env→Balay.', reviewed: true },
    },
    'label.spectralDecay': {
        en: { t: 'Spectral Decay' }, fr: { t: 'Décroiss. spectrale', reviewed: true },
    },
    'label.bitDepth': {
        en: { t: 'Bit Depth' }, fr: { t: 'Résolution', reviewed: true },
    },
    'label.velDecay': {
        en: { t: 'Vel→Decay' }, fr: { t: 'Vél→Décroiss.', reviewed: true },
    },
    // The four ADSR captions are used TWICE each — once in the amplitude
    // envelope group and once in the modulation envelope group. One key, two
    // elements: querySelectorAll drives the sweep, so a repeated key is normal
    // and two keys holding the same word would be two places for it to drift.
    'label.attack': {
        en: { t: 'Attack' },  fr: { t: 'Attaque', reviewed: true },
    },
    'label.decay': {
        en: { t: 'Decay' },   fr: { t: 'Déclin', reviewed: true },
    },
    'label.sustain': {
        en: { t: 'Sustain' }, fr: { t: 'Maintien', reviewed: true },
    },
    'label.release': {
        en: { t: 'Release' }, fr: { t: 'Relâchement', reviewed: true },
    },
    'label.level': {
        en: { t: 'Level' },   fr: { t: 'Niveau', reviewed: true },
    },

    // ── Lesson preset tour ──────────────────────────────────────────────────
    // "Préréglages pédagogiques" is the faithful rendering and was the v1.1.0
    // first draft. It measures 179.1px against the English's 98.8px in this
    // 10px uppercase row, and .tour-label is the only thing before
    // .tour-buttons in a flex row — so it pushed the whole button group 80.3px
    // right. Pinning .tour-label to the French instead would leave an 81px hole
    // beside the English caption, changing a shipping layout for a language
    // nobody has selected. v1.1.0 shipped the bare "Préréglages" (83.8px) and
    // flagged the alternative for review.
    // v1.1.1 (Stage N): "Leçons" — 46.09px, well inside the 99px pin, and one
    // of the two renderings scripts/i18n-fr-glossary.js accepts for "Lesson
    // Presets" (the root "Préréglages de leçon" needs two lines in this box;
    // "Préréglages" alone was not a glossary rendering at all). O-simpleFM
    // settled on the same word at v1.3.1 — the family's six copies converge.
    // The dropped "lesson" still survives in every button's own hover help.
    'label.tourLabel': {
        en: { t: 'Lesson Presets' },
        fr: { t: 'Leçons', reviewed: true },
    },

    // The six button captions. These are NOT exempt under D-02: D-02 protects a
    // preset name that IS a JSON filename, and these six are C++ APVTS
    // snapshots applied by name through applyFactoryPreset. The English name
    // travels in the button's own data-preset attribute, which is never
    // localized, so the caption is free to be a caption.
    'label.lessonSine':   { en: { t: 'Pure Sine' },   fr: { t: 'Sinus pur', reviewed: true } },
    'label.lessonSaw':    { en: { t: 'Sawtooth' },    fr: { t: 'Dent de scie', reviewed: true } },
    'label.lessonSquare': { en: { t: 'Square' },      fr: { t: 'Carrée', reviewed: true } },
    'label.lessonOrgan':  { en: { t: 'Organ' },       fr: { t: 'Orgue', reviewed: true } },
    'label.lessonMorph':  { en: { t: 'Morph Pad' },   fr: { t: 'Nappe morphée', reviewed: true } },
    'label.lessonLofi':   { en: { t: 'Lo-Fi Bells' }, fr: { t: 'Cloches lo-fi', reviewed: true } },

    // The tour caption, written from script through setLabel() on every lesson
    // click — so the element becomes a [data-i18n] element and the language
    // sweep owns it from then on. Seven separate keys and seven separate
    // setLabel() call sites reached by a lookup, never one call with a ternary
    // in its argument (check-i18n assertion 13).
    'label.captionDefault': {
        en: { t: 'Hover any control for an explanation · pick a lesson to hear a concept.' },
        fr: { t: 'Survolez un réglage pour une explication · choisissez une leçon pour entendre une notion.',
              reviewed: true },
    },
    'label.captionSine': {
        en: { t: 'Pure Sine — only the fundamental (H1). One drawbar, one sine: the atom of additive synthesis.' },
        fr: { t: "Sinus pur — seulement la fondamentale (H1). Une tirette, un sinus : l’atome de la synthèse additive.",
              reviewed: true },
    },
    'label.captionSaw': {
        en: { t: 'Sawtooth — every harmonic at 1/k. All overtones falling by 1/k → a bright, buzzy ramp.' },
        fr: { t: 'Dent de scie — tous les harmoniques à 1/k. Tous les partiels décroissant en 1/k → une rampe brillante et bourdonnante.',
              reviewed: true },
    },
    'label.captionSquare': {
        en: { t: 'Square — odd harmonics only, at 1/k. Dropping the even partials gives the hollow, reedy tone.' },
        fr: { t: 'Carrée — harmoniques impairs seulement, à 1/k. Retirer les partiels pairs donne le timbre creux et anché.',
              reviewed: true },
    },
    'label.captionOrgan': {
        en: { t: 'Organ — a Hammond-style drawbar registration: a few low harmonics, instant attack, full sustain.' },
        fr: { t: 'Orgue — un registre de tirettes à la Hammond : quelques harmoniques graves, attaque immédiate, maintien plein.',
              reviewed: true },
    },
    'label.captionMorph': {
        en: { t: 'Morph Pad — the scan LFO slowly morphs your drawbars toward a square; long envelopes make it breathe.' },
        fr: { t: 'Nappe morphée — le LFO de balayage morphe lentement vos tirettes vers une carrée ; de longues enveloppes la font respirer.',
              reviewed: true },
    },
    'label.captionLofi': {
        en: { t: 'Lo-Fi Bells — a spread bell spectrum + spectral-decay tilt + 8-bit quantization for digital grit.' },
        fr: { t: 'Cloches lo-fi — un spectre de cloche étalé + une pente de décroissance spectrale + une quantification 8 bits pour le grain numérique.',
              reviewed: true },
    },

    // ── On-screen keyboard ──────────────────────────────────────────────────
    'label.play': {
        en: { t: 'Play ·' },
        fr: { t: 'Jouer ·', reviewed: true },
    },
    // The letter run is the QWERTY key map, not prose: it names physical keys
    // and stays exactly as it is in both languages. Only the sentence around it
    // is translated.
    'label.kbdHint': {
        // The letter run keeps its HAIR SPACES (U+200A, `&#8202;` in the markup)
        // as \u200a escapes. applyLabel writes this string over the authored
        // markup, so a plain space here would silently widen the key run in
        // BOTH languages — invisible to an en-vs-fr geometry diff, and a
        // change to the shipped English nobody asked for.
        en: { t: 'click the keys or use your computer keyboard (A\u200aS\u200aD\u200aF\u200aG\u200aH\u200aJ\u200aK · W\u200aE\u200aT\u200aY\u200aU)' },
        fr: { t: 'cliquez sur les touches ou utilisez le clavier de l’ordinateur (A\u200aS\u200aD\u200aF\u200aG\u200aH\u200aJ\u200aK · W\u200aE\u200aT\u200aY\u200aU)',
              reviewed: true },
    },

    // ── Accessible names ────────────────────────────────────────────────────
    // Read by data-i18n-aria. The knobs and combos do NOT appear here: their
    // accessible name is their tooltip title, reached through trLabel's I18N
    // fallback, which is the one place on this page reuse is deliberate.
    'aria.settings': {
        en: { t: 'Settings' }, fr: { t: 'Réglages', reviewed: true },
    },
    'aria.langSelect': {
        en: { t: 'Interface language' }, fr: { t: 'Langue de l’interface', reviewed: true },
    },
    'aria.helpToggle': {
        en: { t: 'Toggle hover help' }, fr: { t: "Activer ou désactiver l’aide au survol", reviewed: true },
    },
    // The switch's two faces, written through setLabel from applyTipsEnabled.
    'ui.on': {
        en: { t: 'On' }, fr: { t: 'Activée', reviewed: true },
    },
    'ui.off': {
        en: { t: 'Off' }, fr: { t: 'Désactivée', reviewed: true },
    },
    'aria.keyboard': {
        en: { t: 'On-screen keyboard' }, fr: { t: 'Clavier à l’écran', reviewed: true },
    },
});

// ============================================================================
// I18N_EXEMPT — reasoned exclusions, never silence
//
// Every visible string the coverage scan finds must be a [data-i18n] element, a
// setLabel() call, or an entry HERE WITH A REASON. A bare skip list would let a
// missed label hide as a deliberate one.
// ============================================================================

export const I18N_EXEMPT = [
    // The <h1> splits the product name across two text nodes so the second half
    // can carry the green italic .title-accent. Both halves are the same
    // untranslatable name; keying either would translate half a wordmark.
    ['O – simple',
     'the product name, first half of the split wordmark in the page heading — a product name is never translated'],
    ['Additive',
     'the product name, second half of the split wordmark (.title-accent) — a product name is never translated'],

    // The two endonyms in the language selector. A language name is written in
    // its OWN language: a French speaker looking for their language looks for
    // "Français", not "French".
    ['English',  'endonym — a language name is never translated'],
    ['Français', 'endonym — a language name is never translated'],

    // The Frame B and Bit Depth menu entries are built at runtime from the
    // AudioParameterChoice's own choice strings. Those are the host automation
    // contract and stay English under D-01; translating the menu without
    // translating the automation lane would make the two disagree.
    ['Sine',   'AudioParameterChoice entry (frameBSource) — the host automation name, English under D-01'],
    ['Saw',    'AudioParameterChoice entry (frameBSource) — the host automation name, English under D-01'],
    ['Square', 'AudioParameterChoice entry (frameBSource) — the host automation name, English under D-01'],
    ['Odd',    'AudioParameterChoice entry (frameBSource) — the host automation name, English under D-01'],
    ['Off',    'AudioParameterChoice entry (bitDepth) — the host automation name, English under D-01'],
];

// ============================================================================
// TIP_BINDINGS — [selector, key, wrapper?, vars?]
//
// The tip anchor IS the element the selector finds: this page authors its tips
// on the cell rather than on the knob inside it, so no closest(wrapper) walk is
// needed anywhere.
//
// Through v1.0.7 the anchors carried the tip KEY in their own data-tip
// attribute and js/app.js looked the copy up in a TIPS object. That cannot
// survive canon v2 — applyI18n WRITES data-tip as the tip BODY, so the key and
// the copy would fight over one attribute, and check-i18n assertion 3 requires
// index.html to carry zero data-tip literals. The eighteen parameter cells and
// the sixteen drawbar cells gained a data-param attribute naming the APVTS
// parameter they drive; the drawbar panel gained an id; the six lesson buttons
// are addressed by the data-preset they already carried.
//
// The sixteen drawbar cells are built by buildDrawbars() in js/app.js, which is
// why boot() calls it BEFORE initI18n(): a cell the sweep never sees keeps no
// tip at all.
// ============================================================================

export const TIP_BINDINGS = [
    ['#gear-btn',                        'gear-btn'],
    ['#lang-select',                     'lang-select'],
    ['#help-toggle',                     'help-toggle'],

    ['#drawbar-panel',                   'drawbars'],

    ['[data-param="frameBSource"]',      'frameBSource'],
    ['[data-param="scanPosition"]',      'scanPosition'],
    ['[data-param="scanLfoRate"]',       'scanLfoRate'],
    ['[data-param="scanLfoDepth"]',      'scanLfoDepth'],
    ['[data-param="scanEnvAmount"]',     'scanEnvAmount'],

    ['[data-param="spectralDecay"]',     'spectralDecay'],
    ['[data-param="bitDepth"]',          'bitDepth'],
    ['[data-param="velToDecay"]',        'velToDecay'],

    ['[data-param="ampAttack"]',         'ampAttack'],
    ['[data-param="ampDecay"]',          'ampDecay'],
    ['[data-param="ampSustain"]',        'ampSustain'],
    ['[data-param="ampRelease"]',        'ampRelease'],

    ['[data-param="modAttack"]',         'modAttack'],
    ['[data-param="modDecay"]',          'modDecay'],
    ['[data-param="modSustain"]',        'modSustain'],
    ['[data-param="modRelease"]',        'modRelease'],

    ['[data-param="outputLevel"]',       'outputLevel'],

    ['.tour-btn[data-preset="Pure Sine"]',   'lessonSine'],
    ['.tour-btn[data-preset="Sawtooth"]',    'lessonSaw'],
    ['.tour-btn[data-preset="Square"]',      'lessonSquare'],
    ['.tour-btn[data-preset="Organ"]',       'lessonOrgan'],
    ['.tour-btn[data-preset="Morph Pad"]',   'lessonMorph'],
    ['.tour-btn[data-preset="Lo-Fi Bells"]', 'lessonLofi'],

    ['[data-param="partial1"]',          'partial1'],
    ['[data-param="partial2"]',          'partial2'],
    ['[data-param="partial3"]',          'partial3'],
    ['[data-param="partial4"]',          'partial4'],
    ['[data-param="partial5"]',          'partial5'],
    ['[data-param="partial6"]',          'partial6'],
    ['[data-param="partial7"]',          'partial7'],
    ['[data-param="partial8"]',          'partial8'],
    ['[data-param="partial9"]',          'partial9'],
    ['[data-param="partial10"]',         'partial10'],
    ['[data-param="partial11"]',         'partial11'],
    ['[data-param="partial12"]',         'partial12'],
    ['[data-param="partial13"]',         'partial13'],
    ['[data-param="partial14"]',         'partial14'],
    ['[data-param="partial15"]',         'partial15'],
    ['[data-param="partial16"]',         'partial16'],
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
