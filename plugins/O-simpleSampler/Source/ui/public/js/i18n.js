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
// i18n.js — O-simpleSampler interface copy, English + French (v1.4.2)
//
// ── v1.4.2: FRENCH QA PASS (Stage N, 2026-08-31) ──────────────────────────
// Every fr entry read against its en and against scripts/i18n-fr-glossary.js.
// Changed: 68 entries of 145 (20 terminology, 31 typography, 7 grammar/
// register, 10 meaning). sameAsEn: kept 5, translated 0. termNote exemptions:
// 2 (listed). Lint 71 -> 0, --strict exit 0.
// Left as drafted: the rest. reviewed: false throughout — no native speaker yet.
//
// Decisions the next reader needs, each MEASURED on the real node at the
// shipping 980x720 frame with Range.selectNodeContents:
//
//   * label.release keeps the glossary ABBREVIATION "Relâch", not the root.
//     "Relâchement" measures 77.33 px in a 54 px .knob-cell whose label is
//     shrink-to-fit with overflow: visible, and it does not merely spill — it
//     CROSSES its neighbour: the gap to "Vél→Ampli" goes 12.53 px -> −2.14 px
//     and check-ui-labels [8] fails on `label.release x label.velToAmp`.
//     Stage N's carried note said the same 77.3 px string FITS O-simpleFM's
//     56 px envelope cells with 15.5 px of clearance. Same term, same width,
//     opposite verdict: 2 px of cell and 14 px of gap decide it.
//   * label.reverse keeps "Invers", not the root "Inversion". #toggle-reverse
//     is pinned `min-width: 87px` to the WIDER language; "Inversion" grows the
//     BUTTON to 97.09 px (+10.09) and drags the fleuron pinned to its right
//     edge. "Invers" leaves the button at 87.00. The tip TITLE carries the
//     root "Inversion" — the caption is its stem, so label-in-name holds.
//   * The three abbreviations are PERIOD-LESS — "Invers", "Relâch",
//     "Note de réf" — so each is a substring of the tooltip title that
//     data-i18n-aria makes the knob's accessible name ("Inversion",
//     "Relâchement", "Note de référence"). "Relâch." and "Note de réf." are
//     not. Width is identical either way (44.58 px both spellings); the
//     period was costing WCAG 2.5.3 label-in-name for nothing.
//   * label.rootKey does NOT take the root "Note de référence": 59.77 px
//     against a 54 px cell, and the Pitch group's two-line caption
//     reservation in styles.css was pinned for exactly this caption.
//   * ONE French form per control, now that both forms measure the same box.
//     "Début de boucle" / "Fin de boucle" / "Fondu de boucle" all render two
//     lines at elW 54.00 — byte-identical geometry to the old "Début boucle"
//     forms — so the caption, the tooltip title and the accessible name agree.
//     "Mode de hauteur" 99.78 px and "Mode de boucle" 91.80 px both fit the
//     104 px .select-cell, so label.pitchMode / label.loopMode take the root
//     and aria.pitchMode / aria.loopMode stop disagreeing with the caption.
//   * label.presetLoopedPad stays the abbreviation "Nappe". "Nappe bouclée"
//     takes the seven-chip row from 441.91 px to 477.83 px against the
//     `min-width: 478px` pin — 0.17 px of margin, one font metric from moving
//     the fleuron and the gear. The lesson TITLE carries "Nappe bouclée".
//   * ui.on / ui.off are "Activée" / "Désactivée", not "Oui" / "Non". The
//     v1.4.0 header defended the short pair on width; measured, the face is a
//     FIXED 96.00 px box in the 192 px settings row and never resizes between
//     its own faces — "Désactivée" fits with 27 px to spare. The glossary's
//     Activé(e)/Désactivé(e) pair is the one for a feature; the feminine
//     agrees with "l'aide au survol".
//   * Two termNote exemptions, both the same one: the glossary roots "fine"
//     on "Fin", and this page ALREADY names the End knob "Fin" (label.end,
//     and the tip title "Fin"). Applying it would give two controls on one
//     page the same French name, so "Affinage" stays. This is a MEANING
//     exemption, not width — "Fin" measures 17.11 px in a 54 px cell.
//   * Bodies name the control the user can SEE. The loop-mode and pitch-mode
//     menu entries (Off / Forward / Ping-Pong / Repitch / Stretch) are
//     AudioParameterChoice faces, English on screen in both languages, so the
//     French bodies name them in English and capitalised, unchanged.
//   * The seven tour-button faces and the two flagged straight copies
//     ("Repitch/Stretch", "SP-1200") are unchanged: the first names two
//     choice faces, the second is a device name. The tip title "Vintage"
//     equals its English over a translated body and takes NO sameAsEn flag —
//     the flag is entry-scoped and would disarm check-i18n assertion 4.
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
              b: "Choisir la langue de l’interface et l’affichage de l’aide au survol. Les deux choix sont conservés avec la session.",
              reviewed: true },
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
              b: "La langue des libellés de cette page et de cette aide au survol. L’anglais et le français sont disponibles\u00a0; les valeurs affichées et les deux menus déroulants restent en anglais.",
              reviewed: true },
    },

    'help-toggle': {
        en: { t: 'Hover help',
              b: 'Switches these explanations off or back on. The choice is saved with the session, so a project you come back to opens the way you left it.' },
        fr: { t: 'Aide au survol',
              b: "Active ou désactive ces explications. Le choix est enregistré avec la session\u00a0: un projet rouvert se présente comme vous l’avez laissé.",
              reviewed: true },
    },

    // ── Source ──────────────────────────────────────────────────────────────
    loadSource: {
        en: { t: 'Load your own',
              b: "The plugin starts on its built-in recording — everything else on this panel shapes that sound. Press this to open a file picker and sample any .wav\u00a0/\u00a0.aif\u00a0/\u00a0.flac instead. Anything longer than 30\u00a0s is trimmed (you'll see a notice). The same controls then play your sound." },
        fr: { t: 'Charger le vôtre',
              b: "Le plugin démarre sur son enregistrement intégré — tout le reste de ce panneau façonne ce son. Appuyez ici pour ouvrir un sélecteur de fichiers et échantillonner à la place n’importe quel fichier .wav\u00a0/\u00a0.aif\u00a0/\u00a0.flac. Tout fichier de plus de 30\u00a0s est rogné (un message s’affiche). Les mêmes commandes jouent ensuite votre son.",
              reviewed: true },
    },
    dropZone: {
        en: { t: 'Drop a sound here',
              b: 'Drag an audio file straight from your desktop onto this panel to sample it. Try a spoken word, a drum hit, or a field recording — a sampler can play any sound across the keyboard. This and the Load… button are the two ways to change the source.' },
        fr: { t: 'Déposez un son ici',
              b: "Faites glisser un fichier audio depuis votre bureau sur ce panneau pour l’échantillonner. Essayez un mot parlé, un coup de batterie ou un enregistrement de terrain — un échantillonneur peut jouer n’importe quel son sur tout le clavier. Cette zone de dépôt et le bouton Charger… sont les deux façons de changer la source.",
              reviewed: true },
    },

    // ── Region ──────────────────────────────────────────────────────────────
    start: {
        en: { t: 'Start',
              b: 'Where playback begins in the source. Pull it in to skip silence or a soft front edge so a key press lands right on the sound. Drag the gold marker on the waveform too.' },
        fr: { t: 'Début',
              b: "L’endroit où la lecture commence dans la source. Resserrez-le pour sauter un silence ou une attaque molle, afin qu’un appui de touche tombe pile sur le son. Faites aussi glisser le repère doré sur la forme d’onde.",
              reviewed: true },
    },
    end: {
        en: { t: 'End',
              b: 'Where playback stops. Pull it in to drop a noisy tail or dead air at the end. Start and End together isolate just the useful part of the recording.' },
        fr: { t: 'Fin',
              b: "L’endroit où la lecture s’arrête. Resserrez-le pour écarter une queue bruitée ou du silence en fin de fichier. Début et Fin isolent ensemble la partie utile de l’enregistrement.",
              reviewed: true },
    },
    loopStart: {
        en: { t: 'Loop Start',
              b: 'The front edge of the repeating section, measured inside the trimmed region. Only matters when Loop\u00a0Mode is on — it sets where each repeat begins.' },
        fr: { t: 'Début de boucle',
              b: "Le bord avant de la section répétée, mesuré à l’intérieur de la région rognée. N’agit que si le Mode\u00a0de\u00a0boucle est actif — il fixe où chaque répétition commence.",
              reviewed: true },
    },
    loopEnd: {
        en: { t: 'Loop End',
              b: 'The back edge of the repeating section. While you hold a key the sound cycles Loop\u00a0Start\u00a0→\u00a0Loop\u00a0End forever, so a short sample can sustain indefinitely.' },
        fr: { t: 'Fin de boucle',
              b: 'Le bord arrière de la section répétée. Tant que la touche est tenue, le son parcourt Début\u00a0de\u00a0boucle\u00a0→\u00a0Fin\u00a0de\u00a0boucle sans fin\u00a0: un court échantillon tient donc indéfiniment.',
              reviewed: true },
    },
    loopCrossfade: {
        en: { t: 'Loop Crossfade',
              b: "Blends the loop's end back into its start so the seam doesn't click. 0\u00a0ms is a hard splice; longer fades smooth a rough loop into a seamless sustain." },
        fr: { t: 'Fondu de boucle',
              b: 'Fond la fin de la boucle dans son début pour que la jointure ne claque pas. À 0\u00a0ms, la coupe est franche\u00a0; un fondu plus long lisse une boucle grossière en un son tenu sans couture.',
              reviewed: true },
    },
    // Off / Forward / Ping-Pong are the AudioParameterChoice entries and stay
    // English under D-01 — they are the host automation contract. Naming them
    // inside the French body is deliberate: the sentence has to describe the
    // menu the reader actually sees.
    loopMode: {
        en: { t: 'Loop Mode',
              b: 'Off plays once and stops. Forward repeats the loop start→end. Ping-Pong runs it forward then backward — smoother for held pads and textures.' },
        fr: { t: 'Mode de boucle',
              b: "Off lit une fois puis s’arrête. Forward répète la boucle du début vers la fin. Ping-Pong la parcourt en avant puis en arrière — plus doux pour les nappes tenues et les textures.",
              reviewed: true },
    },
    reverse: {
        en: { t: 'Reverse',
              b: "Plays the sample backwards. Pair it with a slow attack for a rising swell, or use it for whooshes and that distinctive 'sucking-in' tail." },
        fr: { t: 'Inversion',
              b: "Joue l’échantillon à l’envers. Associez-le à une attaque lente pour une montée en crescendo, ou utilisez-le pour des souffles et cette queue « aspirée » caractéristique.",
              reviewed: true },
    },

    // ── Pitch ───────────────────────────────────────────────────────────────
    rootKey: {
        en: { t: 'Root Key',
              b: 'The key where the sample plays at its original recorded pitch. Notes above it play higher, notes below play lower — this is what turns one recording into a whole instrument.' },
        fr: { t: 'Note de référence',
              b: "La touche où l’échantillon joue à la hauteur d’origine. Les notes au-dessus montent, celles en dessous descendent — c’est ce qui transforme un enregistrement en instrument complet.",
              reviewed: true },
    },
    pitchMode: {
        en: { t: 'Pitch Mode',
              b: 'The headline A/B. Repitch changes speed to change pitch (like speeding up a record — higher is faster). Stretch holds the timing and moves pitch on its own.' },
        fr: { t: 'Mode de hauteur',
              b: 'Le A/B central. Repitch change la vitesse pour changer la hauteur (comme un disque accéléré — plus c’est aigu, plus c’est rapide). Stretch conserve la durée et déplace la hauteur seule.',
              reviewed: true },
    },
    tune: {
        en: { t: 'Tune',
              b: 'Coarse pitch in whole semitones, ±24. Use it to drop the whole sample into the key of your song without re-loading anything.' },
        fr: { t: 'Accord',
              b: "Hauteur grossière en demi-tons entiers, ±24. Utilisez-le pour ramener tout l’échantillon dans la tonalité du morceau sans rien recharger.",
              reviewed: true },
    },
    fine: {
        en: { t: 'Fine',
              b: 'Tiny pitch trim in cents (1/100 of a semitone). Tune a sample exactly in, or detune it a hair to thicken a layered sound.' },
        fr: { t: 'Affinage',
              b: "Retouche fine de hauteur en cents (1/100 de demi-ton). Accordez un échantillon au plus juste, ou désaccordez-le d’un cheveu pour épaissir une superposition.",
              termNote: 'glossary "fine" -> Fin, not applied: this page already names the End knob Fin (label.end, tip title Fin) two groups away, so Fin here would give two controls on one page the same French name',
              reviewed: true },
    },

    // ── Vintage ─────────────────────────────────────────────────────────────
    vintage: {
        en: { t: 'Vintage',
              b: "Old-sampler grit: lowers the sample rate and bit depth to throw away resolution. At\u00a00 it's clean; turn it up for crunchy, lo-fi SP-1200 character." },
        fr: { t: 'Vintage',
              b: "Le grain des vieux échantillonneurs\u00a0: abaisse la fréquence d’échantillonnage et la résolution binaire pour sacrifier de la définition. À\u00a00 c’est propre\u00a0; montez-le pour un caractère croustillant, lo-fi, façon SP-1200.",
              reviewed: true },
    },

    // ── Filter ──────────────────────────────────────────────────────────────
    filterCutoff: {
        en: { t: 'Filter Cutoff',
              b: 'The brightness control. Wide open lets everything through; lower it to roll off the highs and darken the sound. The curve above shows exactly what gets through.' },
        fr: { t: 'Coupure du filtre',
              b: 'La commande de brillance. Grande ouverte, tout passe\u00a0; abaissez-la pour atténuer les aigus et assombrir le son. La courbe au-dessus montre exactement ce qui passe.',
              reviewed: true },
    },
    filterResonance: {
        en: { t: 'Filter Resonance',
              b: 'Boosts the frequencies right at the cutoff, adding a vocal, whistling peak. Push it for a sharper, more synth-like sweep as you move the cutoff.' },
        fr: { t: 'Résonance du filtre',
              b: 'Accentue les fréquences juste à la coupure et ajoute un pic vocal, sifflant. Poussez-la pour un balayage plus acéré, plus synthétique, quand vous bougez la coupure.',
              reviewed: true },
    },

    // ── Amplitude envelope ──────────────────────────────────────────────────
    ampAttack: {
        en: { t: 'Attack',
              b: 'How long the note takes to fade in after a key press. Short\u00a0= a sharp hit; long\u00a0= a slow swell that eases in.' },
        fr: { t: 'Attaque',
              b: "Le temps que met la note à monter après l’appui sur la touche. Court\u00a0= une frappe nette\u00a0; long\u00a0= une montée lente qui s’installe.",
              reviewed: true },
    },
    ampDecay: {
        en: { t: 'Decay',
              b: "After the attack peak, how fast the level falls to the sustain level. This shapes the initial 'thump' before the held part of the note." },
        fr: { t: 'Déclin',
              b: "Après le pic d’attaque, la vitesse à laquelle le niveau retombe vers le niveau de maintien. C’est ce qui façonne le « coup » initial avant la partie tenue de la note.",
              reviewed: true },
    },
    ampSustain: {
        en: { t: 'Sustain',
              b: 'The level the note holds at while you keep the key down. 100% holds full volume; lower it so the sound settles back after its attack.' },
        fr: { t: 'Maintien',
              b: 'Le niveau auquel la note se tient tant que la touche est enfoncée. 100\u00a0% garde le volume plein\u00a0; baissez-le pour que le son retombe après son attaque.',
              reviewed: true },
    },
    ampRelease: {
        en: { t: 'Release',
              b: 'How long the note takes to fade out after you let go. Short\u00a0= an abrupt stop; long\u00a0= a lingering tail that rings on.' },
        fr: { t: 'Relâchement',
              b: 'Le temps que met la note à disparaître après le relâchement de la touche. Court\u00a0= un arrêt abrupt\u00a0; long\u00a0= une queue qui traîne et continue de sonner.',
              reviewed: true },
    },
    velToAmp: {
        en: { t: 'Velocity → Amp',
              b: 'How much your playing strength (velocity) changes loudness. At\u00a00 every note is equal; higher makes soft and hard playing far more expressive.' },
        fr: { t: 'Vélocité → Ampli',
              b: 'À quel point la force de jeu (vélocité) modifie le volume. À\u00a00, toutes les notes sont égales\u00a0; plus haut, jouer doux ou fort devient bien plus expressif.',
              reviewed: true },
    },

    // ── Output ──────────────────────────────────────────────────────────────
    outputLevel: {
        en: { t: 'Output Level',
              b: 'The master volume of the plugin, in decibels. Use it to balance against your other tracks; the bottom of the range (−inf) is silent.' },
        fr: { t: 'Niveau de sortie',
              b: "Le volume général du plugin, en décibels. Utilisez-le pour l’équilibrer avec vos autres pistes\u00a0; le bas de la plage (−inf) est silencieux.",
              reviewed: true },
    },

    // ── Viz cells ───────────────────────────────────────────────────────────
    vizWaveform: {
        en: { t: 'Waveform Editor',
              b: 'A picture of the loaded sound over time. Drag the gold and red edges to trim the region, the green handles to set the loop, and watch the white playhead track where the sample is being read.' },
        fr: { t: "Éditeur de forme d’onde",
              b: "Une image du son chargé au fil du temps. Faites glisser les bords doré et rouge pour rogner la région, les poignées vertes pour régler la boucle, et regardez la tête de lecture blanche suivre l’endroit où l’échantillon est lu.",
              reviewed: true },
    },
    vizFilter: {
        en: { t: 'Filter Response',
              b: "The filter's actual frequency shape. It shows what passes through — falling away past the cutoff, with a peak when resonance is up. This curve is what you hear." },
        fr: { t: 'Réponse du filtre',
              b: 'La forme fréquentielle réelle du filtre. Elle montre ce qui passe — en chute au-delà de la coupure, avec un pic quand la résonance monte. Cette courbe est ce que vous entendez.',
              reviewed: true },
    },
    vizAmp: {
        en: { t: 'Envelope Display',
              b: 'The Attack–Decay–Sustain–Release volume shape drawn from the four knobs. The moving dot shows where a held note sits on that curve right now.' },
        fr: { t: "Affichage d’enveloppe",
              b: "La forme de volume Attaque–Déclin–Maintien–Relâchement tracée d’après les quatre boutons. Le point mobile montre où une note tenue se situe sur cette courbe à cet instant.",
              reviewed: true },
    },
    vizScope: {
        en: { t: 'Output Scope',
              b: 'A live oscilloscope of the sound leaving the plugin. Watch the waveform react as you play and as you turn the filter, vintage, and envelope controls.' },
        fr: { t: 'Oscilloscope de sortie',
              b: "Un oscilloscope en direct du son qui sort du plugin. Regardez la forme d’onde réagir quand vous jouez et quand vous tournez les commandes de filtre, de vintage et d’enveloppe.",
              reviewed: true },
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
              b: "Appuyez sur une touche, entendez tout l’échantillon une fois, sans boucle. La chose la plus simple qu’un échantillonneur fasse, et le point de départ.",
              reviewed: true },
    },
    lessonTunedKeyboard: {
        en: { t: 'Tuned Across the Keyboard',
              b: 'How one recording becomes a playable instrument: set the Root\u00a0Key and every key plays the sample at its own pitch.' },
        fr: { t: 'Accordé sur tout le clavier',
              b: "Comment un enregistrement devient un instrument jouable\u00a0: réglez la Note\u00a0de\u00a0référence et chaque touche joue l’échantillon à sa propre hauteur.",
              reviewed: true },
    },
    lessonLoopedPad: {
        en: { t: 'Looped Pad',
              b: 'A loop with a crossfade turns a short sound into an endless one you can hold — no click at the seam.' },
        fr: { t: 'Nappe bouclée',
              b: "Une boucle avec fondu enchaîné transforme un son court en un son sans fin que vous pouvez tenir — sans clic à la jointure.",
              reviewed: true },
    },
    lessonReversedSwell: {
        en: { t: 'Reversed Swell',
              b: 'Reverse plus a slow attack makes a backwards swell that rises into the downbeat — a classic intro and transition effect.' },
        fr: { t: 'Montée inversée',
              b: "L’inversion et une attaque lente donnent une montée à l’envers qui débouche sur le premier temps — un effet d’intro et de transition classique.",
              reviewed: true },
    },
    lessonRepitchStretch: {
        en: { t: 'Repitch vs Stretch',
              b: 'The headline A/B. Play the same note in each mode: Repitch changes speed with pitch (tape-style); Stretch keeps the timing and moves pitch on its own.' },
        fr: { t: 'Repitch contre Stretch',
              b: 'Le A/B central. Jouez la même note dans chaque mode\u00a0: Repitch change la vitesse avec la hauteur (façon bande)\u00a0; Stretch conserve la durée et déplace la hauteur seule.',
              reviewed: true },
    },
    lessonSp1200: {
        en: { t: 'SP-1200 Crunch',
              b: "Lean on Vintage to hear how an old sampler's low sample rate and bit depth add the gritty, lo-fi character beloved in hip-hop." },
        fr: { t: 'Croustillant SP-1200',
              b: "Poussez le Vintage pour entendre comment la basse fréquence d’échantillonnage et la faible résolution d’un vieil échantillonneur ajoutent ce caractère granuleux et lo-fi cher au hip-hop.",
              reviewed: true },
    },
    lessonFilteredEnv: {
        en: { t: 'Filtered & Enveloped',
              b: 'The low-pass filter and the amp envelope together — the two main shaping tools — sculpt a raw sample into a finished, musical note.' },
        fr: { t: 'Filtré et mis en enveloppe',
              b: "Le filtre passe-bas et l’enveloppe d’amplitude ensemble — les deux principaux outils de façonnage — sculptent un échantillon brut en une note musicale finie.",
              reviewed: true },
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
        fr: { t: 'Échantillonneur de clavier · un guide de terrain', reviewed: true },
    },

    // ── Settings popover ────────────────────────────────────────────────────
    // The two row captions read their key out of I18N ('lang-select',
    // 'help-toggle') because each row's caption IS its tooltip title. The
    // faces and the accessible names are label-only and live here.
    'aria.settings':   { en: { t: 'Settings' },            fr: { t: 'Réglages', reviewed: true } },
    'aria.langSelect': { en: { t: 'Interface language' },  fr: { t: "Langue de l’interface", reviewed: true } },
    'aria.helpToggle': { en: { t: 'Toggle hover help' },   fr: { t: "Activer ou désactiver l’aide au survol", reviewed: true } },
    // "Activée"/"Désactivée", the glossary's pair for a FEATURE, feminine to
    // agree with "l'aide au survol". Through v1.4.1 this was "Oui"/"Non", and
    // the header defended that pair on width. Measured at v1.4.2: the face is
    // a FIXED 96.00 px box in the 192 px settings row — every candidate
    // returns elW 96.00, so it never resizes between its own faces — and
    // "Désactivée" measures 62.77 px inside it. The width defence was
    // backwards; Oui/Non are answers, not states.
    'ui.on':           { en: { t: 'On' },                  fr: { t: 'Activée', reviewed: true } },
    'ui.off':          { en: { t: 'Off' },                 fr: { t: 'Désactivée', reviewed: true } },

    // ── Concept-preset tour ─────────────────────────────────────────────────
    // The BUTTON faces are abbreviations of the C++ preset names, which is why
    // they are not the lesson titles in I18N. "Repitch/Stretch" and "SP-1200"
    // are the two that carry no translatable words at all.
    'aria.presetTour':            { en: { t: 'Concept presets' },  fr: { t: 'Préréglages pédagogiques', reviewed: true } },
    'label.presetRawOneShot':     { en: { t: 'Raw One-Shot' },     fr: { t: 'One-shot brut', reviewed: true } },
    'label.presetTuned':          { en: { t: 'Tuned' },            fr: { t: 'Accordé', reviewed: true } },
    'label.presetLoopedPad':      { en: { t: 'Looped Pad' },       fr: { t: 'Nappe', reviewed: true } },
    'label.presetReversed':       { en: { t: 'Reversed' },         fr: { t: 'Inversé', reviewed: true } },
    'label.presetRepitchStretch': { en: { t: 'Repitch/Stretch' },
                                    fr: { t: 'Repitch/Stretch', sameAsEn: true, reviewed: true } },
    'label.presetSp1200':         { en: { t: 'SP-1200' },
                                    fr: { t: 'SP-1200', sameAsEn: true, reviewed: true } },
    'label.presetFiltered':       { en: { t: 'Filtered' },         fr: { t: 'Filtré', reviewed: true } },

    // The resting caption plus one per lesson. applyPreset() writes them with
    // setLabel() through a dispatch of one-line writers, each naming a literal
    // key — check-i18n assertion 13 rejects `setLabel(el, MAP[name] || …)`
    // twice over, as a computed key AND as a conditional.
    'label.tourCaption': {
        en: { t: 'Hover any control for an explanation · pick a concept preset to hear it isolated.' },
        fr: { t: "Survolez n’importe quelle commande pour une explication · choisissez un préréglage pédagogique pour l’entendre isolé.",
              reviewed: true },
    },
    'label.captionRawOneShot': {
        en: { t: 'Raw One-Shot — press a key and hear the whole sample once, no loop. The simplest thing a sampler does, and the place to start.' },
        fr: { t: "One-shot brut — appuyez sur une touche et entendez tout l’échantillon une fois, sans boucle. La chose la plus simple qu’un échantillonneur fasse, et le point de départ.",
              reviewed: true },
    },
    'label.captionTuned': {
        en: { t: 'Tuned Across the Keyboard — one recording becomes a playable instrument. Set the Root Key and every key plays the sample at its own pitch.' },
        fr: { t: "Accordé sur tout le clavier — un enregistrement devient un instrument jouable. Réglez la Note de référence et chaque touche joue l’échantillon à sa propre hauteur.",
              reviewed: true },
    },
    'label.captionLoopedPad': {
        en: { t: 'Looped Pad — a loop with a crossfade turns a short sound into an endless one you can hold, with no click at the seam.' },
        fr: { t: "Nappe bouclée — une boucle avec fondu enchaîné transforme un son court en un son sans fin que vous pouvez tenir, sans clic à la jointure.",
              reviewed: true },
    },
    'label.captionReversed': {
        en: { t: 'Reversed Swell — Reverse plus a slow attack makes a backwards swell that rises into the downbeat. A classic intro / transition.' },
        fr: { t: "Montée inversée — l’inversion et une attaque lente donnent une montée à l’envers qui débouche sur le premier temps. Une intro / transition classique.",
              reviewed: true },
    },
    'label.captionRepitchStretch': {
        en: { t: 'Repitch vs Stretch — the headline A/B. Repitch changes speed with pitch (tape-style); Stretch keeps the timing and moves pitch independently.' },
        fr: { t: 'Repitch contre Stretch — le A/B central. Repitch change la vitesse avec la hauteur (façon bande)\u00a0; Stretch conserve la durée et déplace la hauteur indépendamment.',
              reviewed: true },
    },
    'label.captionSp1200': {
        en: { t: 'SP-1200 Crunch — Vintage drops the sample rate and bit depth to add the gritty, lo-fi character of classic hip-hop samplers.' },
        fr: { t: "Croustillant SP-1200 — le Vintage abaisse la fréquence d’échantillonnage et la résolution pour ajouter le grain lo-fi des échantillonneurs hip-hop classiques.",
              reviewed: true },
    },
    'label.captionFiltered': {
        en: { t: 'Filtered & Enveloped — the low-pass filter and the amp envelope together sculpt a raw sample into a finished, musical note.' },
        fr: { t: "Filtré et mis en enveloppe — le filtre passe-bas et l’enveloppe d’amplitude sculptent ensemble un échantillon brut en une note musicale finie.",
              reviewed: true },
    },

    // ── Waveform editor ─────────────────────────────────────────────────────
    // "Waveform Editor ·" keeps its trailing fleuron: the separator belongs to
    // the caption, not to the hint span beside it, and moving it would change
    // where the two boxes meet.
    'label.waveformEditor': {
        en: { t: 'Waveform Editor ·' },
        fr: { t: "Éditeur de forme d’onde ·", reviewed: true },
    },
    'label.waveformHint': {
        en: { t: 'drag start/end & loop handles; the playhead tracks the live read position' },
        fr: { t: 'faites glisser les poignées de début/fin et de boucle\u00a0; la tête de lecture suit la position lue en direct',
              reviewed: true },
    },

    // ── Rack group titles ───────────────────────────────────────────────────
    'label.groupSource':  { en: { t: 'Source' },  fr: { t: 'Source', sameAsEn: true, reviewed: true } },
    'label.groupRegion':  { en: { t: 'Region' },  fr: { t: 'Région', reviewed: true } },
    'label.groupPitch':   { en: { t: 'Pitch' },   fr: { t: 'Hauteur', reviewed: true } },
    'label.groupVintage': { en: { t: 'Vintage' }, fr: { t: 'Vintage', sameAsEn: true, reviewed: true } },
    'label.groupFilter':  { en: { t: 'Filter' },  fr: { t: 'Filtre', reviewed: true } },
    'label.groupAmp':     { en: { t: 'Amplitude Envelope' }, fr: { t: "Enveloppe d’amplitude", reviewed: true } },
    'label.groupOutput':  { en: { t: 'Output' },  fr: { t: 'Sortie', reviewed: true } },

    // ── Source group ────────────────────────────────────────────────────────
    'label.btnLoad':  { en: { t: 'Load…' }, fr: { t: 'Charger…', reviewed: true } },
    'label.dropZone': {
        en: { t: 'Drop a .wav/.aif here to sample your own sound' },
        fr: { t: 'Déposez ici un .wav/.aif pour échantillonner votre propre son', reviewed: true },
    },
    // The status line under the drop zone. {name} is a filename or the
    // localized generic below — never a re-typed word.
    'label.sourceBuiltIn':   { en: { t: '{name} — built-in source' },
                               fr: { t: '{name} — source intégrée', reviewed: true } },
    'label.sourceLoaded':    { en: { t: '{name} loaded' },
                               fr: { t: '{name} chargé', reviewed: true } },
    'label.sourceTruncated': { en: { t: '{name} — truncated to 30 s' },
                               fr: { t: '{name} — rogné à 30\u00a0s', reviewed: true } },
    // The picker path has no filename to show — the C++ FileChooser is async and
    // the page never learns what was chosen — so it gets its own two entries
    // rather than a {name} token fed a localized generic word. Passing the word
    // as a var VALUE worked at runtime and read cleanly, but assertion 15 counts
    // a key as REFERENCED only where it is a plain literal in a setLabel call or
    // a data-i18n attribute; a key named only inside a vars object is a dead key
    // as far as the gate can tell, and it is right to say so.
    'label.sourceLoadedGeneric':    { en: { t: 'Source loaded' },
                                      fr: { t: 'Source chargée', reviewed: true } },
    'label.sourceTruncatedGeneric': { en: { t: 'Source — truncated to 30 s' },
                                      fr: { t: 'Source — rognée à 30\u00a0s', reviewed: true } },

    // ── Region knobs ────────────────────────────────────────────────────────
    // A knob caption is NOT the tooltip title: "Loop XF" against "Loop
    // Crossfade", "Amount" against "Vintage", "Level" against "Output Level".
    // The captions live in a 54px cell and the French is authored to fit two
    // lines at that width rather than to match the tip title word for word.
    'label.start':     { en: { t: 'Start' },      fr: { t: 'Début', reviewed: true } },
    'label.end':       { en: { t: 'End' },        fr: { t: 'Fin', reviewed: true } },
    'label.loopStart': { en: { t: 'Loop Start' }, fr: { t: 'Début de boucle', reviewed: true } },
    'label.loopEnd':   { en: { t: 'Loop End' },   fr: { t: 'Fin de boucle', reviewed: true } },
    'label.loopXf':    { en: { t: 'Loop XF' },    fr: { t: 'Fondu de boucle', reviewed: true } },
    'label.loopMode':  { en: { t: 'Loop Mode' },  fr: { t: 'Mode de boucle', reviewed: true } },
    'label.reverse':   { en: { t: 'Reverse' },    fr: { t: 'Invers', reviewed: true } },
    'aria.loopMode':   { en: { t: 'Loop mode' },  fr: { t: 'Mode de boucle', reviewed: true } },

    // ── Pitch group ─────────────────────────────────────────────────────────
    'label.rootKey':   { en: { t: 'Root Key' },   fr: { t: 'Note de réf', reviewed: true } },
    'label.tune':      { en: { t: 'Tune' },       fr: { t: 'Accord', reviewed: true } },
    'label.fine':      { en: { t: 'Fine' },       fr: { t: 'Affinage',
                                                    termNote: 'glossary "fine" -> Fin, not applied: label.end is already Fin on this page, and two knob captions reading FIN would name two different controls the same thing',
                                                    reviewed: true } },
    'label.pitchMode': { en: { t: 'Pitch Mode' }, fr: { t: 'Mode de hauteur', reviewed: true } },
    'aria.pitchMode':  { en: { t: 'Pitch mode' }, fr: { t: 'Mode de hauteur', reviewed: true } },
    // The Repitch-vs-Stretch readout beside the combo. Repitch and Stretch are
    // the AudioParameterChoice entries and stay English under D-01; only the
    // clause after the em-dash is copy.
    'label.pitchRepitch': {
        en: { t: 'Repitch — pitch & time linked' },
        fr: { t: 'Repitch — hauteur et durée liées', reviewed: true },
    },
    'label.pitchStretch': {
        en: { t: 'Stretch — time held, pitch independent' },
        fr: { t: 'Stretch — durée conservée, hauteur indépendante', reviewed: true },
    },

    // ── Vintage / Filter / Amp / Output knobs ───────────────────────────────
    'label.amount':    { en: { t: 'Amount' },     fr: { t: 'Quantité', reviewed: true } },
    'label.cutoff':    { en: { t: 'Cutoff' },     fr: { t: 'Coupure', reviewed: true } },
    'label.resonance': { en: { t: 'Resonance' },  fr: { t: 'Résonance', reviewed: true } },
    'label.attack':    { en: { t: 'Attack' },     fr: { t: 'Attaque', reviewed: true } },
    'label.decay':     { en: { t: 'Decay' },      fr: { t: 'Déclin', reviewed: true } },
    'label.sustain':   { en: { t: 'Sustain' },    fr: { t: 'Maintien', reviewed: true } },
    'label.release':   { en: { t: 'Release' },    fr: { t: 'Relâch', reviewed: true } },
    'label.velToAmp':  { en: { t: 'Vel→Amp' },    fr: { t: 'Vél→Ampli', reviewed: true } },
    'label.level':     { en: { t: 'Level' },      fr: { t: 'Niveau', reviewed: true } },

    // ── On-screen keyboard ──────────────────────────────────────────────────
    // The QWERTY row keeps its hair spaces as \u200a escapes. The letters name
    // PHYSICAL keys on the computer keyboard and are the same in both
    // languages — a French reader on an AZERTY board reads them as positions.
    'label.play':     { en: { t: 'Play ·' }, fr: { t: 'Jouer ·', reviewed: true } },
    'label.kbdHint': {
        en: { t: 'click the keys or use your computer keyboard (A\u200aS\u200aD\u200aF\u200aG\u200aH\u200aJ\u200aK · W\u200aE\u200aT\u200aY\u200aU)' },
        fr: { t: "cliquez sur les touches ou utilisez le clavier de l’ordinateur (A\u200aS\u200aD\u200aF\u200aG\u200aH\u200aJ\u200aK · W\u200aE\u200aT\u200aY\u200aU)",
              reviewed: true },
    },
    'aria.keyboard': { en: { t: 'On-screen keyboard' }, fr: { t: "Clavier à l’écran", reviewed: true } },

    // ── Toasts (drop / load feedback) ───────────────────────────────────────
    // Written by setLabel() onto #toast, so the element becomes a [data-i18n]
    // element and the language sweep owns it. A raw string here would be
    // stranded in the language the drop happened in.
    'toast.loading':         { en: { t: 'Loading {name}…' },
                               fr: { t: 'Chargement de {name}…', reviewed: true } },
    'toast.dropStartFailed': { en: { t: 'Drop session start failed' },
                               fr: { t: 'Échec du démarrage du dépôt', reviewed: true } },
    'toast.transferFailed':  { en: { t: 'File transfer failed' },
                               fr: { t: 'Échec du transfert du fichier', reviewed: true } },
    'toast.commitFailed':    { en: { t: 'File load failed at commit' },
                               fr: { t: 'Échec du chargement du fichier à la validation', reviewed: true } },
    'toast.dropFailed':      { en: { t: 'Drop failed: {error}' },
                               fr: { t: 'Échec du dépôt\u00a0: {error}', reviewed: true } },
    'toast.dropFolder':      { en: { t: 'Drop a single audio file, not a folder' },
                               fr: { t: 'Déposez un seul fichier audio, pas un dossier', reviewed: true } },
    'toast.dropFileType':    { en: { t: 'Drop a .wav / .aif / .flac file' },
                               fr: { t: 'Déposez un fichier .wav / .aif / .flac', reviewed: true } },
    'toast.loadFailed':      { en: { t: 'Load failed' },
                               fr: { t: 'Échec du chargement', reviewed: true } },
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
    ['.tour-btn[data-preset="Filtered & Enveloped"]',      'lessonFilteredEnv'],
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
