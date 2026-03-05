# O-Prism Changelog

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
