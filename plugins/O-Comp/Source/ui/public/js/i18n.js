/*
   This file is part of O-Comp, an Ouaricon Audio plugin.
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
// i18n.js — O-Comp page labels and hover-help, English + French (v1.7.1)
//
// An ES module that EXPORTS ONLY. It must never self-execute: a bare top-level
// statement here throws out of module evaluation and takes every later
// initializer on the page with it (pattern_module_toplevel_init_tdz). This
// plugin's controller is a single inline <script type="module"> in index.html,
// so that failure mode would take the WHOLE UI rather than one panel of it.
// scripts/check-i18n.js assertion 7 enforces it.
//
// FILENAME: no hyphen. juce_add_binary_data STRIPS hyphens rather than
// converting them to underscores, so a file named i18n-fr.js would have to be
// reached as the symbol i18nfr_js (critical_binary_data_strips_hyphens). One
// combined file for both languages sidesteps the question.
//
// ── v1.7.1: FRENCH QA PASS (Stage N, 2026-08-31) ───────────────────────
//
// Every fr entry read against its en and against scripts/i18n-fr-glossary.js.
// Changed: 18 of the 43 rows the lint counts, across 16 of the 34 fr entries
// (8 terminology, 11 typography, 1 grammar/agreement, 3 meaning — a row can
// carry more than one). sameAsEn: kept 1 (label.ratio, which is the term in
// French audio too), translated 0. termNote exemptions: 0 — every glossary
// term this page needed either fit or had a listed abbreviation that fit, so
// nothing had to be exempted. Lint 39 findings → 0, --strict exit 0.
// Left as drafted: the rest. reviewed: false throughout — no native speaker
// has read this file, and this pass is a second MACHINE reading, not that one.
//
// SAUVER → ENREG., AND THE WIDTH THAT JUSTIFIED SAUVER DOES NOT SURVIVE
// RE-MEASUREMENT. Sauver is a calque and the glossary forbids it; the root
// term Enregistrer is 39.98 against this button's 30px content box, which is
// why v1.7.0 reached for Sauver. But the glossary's abbreviation ENREG. is
// 23.75 in that same .preset-action-btn — NARROWER than Sauver's 25.00 and
// than Ouvrir's 24.00 — so there was no geometry defending the calque. Every
// v1.7.0 figure was re-measured this pass with the gate's own method
// (Range.selectNodeContents on the real element, at the shipping 620x360) and
// every one reproduced to the hundredth. See the note above label.load.
//
// RELÂCHE → RELÂCH. on the caption, RELÂCHEMENT in the tip title. Same shape:
// the abbreviation is 36.80, narrower than Relâche (38.94) and than English
// Release (37.70), and the .control-group stays at exactly 52. v1.7.0 recorded
// that the only alternative to Relâche was a layout change to the row; the
// glossary's abbreviation was the alternative.
//
// GENOU → COUDE in the caption, the tip title and the tip body. Not a
// geometry choice in either direction (31.23 against 31.83, group 52 either
// way) — genou is the body part.
//
// PARAMÈTRES → RÉGLAGES on aria.settings, which was an INTERNAL contradiction
// as well as a glossary miss: the gear's tooltip title already said Réglages
// while the gear's own accessible name said Paramètres, so a speech user and
// a sighted user had two different names for one control. English said
// "Settings" on both, so this was French-only and no en string moved.
//
// THE CANVAS GR CAPTION TAKES ITS NO-BREAK SPACE AND NOTHING ELSE. Measured in
// this page's own 2D context at 11px Garamond, serif: U+00A0 is 2.75px, the
// same width as the ASCII space it replaces, and the whole string is 62.34
// either way — unchanged from the v1.7.0 figure. U+00A0 is not missing from
// that font: a not-defined probe (U+FFFF) measures 7.944 in the same context,
// so a tofu box would have been 5px wide and visible. The {v} minus and the
// space before dB belong to the READOUT, are composed by index.html, and stay
// ASCII under D-03.
//
// TYPOGRAPHY WAS APPLIED TO FRENCH STRING VALUES ONLY, and counted afterwards:
// 21 straight apostrophes → U+2019, 16 U+00A0 inserted (6 before a colon, 2
// before a semicolon, 8 between a number and its unit), 2 hyphen-minuses
// before a negative number → U+2212. No U+202F anywhere. Verified that every
// U+00A0 in the file sits on a line carrying t: or b: — none reached a key, a
// selector or a comment. The decimal comma was already correct (settled
// 2026-08-30, below) and no \d.\d survives in any French string.
//
// THREE SENTENCES CHANGED MEANING. None of them is a range, a unit or a claim
// about the DSP — every range was re-checked against createParameterLayout and
// every one was already right.
//   tip.attack   "la transitoire" → "le transitoire". A transient is
//                masculine in French signal-processing usage.
//   tip.release  "sur une matière tenue" → "sur des sons tenus", a calque of
//                "sustained material" that no French engineer writes; and
//                "les temps courts ... peuvent pomper" → "peuvent faire
//                pomper le signal", because a release time does not pump, the
//                compressor does. The English shorthand is idiomatic in
//                English and is left alone.
//   tip.langSelect "sous les boutons" → "sous les boutons rotatifs". French
//                uses one word for a push-button and a knob and this page has
//                both; the readouts are under the six knobs only.
//
// KNOWN STALE, NOT FIXED HERE: the CSS comment at index.html:203-213 still
// reasons about "Ouvrir (24.0) and Sauver (25.0)" and closes with "Ouvrir
// clears by 6.0px, Sauver by 5.0px". Its CONCLUSION is unaffected — the button
// is still floored at width: 32px with an 18 → 30 content box, and Enreg.
// clears by 6.25px — but the string it names is gone. Stage N does not touch
// CSS, so it is reported rather than edited.
//
// ── v1.7.0: HOVER-HELP ARRIVES, COPY AND RENDERER TOGETHER ──────────────────
//
// v1.6.0 shipped TIP_BINDINGS: [] and said so in as many words — that was this
// plugin's correct state, not a gap. v1.7.0 authors the nine tooltips below:
// one for each of the SEVEN APVTS parameters (all seven have a control on this
// page — the dump and the markup reconcile exactly) plus #gear-btn and
// #lang-select.
//
// THE COPY ALONE WOULD HAVE SHIPPED NINE INVISIBLE STRINGS. Canon v2's
// applyI18n() writes data-tip-title and data-tip onto the anchors named below
// and stops; the thing that paints a surface is per-plugin code, and at v1.6.0
// this page had no #tooltip element, no `.tooltip` rule and no hover handler.
// Nothing would have caught it: check-i18n assertion 2 only counts bindings,
// check-ui-labels has no tooltip awareness at all, and boot-all-uis counts
// aria-label and title and never data-tip. So index.html gains setupTooltips()
// and its CSS in the same commit, ported from O-simpleFM's delegated
// cursor-following renderer and styled in this page's own paper-and-brown-ink
// vocabulary.
//
// NUMBERS IN A BODY KEEP FRENCH SPELLING — THE DECIMAL SEPARATOR IS A COMMA.
// D-03 exempts the readout NODE, not a number written into prose, so
// "-60 to 0 dB" localizes to "-60 à 0 dB" and "0.1 to 100 ms" to
// "0,1 à 100 ms".
//
// SETTLED BY THE DEVELOPER, 2026-08-30. This file originally shipped the
// POINT, reasoning that .value-display formats "0.1 ms" and "4.0:1" with a
// point in both languages under D-03, so a tip saying "0,1" beside a readout
// saying "0.1" describes a control the page does not have. O-Chorus's executor
// went the other way in the same batch, on the grounds that every one of the 21
// already-shipped tooltip plugins writes the comma. The developer chose the
// COMMA: it is correct French, and it is the house style the rest of the suite
// already has.
//
// The readout keeps its point — that is D-03 and it does not move. The tip and
// the readout therefore spell the number differently ON PURPOSE, because the
// readout is a machine-formatted value and the body is prose. Exactly one
// string on this page was affected: tip.attack.
//
// ── I18N IS NOT EMPTY HERE, AND THE REASON IS THE CANVAS ────────────────────
//
// Three of this page's user-facing strings are painted with
// CanvasRenderingContext2D.fillText into #envelopeCanvas. A canvas string has
// no element, so it can be neither a [data-i18n] node nor a setLabel() target,
// and check-i18n cannot see it at all: assertion 10 walks TEXT NODES and
// assertion 12 scans textContent/innerText writes. Neither reaches fillText,
// so this is a localization NOBODY WOULD HAVE FAILED US FOR SKIPPING — and a
// French user would have read "Envelope" and "Gain Reduction" in English for
// it.
//
// They are housed in I18N with an EMPTY body, which is the shape O-Polystutter
// v1.14.0 established for a homeless composed string and which check-i18n
// assertion 2 names explicitly. They are read through trLabel() from inside
// the render loop, so they follow the language selector on the very next
// animation frame with no re-render hook of their own.
//
// They are NOT in LABELS, and that is deliberate rather than arbitrary:
// assertion 15 fails any LABELS key that no element and no setLabel call
// references, and a trLabel() call from a paint routine is in neither set. A
// canvas string put in LABELS would report as a DEAD key on a page that reads
// it thirty times a second.
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
// I18N — hover-help copy, plus the three CANVAS-PAINTED strings.
//
// TWO KINDS OF ENTRY LIVE HERE AND THE BODY IS WHAT TELLS THEM APART. An entry
// with a non-empty `b` is a TOOLTIP, and assertion 2 demands a TIP_BINDINGS row
// for it — an authored body nothing binds is an ORPHAN and fails. An entry with
// an EMPTY `b` is a homeless composed string (the shape O-Polystutter v1.14.0
// established), and correctly demands no binding.
//
// The nine tips come first, in the page's own left-to-right order. The three
// canvas captions keep their empty bodies at the foot: they are painted with
// fillText into #envelopeCanvas, they have no element, and they are NOT tips.
// Do not give them bodies and do not bind them.
//
// TITLE = THE CAPTION THE USER IS READING, not the automation lane's name.
// `output_gain` is named "Output Gain" in the APVTS and captioned "Output" on
// the page, so the tip is titled Output / Sortie. Same rule for the French
// terms inside the bodies: they reuse this page's own captions (Seuil, Ratio,
// Coude, Sortie, Gain auto, ARRÊT / MARCHE) so a user can match a sentence to a
// control by eye.
//
// EACH BODY ENDS WITH THE RANGE AND THE UNIT, and every unit came from the
// dump's own `label` column (.planning/params.tsv: dB, :1, ms, ms, dB, dB) —
// NOT recovered from a formatter. Confirmed independently against the page's
// own `params` table in index.html, which appends ' dB', ':1' and ' ms' to the
// same six knobs. auto_gain is an AudioParameterBool: its range is its two
// faces, not a number.
// ============================================================================

export const I18N = Object.freeze({

    // ── The seven parameters, left to right across the knob row ─────────────

    // threshold — AudioParameterFloat, -60..0 dB, step 0.1, default -20.
    // The detector is PEAK and stereo-LINKED (processBlock takes the max of the
    // channels), which is the one thing a user cannot learn by turning it.
    'tip.threshold': {
        en: { t: "Threshold",
              b: "The level the detector has to cross before compression starts. Detection is peak and stereo-linked — the louder channel decides, so both sides duck together and the image stays put. -60 to 0 dB." },
        fr: { t: "Seuil",
              b: "Le niveau que le détecteur doit franchir pour que la compression commence. La détection se fait sur la crête et en liaison stéréo : le canal le plus fort décide, les deux côtés baissent ensemble et l’image reste en place. −60 à 0 dB.",
              reviewed: false },
    },

    // ratio — AudioParameterFloat, 1..20 :1, default 2. The French TITLE is
    // byte-identical to the English and needs no sameAsEn flag: assertion 4
    // fires only when t AND b both match, and the bodies differ. It matches the
    // page caption, which LABELS already declares sameAsEn with its reasons.
    'tip.ratio': {
        en: { t: "Ratio",
              b: "How much of each decibel above the threshold survives: at 4:1 an overshoot of 4 dB leaves as 1 dB. At 1:1 nothing is compressed however far the signal goes over, and past roughly 10:1 the behaviour is limiting rather than compression. 1:1 to 20:1." },
        fr: { t: "Ratio",
              b: "La part de chaque décibel au-dessus du seuil qui subsiste : à 4:1 un dépassement de 4 dB ressort à 1 dB. À 1:1 rien n’est comprimé, quelle que soit l’ampleur du dépassement, et au-delà d’environ 10:1 le comportement devient celui d’un limiteur. 1:1 à 20:1.",
              reviewed: false },
    },

    // attack_time — AudioParameterFloat, 0.1..100 ms, default 10. The value is
    // a one-pole TIME CONSTANT (updateCoefficients: 1 - exp(-1/(t*fs/1000))),
    // so the envelope is most of the way there rather than exactly there.
    'tip.attack': {
        en: { t: "Attack",
              b: "How quickly the detector rises once the signal is over the threshold. Short times catch the transient and flatten the front of a drum; long times let the stick through and start compressing behind it. 0.1 to 100 ms." },
        fr: { t: "Attaque",
              b: "La vitesse à laquelle le détecteur monte une fois le signal au-dessus du seuil. Les temps courts saisissent le transitoire et aplatissent le début d’une frappe ; les temps longs laissent passer l’attaque et compriment derrière elle. 0,1 à 100 ms.",
              reviewed: false },
    },

    // release_time — AudioParameterFloat, 10..1000 ms, step 1, default 100.
    'tip.release': {
        en: { t: "Release",
              b: "How quickly the gain comes back once the signal drops under the threshold again. Short times sound lively and can pump audibly on sustained material; long times hold the reduction steady between hits. 10 to 1000 ms." },
        fr: { t: "Relâchement",
              b: "La vitesse à laquelle le gain revient une fois le signal repassé sous le seuil. Les temps courts sonnent vif et peuvent faire pomper le signal de façon audible sur des sons tenus ; les temps longs maintiennent la réduction stable entre les frappes. 10 à 1000 ms.",
              reviewed: false },
    },

    // knee — AudioParameterFloat, 0..20 dB, default 6. calculateGainReduction()
    // opens the soft region at threshold - knee/2, so the band is CENTRED on
    // the threshold and half of it lies below. At 0 the soft branch is skipped
    // outright (it would divide by zero) and the curve corners.
    'tip.knee': {
        en: { t: "Knee",
              b: "The width of the band around the threshold where the ratio arrives gradually instead of all at once. It is centred on the threshold, so half of it sits below and compression begins before the reading reaches the setting. At 0 the knee is hard and the transfer curve corners. 0 to 20 dB." },
        fr: { t: "Coude",
              b: "La largeur de la bande autour du seuil où le ratio s’installe progressivement au lieu d’un seul coup. Elle est centrée sur le seuil : la moitié se trouve en dessous, et la compression commence avant que la lecture n’atteigne le réglage. À 0 le coude est dur et la courbe de transfert forme un angle. 0 à 20 dB.",
              reviewed: false },
    },

    // output_gain — AudioParameterFloat, -12..+24 dB, default 0. Titled from
    // the PAGE CAPTION ("Output"), not the parameter name ("Output Gain").
    // computeMakeupGainLinear() ADDS it to the auto-gain figure, and
    // prepareToPlay smooths the sum over 20 ms.
    'tip.output': {
        en: { t: "Output",
              b: "Makeup gain applied after the compressor, to bring the level back to where it started. It is added to whatever Auto-Gain is contributing rather than replacing it, and the sum is smoothed over 20 ms so an automation move cannot zipper. -12 to +24 dB." },
        fr: { t: "Sortie",
              b: "Gain de compensation appliqué après le compresseur, pour ramener le niveau là où il était. Il s’ajoute à ce qu’apporte le Gain auto au lieu de le remplacer, et la somme est lissée sur 20 ms pour qu’un mouvement d’automation ne crépite pas. −12 à +24 dB.",
              reviewed: false },
    },

    // auto_gain — AudioParameterBool, default off. Its two faces are named in
    // the BODY in the language of the page, and that is not a D-01 arm 1
    // problem: arm 1 exempts an AudioParameterChoice OPTION, so that the page
    // and the host automation lane read the same word. There is no choice
    // parameter anywhere on this plugin — six floats and one bool — so there is
    // no option string to contradict. ARRÊT and MARCHE are the button's own
    // French faces (LABELS label.autoGainOff / label.autoGainOn), which is what
    // a user actually sees on the control this sentence describes.
    'tip.autoGain': {
        en: { t: "Auto-Gain",
              b: "Adds makeup gain worked out from the current Threshold and Ratio — half the theoretical amount, so it compensates without overshooting — and follows both as you move them. It stacks with the Output knob rather than replacing it. Two settings: OFF and ON." },
        fr: { t: "Gain auto",
              b: "Ajoute un gain de compensation calculé à partir du Seuil et du Ratio courants — la moitié de la valeur théorique, pour compenser sans dépasser — et suit les deux quand vous les déplacez. Il s’ajoute au réglage Sortie au lieu de le remplacer. Deux positions : ARRÊT et MARCHE.",
              reviewed: false },
    },

    // ── The two chrome controls ─────────────────────────────────────────────
    //
    // The gear tip is what tells a user hover-help exists at all, so its body
    // describes ONLY what this popover actually holds. O-Comp has no hover-help
    // on/off toggle — not in C++, not in localStorage — and O-Tapestop's wording
    // promises one. A tip that lies is worse than no tip.
    'tip.gearBtn': {
        en: { t: "Settings",
              b: "Opens the panel that sets the language of this interface. That is all it holds: the labels on this page and this hover help switch with it, and the choice is kept with the session, so a project reopens in the language it was saved in." },
        fr: { t: "Réglages",
              b: "Ouvre le panneau qui règle la langue de cette interface. Il ne contient rien d’autre : les libellés de cette page et cette aide au survol changent avec elle, et le choix est conservé avec la session — un projet se rouvre dans la langue où il a été enregistré.",
              reviewed: false },
    },

    // The value readouts named here are the six .value-display nodes, which
    // stay English by D-03. The two canvas captions below DO follow the
    // selector, which is why this body says "under the knobs" rather than
    // "every reading on the page".
    'tip.langSelect': {
        en: { t: "Language",
              b: "The language of the labels on this page and of this hover help. English and French are available. The value readings under the knobs and the preset names stay in English, so the page and the host agree on what a setting is called." },
        fr: { t: "Langue",
              b: "La langue des libellés de cette page et de cette aide au survol. L’anglais et le français sont disponibles. Les valeurs affichées sous les boutons rotatifs et les noms de préréglages restent en anglais, pour que la page et l’hôte s’accordent sur le nom d’un réglage.",
              reviewed: false },
    },

    // ── The three CANVAS-PAINTED strings — NOT tooltips ─────────────────────
    //
    // Empty bodies, deliberately and permanently. See the block comment above.

    // Painted at 9px into #envelopeCanvas by startRenderLoop(). The canvas is
    // 308px wide and both strings are drawn at x = 10, so length is free:
    // measured 33.99 -> 38.49 and 56.74 -> 65.48 with ctx.measureText in this
    // plugin's own context, not borrowed from another plugin.
    'canvas.envelope': {
        en: { t: 'Envelope',  b: '' },
        fr: { t: 'Enveloppe', b: '', reviewed: false },
    },
    'canvas.gainReduction': {
        en: { t: 'Gain Reduction',    b: '' },
        fr: { t: 'Réduction de gain', b: '', reviewed: false },
    },

    // The live gain-reduction readout, painted at 11px. The VALUE is composed
    // by the caller and passed as {v}; the abbreviation and the unit are the
    // only localized parts. Authored as ONE entry with a token rather than two
    // entries chosen by a ternary, per contract section 6 — the shipped code
    // used to pick between `GR: -x dB` and `GR: 0.0 dB`, and that branch is now
    // on the VALUE, where no language can reach it.
    //
    // French puts a NO-BREAK space before a colon (v1.7.1; it was an ASCII space
    // at v1.7.0). Measured 59.59 -> 62.34 at the widest reading, drawn at x = 10
    // in a 308px canvas, and U+00A0 is 2.75px in this context — byte for byte the
    // width of the space it replaces, so 62.34 is unchanged.
    'canvas.gr': {
        en: { t: 'GR: {v} dB',  b: '' },
        fr: { t: 'RG : {v} dB', b: '', reviewed: false },
    },
});

// ============================================================================
// LABELS — the visible text of the page. {en:{t}, fr:{t, reviewed}}.
// One string per entry, no body: a label is not a tooltip.
//
// ── THE THREE-ARM D-01 TEST, AND WHERE EACH ARM LANDED ON THIS PAGE ─────────
//
// arm 1 (byte-identical to an AudioParameterChoice option) exempts NOTHING
//       here. O-Comp has no AudioParameterChoice at all: six
//       AudioParameterFloat and one AudioParameterBool
//       (PluginProcessor.cpp createParameterLayout). There is no automation
//       lane anywhere on this plugin that a translated caption could contradict.
// arm 2 (a number or a unit) exempts all six .value-display readouts —
//       "-20.0 dB", "4.0:1", "10.0 ms", "100 ms", "6.0 dB", "0.0 dB" — and the
//       two nav glyphs. The extractor classifies every one of them READOUT, so
//       assertion 10 never asks about them and they need no I18N_EXEMPT row.
// arm 3 (what ELEMENT receives it) exempts #preset-name, which holds a preset
//       NAME. It is listed in I18N_EXEMPT because the D-02 filename reason is
//       the stronger one and deserves to be written down.
//
// The ONE arm-3 call that went the other way is #auto-gain-toggle. It is
// written by JS from a bool and it is keyed anyway, because that node NEVER
// holds a number: auto_gain is an AudioParameterBool, the node's only two
// states are the two words below, and setLabel()'s permanent data-i18n
// therefore cannot strand a live reading the way it would on a readout that
// alternates between a word and a value (O-Marimba's six timbre words,
// O-Detune's #wobble_rate_value).
//
// ── THE GEOMETRY BUDGET, MEASURED IN THIS PLUGIN'S OWN ELEMENTS ─────────────
//
// Each of the six knob columns is a `.control-group`: position:absolute with
// no width, so it SHRINK-WRAPS to its widest child. In English every one of
// them is exactly 52px wide — the knob face — because every English caption is
// narrower than 52. `align-items: center` then centres the knob inside that
// box, so a French caption WIDER than 52px would widen the group and slide the
// knob, the value readout and the vine arc sideways by half the excess. That
// is the page's one real cliff, and it is invisible to a clip check: nothing
// clips, nothing wraps, the knobs just move.
//
// So 52px is a hard budget, and every French caption below was measured
// against it by writing it into the actual .param-label and reading the box
// back. NOT pinned: a width pin on .control-group would hold the group's
// rectangle still while letting a longer caption overflow it, and assertion 5
// reads that overflow as a French spill its offsetParent did not have in
// English. The budget is the fix; the pin would only move the failure.
//
// Re-measured at v1.7.1 with the same method. Every v1.7.0 figure reproduced
// to the hundredth; two of the French strings changed (Stage N, above).
//
// Threshold 49.11 -> Seuil 25.13    Attack 32.33 -> Attaque 38.33
// Ratio     26.34 -> Ratio 26.34    Release 37.70 -> Relâch. 36.80
// Knee      25.22 -> Coude 31.23    Output 33.56 -> Sortie 29.28
// Auto-Gain 51.55 -> Gain auto 47.58
//
// FRENCH SHRINKS ON FOUR OF THE SEVEN — it was three at v1.7.0, and Relâch.
// is the fourth: at 36.80 the glossary abbreviation is narrower than English
// Release itself. A gate that only looked for growth would have certified this
// page and missed that Seuil takes 24px off the widest caption on the row.
//
// The two French strings that GROW still clear the budget with room: Attaque
// 38.33 and Coude 31.23 against 52. Every .control-group was read back at 52px
// after the change, and check-ui-labels assertion 7 reports 0 non-label
// elements moved between the languages — the same 0 it reported before.
// ============================================================================

export const LABELS = Object.freeze({

    // ── The preset bar ──────────────────────────────────────────────────────
    //
    // Both buttons are `width: 32px` with 1px borders, so the content box is
    // 30px and the button cannot grow — it is a flex item whose min-width floor
    // is its min-content, and every candidate below is under 30px, so the
    // .preset-bar row keeps its 164px and the whole header stays put.
    //
    // OUVRIR / ENREG. — the Stage K measurements below all reproduced this pass
    // to the hundredth, but the CONCLUSION changed on the Save button. Measured
    // in this plugin's own .preset-action-btn against its 30px content box:
    // Charger 28.83 (1.17px of margin, and not taken), Enregistrer 39.98 (does
    // not fit), Ouvrir 24.00, Sauver 25.00 — and ENREG. 23.75, NARROWER than
    // both of the strings v1.7.0 chose for width. So Sauver, which the glossary
    // lists as a calque, had no geometry defending it after all.
    //
    // Ouvrir stays: the glossary accepts it for Load where the button opens a
    // file dialog, and #preset-load calls loadPresetFromFile() — a native open
    // dialog (modules/preset-manager.js). #preset-save is savePresetWithDialog().
    'label.load': { en: { t: 'Load' }, fr: { t: 'Ouvrir', reviewed: false } },
    'label.save': { en: { t: 'Save' }, fr: { t: 'Enreg.', reviewed: false } },

    // ── The six knob captions ───────────────────────────────────────────────
    'label.threshold': { en: { t: 'Threshold' }, fr: { t: 'Seuil',   reviewed: false } },

    // "Ratio" is the term in French audio software as well as English. Keyed
    // with sameAsEn rather than exempted: an exemption is matched by TEXT and
    // says nothing about whether anybody looked, while sameAsEn says this was
    // looked at and translates to itself. "Taux" measured 23.84 and would also
    // have fit — the choice is terminology, not geometry.
    'label.ratio': { en: { t: 'Ratio' }, fr: { t: 'Ratio', reviewed: false, sameAsEn: true } },

    'label.attack': { en: { t: 'Attack' }, fr: { t: 'Attaque', reviewed: false } },

    // "Relâch.", the glossary's ABBREVIATION — and the v1.7.0 note that the only
    // alternative to Relâche was a layout change was wrong. Re-measured in this
    // plugin's .param-label: Relâchement 62.92 (widens the .control-group to
    // 62.92 and slides the knob 5.46px, confirmed by reading the group box back),
    // Relâche 38.94, English Release 37.70 — and Relâch. 36.80, NARROWER than all
    // three, with the group staying at exactly 52. The register is recovered
    // where there is no budget: the tip TITLE is the full "Relâchement".
    //
    // A caption that is the parameter name with letters missing is not a caption
    // that DISAGREES with the parameter name, so the tip carries the full form
    // rather than the truncation (Stage M2 carried trap 9).
    'label.release': { en: { t: 'Release' }, fr: { t: 'Relâch.', reviewed: false } },

    // "Coude". The v1.7.0 claim that Genou is the standard French rendering of a
    // compressor knee does not survive the glossary, which lists genou as the
    // body part and coude as the term. Coude 31.23 against Genou 31.83 — the
    // .control-group stays 52 either way, so this was never a geometry choice.
    'label.knee':   { en: { t: 'Knee' },   fr: { t: 'Coude',  reviewed: false } },
    'label.output': { en: { t: 'Output' }, fr: { t: 'Sortie', reviewed: false } },

    // ── The auto-gain toggle ────────────────────────────────────────────────
    //
    // The caption above the button, then the two faces written INTO it.
    // "Gain automatique" measured 86.52 and would have widened that column from
    // 70px (the toggle) to 86.52 and re-centred the toggle; "Gain auto" is
    // 47.58 and sits well inside.
    'label.autoGain': { en: { t: 'Auto-Gain' }, fr: { t: 'Gain auto', reviewed: false } },

    // MARCHE / ARRÊT, measured 45.56 and 35.02 against the button's 66px
    // content box (70px minus two 2px borders). DÉSACTIVÉ measured 58.36 and
    // would also have fitted, but the pair has to be legible at 10px bold in a
    // 40px-tall button and ARRÊT/MARCHE is the shorter, higher-contrast pair.
    //
    // Written through setLabel() from an if/else, never a ternary inside the
    // call: check-i18n assertion 13 rejects a conditional in a setLabel
    // argument, because contract section 6 authors around an inflection rather
    // than engineering one.
    'label.autoGainOn':  { en: { t: 'ON' },  fr: { t: 'MARCHE', reviewed: false } },
    'label.autoGainOff': { en: { t: 'OFF' }, fr: { t: 'ARRÊT',  reviewed: false } },

    // ── The visualisation caption ───────────────────────────────────────────
    //
    // .viz-label is absolutely positioned, bottom-centred with
    // translateX(-50%), inside a 210px panel. It grows SYMMETRICALLY about the
    // panel's centre line, so it pushes nothing: measured 77.42 -> 94.98 box
    // width, still 115px inside the panel it sits in.
    'label.transferCurve': { en: { t: 'Transfer Curve' }, fr: { t: 'Courbe de transfert', reviewed: false } },

    // ── The preset dropdown, built at runtime ───────────────────────────────
    //
    // Both are written by buildDropdownMenu() into elements it creates, so both
    // go through setLabel() and become [data-i18n] elements from that moment.
    //
    // "Aucun préréglage" is SINGULAR on purpose. French inflects zero as
    // singular and English does not, and contract section 6 declines to build a
    // plural engine for one string: the copy is authored so it reads correctly
    // at zero in both languages instead.
    'label.presets':   { en: { t: 'Presets' },    fr: { t: 'Préréglages',      reviewed: false } },
    'label.noPresets': { en: { t: 'No presets' }, fr: { t: 'Aucun préréglage', reviewed: false } },

    // ── The settings popover ────────────────────────────────────────────────
    'label.language': { en: { t: 'Language' }, fr: { t: 'Langue', reviewed: false } },

    // ── Accessible names ────────────────────────────────────────────────────
    //
    // Every one of these is the text of a native title= that contract section 4
    // DELETES. Nothing here is invented: each string is the one that was on the
    // element at v1.5.0, translated.
    //
    // LABEL-IN-NAME (WCAG 2.5.3). The accessible name of a control with a
    // visible caption must CONTAIN that caption, so a speech user can say what
    // they read. English: "Load" inside "Load preset", "Save" inside "Save
    // preset". French load still holds exactly: "Ouvrir" inside "Ouvrir un
    // préréglage". Choosing CHARGER for the button and leaving "Ouvrir un
    // préréglage" on the name is exactly the defect found on O-DigiDelay in
    // batch K2, and it is only visible when the two strings are read together.
    //
    // FRENCH SAVE NOW HOLDS BY STEM, NOT BYTE-EXACTLY, AND THAT IS FLAGGED.
    // The caption is the glossary's abbreviation "Enreg." and the name is the
    // glossary's only accepted rendering of "Save preset", "Enregistrer le
    // préréglage": the name contains "Enreg" but not the caption's terminal
    // period. Closing that gap needs either a caption the 30px box cannot hold
    // (Enregistrer, 39.98) or an invented third form, which the glossary
    // forbids. Recorded for the developer rather than worked around.
    //
    // The "un"/"le" asymmetry is deliberate and is the glossary's: Save acts on
    // the preset that is loaded, Load picks an arbitrary one.
    'aria.prevPreset':    { en: { t: 'Previous preset' },        fr: { t: 'Préréglage précédent',              reviewed: false } },
    'aria.nextPreset':    { en: { t: 'Next preset' },            fr: { t: 'Préréglage suivant',                reviewed: false } },
    'aria.browsePresets': { en: { t: 'Click to browse presets' }, fr: { t: 'Cliquer pour parcourir les préréglages', reviewed: false } },
    'aria.loadPreset':    { en: { t: 'Load preset' },            fr: { t: 'Ouvrir un préréglage',              reviewed: false } },
    'aria.savePreset':    { en: { t: 'Save preset' },            fr: { t: 'Enregistrer le préréglage',         reviewed: false } },
    'aria.settings':      { en: { t: 'Settings' },               fr: { t: 'Réglages',                          reviewed: false } },
    'aria.langSelect':    { en: { t: 'Interface language' },     fr: { t: 'Langue de l’interface',             reviewed: false } },
});

// ============================================================================
// I18N_EXEMPT — reasoned exclusions, never silence
//
// Every visible string the coverage scan finds must be a [data-i18n] element, a
// setLabel() call, or an entry HERE WITH A REASON. A bare skip list would let a
// missed label hide as a deliberate one.
//
// AN EXEMPTION IS MATCHED BY TEXT, so an unscoped entry silences EVERY node on
// the page carrying that string. The third field is a scope — a comma-separated
// list of `tag`, `.class` or `#id` matched against the node's parent and its
// ancestors — and it is REQUIRED exactly where a string is both exempt and
// keyed on the same page, which is the one state in which the gate cannot tell
// a deliberate skip from a label somebody forgot (assertion 14).
//
// NONE of the four below is in that state. No key in LABELS or I18N resolves to
// "Ouaricon Compressor", "Default", "English" or "Français" in either language,
// so all four are correctly unscoped and assertion 14 passes without one. That
// was checked against the table, not assumed.
// ============================================================================

export const I18N_EXEMPT = [

    // ── The product display name ────────────────────────────────────────────
    ['Ouaricon Compressor',
     'the product display name in div.title, and the same string in the document title element '
     + '— a product name is never translated, and this is the brand-plus-product form of the '
     + 'plugin\'s registered PRODUCT_NAME "O-Comp" in CMakeLists.txt'],

    // ── D-02: the preset name IS the filename ───────────────────────────────
    ['Default',
     'the PRESET NAME shown in #preset-name, not a caption — D-02. The name is the JSON '
     + 'filename on disk (OuariconPresetManager sanitizes it into getUserPresetsDirectory()), '
     + 'and it is written into this node at runtime by the VENDORED '
     + 'modules/preset-manager.js, which is a shared copy: localizing it would rename presets '
     + 'in one language and orphan the files'],

    // ── Endonyms ────────────────────────────────────────────────────────────
    //
    // The extractor classifies both as ENDONYM rather than LABEL, so assertion
    // 10 would skip them regardless. Listed anyway, because the next person to
    // read this file should find the rule written down rather than have to
    // rediscover that a classifier happens to cover it.
    ['English',  'endonym — a language name is never translated'],
    ['Français', 'endonym — a language name is never translated'],
];

// ============================================================================
// TIP_BINDINGS — [selector, key, wrapper]
//
// applyI18n() runs document.querySelector(selector), walks closest(wrapper)
// when a wrapper is given, and writes data-tip-title + data-tip onto whatever
// that lands on. The renderer in index.html then reads them via
// closest('[data-tip]') from whatever the pointer is over.
//
// THE WRAPPER IS THE POINT. Each of the six knobs has an id, so T17's "bind to
// the ids the UI already uses" is true here for the SELECTOR — but #x-knob is
// only the 52px vine face, and a tip that opens on the face alone is a tip that
// closes every time the pointer drifts onto the caption or the reading two
// pixels below it. The wrapper walks up to .control-group, the absolutely
// positioned column that holds knob + caption + readout, so the whole cell is
// the hover area. The auto-gain toggle is bound the same way for the same
// reason: #auto-gain-toggle is the 70x40 button, .control-group also carries
// its "Auto-Gain" caption.
//
// The two chrome anchors are bound BARE. #gear-btn must NOT walk up to
// .settings-cluster: that wrapper also contains .settings-popover, so hovering
// anywhere in the open panel — including over #lang-select and its own tip —
// would resolve to the gear's. They are separate anchors with separate tips and
// the DOM nesting is exactly what keeps them apart.
// ============================================================================

export const TIP_BINDINGS = [
    ['#threshold-knob',    'tip.threshold', '.control-group'],
    ['#ratio-knob',        'tip.ratio',     '.control-group'],
    ['#attack-knob',       'tip.attack',    '.control-group'],
    ['#release-knob',      'tip.release',   '.control-group'],
    ['#knee-knob',         'tip.knee',      '.control-group'],
    ['#output-knob',       'tip.output',    '.control-group'],
    ['#auto-gain-toggle',  'tip.autoGain',  '.control-group'],

    ['#gear-btn',          'tip.gearBtn'],
    ['#lang-select',       'tip.langSelect'],
];

// The tooltip lookup. Returns {t, b} — never null, never a bare key without a
// console.warn saying so, because a silently-missing tip renders as an empty
// surface that looks like a positioning bug rather than a missing entry.
//
// LIVE as of v1.7.0: applyI18n() calls it once per TIP_BINDINGS row, so every
// hover on this page reads its result. It was exported verbatim while the list
// was empty precisely so that adding the bodies above needed no change to this
// file's shape or to the canon block.
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
