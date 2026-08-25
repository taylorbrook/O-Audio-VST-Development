# Changelog — O-simpleGrain

All notable changes to this plugin are documented here.
Format loosely follows [Keep a Changelog](https://keepachangelog.com/).

## [1.2.1] — 2026-08-25

### Fixed
- **Clicks on note-off, at any settings** (ported from O-simpleFM v1.2.5; found
  by a suite-wide sweep of the per-block ADSR push pattern). Root cause: the
  processor pushed ADSR parameters into the live `juce::ADSR` amp envelope every
  block via `setParameters()`, whose `recalculateRates()` recomputes the release
  slope from the SUSTAIN level — clobbering the envelope-value-based rate that
  `noteOff()` had just computed. With sustain = 0 the recomputed rate is 0, and
  `recalculateRates()` treats a zero-rate release as finished: it hard-resets the
  envelope one block after every note-off, truncating the ringing tail to
  silence instantly — the click. (JUCE's ADSR docs explicitly forbid changing
  parameters during playback.)
  Fix in `GrainVoice.h`: envelope params are cached each block but only pushed to
  the live envelope(s) when their values actually change AND the voice is not in
  its release phase; changes made mid-release apply at the next note-on. The
  release therefore always completes at the rate captured at note-off.
- Render-harness: new `noteoff-click` probe (sustain 0, slow decay, note-off
  mid-decay) asserting the release tail still rings after note-off.

### Testing
- Render harness: ALL PASS including the new probe (preRms 0.0068 / tailRms 0.0047).
- Negative control: probe re-run against v1.2.0 voice code fails as expected
  (preRms 0.0051 / tailRms 0.0000 — the tail is truncated to exact silence one block after note-off).

## [1.2.0] — 2026-08-09

Header layout fix and a tooltip on/off toggle. UI-only — no parameter, DSP, or
state-format changes (sessions/presets load unchanged).

### Added
- **"?" button in the header toggles tooltips on/off.** Sits at the end of the
  concept-preset tour bar; state persists across editor opens (localStorage,
  default on). Disabling also hides any tooltip currently showing.

### Fixed
- **Title "O–simpleGrain" wrapped across three lines.** The header's flex layout
  let the wide preset bar squeeze the title block. The title block no longer
  shrinks (`flex-shrink: 0`, `white-space: nowrap`) and the preset bar is pushed
  to the right edge (`margin-left: auto`), so all slack goes to the title.
- **Tooltips appeared in duplicate.** `setupTooltips()` set a native `title=`
  attribute as a fallback on every `data-tip` element, so hovering showed both
  the OS-native tooltip and the custom floating one. The copy now goes into
  `aria-label` (assistive tech keeps it; no native popup) and any stale `title`
  attributes are stripped.

## [1.1.3] — 2026-08-08

First published release (version aligned with the O-simple pedagogical suite).

### Changed
- Cross-platform release builds (macOS VST3+AU, Windows VST3, Linux VST3) via GitHub Actions
- AGPL-3.0 notice headers added to all Ouaricon-authored sources

## [1.1.2] — 2026-07-16

Resolves the nine deferred Info findings from the 2026-07-15 CODE_REVIEW.md
(IN-01..IN-09). Internal quality: no parameter IDs, ranges, or state format
changed (sessions/presets load unchanged).

### Fixed
- **IN-07: grains sprayed out of range emitted constant-value (windowed DC)
  thumps.** A spawn at `playhead ± up to 100%` of the source length could start
  outside `[0, sourceLen)`, where the clamped Lagrange taps all read the edge
  sample — at 100% position spray up to half the spawns were affected. Spawn
  positions now wrap into range, consistent with the playhead's own wrap. (Under
  heavy spray the cloud now carries real signal where the thumps were.)
- **IN-03: Position-knob glide speed depended on the sample rate.** The rest-
  ease was a fixed 0.0008/sample, ~2× faster at 96 kHz than 44.1 kHz. Now derived
  in `prepareToPlay` from a τ ≈ 28.3 ms time constant (`1 − exp(−1/(τ·fs))`),
  reproducing the shipped 44.1 kHz feel at every rate.
- **IN-04: float read positions quantized near the tail of long sources.** At
  96 kHz a 10 s source spans 960k samples where float ULP is 0.0625 samples —
  fractional increments jittered pitch/interpolation for late-reading grains.
  `Grain::readPos`, the voice playhead, and the processor→voice handoff are now
  double end-to-end.
- **IN-08: source status/thumbnail refreshed on fixed timers that raced the
  decode.** The Load… flow polled 1.2 s after the *click* (stale after a longer
  browse); combo switches polled 300 ms after the change. The processor now bumps
  a source-version counter on every successful publish; the editor timer emits a
  `sourceChanged` WebView event on change, and the JS drives the thumbnail +
  truncation status from that (a pending-label handoff keeps the drop's filename
  in the status line).

### Changed (performance / internal)
- **IN-02:** the audio thread no longer runs 2×8 RTTI `dynamic_cast`s per block
  (plus more in `prepareToPlay`) — voices are cached as typed pointers at
  construction (synth-owned for the processor's lifetime).
- **IN-09:** `prepareToPlay` skips the built-in re-decode + resample when the
  published source is already at the engine rate (hosts re-prepare on every
  buffer-size change; decoding 10 s of audio each time was a pointless stall).
  The dropped/user path already skipped via the v1.1.1 CR-01 fix.
- **IN-01:** deleted the editor's dead "reserved" `fileChooser` member (all
  picking goes through the processor's own chooser).
- **IN-05:** the JS grain meter reads `kGlobalGrainCap` pushed once via WebView
  initialisation data instead of a hardcoded 192; the window-formula/σ JS
  re-implementation is pinned by explicit CONTRACT cross-references in both
  WindowLuts.h and app.js.
- **IN-06:** `applyFactoryPreset`'s 19 parameter writes are wrapped in
  begin/endChangeGesture pairs (hosts recording automation logged the ungestured
  writes oddly; strict hosts warn).

### Tests
- All 11 render-harness gates PASS; `auval` SUCCEEDED;
  `pluginval --strictness-level 10` SUCCESS.

## [1.1.1] — 2026-07-16

Resolves the 2026-07-15 CODE_REVIEW.md findings CR-01, CR-02, WR-02, WR-03,
WR-04, WR-05 (WR-01 was already fixed in v1.0.2 — see the v1.1.0 recovery note
below). No parameter IDs, ranges, or state format changed; the `adsrEnabled`
param recovered in v1.1.0 is unchanged.

### Fixed
- **CR-01: a dropped source was silently replaced by the "fire" built-in on
  every host re-prepare.** `prepareToPlay` only special-cased `embedded:`
  identities; a `dropped:<name>` identity (no disk path — WKWebView strips it)
  fell through to the missing-file fallback, so any buffer-size change, engine
  stop/start, or offline bounce discarded the user's live sound mid-session
  (and `juce::File("dropped:…")` fired a debug `jassert`). Now: the raw dropped
  bytes are retained (≤32 MB cap) and re-decoded at a new engine rate; with the
  rate unchanged the published buffer is kept as-is; with the bytes gone the
  live buffer is kept (transposed at worst) — the built-in fallback only runs
  when nothing is realisable (fresh-instance restore of a name-only identity).
  A vanished picker-file path likewise keeps the live buffer instead of
  clobbering. `setStateInformation` got the same identity guards (no
  `juce::File` on non-path strings; same-instance restores reuse the retained
  bytes). *Root cause: the state-restore fallback ran on the live-session
  re-prepare path, where the source it discarded was still published.*
- **CR-02: data race on `currentSourceIdentity` / `currentSampleRate`.** The
  COW `juce::String` identity was written from host-controlled threads
  (`prepareToPlay`, `set/getStateInformation` — VST3 hosts may call these off
  the message thread) and the message thread (drop/picker/preset callbacks)
  with no synchronization — two unsynchronized ref-count ops on one String can
  double-release (UAF). Every access now goes through a `sourceStateLock`
  CriticalSection (never taken on the audio thread); the identity accessor
  returns by value. `currentSampleRate` (plain double, same multi-thread
  pattern) is now `std::atomic<double>`. *Root cause: the sibling state on the
  atomic-publish path was never given the same care as the buffer pointer.*
- **WR-02: whole file decoded before the 10 s cap was applied.** The cap lived
  in the resampler, after a full-file allocation + decode — a 45-min WAV cost a
  ~500 MB spike and a multi-second stall, and `lengthInSamples` (int64) was
  truncated straight to `int`, so a hostile header near INT32_MAX drove a
  multi-GB allocation (`std::bad_alloc`). The decode length is now clamped to
  the cap *before* allocating (int64 math), and the truncation notice fires for
  pre-truncated files too.
- **WR-03: lesson presets randomly kept or discarded a user-loaded source.**
  `applyFactoryPreset` reset `sourceSample` to its default with everything
  else, so pressing any concept button discarded a dropped/picked source *iff*
  the last built-in choice differed from fire — invisible state deciding
  whether your sound survived. Contract now explicit: **presets keep the
  current source** (`sourceSample` is skipped in the reset); **"Granular Fire"
  alone force-loads fire** via `loadBuiltInSource` (the old `setChoice` was
  silently suppressed by the APVTS when the choice already read fire, so the
  preset didn't actually load fire either).
- **WR-04: version drift.** Sources said 1.0.1, the harness said 1.0.1, the
  installed binary said 1.1.0, PLUGINS.md said 1.1.0 (see the v1.1.0 recovery
  note). All version sources now derive from one `OSIMPLEGRAIN_VERSION`
  variable in CMakeLists.txt (the harness's hand-rolled
  `JucePlugin_VersionString/Code` included) — reconciled at 1.1.1.

### Changed (performance / internal)
- **WR-05: the audio thread no longer takes a hidden mutex per block.**
  `std::atomic_load/store(shared_ptr&)` is not lock-free (libc++ backs it with
  a global mutex pool shared across the process) — a real priority-inversion
  risk on the RT path. The audio thread now reads one genuinely lock-free
  `std::atomic<AudioBuffer*>` view per block; shared_ptr ownership stays on the
  message/host side under `sourceStateLock`, and an outgoing source is parked
  in a retired list, freed only once ≥2 audio blocks have completed since the
  swap (per the O-MicrotonalSampler v1.24.0 pattern). Behaviour identical;
  memory bound: at most one parked 10 s buffer between publishes.

### Tests
- Added render-harness gate **11 (`adsr-bypass`)** — guards the reconstructed
  v1.1.0 feature: bypass ignores a 1.5 s attack (flat velocity level at once)
  and drains within ~a grain length of note-off, while the enabled envelope
  still ramps in and tails out. All 11 gates PASS; `auval` SUCCEEDED;
  `pluginval --strictness-level 10` SUCCESS.

## [1.1.0] — 2026-06-25 (source reconstructed 2026-07-16)

**Recovery note:** v1.0.2 and v1.1.0 were built, installed, and recorded in
PLUGINS.md but their source was never committed, and the working tree was later
reverted to v1.0.1 — the 2026-07-15 code review unknowingly reviewed the
regressed tree (its WR-01 "dead keyboard" finding was the already-fixed v1.0.2
bug resurfacing). v1.0.2 was restored from `backups/O-simpleGrain/v1.0.2/`;
v1.1.0's UI delta was recovered byte-exact from the installed binary's embedded
resources and its C++ side re-implemented from the recovered spec below.

### Added
- **`adsrEnabled` parameter (19th param, bool, default ON)** — an "ADSR" toggle
  in the envelope panel. **On** = the v1.0.x per-voice A/D/S/R behaviour,
  unchanged. **Off** bypasses the envelope: each note plays at a flat velocity
  level while held and, on release, the voice simply stops launching new grains
  so the cloud fades out over one grain length through the *Window* envelopes
  (no click) — a raw, immediate gate for the pedagogical "hear the grains
  themselves" use. The A/D/S/R knobs dim + lock while bypassed
  (`.env-bypassed`); the envelope still ticks internally so a mid-note toggle
  lands on a coherent state.

## [1.0.2] — 2026-06-25

Three user-reported bugs: a dead on-screen keyboard, a dead output scope, and an
overall-too-quiet output. No parameters, IDs, ranges, or state format changed
(sessions/presets load unchanged).

### Fixed
- **On-screen keyboard produced no notes.** The WebView keyboard calls the
  `uiMidi` native function on every key, but it was never registered on the C++
  side — and the processor had no `MidiMessageCollector` and no merge of UI notes
  into `processBlock`, so the entire UI-MIDI bridge (present in O-simpleFM) was
  missing. Keys highlighted but emitted nothing. Ported the proven O-simpleFM
  pattern: `midiCollector` member + `reset()` in `prepareToPlay` +
  `removeNextBlockOfMessages()` in `processBlock` + `handleUiMidi()` +
  `withNativeFunction("uiMidi", …)` in the editor. External MIDI was unaffected
  and still works. *Root cause: the Stage-3 UI-MIDI bridge was never wired; no
  automated gate exercised it because the render-harness injects MIDI directly.*
- **Output was ~6–12 dB too quiet on sparse/single-grain patches.** The master
  stage applied a *fixed* `kHeadroom = 0.5f` (−6 dB) sized to stop dense clouds
  clipping. But a single grain peaks near the source level, so that fixed cut —
  stacked on the equal-power pan and amp envelope — left sparse patches far too
  quiet (the deferred "overlap-aware normalization" the code comment promised
  never landed; nor did the "headroom normalisation upstream" the tooltip claims).
  Replaced it with overlap-aware normalization: `normGain = 1 / max(1, overlap×0.5)`
  where `overlap = grainSize × density`. Sparse/single grains now play at full
  level; dense clouds stay tamed below clip. Smoothed via the existing `outputGain`
  ramp (click-free). *Root cause: a fixed headroom factor cannot serve both the
  sparse and dense ends of the density axis.*
- **Output scope showed nothing.** A downstream symptom of the two bugs above —
  the scope data path (post-gain ring → analyzer → `scopeUpdate` → `drawScope`)
  was correct, but with the keyboard dead and the output very quiet there was
  nothing to draw. Restored by the keyboard + loudness fixes; no scope code changed.

### Tests
- Added render-harness gate **10 (`ui-midi-keyboard`)**: injects a held note via
  `handleUiMidi` and renders with an **empty host MIDI buffer**, asserting the
  collector drains the note and the synth sustains audible output — guards the
  UI-MIDI bridge that had no coverage and shipped silent in v1.0.1.

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
