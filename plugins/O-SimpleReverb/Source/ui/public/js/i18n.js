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
// i18n.js — O-SimpleReverb page labels and hover-help, English + French (v1.7.2)
//
// ── v1.7.2: THE WORDMARK READS THE REAL VERSION (Stage O item 47, 2026-08-31) ─
//
// No en or fr entry changed. One I18N_EXEMPT entry changed shape: the text-
// matched wordmark-plus-v1.5.5 (decision 7 below — four versions stale, and
// an exemption pinned to a value that moves with every release) became
// "Ouaricon Audio" SCOPED to .footer, and the version number moved into an
// empty span, #versionLabel, that index.html fills at runtime from the new
// getPluginVersion() native function (PluginEditor.cpp, JucePlugin_VersionString
// — the CMake VERSION). The console banner reads the same value. Fallback when
// the function is absent, throws, rejects or returns nothing: the EMPTY string
// — the span stays empty and the banner prints no number. Under the ui-stub the
// span shows the stub's own "v0.0.0-stub".
// Measured (Range over .footer contents, 500 x 350, identical in en and fr):
// the wordmark + v1.5.5 (one span) was 91.66 px; the wordmark + v1.7.2 (two
// spans, the inter-span space adds 1.50) is 93.16 px; "Ouaricon Audio" alone
// (fallback) 64.16 px; "Ouaricon Audio v0.0.0-stub" (ui-stub) 113.16 px. The
// footer is text-align: center with nothing beside it, so no other element
// moves — check-ui-labels is byte-identical to baseline, moved=0.
// After this commit the wordmark and the banner are NOT version sites: a bump
// touches CMakeLists VERSION and this header line only.
//
// ── v1.7.1: FRENCH QA PASS (Stage N, 2026-08-31) ────────────────────────
//
// Every fr entry read against its en and against scripts/i18n-fr-glossary.js.
// Changed: 14 of 29 entries (9 terminology, 10 typography, 1 grammar, 2 meaning
// — the categories overlap; most entries carry a typographic edit as well as a
// word one). sameAsEn: kept 0, translated 0. termNote exemptions: 0.
// ONE straight copy remains and correctly carries NO flag: tip.type's title is
// "Type" in both languages over a fully translated body, and check-i18n reads
// sameAsEn entry-scoped (title AND body) — flagging it would disarm assertion 4
// for the body too.
// Left as drafted: the other 15. reviewed: false throughout — no native speaker
// has read this file, and this pass is a second machine reading, not that one.
//
// Lint (scripts/i18n-fr-lint.js) went 35 → 0 and --strict exits 0: 11 straight
// apostrophes → U+2019, 5 percent signs / 8 colons / 2 semicolons / 2
// number–unit gaps → U+00A0, 5 glossary misses and 2 forbidden words closed.
// The file was drafted with the straight apostrophe throughout, so the
// typography pass touched every body; it ran as a brace-matched state machine
// over the fr: {} blocks and rewrote only the STRING VALUE of a t:/b: pair.
// The control: all 31 en: {} blocks, all keys, TIP_BINDINGS and I18N_EXEMPT are
// byte-identical to the v1.7.0 file, and no U+00A0 landed outside a t:/b: value.
//
// DECISIONS THE NEXT READER NEEDS:
//
//  1. LIRE → OUV, and the ROOT TERM DOES NOT FIT. `load` settles to Charger
//     (Charg.), with Ouvrir/Ouv accepted where the button opens a file dialog —
//     and #preset-load's handler IS loadPresetFromFile() (index.html:1269). The
//     41/44 px pins recorded below are unchanged, so the content box is 30.00 px:
//     CHARGER 51.30, OUVRIR 41.34, CHARG. 40.52 are all past it; OUV is 23.17,
//     6.83 px spare. LIRE (24.22) fitted and was the forbidden word — lire is to
//     read or to play, not to load.
//     NO PERIOD, for the reason ENR already carries: label-in-name (WCAG 2.5.3)
//     matches case-insensitively, "ouv" IS a substring of aria.loadPreset's new
//     "Ouvrir un préréglage" and "ouv." is NOT. OUV. (24.89) would fit the pin
//     just as well and would break the rule silently.
//
//  2. EFFET → TRAITÉ, AND THE PAIR MOVED WITH IT. The glossary settles wet/dry
//     as Traité / Direct. The v1.7.0 note below argues EFFET/DIRECT is a pair a
//     reviewer should move both of or neither — that still holds, and Traité /
//     Direct IS the pair, with DIRECT already the settled term and unchanged.
//     Measured: TRAITÉ 36.80 px against cliff A's 52.00, so it is 15.20 px UNDER
//     the cliff and INERT — the .knob box stays 52.00 and nothing re-centres.
//     SIGNAL TRAITÉ (77.16) is 25.16 px past the cliff and was rejected on it.
//
//  3. THE v1.7.0 WIDTH TABLE WAS RE-MEASURED AND HOLDS TO THE HUNDREDTH. Nine of
//     twenty-one Stage N plugins found their header wrong about the string it
//     defended, so this was measured rather than inherited, with the gates' own
//     method (Range.selectNodeContents on the real node, French, 500 x 350):
//     LOAD 29.78, LIRE 24.22, CHARG. 40.52, OUVRIR 41.34, CHARGER 51.30, SAVE
//     26.83, ENR 21.50, ENREG. 38.84, WET 21.89, EFFET 30.52, DRY 20.91, DIRECT
//     37.31 — every one identical to the number written below.
//
//  4. LOANWORDS AND ON-SCREEN FACES, THREE RULES, UNCHANGED. The six TYPE option
//     faces (Booth … Ambient) and the CHARACTER readout words (warm, bright,
//     neutral) stay ENGLISH in the French bodies because the page shows them in
//     English; the lower-case generic "une cabine" in tip.type is prose and is
//     translated. What DID change is `voicing`, which was an anglicism and not a
//     face — nothing on this page says the word. It is the type-specific EQ
//     (PluginProcessor.h:162-163, TypePreset::eqType), so the French now reads
//     "un filtre de coloration".
//
//  5. TWO BODY TERMS SETTLED BY THE SUITE RATHER THAN BY THIS PAGE. `crossfade`
//     is fondu ENCHAÏNÉ — fondu alone is a fade, which is a different thing from
//     the crossfade tip.wet says this control is not. And `hover help` is aide au
//     survol, the form the other plugins use, where tip.langSelect said "aide
//     contextuelle". Bodies are not matched against the glossary's TERMS, so
//     neither of these was a lint finding; both are the suite converging.
//
//  6. TIP HEIGHTS DID NOT MOVE. Every no-break space is an unbreakable run where
//     a line break used to be possible, and three bodies also grew. Measured in
//     tests/ui_tip_render_check.js at the shipping 500 x 350, all ten French tip
//     rectangles are IDENTICAL before and after — 202.3 / 186.9 / 140.7 / 125.3 /
//     125.3 / 140.7 / 140.7 / 140.7 / 110.0 / 125.3 px tall, with the tightest
//     bottom clearance still tip.lowCut's 21.9 px. Read the HEIGHTS, not only
//     inFrame: a renderer's bottom floor can park a grown tip over the controls
//     above and still report itself inside the frame.
//
//  7. NOT FIXED, REPORTED. The footer wordmark and the console banner both
//     hard-code "v1.5.5" and have been stale since v1.5.7. The footer string is
//     an I18N_EXEMPT entry matched BY TEXT, and Stage N does not re-open
//     exemptions; fixing only the console half would split a documented pair.
//     The repair the exemption already names — an empty span with
//     id="versionLabel" filled at runtime, as O-DigiDelay and O-Tremolo do — is
//     still the right one and still out of a French-copy commit's scope.
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
// ── v1.7.0 GIVES THIS PLUGIN HOVER-HELP, AND A WAY TO SHOW IT ───────────────
//
// v1.6.0 carried no data-tip and no data-tooltip anywhere on the page — only
// four native title= attributes on the preset bar, which contract §4 DELETED
// rather than localized, moving their existing text into data-i18n-aria. I18N
// and TIP_BINDINGS were both empty, which was that version's CORRECT state
// (check-i18n assertion 2 reported it as "0 tip(s) bound") rather than a gap.
//
// v1.7.0 authors ten entries — the eight APVTS parameters plus the gear and
// the language selector — and binds all ten.
//
// AUTHORING THE COPY ALONE WOULD HAVE SHIPPED TEN INVISIBLE STRINGS. applyI18n()
// only WRITES data-tip-title and data-tip onto the anchors named in
// TIP_BINDINGS; the thing that READS those attributes and paints a surface is
// per-plugin code living outside the canon, and this page had none of it — no
// #tooltip element, no .tooltip rule, no hover handler (measured: three greps,
// all zero). All three shipped gates would have stayed green anyway —
// check-i18n assertion 2 only counts bindings, check-ui-labels has no tooltip
// awareness at all, and boot-all-uis counts aria-label and title and never
// data-tip — so the renderer lands in the SAME commit (index.html,
// setupTooltips()), and tests/ui_tip_render_check.js is the gate that can
// actually see a painted tip.
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
// I18N — hover-help copy. {en:{t, b}, fr:{t, b, reviewed}}.
//
// TEN entries: the EIGHT APVTS parameters, plus the gear and the language
// selector. Eight of eight, with nothing reported as unreachable — every row
// of .planning/params.tsv has a control on this page, which is not what the
// batch's other plugins found (O-Bass dumps five and exposes three).
//
// The preset bar deliberately gets none: its four controls took accessible
// names from their deleted title= attributes at v1.6.0 and are self-describing
// (Stage M brief, "Not in M1").
//
// ── THE DECIMAL SEPARATOR IN A FRENCH BODY IS A COMMA ───────────────────────
//
// SETTLED BY THE DEVELOPER, 2026-08-30. v1.7.0 first shipped tip.decay with the
// readout's POINT — "1.0x", "De 0.5x à 2.0x" — on the unstated assumption that
// a body should spell a number the way .value-display does. Two plugins in the
// same batch disagreed with each other about this, and the comma won: it is
// correct French, and every one of the 21 already-shipped tooltip plugins
// already writes it that way.
//
// The READOUT keeps its point. That is D-03, which exempts the readout NODE,
// and it does not move. A body is prose and takes French convention; a readout
// is a machine-formatted value and takes the page's. They differ on purpose.
//
// One string on this page was affected: tip.decay.
//
// ── THE TITLES ARE THE PAGE'S CAPTIONS, AND TWICE THAT IS NOT THE DUMP ──────
//
// The brief's rule is that where the page's caption differs from the
// parameter's name, the CAPTION wins — the user is reading the page, not the
// automation lane. Two rows here diverge, and BOTH diverge in the same
// direction and for the same reason:
//
//   params.tsv name     page caption     tip title
//   LP Filter Freq      LOW CUT          Low Cut
//   LP Filter On        (the ON/OFF      Low Cut On
//                        toggle inside
//                        the same knob)
//
// The parameter IDs and names say "LP Filter", and the DSP is a HIGH-pass:
// PluginProcessor.cpp:603 calls ArrayCoefficients::makeHighPass(sr, lpFreqValue).
// So the parameter's own name is wrong about its own filter and the page's
// caption is right. This is REPORTED, not repaired — renaming an
// AudioParameterFloat changes what a host shows in its automation lane and what
// every saved session's parameter list reads, which is a host-visible change
// unrelated to localization.
//
// ── THE FRENCH TITLE SPELLS OUT WHAT THE CAPTION ABBREVIATES ────────────────
//
// label.lowCut's French is COUPE-B., an abbreviation forced by cliff A (57.22
// px for COUPE-BAS against a 52.00 px knob box — the measurement is in LABELS
// below). A tooltip has no such cliff: it is a fixed, max-width-capped surface
// that owns its own box. So tip.lowCut's French title is the FULL word,
// Coupe-bas, and the tip is the one place on the page a French user can find
// out what the abbreviated caption stands for. Same move O-Emulator made for
// its GB segment and the Game Boy option behind it.
//
// ── RANGES AND UNITS: EVERY ONE RECOVERED FROM THE PAGE'S FORMATTER ─────────
//
// ALL EIGHT ROWS OF params.tsv HAVE AN EMPTY `label` COLUMN. Not one parameter
// declares withLabel(), so the dump gives bare numbers — 0.0 .. 100.0,
// 0.50 .. 2.00, 20 .. 400 — and says nothing about what any of them counts. The
// brief predicts this tendency and then warns that it is a tendency and not a
// fact; here it is the fact, on all eight, so every unit below was read back out
// of the page's own renderer rather than invented:
//
//   WET, DRY, SIZE   %      index.html:1205,1206,1209 — `${(norm*100).toFixed(0)}%`
//   DECAY            x      index.html:1208 — `${(0.5 + 1.5*Math.pow(norm,1.585)).toFixed(1)}x`
//   CHARACTER        %      index.html:1201,1202 — `warm ${...}%` / `bright ${...}%`,
//                           with the bare word `neutral` inside the dead zone
//   LPFREQ           Hz     index.html:714-715 — the two .hz-label spans reading
//                           20 and 400, whose class is the only place the page
//                           spells the unit; the C++ range agrees
//                           (PluginProcessor.cpp:213, 20.0f .. 400.0f)
//   LPON             words  OFF / ON — the ui.off / ui.on LABELS entries, which
//                           this page KEYS (the D-01 arm-3 overrule in LABELS)
//   TYPE             words  the six AudioParameterChoice options
//
// ── WHICH ENGLISH WORDS SURVIVE INTO A FRENCH BODY, AND WHY ─────────────────
//
// Three sets of on-screen words are named inside these bodies, and they do NOT
// all follow the same rule. The discriminator is whether the PAGE localizes the
// word, not whether the word is English:
//
//   1. The six TYPE options — Booth, Room, Hall, Spring, Plate, Ambient — stay
//      ENGLISH in the French body, because the selector keeps them English
//      (D-01 arm 1, I18N_EXEMPT below: the page and the host automation lane
//      must agree). A French body naming them "Cabine" would name something the
//      user cannot find in the dropdown.
//   2. The CHARACTER readout words — warm, bright, neutral — stay ENGLISH for a
//      different reason: #CHARACTER-value is a READOUT node, exempt under D-01
//      arm 3, so the formatter writes the same English in both languages. Same
//      outcome, different arm.
//   3. The low-cut toggle's caption is the opposite case. #LPFREQ-value IS
//      keyed (the arm-3 overrule recorded in LABELS), so it reads OFF/ON in
//      English and DÉS./ACT. in French. The French bodies of tip.lowCut and
//      tip.lowCutOn therefore say DÉS. and ACT., because that is what the user
//      is looking at. A body that said "ON" there would be pointing at a word
//      the French page does not contain.
//
// ── NUMBERS INSIDE A BODY ARE PROSE (D-03) ─────────────────────────────────
//
// D-03 exempts readout NODES, not digits. `0 to 100 %` becomes `0 à 100 %` and
// `0.5x to 2.0x` becomes `de 0,5x à 2,0x` — French spacing before the percent
// sign AND the French decimal comma, exactly as the 21 already-shipped tooltip
// plugins do it. This sentence originally claimed the comma while the entry
// below shipped the point; both now say comma.
// ============================================================================

export const I18N = Object.freeze({

    // ── The eight parameters, in the page's own top-to-bottom order ─────────

    // TYPE — AudioParameterChoice, six options, default Room (index 1).
    // The French title is byte-identical to the English: "Type" is the same
    // word for the same thing in both languages. The BODY differs, so
    // assertion 4 (which flags an entry only when t AND b both match) does not
    // fire and no sameAsEn flag is needed — `reviewed: false` keeps the entry
    // in the native-speaker worklist regardless.
    //
    // The "still a booth" sentence is the one thing a user cannot discover by
    // turning Size: finalRoomSize = preset.baseRoomSize * (0.5 + size/2)
    // (PluginProcessor.cpp:432-433), and Booth's base is 0.15 against Hall's
    // 0.85, so Booth at 100 % (0.150) really is smaller than Hall at 0 %
    // (0.425).
    'tip.type': {
        en: { t: "Type",
              b: "Picks the reverb algorithm: each name is a whole configuration — room size, damping, stereo width, pre-delay, early-reflection spread, and on most types a voicing filter — not just a bigger or smaller room. Size and Decay then scale whatever the type sets, so Size at 100 % on Booth is still a booth. Six settings: Booth, Room, Hall, Spring, Plate, Ambient." },
        fr: { t: "Type",
              b: "Choisit l’algorithme de réverbération : chaque nom est une configuration complète — taille de la pièce, amortissement, largeur stéréo, pré-délai, étalement des premières réflexions et, sur la plupart des types, un filtre de coloration — et non simplement une pièce plus grande ou plus petite. Taille et Déclin viennent ensuite mettre à l’échelle ce que le type a posé : Taille à 100 % sur Booth reste une cabine. Six réglages : Booth, Room, Hall, Spring, Plate, Ambient.",
              reviewed: true },
    },

    // CHARACTER — AudioParameterFloat, -100 .. +100, default 0.
    // The dead zone is the sentence, because it is what the readout's third
    // state means: PluginProcessor.cpp:563-568 puts Warm below -0.5, Bright
    // above +0.5 and Neutral between, and Neutral runs no filter at all.
    // Warm is a low-pass from 20 kHz down to 2 kHz (line 581); Bright is a
    // 4 kHz high shelf reaching +6 dB (lines 589-592). Both act on wetContext
    // only, which is why "the dry signal is never filtered" is safe to say.
    'tip.character': {
        en: { t: "Character",
              b: "Tilts the tone of the reverb tail only; the dry signal is never filtered. Turned left it is warm, a low-pass closing from 20 kHz down to 2 kHz, and turned right it is bright, a high shelf at 4 kHz reaching +6 dB. The readout reads neutral inside a narrow dead zone at the centre, where the filter is bypassed outright: warm 100 % to bright 100 %." },
        fr: { t: "Caractère",
              b: "Incline la couleur de la seule queue de réverbération ; le signal direct n’est jamais filtré. Vers la gauche le son est chaud, un passe-bas qui se referme de 20 kHz jusqu’à 2 kHz, et vers la droite il est brillant, un filtre en plateau aigu à 4 kHz atteignant +6 dB. L’affichage indique neutral dans une étroite zone morte au centre, où le filtre est totalement contourné : de warm 100 % à bright 100 %.",
              reviewed: true },
    },

    // WET — AudioParameterFloat, 0 .. 100 %, default 25.
    // Independent GAIN, not a crossfade: the mix loop at
    // PluginProcessor.cpp:617 is `dry*dryGain + wet*wetGain` with the two
    // read from separate parameters. That is the non-obvious half and it is
    // what the tip is for; the percentage a user can read off the knob.
    'tip.wet': {
        en: { t: "Wet",
              b: "Sets how much reverb is added to the output. It is an independent gain and not a crossfade: turning it up does not turn Dry down, so the two together set the total level as well as the balance. 0 to 100 %." },
        fr: { t: "Traité",
              b: "Règle la quantité de réverbération ajoutée à la sortie. C’est un gain indépendant et non un fondu enchaîné : l’augmenter ne baisse pas Direct, si bien que les deux ensemble déterminent aussi le niveau global et pas seulement l’équilibre. 0 à 100 %.",
              reviewed: true },
    },

    // DRY — AudioParameterFloat, 0 .. 100 %, default 100.
    // The other half of the same finding, from the other side.
    'tip.dry': {
        en: { t: "Dry",
              b: "Sets how much of the untouched input reaches the output. It is independent of Wet, so pulling it to 0 leaves the reverb tail alone on the output — the setting to use on a send bus — while leaving it at 100 keeps the source at full level. 0 to 100 %." },
        fr: { t: "Direct",
              b: "Règle la quantité de signal d’entrée intact qui atteint la sortie. Elle est indépendante de Traité : la ramener à 0 ne laisse que la queue de réverbération en sortie — le réglage à utiliser sur un bus de départ — tandis que la laisser à 100 conserve la source à plein niveau. 0 à 100 %.",
              reviewed: true },
    },

    // DECAY — AudioParameterFloat, 0.5 .. 2.0, default 1.0, skew 0.6309.
    // "The centre is exactly 1.0x" is a measured claim, not a rounding: the
    // skew was chosen to put 1.0x at the knob's midpoint (the CR-02 note at
    // PluginProcessor.cpp:188-193) and params.tsv agrees — defaultNorm
    // 0.500016 renders defaultText 1.00. It scales room size UP and damping
    // DOWN together (lines 435-441), which is why it reads as a multiplier
    // rather than a time.
    'tip.decay': {
        en: { t: "Decay",
              b: "Stretches or shortens the tail the Type set, by growing the room and easing its damping together. The centre of the knob is exactly 1.0x — the type's own untouched decay — so this is a trim, not a time in seconds. 0.5x to 2.0x." },
        fr: { t: "Déclin",
              b: "Allonge ou raccourcit la queue posée par le Type, en agrandissant la pièce et en relâchant son amortissement à la fois. Le centre du bouton vaut exactement 1,0x, soit le déclin propre du type : c’est donc un ajustement et non une durée en secondes. De 0,5x à 2,0x.",
              reviewed: true },
    },

    // SIZE — AudioParameterFloat, 0 .. 100 %, default 50.
    // Relative, never absolute. See the arithmetic in tip.type's comment.
    'tip.size': {
        en: { t: "Size",
              b: "Scales the room the Type chose, from half its size at 0 % to its full size at 100 %. It is relative rather than absolute — Booth at 100 % is still smaller than Hall at 0 % — and it moves only the space, leaving the length of the tail to Decay. 0 to 100 %." },
        fr: { t: "Taille",
              b: "Met à l’échelle la pièce choisie par le Type, de la moitié de sa taille à 0 % jusqu’à sa taille entière à 100 %. C’est une valeur relative et non absolue — Booth à 100 % reste plus petit que Hall à 0 % — et elle n’agit que sur l’espace, la longueur de la queue restant l’affaire de Déclin. 0 à 100 %.",
              reviewed: true },
    },

    // LPFREQ — AudioParameterFloat, 20 .. 400 Hz, default 200.
    // Title from the CAPTION, not from the dump's "LP Filter Freq": the DSP is
    // makeHighPass (PluginProcessor.cpp:603) and the caption is the half that
    // is right. It runs on wetContext only and ONLY when LPON is on
    // (line 600), which is the sentence a user cannot get from the dial.
    // The French title is the full word where the caption is COUPE-B.
    'tip.lowCut': {
        en: { t: "Low Cut",
              b: "A high-pass on the reverb tail only, for clearing mud out of the bottom of the space without thinning the dry signal. It does nothing at all until the switch below the dial reads ON, and the two small figures either side of the dial are the ends of its travel. 20 to 400 Hz." },
        fr: { t: "Coupe-bas",
              b: "Un passe-haut appliqué à la seule queue de réverbération, pour dégager le bas du spectre sans amaigrir le signal direct. Il ne fait rien du tout tant que l’interrupteur sous le cadran n’affiche pas ACT., et les deux petits chiffres de part et d’autre du cadran sont les extrémités de sa course. De 20 à 400 Hz.",
              reviewed: true },
    },

    // LPON — AudioParameterFloat over NormalisableRange(0, 1, 1), default 0.
    // NOT an AudioParameterBool and NOT an AudioParameterChoice, so D-01 arm 1
    // cannot fire on its two words and there is no host option string for the
    // body to disagree with. Its control is #LPFREQ-value, the same node that
    // displays the state — the one anchor on this page that is nested inside
    // another anchor's wrapper (see TIP_BINDINGS).
    'tip.lowCutOn': {
        en: { t: "Low Cut On",
              b: "Switches the low cut in and out; click the word itself to toggle it. While it reads OFF the dial above is dimmed and the filter is bypassed entirely, so an unused low cut costs nothing. Two settings: OFF and ON." },
        fr: { t: "Coupe-bas actif",
              b: "Met le coupe-bas en service ou hors service ; cliquez sur le mot lui-même pour basculer. Tant qu’il affiche DÉS., le cadran au-dessus est estompé et le filtre est totalement contourné : un coupe-bas inutilisé ne coûte rien. Deux réglages : DÉS. et ACT.",
              reviewed: true },
    },

    // ── The two chrome controls ─────────────────────────────────────────────

    // The gear tip is what tells a user hover-help exists at all, so its body
    // describes ONLY what the popover actually contains. This plugin has no
    // hover-help on/off toggle — not a C++ one, not a localStorage one — so the
    // panel holds the language selector and nothing else, and the tip says
    // exactly that. O-Tapestop's wording promises a toggle that does not exist
    // here; a tip that lies is worse than no tip.
    'tip.gearBtn': {
        en: { t: "Settings",
              b: "Opens the settings panel, which on this plugin holds a single row: the interface language. Nothing else lives in there — every reverb control is on the front panel." },
        fr: { t: "Réglages",
              b: "Ouvre le panneau de réglages, qui ne contient sur ce plugin qu’une seule ligne : la langue de l’interface. Rien d’autre ne s’y trouve — toutes les commandes de la réverbération sont sur la face avant.",
              reviewed: true },
    },

    // The two endonyms are named as the selector spells them, and the selector
    // never translates them (I18N_EXEMPT below). "Saved with the plugin's
    // state" is measured, not assumed: setUiLanguage writes
    // processorRef.uiLanguage (PluginEditor.cpp:153-163) and
    // getStateInformation persists it onto parameters.state as the uiLanguage
    // property (PluginProcessor.cpp:659-660).
    'tip.langSelect': {
        en: { t: "Interface language",
              b: "Switches every caption, accessible name and hover-help on this page between English and French. The change is immediate and is saved with the plugin's state, so it comes back with the session. Two settings: English and Français." },
        fr: { t: "Langue de l’interface",
              b: "Bascule toutes les légendes, tous les noms accessibles et toutes les infobulles de cette page entre l’anglais et le français. Le changement est immédiat et il est enregistré avec l’état du plugin : il revient donc avec la session. Deux réglages : English et Français.",
              reviewed: true },
    },
    // v1.8.0 — the switch that reaches this whole layer.
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
    'label.character': { en: { t: 'Character' }, fr: { t: 'Caractère', reviewed: true } },

    // COUPE-B. IS AN ABBREVIATION AND THE FULL FORM WAS REJECTED ON
    // MEASUREMENT, NOT ON TASTE. COUPE-BAS is 57.22 — 5.22 px PAST cliff A —
    // and would move every child of the LPFREQ knob 2.61 px in French only.
    // COUPE BAS (56.47) and PASSE-HAUT (62.19, the technically correct name
    // for a low cut) are worse. Under the cliff and available to a reviewer:
    // FILTRE (34.48), which is unambiguous because this page has exactly one
    // filter but drops the word "low"; C. BAS (32.81); PASSE-HT (47.59).
    // COUPE-B. keeps the low-cut meaning at 4.84 px of headroom.
    'label.lowCut':    { en: { t: 'Low Cut' },   fr: { t: 'Coupe-b.', reviewed: true } },

    // EFFET / DIRECT is the pair French effects units are silk-screened with,
    // and it is a pair — the reviewer should move both or neither. HUMIDE
    // (40.80) / SEC (18.91) is the literal translation and both members also
    // fit with room to spare, so this is a register choice rather than a
    // geometry one and is recorded as such. DIRECT is 16.40 px WIDER than DRY
    // and that costs nothing, because 37.31 is still 14.69 px under the cliff:
    // measuring the cliff is what turns a scary-looking growth into a non-event.
    'label.wet':       { en: { t: 'Wet' },       fr: { t: 'Traité',   reviewed: true } },
    'label.dry':       { en: { t: 'Dry' },       fr: { t: 'Direct',   reviewed: true } },

    // DÉCROISSANCE is the full word and it is 77.63 — a SINGLE WORD with no
    // break opportunity, 25.63 px past cliff A. It stops short of cliff C
    // (97.50) so it would not steal from the neighbouring columns, but it
    // would move every child of the DECAY knob 12.8 px. DÉCROISS. (52.97) is
    // past the cliff by 0.97 px, which is a miss by rounding. DÉCLIN is the
    // word French reverb manuals use and it fits with 14.20 px to spare;
    // CHUTE (34.00) is the roomier alternative.
    'label.decay':     { en: { t: 'Decay' },     fr: { t: 'Déclin',   reviewed: true } },
    'label.size':      { en: { t: 'Size' },      fr: { t: 'Taille',   reviewed: true } },

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
    'ui.on':           { en: { t: 'ON' },        fr: { t: 'ACT.',     reviewed: true } },
    'ui.off':          { en: { t: 'OFF' },       fr: { t: 'DÉS.',     reviewed: true } },

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
    'label.save':      { en: { t: 'Save' },      fr: { t: 'Enr',      reviewed: true } },
    'label.load':      { en: { t: 'Load' },      fr: { t: 'Ouv',      reviewed: true } },

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
    'label.language':  { en: { t: 'Language' },  fr: { t: 'Langue',   reviewed: true } },

    // v1.8.0. All four renderings below are settled glossary ROOTS, copied
    // rather than authored: scripts/i18n-fr-glossary.js carries them as the
    // roots for 'hover help', 'on', 'off' and 'toggle hover help'. They take
    // the same review mark this file's other roots carry, and for the same
    // reason — they are not new machine output.
    'label.hoverHelp': { en: { t: 'Hover help' }, fr: { t: 'Infobulles', reviewed: true } },

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
    'alt.background':  { en: { t: 'Background' }, fr: { t: 'Arrière-plan',    reviewed: true } },
    'alt.botanical':   { en: { t: 'Botanical' },  fr: { t: 'Motif botanique', reviewed: true } },

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
                         fr: { t: 'Préréglage précédent', reviewed: true } },
    'aria.nextPreset': { en: { t: 'Next preset' },
                         fr: { t: 'Préréglage suivant',   reviewed: true } },
    'aria.savePreset': { en: { t: 'Save preset' },
                         fr: { t: 'Enregistrer le préréglage', reviewed: true } },
    'aria.loadPreset': { en: { t: 'Load preset' },
                         fr: { t: 'Ouvrir un préréglage',      reviewed: true } },

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

    // ── The footer wordmark ─────────────────────────────────────────────────
    //
    // v1.7.2 (Stage O item 47): SCOPED to .footer, and the version number is
    // no longer part of the exempted text. Through v1.7.1 this entry matched
    // the wordmark-plus-v1.5.5 literal by TEXT — an exemption pinned to a
    // value that changes with every release, and one that had been four
    // versions stale. The version now lives in an EMPTY span, #versionLabel,
    // filled at runtime from getPluginVersion() (JucePlugin_VersionString),
    // the O-DigiDelay / O-Tremolo shape. An empty span is not a text node, so
    // it needs no exemption; what it holds at runtime is a readout (D-03).
    ['Ouaricon Audio',
     'the company name in .footer — a company name is never translated. The span beside it '
     + '(#versionLabel) is filled at runtime from getPluginVersion() and holds a version number, '
     + 'which is a readout (D-03)',
     '.footer'],

    // ── Endonyms ────────────────────────────────────────────────────────────
    ['English',  'endonym — a language name is never translated'],
    ['Français', 'endonym — a language name is never translated'],
];

// ============================================================================
// TIP_BINDINGS — [selector, key, wrapper]. TEN, one per I18N entry.
//
// applyI18n() runs document.querySelector(selector), walks closest(wrapper)
// when a wrapper is given, and writes data-tip-title + data-tip onto whatever
// that lands on. The renderer in index.html then reads those attributes off
// the nearest ancestor-or-self carrying data-tip.
//
// ── THE WRAPPER IS THE GRID CELL, NOT THE 3 px SVG STROKE ───────────────────
//
// T17 says "bind to the ids the UI already uses". On this page the ids exist —
// six #X-knob divs, #TYPE, #gear-btn, #lang-select — so the SELECTORS are all
// ids here, which is the first plugin in this stage where that claim holds.
// But an id is not automatically the right hover TARGET. A .knob is
// `column; align-items: center` and is only max(labelWidth, 52) px wide; its
// .knob-control parent is the `1fr` grid cell, 97.50 px on the top row and
// 124.66 px on the bottom, and the gaps between cells belong to nobody. So the
// six knobs bind through closest('.knob-control') and the user aims at the
// whole column, caption and readout included, not at the dial alone.
//
// ── ONE ANCHOR IS NESTED INSIDE ANOTHER, DELIBERATELY ───────────────────────
//
// #LPFREQ-value is the LPON control (a click on it flips the parameter) AND it
// sits inside #LPFREQ-knob, inside the .knob-control that carries tip.lowCut.
// So the low-cut cell holds two anchors, one inside the other:
//
//   .knob-control  (from #LPFREQ-knob)  -> tip.lowCut
//     #LPFREQ-value                     -> tip.lowCutOn
//
// This resolves correctly and does NOT need ordering luck, for two separate
// reasons. The two bindings write to two DIFFERENT nodes, so neither can
// overwrite the other's attributes whichever order the loop runs in. And the
// renderer resolves an anchor with closest('[data-tip]'), which is
// ancestor-OR-SELF and stops at the innermost match — so the pointer gets
// tip.lowCutOn over the ON/OFF word and tip.lowCut everywhere else in the
// cell. Moving between the two fires a real pointerout with a relatedTarget
// that is NOT inside the anchor being left, so the surface is refilled rather
// than left stale.
//
// Giving #LPFREQ-value the '.knob-control' wrapper instead would have
// collapsed both bindings onto the SAME node, and the second write would have
// silently won — one parameter's tip lost with every gate still green.
//
// ── ORDER IS THE PAGE'S READING ORDER ───────────────────────────────────────
//
// Type, then the top knob row left to right, then the bottom row, then the two
// chrome controls. Nothing depends on it; it is here so a reader can check the
// list against the page without cross-referencing.
// ============================================================================

export const TIP_BINDINGS = [
    ['#TYPE',           'tip.type'],

    ['#CHARACTER-knob', 'tip.character', '.knob-control'],
    ['#LPFREQ-knob',    'tip.lowCut',    '.knob-control'],
    ['#LPFREQ-value',   'tip.lowCutOn'],
    ['#WET-knob',       'tip.wet',       '.knob-control'],
    ['#DRY-knob',       'tip.dry',       '.knob-control'],

    ['#DECAY-knob',     'tip.decay',     '.knob-control'],
    ['#SIZE-knob',      'tip.size',      '.knob-control'],

    ['#gear-btn',       'tip.gearBtn'],
    ['#lang-select',    'tip.langSelect'],
    ['#tips-toggle',    'tip.tipsToggle'],
];

// The tooltip lookup. Returns {t, b} — never null, never a bare key without a
// console.warn saying so, because a silently-missing tip renders as an empty
// surface that looks like a positioning bug rather than a missing entry.
//
// LIVE as of v1.7.0: applyI18n() calls it once per TIP_BINDINGS row, ten times
// per language change. It was exported verbatim through v1.6.0 while the list
// was empty so the canon block stayed byte-identical to the other forty-two
// copies; nothing in its shape had to change to turn it on.
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
