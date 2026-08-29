/*
   This file is part of O-Gain, an Ouaricon Audio plugin.
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
// i18n.js — O-Gain UI copy, English + French (v1.3.0, canon v2)
//
// An ES module that EXPORTS ONLY. It must never self-execute: a bare top-level
// statement here throws out of module evaluation and takes every later
// initializer on the page with it (pattern_module_toplevel_init_tdz).
// scripts/check-i18n.js assertion 7 enforces that.
//
// SERVED ROOT IS Source/ui/public, read from CMakeLists.txt before a byte was
// written here. THE BINARY-DATA TARGET CARRIES NO NAMESPACE ARGUMENT —
// juce_add_binary_data(O-Gain_UIResources SOURCES ...) takes the default
// BinaryData namespace and works only because it is the only such target in
// this plugin. This file and js/app.js were added to that EXISTING SOURCES
// list; a second juce_add_binary_data target would collide on the BinaryData
// namespace and break the build in a way that reads like something else
// entirely (critical_dual_binary_data_namespace_collision).
//
// FOUR PLACES, ONE COMMIT — TWICE OVER, because v1.3.0 also EXTRACTED the
// 447-line inline <script type="module"> into js/app.js. Each of the two new
// files needs: the file on disk, the CMake SOURCES entry, a getResource()
// branch in PluginEditor.cpp, and a reference from the page (index.html's
// <script src> for app.js, app.js's import for i18n.js). Miss one and the page
// 404s at runtime and presents as a dead panel with no other symptom
// (assertion 8).
//
// FILENAME: no hyphen. juce_add_binary_data STRIPS hyphens rather than
// converting them to underscores, so a second file named i18n-fr.js would have
// to be reached as the symbol i18nfr_js (critical_binary_data_strips_hyphens).
// One combined file for both languages sidesteps the question entirely.
//
// NO MARKUP. This table is data, never HTML. The tooltip renderer in js/app.js
// builds the tip with createElement + textContent, and check-i18n assertion 9
// rejects any innerHTML reference here and any string literal containing the
// opening angle bracket.
//
// ── THE COUNTS, PARSED OUT OF THE RENDERED DOM ─────────────────────────────
// The plan's table says "23 tips / 41 static text nodes / 2 JS prose", and a
// grep for the data-tooltip token returns 34 hits. Rendered headless through
// scripts/serve-ui.js at the shipping 350 x 500 frame and walked with a
// TreeWalker rather than grepped:
//
//     data-tooltip LIVE ANCHORS           23   (unique strings: 23)
//     rendered text nodes                 40   (29 visible, 11 inside the
//                                               display:none Learn panel)
//     native title=                        1   (#help-btn "Toggle tooltips")
//     aria-label / placeholder             0
//     alt                                  1   (empty — decorative, correct)
//
// The tip count survived: 23 anchors, 23 unique strings, no injected template.
// The 34-hit grep was 23 attributes plus 11 CSS selector/property mentions in
// the `[data-tooltip]::after` block. The text count did not survive: 40, not
// 41. There is no shared registry module on this page and no injected node —
// the plan simply counted one node that is not there.
//
// ── THE SPLIT: 0 CLEAN, 23 HAND-SPLIT ──────────────────────────────────────
// The plan expects copy authored as "Label: sentence." and warns that the
// shape usually does not hold. HERE IT HOLDS ON NONE OF THE TWENTY-THREE.
// O-Gain's tooltip copy is bare sentences with no title prefix at all, so
// every title below is the control's OWN EXISTING ENGLISH CAPTION, reused
// verbatim from index.html — never authored.
//
// FOUR of the twenty-three DO contain a ": ", and a mechanical split on the
// first one would have produced a title that is most of the sentence:
//
//     gain-display  "...adjust manually. Range: -40 to +40 dB"
//     trim-knob     "...Range: -6 to +6 dB. Double-click to reset"
//     mono          "Sum to mono: (L+R)/2 on both channels"
//     ms-dec        "...Note: decoding a normal (un-encoded) L/R signal..."
//
// Each of those colons is inside a sentence. All four bodies below are
// byte-identical to v1.2.1's attribute values, colon included.
//
// ── FRENCH IS SIZED, NOT SHRUNK ────────────────────────────────────────────
// 350 x 500 is the NARROWEST frame in the repo. D-04 forbids an auto-shrink
// font and a short-variant fallback: exactly ONE French string per key,
// nothing chooses between variants at runtime, and every caption below was
// MEASURED AS RENDERED inside its own element. text-transform: uppercase and
// letter-spacing are not in getComputedStyle().font, so a font probe reads
// short and a pin lands under the French.
//
// ALL FRENCH IS MACHINE-DRAFTED AND FLAGGED `reviewed: false`. No native
// speaker has read it. `node scripts/check-i18n.js` prints the worklist.
// ============================================================================

export const LANGUAGES = ['en', 'fr'];

// key -> { en: {t, b}, fr: {t, b, reviewed} }
//   t = tooltip title (the small-caps line), b = tooltip body.
//
// Object.freeze() rather than a bare `{...}` literal for two reasons. It says
// out loud that this module is inert data nothing may mutate at runtime — and
// it keeps the export a SINGLE top-level statement, because a statement written
// `export const X = {...};` closes its brace at depth zero and segments the
// trailing `;` off on its own.
export const I18N = Object.freeze({

    // ── The settings popover (v1.3.0) ───────────────────────────────────────
    // New controls, new copy. The gear takes the exact absolute slot the "?"
    // help button occupied through v1.2.1 (right: 8px; top: 0 inside the 22 px
    // header), so nothing on a 350 x 500 layout had to move to make room for
    // it, and the hover-help switch moves inside beside the language selector.
    'settings': {
        en: { t: 'Settings',
              b: 'Choose the language of this interface and whether hover help appears. The language is remembered with the session; the hover-help switch is not.' },
        fr: { t: 'Réglages',
              b: 'Choisir la langue de cette interface et l’affichage de l’aide au survol. La langue est conservée avec la session ; l’interrupteur d’aide ne l’est pas.',
              reviewed: false },
    },
    'lang-select': {
        en: { t: 'Language',
              b: 'The language of this hover help and of the labels on the page. English and French are available; value readouts, meter-mode names and unit symbols stay in English.' },
        fr: { t: 'Langue',
              b: 'La langue de cette aide au survol et des libellés de la page. L’anglais et le français sont disponibles ; les valeurs affichées, les noms de modes de vumètre et les symboles d’unité restent en anglais.',
              reviewed: false },
    },
    'tips-toggle': {
        en: { t: 'Hover Help',
              b: 'Turns this hover help on and off. With it off, only the gear and this switch keep explaining themselves.' },
        fr: { t: 'Aide au survol',
              b: 'Active ou désactive cette aide au survol. Une fois désactivée, seuls l’engrenage et ce commutateur continuent de s’expliquer.',
              reviewed: false },
    },

    // ── The twenty-three ported tips. NONE split on a ": ". Every title is
    //    the control's own caption, reused verbatim from v1.2.1's markup.
    'input-meter': {
        en: { t: 'Input',
              b: 'Input level after channel utilities, before gain is applied' },
        fr: { t: 'Entrée',
              b: 'Niveau d’entrée après les utilitaires de canal, avant application du gain',
              reviewed: false },
    },
    'output-meter': {
        en: { t: 'Output',
              b: 'Output level after gain is applied' },
        fr: { t: 'Sortie',
              b: 'Niveau de sortie après application du gain',
              reviewed: false },
    },
    'gain-display': {
        en: { t: 'Gain Offset',
              b: 'Main gain offset. Set by Learn or adjust manually. Range: -40 to +40 dB' },
        fr: { t: 'Décalage de gain',
              b: 'Décalage de gain principal. Défini par la mesure ou réglé à la main. Plage : -40 à +40 dB',
              reviewed: false },
    },
    'gain-knob': {
        en: { t: 'Gain',
              b: 'Drag to set gain. Double-click to reset. Hold Shift for fine control' },
        fr: { t: 'Gain',
              b: 'Glisser pour régler le gain. Double-cliquer pour réinitialiser. Maintenir Maj pour un réglage fin',
              reviewed: false },
    },
    'trim-knob': {
        en: { t: 'Trim',
              b: 'Fine adjustment after Learn mode. Range: -6 to +6 dB. Double-click to reset' },
        fr: { t: 'Ajust.',
              b: 'Réglage fin après une mesure. Plage : -6 à +6 dB. Double-cliquer pour réinitialiser',
              reviewed: false },
    },
    'learn-btn': {
        en: { t: 'Learn',
              b: 'Click to start measuring. Play audio for 10-30s, then click again. Gain is set automatically' },
        fr: { t: 'Mesurer',
              b: 'Cliquer pour lancer la mesure. Jouer 10 à 30 s d’audio, puis cliquer de nouveau. Le gain est réglé automatiquement',
              reviewed: false },
    },
    'target-group': {
        en: { t: 'Target',
              b: 'Target loudness for Learn. -18 dB = 0 VU (standard), -14 LUFS = Spotify, -23 LUFS = EBU R128' },
        fr: { t: 'Cible',
              b: 'Sonie visée par la mesure. -18 dB = 0 VU (norme), -14 LUFS = Spotify, -23 LUFS = EBU R128',
              reviewed: false },
    },
    'target-knob': {
        en: { t: 'Target Level',
              b: 'Desired output loudness. Drag knob or double-click to reset to -18 dB' },
        fr: { t: 'Niveau cible',
              b: 'Sonie de sortie souhaitée. Glisser le bouton ou double-cliquer pour revenir à -18 dB',
              reviewed: false },
    },
    'measure-mode': {
        en: { t: 'Measure',
              b: 'Algorithm used by Learn. LUFS = K-weighted loudness (recommended, industry standard). RMS = simple average level' },
        fr: { t: 'Mesure',
              b: 'Algorithme utilisé par la mesure. LUFS = sonie pondérée K (recommandé, norme du métier). RMS = niveau moyen simple',
              reviewed: false },
    },
    'meter-mode': {
        en: { t: 'Meter',
              b: 'Meter display type. Peak = instantaneous peaks. RMS = average level. VU = analog-style 300ms ballistics. LUFS = K-weighted momentary loudness (shown while Learn runs; falls back to RMS otherwise)' },
        fr: { t: 'Vumètre',
              b: 'Type d’affichage du vumètre. Peak = crêtes instantanées. RMS = niveau moyen. VU = balistique analogique de 300 ms. LUFS = sonie momentanée pondérée K (affichée pendant la mesure ; sinon retour au RMS)',
              reviewed: false },
    },
    'info-momentary': {
        en: { t: 'Momentary',
              b: 'Loudness of the last 400ms window' },
        fr: { t: 'Momentané',
              b: 'Sonie de la fenêtre des 400 dernières ms',
              reviewed: false },
    },
    'info-short-term': {
        en: { t: 'Short-term',
              b: 'Loudness averaged over the last 3 seconds' },
        fr: { t: 'Court terme',
              b: 'Sonie moyennée sur les 3 dernières secondes',
              reviewed: false },
    },
    'info-integrated': {
        en: { t: 'Integrated',
              b: 'Overall loudness since Learn started (gated, used for gain calculation)' },
        fr: { t: 'Intégré',
              b: 'Sonie globale depuis le début de la mesure (avec seuillage, utilisée pour le calcul du gain)',
              reviewed: false },
    },
    'info-sample-peak': {
        en: { t: 'Sample Peak',
              b: 'Highest digital sample peak (not oversampled, so inter-sample peaks are not measured). Learn leaves ~3 dB of extra headroom below -1 dBFS to cover them.' },
        fr: { t: 'Crête éch.',
              b: 'Crête numérique la plus haute (sans suréchantillonnage, donc les crêtes inter-échantillons ne sont pas mesurées). La mesure laisse environ 3 dB de marge supplémentaire sous -1 dBFS pour les couvrir.',
              reviewed: false },
    },
    'info-elapsed': {
        en: { t: 'Elapsed',
              b: 'How long Learn has been running. 10-30 seconds recommended' },
        fr: { t: 'Écoulé',
              b: 'Durée de la mesure en cours. 10 à 30 secondes recommandées',
              reviewed: false },
    },
    'info-confidence': {
        en: { t: 'Confidence',
              b: 'Measurement quality. Low = under 5s. Medium = 5-15s. High = over 15s with stable signal' },
        fr: { t: 'Confiance',
              b: 'Qualité de la mesure. Bas = moins de 5 s. Moyen = 5 à 15 s. Haut = plus de 15 s avec un signal stable',
              reviewed: false },
    },
    'phase-l': {
        en: { t: 'PH L',
              b: 'Flip left channel polarity (180 degree phase inversion)' },
        fr: { t: 'PH G',
              b: 'Inverse la polarité du canal gauche (inversion de phase de 180 degrés)',
              reviewed: false },
    },
    'phase-r': {
        en: { t: 'PH R',
              b: 'Flip right channel polarity (180 degree phase inversion)' },
        fr: { t: 'PH D',
              b: 'Inverse la polarité du canal droit (inversion de phase de 180 degrés)',
              reviewed: false },
    },
    'swap': {
        en: { t: 'SWAP',
              b: 'Swap left and right channels' },
        fr: { t: 'PERM',
              b: 'Permute les canaux gauche et droit',
              reviewed: false },
    },
    'mono': {
        en: { t: 'MONO',
              b: 'Sum to mono: (L+R)/2 on both channels' },
        fr: { t: 'MONO',
              b: 'Somme en mono : (G+D)/2 sur les deux canaux',
              reviewed: false },
    },
    'ms-off': {
        en: { t: 'M/S OFF',
              b: 'Mid-Side processing off (normal stereo)' },
        fr: { t: 'M/S ARR',
              b: 'Traitement Mid-Side désactivé (stéréo normale)',
              reviewed: false },
    },
    'ms-enc': {
        en: { t: 'ENC',
              b: 'Encode L/R to Mid/Side. Use with a second O-Gain set to DEC after processing' },
        fr: { t: 'ENC',
              b: 'Encode G/D en Mid/Side. À utiliser avec un second O-Gain réglé sur DÉC après le traitement',
              reviewed: false },
    },
    'ms-dec': {
        en: { t: 'DEC',
              b: 'Decode Mid/Side back to L/R. Place after M/S processing chain. Note: decoding a normal (un-encoded) L/R signal raises level by +6 dB — the inverse gain of ENC — so use ENC->DEC as a matched pair.' },
        fr: { t: 'DÉC',
              b: 'Décode le Mid/Side vers G/D. À placer après la chaîne de traitement M/S. Remarque : décoder un signal G/D normal (non encodé) élève le niveau de +6 dB — le gain inverse d’ENC — donc utiliser ENC->DÉC comme paire appariée.',
              reviewed: false },
    },
});

// ============================================================================
// LABELS — the on-page text (v1.3.0, canon v2)
// ============================================================================
//
// I18N above is HOVER-HELP copy: a title and a body rendered into a wrapping
// 220 px tooltip. LABELS is ON-PAGE copy: one string dropped into a cell that
// mostly does not wrap. They are different problems and this table keeps them
// apart on purpose.
//
// trLabel() falls back to I18N when a key is absent here, so a control whose
// tooltip TITLE already IS its caption could carry ONE key. That is
// deliberately NOT done: the two tables are kept disjoint so a later tooltip
// rewrite cannot silently move a knob caption.
//
// ── ENGLISH WAS MOVED, NOT RE-TYPED ────────────────────────────────────────
// Every `en` below is what index.html carried through v1.2.1, taken from
// scripts/i18n-extract.js's inventory rather than transcribed. There is no
// contract §6 rewrite on this plugin: not one visible string composes a count
// with an inflected noun, so no copy had to be authored around a plural.
//
// ── WHERE THE 350 px FRAME BIT, AND WHAT WAS MEASURED ──────────────────────
// Four captions were sized against a hard box rather than chosen by ear. Each
// number below is getBoundingClientRect().width AS RENDERED in that caption's
// own element, at the shipping frame:
//
//   .meter-label      44 px column, no padding.  ENTRÉE 42.4, SORTIE 39.0.
//                     ENTRÉE clears by 1.6 px, which is why .meter-label also
//                     gained `white-space: nowrap` — a wrap there is not a
//                     clip, it is an extra line that shoves the meter bars
//                     down and moves the whole column.
//   .knob-label       104 px cell in .knob-row. TRIM -> AJUST. 33.6, well in.
//                     "Réglage fin" (66.1) also fits, but AJUST. matches the
//                     abbreviated register of GAIN / TRIM beside it.
//   .learn-btn        pinned to 121 px (see index.html). The four faces below
//                     measure 70.0 / 74.0 / 66.6 / 69.6 px of content against
//                     an 89 px content box.
//   .utility-btn      the seven-button row is 344.8 px of min-content in a
//                     334 px row IN ENGLISH ALREADY — PH L, PH R and M/S OFF
//                     each render on TWO lines at v1.2.1 and still do. Every
//                     French caption below was chosen so its MIN-CONTENT (the
//                     widest single word) is within a pixel of the English
//                     one, so the row keeps exactly two lines and its 32 px
//                     height does not change. PERM 54.4 vs SWAP 54.4;
//                     M/S ARR wraps to M/S + ARR exactly as M/S OFF does.
//
// ALL FRENCH IS MACHINE-DRAFTED, `reviewed: false`. No native speaker has read
// it. `node scripts/check-i18n.js` prints the worklist, LABELS included.
// ============================================================================

export const LABELS = Object.freeze({

    // ── Meter columns ───────────────────────────────────────────────────────
    'label.input':  { en: { t: 'Input' },  fr: { t: 'Entrée', reviewed: false } },
    'label.output': { en: { t: 'Output' }, fr: { t: 'Sortie', reviewed: false } },

    // ── The big gain readout's caption. The readout itself and its "dB" unit
    //    are NOT keyed — contract §5, D-03.
    'label.gainOffset': { en: { t: 'Gain Offset' }, fr: { t: 'Décalage de gain', reviewed: false } },

    // ── Knob captions ───────────────────────────────────────────────────────
    // GAIN is spelled identically in French and is KEYED rather than exempted:
    // it is a caption this page owns, not a parameter option string, and an
    // exemption would say "this cannot be translated" when the truth is "this
    // translates to itself". sameAsEn: true is what says the second thing out
    // loud — assertion 4 rejects a silent fr === en passthrough, because that
    // is indistinguishable from a translation somebody forgot to write.
    'label.gain':        { en: { t: 'Gain' },         fr: { t: 'Gain', reviewed: false, sameAsEn: true } },
    'label.trim':        { en: { t: 'Trim' },         fr: { t: 'Ajust.',       reviewed: false } },
    'label.targetLevel': { en: { t: 'Target Level' }, fr: { t: 'Niveau cible', reviewed: false } },
    'label.target':      { en: { t: 'Target' },       fr: { t: 'Cible',        reviewed: false } },

    // ── Mode-selector captions. The OPTION captions beside them (LUFS, RMS,
    //    Peak, VU) are the AudioParameterChoice option strings verbatim and are
    //    I18N_EXEMPT under D-01 — see the note there.
    'label.measure': { en: { t: 'Measure' }, fr: { t: 'Mesure',  reviewed: false } },
    'label.meter':   { en: { t: 'Meter' },   fr: { t: 'Vumètre', reviewed: false } },

    // ── The Learn panel ─────────────────────────────────────────────────────
    'label.learnAnalysis': { en: { t: 'Learn Analysis' }, fr: { t: 'Analyse de mesure', reviewed: false } },
    'label.momentary':     { en: { t: 'Momentary' },      fr: { t: 'Momentané',   reviewed: false } },
    'label.shortTerm':     { en: { t: 'Short-term' },     fr: { t: 'Court terme',  reviewed: false } },
    'label.integrated':    { en: { t: 'Integrated' },     fr: { t: 'Intégré',      reviewed: false } },
    'label.samplePeak':    { en: { t: 'Sample Peak' },    fr: { t: 'Crête éch.',   reviewed: false } },
    'label.elapsed':       { en: { t: 'Elapsed' },        fr: { t: 'Écoulé',       reviewed: false } },
    'label.confidence':    { en: { t: 'Confidence' },     fr: { t: 'Confiance',    reviewed: false } },

    // ── The utility row. See the min-content note in the block comment above:
    //    every French caption's widest WORD matches its English counterpart's,
    //    so the row's two-line wrap and its 32 px height are unchanged.
    //
    //    PH is an abbreviation of "phase", which is the same word in French, so
    //    only the channel letter moves: L/R -> G/D (gauche / droite).
    'label.phaseL': { en: { t: 'PH L' },    fr: { t: 'PH G',    reviewed: false } },
    'label.phaseR': { en: { t: 'PH R' },    fr: { t: 'PH D',    reviewed: false } },
    'label.swap':   { en: { t: 'SWAP' },    fr: { t: 'PERM',    reviewed: false } },
    // MONO and ENC are spelled identically in French — "mono" is the same word
    // and "ENC" the same abbreviation of encoder/encode. Declared, not silent.
    'label.mono':   { en: { t: 'MONO' },    fr: { t: 'MONO', reviewed: false, sameAsEn: true } },
    // The three M/S captions are NOT the ms_mode option strings — those are
    // "Off", "Encode", "Decode" (PluginProcessor.cpp:238-243) and none of these
    // three matches one byte for byte. They are plain captions, so they
    // localize. ARR is "arrêt", the same abbreviation the hover-help switch
    // uses for Off.
    'label.msOff':  { en: { t: 'M/S OFF' }, fr: { t: 'M/S ARR', reviewed: false } },
    'label.msEnc':  { en: { t: 'ENC' },     fr: { t: 'ENC',  reviewed: false, sameAsEn: true } },
    'label.msDec':  { en: { t: 'DEC' },     fr: { t: 'DÉC',     reviewed: false } },

    // ── The Learn button's four faces, written from script by setLabel() ────
    // ui.learn is also the caption authored in index.html, so the button is a
    // [data-i18n] element from first paint rather than from the first meter
    // tick. The other three replace v1.2.1's raw textContent literals.
    'ui.learn':    { en: { t: 'LEARN' },       fr: { t: 'MESURER',   reviewed: false } },
    'ui.learning': { en: { t: 'LEARNING...' }, fr: { t: 'MESURE...', reviewed: false } },
    'ui.done':     { en: { t: 'DONE' },        fr: { t: 'TERMINÉ',   reviewed: false } },
    'ui.tooQuiet': { en: { t: 'TOO QUIET' },   fr: { t: 'TROP BAS',  reviewed: false } },

    // ── The confidence verdict. See the I18N_EXEMPT note for why these three
    //    are COPY and not a readout, which is the opposite call from the six
    //    timbre words on O-Marimba.
    'ui.confLow':  { en: { t: 'LOW' },  fr: { t: 'BAS',  reviewed: false } },
    'ui.confMed':  { en: { t: 'MED' },  fr: { t: 'MOY',  reviewed: false } },
    'ui.confHigh': { en: { t: 'HIGH' }, fr: { t: 'HAUT', reviewed: false } },

    // ── The settings popover ────────────────────────────────────────────────
    'label.language':  { en: { t: 'Language' },   fr: { t: 'Langue', reviewed: false } },
    'label.hoverHelp': { en: { t: 'Hover help' }, fr: { t: 'Aide',   reviewed: false } },
    'ui.on':           { en: { t: 'On' },         fr: { t: 'Marche', reviewed: false } },
    'ui.off':          { en: { t: 'Off' },        fr: { t: 'Arrêt',  reviewed: false } },

    // ── Accessible names. Keyed through data-i18n-aria, which resolves through
    //    the same sweep with setAttribute. aria.helpToggle replaces the ONE
    //    native title= this page carried (#help-btn "Toggle tooltips"), which
    //    contract §4 deletes: a native title renders a second, untranslated OS
    //    tooltip competing with the measure-then-pin renderer.
    'aria.settings':   { en: { t: 'Settings' },             fr: { t: 'Réglages',                 reviewed: false } },
    'aria.langSelect': { en: { t: 'Interface language' },    fr: { t: 'Langue de l’interface',    reviewed: false } },
    'aria.helpToggle': { en: { t: 'Toggle hover help' },     fr: { t: 'Activer l’aide au survol', reviewed: false } },
});

// ============================================================================
// I18N_EXEMPT — reasoned exclusions, never silence
// ============================================================================
//
// Every visible string the coverage scan finds must be a [data-i18n] element, a
// setLabel() call, or an entry HERE WITH A REASON. A bare skip list would let a
// missed label hide as a deliberate one.
//
// ── O-Gain CARRIES BOTH ARMS OF THE CHOICE-OPTION TEST, WHICH IS RARE ──────
//
// The discriminator settled over Stages I and J is BYTE-IDENTITY against the
// AudioParameterChoice option strings: a caption that matches an option
// verbatim is exempt under D-01, because translating it alone would make the
// page and the host's automation lane disagree about the same setting. A
// caption that does NOT match is a plain caption and it localizes.
//
// Most plugins exercise only one arm. This page exercises both, on two
// controls that look identical:
//
//     meter_mode        StringArray { "Peak", "RMS", "VU", "LUFS" }
//     page captions       Peak   RMS   VU   LUFS        -> IDENTICAL, exempt
//
//     measurement_mode  StringArray { "LUFS", "RMS" }
//     page captions       LUFS   RMS                     -> IDENTICAL, exempt
//
//     ms_mode           StringArray { "Off", "Encode", "Decode" }
//     page captions       M/S OFF   ENC   DEC             -> NO MATCH, localize
//
// The M/S buttons are abbreviations the PAGE invented; they never reached the
// host and no automation lane has ever shown them. They are in LABELS above.
//
// ── WHY LOW / MED / HIGH IS COPY, AND Edge / Center WAS NOT ────────────────
//
// The readout third arm — "a string written into a READOUT node is never
// [data-i18n]" — retired six timbre words on O-Marimba. It does not retire
// these three, and the difference is worth stating rather than assuming:
//
//   1. NO ALTERNATION. O-Marimba's #tone-value shows "62%" at most values and
//      "Warm" at the ends of one knob's travel, so keying it makes the element
//      ENTER and LEAVE the [data-i18n] sweep as the knob turns and a later
//      language change repaints a French word over a percentage.
//      #learn-confidence shows a WORD at every value it ever has: "--" at 0,
//      LOW/MED/HIGH at 1/2/3. It never holds a number, so there is nothing for
//      a French repaint to land on top of. ("--" carries no key at all — the
//      setter deletes data-i18n for that branch, and "--" is
//      language-neutral.)
//   2. NO PARAMETER. The six timbre words mirrored STRIKE_POSITION,
//      OVERTONE_DAMPING and TONE. learnConfidence is not a parameter — it is
//      not in createParameterLayout() at all, it is a field of the Learn
//      seqlock snapshot (PluginEditor.cpp timerCallback). D-01 has nothing to
//      say about it. This is the same call Stage I made for O-simpleBeatmaker's
//      "synced" and the opposite of the one it made for
//      O-simplePhysicalModelSynth's parameter mirror.
//   3. D-04 IS DISCHARGEABLE. The six French faces on O-Marimba existed only at
//      the extremes of three knobs and no committed state could turn a knob.
//      These three are reachable from tests/i18n-states.json by driving
//      window.updateMeters, so all three French faces ARE measured at 350 x 500
//      rather than shipped unmeasured.
//
// And the reason that matters to a user: the tooltip on this very cell reads
// "Low = under 5s. Medium = 5-15s. High = over 15s". A French tooltip over an
// English verdict is exactly the half-localized state this work exists to end.
// ============================================================================

export const I18N_EXEMPT = [
    ['Ouaricon Gain',
     'the product display name in .title — a product name is never translated, and this is the brand-plus-product form of the plugin\'s registered PRODUCT_NAME "O-Gain" in CMakeLists.txt'],

    // ── The two choice controls whose captions ARE the option strings (D-01) ─
    ['Peak',
     'the meter_mode AudioParameterChoice option string VERBATIM (PluginProcessor.cpp:202-207, StringArray {"Peak","RMS","VU","LUFS"}). Translating the caption alone would make the page and the host automation lane disagree about the same setting (D-01)'],
    ['RMS',
     'a meter_mode AND measurement_mode option string VERBATIM (PluginProcessor.cpp:194-207) — D-01, as above. Also an acronym, spelled identically in French'],
    ['VU',
     'the meter_mode option string VERBATIM — D-01, as above. Also the name of the VU standard'],
    ['LUFS',
     'a meter_mode AND measurement_mode option string VERBATIM (PluginProcessor.cpp:194-207) — D-01, as above. Also the ITU-R BS.1770 unit symbol, language-neutral (D-03)'],

    // ── Units and readouts (D-03, contract §5) ──────────────────────────────
    ['dB',
     'unit symbol in .gain-display-unit, language-neutral (D-03)'],
    ['-inf',
     'the #input-db-label / #output-db-label readout floor. The same node shows "-12" whenever the level is above -99 dB, so it is a VALUE MIRROR alternating with a number — contract §5: a readout is never a [data-i18n] element (D-03)'],
    ['-- LUFS',
     'the placeholder face of the four Learn readouts, which otherwise show "-23.4 LUFS". A readout under D-03, and its unit is language-neutral'],
    ['-- dBFS',
     'the placeholder face of the #lufs-true-peak readout, which otherwise shows "-1.2 dBFS" — a readout under D-03'],

    // ── Endonyms ────────────────────────────────────────────────────────────
    ['English',  'endonym — a language name is never translated'],
    ['Français', 'endonym — a language name is never translated'],
];

// [selector, key] or [selector, key, wrapperSelector].
//
// The selector is the BINDING SITE and the wrapper climbs to the box the tip
// should hang off. applyI18n() uses document.querySelector, which returns the
// FIRST match in document order, so every row below names an element that
// carries a UNIQUE id — never a bare class that repeats. v1.2.1 hung its tips
// off .meter-column, .knob-group, .mode-group and .learn-info-item, four
// classes that repeat two, three, two and six times; binding on the class alone
// would have put every tip on the first match, which is how O-Octagon's
// .vunit-group tip nearly landed on the wrong control in Stage C. Eleven ids
// were added to index.html for exactly this reason and for nothing else.
export const TIP_BINDINGS = [
    // The three new controls.
    ['#gear-btn',    'settings'],
    ['#lang-select', 'lang-select'],
    ['#tips-toggle', 'tips-toggle'],

    // Meter columns and the centre stack.
    ['#input-meter-group',  'input-meter'],
    ['#output-meter-group', 'output-meter'],
    ['#gain-display',       'gain-display'],
    ['#gain-knob-group',    'gain-knob'],
    ['#trim-knob-group',    'trim-knob'],
    ['#learn-btn',          'learn-btn'],
    ['#target-group',       'target-group'],
    ['#target-knob-group',  'target-knob'],

    // Mode selectors.
    ['#measure-mode-group', 'measure-mode'],
    ['#meter-mode-group',   'meter-mode'],

    // The Learn panel's six cells.
    ['#info-momentary',   'info-momentary'],
    ['#info-short-term',  'info-short-term'],
    ['#info-integrated',  'info-integrated'],
    ['#info-sample-peak', 'info-sample-peak'],
    ['#info-elapsed',     'info-elapsed'],
    ['#info-confidence',  'info-confidence'],

    // The utility row.
    ['#phase-l-btn', 'phase-l'],
    ['#phase-r-btn', 'phase-r'],
    ['#swap-btn',    'swap'],
    ['#mono-btn',    'mono'],
    ['#ms-off-btn',  'ms-off'],
    ['#ms-enc-btn',  'ms-enc'],
    ['#ms-dec-btn',  'ms-dec'],
];

// The tooltip lookup. Returns {t, b} — never null, never a bare key without a
// console.warn saying so, because a silently-missing tip renders as an empty
// surface that looks like a positioning bug rather than a missing entry.
export function tr(key, lang, vars) {
    const entry = I18N[key];
    if (!entry) { console.warn(`i18n: missing key ${key}`); return { t: key, b: '' }; }
    const s = entry[lang] || entry.en;

    // A var VALUE that is itself an I18N key resolves to that key's localized
    // title; anything else is used literally. TIP_BINDINGS is evaluated once at
    // module load, so a localized string stored there would be frozen at the
    // load-time language.
    const resolve = (v) => {
        const nested = I18N[v];
        return nested ? String((nested[lang] || nested.en).t) : String(v);
    };

    const sub = (v) => vars
        ? String(v).replace(/\{(\w+)\}/g, (m, n) => (n in vars ? resolve(vars[n]) : m))
        : String(v);

    return { t: sub(s.t), b: sub(s.b) };
}
