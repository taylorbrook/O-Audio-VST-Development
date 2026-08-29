/*
   This file is part of O-Freeze, an Ouaricon Audio plugin.
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
// i18n.js — O-Freeze page labels, English + French (v2.1.0)
//
// An ES module that EXPORTS ONLY. It must never self-execute: a bare top-level
// statement here throws out of module evaluation and takes every later
// initializer on the page with it (pattern_module_toplevel_init_tdz). This
// plugin's controller is an inline <script type="module"> in index.html — there
// is no js/app.js and no stylesheet file — so that failure mode would take the
// WHOLE UI, not a panel of it. scripts/check-i18n.js assertion 7 enforces it.
//
// FILENAME: no hyphen. juce_add_binary_data STRIPS hyphens rather than
// converting them to underscores, so a second file named i18n-fr.js would have
// to be reached as the symbol i18nfr_js (critical_binary_data_strips_hyphens).
// One combined file for both languages sidesteps the question entirely.
//
// ── THIS PLUGIN HAS NO HOVER-HELP, AND THIS COMMIT DOES NOT GIVE IT ANY ─────
//
// v2.0.1 carried no data-tip, no data-tooltip and no native title= anywhere on
// the page — the extractor reports `attr 0`. So there is no tooltip copy to
// MOVE here and none is INVENTED: authoring hover-help prose is Stage M's job.
// I18N is therefore empty and TIP_BINDINGS is empty, which is this plugin's
// correct state rather than a gap. check-i18n assertion 2 reports it as
// "0 tip(s) bound" instead of passing silently, and the emptiness is only
// admissible BECAUSE no I18N entry carries a body.
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
// trLabel() falls back through it, so Stage M can add bodies here without
// touching the label keys below.
// ============================================================================

export const I18N = Object.freeze({});

// ============================================================================
// LABELS — the visible text of the page. {en:{t}, fr:{t, reviewed}}.
//
// One string per entry, no body: a label is not a tooltip.
//
// ── THE FOUR READOUT NODES, AND WHY NONE OF THEM IS HERE ────────────────────
//
// The extractor classifies four of this page's twenty-two text nodes as
// READOUT: `-40.0 dB` (#threshold-knob), `400 ms` (#grain-size-knob),
// `5.0 ct` (#detune-knob) and `0.50 Hz` (#lfo-rate-knob). Every one is a
// `div.knob-value`, and every one is overwritten on the first frame by
// setupKnob()'s formatValue() from state.getScaledValue(). They are exempt
// THREE TIMES OVER and it is worth naming which arm each rests on, because the
// arms disagree on other plugins:
//
//   arm 2 (D-03)  each is a number plus a unit symbol — dB, ms, ct, Hz — and a
//                 unit symbol is language-neutral.
//   arm 3         each is a READOUT NODE, and a readout node is never a
//                 [data-i18n] element regardless of the parameter behind it.
//                 This is the arm that would still exempt them if a future
//                 version made one of them wear a word instead of a number:
//                 keying it would make the element enter and leave the label
//                 sweep as the knob turns (the O-Marimba finding).
//   contract §5   a readout and a label share no node on this page. `.knob-label`
//                 and `.knob-value` are already two sibling divs in every one of
//                 the eight knobs, so NO SPLIT WAS NEEDED and no markup was
//                 restructured for one. The split that §5 authorises is the
//                 change that can move geometry; not making it is why this
//                 plugin's English before/after diff is clean.
//
// The two remaining `.knob-value` nodes (`0%`, `8`, `100%`, `50%`) are bare
// numbers and do not even reach the READOUT class.
//
// ── GEOMETRY: EVERY FRENCH STRING WAS CHOSEN AGAINST A MEASURED BUDGET ──────
//
// The six main knobs live in `#knobs-container`, a 530 px `justify-content:
// space-around` flex row. Each `.knob` shrink-wraps to max(60 px visual,
// 60 px `.knob-value` min-width, LABEL WIDTH). So a label under 60 px is FREE —
// the item stays 60 px and nothing in the row moves — and a label over 60 px
// widens its item, changes the row's total, and redistributes the slack across
// all six. That 60.00 px is a hard budget, not a guideline. Measured at
// 550 x 530, rendered text width:
//
//     DRIFT      35.59 -> DÉRIVE  44.14    budget 60, spare 15.9
//     SIZE       26.06 -> TAILLE  41.41    spare 18.6
//     GRAINS     44.16 -> GRAINS  44.16    identical word — sameAsEn
//     DETUNE     47.98 -> ÉCART   37.88    SHRANK, and see the note below
//     MIX        23.28 -> DOSAGE  47.05    spare 13.0
//
// The two LFO knobs are `.knob-small`: a 42 px visual, but `.knob-value` still
// carries `min-width: 60px`, so the budget is the same 60.00 px.
//
//     RATE       33.09 -> VITESSE 52.14    spare 7.9
//     DEPTH      42.91 -> PROF.   37.17    SHRANK, and see the note below
//
// Three of seven SHRINK rather than grow. That is the half a clip check is
// blind to, and the half Stage J found four times in twelve.
// ============================================================================

export const LABELS = Object.freeze({

    // ── The freeze button ───────────────────────────────────────────────────
    // #freeze-label is `position: absolute` + `translate(-50%, -50%)` inside a
    // fixed 140 px button, with `pointer-events: none`. Its width is therefore
    // free: it can neither push a sibling nor be pushed. GELER 69.72 against
    // FREEZE's 81.16, comfortably inside the 140 px shape.
    //
    // The VERB, not the noun. This is the button you press to freeze the
    // buffer, and the parameter behind it is an AudioParameterBool named
    // "Freeze" (PluginProcessor.cpp:38-41) — a bool has no option strings, so
    // there is no automation-lane spelling for the caption to disagree with and
    // D-01 arm 1 does not apply.
    'label.freeze': { en: { t: 'Freeze' }, fr: { t: 'GELER', reviewed: false } },

    // ── The reverse toggle ──────────────────────────────────────────────────
    // Also an AudioParameterBool ("Reverse", PluginProcessor.cpp:112-115), so
    // arm 1 does not apply here either. INVERSE is 55.33 against REVERSE's
    // 57.50 — a 2.17 px SHRINK, which would have pulled #reverse-container (a
    // non-label element, centred by translateX(-50%)) in by 1.09 px and failed
    // assertion 7. The `min-width` pin in index.html holds the pill at its
    // English 95.5 px; see the comment there, and the negative control in the
    // commit message.
    'label.reverse': { en: { t: 'Reverse' }, fr: { t: 'INVERSE', reviewed: false } },

    // ── The six main knob captions ──────────────────────────────────────────
    // Each is the plugin's own caption for a FLOAT or INT parameter, not a
    // choice option, so all six are localizable under D-01 arm 1 and none is a
    // readout node under arm 3. THRESHOLD is the exception and is EXEMPT — see
    // I18N_EXEMPT, where the reason is the whole judgement call on this plugin.
    'label.drift':  { en: { t: 'Drift' },  fr: { t: 'DÉRIVE', reviewed: false } },
    'label.size':   { en: { t: 'Size' },   fr: { t: 'TAILLE', reviewed: false } },

    // GRAINS is the same word in both languages — `grain` is French, and the
    // plural is spelled identically. sameAsEn is the explicit declaration that
    // this is a translation and not an untranslated leftover; without it,
    // check-i18n assertion 4 rejects the entry as a silent passthrough, which
    // is exactly the guard that should fire on a string nobody thought about.
    'label.grains': { en: { t: 'Grains' }, fr: { t: 'GRAINS', sameAsEn: true, reviewed: false } },

    // ÉCART rather than DÉSACCORD, and the reason is BOTH width and meaning.
    // DÉSACCORD renders at 70.14 px against the 60.00 px budget: it would widen
    // #detune-knob from 60 to 70.14, change the space-around row total, and move
    // all six knobs and their eighteen children. ÉCART is 37.88. It is also the
    // better word: DETUNE here sets a RANGE of per-grain pitch offsets in cents
    // (`Per-grain pitch micro-detuning range in cents`, PluginProcessor.cpp:117),
    // and `écart` names a spread where `désaccord` names a state of being out
    // of tune. The budget forced the question; the answer stands on its own.
    'label.detune': { en: { t: 'Detune' }, fr: { t: 'ÉCART', reviewed: false } },

    // DOSAGE over MÉLANGE, which also fits (57.02) but with only 2.98 px of
    // slack. Windows/WebView2 font metrics are this rollout's named
    // hardware-blocked deferral, so a 13 px margin is worth more than a
    // marginally more literal word. DOSAGE is the standard French label for a
    // dry/wet blend amount.
    'label.mix':    { en: { t: 'Mix' },    fr: { t: 'DOSAGE', reviewed: false } },

    // ── The LFO group ───────────────────────────────────────────────────────
    // #lfo-group-label is `position: absolute` inside #lfo-group, so its width
    // is free: 89.03 against 79.22 pushes nothing, and 12 + 89.03 is still well
    // inside the group's 283.67 px.
    'label.driftLfo': { en: { t: 'Drift LFO' }, fr: { t: 'LFO DÉRIVE', reviewed: false } },

    'label.rate':  { en: { t: 'Rate' },  fr: { t: 'VITESSE', reviewed: false } },

    // PROF., abbreviated, and this one is a genuine compromise rather than a
    // better word found under pressure. PROFONDEUR is 87.30 px against a 60.00
    // budget and AMPLEUR is 61.16 — over by 1.16, which is above the gate's
    // 0.5 px tolerance and would re-centre the whole translateX(-50%) LFO group.
    // No pin rescues it: pinning `.knob-small` wider moves the ENGLISH layout,
    // and pinning it at 60 converts the overflow into a clip rather than
    // preventing it. So the caption is abbreviated, the way a tight French UI
    // abbreviates it, and flagged for a native speaker like every other string
    // here.
    'label.depth': { en: { t: 'Depth' }, fr: { t: 'PROF.', reviewed: false } },

    // #lfo-shape-label sits above a 105.67 px selector in a column that
    // shrink-wraps to the WIDER of the two, so FORME's 44.89 is free.
    'label.shape': { en: { t: 'Shape' }, fr: { t: 'FORME', reviewed: false } },

    // ── The three LFO shape captions ────────────────────────────────────────
    // NOT exempt under D-01 arm 1. The LFO_SHAPE AudioParameterChoice options
    // are spelled "Sine", "Triangle", "Random" (PluginProcessor.cpp:108) and
    // these captions are "Sin", "Tri", "Rnd" — abbreviations, not the option
    // strings. Byte-identity is the test, and it fails, so they localize: this
    // is the `CUSTOM` against an option spelled `Scala` case from the contract,
    // not the `12-TET` one.
    //
    // Sin and Tri abbreviate identically in French (sinus, triangle), so both
    // are sameAsEn. Only Rnd changes, and it is the string that made the pin
    // necessary: Alé renders 4.69 px NARROWER than Rnd, which would have shrunk
    // #lfo-shape-selector, shrunk #lfo-shape-toggle, shrunk #lfo-group and
    // re-centred every one of its children. A French string getting SHORTER is
    // the failure mode a clip check cannot see.
    'label.shape.sin': { en: { t: 'Sin' }, fr: { t: 'Sin', sameAsEn: true, reviewed: false } },
    'label.shape.tri': { en: { t: 'Tri' }, fr: { t: 'Tri', sameAsEn: true, reviewed: false } },
    'label.shape.rnd': { en: { t: 'Rnd' }, fr: { t: 'Alé', reviewed: false } },

    // ── The settings popover (v2.1.0) ───────────────────────────────────────
    'label.language': { en: { t: 'Language' }, fr: { t: 'Langue', reviewed: false } },

    // ── Accessible names ────────────────────────────────────────────────────
    // Resolved through the same sweep via data-i18n-aria, so a screen reader
    // hears the same language the page is showing. These are the page's only
    // aria-label attributes; v2.0.1 had none, and none of them replaces a
    // deleted native title= because there were no native title attributes to
    // delete. No hover-help prose is invented here.
    'aria.settings':   { en: { t: 'Settings' },           fr: { t: 'Réglages',              reviewed: false } },
    'aria.langSelect': { en: { t: 'Interface language' }, fr: { t: 'Langue de l’interface', reviewed: false } },
});

// ============================================================================
// I18N_EXEMPT — reasoned exclusions, never silence
//
// Every visible string the coverage scan finds must be a [data-i18n] element, a
// setLabel() call, or an entry HERE WITH A REASON. A bare skip list would let a
// missed label hide as a deliberate one.
// ============================================================================

export const I18N_EXEMPT = [
    ['Ouaricon Granular Freeze',
     'the product display name in the #header h1 — a product name is never translated. It is the brand-plus-product form of the registered PRODUCT_NAME "O-Freeze" in CMakeLists.txt, and the same shape as O-AnalogSaturation\'s exempt "OUARICON SATURATION"'],

    // ── D-01 arm 1: the MODE captions, and the word they drag with them ─────
    //
    // "Manual" and "Threshold" are the MODE AudioParameterChoice option strings
    // BYTE FOR BYTE (PluginProcessor.cpp:52-56, StringArray {"Manual",
    // "Threshold"}). Translating the buttons alone would make the page and the
    // host automation lane disagree about the same setting.
    //
    // "Threshold" ALSO appears as #threshold-knob's `.knob-label`, and that
    // second occurrence is exempt too — deliberately, for three reasons:
    //
    //   1. Byte-identity is the stated test, and the string IS byte-identical
    //      to a live choice option on this page.
    //   2. The knob and the mode button name the SAME threshold. A page reading
    //      SEUIL over the knob and Threshold on the button beside it describes
    //      one control as two.
    //   3. check-i18n's exempt set is matched by TEXT, not by element, so
    //      "localize one occurrence, exempt the other" is not a state the gate
    //      can hold: an exempt entry for the button silently covers the knob as
    //      well. Exempting both is the only reading the gate can express
    //      faithfully, and the divergence is reported rather than worked around.
    //
    // A French user therefore sees three English words on this page, all three
    // naming the same host-visible setting.
    ['Manual',
     'a MODE AudioParameterChoice option string VERBATIM (PluginProcessor.cpp:55) — D-01 arm 1'],
    ['Threshold',
     'a MODE AudioParameterChoice option string VERBATIM (PluginProcessor.cpp:55) — D-01 arm 1. This entry ALSO covers #threshold-knob\'s .knob-label, which carries the same word for the same setting; see the block comment above for why both occurrences stay English'],

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
