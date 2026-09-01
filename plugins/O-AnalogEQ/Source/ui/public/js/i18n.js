/*
   This file is part of O-AnalogEQ, an Ouaricon Audio plugin.
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
// i18n.js — O-AnalogEQ page labels and hover-help, English + French (v1.3.1)
//
// An ES module that EXPORTS ONLY. It must never self-execute: a bare top-level
// statement here throws out of module evaluation and takes every later
// initializer on the page with it (pattern_module_toplevel_init_tdz). This
// plugin's controller is an inline <script type="module"> in index.html, so
// that failure mode would take the WHOLE UI, not a panel of it.
// scripts/check-i18n.js assertion 7 enforces it.
//
// FILENAME: no hyphen. juce_add_binary_data STRIPS hyphens rather than
// converting them to underscores, so a second file named i18n-fr.js would have
// to be reached as the symbol i18nfr_js (critical_binary_data_strips_hyphens).
// One combined file for both languages sidesteps the question entirely.
//
// ── v1.3.1: FRENCH QA PASS (Stage N, 2026-08-31) ───────────────────────────
// Every fr entry read against its en and against scripts/i18n-fr-glossary.js.
// Changed: 14 of 43 entries (3 terminology, 10 typography, 4 grammar/idiom,
// 3 meaning — an entry can carry more than one). sameAsEn: kept 2, translated
// 0. termNote exemptions: 0 — no contextual glossary exemption was needed here.
// i18n-fr-lint 20 findings -> 0, --strict exit 0 (7 T4, 5 T5, 4 T7, 2 G1, 2 F1).
// The lint also lists FOUR straight copies (fr === en once normalised): LMF and
// HMF, which carry sameAsEn, plus label.analog "ANALOG." and tip.analog's title
// "Analog.", which do NOT. Those two are the French ABBREVIATION of Analogique
// — 66.70 px against a 57.00 px box, see the label.analog block — and they are
// spelled like the English word only because the English word is the stem they
// share. They are not left untranslated, so they are not flagged sameAsEn, and
// the glossary settles "analog" as "analog" in any case.
// Left as drafted: the rest. reviewed: false throughout — no native speaker yet.
//
// The decisions a later reader needs:
//
//   SAUVER -> ENREG., AND THE v1.3.0 WIDTH DEFENCE OF SAUVER WAS WRONG BY
//   MEASUREMENT. See the preset-button block in LABELS: the glossary's
//   abbreviation is 3.75 px NARROWER than the calque it replaces.
//
//   LABEL IN NAME HOLDS BY STEM ONLY on #savePreset from here on — "ENREG." is
//   not a substring of "Enregistrer un préréglage dans un fichier". Recorded in
//   the aria block; no caption was invented to close it.
//
//   aria.presetList takes the glossary's settled INFINITIVE ("Cliquer pour
//   parcourir les préréglages") while the three tooltip bodies keep the
//   IMPERATIVE their English uses ("Cliquez sur l’étiquette"). The glossary
//   settles that one accessible name across every preset bar in the suite; the
//   register rule ("pick one per plugin") governs prose bodies, which are not
//   the same register class as a fixed accessible name.
//
//   THE FOUR BAND TIP TITLES AND tip.analog KEEP THE CAPTION'S ABBREVIATION —
//   LF Plat., HF Plat., Analog. — rather than expanding to Plateau LF /
//   Analogique in the roomier tooltip surface. M2's finding that "the caption
//   wins" reverses for a TRUNCATED caption allows either branch; this is the
//   branch taken, because on this page the caption IS the switch the user is
//   pointing at and identification beats expansion. Recorded so the reviewer
//   can reverse it deliberately rather than discover it.
//
//   Off / On STAY ENGLISH in the bodies, unchanged: they are the words the
//   host's automation lane shows for the five AudioParameterBools. The state
//   verb in the same sentence is French ("Quand il est désactivé").
//
//   THE Q VALUES ARE SEPARATED BY SEMICOLONS — "Q 0,5 ; 1,0 et 2,0" — because
//   the decimal COMMA this suite settled on makes "0,5, 1,0 et 2,0" ambiguous.
//   A semicolon list whose members contain commas is the standard French form.
//
//   Ranges read "de X à Y" inside a sentence, per the glossary's Ranges rule.
//   LMF and HMF stay sameAsEn — the band-caption block below says why.
//
// ── <html lang> NOW FOLLOWS THE SELECTOR (canon change, all 43 plugins) ─────
//
// applyI18n() sets document.documentElement.lang = uiLanguage, so assistive
// technology reads the page in the language it is being displayed in. The canon
// was synced into every canon-bearing file at repo level with no version bump;
// the user-visible half of it ships with this version.
//
// ── v1.3.0 GIVES THIS PLUGIN HOVER-HELP, AND A RENDERER TO PAINT IT ────────
//
// v1.2.0 shipped with I18N and TIP_BINDINGS both EMPTY, which was that
// version's correct state: v1.1.11 carried no data-tip anywhere on the page,
// only five native title= attributes on the preset bar that contract §4 DELETES
// rather than localizes. Authoring the prose was deferred to Stage M, and this
// is Stage M.
//
// THE ATTRIBUTES ARE NOT THE FEATURE. applyI18n() writes data-tip-title and
// data-tip onto each anchor named in TIP_BINDINGS and stops there; the code
// that READS those attributes and paints a surface is per-plugin and did not
// exist on this page at all — no #tooltip node, no .tooltip rule, no hover
// handler. Authoring thirteen bodies and binding them, with no other change,
// would have shipped thirteen INVISIBLE strings past three green gates:
// check-i18n reads the table statically, check-ui-labels has no tooltip
// awareness whatsoever, and boot-all-uis counts aria-label and title and never
// data-tip. index.html therefore also gains the delegated renderer and its CSS,
// and plugins/O-AnalogEQ/tests/ui_tip_render_check.js is the seat where a tip
// that never appeared FAILS instead of passing quietly.
//
// THIRTEEN TIPS FOR FIFTEEN OF SIXTEEN PARAMETERS, and both gaps are findings
// rather than omissions — see the TIP_BINDINGS block at the foot of this file
// for the dual-knob pairing and for output_gain, which has no control at all.
//
// COPY IS textContent ON EVERY PATH — never innerHTML. check-i18n assertion 9
// rejects any innerHTML reference here and any string literal containing an
// angle bracket, so machine-drafted French cannot open a markup path.
//
// ALL FRENCH IS MACHINE-DRAFTED AND FLAGGED `reviewed: false`. No native
// speaker has read it. `node scripts/check-i18n.js` prints the worklist.
// ============================================================================

export const LANGUAGES = ['en', 'fr'];

// ============================================================================
// I18N — hover-help copy (v1.3.0). A tooltip entry is {t, b}: a title and a
// body, in each language.
//
// ── WHERE THE RANGES COME FROM ─────────────────────────────────────────────
//
// .planning/params.tsv, produced by a RUNTIME walk of getParameters() on a
// constructed processor (scripts/param-dump), not by a regex over
// createParameterLayout(). Sixteen parameters.
//
// ELEVEN OF THE SIXTEEN CARRY A REAL `label` COLUMN — every *_freq is "Hz" and
// every *_gain is "dB", and each agrees with the page's own formatter at
// index.html:963-971. The brief's "label is empty far more often than the plan
// implies" is FALSE on this plugin: only the two Q choices and the five bools
// have an empty label, and for those the range is option words rather than a
// number, so there is no unit to recover.
//
// ONE RANGE HAD TO BE RECOVERED FROM THE FORMATTER ANYWAY, and it is hf_freq.
// The dump says textAtMax 20000.0, but index.html:970 renders
// `hz >= 10000 ? (hz / 1000).toFixed(1) + 'k' : Math.round(hz)`, so the top of
// that knob READS "20.0k Hz" and never "20000 Hz". The body below quotes what
// the user sees.
//
// ── THE NUMBERS ARE SPELLED DIFFERENTLY IN THE TWO LANGUAGES, ON PURPOSE ────
//
// A tooltip body is PROSE and takes French convention: decimal COMMA, a space
// before a unit, U+2212 for the minus sign. The READOUT keeps its point —
// #lf_db renders "0.0 dB" in both languages because D-03 exempts the readout
// NODE, and that has not moved. So `−12.0 to +12.0 dB` here becomes
// `−12,0 à +12,0 dB` there while the readout under the knob still says
// `-12.0 dB`. They differ because one is prose and the other is a
// machine-formatted value. Settled by the developer, 2026-08-30; three M1
// plugins had to be corrected in f0eb50c8 for getting it the other way.
//
// ── OPTION WORDS STAY ENGLISH INSIDE A FRENCH SENTENCE ──────────────────────
//
// WIDE / MED / TIGHT are lmf_q and hmf_q AudioParameterChoice options, exempt
// on the PAGE under D-01 arm 1 (see I18N_EXEMPT below) because the face and the
// host automation lane must agree. Inside a tooltip body they are being NAMED
// rather than displayed, so the sentence around them is French and the three
// words are not: a French user reading "TIGHT" in the body then finds exactly
// "TIGHT" on the control and in the DAW's automation lane. Off / On are the
// same case for the five booleans.
//
// ── COPY IS textContent ON EVERY PATH ───────────────────────────────────────
//
// No string below contains an angle bracket (check-i18n assertion 9), and the
// renderer builds the surface with createElement + createTextNode rather than
// innerHTML, so machine-drafted French cannot open a markup path.
// ============================================================================

export const I18N = Object.freeze({

    // ── THE FOUR DUAL KNOBS ─────────────────────────────────────────────────
    //
    // ONE TIP EACH, FOR TWO PARAMETERS, AND THAT IS FORCED BY THE PAGE RATHER
    // THAN CHOSEN. Each band is a single concentric control: `.knob-outer`
    // (frequency) and `.knob-inner` (gain) are BOTH `pointer-events: none`
    // (index.html:229, :271), so neither is hoverable and neither can carry a
    // tip. The only node that receives a pointer event is their parent
    // `.dual-knob-container`, which decides outer-vs-inner from the cursor's
    // DISTANCE FROM THE CENTRE at pointerdown (INNER_THRESHOLD 0.60,
    // index.html:1000-1010) — a radius, not a child boundary, and nothing a
    // hover anchor can be attached to.
    //
    // So one anchor holds one tip that names both rings, which is the same
    // trade O-Texture made for its X/Y pad. Adding a second binding on the same
    // node would SILENTLY OVERWRITE the first — applyI18n writes onto whatever
    // the selector resolves to — while check-i18n cheerfully reported two bound
    // tips. Making the two rings separately hoverable is a change to a working
    // control's hit-testing and is NOT made here.
    'tip.lfBand': {
        en: { t: 'LF Frequency & Gain',
              b: 'The outer ring sets the corner frequency of the low shelf and the inner '
               + 'dial sets its gain, so everything below the corner is lifted or cut '
               + 'together. Reach for it to put weight under a thin source, or to clear mud '
               + 'without touching the mids. Frequency 30 to 500 Hz; gain −12.0 to +12.0 dB.' },
        fr: { t: 'LF — Fréquence et gain',
              b: 'La bague extérieure règle la fréquence de coupure du plateau grave et le '
               + 'cadran intérieur son gain : tout ce qui est sous la coupure est relevé '
               + 'ou atténué d’un bloc. À utiliser pour donner du corps à une source maigre, '
               + 'ou pour dégager le bas sans toucher aux médiums. Fréquence de 30 à '
               + '500 Hz ; gain de −12,0 à +12,0 dB.',
              reviewed: true },
    },

    'tip.lmfBand': {
        en: { t: 'LMF Frequency & Gain',
              b: 'The outer ring sets the centre frequency of the low-mid bell and the inner '
               + 'dial sets its gain, lifting or cutting a band around that centre while '
               + 'leaving the rest alone. This is where boxiness and body live on most '
               + 'sources. Frequency 100 to 2000 Hz; gain −12.0 to +12.0 dB.' },
        fr: { t: 'LMF — Fréquence et gain',
              b: 'La bague extérieure règle la fréquence centrale de la cloche bas-médium et '
               + 'le cadran intérieur son gain, relevant ou atténuant une bande autour de ce '
               + 'centre sans toucher au reste. C’est là que se logent le corps et l’effet de '
               + 'boîte sur la plupart des sources. Fréquence de 100 à 2000 Hz ; gain '
               + 'de −12,0 à +12,0 dB.',
              reviewed: true },
    },

    'tip.hmfBand': {
        en: { t: 'HMF Frequency & Gain',
              b: 'The outer ring sets the centre frequency of the high-mid bell and the inner '
               + 'dial sets its gain. Reach for it for presence and attack, or to pull back '
               + 'harshness in the range the ear is most sensitive to. Frequency 500 to '
               + '8000 Hz; gain −12.0 to +12.0 dB.' },
        fr: { t: 'HMF — Fréquence et gain',
              b: 'La bague extérieure règle la fréquence centrale de la cloche haut-médium et '
               + 'le cadran intérieur son gain. À utiliser pour la présence et l’attaque, ou '
               + 'pour adoucir la dureté dans la zone où l’oreille est la plus sensible. '
               + 'Fréquence de 500 à 8000 Hz ; gain de −12,0 à +12,0 dB.',
              reviewed: true },
    },

    // The one range that had to be read off the formatter rather than the dump:
    // index.html:970 prints a "k" abbreviation above 10 kHz, so the top of this
    // knob reads "20.0k Hz" and the dump's 20000.0 is never displayed.
    'tip.hfBand': {
        en: { t: 'HF Frequency & Gain',
              b: 'The outer ring sets the corner frequency of the high shelf and the inner '
               + 'dial sets its gain, so everything above the corner is lifted or cut '
               + 'together. Reach for it for air and sheen, or to take the edge off a bright '
               + 'source. Frequency 2000 Hz to 20.0k Hz; gain −12.0 to +12.0 dB.' },
        fr: { t: 'HF — Fréquence et gain',
              b: 'La bague extérieure règle la fréquence de coupure du plateau aigu et le '
               + 'cadran intérieur son gain : tout ce qui est au-dessus de la coupure '
               + 'est relevé ou atténué d’un bloc. À utiliser pour l’air et le brillant, ou '
               + 'pour arrondir une source trop mordante. Fréquence de 2000 Hz à '
               + '20,0k Hz ; gain de −12,0 à +12,0 dB.',
              reviewed: true },
    },

    // ── THE FOUR BAND SWITCHES ──────────────────────────────────────────────
    //
    // Each band caption IS its own on/off switch (an AudioParameterBool whose
    // only face is a CSS class), so the title here is the caption the user is
    // pointing at rather than the dump's "LF On" — the page wins over the
    // automation lane for a title, per the brief.
    //
    // "Skipped rather than flattened" is measured, not idiomatic: processBlock
    // guards each stage with `if (lfOn) lfFilter.process(context)`
    // (PluginProcessor.cpp:344-347), so an off band costs nothing and cannot
    // colour the signal at all.
    'tip.lfOn': {
        en: { t: 'LF Shelf',
              b: 'Click the caption to switch the low shelf in or out of the signal path. '
               + 'When it is off the filter stage is skipped outright rather than flattened, '
               + 'so the band cannot colour the sound at all, and the caption dims. '
               + 'Off or On.' },
        fr: { t: 'LF Plat.',
              b: 'Cliquez sur l’étiquette pour insérer ou retirer le plateau grave du trajet '
               + 'du signal. Quand il est désactivé, l’étage de filtrage est court-circuité '
               + 'plutôt qu’aplani : la bande ne peut plus colorer le son du tout, et '
               + 'l’étiquette s’estompe. Off ou On.',
              reviewed: true },
    },

    'tip.lmfOn': {
        en: { t: 'LMF Bell',
              b: 'Click the caption to switch the low-mid bell in or out of the signal path. '
               + 'When it is off the filter stage is skipped outright rather than flattened, '
               + 'and the caption dims. Off or On.' },
        fr: { t: 'Cloche LMF',
              b: 'Cliquez sur l’étiquette pour insérer ou retirer la cloche bas-médium du '
               + 'trajet du signal. Quand elle est désactivée, l’étage de filtrage est '
               + 'court-circuité plutôt qu’aplani, et l’étiquette s’estompe. Off ou On.',
              reviewed: true },
    },

    'tip.hmfOn': {
        en: { t: 'HMF Bell',
              b: 'Click the caption to switch the high-mid bell in or out of the signal path. '
               + 'When it is off the filter stage is skipped outright rather than flattened, '
               + 'and the caption dims. Off or On.' },
        fr: { t: 'Cloche HMF',
              b: 'Cliquez sur l’étiquette pour insérer ou retirer la cloche haut-médium du '
               + 'trajet du signal. Quand elle est désactivée, l’étage de filtrage est '
               + 'court-circuité plutôt qu’aplani, et l’étiquette s’estompe. Off ou On.',
              reviewed: true },
    },

    'tip.hfOn': {
        en: { t: 'HF Shelf',
              b: 'Click the caption to switch the high shelf in or out of the signal path. '
               + 'When it is off the filter stage is skipped outright rather than flattened, '
               + 'so the band cannot colour the sound at all, and the caption dims. '
               + 'Off or On.' },
        fr: { t: 'HF Plat.',
              b: 'Cliquez sur l’étiquette pour insérer ou retirer le plateau aigu du trajet '
               + 'du signal. Quand il est désactivé, l’étage de filtrage est court-circuité '
               + 'plutôt qu’aplani : la bande ne peut plus colorer le son du tout, et '
               + 'l’étiquette s’estompe. Off ou On.',
              reviewed: true },
    },

    // ── THE TWO Q SELECTORS ─────────────────────────────────────────────────
    //
    // The three Q values are qValues[] = { 0.5, 1.0, 2.0 } at
    // PluginProcessor.h:122, indexed by the choice. They are quoted because the
    // three option WORDS say which is broader but not by how much, and a
    // numeric Q is what an engineer coming from a console expects to compare.
    'tip.lmfQ': {
        en: { t: 'LMF Q',
              b: 'Sets how wide a slice of the spectrum the low-mid bell lifts or cuts around '
               + 'its centre frequency. WIDE is broad and musical, TIGHT is surgical enough to '
               + 'pull one resonance without thinning the source. Three settings: WIDE, MED, '
               + 'TIGHT — Q 0.5, 1.0 and 2.0.' },
        fr: { t: 'Q LMF',
              b: 'Règle la largeur de la tranche de spectre que la cloche bas-médium relève ou '
               + 'atténue autour de sa fréquence centrale. WIDE est large et musical, TIGHT '
               + 'est assez chirurgical pour retirer une résonance sans amaigrir la source. '
               + 'Trois réglages : WIDE, MED, TIGHT — Q 0,5 ; 1,0 et 2,0.',
              reviewed: true },
    },

    'tip.hmfQ': {
        en: { t: 'HMF Q',
              b: 'Sets how wide a slice of the spectrum the high-mid bell lifts or cuts around '
               + 'its centre frequency. WIDE is broad and musical, TIGHT is surgical enough to '
               + 'pull one resonance without dulling the source. Three settings: WIDE, MED, '
               + 'TIGHT — Q 0.5, 1.0 and 2.0.' },
        fr: { t: 'Q HMF',
              b: 'Règle la largeur de la tranche de spectre que la cloche haut-médium relève ou '
               + 'atténue autour de sa fréquence centrale. WIDE est large et musical, TIGHT '
               + 'est assez chirurgical pour retirer une résonance sans ternir la source. '
               + 'Trois réglages : WIDE, MED, TIGHT — Q 0,5 ; 1,0 et 2,0.',
              reviewed: true },
    },

    // ── THE SATURATION SWITCH ───────────────────────────────────────────────
    //
    // The stage is `tanh(x * 0.5) * 2.0` (PluginProcessor.cpp:253), placed
    // AFTER all four bands and BEFORE the output gain (cpp:344-352). The 0.5
    // pre-gain with the 2.0 post-gain is what makes it warmth rather than
    // drive, and saying "after the four bands" is the part a user cannot guess
    // from the face.
    'tip.analog': {
        en: { t: 'Analog',
              b: 'Switches a gentle saturation stage in after all four bands, adding harmonic '
               + 'warmth and rounding the peaks the EQ has just created. Reach for it when a '
               + 'clean boost sounds brittle; leave it off for surgical corrective work. '
               + 'Off or On.' },
        fr: { t: 'Analog.',
              b: 'Insère après les quatre bandes un étage de saturation douce qui ajoute une '
               + 'chaleur harmonique et arrondit les crêtes que l’égaliseur vient de créer. À '
               + 'utiliser quand un relèvement propre sonne cassant ; à laisser désactivé '
               + 'pour un travail correctif chirurgical. Off ou On.',
              reviewed: true },
    },

    // ── THE CHROME ──────────────────────────────────────────────────────────
    //
    // THE GEAR TIP DESCRIBES ONLY WHAT THIS POPOVER ACTUALLY HOLDS. It is one
    // row and that row is the language selector; this plugin has no hover-help
    // on/off toggle, so the wording O-Tapestop uses would promise a control
    // that is not there. A tip that lies is worse than no tip.
    'tip.settings': {
        en: { t: 'Settings',
              b: 'Opens the settings panel above this button. It holds one control, the '
               + 'interface language, and closes again on a click outside it or on Escape.' },
        fr: { t: 'Réglages',
              b: 'Ouvre le panneau de réglages au-dessus de ce bouton. Il contient une seule '
               + 'commande, la langue de l’interface, et se referme par un clic à l’extérieur '
               + 'ou par la touche Échap.',
              reviewed: true },
    },

    // D-03 IS RESTATED HERE BECAUSE IT IS THE ONE THING A USER WILL TEST. The
    // captions, the accessible names and this hover-help change language; the
    // value readouts under the knobs keep their English units and their decimal
    // POINT, deliberately.
    'tip.language': {
        en: { t: 'Interface language',
              b: 'Chooses the language of the page: every caption, every accessible name and '
               + 'this hover-help. The value readouts under the knobs keep their numbers and '
               + 'their English units. English or Français.' },
        fr: { t: 'Langue de l’interface',
              b: 'Choisit la langue de la page : chaque étiquette, chaque nom accessible '
               + 'et cette aide contextuelle. Les valeurs affichées sous les boutons rotatifs '
               + 'conservent leurs nombres et leurs unités anglaises. English ou Français.',
              reviewed: true },
    },
});

// ============================================================================
// LABELS — the visible text of the page. {en:{t}, fr:{t, reviewed}}.
//
// One string per entry, no body: a label is not a tooltip.
//
// ── THE FRAME IS 920 x 220. WIDE, BUT ONLY 220 TALL ─────────────────────────
//
// Every French string below was chosen against a MEASURED width, rendered in
// the real node with its own letter-spacing, font-weight and text-transform
// (none of which appears in getComputedStyle().font).
//
// ── THIS PAGE HAS THREE CLIFFS, NOT TWO, AND THE THIRD IS INVISIBLE ─────────
//
// O-Chorus (700x125) has a shrink-to-fit caption inside a fixed container, so
// its two cliffs are 50.00 (the rect widens) and 62.00 (the caption wraps).
// O-DigiDelay (700x196) has a hard-pinned .knob-label, so both of its cliffs
// are 60.00 and the mechanism turns on whether the string has a break
// opportunity. THIS PAGE IS NEITHER SHAPE and it carries a mechanism the other
// two do not:
//
//   A. SPILL — #analog, 57.00 px. `.toggle` is `width: 75px; height: 22px`, a
//      hard pin in BOTH axes, so its rectangle is language-invariant and
//      assertion 7 is structurally blind. A caption past 57.00 is painted
//      outside the button onto bare paper. Caught by check-ui-labels
//      assertion 4, the text-wider-than-its-content-box half. ANALOGIQUE
//      (66.70) fails it by 9.70 px and moves NOTHING.
//
//   B. PUSH — #savePreset / #loadPreset. `.preset-bar` is a right-anchored
//      flex row and these two buttons shrink-to-fit, so a wider caption widens
//      the button and shoves the three controls to its LEFT. Caught by
//      assertion 7. Assertion 4 is blind to it: the caption still fits its own
//      grown box. Unpinned, SAUVER + CHARGER move four elements by dx=-34.37.
//      THE PIN BELOW IS WHAT CLOSES IT.
//
//   C. WRAP INSIDE A PINNED, ABSOLUTELY POSITIONED, AUTO-HEIGHT BOX —
//      .band-label, 67.00 px. THIS ONE IS INVISIBLE TO BOTH ASSERTIONS AND IT
//      IS THIS PAGE'S REAL RISK.
//
//      Each band caption is `position: absolute; width: 85px` set inline, with
//      no fixed height. A two-word French caption past 67.00 px does not spill
//      and does not push:
//        - it WRAPS, so no line exceeds the content box  → assertion 4's
//          horizontal half sees nothing;
//        - the box GROWS to hold its own second line, 21 -> 34 px, so the text
//          never exceeds its own content height → assertion 4's vertical twin
//          (fbdb6930) sees nothing;
//        - the caption IS the [data-i18n] element and it is absolutely
//          positioned, so it moves no sibling → assertion 7 sees nothing.
//      And the visible result is a caption 13 px taller reaching y=86 into the
//      knob ring that starts at y=75. PLATEAU BF renders exactly that, with
//      every assertion green.
//
//      `.band-label { white-space: nowrap }` in index.html converts C into A.
//      It is NOT a geometry pin and is not claimed as one — see the note there.
//
// ── THE FOUR BAND CAPTIONS ──────────────────────────────────────────────────
//
// MEASURED at 11 px Garamond, letter-spacing 0.12em, weight 600, against the
// 67.00 px content box of an 85 px band-label (85 - 2 border - 16 padding):
//
//     LF SHELF  63.03 -> LF PLAT.  57.44    9.56 spare
//     LMF       28.41 -> LMF       28.41    sameAsEn
//     HMF       29.63 -> HMF       29.63    sameAsEn
//     HF SHELF  64.25 -> HF PLAT.  58.66    8.34 spare
//
// TWO OF FOUR SHRINK, and the two that do not change at all. A clip-only check
// would have certified this page — and so, here, would a push check.
//
// NOTE HOW LITTLE ROOM THE ENGLISH HAS: LF SHELF has 3.97 px spare and
// HF SHELF 2.75 px, in ENGLISH, at v1.1.11. There was never room for a longer
// French caption on this control, in any language, and that is a property of
// the authored layout rather than of the translation.
// ============================================================================

export const LABELS = Object.freeze({

    // ── The four band captions ──────────────────────────────────────────────
    //
    // These four divs are BOTH the caption and the band on/off switch: each is
    // an AudioParameterBool (lf_on / lmf_on / hmf_on / hf_on) whose only face
    // is a CSS class, never a rewritten string. So the node holds one fixed
    // caption and never a number — D-01 arm 3 does not apply — and a Bool has
    // no option strings for a French caption to disagree with in a host
    // automation lane, so arm 1 does not apply either.
    //
    // LF / LMF / HMF / HF ARE KEPT VERBATIM, and that is a decision, not an
    // oversight. They are the band abbreviations silk-screened on the French
    // market's own consoles (SSL, API, Neve are sold in France with exactly
    // these four), so translating them would make the plugin LESS legible to
    // its French user, not more. Only SHELF — the filter TYPE — is a word, and
    // it becomes PLAT., the standard abbreviation of "filtre en plateau".
    //
    // LMF and HMF are keyed with sameAsEn rather than dropped into
    // I18N_EXEMPT, deliberately and for the reason O-DigiDelay keyed MOD: an
    // exemption would hide a translation JUDGEMENT from the native-speaker
    // worklist forever, whereas a sameAsEn key is one more `reviewed: false`
    // line somebody has to agree with.
    //
    // Rejected on measurement, not on taste. Every fuller French form is past
    // the 67.00 px wrap cliff and would land in mechanism C, silently:
    //     PLATEAU BF   81.77   two lines, box 21 -> 34 px
    //     PLATEAU HF   82.98   two lines
    //     BAS MEDIUM   85.63   two lines
    //     HAUT MEDIUM  97.14   two lines
    // MED.HAUT (70.77) stays on one line only because it has no space in it,
    // and then SPILLS 3.77 px past the box.
    'label.band.lf':  { en: { t: 'LF SHELF' }, fr: { t: 'LF PLAT.', reviewed: true } },
    'label.band.lmf': { en: { t: 'LMF' },      fr: { t: 'LMF', reviewed: true, sameAsEn: true } },
    'label.band.hmf': { en: { t: 'HMF' },      fr: { t: 'HMF', reviewed: true, sameAsEn: true } },
    'label.band.hf':  { en: { t: 'HF SHELF' }, fr: { t: 'HF PLAT.', reviewed: true } },

    // ── The analog-saturation switch ────────────────────────────────────────
    //
    // `analog` is an AudioParameterBool gating the WaveShaper stage
    // (PluginProcessor.cpp), and like the band labels its face is a CSS class
    // rather than a rewritten string — the caption is fixed and never holds a
    // number, so neither arm 1 nor arm 3 of D-01 touches it.
    //
    // ANALOGIQUE is the word a French user would rather read and it does not
    // fit: 66.70 px against a 57.00 px content box in a button pinned to
    // 75 x 22, so it SPILLS 4.85 px past each edge onto bare paper. Widening
    // the button to 85 px would hold it — there is 63 px of empty paper to its
    // right before the VU meter — but that is a layout change caused by
    // French, and this page did not need one. ANALOG. is the standard French
    // abbreviation OF that word (45.30, 11.70 px spare), which is the same
    // trade O-DigiDelay made for REINJ. and O-Chorus for PROF.
    //
    // SATURATION (62.70) also spills. CHALEUR (47.55) fits and names a
    // different claim — warmth rather than the analogue path — so it was not
    // taken. A reviewer who prefers ANALOGIQUE must widen the button with it.
    'label.analog': { en: { t: 'ANALOG' }, fr: { t: 'ANALOG.', reviewed: true } },

    // ── The VU meter caption ────────────────────────────────────────────────
    //
    // `text-transform: uppercase` on `.vu-meter-label`, so this renders LEVEL /
    // NIVEAU. The box is the meter's full 108 px content width and the text is
    // centred in it, so NIVEAU (41.33) has 66.67 px spare — the roomiest string
    // on the page by a wide margin.
    'label.level': { en: { t: 'Level' }, fr: { t: 'Niveau', reviewed: true } },

    // ── The two preset buttons ──────────────────────────────────────────────
    //
    // `#savePreset, #loadPreset` are PINNED to 62 px in index.html. Rendered
    // text against that pin's 48 px content box — 62 border-box less 2 px
    // border and 12 px padding. RE-MEASURED at v1.3.1 with the gate's own
    // Range.selectNodeContents, on the real node, at the shipping 920 x 220
    // frame:
    //
    //     SAVE  24.52 -> ENREG.   35.50   12.50 px spare   (v1.3.0: SAUVER 39.25)
    //     LOAD  27.16 -> CHARGER  46.80    1.20 px spare   TIGHTEST ON THE PAGE
    //
    // v1.3.0 SHIPPED SAUVER AND DEFENDED IT ON WIDTH, AND THE DEFENCE WAS WRONG
    // BY MEASUREMENT RATHER THAN BY TASTE. ENREG. — what the suite glossary
    // settles for "Save" where a caption is pinned — is 3.75 px NARROWER than
    // the calque it replaces. The term and the geometry wanted the same string
    // all along, and only the second half was ever checked. ENREGISTRER (66.95)
    // still does not fit; the pin is NOT raised to 82 px.
    //
    // CHARGER is the glossary's ROOT term and stays. 62 px is O-Chorus's and
    // O-DigiDelay's number, kept so the suite's preset bar is one shape across
    // the batch. CHARGER's 1.20 px is the tightest French margin shipped here
    // and it is thinner than O-DigiDelay's 1.48 px on the identical string,
    // because this page's 9 px type carries 0.06em letter-spacing. OUVRIR
    // (37.75, 10.25 px spare) is glossary-accepted for "Load" and is the
    // reviewer's lever if 1.20 px is judged too thin on Windows metrics — it is
    // what O-Detune already ships for this control — and taking it would
    // require moving aria.loadPreset's French to match, so that label-in-name
    // still holds.
    'label.save': { en: { t: 'SAVE' }, fr: { t: 'ENREG.',  reviewed: true } },
    'label.load': { en: { t: 'LOAD' }, fr: { t: 'CHARGER', reviewed: true } },

    // ── The preset dropdown's empty line, written through setLabel() ────────
    //
    // CONTRACT §6 — PLURALS ARE AVOIDED, NOT ENGINEERED. This is the one string
    // on the page that could have carried a count, and it is authored so that
    // it never does: "Aucun préréglage" is categorical, correct at exactly
    // zero, and needs no inflection — French treats zero as singular and
    // English does not, and a plural engine for one string on one plugin is not
    // a trade worth making. check-i18n assertion 13 rejects a ternary inside a
    // setLabel argument so a count cannot creep back in later.
    //
    // The English is byte-identical to what v1.1.11 wrote at index.html:1017.
    // No prose was invented; it was moved into the table.
    'label.noPresets': { en: { t: 'No presets' }, fr: { t: 'Aucun préréglage', reviewed: true } },

    // ── The settings popover (v1.2.0) ───────────────────────────────────────
    'label.language': { en: { t: 'Language' }, fr: { t: 'Langue', reviewed: true } },

    // ── Accessible names ────────────────────────────────────────────────────
    //
    // Resolved through the same sweep via data-i18n-aria, so a screen reader
    // hears the language the page is showing.
    //
    // THE FIVE PRESET-BAR NAMES ARE THE DELETED title= TEXT, MOVED, NOT
    // AUTHORED. v1.1.11 carried title="Previous preset", "Click to browse
    // presets", "Next preset", "Save preset to file" and "Load preset from
    // file"; contract §4 deletes the native attribute — on an element that also
    // has a data-tip it renders a second, untranslated OS tooltip, and leaving
    // it on an element that has none is still an untranslated string — and
    // moves its existing English into the accessible name. Every English string
    // below is byte-identical to what v1.1.11 shipped.
    //
    // LABEL IN NAME. #savePreset and #loadPreset carry BOTH a visible caption
    // and an aria-label, and an aria-label REPLACES the accessible name rather
    // than extending it. Each of those two names therefore CONTAINS its own
    // visible caption — "Save" in "Save preset to file", "CHARGER" in "Charger
    // un préréglage depuis un fichier" — so a voice-control user saying the
    // caption still hits the button (WCAG 2.5.3, which matches
    // case-insensitively).
    //
    // ONE EXCEPTION AS OF v1.3.1, AND IT IS THE ABBREVIATION'S. The French save
    // caption is ENREG., and "ENREG." is NOT a substring of "Enregistrer un
    // préréglage dans un fichier" — the period ends it. Containment holds on the
    // STEM "Enreg" only, which is the same partial that O-Comp recorded for the
    // identical pair. No caption was invented to close the gap: the full word is
    // 66.95 px against a 48 px content box. The other repair — cutting the
    // accessible name down to "Enreg. un préréglage…" — buys the voice-control
    // user a substring by making the name worse for the screen-reader user, and
    // was not made.
    //
    // That is why aria.loadPreset's French says "Charger" and not "Ouvrir":
    // the visible French caption is CHARGER. O-DigiDelay v1.3.0 ships
    // label.load CHARGER against aria.loadPreset "Ouvrir un préréglage depuis
    // un fichier", which breaks the rule in French only — reported to the
    // orchestrator, not edited from here.
    //
    // #presetName is the one place the rule cannot be honoured, and the
    // divergence is deliberate rather than overlooked: its visible text is the
    // PRESET NAME, which changes at runtime and is exempt under D-02, so no
    // fixed accessible name can contain it. The same trade was made on
    // O-Detune, O-DigiDelay, O-FreqPulse and O-Lyrica for the identical
    // control.
    'aria.prevPreset': { en: { t: 'Previous preset' },
                         fr: { t: 'Préréglage précédent', reviewed: true } },
    'aria.nextPreset': { en: { t: 'Next preset' },
                         fr: { t: 'Préréglage suivant',   reviewed: true } },
    'aria.presetList': { en: { t: 'Click to browse presets' },
                         fr: { t: 'Cliquer pour parcourir les préréglages', reviewed: true } },
    'aria.savePreset': { en: { t: 'Save preset to file' },
                         fr: { t: 'Enregistrer un préréglage dans un fichier', reviewed: true } },
    'aria.loadPreset': { en: { t: 'Load preset from file' },
                         fr: { t: 'Charger un préréglage depuis un fichier', reviewed: true } },

    'aria.settings':   { en: { t: 'Settings' },           fr: { t: 'Réglages',              reviewed: true } },
    'aria.langSelect': { en: { t: 'Interface language' }, fr: { t: 'Langue de l’interface', reviewed: true } },
});

// ============================================================================
// I18N_EXEMPT — reasoned exclusions, never silence
//
// Every visible string the coverage scan finds must be a [data-i18n] element, a
// setLabel() call, or an entry HERE WITH A REASON. A bare skip list would let a
// missed label hide as a deliberate one.
//
// AN ENTRY IS [text, reason] OR [text, reason, scope]. An exemption is matched
// by TEXT, so an unscoped one silences EVERY node carrying that string. A scope
// is REQUIRED exactly when a string is exempt AND keyed on the same page, which
// is the one state in which the gate cannot tell a deliberate skip from a
// forgotten label (check-i18n assertion 14).
//
// NONE of the entries below is in that state — no key in LABELS above resolves
// to any of these strings — so all of them are correctly unscoped and assertion
// 14 passes without one. Checked rather than assumed: the closest call is MED,
// which is a Q option here and appears in no French caption; the band captions
// use PLAT., not MED.
// ============================================================================

export const I18N_EXEMPT = [
    // ── D-01 arm 1: byte-identical AudioParameterChoice option strings ──────
    //
    // WIDE / MED / TIGHT are the three options of BOTH lmf_q and hmf_q,
    // declared verbatim as juce::StringArray { "WIDE", "MED", "TIGHT" } at
    // PluginProcessor.cpp:56 and :69. Six nodes, three strings. The page and
    // the host automation lane must agree, so these do not translate: a French
    // user reading "MOYEN" on the face while the DAW's automation lane offers
    // "MED" is the divergence arm 1 exists to prevent.
    //
    // Unscoped is correct AND is the strictly stronger choice here: both
    // occurrences of each string are the same parameter case, and there is no
    // fourth node anywhere on the page carrying any of the three words, so
    // there is nothing for the exemption to over-silence.
    //
    // Their consequence for this stage is that `.three-way-option`'s
    // `white-space: nowrap` — one of the three the plan flags as this plugin's
    // clip risk — is NEVER REACHED BY FRENCH. See the report.
    ['WIDE',  'an lmf_q and hmf_q AudioParameterChoice option string VERBATIM '
            + '(PluginProcessor.cpp:56, :69) — D-01 arm 1: the page and the host automation '
            + 'lane must agree'],
    ['MED',   'an lmf_q and hmf_q AudioParameterChoice option string VERBATIM '
            + '(PluginProcessor.cpp:56, :69) — D-01 arm 1'],
    ['TIGHT', 'an lmf_q and hmf_q AudioParameterChoice option string VERBATIM '
            + '(PluginProcessor.cpp:56, :69) — D-01 arm 1'],

    // ── The product display name ────────────────────────────────────────────
    ['OUARICON ANALOG EQUALIZER',
     'the product display name in .title — a product name is never translated, and this is '
     + 'the brand-plus-product form of the plugin\'s registered PRODUCT_NAME "O-AnalogEQ" in '
     + 'CMakeLists.txt'],

    // ── D-02: the preset name IS the filename ───────────────────────────────
    ['Default',
     'the PRESET NAME shown in #presetName, not a caption — D-02. The name is the JSON '
     + 'filename on disk (OuariconPresetManager sanitizes it into getUserPresetsDirectory()), '
     + 'and it is written into this node at runtime by the VENDORED '
     + 'modules/preset-manager.js, which is shared across plugins: localizing it here would '
     + 'rename presets in one language and orphan the files'],

    // ── Endonyms ────────────────────────────────────────────────────────────
    ['English',  'endonym — a language name is never translated'],
    ['Français', 'endonym — a language name is never translated'],
];

// ============================================================================
// TIP_BINDINGS — [selector, key] or [selector, key, wrapper].
//
// applyI18n() runs document.querySelector(selector), then closest(wrapper) if a
// wrapper is given, and writes data-tip-title + data-tip onto whatever it lands
// on. The delegated renderer in index.html then walks closest('[data-tip]')
// from the pointer's target, so the node named here IS the hover cell.
//
// ── T17'S "BIND TO THE ids THE UI ALREADY USES" IS WRONG HERE TWICE ─────────
//
// It is now wrong on six plugins out of six, for a different reason each time,
// and the SELECTOR half and the TARGET half fail independently. On this page:
//
//   SELECTOR half — the four dual knobs have no per-band id that is usable.
//   `#lf_freq_knob` and `#lf_gain_knob` DO exist, and binding either would be
//   the naive reading of T17, but both are `pointer-events: none`
//   (index.html:229, :271): a tip bound to them can never open. The
//   addressable node is `.dual-knob-container[data-param-outer="lf_freq"]`.
//
//   TARGET half — that container is 65 x 65, and it sits inside an 85 x 85
//   `.dual-knob-wrapper` that also carries the frequency scale (`.freq-notches`,
//   itself pointer-events: none, so the wrapper receives those events). The
//   scale ring is part of the control the user is aiming at, so the wrapper
//   walk widens the hover cell from 65 x 65 to 85 x 85 — 4225 px2 to 7225 px2,
//   a 71% larger target — without moving a pixel of paint.
//
// The other nine bind BARE, and each is checked rather than assumed:
//   #lf_on / #lmf_on / #hmf_on / #hf_on  the .band-label IS the switch and IS
//                                       the 85 x 21 hover cell.
//   #lmf_q / #hmf_q                      the .three-way-toggle is 110 x 18 and
//                                       its three options are children, so
//                                       closest() reaches it from any of them.
//   #analog                              75 x 22, its own cell.
//
// ── THE CHROME BINDS BARE, AND THAT IS LOAD-BEARING ────────────────────────
//
// `#gear-btn` and `#lang-select` share the ancestor `.settings-cluster`, so a
// wrapper walk on either would resolve BOTH to the cluster — and the cluster's
// own rect is the gear's 18 x 18 box, because the popover inside it is
// absolutely positioned. Hovering the language selector would then open the
// GEAR's tip. O-Comp hit exactly this and it is the reason both rows below have
// no third element.
//
// ── FIFTEEN PARAMETERS, THIRTEEN TIPS, AND BOTH GAPS ARE FINDINGS ──────────
//
// 1. THE FOUR DUAL KNOBS CARRY TWO PARAMETERS EACH ON ONE HOVER TARGET. See
//    the I18N comment above: the two rings are pointer-events: none and the
//    control resolves them by cursor RADIUS, so there is exactly one anchor and
//    it gets one tip naming both. A second row on the same node would silently
//    overwrite the first while check-i18n reported two bound tips.
//
// 2. output_gain HAS NO CONTROL ON THE PAGE AT ALL, and that is deliberate and
//    documented in the source rather than an oversight: PluginProcessor.cpp:88
//    records it as removed in the v1.0.5 UI simplification (review item IN-01),
//    kept because it is host-automatable and because factory presets set it
//    ("Surgical Cut" = +1 dB). It is host-reachable and page-unreachable.
//    NO CONTROL WAS ADDED TO SATISFY THE COUNT — that would be a feature change
//    with a geometry cost on a 220 px frame — and no body was authored for it,
//    because an authored body with nothing to bind is an ORPHAN that check-i18n
//    assertion 2 fails.
// ============================================================================

export const TIP_BINDINGS = [
    ['.dual-knob-container[data-param-outer="lf_freq"]',  'tip.lfBand',  '.dual-knob-wrapper'],
    ['.dual-knob-container[data-param-outer="lmf_freq"]', 'tip.lmfBand', '.dual-knob-wrapper'],
    ['.dual-knob-container[data-param-outer="hmf_freq"]', 'tip.hmfBand', '.dual-knob-wrapper'],
    ['.dual-knob-container[data-param-outer="hf_freq"]',  'tip.hfBand',  '.dual-knob-wrapper'],
    ['#lf_on',      'tip.lfOn'],
    ['#lmf_on',     'tip.lmfOn'],
    ['#hmf_on',     'tip.hmfOn'],
    ['#hf_on',      'tip.hfOn'],
    ['#lmf_q',      'tip.lmfQ'],
    ['#hmf_q',      'tip.hmfQ'],
    ['#analog',     'tip.analog'],
    ['#gear-btn',   'tip.settings'],
    ['#lang-select', 'tip.language'],
];

// The tooltip lookup. Returns {t, b} — never null, never a bare key without a
// console.warn saying so, because a silently-missing tip renders as an empty
// surface that looks like a positioning bug rather than a missing entry.
//
// Live as of v1.3.0: applyI18n() calls it once per TIP_BINDINGS row, and there
// are now thirteen. It was exported verbatim while the table was empty so that
// the canon block stayed byte-identical to the other forty-two copies, and
// adding the bodies needed no change to it at all.
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
