/*
   This file is part of O-simpleSubtractive, an Ouaricon Audio plugin.
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
// i18n.js — O-simpleSubtractive interface copy, English + French (v1.3.0)
//
// UI ROOT IS Source/ui/public. There is no second UI root in this plugin and no
// Resources/ui staging directory. This file is the sixth SOURCES entry in
// juce_add_binary_data(O-simpleSubtractive_UIResources).
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
// COPY IS textContent ON EVERY PATH — never innerHTML. v1.2.5's tooltip
// renderer built its tip with `tip.innerHTML = ...`; v1.3.0 builds it with
// createElement + textContent, because the tip text is now table-sourced and
// localized rather than a fixed literal. check-i18n assertion 9 rejects any
// innerHTML reference here and any string literal containing an opening angle
// bracket.
//
// THE ENGLISH WAS MOVED, NOT REWRITTEN. Every `en` entry below was extracted
// mechanically from v1.2.5's `TIPS` / `LESSONS` tables in js/app.js and from
// index.html, and compared back to the source with entities decoded, rather
// than re-typed. HTML entities are decoded to the characters they named
// (&#183; -> ·, &#8594; -> →, &#8202; -> a \u200a escape, NOT a plain space)
// because setAttribute and textContent do not decode entities.
//
// ONE DELIBERATE ENGLISH CHANGE, recorded in the CHANGELOG: the tip bodies have
// lost their strong/em emphasis tags. The WORDS are unchanged. Assertion 9
// forbids an angle bracket in a string literal here, and it is right to: the
// renderer now writes textContent, so a tag would render as literal characters
// rather than as emphasis.
//
// KEYS ARE THE PARAMETER ID where the anchor is a parameter cell, and a
// `label.*` / `aria.*` slug otherwise. The parameter cells are addressed by the
// data-param attribute added in v1.3.0; through v1.2.5 they carried the tip KEY
// in their own data-tip attribute, and a `.knob-cell` selector would have
// matched the FIRST of sixteen — the failure canon §1 names and the one
// O-Octagon's .vunit-group tip hit for real in Stage C.
//
// LABELS NEVER REUSE A TOOLTIP KEY HERE. trLabel() falls back to I18N and four
// of this page's captions do happen to equal their tip title today ("Noise",
// "Resonance", "Signal Path", "Output Waveform") — but twenty others do not
// ("Cutoff" vs "Cutoff Frequency", "Level" vs "Output Level", "Mode" vs "Voice
// Mode"), and a rule that holds for four of twenty-four is a rule nobody can
// apply. A caption and a tip title also diverge the moment either is edited,
// which would make a tooltip copy edit a silent geometry change to a control.
// The ONE place the fallback is used on purpose is `data-i18n-aria` on the
// knobs, combos and canvases: an accessible name IS the control's name, so it
// reads the tooltip title by design.
//
// ALL FRENCH IS MACHINE-DRAFTED AND FLAGGED `reviewed: false`. No native
// speaker has read it. `node scripts/check-i18n.js` prints the worklist.
// ============================================================================

export const LANGUAGES = ['en', 'fr'];

export const I18N = Object.freeze({

    // ── The settings popover (v1.3.0) ───────────────────────────────────────
    // The gear is new. This plugin has no hover-help bridge and never had a "?"
    // toggle — not a C++ one, not a localStorage one — so the panel holds the
    // language selector ALONE. A toggle row would be a control for a preference
    // that does not exist.
    'gear-btn': {
        en: { t: 'Settings',
              b: 'Choose the language of the interface. The choice is remembered with the session.' },
        fr: { t: 'Réglages',
              b: "Choisir la langue de l'interface. Le choix est conservé avec la session.",
              reviewed: false },
    },

    // Written to say what is TRUE of canon v2, in both languages: the labels DO
    // change, and the halves that stay English are named rather than left to be
    // discovered — value readouts (D-03) and the four drop-down menus, whose
    // entries come from the C++ AudioParameterChoice and are the host
    // automation contract (D-01).
    'lang-select': {
        en: { t: 'Language',
              b: 'The language of the labels on this page and of this hover help. English and French are available; value readouts and the four drop-down menus stay in English.' },
        fr: { t: 'Langue',
              b: "La langue des libellés de cette page et de cette aide au survol. L'anglais et le français sont disponibles ; les valeurs affichées et les quatre menus déroulants restent en anglais.",
              reviewed: false },
    },

    // ── Oscillator group ────────────────────────────────────────────────────
    oscWave: {
        en: { t: 'Oscillator Wave',
              b: 'The raw tone the filter carves from. Saw is the brightest (all harmonics), Square is hollow (odd harmonics), Triangle is soft, Sine is pure — nothing for the filter to remove.' },
        fr: { t: "Onde de l'oscillateur",
              b: "Le timbre brut dans lequel le filtre taille. Saw est la plus brillante (tous les harmoniques), Square est creuse (harmoniques impairs), Triangle est douce, Sine est pure — le filtre n'a rien à retirer.",
              reviewed: false },
    },
    subLevel: {
        en: { t: 'Sub Oscillator',
              b: 'Mixes in a square wave one octave below the note. Adds body and weight underneath the main oscillator — useful for basses.' },
        fr: { t: 'Sous-oscillateur',
              b: "Ajoute au mélange une onde carrée une octave sous la note. Apporte du corps et du poids sous l'oscillateur principal — utile pour les basses.",
              reviewed: false },
    },
    noiseLevel: {
        en: { t: 'Noise',
              b: "Mixes in white noise — every frequency at once. Feed it through the filter to hear the filter's shape on its own, or add breath/air to a tone." },
        fr: { t: 'Bruit',
              b: "Ajoute au mélange du bruit blanc — toutes les fréquences à la fois. Passez-le dans le filtre pour entendre la forme du filtre seule, ou pour ajouter du souffle et de l'air à un timbre.",
              reviewed: false },
    },

    // ── Filter group ────────────────────────────────────────────────────────
    filterType: {
        en: { t: 'Filter Type',
              b: 'Which side of the cutoff is kept. Low-pass keeps lows (the classic subtractive sound), High-pass keeps highs, Band-pass keeps a band around cutoff, Notch removes a band.' },
        fr: { t: 'Type de filtre',
              b: "Quel côté de la coupure est conservé. Low-pass garde le grave (le son soustractif classique), High-pass garde l'aigu, Band-pass garde une bande autour de la coupure, Notch retire une bande.",
              reviewed: false },
    },
    filterSlope: {
        en: { t: 'Filter Slope (poles)',
              b: 'How sharply the filter cuts past the cutoff. 6 dB/oct = 1 pole, gentle. 24 dB/oct = 4 poles, steep and aggressive. Steeper = more of the spectrum removed just past the knee.' },
        fr: { t: 'Pente du filtre (pôles)',
              b: 'Avec quelle netteté le filtre coupe au-delà de la coupure. 6 dB/oct = 1 pôle, doux. 24 dB/oct = 4 pôles, raide et agressif. Plus raide = plus de spectre retiré juste après le coude.',
              reviewed: false },
    },
    cutoff: {
        en: { t: 'Cutoff Frequency',
              b: "The corner where the filter starts working. Lower it and watch the spectrum bars above the curve fall away — that's harmonics being removed, the heart of subtractive synthesis." },
        fr: { t: 'Fréquence de coupure',
              b: 'Le coude où le filtre commence à agir. Abaissez-la et regardez les barres du spectre au-dessus de la courbe disparaître — ce sont des harmoniques qui sont retirés, le cœur de la synthèse soustractive.',
              reviewed: false },
    },
    resonance: {
        en: { t: 'Resonance',
              b: "Boosts a peak right at the cutoff. A little adds vocal emphasis; push it far and the filter rings, then self-oscillates into a pure sine whistle at the cutoff — the curve's peak grows into a spike." },
        fr: { t: 'Résonance',
              b: 'Accentue un pic juste à la coupure. Un peu apporte une emphase vocale ; poussez loin et le filtre sonne, puis auto-oscille en un sifflement sinusoïdal pur à la coupure — le pic de la courbe devient une pointe.',
              reviewed: false },
    },
    filterEnvAmount: {
        en: { t: 'Filter Env Amount',
              b: 'How far the filter envelope sweeps the cutoff, and in which direction. Positive opens the filter on each note (bright attack); negative closes it. Bipolar: zero means the envelope does nothing.' },
        fr: { t: "Taux d'enveloppe de filtre",
              b: "De combien l'enveloppe de filtre balaie la coupure, et dans quel sens. Positif ouvre le filtre à chaque note (attaque brillante) ; négatif le ferme. Bipolaire : à zéro l'enveloppe ne fait rien.",
              reviewed: false },
    },
    keyTrack: {
        en: { t: 'Key Tracking',
              b: 'Makes the cutoff follow the note pitch — higher notes open the filter more. At 100% the filter tracks the keyboard so timbre stays consistent across the range.' },
        fr: { t: 'Suivi de clavier',
              b: 'Fait suivre la hauteur de la note par la coupure — les notes aiguës ouvrent davantage le filtre. À 100 % le filtre suit le clavier, si bien que le timbre reste constant sur toute la tessiture.',
              reviewed: false },
    },

    // ── Filter envelope ─────────────────────────────────────────────────────
    filterAttack: {
        en: { t: 'Filter Attack',
              b: 'Time for the filter envelope to rise after note-on — how fast the filter sweep opens.' },
        fr: { t: 'Attaque du filtre',
              b: "Temps que met l'enveloppe de filtre à monter après le début de note — la vitesse à laquelle le balayage du filtre s'ouvre.",
              reviewed: false },
    },
    filterDecay: {
        en: { t: 'Filter Decay',
              b: 'Time for the filter envelope to fall from its peak to the sustain level — shapes the bright-to-dark motion of a pluck.' },
        fr: { t: 'Déclin du filtre',
              b: "Temps que met l'enveloppe de filtre à retomber de son sommet au niveau de maintien — façonne le passage du brillant au sombre d'un pincement.",
              reviewed: false },
    },
    filterSustain: {
        en: { t: 'Filter Sustain',
              b: 'The cutoff-sweep level held while the key stays down.' },
        fr: { t: 'Maintien du filtre',
              b: 'Le niveau de balayage de coupure tenu tant que la touche reste enfoncée.',
              reviewed: false },
    },
    filterRelease: {
        en: { t: 'Filter Release',
              b: 'Time for the filter sweep to fall back after the key is released.' },
        fr: { t: 'Relâchement du filtre',
              b: 'Temps que met le balayage du filtre à redescendre après le relâchement de la touche.',
              reviewed: false },
    },

    // ── Amp envelope ────────────────────────────────────────────────────────
    ampAttack: {
        en: { t: 'Amp Attack',
              b: 'Time for loudness to rise after note-on. Short = a percussive start; long = a slow swell.' },
        fr: { t: "Attaque d'amplitude",
              b: 'Temps que met le volume à monter après le début de note. Court = un départ percussif ; long = une montée lente.',
              reviewed: false },
    },
    ampDecay: {
        en: { t: 'Amp Decay',
              b: 'Time for loudness to fall from its peak to the sustain level.' },
        fr: { t: "Déclin d'amplitude",
              b: 'Temps que met le volume à retomber de son sommet au niveau de maintien.',
              reviewed: false },
    },
    ampSustain: {
        en: { t: 'Amp Sustain',
              b: 'Loudness held while the key stays down.' },
        fr: { t: "Maintien d'amplitude",
              b: 'Volume tenu tant que la touche reste enfoncée.',
              reviewed: false },
    },
    ampRelease: {
        en: { t: 'Amp Release',
              b: 'Time for loudness to fade after the key is released — also sets how long the voice rings out.' },
        fr: { t: "Relâchement d'amplitude",
              b: 'Temps que met le volume à disparaître après le relâchement de la touche — fixe aussi la durée pendant laquelle la voix continue de sonner.',
              reviewed: false },
    },

    // ── Voice / Output ──────────────────────────────────────────────────────
    voiceMode: {
        en: { t: 'Voice Mode',
              b: 'Poly plays chords. Mono plays one note, retriggering the envelopes each time. Legato plays one note but slurs — overlapping notes glide without retriggering.' },
        fr: { t: 'Mode de voix',
              b: 'Poly joue des accords. Mono joue une seule note, en redéclenchant les enveloppes à chaque fois. Legato joue une seule note mais lie — les notes qui se chevauchent glissent sans redéclenchement.',
              reviewed: false },
    },
    glide: {
        en: { t: 'Glide (portamento)',
              b: 'Time to slide pitch from one note to the next. Most audible in Mono/Legato — zero is an instant jump.' },
        fr: { t: 'Glissando (portamento)',
              b: "Temps de glissement de la hauteur d'une note à la suivante. Surtout audible en Mono/Legato — à zéro le saut est instantané.",
              reviewed: false },
    },
    outputLevel: {
        en: { t: 'Output Level',
              b: 'Master output trim in decibels. -60 dB is silence.' },
        fr: { t: 'Niveau de sortie',
              b: 'Réglage de la sortie générale en décibels. -60 dB, c’est le silence.',
              reviewed: false },
    },

    // ── The three displays and the diagram ──────────────────────────────────
    headline: {
        en: { t: 'Filter Response over Spectrum',
              b: "The amber line is the filter's frequency response — the shape it imposes. The bars are the live output spectrum. Harmonics sitting above the curve's knee get pushed down: you are watching the filter remove sound." },
        fr: { t: 'Réponse du filtre sur le spectre',
              b: "La ligne ambrée est la réponse en fréquence du filtre — la forme qu'il impose. Les barres sont le spectre de sortie en direct. Les harmoniques situés au-dessus du coude de la courbe sont abaissés : vous regardez le filtre retirer du son.",
              reviewed: false },
    },
    scope: {
        en: { t: 'Output Waveform',
              b: 'The post-filter signal in the time domain. Watch a bright saw round off into a smooth shape as you lower the cutoff, or ring as resonance climbs.' },
        fr: { t: 'Forme d’onde de sortie',
              b: 'Le signal après filtrage, dans le domaine temporel. Regardez une dent de scie brillante s’arrondir en une forme lisse quand vous abaissez la coupure, ou se mettre à sonner quand la résonance monte.',
              reviewed: false },
    },
    filterAdsr: {
        en: { t: 'Filter Envelope → cutoff',
              b: "The shape that sweeps the cutoff over time (Attack-Decay-Sustain-Release). The dashed marker shows the envelope's live output as you play — this scale drives brightness, not loudness." },
        fr: { t: 'Enveloppe de filtre → coupure',
              b: "La forme qui balaie la coupure dans le temps (attaque, déclin, maintien, relâchement). Le repère pointillé montre la sortie de l'enveloppe en direct pendant que vous jouez — cette échelle pilote la brillance, pas le volume.",
              reviewed: false },
    },
    ampAdsr: {
        en: { t: 'Amp Envelope → level',
              b: 'The shape that controls loudness over time. The dashed marker shows its live output — an independent scale from the filter envelope, so brightness and volume can move separately.' },
        fr: { t: 'Enveloppe d’amplitude → niveau',
              b: "La forme qui contrôle le volume dans le temps. Le repère pointillé montre sa sortie en direct — une échelle indépendante de celle de l'enveloppe de filtre, si bien que brillance et volume peuvent bouger séparément.",
              reviewed: false },
    },
    routing: {
        en: { t: 'Signal Path',
              b: 'Oscillator → Filter → Amplifier. The filter envelope routes up into the filter (sweeping cutoff); the amp envelope routes up into the VCA (shaping loudness). Two envelopes, two destinations.' },
        fr: { t: 'Chaîne du signal',
              b: "Oscillateur → filtre → amplificateur. L'enveloppe de filtre remonte dans le filtre (elle balaie la coupure) ; l'enveloppe d'amplitude remonte dans le VCA (elle façonne le volume). Deux enveloppes, deux destinations.",
              reviewed: false },
    },

    // ── The eight lesson presets ────────────────────────────────────────────
    lessonSawSweep: {
        en: { t: "Saw → LP Sweep · how it's built",
              b: 'The headline move: a bright saw through a 24 dB low-pass with a slow filter envelope. Watch the upper harmonics fall away under the curve as the cutoff opens and closes — the literal subtraction the method is named for.' },
        fr: { t: 'Dent de scie → balayage passe-bas · comment c’est fait',
              b: 'Le geste emblématique : une dent de scie brillante à travers un passe-bas 24 dB avec une enveloppe de filtre lente. Regardez les harmoniques aigus disparaître sous la courbe pendant que la coupure s’ouvre et se referme — la soustraction littérale qui donne son nom à la méthode.',
              reviewed: false },
    },
    lessonPluck: {
        en: { t: "Pluck · how it's built",
              b: 'A fast filter envelope (short decay, low sustain) snaps the cutoff bright-then-dark, while a quick amp decay makes a percussive note. Filter env does the timbral work.' },
        fr: { t: 'Pincement · comment c’est fait',
              b: "Une enveloppe de filtre rapide (déclin court, maintien bas) fait claquer la coupure du brillant vers le sombre, pendant qu'un déclin d'amplitude rapide donne une note percussive. C'est l'enveloppe de filtre qui fait le travail de timbre.",
              reviewed: false },
    },
    lessonSweep: {
        en: { t: "Sweep Pad · how it's built",
              b: 'Slow amp attack swells the level in; a long, deep filter envelope opens the cutoff gradually — you hear the spectrum brighten over seconds. Pads are about slow envelopes.' },
        fr: { t: 'Nappe balayée · comment c’est fait',
              b: "Une attaque d'amplitude lente fait monter le niveau ; une enveloppe de filtre longue et profonde ouvre la coupure peu à peu — on entend le spectre s'éclaircir sur plusieurs secondes. Les nappes sont affaire d'enveloppes lentes.",
              reviewed: false },
    },
    lessonAcid: {
        en: { t: "Acid Bass · how it's built",
              b: 'High resonance + a snappy filter envelope on a saw through a 24 dB low-pass — the squelchy, ringing peak that defines the acid sound. Mono with a touch of glide.' },
        fr: { t: 'Basse acid · comment c’est fait',
              b: 'Résonance élevée et enveloppe de filtre nerveuse sur une dent de scie à travers un passe-bas 24 dB — le pic sonnant et gluant qui définit le son acid. En mono, avec un soupçon de glissando.',
              reviewed: false },
    },
    lessonSelfOsc: {
        en: { t: "Self-Oscillation · how it's built",
              b: 'Resonance pushed to the limit with the oscillators down: the filter rings on its own into a pure sine at the cutoff. The filter becomes the sound source.' },
        fr: { t: 'Auto-oscillation · comment c’est fait',
              b: 'Résonance poussée à la limite, oscillateurs baissés : le filtre sonne tout seul en une sinusoïde pure à la coupure. Le filtre devient la source sonore.',
              reviewed: false },
    },
    lessonBrass: {
        en: { t: "Brass Stab · how it's built",
              b: 'Positive filter-env amount so the cutoff opens with the attack and holds — brightness tracks the note like a blown brass instrument. A short, firm amp envelope gives the stab.' },
        fr: { t: 'Coup de cuivres · comment c’est fait',
              b: "Taux d'enveloppe de filtre positif, si bien que la coupure s'ouvre avec l'attaque et s'y tient — la brillance suit la note comme un cuivre soufflé. Une enveloppe d'amplitude courte et ferme donne le coup.",
              reviewed: false },
    },
    lessonSquareBass: {
        en: { t: "Square Bass · how it's built",
              b: 'A hollow square wave (odd harmonics only) plus the sub-oscillator an octave down for weight, through a 24 dB low-pass. Mono, so it plays as one solid bass voice — a polysynth is just several of these in parallel.' },
        fr: { t: 'Basse carrée · comment c’est fait',
              b: 'Une onde carrée creuse (harmoniques impairs seulement) plus le sous-oscillateur une octave en dessous pour le poids, à travers un passe-bas 24 dB. En mono, pour une seule voix de basse bien pleine — un polysynthé, ce sont simplement plusieurs de ces voix en parallèle.',
              reviewed: false },
    },
    lessonNoiseWind: {
        en: { t: "Filtered Noise · how it's built",
              b: 'Push the noise source up and band-pass it: with no harmonic source the filter sculpts pitchless air into wind. Slow envelopes swell it in and out — how subtractive synthesis makes breath and percussion, not just notes.' },
        fr: { t: 'Bruit filtré · comment c’est fait',
              b: "Montez la source de bruit et passez-la en passe-bande : sans source harmonique, le filtre sculpte de l'air sans hauteur en vent. Des enveloppes lentes le font entrer et sortir — comment la synthèse soustractive fabrique du souffle et de la percussion, et pas seulement des notes.",
              reviewed: false },
    },
});

// ============================================================================
// LABELS — the page's own captions, v1.3.0
//
// Separate from I18N because a tooltip entry is a {title, body} PAIR and a
// label is one string.
//
// THE REUSE RULE, as applied on this page: NO caption reuses a tooltip key.
// The header comment above records why — four of twenty-four captions match
// their tip title and twenty do not, so reuse would be a rule nobody could
// apply, and a tooltip copy edit would become a silent geometry change to a
// control.
//
// The `aria.*` keys are the exception the fallback exists for: they are read by
// data-i18n-aria on the knobs, combos and canvases, where the accessible name
// IS the control's name and reading the tooltip title is the point.
//
// `sameAsEn: true` marks the three captions whose faithful French IS the
// English word. It is a declaration, not a skip: assertion 4 rejects a silent
// passthrough and accepts a declared one.
//
// ALL FRENCH IS MACHINE-DRAFTED, `reviewed: false`.
// ============================================================================

export const LABELS = Object.freeze({

    // ── Header ──────────────────────────────────────────────────────────────
    // The product name itself is NOT here — see I18N_EXEMPT. Only the strapline
    // under it is copy.
    'label.subtitle': {
        en: { t: 'Subtractive Synthesizer · Osc → Filter → Amp · A Field Guide' },
        fr: { t: 'Synthétiseur soustractif · osc → filtre → ampli · un guide de terrain',
              reviewed: false },
    },

    // ── The two displays ────────────────────────────────────────────────────
    // "Filter Response over Spectrum ·" keeps its trailing separator: the
    // fleuron belongs to the caption, not to the hint span beside it, and
    // moving it would change where the two boxes meet.
    'label.headline': {
        en: { t: 'Filter Response over Spectrum ·' },
        fr: { t: 'Réponse du filtre sur le spectre ·', reviewed: false },
    },
    'label.headlineHint': {
        en: { t: 'the curve is the filter; the bars are what it lets through' },
        fr: { t: 'la courbe est le filtre ; les barres sont ce qu’il laisse passer',
              reviewed: false },
    },
    'label.scope': {
        en: { t: 'Output Waveform ·' },
        fr: { t: 'Forme d’onde de sortie ·', reviewed: false },
    },
    'label.scopeHint': {
        en: { t: 'morphs with cutoff / res / envelope' },
        fr: { t: 'change avec coupure / rés. / enveloppe', reviewed: false },
    },

    // ── Signal-path diagram ─────────────────────────────────────────────────
    // The three node captions and the two route captions are SVG <text> nodes.
    // applyLabel writes textContent, which an SVG text node has; they are
    // text-anchor="middle" at a fixed x, so translating them moves nothing.
    'label.signalPath': {
        en: { t: 'Signal Path' },
        fr: { t: 'Chaîne du signal', reviewed: false },
    },
    'label.routeFilterEnv': {
        en: { t: 'filter env → cutoff' },
        fr: { t: 'env. filtre → coupure', reviewed: false },
    },
    'label.routeAmpEnv': {
        en: { t: 'amp env → level' },
        fr: { t: 'env. ampli → niveau', reviewed: false },
    },
    'label.nodeOsc': {
        en: { t: 'OSC' },
        fr: { t: 'OSC', sameAsEn: true, reviewed: false },
    },
    'label.nodeFilter': {
        en: { t: 'FILTER' },
        fr: { t: 'FILTRE', reviewed: false },
    },
    // VCA is the standard French term too — voltage-controlled amplifier keeps
    // its English initialism in French synth vocabulary.
    'label.nodeVca': {
        en: { t: 'VCA' },
        fr: { t: 'VCA', sameAsEn: true, reviewed: false },
    },
    // The abbreviation beside the resonance numeral. Its own span, so the
    // fleuron and the hair space around it stay literal text nodes: applyLabel
    // writes textContent, which on the parent would delete both readout spans.
    'label.res': {
        en: { t: 'Res' },
        fr: { t: 'Rés', reviewed: false },
    },

    // ── Oscillator group ────────────────────────────────────────────────────
    'label.groupOsc': {
        en: { t: 'Oscillator' },
        fr: { t: 'Oscillateur', reviewed: false },
    },
    'label.wave': { en: { t: 'Wave' },  fr: { t: 'Onde',  reviewed: false } },
    'label.sub':   { en: { t: 'Sub' },   fr: { t: 'Sous',  reviewed: false } },
    'label.noise': { en: { t: 'Noise' }, fr: { t: 'Bruit', reviewed: false } },

    // ── Filter group ────────────────────────────────────────────────────────
    'label.groupFilter': { en: { t: 'Filter' }, fr: { t: 'Filtre', reviewed: false } },
    'label.type':      { en: { t: 'Type' },      fr: { t: 'Type', sameAsEn: true, reviewed: false } },
    'label.slope':     { en: { t: 'Slope' },     fr: { t: 'Pente', reviewed: false } },
    'label.cutoff':    { en: { t: 'Cutoff' },    fr: { t: 'Coupure', reviewed: false } },
    'label.resonance': { en: { t: 'Resonance' }, fr: { t: 'Résonance', reviewed: false } },
    'label.envAmt':    { en: { t: 'Env Amt' },   fr: { t: 'Taux env.', reviewed: false } },
    'label.keyTrack':  { en: { t: 'Key Track' }, fr: { t: 'Suivi clavier', reviewed: false } },

    // ── The two envelope groups ─────────────────────────────────────────────
    // Each title is a caption plus a routing suffix in its own .group-route
    // span. The two halves are two keys: applyLabel writes textContent, so a
    // key on the <h2> would delete the span.
    'label.groupFilterEnv': {
        en: { t: 'Filter Envelope' },
        fr: { t: 'Enveloppe de filtre', reviewed: false },
    },
    'label.routeToCutoff': {
        en: { t: '→ cutoff' },
        fr: { t: '→ coupure', reviewed: false },
    },
    'label.groupAmpEnv': {
        en: { t: 'Amp Envelope' },
        fr: { t: 'Enveloppe d’ampli', reviewed: false },
    },
    'label.routeToLevel': {
        en: { t: '→ level' },
        fr: { t: '→ niveau', reviewed: false },
    },
    // The four ADSR captions are shared by BOTH envelope groups — eight
    // elements, four keys.
    'label.attack':  { en: { t: 'Attack' },  fr: { t: 'Attaque', reviewed: false } },
    'label.decay':   { en: { t: 'Decay' },   fr: { t: 'Déclin', reviewed: false } },
    'label.sustain': { en: { t: 'Sustain' }, fr: { t: 'Maintien', reviewed: false } },
    'label.release': { en: { t: 'Release' }, fr: { t: 'Relâche', reviewed: false } },

    // ── Voice / Output group ────────────────────────────────────────────────
    'label.groupOutput': {
        en: { t: 'Voice / Output' },
        fr: { t: 'Voix / sortie', reviewed: false },
    },
    'label.mode':  { en: { t: 'Mode' },  fr: { t: 'Mode', sameAsEn: true, reviewed: false } },
    'label.glide': { en: { t: 'Glide' }, fr: { t: 'Glissando', reviewed: false } },
    'label.level': { en: { t: 'Level' }, fr: { t: 'Niveau', reviewed: false } },

    // ── Preset tour ─────────────────────────────────────────────────────────
    // The button FACES are localized; the data-preset beside each one is the
    // C++ snapshot name and is never translated (it is the applyFactoryPreset
    // argument and the LESSON_CAPTION_WRITERS key in js/app.js).
    'label.tourLabel': {
        en: { t: 'Lesson Presets' },
        fr: { t: 'Leçons', reviewed: false },
    },
    'label.lessonSawSweep':   { en: { t: 'Saw Sweep' },        fr: { t: 'Balayage scie', reviewed: false } },
    'label.lessonPluck':      { en: { t: 'Pluck' },            fr: { t: 'Pincement', reviewed: false } },
    'label.lessonBrass':      { en: { t: 'Brass Stab' },       fr: { t: 'Coup de cuivres', reviewed: false } },
    'label.lessonSweep':      { en: { t: 'Sweep Pad' },        fr: { t: 'Nappe balayée', reviewed: false } },
    'label.lessonAcid':       { en: { t: 'Acid Bass' },        fr: { t: 'Basse acid', reviewed: false } },
    'label.lessonSquareBass': { en: { t: 'Square Bass' },      fr: { t: 'Basse carrée', reviewed: false } },
    'label.lessonNoiseWind':  { en: { t: 'Noise Wind' },       fr: { t: 'Vent de bruit', reviewed: false } },
    'label.lessonSelfOsc':    { en: { t: 'Self-Oscillation' }, fr: { t: 'Auto-oscillation', reviewed: false } },

    // The tour caption. Nine entries: the resting one authored in the markup,
    // and one per lesson written by setLabel when a button is clicked. Through
    // v1.2.5 the eight lesson captions lived in a LESSONS table in js/app.js
    // and were written with a raw textContent assignment; a string written that
    // way is stranded in the language it was picked in the instant the selector
    // fires, and it is the one string on this page chosen by a click.
    'label.captionDefault': {
        en: { t: 'Hover any control for an explanation · pick a lesson to hear a concept.' },
        fr: { t: 'Survolez n’importe quel réglage pour une explication · choisissez une leçon pour entendre un concept.',
              reviewed: false },
    },
    'label.captionSawSweep': {
        en: { t: 'Saw → LP Sweep — a bright saw through a 24 dB low-pass with a slow filter envelope. Watch the harmonics fall away under the curve: the subtraction the method is named for.' },
        fr: { t: 'Dent de scie → balayage passe-bas — une dent de scie brillante à travers un passe-bas 24 dB avec une enveloppe de filtre lente. Regardez les harmoniques disparaître sous la courbe : la soustraction qui donne son nom à la méthode.',
              reviewed: false },
    },
    'label.captionPluck': {
        en: { t: 'Pluck — a fast filter envelope snaps bright-then-dark while the amp decays quickly; the filter envelope does the timbral work.' },
        fr: { t: 'Pincement — une enveloppe de filtre rapide fait claquer le son du brillant vers le sombre pendant que l’amplitude décline vite ; c’est l’enveloppe de filtre qui fait le travail de timbre.',
              reviewed: false },
    },
    'label.captionBrass': {
        en: { t: 'Brass Stab — positive filter-env amount opens the cutoff with the attack and holds it, so brightness tracks the note like brass.' },
        fr: { t: 'Coup de cuivres — un taux d’enveloppe de filtre positif ouvre la coupure avec l’attaque et la tient, si bien que la brillance suit la note comme un cuivre.',
              reviewed: false },
    },
    'label.captionSweep': {
        en: { t: 'Sweep Pad — slow amp swell + a long, deep filter sweep open the spectrum gradually. Pads live in slow envelopes.' },
        fr: { t: 'Nappe balayée — une montée d’amplitude lente et un balayage de filtre long et profond ouvrent le spectre peu à peu. Les nappes vivent dans les enveloppes lentes.',
              reviewed: false },
    },
    'label.captionAcid': {
        en: { t: 'Acid Bass — high resonance and a snappy filter envelope through a 24 dB low-pass make the squelchy, ringing acid sound. Mono with a touch of glide.' },
        fr: { t: 'Basse acid — une résonance élevée et une enveloppe de filtre nerveuse à travers un passe-bas 24 dB donnent le son acid gluant et sonnant. En mono, avec un soupçon de glissando.',
              reviewed: false },
    },
    'label.captionSquareBass': {
        en: { t: 'Square Bass — a hollow square plus the sub-oscillator for weight, played mono. The same voice as a polysynth, just one note at a time.' },
        fr: { t: 'Basse carrée — une onde carrée creuse plus le sous-oscillateur pour le poids, jouée en mono. La même voix qu’un polysynthé, mais une seule note à la fois.',
              reviewed: false },
    },
    'label.captionNoiseWind': {
        en: { t: 'Noise Wind — band-passed white noise with no harmonic source: the filter sculpts pitchless air into wind. Subtractive synthesis beyond notes.' },
        fr: { t: 'Vent de bruit — du bruit blanc en passe-bande, sans source harmonique : le filtre sculpte de l’air sans hauteur en vent. La synthèse soustractive au-delà des notes.',
              reviewed: false },
    },
    'label.captionSelfOsc': {
        en: { t: 'Self-Oscillation — resonance at the limit: the filter rings into a pure sine that plays in tune across the keyboard. The filter becomes the source.' },
        fr: { t: 'Auto-oscillation — résonance à la limite : le filtre sonne en une sinusoïde pure qui joue juste sur tout le clavier. Le filtre devient la source.',
              reviewed: false },
    },

    // ── Keyboard ────────────────────────────────────────────────────────────
    'label.play': {
        en: { t: 'Play ·' },
        fr: { t: 'Jouer ·', reviewed: false },
    },
    // The letter run is the QWERTY key map, not prose: it names physical keys
    // and stays exactly as it is in both languages. Only the sentence around it
    // is translated.
    //
    // The letter run keeps its HAIR SPACES (U+200A, `&#8202;` in the markup) as
    // \u200a escapes. applyLabel writes this string over the authored markup,
    // so a plain space here would silently widen the key run in BOTH languages
    // — invisible to an en-vs-fr geometry diff, and a change to the shipped
    // English nobody asked for.
    'label.kbdHint': {
        en: { t: 'click the keys or use your computer keyboard (A\u200aS\u200aD\u200aF\u200aG\u200aH\u200aJ\u200aK · W\u200aE\u200aT\u200aY\u200aU)' },
        fr: { t: 'cliquez les touches ou utilisez le clavier de l’ordinateur (A\u200aS\u200aD\u200aF\u200aG\u200aH\u200aJ\u200aK · W\u200aE\u200aT\u200aY\u200aU)',
              reviewed: false },
    },

    // ── Accessible names ────────────────────────────────────────────────────
    // Read by data-i18n-aria. The knobs, combos and the two focusable canvases
    // read their tooltip TITLE through trLabel's I18N fallback instead — an
    // accessible name IS the control's name — so only the names that have no
    // tooltip live here.
    'aria.settings': {
        en: { t: 'Settings' }, fr: { t: 'Réglages', reviewed: false },
    },
    'aria.langSelect': {
        en: { t: 'Interface language' }, fr: { t: 'Langue de l’interface', reviewed: false },
    },
    'aria.keyboard': {
        en: { t: 'On-screen keyboard' }, fr: { t: 'Clavier à l’écran', reviewed: false },
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
    // The h1 splits the product name across two text nodes so the second half
    // can carry the green italic .title-accent. Both halves are the same
    // untranslatable name; keying either would translate half a wordmark.
    ['O – simple',
     'the product name, first half of the split wordmark in the page heading — a product name is never translated'],
    ['Subtractive',
     'the product name, second half of the split wordmark (.title-accent) — a product name is never translated'],

    // The two endonyms in the language selector. A language name is written in
    // its OWN language: a French speaker looking for their language looks for
    // "Français", not "French".
    ['English',  'endonym — a language name is never translated'],
    ['Français', 'endonym — a language name is never translated'],

    // ── The signal-path diagram's three VALUE displays ──────────────────────
    // #oscModeText, #filterModeText and #routeMeta are written by
    // updateDiagram() from the four AudioParameterChoice values. Those choice
    // strings are the host automation contract and stay English under D-01;
    // translating the diagram without translating the automation lane would
    // make the page and the host disagree about what the plugin is set to. The
    // strings below are the authored MARKUP DEFAULTS for the same three nodes,
    // overwritten by updateDiagram on the first frame.
    ['Saw',
     'AudioParameterChoice entry (oscWave), mirrored in the OSC node of the diagram — the host automation name, English under D-01'],
    ['24 dB LP',
     'the FILTER node value display, composed from the filterSlope and filterType choice values — English under D-01'],
    ['low-pass · 24 dB/oct',
     'the routing meta readout, composed from the filterType and filterSlope choice values — English under D-01'],
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
// index.html to carry zero data-tip literals. The twenty-two parameter cells
// gained a data-param attribute naming the APVTS parameter they drive; the
// three panels and the two envelope canvases gained an id; the eight lesson
// buttons are addressed by the data-preset they already carried.
// ============================================================================

export const TIP_BINDINGS = [
    ['#gear-btn',                       'gear-btn'],
    ['#lang-select',                    'lang-select'],

    ['#headlineWrap',                   'headline'],
    ['#scopeWrap',                      'scope'],
    ['#routingPanel',                   'routing'],

    ['[data-param="oscWave"]',          'oscWave'],
    ['[data-param="subLevel"]',         'subLevel'],
    ['[data-param="noiseLevel"]',       'noiseLevel'],

    ['[data-param="filterType"]',       'filterType'],
    ['[data-param="filterSlope"]',      'filterSlope'],
    ['[data-param="cutoff"]',           'cutoff'],
    ['[data-param="resonance"]',        'resonance'],
    ['[data-param="filterEnvAmount"]',  'filterEnvAmount'],
    ['[data-param="keyTrack"]',         'keyTrack'],

    ['#filterAdsrWrap',                 'filterAdsr'],
    ['[data-param="filterAttack"]',     'filterAttack'],
    ['[data-param="filterDecay"]',      'filterDecay'],
    ['[data-param="filterSustain"]',    'filterSustain'],
    ['[data-param="filterRelease"]',    'filterRelease'],

    ['#ampAdsrWrap',                    'ampAdsr'],
    ['[data-param="ampAttack"]',        'ampAttack'],
    ['[data-param="ampDecay"]',         'ampDecay'],
    ['[data-param="ampSustain"]',       'ampSustain'],
    ['[data-param="ampRelease"]',       'ampRelease'],

    ['[data-param="voiceMode"]',        'voiceMode'],
    ['[data-param="glide"]',            'glide'],
    ['[data-param="outputLevel"]',      'outputLevel'],

    ['.tour-btn[data-preset="Saw Sweep"]',        'lessonSawSweep'],
    ['.tour-btn[data-preset="Pluck"]',            'lessonPluck'],
    ['.tour-btn[data-preset="Brass Stab"]',       'lessonBrass'],
    ['.tour-btn[data-preset="Sweep Pad"]',        'lessonSweep'],
    ['.tour-btn[data-preset="Acid Bass"]',        'lessonAcid'],
    ['.tour-btn[data-preset="Square Bass"]',      'lessonSquareBass'],
    ['.tour-btn[data-preset="Noise Wind"]',       'lessonNoiseWind'],
    ['.tour-btn[data-preset="Self-Oscillation"]', 'lessonSelfOsc'],
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
