/*
   This file is part of O-Comp, an Ouaricon Audio plugin.
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
// i18n.js — O-Comp page labels, English + French (v1.6.0)
//
// An ES module that EXPORTS ONLY. It must never self-execute: a bare top-level
// statement here throws out of module evaluation and takes every later
// initializer on the page with it (pattern_module_toplevel_init_tdz). This
// plugin's controller is a single inline <script type="module"> in index.html,
// so that failure mode would take the WHOLE UI rather than one panel of it.
// scripts/check-i18n.js assertion 7 enforces it.
//
// FILENAME: no hyphen. juce_add_binary_data STRIPS hyphens rather than
// converting them to underscores, so a file named i18n-fr.js would have to be
// reached as the symbol i18nfr_js (critical_binary_data_strips_hyphens). One
// combined file for both languages sidesteps the question.
//
// ── THIS PLUGIN HAS NO HOVER-HELP, AND THIS COMMIT DOES NOT GIVE IT ANY ─────
//
// v1.5.0 carried no data-tip and no data-tooltip anywhere — only five native
// title= attributes on the preset bar, which contract section 4 DELETES rather
// than localizes. Their text moved to data-i18n-aria; nothing was invented.
// Authoring hover-help prose is Stage M's job. TIP_BINDINGS is therefore empty,
// which is this plugin's correct state rather than a gap: check-i18n assertion
// 2 reports it as "0 tip(s) bound", and the emptiness is admissible only
// BECAUSE no I18N entry carries a non-empty body.
//
// ── I18N IS NOT EMPTY HERE, AND THE REASON IS THE CANVAS ────────────────────
//
// Three of this page's user-facing strings are painted with
// CanvasRenderingContext2D.fillText into #envelopeCanvas. A canvas string has
// no element, so it can be neither a [data-i18n] node nor a setLabel() target,
// and check-i18n cannot see it at all: assertion 10 walks TEXT NODES and
// assertion 12 scans textContent/innerText writes. Neither reaches fillText,
// so this is a localization NOBODY WOULD HAVE FAILED US FOR SKIPPING — and a
// French user would have read "Envelope" and "Gain Reduction" in English for
// it.
//
// They are housed in I18N with an EMPTY body, which is the shape O-Polystutter
// v1.14.0 established for a homeless composed string and which check-i18n
// assertion 2 names explicitly. They are read through trLabel() from inside
// the render loop, so they follow the language selector on the very next
// animation frame with no re-render hook of their own.
//
// They are NOT in LABELS, and that is deliberate rather than arbitrary:
// assertion 15 fails any LABELS key that no element and no setLabel call
// references, and a trLabel() call from a paint routine is in neither set. A
// canvas string put in LABELS would report as a DEAD key on a page that reads
// it thirty times a second.
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
// I18N — hover-help copy, plus the three CANVAS-PAINTED strings.
//
// Every entry below carries an EMPTY body. A body is what makes an entry a
// TOOLTIP and what makes assertion 2 demand a TIP_BINDINGS row for it; these
// three are captions with nowhere to live, not tips.
// ============================================================================

export const I18N = Object.freeze({

    // Painted at 9px into #envelopeCanvas by startRenderLoop(). The canvas is
    // 308px wide and both strings are drawn at x = 10, so length is free:
    // measured 33.99 -> 38.49 and 56.74 -> 65.48 with ctx.measureText in this
    // plugin's own context, not borrowed from another plugin.
    'canvas.envelope': {
        en: { t: 'Envelope',  b: '' },
        fr: { t: 'Enveloppe', b: '', reviewed: false },
    },
    'canvas.gainReduction': {
        en: { t: 'Gain Reduction',    b: '' },
        fr: { t: 'Réduction de gain', b: '', reviewed: false },
    },

    // The live gain-reduction readout, painted at 11px. The VALUE is composed
    // by the caller and passed as {v}; the abbreviation and the unit are the
    // only localized parts. Authored as ONE entry with a token rather than two
    // entries chosen by a ternary, per contract section 6 — the shipped code
    // used to pick between `GR: -x dB` and `GR: 0.0 dB`, and that branch is now
    // on the VALUE, where no language can reach it.
    //
    // French puts a space before a colon. Measured 59.59 -> 62.34 at the widest
    // reading, drawn at x = 10 in a 308px canvas.
    'canvas.gr': {
        en: { t: 'GR: {v} dB',  b: '' },
        fr: { t: 'RG : {v} dB', b: '', reviewed: false },
    },
});

// ============================================================================
// LABELS — the visible text of the page. {en:{t}, fr:{t, reviewed}}.
// One string per entry, no body: a label is not a tooltip.
//
// ── THE THREE-ARM D-01 TEST, AND WHERE EACH ARM LANDED ON THIS PAGE ─────────
//
// arm 1 (byte-identical to an AudioParameterChoice option) exempts NOTHING
//       here. O-Comp has no AudioParameterChoice at all: six
//       AudioParameterFloat and one AudioParameterBool
//       (PluginProcessor.cpp createParameterLayout). There is no automation
//       lane anywhere on this plugin that a translated caption could contradict.
// arm 2 (a number or a unit) exempts all six .value-display readouts —
//       "-20.0 dB", "4.0:1", "10.0 ms", "100 ms", "6.0 dB", "0.0 dB" — and the
//       two nav glyphs. The extractor classifies every one of them READOUT, so
//       assertion 10 never asks about them and they need no I18N_EXEMPT row.
// arm 3 (what ELEMENT receives it) exempts #preset-name, which holds a preset
//       NAME. It is listed in I18N_EXEMPT because the D-02 filename reason is
//       the stronger one and deserves to be written down.
//
// The ONE arm-3 call that went the other way is #auto-gain-toggle. It is
// written by JS from a bool and it is keyed anyway, because that node NEVER
// holds a number: auto_gain is an AudioParameterBool, the node's only two
// states are the two words below, and setLabel()'s permanent data-i18n
// therefore cannot strand a live reading the way it would on a readout that
// alternates between a word and a value (O-Marimba's six timbre words,
// O-Detune's #wobble_rate_value).
//
// ── THE GEOMETRY BUDGET, MEASURED IN THIS PLUGIN'S OWN ELEMENTS ─────────────
//
// Each of the six knob columns is a `.control-group`: position:absolute with
// no width, so it SHRINK-WRAPS to its widest child. In English every one of
// them is exactly 52px wide — the knob face — because every English caption is
// narrower than 52. `align-items: center` then centres the knob inside that
// box, so a French caption WIDER than 52px would widen the group and slide the
// knob, the value readout and the vine arc sideways by half the excess. That
// is the page's one real cliff, and it is invisible to a clip check: nothing
// clips, nothing wraps, the knobs just move.
//
// So 52px is a hard budget, and every French caption below was measured
// against it by writing it into the actual .param-label and reading the box
// back. NOT pinned: a width pin on .control-group would hold the group's
// rectangle still while letting a longer caption overflow it, and assertion 5
// reads that overflow as a French spill its offsetParent did not have in
// English. The budget is the fix; the pin would only move the failure.
//
// Threshold 49.11 -> Seuil 25.13    Attack 32.33 -> Attaque 38.33
// Ratio     26.34 -> Ratio 26.34    Release 37.70 -> Relâche 38.94
// Knee      25.22 -> Genou 31.83    Output 33.56 -> Sortie 29.28
// Auto-Gain 51.55 -> Gain auto 47.58
//
// FRENCH SHRINKS ON THREE OF THE SEVEN. A gate that only looked for growth
// would have certified this page and missed that Seuil takes 24px off the
// widest caption on the row.
// ============================================================================

export const LABELS = Object.freeze({

    // ── The preset bar ──────────────────────────────────────────────────────
    //
    // Both buttons are `width: 32px` with 1px borders, so the content box is
    // 30px and the button cannot grow — it is a flex item whose min-width floor
    // is its min-content, and every candidate below is under 30px, so the
    // .preset-bar row keeps its 164px and the whole header stays put.
    //
    // OUVRIR / SAUVER rather than CHARGER / ENREGISTRER: measured in this
    // plugin's own .preset-action-btn, Charger is 28.83 against a 30px content
    // box (1.17px of margin, tighter than anything shipped in Stage K so far)
    // and Enregistrer is 39.98 and does not fit at all. Ouvrir 24.00 and
    // Sauver 25.00 leave 5-6px.
    'label.load': { en: { t: 'Load' }, fr: { t: 'Ouvrir', reviewed: false } },
    'label.save': { en: { t: 'Save' }, fr: { t: 'Sauver', reviewed: false } },

    // ── The six knob captions ───────────────────────────────────────────────
    'label.threshold': { en: { t: 'Threshold' }, fr: { t: 'Seuil',   reviewed: false } },

    // "Ratio" is the term in French audio software as well as English. Keyed
    // with sameAsEn rather than exempted: an exemption is matched by TEXT and
    // says nothing about whether anybody looked, while sameAsEn says this was
    // looked at and translates to itself. "Taux" measured 23.84 and would also
    // have fit — the choice is terminology, not geometry.
    'label.ratio': { en: { t: 'Ratio' }, fr: { t: 'Ratio', reviewed: false, sameAsEn: true } },

    'label.attack': { en: { t: 'Attack' }, fr: { t: 'Attaque', reviewed: false } },

    // "Relâche", NOT "Relâchement", and the reason is the 52px budget above:
    // Relâchement measures 62.92 in this plugin's .param-label and would widen
    // the Release column to 62.92, sliding its knob 5.46px left of every other
    // knob on the row. Relâche is 38.94 and changes nothing.
    //
    // FLAGGED FOR THE REVIEWER: Relâchement is the fuller term a French
    // compressor UI would normally use, and this entry trades register for the
    // column staying where it is. If a native speaker rejects Relâche, the
    // alternative is not a longer string — it is a layout change to the row.
    'label.release': { en: { t: 'Release' }, fr: { t: 'Relâche', reviewed: false } },

    // "Genou" is the standard French rendering of a compressor knee (genou
    // doux / genou dur). "Coude" measured 31.23 and would also fit.
    'label.knee':   { en: { t: 'Knee' },   fr: { t: 'Genou',  reviewed: false } },
    'label.output': { en: { t: 'Output' }, fr: { t: 'Sortie', reviewed: false } },

    // ── The auto-gain toggle ────────────────────────────────────────────────
    //
    // The caption above the button, then the two faces written INTO it.
    // "Gain automatique" measured 86.52 and would have widened that column from
    // 70px (the toggle) to 86.52 and re-centred the toggle; "Gain auto" is
    // 47.58 and sits well inside.
    'label.autoGain': { en: { t: 'Auto-Gain' }, fr: { t: 'Gain auto', reviewed: false } },

    // MARCHE / ARRÊT, measured 45.56 and 35.02 against the button's 66px
    // content box (70px minus two 2px borders). DÉSACTIVÉ measured 58.36 and
    // would also have fitted, but the pair has to be legible at 10px bold in a
    // 40px-tall button and ARRÊT/MARCHE is the shorter, higher-contrast pair.
    //
    // Written through setLabel() from an if/else, never a ternary inside the
    // call: check-i18n assertion 13 rejects a conditional in a setLabel
    // argument, because contract section 6 authors around an inflection rather
    // than engineering one.
    'label.autoGainOn':  { en: { t: 'ON' },  fr: { t: 'MARCHE', reviewed: false } },
    'label.autoGainOff': { en: { t: 'OFF' }, fr: { t: 'ARRÊT',  reviewed: false } },

    // ── The visualisation caption ───────────────────────────────────────────
    //
    // .viz-label is absolutely positioned, bottom-centred with
    // translateX(-50%), inside a 210px panel. It grows SYMMETRICALLY about the
    // panel's centre line, so it pushes nothing: measured 77.42 -> 94.98 box
    // width, still 115px inside the panel it sits in.
    'label.transferCurve': { en: { t: 'Transfer Curve' }, fr: { t: 'Courbe de transfert', reviewed: false } },

    // ── The preset dropdown, built at runtime ───────────────────────────────
    //
    // Both are written by buildDropdownMenu() into elements it creates, so both
    // go through setLabel() and become [data-i18n] elements from that moment.
    //
    // "Aucun préréglage" is SINGULAR on purpose. French inflects zero as
    // singular and English does not, and contract section 6 declines to build a
    // plural engine for one string: the copy is authored so it reads correctly
    // at zero in both languages instead.
    'label.presets':   { en: { t: 'Presets' },    fr: { t: 'Préréglages',      reviewed: false } },
    'label.noPresets': { en: { t: 'No presets' }, fr: { t: 'Aucun préréglage', reviewed: false } },

    // ── The settings popover ────────────────────────────────────────────────
    'label.language': { en: { t: 'Language' }, fr: { t: 'Langue', reviewed: false } },

    // ── Accessible names ────────────────────────────────────────────────────
    //
    // Every one of these is the text of a native title= that contract section 4
    // DELETES. Nothing here is invented: each string is the one that was on the
    // element at v1.5.0, translated.
    //
    // LABEL-IN-NAME (WCAG 2.5.3) HELD IN BOTH LANGUAGES. The accessible name of
    // a control with a visible caption must CONTAIN that caption, so a speech
    // user can say what they read. English: "Load" inside "Load preset",
    // "Save" inside "Save preset". French: "Ouvrir" inside "Ouvrir un
    // préréglage", "Sauver" inside "Sauver un préréglage". Choosing CHARGER for
    // the button and leaving "Ouvrir un préréglage" on the name is exactly the
    // defect found on O-DigiDelay in batch K2, and it is only visible when the
    // two strings are read together.
    'aria.prevPreset':    { en: { t: 'Previous preset' },        fr: { t: 'Préréglage précédent',              reviewed: false } },
    'aria.nextPreset':    { en: { t: 'Next preset' },            fr: { t: 'Préréglage suivant',                reviewed: false } },
    'aria.browsePresets': { en: { t: 'Click to browse presets' }, fr: { t: 'Cliquer pour parcourir les préréglages', reviewed: false } },
    'aria.loadPreset':    { en: { t: 'Load preset' },            fr: { t: 'Ouvrir un préréglage',              reviewed: false } },
    'aria.savePreset':    { en: { t: 'Save preset' },            fr: { t: 'Sauver un préréglage',              reviewed: false } },
    'aria.settings':      { en: { t: 'Settings' },               fr: { t: 'Paramètres',                        reviewed: false } },
    'aria.langSelect':    { en: { t: 'Interface language' },     fr: { t: "Langue de l'interface",             reviewed: false } },
});

// ============================================================================
// I18N_EXEMPT — reasoned exclusions, never silence
//
// Every visible string the coverage scan finds must be a [data-i18n] element, a
// setLabel() call, or an entry HERE WITH A REASON. A bare skip list would let a
// missed label hide as a deliberate one.
//
// AN EXEMPTION IS MATCHED BY TEXT, so an unscoped entry silences EVERY node on
// the page carrying that string. The third field is a scope — a comma-separated
// list of `tag`, `.class` or `#id` matched against the node's parent and its
// ancestors — and it is REQUIRED exactly where a string is both exempt and
// keyed on the same page, which is the one state in which the gate cannot tell
// a deliberate skip from a label somebody forgot (assertion 14).
//
// NONE of the four below is in that state. No key in LABELS or I18N resolves to
// "Ouaricon Compressor", "Default", "English" or "Français" in either language,
// so all four are correctly unscoped and assertion 14 passes without one. That
// was checked against the table, not assumed.
// ============================================================================

export const I18N_EXEMPT = [

    // ── The product display name ────────────────────────────────────────────
    ['Ouaricon Compressor',
     'the product display name in div.title, and the same string in the document title element '
     + '— a product name is never translated, and this is the brand-plus-product form of the '
     + 'plugin\'s registered PRODUCT_NAME "O-Comp" in CMakeLists.txt'],

    // ── D-02: the preset name IS the filename ───────────────────────────────
    ['Default',
     'the PRESET NAME shown in #preset-name, not a caption — D-02. The name is the JSON '
     + 'filename on disk (OuariconPresetManager sanitizes it into getUserPresetsDirectory()), '
     + 'and it is written into this node at runtime by the VENDORED '
     + 'modules/preset-manager.js, which is a shared copy: localizing it would rename presets '
     + 'in one language and orphan the files'],

    // ── Endonyms ────────────────────────────────────────────────────────────
    //
    // The extractor classifies both as ENDONYM rather than LABEL, so assertion
    // 10 would skip them regardless. Listed anyway, because the next person to
    // read this file should find the rule written down rather than have to
    // rediscover that a classifier happens to cover it.
    ['English',  'endonym — a language name is never translated'],
    ['Français', 'endonym — a language name is never translated'],
];

// ============================================================================
// TIP_BINDINGS — EMPTY. See the header: this plugin has no hover-help.
//
// Exported because the canonical import line names it and applyI18n() iterates
// it. A zero-length loop is the correct no-op; omitting the export and editing
// the canon block to match would put this plugin's copy of the runtime out of
// step with the other forty-plus, which is the drift the canon gate exists to
// prevent.
// ============================================================================

export const TIP_BINDINGS = [];

// The tooltip lookup. Returns {t, b} — never null, never a bare key without a
// console.warn saying so, because a silently-missing tip renders as an empty
// surface that looks like a positioning bug rather than a missing entry.
//
// Unreferenced at runtime today: applyI18n() calls it only from the
// TIP_BINDINGS loop, which is empty. It is exported verbatim all the same, so
// the canon block is byte-identical to every other copy and Stage M can add
// bodies to I18N without touching this file's shape.
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
