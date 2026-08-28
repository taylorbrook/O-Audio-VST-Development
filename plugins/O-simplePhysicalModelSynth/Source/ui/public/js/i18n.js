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
// i18n.js — O-simplePhysicalModelSynth interface copy, English + French (v1.2.0)
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
              b: "Choisir la langue de l'interface et l'affichage de l'aide au survol. La langue est conservée avec la session ; l'état de l'aide est conservé sur cet ordinateur.",
              reviewed: false },
    },
    'lang-select': {
        en: { t: "Language",
              b: "The language of the labels on this page and of this hover help. English and French are available; the value readouts, the preset names and the three drop-down menus stay in English." },
        fr: { t: "Langue",
              b: "La langue des libellés de cette page et de cette aide au survol. L'anglais et le français sont disponibles ; les valeurs affichées, les noms de préréglages et les trois menus déroulants restent en anglais.",
              reviewed: false },
    },
    'help-toggle': {
        en: { t: "Hover help",
              b: "Turns these hover explanations off or back on. The switch is remembered on this computer rather than in the session, so it follows you from one project to the next." },
        fr: { t: "Aide au survol",
              b: "Active ou désactive ces explications au survol. Le réglage est conservé sur cet ordinateur et non dans la session : il vous suit d'un projet à l'autre.",
              reviewed: false },
    },

    // ── Excitation ──────────────────────────────────────────────────────────
    excitationType: {
        en: { t: "Excitation",
              b: "How energy enters the model: Pluck (noise burst), Strike (mallet impulse), or Bow (sustained friction). Swap it to hear why the same string sounds different plucked vs struck vs bowed." },
        fr: { t: "Excitation",
              b: "Comment l'énergie entre dans le modèle : Pluck (bouffée de bruit), Strike (impulsion de mailloche) ou Bow (frottement entretenu). Changez-la pour entendre pourquoi une même corde sonne autrement pincée, frappée ou frottée.",
              reviewed: false },
    },
    excitationPosition: {
        en: { t: "Excite Position",
              b: "Where along the string the energy enters. Mid → rounder; near an end → thinner and brighter (a comb filter on the exciter)." },
        fr: { t: "Position d'excitation",
              b: "Où l'énergie entre le long de la corde. Au milieu → plus rond ; près d'une extrémité → plus mince et plus brillant (un filtre en peigne sur l'excitateur).",
              reviewed: false },
    },
    excitationColor: {
        en: { t: "Excite Color",
              b: "Brightness / hardness of the exciter. Low = a soft mallet; high = a hard, bright attack." },
        fr: { t: "Couleur d'excitation",
              b: "Brillance et dureté de l'excitateur. Bas = une mailloche douce ; haut = une attaque dure et brillante.",
              reviewed: false },
    },
    bowForce: {
        en: { t: "Bow Force",
              b: "Bow only. Friction pressure on the stick-slip drive — more force gives a noisier, richer attack. Greyed unless Excitation = Bow." },
        fr: { t: "Pression d'archet",
              b: "Bow uniquement. Pression de frottement sur l'entraînement adhérence-glissement — plus de force donne une attaque plus bruitée et plus riche. Grisé sauf si Excitation = Bow.",
              reviewed: false },
    },

    // ── Resonator ───────────────────────────────────────────────────────────
    resonatorType: {
        en: { t: "Resonator",
              b: "The engine switch. String = Karplus-Strong (a harmonic comb). Modal = a bank of decaying sines (the inharmonic modes of bars & bells)." },
        fr: { t: "Résonateur",
              b: "Le sélecteur de moteur. String = Karplus-Strong (un peigne harmonique). Modal = un banc de sinus qui s'éteignent (les modes inharmoniques des barres et des cloches).",
              reviewed: false },
    },
    stringModel: {
        en: { t: "String Model",
              b: "Karplus-Strong is the v1.0 engine. The two-delay Waveguide (which makes excitation position physical) arrives in v1.1." },
        fr: { t: "Modèle de corde",
              b: "Karplus-Strong est le moteur de la v1.0. Le Waveguide à deux lignes à retard (qui rend physique la position d'excitation) arrive en v1.1.",
              reviewed: false },
    },
    inharmonicity: {
        en: { t: "Inharmonicity",
              b: "Modal only. Stretches the mode spacing from harmonic (bar-like) toward inharmonic (bell-like): fₖ = f₀·k·√(1+B·k²). The control that makes a bell sound like a bell, not a string." },
        fr: { t: "Inharmonicité",
              b: "Modal uniquement. Étire l'espacement des modes de l'harmonique (proche d'une barre) vers l'inharmonique (proche d'une cloche) : fₖ = f₀·k·√(1+B·k²). La commande qui fait qu'une cloche sonne comme une cloche et non comme une corde.",
              reviewed: false },
    },
    modeBrightness: {
        en: { t: "Mode Brightness",
              b: "Modal only. Tilts the upper modes louder and longer — how bright and metallic the struck body is." },
        fr: { t: "Brillance des modes",
              b: "Modal uniquement. Rend les modes aigus plus forts et plus longs — la brillance métallique du corps frappé.",
              reviewed: false },
    },

    // ── Material / damping ──────────────────────────────────────────────────
    damping: {
        en: { t: "Damping",
              b: "The loop low-pass cutoff. It shaves a little high end on every pass, so the tone darkens as it decays — bright steel ↔ muted nylon." },
        fr: { t: "Amortissement",
              b: "La fréquence de coupure du passe-bas de la boucle. Elle rogne un peu d'aigu à chaque passage : le timbre s'assombrit en s'éteignant — acier brillant ↔ nylon feutré.",
              reviewed: false },
    },
    decay: {
        en: { t: "Decay",
              b: "The loop feedback / ring time. Near one = long sustain; lower = a short, damped pluck. Always clamped below 1 so the loop can't run away." },
        fr: { t: "Déclin",
              b: "La réinjection de la boucle, donc la durée de résonance. Proche de un = tenue longue ; plus bas = un pincement court et étouffé. Toujours borné sous 1 pour que la boucle ne s'emballe pas.",
              reviewed: false },
    },
    material: {
        en: { t: "Material",
              b: "One knob that co-moves Damping + Decay along the steel↔nylon axis — watch both knobs track as you turn it." },
        fr: { t: "Matériau",
              b: "Un seul bouton qui déplace ensemble Amortissement et Déclin le long de l'axe acier↔nylon — regardez les deux boutons suivre quand vous le tournez.",
              reviewed: false },
    },

    // ── Tuning ─────────────────────────────────────────────────────────────
    coarseTune: {
        en: { t: "Coarse Tune",
              b: "Transpose in semitones (±24)." },
        fr: { t: "Accord grossier",
              b: "Transposition en demi-tons (±24).",
              reviewed: false },
    },
    fineTune: {
        en: { t: "Fine Tune",
              b: "Fine pitch in cents (±100)." },
        fr: { t: "Accord fin",
              b: "Hauteur fine en centièmes (±100).",
              reviewed: false },
    },

    // ── Amp / dynamics ─────────────────────────────────────────────────────
    ampAttack: {
        en: { t: "Amp Attack",
              b: "Output amplitude fade-in. Matters most for the sustained Bow — the body's own decay is intrinsic to the model." },
        fr: { t: "Attaque d'ampli",
              b: "Montée de l'amplitude de sortie. Surtout utile pour le Bow entretenu — l'extinction propre du corps est intrinsèque au modèle.",
              reviewed: false },
    },
    ampRelease: {
        en: { t: "Amp Release",
              b: "Output fade-out after note-off — how quickly the note is damped when you let go." },
        fr: { t: "Relâchement d'ampli",
              b: "Descente de la sortie après le relâchement de la touche — la vitesse à laquelle la note est étouffée quand vous lâchez.",
              reviewed: false },
    },
    velToBrightness: {
        en: { t: "Velocity → Brightness",
              b: "How much harder playing brightens and strengthens the excitation. The model's dynamic response — play harder, hear brighter." },
        fr: { t: "Vélocité → brillance",
              b: "À quel point jouer plus fort éclaircit et renforce l'excitation. La réponse dynamique du modèle : jouez plus fort, entendez plus brillant.",
              reviewed: false },
    },
    outputLevel: {
        en: { t: "Output Level",
              b: "Master output gain (−60 … 0 dB)." },
        fr: { t: "Niveau de sortie",
              b: "Gain de sortie général (−60 … 0 dB).",
              reviewed: false },
    },

    // ── Diagram boxes ──────────────────────────────────────────────────────
    // The four nodes of the signal-flow SVG. Their anchors were the <rect>'s own
    // data-tip through v1.1.0 and are ids from v1.2.0.
    diagExcitation: {
        en: { t: "Excitation",
              b: "Energy is injected here — a pluck, strike, or bow. Its position and color shape the attack before it reaches the resonator." },
        fr: { t: "Excitation",
              b: "L'énergie est injectée ici — un pincement, une frappe ou un coup d'archet. Sa position et sa couleur façonnent l'attaque avant qu'elle n'atteigne le résonateur.",
              reviewed: false },
    },
    diagResonator: {
        en: { t: "Resonator loop",
              b: "Pitch comes from the loop length (fundamental = sample rate ÷ delay length). The pulse circling here dims a little each pass — that fading is the note decaying. In Modal mode it becomes the ringing mode stems." },
        fr: { t: "Boucle du résonateur",
              b: "La hauteur vient de la longueur de la boucle (fondamentale = fréquence d'échantillonnage ÷ longueur du retard). L'impulsion qui tourne ici pâlit un peu à chaque passage — cette extinction est la note qui s'éteint. En mode Modal, elle devient les tiges des modes qui résonnent.",
              reviewed: false },
    },
    diagMaterial: {
        en: { t: "Material / damping",
              b: "Each pass loses a little energy: the low-pass (Damping) darkens it and the feedback (Decay) sets how long it rings. This is what turns steel into nylon." },
        fr: { t: "Matériau et amortissement",
              b: "Chaque passage perd un peu d'énergie : le passe-bas (Amortissement) l'assombrit et la réinjection (Déclin) fixe la durée de résonance. C'est ce qui transforme l'acier en nylon.",
              reviewed: false },
    },
    diagOut: {
        en: { t: "Output",
              b: "The summed 16-voice signal leaving the instrument, scaled by Output Level." },
        fr: { t: "Sortie",
              b: "Le signal des 16 voix sommées qui quitte l'instrument, mis à l'échelle par le Niveau de sortie.",
              reviewed: false },
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
        fr: { t: "Synthétiseur à corde Karplus–Strong et résonateur modal · un guide de terrain", reviewed: false },
    },
    'label.presetSave': {
        en: { t: "Save" },
        fr: { t: "Enreg.", reviewed: false },
    },
    'label.presetDelete': {
        en: { t: "Delete" },
        fr: { t: "Suppr.", reviewed: false },
    },

    // The two dropdown group headings, written by buildPresetDropdown() through
    // one-line setLabel writers rather than a label STRING argument.
    'label.presetFactory': {
        en: { t: "Factory" },
        fr: { t: "Usine", reviewed: false },
    },
    'label.presetUser': {
        en: { t: "User" },
        fr: { t: "Utilisateur", reviewed: false },
    },

    // ── Visualization captions ──────────────────────────────────────────────
    // Each viz-label is a caption span PLUS a hint span: applyLabel writes
    // textContent, so keying the parent would delete the hint. The middot belongs
    // to the caption — it is the separator the caption ends with in both languages.
    'label.vizSignalFlow': {
        en: { t: "Signal Flow ·" },
        fr: { t: "Flux du signal ·", reviewed: false },
    },
    'label.vizSignalFlowHint': {
        en: { t: "excitation → resonator loop → material → out" },
        fr: { t: "excitation → boucle du résonateur → matériau → sortie", reviewed: false },
    },
    'label.vizSpectrum': {
        en: { t: "Spectrum ·" },
        fr: { t: "Spectre ·", reviewed: false },
    },
    'label.vizSpectrumHint': {
        en: { t: "harmonic comb vs inharmonic modes" },
        fr: { t: "peigne harmonique ou modes inharmoniques", reviewed: false },
    },
    'label.vizWaveform': {
        en: { t: "Waveform ·" },
        fr: { t: "Forme d'onde ·", reviewed: false },
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
        fr: { t: "la corde ou le corps qui s'éteint", reviewed: false },
    },

    // ── Signal-flow diagram ─────────────────────────────────────────────────
    // The three named nodes and the MATERIAL node's sub-caption. The EXCITE node's
    // sub-caption ("pluck·strike·bow") and the RESONATOR node's state line
    // ("string · loop" / "modal · stems") are NOT here — see I18N_EXEMPT.
    'label.diagExcite': {
        en: { t: "EXCITE" },
        fr: { t: "EXCITER", reviewed: false },
    },
    'label.diagResonator': {
        en: { t: "RESONATOR" },
        fr: { t: "RÉSONATEUR", reviewed: false },
    },
    'label.diagMaterial': {
        en: { t: "MATERIAL" },
        fr: { t: "MATÉRIAU", reviewed: false },
    },
    'label.diagMaterialSub': {
        en: { t: "damp·decay" },
        fr: { t: "amort.·déclin", reviewed: false },
    },

    // ── Column headings ─────────────────────────────────────────────────────
    'label.group1': {
        en: { t: "1 · Excitation" },
        fr: { t: "1 · Excitation", sameAsEn: true, reviewed: false },
    },
    'label.group2': {
        en: { t: "2 · Resonator" },
        fr: { t: "2 · Résonateur", reviewed: false },
    },
    'label.group3': {
        en: { t: "3 · Material · Tuning" },
        fr: { t: "3 · Matériau · Accord", reviewed: false },
    },
    'label.group4': {
        en: { t: "4 · Amp · Output" },
        fr: { t: "4 · Ampli · Sortie", reviewed: false },
    },

    // ── Control captions ────────────────────────────────────────────────────
    // Every one of these sits in a 60px (knob) or 92px (select) cell that
    // shrink-wraps its caption. See the CHANGELOG for the three that were measured
    // and shortened rather than fitted.
    'label.knobType': {
        en: { t: "Type" },
        fr: { t: "Type", sameAsEn: true, reviewed: false },
    },
    'label.knobPosition': {
        en: { t: "Position" },
        fr: { t: "Position", sameAsEn: true, reviewed: false },
    },
    'label.knobColor': {
        en: { t: "Color" },
        fr: { t: "Couleur", reviewed: false },
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
        fr: { t: "Pression", reviewed: false },
    },
    'label.knobEngine': {
        en: { t: "Engine" },
        fr: { t: "Moteur", reviewed: false },
    },
    'label.knobStringModel': {
        en: { t: "String Model" },
        fr: { t: "Modèle corde", reviewed: false },
    },
    'label.knobInharmonicity': {
        en: { t: "Inharmonicity" },
        fr: { t: "Inharmonicité", reviewed: false },
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
        fr: { t: "Brillance modes", reviewed: false },
    },
    'label.knobMaterial': {
        en: { t: "Material" },
        fr: { t: "Matériau", reviewed: false },
    },
    'label.knobDamping': {
        en: { t: "Damping" },
        fr: { t: "Amortis.", reviewed: false },
    },
    'label.knobDecay': {
        en: { t: "Decay" },
        fr: { t: "Déclin", reviewed: false },
    },
    'label.knobCoarse': {
        en: { t: "Coarse" },
        fr: { t: "Grossier", reviewed: false },
    },
    'label.knobFine': {
        en: { t: "Fine" },
        fr: { t: "Fin", reviewed: false },
    },
    'label.knobAttack': {
        en: { t: "Attack" },
        fr: { t: "Attaque", reviewed: false },
    },
    'label.knobRelease': {
        en: { t: "Release" },
        fr: { t: "Relâche", reviewed: false },
    },
    'label.knobVelBright': {
        en: { t: "Vel→Bright" },
        fr: { t: "Vél→Brill.", reviewed: false },
    },
    'label.knobLevel': {
        en: { t: "Level" },
        fr: { t: "Niveau", reviewed: false },
    },

    // ── On-screen keyboard ──────────────────────────────────────────────────
    // The hint keeps the hair spaces the markup authored between the QWERTY
    // keycaps, written as \u200a escapes. The keycap letters themselves are
    // language-neutral and are not translated.
    'label.keyboard': {
        en: { t: "Play ·" },
        fr: { t: "Jouer ·", reviewed: false },
    },
    'label.keyboardHint': {
        en: { t: "click the keys or use your computer keyboard (A\u200aS\u200aD\u200aF\u200aG\u200aH\u200aJ\u200aK · W\u200aE\u200aT\u200aY\u200aU)" },
        fr: { t: "cliquez les touches ou utilisez le clavier de l'ordinateur (A\u200aS\u200aD\u200aF\u200aG\u200aH\u200aJ\u200aK · W\u200aE\u200aT\u200aY\u200aU)", reviewed: false },
    },

    // ── Accessible names ────────────────────────────────────────────────────
    // Five of these were native title= attributes through v1.1.0 (contract §4
    // deletes those); the rest were already aria-labels and are now keyed.
    'aria.presetPrev': {
        en: { t: "Previous preset" },
        fr: { t: "Préréglage précédent", reviewed: false },
    },
    'aria.presetName': {
        en: { t: "Browse presets" },
        fr: { t: "Parcourir les préréglages", reviewed: false },
    },
    'aria.presetNext': {
        en: { t: "Next preset" },
        fr: { t: "Préréglage suivant", reviewed: false },
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
    'aria.loopDiagram': {
        en: { t: "Excitation to resonator to material to output signal-flow diagram" },
        fr: { t: "Schéma du flux du signal : excitation, résonateur, matériau, sortie", reviewed: false },
    },
    'aria.comboExcitation': {
        en: { t: "Excitation type" },
        fr: { t: "Type d'excitation", reviewed: false },
    },
    'aria.comboResonator': {
        en: { t: "Resonator type" },
        fr: { t: "Type de résonateur", reviewed: false },
    },
    'aria.comboStringModel': {
        en: { t: "String model" },
        fr: { t: "Modèle de corde", reviewed: false },
    },
    'aria.keyboard': {
        en: { t: "On-screen keyboard" },
        fr: { t: "Clavier à l'écran", reviewed: false },
    },
    'aria.langSelect': {
        en: { t: "Interface language" },
        fr: { t: "Langue de l'interface", reviewed: false },
    },
    'aria.helpToggle': {
        en: { t: "Toggle tooltips" },
        fr: { t: "Activer ou désactiver les infobulles", reviewed: false },
    },

    // ── The hover-help toggle's two faces ───────────────────────────────────
    // Written by applyTipsEnabled() through two setLabel calls behind an if/else,
    // never a ternary in the argument (check-i18n assertion 13).
    'ui.on': {
        en: { t: "On" },
        fr: { t: "Activée", reviewed: false },
    },
    'ui.off': {
        en: { t: "Off" },
        fr: { t: "Désactivée", reviewed: false },
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
