/*
   This file is part of O-Wind, an Ouaricon Audio plugin.
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
// i18n.js — O-Wind visible-text table, English + French (v1.17.0)
//
// An ES module that EXPORTS ONLY. A bare top-level statement here throws out of
// module evaluation and takes every later initializer on the page with it
// (pattern_module_toplevel_init_tdz). check-i18n assertion 7 enforces it.
//
// FILENAME: no hyphen. juce_add_binary_data STRIPS hyphens rather than
// converting them to underscores, so a second file named i18n-fr.js would have
// to be reached as the symbol i18nfr_js (critical_binary_data_strips_hyphens).
// One combined file for both languages sidesteps the question entirely.
//
// COPY IS textContent ON EVERY PATH — never innerHTML. check-i18n assertion 9
// rejects any innerHTML reference here and any string literal containing `<`.
//
// THE ENGLISH WAS MOVED, NOT REWRITTEN. Every `en` entry below is byte-for-byte
// what index.html carried through v1.16.3, extracted mechanically rather than
// re-typed. The authored English also STAYS in the markup as the fallback that
// renders if applyI18n never runs (contract section 1).
//
// ALL FRENCH IS MACHINE-DRAFTED AND FLAGGED `reviewed: false`. No native
// speaker has read it. `node scripts/check-i18n.js` prints the worklist.
//
// ── WHAT THE EXTRACTOR COULD NOT SEE, AND IS THE REAL FINDING HERE ──────────
//
// `node scripts/i18n-extract.js --plugin O-Wind` reports 61 LABEL, 7 READOUT,
// 3 UNSURE, 3 attributes and 4 JS-prose strings. Every one of those numbers is
// right, and the JS-prose one is right about a set that is far too small.
//
// THE EFFECTS TAB IS BUILT ENTIRELY FROM SCRIPT. Sixteen knob captions are
// passed as ordinary function arguments —
//
//     populateFxKnobs('reverb-knobs', [{ id: 'reverbSize', label: 'Size' }, ...])
//     delayRow.appendChild(makeFxKnob('delayTime', 'Time'))
//
// — and makeFxKnob() interpolates them into an innerHTML template. The
// extractor's js-prose scan looks for a PROSE LITERAL on the right-hand side of
// a `.textContent =` / `.innerHTML =` assignment; an English word sitting in an
// object literal one frame away from the write is neither. Assertion 12 is
// blind to it for exactly the same reason. That is the O-Tremolo shape the
// batch addendum names, at sixteen strings rather than one, and leaving them
// alone would have shipped an English Effects tab inside a French plugin with
// every gate GREEN.
//
// The fix is not a cleverer scan — it is that every one of the sixteen now
// carries a LITERAL key at its own call site, so assertions 13 and 15 can both
// read it:
//
//     setLabel(addFxKnob('reverb-knobs', 'reverbSize'), 'label.fx.size');
//
// A key computed from `k.key` inside populateFxKnobs() would have failed
// assertion 13 (a computed setLabel key cannot be checked) and would have been
// invisible to assertion 15's dead-key sweep. Sixteen literal call sites are
// verbose and they are the only shape both gates can see.
//
// SIXTEEN NATIVE title= ATTRIBUTES WERE ALSO WRITTEN FROM SCRIPT. setupFxKnob()
// did `valueDisplay.title = 'Double-click to edit'` on every FX readout. The
// page rendered NINETEEN native titles (boot-all-uis, v1.16.3) against the
// THREE the markup declares. Assertion 11 reads index.html only, so it counted
// three. Contract section 4 deletes a native title= rather than localizing it,
// and where the title is an element's only help its text moves to
// data-i18n-aria with NO NEW PROSE INVENTED — so the sixteen are now
// `valueDisplay.dataset.i18nAria = 'aria.fxValueEdit'`, carrying the same
// eighteen characters into an accessible name the language sweep owns.
//
// ── THE D-01 TEST ON THIS PLUGIN ────────────────────────────────────────────
//
// ARM 1 — O-Wind has exactly TWO AudioParameterChoice parameters:
//   tuningSystem  { 'Scala/TUN', 'MTS-ESP', '12-TET' }   PluginProcessor.cpp:347
//   delayMode     { 'Normal', 'PingPong' }               PluginProcessor.cpp:377
// The three tuningSystem options appear only inside the Tuning tab, which is
// the shared module and out of scope (see I18N_EXEMPT). The two delayMode
// options ARE page copy — initializeEffects() writes them into the delay-mode
// <select> — and they are byte-identical to the option strings, so the page and
// the host automation lane must name them identically. Both are exempt on arm 1
// and neither is keyed anywhere on this page, so neither needs a scope.
//
// NOT AN ARM-1 CASE, and worth saying because it looks like one:
// `instrumentPreset` is an AudioParameterInt (0..7), NOT a Choice. Its eight
// <option> captions are exempt on a different rule — see I18N_EXEMPT.
//
// ARM 2 / ARM 3 — the seven `.knob-value` READOUT rows and the sixteen FX value
// readouts. formatValue() and setupFxKnob()'s updateVisual() overwrite every one
// of them on the first valueChangedEvent. They are exempt on arm 2 (a number
// and its unit are language-neutral, D-03) AND on arm 3 (a readout node is never
// a [data-i18n] element whatever parameter type is behind it — keying one would
// make the element enter and leave the sweep as the knob turns). They are not
// listed individually in I18N_EXEMPT: the coverage scan already classes them
// non-LABEL, and twenty-three entries whose text changes on the first mouse drag
// would be twenty-three entries that never match anything again.
//
// ── THE NINE NON-LITERAL textContent WRITES, WALKED TO THEIR CALL SITES ─────
//
// The batch addendum measures nine `textContent`/`innerText` assignments with a
// non-literal right-hand side on this page. Every one was walked:
//
//   1310, 1315, 1319  `.knob-value = formatValue(param, state.getScaledValue())`
//                     a number and a unit. Arm 2/3. EXEMPT.
//   1511              `item.textContent = name` — a preset-dropdown row. The
//                     name comes from getPresetList() and IS the JSON filename
//                     (OuariconPresetManager.h). D-02. EXEMPT.
//   1696              `valueDisplay.textContent = formatter(realValue) + suffix`
//                     a number and a unit. Arm 2/3. EXEMPT.
//   1743              `valueDisplay.textContent = ''` — clearing the node before
//                     an <input> goes in. No string at all.
//   1788              `btn.textContent = bypassed ? 'Off' : 'On'` — TWO ENGLISH
//                     LITERALS one frame away. Now two literal setLabel calls,
//                     not a ternary: contract section 6, assertion 13.
//   1823              `opt.textContent = name` over ['Normal', 'PingPong'] —
//                     delayMode option strings VERBATIM. Arm 1. EXEMPT.
//   1654              `container.innerHTML = ...${label}...` inside makeFxKnob —
//                     THE SIXTEEN. See above.
//
// Two of the nine were carrying English into a French UI. Both are fixed here.
// ============================================================================

export const LANGUAGES = ['en', 'fr'];

// ============================================================================
// I18N — hover-help copy. EMPTY, deliberately.
//
// A tooltip entry is {t, b}: a title and a body. v1.16.3 had no data-tip
// anywhere and this stage does not author hover-help — that is Stage M. The
// table is exported all the same, because the canonical import line names it
// and trLabel() falls back through it: a control whose tooltip title already IS
// its caption is meant to carry ONE key, and that fallback must exist even on a
// plugin with no tooltips today.
// ============================================================================

export const I18N = Object.freeze({});

// ============================================================================
// LABELS — the visible text of the page. {en:{t}, fr:{t, reviewed}}.
// One string per entry, no body: a label is not a tooltip.
//
// ── THE GEOMETRY CLIFF ON THIS PAGE, MEASURED ───────────────────────────────
//
// `.knob-label` is `font-size: 9px; text-transform: uppercase;
// letter-spacing: 0.5px; max-width: 72px; white-space: nowrap; overflow:
// hidden; text-overflow: ellipsis`. An over-long caption renders an ELLIPSIS
// rather than overflowing, which is the half a spill check cannot see, and
// `#tab-effects .knob-label` inherits the same cap — it overrides only colour,
// weight and font-size.
//
// Every number below was read from the RENDERED element in this plugin's own
// 900 x 600 frame. text-transform and letter-spacing are not in
// getComputedStyle().font, and K2 proved two plugins give absolutes 8 px apart
// for the same two words at the same declared size, so no width here was
// borrowed from another plugin's page.
//
// ENGLISH IS ALREADY WITHIN 1.3 PX OF THE CAP: "Embouchure" measures 70.73 in
// the 72 px box. It is the same word in French, so it neither clips nor moves —
// but it is why no French caption on this page was drafted longer than ten
// characters, and why several are abbreviated where the English is not.
// ============================================================================

export const LABELS = Object.freeze({

    // ── Preset bar ──────────────────────────────────────────────────────────
    // "Enregistrer" is 11 characters against "Save"'s 4 and would grow
    // .preset-save-btn by ~44 px, shrinking the flex:1 #preset-name beside it.
    // "Enreg." is the abbreviation a transport bar uses, and it is what keeps
    // the bar's geometry language-invariant without pinning a width on a button
    // whose neighbour is the elastic one.
    'label.presetSave': { en: { t: 'Save' }, fr: { t: 'Enreg.', reviewed: false } },

    'aria.presetPrev': {
        en: { t: 'Previous Preset' },
        fr: { t: 'Préréglage précédent', reviewed: false },
    },
    'aria.presetNext': {
        en: { t: 'Next Preset' },
        fr: { t: 'Préréglage suivant', reviewed: false },
    },
    // MOVED, NOT AUTHORED. This is the v1.16.3 native title= on
    // #preset-name, verbatim. Contract section 4 deletes a native title rather
    // than localizing it, and where the title is an element's only help its text
    // becomes the accessible name — no new prose is invented here.
    //
    // #preset-name is deliberately NOT a [data-i18n] element. It holds
    // #preset-dropdown as a CHILD, and applyLabel() writes textContent, which
    // would delete the entire dropdown on the first language sweep.
    'aria.presetName': {
        en: { t: 'Click to browse presets' },
        fr: { t: 'Cliquer pour parcourir les préréglages', reviewed: false },
    },

    // ── Tab bar ─────────────────────────────────────────────────────────────
    // `.tab-btn` is `flex: 1` in a 900 px bar, so each is exactly 300 px wide
    // and centred whatever it says. Nothing here can push anything.
    'label.tab.sound':   { en: { t: 'Sound' },   fr: { t: 'Son',     reviewed: false } },
    'label.tab.tuning':  { en: { t: 'Tuning' },  fr: { t: 'Accord',  reviewed: false } },
    'label.tab.effects': { en: { t: 'Effects' }, fr: { t: 'Effets',  reviewed: false } },

    // ── Settings popover (v1.17.0) ──────────────────────────────────────────
    'label.language':  { en: { t: 'Language' }, fr: { t: 'Langue', reviewed: false } },
    'aria.settings':   { en: { t: 'Settings' }, fr: { t: 'Réglages', reviewed: false } },
    'aria.langSelect': {
        en: { t: 'Interface language' },
        fr: { t: 'Langue de l’interface', reviewed: false },
    },

    // ── Instrument strip ────────────────────────────────────────────────────
    'label.instrument': { en: { t: 'Instrument' }, fr: { t: 'Instrument', reviewed: false, sameAsEn: true } },
    // The flute term is "trous de jeu". At 11 px uppercase that is ~90 px
    // against "Tone Holes"' 75.42, and .toggle-control is a flex item in the
    // instrument strip, so the widening would push the preset selector beside
    // it. .toggle-label is pinned to a box that holds BOTH — see index.html.
    'label.toneHoles':  { en: { t: 'Tone Holes' }, fr: { t: 'Trous de jeu', reviewed: false } },
    'label.preset':     { en: { t: 'Preset' },     fr: { t: 'Préréglage',   reviewed: false } },

    // ── Excitation ──────────────────────────────────────────────────────────
    'label.excitation': { en: { t: 'Excitation' }, fr: { t: 'Excitation', reviewed: false, sameAsEn: true } },
    'label.breath':     { en: { t: 'Breath' },     fr: { t: 'Souffle',    reviewed: false } },
    // Identical in French. sameAsEn is an ASSERTION, not a shrug: it is what
    // stops assertion 4 reading an identical string as an untranslated one.
    'label.embouchure': { en: { t: 'Embouchure' }, fr: { t: 'Embouchure', reviewed: false, sameAsEn: true } },
    'label.noise':      { en: { t: 'Noise' },      fr: { t: 'Bruit',      reviewed: false } },

    // ── Resonator ───────────────────────────────────────────────────────────
    'label.resonator': { en: { t: 'Resonator' },  fr: { t: 'Résonateur', reviewed: false } },
    'label.material':  { en: { t: 'Material' },   fr: { t: 'Matériau',   reviewed: false } },
    'label.toneColor': { en: { t: 'Tone Color' }, fr: { t: 'Timbre',     reviewed: false } },
    // "Colonne d'air" is 13 characters — ~80 px in the 72 px box, an ellipsis.
    // "Colonne" is unambiguous under a knob in the RESONATOR section.
    'label.airColumn': { en: { t: 'Air Column' }, fr: { t: 'Colonne',    reviewed: false } },
    'label.jetRefl':   { en: { t: 'Jet Refl.' },  fr: { t: 'Réfl. jet',  reviewed: false } },
    'label.endRefl':   { en: { t: 'End Refl.' },  fr: { t: 'Réfl. bout', reviewed: false } },

    // ── ADSR envelope ───────────────────────────────────────────────────────
    // The caption was SPLIT out of `.section-label` into its own <span>
    // (contract section 5): the div also holds the #adsr-toggle as an element
    // child, and applyLabel() writes textContent, which would have deleted the
    // toggle on the first language sweep.
    'label.adsrEnvelope': { en: { t: 'ADSR Envelope' }, fr: { t: 'Enveloppe ADSR', reviewed: false } },
    'label.attack':       { en: { t: 'Attack' },  fr: { t: 'Attaque',  reviewed: false } },
    'label.decay':        { en: { t: 'Decay' },   fr: { t: 'Chute',    reviewed: false } },
    'label.sustain':      { en: { t: 'Sustain' }, fr: { t: 'Maintien', reviewed: false } },
    'label.release':      { en: { t: 'Release' }, fr: { t: 'Relâche',  reviewed: false } },

    // ── Expression ──────────────────────────────────────────────────────────
    'label.expression':  { en: { t: 'Expression' },  fr: { t: 'Expression', reviewed: false, sameAsEn: true } },
    'label.vibRate':     { en: { t: 'Vib Rate' },    fr: { t: 'Vit. vibr.', reviewed: false } },
    'label.vibPitch':    { en: { t: 'Vib Pitch' },   fr: { t: 'Haut. vibr.', reviewed: false } },
    // "Trémolo vibr." would be 13 characters in a 72 px box. The knob sits
    // between Vib Pitch and Drift Depth in the EXPRESSION section, so the
    // vibrato is already named by its neighbours.
    'label.vibTremolo':  { en: { t: 'Vib Tremolo' }, fr: { t: 'Trémolo',    reviewed: false } },
    'label.driftDepth':  { en: { t: 'Drift Depth' }, fr: { t: 'Prof. dér.', reviewed: false } },
    'label.driftSpeed':  { en: { t: 'Drift Speed' }, fr: { t: 'Vit. dér.',  reviewed: false } },
    // "Frullato" is the term French orchestration uses for flutter-tonguing;
    // "flatterzunge" is the other, and it is 12 characters.
    'label.flutter':     { en: { t: 'Flutter' },     fr: { t: 'Frullato',   reviewed: false } },
    'label.flutRate':    { en: { t: 'Flut Rate' },   fr: { t: 'Vit. frul.', reviewed: false } },
    // A loanword in French jazz vocabulary, spelled identically.
    'label.growl':       { en: { t: 'Growl' },       fr: { t: 'Growl',      reviewed: false, sameAsEn: true } },

    // ── Output ──────────────────────────────────────────────────────────────
    // ONE key on TWO elements: the OUTPUT section legend and the outputLevel
    // knob caption. One concept, one string — two entries would be two copies
    // of the same word in one table, drifting apart on the first edit.
    'label.output':   { en: { t: 'Output' },   fr: { t: 'Sortie',  reviewed: false } },
    'label.width':    { en: { t: 'Width' },    fr: { t: 'Largeur', reviewed: false } },
    'label.formant':  { en: { t: 'Formant' },  fr: { t: 'Formant', reviewed: false, sameAsEn: true } },

    // ── Impossible physics ──────────────────────────────────────────────────
    'label.impossiblePhysics': {
        en: { t: 'Impossible Physics' },
        fr: { t: 'Physique impossible', reviewed: false },
    },
    'label.infSustain': { en: { t: 'Inf. Sustain' }, fr: { t: 'Tenue inf.', reviewed: false } },
    'label.revJet':     { en: { t: 'Rev. Jet' },     fr: { t: 'Jet inv.',   reviewed: false } },
    'label.subHarm':    { en: { t: 'Sub Harm.' },    fr: { t: 'Sous-harm.', reviewed: false } },

    // ── Effects tab: section titles and bypass faces ────────────────────────
    'label.fx.chorus': { en: { t: 'Chorus' }, fr: { t: 'Chorus',   reviewed: false, sameAsEn: true } },
    'label.fx.delay':  { en: { t: 'Delay' },  fr: { t: 'Délai',    reviewed: false } },
    'label.fx.reverb': { en: { t: 'Reverb' }, fr: { t: 'Réverb.',  reviewed: false } },
    // The three-letter abbreviation French audio uses too. Not "Égaliseur":
    // .fx-title is `margin-right: auto` in a flex row whose knobs follow it,
    // and a 9-character legend would move all four EQ knobs.
    'label.fx.eq':     { en: { t: 'EQ' },     fr: { t: 'EQ',       reviewed: false, sameAsEn: true } },

    // The bypass button's two faces. Written through setLabel() from two
    // LITERAL call sites rather than one ternary — contract section 6, and
    // assertion 13 rejects a ternary inside a setLabel argument. "Marche" /
    // "Arrêt" rather than "Activé" / "Désactivé": the button is 9 px in a
    // 2px-8px pad, and this is the vocabulary a piece of hardware uses.
    'label.fx.on':  { en: { t: 'On' },  fr: { t: 'Marche', reviewed: false } },
    'label.fx.off': { en: { t: 'Off' }, fr: { t: 'Arrêt',  reviewed: false } },

    // ── Effects tab: the sixteen script-written knob captions ───────────────
    // Each is written by ONE literal setLabel() call at its own site. See the
    // header for why a data-driven key would have been invisible to the gates.
    'label.fx.rate':     { en: { t: 'Rate' },     fr: { t: 'Vitesse',    reviewed: false } },
    'label.fx.depth':    { en: { t: 'Depth' },    fr: { t: 'Profond.',   reviewed: false } },
    'label.fx.mix':      { en: { t: 'Mix' },      fr: { t: 'Mixage',     reviewed: false } },
    'label.fx.time':     { en: { t: 'Time' },     fr: { t: 'Durée',      reviewed: false } },
    'label.fx.feedback': { en: { t: 'Feedback' }, fr: { t: 'Réinject.',  reviewed: false } },
    'label.fx.mode':     { en: { t: 'Mode' },     fr: { t: 'Mode',       reviewed: false, sameAsEn: true } },
    'label.fx.low':      { en: { t: 'Low' },      fr: { t: 'Grave',      reviewed: false } },
    'label.fx.mid':      { en: { t: 'Mid' },      fr: { t: 'Médium',     reviewed: false } },
    'label.fx.midFreq':  { en: { t: 'Mid Freq' }, fr: { t: 'Fréq. méd.', reviewed: false } },
    'label.fx.high':     { en: { t: 'High' },     fr: { t: 'Aigu',       reviewed: false } },
    'label.fx.size':     { en: { t: 'Size' },     fr: { t: 'Taille',     reviewed: false } },
    'label.fx.damp':     { en: { t: 'Damp' },     fr: { t: 'Amortis.',   reviewed: false } },
    'label.fx.predelay': { en: { t: 'Pre-dly' },  fr: { t: 'Pré-dél.',   reviewed: false } },
    'label.fx.mod':      { en: { t: 'Mod' },      fr: { t: 'Mod',        reviewed: false, sameAsEn: true } },
    // "Chatoiement" is 11 characters and the accurate word; the knob caption
    // box is 72 px and this is a reverb parameter every French-language DAW
    // ships as "Shimmer".
    'label.fx.shimmer':  { en: { t: 'Shimmer' },  fr: { t: 'Shimmer',    reviewed: false, sameAsEn: true } },

    // The accessible name that REPLACES the native title= setupFxKnob() used to
    // write onto all sixteen FX readouts. Moved verbatim, not authored.
    'aria.fxValueEdit': {
        en: { t: 'Double-click to edit' },
        fr: { t: 'Double-cliquer pour modifier', reviewed: false },
    },

    // ── The tuning panel's load-failure notice ──────────────────────────────
    // O-Wind's OWN string, written by O-Wind's own catch block — not the shared
    // module's copy. It was injected with innerHTML inside a styled <div>; it is
    // now a createElement + setLabel, because a machine-drafted translation must
    // never open a markup path (assertion 9).
    'label.tuningFailed': {
        en: { t: 'Tuning panel failed to load.' },
        fr: { t: 'Échec du chargement du panneau d’accord.', reviewed: false },
    },
});

// ============================================================================
// I18N_EXEMPT — reasoned exclusions, never silence.
//
// An entry is [text, reason] or [text, reason, scope]. The scope is a
// comma-separated list of `tag`, `.class` or `#id` matched against the node's
// own parent and its ancestors, and it is REQUIRED exactly where a string is
// exempt AND keyed on the same page — the one state in which the gate cannot
// tell a deliberate skip from a label somebody forgot. Nothing on this page is
// in that state, so every entry below is legitimately unscoped: each text
// appears in exactly one place.
// ============================================================================

export const I18N_EXEMPT = [

    ['Ouaricon',
     'the company name — never translated. It is the LAST flex item in the preset bar with nothing after it, so no localized string shares its line and nothing re-centres when the language changes'],

    // ── D-02: a preset name IS its filename ─────────────────────────────────
    ['Default',
     'the placeholder #preset-name carries until refreshPresetDisplay() overwrites it with the loaded preset. A preset name IS the JSON filename (OuariconPresetManager.h), so translating it breaks recall: a session saved against "Concert Flute" would not resolve "Flûte de concert"'],

    // ── The eight instrument <option> captions ──────────────────────────────
    //
    // NOT an arm-1 case, and it is worth being precise about that because it
    // looks like one. `instrumentPreset` is an AudioParameterInt (0..7,
    // PluginProcessor.cpp:326), not an AudioParameterChoice, so the host
    // automation lane shows a NUMBER and there is no option string to be
    // byte-identical to.
    //
    // They are exempt on D-02 instead, and the collision is real: these eight
    // strings are BYTE-IDENTICAL to the eight FACTORY PRESET NAMES
    // (PluginProcessor.cpp:814-1045, initializeFactoryPresets), and the preset
    // browser 30 px above the strip lists those same eight names in English,
    // read straight off the JSON filenames through getPresetList(). Localizing
    // the <select> while the browser above it stays English would put two
    // languages on the same eight names inside one 900 x 600 frame.
    //
    // A FRENCH USER THEREFORE STILL READS "Concert Flute" AND "Pan Flute" IN
    // THE INSTRUMENT SELECTOR. That is the cost, it is deliberate, and it is
    // reported rather than left to be discovered.
    ['Concert Flute',    'byte-identical to a factory preset name — the name IS the JSON filename (D-02), and the preset browser above lists the same eight in English'],
    ['Shakuhachi',       'byte-identical to a factory preset name — D-02, see Concert Flute'],
    ['Bansuri',          'byte-identical to a factory preset name — D-02, see Concert Flute'],
    ['Native Am. Flute', 'byte-identical to a factory preset name — D-02, see Concert Flute'],
    ['Recorder',         'byte-identical to a factory preset name — D-02, see Concert Flute'],
    ['Pan Flute',        'byte-identical to a factory preset name — D-02, see Concert Flute'],
    ['Piccolo',          'byte-identical to a factory preset name — D-02, see Concert Flute'],
    ['Ocarina',          'byte-identical to a factory preset name — D-02, see Concert Flute'],

    // ── D-01 arm 1: the delayMode option strings ────────────────────────────
    // initializeEffects() writes these into #delayModeSelect from the literal
    // array ['Normal', 'PingPong']. They are the delayMode AudioParameterChoice
    // options VERBATIM (PluginProcessor.cpp:377), so the page and the host
    // automation lane must name them identically. Neither word is keyed
    // anywhere else on this page, so neither needs a scope.
    ['Normal',   'a delayMode AudioParameterChoice option string VERBATIM — the page and the host automation lane must name it identically (D-01 arm 1)'],
    ['PingPong', 'a delayMode AudioParameterChoice option string VERBATIM — D-01 arm 1'],

    // ── Endonyms ────────────────────────────────────────────────────────────
    ['English',  'endonym — a language name is never translated'],
    ['Français', 'endonym — a language name is never translated'],

    // ── The shared tuning module ────────────────────────────────────────────
    // O-Wind is the only plugin in batch K4 that consumes the module file BY
    // REFERENCE rather than carrying a diverged plugin-owned copy:
    // CMakeLists.txt:92 embeds
    // ${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/js/tuning-panel.js
    // straight from the module tree. Localizing it is a cross-plugin change and
    // is out of scope for a per-plugin commit; a local edit would also be
    // reverted by /module-upgrade.
    //
    // O-WIND'S TUNING TAB IS THEREFORE ENGLISH IN BOTH LANGUAGES. It is one of
    // the three tabs, so a French user meets an English page on roughly a third
    // of this plugin's navigable surface. That is a scope statement, not an
    // oversight.
    ['Tuning tab captions',
     'every caption inside the Tuning tab belongs to the SHARED module ${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine — CMakeLists.txt:92 embeds js/tuning-panel.js by reference from the module tree, not a plugin-owned copy. Localizing it is a cross-plugin change and any local edit would be reverted by /module-upgrade. Its "Scala/TUN", "MTS-ESP" and "12-TET" strings are also tuningSystem AudioParameterChoice options, so they are exempt twice over'],
];

// ============================================================================
// TIP_BINDINGS — EMPTY. This plugin has no hover-help: v1.16.3 carried no
// data-tip anywhere, and authoring that copy is Stage M's job.
//
// Exported because the canonical import line names it and applyI18n() iterates
// it. A zero-length loop is the correct no-op; omitting the export and editing
// the canon block to match would put this plugin's copy of the runtime out of
// step with the other forty-plus, which is the whole drift the canon gate
// exists to prevent.
// ============================================================================

export const TIP_BINDINGS = [];

// The tooltip lookup. Returns {t, b} — never null, never a bare key without a
// console.warn saying so, because a silently-missing tip renders as an empty
// surface that looks like a positioning bug rather than a missing entry.
//
// Unreferenced at runtime today: applyI18n() calls it only from the
// TIP_BINDINGS loop, which is empty. It is exported verbatim all the same, so
// that the canon block stays byte-identical to every other copy and Stage M can
// add bodies to I18N without touching this file's shape.
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
