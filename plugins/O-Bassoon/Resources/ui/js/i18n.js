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
// i18n.js — O-Bassoon page labels, English + French (v1.1.0)
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
// ── THIS PLUGIN HAS NO HOVER-HELP, AND THIS COMMIT DOES NOT GIVE IT ANY ─────
//
// v1.0.0 carried no data-tip and no data-tooltip anywhere — only three native
// title= attributes, which contract §4 DELETES rather than localizes. Each of
// those three was the only help its element had, so its text moved to
// data-i18n-aria and NO NEW PROSE WAS INVENTED. Authoring hover-help copy is
// Stage M's job. I18N is therefore empty and TIP_BINDINGS is empty, which is
// this plugin's correct state rather than a gap: check-i18n assertion 2 reports
// it as "0 tip(s) bound" instead of passing silently, and the emptiness is only
// admissible BECAUSE no I18N entry carries a body.
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
// I18N — hover-help copy. EMPTY, deliberately.
//
// A tooltip entry is {t, b}: a title and a body. This page has neither, so the
// table has no entries. It is exported all the same because the canonical
// import line names it and trLabel() falls back through it — a control whose
// tooltip title already IS its caption is meant to carry ONE key, and that
// fallback must exist even on a plugin that has no tooltips today, so Stage M
// can add bodies here without touching the label keys below.
// ============================================================================

export const I18N = Object.freeze({});

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
//     RELEASE     46.9 -> RELÂCHE     49.2
//     VOICES      38.3 -> VOIX        26.3    SHRANK
//     OUTPUT      42.6 -> SORTIE      38.5    SHRANK
//
// Five of the ten SHRINK. That is the half of the risk a clip check is blind to
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
        fr: { t: 'Ouaricon · Basson à synthèse modale', reviewed: false },
    },

    // ── Tab bar ─────────────────────────────────────────────────────────────
    // Three `flex: 1` buttons, 300 px each, centred text. The widest French
    // caption is 72.5 px, so the row cannot be pushed by any of them.
    'label.tab.sound':  { en: { t: 'Sound' },  fr: { t: 'Son',      reviewed: false } },
    'label.tab.tuning': { en: { t: 'Tuning' }, fr: { t: 'Accord',   reviewed: false } },
    'label.tab.about':  { en: { t: 'About' },  fr: { t: 'À propos', reviewed: false } },

    // ── Section headings ────────────────────────────────────────────────────
    // `sameAsEn: true` is an ASSERTION, not a shrug: "Vibrato" and "Expression"
    // are the same word in French, and the flag is what stops assertion 4
    // reading an identical string as an untranslated one. Without it the gate
    // cannot tell a deliberate cognate from a forgotten entry.
    'label.section.vibrato': {
        en: { t: 'Vibrato' },
        fr: { t: 'Vibrato', reviewed: false, sameAsEn: true },
    },
    'label.section.expression': {
        en: { t: 'Expression' },
        fr: { t: 'Expression', reviewed: false, sameAsEn: true },
    },
    'label.section.envelope': {
        en: { t: 'Envelope' },
        fr: { t: 'Enveloppe', reviewed: false },
    },
    'label.section.voicing': {
        en: { t: 'Voicing & Output' },
        fr: { t: 'Voix et sortie', reviewed: false },
    },

    // ── Knob captions ───────────────────────────────────────────────────────
    // Captions under the knob, never the value beside them: the .knob-value
    // sibling is a separate node and stays untouched (contract §5 — the split
    // this page already had).
    'label.knob.rate':   { en: { t: 'Rate' },   fr: { t: 'Vitesse',    reviewed: false } },
    'label.knob.depth':  { en: { t: 'Depth' },  fr: { t: 'Profondeur', reviewed: false } },

    // vibrato_onset is the DELAY before the vibrato speaks, 0-2000 ms
    // (PluginProcessor.cpp, "Vibrato Onset"). "Délai" is the French term for
    // that delay; "Début" would name the moment rather than the wait.
    'label.knob.onset':  { en: { t: 'Onset' },  fr: { t: 'Délai',      reviewed: false } },

    'label.knob.breath': { en: { t: 'Breath' }, fr: { t: 'Souffle',    reviewed: false } },
    'label.knob.tone':   { en: { t: 'Tone' },   fr: { t: 'Timbre',     reviewed: false } },

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
        fr: { t: 'Caractère', reviewed: false },
    },

    'label.knob.attack':  { en: { t: 'Attack' },  fr: { t: 'Attaque', reviewed: false } },
    'label.knob.release': { en: { t: 'Release' }, fr: { t: 'Relâche', reviewed: false } },
    'label.knob.voices':  { en: { t: 'Voices' },  fr: { t: 'Voix',    reviewed: false } },
    'label.knob.output':  { en: { t: 'Output' },  fr: { t: 'Sortie',  reviewed: false } },

    // ── The attack_character end-label pair ─────────────────────────────────
    // "Détaché" is the bassoon articulation term a French player would use for a
    // tongued note; "Coup de langue" is the literal phrase and is twice as wide
    // in a 78 px row that already carries two captions.
    'label.end.soft':    { en: { t: 'Soft' },    fr: { t: 'Doux',    reviewed: false } },
    'label.end.tongued': { en: { t: 'Tongued' }, fr: { t: 'Détaché', reviewed: false } },

    // ── About card ──────────────────────────────────────────────────────────
    // "Version" is the same word in French, hence the flag. The NUMBER beside it
    // was split into its own span in this commit and carries no key: a version
    // string is not copy, and leaving it inside the localized string would put
    // the shipping version behind a translation nobody re-checks (contract §5).
    'label.about.version': {
        en: { t: 'Version' },
        fr: { t: 'Version', reviewed: false, sameAsEn: true },
    },
    'label.about.tagline': {
        en: { t: 'Modal-synthesis bassoon for sustained microtonal long tones.' },
        fr: { t: 'Basson à synthèse modale pour de longues tenues microtonales.', reviewed: false },
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
        fr: { t: 'Polyphonie de 1 à 16 voix, VST3 Note Expression + MPE pour la lecture microtonale dans Dorico, expression au souffle/CC2, vibrato, et la famille de systèmes d’accord Ouaricon. Conçu avec JUCE 8.', reviewed: false },
    },
    'label.about.madeBy': {
        en: { t: 'Made by' },
        fr: { t: 'Réalisé par', reviewed: false },
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
        fr: { t: 'Ouaricon', reviewed: false, sameAsEn: true },
    },

    // ── The settings popover (v1.1.0) ───────────────────────────────────────
    'label.language': { en: { t: 'Language' }, fr: { t: 'Langue', reviewed: false } },

    // ── The one JS-written string on this page ──────────────────────────────
    // The tuning panel is lazy-mounted on the first Tuning-tab activation and
    // this is what the container says if that dynamic import fails. It is
    // written through setLabel(), so the node becomes a [data-i18n] element from
    // that moment on and the language sweep owns it — a failure notice stranded
    // in the previous language is exactly the bug contract §3 exists to prevent.
    'label.tuningLoadFailed': {
        en: { t: 'Tuning panel failed to load.' },
        fr: { t: 'Échec du chargement du panneau d’accord.', reviewed: false },
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
        fr: { t: 'Enveloppe du vibrato', reviewed: false },
    },
    'aria.breathMeter': {
        en: { t: 'Effective breath (UI breath × CC2)' },
        fr: { t: 'Souffle effectif (souffle interface × CC2)', reviewed: false },
    },
    'aria.voiceDots': {
        en: { t: 'Live active voice count' },
        fr: { t: 'Nombre de voix actives en direct', reviewed: false },
    },

    'aria.settings': {
        en: { t: 'Settings' },
        fr: { t: 'Réglages', reviewed: false },
    },
    'aria.langSelect': {
        en: { t: 'Interface language' },
        fr: { t: 'Langue de l’interface', reviewed: false },
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
