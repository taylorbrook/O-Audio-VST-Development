# Changelog

All notable changes to O-Texture will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.1.2] - 2026-07-15

Resolves CODE_REVIEW.md info findings IN-01 and IN-03–IN-10 (IN-02 was already
resolved incidentally by CR-01 in v0.1.1).

### Fixed

- **IN-04: CMake hardcoded the macOS-arm64 ONNX Runtime path.** Root cause: the
  post-build embed step baked in `onnxruntime-1.19.2-macOS-arm64`, which breaks
  x86_64/universal builds. The dylib path is now derived from ANIRA's exported
  `ANIRA_ONNXRUNTIME_SHARED_LIB_PATH` (arch/OS-correct), with per-platform lib
  names for future Windows/Linux builds.
- **IN-09: Perlin cursors grew unboundedly.** Root cause: `cursors[ch] += step`
  forever — after days of playback float ULP exceeds small steps and the evolve
  modulation quantizes, then stalls. The noise is periodic in 256 (`hashAt` masks
  with `& 0xFF`), so cursors now wrap losslessly at 256 in `advance()`.
- **IN-10: Canvas backing store sized once at DOMContentLoaded.** Root cause: a
  cold WebView can fire DOMContentLoaded before the stylesheet settles, mis-sizing
  the XY pad. The canvas is now sized by a `ResizeObserver` with a DPR-aware
  backing store (`clientWidth × devicePixelRatio` + `setTransform`) for crisp
  Retina rendering.
- **IN-08: Dead fallback in the processBlock read loop.** The unreachable
  `samplesToRead <= 0` branch (which would have read across a hop boundary) is
  replaced with a `jassert` plus a defensive `break` that prevents an infinite
  loop if the OLA invariant is ever broken.

### Changed

- **IN-05: ANIRA unlinked from the plugin.** Its inference engine was never used
  (v0.1.1's CR-01 fix runs raw ONNX Runtime on a dedicated thread). The ANIRA
  fetch is kept solely as the cross-platform ONNX Runtime downloader;
  `libonnxruntime` is linked directly and `libanira.dylib` (384 KB × 3 formats)
  is no longer embedded in the bundles.
- **IN-06: encoder.onnx / prior.onnx removed from BinaryData** (~140 KB). They
  stay on disk under `Resources/models/placeholder/` and will be re-embedded when
  Transform mode lands.
- **IN-01: Dead `decoderReady` flag removed** (written in three places, never read).
- **IN-03: Unused JS removed** — `getBackendResourceAddress` import and the
  unused `thumb` lookup in `bindVerticalSlider`.

### Not Changed (investigated)

- **IN-07: MIDI input is kept, now documented as load-bearing.** Removing
  `NEEDS_MIDI_INPUT`/`acceptsMidi` was attempted and reverted: an `aumu`
  MusicDevice must accept MIDI — auval fails `MusicDeviceMIDIEventList` with
  error -4 without it. Comments at both declaration sites now explain why the
  (currently unused) MIDI input must stay.

### Testing

- Built VST3 + AU, installed with cache clear + dual-variant sweep.
- `auval -v aumu OuTx OuDv` PASS; pluginval strictness 5 PASS.
- `main.js` module parse check clean (no load-time errors that would blank the UI).

## [0.1.1] - 2026-07-15

Resolves CODE_REVIEW.md findings CR-01, CR-02, WR-01–WR-07 (IN-02 incidentally).

### Fixed

- **CR-01: Neural inference moved off the audio thread.** Root cause: `processBlock`
  ran `Ort::Session::Run` synchronously per hop, heap-allocated `Ort::MemoryInfo`/tensor
  wrappers per call, and logged from the audio-thread catch path. Now a dedicated
  inference thread (`juce::Thread`, 1 ms poll, high priority) decodes; the audio thread
  publishes latent vectors and consumes finished blocks through a seq-counter handoff
  (`reqSeq`/`doneSeq`, one request in flight, no locks). A missed hop plays the OLA
  Hann tail and retries — never blocks. Offline renders (`isNonRealtime()`) decode
  synchronously so bounces stay deterministic. `Ort::MemoryInfo` is created once in
  `initDecoderSession()`; the catch path sets an atomic `decodeError` flag instead of
  logging. Output tensors now bind directly to caller buffers, removing the shared
  intermediate buffer and its 16 KB copy per hop (also resolves IN-02).
- **CR-02: Out-of-bounds writes for >stereo layouts.** Root cause: no
  `isBusesLayoutSupported` override, so hosts could open the output bus as 5.1/7.1
  while `TiltFilter` was prepared for 2 channels — `lpState[ch]` indexed past the
  vector. Added the override (stereo out; sidechain disabled-or-stereo), clamped
  `TiltFilter::processBlock` to the prepared channel count, and `prepareToPlay` now
  passes the real output channel count.
- **WR-01: Data races on Perlin noise state.** Root cause: `setStateInformation`
  (host thread) rebuilt the permutation table and cursors while the audio thread read
  them. Restored seed/cursors are now staged under a `SpinLock` and applied by the
  audio thread at the top of `processBlock` (try-lock, never blocks); the audio thread
  publishes a per-hop seed/cursor snapshot that `getStateInformation` reads. If a
  restore hasn't been applied yet (no audio running), saving passes the staged values
  through verbatim so load→save round-trips.
- **WR-02: Restored evolve state wiped by `prepareToPlay`.** Root cause: unconditional
  `evolveNoise.reset()` in `prepareToPlay` discarded the restored evolution position
  in the standard host call order (setState → prepare). The reset is removed — Perlin
  state is hop-domain and sample-rate independent.
- **WR-03: FREEZE didn't freeze; sound not reproducible.** Root cause: inactive latent
  dims were re-randomized every hop from a wall-clock-seeded RNG. Inactive-dim values
  are now drawn once per seed (`regenerateInactiveValues`), so FREEZE holds still,
  EVOLVE=0 is static, and a saved project reproduces its texture from the persisted seed.
- **WR-04: Phantom 6144-sample latency (~128 ms PDC).** Root cause: latency reported
  as `ACCUM_SIZE` although the generator outputs from sample 0 with no input-to-output
  delay. Now reports 0.
- **WR-05: `M_PI` broke MSVC builds.** Replaced with a literal `twoPi` constant in
  `HannWindow.h` (header is deliberately JUCE-free).
- **WR-06: Double-click reset used 0.5 for every control.** Root cause: hardcoded
  normalized reset in JS drifted from C++ defaults (MIX=1.0, EVOLVE=0.3). Reset now
  takes a per-control default matching `createParameterLayout()`.
- **WR-07: Dead SOURCE/MODE controls presented as live.** Only the Rain model exists
  in v0.1.x, and Transform mode is unimplemented. The five other source buttons and
  the Transform toggle are now disabled with "Coming soon" tooltips. The SOURCE/MODE
  parameters (and sidechain bus) are intentionally kept so the parameter contract is
  stable when real models land.

### Testing

- Built VST3 + AU, installed with dual-variant sweep, `auval` validation.

## [0.1.0] - 2026-02-15

### Added

- Neural texture synthesis engine (1D CNN VAE, 32-dimensional latent space)
- 10 real-time parameters: Source, Mode, X, Y, Character A/B, Evolve, Freeze, Brightness, Mix
- 6 source categories: Rain, Metal, Wind, Crowd, Synth, Organic
- Generate and Transform modes (IS_SYNTH instrument plugin)
- XY pad with orbital trail animation (Canvas 2D, 30fps throttled)
- Evolve modulation via 28-channel Perlin noise with quintic interpolation
- Freeze mode (halts latent evolution, pauses trail animation)
- Brightness tilt filter (1-pole, 800 Hz pivot, SmoothedValue for zipper-free)
- Stereo decorrelation via latent offset (0.1 on X/Y dimensions)
- Overlap-add crossfading (4096-sample blocks, 2048-sample hop, Hann window)
- ONNX Runtime decoder inference (direct C++ API, synchronous)
- Ouaricon Naturalist WebView UI (aged paper, botanical motifs, serif typography)
- 3 vertical sliders (Character A, B, Evolve) with naturalist styling
- 6 source icon buttons with inline SVG line art
- 2 rotary knobs (Brightness, Mix) with seed cross-section visuals
- Fern botanical overlay (bottom-right, low opacity)
- State serialization including evolve noise seed and cursor positions
- Full JUCE 8 WebView relay/attachment parameter binding system
- ANIRA v2.0.3 + ONNX Runtime 1.19.2 embedded in plugin bundles (macOS)
- Ad-hoc code signing for local testing

### Technical Notes

- Uses placeholder ONNX models; real trained models will be integrated in a future version
- Plugin registers as AU instrument (aumu OuTx OuDv)
- JUCE 8.0.4, C++20
- Latent space mapping loaded from dim_map_rain.json (BinaryData)
- Pre-allocated decoder output buffer for real-time safety
