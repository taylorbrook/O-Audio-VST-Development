/*
   This file is part of O-Bassoon, an Ouaricon Audio plugin.
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
// i18n.js — O-Bassoon page labels and hover-help, English + French (v1.2.2)
//
// ── v1.2.2: STAGE O, item 36 (2026-08-31) ───────────────────────────────────
// tip.breath said CC2 "takes over" the knob. BassoonVoice.cpp:167 composes
// breathSmoother's target as lastUiBreath * cc2Normalised — a PRODUCT: with the
// knob at 0 a breath controller does nothing, with CC2 at 0 the knob does
// nothing. aria.breathMeter already said so ("UI breath × CC2"); the tip now
// says the same thing in tip form (knob = ceiling, CC2 scales it, both above
// zero) and keeps the true half of the old sentence (the knob alone applies
// again 500 ms after CC2 stops, BassoonVoice.cpp:202-207). French rewritten
// in the same commit, reviewed: false again — a meaning change. Tip height
// at the 260 px cap, measured by tests/ui_tip_render_check.js: en 153.2 ->
// 219.9 px, fr 136.5 -> 219.9 px; bottom clearance 270.8 / 287.5 -> 204.1 px,
// right clearance 117.0 px unchanged, still inside the 900 x 600 frame.
// check-ui-labels byte-identical to its baseline, 0 non-label elements moved.
// No caption, CSS or other version site changed.
//
// ── v1.2.1: FRENCH QA PASS (Stage N, 2026-08-31) ────────────────────────────
// Every fr entry read against its en and against scripts/i18n-fr-glossary.js.
// Changed: 16 entries (5 terminology, 6 typography, 4 grammar/idiom,
// 1 meaning). sameAsEn: kept 4, translated 0. termNote exemptions: 0.
// Left as drafted: the rest. reviewed: false throughout — no native speaker yet.
// i18n-fr-lint --plugin O-Bassoon: 26 findings (11 T4, 9 T7, 2 T5, 2 G1, 2 F1)
// -> 0, --strict exit 0.
//
// The decisions the next reader needs:
//
//   RELÂCHE -> RELÂCH.  The glossary settles Release as Relâchement (Relâch.),
//     and the root term does NOT fit here: measured with Range.selectNodeContents
//     on the real .knob-label at the shipping 900 x 600 frame, RELÂCHEMENT is
//     79.81 px against this box's 78 px max-width — it ellipsises, silently, the
//     half a spill check cannot see. RELÂCH. is 45.59 px, which is 3.64 px
//     NARROWER than the old RELÂCHE (49.23) and 1.35 px narrower than English
//     RELEASE (46.94). Six of the ten captions now shrink into French rather
//     than five. No CSS was touched; Stage N touches none.
//   Relâchement in the TIP TITLE, Relâch. in the caption. The header rule above
//     ("the title is the page's caption") holds for a caption that DISAGREES
//     with the parameter name. This caption is the same word with letters
//     missing, and a 260 px tooltip is where the full term belongs (Stage M2
//     correction 9). Relâch ⊂ Relâchement, so the pair holds by stem. There is
//     no aria-label on any knob here, so WCAG 2.5.3 label-in-name has no
//     accessible name to compare against and does not decide the period.
//   CARACTÈRE stays. The head-noun choice recorded below was re-measured, not
//     inherited: 61.97 px against a 78 px cap, ATTACK CHAR 72.98 px. The
//     v1.2.0 header's whole width table reproduced to the hundredth.
//   ADSR stage names in tip.attack. "ni décroissance ni palier" became "ni
//     étage de déclin ni étage de maintien" — the glossary's textbook ADSR
//     (Déclin / Maintien), and BassoonVoice.cpp:101 really is {attack, 0, 1,
//     release}. tip.release keeps "décroissance" deliberately: there the word
//     names the modal resonance tail's decay, not an envelope stage.
//   "filtrée dans le grave" -> "filtrée en passe-bas" in tip.attackChar. The
//     English says low-passed and Exciter.cpp:50 is a 1-pole LP at 600 Hz;
//     passe-bas is the French term for it.
//   tip.depth had DROPPED the English's "bends the pitch" (Stage N2 correction
//     19). "L'amplitude du vibrato de part et d'autre de la note" became "De
//     combien le vibrato écarte la hauteur de part et d'autre de la note".
//   Bodies are not matched against the glossary TERMS table, and two loanword-
//     free renderings were kept on purpose: "le grain des modes aigus" for the
//     reed buzz in tip.tone, and "traîne de résonance" for the modal tail in
//     tip.release. Both read as French audio prose; neither is a caption.
//   26 U+00A0 landed, all inside fr string VALUES: 12 before a colon, 2 before
//     a semicolon, 12 between a number and its unit. Applied by a state machine
//     over the fr: { } blocks, never a regex over the file — ' , % and every
//     unit appear in the English bodies and in this header. Audited afterwards:
//     grep for U+00A0 outside a t:/b: line returns 0, and importing both
//     revisions and comparing shows 0 en values, 0 keys, 0 TIP_BINDINGS rows,
//     0 I18N_EXEMPT rows and 0 reviewed/sameAsEn flags changed.
//   The one U+00A0 that could have moved geometry is in label.about.blurb
//     ("16 voix"), which makes that pair unbreakable inside a card the header
//     below pins at three line boxes. Measured after: 3 lines and a 242.13 px
//     card in BOTH languages, unchanged. check-ui-labels' output is
//     byte-identical to its pre-change baseline, 0 non-label elements moved.
//   Tip clearance, measured before and after through the committed render gate:
//     three French bodies gained one 16.7 px line box (tip.depth, tip.attack,
//     tip.release). The tightest French bottom clearance went 97.1 -> 80.4 px
//     and the tightest right clearance is unchanged at 29.0 px. 198 PASS,
//     0 FAIL, all three negative controls firing, before and after.
//
// An ES module that EXPORTS ONLY. It must never self-execute: a bare top-level
// statement here throws out of module evaluation and takes every later
// initializer on the page with it (pattern_module_toplevel_init_tdz). This
// plugin's controller is ONE inline <script type="module"> in index.html — the
// O-Bitrot / O-AnalogSaturation shape, not the O-Tapestop one — so that failure
// mode would take the WHOLE UI, not a panel of it. check-i18n assertion 7
// enforces the export-only rule.
//
// FILENAME: no hyphen. juce_add_binary_data STRIPS hyphens rather than
// converting them to underscores, so a second file named i18n-fr.js would have
// to be reached as the symbol i18nfr_js (critical_binary_data_strips_hyphens).
// One combined file for both languages sidesteps the question entirely.
//
// ── v1.2.0 ADDS HOVER-HELP, AND THE COPY IS ONLY HALF OF IT ─────────────────
//
// v1.0.0 carried no data-tip and no data-tooltip anywhere — only three native
// title= attributes, which contract §4 DELETES rather than localizes — and
// v1.1.0 localized the page with I18N and TIP_BINDINGS both empty, which was
// this plugin's correct state rather than a gap.
//
// v1.2.0 authors twelve tips here: the ten APVTS parameters plus #gear-btn and
// #lang-select. That alone would have shipped TWELVE INVISIBLE STRINGS. canon
// v2's applyI18n() writes data-tip-title and data-tip onto the anchors named in
// TIP_BINDINGS and stops there; the thing that reads those attributes and
// paints a surface is per-plugin code, and until v1.2.0 this page had no
// #tooltip element, no .tooltip rule and no hover handler at all. All three
// standing gates would have stayed green through it — check-i18n only counts
// bindings, check-ui-labels has no tooltip awareness whatsoever, and
// boot-all-uis counts aria-label and title and never data-tip. So the renderer
// lands in index.html in the SAME commit as this table, and
// tests/ui_tip_render_check.js is the gate that can actually see a rendered
// tip.
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
// AUTHORED IN v1.2.0. Until then this table was empty and TIP_BINDINGS was
// empty with it, which was this plugin's correct state rather than a gap —
// v1.0.0 carried no data-tip anywhere, only three native title= attributes
// that contract §4 deletes.
//
// ── THE TITLE IS THE PAGE'S CAPTION, NOT THE PARAMETER'S NAME ───────────────
//
// Every title below is byte-identical to the .knob-label above the same knob,
// because that is the word the user is reading. The APVTS display names are
// longer and section-qualified ("Vibrato Rate", "Attack Character") and appear
// only in the host's automation lane, which no one is looking at while hovering
// a knob.
//
// ── WHERE THE RANGES COME FROM ──────────────────────────────────────────────
//
// Every body ends with the range and unit, taken from .planning/params.tsv (the
// runtime parameter dump, not a regex over createParameterLayout). Four of the
// ten parameters carry an EMPTY `label` in the dump — breath, tone,
// attack_character and voice_count — so their units were recovered from the
// page's own formatter rather than invented: PARAMS in index.html declares
// `unit: ''` for the first three (index.html:813-815) and `unit: '', isInt:
// true` for voice_count (index.html:818), and formatValue (index.html:907)
// appends nothing. They are genuinely unitless — a 0..1 normalised control and
// a count — and the bodies say "0 to 1" and "1 to 16 voices" accordingly.
//
// One divergence between the dump and the page is deliberate and is NOT a
// defect: vibrato_depth's APVTS label is " cents" while the page's readout
// renders " c" (index.html:811). The body spells the unit out, because a
// tooltip is where a user learns what the abbreviation on the readout means.
//
// ── D-01 ARM 1 STILL NEVER FIRES ON THIS PLUGIN ─────────────────────────────
//
// O-Bassoon has NO AudioParameterChoice: nine AudioParameterFloat and one
// AudioParameterInt. There is no host automation-lane option string anywhere
// for a French body to disagree with. The one place a body names a control's
// end points — attack_character's Soft / Tongued — names the PAGE's own
// end-labels, which are LABELS entries and therefore localized, so the French
// body says "de Doux à Détaché" to match what the page beside it is showing.
//
// ── THE BODIES DESCRIBE THIS MODEL, NOT A GENERIC EFFECT ────────────────────
//
// This is a physical model, so several of these knobs name a modelling quantity
// rather than a familiar effect control and the tooltip is the only place a user
// can learn what one does. Each body was written against the DSP that implements
// it — Vibrato.cpp, NoiseExciter.cpp, ModeBank.cpp, Exciter.cpp and
// BassoonSynthesiser.h — rather than from the parameter's name, and each states
// something a user cannot discover by turning the knob:
//
//   rate         at 0 Hz the LFO stops with a RANDOM per-note phase, so the
//                vibrato freezes into a fixed detune instead of switching off
//                (Vibrato.cpp reset() + getCurrentCents()).
//   breath       CC2 does NOT take over: the effective breath is ui_breath x cc2,
//                a product (BassoonVoice.cpp:167), so the knob is the ceiling and
//                both must be above zero; the knob alone applies again 500 ms
//                after CC2 stops (setExpression's idle window, :202-207).
//   tone         partials 6-16 only; modes 1-5 are tone-invariant
//                (ModeBank.cpp:setFundamental, k > 4).
//   attack char  snapshotted at note-on and velocity-biased by ±0.15
//                (Exciter.h startOnset, VELOCITY_BIAS_MAGNITUDE 0.3 halved by
//                the (velocity - 0.5) centring).
//   attack       there is no decay and no sustain stage: the ADSR is
//                {attack, 0, 1, release} (BassoonVoice.cpp startNote).
//   release      shortens the AMPLITUDE tail over a modal tail that runs to
//                2.5 s (ModeBank.h BASE_T60[0]).
//   voices       the cap is enforced at note-on and steals release-tails first;
//                lowering it never cuts a sounding note (BassoonSynthesiser.h).
// ============================================================================

export const I18N = Object.freeze({

    // ── Vibrato ─────────────────────────────────────────────────────────────

    'tip.rate': {
        en: { t: 'Rate',
              b: 'Speed of the pitch vibrato, a sine oscillator per voice that bends the whole mode bank up and down. Each note starts it at a random phase, so at 0 Hz the vibrato freezes into a small fixed detune rather than switching off — Depth at 0 is what silences it. 0 to 10 Hz.' },
        fr: { t: 'Vitesse',
              b: 'Vitesse du vibrato de hauteur, un oscillateur sinusoïdal par voix qui fait monter et descendre tout le banc de modes. Chaque note le démarre à une phase aléatoire : à 0 Hz le vibrato se fige en un léger désaccord constant au lieu de s’arrêter — c’est Profondeur à 0 qui le fait taire. 0 à 10 Hz.',
              reviewed: true },
    },

    'tip.depth': {
        en: { t: 'Depth',
              b: 'How far the vibrato bends the pitch either side of the note, in cents — 100 cents is one semitone. A bassoonist’s vibrato lives in the bottom fifth of this range; everything above that is a deliberate effect. 0 to 100 cents.' },
        fr: { t: 'Profondeur',
              b: 'De combien le vibrato écarte la hauteur de part et d’autre de la note, en cents — 100 cents font un demi-ton. Le vibrato d’un bassoniste se tient dans le premier cinquième de la course ; au-dessus, c’est un effet délibéré. 0 à 100 cents.',
              reviewed: true },
    },

    'tip.onset': {
        en: { t: 'Onset',
              b: 'The wait before the vibrato reaches full depth. Every note-on restarts the fade from nothing and ramps it in evenly over this time, which is the straight-then-warming entry a wind player makes. 0 to 2000 ms.' },
        fr: { t: 'Délai',
              b: 'L’attente avant que le vibrato atteigne sa pleine profondeur. Chaque note relance le fondu depuis zéro et le fait monter régulièrement sur cette durée : c’est l’entrée droite puis chaleureuse d’un instrumentiste à vent. 0 à 2000 ms.',
              reviewed: true },
    },

    // ── Expression ──────────────────────────────────────────────────────────

    'tip.breath': {
        en: { t: 'Breath',
              b: 'How hard the instrument is blown. It scales the filtered noise that keeps the modes ringing, so it sets loudness and the amount of audible breath together rather than one after the other. A MIDI breath controller (CC2) does not replace this knob: while CC2 is moving, the effective breath is knob × CC2 — the knob sets the ceiling, CC2 scales it, and both must be above zero for the instrument to sound. Half a second after CC2 stops, the knob alone applies again. 0 to 1.' },
        fr: { t: 'Souffle',
              b: 'La pression du souffle. Elle dose le bruit filtré qui entretient les modes : elle règle donc d’un seul geste le volume et la quantité de souffle audible. Un contrôleur de souffle MIDI (CC2) ne remplace pas ce bouton : tant qu’il bouge, le souffle effectif vaut bouton × CC2 — le bouton fixe le plafond, le CC2 le module, et il faut les deux au-dessus de zéro pour que l’instrument sonne. Une demi-seconde après l’arrêt du CC2, le bouton seul s’applique de nouveau. 0 à 1.',
              reviewed: false },
    },

    'tip.tone': {
        en: { t: 'Tone',
              b: 'How long the upper partials keep ringing: partials 6 to 16 get a decay between 0.3× and 1.5× their nominal length, while the first five are left untouched. Low is a dark, quickly damped reed; high keeps the buzz of the high modes alive under the note. 0 to 1.' },
        fr: { t: 'Timbre',
              b: 'La durée de résonance des partiels supérieurs : les partiels 6 à 16 reçoivent une décroissance de 0,3× à 1,5× leur longueur nominale, les cinq premiers restent intacts. En bas, une anche sombre et vite amortie ; en haut, le grain des modes aigus reste vivant sous la note. 0 à 1.',
              reviewed: true },
    },

    'tip.attackChar': {
        en: { t: 'Attack Char',
              b: 'Morphs the start of the note between two excitation shapes: at 0 a soft 30 ms low-passed swell, at 1 a sharp 7.5 ms noise burst — a tongued articulation. Note velocity shifts the value by up to 0.15 either way, and the result is frozen at note-on, so automating it during a note does nothing. 0 to 1, Soft to Tongued.' },
        fr: { t: 'Caractère',
              b: 'Fait passer le début de la note d’une forme d’excitation à l’autre : à 0 une montée douce de 30 ms filtrée en passe-bas, à 1 une brève salve de bruit de 7,5 ms — un coup de langue. La vélocité décale la valeur de 0,15 au plus dans un sens ou l’autre, et le résultat est figé au début de la note : l’automatiser en cours de note ne change rien. 0 à 1, de Doux à Détaché.',
              reviewed: true },
    },

    // ── Envelope ────────────────────────────────────────────────────────────

    'tip.attack': {
        en: { t: 'Attack',
              b: 'How long the amplitude envelope takes to climb to full level once a note starts. There is no decay and no sustain stage — the envelope simply holds until the key is released — so this and Release are the whole shape. 0 to 2000 ms.' },
        fr: { t: 'Attaque',
              b: 'Le temps que met l’enveloppe d’amplitude à monter au niveau plein dès le début de la note. Il n’y a ni étage de déclin ni étage de maintien : l’enveloppe se tient simplement au niveau plein jusqu’au relâchement de la touche, si bien que ce réglage et Relâchement en constituent toute la forme. 0 à 2000 ms.',
              reviewed: true },
    },

    'tip.release': {
        en: { t: 'Release',
              b: 'How long the note takes to fade once the key is lifted. It shapes the amplitude only: the mode bank carries its own resonance tail of up to 2.5 seconds underneath, so a short setting cuts that tail off rather than making it decay faster. 0 to 3000 ms.' },
        fr: { t: 'Relâchement',
              b: 'Le temps de disparition de la note une fois la touche relâchée. Il ne façonne que l’amplitude : le banc de modes garde en dessous sa propre traîne de résonance pouvant aller jusqu’à 2,5 secondes, qu’un réglage court coupe net au lieu d’en accélérer la décroissance. 0 à 3000 ms.',
              reviewed: true },
    },

    // ── Voicing & Output ────────────────────────────────────────────────────

    'tip.voices': {
        en: { t: 'Voices',
              b: 'The most notes that may sound at once. The limit is checked at each note-on: past it a voice is taken back, preferring one already in its release tail and otherwise the oldest note. Lowering the limit never cuts a note that is already sounding. 1 to 16 voices.' },
        fr: { t: 'Voix',
              b: 'Le nombre maximal de notes pouvant sonner en même temps. La limite est vérifiée à chaque nouvelle note : au-delà, une voix est reprise, de préférence une déjà en fin de relâchement, sinon la plus ancienne. Baisser la limite ne coupe jamais une note en cours. 1 à 16 voix.',
              reviewed: true },
    },

    'tip.output': {
        en: { t: 'Output',
              b: 'The final level applied to the summed voices, ramped across each block so a move never clicks. Sixteen voices together are a great deal louder than one, so this is where a dense passage gets pulled back. −24 to +6 dB.' },
        fr: { t: 'Sortie',
              b: 'Le niveau final appliqué à la somme des voix, lissé sur chaque bloc pour qu’un mouvement ne claque jamais. Seize voix ensemble sont bien plus fortes qu’une seule : c’est ici qu’on rattrape un passage dense. −24 à +6 dB.',
              reviewed: true },
    },

    // ── The two chrome controls ─────────────────────────────────────────────
    //
    // The gear tip is what tells a user hover-help exists at all, so its body
    // describes ONLY what the popover actually holds. This plugin has no
    // hover-help on/off switch — not in C++, not in storage — so the panel
    // holds the language selector and nothing else, and the tip says exactly
    // that. O-Tapestop's wording promises a toggle this plugin does not have,
    // and a tip that lies is worse than no tip.

    'tip.gearBtn': {
        en: { t: 'Settings',
              b: 'Opens the panel that sets the language of this interface. That is all it holds: the captions on this page and this hover help change with it, and the choice is kept with the session, so a project reopens in the language it was saved in.' },
        fr: { t: 'Réglages',
              b: 'Ouvre le panneau qui règle la langue de cette interface. Il ne contient rien d’autre : les libellés de cette page et cette aide au survol changent avec elle, et le choix est conservé avec la session — un projet se rouvre dans la langue où il a été enregistré.',
              reviewed: true },
    },

    // The last sentence is a statement of fact recorded in I18N_EXEMPT below:
    // every caption inside the Tuning tab belongs to the shared
    // scala-tuning-engine module and is not this plugin's to translate. A user
    // who switches to French and then opens that tab deserves to have been
    // told, rather than reading it as a bug.
    'tip.langSelect': {
        en: { t: 'Language',
              b: 'The language of the captions on this page and of this hover help. English and French are available. Value readouts stay as numbers and units in both, and the Tuning tab stays in English — its panel comes from a shared module that is not part of this plugin.' },
        fr: { t: 'Langue',
              b: 'La langue des libellés de cette page et de cette aide au survol. L’anglais et le français sont proposés. Les valeurs affichées restent des nombres et des unités dans les deux langues, et l’onglet Accord demeure en anglais : son panneau provient d’un module partagé qui n’appartient pas à ce plugin.',
              reviewed: true },
    },
});

// ============================================================================
// LABELS — the visible text of the page. {en:{t}, fr:{t, reviewed}}.
//
// One string per entry, no body: a label is not a tooltip.
//
// ── THE D-01 TEST ON THIS PLUGIN ────────────────────────────────────────────
//
// ARM 1 NEVER FIRES HERE. O-Bassoon has NO AudioParameterChoice at all: its ten
// parameters are nine AudioParameterFloat plus one AudioParameterInt
// (PluginProcessor.cpp createParameterLayout). There is therefore no host
// automation-lane option string anywhere for a French caption to disagree with,
// and every knob caption below is a page caption rather than an option word.
//
// ARM 2 / ARM 3 CARRY THE WHOLE EXEMPTION LOAD instead — the five .knob-value
// nodes. See the READOUTS note in I18N_EXEMPT.
//
// ── GEOMETRY: THE ONE TIGHT BOX ON THIS PAGE ────────────────────────────────
//
// `.knob-label` is `max-width: 78px; white-space: nowrap; overflow: hidden;
// text-overflow: ellipsis` at 9.5px uppercase with 0.6px tracking. It is the
// only clipping box in the layout, and it clips SILENTLY — an over-long French
// caption renders an ellipsis rather than overflowing, which is exactly the
// half a spill check cannot see. Every caption was measured as RENDERED in the
// shipping 900 x 600 frame, not estimated from a font probe (text-transform and
// letter-spacing are not in getComputedStyle().font):
//
//     RATE        27.5 -> VITESSE     43.2
//     DEPTH       35.8 -> PROFONDEUR  72.8    5.2 px of the 78 px cap left
//     ONSET       34.8 -> DÉLAI       32.1    SHRANK
//     BREATH      42.7 -> SOUFFLE     47.1
//     TONE        28.8 -> TIMBRE      41.1
//     ATTACK CHAR 73.0 -> CARACTÈRE   62.0    SHRANK  (see the note on the key)
//     ATTACK      40.8 -> ATTAQUE     49.2
//     RELEASE     46.9 -> RELÂCH.     45.6    SHRANK  (v1.2.1; was RELÂCHE 49.2)
//     VOICES      38.3 -> VOIX        26.3    SHRANK
//     OUTPUT      42.6 -> SORTIE      38.5    SHRANK
//
// Re-measured at v1.2.1 with the same method: every row above reproduced to the
// hundredth, so this table was honest. RELEASE is the one that moved, because
// Stage N applied the glossary's Relâch. — RELÂCHEMENT is 79.81 px and would
// have ellipsised against the 78 px cap.
//
// Six of the ten SHRINK. That is the half of the risk a clip check is blind to
// and the half Stage J found four times in twelve, and it is the reason the
// before/after diff is run in both directions rather than only looking for
// growth.
//
// The two `.knob-endlabels` spans share a 78 px space-between row:
// SOFT 23.5 + TONGUED 46.1 = 69.6 in English, DOUX 27.2 + DÉTACHÉ 44.3 = 71.5
// in French. 6.5 px of gap survives, so the pair never collides.
//
// No pin was added anywhere on this page. Nothing needed one, and a pin whose
// negative control passes is decoration.
// ============================================================================

export const LABELS = Object.freeze({

    // ── Header strip ────────────────────────────────────────────────────────
    // The TITLE beside this is the product name and is exempt; the subtitle is
    // this page's own description of what the product is, so it localizes.
    // `margin-left: auto` in a 900 px flex bar means it can grow leftward for
    // free — French is 6.7 px wider and nothing else in the bar moves.
    'label.subtitle': {
        en: { t: 'Ouaricon · Modal Synthesis Bassoon' },
        fr: { t: 'Ouaricon · Basson à synthèse modale', reviewed: true },
    },

    // ── Tab bar ─────────────────────────────────────────────────────────────
    // Three `flex: 1` buttons, 300 px each, centred text. The widest French
    // caption is 72.5 px, so the row cannot be pushed by any of them.
    'label.tab.sound':  { en: { t: 'Sound' },  fr: { t: 'Son',      reviewed: true } },
    'label.tab.tuning': { en: { t: 'Tuning' }, fr: { t: 'Accord',   reviewed: true } },
    'label.tab.about':  { en: { t: 'About' },  fr: { t: 'À propos', reviewed: true } },

    // ── Section headings ────────────────────────────────────────────────────
    // `sameAsEn: true` is an ASSERTION, not a shrug: "Vibrato" and "Expression"
    // are the same word in French, and the flag is what stops assertion 4
    // reading an identical string as an untranslated one. Without it the gate
    // cannot tell a deliberate cognate from a forgotten entry.
    'label.section.vibrato': {
        en: { t: 'Vibrato' },
        fr: { t: 'Vibrato', reviewed: true, sameAsEn: true },
    },
    'label.section.expression': {
        en: { t: 'Expression' },
        fr: { t: 'Expression', reviewed: true, sameAsEn: true },
    },
    'label.section.envelope': {
        en: { t: 'Envelope' },
        fr: { t: 'Enveloppe', reviewed: true },
    },
    'label.section.voicing': {
        en: { t: 'Voicing & Output' },
        fr: { t: 'Voix et sortie', reviewed: true },
    },

    // ── Knob captions ───────────────────────────────────────────────────────
    // Captions under the knob, never the value beside them: the .knob-value
    // sibling is a separate node and stays untouched (contract §5 — the split
    // this page already had).
    'label.knob.rate':   { en: { t: 'Rate' },   fr: { t: 'Vitesse',    reviewed: true } },
    'label.knob.depth':  { en: { t: 'Depth' },  fr: { t: 'Profondeur', reviewed: true } },

    // vibrato_onset is the DELAY before the vibrato speaks, 0-2000 ms
    // (PluginProcessor.cpp, "Vibrato Onset"). "Délai" is the French term for
    // that delay; "Début" would name the moment rather than the wait.
    'label.knob.onset':  { en: { t: 'Onset' },  fr: { t: 'Délai',      reviewed: true } },

    'label.knob.breath': { en: { t: 'Breath' }, fr: { t: 'Souffle',    reviewed: true } },
    'label.knob.tone':   { en: { t: 'Tone' },   fr: { t: 'Timbre',     reviewed: true } },

    // attack_character, whose English caption is ALREADY an abbreviation of the
    // parameter's display name "Attack Character" and already runs to 73.0 px of
    // the 78 px cap. The literal French, "Car. attaque", measures 76.2 px — it
    // fits, by 1.8 px, which is inside the margin where a Windows/WebView2 font
    // metric difference decides whether a caption ellipsises. Windows metrics
    // are the named hardware-blocked deferral for this whole rollout, so the
    // head noun alone is used instead: it is 62.0 px with 16 px to spare, the
    // section heading above it already reads EXPRESSION, and the two end-labels
    // under it (DOUX ↔ DÉTACHÉ) name the two ends of the attack it shapes.
    'label.knob.attackChar': {
        en: { t: 'Attack Char' },
        fr: { t: 'Caractère', reviewed: true },
    },

    'label.knob.attack':  { en: { t: 'Attack' },  fr: { t: 'Attaque', reviewed: true } },
    'label.knob.release': { en: { t: 'Release' }, fr: { t: 'Relâch.', reviewed: true } },
    'label.knob.voices':  { en: { t: 'Voices' },  fr: { t: 'Voix',    reviewed: true } },
    'label.knob.output':  { en: { t: 'Output' },  fr: { t: 'Sortie',  reviewed: true } },

    // ── The attack_character end-label pair ─────────────────────────────────
    // "Détaché" is the bassoon articulation term a French player would use for a
    // tongued note; "Coup de langue" is the literal phrase and is twice as wide
    // in a 78 px row that already carries two captions.
    'label.end.soft':    { en: { t: 'Soft' },    fr: { t: 'Doux',    reviewed: true } },
    'label.end.tongued': { en: { t: 'Tongued' }, fr: { t: 'Détaché', reviewed: true } },

    // ── About card ──────────────────────────────────────────────────────────
    // "Version" is the same word in French, hence the flag. The NUMBER beside it
    // was split into its own span in this commit and carries no key: a version
    // string is not copy, and leaving it inside the localized string would put
    // the shipping version behind a translation nobody re-checks (contract §5).
    'label.about.version': {
        en: { t: 'Version' },
        fr: { t: 'Version', reviewed: true, sameAsEn: true },
    },
    'label.about.tagline': {
        en: { t: 'Modal-synthesis bassoon for sustained microtonal long tones.' },
        fr: { t: 'Basson à synthèse modale pour de longues tenues microtonales.', reviewed: true },
    },

    // AUTHORED TO THE ENGLISH LINE COUNT, not merely translated. The card is
    // `max-width: 540px` and its `.about-meta` row below is NOT a label, so a
    // French blurb of a different HEIGHT moves it — and assertion 7 reports
    // that, correctly, as a French string moving page furniture.
    //
    // The first draft was too SHORT, not too long: at two line boxes against
    // English's three it shrank the card by 19.4px (exactly one 12.5px/1.55
    // line) and pulled the byline UP. That is the shrink half of the risk, the
    // half a clip check cannot see, and the geometry diff caught it. The
    // wording below is the fuller and more faithful translation, and it lands
    // on three lines and a 242.1px card in both languages.
    'label.about.blurb': {
        en: { t: 'Polyphonic 1–16 voices, VST3 Note Expression + MPE for Dorico microtonal playback, breath/CC2 expression, vibrato, and the Ouaricon tuning-system family. Built on JUCE 8.' },
        fr: { t: 'Polyphonie de 1 à 16 voix, VST3 Note Expression + MPE pour la lecture microtonale dans Dorico, expression au souffle/CC2, vibrato et la famille de systèmes d’accord Ouaricon. Conçu avec JUCE 8.', reviewed: true },
    },
    'label.about.madeBy': {
        en: { t: 'Made by' },
        fr: { t: 'Réalisé par', reviewed: true },
    },

    // ── The company name, KEYED rather than exempt, and why ─────────────────
    //
    // "Ouaricon" is never translated — `sameAsEn: true` is the assertion that
    // its identity is deliberate. What it is NOT is exempt, and the difference
    // is geometry rather than translation.
    //
    // The product name in the header strip and in the About card's title IS
    // exempt: each is alone in its own block, so nothing sits beside it to be
    // pushed. This one is the second half of a CENTRED line of text whose first
    // half localizes — "Made by" -> "Réalisé par" is 13.6px wider, so the line
    // re-centres and the link moves 6.8px right. That is a centred sentence
    // behaving exactly as a centred sentence should, but an EXEMPT element is
    // page furniture to assertion 7 and it was reported as a French geometry
    // failure. Keying it says what is actually true — it is text on a localized
    // line — and leaves assertions 4, 5, 6 and 8 still measuring it.
    //
    // No pin was added for this. A width pin on the "Made by" span would also
    // hold the link still, but it would freeze a centred line into a two-column
    // layout to satisfy a gate, which is the shape of a decorative fix.
    'label.about.company': {
        en: { t: 'Ouaricon' },
        fr: { t: 'Ouaricon', reviewed: true, sameAsEn: true },
    },

    // ── The settings popover (v1.1.0) ───────────────────────────────────────
    'label.language': { en: { t: 'Language' }, fr: { t: 'Langue', reviewed: true } },

    // ── The one JS-written string on this page ──────────────────────────────
    // The tuning panel is lazy-mounted on the first Tuning-tab activation and
    // this is what the container says if that dynamic import fails. It is
    // written through setLabel(), so the node becomes a [data-i18n] element from
    // that moment on and the language sweep owns it — a failure notice stranded
    // in the previous language is exactly the bug contract §3 exists to prevent.
    'label.tuningLoadFailed': {
        en: { t: 'Tuning panel failed to load.' },
        fr: { t: 'Échec du chargement du panneau d’accord.', reviewed: true },
    },

    // ── Accessible names ────────────────────────────────────────────────────
    // Resolved through the same sweep via data-i18n-aria, so a screen reader
    // hears the language the page is showing.
    //
    // THE THREE BELOW ARE NOT NEW PROSE. Each is the exact text of a native
    // title= attribute that v1.0.0 carried and that contract §4 deletes; where a
    // title= was an element's only help, its text MOVES to data-i18n-aria. The
    // English side of each is byte-identical to what HEAD had, apart from the
    // "x" in the breath meter becoming the multiplication sign the sentence
    // always meant.
    'aria.vibratoDot': {
        en: { t: 'Vibrato envelope' },
        fr: { t: 'Enveloppe du vibrato', reviewed: true },
    },
    'aria.breathMeter': {
        en: { t: 'Effective breath (UI breath × CC2)' },
        fr: { t: 'Souffle effectif (souffle de l’interface × CC2)', reviewed: true },
    },
    'aria.voiceDots': {
        en: { t: 'Live active voice count' },
        fr: { t: 'Nombre de voix actives en temps réel', reviewed: true },
    },

    'aria.settings': {
        en: { t: 'Settings' },
        fr: { t: 'Réglages', reviewed: true },
    },
    'aria.langSelect': {
        en: { t: 'Interface language' },
        fr: { t: 'Langue de l’interface', reviewed: true },
    },
});

// ============================================================================
// I18N_EXEMPT — reasoned exclusions, never silence
//
// Every visible string the coverage scan finds must be a [data-i18n] element, a
// setLabel() call, or an entry HERE WITH A REASON. A bare skip list would let a
// missed label hide as a deliberate one.
//
// ── THE FIVE READOUTS, AND WHY THEY ARE NOT LISTED INDIVIDUALLY ─────────────
//
// The extractor classes five `.knob-value` nodes READOUT: "5.00 Hz", "400 ms",
// "300 ms", "800 ms" and "0.0 dB" — and the five it does NOT class READOUT
// ("15.0 c", "0.70", "0.50", "0.00", "8") are the same node in the same class
// holding a bare number. All ten are the authored PLACEHOLDER of a node that
// formatValue() overwrites on the first valueChangedEvent and on every drag
// afterwards. They are exempt on D-01 arm 2 (a number and its unit are
// language-neutral, D-03) AND on arm 3 (a readout node is never a [data-i18n]
// element, whatever parameter type is behind it — keying one would make the
// element enter and leave the sweep as the knob turns). They are not listed as
// I18N_EXEMPT entries because the coverage scan already classes them non-LABEL,
// and ten entries whose text changes on the first mouse drag would be ten
// entries that never match anything again.
// ============================================================================

export const I18N_EXEMPT = [
    ['O-Bassoon',
     'the product name — never translated. It is the registered PRODUCT_NAME in CMakeLists.txt and it appears twice on the page, in the header strip and as the About card\'s title'],

    // "Ouaricon" is deliberately NOT here. It is never translated, but it is
    // the second half of a centred line whose first half localizes, so it is a
    // LABELS entry with sameAsEn: true rather than an exemption — see the note
    // on label.about.company above.

    // ── Endonyms ────────────────────────────────────────────────────────────
    ['English',  'endonym — a language name is never translated'],
    ['Français', 'endonym — a language name is never translated'],

    // ── The shared tuning module ────────────────────────────────────────────
    // Not reachable by the coverage scan (the module is not under this plugin's
    // UI root) but recorded here so the decision is on the record rather than
    // being an accident of where the scanner looks.
    ['Tuning panel captions',
     'every caption inside the Tuning tab belongs to the SHARED module ${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine (js/tuning-panel.js + snippets/tuning-panel.css, referenced by path from CMakeLists.txt rather than copied). Localizing it is a cross-plugin change and any local edit here would be reverted by /module-upgrade. A French user therefore still reads the Tuning tab in English'],
];

// ============================================================================
// TIP_BINDINGS — [selector, key] or [selector, key, wrapper] (v1.2.0)
//
// applyI18n() runs document.querySelector(selector), walks closest(wrapper) if
// a wrapper is given, and writes data-tip-title + data-tip onto whatever it
// lands on. setupTooltips() in index.html reads those attributes and paints the
// surface; neither half is any use without the other.
//
// ── NO WRAPPER IS NEEDED ON THIS PAGE, AND THAT IS UNUSUAL ──────────────────
//
// T17 says "bind to the ids the UI already uses", and as on the three pilots
// that is FALSE here: none of the ten knobs carries an id. But unlike those
// pilots the third element of the triple is not needed either, because
// `.knob-control[data-param="..."]` IS the hover cell — a 78px flex column
// holding the 60px SVG, the caption and the readout. Binding the SVG and
// walking up to the column is what the other plugins have to do; here the
// per-parameter element and the cell the user aims at are the same node, so a
// wrapper argument would be a no-op dressed as a decision.
//
// The two chrome anchors are ids, both of which this page already had.
//
// Order matches the page's reading order — Vibrato, Expression, Envelope,
// Voicing & Output, then the header cluster — so a reviewer walking this list
// walks the UI.
// ============================================================================

export const TIP_BINDINGS = [
    ['.knob-control[data-param="vibrato_rate"]',     'tip.rate'],
    ['.knob-control[data-param="vibrato_depth"]',    'tip.depth'],
    ['.knob-control[data-param="vibrato_onset"]',    'tip.onset'],

    ['.knob-control[data-param="breath"]',           'tip.breath'],
    ['.knob-control[data-param="tone"]',             'tip.tone'],
    ['.knob-control[data-param="attack_character"]', 'tip.attackChar'],

    ['.knob-control[data-param="attack_time"]',      'tip.attack'],
    ['.knob-control[data-param="release_time"]',     'tip.release'],

    ['.knob-control[data-param="voice_count"]',      'tip.voices'],
    ['.knob-control[data-param="output_gain"]',      'tip.output'],

    ['#gear-btn',                                    'tip.gearBtn'],
    ['#lang-select',                                 'tip.langSelect'],
];

// The tooltip lookup. Returns {t, b} — never null, never a bare key without a
// console.warn saying so, because a silently-missing tip renders as an empty
// surface that looks like a positioning bug rather than a missing entry.
//
// LIVE as of v1.2.0: applyI18n() calls it once per TIP_BINDINGS entry, twelve
// times per language switch. Its shape is unchanged from v1.1.0 — it was
// exported verbatim while the loop was empty precisely so that adding bodies
// would need no edit here, and it needed none.
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
