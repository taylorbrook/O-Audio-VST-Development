# Stage 2: DSP - Execution Plan

**Created:** 2026-04-05
**Plugin:** O-Wind
**Stage:** 2 (DSP)
**Phases:** 3.1, 3.2, 3.3, 3.4

## Goal

Implement the complete DSP engine for O-Wind: a jet-drive waveguide flute physical model with bidirectional bore, two-tier tone holes, per-voice 2x oversampling, 8 instrument presets, impossible physics parameters, and MPE polyphony. All 4 DSP phases executed sequentially -- each phase builds on the previous, validated by build + listening test before advancing.

**Architecture contract:** `plugins/O-Wind/.planning/research/ARCHITECTURE.md`
**Research:** `plugins/O-Wind/.planning/stages/2-dsp/RESEARCH.md`

---

## Phase 3.1: Minimal Oscillating Model

**Goal:** Single monophonic voice producing a flute-like tone with jet + bore feedback loop. Validate self-oscillation and pitch tracking before adding complexity.

**Parameters active:** BREATH_PRESSURE, EMBOUCHURE, TONE_COLOR, JET_REFLECTION, END_REFLECTION, OUTPUT_LEVEL

### Tasks

1. [ ] Create `Source/DSP/JetExciter.h` -- breath model + embouchure summation
   - Breath pressure -> jet velocity via `pow(pressure, 1.5f)` (Bernoulli)
   - Embouchure summation: `jetVelocity + boreFeedback * jetReflection`
   - Velocity attack ramp (fixed 10ms for Phase 3.1)
   - Velocity release ramp (fixed 50ms for Phase 3.1)
   - `prepare(double sampleRate)`, `reset()`, `processSample(float boreFeedback, ...)` API
   - No noise, no vibrato yet (Phase 3.2)
   - Depends on: none

2. [ ] Create `Source/DSP/JetNonlinearity.h` -- tanh saturation
   - `output = tanh(jetGain * input)` with jetGain default 2.0f
   - Input clamp to [-3, 3] for safety
   - REVERSED_JET stub (param read but not active -- always 0 in 3.1)
   - Inline header-only, no .cpp needed
   - Depends on: none

3. [ ] Create `Source/DSP/DCBlocker.h` -- inline DC removal
   - `y[n] = x[n] - x[n-1] + 0.995 * y[n-1]`
   - State: `xPrev`, `yPrev` floats
   - `reset()`, `processSample(float)` API
   - Header-only
   - Depends on: none

4. [ ] Create `Source/DSP/BoreWaveguide.h` -- bidirectional Thiran delay lines + filters
   - Two `DelayLine<float, Thiran>` (forward + backward), max 2048 samples
   - Bore loss filter: `IIR::Filter<float>` 2nd-order lowpass (TONE_COLOR -> cutoff 1000-12000 Hz log)
   - End reflection filter: `IIR::Filter<float>` 1st-order lowpass with sign inversion
   - Radiation filter: `IIR::Filter<float>` 1st-order highpass (~300 Hz default)
   - `prepare(double sampleRate)`, `reset()`, `setNoteDelay(float totalDelay)`, `processSample(float input, float endReflectionCoeff) -> {voiceOutput, feedback}` API
   - Tier 1 tone holes: MIDI note -> bore delay length via lookup table (no crossfade yet)
   - Filter group delay compensation on bore delay (subtract ~2 samples, clamp min 4.0f)
   - Depends on: none

5. [ ] Create `Source/FluteSynthVoice.h` and `Source/FluteSynthVoice.cpp` -- voice orchestrator
   - Extends `juce::SynthesiserVoice`, constructor takes `APVTS*` + `TuningEngine*`
   - Owns: JetExciter, JetNonlinearity, DCBlocker, BoreWaveguide
   - `prepareToPlay(double sampleRate, int maxBlockSize)` -- prepare all components
   - `startNote()`: compute bore delay from `tuningEngine->getFrequency(midiNote)`, start breath attack
   - `stopNote()`: start breath release, `clearCurrentNote()` when energy decayed
   - `renderNextBlock()`: read params once via `getRawParameterValue()->load()`, per-sample loop:
     - JetExciter -> JetNonlinearity -> DCBlocker -> BoreWaveguide -> output
   - `pitchWheelMoved()`, `controllerMoved()` stubs (active in Phase 3.2)
   - Hard-clip output: `jlimit(-2.0f, 2.0f, sample)`
   - Write to all channels: `outputBuffer.addSample(ch, startSample, sample)`
   - Files: `Source/FluteSynthVoice.h`, `Source/FluteSynthVoice.cpp`
   - Depends on: Tasks 1-4

6. [ ] Wire `PluginProcessor` -- voice creation, MIDI routing, output level
   - `prepareToPlay()`: clear voices, create 1 FluteSynthVoice, call `voice->prepareToPlay()`
   - `processBlock()`: `ScopedNoDenormals`, `synthesiser.renderNextBlock()`, apply OUTPUT_LEVEL (dB->linear)
   - Add `#include "FluteSynthVoice.h"` to PluginProcessor.cpp
   - Remove Stage 1 placeholder `buffer.clear()` and `ignoreUnused`
   - Files: `Source/PluginProcessor.h`, `Source/PluginProcessor.cpp`
   - Depends on: Task 5

7. [ ] Build and validate Phase 3.1
   - `ninja O-Wind_VST3 O-Wind_AU` -- zero errors
   - Install to system folders, clear AU cache
   - Load in DAW: MIDI note-on produces audible tone
   - Pitch tracks C4-C7 range
   - BREATH_PRESSURE, EMBOUCHURE, TONE_COLOR affect sound
   - No DC drift or runaway oscillation
   - Note-off produces natural decay
   - Depends on: Task 6

### Phase 3.1 Success Criteria

- [ ] MIDI note-on produces audible flute-like tone
- [ ] Pitch tracks correctly across C4-C7
- [ ] BREATH_PRESSURE affects volume and tone quality
- [ ] EMBOUCHURE changes register at extremes
- [ ] TONE_COLOR affects brightness
- [ ] JET_REFLECTION and END_REFLECTION affect sustain character
- [ ] Note-off triggers natural breath decay
- [ ] No DC drift or runaway oscillation
- [ ] No clicks on note transitions (basic)

---

## Phase 3.2: Expression + Oversampling

**Goal:** Add breath noise, vibrato, 2x oversampling, parameter smoothing, polyphony, crossfade tone holes, CC/MPE mapping, and attack/release envelopes.

**Parameters active:** BREATH_PRESSURE, EMBOUCHURE, BREATH_NOISE, TONE_COLOR, AIR_COLUMN, JET_REFLECTION, END_REFLECTION, VIBRATO_RATE, VIBRATO_DEPTH, WIDTH, OUTPUT_LEVEL

### Tasks

8. [ ] Add turbulence noise to `JetExciter`
   - `juce::Random` noise source + `IIR::Filter<float>` 1st-order lowpass shaping
   - Amplitude: `noiseGain = breathNoiseParam * jetVelocity * jetVelocity` (quadratic scaling)
   - Noise filter cutoff proportional to jet velocity (1000-6000 Hz)
   - Inject at embouchure summation: `excitation = jetVel + noise + boreFeedback * jetReflection`
   - BREATH_NOISE parameter connection
   - Depends on: Phase 3.1 complete (Task 7)

9. [ ] Add vibrato LFO to `JetExciter`
   - Sine LFO modulating breath pressure: `p_breath * (1 + depth * sin(2*pi*rate*t))`
   - VIBRATO_RATE (2-8 Hz), VIBRATO_DEPTH (0-1) parameter reads
   - Phase counter incremented per sample, reset on note start
   - Pressure vibrato (NOT pitch vibrato) -- authentic flute vibrato
   - Depends on: Task 8

10. [ ] Add per-voice 2x oversampling to `FluteSynthVoice`
    - `juce::dsp::Oversampling<float>` per voice (1 channel, 2x, polyphase IIR)
    - Temp buffers: `excitationBuffer` (native rate), `outputBuffer` (native rate)
    - renderNextBlock phases:
      1. Generate excitation at native rate into excitationBuffer
      2. Upsample via `processSamplesUp()`
      3. Per-sample waveguide at 2x rate (jet delay, tanh, DC block, bore, loss, reflection)
      4. Downsample via `processSamplesDown()` into outputBuffer
      5. Write output to buffer
    - All DSP components prepared at `sampleRate * 2.0` (internal rate)
    - Bore delay table recalculated at internal rate
    - `oversampling.initProcessing(maxBlockSize)` in prepareToPlay
    - Depends on: Task 9

11. [ ] Add SmoothedValue crossfade for bore delay (Tier 1 tone holes)
    - `SmoothedValue<float>` on bore delay, `reset(internalSampleRate, 0.003)` (3ms ramp)
    - On note change: `boreDelaySmoothed.setTargetValue(newDelay)`
    - Per oversampled sample: `getNextValue()` feeds bore forward/backward delay lengths
    - SmoothedValue on embouchure param (prevents register jumps), 5ms ramp
    - Depends on: Task 10

12. [ ] Add breath attack/release envelopes
    - Attack ramp: velocity 127 = 5ms, velocity 1 = 30ms (linear interpolation)
    - Release ramp: 50ms default (on note-off, ramp breath pressure to 0)
    - `clearCurrentNote()` when release complete AND bore energy < threshold
    - Energy detection: `abs(voiceOutput) < 0.0001f` for N consecutive samples
    - Depends on: Task 10

13. [ ] Add polyphony (4 default, 8 max)
    - `prepareToPlay()`: create 8 voices, all prepared
    - Synthesiser handles voice stealing
    - Each voice owns independent DSP state + oversampling instance
    - Memory: 8 voices * (2 DelayLine@2048 + Oversampling + filters) -- trivial (~64KB total)
    - Depends on: Tasks 10-12

14. [ ] Add CC/MPE mapping in `FluteSynthVoice`
    - `controllerMoved()`: CC2 -> breath pressure, CC74 -> embouchure, CC1 -> vibrato depth, CC11 -> output level
    - `pitchWheelMoved()`: pitch bend -> bore delay offset (configurable semitones, default +/- 2)
    - Aftertouch -> breath pressure (via `channelPressureChanged()` if available, else manual MIDI parse)
    - Store CC values as atomics, read in `updateParametersFromAPVTS()`
    - APVTS params take priority; CC overrides only when non-zero
    - Depends on: Task 13

15. [ ] Report oversampling latency to host
    - `setLatencySamples(static_cast<int>(std::ceil(oversampling.getLatencyInSamples())))` in prepareToPlay
    - Use first voice's oversampling instance for latency query
    - Depends on: Task 13

16. [ ] Build and validate Phase 3.2
    - Zero build errors
    - Breath noise audible, scales with dynamics
    - Vibrato sounds natural (pressure modulation)
    - No aliasing artifacts (A/B oversampling on/off)
    - Tone hole transitions click-free (3ms crossfade)
    - 4-voice polyphony stable, no CPU spikes
    - CC2 maps to dynamics, CC74 maps to embouchure
    - Velocity affects attack character
    - Latency reported correctly
    - Depends on: Tasks 8-15

### Phase 3.2 Success Criteria

- [ ] Breath noise audible and scales with playing dynamics
- [ ] Vibrato sounds natural (pressure + amplitude modulation)
- [ ] No aliasing artifacts (oversampling working)
- [ ] Tone hole transitions click-free (crossfade)
- [ ] 4-voice polyphony works without CPU spikes
- [ ] CC2 (breath controller) maps to dynamics
- [ ] CC74 maps to embouchure/timbre
- [ ] Velocity affects attack character
- [ ] Latency reported correctly to host

---

## Phase 3.3: Impossible Physics + Instrument Presets

**Goal:** Add creative parameters (infinite sustain, reversed jet, sub-harmonics), 4 core instrument presets, stereo width, and AIR_COLUMN connection.

**Parameters active:** All 14 plugin parameters

### Tasks

17. [ ] Connect INFINITE_SUSTAIN to bore loss filter
    - In `BoreWaveguide`: bore loss filter gain approaches 1.0 as INFINITE_SUSTAIN approaches 1.0
    - `effectiveLoss = baseLoss * (1.0f - infiniteSustainParam * 0.95f)` -- never fully zero loss (stability)
    - Modifies filter Q and cutoff to reduce energy drain
    - Depends on: Phase 3.2 complete (Task 16)

18. [ ] Connect REVERSED_JET in `JetNonlinearity`
    - Phase-invert jet signal before tanh: `effectiveJet = jmap(reversedJetParam, jetOutput, -jetOutput)`
    - `labiumOutput = tanh(jetGain * effectiveJet)`
    - Changes phase relationship between jet and bore -> different resonance modes
    - Depends on: Task 17

19. [ ] Create `Source/DSP/SubHarmonics.h` -- sub-octave generator
    - Asymmetric soft-clipping in feedback path (pre-radiation):
      - Positive half: pass through
      - Negative half: `* 0.5f` (reduce)
    - Blend: `pFiltered + subParam * (asymmetric - pFiltered)`
    - Period-doubling effect creates sub-octave content
    - SUB_HARMONICS param (0 = bypass, 1 = prominent)
    - Integrate into `BoreWaveguide::processSample()` or `FluteSynthVoice` loop
    - Depends on: Task 17

20. [ ] Connect AIR_COLUMN parameter to bore loss
    - AIR_COLUMN (0-1) controls viscothermal loss amount
    - 0 = bright (minimal HF loss, high cutoff)
    - 1 = warm (heavy HF loss, low cutoff)
    - Combined with TONE_COLOR in bore loss filter coefficient calculation
    - `effectiveCutoff = toneColor * (1.0f - airColumn * 0.7f)` (reduces cutoff range with more air column)
    - Depends on: Task 17

21. [ ] Create `Source/DSP/InstrumentPresets.h` -- 4 core preset parameter sets
    - `InstrumentPreset` struct: jetGain, noiseLevel, noiseCutoffBase, radiationCutoff, boreLossCutoff, boreLossQ, endReflCutoff, embouchureMin, embouchureMax, defaultBreath, attackTimeMs
    - Concert Flute, Shakuhachi, Bansuri, Native American Flute (values from RESEARCH.md)
    - `getPreset(int index)` accessor
    - Voice applies preset values as internal DSP coefficients (not overriding APVTS params -- preset tunes the internal physics model, user params sit on top)
    - Preset selection stored in processor state (ValueTree, not APVTS)
    - Depends on: Task 17

22. [ ] Create `Source/DSP/StereoWidth.h` -- post-voice stereo processor
    - Decorrelation allpass on right channel: `IIR::Filter<float>::makeFirstOrderAllPass(sampleRate, 2000.0f)`
    - Mid-side matrix: `M = (L+R)/2, S = (L-R)/2`
    - `L_out = M + S * width, R_out = M - S * width`
    - WIDTH param (0 = mono, 1 = natural, 2 = wide)
    - Applied in `processBlock()` after `synthesiser.renderNextBlock()`
    - `prepare(double sampleRate)`, `reset()`, `process(AudioBuffer<float>&, int numSamples)`
    - Depends on: Task 17

23. [ ] Wire presets + stereo + impossible physics into processor
    - Processor: preset selection state, stereo width processing after voice render
    - Voice: read preset on startNote, apply internal coefficients
    - processBlock order: renderNextBlock -> output level (dB->linear) -> stereo width
    - getStateInformation/setStateInformation: save/restore preset index
    - Depends on: Tasks 17-22

24. [ ] Build and validate Phase 3.3
    - INFINITE_SUSTAIN creates drone-like endless decay
    - REVERSED_JET produces unusual timbres (no crashes)
    - SUB_HARMONICS adds audible sub-octave
    - Each preset sounds distinctly different
    - Stereo width works (mono to wide sweep)
    - Preset switching is click-free
    - All 14 params connected and functional
    - Depends on: Task 23

### Phase 3.3 Success Criteria

- [ ] INFINITE_SUSTAIN creates drone-like endless decay
- [ ] REVERSED_JET produces unusual timbres without crashes
- [ ] SUB_HARMONICS adds audible sub-octave content
- [ ] Concert Flute: clear and projecting
- [ ] Shakuhachi: breathy, expressive character
- [ ] Bansuri: warm, airy quality
- [ ] Native American Flute: warm, haunting tone
- [ ] Stereo width functional (mono to wide)
- [ ] Preset switching is click-free
- [ ] AIR_COLUMN affects resonance character

---

## Phase 3.4: Advanced Features

**Goal:** Tier 2 tone holes (Keefe scattering), expansion presets, full MPE refinement, tuning system integration.

### Tasks

25. [ ] Create `Source/DSP/ToneHoleSystem.h` -- Tier 2 Keefe 3-port scattering
    - `ToneHoleJunction` struct: open/closed `IIR::Filter<float>` (2nd-order), openAmount (0-1), reflectionCoeff, transmissionCoeff
    - 8 junctions inline with bore waveguide
    - Scattering: `R = (Z_hole - Z_bore) / (Z_hole + Z_bore)`, `T = 1 + R`
    - Half-holing: continuous 0-1 parameter sweeps impedance for pitch bending
    - Cross-fingering: alternate fingering patterns via junction open/close states
    - Fingering table: maps MIDI note -> array of 8 open/close states
    - `prepare()`, `reset()`, `processSample(float& pPlus, float& pMinus)` API
    - Integrate into bore waveguide loop (between bore forward delay and loss filter)
    - Toggle: Tier 1 (bore length switch) vs Tier 2 (scattering) selectable
    - CPU: ~64 ops per sample for 8 junctions at 2x oversampling
    - Depends on: Phase 3.3 complete (Task 24)

26. [ ] Add expansion presets to `InstrumentPresets.h`
    - Recorder: tight embouchure (0.42-0.48), low noise, bright
    - Pan Flute: breathy (high noise 0.40), low jet gain, warm
    - Piccolo: high jet gain (2.2), high radiation cutoff (500 Hz), bright/piercing
    - Ocarina: low noise (0.05), moderate jet gain, mellow
    - Values from RESEARCH.md preset table
    - Each preset may also define a Tier 2 fingering table variant
    - Depends on: Task 25

27. [ ] Full MPE integration
    - Per-note pitch bend: smooth bore delay modulation via SmoothedValue (portamento)
    - Per-note slide (Y): embouchure control per voice
    - Per-note pressure (Z): breath pressure per voice
    - MPE zone configuration (lower/upper zone awareness)
    - `controllerMoved()` and `pitchWheelMoved()` fully wired
    - CC1 -> vibrato depth mapping
    - Depends on: Task 25

28. [ ] Tuning system integration
    - Wire `tuningEngine` pointer to each voice (already passed in constructor)
    - Voice `startNote()`: `tuningEngine->getFrequency(midiNote)` for bore delay calculation
    - Bore delay table recalculated when tuning changes
    - REFERENCE_PITCH and TUNING_SYSTEM APVTS params connected to TuningEngine
    - Scala/TUN file loading via existing shared module
    - Depends on: Task 25

29. [ ] Build and validate Phase 3.4
    - Tier 2 tone holes produce more realistic timbral variation
    - Half-holing enables smooth pitch bends
    - Cross-fingering produces alternate timbres
    - Expansion presets playable and distinct
    - MPE per-note expression works correctly
    - Tuning system accepts Scala files
    - CPU: 4 voices < 14% with Tier 2 enabled
    - No stability issues
    - Depends on: Tasks 25-28

### Phase 3.4 Success Criteria

- [ ] Tier 2 tone holes produce more realistic timbral variation
- [ ] Half-holing enables smooth pitch bends (shakuhachi meri/kari)
- [ ] Cross-fingering produces alternate timbres for same pitch
- [ ] Expansion presets are playable and distinct
- [ ] MPE per-note expression works correctly
- [ ] Tuning system accepts Scala files
- [ ] CPU within budget (4 voices < 14% with Tier 2)
- [ ] No stability issues with full tone hole scattering

---

## Files Created/Modified Summary

### New Files (13)

| File | Phase | Purpose |
|------|-------|---------|
| `Source/DSP/JetExciter.h` | 3.1 | Breath model, noise, vibrato LFO |
| `Source/DSP/JetNonlinearity.h` | 3.1 | tanh saturation, reversed jet |
| `Source/DSP/DCBlocker.h` | 3.1 | Inline DC removal |
| `Source/DSP/BoreWaveguide.h` | 3.1 | Bidirectional Thiran delay + filters |
| `Source/FluteSynthVoice.h` | 3.1 | Voice header |
| `Source/FluteSynthVoice.cpp` | 3.1 | Voice implementation |
| `Source/DSP/SubHarmonics.h` | 3.3 | Sub-octave generator |
| `Source/DSP/InstrumentPresets.h` | 3.3 | 4+4 preset parameter sets |
| `Source/DSP/StereoWidth.h` | 3.3 | Post-voice stereo processing |
| `Source/DSP/ToneHoleSystem.h` | 3.4 | Tier 2 Keefe scattering junctions |

### Modified Files (2)

| File | Phase | Changes |
|------|-------|---------|
| `Source/PluginProcessor.h` | 3.1+ | Add StereoWidth member, preset state |
| `Source/PluginProcessor.cpp` | 3.1+ | Voice creation, processBlock routing, state persistence |

---

## Dependency Graph

```
Phase 3.1 (Tasks 1-7):
  [1] JetExciter ──┐
  [2] JetNonlin. ──┤
  [3] DCBlocker  ──┼──> [5] FluteSynthVoice ──> [6] Processor Wiring ──> [7] Build/Validate
  [4] BoreWaveguide┘

Phase 3.2 (Tasks 8-16):  [7] ──> [8] Noise ──> [9] Vibrato ──> [10] Oversampling ──> [11] SmoothedValue
                                                                                    ──> [12] Envelopes
                                                                  [10-12] ──> [13] Polyphony ──> [14] CC/MPE
                                                                                              ──> [15] Latency
                                                                  [8-15] ──> [16] Build/Validate

Phase 3.3 (Tasks 17-24): [16] ──> [17] InfSustain ──> [18] ReversedJet
                                                    ──> [19] SubHarmonics
                                                    ──> [20] AirColumn
                                                    ──> [21] Presets
                                                    ──> [22] StereoWidth
                                   [17-22] ──> [23] Wire All ──> [24] Build/Validate

Phase 3.4 (Tasks 25-29): [24] ──> [25] Tier2 ToneHoles ──> [26] ExpansionPresets
                                                          ──> [27] MPE
                                                          ──> [28] Tuning
                                   [25-28] ──> [29] Build/Validate
```

## Execution Notes

- **O-Bowed reference:** Follow the split-DSP-component pattern from `plugins/O-Bowed/Source/DSP/`
- **Phase 3.1 no oversampling:** Run waveguide at native rate to validate oscillation first. Oversampling wraps in Phase 3.2.
- **Header-only DSP components:** JetNonlinearity, DCBlocker, SubHarmonics are trivial enough for header-only. JetExciter and BoreWaveguide may need .h only or .h+.cpp depending on complexity.
- **Per-block param reads:** All APVTS parameters read once per block via `getRawParameterValue()->load()`, NOT per sample.
- **Audio thread safety:** No allocations in renderNextBlock. `setMaximumDelayInSamples()` only in prepareToPlay.
- **`ScopedNoDenormals`** in processBlock handles filter denormals; no need for per-filter `snapToZero()` calls.
