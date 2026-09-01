/*
   This file is part of O-Wind, an Ouaricon Audio plugin.
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
// i18n.js — O-Wind visible-text table and hover-help copy, EN + FR (v1.18.1)
//
// ── v1.18.1: FRENCH QA PASS (Stage N, 2026-08-31) ───────────────────────────
// Every fr entry read against its en and against scripts/i18n-fr-glossary.js.
// Changed: 59 of 119 entries (22 terminology, 26 typography, 2 grammar,
// 7 meaning, 2 register). sameAsEn: kept 14, translated 0, ADDED 1
// (label.fx.mix — the glossary root for "Mix" IS "Mix", so applying it makes
// the entry a straight copy and check-i18n assertion 4 hard-FAILS until the
// flag declares it; N2 correction 15). termNote exemptions: 2 (both listed
// below, both on the Flatterzunge pair). Left as drafted: the other 60.
// reviewed: false throughout — no native speaker has read any of it.
// Lint 65 -> 0, `--strict` exit 0 (13 G1, 13 F1, 9 T3, 3 T4, 12 T5, 15 T7
// closed). 47 U+00A0 inserted (0 -> 47), every one inside a `t:`/`b:` string
// value of an `fr:` object: the pass was a brace-matched scope over the fr
// objects with whole-line comments masked out first, and the control that both
// `en` sub-objects, all 119 keys, I18N_EXEMPT and TIP_BINDINGS are
// byte-identical was run by IMPORTING both revisions, not by reading a diff a
// no-break space is invisible in.
//
// DECISIONS THE NEXT READER NEEDS:
//
//  1. THE 72 px .knob-label CAP WAS RE-MEASURED, and v1.17.0's header was RIGHT
//     both times it defended a string on width. "Colonne d'air" is 77.61 px
//     against the 72.00 px cap (the header guessed ~80) and "Colonne" stays;
//     "Enregistrer" is 65.73 px against .preset-save-btn's 44.00 px CONTENT box
//     — the button is shrink-to-fit with no max-width, so it would grow 27.73 px
//     and take that width off the flex:1 #preset-name beside it, exactly as the
//     header says. "Enreg." (38.00 px) stays. Nine of twenty-four headers in
//     this stage have been wrong about the string they defended; this one is
//     not, and it was measured rather than inherited.
//  2. FOUR GLOSSARY ROOTS DO NOT FIT AND SHIP AS THE LISTED ABBREVIATION, all
//     against the same 72.00 px cap: Relâchement 74.86 -> Relâch. 42.72;
//     Profondeur 75.52 -> Prof. 32.38; Réinjection 74.13 -> Réinj. 36.55;
//     Amortissement 94.84 -> Amort. 42.70. A fifth, "Maintien inf." 73.56, ships
//     as "Maint. inf." 58.69. No third form was invented anywhere: every one of
//     these is a rendering scripts/i18n-fr-glossary.js already lists.
//  3. FOUR CAPTIONS WERE A THIRD FORM and are now the listed one — Prof. dér.
//     -> Prof. dérive (68.81 px, 3.19 px of the cap), Profond. -> Prof.,
//     Réinject. -> Réinj., Amortis. -> Amort. The 3.19 px on Prof. dérive is the
//     tightest margin this file ships and .knob-label carries
//     text-overflow: ellipsis, so a wider Windows/WebView2 font metric would
//     truncate it silently rather than overflow visibly.
//  4. TWO MATCHED PAIRS MOVED TOGETHER, because applying the glossary to only
//     the half that fits puts one concept on two vocabularies across two
//     adjacent captions: Prof. dér./Vit. dér. -> Prof. dérive/Vit. dérive
//     (59.50 px), and Frullato/Vit. frul. -> Flatt./Vit. flatt. (55.59 px).
//  5. THE FOUR BYPASS BODIES NAMED A BUTTON FACE THAT DOES NOT EXIST IN FRENCH.
//     See the note above tip.chorusBypass. This is N4 correction 34 in reverse:
//     the parameter-choice faces on this page ARE English in both languages and
//     are named as such, but label.fx.on is LOCALIZED and the drafts named it
//     "On" anyway.
//  6. "Tenue" -> "Maintien" HERE, where O-Bowed keeps Tenue. O-Bowed's
//     infinite-sustain exemption rests on that page having no ADSR at all; this
//     page has one, captioned Maintien, so Tenue inf. beside it was one English
//     word wearing two French ones inside a 900 x 600 frame.
//  7. THE EFFECTS TAB IS NEVER MEASURED BY check-ui-labels ON THIS PLUGIN, and
//     four of the twelve moved captions live there. tests/i18n-states.json
//     drives the settings popover FIRST and #settings-popover renders at
//     698,39 190x40, covering the right 190 px of the 300 px Effects tab button
//     at 600,40 300x35 — so a centre-point click lands on SPAN.settings-label
//     inside the popover and the EFFECTS state never fires. 25 of 65
//     [data-i18n] elements have never been geometry-measured by that gate. The
//     four were measured on the shipping node instead (a scratchpad probe
//     replicating assertions 4, 5, 7 and 8 on the open tab): 0 non-label
//     elements moved, no caption clipped in either language, and all four
//     SHRANK — 56.31->32.38, 44.09->22.02, 57.14->36.55, 53.56->42.70 px.
//     Reported, not fixed: the states file is not a Stage N surface.
//
// v1.18.0 (Stage M) ADDS THE 52 HOVER-HELP ENTRIES AND THEIR BINDINGS. v1.17.0
// shipped 67 LABELS with I18N and TIP_BINDINGS both empty, because v1.16.3 had
// no data-tip anywhere and authoring the copy was deferred. The LABELS block,
// the I18N_EXEMPT block and every geometry note below are UNCHANGED from
// v1.17.0 — nothing in this release re-opens a Stage K decision.
//
// AND IT ADDS A RENDERER, WHICH IS NOT WHAT THE PLAN SAID THIS STAGE WAS.
// applyI18n() writes data-tip-title and data-tip ATTRIBUTES onto the anchors
// and stops. The thing that reads those attributes and paints a surface is
// per-plugin code outside the canon, and at v1.17.0 O-Wind had none: no
// #tooltip element, no .tooltip rule, no hover handler. Authoring 52 bodies
// and binding them with no other change would have shipped 52 INVISIBLE
// strings behind three green gates — check-i18n counts bindings, check-ui-
// labels has no tooltip awareness at all, and boot-all-uis counts aria-label
// and title and never data-tip. setupTooltips() in index.html is the other
// half, and tests/ui_tip_render_check.js is the only thing in this repo that
// can see it work.
//
// An ES module that EXPORTS ONLY. A bare top-level statement here throws out of
// module evaluation and takes every later initializer on the page with it
// (pattern_module_toplevel_init_tdz). check-i18n assertion 7 enforces it.
//
// FILENAME: no hyphen. juce_add_binary_data STRIPS hyphens rather than
// converting them to underscores, so a second file named i18n-fr.js would have
// to be reached as the symbol i18nfr_js (critical_binary_data_strips_hyphens).
// One combined file for both languages sidesteps the question entirely.
//
// COPY IS textContent ON EVERY PATH — never innerHTML. check-i18n assertion 9
// rejects any innerHTML reference here and any string literal containing `<`.
//
// THE ENGLISH WAS MOVED, NOT REWRITTEN. Every `en` entry below is byte-for-byte
// what index.html carried through v1.16.3, extracted mechanically rather than
// re-typed. The authored English also STAYS in the markup as the fallback that
// renders if applyI18n never runs (contract section 1).
//
// ALL FRENCH IS MACHINE-DRAFTED AND FLAGGED `reviewed: false`. No native
// speaker has read it. `node scripts/check-i18n.js` prints the worklist.
//
// ── SIX PARAMETERS HAVE NO CONTROL ON THIS PAGE (v1.18.0) ───────────────────
//
// .planning/params.tsv dumps 56 parameters from a runtime walk of the
// constructed processor. FIFTY have a control here; six do not, and that is a
// FINDING rather than a gap. No control was added to satisfy a count.
//
//   attackChiff     AudioParameterFloat 0..1, read at FluteSynthVoice.cpp:289
//   humanize        AudioParameterFloat 0..1, read at FluteSynthVoice.cpp:290
//   vibratoOnset    AudioParameterFloat 0..1000 ms, FluteSynthVoice.cpp:283
//   inharmonicity   AudioParameterFloat 0..1, FluteSynthVoice.cpp:293
//     — four live DSP parameters with no relay, no native function and zero
//       occurrences of the id anywhere in the served root. Automatable and
//       host-reachable; not reachable from this page at all.
//
//   referencePitch  AudioParameterFloat 400..480 Hz. NOT vestigial and NOT
//     absent from the UI — it is reachable, under a different name, through a
//     native-function ALIAS. The shared tuning panel's master-tune knob
//     (#ref-pitch-indicator / #ref-pitch-value, tuning-panel.js:940-1000)
//     calls getMasterTune / setMasterTune, and PluginEditor.cpp:337-355 routes
//     both straight onto this parameter. The id therefore appears NOWHERE in
//     the served root or in the panel, which is why a static scan reports it
//     unreachable. It gets no tip because that panel is lazy-`import()`ed on
//     the first Tuning-tab click (index.html:2232) and is absent from the DOM
//     when applyI18n() runs — O-Reed's referencePitch trap, in a second shape —
//     and because the panel is the SHARED module, out of a per-plugin commit's
//     scope. The panel was not force-mounted to satisfy the count.
//
//   tuningSystem    AudioParameterChoice { Scala/TUN, MTS-ESP, 12-TET }. A
//     SIXTH, not named in the batch measurement, and it is a genuine one: zero
//     occurrences in the served root, zero in the shared panel, and the shared
//     panel has no tuning-mode selector at all (its only mode switch is the
//     visualisation mode, tuning-panel.js:385). The parameter is a MIRROR —
//     PluginProcessor.cpp:529-530 writes it from setStateInformation so the
//     APVTS choice follows the engine's own mode. A host can automate a tuning
//     system the page cannot show.
//
//     This also CORRECTS a sentence in the v1.17.0 header and in I18N_EXEMPT
//     below, which said the three tuningSystem option strings "appear only
//     inside the Tuning tab". They appear nowhere on the page at all. The
//     exemption stands on its other leg — every caption inside the Tuning tab
//     belongs to the shared module — and is left as written rather than edited
//     inside a hover-help commit.
//
// ── ONE DEAD PARAMETER, AND ITS TIP SAYS SO ─────────────────────────────────
//
// toneHoleToggle has a control, moves, and does nothing audible.
// PluginProcessor.cpp:316-319 records that the tone-hole scattering DSP was
// never implemented and that its scaffolding was removed in v1.16.2. A scan of
// Source/ confirms it: the id appears in the layout, the relay and the
// attachment, and in no DSP file. `tip.toneHoleToggle` says that outright. A
// tip that lies is worse than no tip.
//
// ── WHAT THE EXTRACTOR COULD NOT SEE, AND IS THE REAL FINDING HERE ──────────
//
// `node scripts/i18n-extract.js --plugin O-Wind` reports 61 LABEL, 7 READOUT,
// 3 UNSURE, 3 attributes and 4 JS-prose strings. Every one of those numbers is
// right, and the JS-prose one is right about a set that is far too small.
//
// THE EFFECTS TAB IS BUILT ENTIRELY FROM SCRIPT. Sixteen knob captions are
// passed as ordinary function arguments —
//
//     populateFxKnobs('reverb-knobs', [{ id: 'reverbSize', label: 'Size' }, ...])
//     delayRow.appendChild(makeFxKnob('delayTime', 'Time'))
//
// — and makeFxKnob() interpolates them into an innerHTML template. The
// extractor's js-prose scan looks for a PROSE LITERAL on the right-hand side of
// a `.textContent =` / `.innerHTML =` assignment; an English word sitting in an
// object literal one frame away from the write is neither. Assertion 12 is
// blind to it for exactly the same reason. That is the O-Tremolo shape the
// batch addendum names, at sixteen strings rather than one, and leaving them
// alone would have shipped an English Effects tab inside a French plugin with
// every gate GREEN.
//
// The fix is not a cleverer scan — it is that every one of the sixteen now
// carries a LITERAL key at its own call site, so assertions 13 and 15 can both
// read it:
//
//     setLabel(addFxKnob('reverb-knobs', 'reverbSize'), 'label.fx.size');
//
// A key computed from `k.key` inside populateFxKnobs() would have failed
// assertion 13 (a computed setLabel key cannot be checked) and would have been
// invisible to assertion 15's dead-key sweep. Sixteen literal call sites are
// verbose and they are the only shape both gates can see.
//
// SIXTEEN NATIVE title= ATTRIBUTES WERE ALSO WRITTEN FROM SCRIPT. setupFxKnob()
// did `valueDisplay.title = 'Double-click to edit'` on every FX readout. The
// page rendered NINETEEN native titles (boot-all-uis, v1.16.3) against the
// THREE the markup declares. Assertion 11 reads index.html only, so it counted
// three. Contract section 4 deletes a native title= rather than localizing it,
// and where the title is an element's only help its text moves to
// data-i18n-aria with NO NEW PROSE INVENTED — so the sixteen are now
// `valueDisplay.dataset.i18nAria = 'aria.fxValueEdit'`, carrying the same
// eighteen characters into an accessible name the language sweep owns.
//
// ── THE D-01 TEST ON THIS PLUGIN ────────────────────────────────────────────
//
// ARM 1 — O-Wind has exactly TWO AudioParameterChoice parameters:
//   tuningSystem  { 'Scala/TUN', 'MTS-ESP', '12-TET' }   PluginProcessor.cpp:347
//   delayMode     { 'Normal', 'PingPong' }               PluginProcessor.cpp:377
// The three tuningSystem options appear only inside the Tuning tab, which is
// the shared module and out of scope (see I18N_EXEMPT). The two delayMode
// options ARE page copy — initializeEffects() writes them into the delay-mode
// <select> — and they are byte-identical to the option strings, so the page and
// the host automation lane must name them identically. Both are exempt on arm 1
// and neither is keyed anywhere on this page, so neither needs a scope.
//
// NOT AN ARM-1 CASE, and worth saying because it looks like one:
// `instrumentPreset` is an AudioParameterInt (0..7), NOT a Choice. Its eight
// <option> captions are exempt on a different rule — see I18N_EXEMPT.
//
// ARM 2 / ARM 3 — the seven `.knob-value` READOUT rows and the sixteen FX value
// readouts. formatValue() and setupFxKnob()'s updateVisual() overwrite every one
// of them on the first valueChangedEvent. They are exempt on arm 2 (a number
// and its unit are language-neutral, D-03) AND on arm 3 (a readout node is never
// a [data-i18n] element whatever parameter type is behind it — keying one would
// make the element enter and leave the sweep as the knob turns). They are not
// listed individually in I18N_EXEMPT: the coverage scan already classes them
// non-LABEL, and twenty-three entries whose text changes on the first mouse drag
// would be twenty-three entries that never match anything again.
//
// ── THE NINE NON-LITERAL textContent WRITES, WALKED TO THEIR CALL SITES ─────
//
// The batch addendum measures nine `textContent`/`innerText` assignments with a
// non-literal right-hand side on this page. Every one was walked:
//
//   1310, 1315, 1319  `.knob-value = formatValue(param, state.getScaledValue())`
//                     a number and a unit. Arm 2/3. EXEMPT.
//   1511              `item.textContent = name` — a preset-dropdown row. The
//                     name comes from getPresetList() and IS the JSON filename
//                     (OuariconPresetManager.h). D-02. EXEMPT.
//   1696              `valueDisplay.textContent = formatter(realValue) + suffix`
//                     a number and a unit. Arm 2/3. EXEMPT.
//   1743              `valueDisplay.textContent = ''` — clearing the node before
//                     an <input> goes in. No string at all.
//   1788              `btn.textContent = bypassed ? 'Off' : 'On'` — TWO ENGLISH
//                     LITERALS one frame away. Now two literal setLabel calls,
//                     not a ternary: contract section 6, assertion 13.
//   1823              `opt.textContent = name` over ['Normal', 'PingPong'] —
//                     delayMode option strings VERBATIM. Arm 1. EXEMPT.
//   1654              `container.innerHTML = ...${label}...` inside makeFxKnob —
//                     THE SIXTEEN. See above.
//
// Two of the nine were carrying English into a French UI. Both are fixed here.
// ============================================================================

export const LANGUAGES = ['en', 'fr'];

// ============================================================================
// I18N — hover-help copy. {t, b}: a title and a body.
//
// FIFTY-TWO entries as of v1.18.0: 50 parameter tips and 2 chrome tips. The
// count is 50 rather than 56 because six of this plugin's parameters have no
// control on this page at all — see the header for the measurement.
//
// TITLES ARE THE PARAMETER'S FULL NAME, not the knob caption. This page's
// captions are TRUNCATIONS — "Vib Pitch", "Jet Refl.", "Sub Harm.", "Pre-dly",
// "Damp", "Flut Rate" — because `.knob-label` is a 72 px box with
// text-overflow: ellipsis and English already measures 70.73 px in it
// ("Embouchure"). A caption that is the same name with letters missing is not
// a caption DISAGREEING with the parameter; a 260 px tooltip is exactly where
// "Reverb Pre-delay" belongs, and it is also the automation-lane name. The
// rule "the caption wins" is kept for the two places the caption says
// something genuinely different: the tone-hole switch is captioned "Tone
// Holes" (plural) and the ADSR switch has no caption of its own, so its title
// is the section legend "ADSR Envelope" the user actually reads beside it.
//
// RANGES COME FROM THE PAGE'S OWN FORMATTER, never invented. Only 16 of the 56
// parameters carry a `label` in .planning/params.tsv (28%). The other 40 are
// phrased from `PARAMS` (index.html:1750-1777) and `formatValue()`
// (index.html:1870-1877) for the Sound tab, and from the `setupFxKnob()` call
// sites (index.html:2540-2555) for the Effects tab, where the display factor
// and the suffix are passed as arguments.
//
// TWO PLACES THE DUMP AND THE PAGE DISAGREE, and the PAGE wins because the user
// is reading the page:
//   delayTime    dumps `s` over 0.001..2.000; setupFxKnob passes displayFactor
//                1000 and the suffix ' ms' (index.html:2543), so the readout
//                says `375 ms`. The body says milliseconds and says why.
//   adsrAttack / adsrDecay / adsrRelease
//                dump `s`; formatValue() switches to milliseconds below one
//                second (index.html:1874-1875), so one knob shows both units.
//
// FRENCH BODIES ARE PROSE AND TAKE FRENCH CONVENTION — decimal COMMA, a space
// before %, U+2212 for the minus. The READOUT keeps its point, because D-03
// exempts the readout NODE and that has not moved. They differ on purpose: the
// readout is a machine-formatted value, the body is a sentence.
//
// ALL FRENCH IS MACHINE-DRAFTED, `reviewed: false` on every entry.
// ============================================================================

export const I18N = Object.freeze({

    // ── Excitation ──────────────────────────────────────────────────────────
    'tip.breathPressure': {
        en: { t: 'Breath Pressure',
              b: 'How hard the player blows across the embouchure hole. Low values give a soft, breathy tone that barely speaks; high values push the jet into a loud, harmonically rich regime. Range 0.00 to 1.00.' },
        fr: { t: 'Pression du souffle',
              b: 'Force avec laquelle le souffle passe sur le biseau. Les valeurs basses donnent un son doux et soufflé qui parle à peine ; les valeurs hautes poussent le jet vers un régime fort et riche en harmoniques. Plage 0,00 à 1,00.',
              reviewed: false },
    },
    'tip.embouchure': {
        en: { t: 'Embouchure',
              b: 'Shapes the air jet against the edge of the embouchure hole — the ratio of jet width to bore. Lower values darken and steady the tone; higher values brighten it and make the octave break easier. Range 0.00 to 1.00.' },
        fr: { t: 'Embouchure',
              b: 'Règle la forme du jet d’air sur le biseau — le rapport entre la largeur du jet et la perce. Les valeurs basses assombrissent et stabilisent le son ; les hautes l’éclaircissent et facilitent le passage à l’octave. Plage 0,00 à 1,00.',
              reviewed: false },
    },
    'tip.breathNoise': {
        en: { t: 'Breath Noise',
              b: 'Turbulence mixed into the air jet. A little keeps the tone alive; too much buries the pitch under wind. Range 0.00 to 1.00.' },
        fr: { t: 'Bruit de souffle',
              b: 'Turbulence mêlée au jet d’air. Un peu garde le son vivant ; trop enterre la hauteur sous le vent. Plage 0,00 à 1,00.',
              reviewed: false },
    },

    // ── Resonator ───────────────────────────────────────────────────────────
    'tip.material': {
        en: { t: 'Material',
              b: 'A timbral macro across the bore material: 0.00 is dark wood or bamboo, 1.00 is bright metal. Reach for it first when an instrument sounds right but the wrong colour. Range 0.00 to 1.00.' },
        fr: { t: 'Matériau',
              b: 'Macro de timbre sur la matière de la perce : 0,00 pour un bois ou un bambou sombre, 1,00 pour un métal brillant. À utiliser en premier quand un instrument sonne juste mais dans la mauvaise couleur. Plage 0,00 à 1,00.',
              reviewed: false },
    },
    'tip.toneColor': {
        en: { t: 'Tone Color',
              b: 'Tilts the spectrum, trading weight in the fundamental against harmonic brightness. It works after Material rather than instead of it. Range 0.00 to 1.00.' },
        fr: { t: 'Timbre',
              b: 'Incline le spectre, échangeant le poids du fondamental contre la brillance harmonique. Il agit après Matériau plutôt qu’à sa place. Plage 0,00 à 1,00.',
              reviewed: false },
    },
    'tip.airColumn': {
        en: { t: 'Air Column',
              b: 'Scales the effective bore length, which sets where the resonances sit against the played pitch. Low values feel short and piercing like a piccolo; high values long and hollow. Range 0.00 to 1.00.' },
        fr: { t: 'Colonne d’air',
              b: 'Met à l’échelle la longueur utile de la perce, qui fixe la position des résonances par rapport à la note jouée. Les valeurs basses donnent un tube court et perçant façon piccolo ; les hautes, un tube long et creux. Plage 0,00 à 1,00.',
              reviewed: false },
    },
    'tip.jetReflection': {
        en: { t: 'Jet Reflection',
              b: 'Reflection coefficient at the junction between jet and bore. Negative values invert the returning wave and thin the tone; positive values reinforce it. Range −1.00 to 1.00.' },
        fr: { t: 'Réflexion du jet',
              b: 'Coefficient de réflexion à la jonction entre le jet et la perce. Les valeurs négatives inversent l’onde de retour et amincissent le son ; les positives le renforcent. Plage −1,00 à 1,00.',
              reviewed: false },
    },
    'tip.endReflection': {
        en: { t: 'End Reflection',
              b: 'Reflection coefficient at the open end of the bore. It sets how much energy returns instead of radiating away, so low values leak and high values ring. Range −1.00 to 1.00.' },
        fr: { t: 'Réflexion de l’extrémité',
              b: 'Coefficient de réflexion à l’extrémité ouverte de la perce. Il fixe l’énergie qui revient au lieu de rayonner : les valeurs basses fuient, les hautes font sonner. Plage −1,00 à 1,00.',
              reviewed: false },
    },

    // ── ADSR envelope ───────────────────────────────────────────────────────
    // The switch carries no caption of its own — the legend beside it reads
    // "ADSR Envelope", and that is what a user reads. The automation lane names
    // the parameter "ADSR Enabled"; the body says so rather than leaving the
    // two names to be discovered.
    'tip.adsrEnabled': {
        en: { t: 'ADSR Envelope',
              b: 'Switches the amplitude envelope on. With it off a note follows breath pressure alone, which is how a wind instrument normally behaves, and the four knobs beside it do nothing. The automation lane names it ADSR Enabled; it reads Off or On.' },
        fr: { t: 'Enveloppe ADSR',
              b: 'Active l’enveloppe d’amplitude. Désactivée, une note ne suit que la pression du souffle, ce qui est le comportement normal d’un instrument à vent, et les quatre potentiomètres voisins n’ont aucun effet. La ligne d’automation la nomme ADSR Enabled ; elle affiche Off ou On.',
              reviewed: false },
    },
    'tip.adsrAttack': {
        en: { t: 'ADSR Attack',
              b: 'Time the envelope takes to reach full level after a note starts. Only active while the ADSR envelope is switched on. Range 1 ms to 5 s, shown in milliseconds below one second.' },
        fr: { t: 'Attaque ADSR',
              b: 'Temps que met l’enveloppe pour atteindre son niveau maximal après le début d’une note. Actif seulement lorsque l’enveloppe ADSR est activée. Plage 1 ms à 5 s, affichée en millisecondes sous une seconde.',
              reviewed: false },
    },
    'tip.adsrDecay': {
        en: { t: 'ADSR Decay',
              b: 'Time to fall from the attack peak down to the sustain level. Only active while the ADSR envelope is switched on. Range 1 ms to 5 s, shown in milliseconds below one second.' },
        fr: { t: 'Déclin ADSR',
              b: 'Temps de descente du sommet de l’attaque jusqu’au niveau de maintien. Actif seulement lorsque l’enveloppe ADSR est activée. Plage 1 ms à 5 s, affichée en millisecondes sous une seconde.',
              reviewed: false },
    },
    'tip.adsrSustain': {
        en: { t: 'ADSR Sustain',
              b: 'Level the note holds once the decay has finished, as a fraction of the attack peak. Only active while the ADSR envelope is switched on. Range 0.00 to 1.00.' },
        fr: { t: 'Maintien ADSR',
              b: 'Niveau auquel la note se tient une fois le déclin terminé, en fraction du sommet de l’attaque. Actif seulement lorsque l’enveloppe ADSR est activée. Plage 0,00 à 1,00.',
              reviewed: false },
    },
    'tip.adsrRelease': {
        en: { t: 'ADSR Release',
              b: 'Time the note takes to fade out after the key is let go. Only active while the ADSR envelope is switched on. Range 1 ms to 10 s, shown in milliseconds below one second.' },
        fr: { t: 'Relâchement ADSR',
              b: 'Temps que met la note à s’éteindre après le relâchement de la touche. Actif seulement lorsque l’enveloppe ADSR est activée. Plage 1 ms à 10 s, affichée en millisecondes sous une seconde.',
              reviewed: false },
    },

    // ── Expression ──────────────────────────────────────────────────────────
    'tip.vibratoRate': {
        en: { t: 'Vibrato Rate',
              b: 'Speed of the pitch vibrato. Around 5 Hz is the orchestral norm; slower reads as a swell, faster as a nervous shake. Range 2.0 to 8.0 Hz.' },
        fr: { t: 'Vitesse du vibrato',
              b: 'Vitesse du vibrato de hauteur. Environ 5 Hz est la norme orchestrale ; plus lent donne une houle, plus rapide un tremblement nerveux. Plage 2,0 à 8,0 Hz.',
              reviewed: false },
    },
    'tip.vibratoDepth': {
        en: { t: 'Vibrato Pitch',
              b: 'How far the vibrato bends the pitch. At 0.00 the vibrato is silent no matter what Vibrato Rate is doing. Range 0.00 to 1.00.' },
        fr: { t: 'Hauteur du vibrato',
              b: 'Amplitude de la déviation de hauteur du vibrato. À 0,00 le vibrato est muet quoi que fasse la vitesse du vibrato. Plage 0,00 à 1,00.',
              reviewed: false },
    },
    'tip.vibratoTremolo': {
        en: { t: 'Vibrato Tremolo',
              b: 'Adds amplitude modulation locked to the vibrato’s own phase, so the note breathes in level as well as in pitch. Range 0.00 to 1.00.' },
        fr: { t: 'Trémolo du vibrato',
              b: 'Ajoute une modulation d’amplitude verrouillée sur la phase du vibrato, de sorte que la note respire en niveau autant qu’en hauteur. Plage 0,00 à 1,00.',
              reviewed: false },
    },
    'tip.vibratoDriftDepth': {
        en: { t: 'Vibrato Drift Depth',
              b: 'How far the vibrato’s rate and depth wander on their own. A small amount stops a machine-steady vibrato from sounding synthetic. Range 0.00 to 1.00.' },
        fr: { t: 'Profondeur de la dérive du vibrato',
              b: 'Amplitude de l’errance spontanée de la vitesse et de la profondeur du vibrato. Un peu suffit pour qu’un vibrato d’une régularité mécanique cesse de sonner synthétique. Plage 0,00 à 1,00.',
              reviewed: false },
    },
    'tip.vibratoDriftSpeed': {
        en: { t: 'Vibrato Drift Speed',
              b: 'How fast that wandering evolves. It does nothing while Vibrato Drift Depth sits at zero. Range 0.10 to 2.00 Hz.' },
        fr: { t: 'Vitesse de la dérive du vibrato',
              b: 'Vitesse d’évolution de cette errance. Sans effet tant que la profondeur de la dérive reste à zéro. Plage 0,10 à 2,00 Hz.',
              reviewed: false },
    },
    'tip.flutterTongue': {
        en: { t: 'Flutter Tongue',
              b: 'Depth of the flutter-tongue amplitude modulation — the rolled-r a player makes with the tongue while blowing. Range 0.00 to 1.00.' },
        fr: { t: 'Flatterzunge',
              b: 'Profondeur de la modulation d’amplitude du flatterzunge — le r roulé que le joueur produit avec la langue en soufflant. Plage 0,00 à 1,00.',
              reviewed: false },
    },
    'tip.flutterRate': {
        en: { t: 'Flutter Rate',
              b: 'Speed of the flutter-tongue modulation. It does nothing while Flutter Tongue sits at zero. Range 15.0 to 30.0 Hz.' },
        fr: { t: 'Vitesse du Flatterzunge',
              b: 'Vitesse de la modulation du flatterzunge. Sans effet tant que le flatterzunge reste à zéro. Plage 15,0 à 30,0 Hz.',
              reviewed: false },
    },
    'tip.growl': {
        en: { t: 'Growl',
              b: 'A second oscillator modulating the bore feedback, standing in for the vocal-fold coupling a player gets by humming while blowing. It roughens the tone rather than sweetening it. Range 0.00 to 1.00.' },
        fr: { t: 'Growl',
              b: 'Un second oscillateur qui module la réinjection de la perce, à la place du couplage des cordes vocales qu’un joueur obtient en fredonnant tout en soufflant. Il rend le son plus rugueux au lieu de l’adoucir. Plage 0,00 à 1,00.',
              reviewed: false },
    },

    // ── Output ──────────────────────────────────────────────────────────────
    'tip.width': {
        en: { t: 'Width',
              b: 'Stereo spread of the output. 0.00 is mono, 1.00 is the natural width and 2.00 pushes past it. Range 0.00 to 2.00.' },
        fr: { t: 'Largeur',
              b: 'Étalement stéréo de la sortie. 0,00 donne du mono, 1,00 la largeur naturelle, et 2,00 va au-delà. Plage 0,00 à 2,00.',
              reviewed: false },
    },
    'tip.formant': {
        en: { t: 'Formant',
              b: 'Prominence of the headjoint formant resonance — a fixed peak that colours every note the same way. It adds up to 6 dB of gain at the peak. Range 0.00 to 1.00.' },
        fr: { t: 'Formant',
              b: 'Importance de la résonance de formant de la tête — un pic fixe qui colore toutes les notes de la même façon. Il ajoute jusqu’à 6 dB de gain au sommet. Plage 0,00 à 1,00.',
              reviewed: false },
    },
    'tip.outputLevel': {
        en: { t: 'Output Level',
              b: 'Master gain on the way out, applied after the effects chain. Range −60.0 to +12.0 dB.' },
        fr: { t: 'Niveau de sortie',
              b: 'Gain général en sortie, appliqué après la chaîne d’effets. Plage −60,0 à +12,0 dB.',
              reviewed: false },
    },

    // ── Impossible physics ──────────────────────────────────────────────────
    'tip.infiniteSustain': {
        en: { t: 'Infinite Sustain',
              b: 'Removes damping from the bore so the note keeps ringing after the breath stops. At 1.00 it barely decays at all. Range 0.00 to 1.00.' },
        fr: { t: 'Maintien infini',
              b: 'Retire l’amortissement de la perce pour que la note continue de sonner une fois le souffle arrêté. À 1,00 elle ne décroît presque plus. Plage 0,00 à 1,00.',
              reviewed: false },
    },
    'tip.reversedJet': {
        en: { t: 'Reversed Jet',
              b: 'Inverts the direction of the jet delay, which no real flute can do. It hollows the tone and moves where the octave breaks. Range 0.00 to 1.00.' },
        fr: { t: 'Jet inversé',
              b: 'Inverse le sens du retard du jet, ce qu’aucune flûte réelle ne peut faire. Cela creuse le son et déplace le point de passage à l’octave. Plage 0,00 à 1,00.',
              reviewed: false },
    },
    'tip.subHarmonics': {
        en: { t: 'Sub-Harmonics',
              b: 'Adds sub-octave content through nonlinear feedback in the bore. Small amounts thicken the low register; large amounts growl. Range 0.00 to 1.00.' },
        fr: { t: 'Sous-harmoniques',
              b: 'Ajoute du contenu à l’octave inférieure par réinjection non linéaire dans la perce. En petite dose cela épaissit le grave ; en grande dose cela gronde. Plage 0,00 à 1,00.',
              reviewed: false },
    },

    // ── Instrument strip ────────────────────────────────────────────────────
    //
    // A TIP THAT LIES IS WORSE THAN NO TIP. toneHoleToggle is a DEAD parameter:
    // PluginProcessor.cpp:316-319 records that the tone-hole scattering DSP was
    // never implemented and that its scaffolding was removed in v1.16.2, and a
    // scan of Source/ confirms it — the id appears only in the layout, the
    // relay and the attachment. No DSP reads it, no factory preset sets it.
    // The switch moves, the automation lane moves, and nothing is heard. The
    // body says that rather than describing a feature that does not exist.
    'tip.toneHoleToggle': {
        en: { t: 'Tone Holes',
              b: 'This switch currently does nothing you can hear. The tone-hole scattering was never implemented and its scaffolding was removed in v1.16.2; the parameter is kept registered only so that existing sessions and automation stay valid. It reads Off or On.' },
        fr: { t: 'Trous de jeu',
              b: 'Cet interrupteur n’a pour l’instant aucun effet audible. La diffusion par les trous de jeu n’a jamais été implémentée et son échafaudage a été retiré en v1.16.2 ; le paramètre reste déclaré uniquement pour que les sessions et les automations existantes restent valides. Il affiche Off ou On.',
              reviewed: false },
    },
    'tip.instrumentPreset': {
        en: { t: 'Instrument Preset',
              b: 'Picks one of eight bore and jet configurations, from Concert Flute to Ocarina. It rewrites the physical model rather than the knob positions, so the change is heard at once and nothing on this page moves. Eight choices, numbered 0 to 7 in the automation lane.' },
        fr: { t: 'Préréglage d’instrument',
              b: 'Choisit une des huit configurations de perce et de jet, de Concert Flute à Ocarina. Il réécrit le modèle physique et non la position des potentiomètres : le changement s’entend aussitôt et rien ne bouge sur cette page. Huit choix, numérotés de 0 à 7 dans la ligne d’automation.',
              reviewed: false },
    },

    // ── Effects: chorus ─────────────────────────────────────────────────────
    //
    // THE FOUR BYPASS BUTTONS ARE INVERTED AGAINST THEIR PARAMETER AND THE BODY
    // SAYS SO. setupFxBypassToggle() reads the parameter as `bypassed`, so the
    // face says "On" when the parameter is Off. Describing the button without
    // naming the inversion would put a false sentence in front of anyone
    // reading the automation lane at the same time.
    //
    // v1.18.1: THE FOUR FRENCH BODIES NAMED A FACE THAT DOES NOT EXIST IN
    // FRENCH. Each button carries data-i18n="label.fx.on" (index.html:1357,
    // :1366, :1375, :1384), so under French it reads MARCHE — the drafts said
    // "Le bouton affiche On", which is true only in English. The four now say
    // "affiche Marche"; the PARAMETER's two values stay Off / On, because the
    // host automation lane is English in both languages (N4 correction 34).
    'tip.chorusBypass': {
        en: { t: 'Chorus Bypass',
              b: 'Takes the chorus in and out of the signal path. The button reads On while the effect is running, which is the automation parameter Chorus Bypass sitting at Off — the two are inverted on purpose. Off or On.' },
        fr: { t: 'Contournement du chorus',
              b: 'Insère ou retire le chorus du trajet du signal. Le bouton affiche Marche quand l’effet fonctionne, ce qui correspond au paramètre d’automation Chorus Bypass placé sur Off — les deux sont inversés à dessein. Off ou On.',
              reviewed: false },
    },
    'tip.chorusRate': {
        en: { t: 'Chorus Rate',
              b: 'Speed of the chorus oscillator. Slow settings widen and drift; fast settings shimmer. Range 0.10 to 10.00 Hz.' },
        fr: { t: 'Vitesse du chorus',
              b: 'Vitesse de l’oscillateur du chorus. Les réglages lents élargissent et font dériver ; les rapides font miroiter. Plage 0,10 à 10,00 Hz.',
              reviewed: false },
    },
    'tip.chorusDepth': {
        en: { t: 'Chorus Depth',
              b: 'How far the chorus oscillator sweeps its delay line. Range 0 to 100 %.' },
        fr: { t: 'Profondeur du chorus',
              b: 'Amplitude du balayage de la ligne à retard par l’oscillateur du chorus. Plage 0 à 100 %.',
              reviewed: false },
    },
    'tip.chorusMix': {
        en: { t: 'Chorus Mix',
              b: 'Balance between the dry signal and the chorused one. It ships at 0 %, so the chorus is inaudible until this is raised. Range 0 to 100 %.' },
        fr: { t: 'Mix du chorus',
              b: 'Équilibre entre le signal direct et le signal traité par le chorus. Il est livré à 0 %, donc le chorus reste inaudible tant que ce réglage n’est pas monté. Plage 0 à 100 %.',
              reviewed: false },
    },

    // ── Effects: delay ──────────────────────────────────────────────────────
    'tip.delayBypass': {
        en: { t: 'Delay Bypass',
              b: 'Takes the delay in and out of the signal path. The button reads On while the effect is running, which is the automation parameter Delay Bypass sitting at Off — the two are inverted on purpose. Off or On.' },
        fr: { t: 'Contournement du délai',
              b: 'Insère ou retire le délai du trajet du signal. Le bouton affiche Marche quand l’effet fonctionne, ce qui correspond au paramètre d’automation Delay Bypass placé sur Off — les deux sont inversés à dessein. Off ou On.',
              reviewed: false },
    },
    'tip.delayTime': {
        en: { t: 'Delay Time',
              b: 'Time between repeats. The readout is in milliseconds even though the host automation lane reports the same parameter in seconds. Range 1 to 2000 ms.' },
        fr: { t: 'Durée du délai',
              b: 'Temps entre les répétitions. L’affichage est en millisecondes alors que la ligne d’automation de l’hôte donne le même paramètre en secondes. Plage 1 à 2000 ms.',
              reviewed: false },
    },
    'tip.delayFeedback': {
        en: { t: 'Delay Feedback',
              b: 'How much of each repeat is fed back into the line. It stops short of unity so the delay cannot run away. Range 0 to 95 %.' },
        fr: { t: 'Réinjection du délai',
              b: 'Proportion de chaque répétition renvoyée dans la ligne. Elle s’arrête avant l’unité pour que le délai ne s’emballe pas. Plage 0 à 95 %.',
              reviewed: false },
    },
    // The two option words stay English on the page — they are the delayMode
    // AudioParameterChoice options verbatim, so the page and the host lane must
    // name them identically (D-01 arm 1, and both carry an I18N_EXEMPT entry).
    // Naming them inside a FRENCH sentence is a different thing: the option in
    // the selector is exempt, the sentence describing it is prose and is
    // localized. The words themselves are quoted unchanged in both languages.
    'tip.delayMode': {
        en: { t: 'Delay Mode',
              b: 'Normal repeats on both channels together; PingPong alternates them left and right through cross-feedback. Both option words stay English because the host automation lane names them that way. Normal or PingPong.' },
        fr: { t: 'Mode de délai',
              b: 'Normal répète sur les deux canaux ensemble ; PingPong les fait alterner à gauche et à droite par réinjection croisée. Les deux mots restent en anglais parce que la ligne d’automation de l’hôte les nomme ainsi. Normal ou PingPong.',
              reviewed: false },
    },
    'tip.delayMix': {
        en: { t: 'Delay Mix',
              b: 'Balance between the dry signal and the delayed one. It ships at 0 %, so the delay is inaudible until this is raised. Range 0 to 100 %.' },
        fr: { t: 'Mix du délai',
              b: 'Équilibre entre le signal direct et le signal retardé. Il est livré à 0 %, donc le délai reste inaudible tant que ce réglage n’est pas monté. Plage 0 à 100 %.',
              reviewed: false },
    },

    // ── Effects: EQ ─────────────────────────────────────────────────────────
    'tip.eqBypass': {
        en: { t: 'EQ Bypass',
              b: 'Takes the equaliser in and out of the signal path. The button reads On while the effect is running, which is the automation parameter EQ Bypass sitting at Off — the two are inverted on purpose. Off or On.' },
        fr: { t: 'Contournement de l’EQ',
              b: 'Insère ou retire l’égaliseur du trajet du signal. Le bouton affiche Marche quand l’effet fonctionne, ce qui correspond au paramètre d’automation EQ Bypass placé sur Off — les deux sont inversés à dessein. Off ou On.',
              reviewed: false },
    },
    'tip.eqLowGain': {
        en: { t: 'EQ Low Gain',
              b: 'Shelving cut or boost on the low band. Range −12.0 to +12.0 dB.' },
        fr: { t: 'Gain des graves',
              b: 'Atténuation ou accentuation en plateau sur la bande grave. Plage −12,0 à +12,0 dB.',
              reviewed: false },
    },
    'tip.eqMidGain': {
        en: { t: 'EQ Mid Gain',
              b: 'Peaking cut or boost on the mid band, centred wherever EQ Mid Freq is set. Range −12.0 to +12.0 dB.' },
        fr: { t: 'Gain des médiums',
              b: 'Atténuation ou accentuation en cloche sur la bande médium, centrée là où est réglée la fréquence des médiums. Plage −12,0 à +12,0 dB.',
              reviewed: false },
    },
    'tip.eqMidFreq': {
        en: { t: 'EQ Mid Freq',
              b: 'Centre frequency of the mid band. It changes nothing while EQ Mid Gain sits at 0 dB. Range 200 to 8000 Hz.' },
        fr: { t: 'Fréquence des médiums',
              b: 'Fréquence centrale de la bande médium. Sans effet tant que le gain des médiums reste à 0 dB. Plage 200 à 8000 Hz.',
              reviewed: false },
    },
    'tip.eqHighGain': {
        en: { t: 'EQ High Gain',
              b: 'Shelving cut or boost on the high band. Range −12.0 to +12.0 dB.' },
        fr: { t: 'Gain des aigus',
              b: 'Atténuation ou accentuation en plateau sur la bande aiguë. Plage −12,0 à +12,0 dB.',
              reviewed: false },
    },

    // ── Effects: reverb ─────────────────────────────────────────────────────
    'tip.reverbBypass': {
        en: { t: 'Reverb Bypass',
              b: 'Takes the reverb in and out of the signal path. The button reads On while the effect is running, which is the automation parameter Reverb Bypass sitting at Off — the two are inverted on purpose. Off or On.' },
        fr: { t: 'Contournement de la réverbération',
              b: 'Insère ou retire la réverbération du trajet du signal. Le bouton affiche Marche quand l’effet fonctionne, ce qui correspond au paramètre d’automation Reverb Bypass placé sur Off — les deux sont inversés à dessein. Off ou On.',
              reviewed: false },
    },
    'tip.reverbSize': {
        en: { t: 'Reverb Size',
              b: 'Size of the simulated room, which sets how long the tail runs. Range 0 to 100 %.' },
        fr: { t: 'Taille de la réverbération',
              b: 'Taille de la salle simulée, qui fixe la longueur de la traîne. Plage 0 à 100 %.',
              reviewed: false },
    },
    'tip.reverbDamp': {
        en: { t: 'Reverb Damping',
              b: 'How fast the high frequencies die away inside the tail. More damping reads as soft furnishings; less as bare stone. Range 0 to 100 %.' },
        fr: { t: 'Amortissement de la réverbération',
              b: 'Vitesse à laquelle les aigus s’éteignent dans la traîne. Beaucoup d’amortissement évoque une salle meublée ; peu, la pierre nue. Plage 0 à 100 %.',
              reviewed: false },
    },
    'tip.reverbPredelay': {
        en: { t: 'Reverb Pre-delay',
              b: 'Gap between the dry note and the first reflections. A little keeps the attack clear of the tail. Range 0 to 200 ms.' },
        fr: { t: 'Pré-délai de la réverbération',
              b: 'Écart entre la note directe et les premières réflexions. Un peu suffit pour dégager l’attaque de la traîne. Plage 0 à 200 ms.',
              reviewed: false },
    },
    'tip.reverbMod': {
        en: { t: 'Reverb Mod',
              b: 'Modulates the reverb’s delay lines so the tail moves instead of ringing on one pitch. Range 0 to 100 %.' },
        fr: { t: 'Modulation de la réverbération',
              b: 'Module les lignes à retard de la réverbération pour que la traîne bouge au lieu de sonner sur une seule hauteur. Plage 0 à 100 %.',
              reviewed: false },
    },
    'tip.reverbShimmer': {
        en: { t: 'Reverb Shimmer',
              b: 'Feeds an octave-up copy of the tail back into the reverb, so the sound rises as it decays. Range 0 to 100 %.' },
        fr: { t: 'Shimmer de la réverbération',
              b: 'Réinjecte dans la réverbération une copie de la traîne transposée à l’octave supérieure, si bien que le son monte en décroissant. Plage 0 à 100 %.',
              reviewed: false },
    },
    'tip.reverbMix': {
        en: { t: 'Reverb Mix',
              b: 'Balance between the dry signal and the reverberated one. It ships at 0 %, so the reverb is inaudible until this is raised. Range 0 to 100 %.' },
        fr: { t: 'Mix de la réverbération',
              b: 'Équilibre entre le signal direct et le signal réverbéré. Il est livré à 0 %, donc la réverbération reste inaudible tant que ce réglage n’est pas monté. Plage 0 à 100 %.',
              reviewed: false },
    },

    // ── Chrome ──────────────────────────────────────────────────────────────
    //
    // The gear tip is what tells a user hover-help exists at all, so it must
    // describe ONLY what this popover contains. O-Wind has no hover-help
    // on/off toggle, and the panel opens BELOW the button
    // (.settings-popover is `top: calc(100% + 8px)`), not above it — copying
    // another plugin's wording would have shipped two false sentences.
    'tip.gear': {
        en: { t: 'Settings',
              b: 'Opens the settings panel just below this button. It holds the interface language, and nothing else.' },
        fr: { t: 'Réglages',
              b: 'Ouvre le panneau de réglages juste sous ce bouton. Il ne contient que la langue de l’interface.',
              reviewed: false },
    },
    // The third sentence is a scope statement, not a boast. O-Wind consumes the
    // tuning panel from the SHARED module by reference (CMakeLists.txt:97-98),
    // so localizing it is a cross-plugin change and one of this plugin's three
    // tabs stays English for a French user. Saying so here is cheaper than
    // letting it be discovered.
    'tip.langSelect': {
        en: { t: 'Interface language',
              b: 'Switches every caption and every hover-help sentence between English and French. Number readouts keep their English format, and the Tuning tab stays English in both languages.' },
        fr: { t: 'Langue de l’interface',
              b: 'Bascule chaque légende et chaque phrase d’aide au survol entre l’anglais et le français. Les valeurs numériques gardent leur format anglais, et l’onglet Accord reste en anglais dans les deux langues.',
              reviewed: false },
    },
});

// ============================================================================
// LABELS — the visible text of the page. {en:{t}, fr:{t, reviewed}}.
// One string per entry, no body: a label is not a tooltip.
//
// ── THE GEOMETRY CLIFF ON THIS PAGE, MEASURED ───────────────────────────────
//
// `.knob-label` is `font-size: 9px; text-transform: uppercase;
// letter-spacing: 0.5px; max-width: 72px; white-space: nowrap; overflow:
// hidden; text-overflow: ellipsis`. An over-long caption renders an ELLIPSIS
// rather than overflowing, which is the half a spill check cannot see, and
// `#tab-effects .knob-label` inherits the same cap — it overrides only colour,
// weight and font-size.
//
// Every number below was read from the RENDERED element in this plugin's own
// 900 x 600 frame. text-transform and letter-spacing are not in
// getComputedStyle().font, and K2 proved two plugins give absolutes 8 px apart
// for the same two words at the same declared size, so no width here was
// borrowed from another plugin's page.
//
// ENGLISH IS ALREADY WITHIN 1.3 PX OF THE CAP: "Embouchure" measures 70.73 in
// the 72 px box. It is the same word in French, so it neither clips nor moves —
// but it is why no French caption on this page was drafted longer than ten
// characters, and why several are abbreviated where the English is not.
// ============================================================================

export const LABELS = Object.freeze({

    // ── Preset bar ──────────────────────────────────────────────────────────
    // "Enregistrer" is 11 characters against "Save"'s 4 and would grow
    // .preset-save-btn by ~44 px, shrinking the flex:1 #preset-name beside it.
    // "Enreg." is the abbreviation a transport bar uses, and it is what keeps
    // the bar's geometry language-invariant without pinning a width on a button
    // whose neighbour is the elastic one.
    'label.presetSave': { en: { t: 'Save' }, fr: { t: 'Enreg.', reviewed: false } },

    'aria.presetPrev': {
        en: { t: 'Previous Preset' },
        fr: { t: 'Préréglage précédent', reviewed: false },
    },
    'aria.presetNext': {
        en: { t: 'Next Preset' },
        fr: { t: 'Préréglage suivant', reviewed: false },
    },
    // MOVED, NOT AUTHORED. This is the v1.16.3 native title= on
    // #preset-name, verbatim. Contract section 4 deletes a native title rather
    // than localizing it, and where the title is an element's only help its text
    // becomes the accessible name — no new prose is invented here.
    //
    // #preset-name is deliberately NOT a [data-i18n] element. It holds
    // #preset-dropdown as a CHILD, and applyLabel() writes textContent, which
    // would delete the entire dropdown on the first language sweep.
    'aria.presetName': {
        en: { t: 'Click to browse presets' },
        fr: { t: 'Cliquer pour parcourir les préréglages', reviewed: false },
    },

    // ── Tab bar ─────────────────────────────────────────────────────────────
    // `.tab-btn` is `flex: 1` in a 900 px bar, so each is exactly 300 px wide
    // and centred whatever it says. Nothing here can push anything.
    'label.tab.sound':   { en: { t: 'Sound' },   fr: { t: 'Son',     reviewed: false } },
    'label.tab.tuning':  { en: { t: 'Tuning' },  fr: { t: 'Accord',  reviewed: false } },
    'label.tab.effects': { en: { t: 'Effects' }, fr: { t: 'Effets',  reviewed: false } },

    // ── Settings popover (v1.17.0) ──────────────────────────────────────────
    'label.language':  { en: { t: 'Language' }, fr: { t: 'Langue', reviewed: false } },
    'aria.settings':   { en: { t: 'Settings' }, fr: { t: 'Réglages', reviewed: false } },
    'aria.langSelect': {
        en: { t: 'Interface language' },
        fr: { t: 'Langue de l’interface', reviewed: false },
    },

    // ── Instrument strip ────────────────────────────────────────────────────
    'label.instrument': { en: { t: 'Instrument' }, fr: { t: 'Instrument', reviewed: false, sameAsEn: true } },
    // The flute term is "trous de jeu". At 11 px uppercase that is ~90 px
    // against "Tone Holes"' 75.42, and .toggle-control is a flex item in the
    // instrument strip, so the widening would push the preset selector beside
    // it. .toggle-label is pinned to a box that holds BOTH — see index.html.
    'label.toneHoles':  { en: { t: 'Tone Holes' }, fr: { t: 'Trous de jeu', reviewed: false } },
    'label.preset':     { en: { t: 'Preset' },     fr: { t: 'Préréglage',   reviewed: false } },

    // ── Excitation ──────────────────────────────────────────────────────────
    'label.excitation': { en: { t: 'Excitation' }, fr: { t: 'Excitation', reviewed: false, sameAsEn: true } },
    'label.breath':     { en: { t: 'Breath' },     fr: { t: 'Souffle',    reviewed: false } },
    // Identical in French. sameAsEn is an ASSERTION, not a shrug: it is what
    // stops assertion 4 reading an identical string as an untranslated one.
    'label.embouchure': { en: { t: 'Embouchure' }, fr: { t: 'Embouchure', reviewed: false, sameAsEn: true } },
    'label.noise':      { en: { t: 'Noise' },      fr: { t: 'Bruit',      reviewed: false } },

    // ── Resonator ───────────────────────────────────────────────────────────
    'label.resonator': { en: { t: 'Resonator' },  fr: { t: 'Résonateur', reviewed: false } },
    'label.material':  { en: { t: 'Material' },   fr: { t: 'Matériau',   reviewed: false } },
    'label.toneColor': { en: { t: 'Tone Color' }, fr: { t: 'Timbre',     reviewed: false } },
    // "Colonne d'air" is 13 characters — ~80 px in the 72 px box, an ellipsis.
    // "Colonne" is unambiguous under a knob in the RESONATOR section.
    'label.airColumn': { en: { t: 'Air Column' }, fr: { t: 'Colonne',    reviewed: false } },
    'label.jetRefl':   { en: { t: 'Jet Refl.' },  fr: { t: 'Réfl. jet',  reviewed: false } },
        // "bout" is colloquial for a bore's open end and the tip body already says
    // "l'extrémité ouverte de la perce" — the caption and its own body were
    // naming one thing two ways. 59.70 px against the 72.00 px cap.
    'label.endRefl':   { en: { t: 'End Refl.' },  fr: { t: 'Réfl. extr.', reviewed: false } },

    // ── ADSR envelope ───────────────────────────────────────────────────────
    // The caption was SPLIT out of `.section-label` into its own <span>
    // (contract section 5): the div also holds the #adsr-toggle as an element
    // child, and applyLabel() writes textContent, which would have deleted the
    // toggle on the first language sweep.
    'label.adsrEnvelope': { en: { t: 'ADSR Envelope' }, fr: { t: 'Enveloppe ADSR', reviewed: false } },
    'label.attack':       { en: { t: 'Attack' },  fr: { t: 'Attaque',  reviewed: false } },
    'label.decay':        { en: { t: 'Decay' },   fr: { t: 'Déclin',   reviewed: false } },
    'label.sustain':      { en: { t: 'Sustain' }, fr: { t: 'Maintien', reviewed: false } },
    'label.release':      { en: { t: 'Release' }, fr: { t: 'Relâch.',  reviewed: false } },

    // ── Expression ──────────────────────────────────────────────────────────
    'label.expression':  { en: { t: 'Expression' },  fr: { t: 'Expression', reviewed: false, sameAsEn: true } },
    'label.vibRate':     { en: { t: 'Vib Rate' },    fr: { t: 'Vit. vibr.', reviewed: false } },
    'label.vibPitch':    { en: { t: 'Vib Pitch' },   fr: { t: 'Haut. vibr.', reviewed: false } },
    // "Trémolo vibr." would be 13 characters in a 72 px box. The knob sits
    // between Vib Pitch and Drift Depth in the EXPRESSION section, so the
    // vibrato is already named by its neighbours.
    'label.vibTremolo':  { en: { t: 'Vib Tremolo' }, fr: { t: 'Trémolo',    reviewed: false } },
    'label.driftDepth':  { en: { t: 'Drift Depth' }, fr: { t: 'Prof. dérive', reviewed: false } },
    'label.driftSpeed':  { en: { t: 'Drift Speed' }, fr: { t: 'Vit. dérive', reviewed: false } },
    // MEANING BEFORE WIDTH (v1.18.1). The parameter is flutterTongue — the wind
    // technique — so the glossary row that applies is `flutter tongue ->
    // Flatterzunge`, not `flutter -> Scintillement`, which names tape
    // wow-and-flutter and would be wrong on a flute at any width. Flatterzunge
    // itself measures 77.73 px against the 72.00 px .knob-label cap and
    // TRUNCATES to an ellipsis; "Flatt." is the abbreviation French scores
    // print and is what a French wind player reads. The tip title spells it out.
    'label.flutter':     { en: { t: 'Flutter' },     fr: { t: 'Flatt.',     reviewed: false,
        termNote: 'MEANING, then width. flutterTongue is the wind technique, so the row is `flutter tongue -> Flatterzunge`, not `flutter -> Scintillement` (tape wow-and-flutter — wrong on a flute). Flatterzunge measures 77.73 px against the 72.00 px .knob-label cap and truncates to an ellipsis; Flatt. is the abbreviation French orchestral scores print, and tip.flutterTongue spells Flatterzunge out in full' } },
    // The matched half. "Vit. frul." would leave the same technique named two
    // ways on two adjacent captions (55.59 px, 16.41 px of the cap).
    'label.flutRate':    { en: { t: 'Flut Rate' },   fr: { t: 'Vit. flatt.', reviewed: false,
        termNote: 'the matched half of label.flutter — same meaning exemption (flutterTongue, not tape flutter), same abbreviation. "Vit. frul." would leave one technique named two ways on two adjacent captions. 55.59 px against the 72.00 px .knob-label cap' } },
    // A loanword in French jazz vocabulary, spelled identically.
    'label.growl':       { en: { t: 'Growl' },       fr: { t: 'Growl',      reviewed: false, sameAsEn: true } },

    // ── Output ──────────────────────────────────────────────────────────────
    // ONE key on TWO elements: the OUTPUT section legend and the outputLevel
    // knob caption. One concept, one string — two entries would be two copies
    // of the same word in one table, drifting apart on the first edit.
    'label.output':   { en: { t: 'Output' },   fr: { t: 'Sortie',  reviewed: false } },
    'label.width':    { en: { t: 'Width' },    fr: { t: 'Largeur', reviewed: false } },
    'label.formant':  { en: { t: 'Formant' },  fr: { t: 'Formant', reviewed: false, sameAsEn: true } },

    // ── Impossible physics ──────────────────────────────────────────────────
    'label.impossiblePhysics': {
        en: { t: 'Impossible Physics' },
        fr: { t: 'Physique impossible', reviewed: false },
    },
    // "Maint." and not "Tenue": this page HAS an ADSR whose Sustain caption is
    // "Maintien", and two French words for one English one inside a 900 x 600
    // frame is the defect. "Maintien inf." is 73.56 px against the 72.00 px cap
    // and truncates; "Maint. inf." is 58.69 px. (O-Bowed keeps Tenue because it
    // has no ADSR at all — the exemption there does not reach here.)
    'label.infSustain': { en: { t: 'Inf. Sustain' }, fr: { t: 'Maint. inf.', reviewed: false } },
    'label.revJet':     { en: { t: 'Rev. Jet' },     fr: { t: 'Jet inv.',   reviewed: false } },
    'label.subHarm':    { en: { t: 'Sub Harm.' },    fr: { t: 'Sous-harm.', reviewed: false } },

    // ── Effects tab: section titles and bypass faces ────────────────────────
    'label.fx.chorus': { en: { t: 'Chorus' }, fr: { t: 'Chorus',   reviewed: false, sameAsEn: true } },
    'label.fx.delay':  { en: { t: 'Delay' },  fr: { t: 'Délai',    reviewed: false } },
    'label.fx.reverb': { en: { t: 'Reverb' }, fr: { t: 'Réverb.',  reviewed: false } },
    // The three-letter abbreviation French audio uses too. Not "Égaliseur":
    // .fx-title is `margin-right: auto` in a flex row whose knobs follow it,
    // and a 9-character legend would move all four EQ knobs.
    'label.fx.eq':     { en: { t: 'EQ' },     fr: { t: 'EQ',       reviewed: false, sameAsEn: true } },

    // The bypass button's two faces. Written through setLabel() from two
    // LITERAL call sites rather than one ternary — contract section 6, and
    // assertion 13 rejects a ternary inside a setLabel argument. "Marche" /
    // "Arrêt" rather than "Activé" / "Désactivé": the button is 9 px in a
    // 2px-8px pad, and this is the vocabulary a piece of hardware uses.
    'label.fx.on':  { en: { t: 'On' },  fr: { t: 'Marche', reviewed: false } },
    'label.fx.off': { en: { t: 'Off' }, fr: { t: 'Arrêt',  reviewed: false } },

    // ── Effects tab: the sixteen script-written knob captions ───────────────
    // Each is written by ONE literal setLabel() call at its own site. See the
    // header for why a data-driven key would have been invisible to the gates.
    'label.fx.rate':     { en: { t: 'Rate' },     fr: { t: 'Vitesse',    reviewed: false } },
    'label.fx.depth':    { en: { t: 'Depth' },    fr: { t: 'Prof.',      reviewed: false } },
    'label.fx.mix':      { en: { t: 'Mix' },      fr: { t: 'Mix',        reviewed: false, sameAsEn: true } },
    'label.fx.time':     { en: { t: 'Time' },     fr: { t: 'Durée',      reviewed: false } },
    'label.fx.feedback': { en: { t: 'Feedback' }, fr: { t: 'Réinj.',     reviewed: false } },
    'label.fx.mode':     { en: { t: 'Mode' },     fr: { t: 'Mode',       reviewed: false, sameAsEn: true } },
    'label.fx.low':      { en: { t: 'Low' },      fr: { t: 'Grave',      reviewed: false } },
    'label.fx.mid':      { en: { t: 'Mid' },      fr: { t: 'Médium',     reviewed: false } },
    'label.fx.midFreq':  { en: { t: 'Mid Freq' }, fr: { t: 'Fréq. méd.', reviewed: false } },
    'label.fx.high':     { en: { t: 'High' },     fr: { t: 'Aigu',       reviewed: false } },
    'label.fx.size':     { en: { t: 'Size' },     fr: { t: 'Taille',     reviewed: false } },
    'label.fx.damp':     { en: { t: 'Damp' },     fr: { t: 'Amort.',     reviewed: false } },
    'label.fx.predelay': { en: { t: 'Pre-dly' },  fr: { t: 'Pré-dél.',   reviewed: false } },
    'label.fx.mod':      { en: { t: 'Mod' },      fr: { t: 'Mod',        reviewed: false, sameAsEn: true } },
    // "Chatoiement" is 11 characters and the accurate word; the knob caption
    // box is 72 px and this is a reverb parameter every French-language DAW
    // ships as "Shimmer".
    'label.fx.shimmer':  { en: { t: 'Shimmer' },  fr: { t: 'Shimmer',    reviewed: false, sameAsEn: true } },

    // The accessible name that REPLACES the native title= setupFxKnob() used to
    // write onto all sixteen FX readouts. Moved verbatim, not authored.
    'aria.fxValueEdit': {
        en: { t: 'Double-click to edit' },
        fr: { t: 'Double-cliquer pour modifier', reviewed: false },
    },

    // ── The tuning panel's load-failure notice ──────────────────────────────
    // O-Wind's OWN string, written by O-Wind's own catch block — not the shared
    // module's copy. It was injected with innerHTML inside a styled <div>; it is
    // now a createElement + setLabel, because a machine-drafted translation must
    // never open a markup path (assertion 9).
    'label.tuningFailed': {
        en: { t: 'Tuning panel failed to load.' },
        fr: { t: 'Échec du chargement du panneau d’accord.', reviewed: false },
    },
});

// ============================================================================
// I18N_EXEMPT — reasoned exclusions, never silence.
//
// An entry is [text, reason] or [text, reason, scope]. The scope is a
// comma-separated list of `tag`, `.class` or `#id` matched against the node's
// own parent and its ancestors, and it is REQUIRED exactly where a string is
// exempt AND keyed on the same page — the one state in which the gate cannot
// tell a deliberate skip from a label somebody forgot. Nothing on this page is
// in that state, so every entry below is legitimately unscoped: each text
// appears in exactly one place.
// ============================================================================

export const I18N_EXEMPT = [

    ['Ouaricon',
     'the company name — never translated. It is the LAST flex item in the preset bar with nothing after it, so no localized string shares its line and nothing re-centres when the language changes'],

    // ── D-02: a preset name IS its filename ─────────────────────────────────
    ['Default',
     'the placeholder #preset-name carries until refreshPresetDisplay() overwrites it with the loaded preset. A preset name IS the JSON filename (OuariconPresetManager.h), so translating it breaks recall: a session saved against "Concert Flute" would not resolve "Flûte de concert"'],

    // ── The eight instrument <option> captions ──────────────────────────────
    //
    // NOT an arm-1 case, and it is worth being precise about that because it
    // looks like one. `instrumentPreset` is an AudioParameterInt (0..7,
    // PluginProcessor.cpp:326), not an AudioParameterChoice, so the host
    // automation lane shows a NUMBER and there is no option string to be
    // byte-identical to.
    //
    // They are exempt on D-02 instead, and the collision is real: these eight
    // strings are BYTE-IDENTICAL to the eight FACTORY PRESET NAMES
    // (PluginProcessor.cpp:814-1045, initializeFactoryPresets), and the preset
    // browser 30 px above the strip lists those same eight names in English,
    // read straight off the JSON filenames through getPresetList(). Localizing
    // the <select> while the browser above it stays English would put two
    // languages on the same eight names inside one 900 x 600 frame.
    //
    // A FRENCH USER THEREFORE STILL READS "Concert Flute" AND "Pan Flute" IN
    // THE INSTRUMENT SELECTOR. That is the cost, it is deliberate, and it is
    // reported rather than left to be discovered.
    ['Concert Flute',    'byte-identical to a factory preset name — the name IS the JSON filename (D-02), and the preset browser above lists the same eight in English'],
    ['Shakuhachi',       'byte-identical to a factory preset name — D-02, see Concert Flute'],
    ['Bansuri',          'byte-identical to a factory preset name — D-02, see Concert Flute'],
    ['Native Am. Flute', 'byte-identical to a factory preset name — D-02, see Concert Flute'],
    ['Recorder',         'byte-identical to a factory preset name — D-02, see Concert Flute'],
    ['Pan Flute',        'byte-identical to a factory preset name — D-02, see Concert Flute'],
    ['Piccolo',          'byte-identical to a factory preset name — D-02, see Concert Flute'],
    ['Ocarina',          'byte-identical to a factory preset name — D-02, see Concert Flute'],

    // ── D-01 arm 1: the delayMode option strings ────────────────────────────
    // initializeEffects() writes these into #delayModeSelect from the literal
    // array ['Normal', 'PingPong']. They are the delayMode AudioParameterChoice
    // options VERBATIM (PluginProcessor.cpp:377), so the page and the host
    // automation lane must name them identically. Neither word is keyed
    // anywhere else on this page, so neither needs a scope.
    ['Normal',   'a delayMode AudioParameterChoice option string VERBATIM — the page and the host automation lane must name it identically (D-01 arm 1)'],
    ['PingPong', 'a delayMode AudioParameterChoice option string VERBATIM — D-01 arm 1'],

    // ── Endonyms ────────────────────────────────────────────────────────────
    ['English',  'endonym — a language name is never translated'],
    ['Français', 'endonym — a language name is never translated'],

    // ── The shared tuning module ────────────────────────────────────────────
    // O-Wind is the only plugin in batch K4 that consumes the module file BY
    // REFERENCE rather than carrying a diverged plugin-owned copy:
    // CMakeLists.txt:92 embeds
    // ${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/js/tuning-panel.js
    // straight from the module tree. Localizing it is a cross-plugin change and
    // is out of scope for a per-plugin commit; a local edit would also be
    // reverted by /module-upgrade.
    //
    // O-WIND'S TUNING TAB IS THEREFORE ENGLISH IN BOTH LANGUAGES. It is one of
    // the three tabs, so a French user meets an English page on roughly a third
    // of this plugin's navigable surface. That is a scope statement, not an
    // oversight.
    ['Tuning tab captions',
     'every caption inside the Tuning tab belongs to the SHARED module ${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine — CMakeLists.txt:92 embeds js/tuning-panel.js by reference from the module tree, not a plugin-owned copy. Localizing it is a cross-plugin change and any local edit would be reverted by /module-upgrade. Its "Scala/TUN", "MTS-ESP" and "12-TET" strings are also tuningSystem AudioParameterChoice options, so they are exempt twice over'],
];

// ============================================================================
// TIP_BINDINGS — [selector, key] or [selector, key, wrapper].
//
// FIFTY-TWO anchors: 50 parameters with a control plus the two chrome
// controls. applyI18n() resolves `selector` with document.querySelector and,
// where a wrapper is given, walks `closest(wrapper)` to the cell the tip
// belongs on.
//
// "BIND TO THE IDS THE UI ALREADY USES" IS FALSE ON BOTH HALVES HERE, and the
// two halves fail for different reasons — check them separately.
//
//   SELECTOR HALF, false for 26 of 52. Not one .knob-control on the Sound tab
//   carries an id; they are addressed as .knob-control[data-param="..."],
//   which is also how the page's own bindSliderParam() finds them
//   (index.html:1910). The other 26 selectors ARE ids.
//
//   TARGET HALF, false for 18 of 52. The 26 Sound-tab cells need NO wrapper —
//   .knob-control is itself the 72 x 87.8 px flex column holding the SVG, the
//   caption and the readout, so the id-less selector already lands on the cell
//   the pointer aims at. The 16 Effects knobs DO need one: #<id>Knob is only
//   the 44 x 44 px vine face, and .knob-container is the 66 x 70.8 px cell that
//   also holds the caption and the readout. #instrument-select and
//   #delayModeSelect walk to their label + control pair the same way.
//
// THE CHROME BINDS BARE, both of them. .settings-cluster contains BOTH
// #gear-btn AND #settings-popover, so a wrapper walk would make hovering
// #lang-select resolve to the gear's anchor and show the gear's tip over the
// panel the user just opened. Carried from O-Comp, and true here too.
//
// THE FOUR BYPASS BUTTONS ALSO BIND BARE. .fx-header holds the section title
// and the bypass button, so a wrapper there would put the bypass tip over a
// caption that is not the bypass.
//
// TWENTY-ONE OF THESE ANCHORS LIVE INSIDE #tab-effects, which is a
// `.tab-panel` and therefore `display: none` until its tab is clicked. They
// still RESOLVE at applyI18n() time — initializeEffects() has already built
// them, and initI18n() is called after it — so nothing here is a
// "tip target not found". They simply cannot be HOVERED until the tab is
// active, which is why tests/ui_tip_render_check.js clicks the tab through the
// page's own handler rather than stripping the class.
// ============================================================================

export const TIP_BINDINGS = [

    // ── Sound tab: excitation ───────────────────────────────────────────────
    ['.knob-control[data-param="breathPressure"]',    'tip.breathPressure'],
    ['.knob-control[data-param="embouchure"]',        'tip.embouchure'],
    ['.knob-control[data-param="breathNoise"]',       'tip.breathNoise'],

    // ── Sound tab: resonator ────────────────────────────────────────────────
    ['.knob-control[data-param="material"]',          'tip.material'],
    ['.knob-control[data-param="toneColor"]',         'tip.toneColor'],
    ['.knob-control[data-param="airColumn"]',         'tip.airColumn'],
    ['.knob-control[data-param="jetReflection"]',     'tip.jetReflection'],
    ['.knob-control[data-param="endReflection"]',     'tip.endReflection'],

    // ── Sound tab: ADSR ─────────────────────────────────────────────────────
    // #adsr-toggle is bare: its parent .section-label also holds the
    // "ADSR Envelope" caption span, which is a different control's text.
    ['#adsr-toggle',                                  'tip.adsrEnabled'],
    ['.knob-control[data-param="adsrAttack"]',        'tip.adsrAttack'],
    ['.knob-control[data-param="adsrDecay"]',         'tip.adsrDecay'],
    ['.knob-control[data-param="adsrSustain"]',       'tip.adsrSustain'],
    ['.knob-control[data-param="adsrRelease"]',       'tip.adsrRelease'],

    // ── Sound tab: expression ───────────────────────────────────────────────
    ['.knob-control[data-param="vibratoRate"]',       'tip.vibratoRate'],
    ['.knob-control[data-param="vibratoDepth"]',      'tip.vibratoDepth'],
    ['.knob-control[data-param="vibratoTremolo"]',    'tip.vibratoTremolo'],
    ['.knob-control[data-param="vibratoDriftDepth"]', 'tip.vibratoDriftDepth'],
    ['.knob-control[data-param="vibratoDriftSpeed"]', 'tip.vibratoDriftSpeed'],
    ['.knob-control[data-param="flutterTongue"]',     'tip.flutterTongue'],
    ['.knob-control[data-param="flutterRate"]',       'tip.flutterRate'],
    ['.knob-control[data-param="growl"]',             'tip.growl'],

    // ── Sound tab: output ───────────────────────────────────────────────────
    ['.knob-control[data-param="width"]',             'tip.width'],
    ['.knob-control[data-param="formant"]',           'tip.formant'],
    ['.knob-control[data-param="outputLevel"]',       'tip.outputLevel'],

    // ── Sound tab: impossible physics ───────────────────────────────────────
    ['.knob-control[data-param="infiniteSustain"]',   'tip.infiniteSustain'],
    ['.knob-control[data-param="reversedJet"]',       'tip.reversedJet'],
    ['.knob-control[data-param="subHarmonics"]',      'tip.subHarmonics'],

    // ── Sound tab: instrument strip ─────────────────────────────────────────
    // #tone-hole-toggle IS the 130 x 28 px cell (track + caption), bare.
    // #instrument-select walks to .instrument-selector, the label + select
    // pair; that class matches exactly once on this page.
    ['#tone-hole-toggle',                             'tip.toneHoleToggle'],
    ['#instrument-select',                            'tip.instrumentPreset', '.instrument-selector'],

    // ── Effects tab: chorus ─────────────────────────────────────────────────
    ['#chorusBypassBtn',                              'tip.chorusBypass'],
    ['#chorusRateKnob',                               'tip.chorusRate',     '.knob-container'],
    ['#chorusDepthKnob',                              'tip.chorusDepth',    '.knob-container'],
    ['#chorusMixKnob',                                'tip.chorusMix',      '.knob-container'],

    // ── Effects tab: delay ──────────────────────────────────────────────────
    // .fx-dropdown-container is created only for the delay mode selector, so
    // the walk is unambiguous — verified, one node on the page.
    ['#delayBypassBtn',                               'tip.delayBypass'],
    ['#delayTimeKnob',                                'tip.delayTime',      '.knob-container'],
    ['#delayFeedbackKnob',                            'tip.delayFeedback',  '.knob-container'],
    ['#delayModeSelect',                              'tip.delayMode',      '.fx-dropdown-container'],
    ['#delayMixKnob',                                 'tip.delayMix',       '.knob-container'],

    // ── Effects tab: EQ ─────────────────────────────────────────────────────
    ['#eqBypassBtn',                                  'tip.eqBypass'],
    ['#eqLowGainKnob',                                'tip.eqLowGain',      '.knob-container'],
    ['#eqMidGainKnob',                                'tip.eqMidGain',      '.knob-container'],
    ['#eqMidFreqKnob',                                'tip.eqMidFreq',      '.knob-container'],
    ['#eqHighGainKnob',                               'tip.eqHighGain',     '.knob-container'],

    // ── Effects tab: reverb ─────────────────────────────────────────────────
    ['#reverbBypassBtn',                              'tip.reverbBypass'],
    ['#reverbSizeKnob',                               'tip.reverbSize',     '.knob-container'],
    ['#reverbDampKnob',                               'tip.reverbDamp',     '.knob-container'],
    ['#reverbPredelayKnob',                           'tip.reverbPredelay', '.knob-container'],
    ['#reverbModKnob',                                'tip.reverbMod',      '.knob-container'],
    ['#reverbShimmerKnob',                            'tip.reverbShimmer',  '.knob-container'],
    ['#reverbMixKnob',                                'tip.reverbMix',      '.knob-container'],

    // ── Chrome. BARE, both — see the header. ────────────────────────────────
    ['#gear-btn',                                     'tip.gear'],
    ['#lang-select',                                  'tip.langSelect'],
];

// The tooltip lookup. Returns {t, b} — never null, never a bare key without a
// console.warn saying so, because a silently-missing tip renders as an empty
// surface that looks like a positioning bug rather than a missing entry.
//
// Unreferenced at runtime today: applyI18n() calls it only from the
// TIP_BINDINGS loop, which is empty. It is exported verbatim all the same, so
// that the canon block stays byte-identical to every other copy and Stage M can
// add bodies to I18N without touching this file's shape.
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
