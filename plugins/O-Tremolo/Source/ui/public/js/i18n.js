/*
   This file is part of O-Tremolo, an Ouaricon Audio plugin.
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
// i18n.js — O-Tremolo page labels, English + French (v1.7.0)
//
// An ES module that EXPORTS ONLY. It must never self-execute: a bare top-level
// statement here throws out of module evaluation and takes every later
// initializer on the page with it (pattern_module_toplevel_init_tdz). This
// plugin's controller is ONE inline <script type="module"> in index.html, so
// that failure would take the WHOLE UI — both knobs, the preset bar, the
// waveform canvas — rather than a panel of it. check-i18n assertion 7 enforces
// it.
//
// FILENAME: no hyphen. juce_add_binary_data STRIPS hyphens rather than
// converting them to underscores, so a second file named i18n-fr.js would have
// to be reached as the symbol i18nfr_js (critical_binary_data_strips_hyphens).
// One combined file for both languages sidesteps the question entirely.
//
// ── THIS PLUGIN HAS NO HOVER-HELP, AND THIS COMMIT DOES NOT GIVE IT ANY ─────
//
// v1.6.0 carried no data-tip and no data-tooltip anywhere on the page — only
// five native title= attributes on the preset bar, which contract §4 DELETES
// rather than localizes, moving their existing English into data-i18n-aria. No
// hover-help prose is INVENTED here: authoring it is Stage M's job. I18N is
// therefore empty and TIP_BINDINGS is empty, which is this plugin's correct
// state rather than a gap. check-i18n assertion 2 reports it as "0 tip(s)
// bound", and that emptiness is admissible only BECAUSE no I18N entry carries
// a body — an emptied TIP_BINDINGS over a bodied table would be orphaned copy.
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
// One string per entry, no body: a label is not a tooltip. The authored English
// stays in the markup as the fallback that renders if applyI18n never runs, and
// every en entry below is byte-for-byte what v1.6.0 shipped.
//
// ── THE FOUR GEOMETRY BUDGETS ON THIS PAGE ──────────────────────────────────
//
// Every French string below was chosen against a MEASURED box in THIS page's
// own elements, never against a sibling plugin's number. O-SimpleReverb and
// O-Bass report the same two words 8.24 px apart at the same declared
// font-size, so a borrowed absolute is a wrong number that reads like a right
// one.
//
//   A. THE PRESET BAR is right-anchored by `justify-content: space-between` on
//      .header, and #loadPreset / #savePreset are shrink-to-fit. Unpinned, LIRE
//      and ENR contract those two buttons by 5.56 and 5.33 px, .preset-bar
//      contracts with them, and space-between slides #prevPreset, #presetName
//      and #nextPreset 10.89 px to the RIGHT. Both buttons are therefore PINNED
//      in index.html to their own English border box rounded up — 48 px and
//      45 px against 47.78 and 44.83 — and the two pins together add 0.39 px
//      to a header with 114.06 px of slack, which moves nothing.
//
//        content box 30.00   LOAD 29.78 -> LIRE 24.22
//        content box 27.00   SAVE 26.83 -> ENR  21.50
//
//   B. THE TWO KNOB COLUMNS are 60 px wide because the knob is, and
//      .knob-container is shrink-to-fit, so a caption WIDER than 60 px grows
//      the column. PROFONDEUR is 64.05 px: the depth column widens 4.00 px and
//      its box slides 2.00 px left as .knobs-row re-centres it. #depthKnob
//      itself does NOT move — the knob is centred in the column and the column
//      in the row, so the two centrings cancel — and the pin is required on
//      the FULL delta rather than on dx: dw = 4.00 px is eight times the
//      0.5 px tolerance. .knob-container is therefore pinned to 60 px, a no-op
//      in English (it already measures exactly 60.00), and the French caption
//      overhangs the column symmetrically, centred on the knob exactly as the
//      English one is.
//
//        VITESSE    38.52   inside the column
//        PROFONDEUR 64.05   overhangs 2.03 px each side, centred
//
//   C. THE WAVEFORM SELECT needs NO pin, and that was measured rather than
//      assumed. .waveform-section is shrink-to-fit over a stretched caption and
//      a `width: 100%` <select>, and the SELECT's own intrinsic width (88 px)
//      is the larger of the two — so any caption at or below 88 px leaves the
//      section, the select and everything centred beside them exactly where
//      they are. FORME D'ONDE is 106.34 px and would grow all three; ONDE is
//      38.67 px and moves nothing. The budget is what chose the word.
//
//   D. THE SMOOTHING ROW has the same shape with a much larger budget: the
//      range input's intrinsic 129 px sets .slider-container, and LISSAGE is
//      59.03 px. No pin, and a comfortable margin rather than a tight one.
//
// ALL FRENCH IS MACHINE-DRAFTED AND FLAGGED `reviewed: false`.
// ============================================================================

export const LABELS = Object.freeze({

    // ── The two preset buttons ──────────────────────────────────────────────
    //
    // See budget A above for the pins. WHY NOT THE FULL WORDS: CHARGER is
    // 51.30 px and SAUVER 43.02 px against 30 px and 27 px content boxes —
    // both spill, and both would have to grow the button rather than fit it.
    //
    // ENR CARRIES NO PERIOD, AND THAT IS THE POINT. aria.savePreset's French is
    // "Enregistrer les réglages actuels"; label-in-name (WCAG 2.5.3) matches
    // case-insensitively, and "enr" IS a substring of "enregistrer" while
    // "enr." is NOT. Dropping the period is what keeps a voice-control user's
    // "ENR" hitting the button. ENR. would fit the pin just as well and would
    // break the rule silently. LIRE is a whole word, so aria.loadPreset simply
    // begins with it.
    'label.load':      { en: { t: 'Load' },       fr: { t: 'Lire',       reviewed: false } },
    'label.save':      { en: { t: 'Save' },       fr: { t: 'Enr',        reviewed: false } },

    // ── The two sync toggles ────────────────────────────────────────────────
    //
    // #panButton and #tempoButton are `width: 70px` with `overflow: hidden`, so
    // the risk here is not width but a THIRD LINE: 42 px of content box already
    // holds each English caption on two lines, and a caption that gained a line
    // would grow the button 13 px, push its sibling down and take the whole
    // knob column with it.
    //
    // Measured in the button itself: PAN SYNC 28.69 -> SYNC PAN 28.69, and
    // TEMPO SYNC 36.41 -> SYNC TEMPO 36.41. Both stay on two lines at exactly
    // the same line-box width, because French reorders the two words rather
    // than lengthening either. PANORAMIQUE (78.48, one line) was measured and
    // rejected: it overruns the 42 px content box outright.
    'label.panSync':   { en: { t: 'Pan Sync' },   fr: { t: 'Sync Pan',   reviewed: false } },
    'label.tempoSync': { en: { t: 'Tempo Sync' }, fr: { t: 'Sync Tempo', reviewed: false } },

    // ── The two knob captions ───────────────────────────────────────────────
    //
    // See budget B. PROFONDEUR is the standard French for a modulation DEPTH
    // and it is kept rather than traded for a shorter near-synonym; AMPLEUR
    // (49.84) and INTENSITÉ (48.39) both fit the unpinned column and both say
    // something slightly different. The 60 px pin is what makes keeping the
    // right word free.
    'label.speed':     { en: { t: 'Speed' },      fr: { t: 'Vitesse',    reviewed: false } },
    'label.depth':     { en: { t: 'Depth' },      fr: { t: 'Profondeur', reviewed: false } },

    // ── The two section headings ────────────────────────────────────────────
    //
    // ONDE, not FORME D'ONDE — see budget C. The word names exactly what the
    // select picks (Sine, Triangle, Phasor, Noise, Square, Pulse are all
    // ondes), and it is the term a French-language modular front panel uses.
    // The 88 px ceiling is a hard one: FORME D'ONDE at 106.34 px grows the
    // select it sits above.
    'label.waveform':  { en: { t: 'Waveform' },   fr: { t: 'Onde',       reviewed: false } },
    'label.smoothing': { en: { t: 'Smoothing' },  fr: { t: 'Lissage',    reviewed: false } },

    // ── The preset dropdown, written by setLabel() at runtime ───────────────
    //
    // These three are the only JS-written prose on this page. They were English
    // literals assigned straight to textContent in v1.6.0; each is now a
    // setLabel() call, so the element becomes a [data-i18n] element the moment
    // it is written and the language sweep owns it from then on. That is the
    // whole point of contract §3 — a string written outside the table is
    // stranded in the previous language the instant the selector fires.
    //
    // NO PLURAL ENGINE (contract §6). 'No presets available' has no count in
    // it, in either language, so there is no inflection to get wrong at 0, 1
    // and n. The dropdown row it renders into is 256.61 px of content box and
    // the French is well inside it, so the panel does not gain a line.
    'label.factory':   { en: { t: 'Factory' },    fr: { t: 'Usine',       reviewed: false } },
    'label.user':      { en: { t: 'User' },       fr: { t: 'Utilisateur', reviewed: false } },
    'label.noPresets': { en: { t: 'No presets available' },
                         fr: { t: 'Aucun préréglage disponible', reviewed: false } },

    // ── The settings popover (v1.7.0) ───────────────────────────────────────
    //
    // LANGUAGE 63.55 -> LANGUE 47.11, MEASURED IN THIS PAGE'S OWN
    // .settings-label. That happens to be byte-for-byte O-SimpleReverb's pair
    // and 8.24 px off O-Bass's for the same two words at the same declared
    // font-size — which is exactly why it was measured here rather than
    // borrowed. A coincidence that a sibling's number would have been right is
    // not a reason to have used it.
    //
    // It SHRINKS, which is why .settings-popover carries a hard width: unpinned
    // the panel is shrink-to-fit over max(caption, select), so it would measure
    // 63.55 in English and 62.00 in French and assertion 7 would report the
    // panel, its row and the select as moved.
    'label.language':  { en: { t: 'Language' },   fr: { t: 'Langue',     reviewed: false } },

    // ── Image alternative text ──────────────────────────────────────────────
    //
    // Two decorative <img> layers, keyed through data-i18n-alt rather than
    // emptied: check-i18n assertion 11 requires every alt carrying PROSE to be
    // keyed or exempt, and these two carry prose. Emptying them to alt="" would
    // also satisfy the assertion and is arguably the better a11y answer for a
    // pure decoration — but it deletes an authored string for a reason that has
    // nothing to do with localization. The images here are img/paper.jpg and
    // img/carrot.png; the authored English is the same two words several
    // siblings carry, and the French matches theirs.
    'alt.background':  { en: { t: 'Background' }, fr: { t: 'Arrière-plan',    reviewed: false } },
    'alt.botanical':   { en: { t: 'Botanical' },  fr: { t: 'Motif botanique', reviewed: false } },

    // ── Accessible names ────────────────────────────────────────────────────
    //
    // Resolved through the same sweep via data-i18n-aria, so a screen reader
    // hears the language the page is showing.
    //
    // THE FIVE PRESET-BAR NAMES ARE THE DELETED title= TEXT, MOVED, NOT
    // AUTHORED. v1.6.0 carried title="Previous preset", "Click to see all
    // presets", "Next preset", "Load preset from file" and "Save current
    // settings"; contract §4 deletes the native attribute — it is a second,
    // untranslated OS tooltip either way — and moves its existing English into
    // the accessible name. Every English string below is byte-identical to what
    // v1.6.0 shipped. THIS PLUGIN'S WORDING IS ITS OWN: "Load preset from file"
    // and "Save current settings", not the shorter forms O-SimpleReverb
    // carries. Nothing was harmonised.
    //
    // #presetName DOES get an accessible name here, unlike O-SimpleReverb's
    // display span, because this page HAD a title= on it ("Click to see all
    // presets") and that is existing English to move rather than prose to
    // invent. Five titles in, five aria names out.
    //
    // LABEL IN NAME. #loadPreset and #savePreset carry BOTH a visible caption
    // and an aria-label, and an aria-label REPLACES the accessible name rather
    // than extending it. Each of those two names therefore CONTAINS its own
    // visible caption — "Load" in "Load preset from file", "LIRE" in "Lire un
    // préréglage depuis un fichier", "Save" in "Save current settings", "ENR"
    // in "Enregistrer les réglages actuels" — so a voice-control user saying
    // the caption still hits the button (WCAG 2.5.3, matched
    // case-insensitively).
    'aria.prevPreset': { en: { t: 'Previous preset' },
                         fr: { t: 'Préréglage précédent', reviewed: false } },
    'aria.nextPreset': { en: { t: 'Next preset' },
                         fr: { t: 'Préréglage suivant',   reviewed: false } },
    'aria.presetList': { en: { t: 'Click to see all presets' },
                         fr: { t: 'Cliquer pour voir tous les préréglages', reviewed: false } },
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
// NONE of the eleven entries below is in that state, and that was CHECKED
// rather than assumed. The keyed texts on this page are Load, Save, Pan Sync,
// Tempo Sync, Speed, Depth, Waveform, Smoothing, Language, plus Factory, User
// and "No presets available" written into the dropdown at runtime. No entry
// below collides with any of them, so all eleven are correctly UNSCOPED and
// assertion 14 passes without a scope.
//
// THE TWO READOUT NODES ARE NOT LISTED, and that is correct rather than an
// omission. #speedValue and #depthValue are written from composed expressions
// — `realValue.toFixed(1) + unit` and a musical-division NAME such as "1/8T" —
// so extractJsRows produces no LABEL row for either and an entry would be
// inert. Both are exempt three times over regardless: a number (D-01 arm 2), a
// unit (D-03), and written into a readout node (D-01 arm 3, which is exempt
// REGARDLESS of the backing parameter type). The synced readout is the
// interesting one: "1/8T" is a SYNC_DIVISION_PARAM choice string verbatim, so
// it is exempt under arm 1 as well — the page and the host automation lane must
// agree on what the division is called.
// ============================================================================

export const I18N_EXEMPT = [
    // ── The product name ────────────────────────────────────────────────────
    ['O-Tremolo',
     'the product name, in h1.title and in the document title element — a product name is '
     + 'never translated, and this string is the plugin\'s registered PRODUCT_NAME in '
     + 'CMakeLists.txt'],

    ['Ouaricon Audio',
     'the company name in .footer-brand — a company name is never translated. The span '
     + 'beside it (#versionLabel) is filled at runtime from getPluginVersion() and holds a '
     + 'version number, which is a readout (D-03)'],

    // ── D-02: the preset name IS the filename ───────────────────────────────
    ['Default',
     'the PRESET NAME shown in #presetName, not a caption — D-02. The name is the JSON '
     + 'filename on disk (OuariconPresetManager sanitizes it into getUserPresetsDirectory()), '
     + 'and it is overwritten at runtime from getCurrentPreset(): localizing it would rename '
     + 'presets in one language and orphan the files saved under the other'],

    // ── D-01 arm 1: the six waveform options are AudioParameterChoice strings ─
    //
    // BYTE-IDENTICAL is the test, and it holds for all six: PluginProcessor.cpp
    // line 96 declares
    //     juce::StringArray { "Sine", "Triangle", "Phasor", "Noise", "Square", "Pulse" }
    // and index.html's six <option> texts are those same six strings. The page
    // and the host automation lane must agree on what the waveform is called,
    // so translating the caption while the DAW's automation lane keeps the
    // English would put two names on one control.
    ['Sine',     'a WAVEFORM_PARAM AudioParameterChoice option, byte-identical — D-01 arm 1 '
               + '(PluginProcessor.cpp:96)'],
    ['Triangle', 'a WAVEFORM_PARAM AudioParameterChoice option, byte-identical — D-01 arm 1'],
    ['Phasor',   'a WAVEFORM_PARAM AudioParameterChoice option, byte-identical — D-01 arm 1'],
    ['Noise',    'a WAVEFORM_PARAM AudioParameterChoice option, byte-identical — D-01 arm 1'],
    ['Square',   'a WAVEFORM_PARAM AudioParameterChoice option, byte-identical — D-01 arm 1'],
    ['Pulse',    'a WAVEFORM_PARAM AudioParameterChoice option, byte-identical — D-01 arm 1'],

    // ── Endonyms ────────────────────────────────────────────────────────────
    ['English',  'an endonym in #lang-select — a language name is never translated, because a '
               + 'French speaker looking for their language looks for "Français"'],
    ['Français', 'an endonym in #lang-select — a language name is never translated'],
];

// ============================================================================
// TIP_BINDINGS — EMPTY, and that is this plugin's correct state.
//
// [selector, key] or [selector, key, wrapperSelector]. There is nothing to bind
// because there is no hover-help copy: v1.6.0 had none and this commit authors
// none. check-i18n assertion 2 accepts an empty list only while no I18N entry
// carries a body, which is exactly the case here — so an emptied TIP_BINDINGS
// over a bodied table still fails, on this plugin as on every other.
// ============================================================================

export const TIP_BINDINGS = [];

export function tr(key, lang, vars) {
    const entry = I18N[key];
    if (!entry) { console.warn(`i18n: missing key ${key}`); return { t: key, b: '' }; }
    const s = entry[lang] || entry.en;

    // A var VALUE that is itself an I18N key resolves to that key's localized
    // title; anything else is used literally. This plugin needs neither arm
    // today, but the resolving arm is what lets a plugin compose a localized
    // name into a tip without pinning TIP_BINDINGS — which is static data
    // evaluated once — to the load-time language. The canon is one shape across
    // all 43 plugins; this function is not trimmed per plugin.
    const resolve = (v) => {
        const nested = I18N[v];
        return nested ? String((nested[lang] || nested.en).t) : String(v);
    };

    const sub = (v) => vars
        ? String(v).replace(/\{(\w+)\}/g, (m, n) => (n in vars ? resolve(vars[n]) : m))
        : String(v);

    return { t: sub(s.t), b: sub(s.b) };
}
