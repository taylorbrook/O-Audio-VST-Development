/*
   This file is part of O-GrainScatter, an Ouaricon Audio plugin.
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
// i18n.js — O-GrainScatter LABEL table, English + French (v2.5.0)
//
// An ES module that EXPORTS ONLY. It must never self-execute: a bare top-level
// statement here throws out of module evaluation and takes every later
// initializer on the page with it (pattern_module_toplevel_init_tdz).
// scripts/check-i18n.js assertion 7 enforces it.
//
// FILENAME: no hyphen. juce_add_binary_data STRIPS hyphens rather than
// converting them to underscores, so a second file named i18n-fr.js would have
// to be reached as the symbol i18nfr_js (critical_binary_data_strips_hyphens).
// One combined file for both languages sidesteps the question entirely.
//
// NO HOVER-HELP COPY EXISTS ON THIS PAGE, AND THIS COMMIT DOES NOT AUTHOR ANY.
// v2.4.4 shipped ZERO native title= attributes, zero aria-label, zero
// placeholder and zero data-tip — verified by grep, not assumed. So `I18N` is
// EMPTY and `TIP_BINDINGS` is EMPTY, which is this plugin's correct state under
// Stage K and which check-i18n assertion 2 reports as "0 tip(s) bound" rather
// than passing silently. Authoring hover help is Stage M's job.
//
// COPY IS textContent ON EVERY PATH — never innerHTML. check-i18n assertion 9
// rejects any innerHTML reference here and any string literal containing `<`.
//
// THE ENGLISH WAS MOVED, NOT REWRITTEN. Every `en` entry below is byte-for-byte
// what index.html carried through v2.4.4, extracted mechanically rather than
// re-typed, with its HTML entities decoded to the characters they named
// (&amp; -> &) because textContent does not decode.
//
// ── FRENCH IS SIZED, NOT SHRUNK ────────────────────────────────────────────
// D-04 forbids an auto-shrink font and a short-variant fallback: there is
// exactly ONE French string per key here and nothing chooses between variants
// at runtime. Every caption below was measured IN THIS PAGE'S OWN ELEMENT — a
// width borrowed from another plugin is a wrong number that reads exactly like
// a right one (K2 measured the same two words 8.24 px apart on two pages).
//
// The two caps that matter on this page:
//   .knob-name     lives in a .knob-container of width: 62px
//   .dropdown-name lives in a .dropdown-container of width: 80px
// Both containers carry min-width: auto, so a caption whose LONGEST WORD passes
// the cap raises the container's min-content and pushes its whole flex row.
// The spatial row has ZERO px of slack, so that push is not cosmetic there.
//
// ALL FRENCH IS MACHINE-DRAFTED AND FLAGGED `reviewed: false`. No native
// speaker has read it. `node scripts/check-i18n.js` prints the worklist.
// ============================================================================

export const LANGUAGES = ['en', 'fr'];

// EMPTY, and deliberately so. This page has no hover-help copy: v2.4.4 carried
// no title=, no data-tip and no aria-label anywhere in index.html. An I18N
// entry here would be tooltip prose invented by this commit, which contract §4
// and the Stage K brief both forbid — that is Stage M.
export const I18N = Object.freeze({});

// ============================================================================
// LABELS — one string per key, no body. `{en:{t}, fr:{t, reviewed}}`.
// Widths in the comments are the RENDERED line box measured in this page's own
// element at 900x800, in px.
// ============================================================================

export const LABELS = Object.freeze({

    // ── Header ──────────────────────────────────────────────────────────────
    // The <h1> is the product name and is NOT keyed — see I18N_EXEMPT.
    'label.tagline':       { en: { t: 'Harmonic Stutter Engine' },
                             fr: { t: 'Moteur de bégaiement harmonique', reviewed: false } },

    // ── Visualisation panel captions ────────────────────────────────────────
    // Both are position:absolute inside their .viz-panel, so neither can push
    // anything; the only budget is the panel's own width (~558 and ~300 px).
    // NOT the product name: "O-GrainScatter" is one token and is exempt, while
    // this is a description of what the panel draws.
    'label.vizGrain':      { en: { t: 'Grain Scatter' },
                             fr: { t: 'Dispersion de grains', reviewed: false } },   // 84.67 -> 126.45
    'label.vizEuclidean':  { en: { t: 'Euclidean' },
                             fr: { t: 'Euclidien', reviewed: false } },              // 61.97 -> 59.44  SHRANK

    // ── Freeze toggle ───────────────────────────────────────────────────────
    // Alone in a full-width, centre-justified .freeze-bar, so its own box is
    // the only thing that changes and it is a [data-i18n] element. No pin.
    'label.freeze':        { en: { t: 'Freeze' },
                             fr: { t: 'Geler', reviewed: false } },                  // 71.67 -> 66.72 border box  SHRANK

    // ── Group headings ──────────────────────────────────────────────────────
    // Every .group-label is a full-width block, so its rectangle is
    // language-invariant by construction and none of these can move anything.
    'label.coreEngine':      { en: { t: 'Core Engine' },
                               fr: { t: 'Moteur principal', reviewed: false } },
    'label.pitchScale':      { en: { t: 'Pitch & Scale' },
                               fr: { t: 'Hauteur et gamme', reviewed: false } },
    'label.beatSync':        { en: { t: 'Beat Sync' },
                               fr: { t: 'Synchro rythmique', reviewed: false } },
    'label.euclideanRhythm': { en: { t: 'Euclidean Rhythm' },
                               fr: { t: 'Rythme euclidien', reviewed: false } },
    'label.spatialAudio':    { en: { t: 'Spatial Audio' },
                               fr: { t: 'Audio spatial', reviewed: false } },

    // ── Core Engine ─────────────────────────────────────────────────────────
    // "Taille" rather than "Taille grain": every knob in this group acts on a
    // grain, the sibling GRAIN SHAPE caption is already the bare word "Shape",
    // and "Taille grain" wraps to two 9 px line boxes in an 18 px content box —
    // 0.5 px from the assertion-4 wrap failure on a Windows metric.
    'label.grainSize':     { en: { t: 'Grain Size' },
                             fr: { t: 'Taille', reviewed: false } },                 // 50.61 -> 31.33  SHRANK
    'label.density':       { en: { t: 'Density' },
                             fr: { t: 'Densité', reviewed: false } },                // 38.34 -> 38.64
    'label.scan':          { en: { t: 'Scan' },
                             fr: { t: 'Balayage', reviewed: false } },               // 23.14 -> 46.11
    'label.spread':        { en: { t: 'Spread' },
                             fr: { t: 'Dispersion', reviewed: false } },             // 34.58 -> 54.02, 7.98 under the 62 px cap
    'label.reverse':       { en: { t: 'Reverse' },
                             fr: { t: 'Inverse', reviewed: false } },                // 40.23 -> 38.64  SHRANK
    // "Réinjection" (60.27) is the precise term and clears the cap by 1.73 px —
    // rejected on that margin alone, not on taste: it is tighter than every
    // margin shipped in batch K2 and this page's Windows metrics are unmeasured.
    // "Retour" (36.41) is the reviewer's roomier lever.
    'label.feedback':      { en: { t: 'Feedback' },
                             fr: { t: 'Réinject.', reviewed: false } },              // 46.53 -> 46.20
    'label.dryWet':        { en: { t: 'Dry/Wet' },
                             fr: { t: 'Sec/Effet', reviewed: false } },              // 41.77 -> 48.09
    'label.sizeRnd':       { en: { t: 'Size Rnd' },
                             fr: { t: 'Alé. taille', reviewed: false } },            // 41.33 -> 53.33
    'label.ampRnd':        { en: { t: 'Amp Rnd' },
                             fr: { t: 'Alé. ampl.', reviewed: false } },             // 40.84 -> 49.16
    'label.shape':         { en: { t: 'Shape' },
                             fr: { t: 'Forme', reviewed: false } },                  // 28.98 -> 31.52

    // ── Pitch & Scale ───────────────────────────────────────────────────────
    'label.pitchRnd':      { en: { t: 'Pitch Rnd' },
                             fr: { t: 'Alé. haut.', reviewed: false } },             // 48.78 -> 49.56
    'label.panRnd':        { en: { t: 'Pan Rnd' },
                             fr: { t: 'Alé. pan', reviewed: false } },               // 39.56 -> 39.89
    'label.scale':         { en: { t: 'Scale' },
                             fr: { t: 'Gamme', reviewed: false } },                  // 27.55 -> 33.73
    'label.rootNote':      { en: { t: 'Root Note' },
                             fr: { t: 'Fondamentale', reviewed: false } },           // 51.17 -> 73.41, 6.59 under the 80 px cap
    'label.pitchMode':     { en: { t: 'Pitch Mode' },
                             fr: { t: 'Mode hauteur', reviewed: false } },           // 56.13 -> 72.30
    // The hint NAMES a control whose caption this table also owns, so it takes
    // the caption as a {n} token rather than a second copy of the same words.
    // trLabel() resolves a token that is itself a LABELS key, so a reviewer who
    // changes label.pitchRnd changes this sentence with it instead of leaving
    // the two to drift. The markup keeps the fully-written English as its
    // render-if-applyI18n-never-runs fallback.
    'label.pitchHint':     { en: { t: 'Increase {n} to activate' },
                             fr: { t: 'Augmenter {n} pour activer', reviewed: false } },

    // ── Beat Sync ───────────────────────────────────────────────────────────
    'label.syncMode':      { en: { t: 'Sync Mode' },
                             fr: { t: 'Mode synchro', reviewed: false } },           // 51.70 -> 71.30
    // The TIGHTEST caption on the page: 2.98 px under the 62 px cap. Kept as
    // the whole word because the ENGLISH is the same long word at 58.72 — the
    // French is 0.30 px wider than what already ships, so an abbreviation here
    // would buy nothing English does not already spend. "Probab." (37.94) is
    // the reviewer's lever if a Windows metric ever proves it necessary.
    'label.probability':   { en: { t: 'Probability' },
                             fr: { t: 'Probabilité', reviewed: false } },            // 58.72 -> 59.02  TIGHTEST
    // "Répétitions" (59.16) clears the cap by 2.84 px. Unlike PROBABILITÉ there
    // is no English precedent for spending that: "Repeats" is 39.25. The page's
    // own register already abbreviates (Size Rnd, Amp Rnd, Dist LPF, Traj Speed).
    'label.repeats':       { en: { t: 'Repeats' },
                             fr: { t: 'Répét.', reviewed: false } },                 // 39.25 -> 31.06  SHRANK
    // Pinned to 110px in index.html. "Porte bégaiement" (138.58 border box) is
    // 28.58 px past that pin and would have to move the pin, which would move
    // ENGLISH. "Bégaiement" (102.00) fits and is the reviewer's lever; it drops
    // the gate half of the name, which is why it is not the shipped choice.
    'label.stutterGate':   { en: { t: 'Stutter Gate' },
                             fr: { t: 'Porte bég.', reviewed: false } },             // 109.95 -> 93.08 border box  SHRANK

    // ── Euclidean Rhythm ────────────────────────────────────────────────────
    'label.pulses':        { en: { t: 'Pulses' },
                             fr: { t: 'Impulsions', reviewed: false } },             // 32.97 -> 55.48, 6.52 under the cap
    'label.steps':         { en: { t: 'Steps' },
                             fr: { t: 'Pas', reviewed: false } },                    // 26.53 -> 16.23  SHRANK
    'label.rotation':      { en: { t: 'Rotation' },
                             fr: { t: 'Rotation', reviewed: false, sameAsEn: true } },
    // The musical term is used untranslated in French practice, and there is no
    // French word for it that is not a paraphrase ("balancement" is 66.53 and
    // 4.53 px OVER the cap in its own right).
    'label.swing':         { en: { t: 'Swing' },
                             fr: { t: 'Swing', reviewed: false, sameAsEn: true } },

    // ── Spatial Audio ───────────────────────────────────────────────────────
    // This is the ZERO-SLACK row: its twelve controls sum to exactly 846.00 px
    // inside an 846.00 px container. Every French caption below was chosen so
    // its longest word stays under its container's cap, because one that does
    // not wraps SMOOTHING onto a second row, grows #spatial-group by 87 px and
    // pushes the page past its own 800 px frame.
    'label.mode':          { en: { t: 'Mode' },
                             fr: { t: 'Mode', reviewed: false, sameAsEn: true } },
    'label.azimuth':       { en: { t: 'Azimuth' },
                             fr: { t: 'Azimut', reviewed: false } },                 // 41.73 -> 34.72  SHRANK
    'label.elevation':     { en: { t: 'Elevation' },
                             fr: { t: 'Élévation', reviewed: false } },              // 50.64 -> 50.64  IDENTICAL WIDTH
    'label.azSpread':      { en: { t: 'Az Spread' },
                             fr: { t: 'Disp. az.', reviewed: false } },              // 48.19 -> 39.41  SHRANK
    'label.elSpread':      { en: { t: 'El Spread' },
                             fr: { t: 'Disp. él.', reviewed: false } },              // 48.06 -> 39.28  SHRANK
    'label.distance':      { en: { t: 'Distance' },
                             fr: { t: 'Distance', reviewed: false, sameAsEn: true } },
    'label.width':         { en: { t: 'Width' },
                             fr: { t: 'Largeur', reviewed: false } },                // 30.89 -> 42.00
    // KEYED, while the spatial_mode OPTION spelled the same way is EXEMPT.
    // That is the one state assertion 14 demands a scope for, and the exempt
    // entry below carries `option`.
    'label.trajectory':    { en: { t: 'Trajectory' },
                             fr: { t: 'Trajectoire', reviewed: false } },            // 56.88 -> 60.80
    'label.trajSpeed':     { en: { t: 'Traj Speed' },
                             fr: { t: 'Vit. traj.', reviewed: false } },             // 52.81 -> 44.72  SHRANK
    'label.distLpf':       { en: { t: 'Dist LPF' },
                             fr: { t: 'PB dist.', reviewed: false } },               // 38.98 -> 36.75  SHRANK
    'label.doppler':       { en: { t: 'Doppler' },
                             fr: { t: 'Doppler', reviewed: false, sameAsEn: true } },
    'label.smoothing':     { en: { t: 'Smoothing' },
                             fr: { t: 'Lissage', reviewed: false } },                // 54.84 -> 36.83  SHRANK
    // "Scatter" and "Trajectory" stay ENGLISH inside the French sentence on
    // purpose: they are the two spatial_mode option strings the user has to
    // find in the dropdown beside it, and those are exempt under D-01 arm 1.
    // A translated instruction naming a control that is not translated is the
    // instruction that cannot be followed.
    'label.spatialHint':   { en: { t: 'Set Mode to Scatter or Trajectory to enable' },
                             fr: { t: 'Régler Mode sur Scatter ou Trajectory pour activer',
                                   reviewed: false } },                              // 154.25 -> 184.22, in an 846 px block

    // ── Settings popover (new in v2.5.0) ────────────────────────────────────
    'label.language':      { en: { t: 'Language' },
                             fr: { t: 'Langue', reviewed: false } },

    // ── Accessible names ────────────────────────────────────────────────────
    // An aria-label is user-visible text by any definition that matters — it is
    // the accessible NAME, and a screen reader in French reading an English
    // name is the same failure as a French page with an English caption. These
    // have no rendered box, so neither is a geometry risk.
    //
    // These two are the ONLY accessible names on the page, and both belong to
    // controls this commit ADDS. No existing control gains one: v2.4.4 carried
    // no title= to move under contract §4, and inventing hover-help prose for
    // the other 46 captions is Stage M.
    'aria.settings':       { en: { t: 'Settings' },
                             fr: { t: 'Réglages', reviewed: false } },
    'aria.langSelect':     { en: { t: 'Interface language' },
                             fr: { t: 'Langue de l’interface', reviewed: false } },
});

// ============================================================================
// I18N_EXEMPT — reasoned exclusions, never silence
// ============================================================================
//
// Every visible string the coverage scan finds must be a [data-i18n] element,
// a setLabel() call, or an entry HERE WITH A REASON. A bare skip list would let
// a missed label hide as a deliberate one.
//
// An entry is [text, reason] or [text, reason, scope]. A scope is REQUIRED
// exactly where the same string is also KEYED on this page — the one state in
// which a text match cannot tell a deliberate skip from a forgotten label.
// On this page that is "Trajectory", and only "Trajectory".
// ============================================================================

export const I18N_EXEMPT = [
    // The product name, in the <h1>. One token, no split.
    ['O-GrainScatter', 'the product name — a product name is never translated'],

    // ── D-01 ARM 1: AudioParameterChoice option strings, BYTE-IDENTICAL ─────
    // Verified against PluginProcessor.cpp rather than assumed: grain_shape at
    // :135, scale at :105, root_note at :111, pitch_mode at :197, sync_mode at
    // :164, spatial_mode at :239, trajectory at :281. The page and the host
    // automation lane must agree on these, character for character.
    //
    // The twelve root_note options (C, C#, D … B) are not listed: they are
    // single letters, the coverage scan never classifies them as LABEL, and an
    // entry the scan cannot reach is decoration rather than an exemption.
    ['Hann',       'a grain_shape option string VERBATIM (PluginProcessor.cpp:135) — D-01 arm 1'],
    ['Triangle',   'a grain_shape option string VERBATIM (PluginProcessor.cpp:135) — D-01 arm 1'],
    ['Trapezoid',  'a grain_shape option string VERBATIM (PluginProcessor.cpp:135) — D-01 arm 1'],
    ['Tukey',      'a grain_shape option string VERBATIM (PluginProcessor.cpp:135) — D-01 arm 1'],
    ['Blackman',   'a grain_shape option string VERBATIM (PluginProcessor.cpp:135) — D-01 arm 1'],
    ['Exp Decay',  'a grain_shape option string VERBATIM (PluginProcessor.cpp:135) — D-01 arm 1'],

    ['Chromatic',  'a scale option string VERBATIM (PluginProcessor.cpp:105) — D-01 arm 1'],
    ['Major',      'a scale option string VERBATIM (PluginProcessor.cpp:105) — D-01 arm 1'],
    ['Minor',      'a scale option string VERBATIM (PluginProcessor.cpp:105) — D-01 arm 1'],
    ['Pentatonic', 'a scale option string VERBATIM (PluginProcessor.cpp:105) — D-01 arm 1'],
    ['Whole Tone', 'a scale option string VERBATIM (PluginProcessor.cpp:105) — D-01 arm 1'],

    ['Random',       'a pitch_mode AND a trajectory option string VERBATIM '
                   + '(PluginProcessor.cpp:197, :281) — D-01 arm 1. Unscoped is correct: '
                   + 'both nodes carrying this text are options, and no LABELS key resolves to it'],
    ['Ladder Up',    'a pitch_mode option string VERBATIM (PluginProcessor.cpp:197) — D-01 arm 1'],
    ['Ladder Down',  'a pitch_mode option string VERBATIM (PluginProcessor.cpp:197) — D-01 arm 1'],
    ['Pendulum',     'a pitch_mode option string VERBATIM (PluginProcessor.cpp:197) — D-01 arm 1'],

    ['Free',       'a sync_mode option string VERBATIM (PluginProcessor.cpp:164) — D-01 arm 1'],

    ['Off',        'a spatial_mode option string VERBATIM (PluginProcessor.cpp:239) — D-01 arm 1'],
    ['Scatter',    'a spatial_mode option string VERBATIM (PluginProcessor.cpp:239) — D-01 arm 1. '
                 + 'Also the word the French spatial hint keeps in English, for the same reason'],

    // THE ONE SCOPED ENTRY. "Trajectory" is a spatial_mode option AND the
    // caption of the trajectory dropdown, which IS keyed (label.trajectory ->
    // "Trajectoire"). Unscoped, this entry would silence the caption too, and a
    // forgotten label there would read exactly like this deliberate skip —
    // O-Detune's "Random" in Stage K, on a different page. The scope pins it to
    // the <option> and nothing else.
    ['Trajectory', 'a spatial_mode option string VERBATIM (PluginProcessor.cpp:239) — D-01 arm 1. '
                 + 'SCOPED because the .dropdown-name caption spelled the same way IS keyed as '
                 + 'label.trajectory and must translate',
                   'option'],

    ['Static',     'a trajectory option string VERBATIM (PluginProcessor.cpp:281) — D-01 arm 1'],
    ['Orbital',    'a trajectory option string VERBATIM (PluginProcessor.cpp:281) — D-01 arm 1'],
    ['Spiral',     'a trajectory option string VERBATIM (PluginProcessor.cpp:281) — D-01 arm 1'],

    // ── CANVAS TEXT: a named gap, recorded rather than discovered ───────────
    // GrainScatterViz.draw() paints "<n> grains" with ctx.fillText into
    // #grain-canvas. A 2D-context string is not a DOM node: the canon sweep
    // cannot reach it, neither gate can see it, and localizing it would need a
    // repaint hook outside the canon. This is the suite's existing position —
    // O-Orbit ships FRONT and ELEV the same way, O-MultiBandCompressor its
    // analyzer placeholder, O-simpleSampler two waveform-editor strings. Named
    // gap, owner none, carried into Stage M.
    ['grains',     'canvas ctx.fillText in GrainScatterViz.draw (app.js) — not a DOM node, so the '
                 + 'canon sweep cannot reach it; the same named gap as O-Orbit, '
                 + 'O-MultiBandCompressor and O-simpleSampler'],
    // The canvas's other four strings — "0s", "2s", "+24st", "-24st" and the
    // Euclidean centre readout "4/8 r2" — are axis units and numbers, exempt
    // under D-03 in their own right as well as being canvas text.
];

// [selector, key] or [selector, key, wrapperSelector].
//
// EMPTY. This page has no hover-help anchors because it has no hover-help copy.
// check-i18n assertion 2 accepts an empty TIP_BINDINGS only while no I18N entry
// carries a body — I18N is empty above, so the pair is consistent rather than
// orphaned, and the gate reports "0 tip(s) bound" rather than passing silently.
export const TIP_BINDINGS = [];

// The tooltip lookup. Returns {t, b} — never null, never a bare key without a
// console.warn saying so, because a silently-missing tip renders as an empty
// surface that looks like a positioning bug rather than a missing entry.
//
// UNREFERENCED AT RUNTIME TODAY: applyI18n() calls it only from the
// TIP_BINDINGS loop, which is empty, and every visible string on this page goes
// through trLabel() instead. It is exported verbatim all the same, so that this
// plugin's copy of the canon block stays byte-identical to the other forty-two
// — the canonical import line NAMES tr, and assertion 6 byte-compares it — and
// so Stage M can add bodies to I18N without touching this file's shape.
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
