# O-Prism Changelog

## v1.13.3 (2026-04-10)

### Fixed
- **WebView event listener leaks**: Document- and window-level listeners were being attached without cleanup paths, so they persisted for the lifetime of the page and could accumulate if their host scopes ever re-ran.
  - **`wavetable-editor.js`**: `document.keydown`, `window.resize`, and `window.mouseup` (harmonic mouse-up) were bound inside `bindEvents()` as anonymous handlers and never removed. Moved them into new `bindGlobalListeners()` / `unbindGlobalListeners()` functions that run on `onTabActivated()` / `onTabDeactivated()`, guarded by a `globalListenersBound` flag. Handlers are now stored as module-level refs so they can be removed by identity.
  - **`tuning-panel.js`**: `setupRefPitchKnob()` attached `document.mousemove` and `document.mouseup` as inline anonymous arrows. Converted to instance-stored refs (`_refPitchMouseMove`, `_refPitchMouseUp`) and added a `destroy()` method that detaches them for clean teardown when the panel is replaced.
  - **`index.html` (inline tuning IIFE, ~line 2823)**: `document.mousemove` and `document.mouseup` on the A4 reference-pitch knob were anonymous. Converted to named handlers (`refPitchMouseMoveHandler`, `refPitchMouseUpHandler`) and registered a `window.__prismTuningCleanup()` hook that detaches them, giving any future teardown path a way to prevent accumulation across re-inits.

### Technical Notes
- Domain: UI (WebView)
- Root cause: Global-scope (`document`/`window`) listeners attached as anonymous functions with no removal path. While the JUCE WebView reloads fresh on each editor open (so leaks don't currently persist across plugin instances), the pattern was fragile — any future re-activation logic or UI rebuild would have caused real accumulation. This change makes every such listener removable by identity.
- No parameter changes, no DSP changes — behavior is byte-identical. Element-scoped listeners (on knob/canvas nodes inside the container) are left untouched since they die with their DOM nodes when the container is cleared.
- Note: `tuning-panel.js` is not currently imported by `index.html` (the live tuning UI is inlined), but was fixed preemptively so the pattern is correct if the module is wired in later.

## v1.13.2 (2026-04-10)

### Fixed
- **Reference pitch (A4) knob sync**: The tuning panel's reference pitch knob was desynced from the backend master tune parameter in two ways:
  1. On init, the knob hardcoded 440 Hz instead of fetching the actual `masterTune` value from the backend — the UI showed 440 even when a saved session had a different value.
  2. On mousedown, the drag baseline (`startValue`) was captured once at setup and never refreshed, so after any external parameter change (preset load, automation, undo) subsequent drags jumped relative to a stale baseline.

  Fix: `loadInitialState()` now calls `getMasterTune` and updates the knob UI via a new `updateRefPitchKnobUI()` method. The current value is mirrored on `this.masterTune`, which `mousedown` reads to set a fresh `startValue` on every drag (`tuning-panel.js`).

### Technical Notes
- Domain: UI (WebView)
- Root cause: Drag-baseline closure captured once at `setupRefPitchKnob()` time and UI init didn't query backend state
- No parameter changes — full backward compatibility

## v1.13.1 (2026-04-10)

### Fixed
- **Parallel filter routing +6dB gain inflation**: Parallel mode (Filter A + B) summed both filters at unity, producing ~2x the level of serial mode (A → B). Added 0.5x scaling to the parallel sum so both routing modes output at matched levels.

### Changed
- **Pitch mod cleanup**: Removed redundant `* 12.0 / 12.0` no-op from `pitchModRatio` calculation and replaced `std::pow(2.0, x)` with `std::exp2(x)` for clarity (`PrismVoice.cpp:429`). Zero behavior change — algebraic identity.

### Technical Notes
- Domain: DSP
- Root cause: Uncompensated additive sum in parallel filter path (`PrismVoice.cpp:601-602`)
- No parameter changes — full backward compatibility

## v1.13.0 (2026-04-09)

### Changed
- **Reorder FX chain**: Moved reverb from last to second-last position in the effects chain. New order: Distortion → Chorus → Delay → Reverb → EQ. EQ is now the final stage for post-reverb tonal shaping.
- **Effects tab UI**: Reordered to match signal chain — reverb now appears directly above EQ at bottom of effects tab.

### Technical Notes
- Domain: DSP + GUI
- No parameter changes — full backward compatibility
- Presets load identically; only processing order changed

## v1.12.1 (2026-04-09)

### Fixed
- **Reverb buzzing**: `tankFeedbackA`/`tankFeedbackB` were local variables in `process()`, reset to zero every buffer call. The Dattorro figure-8 cross-feedback was broken at buffer boundaries (~86 Hz discontinuity at 512-sample buffers). Moved to member variables for correct inter-buffer persistence.
- **Incorrect output tap formula**: Only 5 taps per channel with wrong delay line sources and reused tap positions. Replaced with full Dattorro Table 1 output: 7 taps per channel from correct delay lines and allpass diffuser nodes. Added `read()` method to `Allpass` struct for diffuser taps.
- **Static decay diffusion 2 coefficient**: Second decay diffuser used fixed `0.7f` instead of the Dattorro decay-dependent formula `decay² × 0.5 + 0.15`. Now updates dynamically with the Size parameter for natural decay character.
- **Per-sample `scaledDelay()` in audio loop**: `tankDelayA2` and `tankDelayB2` read positions were recomputed every sample via float division. Precomputed as `delayA2len`/`delayB2len` in `prepare()`.

### Technical Notes
- Domain: DSP
- Root cause: v1.12.0 Dattorro implementation had local feedback state and incomplete output tap network
- No parameter changes — full backward compatibility

## v1.12.0 (2026-04-08)

### Changed
- **Dattorro plate reverb**: Replaced stock `juce::dsp::Reverb` (Freeverb/Schroeder-Moorer) with a full Dattorro plate reverb implementation. Figure-8 tank topology with 4-stage input diffusion, cross-fed parallel decay paths, one-pole damping filters, and multi-tap stereo output. All delay lengths scaled from the original 29761 Hz reference rate.

### Added
- **Reverb modulation controls**: Two new parameters — `reverbModDepth` (0-100%, default 30%) and `reverbModRate` (0.1-5.0 Hz, default 1.0 Hz). LFO modulates tank delay lines with 90-degree phase offset between left and right paths for lush stereo movement characteristic of plate reverbs.

### Technical Notes
- Domain: DSP + GUI
- Dattorro reference: "Effect Design Part 1: Reverberator and Other Filters", J. Audio Eng. Soc., 1997
- Custom allpass, delay line, and one-pole filter structs (no heap allocation in audio thread)
- Existing parameters (Size, Damp, Pre-Dly, Mix) preserved with same IDs — full backward compatibility
- Size maps to tank decay coefficient (0.0-0.98), Damp maps to one-pole LPF coefficient
- No breaking parameter changes — existing presets load without issue

## v1.11.0 (2026-03-09)

### Added
- **Oscillator warp modes**: 4 post-wavetable-lookup warp algorithms applied per-unison-voice for maximum richness:
  - **Sync** — Hard self-sync with dual phase accumulators. Slave runs at up to 4x master frequency, hard-resets on master wrap. Creates classic formant-shifting buzz.
  - **Bend** — Asymmetric phase distortion via `pow(phase, exponent)` where exponent ranges 1-4. Shifts harmonics through nonlinear phase remapping (Casio CZ-style).
  - **FM** — Phase modulation from the other oscillator's previous sample output. Safe cross-routing allows mutual FM without ordering dependency.
  - **Window** — Windowed sync: same as Sync but with `sin(pi * masterPhase)` amplitude envelope per cycle, smoothing reset discontinuities for cleaner formant character.
- **New parameters**: `oscAWarpType` / `oscBWarpType` (Choice: Off, Sync, Bend, FM, Window) and `oscAWarpAmt` / `oscBWarpAmt` (Float 0-1) per oscillator.
- **Mod matrix destinations**: `OscA Warp` and `OscB Warp` added as modulation targets, enabling LFO/envelope-driven warp amount sweeps.

### Technical Notes
- Domain: DSP + GUI
- Per-voice FM cross-routing uses 1-sample delay (mono sum of other osc's stereo output) for stable mutual modulation
- Sync ratio range: 1x-4x (warp amount 0-100%)
- Bend exponent range: 1.0-4.0
- No breaking parameter changes — full backward compatibility with existing presets

## v1.10.0 (2026-03-08)

### Added
- **Wavetable Editor** (5th tab): Per-frame harmonic bar editing with real-time iFFT preview. Canvas-based frame strip with click/shift+click/ctrl+click multi-selection. Osc A/B toggle to edit either oscillator's table. Configurable bin count (32/64/128/256). Frame operations: Normalize (per-frame/global), Fade Edges, Reverse Audio, Reverse Order, Smooth (6dB/oct spectral rolloff). Save edited tables as new user wavetables. Undo/redo support (Ctrl+Z / Ctrl+Shift+Z, max 50 entries). DPR-aware canvas rendering for Retina displays.
- **Per-frame mipmap regeneration**: `WavetableGenerator::generateMipmapsForFrame()` regenerates all 10 mipmap levels for a single frame (~0.05ms vs ~12ms for full table), enabling real-time harmonic editing without audio glitches.
- **WavetableEditor C++ class**: Deep-copy working table management, FFT-based harmonic analysis with phase preservation, and 5 frame operations. Editor points oscillator at working copy via atomic pointer for live preview.
- 8 new native functions for WebView ↔ C++ communication: `startWavetableEditor`, `stopWavetableEditor`, `getEditorFrameWaveform`, `getFrameHarmonics`, `setFrameHarmonics`, `applyFrameOperation`, `saveEditedWavetable`, `getAllEditorFrameWaveforms`.

### Technical Notes
- Domain: Mixed (DSP + GUI)
- Milestone: add-wavetable-editor
- No new APVTS parameters — editor uses native functions for all state
- Full backward compatibility — no preset or parameter changes

## v1.9.0 (2026-03-08)

### Added
- **Custom wavetable import from .wav files**: FFT-based analysis slices audio into 2048-sample frames (up to 256 frames), builds band-limited mipmap hierarchy, and registers as a selectable user wavetable. Follows Serum's FFT 2048 import standard — short files produce fewer frames, long files truncate at 256 frames. Supports WAV, AIFF, FLAC via JUCE AudioFormatManager.
- **Drag-and-drop .wav import**: Drop audio files directly onto oscillator A or B canvas in the WebView UI. Files are read via HTML5 FileReader, base64-encoded, and decoded in C++ for FFT processing.
- **Persistent user wavetable storage**: Imported wavetables saved as 32-bit float WAV files in `~/.ouaricon/wavetables/` — survive sessions and plugin restarts. Loaded on plugin construction.
- **User wavetable management modal**: View and delete imported wavetables from the UI. Deletions auto-clear any active oscillator overrides.
- **User wavetable state persistence**: Active user table selections saved/restored in plugin state via `getStateInformation`/`setStateInformation` (backward compatible — old presets load without user tables).
- **New C++ classes**: `WavetableImporter` (FFT import pipeline), `UserWavetableManager` (persistent storage + registry). Non-APVTS override architecture preserves factory parameter range (0-27) for full backward compatibility.

## v1.8.1 (2026-03-06)

### Fixed
- **Broken oscillator & tuning visualizations**: `bindLfoSync()` used the old JUCE API `syncState.addListener({handleToggleStateChange})` instead of JUCE 8's `syncState.valueChangedEvent.addListener()`. The `TypeError` halted the ES module, preventing all subsequent code (WavetableDisplay, tuning system) from initializing. Introduced in v1.4.0 when tempo-synced LFO rates were added. Also fixes LFO sync toggle not reflecting state changes from DAW automation.

## v1.8.0 (2026-03-05)

### Added
- **Master stereo width control**: New `stereoWidth` parameter (0.0–2.0, default 1.0) applies mid-side processing after the effects chain and before master volume. 0.0 = mono, 1.0 = normal stereo, 2.0 = extra wide. Formula: `mid = (L+R)*0.5, side = (L-R)*0.5, L = mid + side*width, R = mid - side*width`. Uses per-sample smoothing to prevent zipper noise. Mono buffer fallback for single-channel hosts.

## v1.7.0 (2026-03-05)

### Changed
- **3-voice ensemble chorus**: Replaced `juce::dsp::Chorus` (single-voice) with custom `EnsembleChorus` engine. Three independent delay lines with staggered center delays (5ms, 7ms, 9ms), each modulated by sine LFOs at slightly different rates (1.0x, 0.93x, 1.07x) with 120-degree phase offsets. Equal-power stereo panning spreads voices across the stereo field (L/C/R at -0.6/0.0/+0.6). Max LFO modulation depth of 2ms. Wet gain normalized by 1/sqrt(3) for consistent output level. Same `chorusRate`, `chorusDepth`, `chorusMix` parameters — no preset breakage.

## v1.6.0 (2026-03-05)

### Added
- **Velocity curve parameter**: New `velocityCurve` choice parameter with 4 modes — Linear (default, unchanged behavior), Soft (sqrt curve, more dynamic range at low velocities), Hard (squared curve, requires harder hits), and Fixed (always full velocity regardless of key strike). Curve transformation applied in `startNote()` so `noteVelocity` is already curved before use in `renderNextBlock()`. Fully DAW-automatable.

## v1.5.0 (2026-03-05)

### Changed
- **Stereo noise generator**: `NoiseGenerator` now produces independent noise per channel via `getNextSampleStereo()`. White and Digital types use separate PRNG instances (randomL/randomR). Pink noise has independent Paul Kellet filter states per channel (b0L/b1L/b2L, b0R/b1R/b2R). Brown noise has independent integrator states. Vinyl has independent bandpass filters and crackle events per channel. Wind shares the LFO (coherent spectral sweep) but uses independent brown noise sources and lowpass filter states per channel. Previously a single mono sample was added identically to both L and R — now decorrelated noise provides true stereo width.

## v1.4.0 (2026-03-05)

### Added
- **Tempo-synced LFO rates**: Each of the 4 LFOs now has a Sync toggle and note Division selector. When Sync is enabled, LFO rate is calculated from host BPM instead of the free-running Hz knob. 18 note divisions available: straight (1/1 through 1/32), dotted (1/1D through 1/32D), and triplet (1/1T through 1/32T). BPM is read from the DAW transport via `getPlayHead()->getPosition()->getBpm()`. 8 new APVTS parameters: `lfo1Sync`, `lfo1Division`, `lfo2Sync`, `lfo2Division`, `lfo3Sync`, `lfo3Division`, `lfo4Sync`, `lfo4Division`. UI shows Free/Sync toggle per LFO — when synced, the rate knob hides and division dropdown appears.

## v1.3.0 (2026-03-05)

### Added
- **Pitch modulation destination**: Added "Pitch" as the 23rd mod destination in the modulation matrix. Routes any source (LFO, envelope, velocity, mod wheel, etc.) to pitch for vibrato, pitch envelopes, and velocity-to-pitch effects. Applied as a semitone offset (±12 semitones at full modulation) multiplied into oscillator and sub-oscillator frequency calculations.

## v1.2.2 (2026-03-05)

### Changed
- **Cached PrismVoice APVTS pointers**: Cache all 50 `std::atomic<float>*` parameter pointers once in `setAPVTS()` instead of performing string-based hash map lookups via `getRawParameterValue()` every audio block. `renderNextBlock()` (44 reads) and `startNote()` (24 reads) now do direct atomic loads. At 8 voices, eliminates ~352 hash map lookups per block.

## v1.2.1 (2026-03-05)

### Fixed
- **Report correct latency from distortion oversampling**: `DistortionProcessor` uses 2x oversampling which introduces latency, but `prepareToPlay()` called `setLatencySamples(0)`. Now reads `oversampling.getLatencyInSamples()` after preparing the distortion processor and reports it to the host so DAWs can apply proper delay compensation.

## v1.2.0 (2026-03-05)

### Changed
- **Cached ModulationMatrix APVTS pointers**: Cache all 64 `std::atomic<float>*` parameter pointers once in `setAPVTS()` instead of constructing 16 prefix strings and performing 64 hash map lookups every `processBlock` call. `updateFromAPVTS()` now does 64 direct atomic loads with zero string allocation or map traversal.

## v1.1.9 (2026-03-04)

### Changed
- **Per-block oscillator tuning reads**: Moved `oscACoarse`, `oscAFine`, `oscBCoarse`, `oscBFine` APVTS reads from per-sample to per-block in `PrismVoice::renderNextBlock`. Precompute pitch ratios (`std::pow`) once per block instead of every sample — eliminates 4 atomic loads and 2 `std::pow` calls per sample per voice.

## v1.1.8 (2026-03-04)

### Changed
- **SVFFilter coefficient caching**: Added dirty-flag to `SVFFilter` so `updateCoefficients()` (which computes `std::tan()`) only runs when cutoff or resonance actually change. Previously `setCutoff()` and `setResonance()` each triggered a full recompute — 8 `std::tan()` calls per sample per voice. Now deferred to `processSample()` with value-change detection: 2x reduction when modulated, zero cost when static.

## v1.1.7 (2026-03-04)

### Removed
- **Deprecated compatibility stubs**: Removed `connectMTSClient()` (always returned false with a DBG message) and dual-arg `loadScalaFile(File&, File&)` (ignored second argument, delegated to single-arg overload). Neither had any callers.

## v1.1.6 (2026-03-04)

### Changed
- **JSON array helpers**: Extracted `toJsonArray` (template with lambda) and `toJsonFloatArray` (strided raw pointer) helpers in PluginEditor.cpp — replaced 11 instances of manual `"[" + for-loop + "]"` JSON string building across `addNativeFunctions()` and `timerCallback()`

## v1.1.5 (2026-03-04)

### Changed
- **Shared math constants**: Consolidated `kPi`, `kTwoPi`, `kHalfPi` definitions from 8 source files into a single `dsp/MathConstants.h` header. Removed 10 duplicate `static constexpr` locals across WavetableOscillator, SubOscillator, NoiseGenerator, DistortionProcessor, PrismVoice, SVFFilter, WavetableFactory, and WavetableGenerator.

## v1.1.4 (2026-03-04)

### Removed
- **Dead parameter**: Removed unused `polyphony` APVTS parameter — was defined in `createGlobalParameters()` and bound in UI footer but never read by processBlock or voice management. Synth always uses 16 voices. Removed from PluginProcessor.cpp, PrismParamIds.h, and WebView UI footer.

## v1.1.3 (2026-03-04)

### Removed
- **Dead code**: Removed unused `prevPhase` variable in LFO.cpp — was assigned from `phase` but never read

## v1.1.2 (2026-03-04)

### Removed
- **Dead code**: Removed unused `activeNotesMutex` from PluginProcessor — note tracking already uses lock-free `std::atomic<bool>` array, the mutex was declared but never locked anywhere

## v1.1.1 (2026-03-04)

### Changed
- **Knob visual overhaul**: Replaced all 63 knobs from CSS conic-gradient rotary style to SVG vine-arc style (matching O-Detune). Green vine stroke (#5a7a6a) animates around a tan track with smooth requestAnimationFrame interpolation. Three sizes: standard (52px), small (44px, footer), large (64px, A4 ref pitch). Added mouse wheel support and double-click reset to all knobs.

## v1.0.1 (2026-03-03)

### Fixed
- **Sticky unison knobs**: Osc A/B Unison knobs required ~25px of drag to change by one step, making them feel stuck. Added adaptive drag sensitivity — discrete parameters (≤16 steps) now require ~8px per step instead. Continuous knobs are unaffected.

## v1.0.0 (2026-02-23)

### Breaking Changes
- Removed `lfo1Depth`, `lfo2Depth`, `lfo1Dest`, `lfo2Dest` APVTS parameters (replaced by modulation matrix)
- Presets saved with v0.12.0 will lose LFO depth/destination settings on load; re-create them as mod matrix routes

### Added
- **16-slot modulation matrix** with per-sample evaluation in each voice
- 9 modulation sources: None, LFO1, LFO2, AmpEnv, FilterEnv, Velocity, NoteNum, ModWheel, Aftertouch
- 21 modulation destinations: None, OscA/B Position, FiltA/B Cutoff, FiltA/B Resonance, Osc Mix, Sub Level, Noise Level, LFO1/2 Rate, OscA/B Detune, OscA/B Pan, Reverb/Delay/Chorus/Dist Mix, Master Vol
- Each slot has: source selector, destination selector, bipolar amount (-100% to +100%), on/off toggle
- 64 new APVTS parameters (4 per slot x 16 slots), all fully DAW-automatable
- MIDI ModWheel (CC1) and Channel Aftertouch captured as global mod sources
- New "Mod" tab in WebView UI with interactive routing list (dropdowns + sliders)
- `ModulationMatrix` DSP class (`Source/dsp/ModulationMatrix.h/.cpp`) with fixed-size arrays for zero-allocation audio-thread operation

### Changed
- LFO sections in Synth tab now show Rate + Shape only (routing moved to Mod tab)
- Filter resonance now modulatable per-sample via mod matrix (previously static per-block)
- Pan modulation now computed per-sample when mod routes target OscA/B Pan

### Technical Notes
- Mod matrix routes evaluated per-sample inside `PrismVoice::renderNextBlock` for click-free modulation
- Source values computed once per sample, then all 16 slots iterated (early-out for disabled/None slots)
- Cutoff modulation uses multiplicative octave-scaling: `cutoff * pow(2, modOffset * 4)` matching the filter envelope pattern
- Additive modulation for position/level/pan destinations, clamped to valid ranges
- Processor stores ModWheel/Aftertouch as `std::atomic<float>`, read by voices each sample

## v0.12.0 (2026-02-24)

### Added
- **LFO system** with 2 independent per-voice LFOs for smooth per-sample modulation
- LFO1 hardcoded to modulate Osc A wavetable position, LFO2 hardcoded to modulate Filter A cutoff
- Each LFO has Rate (0.01–20 Hz, skewed), Shape (Sine/Triangle/Saw/Square/S&H), Depth (0–100%), and Dest selector
- 8 new APVTS parameters: `lfo1Rate`, `lfo1Shape`, `lfo1Depth`, `lfo1Dest`, `lfo2Rate`, `lfo2Shape`, `lfo2Depth`, `lfo2Dest`
- Generic reusable `LFO` DSP class (`Source/dsp/LFO.h/.cpp`) with phase accumulator design — ready for future modulation matrix
- Dest parameters included as Choice params (Osc A Pos / Osc B Pos / Filt A Cut / Filt B Cut / Pitch) for future routing
- WebView UI: LFO 1 and LFO 2 sections in Synth tab with rate knobs, shape/dest dropdowns, and depth knobs

### Technical Notes
- LFO modulation applied per-sample inside `PrismVoice::renderNextBlock` for click-free smooth modulation
- LFO1 applies additive modulation to wavetable position: `pos + lfoVal * depth`, clamped [0,1]
- LFO2 applies multiplicative modulation to filter cutoff: `cutoff * pow(2, lfoVal * depth * 4)` — same pattern as filter envelope
- LFOs reset phase on note-on for consistent attack character
- S&H shape triggers new random value on phase wrap

## v0.11.0 (2026-02-23)

### Added
- Expanded factory wavetable library from 4 single-frame tables to 28 multi-frame wavetables across 5 categories
- **Analog** (3 new): PWM Sweep, Supersaw, Sync Sweep (32 frames each)
- **Digital** (5 new): FM E.Piano, FM Bell, FM Metallic, Wavefold, Bitcrush (32 frames each)
- **Formant** (4 new): Vowel Morph (64 frames), Choir Pad (48 frames), Vocal Lead (32 frames), Formant Filter (32 frames)
- **Spectral** (6 new): Harmonic Series, Spectral Tilt, Odd Harmonics, Harmonic Stretch, Comb Sweep, Prism Spectrum (32 frames each)
- **Organic** (6 new): Breath, Plucked String, Church Bell, Organ Sweep, Wind, Filtered Noise (16-32 frames each)
- Categorized wavetable dropdown menus with optgroup sections
- WavetableFactory class for procedural multi-frame table generation
- All tables generated procedurally with deterministic RNG seeds

### Changed
- Wavetable selector range expanded from 0-3 to 0-27
- Position knob now sweeps through multiple frames per table for musically useful morphing
- Original 4 tables (Saw, Square, Triangle, Sine) preserved at indices 0-3 for preset compatibility

## v0.9.2 (2026-02-18)

### Fixed
- **Stereo filter distortion**: Mono filter + stereo balance reconstruction caused full-wave rectification on left channel and 3x amplification on right channel during negative signal excursions. All waveforms were severely distorted (sine sounded like square). Replaced with true stereo filter processing using independent L/R filter instances.
- **Wavetable selection mapping**: oscATable/oscBTable parameter range was [0, 15] but only 4 factory tables exist. UI dropdown normalized values mapped incorrectly — selecting Square or Triangle both loaded the Sine table. Fixed parameter range to [0, 3] matching the 4 factory waveforms.

## v0.9.1 (2026-02-18)

- Initial release with tuning panel v2.0.0
