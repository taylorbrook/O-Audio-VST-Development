# Stage 2: DSP - Execution Plan

**Plugin:** O-Prism (Microtonal Wavetable Synthesizer)
**Created:** 2026-02-16
**Phases:** 5 (2.1 through 2.5)
**Total Tasks:** 28

---

## Goal

Implement the complete DSP engine for O-Prism across 5 phases: wavetable playback, mipmap anti-aliasing, unison/sub/noise/glide, dual filters with envelope, and effects chain with master output. At the end, playing a MIDI note should produce a professional-quality wavetable synthesizer sound with filters, effects, and microtonal support.

---

## Phase 2.1: Basic Wavetable Playback (6 tasks)

**Phase Goal:** Single wavetable oscillator playing notes with ADSR — the minimum viable synthesizer.

### Task 1: Create WavetableData structure
- **Files:** `Source/dsp/WavetableData.h` (NEW)
- **Depends on:** None
- **Details:**
  - `WavetableData` struct: flat `std::vector<float>` storage
  - Constants: `kTableSize=2048`, `kGuardSamples=1`, `kFrameSize=2049`, `kMaxFrames=256`, `kNumMipmapLevels=10`
  - `getSample(level, frame, sampleIndex)` accessor
  - `numFrames` member
  - Memory layout: `[level][frame][sample+guard]`
  - Guard sample = copy of first sample per frame (eliminates modulo in inner loop)

### Task 2: Create procedural wavetable generator
- **Files:** `Source/dsp/WavetableGenerator.h`, `Source/dsp/WavetableGenerator.cpp` (NEW)
- **Depends on:** Task 1
- **Details:**
  - Generate 4 factory test tables using additive synthesis (NOT naive formulas — alias-free source):
    - **Saw:** `sum(sin(n*2pi*i/2048) / n)` for n=1..1024, normalize
    - **Square:** odd harmonics only, `sum(sin(n*2pi*i/2048) / n)` for odd n
    - **Triangle:** odd harmonics, `sum(sign * sin(n*2pi*i/2048) / n^2)` for odd n
    - **Sine:** single harmonic `sin(2pi*i/2048)`
  - Each table = single frame (single-cycle), stored as WavetableData with 1 frame
  - Store at mipmap level 0 only (mipmap generation deferred to Phase 2.2)
  - Set guard samples for each frame
  - Static method: `generateProceduralTable(WaveShape shape) -> std::unique_ptr<WavetableData>`

### Task 3: Create WavetableOscillator class
- **Files:** `Source/dsp/WavetableOscillator.h`, `Source/dsp/WavetableOscillator.cpp` (NEW)
- **Depends on:** Task 1
- **Details:**
  - 64-bit double precision phase accumulator
  - `setFrequency(double freq)` → calculates `phaseIncrement = freq / sampleRate`
  - `setPosition(float pos)` → wavetable frame position 0.0–1.0
  - `prepare(double sampleRate)`
  - `getNextSample() -> double` — core rendering:
    - Phase accumulator increment with wrap
    - Sample position = `phase * 2048.0`, use guard sample for wrap-free linear interp
    - Frame interpolation between frame0/frame1 using position
    - For Phase 2.1: read from mipmap level 0 only (no level selection yet)
  - `setWavetable(const WavetableData* table)` — atomic-safe pointer set
  - `reset()` — reset phase accumulator

### Task 4: Implement amplitude ADSR in PrismVoice
- **Files:** `Source/PrismVoice.h` (MODIFY), `Source/PrismVoice.cpp` (MODIFY)
- **Depends on:** None
- **Details:**
  - Add `juce::ADSR ampEnvelope` member
  - In `prepare()`: call `ampEnvelope.setSampleRate(sampleRate)` BEFORE setParameters
  - In `startNote()`: read APVTS amp params, call `ampEnvelope.setParameters(...)`, call `ampEnvelope.noteOn()`
  - In `stopNote()`: call `ampEnvelope.noteOff()` when `allowTailOff=true`; instant kill when false
  - Per-sample in `renderNextBlock()`: `float envVal = ampEnvelope.getNextSample()` — cast to double
  - Voice lifecycle: `if (!ampEnvelope.isActive()) clearCurrentNote()`

### Task 5: Implement Osc A rendering in PrismVoice
- **Files:** `Source/PrismVoice.h` (MODIFY), `Source/PrismVoice.cpp` (MODIFY)
- **Depends on:** Tasks 3, 4
- **Details:**
  - Add `WavetableOscillator oscA` member
  - In `prepare()`: `oscA.prepare(sampleRate)`
  - In `startNote()`:
    - Get frequency from TuningEngine: `tuningEngine->getFrequency(midiNote)`
    - Apply coarse/fine: `freq *= pow(2.0, (coarse + fine/100.0) / 12.0)`
    - `oscA.setFrequency(freq)` and `oscA.reset()` (if phase reset mode)
  - In `renderNextBlock()`:
    - Read `oscAPos` from APVTS
    - Per-sample loop: `double sample = oscA.getNextSample() * envVal * velocity`
    - Write to outputBuffer (both channels, mono for now)
  - Verify: playing MIDI notes produces sound

### Task 6: Wire up procedural tables and update CMakeLists.txt
- **Files:** `CMakeLists.txt` (MODIFY), `Source/PluginProcessor.h` (MODIFY), `Source/PluginProcessor.cpp` (MODIFY)
- **Depends on:** Tasks 2, 5
- **Details:**
  - Add all new source files to `target_sources()` in CMakeLists.txt
  - In PluginProcessor: generate 4 procedural tables at construction
  - Store as `std::vector<std::unique_ptr<WavetableData>> factoryTables`
  - Pass table pointer to each voice via `setWavetable(factoryTables[tableIndex])`
  - Read `oscATable` param to select which table to use (0=Saw, 1=Square, 2=Triangle, 3=Sine)
  - Build and test: `ninja O-Prism_VST3 O-Prism_AU`

**Phase 2.1 Success Criteria:**
- [ ] Playing MIDI notes produces wavetable sound
- [ ] Different oscATable values select different waveforms
- [ ] oscAPos morphs between frames (single-frame tables won't morph, but code path verified)
- [ ] Amplitude ADSR shapes notes correctly (attack ramp, sustain hold, release tail)
- [ ] Polyphony works (play chords)
- [ ] Voice stealing works (exceed 16 voices)
- [ ] No clicks on note-on or note-off (ADSR tail-off)
- [ ] Tuning accurate (A4 = 440 Hz, other tunings via TuningEngine)

---

## Phase 2.2: Mipmap Anti-Aliasing + Osc B + Mixing (5 tasks)

**Phase Goal:** Bandlimited wavetable playback with FFT mipmaps, both oscillators active, proper mixing.

### Task 7: Implement FFT-based mipmap generation
- **Files:** `Source/dsp/WavetableGenerator.cpp` (MODIFY), `Source/dsp/WavetableData.h` (MODIFY)
- **Depends on:** Task 6 (Phase 2.1 complete)
- **Details:**
  - Use `juce::dsp::FFT(11)` — order 11 = 2048 size
  - For each frame in a table:
    - Copy 2048 samples into fftBuffer (4096 floats — 2 * getSize())
    - `fft.performRealOnlyForwardTransform(buf, false)` — `false` for full spectrum
    - For each mipmap level 0-9:
      - maxHarmonic = (2048/2) >> level → 1024, 512, 256, ...
      - Zero bins above maxHarmonic (both positive + negative freq)
      - Zero DC bin (prevent DC offset)
      - `fft.performRealOnlyInverseTransform(workBuf)` — JUCE divides by N internally
      - Store result + set guard sample
  - Call during `generateProceduralTable()` — generate all 10 levels for each table
  - WavetableData must now allocate `10 * numFrames * 2049` floats

### Task 8: Implement mipmap level selection in WavetableOscillator
- **Files:** `Source/dsp/WavetableOscillator.cpp` (MODIFY)
- **Depends on:** Task 7
- **Details:**
  - Calculate mipmap level from frequency:
    - `baseFreq = sampleRate / 2048.0` (~21.5 Hz at 44.1k)
    - `levelFloat = log2(frequency / baseFreq)`, clamped to [0, 9]
    - Interpolate between two adjacent levels for smooth transitions
  - Upgrade `getNextSample()` to trilinear interpolation (8 lookups):
    - Sample interp within frame (2 lookups)
    - Frame interp between frame0/frame1 (x2 = 4 lookups)
    - Mipmap interp between level0/level1 (x2 = 8 lookups total)
  - Verify: high notes (>C6) should sound clean without aliasing

### Task 9: Activate Osc B in PrismVoice
- **Files:** `Source/PrismVoice.h` (MODIFY), `Source/PrismVoice.cpp` (MODIFY)
- **Depends on:** Task 8
- **Details:**
  - Add `WavetableOscillator oscB` member
  - In `prepare()`: `oscB.prepare(sampleRate)`
  - In `startNote()`: calculate oscB frequency (same base + coarse/fine for B)
  - In `renderNextBlock()`: read `oscBPos`, `oscBLevel`, `oscBPan`, `oscBCoarse`, `oscBFine`
  - Generate oscB samples in same per-sample loop
  - Separate wavetable for B: read `oscBTable` param

### Task 10: Implement oscillator mixing with level/pan
- **Files:** `Source/PrismVoice.cpp` (MODIFY)
- **Depends on:** Task 9
- **Details:**
  - Read `oscMix`, `oscALevel`, `oscAPan`, `oscBLevel`, `oscBPan` from APVTS
  - Per-sample mixing: `mixed = oscA * (1.0 - oscMix) + oscB * oscMix`
  - Apply per-oscillator level: `oscA *= oscALevel`
  - Apply equal-power pan law per oscillator:
    - `panNorm = (pan + 1) * 0.5`
    - `leftGain = cos(panNorm * halfPi)`, `rightGain = sin(panNorm * halfPi)`
  - Write stereo output to buffer

### Task 11: Thread-safe wavetable delivery from processor
- **Files:** `Source/PluginProcessor.h` (MODIFY), `Source/PluginProcessor.cpp` (MODIFY)
- **Depends on:** Task 9
- **Details:**
  - Store active table pointers: `std::atomic<const WavetableData*> activeTableA, activeTableB`
  - In `processBlock()`: check if `oscATable`/`oscBTable` params changed, swap pointers
  - Voices read tables via atomic load (lock-free)
  - Add method: `const WavetableData* getActiveTable(int oscIndex)` for voice access

**Phase 2.2 Success Criteria:**
- [ ] No audible aliasing on high notes (C6 and above)
- [ ] Both oscillators producing sound simultaneously
- [ ] oscMix sweeps smoothly between A and B
- [ ] Level and pan controls work per oscillator
- [ ] Stereo output from pan controls
- [ ] CPU reasonable (<5% per voice with both osc)

---

## Phase 2.3: Unison + Sub + Noise + Glide (5 tasks)

**Phase Goal:** Full oscillator section — thick unison, sub bass, noise textures, smooth glide.

### Task 12: Implement unison engine in WavetableOscillator
- **Files:** `Source/dsp/WavetableOscillator.h` (MODIFY), `Source/dsp/WavetableOscillator.cpp` (MODIFY)
- **Depends on:** Task 10 (Phase 2.2 complete)
- **Details:**
  - Add internal array of up to 8 phase accumulators (one per unison voice)
  - `setUnison(int count, float detune, float width)`:
    - For N voices: `centerIndex = (N-1) / 2.0`
    - Per-voice detune: `pow(2.0, normalizedPos * detuneAmount * 50.0 / 1200.0)`
    - Per-voice pan: `normalizedPos * widthAmount` (equal-power pan law)
    - Per-voice gain: `1.0 / sqrt(N)`
    - Random phase offset per voice (set at note-on via `resetWithRandomPhase()`)
  - `getNextSampleStereo(double& outL, double& outR)` — returns summed unison output in stereo
  - Each unison voice = separate phase accumulator, same WavetableData pointer

### Task 13: Create SubOscillator class (polyBLEP)
- **Files:** `Source/dsp/SubOscillator.h`, `Source/dsp/SubOscillator.cpp` (NEW)
- **Depends on:** Task 12
- **Details:**
  - polyBLEP correction function (static inline)
  - Shapes: Sine, Triangle, Saw, Square
  - `setFrequency(double freq)` — applies octave offset internally
  - `setShape(int shape)`, `setOctave(int octave)` (-2, -1, 0)
  - `prepare(double sampleRate)`, `reset()`
  - `getNextSample() -> double`
  - Triangle via leaky-integrated polyBLEP square
  - Routes direct to output (bypasses filters) — handled in PrismVoice routing

### Task 14: Create NoiseGenerator class (6 types)
- **Files:** `Source/dsp/NoiseGenerator.h`, `Source/dsp/NoiseGenerator.cpp` (NEW)
- **Depends on:** None (can be done in parallel with Task 12-13)
- **Details:**
  - `juce::Random` instance (per-voice to avoid contention)
  - White: `random.nextDouble() * 2.0 - 1.0`
  - Pink: Paul Kellet economy filter (3 state vars: `b0`, `b1`, `b2`)
  - Brown: leaky integrator (`state += white * 0.02; state *= 0.998`), scale step by `44100/sr`
  - Digital: sample-and-hold at ~5.5kHz, quantize to 8 levels
  - Vinyl: bandpass-filtered white + Poisson-distributed crackle impulses
  - Wind: LFO (0.2Hz) modulated lowpass on brown noise
  - `setType(int type)`, `prepare(double sampleRate)`, `reset()`
  - `getNextSample() -> double`

### Task 15: Create GlideProcessor class
- **Files:** `Source/dsp/GlideProcessor.h` (NEW — header-only)
- **Depends on:** None
- **Details:**
  - Exponential frequency interpolation: one-pole smoother
  - `glideCoeff = exp(-1.0 / (glideTime * sampleRate))`
  - Per-sample: `currentFreq = currentFreq * glideCoeff + targetFreq * (1.0 - glideCoeff)`
  - Stop gliding when within 0.01 cents of target
  - Modes: Off (instant), Legato (glide only if previously playing), Always
  - `setTarget(double freq)`, `setMode(int mode)`, `setTime(double seconds)`
  - `getNextFrequency() -> double`
  - Legato detection: check if voice was already active before startNote

### Task 16: Integrate sub, noise, glide into PrismVoice + update CMakeLists
- **Files:** `Source/PrismVoice.h` (MODIFY), `Source/PrismVoice.cpp` (MODIFY), `CMakeLists.txt` (MODIFY)
- **Depends on:** Tasks 12, 13, 14, 15
- **Details:**
  - Add `SubOscillator subOsc`, `NoiseGenerator noiseGen`, `GlideProcessor glide` members
  - In `startNote()`:
    - Configure glide target frequency (check legato via `getCurrentlyPlayingNote() >= 0`)
    - Set sub oscillator frequency: `voiceFreq * pow(2.0, subOctave)`
    - Reset noise generator
  - In `renderNextBlock()`:
    - Read unison/sub/noise/glide params from APVTS each block
    - Per-sample: get glide-smoothed frequency, set osc frequencies
    - Get sub sample, apply amp envelope, add to output (bypass filters)
    - Get noise sample, mix into oscillator signal (routed to filter input)
    - Get unison stereo output for oscA and oscB
  - Update CMakeLists.txt with new source files
  - Build and test

**Phase 2.3 Success Criteria:**
- [ ] Unison 8 voices produces thick, wide stereo sound
- [ ] Detune spread is symmetric and sounds correct
- [ ] Sub oscillator sounds at correct octave below fundamental
- [ ] All 4 sub shapes (sine/tri/saw/square) are clean
- [ ] All 6 noise types sound distinct
- [ ] Glide creates smooth pitch transitions
- [ ] Legato mode only glides when previous note held
- [ ] CPU within budget at 16 voices x 8 unison

---

## Phase 2.4: Dual Filters + Filter Envelope (4 tasks)

**Phase Goal:** Dual SVF filters with serial/parallel routing, filter envelope modulation.

### Task 17: Create SVFFilter wrapper class
- **Files:** `Source/dsp/SVFFilter.h`, `Source/dsp/SVFFilter.cpp` (NEW)
- **Depends on:** Task 16 (Phase 2.3 complete)
- **Details:**
  - Wraps `juce::dsp::StateVariableTPTFilter<double>`
  - Supports all 7 types: LP12, LP24, HP12, HP24, BP12, BP24, Notch
  - 24dB modes (LP24/HP24/BP24): two cascaded SVF instances, resonance on first only
  - Notch: custom 6-line SVF computation returning `yLP + yHP` from single computation
  - Resonance mapping (CRITICAL): `svfResonance = 1.0 / (1.0 + param * 19.0)` — inverse Q
  - Pre-filter drive: `tanh(input * (1.0 + drive * 9.0))`
  - Key tracking: `cutoff *= pow(2.0, keyTrackAmount * (midiNote - 60) / 12.0)`
  - `prepare(double sampleRate)`, `reset()`
  - `setType(int type)`, `setCutoff(double hz)`, `setResonance(double res)`, `setDrive(double drive)`
  - `processSample(double input) -> double` — per-sample for envelope modulation

### Task 18: Implement filter envelope in PrismVoice
- **Files:** `Source/PrismVoice.h` (MODIFY), `Source/PrismVoice.cpp` (MODIFY)
- **Depends on:** Task 17
- **Details:**
  - Add `juce::ADSR filterEnvelope` member
  - In `prepare()`: `filterEnvelope.setSampleRate(sampleRate)` BEFORE setParameters
  - In `startNote()`: read filter ADSR params, set parameters, noteOn
  - In `stopNote()`: `filterEnvelope.noteOff()`
  - Per-sample cutoff modulation:
    - `filtEnvVal = static_cast<double>(filterEnvelope.getNextSample())`
    - `modulatedCutoff = baseCutoff * pow(2.0, filtEnvVal * envDepth * 4.0)` (±4 octaves)
    - Clamp to [20, 20000] Hz
    - Apply key tracking offset
    - Call `filter.setCutoff(modulatedCutoff)` per sample (OK for TPT design)

### Task 19: Implement dual filter routing in PrismVoice
- **Files:** `Source/PrismVoice.cpp` (MODIFY)
- **Depends on:** Task 18
- **Details:**
  - Add `SVFFilter filterA, filterB` members
  - Read `filtAType/BCutoff/BRes/BDrive/BKeyTrack`, `filtRouting` from APVTS
  - In `startNote()`: `filterA.reset()`, `filterB.reset()` (clear state for new note)
  - In `renderNextBlock()`, per-sample:
    - Calculate modulated cutoff for A and B (each with shared filter envelope)
    - **Serial mode:** `input -> driveA -> filterA -> driveB -> filterB -> output`
    - **Parallel mode:** `input -> (driveA -> filterA) + (driveB -> filterB) -> output`
  - Sub oscillator signal bypasses both filters (already summed after filter output)

### Task 20: Update CMakeLists and build test
- **Files:** `CMakeLists.txt` (MODIFY)
- **Depends on:** Task 19
- **Details:**
  - Add `Source/dsp/SVFFilter.cpp` to target_sources
  - Build: `ninja O-Prism_VST3 O-Prism_AU`
  - Test: all filter types, resonance, drive, key tracking, envelope, routing

**Phase 2.4 Success Criteria:**
- [ ] All 7 filter types produce expected frequency response
- [ ] LP24 has steeper rolloff than LP12
- [ ] Resonance self-oscillates at maximum for LP/HP
- [ ] Drive adds harmonics
- [ ] Key tracking shifts cutoff with pitch
- [ ] Filter envelope sweeps cutoff smoothly (no clicks)
- [ ] Serial routing: A->B compounds filtering
- [ ] Parallel routing: A+B sum sounds different
- [ ] No filter instability at extreme settings

---

## Phase 2.5: Effects Chain + Master (8 tasks)

**Phase Goal:** Global effects processing chain — the final DSP stage. All effects in float precision.

### Task 21: Create DistortionProcessor class
- **Files:** `Source/dsp/DistortionProcessor.h`, `Source/dsp/DistortionProcessor.cpp` (NEW)
- **Depends on:** Task 20 (Phase 2.4 complete)
- **Details:**
  - 4 algorithms: SoftClip `tanh(g*x)`, HardClip `clamp(g*x,-1,1)`, Tube (asymmetric), Fold `sin(g*x*PI)`
  - Drive mapping: `gain = 1.0 + drive01 * 9.0` (range 1-10x)
  - 2x oversampling: `juce::dsp::Oversampling<float>` with `numChannels=2, factor=1` (factor is EXPONENT — 1 = 2x)
  - Use `filterHalfBandPolyphaseIIR` for low latency
  - Dry/wet: `juce::dsp::DryWetMixer<float>` with `setWetLatency(oversampling.getLatencyInSamples())`
  - `prepare(dsp::ProcessSpec)`, `process(dsp::AudioBlock<float>)`, `reset()`
  - `setType(int)`, `setDrive(float)`, `setMix(float)`

### Task 22: Create DelayProcessor class
- **Files:** `Source/dsp/DelayProcessor.h`, `Source/dsp/DelayProcessor.cpp` (NEW)
- **Depends on:** None (parallel with Task 21)
- **Details:**
  - `juce::dsp::DelayLine<float, Lagrange3rd>` for L and R (max 192000 samples)
  - Feedback with LP filter: `StateVariableTPTFilter<float>` at 8kHz
  - Normal mode: straight L/R feedback
  - PingPong: cross-feedback (L reads R's delay, R reads L's delay)
  - `DryWetMixer<float>` for dry/wet
  - Tempo sync: read BPM from `getPlayHead()->getPosition()->getBpm()`
  - `prepare(dsp::ProcessSpec)`, `process(dsp::AudioBlock<float>)`, `reset()`
  - `setTime(float seconds)`, `setFeedback(float)`, `setMode(int)`, `setSync(bool)`, `setMix(float)`
  - `setPlayHead(juce::AudioPlayHead*)` for tempo sync

### Task 23: Create ReverbProcessor class
- **Files:** `Source/dsp/ReverbProcessor.h`, `Source/dsp/ReverbProcessor.cpp` (NEW)
- **Depends on:** None (parallel with Task 21)
- **Details:**
  - `juce::dsp::Reverb` (float-only, Freeverb-based)
  - Pre-delay: `juce::dsp::DelayLine<float, Linear>` for L and R (max 19200 samples = ~200ms at 96k)
  - **CRITICAL:** Set reverb internal wetLevel=1.0, dryLevel=0.0; use external `DryWetMixer<float>` for blend
  - `prepare(dsp::ProcessSpec)`, `process(dsp::AudioBlock<float>)`, `reset()`
  - `setSize(float)`, `setDamping(float)`, `setPredelay(float ms)`, `setMix(float)`

### Task 24: Create EQProcessor class
- **Files:** `Source/dsp/EQProcessor.h`, `Source/dsp/EQProcessor.cpp` (NEW)
- **Depends on:** None (parallel with Task 21)
- **Details:**
  - 3 bands using `juce::dsp::ProcessorDuplicator<IIR::Filter<float>, IIR::Coefficients<float>>`
  - Low shelf at 200Hz, Mid peak at variable freq, High shelf at 8000Hz
  - Coefficient updates: dereference and copy in-place `*filter.state = *Coefficients::make...()`
  - **CRITICAL:** `gainFactor` is LINEAR, not dB — use `Decibels::decibelsToGain()`
  - Q for shelves: 0.707 (Butterworth), Q for mid peak: 1.0
  - `prepare(dsp::ProcessSpec)`, `process(dsp::AudioBlock<float>)`, `reset()`
  - `setLowGain(float dB)`, `setMidGain(float dB)`, `setMidFreq(float Hz)`, `setHighGain(float dB)`

### Task 25: Integrate effects chain in PluginProcessor
- **Files:** `Source/PluginProcessor.h` (MODIFY), `Source/PluginProcessor.cpp` (MODIFY)
- **Depends on:** Tasks 21, 22, 23, 24
- **Details:**
  - Add members: `DistortionProcessor`, `juce::dsp::Chorus<float>`, `DelayProcessor`, `EQProcessor`, `ReverbProcessor`
  - Add `juce::SmoothedValue<float> masterVolSmoothed` (20ms ramp)
  - In `prepareToPlay()`:
    - Create `juce::dsp::ProcessSpec` with `{sampleRate, (uint32)samplesPerBlock, 2}`
    - Prepare all effects + chorus
    - Chorus config: `setCentreDelay(7.0f)`, `setFeedback(0.0f)`
    - `masterVolSmoothed.reset(sampleRate, 0.02)`
  - In `processBlock()`:
    - After `synthesiser.renderNextBlock()` (voice sum is now in float buffer)
    - Read all effect params from APVTS
    - Create `dsp::AudioBlock<float>` from buffer
    - Chain order: **Distortion → Chorus → Delay → EQ → Reverb**
    - Apply smoothed master volume per-sample (prevents zipper noise)
  - Pass `getPlayHead()` to DelayProcessor for tempo sync

### Task 26: Implement Chorus wrapper in processBlock
- **Files:** `Source/PluginProcessor.cpp` (MODIFY)
- **Depends on:** Task 25
- **Details:**
  - `juce::dsp::Chorus<float>` handles dry/wet internally — NO external DryWetMixer
  - Per-block: `chorus.setRate(rate)`, `chorus.setDepth(depth)`, `chorus.setMix(mix)`
  - `chorus.process(ProcessContextReplacing<float>(block))`

### Task 27: Update CMakeLists and build all effects
- **Files:** `CMakeLists.txt` (MODIFY)
- **Depends on:** Task 26
- **Details:**
  - Add all new dsp/*.cpp files to target_sources
  - Build: `ninja O-Prism_VST3 O-Prism_AU`
  - Full integration test with all effects active

### Task 28: Final Stage 2 integration test and cleanup
- **Files:** Various (minor fixes)
- **Depends on:** Task 27
- **Details:**
  - Verify complete signal chain: MIDI → TuningEngine → Osc A/B (unison) → Mix → Filters → Amp Env → Sub → Effects → Master → Output
  - Test all parameter combinations
  - Check for denormals (`ScopedNoDenormals` in processBlock)
  - Verify voice stealing at max polyphony
  - CPU benchmark: 16 voices, max unison, both osc, filters active
  - Fix any clicks, pops, or artifacts discovered during testing
  - Clean up any debug code

**Phase 2.5 Success Criteria:**
- [ ] All 4 distortion types sound distinct and musical
- [ ] Distortion has no aliasing (2x oversampling verified)
- [ ] Chorus creates movement and width
- [ ] Delay time accurate, ping-pong alternates channels
- [ ] Delay feedback capped, no runaway
- [ ] Reverb creates convincing space
- [ ] Pre-delay separates dry attack from reverb onset
- [ ] EQ bands boost/cut at correct frequencies
- [ ] Master volume scales smoothly (no zipper noise)
- [ ] All effects at mix=0.0 pass signal unchanged
- [ ] Complete signal chain works end-to-end

---

## Files Summary

### New Files (14)
| File | Phase | Purpose |
|------|-------|---------|
| `Source/dsp/WavetableData.h` | 2.1 | Wavetable data structure with mipmap storage |
| `Source/dsp/WavetableGenerator.h` | 2.1 | Procedural wavetable generation |
| `Source/dsp/WavetableGenerator.cpp` | 2.1 | Procedural wavetable generation + FFT mipmap |
| `Source/dsp/WavetableOscillator.h` | 2.1 | Wavetable playback engine |
| `Source/dsp/WavetableOscillator.cpp` | 2.1 | Phase accumulator, interpolation, unison |
| `Source/dsp/SubOscillator.h` | 2.3 | polyBLEP sub oscillator |
| `Source/dsp/SubOscillator.cpp` | 2.3 | polyBLEP sub oscillator |
| `Source/dsp/NoiseGenerator.h` | 2.3 | 6 noise type generator |
| `Source/dsp/NoiseGenerator.cpp` | 2.3 | 6 noise type generator |
| `Source/dsp/GlideProcessor.h` | 2.3 | Portamento/glide (header-only) |
| `Source/dsp/SVFFilter.h` | 2.4 | Multi-mode filter wrapper |
| `Source/dsp/SVFFilter.cpp` | 2.4 | Multi-mode filter wrapper |
| `Source/dsp/DistortionProcessor.h/.cpp` | 2.5 | 4-type distortion with 2x oversampling |
| `Source/dsp/DelayProcessor.h/.cpp` | 2.5 | Delay with feedback, ping-pong, sync |
| `Source/dsp/ReverbProcessor.h/.cpp` | 2.5 | Reverb with pre-delay |
| `Source/dsp/EQProcessor.h/.cpp` | 2.5 | 3-band parametric EQ |

### Modified Files (4)
| File | Phases | Changes |
|------|--------|---------|
| `Source/PrismVoice.h` | 2.1-2.4 | Add osc, filter, envelope, sub, noise, glide members |
| `Source/PrismVoice.cpp` | 2.1-2.4 | Implement full per-voice DSP rendering |
| `Source/PluginProcessor.h` | 2.1, 2.2, 2.5 | Add wavetable storage, effects chain members |
| `Source/PluginProcessor.cpp` | 2.1, 2.2, 2.5 | Table generation, effects processing, master volume |
| `CMakeLists.txt` | 2.1, 2.3, 2.4, 2.5 | Add new source files |

---

## Critical Gotchas (from RESEARCH.md)

| # | Gotcha | Where | Mitigation |
|---|--------|-------|------------|
| 1 | Reverb is float-only | Phase 2.5 | Process entire effects chain in float |
| 2 | SVF has NO notch type | Phase 2.4 | Custom LP+HP sum from single SVF computation |
| 3 | SVF resonance is inverse-Q | Phase 2.4 | Map: `1.0 / (1.0 + param * 19.0)`, never pass 0 |
| 4 | ADSR returns float | Phase 2.1 | Cast to double for voice processing |
| 5 | ADSR: setSampleRate before setParameters | Phase 2.1 | Always call in prepare() first |
| 6 | Oversampling factor is exponent | Phase 2.5 | Pass `1` for 2x (not `2`) |
| 7 | FFT buffer must be 2*getSize() | Phase 2.2 | Allocate 4096 floats |
| 8 | FFT needs false for IFFT compat | Phase 2.2 | `performRealOnlyForwardTransform(buf, false)` |
| 9 | IIR gainFactor is linear not dB | Phase 2.5 | Use `Decibels::decibelsToGain()` |
| 10 | Brown noise step rate-dependent | Phase 2.3 | Scale by `44100/sampleRate` |
| 11 | DryWetMixer needs latency for OS | Phase 2.5 | `setWetLatency()` after `initProcessing()` |

---

## Dependency Graph

```
Phase 2.1: [T1] -> [T2] -> [T6]
           [T1] -> [T3] -> [T5] -> [T6]
           [T4] ---------> [T5]

Phase 2.2: [T7] -> [T8] -> [T9] -> [T10]
                             [T9] -> [T11]

Phase 2.3: [T12] -> [T16]
           [T13] -> [T16]
           [T14] -> [T16]
           [T15] -> [T16]

Phase 2.4: [T17] -> [T18] -> [T19] -> [T20]

Phase 2.5: [T21] -> [T25] -> [T26] -> [T27] -> [T28]
           [T22] -> [T25]
           [T23] -> [T25]
           [T24] -> [T25]
```

Each phase is sequential (2.1 must complete before 2.2, etc.). Within each phase, tasks with no dependency arrows can be done in parallel.
