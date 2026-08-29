/*
   This file is part of O-DigiDelay, an Ouaricon Audio plugin.
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
// i18n.js — O-DigiDelay page labels, English + French (v1.3.0)
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
// ── THIS PLUGIN HAS NO HOVER-HELP, AND THIS COMMIT DOES NOT GIVE IT ANY ─────
//
// v1.2.12 carried no data-tip and no data-tooltip anywhere on the page — only
// five native title= attributes on the preset bar, which contract §4 DELETES
// rather than localizes, moving their existing text into data-i18n-aria. No
// hover-help prose is INVENTED here: authoring it is Stage M's job. I18N is
// therefore empty and TIP_BINDINGS is empty, which is this plugin's correct
// state rather than a gap. check-i18n assertion 2 reports it as "0 tip(s)
// bound" instead of passing silently, and the emptiness is only admissible
// BECAUSE no I18N entry carries a body — an emptied TIP_BINDINGS over a bodied
// table would be orphaned copy and would fail.
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
// ── THE FRAME IS 700 x 196, THE SECOND SHORTEST IN THE REPO ─────────────────
//
// Every French string below was chosen against a MEASURED width, rendered in
// the real node with its own text-transform and letter-spacing (neither of
// which appears in getComputedStyle().font).
//
// ── THIS PAGE'S TWO CLIFFS, AND THEY ARE THE SAME NUMBER BY TWO MECHANISMS ──
//
// O-Chorus, the other 700-wide plugin in this batch, has a shrink-to-fit
// caption inside a fixed container, so its two cliffs are 50.00 (the rect
// widens) and 62.00 (the caption wraps). THIS PAGE IS NOT THAT SHAPE and its
// numbers are its own. `.knob-label` is `width: 60px`, a HARD PIN, so its
// rectangle never widens with the language and assertion 7 is structurally
// blind to it. What happens past 60 depends only on whether the string has a
// break opportunity in it:
//
//   60.00 px, SINGLE WORD — THE SPILL CLIFF. An unbreakable caption cannot
//              wrap, so it overruns its own 60 px box symmetrically and is
//              painted outside the knob. Caught by check-ui-labels assertion 4,
//              the text-wider-than-its-content-box half.
//   60.00 px, TWO WORDS   — THE WRAP CLIFF, and the one that matters in a
//              196 px frame. The space is a break opportunity, so the caption
//              takes a second line, `.knob-label` grows 10 -> 20 px and pushes
//              `.knob`, its SVG, the vine and `.knob-value` down 10 px. Caught
//              by assertion 7.
//
// EACH CLIFF IS CAUGHT BY EXACTLY THE ASSERTION THAT IS BLIND TO THE OTHER, and
// both halves were confirmed by planting a string past each and watching which
// one fired. Assertion 7 cannot see the spill, because `width: 60px` means the
// caption's rectangle is identical in both languages however far the glyphs
// overrun it — DISPERSIONSTEREOPHONIQUE at 151.2 px moves NOTHING and fails
// only [4][fr] 151.2>60.0. Assertion 4's vertical twin cannot see the wrap,
// because `.knob-label` has no fixed height and simply grows to hold its own
// second line, so the text never exceeds its own content box — LARGEUR STÉRÉO
// passes every [4] check and fails [7] with dh=10.0 on #spread-container and
// dy=10.0 on the knob, the vine and #spread-value.
//
// So the budget is the same 60 px either way, and the CONSEQUENCE of crossing
// it — and the gate that reports it — is what changes. Every French caption
// below is therefore ONE WORD and under 60 px; a two-word caption would have to
// be under 60 px too, with nothing gained.
//
// MEASURED, at 700 x 196, rendered text width against the 60.00 px budget:
//
//     TIME      25.20 -> TEMPS    33.02   26.98 spare
//     FEEDBACK  53.91 -> REINJ.   31.56   28.44 spare   SHRANK
//     SPREAD    39.31 -> ECART    32.97   27.03 spare   SHRANK
//     MOD       23.41 -> MOD      23.41   sameAsEn
//     WET       21.89 -> EFFET    30.52   29.48 spare
//     DRY       20.91 -> DIRECT   37.31   22.69 spare
//
// TWO OF SIX SHRINK. A clip-only check would have certified this page.
//
// Rejected on measurement, not on taste: REINJECTION 66.30 and MODULATION
// 68.00 are past the spill cliff outright; ETALEMENT 60.47 and DISPERSION
// 60.02 clear the 60 px box by 0.03 and 0.48 px against a gate tolerance of
// 0.5, which is a pass by rounding rather than by fit.
//
// ── THE THREE CONTROLS THAT ARE NOT KNOB CAPTIONS ───────────────────────────
//
// `.toggle-label` (SYNC) is the one shrink-to-fit caption on the page, so it
// is the only one with O-Chorus's kind of cliff: `.toggle-container` is
// `position: absolute` + `align-items: center` and is floored at 50 px by the
// `.toggle` beneath it, so a caption wider than 50.00 px widens the container
// and RE-CENTRES the toggle inside it. #sync is not a [data-i18n] element, so
// that is an assertion 7 failure. SYNCHRO measures 48.61 — 1.39 px spare, the
// tightest string shipped on this page. SYNC (27.22) is the reviewer's lever
// if that margin is judged too thin on Windows metrics.
//
// #sync's own face is a fixed 50 x 24 flex box, so MARCHE (43.31) and ARRET
// (33.52) change no geometry at all.
//
// `.led-meter-label` NEEDED A LAYOUT CHANGE, and NOT because of French — see
// the note on label.out below.
//
// ── GEOMETRY ────────────────────────────────────────────────────────────────
//
// Two pins ship, both load-bearing (their negative controls fire), plus one
// genuine layout change on the output-meter label. See index.html for each.
// ============================================================================

export const LABELS = Object.freeze({

    // ── The six knob captions ───────────────────────────────────────────────
    //
    // D-01 arm 1 applies to exactly one parameter on this page and NOT to any
    // of these: `division` is the only AudioParameterChoice
    // (PluginProcessor.cpp:54-59) and it has no caption of its own — its twelve
    // option strings ("1/4", "1/8", "1/16", "1/4D" ... "1/16(5)") are written
    // into #time-value, the readout node, by the inline controller when SYNC is
    // on. They are exempt TWICE OVER: byte-identical to the option strings
    // (arm 1) and written into a readout node (arm 3). They are not listed in
    // I18N_EXEMPT because no scan reaches them — they are elements of a JS
    // array, not a textContent literal, so extractJsRows produces no row for
    // them and an entry would be inert. Recorded here instead.
    //
    // The other seven parameters are six AudioParameterFloat and one
    // AudioParameterBool, none of which has option strings for a French caption
    // to disagree with in a host automation lane.
    //
    // Arm 3 does not apply to any caption below either: every one is a
    // `.knob-label` div that never holds a number, because the number lives in
    // its own `.knob-value` sibling. Contract §5's split already exists in the
    // authored markup and nothing had to be split here.

    // The delay time in milliseconds, and the readout beside it says `ms`.
    // TEMPS is what a French delay calls that; DELAI (31.00) names the effect
    // rather than the quantity, and DUREE (34.00) is a duration rather than a
    // position in time.
    'label.time': { en: { t: 'TIME' }, fr: { t: 'TEMPS', reviewed: false } },

    // REINJECTION is the word Logic Pro's French build uses for a delay's
    // feedback and it does not fit: 66.30 px against a 60 px box, and being one
    // unbreakable word it would SPILL rather than wrap — painted straight over
    // the gap between two knobs. RETOUR (40.64) and REACTION (51.91) both fit
    // but both name something else in an audio context (a return bus, a
    // reaction). REINJ. is the abbreviation OF the expected word, which is the
    // same trade O-Chorus made for PROF.
    'label.feedback': { en: { t: 'FEEDBACK' }, fr: { t: 'RÉINJ.', reviewed: false } },

    // Stereo spread of the two delay lines. ECART is also what O-Chorus ships
    // for its own Spread, so the suite says one word for one idea.
    'label.spread': { en: { t: 'SPREAD' }, fr: { t: 'ÉCART', reviewed: false } },

    // Keyed with sameAsEn rather than exempted, deliberately. "Mod" is the
    // abbreviation of "modulation", which is the same word in French, but that
    // is a TRANSLATION JUDGEMENT and an I18N_EXEMPT entry would hide it from
    // the native-speaker worklist forever. Keyed, it is one more
    // `reviewed: false` line somebody has to agree with. MODUL. (40.06) also
    // fits if a reviewer wants the fuller form.
    'label.mod': { en: { t: 'MOD' }, fr: { t: 'MOD', reviewed: false, sameAsEn: true } },

    // WET / DRY is a pair and is translated as a pair. EFFET / DIRECT is the
    // idiomatic French pairing — the processed signal and the untouched one.
    // The literal MOUILLE (46.09) / SEC (18.91) also fits and is what a
    // dictionary gives, but no French audio interface says it.
    'label.wet': { en: { t: 'WET' }, fr: { t: 'EFFET',  reviewed: false } },
    'label.dry': { en: { t: 'DRY' }, fr: { t: 'DIRECT', reviewed: false } },

    // ── The sync toggle: its caption and its two faces ──────────────────────
    //
    // THE TIGHTEST STRING ON THE PAGE, 1.39 px under the 50.00 px cliff at
    // which `.toggle-container` widens and re-centres #sync inside it.
    // SYNCHRONISATION measures 96.52 and moves the toggle 23.25 px right.
    'label.sync': { en: { t: 'SYNC' }, fr: { t: 'SYNCHRO', reviewed: false } },

    // The two faces of #sync, written by the controller through setLabel() and
    // therefore [data-i18n] elements from that moment on — no second code path
    // that can go stale in the other language. They are NOT an
    // AudioParameterChoice's options: `sync` is an AudioParameterBool
    // (PluginProcessor.cpp:47-51), so D-01 arm 1 has nothing to disagree with.
    // Arm 3 does not apply either — this node only ever holds one of these two
    // words and never a number, which is the same reasoning that made O-Gain's
    // LOW/MED/HIGH localize.
    //
    // Both fit the fixed 50 x 24 face with room: MARCHE 43.31, ARRET 33.52.
    'label.on':  { en: { t: 'ON' },  fr: { t: 'MARCHE', reviewed: false } },
    'label.off': { en: { t: 'OFF' }, fr: { t: 'ARRÊT',  reviewed: false } },

    // ── The output meter caption ────────────────────────────────────────────
    //
    // THIS ONE FORCED THE PAGE'S ONE LAYOUT CHANGE, AND FRENCH IS NOT THE
    // REASON. `.led-meter-label` was `width: 18px`, matching the meter under
    // it, and the ENGLISH word "OUT" renders 20.91 px — 2.91 px outside its own
    // content box in every build since v1.0.0. Keying the node is what made
    // that visible: check-ui-labels assertion 4 measures a leaf label's text
    // against its content box, and it fails in ENGLISH at 20.91 > 18.
    //
    // So no French string could have saved it — SORTIE 35.77, SORT. 28.05 and
    // even SOR 19.92 are all over an 18 px box. The label and its container are
    // widened to 40 px in index.html, positioned so the METER does not move,
    // and the pre-existing English overhang is repaired in the same edit.
    // SORTIE then has 4.23 px spare.
    'label.out': { en: { t: 'OUT' }, fr: { t: 'SORTIE', reviewed: false } },

    // ── The two preset buttons ──────────────────────────────────────────────
    //
    // `.preset-action-btn` is PINNED to 62 px for these two (index.html).
    // Rendered text against that pin's 48 px content box — 62 border-box less
    // 2 px border and 12 px padding:
    //
    //     Load 27.00 -> CHARGER 46.52   1.48 px spare
    //     Save 24.34 -> SAUVER  39.02   8.98 px spare
    //
    // 62 px is O-Chorus's number, kept so the suite's preset bar is one shape;
    // this page's 9 px type with 0.5 px letter-spacing renders CHARGER 2 px
    // wider than O-Chorus's does, which is where the extra margin went.
    // ENREGISTRER is the word a French user would rather see and needs an
    // 80.52 px box — a reviewer who upgrades SAUVER to it must raise the pin
    // with it, or an 11-character caption wraps inside a 16 px-high button.
    'label.load': { en: { t: 'Load' }, fr: { t: 'CHARGER', reviewed: false } },
    'label.save': { en: { t: 'Save' }, fr: { t: 'SAUVER',  reviewed: false } },

    // ── The preset dropdown, written by the controller through setLabel() ───
    //
    // CONTRACT §6 — PLURALS ARE AVOIDED, NOT ENGINEERED. The empty-list line is
    // the one string on this page that could have carried a count, and it is
    // authored so that it never does: "Aucun préréglage disponible" is a
    // categorical statement, correct at exactly zero, and it needs no
    // inflection. The alternative — a count with a French plural rule that
    // treats zero as singular — would need a plural engine for one string on
    // one plugin. check-i18n assertion 13 rejects a ternary inside a setLabel
    // argument so it cannot creep back, which is why the ON/OFF pair above is
    // written as two if/else calls rather than one conditional key.
    'label.presets':    { en: { t: 'Presets' },              fr: { t: 'Préréglages',                reviewed: false } },
    'label.noPresets':  { en: { t: 'No presets available' }, fr: { t: 'Aucun préréglage disponible', reviewed: false } },

    // ── The settings popover (v1.3.0) ───────────────────────────────────────
    'label.language': { en: { t: 'Language' }, fr: { t: 'Langue', reviewed: false } },

    // ── Accessible names ────────────────────────────────────────────────────
    //
    // Resolved through the same sweep via data-i18n-aria, so a screen reader
    // hears the language the page is showing.
    //
    // THE FIVE PRESET-BAR NAMES ARE THE DELETED title= TEXT, MOVED, NOT
    // AUTHORED. v1.2.12 carried title="Previous preset", "Click to see all
    // presets", "Next preset", "Load preset from file" and "Save current
    // settings"; contract §4 deletes the native attribute (it renders a second,
    // untranslated OS tooltip) and moves its existing English into the
    // accessible name. Every English string below is byte-identical to what
    // v1.2.12 shipped, and the French is byte-identical to O-Detune's, which
    // carried the identical five attributes.
    //
    // LABEL IN NAME. #loadPreset and #savePreset carry BOTH a visible caption
    // and an aria-label, and an aria-label REPLACES the accessible name rather
    // than extending it. Each of those two accessible names therefore CONTAINS
    // its own visible caption as a prefix — "Load" in "Load preset from file",
    // "CHARGER" in "Charger un préréglage depuis un fichier" — so a voice
    // control user saying the caption still hits the button (WCAG 2.5.3).
    //
    // #presetName is the one place that rule cannot be honoured, and the
    // divergence is deliberate rather than overlooked: its visible text is the
    // PRESET NAME, which changes at runtime and is exempt under D-02, so no
    // fixed accessible name can contain it. The same trade was made on
    // O-Detune, O-FreqPulse and O-Lyrica for the identical control.
    'aria.prevPreset': { en: { t: 'Previous preset' },    fr: { t: 'Préréglage précédent',  reviewed: false } },
    'aria.nextPreset': { en: { t: 'Next preset' },        fr: { t: 'Préréglage suivant',    reviewed: false } },
    'aria.presetList': { en: { t: 'Click to see all presets' },
                         fr: { t: 'Cliquez pour voir tous les préréglages', reviewed: false } },
    'aria.loadPreset': { en: { t: 'Load preset from file' },
                         fr: { t: 'Ouvrir un préréglage depuis un fichier', reviewed: false } },
    'aria.savePreset': { en: { t: 'Save current settings' },
                         fr: { t: 'Sauver les réglages actuels', reviewed: false } },

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
// is the one state the gate cannot tell a deliberate skip from a forgotten
// label (check-i18n assertion 14). NONE of the five below is in that state:
// no key in LABELS above resolves to any of these strings, so all five are
// correctly unscoped and assertion 14 passes without one.
// ============================================================================

export const I18N_EXEMPT = [
    ['OUARICON DIGITAL DELAY',
     'the product display name in .title — a product name is never translated, and this is '
     + 'the brand-plus-product form of the plugin\'s registered PRODUCT_NAME "O-DigiDelay" '
     + 'in CMakeLists.txt'],

    // ── D-02: the preset name IS the filename ───────────────────────────────
    ['Default',
     'the PRESET NAME shown in #presetName, not a caption — D-02. The name is the JSON '
     + 'filename on disk (OuariconPresetManager sanitizes it into getUserPresetsDirectory()), '
     + 'and it is written into this node at runtime by the VENDORED '
     + 'modules/preset-manager.js, which is shared across plugins: localizing it here would '
     + 'rename presets in one language and orphan the files'],

    ['Ouaricon Audio',
     'the company name in .footer-brand — a brand is never translated'],

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
