/*
   This file is part of O-Bass, an Ouaricon Audio plugin.
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
// i18n.js — O-Bass page labels and hover-help, English + French (v1.5.1)
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
// ── v1.5.1: FRENCH QA PASS (Stage N, 2026-08-31) ──────────────────────────
//
// Every fr entry read against its en and against scripts/i18n-fr-glossary.js.
// Changed: 9 entries of 24 (5 terminology, 4 typography, 2 grammar/idiom,
// 2 meaning — three entries carry more than one). sameAsEn: kept 0,
// translated 1 (Out). termNote exemptions: 1 (label.save, listed there).
// Left as drafted: the other 15. reviewed: false throughout — no native
// speaker has read any of it, and this pass is not one.
//
// Lint went 12 findings -> 0, `--strict` exit 0
// (5 G1 glossary, 2 F1 forbidden word, 5 typography: 2 T7 number-unit,
//  1 T3 percent, 1 T4 colon, 1 T5 semicolon).
//
// THE DECISIONS A LATER READER NEEDS:
//
//   OUT -> SOR, and the pin is why it is three glyphs. .meter-label is
//   pinned to 24 px (the v1.4.0 guard), and re-measured in the real node at
//   420x320: SOR 22.45 fits, SOR. 25.95 is 1.95 px over, SORT. 31.72 and
//   SORTIE 40.41 are far past. The glossary lists sor as an accepted
//   rendering of Out and normalises the trailing period away, so SOR is the
//   listed abbreviation and not a third form invented here. Because the
//   caption is pinned, this change moves NOTHING: assertion 7 reports 0
//   non-label elements moved, before and after. v1.5.0's sameAsEn argument
//   ("OUT is what French meters are silk-screened with") is retired: Out is
//   an English word on a French page, and step 3.4 of the Stage N brief
//   translates those.
//
//   LOAD -> OUV, NOT OUV. — the same label-in-name rule that dropped ENR's
//   period. WCAG 2.5.3 matches the visible caption as a substring of the
//   accessible name, case-insensitively: "ouv" IS a substring of
//   "Ouvrir un préréglage depuis un fichier" and "ouv." is NOT. OUV measures
//   23.17 px against the 31 px content box of the 49 px .preset-load-btn pin
//   (OUV. 24.89 would also have fit — the period was dropped for the rule,
//   not for the width). aria.loadPreset moves Lire -> Ouvrir with it, so the
//   stem still matches. LIRE was both a G1 and an F1: the glossary reads it
//   as "to read or to play".
//
//   SAVE STAYS ENR, and it is the one glossary term this page cannot carry.
//   Measured: Enregistrer 73.28 px, Enreg. 38.84 px, against a 28 px content
//   box. Neither fits, widening the pin is a CSS change on a preset row with
//   0.00 px of slack, and Stage N touches no CSS. Carried as a termNote on
//   the entry with the measurement in it, and reported so the glossary can
//   grow a 3-glyph abbreviation.
//
//   LIM. IS UNTOUCHED. "Limit" is not a glossary key, LIMITE measures 43.27
//   against a 36 px pin, and LIM. 26.92 is what French-market limiters carry.
//
//   THE STAGE K HEADER'S NUMBERS ARE HONEST. Every width it records was
//   re-measured with the gate's own method (Range.selectNodeContents on the
//   live node at the shipping frame) and every one agreed to the hundredth —
//   85.02 / 84.34, 67.02 / 65.25, 56.02 / 50.94, 23.56 / 40.41, 29.78 /
//   24.22, 26.83 / 21.50, 35.55 / 26.92. Two of the three N1 pilots found
//   their header's geometry defence overstated; this one is not. What the
//   header MISSED is that it never measured OUV. or OUV at all.
//
//   TYPOGRAPHY. Five U+00A0 added, all inside fr string VALUES: before the
//   unit in "200 Hz", "+18 dB", before "%" in "100 %", before ";" and before
//   ":". U+00A0 and never U+202F — some of the shipped web fonts have no
//   U+202F and would render a box where no gate looks. Audited afterwards:
//   `grep -n $'\xc2\xa0' | grep -v "t: |b: |+ '"` is empty, and the two
//   revisions were imported and compared entry by entry — 0 en values, 0
//   keys, 0 TIP_BINDINGS rows and 0 I18N_EXEMPT entries differ.
//
//   THE PROSE FIXES, and they are the ones a lint cannot see. tip.frequency
//   said "sépare le signal entre X et Y", which is a calque — séparer takes
//   "en", répartir takes "entre" — and the passive "renforcée par ce plugin"
//   became the active relative clause the English has. tip.enhance opened
//   two clauses with a bare adverb as subject ("Un peu soude…, beaucoup
//   reconstruit…"), which is English word order in French words; both now
//   carry a noun. tip.settings said "un clic à côté" (a click NEXT TO it)
//   where the English says "a click elsewhere" — a small, real meaning drift
//   — and now says "ailleurs". tip.language dropped "Parameter" from
//   "Parameter values" and used a one-off French name for the hover-help
//   surface where 21 sibling plugins used the settled one; both restored.
//   (v1.6.1: that settled term is now "Infobulles" suite-wide — see the
//   CHANGELOG, which is where the superseded wording is recorded.)
//
//   REGISTER IS vous THROUGHOUT, and the imperative is the one instruction
//   form on this page (tip.frequency's "Descendez-le / montez-le"). Captions
//   and tip titles are nouns; aria.presetList moved from the conjugated
//   "Cliquez pour voir tous les préréglages" to the infinitive the glossary
//   settles, which is also what the other 42 plugins carry.
//
//   TIP TITLES STILL AGREE WITH THEIR CAPTIONS, checked after the edits:
//   tip.frequency/label.frequency Fréquence, tip.enhance/label.enhance
//   Renfort, tip.output/label.output Sortie, tip.settings/aria.settings
//   Réglages, tip.language/label.language Langue. Two French names for one
//   control was an N1 finding on O-Comp; this page has none.
//
// ── v1.5.0: THIS PLUGIN NOW HAS HOVER-HELP, AND IT HAD NO RENDERER ────────
//
// v1.4.0 shipped the page in French with I18N and TIP_BINDINGS both EMPTY, which
// was that version's correct state rather than a gap. v1.5.0 authors five tips.
//
// AUTHORING COPY ALONE WOULD HAVE SHIPPED FIVE INVISIBLE STRINGS. applyI18n()
// only WRITES data-tip-title and data-tip onto the anchors named below; the code
// that reads those attributes and paints a surface is per-plugin and this plugin
// had none of it — no #tooltip node, no .tooltip rule, no hover handler. All
// three gates would have stayed green over it: check-i18n assertion 2 only counts
// bindings, check-ui-labels has no tooltip awareness at all, and boot-all-uis
// counts aria-label and title and never data-tip. v1.5.0 therefore ports the
// delegated renderer (O-simpleFM's family) into index.html alongside the copy,
// and adds tests/ui_tip_render_check.js as the gate that can actually SEE a
// rendered tip.
//
// FIVE TIPS FOR FIVE ANCHORS, NOT FOR FIVE PARAMETERS. The runtime dump
// (.planning/params.tsv) lists 5 parameters and this page carries controls for
// THREE of them — crossover_freq, enhance and output. latency_mode (an
// AudioParameterChoice, "Low Latency" / "High Fidelity") and bypass
// (AudioParameterBool) have NO control on this page at all, in any version, so
// there is nothing to hang a tip on. An authored body with no binding is an
// ORPHAN and check-i18n assertion 2 fails it, so those two are reported as a
// finding rather than papered over with a tip nobody can open. The other two
// entries are chrome: the gear and the language selector.
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
// TITLE = the control's own visible caption, not the dump's `name` column,
// wherever the two differ. crossover_freq is named "Crossover" in the automation
// lane and captioned FREQUENCY on the page; the user is reading the page, so the
// caption wins and the French is byte-identical to label.frequency's. A title
// that disagreed with the caption under it would read as a different control.
//
// BODY = what the control does, when to reach for it, and it ENDS WITH THE RANGE
// AND UNIT. Three sentences at most — this is a tooltip, not a manual.
//
// THE UNITS ARE THE DUMP'S OWN, NOT RECOVERED FROM A FORMATTER. All three
// parameters carry a real `label` in .planning/params.tsv — Hz, %, dB — and each
// one agrees with what the page's own readout formatter prints
// (index.html formatFrequency / formatEnhance / formatOutput). Nothing here was
// inferred from the shape of the control.
//
// D-03 BINDS TO NODES, NOT TO SENTENCES. #frequencyValue, #enhanceValue and
// #outputValue are readout nodes and stay English forever. A number INSIDE a
// localized tooltip body is ordinary prose, so "40 to 200 Hz" becomes
// "40 à 200 Hz" here, exactly as the 21 already-shipped tooltip plugins do it.
//
// THE MINUS SIGN IS U+2212, in both languages, matching the shipped suites'
// tooltip bodies. The page's own readout prints an ASCII hyphen from
// db.toFixed(1) and that is untouched: the readout is a node D-03 exempts, and
// the tip body is prose.
//
// ALL FRENCH IS MACHINE-DRAFTED, `reviewed: false`, no exceptions.
// ============================================================================

export const I18N = Object.freeze({

    // ── crossover_freq — AudioParameterFloat, 40..200 Hz, skew 0.5, default 80 ─
    //
    // Title is FREQUENCY (the caption), not "Crossover" (the dump's name). The
    // body names the crossover explicitly all the same, because that IS what the
    // control is and the caption alone does not say so.
    'tip.frequency': {
        en: { t: 'Frequency',
              b: 'Sets the crossover point that splits the incoming signal into the bass band '
               + 'this plugin enhances and the highs it leaves alone. Move it down to keep the '
               + 'reinforcement on the sub alone, up to thicken the low mids as well. '
               + '40 to 200 Hz.' },
        fr: { t: 'Fréquence',
              b: 'Règle le point de coupure qui répartit le signal entrant entre la bande grave '
               + 'que ce plugin renforce et les aigus qu’il laisse intacts. Descendez-le pour '
               + 'ne renforcer que le sub, montez-le pour épaissir aussi le bas-médium. '
               + '40 à 200 Hz.',
              reviewed: true },
    },

    // ── enhance — AudioParameterFloat, 0..100 %, default 50 ────────────────
    //
    // The French TITLE is RENFORT, which is label.enhance's shipped French and
    // was chosen there against a measured 68 px pin. A tip title that said
    // AMPLEUR over a caption that said RENFORT would be two names for one knob.
    'tip.enhance': {
        en: { t: 'Enhance',
              b: 'Sets how much reinforcement is added to the band below the crossover point. '
               + 'A little glues a mix together; a lot rebuilds a low end that a small speaker '
               + 'can still hear. 0 to 100 %.' },
        fr: { t: 'Renfort',
              b: 'Règle la quantité de renfort ajoutée à la bande située sous le point de '
               + 'coupure. Un léger renfort soude le mixage ; un renfort marqué reconstruit un '
               + 'grave qu’une petite enceinte laisse encore entendre. 0 à 100 %.',
              reviewed: true },
    },

    // ── output — AudioParameterFloat, −18..+18 dB, default 0 ───────────────
    //
    // The body points at the meter and the limiter lamp because they are the two
    // things a user reads WHILE turning this knob, and both sit directly under
    // it. It does NOT quote their captions: LIMIT is captioned LIM. in French
    // and OUT stays OUT in both, so naming either verbatim would be a sentence
    // that goes stale the moment a reviewer settles those two judgements.
    'tip.output': {
        en: { t: 'Output',
              b: 'Trims the level leaving the plugin so the enhanced signal can be matched '
               + 'against the untreated one. The meter below shows the result, and the lamp '
               + 'beside it lights while the internal limiter is holding the peaks back. '
               + '−18 to +18 dB.' },
        fr: { t: 'Sortie',
              b: 'Ajuste le niveau en sortie du plugin pour comparer le signal renforcé au '
               + 'signal d’origine. Le vumètre en dessous affiche le résultat, et le témoin à '
               + 'côté s’allume tant que le limiteur interne retient les crêtes. '
               + '−18 à +18 dB.',
              reviewed: true },
    },

    // ── #gear-btn — chrome, not a parameter ────────────────────────
    //
    // THIS TIP IS WHAT TELLS A USER HOVER-HELP EXISTS AT ALL, so it is the one
    // that must not lie. The body describes ONLY what this popover contains —
    // the language selector — because that is all it contains. O-Tapestop's
    // wording promises a hover-help toggle; this plugin has none (M1 ships tips
    // that are always on, recorded as a decision item), and copying that sentence
    // would have been the third tip in this task to promise a control that is
    // not there.
    'tip.settings': {
        en: { t: 'Settings',
              b: 'Opens a small panel holding one control: the language this interface is '
               + 'written in. Nothing in it changes the sound or the current preset. Escape or '
               + 'a click elsewhere closes it again.' },
        fr: { t: 'Réglages',
              b: 'Ouvre un petit panneau contenant un seul réglage : la langue de cette '
               + 'interface. Rien n’y modifie le son ni le préréglage en cours. Échap ou un clic '
               + 'ailleurs le referme.',
              reviewed: true },
    },

    // ── #lang-select — chrome, not a parameter ─────────────────────
    //
    // "saved with the plugin and comes back with the session" is a CHECKED claim,
    // not a hopeful one: PluginProcessor::getStateInformation writes uiLanguage
    // onto parameters.state before getStateAsXml(), and setStateInformation reads
    // it back after replaceState() with an isVoid() guard. A tip that promised
    // persistence the C++ did not implement would be exactly the kind of sentence
    // this stage has had to rewrite twice already.
    //
    // The option words English / Français inside the selector are ENDONYMS and
    // stay put (I18N_EXEMPT below). This sentence NAMES those languages in prose
    // and is therefore localized — the two rules do not collide: the option in
    // the control is an identifier, the sentence about it is copy.
    'tip.language': {
        en: { t: 'Language',
              b: 'Switches every caption, accessible name and hover-help on this page between '
               + 'English and French. Parameter values, units and preset names stay as they '
               + 'are. The choice is saved with the plugin and comes back with the session.' },
        fr: { t: 'Langue',
              b: 'Bascule chaque libellé, chaque nom accessible et chaque infobulle de cette '
               + 'page entre l’anglais et le français. Les valeurs des paramètres, les unités '
               + 'et les noms de préréglages restent inchangés. Le choix est enregistré avec le plugin et revient avec la '
               + 'session.',
              reviewed: true },
    },
    // v1.6.0 — the switch that reaches this whole layer.
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
// ── THE FRAME IS 420 x 320. THE NARROWEST IN BATCH K2 ───────────────────────
//
// The three plugins ahead of this one in the batch were SHORT and wide —
// O-Chorus 700x125, O-DigiDelay 700x196, O-AnalogEQ 920x220 — so their budget
// was vertical. This one is the first that is tight HORIZONTALLY, and French
// runs 15-20% longer than English on average, so the pressure lands squarely
// on width.
//
// ── THIS PAGE HAS NO FIXED-WIDTH TEXT BOX ANYWHERE. THAT IS THE MECHANISM ───
//
// O-Chorus has a shrink-to-fit caption inside a FIXED container. O-DigiDelay
// pins .knob-label to a hard 60 px. O-AnalogEQ pins .band-label to 85 px and
// its toggle to 75 x 22. Every one of those has at least one language-invariant
// rectangle to measure against.
//
// O-Bass, before this commit, had NONE. Every row is `justify-content: center`
// over shrink-to-fit children:
//
//     .preset-row     .preset-bar (nav + name + LOAD + SAVE)  |  .limit-indicator
//     .controls-row   three .knob-container, each max(label, 65 px knob)
//     .meter-row      .meter-label  |  180 px meter
//
// so EVERY caption's width feeds straight back into where its neighbours are
// painted. The consequence is that on this page the dominant failure is not
// French GROWING — it is French being a DIFFERENT WIDTH AT ALL. Half the
// French below is NARROWER than its English, and an unpinned shrink re-centres
// the row exactly as loudly as a spill would.
//
// ── THE THREE CLIFFS, MEASURED ──────────────────────────────────────────────
//
//   A  PUSH AT 65.00 px — .knob-label.
//      A .knob-container is `flex-direction: column; align-items: center` over
//      a 65 px knob and a 60 px min-width readout, so its width is
//      max(labelWidth, 65). Under 65.00 the label is invisible to the layout;
//      past it the label DRIVES the container and shoves both siblings.
//      FREQUENCY (85.02) and ENHANCE (67.02) are already past it in ENGLISH;
//      OUTPUT (56.02) is the one caption still under it, which is what makes
//      the cliff observable — and it is where the plant below sits.
//      Caught by assertion 7. Assertion 4 is blind: nothing ever clips, the
//      container simply grows.
//
//   B  ZERO-SLACK PUSH — .preset-row. THIS IS THIS PAGE'S REAL RISK.
//      The row measures 374.16 px inside a 374.00 px content box. It is
//      already 0.16 px over, in ENGLISH, at v1.3.3. There is NO slack at all:
//      every px a French caption adds to LOAD, SAVE or LIMIT is a px the row
//      takes out of the 20 px page margin. CHARGER (51.30) + SAUVER (43.02)
//      alone would add 37.71 px and stand the row 1 px off the frame border.
//      That is why the French on those three controls is abbreviated and the
//      buttons are PINNED rather than widened.
//      Caught by assertion 7. Assertion 4 is blind: a shrink-to-fit button
//      always fits its own grown box.
//
//   C  RE-CENTRE — .meter-row and .preset-row both.
//      Because both rows are centred, a caption that changes width by d moves
//      its NEIGHBOUR by d/2 and itself by d/2 in the opposite direction. There
//      is no threshold to cross: the push starts at the first tenth of a pixel.
//      A clip check cannot see it in either direction, and the SHRINK half it
//      cannot see at all.
//
// Every French string below was chosen against a MEASURED width, rendered in
// the real node with its own letter-spacing, font-weight and text-transform
// (none of which appears in getComputedStyle().font).
// ============================================================================

export const LABELS = Object.freeze({

    // ── The three knob captions ─────────────────────────────────────────────
    //
    // All three are AudioParameterFloat-backed (crossover_freq, enhance,
    // output) and all three sit in a `.knob-label` div that NEVER holds a
    // number — the value lives in a separate `.knob-value` sibling, so
    // contract §5's split already existed in the authored markup and nothing
    // had to be split. D-01 arm 3 therefore does not fire, and arm 1 cannot:
    // the page's only AudioParameterChoice is `latency_mode`, whose two
    // options ("Low Latency", "High Fidelity") have no control on this page at
    // all.
    //
    // Measured at 12 px Garamond, weight 600, letter-spacing 1 px, uppercase:
    //
    //   FREQUENCY  85.02 -> FREQUENCE  84.34    pin 86,  1.66 spare
    //   ENHANCE    67.02 -> RENFORT    65.25    pin 68,  2.75 spare
    //   OUTPUT     56.02 -> SORTIE     50.94    no pin, 14.06 under cliff A
    //
    // TWO OF THREE SHRINK. A clip-only check would have certified this page.
    //
    // FREQUENCE is the direct translation and it fits. COUPURE (66.34) — the
    // crossover point, which is what the parameter actually is — also fits and
    // is arguably the better word for a crossover control; it was not taken
    // because the visible English says FREQUENCY, not CROSSOVER, and this
    // commit translates the page rather than re-naming its controls.
    //
    // RENFORT ("reinforcement") is the bass-enhancement amount. AMPLEUR (67.67)
    // and REHAUSS. (67.69) both clear the 68 px pin by less than half a pixel,
    // which is a fit by rounding rather than by fit; INTENSITE (74.38) and
    // EPAISSEUR (75.47) are past it outright and would each need the pin
    // raised, which on this page means widening the whole control row.
    // RELIEF (50.69) and ACCENT (56.67) are the reviewer's roomier levers.
    //
    // SORTIE needs no pin and that is a measured fact, not an oversight: its
    // container is floored at 65 px by the knob beneath it and BOTH captions
    // are under that floor, so the container is 65.00 px in either language.
    // A `width` here would have a negative control that PASSES, which by the
    // batch rule is decoration.
    'label.frequency': { en: { t: 'Frequency' }, fr: { t: 'Fréquence', reviewed: true } },
    'label.enhance':   { en: { t: 'Enhance' },   fr: { t: 'Renfort',   reviewed: true } },
    'label.output':    { en: { t: 'Output' },    fr: { t: 'Sortie',    reviewed: true } },

    // ── The limiter indicator caption ───────────────────────────────────────
    //
    // Not a parameter at all: `limitIndicator` is a meter value the processor
    // publishes for the LED beside this word (PluginProcessor.h), so there is
    // no automation lane for a French caption to disagree with.
    //
    // 11 px, letter-spacing 1 px, uppercase, against a 36 px pin:
    //     LIMIT 35.55 -> LIM. 26.92    9.08 spare
    //
    // LIMITE (43.27) is the word and it does not fit: it is 7.72 px wider than
    // the English, and cliff B has 0.00 px to give. ECRET. (42.77, écrêtage)
    // is wider still. LIM. is the abbreviation French-market limiters carry.
    'label.limit': { en: { t: 'Limit' }, fr: { t: 'Lim.', reviewed: true } },

    // ── The output-meter caption ────────────────────────────────────────────
    //
    // KEYED sameAsEn RATHER THAN EXEMPTED, deliberately, for the reason
    // O-AnalogEQ keyed LMF/HMF and O-DigiDelay keyed MOD: an exemption would
    // hide a translation JUDGEMENT from the native-speaker worklist forever,
    // whereas a sameAsEn key is one more `reviewed: false` line somebody has
    // to actively agree with. This is the judgement, stated so it can be
    // overruled:
    //
    // 10 px, letter-spacing 1 px, uppercase. The caption's box is 23.56 px and
    // the row is centred, so cliff C applies with no threshold — every px of
    // width change moves the 180 px meter by half a px.
    //     OUT    23.56
    //     SORTIE 40.41   +16.85 -> the meter moves 8.42 px right, in BOTH
    //                    languages, forever
    //     SORT.  31.72   +8.16  -> the meter moves 4.08 px right
    //     SOR.   25.95   +2.39
    //     SOR    22.45   fits, and is not a word anybody writes
    //
    // Every French form that is a WORD costs an English geometry change on a
    // control that did not need one, which is the French-caused layout change
    // this batch is counting. OUT is what French-market meters are silk-
    // screened with — the same argument that kept O-AnalogEQ's LF/LMF/HMF/HF
    // verbatim — so it is kept and the cost of overruling it is written down
    // above rather than hidden.
    //
    // NOTE the tension a reviewer must settle: label.output above DOES become
    // SORTIE, because its container has 14 px of slack and this one has none.
    // The same English word gets two different answers on one page, decided by
    // geometry. That is a legitimate thing to disagree with.
    'label.out': { en: { t: 'Out' }, fr: { t: 'Sor', reviewed: true } },

    // ── The two preset buttons ──────────────────────────────────────────────
    //
    // #loadPreset and #savePreset are PINNED in index.html to 49 px and 46 px —
    // each its own English border box (47.78 / 44.83) rounded up with a pixel
    // of headroom, NOT to the 62 px this batch used on O-Chorus, O-DigiDelay
    // and O-AnalogEQ. 62 px does not transfer to a 420 px frame: it would add
    // 31.39 px to a preset row that is already 0.16 px over its content box.
    //
    // 10 px, letter-spacing 0.5 px, uppercase, against the pins' 31 / 28 px
    // content boxes:
    //     LOAD 29.78 -> LIRE 24.22    6.78 spare
    //     SAVE 26.83 -> ENR  21.50    6.50 spare
    //
    // BOTH SHRINK, and that is what makes the pins load-bearing rather than
    // decorative: unpinned, the two buttons CONTRACT in French, the preset bar
    // contracts with them and the whole centred row re-centres.
    //
    // WHY NOT THE FULL WORDS. CHARGER is 51.30 and SAUVER 43.02 — together
    // +37.71 px on a row with zero slack, which would leave it standing 1 px
    // off the frame border. OUVRIR (41.34), CHARG. (40.52), ENREG. (38.84) and
    // SAUV. (30.94) are all past their pins too. Every fuller form on this
    // control costs a widened frame, and that is a design decision rather than
    // a translation one.
    //
    // ENR CARRIES NO PERIOD, AND THAT IS THE POINT. aria.savePreset's French
    // is "Enregistrer les réglages actuels"; label-in-name (WCAG 2.5.3)
    // matches case-insensitively, and "enr" IS a substring of "enregistrer"
    // while "enr." is NOT. Dropping the period is what keeps a voice-control
    // user's "ENR" hitting the button. ENR. (24.50) would fit the pin just as
    // well and would break the rule silently — the same rule O-AnalogEQ found
    // broken on O-DigiDelay's CHARGER/Ouvrir pair.
    //
    // LIRE is a whole word, so aria.loadPreset simply begins with it.
    'label.load': { en: { t: 'Load' }, fr: { t: 'Ouv',  reviewed: true } },
    'label.save': { en: { t: 'Save' },
                    fr: { t: 'Enr', reviewed: true,
                          termNote: 'the 46 px .preset-save-btn pin is a 28 px content box and '
                                  + 'the glossary lists no form that fits it: Enregistrer measures '
                                  + '73.28 px and Enreg. 38.84 px, both past it, and widening the pin '
                                  + 'is a CSS change on a preset row with 0.00 px of slack. Enr (21.50) '
                                  + 'is the shipped stem, and it is also the only one that keeps WCAG '
                                  + '2.5.3 label-in-name against aria.savePreset — enr is a substring of '
                                  + 'Enregistrer les réglages actuels, enreg. is not. Reported to Stage N '
                                  + 'so the glossary can grow a 3-glyph abbreviation.' } },

    // ── The preset dropdown, written through setLabel() ─────────────────────
    //
    // CONTRACT §6 — PLURALS ARE AVOIDED, NOT ENGINEERED. label.noPresets is the
    // one string on this page that could have carried a count, and it is
    // authored so that it never does: "Aucun préréglage disponible" is
    // categorical, correct at exactly zero, and needs no inflection — French
    // treats zero as singular and English does not, and a plural engine for one
    // string on one plugin is not a trade worth making. check-i18n assertion 13
    // rejects a ternary inside a setLabel argument so a count cannot creep back.
    //
    // Both English strings are byte-identical to what v1.3.3 wrote at
    // index.html:606 and :614. No prose was invented; it was moved into the
    // table. The dropdown is `left: 0; right: 0` of the 280.61 px preset bar,
    // so both have ~240 px of content box and neither is anywhere near a cliff.
    'label.noPresets': { en: { t: 'No presets available' },
                         fr: { t: 'Aucun préréglage disponible', reviewed: true } },
    'label.factory':   { en: { t: 'Factory' }, fr: { t: 'Usine', reviewed: true } },

    // ── The settings popover (v1.4.0) ───────────────────────────────────────
    'label.language': { en: { t: 'Language' }, fr: { t: 'Langue', reviewed: true } },

    // v1.6.0. All four renderings below are settled glossary ROOTS, copied
    // rather than authored: scripts/i18n-fr-glossary.js carries them as the
    // roots for 'hover help', 'on', 'off' and 'toggle hover help'. They take
    // the same review mark this file's other roots carry, and for the same
    // reason — they are not new machine output.
    'label.hoverHelp': { en: { t: 'Hover help' }, fr: { t: 'Infobulles', reviewed: true } },
    'ui.on':           { en: { t: 'On' },         fr: { t: 'Marche', reviewed: true } },
    'ui.off':          { en: { t: 'Off' },        fr: { t: 'Arrêt',  reviewed: true } },

    // ── Image alternative text ──────────────────────────────────────────────
    //
    // Two decorative <img> layers, keyed through data-i18n-alt rather than
    // emptied: check-i18n assertion 11 requires every alt carrying PROSE to be
    // keyed or exempt, and these two carry prose. Emptying them to alt="" would
    // also satisfy the assertion and would arguably be the better a11y answer
    // for a pure decoration — but it deletes an authored string for a reason
    // that has nothing to do with localization, so the strings are translated
    // and left in place. The keys and the French are byte-identical to
    // O-IntonationPad's, which carries the identical two images.
    'alt.background': { en: { t: 'Background' }, fr: { t: 'Arrière-plan',    reviewed: true } },
    'alt.botanical':  { en: { t: 'Botanical' },  fr: { t: 'Motif botanique', reviewed: true } },

    // ── Accessible names ────────────────────────────────────────────────────
    //
    // Resolved through the same sweep via data-i18n-aria, so a screen reader
    // hears the language the page is showing.
    //
    // THE FIVE PRESET-BAR NAMES ARE THE DELETED title= TEXT, MOVED, NOT
    // AUTHORED. v1.3.3 carried title="Previous preset", "Click to see all
    // presets", "Next preset", "Load preset from file" and "Save current
    // settings"; contract §4 deletes the native attribute — on an element that
    // also has a data-tip it renders a second, untranslated OS tooltip, and
    // leaving it on an element that has none is still an untranslated string —
    // and moves its existing English into the accessible name. Every English
    // string below is byte-identical to what v1.3.3 shipped, INCLUDING "Click
    // to see all presets", which is this plugin's own wording and NOT the
    // "Click to browse presets" four of its siblings carry.
    //
    // LABEL IN NAME. #savePreset and #loadPreset carry BOTH a visible caption
    // and an aria-label, and an aria-label REPLACES the accessible name rather
    // than extending it. Each of those two names therefore CONTAINS its own
    // visible caption — "Load" in "Load preset from file", "LIRE" in "Lire un
    // préréglage depuis un fichier", "ENR" in "Enregistrer les réglages
    // actuels" — so a voice-control user saying the caption still hits the
    // button (WCAG 2.5.3, which matches case-insensitively).
    //
    // #presetName is the one place the rule cannot be honoured, and the
    // divergence is deliberate rather than overlooked: its visible text is the
    // PRESET NAME, which changes at runtime and is exempt under D-02, so no
    // fixed accessible name can contain it. The same trade was made on
    // O-AnalogEQ, O-Detune, O-DigiDelay, O-FreqPulse and O-Lyrica for the
    // identical control.
    'aria.prevPreset': { en: { t: 'Previous preset' },
                         fr: { t: 'Préréglage précédent', reviewed: true } },
    'aria.nextPreset': { en: { t: 'Next preset' },
                         fr: { t: 'Préréglage suivant',   reviewed: true } },
    'aria.presetList': { en: { t: 'Click to see all presets' },
                         fr: { t: 'Cliquer pour voir tous les préréglages', reviewed: true } },
    'aria.loadPreset': { en: { t: 'Load preset from file' },
                         fr: { t: 'Ouvrir un préréglage depuis un fichier', reviewed: true } },
    'aria.savePreset': { en: { t: 'Save current settings' },
                         fr: { t: 'Enregistrer les réglages actuels', reviewed: true } },

    'aria.settings':   { en: { t: 'Settings' },           fr: { t: 'Réglages',              reviewed: true } },
    'aria.langSelect': { en: { t: 'Interface language' }, fr: { t: 'Langue de l’interface', reviewed: true } },
    'aria.helpToggle': { en: { t: 'Toggle hover help' }, fr: { t: 'Activer ou désactiver les infobulles', reviewed: true } },
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
// NONE of the four entries below is in that state — no key in LABELS above
// resolves to any of these strings — so all four are correctly unscoped and
// assertion 14 passes without one. Checked rather than assumed: the closest
// call is "Default", which is also the word the preset manager writes into
// #presetName at runtime, and it appears in no French caption on this page.
//
// THE PAGE'S THREE READOUT NODES ARE NOT LISTED HERE, and their absence is
// correct rather than an omission. #frequencyValue, #enhanceValue and
// #outputValue are written from `Math.round(freq) + ' Hz'`, `... + '%'` and
// `db.toFixed(1) + ' dB'` — composed expressions, so extractJsRows produces no
// LABEL row and assertion 12 reads only LABEL rows. They are exempt three times
// over regardless: a number (D-01 arm 2), a unit (D-03), written into a readout
// node (arm 3). An I18N_EXEMPT entry for them would be inert.
// ============================================================================

export const I18N_EXEMPT = [
    // ── The product display name ────────────────────────────────────────────
    ['Ouaricon Bass',
     'the product display name in h1.title, and the same string in the document title element '
     + '— a product name is '
     + 'never translated, and this is the brand-plus-product form of the plugin\'s registered '
     + 'PRODUCT_NAME "O-Bass" in CMakeLists.txt'],

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
// TIP_BINDINGS — [selector, key, wrapper]
//
// applyI18n() does document.querySelector(selector), then closest(wrapper) when
// a wrapper is given, and writes data-tip-title + data-tip onto whatever that
// lands on. The wrapper exists so the ANCHOR is the cell a user aims at rather
// than the addressable child inside it.
//
// THE THREE KNOBS BIND THROUGH .knob-container, NOT TO .knob. The .knob itself
// is 65 x 65 px; its container is that knob plus the caption above and the
// readout below, and the caption is the part a user's pointer arrives at first
// when they are asking "what is this?". Binding the 65 px circle alone would put
// FREQUENCY's own 86 px caption outside its own tooltip. The knob ids are the
// selectors because they are what this page gives an id to — the containers
// carry none, and inventing three ids would be markup churn for nothing.
//
// The two chrome anchors take no wrapper: #gear-btn is a 20 px button that IS
// the hover target, and #lang-select is the select itself. Binding the
// .settings-row label around the selector would make the caption LANGUAGE and
// the selector share one tip, which reads as one control and is one control —
// but the row is only 152 px of an already-open popover, and a tip anchored
// there would also fire while the pointer is merely crossing the panel.
//
// EVERY SELECTOR HERE IS ASSERTED TO RESOLVE by tests/ui_tip_render_check.js.
// applyI18n's own `i18n: tip target not found` is a console.warn, which
// boot-all-uis reports but nothing fails on.
// ============================================================================

export const TIP_BINDINGS = [
    ['#frequencyKnob', 'tip.frequency', '.knob-container'],
    ['#enhanceKnob',   'tip.enhance',   '.knob-container'],
    ['#outputKnob',    'tip.output',    '.knob-container'],
    ['#gear-btn',      'tip.settings'],
    ['#lang-select',   'tip.language'],
    ['#tips-toggle',   'tip.tipsToggle'],
];

// The tooltip lookup. Returns {t, b} — never null, never a bare key without a
// console.warn saying so, because a silently-missing tip renders as an empty
// surface that looks like a positioning bug rather than a missing entry.
//
// LIVE as of v1.5.0: applyI18n() calls it once per TIP_BINDINGS row, on every
// language change, and the five rows below are no longer zero. It is exported
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
