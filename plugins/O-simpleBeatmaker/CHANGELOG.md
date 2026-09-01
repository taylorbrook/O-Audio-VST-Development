# Changelog — O-simpleBeatmaker

All notable changes to this plugin are documented here.
Format loosely follows [Keep a Changelog](https://keepachangelog.com/).

## [1.1.1] - 2026-08-31

French copy revised. Stage N of the repo-wide i18n rollout.

### Changed
- **32 of the 81 French entries revised** against the suite glossary
  (`scripts/i18n-fr-glossary.js`) and lint (`scripts/i18n-fr-lint.js`): 42
  findings to 0, `--strict` exit 0. Twenty-one entries were typography only —
  straight apostrophes to typographic ones throughout, and no-break spaces
  before `%`, `:`, `;` and between a number and its unit (`-60 dB`,
  `12 demi-tons`, `0 %`, `100 %`). Eleven carry a wording change.
- **"Cliquez sur", not "cliquez"** — `cliquer` is intransitive in French, and
  three sites carried the English transitive. The width-pinned step-grid hint
  now reads *cliquez sur une case pour allumer un pas* : *case* is the ordinary
  French for a square in a grid and is short enough to keep the Clear-all
  button on one row (344.03 px against the shipped 336.69; the faithful
  *cellule* is 354.70 and wraps it). *Case* carries through the grid and
  pattern-length tooltips so the page has one word for one thing.
- **Three tooltips no longer say something the English does not.** *Fût* is a
  drum shell, and two of the six voices have none, so Tone and Solo now say
  *l'instrument* where the English says "the drum". The Ghost Notes lesson said
  *entre les temps forts* — beats 1 and 3 in French — where the English says
  BACKBEATS, and now names *le backbeat*. The MIDI readout tooltip had dropped
  the English's "the SAME buffer" and has it back.
- **`<html lang>` now follows the language selector** (canon change, all
  plugins), so assistive technology reads the page in the language it is
  displayed in.

### Notes
- `reviewed: false` stays `false` on all 81 entries. The flag means *a native
  speaker read this*; Stage N is a second machine reading against a glossary
  and a lint, and it is recorded in the `js/i18n.js` header instead.
- The step-grid hint's clearance inside its row is now 7.17 px, down from
  17.56 px. That margin was banked in v1.1.0 against Windows/WebView2 font
  metrics, which remain unmeasured; the header records the one-line reversal if
  a Windows pass ever needs it back.
- No English copy, key, binding, selector, exemption or CSS rule changed.

## [1.1.0] — 2026-08-28

**The PAGE speaks French, not only the hover help.** Every caption, column
heading, hint, legend key, button face, voice name, MIDI-readout field and
tooltip switches with a language selector in a new header gear. The value
readouts, the six lesson-preset names and the MIDI note numbers stay English
(D-03 / D-02 / D-01). 81 French entries (29 tooltip, 52 label), all
`reviewed: false` — no native speaker has read them.

Seventh and last of the `O-simple*` family on canon v2, and the only plugin in
the batch with a RESIZABLE frame.

### Added
- **`Source/ui/public/js/i18n.js`** — the interface copy table, English and
  French, embedded through `juce_add_binary_data` and served from
  `PluginEditor::getResource` at `/js/i18n.js`. The tooltip copy MOVED out of the
  `TIPS` object in `js/app.js`; 78 English strings were compared back to v1.0.3
  byte-for-byte with entities decoded rather than re-typed, and 0 differed. The
  strings that are NOT in v1.0.3 are the gear panel's own copy, the two endonyms,
  the four step-cell accessible-name sentences and the `● synced` face (which
  lived in `js/app.js`, not in the markup).
- **A settings gear in the header**, holding the language selector alone. This
  plugin has never had a hover-help switch and does not gain one here. Styled in
  its own aged-paper vocabulary: the gear wears the lesson chip's paper fill and
  brown border rounded to a circle and lights the same green when open; the panel
  wears the `.group` plate; the selector wears `select.combo`'s border, radius
  and hand-drawn caret at panel scale.
- **The interface language rides the APVTS tree** as a plain `uiLanguage`
  property on the root, beside the `PATTERN` child the step grid already
  persists — the string `"en"`/`"fr"`, restored behind an `isVoid()` gate,
  because the ValueTree→XML round trip rebuilds every property as a string var
  and `isInt()` would never fire. Deliberately not an `AudioParameterChoice`: it
  must not reach a DAW automation lane, and loading a lesson preset must not
  change which language somebody reads their interface in.
- `tests/i18n-states.json` — four states the label gate cannot reach at rest: the
  gear popover open, the host-transport SYNCED face, one SEQ row and one MIDI row
  in the live readout, and a lesson preset loaded. With them the gate measures
  90 of 90 `[data-i18n]` elements.

### Changed
- **The tooltip anchors moved off `data-tip`.** Canon v2 WRITES `data-tip` as the
  tip BODY, and the family convention put the tip KEY there — the two cannot
  share one attribute. The 19 markup anchors moved to `data-param` (six parameter
  cells), an `id` (three panels, the lesson row, the Clear-all button) and the
  `data-preset` the six lesson chips already carried; the 36 generated anchors
  carry `data-param` (24 voice knobs) or the `id` `bindToggle` already needs (12
  mute/solo buttons). All 55 are named individually in `TIP_BINDINGS`, because
  `document.querySelector` returns the FIRST match and a class selector would
  have put all six Tune tips on the Kick strip.
- **The tooltip listeners are DELEGATED on the document.** No anchor carries
  `data-tip` until the first sweep runs, and 36 of the 55 do not exist when the
  markup is parsed, so v1.0.3's `querySelectorAll("[data-tip]")` at setup time
  would have bound nothing at all. `pointerover`/`pointerout` and
  `focusin`/`focusout`, because those bubble; a `pointerout` into the same anchor
  is ignored so the tip cannot flicker.
- **Four `innerHTML` builders became `createElement` + `textContent`** — the
  tooltip, the step cell, the MIDI row and the six voice strips. Localized copy
  must never reach a markup path, and a template string carrying prose is a raw
  English write no exemption can cover (an exemption lives in `i18n.js`, where an
  angle bracket is forbidden).
- **The tip bodies lost their `strong`/`em` tags.** The WORDS are unchanged.
- **The timing-lane hint is now one string and lost the italic on "actual".**
  Splitting it at the emphasis, the way the step-grid hint IS split, needs a
  fragment reading "each hit's" — and French moves both the possessive and the
  adjective, so that fragment has no translation. The step-grid hint keeps its
  emphasis and its `Del` keycap because its five fragments are whole clauses
  whose order survives.
- **The transport strip's `SEQ` tag no longer pads itself with a trailing space.**
  A localized string must not carry layout; the column is a CSS `min-width` and
  both faces are the bare word.
- The two dead `.tooltip strong` / `.tooltip em` rules are removed — the tip is
  built with `textContent` now, so a tip body can never contain an element for
  them to match.
- The transport strip and the gear share one `.header-right` cluster, so the
  header stays a TWO-item space-between row. A third top-level item would have
  re-spread the strip toward the centre in English as well as French.

### Fixed
- **Pre-existing: the render harness had drifted two patch releases.**
  `tests/render-harness/CMakeLists.txt` hard-coded `JucePlugin_VersionString="1.0.1"`
  and `JucePlugin_VersionCode=0x10000` while the plugin shipped 1.0.3. Both files
  now derive the version BY REFERENCE from one `set(OSIMPLEBEATMAKER_VERSION ...)`,
  the shape O-simpleFM, O-simpleGrain and O-simplePhysicalModelSynth already use.
  Unlike O-simplePhysicalModelSynth this plugin has NO preset manager and
  therefore no `.factory-version` sentinel — `~/Library/O-simpleBeatmaker/` does
  not exist and was checked rather than assumed — so the drift had no on-disk
  consequence here. It was still a harness compiled against a version the plugin
  had not been for two releases.
- **Pre-existing: the 29 knobs had a language-frozen accessible name.**
  `bindKnob()` copied its caption's `textContent` into `aria-label` once, at bind
  time, which runs before `initI18n()` — so the name was the English fallback
  forever and never followed a language switch. The name now comes from
  `data-i18n-aria` (markup for the five global knobs, set by `buildVoiceStrips`
  for the 24 generated ones) and is rewritten by every sweep. 33 keyed
  attributes, 31 of 33 confirmed switching by the gate (the two that do not are
  `MIDI` and the `Tempo`/`Swing` titles, which are the same word in French).
- **Pre-existing: the transport state line resized the header when the host
  started playing.** `● free-run` is 52.7px and `● synced` is 44.9px, so the
  strip — which sits at the right-hand end of a space-between header — shifted
  every time the transport changed. The 53px pin below covers all four faces in
  both languages, so it no longer moves at all.

### Geometry — every rule reverted ALONE to confirm the gate re-breaks
- `.title-block { flex: 1 1 0; min-width: 0 }` (was the default `0 1 auto`) — the
  header is a two-item space-between row, so the block's used width WAS its own
  max-content, which is the strapline's and therefore language-dependent: 625.6px
  English against the 640px cap wrapped in French. At basis 0 it is 702.3px in
  both. Costs English nothing — the h1 and the strapline are left-aligned blocks
  in a transparent box. Fourth plugin in a row where the BASIS, not the
  specificity, was the thing to check. **2 moved elements when reverted.**
- `.subtitle { max-width: 640px → 700px }` — the room the title block actually
  has now that its basis is pinned. **236 moved when reverted.**
- `.tr-key-length { min-width: 60px }` — LENGTH 43.3 / LONGUEUR 60.0, measured AS
  RENDERED: `.tr-label` is uppercased and letter-spaced by CSS, and a probe that
  copies only the font reads it 12.6px too narrow and lands the pin under the
  French. Pinned per ELEMENT, not per class: "tempo" is the same word in both
  languages and one shared `.tr-label` rule would have paid 16.7px on it for
  nothing. **8 moved when reverted.**
- `.tr-unit-steps { min-width: 21px }` — steps 21.0 / pas 14.0. **9 moved when
  reverted.**
- `#readTransport { min-width: 53px }` — free-run 52.7 / libre 34.0, and it also
  covers `● synced` 44.9 / `● synchro` 50.7. **10 moved when reverted.**
- **The `.grid-hint` WRAPPER SPAN was removed** and `.grid-hint` carried onto each
  of the five fragments instead. A wrapper is not a `[data-i18n]` element, so the
  label gate measures its box — and its box IS the French sentence's width, which
  is 142.9px wider than the English one and can never be anything else. Renders
  identically. **1 moved when reverted, and it was the last one.**

### Copy measured against the layout — each reverted ALONE
- `label.subtitle` **SHORTENED**: the faithful French is 806.4px against the
  706.4px the header can give the title block, so it wrapped and pushed the whole
  page down 15px. 690.0px, 16.4px of clearance. **236 moved when reverted.**
- `label.gridHintA` **SHORTENED** ("cliquez à nouveau" → "cliquez encore",
  353.6 → 336.7px): the hint line pushed the Clear-all button onto a second row
  and took the step-grid panel 15px taller than the English one.
- `label.gridHintC` **SHORTENED FOR MARGIN, NOT FOR THE GATE** ("la barre qui
  balaie" → "la barre mobile", 259.8 → 244.2px). Reverting EITHER of these two
  alone passes — each is independently sufficient — and reverting BOTH breaks the
  gate on 224 elements. They are an OR, not two fixes. gridHintC is kept anyway
  because with gridHintA alone the Clear-all button ends **2.0px** inside the
  1035px content edge instead of 17.6px, and Windows/WebView2 font metrics are a
  named deferral. "qui balaie" survives verbatim in the grid tooltip.
- `label.voice*`: the six drum names are the longest French additions on the page
  ("Grosse caisse", "Charley ouvert"). They land in a fixed 92px grid gutter and
  measure 13.2px tall — ONE line, the same line count as the English. No pin
  needed and none added.

### Verification
- `check-i18n --plugin` exit 0 on canon v2; `check-i18n --strict-v2` exit 0 with
  **14 v2 / 0 v1** — the whole `data-tip` convention is now localized.
- `check-ui-labels` **ALL CHECKS PASSED** across four states with **ZERO**
  non-label elements moved, French confirmed rendered (69/82 labels differ),
  `dataset.label === textContent` after init, after the switch and after a state
  pass in both languages, **90 of 90** keyed elements measured, no coverage hole,
  no page error, every resource served.
- `boot-all-uis` clean — 107 text elements, 81 aria, **0 title**, 82 i18n.
- Render harness **ALL PASS**, and **this harness is DETERMINISTIC**: three
  consecutive runs of one binary are digit-identical, at v1.0.3 and at v1.1.0
  alike. Comparing the two builds, every number is IDENTICAL, digit for digit —
  which is the strongest statement available here and is the right one, because
  this harness has no state-roundtrip probe for the new XML attribute to move.
- `auval -a` lists `aumu OSiB OuDv`.

### Measured at the 860 x 640 RESIZE MINIMUM — which no gate sees
`check-ui-labels` pins the viewport to the `setSize()` default it parses out of
`PluginEditor.cpp`, so it only ever measures 1060 x 900. This is the one
resizable frame in the family (`setResizeLimits(860, 640, 1920, 1400)`), and a
French label that fits at the default can still clip 200px narrower. Measured by
hand, in both languages, with the popover open and closed:
- **Nothing clips.** No leaf label's rendered text is wider than its own box in
  either language, at either size.
- Nothing overflows horizontally: the frame's scroll width equals its client
  width (854px) in both languages.
- The French page is **15px taller** than the English (1298px vs 1283px of scroll
  extent) inside a 634px pane that is already 649px over in English. The single
  French-only reflow is the timing-lane hint gaining a second line at 854px — a
  wrap inside a scrolling pane, not a clip.
The 1060 x 900 default and the 860 x 640 minimum are untouched.

### Not verified
- The C++ persistence round-trip has never been executed by hand on this plugin.
- Windows / WebView2 font metrics remain a named deferral, and the tightest
  French margin here is the step-grid line's 17.6px and the strapline's 16.4px.

## [1.0.3] — 2026-08-09

License-compliance release; no functional changes.

### Changed
- AGPL-3.0 notice headers added to all Ouaricon-authored sources (repo-wide
  compliance pass)
- Render-harness CMake config aligned with the shared harness template (test
  infrastructure only)

## [1.0.2] — 2026-07-15

Resolves the remaining Info findings from the 2026-07-15 deep code review
(`CODE_REVIEW.md` IN-01..IN-09). All PATCH-level consistency/robustness fixes;
no parameter or state-format changes.

### Fixed
- **Binary self-reported 1.0.1.** The initial 1.0.2 build shipped without the
  `CMakeLists.txt` VERSION bump; now `1.0.2` (this rebuild supersedes it).
- **IN-09 — Viz loop constructed a MidiMessage for SysEx on the audio thread.**
  Root cause: the host-MIDI viz readout called `meta.getMessage()` before any
  filtering, so a multi-KB SysEx heap-allocated a `MidiMessage` in `processBlock`
  — the same allocation class WR-03 fixed in the merge loop below it. Fix: same
  raw-byte gate (`numBytes == 3 && (data[0] & 0xF0) == 0x90 && data[2] != 0`)
  applied before touching the message; note/velocity read from raw bytes, no
  `MidiMessage` constructed at all. Note-on behavior unchanged (`isNoteOn()`
  already excluded velocity-0) (`PluginProcessor.cpp`).
- **IN-01 — Muted voices drew viz dots for host MIDI.** Root cause: sequencer hits
  were gated by `router.isVoiceAudible` before the viz push, but host note-ons
  pushed a `VizEvent` unconditionally while `handleTrigger` dropped the muted
  trigger — a dot and a MIDI-readout row for a hit that makes no sound. Fix: gate
  the host viz push with the same `isVoiceAudible` check (`PluginProcessor.cpp`).
- **IN-02 — C++ silence threshold (−59.5 dB) disagreed with the UI's −∞ display
  threshold (−59.95 dB).** Knob values −59.9…−59.6 showed a finite dB readout but
  rendered hard silence. Fix: `dbToGain` threshold aligned to −59.95
  (`DrumVoiceEngine.h`).
- **IN-03 — Free-run playhead transiently exceeded a shrunken pattern length.**
  Root cause: `freeRunStepPos` was wrapped *after* enumeration; on a 32→8 length
  change the reported phase could be e.g. 30 on an 8-step grid for one block
  (one-step visual hiccup). Fix: wrap the carried phase against the current
  `barLenSteps` at the top of the free-run branch (`SequencerClock.h`); JS
  additionally resets `lastPhaseCol` when the grid is rebuilt so the playhead
  class re-applies immediately (`app.js`).
- **IN-04 — Sample rate was fetched once at boot and read non-atomically.** A host
  SR switch (or an editor opened before the first `prepareToPlay`) left the timing
  lane's Δt-in-steps scale wrong until reload. Fix: the per-frame "frame" event
  now carries `sr` (JS updates live), and `currentSampleRate` is a relaxed
  `std::atomic<double>` (`PluginEditor.cpp`, `PluginProcessor.h`, `app.js`).
- **IN-06 — Synced step enumeration could miss steps if one block spanned more
  than a full pattern period.** Root cause: the candidate loop covered a fixed
  `bar ∈ {−1, 0, +1}` around `barStart`; with `blockPpq > barLenPpq` (tiny
  pattern + huge buffer + extreme bpm) later repetitions fell inside the window
  unenumerated. Fix: upper bound derived from the window
  (`barsSpanned = 1 + ceil(blockPpq / barLenPpq)`) (`SequencerClock.h`).
- **IN-07 — Knob drags could leave an open host automation gesture.** Root cause:
  drag end relied solely on window `pointerup`; a `pointercancel` (pen/touch, OS
  gesture interruption) never fired `sliderDragEnded`. Fix: pointer capture on
  pointerdown + `pointercancel` registered alongside `pointerup` (`app.js`).
- **IN-08 — 4 Hz grid poll could transiently revert a cell clicked mid-round-trip.**
  Root cause: a stale `getGrid` snapshot dispatched before a click overwrote the
  local grid state on arrival (visible flicker; C++ state was never wrong). Fix:
  local edits stamp `lastLocalEditTime` and poll results within 300 ms of an edit
  are dropped (re-checked after the await); boot/preset pulls bypass the holdoff
  via `refreshGridFromBackend(true)` (`app.js`).

### Added
- **IN-05 — Double-click-to-default on every knob.** New `getParameterDefaults`
  native fn returns `{ paramID: normalisedDefault }`; JS resets the knob inside a
  proper drag gesture on `dblclick` (project pattern from O-MicrotonalSampler
  v1.23.7) (`PluginEditor.cpp`, `app.js`).

### Testing
- Offline render-harness: all 12 probes passing (grid-accuracy, block-boundary,
  swing/humanize/quantize, viz-truth, and mono-parity all re-verified against the
  SequencerClock changes).
- auval revalidated post-build (aumu OSiB OuDv: PASS); app.js syntax-checked as an
  ES module before embedding.

## [1.0.1] — 2026-07-15

Resolves all Critical + Warning findings from the 2026-07-15 deep code review
(`CODE_REVIEW.md` CR-01, CR-02, WR-01, WR-02, WR-03).

### Fixed
- **CR-01 — Tonal voices detuned/disintegrated after ~30 min of sustained playback.**
  Root cause: Kick/Tom/Snare accumulated oscillator phase in an unwrapped `float`;
  past ~4×10⁶ rad the float ulp exceeds the per-sample increment and the phase
  quantizes to garbage. `fastSine()` wraps its *input*, not the accumulator. Fix:
  conditional `phase -= twoPi` wrap in each render loop (`KickVoice::phase`,
  `TomVoice::phase`, `SnareVoice::ph1/ph2`). Noise-based Hat/Clap unaffected.
- **CR-02 — Mono output bus was +6 dB (every voice double-added).** Root cause:
  `renderAll` aliases `R` to `L` on 1-channel buffers, and every voice's loop did
  `L += s; R += s;` — two adds into the same sample. Fix: `if (R != L)` guard on
  the second add in all five voices; mono now matches one stereo channel's level.
- **WR-01 — Lesson presets applied partially; stale mute/solo/level state silently
  broke the lesson.** Root cause: `applyConceptPreset` set only the 5 timing-feel
  params + grid, inheriting whatever the user last set for the other 37 (a soloed
  kick made the "Ghost Notes" snare inaudible). Fix: reset all 42 params to their
  defaults via `setValueNotifyingHost(getDefaultValue())` before stamping the
  preset (project pattern: applyPresetJson must reset to defaults first).
- **WR-02 — Offline bounces truncated max-decay kick/open-hat tails.** Root cause:
  `getTailLengthSeconds()` reported 3.0 s but a max-decay kick (tc = 1.2 s
  exponential) is still −22 dB there; −60 dB lands at ≈ 8.3 s. Fix: report 9.0 s.
- **WR-03 — Host SysEx/CC flood could malloc on the audio thread.** Root cause:
  `sequencerMidi.addEvents(midiMessages, …)` copied *every* host event into a 4 KB
  reserve; a multi-KB SysEx dump grew the `MidiBuffer` heap storage inside
  `processBlock` (PERF-01 violation). Fix: merge filtered to note-ons only via a
  raw-byte check (no `MidiMessage` construction for foreign events), and reserve
  raised to 16 KB as belt-and-braces.

### Testing
- Offline render-harness: all probes re-run and passing (includes new mono-parity
  probe asserting mono RMS matches one stereo channel).
- auval + pluginval revalidated post-build.

## [1.0.0] — 2026-06-25

First release. A pedagogical TR-808/909-lineage step-sequencer drum machine built
for the MUSC319 wk09 MIDI & beatmaking session: program a beat on a 16-step grid,
then **watch and hear** velocity, swing, quantize, and humanize reshape it in real
time. The step grid and the piano roll are literally two views of one MIDI stream —
the internal sequencer emits GM-mapped note-ons at sample-accurate offsets into the
same `MidiBuffer` as incoming host MIDI, so the voices and the visualiser see one
merged stream. Playable live over MIDI as a real 808/909-style instrument too.

Built in four staged passes (Foundation → DSP → GUI → Polish), each gated by an
offline render-harness and pluginval/auval.

### Added

- **Six synthesized drum voices** (no samples): Kick, Snare, Clap, Closed Hat,
  Open Hat, Tom — 808/909 flavour per voice, with the closed hat choking the open
  hat. Per-voice **tune / decay / tone / level** plus **mute / solo**. GM drum map
  36/38/39/42/46/45 — every voice is MIDI-playable from the DAW piano roll or a pad.
- **Host-synced step sequencer** with a **sample-accurate sub-step Δt** scheduler.
  Reads the host transport (tempo / ppq / play state), enumerates firing steps per
  block, and emits each hit at its exact sample offset (no block-boundary snapping).
  Free-runs at the `tempo` knob when the host is stopped / in Standalone.
- **Timing-feel engine** — the pedagogical heart:
  - **Swing** (0–75%) delays the off-beat 16ths into long-short pairs.
  - **Humanize** (0–100%) adds small pre-seeded random per-hit timing + velocity
    offsets so repeats aren't identical.
  - **Quantize Strength** (0–100%) pulls the *humanized* deviation back toward the
    grid — and, critically, **leaves intentional swing untouched** (the DSP-04
    invariant: `Δt = Δswing + Δhuman·(1−q)`).
- **Per-step velocity** (0–127) with ghost / normal / accent quick-states; velocity
  drives loudness and a little timbre (harder = brighter / snappier).
- **Selectable pattern length** (8 / 16 / 32 steps); custom 6×32 grid persisted in a
  `PATTERN` ValueTree child (lock-free `std::atomic<uint8_t>` grid, not 384 params).
- **WebView teaching UI** (single projector-readable page):
  - 6×16 step grid with click-to-toggle, click-again velocity cycle, and a live
    amber **playhead** sweeping in sync with the transport.
  - **Timing / groove lane** drawing each hit's **applied Δt** (the exact offset
    baked into the audio, not a UI recompute) — swing pushes off-beats late,
    humanize scatters, quantize pulls back, all visible in real time.
  - **Live MIDI readout** printing note-on (note#, velocity) events from both the
    internal sequencer and incoming MIDI, with a source flag.
  - Plain-language **tooltips** on every control (pointer + keyboard focus), grid
    keyboard operability, ARIA labelling, and a **Clear all** affordance.
- **Six concept-isolating factory presets** (the lesson tour) — each isolates one
  idea so a student can reverse-engineer the move:
  **Straight** (flat / no-feel baseline) · **Backbeat + Accents** (velocity alone) ·
  **Ghost Notes** (quiet snares that make it breathe) · **Triplet Swing** (clean
  shuffle, no scatter) · **Humanized** (loosened off the grid) · **Quantize Demo**
  (sweep quantize to pull the scatter back while the swing stays). Loading a preset
  sets the timing-feel knobs (host-notifying, so the UI updates) and stamps the grid.
- **Offline render-harness** (`tests/render-harness`, `-DOUARICON_BUILD_TESTS=ON`) —
  the DSP correctness gate: a headless console app injects a synthetic transport and
  asserts the six probes (grid accuracy ±0 samples, swing offset, humanize+quantize
  bounds, block-boundary independence, MIDI-playable voices + hat choke + aliasing
  budget, and viz-truth = the lane offset equals the applied audio Δt).

### Technical

- Real-time safe `processBlock`: no allocations / locks / file I/O on the audio
  thread; humanize RNG pre-seeded in `prepareToPlay`; `fastSine` LUT warmed there
  too. Audio→UI handoff via a lock-free `AbstractFifo` + atomics. Zero added latency
  (`setLatencySamples(0)`; the scheduling lookahead is bookkeeping, not a delay line).
- Cross-platform WebView: `NEEDS_WEBVIEW2 TRUE` + `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`,
  Windows `withUserDataFolder(tempDir)`, bare-path resource provider, single
  `O-simpleBeatmaker_UIResources` binary-data target (default BinaryData namespace).
- Validation: clean VST3 + AU + Standalone build; **auval `aumu OSiB OuDv` SUCCEEDED**
  (render / 1-channel / bad-max-frames / parameter set + ramp / MIDI);
  **pluginval `--strictness-level 10` SUCCESS** for both VST3 and AU; render-harness
  6/6 probes green.
