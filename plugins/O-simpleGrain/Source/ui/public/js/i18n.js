/*
   This file is part of O-simpleGrain, an Ouaricon Audio plugin.
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
// i18n.js — O-simpleGrain interface copy, English + French (v1.3.0)
//
// UI ROOT IS Source/ui/public. There is no second UI root in this plugin and no
// Resources/ui staging directory. This file is a SOURCES entry in
// juce_add_binary_data(O-simpleGrain_UIResources) and is served by
// PluginEditor::getResource at /js/i18n.js. Embedded but not served, or served
// but not embedded, is a 404 that presents as a blank page — the import in
// app.js fails to resolve and module evaluation never starts. check-i18n
// assertion 8 checks both halves.
//
// NOTE the NAMESPACE: this plugin has TWO juce_add_binary_data targets, and the
// UI one uses NAMESPACE UIBinaryData / HEADER_NAME UIBinaryData.h so it cannot
// collide with the O-simpleGrain_Samples target that owns BinaryData:: for the
// four embedded .wav sources. The symbol this file becomes is therefore
// UIBinaryData::i18n_js, not BinaryData::i18n_js.
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
// COPY IS textContent ON EVERY PATH — never innerHTML. v1.2.1's tooltip
// renderer built its tip with tip.innerHTML = ...; v1.3.0 builds it with
// createElement + textContent, because the tip text is now table-sourced and
// localized rather than a fixed literal. check-i18n assertion 9 rejects any
// innerHTML reference here and any string literal containing an opening angle
// bracket.
//
// THE ENGLISH WAS MOVED, NOT REWRITTEN. Every en entry below was extracted
// mechanically from v1.2.1's TIPS and LESSONS tables in js/app.js, from the
// toast / source-status literals in the same file, and from index.html, then
// compared back to the source with entities decoded — never re-typed. HTML
// entities are decoded to the characters they named (&#183; to ·, &#8594; to →,
// &nbsp; to a \u00a0 escape and &#8202; to a \u200a escape, NOT to plain
// spaces) because setAttribute and textContent do not decode entities.
//
// ONE DELIBERATE ENGLISH CHANGE, recorded in the CHANGELOG: the tip bodies have
// lost their strong/em/code emphasis tags. The WORDS are unchanged. Assertion 9
// forbids an angle bracket in a string literal here, and it is right to: the
// renderer now writes textContent, so a tag would render as literal characters
// rather than as emphasis.
//
// KEYS ARE THE PARAMETER ID where the anchor is a parameter cell, and a
// label.* / aria.* / toast.* / ui.* slug otherwise. The parameter cells are
// addressed by the data-param attribute MOVED IN v1.3.0 from the inner .knob
// div (where nothing read it) up to the cell; through v1.2.1 the cells carried
// the tip KEY in their own data-tip attribute, and a .knob-cell selector would
// have matched the FIRST of fifteen — the failure canon section 1 names and the
// one O-Octagon's .vunit-group tip hit for real in Stage C.
//
// LABELS NEVER REUSE A TOOLTIP KEY for a caption. trLabel() falls back to I18N
// and a few of this page's captions do happen to equal their tip title today
// ("Scatter", "Position") — but most do not ("Size" vs "Grain Size", "Level" vs
// "Output Level", "Shape" vs "Window Shape"), and a rule that holds for two of
// twenty-two is a rule nobody can apply. A caption and a tip title also diverge
// the moment either is edited, which would make a tooltip copy edit a silent
// geometry change to a control. The ONE place the fallback is used on purpose
// is data-i18n-aria on the knobs, combos, toggles and the Load button: an
// accessible name IS the control's name, so it reads the tooltip title by
// design.
//
// THE TWO DROP-DOWN MENUS STAY ENGLISH. combo-sourceSample and
// combo-windowShape are filled at runtime from the C++ AudioParameterChoice
// strings ("Fire"/"Voice"/"Water"/"Piano", "Rectangular"/…/"Hann"). Those are
// the host automation contract and stay English under D-01; translating the
// page without translating the automation lane would make the page and the host
// disagree about what the plugin is set to. They are not authored in the markup
// and so are not in this table at all.
//
// ALL FRENCH IS MACHINE-DRAFTED AND FLAGGED reviewed: false. No native speaker
// has read it. node scripts/check-i18n.js prints the worklist.
// ============================================================================

export const LANGUAGES = ['en', 'fr'];

export const I18N = Object.freeze({

    // ── The settings popover (v1.3.0)
    // The gear is new. The page already had a hover-help toggle — a "?" chip in the
    // preset bar, backed by localStorage — and it MOVED in here rather than sitting
    // beside a gear: a plugin should not grow a second settings surface. ────
    'gear-btn': {
        en: { t: "Settings",
              b: "Choose the language of the interface, and whether the hover help appears at all. The language is remembered with the session; the hover-help switch is remembered on this computer." },
        fr: { t: "Réglages",
              b: "Choisir la langue de l'interface et l'affichage de l'aide au survol. La langue est conservée avec la session ; l'état de l'aide est conservé sur cet ordinateur.",
              reviewed: false },
    },
    'lang-select': {
        en: { t: "Language",
              b: "The language of the labels on this page and of this hover help. English and French are available; value readouts and the two drop-down menus stay in English." },
        fr: { t: "Langue",
              b: "La langue des libellés de cette page et de cette aide au survol. L'anglais et le français sont disponibles ; les valeurs affichées et les deux menus déroulants restent en anglais.",
              reviewed: false },
    },
    'help-toggle': {
        en: { t: "Hover help",
              b: "Turns these hover explanations off or back on. The switch is remembered on this computer rather than in the session, so it follows you from one project to the next." },
        fr: { t: "Aide au survol",
              b: "Active ou désactive ces explications au survol. Le réglage est conservé sur cet ordinateur et non dans la session : il vous suit d'un projet à l'autre.",
              reviewed: false },
    },

    // ── Source ──────────────────────────────────────────────────────────────
    sourceSample: {
        en: { t: "Source Sample",
              b: "The short sound the synth chops into grains. Granular synthesis never makes tone from scratch — it sprinkles tiny slices of this recording. Pick fire, voice, water, or piano; or drop your own below." },
        fr: { t: "Échantillon source",
              b: "Le son court que le synthé découpe en grains. La synthèse granulaire ne fabrique jamais un timbre de zéro — elle saupoudre de minuscules tranches de cet enregistrement. Choisissez feu, voix, eau ou piano ; ou déposez le vôtre ci-dessous.",
              reviewed: false },
    },
    loadSource: {
        en: { t: "Load your own",
              b: "Open a file picker to granulate any short .wav / .aif (capped at 10\u00a0s). The same grain controls then operate on your sound — the engine doesn't care what the source is." },
        fr: { t: "Charger le vôtre",
              b: "Ouvre un sélecteur de fichier pour granuler n'importe quel .wav / .aif court (limité à 10\u00a0s). Les mêmes commandes de grain agissent alors sur votre son — le moteur se moque de la source.",
              reviewed: false },
    },
    dropZone: {
        en: { t: "Drop a source",
              b: "Drag a .wav / .aif here to granulate your own sound. Files over 10\u00a0s are trimmed (you'll see a notice). Try a spoken word or a field recording — granular makes textures out of anything." },
        fr: { t: "Déposer une source",
              b: "Glissez un .wav / .aif ici pour granuler votre propre son. Les fichiers de plus de 10\u00a0s sont rognés (un avis s'affiche). Essayez un mot parlé ou un enregistrement de terrain — le granulaire fait des textures avec tout.",
              reviewed: false },
    },

    // ── Grain ───────────────────────────────────────────────────────────────
    grainSize: {
        en: { t: "Grain Size",
              b: "How long each slice is, 2–500\u00a0ms. This is the buzz\u00a0↔\u00a0fragments axis: very short grains (a few\u00a0ms) lose the source and turn to tone; long grains (>60\u00a0ms) keep recognisable chunks. With Density it sets how deeply grains overlap (overlap = size\u00a0×\u00a0density)." },
        fr: { t: "Taille du grain",
              b: "Durée de chaque tranche, 2–500\u00a0ms. C'est l'axe bourdonnement\u00a0↔\u00a0fragments : les grains très courts (quelques\u00a0ms) perdent la source et deviennent un timbre ; les grains longs (>60\u00a0ms) gardent des morceaux reconnaissables. Avec la densité, cela fixe la profondeur du recouvrement des grains (recouvrement = taille\u00a0×\u00a0densité).",
              reviewed: false },
    },
    density: {
        en: { t: "Density",
              b: "Grains fired per second, 1–200. Sparse = you hear separated grains; dense = they fuse into a continuous cloud. Overlap-add only sounds smooth when many grains overlap, so Density and Size work together (watch the Overlap readout)." },
        fr: { t: "Densité",
              b: "Grains déclenchés par seconde, 1–200. Clairsemé = vous entendez des grains séparés ; dense = ils fusionnent en un nuage continu. La sommation par recouvrement ne sonne lisse que si beaucoup de grains se recouvrent : densité et taille travaillent ensemble (surveillez l'affichage Overlap).",
              reviewed: false },
    },
    position: {
        en: { t: "Position",
              b: "Where in the source the read head rests, 0–100\u00a0%. It's the point grains are sliced from — sweep it to scrub through the recording. Pairs with Scan (which moves the head) and Freeze (which pins it)." },
        fr: { t: "Position",
              b: "L'endroit de la source où repose la tête de lecture, 0–100\u00a0%. C'est le point où les grains sont découpés — balayez-le pour parcourir l'enregistrement. Va de pair avec Scan (qui déplace la tête) et Freeze (qui la fige).",
              reviewed: false },
    },
    scan: {
        en: { t: "Scan / Time-Stretch",
              b: "How fast the read head travels, −200…+200\u00a0%. 0\u00a0% holds on one instant; below 100\u00a0% stretches the source in time without changing pitch; negative scans backwards. This is granular time-stretch." },
        fr: { t: "Balayage / étirement temporel",
              b: "Vitesse de déplacement de la tête de lecture, −200…+200\u00a0%. À 0\u00a0% elle reste sur un instant ; sous 100\u00a0% la source est étirée dans le temps sans changer de hauteur ; en négatif le balayage se fait à l'envers. C'est l'étirement temporel granulaire.",
              reviewed: false },
    },
    freeze: {
        en: { t: "Freeze",
              b: "Pins the read head on the current instant and sustains it forever — the grain stream keeps flowing but never advances through the source. Add Pitch Spray for a shimmering frozen pad. The pin crossfades in, so no click." },
        fr: { t: "Gel",
              b: "Fige la tête de lecture sur l'instant courant et le tient indéfiniment — le flux de grains continue mais n'avance plus dans la source. Ajoutez du Pitch Spray pour une nappe gelée scintillante. Le gel s'enclenche en fondu, donc sans clic.",
              reviewed: false },
    },

    // ── Window ──────────────────────────────────────────────────────────────
    windowShape: {
        en: { t: "Window Shape",
              b: "The fade envelope on each grain. Hann/Gauss fade in and out smoothly so overlapping grains crossfade cleanly. Rectangular is flat with only a 1\u00a0ms guard at each edge — hard-edged and buzzy, the rough end of the lesson. Tukey sits in between: a flat top with a Hann-shaped fade whose length is set by Taper." },
        fr: { t: "Forme de fenêtre",
              b: "L'enveloppe de fondu appliquée à chaque grain. Hann/Gauss entrent et sortent en douceur, si bien que les grains superposés se fondent proprement. Rectangular est plate avec seulement 1\u00a0ms de garde à chaque bord — dure et bourdonnante, l'extrémité rugueuse de la leçon. Tukey se situe entre les deux : un sommet plat avec un fondu en forme de Hann dont la longueur est réglée par Taper.",
              reviewed: false },
    },
    windowTaper: {
        en: { t: "Taper",
              b: "Tukey only: how much of each grain is spent fading, 0–100\u00a0%, split between the two edges. 0\u00a0% is the flat rectangular window (guard-faded); 100\u00a0% is a full Hann. In between you keep a flat, loud middle and buy just enough fade to stop the edges clicking." },
        fr: { t: "Fondu",
              b: "Tukey uniquement : la part de chaque grain passée en fondu, 0–100\u00a0%, répartie entre les deux bords. 0\u00a0% est la fenêtre rectangulaire plate (avec garde) ; 100\u00a0% est un Hann complet. Entre les deux, on garde un milieu plat et fort et on achète juste assez de fondu pour que les bords cessent de claquer.",
              reviewed: false },
    },

    // ── Spray and scatter ───────────────────────────────────────────────────
    pitchSpray: {
        en: { t: "Pitch Spray",
              b: "Random per-grain transposition, 0–12\u00a0st. Each grain is nudged up/down by a random amount, so a frozen or static texture starts to shimmer and thicken instead of sitting on one dead pitch." },
        fr: { t: "Dispersion de hauteur",
              b: "Transposition aléatoire grain par grain, 0–12\u00a0demi-tons. Chaque grain est décalé au hasard vers le haut ou le bas, si bien qu'une texture gelée ou statique se met à scintiller et à s'épaissir au lieu de rester sur une hauteur morte.",
              reviewed: false },
    },
    positionSpray: {
        en: { t: "Position Spray",
              b: "Random per-grain read position, 0–100\u00a0%. Scatters where each grain is sliced from around the Position point — turns a tight read into a wash drawn from a whole region of the source (shown as the green band on the waveform)." },
        fr: { t: "Dispersion de position",
              b: "Position de lecture aléatoire grain par grain, 0–100\u00a0%. Disperse l'endroit d'où chaque grain est découpé autour du point Position — transforme une lecture serrée en un lavis puisé dans toute une région de la source (visible en bande verte sur la forme d'onde).",
              reviewed: false },
    },
    scatter: {
        en: { t: "Scatter",
              b: "Randomises the timing of grains, 0–100\u00a0%. This is the synchronous\u00a0↔\u00a0asynchronous axis: at 0\u00a0% grains fire on a perfect clock (a pitched comb, discrete sidebands); high scatter dissolves the comb into broadband noise. Watch the Spectrum." },
        fr: { t: "Étalement temporel",
              b: "Rend aléatoire le moment de déclenchement des grains, 0–100\u00a0%. C'est l'axe synchrone\u00a0↔\u00a0asynchrone : à 0\u00a0% les grains suivent une horloge parfaite (un peigne harmonique, des bandes latérales discrètes) ; une forte dispersion dissout le peigne en bruit large bande. Regardez le spectre.",
              reviewed: false },
    },
    grainPitch: {
        en: { t: "Grain Pitch",
              b: "Global transposition of every grain, −24…+24\u00a0st. Stacks on top of MIDI key-tracking and Pitch Spray — shift the whole cloud up an octave without touching the keyboard." },
        fr: { t: "Hauteur du grain",
              b: "Transposition globale de tous les grains, −24…+24\u00a0demi-tons. S'ajoute au suivi de clavier MIDI et à la dispersion de hauteur — montez tout le nuage d'une octave sans toucher au clavier.",
              reviewed: false },
    },
    panSpray: {
        en: { t: "Pan Spray",
              b: "Per-grain stereo spread, 0–100\u00a0%. At 0 every grain is centred; raise it and grains scatter left/right (equal-power), widening a mono source into an immersive stereo cloud." },
        fr: { t: "Dispersion stéréo",
              b: "Étalement stéréo grain par grain, 0–100\u00a0%. À 0 chaque grain est centré ; augmentez et les grains se dispersent à gauche et à droite (à puissance constante), élargissant une source mono en un nuage stéréo enveloppant.",
              reviewed: false },
    },
    velToDensity: {
        en: { t: "Velocity → Density",
              b: "How much your playing velocity drives Density, 0–100\u00a0%. At 0 density is fixed; raise it and harder keys spawn thicker clouds (loudness already follows velocity through the amp envelope — this adds thickness on top)." },
        fr: { t: "Vélocité → densité",
              b: "À quel point votre vélocité de jeu pilote la densité, 0–100\u00a0%. À 0 la densité est fixe ; augmentez et les touches jouées fort engendrent des nuages plus épais (le volume suit déjà la vélocité par l'enveloppe d'amplitude — ceci ajoute l'épaisseur par-dessus).",
              reviewed: false },
    },

    // ── Amplitude envelope ──────────────────────────────────────────────────
    adsrEnabled: {
        en: { t: "ADSR On / Off",
              b: "Switches the per-voice amplitude envelope on or off. Off bypasses Attack/Decay/Sustain/Release — each note plays at a flat level while held and, on release, simply stops launching new grains so the cloud fades out over one grain length through the Window envelopes (no click). Turn it on for shaped swells and pads; off for a raw, immediate gate." },
        fr: { t: "ADSR activé / désactivé",
              b: "Active ou désactive l'enveloppe d'amplitude de chaque voix. Désactivée, elle contourne attaque/déclin/maintien/relâchement — chaque note joue à niveau constant tant qu'elle est tenue puis, au relâchement, cesse simplement de lancer de nouveaux grains, si bien que le nuage s'éteint sur la durée d'un grain à travers les enveloppes de fenêtre (sans clic). Activez-la pour des montées et des nappes façonnées ; désactivez-la pour une porte brute et immédiate.",
              reviewed: false },
    },
    ampAttack: {
        en: { t: "Amp Attack",
              b: "How quickly a note fades in, 0–5\u00a0s. This is the per-voice envelope over the whole grain stream — short for percussive, long for a pad swell. (Each grain has its own tiny Window envelope; this is the bigger one.)" },
        fr: { t: "Attaque d'amplitude",
              b: "Rapidité d'entrée en fondu d'une note, 0–5\u00a0s. C'est l'enveloppe de la voix entière sur tout le flux de grains — courte pour du percussif, longue pour une montée de nappe. (Chaque grain a sa propre petite enveloppe de fenêtre ; celle-ci est la grande.)",
              reviewed: false },
    },
    ampDecay: {
        en: { t: "Amp Decay",
              b: "How the note falls from its attack peak toward the sustain level, 0–5\u00a0s. Together with Sustain it shapes the body of a held note." },
        fr: { t: "Déclin d'amplitude",
              b: "Manière dont la note redescend de son pic d'attaque vers le niveau de maintien, 0–5\u00a0s. Avec le maintien, elle façonne le corps d'une note tenue.",
              reviewed: false },
    },
    ampSustain: {
        en: { t: "Amp Sustain",
              b: "The level a held note settles at after the attack/decay, 0–100\u00a0%. 100\u00a0% holds full volume while a key is down; lower it for notes that bloom then back off." },
        fr: { t: "Maintien d'amplitude",
              b: "Le niveau auquel une note tenue se stabilise après l'attaque et le déclin, 0–100\u00a0%. À 100\u00a0% le volume reste plein tant que la touche est enfoncée ; abaissez-le pour des notes qui s'épanouissent puis se retirent.",
              reviewed: false },
    },
    ampRelease: {
        en: { t: "Amp Release",
              b: "How long the note fades out after you let go, 0–5\u00a0s. Long release lets clouds ring on and overlap into the next note — granular pads love a generous release." },
        fr: { t: "Relâchement d'amplitude",
              b: "Durée d'extinction de la note après le relâchement de la touche, 0–5\u00a0s. Un long relâchement laisse les nuages résonner et se superposer à la note suivante — les nappes granulaires adorent un relâchement généreux.",
              reviewed: false },
    },

    // ── Output ──────────────────────────────────────────────────────────────
    outputLevel: {
        en: { t: "Output Level",
              b: "Master volume trim, −∞…0\u00a0dB. Dense overlapping clouds can pile up energy, so trim here if a thick patch peaks. (Headroom normalisation upstream already tames the worst of it.)" },
        fr: { t: "Niveau de sortie",
              b: "Réglage du volume général, −∞…0\u00a0dB. Des nuages denses et superposés peuvent accumuler de l'énergie : baissez ici si un patch épais sature. (La normalisation de marge en amont en dompte déjà le pire.)",
              reviewed: false },
    },

    // ── Visualizations ──────────────────────────────────────────────────────
    vizCloud: {
        en: { t: "Grain Cloud",
              b: "Every grain that spawns drops a sepia dot — horizontal = where in the source it was read, vertical = its pitch, dot size = grain length. Raise Density and the cloud thickens; raise the sprays and it spreads out." },
        fr: { t: "Nuage de grains",
              b: "Chaque grain engendré dépose un point sépia — horizontalement l'endroit de la source où il a été lu, verticalement sa hauteur, la taille du point sa durée. Augmentez la densité et le nuage s'épaissit ; augmentez les dispersions et il s'étale.",
              reviewed: false },
    },
    vizWave: {
        en: { t: "Source Waveform",
              b: "The loaded source drawn as a waveform. The brown line is the live read head (Position + Scan), the green band is the Position-Spray range grains are drawn from, and a snowflake pins the head when Freeze is on." },
        fr: { t: "Forme d'onde de la source",
              b: "La source chargée tracée en forme d'onde. La ligne brune est la tête de lecture en direct (Position + Scan), la bande verte est la plage de dispersion de position dans laquelle les grains sont puisés, et un flocon fige la tête quand Freeze est actif.",
              reviewed: false },
    },
    vizScope: {
        en: { t: "Output Scope",
              b: "The actual audio coming out, plotted as a waveform. Useful for spotting the hard edges of a rectangular window (the clicks) versus the smooth crossfades of Hann." },
        fr: { t: "Oscilloscope de sortie",
              b: "L'audio réellement produit, tracé en forme d'onde. Utile pour repérer les bords durs d'une fenêtre rectangulaire (les clics) face aux fondus doux de Hann.",
              reviewed: false },
    },
    vizSpectrum: {
        en: { t: "Spectrum",
              b: "The frequency content of the output. At Scatter\u00a00 you'll see discrete spikes (the synchronous grain comb — a pitched sound); push Scatter up and the spikes smear into a continuous noise floor. The sync\u00a0→\u00a0async lesson, made visible." },
        fr: { t: "Spectre",
              b: "Le contenu fréquentiel de la sortie. À Scatter\u00a00 vous verrez des pics discrets (le peigne de grains synchrone — un son harmonique) ; montez Scatter et les pics s'étalent en un plancher de bruit continu. La leçon synchrone\u00a0→\u00a0asynchrone, rendue visible.",
              reviewed: false },
    },
    readout: {
        en: { t: "Grain Readout",
              b: "Live cost meter. Grains = active grains out of the 192 global cap. Overlap = grain size × density (how many grains sound at once — over ~2× they fuse). The CPU bar tracks the grain load: density × size × polyphony is what makes granular expensive." },
        fr: { t: "Affichage des grains",
              b: "Compteur de coût en direct. Grains = grains actifs sur la limite globale de 192. Overlap = taille du grain × densité (combien de grains sonnent en même temps — au-delà d'environ 2× ils fusionnent). La barre CPU suit la charge de grains : densité × taille × polyphonie, voilà ce qui rend le granulaire coûteux.",
              reviewed: false },
    },

    // ── Concept presets (the 8-stop tour) ───────────────────────────────────
    lessonSingleGrain: {
        en: { t: "Single Grain",
              b: "One long grain fired slowly — density at the floor so grains stay separated. Hear a single slice on its own: the atom of granular synthesis." },
        fr: { t: "Grain unique",
              b: "Un seul grain long déclenché lentement — densité au plancher pour que les grains restent séparés. Écoutez une tranche seule : l'atome de la synthèse granulaire.",
              reviewed: false },
    },
    lessonPitchedBuzz: {
        en: { t: "Pitched Buzz",
              b: "Tiny grains fired fast and perfectly in sync. The grain rate itself becomes an audible pitch (a comb) — granular can make tone, not just texture." },
        fr: { t: "Bourdon harmonique",
              b: "De minuscules grains déclenchés vite et parfaitement en phase. La cadence des grains devient elle-même une hauteur audible (un peigne) — le granulaire peut faire du timbre, pas seulement de la texture.",
              reviewed: false },
    },
    lessonFragments: {
        en: { t: "Fragments",
              b: "Medium grains, sparse. You still recognise chunks of the source — the middle ground between one grain and a smooth cloud." },
        fr: { t: "Fragments",
              b: "Grains moyens, clairsemés. Vous reconnaissez encore des morceaux de la source — le terrain intermédiaire entre un grain isolé et un nuage lisse.",
              reviewed: false },
    },
    lessonSmoothCloud: {
        en: { t: "Smooth Cloud",
              b: "Many overlapping Hann grains fuse into one continuous, glassy texture. Overlap-add doing its job: size × density well above 1." },
        fr: { t: "Nuage lisse",
              b: "De nombreux grains Hann superposés fusionnent en une texture continue et vitreuse. La sommation par recouvrement à l'œuvre : taille × densité bien au-dessus de 1.",
              reviewed: false },
    },
    lessonFrozenPad: {
        en: { t: "Frozen Pad",
              b: "Freeze pins the read head; Pitch Spray shimmers the frozen instant into a sustained, evolving pad that never moves through the source." },
        fr: { t: "Nappe gelée",
              b: "Freeze fige la tête de lecture ; la dispersion de hauteur fait scintiller l'instant gelé en une nappe tenue et évolutive qui n'avance jamais dans la source.",
              reviewed: false },
    },
    lessonAsyncCloud: {
        en: { t: "Asynchronous Cloud",
              b: "High Scatter randomises the grain timing — the pitched comb dissolves and the spectrum smears into broadband noise. The async end of the axis." },
        fr: { t: "Nuage asynchrone",
              b: "Une forte dispersion temporelle rend aléatoire le déclenchement des grains — le peigne harmonique se dissout et le spectre s'étale en bruit large bande. L'extrémité asynchrone de l'axe.",
              reviewed: false },
    },
    lessonGranularFire: {
        en: { t: "Granular Fire",
              b: "The worked example on the crackling-fire recording: a lively grain/spray set that turns a field recording into a moving granular bed." },
        fr: { t: "Feu granulaire",
              b: "L'exemple travaillé sur l'enregistrement de feu crépitant : un réglage vif de grain et de dispersion qui transforme un enregistrement de terrain en un lit granulaire mouvant.",
              reviewed: false },
    },
    lessonRectClick: {
        en: { t: "Rect Click",
              b: "The rough end of the lesson: a rectangular window is flat with only a 1\u00a0ms guard at each edge, so every grain starts and stops abruptly. Sparse grains let each hard edge stand alone — compare with Hann to hear why windows matter." },
        fr: { t: "Clic rectangulaire",
              b: "L'extrémité rugueuse de la leçon : une fenêtre rectangulaire est plate avec seulement 1\u00a0ms de garde à chaque bord, donc chaque grain démarre et s'arrête brutalement. Des grains clairsemés laissent chaque bord dur isolé — comparez avec Hann pour entendre pourquoi les fenêtres comptent.",
              reviewed: false },
    },
});

// ============================================================================
// LABELS — the page's own captions, v1.3.0
//
// Separate from I18N because a tooltip entry is a {title, body} PAIR and a
// label is one string.
//
// sameAsEn: true marks the seven captions whose faithful French IS the English
// word. It is a declaration, not a skip: assertion 4 rejects a silent
// passthrough and accepts a declared one.
//
// The toast.* and label.source* entries are the strings js/app.js writes at
// runtime through setLabel(). They were flat literals and template strings in
// v1.2.1; the prose outside the interpolation is what made them untranslatable
// where they stood, and the {name} / {error} tokens are the composed shape
// canon section 3 exists for.
//
// ALL FRENCH IS MACHINE-DRAFTED, reviewed: false.
// ============================================================================

export const LABELS = Object.freeze({

    // ── Header ──────────────────────────────────────────────────────────────
    'label.subtitle': {
        en: { t: "Granular Synthesizer · A Field Guide" },
        fr: { t: "Synthétiseur granulaire · un guide de terrain", reviewed: false },
    },
    'aria.presetTour': {
        en: { t: "Concept presets" },
        fr: { t: "Préréglages conceptuels", reviewed: false },
    },
    'label.tourSingleGrain': {
        en: { t: "Single Grain" },
        fr: { t: "Grain unique", reviewed: false },
    },
    'label.tourPitchedBuzz': {
        en: { t: "Pitched Buzz" },
        fr: { t: "Bourdon harmonique", reviewed: false },
    },
    'label.tourFragments': {
        en: { t: "Fragments" },
        fr: { t: "Fragments", sameAsEn: true, reviewed: false },
    },
    'label.tourSmoothCloud': {
        en: { t: "Smooth Cloud" },
        fr: { t: "Nuage lisse", reviewed: false },
    },
    'label.tourFrozenPad': {
        en: { t: "Frozen Pad" },
        fr: { t: "Nappe gelée", reviewed: false },
    },
    'label.tourAsyncCloud': {
        en: { t: "Async Cloud" },
        fr: { t: "Nuage async", reviewed: false },
    },
    'label.tourGranularFire': {
        en: { t: "Granular Fire" },
        fr: { t: "Feu granulaire", reviewed: false },
    },
    'label.tourRectClick': {
        en: { t: "Rect Click" },
        fr: { t: "Clic rect.", reviewed: false },
    },
    'label.tourCaption': {
        en: { t: "Hover any control for an explanation · pick a concept preset to hear it isolated." },
        fr: { t: "Survolez n'importe quelle commande pour une explication · choisissez un préréglage conceptuel pour l'entendre isolé.", reviewed: false },
    },

    // ── Settings popover — the accessible names and the toggle face.
    // ui.on / ui.off are written by applyTipsEnabled() through two setLabel calls
    // behind an if/else, never a ternary in the argument (assertion 13). ────────
    'aria.langSelect': {
        en: { t: "Interface language" },
        fr: { t: "Langue de l'interface", reviewed: false },
    },
    'aria.helpToggle': {
        en: { t: "Toggle tooltips" },
        fr: { t: "Activer ou désactiver les infobulles", reviewed: false },
    },
    'ui.on': {
        en: { t: "On" },
        fr: { t: "Activée", reviewed: false },
    },
    'ui.off': {
        en: { t: "Off" },
        fr: { t: "Désactivée", reviewed: false },
    },

    // ── Visualization captions. Each viz-label is a caption span PLUS a hint span:
    // applyLabel writes textContent, so keying the parent would delete the hint.
    //
    // MEASURED, AND FLAGGED FOR THE REVIEWER: label.vizScope and its hint are the
    // one pair on this page where a faithful French did not fit. The scope cell
    // caption box is 261px; the English pair is 11.0px tall (one line) and the
    // longer French draft was 22.0px (two), which shrank the scope canvas under
    // it by 11px in French only and made the two spans union rects intersect
    // where they are disjoint in English (check-ui-labels assertion 8). The
    // other three cells are two lines in BOTH languages already, so only this
    // one flags. The draft was Oscilloscope de sortie plus la forme d onde
    // apres gain; it is now Oscilloscope plus l onde apres le gain, one line at
    // 261px and still accurate. A native-speaker reviewer may prefer the longer
    // pair; the cost of taking it back is 11px of scope canvas in French. ────
    'label.vizCloud': {
        en: { t: "Grain Cloud ·" },
        fr: { t: "Nuage de grains ·", reviewed: false },
    },
    'label.vizCloudHint': {
        en: { t: "each grain scatters as a dot (read-position × pitch)" },
        fr: { t: "chaque grain se dépose en point (position de lecture × hauteur)", reviewed: false },
    },
    'label.vizWave': {
        en: { t: "Source Waveform ·" },
        fr: { t: "Forme d'onde source ·", reviewed: false },
    },
    'label.vizWaveHint': {
        en: { t: "playhead, freeze pin & spray range" },
        fr: { t: "tête de lecture, épingle de gel et plage de dispersion", reviewed: false },
    },
    'label.windowInset': {
        en: { t: "Envelope" },
        fr: { t: "Enveloppe", reviewed: false },
    },
    'label.vizScope': {
        en: { t: "Output Scope ·" },
        fr: { t: "Oscilloscope ·", reviewed: false },
    },
    'label.vizScopeHint': {
        en: { t: "the post-gain waveform" },
        fr: { t: "l'onde après le gain", reviewed: false },
    },
    'label.vizSpectrum': {
        en: { t: "Spectrum ·" },
        fr: { t: "Spectre ·", reviewed: false },
    },
    'label.vizSpectrumHint': {
        en: { t: "discrete sidebands at scatter 0 → noise as scatter rises" },
        fr: { t: "bandes latérales discrètes à étalement 0 → bruit quand il monte", reviewed: false },
    },
    'label.readoutGrains': {
        en: { t: "Grains" },
        fr: { t: "Grains", sameAsEn: true, reviewed: false },
    },
    'label.readoutOverlap': {
        en: { t: "Overlap" },
        fr: { t: "Recouvrement", reviewed: false },
    },
    'label.readoutCpu': {
        en: { t: "CPU" },
        fr: { t: "CPU", sameAsEn: true, reviewed: false },
    },

    // ── Side rail ───────────────────────────────────────────────────────────
    'label.groupSource': {
        en: { t: "Source" },
        fr: { t: "Source", sameAsEn: true, reviewed: false },
    },
    'label.btnLoad': {
        en: { t: "Load…" },
        fr: { t: "Charger…", reviewed: false },
    },
    'label.dropZone': {
        en: { t: "Drag a .wav / .aif here to granulate your own sound" },
        fr: { t: "Glissez un .wav / .aif ici pour granuler votre propre son", reviewed: false },
    },
    'label.groupGrain': {
        en: { t: "Grain" },
        fr: { t: "Grain", sameAsEn: true, reviewed: false },
    },
    'label.knobSize': {
        en: { t: "Size" },
        fr: { t: "Taille", reviewed: false },
    },
    'label.knobDensity': {
        en: { t: "Density" },
        fr: { t: "Densité", reviewed: false },
    },
    'label.knobPosition': {
        en: { t: "Position" },
        fr: { t: "Position", sameAsEn: true, reviewed: false },
    },
    'label.knobScan': {
        en: { t: "Scan" },
        fr: { t: "Balayage", reviewed: false },
    },
    'label.toggleFreeze': {
        en: { t: "Freeze" },
        fr: { t: "Gel", reviewed: false },
    },
    'label.groupWindow': {
        en: { t: "Window" },
        fr: { t: "Fenêtre", reviewed: false },
    },
    'label.knobShape': {
        en: { t: "Shape" },
        fr: { t: "Forme", reviewed: false },
    },
    'label.knobTaper': {
        en: { t: "Taper" },
        fr: { t: "Fondu", reviewed: false },
    },
    'label.groupSpray': {
        en: { t: "Spray & Scatter" },
        fr: { t: "Dispersion & étalement", reviewed: false },
    },
    'label.knobPitchSpray': {
        en: { t: "Pitch Spray" },
        fr: { t: "Dispersion hauteur", reviewed: false },
    },
    'label.knobPosSpray': {
        en: { t: "Pos Spray" },
        fr: { t: "Dispersion position", reviewed: false },
    },
    'label.knobScatter': {
        en: { t: "Scatter" },
        fr: { t: "Étalement", reviewed: false },
    },
    'label.knobGrainPitch': {
        en: { t: "Grain Pitch" },
        fr: { t: "Hauteur du grain", reviewed: false },
    },
    'label.knobPanSpray': {
        en: { t: "Pan Spray" },
        fr: { t: "Dispersion stéréo", reviewed: false },
    },
    'label.knobVelDensity': {
        en: { t: "Vel→Density" },
        fr: { t: "Vél.→Densité", reviewed: false },
    },
    'label.groupEnv': {
        en: { t: "Amplitude Envelope" },
        fr: { t: "Enveloppe d'amplitude", reviewed: false },
    },
    'label.toggleAdsr': {
        en: { t: "ADSR" },
        fr: { t: "ADSR", sameAsEn: true, reviewed: false },
    },
    'label.knobAttack': {
        en: { t: "Attack" },
        fr: { t: "Attaque", reviewed: false },
    },
    'label.knobDecay': {
        en: { t: "Decay" },
        fr: { t: "Déclin", reviewed: false },
    },
    'label.knobSustain': {
        en: { t: "Sustain" },
        fr: { t: "Maintien", reviewed: false },
    },
    'label.knobRelease': {
        en: { t: "Release" },
        fr: { t: "Relâchement", reviewed: false },
    },
    'label.groupOutput': {
        en: { t: "Output" },
        fr: { t: "Sortie", reviewed: false },
    },
    'label.knobLevel': {
        en: { t: "Level" },
        fr: { t: "Niveau", reviewed: false },
    },

    // ── On-screen keyboard ──────────────────────────────────────────────────
    'label.keyboard': {
        en: { t: "Play ·" },
        fr: { t: "Jouer ·", reviewed: false },
    },
    'label.keyboardHint': {
        en: { t: "click the keys or use your computer keyboard (A\u200aS\u200aD\u200aF\u200aG\u200aH\u200aJ\u200aK · W\u200aE\u200aT\u200aY\u200aU)" },
        fr: { t: "cliquez les touches ou utilisez le clavier de votre ordinateur (A S D F G H J K · W E T Y U)", reviewed: false },
    },
    'aria.keyboard': {
        en: { t: "On-screen keyboard" },
        fr: { t: "Clavier à l'écran", reviewed: false },
    },

    // ── Drop / load status, written from script through setLabel() ──────────
    'toast.loading': {
        en: { t: "Loading {name}…" },
        fr: { t: "Chargement de {name}…", reviewed: false },
    },
    'toast.dropStartFailed': {
        en: { t: "Drop session start failed" },
        fr: { t: "Échec du démarrage du dépôt", reviewed: false },
    },
    'toast.transferFailed': {
        en: { t: "File transfer failed" },
        fr: { t: "Échec du transfert du fichier", reviewed: false },
    },
    'toast.commitFailed': {
        en: { t: "File load failed at commit" },
        fr: { t: "Échec du chargement du fichier à la validation", reviewed: false },
    },
    'toast.dropFailed': {
        en: { t: "Drop failed: {error}" },
        fr: { t: "Échec du dépôt : {error}", reviewed: false },
    },
    'toast.dropFolder': {
        en: { t: "Drop a single audio file, not a folder" },
        fr: { t: "Déposez un seul fichier audio, pas un dossier", reviewed: false },
    },
    'toast.dropFileType': {
        en: { t: "Drop a .wav / .aif / .aiff file" },
        fr: { t: "Déposez un fichier .wav / .aif / .aiff", reviewed: false },
    },
    'toast.loadFailed': {
        en: { t: "Load failed" },
        fr: { t: "Échec du chargement", reviewed: false },
    },
    'label.sourceTruncated': {
        en: { t: "{name} — truncated to 10 s" },
        fr: { t: "{name} — rogné à 10 s", reviewed: false },
    },
    'label.sourceLoaded': {
        en: { t: "{name} loaded" },
        fr: { t: "{name} chargé", reviewed: false },
    },
    'label.sourceTruncatedGeneric': {
        en: { t: "Source — truncated to 10 s" },
        fr: { t: "Source — rognée à 10 s", reviewed: false },
    },
    'label.sourceLoadedGeneric': {
        en: { t: "Source loaded" },
        fr: { t: "Source chargée", reviewed: false },
    },

    // ── Lesson captions. Chosen by a click, so never the resting string — written
    // raw they would be stranded in whichever language the button was pressed in. ────
    'label.captionSingleGrain': {
        en: { t: "Single Grain — one long grain fired slowly. Density at the floor keeps grains separated: hear a single slice on its own, the atom of granular synthesis." },
        fr: { t: "Grain unique — un seul grain long déclenché lentement. La densité au plancher garde les grains séparés : écoutez une tranche seule, l'atome de la synthèse granulaire.", reviewed: false },
    },
    'label.captionPitchedBuzz': {
        en: { t: "Pitched Buzz — tiny grains fired fast and perfectly in sync. The grain rate itself becomes an audible pitch (a comb). Granular can make tone, not just texture." },
        fr: { t: "Bourdon harmonique — de minuscules grains déclenchés vite et parfaitement en phase. La cadence des grains devient elle-même une hauteur audible (un peigne). Le granulaire peut faire du timbre, pas seulement de la texture.", reviewed: false },
    },
    'label.captionFragments': {
        en: { t: "Fragments — medium grains, sparse. You still recognise chunks of the source: the middle ground between one grain and a smooth cloud." },
        fr: { t: "Fragments — grains moyens, clairsemés. Vous reconnaissez encore des morceaux de la source : le terrain intermédiaire entre un grain isolé et un nuage lisse.", reviewed: false },
    },
    'label.captionSmoothCloud': {
        en: { t: "Smooth Cloud — many overlapping Hann grains fuse into one continuous, glassy texture. Overlap-add at work: size × density well above 1." },
        fr: { t: "Nuage lisse — de nombreux grains Hann superposés fusionnent en une texture continue et vitreuse. La sommation par recouvrement à l'œuvre : taille × densité bien au-dessus de 1.", reviewed: false },
    },
    'label.captionFrozenPad': {
        en: { t: "Frozen Pad — Freeze pins the read head; Pitch Spray shimmers the frozen instant into a sustained, evolving pad that never moves through the source." },
        fr: { t: "Nappe gelée — Freeze fige la tête de lecture ; la dispersion de hauteur fait scintiller l'instant gelé en une nappe tenue et évolutive qui n'avance jamais dans la source.", reviewed: false },
    },
    'label.captionAsyncCloud': {
        en: { t: "Asynchronous Cloud — high Scatter randomises grain timing. The pitched comb dissolves and the spectrum smears into broadband noise: the async end of the axis." },
        fr: { t: "Nuage asynchrone — un fort étalement rend aléatoire le déclenchement des grains. Le peigne harmonique se dissout et le spectre s'étale en bruit large bande : l'extrémité asynchrone de l'axe.", reviewed: false },
    },
    'label.captionGranularFire': {
        en: { t: "Granular Fire — the worked example on the crackling-fire recording. A lively grain/spray set turns a field recording into a moving granular bed." },
        fr: { t: "Feu granulaire — l'exemple travaillé sur l'enregistrement de feu crépitant. Un réglage vif de grain et de dispersion transforme un enregistrement de terrain en un lit granulaire mouvant.", reviewed: false },
    },
    'label.captionRectClick': {
        en: { t: "Rect Click — the rough end of the lesson: a rectangular window is flat with only a 1\u00a0ms guard at each edge, so every grain starts and stops abruptly. Sparse grains let each hard edge stand alone. Compare with Hann to hear why windows matter." },
        fr: { t: "Clic rectangulaire — l'extrémité rugueuse de la leçon : une fenêtre rectangulaire est plate avec seulement 1\u00a0ms de garde à chaque bord, donc chaque grain démarre et s'arrête brutalement. Des grains clairsemés laissent chaque bord dur isolé. Comparez avec Hann pour entendre pourquoi les fenêtres comptent.", reviewed: false },
    },
});

// ============================================================================
// I18N_EXEMPT — reasoned exclusions, never silence
//
// Every visible string the coverage scan finds must be a [data-i18n] element, a
// setLabel() call, or an entry HERE WITH A REASON. A bare skip list would let a
// missed label hide as a deliberate one.
//
// The strings are matched against the extractor's NORMALISED text, which
// collapses runs of whitespace — so the wordmark's hair spaces come back as
// plain spaces and the entry below is written that way. In the markup itself
// the character is still U+200A.
// ============================================================================

export const I18N_EXEMPT = [
    // The h1 splits the product name across two text nodes so the second half
    // can carry the green italic .title-accent. Both halves are the same
    // untranslatable name; keying either would translate half a wordmark.
    ['O – simple',
     'the product name, first half of the split wordmark in the page heading — a product name is never translated'],

    // CAUTION, and the reason this entry carries a longer note than its
    // neighbours: the second half of the wordmark is the bare word "Grain",
    // which is ALSO the Grain group heading and the "Grain Pitch" caption's
    // first word. Exemption matches on TEXT with no selector, so this entry
    // silently covers the group heading too. That heading carries its own
    // data-i18n key (label.groupGrain) and is translated; it is not relying on
    // this entry. Anyone adding a third bare "Grain" to this page must key it
    // deliberately — the coverage assertion will not catch a miss.
    ['Grain',
     'the product name, second half of the split wordmark (.title-accent) — a product name is never translated. Collides by text with the Grain group heading, which carries label.groupGrain of its own'],

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
// Through v1.2.1 the anchors carried the tip KEY in their own data-tip
// attribute and js/app.js looked the copy up in a TIPS object. That cannot
// survive canon v2 — applyI18n WRITES data-tip as the tip BODY, so the key and
// the copy would fight over one attribute, and check-i18n assertion 3 requires
// index.html to carry zero data-tip literals. The fifteen knob cells and the
// two select cells are addressed by data-param, which MOVED up from the inner
// .knob div (nothing read it there — no JS, no CSS, no gate); the two toggles
// gained one; the four visualization cells and the readout strip gained an id;
// the eight lesson buttons are addressed by the data-preset they already
// carried, which is the C++ preset name and is never localized.
//
// The selector on a knob cell must find the CELL, not the .knob inside it —
// the cell is the hover target and the box the tip is measured against. Only
// the cell carries data-param now, so there is no ambiguity to resolve by
// document order.
// ============================================================================

export const TIP_BINDINGS = [
    ['#gear-btn',                                    'gear-btn'],
    ['#lang-select',                                 'lang-select'],
    ['#help-toggle',                                 'help-toggle'],

    ['#vizCloudCell',                                'vizCloud'],
    ['#vizWaveCell',                                 'vizWave'],
    ['#vizScopeCell',                                'vizScope'],
    ['#vizSpectrumCell',                             'vizSpectrum'],
    ['#grainReadout',                                'readout'],

    ['[data-param="sourceSample"]',                  'sourceSample'],
    ['#btnLoad',                                     'loadSource'],
    ['#source-drop-zone',                            'dropZone'],

    ['[data-param="grainSize"]',                     'grainSize'],
    ['[data-param="density"]',                       'density'],
    ['[data-param="position"]',                      'position'],
    ['[data-param="scan"]',                          'scan'],
    ['[data-param="freeze"]',                        'freeze'],

    ['[data-param="windowShape"]',                   'windowShape'],
    ['[data-param="windowTaper"]',                   'windowTaper'],

    ['[data-param="pitchSpray"]',                    'pitchSpray'],
    ['[data-param="positionSpray"]',                 'positionSpray'],
    ['[data-param="scatter"]',                       'scatter'],
    ['[data-param="grainPitch"]',                    'grainPitch'],
    ['[data-param="panSpray"]',                      'panSpray'],
    ['[data-param="velToDensity"]',                  'velToDensity'],

    ['[data-param="adsrEnabled"]',                   'adsrEnabled'],
    ['[data-param="ampAttack"]',                     'ampAttack'],
    ['[data-param="ampDecay"]',                      'ampDecay'],
    ['[data-param="ampSustain"]',                    'ampSustain'],
    ['[data-param="ampRelease"]',                    'ampRelease'],

    ['[data-param="outputLevel"]',                   'outputLevel'],

    ['.tour-btn[data-preset="Single Grain"]',        'lessonSingleGrain'],
    ['.tour-btn[data-preset="Pitched Buzz"]',        'lessonPitchedBuzz'],
    ['.tour-btn[data-preset="Fragments"]',           'lessonFragments'],
    ['.tour-btn[data-preset="Smooth Cloud"]',        'lessonSmoothCloud'],
    ['.tour-btn[data-preset="Frozen Pad"]',          'lessonFrozenPad'],
    ['.tour-btn[data-preset="Asynchronous Cloud"]',  'lessonAsyncCloud'],
    ['.tour-btn[data-preset="Granular Fire"]',       'lessonGranularFire'],
    ['.tour-btn[data-preset="Rect Click"]',          'lessonRectClick'],
];

export function tr(key, lang, vars) {
    const entry = I18N[key];
    if (!entry) { console.warn(`i18n: missing key ${key}`); return { t: key, b: '' }; }
    const s = entry[lang] || entry.en;

    // A var VALUE that is itself an I18N key resolves to that key's localized
    // title; anything else is used literally. This plugin's composed strings
    // pass a dropped file's NAME and a browser error message, neither of which
    // is a key, so only the literal arm runs here today. The resolving arm is
    // what lets a plugin compose a localized name into a tip without pinning
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
