# Changelog — O-simpleBeatmaker

All notable changes to this plugin are documented here.
Format loosely follows [Keep a Changelog](https://keepachangelog.com/).

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
