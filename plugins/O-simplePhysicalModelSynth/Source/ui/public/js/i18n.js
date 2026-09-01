/*
   This file is part of O-simplePhysicalModelSynth, an Ouaricon Audio plugin.
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
// i18n.js — O-simplePhysicalModelSynth interface copy, English + French (v1.2.2)
//
// ── v1.2.2: STAGE O (2026-08-31) — the English defects Stage N found ────────
// item 53: the stringModel dropdown is HIDDEN (index.html `hidden` on its
//   .select-cell; styles.css `.select-cell[hidden]`). The parameter is never
//   load()ed — StringResonator.h is a single-rail Karplus-Strong loop and the
//   "dual-rail Waveguide deferred to v1.1" it names was never written — so the
//   control moved an automation lane and nothing else. Parameter ID, choice
//   list, COMBO_IDS relay and the TIP_BINDINGS row all stay: sessions load, the
//   selector resolves (boot-all-uis: no DEAD, late count unchanged). Both
//   bodies now say the control is reserved; fr reviewed: false (meaning change).
// item 54: aria.helpToggle en "Toggle tooltips" → "Toggle hover help", the
//   family's settled name (O-simpleGrain shipped it in O3). The fr already read
//   "Activer ou désactiver l’aide au survol"; reviewed: true stays.
//
// ── v1.2.1: FRENCH QA PASS (Stage N, 2026-08-31) ────────────────────────────
// Every fr entry read against its en and against scripts/i18n-fr-glossary.js.
// Changed: 33 entries of 77 (9 terminology, 21 typography, 1 grammar,
// 2 meaning/register). sameAsEn: kept 3, translated 0. termNote exemptions: 0
// (none needed — every glossary hit resolved to a listed rendering).
// i18n-fr-lint: 49 findings → 0, --strict exit 0 (30 T1, 7 T4, 5 T5, 2 T7,
// 4 G1, 1 F1). Left as drafted: the rest. reviewed: false throughout — no
// native speaker yet.
//
// Decisions the next reader needs, each MEASURED at the shipping 1040x860 frame
// with Range.selectNodeContents on the live node (this page's .knob-label is
// shrink-to-fit inside a fixed 60px .knob-cell with overflow visible, so a long
// caption does not wrap or clip — it OVERHANGS, and the only geometry question
// is whether it reaches its neighbour):
//
//   - Amortis. → AMORTISSEMENT. The glossary root, and "Amortis." was an
//     invented third form the list does not carry. 86.69px, 4.11px clear of
//     MATÉRIAU. The page's own tightest authored pair is INHARMONICITY ↔ MODE
//     BRIGHT at 1.95px in ENGLISH, so 4.11px is inside this page's own
//     tolerance. The listed abbreviation Amort. measures 38.42px (28.23px
//     clear) and is the fallback if a font change ever eats that margin.
//   - Relâche → RELÂCHEMENT. Root, and Relâche was also the forbidden word
//     (a theatre closure). 76.22px, 12.63px clear of ATTAQUE; the abbreviation
//     Relâch. is 43.88px. The root also matches the tip title now that
//     ampRelease reads "Relâchement d'amplitude".
//   - Modèle corde KEPT over the root "Modèle de corde": the root measures
//     92.00px and WRAPS TO TWO LINES in the 92px .select-label box, adding
//     10.44px of cell height and moving the row. The short form is 80.16px on
//     one line and is the glossary's second listed rendering, not an invention.
//   - Pression kept for Bow Force (the v1.2.0 header measured it): "Pression
//     d'archet" is 2 lines / 96.88px of cell against 86.44px. The glossary
//     lists both, and the tip title carries the full term.
//   - EXCITER → EXCITATION on the diagram node. EXCITER is the infinitive verb
//     where the two sibling nodes are nouns (RÉSONATEUR, MATÉRIAU), and the
//     page's own flow hint already names the stage "excitation". 86.19 SVG
//     units centred in the 96-unit box-rect — inside it, 4.9 units each side.
//   - mailloche → maillet in two bodies: a mailloche beats a bass drum; the
//     glossary roots "mallet" on maillet.
//   - centièmes → cents in the Fine Tune body. The cent is the unit; a
//     centième is a hundredth of anything.
//   - aria.helpToggle said "les infobulles" while the same control's tip title
//     says "Aide au survol" — two French names for one control. Now one.
//   - Label-in-name (WCAG 2.5.3) holds BY STEM on the two abbreviated preset
//     verbs: Enreg. ⊂ "Enregistrer les réglages actuels…", Suppr. ⊂
//     "Supprimer le préréglage utilisateur actuel". Not invented captions.
//
// Two ENGLISH defects were found by reading the French and deliberately NOT
// fixed in v1.2.1 (Stage N does not change English) — both closed in v1.2.2,
// see the Stage O block above: the stringModel tip promised a Waveguide for
// v1.1 while the plugin shipped 1.2.1 and the parameter is never
// load()ed anywhere in Source/; and aria.helpToggle's English read "Toggle
// tooltips" where the same control's tip title reads "Hover help".
//
// UI ROOT IS Source/ui/public. There is no second UI root in this plugin. This
// file is a SOURCES entry in juce_add_binary_data(O-simplePhysicalModelSynth_UIResources)
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
// COPY IS textContent ON EVERY PATH — never innerHTML. Through v1.1.0 the
// tooltip renderer built its tip with tip.innerHTML = ...; v1.2.0 builds it with
// createElement + textContent, because the tip text is table-sourced and
// localized now rather than a fixed literal. check-i18n assertion 9 rejects any
// innerHTML reference here and any string literal containing an opening angle
// bracket.
//
// THE ENGLISH WAS MOVED, NOT REWRITTEN. Every en entry below was extracted
// MECHANICALLY from v1.1.0 — the TIPS table in js/app.js and the text nodes and
// attributes of index.html, read by the same decodeEntities the inventory uses —
// and never re-typed. HTML entities are decoded to the characters they named
// (&#183; to ·, &#8594; to →, &#8202; to a \u200a ESCAPE, NOT to a plain space)
// because setAttribute and textContent do not decode entities. Invisible
// characters are written as escapes and never as literal codepoints: a literal
// U+200A survives a heredoc, works at runtime and is invisible in review, which
// is the whole failure mode.
//
// ONE DELIBERATE ENGLISH CHANGE, recorded in the CHANGELOG: the tip bodies have
// lost their strong/em tags. The WORDS are unchanged. Assertion 9 forbids an
// angle bracket in a string literal here, and it is right to — the renderer
// writes textContent, so a tag would render as literal characters.
//
// KEYS ARE THE PARAMETER ID where the anchor is a parameter cell, and a
// label.* / aria.* / ui.* slug otherwise. The parameter cells are addressed by a
// data-param attribute ADDED IN v1.2.0; through v1.1.0 they carried the tip KEY
// in their own data-tip attribute, which canon v2 overwrites with the tip BODY.
//
// LABELS NEVER REUSE A TOOLTIP KEY for a caption. trLabel() falls back to I18N,
// and a few captions do happen to equal their tip title today ("Material",
// "Damping", "Decay") — but most do not ("Level" vs "Output Level", "Coarse" vs
// "Coarse Tune", "Engine" vs "Resonator", "Position" vs "Excite Position"), and
// a rule that holds for three of seventeen is a rule nobody can apply. A caption
// and a tip title also diverge the moment either is edited, which would make a
// tooltip copy edit a silent geometry change to a control. The ONE place the
// fallback is used on purpose is data-i18n-aria on the fourteen knobs: an
// accessible name IS the control's name, so it reads the tooltip title by design.
//
// ALL FRENCH IS MACHINE-DRAFTED AND FLAGGED reviewed: false. No native speaker
// has read it. node scripts/check-i18n.js prints the worklist.
// ============================================================================

export const LANGUAGES = ['en', 'fr'];

export const I18N = Object.freeze({

    // ── The settings popover (v1.2.0) ───────────────────────────────────────
    // The gear is new. The page already had a hover-help preference — the "?" chip
    // at the end of the preset bar, backed by localStorage under
    // "opms.tipsEnabled" — and it MOVED in here rather than sitting beside a
    // gear: a plugin should not grow a second settings surface, and the two
    // settings that decide what the hover help says and whether it says it belong
    // in one place. The storage key is unchanged, so a preference set before this
    // version survives the move.
    'gear-btn': {
        en: { t: "Settings",
              b: "Choose the language of the interface, and whether the hover help appears at all. The language is remembered with the session; the hover-help switch is remembered on this computer." },
        fr: { t: "Réglages",
              b: "Choisissez la langue de l’interface, et si l’aide au survol s’affiche. La langue est conservée avec la session ; l’état de l’aide est conservé sur cet ordinateur.",
              reviewed: true },
    },
    'lang-select': {
        en: { t: "Language",
              b: "The language of the labels on this page and of this hover help. English and French are available; the value readouts, the preset names and the three drop-down menus stay in English." },
        fr: { t: "Langue",
              b: "La langue des libellés de cette page et de cette aide au survol. L’anglais et le français sont disponibles ; les valeurs affichées, les noms de préréglages et les trois menus déroulants restent en anglais.",
              reviewed: true },
    },
    'help-toggle': {
        en: { t: "Hover help",
              b: "Turns these hover explanations off or back on. The switch is remembered on this computer rather than in the session, so it follows you from one project to the next." },
        fr: { t: "Aide au survol",
              b: "Active ou désactive ces explications au survol. Le réglage est conservé sur cet ordinateur et non dans la session : il vous suit d’un projet à l’autre.",
              reviewed: true },
    },

    // ── Excitation ──────────────────────────────────────────────────────────
    excitationType: {
        en: { t: "Excitation",
              b: "How energy enters the model: Pluck (noise burst), Strike (mallet impulse), or Bow (sustained friction). Swap it to hear why the same string sounds different plucked vs struck vs bowed." },
        fr: { t: "Excitation",
              b: "Comment l’énergie entre dans le modèle : Pluck (bouffée de bruit), Strike (impulsion de maillet) ou Bow (frottement entretenu). Changez-la pour entendre pourquoi une même corde sonne autrement pincée, frappée ou frottée.",
              reviewed: true },
    },
    excitationPosition: {
        en: { t: "Excite Position",
              b: "Where along the string the energy enters. Mid → rounder; near an end → thinner and brighter (a comb filter on the exciter)." },
        fr: { t: "Position d’excitation",
              b: "Où l’énergie entre le long de la corde. Au milieu → plus rond ; près d’une extrémité → plus mince et plus brillant (un filtre en peigne sur l’excitateur).",
              reviewed: true },
    },
    excitationColor: {
        en: { t: "Excite Color",
              b: "Brightness / hardness of the exciter. Low = a soft mallet; high = a hard, bright attack." },
        fr: { t: "Couleur d’excitation",
              b: "Brillance et dureté de l’excitateur. Bas = un maillet doux ; haut = une attaque dure et brillante.",
              reviewed: true },
    },
    bowForce: {
        en: { t: "Bow Force",
              b: "Bow only. Friction pressure on the stick-slip drive — more force gives a noisier, richer attack. Greyed unless Excitation = Bow." },
        fr: { t: "Pression d’archet",
              b: "Bow uniquement. Pression de frottement sur l’entraînement adhérence-glissement — plus de force donne une attaque plus bruitée et plus riche. Grisé sauf si Excitation = Bow.",
              reviewed: true },
    },

    // ── Resonator ───────────────────────────────────────────────────────────
    resonatorType: {
        en: { t: "Resonator",
              b: "The engine switch. String = Karplus-Strong (a harmonic comb). Modal = a bank of decaying sines (the inharmonic modes of bars & bells)." },
        fr: { t: "Résonateur",
              b: "Le sélecteur de moteur. String = Karplus-Strong (un peigne harmonique). Modal = un banc de sinus qui s’éteignent (les modes inharmoniques des barres et des cloches).",
              reviewed: true },
    },
    stringModel: {
        // v1.2.2 (Stage O item 53): the control is hidden — this entry stays
        // because the anchor and its TIP_BINDINGS row stay (session and gate
        // continuity), and it says so rather than promising a v1.1 Waveguide.
        en: { t: "String Model",
              b: "Reserved. Karplus-Strong is the only engine; the Waveguide choice is not implemented and the control is hidden. The parameter stays so existing sessions and automation still load." },
        fr: { t: "Modèle de corde",
              b: "Réservé. Karplus-Strong est le seul moteur ; le choix Waveguide n’est pas implémenté et la commande est masquée. Le paramètre reste pour que les sessions et l’automation existantes se chargent encore.",
              reviewed: false },
    },
    inharmonicity: {
        en: { t: "Inharmonicity",
              b: "Modal only. Stretches the mode spacing from harmonic (bar-like) toward inharmonic (bell-like): fₖ = f₀·k·√(1+B·k²). The control that makes a bell sound like a bell, not a string." },
        fr: { t: "Inharmonicité",
              b: "Modal uniquement. Étire l’espacement des modes de l’harmonique (proche d’une barre) vers l’inharmonique (proche d’une cloche) : fₖ = f₀·k·√(1+B·k²). La commande qui fait qu’une cloche sonne comme une cloche et non comme une corde.",
              reviewed: true },
    },
    modeBrightness: {
        en: { t: "Mode Brightness",
              b: "Modal only. Tilts the upper modes louder and longer — how bright and metallic the struck body is." },
        fr: { t: "Brillance des modes",
              b: "Modal uniquement. Rend les modes aigus plus forts et plus longs — la brillance métallique du corps frappé.",
              reviewed: true },
    },

    // ── Material / damping ──────────────────────────────────────────────────
    damping: {
        en: { t: "Damping",
              b: "The loop low-pass cutoff. It shaves a little high end on every pass, so the tone darkens as it decays — bright steel ↔ muted nylon." },
        fr: { t: "Amortissement",
              b: "La fréquence de coupure du passe-bas de la boucle. Elle rogne un peu d’aigu à chaque passage : le timbre s’assombrit en s’éteignant — acier brillant ↔ nylon feutré.",
              reviewed: true },
    },
    decay: {
        en: { t: "Decay",
              b: "The loop feedback / ring time. Near one = long sustain; lower = a short, damped pluck. Always clamped below 1 so the loop can't run away." },
        fr: { t: "Déclin",
              b: "La réinjection de la boucle, donc la durée de résonance. Proche de un = tenue longue ; plus bas = un pincement court et étouffé. Toujours borné sous 1 pour que la boucle ne s’emballe pas.",
              reviewed: true },
    },
    material: {
        en: { t: "Material",
              b: "One knob that co-moves Damping + Decay along the steel↔nylon axis — watch both knobs track as you turn it." },
        fr: { t: "Matériau",
              b: "Un seul bouton qui déplace ensemble Amortissement et Déclin le long de l’axe acier↔nylon — regardez les deux boutons suivre quand vous le tournez.",
              reviewed: true },
    },

    // ── Tuning ─────────────────────────────────────────────────────────────
    coarseTune: {
        en: { t: "Coarse Tune",
              b: "Transpose in semitones (±24)." },
        fr: { t: "Accord grossier",
              b: "Transposition en demi-tons (±24).",
              reviewed: true },
    },
    fineTune: {
        en: { t: "Fine Tune",
              b: "Fine pitch in cents (±100)." },
        fr: { t: "Accord fin",
              b: "Hauteur fine en cents (±100).",
              reviewed: true },
    },

    // ── Amp / dynamics ─────────────────────────────────────────────────────
    ampAttack: {
        en: { t: "Amp Attack",
              b: "Output amplitude fade-in. Matters most for the sustained Bow — the body's own decay is intrinsic to the model." },
        fr: { t: "Attaque d’amplitude",
              b: "Montée de l’amplitude de sortie. Surtout utile pour le Bow entretenu — le déclin propre du corps est intrinsèque au modèle.",
              reviewed: true },
    },
    ampRelease: {
        en: { t: "Amp Release",
              b: "Output fade-out after note-off — how quickly the note is damped when you let go." },
        fr: { t: "Relâchement d’amplitude",
              b: "Descente de la sortie après le relâchement de la touche — la vitesse à laquelle la note est étouffée quand vous lâchez.",
              reviewed: true },
    },
    velToBrightness: {
        en: { t: "Velocity → Brightness",
              b: "How much harder playing brightens and strengthens the excitation. The model's dynamic response — play harder, hear brighter." },
        fr: { t: "Vélocité → brillance",
              b: "À quel point jouer plus fort éclaircit et renforce l’excitation. La réponse dynamique du modèle : jouez plus fort, entendez plus brillant.",
              reviewed: true },
    },
    outputLevel: {
        en: { t: "Output Level",
              b: "Master output gain (−60 … 0 dB)." },
        fr: { t: "Niveau de sortie",
              b: "Gain de sortie général (−60 … 0 dB).",
              reviewed: true },
    },

    // ── Diagram boxes ──────────────────────────────────────────────────────
    // The four nodes of the signal-flow SVG. Their anchors were the <rect>'s own
    // data-tip through v1.1.0 and are ids from v1.2.0.
    diagExcitation: {
        en: { t: "Excitation",
              b: "Energy is injected here — a pluck, strike, or bow. Its position and color shape the attack before it reaches the resonator." },
        fr: { t: "Excitation",
              b: "L’énergie est injectée ici — un pincement, une frappe ou un coup d’archet. Sa position et sa couleur façonnent l’attaque avant qu’elle n’atteigne le résonateur.",
              reviewed: true },
    },
    diagResonator: {
        en: { t: "Resonator loop",
              b: "Pitch comes from the loop length (fundamental = sample rate ÷ delay length). The pulse circling here dims a little each pass — that fading is the note decaying. In Modal mode it becomes the ringing mode stems." },
        fr: { t: "Boucle du résonateur",
              b: "La hauteur vient de la longueur de la boucle (fondamentale = fréquence d’échantillonnage ÷ longueur du retard). L’impulsion qui tourne ici pâlit un peu à chaque passage — cet affaiblissement est la note qui s’éteint. En mode Modal, elle devient les tiges des modes qui résonnent.",
              reviewed: true },
    },
    diagMaterial: {
        en: { t: "Material / damping",
              b: "Each pass loses a little energy: the low-pass (Damping) darkens it and the feedback (Decay) sets how long it rings. This is what turns steel into nylon." },
        fr: { t: "Matériau et amortissement",
              b: "Chaque passage perd un peu d’énergie : le passe-bas (Amortissement) l’assombrit et la réinjection (Déclin) fixe la durée de résonance. C’est ce qui transforme l’acier en nylon.",
              reviewed: true },
    },
    diagOut: {
        en: { t: "Output",
              b: "The summed 16-voice signal leaving the instrument, scaled by Output Level." },
        fr: { t: "Sortie",
              b: "Le signal des 16 voix sommées qui quitte l’instrument, mis à l’échelle par le Niveau de sortie.",
              reviewed: true },
    },

});

// ============================================================================
// LABELS — one string per key, no body. A label is not a tooltip.
// ============================================================================

export const LABELS = Object.freeze({

    // ── Header ──────────────────────────────────────────────────────────────
    // The product name itself is NOT here — see I18N_EXEMPT. Only the strapline
    // under it is copy. The preset bar's two verbs are; the NAME between them is a
    // preset name and never is (D-02).
    'label.subtitle': {
        en: { t: "Karplus–Strong String & Modal Resonator Synthesizer · A Field Guide" },
        // SHORTENED, and flagged for the reviewer. The faithful "Synthétiseur à
        // corde de Karplus–Strong et à résonateur modal" measures 640.5px against
        // the 626.3px the header can give the title block once the preset bar is
        // pinned, so it wrapped to a second line and pushed the whole page down
        // 12px. Two connecting words are dropped; the sentence still names the
        // engine, the resonator and the genre. 605.4px, 20.9px of clearance.
        fr: { t: "Synthétiseur à corde Karplus–Strong et résonateur modal · un guide de terrain", reviewed: true },
    },
    'label.presetSave': {
        en: { t: "Save" },
        fr: { t: "Enreg.", reviewed: true },
    },
    'label.presetDelete': {
        en: { t: "Delete" },
        fr: { t: "Suppr.", reviewed: true },
    },

    // The two dropdown group headings, written by buildPresetDropdown() through
    // one-line setLabel writers rather than a label STRING argument.
    'label.presetFactory': {
        en: { t: "Factory" },
        fr: { t: "Usine", reviewed: true },
    },
    'label.presetUser': {
        en: { t: "User" },
        fr: { t: "Utilisateur", reviewed: true },
    },

    // ── Visualization captions ──────────────────────────────────────────────
    // Each viz-label is a caption span PLUS a hint span: applyLabel writes
    // textContent, so keying the parent would delete the hint. The middot belongs
    // to the caption — it is the separator the caption ends with in both languages.
    'label.vizSignalFlow': {
        en: { t: "Signal Flow ·" },
        fr: { t: "Flux du signal ·", reviewed: true },
    },
    'label.vizSignalFlowHint': {
        en: { t: "excitation → resonator loop → material → out" },
        fr: { t: "excitation → boucle du résonateur → matériau → sortie", reviewed: true },
    },
    'label.vizSpectrum': {
        en: { t: "Spectrum ·" },
        fr: { t: "Spectre ·", reviewed: true },
    },
    'label.vizSpectrumHint': {
        en: { t: "harmonic comb vs inharmonic modes" },
        fr: { t: "peigne harmonique ou modes inharmoniques", reviewed: true },
    },
    'label.vizWaveform': {
        en: { t: "Waveform ·" },
        fr: { t: "Forme d’onde ·", reviewed: true },
    },
    'label.vizWaveformHint': {
        en: { t: "the ringing string / body, decaying" },
        // SHORTENED, and flagged for the reviewer. The faithful "la corde ou le
        // corps qui résonne, en s'éteignant" measures 203.6px against the 177.3px
        // this hint has beside its 93.6px caption in a 273.7px viz block, so it
        // wrapped and shrank the scope canvas 11px in French only. "résonne" is
        // dropped and the participle folded into the verb. 135.5px, 41.8px of
        // clearance. The alternative — reserving the second line in BOTH
        // languages — costs 11px of canvas in English for nothing.
        fr: { t: "la corde ou le corps qui s’éteint", reviewed: true },
    },

    // ── Signal-flow diagram ─────────────────────────────────────────────────
    // The three named nodes and the MATERIAL node's sub-caption. The EXCITE node's
    // sub-caption ("pluck·strike·bow") and the RESONATOR node's state line
    // ("string · loop" / "modal · stems") are NOT here — see I18N_EXEMPT.
    'label.diagExcite': {
        en: { t: "EXCITE" },
        fr: { t: "EXCITATION", reviewed: true },
    },
    'label.diagResonator': {
        en: { t: "RESONATOR" },
        fr: { t: "RÉSONATEUR", reviewed: true },
    },
    'label.diagMaterial': {
        en: { t: "MATERIAL" },
        fr: { t: "MATÉRIAU", reviewed: true },
    },
    'label.diagMaterialSub': {
        en: { t: "damp·decay" },
        fr: { t: "amort.·déclin", reviewed: true },
    },

    // ── Column headings ─────────────────────────────────────────────────────
    'label.group1': {
        en: { t: "1 · Excitation" },
        fr: { t: "1 · Excitation", sameAsEn: true, reviewed: true },
    },
    'label.group2': {
        en: { t: "2 · Resonator" },
        fr: { t: "2 · Résonateur", reviewed: true },
    },
    'label.group3': {
        en: { t: "3 · Material · Tuning" },
        fr: { t: "3 · Matériau · Accord", reviewed: true },
    },
    'label.group4': {
        en: { t: "4 · Amp · Output" },
        fr: { t: "4 · Ampli · Sortie", reviewed: true },
    },

    // ── Control captions ────────────────────────────────────────────────────
    // Every one of these sits in a 60px (knob) or 92px (select) cell that
    // shrink-wraps its caption. See the CHANGELOG for the three that were measured
    // and shortened rather than fitted.
    'label.knobType': {
        en: { t: "Type" },
        fr: { t: "Type", sameAsEn: true, reviewed: true },
    },
    'label.knobPosition': {
        en: { t: "Position" },
        fr: { t: "Position", sameAsEn: true, reviewed: true },
    },
    'label.knobColor': {
        en: { t: "Color" },
        fr: { t: "Couleur", reviewed: true },
    },
    'label.knobBowForce': {
        en: { t: "Bow Force" },
        // SHORTENED, and flagged for the reviewer. "Force archet" measures 76.5px
        // in a 60px cell, so it wraps to two lines and makes its cell 10.5px
        // taller than the English "Bow Force" (59.5px, one line) — the only
        // caption on the page whose LINE COUNT differed between the two
        // languages. "Pression" is 48.9px and one line. The knob is greyed unless
        // Excitation = Bow and its tooltip title spells out "Pression d'archet",
        // so the column context carries the word this caption drops.
        fr: { t: "Pression", reviewed: true },
    },
    'label.knobEngine': {
        en: { t: "Engine" },
        fr: { t: "Moteur", reviewed: true },
    },
    'label.knobStringModel': {
        en: { t: "String Model" },
        fr: { t: "Modèle corde", reviewed: true },
    },
    'label.knobInharmonicity': {
        en: { t: "Inharmonicity" },
        fr: { t: "Inharmonicité", reviewed: true },
    },
    'label.knobModeBright': {
        en: { t: "Mode Bright" },
        // Two words ON PURPOSE. "Mode Bright" is 71.2px in a 60px cell and is the
        // one English caption on this page that already wraps to two lines;
        // "Brillance modes" is 96.4px and wraps the same way (line one
        // "BRILLANCE" at 57.8px clears the cell), so the cell is the same height
        // in both languages. A single-word "Brillance" would fit on ONE line and
        // make the French cell 10.5px SHORTER — the same defect as a French
        // caption growing, in the other direction (§7: French getting shorter
        // flags as loudly as French getting longer).
        fr: { t: "Brillance modes", reviewed: true },
    },
    'label.knobMaterial': {
        en: { t: "Material" },
        fr: { t: "Matériau", reviewed: true },
    },
    'label.knobDamping': {
        en: { t: "Damping" },
        fr: { t: "Amortissement", reviewed: true },
    },
    'label.knobDecay': {
        en: { t: "Decay" },
        fr: { t: "Déclin", reviewed: true },
    },
    'label.knobCoarse': {
        en: { t: "Coarse" },
        fr: { t: "Grossier", reviewed: true },
    },
    'label.knobFine': {
        en: { t: "Fine" },
        fr: { t: "Fin", reviewed: true },
    },
    'label.knobAttack': {
        en: { t: "Attack" },
        fr: { t: "Attaque", reviewed: true },
    },
    'label.knobRelease': {
        en: { t: "Release" },
        fr: { t: "Relâchement", reviewed: true },
    },
    'label.knobVelBright': {
        en: { t: "Vel→Bright" },
        fr: { t: "Vél→Brill.", reviewed: true },
    },
    'label.knobLevel': {
        en: { t: "Level" },
        fr: { t: "Niveau", reviewed: true },
    },

    // ── On-screen keyboard ──────────────────────────────────────────────────
    // The hint keeps the hair spaces the markup authored between the QWERTY
    // keycaps, written as \u200a escapes. The keycap letters themselves are
    // language-neutral and are not translated.
    'label.keyboard': {
        en: { t: "Play ·" },
        fr: { t: "Jouer ·", reviewed: true },
    },
    'label.keyboardHint': {
        en: { t: "click the keys or use your computer keyboard (A\u200aS\u200aD\u200aF\u200aG\u200aH\u200aJ\u200aK · W\u200aE\u200aT\u200aY\u200aU)" },
        fr: { t: "cliquez sur les touches ou utilisez le clavier de l’ordinateur (A\u200aS\u200aD\u200aF\u200aG\u200aH\u200aJ\u200aK · W\u200aE\u200aT\u200aY\u200aU)", reviewed: true },
    },

    // ── Accessible names ────────────────────────────────────────────────────
    // Five of these were native title= attributes through v1.1.0 (contract §4
    // deletes those); the rest were already aria-labels and are now keyed.
    'aria.presetPrev': {
        en: { t: "Previous preset" },
        fr: { t: "Préréglage précédent", reviewed: true },
    },
    'aria.presetName': {
        en: { t: "Browse presets" },
        fr: { t: "Parcourir les préréglages", reviewed: true },
    },
    'aria.presetNext': {
        en: { t: "Next preset" },
        fr: { t: "Préréglage suivant", reviewed: true },
    },
    'aria.presetSave': {
        en: { t: "Save current settings as a user preset" },
        fr: { t: "Enregistrer les réglages actuels comme préréglage utilisateur", reviewed: true },
    },
    'aria.presetDelete': {
        en: { t: "Delete the current user preset" },
        fr: { t: "Supprimer le préréglage utilisateur actuel", reviewed: true },
    },
    'aria.presetDropdown': {
        en: { t: "Presets" },
        fr: { t: "Préréglages", reviewed: true },
    },
    'aria.loopDiagram': {
        en: { t: "Excitation to resonator to material to output signal-flow diagram" },
        fr: { t: "Schéma du flux du signal : excitation, résonateur, matériau, sortie", reviewed: true },
    },
    'aria.comboExcitation': {
        en: { t: "Excitation type" },
        fr: { t: "Type d’excitation", reviewed: true },
    },
    'aria.comboResonator': {
        en: { t: "Resonator type" },
        fr: { t: "Type de résonateur", reviewed: true },
    },
    'aria.comboStringModel': {
        en: { t: "String model" },
        fr: { t: "Modèle de corde", reviewed: true },
    },
    'aria.keyboard': {
        en: { t: "On-screen keyboard" },
        fr: { t: "Clavier à l’écran", reviewed: true },
    },
    'aria.langSelect': {
        en: { t: "Interface language" },
        fr: { t: "Langue de l’interface", reviewed: true },
    },
    'aria.helpToggle': {
        en: { t: "Toggle hover help" },
        fr: { t: "Activer ou désactiver l’aide au survol", reviewed: true },
    },

    // ── The hover-help toggle's two faces ───────────────────────────────────
    // Written by applyTipsEnabled() through two setLabel calls behind an if/else,
    // never a ternary in the argument (check-i18n assertion 13).
    'ui.on': {
        en: { t: "On" },
        fr: { t: "Activée", reviewed: true },
    },
    'ui.off': {
        en: { t: "Off" },
        fr: { t: "Désactivée", reviewed: true },
    },

});
// ============================================================================
// I18N_EXEMPT — reasoned exclusions, never silence.
//
// Every visible string the coverage scan finds must be a [data-i18n] element, a
// setLabel() call, or an entry HERE WITH A REASON. A bare skip list would let a
// missed label hide as a deliberate one.
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
    ['PhysicalModel',
     'the product name, second half of the split wordmark (.title-accent) — a product name is never translated. No other text node on this page carries this word'],

    // The preset-name button's authored face before the manager fills it in.
    ['Default',
     'factory preset name — the name IS the JSON filename (D-02). It is the resting face of the preset button, replaced at init by whatever preset is current'],

    // ── The two diagram strings that mirror AudioParameterChoice values ──────
    //
    // Both are English under D-01 for the same reason O-simpleSubtractive holds
    // its own three diagram value-mirrors English: the choice strings are the
    // host automation contract, and translating the diagram without translating
    // the automation lane would make the page and the host disagree about what
    // the plugin is SET to.
    //
    // The discriminator against the node captions above it — EXCITE, RESONATOR,
    // MATERIAL, which ARE keyed — is whether the string names a parameter VALUE.
    // A caption names the box; these two name what is selected inside it.
    ['pluck·strike·bow',
     'the excitationType choice list verbatim, lowercased — Pluck / Strike / Bow are the AudioParameterChoice entries the combo one column below displays in English under D-01. Translating this line alone would make the diagram and the combo disagree about what the three options are called. No other text node on this page carries this string'],
    ['string · loop',
     'the RESONATOR node state line, written by applyDiagramSkin() from the resonatorType choice index — the String half is the AudioParameterChoice entry the Engine combo displays, English under D-01. Also the markup default for the same node. No other text node on this page carries this string'],
    ['modal · stems',
     'the other face of the same state line — the Modal half is the AudioParameterChoice entry the Engine combo displays, English under D-01. Never present in the markup; written from js/app.js only'],

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
// Through v1.1.0 the anchors carried the tip KEY in their own data-tip
// attribute and js/app.js looked the copy up in a TIPS object. That cannot
// survive canon v2 — applyI18n WRITES data-tip as the tip BODY, so the key and
// the copy would fight over one attribute, and check-i18n assertion 3 requires
// index.html to carry zero data-tip literals. The seventeen control cells are
// addressed by a data-param attribute added in v1.2.0; the four diagram boxes
// by an id.
//
// A DYNAMIC SELECTOR MOVED WITH THEM, and a literal grep does not find it.
// js/app.js:232 built its selector from a TEMPLATE LITERAL —
// document.querySelector(`[data-tip="${id}"]`) — inside setDisabled(), the
// resonator-/exciter-aware grey-out. It matches NOTHING once the anchor moves,
// and it fails silently: no error, the four gated controls simply stop dimming.
// It now reads `[data-param="${id}"]`.
// ============================================================================

export const TIP_BINDINGS = [
    ['#gear-btn',                            'gear-btn'],
    ['#lang-select',                         'lang-select'],
    ['#help-toggle',                         'help-toggle'],

    ['#diagExciteBox',                       'diagExcitation'],
    ['#diagResonatorBox',                    'diagResonator'],
    ['#diagMaterialBox',                     'diagMaterial'],
    ['#diagOutBox',                          'diagOut'],

    ['[data-param="excitationType"]',        'excitationType'],
    ['[data-param="excitationPosition"]',    'excitationPosition'],
    ['[data-param="excitationColor"]',       'excitationColor'],
    ['[data-param="bowForce"]',              'bowForce'],

    ['[data-param="resonatorType"]',         'resonatorType'],
    ['[data-param="stringModel"]',           'stringModel'],
    ['[data-param="inharmonicity"]',         'inharmonicity'],
    ['[data-param="modeBrightness"]',        'modeBrightness'],

    ['[data-param="material"]',              'material'],
    ['[data-param="damping"]',               'damping'],
    ['[data-param="decay"]',                 'decay'],
    ['[data-param="coarseTune"]',            'coarseTune'],
    ['[data-param="fineTune"]',              'fineTune'],

    ['[data-param="ampAttack"]',             'ampAttack'],
    ['[data-param="ampRelease"]',            'ampRelease'],
    ['[data-param="velToBrightness"]',       'velToBrightness'],
    ['[data-param="outputLevel"]',           'outputLevel'],
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
