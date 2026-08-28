/*
   This file is part of O-simpleFM, an Ouaricon Audio plugin.
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
// i18n.js — O-simpleFM interface copy, English + French (v1.3.0)
//
// UI ROOT IS Source/ui/public. There is no second UI root in this plugin. This
// file is a SOURCES entry in juce_add_binary_data(O-simpleFM_UIResources) and is
// served by PluginEditor::getResource at /js/i18n.js. Embedded but not served,
// or served but not embedded, is a 404 that presents as a BLANK page rather than
// an English one — the import in app.js fails to resolve and module evaluation
// never starts. check-i18n assertion 8 checks both halves.
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
// COPY IS textContent ON EVERY PATH — never innerHTML. Through v1.2.5 the
// tooltip renderer built its tip with tip.innerHTML = ...; v1.3.0 builds it with
// createElement + textContent, because the tip text is table-sourced and
// localized now rather than a fixed literal. check-i18n assertion 9 rejects any
// innerHTML reference here and any string literal containing an opening angle
// bracket.
//
// THE ENGLISH WAS MOVED, NOT REWRITTEN. Every en entry below was extracted
// mechanically from v1.2.5's TIPS and LESSONS tables in js/app.js and from
// index.html, then compared back to the source with entities decoded — never
// re-typed. HTML entities are decoded to the characters they named (&#183; to ·,
// &#8594; to →, &#8202; to a \u200a escape and &#8203; to a \u200b escape, NOT to
// plain spaces) because setAttribute and textContent do not decode entities.
// Invisible characters are written as ESCAPES, never as literal codepoints: a
// literal U+200A survives a heredoc, works at runtime and is invisible in
// review, which is the whole failure mode.
//
// THREE DELIBERATE ENGLISH CHANGES, all recorded in the CHANGELOG:
//   1. The tip bodies have lost their strong/em tags. The WORDS are unchanged.
//      Assertion 9 forbids an angle bracket in a string literal here, and it is
//      right to: the renderer writes textContent, so a tag would render as
//      literal characters rather than as emphasis.
//   2. The routing meta line was "harmonic · ≈ 1 sideband" / "≈ 4 sidebands",
//      an inline English plural. Contract §6 forbids engineering plural
//      inflection for one plugin, so the count moved to the END of a
//      count-neutral phrase that reads correctly at 1 and at n in both
//      languages.
//   3. The carrier-null badge's explanation was a native title=. Contract §4
//      deletes native titles; this page already owns a tooltip renderer, so the
//      copy moved into it verbatim rather than becoming an aria-label nobody
//      would ever hear.
//
// KEYS ARE THE PARAMETER ID where the anchor is a parameter cell, and a
// label.* / aria.* / ui.* slug otherwise. The parameter cells are addressed by a
// data-param attribute ADDED IN v1.3.0; through v1.2.5 they carried the tip KEY
// in their own data-tip attribute, and a bare .knob-cell selector would have
// matched the FIRST of fifteen — the failure canon section 1 names and the one
// O-Octagon's .vunit-group tip hit for real in Stage C.
//
// LABELS NEVER REUSE A TOOLTIP KEY for a caption. trLabel() falls back to I18N,
// and three of this page's captions do happen to equal their tip title today
// ("Feedback", "Ratio Snap", "Fixed Mode") — but most do not ("Level" vs
// "Output Level", "Fixed Hz" vs "Fixed Modulator Hz", "Ratio C:M" vs
// "Ratio (C : M)"), and a rule that holds for three of nineteen is a rule nobody
// can apply. A caption and a tip title also diverge the moment either is edited,
// which would make a tooltip copy edit a silent geometry change to a control.
// The ONE place the fallback is used on purpose is data-i18n-aria on the fifteen
// knobs, the two toggles and the two hoverable panels: an accessible name IS the
// control's name, so it reads the tooltip title by design.
//
// THE FIVE LESSON BUTTON FACES STAY ENGLISH, and they are the one place this
// plugin departs from O-simpleGrain, which translated its chips. Here the face
// IS the factory preset name, and this plugin — unlike O-simpleGrain — DISPLAYS
// that name in its header preset bar. Translating the face would put "Piano
// électrique" on a button that loads a preset the bar above it calls "E-Piano",
// which is exactly the page-versus-preset disagreement D-02 exists to prevent.
// The five are I18N_EXEMPT with that reason; their tooltips are translated.
//
// ALL FRENCH IS MACHINE-DRAFTED AND FLAGGED reviewed: false. No native speaker
// has read it. node scripts/check-i18n.js prints the worklist.
// ============================================================================

export const LANGUAGES = ['en', 'fr'];

export const I18N = Object.freeze({

    // ── The settings popover (v1.3.0) ───────────────────────────────────────
    // The gear is new. This plugin has no hover-help bridge and never had a "?"
    // toggle — not a C++ one, not a localStorage one — so the panel holds the
    // LANGUAGE SELECTOR ALONE. A toggle row here would be a control for a
    // preference that does not exist.
    'gear-btn': {
        en: { t: "Settings",
              b: "Choose the language of the interface. The choice is remembered with the session, so a project reopens in the language it was saved in." },
        fr: { t: "Réglages",
              b: "Choisir la langue de l'interface. Le choix est conservé avec la session : un projet se rouvre dans la langue où il a été enregistré.",
              reviewed: false },
    },
    'lang-select': {
        en: { t: "Language",
              b: "The language of the labels on this page and of this hover help. English and French are available; value readouts, the preset names and the lesson buttons stay in English." },
        fr: { t: "Langue",
              b: "La langue des libellés de cette page et de cette aide au survol. L'anglais et le français sont disponibles ; les valeurs affichées, les noms de préréglages et les boutons de leçon restent en anglais.",
              reviewed: false },
    },

    // ── Operators ───────────────────────────────────────────────────────────
    ratio: {
        en: { t: "Ratio (C : M)",
              b: "Frequency of the modulator relative to the carrier. Whole-number ratios (1, 2, 3...) give harmonic, pitched timbres; irrational ratios (1.41, 2.76) give inharmonic, bell-like tones." },
        fr: { t: "Rapport (P : M)",
              b: "Fréquence du modulateur par rapport à la porteuse. Les rapports entiers (1, 2, 3...) donnent des timbres harmoniques et clairement accordés ; les rapports irrationnels (1,41 ; 2,76) donnent des sons inharmoniques, proches de la cloche.",
              reviewed: false },
    },
    modIndex: {
        en: { t: "Modulation Index",
              b: "How hard the modulator bends the carrier's phase. Zero = pure sine. Raising it grows more and louder sidebands — the core of FM brightness." },
        fr: { t: "Indice de modulation",
              b: "À quel point le modulateur infléchit la phase de la porteuse. Zéro = sinusoïde pure. En l'augmentant, les bandes latérales deviennent plus nombreuses et plus fortes — le cœur de la brillance FM.",
              reviewed: false },
    },
    feedback: {
        en: { t: "Feedback",
              b: "Routes the modulator back into itself. Pushes the modulator's shape from sine toward sawtooth, then toward noise — smearing the spectrum." },
        fr: { t: "Réinjection",
              b: "Renvoie le modulateur dans lui-même. Fait passer sa forme d'onde de la sinusoïde vers la dent de scie, puis vers le bruit — ce qui étale le spectre.",
              reviewed: false },
    },
    modFixedHz: {
        en: { t: "Fixed Modulator Hz",
              b: "When Fixed Mode is on, the modulator runs at this absolute frequency instead of tracking the played note — producing formant-like, key-independent colour." },
        fr: { t: "Fréquence fixe du modulateur",
              b: "Quand le mode fixe est actif, le modulateur tourne à cette fréquence absolue au lieu de suivre la note jouée — d'où une couleur de type formant, indépendante du clavier.",
              reviewed: false },
    },
    modEnvToIndex: {
        en: { t: "Env → Index Depth",
              b: "How much the modulator envelope drives the index over time. This makes the timbre evolve after the key is struck (bright attack → mellow tail)." },
        fr: { t: "Profondeur env. → indice",
              b: "Dans quelle mesure l'enveloppe du modulateur pilote l'indice au fil du temps. C'est ce qui fait évoluer le timbre après la frappe (attaque brillante → fin de son adoucie).",
              reviewed: false },
    },
    velToIndex: {
        en: { t: "Velocity → Index",
              b: "Lets how hard you play add to the modulation index — harder strikes become brighter, like an acoustic instrument." },
        fr: { t: "Vélocité → indice",
              b: "Laisse la force de jeu s'ajouter à l'indice de modulation — plus la frappe est forte, plus le son est brillant, comme sur un instrument acoustique.",
              reviewed: false },
    },

    // ── Modulator envelope ──────────────────────────────────────────────────
    modAttack: {
        en: { t: "Mod Attack",
              b: "Time for the modulator (brightness) envelope to rise after note-on." },
        fr: { t: "Attaque mod.",
              b: "Temps que met l'enveloppe du modulateur (la brillance) à monter après le début de la note.",
              reviewed: false },
    },
    modDecay: {
        en: { t: "Mod Decay",
              b: "Time for the modulator envelope to fall from peak to its sustain level." },
        fr: { t: "Chute mod.",
              b: "Temps que met l'enveloppe du modulateur à descendre du sommet vers son niveau de maintien.",
              reviewed: false },
    },
    modSustain: {
        en: { t: "Mod Sustain",
              b: "Held brightness level while the key stays down." },
        fr: { t: "Maintien mod.",
              b: "Niveau de brillance tenu tant que la touche reste enfoncée.",
              reviewed: false },
    },
    modRelease: {
        en: { t: "Mod Release",
              b: "Time for brightness to fade after the key is released." },
        fr: { t: "Relâche mod.",
              b: "Temps que met la brillance à s'éteindre après le relâchement de la touche.",
              reviewed: false },
    },

    // ── Amplitude envelope ──────────────────────────────────────────────────
    ampAttack: {
        en: { t: "Amp Attack",
              b: "Time for loudness to rise after note-on." },
        fr: { t: "Attaque ampl.",
              b: "Temps que met le volume à monter après le début de la note.",
              reviewed: false },
    },
    ampDecay: {
        en: { t: "Amp Decay",
              b: "Time for loudness to fall from peak to its sustain level." },
        fr: { t: "Chute ampl.",
              b: "Temps que met le volume à descendre du sommet vers son niveau de maintien.",
              reviewed: false },
    },
    ampSustain: {
        en: { t: "Amp Sustain",
              b: "Held loudness while the key stays down." },
        fr: { t: "Maintien ampl.",
              b: "Volume tenu tant que la touche reste enfoncée.",
              reviewed: false },
    },
    ampRelease: {
        en: { t: "Amp Release",
              b: "Time for loudness to fade after the key is released — also sets how long the voice rings out." },
        fr: { t: "Relâche ampl.",
              b: "Temps que met le volume à s'éteindre après le relâchement de la touche — cela fixe aussi la durée pendant laquelle la voix continue de sonner.",
              reviewed: false },
    },

    // ── Output ──────────────────────────────────────────────────────────────
    outputLevel: {
        en: { t: "Output Level",
              b: "Master output trim in decibels." },
        fr: { t: "Niveau de sortie",
              b: "Ajustement du niveau général de sortie, en décibels.",
              reviewed: false },
    },

    // ── Toggles ─────────────────────────────────────────────────────────────
    ratioSnap: {
        en: { t: "Ratio Snap",
              b: "Quantises the C:M ratio to whole numbers — instantly snaps an inharmonic tone to a harmonic one." },
        fr: { t: "Rapport entier",
              b: "Arrondit le rapport P:M à des nombres entiers — ramène instantanément un son inharmonique vers un son harmonique.",
              reviewed: false },
    },
    modFixedMode: {
        en: { t: "Fixed Mode",
              b: "Switches the modulator from tracking the note (Ratio) to a fixed frequency in Hz (set by Fixed Hz)." },
        fr: { t: "Mode fixe",
              b: "Fait passer le modulateur du suivi de la note (rapport) à une fréquence fixe en Hz (réglée par Fréq. fixe).",
              reviewed: false },
    },

    // ── The routing diagram, its readout and its teaching badge ─────────────
    // MOD and CAR are the node captions in the SVG, so the tip has to name the
    // same three-letter abbreviations the diagram shows. The French node reads
    // POR (porteuse), so the French tip says POR.
    routing: {
        en: { t: "Signal Path",
              b: "MOD modulates the phase of CAR; MOD's self-loop is Feedback. Arrow thickness reflects Mod Index and Feedback amount." },
        fr: { t: "Chemin du signal",
              b: "MOD module la phase de POR ; la boucle de MOD sur lui-même est la réinjection. L'épaisseur des flèches reflète l'indice de modulation et le taux de réinjection.",
              reviewed: false },
    },
    readout: {
        en: { t: "Live FM Readout",
              b: "The two numbers that define the tone, updating as you play. Left — the C : M ratio: the modulator's frequency relative to the played note, which sets which harmonics appear (whole numbers = pitched, irrational = bell-like). Right — I, the modulation index: how hard the modulator bends the carrier, which sets the brightness (more index = more sidebands)." },
        fr: { t: "Affichage FM en direct",
              b: "Les deux nombres qui définissent le timbre, mis à jour pendant le jeu. À gauche — le rapport P : M, la fréquence du modulateur par rapport à la note jouée, qui décide quelles harmoniques apparaissent (entiers = son accordé, irrationnels = son de cloche). À droite — I, l'indice de modulation : à quel point le modulateur infléchit la porteuse, ce qui décide la brillance (indice plus élevé = plus de bandes latérales).",
              reviewed: false },
    },
    // MOVED from the native title= on #carrierNullBadge, verbatim, entities
    // decoded. The zero-width space inside "f\u200bc" is authored: it lets the
    // subscript read as one token without a ligature.
    carrierNull: {
        en: { t: "Carrier null",
              b: "Carrier null: the modulation index sits at the first Bessel J₀ zero (β ≈ 2.405), so the carrier (f\u200bc) vanishes and all energy moves into the sidebands." },
        fr: { t: "Extinction de la porteuse",
              b: "Extinction de la porteuse : l'indice de modulation se trouve au premier zéro de la fonction de Bessel J₀ (β ≈ 2,405), donc la porteuse (f\u200bc) disparaît et toute l'énergie passe dans les bandes latérales.",
              reviewed: false },
    },

    // ── Lesson presets ──────────────────────────────────────────────────────
    // The titles keep the English preset NAME and translate only the phrase
    // after it, for the same reason the button faces are exempt below.
    lessonEpiano: {
        en: { t: "E-Piano · how it's built",
              b: "Carrier:modulator 1:1 (harmonic). A fast mod-envelope (decay 0.45 s → zero sustain) sweeps the index down from 5.5, so a bright pluck attack collapses to a near-pure sine as it rings. Velocity adds index — strike harder, sound brighter." },
        fr: { t: "E-Piano · comment il est fait",
              b: "Porteuse:modulateur 1:1 (harmonique). Une enveloppe de modulation rapide (chute 0,45 s → maintien nul) fait descendre l'indice depuis 5,5 : une attaque pincée brillante s'effondre vers une sinusoïde presque pure pendant que la note sonne. La vélocité ajoute de l'indice — plus fort on frappe, plus c'est brillant.",
              reviewed: false },
    },
    lessonTubular: {
        en: { t: "Tubular Bell · how it's built",
              b: "Inharmonic ratio 1.41 (≈√2, snap off) puts sidebands at non-integer multiples, so the partials never fuse into a pitch — that's the metallic ring. High index (8) plus long ≈3 s decays let it shimmer out." },
        fr: { t: "Tubular Bell · comment il est fait",
              b: "Un rapport inharmonique de 1,41 (≈√2, arrondi désactivé) place les bandes latérales à des multiples non entiers : les partiels ne fusionnent jamais en une hauteur claire — d'où la sonnerie métallique. Un indice élevé (8) et de longues chutes d'environ 3 s le laissent scintiller jusqu'au bout.",
              reviewed: false },
    },
    lessonBrass: {
        en: { t: "Brass · how it's built",
              b: "Carrier:modulator 1:1 (harmonic). The index (4) swells in with the attack and holds at sustain — brightness tracks loudness, the way a blown brass note brightens as it gets louder." },
        fr: { t: "Brass · comment il est fait",
              b: "Porteuse:modulateur 1:1 (harmonique). L'indice (4) enfle avec l'attaque et se maintient — la brillance suit le volume, comme une note de cuivre qui s'éclaircit à mesure qu'on souffle plus fort.",
              reviewed: false },
    },
    lessonClarinet: {
        en: { t: "Clarinet · how it's built",
              b: "Carrier:modulator 2:1. A low index (2.2) keeps the spectrum sparse, and the 2:1 ratio emphasises odd harmonics → the hollow, stopped-pipe woody tone. High sustain, so it speaks steadily like a reed." },
        fr: { t: "Clarinet · comment il est fait",
              b: "Porteuse:modulateur 2:1. Un indice faible (2,2) garde le spectre clairsemé, et le rapport 2:1 met en avant les harmoniques impaires → le timbre creux et boisé du tuyau bouché. Maintien élevé : le son parle de façon régulière, comme une anche.",
              reviewed: false },
    },
    lessonClang: {
        en: { t: "Clang Bell · how it's built",
              b: "Inharmonic ratio 3.46 plus a very high index (14) throw a dense thicket of non-integer sidebands; 60% feedback bends the modulator toward noise. The spectrum smears into an atonal clang rather than a pitch." },
        fr: { t: "Clang Bell · comment il est fait",
              b: "Un rapport inharmonique de 3,46 et un indice très élevé (14) projettent un fourré dense de bandes latérales non entières ; 60 % de réinjection pousse le modulateur vers le bruit. Le spectre s'étale en un fracas atonal plutôt qu'en une hauteur.",
              reviewed: false },
    },
});

// ============================================================================
// LABELS — one string per key, no body. A label is not a tooltip.
// ============================================================================

export const LABELS = Object.freeze({

    // ── Header ──────────────────────────────────────────────────────────────
    'label.subtitle': {
        en: { t: "Two-Operator Phase-Modulation Synthesizer · A Field Guide" },
        fr: { t: "Synthétiseur à modulation de phase à deux opérateurs · un guide de terrain", reviewed: false },
    },
    // The preset bar. The NAME shown between them is a preset name and is never
    // translated (D-02); these two are verbs on buttons and are.
    'label.presetSave': {
        en: { t: "Save" },
        fr: { t: "Enreg.", reviewed: false },
    },
    'label.presetDelete': {
        en: { t: "Delete" },
        fr: { t: "Suppr.", reviewed: false },
    },

    // ── Visualization captions ──────────────────────────────────────────────
    // The middot belongs to the caption, not to the hint: it is the separator
    // the caption ends with in both languages.
    'label.vizSpectrum': {
        en: { t: "Spectrum ·" },
        fr: { t: "Spectre ·", reviewed: false },
    },
    'label.vizSpectrumHint': {
        en: { t: "discrete sidebands bloom as Mod Index rises" },
        fr: { t: "les bandes latérales discrètes éclosent quand l'indice monte", reviewed: false },
    },
    'label.vizWaveform': {
        en: { t: "Waveform ·" },
        fr: { t: "Forme d'onde ·", reviewed: false },
    },
    // SHORTENED, and flagged for the reviewer. The faithful "la forme de la
    // porteuse qui en résulte" measures 226.1px against the ~165px this hint has
    // beside its caption in a 240px viz block, so it wrapped to a second line and
    // shrank the scope canvas 11px in French only. "shape" is dropped; the
    // caption directly to its left already reads "Forme d'onde", so the phrase
    // still says what the picture is. The alternative — reserving the second line
    // in BOTH languages — costs 11px of English page height on a frame that
    // already scrolls, so it was not taken.
    'label.vizWaveformHint': {
        en: { t: "the resulting carrier shape" },
        fr: { t: "la porteuse qui en résulte", reviewed: false },
    },

    // ── Routing diagram ─────────────────────────────────────────────────────
    'label.signalPath': {
        en: { t: "Signal Path" },
        fr: { t: "Chemin du signal", reviewed: false },
    },
    // The two operator nodes. Three letters each in BOTH languages, on purpose:
    // they sit inside 48 px SVG circles and the French abbreviation of
    // "modulateur" is the same three letters as the English. "POR" is
    // "porteuse"; the routing and readout tips spell both out.
    'label.opMod': {
        en: { t: "MOD" },
        fr: { t: "MOD", sameAsEn: true, reviewed: false },
    },
    'label.opCar': {
        en: { t: "CAR" },
        fr: { t: "POR", reviewed: false },
    },
    'label.carrierNull': {
        en: { t: "carrier null" },
        fr: { t: "porteuse nulle", reviewed: false },
    },
    // The routing meta line, composed. Through v1.2.5 this read
    // "harmonic · ≈ 1 sideband" / "≈ 4 sidebands" — an inline English plural.
    // Contract §6 forbids engineering plural inflection for one plugin, so the
    // count moved to the END of a count-neutral phrase that is correct at 1 and
    // at n in both languages. Two keys rather than one with a {kind} token: a
    // dispatch of two literal-keyed writers is what assertion 13 can check, and
    // what a reviewer can read.
    'label.metaHarmonic': {
        en: { t: "harmonic · sidebands ≈ {n}" },
        fr: { t: "harmonique · bandes latérales ≈ {n}", reviewed: false },
    },
    'label.metaInharmonic': {
        en: { t: "inharmonic · sidebands ≈ {n}" },
        fr: { t: "inharmonique · bandes latérales ≈ {n}", reviewed: false },
    },

    // ── Group headings ──────────────────────────────────────────────────────
    'label.groupOperators': {
        en: { t: "Operators" },
        fr: { t: "Opérateurs", reviewed: false },
    },
    'label.groupModEnv': {
        en: { t: "Modulator Envelope" },
        fr: { t: "Enveloppe du modulateur", reviewed: false },
    },
    'label.groupAmpEnv': {
        en: { t: "Amplitude Envelope" },
        fr: { t: "Enveloppe d'amplitude", reviewed: false },
    },
    'label.groupOutput': {
        en: { t: "Output" },
        fr: { t: "Sortie", reviewed: false },
    },

    // ── Knob captions ───────────────────────────────────────────────────────
    // The four envelope captions are ONE key each, shared by the modulator and
    // the amplitude rack: the two racks show the same four words, and two keys
    // per word would be two translations to keep in step.
    'label.knobRatio': {
        en: { t: "Ratio C:M" },
        fr: { t: "Rapport P:M", reviewed: false },
    },
    'label.knobModIndex': {
        en: { t: "Mod Index" },
        fr: { t: "Indice mod.", reviewed: false },
    },
    'label.knobFeedback': {
        en: { t: "Feedback" },
        fr: { t: "Réinjection", reviewed: false },
    },
    'label.knobFixedHz': {
        en: { t: "Fixed Hz" },
        fr: { t: "Fréq. fixe", reviewed: false },
    },
    'label.knobEnvIndex': {
        en: { t: "Env→Index" },
        fr: { t: "Env→Indice", reviewed: false },
    },
    'label.knobVelIndex': {
        en: { t: "Vel→Index" },
        fr: { t: "Véloc→Indice", reviewed: false },
    },
    'label.knobAttack': {
        en: { t: "Attack" },
        fr: { t: "Attaque", reviewed: false },
    },
    'label.knobDecay': {
        en: { t: "Decay" },
        fr: { t: "Chute", reviewed: false },
    },
    'label.knobSustain': {
        en: { t: "Sustain" },
        fr: { t: "Maintien", reviewed: false },
    },
    'label.knobRelease': {
        en: { t: "Release" },
        fr: { t: "Relâche", reviewed: false },
    },
    'label.knobLevel': {
        en: { t: "Level" },
        fr: { t: "Niveau", reviewed: false },
    },

    // ── Toggle faces ────────────────────────────────────────────────────────
    'label.toggleRatioSnap': {
        en: { t: "Ratio Snap" },
        fr: { t: "Rapport entier", reviewed: false },
    },
    'label.toggleFixedMode': {
        en: { t: "Fixed Mode" },
        fr: { t: "Mode fixe", reviewed: false },
    },

    // ── Lesson tour ─────────────────────────────────────────────────────────
    'label.lessonPresets': {
        en: { t: "Lesson Presets" },
        fr: { t: "Leçons", reviewed: false },
    },
    'label.tourCaption': {
        en: { t: "Hover any control for an explanation · pick a lesson to hear a concept." },
        fr: { t: "Survolez un réglage pour une explication · choisissez une leçon pour entendre une notion.", reviewed: false },
    },
    // The five captions the tour buttons write. Each begins with the preset
    // NAME, which stays English for the same reason the button faces do.
    'label.captionEpiano': {
        en: { t: "E-Piano — ratio 1:1 + a fast mod-envelope makes a bright pluck that mellows to a sine." },
        fr: { t: "E-Piano — rapport 1:1 et une enveloppe de modulation rapide : un pincement brillant qui s'adoucit vers une sinusoïde.", reviewed: false },
    },
    'label.captionTubular': {
        en: { t: "Tubular Bell — an inharmonic ratio (1.41) sprays non-integer sidebands → metallic ring." },
        fr: { t: "Tubular Bell — un rapport inharmonique (1,41) projette des bandes latérales non entières → sonnerie métallique.", reviewed: false },
    },
    'label.captionBrass': {
        en: { t: "Brass — ratio 1:1, index rises with the amp envelope; sustained, vowel-bright." },
        fr: { t: "Brass — rapport 1:1, l'indice monte avec l'enveloppe d'amplitude ; tenu, brillant comme une voyelle.", reviewed: false },
    },
    'label.captionClarinet': {
        en: { t: "Clarinet — ratio 2:1 + low index emphasises odd harmonics → hollow, woody tone." },
        fr: { t: "Clarinet — rapport 2:1 et indice faible mettent en avant les harmoniques impaires → timbre creux et boisé.", reviewed: false },
    },
    'label.captionClang': {
        en: { t: "Clang Bell — high index + feedback smears the spectrum into a dense, noisy strike." },
        fr: { t: "Clang Bell — indice élevé et réinjection étalent le spectre en une frappe dense et bruitée.", reviewed: false },
    },

    // ── On-screen keyboard ──────────────────────────────────────────────────
    'label.play': {
        en: { t: "Play ·" },
        fr: { t: "Jouer ·", reviewed: false },
    },
    // The letter run is the QWERTY row the page listens for, separated by hair
    // spaces. The letters are key NAMES on the user's own keyboard and are not
    // translated; only the sentence around them is.
    'label.kbdHint': {
        en: { t: "click the keys or use your computer keyboard (A\u200aS\u200aD\u200aF\u200aG\u200aH\u200aJ\u200aK · W\u200aE\u200aT\u200aY\u200aU)" },
        fr: { t: "cliquez les touches ou utilisez le clavier de l'ordinateur (A\u200aS\u200aD\u200aF\u200aG\u200aH\u200aJ\u200aK · W\u200aE\u200aT\u200aY\u200aU)", reviewed: false },
    },

    // ── Built from script ───────────────────────────────────────────────────
    // The two preset-dropdown group headings. Written through a dispatch of two
    // one-line writers, each naming its own literal key — a computed key would
    // fail assertion 13 twice over and could not be checked by anybody.
    'label.presetFactory': {
        en: { t: "Factory" },
        fr: { t: "Usine", reviewed: false },
    },
    'label.presetUser': {
        en: { t: "User" },
        fr: { t: "Utilisateur", reviewed: false },
    },
    // The in-DOM delete confirmation. The preset NAME is substituted, never
    // translated (D-02 — the name is the JSON filename); trLabel resolves a var
    // value that is not itself a key literally, which is what is wanted here.
    'ui.deleteConfirm': {
        en: { t: "Delete preset \"{name}\"?" },
        fr: { t: "Supprimer le préréglage « {name} » ?", reviewed: false },
    },
    'ui.delete': {
        en: { t: "Delete" },
        fr: { t: "Supprimer", reviewed: false },
    },
    'ui.cancel': {
        en: { t: "Cancel" },
        fr: { t: "Annuler", reviewed: false },
    },

    // ── Accessible names ────────────────────────────────────────────────────
    // aria.* keys name things that have no visible caption of their own. The
    // fifteen knobs, the two toggles and the two hoverable panels do NOT appear
    // here: their accessible name IS the control's name, so they read their own
    // tooltip title through trLabel's I18N fallback.
    'aria.settings': {
        en: { t: "Settings" },
        fr: { t: "Réglages", reviewed: false },
    },
    'aria.langSelect': {
        en: { t: "Interface language" },
        fr: { t: "Langue de l'interface", reviewed: false },
    },
    // MOVED from four native title= attributes deleted per contract §4. Two of
    // them (prev/next) duplicated an aria-label that was already there; the
    // other two were the only help those buttons had.
    'aria.presetPrev': {
        en: { t: "Previous preset" },
        fr: { t: "Préréglage précédent", reviewed: false },
    },
    'aria.presetNext': {
        en: { t: "Next preset" },
        fr: { t: "Préréglage suivant", reviewed: false },
    },
    'aria.presetName': {
        en: { t: "Browse presets" },
        fr: { t: "Parcourir les préréglages", reviewed: false },
    },
    'aria.presetSave': {
        en: { t: "Save current settings as a user preset" },
        fr: { t: "Enregistrer les réglages actuels comme préréglage utilisateur", reviewed: false },
    },
    'aria.presetDelete': {
        en: { t: "Delete the current user preset" },
        fr: { t: "Supprimer le préréglage utilisateur actuel", reviewed: false },
    },
    'aria.presetDropdown': {
        en: { t: "Presets" },
        fr: { t: "Préréglages", reviewed: false },
    },
    'aria.keyboard': {
        en: { t: "On-screen keyboard" },
        fr: { t: "Clavier à l'écran", reviewed: false },
    },
});

// ============================================================================
// I18N_EXEMPT — reasoned exclusions, never silence.
//
// Exemption matches on TEXT with no selector, so an entry can silently cover a
// same-worded label elsewhere on the page. Each entry below records whether it
// does.
// ============================================================================

export const I18N_EXEMPT = [
    // The h1 splits the product name across two text nodes so the second half
    // can carry the green italic .title-accent. Both halves are the same
    // untranslatable name; keying either would translate half a wordmark.
    //
    // WRITTEN IN THE COLLAPSED FORM. The markup authors hair spaces either side
    // of the en dash, but the coverage scan trims and collapses whitespace
    // before it compares, so the hair-spaced original would never match.
    ['O – simple',
     'the product name, first half of the split wordmark in the page heading — a product name is never translated'],
    ['FM',
     'the product name, second half of the split wordmark (.title-accent) — a product name is never translated. No other bare "FM" text node exists on this page; the Mod Index caption and the readout tip both spell out more words'],

    // The five lesson-button faces. See the header note: the face IS the factory
    // preset name, and this plugin displays that name in its header preset bar,
    // so translating the face would make the button and the bar disagree about
    // what is loaded. Their TOOLTIPS are translated.
    ['E-Piano',
     'factory preset name — the name IS the JSON filename (D-02), and the header preset bar displays it'],
    ['Tubular Bell',
     'factory preset name — the name IS the JSON filename (D-02), and the header preset bar displays it'],
    ['Brass',
     'factory preset name — the name IS the JSON filename (D-02), and the header preset bar displays it'],
    ['Clarinet',
     'factory preset name — the name IS the JSON filename (D-02), and the header preset bar displays it'],
    ['Clang Bell',
     'factory preset name — the name IS the JSON filename (D-02), and the header preset bar displays it'],

    // The preset-name button's authored face before the manager fills it in.
    ['Default',
     'factory preset name — the name IS the JSON filename (D-02). It is the resting face of the preset button, replaced at init by whatever preset is current'],

    // The two endonyms in the language selector. A language name is written in
    // its OWN language: a French speaker looking for their language looks for
    // "Français", not "French".
    ['English',  'endonym — a language name is never translated'],
    ['Français', 'endonym — a language name is never translated'],
];

// ============================================================================
// TIP_BINDINGS — [selector, key, wrapper?, vars?]
//
// The tip anchor IS the element the selector finds: this page authors its tips
// on the cell rather than on the knob inside it, so no closest(wrapper) walk is
// needed anywhere.
//
// Through v1.2.5 the anchors carried the tip KEY in their own data-tip
// attribute and js/app.js looked the copy up in a TIPS object. That cannot
// survive canon v2 — applyI18n WRITES data-tip as the tip BODY, so the key and
// the copy would fight over one attribute, and check-i18n assertion 3 requires
// index.html to carry zero data-tip literals. The fifteen knob cells are
// addressed by a data-param attribute added in v1.3.0; the two toggles and the
// routing panel, its readout and its badge by an id; the five lesson buttons by
// the data-preset they already had.
//
// A HARD-CODED SELECTOR MOVED WITH THEM. js/app.js:222 read
// '.knob-cell[data-tip="modFixedHz"]' to dim the Fixed Hz cell when Fixed Mode
// is off. That selector matches NOTHING once the anchor moves, and it fails
// silently — no error, the cell simply stops dimming. It now reads
// '.knob-cell[data-param="modFixedHz"]'.
// ============================================================================

export const TIP_BINDINGS = [
    ['#gear-btn',                            'gear-btn'],
    ['#lang-select',                         'lang-select'],

    ['#routingPanel',                        'routing'],
    ['#routingReadout',                      'readout'],
    ['#carrierNullBadge',                    'carrierNull'],

    ['[data-param="ratio"]',                 'ratio'],
    ['[data-param="modIndex"]',              'modIndex'],
    ['[data-param="feedback"]',              'feedback'],
    ['[data-param="modFixedHz"]',            'modFixedHz'],
    ['[data-param="modEnvToIndex"]',         'modEnvToIndex'],
    ['[data-param="velToIndex"]',            'velToIndex'],

    ['#toggle-ratioSnap',                    'ratioSnap'],
    ['#toggle-modFixedMode',                 'modFixedMode'],

    ['[data-param="modAttack"]',             'modAttack'],
    ['[data-param="modDecay"]',              'modDecay'],
    ['[data-param="modSustain"]',            'modSustain'],
    ['[data-param="modRelease"]',            'modRelease'],

    ['[data-param="ampAttack"]',             'ampAttack'],
    ['[data-param="ampDecay"]',              'ampDecay'],
    ['[data-param="ampSustain"]',            'ampSustain'],
    ['[data-param="ampRelease"]',            'ampRelease'],

    ['[data-param="outputLevel"]',           'outputLevel'],

    ['.tour-btn[data-preset="epiano"]',      'lessonEpiano'],
    ['.tour-btn[data-preset="tubular"]',     'lessonTubular'],
    ['.tour-btn[data-preset="brass"]',       'lessonBrass'],
    ['.tour-btn[data-preset="clarinet"]',    'lessonClarinet'],
    ['.tour-btn[data-preset="clang"]',       'lessonClang'],
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
