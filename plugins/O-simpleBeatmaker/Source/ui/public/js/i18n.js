/*
   This file is part of O-simpleBeatmaker, an Ouaricon Audio plugin.
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
   along with this program.  If not, see https://www.gnu.org/licenses/ .
*/
// ============================================================================
// i18n.js — O-simpleBeatmaker interface copy, English + French (v1.1.1)
//
// ── v1.1.1: FRENCH QA PASS (Stage N, 2026-08-31) ────────────────────────────
// Every fr entry read against its en and against scripts/i18n-fr-glossary.js.
// Changed: 32 of 81 entries (5 terminology, 21 typography, 3 grammar, 3
// meaning). sameAsEn: kept 8, translated 0. termNote exemptions: 0.
// Left as drafted: the other 49. reviewed: false throughout — no native speaker
// has read any of it yet, and this pass is a second MACHINE reading.
//
// The decisions the next reader needs:
//
//  * "Cliquer" is INTRANSITIVE in French. Three sites said "cliquez une
//    cellule" / "cliquez un préréglage", which is a direct calque of the
//    English transitive. All three now say "cliquez sur". In the width-pinned
//    grid hint that cost 18.01 px, which is 0.45 px MORE than the line had —
//    so "cellule" became "case", the ordinary French for a square in a grid
//    (and the shorter word): the hint measures 344.03 px against the shipped
//    336.69, and the Clear-all button stays on row 1 at both frames. "case"
//    then carries through the grid and pattern-length BODIES so the page has
//    one word for one thing.
//  * THAT SPENDS BANKED MARGIN, and the number is here so the call can be
//    reversed cheaply. The Clear-all button now ends 7.17 px inside the
//    1035 px content edge, where v1.1.0 left 17.56 px. The margin was banked
//    against Windows/WebView2 font metrics, which are still a named deferral
//    in this task and are still unmeasured. If a Windows pass finds the row
//    wrapping, the cheap fix is "cliquez sur une case pour l'allumer"
//    (328.14 px, 23.06 px clear) at the cost of not naming the step.
//  * The step-grid hint's ';' fragment (label.gridHintB) now OPENS with a
//    U+00A0. The em and the span abut with no whitespace at all in the markup,
//    which is right for English ("ghost; right-click") and wrong for French,
//    which spaces before a semicolon. +3.05 px, counted in the 7.17 above.
//  * "Fût" is a drum SHELL. Two of the six voices on this page (Clap, and the
//    two hats) have no shell, so the Tone and Solo bodies now say
//    "l'instrument" where the English says "the drum" / "one drum".
//  * "Les temps forts" are beats 1 and 3 in French. The English lessonGhost
//    body says BACKBEATS, which the lesson beside it defines as the snare on
//    2 and 4, so the French now names "le backbeat" — the word the user can
//    read on the button (the six lesson faces are I18N_EXEMPT).
//  * "Mixage" is kept in the voiceLevel BODY ("dans le mixage"): the glossary
//    forbids it as a LABEL, where it would be the Mix control, and bodies are
//    not matched against TERMS. There is no Mix control on this page.
//  * "Sans feeling" (lessonStraight) and "un fill" (voiceTune) are kept: they
//    are the loanwords French drummers use, and the page's own French for
//    "Timing Feel" is "Placement rythmique", which does not fit either clause.
//  * "STAN" for DAW is kept — 12 occurrences across the suite use it.
//  * The nine tooltip TITLES that equal their English carry NO sameAsEn flag:
//    check-i18n reads that flag entry-scoped (title AND body), so flagging a
//    title over a translated body would disarm assertion 4 for the whole
//    entry (Stage N correction 26). Six of the nine are the lesson-preset
//    names, English by D-02; the other three (Swing, Tempo, Solo) are French
//    words. The eight LABELS copies keep sameAsEn: true, which is the correct,
//    entry-scoped declaration for a caption.
//
// UI ROOT IS Source/ui/public. There is no second UI root in this plugin. This
// file is a SOURCES entry in juce_add_binary_data(O-simpleBeatmaker_UIResources)
// and is served by PluginEditor::getResource at /js/i18n.js. Embedded but not
// served, or served but not embedded, is a 404 that presents as a BLANK page
// rather than an English one — the import in app.js fails to resolve and module
// evaluation never starts. check-i18n assertion 8 checks both halves.
//
// An ES module that EXPORTS ONLY. It must never self-execute: a bare top-level
// statement here throws out of module evaluation and takes every later
// initializer on the page with it (pattern_module_toplevel_init_tdz).
// scripts/check-i18n.js assertion 7 enforces it.
//
// FILENAME: no hyphen. juce_add_binary_data STRIPS hyphens rather than
// converting them to underscores (critical_binary_data_strips_hyphens), so one
// combined file for both languages sidesteps the question entirely.
//
// COPY IS textContent ON EVERY PATH — never innerHTML. Through v1.0.3 the page
// built its tooltip, its grid cells, its MIDI rows and its six voice strips with
// innerHTML; v1.1.0 builds all four with createElement + textContent, because
// the text is table-sourced and localized now rather than a fixed literal.
// check-i18n assertion 9 rejects any innerHTML reference here and any string
// literal containing an opening angle bracket.
//
// THE ENGLISH WAS MOVED, NOT REWRITTEN. Every en value below was extracted
// MECHANICALLY from v1.0.3 — the TIPS table in js/app.js and the text nodes and
// attributes of index.html, read by the same decodeEntities the inventory uses —
// and never re-typed. HTML entities are decoded to the characters they named
// (&#183; to ·, &#8594; to →, &#8212; to —, &#9474; to │, &#9679; to ●). The two
// hair spaces this page carries (&#8202;, either side of the en dash in the
// wordmark) are in the ONE string that is not in this table at all: the product
// name, which is I18N_EXEMPT. Nothing here needs an invisible-character escape,
// and nothing here contains one — verified by grep after the file was written.
//
// TWO DELIBERATE ENGLISH CHANGES, both recorded in the CHANGELOG:
//   1. The tip bodies have lost their strong/em tags. The WORDS are unchanged.
//      Assertion 9 forbids an angle bracket in a string literal here, and it is
//      right to — the renderer writes textContent, so a tag would render as
//      literal characters.
//   2. The timing-lane hint has lost the italic on the word "actual" and is now
//      ONE string. Splitting it at the emphasis, the way the step-grid hint IS
//      split, would need a fragment reading "each hit's" — and French moves both
//      the possessive and the adjective, so that fragment has no translation.
//      The step-grid hint keeps its emphasis and its keycap because its
//      fragments are whole clauses whose order survives the translation.
//
// KEYS ARE THE PARAMETER ID where the anchor is a parameter cell, and a
// label.* / aria.* / ui.* slug otherwise. The parameter cells are addressed by a
// data-param attribute ADDED IN v1.1.0; through v1.0.3 they carried the tip KEY
// in their own data-tip attribute, which canon v2 overwrites with the tip BODY.
//
// LABELS NEVER REUSE A TOOLTIP KEY for a caption, even where the two strings
// happen to be equal today ("Swing", "Tempo", "Clear all" vs "Clear All"). A
// caption and a tip title diverge the moment either is edited, and a rule that
// holds for three of thirty-eight is a rule nobody can apply. The ONE place the
// I18N fallback is used on purpose is data-i18n-aria on the twenty-nine knobs
// and the length selector: an accessible name IS the control's name, so it reads
// the tooltip title by design.
//
// FOUR aria.cell* ENTRIES LIVE IN I18N RATHER THAN LABELS, with an empty body.
// They are the accessible name of a step-grid cell, composed per cell through
// trLabel() with vars (voice, step number, velocity) — 96 to 192 elements whose
// name is different in every one, so it cannot be a markup attribute and cannot
// be a setLabel call. A direct trLabel() call is not a reference assertion 15
// can see, so a LABELS home would report all four DEAD: the gate describing a
// violation of a rule the code is obeying. The empty body is honest — they are
// never rendered as hover help.
//
// ALL FRENCH IS MACHINE-DRAFTED AND FLAGGED reviewed: false. No native speaker
// has read it. node scripts/check-i18n.js prints the worklist.
// ============================================================================

export const LANGUAGES = ['en', 'fr'];

export const I18N = Object.freeze({

    // ── The settings popover (v1.1.0) ───────────────────────────────────────
    // The gear is new. This plugin has never had a hover-help switch and does
    // not gain one here: the panel holds the language selector alone, so there
    // is exactly one setting in it and nothing to group.
    'gear-btn': {
        en: { t: "Settings",
              b: "Choose the language of the interface. The choice is remembered with the session, so a project reopens in the language you left it in." },
        fr: { t: "Réglages",
              b: "Choisir la langue de l’interface. Le choix est conservé avec la session : un projet se rouvre dans la langue où vous l’avez laissé.",
              reviewed: true },
    },
    'lang-select': {
        en: { t: "Language",
              b: "The language of the labels on this page and of this hover help. English and French are available; the value readouts, the six lesson-preset names and the MIDI note numbers stay in English." },
        fr: { t: "Langue",
              b: "La langue des libellés de cette page et de cette aide au survol. L’anglais et le français sont disponibles ; les valeurs affichées, les noms des six préréglages de leçon et les numéros de note MIDI restent en anglais.",
              reviewed: true },
    },

    // ── The three visualization panels ──────────────────────────────────────
    grid: {
        en: { t: "The Step Grid",
              b: "Each row is a drum voice, each column a sixteenth-note step. Click a cell to light a step; click again to cycle normal → accent → ghost (cell height = velocity); right-click to clear. The amber bar sweeping across is the playhead — when it crosses a lit cell, that voice fires." },
        fr: { t: "La grille de pas",
              b: "Chaque ligne est une voix de batterie, chaque colonne un pas de double croche. Cliquez sur une case pour allumer un pas ; cliquez à nouveau pour parcourir normal → accent → fantôme (la hauteur de la case = la vélocité) ; clic droit pour effacer. La barre ambre qui balaie est la tête de lecture — quand elle croise une case allumée, cette voix se déclenche.",
              reviewed: true },
    },
    lane: {
        en: { t: "Timing / Groove Lane",
              b: "The standout view: each dot is a hit, placed at its actual moment relative to its grid line. A line to the left of the tick = early, right = late. This is the exact Δt applied to the audio — turn Swing and watch off-beats drift right together; Humanize scatters them; Quantize pulls the scatter back toward the grid while leaving swing." },
        fr: { t: "Voie de placement / groove",
              b: "La vue phare : chaque point est une frappe, placée à son moment réel par rapport à sa ligne de grille. Un trait à gauche du repère = en avance, à droite = en retard. C’est le Δt exact appliqué à l’audio — tournez Swing et regardez les contretemps glisser ensemble vers la droite ; Humaniser les disperse ; Quantifier resserre la dispersion tout en laissant le swing.",
              reviewed: true },
    },
    midi: {
        en: { t: "Live MIDI Readout",
              b: "Every note-on as it fires, from the sequencer (SEQ) and from notes you play in (MIDI). The grid and this list are two views of one MIDI stream — the sequencer literally emits these messages into the same buffer your playing does." },
        fr: { t: "Lecture MIDI en direct",
              b: "Chaque note-on au moment où elle part, depuis le séquenceur (SÉQ) et depuis les notes que vous jouez (MIDI). La grille et cette liste sont deux vues d’un même flux MIDI — le séquenceur émet littéralement ces messages dans le même tampon que votre jeu.",
              reviewed: true },
    },
    clearGrid: {
        en: { t: "Clear All",
              b: "Erases every step in the pattern across all six voices — a blank grid to start a fresh beat from." },
        fr: { t: "Tout effacer",
              b: "Efface tous les pas du motif sur les six voix — une grille vierge pour repartir sur un nouveau rythme.",
              reviewed: true },
    },

    // ── Global timing-feel parameters ───────────────────────────────────────
    swing: {
        en: { t: "Swing",
              b: "Delays every off-beat sixteenth, turning a stiff grid into a shuffle. 0% is dead-straight; 75% is the maximum MPC-style swing. Crucially, swing is not removed by Quantize — it is a deliberate, musical lateness, not an error." },
        fr: { t: "Swing",
              b: "Retarde chaque double croche à contretemps, ce qui transforme une grille rigide en shuffle. 0 % est parfaitement droit ; 75 % est le swing maximal à la manière des MPC. Point clé : le swing n’est pas retiré par la quantification — c’est un retard musical voulu, pas une erreur.",
              reviewed: true },
    },
    humanize: {
        en: { t: "Humanize",
              b: "Adds a small random timing and velocity wobble to every hit, like a human drummer who never lands exactly on the grid. A little brings a flat pattern to life; too much sounds sloppy. Watch the lane scatter as you raise it." },
        fr: { t: "Humaniser",
              b: "Ajoute à chaque frappe une petite instabilité aléatoire de placement et de vélocité, comme un batteur humain qui ne tombe jamais exactement sur la grille. Une petite dose réveille un motif plat ; l’excès le rend brouillon. Regardez la voie se disperser à mesure que vous montez le bouton.",
              reviewed: true },
    },
    quantizeStrength: {
        en: { t: "Quantize Strength",
              b: "How hard hits are pulled back onto the grid. At 100% the random humanize is fully removed (dead tight); at 0% the full wobble is kept. The exact tradeoff the craft names: quantize enough that the part is solid, without over-quantizing the life out of it. Swing survives quantize — only the random part is pulled in." },
        fr: { t: "Force de quantification",
              b: "À quel point les frappes sont ramenées sur la grille. À 100 % l’humanisation aléatoire est entièrement retirée (parfaitement serré) ; à 0 % toute l’instabilité est gardée. C’est exactement le compromis dont parle le métier : quantifier assez pour que la partie tienne, sans quantifier au point de lui retirer sa vie. Le swing survit à la quantification — seule la part aléatoire est ramenée.",
              reviewed: true },
    },
    tempo: {
        en: { t: "Tempo",
              b: "Playback speed in beats per minute — used when there is no host transport (the standalone app, or a stopped DAW). When a DAW is playing, the grid locks to the host's tempo instead." },
        fr: { t: "Tempo",
              b: "Vitesse de lecture en battements par minute — utilisée quand il n’y a pas de transport hôte (l’application autonome, ou une STAN à l’arrêt). Quand une STAN joue, la grille se cale sur le tempo de l’hôte à la place.",
              reviewed: true },
    },
    patternLength: {
        en: { t: "Pattern Length",
              b: "How many steps the loop is before it repeats: 8, 16, or 32. Shrinking then re-growing keeps the cells you drew — they are remembered, just not played while the loop is short." },
        fr: { t: "Longueur du motif",
              b: "Combien de pas dure la boucle avant de se répéter : 8, 16 ou 32. Raccourcir puis rallonger conserve les cases que vous avez dessinées — elles sont mémorisées, simplement pas jouées tant que la boucle est courte.",
              reviewed: true },
    },
    outputLevel: {
        en: { t: "Output Level",
              b: "Master output trim in decibels. −60 dB is silence." },
        fr: { t: "Niveau de sortie",
              b: "Ajustement du niveau général en décibels. −60 dB, c’est le silence.",
              reviewed: true },
    },

    // ── Per-voice parameters ────────────────────────────────────────────────
    // One tip per SUFFIX, shared by all six voices: the four knob suffixes and
    // the two toggles repeat identically down the rack, so there are six anchors
    // per key rather than six copies of the key. TIP_BINDINGS names each anchor
    // individually because document.querySelector returns the FIRST match — a
    // class selector would land every one of them on the Kick strip.
    voiceTune: {
        en: { t: "Tune",
              b: "Shifts this voice's pitch up or down by up to an octave (±12 semitones). Tune the kick down for weight, the toms across a fill." },
        fr: { t: "Accord",
              b: "Décale la hauteur de cette voix vers le haut ou le bas, jusqu’à une octave (±12 demi-tons). Accordez la grosse caisse vers le bas pour du poids, les toms en escalier sur un fill.",
              reviewed: true },
    },
    voiceDecay: {
        en: { t: "Decay",
              b: "How long the voice rings out. Short snaps it into a tight tick; long lets it boom or sizzle. The musical range differs per voice (a kick boom vs. a closed-hat tick)." },
        fr: { t: "Déclin",
              b: "Combien de temps la voix résonne. Court la réduit à un tic serré ; long la laisse gronder ou grésiller. La plage musicale diffère selon la voix (le grondement d’une grosse caisse contre le tic d’un charley fermé).",
              reviewed: true },
    },
    voiceTone: {
        en: { t: "Tone",
              b: "The voice's character knob — snap/brightness/body-vs-noise, depending on the drum. Sweep it to hear the timbre shift from dark to bright (or body to noise)." },
        fr: { t: "Timbre",
              b: "Le bouton de caractère de la voix — claquant, brillance, corps contre bruit, selon l’instrument. Balayez-le pour entendre le timbre passer du sombre au brillant (ou du corps au bruit).",
              reviewed: true },
    },
    voiceLevel: {
        en: { t: "Level",
              b: "This voice's volume in the mix, in decibels. −60 dB silences it." },
        fr: { t: "Niveau",
              b: "Le volume de cette voix dans le mixage, en décibels. −60 dB la rend muette.",
              reviewed: true },
    },
    voiceMute: {
        en: { t: "Mute",
              b: "Silences this voice without erasing its pattern — solo a part by muting the rest, or drop a voice out and back in." },
        fr: { t: "Muet",
              b: "Rend cette voix silencieuse sans effacer son motif — isolez une partie en rendant les autres muettes, ou faites sortir puis revenir une voix.",
              reviewed: true },
    },
    voiceSolo: {
        en: { t: "Solo",
              b: "Plays only the soloed voice(s), muting everything else. Great for hearing exactly what one drum is doing in the groove." },
        fr: { t: "Solo",
              b: "Ne joue que la ou les voix isolées et rend tout le reste muet. Idéal pour entendre exactement ce que fait un seul instrument dans le groove.",
              reviewed: true },
    },

    // ── The lesson tour ─────────────────────────────────────────────────────
    // EVERY lesson TITLE below stays in English on purpose, and the six are NOT
    // an oversight of assertion 4 — each body differs, so the passthrough check
    // passes on its own. They are the factory preset names the C++ side knows
    // (kBeatPresets[]), and the tour caption prints the clicked one back
    // verbatim through a {name} token (D-02). A French tip title over an English
    // caption is exactly the page-versus-preset disagreement D-02 exists to
    // prevent. Same discriminator O-simpleFM applied to its five lesson chips:
    // the name is visible ELSEWHERE on the page, so it is not translated.
    presets: {
        en: { t: "Lesson Presets",
              b: "A guided tour where each preset isolates one idea — straight vs. swung, accents, ghost notes, humanize, quantize. Click one to load it, then tweak a knob to hear the concept." },
        fr: { t: "Préréglages de leçon",
              b: "Une visite guidée où chaque préréglage isole une seule idée — droit contre swingué, accents, notes fantômes, humanisation, quantification. Cliquez-en un pour le charger, puis tournez un bouton pour entendre le concept.",
              reviewed: true },
    },
    lessonStraight: {
        en: { t: "Straight",
              b: "A flat, no-feel pattern — every hit dead on the grid at one velocity. The baseline that everything else departs from." },
        fr: { t: "Straight",
              b: "Un motif plat, sans feeling — chaque frappe exactement sur la grille, à une seule vélocité. La référence dont tout le reste s’écarte.",
              reviewed: true },
    },
    lessonAccents: {
        en: { t: "Backbeat + Accents",
              b: "Snare on 2 and 4 with hard accents, quieter hits between — how velocity alone turns a march into a groove." },
        fr: { t: "Backbeat + Accents",
              b: "Caisse claire sur 2 et 4 avec des accents marqués, des frappes plus douces entre — comment la vélocité seule transforme une marche en groove.",
              reviewed: true },
    },
    lessonGhost: {
        en: { t: "Ghost Notes",
              b: "Quiet snare hits tucked between the backbeats — the secret to a pattern that breathes." },
        fr: { t: "Ghost Notes",
              b: "Des frappes de caisse claire discrètes glissées entre celles du backbeat — le secret d’un motif qui respire.",
              reviewed: true },
    },
    lessonSwing: {
        en: { t: "Triplet Swing",
              b: "The same pattern with swing pushed up — feel the off-beats slide late into a shuffle." },
        fr: { t: "Triplet Swing",
              b: "Le même motif avec le swing poussé — sentez les contretemps glisser en retard vers le shuffle.",
              reviewed: true },
    },
    lessonHumanized: {
        en: { t: "Humanized",
              b: "A tight pattern loosened with humanize — watch the lane scatter off the grid lines." },
        fr: { t: "Humanized",
              b: "Un motif serré assoupli par l’humanisation — regardez la voie se disperser hors des lignes de grille.",
              reviewed: true },
    },
    lessonQuantize: {
        en: { t: "Quantize Demo",
              b: "Humanize up, then sweep quantize strength to pull the scatter back — the tradeoff made audible and visible." },
        fr: { t: "Quantize Demo",
              b: "Montez l’humanisation, puis balayez la force de quantification pour resserrer la dispersion — le compromis rendu audible et visible.",
              reviewed: true },
    },

    // ── Step-cell accessible names (NOT hover help — see the header note) ────
    // Composed per cell by paintCell() through trLabel(). {voice} is itself a
    // LABELS key and resolves to the localized voice name; {step} and {vel} are
    // numbers and substitute verbatim (D-03).
    'aria.cellOff': {
        en: { t: "{voice} step {step}: off", b: '' },
        fr: { t: "{voice} pas {step} : éteint", b: '', reviewed: true },
    },
    'aria.cellNormal': {
        en: { t: "{voice} step {step}: normal (velocity {vel})", b: '' },
        fr: { t: "{voice} pas {step} : normal (vélocité {vel})", b: '', reviewed: true },
    },
    'aria.cellAccent': {
        en: { t: "{voice} step {step}: accent (velocity {vel})", b: '' },
        fr: { t: "{voice} pas {step} : accent (vélocité {vel})", b: '', reviewed: true },
    },
    'aria.cellGhost': {
        en: { t: "{voice} step {step}: ghost (velocity {vel})", b: '' },
        fr: { t: "{voice} pas {step} : fantôme (vélocité {vel})", b: '', reviewed: true },
    },
});

// ────────────────────────────────────────────────────────────────── LABELS ──
// One string, no body: a label is not a tooltip.
export const LABELS = Object.freeze({

    // ── Header ──────────────────────────────────────────────────────────────
    // The product name itself is NOT here — see I18N_EXEMPT. Only the strapline
    // under it is copy.
    'label.subtitle': {
        en: { t: "Step-Sequencer Drum Machine · program a beat, then watch velocity, swing, quantize & humanize reshape it · A Field Guide" },
        // SHORTENED, and flagged for the reviewer. The faithful "Boîte à rythmes
        // à séquenceur pas à pas · programmez un rythme, puis regardez vélocité,
        // swing, quantification et humanisation le remodeler · un guide de
        // terrain" measures 806.4px against the 706.4px the header can give the
        // title block once the transport strip is pinned, so it wrapped to a
        // second line and pushed the whole page down 15px. Two connecting
        // phrases are dropped and the object moves to the end of the clause; the
        // sentence still names the machine, all four timing controls and the
        // series. 690.0px, 16.4px of clearance.
        fr: { t: "Boîte à rythmes pas à pas · programmez, puis voyez vélocité, swing, quantification et humanisation remodeler le rythme · guide de terrain", reviewed: true },
    },

    // The transport strip. The NUMBERS beside these three are readouts and are
    // never keyed (D-03); "BPM" is a unit symbol and is language-neutral.
    'label.trTempo': {
        en: { t: "tempo" },
        fr: { t: "tempo", sameAsEn: true, reviewed: true },
    },
    'label.trLength': {
        en: { t: "length" },
        fr: { t: "longueur", reviewed: true },
    },
    'label.trSteps': {
        en: { t: "steps" },
        fr: { t: "pas", reviewed: true },
    },
    // The two faces of the transport state line. It is NOT a parameter mirror:
    // there is no sync parameter in the APVTS — frame.sync is host transport
    // state pushed from the editor Timer — so this is a status string and it is
    // in scope. The bullet belongs to the string in BOTH languages and in both
    // faces, so the glyph never has to be re-attached by the writer.
    'label.freeRun': {
        en: { t: "● free-run" },
        fr: { t: "● libre", reviewed: true },
    },
    'label.synced': {
        en: { t: "● synced" },
        fr: { t: "● synchro", reviewed: true },
    },

    // ── Step grid ───────────────────────────────────────────────────────────
    // The hint is FIVE keys because it carries an emphasis and a keycap chip
    // that are real affordances. Each fragment is a whole clause and the order
    // survives the translation — see the header note on the lane hint, which is
    // the case where it does NOT and is therefore one flat string.
    'label.gridCaption': {
        en: { t: "Step Grid ·" },
        fr: { t: "Grille de pas ·", reviewed: true },
    },
    'label.gridHintA': {
        en: { t: "click a cell to light a step; click again to cycle" },
        // SHORTENED, and flagged for the reviewer. "cliquez à nouveau" is 353.6px
        // and the whole hint line then pushed the Clear-all button onto a second
        // row, taking the step-grid panel 15px taller than the English one.
        // "cliquez encore" is 336.7px and is the same instruction.
        //
        // v1.1.1 (Stage N) — RE-MEASURED, at the shipping frame, with the
        // gridHintB no-break space already applied. "cliquer" is intransitive
        // in French, so "cliquez une cellule" is a calque and had to go; the
        // faithful "cliquez sur une cellule pour allumer un pas" is 354.70px
        // and WRAPS the row (button y 84 -> 99, panel 240 -> 255). "case" is
        // the ordinary French for a square in a grid and is shorter:
        //     cliquez une cellule …   336.69px   button right 1020.48
        //     cliquez sur une cellule 354.70px   WRAPS
        //     cliquez sur une case …  344.03px   button right 1027.83  (shipped)
        //     cliquez sur une case pour l'allumer
        //                             328.14px   button right 1011.94
        // The shipped line leaves 7.17px inside the 1035px content edge, down
        // from 17.56px. The last candidate is the reversal if a Windows pass
        // ever needs the margin back; it costs naming the step.
        fr: { t: "cliquez sur une case pour allumer un pas ; cliquez encore pour parcourir", reviewed: true },
    },
    'label.gridHintEm': {
        en: { t: "normal → accent → ghost" },
        fr: { t: "normal → accent → fantôme", reviewed: true },
    },
    'label.gridHintB': {
        en: { t: "; right-click (or" },
        // OPENS WITH A U+00A0 (v1.1.1, Stage N). index.html abuts the </em> and
        // this <span> with no whitespace at all, so the semicolon lands directly
        // against "ghost" / "fantôme". That is correct English and wrong French,
        // which spaces before a semicolon — and the space must be no-break, or a
        // line could start with the ';'. +3.05px on the hint row.
        fr: { t: " ; clic droit (ou", reviewed: true },
    },
    // The keycap. French keyboards print "Suppr" on the key English keyboards
    // print "Del" on, so this is the one string on the page whose translation is
    // decided by hardware rather than by language.
    'label.gridHintKbd': {
        en: { t: "Del" },
        fr: { t: "Suppr", reviewed: true },
    },
    'label.gridHintC': {
        en: { t: ") to clear · the bar sweeping across is the playhead" },
        // SHORTENED FOR MARGIN, NOT FOR THE GATE, and flagged for the reviewer.
        // The faithful ") pour effacer · la barre qui balaie est la tête de
        // lecture" is 259.8px and the gate PASSES with it, because the
        // gridHintA shortening above is already enough on its own. What it costs
        // is clearance: the Clear-all button then ends 2.0px inside the
        // 1035px content edge instead of 17.6px, and Windows/WebView2 font
        // metrics are a named deferral in this task. "qui balaie" survives
        // verbatim in the grid TOOLTIP, which is where the description belongs.
        //
        // v1.1.1 (Stage N): the reference figures above are v1.1.0's. The
        // gridHintA re-measurement and the gridHintB no-break space have since
        // taken the shipped clearance to 7.17px, so the faithful ") pour effacer
        // · la barre qui balaie …" would now OVERFLOW the row rather than leave
        // 2.0px. This string is unchanged and the trade it records is now worth
        // more, not less.
        fr: { t: ") pour effacer · la barre mobile est la tête de lecture", reviewed: true },
    },
    'label.clearAll': {
        en: { t: "Clear all" },
        fr: { t: "Tout effacer", reviewed: true },
    },

    // ── Timing / groove lane ────────────────────────────────────────────────
    'label.laneCaption': {
        en: { t: "Timing / Groove Lane ·" },
        fr: { t: "Voie de placement / groove ·", reviewed: true },
    },
    'label.laneHint': {
        en: { t: "each hit's actual offset from its grid line — left of the line = early, right = late. This is the Δt baked into the audio, not a guess." },
        fr: { t: "le décalage réel de chaque frappe par rapport à sa ligne de grille — à gauche de la ligne = en avance, à droite = en retard. C’est le Δt inscrit dans l’audio, pas une estimation.", reviewed: true },
    },
    'label.lkGrid': {
        en: { t: "│ grid line (nominal step)" },
        fr: { t: "│ ligne de grille (pas nominal)", reviewed: true },
    },
    'label.lkSwing': {
        en: { t: "swing → steady lateness on off-beats" },
        fr: { t: "swing → retard régulier sur les contretemps", reviewed: true },
    },
    'label.lkHuman': {
        en: { t: "humanize → random scatter" },
        fr: { t: "humaniser → dispersion aléatoire", reviewed: true },
    },
    'label.lkQuant': {
        en: { t: "quantize → pulls scatter back" },
        fr: { t: "quantifier → resserre la dispersion", reviewed: true },
    },

    // ── Live MIDI readout ───────────────────────────────────────────────────
    'label.midiCaption': {
        en: { t: "Live MIDI Readout ·" },
        fr: { t: "Lecture MIDI en direct ·", reviewed: true },
    },
    'label.midiHint': {
        en: { t: "note-on messages as steps fire — the grid and this list are two views of one MIDI stream" },
        fr: { t: "les messages note-on au déclenchement de chaque pas — la grille et cette liste sont deux vues d’un même flux MIDI", reviewed: true },
    },
    // The two source tags and the two field names of a readout ROW. The numbers
    // they carry are readouts and substitute verbatim (D-03); the words around
    // them are copy. "SEQ" is an abbreviation of the sequencer and does change;
    // "MIDI" is a protocol name and does not.
    'label.srcSeq': {
        en: { t: "SEQ" },
        fr: { t: "SÉQ", reviewed: true },
    },
    'label.srcMidi': {
        en: { t: "MIDI" },
        fr: { t: "MIDI", sameAsEn: true, reviewed: true },
    },
    'label.midiNote': {
        en: { t: "note {n}" },
        fr: { t: "note {n}", sameAsEn: true, reviewed: true },
    },
    'label.midiVel': {
        en: { t: "vel {v}" },
        fr: { t: "vél {v}", reviewed: true },
    },

    // ── Control groups ──────────────────────────────────────────────────────
    'label.groupTiming': {
        en: { t: "Timing Feel" },
        fr: { t: "Placement rythmique", reviewed: true },
    },
    'label.knobSwing': {
        en: { t: "Swing" },
        fr: { t: "Swing", sameAsEn: true, reviewed: true },
    },
    'label.knobHumanize': {
        en: { t: "Humanize" },
        fr: { t: "Humaniser", reviewed: true },
    },
    'label.knobQuantize': {
        en: { t: "Quantize" },
        fr: { t: "Quantifier", reviewed: true },
    },
    'label.knobTempo': {
        en: { t: "Tempo" },
        fr: { t: "Tempo", sameAsEn: true, reviewed: true },
    },
    'label.knobPatternLength': {
        en: { t: "Pattern Length" },
        fr: { t: "Longueur du motif", reviewed: true },
    },
    'label.groupVoices': {
        en: { t: "Drum Voices" },
        fr: { t: "Voix de batterie", reviewed: true },
    },
    'label.groupRoute': {
        en: { t: "tune · decay · tone · level · mute / solo" },
        fr: { t: "accord · déclin · timbre · niveau · muet / solo", reviewed: true },
    },
    'label.groupMaster': {
        en: { t: "Master" },
        fr: { t: "Général", reviewed: true },
    },
    'label.knobOutput': {
        en: { t: "Output" },
        fr: { t: "Sortie", reviewed: true },
    },

    // ── The six voice names ─────────────────────────────────────────────────
    // Written by six one-line setLabel writers, each with a plain literal key.
    // They land in TWO places per voice — the grid row label and the voice
    // strip's own heading — and they are also the {voice} token of the four
    // step-cell accessible names.
    'label.voiceKick': {
        en: { t: "Kick" },
        fr: { t: "Grosse caisse", reviewed: true },
    },
    'label.voiceSnare': {
        en: { t: "Snare" },
        fr: { t: "Caisse claire", reviewed: true },
    },
    'label.voiceClap': {
        en: { t: "Clap" },
        fr: { t: "Clap", sameAsEn: true, reviewed: true },
    },
    'label.voiceClosedHat': {
        en: { t: "Closed Hat" },
        fr: { t: "Charley fermé", reviewed: true },
    },
    'label.voiceOpenHat': {
        en: { t: "Open Hat" },
        fr: { t: "Charley ouvert", reviewed: true },
    },
    'label.voiceTom': {
        en: { t: "Tom" },
        fr: { t: "Tom", sameAsEn: true, reviewed: true },
    },

    // ── The four repeating voice-knob captions and the two toggles ──────────
    // Keyed ONCE each and applied to all six strips by the same writers, rather
    // than six copies of one key. Their tooltips are voiceTune / voiceDecay /
    // voiceTone / voiceLevel / voiceMute / voiceSolo above, which happen to hold
    // the same words today and are deliberately still separate entries.
    'label.knobTune': {
        en: { t: "Tune" },
        fr: { t: "Accord", reviewed: true },
    },
    'label.knobDecay': {
        en: { t: "Decay" },
        fr: { t: "Déclin", reviewed: true },
    },
    'label.knobTone': {
        en: { t: "Tone" },
        fr: { t: "Timbre", reviewed: true },
    },
    'label.knobLevel': {
        en: { t: "Level" },
        fr: { t: "Niveau", reviewed: true },
    },
    'label.mute': {
        en: { t: "Mute" },
        fr: { t: "Muet", reviewed: true },
    },
    'label.solo': {
        en: { t: "Solo" },
        fr: { t: "Solo", sameAsEn: true, reviewed: true },
    },

    // ── Lesson tour ─────────────────────────────────────────────────────────
    // The six BUTTON FACES are not here: they are factory preset names and are
    // I18N_EXEMPT under D-02, for the reason recorded on the lesson tips above.
    'label.tourHeading': {
        en: { t: "Lesson Presets" },
        fr: { t: "Préréglages de leçon", reviewed: true },
    },
    'label.tourSoon': {
        en: { t: "(click one — each isolates a single idea)" },
        fr: { t: "(cliquez-en un — chacun isole une seule idée)", reviewed: true },
    },
    'label.tourCaption': {
        en: { t: "Hover any control for a plain-language explanation · click a lesson preset to load it, then tweak a knob." },
        fr: { t: "Survolez une commande pour une explication en langage clair · cliquez sur un préréglage de leçon pour le charger, puis tournez un bouton.", reviewed: true },
    },
    // {name} is the preset name the button carries and substitutes VERBATIM: it
    // is the name the lesson row shows and the name C++ knows (D-02).
    'label.tourLoaded': {
        en: { t: "“{name}” loaded — tweak a knob to hear the concept." },
        fr: { t: "« {name} » chargé — tournez un bouton pour entendre le concept.", reviewed: true },
    },

    // ── The settings popover ────────────────────────────────────────────────
    // The popover's visible caption reads the 'lang-select' TOOLTIP title
    // through trLabel's I18N fallback — the one deliberate use of that fallback
    // for a caption, because the selector's name and its tip title are the same
    // word by construction.
    'aria.langSelect': {
        en: { t: "Interface language" },
        fr: { t: "Langue de l’interface", reviewed: true },
    },
});

// ─────────────────────────────────────────────────────────────── bindings ──
// [selector, I18N key]. document.querySelector returns the FIRST match in
// document order, so every repeating anchor is named INDIVIDUALLY: the 24 voice
// knob cells by their data-param (the parameter ID), the 12 mute/solo buttons by
// the id bindToggle already needs, and the six lesson chips by their
// data-preset. A '.knob-cell' style selector would put all six Tune tips on the
// Kick strip — the failure O-Octagon's .vunit-group nearly shipped.
export const TIP_BINDINGS = [
    ['#gear-btn',                            'gear-btn'],
    ['#lang-select',                         'lang-select'],

    ['#gridPanel',                           'grid'],
    ['#lanePanel',                           'lane'],
    ['#midiPanel',                           'midi'],
    ['#clearGridBtn',                        'clearGrid'],

    ['[data-param="swing"]',                 'swing'],
    ['[data-param="humanize"]',              'humanize'],
    ['[data-param="quantizeStrength"]',      'quantizeStrength'],
    ['[data-param="tempo"]',                 'tempo'],
    ['[data-param="patternLength"]',         'patternLength'],
    ['[data-param="outputLevel"]',           'outputLevel'],

    ['[data-param="kickTune"]',              'voiceTune'],
    ['[data-param="kickDecay"]',             'voiceDecay'],
    ['[data-param="kickTone"]',              'voiceTone'],
    ['[data-param="kickLevel"]',             'voiceLevel'],
    ['#toggle-kickMute',                     'voiceMute'],
    ['#toggle-kickSolo',                     'voiceSolo'],

    ['[data-param="snareTune"]',             'voiceTune'],
    ['[data-param="snareDecay"]',            'voiceDecay'],
    ['[data-param="snareTone"]',             'voiceTone'],
    ['[data-param="snareLevel"]',            'voiceLevel'],
    ['#toggle-snareMute',                    'voiceMute'],
    ['#toggle-snareSolo',                    'voiceSolo'],

    ['[data-param="clapTune"]',              'voiceTune'],
    ['[data-param="clapDecay"]',             'voiceDecay'],
    ['[data-param="clapTone"]',              'voiceTone'],
    ['[data-param="clapLevel"]',             'voiceLevel'],
    ['#toggle-clapMute',                     'voiceMute'],
    ['#toggle-clapSolo',                     'voiceSolo'],

    ['[data-param="closedHatTune"]',         'voiceTune'],
    ['[data-param="closedHatDecay"]',        'voiceDecay'],
    ['[data-param="closedHatTone"]',         'voiceTone'],
    ['[data-param="closedHatLevel"]',        'voiceLevel'],
    ['#toggle-closedHatMute',                'voiceMute'],
    ['#toggle-closedHatSolo',                'voiceSolo'],

    ['[data-param="openHatTune"]',           'voiceTune'],
    ['[data-param="openHatDecay"]',          'voiceDecay'],
    ['[data-param="openHatTone"]',           'voiceTone'],
    ['[data-param="openHatLevel"]',          'voiceLevel'],
    ['#toggle-openHatMute',                  'voiceMute'],
    ['#toggle-openHatSolo',                  'voiceSolo'],

    ['[data-param="tomTune"]',               'voiceTune'],
    ['[data-param="tomDecay"]',              'voiceDecay'],
    ['[data-param="tomTone"]',               'voiceTone'],
    ['[data-param="tomLevel"]',              'voiceLevel'],
    ['#toggle-tomMute',                      'voiceMute'],
    ['#toggle-tomSolo',                      'voiceSolo'],

    ['#presetTour',                          'presets'],
    ['[data-preset="Straight"]',             'lessonStraight'],
    ['[data-preset="Backbeat + Accents"]',   'lessonAccents'],
    ['[data-preset="Ghost Notes"]',          'lessonGhost'],
    ['[data-preset="Triplet Swing"]',        'lessonSwing'],
    ['[data-preset="Humanized"]',            'lessonHumanized'],
    ['[data-preset="Quantize Demo"]',        'lessonQuantize'],
];

// ─────────────────────────────────────────────────────────────── exempt ──
export const I18N_EXEMPT = [
    // The h1 splits the product name across two text nodes so the second half
    // can carry the green .title-accent. Both halves are the same untranslatable
    // name; keying either would translate half a wordmark.
    //
    // WRITTEN IN THE COLLAPSED FORM. The markup authors hair spaces either side
    // of the en dash, but the coverage scan trims and collapses whitespace
    // before it compares, so the hair-spaced original would never match.
    ['O – simple',
     'the product name, first half of the split wordmark in the page heading — a product name is never translated'],
    ['Beatmaker',
     'the product name, second half of the split wordmark (.title-accent) — a product name is never translated. No other text node on this page carries this word, so exempting it hides nothing'],

    // ── The six lesson-preset button faces (D-02) ───────────────────────────
    // Each is the name of a factory preset in the C++ kBeatPresets[] table, and
    // each is printed back verbatim into the tour caption through the {name}
    // token the moment it is clicked. Translating the face would make the button
    // and the caption disagree about what was just loaded, which is the
    // page-versus-preset disagreement D-02 exists to prevent. The tip TITLES
    // above stay English for the same reason.
    //
    // None of the six collides with another text node on this page: the knob
    // caption is "Quantize", not "Quantize Demo", and "Humanize", not
    // "Humanized". Checked because an exemption matches on TEXT with no
    // selector, so a collision would silently cover a real label.
    ['Straight',           'factory preset name — the tour caption prints it back verbatim (D-02)'],
    ['Backbeat + Accents', 'factory preset name — the tour caption prints it back verbatim (D-02)'],
    ['Ghost Notes',        'factory preset name — the tour caption prints it back verbatim (D-02)'],
    ['Triplet Swing',      'factory preset name — the tour caption prints it back verbatim (D-02)'],
    ['Humanized',          'factory preset name — the tour caption prints it back verbatim (D-02)'],
    ['Quantize Demo',      'factory preset name — the tour caption prints it back verbatim (D-02)'],

    // The two endonyms in the language selector. A language name is written in
    // its OWN language: a French speaker looking for their language looks for
    // "Français", not "French".
    ['English',  'endonym — a language name is never translated'],
    ['Français', 'endonym — a language name is never translated'],
];

export function tr(key, lang, vars) {
    const entry = I18N[key];
    if (!entry) { console.warn(`i18n: missing key ${key}`); return { t: key, b: '' }; }
    const s = entry[lang] || entry.en;

    // A var VALUE that is itself an I18N key resolves to that key's localized
    // title; anything else is used literally. This plugin passes no vars to a
    // TOOLTIP at all today, so only the literal arm runs here. The resolving arm
    // is what lets a plugin compose a localized name into a tip without pinning
    // TIP_BINDINGS — which is static data evaluated once — to the load-time
    // language. The canon is one shape across all 43 plugins; this function is
    // not trimmed per plugin.
    const resolve = (v) => {
        const nested = I18N[v];
        return nested ? String((nested[lang] || nested.en).t) : String(v);
    };

    const sub = (v) => vars
        ? String(v).replace(/\{(\w+)\}/g, (m, n) => (n in vars ? resolve(vars[n]) : m))
        : String(v);

    return { t: sub(s.t), b: sub(s.b) };
}
