# Changelog — O-simpleGrain

All notable changes to this plugin are documented here.
Format loosely follows [Keep a Changelog](https://keepachangelog.com/).

## [1.0.1] — 2026-06-25

Code-review fixes — two correctness bugs, two real-time hot-path simplifications,
and test coverage for the bug that had none. No parameters, IDs, ranges, or state
format changed (sessions/presets load unchanged).

### Fixed
- **Velocity → Density was 100× over-scaled** — a hard switch instead of a graded
  depth. The `velToDensity` parameter is stored 0–100 (%), but `GrainVoice` consumes
  it as a 0..1 depth; the processor pushed the raw 0–100 value, so any setting above
  ~1 % slammed the effective density to its rail (1 or 200) for any non-mid velocity.
  Now scaled ×0.01 at the push site (`PluginProcessor::processBlock`). The control is
  smooth across its full travel again. *Root cause: missing unit conversion between
  the 0–100 % param range and the voice's documented 0..1 depth contract; it escaped
  Stage-2 validation because the render-harness pinned `velToDensity` to 0.*
- **Restored "load-your-own" source could be clobbered on session reload.** A
  user/dropped source restored in `setStateInformation` could be overwritten by the
  built-in chosen by `sourceSample`, because `replaceState()` queues a deferred
  `AsyncUpdater` rebuild that ran *after* the synchronous `suppressChoiceRebuild`
  guard had already been cleared. Now the restore publishes the correct source and
  then `cancelPendingUpdate()`s the queued rebuild; the ineffective guard flag was
  removed. *Root cause: a synchronous flag cannot gate a deferred async callback.*

### Changed (performance / internal)
- **No transcendentals in the per-sample grain render loop.** Equal-power pan gains
  (`cos`/`sin`) and the anti-aliasing one-pole coefficient (`exp`) are constant for a
  grain's life but were recomputed every sample for every active grain (up to 192).
  They are now computed once on spawn and stored on `Grain` (`panL`/`panR`,
  `aaCoeff`/`aaEngaged`); the inner loop is a multiply / branch + multiply-add.
  Behaviour is equivalent — purely a hot-loop hoist (CPU win scales with cloud
  density). The AA bypass edge (`state = x` at rate ≤ 1) is preserved.

### Tests
- Added render-harness gate **9 (`velToDensity-depth`)**: asserts the grain count is
  velocity-independent at depth 0 and tracks velocity at full depth — guards the
  scaling fix above from regressing (the param was previously exercised by no gate).

## [1.0.0] — 2026-06-25

First release. A pedagogical **granular synthesizer** with a field-guide "Naturalist"
WebView UI, built to make *"oh, THAT's what granular synthesis is"* land in a few
minutes — single grains, grain clouds, freeze, and the sync→async axis, each made
visible and audible.

### Synth engine (DSP)
- 8-voice polyphonic **granular** instrument. Each `GrainVoice` schedules grains from a
  preallocated `std::array<Grain, 24>` (steal-oldest when full) against a global cap of
  **192 grains** — no allocation or locks in `processBlock`, no xruns under stress.
- **Density** scheduler (1–200 grains/s) with a derived live **overlap** readout
  (`grainSize × density`); **Grain Size** 2–200 ms is the buzz↔fragments control.
- Five precomputed 2048-pt **window** LUTs (Rect / Tri / Welch / Gauss / Hann). The
  rectangular window intentionally **clicks** — a teaching artifact, not a bug.
- 4-point **Lagrange** interpolated read + overlap-add; **key-tracked resample**
  (root C3 / note 60) combined multiplicatively with Grain Pitch (±24 st) and per-grain
  Pitch Spray (0–12 st); per-grain **rate-tracking one-pole** anti-aliasing.
- **Read head**: Position (0–100%), Scan / time-stretch (±200%, reverse), and **Freeze**
  — pins the read head on one instant and sustains indefinitely, with a smoothed,
  click-free crossfade on toggle (zipper-free).
- **Spray & scatter**: Position Spray, Pan Spray (equal-power), Pitch Spray, and
  **Scatter** (0–100%) — the sync→async axis that dissolves the discrete grain-rate comb
  into broadband noise. Velocity → Density (opt-in) and velocity → amp (always-on).
- Per-voice amplitude **ADSR**; 20 ms-smoothed **Output Level** trim with overlap-aware
  headroom. `setLatencySamples(0)`.
- **Sources**: 4 embedded built-ins (fire / voice / water / piano) via
  `juce_add_binary_data`, hot-swapped on an atomic source pointer. **Load-your-own**
  short source (≤10 s) by macOS WebView content-streaming drag-drop
  (`juce::Base64::convertFromBase64`) or file-picker fallback; loaded-source identity
  persisted as custom (non-APVTS) ValueTree state.

### Interface (WebView)
- Single-page Ouaricon-Naturalist UI; all **18 parameters** two-way bound (relative-drag
  knobs, wheel, keyboard arrow-keys; two combo boxes + Freeze toggle).
- **Four live visualizations**, pushed at 30 Hz off lock-free audio-thread taps
  (`TripleBuffer` grain events + `VizRing` samples + atomic count; FFT on the message
  thread, never the audio thread):
  - **Grain cloud** — scatter accumulates as density thickens and spray widens it.
  - **Source waveform** — live per-grain read playheads, the ❄ freeze pin, and a shaded
    spray band.
  - **Oscilloscope** + **spectrum** — discrete sidebands at scatter=0 smearing to noise.
- **Window-envelope inset** that redraws on Window-shape change, and a live
  **grain / overlap / CPU** readout counting `N / 192`.
- Plain-language **tooltips** on every control, reachable by mouse *and* keyboard focus.
- **Source loading** by drag-drop or Load… picker, both granulating a user file.

### Presets
- 8 factory **concept presets**, each isolating one granular idea — knobs, combos, and
  toggle snap together with a caption and active-state, written through the APVTS:
  **Single Grain · Pitched Buzz · Fragments · Smooth Cloud · Frozen Pad ·
  Asynchronous Cloud · Granular Fire · Rect Click**.

### Validation
- `auval -v aumu OsGr OuDv` → **AU VALIDATION SUCCEEDED** (AU).
- `pluginval --skip-gui-tests --strictness-level 10` → **SUCCESS** on the installed VST3
  (parameter automation, thread-safety, state, bus layouts, fuzz). GUI-open suite folds
  into the DAW listen.
- Offline render harness **8/8** — makes-sound, density→continuity, pitch-tracks-MIDI
  (C2/C3/C4 exact), window-rect-clicks, freeze-sustains, scatter sync→async,
  stress-bounded (≤192 grains), up-transposition stable.
- 8 factory presets desk-checked against `parameter-spec.md` — all write in-range,
  finite, denormal-free APVTS state.
- Live viz animation, audible preset character, drag-drop, and host-automation→UI
  round-trip are confirmed by a DAW listen (handed over at Stage-4 close).

### Internal
- Render-harness CMake now links `O-simpleGrain_UIResources` (NAMESPACE `UIBinaryData`),
  which the Stage-3 `PluginEditor.cpp` `getResource()` requires — fixes a test-harness
  link regression surfaced when the offline harness was re-run against the Stage-3 editor.
  No product DSP, parameter, or UI change.

### Platforms
- macOS: VST3 + AU + Standalone. Windows VST3 cross-platform flags in place
  (`NEEDS_WEBVIEW2 TRUE` + `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` +
  `withUserDataFolder`), statically verified; Windows build **deferred to publish/CI**.
