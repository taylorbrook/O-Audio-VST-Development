/*
   This file is part of O-simpleSampler, an Ouaricon Audio plugin.
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
// i18n.js — O-simpleSampler interface copy, English + French (v1.4.0)
//
// UI ROOT IS Source/ui/public. There is no second UI root in this plugin and no
// Resources/ui staging directory. This file is a SOURCES entry in
// juce_add_binary_data(O-simpleSampler_UIResources), whose NAMESPACE is
// UIBinaryData — NOT BinaryData, which this plugin's second binary-data target
// already owns for the embedded piano.wav. The editor serves it as
// UIBinaryData::i18n_js.
//
// An ES module that EXPORTS ONLY. It must never self-execute: a bare top-level
// statement here throws out of module evaluation and takes every later
// initializer on the page with it (pattern_module_toplevel_init_tdz).
// scripts/check-i18n.js assertion 7 enforces it.
//
// FILENAME: no hyphen. juce_add_binary_data STRIPS hyphens rather than
// converting them to underscores, so a second file named i18n-fr.js would have
// to be reached as the symbol i18nfr_js (critical_binary_data_strips_hyphens).
// One combined file for both languages sidesteps the question. The symbol this
// file becomes is UIBinaryData::i18n_js, which collides with nothing already
// served by this editor.
//
// COPY IS textContent ON EVERY PATH — never innerHTML. check-i18n assertion 9
// rejects any innerHTML reference here and any string literal containing "<".
//
// THE ENGLISH WAS MOVED, NOT REWRITTEN — with ONE mechanical transformation,
// applied uniformly and recorded in the CHANGELOG. Every `en` string below was
// extracted from v1.3.1's `TIPS` / `PRESET_LESSONS` / `PITCH_MODE_TEXT` tables
// in js/app.js and from index.html, then compared back byte-for-byte, rather
// than re-typed.
//
// The transformation: v1.3.1's tip BODIES carried markup — `<em>`, `<strong>`,
// `<code>` — because the renderer dropped them into innerHTML. Canon v2 renders
// a tip with createElement + textContent, and assertion 9 forbids "<" in a
// string literal here, so the tags are gone and the WORDS are unchanged. This
// is not a rewrite and not a loss of a shipped surface: v1.3.1 ALSO installed a
// plain-text `title=` fallback on every anchor, built by stripping exactly these
// tags, so the tag-free form below is a string this plugin has already been
// shipping since v1.3.0.
//
// HTML entities are decoded to the characters they named (&#183; -> ·,
// &rarr; -> →, &plusmn; -> ±, &minus; -> −, &ndash; -> –) because setAttribute
// and textContent do not decode entities. A non-breaking space (&nbsp;) and a
// hair space (&#8202;) are written as \u00a0 and \u200a ESCAPES, never as the
// characters themselves and never as a plain space: the extractor's inventory
// normalises both to a plain space, and copying that normalised form in would
// silently widen the string in BOTH languages, where the en-vs-fr geometry diff
// cannot see it.
//
// KEYS ARE THE PARAMETER ID where the anchor is a parameter cell, and a
// `label.*` / `aria.*` / `toast.*` slug otherwise. Through v1.3.1 each anchor
// carried the tip KEY in its own data-tip attribute and js/app.js looked the
// copy up in a TIPS object. That cannot survive canon v2 — applyI18n WRITES
// data-tip as the tip BODY, and check-i18n assertion 3 requires index.html to
// carry zero data-tip literals. The seventeen knob cells and the two select
// cells gained a data-param attribute naming the APVTS parameter they drive;
// the four viz cells and the drop zone are addressed by an id; the seven lesson
// buttons by the data-preset they already carried.
//
// LABELS NEVER REUSE A TOOLTIP KEY. trLabel() falls back to I18N and some of
// this page's captions do happen to equal their tip title today ("Start",
// "Attack") — but many do not ("Amount" vs "Vintage", "Cutoff" vs "Filter
// Cutoff", "Level" vs "Output Level"), and a rule that holds for half the
// controls is a rule nobody can apply. A caption and a tip title also diverge
// the moment either is edited, which would make a tooltip copy edit a silent
// geometry change to a control. The ONE place the fallback is used on purpose
// is `data-i18n-aria` on the knobs: an accessible name IS the control's name,
// so it reads the tooltip title by design.
//
// ALL FRENCH IS MACHINE-DRAFTED AND FLAGGED `reviewed: false`. No native
// speaker has read it. `node scripts/check-i18n.js` prints the worklist.
// ============================================================================

export const LANGUAGES = ['en', 'fr'];

export const I18N = Object.freeze({

    // ── The settings popover (v1.4.0) ───────────────────────────────────────
    // The gear is new; the hover-help toggle inside it is not. Through v1.3.1
    // the toggle was a "?" chip standing on its own in the header. It moved
    // rather than being duplicated, so the two settings that decide what the
    // hover help SAYS and WHETHER it says it live in one place.
    'gear-btn': {
        en: { t: 'Settings',
              b: 'Choose the language of the interface and whether the hover help appears. Both choices are remembered with the session.' },
        fr: { t: 'Réglages',
              b: "Choisir la langue de l'interface et l'affichage de l'aide au survol. Les deux choix sont conservés avec la session.",
              reviewed: false },
    },

    // Written to say what is TRUE of canon v2, in both languages: the labels DO
    // change, and the halves that stay English are named rather than left to be
    // discovered — value readouts (D-03) and the Loop Mode / Pitch Mode menu
    // entries, which come from the C++ AudioParameterChoice and are the host
    // automation contract (D-01).
    'lang-select': {
        en: { t: 'Language',
              b: 'The language of the labels on this page and of this hover help. English and French are available; value readouts and the two drop-down menus stay in English.' },
        fr: { t: 'Langue',
              b: "La langue des libellés de cette page et de cette aide au survol. L'anglais et le français sont disponibles ; les valeurs affichées et les deux menus déroulants restent en anglais.",
              reviewed: false },
    },

    'help-toggle': {
        en: { t: 'Hover help',
              b: 'Switches these explanations off or back on. The choice is saved with the session, so a project you come back to opens the way you left it.' },
        fr: { t: 'Aide au survol',
              b: "Active ou désactive ces explications. Le choix est enregistré avec la session : un projet rouvert se présente comme vous l'avez laissé.",
              reviewed: false },
    },

    // ── Source ──────────────────────────────────────────────────────────────
    loadSource: {
        en: { t: 'Load your own',
              b: "The plugin starts on its built-in recording — everything else on this panel shapes that sound. Press this to open a file picker and sample any .wav\u00a0/\u00a0.aif\u00a0/\u00a0.flac instead. Anything longer than 30\u00a0s is trimmed (you'll see a notice). The same controls then play your sound." },
        fr: { t: 'Charger votre son',
              b: "Le plugin démarre sur son enregistrement intégré — tout le reste de ce panneau façonne ce son. Appuyez ici pour ouvrir un sélecteur de fichiers et échantillonner à la place n'importe quel .wav\u00a0/\u00a0.aif\u00a0/\u00a0.flac. Au-delà de 30\u00a0s le son est rogné (un avis s'affiche). Les mêmes commandes jouent ensuite votre son.",
              reviewed: false },
    },
    dropZone: {
        en: { t: 'Drop a sound here',
              b: 'Drag an audio file straight from your desktop onto this panel to sample it. Try a spoken word, a drum hit, or a field recording — a sampler can play any sound across the keyboard. This and the Load… button are the two ways to change the source.' },
        fr: { t: 'Déposez un son ici',
              b: "Faites glisser un fichier audio depuis votre bureau sur ce panneau pour l'échantillonner. Essayez un mot parlé, un coup de batterie ou un enregistrement de terrain — un échantillonneur peut jouer n'importe quel son sur tout le clavier. Ce dépôt et le bouton Charger… sont les deux façons de changer la source.",
              reviewed: false },
    },

    // ── Region ──────────────────────────────────────────────────────────────
    start: {
        en: { t: 'Start',
              b: 'Where playback begins in the source. Pull it in to skip silence or a soft front edge so a key press lands right on the sound. Drag the gold marker on the waveform too.' },
        fr: { t: 'Début',
              b: "L'endroit où la lecture commence dans la source. Resserrez-le pour sauter un silence ou une attaque molle, afin qu'un appui de touche tombe pile sur le son. Le repère doré sur la forme d'onde se déplace aussi à la souris.",
              reviewed: false },
    },
    end: {
        en: { t: 'End',
              b: 'Where playback stops. Pull it in to drop a noisy tail or dead air at the end. Start and End together isolate just the useful part of the recording.' },
        fr: { t: 'Fin',
              b: "L'endroit où la lecture s'arrête. Resserrez-le pour écarter une queue bruitée ou du silence en fin de fichier. Début et Fin isolent ensemble la seule partie utile de l'enregistrement.",
              reviewed: false },
    },
    loopStart: {
        en: { t: 'Loop Start',
              b: 'The front edge of the repeating section, measured inside the trimmed region. Only matters when Loop\u00a0Mode is on — it sets where each repeat begins.' },
        fr: { t: 'Début de boucle',
              b: "Le bord avant de la section répétée, mesuré à l'intérieur de la région rognée. N'agit que si le Mode\u00a0boucle est actif — il fixe où chaque répétition commence.",
              reviewed: false },
    },
    loopEnd: {
        en: { t: 'Loop End',
              b: 'The back edge of the repeating section. While you hold a key the sound cycles Loop\u00a0Start\u00a0→\u00a0Loop\u00a0End forever, so a short sample can sustain indefinitely.' },
        fr: { t: 'Fin de boucle',
              b: 'Le bord arrière de la section répétée. Tant que la touche est tenue, le son parcourt Début\u00a0de\u00a0boucle\u00a0→\u00a0Fin\u00a0de\u00a0boucle sans fin : un court échantillon tient donc indéfiniment.',
              reviewed: false },
    },
    loopCrossfade: {
        en: { t: 'Loop Crossfade',
              b: "Blends the loop's end back into its start so the seam doesn't click. 0\u00a0ms is a hard splice; longer fades smooth a rough loop into a seamless sustain." },
        fr: { t: 'Fondu de boucle',
              b: 'Mélange la fin de la boucle dans son début pour que la jointure ne claque pas. À 0\u00a0ms la coupe est franche ; un fondu plus long lisse une boucle grossière en une tenue sans couture.',
              reviewed: false },
    },
    // Off / Forward / Ping-Pong are the AudioParameterChoice entries and stay
    // English under D-01 — they are the host automation contract. Naming them
    // inside the French body is deliberate: the sentence has to describe the
    // menu the reader actually sees.
    loopMode: {
        en: { t: 'Loop Mode',
              b: 'Off plays once and stops. Forward repeats the loop start→end. Ping-Pong runs it forward then backward — smoother for held pads and textures.' },
        fr: { t: 'Mode boucle',
              b: "Off lit une fois puis s'arrête. Forward répète la boucle du début vers la fin. Ping-Pong la parcourt en avant puis en arrière — plus doux pour les nappes tenues et les textures.",
              reviewed: false },
    },
    reverse: {
        en: { t: 'Reverse',
              b: "Plays the sample backwards. Pair it with a slow attack for a rising swell, or use it for whooshes and that distinctive 'sucking-in' tail." },
        fr: { t: 'Inverse',
              b: "Joue l'échantillon à l'envers. Associez-le à une attaque lente pour une montée en crescendo, ou utilisez-le pour des souffles et cette queue « aspirée » caractéristique.",
              reviewed: false },
    },

    // ── Pitch ───────────────────────────────────────────────────────────────
    rootKey: {
        en: { t: 'Root Key',
              b: 'The key where the sample plays at its original recorded pitch. Notes above it play higher, notes below play lower — this is what turns one recording into a whole instrument.' },
        fr: { t: 'Note de référence',
              b: "La touche où l'échantillon joue à la hauteur d'origine. Les notes au-dessus montent, celles en dessous descendent — c'est ce qui transforme un enregistrement en instrument complet.",
              reviewed: false },
    },
    pitchMode: {
        en: { t: 'Pitch Mode',
              b: 'The headline A/B. Repitch changes speed to change pitch (like speeding up a record — higher is faster). Stretch holds the timing and moves pitch on its own.' },
        fr: { t: 'Mode de hauteur',
              b: 'Le A/B central. Repitch change la vitesse pour changer la hauteur (comme un disque accéléré — plus aigu, plus rapide). Stretch conserve la durée et déplace la hauteur seule.',
              reviewed: false },
    },
    tune: {
        en: { t: 'Tune',
              b: 'Coarse pitch in whole semitones, ±24. Use it to drop the whole sample into the key of your song without re-loading anything.' },
        fr: { t: 'Accord',
              b: "Hauteur grossière en demi-tons entiers, ±24. Sert à ramener tout l'échantillon dans la tonalité du morceau sans rien recharger.",
              reviewed: false },
    },
    fine: {
        en: { t: 'Fine',
              b: 'Tiny pitch trim in cents (1/100 of a semitone). Tune a sample exactly in, or detune it a hair to thicken a layered sound.' },
        fr: { t: 'Affinage',
              b: "Retouche fine de hauteur en cents (1/100 de demi-ton). Accordez un échantillon au plus juste, ou désaccordez-le d'un cheveu pour épaissir une superposition.",
              reviewed: false },
    },

    // ── Vintage ─────────────────────────────────────────────────────────────
    vintage: {
        en: { t: 'Vintage',
              b: "Old-sampler grit: lowers the sample rate and bit depth to throw away resolution. At\u00a00 it's clean; turn it up for crunchy, lo-fi SP-1200 character." },
        fr: { t: 'Vintage',
              b: "Le grain des vieux échantillonneurs : abaisse la fréquence d'échantillonnage et la résolution binaire pour jeter de la définition. À\u00a00 c'est propre ; montez pour un caractère croustillant, lo-fi, façon SP-1200.",
              reviewed: false },
    },

    // ── Filter ──────────────────────────────────────────────────────────────
    filterCutoff: {
        en: { t: 'Filter Cutoff',
              b: 'The brightness control. Wide open lets everything through; lower it to roll off the highs and darken the sound. The curve above shows exactly what gets through.' },
        fr: { t: 'Coupure du filtre',
              b: 'La commande de brillance. Grande ouverte, tout passe ; abaissez-la pour atténuer les aigus et assombrir le son. La courbe au-dessus montre exactement ce qui passe.',
              reviewed: false },
    },
    filterResonance: {
        en: { t: 'Filter Resonance',
              b: 'Boosts the frequencies right at the cutoff, adding a vocal, whistling peak. Push it for a sharper, more synth-like sweep as you move the cutoff.' },
        fr: { t: 'Résonance du filtre',
              b: 'Accentue les fréquences juste à la coupure et ajoute un pic vocal, sifflant. Poussez-la pour un balayage plus acéré, plus synthétique, quand vous bougez la coupure.',
              reviewed: false },
    },

    // ── Amplitude envelope ──────────────────────────────────────────────────
    ampAttack: {
        en: { t: 'Attack',
              b: 'How long the note takes to fade in after a key press. Short\u00a0= a sharp hit; long\u00a0= a slow swell that eases in.' },
        fr: { t: 'Attaque',
              b: "Le temps que met la note à monter après l'appui sur la touche. Court\u00a0= une frappe nette ; long\u00a0= une montée lente qui s'installe.",
              reviewed: false },
    },
    ampDecay: {
        en: { t: 'Decay',
              b: "After the attack peak, how fast the level falls to the sustain level. This shapes the initial 'thump' before the held part of the note." },
        fr: { t: 'Déclin',
              b: "Après le pic d'attaque, la vitesse à laquelle le niveau retombe vers le maintien. C'est ce qui façonne le « coup » initial avant la partie tenue de la note.",
              reviewed: false },
    },
    ampSustain: {
        en: { t: 'Sustain',
              b: 'The level the note holds at while you keep the key down. 100% holds full volume; lower it so the sound settles back after its attack.' },
        fr: { t: 'Maintien',
              b: 'Le niveau auquel la note se tient tant que la touche est enfoncée. 100\u00a0% garde le volume plein ; baissez-le pour que le son retombe après son attaque.',
              reviewed: false },
    },
    ampRelease: {
        en: { t: 'Release',
              b: 'How long the note takes to fade out after you let go. Short\u00a0= an abrupt stop; long\u00a0= a lingering tail that rings on.' },
        fr: { t: 'Relâchement',
              b: 'Le temps que met la note à disparaître après le relâchement. Court\u00a0= un arrêt abrupt ; long\u00a0= une queue qui traîne et continue de sonner.',
              reviewed: false },
    },
    velToAmp: {
        en: { t: 'Velocity → Amp',
              b: 'How much your playing strength (velocity) changes loudness. At\u00a00 every note is equal; higher makes soft and hard playing far more expressive.' },
        fr: { t: 'Vélocité → Ampli',
              b: 'À quel point la force de jeu (vélocité) modifie le volume. À\u00a00 toutes les notes sont égales ; plus haut, jouer doux ou fort devient bien plus expressif.',
              reviewed: false },
    },

    // ── Output ──────────────────────────────────────────────────────────────
    outputLevel: {
        en: { t: 'Output Level',
              b: 'The master volume of the plugin, in decibels. Use it to balance against your other tracks; the bottom of the range (−inf) is silent.' },
        fr: { t: 'Niveau de sortie',
              b: "Le volume général du plugin, en décibels. Sert à l'équilibrer avec vos autres pistes ; le bas de la plage (−inf) est silencieux.",
              reviewed: false },
    },

    // ── Viz cells ───────────────────────────────────────────────────────────
    vizWaveform: {
        en: { t: 'Waveform Editor',
              b: 'A picture of the loaded sound over time. Drag the gold and red edges to trim the region, the green handles to set the loop, and watch the white playhead track where the sample is being read.' },
        fr: { t: "Éditeur de forme d'onde",
              b: "Une image du son chargé au fil du temps. Faites glisser les bords doré et rouge pour rogner la région, les poignées vertes pour régler la boucle, et regardez la tête de lecture blanche suivre l'endroit lu.",
              reviewed: false },
    },
    vizFilter: {
        en: { t: 'Filter Response',
              b: "The filter's actual frequency shape. It shows what passes through — falling away past the cutoff, with a peak when resonance is up. This curve is what you hear." },
        fr: { t: 'Réponse du filtre',
              b: 'La forme fréquentielle réelle du filtre. Elle montre ce qui passe — en chute au-delà de la coupure, avec un pic quand la résonance monte. Cette courbe est ce que vous entendez.',
              reviewed: false },
    },
    vizAmp: {
        en: { t: 'Envelope Display',
              b: 'The Attack–Decay–Sustain–Release volume shape drawn from the four knobs. The moving dot shows where a held note sits on that curve right now.' },
        fr: { t: "Affichage d'enveloppe",
              b: "La forme de volume Attaque–Déclin–Maintien–Relâchement tracée d'après les quatre boutons. Le point mobile montre où une note tenue se situe sur cette courbe.",
              reviewed: false },
    },
    vizScope: {
        en: { t: 'Output Scope',
              b: 'A live oscilloscope of the sound leaving the plugin. Watch the waveform react as you play and as you turn the filter, vintage, and envelope controls.' },
        fr: { t: 'Oscilloscope de sortie',
              b: "Un oscilloscope en direct du son qui sort du plugin. Regardez la forme d'onde réagir quand vous jouez et quand vous tournez le filtre, le vintage et l'enveloppe.",
              reviewed: false },
    },

    // ── Concept presets ─────────────────────────────────────────────────────
    // Through v1.3.1 the tip and the caption were two separate tables that
    // happened to say nearly the same thing. They still are, and both halves
    // are authored: a derived string is invisible to the translator reviewing
    // this file, and the caption is the one string on this page that is chosen
    // by a click.
    lessonRawOneShot: {
        en: { t: 'Raw One-Shot',
              b: 'Press a key, hear the whole sample once, no loop. The simplest thing a sampler does and the place to start.' },
        fr: { t: 'One-shot brut',
              b: "Appuyez sur une touche, entendez tout l'échantillon une fois, sans boucle. La chose la plus simple qu'un échantillonneur fasse, et le point de départ.",
              reviewed: false },
    },
    lessonTunedKeyboard: {
        en: { t: 'Tuned Across the Keyboard',
              b: 'How one recording becomes a playable instrument: set the Root\u00a0Key and every key plays the sample at its own pitch.' },
        fr: { t: 'Accordé sur tout le clavier',
              b: "Comment un enregistrement devient un instrument jouable : réglez la Note\u00a0de\u00a0référence et chaque touche joue l'échantillon à sa propre hauteur.",
              reviewed: false },
    },
    lessonLoopedPad: {
        en: { t: 'Looped Pad',
              b: 'A loop with a crossfade turns a short sound into an endless one you can hold — no click at the seam.' },
        fr: { t: 'Nappe bouclée',
              b: "Une boucle avec fondu transforme un son court en un son sans fin que l'on peut tenir — sans clic à la jointure.",
              reviewed: false },
    },
    lessonReversedSwell: {
        en: { t: 'Reversed Swell',
              b: 'Reverse plus a slow attack makes a backwards swell that rises into the downbeat — a classic intro and transition effect.' },
        fr: { t: 'Montée inversée',
              b: "Inverse plus une attaque lente donnent une montée à l'envers qui débouche sur le premier temps — un effet d'intro et de transition classique.",
              reviewed: false },
    },
    lessonRepitchStretch: {
        en: { t: 'Repitch vs Stretch',
              b: 'The headline A/B. Play the same note in each mode: Repitch changes speed with pitch (tape-style); Stretch keeps the timing and moves pitch on its own.' },
        fr: { t: 'Repitch contre Stretch',
              b: 'Le A/B central. Jouez la même note dans chaque mode : Repitch change la vitesse avec la hauteur (façon bande) ; Stretch conserve la durée et déplace la hauteur seule.',
              reviewed: false },
    },
    lessonSp1200: {
        en: { t: 'SP-1200 Crunch',
              b: "Lean on Vintage to hear how an old sampler's low sample rate and bit depth add the gritty, lo-fi character beloved in hip-hop." },
        fr: { t: 'Croustillant SP-1200',
              b: "Appuyez sur le Vintage pour entendre comment la basse fréquence d'échantillonnage et la faible résolution d'un vieil appareil ajoutent ce grain lo-fi cher au hip-hop.",
              reviewed: false },
    },
    lessonFilteredEnv: {
        en: { t: 'Filtered & Enveloped',
              b: 'The low-pass filter and the amp envelope together — the two main shaping tools — sculpt a raw sample into a finished, musical note.' },
        fr: { t: 'Filtré et mis en enveloppe',
              b: "Le filtre passe-bas et l'enveloppe d'amplitude ensemble — les deux principaux outils de façonnage — sculptent un échantillon brut en une note musicale finie.",
              reviewed: false },
    },
});

// ============================================================================
// LABELS — the copy ON the page, as opposed to the copy ABOUT it.
//
// A LABELS entry is { en: { t }, fr: { t, reviewed } }: one string, no body,
// because a label is not a tooltip. trLabel() reads LABELS first and falls back
// to I18N, so a `data-i18n-aria` naming a parameter id resolves to that
// control's tooltip TITLE — which is exactly what an accessible name should be.
//
// The `toast.*` and `label.source*` entries are written by setLabel() at
// runtime rather than authored in the markup: a toast has no resting text and a
// source-status line has none until something is loaded. They carry {name} /
// {error} tokens, substituted by trLabel — a filename and an error string, both
// of which are used literally and neither of which is copy.
//
// ALL FRENCH IS MACHINE-DRAFTED, `reviewed: false`.
// ============================================================================

export const LABELS = Object.freeze({

    // ── Header ──────────────────────────────────────────────────────────────
    // The product name itself is NOT here — see I18N_EXEMPT. Only the strapline
    // under it is copy.
    'label.subtitle': {
        en: { t: 'Keyboard Sampler · A Field Guide' },
        fr: { t: 'Échantillonneur de clavier · un guide de terrain', reviewed: false },
    },

    // ── Settings popover ────────────────────────────────────────────────────
    // The two row captions read their key out of I18N ('lang-select',
    // 'help-toggle') because each row's caption IS its tooltip title. The
    // faces and the accessible names are label-only and live here.
    'aria.settings':   { en: { t: 'Settings' },            fr: { t: 'Réglages', reviewed: false } },
    'aria.langSelect': { en: { t: 'Interface language' },  fr: { t: "Langue de l'interface", reviewed: false } },
    'aria.helpToggle': { en: { t: 'Toggle hover help' },   fr: { t: "Activer ou désactiver l'aide au survol", reviewed: false } },
    // "Oui"/"Non" rather than "Activé"/"Désactivé": the same pair O-Orbit's
    // help toggle already ships, and a two-face button in a 214px panel is the
    // one place in this suite where the shorter French reads better AND fits.
    'ui.on':           { en: { t: 'On' },                  fr: { t: 'Oui', reviewed: false } },
    'ui.off':          { en: { t: 'Off' },                 fr: { t: 'Non', reviewed: false } },

    // ── Concept-preset tour ─────────────────────────────────────────────────
    // The BUTTON faces are abbreviations of the C++ preset names, which is why
    // they are not the lesson titles in I18N. "Repitch/Stretch" and "SP-1200"
    // are the two that carry no translatable words at all.
    'aria.presetTour':            { en: { t: 'Concept presets' },  fr: { t: 'Préréglages pédagogiques', reviewed: false } },
    'label.presetRawOneShot':     { en: { t: 'Raw One-Shot' },     fr: { t: 'One-shot brut', reviewed: false } },
    'label.presetTuned':          { en: { t: 'Tuned' },            fr: { t: 'Accordé', reviewed: false } },
    'label.presetLoopedPad':      { en: { t: 'Looped Pad' },       fr: { t: 'Nappe', reviewed: false } },
    'label.presetReversed':       { en: { t: 'Reversed' },         fr: { t: 'Inversé', reviewed: false } },
    'label.presetRepitchStretch': { en: { t: 'Repitch/Stretch' },
                                    fr: { t: 'Repitch/Stretch', sameAsEn: true, reviewed: false } },
    'label.presetSp1200':         { en: { t: 'SP-1200' },
                                    fr: { t: 'SP-1200', sameAsEn: true, reviewed: false } },
    'label.presetFiltered':       { en: { t: 'Filtered' },         fr: { t: 'Filtré', reviewed: false } },

    // The resting caption plus one per lesson. applyPreset() writes them with
    // setLabel() through a dispatch of one-line writers, each naming a literal
    // key — check-i18n assertion 13 rejects `setLabel(el, MAP[name] || …)`
    // twice over, as a computed key AND as a conditional.
    'label.tourCaption': {
        en: { t: 'Hover any control for an explanation · pick a concept preset to hear it isolated.' },
        fr: { t: "Survolez n'importe quelle commande pour une explication · choisissez un préréglage pour l'entendre isolé.",
              reviewed: false },
    },
    'label.captionRawOneShot': {
        en: { t: 'Raw One-Shot — press a key and hear the whole sample once, no loop. The simplest thing a sampler does, and the place to start.' },
        fr: { t: "One-shot brut — appuyez sur une touche et entendez tout l'échantillon une fois, sans boucle. La chose la plus simple qu'un échantillonneur fasse, et le point de départ.",
              reviewed: false },
    },
    'label.captionTuned': {
        en: { t: 'Tuned Across the Keyboard — one recording becomes a playable instrument. Set the Root Key and every key plays the sample at its own pitch.' },
        fr: { t: "Accordé sur tout le clavier — un enregistrement devient un instrument jouable. Réglez la Note de référence et chaque touche joue l'échantillon à sa propre hauteur.",
              reviewed: false },
    },
    'label.captionLoopedPad': {
        en: { t: 'Looped Pad — a loop with a crossfade turns a short sound into an endless one you can hold, with no click at the seam.' },
        fr: { t: "Nappe bouclée — une boucle avec fondu transforme un son court en un son sans fin que l'on peut tenir, sans clic à la jointure.",
              reviewed: false },
    },
    'label.captionReversed': {
        en: { t: 'Reversed Swell — Reverse plus a slow attack makes a backwards swell that rises into the downbeat. A classic intro / transition.' },
        fr: { t: "Montée inversée — Inverse plus une attaque lente donnent une montée à l'envers qui débouche sur le premier temps. Une intro / transition classique.",
              reviewed: false },
    },
    'label.captionRepitchStretch': {
        en: { t: 'Repitch vs Stretch — the headline A/B. Repitch changes speed with pitch (tape-style); Stretch keeps the timing and moves pitch independently.' },
        fr: { t: 'Repitch contre Stretch — le A/B central. Repitch change la vitesse avec la hauteur (façon bande) ; Stretch conserve la durée et déplace la hauteur indépendamment.',
              reviewed: false },
    },
    'label.captionSp1200': {
        en: { t: 'SP-1200 Crunch — Vintage drops the sample rate and bit depth to add the gritty, lo-fi character of classic hip-hop samplers.' },
        fr: { t: "Croustillant SP-1200 — le Vintage abaisse la fréquence d'échantillonnage et la résolution pour ajouter le grain lo-fi des échantillonneurs hip-hop classiques.",
              reviewed: false },
    },
    'label.captionFiltered': {
        en: { t: 'Filtered & Enveloped — the low-pass filter and the amp envelope together sculpt a raw sample into a finished, musical note.' },
        fr: { t: "Filtré et mis en enveloppe — le filtre passe-bas et l'enveloppe d'amplitude sculptent ensemble un échantillon brut en une note musicale finie.",
              reviewed: false },
    },

    // ── Waveform editor ─────────────────────────────────────────────────────
    // "Waveform Editor ·" keeps its trailing fleuron: the separator belongs to
    // the caption, not to the hint span beside it, and moving it would change
    // where the two boxes meet.
    'label.waveformEditor': {
        en: { t: 'Waveform Editor ·' },
        fr: { t: "Éditeur de forme d'onde ·", reviewed: false },
    },
    'label.waveformHint': {
        en: { t: 'drag start/end & loop handles; the playhead tracks the live read position' },
        fr: { t: 'faites glisser les poignées de début/fin et de boucle ; la tête de lecture suit la position lue en direct',
              reviewed: false },
    },

    // ── Rack group titles ───────────────────────────────────────────────────
    'label.groupSource':  { en: { t: 'Source' },  fr: { t: 'Source', sameAsEn: true, reviewed: false } },
    'label.groupRegion':  { en: { t: 'Region' },  fr: { t: 'Région', reviewed: false } },
    'label.groupPitch':   { en: { t: 'Pitch' },   fr: { t: 'Hauteur', reviewed: false } },
    'label.groupVintage': { en: { t: 'Vintage' }, fr: { t: 'Vintage', sameAsEn: true, reviewed: false } },
    'label.groupFilter':  { en: { t: 'Filter' },  fr: { t: 'Filtre', reviewed: false } },
    'label.groupAmp':     { en: { t: 'Amplitude Envelope' }, fr: { t: "Enveloppe d'amplitude", reviewed: false } },
    'label.groupOutput':  { en: { t: 'Output' },  fr: { t: 'Sortie', reviewed: false } },

    // ── Source group ────────────────────────────────────────────────────────
    'label.btnLoad':  { en: { t: 'Load…' }, fr: { t: 'Charger…', reviewed: false } },
    'label.dropZone': {
        en: { t: 'Drop a .wav/.aif here to sample your own sound' },
        fr: { t: 'Déposez ici un .wav/.aif pour échantillonner votre propre son', reviewed: false },
    },
    // The status line under the drop zone. {name} is a filename or the
    // localized generic below — never a re-typed word.
    'label.sourceBuiltIn':   { en: { t: '{name} — built-in source' },
                               fr: { t: '{name} — source intégrée', reviewed: false } },
    'label.sourceLoaded':    { en: { t: '{name} loaded' },
                               fr: { t: '{name} chargé', reviewed: false } },
    'label.sourceTruncated': { en: { t: '{name} — truncated to 30 s' },
                               fr: { t: '{name} — rogné à 30 s', reviewed: false } },
    // The picker path has no filename to show — the C++ FileChooser is async and
    // the page never learns what was chosen — so it gets its own two entries
    // rather than a {name} token fed a localized generic word. Passing the word
    // as a var VALUE worked at runtime and read cleanly, but assertion 15 counts
    // a key as REFERENCED only where it is a plain literal in a setLabel call or
    // a data-i18n attribute; a key named only inside a vars object is a dead key
    // as far as the gate can tell, and it is right to say so.
    'label.sourceLoadedGeneric':    { en: { t: 'Source loaded' },
                                      fr: { t: 'Source chargée', reviewed: false } },
    'label.sourceTruncatedGeneric': { en: { t: 'Source — truncated to 30 s' },
                                      fr: { t: 'Source — rognée à 30 s', reviewed: false } },

    // ── Region knobs ────────────────────────────────────────────────────────
    // A knob caption is NOT the tooltip title: "Loop XF" against "Loop
    // Crossfade", "Amount" against "Vintage", "Level" against "Output Level".
    // The captions live in a 54px cell and the French is authored to fit two
    // lines at that width rather than to match the tip title word for word.
    'label.start':     { en: { t: 'Start' },      fr: { t: 'Début', reviewed: false } },
    'label.end':       { en: { t: 'End' },        fr: { t: 'Fin', reviewed: false } },
    'label.loopStart': { en: { t: 'Loop Start' }, fr: { t: 'Début boucle', reviewed: false } },
    'label.loopEnd':   { en: { t: 'Loop End' },   fr: { t: 'Fin boucle', reviewed: false } },
    'label.loopXf':    { en: { t: 'Loop XF' },    fr: { t: 'Fondu boucle', reviewed: false } },
    'label.loopMode':  { en: { t: 'Loop Mode' },  fr: { t: 'Mode boucle', reviewed: false } },
    'label.reverse':   { en: { t: 'Reverse' },    fr: { t: 'Inverse', reviewed: false } },
    'aria.loopMode':   { en: { t: 'Loop mode' },  fr: { t: 'Mode de boucle', reviewed: false } },

    // ── Pitch group ─────────────────────────────────────────────────────────
    'label.rootKey':   { en: { t: 'Root Key' },   fr: { t: 'Note de réf.', reviewed: false } },
    'label.tune':      { en: { t: 'Tune' },       fr: { t: 'Accord', reviewed: false } },
    'label.fine':      { en: { t: 'Fine' },       fr: { t: 'Affinage', reviewed: false } },
    'label.pitchMode': { en: { t: 'Pitch Mode' }, fr: { t: 'Mode hauteur', reviewed: false } },
    'aria.pitchMode':  { en: { t: 'Pitch mode' }, fr: { t: 'Mode de hauteur', reviewed: false } },
    // The Repitch-vs-Stretch readout beside the combo. Repitch and Stretch are
    // the AudioParameterChoice entries and stay English under D-01; only the
    // clause after the em-dash is copy.
    'label.pitchRepitch': {
        en: { t: 'Repitch — pitch & time linked' },
        fr: { t: 'Repitch — hauteur et durée liées', reviewed: false },
    },
    'label.pitchStretch': {
        en: { t: 'Stretch — time held, pitch independent' },
        fr: { t: 'Stretch — durée tenue, hauteur indépendante', reviewed: false },
    },

    // ── Vintage / Filter / Amp / Output knobs ───────────────────────────────
    'label.amount':    { en: { t: 'Amount' },     fr: { t: 'Quantité', reviewed: false } },
    'label.cutoff':    { en: { t: 'Cutoff' },     fr: { t: 'Coupure', reviewed: false } },
    'label.resonance': { en: { t: 'Resonance' },  fr: { t: 'Résonance', reviewed: false } },
    'label.attack':    { en: { t: 'Attack' },     fr: { t: 'Attaque', reviewed: false } },
    'label.decay':     { en: { t: 'Decay' },      fr: { t: 'Déclin', reviewed: false } },
    'label.sustain':   { en: { t: 'Sustain' },    fr: { t: 'Maintien', reviewed: false } },
    'label.release':   { en: { t: 'Release' },    fr: { t: 'Relâche', reviewed: false } },
    'label.velToAmp':  { en: { t: 'Vel→Amp' },    fr: { t: 'Vél→Ampli', reviewed: false } },
    'label.level':     { en: { t: 'Level' },      fr: { t: 'Niveau', reviewed: false } },

    // ── On-screen keyboard ──────────────────────────────────────────────────
    // The QWERTY row keeps its hair spaces as \u200a escapes. The letters name
    // PHYSICAL keys on the computer keyboard and are the same in both
    // languages — a French reader on an AZERTY board reads them as positions.
    'label.play':     { en: { t: 'Play ·' }, fr: { t: 'Jouer ·', reviewed: false } },
    'label.kbdHint': {
        en: { t: 'click the keys or use your computer keyboard (A\u200aS\u200aD\u200aF\u200aG\u200aH\u200aJ\u200aK · W\u200aE\u200aT\u200aY\u200aU)' },
        fr: { t: "cliquez les touches ou utilisez le clavier de l'ordinateur (A\u200aS\u200aD\u200aF\u200aG\u200aH\u200aJ\u200aK · W\u200aE\u200aT\u200aY\u200aU)",
              reviewed: false },
    },
    'aria.keyboard': { en: { t: 'On-screen keyboard' }, fr: { t: "Clavier à l'écran", reviewed: false } },

    // ── Toasts (drop / load feedback) ───────────────────────────────────────
    // Written by setLabel() onto #toast, so the element becomes a [data-i18n]
    // element and the language sweep owns it. A raw string here would be
    // stranded in the language the drop happened in.
    'toast.loading':         { en: { t: 'Loading {name}…' },
                               fr: { t: 'Chargement de {name}…', reviewed: false } },
    'toast.dropStartFailed': { en: { t: 'Drop session start failed' },
                               fr: { t: 'Échec du démarrage du dépôt', reviewed: false } },
    'toast.transferFailed':  { en: { t: 'File transfer failed' },
                               fr: { t: 'Échec du transfert du fichier', reviewed: false } },
    'toast.commitFailed':    { en: { t: 'File load failed at commit' },
                               fr: { t: 'Échec du chargement du fichier à la validation', reviewed: false } },
    'toast.dropFailed':      { en: { t: 'Drop failed: {error}' },
                               fr: { t: 'Échec du dépôt : {error}', reviewed: false } },
    'toast.dropFolder':      { en: { t: 'Drop a single audio file, not a folder' },
                               fr: { t: 'Déposez un seul fichier audio, pas un dossier', reviewed: false } },
    'toast.dropFileType':    { en: { t: 'Drop a .wav / .aif / .flac file' },
                               fr: { t: 'Déposez un fichier .wav / .aif / .flac', reviewed: false } },
    'toast.loadFailed':      { en: { t: 'Load failed' },
                               fr: { t: 'Échec du chargement', reviewed: false } },
});

// ============================================================================
// I18N_EXEMPT — reasoned exclusions, never silence.
//
// Every visible string the coverage scan finds must be a [data-i18n] element, a
// setLabel() call, or an entry HERE WITH A REASON. A bare skip list would let a
// missed label hide as a deliberate one.
// ============================================================================

export const I18N_EXEMPT = [
    // The h1 splits the product name across two text nodes so the second half
    // can carry the green italic .title-accent. Both halves are the same
    // untranslatable name; keying either would translate half a wordmark. The
    // spelling here is the extractor's NORMALISED form — it decodes the two
    // hair spaces around the en-dash to plain spaces, and assertion 10 compares
    // against what the extractor produces.
    ['O – simple',
     'the product name, first half of the split wordmark in the page heading — a product name is never translated'],
    ['Sampler',
     'the product name, second half of the split wordmark (.title-accent) — a product name is never translated'],

    // The two endonyms in the language selector.
    ['English',  'endonym — a language name is never translated'],
    ['Français', 'endonym — a language name is never translated'],

    // The Loop Mode and Pitch Mode menu entries are built at runtime from the
    // AudioParameterChoice choice strings. Those are the host automation
    // contract and stay English under D-01; translating the menu without
    // translating the automation lane would make the two disagree.
    ['Off',       'AudioParameterChoice entry (loopMode) — the host automation name, English under D-01'],
    ['Forward',   'AudioParameterChoice entry (loopMode) — the host automation name, English under D-01'],
    ['Ping-Pong', 'AudioParameterChoice entry (loopMode) — the host automation name, English under D-01'],
    ['Repitch',   'AudioParameterChoice entry (pitchMode) — the host automation name, English under D-01'],
    ['Stretch',   'AudioParameterChoice entry (pitchMode) — the host automation name, English under D-01'],

    // The seven data-preset values. They are the C++ factory-preset names that
    // applyFactoryPreset matches on, and they are also this page's tip anchors.
    ['Raw One-Shot',             'C++ factory-preset name (applyFactoryPreset) and the tip anchor — never localized'],
    ['Tuned Across the Keyboard','C++ factory-preset name (applyFactoryPreset) and the tip anchor — never localized'],
    ['Looped Pad',               'C++ factory-preset name (applyFactoryPreset) and the tip anchor — never localized'],
    ['Reversed Swell',           'C++ factory-preset name (applyFactoryPreset) and the tip anchor — never localized'],
    ['Repitch vs Stretch A/B',   'C++ factory-preset name (applyFactoryPreset) and the tip anchor — never localized'],
    ['SP-1200 Crunch',           'C++ factory-preset name (applyFactoryPreset) and the tip anchor — never localized'],
    ['Filtered & Enveloped',     'C++ factory-preset name (applyFactoryPreset) and the tip anchor — never localized'],

    // The two strings drawWaveformEditor paints with ctx.fillText. A 2D-context
    // string is not a DOM node: the canon sweep cannot reach it, neither gate
    // can see it, and localizing it would need a repaint hook outside the
    // canon. O-Orbit ships FRONT and ELEV the same way, and
    // O-MultiBandCompressor ships its analyzer placeholder the same way; this
    // is the suite's existing position on canvas text, recorded rather than
    // discovered. Named gap, owner none, carried into Stage M.
    ['drop or load a source to see its waveform',
     'canvas ctx.fillText, not a DOM node — the canon sweep cannot reach it; a named gap, matching O-Orbit and O-MultiBandCompressor'],
    ['root',
     'canvas ctx.fillText prefix on the root-key marker, not a DOM node — same named gap as the line above'],

    // The built-in source name. It is the embedded WAV's filename and the
    // string kBuiltInNames[0] in PluginProcessor.h; the UI mirrors it and
    // cannot translate it without disagreeing with the saved <SOURCE identity>.
    ['piano',
     'the embedded source filename, mirrored from kBuiltInNames[0] — it is also the identity attribute of the saved SOURCE state child, so translating it would desync the session'],
];

// ============================================================================
// TIP_BINDINGS — [selector, key, wrapper?, vars?]
//
// The tip anchor IS the element the selector finds: this page authors its tips
// on the CELL rather than on the knob inside it, so no closest(wrapper) walk is
// needed anywhere.
//
// Through v1.3.1 the anchors carried the tip KEY in their own data-tip
// attribute. That cannot survive canon v2 — applyI18n WRITES data-tip as the
// tip BODY, so the key and the copy would fight over one attribute, and
// check-i18n assertion 3 requires index.html to carry zero data-tip literals.
//
// The seventeen knob cells and the two select cells gained a data-param
// attribute naming the APVTS parameter they drive, and the data-param that used
// to sit on the inner .knob (unread by any JS or CSS) was moved onto the cell
// rather than duplicated — two elements answering [data-param="start"] would
// make querySelector's document-order rule load-bearing, which is exactly how
// O-Octagon's .vunit-group tip nearly landed on the wrong control in Stage C.
//
// pitchMode appears TWICE, on two different anchors: the select cell and the
// readout beside it are two halves of one control and say the same thing.
// ============================================================================

export const TIP_BINDINGS = [
    ['#gear-btn',                        'gear-btn'],
    ['#lang-select',                     'lang-select'],
    ['#help-toggle',                     'help-toggle'],

    ['#btnLoad',                         'loadSource'],
    ['#source-drop-zone',                'dropZone'],

    ['#viz-waveform',                    'vizWaveform'],
    ['#viz-filter',                      'vizFilter'],
    ['#viz-amp',                         'vizAmp'],
    ['#viz-scope',                       'vizScope'],

    ['[data-param="start"]',             'start'],
    ['[data-param="end"]',               'end'],
    ['[data-param="loopStart"]',         'loopStart'],
    ['[data-param="loopEnd"]',           'loopEnd'],
    ['[data-param="loopCrossfade"]',     'loopCrossfade'],
    ['[data-param="loopMode"]',          'loopMode'],
    ['#toggle-reverse',                  'reverse'],

    ['[data-param="rootKey"]',           'rootKey'],
    ['[data-param="pitchMode"]',         'pitchMode'],
    ['#pitchModeReadout',                'pitchMode'],
    ['[data-param="tune"]',              'tune'],
    ['[data-param="fine"]',              'fine'],

    ['[data-param="vintage"]',           'vintage'],

    ['[data-param="filterCutoff"]',      'filterCutoff'],
    ['[data-param="filterResonance"]',   'filterResonance'],

    ['[data-param="ampAttack"]',         'ampAttack'],
    ['[data-param="ampDecay"]',          'ampDecay'],
    ['[data-param="ampSustain"]',        'ampSustain'],
    ['[data-param="ampRelease"]',        'ampRelease'],
    ['[data-param="velToAmp"]',          'velToAmp'],

    ['[data-param="outputLevel"]',       'outputLevel'],

    ['.tour-btn[data-preset="Raw One-Shot"]',              'lessonRawOneShot'],
    ['.tour-btn[data-preset="Tuned Across the Keyboard"]', 'lessonTunedKeyboard'],
    ['.tour-btn[data-preset="Looped Pad"]',                'lessonLoopedPad'],
    ['.tour-btn[data-preset="Reversed Swell"]',            'lessonReversedSwell'],
    ['.tour-btn[data-preset="Repitch vs Stretch A/B"]',    'lessonRepitchStretch'],
    ['.tour-btn[data-preset="SP-1200 Crunch"]',            'lessonSp1200'],
    ['.tour-btn[data-preset="Filtered &amp; Enveloped"]',  'lessonFilteredEnv'],
];

export function tr(key, lang, vars) {
    const entry = I18N[key];
    if (!entry) { console.warn(`i18n: missing key ${key}`); return { t: key, b: '' }; }
    const s = entry[lang] || entry.en;

    // A var VALUE that is itself an I18N key resolves to that key's localized
    // title; anything else is used literally. This plugin needs neither arm in
    // its TOOLTIPS today, but the resolving arm is what lets a plugin compose a
    // localized name into a tip without pinning TIP_BINDINGS — which is static
    // data evaluated once — to the load-time language. The canon is one shape
    // across all 43 plugins; this function is not trimmed per plugin.
    const resolve = (v) => {
        const nested = I18N[v];
        return nested ? String((nested[lang] || nested.en).t) : String(v);
    };

    const sub = (v) => vars
        ? String(v).replace(/\{(\w+)\}/g, (m, n) => (n in vars ? resolve(vars[n]) : m))
        : String(v);

    return { t: sub(s.t), b: sub(s.b) };
}
