/*
   This file is part of O-Prism, an Ouaricon Audio plugin.
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
// i18n.js — O-Prism UI labels and hover-help, English + French (v1.22.0)
//
// An ES module that EXPORTS ONLY. A bare top-level statement here throws out of
// module evaluation and takes every later initializer on the page with it
// (pattern_module_toplevel_init_tdz). check-i18n assertion 7 enforces it.
//
// TWO TABLES, TWO JOBS. LABELS carries the page's own visible captions and has
// one string per entry. I18N carries HOVER-HELP and has a title AND a body per
// entry, bound to an anchor through TIP_BINDINGS at the foot of this file. The
// canon writes both onto the DOM; only the labels are painted by it. The tooltip
// SURFACE is per-plugin page code — setupTooltips() in index.html — and did not
// exist before v1.22.0, which is why authoring these 107 bodies without it would
// have shipped 107 invisible strings past three green gates.
//
// THE ENGLISH WAS MOVED, NOT REWRITTEN. Every `en` entry is byte-for-byte what
// index.html carried through v1.20.0, with two deliberate exceptions, both
// recorded in CHANGELOG.md:
//   - `label.intervalCount` drops the word "notes" from "Intervals (12 notes)".
//     Contract section 6: French pluralizes zero as singular where English does
//     not, and the existing English already reads "1 notes" on a one-degree
//     scale. The count in a panel captioned "Intervals" needs no noun.
//   - `label.a4Ref` keeps "A4" untranslated inside "Réf. A4". French note
//     naming would make it "La3", and a pitch designation printed beside a
//     440.0 Hz readout is a technical reference, not prose.
//
// FILENAME: no hyphen. juce_add_binary_data STRIPS hyphens rather than
// converting them to underscores (critical_binary_data_strips_hyphens), so one
// combined file for both languages sidesteps the question entirely.
//
// COPY IS textContent ON EVERY PATH — never innerHTML. check-i18n assertion 9
// rejects any innerHTML reference here and any string literal containing `<`.
//
// GEOMETRY BUDGET. 64 of the labels below are knob captions inside
// `.knob-container`, an inline-flex box that SHRINK-WRAPS: it is 52 px wide
// because `.knob-visual` is, and a caption wider than that widens the container
// and pushes every knob to its right. Several French words were shortened for
// that reason and only that reason — `Prof.`, `Réso`, `Étir.`, `Amort.` — and
// the caption-fit probe measured every one against its own content box. A
// native speaker reviewing this file should read those as layout constraints,
// not as preferred vocabulary.
//
// ALL FRENCH IS MACHINE-DRAFTED AND FLAGGED `reviewed: false`. No native
// speaker has read it. `node scripts/check-i18n.js` prints the worklist.
// ============================================================================

export const LANGUAGES = ['en', 'fr'];

// ============================================================================
// I18N — hover-help, added in v1.22.0
// ============================================================================
//
// 107 entries: 105 parameter tips and 2 chrome tips. Every one carries an `en`
// and an `fr` with a title `t` and a body `b`; assertion 1 requires all four.
//
// EVERY RANGE HERE WAS READ OFF THE PAGE'S OWN FORMATTER, not off the parameter
// dump. All 173 of O-Prism's parameters carry an EMPTY `label` column — 0 %, the
// worst in the suite — so there is no unit to inherit and inventing one is
// forbidden. Each formatter's index.html line is cited in the group comments
// below; `bindKnob(paramId, formatFn, defaultNorm, size)` at index.html:2513 is
// where they are passed.
//
// A BODY IS PROSE, so it takes FRENCH CONVENTION: decimal comma, a space before
// `%`, U+2212 for a minus sign. The READOUT keeps its point, because D-03
// exempts the readout NODE and that has not moved. They differ on purpose — the
// readout is a machine-formatted value, the body is a sentence.
//
// An AudioParameterChoice option named inside a body is PROSE and is described
// in French; the option in the dropdown stays English (D-01 arm 1, and it is in
// I18N_EXEMPT below) so the page and the host automation lane agree. Where a
// body names one, it names the English word the user is looking at.
//
// 68 OF THE 173 PARAMETERS GET NO TIP, AND EACH IS A FINDING RATHER THAN A GAP:
//   * 64 mod-matrix parameters (modSlot0..15 x Src/Dst/Amt/On). `#mod-matrix-rows`
//     is EMPTY in the static markup (index.html:1599); an async IIFE at :2989
//     awaits getModSourceNames()/getModDestNames() and only then builds 16 rows
//     with `const prefix = 'modSlot' + i`. Because of the await, all 64 anchors
//     are absent from the DOM when applyI18n() runs, so a TIP_BINDINGS entry for
//     any of them resolves to null, warns `i18n: tip target not found`, and binds
//     nothing. Measurable without running the page: 0 of the 64 modSlot* IDs
//     appear as a whole string literal anywhere in the served root — they exist
//     only as that concatenation. localizeSubtree() (:1905) is this page's
//     injected-subtree hook and does NOT solve it: it loops applyLabel over
//     [data-i18n] and applyI18nAttributes over aria/placeholder/alt, writes no
//     tip attributes at all, and the mod-matrix builder never calls it. Reaching
//     these needs a canon decision, not a one-plugin workaround.
//   * `tonic` — it HAS a control, the `.tonic-selector` arrows, but that markup
//     is injected by updateIntervalListUI() (:3918) after `await getTonicNote()`,
//     from a tuning IIFE that starts on `setTimeout(tryInit, 300)`. Same class as
//     the 64: absent at applyI18n() time.
//   * `tuningPreset` — no control at all. The page takes a slider state for it
//     (:3017) only to LISTEN: a change schedules a tuning refresh. The library
//     list loads a tuning through the loadEmbeddedTuning native fn, and
//     PluginEditor.cpp:167 forces the parameter to Custom on a hand edit. Host-
//     reachable, page-unreachable.
//   * `stereoWidth` and `velocityCurve` — zero occurrences anywhere in the served
//     root. Both are live DSP (PluginProcessor.cpp:889-902, PrismVoice.cpp:131)
//     and both are set by factory presets, so a DAW can automate a width and a
//     velocity response the user cannot see.
// No control was added to satisfy a count. That is the standing rule of this
// stage and it held here at a scale of 68.
// ============================================================================

export const I18N = Object.freeze({

    // ── Oscillator A ────────────────────────────────────────────────────────
    // Formatters, all from index.html: pct :2469, panFmt :2474, coarseFmt :2526,
    // fineFmt :2530, unisonFmt :2534.

    'tip.oscATable': {
        en: { t: 'Osc A Wavetable',
              b: 'Chooses the wavetable oscillator A reads. The 28 factory tables are grouped '
               + 'Analog, Digital, Formant, Spectral and Organic; dropping a WAV file on the '
               + 'display above loads your own instead. Range: 28 factory tables plus any you import.' },
        fr: { t: 'Table d’onde osc A',
              b: 'Choisit la table d’onde lue par l’oscillateur A. Les 28 tables d’usine sont '
               + 'regroupées en Analog, Digital, Formant, Spectral et Organic ; déposer un fichier '
               + 'WAV sur l’affichage ci-dessus charge la vôtre à la place. Plage : 28 tables '
               + 'd’usine, plus celles que vous importez.',
              reviewed: false },
    },
    'tip.oscAPos': {
        en: { t: 'Osc A Position',
              b: 'Scans through the frames of the selected wavetable, morphing smoothly between '
               + 'them rather than stepping. Modulate it from an LFO or the filter envelope for '
               + 'the moving timbre wavetable synthesis is for. Range 0 to 100 %.' },
        fr: { t: 'Position osc A',
              b: 'Balaie les trames de la table sélectionnée, en fondu continu plutôt que par '
               + 'paliers. À moduler depuis un OBF ou l’enveloppe de filtre pour obtenir le timbre '
               + 'mouvant qui fait tout l’intérêt de la synthèse à tables d’onde. Plage de 0 à 100 %.',
              reviewed: false },
    },
    'tip.oscALevel': {
        en: { t: 'Osc A Level',
              b: 'Output level of oscillator A before the filters. The balance against oscillator B '
               + 'is set separately by Osc Mix in the footer, so this is the control to trim when '
               + 'unison or a loud table pushes the voice hot. Range 0 to 100 %.' },
        fr: { t: 'Niveau osc A',
              b: 'Niveau de sortie de l’oscillateur A avant les filtres. L’équilibre avec '
               + 'l’oscillateur B se règle séparément par Mix osc en pied de page ; utilisez donc '
               + 'cette commande pour corriger un unisson ou une table trop forte. Plage de 0 à 100 %.',
              reviewed: false },
    },
    'tip.oscAPan': {
        en: { t: 'Osc A Pan',
              b: 'Places oscillator A in the stereo field, before the filters and the effects. '
               + 'Panning A and B to opposite sides is the cheapest way to widen a two-oscillator '
               + 'patch. Range 100L through C to 100R.' },
        fr: { t: 'Panoramique osc A',
              b: 'Place l’oscillateur A dans l’image stéréo, avant les filtres et les effets. Étaler '
               + 'A et B de part et d’autre est la façon la plus simple d’élargir un son à deux '
               + 'oscillateurs. Plage de 100L à 100R, C au centre.',
              reviewed: false },
    },
    'tip.oscACoarse': {
        en: { t: 'Osc A Coarse',
               b: 'Transposes oscillator A in whole semitones. Two octaves either way, so a fifth, '
               + 'an octave or a two-octave lead layer are all one turn away. Range −24 to +24 st.' },
        fr: { t: 'Accord grossier osc A',
              b: 'Transpose l’oscillateur A par demi-tons entiers. Deux octaves de part et d’autre : '
               + 'une quinte, une octave ou une couche de lead à deux octaves ne sont qu’à un geste. '
               + 'Plage de −24 à +24 demi-tons.',
              reviewed: false },
    },
    'tip.oscAFine': {
        en: { t: 'Osc A Fine',
              b: 'Detunes oscillator A in cents, a full semitone either way. A few cents against '
               + 'oscillator B gives the slow beating that thickens a patch without unison. '
               + 'Range −100 to +100 ct.' },
        fr: { t: 'Accord fin osc A',
              b: 'Désaccorde l’oscillateur A en cents, jusqu’à un demi-ton de part et d’autre. '
               + 'Quelques cents d’écart avec l’oscillateur B produisent le battement lent qui '
               + 'épaissit un son sans recourir à l’unisson. Plage de −100 à +100 cents.',
              reviewed: false },
    },
    'tip.oscAPhase': {
        en: { t: 'Osc A Phase',
              b: 'Sets the point in the waveform where oscillator A starts on every note-on. A '
               + 'fixed start makes attacks identical from note to note, which matters most on '
               + 'short percussive sounds. Range 0 to 100 %.' },
        fr: { t: 'Phase osc A',
              b: 'Fixe le point de la forme d’onde où l’oscillateur A démarre à chaque note. Un '
               + 'départ fixe rend les attaques identiques d’une note à l’autre, ce qui compte '
               + 'surtout sur les sons percussifs courts. Plage de 0 à 100 %.',
              reviewed: false },
    },
    'tip.oscAUnison': {
        en: { t: 'Osc A Unison',
              b: 'Stacks up to eight detuned copies of oscillator A on one note. The stack is gain-'
               + 'compensated, so raising it thickens the sound without making it louder; Detune '
               + 'and Width shape the result. Range 1 to 8 voices.' },
        fr: { t: 'Unisson osc A',
              b: 'Empile jusqu’à huit copies désaccordées de l’oscillateur A sur une même note. La '
               + 'pile est compensée en gain : l’augmenter épaissit le son sans le rendre plus fort. '
               + 'Désacc. et Larg. en façonnent le résultat. Plage de 1 à 8 voix.',
              reviewed: false },
    },
    'tip.oscADetune': {
        en: { t: 'Osc A Detune',
              b: 'Spreads the unison copies apart in pitch, up to 50 cents across the whole stack. '
               + 'It does nothing while Unison is 1. Small values give a chorus; large ones give the '
               + 'supersaw. Range 0 to 100 %.' },
        fr: { t: 'Désaccord osc A',
              b: 'Écarte en hauteur les copies d’unisson, jusqu’à 50 cents sur l’ensemble de la pile. '
               + 'Sans effet tant que Unisson vaut 1. Les petites valeurs donnent un chorus, les '
               + 'grandes la supersaw. Plage de 0 à 100 %.',
              reviewed: false },
    },
    'tip.oscAWidth': {
        en: { t: 'Osc A Width',
              b: 'Spreads the unison copies across the stereo field on an equal-power pan law. At 0 '
               + 'the whole stack sits where Pan puts it; at 100 % the outer voices reach hard left '
               + 'and right. It does nothing while Unison is 1. Range 0 to 100 %.' },
        fr: { t: 'Largeur osc A',
              b: 'Étale les copies d’unisson dans l’image stéréo selon une loi de panoramique à '
               + 'puissance constante. À 0, toute la pile reste où Pano la place ; à 100 %, les voix '
               + 'extrêmes atteignent les bords. Sans effet tant que Unisson vaut 1. Plage de 0 à 100 %.',
              reviewed: false },
    },
    'tip.oscAWarpType': {
        en: { t: 'Osc A Warp Type',
              b: 'Selects how Warp Amt reshapes oscillator A. Sync is hard sync against a master '
               + 'phase, Bend is phase distortion, FM takes its modulator from oscillator B, and '
               + 'Window is Sync with a half-sine window over each cycle. Range: Off, Sync, Bend, '
               + 'FM, Window.' },
        fr: { t: 'Type de déformation osc A',
              b: 'Choisit la manière dont Qté déf. remodèle l’oscillateur A. Sync est une '
               + 'synchronisation dure sur une phase maîtresse, Bend une distorsion de phase, FM '
               + 'prend son modulateur sur l’oscillateur B, et Window reprend Sync avec une '
               + 'fenêtre en demi-sinus sur chaque cycle. Plage : Off, Sync, Bend, FM, Window.',
              reviewed: false },
    },
    'tip.oscAWarpAmt': {
        en: { t: 'Osc A Warp Amount',
              b: 'How far the selected Warp type is pushed — the sync ratio, the phase-distortion '
               + 'exponent or the FM index, depending on which is chosen. It does nothing while '
               + 'Warp is Off. Range 0 to 100 %.' },
        fr: { t: 'Quantité de déformation osc A',
              b: 'Intensité de la déformation choisie : rapport de synchronisation, exposant de '
               + 'distorsion de phase ou indice de FM selon le type retenu. Sans effet tant que '
               + 'Déform. est sur Off. Plage de 0 à 100 %.',
              reviewed: false },
    },

    // ── Oscillator B ────────────────────────────────────────────────────────
    // The same twelve controls and the same formatters. The bodies are not
    // copies of A's: B's FM modulator is A, and B's default Level is 0, which is
    // the first thing a user needs told.

    'tip.oscBTable': {
        en: { t: 'Osc B Wavetable',
              b: 'Chooses the wavetable oscillator B reads, from the same 28 factory tables as '
               + 'oscillator A. Oscillator B starts at Level 0, so raise its Level before expecting '
               + 'to hear this. Range: 28 factory tables plus any you import.' },
        fr: { t: 'Table d’onde osc B',
              b: 'Choisit la table d’onde lue par l’oscillateur B, parmi les mêmes 28 tables d’usine '
               + 'que l’oscillateur A. L’oscillateur B démarre à un niveau de 0 : montez son Niv. '
               + 'avant d’espérer l’entendre. Plage : 28 tables d’usine, plus celles que vous importez.',
              reviewed: false },
    },
    'tip.oscBPos': {
        en: { t: 'Osc B Position',
              b: 'Scans through the frames of oscillator B’s wavetable. Modulating A and B from '
               + 'different LFOs is what keeps a two-oscillator pad from moving as one block. '
               + 'Range 0 to 100 %.' },
        fr: { t: 'Position osc B',
              b: 'Balaie les trames de la table de l’oscillateur B. Moduler A et B depuis deux OBF '
               + 'différents évite qu’une nappe à deux oscillateurs ne bouge d’un seul bloc. '
               + 'Plage de 0 à 100 %.',
              reviewed: false },
    },
    'tip.oscBLevel': {
        en: { t: 'Osc B Level',
              b: 'Output level of oscillator B before the filters. It defaults to 0, so a fresh '
               + 'patch is oscillator A alone until this is raised. Range 0 to 100 %.' },
        fr: { t: 'Niveau osc B',
              b: 'Niveau de sortie de l’oscillateur B avant les filtres. Sa valeur par défaut est 0 : '
               + 'un son neuf n’utilise que l’oscillateur A tant que vous ne montez pas ce niveau. '
               + 'Plage de 0 à 100 %.',
              reviewed: false },
    },
    'tip.oscBPan': {
        en: { t: 'Osc B Pan',
              b: 'Places oscillator B in the stereo field, before the filters and the effects. '
               + 'Range 100L through C to 100R.' },
        fr: { t: 'Panoramique osc B',
              b: 'Place l’oscillateur B dans l’image stéréo, avant les filtres et les effets. '
               + 'Plage de 100L à 100R, C au centre.',
              reviewed: false },
    },
    'tip.oscBCoarse': {
        en: { t: 'Osc B Coarse',
              b: 'Transposes oscillator B in whole semitones, two octaves either way. Offsetting B '
               + 'from A by a fifth or an octave is how most layered patches are built. '
               + 'Range −24 to +24 st.' },
        fr: { t: 'Accord grossier osc B',
              b: 'Transpose l’oscillateur B par demi-tons entiers, sur deux octaves de part et '
               + 'd’autre. Décaler B par rapport à A d’une quinte ou d’une octave est la base de la '
               + 'plupart des sons superposés. Plage de −24 à +24 demi-tons.',
              reviewed: false },
    },
    'tip.oscBFine': {
        en: { t: 'Osc B Fine',
              b: 'Detunes oscillator B in cents, a full semitone either way. Ten or fifteen cents '
               + 'against oscillator A is the classic slow beat. Range −100 to +100 ct.' },
        fr: { t: 'Accord fin osc B',
              b: 'Désaccorde l’oscillateur B en cents, jusqu’à un demi-ton de part et d’autre. Dix ou '
               + 'quinze cents d’écart avec l’oscillateur A donnent le battement lent classique. '
               + 'Plage de −100 à +100 cents.',
              reviewed: false },
    },
    'tip.oscBPhase': {
        en: { t: 'Osc B Phase',
              b: 'Sets the point in the waveform where oscillator B starts on every note-on. '
               + 'Offsetting it from oscillator A’s start phase changes how the two sum at the '
               + 'very front of the note. Range 0 to 100 %.' },
        fr: { t: 'Phase osc B',
              b: 'Fixe le point de la forme d’onde où l’oscillateur B démarre à chaque note. La '
               + 'décaler par rapport à la phase de départ de l’oscillateur A modifie la façon dont '
               + 'les deux s’additionnent au tout début de la note. Plage de 0 à 100 %.',
              reviewed: false },
    },
    'tip.oscBUnison': {
        en: { t: 'Osc B Unison',
              b: 'Stacks up to eight detuned copies of oscillator B on one note, gain-compensated '
               + 'so the sound thickens without getting louder. Range 1 to 8 voices.' },
        fr: { t: 'Unisson osc B',
              b: 'Empile jusqu’à huit copies désaccordées de l’oscillateur B sur une même note, avec '
               + 'compensation de gain : le son s’épaissit sans devenir plus fort. Plage de 1 à 8 voix.',
              reviewed: false },
    },
    'tip.oscBDetune': {
        en: { t: 'Osc B Detune',
              b: 'Spreads oscillator B’s unison copies apart in pitch, up to 50 cents across the '
               + 'stack. It does nothing while Unison is 1. Range 0 to 100 %.' },
        fr: { t: 'Désaccord osc B',
              b: 'Écarte en hauteur les copies d’unisson de l’oscillateur B, jusqu’à 50 cents sur '
               + 'l’ensemble de la pile. Sans effet tant que Unisson vaut 1. Plage de 0 à 100 %.',
              reviewed: false },
    },
    'tip.oscBWidth': {
        en: { t: 'Osc B Width',
              b: 'Spreads oscillator B’s unison copies across the stereo field. Giving A and B '
               + 'different widths keeps the two stacks from occupying exactly the same space. '
               + 'It does nothing while Unison is 1. Range 0 to 100 %.' },
        fr: { t: 'Largeur osc B',
              b: 'Étale les copies d’unisson de l’oscillateur B dans l’image stéréo. Donner à A et B '
               + 'des largeurs différentes évite que les deux piles n’occupent exactement la même '
               + 'place. Sans effet tant que Unisson vaut 1. Plage de 0 à 100 %.',
              reviewed: false },
    },
    'tip.oscBWarpType': {
        en: { t: 'Osc B Warp Type',
              b: 'Selects how Warp Amt reshapes oscillator B. It is the mirror of oscillator A’s: '
               + 'setting both to FM cross-modulates the pair, since B’s modulator is A. '
               + 'Range: Off, Sync, Bend, FM, Window.' },
        fr: { t: 'Type de déformation osc B',
              b: 'Choisit la manière dont Qté déf. remodèle l’oscillateur B. C’est le miroir de celui '
               + 'de l’oscillateur A : régler les deux sur FM crée une modulation croisée, puisque le '
               + 'modulateur de B est A. Plage : Off, Sync, Bend, FM, Window.',
              reviewed: false },
    },
    'tip.oscBWarpAmt': {
        en: { t: 'Osc B Warp Amount',
              b: 'How far oscillator B’s selected Warp type is pushed. It does nothing while Warp '
               + 'is Off. Range 0 to 100 %.' },
        fr: { t: 'Quantité de déformation osc B',
              b: 'Intensité de la déformation choisie pour l’oscillateur B. Sans effet tant que '
               + 'Déform. est sur Off. Plage de 0 à 100 %.',
              reviewed: false },
    },

    // ── Sub oscillator and noise ────────────────────────────────────────────
    // pct :2469 again for the two levels.

    'tip.subShape': {
        en: { t: 'Sub Shape',
              b: 'Waveform of the sub oscillator. Sine adds weight and nothing else; Square and Saw '
               + 'add harmonics the filters can then work on. Range: Sine, Triangle, Saw, Square.' },
        fr: { t: 'Forme du sous-oscillateur',
              b: 'Forme d’onde du sous-oscillateur. Sine n’ajoute que du poids ; Square et Saw '
               + 'ajoutent des harmoniques sur lesquelles les filtres peuvent agir. Plage : Sine, '
               + 'Triangle, Saw, Square.',
              reviewed: false },
    },
    'tip.subOctave': {
        en: { t: 'Sub Octave',
              b: 'How far below the played note the sub oscillator sounds. One octave down is the '
               + 'usual bass reinforcement; three and four octaves down are felt more than heard on '
               + 'most systems. Range: −1, −2, −3 or −4 octaves.' },
        fr: { t: 'Octave du sous-oscillateur',
              b: 'Hauteur du sous-oscillateur sous la note jouée. Une octave en dessous est le '
               + 'renfort de grave habituel ; trois ou quatre octaves plus bas se ressentent plus '
               + 'qu’elles ne s’entendent sur la plupart des systèmes. Plage : −1, −2, −3 ou '
               + '−4 octaves.',
              reviewed: false },
    },
    'tip.subLevel': {
        en: { t: 'Sub Level',
              b: 'Level of the sub oscillator. It defaults to 0, so the sub is silent until this is '
               + 'raised; where it lands in the voice is set by Routing beside it. Range 0 to 100 %.' },
        fr: { t: 'Niveau du sous-oscillateur',
              b: 'Niveau du sous-oscillateur. Sa valeur par défaut est 0 : le sub reste muet tant que '
               + 'vous ne le montez pas. Sa place dans la voix est fixée par Routage, juste à côté. '
               + 'Plage de 0 à 100 %.',
              reviewed: false },
    },
    'tip.noiseType': {
        en: { t: 'Noise Type',
              b: 'Colour of the noise source. White is flat, Pink and Brown tilt progressively '
               + 'towards the low end, Digital is a quantised sample-and-hold, Vinyl is bandpassed '
               + 'noise with crackle, and Wind is brown noise under a slow filter sweep. '
               + 'Range: White, Pink, Brown, Digital, Vinyl, Wind.' },
        fr: { t: 'Type de bruit',
              b: 'Couleur de la source de bruit. White est plat, Pink et Brown penchent de plus en '
               + 'plus vers le grave, Digital est un échantillonneur-bloqueur quantifié, Vinyl un '
               + 'bruit filtré en bande avec craquements, et Wind un bruit brun sous un balayage de '
               + 'filtre lent. Plage : White, Pink, Brown, Digital, Vinyl, Wind.',
              reviewed: false },
    },
    'tip.noiseLevel': {
        en: { t: 'Noise Level',
              b: 'Level of the noise source, which follows the same routing as the sub. It defaults '
               + 'to 0. A trace of noise under a pad gives the filter something to bite on in the '
               + 'top octaves. Range 0 to 100 %.' },
        fr: { t: 'Niveau de bruit',
              b: 'Niveau de la source de bruit, qui suit le même routage que le sub. Sa valeur par '
               + 'défaut est 0. Un soupçon de bruit sous une nappe donne au filtre matière à '
               + 'travailler dans l’aigu. Plage de 0 à 100 %.',
              reviewed: false },
    },
    'tip.subRouting': {
        en: { t: 'Sub Routing',
              b: 'Whether the sub and the noise pass through the filters or bypass them. Post-Filter '
               + 'keeps the low end steady while the filter sweeps; Pre-Filter lets the filter shape '
               + 'the sub with everything else. Range: Post-Filter, Pre-Filter.' },
        fr: { t: 'Routage du sous-oscillateur',
              b: 'Détermine si le sub et le bruit traversent les filtres ou les contournent. '
               + 'Post-Filter garde le grave stable pendant un balayage de filtre ; Pre-Filter laisse '
               + 'le filtre modeler le sub avec le reste. Plage : Post-Filter, Pre-Filter.',
              reviewed: false },
    },

    // ── Amplitude envelope ──────────────────────────────────────────────────
    // envTimeFmtA :2494 (0.001..10 s, skew 0.35) and envTimeFmtR :2498
    // (0.001..20 s, skew 0.3). Both print ms below 1 s and s above it.

    'tip.ampAttack': {
        en: { t: 'Amp Attack',
              b: 'Time the note takes to reach full level after a key is pressed. A millisecond or '
               + 'two is a pluck or a key; a second or more is a pad that fades in. '
               + 'Range 1 ms to 10 s.' },
        fr: { t: 'Attaque d’ampli',
              b: 'Temps que met la note à atteindre son niveau plein après l’enfoncement d’une '
               + 'touche. Une ou deux millisecondes donnent un pincé ou un clavier ; une seconde ou '
               + 'plus, une nappe qui monte en fondu. Plage de 1 ms à 10 s.',
              reviewed: false },
    },
    'tip.ampDecay': {
        en: { t: 'Amp Decay',
              b: 'Time the note takes to fall from its peak to the Sustain level while the key is '
               + 'still held. With Sustain at 0 this alone decides the length of a plucked note. '
               + 'Range 1 ms to 10 s.' },
        fr: { t: 'Déclin d’ampli',
              b: 'Temps que met la note à descendre de son sommet jusqu’au niveau de Tenue, touche '
               + 'toujours enfoncée. Avec une Tenue à 0, c’est cette commande seule qui décide de la '
               + 'longueur d’une note pincée. Plage de 1 ms à 10 s.',
              reviewed: false },
    },
    'tip.ampSustain': {
        en: { t: 'Amp Sustain',
              b: 'Level the note holds at for as long as the key is down, as a fraction of its peak. '
               + 'At 0 the note dies away on its own after Decay; at 100 % Decay has no audible '
               + 'effect. Range 0 to 100 %.' },
        fr: { t: 'Tenue d’ampli',
              b: 'Niveau auquel la note se maintient tant que la touche est enfoncée, en fraction de '
               + 'son sommet. À 0, la note s’éteint d’elle-même après le Déclin ; à 100 %, le Déclin '
               + 'n’a plus d’effet audible. Plage de 0 à 100 %.',
              reviewed: false },
    },
    'tip.ampRelease': {
        en: { t: 'Amp Release',
              b: 'Time the note takes to fade to silence after the key is let go. Long releases '
               + 'overlap notes and use more voices, which is what you want on a pad and not on a '
               + 'bass. Range 1 ms to 20 s.' },
        fr: { t: 'Relâche d’ampli',
              b: 'Temps que met la note à s’éteindre après le relâchement de la touche. Une relâche '
               + 'longue superpose les notes et consomme plus de voix : souhaitable sur une nappe, '
               + 'moins sur une basse. Plage de 1 ms à 20 s.',
              reviewed: false },
    },

    // ── Filter envelope ─────────────────────────────────────────────────────
    // Same two time formatters; pctSigned :2470 for the two depths.

    'tip.filtAttack': {
        en: { t: 'Filter Attack',
              b: 'Time the filter envelope takes to reach its peak. Short values give the click at '
               + 'the front of a plucked or percussive filter sweep. Range 1 ms to 10 s.' },
        fr: { t: 'Attaque de filtre',
              b: 'Temps que met l’enveloppe de filtre à atteindre son sommet. Les valeurs courtes '
               + 'donnent le claquement en tête d’un balayage de filtre pincé ou percussif. '
               + 'Plage de 1 ms à 10 s.',
              reviewed: false },
    },
    'tip.filtDecay': {
        en: { t: 'Filter Decay',
              b: 'Time the filter envelope takes to fall from its peak to its Sustain level. This '
               + 'is the control that sets how fast a filter sweep closes. Range 1 ms to 10 s.' },
        fr: { t: 'Déclin de filtre',
              b: 'Temps que met l’enveloppe de filtre à descendre de son sommet jusqu’à sa Tenue. '
               + 'C’est cette commande qui décide de la vitesse de fermeture d’un balayage. '
               + 'Plage de 1 ms à 10 s.',
              reviewed: false },
    },
    'tip.filtSustain': {
        en: { t: 'Filter Sustain',
              b: 'Level the filter envelope holds at while the key is down. It is not a cutoff — '
               + 'the two Depth knobs decide how much of it reaches each filter. Range 0 to 100 %.' },
        fr: { t: 'Tenue de filtre',
              b: 'Niveau auquel l’enveloppe de filtre se maintient tant que la touche est enfoncée. '
               + 'Ce n’est pas une fréquence de coupure : ce sont les deux boutons Prof. qui décident '
               + 'de la part qui atteint chaque filtre. Plage de 0 à 100 %.',
              reviewed: false },
    },
    'tip.filtRelease': {
        en: { t: 'Filter Release',
              b: 'Time the filter envelope takes to fall away after the key is let go. Set it '
               + 'shorter than the amp release and the tail darkens as it fades. Range 1 ms to 20 s.' },
        fr: { t: 'Relâche de filtre',
              b: 'Temps que met l’enveloppe de filtre à retomber après le relâchement de la touche. '
               + 'Plus courte que la relâche d’ampli, elle assombrit la traîne pendant qu’elle '
               + 's’éteint. Plage de 1 ms à 20 s.',
              reviewed: false },
    },
    'tip.filtAEnvDepth': {
        en: { t: 'Filter A Env Depth',
              b: 'How much of the filter envelope is added to filter A’s cutoff, and in which '
               + 'direction. Negative values close the filter as the envelope rises. It defaults to '
               + '0, so the envelope does nothing until this is turned. Range −100 to +100 %.' },
        fr: { t: 'Profondeur d’enveloppe filtre A',
              b: 'Part de l’enveloppe de filtre ajoutée à la coupure du filtre A, et dans quel sens. '
               + 'Les valeurs négatives ferment le filtre à mesure que l’enveloppe monte. Valeur par '
               + 'défaut 0 : l’enveloppe reste sans effet tant que ce bouton n’est pas tourné. '
               + 'Plage de −100 à +100 %.',
              reviewed: false },
    },
    'tip.filtBEnvDepth': {
        en: { t: 'Filter B Env Depth',
              b: 'How much of the filter envelope is added to filter B’s cutoff, and in which '
               + 'direction. Opposite signs on A and B open one filter while the other closes. '
               + 'Range −100 to +100 %.' },
        fr: { t: 'Profondeur d’enveloppe filtre B',
              b: 'Part de l’enveloppe de filtre ajoutée à la coupure du filtre B, et dans quel sens. '
               + 'Des signes opposés sur A et B ouvrent un filtre pendant que l’autre se ferme. '
               + 'Plage de −100 à +100 %.',
              reviewed: false },
    },

    // ── Filters A and B ─────────────────────────────────────────────────────
    // cutoffFmt :2502 maps 20..20000 Hz on a 0.25 skew and switches to kHz above
    // 1000, which is where "20 Hz to 20 kHz" comes from — there is no label
    // column to inherit it from.

    'tip.filtAType': {
        en: { t: 'Filter A Type',
              b: 'Response of filter A. The 12 and 24 suffixes are the slope in dB per octave: 24 '
               + 'is steeper and more dramatic under a sweep, 12 is gentler and leaves more of the '
               + 'top. Range: LP12, LP24, HP12, HP24, BP12, BP24, Notch.' },
        fr: { t: 'Type de filtre A',
              b: 'Réponse du filtre A. Les suffixes 12 et 24 donnent la pente en dB par octave : 24 '
               + 'est plus raide et plus spectaculaire sous un balayage, 12 plus doux et laisse '
               + 'davantage d’aigu. Plage : LP12, LP24, HP12, HP24, BP12, BP24, Notch.',
              reviewed: false },
    },
    'tip.filtACutoff': {
        en: { t: 'Filter A Cutoff',
              b: 'Corner frequency of filter A. It opens fully by default, so the filter is out of '
               + 'the way until you close it or point the filter envelope at it. '
               + 'Range 20 Hz to 20 kHz.' },
        fr: { t: 'Coupure du filtre A',
              b: 'Fréquence de coupure du filtre A. Elle est grande ouverte par défaut : le filtre '
               + 'reste transparent tant que vous ne le fermez pas ou que l’enveloppe de filtre ne le '
               + 'vise pas. Plage de 20 Hz à 20 kHz.',
              reviewed: false },
    },
    'tip.filtARes': {
        en: { t: 'Filter A Resonance',
              b: 'Emphasis at the cutoff frequency of filter A. A little sharpens a sweep; a lot '
               + 'turns the filter into a pitched whistle that tracks the cutoff. Range 0 to 100 %.' },
        fr: { t: 'Résonance du filtre A',
              b: 'Accentuation à la fréquence de coupure du filtre A. Un peu affûte un balayage ; '
               + 'beaucoup transforme le filtre en sifflement chantant qui suit la coupure. '
               + 'Plage de 0 à 100 %.',
              reviewed: false },
    },
    'tip.filtADrive': {
        en: { t: 'Filter A Drive',
              b: 'Saturation inside filter A, applied where the signal enters it. It thickens the '
               + 'sound and tames a high resonance at the same time. Range 0 to 100 %.' },
        fr: { t: 'Saturation du filtre A',
              b: 'Saturation à l’intérieur du filtre A, appliquée à l’entrée du signal. Elle épaissit '
               + 'le son et dompte en même temps une résonance élevée. Plage de 0 à 100 %.',
              reviewed: false },
    },
    'tip.filtAKeyTrack': {
        en: { t: 'Filter A KeyTrack',
              b: 'How far filter A’s cutoff follows the played note, pivoting around middle C. At '
               + '100 % the cutoff doubles with each octave up, so the timbre stays even across the '
               + 'keyboard instead of dulling in the top. Range 0 to 100 %.' },
        fr: { t: 'Suivi de clavier du filtre A',
              b: 'Mesure dans laquelle la coupure du filtre A suit la note jouée, en pivotant autour '
               + 'du do central. À 100 %, la coupure double à chaque octave montée : le timbre reste '
               + 'régulier sur tout le clavier au lieu de s’assourdir dans l’aigu. Plage de 0 à 100 %.',
              reviewed: false },
    },
    'tip.filtBType': {
        en: { t: 'Filter B Type',
              b: 'Response of filter B, from the same seven shapes as filter A. A low-pass A into a '
               + 'high-pass B in Serial is the usual way to build a band. '
               + 'Range: LP12, LP24, HP12, HP24, BP12, BP24, Notch.' },
        fr: { t: 'Type de filtre B',
              b: 'Réponse du filtre B, parmi les mêmes sept formes que le filtre A. Un passe-bas en A '
               + 'suivi d’un passe-haut en B, en Serial, est la manière habituelle de construire une '
               + 'bande. Plage : LP12, LP24, HP12, HP24, BP12, BP24, Notch.',
              reviewed: false },
    },
    'tip.filtBCutoff': {
        en: { t: 'Filter B Cutoff',
              b: 'Corner frequency of filter B, which also opens fully by default. Where it sits '
               + 'relative to filter A is what Filter Routing turns into a band or a pair of peaks. '
               + 'Range 20 Hz to 20 kHz.' },
        fr: { t: 'Coupure du filtre B',
              b: 'Fréquence de coupure du filtre B, elle aussi grande ouverte par défaut. Sa position '
               + 'par rapport au filtre A est ce que Routage filt. transforme en bande ou en paire de '
               + 'pointes. Plage de 20 Hz à 20 kHz.',
              reviewed: false },
    },
    'tip.filtBRes': {
        en: { t: 'Filter B Resonance',
              b: 'Emphasis at the cutoff frequency of filter B. Two resonant peaks at different '
               + 'frequencies in Parallel give a vowel-like colour. Range 0 to 100 %.' },
        fr: { t: 'Résonance du filtre B',
              b: 'Accentuation à la fréquence de coupure du filtre B. Deux pointes résonantes à des '
               + 'fréquences différentes, en Parallel, donnent une couleur vocalique. '
               + 'Plage de 0 à 100 %.',
              reviewed: false },
    },
    'tip.filtBDrive': {
        en: { t: 'Filter B Drive',
              b: 'Saturation inside filter B. In Serial it lands on a signal filter A has already '
               + 'shaped, so it colours differently from filter A’s Drive at the same setting. '
               + 'Range 0 to 100 %.' },
        fr: { t: 'Saturation du filtre B',
              b: 'Saturation à l’intérieur du filtre B. En Serial, elle s’applique à un signal déjà '
               + 'façonné par le filtre A : elle colore donc autrement que la Satur. du filtre A au '
               + 'même réglage. Plage de 0 à 100 %.',
              reviewed: false },
    },
    'tip.filtBKeyTrack': {
        en: { t: 'Filter B KeyTrack',
              b: 'How far filter B’s cutoff follows the played note, pivoting around middle C. '
               + 'Range 0 to 100 %.' },
        fr: { t: 'Suivi de clavier du filtre B',
              b: 'Mesure dans laquelle la coupure du filtre B suit la note jouée, en pivotant autour '
               + 'du do central. Plage de 0 à 100 %.',
              reviewed: false },
    },
    'tip.filtRouting': {
        en: { t: 'Filter Routing',
              b: 'How the two filters are wired. Serial sends the voice through A and then B, which '
               + 'is how you build a band-pass out of two slopes; Parallel runs them side by side '
               + 'and sums the results at half gain. Range: Serial, Parallel.' },
        fr: { t: 'Routage des filtres',
              b: 'Câblage des deux filtres. Serial fait passer la voix par A puis par B, ce qui '
               + 'permet de construire un passe-bande à partir de deux pentes ; Parallel les fait '
               + 'travailler côte à côte et somme les résultats à mi-gain. Plage : Serial, Parallel.',
              reviewed: false },
    },

    // ── Tuning tab ──────────────────────────────────────────────────────────
    // masterTuneFmt :2539 (420..460 Hz) drives the bespoke A4 knob's readout at
    // index.html:4038, NOT the dead bindKnob('masterTune') call at :2721 — that
    // one looks for #knob-masterTune, which does not exist, and returns early.
    // The Stretch readout is `v.toFixed(2)` at :4110 over 0.95..1.25, so its two
    // decimals come from there and not from octStretchFmt's three.
    // pbRangeFmt :2547, glideTimeFmt :2551.

    'tip.masterTune': {
        en: { t: 'Master Tune',
              b: 'Reference pitch for A4, and therefore for the whole instrument. 440 Hz is the '
               + 'modern standard; 415 is baroque territory and 432 a common alternative. '
               + 'Range 420.0 to 460.0 Hz.' },
        fr: { t: 'Diapason',
              b: 'Hauteur de référence du la3, et donc de tout l’instrument. 440 Hz est la norme '
               + 'moderne ; 415 relève du répertoire baroque et 432 est une variante répandue. '
               + 'Plage de 420,0 à 460,0 Hz.',
              reviewed: false },
    },
    'tip.octaveStretch': {
        en: { t: 'Octave Stretch',
              b: 'Widens or narrows the octave itself. 1.00 is a pure 2:1; above it the octaves '
               + 'stretch the way a piano’s do, below it they shrink. Values away from 1.00 change '
               + 'every interval in the scale. Range 0.95 to 1.25.' },
        fr: { t: 'Étirement d’octave',
              b: 'Élargit ou resserre l’octave elle-même. 1,00 correspond à un rapport 2:1 pur ; '
               + 'au-dessus, les octaves s’étirent comme sur un piano, en dessous elles se '
               + 'resserrent. S’écarter de 1,00 modifie tous les intervalles de la gamme. '
               + 'Plage de 0,95 à 1,25.',
              reviewed: false },
    },
    'tip.pitchBendRange': {
        en: { t: 'Pitch Bend Range',
              b: 'How far the pitch wheel bends, in semitones, in both directions. Two semitones is '
               + 'the usual default; twelve turns the wheel into an octave lever. Range 1 to 48 st.' },
        fr: { t: 'Amplitude de pitch bend',
              b: 'Amplitude de la molette de hauteur, en demi-tons, dans les deux sens. Deux '
               + 'demi-tons est la valeur habituelle ; douze font de la molette un levier d’octave. '
               + 'Plage de 1 à 48 demi-tons.',
              reviewed: false },
    },
    'tip.glideMode': {
        en: { t: 'Glide Mode',
              b: 'When one note slides into the next. Off never glides, Legato glides only between '
               + 'overlapping notes, and Always glides on every note. Glide sets how long the slide '
               + 'takes. Range: Off, Legato, Always.' },
        fr: { t: 'Mode de glissando',
              b: 'Détermine quand une note glisse vers la suivante. Off ne glisse jamais, Legato '
               + 'glisse seulement entre notes qui se chevauchent, Always glisse à chaque note. '
               + 'Glissé en fixe la durée. Plage : Off, Legato, Always.',
              reviewed: false },
    },
    'tip.glideTime': {
        en: { t: 'Glide Time',
              b: 'How long a note takes to slide to the next pitch. It does nothing while Glide Mode '
               + 'is Off. Range 1 ms to 5 s.' },
        fr: { t: 'Durée de glissando',
              b: 'Temps que met une note à glisser vers la hauteur suivante. Sans effet tant que Mode '
               + 'glissé est sur Off. Plage de 1 ms à 5 s.',
              reviewed: false },
    },

    // ── Effects: Reverb ─────────────────────────────────────────────────────
    // predelayFmt :2518 (0..200 ms, skew 0.5), reverbModRateFmt :2563
    // (0.1..5 Hz, skew 0.5), pct :2469 for the rest.
    //
    // THE FIVE BYPASS BUTTONS READ THE INVERSE OF THEIR PARAMETER, and the tip
    // says so rather than pretending otherwise. The face is ON while the effect
    // is RUNNING (bindBypassToggle, index.html:3213), but the parameter is named
    // <fx>Bypass and its host text is Off/On, so the automation lane's On means
    // the effect is OFF. A tip that hid that would be a tip that lies.

    'tip.reverbBypass': {
        en: { t: 'Reverb Bypass',
              b: 'Switches the reverb in and out. The button reads ON while the reverb is running '
               + 'and OFF while it is bypassed — note that the automation lane calls this parameter '
               + 'Bypass, so its On is this button’s OFF. Range: ON, OFF.' },
        fr: { t: 'Contournement de la réverbe',
              b: 'Active ou contourne la réverbération. Le bouton affiche MARCHE tant que la réverbe '
               + 'fonctionne et ARRÊT quand elle est contournée — attention, la ligne d’automation '
               + 'nomme ce paramètre Bypass : son On correspond à l’ARRÊT du bouton. '
               + 'Plage : MARCHE, ARRÊT.',
              reviewed: false },
    },
    'tip.reverbSize': {
        en: { t: 'Reverb Size',
              b: 'Decay length of the plate. Small values give a room, large ones a hall that runs '
               + 'on for several seconds after the note. Range 0 to 100 %.' },
        fr: { t: 'Taille de la réverbe',
              b: 'Longueur du déclin de la plaque. Les petites valeurs donnent une pièce, les grandes '
               + 'une salle qui se prolonge plusieurs secondes après la note. Plage de 0 à 100 %.',
              reviewed: false },
    },
    'tip.reverbDamp': {
        en: { t: 'Reverb Damping',
              b: 'How fast the high frequencies die away inside the tail. Raise it for a soft, '
               + 'wooden room; lower it for a bright plate that keeps its top end to the end. '
               + 'Range 0 to 100 %.' },
        fr: { t: 'Amortissement de la réverbe',
              b: 'Vitesse à laquelle les aigus s’éteignent dans la traîne. Montez-le pour une pièce '
               + 'douce et boisée, baissez-le pour une plaque brillante qui garde son aigu jusqu’au '
               + 'bout. Plage de 0 à 100 %.',
              reviewed: false },
    },
    'tip.reverbPredelay': {
        en: { t: 'Reverb Pre-delay',
              b: 'Silence between the dry note and the first of the reverb. Twenty to forty '
               + 'milliseconds keeps a lead readable in front of a long tail. Range 0 to 200 ms.' },
        fr: { t: 'Pré-délai de la réverbe',
              b: 'Silence entre la note directe et le début de la réverbération. Vingt à quarante '
               + 'millisecondes gardent un lead lisible devant une traîne longue. '
               + 'Plage de 0 à 200 ms.',
              reviewed: false },
    },
    'tip.reverbMix': {
        en: { t: 'Reverb Mix',
              b: 'Balance between the dry signal and the reverb. It defaults to 0, so the reverb is '
               + 'inaudible until this is raised even with the section switched ON. '
               + 'Range 0 to 100 %.' },
        fr: { t: 'Dosage de la réverbe',
              b: 'Équilibre entre le signal direct et la réverbération. Sa valeur par défaut est 0 : '
               + 'la réverbe reste inaudible tant que vous ne la montez pas, même section sur MARCHE. '
               + 'Plage de 0 à 100 %.',
              reviewed: false },
    },
    'tip.reverbModDepth': {
        en: { t: 'Reverb Mod Depth',
              b: 'How far the plate’s internal delay lines are modulated. A little breaks up the '
               + 'metallic ring a static plate can develop; a lot detunes the tail audibly. '
               + 'Range 0 to 100 %.' },
        fr: { t: 'Profondeur de modulation de la réverbe',
              b: 'Amplitude de modulation des lignes à retard internes de la plaque. Un peu casse la '
               + 'sonnerie métallique qu’une plaque statique peut développer ; beaucoup désaccorde la '
               + 'traîne de façon audible. Plage de 0 à 100 %.',
              reviewed: false },
    },
    'tip.reverbModRate': {
        en: { t: 'Reverb Mod Rate',
              b: 'Speed of that internal modulation. Slow rates read as drift, fast ones as a '
               + 'vibrato in the tail. It does nothing while Mod is 0. Range 0.1 to 5.0 Hz.' },
        fr: { t: 'Vitesse de modulation de la réverbe',
              b: 'Vitesse de cette modulation interne. Les vitesses lentes se perçoivent comme une '
               + 'dérive, les rapides comme un vibrato dans la traîne. Sans effet tant que Mod. vaut '
               + '0. Plage de 0,1 à 5,0 Hz.',
              reviewed: false },
    },

    // ── Effects: Delay ──────────────────────────────────────────────────────
    // delayTimeFmt :2510 (0.001..2 s, skew 0.35), feedbackFmt :2555 (n * 95, so
    // the readout tops out at 95 % and NOT at 100 %).

    'tip.delayBypass': {
        en: { t: 'Delay Bypass',
              b: 'Switches the delay in and out. The button reads ON while the delay is running and '
               + 'OFF while it is bypassed; the automation lane’s Bypass On is this button’s OFF. '
               + 'Range: ON, OFF.' },
        fr: { t: 'Contournement du délai',
              b: 'Active ou contourne le délai. Le bouton affiche MARCHE tant que le délai fonctionne '
               + 'et ARRÊT quand il est contourné ; le On du paramètre Bypass correspond à l’ARRÊT du '
               + 'bouton. Plage : MARCHE, ARRÊT.',
              reviewed: false },
    },
    'tip.delayTime': {
        en: { t: 'Delay Time',
              b: 'Spacing between repeats when Sync is off. Short times build a slapback or a comb; '
               + 'long ones leave audible gaps. Turning Sync on hands this job to Division. '
               + 'Range 1 ms to 2 s.' },
        fr: { t: 'Temps de délai',
              b: 'Écart entre les répétitions lorsque Sync est désactivé. Les temps courts donnent un '
               + 'slapback ou un peigne, les longs laissent des trous audibles. Activer Sync confie '
               + 'ce rôle à Division. Plage de 1 ms à 2 s.',
              reviewed: false },
    },
    'tip.delayFeedback': {
        en: { t: 'Delay Feedback',
              b: 'How much of each repeat is fed back for the next one. The readout tops out at '
               + '95 %, which is deliberate: the line stays short of self-oscillation however far '
               + 'you turn it. Range 0 to 95 %.' },
        fr: { t: 'Réinjection du délai',
              b: 'Part de chaque répétition réinjectée pour produire la suivante. Le relevé plafonne '
               + 'à 95 %, et c’est délibéré : la ligne reste en deçà de l’auto-oscillation quelle que '
               + 'soit la position du bouton. Plage de 0 à 95 %.',
              reviewed: false },
    },
    'tip.delaySync': {
        en: { t: 'Delay Sync',
              b: 'Locks the delay to the host tempo. With it on, Division sets the spacing and the '
               + 'Time knob is ignored; with it off, Time is in charge. Range: Off, On.' },
        fr: { t: 'Synchronisation du délai',
              b: 'Verrouille le délai sur le tempo de l’hôte. Activé, c’est Division qui fixe l’écart '
               + 'et le bouton Temps est ignoré ; désactivé, c’est Temps qui commande. '
               + 'Plage : Off, On.',
              reviewed: false },
    },
    'tip.delayDivision': {
        en: { t: 'Delay Division',
              b: 'Note value the repeats land on while Sync is on. The plain divisions are straight, '
               + 'a D suffix is dotted and a T suffix is a triplet. It does nothing while Sync is '
               + 'off. Range: 1/1 down to 1/32, each also dotted and triplet.' },
        fr: { t: 'Division du délai',
              b: 'Valeur de note sur laquelle tombent les répétitions quand Sync est actif. Les '
               + 'divisions simples sont binaires, le suffixe D marque le pointé et le suffixe T le '
               + 'triolet. Sans effet quand Sync est désactivé. Plage : de 1/1 à 1/32, chacune '
               + 'également pointée et en triolet.',
              reviewed: false },
    },
    'tip.delayMode': {
        en: { t: 'Delay Mode',
              b: 'How the two channels feed each other. Normal keeps left and right independent; '
               + 'PingPong cross-feeds them so the repeats bounce from side to side. '
               + 'Range: Normal, PingPong.' },
        fr: { t: 'Mode de délai',
              b: 'Manière dont les deux canaux s’alimentent. Normal garde la gauche et la droite '
               + 'indépendantes ; PingPong les croise, si bien que les répétitions rebondissent d’un '
               + 'côté à l’autre. Plage : Normal, PingPong.',
              reviewed: false },
    },
    'tip.delayMix': {
        en: { t: 'Delay Mix',
              b: 'Balance between the dry signal and the repeats. It defaults to 0, so the delay is '
               + 'inaudible until this is raised. Range 0 to 100 %.' },
        fr: { t: 'Dosage du délai',
              b: 'Équilibre entre le signal direct et les répétitions. Sa valeur par défaut est 0 : '
               + 'le délai reste inaudible tant que vous ne le montez pas. Plage de 0 à 100 %.',
              reviewed: false },
    },

    // ── Effects: Chorus ─────────────────────────────────────────────────────
    // chorusRateFmt :2514 (0.1..10 Hz, skew 0.4).

    'tip.chorusBypass': {
        en: { t: 'Chorus Bypass',
              b: 'Switches the chorus in and out. The button reads ON while the chorus is running '
               + 'and OFF while it is bypassed; the automation lane’s Bypass On is this button’s '
               + 'OFF. Range: ON, OFF.' },
        fr: { t: 'Contournement du chorus',
              b: 'Active ou contourne le chorus. Le bouton affiche MARCHE tant que le chorus '
               + 'fonctionne et ARRÊT quand il est contourné ; le On du paramètre Bypass correspond à '
               + 'l’ARRÊT du bouton. Plage : MARCHE, ARRÊT.',
              reviewed: false },
    },
    'tip.chorusRate': {
        en: { t: 'Chorus Rate',
              b: 'Speed of the chorus modulation. Under 1 Hz it reads as slow drift; above a few '
               + 'hertz it becomes a vibrato. Range 0.1 to 10.0 Hz.' },
        fr: { t: 'Vitesse du chorus',
              b: 'Vitesse de la modulation du chorus. En dessous de 1 Hz, elle se perçoit comme une '
               + 'dérive lente ; au-delà de quelques hertz, elle devient un vibrato. '
               + 'Plage de 0,1 à 10,0 Hz.',
              reviewed: false },
    },
    'tip.chorusDepth': {
        en: { t: 'Chorus Depth',
              b: 'How far the modulation swings the delay time, which is what the ear hears as '
               + 'detuning. Small depths widen; large ones warble. Range 0 to 100 %.' },
        fr: { t: 'Profondeur du chorus',
              b: 'Amplitude avec laquelle la modulation fait varier le temps de retard — ce que '
               + 'l’oreille perçoit comme un désaccord. Les petites profondeurs élargissent, les '
               + 'grandes font chevroter. Plage de 0 à 100 %.',
              reviewed: false },
    },
    'tip.chorusMix': {
        en: { t: 'Chorus Mix',
              b: 'Balance between the dry signal and the chorused one. It defaults to 0, so the '
               + 'chorus is inaudible until this is raised. Range 0 to 100 %.' },
        fr: { t: 'Dosage du chorus',
              b: 'Équilibre entre le signal direct et le signal traité. Sa valeur par défaut est 0 : '
               + 'le chorus reste inaudible tant que vous ne le montez pas. Plage de 0 à 100 %.',
              reviewed: false },
    },

    // ── Effects: Distortion ─────────────────────────────────────────────────

    'tip.distBypass': {
        en: { t: 'Distortion Bypass',
              b: 'Switches the distortion in and out. The button reads ON while it is running and '
               + 'OFF while it is bypassed; the automation lane’s Bypass On is this button’s OFF. '
               + 'Range: ON, OFF.' },
        fr: { t: 'Contournement de la distorsion',
              b: 'Active ou contourne la distorsion. Le bouton affiche MARCHE tant qu’elle fonctionne '
               + 'et ARRÊT quand elle est contournée ; le On du paramètre Bypass correspond à l’ARRÊT '
               + 'du bouton. Plage : MARCHE, ARRÊT.',
              reviewed: false },
    },
    'tip.distType': {
        en: { t: 'Distortion Type',
              b: 'Shape of the saturation curve. SoftClip rounds the peaks, HardClip squares them '
               + 'off, Tube clips the two halves of the wave differently for an asymmetric colour, '
               + 'and Fold turns the signal back on itself for a ring-modulated edge. '
               + 'Range: SoftClip, HardClip, Tube, Fold.' },
        fr: { t: 'Type de distorsion',
              b: 'Forme de la courbe de saturation. SoftClip arrondit les crêtes, HardClip les coupe '
               + 'net, Tube écrête différemment les deux alternances pour une couleur asymétrique, et '
               + 'Fold replie le signal sur lui-même pour un grain proche de la modulation en anneau. '
               + 'Plage : SoftClip, HardClip, Tube, Fold.',
              reviewed: false },
    },
    'tip.distDrive': {
        en: { t: 'Distortion Drive',
              b: 'How hard the signal is pushed into the selected curve. The stage is oversampled, '
               + 'so hard settings add harmonics rather than aliasing. Range 0 to 100 %.' },
        fr: { t: 'Saturation de la distorsion',
              b: 'Force avec laquelle le signal est poussé dans la courbe choisie. L’étage est '
               + 'suréchantillonné : les réglages extrêmes ajoutent des harmoniques plutôt que du '
               + 'repliement. Plage de 0 à 100 %.',
              reviewed: false },
    },
    'tip.distMix': {
        en: { t: 'Distortion Mix',
              b: 'Balance between the clean signal and the distorted one, so heavy drive can be '
               + 'blended under an intact original. It defaults to 0. Range 0 to 100 %.' },
        fr: { t: 'Dosage de la distorsion',
              b: 'Équilibre entre le signal propre et le signal distordu : une saturation lourde peut '
               + 'ainsi être mêlée sous un original intact. Sa valeur par défaut est 0. '
               + 'Plage de 0 à 100 %.',
              reviewed: false },
    },

    // ── Effects: 3-band EQ ──────────────────────────────────────────────────
    // dbFmt :2522 (−12..+12 dB), midFreqFmt :2506 (200..8000 Hz, skew 0.35).

    'tip.eqBypass': {
        en: { t: 'EQ Bypass',
              b: 'Switches the three-band EQ in and out. The button reads ON while it is running and '
               + 'OFF while it is bypassed; the automation lane’s Bypass On is this button’s OFF. '
               + 'Range: ON, OFF.' },
        fr: { t: 'Contournement de l’égaliseur',
              b: 'Active ou contourne l’égaliseur trois bandes. Le bouton affiche MARCHE tant qu’il '
               + 'fonctionne et ARRÊT quand il est contourné ; le On du paramètre Bypass correspond à '
               + 'l’ARRÊT du bouton. Plage : MARCHE, ARRÊT.',
              reviewed: false },
    },
    'tip.eqLowGain': {
        en: { t: 'EQ Low Gain',
              b: 'Cut or boost of the low shelf, at the very end of the chain. Trimming here is '
               + 'usually cleaner than reaching back for the sub level. Range −12.0 to +12.0 dB.' },
        fr: { t: 'Gain grave de l’égaliseur',
              b: 'Atténuation ou accentuation du plateau grave, tout en fin de chaîne. Corriger ici '
               + 'est en général plus propre que de revenir sur le niveau du sub. '
               + 'Plage de −12,0 à +12,0 dB.',
              reviewed: false },
    },
    'tip.eqMidGain': {
        en: { t: 'EQ Mid Gain',
              b: 'Cut or boost of the sweepable mid band. A few decibels out of the mids is what '
               + 'makes room for a vocal over a wide pad. Range −12.0 to +12.0 dB.' },
        fr: { t: 'Gain médium de l’égaliseur',
              b: 'Atténuation ou accentuation de la bande médium balayable. Quelques décibels retirés '
               + 'dans le médium suffisent à laisser la place à une voix devant une nappe large. '
               + 'Plage de −12,0 à +12,0 dB.',
              reviewed: false },
    },
    'tip.eqMidFreq': {
        en: { t: 'EQ Mid Freq',
              b: 'Centre frequency of the mid band. Sweep it with Mid boosted to find the resonance '
               + 'you want gone, then cut there. Range 200 Hz to 8 kHz.' },
        fr: { t: 'Fréquence médium de l’égaliseur',
              b: 'Fréquence centrale de la bande médium. Balayez-la avec le Méd. accentué pour '
               + 'repérer la résonance à supprimer, puis creusez à cet endroit. '
               + 'Plage de 200 Hz à 8 kHz.',
              reviewed: false },
    },
    'tip.eqHighGain': {
        en: { t: 'EQ High Gain',
              b: 'Cut or boost of the high shelf. A gentle cut here tames the top of a bright '
               + 'wavetable without closing the filters. Range −12.0 to +12.0 dB.' },
        fr: { t: 'Gain aigu de l’égaliseur',
              b: 'Atténuation ou accentuation du plateau aigu. Une légère atténuation ici dompte le '
               + 'haut d’une table brillante sans avoir à fermer les filtres. '
               + 'Plage de −12,0 à +12,0 dB.',
              reviewed: false },
    },

    // ── LFO 1 to 4 ──────────────────────────────────────────────────────────
    // lfoRateFmt :2559 maps 0.01..20 Hz on a 0.35 skew and prints one decimal
    // below 10 Hz, so the BOTTOM of the range renders as `0.0 Hz` — a readout
    // quirk the bodies state rather than round away.
    //
    // The four LFOs are structurally identical: nothing distinguishes LFO 3 from
    // LFO 1 but the mod-matrix slot it can be routed from. The bodies therefore
    // differ only in the number, which is honest — inventing a different purpose
    // for each would be prose describing a synth this is not.
    //
    // The Sync and Free Run button FACES are localized (ui.sync / ui.free /
    // ui.freeRun / ui.retrig above), so the French bodies name the French faces.
    // The Division dropdown is `display: none` until Sync is on, and the Rate
    // knob hides in its place — the bodies say so, because a control the reader
    // cannot currently see is the one thing a tooltip has to explain.

    'tip.lfo1Rate': {
        en: { t: 'LFO 1 Rate',
              b: 'Speed of LFO 1 while Sync is off. An LFO reaches nothing on its own — route it to '
               + 'a destination in the Mod tab first. Range 0.01 to 20 Hz, though the one-decimal '
               + 'readout shows the bottom of that range as 0.0 Hz.' },
        fr: { t: 'Vitesse de l’OBF 1',
              b: 'Vitesse de l’OBF 1 lorsque la synchronisation est désactivée. Un OBF n’agit sur '
               + 'rien de lui-même : routez-le d’abord vers une destination dans l’onglet Modul. '
               + 'Plage de 0,01 à 20 Hz, le relevé à une décimale affichant 0,0 Hz au bas de cette '
               + 'plage.',
              reviewed: false },
    },
    'tip.lfo1Shape': {
        en: { t: 'LFO 1 Shape',
              b: 'Waveform of LFO 1. S&H holds a new random value for each cycle instead of sweeping '
               + 'through one. Range: Sine, Triangle, Saw, Square, S&H.' },
        fr: { t: 'Forme de l’OBF 1',
              b: 'Forme d’onde de l’OBF 1. S&H maintient une nouvelle valeur aléatoire à chaque cycle '
               + 'au lieu de balayer. Plage : Sine, Triangle, Saw, Square, S&H.',
              reviewed: false },
    },
    'tip.lfo1Sync': {
        en: { t: 'LFO 1 Sync',
              b: 'Locks LFO 1 to the host tempo. The button reads Free while the LFO runs at its own '
               + 'Rate and Sync while it follows a note division; switching also swaps which of the '
               + 'two controls beside it is shown. Range: Free, Sync.' },
        fr: { t: 'Synchronisation de l’OBF 1',
              b: 'Verrouille l’OBF 1 sur le tempo de l’hôte. Le bouton affiche Libre tant que l’OBF '
               + 'tourne à sa propre Vit. et Synchro lorsqu’il suit une division ; le basculement '
               + 'échange aussi les deux commandes voisines. Plage : Libre, Synchro.',
              reviewed: false },
    },
    'tip.lfo1Division': {
        en: { t: 'LFO 1 Division',
              b: 'Note value LFO 1 cycles on while Sync is on. A D suffix is dotted and a T suffix is '
               + 'a triplet. This control is hidden, and the Rate knob shown in its place, whenever '
               + 'Sync is off. Range: 1/1 down to 1/32, each also dotted and triplet.' },
        fr: { t: 'Division de l’OBF 1',
              b: 'Valeur de note sur laquelle l’OBF 1 boucle quand la synchronisation est active. Le '
               + 'suffixe D marque le pointé, le suffixe T le triolet. Cette commande est masquée, et '
               + 'le bouton Vit. affiché à sa place, dès que la synchronisation est coupée. '
               + 'Plage : de 1/1 à 1/32, chacune également pointée et en triolet.',
              reviewed: false },
    },
    'tip.lfo1FreeRun': {
        en: { t: 'LFO 1 Free Run',
              b: 'Decides whether LFO 1’s phase restarts on each note. Retrig gives every note the '
               + 'same sweep from the same point; Free Run keeps one phase running across notes, so '
               + 'a held chord moves together rather than in scattered phases. '
               + 'Range: Retrig, Free Run.' },
        fr: { t: 'Défilement libre de l’OBF 1',
              b: 'Détermine si la phase de l’OBF 1 repart à chaque note. Redécl. donne à chaque note '
               + 'le même balayage depuis le même point ; Continu conserve une phase unique d’une '
               + 'note à l’autre, si bien qu’un accord tenu évolue d’un seul mouvement. '
               + 'Plage : Redécl., Continu.',
              reviewed: false },
    },

    'tip.lfo2Rate': {
        en: { t: 'LFO 2 Rate',
              b: 'Speed of LFO 2 while Sync is off. An LFO reaches nothing on its own — route it to '
               + 'a destination in the Mod tab first. Range 0.01 to 20 Hz, though the one-decimal '
               + 'readout shows the bottom of that range as 0.0 Hz.' },
        fr: { t: 'Vitesse de l’OBF 2',
              b: 'Vitesse de l’OBF 2 lorsque la synchronisation est désactivée. Un OBF n’agit sur '
               + 'rien de lui-même : routez-le d’abord vers une destination dans l’onglet Modul. '
               + 'Plage de 0,01 à 20 Hz, le relevé à une décimale affichant 0,0 Hz au bas de cette '
               + 'plage.',
              reviewed: false },
    },
    'tip.lfo2Shape': {
        en: { t: 'LFO 2 Shape',
              b: 'Waveform of LFO 2. S&H holds a new random value for each cycle instead of sweeping '
               + 'through one. Range: Sine, Triangle, Saw, Square, S&H.' },
        fr: { t: 'Forme de l’OBF 2',
              b: 'Forme d’onde de l’OBF 2. S&H maintient une nouvelle valeur aléatoire à chaque cycle '
               + 'au lieu de balayer. Plage : Sine, Triangle, Saw, Square, S&H.',
              reviewed: false },
    },
    'tip.lfo2Sync': {
        en: { t: 'LFO 2 Sync',
              b: 'Locks LFO 2 to the host tempo. The button reads Free while the LFO runs at its own '
               + 'Rate and Sync while it follows a note division; switching also swaps which of the '
               + 'two controls beside it is shown. Range: Free, Sync.' },
        fr: { t: 'Synchronisation de l’OBF 2',
              b: 'Verrouille l’OBF 2 sur le tempo de l’hôte. Le bouton affiche Libre tant que l’OBF '
               + 'tourne à sa propre Vit. et Synchro lorsqu’il suit une division ; le basculement '
               + 'échange aussi les deux commandes voisines. Plage : Libre, Synchro.',
              reviewed: false },
    },
    'tip.lfo2Division': {
        en: { t: 'LFO 2 Division',
              b: 'Note value LFO 2 cycles on while Sync is on. A D suffix is dotted and a T suffix is '
               + 'a triplet. This control is hidden, and the Rate knob shown in its place, whenever '
               + 'Sync is off. Range: 1/1 down to 1/32, each also dotted and triplet.' },
        fr: { t: 'Division de l’OBF 2',
              b: 'Valeur de note sur laquelle l’OBF 2 boucle quand la synchronisation est active. Le '
               + 'suffixe D marque le pointé, le suffixe T le triolet. Cette commande est masquée, et '
               + 'le bouton Vit. affiché à sa place, dès que la synchronisation est coupée. '
               + 'Plage : de 1/1 à 1/32, chacune également pointée et en triolet.',
              reviewed: false },
    },
    'tip.lfo2FreeRun': {
        en: { t: 'LFO 2 Free Run',
              b: 'Decides whether LFO 2’s phase restarts on each note. Retrig gives every note the '
               + 'same sweep from the same point; Free Run keeps one phase running across notes, so '
               + 'a held chord moves together rather than in scattered phases. '
               + 'Range: Retrig, Free Run.' },
        fr: { t: 'Défilement libre de l’OBF 2',
              b: 'Détermine si la phase de l’OBF 2 repart à chaque note. Redécl. donne à chaque note '
               + 'le même balayage depuis le même point ; Continu conserve une phase unique d’une '
               + 'note à l’autre, si bien qu’un accord tenu évolue d’un seul mouvement. '
               + 'Plage : Redécl., Continu.',
              reviewed: false },
    },

    'tip.lfo3Rate': {
        en: { t: 'LFO 3 Rate',
              b: 'Speed of LFO 3 while Sync is off. An LFO reaches nothing on its own — route it to '
               + 'a destination in the Mod tab first. Range 0.01 to 20 Hz, though the one-decimal '
               + 'readout shows the bottom of that range as 0.0 Hz.' },
        fr: { t: 'Vitesse de l’OBF 3',
              b: 'Vitesse de l’OBF 3 lorsque la synchronisation est désactivée. Un OBF n’agit sur '
               + 'rien de lui-même : routez-le d’abord vers une destination dans l’onglet Modul. '
               + 'Plage de 0,01 à 20 Hz, le relevé à une décimale affichant 0,0 Hz au bas de cette '
               + 'plage.',
              reviewed: false },
    },
    'tip.lfo3Shape': {
        en: { t: 'LFO 3 Shape',
              b: 'Waveform of LFO 3. S&H holds a new random value for each cycle instead of sweeping '
               + 'through one. Range: Sine, Triangle, Saw, Square, S&H.' },
        fr: { t: 'Forme de l’OBF 3',
              b: 'Forme d’onde de l’OBF 3. S&H maintient une nouvelle valeur aléatoire à chaque cycle '
               + 'au lieu de balayer. Plage : Sine, Triangle, Saw, Square, S&H.',
              reviewed: false },
    },
    'tip.lfo3Sync': {
        en: { t: 'LFO 3 Sync',
              b: 'Locks LFO 3 to the host tempo. The button reads Free while the LFO runs at its own '
               + 'Rate and Sync while it follows a note division; switching also swaps which of the '
               + 'two controls beside it is shown. Range: Free, Sync.' },
        fr: { t: 'Synchronisation de l’OBF 3',
              b: 'Verrouille l’OBF 3 sur le tempo de l’hôte. Le bouton affiche Libre tant que l’OBF '
               + 'tourne à sa propre Vit. et Synchro lorsqu’il suit une division ; le basculement '
               + 'échange aussi les deux commandes voisines. Plage : Libre, Synchro.',
              reviewed: false },
    },
    'tip.lfo3Division': {
        en: { t: 'LFO 3 Division',
              b: 'Note value LFO 3 cycles on while Sync is on. A D suffix is dotted and a T suffix is '
               + 'a triplet. This control is hidden, and the Rate knob shown in its place, whenever '
               + 'Sync is off. Range: 1/1 down to 1/32, each also dotted and triplet.' },
        fr: { t: 'Division de l’OBF 3',
              b: 'Valeur de note sur laquelle l’OBF 3 boucle quand la synchronisation est active. Le '
               + 'suffixe D marque le pointé, le suffixe T le triolet. Cette commande est masquée, et '
               + 'le bouton Vit. affiché à sa place, dès que la synchronisation est coupée. '
               + 'Plage : de 1/1 à 1/32, chacune également pointée et en triolet.',
              reviewed: false },
    },
    'tip.lfo3FreeRun': {
        en: { t: 'LFO 3 Free Run',
              b: 'Decides whether LFO 3’s phase restarts on each note. Retrig gives every note the '
               + 'same sweep from the same point; Free Run keeps one phase running across notes, so '
               + 'a held chord moves together rather than in scattered phases. '
               + 'Range: Retrig, Free Run.' },
        fr: { t: 'Défilement libre de l’OBF 3',
              b: 'Détermine si la phase de l’OBF 3 repart à chaque note. Redécl. donne à chaque note '
               + 'le même balayage depuis le même point ; Continu conserve une phase unique d’une '
               + 'note à l’autre, si bien qu’un accord tenu évolue d’un seul mouvement. '
               + 'Plage : Redécl., Continu.',
              reviewed: false },
    },

    'tip.lfo4Rate': {
        en: { t: 'LFO 4 Rate',
              b: 'Speed of LFO 4 while Sync is off. An LFO reaches nothing on its own — route it to '
               + 'a destination in the Mod tab first. Range 0.01 to 20 Hz, though the one-decimal '
               + 'readout shows the bottom of that range as 0.0 Hz.' },
        fr: { t: 'Vitesse de l’OBF 4',
              b: 'Vitesse de l’OBF 4 lorsque la synchronisation est désactivée. Un OBF n’agit sur '
               + 'rien de lui-même : routez-le d’abord vers une destination dans l’onglet Modul. '
               + 'Plage de 0,01 à 20 Hz, le relevé à une décimale affichant 0,0 Hz au bas de cette '
               + 'plage.',
              reviewed: false },
    },
    'tip.lfo4Shape': {
        en: { t: 'LFO 4 Shape',
              b: 'Waveform of LFO 4. S&H holds a new random value for each cycle instead of sweeping '
               + 'through one. Range: Sine, Triangle, Saw, Square, S&H.' },
        fr: { t: 'Forme de l’OBF 4',
              b: 'Forme d’onde de l’OBF 4. S&H maintient une nouvelle valeur aléatoire à chaque cycle '
               + 'au lieu de balayer. Plage : Sine, Triangle, Saw, Square, S&H.',
              reviewed: false },
    },
    'tip.lfo4Sync': {
        en: { t: 'LFO 4 Sync',
              b: 'Locks LFO 4 to the host tempo. The button reads Free while the LFO runs at its own '
               + 'Rate and Sync while it follows a note division; switching also swaps which of the '
               + 'two controls beside it is shown. Range: Free, Sync.' },
        fr: { t: 'Synchronisation de l’OBF 4',
              b: 'Verrouille l’OBF 4 sur le tempo de l’hôte. Le bouton affiche Libre tant que l’OBF '
               + 'tourne à sa propre Vit. et Synchro lorsqu’il suit une division ; le basculement '
               + 'échange aussi les deux commandes voisines. Plage : Libre, Synchro.',
              reviewed: false },
    },
    'tip.lfo4Division': {
        en: { t: 'LFO 4 Division',
              b: 'Note value LFO 4 cycles on while Sync is on. A D suffix is dotted and a T suffix is '
               + 'a triplet. This control is hidden, and the Rate knob shown in its place, whenever '
               + 'Sync is off. Range: 1/1 down to 1/32, each also dotted and triplet.' },
        fr: { t: 'Division de l’OBF 4',
              b: 'Valeur de note sur laquelle l’OBF 4 boucle quand la synchronisation est active. Le '
               + 'suffixe D marque le pointé, le suffixe T le triolet. Cette commande est masquée, et '
               + 'le bouton Vit. affiché à sa place, dès que la synchronisation est coupée. '
               + 'Plage : de 1/1 à 1/32, chacune également pointée et en triolet.',
              reviewed: false },
    },
    'tip.lfo4FreeRun': {
        en: { t: 'LFO 4 Free Run',
              b: 'Decides whether LFO 4’s phase restarts on each note. Retrig gives every note the '
               + 'same sweep from the same point; Free Run keeps one phase running across notes, so '
               + 'a held chord moves together rather than in scattered phases. '
               + 'Range: Retrig, Free Run.' },
        fr: { t: 'Défilement libre de l’OBF 4',
              b: 'Détermine si la phase de l’OBF 4 repart à chaque note. Redécl. donne à chaque note '
               + 'le même balayage depuis le même point ; Continu conserve une phase unique d’une '
               + 'note à l’autre, si bien qu’un accord tenu évolue d’un seul mouvement. '
               + 'Plage : Redécl., Continu.',
              reviewed: false },
    },

    // ── Footer: the two global knobs ────────────────────────────────────────
    // Both are `pct` :2469 on the 44 px `small` knob variant.

    'tip.masterVol': {
        en: { t: 'Master Volume',
              b: 'Output level of the whole instrument, after the effects. It is the last thing in '
               + 'the chain, so use it to match the patch to the rest of the session rather than to '
               + 'balance oscillators. Range 0 to 100 %.' },
        fr: { t: 'Volume général',
              b: 'Niveau de sortie de l’instrument entier, après les effets. C’est le dernier maillon '
               + 'de la chaîne : servez-vous-en pour caler le son sur le reste de la session, pas '
               + 'pour équilibrer les oscillateurs. Plage de 0 à 100 %.',
              reviewed: false },
    },
    'tip.oscMix': {
        en: { t: 'Osc Mix',
              b: 'Crossfades between oscillator A and oscillator B. At 0 only A is heard, at 100 % '
               + 'only B, and the 50 % default gives both at equal weight. It is a mix, not a mute — '
               + 'each oscillator still has its own Level. Range 0 to 100 %.' },
        fr: { t: 'Mix des oscillateurs',
              b: 'Fondu enchaîné entre l’oscillateur A et l’oscillateur B. À 0 on n’entend que A, à '
               + '100 % que B, et la valeur par défaut de 50 % donne les deux à poids égal. C’est un '
               + 'mélange et non une coupure : chaque oscillateur garde son propre Niv. '
               + 'Plage de 0 à 100 %.',
              reviewed: false },
    },

    // ── Chrome ──────────────────────────────────────────────────────────────
    // The gear tip is what tells a user hover-help exists at all, so its body
    // must describe ONLY what that popover actually contains. On this page that
    // is the language selector and nothing else: there is no hover-help toggle
    // here, and O-Tapestop's wording — which promises one — would be a sentence
    // that lies. The popover opens BELOW the gear (`.settings-popover`,
    // top: 26px, index.html:117), which is the other detail a copied tip gets
    // wrong.
    //
    // THE ONE DECIMAL POINT IN THE FRENCH COPY IS `1.2kHz`, IN tip.language, AND
    // IT IS DELIBERATE. Every other French body here uses the decimal comma,
    // which is the settled rule (M1, 2026-08-30). That one is not a number in a
    // sentence — it is a QUOTATION of what the readout node actually prints, in
    // a sentence whose whole point is that readouts stay English under D-03.
    // Writing `1,2kHz` there would misquote the screen. A `\\d+\\.\\d+` scan of
    // this file finds exactly one hit and this is it.

    'tip.gear': {
        en: { t: 'Settings',
              b: 'Opens the settings panel below this button. It holds one control, the interface '
               + 'language, and the choice is saved with the session rather than sent to the host as '
               + 'automation.' },
        fr: { t: 'Réglages',
              b: 'Ouvre le panneau de réglages sous ce bouton. Il ne contient qu’une commande, la '
               + 'langue de l’interface, et ce choix est enregistré avec la session plutôt qu’envoyé '
               + 'à l’hôte comme automation.',
              reviewed: false },
    },
    'tip.language': {
        en: { t: 'Language',
              b: 'Switches every caption, section heading and hover-help on this page between '
               + 'English and French, at once and without reopening the plugin. Value readouts stay '
               + 'in English — a figure like 375ms or 1.2kHz is the same in both. '
               + 'Range: English, Français.' },
        fr: { t: 'Langue',
              b: 'Bascule d’un coup toutes les légendes, tous les titres de section et toute l’aide '
               + 'contextuelle de cette page entre l’anglais et le français, sans rouvrir le '
               + 'greffon. Les relevés de valeur restent en anglais : une indication comme 375ms ou '
               + '1.2kHz est identique dans les deux langues. Plage : English, Français.',
              reviewed: false },
    },
});

// ============================================================================
// TIP_BINDINGS — [selector, key] or [selector, key, wrapper]
// ============================================================================
//
// applyI18n() does `document.querySelector(selector)`, then
// `el.closest(wrapper) || el`, and writes `data-tip-title` + `data-tip` on the
// result. The wrapper exists so the selector can find an ADDRESSABLE child while
// the tip lands on the CELL the user aims at.
//
// "BIND TO THE IDS THE UI ALREADY USES" (T17) IS TRUE ON THIS PAGE, and it is
// the first plugin in fifteen where both halves hold:
//   * SELECTOR half — all 107 are ids. The page carries 154 of them and no
//     `data-param` attributes at all, so there was never an attribute selector
//     to reach for.
//   * TARGET half — the id IS the cell, for all but four. `.knob-container` is
//     an inline-flex COLUMN whose only child is `#knob-<paramId>`, so the two
//     rects are identical and a `closest()` walk would buy nothing; the same is
//     true of `#select-<paramId>` inside `.dropdown-group`, where the walk WOULD
//     add the caption above it. Both were measured rather than assumed — see
//     tests/ui_tip_render_check.js assertion [1], which prints the self-area and
//     the walked area for every row.
//
// FOUR ROWS DECLARE A WRAPPER, and each is a case where the id'd node genuinely
// is not the hover target:
//   * the 23 `#select-*` rows walk to `.dropdown-group` so the caption above the
//     dropdown opens the same tip as the dropdown;
//   * `#knob-masterVol` and `#knob-oscMix` are BESPOKE markup holding only the
//     44 px svg — their caption and readout are SIBLINGS inside `.footer-param`;
//   * `#octave-stretch` is a 90 px `<input type="range">` in a row that also
//     holds its caption and its readout.
//
// THE CHROME BINDS BARE, on purpose. `.settings-wrap` contains BOTH `#gear-btn`
// and the popover that holds `#lang-select`, so any wrapper walk would make
// hovering the selector resolve to the gear's own tip — M2's finding 7, which
// bit O-Comp for the same reason.
//
// The 14 `#toggle-*` rows also bind bare: each sits inside a `.section-header`
// or a caption wrapper it SHARES with another control, and a walk would put two
// bindings on one node. The render gate asserts every binding lands on a
// DISTINCT node precisely so that a second row silently overwriting the first
// cannot pass while reporting two bound tips.
export const TIP_BINDINGS = [

    // ── Oscillator A ────────────────────────────────────────────────────────
    ['#select-oscATable',      'tip.oscATable',      '.dropdown-group'],
    ['#knob-oscAPos',          'tip.oscAPos'],
    ['#knob-oscALevel',        'tip.oscALevel'],
    ['#knob-oscAPan',          'tip.oscAPan'],
    ['#knob-oscACoarse',       'tip.oscACoarse'],
    ['#knob-oscAFine',         'tip.oscAFine'],
    ['#knob-oscAPhase',        'tip.oscAPhase'],
    ['#knob-oscAUnison',       'tip.oscAUnison'],
    ['#knob-oscADetune',       'tip.oscADetune'],
    ['#knob-oscAWidth',        'tip.oscAWidth'],
    ['#select-oscAWarpType',   'tip.oscAWarpType',   '.dropdown-group'],
    ['#knob-oscAWarpAmt',      'tip.oscAWarpAmt'],

    // ── Oscillator B ────────────────────────────────────────────────────────
    ['#select-oscBTable',      'tip.oscBTable',      '.dropdown-group'],
    ['#knob-oscBPos',          'tip.oscBPos'],
    ['#knob-oscBLevel',        'tip.oscBLevel'],
    ['#knob-oscBPan',          'tip.oscBPan'],
    ['#knob-oscBCoarse',       'tip.oscBCoarse'],
    ['#knob-oscBFine',         'tip.oscBFine'],
    ['#knob-oscBPhase',        'tip.oscBPhase'],
    ['#knob-oscBUnison',       'tip.oscBUnison'],
    ['#knob-oscBDetune',       'tip.oscBDetune'],
    ['#knob-oscBWidth',        'tip.oscBWidth'],
    ['#select-oscBWarpType',   'tip.oscBWarpType',   '.dropdown-group'],
    ['#knob-oscBWarpAmt',      'tip.oscBWarpAmt'],

    // ── Sub oscillator and noise ────────────────────────────────────────────
    ['#select-subShape',       'tip.subShape',       '.dropdown-group'],
    ['#select-subOctave',      'tip.subOctave',      '.dropdown-group'],
    ['#knob-subLevel',         'tip.subLevel'],
    ['#select-noiseType',      'tip.noiseType',      '.dropdown-group'],
    ['#knob-noiseLevel',       'tip.noiseLevel'],
    ['#select-subRouting',     'tip.subRouting',     '.dropdown-group'],

    // ── Envelopes ───────────────────────────────────────────────────────────
    ['#knob-ampAttack',        'tip.ampAttack'],
    ['#knob-ampDecay',         'tip.ampDecay'],
    ['#knob-ampSustain',       'tip.ampSustain'],
    ['#knob-ampRelease',       'tip.ampRelease'],
    ['#knob-filtAttack',       'tip.filtAttack'],
    ['#knob-filtDecay',        'tip.filtDecay'],
    ['#knob-filtSustain',      'tip.filtSustain'],
    ['#knob-filtRelease',      'tip.filtRelease'],
    ['#knob-filtAEnvDepth',    'tip.filtAEnvDepth'],
    ['#knob-filtBEnvDepth',    'tip.filtBEnvDepth'],

    // ── Filters ─────────────────────────────────────────────────────────────
    ['#select-filtAType',      'tip.filtAType',      '.dropdown-group'],
    ['#knob-filtACutoff',      'tip.filtACutoff'],
    ['#knob-filtARes',         'tip.filtARes'],
    ['#knob-filtADrive',       'tip.filtADrive'],
    ['#knob-filtAKeyTrack',    'tip.filtAKeyTrack'],
    ['#select-filtBType',      'tip.filtBType',      '.dropdown-group'],
    ['#knob-filtBCutoff',      'tip.filtBCutoff'],
    ['#knob-filtBRes',         'tip.filtBRes'],
    ['#knob-filtBDrive',       'tip.filtBDrive'],
    ['#knob-filtBKeyTrack',    'tip.filtBKeyTrack'],
    ['#select-filtRouting',    'tip.filtRouting',    '.dropdown-group'],

    // ── Performance and tuning ──────────────────────────────────────────────
    // `#ref-pitch-knob` is the bespoke 64 px A4 knob and already holds its own
    // caption and readout, so it binds bare. `#octave-stretch` is a slider whose
    // caption and readout are siblings, so it walks.
    ['#knob-pitchBendRange',   'tip.pitchBendRange'],
    ['#select-glideMode',      'tip.glideMode',      '.dropdown-group'],
    ['#knob-glideTime',        'tip.glideTime'],
    ['#ref-pitch-knob',        'tip.masterTune'],
    ['#octave-stretch',        'tip.octaveStretch',  '.octave-stretch-row'],

    // ── Effects ─────────────────────────────────────────────────────────────
    ['#toggle-delayBypass',    'tip.delayBypass'],
    ['#knob-delayTime',        'tip.delayTime'],
    ['#knob-delayFeedback',    'tip.delayFeedback'],
    ['#select-delayMode',      'tip.delayMode',      '.dropdown-group'],
    ['#toggle-delaySync',      'tip.delaySync'],
    ['#select-delayDivision',  'tip.delayDivision',  '.dropdown-group'],
    ['#knob-delayMix',         'tip.delayMix'],

    ['#toggle-chorusBypass',   'tip.chorusBypass'],
    ['#knob-chorusRate',       'tip.chorusRate'],
    ['#knob-chorusDepth',      'tip.chorusDepth'],
    ['#knob-chorusMix',        'tip.chorusMix'],

    ['#toggle-distBypass',     'tip.distBypass'],
    ['#select-distType',       'tip.distType',       '.dropdown-group'],
    ['#knob-distDrive',        'tip.distDrive'],
    ['#knob-distMix',          'tip.distMix'],

    ['#toggle-reverbBypass',   'tip.reverbBypass'],
    ['#knob-reverbSize',       'tip.reverbSize'],
    ['#knob-reverbDamp',       'tip.reverbDamp'],
    ['#knob-reverbPredelay',   'tip.reverbPredelay'],
    ['#knob-reverbMix',        'tip.reverbMix'],
    ['#knob-reverbModDepth',   'tip.reverbModDepth'],
    ['#knob-reverbModRate',    'tip.reverbModRate'],

    ['#toggle-eqBypass',       'tip.eqBypass'],
    ['#knob-eqLowGain',        'tip.eqLowGain'],
    ['#knob-eqMidGain',        'tip.eqMidGain'],
    ['#knob-eqMidFreq',        'tip.eqMidFreq'],
    ['#knob-eqHighGain',       'tip.eqHighGain'],

    // ── LFOs ────────────────────────────────────────────────────────────────
    // The Rate knob and the Division dropdown are mutually exclusive: one of the
    // two is `display: none` at any moment, decided by that LFO's Sync button.
    // Both are bound; the render gate drives the Sync button through the page's
    // own click handler to reach the hidden half, rather than stripping the
    // inline style, which would measure a state the plugin never reaches.
    ['#knob-lfo1Rate',         'tip.lfo1Rate'],
    ['#select-lfo1Shape',      'tip.lfo1Shape',      '.dropdown-group'],
    ['#toggle-lfo1Sync',       'tip.lfo1Sync'],
    ['#select-lfo1Division',   'tip.lfo1Division',   '.dropdown-group'],
    ['#toggle-lfo1FreeRun',    'tip.lfo1FreeRun'],

    ['#knob-lfo2Rate',         'tip.lfo2Rate'],
    ['#select-lfo2Shape',      'tip.lfo2Shape',      '.dropdown-group'],
    ['#toggle-lfo2Sync',       'tip.lfo2Sync'],
    ['#select-lfo2Division',   'tip.lfo2Division',   '.dropdown-group'],
    ['#toggle-lfo2FreeRun',    'tip.lfo2FreeRun'],

    ['#knob-lfo3Rate',         'tip.lfo3Rate'],
    ['#select-lfo3Shape',      'tip.lfo3Shape',      '.dropdown-group'],
    ['#toggle-lfo3Sync',       'tip.lfo3Sync'],
    ['#select-lfo3Division',   'tip.lfo3Division',   '.dropdown-group'],
    ['#toggle-lfo3FreeRun',    'tip.lfo3FreeRun'],

    ['#knob-lfo4Rate',         'tip.lfo4Rate'],
    ['#select-lfo4Shape',      'tip.lfo4Shape',      '.dropdown-group'],
    ['#toggle-lfo4Sync',       'tip.lfo4Sync'],
    ['#select-lfo4Division',   'tip.lfo4Division',   '.dropdown-group'],
    ['#toggle-lfo4FreeRun',    'tip.lfo4FreeRun'],

    // ── Footer ──────────────────────────────────────────────────────────────
    ['#knob-masterVol',        'tip.masterVol',      '.footer-param'],
    ['#knob-oscMix',           'tip.oscMix',         '.footer-param'],

    // ── Chrome — BARE, see the note above ───────────────────────────────────
    ['#gear-btn',              'tip.gear'],
    ['#lang-select',           'tip.language'],
];

export const LABELS = Object.freeze({

    // ── Header ──────────────────────────────────────────────────────────────
    // MEASURED CONSTRAINT, not a style preference: `.header-bar` is
    // `justify-content: space-between`, so this caption's width decides where
    // the preset browser sits. `Synthétiseur microtonal à tables d’ondes` is
    // 242.13 px against the English 202.78 and drags the browser 13.1 px left.
    // Anything at or under 202.78 px is free.
    'label.subtitle':        { en: { t: 'Microtonal Wavetable Synthesizer' },
                               fr: { t: 'Synthé microtonal à tables d’onde', reviewed: false } },
    'label.language':        { en: { t: 'Language' },      fr: { t: 'Langue',        reviewed: false } },
    'aria.settings':         { en: { t: 'Settings' },      fr: { t: 'Réglages',      reviewed: false } },
    'aria.presetPrev':       { en: { t: 'Previous Preset' },
                               fr: { t: 'Préréglage précédent', reviewed: false } },
    'aria.presetNext':       { en: { t: 'Next Preset' },   fr: { t: 'Préréglage suivant', reviewed: false } },
    'aria.presetSave':       { en: { t: 'Save Preset' },   fr: { t: 'Enregistrer le préréglage', reviewed: false } },
    'aria.presetBrowse':     { en: { t: 'Click to browse presets' },
                               fr: { t: 'Cliquer pour parcourir les préréglages', reviewed: false } },

    // ── Tabs ────────────────────────────────────────────────────────────────
    // `.tab` is `flex: 1` across the full 1200 px frame, so these five have the
    // most room on the page and are the only captions written out in full.
    'tab.synth':             { en: { t: 'Synth' },         fr: { t: 'Synthé',        reviewed: false } },
    'tab.mod':               { en: { t: 'Mod' },           fr: { t: 'Modul.',        reviewed: false } },
    'tab.tuning':            { en: { t: 'Tuning' },        fr: { t: 'Accord',        reviewed: false } },
    'tab.effects':           { en: { t: 'Effects' },       fr: { t: 'Effets',        reviewed: false } },
    'tab.wavetable':         { en: { t: 'Wavetable' },     fr: { t: 'Table d’onde',  reviewed: false } },

    // ── Synth tab: section headers ──────────────────────────────────────────
    'label.oscA':            { en: { t: 'Oscillator A' },  fr: { t: 'Oscillateur A', reviewed: false } },
    'label.oscB':            { en: { t: 'Oscillator B' },  fr: { t: 'Oscillateur B', reviewed: false } },
    'label.subOsc':          { en: { t: 'Sub Oscillator' }, fr: { t: 'Sous-oscillateur', reviewed: false } },
    'label.noise':           { en: { t: 'Noise' },         fr: { t: 'Bruit',         reviewed: false } },
    'label.performance':     { en: { t: 'Performance' },   fr: { t: 'Performance',   reviewed: false, sameAsEn: true } },
    'label.filterA':         { en: { t: 'Filter A' },      fr: { t: 'Filtre A',      reviewed: false } },
    'label.filterB':         { en: { t: 'Filter B' },      fr: { t: 'Filtre B',      reviewed: false } },
    'label.ampEnv':          { en: { t: 'Amp Envelope' },  fr: { t: 'Enveloppe ampli', reviewed: false } },
    'label.filtEnv':         { en: { t: 'Filter Envelope' }, fr: { t: 'Enveloppe filtre', reviewed: false } },
    // The four LFO headers hold BUTTON CHILDREN as well as this text, so each
    // one is split into its own <span> (contract section 5). Writing textContent
    // on the header itself would delete the Free / Retrig buttons beside it.
    'label.lfo1':            { en: { t: 'LFO 1' },         fr: { t: 'OBF 1',         reviewed: false } },
    'label.lfo2':            { en: { t: 'LFO 2' },         fr: { t: 'OBF 2',         reviewed: false } },
    'label.lfo3':            { en: { t: 'LFO 3' },         fr: { t: 'OBF 3',         reviewed: false } },
    'label.lfo4':            { en: { t: 'LFO 4' },         fr: { t: 'OBF 4',         reviewed: false } },

    // ── Dropdown captions ───────────────────────────────────────────────────
    // `.dropdown-group` is inline-flex and shrink-wraps around the WIDER of its
    // caption and its <select>, so a caption longer than the select widens the
    // group and pushes every control to its right.
    'label.shape':           { en: { t: 'Shape' },         fr: { t: 'Forme',         reviewed: false } },
    'label.warp':            { en: { t: 'Warp' },          fr: { t: 'Déform.',       reviewed: false } },
    'label.octave':          { en: { t: 'Octave' },        fr: { t: 'Octave',        reviewed: false, sameAsEn: true } },
    'label.routing':         { en: { t: 'Routing' },       fr: { t: 'Routage',       reviewed: false } },
    'label.type':            { en: { t: 'Type' },          fr: { t: 'Type',          reviewed: false, sameAsEn: true } },
    'label.glideMode':       { en: { t: 'Glide Mode' },    fr: { t: 'Mode glissé',   reviewed: false } },
    'label.filterRouting':   { en: { t: 'Filter Routing' }, fr: { t: 'Routage filt.', reviewed: false } },
    'label.division':        { en: { t: 'Division' },      fr: { t: 'Division',      reviewed: false, sameAsEn: true } },
    'label.mode':            { en: { t: 'Mode' },          fr: { t: 'Mode',          reviewed: false, sameAsEn: true } },
    'label.dropWav':         { en: { t: 'Drop WAV' },      fr: { t: 'Déposer WAV',   reviewed: false } },

    // ── The 64 knob captions ────────────────────────────────────────────────
    // Keyed on the STATIC `.knob-container[data-i18n]`, moved onto the generated
    // `.knob-label` span by expandKnobMarkup(). 64 attributes, 35 distinct
    // strings, 35 keys — one per string, shared wherever the caption repeats.
    'label.position':        { en: { t: 'Position' },      fr: { t: 'Position',      reviewed: false, sameAsEn: true } },
    'label.level':           { en: { t: 'Level' },         fr: { t: 'Niv.',          reviewed: false } },
    'label.pan':             { en: { t: 'Pan' },           fr: { t: 'Pano',          reviewed: false } },
    // `Grossier` is 51.75 px and clears neither test: the knob column is 52 px, and
    // the neighbouring knob's ROTATED svg puts its own bounding box 5.1 px into
    // this column, which caps a caption here at 49.8 px.
    'label.coarse':          { en: { t: 'Coarse' },        fr: { t: 'Gross.',        reviewed: false } },
    'label.fine':            { en: { t: 'Fine' },          fr: { t: 'Fin',           reviewed: false } },
    'label.phase':           { en: { t: 'Phase' },         fr: { t: 'Phase',         reviewed: false, sameAsEn: true } },
    'label.unison':          { en: { t: 'Unison' },        fr: { t: 'Unisson',       reviewed: false } },
    'label.detune':          { en: { t: 'Detune' },        fr: { t: 'Désacc.',       reviewed: false } },
    'label.width':           { en: { t: 'Width' },         fr: { t: 'Larg.',         reviewed: false } },
    'label.warpAmt':         { en: { t: 'Warp Amt' },      fr: { t: 'Qté déf.',      reviewed: false } },
    'label.pbRange':         { en: { t: 'PB Range' },      fr: { t: 'Ampl. PB',      reviewed: false } },
    'label.glide':           { en: { t: 'Glide' },         fr: { t: 'Glissé',        reviewed: false } },
    'label.cutoff':          { en: { t: 'Cutoff' },        fr: { t: 'Coupure',       reviewed: false } },
    'label.reso':            { en: { t: 'Reso' },          fr: { t: 'Réso',          reviewed: false } },
    'label.drive':           { en: { t: 'Drive' },         fr: { t: 'Satur.',        reviewed: false } },
    'label.keyTrk':          { en: { t: 'Key Trk' },       fr: { t: 'Suivi',         reviewed: false } },
    'label.attack':          { en: { t: 'Attack' },        fr: { t: 'Attaque',       reviewed: false } },
    'label.decay':           { en: { t: 'Decay' },         fr: { t: 'Chute',         reviewed: false } },
    'label.sustain':         { en: { t: 'Sustain' },       fr: { t: 'Tenue',         reviewed: false } },
    'label.release':         { en: { t: 'Release' },       fr: { t: 'Relâche',       reviewed: false } },
    'label.depA':            { en: { t: 'Dep A' },         fr: { t: 'Prof A',        reviewed: false } },
    'label.depB':            { en: { t: 'Dep B' },         fr: { t: 'Prof B',        reviewed: false } },
    'label.rate':            { en: { t: 'Rate' },          fr: { t: 'Vit.',          reviewed: false } },
    'label.time':            { en: { t: 'Time' },          fr: { t: 'Temps',         reviewed: false } },
    'label.feedback':        { en: { t: 'Feedback' },      fr: { t: 'Réaction',      reviewed: false } },
    // `Mix` is the word French audio software uses; `Dosage` is more correct and
    // 21 px wider, which is 3 px past the 38.45 px this column can hold.
    'label.mix':             { en: { t: 'Mix' },           fr: { t: 'Mix',           reviewed: false, sameAsEn: true } },
    'label.depth':           { en: { t: 'Depth' },         fr: { t: 'Prof.',         reviewed: false } },
    'label.size':            { en: { t: 'Size' },          fr: { t: 'Taille',        reviewed: false } },
    'label.damp':            { en: { t: 'Damp' },          fr: { t: 'Amor.',         reviewed: false } },
    'label.preDly':          { en: { t: 'Pre-Dly' },       fr: { t: 'Pré-dél.',      reviewed: false } },
    'label.modAmt':          { en: { t: 'Mod' },           fr: { t: 'Mod.',          reviewed: false } },
    'label.low':             { en: { t: 'Low' },           fr: { t: 'Grave',         reviewed: false } },
    'label.mid':             { en: { t: 'Mid' },           fr: { t: 'Méd.',          reviewed: false } },
    'label.midFreq':         { en: { t: 'Mid Freq' },      fr: { t: 'Fq. méd',       reviewed: false } },
    'label.high':            { en: { t: 'High' },          fr: { t: 'Aigu',          reviewed: false } },

    // ── Mod matrix ──────────────────────────────────────────────────────────
    'label.modMatrix':       { en: { t: 'Modulation Matrix' },
                               fr: { t: 'Matrice de modulation', reviewed: false } },
    'label.modMatrixInfo':   { en: { t: 'Route any source to any destination. 16 slots available.' },
                               fr: { t: 'Acheminer n’importe quelle source vers n’importe quelle destination. 16 emplacements.', reviewed: false } },
    // `.mod-col-on` is a fixed 36 px column, which is what decides this against
    // the fuller `Activé`.
    'label.colOn':           { en: { t: 'On' },            fr: { t: 'Act.',          reviewed: false } },
    'label.colSource':       { en: { t: 'Source' },        fr: { t: 'Source',        reviewed: false, sameAsEn: true } },
    'label.colDest':         { en: { t: 'Destination' },   fr: { t: 'Destination',   reviewed: false, sameAsEn: true } },
    'label.colAmount':       { en: { t: 'Amount' },        fr: { t: 'Quantité',      reviewed: false } },

    // ── Tuning tab ──────────────────────────────────────────────────────────
    // The count is a {token} and the noun is gone — see the header note.
    'label.intervalCount':   { en: { t: 'Intervals ({n})' },
                               fr: { t: 'Intervalles ({n})', reviewed: false } },
    'label.tonic':           { en: { t: 'Tonic:' },        fr: { t: 'Ton. :',        reviewed: false } },
    'label.vizCircle':       { en: { t: 'Circle' },        fr: { t: 'Cercle',        reviewed: false } },
    'label.vizPolar':        { en: { t: 'Polar' },         fr: { t: 'Polaire',       reviewed: false } },
    'label.vizMatrix':       { en: { t: 'Matrix' },        fr: { t: 'Matrice',       reviewed: false } },
    'label.vizTrueKeys':     { en: { t: 'True Keys' },     fr: { t: 'Touches',       reviewed: false } },
    'label.vizRotation':     { en: { t: 'Rotation' },      fr: { t: 'Rotation',      reviewed: false, sameAsEn: true } },
    'label.scaleIntervals':  { en: { t: 'Scale Intervals' }, fr: { t: 'Intervalles de la gamme', reviewed: false } },
    'label.tkHint':          { en: { t: 'Hold 2+ notes to see intervals' },
                               fr: { t: 'Tenir 2 notes ou plus pour voir les intervalles', reviewed: false } },
    'label.heldIntervals':   { en: { t: 'Intervals:' },    fr: { t: 'Interv. :',     reviewed: false } },
    'label.span':            { en: { t: 'Span' },          fr: { t: 'Écart',         reviewed: false } },
    'label.totalSpan':       { en: { t: 'Total span' },    fr: { t: 'Écart total',   reviewed: false } },
    'label.rotationMode':    { en: { t: 'Mode' },          fr: { t: 'Mode',          reviewed: false, sameAsEn: true } },
    'label.tuningLibrary':   { en: { t: 'Tuning Library' }, fr: { t: 'Bibliothèque',  reviewed: false } },
    // The five category captions are keyed HERE, on the filter <select>'s
    // options, and the same five keys are reused by the library list's own
    // category span. The <option value="..."> attributes stay English because
    // they are matched against the C++ category strings.
    'label.catAll':          { en: { t: 'All Categories' }, fr: { t: 'Toutes catégories', reviewed: false } },
    'label.catHistorical':   { en: { t: 'Historical' },    fr: { t: 'Historique',    reviewed: false } },
    'label.catJust':         { en: { t: 'Just Intonation' }, fr: { t: 'Intonation juste', reviewed: false } },
    'label.catEqual':        { en: { t: 'Equal Divisions' }, fr: { t: 'Divisions égales', reviewed: false } },
    'label.catNonOctave':    { en: { t: 'Non-Octave' },    fr: { t: 'Non-octaviant', reviewed: false } },
    'label.catWorld':        { en: { t: 'World' },         fr: { t: 'Monde',         reviewed: false } },
    'label.libNotes':        { en: { t: 'notes' },         fr: { t: 'notes',         reviewed: false, sameAsEn: true } },
    'label.libPeriod':       { en: { t: 'period' },        fr: { t: 'période',       reviewed: false } },
    // `.knob-label` inside the bespoke A4 knob — same 52 px column as the 64.
    'label.a4Ref':           { en: { t: 'A4 Ref' },        fr: { t: 'Réf. A4',       reviewed: false } },
    // `.octave-stretch-label` is `min-width: 40px` in a 210 px panel and the
    // slider beside it takes the remainder, so `Étirement` would shrink the
    // slider by 8 px. That is what decides this abbreviation.
    'label.stretch':         { en: { t: 'Stretch' },       fr: { t: 'Étir.',         reviewed: false } },
    'label.loadScl':         { en: { t: 'Load .SCL' },     fr: { t: 'Charger .SCL',  reviewed: false } },
    'label.loadKbm':         { en: { t: 'Load .KBM' },     fr: { t: 'Charger .KBM',  reviewed: false } },
    'label.saveScl':         { en: { t: 'Save .SCL' },     fr: { t: 'Enreg. .SCL',   reviewed: false } },
    'label.saveKbm':         { en: { t: 'Save .KBM' },     fr: { t: 'Enreg. .KBM',   reviewed: false } },
    'label.exportHtml':      { en: { t: 'Export HTML' },   fr: { t: 'Exporter HTML', reviewed: false } },
    'label.generateScale':   { en: { t: 'Generate Scale' }, fr: { t: 'Générer gamme', reviewed: false } },
    'label.genEdo':          { en: { t: 'EDO (Equal Division)' },
                               fr: { t: 'EDO (division égale)', reviewed: false } },
    'label.genHarmonic':     { en: { t: 'Harmonic Series' }, fr: { t: 'Série harmonique', reviewed: false } },
    'label.genRank2':        { en: { t: 'Rank-2 Temperament' },
                               fr: { t: 'Tempérament de rang 2', reviewed: false } },
    'label.genDivisions':    { en: { t: 'Divisions' },     fr: { t: 'Divisions',     reviewed: false, sameAsEn: true } },
    'label.genPeriod':       { en: { t: 'Period (cents)' }, fr: { t: 'Période (cents)', reviewed: false } },
    'label.genStartHarm':    { en: { t: 'Start Harmonic' }, fr: { t: 'Harmonique de départ', reviewed: false } },
    'label.genEndHarm':      { en: { t: 'End Harmonic' },  fr: { t: 'Harmonique de fin', reviewed: false } },
    'label.genGenerator':    { en: { t: 'Generator (cents)' },
                               fr: { t: 'Générateur (cents)', reviewed: false } },
    'label.genNotes':        { en: { t: 'Notes' },         fr: { t: 'Notes',         reviewed: false, sameAsEn: true } },
    'label.generate':        { en: { t: 'Generate' },      fr: { t: 'Générer',       reviewed: false } },

    // ── Effects tab ─────────────────────────────────────────────────────────
    'label.delay':           { en: { t: 'Delay' },         fr: { t: 'Délai',         reviewed: false } },
    'label.chorus':          { en: { t: 'Chorus' },        fr: { t: 'Chorus',        reviewed: false, sameAsEn: true } },
    'label.distortion':      { en: { t: 'Distortion' },    fr: { t: 'Distorsion',    reviewed: false } },
    'label.reverb':          { en: { t: 'Reverb' },        fr: { t: 'Réverb.',       reviewed: false } },
    'label.eq3':             { en: { t: '3-Band EQ' },     fr: { t: 'ÉG 3 bandes',   reviewed: false } },
    // The caption above the delay Sync toggle. `Synchro` is 21 px wider and the
    // cell it sits in is 44.44 px, so it pushed the whole delay row 16 px.
    // `Sync` is current usage in French DAWs; the LFO button keeps `Synchro`,
    // where a section header gives it room.
    'label.sync':            { en: { t: 'Sync' },          fr: { t: 'Sync',          reviewed: false, sameAsEn: true } },

    // ── Wavetable tab ───────────────────────────────────────────────────────
    // `Osc A` is already the French abbreviation; the added period cost 4.2 px in
    // a shrink-wrapping toggle that pushes the harmonic toolbar behind it.
    'label.oscAShort':       { en: { t: 'Osc A' },         fr: { t: 'Osc A',         reviewed: false, sameAsEn: true } },
    'label.oscBShort':       { en: { t: 'Osc B' },         fr: { t: 'Osc B',         reviewed: false, sameAsEn: true } },
    // The seven ops-bar captions below are pinned to their English boxes in CSS,
    // so the row's three separators and its undo/redo pair hold still. Each
    // French string is the longest form that FITS its own English button:
    // `Normaliser` is 60.06 px in a 55.19 px box, `Fondre bords` 71.48 in
    // 60.61, `Inverser` 44.66 in 41.91, `Inverser ordre` 76.89 in 76.39, and
    // `Enregistrer` 60.72 in 24.50. The alternative was pinning to the FRENCH
    // and moving the English row 38 px, which is a visible change to a shipped
    // English UI for no English benefit.
    'label.harmonics':       { en: { t: 'Harmonics' },     fr: { t: 'Harmon.',       reviewed: false } },
    'label.waveform':        { en: { t: 'Waveform' },      fr: { t: 'Forme d’onde',  reviewed: false } },
    'label.normalize':       { en: { t: 'Normalize' },     fr: { t: 'Norm.',         reviewed: false } },
    'label.normalizeGlobal': { en: { t: 'Normalize Global' }, fr: { t: 'Normaliser tout', reviewed: false } },
    'label.fadeEdges':       { en: { t: 'Fade Edges' },    fr: { t: 'Fondre',        reviewed: false } },
    'label.reverse':         { en: { t: 'Reverse' },       fr: { t: 'Invers.',       reviewed: false } },
    'label.reverseOrder':    { en: { t: 'Reverse Order' }, fr: { t: 'Ordre inv.',    reviewed: false } },
    'label.smooth':          { en: { t: 'Smooth' },        fr: { t: 'Lisser',        reviewed: false } },
    // TWO keys for one English word. The two modal buttons have room for
    // `Enregistrer`; the ops-bar button is a 24.50 px box.
    'label.save':            { en: { t: 'Save' },          fr: { t: 'Enregistrer',   reviewed: false } },
    'label.saveShort':       { en: { t: 'Save' },          fr: { t: 'Enr.',          reviewed: false } },
    'label.cancel':          { en: { t: 'Cancel' },        fr: { t: 'Annuler',       reviewed: false } },
    'label.saveWavetable':   { en: { t: 'Save Wavetable' }, fr: { t: 'Enregistrer la table', reviewed: false } },
    'label.savePreset':      { en: { t: 'Save Preset' },   fr: { t: 'Enregistrer le préréglage', reviewed: false } },
    'label.userWavetables':  { en: { t: 'User Wavetables' }, fr: { t: 'Tables utilisateur', reviewed: false } },
    'label.close':           { en: { t: 'Close' },         fr: { t: 'Fermer',        reviewed: false } },
    'label.delete':          { en: { t: 'Delete' },        fr: { t: 'Supprimer',     reviewed: false } },
    'label.importWav':       { en: { t: 'Import WAV...' }, fr: { t: 'Importer WAV…', reviewed: false } },
    'label.manage':          { en: { t: 'Manage...' },     fr: { t: 'Gérer…',        reviewed: false } },
    'label.noUserWavetables': { en: { t: 'No user wavetables imported yet.' },
                               fr: { t: 'Aucune table utilisateur importée.', reviewed: false } },
    'aria.wavetableName':    { en: { t: 'Wavetable name...' }, fr: { t: 'Nom de la table…', reviewed: false } },
    'aria.presetName':       { en: { t: 'Preset name...' }, fr: { t: 'Nom du préréglage…', reviewed: false } },
    'aria.undo':             { en: { t: 'Undo (Ctrl+Z)' }, fr: { t: 'Annuler (Ctrl+Z)', reviewed: false } },
    'aria.redo':             { en: { t: 'Redo (Ctrl+Shift+Z)' },
                               fr: { t: 'Rétablir (Ctrl+Maj+Z)', reviewed: false } },

    // ── Footer ──────────────────────────────────────────────────────────────
    'label.master':          { en: { t: 'Master' },        fr: { t: 'Maître',        reviewed: false } },
    'label.oscMix':          { en: { t: 'Osc Mix' },       fr: { t: 'Mix osc',       reviewed: false } },

    // ── State faces written from script ─────────────────────────────────────
    // Every one goes through setLabel(), so the element becomes a [data-i18n]
    // element from that moment on and the language sweep owns it. A state string
    // written as a raw literal is stranded in the previous language the instant
    // the selector fires.
    //
    // These are the SIX TOGGLE BUTTON FACES, and they are localized rather than
    // exempted even though `delaySync`, `lfoNSync`, `lfoNFreeRun` and the five
    // bypass flags are all parameters. D-01 arm 1 exempts an
    // `AudioParameterChoice` OPTION that the page reproduces byte-identically;
    // every one of these is an `AudioParameterBool`, whose host text is JUCE's
    // generic Off/On boilerplate and not an authored choice name. `Free`,
    // `Retrig`, `Free Run` and `Sync` are not byte-identical to anything in the
    // automation lane in the first place.
    'ui.free':               { en: { t: 'Free' },          fr: { t: 'Libre',         reviewed: false } },
    'ui.sync':               { en: { t: 'Sync' },          fr: { t: 'Synchro',       reviewed: false } },
    // "Free Run" and "Free" are different concepts on adjacent buttons — one is
    // "not tempo-synced", the other is "phase runs across notes" — so they get
    // different French rather than colliding on `Libre`.
    'ui.freeRun':            { en: { t: 'Free Run' },      fr: { t: 'Continu',       reviewed: false } },
    'ui.retrig':             { en: { t: 'Retrig' },        fr: { t: 'Redécl.',       reviewed: false } },
    // The five bypass buttons carry the same two words in the markup's own
    // upper case. Separate keys, because the ownership mirror asserts
    // dataset.label === textContent and CSS text-transform is not textContent.
    'ui.bypassOn':           { en: { t: 'ON' },            fr: { t: 'MARCHE',        reviewed: false } },
    'ui.bypassOff':          { en: { t: 'OFF' },           fr: { t: 'ARRÊT',         reviewed: false } },
});

// ============================================================================
// I18N_EXEMPT — reasoned exclusions, never silence
// ============================================================================
//
// Every visible string the coverage scan finds must be a [data-i18n] element, a
// setLabel() call, or an entry HERE WITH A REASON. A bare skip list would let a
// missed label hide as a deliberate one.
//
// AN EXEMPTION IS MATCHED BY TEXT, so an unscoped one silences EVERY node with
// that string. On this page that hazard is real and not theoretical: `Sine`,
// `Square`, `Triangle`, `Saw`, `Harmonic Series`, `Off`, `Sync`, `Wind` and
// `Digital` each appear BOTH as a parameter dropdown option (exempt) and as a
// caption or a non-parameter option that must translate. So every option
// exemption below is SCOPED to `.param-select`, the class the 23 parameter
// dropdowns carry and the library filter and the scale generator do not.
// ============================================================================

export const I18N_EXEMPT = [
    // ── The delay Sync toggle face — D-01 arm 1, and the geometry that decided it
    // #toggle-delaySync shows `On` / `Off`, which is BYTE-IDENTICAL to what the
    // `delaySync` parameter reports to the host (measured in the runtime dump:
    // textAtMin "Off", textAtMax "On"). The five BYPASS buttons are keyed rather
    // than exempted because their faces are `ON` / `OFF` in the markup's own
    // upper case, which is not byte-identical to anything.
    //
    // Geometry made the call unambiguous: this button and its caption share a
    // 44.44 px cell in the middle of the delay row, and `Arrêt` / `Marche` push
    // every control to their right by 16 px. Scoped, because `Off` is also a
    // parameter dropdown option and `On` is also the mod-matrix column caption
    // `label.colOn`, which IS keyed.
    ['On',  'the #toggle-delaySync face, byte-identical to the delaySync parameter\'s '
          + 'host text ("Off"/"On", runtime param dump) — D-01 arm 1', '#toggle-delaySync'],
    ['Off', 'the #toggle-delaySync face, byte-identical to the delaySync parameter\'s '
          + 'host text ("Off"/"On", runtime param dump) — D-01 arm 1', '#toggle-delaySync'],

    // ── The product name ───────────────────────────────────────────────────
    ['O-PRISM', 'the product name — a product name is never translated'],

    // ── D-02: a name that IS an identifier ─────────────────────────────────
    ['— Init —',
     'the placeholder in #preset-current-name, which DISPLAYS a preset name. '
     + 'The name IS the JSON filename (OuariconPresetManager.h), so translating it '
     + 'breaks recall: a session saved against "Cathedral" would not resolve '
     + '"Cathédrale". Confirmed to be the name span and not a caption beside it — '
     + '#preset-current-category is its sibling and carries the category',
     '#preset-current-name'],

    // ── D-01 arm 3: a node that otherwise holds backend-supplied data ──────
    ['12-TET Standard',
     'the fallback written into #scale-name-display, whose every other value comes '
     + 'from TuningEngine::getScaleName() — an .scl filename, a library entry, or a '
     + 'generated name. D-01 arm 3: keying a node that holds data would make it '
     + 'enter and leave the language sweep as the scale changes',
     '#scale-name-display'],

    // ── D-01 arm 1: AudioParameterChoice options, byte-identical ───────────
    // Verified against the RUNTIME parameter dump (173 parameters,
    // .planning/params.tsv) and the StringArrays in PluginProcessor.cpp.
    // Localizing one would make the page and the host automation lane disagree
    // about what the user just selected.
    ...[
        ['Off',        'oscA/BWarpType + glideMode option (PluginProcessor.cpp:109,247)'],
        ['Sync',       'oscA/BWarpType option (PluginProcessor.cpp:109)'],
        ['Bend',       'oscA/BWarpType option (PluginProcessor.cpp:109)'],
        ['FM',         'oscA/BWarpType option (PluginProcessor.cpp:109)'],
        ['Window',     'oscA/BWarpType option (PluginProcessor.cpp:109)'],
        ['-1 Oct',     'subOctave option (PluginProcessor.cpp:126)'],
        ['-2 Oct',     'subOctave option (PluginProcessor.cpp:126)'],
        ['-3 Oct',     'subOctave option (PluginProcessor.cpp:126)'],
        ['-4 Oct',     'subOctave option (PluginProcessor.cpp:126)'],
        ['Post-Filter', 'subRouting option (PluginProcessor.cpp:138)'],
        ['Pre-Filter', 'subRouting option (PluginProcessor.cpp:138)'],
        ['White',      'noiseType option (PluginProcessor.cpp:132)'],
        ['Pink',       'noiseType option (PluginProcessor.cpp:132)'],
        ['Brown',      'noiseType option (PluginProcessor.cpp:132)'],
        ['Digital',    'noiseType option (PluginProcessor.cpp:132)'],
        ['Vinyl',      'noiseType option (PluginProcessor.cpp:132)'],
        ['Legato',     'glideMode option (PluginProcessor.cpp:247)'],
        ['Always',     'glideMode option (PluginProcessor.cpp:247)'],
        ['LP12',       'filtA/BType option (PluginProcessor.cpp:197)'],
        ['LP24',       'filtA/BType option (PluginProcessor.cpp:197)'],
        ['HP12',       'filtA/BType option (PluginProcessor.cpp:197)'],
        ['HP24',       'filtA/BType option (PluginProcessor.cpp:197)'],
        ['BP12',       'filtA/BType option (PluginProcessor.cpp:197)'],
        ['BP24',       'filtA/BType option (PluginProcessor.cpp:197)'],
        ['Notch',      'filtA/BType option (PluginProcessor.cpp:197)'],
        ['Serial',     'filtRouting option (PluginProcessor.cpp:220)'],
        ['Parallel',   'filtRouting option (PluginProcessor.cpp:220)'],
        ['Normal',     'delayMode option (PluginProcessor.cpp:305)'],
        ['PingPong',   'delayMode option (PluginProcessor.cpp:305)'],
        ['SoftClip',   'distType option (PluginProcessor.cpp:340)'],
        ['HardClip',   'distType option (PluginProcessor.cpp:340)'],
        ['Tube',       'distType option (PluginProcessor.cpp:340)'],
        ['Fold',       'distType option (PluginProcessor.cpp:340)'],
    ].map(([t, where]) => [t,
        'a parameter dropdown option reproduced BYTE-IDENTICALLY from ' + where
        + ' — D-01 arm 1: the page and the host automation lane must agree',
        '.param-select']),

    // ── The wavetable catalogue: 28 option texts + their optgroup labels ────
    // Not arm 1 — `oscATable` is an AudioParameterInt (0..27, host text "0".."27"),
    // so there is no choice option to match. They are exempt for two other
    // reasons, and either alone would be enough:
    //   1. They are a hand-mirrored copy of WavetableFactory::getTableInfoList()
    //      (WavetableFactory.cpp:93-130), a C++-owned catalogue of factory
    //      content NAMES. The optgroup labels mirror its category column.
    //   2. Four of them — Saw, Square, Triangle, Sine — are byte-identical to
    //      subShape and lfoNShape options that ARE arm 1. Translating the osc
    //      list would make the same word French in one dropdown on the page and
    //      English in the next.
    ...[
        'Saw', 'Square', 'Triangle', 'Sine', 'PWM Sweep', 'Supersaw', 'Sync Sweep',
        'FM E.Piano', 'FM Bell', 'FM Metallic', 'Wavefold', 'Bitcrush',
        'Vowel Morph', 'Choir Pad', 'Vocal Lead', 'Formant Filter',
        'Harmonic Series', 'Spectral Tilt', 'Odd Harmonics', 'Harmonic Stretch',
        'Comb Sweep', 'Prism Spectrum',
        'Breath', 'Plucked String', 'Church Bell', 'Organ Sweep', 'Wind', 'Filtered Noise',
    ].map((t) => [t,
        'a factory wavetable NAME, mirrored from WavetableFactory::getTableInfoList() '
        + '(WavetableFactory.cpp:93-130). Its parameter is an AudioParameterInt, so '
        + 'arm 1 does not reach it; it is exempt as C++-owned content, and four of the '
        + '28 are byte-identical to arm-1 subShape/lfoNShape options on the same page',
        '.param-select']),

    // ── The 37 modulation matrix names ─────────────────────────────────────
    // getModSourceNames() / getModDestNames() (dsp/ModulationMatrix.h:86-101)
    // feed BOTH the page (over the getModSourceNames / getModDestNames native
    // fns, PluginEditor.cpp) and the mod-slot AudioParameterChoice options
    // (PluginProcessor.cpp:415-427). Byte-identical BY CONSTRUCTION — D-01 arm 1.
    //
    // Scoped even though they are injected rather than authored, because `Osc Mix`
    // is ALSO the footer caption `label.oscMix`: unscoped, this entry would
    // silence a live keyed node and assertion 14 could not tell a deliberate
    // skip from a forgotten label.
    ...[
        'None', 'LFO1', 'LFO2', 'LFO3', 'LFO4', 'AmpEnv', 'FilterEnv',
        'Velocity', 'NoteNum', 'ModWheel', 'Aftertouch',
        'OscA Pos', 'OscB Pos', 'FiltA Cut', 'FiltB Cut', 'FiltA Res', 'FiltB Res',
        'Osc Mix', 'Sub Level', 'Noise Level',
        'LFO1 Rate', 'LFO2 Rate', 'LFO3 Rate', 'LFO4 Rate',
        'OscA Detune', 'OscB Detune', 'OscA Pan', 'OscB Pan',
        'Reverb Mix', 'Delay Mix', 'Chorus Mix', 'Dist Mix', 'Master Vol', 'Pitch',
        'OscA Warp', 'OscB Warp',
    ].map((t) => [t,
        'a modulation source/destination name from dsp/ModulationMatrix.h:86-101, which '
        + 'builds the mod-slot AudioParameterChoice options in PluginProcessor.cpp:415-427 '
        + 'and is pushed to the page over the same two functions — byte-identical by '
        + 'construction, D-01 arm 1',
        '#mod-matrix-rows']),

    // ── D-01 arm 1, written from script ────────────────────────────────────
    ['Custom',
     'written into #scale-name-display when an interval is hand-edited, and '
     + 'byte-identical to the last tuningPreset choice option '
     + '(PluginProcessor.cpp:231-233) — D-01 arm 1. It is also arm 3: the node it '
     + 'lands in holds backend-supplied scale names'],

    // ── Language endonyms ──────────────────────────────────────────────────
    ['English',  'an endonym — a language name is never translated'],
    ['Français', 'an endonym — a language name is never translated'],
];

// The canon imports tr() alongside the tables, and as of v1.22.0 applyI18n()
// really does call it — once per TIP_BINDINGS row, on every language change, to
// resolve the title and body it writes onto each anchor. The canon block is ONE
// shape across all 43 plugins and is not trimmed per plugin, and an import of a
// name this module does not export throws at module evaluation and takes the
// whole UI down (pattern_module_toplevel_init_tdz).
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
