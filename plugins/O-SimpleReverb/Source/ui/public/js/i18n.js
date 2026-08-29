/*
   This file is part of O-SimpleReverb, an Ouaricon Audio plugin.
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
// i18n.js — O-SimpleReverb page labels, English + French (v1.6.0)
//
// An ES module that EXPORTS ONLY. It must never self-execute: a bare top-level
// statement here throws out of module evaluation and takes every later
// initializer on the page with it (pattern_module_toplevel_init_tdz). This
// plugin's controller is an inline <script type="module"> in index.html, so
// that failure mode would take the WHOLE UI — every knob, the preset bar, the
// VU meter — rather than a panel of it. check-i18n assertion 7 enforces it.
//
// FILENAME: no hyphen. juce_add_binary_data STRIPS hyphens rather than
// converting them to underscores, so a second file named i18n-fr.js would have
// to be reached as the symbol i18nfr_js (critical_binary_data_strips_hyphens).
// One combined file for both languages sidesteps the question entirely.
//
// ── THIS PLUGIN HAS NO HOVER-HELP, AND THIS COMMIT DOES NOT GIVE IT ANY ─────
//
// v1.5.7 carried no data-tip and no data-tooltip anywhere on the page — only
// four native title= attributes on the preset bar, which contract §4 DELETES
// rather than localizes, moving their existing text into data-i18n-aria. No
// hover-help prose is INVENTED here: authoring it is Stage M's job. I18N is
// therefore empty and TIP_BINDINGS is empty, which is this plugin's correct
// state rather than a gap. check-i18n assertion 2 reports it as "0 tip(s)
// bound", and the emptiness is only admissible BECAUSE no I18N entry carries a
// body — an emptied TIP_BINDINGS over a bodied table would be orphaned copy.
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
// I18N — hover-help copy. EMPTY, deliberately.
//
// A tooltip entry is {t, b}: a title and a body. This page has neither, so the
// table has no entries. It is exported all the same because the canonical
// import line names it and trLabel() falls back through it — a control whose
// tooltip title already IS its caption is meant to carry ONE key, and that
// fallback must exist even on a plugin with no tooltips today, so Stage M can
// add bodies here without touching the label keys below.
// ============================================================================

export const I18N = Object.freeze({});

// ============================================================================
// LABELS — the visible text of the page. {en:{t}, fr:{t, reviewed}}.
//
// One string per entry, no body: a label is not a tooltip.
//
// ── THE FRAME IS 500 x 350, AND THIS IS THE DENSEST PAGE IN BATCH K2 ────────
//
// 26 LABEL rows in 175 000 px² is the highest text-per-pixel density of the
// five tight frames — O-Chorus 15 in 87 500, O-DigiDelay 21 in 137 200,
// O-AnalogEQ 20 in 202 400, O-Bass 16 in 134 400. The plan claims "20 text"
// and undercounts by six.
//
// THE DENSITY IS PAID FOR BY A GRID, WHICH IS THIS PAGE'S GREAT ADVANTAGE.
// The six knobs sit in two CSS grids — `repeat(4, 1fr)` over 414 px of content
// and `repeat(3, 1fr)` over 380 px — so every knob column is a
// LANGUAGE-INVARIANT RECTANGLE by construction: 97.50 px on the top row,
// 124.66 px on the bottom. Four of the five K2 plugins had at most one such
// rectangle and O-Bass had none at all. Here there are seven, and they are
// what makes 26 labels in 500 x 350 cheap rather than expensive: a caption
// that changes width re-centres inside its own column and touches nothing
// outside it, up to the point where the column's own floor gives way.
//
// ── THE THREE CLIFFS, MEASURED ──────────────────────────────────────────────
//
//   A  RE-CENTRE AT 52.00 px — the `.knob` box.
//      A `.knob` is `column; align-items: center` over a 52 px `.knob-visual`
//      and a 50 px min-width `.knob-value`, so its width is
//      max(labelWidth, 52). UNDER 52.00 the caption is invisible to the
//      layout — the box is the knob's. PAST it the caption DRIVES the box, and
//      because `.knob-control` centres that box in its grid column, every
//      child of the knob — the visual, the SVG, the vine, the readout — moves
//      by half the width change, in either direction.
//      CHARACTER (61.72) is the ONLY caption past the cliff in English. The
//      other five are under it and therefore inert, which is what keeps the
//      cliff observable: a French string that crosses 52.00 announces itself.
//      Caught by assertion 7. Assertion 4 is blind — nothing clips, the box
//      simply grows.
//
//   B  ZERO-SLACK HEADER — .header, `justify-content: space-between`.
//      h1.title measures 163.58 and .preset-bar 290.42 inside a 454.00 px
//      content box. 163.58 + 290.42 = 454.00 EXACTLY. There is no slack at
//      all, and the title is ALREADY wrapped to two lines at that width. Every
//      px a French caption adds to SAVE or LOAD is a px taken out of the
//      title's box, and the title's next stop is a THIRD LINE — a vertical
//      push on a page whose content already overflows its own box by 3 px.
//      That is why SAVE and LOAD are abbreviated and PINNED.
//      Caught by assertion 7. Assertion 4 is blind: a shrink-to-fit button
//      always fits its own grown box.
//
//   C  GRID-TRACK SPILL AT 97.50 px — `repeat(4, 1fr)`.
//      A `1fr` track is `minmax(auto, 1fr)`, so its floor is min-content. A
//      caption whose LONGEST WORD exceeds 97.50 px raises that floor and the
//      column steals width from its three siblings — every knob on the row
//      moves. A caption with a break opportunity WRAPS instead and grows
//      downward. This is O-DigiDelay's single-word-spills / two-words-wrap
//      split, transposed onto a grid, and nothing shipped here goes anywhere
//      near it: the widest French below is 60.72.
//
// Every French string below was chosen against a MEASURED width, rendered in
// the real node with its own letter-spacing, text-transform and font stack
// (none of which appears in getComputedStyle().font).
// ============================================================================

export const LABELS = Object.freeze({

    // ── The six knob captions ───────────────────────────────────────────────
    //
    // All six are AudioParameterFloat-backed (CHARACTER, LPFREQ, WET, DRY,
    // DECAY, SIZE) and all six sit in a `.knob-label` span that NEVER holds a
    // number — the value lives in a separate `.knob-value` sibling, so
    // contract §5's split already existed in the authored markup and nothing
    // had to be split. D-01 arm 3 therefore does not fire on any of them.
    //
    // Measured at 9 px Garamond, letter-spacing 0.8 px, uppercase, against
    // CLIFF A at 52.00 px:
    //
    //   CHARACTER  61.72 -> CARACTÈRE  60.72   pin 62,  1.28 spare  TIGHTEST
    //   LOW CUT    46.19 -> COUPE-B.   47.16   no pin,  4.84 under the cliff
    //   WET        21.89 -> EFFET      30.52   no pin, 21.48 under the cliff
    //   DRY        20.91 -> DIRECT     37.31   no pin, 14.69 under the cliff
    //   DECAY      34.19 -> DÉCLIN     37.80   no pin, 14.20 under the cliff
    //   SIZE       22.20 -> TAILLE     35.58   no pin, 16.42 under the cliff
    //
    // ONLY THE FIRST IS PINNED, and only because it is the only one past the
    // cliff in English. Pinning any of the other five would have a negative
    // control that PASSES — both captions sit under a floor the knob owns, so
    // the box is 52.00 px in either language and a width there is decoration.
    // Leaving them unpinned is also what keeps cliff A observable.
    //
    // CARACTÈRE IS 1.00 px NARROWER THAN CHARACTER. That is the batch's
    // recurring finding rather than an accident — French shrinks at least as
    // often as it grows, and an unpinned shrink re-centres a column exactly as
    // loudly as a spill would. TIMBRE (38.81) and COULEUR (48.11) are the
    // reviewer's roomier levers; both are real French tone-control words and
    // both would sit 20+ px inside the pin. CARACTÈRE is kept because it is
    // the direct translation of the visible English and this commit translates
    // the page rather than re-naming its controls.
    'label.character': { en: { t: 'Character' }, fr: { t: 'Caractère', reviewed: false } },

    // COUPE-B. IS AN ABBREVIATION AND THE FULL FORM WAS REJECTED ON
    // MEASUREMENT, NOT ON TASTE. COUPE-BAS is 57.22 — 5.22 px PAST cliff A —
    // and would move every child of the LPFREQ knob 2.61 px in French only.
    // COUPE BAS (56.47) and PASSE-HAUT (62.19, the technically correct name
    // for a low cut) are worse. Under the cliff and available to a reviewer:
    // FILTRE (34.48), which is unambiguous because this page has exactly one
    // filter but drops the word "low"; C. BAS (32.81); PASSE-HT (47.59).
    // COUPE-B. keeps the low-cut meaning at 4.84 px of headroom.
    'label.lowCut':    { en: { t: 'Low Cut' },   fr: { t: 'Coupe-b.', reviewed: false } },

    // EFFET / DIRECT is the pair French effects units are silk-screened with,
    // and it is a pair — the reviewer should move both or neither. HUMIDE
    // (40.80) / SEC (18.91) is the literal translation and both members also
    // fit with room to spare, so this is a register choice rather than a
    // geometry one and is recorded as such. DIRECT is 16.40 px WIDER than DRY
    // and that costs nothing, because 37.31 is still 14.69 px under the cliff:
    // measuring the cliff is what turns a scary-looking growth into a non-event.
    'label.wet':       { en: { t: 'Wet' },       fr: { t: 'Effet',    reviewed: false } },
    'label.dry':       { en: { t: 'Dry' },       fr: { t: 'Direct',   reviewed: false } },

    // DÉCROISSANCE is the full word and it is 77.63 — a SINGLE WORD with no
    // break opportunity, 25.63 px past cliff A. It stops short of cliff C
    // (97.50) so it would not steal from the neighbouring columns, but it
    // would move every child of the DECAY knob 12.8 px. DÉCROISS. (52.97) is
    // past the cliff by 0.97 px, which is a miss by rounding. DÉCLIN is the
    // word French reverb manuals use and it fits with 14.20 px to spare;
    // CHUTE (34.00) is the roomier alternative.
    'label.decay':     { en: { t: 'Decay' },     fr: { t: 'Déclin',   reviewed: false } },
    'label.size':      { en: { t: 'Size' },      fr: { t: 'Taille',   reviewed: false } },

    // ── The low-cut ON/OFF toggle ───────────────────────────────────────────
    //
    // D-01 ARM 3 SAYS EXEMPT AND IS OVERRULED HERE, WITH REASONS — the same
    // overrule O-Gain made for LOW/MED/HIGH. #LPFREQ-value carries the class
    // `knob-value`, which on this page is the readout class, and arm 3 exempts
    // a readout node regardless of the parameter behind it. Four reasons to
    // key it anyway:
    //   1. THE NODE NEVER HOLDS A NUMBER. Every other `.knob-value` on this
    //      page does — "neutral", "50%", "1.0x" — and this one is always
    //      exactly one of two words. Keying it cannot make the element enter
    //      and leave the sweep as the knob turns, which is the failure arm 3
    //      exists to prevent (O-Marimba's six timbre words).
    //   2. IT IS A CONTROL, NOT A DISPLAY. It carries `toggle-btn`, an
    //      `.active` state and a click listener that flips LPON. The
    //      frequency it belongs to is displayed by the two `hz-label` spans
    //      (20 / 400), not here.
    //   3. ARM 1 CANNOT FIRE. LPON is an AudioParameterFloat over
    //      NormalisableRange(0, 1, 1), not an AudioParameterChoice, so there
    //      is no host automation-lane option string for a French caption to
    //      disagree with.
    //   4. THE EXTRACTOR ALREADY FOUND THEM. `ON` and `OFF` are two of this
    //      plugin's two js-prose LABEL rows, written from a ternary at
    //      index.html:991. Leaving them unkeyed needs an I18N_EXEMPT entry
    //      anyway, and assertion 12 matches an exemption by TEXT — so the
    //      cheap answer is also the one that hides a missed label.
    //
    // GEOMETRY: `.knob-value` is `min-width: 50px` with box-sizing: border-box
    // and `.toggle-btn` adds 6 px of side padding and a 1 px border, so the
    // content box is 32.00 px and the border box is floored at 50.00.
    //     ON   14.45 -> ACT.  21.77   10.23 spare, box stays 50.00
    //     OFF  18.36 -> DÉS.  21.41   10.59 spare, box stays 50.00
    // BOTH FRENCH FORMS ARE INERT: the border box does not move, so the knob
    // stays 52.00 px and nothing re-centres.
    //
    // MARCHE / ARRÊT REJECTED ON MEASUREMENT. MARCHE is 42.80, a 60.80 px
    // border box — 10.80 past the 50 px floor and 8.80 past cliff A — which
    // would move every child of the LPFREQ knob 4.40 px in French only. ARRÊT
    // (32.78) fits on its own; a pair where only one member fits is not a
    // pair. MAR. (25.30) / ARR. (23.06) both fit and are the reviewer's
    // alternative; ACT. / DÉS. is preferred because activé/désactivé is the
    // idiom for a filter ENABLE, where marche/arrêt is a power idiom.
    'ui.on':           { en: { t: 'ON' },        fr: { t: 'ACT.',     reviewed: false } },
    'ui.off':          { en: { t: 'OFF' },       fr: { t: 'DÉS.',     reviewed: false } },

    // ── The two preset buttons ──────────────────────────────────────────────
    //
    // #preset-save and #preset-load are PINNED in index.html to 41 px and
    // 44 px — each its own English border box (40.83 / 43.78) rounded up —
    // NOT the 62 px this batch used on O-Chorus, O-DigiDelay and O-AnalogEQ,
    // and not O-Bass's 49/46 either. Every one of those numbers belongs to its
    // own page: here the header has ZERO slack (cliff B), so the pins are
    // deliberately the smallest values that make each rectangle
    // language-invariant. Together they add 0.39 px to a row with nothing to
    // give, and the measured cost of that 0.39 px is nothing at all — the
    // title's two line boxes do not reflow.
    //
    // 10 px, letter-spacing 0.5 px, uppercase, against the pins' 27 / 30 px
    // content boxes:
    //     SAVE 26.83 -> ENR  21.50    5.50 spare
    //     LOAD 29.78 -> LIRE 24.22    5.78 spare
    //
    // BOTH SHRINK, which is what makes the pins load-bearing rather than
    // decorative: unpinned, the two buttons CONTRACT by 10.89 px between them,
    // .preset-bar contracts with them, and `space-between` slides the whole
    // preset cluster right while the title's box grows.
    //
    // WHY NOT THE FULL WORDS. SAUVER is 43.02 and CHARGER 51.30 — together
    // +37.71 px on a header with 0.00 px of slack, which would squeeze
    // h1.title from 163.58 to 125.87 and take "Ouaricon Simple Reverb" to a
    // THIRD line. SAUV. (30.94), ENREG. (38.84), CHARG. (40.52) and OUVRIR
    // (41.34) are all past their pins too.
    //
    // ENR CARRIES NO PERIOD, AND THAT IS THE POINT. aria.savePreset's French
    // is "Enregistrer un préréglage"; label-in-name (WCAG 2.5.3) matches
    // case-insensitively, and "enr" IS a substring of "enregistrer" while
    // "enr." is NOT. Dropping the period is what keeps a voice-control user's
    // "ENR" hitting the button. ENR. (24.50) would fit the pin just as well
    // and would break the rule silently — the same rule O-AnalogEQ found
    // broken on O-DigiDelay, and which O-Marimba already ships broken twice.
    // LIRE is a whole word, so aria.loadPreset simply begins with it.
    'label.save':      { en: { t: 'Save' },      fr: { t: 'Enr',      reviewed: false } },
    'label.load':      { en: { t: 'Load' },      fr: { t: 'Lire',     reviewed: false } },

    // ── The settings popover (v1.6.0) ───────────────────────────────────────
    //
    // LANGUAGE 63.55 -> LANGUE 47.11, MEASURED IN THIS PAGE'S OWN
    // .settings-label rather than inherited: O-Bass reports 55.31 -> 38.87 for
    // the same two words at the same declared font-size, letter-spacing and
    // text-transform, and the 8.24 px difference is the font stack resolving
    // differently. The DELTA is identical (-16.44) and it is the delta that
    // matters here, but copying a sibling's absolute number would have been a
    // wrong number that reads exactly like a right one.
    //
    // It SHRINKS, which is why .settings-popover carries a hard width: an
    // auto-width panel would contract in French and assertion 7 would report
    // the panel, its row and the select as moved.
    'label.language':  { en: { t: 'Language' },  fr: { t: 'Langue',   reviewed: false } },

    // ── Image alternative text ──────────────────────────────────────────────
    //
    // Two decorative <img> layers, keyed through data-i18n-alt rather than
    // emptied: check-i18n assertion 11 requires every alt carrying PROSE to be
    // keyed or exempt, and these two carry prose. Emptying them to alt="" would
    // also satisfy the assertion and is arguably the better a11y answer for a
    // pure decoration — but it deletes an authored string for a reason that
    // has nothing to do with localization. The keys and the French are
    // byte-identical to O-Bass's and O-IntonationPad's; the FILES differ
    // (img/paper.jpg + img/flora.png here, not botanical.png) but the authored
    // English alt text is the same two words.
    'alt.background':  { en: { t: 'Background' }, fr: { t: 'Arrière-plan',    reviewed: false } },
    'alt.botanical':   { en: { t: 'Botanical' },  fr: { t: 'Motif botanique', reviewed: false } },

    // ── Accessible names ────────────────────────────────────────────────────
    //
    // Resolved through the same sweep via data-i18n-aria, so a screen reader
    // hears the language the page is showing.
    //
    // THE FOUR PRESET-BAR NAMES ARE THE DELETED title= TEXT, MOVED, NOT
    // AUTHORED. v1.5.7 carried title="Previous preset", "Next preset", "Save
    // preset" and "Load preset"; contract §4 deletes the native attribute —
    // on an element that also has a data-tip it renders a second,
    // untranslated OS tooltip, and on an element that has none it is still an
    // untranslated string — and moves its existing English into the accessible
    // name. Every English string below is byte-identical to what v1.5.7
    // shipped. THIS PLUGIN'S WORDING IS ITS OWN: "Save preset" / "Load
    // preset", NOT O-Bass's "Save current settings" / "Load preset from file"
    // and not the "Click to browse presets" four siblings carry. Nothing was
    // harmonised.
    //
    // #preset-display CARRIES NO ACCESSIBLE NAME, and that is deliberate
    // restraint rather than an omission: v1.5.7 gave it no title=, so there is
    // no existing English to move, and inventing one is Stage M's job. Four
    // titles in, four aria names out.
    //
    // LABEL IN NAME. #preset-save and #preset-load carry BOTH a visible
    // caption and an aria-label, and an aria-label REPLACES the accessible
    // name rather than extending it. Each of those two names therefore
    // CONTAINS its own visible caption — "Save" in "Save preset", "ENR" in
    // "Enregistrer un préréglage", "Load" in "Load preset", "LIRE" in "Lire un
    // préréglage" — so a voice-control user saying the caption still hits the
    // button (WCAG 2.5.3, which matches case-insensitively).
    'aria.prevPreset': { en: { t: 'Previous preset' },
                         fr: { t: 'Préréglage précédent', reviewed: false } },
    'aria.nextPreset': { en: { t: 'Next preset' },
                         fr: { t: 'Préréglage suivant',   reviewed: false } },
    'aria.savePreset': { en: { t: 'Save preset' },
                         fr: { t: 'Enregistrer un préréglage', reviewed: false } },
    'aria.loadPreset': { en: { t: 'Load preset' },
                         fr: { t: 'Lire un préréglage',        reviewed: false } },

    'aria.settings':   { en: { t: 'Settings' },           fr: { t: 'Réglages',              reviewed: false } },
    'aria.langSelect': { en: { t: 'Interface language' }, fr: { t: 'Langue de l’interface', reviewed: false } },
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
// NONE of the twelve entries below is in that state. Checked rather than
// assumed: the keyed texts on this page are Character, Low Cut, Wet, Dry,
// Decay, Size, Save, Load, OFF and Language, and no entry below collides with
// any of them. All twelve are therefore correctly UNSCOPED and assertion 14
// passes without one. The closest calls are "Room", which is also a preset
// CATEGORY header the dropdown writes at runtime from a preset name — exempt
// under D-02 there too, and invisible to a static scan either way — and
// "Default", which is the same string in both roles.
//
// THE PAGE'S FIVE REMAINING READOUT NODES ARE NOT ALL LISTED HERE, and that is
// correct rather than an omission. #WET-value, #DRY-value, #DECAY-value and
// #SIZE-value are written from composed templates — `${(norm*100).toFixed(0)}%`
// and `${(...).toFixed(1)}x` — so extractJsRows produces no LABEL row for them
// and an entry would be inert. They are exempt three times over regardless: a
// number (D-01 arm 2), a unit (D-03), written into a readout node (arm 3).
// #CHARACTER-value IS listed, because its authored English is the bare word
// "neutral" and a static text node with no digits in it does survive the
// extractor's filter.
// ============================================================================

export const I18N_EXEMPT = [
    // ── The product display name ────────────────────────────────────────────
    ['Ouaricon Simple Reverb',
     'the product display name in h1.title, and the same string in the document title element '
     + '— a product name is never translated, and this is the brand-plus-product form of the '
     + 'plugin\'s registered PRODUCT_NAME "O-SimpleReverb" in CMakeLists.txt'],

    // ── D-02: the preset name IS the filename ───────────────────────────────
    ['Default',
     'the PRESET NAME shown in #preset-name, not a caption — D-02. The name is the JSON '
     + 'filename on disk (OuariconPresetManager sanitizes it into getUserPresetsDirectory()), '
     + 'and it is written into this node at runtime from getCurrentPreset(): localizing it '
     + 'would rename presets in one language and orphan the files. It is also the dropdown\'s '
     + 'fallback CATEGORY header for any preset whose name has no " - " prefix'],

    // ── D-01 arm 1: the six TYPE options are AudioParameterChoice strings ───
    //
    // PluginProcessor.cpp:157 declares
    //   juce::StringArray { "Booth", "Room", "Hall", "Spring", "Plate", "Ambient" }
    // and the six <option> texts in index.html are BYTE-IDENTICAL to it.
    // Byte-identity is the test: the page and the host automation lane must
    // agree about what the sixth reverb algorithm is called, and a French
    // <option> would leave a DAW writing "Ambient" into a lane the page reads
    // as "Ambiance". This is the arm-1 case in its purest form — a
    // WebComboBoxRelay drives #TYPE by INDEX, so the visible strings are the
    // only place the two representations can diverge.
    //
    // Unscoped, deliberately: none of the six is keyed anywhere on this page,
    // so no scope is required and demanding one would be noise. "Room" is the
    // only one that recurs at all — as a runtime preset-category header, which
    // is exempt in its own right under D-02 and which a static scan never sees.
    ['Booth',   'an AudioParameterChoice TYPE option VERBATIM (PluginProcessor.cpp:157) — D-01 arm 1'],
    ['Room',    'an AudioParameterChoice TYPE option VERBATIM (PluginProcessor.cpp:157) — D-01 arm 1'],
    ['Hall',    'an AudioParameterChoice TYPE option VERBATIM (PluginProcessor.cpp:157) — D-01 arm 1'],
    ['Spring',  'an AudioParameterChoice TYPE option VERBATIM (PluginProcessor.cpp:157) — D-01 arm 1'],
    ['Plate',   'an AudioParameterChoice TYPE option VERBATIM (PluginProcessor.cpp:157) — D-01 arm 1'],
    ['Ambient', 'an AudioParameterChoice TYPE option VERBATIM (PluginProcessor.cpp:157) — D-01 arm 1'],

    // ── D-01 arm 3: a readout node is never a [data-i18n] element ───────────
    ['neutral',
     'the authored English of #CHARACTER-value, a READOUT node — D-01 arm 3. The formatter at '
     + 'index.html:806 writes `warm 42%`, `bright 30%` or the bare word `neutral` into it '
     + 'depending on where CHARACTER sits relative to a +/-0.5 dead zone, so the node holds a '
     + 'NUMBER in two of its three states. Keying it would make the element enter and leave '
     + 'the language sweep as the knob turns — the exact failure arm 3 exists to prevent. '
     + 'Contrast #LPFREQ-value, which carries the same class, never holds a number, and IS '
     + 'keyed with the overrule written down in LABELS above'],

    // ── The footer wordmark and its version number ──────────────────────────
    ['Ouaricon Audio v1.5.5',
     'the company name plus a VERSION NUMBER (D-03) — neither half is translatable. NOTE for '
     + 'a human: this string is STALE. It is hard-coded and reads v1.5.5 while CMakeLists.txt '
     + 'declared VERSION 1.5.7 before this commit and 1.6.0 after it. Deliberately NOT fixed '
     + 'here: correcting it is a user-visible change unrelated to localization, and two '
     + 'siblings (O-DigiDelay, O-Tremolo) already solve it properly with an EMPTY span carrying '
     + 'id="versionLabel" that is filled at runtime, which is the repair this one wants rather '
     + 'than another hard-coded literal'],

    // ── Endonyms ────────────────────────────────────────────────────────────
    ['English',  'endonym — a language name is never translated'],
    ['Français', 'endonym — a language name is never translated'],
];

// ============================================================================
// TIP_BINDINGS — EMPTY. See the header: this plugin has no hover-help.
//
// Exported because the canonical import line names it and applyI18n() iterates
// it. A zero-length loop is the correct no-op; the alternative — omitting the
// export and editing the canon block to match — would put this plugin's copy of
// the runtime out of step with the other forty-two, which is the whole drift
// the canon gate exists to prevent.
// ============================================================================

export const TIP_BINDINGS = [];

// The tooltip lookup. Returns {t, b} — never null, never a bare key without a
// console.warn saying so, because a silently-missing tip renders as an empty
// surface that looks like a positioning bug rather than a missing entry.
//
// Unreferenced at runtime today: applyI18n() calls it only from the
// TIP_BINDINGS loop, which is empty. It is exported verbatim all the same, so
// that the canon block is byte-identical to the other forty-two copies and
// Stage M can add bodies to I18N without touching this file's shape.
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
