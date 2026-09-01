/*
   This file is part of O-Bells, an Ouaricon Audio plugin.
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
// i18n.js — O-Bells UI copy, English + French (v4.3.2, canon v2)
//
// ── v4.3.2: ENGLISH READ AGAINST THE DSP (Stage O, 2026-08-31) ────────────
// Two bodies said more than the code does, in both languages (item 66):
//  - tip.partialTuning claimed "the upper partials". BellVoice.cpp
//    calculatePartialFrequency() scales ONE ratio, partialIndex == 2 — the
//    tierce, 2.4x in bellRatios — by 2^(cents/1200). Hum (0.5x), prime (1x)
//    and partials 3-7 never move. The body now names the third partial.
//  - tip.damping claimed "the bell's partials". The live reach of `damping`
//    is updateMultiStageCoefficients() (hum-stage coefficients of partials
//    0-1 only, x3 at 0 -> x1.4 at 1) and stopNote()'s release
//    (jmap(damping, 3.0 s, 0.5 s)). Strike/body-stage decay of every partial
//    and hum-stage decay of partials 2-7 come from the stage times, Material
//    and Acoustic Brightness. The body now says exactly that, with the two
//    release endpoints.
// Both French bodies rewritten for the meaning change: reviewed: false on
// those two entries; the other 184 keep the developer's reviewed: true.
// Heights at the 260 px cap, en/fr: damping 103.2/103.2 -> 153.2/186.5
// (bottom clearance 289.6 -> 239.6/206.3); partialTuning 103.2/103.2 ->
// 153.2/153.2 (bottom clearance 96.6 -> 46.6). Measured by
// tests/ui_tip_render_check.js, both languages, every tab.
//
// ── v4.3.1: FRENCH QA PASS (Stage N, 2026-08-31) ──────────────────────────
// Every fr entry read against its en and against scripts/i18n-fr-glossary.js.
// Changed: 72 of 186 entries (29 terminology, 56 typography, 0 grammar/
// agreement, 4 meaning, 2 idiom/register — 19 entries took both passes).
// sameAsEn: kept 15, translated 0, ADDED 1 (label.mix — Mix is the glossary
// root AND the English word, so check-i18n assertion 4 needs the flag).
// termNote exemptions: 0 — every glossary term applied, root or listed
// abbreviation, and nothing on this page needed a contextual exemption.
// Left as drafted: the rest. reviewed: false throughout — no native speaker yet.
//
// DECISIONS THE NEXT READER NEEDS, each with the measurement behind it:
//
//  - RÉVERB, NOT RÉVERBÉRATION. The .fx-title box is 62.00 px and the root
//    measures 109.38 px against Réverb's 51.84. So the six reverb tip TITLES
//    all read "… de la réverb" as well, matching the caption the user can see,
//    rather than putting the root in the titles and the abbreviation on screen.
//  - FILTRE PB stays on the caption — .lp-filter-toggle's content box is 78 px
//    and "Filtre passe-bas" measures 100.16 — while tip.lpFilterEnabled's title
//    spells it out. Both are listed forms of one glossary row, and a 260 px
//    surface is where an abbreviation stops being necessary (M2 finding 9).
//  - AMOUNT → QUANTITÉ. Dosage is forbidden and Ampleur is forbidden too (it is
//    the suite's Depth). The three fine-band BODIES already said "la quantité
//    d'éclosion", so the caption is settled by its own body. Measured 116.09 /
//    97.41 / 106.72 / 87.89 / 110.59 px against 234 / 169 / 169 / 169 / 234 px
//    cells — every one fits with room.
//  - HUM SUSTAIN → MAINTIEN BOURDON. Tenue is forbidden, and unlike O-Bowed and
//    O-Formant this page has NO competing Sustain control, so there is nothing
//    for a termNote to exempt. 114.19 px in a 169 px cell. tip.damping's body
//    follows it — one page, one word for sustain.
//  - TUNING TAB → ACCORD, not Gamme. The glossary keeps two words for two
//    things and this page has both: the tab is the ACCORD, the library holds
//    GAMMES. 63.19 px in a 265 px flex:1 tab.
//  - PARTIAL TUNE → "Accord des partiels" on the CAPTION as well as in the tip,
//    because the root fits: 125.92 px in a 174.5 px cell. One control, one
//    French name — the caption and the tip title no longer differ.
//  - "EXP." / "LOG." KEEP THEIR FRENCH ABBREVIATING PERIOD, and
//    tip.velocityCurve's body still names the ENGLISH faces "Exp" and "Log".
//    The lint reports both captions as straight copies because its norm()
//    strips a trailing period; check-i18n compares bytes and does not. NEITHER
//    may take sameAsEn — flagging one disarms assertion 4 for it (N3
//    correction 26). Reported to the orchestrator as a lint over-report.
//  - tip.highFidelity's BODY IS NO LONGER THE v4.1.5 PROSE VERBATIM IN FRENCH.
//    The draft said "pour une tenue maximale" where the English says "for
//    maximum sustain fidelity" — it had dropped the fidelity, which is the
//    whole point of the control. The ENGLISH is still verbatim. It is also the
//    only tip that grew: 103.2 → 119.9 px, bottom clearance 53.6 → 37.0, top
//    pinned at 443.2, so it grew DOWNWARD and never reached the y floor.
//  - CHOICE-PARAMETER FACES STAY ENGLISH inside the French bodies (Bronze, Cast
//    Iron, Click, Thud, Ping, Normal, PingPong, Linear) because they are
//    I18N_EXEMPT and render English on screen; LOCALIZED faces are named in
//    French — the four bypass bodies say "affiche Marche" and ui.on IS Marche.
//    N4 correction 34 and N8 correction 53, both arms, checked on this page.
//  - THE TYPOGRAPHY PASS added 69 U+00A0 (30 before %, 4 before a colon, 11
//    before ; ! ?, 24 between a number and its unit), scoped by a state machine
//    over the fr: blocks with comment lines skipped, and audited afterwards:
//    zero U+00A0 outside a t:/b: value, zero en entries changed by import
//    comparison, TIP_BINDINGS and I18N_EXEMPT byte-identical.
//
// LABELS AND HOVER-HELP. v4.2.0 localized the page's captions and shipped I18N
// empty with TIP_BINDINGS = [], because the plugin had no tooltip renderer at
// all — no #tooltip surface, no .tooltip rule, no hover handler. v4.3.0 adds
// the renderer (index.html, setupTooltips(), ported from O-simpleFM's delegated
// cursor-following family) and 65 hover-help entries in the SAME commit.
//
// That pairing is the point. canon v2's applyI18n() writes data-tip-title and
// data-tip ATTRIBUTES onto the anchors in TIP_BINDINGS and stops there; the
// thing that reads them and paints a surface is per-plugin code outside the
// canon. Authoring the copy alone would have shipped 65 invisible strings and
// three green gates — check-i18n reads the table statically, check-ui-labels
// has no tooltip awareness whatsoever, and boot-all-uis counts aria-label and
// title and never data-tip. tests/ui_tip_render_check.js is the assertion none
// of those three can make, and it is the first runnable gate this plugin has.
//
// An ES module that EXPORTS ONLY. It must never self-execute: a bare top-level
// statement here throws out of module evaluation and takes every later
// initializer on the page with it (pattern_module_toplevel_init_tdz).
// check-i18n assertion 7 enforces it.
//
// SERVED ROOT IS Resources/ui, read from CMakeLists.txt before a byte was
// written here — NOT Source/ui/public, which this plugin does not have. The
// binary-data target O-Bells_UIResources carries no namespace argument, so it
// takes the default BinaryData namespace and works only because it is the only
// such target in this plugin; this file was added to that EXISTING SOURCES list
// rather than to a second target, which would collide on the BinaryData
// namespace (critical_dual_binary_data_namespace_collision).
//
// FOUR PLACES, ONE COMMIT: this file on disk, the SOURCES list in
// CMakeLists.txt, a getResource() branch in PluginEditor.cpp, and the import in
// the inline module in index.html. Miss one and the page 404s at runtime and
// presents as a dead panel with no other symptom (assertion 8).
//
// FILENAME: no hyphen. juce_add_binary_data STRIPS hyphens rather than
// converting them to underscores, so a second file named i18n-fr.js would have
// to be reached as the symbol i18nfr_js (critical_binary_data_strips_hyphens).
// One combined file for both languages sidesteps the question entirely.
//
// NO MARKUP. This table is data, never HTML. check-i18n assertion 9 rejects any
// innerHTML reference here and any string literal containing an opening angle
// bracket.
//
// ── WHERE THE COPY LIVES, AND WHY THE COUNT IS NOT THE EXTRACTOR'S ─────────
//
// scripts/i18n-extract.js skips js/tuning-panel.js BY FILENAME
// (i18n-extract.js:442), with no ownership test. That skip is correct for
// O-Wind, which consumes the MODULE file by reference from
// ${CMAKE_SOURCE_DIR}/modules/. It is wrong here: this plugin owns its copy —
// the header says "part of O-Bells", it is 279 lines diverged from
// modules/tuning/scala-tuning-engine/js/tuning-panel.js, and O-Bells has no
// dependencies.json listing the module, so /module-upgrade will not revert an
// edit to it. So the Tuning tab IS in scope and its ~34 strings are keyed here,
// on top of the 79 the extractor found in index.html.
//
// Half of that file IS reachable by the gates and half is not, which is worth
// stating precisely rather than repeating the dispatch's blanket claim that
// neither gate sees it:
//   - check-i18n assertions 12/13/15 DO scan js/tuning-panel.js — pageModules
//     is built from every top-level .js in the served js/ directory.
//   - assertion 10 does NOT, because it walks index.html's text nodes only.
//   - assertion 12's innerHTML arm reads only a literal sitting on the RHS of
//     the assignment, so the panel's `html += ...` accumulators (the interval
//     list, the matrix, the rotation table, the library list) are invisible to
//     it. They are keyed here regardless, and driven in the browser to prove it.
//
// ALL FRENCH IS MACHINE-DRAFTED AND FLAGGED `reviewed: false`. No native
// speaker has read it. `node scripts/check-i18n.js` prints the worklist.
//
// The tuning-panel and effects-chain French below is taken VERBATIM from
// O-IntonationPad v2.9.0, which localized the same panel and the same
// Chorus/Delay/EQ/Reverb chain. Two hand-copies of one panel disagreeing about
// the French would be a worse outcome than either translation alone.
// ============================================================================

export const LANGUAGES = ['en', 'fr'];

// ── HOVER-HELP (v4.3.0, Stage M batch M3) ──────────────────────────────────
//
// 63 parameter tips + 2 chrome tips = 65 entries, one per BOUND anchor below.
// v4.2.0 shipped this object EMPTY and TIP_BINDINGS as [], because the page had
// no renderer to paint what applyI18n() writes. v4.3.0 adds the renderer
// (index.html, setupTooltips()) and this copy in the same commit; either one
// alone is invisible.
//
// TITLE. The page's own caption, verbatim from LABELS, wherever the caption and
// the parameter name agree. Where the caption is that name with letters missing
// — P.Env Time, Pre-dly, Sub, Oct, Mid Freq, A4 REF — the tip spells it out:
// a 260px surface is exactly where an abbreviation stops being necessary, and
// the spelled form is also the automation-lane name. That is M2 finding 9's
// rule, and it is NOT the same as overruling a caption that disagrees.
//
// BODY. What the control does, when to reach for it, ending with the range and
// unit. Prose, so it takes FRENCH convention: decimal COMMA, a space before %,
// U+2212 for the minus. The READOUT keeps its point (0.50, 1.00) because D-03
// exempts the readout NODE and that has not moved. They differ on purpose.
//
// WHERE THE RANGES COME FROM. plugins/O-Bells/.planning/params.tsv is the
// runtime inventory and is authoritative for the parameter's own range. 45 of
// the 65 rows carry a real `label`; for the other 20 the unit is recovered from
// the page's OWN formatter and the file:line is cited on the entry. Three
// ranges are recovered even though a label exists, because the page's formatter
// disagrees with the raw range and the user reads the page:
//   - every 0..1 percentage renders 0-100 % (index.html:2034 and its 20 siblings);
//   - delayTime dumps `s` 0.001..2.000 and the knob renders MILLISECONDS
//     (index.html:3008, setupFxKnob('delayTime', ..., 1, 2000, ' ms'));
//   - unisonDetune / partialTuning dump `cents` and the readout writes `ct`
//     (index.html:2060, :2064) — the body says "cents" because that is the
//     word, and the readout's abbreviation is not prose.
//
// CHOICE OPTION STRINGS INSIDE A BODY. Bronze / Cast Iron / Click / Ping /
// Normal / PingPong / Linear stay in ENGLISH inside the French bodies, and this
// is deliberate rather than an oversight: they are AudioParameterChoice options
// the host shows in its automation lane and the page shows unlocalized under
// D-01 arm 1 (see I18N_EXEMPT). A body that translated them would name buttons
// that do not exist. The SENTENCE around them is French; the identifiers are
// not. The Off/On of a boolean is a different case — Off and On are not
// rendered anywhere as option text on this page, so the body says
// "Arrêt ou Marche", matching the ui.off / ui.on captions the FX buttons wear.
//
// TWO PARAMETERS HAVE NO ENTRY, AND THAT IS A FINDING, NOT A GAP:
// tuning_pitchBendRange and tuning_temperamentPreset are host-reachable and
// page-unreachable. See the note above TIP_BINDINGS.
//
// ALL FRENCH IS A MACHINE DRAFT, `reviewed: false` on all 65.
export const I18N = Object.freeze({

    // ── Instrument tab: Synthesis ───────────────────────────────────────────
    'tip.damping': {
        en: { t: 'Damping',
              b: 'Sets how quickly the hum and the prime — the two lowest partials — give up their energy once the strike and body stages have passed, and how fast the bell fades after the key is released: a 3 s release at 0 %, 0.5 s at 100 %. The higher partials are not touched; the stage times, Material and Acoustic Brightness set their decay. 0 to 100 %.' },
        fr: { t: 'Amortissement',
              b: 'Règle la vitesse à laquelle le bourdon et la prime — les deux partiels les plus graves — perdent leur énergie une fois les étapes de frappe et de corps passées, et la vitesse à laquelle la cloche s’éteint après le relâchement de la touche : relâchement de 3 s à 0 %, de 0,5 s à 100 %. Les partiels aigus ne sont pas touchés ; les durées d’étape, le matériau et la brillance acoustique règlent leur déclin. 0 à 100 %.',
              reviewed: false },
    },
    'tip.overtoneBrightness': {
        en: { t: 'Overtone Brightness',
              b: 'Balances the upper partials against the fundamental. Raise it for a glassy, bell-like shimmer; lower it for a darker, more wooden strike. 0 to 100 %.' },
        fr: { t: 'Brillance harmonique',
              b: 'Équilibre les partiels aigus par rapport au fondamental. Augmentez pour un miroitement vitreux ; diminuez pour une frappe plus sombre et boisée. 0 à 100 %.',
              reviewed: true },
    },
    'tip.acousticBrightness': {
        en: { t: 'Acoustic Brightness',
              b: 'Tilts the whole resonator towards the treble, the way a thinner casting or a harder alloy would. It colours the body rather than the strike. 0 to 100 %.' },
        fr: { t: 'Brillance acoustique',
              b: 'Incline tout le résonateur vers l’aigu, comme le ferait une fonte plus fine ou un alliage plus dur. Colore le corps plutôt que la frappe. 0 à 100 %.',
              reviewed: true },
    },
    // The option words stay English on BOTH sides — D-01 arm 1, and they are
    // also what the .param-value readout writes (arm 3). See I18N_EXEMPT.
    'tip.material': {
        en: { t: 'Material',
              b: 'Chooses the alloy the resonator models, which sets its partial ratios and its natural decay. Bronze is the orchestral tubular-bell voice; Cast Iron is the heaviest and the slowest to die away. Bronze, Brass, Steel, Aluminum or Cast Iron.' },
        fr: { t: 'Matériau',
              b: 'Choisit l’alliage modélisé par le résonateur, ce qui fixe ses rapports de partiels et sa décroissance naturelle. Bronze est la voix de cloche tubulaire d’orchestre ; Cast Iron est la plus lourde et la plus lente à s’éteindre. Bronze, Brass, Steel, Aluminum ou Cast Iron.',
              reviewed: true },
    },
    'tip.inharmonicity': {
        en: { t: 'Inharmonicity',
              b: 'Pushes the partials away from whole-number ratios, which is what makes a bell sound like a bell rather than an organ pipe. Too much and the pitch turns ambiguous. 0 to 100 %.' },
        fr: { t: 'Inharmonicité',
              b: 'Écarte les partiels des rapports entiers, ce qui donne à une cloche son timbre plutôt que celui d’un tuyau d’orgue. Trop d’inharmonicité rend la hauteur ambiguë. 0 à 100 %.',
              reviewed: true },
    },
    'tip.airAbsorption': {
        en: { t: 'Air Absorption',
              b: 'Models the high frequencies losing energy to the air as the note travels. It darkens the tail progressively rather than all at once. 0 to 100 %.' },
        fr: { t: 'Absorption air',
              b: 'Modélise la perte d’énergie des aigus dans l’air pendant que la note voyage. Assombrit la queue progressivement plutôt que d’un seul coup. 0 à 100 %.',
              reviewed: true },
    },
    // UNIT RECOVERED — params.tsv `label` is empty. index.html:2038-2042 renders
    // ms below one second and seconds above it.
    'tip.airAbsorptionTime': {
        en: { t: 'Air Time',
              b: 'Sets how long that air-absorption darkening takes to run its course. Short values dull the tail almost immediately; long values keep the top open for seconds. 100 ms to 10.0 s.' },
        fr: { t: 'Durée air',
              b: 'Règle le temps que met cet assombrissement par absorption de l’air à s’accomplir. Les valeurs courtes ternissent la queue presque aussitôt ; les longues gardent l’aigu ouvert pendant des secondes. 100 ms à 10,0 s.',
              reviewed: true },
    },
    // UNIT RECOVERED — empty `label`; index.html:2048 renders 25 + v*375 ms.
    'tip.bloomSpeed': {
        en: { t: 'Bloom Speed',
              b: 'Bloom is the swell that arrives just after the strike, as the partials come into phase. This sets how long that swell takes to arrive. 25 to 400 ms.' },
        fr: { t: 'Vitesse éclosion',
              b: 'L’éclosion est l’enflement qui suit immédiatement la frappe, quand les partiels se mettent en phase. Règle le temps que met cet enflement à venir. 25 à 400 ms.',
              reviewed: true },
    },
    'tip.bloomAmount': {
        en: { t: 'Bloom Amount',
              b: 'Sets how much of that post-strike swell is audible. At zero the bell simply decays from its loudest point. 0 to 100 %.' },
        fr: { t: 'Quantité éclosion',
              b: 'Règle la part audible de cet enflement après la frappe. À zéro, la cloche décroît simplement depuis son maximum. 0 à 100 %.',
              reviewed: true },
    },
    'tip.shimmer': {
        en: { t: 'Shimmer',
              b: 'Adds a slow beating between closely detuned partials — the flutter a real bell gets from its own asymmetry. 0 to 100 %.' },
        fr: { t: 'Shimmer',
              b: 'Ajoute un battement lent entre des partiels légèrement désaccordés — le frémissement qu’une vraie cloche tient de sa propre asymétrie. 0 à 100 %.',
              reviewed: true },
    },
    'tip.bloomFineEnabled': {
        en: { t: 'Bloom Fine Controls',
              b: 'Replaces the two main Bloom sliders with six per-band controls, so low, mid and high partials can bloom at different speeds and depths. The two main sliders are disabled while it is on. Off or On.' },
        fr: { t: 'Réglages fins d’éclosion',
              b: 'Remplace les deux curseurs principaux d’éclosion par six commandes par bande, afin que les partiels graves, médiums et aigus éclosent à des vitesses et des profondeurs différentes. Les deux curseurs principaux sont désactivés tant qu’il est actif. Arrêt ou Marche.',
              reviewed: true },
    },
    // UNIT RECOVERED for all three — empty `label`; index.html:2051, :2052,
    // :2053 render 15 + v*235, 25 + v*375 and 50 + v*750 milliseconds.
    'tip.bloomSpeedLow': {
        en: { t: 'Speed Low',
              b: 'Sets the bloom time for the low partials alone. Large bells swell slowest down here. Available only while Bloom Fine Controls is on. 15 to 250 ms.' },
        fr: { t: 'Vitesse grave',
              b: 'Règle le temps d’éclosion des seuls partiels graves. C’est ici que les grandes cloches enflent le plus lentement. Disponible seulement quand les réglages fins d’éclosion sont actifs. 15 à 250 ms.',
              reviewed: true },
    },
    'tip.bloomSpeedMid': {
        en: { t: 'Speed Mid',
              b: 'Sets the bloom time for the middle partials, the band that carries most of the perceived pitch. Available only while Bloom Fine Controls is on. 25 to 400 ms.' },
        fr: { t: 'Vitesse médium',
              b: 'Règle le temps d’éclosion des partiels médiums, la bande qui porte l’essentiel de la hauteur perçue. Disponible seulement quand les réglages fins d’éclosion sont actifs. 25 à 400 ms.',
              reviewed: true },
    },
    'tip.bloomSpeedHigh': {
        en: { t: 'Speed High',
              b: 'Sets the bloom time for the high partials. Slow values here make the top of the bell arrive noticeably after the strike. Available only while Bloom Fine Controls is on. 50 to 800 ms.' },
        fr: { t: 'Vitesse aigu',
              b: 'Règle le temps d’éclosion des partiels aigus. Une valeur lente fait arriver le haut de la cloche nettement après la frappe. Disponible seulement quand les réglages fins d’éclosion sont actifs. 50 à 800 ms.',
              reviewed: true },
    },
    'tip.bloomAmountLow': {
        en: { t: 'Amount Low',
              b: 'Sets how much bloom the low partials get. Available only while Bloom Fine Controls is on. 0 to 100 %.' },
        fr: { t: 'Quantité grave',
              b: 'Règle la quantité d’éclosion appliquée aux partiels graves. Disponible seulement quand les réglages fins d’éclosion sont actifs. 0 à 100 %.',
              reviewed: true },
    },
    'tip.bloomAmountMid': {
        en: { t: 'Amount Mid',
              b: 'Sets how much bloom the middle partials get. Available only while Bloom Fine Controls is on. 0 to 100 %.' },
        fr: { t: 'Quantité médium',
              b: 'Règle la quantité d’éclosion appliquée aux partiels médiums. Disponible seulement quand les réglages fins d’éclosion sont actifs. 0 à 100 %.',
              reviewed: true },
    },
    'tip.bloomAmountHigh': {
        en: { t: 'Amount High',
              b: 'Sets how much bloom the high partials get. Available only while Bloom Fine Controls is on. 0 to 100 %.' },
        fr: { t: 'Quantité aigu',
              b: 'Règle la quantité d’éclosion appliquée aux partiels aigus. Disponible seulement quand les réglages fins d’éclosion sont actifs. 0 à 100 %.',
              reviewed: true },
    },

    // ── Instrument tab: Ensemble ────────────────────────────────────────────
    // UNIT RECOVERED — empty `label`; index.html:2059 renders a bare count.
    'tip.unisonCount': {
        en: { t: 'Unison',
              b: 'Stacks slightly detuned copies of the bell for a wider, thicker strike. Every extra voice costs CPU on every note. 1 to 4 voices.' },
        fr: { t: 'Unisson',
              b: 'Empile des copies légèrement désaccordées de la cloche pour une frappe plus large et plus épaisse. Chaque voix supplémentaire coûte du processeur à chaque note. 1 à 4 voix.',
              reviewed: true },
    },
    'tip.unisonDetune': {
        en: { t: 'Detune',
              b: 'Spreads the unison voices apart in pitch. A little thickens; a lot produces a chorus and then sours. Has no effect at one voice. 0 to 50 cents.' },
        fr: { t: 'Désaccord',
              b: 'Écarte les voix d’unisson en hauteur. Un peu épaissit ; beaucoup produit un chorus puis une fausseté. Sans effet à une seule voix. 0 à 50 cents.',
              reviewed: true },
    },
    'tip.octaveBlendSub': {
        en: { t: 'Sub Octave',
              b: 'Blends in a copy of the bell an octave below, for weight underneath the strike. 0 to 100 %.' },
        fr: { t: 'Sous-octave',
              b: 'Mélange une copie de la cloche une octave plus bas, pour donner du poids sous la frappe. 0 à 100 %.',
              reviewed: true },
    },
    'tip.octaveBlendOct': {
        en: { t: 'Octave Up',
              b: 'Blends in a copy of the bell an octave above, for sparkle on the attack. 0 to 100 %.' },
        fr: { t: 'Octave supérieure',
              b: 'Mélange une copie de la cloche une octave plus haut, pour donner de l’éclat à l’attaque. 0 à 100 %.',
              reviewed: true },
    },
    'tip.stereoSpread': {
        en: { t: 'Spread',
              b: 'Places the unison and octave voices across the stereo field. At zero everything sits in the centre. 0 to 100 %.' },
        fr: { t: 'Étalement',
              b: 'Répartit les voix d’unisson et d’octave dans le champ stéréo. À zéro, tout reste au centre. 0 à 100 %.',
              reviewed: true },
    },

    // ── Instrument tab: Onsets ──────────────────────────────────────────────
    'tip.strikePosition': {
        en: { t: 'Strike',
              b: 'Moves the striking point along the bell. Near the rim the upper partials dominate; near the crown the fundamental does. 0 to 100 %.' },
        fr: { t: 'Frappe',
              b: 'Déplace le point de frappe le long de la cloche. Près du bord, les partiels aigus dominent ; près du sommet, c’est le fondamental. 0 à 100 %.',
              reviewed: true },
    },
    'tip.malletHardness': {
        en: { t: 'Mallet',
              b: 'Sets how hard the mallet head is. Soft heads excite the low partials and little else; hard heads put a bright click on the front of the note. 0 to 100 %.' },
        fr: { t: 'Maillet',
              b: 'Règle la dureté de la tête de maillet. Une tête douce n’excite guère que les partiels graves ; une tête dure place un clic brillant en tête de la note. 0 à 100 %.',
              reviewed: true },
    },
    'tip.attackLevel': {
        en: { t: 'Attack Amount',
              b: 'Sets how loud the strike transient is against the ringing body that follows it. 0 to 100 %.' },
        fr: { t: 'Quantité attaque',
              b: 'Règle le niveau du transitoire de frappe par rapport au corps résonant qui le suit. 0 à 100 %.',
              reviewed: true },
    },
    'tip.strikeNoiseChar': {
        en: { t: 'Noise',
              b: 'Chooses the character of the contact noise at the instant of the strike. Click is dry and wooden, Thud is dull and low, Ping is bright and metallic. Click, Thud or Ping.' },
        fr: { t: 'Bruit',
              b: 'Choisit le caractère du bruit de contact à l’instant de la frappe. Click est sec et boisé, Thud est sourd et grave, Ping est brillant et métallique. Click, Thud ou Ping.',
              reviewed: true },
    },
    // The BODY names the three buttons the page shows (Linear / Exp / Log), not
    // the parameter's own option list (Linear / Exponential / Logarithmic): the
    // user is reading the page. Exp and Log are keyed captions; Linear is the
    // one of the three that is byte-identical to its option and therefore exempt.
    'tip.velocityCurve': {
        en: { t: 'Velocity',
              b: 'Maps how hard you play onto how loud the bell speaks. Linear is one-to-one, Exp needs a firmer touch for the same level, and Log lifts quiet playing. Linear, Exp or Log.' },
        fr: { t: 'Vélocité',
              b: 'Détermine comment la force de jeu se traduit en niveau sonore. Linear est proportionnel, Exp demande un toucher plus ferme pour le même niveau, et Log relève le jeu doux. Linear, Exp ou Log.',
              reviewed: true },
    },

    // ── Instrument tab: Advanced ────────────────────────────────────────────
    'tip.partialTuning': {
        en: { t: 'Partial Tune',
              b: 'Shifts the third partial alone — the tierce, the partial that carries a church bell’s minor third — against the fundamental without moving the perceived pitch. The hum, the prime and the higher partials stay where they are. It is the shortest road from a tuned bell to a clangorous one. −100 to +100 cents.' },
        fr: { t: 'Accord des partiels',
              b: 'Ne décale que le troisième partiel — la tierce, le partiel qui porte la tierce mineure de la cloche d’église — par rapport au fondamental, sans déplacer la hauteur perçue. Le bourdon, la prime et les partiels aigus restent en place. C’est le chemin le plus court d’une cloche juste à une cloche au timbre discordant. −100 à +100 cents.',
              reviewed: false },
    },
    'tip.pitchEnvelope': {
        en: { t: 'Pitch Envelope',
              b: 'Sets how far the pitch falls away from its struck value in the first instants of the note — the drop a heavy bell makes as it settles. 0 to 100 %.' },
        fr: { t: 'Enveloppe de hauteur',
              b: 'Règle l’ampleur de la chute de hauteur dans les premiers instants de la note — l’affaissement d’une cloche lourde qui se stabilise. 0 à 100 %.',
              reviewed: true },
    },
    'tip.pitchEnvTime': {
        en: { t: 'Pitch Envelope Time',
              b: 'Sets how long that pitch drop takes to complete. Short values read as a click on the attack; long values as an audible bend. 5 to 200 ms.' },
        fr: { t: 'Durée d’enveloppe de hauteur',
              b: 'Règle la durée de cette chute de hauteur. Les valeurs courtes s’entendent comme un clic sur l’attaque ; les longues comme un glissando audible. 5 à 200 ms.',
              reviewed: true },
    },
    'tip.nonlinearEffects': {
        en: { t: 'Nonlinear',
              b: 'Adds the amplitude-dependent coupling a real bell shows when it is struck hard: partials trade energy and the timbre changes with level. 0 to 100 %.' },
        fr: { t: 'Non linéaire',
              b: 'Ajoute le couplage dépendant de l’amplitude qu’une vraie cloche manifeste sous une frappe forte : les partiels échangent de l’énergie et le timbre change avec le niveau. 0 à 100 %.',
              reviewed: true },
    },

    // ── Instrument tab: Multi-Stage Envelope ────────────────────────────────
    'tip.strikeTime': {
        en: { t: 'Strike Time',
              b: 'Sets the length of the first envelope stage — the strike itself — before the body of the note takes over. 5 to 100 ms.' },
        fr: { t: 'Durée frappe',
              b: 'Règle la durée de la première étape d’enveloppe — la frappe elle-même — avant que le corps de la note prenne le relais. 5 à 100 ms.',
              reviewed: true },
    },
    'tip.brilliance': {
        en: { t: 'Brilliance',
              b: 'Sets how long the highest partials survive into the decay. Low values let the top die first, which is what a large bell does. 0 to 100 %.' },
        fr: { t: 'Brillance',
              b: 'Règle la durée de survie des partiels les plus aigus dans la décroissance. Une valeur basse laisse l’aigu mourir en premier, comme le fait une grande cloche. 0 à 100 %.',
              reviewed: true },
    },
    // UNIT RECOVERED for the upper end — params.tsv gives `ms` 100..5000 and
    // index.html:2074-2077 switches the readout to seconds at 1000 ms.
    'tip.bodyTime': {
        en: { t: 'Body Time',
              b: 'Sets the length of the middle envelope stage — the ringing body between the strike and the hum. 100 ms to 5.0 s.' },
        fr: { t: 'Durée corps',
              b: 'Règle la durée de l’étape médiane de l’enveloppe — le corps résonant entre la frappe et le bourdon. 100 ms à 5,0 s.',
              reviewed: true },
    },
    'tip.humSustain': {
        en: { t: 'Hum Sustain',
              b: 'Sets how long the hum note holds — the lowest partial, an octave under the strike tone — once everything above it has gone. 0 to 100 %.' },
        fr: { t: 'Maintien bourdon',
              b: 'Règle la durée de maintien du bourdon — le partiel le plus grave, une octave sous le son de frappe — une fois que tout ce qui est au-dessus a disparu. 0 à 100 %.',
              reviewed: true },
    },

    // ── Instrument tab: Filter ──────────────────────────────────────────────
    'tip.lpFilterEnabled': {
        en: { t: 'LP Filter',
              b: 'Switches a low-pass filter into the output path, for taming the top of a bright bell without retuning it. Off or On.' },
        fr: { t: 'Filtre passe-bas',
              b: 'Insère un filtre passe-bas dans le trajet de sortie, pour adoucir l’aigu d’une cloche brillante sans la réaccorder. Arrêt ou Marche.',
              reviewed: true },
    },
    // UNIT RECOVERED — empty `label`; index.html:2080-2084 renders Hz below one
    // kilohertz and kHz above it.
    'tip.lpFilterCutoff': {
        en: { t: 'Cutoff',
              b: 'Sets where that low-pass filter starts to cut. It does nothing until LP Filter is switched on. 200 Hz to 20.0 kHz.' },
        fr: { t: 'Coupure',
              b: 'Règle la fréquence à laquelle ce filtre passe-bas commence à couper. Sans effet tant que le filtre passe-bas n’est pas activé. 200 Hz à 20,0 kHz.',
              reviewed: true },
    },

    // ── Instrument tab: Performance ─────────────────────────────────────────
    // The BODY is the v4.1.5 .toggle-tooltip prose VERBATIM, both languages,
    // plus the range sentence every body ends with. That bespoke :hover-only
    // note was DELETED in v4.3.0 — it was a second hover surface at z-index 100
    // and would have painted alongside this one. No new prose invented, which
    // is the same rule contract §4 applies to a native title=.
    'tip.highFidelity': {
        en: { t: 'High Fidelity',
              b: 'Disables voice culling for maximum sustain fidelity. May cause CPU overload with long-decay presets and dense polyphony. Off or On.' },
        fr: { t: 'Haute fidélité',
              b: 'Désactive l’élagage des voix pour une fidélité maximale du maintien. Peut surcharger le processeur avec des préréglages à longue décroissance et une polyphonie dense. Arrêt ou Marche.',
              reviewed: true },
    },

    // ── Instrument tab: Output ──────────────────────────────────────────────
    'tip.humanize': {
        en: { t: 'Humanize',
              b: 'Varies timing, level and tuning very slightly from note to note, so repeated strikes stop sounding identical. 0 to 100 %.' },
        fr: { t: 'Humanisation',
              b: 'Fait varier très légèrement le placement, le niveau et l’accord d’une note à l’autre, pour que les frappes répétées cessent d’être identiques. 0 à 100 %.',
              reviewed: true },
    },
    'tip.outputGain': {
        en: { t: 'Gain',
              b: 'Sets the level leaving the plugin, after the whole effects chain. −24 to +12 dB.' },
        fr: { t: 'Gain',
              b: 'Règle le niveau en sortie, après toute la chaîne d’effets. −24 à +12 dB.',
              reviewed: true },
    },

    // ── Tuning tab (js/tuning-panel.js, lazily imported) ────────────────────
    // These two anchors live in the panel imported at index.html:3051, which is
    // absent from the DOM when initI18n() runs. They bind on the
    // window.__reapplyI18n() the panel's own init calls once it has mounted —
    // see the note above TIP_BINDINGS, which records the cost.
    //
    // A4 stays A4 in the French body. Letter pitch notation is deliberately not
    // localized anywhere on this page (see the note above I18N_EXEMPT); writing
    // "la3" here would name something the panel does not say.
    'tip.tuning_masterTune': {
        en: { t: 'A4 Reference',
              b: 'Sets the reference frequency the whole tuning is built from — A4, the concert-pitch anchor. Drag the knob up or down. 400.0 to 480.0 Hz.' },
        fr: { t: 'Référence A4',
              b: 'Règle la fréquence de référence sur laquelle repose tout l’accord — A4, l’ancrage du diapason. Glissez le bouton vers le haut ou vers le bas. 400,0 à 480,0 Hz.',
              reviewed: true },
    },
    // UNIT RECOVERED — empty `label`, and the value is a RATIO with no unit:
    // js/tuning-panel.js:972 renders it as a bare two-decimal number.
    'tip.tuning_octaveStretch': {
        en: { t: 'Octave Stretch',
              b: 'Widens or narrows every octave by a fixed ratio, the way a piano is stretch-tuned so its inharmonic partials agree. 1.00 leaves the octave pure. 0.95 to 1.25.' },
        fr: { t: 'Étirement d’octave',
              b: 'Élargit ou resserre chaque octave d’un rapport fixe, comme un piano dont l’accord étiré fait concorder les partiels inharmoniques. 1,00 laisse l’octave pure. 0,95 à 1,25.',
              reviewed: true },
    },

    // ── Effects tab: Chorus ─────────────────────────────────────────────────
    // A bypass button's own caption is Marche / Arrêt (ui.on / ui.off), so the
    // tip is titled with the SECTION it switches, which is what sits beside it.
    'tip.chorusBypass': {
        en: { t: 'Chorus',
              b: 'Switches the chorus section in and out of the signal path. The button reads On while the effect is running. Off or On.' },
        fr: { t: 'Chorus',
              b: 'Insère ou retire la section chorus du trajet du signal. Le bouton affiche Marche tant que l’effet fonctionne. Arrêt ou Marche.',
              reviewed: true },
    },
    'tip.chorusRate': {
        en: { t: 'Chorus Rate',
              b: 'Sets how fast the chorus delay is modulated. Slow settings widen the image; fast settings warble. 0.10 to 10.00 Hz.' },
        fr: { t: 'Vitesse du chorus',
              b: 'Règle la vitesse de modulation du retard du chorus. Les réglages lents élargissent l’image ; les rapides font trembler. 0,10 à 10,00 Hz.',
              reviewed: true },
    },
    'tip.chorusDepth': {
        en: { t: 'Chorus Depth',
              b: 'Sets how far the chorus delay swings. A little thickens; a lot detunes audibly. 0 to 100 %.' },
        fr: { t: 'Profondeur du chorus',
              b: 'Règle l’amplitude du balayage du retard du chorus. Un peu épaissit ; beaucoup désaccorde de façon audible. 0 à 100 %.',
              reviewed: true },
    },
    'tip.chorusMix': {
        en: { t: 'Chorus Mix',
              b: 'Balances the chorused signal against the dry bell. 0 to 100 %.' },
        fr: { t: 'Mix du chorus',
              b: 'Équilibre le signal traité par le chorus et la cloche sèche. 0 à 100 %.',
              reviewed: true },
    },

    // ── Effects tab: Delay ──────────────────────────────────────────────────
    'tip.delayBypass': {
        en: { t: 'Delay',
              b: 'Switches the delay section in and out of the signal path. The button reads On while the effect is running. Off or On.' },
        fr: { t: 'Délai',
              b: 'Insère ou retire la section de délai du trajet du signal. Le bouton affiche Marche tant que l’effet fonctionne. Arrêt ou Marche.',
              reviewed: true },
    },
    // RANGE RECOVERED FROM THE PAGE, against the dump. params.tsv gives `s`
    // 0.001..2.000; the knob is built with a 1..2000 ms display range and a
    // ' ms' suffix at index.html:2955. The user reads the knob.
    'tip.delayTime': {
        en: { t: 'Delay Time',
              b: 'Sets the gap between the bell and its first echo. 1 to 2000 ms.' },
        fr: { t: 'Durée du délai',
              b: 'Règle l’écart entre la cloche et son premier écho. 1 à 2000 ms.',
              reviewed: true },
    },
    'tip.delayFeedback': {
        en: { t: 'Delay Feedback',
              b: 'Sets how much of each echo is fed back to make the next one. High values build long trails that fade slowly. 0 to 95 %.' },
        fr: { t: 'Réinjection du délai',
              b: 'Règle la part de chaque écho réinjectée pour produire le suivant. Les valeurs élevées créent de longues traînées qui s’effacent lentement. 0 à 95 %.',
              reviewed: true },
    },
    'tip.delayMode': {
        en: { t: 'Delay Mode',
              b: 'Chooses how the repeats are placed in the stereo field. Normal keeps each echo where the bell was; PingPong alternates them left and right. Normal or PingPong.' },
        fr: { t: 'Mode de délai',
              b: 'Choisit le placement des répétitions dans le champ stéréo. Normal garde chaque écho là où était la cloche ; PingPong les alterne à gauche et à droite. Normal ou PingPong.',
              reviewed: true },
    },
    'tip.delayMix': {
        en: { t: 'Delay Mix',
              b: 'Balances the echoes against the dry bell. 0 to 100 %.' },
        fr: { t: 'Mix du délai',
              b: 'Équilibre les échos et la cloche sèche. 0 à 100 %.',
              reviewed: true },
    },

    // ── Effects tab: EQ ─────────────────────────────────────────────────────
    'tip.eqBypass': {
        en: { t: 'EQ',
              b: 'Switches the three-band equaliser in and out of the signal path. The button reads On while the effect is running. Off or On.' },
        fr: { t: 'EQ',
              b: 'Insère ou retire l’égaliseur trois bandes du trajet du signal. Le bouton affiche Marche tant que l’effet fonctionne. Arrêt ou Marche.',
              reviewed: true },
    },
    'tip.eqLowGain': {
        en: { t: 'EQ Low',
              b: 'Cuts or boosts the low shelf, where the hum note lives. −12 to +12 dB.' },
        fr: { t: 'EQ grave',
              b: 'Atténue ou accentue le plateau grave, là où se trouve le bourdon. −12 à +12 dB.',
              reviewed: true },
    },
    'tip.eqMidGain': {
        en: { t: 'EQ Mid',
              b: 'Cuts or boosts a peaking band centred on the Mid Freq knob beside it. −12 to +12 dB.' },
        fr: { t: 'EQ médium',
              b: 'Atténue ou accentue une bande en cloche centrée sur la fréquence réglée par le bouton Fréq. méd. voisin. −12 à +12 dB.',
              reviewed: true },
    },
    'tip.eqMidFreq': {
        en: { t: 'EQ Mid Frequency',
              b: 'Sets where the mid band sits. Sweep it to find the ringing partial you want to lift or tame. 200 to 8000 Hz.' },
        fr: { t: 'Fréquence médium de l’EQ',
              b: 'Règle la position de la bande médium. Balayez-la pour trouver le partiel résonant à relever ou à adoucir. 200 à 8000 Hz.',
              reviewed: true },
    },
    'tip.eqHighGain': {
        en: { t: 'EQ High',
              b: 'Cuts or boosts the high shelf, where the strike noise and the brightest partials sit. −12 to +12 dB.' },
        fr: { t: 'EQ aigu',
              b: 'Atténue ou accentue le plateau aigu, là où se trouvent le bruit de frappe et les partiels les plus brillants. −12 à +12 dB.',
              reviewed: true },
    },

    // ── Effects tab: Reverb ─────────────────────────────────────────────────
    'tip.reverbBypass': {
        en: { t: 'Reverb',
              b: 'Switches the reverb section in and out of the signal path. The button reads On while the effect is running. Off or On.' },
        fr: { t: 'Réverb',
              b: 'Insère ou retire la section de réverbération du trajet du signal. Le bouton affiche Marche tant que l’effet fonctionne. Arrêt ou Marche.',
              reviewed: true },
    },
    'tip.reverbSize': {
        en: { t: 'Reverb Size',
              b: 'Sets how large the modelled space is, and with it how long the tail runs. 0 to 100 %.' },
        fr: { t: 'Taille de la réverb',
              b: 'Règle la taille de l’espace modélisé, et par là même la longueur de la queue. 0 à 100 %.',
              reviewed: true },
    },
    'tip.reverbDamp': {
        en: { t: 'Reverb Damp',
              b: 'Sets how fast the high frequencies disappear from the tail, as soft furnishings would take them. 0 to 100 %.' },
        fr: { t: 'Amortissement de la réverb',
              b: 'Règle la vitesse à laquelle les aigus disparaissent de la queue, comme le feraient des matériaux absorbants. 0 à 100 %.',
              reviewed: true },
    },
    'tip.reverbPredelay': {
        en: { t: 'Reverb Pre-delay',
              b: 'Holds the reverb back after the strike, which keeps the attack clear and pushes the room further away. 0 to 200 ms.' },
        fr: { t: 'Pré-délai de la réverb',
              b: 'Retarde la réverbération après la frappe, ce qui garde l’attaque nette et éloigne la pièce. 0 à 200 ms.',
              reviewed: true },
    },
    'tip.reverbMod': {
        en: { t: 'Reverb Mod',
              b: 'Moves the reverb’s internal delays slowly, which stops a long tail settling into a metallic ring. 0 to 100 %.' },
        fr: { t: 'Modulation de la réverb',
              b: 'Déplace lentement les retards internes de la réverbération, ce qui empêche une longue queue de se figer en résonance métallique. 0 à 100 %.',
              reviewed: true },
    },
    'tip.reverbShimmer': {
        en: { t: 'Reverb Shimmer',
              b: 'Feeds an octave-up copy of the tail back into the reverb, so the decay rises instead of only fading. 0 to 100 %.' },
        fr: { t: 'Shimmer de la réverb',
              b: 'Réinjecte dans la réverbération une copie de la queue transposée à l’octave supérieure, de sorte que la décroissance monte au lieu de seulement s’effacer. 0 à 100 %.',
              reviewed: true },
    },
    'tip.reverbMix': {
        en: { t: 'Reverb Mix',
              b: 'Balances the reverb against the dry bell. 0 to 100 %.' },
        fr: { t: 'Mix de la réverb',
              b: 'Équilibre la réverbération et la cloche sèche. 0 à 100 %.',
              reviewed: true },
    },

    // ── Chrome ──────────────────────────────────────────────────────────────
    // This body describes what the popover ACTUALLY holds — one row, the
    // language selector — and nothing else. O-Tapestop's wording promises a
    // hover-help on/off switch that this plugin does not have, and a tip that
    // lies is worse than no tip.
    'tip.gearBtn': {
        en: { t: 'Settings',
              b: 'Opens the settings panel below the gear. It holds one control: the interface language. Value readouts stay in English whichever language is chosen.' },
        fr: { t: 'Réglages',
              b: 'Ouvre le panneau de réglages sous l’engrenage. Il contient une seule commande : la langue de l’interface. Les valeurs affichées restent en anglais quelle que soit la langue choisie.',
              reviewed: true },
    },
    'tip.langSelect': {
        en: { t: 'Language',
              b: 'Switches every caption and every hover-help note on the page between English and French. Value readouts stay in English so they keep matching the host’s automation lane. English or Français.' },
        fr: { t: 'Langue',
              b: 'Bascule chaque libellé et chaque bulle d’aide de la page entre l’anglais et le français. Les valeurs affichées restent en anglais afin de continuer à correspondre à la piste d’automation de l’hôte. English ou Français.',
              reviewed: true },
    },
});

export const LABELS = Object.freeze({

    // ── Header: preset browser ──────────────────────────────────────────────
    // The two arrows and the name display are NOT keyed: the arrows are glyphs
    // and the name display shows a preset name, which is the JSON filename
    // (D-02, and I18N_EXEMPT below).
    'label.presetSave':  { en: { t: 'Save' }, fr: { t: 'Enreg.', reviewed: true } },
    'label.presetLoad':  { en: { t: 'Load' }, fr: { t: 'Ouvrir', reviewed: true } },

    // ── Tab row ─────────────────────────────────────────────────────────────
    'label.tabInstrument': { en: { t: 'Instrument' }, fr: { t: 'Instrument', reviewed: true, sameAsEn: true } },
    'label.tabTuning':     { en: { t: 'Tuning' },     fr: { t: 'Accord',     reviewed: true } },
    'label.tabEffects':    { en: { t: 'Effects' },    fr: { t: 'Effets',     reviewed: true } },

    // ── The CPU warning banner, SPLIT — and the readout moved to the FRONT ──
    // v4.1.5 wrote this as one .cpu-warning-text span holding two text nodes
    // with the live estimate element BETWEEN them: "Estimated bell decay: ~18s.
    // Long decays...". Keying that span whole would make applyLabel's
    // `el.textContent = s` delete the estimate — contract §5's split rule, and
    // the extractor flagged both halves as UNSURE for exactly this reason.
    //
    // Splitting it into prefix + readout + suffix was the first attempt and it
    // FAILED the geometry diff: the French prefix is 54.41px wider than the
    // English one, so the readout moved by exactly that, and no word choice can
    // land two translations within the gate's 0.5px tolerance. Contract §6
    // authors the copy around a constraint rather than engineering around it, so
    // the readout now LEADS — its x is the start of the text block and is
    // language-invariant — and everything after it is this one keyed span, which
    // the diff excludes because it is a label. The number keeps its own bold
    // .cpu-warning-estimate styling, which folding the whole banner into a single
    // composed {s} label would have thrown away.
    'label.cpuWarning': { en: { t: 'estimated bell decay. Long decays with polyphony may cause high CPU usage, distortion, or stuttering.' },
                          fr: { t: 'de décroissance estimée. Les décroissances longues avec polyphonie peuvent provoquer une charge CPU élevée, de la distorsion ou des coupures.', reviewed: true } },

    // ── Instrument tab: Synthesis ───────────────────────────────────────────
    'label.secSynthesis':       { en: { t: 'Synthesis' },           fr: { t: 'Synthèse', reviewed: true } },
    'label.damping':            { en: { t: 'Damping' },             fr: { t: 'Amortissement', reviewed: true } },
    'label.overtoneBrightness': { en: { t: 'Overtone Brightness' }, fr: { t: 'Brillance harmonique', reviewed: true } },
    'label.acousticBrightness': { en: { t: 'Acoustic Brightness' }, fr: { t: 'Brillance acoustique', reviewed: true } },
    'label.material':           { en: { t: 'Material' },            fr: { t: 'Matériau', reviewed: true } },
    'label.inharmonicity':      { en: { t: 'Inharmonicity' },       fr: { t: 'Inharmonicité', reviewed: true } },
    'label.airAbsorption':      { en: { t: 'Air Absorption' },      fr: { t: 'Absorption air', reviewed: true } },
    'label.airTime':            { en: { t: 'Air Time' },            fr: { t: 'Durée air', reviewed: true } },

    // "Bloom" is the bell-acoustics term for the swell that follows the strike.
    // "Éclosion" is the French rendering; it is NOT kept in English the way
    // "Shimmer" is, because "shimmer" is a shipped effect NAME across this suite
    // and "bloom" here is a described behaviour.
    'label.bloomSpeed':     { en: { t: 'Bloom Speed' },  fr: { t: 'Vitesse éclosion', reviewed: true } },
    'label.bloomAmount':    { en: { t: 'Bloom Amount' }, fr: { t: 'Quantité éclosion', reviewed: true } },
    'label.shimmer':        { en: { t: 'Shimmer' },      fr: { t: 'Shimmer', reviewed: true, sameAsEn: true } },
    'label.bloomFineToggle': { en: { t: 'Bloom Fine Controls (Override Mode)' },
                               fr: { t: 'Réglages fins d’éclosion (mode prioritaire)', reviewed: true } },
    'label.bloomFineHint':  { en: { t: 'Per-band control - main sliders disabled when active' },
                              fr: { t: 'Réglage par bande - curseurs principaux désactivés', reviewed: true } },
    'label.speedLow':       { en: { t: 'Speed Low' },   fr: { t: 'Vitesse grave', reviewed: true } },
    'label.speedMid':       { en: { t: 'Speed Mid' },   fr: { t: 'Vitesse médium', reviewed: true } },
    'label.speedHigh':      { en: { t: 'Speed High' },  fr: { t: 'Vitesse aigu', reviewed: true } },
    'label.amountLow':      { en: { t: 'Amount Low' },  fr: { t: 'Quantité grave', reviewed: true } },
    'label.amountMid':      { en: { t: 'Amount Mid' },  fr: { t: 'Quantité médium', reviewed: true } },
    'label.amountHigh':     { en: { t: 'Amount High' }, fr: { t: 'Quantité aigu', reviewed: true } },

    // ── Instrument tab: Ensemble ────────────────────────────────────────────
    'label.secEnsemble': { en: { t: 'Ensemble' }, fr: { t: 'Ensemble', reviewed: true, sameAsEn: true } },
    'label.unison':      { en: { t: 'Unison' },   fr: { t: 'Unisson',  reviewed: true } },
    'label.detune':      { en: { t: 'Detune' },   fr: { t: 'Désacc.',  reviewed: true } },
    // "Sub" and "Oct" are the octave-below and octave-above blend amounts. Both
    // are the same truncation in French (sub-octave, octave), so both carry
    // sameAsEn rather than a fabricated difference.
    'label.sub':         { en: { t: 'Sub' },      fr: { t: 'Sub',      reviewed: true, sameAsEn: true } },
    'label.oct':         { en: { t: 'Oct' },      fr: { t: 'Oct',      reviewed: true, sameAsEn: true } },
    'label.spread':      { en: { t: 'Spread' },   fr: { t: 'Étalement', reviewed: true } },

    // ── Instrument tab: Onsets ──────────────────────────────────────────────
    'label.secOnsets':    { en: { t: 'Onsets' },        fr: { t: 'Attaques', reviewed: true } },
    'label.strike':       { en: { t: 'Strike' },        fr: { t: 'Frappe', reviewed: true } },
    'label.mallet':       { en: { t: 'Mallet' },        fr: { t: 'Maillet', reviewed: true } },
    'label.attackAmount': { en: { t: 'Attack Amount' }, fr: { t: 'Quantité attaque', reviewed: true } },
    'label.noise':        { en: { t: 'Noise' },         fr: { t: 'Bruit', reviewed: true } },
    'label.velocity':     { en: { t: 'Velocity' },      fr: { t: 'Vélocité', reviewed: true } },
    // The velocityCurve group is three buttons over ONE AudioParameterChoice
    // whose options are Linear / Exponential / Logarithmic. Only the first
    // caption is byte-identical to its option, so only that one is exempt under
    // D-01 arm 1; "Exp" and "Log" are the plugin's own abbreviations and are
    // plain captions that localize. French takes a point on a truncated
    // abbreviation, which is what makes these genuinely different strings and
    // not a sameAsEn pair.
    'label.velExp':       { en: { t: 'Exp' }, fr: { t: 'Exp.', reviewed: true } },
    'label.velLog':       { en: { t: 'Log' }, fr: { t: 'Log.', reviewed: true } },

    // ── Instrument tab: Advanced ────────────────────────────────────────────
    'label.secAdvanced': { en: { t: 'Advanced' },     fr: { t: 'Avancé', reviewed: true } },
    'label.partialTune': { en: { t: 'Partial Tune' }, fr: { t: 'Accord des partiels', reviewed: true } },
    'label.pitchEnv':    { en: { t: 'Pitch Env' },    fr: { t: 'Env. hauteur', reviewed: true } },
    // The English is ALREADY abbreviated to fit a quarter-width cell, so the
    // French is held to the same budget rather than spelled out.
    'label.pEnvTime':    { en: { t: 'P.Env Time' },   fr: { t: 'Durée env.', reviewed: true } },
    'label.nonlinear':   { en: { t: 'Nonlinear' },    fr: { t: 'Non linéaire', reviewed: true } },

    // ── Instrument tab: Multi-Stage Envelope ────────────────────────────────
    'label.secEnvelope':  { en: { t: 'Multi-Stage Envelope' }, fr: { t: 'Enveloppe multi-étages', reviewed: true } },
    'label.envelopeHint': { en: { t: 'Controls how different frequencies decay over time' },
                            fr: { t: 'Règle la décroissance des différentes fréquences dans le temps', reviewed: true } },
    'label.strikeTime':   { en: { t: 'Strike Time' }, fr: { t: 'Durée frappe', reviewed: true } },
    'label.brilliance':   { en: { t: 'Brilliance' },  fr: { t: 'Brillance', reviewed: true } },
    'label.bodyTime':     { en: { t: 'Body Time' },   fr: { t: 'Durée corps', reviewed: true } },
    'label.humSustain':   { en: { t: 'Hum Sustain' }, fr: { t: 'Maintien bourdon', reviewed: true } },

    // ── Instrument tab: Filter / Performance / Output ───────────────────────
    'label.secFilter':      { en: { t: 'Filter' },    fr: { t: 'Filtre', reviewed: true } },
    'label.lpFilter':       { en: { t: 'LP Filter' }, fr: { t: 'Filtre PB', reviewed: true } },
    'label.cutoff':         { en: { t: 'Cutoff' },    fr: { t: 'Coupure', reviewed: true } },
    'label.secPerformance': { en: { t: 'Performance' }, fr: { t: 'Performance', reviewed: true, sameAsEn: true } },
    'label.highFidelity':   { en: { t: 'High Fidelity' }, fr: { t: 'Haute fidélité', reviewed: true } },
    // RETIRED IN v4.3.0 — `label.hiFiNote` used to key the .hi-fi-toggle's own
    // bespoke :hover-only note (a .toggle-tooltip div at z-index 100, shown by
    // a CSS :hover rule). That was a SECOND hover surface: with the new
    // renderer in place, hovering High Fidelity would have painted both at
    // once. The div and its three CSS rules are deleted and the sentence moved
    // VERBATIM, in both languages, into tip.highFidelity's body — the same
    // "reuse the existing wording, invent nothing" rule contract §4 applies to
    // a native title=. Leaving the key here would fail assertion 15 as a dead
    // LABELS entry, so it is gone rather than orphaned.
    'label.secOutput':      { en: { t: 'Output' },   fr: { t: 'Sortie', reviewed: true } },
    'label.humanize':       { en: { t: 'Humanize' }, fr: { t: 'Humanisation', reviewed: true } },
    'label.level':          { en: { t: 'Level' },    fr: { t: 'Niveau', reviewed: true } },

    // ── Effects tab ─────────────────────────────────────────────────────────
    // Same chain, same French as O-IntonationPad v2.9.0.
    'label.fxChorus': { en: { t: 'Chorus' }, fr: { t: 'Chorus',  reviewed: true, sameAsEn: true } },
    'label.fxDelay':  { en: { t: 'Delay' },  fr: { t: 'Délai',   reviewed: true } },
    'label.fxEq':     { en: { t: 'EQ' },     fr: { t: 'EQ',      reviewed: true, sameAsEn: true } },
    'label.fxReverb': { en: { t: 'Reverb' }, fr: { t: 'Réverb', reviewed: true } },
    'label.rate':     { en: { t: 'Rate' },     fr: { t: 'Vitesse', reviewed: true } },
    'label.depth':    { en: { t: 'Depth' },    fr: { t: 'Prof.',   reviewed: true } },
    'label.mix':      { en: { t: 'Mix' },      fr: { t: 'Mix',  reviewed: true, sameAsEn: true } },
    'label.time':     { en: { t: 'Time' },     fr: { t: 'Durée',   reviewed: true } },
    'label.feedback': { en: { t: 'Feedback' }, fr: { t: 'Réinj.',  reviewed: true } },
    'label.low':      { en: { t: 'Low' },      fr: { t: 'Grave',   reviewed: true } },
    'label.mid':      { en: { t: 'Mid' },      fr: { t: 'Médium',  reviewed: true } },
    'label.midFreq':  { en: { t: 'Mid Freq' }, fr: { t: 'Fréq. méd.', reviewed: true } },
    'label.high':     { en: { t: 'High' },     fr: { t: 'Aigu',    reviewed: true } },
    'label.size':     { en: { t: 'Size' },     fr: { t: 'Taille',  reviewed: true } },
    'label.damp':     { en: { t: 'Damp' },     fr: { t: 'Amort.', reviewed: true } },
    'label.preDly':   { en: { t: 'Pre-dly' },  fr: { t: 'Pré-dél.', reviewed: true } },
    'label.mod':      { en: { t: 'Mod' },      fr: { t: 'Mod',     reviewed: true, sameAsEn: true } },
    // Worn by the delay-mode dropdown caption AND by the rotation table's first
    // column header in the tuning panel. One string, one key, two anchors.
    'label.mode':     { en: { t: 'Mode' },     fr: { t: 'Mode',    reviewed: true, sameAsEn: true } },
    // The two faces of the four FX bypass buttons. Written ONLY by setLabel,
    // from an if/else and never a ternary — check-i18n assertion 13 rejects a
    // conditional inside a setLabel argument (contract §6).
    'ui.on':          { en: { t: 'On' },  fr: { t: 'Marche', reviewed: true } },
    'ui.off':         { en: { t: 'Off' }, fr: { t: 'Arrêt',  reviewed: true } },

    // ── Footer ──────────────────────────────────────────────────────────────
    'label.gain':         { en: { t: 'Gain' }, fr: { t: 'Gain', reviewed: true, sameAsEn: true } },
    // The key NAMES stay: Z-M and Q-P are physical QWERTY positions the page
    // maps by e.key, not words. Only the sentence around them moves.
    'label.keyboardHelp': { en: { t: 'Click or use Z-M, Q-P keys' },
                            fr: { t: 'Cliquer ou utiliser les touches Z-M, Q-P', reviewed: true } },

    // ── The settings popover (new in v4.2.0) ────────────────────────────────
    'label.language': { en: { t: 'Language' }, fr: { t: 'Langue', reviewed: true } },

    // ── Tuning panel (js/tuning-panel.js) ───────────────────────────────────
    // PARAMETERISED entries, written through data-i18n-vars. Contract §6: the
    // inflection is authored AROUND, not engineered. English pluralizes zero as
    // "0 notes" and French as "0 note", so the noun moved in front of the
    // number and the count now stands alone — correct at 0, 1 and n in both
    // languages with no plural engine anywhere.
    'label.intervalsHeader': { en: { t: 'Intervals: {n}' }, fr: { t: 'Intervalles : {n}', reviewed: true } },
    'label.noteCount':       { en: { t: 'Notes: {n}' },     fr: { t: 'Notes : {n}',       reviewed: true } },
    'label.tonic':           { en: { t: 'Tonic' },          fr: { t: 'Tonique', reviewed: true } },
    'label.scaleIntervals':  { en: { t: 'Scale Intervals' }, fr: { t: 'Intervalles de la gamme', reviewed: true } },
    'label.vizCircle':       { en: { t: 'Circle' },    fr: { t: 'Cercle',   reviewed: true } },
    'label.vizPolar':        { en: { t: 'Polar' },     fr: { t: 'Polaire',  reviewed: true } },
    'label.vizMatrix':       { en: { t: 'Matrix' },    fr: { t: 'Matrice',  reviewed: true } },
    'label.vizTrueKeys':     { en: { t: 'True Keys' }, fr: { t: 'Touches',  reviewed: true } },
    'label.vizRotation':     { en: { t: 'Rotation' },  fr: { t: 'Rotation', reviewed: true, sameAsEn: true } },
    'label.tkHint':          { en: { t: 'Hold 2+ notes to see intervals' },
                               fr: { t: 'Tenir 2 notes ou plus pour voir les intervalles', reviewed: true } },
    'label.totalSpan':       { en: { t: 'Total span' }, fr: { t: 'Écart total', reviewed: true } },
    'label.tuningLibrary':   { en: { t: 'Tuning Library' }, fr: { t: 'Bibliothèque', reviewed: true } },
    // The library filter is a PLAIN select over the strings all / Historical /
    // ... — it is not an AudioParameterChoice, no host ever shows these six
    // strings, and translating them cannot make the page and an automation lane
    // disagree. That is the discriminator; the choice-parameter option strings
    // are exempt below for exactly the opposite reason. The option VALUES are
    // untouched, so filtering still matches the categories the C++ side reports.
    'label.catAllCategories':  { en: { t: 'All Categories' },  fr: { t: 'Toutes catégories', reviewed: true } },
    'label.catHistorical':     { en: { t: 'Historical' },      fr: { t: 'Historiques',   reviewed: true } },
    'label.catJustIntonation': { en: { t: 'Just Intonation' }, fr: { t: 'Intonation juste', reviewed: true } },
    'label.catEqualDivisions': { en: { t: 'Equal Divisions' }, fr: { t: 'Divisions égales', reviewed: true } },
    'label.catNonOctave':      { en: { t: 'Non-Octave' },      fr: { t: 'Non octaviantes', reviewed: true } },
    'label.catWorld':          { en: { t: 'World' },           fr: { t: 'Du monde',      reviewed: true } },
    // "A4" is a pitch identifier and stays; only the abbreviation "REF" moves.
    'label.a4Ref':   { en: { t: 'A4 REF' },  fr: { t: 'RÉF. A4',   reviewed: true } },
    'label.stretch': { en: { t: 'Stretch' }, fr: { t: 'Étirement', reviewed: true } },
    // The four file buttons keep their EXTENSIONS, which are file-format
    // identifiers, and translate only the verb.
    'label.loadScl':    { en: { t: 'Load .SCL' },   fr: { t: 'Ouvrir .SCL', reviewed: true } },
    'label.loadKbm':    { en: { t: 'Load .KBM' },   fr: { t: 'Ouvrir .KBM', reviewed: true } },
    'label.saveScl':    { en: { t: 'Save .SCL' },   fr: { t: 'Enreg. .SCL', reviewed: true } },
    'label.saveKbm':    { en: { t: 'Save .KBM' },   fr: { t: 'Enreg. .KBM', reviewed: true } },
    'label.exportHtml': { en: { t: 'Export HTML' }, fr: { t: 'Exporter HTML', reviewed: true } },
    'label.generateScale': { en: { t: 'Generate Scale' }, fr: { t: 'Générer une gamme', reviewed: true } },
    'label.genEdo':      { en: { t: 'EDO (Equal Division)' }, fr: { t: 'EDO (division égale)', reviewed: true } },
    'label.genHarmonic': { en: { t: 'Harmonic Series' },      fr: { t: 'Série harmonique',     reviewed: true } },
    'label.genRank2':    { en: { t: 'Rank-2 Temperament' },   fr: { t: 'Tempérament de rang 2', reviewed: true } },
    'label.genDivisions': { en: { t: 'Divisions' },     fr: { t: 'Divisions', reviewed: true, sameAsEn: true } },
    'label.genPeriod':   { en: { t: 'Period (c)' },     fr: { t: 'Période (c)', reviewed: true } },
    'label.genStart':    { en: { t: 'Start Harmonic' }, fr: { t: 'Harmonique de départ', reviewed: true } },
    'label.genEnd':      { en: { t: 'End Harmonic' },   fr: { t: 'Harmonique de fin',    reviewed: true } },
    'label.genGenerator': { en: { t: 'Generator (c)' }, fr: { t: 'Générateur (c)', reviewed: true } },
    'label.genCount':    { en: { t: 'Notes' },          fr: { t: 'Notes', reviewed: true, sameAsEn: true } },
    'label.generate':    { en: { t: 'Generate' },       fr: { t: 'Générer', reviewed: true } },

    // ── Accessible names ────────────────────────────────────────────────────
    // alt.snail was a native alt=; aria.doubleClickEdit was the ONE native
    // title= this plugin had, written from JS onto all sixteen effects-knob
    // readouts (`valueDisplay.title = 'Double-click to edit'`). Contract §4
    // DELETES a native title= rather than localizing it, and where the title was
    // an element's only help its text becomes the accessible name. NO NEW PROSE
    // WAS INVENTED: both strings are the plugin's own v4.1.5 wording.
    'alt.snail':            { en: { t: 'Snail' }, fr: { t: 'Escargot', reviewed: true } },
    'aria.settings':        { en: { t: 'Settings' }, fr: { t: 'Réglages', reviewed: true } },
    'aria.langSelect':      { en: { t: 'Interface language' }, fr: { t: 'Langue de l’interface', reviewed: true } },
    'aria.doubleClickEdit': { en: { t: 'Double-click to edit' }, fr: { t: 'Double-cliquer pour modifier', reviewed: true } },
});

// ============================================================================
// I18N_EXEMPT — reasoned exclusions, never silence
// ============================================================================
//
// Every visible string the coverage scan finds must be a [data-i18n] element, a
// setLabel() call, or an entry HERE WITH A REASON. A bare skip list would let a
// missed label hide as a deliberate one.
//
// An entry is [text, reason] or [text, reason, SCOPE]. An exemption is matched
// by TEXT, so an unscoped one silences EVERY node with that string. A scope is
// REQUIRED where the same string is also keyed on this page — the one state in
// which the gate cannot tell a deliberate skip from a forgotten label
// (assertion 14). None of the strings below is keyed on this page; "Default"
// carries a scope anyway because it is the one string a future caption could
// plausibly collide with.
//
// ── NOT LISTED HERE, AND WHY ───────────────────────────────────────────────
//
// Runtime DATA has no fixed literal to exempt, so listing it would be an inert
// entry that reads like a rule. Named instead:
//   - the preset-dropdown CATEGORY headers and preset ITEM names
//     (`header.textContent = category`, `item.textContent = preset`) come from
//     getPresetListWithCategories() on the C++ side. The name IS the JSON
//     filename and the category IS its folder, so both are D-02.
//   - the tuning library item names and the #scale-name-display value come from
//     getEmbeddedTuningList() / getTuningName(). Same rule.
//   - the scale names the generator BUILDS (`19-EDO`, `Harmonics 8-16`,
//     `Rank-2 (696.6c, 12 notes)`) are passed to applyGeneratedScale() and are
//     written into .scl files and the exported HTML. Localizing them would put
//     French inside a Scala file.
//
// LETTER PITCH NOTATION IS DELIBERATELY NOT LOCALIZED, across the whole page:
// the footer keyboard's C/D/E/F/G/A/B and C3/C4, the tuning panel's twelve
// noteNames, and the interval-quality abbreviations m2/M2/…/P8 that sit beside
// them in True Keys. French solfège would be Do/Ré/Mi and 2m/2M/…/8J, and the
// C++ TuningEngine, the .scl and .kbm file formats and the exported HTML all
// speak the letter system. Translating the page alone would desync it from the
// files it reads and writes; translating both is a data-format change, not a
// caption change. Only "TT" survives the readout classifier as prose, so it is
// the only one of the twelve that needs an entry.
// ============================================================================

export const I18N_EXEMPT = [
    ['Ouaricon Bells',
     'the product name in the h1 — a product name is never translated, and this is the display form of the plugin’s registered PRODUCT_NAME in CMakeLists.txt'],
    ['Ouaricon Audio',
     'the company name in the footer — a company name is never translated'],

    // #preset-name-display shows the loaded preset. The name IS the JSON
    // filename (OuariconPresetManager.h), so translating it breaks recall.
    // "Default" is both the authored markup fallback and the `name || 'Default'`
    // fallback in updatePresetDisplay().
    ['Default', 'the preset-name display’s fallback — exempt under D-02, because a preset name IS the JSON filename',
                '#preset-name-display'],

    // ── AudioParameterChoice option strings (D-01 arm 1) ────────────────────
    // Byte-identical to the parameter's own choice list, which the host shows in
    // its automation lane and which some hosts cache. Translating the option
    // text would make the page and the automation lane disagree about the same
    // parameter. The library filter and the generator type ARE localized above,
    // and the difference is exactly this: those two are plain selects no host
    // ever sees.
    ['Click',  'strikeNoiseChar choice-parameter value, byte-identical — host automation contract (D-01 arm 1)'],
    ['Thud',   'strikeNoiseChar choice-parameter value, byte-identical — host automation contract (D-01 arm 1)'],
    ['Ping',   'strikeNoiseChar choice-parameter value, byte-identical — host automation contract (D-01 arm 1)'],
    ['Linear', 'velocityCurve choice-parameter value, byte-identical — host automation contract (D-01 arm 1). Its two siblings Exp/Log are NOT byte-identical to "Exponential"/"Logarithmic" and are keyed above'],
    ['Normal',   'delayMode choice-parameter value, byte-identical — host automation contract (D-01 arm 1)'],
    ['PingPong', 'delayMode choice-parameter value, byte-identical — host automation contract (D-01 arm 1)'],

    // ── material: exempt on TWO arms at once ────────────────────────────────
    // Arm 1: byte-identical to the material AudioParameterChoice options.
    // Arm 3: the receiving element is `.param-value[data-value="material"]`, a
    // READOUT node that holds a percentage for every other slider on the page.
    // Keying one would make the element enter and leave the sweep as the knob
    // turns — the O-Marimba case, here with arm 1 agreeing.
    ['Bronze',    'material choice-parameter value written into a .param-value READOUT node — D-01 arms 1 and 3'],
    ['Brass',     'material choice-parameter value written into a .param-value READOUT node — D-01 arms 1 and 3'],
    ['Steel',     'material choice-parameter value written into a .param-value READOUT node — D-01 arms 1 and 3'],
    ['Aluminum',  'material choice-parameter value written into a .param-value READOUT node — D-01 arms 1 and 3'],
    ['Cast Iron', 'material choice-parameter value written into a .param-value READOUT node — D-01 arms 1 and 3'],

    // ── Identifiers, not words ──────────────────────────────────────────────
    ['12-TET Standard',
     'a tuning IDENTIFIER, not a caption — it is the name the tuning engine reports for the loaded scale and is matched against Scala file names'],
    ['TT',
     'the tritone’s interval-quality abbreviation in the True Keys view, beside letter pitch names the C++ engine and the .scl/.kbm formats also use — see the note above I18N_EXEMPT. The other eleven (m2, M2, m3, M3, P4, P5, m6, M6, m7, M7, P8) classify as READOUT and need no entry'],
    ['English',  'endonym — a language name is never translated'],
    ['Français', 'endonym — a language name is never translated'],
];

// ── TIP_BINDINGS ────────────────────────────────────────────────────────────
//
// [selector, key, wrapper]. applyI18n() runs document.querySelector(selector),
// then el.closest(wrapper) — falling back to `el` itself when the walk finds
// nothing. That FALLBACK is why tests/ui_tip_render_check.js asserts the wrapper
// walk SEPARATELY and treats a miss as a hard FAIL: a broken wrapper still opens
// a tip, on the wrong-sized cell, and every "the tip appeared with the right
// text inside the viewport" assertion stays green (M2 finding 3, from O-Reed).
//
// "BIND TO THE IDS THE UI ALREADY USES" IS FALSE ON THIS PAGE, ON BOTH HALVES,
// and the two halves fail for different reasons — which is the score on every
// plugin in this stage but one.
//
//   SELECTOR half. 37 of the 65 anchors carry NO id at all: the instrument
//   tab's sliders and its two choice groups are addressed as
//   `[data-param="<id>"]`, the same attribute the page's own binding loop uses
//   (index.html:2097). The other 28 do have ids — the 16 FX knobs are
//   `#<paramId>Knob` from makeFxKnob(), the four bypass buttons are
//   `#<fx>BypassBtn`, and the toggles and chrome are hand-authored ids.
//
//   TARGET half. 55 of the 65 need the closest() walk. `.slider` is a 6px-tall
//   track and `#<id>Knob` is a 44px SVG box; what the user aims at is the
//   `.param-control` column (caption + control + readout) or the
//   `.knob-container` (knob + caption + readout). A tip bound to the bare track
//   is a tip nobody can hold open.
//
// THE CHROME BINDS BARE, and it has to. `.settings-cluster` contains BOTH
// #gear-btn and #settings-popover, so a wrapper walk from #lang-select would
// reach the cluster and hovering the selector would open the GEAR's tip
// (O-Comp's carried trap). #gear-btn is its own anchor; #lang-select walks only
// as far as the `.settings-row` <label>, which does not contain the gear.
//
// TWO ANCHORS ARE IN THE LAZILY-IMPORTED TUNING PANEL, and the cost is stated
// rather than hidden. `#ref-pitch-knob` and `#octave-stretch` are built by
// js/tuning-panel.js, which index.html imports at :3051 inside an async IIFE —
// so they are ABSENT from the DOM when initI18n() sweeps, and applyI18n()
// console.warns "tip target not found" for each on that first pass.
//
//   They still bind, and this is where O-Bells diverges from O-Reed, whose
//   referencePitch was reported page-unreachable in batch M2 for exactly this
//   shape. This page ALREADY carries window.__reapplyI18n() (index.html:1992)
//   and the panel's own init calls it after mounting (index.html:3066), so the
//   second sweep resolves both selectors and writes both attribute pairs. The
//   warning is the whole cost, and it is a console.warn: boot-all-uis filters
//   on m.type() === 'error' (scripts/boot-all-uis.js:141) and never sees it.
//   tests/ui_tip_render_check.js asserts the warning set is EXACTLY these two
//   selectors rather than relaxing the assertion, so a third one appearing is
//   still a failure.
//
// TWO PARAMETERS GET NO BINDING, AND NO BODY. `tuning_pitchBendRange` and
// `tuning_temperamentPreset` are host-reachable and page-unreachable:
//   - zero occurrences of either parameter ID anywhere in the served root;
//   - `tuning_pitchBendRange` has a WebSliderRelay (PluginEditor.cpp:107) that
//     nothing on the page ever asks for, and no native function at all;
//   - `tuning_temperamentPreset` has BOTH halves of a native-function bridge
//     (PluginEditor.cpp:517 setTemperamentPreset, :533 getTemperamentPreset)
//     and neither name appears in index.html or js/tuning-panel.js. A dead
//     bridge, not a missing file.
// Both are automatable and both reach the TuningEngine through
// parameterChanged() (PluginProcessor.cpp:1634, :1638). No control was added to
// satisfy the count — that is a feature change with a geometry cost.
//
// The four preset-bar controls get no tips either. They took accessible names
// from their deleted title= attributes in Stage K and are self-describing;
// M1 decision item 2 put them out of scope for this stage.
export const TIP_BINDINGS = [
    // ── Instrument tab: Synthesis ───────────────────────────────────────────
    ['.slider[data-param="damping"]',            'tip.damping',            '.param-control'],
    ['.slider[data-param="overtoneBrightness"]', 'tip.overtoneBrightness', '.param-control'],
    ['.slider[data-param="acousticBrightness"]', 'tip.acousticBrightness', '.param-control'],
    ['.slider[data-param="material"]',           'tip.material',           '.param-control'],
    ['.slider[data-param="inharmonicity"]',      'tip.inharmonicity',      '.param-control'],
    ['.slider[data-param="airAbsorption"]',      'tip.airAbsorption',      '.param-control'],
    ['.slider[data-param="airAbsorptionTime"]',  'tip.airAbsorptionTime',  '.param-control'],
    ['.slider[data-param="bloomSpeed"]',         'tip.bloomSpeed',         '.param-control'],
    ['.slider[data-param="bloomAmount"]',        'tip.bloomAmount',        '.param-control'],
    ['.slider[data-param="shimmer"]',            'tip.shimmer',            '.param-control'],
    // Bound BARE: the toggle is a chip inside a .param-control that holds
    // nothing else, so the walk would widen the anchor over empty space.
    ['#bloom-fine-toggle',                       'tip.bloomFineEnabled',   null],
    ['.slider[data-param="bloomSpeedLow"]',      'tip.bloomSpeedLow',      '.param-control'],
    ['.slider[data-param="bloomSpeedMid"]',      'tip.bloomSpeedMid',      '.param-control'],
    ['.slider[data-param="bloomSpeedHigh"]',     'tip.bloomSpeedHigh',     '.param-control'],
    ['.slider[data-param="bloomAmountLow"]',     'tip.bloomAmountLow',     '.param-control'],
    ['.slider[data-param="bloomAmountMid"]',     'tip.bloomAmountMid',     '.param-control'],
    ['.slider[data-param="bloomAmountHigh"]',    'tip.bloomAmountHigh',    '.param-control'],

    // ── Instrument tab: Ensemble ────────────────────────────────────────────
    ['.slider[data-param="unisonCount"]',        'tip.unisonCount',        '.param-control'],
    ['.slider[data-param="unisonDetune"]',       'tip.unisonDetune',       '.param-control'],
    ['.slider[data-param="octaveBlendSub"]',     'tip.octaveBlendSub',     '.param-control'],
    ['.slider[data-param="octaveBlendOct"]',     'tip.octaveBlendOct',     '.param-control'],
    ['.slider[data-param="stereoSpread"]',       'tip.stereoSpread',       '.param-control'],

    // ── Instrument tab: Onsets ──────────────────────────────────────────────
    ['.slider[data-param="strikePosition"]',     'tip.strikePosition',     '.param-control'],
    ['.slider[data-param="malletHardness"]',     'tip.malletHardness',     '.param-control'],
    ['.slider[data-param="attackLevel"]',        'tip.attackLevel',        '.param-control'],
    // The two choice groups are .choice-group, not .slider — same wrapper.
    ['.choice-group[data-param="strikeNoiseChar"]', 'tip.strikeNoiseChar', '.param-control'],
    ['.choice-group[data-param="velocityCurve"]',   'tip.velocityCurve',   '.param-control'],

    // ── Instrument tab: Advanced ────────────────────────────────────────────
    ['.slider[data-param="partialTuning"]',      'tip.partialTuning',      '.param-control'],
    ['.slider[data-param="pitchEnvelope"]',      'tip.pitchEnvelope',      '.param-control'],
    ['.slider[data-param="pitchEnvTime"]',       'tip.pitchEnvTime',       '.param-control'],
    ['.slider[data-param="nonlinearEffects"]',   'tip.nonlinearEffects',   '.param-control'],

    // ── Instrument tab: Multi-Stage Envelope ────────────────────────────────
    ['.slider[data-param="strikeTime"]',         'tip.strikeTime',         '.param-control'],
    ['.slider[data-param="brilliance"]',         'tip.brilliance',         '.param-control'],
    ['.slider[data-param="bodyTime"]',           'tip.bodyTime',           '.param-control'],
    ['.slider[data-param="humSustain"]',         'tip.humSustain',         '.param-control'],

    // ── Instrument tab: Filter / Performance / Output ───────────────────────
    ['#lp-filter-toggle',                        'tip.lpFilterEnabled',    null],
    ['.slider[data-param="lpFilterCutoff"]',     'tip.lpFilterCutoff',     '.param-control'],
    ['#hi-fi-toggle',                            'tip.highFidelity',       null],
    ['.slider[data-param="humanize"]',           'tip.humanize',           '.param-control'],
    // The footer gain is NOT in a .param-control — it is the .footer-gain
    // flex row beside the keyboard. A .param-control walk would find the
    // nearest one two sections up the tree.
    ['.slider[data-param="outputGain"]',         'tip.outputGain',         '.footer-gain'],

    // ── Tuning tab (lazily mounted; see the note above) ─────────────────────
    ['#ref-pitch-knob',   'tip.tuning_masterTune',    '.tuning-ref-section'],
    ['#octave-stretch',   'tip.tuning_octaveStretch', '.octave-stretch-row'],

    // ── Effects tab ─────────────────────────────────────────────────────────
    // The bypass buttons bind BARE. .fx-header holds the section title AND the
    // button, so a walk there would put the bypass tip over the title too.
    ['#chorusBypassBtn',  'tip.chorusBypass',   null],
    ['#chorusRateKnob',   'tip.chorusRate',     '.knob-container'],
    ['#chorusDepthKnob',  'tip.chorusDepth',    '.knob-container'],
    ['#chorusMixKnob',    'tip.chorusMix',      '.knob-container'],

    ['#delayBypassBtn',   'tip.delayBypass',    null],
    ['#delayTimeKnob',    'tip.delayTime',      '.knob-container'],
    ['#delayFeedbackKnob','tip.delayFeedback',  '.knob-container'],
    // The delay mode dropdown is the one FX control that is not a knob; its
    // caption and <select> sit in a .fx-dropdown-container built at
    // index.html:2942.
    ['#delayModeSelect',  'tip.delayMode',      '.fx-dropdown-container'],
    ['#delayMixKnob',     'tip.delayMix',       '.knob-container'],

    ['#eqBypassBtn',      'tip.eqBypass',       null],
    ['#eqLowGainKnob',    'tip.eqLowGain',      '.knob-container'],
    ['#eqMidGainKnob',    'tip.eqMidGain',      '.knob-container'],
    ['#eqMidFreqKnob',    'tip.eqMidFreq',      '.knob-container'],
    ['#eqHighGainKnob',   'tip.eqHighGain',     '.knob-container'],

    ['#reverbBypassBtn',  'tip.reverbBypass',   null],
    ['#reverbSizeKnob',   'tip.reverbSize',     '.knob-container'],
    ['#reverbDampKnob',   'tip.reverbDamp',     '.knob-container'],
    ['#reverbPredelayKnob', 'tip.reverbPredelay', '.knob-container'],
    ['#reverbModKnob',    'tip.reverbMod',      '.knob-container'],
    ['#reverbShimmerKnob','tip.reverbShimmer',  '.knob-container'],
    ['#reverbMixKnob',    'tip.reverbMix',      '.knob-container'],

    // ── Chrome ──────────────────────────────────────────────────────────────
    ['#gear-btn',    'tip.gearBtn',    null],
    ['#lang-select', 'tip.langSelect', '.settings-row'],
];

export function tr(key, lang, vars) {
    const entry = I18N[key];
    if (!entry) { console.warn(`i18n: missing key ${key}`); return { t: key, b: '' }; }
    const s = entry[lang] || entry.en;

    // A var VALUE that is itself an I18N key resolves to that key's localized
    // title; anything else is used literally. This plugin needs neither arm
    // today, but the canon is one shape across all 43 plugins and this function
    // is not trimmed per plugin.
    const resolve = (v) => {
        const nested = I18N[v];
        return nested ? String((nested[lang] || nested.en).t) : String(v);
    };

    const sub = (v) => vars
        ? String(v).replace(/\{(\w+)\}/g, (m, n) => (n in vars ? resolve(vars[n]) : m))
        : String(v);

    return { t: sub(s.t), b: sub(s.b) };
}
