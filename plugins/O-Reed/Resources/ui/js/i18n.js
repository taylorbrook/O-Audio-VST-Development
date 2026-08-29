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
// i18n.js — O-Reed page labels, English + French (v1.2.0)
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
// ── THIS PLUGIN HAS NO HOVER-HELP, AND THIS COMMIT DOES NOT GIVE IT ANY ─────
//
// v1.1.0 carried NO data-tip, NO data-tooltip and — measured, not assumed —
// ZERO native title=, aria-label=, placeholder= and alt= attributes anywhere in
// index.html. The measured inventory's "0 attributes" column is correct, which
// is worth saying out loud because it is the one column that is a claim about
// absence. There was therefore nothing for contract §4 to delete and nothing to
// move into data-i18n-aria, and NO NEW PROSE WAS INVENTED. The two aria keys
// below belong to the two elements this commit ADDS (the gear button and the
// language selector) and match the O-Bassoon precedent verbatim.
//
// Authoring hover-help copy is Stage M's job. I18N is therefore empty and
// TIP_BINDINGS is empty, which is this plugin's correct state rather than a
// gap: check-i18n assertion 2 reports it as "0 tip(s) bound" instead of passing
// silently, and the emptiness is only admissible BECAUSE no I18N entry carries
// a body.
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
// I18N — hover-help copy. EMPTY, deliberately.
//
// A tooltip entry is {t, b}: a title and a body. This page has neither, so the
// table has no entries. It is exported all the same because the canonical
// import line names it and trLabel() falls back through it — a control whose
// tooltip title already IS its caption is meant to carry ONE key, and that
// fallback must exist even on a plugin that has no tooltips today, so Stage M
// can add bodies here without touching the label keys below.
// ============================================================================

export const I18N = Object.freeze({});

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
//       Damping       40.81 -> Amortis.      41.47
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
//       Feedback      44.94 -> Réinject.     44.41   SHRANK
//       Drone Pitch   58.77 -> Bourdon       43.03   SHRANK
//       Max Voices    52.66 -> Voix max      42.89   SHRANK
//
//   TWELVE of the 27 SHRINK. That is the half of the risk a clip check is
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
        fr: { t: 'Instrument', reviewed: false, sameAsEn: true },
    },
    'label.tab.tuning': { en: { t: 'Tuning' }, fr: { t: 'Accord', reviewed: false } },
    'label.tab.fx': {
        en: { t: 'FX' },
        fr: { t: 'FX', reviewed: false, sameAsEn: true },
    },

    // ── XY pad ──────────────────────────────────────────────────────────────
    // The pad title sits in an 872px block and cannot push anything.
    'label.xy.title': {
        en: { t: 'Instrument Morphing' },
        fr: { t: 'Morphing d’instrument', reviewed: false },
    },

    // See CLIFF 2. `Caract. perce` rather than the literal `Caractère de perce`
    // because the literal newly intersects the B.Sax marker; the abbreviation
    // matches this page's own caption style (Reed Hard., Inf. Sustain, Vib
    // Depth) and its marker hit set is identical to English's.
    'label.xy.axisX': {
        en: { t: 'Bore Character' },
        fr: { t: 'Caract. perce', reviewed: false },
    },
    'label.xy.axisY': {
        en: { t: 'Double Reed' },
        fr: { t: 'Anche double', reviewed: false },
    },

    // The two readout captions, SPLIT out of their value spans in this commit
    // (contract §5) so the caption can carry a key while the number beside it
    // stays a readout. French puts a space before a colon; U+00A0 keeps it from
    // being a line-break opportunity in a `flex-wrap: wrap` row.
    'label.xy.boreKey': { en: { t: 'Bore:' }, fr: { t: 'Perce :', reviewed: false } },
    'label.xy.reedKey': { en: { t: 'Reed:' }, fr: { t: 'Anche :', reviewed: false } },

    // ── Section headings ────────────────────────────────────────────────────
    // "la perce" is the French term for a wind instrument's bore, and it is used
    // consistently for every bore-derived caption below.
    'label.section.primary': {
        en: { t: 'Primary Controls' },
        fr: { t: 'Réglages principaux', reviewed: false },
    },
    'label.section.bore': {
        en: { t: 'Bore & Resonance' },
        fr: { t: 'Perce et résonance', reviewed: false },
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
        fr: { t: 'Coupe de la perce', reviewed: false },
    },
    'label.section.reed': { en: { t: 'Reed' }, fr: { t: 'Anche', reviewed: false } },
    'label.section.expression': {
        en: { t: 'Expression' },
        fr: { t: 'Expression', reviewed: false, sameAsEn: true },
    },
    // Same cliff, and this one was passing BY 0.09 PIXELS. "Conception sonore"
    // is 137.91 wide, ending at x=175.91 against the third collapsed knob
    // wrapper's left edge at 176.00 — a coin flip across a font-metric change,
    // and the gate was green only because the coin landed the right way up.
    // "Design sonore" is the term French audio actually uses, is 103.84 wide,
    // and ends at 141.84 with 34.16px of clearance.
    'label.section.soundDesign': {
        en: { t: 'Sound Design' },
        fr: { t: 'Design sonore', reviewed: false },
    },
    'label.section.voice': { en: { t: 'Voice' }, fr: { t: 'Voix', reviewed: false } },

    // ── Knob captions ───────────────────────────────────────────────────────
    // The caption under the knob, never the value beside it: `.knob-value` is a
    // separate node and stays untouched (contract §5).

    // KEYED even though "Breath" is byte-identical to a vibratoSource option.
    // See the D-01 note above: two different controls, one shared word. The
    // OPTION is the scoped I18N_EXEMPT entry; this is the caption.
    'label.knob.breath': { en: { t: 'Breath' }, fr: { t: 'Souffle', reviewed: false } },

    // The English caption is already the French word. sameAsEn: true is the
    // assertion that its identity is deliberate.
    'label.knob.embouchure': {
        en: { t: 'Embouchure' },
        fr: { t: 'Embouchure', reviewed: false, sameAsEn: true },
    },

    // reedHardness. "Dureté anche" is the literal form and measures 66.98 of the
    // 68px cap — 1.02px, inside the Windows-metric band. The head noun alone is
    // 34.86, and the knob sits between Embouchure and Reed Hard.'s own siblings
    // in the PRIMARY section where nothing else is a hardness.
    'label.knob.reedHard': { en: { t: 'Reed Hard.' }, fr: { t: 'Dureté', reviewed: false } },

    'label.knob.output': { en: { t: 'Output' }, fr: { t: 'Sortie', reviewed: false } },

    'label.knob.character': { en: { t: 'Character' }, fr: { t: 'Caractère', reviewed: false } },
    'label.knob.diameter':  { en: { t: 'Diameter' },  fr: { t: 'Diamètre',  reviewed: false } },

    // bellSize. "le pavillon" IS the bell of a wind instrument; "Taille pav."
    // measures 51.28 and says the same thing twice, since the readout beside it
    // is the size.
    'label.knob.bellSize': { en: { t: 'Bell Size' }, fr: { t: 'Pavillon', reviewed: false } },

    'label.knob.length': { en: { t: 'Length' }, fr: { t: 'Longueur', reviewed: false } },

    // toneHoleCutoff, 200-8000 Hz. "les trous de jeu" are a woodwind's finger
    // holes; the readout beside it carries the Hz.
    'label.knob.toneHole': { en: { t: 'Tone Hole' }, fr: { t: 'Trous de jeu', reviewed: false } },

    'label.knob.register': { en: { t: 'Register' }, fr: { t: 'Registre', reviewed: false } },

    // CLIFF 4 again, and this time it costs a whole word. `.knob-label` is
    // CENTRED in its 68px control, so a longer caption grows LEFTWARD as well as
    // right, and the reedOpening knob is the first in its row — flush with the
    // section's 8px left padding. The Reed section ships collapsed, so its
    // caption keeps its rectangle inside a max-height:0 box and lands on the
    // "Sound Design" chevron below: chevron right edge 33.00, English "Opening"
    // left edge 37.31 (4.31px clear), "Ouverture" left edge 30.64 (2.36px OVER).
    // "Ouvert." is 38.30 — 5.30px clear, BETTER than English — and matches this
    // page's own abbreviation style (Reed Hard., Inf. Sustain, Amortis., Flatt.,
    // Réinject.). See the gate note in the commit message: nothing here is ever
    // PAINTED on the chevron, because the caption is clipped out of existence by
    // its ancestor, and assertion 8b compares rectangles without asking that.
    'label.knob.opening': { en: { t: 'Opening' }, fr: { t: 'Ouvert.', reviewed: false } },
    'label.knob.mass':    { en: { t: 'Mass' },    fr: { t: 'Masse',     reviewed: false } },
    'label.knob.damping': { en: { t: 'Damping' }, fr: { t: 'Amortis.',  reviewed: false } },

    // "Anche double" is 67.20 against a 68px cap — 0.80px, which is the same
    // Windows-metric band that rejected "Dureté anche". The page's own
    // abbreviation style gives "Anche dble" at 54.59 with 13.41px to spare, and
    // the XY pad's Y-axis caption above it already spells the phrase out in
    // full where there is room for it.
    'label.knob.doubleReed': { en: { t: 'Double Reed' }, fr: { t: 'Anche dble', reviewed: false } },

    // mouthpieceVol. "le bec" is the mouthpiece of a clarinet or saxophone.
    'label.knob.mouthpiece': { en: { t: 'Mouthpiece' }, fr: { t: 'Bec', reviewed: false } },

    'label.knob.vibDepth': { en: { t: 'Vib Depth' }, fr: { t: 'Vib Prof.', reviewed: false } },
    'label.knob.vibRate':  { en: { t: 'Vib Rate' },  fr: { t: 'Vib Vit.',  reviewed: false } },

    // A saxophone growl is called a growl in French too.
    'label.knob.growl': {
        en: { t: 'Growl' },
        fr: { t: 'Growl', reviewed: false, sameAsEn: true },
    },

    // flutterTongue. French scores mark it "Flatt." (Flatterzunge), the same
    // abbreviation a French wind player reads on the page.
    'label.knob.flutter': { en: { t: 'Flutter' }, fr: { t: 'Flatt.', reviewed: false } },

    // A saxophone subtone is called a subtone in French too.
    'label.knob.subtone': {
        en: { t: 'Subtone' },
        fr: { t: 'Subtone', reviewed: false, sameAsEn: true },
    },

    // attackChiff — the breathy onset transient. "Chiff" has no French currency;
    // "Attaque" names the thing the knob shapes and is unambiguous because this
    // page has no other attack control.
    'label.knob.chiff': { en: { t: 'Chiff' }, fr: { t: 'Attaque', reviewed: false } },

    'label.knob.airNoise': { en: { t: 'Air Noise' }, fr: { t: 'Bruit d’air', reviewed: false } },

    'label.knob.infSustain': { en: { t: 'Inf. Sustain' }, fr: { t: 'Tenue inf.', reviewed: false } },
    'label.knob.revBore':    { en: { t: 'Rev. Bore' },    fr: { t: 'Perce inv.', reviewed: false } },
    'label.knob.feedback':   { en: { t: 'Feedback' },     fr: { t: 'Réinject.',  reviewed: false } },

    // dronePitch, -2400..2400 cents. "un bourdon" IS a drone; the readout beside
    // it carries the cents.
    'label.knob.dronePitch': { en: { t: 'Drone Pitch' }, fr: { t: 'Bourdon', reviewed: false } },

    'label.knob.maxVoices': { en: { t: 'Max Voices' }, fr: { t: 'Voix max', reviewed: false } },

    // ── Dropdown captions ───────────────────────────────────────────────────
    // Each sits above a full-width <select> in a block that is at least 662px
    // wide, so none of them can push anything. The OPTIONS inside those selects
    // are host-owned Choice strings and are not page copy — see the D-01 note.
    'label.dropdown.instrumentPreset': {
        en: { t: 'Instrument Preset' },
        fr: { t: 'Préréglage d’instrument', reviewed: false },
    },
    'label.dropdown.boreProfile': {
        en: { t: 'Bore Profile' },
        fr: { t: 'Profil de perce', reviewed: false },
    },
    'label.dropdown.vibratoSource': {
        en: { t: 'Vibrato Source' },
        fr: { t: 'Source du vibrato', reviewed: false },
    },
    'label.dropdown.polyMode': {
        en: { t: 'Poly Mode' },
        fr: { t: 'Mode polyphonique', reviewed: false },
    },
    'label.dropdown.oversampling': {
        en: { t: 'Oversampling' },
        fr: { t: 'Suréchantillonnage', reviewed: false },
    },

    // ── Toggle ──────────────────────────────────────────────────────────────
    'label.toggle.dualBore': { en: { t: 'Dual Bore' }, fr: { t: 'Double perce', reviewed: false } },

    // ── Placeholders ────────────────────────────────────────────────────────
    // Both live in centred flex boxes with hundreds of pixels of slack, so the
    // French length is unconstrained. They are the page's most user-facing
    // prose after the knob captions: a French user who opens the FX tab reads a
    // sentence, not a caption.
    'label.boreViz.placeholder': {
        en: { t: 'Bore cross-section visualization (coming soon)' },
        fr: { t: 'Visualisation en coupe de la perce (bientôt disponible)', reviewed: false },
    },
    'label.fx.title': {
        en: { t: 'Coming Soon' },
        fr: { t: 'Bientôt disponible', reviewed: false },
    },
    'label.fx.body': {
        en: { t: 'Effects processing will be added in a future update.' },
        fr: { t: 'Le traitement d’effets sera ajouté dans une future mise à jour.', reviewed: false },
    },

    // ── The settings popover (v1.2.0) ───────────────────────────────────────
    'label.language': { en: { t: 'Language' }, fr: { t: 'Langue', reviewed: false } },

    // ── The one JS-written string on this page ──────────────────────────────
    // The tuning panel is lazy-mounted on the first Tuning-tab activation and
    // this is what the container says if that dynamic import fails. It is
    // written through setLabel(), so the node becomes a [data-i18n] element from
    // that moment on and the language sweep owns it — a failure notice stranded
    // in the previous language is exactly the bug contract §3 exists to prevent.
    // At v1.1.0 it was an innerHTML string; assertion 9 forbids that path.
    'label.tuningLoadFailed': {
        en: { t: 'Tuning panel failed to load.' },
        fr: { t: 'Échec du chargement du panneau d’accord.', reviewed: false },
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
        fr: { t: 'Réglages', reviewed: false },
    },
    'aria.langSelect': {
        en: { t: 'Interface language' },
        fr: { t: 'Langue de l’interface', reviewed: false },
    },
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
// TIP_BINDINGS — EMPTY. See the header: this plugin has no hover-help.
//
// Exported because the canonical import line names it and applyI18n() iterates
// it. A zero-length loop is the correct no-op; the alternative — omitting the
// export and editing the canon block to match — would put this plugin's copy of
// the runtime out of step with the other forty-plus, which is the whole drift
// the canon gate exists to prevent.
// ============================================================================

export const TIP_BINDINGS = [];

// The tooltip lookup. Returns {t, b} — never null, never a bare key without a
// console.warn saying so, because a silently-missing tip renders as an empty
// surface that looks like a positioning bug rather than a missing entry.
//
// Unreferenced at runtime today: applyI18n() calls it only from the
// TIP_BINDINGS loop, which is empty. It is exported verbatim all the same, so
// that the canon block is byte-identical to every other copy and Stage M can
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
