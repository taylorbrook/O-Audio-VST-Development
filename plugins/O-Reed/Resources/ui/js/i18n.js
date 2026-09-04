/*
   This file is part of O-Reed, an Ouaricon Audio plugin.
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
// i18n.js — O-Reed page labels and hover-help, English + French (v1.3.1)
//
// ── v1.3.1: FRENCH QA PASS (Stage N, 2026-08-31) ────────────────────────────
// Every fr entry read against its en and against scripts/i18n-fr-glossary.js.
// Changed: 42 entries of 90 (2 terminology, 26 typography, 2 grammar/agreement,
// 4 idiom/register, 3 meaning, 5 termNote-only). sameAsEn: kept 6, translated 0.
// termNote exemptions: 6 (listed below). Left as drafted: the other 48.
// reviewed: false throughout — no native speaker yet.
//
// Lint 53 -> 0, --strict exit 0 (6 G1, 3 F1, 4 T3, 20 T4, 11 T5, 9 T7 closed).
// 59 U+00A0 inserted (2 -> 61), all inside a `t:`/`b:` string value of an `fr:`
// object; the pass was a brace-matched scope over the fr objects with comments
// masked out, and the control that both `en` sub-objects, all 90 keys,
// I18N_EXEMPT and TIP_BINDINGS are byte-identical was run by IMPORTING both
// revisions, not by reading the diff.
//
// DECISIONS THE NEXT READER NEEDS:
//
//  1. THE 68px CAP WAS RE-MEASURED, and v1.3.0's header was RIGHT twice. Anche
//     double is 67.20 px and Flatterzunge 67.36 px against the 68.00 px
//     .knob-label max-width — 0.80 and 0.64 px, tighter than the 1.02 px that
//     made v1.3.0 reject "Dureté anche", and .knob-label carries
//     text-overflow: ellipsis, so the failure mode is a silent truncation on a
//     Windows/WebView2 font metric. Both keep their abbreviation with a
//     termNote carrying the number.
//  2. CLIFF 4 CLAIMED A THIRD CAPTION, and the gate proved it rather than the
//     header arguing it. The glossary form "Prof. vibr." (50.08 px) was shipped
//     into check-ui-labels and [8b] FAILED: vibratoDepth is FIRST in its row,
//     centred at x 57.00, and the collapsed Expression section keeps its
//     rectangle over the next section-header, whose chevron ends at x 33.00 —
//     so anything wider than 48.00 px intersects it. Root "Prof. vibrato" is
//     64.80 and the listed "Prof. vibr" is 47.63 (0.19 px clear). Vib Prof.
//     stays, and Vib Vit. stays WITH it: the two are one matched pair on one
//     row, and "Vit. vibr." fits perfectly well on its own (41.94 px, 17 px
//     clear) — applying the glossary to only the half that fits would put the
//     head noun on opposite sides of two adjacent captions.
//  3. Amortis. -> Amort. and Réinject. -> Réinjection are the two captions the
//     glossary MOVED. Amortissement itself is 74.27 px and clips outright;
//     Réinjection is 58.06 px with 9.94 px of the cap and clears every
//     neighbour in the collapsed state, so the ROOT ships.
//  4. Flatt. is a MEANING exemption before it is a width one: the parameter is
//     flutterTongue, so the glossary row that applies is `flutter tongue ->
//     Flatterzunge`, not `flutter -> Scintillement` (a tape artefact, and
//     69.47 px, which clips). Tenue inf. / Tenue infinie are meaning
//     exemptions too — O-Reed has NO ADSR and no sustain-level parameter
//     anywhere, so Maintien would name a control that does not exist.
//  5. ONE CONTROL HAD TWO FRENCH NAMES (the O-Comp N1 finding). The dropdown
//     caption read "Mode polyphonique" while its own tip title and
//     tip.maxVoices' body both said "Mode de polyphonie". The caption moved to
//     "Mode de polyphonie" — it sits in a >=662 px block and is not width-
//     constrained. The mirror case went the other way: tip.polyMode's body
//     said "Voix maximales" where the knob it points at reads "Voix max", so
//     the BODY moved, restoring the English pattern where a body names a
//     control by the caption the user is looking at.
//  6. THE LOANWORDS STAY. Growl, Subtone, Embouchure, Instrument, FX and
//     Expression are the six sameAsEn captions and every one is a word French
//     audio uses unchanged; none was translated. tip.embouchure and tip.subtone
//     carry an English-equal TITLE over a translated body and take NO flag
//     (N3 correction 26 — the flag is entry-scoped and would disarm check-i18n
//     assertion 4 for the body).
//  7. ONE TIP GREW A LINE: tip.embouchure 136.5 -> 153.2 px, because the French
//     had dropped the English's "rest opening" and had a dangling gérondif.
//     It is not floored (136.8 px of bottom clearance) and the page's tallest
//     tip is unchanged at 186.5 px. A second improvement was REVERTED for the
//     same reason O-Texture reverted "appuyer dessus" in N2: tip.revBore's
//     wording was taste, not a defect, and it pushed that tip onto the
//     renderer's bottom FLOOR — the top edge moved 409.9 -> 393.2 px while the
//     bottom clearance stayed pinned at 36.9, i.e. the growth went UPWARD over
//     the controls with inFrame still green (N3 correction 30).
//  8. NOTHING HERE TOUCHES THE TUNING TAB. Unlike the other seven plugins that
//     carry scala-tuning-engine panel copies, O-Reed keeps the whole tab in
//     I18N_EXEMPT as shared-module territory, so there were no tuning strings
//     to converge on the settled forms. #ref-pitch-knob is module-owned and was
//     not touched.
//
// An ES module that EXPORTS ONLY. It must never self-execute: a bare top-level
// statement here throws out of module evaluation and takes every later
// initializer on the page with it (pattern_module_toplevel_init_tdz). This
// plugin's controller is ONE inline <script type="module"> in index.html — the
// O-Bassoon / O-Bitrot shape, not the O-Tapestop js/app.js one — so that
// failure mode would take the WHOLE UI, not a panel of it. check-i18n
// assertion 7 enforces the export-only rule.
//
// FILENAME: no hyphen. juce_add_binary_data STRIPS hyphens rather than
// converting them to underscores, so a second file named i18n-fr.js would have
// to be reached as the symbol i18nfr_js (critical_binary_data_strips_hyphens).
// One combined file for both languages sidesteps the question entirely.
//
// ── v1.2.0 HAD NO HOVER-HELP; v1.3.0 AUTHORS IT ────────────────────────────
//
// v1.1.0 carried NO data-tip, NO data-tooltip and — measured, not assumed —
// ZERO native title=, aria-label=, placeholder= and alt= attributes anywhere in
// index.html. The measured inventory's "0 attributes" column is correct, which
// is worth saying out loud because it is the one column that is a claim about
// absence. There was therefore nothing for contract §4 to delete and nothing to
// move into data-i18n-aria, and NO NEW PROSE WAS INVENTED into v1.2.0. The two
// aria keys below belong to the two elements v1.2.0 ADDED (the gear button and
// the language selector) and match the O-Bassoon precedent verbatim.
//
// v1.3.0 authors 35 tooltips — 33 parameters plus the gear and the language
// selector — AND the renderer that paints them, in one commit, because the
// canon writes the tip ATTRIBUTES and nothing else. See the I18N and
// TIP_BINDINGS headers below. Native title= is still ZERO after the renderer
// lands; contract §4 forbids reintroducing one and tests/ui_tip_render_check.js
// asserts it.
//
// COPY IS textContent ON EVERY PATH — never innerHTML. check-i18n assertion 9
// rejects any innerHTML reference here and any string literal containing an
// angle bracket, so machine-drafted French cannot open a markup path. The one
// innerHTML site the controller had (the tuning-panel load-failure notice) was
// rebuilt with createElement + setLabel in the same commit.
//
// ALL FRENCH IS MACHINE-DRAFTED AND FLAGGED `reviewed: false`. No native
// speaker has read it. `node scripts/check-i18n.js` prints the worklist.
// ============================================================================

export const LANGUAGES = ['en', 'fr'];

// ============================================================================
// I18N — hover-help copy. {en:{t,b}, fr:{t,b,reviewed}}: a title and a body.
//
// AUTHORED IN v1.3.0. Until then this table was empty and TIP_BINDINGS was
// empty with it, which was this plugin's correct state rather than a gap:
// v1.2.0 carried no data-tip anywhere and — measured — no native title=
// either, so there was nothing to migrate and nothing to delete.
//
// ── 35 ENTRIES: 33 PARAMETERS + 2 CHROME. THE DUMP SAYS 35 PARAMETERS ───────
//
// .planning/params.tsv is the runtime walk of AudioProcessor::getParameters()
// on a constructed processor, not a regex over createParameterLayout(), and it
// reports 35. TWO of them have no control on this page and therefore get no
// tip, because a body nothing binds is an ORPHAN and fails check-i18n
// assertion 2:
//
//   referencePitch  AudioParameterFloat, 220..880 Hz. Its control EXISTS but it
//                   belongs to the SHARED module — #ref-pitch-knob in
//                   modules/tuning/scala-tuning-engine/js/tuning-panel.js —
//                   which is lazy-mounted on the first Tuning-tab activation
//                   and is absent from the DOM when applyI18n() first runs.
//                   Binding it would log `tip target not found` on every load,
//                   which boot-all-uis reports, and editing the module to add
//                   an anchor is a cross-plugin change reverted by
//                   /module-upgrade. index.html's own bindSliderParam()
//                   already warns `No control for: referencePitch` at load for
//                   the same reason (index.html:1273).
//   tuningSystem    AudioParameterChoice, Scala/TUN | MTS-ESP | 12-TET. It has
//                   NO control anywhere: bindComboBox('tuningSystem',
//                   'tuningSystem') (index.html:1744) resolves
//                   getElementById('tuningSystem') to null and warns `No select
//                   for: tuningSystem` on every load. Host-reachable and
//                   automatable; not page-reachable in any version.
//
// Both are reported rather than fixed. Adding a control to satisfy the count is
// a feature change with a geometry cost, which this stage does not have.
//
// ── THE TITLE IS THE PARAMETER'S FULL NAME, NOT THE PAGE'S CAPTION ──────────
//
// This REVERSES the stage brief's default ("where the page's caption differs
// from the parameter's name, the caption wins"). That rule exists so the
// tooltip names what the user is reading, and on this page it would do the
// opposite: all 27 knob captions were cut to fit the 68px `.knob-control` box
// measured in v1.2.0, and the cut is visible — `Reed Hard.`, `Inf. Sustain`,
// `Rev. Bore` each end in a truncating period, and `Vib Depth`, `Vib Rate`,
// `Chiff`, `Character`, `Diameter`, `Length`, `Opening`, `Mass`, `Damping`,
// `Mouthpiece`, `Register`, `Tone Hole`, `Growl`, `Flutter`, `Feedback` and
// `Output` are each a head word lifted out of a longer name. A 260px tooltip is
// exactly where `Reed Hard.` becomes `Reed Hardness`. Repeating the
// abbreviation into a box with room for the word teaches nothing, and the full
// form is also what the host's automation lane shows, so page and lane agree.
// The French titles are the full French forms of the same names, which is why
// they are longer than the LABELS captions below them.
//
// ── WHERE THE RANGES COME FROM ──────────────────────────────────────────────
//
// Every body ends with the range and unit. FIVE of the 35 dump rows carry a
// non-empty `label`: toneHoleCutoff (Hz), vibratoRate (Hz), dronePitch (cents),
// referencePitch (Hz, no control) and outputGain (dB). The other 30 are EMPTY,
// so 24 of the 33 authored ranges were recovered from THE PAGE'S OWN FORMATTER
// rather than invented — the PARAMS table at index.html:1114-1143 and
// formatValue() at index.html:1260-1264, which is
// `rawValue.toFixed(def.decimals) + def.unit`:
//
//   23 knobs   unit: '', decimals: 2  ->  renders `0.50`  ->  unitless, "0 to 1"
//   maxVoices  unit: '', decimals: 0  ->  renders `8`     ->  a count, "1 to 16 voices"
//
// The remaining 6 authored ranges are the option words of the five Choice
// parameters and the one Bool, which are not numbers at all.
//
// ONE DUMP/PAGE DIVERGENCE, deliberate and not a defect: dronePitch's APVTS
// label is `cents` while the page's readout renders ` ct` (index.html:1138).
// The body spells the unit out, because a tooltip is where a user learns what
// the abbreviation on the readout stands for. Same reasoning as O-Bassoon's
// vibrato_depth in batch M1.
//
// ── D-01 ARM 1 INSIDE A TOOLTIP BODY, AND WHAT WAS DECIDED ──────────────────
//
// Six parameters are AudioParameterChoice (five with a control) plus one
// AudioParameterBool, so their option strings are D-01 arm 1 EXEMPT ON THE PAGE
// — the `<select>` must name them byte-for-byte as the host's automation lane
// does. A tooltip body is prose and prose is localized, and the two rules do
// not conflict, but they leave one thing to decide per entry: is the option
// TOKEN inside a French sentence translated, or reproduced verbatim?
//
// DECIDED: THE PROSE IS FRENCH, THE TOKEN IS VERBATIM ENGLISH, in guillemets.
// The rule is "match what the page shows", which is the same rule the two M1
// precedents follow from opposite sides: O-Comp's body says ARRÊT / MARCHE
// because its on-page faces are LABELS entries and ARE localized, and
// O-Texture's body says "Rain, Metal, Wind, Crowd, Synth, Organic" because its
// on-page buttons are not. On O-Reed the six `<select>` elements are EMPTY in
// index.html and are filled at runtime from the host's own choice properties
// (bindComboBox, index.html:1350-1372), so what the user reads 20px from the
// tooltip is the English option string. Translating `Multi-segment` inside the
// French body would name a value that appears nowhere on the control.
//
// The knob captions are the opposite case and are handled the opposite way: a
// French body naming another knob calls it by its LOCALIZED caption (Caractère
// de la perce, Voix maximales, Double perce), because those ARE keyed and the
// page shows the French.
//
// ── THE BODIES DESCRIBE THIS MODEL, NOT A GENERIC SYNTH ─────────────────────
//
// This is a reed/bore waveguide, so most of these knobs name a physical
// quantity rather than a familiar effect control, and the tooltip is the only
// place a user can find out what one does. Each body was written against the
// DSP that implements it — ReedModel.h, BoreWaveguide.h, BreathEnvelope.h,
// BreathNoise.h, MouthpieceChamber.h and ReedWindVoice.cpp — rather than from
// the parameter's name, and each states something a user cannot discover by
// turning the knob. The measured mappings behind the numbers in the copy:
//
//   reedHardness    k_r  2e6 .. 20e6 N/m^3          ReedModel.h:79
//   reedOpening     H    0.1mm .. 1.5mm             ReedModel.h:82
//   reedMass        mu_r 1e-4 .. 0.06 kg/m^2        ReedModel.h:73
//   reedDamping     g_r  500 .. 6000 s^-1           ReedModel.h:76
//   boreDiameter    throat radius 2mm .. 20mm       BoreWaveguide.h:163
//   boreLength      0.2m .. 1.5m                    BoreWaveguide.h:166
//   bellSize        bell cutoff 800Hz .. 6000Hz     BoreWaveguide.h:210
//   toneHoleCutoff  200Hz = ALL FOUR HOLES OPEN     BoreWaveguide.h:243-250
//                   8000Hz = all four CLOSED         (the reading is inverted)
//   mouthpieceVol   0 .. 15 cm^3 Helmholtz          ReedWindVoice.cpp:432
//   growlAmount     120Hz sine, +/-30% on p_mouth   ReedWindVoice.cpp:526-531
//   flutterTongue   25Hz half-wave, -40% on p_mouth ReedWindVoice.cpp:534-540
//   subtone         +0.3 air, +0.3 emb, -30% press  ReedWindVoice.cpp:494-510
//   attackChiff     overshoot 1 + chiff*vel*0.3     BreathEnvelope.h:60
//   velocity        attack 50ms (v=0) .. 5ms (v=1)  BreathEnvelope.h:55
//   feedbackPath    blend up to 0.5 of bore2        ReedWindVoice.cpp:529-532
//   infiniteSustain bell -> Nyquist, visc g -> 1.0  BoreWaveguide.h:214,235
//   reverseBore     segment centres 0.25/0.75 swap  BoreWaveguide.h:190
//   boreProfile     taper ratios 1.0/1.0 -> 0.5/1.5 BoreWaveguide.h:172-174
//   vibratoSource   Lip +/-0.15 emb, Breath +/-10%  ReedWindVoice.cpp:518-522
//                   p_mouth, Throat +/-3% bore scale
//
// ── ONE BODY REPORTS A DEAD CONTROL, DELIBERATELY ───────────────────────────
//
// tip.instrumentPreset says the dropdown does not change the sound, because it
// does not. `pInstrumentPreset` is fetched in ReedWindVoice.cpp:50 and is never
// load()ed anywhere in Source/ — the parameter has no consumer. A tip that lies
// is worse than no tip, and O-Texture's tip.source is the M1 precedent for
// saying so in the copy. Reported as a defect and NOT fixed here: wiring it is
// an audio change, not an i18n one.
//
// ALL FRENCH IS MACHINE-DRAFTED AND FLAGGED `reviewed: false`. No native
// speaker has read it. French bodies take FRENCH convention — decimal COMMA, a
// space before %, U+2212 for the minus — while the readout NODES keep their
// point, because D-03 exempts the readout and a body is prose. Settled
// repo-wide on 2026-08-30.
// ============================================================================

export const I18N = Object.freeze({

    // ── The XY pad's dropdown ───────────────────────────────────────────────
    //
    // THE DEAD CONTROL. See the note above. The 21 option strings are verbatim
    // English in both bodies (the `<select>` is filled from the host's choice
    // properties and shows them in English), and the two named here are the
    // dump's textAtMin and textAtMax.
    'tip.instrumentPreset': {
        en: { t: 'Instrument Preset',
              b: 'Records which of the twenty-one instruments this patch is aiming at, and it rides with the session and with the host’s automation lane. It moves no other control and no part of the sound reads it: the instrument is shaped by the pad above and by the bore and reed knobs below. Bb Clarinet to Impossible Bore, 21 choices.' },
        fr: { t: 'Préréglage d’instrument',
              b: 'Indique lequel des vingt-et-un instruments ce patch vise ; la valeur est conservée avec la session et exposée à l’automation de l’hôte. Elle ne déplace aucune autre commande et aucune partie du son ne la lit : l’instrument se façonne avec le pavé ci-dessus et avec les boutons de perce et d’anche en dessous. De « Bb Clarinet » à « Impossible Bore », 21 choix.',
              reviewed: true },
    },

    // ── Primary Controls ────────────────────────────────────────────────────
    'tip.breath': {
        en: { t: 'Breath Pressure',
              b: 'Mouth pressure on the reed, and the main dynamic control here: at zero the reed never beats and the note does not speak, and raising it opens and brightens the tone. Note velocity shortens the attack on top of this, from 50 ms down to 5 ms, so the same setting speaks differently under a hard strike. 0 to 1.' },
        fr: { t: 'Pression du souffle',
              b: 'La pression de la bouche sur l’anche, et le principal réglage de nuance ici : à zéro l’anche ne bat pas et la note ne parle pas, et plus elle monte, plus le son s’ouvre et s’éclaircit. La vélocité raccourcit l’attaque par-dessus, de 50 ms à 5 ms, si bien que le même réglage parle autrement sous une frappe forte. 0 à 1.',
              reviewed: true },
    },

    // The French body names the Lip option verbatim (arm 1, the token the
    // `<select>` shows) and Source du vibrato in French (a keyed caption).
    'tip.embouchure': {
        en: { t: 'Embouchure',
              b: 'Lip force on the reed: raising it stiffens the reed, damps it and closes its rest opening, so the tone brightens, the pitch lifts a little and the note is harder to start. It is also the target the Lip setting of Vibrato Source modulates. 0 to 1.' },
        fr: { t: 'Embouchure',
              b: 'La force des lèvres sur l’anche : plus elle augmente, plus l’anche se raidit, s’amortit et referme son ouverture au repos, donc le son s’éclaircit, la hauteur monte un peu et la note démarre plus difficilement. C’est aussi la cible que module le réglage « Lip » de Source du vibrato. 0 à 1.',
              reviewed: true },
    },

    'tip.reedHard': {
        en: { t: 'Reed Hardness',
              b: 'Stiffness of the reed, from a soft one to a hard one. A soft reed speaks easily and stays dark under pressure; a hard reed needs more breath, takes longer to start and holds its brightness in the loud dynamics. 0 to 1.' },
        fr: { t: 'Dureté de l’anche',
              b: 'La raideur de l’anche, d’une anche souple à une anche dure. Une anche souple parle facilement et reste sombre sous la pression ; une anche dure demande plus de souffle, met plus de temps à démarrer et garde sa brillance dans les nuances fortes. 0 à 1.',
              reviewed: true },
    },

    'tip.output': {
        en: { t: 'Output Gain',
              b: 'Master level, applied after the bore, the radiation filter and the output limiter. It is the last stage, so it cannot be used to drive the model harder — Breath Pressure is the control for that. −60.0 to +12.0 dB.' },
        fr: { t: 'Gain de sortie',
              b: 'Le niveau général, appliqué après la perce, le filtre de rayonnement et le limiteur de sortie. C’est le dernier étage : il ne permet pas de pousser le modèle plus fort — Pression du souffle est la commande faite pour cela. −60,0 à +12,0 dB.',
              reviewed: true },
    },

    // ── Bore & Resonance ────────────────────────────────────────────────────
    'tip.character': {
        en: { t: 'Bore Character',
              b: 'Morphs the bore from cylindrical to conical, and it is the X axis of the pad above. A cylinder resonates at a quarter wavelength and sounds odd harmonics only — the hollow clarinet register; a cone resonates at a half wavelength and sounds the whole series, the saxophone and oboe end. 0 to 1.' },
        fr: { t: 'Caractère de la perce',
              b: 'Fait passer la perce du cylindre au cône, et c’est l’axe X du pavé ci-dessus. Un cylindre résonne au quart d’onde et ne sonne que les harmoniques impaires — le registre creux de la clarinette ; un cône résonne à la demi-onde et sonne toute la série, du côté du saxophone et du hautbois. 0 à 1.',
              reviewed: true },
    },

    'tip.diameter': {
        en: { t: 'Bore Diameter',
              b: 'Width of the bore at the throat, from about 2 mm to 20 mm. A narrow bore loses more high frequency to wall friction and sounds darker and more resistant; a wide one is brighter and blows freer. 0 to 1.' },
        fr: { t: 'Diamètre de la perce',
              b: 'La largeur de la perce à la gorge, d’environ 2 mm à 20 mm. Une perce étroite perd davantage d’aigu par frottement aux parois et sonne plus sombre et plus résistante ; une perce large est plus claire et se joue plus librement. 0 à 1.',
              reviewed: true },
    },

    'tip.bellSize': {
        en: { t: 'Bell Size',
              b: 'Flare of the bell, which sets the frequency above which the bore stops reflecting and starts radiating — 800 Hz at the bottom of the range, 6 kHz at the top. A small bell keeps the energy inside the tube and sounds contained; a large one projects and brightens. 0 to 1.' },
        fr: { t: 'Taille du pavillon',
              b: 'L’évasement du pavillon, qui fixe la fréquence au-dessus de laquelle la perce cesse de réfléchir et se met à rayonner — 800 Hz en bas de la course, 6 kHz en haut. Un petit pavillon garde l’énergie dans le tube et sonne contenu ; un grand projette et éclaircit. 0 à 1.',
              reviewed: true },
    },

    'tip.length': {
        en: { t: 'Bore Length',
              b: 'Effective length of the tube, about 20 cm to 1.5 m. The played pitch does not follow it — that comes from the keyboard and from the tuning system — this sets how far the cone has flared along the tube, so it colours the tone instead of transposing it, and it does nothing at all while Bore Character is fully cylindrical. 0 to 1.' },
        fr: { t: 'Longueur de la perce',
              b: 'La longueur utile du tube, d’environ 20 cm à 1,5 m. La hauteur jouée ne la suit pas — elle vient du clavier et du système d’accord — ce réglage fixe l’évasement du cône le long du tube : il colore le timbre au lieu de transposer, et il ne fait rien tant que Caractère de la perce est entièrement cylindrique. 0 à 1.',
              reviewed: true },
    },

    // The one knob on this page whose reading is INVERTED, which is exactly the
    // thing a user cannot discover by turning it.
    'tip.toneHole': {
        en: { t: 'Tone Hole Cutoff',
              b: 'Opens and closes four tone holes together, and it reads backwards from a filter cutoff: at 200 Hz all four are open and the tone is dark and vented, at 8000 Hz all four are closed and the bore rings its full length. They open one at a time as the value falls, so the change arrives in steps rather than smoothly. 200 to 8000 Hz.' },
        fr: { t: 'Coupure des trous de jeu',
              b: 'Ouvre et ferme quatre trous de jeu ensemble, et la lecture est inversée par rapport à une fréquence de coupure : à 200 Hz les quatre sont ouverts et le son est sombre et aéré, à 8000 Hz les quatre sont fermés et la perce sonne sur toute sa longueur. Ils s’ouvrent un par un à mesure que la valeur descend : le changement arrive par paliers. 200 à 8000 Hz.',
              reviewed: true },
    },

    'tip.register': {
        en: { t: 'Register Hole',
              b: 'Opens the register hole — the small vent a player uses to overblow into the upper register. It is a narrower junction than the tone holes, so it thins the fundamental and lets the second mode take over rather than simply darkening the tone. 0 to 1.' },
        fr: { t: 'Trou de registre',
              b: 'Ouvre le trou de registre — le petit évent dont le joueur se sert pour passer dans le registre supérieur. C’est une jonction plus étroite que les trous de jeu : il affaiblit le fondamental et laisse le deuxième mode prendre le dessus au lieu de simplement assombrir le son. 0 à 1.',
              reviewed: true },
    },

    // Both option tokens verbatim English in both languages: the `<select>`
    // beside this tooltip is filled from the host's choice properties.
    'tip.boreProfile': {
        en: { t: 'Bore Profile',
              b: 'Chooses how the cone’s taper is spread along the tube: Simple gives one even taper, and Multi-segment narrows the first half and flares the second, which is closer to a real saxophone and puts weight in the low register. Neither does anything while Bore Character is fully cylindrical. Simple or Multi-segment.' },
        fr: { t: 'Profil de perce',
              b: 'Choisit la répartition de la conicité le long du tube : « Simple » donne une conicité uniforme, « Multi-segment » resserre la première moitié et évase la seconde, ce qui se rapproche d’un vrai saxophone et donne du corps au grave. Ni l’un ni l’autre n’agit tant que Caractère de la perce est entièrement cylindrique. « Simple » ou « Multi-segment ».',
              reviewed: true },
    },

    // ── Reed ────────────────────────────────────────────────────────────────
    'tip.opening': {
        en: { t: 'Reed Opening',
              b: 'How far the reed sits off the lay when nothing is blowing, from 0.1 mm to 1.5 mm. A wide opening gives a large dynamic range but needs more breath to close; a narrow one speaks at a whisper and saturates early. 0 to 1.' },
        fr: { t: 'Ouverture de l’anche',
              b: 'L’écart entre l’anche et la table quand rien ne souffle, de 0,1 mm à 1,5 mm. Une grande ouverture donne une large dynamique mais demande plus de souffle pour se fermer ; une petite ouverture parle dans un murmure et sature tôt. 0 à 1.',
              reviewed: true },
    },

    'tip.mass': {
        en: { t: 'Reed Mass',
              b: 'Mass per unit area of the reed. A light reed follows the air almost instantly and gives a clean, fast onset; a heavy one lags, rings at its own resonance and puts a thicker, slower transient in front of every note. 0 to 1.' },
        fr: { t: 'Masse de l’anche',
              b: 'La masse par unité de surface de l’anche. Une anche légère suit l’air presque instantanément et donne une attaque nette et rapide ; une anche lourde traîne, résonne à sa propre fréquence et place un transitoire plus épais et plus lent devant chaque note. 0 à 1.',
              reviewed: true },
    },

    'tip.damping': {
        en: { t: 'Reed Damping',
              b: 'How quickly the reed’s own vibration dies away. Low values let the reed ring alongside the bore and add a reedy edge; high values mute it, and the tone is then driven almost entirely by the tube. 0 to 1.' },
        fr: { t: 'Amortissement de l’anche',
              b: 'La vitesse à laquelle la vibration propre de l’anche s’éteint. Des valeurs basses laissent l’anche résonner en même temps que la perce et ajoutent un mordant d’anche ; des valeurs hautes l’étouffent, et le son est alors porté presque entièrement par le tube. 0 à 1.',
              reviewed: true },
    },

    'tip.doubleReed': {
        en: { t: 'Double Reed',
              b: 'Moves the reed from single to double, and it is the Y axis of the pad above. A double reed confines the airflow between two blades instead of a blade and a lay, which narrows the opening and brings in the buzz and the compressed dynamics of the oboe and duduk family. 0 to 1.' },
        fr: { t: 'Anche double',
              b: 'Fait passer l’anche de simple à double, et c’est l’axe Y du pavé ci-dessus. Une anche double confine le flux entre deux lames au lieu d’une lame et d’une table : l’ouverture se resserre, et l’on gagne le grain et la dynamique resserrée de la famille du hautbois et du duduk. 0 à 1.',
              reviewed: true },
    },

    'tip.mouthpiece': {
        en: { t: 'Mouthpiece Volume',
              b: 'Volume of the mouthpiece chamber, from nothing to about 15 cm³. At zero the chamber is bypassed entirely and the reed drives the bore directly; opening it adds a Helmholtz compliance that flattens the upper register and rounds the attack. 0 to 1.' },
        fr: { t: 'Volume du bec',
              b: 'Le volume de la chambre du bec, de rien à environ 15 cm³. À zéro la chambre est court-circuitée et l’anche attaque la perce directement ; en l’ouvrant, on ajoute une compliance de Helmholtz qui baisse le registre aigu et arrondit l’attaque. 0 à 1.',
              reviewed: true },
    },

    // ── Expression ──────────────────────────────────────────────────────────
    'tip.vibDepth': {
        en: { t: 'Vibrato Depth',
              b: 'Depth of the vibrato. Where it lands is set by Vibrato Source, so one depth can read as a pitch bend, a swell or a throat flutter — and at zero the modulator is skipped entirely rather than running silently. 0 to 1.' },
        fr: { t: 'Profondeur du vibrato',
              b: 'La profondeur du vibrato. Sa cible est fixée par Source du vibrato : à profondeur égale, il peut s’entendre comme une inflexion de hauteur, un gonflement ou un battement de gorge — et à zéro le modulateur est court-circuité plutôt que de tourner en silence. 0 à 1.',
              reviewed: true },
    },

    'tip.vibRate': {
        en: { t: 'Vibrato Rate',
              b: 'Speed of the vibrato. Wind players sit between about 4 and 7 Hz — below that it reads as a swell, above it as a bleat — and the phase restarts at every note, so repeated notes vibrate identically. 1.0 to 10.0 Hz.' },
        fr: { t: 'Vitesse du vibrato',
              b: 'La vitesse du vibrato. Les vents se tiennent entre 4 et 7 Hz environ — plus lent, cela s’entend comme un gonflement, plus rapide comme un chevrotement — et la phase repart à chaque note, si bien que des notes répétées vibrent à l’identique. 1,0 à 10,0 Hz.',
              reviewed: true },
    },

    // Arm 1 again: all three option tokens verbatim English inside the French.
    'tip.vibratoSource': {
        en: { t: 'Vibrato Source',
              b: 'Chooses what the vibrato modulates. Lip moves the embouchure and reads as a vibrato of pitch and colour; Breath moves the mouth pressure and reads as a swell; Throat modulates the bore itself and gives a shallower, more internal flutter. Lip, Breath or Throat.' },
        fr: { t: 'Source du vibrato',
              b: 'Choisit ce que module le vibrato. « Lip » agit sur l’embouchure et s’entend comme un vibrato de hauteur et de couleur ; « Breath » agit sur la pression de bouche et s’entend comme un gonflement ; « Throat » module la perce elle-même et donne un battement plus discret et plus interne. « Lip », « Breath » ou « Throat ».',
              reviewed: true },
    },

    'tip.growl': {
        en: { t: 'Growl Amount',
              b: 'Adds the growl a player gets by humming into the instrument: a fixed 120 Hz modulation of the mouth pressure, up to 30% deep. The rate does not follow the note, so the growl beats against the played pitch instead of tracking it. 0 to 1.' },
        fr: { t: 'Quantité de growl',
              b: 'Ajoute le growl qu’un joueur obtient en chantant dans l’instrument : une modulation fixe de la pression de bouche à 120 Hz, jusqu’à 30 % de profondeur. La vitesse ne suit pas la note : le growl bat contre la hauteur jouée au lieu de la suivre. 0 à 1.',
              reviewed: true },
    },

    'tip.flutter': {
        en: { t: 'Flutter Tongue',
              b: 'Flutter tongue — the rolled tongue against the reed, built here from a 25 Hz half-wave that digs into the mouth pressure by up to 40%. It only ever takes pressure away and never adds any, so raising it lowers the average level as well as roughening the tone. 0 to 1.' },
        fr: { t: 'Flatterzunge',
              b: 'Le flatterzunge — la langue roulée contre l’anche, réalisé ici par une demi-onde à 25 Hz qui creuse la pression de bouche jusqu’à 40 %. Il ne fait que retirer de la pression, jamais en ajouter : en montant, on baisse aussi le niveau moyen en plus de rendre le son plus rugueux. 0 à 1.',
              reviewed: true },
    },

    'tip.subtone': {
        en: { t: 'Subtone',
              b: 'The breathy subtone a player gets by damping the reed with the lip and blowing softly. It does three things at once — adds air noise, tightens the embouchure and takes up to 30% off the mouth pressure — so the note goes quieter as well as breathier. 0 to 1.' },
        fr: { t: 'Subtone',
              b: 'Le subtone soufflé qu’un joueur obtient en amortissant l’anche avec la lèvre et en soufflant doucement. Il fait trois choses à la fois — il ajoute du bruit d’air, resserre l’embouchure et retire jusqu’à 30 % de la pression de bouche — la note devient donc à la fois plus douce et plus soufflée. 0 à 1.',
              reviewed: true },
    },

    'tip.chiff': {
        en: { t: 'Attack Chiff',
              b: 'The pressure overshoot at the start of a note — the chiff a wind instrument makes before the tone settles. It is scaled by velocity as well as by this control, up to 30% above the held pressure, so at a soft velocity it barely appears however high this is set. 0 to 1.' },
        fr: { t: 'Bruit d’attaque',
              b: 'Le dépassement de pression au début de la note — le bruit d’attaque qu’un instrument à vent produit avant que le son s’installe. Il est pondéré par la vélocité autant que par ce réglage, jusqu’à 30 % au-dessus de la pression tenue, si bien qu’à faible vélocité il s’entend à peine quel que soit le réglage. 0 à 1.',
              reviewed: true },
    },

    'tip.airNoise': {
        en: { t: 'Air Noise',
              b: 'Breath noise mixed in at the reed. It is scaled by the airflow and by the mouth pressure rather than sitting there as a fixed layer, so it swells with the dynamics and disappears when the note is not sounding. 0 to 1.' },
        fr: { t: 'Bruit d’air',
              b: 'Le bruit de souffle introduit à l’anche. Il est pondéré par le débit d’air et par la pression de bouche plutôt que d’être une couche fixe : il enfle avec la nuance et disparaît quand la note ne sonne pas. 0 à 1.',
              reviewed: true },
    },

    // ── Sound Design ────────────────────────────────────────────────────────
    'tip.infSustain': {
        en: { t: 'Infinite Sustain',
              b: 'Removes the bore’s losses: the bell stops radiating and reflects everything back, and the wall losses fall away to nothing. At 1 the tube is lossless and rings on indefinitely after the breath stops, instead of decaying. 0 to 1.' },
        fr: { t: 'Tenue infinie',
              termNote: 'meaning: no ADSR on this page (no sustain-level parameter exists in O-Reed) — this is the bore ringing on without losses, not an envelope stage. Maintien infini would name an envelope O-Reed does not have. Matches label.knob.infSustain',
              b: 'Supprime les pertes de la perce : le pavillon cesse de rayonner et renvoie tout, et les pertes aux parois tombent à zéro. À 1, le tube est sans pertes et continue de sonner indéfiniment après l’arrêt du souffle au lieu de s’éteindre. 0 à 1.',
              reviewed: true },
    },

    'tip.revBore': {
        en: { t: 'Reverse Bore',
              b: 'Runs the bore’s taper backwards, so the tube narrows toward the bell instead of flaring — nothing is built this way, and the result sits somewhere between a pinched hichiriki and no acoustic parallel at all. It does nothing while Bore Character is fully cylindrical, because a cylinder has no taper to reverse. 0 to 1.' },
        fr: { t: 'Perce inversée',
              b: 'Inverse la conicité de la perce : le tube se resserre vers le pavillon au lieu de s’évaser — rien ne se fabrique ainsi, et le résultat tient du hichiriki pincé et de rien de connu. Le réglage ne fait rien tant que Caractère de la perce est entièrement cylindrique, puisqu’un cylindre n’a pas de conicité à inverser. 0 à 1.',
              reviewed: true },
    },

    'tip.feedback': {
        en: { t: 'Feedback Path',
              b: 'Cross-couples the two bores: the reed hears a blend of its own tube and the drone tube instead of its own alone, up to half and half. It has no effect unless Dual Bore is on, and high settings lock the two tubes together so the drone pulls the played pitch. 0 to 1.' },
        fr: { t: 'Chemin de réinjection',
              b: 'Couple les deux perces : l’anche entend un mélange de son propre tube et du tube de bourdon au lieu du sien seul, jusqu’à parts égales. Le réglage reste sans effet si Double perce n’est pas activée, et en position haute les deux tubes se verrouillent, si bien que le bourdon tire la hauteur jouée. 0 à 1.',
              reviewed: true },
    },

    // The readout beside this knob renders ` ct`; the body spells the unit out,
    // which is what a tooltip is for. The dump's label is `cents`.
    'tip.dronePitch': {
        en: { t: 'Drone Pitch',
              b: 'Tunes the second bore against the played note, in cents: 0 is a unison, ±1200 is an octave and ±2400 is two. It has no effect unless Dual Bore is on. −2400 to +2400 cents.' },
        fr: { t: 'Hauteur du bourdon',
              b: 'Accorde la seconde perce par rapport à la note jouée, en cents : 0 pour l’unisson, ±1200 pour l’octave et ±2400 pour deux octaves. Le réglage reste sans effet si Double perce n’est pas activée. −2400 à +2400 cents.',
              reviewed: true },
    },

    // The one AudioParameterBool. Off / On are its option strings and are kept
    // verbatim in the French for the same reason as the Choice tokens.
    'tip.dualBore': {
        en: { t: 'Dual Bore',
              b: 'Runs a second waveguide in parallel with the first, tuned by Drone Pitch — the arghul and launeddas drone. It doubles the bore cost of every voice, and it is what both Drone Pitch and Feedback Path are waiting on before either of them does anything. Off or On.' },
        fr: { t: 'Double perce',
              b: 'Fait tourner une seconde perce en parallèle de la première, accordée par Hauteur du bourdon — le bourdon de l’arghul et des launeddas. Elle double le coût de perce de chaque voix, et c’est elle que Hauteur du bourdon et Chemin de réinjection attendent tous deux pour agir. « Off » ou « On ».',
              reviewed: true },
    },

    // ── Voice ───────────────────────────────────────────────────────────────
    'tip.maxVoices': {
        en: { t: 'Max Voices',
              b: 'Ceiling on how many notes can sound at once, and it only applies while Polyphony Mode is Polyphonic — in Monophonic the instrument plays one note whatever this says. Each voice is a full waveguide, so the CPU cost rises with it. 1 to 16 voices.' },
        fr: { t: 'Voix maximales',
              b: 'Le plafond du nombre de notes simultanées, et il ne s’applique que si Mode de polyphonie est sur « Polyphonic » — en « Monophonic », l’instrument ne joue qu’une note quel que soit ce réglage. Chaque voix est un guide d’onde complet : le coût processeur monte avec elle. 1 à 16 voix.',
              reviewed: true },
    },

    'tip.polyMode': {
        en: { t: 'Polyphony Mode',
              b: 'Monophonic plays one note at a time, which is what a real reed instrument does and what the legato and voice-stealing behaviour is written for. Polyphonic lets notes overlap, up to the Max Voices ceiling and at the matching cost in CPU. Monophonic or Polyphonic.' },
        fr: { t: 'Mode de polyphonie',
              b: '« Monophonic » ne joue qu’une note à la fois, ce que fait un vrai instrument à anche et ce pour quoi le legato et le vol de voix sont écrits. « Polyphonic » laisse les notes se superposer, jusqu’au plafond de Voix max et pour le coût processeur correspondant. « Monophonic » ou « Polyphonic ».',
              reviewed: true },
    },

    // The latency clause is measured, not hedged: setLatencySamples() is called
    // once in prepareToPlay() from the DEFAULT 2x oversampler
    // (PluginProcessor.cpp:392-394) and is never re-reported when this control
    // changes. Reported as a defect and not fixed here — it is an audio-timing
    // change, not an i18n one — but a tip that omitted it would be misleading
    // about the one thing a user cannot hear.
    'tip.oversampling': {
        en: { t: 'Oversampling',
              b: 'Internal sample-rate multiplier for the reed and the bore, which are nonlinear and would alias without it. 4x is cleaner on high notes and at extreme reed settings and costs roughly twice the CPU; the latency the plugin reports to the host is fixed at the 2x figure and does not follow this control. 2x or 4x.' },
        fr: { t: 'Suréchantillonnage',
              b: 'Le multiplicateur de fréquence d’échantillonnage interne de l’anche et de la perce, qui sont non linéaires et créeraient du repliement sans lui. « 4x » est plus propre dans l’aigu et sur les réglages d’anche extrêmes, pour environ deux fois le coût processeur ; la latence annoncée à l’hôte reste celle de « 2x » et ne suit pas ce réglage. « 2x » ou « 4x ».',
              reviewed: true },
    },

    // ── The two chrome controls ─────────────────────────────────────────────
    //
    // The gear tip is what tells a user hover-help exists at all, so its body
    // describes ONLY what this popover actually holds. O-Reed has no hover-help
    // on/off toggle — not in C++, not in localStorage — and O-Tapestop's wording
    // promises one, so it is not copied. This body is byte-equal to O-Bassoon's
    // because the popover is byte-equal to O-Bassoon's: one row, one selector.
    'tip.gearBtn': {
        en: { t: 'Settings',
              b: 'Opens the panel that sets the language of this interface. That is all it holds: the captions on this page and this hover help change with it, and the choice is kept with the session, so a project reopens in the language it was saved in.' },
        fr: { t: 'Réglages',
              b: 'Ouvre le panneau qui règle la langue de cette interface. Il ne contient rien d’autre : les libellés de cette page et ces infobulles changent avec elle, et le choix est conservé avec la session — un projet se rouvre dans la langue où il a été enregistré.',
              reviewed: true },
    },

    // The last sentence lists this page's THREE standing English regions, each
    // of which is an I18N_EXEMPT entry below with its reason: the option words
    // inside the six dropdowns (D-01 arm 1), the fifteen XY-pad instrument
    // markers, and the whole Tuning tab (shared module). A user who switches to
    // French and then meets one of them deserves to have been told, rather than
    // reading it as a bug.
    'tip.langSelect': {
        en: { t: 'Language',
              b: 'The language of the captions on this page and of this hover help. English and French are available. Value readouts, the option words inside the dropdowns and the fifteen instrument markers on the pad stay in English in both languages, and so does the Tuning tab — its panel comes from a shared module that is not part of this plugin.' },
        fr: { t: 'Langue',
              b: 'La langue des libellés de cette page et de ces infobulles. L’anglais et le français sont proposés. Les valeurs affichées, les intitulés d’options des menus déroulants et les quinze repères d’instruments du pavé restent en anglais dans les deux langues, tout comme l’onglet Accord : son panneau provient d’un module partagé qui n’appartient pas à ce plugin.',
              reviewed: true },
    },
    // v1.4.0 — the switch that reaches this whole layer.
    'tip.tipsToggle': {
        en: { t: 'Hover Help',
              b: 'Turns this hover help on and off. With it off, only the gear and this '
               + 'switch keep explaining themselves.' },
        fr: { t: 'Infobulles',
              b: 'Active ou désactive ces infobulles. Une fois désactivées, seuls '
               + 'l’engrenage et ce commutateur continuent de s’expliquer.',
              reviewed: true },
    },
});

// ============================================================================
// LABELS — the visible text of the page. {en:{t}, fr:{t, reviewed}}.
//
// One string per entry, no body: a label is not a tooltip.
//
// ── THE D-01 TEST ON THIS PLUGIN, AND THE ONE ARM-1 COLLISION ───────────────
//
// O-Reed has SIX AudioParameterChoice parameters, so arm 1 fires here in a way
// it could not on O-Bassoon (which has none). Every option string was compared
// BYTE-FOR-BYTE against every visible string on the page, and exactly one pair
// matches:
//
//     "Breath"  is a vibratoSource option (PluginProcessor.cpp: StringArray
//               { "Lip", "Breath", "Throat" }) AND the caption under the
//               breathPressure knob.
//
// The caption LOCALIZES and the option does not. They are two different
// controls that happen to share a word: the caption names an
// AudioParameterFloat (breath pressure, 0..1), while the option is one of three
// values of an unrelated Choice that the host's automation lane must be able to
// name. O-Detune proved in this same stage that resolving such a collision by
// EXEMPTING the text silences the caption too, because an exemption matches by
// TEXT — so the option is carried as a SCOPED I18N_EXEMPT entry ('option') and
// the caption is keyed as label.knob.breath. Assertion 14 requires exactly that
// shape and fails a bare unscoped entry.
//
// Every other option string — "Simple", "Multi-segment", "Lip", "Throat",
// "Monophonic", "Polyphonic", "2x", "4x", "Scala/TUN", "MTS-ESP", "12-TET" and
// the 21 instrumentPreset names — appears NOWHERE in the authored markup. The
// six <select> elements are empty in index.html and are filled at runtime from
// the host's own choice properties, so the option text is never page copy.
//
// ARM 2 / ARM 3: the 27 `.knob-value` nodes are readouts. formatValue()
// overwrites every one of them on the first valueChangedEvent and on every drag
// afterwards. They are exempt on arm 2 (a number and its unit are
// language-neutral, D-03) AND on arm 3 (a readout node is never a [data-i18n]
// element whatever parameter type is behind it — keying one would make the
// element enter and leave the sweep as the knob turns). They are not listed
// individually in I18N_EXEMPT because the coverage scan already classes them
// non-LABEL, and 27 entries whose text changes on the first mouse drag would be
// 27 entries that never match anything again. The same is true of the two
// `#xy-bore-val` / `#xy-reed-val` spans, which this commit SPLIT out of their
// captions (contract §5) precisely so the caption could be keyed without the
// number going with it.
//
// ── GEOMETRY: THE FOUR CLIFFS ON THIS PAGE ──────────────────────────────────
//
// Measured, not estimated — text-transform and letter-spacing are not in
// getComputedStyle().font, so every number below was read from the RENDERED
// element in this plugin's own 900 x 600 frame. No width was borrowed from
// another plugin; K2 proved two plugins give absolutes 8.24px apart for the
// same two words at the same declared font-size.
//
// CLIFF 1 — `.knob-label` clips SILENTLY. `white-space: nowrap; overflow:
// hidden; text-overflow: ellipsis` at 8px uppercase with 0.3px tracking, capped
// by `.knob-control { width }`. An over-long caption renders an ellipsis rather
// than overflowing, which is the half a spill check cannot see.
//
//   AND IT WAS ALREADY CLIPPING IN ENGLISH AT v1.1.0, on two of the 27:
//       Embouchure   61.44 in a 60px box   — renders "EMBOUCHUR…"
//       Double Reed  60.58 in a 60px box   — renders "DOUBLE REE…"
//   That is a pre-existing English defect the keying EXPOSED rather than
//   caused, and a hard [4][en] failure. Fixed at the box, not at the caption:
//   `.knob-control` width and `.knob-label` max-width go 60px -> 68px. Every
//   `.param-row` is `flex-wrap: wrap` inside an 854px content box and the
//   widest row holds SEVEN knobs — 7 x 68 + 6 x 4 = 500px — so no row rewraps
//   and no row changes height. Negative control: reverted to 60px alone,
//   [4][en] fails again on exactly those two captions.
//
//   The 27 captions as rendered, English -> French, against the 68px cap:
//
//       Breath        34.72 -> Souffle       38.23
//       Embouchure    61.44 -> Embouchure    61.44   sameAsEn — 6.56 left
//       Reed Hard.    52.64 -> Dureté        34.86   SHRANK
//       Output        34.64 -> Sortie        31.16   SHRANK
//       Character     51.64 -> Caractère     50.34   SHRANK
//       Diameter      45.31 -> Diamètre      45.31   sameAsEn is FALSE here
//       Bell Size     42.39 -> Pavillon      42.84
//       Length        35.27 -> Longueur      48.06
//       Tone Hole     49.44 -> Trous de jeu  61.16   6.84 left, the tightest
//       Register      42.44 -> Registre      42.44
//       Opening       39.36 -> Ouvert.       37.39   SHRANK — see CLIFF 4
//       Mass          22.97 -> Masse         28.50
//       Damping       40.81 -> Amort.        33.27   SHRANK  (v1.3.1)
//       Double Reed   60.58 -> Anche dble    54.59   SHRANK
//       Mouthpiece    57.48 -> Bec           16.50   SHRANK
//       Vib Depth     45.89 -> Vib Prof.     41.72   SHRANK
//       Vib Rate      39.17 -> Vib Vit.      33.58   SHRANK
//       Growl         31.50 -> Growl         31.50   sameAsEn
//       Flutter       38.52 -> Flatt.        28.84   SHRANK
//       Subtone       40.14 -> Subtone       40.14   sameAsEn
//       Chiff         25.86 -> Attaque       39.97
//       Air Noise     43.66 -> Bruit d’air   52.09
//       Inf. Sustain  56.34 -> Tenue inf.    48.73   SHRANK
//       Rev. Bore     44.98 -> Perce inv.    47.77
//       Feedback      44.94 -> Réinjection   58.06           (v1.3.1)
//       Drone Pitch   58.77 -> Bourdon       43.03   SHRANK
//       Max Voices    52.66 -> Voix max      42.89   SHRANK
//
//   TWELVE of the 27 SHRINK (v1.3.1 moved one in and one out: Amort. shrank,
//   Réinjection stopped shrinking, so the count is unchanged). That is the half of the risk a clip check is
//   blind to, and the reason the before/after diff is run in both directions.
//   "Dureté anche" was measured and REJECTED at 66.98 — 1.02px of a 68px cap is
//   inside the band where a Windows/WebView2 font metric decides whether a
//   caption ellipsises, and Windows metrics are the named hardware-blocked
//   deferral for this whole rollout.
//
// CLIFF 2 — the XY pad's X-axis caption is an ABSOLUTELY POSITIONED, CENTRED
// box inside a 200 x 170 `overflow: hidden` pad that already holds fifteen
// instrument markers. It grows from its centre, so it reaches toward markers on
// BOTH sides and spills nothing while doing it. In English it already overlaps
// A.Sax, T.Sax and S.Sax. Measured against every marker rect:
//
//       Bore Character       79.89   x 74.05..153.95   hits A.Sax T.Sax S.Sax
//       Caractère de perce   97.80   x 65.10..162.90   hits ... + B.SAX  NEW
//       Caractère perce      83.16   x 72.42..155.58   hits ... + B.SAX  NEW
//       Caract. perce        68.25   x 79.88..148.13   hits A.Sax T.Sax S.Sax
//
//   B.Sax starts at x=154.77 and is DISJOINT from the English caption by
//   0.82px, so any French caption wider than ~81.5px newly intersects it —
//   assertion 8's exact shape, and invisible to every width and clip check
//   because nothing overflows anything. `Caract. perce` is the caption that
//   ships: its hit set is IDENTICAL to English's, with 6.64px of clearance to
//   B.Sax. No pin was needed, and a pin here would have been decoration.
//
//   The Y-axis caption is the same box rotated -90deg: `Double Reed`
//   115.61..178.39 vertically -> `Anche double` 112.20..181.80, hitting Ddk in
//   both languages and nothing else in either.
//
// CLIFF 3 — `.section-content { max-height: 500px; overflow: hidden }` is a
// collapse cliff, and `.param-row` is `flex-wrap: wrap`. Neither fires: the
// tallest section content is one 7-knob row at 68px, still one line and still
// far under 500px in both languages. Measured after the pin, not assumed.
//
// The page HOLDS STILL: every one of the 382 elements occupies an identical
// rectangle at 180ms and at 1.7s, in English at v1.1.0. There is no animation,
// no transition mid-flight and no self-feeding layout runaway, so a geometry
// number taken here means what it says.
// ============================================================================

export const LABELS = Object.freeze({

    // ── Tab bar ─────────────────────────────────────────────────────────────
    // `.tab-btn` is padded 4px 14px and shrink-wraps inside a 900px header that
    // carries 466px of content at v1.2.0, so none of the three can push
    // anything. "Instrument" and "FX" are the same in French — `sameAsEn: true`
    // is an ASSERTION, not a shrug: it is what stops assertion 4 reading an
    // identical string as an untranslated one, and without it the gate cannot
    // tell a deliberate cognate from a forgotten entry.
    'label.tab.instrument': {
        en: { t: 'Instrument' },
        fr: { t: 'Instrument', reviewed: true, sameAsEn: true },
    },
    'label.tab.tuning': { en: { t: 'Tuning' }, fr: { t: 'Accord', reviewed: true } },
    'label.tab.fx': {
        en: { t: 'FX' },
        fr: { t: 'FX', reviewed: true, sameAsEn: true },
    },

    // ── XY pad ──────────────────────────────────────────────────────────────
    // The pad title sits in an 872px block and cannot push anything.
    'label.xy.title': {
        en: { t: 'Instrument Morphing' },
        fr: { t: 'Morphing d’instrument', reviewed: true },
    },

    // See CLIFF 2. `Caract. perce` rather than the literal `Caractère de perce`
    // because the literal newly intersects the B.Sax marker; the abbreviation
    // matches this page's own caption style (Reed Hard., Inf. Sustain, Vib
    // Depth) and its marker hit set is identical to English's.
    'label.xy.axisX': {
        en: { t: 'Bore Character' },
        fr: { t: 'Caract. perce', reviewed: true },
    },
    'label.xy.axisY': {
        en: { t: 'Double Reed' },
        fr: { t: 'Anche double', reviewed: true },
    },

    // The two readout captions, SPLIT out of their value spans in this commit
    // (contract §5) so the caption can carry a key while the number beside it
    // stays a readout. French puts a space before a colon; U+00A0 keeps it from
    // being a line-break opportunity in a `flex-wrap: wrap` row.
    'label.xy.boreKey': { en: { t: 'Bore:' }, fr: { t: 'Perce :', reviewed: true } },
    'label.xy.reedKey': { en: { t: 'Reed:' }, fr: { t: 'Anche :', reviewed: true } },

    // ── Section headings ────────────────────────────────────────────────────
    // "la perce" is the French term for a wind instrument's bore, and it is used
    // consistently for every bore-derived caption below.
    'label.section.primary': {
        en: { t: 'Primary Controls' },
        fr: { t: 'Réglages principaux', reviewed: true },
    },
    'label.section.bore': {
        en: { t: 'Bore & Resonance' },
        fr: { t: 'Perce et résonance', reviewed: true },
    },
    // CLIFF 4, and it is invisible to every width, clip and spill check.
    // `.section-content { max-height: 0; overflow: hidden }` does NOT remove the
    // collapsed section's children from layout — they keep their natural
    // rectangles inside a zero-height box and are merely clipped from PAINTING.
    // So the six knob wrappers of the collapsed "Bore & Resonance" section above
    // still occupy x = 32..82, 104..154, 176..226 ... across this header.
    // English "Bore Visualization" ends at x=175.06 and clears the third wrapper
    // by 0.94px; "Visualisation de la perce" ends at 221.95 and lands squarely
    // on it, together with its svg, its two paths and its circle — the seven to
    // nine [8b] intersections the gate reported. "Coupe de la perce" ends at
    // 165.73, which is NARROWER than the English caption, so its intersection
    // set is a subset of English's by construction. It also describes the
    // placeholder underneath it more exactly, which reads "Bore cross-section
    // visualization".
    'label.section.boreViz': {
        en: { t: 'Bore Visualization' },
        fr: { t: 'Coupe de la perce', reviewed: true },
    },
    'label.section.reed': { en: { t: 'Reed' }, fr: { t: 'Anche', reviewed: true } },
    'label.section.expression': {
        en: { t: 'Expression' },
        fr: { t: 'Expression', reviewed: true, sameAsEn: true },
    },
    // Same cliff, and this one was passing BY 0.09 PIXELS. "Conception sonore"
    // is 137.91 wide, ending at x=175.91 against the third collapsed knob
    // wrapper's left edge at 176.00 — a coin flip across a font-metric change,
    // and the gate was green only because the coin landed the right way up.
    // "Design sonore" is the term French audio actually uses, is 103.84 wide,
    // and ends at 141.84 with 34.16px of clearance.
    'label.section.soundDesign': {
        en: { t: 'Sound Design' },
        fr: { t: 'Design sonore', reviewed: true },
    },
    'label.section.voice': { en: { t: 'Voice' }, fr: { t: 'Voix', reviewed: true } },

    // ── Knob captions ───────────────────────────────────────────────────────
    // The caption under the knob, never the value beside it: `.knob-value` is a
    // separate node and stays untouched (contract §5).

    // KEYED even though "Breath" is byte-identical to a vibratoSource option.
    // See the D-01 note above: two different controls, one shared word. The
    // OPTION is the scoped I18N_EXEMPT entry; this is the caption.
    'label.knob.breath': { en: { t: 'Breath' }, fr: { t: 'Souffle', reviewed: true } },

    // The English caption is already the French word. sameAsEn: true is the
    // assertion that its identity is deliberate.
    'label.knob.embouchure': {
        en: { t: 'Embouchure' },
        fr: { t: 'Embouchure', reviewed: true, sameAsEn: true },
    },

    // reedHardness. "Dureté anche" is the literal form and measures 66.98 of the
    // 68px cap — 1.02px, inside the Windows-metric band. The head noun alone is
    // 34.86, and the knob sits between Embouchure and Reed Hard.'s own siblings
    // in the PRIMARY section where nothing else is a hardness.
    'label.knob.reedHard': { en: { t: 'Reed Hard.' }, fr: { t: 'Dureté', reviewed: true } },

    'label.knob.output': { en: { t: 'Output' }, fr: { t: 'Sortie', reviewed: true } },

    'label.knob.character': { en: { t: 'Character' }, fr: { t: 'Caractère', reviewed: true } },
    'label.knob.diameter':  { en: { t: 'Diameter' },  fr: { t: 'Diamètre',  reviewed: true } },

    // bellSize. "le pavillon" IS the bell of a wind instrument; "Taille pav."
    // measures 51.28 and says the same thing twice, since the readout beside it
    // is the size.
    'label.knob.bellSize': { en: { t: 'Bell Size' }, fr: { t: 'Pavillon', reviewed: true } },

    'label.knob.length': { en: { t: 'Length' }, fr: { t: 'Longueur', reviewed: true } },

    // toneHoleCutoff, 200-8000 Hz. "les trous de jeu" are a woodwind's finger
    // holes; the readout beside it carries the Hz.
    'label.knob.toneHole': { en: { t: 'Tone Hole' }, fr: { t: 'Trous de jeu', reviewed: true } },

    'label.knob.register': { en: { t: 'Register' }, fr: { t: 'Registre', reviewed: true } },

    // CLIFF 4 again, and this time it costs a whole word. `.knob-label` is
    // CENTRED in its 68px control, so a longer caption grows LEFTWARD as well as
    // right, and the reedOpening knob is the first in its row — flush with the
    // section's 8px left padding. The Reed section ships collapsed, so its
    // caption keeps its rectangle inside a max-height:0 box and lands on the
    // "Sound Design" chevron below: chevron right edge 33.00, English "Opening"
    // left edge 37.31 (4.31px clear), "Ouverture" left edge 30.64 (2.36px OVER).
    // "Ouvert." is 38.30 — 5.30px clear, BETTER than English — and matches this
    // page's own abbreviation style (Reed Hard., Inf. Sustain, Amort., Flatt.,
    // Vib Prof.). See the gate note in the commit message: nothing here is ever
    // PAINTED on the chevron, because the caption is clipped out of existence by
    // its ancestor, and assertion 8b compares rectangles without asking that.
    'label.knob.opening': { en: { t: 'Opening' }, fr: { t: 'Ouvert.', reviewed: true } },
    'label.knob.mass':    { en: { t: 'Mass' },    fr: { t: 'Masse',     reviewed: true } },
    'label.knob.damping': { en: { t: 'Damping' }, fr: { t: 'Amort.',    reviewed: true } },

    // "Anche double" is 67.20 against a 68px cap — 0.80px, which is the same
    // Windows-metric band that rejected "Dureté anche". The page's own
    // abbreviation style gives "Anche dble" at 54.59 with 13.41px to spare, and
    // the XY pad's Y-axis caption above it already spells the phrase out in
    // full where there is room for it.
    'label.knob.doubleReed': {
        en: { t: 'Double Reed' },
        fr: { t: 'Anche dble', reviewed: true,
              termNote: 'width, re-measured in v1.3.1: the glossary root Anche double is 67.20 px against the 68.00 px .knob-label max-width — 0.80 px, inside the same Windows/WebView2 font-metric band that made v1.3.0 reject Dureté anche at 1.02 px, and .knob-label carries text-overflow: ellipsis so the failure is a silent truncation. The full term already ships twice on this page: the XY pad Y-axis caption (label.xy.axisY) and this control tip title' },
    },

    // mouthpieceVol. "le bec" is the mouthpiece of a clarinet or saxophone.
    'label.knob.mouthpiece': { en: { t: 'Mouthpiece' }, fr: { t: 'Bec', reviewed: true } },

    'label.knob.vibDepth': {
        en: { t: 'Vib Depth' },
        fr: { t: 'Vib Prof.', reviewed: true,
              termNote: 'GEOMETRY, measured in v1.3.1, and the gate proved it: CLIFF 4 below. This caption is centred at x 57.00 in a 23..91 cell, and the collapsed Expression section keeps its rectangle over the next section-header, whose chevron ends at x 33.00 — so any caption wider than 48.00 px intersects it. The glossary root Prof. vibrato is 64.80 px and the listed abbreviation Prof. vibr is 47.63 px (0.19 px of clearance). Prof. vibr. was SHIPPED into check-ui-labels and [8b] FAILED on exactly this pair of rectangles at 50.08 px. Kept with label.knob.vibRate as one matched pair' },
    },
    'label.knob.vibRate':  {
        en: { t: 'Vib Rate' },
        fr: { t: 'Vib Vit.', reviewed: true,
              termNote: 'the other half of the pair. Vit. vibr. (41.94 px) and even the root Vit. vibrato (56.66 px) both FIT here — this knob is second in the row with 17 px of clearance to its left neighbour, not first against a chevron. It is held to Vib Prof. because the two are one matched pair on one row (English Vib Depth / Vib Rate), and applying the glossary to only the half that fits would put the head noun on opposite sides of two adjacent captions. N6 correction 40: the pair is the unit, one termNote per half' },
    },

    // A saxophone growl is called a growl in French too.
    'label.knob.growl': {
        en: { t: 'Growl' },
        fr: { t: 'Growl', reviewed: true, sameAsEn: true },
    },

    // flutterTongue. French scores mark it "Flatt." (Flatterzunge), the same
    // abbreviation a French wind player reads on the page.
    'label.knob.flutter': {
        en: { t: 'Flutter' },
        fr: { t: 'Flatt.', reviewed: true,
              termNote: 'MEANING, then width. The parameter is flutterTongue — the wind technique — not tape wow-and-flutter, so the glossary row that applies is `flutter tongue -> Flatterzunge`, not `flutter -> Scintillement` (which is also 69.47 px and clips the 68.00 px cap outright). Flatterzunge itself measures 67.36 px, 0.64 px of the cap, tighter than the Anche double case above. Flatt. is the abbreviation French scores print for it and is the form a French wind player reads; the tip title spells Flatterzunge out in full' },
    },

    // A saxophone subtone is called a subtone in French too.
    'label.knob.subtone': {
        en: { t: 'Subtone' },
        fr: { t: 'Subtone', reviewed: true, sameAsEn: true },
    },

    // attackChiff — the breathy onset transient. "Chiff" has no French currency;
    // "Attaque" names the thing the knob shapes and is unambiguous because this
    // page has no other attack control.
    'label.knob.chiff': { en: { t: 'Chiff' }, fr: { t: 'Attaque', reviewed: true } },

    'label.knob.airNoise': { en: { t: 'Air Noise' }, fr: { t: 'Bruit d’air', reviewed: true } },

    'label.knob.infSustain': {
        en: { t: 'Inf. Sustain' },
        fr: { t: 'Tenue inf.', reviewed: true,
              termNote: 'meaning: this page has no ADSR — there is no sustain-level parameter anywhere in O-Reed, and infiniteSustain removes the bore losses so the TUBE holds on. Maintien is the ADSR sustain level and would name a control that does not exist here. Same exemption O-Bowed took in N4 for the same shape' },
    },
    'label.knob.revBore':    { en: { t: 'Rev. Bore' },    fr: { t: 'Perce inv.', reviewed: true } },
    'label.knob.feedback':   { en: { t: 'Feedback' },     fr: { t: 'Réinjection', reviewed: true } },

    // dronePitch, -2400..2400 cents. "un bourdon" IS a drone; the readout beside
    // it carries the cents.
    'label.knob.dronePitch': { en: { t: 'Drone Pitch' }, fr: { t: 'Bourdon', reviewed: true } },

    'label.knob.maxVoices': { en: { t: 'Max Voices' }, fr: { t: 'Voix max', reviewed: true } },

    // ── Dropdown captions ───────────────────────────────────────────────────
    // Each sits above a full-width <select> in a block that is at least 662px
    // wide, so none of them can push anything. The OPTIONS inside those selects
    // are host-owned Choice strings and are not page copy — see the D-01 note.
    'label.dropdown.instrumentPreset': {
        en: { t: 'Instrument Preset' },
        fr: { t: 'Préréglage d’instrument', reviewed: true },
    },
    'label.dropdown.boreProfile': {
        en: { t: 'Bore Profile' },
        fr: { t: 'Profil de perce', reviewed: true },
    },
    'label.dropdown.vibratoSource': {
        en: { t: 'Vibrato Source' },
        fr: { t: 'Source du vibrato', reviewed: true },
    },
    'label.dropdown.polyMode': {
        en: { t: 'Poly Mode' },
        fr: { t: 'Mode de polyphonie', reviewed: true },
    },
    'label.dropdown.oversampling': {
        en: { t: 'Oversampling' },
        fr: { t: 'Suréchantillonnage', reviewed: true },
    },

    // ── Toggle ──────────────────────────────────────────────────────────────
    'label.toggle.dualBore': { en: { t: 'Dual Bore' }, fr: { t: 'Double perce', reviewed: true } },

    // ── Placeholders ────────────────────────────────────────────────────────
    // Both live in centred flex boxes with hundreds of pixels of slack, so the
    // French length is unconstrained. They are the page's most user-facing
    // prose after the knob captions: a French user who opens the FX tab reads a
    // sentence, not a caption.
    'label.boreViz.placeholder': {
        en: { t: 'Bore cross-section visualization (coming soon)' },
        fr: { t: 'Visualisation en coupe de la perce (bientôt disponible)', reviewed: true },
    },
    'label.fx.title': {
        en: { t: 'Coming Soon' },
        fr: { t: 'Bientôt disponible', reviewed: true },
    },
    'label.fx.body': {
        en: { t: 'Effects processing will be added in a future update.' },
        fr: { t: 'Le traitement d’effets sera ajouté dans une future mise à jour.', reviewed: true },
    },

    // ── The settings popover (v1.2.0) ───────────────────────────────────────
    'label.language': { en: { t: 'Language' }, fr: { t: 'Langue', reviewed: true } },

    // v1.4.0. All four renderings below are settled glossary ROOTS, copied
    // rather than authored: scripts/i18n-fr-glossary.js carries them as the
    // roots for 'hover help', 'on', 'off' and 'toggle hover help'. They take
    // the same review mark this file's other roots carry, and for the same
    // reason — they are not new machine output.
    'label.hoverHelp': { en: { t: 'Hover help' }, fr: { t: 'Infobulles', reviewed: true } },
    'ui.on':           { en: { t: 'On' },         fr: { t: 'Marche', reviewed: true } },
    'ui.off':          { en: { t: 'Off' },        fr: { t: 'Arrêt',  reviewed: true } },

    // ── The one JS-written string on this page ──────────────────────────────
    // The tuning panel is lazy-mounted on the first Tuning-tab activation and
    // this is what the container says if that dynamic import fails. It is
    // written through setLabel(), so the node becomes a [data-i18n] element from
    // that moment on and the language sweep owns it — a failure notice stranded
    // in the previous language is exactly the bug contract §3 exists to prevent.
    // At v1.1.0 it was an innerHTML string; assertion 9 forbids that path.
    'label.tuningLoadFailed': {
        en: { t: 'Tuning panel failed to load.' },
        fr: { t: 'Échec du chargement du panneau d’accord.', reviewed: true },
    },

    // ── Accessible names ────────────────────────────────────────────────────
    // Resolved through the same sweep via data-i18n-aria, so a screen reader
    // hears the language the page is showing.
    //
    // NOT MIGRATED PROSE AND NOT NEW PROSE FOR AN EXISTING CONTROL: v1.1.0 had
    // zero aria-label attributes and zero native title= attributes, measured.
    // These two name the two elements this commit ADDS, and both are byte-equal
    // to the O-Bassoon precedent so the settings cluster reads the same on every
    // plugin that has one.
    'aria.settings': {
        en: { t: 'Settings' },
        fr: { t: 'Réglages', reviewed: true },
    },
    'aria.langSelect': {
        en: { t: 'Interface language' },
        fr: { t: 'Langue de l’interface', reviewed: true },
    },
    'aria.helpToggle': { en: { t: 'Toggle hover help' }, fr: { t: 'Activer ou désactiver les infobulles', reviewed: true } },
});

// ============================================================================
// I18N_EXEMPT — reasoned exclusions, never silence
//
// Every visible string the coverage scan finds must be a [data-i18n] element, a
// setLabel() call, or an entry HERE WITH A REASON. A bare skip list would let a
// missed label hide as a deliberate one.
//
// AN EXEMPTION IS MATCHED BY TEXT, so an unscoped one silences EVERY node with
// that string. The third field names where it applies, and it is REQUIRED
// exactly where the string is also keyed on this page — which is the one state
// in which the gate cannot tell a deliberate skip from a forgotten label.
// Assertion 14 enforces it. Exactly one entry here needs a scope.
// ============================================================================

export const I18N_EXEMPT = [
    ['O-Reed',
     'the product name — never translated. It is the registered PRODUCT_NAME in CMakeLists.txt and it is alone in its own block at the left of the header bar, so nothing sits beside it to be pushed'],

    ['Ouaricon',
     'the company name — never translated. Unlike O-Bassoon\'s About card, where "Ouaricon" is the second half of a CENTRED line whose first half localizes and therefore had to be keyed, this one is the LAST flex item in the header bar with nothing after it: no localized string shares its line, so nothing re-centres and assertion 7 has nothing to report'],

    // ── THE ARM-1 COLLISION, SCOPED ─────────────────────────────────────────
    // "Breath" is a vibratoSource option string VERBATIM (PluginProcessor.cpp,
    // StringArray { "Lip", "Breath", "Throat" }) and the page and the host
    // automation lane must agree about it. It is ALSO the caption under the
    // breathPressure knob, which is an AudioParameterFloat and localizes to
    // "Souffle" as label.knob.breath.
    //
    // Without the scope this entry would silence that caption too and leave bare
    // English on the page with the gate GREEN — the exact hole O-Detune's
    // "Random" opened in this stage. The scope is `option`, matched against the
    // node's parent and ancestors, so it reaches an <option> and nothing else.
    ['Breath',
     'a vibratoSource AudioParameterChoice option string VERBATIM — the page and the host automation lane must name it identically (D-01 arm 1). The breathPressure knob CAPTION shares the word and is keyed as label.knob.breath; this entry is scoped so it cannot silence that caption',
     'option'],

    // ── The fifteen XY pad instrument markers ───────────────────────────────
    // Not reachable by the coverage scan: populateXYMarkers() writes them from a
    // literal array of {x, y, label} and the extractor classes none of them,
    // because a textContent write whose right-hand side is a property access is
    // not a prose literal. Recorded here so the decision is on the record rather
    // than being an accident of where the scanner looks — which is the same
    // reason O-Bassoon records its tuning-panel captions.
    //
    // THREE OF THE FIFTEEN ARE BYTE-IDENTICAL TO AN instrumentPreset OPTION —
    // "Oboe", "Suona" and "Piri" appear verbatim in the same StringArray the
    // dropdown 30px away is filled from. That is D-01 arm 1, and it decides the
    // whole set: the markers are the abbreviation set of ONE Choice parameter
    // (Clar/B.Clar/A.Sax/T.Sax/S.Sax/B.Sax/Oboe/E.Hrn/Bsn/Ddk/Shn/Suona/Hch/
    // Zrn/Piri against "Bb Clarinet"..."Piri"), and localizing the twelve that
    // are not byte-identical while three stayed English would put two languages
    // inside one 15-item set, next to a host-owned dropdown that names the same
    // fifteen instruments in English. Six of them — Ddk, Shn, Suona, Hch, Zrn,
    // Piri — are loanwords spelled identically in French in any case.
    //
    // A FRENCH USER THEREFORE STILL READS "Oboe" AND "E.Hrn" ON THE XY PAD.
    // That is the cost, it is deliberate, and it is reported.
    ['XY pad instrument markers',
     'the fifteen 7px markers on the XY pad are the abbreviation set of the instrumentPreset AudioParameterChoice, and THREE of them — "Oboe", "Suona", "Piri" — are byte-identical to their option strings (D-01 arm 1). The pad must name the same instruments the host automation lane and the dropdown beside it name, so the whole set stays English rather than being split across two languages. Written by populateXYMarkers() from a literal array, so the coverage scan does not reach them'],

    // ── Endonyms ────────────────────────────────────────────────────────────
    ['English',  'endonym — a language name is never translated'],
    ['Français', 'endonym — a language name is never translated'],

    // ── The shared tuning module ────────────────────────────────────────────
    ['Tuning tab captions',
     'every caption inside the Tuning tab belongs to the SHARED module ${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine (js/tuning-panel.js + snippets/tuning-panel.css, referenced by path from CMakeLists.txt rather than copied into this plugin). Localizing it is a cross-plugin change and any local edit here would be reverted by /module-upgrade. A French user therefore still reads the Tuning tab in English. Its "Scala/TUN", "MTS-ESP" and "12-TET" strings are also tuningSystem option strings, so they are exempt twice over'],
];

// ============================================================================
// TIP_BINDINGS — [selector, key] or [selector, key, wrapper].
//
// applyI18n() resolves each selector with document.querySelector, walks
// closest(wrapper) when a third element is given, and writes data-tip-title and
// data-tip onto the result in the current language. It rewrites both on every
// language change. THAT IS ALL THE CANON DOES: the thing that reads those two
// attributes and paints a surface is per-plugin code, and before v1.3.0 this
// page had no #tooltip element, no .tooltip rule and no hover handler at all.
// Authoring the table above without it would have shipped 35 INVISIBLE STRINGS
// past three green gates — check-i18n only counts bindings, check-ui-labels has
// no tooltip awareness whatsoever, and boot-all-uis counts aria-label and title
// and never data-tip. The renderer therefore lands in index.html in the SAME
// commit as this table, and tests/ui_tip_render_check.js is the gate that can
// actually see a rendered tip.
//
// ── "BIND TO THE IDS THE UI ALREADY USES" IS HALF FALSE HERE ────────────────
//
// T17 says that; the M1 pilots found it wrong on five plugins out of five, for
// a different reason each time, and the brief's own conclusion is that the
// SELECTOR half and the TARGET half fail independently. On O-Reed they fail on
// opposite sides of the same page:
//
//   SELECTOR half, FALSE for 27 of 35. None of the 27 knobs carries an id —
//   they are `.knob-control[data-param="..."]`, which is what the controller
//   itself queries (index.html:1273). The 8 non-knob anchors ARE ids.
//
//   TARGET half, FALSE for 6 of 35, and TRUE for the 27 knobs. Unlike O-Comp
//   and O-Tremolo, `.knob-control` IS the hover cell here — a 68px flex column
//   holding the 50px SVG, the caption and the readout — so a wrapper argument
//   on a knob would be a no-op dressed as a decision. The five `<select>`
//   elements are the opposite case: a bare 26px control with its caption in a
//   sibling `<div class="dropdown-label">`, so each walks up to its own
//   `.dropdown-control` and the caption becomes part of the hover area.
//
// ── THE CHROME IS BOUND BARE AND HALF-WRAPPED, WHICH IS DELIBERATE ──────────
//
// O-Comp's carried trap: bind the chrome BARE wherever the gear and the
// selector share an ancestor, or hovering #lang-select resolves to the gear's
// tip. Here `.settings-cluster` contains BOTH the gear button and the popover,
// so #gear-btn is bound BARE — a wrapper walk to `.settings-cluster` would make
// every hover anywhere in the open popover show the gear's tip.
//
// #lang-select walks to `.settings-row` instead, which is the `<label>` INSIDE
// the popover holding the caption and the select and NOT the gear. It is one
// node, it cannot reach the gear, and it makes the word "Language" part of the
// selector's own hover area. Checked both ways: `.settings-row` matches exactly
// one node on this page and `closest()` from #lang-select reaches it.
//
// #dualBore-toggle is bound bare for the same reason `.knob-control` is: the
// id is already on the wrapper that holds the track and the caption.
//
// The popover ships `hidden`, so #lang-select has no box until the gear is
// pressed. querySelector still finds it, so the binding resolves at load; the
// render gate opens the popover before hovering it.
//
// Order matches the page's reading order — the XY dropdown, then the six
// sections top to bottom, then the header cluster — so a reviewer walking this
// list walks the UI.
// ============================================================================

export const TIP_BINDINGS = [
    // XY pad info panel
    ['#instrumentPreset',                             'tip.instrumentPreset', '.dropdown-control'],

    // Primary Controls
    ['.knob-control[data-param="breathPressure"]',    'tip.breath'],
    ['.knob-control[data-param="embouchure"]',        'tip.embouchure'],
    ['.knob-control[data-param="reedHardness"]',      'tip.reedHard'],
    ['.knob-control[data-param="outputGain"]',        'tip.output'],

    // Bore & Resonance
    ['.knob-control[data-param="boreCharacter"]',     'tip.character'],
    ['.knob-control[data-param="boreDiameter"]',      'tip.diameter'],
    ['.knob-control[data-param="bellSize"]',          'tip.bellSize'],
    ['.knob-control[data-param="boreLength"]',        'tip.length'],
    ['.knob-control[data-param="toneHoleCutoff"]',    'tip.toneHole'],
    ['.knob-control[data-param="registerHole"]',      'tip.register'],
    ['#boreProfile',                                  'tip.boreProfile',      '.dropdown-control'],

    // Reed
    ['.knob-control[data-param="reedOpening"]',       'tip.opening'],
    ['.knob-control[data-param="reedMass"]',          'tip.mass'],
    ['.knob-control[data-param="reedDamping"]',       'tip.damping'],
    ['.knob-control[data-param="doubleReed"]',        'tip.doubleReed'],
    ['.knob-control[data-param="mouthpieceVol"]',     'tip.mouthpiece'],

    // Expression
    ['.knob-control[data-param="vibratoDepth"]',      'tip.vibDepth'],
    ['.knob-control[data-param="vibratoRate"]',       'tip.vibRate'],
    ['.knob-control[data-param="growlAmount"]',       'tip.growl'],
    ['.knob-control[data-param="flutterTongue"]',     'tip.flutter'],
    ['.knob-control[data-param="subtone"]',           'tip.subtone'],
    ['.knob-control[data-param="attackChiff"]',       'tip.chiff'],
    ['.knob-control[data-param="airNoise"]',          'tip.airNoise'],
    ['#vibratoSource',                                'tip.vibratoSource',    '.dropdown-control'],

    // Sound Design
    ['.knob-control[data-param="infiniteSustain"]',   'tip.infSustain'],
    ['.knob-control[data-param="reverseBore"]',       'tip.revBore'],
    ['.knob-control[data-param="feedbackPath"]',      'tip.feedback'],
    ['.knob-control[data-param="dronePitch"]',        'tip.dronePitch'],
    ['#dualBore-toggle',                              'tip.dualBore'],

    // Voice
    ['.knob-control[data-param="maxVoices"]',         'tip.maxVoices'],
    ['#polyMode',                                     'tip.polyMode',         '.dropdown-control'],
    ['#oversampling',                                 'tip.oversampling',     '.dropdown-control'],

    // Header cluster
    ['#gear-btn',                                     'tip.gearBtn'],
    ['#lang-select',                                  'tip.langSelect',       '.settings-row'],
    ['#tips-toggle',                                  'tip.tipsToggle'],
];

// The tooltip lookup. Returns {t, b} — never null, never a bare key without a
// console.warn saying so, because a silently-missing tip renders as an empty
// surface that looks like a positioning bug rather than a missing entry.
//
// LIVE as of v1.3.0: applyI18n() calls it once per TIP_BINDINGS entry, 35 times
// per language switch. Its shape is unchanged from v1.2.0 — it was exported
// verbatim while the loop was empty precisely so that adding bodies here would
// not need it edited, and it did not.
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

    const fill = (str) => (vars
        ? String(str).replace(/\{(\w+)\}/g, (m, n) => (n in vars ? resolve(vars[n]) : m))
        : String(str));

    return { t: fill(s.t), b: fill(s.b) };
}
