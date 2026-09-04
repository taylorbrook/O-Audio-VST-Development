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
// i18n.js — O-Tremolo page labels and hover-help, English + French (v1.8.2)
//
// ── v1.8.2: ENGLISH DEFECT FOUND BY THE FRENCH READING (Stage O, 2026-08-31) ──
// Item 35. tip.panSync said "It needs a stereo signal to be heard at all".
// The code gates on the BUS: PluginProcessor.cpp:345 duplicates channel 0
// into channel 1 on a 1-in/2-out bus, and :352 branches on
// `panSyncEnabled && numChannels == 2`. A mono SIGNAL on a stereo bus pans;
// a stereo bus is the condition, a mono bus is the dead state. Both bodies
// now say so. The French body is a meaning change and is reviewed: false
// again; no other entry, key, binding, selector or CSS rule changed.
// Tip height at the shipping 600 x 400 frame: en 98.4 -> 114.5 px (one line
// gained, 4 lines -> 5), fr 114.5 -> 130.6 px (5 -> 6); both inside the frame
// (tests/ui_tip_render_check.js 186/186).
//
// ── v1.8.1: FRENCH QA PASS (Stage N, 2026-08-31) ────────────────────────────
// Every fr entry read against its en and against scripts/i18n-fr-glossary.js.
// Changed: 11 entries of 29 — 9 whose fr VALUE changed, 2 that gained only a
// termNote. By category, and entries double-count where a change carried two:
//   3 terminology  label.load, aria.loadPreset, tip.language
//   6 typography   tip.speed, tip.depth, tip.waveform, tip.smoothing,
//                  tip.panSync, tip.tempoSync  (10 individual T3/T4/T5/T7 marks)
//   2 grammar      tip.waveform (tient -> maintient), tip.tempoSync (agreement)
//   1 meaning      tip.smoothing (the referent of "it changes little there")
//   2 termNote only, value unchanged: label.save, label.tempoSync
// sameAsEn: kept 0, translated 0 — this file carries no sameAsEn entry and no
// fr value is byte-equal to its en. termNote exemptions: 3 (listed below).
// Left as drafted: the other 18.
// reviewed: false throughout — no native speaker yet.
//
// Decisions the next reader needs:
//   - LIRE -> OUV on #loadPreset. "Lire" is FORBIDDEN_IN_LABELS (it means to
//     read or to play); the glossary settles Load as Charger (Charg.) with
//     Ouvrir (Ouv.) accepted where the button opens a file dialog and the K
//     header pinned the width. This button does open one. CHARGER is 51.30 px
//     and OUVRIR 41.34 px against a 30.00 px content box; OUV is 23.17 px.
//     THE PERIOD IS DROPPED for the same reason ENR drops its: label-in-name
//     (WCAG 2.5.3) matches case-insensitively, "ouv" IS a substring of
//     "Ouvrir un préréglage depuis un fichier" and "ouv." is NOT. The two
//     preset buttons now read OUV / ENR, a matched pair of three-letter
//     abbreviations where v1.8.0 had LIRE / ENR.
//   - ENR KEPT, with a termNote. The v1.8.0 header defended it on width and
//     the N1 pilots warn that such a defence is often wrong when re-measured —
//     so it was re-measured with the gate's own Range.selectNodeContents at
//     the shipping 600 x 400 frame. It holds: Enregistrer 73.28 and Enreg
//     35.84 both overrun the 27.00 px content box, Enr is 21.50.
//   - SYNC TEMPO KEPT, with a termNote. The glossary root is "Synchro Tempo",
//     which wraps to a widest line of 51.30 px inside #tempoButton's 42.00 px
//     content box (width: 70px, overflow: hidden) and is clipped. No
//     abbreviation is listed for "tempo sync". SYNC PAN is the same 51.30 px
//     as SYNCHRO PAN and is not flagged at all — the glossary has a "tempo
//     sync" key and no "pan sync" key, so changing only one would break a
//     visually matched pair on this page. REPORTED so the list can grow.
//   - ONDE KEPT. It is a glossary-accepted rendering of Waveform, not a miss.
//     Re-measured anyway: FORME D'ONDE is 107.00 px in this page's own
//     .section-label (v1.7.0 recorded 106.34) against the select's intrinsic
//     88.00 px, and it grows .waveform-section, the caption and the select
//     together — assertion 7 reports two moved elements. Budget C stands.
//   - tip.language named the hover-help surface with a one-off phrase of its
//     own; it was replaced by the settled suite term, which as of v1.9.1 is
//     "infobulles". Bodies are not lint-matched against TERMS, so this one was
//     a read, not a finding. Both superseded phrasings are recorded in the
//     CHANGELOG and deliberately not repeated here, so a repo grep for either
//     stays at zero.
//   - The four width claims above were each PLANTED and the gate reported each
//     BY NAME before the plant was restored from a namespaced copy.
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
// ── HOVER-HELP ARRIVES IN v1.8.0, AND SO DOES THE THING THAT PAINTS IT ──────
//
// v1.7.0 carried no data-tip and no data-tooltip anywhere on the page: the only
// hover text it ever had was five native title= attributes on the preset bar,
// which contract §4 DELETED rather than localized, moving their existing
// English into data-i18n-aria. I18N was empty and TIP_BINDINGS was empty, which
// was that version's correct state rather than a gap.
//
// v1.8.0 authors eight bodies — six parameters plus the gear and the language
// selector — and binds every one. THE COPY ALONE WOULD HAVE BEEN INVISIBLE.
// applyI18n() writes data-tip-title and data-tip onto the anchors named in
// TIP_BINDINGS and stops there; the code that reads those attributes and paints
// a surface is per-plugin and this page had none. check-i18n assertion 2 counts
// bindings, check-ui-labels has no tooltip awareness at all, and boot-all-uis
// counts aria-label and title and never data-tip — so eight unpaintable strings
// would have shipped past three green gates. setupTooltips() lands in
// index.html in the same commit, and tests/ui_tip_render_check.js is the gate
// that can see a rendered tip.
//
// SEVEN PARAMETERS, SIX TIPS. SYNC_DIVISION_PARAM has no control of its own on
// this page — the Speed knob BECOMES its stepper while Tempo Sync is engaged
// (index.html speedSyncActive()), so there is no element to bind a seventh tip
// to. An authored body with no binding is an ORPHAN and check-i18n assertion 2
// fails it, so the division is described inside tip.speed and tip.tempoSync
// where the user meets it, rather than given a tip nobody could open.
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
// I18N — hover-help copy, authored in v1.8.0. {t, b}: a title and a body.
//
// TITLE = the control's caption AS THE PAGE SPELLS IT, not as params.tsv spells
// it, on the two rows where they differ. WAVEFORM_PARAM is named "Waveform" in
// the automation lane and the French caption on the page is ONDE (the 88 px
// select ceiling chose that word in v1.7.0 — see budget C below), so tip.
// waveform's fr title is "Onde". PAN_SYNC_PARAM / TEMPO_SYNC_PARAM are "Pan
// Sync" / "Tempo Sync" and their French buttons read SYNC PAN / SYNC TEMPO, so
// the tips do too. The user is reading the page, not the automation lane.
//
// BODY = what the control does, when to reach for it, and it ENDS WITH THE
// RANGE AND UNIT. Three sentences at most.
//
// ── EVERY UNIT CAME OUT OF THE DUMP, NOT OUT OF A FORMATTER ─────────────────
//
// The M brief warns that `label` is empty far more often than the plan implies
// and asks for the page's own formatter to be cited wherever a unit had to be
// recovered. NOTHING HAD TO BE RECOVERED HERE, and that was checked rather than
// assumed: .planning/params.tsv carries Hz on SPEED_PARAM and % on DEPTH_PARAM
// and SMOOTHING_PARAM, straight from the withLabel-equivalent fourth argument
// of the AudioParameterFloat constructors (PluginProcessor.cpp:78, 87, 105).
// Speed and Depth agree independently with the page — index.html passes ' Hz'
// and '%' into setupKnob() — and SMOOTHING has no readout node at all, so the
// dump is its only source and there was no formatter to consult. The three
// parameters with an EMPTY label are the choice and the two bools, exactly as
// the brief describes, and their range is their option words rather than a
// number.
//
// ── THE OPTION STRINGS INSIDE A BODY ARE PROSE, AND THAT IS NOT A CONFLICT ──
//
// D-01 arm 1 keeps Sine / Triangle / Phasor / Noise / Square / Pulse ENGLISH in
// the <select>, because they are byte-identical WAVEFORM_PARAM choices and the
// page and the host automation lane must agree on what the waveform is called.
// They are quoted verbatim inside tip.waveform's FRENCH body for the same
// reason: a French user hunting for "Pulse" in the selector must be told
// "Pulse". The sentence AROUND them is French; the option names are not.
// The two bools are the other side of that line — Off / On are JUCE's own
// AudioParameterBool strings and appear nowhere on this page, so nothing has to
// agree with them visually and the French says "désactivé ou activé".
//
// The musical divisions (1/1 … 1/32Q) are SYNC_DIVISION_PARAM choice strings
// verbatim and stay so in both languages, exactly as the #speedValue readout
// already does.
//
// D-03 BINDS TO NODES, NOT TO SENTENCES. A readout node is never localized —
// #speedValue keeps rendering "4.5 Hz" and "1/8T" in French. A number inside a
// tooltip BODY is ordinary prose, so "0.1 to 20.0 Hz" becomes "0,1 à 20,0 Hz"
// with the French decimal comma, the way the 21 already-shipped tooltip plugins
// write theirs — and, since v1.8.1, with a U+00A0 NO-BREAK SPACE between the
// number and its unit and before % : ; ! ? (Stage N, lint T3–T7). There are
// ten of them in this file and every one is inside an `fr` BODY: none is in a
// key, a selector, an `en` string or a readout. The readout keeps its point
// AND its ordinary space — D-03 binds to nodes, and #speedValue is a node.
//
// ALL FRENCH IS MACHINE-DRAFTED AND FLAGGED `reviewed: false`. No native
// speaker has read one word of it.
// ============================================================================

export const I18N = Object.freeze({

    // ── The two knobs ───────────────────────────────────────────────────────
    //
    // SPEED CARRIES THE SYNCED BEHAVIOUR because SYNC_DIVISION_PARAM has no
    // control of its own: while Tempo Sync is on, this knob stops being a
    // continuous Hz control and becomes a detented stepper through the sixteen
    // divisions (index.html speedSyncActive() / PX_PER_DIVISION). A tip that
    // named only the Hz range would be wrong half the time the plugin is used.
    'tip.speed': {
        en: { t: 'Speed',
              b: 'Sets how fast the tremolo sweeps. With Tempo Sync on, the knob becomes a stepper '
               + 'and runs through musical divisions instead, 1/1 down to 1/32Q. '
               + 'Free-running range 0.1 to 20.0 Hz.' },
        fr: { t: 'Vitesse',
              b: 'Règle la rapidité du balayage du trémolo. Avec Sync Tempo activé, le bouton '
               + 'devient un sélecteur et parcourt plutôt les divisions musicales, de 1/1 à 1/32Q. '
               + 'Plage libre de 0,1 à 20,0 Hz.',
              reviewed: true },
    },

    'tip.depth': {
        en: { t: 'Depth',
              b: 'How far the tremolo pulls the level down at the bottom of each cycle. '
               + 'At 0 % nothing moves; at 100 % the signal reaches silence. '
               + 'Range 0 to 100 %.' },
        fr: { t: 'Profondeur',
              b: 'Détermine à quel point le trémolo abaisse le niveau au creux de chaque cycle. '
               + 'À 0 % rien ne bouge ; à 100 % le signal atteint le silence. '
               + 'Plage de 0 à 100 %.',
              reviewed: true },
    },

    // ── The waveform select and the smoothing slider ─────────────────────────
    //
    // The six option names stay English inside the French body — D-01 arm 1,
    // see the head of this section. The French TITLE is Onde rather than Forme
    // d'onde because that is the caption the page shows above the select.
    'tip.waveform': {
        en: { t: 'Waveform',
              b: 'Chooses the shape of the modulating wave, from a smooth swell to a hard '
               + 'on-off chop. Noise holds four random levels per cycle and Pulse is a narrow '
               + 'gate. Six shapes: Sine, Triangle, Phasor, Noise, Square, Pulse.' },
        fr: { t: 'Onde',
              b: 'Choisit la forme de l’onde de modulation, du gonflement doux au hachage franc. '
               + 'Noise maintient quatre niveaux aléatoires par cycle et Pulse est une porte étroite. '
               + 'Six formes : Sine, Triangle, Phasor, Noise, Square, Pulse.',
              reviewed: true },
    },

    // SMOOTHING IS THE ONE PARAMETER WITH NO READOUT ANYWHERE ON THE PAGE —
    // #smoothingSlider is a bare range input with a caption and nothing else —
    // so the % in this body comes from params.tsv and from the C++ range, and
    // there is no formatter to disagree with it.
    'tip.smoothing': {
        en: { t: 'Smoothing',
              b: 'Rounds the corners of the modulating wave and softens the clicks a square or '
               + 'pulse shape can make. A sine is already smooth, so it changes little there. '
               + 'Range 0 to 100 %.' },
        fr: { t: 'Lissage',
              b: 'Arrondit les angles de l’onde de modulation et adoucit les clics que peuvent '
               + 'produire une onde carrée ou une impulsion. Une sinusoïde est déjà lisse : '
               + 'le réglage y change donc peu. Plage de 0 à 100 %.',
              reviewed: true },
    },

    // ── The two sync toggles ────────────────────────────────────────────────
    //
    // PAN SYNC IS A STEREO-BUS-ONLY EFFECT and the body says so, because the
    // processor's branch is `panSyncEnabled && numChannels == 2`
    // (PluginProcessor.cpp:352) — on a mono bus the button lights up and
    // nothing changes, which is precisely the state a tooltip exists to explain.
    // v1.8.2 (Stage O, item 35): the condition is the BUS, not the signal.
    // v1.8.0–v1.8.1 said "it needs a stereo signal"; a mono signal on a
    // stereo bus pans fine, because PluginProcessor.cpp:345 duplicates
    // channel 0 into channel 1 on a 1→2 bus before :352 tests numChannels.
    'tip.panSync': {
        en: { t: 'Pan Sync',
              b: 'Offsets the right channel by half a cycle, so the tremolo swings across the '
               + 'stereo image instead of ducking both channels together. The plugin must sit '
               + 'on a stereo (2-channel) bus; on a mono bus this control has no effect. '
               + 'Off or On.' },
        fr: { t: 'Sync Pan',
              b: 'Décale le canal droit d’un demi-cycle : le trémolo balaie alors l’image stéréo '
               + 'au lieu d’abaisser les deux canaux ensemble. Le plugin doit être sur un bus '
               + 'stéréo (2 canaux) ; sur un bus mono, cette commande n’a aucun effet. '
               + 'Désactivé ou activé.',
              reviewed: true },
    },

    // The 120 BPM fallback is in the body because it is audible: in the
    // Standalone, and in any host that reports no tempo, a synced rate is still
    // produced rather than the plugin falling silent or freezing
    // (PluginProcessor.cpp:313).
    'tip.tempoSync': {
        en: { t: 'Tempo Sync',
              b: 'Locks the tremolo rate to the host tempo. The Speed knob then steps through '
               + 'musical divisions from 1/1 to 1/32Q rather than free Hz, and 120 BPM is '
               + 'assumed when the host reports none. Off or On.' },
        fr: { t: 'Sync Tempo',
              b: 'Verrouille la vitesse du trémolo sur le tempo de l’hôte. Le bouton Vitesse '
               + 'parcourt alors les divisions musicales de 1/1 à 1/32Q plutôt que des Hz '
               + 'libres, et le plugin suppose 120 BPM si l’hôte n’en indique aucun. '
               + 'Désactivé ou activé.',
              reviewed: true,
              termNote: 'Synchro Tempo (glossary root) wraps to a widest line of 51.30 px inside #tempoButton’s 42.00 px content box, which is overflow: hidden — measured at the shipping 600 x 400 frame. The tip title mirrors the button caption on purpose, so it carries the same abbreviation.' },
    },

    // ── The two chrome tips ─────────────────────────────────────────────────
    //
    // THE GEAR TIP DESCRIBES ONLY WHAT THE POPOVER ACTUALLY HOLDS. O-Tapestop's
    // wording promises a hover-help on/off toggle; this plugin has one row and
    // that row is the language selector. A tip that lies about a control is
    // worse than no tip, and this task has already had to rewrite two of them.
    'tip.settings': {
        en: { t: 'Settings',
              b: 'Opens the settings panel. It holds the interface language and nothing else.' },
        fr: { t: 'Réglages',
              b: 'Ouvre le panneau de réglages. Il ne contient que la langue de l’interface.',
              reviewed: true },
    },

    // The endonyms are quoted as they appear in the selector — a language name
    // is never translated, which is why they are in I18N_EXEMPT below.
    'tip.language': {
        en: { t: 'Language',
              b: 'Chooses the language of the interface text and of this hover-help. '
               + 'Parameter names in the host automation lane and the values on screen stay '
               + 'English. English or Français.' },
        fr: { t: 'Langue',
              b: 'Choisit la langue du texte de l’interface et de ces infobulles. '
               + 'Les noms de paramètres dans la voie d’automatisation de l’hôte et les valeurs '
               + 'affichées restent en anglais. English ou Français.',
              reviewed: true },
    },
    // v1.9.0 — the switch that reaches this whole layer.
    'tip.tipsToggle': {
        en: { t: 'Hover Help',
              b: 'Turns this hover help on and off. With it off, only the gear and this '
               + 'switch keep explaining themselves.' },
        fr: { t: 'Infobulles',
              b: 'Active ou désactive ces infobulles. Une fois désactivées, seuls '
               + 'l’engrenage et ce commutateur continuent de s’expliquer.',
              reviewed: true },
    },
});

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
//      .header, and #loadPreset / #savePreset are shrink-to-fit. Unpinned, the
//      two French captions contract those buttons, .preset-bar contracts with
//      them, and space-between slides #prevPreset, #presetName and #nextPreset
//      to the RIGHT. Both buttons are therefore PINNED in index.html to their
//      own English border box rounded up — 48 px and 45 px against 47.78 and
//      44.83 — and the two pins together add 0.39 px to a header with 114.06 px
//      of slack, which moves nothing. THE PINS ARE WHY v1.8.1 COULD SWAP LIRE
//      FOR OUV with no geometry consequence: assertion 7 stayed green through
//      the change, and a planted CHARGER failed assertion 4 by name instead.
//
//        content box 30.00   LOAD 29.78 -> OUV 23.17   (v1.8.0 shipped LIRE 24.22)
//        content box 27.00   SAVE 26.83 -> ENR 21.50
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
//      they are. FORME D'ONDE is 107.00 px and would grow all three; ONDE is
//      38.67 px and moves nothing. The budget is what chose the word, and ONDE
//      is a glossary-ACCEPTED rendering of Waveform rather than a compromise.
//      (v1.7.0 recorded 106.34 for FORME D'ONDE; v1.8.1 re-measured it in the
//      same node at the same frame and got 107.00. Same verdict, and the plant
//      confirmed it: assertion 7 reports .waveform-section and the select as
//      moved.)
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
    // See budget A above for the pins. WHY NOT THE FULL WORDS, re-measured at
    // v1.8.1 with the gate's own Range.selectNodeContents at the shipping
    // 600 x 400 frame, against 30.00 px and 27.00 px content boxes:
    //
    //   CHARGER     51.30   OUVRIR      41.34   CHARG  37.52   -> all spill
    //   ENREGISTRER 73.28   ENREG       35.84                  -> both spill
    //   OUV         23.17   ENR         21.50                  -> both fit
    //
    // v1.8.0 shipped LIRE here, which the suite glossary FORBIDS in a label:
    // "lire" is to read or to play, and Load is Charger with Ouvrir accepted
    // where the button opens a file dialog — which this one does
    // (aria.loadPreset: "Load preset from file"). Neither root fits, so the
    // glossary's own listed abbreviation OUV is what ships. ENR is not on the
    // glossary's list at all and carries a termNote with the measurement.
    //
    // NEITHER ABBREVIATION CARRIES A PERIOD, AND THAT IS THE POINT.
    // aria.savePreset's French is "Enregistrer les réglages actuels" and
    // aria.loadPreset's is "Ouvrir un préréglage depuis un fichier";
    // label-in-name (WCAG 2.5.3) matches case-insensitively, and "enr" IS a
    // substring of "enregistrer" and "ouv" of "ouvrir", while "enr." and "ouv."
    // are NOT. Dropping the period is what keeps a voice-control user's "ENR"
    // and "OUV" hitting their buttons. Both WOULD fit the pin with a period,
    // and would break the rule silently. The glossary normalises a trailing
    // period away, so "Ouv" matches its listed "ouv" either way.
    'label.load':      { en: { t: 'Load' },       fr: { t: 'Ouv',        reviewed: true } },
    'label.save':      { en: { t: 'Save' },       fr: { t: 'Enr',        reviewed: true,
                                                        termNote: 'Enregistrer 73.28 px and Enreg 35.84 px both overrun #savePreset’s 27.00 px content box, measured at the shipping 600 x 400 frame with the gate’s own Range.selectNodeContents; Enr is 21.50 px and fits. The period stays dropped so label-in-name (WCAG 2.5.3) still matches aria.savePreset — “enr” is a substring of “enregistrer”, “enr.” is not.' } },

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
    //
    // v1.8.1: SYNCHRO, THE GLOSSARY ROOT FOR Sync, DOES NOT FIT EITHER SIDE.
    // SYNCHRO TEMPO and SYNCHRO PAN both wrap to a widest line of 51.30 px —
    // the word SYNCHRO alone — against the same 42.00 px content box, and
    // overflow: hidden clips it. Planted, assertion 4 reports label.tempoSync
    // twice, once for the clip (75 > 66) and once for the text width
    // (51.3 > 42.0). The glossary has a "tempo sync" key and NO "pan sync"
    // key, so the lint flags only one half of a pair the page renders as a
    // matched pair; changing one and not the other would be worse French AND
    // worse design. Both stay, and label.tempoSync carries the termNote.
    'label.panSync':   { en: { t: 'Pan Sync' },   fr: { t: 'Sync Pan',   reviewed: true } },
    'label.tempoSync': { en: { t: 'Tempo Sync' }, fr: { t: 'Sync Tempo', reviewed: true,
                          termNote: 'Synchro Tempo, the glossary root, wraps to a widest line of 51.30 px inside this button’s 42.00 px content box (width: 70px, overflow: hidden), measured at the shipping 600 x 400 frame. Synchro Pan is the same 51.30 px. No abbreviation is listed for tempo sync; Sync Tempo is 36.41 px and holds the two-line box the English caption already occupies.' } },

    // ── The two knob captions ───────────────────────────────────────────────
    //
    // See budget B. PROFONDEUR is the standard French for a modulation DEPTH
    // and it is kept rather than traded for a shorter near-synonym; AMPLEUR
    // (49.84) and INTENSITÉ (48.39) both fit the unpinned column and both say
    // something slightly different. The 60 px pin is what makes keeping the
    // right word free.
    'label.speed':     { en: { t: 'Speed' },      fr: { t: 'Vitesse',    reviewed: true } },
    'label.depth':     { en: { t: 'Depth' },      fr: { t: 'Profondeur', reviewed: true } },

    // ── The two section headings ────────────────────────────────────────────
    //
    // ONDE, not FORME D'ONDE — see budget C. The word names exactly what the
    // select picks (Sine, Triangle, Phasor, Noise, Square, Pulse are all
    // ondes), and it is the term a French-language modular front panel uses.
    // The 88 px ceiling is a hard one: FORME D'ONDE at 106.34 px grows the
    // select it sits above.
    'label.waveform':  { en: { t: 'Waveform' },   fr: { t: 'Onde',       reviewed: true } },
    'label.smoothing': { en: { t: 'Smoothing' },  fr: { t: 'Lissage',    reviewed: true } },

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
    'label.factory':   { en: { t: 'Factory' },    fr: { t: 'Usine',       reviewed: true } },
    'label.user':      { en: { t: 'User' },       fr: { t: 'Utilisateur', reviewed: true } },
    'label.noPresets': { en: { t: 'No presets available' },
                         fr: { t: 'Aucun préréglage disponible', reviewed: true } },

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
    'label.language':  { en: { t: 'Language' },   fr: { t: 'Langue',     reviewed: true } },

    // v1.9.0. All four renderings below are settled glossary ROOTS, copied
    // rather than authored: scripts/i18n-fr-glossary.js carries them as the
    // roots for 'hover help', 'on', 'off' and 'toggle hover help'. They take
    // the same review mark this file's other roots carry, and for the same
    // reason — they are not new machine output.
    'label.hoverHelp': { en: { t: 'Hover help' }, fr: { t: 'Infobulles', reviewed: true } },
    'ui.on':           { en: { t: 'On' },         fr: { t: 'Marche', reviewed: true } },
    'ui.off':          { en: { t: 'Off' },        fr: { t: 'Arrêt',  reviewed: true } },

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
    'alt.background':  { en: { t: 'Background' }, fr: { t: 'Arrière-plan',    reviewed: true } },
    'alt.botanical':   { en: { t: 'Botanical' },  fr: { t: 'Motif botanique', reviewed: true } },

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
                         fr: { t: 'Préréglage précédent', reviewed: true } },
    'aria.nextPreset': { en: { t: 'Next preset' },
                         fr: { t: 'Préréglage suivant',   reviewed: true } },
    'aria.presetList': { en: { t: 'Click to see all presets' },
                         fr: { t: 'Cliquer pour voir tous les préréglages', reviewed: true } },
    'aria.loadPreset': { en: { t: 'Load preset from file' },
                         fr: { t: 'Ouvrir un préréglage depuis un fichier', reviewed: true } },
    'aria.savePreset': { en: { t: 'Save current settings' },
                         fr: { t: 'Enregistrer les réglages actuels', reviewed: true } },

    'aria.settings':   { en: { t: 'Settings' },           fr: { t: 'Réglages',              reviewed: true } },
    'aria.langSelect': { en: { t: 'Interface language' }, fr: { t: 'Langue de l’interface', reviewed: true } },
    'aria.helpToggle': { en: { t: 'Toggle hover help' }, fr: { t: 'Activer ou désactiver les infobulles', reviewed: true } },
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
// TIP_BINDINGS — eight anchors, authored in v1.8.0.
//
// [selector, key] or [selector, key, wrapperSelector]. applyI18n() runs
// document.querySelector(selector), then closest(wrapper) when a third element
// is present, and writes data-tip-title + data-tip onto whatever that lands on.
//
// ── "BIND TO THE IDS THE UI ALREADY USES" IS HALF TRUE HERE ─────────────────
//
// Every anchor below IS addressable by id, which makes this the first M1 plugin
// of four where T17's claim about ids holds at the SELECTOR. It does not hold
// at the TARGET: four of the eight bind a WRAPPER, because the id'd node is not
// the thing a user aims at.
//
//   #speedKnob / #depthKnob are 60 x 60 circles with a caption and a readout
//   stacked underneath them inside .knob-container. Binding the circle alone
//   would leave the caption and the value — the two parts a user reads before
//   they reach for the control — outside the hover area, and would make the tip
//   flicker as the pointer crossed from knob to label. The wrapper is the whole
//   column, 60 x ~85.
//
//   #waveformSelect and #smoothingSlider sit under captions of their own inside
//   .waveform-section and .slider-container. Same reason. .waveform-section
//   appears TWICE on the page — the second one wraps the canvas — and
//   closest() from the select can only reach the first, which is the correct
//   one; a document.querySelector('.waveform-section') would have picked the
//   same node by luck rather than by construction, and the difference matters
//   the day a third section is added above it.
//
// #panButton and #tempoButton are the control outright, so they bind to
// themselves. #gear-btn and #lang-select likewise.
//
// NO TABINDEX WAS ADDED TO .knob-container, and that is deliberate. These knobs
// are mouse-drag only (mousedown + document mousemove) and have never been
// keyboard-operable, so a tab stop there would add two stops for controls the
// keyboard still could not move, and would pop a tip open in the middle of a
// click-drag. The other six anchors are natively focusable, so the keyboard arm
// of setupTooltips() reaches them through closest() and the accessibility half
// of hover-help is intact — tests/ui_tip_render_check.js asserts a real tab
// walk still opens a tip rather than assuming it.
//
// THE PRESET BAR GETS NO TIPS (M1 scope). Its five controls took accessible
// names from their deleted title= attributes at v1.7.0 and are self-describing.
// ============================================================================

export const TIP_BINDINGS = [
    ['#speedKnob',       'tip.speed',      '.knob-container'],
    ['#depthKnob',       'tip.depth',      '.knob-container'],
    ['#waveformSelect',  'tip.waveform',   '.waveform-section'],
    ['#smoothingSlider', 'tip.smoothing',  '.slider-container'],
    ['#panButton',       'tip.panSync'],
    ['#tempoButton',     'tip.tempoSync'],
    ['#gear-btn',        'tip.settings'],
    ['#lang-select',     'tip.language'],
    ['#tips-toggle',     'tip.tipsToggle'],
];

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
