/*
   This file is part of O-Marimba, an Ouaricon Audio plugin.
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
// i18n.js — O-Marimba UI copy, English + French (v1.13.0, canon v2)
//
// An ES module that EXPORTS ONLY. It must never self-execute: a bare top-level
// statement here throws out of module evaluation and takes every later
// initializer on the page with it (pattern_module_toplevel_init_tdz).
// scripts/check-i18n.js assertion 7 enforces that.
//
// SERVED ROOT IS Source/ui/public, read from CMakeLists.txt before a byte was
// written here. THE BINARY-DATA TARGET CARRIES NO NAMESPACE ARGUMENT —
// juce_add_binary_data(OMarimba_UIResources SOURCES ...) takes the default
// BinaryData namespace and works only because it is the only such target in
// this plugin. This file and js/app.js were added to that EXISTING SOURCES
// list; a second juce_add_binary_data target would collide on the BinaryData
// namespace and break the build in a way that reads like something else
// entirely (critical_dual_binary_data_namespace_collision).
//
// FOUR PLACES, ONE COMMIT — TWICE OVER, because v1.13.0 also EXTRACTED the
// 1,248-line inline <script type="module"> into js/app.js. Each of the two new
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
// The plan's table says "15 tips, ~40 static text nodes". Rendered headless
// through scripts/serve-ui.js and walked with a TreeWalker rather than grepped:
//
//     data-tooltip LIVE ANCHORS           15   (unique strings: 15)
//     index.html text nodes               39
//     text nodes injected by the page      2   (interval header, "Tonic:")
//     text nodes from the two SHARED FX modules  20
//     ──────────────────────────────────────────
//     RENDERED text nodes                 61
//
//     native title=      8   (5 authored in index.html, 3 injected by
//                             updateIntervalListUI's tonic selector)
//     aria-label         0
//     alt                1
//     placeholder       12   (all numeric — READOUT, exempt under D-03)
//
// The tip count is the FIRST plan figure in this task that survived being
// parsed: 15 anchors, 15 unique strings, no makeKnob-applied table and no
// injected tip template. The TEXT count did not — 61 rendered against a plan
// figure of ~40, and the 20-node difference is the Effects tab, which is built
// by two SHARED REGISTRY MODULES this commit deliberately does not edit. See
// I18N_EXEMPT.
//
// ── THE SPLIT: 15 CLEAN, 0 HAND-SPLIT ──────────────────────────────────────
// The plan expects copy authored as "Label: sentence." and warns the shape
// usually does not hold. Here it holds on ALL FIFTEEN. Every string contains
// EXACTLY ONE ": " and in every case it is the title separator, so the split is
// mechanical and the bodies below are byte-identical to v1.12.1's attribute
// values. Verified by re-reading the rendered attributes and comparing, not by
// eye.
//
// ── FRENCH IS SIZED, NOT SHRUNK ────────────────────────────────────────────
// 600 x 400 is the third-narrowest frame in this stage and its knob captions
// live in 55 px columns on an 80 px pitch. D-04 forbids an auto-shrink font and
// a short-variant fallback: exactly ONE French string per key, nothing chooses
// between variants at runtime, and every caption below was MEASURED AS RENDERED
// inside its own element — text-transform: uppercase and letter-spacing: 1px
// are not in getComputedStyle().font, so a font probe reads short and a pin
// lands under the French.
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

    // ── The settings popover (v1.13.0) ──────────────────────────────────────
    // New controls, new copy. The gear takes the exact absolute slot the
    // floating "?" occupied through v1.12.1 (bottom: 50px; right: 15px), so
    // nothing on a packed 600 x 400 layout had to move to make room for it, and
    // the hover-help switch moves inside beside the language selector.
    'settings': {
        en: { t: 'Settings',
              b: 'Choose the language of this interface and whether hover help appears. Both choices are remembered with the session.' },
        fr: { t: 'Réglages',
              b: 'Choisir la langue de cette interface et l’affichage de l’aide au survol. Les deux choix sont conservés avec la session.',
              reviewed: false },
    },
    'lang-select': {
        en: { t: 'Language',
              b: 'The language of this hover help and of the labels on the page. English and French are available; value readouts, note names, tuning names and preset names stay in English.' },
        fr: { t: 'Langue',
              b: 'La langue de cette aide au survol et des libellés de la page. L’anglais et le français sont disponibles ; les valeurs affichées, les noms de notes, les noms de gammes et les noms de préréglages restent en anglais.',
              reviewed: false },
    },
    'tips-toggle': {
        en: { t: 'Hover Help',
              b: 'Turns this hover help on and off. With it off, only the gear and this switch keep explaining themselves.' },
        fr: { t: 'Aide au survol',
              b: 'Active ou désactive cette aide au survol. Une fois désactivée, seuls l’engrenage et ce commutateur continuent de s’expliquer.',
              reviewed: false },
    },

    // ── SOUND tab. All fifteen below SPLIT CLEANLY on the single ": ". ──────
    'mallet': {
        en: { t: 'Mallet Hardness',
              b: 'Controls the hardness of the virtual mallet. Soft mallets (0%) produce warm, mellow tones; hard mallets (100%) create bright, articulate attacks.' },
        fr: { t: 'Dureté du maillet',
              b: 'Règle la dureté du maillet virtuel. Les maillets doux (0 %) donnent un son chaud et moelleux ; les maillets durs (100 %) créent des attaques claires et articulées.',
              reviewed: false },
    },
    'material': {
        en: { t: 'Material Hardness',
              b: 'Simulates the bar material density. Lower values emulate softer rosewood; higher values emulate denser synthetic materials with more sustain.' },
        fr: { t: 'Dureté de la matière',
              b: 'Simule la densité de la matière des lames. Les valeurs basses imitent un palissandre plus tendre ; les valeurs hautes imitent des matières synthétiques plus denses, avec plus de tenue.',
              reviewed: false },
    },
    'resonance': {
        en: { t: 'Resonance',
              b: 'Controls the resonator tube coupling. Higher values increase sustain and add the characteristic \'bloom\' of marimba resonators.' },
        fr: { t: 'Résonance',
              b: 'Règle le couplage des tubes résonateurs. Les valeurs hautes allongent la tenue et ajoutent l’épanouissement caractéristique des résonateurs de marimba.',
              reviewed: false },
    },
    'strike': {
        en: { t: 'Strike Position',
              b: 'Where the mallet strikes the bar. Center (0%) emphasizes the fundamental; edge (100%) brings out higher partials and overtones.' },
        fr: { t: 'Point de frappe',
              b: 'Endroit où le maillet frappe la lame. Le centre (0 %) met en avant le fondamental ; le bord (100 %) fait ressortir les partiels aigus et les harmoniques.',
              reviewed: false },
    },
    'damping': {
        en: { t: 'Overtone Damping',
              b: 'Controls how quickly upper harmonics decay. Lower values preserve bright overtones; higher values create a purer, more fundamental tone.' },
        fr: { t: 'Amortissement des harmoniques',
              b: 'Règle la vitesse d’extinction des harmoniques aiguës. Les valeurs basses conservent des harmoniques brillantes ; les valeurs hautes donnent un son plus pur, centré sur le fondamental.',
              reviewed: false },
    },
    'tone': {
        en: { t: 'Tone',
              b: 'Overall brightness control. Acts as a gentle low-pass filter. Lower values produce darker tones; higher values retain more high-frequency content.' },
        fr: { t: 'Timbre',
              b: 'Réglage général de brillance. Agit comme un filtre passe-bas doux. Les valeurs basses donnent un son plus sombre ; les valeurs hautes conservent plus d’aigus.',
              reviewed: false },
    },
    'waveform': {
        en: { t: 'Waveform',
              b: 'Real-time visualization of the marimba\'s sound wave. Shows how the timbre controls affect the harmonic content of each note.' },
        fr: { t: 'Forme d’onde',
              b: 'Visualisation en temps réel de l’onde sonore du marimba. Montre comment les réglages de timbre agissent sur le contenu harmonique de chaque note.',
              reviewed: false },
    },
    'velocity': {
        en: { t: 'Velocity Curve',
              b: 'Shapes how MIDI velocity affects volume. Low values create a compressed, even response; high values create dynamic, expressive playing with more contrast.' },
        fr: { t: 'Courbe de vélocité',
              b: 'Détermine l’effet de la vélocité MIDI sur le volume. Les valeurs basses donnent une réponse compressée et régulière ; les valeurs hautes donnent un jeu dynamique et expressif, plus contrasté.',
              reviewed: false },
    },
    'response': {
        en: { t: 'Response Graph',
              b: 'Visual representation of the velocity curve. X-axis is input velocity; Y-axis is output volume. Steeper curves = more dynamic range.' },
        fr: { t: 'Graphe de réponse',
              b: 'Représentation visuelle de la courbe de vélocité. L’axe X est la vélocité d’entrée ; l’axe Y le volume de sortie. Plus la courbe est raide, plus la dynamique est large.',
              reviewed: false },
    },
    'output': {
        en: { t: 'Output Level',
              b: 'Master volume control in decibels (dB). 0 dB is unity gain. Use to match levels with other instruments in your mix.' },
        fr: { t: 'Niveau de sortie',
              b: 'Réglage de volume général en décibels (dB). 0 dB correspond au gain unitaire. Sert à ajuster le niveau par rapport aux autres instruments du mixage.',
              reviewed: false },
    },
    'level-meter': {
        en: { t: 'Level Meter',
              b: 'Shows the current output level. Keep peaks below 0 dB to avoid clipping. The needle responds to audio dynamics in real-time.' },
        fr: { t: 'Vumètre',
              b: 'Affiche le niveau de sortie courant. Gardez les crêtes sous 0 dB pour éviter l’écrêtage. L’aiguille suit la dynamique audio en temps réel.',
              reviewed: false },
    },

    // ── TUNING tab ──────────────────────────────────────────────────────────
    'interval-list': {
        en: { t: 'Interval List',
              b: 'Shows the cents offset for each scale degree. 100 cents = 1 semitone. In CUSTOM mode, these can be edited for microtonal scales.' },
        fr: { t: 'Liste des intervalles',
              b: 'Affiche l’écart en cents de chaque degré de la gamme. 100 cents = 1 demi-ton. En mode CUSTOM, ces valeurs sont modifiables pour des gammes microtonales.',
              reviewed: false },
    },
    'pitch-circle': {
        en: { t: 'Scale Circle',
              b: 'Visual representation of the scale intervals around the octave. Lines show where each note falls relative to equal temperament.' },
        fr: { t: 'Cercle de gamme',
              b: 'Représentation visuelle des intervalles de la gamme sur l’octave. Les traits montrent la position de chaque note par rapport au tempérament égal.',
              reviewed: false },
    },
    'tuning-mode': {
        en: { t: 'Tuning Mode',
              b: '12-TET uses standard equal temperament; CUSTOM enables Scala file loading for microtonal scales; MTS-ESP connects to external tuning masters.' },
        fr: { t: 'Mode d’accord',
              b: '12-TET utilise le tempérament égal standard ; CUSTOM active le chargement de fichiers Scala pour les gammes microtonales ; MTS-ESP se connecte à une source d’accord externe.',
              reviewed: false },
    },
    'a4-ref': {
        en: { t: 'A4 Reference',
              b: 'The concert pitch for A4. Standard is 440 Hz. Historical tunings may use 415 Hz (Baroque) or 432 Hz. Adjustable from 400-480 Hz.' },
        fr: { t: 'Référence A4',
              b: 'Le diapason de A4. La norme est 440 Hz. Les accords historiques utilisent parfois 415 Hz (baroque) ou 432 Hz. Réglable de 400 à 480 Hz.',
              reviewed: false },
    },
});

// ============================================================================
// LABELS — the on-page text (v1.13.0, canon v2)
// ============================================================================
//
// I18N above is HOVER-HELP copy: a title and a body rendered into a wrapping
// 200 px tooltip. LABELS is ON-PAGE copy: one string dropped into a cell that
// mostly does not wrap. They are different problems and this table keeps them
// apart on purpose.
//
// trLabel() falls back to I18N when a key is absent here, so a control whose
// tooltip TITLE already IS its caption could carry ONE key. That is deliberately
// NOT done: the two tables are kept disjoint so a later tooltip rewrite cannot
// silently move a knob caption.
//
// ── ENGLISH WAS MOVED, NOT RE-TYPED ────────────────────────────────────────
// Every `en` below is what index.html carried through v1.12.1, taken from
// scripts/i18n-extract.js's inventory rather than transcribed — with ONE
// exception, a contract §6 rewrite, named here:
//
//     'Intervals (12 notes)'  ->  'Intervals: 12'
//
// It was composed with a count and an inflected noun, and `total` is
// currentIntervals.length, which a degenerate one-line .scl file makes 1.
// French pluralises one AND zero as singular where English pluralises only one,
// so the two languages disagree at n = 0. Contract §6 says copy that needs a
// count is authored AROUND the inflection rather than through a plural engine,
// so the noun was dropped and the number moved behind a colon, where it is
// invariant in both languages. The English loses the word "notes" from the
// interval header; that is the visible cost and it is recorded rather than
// hidden. Same rewrite, same reason, as O-IntonationPad v2.9.0.
//
// ── THE FOUR TWO-LINE KNOB CAPTIONS ────────────────────────────────────────
// MALLET/HARDNESS, MATERIAL/HARDNESS, STRIKE/POSITION and OVERTONE/DAMPING are
// authored as `TEXT<br>TEXT` inside one .knob-label. The <br> is an ELEMENT
// child, so keying the .knob-label and letting applyLabel write textContent
// would DELETE it and collapse the caption to one line. Each line therefore
// carries its own key on its own <span> — keyed FRAGMENTS, never a keyed
// wrapper, which is also what keeps assertion 7 measuring the caption's real
// box rather than a wrapper whose box IS the French sentence.
//
// READ THE PAIRS TOGETHER, NOT ROW BY ROW. French inverts head and modifier, so
// line 1 and line 2 do not translate each other:
//
//     MALLET   / HARDNESS  ->  DURETÉ  / MAILLET     ("hardness of-the mallet")
//     MATERIAL / HARDNESS  ->  DURETÉ  / MATIÈRE
//     STRIKE   / POSITION  ->  POINT   / DE FRAPPE
//     OVERTONE / DAMPING   ->  AMORT.  / HARMON.
//
// A reviewer reading `label.malletL1: en MALLET / fr DURETÉ` in isolation will
// read it as a mistranslation. It is not. The caption is the two lines.
//
// AMORT. / HARMON. are abbreviations rather than "AMORTISSEMENT" and
// "HARMONIQUES": the full words render 118.9 px and 106.0 px at 10 px uppercase
// with 1 px letter-spacing, in an 80 px knob column. That is a §6-style
// authored-around, not an auto-shrink — one string, chosen once, measured.
//
// ALL FRENCH IS MACHINE-DRAFTED, `reviewed: false`. No native speaker has read
// it. `node scripts/check-i18n.js` prints the worklist, LABELS included.
// ============================================================================

export const LABELS = Object.freeze({

    // ── Header: preset bar ──────────────────────────────────────────────────
    // The preset NAME is not keyed: the name IS the JSON filename
    // (OuariconPresetManager.h:283-285), so translating it breaks recall.
    'label.load':  { en: { t: 'LOAD' }, fr: { t: 'CHARG.', reviewed: false } },
    'label.save':  { en: { t: 'SAVE' }, fr: { t: 'ENREG.', reviewed: false } },

    // ── Tab row ─────────────────────────────────────────────────────────────
    'label.tabSound':   { en: { t: 'SOUND' },   fr: { t: 'SON',    reviewed: false } },
    'label.tabTuning':  { en: { t: 'TUNING' },  fr: { t: 'GAMME',  reviewed: false } },
    'label.tabEffects': { en: { t: 'EFFECTS' }, fr: { t: 'EFFETS', reviewed: false } },

    // ── SOUND tab: the six knob captions. The four two-line ones are FRAGMENT
    //    pairs — see the block comment above before reading a row alone.
    'label.malletL1':   { en: { t: 'MALLET' },   fr: { t: 'DURETÉ',    reviewed: false } },
    'label.malletL2':   { en: { t: 'HARDNESS' }, fr: { t: 'MAILLET',   reviewed: false } },
    'label.materialL1': { en: { t: 'MATERIAL' }, fr: { t: 'DURETÉ',    reviewed: false } },
    'label.materialL2': { en: { t: 'HARDNESS' }, fr: { t: 'MATIÈRE',   reviewed: false } },
    'label.resonance':  { en: { t: 'RESONANCE' }, fr: { t: 'RÉSONANCE', reviewed: false } },
    'label.strikeL1':   { en: { t: 'STRIKE' },   fr: { t: 'POINT',     reviewed: false } },
    // "DE FRAPPE" measured 61.52 px against the 55 px knob column and widened
    // it, dragging the knob 3.3 px sideways. French routinely drops the
    // preposition in a stacked caption, so the line is "FRAPPE" (44 px) and the
    // caption reads POINT / FRAPPE.
    'label.strikeL2':   { en: { t: 'POSITION' }, fr: { t: 'FRAPPE',    reviewed: false } },
    'label.dampingL1':  { en: { t: 'OVERTONE' }, fr: { t: 'AMORT.',    reviewed: false } },
    'label.dampingL2':  { en: { t: 'DAMPING' },  fr: { t: 'HARMON.',   reviewed: false } },
    'label.tone':       { en: { t: 'TONE' },     fr: { t: 'TIMBRE',    reviewed: false } },

    // ── SOUND tab: velocity section ─────────────────────────────────────────
    'label.velocity': { en: { t: 'VELOCITY' }, fr: { t: 'VÉLOCITÉ', reviewed: false } },
    'label.response': { en: { t: 'RESPONSE' }, fr: { t: 'RÉPONSE',  reviewed: false } },
    // The two axis captions are 6 px absolute-positioned marks INSIDE a 105 px
    // curve box, 2 px from its edges. "Sortie"/"Entrée" render 17.7 px and
    // 18.5 px against "Out" 10.1 px and "In" 6.4 px, which pushes the x mark
    // over the curve. Abbreviated to the same three-glyph budget the English
    // uses; measured, not guessed.
    'label.axisOut':  { en: { t: 'Out' },      fr: { t: 'Sort',     reviewed: false } },
    'label.axisIn':   { en: { t: 'In' },       fr: { t: 'Entr',     reviewed: false } },

    // ── SOUND tab: output section ───────────────────────────────────────────
    'label.output': { en: { t: 'OUTPUT' }, fr: { t: 'SORTIE', reviewed: false } },
    'label.level':  { en: { t: 'Level' },  fr: { t: 'Niveau', reviewed: false } },

    // ── TUNING tab ──────────────────────────────────────────────────────────
    // The static header index.html authors, and the one updateIntervalListUI()
    // injects with the count. Two keys because they are two different strings
    // in the same slot: the static one renders only in the instant before the
    // first updateIntervalListUI() pass.
    'label.intervals': { en: { t: 'Intervals' }, fr: { t: 'Intervalles', reviewed: false } },
    'label.intervalHeader': {
        en: { t: 'Intervals: {n}' },
        fr: { t: 'Intervalles : {n}', reviewed: false },
    },
    'label.tonic': { en: { t: 'Tonic:' }, fr: { t: 'Tonique :', reviewed: false } },
    // The circle's own caption. "Intervalles de la gamme" is 108.6 px under a
    // 150 px circle whose caption cell is 150 px, so the full form fits; it is
    // used rather than abbreviated.
    'label.scaleIntervals': {
        en: { t: 'Scale Intervals' },
        fr: { t: 'Intervalles de gamme', reviewed: false },
    },
    // Only the middle button of the three-way tuning-mode control is keyed.
    // 12-TET and MTS-ESP are the AudioParameterChoice option strings verbatim
    // and are I18N_EXEMPT under D-01; CUSTOM is not (the option is "Scala"),
    // so it is a plain caption and localizes. See the I18N_EXEMPT note.
    'label.custom': { en: { t: 'CUSTOM' }, fr: { t: 'PERSO', reviewed: false } },
    // "A4" is kept: it is the pitch identifier the 440 Hz readout beside it
    // refers to, not a word. French note naming would make it "La3", which
    // would disagree with every tuning reference the user reads elsewhere.
    'label.a4ref': { en: { t: 'A4 REF' }, fr: { t: 'RÉF. A4', reviewed: false } },

    // The four Scala file buttons are .btn-small at 7 px. "CHARG." and "ENREG."
    // are the same abbreviations the preset bar uses, so the page abbreviates
    // one way rather than two.
    'label.loadScl': { en: { t: 'LOAD .SCL' }, fr: { t: 'CHARG. .SCL', reviewed: false } },
    'label.loadKbm': { en: { t: 'LOAD .KBM' }, fr: { t: 'CHARG. .KBM', reviewed: false } },
    'label.saveScl': { en: { t: 'SAVE .SCL' }, fr: { t: 'ENREG. .SCL', reviewed: false } },
    'label.saveKbm': { en: { t: 'SAVE .KBM' }, fr: { t: 'ENREG. .KBM', reviewed: false } },

    // The MTS-ESP status line is one .mts-label holding a prefix and a status
    // span. The prefix is keyed on its own fragment so applyLabel's textContent
    // write cannot delete the status span beside it. The protocol name is not
    // translated; the French colon takes its narrow no-break space.
    'label.mtsPrefix':   { en: { t: 'MTS-ESP:' },     fr: { t: 'MTS-ESP :',   reviewed: false } },
    'label.disconnected': { en: { t: 'Disconnected' }, fr: { t: 'Déconnecté', reviewed: false } },

    'label.clickToPlay': { en: { t: 'Click to play' }, fr: { t: 'Cliquer pour jouer', reviewed: false } },

    // ── The preset dropdown's two group headings ────────────────────────────
    // Chrome, not data: these are section captions the page invents, and unlike
    // the preset NAMES beneath them they never reach C++ and never become a
    // filename. Same call O-Lyrica made for Factory / Custom.
    'label.presetFactory': { en: { t: 'Factory' }, fr: { t: 'Usine',       reviewed: false } },
    'label.presetUser':    { en: { t: 'User' },    fr: { t: 'Utilisateur', reviewed: false } },

    // ── The settings popover ────────────────────────────────────────────────
    'label.language':  { en: { t: 'Language' },   fr: { t: 'Langue',        reviewed: false } },
    'label.hoverHelp': { en: { t: 'Hover help' }, fr: { t: 'Aide',          reviewed: false } },
    'ui.on':           { en: { t: 'On' },         fr: { t: 'Marche',        reviewed: false } },
    'ui.off':          { en: { t: 'Off' },        fr: { t: 'Arrêt',         reviewed: false } },

    // ── Accessible names. Keyed through data-i18n-aria, which resolves through
    //    the same sweep with setAttribute. Every one of these replaces a native
    //    title= that contract §4 deletes: a native title renders a second,
    //    untranslated OS tooltip competing with the measure-then-pin renderer.
    'aria.settings':    { en: { t: 'Settings' },              fr: { t: 'Réglages',                  reviewed: false } },
    'aria.langSelect':  { en: { t: 'Interface language' },     fr: { t: 'Langue de l’interface',     reviewed: false } },
    'aria.helpToggle':  { en: { t: 'Toggle hover help' },      fr: { t: 'Activer l’aide au survol',  reviewed: false } },
    'aria.prevPreset':  { en: { t: 'Previous preset' },        fr: { t: 'Préréglage précédent',      reviewed: false } },
    'aria.nextPreset':  { en: { t: 'Next preset' },            fr: { t: 'Préréglage suivant',        reviewed: false } },
    'aria.loadPreset':  { en: { t: 'Load preset from file' },  fr: { t: 'Charger un préréglage depuis un fichier', reviewed: false } },
    'aria.savePreset':  { en: { t: 'Save preset' },            fr: { t: 'Enregistrer le préréglage', reviewed: false } },
    'aria.botanical':   { en: { t: 'Botanical illustration' }, fr: { t: 'Illustration botanique',    reviewed: false } },
    // The three injected by updateIntervalListUI()'s tonic selector.
    'aria.tonicSelector': { en: { t: 'Change tonic note (transposes instrument)' },
                            fr: { t: 'Changer la tonique (transpose l’instrument)', reviewed: false } },
    'aria.tonicPrev':     { en: { t: 'Previous note' }, fr: { t: 'Note précédente', reviewed: false } },
    'aria.tonicNext':     { en: { t: 'Next note' },     fr: { t: 'Note suivante',   reviewed: false } },
});

// ============================================================================
// I18N_EXEMPT — reasoned exclusions, never silence
// ============================================================================
//
// Every visible string the coverage scan finds must be a [data-i18n] element, a
// setLabel() call, or an entry HERE WITH A REASON. A bare skip list would let a
// missed label hide as a deliberate one.
//
// ── THE SEVEN JS-WRITTEN "MODE NAMES", AND WHY ALL SEVEN STAY ENGLISH ──────
//
// The plan says O-Marimba has seven JS-written prose strings, that they are
// mode names, and that "every one needs setLabel". Parsed rather than assumed,
// the seven are Edge, Center, Shimmer, Focused, Warm, Bright and
// "MTS-ESP (stub)" — and NONE of them is a caption. All seven are VALUE
// MIRRORS, and the discriminator that decides it is not the one the plan
// reached for.
//
// The plan's test is "is it an AudioParameterChoice option?". Grepped against
// createParameterLayout(), the answer for the six timbre words is NO:
//
//     STRIKE_POSITION   AudioParameterFloat  0..1   -> Edge / Center
//     OVERTONE_DAMPING  AudioParameterFloat  0..1   -> Shimmer / Focused
//     TONE              AudioParameterFloat  0..1   -> Warm / Bright
//
// On that test alone they would localize. They do not, because the test has a
// third arm the plan's binary does not: the element they are written into.
//
// All six are written to a .knob-sublabel whose id ends in -value — the SAME
// node that shows `Math.round(v * 100) + "%"` everywhere else in its range.
// The word is not a label the knob wears; it is the knob's READOUT, wearing a
// word instead of a number at the ends of its travel. Contract §5 is explicit:
// "A readout is never a [data-i18n] element."
//
// Three consequences, each of which is a reason on its own:
//
//   1. D-03. The readout is the thing D-03 keeps in English. `50%` stays `50%`
//      in French; `Center` is the same readout at a different value.
//   2. Keying it makes the element ENTER and LEAVE the [data-i18n] sweep as the
//      knob turns. setLabel() writes data-i18n; the numeric branch writes
//      textContent directly and leaves the key behind. The next language change
//      then repaints "Chaud" over "62%" — a new bug invented to translate six
//      adjectives.
//   3. D-04 could not be discharged on them. The six French faces exist only at
//      the extremes of three knobs, and no committed check-ui-labels state can
//      turn a knob. On the third-narrowest frame in the stage that is an
//      unmeasurable overflow obligation, which is the opposite of what D-04
//      asks for.
//
// The seventh, "MTS-ESP (stub)", is written to #scale-name — the same node that
// receives getActiveTuningName() from the tuning engine and the scaleName field
// of a loaded preset. It is a data mirror for the same reason "12-TET Standard"
// is, and MTS-ESP is a protocol name besides.
//
// Recorded here rather than argued in a commit message because the next plugin
// with a descriptive float readout will meet the same question.
// ============================================================================

export const I18N_EXEMPT = [
    ['O-Marimba',
     'the product name in .title — a product name is never translated, and this one is the display form of the plugin\'s registered PRODUCT_NAME in CMakeLists.txt'],

    ['Default Marimba',
     'a factory preset name shown in #preset-name-display — exempt under D-02, because the name IS the JSON filename (OuariconPresetManager.h:283-285)'],
    ['Default',
     'the preset-name fallback PresetManager::setStateFromXml writes when a session carries no presetName — a preset name under D-02'],

    // ── The six timbre-knob readout faces (D-01 / D-03, contract §5) ────────
    ['Edge',
     'STRIKE_POSITION readout face — a VALUE MIRROR written into the same .knob-sublabel that shows "50%", not a caption. Contract §5: a readout is never a [data-i18n] element (D-03). The parameter is an AudioParameterFloat, so no automation lane names it either'],
    ['Center',
     'STRIKE_POSITION readout face — same node, same rule as Edge'],
    ['Shimmer',
     'OVERTONE_DAMPING readout face — a VALUE MIRROR in #damping-value, alternating with "50%" as a function of the AudioParameterFloat behind it (D-03, contract §5)'],
    ['Focused',
     'OVERTONE_DAMPING readout face — same node, same rule as Shimmer'],
    ['Warm',
     'TONE readout face — a VALUE MIRROR in #tone-value, alternating with "75%" (D-03, contract §5)'],
    ['Bright',
     'TONE readout face — same node, same rule as Warm'],

    // ── Tuning identifiers and the two choice-option captions (D-01) ────────
    ['12-TET',
     'the TUNING_MODE AudioParameterChoice option string VERBATIM (PluginProcessor.cpp:93-98, StringArray {"12-TET","Scala","MTS-ESP"}). Translating the caption alone would make the page and the host automation lane disagree about the same setting (D-01). It is also a temperament identifier'],
    ['MTS-ESP',
     'the TUNING_MODE AudioParameterChoice option string VERBATIM — D-01, as above. Also the name of the MTS-ESP protocol'],
    ['MTS-ESP (stub)',
     'written to #scale-name, the node that also receives TuningEngine::getActiveTuningName() and a preset\'s scaleName field — a data mirror, not a caption. The protocol name is not translated'],
    ['12-TET Standard',
     'a tuning IDENTIFIER, not a caption — it is the name the tuning engine reports for the loaded scale, is sent back to C++ by setTuningIntervals(), and is persisted in the session and in preset JSON'],
    ['Just Intonation',
     'a tuning IDENTIFIER — same path as 12-TET Standard: written to #scale-name and sent to C++ as the active scale name'],
    ['Pythagorean',
     'a tuning IDENTIFIER — same path as 12-TET Standard'],
    ['Custom',
     'the scale-name value the interval editor assigns when a user edits a cents field. It is SENT TO C++ by applyTuning() and persisted as the session\'s scaleName, so translating it would make a French-saved session recall a scale named "Personnalisé". Distinct from the CUSTOM button caption, which is localized'],

    // ── The two shared Effects-tab modules ─────────────────────────────────
    // The Effects tab is built at runtime by modules/effects/analog-eq-unit and
    // modules/effects/compressor-unit, embedded from ${CMAKE_SOURCE_DIR} rather
    // than from this plugin's UI root. They are registry-tracked shared modules
    // (modules/registry.yaml, analog-eq-unit 1.2.0, compressor-unit 1.2.1).
    // Localizing them is a CROSS-PLUGIN change that does not belong inside a
    // per-plugin commit, and a local edit would be silently reverted the next
    // time /module-upgrade runs. Twenty rendered text nodes, of which these are
    // the ones that are actually English words rather than acronyms or units.
    ['ANALOG',
     'analog-eq-unit shared module caption — a registry-tracked module embedded from ${CMAKE_SOURCE_DIR}/modules/effects/. Localizing it is a cross-plugin change and a local edit is reverted by /module-upgrade'],
    ['Thresh',
     'compressor-unit shared module caption — same reason as ANALOG'],
    ['Attack',
     'compressor-unit shared module caption — same reason as ANALOG'],
    ['Release',
     'compressor-unit shared module caption — same reason as ANALOG'],
    ['Ratio',
     'compressor-unit shared module caption — same reason as ANALOG; also spelled identically in French'],
    ['AUTO',
     'compressor-unit shared module caption — same reason as ANALOG; also spelled identically in French'],

    // ── Endonyms ────────────────────────────────────────────────────────────
    ['English',  'endonym — a language name is never translated'],
    ['Français', 'endonym — a language name is never translated'],
];

// [selector, key] or [selector, key, wrapperSelector].
//
// The selector is the BINDING SITE and the wrapper climbs to the box the tip
// should hang off. applyI18n() uses document.querySelector, which returns the
// FIRST match in document order, so every row below names an element that
// carries a UNIQUE id — never a bare class that repeats. Six .knob-container
// boxes wear a tip and the class alone would have put all six on the first one,
// which is how O-Octagon's .vunit-group tip nearly landed on the wrong control
// in Stage C.
export const TIP_BINDINGS = [
    // The three new controls.
    ['#gear-btn',    'settings'],
    ['#lang-select', 'lang-select'],
    ['#tips-toggle', 'tips-toggle'],

    // SOUND tab — the six timbre knobs, bound on the knob's own id and lifted
    // to the container that holds the knob, its caption and its readout.
    ['#mallet-knob',    'mallet',    '.knob-container'],
    ['#material-knob',  'material',  '.knob-container'],
    ['#resonance-knob', 'resonance', '.knob-container'],
    ['#strike-knob',    'strike',    '.knob-container'],
    ['#damping-knob',   'damping',   '.knob-container'],
    ['#tone-knob',      'tone',      '.knob-container'],

    // SOUND tab — the two displays and the two remaining knobs.
    ['#waveform-path',   'waveform', '.waveform-display'],
    ['#velocity-knob',   'velocity', '.knob-container'],
    ['#velocity-curve',  'response', '.velocity-curve-display'],
    ['#output-knob',     'output',   '.knob-container'],
    ['#vu-needle',       'level-meter', '.vu-meter'],

    // TUNING tab.
    ['#interval-list',    'interval-list'],
    ['#pitch-circle-svg', 'pitch-circle', '.pitch-circle'],
    ['#btn-12tet',        'tuning-mode',  '.tuning-mode-section'],
    ['#ref-pitch-knob',   'a4-ref',       '.knob-container'],
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
