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
// i18n.js — O-DigiDelay page labels and hover-help, English + French (v1.4.1)
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
// ── v1.4.1: FRENCH QA PASS (Stage N, 2026-08-31) ───────────────────────────
// Every fr entry read against its en and against scripts/i18n-fr-glossary.js.
// Changed: 20 entries (8 terminology, 9 typography, 0 grammar/agreement,
// 3 meaning). sameAsEn: kept 1, translated 0. termNote exemptions: 0.
// Left as drafted: the rest. reviewed: false throughout — no native speaker yet.
// scripts/i18n-fr-lint.js --plugin O-DigiDelay: 40 findings -> 0, --strict exit 0.
//
// THE DECISIONS THE NEXT READER NEEDS:
//
//   TEMPS -> DURÉE. The glossary settles `time` as *durée* and DURÉE measures
//     34.00 px against the 60 px .knob-label pin (TEMPS was 33.02), so the
//     v1.4.0 header's meaning defence — "DUREE is a duration rather than a
//     position in time" — was overruled by the suite list, not by width. The
//     control IS a duration: 1 to 2000 ms, or a note value under SYNC.
//   ÉCART -> ÉTAL., and this is a WIDTH decision. *Étalement* is the glossary
//     root and it measures 60.47 px against the 60 px pin — a single word, so
//     it SPILLS rather than wraps, and 0.47 over is a pass by the gate's 0.5
//     tolerance rather than by fit. ÉTAL. is 28.53. Same trade as RÉINJ.
//     (RÉINJECTION 66.30) and the body opens "Étalement :" the way
//     tip.feedback opens "Réinjection :".
//   EFFET -> TRAITÉ. The glossary root FITS here — 36.80 px against 60 — so
//     the WET/DRY pair is now the settled TRAITÉ / DIRECT. tip.dry's body
//     names the caption and moved with it ("Indépendant de TRAITÉ"), which
//     also retired an EFFET/effet collision the draft had in one sentence.
//   SAUVER -> Enreg, CHARGER -> Charger. *Sauver* is a calque and forbidden;
//     *Enregistrer* needs 66.52 px against .preset-action-btn's 48 px content
//     box, so the glossary's abbreviation ships. NO TRAILING PERIOD, and that
//     is label-in-name (WCAG 2.5.3), not taste: `Enreg` is a substring of
//     "Enregistrer les réglages actuels" and `Enreg.` is not. The knob
//     captions keep their periods (RÉINJ., ÉTAL.) because no accessible name
//     depends on their stem. Both captions are now SENTENCE CASE to match the
//     English "Load"/"Save" they replace (lint C1); .preset-action-btn is
//     text-transform: uppercase, so the screen is byte-identical either way
//     and only the lint and the accessible name can see the difference.
//   aria.loadPreset: *Ouvrir* -> *Charger*. Both are glossary-accepted for
//     `load`, but the caption above it is CHARGER, so "Ouvrir un préréglage
//     depuis un fichier" did not CONTAIN its own visible label — label-in-name
//     was broken in French only. The LABEL IN NAME note below claimed
//     "Charger un préréglage depuis un fichier"; the code said Ouvrir. The
//     note was right and the string was wrong. This also breaks the
//     byte-identity with O-Detune's five aria strings that the note records:
//     three of the five now differ, deliberately.
//   aria.presetList: *Cliquez* -> *Cliquer*. An accessible NAME is a name, and
//     the glossary's register rule makes names infinitive, not conjugated.
//   BODIES, three meaning-side changes: "durée de retard" / "la durée du
//     retard" -> "de délai" / "du délai" (the glossary settles `delay` as
//     *délai*; *ligne à retard* stays, it is the standard French for a delay
//     LINE); "un réglage de dosage" -> "un réglage Mix" (*Dosage* is the
//     forbidden rendering of Mix, and this plugin has no mix control — the
//     sentence exists to say WET is not half of one).
//   TYPOGRAPHY: 9 straight apostrophes -> U+2019, 7 colons and 1 semicolon and
//     5 `%` and 4 number-unit gaps given U+00A0. Applied by exact-literal
//     replacement, one pair per value, each asserted to occur exactly once —
//     no regex over the file. Audited after: every U+00A0 sits on a `t:` or
//     `b:` line, and re-importing both revisions shows 0 `en` values changed.
//
// GEOMETRY, re-measured at the shipping 700 x 196 frame with the gate's own
// Range.selectNodeContents method on the real node (correction 4 — three
// header width defences in this task have measured backwards):
//
//     TIME      25.20 -> DURÉE   34.00   26.00 spare
//     FEEDBACK  53.91 -> RÉINJ.  31.56   28.44 spare
//     SPREAD    39.31 -> ÉTAL.   28.53   31.47 spare   SHRANK
//     MOD       23.41 -> MOD     23.41   sameAsEn
//     WET       21.89 -> TRAITÉ  36.80   23.20 spare
//     DRY       20.91 -> DIRECT  37.31   22.69 spare
//     Load      27.00 -> Charger 46.52    1.48 spare   (48 px content box)
//     Save      24.34 -> Enreg   32.50   15.50 spare   SHRANK vs SAUVER 39.02
//
// Non-label elements moved between the languages: 0 before, 0 after.
// tests/ui_tip_render_check.js 216/216 both runs. Two tips grew a line in
// French — #spread-container 92 -> 108 px (the "Étalement :" opener) and
// #dry-container 77 -> 92 px — neither exceeding the 108 px that #sync, #time,
// #feedback, #gear-btn and #lang-select already rendered at v1.4.0, so no tip
// parks lower on the page than one that already shipped.
//
// ── v1.4.0 GIVES THIS PLUGIN HOVER-HELP, AND A RENDERER TO PAINT IT ────────
//
// v1.3.0 shipped an EMPTY I18N and an EMPTY TIP_BINDINGS, which was that
// version's correct state: the page carried no data-tip and no data-tooltip
// anywhere, so there was no tooltip copy to move and none was invented.
// v1.4.0 authors it — NINE entries, seven controls plus the gear and the
// language selector.
//
// AUTHORING COPY ALONE WOULD HAVE SHIPPED NINE INVISIBLE STRINGS. applyI18n()
// only WRITES data-tip-title and data-tip onto the anchors named below; the
// thing that reads those attributes and paints a surface is per-plugin code,
// and this plugin had none of it — no #tooltip element, no .tooltip rule, no
// hover handler. All three repo gates would have stayed green anyway:
// check-i18n assertion 2 sees bindings > 0, check-ui-labels has no tooltip
// awareness at all, and boot-all-uis counts aria-label and title and never
// data-tip. So the renderer lands in the same commit (index.html,
// setupTooltips()), and tests/ui_tip_render_check.js is the gate that can
// actually see a painted tip.
//
// ── WHAT THE BODIES DO NOT RESTATE ─────────────────────────────────────────
//
// The Stage K inventory found FOUR js-prose strings and ONE js-composed string
// on this page, and all five are already resolved — no tip body repeats them as
// if they were new copy:
//
//   label.on / label.off        the two faces of #sync, written by
//                               setupToggle() through setLabel(). tip.sync ends
//                               with those two words as its RANGE ("Off or On"
//                               / "Arret ou Marche"), spelled exactly as the
//                               page's own faces spell them, so the tip and the
//                               control agree.
//   label.presets /             the preset dropdown's header and its empty
//   label.noPresets             line, written by showPresetDropdown() through
//                               setLabel(). The preset bar gets NO tips in this
//                               stage, so nothing here touches them.
//   `${Math.round(1.0 + normalized * 1999.0)} ms`
//                               the delay-time readout, the page's one COMPOSED
//                               string (index.html, setupTimeKnob). It is a
//                               readout and D-03 exempts it outright. tip.time
//                               ends "1 to 2000 ms" — the same numbers as PROSE
//                               inside a localized body, which is a different
//                               thing from the readout NODE and is localized
//                               ("1 a 2000 ms") exactly as the 21 shipped
//                               tooltip plugins do it.
//
// tip.langSelect is where the user is told that readouts, the note divisions
// and preset names stay in English. That sentence exists once, in one entry,
// rather than being repeated into every knob's body.
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
// I18N — hover-help copy. {en:{t, b}, fr:{t, b, reviewed}}.
//
// NINE entries: the seven controls the page actually carries, plus the gear and
// the language selector. The preset bar deliberately gets none — its five
// controls already carry accessible names (data-i18n-aria, v1.3.0) and are
// self-describing — and the OUT meter gets none either: it is a readout, not a
// control, and D-03 keeps readouts out of the localized set.
//
// ── SEVEN CONTROLS FOR EIGHT PARAMETERS, AND THE EIGHTH IS NOT A GAP ───────
//
// .planning/params.tsv dumps EIGHT parameters. The page has SEVEN hover cells:
// #sync-container and six #<param>-container knobs. `division` has no cell of
// its own — it SHARES the TIME knob. When `sync` is on, a drag or a wheel on
// #time-knob steps `division` instead of `time` (index.html, setupTimeKnob),
// and #time-value shows the note name rather than a millisecond figure.
//
// So `division` is page-reachable, unlike O-Bass's latency_mode and bypass
// which are host-reachable only. It still gets no entry of its own, because a
// second TIP_BINDINGS row on the same anchor would simply overwrite the first
// one's data-tip attributes — last write wins, silently. Instead tip.time and
// tip.sync each carry the sentence that explains the shared knob, which is
// where a user pointing at that cell will look for it.
//
// ── TITLES ARE THE PAGE'S CAPTIONS, BYTE-IDENTICAL TO LABELS ───────────────
//
// Not the dump's `name` column, where the two disagree. The user is reading the
// page: the cell they are pointing at says RÉINJ., so the tip above it says
// RÉINJ. too. Those French captions are abbreviations forced by a 60 px
// .knob-label box (see the LABELS block below); a tooltip has 300 px and no
// such constraint, so each BODY opens by spelling the full word out —
// "Réinjection :" under a title reading RÉINJ. That keeps one string per idea
// and still gives the fuller word somewhere on screen.
//
// ── RANGES ─────────────────────────────────────────────────────────────────
//
// Every body ends with its range and unit, and on this plugin NO unit had to be
// recovered from a formatter: six of the eight parameters carry a real `label`
// in the dump (ms on `time`, % on the other five) and every one agrees with the
// page's own readout formatter. The two with an empty label are the two
// discrete parameters, whose range is words rather than a number — `sync` is
// Off / On and `division` is a twelve-way note choice.
//
// D-01 ARM 1 AND THE OPTION WORDS. `division`'s twelve option strings ("1/4"
// ... "1/16(5)") appear in tip.sync's body as prose naming a set, and they stay
// in their canonical form in BOTH languages: a fraction is a fraction, and the
// page writes those same twelve strings into #time-value where arms 1 and 3
// both exempt them. `sync` is an AudioParameterBool, not a Choice, so its
// Off/On has no automation lane to disagree with — the page already localizes
// those two faces (label.on / label.off, v1.3.0), and tip.sync's range names
// them the same way the faces do.
// ============================================================================

export const I18N = Object.freeze({

    // ── The seven controls, in the page's own left-to-right order ───────────

    // sync — AudioParameterBool, default Off. Named first because it changes
    // what the knob beside it means.
    //
    // The tempo caveat is the one thing a user cannot discover by clicking it:
    // processBlock only overrides the millisecond time when getPlayHead()
    // returns a position WITH a bpm (PluginProcessor.cpp:206-222), so in a host
    // that reports none — or in the Standalone with no transport — SYNC on
    // still plays the TIME knob's milliseconds.
    'tip.sync': {
        en: { t: "SYNC",
              b: "Locks the delay to the host's tempo, so the TIME knob chooses a note value instead of a time in milliseconds: 1/4, 1/8 and 1/16, each of them straight, dotted, triplet or quintuplet. If the host reports no tempo the delay keeps its millisecond time. Off or On." },
        fr: { t: "SYNCHRO",
              b: "Verrouille le délai sur le tempo de l’hôte : le bouton DURÉE choisit alors une valeur de note au lieu d’une durée en millisecondes — 1/4, 1/8 et 1/16, chacune simple, pointée, en triolet ou en quintolet. Si l’hôte n’annonce aucun tempo, le délai garde sa durée en millisecondes. Arrêt ou Marche.",
              reviewed: true },
    },

    // time — 1 .. 2000 ms, default 500. This is also the entry that documents
    // the shared knob, because #time-container is the cell `division` lives in.
    'tip.time': {
        en: { t: "TIME",
              b: "How long an echo waits before it repeats. With SYNC off this knob is a free delay time in milliseconds; with SYNC on the same knob steps through note values instead, and the readout shows the division rather than a figure. 1 to 2000 ms." },
        fr: { t: "DURÉE",
              b: "Le temps qu’un écho attend avant de se répéter. SYNCHRO à l’arrêt, ce bouton donne une durée de délai libre en millisecondes ; SYNCHRO en marche, le même bouton parcourt des valeurs de note et l’affichage montre la division au lieu d’un chiffre. 1 à 2000 ms.",
              reviewed: true },
    },

    // feedback — 0 .. 100 %, default 30. The 0.95 ceiling is the sentence: the
    // knob reads 100 but processBlock clamps the return to 0.95
    // (PluginProcessor.cpp:245), so the tail always decays.
    'tip.feedback': {
        en: { t: "FEEDBACK",
              b: "How much of each echo is fed back into the delay line, which is what sets how many repeats you hear and how slowly they fade. The return is capped just under unity, so even at the top of the range the tail decays instead of running away. 0 to 100 %." },
        fr: { t: "RÉINJ.",
              b: "Réinjection : la part de chaque écho renvoyée dans la ligne à retard, ce qui décide du nombre de répétitions et de la lenteur de leur extinction. Le retour est plafonné juste sous l’unité, donc même en haut de la course la traîne s’éteint au lieu de s’emballer. 0 à 100 %.",
              reviewed: true },
    },

    // spread — 0 .. 100 %, default 0. It offsets the RIGHT channel only, by up
    // to 15 ms (PluginProcessor.cpp:252, 256-257). "Widens" is the effect;
    // "right channel" is the mechanism, and it is why a mono track still
    // spreads and a mono OUTPUT does not.
    'tip.spread': {
        en: { t: "SPREAD",
              b: "Offsets the right channel's delay from the left by up to 15 ms, so the repeats reach each ear at slightly different times and the echo widens. At 0 both channels share one delay time and the tail stays centred. 0 to 100 %." },
        fr: { t: "ÉTAL.",
              b: "Étalement : décale le délai du canal droit par rapport au gauche jusqu’à 15 ms, si bien que les répétitions atteignent chaque oreille à des instants légèrement différents et que l’écho s’élargit. À 0 les deux canaux partagent la même durée et la traîne reste centrée. 0 à 100 %.",
              reviewed: true },
    },

    // mod — 0 .. 100 %, default 0. One 0.3 Hz sine (prepareToPlay, lfo
    // setFrequency 0.3f) moving BOTH delay lines by up to 10 ms
    // (PluginProcessor.cpp:253). Both channels share the LFO, which is exactly
    // what makes it a thickener rather than a widener — the distinction from
    // SPREAD is the reason the sentence is here.
    'tip.mod': {
        en: { t: "MOD",
              b: "A slow sine at 0.3 Hz that wanders the delay time by up to 10 ms, detuning each repeat the way tape wow does. Both channels move on the same wave, so it thickens the echo rather than widening it. 0 to 100 %." },
        fr: { t: "MOD",
              b: "Une sinusoïde lente à 0,3 Hz qui fait dériver la durée du délai jusqu’à 10 ms et désaccorde chaque répétition comme le pleurage d’une bande. Les deux canaux suivent la même onde : cela épaissit l’écho au lieu de l’élargir. 0 à 100 %.",
              reviewed: true },
    },

    // wet — 0 .. 100 %, default 30. WET and DRY are two independent gains
    // summed at the output (PluginProcessor.cpp:265, 275), NOT the two halves
    // of one mix control, and that is the thing a user cannot see from the
    // panel. The pair is documented as a pair, once from each side.
    'tip.wet': {
        en: { t: "WET",
              b: "The level of the delayed signal at the output. It is a level of its own rather than one half of a mix knob, so it does not take anything away from DRY and the two can be raised together. 0 to 100 %." },
        fr: { t: "TRAITÉ",
              b: "Le niveau du signal retardé en sortie. C’est un niveau à part entière et non la moitié d’un réglage Mix : il ne retire rien à DIRECT et les deux peuvent monter ensemble. 0 à 100 %.",
              reviewed: true },
    },

    // dry — 0 .. 100 %, default 100. The send case is the one that earns the
    // second sentence: on an aux bus the useful setting is DRY at 0.
    'tip.dry': {
        en: { t: "DRY",
              b: "The level of the untouched input at the output. Independent of WET, so pulling it to the bottom leaves the echoes alone on an effect send, and leaving it at the top keeps the source at full level. 0 to 100 %." },
        fr: { t: "DIRECT",
              b: "Le niveau de l’entrée intacte en sortie. Indépendant de TRAITÉ : le ramener en bas ne laisse que les échos sur un départ d’effet, le laisser en haut garde la source à plein niveau. 0 à 100 %.",
              reviewed: true },
    },

    // ── The two chrome controls ─────────────────────────────────────────────
    //
    // The gear tip is what tells a user that hover-help exists at all, so its
    // body describes ONLY what the popover actually contains. This plugin has
    // no hover-help on/off toggle — not a C++ one, not a localStorage one — so
    // the panel holds the language selector and nothing else, and the tip says
    // exactly that. A tip that promised a toggle would be a tip that lies.
    //
    // Both titles are byte-identical to an existing accessible name
    // (aria.settings) and an existing caption (label.language) respectively, so
    // the tip, the caption and the screen reader all say one word.
    'tip.gearBtn': {
        en: { t: "Settings",
              b: "Opens the panel that sets the language of this interface. That is all it holds: the labels on this page and this hover help switch with it, and the choice is kept with the session, so a project reopens in the language it was saved in." },
        fr: { t: "Réglages",
              b: "Ouvre le panneau qui règle la langue de cette interface. Il ne contient rien d’autre : les libellés de cette page et cette aide au survol changent avec elle, et le choix est conservé avec la session — un projet se rouvre dans la langue où il a été enregistré.",
              reviewed: true },
    },

    // The one entry that tells a user what does NOT change with the selector.
    // The three things named are exactly this page's exempt set: the readouts
    // (D-03, including the composed "500 ms"), `division`'s twelve option
    // strings (D-01 arms 1 and 3), and preset names (D-02).
    'tip.langSelect': {
        en: { t: "Language",
              b: "The language of the labels on this page and of this hover help. English and French are available. Value readouts, the note divisions and preset names stay in English, so the page and the host always name the same thing the same way." },
        fr: { t: "Langue",
              b: "La langue des libellés de cette page et de cette aide au survol. L’anglais et le français sont disponibles. Les valeurs affichées, les divisions rythmiques et les noms de préréglages restent en anglais, pour que la page et l’hôte nomment toujours la même chose de la même façon.",
              reviewed: true },
    },
    // v1.5.0 — the switch that reaches this whole layer.
    'tip.tipsToggle': {
        en: { t: 'Hover Help',
              b: 'Turns this hover help on and off. With it off, only the gear and this '
               + 'switch keep explaining themselves.' },
        fr: { t: 'Aide au survol',
              b: 'Active ou désactive cette aide au survol. Une fois désactivée, seuls '
               + 'l’engrenage et ce commutateur continuent de s’expliquer.',
              reviewed: true },
    },
});

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
    'label.time': { en: { t: 'TIME' }, fr: { t: 'DURÉE', reviewed: true } },

    // REINJECTION is the word Logic Pro's French build uses for a delay's
    // feedback and it does not fit: 66.30 px against a 60 px box, and being one
    // unbreakable word it would SPILL rather than wrap — painted straight over
    // the gap between two knobs. RETOUR (40.64) and REACTION (51.91) both fit
    // but both name something else in an audio context (a return bus, a
    // reaction). REINJ. is the abbreviation OF the expected word, which is the
    // same trade O-Chorus made for PROF.
    'label.feedback': { en: { t: 'FEEDBACK' }, fr: { t: 'RÉINJ.', reviewed: true } },

    // Stereo spread of the two delay lines. ECART is also what O-Chorus ships
    // for its own Spread, so the suite says one word for one idea.
    'label.spread': { en: { t: 'SPREAD' }, fr: { t: 'ÉTAL.', reviewed: true } },

    // Keyed with sameAsEn rather than exempted, deliberately. "Mod" is the
    // abbreviation of "modulation", which is the same word in French, but that
    // is a TRANSLATION JUDGEMENT and an I18N_EXEMPT entry would hide it from
    // the native-speaker worklist forever. Keyed, it is one more
    // `reviewed: false` line somebody has to agree with. MODUL. (40.06) also
    // fits if a reviewer wants the fuller form.
    'label.mod': { en: { t: 'MOD' }, fr: { t: 'MOD', reviewed: true, sameAsEn: true } },

    // WET / DRY is a pair and is translated as a pair. EFFET / DIRECT is the
    // idiomatic French pairing — the processed signal and the untouched one.
    // The literal MOUILLE (46.09) / SEC (18.91) also fits and is what a
    // dictionary gives, but no French audio interface says it.
    'label.wet': { en: { t: 'WET' }, fr: { t: 'TRAITÉ', reviewed: true } },
    'label.dry': { en: { t: 'DRY' }, fr: { t: 'DIRECT', reviewed: true } },

    // ── The sync toggle: its caption and its two faces ──────────────────────
    //
    // THE TIGHTEST STRING ON THE PAGE, 1.39 px under the 50.00 px cliff at
    // which `.toggle-container` widens and re-centres #sync inside it.
    // SYNCHRONISATION measures 96.52 and moves the toggle 23.25 px right.
    'label.sync': { en: { t: 'SYNC' }, fr: { t: 'SYNCHRO', reviewed: true } },

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
    'label.on':  { en: { t: 'ON' },  fr: { t: 'MARCHE', reviewed: true } },
    'label.off': { en: { t: 'OFF' }, fr: { t: 'ARRÊT',  reviewed: true } },

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
    'label.out': { en: { t: 'OUT' }, fr: { t: 'SORTIE', reviewed: true } },

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
    'label.load': { en: { t: 'Load' }, fr: { t: 'Charger', reviewed: true } },
    'label.save': { en: { t: 'Save' }, fr: { t: 'Enreg',   reviewed: true } },

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
    'label.presets':    { en: { t: 'Presets' },              fr: { t: 'Préréglages',                reviewed: true } },
    'label.noPresets':  { en: { t: 'No presets available' }, fr: { t: 'Aucun préréglage disponible', reviewed: true } },

    // ── The settings popover (v1.3.0) ───────────────────────────────────────
    'label.language': { en: { t: 'Language' }, fr: { t: 'Langue', reviewed: true } },

    // v1.5.0. All four renderings below are settled glossary ROOTS, copied
    // rather than authored: scripts/i18n-fr-glossary.js carries them as the
    // roots for 'hover help', 'on', 'off' and 'toggle hover help'. They take
    // the same review mark this file's other roots carry, and for the same
    // reason — they are not new machine output.
    'label.hoverHelp': { en: { t: 'Hover help' }, fr: { t: 'Aide au survol', reviewed: true } },
    'ui.on':           { en: { t: 'On' },         fr: { t: 'Marche', reviewed: true } },
    'ui.off':          { en: { t: 'Off' },        fr: { t: 'Arrêt',  reviewed: true } },

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
    'aria.prevPreset': { en: { t: 'Previous preset' },    fr: { t: 'Préréglage précédent',  reviewed: true } },
    'aria.nextPreset': { en: { t: 'Next preset' },        fr: { t: 'Préréglage suivant',    reviewed: true } },
    'aria.presetList': { en: { t: 'Click to see all presets' },
                         fr: { t: 'Cliquer pour voir tous les préréglages', reviewed: true } },
    'aria.loadPreset': { en: { t: 'Load preset from file' },
                         fr: { t: 'Charger un préréglage depuis un fichier', reviewed: true } },
    'aria.savePreset': { en: { t: 'Save current settings' },
                         fr: { t: 'Enregistrer les réglages actuels', reviewed: true } },

    'aria.settings':   { en: { t: 'Settings' },           fr: { t: 'Réglages',              reviewed: true } },
    'aria.langSelect': { en: { t: 'Interface language' }, fr: { t: 'Langue de l’interface', reviewed: true } },
    'aria.helpToggle': { en: { t: 'Toggle hover help' }, fr: { t: 'Activer ou désactiver l’aide au survol', reviewed: true } },
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
// TIP_BINDINGS — [selector, key, wrapper?, vars?]
//
// applyI18n() runs document.querySelector(selector), then closest(wrapper) if a
// wrapper is given, and writes data-tip-title / data-tip onto whatever it lands
// on.
//
// ── EVERY ANCHOR HERE IS AN ID, AND THAT IS A DIVERGENCE ───────────────────
//
// The Stage M brief records that "bind TIP_BINDINGS to the ids the UI already
// uses" was false on all three M1 pilots — O-Chorus's knobs carry no id,
// O-Emulator needed CSS selectors for five of seven anchors. It is TRUE here,
// and for a structural reason rather than luck: this page positions each
// control by id (#sync-container, #time-container ... #dry-container are the
// CONTROL POSITIONING block in index.html), so the cell a user aims at already
// has to be addressable. No wrapper walk is needed on any row.
//
// THE CONTAINER IS THE HOVER TARGET, not the knob inside it. #time-knob is the
// 60 x 60 visual alone; #time-container is the caption, the knob and the
// readout stacked with a 4 px gap — 60 x 91 px, and the tip opens anywhere in
// it, including over the caption the tip is titled after. Binding to
// circle.knob-vine, a 3 px stroke, would be a tip nobody could open.
//
// #gear-btn and #lang-select are bound directly. #lang-select's parent is the
// .settings-row label, and putting the tip there would make the row's caption
// ("Language") open a tip titled "Language", which reads as a bug.
//
// SEVEN ROWS FOR EIGHT PARAMETERS. `division` shares #time-container with
// `time` and gets no row of its own: applyI18n writes onto the anchor, so two
// rows naming the same element would leave whichever ran last, silently. See
// the I18N header.
//
// EVERY BINDING MUST RESOLVE. applyI18n() logs `i18n: tip target not found:
// <selector>` for one that does not, and boot-all-uis is the gate that sees
// that console warning. tests/ui_tip_render_check.js asserts resolution AND
// that each anchor actually paints a tip, which is the assertion no repo-wide
// gate can make.
// ============================================================================

export const TIP_BINDINGS = [
    ['#sync-container',     'tip.sync'],
    ['#time-container',     'tip.time'],
    ['#feedback-container', 'tip.feedback'],
    ['#spread-container',   'tip.spread'],
    ['#mod-container',      'tip.mod'],
    ['#wet-container',      'tip.wet'],
    ['#dry-container',      'tip.dry'],

    ['#gear-btn',           'tip.gearBtn'],
    ['#lang-select',        'tip.langSelect'],
    ['#tips-toggle',        'tip.tipsToggle'],
];

// The tooltip lookup. Returns {t, b} — never null, never a bare key without a
// console.warn saying so, because a silently-missing tip renders as an empty
// surface that looks like a positioning bug rather than a missing entry.
//
// LIVE as of v1.4.0: applyI18n() calls it once per TIP_BINDINGS row, on every
// language change, and setupTooltips() in index.html paints what it returns.
// Through v1.3.0 the loop was empty and this function ran zero times.
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
