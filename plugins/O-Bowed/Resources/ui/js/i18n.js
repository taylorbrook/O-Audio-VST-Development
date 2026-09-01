/*
   This file is part of O-Bowed, an Ouaricon Audio plugin.
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
// i18n.js — O-Bowed on-page copy and hover-help, English + French
//           (v1.6.1, canon v2)
//
// ── v1.6.1: FRENCH QA PASS (Stage N, 2026-08-31) ─────────────────────────
//
// Every fr entry read against its en and against scripts/i18n-fr-glossary.js.
// Changed: 24 entries of 71 (7 terminology, 8 typography, 8 grammar/agreement,
// 1 meaning). sameAsEn: kept 3, translated 0. termNote exemptions: 3 (listed by
// `node scripts/i18n-fr-lint.js --plugin O-Bowed --strict`, which exits 0).
// Left as drafted: the other 47. reviewed: false throughout — the flag means a
// NATIVE SPEAKER read it, and none has; this pass was a second machine reading.
//
// Decisions the next reader needs:
//
//   * "Matière" → "Matériau" (the glossary root term). It measures 45.52 px in
//     .knob-label — whose cap is 64 px, NOT the 62 px the v1.6.0 block below
//     still states: index.html:463 raised max-width to 64px at v1.5.0 and this
//     header was never updated with it. Consequence: "Raideur crin" (63.0 px,
//     rejected below on the 62 px number) would now FIT. It is kept abbreviated
//     anyway, because the ENGLISH caption is abbreviated too ("Hair Stiff.")
//     and matching it is this page's own stated rule.
//   * "Fréq." KEPT for Rate, with a termNote. HumanizeEngine.h:83 maps the knob
//     to a 0.15–8 Hz smoothing corner, so it really is a frequency, and it sits
//     directly under a column captioned Vitesse. "Vitesse" FITS (34.94 px in
//     the 64 px cap) — the exemption is meaning, not width.
//   * "Tenue" KEPT for Infinite Sustain, with a termNote. The glossary forbids
//     it as a rendering of the ADSR sustain SEGMENT; this page carries no
//     envelope at all, and the control removes the string's damping so the note
//     keeps ringing — which is "une tenue". "Maintien inf." also fits
//     (63.52 px). Meaning again, not width; both alternatives were measured
//     rather than argued.
//   * "archeté" is not a French word. "la corde frottée" (the organology term
//     for a bowed string) and "une note jouée à l'archet" replace it in the
//     three bodies that had it.
//   * "Le dosage" is KEPT in tip.bodyAmt. Dosage is forbidden as a LABEL
//     rendering of Mix; this is a BODY describing a dry/wet blend, where it is
//     the natural French, and the lint does not match bodies against TERMS.
//   * Bodies address the user as vous and use the imperative ("Baissez-la",
//     "Déplacez-la", "Appuyez sur Échap"). Unchanged — recorded so the next
//     pass keeps one register rather than mixing in the infinitive.
//   * U+00A0 landed 24 times: before ‘:’ and ‘;’, and between every number and
//     its unit — including m/s and N, which the lint's UNITS list does not
//     carry, so those two are typography the lint never asked for. Nothing
//     outside a French string VALUE was touched: both revisions were loaded and
//     compared field by field — en values changed 0, keys changed 0, NBSP
//     outside a string literal 0.
//
// An ES module that EXPORTS ONLY. It must never self-execute: a bare top-level
// statement here throws out of module evaluation and takes every later
// initializer on the page with it (pattern_module_toplevel_init_tdz). On this
// plugin "every later initializer" is the ENTIRE UI, because the controller is
// one inline <script type="module"> in index.html rather than a file that can
// fail in isolation. check-i18n assertion 7 enforces the rule.
//
// FILENAME: no hyphen. juce_add_binary_data STRIPS hyphens rather than
// converting them to underscores, so a file named i18n-fr.js would have to be
// reached as the symbol i18nfr_js (critical_binary_data_strips_hyphens). One
// combined file for both languages sidesteps the question.
//
// HOVER-HELP ARRIVED AT v1.6.0, AND THE TABLE IS ONLY HALF OF IT. v1.5.0 had
// an empty I18N and an empty TIP_BINDINGS, correctly: v1.4.1 carried no
// data-tip anywhere, only three native title= attributes that contract §4
// DELETES rather than localizes. v1.6.0 authors 30 entries — 28 parameters
// plus the gear and the language selector — and binds every one.
//
// CANON v2 WRITES ATTRIBUTES AND STOPS THERE. applyI18n() puts data-tip-title
// and data-tip onto each anchor named in TIP_BINDINGS; the code that reads
// them and paints a surface is setupTooltips() in index.html, added in the
// same commit. Measured across all 22 bare plugins before this stage began:
// id="tooltip" 0, `.tooltip {` 0, closest("[data-tip]") 0. Landing this file
// alone would ship 30 invisible strings past three green gates — check-i18n
// reads the table statically, check-ui-labels has no tooltip awareness at all,
// and boot-all-uis counts aria-label and title but never data-tip. The
// assertion those three cannot make lives in tests/ui_tip_render_check.js.
//
// ALL FRENCH IS MACHINE-DRAFTED AND FLAGGED `reviewed: false`. No native
// speaker has read it. `node scripts/check-i18n.js` prints the worklist.
//
// ── EVERY FRENCH STRING HERE WAS MEASURED IN ITS OWN ELEMENT ────────────────
//
// Never in another plugin's: K2 proved two plugins render the same two words at
// the same declared font-size 8.24 px apart, so a borrowed absolute reads
// exactly like a right number and is not one. The three boxes that constrain
// this page, measured at v1.4.1:
//
//   .knob-label          62 px hard cap, `nowrap` + `overflow: hidden` +
//                        `text-overflow: ellipsis`. Past 62 px a caption is
//                        CLIPPED, silently — it never pushes anything, so the
//                        geometry diff cannot see it and only a per-string
//                        measurement can. Two candidates were rejected on this
//                        alone: "Raideur crin" (63.0) and "Tenue infinie"
//                        (65.0). NOTE: "Rev. Friction" ALREADY clips by 1 px in
//                        ENGLISH at v1.4.1 (scrollWidth 63 vs clientWidth 62).
//   .humanize-col-label  no cap at all, and its width DRIVES its grid column,
//                        which is a non-label element the geometry diff
//                        measures. "Colophane" (52.70) would widen the column
//                        from 38 to 52.70 and grow left ON TOP of its
//                        neighbour. "Coloph." (37.53) sits inside the 39.75 px
//                        track and moves nothing — hence two keys for one
//                        English word, decided by geometry.
//   .footer-bar          `min-width: 50px` on the label, so every French
//                        caption at or under 50 px leaves the row untouched.
//                        "Diapason réf." (72.55) would have clipped.
// ============================================================================

export const LANGUAGES = ['en', 'fr'];

// ============================================================================
// I18N — hover-help, added at v1.6.0 (Stage M batch M2)
//
// An entry is {en:{t,b}, fr:{t,b,reviewed}}. `t` is the tooltip TITLE, `b` the
// body. Canon v2's applyI18n() writes both onto the anchor named in
// TIP_BINDINGS as data-tip-title / data-tip and stops there; the code that
// READS those attributes and paints a surface is setupTooltips() in
// index.html, added in the same commit. Attributes without a renderer are 30
// invisible strings past three green gates — which is what v1.5.0 would have
// shipped had this file changed alone.
//
// ── TITLES ──────────────────────────────────────────────────────────────────
//
// The title is the parameter's display name from .planning/params.tsv, EXCEPT
// where the page's caption carries something the dump's name does not. Two
// rules, applied per entry and stated because they disagree with each other:
//
//   * where the caption is a genuinely different word, the CAPTION wins — the
//     user is reading the page, not the automation lane.
//   * where the caption is the dump name minus its section prefix ("Speed"
//     inside the Bow box) or an abbreviation of it ("Rev. Friction",
//     "Ref Pitch", "Body Amt"), the tip carries the FULL DUMP NAME. A tooltip
//     floats free of the section box that disambiguates the caption, and this
//     page has a second "Speed", a second "Position" and a second "Rosin" in
//     the Humanize grid. Restoring the prefix is not a third name; it is the
//     page's own composition, and it is what the host automation lane shows.
//
// The one place they genuinely diverge is sympatheticCount: the dump says
// "Sympathetic Strings", the caption says "Count". The dump name is used, and
// the body names the caption's word, because "Count" alone in a floating tip
// says nothing about what is being counted.
//
// ── RANGES AND UNITS ────────────────────────────────────────────────────────
//
// Only 5 of the 28 dumped parameters that have a control carry a `label` in
// params.tsv: bowSpeed (m/s), bowPressure (N), brightness (Hz), outputLevel
// (dB), referencePitch (Hz). The other 23 have an EMPTY label column, and
// every one of them is genuinely dimensionless — recovered from the page's own
// formatter, not invented: index.html:1170 formatValue() is
// `rawValue.toFixed(def.decimals) + def.unit`, and every one of those 23 rows
// in the PARAMS table at index.html:1077-1114 declares `unit: ''`. So the
// range in each body is a bare number at the decimals the readout uses, which
// is what a user comparing tip to knob actually sees.
//
// sympatheticCount is the exception inside that group: it is an
// AudioParameterInt rendered at 0 decimals, so its range is stated as a count
// of strings rather than a bare number.
//
// The eight Humanize Rate knobs read 0.00-1.00 on the page and there is no
// unit to recover, but the number is meaningless on its own — HumanizeEngine.h:83
// maps it to a 0.15-8 Hz smoothing corner. Each body states the mapping AND
// the on-page range, because the tip has to agree with the readout beside it.
//
// ── FRENCH ──────────────────────────────────────────────────────────────────
//
// A tooltip body is PROSE and takes French convention: decimal COMMA, U+2212
// for the minus. SETTLED for the whole task on 2026-08-30 by the developer,
// after M1 split on it and one plugin shipped a point without flagging it.
// The value READOUT keeps its point — D-03 exempts the readout NODE, and
// .knob-value is machine-formatted rather than prose. They differ on purpose.
//
// D-01 arm 1 does not arise on this page. O-Bowed has exactly one
// AudioParameterChoice, `tuningSystem` ("Scala/TUN" / "MTS-ESP" / "12-TET"),
// and it has NO control in the WebView at all — see the FINDING at the foot of
// TIP_BINDINGS. No option string is named in any body below, in either
// language, so the "option stays English, the sentence naming it is French"
// split never had to be made here.
//
// ALL FRENCH BELOW IS MACHINE-DRAFTED, every entry `reviewed: false`.
// ============================================================================

export const I18N = Object.freeze({

    // ── Bow ─────────────────────────────────────────────────────────────────
    // Caption "Speed" inside the Bow box; dump name "Bow Speed". Prefix
    // restored — the Humanize grid has its own "Speed" column.
    'tip.bowSpeed': {
        en: { t: 'Bow Speed',
              b: 'How fast the bow is drawn across the string. Faster bowing drives the string '
               + 'harder and brightens the tone; too fast for the pressure currently set and the '
               + 'string slips into a whistle instead of speaking. 0.02 to 2.00 m/s.' },
        fr: { t: 'Vitesse d’archet',
              b: 'La vitesse à laquelle l’archet est tiré sur la corde. Un archet rapide excite '
               + 'davantage la corde et éclaircit le timbre ; si l’archet va trop vite pour la '
               + 'pression réglée, la corde siffle au lieu de parler. 0,02 à 2,00 m/s.',
              reviewed: false },
    },
    'tip.bowPressure': {
        en: { t: 'Bow Pressure',
              b: 'The normal force the bow presses onto the string. Low pressure gives a thin, '
               + 'airy surface tone; high pressure grips harder and thickens the sound until it '
               + 'crunches. 0.01 to 5.00 N.' },
        fr: { t: 'Pression d’archet',
              b: 'La force normale que l’archet exerce sur la corde. Une pression faible donne '
               + 'un son de surface fin et aéré ; une pression forte accroche davantage et '
               + 'épaissit le son jusqu’au grincement. 0,01 à 5,00 N.',
              reviewed: false },
    },
    'tip.bowPosition': {
        en: { t: 'Bow Position',
              b: 'Where the bow contacts the string, as a fraction of its length from the '
               + 'bridge. Small values are sul ponticello — glassy and rich in upper partials; '
               + 'large values move toward sul tasto and soften the tone. 0.02 to 0.30.' },
        fr: { t: 'Position d’archet',
              b: 'Le point de contact de l’archet sur la corde, en fraction de sa longueur '
               + 'depuis le chevalet. Les petites valeurs donnent un jeu sul ponticello, vitreux '
               + 'et riche en partiels aigus ; les grandes tendent vers le sul tasto et '
               + 'adoucissent le timbre. 0,02 à 0,30.',
              reviewed: false },
    },
    'tip.rosin': {
        en: { t: 'Rosin',
              b: 'Shapes the friction curve between hair and string, from smooth to aggressive. '
               + 'More rosin makes the stick-slip cycle snap harder, which sharpens the attack '
               + 'and adds bite. 0.00 to 1.00.' },
        fr: { t: 'Colophane',
              b: 'Façonne la courbe de friction entre le crin et la corde, du lisse à '
             + 'l’agressif. Plus de colophane fait claquer le cycle adhérence-glissement, ce qui '
             + 'aiguise l’attaque et ajoute du mordant. 0,00 à 1,00.',
              reviewed: false },
    },
    'tip.bowNoise': {
        en: { t: 'Bow Noise',
              b: 'Adds the broadband scrape of hair on string on top of the pitched tone. A '
               + 'little restores the breath a pure waveguide leaves out; a lot pushes toward a '
               + 'pressed, unvoiced sound with no clear note in it. 0.00 to 1.00.' },
        fr: { t: 'Bruit d’archet',
              b: 'Ajoute le frottement large bande du crin sur la corde par-dessus le son de '
             + 'hauteur définie. En petite quantité, il redonne le souffle qu’un guide d’ondes '
             + 'pur laisse de côté ; en grande quantité, il pousse vers un son écrasé, sans note '
             + 'claire. 0,00 à 1,00.',
              reviewed: false },
    },
    // Caption "Hair Stiff." is an abbreviation forced by the 62 px .knob-label
    // cap recorded in this file's LABELS header. The tip has 260 px, so it
    // carries the dump name unabbreviated.
    'tip.hairStiff': {
        en: { t: 'Bow Hair Stiffness',
              b: 'Blends between the simple friction core and a full elasto-plastic bristle '
               + 'model. At 0 the tone matches the classic O-Bowed voice, which is why every '
               + 'factory preset leaves it there; raising it lets the hair bend and release, '
               + 'loosening the attack. 0.00 to 1.00.' },
        fr: { t: 'Raideur du crin',
              b: 'Passe progressivement du noyau de friction simple à un modèle de crin '
             + 'élasto-plastique complet. À 0 le timbre correspond à la voix classique '
             + 'd’O-Bowed, ce qui explique que tous les préréglages d’usine l’y laissent ; à '
             + 'mesure qu’on le monte, le crin fléchit puis lâche et l’attaque s’assouplit. 0,00 '
             + 'à 1,00.',
              reviewed: false },
    },

    // ── Humanize ────────────────────────────────────────────────────────────
    // Eight knobs captioned "Amt" and "Rate" under four column headings. The
    // caption alone identifies nothing, so every title here is the dump name.
    // The Rate bodies name the column caption the knob depends on ("Speed Amt"
    // / "Qté Vitesse") because that is the control the user has to find, and
    // "Coloph." rather than "Colophane" in French because label.rosinShort is
    // what that column actually renders.
    'tip.humSpeedAmt': {
        en: { t: 'Speed Humanize',
              b: 'How far bow speed wanders on its own, independently per voice, so two '
               + 'repetitions of a note are never identical. At 0 the drift is off entirely and '
               + 'the Rate knob beside it does nothing. 0.00 to 1.00.' },
        fr: { t: 'Humanisation de la vitesse',
              b: 'L’ampleur de la dérive spontanée de la vitesse d’archet, indépendante pour '
               + 'chaque voix, afin que deux répétitions d’une note ne soient jamais '
               + 'identiques. À 0 la dérive est entièrement désactivée et le bouton Fréq. voisin '
               + 'reste sans effet. 0,00 à 1,00.',
              reviewed: false },
    },
    'tip.humSpeedRate': {
        en: { t: 'Speed Humanize Rate',
              b: 'How quickly the bow-speed drift moves, from a slow swell to a fast tremble. '
               + 'The knob maps internally to a 0.15 to 8 Hz smoothing corner, and it does '
               + 'nothing while Speed Amt is 0. 0.00 to 1.00.' },
        fr: { t: 'Fréquence d’humanisation de la vitesse',
              b: 'La rapidité de la dérive de vitesse d’archet, d’une lente ondulation à un '
               + 'tremblement rapide. Le bouton correspond en interne à une coupure de lissage '
               + 'de 0,15 à 8 Hz, et il reste sans effet tant que Qté Vitesse vaut 0. '
               + '0,00 à 1,00.',
              reviewed: false },
    },
    'tip.humPressAmt': {
        en: { t: 'Pressure Humanize',
              b: 'How far bow pressure wanders on its own, independently per voice, so a held '
               + 'chord breathes instead of sitting still. At 0 the drift is off entirely and '
               + 'the Rate knob beside it does nothing. 0.00 to 1.00.' },
        fr: { t: 'Humanisation de la pression',
              b: 'L’ampleur de la dérive spontanée de la pression d’archet, indépendante pour '
               + 'chaque voix, afin qu’un accord tenu respire au lieu de rester figé. À 0 la '
               + 'dérive est entièrement désactivée et le bouton Fréq. voisin reste sans effet. '
               + '0,00 à 1,00.',
              reviewed: false },
    },
    'tip.humPressRate': {
        en: { t: 'Pressure Humanize Rate',
              b: 'How quickly the bow-pressure drift moves. The knob maps internally to a 0.15 '
               + 'to 8 Hz smoothing corner, and it does nothing while Pressure Amt is 0. '
               + '0.00 to 1.00.' },
        fr: { t: 'Fréquence d’humanisation de la pression',
              b: 'La rapidité de la dérive de pression d’archet. Le bouton correspond en interne '
               + 'à une coupure de lissage de 0,15 à 8 Hz, et il reste sans effet tant que Qté '
               + 'Pression vaut 0. 0,00 à 1,00.',
              reviewed: false },
    },
    'tip.humPosAmt': {
        en: { t: 'Position Humanize',
              b: 'How far the bow contact point wanders on its own, independently per voice — '
               + 'the drift a player’s arm makes across a long note. At 0 it is off entirely and '
               + 'the Rate knob beside it does nothing. 0.00 to 1.00.' },
        fr: { t: 'Humanisation de la position',
              b: 'L’ampleur de la dérive spontanée du point de contact, indépendante pour chaque '
               + 'voix — celle que le bras d’un instrumentiste imprime sur une note longue. À 0 '
               + 'elle est entièrement désactivée et le bouton Fréq. voisin reste sans effet. '
               + '0,00 à 1,00.',
              reviewed: false },
    },
    'tip.humPosRate': {
        en: { t: 'Position Humanize Rate',
              b: 'How quickly the contact-point drift moves. The knob maps internally to a 0.15 '
               + 'to 8 Hz smoothing corner, and it does nothing while Position Amt is 0. '
               + '0.00 to 1.00.' },
        fr: { t: 'Fréquence d’humanisation de la position',
              b: 'La rapidité de la dérive du point de contact. Le bouton correspond en interne '
               + 'à une coupure de lissage de 0,15 à 8 Hz, et il reste sans effet tant que Qté '
               + 'Position vaut 0. 0,00 à 1,00.',
              reviewed: false },
    },
    'tip.humRosinAmt': {
        en: { t: 'Rosin Humanize',
              b: 'How far the friction-curve setting wanders on its own, independently per '
               + 'voice, so the grip of the attack varies from note to note. At 0 it is off '
               + 'entirely and the Rate knob beside it does nothing. 0.00 to 1.00.' },
        fr: { t: 'Humanisation de la colophane',
              b: 'L’ampleur de la dérive spontanée du réglage de colophane, indépendante pour '
               + 'chaque voix, afin que l’accroche de l’attaque varie d’une note à l’autre. À 0 '
               + 'elle est entièrement désactivée et le bouton Fréq. voisin reste sans effet. '
               + '0,00 à 1,00.',
              reviewed: false },
    },
    'tip.humRosinRate': {
        en: { t: 'Rosin Humanize Rate',
              b: 'How quickly the rosin drift moves. The knob maps internally to a 0.15 to 8 Hz '
               + 'smoothing corner, and it does nothing while Rosin Amt is 0. 0.00 to 1.00.' },
        fr: { t: 'Fréquence d’humanisation de la colophane',
              b: 'La rapidité de la dérive de colophane. Le bouton correspond en interne à une '
               + 'coupure de lissage de 0,15 à 8 Hz, et il reste sans effet tant que Qté Coloph. '
               + 'vaut 0. 0,00 à 1,00.',
              reviewed: false },
    },

    // ── Impossible physics ──────────────────────────────────────────────────
    // Three captions abbreviated against the 62 px cap; all three tips carry
    // the full dump name.
    'tip.infSustain': {
        en: { t: 'Infinite Sustain',
              b: 'Removes damping from the string so a bowed note keeps ringing after the bow '
               + 'has left it. At 1 the loop loses almost nothing per pass and the note holds '
               + 'indefinitely. 0.00 to 1.00.' },
        fr: { t: 'Tenue infinie',
              termNote: 'not the ADSR sustain segment — this page carries no envelope at all. '
             + 'The control removes the string’s damping so the note keeps ringing, and "une '
             + 'tenue" is the French musical term for a held note. Maintien is the glossary’s '
             + 'answer for the envelope stage, which does not exist here; it is not kept out on '
             + 'width (Maintien inf. measures 63.52 px inside the 64 px .knob-label cap, and '
             + 'Maint. inf. 50.58).',
              b: 'Retire l’amortissement de la corde : une note jouée à l’archet continue de '
             + 'sonner une fois l’archet parti. À 1 la boucle ne perd presque rien à chaque '
             + 'passage et la note se tient indéfiniment. 0,00 à 1,00.',
              reviewed: false },
    },
    'tip.revFriction': {
        en: { t: 'Reversed Friction',
              b: 'Inverts the friction curve, so the string grips harder the faster it slips '
               + 'instead of letting go. No real bow hair can do this; the result is a '
               + 'sputtering, unstable attack that settles into an odd steady tone. '
               + '0.00 to 1.00.' },
        fr: { t: 'Friction inversée',
              b: 'Inverse la courbe de friction : la corde accroche d’autant plus qu’elle glisse '
               + 'vite, au lieu de lâcher. Aucun crin réel n’en est capable ; il en résulte une '
               + 'attaque instable et crachotante qui se stabilise sur un son tenu étrange. '
               + '0,00 à 1,00.',
              reviewed: false },
    },
    'tip.subHarm': {
        en: { t: 'Sub-Harmonics',
              b: 'Feeds the string back through a nonlinearity that adds content an octave and '
               + 'more below the played note. Useful for weight under a thin high register; at '
               + 'high settings the pitch itself starts to fold down. 0.00 to 1.00.' },
        fr: { t: 'Sous-harmoniques',
              b: 'Réinjecte le signal de la corde dans une non-linéarité qui ajoute du contenu '
             + 'une octave sous la note jouée, et plus bas encore. Utile pour donner du poids '
             + 'sous un registre aigu maigre ; à réglage élevé, la hauteur elle-même commence à '
             + 'se replier. 0,00 à 1,00.',
              reviewed: false },
    },

    // ── Body ────────────────────────────────────────────────────────────────
    // "Material" and "Size" are the dump names AND the captions — no prefix to
    // restore, and each body opens by naming the body resonator so a floating
    // tip is not ambiguous about what is being sized.
    'tip.material': {
        en: { t: 'Material',
              b: 'Morphs the body resonator through membrane, wood, metal and glass as it is '
               + 'raised. Wood sits around the middle of the travel and is what the factory '
               + 'presets assume. 0.00 to 1.00.' },
        fr: { t: 'Matériau',
              b: 'Fait évoluer le résonateur de caisse de la membrane au bois, puis au métal et '
               + 'au verre à mesure qu’on le monte. Le bois se situe vers le milieu de la course '
               + 'et c’est ce que supposent les préréglages d’usine. 0,00 à 1,00.',
              reviewed: false },
    },
    'tip.size': {
        en: { t: 'Size',
              b: 'Scales the body resonator’s frequencies, from a violin-sized box at the bottom '
               + 'of the travel to a double-bass-sized one at the top. Larger bodies move their '
               + 'formants down and add low-mid weight. 0.00 to 1.00.' },
        fr: { t: 'Taille',
              b: 'Met à l’échelle les fréquences du résonateur de caisse, d’une caisse de violon '
               + 'en bas de course à une caisse de contrebasse en haut. Les grandes caisses '
               + 'descendent leurs formants et ajoutent du poids dans le bas-médium. '
               + '0,00 à 1,00.',
              reviewed: false },
    },
    'tip.brightness': {
        en: { t: 'Brightness',
              b: 'Sets the bridge filter cutoff, which is the top edge of everything the string '
               + 'sends into the body. Lower it to darken a harsh bow; the knob is skewed so '
               + 'most of its travel sits in the musically useful part of the range. '
               + '20 to 20000 Hz.' },
        fr: { t: 'Brillance',
              b: 'Règle la coupure du filtre de chevalet, soit la limite haute de tout ce que la '
             + 'corde envoie dans la caisse. Baissez-la pour assombrir un archet dur ; la '
             + 'réponse du bouton est non linéaire afin que l’essentiel de sa course couvre la '
             + 'partie réellement utile de la plage. 20 à 20000 Hz.',
              reviewed: false },
    },
    'tip.bodyAmt': {
        en: { t: 'Body Amount',
              b: 'The dry/wet blend between the bare string and the body resonator. At 0 you '
               + 'hear the waveguide alone, which is thin and synthetic; at 1 the body '
               + 'dominates. 0.00 to 1.00.' },
        fr: { t: 'Quantité de caisse',
              b: 'Le dosage entre la corde nue et le résonateur de caisse. À 0 on entend le '
               + 'guide d’ondes seul, mince et synthétique ; à 1 la caisse domine. '
               + '0,00 à 1,00.',
              reviewed: false },
    },

    // ── String ──────────────────────────────────────────────────────────────
    'tip.gauge': {
        en: { t: 'String Gauge',
              b: 'Sets the string’s wave impedance — thin and bright at the bottom of the '
               + 'travel, thick and dark at the top. It changes how much of the bow’s energy the '
               + 'string accepts, so a heavy gauge needs more pressure to speak. 0.10 to 2.00.' },
        fr: { t: 'Calibre de corde',
              b: 'Règle l’impédance d’onde de la corde : fine et brillante en bas de course, '
               + 'épaisse et sombre en haut. Cela change la part d’énergie que la corde accepte '
               + 'de l’archet, si bien qu’un gros calibre demande plus de pression pour parler. '
               + '0,10 à 2,00.',
              reviewed: false },
    },

    // ── Sympathetic strings ─────────────────────────────────────────────────
    // THE ONE PLACE THE DUMP NAME AND THE CAPTION GENUINELY DISAGREE: the dump
    // says "Sympathetic Strings", the caption says "Count". The dump name is
    // used and the body names the caption's word, because "Count" alone in a
    // floating tip says nothing about what is being counted.
    'tip.count': {
        en: { t: 'Sympathetic Strings',
              b: 'How many passive strings ring along with the bowed one, viola d’amore '
               + 'fashion — this is the knob captioned Count. At 0 the section is off and its '
               + 'Amount knob is hidden. 0 to 12 strings.' },
        fr: { t: 'Cordes sympathiques',
              b: 'Le nombre de cordes passives qui vibrent avec la corde frottée, à la manière '
               + 'd’une viole d’amour — c’est le bouton intitulé Nombre. À 0 la section est '
               + 'désactivée et son bouton Quantité est masqué. 0 à 12 cordes.',
              reviewed: false },
    },
    'tip.amount': {
        en: { t: 'Sympathetic Amount',
              b: 'How strongly the bowed string couples into the passive strings, and so how '
               + 'loud their halo sits under the note. This knob is only on screen while '
               + 'Count is above 0. 0.00 to 1.00.' },
        fr: { t: 'Quantité de sympathiques',
              b: 'L’intensité du couplage entre la corde frottée et les cordes passives, donc le '
             + 'volume du halo qu’elles déposent sous la note. Ce bouton n’est à l’écran que '
             + 'lorsque Nombre est supérieur à 0. 0,00 à 1,00.',
              reviewed: false },
    },
    'tip.decay': {
        en: { t: 'Sympathetic Decay',
              b: 'How long the sympathetic strings keep ringing once excited, set by their loss '
               + 'per round trip. Short values give a brief shimmer, long ones a wash that '
               + 'outlasts the note that started it. 0.00 to 1.00.' },
        fr: { t: 'Déclin des sympathiques',
              b: 'La durée pendant laquelle les cordes sympathiques continuent de sonner une '
               + 'fois excitées, fixée par leur perte à chaque aller-retour. Les valeurs courtes '
               + 'donnent un miroitement bref, les longues une nappe qui survit à la note qui '
               + 'l’a déclenchée. 0,00 à 1,00.',
              reviewed: false },
    },

    // ── Footer ──────────────────────────────────────────────────────────────
    'tip.refPitch': {
        en: { t: 'Reference Pitch',
              b: 'The frequency of the reference A that the tuning system is built from. Move it '
               + 'to join an ensemble tuned away from concert pitch; every note follows it. '
               + '220.0 to 880.0 Hz.' },
        fr: { t: 'Diapason',
              b: 'La fréquence du la de référence sur laquelle le système d’accord est '
               + 'construit. Déplacez-la pour rejoindre un ensemble accordé hors du diapason de '
               + 'concert ; toutes les notes suivent. 220,0 à 880,0 Hz.',
              reviewed: false },
    },
    'tip.width': {
        en: { t: 'Stereo Width',
              b: 'Spreads the string and body output across the stereo field. 1.00 is the '
               + 'natural image, below it narrows toward mono, above it widens past the '
               + 'speakers. 0.00 to 2.00.' },
        fr: { t: 'Largeur stéréo',
              b: 'Étale la sortie de la corde et de la caisse dans le champ stéréo. 1,00 est '
               + 'l’image naturelle, en dessous elle se resserre vers le mono, au-dessus elle '
               + 's’élargit au-delà des enceintes. 0,00 à 2,00.',
              reviewed: false },
    },
    // The only parameter on this page with a negative range. U+2212 in BOTH
    // languages: a typographic minus is correct in English prose too, and an
    // entry that spells its own number two ways invites the reviewer to
    // "fix" one of them.
    'tip.output': {
        en: { t: 'Output Level',
              b: 'Master gain on the way out, applied after the body resonator and the stereo '
               + 'stage. Physical models vary a lot in level between presets, so this is the '
               + 'trim that matches them to each other. −60.0 to +12.0 dB.' },
        fr: { t: 'Niveau de sortie',
              b: 'Le gain général en sortie, appliqué après le résonateur de caisse et l’étage '
               + 'stéréo. Le niveau des modèles physiques varie beaucoup d’un préréglage à '
               + 'l’autre ; c’est ici qu’on les égalise entre eux. −60,0 à +12,0 dB.',
              reviewed: false },
    },

    // ── The gear ────────────────────────────────────────────────────────────
    //
    // THIS BODY DESCRIBES ONLY WHAT THE POPOVER ACTUALLY HOLDS. O-Tapestop's
    // wording promises a hover-help on/off toggle; this plugin has no such
    // control and Stage M does not add one, so promising it would be a tip
    // that lies. And it opens DOWNWARDS here — .settings-popover is
    // `top: calc(100% + 8px)`, because the gear sits in a 40 px header strip
    // at the top of a 600 px frame. O-Chorus's tip says "above" and is right
    // about O-Chorus; copying it would have been wrong about this page.
    'tip.settings': {
        en: { t: 'Settings',
              b: 'Opens the settings panel below this button. It holds the interface language '
               + 'and nothing else. Press Escape to close it.' },
        fr: { t: 'Réglages',
              b: 'Ouvre le panneau de réglages sous ce bouton. Il contient la langue de '
               + 'l’interface et rien d’autre. Appuyez sur Échap pour le fermer.',
              reviewed: false },
    },

    // ── The language selector ───────────────────────────────────────────────
    //
    // The two option words are ENDONYMS in both bodies — a language name is
    // never translated. They are not AudioParameterChoice options, so D-01
    // arm 1 is not in play.
    //
    // THE TUNING CLAUSE IS LOAD-BEARING AND IT IS TRUE. This plugin's Tuning
    // page is the SHARED module (CMakeLists.txt embeds
    // modules/tuning/scala-tuning-engine/js/tuning-panel.js by reference), so
    // it is English in both languages — the same verdict K4 recorded for
    // O-Wind. Leaving the clause out would make this the one tip on the page
    // that overpromises.
    'tip.language': {
        en: { t: 'Language',
              b: 'Chooses the language of every caption, tooltip and accessible name on this '
               + 'panel, and the choice is saved with the plugin. Value readouts and the Tuning '
               + 'page stay in English. English or Français.' },
        fr: { t: 'Langue',
              b: 'Choisit la langue de tous les libellés, info-bulles et noms accessibles de ce '
               + 'panneau ; le choix est enregistré avec le plugin. Les valeurs affichées et la '
               + 'page Accord restent en anglais. English ou Français.',
              reviewed: false },
    },
});

// ============================================================================
// LABELS — the on-page text
//
// A LABELS entry is {en:{t}, fr:{t, reviewed}} — ONE string, no body, because a
// label is not a tooltip. Rendered into a fixed box on the page rather than
// into a tip that can size itself, which is why every entry below was chosen
// against a measured budget rather than for prose quality alone.
// ============================================================================

export const LABELS = Object.freeze({

    // ── Header bar ──────────────────────────────────────────────────────────
    // "Enr." rather than "Enreg." (38.00 px of text) or "Sauver" (39.36): the
    // button is pinned to 54 px so it cannot push the flex:1 preset display,
    // which leaves 28 px of text budget inside its 12 px side padding. English
    // "Save" is 26.55 and "Enr." is 25.09, so both clear it and neither wraps.
    'label.save':      { en: { t: 'Save' },     fr: { t: 'Enr.',     reviewed: false } },
    // "Accordage" (55.09) does not fit the 62 px pin; "Accord" (37.13) does,
    // against English "Tuning" at 37.97.
    'label.tuning':    { en: { t: 'Tuning' },   fr: { t: 'Accord',   reviewed: false } },
    // The settings popover's one row.
    'label.language':  { en: { t: 'Language' }, fr: { t: 'Langue',   reviewed: false } },

    // ── Bow ─────────────────────────────────────────────────────────────────
    'label.bow':       { en: { t: 'Bow' },      fr: { t: 'Archet',   reviewed: false } },
    'label.speed':     { en: { t: 'Speed' },    fr: { t: 'Vitesse',  reviewed: false } },
    'label.pressure':  { en: { t: 'Pressure' }, fr: { t: 'Pression', reviewed: false } },
    'label.position':  { en: { t: 'Position' }, fr: { t: 'Position', reviewed: false, sameAsEn: true } },
    // Colophane is the instrument-maker's word for rosin, not a calque.
    'label.rosin':     { en: { t: 'Rosin' },    fr: { t: 'Colophane', reviewed: false } },
    // THE SAME ENGLISH WORD, A SECOND ANSWER, DECIDED BY GEOMETRY — the O-Bass
    // OUT/OUTPUT precedent. This one labels a Humanize grid COLUMN, whose width
    // is the column's width, and the full "Colophane" would push the column 14
    // px wider than its 39.75 px track and overlap its neighbour. Keyed rather
    // than exempted so the abbreviation stays on the reviewer's list.
    'label.rosinShort': { en: { t: 'Rosin' },   fr: { t: 'Coloph.',  reviewed: false } },
    'label.noise':     { en: { t: 'Noise' },    fr: { t: 'Bruit',    reviewed: false } },
    // "Raideur du crin" is bow-hair stiffness; the English is already
    // abbreviated ("Hair Stiff.") and the French is abbreviated to match,
    // because the unabbreviated form measures 63 px in a 62 px box.
    'label.hairStiff': { en: { t: 'Hair Stiff.' }, fr: { t: 'Raid. crin', reviewed: false } },

    // ── Humanize ────────────────────────────────────────────────────────────
    'label.humanize':  { en: { t: 'Humanize' }, fr: { t: 'Humanisation', reviewed: false } },
    'label.amt':       { en: { t: 'Amt' },      fr: { t: 'Qté',      reviewed: false } },
    // "Fréq." and not "Vitesse": this knob sets the drift rate in Hz and sits
    // directly under a column captioned "Vitesse" (Speed). Two adjacent knobs
    // both reading VITESSE would be a translation that loses information the
    // English carries.
    'label.rate':      { en: { t: 'Rate' },
                       fr: { t: 'Fréq.', reviewed: false,
                             termNote: 'the control IS a frequency in Hz — HumanizeEngine.h:83 '
                            + 'maps it to a 0.15-8 Hz smoothing corner — and it sits directly '
                            + 'under a column captioned Vitesse (Speed). Two adjacent knobs both '
                            + 'reading VITESSE would drop information the English carries. '
                            + 'Vitesse fits (34.94 px in a 64 px cap); it is meaning, not width.' } },

    // ── Visualisation tabs ──────────────────────────────────────────────────
    // The tab buttons are `flex: 1` in a 498 px bar, so each box is 166 px in
    // both languages by construction. Only the TEXT can misbehave, and the
    // longest French here is 133.19 px — one line, 32.8 px of slack.
    'label.vizBowString':     { en: { t: 'Bow-String' },    fr: { t: 'Archet-corde', reviewed: false } },
    'label.vizBodySpectrum':  { en: { t: 'Body Spectrum' }, fr: { t: 'Spectre de la caisse', reviewed: false } },
    // Schelleng is the physicist; a name is never translated.
    'label.vizSchelleng':     { en: { t: 'Schelleng' },     fr: { t: 'Schelleng', reviewed: false, sameAsEn: true } },

    // ── Impossible physics ──────────────────────────────────────────────────
    // .impossible-label is `writing-mode: vertical-rl`, so its LENGTH is its
    // HEIGHT. The word is identical in both languages, which is the only reason
    // that box needs no pin.
    'label.impossible':   { en: { t: 'Impossible' },    fr: { t: 'Impossible', reviewed: false, sameAsEn: true } },
    'label.infSustain':   { en: { t: 'Inf. Sustain' },
                            fr: { t: 'Tenue inf.', reviewed: false,
                                  termNote: 'the caption half of tip.infSustain’s exemption — '
                                 + 'sustain here is a note that keeps ringing, not the ADSR '
                                 + 'segment' } },
    'label.revFriction':  { en: { t: 'Rev. Friction' }, fr: { t: 'Frict. inv.', reviewed: false } },
    'label.subHarm':      { en: { t: 'Sub Harm.' },     fr: { t: 'Sous-harm.', reviewed: false } },

    // ── Body ────────────────────────────────────────────────────────────────
    // "Caisse" — the soundbox of a bowed string instrument, not "corps".
    'label.body':       { en: { t: 'Body' },       fr: { t: 'Caisse',    reviewed: false } },
    'label.material':   { en: { t: 'Material' },   fr: { t: 'Matériau',  reviewed: false } },
    'label.size':       { en: { t: 'Size' },       fr: { t: 'Taille',    reviewed: false } },
    'label.brightness': { en: { t: 'Brightness' }, fr: { t: 'Brillance', reviewed: false } },
    'label.bodyAmt':    { en: { t: 'Body Amt' },   fr: { t: 'Qté caisse', reviewed: false } },

    // ── String ──────────────────────────────────────────────────────────────
    'label.string':     { en: { t: 'String' },     fr: { t: 'Corde',     reviewed: false } },
    'label.gauge':      { en: { t: 'Gauge' },      fr: { t: 'Calibre',   reviewed: false } },

    // ── Sympathetic strings ─────────────────────────────────────────────────
    'label.sympathetic': { en: { t: 'Sympathetic' }, fr: { t: 'Sympathiques', reviewed: false } },
    'label.count':      { en: { t: 'Count' },      fr: { t: 'Nombre',    reviewed: false } },
    'label.amount':     { en: { t: 'Amount' },     fr: { t: 'Quantité',  reviewed: false } },
    'label.decay':      { en: { t: 'Decay' },      fr: { t: 'Déclin',    reviewed: false } },

    // ── Footer ──────────────────────────────────────────────────────────────
    // "Diapason" is the reference pitch itself (le diapason est à 440 Hz), so
    // the qualifier the English needs is carried by the word. "Diapason réf."
    // measures 72.55 px against a 62 px cap and would have been clipped.
    'label.refPitch':   { en: { t: 'Ref Pitch' },  fr: { t: 'Diapason',  reviewed: false } },
    'label.width':      { en: { t: 'Width' },      fr: { t: 'Largeur',   reviewed: false } },
    'label.output':     { en: { t: 'Output' },     fr: { t: 'Sortie',    reviewed: false } },

    // ── Accessible names ────────────────────────────────────────────────────
    // Every one of these is the text of a native title= attribute v1.4.1
    // carried, MOVED not rewritten (contract §4). No hover-help prose is
    // invented here; that is Stage M.
    'aria.presetPrev':   { en: { t: 'Previous Preset' },
                           fr: { t: 'Préréglage précédent', reviewed: false } },
    'aria.presetNext':   { en: { t: 'Next Preset' },
                           fr: { t: 'Préréglage suivant', reviewed: false } },
    'aria.presetBrowse': { en: { t: 'Click to browse presets' },
                           fr: { t: 'Cliquer pour parcourir les préréglages', reviewed: false } },
    'aria.settings':     { en: { t: 'Settings' },
                           fr: { t: 'Réglages', reviewed: false } },
    'aria.langSelect':   { en: { t: 'Interface language' },
                           fr: { t: 'Langue de l’interface', reviewed: false } },

    // ── The one string this page writes from script ─────────────────────────
    // Written through setLabel(), so the element becomes a [data-i18n] element
    // from that moment on and the language sweep owns it. A raw literal there
    // is stranded in the previous language the instant the selector fires.
    'ui.tuningPanelFailed': { en: { t: 'Tuning panel failed to load.' },
                              fr: { t: 'Échec du chargement du panneau d’accord.', reviewed: false } },
});

// ============================================================================
// I18N_EXEMPT — reasoned exclusions, never silence
//
// Every visible string the coverage scan finds must be a [data-i18n] element, a
// setLabel() call, or an entry HERE WITH A REASON. A bare skip list would let a
// missed label hide as a deliberate one.
//
// An entry is [text, reason] or [text, reason, scope]. A SCOPE is required only
// where a string is exempt AND keyed on the same page — the one state in which
// the gate cannot tell a deliberate skip from a label somebody forgot. None of
// the three below is in that state: each of these strings occurs exactly once
// in the served markup and is keyed nowhere, so all three are correctly
// unscoped. Verified by grep, not assumed.
// ============================================================================

export const I18N_EXEMPT = [
    ['O-Bowed',  'the product name, in .plugin-name — a product name is never translated'],
    ['Ouaricon', 'the maker, in .brand-label — a company name is never translated'],
    // The preset manager writes the loaded preset's name into this node at init
    // and on every preset change; "Default" is the placeholder it overwrites on
    // its first pass.
    ['Default',  'a factory preset name — exempt under D-02, because the name IS the JSON '
               + 'filename (OuariconPresetManager.h): a session saved against "Violin" would '
               + 'not resolve "Violon"'],
];

// ============================================================================
// TIP_BINDINGS — [selector, key] or [selector, key, wrapper]
//
// applyI18n() runs document.querySelector(sel), then closest(wrapper) if a
// third element is present, and writes data-tip-title + data-tip onto whatever
// it lands on. A binding that resolves to nothing only console.warns, so
// tests/ui_tip_render_check.js asserts every one of these resolves and that
// boot-all-uis sees no "tip target not found" line.
//
// ── "BIND TO THE IDS THE UI ALREADY USES" IS WRONG HERE TOO, and for the
//    sixth distinct reason in this stage ──────────────────────────────────────
//
// The two halves of that instruction fail INDEPENDENTLY and this page fails
// only the first. Measured, both ways:
//
//   SELECTOR half — FALSE. 28 of the 30 anchors below carry no id at all.
//     The knobs are addressed the way the page's own bindSliderParam() does it
//     (index.html:1191), by `.knob-control[data-param="..."]`. Exactly two
//     .knob-control nodes have an id — #sympAmount-ctrl and #sympDecay-ctrl —
//     and using them would make two rows read differently from the other
//     twenty-six for no gain. CORRECTED at v1.6.1: only the FIRST is on the
//     conditional-visibility path. updateSympVisibility() (index.html:1395)
//     reads #sympAmount-ctrl and nothing else, so #sympDecay-ctrl's id is
//     unused by any code on the page and Sympathetic Decay stays on screen at
//     Count 0, where it is as inert as the knob beside it that hides.
//
//   WRAPPER half — NOT NEEDED. .knob-control IS the hover cell: a 62 px column
//     holding the 55 px SVG, the caption and the readout, and nothing else
//     (index.html:400 `.knob-control { width: 62px }`). There is no 4 px
//     stroke to aim at and no shrink-wrapping parent to walk up to, so every
//     binding is a bare two-element row. This is the O-Bassoon shape, not the
//     O-Comp one.
//
// ── THE CHROME BINDS BARE, and on this page it MUST ─────────────────────────
//
// #gear-btn and #settings-popover are both inside .settings-cluster
// (index.html:769). A wrapper walk to that cluster would make hovering
// #lang-select resolve to the gear's own tip — the O-Comp trap, and the same
// ancestor shape. Both rows are bare.
//
// ── FINDING: one dumped parameter has NO CONTROL and therefore NO TIP ────────
//
// `tuningSystem` — AudioParameterChoice, 3 options ("Scala/TUN", "MTS-ESP",
// "12-TET"), default 12-TET (PluginProcessor.cpp:224-230). It is automatable
// and host-reachable, and PluginEditor.cpp:78 even builds a WebComboBoxRelay
// for it — but there is no <select> anywhere on the page bound to it, and the
// page's own bindComboBox() helper (index.html:1275) is never called. The
// shared tuning panel does not carry one either. So the dump's 29 parameters
// produce 28 controls and 28 parameter tips.
//
// NOT FIXED, deliberately. Adding a selector is a feature change with a
// geometry cost and a host-visible surface, which is not this stage's scope,
// and authoring a body for it would be an ORPHAN that check-i18n assertion 2
// fails by design.
// ============================================================================

export const TIP_BINDINGS = [
    // Bow
    ['.knob-control[data-param="bowSpeed"]',              'tip.bowSpeed'],
    ['.knob-control[data-param="bowPressure"]',           'tip.bowPressure'],
    ['.knob-control[data-param="bowPosition"]',           'tip.bowPosition'],
    ['.knob-control[data-param="rosin"]',                 'tip.rosin'],
    ['.knob-control[data-param="bowNoise"]',              'tip.bowNoise'],
    ['.knob-control[data-param="bowHairStiffness"]',      'tip.hairStiff'],

    // Humanize
    ['.knob-control[data-param="humanizeSpeedRange"]',    'tip.humSpeedAmt'],
    ['.knob-control[data-param="humanizeSpeedRate"]',     'tip.humSpeedRate'],
    ['.knob-control[data-param="humanizePressureRange"]', 'tip.humPressAmt'],
    ['.knob-control[data-param="humanizePressureRate"]',  'tip.humPressRate'],
    ['.knob-control[data-param="humanizePositionRange"]', 'tip.humPosAmt'],
    ['.knob-control[data-param="humanizePositionRate"]',  'tip.humPosRate'],
    ['.knob-control[data-param="humanizeRosinRange"]',    'tip.humRosinAmt'],
    ['.knob-control[data-param="humanizeRosinRate"]',     'tip.humRosinRate'],

    // Impossible physics
    ['.knob-control[data-param="infiniteSustain"]',       'tip.infSustain'],
    ['.knob-control[data-param="reversedFriction"]',      'tip.revFriction'],
    ['.knob-control[data-param="subHarmonics"]',          'tip.subHarm'],

    // Body
    ['.knob-control[data-param="bodyMaterial"]',          'tip.material'],
    ['.knob-control[data-param="bodySize"]',              'tip.size'],
    ['.knob-control[data-param="brightness"]',            'tip.brightness'],
    ['.knob-control[data-param="bodyAmount"]',            'tip.bodyAmt'],

    // String
    ['.knob-control[data-param="stringGauge"]',           'tip.gauge'],

    // Sympathetic
    ['.knob-control[data-param="sympatheticCount"]',      'tip.count'],
    ['.knob-control[data-param="sympatheticAmount"]',     'tip.amount'],
    ['.knob-control[data-param="sympatheticDecay"]',      'tip.decay'],

    // Footer
    ['.knob-control[data-param="referencePitch"]',        'tip.refPitch'],
    ['.knob-control[data-param="width"]',                 'tip.width'],
    ['.knob-control[data-param="outputLevel"]',           'tip.output'],

    // Chrome — BARE, see above
    ['#gear-btn',                                         'tip.settings'],
    ['#lang-select',                                      'tip.language'],
];

export function tr(key, lang, vars) {
    const entry = I18N[key];
    if (!entry) { console.warn(`i18n: missing key ${key}`); return { t: key, b: '' }; }
    const s = entry[lang] || entry.en;

    // A var VALUE that is itself an I18N key resolves to that key's localized
    // title; anything else is used literally. This plugin needs neither arm
    // today, but the canon is ONE shape across all 43 plugins and this function
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
