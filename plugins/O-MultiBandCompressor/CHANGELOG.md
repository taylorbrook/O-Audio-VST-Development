# O-MultiBandCompressor Changelog

## Version 1.4.0 (2026-07-22)

UI-only release. No DSP, parameter, or state-format changes — presets and automation from
v1.3.0 load unchanged.

### Added

- **Tooltips across the whole interface.** Every control now shows hover help: the six
  knobs and three buttons in each of the four bands, all five global controls, both level
  meters, the four gain-reduction meters, the band headers, the spectrum analyzer, and the
  three crossover handles. Copy explains what the control does and states its range, so
  the numbers in the readouts have context.
  Implemented as a styled tooltip layer rather than native `title=` attributes: the OS
  tooltip has a fixed ~1 s delay and system chrome that clashes with the parchment theme.
  The custom layer uses a 120 ms delay, parchment fill with a `#5C4033` border in Garamond,
  flips above/below when it would run off the top, clamps horizontally to the viewport
  while its arrow keeps pointing at the control, and hides instantly on mouse-down so it
  never sits over a knob being dragged.
  The tooltip element lives outside `.plugin-container` (which is `overflow: hidden`) so
  tips on edge controls are not clipped. Copy is written via `textContent`, never
  `innerHTML`. Per-band wording is defined once in `BAND_TOOLTIPS` and applied to all four
  bands, so the four copies cannot drift apart.

### Fixed

- **Band header frequency ranges never updated.** The four `.band-range` readouts were
  literal strings in `index.html` (`20 Hz - 200 Hz`, etc.) and no code ever wrote to them.
  Moving a crossover repositioned the line, relabelled that one line, and repatched the
  DSP, while all four band headers went on advertising the factory defaults — so at any
  non-default crossover setting the headers were simply wrong. Preset loads and host
  automation had the same gap.
  Fix: `updateBandRanges()` rewrites all four readouts from the current crossover
  frequencies, called from both paths that can change them — `updateCrossoverPositions()`
  (the 30 Hz C++ push, which covers automation and preset recall) and `handleCrossoverDrag()`
  (using the live drag value, so the headers track the handle instead of trailing it by up
  to a frame).

- **Plugin bundle reported version 1.0.0.** `juce_add_plugin()` had no `VERSION` argument,
  so JUCE fell back to `PROJECT_VERSION` and every build since the first shipped as 1.0.0
  regardless of the CHANGELOG — confirmed against the installed bundle's
  `CFBundleShortVersionString`. Added `VERSION 1.4.0`. Hosts that key their plugin cache on
  the bundle version were unable to tell releases apart before this.

- **Crossover lines rendered at the wrong position on open.** The markup hardcoded
  `left: 15% / 45% / 75%`, but the log-scale positions of the 200 Hz / 2 kHz / 8 kHz
  defaults are `33.3% / 66.7% / 86.7%`. The lines were visibly misplaced until the first
  timer push corrected them. Initial values now match the defaults.

- **`applyOrderingConstraints()` misread a crossover parked at its minimum.** It used
  `getNormalisedValue() || 0.5`, so a legitimate normalised value of exactly `0` (the
  bottom of the range) was replaced by the `0.5` fallback — e.g. XOVER1 at 20 Hz was
  treated as ~68 Hz when constraining its neighbours. Changed to `??` and factored the
  three reads into `getCrossoverFreqs()`, now shared with the drag handler.

### Changed

- `formatFrequency()` drops a trailing `.0`, so 2000 Hz reads `2 kHz` rather than `2.0 kHz`,
  matching the strings the markup already shipped. Affects the crossover line labels as
  well as the new band ranges.

### Verification

- **Static frontend check:** all 55 tooltip selectors resolve against `index.html`; every
  `.closest()` wrapper class exists; the `.tooltip*` classes, `--arrow-x`, and
  `[data-placement]` rules are present in `styles.css`; all four `range-*` ids exist in the
  markup and are written by `app.js`; and `updateBandRanges` is reachable from **both**
  `updateCrossoverPositions` and `handleCrossoverDrag` — a regression in either alone would
  leave half the feature dead while the other half still looked correct.

- **Browser harness** — `app.js`/`index.html`/`styles.css` loaded unmodified against a stub
  of the JUCE `getSliderState`/`getToggleState`/`getComboBoxState` bridge, driven with real
  mouse events. This caught a defect that every other gate passed:
  `initializeTooltips()` was called from `initializeUI()`, which runs at module top level —
  above the `let`/`const` tooltip state, still in the temporal dead zone — throwing
  `ReferenceError: Cannot access 'tooltipEl' before initialization`. The throw escaped
  module evaluation, so **`initializeCrossoverDrag()` at the foot of the file never ran and
  crossover dragging was entirely dead**. The C++ build, `auval`, and the static check above
  all passed with the UI in that state. Tooltip init now happens at the foot of the file,
  after its state is evaluated, with a comment recording why it must stay there.
  Post-fix results: 0 console errors; 41/41 interactive controls covered by a tooltip
  (55 tip targets total); dragging crossover 1 to ~316 Hz updated LOW and LOW-MID live, and
  crossover 3 to ~3.5 kHz updated HIGH-MID and HIGH; the arrow lands within 1 px of the
  control centre; tips clamp inside the viewport at both panel edges while the arrow keeps
  tracking; tips over the analyzer flip below; and tips hide on mouse-down and stay
  suppressed for the duration of a drag.

- `node --check` clean on `app.js`; `auval -v aufx OMbc OuDv` **PASS**.
- Bundle version confirmed at the binary: `CFBundleShortVersionString` 1.4.0 and
  `AudioComponents` version `66560` (`0x010400`), up from `65536` (`0x010000` = 1.0.0).

## Version 1.3.0 (2026-07-01)

Transparency fix from `.planning/CODE-REVIEW.md` (WR-03). **Changes the sound** (for the
better): the plugin is now magnitude-flat at unity with all compressors bypassed.

### Fixed

- **WR-03 — Serial crossover summed with magnitude ripple even at rest.**
  Root cause: the 4-way split is serial (LOW exits at crossover 1, the remainder is
  split again at crossovers 2 and 3), so LOW never accumulated the phase rotation the
  upper bands pick up from crossovers 2/3, and LOMID never saw crossover 3's. Summing
  the bands therefore rippled up to **0.63 dB** around the crossover points with every
  compressor bypassed. The "Linkwitz-Riley guarantees flat magnitude" assumption only
  holds for a single 2-way split.
  Fix: all-pass compensation in `CrossoverNetwork` — an LR4 pair sums to a 2nd-order
  all-pass at its crossover frequency (Q = 1/√2), so LOW now passes through AP(f2)·AP(f3)
  and LOMID through AP(f3). The 4-band sum is then AP(f1)·AP(f2)·AP(f3): pure all-pass,
  flat magnitude. Costs 3 extra biquads per channel; coefficients follow the existing
  RT-safe in-place `ArrayCoefficients` pattern (CR-01) — no audio-thread allocation.
  Side benefit: bands are now phase-coherent at the sum, so per-band gain changes
  (compression, makeup) produce less phase-cancellation artifact around the crossovers.

### Verification

- **Offline A/B harness** (v1.2.2 crossover vs v1.3.0, 48 kHz, 20 Hz–20 kHz):
  - FFT of the summed impulse response (131k samples): old ripple 0.455–0.625 dB
    depending on crossover settings; new worst-case **0.014 dB** (float32 coefficient
    quantization floor, at 20 Hz with the 60/300/2.5k setting).
  - Stepped swept sine (120 log-spaced tones, whole-cycle RMS windows): old
    0.454–0.625 dB; new worst-case **0.013 dB**. Both methods, 4 crossover settings
    (default 200/2k/8k, 60/300/2.5k, 500/5k/16k, 120/800/3.5k) — PASS at ±0.02 dB.
- **pluginval** strictness 10 — PASS.
- **auval** (`aufx OMbc`) — PASS.

## Version 1.2.2 (2026-07-01)

Correctness + polish pass from `.planning/CODE-REVIEW.md` (WR-02, WR-04, IN-05, IN-06).

### Fixed

- **WR-02 — Mid/Side modes under-detected by −6 dB (compressed too little).**
  Root cause: the band buffers are preallocated stereo, but in mono M/S modes the
  crossover only fills channel 0 — `Compressor::processStereo` then averaged the silent
  channel 1 into the detector, halving the detected level. The active channel count is
  now threaded from `processMultiband` into `processStereo`, so detection runs over
  channels that actually carry signal. Mid/Side now apply the same gain reduction as
  Off/Both for identical threshold/ratio settings.
- **WR-04 — Attack/Release readouts didn't match the DSP value.** The APVTS ranges use
  skew 0.3 (`value = min + (max−min)·norm^(1/0.3)`) but the labels used a pure-log
  interpolation — at mid-travel the Attack label read ~4.5 ms while the DSP ran ~20 ms.
  The formatters in `app.js` now use the skew-aware mapping. Display-only; no DSP change.
- **IN-05 — Resource provider matched on basename only.** `getResource` now matches the
  full relative path via an explicit path→resource table (same pattern as O-DigiDelay),
  so same-named files in different folders can never collide.

### Changed

- **IN-06 — Spectrum analyzer now uses a log frequency axis.** The 64 UI bins are
  log-spaced 20 Hz–20 kHz (edges precomputed in `prepareToPlay`, matching the crossover
  overlay's log axis) instead of linear FFT-bin grouping that crammed everything below
  ~5 kHz into the left sliver. Each UI bin takes the peak (not average) of its FFT bins,
  so narrowband energy is no longer smeared. Cosmetic/analyzer-fidelity only.

## Version 1.2.1 (2026-07-01)

Real-time-safety pass from `.planning/CODE-REVIEW.md`. Removes all audio-thread
allocation, locking, and redundant work. **No intended sonic change** — the crossover
refactor is verified bit-identical to v1.2.0 (see Verification).

### Fixed (Real-Time Safety)

- **CR-01 — Crossover redesigned every block on the audio thread.**
  `CrossoverNetwork::updateCoefficients` no longer calls the heap-allocating,
  trig-heavy `FilterDesign::designIIR...ButterworthMethod` ×6 per block. It now caches
  the last `xover1/2/3` + sample rate and early-outs when unchanged (the common case),
  and when they change it designs with stack-only `IIR::ArrayCoefficients` and assigns
  into pre-allocated coefficient storage — zero heap allocation on the audio thread. The
  2nd-order Butterworth (Q = 1/√2) is numerically identical to the old order-2 design.
- **CR-02 — 48 `juce::String` allocations per block.** The per-band parameter IDs are no
  longer built as runtime strings each block. All 56 `std::atomic<float>*` pointers are
  resolved once in `prepareToPlay` into a `[band][param]` table and indexed in
  `processBlock`.
- **CR-03 — Per-block `AudioBuffer` allocation in M/S Mid/Side modes.** The mono M/S
  scratch buffer is now pre-allocated in `prepareToPlay` and reused via
  `setSize(..., avoidReallocating=true)` — no allocation on the audio thread.
- **WR-01 — `std::mutex` locked on the audio thread for the spectrum publish.** Replaced
  with a lock-free triple-buffer + atomic slot hand-off. `processBlock` never locks; the
  UI thread claims the most-recent frame without blocking the audio thread.

### Changed (Minor / Efficiency)

- **IN-01** — Crossover and compressor hot loops use cached `getReadPointer`/
  `getWritePointer` instead of per-sample bounds-checked `getSample`/`setSample`.
- **IN-02** — Attack/release ballistics coefficients are recomputed only when the
  attack/release time actually changes (was ~8 `exp()` per block, unconditionally).
- **IN-03** — Input/Output `dsp::Gain` now uses a ~20 ms ramp so gain automation no
  longer zippers.
- **IN-04** — Removed dead members (`EnvelopeDetector::peakValue`/`rmsValue`,
  `MultiBandProcessor::maxSamplesPerBlock`/`channelCount`, and the crossover's unused
  `spec` member).

### Fixed (regression caught during verification)

- The first cut of the CR-01 in-place coefficient write assumed `IIR::Coefficients`
  stores 6 taps; it actually stores the normalized 5-tap form (`{b0,b1,b2,a1,a2}`).
  Writing 6 floats overflowed the coefficient array (a heap overrun that produced NaNs
  under pluginval's parameter fuzz at strictness 10). Fixed by assigning through
  `IIR::Coefficients::operator=(std::array)`, which normalizes correctly and reuses the
  pre-allocated storage (RT-safe).

### Out of Scope (deferred — would change the sound)

- **WR-02** (M/S Mid/Side −6 dB detection), **WR-03** (serial-crossover all-pass
  compensation), **WR-04** (Attack/Release readout skew), **IN-05/06** (resource-provider
  basename match, log-frequency analyzer mapping) are intentionally *not* addressed here;
  several change audible behavior and belong in a separate MINOR release.

### Verification

- **Bit-exact A/B:** offline harness ran identical white noise + a swept crossover
  through the old (`designIIR`) and new (`ArrayCoefficients`) `CrossoverNetwork` for
  ~4.3 s; max sample difference = `0.000e+00` (bit-identical).
- **pluginval** strictness 10 — PASS (4 runs, including 3 randomized seeds); the fuzz
  test exercises M/S modes, solo, and bypass.
- **auval** (`aufx OMbc`) — PASS at 44.1/11/192 kHz, mono and 1→2-channel.

---

## Version 1.2.0 (2026-01-26)

### Added

- **Real-Time FFT Spectrum Analyzer:** Live frequency visualization in the spectrum display
  - 2048-sample FFT with Hann windowing (`juce::dsp::FFT`)
  - 64-bin downsampled output for efficient UI transfer
  - Mutex-protected thread-safe audio→UI communication
  - Smooth visual transitions (0.7 smoothing factor)
  - Gradient fill with olive/brown color scheme
  - Grid overlay for frequency reference

### Technical Details

- FFT computed when 2048 samples accumulated (mono sum of L/R)
- Magnitude values converted to normalized dB scale (-80 to 0 dB → 0 to 1)
- 30 Hz UI update rate with conditional send (only when new data ready)
- Error handling in JavaScript to prevent UI crashes

---

## Version 1.1.0 (2026-01-25)

### Added

- **Draggable Crossover Controls:** Click and drag crossover lines to adjust frequency split points directly in the UI
  - XOVER1 (Low/Low-Mid): 20-500 Hz
  - XOVER2 (Low-Mid/High-Mid): 200-5000 Hz
  - XOVER3 (High-Mid/High): 2000-16000 Hz
- Ordering constraints enforced during drag (XOVER1 < XOVER2 < XOVER3 with 100 Hz minimum gap)
- Visual feedback: hover highlights, drag state with green accent, enlarged handles
- Touch support for tablet/trackpad use
- Proper JUCE drag start/end notifications for undo/redo grouping

### Changed

- Crossover lines now have smooth hover transitions
- Disabled CSS transitions during drag for instant visual response

---

## Version 1.0.0 - Stage 3 Complete (2026-01-25)

**PRODUCTION READY:** All stages complete - build system, DSP processing, and GUI with real-time metering functional.

### Phase 5.3 - Real-Time Metering (2026-01-25) - FINAL PHASE

**Metering Infrastructure:**
- Timer callback at 30 Hz for smooth updates
- 4 per-band gain reduction meters (LOW, LOMID, HIMID, HIGH)
- GR meters display 0 to -24 dB compression activity
- Input/output level meters with stereo averaging
- Peak level detection using buffer.getMagnitude()
- Atomic floats for thread-safe communication (audio → UI)
- JavaScript meter update functions
- CSS transitions for smooth animations (30-50ms ease-out)

**Crossover Visualization:**
- Dynamic crossover line positioning
- Reads XOVER1/XOVER2/XOVER3 parameters in real-time
- Logarithmic frequency-to-position mapping (20 Hz - 20 kHz)
- Auto-updating frequency labels with Hz/kHz formatting

**Performance:**
- 30 Hz UI refresh rate (33ms interval)
- will-change CSS optimization for 60fps rendering
- No UI thread starvation
- Stable in DAW during playback

### Phase 5.2 - Parameter Binding (2026-01-25)

**Parameter Bindings:**
- 56 WebSliderParameterAttachment (8 global + 48 per-band)
- 13 WebToggleButtonParameterAttachment (1 global + 12 per-band)
- 1 WebComboBoxParameterAttachment (MS_MODE)
- All relays registered with .withOptionsFrom()
- Bidirectional sync: UI ↔ APVTS
- Automation support verified
- Preset save/load functional

**UI Controls:**
- Per-band knobs: Threshold, Ratio, Attack, Release, Knee, Makeup (24 total)
- Per-band buttons: Solo, Bypass, SC Listen (12 total)
- Global knobs: Input, Output, Mix (3 total)
- Global toggle: Auto-Makeup
- Global select: M/S Mode
- Value formatting: dB, ms, ratio, %

### Phase 5.1 - WebView Layout (2026-01-25)

**UI Structure:**
- WebView integration using juce::WebBrowserComponent
- Botanical/Ouaricon aesthetic (900x600px)
- HTML/CSS layout with 4-band sections
- Spectrum analyzer placeholder with grid
- Input/output meter placeholders
- Per-band GR meter placeholders
- Crossover visualization (3 handles)
- JUCE JavaScript bridge
- Resource provider for embedded assets
- Color-coded bands (brown, green, gold, orange)

### Phase 4.3 - Advanced Features (2026-01-25)

**Sidechain Filtering:**
- Per-band sidechain HPF (20-2000 Hz)
- Per-band sidechain LPF (500-20000 Hz)
- Filters applied to detector path only
- SC Listen mode for monitoring filtered signal

**Mid/Side Processing:**
- M/S encoding/decoding (power-preserving with sqrt(2))
- 4 processing modes: Off/Mid/Side/Both
- Both mode = 8 independent compressors (mid + side per band)

**Global Features:**
- Auto-makeup gain (80% compensation, 500ms smoothing)
- Dry/wet mixer (juce::dsp::DryWetMixer)
- Parallel compression capability
- All 56 parameters fully integrated

### Phase 4.2 - Crossover Network (2026-01-25)

**Multiband Architecture:**
- Linkwitz-Riley 4th order crossover (24 dB/oct)
- 3 crossover points with frequency validation
- Cascaded 2nd order Butterworth filters (12 IIR per channel)
- 4-band output: LOW, LOMID, HIMID, HIGH
- Flat magnitude summing (Linkwitz-Riley property)
- 4 independent compressor instances
- Per-band solo/bypass routing
- Gain reduction metering for all 4 bands

### Phase 4.1 - Compressor Foundation (2026-01-25)

**Compression Engine:**
- Feed-forward compressor topology
- Soft knee with quadratic interpolation (0-24 dB)
- Peak detector (absolute value)
- RMS detector (circular buffer, 10ms window)
- Peak/RMS blend (0-100% continuous)
- Envelope smoother with attack/release ballistics
- Input/output gain stages
- Bypass logic
- Stereo-linked compression
- Gain reduction metering (atomic float)

### Stage 1 - Foundation Complete (2026-01-25)

**Build System:**
- Created CMakeLists.txt with JUCE 8 configuration
- Plugin code: OMbc (4 chars)
- Manufacturer code: OuAu (Ouaricon Audio)
- Formats: VST3, AU, Standalone
- NEEDS_WEB_BROWSER TRUE for future WebView UI
- juce_dsp module added for DSP components

**Parameters (56 total):**
- 8 global parameters implemented (INPUT_GAIN, OUTPUT_GAIN, MIX, AUTO_MAKEUP, MS_MODE, XOVER1, XOVER2, XOVER3)
- 48 per-band parameters implemented (12 per band × 4 bands)
- Band prefixes: LOW, LOMID, HIMID, HIGH
- All parameters use JUCE 8 ParameterID format
- Logarithmic scaling for crossover frequencies (0.3 skew factor)
- State management (save/load) implemented

**Audio Processing:**
- PluginProcessor with pass-through audio (no DSP yet)
- Stereo input/output bus configuration
- prepareToPlay/releaseResources stubs ready for DSP
- Real-time safe parameter access example provided

**UI:**
- PluginEditor placeholder (900x600)
- Shows plugin name and parameter count
- WebView integration pending Stage 3

---

## Stage 0 - Research & Planning (2026-01-25)

**Research:**
- Professional multiband compressor examples studied (FabFilter Pro-MB, Waves C6, UAD Precision Multiband)
- JUCE modules identified for implementation
- DSP feasibility verified

**Architecture:**
- Linkwitz-Riley 4th order crossovers (24 dB/octave)
- Feed-forward compressor topology with soft knee
- Peak/RMS blend detection
- Mid/Side processing modes
- Per-band sidechain filtering
- Real-time FFT spectrum analyzer
- Auto-makeup gain with slow ballistics
- Global dry/wet mixer

**Complexity Assessment:**
- Score: 5.0 (Maximum complexity)
- 56 parameters
- 10 DSP components
- Phased implementation strategy defined

**Contracts Created:**
- architecture.md (complete DSP specification)
- plan.md (phase breakdown with test criteria)
- creative-brief.md (original concept)
- parameter-spec.md (56 parameter definitions)
