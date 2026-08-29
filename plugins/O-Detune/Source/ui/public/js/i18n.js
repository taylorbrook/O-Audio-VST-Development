/*
   This file is part of O-Detune, an Ouaricon Audio plugin.
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
// i18n.js — O-Detune page labels, English + French (v1.6.0)
//
// An ES module that EXPORTS ONLY. It must never self-execute: a bare top-level
// statement here throws out of module evaluation and takes every later
// initializer on the page with it (pattern_module_toplevel_init_tdz). This
// plugin's controller is a single inline <script type="module"> in index.html,
// so that failure mode would take the WHOLE UI, not one panel of it.
// scripts/check-i18n.js assertion 7 enforces it.
//
// FILENAME: no hyphen. juce_add_binary_data STRIPS hyphens rather than
// converting them to underscores, so a file named i18n-fr.js would have to be
// reached as the symbol i18nfr_js (critical_binary_data_strips_hyphens). One
// combined file for both languages sidesteps the question.
//
// ── THIS PLUGIN HAS NO HOVER-HELP, AND THIS COMMIT DOES NOT GIVE IT ANY ─────
//
// v1.5.4 carried no data-tip and no data-tooltip anywhere — only five native
// title= attributes on the preset bar, which contract §4 DELETES rather than
// localizes. Their text moved to data-i18n-aria; nothing was invented.
// Authoring hover-help prose is Stage M's job. I18N is therefore empty and
// TIP_BINDINGS is empty, which is this plugin's correct state rather than a
// gap. check-i18n assertion 2 reports it as "0 tip(s) bound" instead of
// passing silently, and the emptiness is only admissible BECAUSE no I18N entry
// carries a body.
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
// I18N — hover-help copy. EMPTY, deliberately. See the header.
//
// Exported all the same because the canonical import line names it and
// trLabel() falls back through it: a control whose tooltip title already IS its
// caption is meant to carry ONE key, and that fallback must exist even on a
// plugin with no tooltips today so Stage M can add bodies here without
// touching the label keys below.
// ============================================================================

export const I18N = Object.freeze({});

// ============================================================================
// LABELS — the visible text of the page. {en:{t}, fr:{t, reviewed}}.
// One string per entry, no body: a label is not a tooltip.
//
// ── THE THREE-ARM D-01 TEST, AND WHERE EACH ARM LANDED ON THIS PAGE ─────────
//
// arm 1 (byte-identical to an AudioParameterChoice option) EXEMPTS eleven
//       strings: Sine / Triangle / Random from wobble_shape, Linear / Exp /
//       Random from unison_dist, 60s / 70s / 80s from wobble_era, and the
//       digit captions of unison_voices. See I18N_EXEMPT.
// arm 2 (a number or a unit) exempts every .knob-value readout and the ' Hz'
//       suffix composed in the controller.
// arm 3 (what ELEMENT receives it) exempts #wobble_rate_value's three
//       tempo-sync words — 4 bars / 2 bars / 1 bar — which are English PROSE
//       written into a node that also holds "2.0 Hz". Reasoned in I18N_EXEMPT
//       rather than skipped in silence.
//
// ── THE COLLISION THIS PAGE FOUND, AND WHY 'Random' IS BOTH ─────────────────
//
// check-i18n assertion 10 matches I18N_EXEMPT by TEXT, not by element. The
// word "Random" appears THREE times on this page: as a wobble_shape option, as
// a unison_dist option, and as the CAPTION of the random_amt knob — whose
// parameter is an AudioParameterFloat named "Randomization" and which is
// therefore a plain caption that must localize. The exempt entry needed by the
// two <option> nodes silences the knob caption too. The caption is keyed
// anyway (label.random, below) and IS localized at runtime; the exemption only
// stops assertion 10 asking for coverage it already has. Reported upstream as
// a gate shape finding, with the negative control that proves it.
//
// "Detune" has the same shape and is handled the other way: the .logo span is
// KEYED with sameAsEn: true rather than exempted, so the knob caption "Detune"
// keeps its own coverage requirement. A text exemption there would have hidden
// the one caption on this page most likely to be forgotten.
//
// ── GEOMETRY, MEASURED AT 600 x 480 ────────────────────────────────────────
//
// Three layout rows are content-sized and would move under French, so three
// per-element pins were added and each was reverted alone to confirm it
// re-breaks the gate. They are named in the CSS beside the rule. Full numbers
// are in the commit message; the short form:
//
//   .preset-action-btn   pinned to 60 px so OUVRIR / SAUVER cannot push
//                        #prevPreset, #presetName and #nextPreset leftward
//   .engine-controls .knob   width: 100% so a caption wider than the 52 px
//                        knob face cannot re-centre the face inside its 91 px
//                        grid cell — probed to move ZERO children in English
//   #feedback_knob .knob-label  pinned to 55 px so RETOUR, which is 13 px
//                        NARROWER than Feedback, cannot shrink the auto grid
//                        column and slide the whole output row
//
// THREE of this page's French strings are SHORTER than their English, which is
// the half a clip check is blind to, and two of the three needed a pin BECAUSE
// they shrink: VOIX 37.7 -> 25.8, RETOUR 54.3 -> 42.4, MONO-SÛR 58.9 -> 54.2.
// ============================================================================

export const LABELS = Object.freeze({

    // ── The header ──────────────────────────────────────────────────────────
    //
    // "Ouaricon Detune" is the product name. The brand word is an I18N_EXEMPT
    // entry (it is unique on the page); the product word is KEYED with
    // sameAsEn rather than exempted, because an exemption is matched by text
    // and "Detune" is also the caption of the unison detune knob, which does
    // translate. sameAsEn says "this was looked at and translates to itself"
    // where silence would say nothing.
    'label.productName': { en: { t: 'Detune' }, fr: { t: 'Detune', reviewed: false, sameAsEn: true } },

    // ── The preset bar ──────────────────────────────────────────────────────
    //
    // OUVRIR / SAUVER rather than CHARGER / ENREGISTRER. Both buttons are
    // shrink-to-fit inside a `justify-content: space-between` header, so every
    // pixel a caption gains pushes #prevPreset, #presetName and #nextPreset to
    // the LEFT, toward a 22 px logo that ends at x = 282. The buttons are
    // pinned to 60 px to make the row language-invariant, and CHARGER (47 px
    // of text) plus ENREGISTRER (74 px) would have needed a 66 px pin that
    // leaves a 2 px gap to the logo. OUVRIR and SAUVER are the compact
    // idiomatic pair and fit the 42 px content box with 2 px to spare.
    'label.load': { en: { t: 'Load' }, fr: { t: 'Ouvrir', reviewed: false } },
    'label.save': { en: { t: 'Save' }, fr: { t: 'Sauver', reviewed: false } },

    // ── The wobble engine ───────────────────────────────────────────────────
    //
    // "Pleurage" is the French audio term for tape wow — the exact effect this
    // engine models — where a literal "Oscillation" would name the mechanism
    // and lose the tape. The panel caption is a 191 px block, so length is
    // free here.
    'label.wobble': { en: { t: 'Wobble' }, fr: { t: 'Pleurage',  reviewed: false } },
    'label.era':    { en: { t: 'Era' },    fr: { t: 'Époque',    reviewed: false } },
    'label.shape':  { en: { t: 'Shape' },  fr: { t: 'Forme',     reviewed: false } },
    'label.rate':   { en: { t: 'Rate' },   fr: { t: 'Vitesse',   reviewed: false } },

    // "Sync" is the term in French audio software as well, and this caption
    // sits under a 36 px toggle in a container the toggle sizes: a translation
    // wider than 36 px would grow the container and re-centre the toggle
    // inside its grid cell. Both facts point the same way.
    'label.sync':   { en: { t: 'Sync' },   fr: { t: 'Sync', reviewed: false, sameAsEn: true } },

    // "Ampleur" rather than "Profondeur". Both are correct for modulation
    // depth; Profondeur renders 68 px against a 52 px knob face and would need
    // the caption to overflow its own box, which assertion 4 reads as a spill.
    // Ampleur is 48 px and fits the face with room.
    'label.depth':  { en: { t: 'Depth' },  fr: { t: 'Ampleur',   reviewed: false } },

    // ── The blend control ───────────────────────────────────────────────────
    // The section is sized by the 64 px knob face, not by this caption, so
    // MÉLANGE at 55 px changes nothing.
    'label.blend':  { en: { t: 'Blend' },  fr: { t: 'Mélange',   reviewed: false } },

    // ── The unison engine ───────────────────────────────────────────────────
    'label.unison': { en: { t: 'Unison' }, fr: { t: 'Unisson',   reviewed: false } },
    'label.voices': { en: { t: 'Voices' }, fr: { t: 'Voix',      reviewed: false } },

    // "Dist" is itself an abbreviation of the parameter's display name,
    // "Unison Distribution". The French abbreviates the same way rather than
    // spelling out RÉPARTITION where the English does not.
    'label.dist':   { en: { t: 'Dist' },   fr: { t: 'Répart.',   reviewed: false } },

    // The knob caption, NOT the product name in the logo. This one translates.
    'label.detune': { en: { t: 'Detune' }, fr: { t: 'Désaccord', reviewed: false } },

    // Stereo panning width of the unison voices — distinct from the output
    // Width slider below, which is the stereo image of the whole plugin.
    // Étalement and Largeur keep the two apart in French as Spread and Width
    // do in English.
    'label.spread': { en: { t: 'Spread' }, fr: { t: 'Étalement', reviewed: false } },

    // The per-voice variation knob. Its parameter is an AudioParameterFloat
    // named "Randomization" — NOT one of unison_dist's option strings — so
    // arm 1 does not reach it and it localizes. See the collision note above.
    'label.random': { en: { t: 'Random' }, fr: { t: 'Aléatoire', reviewed: false } },

    // ── The output section ──────────────────────────────────────────────────
    //
    // The colon rides INSIDE the key. French typography sets a space before a
    // colon and English does not, and that difference belongs in the table
    // rather than in the markup, where only one of the two languages could
    // have it. The caption was split out of the node it shared with
    // #width_value per contract §5.
    'label.width':    { en: { t: 'Width:' },  fr: { t: 'Largeur :', reviewed: false } },

    // Pré-Dly mirrors the English abbreviation of "Pre-Delay" rather than
    // spelling out PRÉ-DÉLAI, which is 61 px against a 50 px knob column.
    'label.preDelay': { en: { t: 'Pre-Dly' }, fr: { t: 'Pré-Dly',   reviewed: false } },

    // 13 px NARROWER than the English. The caption is pinned to the English
    // box precisely because of that: the output row's middle columns are
    // `auto`, so a caption that shrinks slides the row exactly as one that
    // grows does.
    'label.feedback': { en: { t: 'Feedback' }, fr: { t: 'Retour',   reviewed: false } },

    // A coined compound mirroring the English one, which is itself coined.
    // "Compatible mono" is the descriptive French but renders 98 px into a
    // 59 px pinned box.
    'label.monoSafe': { en: { t: 'Mono-Safe' }, fr: { t: 'Mono-Sûr', reviewed: false } },

    // "Mix" is the term in French audio software. Kept rather than "Mixage",
    // which is the ACT of mixing rather than the dry/wet control.
    'label.mix':      { en: { t: 'Mix' }, fr: { t: 'Mix', reviewed: false, sameAsEn: true } },

    // ── The settings popover (v1.6.0) ───────────────────────────────────────
    'label.language': { en: { t: 'Language' }, fr: { t: 'Langue', reviewed: false } },

    // ── Accessible names ────────────────────────────────────────────────────
    //
    // The five entries below are the text of the five native title=
    // attributes v1.5.4 carried, moved verbatim under contract §4 and then
    // translated. Nothing here is new prose.
    'aria.settings':     { en: { t: 'Settings' },           fr: { t: 'Réglages',              reviewed: false } },
    'aria.langSelect':   { en: { t: 'Interface language' }, fr: { t: 'Langue de l’interface', reviewed: false } },
    'aria.prevPreset':   { en: { t: 'Previous preset' },    fr: { t: 'Préréglage précédent',  reviewed: false } },
    'aria.nextPreset':   { en: { t: 'Next preset' },        fr: { t: 'Préréglage suivant',    reviewed: false } },
    'aria.presetList':   { en: { t: 'Click to see all presets' },
                           fr: { t: 'Cliquez pour voir tous les préréglages', reviewed: false } },
    'aria.loadPreset':   { en: { t: 'Load preset from file' },
                           fr: { t: 'Ouvrir un préréglage depuis un fichier', reviewed: false } },
    'aria.savePreset':   { en: { t: 'Save current settings' },
                           fr: { t: 'Sauver les réglages actuels', reviewed: false } },
});

// ============================================================================
// I18N_EXEMPT — reasoned exclusions, never silence
//
// Every visible string the coverage scan finds must be a [data-i18n] element, a
// setLabel() call, or an entry HERE WITH A REASON. A bare skip list would let a
// missed label hide as a deliberate one.
//
// READ THE COLLISION NOTE ABOVE BEFORE ADDING TO THIS LIST. Assertion 10
// matches these by TEXT, so an entry added for one node silences EVERY node on
// the page carrying the same string.
// ============================================================================

export const I18N_EXEMPT = [

    // ── Names ───────────────────────────────────────────────────────────────
    ['Ouaricon',
     'the brand word of the .logo product name "Ouaricon Detune" — a brand is never translated. Unique on this page, so a text-matched exemption is safe here; the product word beside it is KEYED with sameAsEn instead, because "Detune" is also the unison knob caption'],
    ['Default',
     'the preset NAME rendered in #presetName — exempt under D-02, because the name IS the JSON filename the preset manager reads and writes (OuariconPresetManager createPresetJson / loadPreset). Translating it would ask the loader for a file that does not exist'],

    // ── D-01 arm 1: captions that ARE the AudioParameterChoice option strings
    //
    // Byte-identity is the test. A DAW automation lane showing
    // wobble_shape = "Triangle" beside a page reading "Dents de scie" is a bug
    // report, not a localization.
    ['Sine',
     'a wobble_shape AudioParameterChoice option string VERBATIM (PluginProcessor.cpp:79, StringArray {"Sine","Triangle","Random"}) — D-01 arm 1'],
    ['Triangle',
     'a wobble_shape option string VERBATIM (PluginProcessor.cpp:79) — D-01 arm 1. Also spelled identically in French'],
    ['Random',
     'a wobble_shape AND a unison_dist option string VERBATIM (PluginProcessor.cpp:79 and :113) — D-01 arm 1. WARNING: assertion 10 matches this list by TEXT, so this entry also silences the random_amt .knob-label, which is NOT exempt — its parameter is an AudioParameterFloat named "Randomization". That caption is keyed as label.random and is localized at runtime; the exemption only removes a coverage demand it already satisfies'],
    ['Linear',
     'a unison_dist option string VERBATIM (PluginProcessor.cpp:113, StringArray {"Linear","Exp","Random"}) — D-01 arm 1'],
    ['Exp',
     'a unison_dist option string VERBATIM (PluginProcessor.cpp:113) — D-01 arm 1'],
    ['60s',
     'a wobble_era option string VERBATIM (PluginProcessor.cpp:53, StringArray {"60s","70s","80s"}) — D-01 arm 1, and a decade label besides'],
    ['70s',
     'a wobble_era option string VERBATIM (PluginProcessor.cpp:53) — D-01 arm 1'],
    ['80s',
     'a wobble_era option string VERBATIM (PluginProcessor.cpp:53) — D-01 arm 1'],

    // ── D-01 arm 3: prose written into a READOUT node ───────────────────────
    //
    // #wobble_rate_value holds "2.0 Hz" when tempo sync is off and a musical
    // division when it is on. Three of the eleven divisions are English words.
    // They are NOT keyed, and the reason is arm 3 rather than convenience: the
    // canon's setLabel() sets el.dataset.i18n PERMANENTLY, so keying this node
    // while sync is on would leave it a [data-i18n] element after sync goes
    // off, and the next language change would overwrite the live "2.0 Hz" with
    // a stale "4 mesures". That is exactly the enter-and-leave-the-sweep bug
    // O-Marimba's six timbre words produced.
    ['4 bars',
     'written into the #wobble_rate_value READOUT node by getMusicalDivision() when wobble_sync is on — D-01 arm 3. Keying it would make a readout node a permanent [data-i18n] element and clobber the Hz reading after sync goes off'],
    ['2 bars',
     'a tempo-sync division written into the #wobble_rate_value readout — D-01 arm 3, same reason as "4 bars"'],
    ['1 bar',
     'a tempo-sync division written into the #wobble_rate_value readout — D-01 arm 3, same reason as "4 bars". Also the string that would need a plural engine, which contract §6 declines to build'],

    // ── Endonyms ────────────────────────────────────────────────────────────
    ['English',  'endonym — a language name is never translated'],
    ['Français', 'endonym — a language name is never translated'],
];

// ============================================================================
// TIP_BINDINGS — EMPTY. See the header: this plugin has no hover-help.
//
// Exported because the canonical import line names it and applyI18n() iterates
// it. A zero-length loop is the correct no-op; omitting the export and editing
// the canon block to match would put this plugin's copy of the runtime out of
// step with the other forty-two, which is the drift the canon gate exists to
// prevent.
// ============================================================================

export const TIP_BINDINGS = [];

// The tooltip lookup. Returns {t, b} — never null, never a bare key without a
// console.warn saying so, because a silently-missing tip renders as an empty
// surface that looks like a positioning bug rather than a missing entry.
//
// Unreferenced at runtime today: applyI18n() calls it only from the
// TIP_BINDINGS loop, which is empty. It is exported verbatim all the same, so
// the canon block is byte-identical to the other forty-two copies and Stage M
// can add bodies to I18N without touching this file's shape.
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
