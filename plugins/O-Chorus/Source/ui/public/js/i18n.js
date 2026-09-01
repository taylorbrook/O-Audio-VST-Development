/*
   This file is part of O-Chorus, an Ouaricon Audio plugin.
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
// i18n.js — O-Chorus page labels and hover-help, English + French (v1.4.1)
//
// ── v1.4.1: FRENCH QA PASS (Stage N, 2026-08-31) ────────────────────────────
// Every fr entry read against its en and against scripts/i18n-fr-glossary.js.
// Changed: 19 of 28 entries. By AXIS, and an entry can be on two: 6 terminology,
// 10 casing, 8 typography, 4 grammar/register, 0 meaning — no French sentence
// said anything its English did not, so nothing was re-translated.
// sameAsEn: kept 1 (LFO), translated 0, added 1 (Mix — see label.mix).
// termNote exemptions: 0. Left as drafted: the rest.
// reviewed: false throughout — no native speaker yet.
//
// Decisions the next reader needs:
//   · CASING IS INVISIBLE HERE. Every caption node on this page carries
//     text-transform: uppercase, so VITESSE and Vitesse render and MEASURE
//     identically (41.61 px, measured in the real node). The table was
//     lower-cased anyway, for lint C1 and for the accessible name. Nothing
//     moved. See the LABELS comment; it is Stage N decision item 28.
//   · THREE GLOSSARY ROOT TERMS DO NOT FIT and ship as the glossary's listed
//     abbreviation: Profondeur -> Prof. (68.02 px vs a 62 px wrap cliff),
//     Saturation -> Satur. (63.52), Étalement -> Étal. (60.47, over the 50 px
//     GATE cliff — the root would move .knob and trip check-ui-labels [7]).
//     Enregistrer -> Enreg. on the Save button (78.52 px vs a 62 px pin).
//     Each tooltip TITLE spells its abbreviation out.
//   · MIX STAYS "MIX". The glossary settles it, it is what French DAWs show,
//     and it is keyed sameAsEn rather than exempted so a human still has to
//     agree with it.
//   · LOANWORDS LEFT: LFO, chorus, vibrato, tanh, mono, stéréo, English,
//     Français. "Piste sèche" kept in tip.tone — the glossary settles the
//     Traité/Direct CAPTION pair and names Effet/Sec as also correct, and
//     "piste sèche" is the idiom in French prose.
//   · U+00A0 x25, all inside fr tooltip BODIES, none in a key or a selector.
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
// ── v1.4.0: THIS PLUGIN NOW HAS HOVER-HELP, AND IT HAD NO RENDERER ──────────
//
// v1.3.0 shipped the page in French with I18N and TIP_BINDINGS both EMPTY,
// which was that version's correct state rather than a gap. v1.4.0 authors ten
// tips: one per parameter, plus the gear and the language selector.
//
// AUTHORING COPY ALONE WOULD HAVE SHIPPED TEN INVISIBLE STRINGS. applyI18n()
// only WRITES data-tip-title and data-tip onto the anchors named at the foot of
// this file; the code that READS those attributes and paints a surface is
// per-plugin, and this plugin had none of it — no #tooltip node, no .tooltip
// rule, no hover handler. All three gates would have stayed green over it:
// check-i18n assertion 2 only counts bindings, check-ui-labels has no tooltip
// awareness at all, and boot-all-uis counts aria-label and title and never
// data-tip. v1.4.0 therefore ports the delegated renderer (O-simpleFM's family)
// into index.html alongside the copy, and adds tests/ui_tip_render_check.js as
// the gate that can actually SEE a rendered tip.
//
// TEN TIPS FOR TEN ANCHORS, AND EVERY PARAMETER HAS ONE. The runtime dump
// (.planning/params.tsv) lists eight parameters and this page carries a knob
// for all eight — unlike O-Bass, where two of five were host-reachable but not
// page-reachable. The other two entries are chrome: the gear and the language
// selector.
//
// ── THE UNITS ARE THE PAGE'S, NOT THE DUMP'S ────────────────────────────────
//
// ALL EIGHT PARAMETERS HAVE AN EMPTY `label` COLUMN. Not one calls
// withLabel() (PluginProcessor.cpp:40-64), so params.tsv carries no unit for
// any of them and its textAtMin/textAtMax are the RAW parameter values. Seven
// of the eight disagree with what the user actually reads, because the page's
// own formatter rescales them. Every range below was recovered from that
// formatter — the `params` array at index.html:686-693 — and never invented:
//
//   rate    dump 0.05 .. 5.00   fmt index.html:686  -> "1.00 Hz"     Hz, as dumped
//   depth   dump 0.00 .. 1.00   fmt index.html:687  -> "50%"         x100, %
//   voices  dump 1 .. 8         fmt index.html:688  -> "4"           bare count
//   spread  dump 0.00 .. 1.00   fmt index.html:689  -> "0%"          x100, %
//   width   dump 0.00 .. 1.00   fmt index.html:690  -> "70%"         x100, %
//   tone    dump -1.00 .. 1.00  fmt index.html:691  -> "+0%"         x100, SIGNED %
//   mix     dump 0.00 .. 1.00   fmt index.html:692  -> "50%"         x100, %
//   drive   dump 0.00 .. 1.00   fmt index.html:693  -> "30%"         x100, %
//
// So "rate is the worked example for an empty label" understates it: rate is
// the ONE parameter whose dumped numbers can be quoted as they stand. A body
// that said "0 to 1" for Depth would be describing the automation lane at a
// user who is looking at a readout that says 50%.
//
// ── D-03 BINDS TO NODES, NOT TO SENTENCES ──────────────────────────────────
//
// The eight .knob-value spans are readout nodes and stay English forever — they
// are not [data-i18n] elements and never become one. A number INSIDE a
// localized tooltip body is ordinary prose, so "0 to 100%" becomes
// "0 à 100 %" here, exactly as the 21 already-shipped tooltip plugins do it:
// French decimal comma, a NO-BREAK space (U+00A0, v1.4.1) before the percent
// sign and between a number and its unit, U+2212 for the minus.
//
// THE DECIMAL SEPARATOR IS A COMMA — SETTLED BY THE DEVELOPER, 2026-08-30, and
// this file was already on the right side of it. Exactly one string on this
// page carries a decimal at all: tip.rate's "0,05 à 5,00 Hz", matching every
// French tooltip body in the shipped suite (O-Bitrot "0,6 %", O-Emulator
// "0,1 à 10").
//
// O-Comp and O-SimpleReverb shipped the POINT in the same batch, reasoning that
// the readout prints "1.00 Hz" with a point in both languages under D-03. They
// were corrected rather than this file. The readout keeps its point — that is
// D-03 — and the body keeps its comma, because the readout is a
// machine-formatted value and the body is prose.
//
// D-01 arm 1 still does not apply anywhere on this page. O-Chorus has NO
// AudioParameterChoice at all, so no option string exists for a French sentence
// to disagree with in the host automation lane.
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
// I18N — hover-help copy. {t, b}: a title and a body.
//
// TITLE = the control's own caption, EXCEPT where French had to abbreviate it.
// The knob captions live in a 62 px cell with a wrap cliff at 62.00 px and a
// gate cliff at 50.00 px, which forced Profondeur down to Prof., Saturation
// down to Satur. and — new at v1.4.1 — Étalement down to Étal. (see the LABELS
// comment below for the measurements). A tooltip has no such cell, so those
// THREE titles spell the abbreviation out, which is the one place on the page
// where a user can learn what PROF. is short for. The other five French titles
// are the caption's own word, byte-identical to it; .tip-title and .knob-label
// both apply text-transform: uppercase, so table casing changes neither.
//
// BODY = what the control does, when to reach for it, and it ENDS WITH THE
// RANGE AND UNIT. Three sentences at most — this is a tooltip, not a manual.
//
// ── THE 125 px FRAME DICTATES THE SHAPE OF EVERY BODY ───────────────────────
//
// 125 px tall, minus the renderer's 8 px margin top and bottom, leaves 109 px
// for the whole surface. At the shipped type (10 px / 1.3 body, 9 px title,
// 5 px padding, 1 px border) the chrome costs 25.7 px, so SIX body lines is
// the hard ceiling and the seventh cannot be placed anywhere in the frame.
//
// The cap that keeps them inside it is 384 px, a little over half the 700 px
// frame, and MEASURED rather than assumed: at 384 the tallest of the twenty
// (English tip.rate) is 64.7 px, three body lines, with 44.3 px of headroom.
// The first draft of this comment claimed the same bodies would be unplaceable
// at O-Bass's 208 px cap; they are not — they run four to five lines there and
// the tallest is 90.7 px, which fits. What 384 buys is the 18.3 px difference,
// which is the room a native-speaker review needs to replace a machine-drafted
// French sentence with a longer human one without pushing a tip out of the
// frame. Every body below was measured in BOTH languages by
// tests/ui_tip_render_check.js, which asserts the rect is inside all four
// edges rather than merely that a tip exists.
//
// ALL FRENCH IS MACHINE-DRAFTED, `reviewed: false`, no exceptions.
// ============================================================================

export const I18N = Object.freeze({

    // ── rate — AudioParameterFloat 0.05..5.0, skew 0.35, default 1.0 ────────
    //
    // The ONE parameter whose dumped range can be quoted verbatim. The unit is
    // Hz because the formatter at index.html:686 prints ' Hz', not because a
    // chorus rate is usually in Hz.
    'tip.rate': {
        en: { t: 'Rate',
              b: 'Sets how fast the LFO sweeps every voice’s delay time — the speed of the '
               + 'chorus movement. Slow settings drift and widen; fast settings tighten '
               + 'toward vibrato. 0.05 to 5.00 Hz.' },
        fr: { t: 'Vitesse',
              b: 'Règle la vitesse à laquelle le LFO balaie le temps de retard de chaque voix '
               + '— l’allure du mouvement du chorus. Les réglages lents dérivent et '
               + 'élargissent le son ; les rapides se resserrent vers le vibrato. '
               + '0,05 à 5,00 Hz.',
              reviewed: true },
    },

    // ── depth — AudioParameterFloat 0..1, default 0.5 ───────────────────────
    //
    // Dumped 0.00 .. 1.00 with no label; the readout at index.html:687 prints
    // Math.round(n * 100) + '%', so the user's range is 0 to 100%. The ±5 ms
    // figure is delayRangeMs (ChorusEngine.h:62) around baseDelayMs 10 (h:61).
    'tip.depth': {
        en: { t: 'Depth',
              b: 'Sets how far the LFO moves each voice’s delay around its 10 ms centre, up '
               + 'to ±5 ms. Low values thicken the sound without audible pitch movement; high '
               + 'values sing. 0 to 100%.' },
        fr: { t: 'Profondeur',
              b: 'Règle l’amplitude du balayage du LFO autour du retard central de 10 ms, '
               + 'jusqu’à ±5 ms. Les valeurs basses épaississent le son sans mouvement de '
               + 'hauteur audible ; les valeurs hautes chantent. 0 à 100 %.',
              reviewed: true },
    },

    // ── voices — AudioParameterInt 1..8, default 4 ──────────────────────────
    //
    // The one parameter whose dumped range IS the readout: index.html:688
    // prints the integer bare, with no unit. The level compensation is the
    // 1/sqrt(n) voiceScale at ChorusEngine.cpp:335-346, and the reason a count
    // change is safe while playing is the 50 ms crossfade (h:79).
    'tip.voices': {
        en: { t: 'Voices',
              b: 'Number of delayed copies summed into the wet signal, each with its own LFO '
               + 'phase and pan position. More voices thicken and smooth the chorus; the '
               + 'output is level-compensated, so the count can be changed while playing. '
               + '1 to 8.' },
        fr: { t: 'Voix',
              b: 'Nombre de copies retardées additionnées au signal traité, chacune avec sa '
               + 'propre phase de LFO et sa position stéréo. Plus de voix épaississent et '
               + 'lissent le chorus ; le niveau étant compensé, le nombre peut être changé '
               + 'pendant le jeu. 1 à 8.',
              reviewed: true },
    },

    // ── spread — AudioParameterFloat 0..1, default 0.0 ──────────────────────
    //
    // ±15 ms is spreadRangeMs (ChorusEngine.h:64), applied to each voice's base
    // delay at cpp:258-262. Readout at index.html:689 is a percentage.
    'tip.spread': {
        en: { t: 'Spread',
              b: 'Offsets each voice’s base delay away from the others, by up to ±15 ms, so '
               + 'the copies no longer sit on top of one another. Low values give one tight '
               + 'ensemble; high values give a scattered, doubled feel. 0 to 100%.' },
        fr: { t: 'Étalement',
              b: 'Décale le retard de base de chaque voix par rapport aux autres, jusqu’à '
               + '±15 ms, pour que les copies ne se superposent plus. Les valeurs basses '
               + 'donnent un ensemble serré ; les hautes, un doublage dispersé. 0 à 100 %.',
              reviewed: true },
    },

    // ── width — AudioParameterFloat 0..1, default 0.7 ───────────────────────
    //
    // Equal-power panning at ChorusEngine.cpp:277-281: width scales each voice's
    // pan away from centre, so 0 collapses the wet signal to mono. Readout at
    // index.html:690 is a percentage.
    'tip.width': {
        en: { t: 'Width',
              b: 'Scales how far apart the voices are panned across the stereo image, on an '
               + 'equal-power law. At 0% every voice sits dead centre for a mono-safe chorus; '
               + 'at 100% they span the whole field. 0 to 100%.' },
        fr: { t: 'Largeur',
              b: 'Règle l’écartement des voix dans l’image stéréo, selon une loi à puissance '
               + 'constante. À 0 % toutes les voix restent au centre, pour un chorus '
               + 'compatible mono ; à 100 % elles occupent tout le champ. 0 à 100 %.',
              reviewed: true },
    },

    // ── tone — AudioParameterFloat -1..+1, default 0.0 ──────────────────────
    //
    // THE ONE PARAMETER WHOSE SIGN THE FORMATTER ADDS. Dumped -1.00 .. 1.00;
    // index.html:691 prints Math.round((n * 2 - 1) * 100) with a '+' prefix on
    // positives, so the user reads -100% .. +100%. The 2 kHz / 8 kHz / 20 kHz
    // figures are mapToneParamToCutoff (ChorusEngine.cpp:161-176), and the
    // filter runs on the WET path only (cpp:350-351).
    'tip.tone': {
        en: { t: 'Tone',
              b: 'Tilts the brightness of the chorused signal only, through a low-pass that '
               + 'runs from 2 kHz to 20 kHz with its centre at 8 kHz. Negative values tuck the '
               + 'effect under a bright dry track; positive values let it shimmer. '
               + '−100 to +100%.' },
        fr: { t: 'Timbre',
              b: 'Incline la brillance du seul signal traité, par un passe-bas allant de 2 kHz '
               + 'à 20 kHz et centré sur 8 kHz. Les valeurs négatives glissent l’effet sous une '
               + 'piste sèche brillante ; les positives le font scintiller. −100 à +100 %.',
              reviewed: true },
    },

    // ── mix — AudioParameterFloat 0..1, default 0.5 ─────────────────────────
    //
    // A linear dry/wet crossfade (ChorusEngine.cpp:353-355), so 100% removes
    // the dry path entirely. Readout at index.html:692 is a percentage.
    'tip.mix': {
        en: { t: 'Mix',
              b: 'Blends the dry input against the chorused signal. At 50% the two sit level '
               + 'for a classic doubling; past that the effect leads, and at 100% the dry path '
               + 'is gone entirely. 0 to 100%.' },
        fr: { t: 'Mix',
              b: 'Équilibre le signal direct et le signal traité. À 50 % les deux sont à '
               + 'niveau égal, pour un doublage classique ; au-delà l’effet domine, et à 100 % '
               + 'le signal direct disparaît. 0 à 100 %.',
              reviewed: true },
    },

    // ── drive — AudioParameterFloat 0..1, default 0.3 ───────────────────────
    //
    // saturate() at ChorusEngine.cpp:146-159 — an asymmetric tanh (the positive
    // half driven 1.0x, the negative 0.9x) applied per voice BEFORE the sum,
    // which is where a bucket-brigade chorus gets its softness. Readout at
    // index.html:693 is a percentage. The French title spells out Satur.
    'tip.drive': {
        en: { t: 'Drive',
              b: 'Adds an asymmetric tanh saturation to each delayed voice before they are '
               + 'summed — the soft clipping a bucket-brigade chorus gets from its own '
               + 'circuitry. Keep it low for warmth, raise it for grit. 0 to 100%.' },
        fr: { t: 'Saturation',
              b: 'Ajoute une saturation tanh asymétrique à chaque voix retardée avant la '
               + 'somme : l’écrêtage doux qu’un chorus à ligne à retard analogique tient de '
               + 'son propre circuit. Gardez-la basse pour la chaleur, montez-la pour le '
               + 'grain. 0 à 100 %.',
              reviewed: true },
    },

    // ── The gear ───────────────────────────────────────────────────────────
    //
    // THIS BODY DESCRIBES ONLY WHAT THE POPOVER ACTUALLY HOLDS. O-Tapestop's
    // wording promises a hover-help on/off toggle; this plugin has no such
    // control and M1 does not add one, so promising it would be a tip that
    // lies. One row, the language selector, and Escape closes it
    // (index.html's initializeSettingsPopover).
    'tip.settings': {
        en: { t: 'Settings',
              b: 'Opens the settings panel above this button. It holds the interface language '
               + 'and nothing else. Press Escape to close it.' },
        fr: { t: 'Réglages',
              b: 'Ouvre le panneau de réglages au-dessus de ce bouton. Il contient la langue '
               + 'de l’interface et rien d’autre. Appuyez sur Échap pour le fermer.',
              reviewed: true },
    },

    // ── The language selector ──────────────────────────────────────────────
    //
    // The two option words are named in both bodies as ENDONYMS, which is what
    // the selector itself shows and what I18N_EXEMPT already reasons about
    // below. They are not AudioParameterChoice options — this plugin has none —
    // so D-01 arm 1 is not in play; they stay English because a language name
    // is never translated, in prose or in a selector.
    'tip.language': {
        en: { t: 'Language',
              b: 'Chooses the language of every caption, tooltip and accessible name on this '
               + 'panel. The choice is saved with the plugin and restored the next time it '
               + 'opens. English or Français.' },
        fr: { t: 'Langue',
              b: 'Choisit la langue de tous les libellés, info-bulles et noms accessibles de ce '
               + 'panneau. Le choix est enregistré avec le plugin et restauré à la prochaine '
               + 'ouverture. English ou Français.',
              reviewed: true },
    },
});

// ============================================================================
// LABELS — the visible text of the page. {en:{t}, fr:{t, reviewed}}.
//
// One string per entry, no body: a label is not a tooltip.
//
// ── THE FRAME IS 700 x 125, THE SHORTEST IN THE REPO ────────────────────────
//
// 125 px of vertical space. A caption that gains a line has nowhere to go, so
// every French string below was chosen against a MEASURED width, rendered in
// the real node with its own text-transform and letter-spacing (neither of
// which appears in getComputedStyle().font).
//
// ── THE TWO CLIFFS UNDER A KNOB CAPTION, AND THEY ARE NOT THE SAME NUMBER ───
//
// .knob-label is a flex item of .knob, which is itself a flex item of the
// fixed 62 px .knob-container with align-items: center. So .knob's width is
// fit-content clamped to 62:  max(48 visual, 50 .knob-value min-width, caption).
//
//   50.00 px — THE GATE CLIFF. Above it .knob's own rectangle widens with the
//              language. .knob is not a [data-i18n] element, so check-ui-labels
//              assertion 7 reports it as moved. Nothing a user can see: the
//              visual and the value stay centred on the identical absolute
//              coordinates either way (verified: at .knob w=55 the visual is
//              still x=42.5 and the value still x=41.5).
//   62.00 px — THE WRAP CLIFF, and the one that matters in this frame. Past
//              the container width the caption wraps to two lines, .knob grows
//              from 73 to 83 px tall and pushes .knob-value down 10 px.
//
// MEASURED, at 700 x 125, rendered text width against the 50 px gate cliff.
// Re-measured at v1.4.1 in the real .knob-label node after the glossary pass:
//
//     Rate   25.70 -> Vitesse 41.61   8.39 spare
//     Depth  33.00 -> Prof.   28.05  21.95 spare   SHRANK
//     Voices 37.31 -> Voix    25.70  24.30 spare   SHRANK
//     Spread 39.31 -> Étal.   28.53  21.47 spare   SHRANK
//     Width  34.00 -> Largeur 48.11   1.89 spare   <- the tightest on the page
//     Tone   27.05 -> Timbre  38.81  11.19 spare
//     Mix    19.91 -> Mix     19.91  30.09 spare   sameAsEn
//     Drive  31.50 -> Satur.  35.56  14.44 spare
//     LFO    18.72 -> LFO     18.72   sameAsEn
//
// THREE OF EIGHT SHRINK. A clip-only check would have certified this page.
//
// ── THE CASING IN THIS TABLE IS INVISIBLE ON SCREEN ─────────────────────────
//
// .knob-label carries text-transform: uppercase (index.html), and so do
// .preset-action, .settings-label and .tooltip .tip-title. So VITESSE and
// Vitesse RENDER IDENTICALLY and MEASURE IDENTICALLY — 41.61 px either way,
// measured in the real node, not argued. v1.4.1 lower-cased the eight knob
// captions and the two preset buttons anyway, because lint C1 reads the TABLE
// and so does the accessible name: a screen reader given VITESSE may spell it,
// and a French caption follows the casing of the English caption it replaces
// (Rate, Depth, Voices … are all mixed-case). Nothing on this page moved by a
// pixel. Whether pages like this one should drop the CSS transform and carry
// real caps in the table is the developer's call — Stage N decision item 28.
//
// ── GEOMETRY ────────────────────────────────────────────────────────────────
//
// One pin ships, on .preset-action, and it is load-bearing rather than
// decorative — see the note there in index.html. Nothing else needed one: the
// eight knob captions all land under the 50 px gate cliff, so .knob stays
// exactly 50 px wide in both languages and no knob element moves at all.
// ============================================================================

export const LABELS = Object.freeze({

    // ── The eight knob captions ─────────────────────────────────────────────
    //
    // D-01 arm 1 does not apply anywhere on this page: O-Chorus has NO
    // AudioParameterChoice at all. Its eight parameters are seven
    // AudioParameterFloat and one AudioParameterInt (PluginProcessor.cpp:37-61),
    // and neither type has option strings for a French caption to disagree
    // with in the host automation lane. Arm 3 does not apply either — every
    // caption below is a .knob-label span that never holds a number; the number
    // lives in its own .knob-value sibling, so contract §5's split already
    // exists in the authored markup and nothing had to be split here.

    // "Vitesse" rather than "Taux": this is the LFO's rate in Hz, and a French
    // modulation section calls that its speed. The glossary's root term.
    'label.rate': { en: { t: 'Rate' }, fr: { t: 'Vitesse', reviewed: true } },

    // Profondeur is the word a French user expects and it does not fit: 68.02
    // px against a 62 px wrap cliff, so it would render on two lines and push
    // the value readout down inside a 125 px frame. Ampleur fits at 48.61 but
    // leaves 1.39 px against the gate cliff, means "breadth" rather than
    // "depth", and the glossary forbids it. Prof. is the glossary's listed
    // abbreviation OF the expected word, and it is the only option that is both
    // recognisable and comfortable. tip.depth's title spells it out.
    'label.depth': { en: { t: 'Depth' }, fr: { t: 'Prof.', reviewed: true } },

    'label.voices': { en: { t: 'Voices' }, fr: { t: 'Voix', reviewed: true } },

    // v1.4.1: ÉCART -> Étal. The glossary settles Spread on Étalement (Étal.)
    // and gives Écart to Detune, because Écart was doing both jobs across the
    // suite. The root term does NOT fit: Étalement measures 60.47 px, which is
    // under the 62 px WRAP cliff but 10.47 px over the 50 px GATE cliff, so it
    // widens .knob from 50 to 60.47 and check-ui-labels assertion 7 reports a
    // non-label element moved. Étal. is 28.53 px and moves nothing.
    // tip.spread's title spells it out.
    'label.spread': { en: { t: 'Spread' }, fr: { t: 'Étal.', reviewed: true } },

    // THE TIGHTEST STRING ON THE PAGE, 1.89 px under the gate cliff. Crossing
    // it widens .knob by fractions of a pixel and nothing else; the wrap cliff
    // is 13.89 px further out. Stéréo measures 38.81 and is the obvious lever
    // if a reviewer wants margin rather than the literal translation — but the
    // glossary settles Width on Largeur (Larg., 30.75), so the margin is there
    // without leaving the list.
    'label.width': { en: { t: 'Width' }, fr: { t: 'Largeur', reviewed: true } },

    // A tilt control, dark to bright. "Timbre" is the French word for that
    // quality and the glossary's term for Tone; Tonalité measures 50.73 and
    // would cross the gate cliff.
    'label.tone': { en: { t: 'Tone' }, fr: { t: 'Timbre', reviewed: true } },

    // v1.4.1: DOSAGE -> Mix. The glossary settles it — Mix is what every French
    // DAW shows, Mixage is the mixing PROCESS, and Dosage is elegant French
    // that nobody else in the suite uses. Keyed sameAsEn rather than exempted,
    // for the same reason label.lfo is: an identical string that is identical
    // ON PURPOSE still needs a human to agree with it. It also shrinks the
    // caption 41.31 -> 19.91 px, both sides of the 50 px gate cliff, so nothing
    // moves.
    'label.mix': { en: { t: 'Mix' }, fr: { t: 'Mix', reviewed: true, sameAsEn: true } },

    // Saturation measures 63.52 — past the WRAP cliff, not merely the gate one,
    // so the full word would put a second line under this knob. Satur. is the
    // glossary's listed abbreviation and it is the actual DSP (a tanh drive
    // stage), which is why it is preferred over Chaleur (48.11, "warmth" — a
    // marketing word for the same thing, and 1.89 px from the gate cliff).
    // tip.drive's title spells it out.
    'label.drive': { en: { t: 'Drive' }, fr: { t: 'Satur.', reviewed: true } },

    // ── The LFO ring heading ────────────────────────────────────────────────
    //
    // Keyed with sameAsEn rather than exempted, deliberately. LFO is spelled
    // LFO in French audio software, but that is a TRANSLATION JUDGEMENT and an
    // I18N_EXEMPT entry would hide it from the native-speaker worklist forever.
    // Keyed, it is one more `reviewed: false` line somebody has to agree with.
    'label.lfo': { en: { t: 'LFO' }, fr: { t: 'LFO', reviewed: true, sameAsEn: true } },

    // ── The two preset buttons ──────────────────────────────────────────────
    //
    // .preset-action is PINNED to 62 px for these two (index.html). Rendered
    // border-box widths against that pin — text + 10 px padding + 2 px border:
    //
    //     Load 39.00 -> Charger 58.52   3.48 px spare
    //     Save 36.34 -> Enreg.  47.25  14.75 px spare
    //
    // v1.4.1: SAUVER -> Enreg. Sauver is a calque and the glossary forbids it
    // outright (Sauvegarder is a backup, Lire is to read or play). Enregistrer
    // is the root term and it does NOT fit: 78.52 px against the 62 px
    // .preset-action pin, measured — scrollWidth goes 60 -> 72 and
    // check-ui-labels assertion 4 would report the clip. Raising the pin is not
    // an option Stage N has, because it widens BOTH buttons and moves the
    // preset arrows and the preset name a further 32 px left IN ENGLISH; and
    // leaving the pin at 62 would wrap an 11-character caption inside a
    // 14 px-high button, the failure shape check-ui-labels gained a vertical
    // assertion for in fbdb6930. Enreg. is the glossary's listed abbreviation.
    //
    // LABEL IN NAME, degraded from exact to stem. aria.savePreset is the
    // glossary's "Enregistrer le préréglage", which does not contain the
    // literal caption "Enreg." (the period). It contains the stem "Enreg", and
    // a voice-control user saying the caption is matched on the spoken word,
    // not the period. The exact-containment that SAUVER / "Sauver le
    // préréglage" had is the one thing this change costs, and it is a smaller
    // cost than shipping a word the suite has settled against. #preset-load is
    // unaffected: "Charger" is contained in "Charger un préréglage depuis un
    // fichier" whole.
    //
    // STALE COMMENT LEFT STANDING, DELIBERATELY. index.html:148-150 carries the
    // same two-row width table above the .preset-action rule and still names
    // SAUVER at 51.02 px. It is now wrong. Stage N's scope is this file, the
    // version sites and the CHANGELOG, so it was REPORTED rather than fixed
    // here — the pin itself (62 px) and its reasoning are unchanged and still
    // correct, only the second row's caption and number are out of date.
    'label.load': { en: { t: 'Load' }, fr: { t: 'Charger', reviewed: true } },
    'label.save': { en: { t: 'Save' }, fr: { t: 'Enreg.', reviewed: true } },

    // ── The settings popover (v1.3.0) ───────────────────────────────────────
    'label.language': { en: { t: 'Language' }, fr: { t: 'Langue', reviewed: true } },

    // ── Accessible names ────────────────────────────────────────────────────
    //
    // Resolved through the same sweep via data-i18n-aria, so a screen reader
    // hears the language the page is showing.
    //
    // THE FOUR PRESET-BAR NAMES ARE THE DELETED title= TEXT, MOVED, NOT
    // AUTHORED. v1.2.3 carried title="Previous preset", "Next preset", "Load
    // preset from file" and "Save preset"; contract §4 deletes the native
    // attribute (it renders a second, untranslated OS tooltip) and moves its
    // existing English into the accessible name. Every English string below is
    // byte-identical to what v1.2.3 shipped. Nothing new was invented.
    //
    // LABEL IN NAME. #preset-load and #preset-save carry BOTH a visible caption
    // and an aria-label, and an aria-label REPLACES the accessible name rather
    // than extending it. Each accessible name therefore CONTAINS its own
    // visible caption as a prefix — "Load" in "Load preset from file",
    // "CHARGER" in "Charger un préréglage depuis un fichier" — so a voice
    // control user saying the caption still hits the button (WCAG 2.5.3). This
    // is the constraint O-Texture's "Metal — coming soon" landed on from the
    // other direction.
    'aria.prevPreset': { en: { t: 'Previous preset' },        fr: { t: 'Préréglage précédent',                 reviewed: true } },
    'aria.nextPreset': { en: { t: 'Next preset' },            fr: { t: 'Préréglage suivant',                   reviewed: true } },
    'aria.loadPreset': { en: { t: 'Load preset from file' },  fr: { t: 'Charger un préréglage depuis un fichier', reviewed: true } },
    'aria.savePreset': { en: { t: 'Save preset' },            fr: { t: 'Enregistrer le préréglage',            reviewed: true } },

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
// is the one state the gate cannot tell a deliberate skip from a forgotten
// label (check-i18n assertion 14). NONE of the four below is in that state:
// no key on this page resolves to any of these strings, so all four are
// correctly unscoped and assertion 14 passes without one.
// ============================================================================

export const I18N_EXEMPT = [
    ['Ouaricon Chorus',
     'the product display name in .title — a product name is never translated, and this is the brand-plus-product form of the plugin\'s registered PRODUCT_NAME "O-Chorus" in CMakeLists.txt'],

    // ── D-02: the preset name IS the filename ───────────────────────────────
    ['Default',
     'the PRESET NAME shown in #preset-display, not a caption — D-02. The name is the JSON filename on disk (OuariconPresetManager sanitizes it into getUserPresetsDirectory()), and it is written into this node at runtime by the VENDORED modules/preset-manager.js, which is shared across plugins: localizing it here would rename presets in one language and orphan the files'],

    // ── Endonyms ────────────────────────────────────────────────────────────
    ['English',  'endonym — a language name is never translated'],
    ['Français', 'endonym — a language name is never translated'],
];

// ============================================================================
// TIP_BINDINGS — [selector, key, wrapper]
//
// applyI18n() does document.querySelector(selector), then closest(wrapper) when
// a wrapper is given, and writes data-tip-title + data-tip onto whatever that
// lands on. The wrapper exists so the ANCHOR is the cell a user aims at rather
// than the addressable child inside it.
//
// BOTH HALVES OF T17'S "BIND TO THE IDS THE UI ALREADY USES" ARE FALSE HERE,
// and they are false for different reasons, so they were checked separately.
//
//   THE ID HALF. Not one of the eight knobs carries an id. The only id inside
//   a knob is on the SVG arc — id="vine-rate" and friends — and .knob-vine is
//   `fill: none` with `stroke-width: 3`, so under SVG's default
//   pointer-events: visiblePainted only the PAINTED STROKE is hittable. Walked
//   pixel by pixel over one cell with elementFromPoint: 147 of 4526 points
//   inside the .knob-container land on #vine-rate. 3.2 %. And the painted
//   length is stroke-dashoffset, which the knob rewrites on every value
//   change — so the size of that 3.2 % target moves with the parameter. A tip
//   bound there is a tip nobody can open. What the markup gives instead is
//   .knob[data-param="..."], so every knob binding below is an attribute
//   selector.
//
//   THE WRAPPER HALF. .knob-container, NOT .knob, and it is load-bearing
//   rather than tidiness: measured at 700 x 125, .knob is 50 x 73 and its
//   container is 62 x 73, in BOTH languages. The 6 px of cell either side is
//   exactly where a pointer arriving from the neighbouring knob crosses, and
//   binding .knob would open and close the tip in that gap. The container is
//   also the box the caption's own width belongs to.
//
// THE TWO CHROME ANCHORS TAKE NO WRAPPER, AND THAT IS DELIBERATE. #gear-btn
// and #settings-popover share one ancestor, .settings-cluster; a wrapper walk
// from #lang-select would climb past the popover into that cluster and resolve
// to the GEAR's anchor, so hovering the selector would show the gear's tip
// (O-Comp hit exactly this). #gear-btn is a 20 x 20 button that IS its own
// hover target, and #lang-select is the select itself. Wrapping the select in
// .settings-row instead would make the caption LANGUAGE share one tip with it
// across 152 px of an already-open 170 px panel, firing while the pointer was
// merely crossing the panel to reach the selector.
//
// EVERY SELECTOR HERE IS ASSERTED TO RESOLVE by tests/ui_tip_render_check.js.
// applyI18n's own `i18n: tip target not found` is a console.warn, which
// boot-all-uis reports and nothing fails on.
// ============================================================================

export const TIP_BINDINGS = [
    ['.knob[data-param="rate"]',   'tip.rate',   '.knob-container'],
    ['.knob[data-param="depth"]',  'tip.depth',  '.knob-container'],
    ['.knob[data-param="voices"]', 'tip.voices', '.knob-container'],
    ['.knob[data-param="spread"]', 'tip.spread', '.knob-container'],
    ['.knob[data-param="width"]',  'tip.width',  '.knob-container'],
    ['.knob[data-param="tone"]',   'tip.tone',   '.knob-container'],
    ['.knob[data-param="mix"]',    'tip.mix',    '.knob-container'],
    ['.knob[data-param="drive"]',  'tip.drive',  '.knob-container'],
    ['#gear-btn',                  'tip.settings'],
    ['#lang-select',               'tip.language'],
];

// The tooltip lookup. Returns {t, b} — never null, never a bare key without a
// console.warn saying so, because a silently-missing tip renders as an empty
// surface that looks like a positioning bug rather than a missing entry.
//
// LIVE as of v1.4.0: applyI18n() calls it once per TIP_BINDINGS row, on every
// language change, and the ten rows above are no longer zero. It is exported
// verbatim all the same, so the canon block stays byte-identical to the other
// forty-two copies.
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
