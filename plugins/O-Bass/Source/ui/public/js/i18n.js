/*
   This file is part of O-Bass, an Ouaricon Audio plugin.
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
// i18n.js — O-Bass page labels, English + French (v1.4.0)
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
// v1.3.3 carried no data-tip and no data-tooltip anywhere on the page — only
// five native title= attributes on the preset bar, which contract §4 DELETES
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
// ── THE FRAME IS 420 x 320. THE NARROWEST IN BATCH K2 ───────────────────────
//
// The three plugins ahead of this one in the batch were SHORT and wide —
// O-Chorus 700x125, O-DigiDelay 700x196, O-AnalogEQ 920x220 — so their budget
// was vertical. This one is the first that is tight HORIZONTALLY, and French
// runs 15-20% longer than English on average, so the pressure lands squarely
// on width.
//
// ── THIS PAGE HAS NO FIXED-WIDTH TEXT BOX ANYWHERE. THAT IS THE MECHANISM ───
//
// O-Chorus has a shrink-to-fit caption inside a FIXED container. O-DigiDelay
// pins .knob-label to a hard 60 px. O-AnalogEQ pins .band-label to 85 px and
// its toggle to 75 x 22. Every one of those has at least one language-invariant
// rectangle to measure against.
//
// O-Bass, before this commit, had NONE. Every row is `justify-content: center`
// over shrink-to-fit children:
//
//     .preset-row     .preset-bar (nav + name + LOAD + SAVE)  |  .limit-indicator
//     .controls-row   three .knob-container, each max(label, 65 px knob)
//     .meter-row      .meter-label  |  180 px meter
//
// so EVERY caption's width feeds straight back into where its neighbours are
// painted. The consequence is that on this page the dominant failure is not
// French GROWING — it is French being a DIFFERENT WIDTH AT ALL. Half the
// French below is NARROWER than its English, and an unpinned shrink re-centres
// the row exactly as loudly as a spill would.
//
// ── THE THREE CLIFFS, MEASURED ──────────────────────────────────────────────
//
//   A  PUSH AT 65.00 px — .knob-label.
//      A .knob-container is `flex-direction: column; align-items: center` over
//      a 65 px knob and a 60 px min-width readout, so its width is
//      max(labelWidth, 65). Under 65.00 the label is invisible to the layout;
//      past it the label DRIVES the container and shoves both siblings.
//      FREQUENCY (85.02) and ENHANCE (67.02) are already past it in ENGLISH;
//      OUTPUT (56.02) is the one caption still under it, which is what makes
//      the cliff observable — and it is where the plant below sits.
//      Caught by assertion 7. Assertion 4 is blind: nothing ever clips, the
//      container simply grows.
//
//   B  ZERO-SLACK PUSH — .preset-row. THIS IS THIS PAGE'S REAL RISK.
//      The row measures 374.16 px inside a 374.00 px content box. It is
//      already 0.16 px over, in ENGLISH, at v1.3.3. There is NO slack at all:
//      every px a French caption adds to LOAD, SAVE or LIMIT is a px the row
//      takes out of the 20 px page margin. CHARGER (51.30) + SAUVER (43.02)
//      alone would add 37.71 px and stand the row 1 px off the frame border.
//      That is why the French on those three controls is abbreviated and the
//      buttons are PINNED rather than widened.
//      Caught by assertion 7. Assertion 4 is blind: a shrink-to-fit button
//      always fits its own grown box.
//
//   C  RE-CENTRE — .meter-row and .preset-row both.
//      Because both rows are centred, a caption that changes width by d moves
//      its NEIGHBOUR by d/2 and itself by d/2 in the opposite direction. There
//      is no threshold to cross: the push starts at the first tenth of a pixel.
//      A clip check cannot see it in either direction, and the SHRINK half it
//      cannot see at all.
//
// Every French string below was chosen against a MEASURED width, rendered in
// the real node with its own letter-spacing, font-weight and text-transform
// (none of which appears in getComputedStyle().font).
// ============================================================================

export const LABELS = Object.freeze({

    // ── The three knob captions ─────────────────────────────────────────────
    //
    // All three are AudioParameterFloat-backed (crossover_freq, enhance,
    // output) and all three sit in a `.knob-label` div that NEVER holds a
    // number — the value lives in a separate `.knob-value` sibling, so
    // contract §5's split already existed in the authored markup and nothing
    // had to be split. D-01 arm 3 therefore does not fire, and arm 1 cannot:
    // the page's only AudioParameterChoice is `latency_mode`, whose two
    // options ("Low Latency", "High Fidelity") have no control on this page at
    // all.
    //
    // Measured at 12 px Garamond, weight 600, letter-spacing 1 px, uppercase:
    //
    //   FREQUENCY  85.02 -> FREQUENCE  84.34    pin 86,  1.66 spare
    //   ENHANCE    67.02 -> RENFORT    65.25    pin 68,  2.75 spare
    //   OUTPUT     56.02 -> SORTIE     50.94    no pin, 14.06 under cliff A
    //
    // TWO OF THREE SHRINK. A clip-only check would have certified this page.
    //
    // FREQUENCE is the direct translation and it fits. COUPURE (66.34) — the
    // crossover point, which is what the parameter actually is — also fits and
    // is arguably the better word for a crossover control; it was not taken
    // because the visible English says FREQUENCY, not CROSSOVER, and this
    // commit translates the page rather than re-naming its controls.
    //
    // RENFORT ("reinforcement") is the bass-enhancement amount. AMPLEUR (67.67)
    // and REHAUSS. (67.69) both clear the 68 px pin by less than half a pixel,
    // which is a fit by rounding rather than by fit; INTENSITE (74.38) and
    // EPAISSEUR (75.47) are past it outright and would each need the pin
    // raised, which on this page means widening the whole control row.
    // RELIEF (50.69) and ACCENT (56.67) are the reviewer's roomier levers.
    //
    // SORTIE needs no pin and that is a measured fact, not an oversight: its
    // container is floored at 65 px by the knob beneath it and BOTH captions
    // are under that floor, so the container is 65.00 px in either language.
    // A `width` here would have a negative control that PASSES, which by the
    // batch rule is decoration.
    'label.frequency': { en: { t: 'Frequency' }, fr: { t: 'Fréquence', reviewed: false } },
    'label.enhance':   { en: { t: 'Enhance' },   fr: { t: 'Renfort',   reviewed: false } },
    'label.output':    { en: { t: 'Output' },    fr: { t: 'Sortie',    reviewed: false } },

    // ── The limiter indicator caption ───────────────────────────────────────
    //
    // Not a parameter at all: `limitIndicator` is a meter value the processor
    // publishes for the LED beside this word (PluginProcessor.h), so there is
    // no automation lane for a French caption to disagree with.
    //
    // 11 px, letter-spacing 1 px, uppercase, against a 36 px pin:
    //     LIMIT 35.55 -> LIM. 26.92    9.08 spare
    //
    // LIMITE (43.27) is the word and it does not fit: it is 7.72 px wider than
    // the English, and cliff B has 0.00 px to give. ECRET. (42.77, écrêtage)
    // is wider still. LIM. is the abbreviation French-market limiters carry.
    'label.limit': { en: { t: 'Limit' }, fr: { t: 'Lim.', reviewed: false } },

    // ── The output-meter caption ────────────────────────────────────────────
    //
    // KEYED sameAsEn RATHER THAN EXEMPTED, deliberately, for the reason
    // O-AnalogEQ keyed LMF/HMF and O-DigiDelay keyed MOD: an exemption would
    // hide a translation JUDGEMENT from the native-speaker worklist forever,
    // whereas a sameAsEn key is one more `reviewed: false` line somebody has
    // to actively agree with. This is the judgement, stated so it can be
    // overruled:
    //
    // 10 px, letter-spacing 1 px, uppercase. The caption's box is 23.56 px and
    // the row is centred, so cliff C applies with no threshold — every px of
    // width change moves the 180 px meter by half a px.
    //     OUT    23.56
    //     SORTIE 40.41   +16.85 -> the meter moves 8.42 px right, in BOTH
    //                    languages, forever
    //     SORT.  31.72   +8.16  -> the meter moves 4.08 px right
    //     SOR.   25.95   +2.39
    //     SOR    22.45   fits, and is not a word anybody writes
    //
    // Every French form that is a WORD costs an English geometry change on a
    // control that did not need one, which is the French-caused layout change
    // this batch is counting. OUT is what French-market meters are silk-
    // screened with — the same argument that kept O-AnalogEQ's LF/LMF/HMF/HF
    // verbatim — so it is kept and the cost of overruling it is written down
    // above rather than hidden.
    //
    // NOTE the tension a reviewer must settle: label.output above DOES become
    // SORTIE, because its container has 14 px of slack and this one has none.
    // The same English word gets two different answers on one page, decided by
    // geometry. That is a legitimate thing to disagree with.
    'label.out': { en: { t: 'Out' }, fr: { t: 'Out', reviewed: false, sameAsEn: true } },

    // ── The two preset buttons ──────────────────────────────────────────────
    //
    // #loadPreset and #savePreset are PINNED in index.html to 49 px and 46 px —
    // each its own English border box (47.78 / 44.83) rounded up with a pixel
    // of headroom, NOT to the 62 px this batch used on O-Chorus, O-DigiDelay
    // and O-AnalogEQ. 62 px does not transfer to a 420 px frame: it would add
    // 31.39 px to a preset row that is already 0.16 px over its content box.
    //
    // 10 px, letter-spacing 0.5 px, uppercase, against the pins' 31 / 28 px
    // content boxes:
    //     LOAD 29.78 -> LIRE 24.22    6.78 spare
    //     SAVE 26.83 -> ENR  21.50    6.50 spare
    //
    // BOTH SHRINK, and that is what makes the pins load-bearing rather than
    // decorative: unpinned, the two buttons CONTRACT in French, the preset bar
    // contracts with them and the whole centred row re-centres.
    //
    // WHY NOT THE FULL WORDS. CHARGER is 51.30 and SAUVER 43.02 — together
    // +37.71 px on a row with zero slack, which would leave it standing 1 px
    // off the frame border. OUVRIR (41.34), CHARG. (40.52), ENREG. (38.84) and
    // SAUV. (30.94) are all past their pins too. Every fuller form on this
    // control costs a widened frame, and that is a design decision rather than
    // a translation one.
    //
    // ENR CARRIES NO PERIOD, AND THAT IS THE POINT. aria.savePreset's French
    // is "Enregistrer les réglages actuels"; label-in-name (WCAG 2.5.3)
    // matches case-insensitively, and "enr" IS a substring of "enregistrer"
    // while "enr." is NOT. Dropping the period is what keeps a voice-control
    // user's "ENR" hitting the button. ENR. (24.50) would fit the pin just as
    // well and would break the rule silently — the same rule O-AnalogEQ found
    // broken on O-DigiDelay's CHARGER/Ouvrir pair.
    //
    // LIRE is a whole word, so aria.loadPreset simply begins with it.
    'label.load': { en: { t: 'Load' }, fr: { t: 'Lire', reviewed: false } },
    'label.save': { en: { t: 'Save' }, fr: { t: 'Enr',  reviewed: false } },

    // ── The preset dropdown, written through setLabel() ─────────────────────
    //
    // CONTRACT §6 — PLURALS ARE AVOIDED, NOT ENGINEERED. label.noPresets is the
    // one string on this page that could have carried a count, and it is
    // authored so that it never does: "Aucun préréglage disponible" is
    // categorical, correct at exactly zero, and needs no inflection — French
    // treats zero as singular and English does not, and a plural engine for one
    // string on one plugin is not a trade worth making. check-i18n assertion 13
    // rejects a ternary inside a setLabel argument so a count cannot creep back.
    //
    // Both English strings are byte-identical to what v1.3.3 wrote at
    // index.html:606 and :614. No prose was invented; it was moved into the
    // table. The dropdown is `left: 0; right: 0` of the 280.61 px preset bar,
    // so both have ~240 px of content box and neither is anywhere near a cliff.
    'label.noPresets': { en: { t: 'No presets available' },
                         fr: { t: 'Aucun préréglage disponible', reviewed: false } },
    'label.factory':   { en: { t: 'Factory' }, fr: { t: 'Usine', reviewed: false } },

    // ── The settings popover (v1.4.0) ───────────────────────────────────────
    'label.language': { en: { t: 'Language' }, fr: { t: 'Langue', reviewed: false } },

    // ── Image alternative text ──────────────────────────────────────────────
    //
    // Two decorative <img> layers, keyed through data-i18n-alt rather than
    // emptied: check-i18n assertion 11 requires every alt carrying PROSE to be
    // keyed or exempt, and these two carry prose. Emptying them to alt="" would
    // also satisfy the assertion and would arguably be the better a11y answer
    // for a pure decoration — but it deletes an authored string for a reason
    // that has nothing to do with localization, so the strings are translated
    // and left in place. The keys and the French are byte-identical to
    // O-IntonationPad's, which carries the identical two images.
    'alt.background': { en: { t: 'Background' }, fr: { t: 'Arrière-plan',    reviewed: false } },
    'alt.botanical':  { en: { t: 'Botanical' },  fr: { t: 'Motif botanique', reviewed: false } },

    // ── Accessible names ────────────────────────────────────────────────────
    //
    // Resolved through the same sweep via data-i18n-aria, so a screen reader
    // hears the language the page is showing.
    //
    // THE FIVE PRESET-BAR NAMES ARE THE DELETED title= TEXT, MOVED, NOT
    // AUTHORED. v1.3.3 carried title="Previous preset", "Click to see all
    // presets", "Next preset", "Load preset from file" and "Save current
    // settings"; contract §4 deletes the native attribute — on an element that
    // also has a data-tip it renders a second, untranslated OS tooltip, and
    // leaving it on an element that has none is still an untranslated string —
    // and moves its existing English into the accessible name. Every English
    // string below is byte-identical to what v1.3.3 shipped, INCLUDING "Click
    // to see all presets", which is this plugin's own wording and NOT the
    // "Click to browse presets" four of its siblings carry.
    //
    // LABEL IN NAME. #savePreset and #loadPreset carry BOTH a visible caption
    // and an aria-label, and an aria-label REPLACES the accessible name rather
    // than extending it. Each of those two names therefore CONTAINS its own
    // visible caption — "Load" in "Load preset from file", "LIRE" in "Lire un
    // préréglage depuis un fichier", "ENR" in "Enregistrer les réglages
    // actuels" — so a voice-control user saying the caption still hits the
    // button (WCAG 2.5.3, which matches case-insensitively).
    //
    // #presetName is the one place the rule cannot be honoured, and the
    // divergence is deliberate rather than overlooked: its visible text is the
    // PRESET NAME, which changes at runtime and is exempt under D-02, so no
    // fixed accessible name can contain it. The same trade was made on
    // O-AnalogEQ, O-Detune, O-DigiDelay, O-FreqPulse and O-Lyrica for the
    // identical control.
    'aria.prevPreset': { en: { t: 'Previous preset' },
                         fr: { t: 'Préréglage précédent', reviewed: false } },
    'aria.nextPreset': { en: { t: 'Next preset' },
                         fr: { t: 'Préréglage suivant',   reviewed: false } },
    'aria.presetList': { en: { t: 'Click to see all presets' },
                         fr: { t: 'Cliquez pour voir tous les préréglages', reviewed: false } },
    'aria.loadPreset': { en: { t: 'Load preset from file' },
                         fr: { t: 'Lire un préréglage depuis un fichier', reviewed: false } },
    'aria.savePreset': { en: { t: 'Save current settings' },
                         fr: { t: 'Enregistrer les réglages actuels', reviewed: false } },

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
// NONE of the four entries below is in that state — no key in LABELS above
// resolves to any of these strings — so all four are correctly unscoped and
// assertion 14 passes without one. Checked rather than assumed: the closest
// call is "Default", which is also the word the preset manager writes into
// #presetName at runtime, and it appears in no French caption on this page.
//
// THE PAGE'S THREE READOUT NODES ARE NOT LISTED HERE, and their absence is
// correct rather than an omission. #frequencyValue, #enhanceValue and
// #outputValue are written from `Math.round(freq) + ' Hz'`, `... + '%'` and
// `db.toFixed(1) + ' dB'` — composed expressions, so extractJsRows produces no
// LABEL row and assertion 12 reads only LABEL rows. They are exempt three times
// over regardless: a number (D-01 arm 2), a unit (D-03), written into a readout
// node (arm 3). An I18N_EXEMPT entry for them would be inert.
// ============================================================================

export const I18N_EXEMPT = [
    // ── The product display name ────────────────────────────────────────────
    ['Ouaricon Bass',
     'the product display name in h1.title, and the same string in the document title element '
     + '— a product name is '
     + 'never translated, and this is the brand-plus-product form of the plugin\'s registered '
     + 'PRODUCT_NAME "O-Bass" in CMakeLists.txt'],

    // ── D-02: the preset name IS the filename ───────────────────────────────
    ['Default',
     'the PRESET NAME shown in #presetName, not a caption — D-02. The name is the JSON '
     + 'filename on disk (OuariconPresetManager sanitizes it into getUserPresetsDirectory()), '
     + 'and it is written into this node at runtime by the VENDORED '
     + 'modules/preset-manager.js, which is shared across plugins: localizing it here would '
     + 'rename presets in one language and orphan the files'],

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
