# Stage 2 DSP -- Phase 3.5 Plan: Oversampling + MPE + Tuning + Bow Noise + Optimization

**Plugin:** O-Bowed
**Phase:** 3.5
**Input:** CONTEXT-3.5.md, RESEARCH-3.5.md
**Date:** 2026-04-05

---

## Goal

Add 2x per-voice oversampling for the friction/waveguide inner loop, migrate from `juce::Synthesiser` to `juce::MPESynthesiser` for full MPE support, wire TuningEngine to voice pitch (Scala/TUN + MTS-ESP + reference pitch), create a bow noise generator, connect the final 3 parameters (referencePitch, tuningSystem, bowNoise), and run an optimization pass. After this phase, all 23 parameters are connected and all must-priority DSP requirements are satisfied.

---

## Tasks

### Task 1: Create BowNoiseGenerator.h

- **Create:** `Source/DSP/BowNoiseGenerator.h`
- **Depends on:** none
- **Details:**
  - Header-only class (~50 lines)
  - Per-voice bandpass-filtered noise generator
  - Random source: `juce::Random` instance seeded by voice index (`voiceIndex * 31337`)
  - Bandpass filter: `juce::dsp::IIR::Filter<float>` at 3464 Hz (geometric mean 2-6 kHz), Q = 0.87
  - `prepare(double sampleRate, int voiceIndex)`: seed Random, compute bandpass coefficients via `makeBandPass(sampleRate, 3464.0f, 0.87f)`
  - `processSample(float bowPressure, float bowSpeed, float noiseAmount) -> float`:
    - If noiseAmount < 0.001f, return 0.0f (early out)
    - Generate white noise: `noiseRandom.nextFloat() * 2.0f - 1.0f`
    - Filter through bandpass
    - Amplitude: `bowPressure * bowSpeed * noiseAmount * 0.03f`
    - Return filtered noise * amplitude
  - `reset()`: reset IIR filter state
  - NOT in waveguide feedback loop -- caller adds output post-body

### Task 2: Create BowedMPESynthesiser.h

- **Create:** `Source/BowedMPESynthesiser.h`
- **Depends on:** none
- **Details:**
  - Header-only class (~40 lines)
  - `class BowedMPESynthesiser : public juce::MPESynthesiser`
  - Override `handleController(int channel, int controller, int value)`:
    - If `controller == 11` (Expression): iterate voices on matching channel, set expression value
    - Base class `handleController` is empty (CC64/CC66 handled by MPEInstrument internally)
  - Add `void setExpressionForChannel(int channel, float normalizedValue)` helper that iterates voices

### Task 3: Add bowNoise parameter to APVTS

- **Modify:** `Source/PluginProcessor.cpp` (createParameterLayout only)
- **Depends on:** none
- **Details:**
  - Add `bowNoise` parameter in "Bow Controls" section (after rosin):
    ```cpp
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "bowNoise", 1 },
        "Bow Noise",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.0f
    ));
    ```
  - Default 0.0 (off) -- subtle effect, user opts in
  - Total parameters after: 24 (was 23 -- bowNoise is new, not in original spec count)

### Task 4: Migrate BowedStringVoice to MPESynthesiserVoice

- **Modify:** `Source/BowedStringVoice.h`, `Source/BowedStringVoice.cpp`
- **Depends on:** Tasks 1, 2
- **Details:**

  **Header changes (BowedStringVoice.h):**
  - Change base class: `juce::SynthesiserVoice` -> `juce::MPESynthesiserVoice`
  - Remove: `canPlaySound()`, `startNote()`, `stopNote()`, `pitchWheelMoved()`, `controllerMoved()`
  - Add MPE overrides: `noteStarted()`, `noteStopped(bool)`, `notePitchbendChanged()`, `notePressureChanged()`, `noteTimbreChanged()`
  - Add includes: `BowNoiseGenerator.h`, `<juce_dsp/juce_dsp.h>` (for Oversampling)
  - Forward-declare `TuningEngine` class (or include TuningEngine.h)
  - Add members:
    - `TuningEngine* tuningEngine = nullptr` + `void setTuningEngine(TuningEngine*)`
    - `juce::dsp::Oversampling<float> oversampling { 1, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR }` (mono, 2x, IIR)
    - `juce::AudioBuffer<float> voiceBuffer` (mono, sized in prepareToPlay)
    - `BowNoiseGenerator bowNoiseGen`
    - `float mpeExpression = 1.0f` (CC11, set from BowedMPESynthesiser)
    - `float bowNoiseAmount = 0.0f`
    - `int voiceIndex = 0` + `void setVoiceIndex(int idx)`
  - Remove `BowedStringSound.h` include

  **Implementation changes (BowedStringVoice.cpp):**
  - Remove `BowedStringSound.h` include and `canPlaySound()` method

  - `prepareToPlay(double sampleRate, int maxBlockSize)`:
    - Call `waveguideString.prepare(sampleRate * 2.0, maxBlockSize * 2)` -- **internal 2x rate**
    - Call `bowModel.prepare(sampleRate * 2.0)` -- envelope runs at 2x
    - Call `oversampling.initProcessing((size_t)maxBlockSize)`
    - Resize `voiceBuffer` to 1 channel, maxBlockSize samples
    - Call `bowNoiseGen.prepare(sampleRate, voiceIndex)`
    - Cache `dt = 1.0f / (float)(sampleRate * 2.0)` -- dt at internal rate
    - Call `qualityFriction.prepare(sampleRate * 2.0)` -- LUT at 2x rate

  - `noteStarted()`:
    - Get MIDI note: `getCurrentlyPlayingNote().initialNote`
    - Get velocity: `getCurrentlyPlayingNote().noteOnVelocity.asUnsignedFloat()`
    - Get frequency from tuning engine: `tuningEngine ? (float)tuningEngine->getFrequency(midiNote) : juce::MidiMessage::getMidiNoteInHertz(midiNote)`
    - Store `currentFrequency`
    - Apply initial pitch bend: `currentFrequency *= std::pow(2.0f, getCurrentlyPlayingNote().totalPitchbendInSemitones / 12.0f)`
    - Call `waveguideString.trigger(currentFrequency)` + `bowModel.startBow(velocity)`
    - Reset oversampling: `oversampling.reset()`

  - `noteStopped(bool allowTailOff)`:
    - Same logic as current `stopNote()`, adapted for MPE API
    - Use `clearCurrentNote()` for hard stop

  - `notePitchbendChanged()`:
    - Get base frequency from tuning engine (using `getCurrentlyPlayingNote().initialNote`)
    - Apply bend: `baseFreq * pow(2, getCurrentlyPlayingNote().totalPitchbendInSemitones / 12.0f)`
    - Update `currentFrequency`, call `waveguideString.trigger(currentFrequency)` for real-time pitch update

  - `notePressureChanged()`:
    - Store `getCurrentlyPlayingNote().pressure.asUnsignedFloat()` for use in updateParametersFromAPVTS
    - MPE pressure -> bow pressure multiply: `0.5f + pressure * 1.5f`

  - `noteTimbreChanged()`:
    - Store `getCurrentlyPlayingNote().timbre.asSignedFloat()` for use in updateParametersFromAPVTS
    - MPE timbre (CC74) -> bow position offset

  - `renderNextBlock(outputBuffer, startSample, numSamples)` **RESTRUCTURED for block-based oversampling:**
    1. `updateParametersFromAPVTS()` (apply MPE modulation on top of knob base)
    2. Check voice active (same as current)
    3. Prepare mono input block of silence (numSamples):
       ```cpp
       voiceBuffer.clear(0, 0, numSamples);
       juce::dsp::AudioBlock<float> inputBlock(voiceBuffer);
       auto block = inputBlock.getSubBlock(0, (size_t)numSamples);
       ```
    4. Upsample: `auto oversampledBlock = oversampling.processSamplesUp(block);`
    5. Per-sample loop at 2x rate (`oversampledBlock.getNumSamples()`):
       - Same friction/waveguide logic as current (bowModel.updateEnvelope, readJunction, tier dispatch, reversed friction, writeJunction, subHarmonics)
       - Write sample into oversampledBlock
    6. Downsample: `oversampling.processSamplesDown(block);`
    7. Mix voiceBuffer into outputBuffer with panning + bow noise:
       ```cpp
       for (int i = 0; i < numSamples; ++i) {
           float sample = voiceBuffer.getSample(0, i) * outputGainLinear;
           float noise = bowNoiseGen.processSample(bowPressure, bowSpeed, bowNoiseAmount);
           sample += noise;
           sample = juce::jlimit(-2.0f, 2.0f, sample);
           outputBuffer.addSample(0, startSample + i, sample * panL);
           outputBuffer.addSample(1, startSample + i, sample * panR);
       }
       ```
       Note: bow noise is at native rate (post-downsample), NOT oversampled

  - `updateParametersFromAPVTS()`:
    - Read existing params (same as current)
    - Read `bowNoise` from APVTS
    - Apply MPE modulation on top of knob base values:
      - `effectivePressure = bowPressure * (0.5f + getCurrentlyPlayingNote().pressure.asUnsignedFloat() * 1.5f)`
      - `effectivePosition = bowPos + getCurrentlyPlayingNote().timbre.asSignedFloat() * 0.1f` (±0.1 range offset)
      - `effectiveSpeed = bowSpeed * mpeExpression`
    - Pass effective values to DSP components instead of raw knob values

### Task 5: Migrate PluginProcessor to BowedMPESynthesiser

- **Modify:** `Source/PluginProcessor.h`, `Source/PluginProcessor.cpp`
- **Depends on:** Tasks 2, 3, 4
- **Details:**

  **Header changes (PluginProcessor.h):**
  - Replace `#include "BowedStringSound.h"` with `#include "BowedMPESynthesiser.h"`
  - Replace `juce::Synthesiser synthesiser` with `BowedMPESynthesiser synthesiser`

  **Implementation changes (PluginProcessor.cpp):**
  - Constructor:
    - Remove `synthesiser.addSound(new BowedStringSound())` (MPE has no Sound concept)
    - Voice creation loop: set voice index and tuning engine pointer:
      ```cpp
      for (int i = 0; i < 8; ++i) {
          auto* voice = new BowedStringVoice(&parameters);
          voice->setVoiceIndex(i);
          voice->setTuningEngine(&tuningEngine);
          synthesiser.addVoice(voice);
      }
      ```
    - Enable legacy mode for non-MPE controllers: `synthesiser.enableLegacyMode(2)` (±2 semitone default bend)

  - `prepareToPlay()`:
    - After voice preparation, report oversampling latency:
      ```cpp
      // All voices share identical oversampling config -- query the first
      if (auto* voice = dynamic_cast<BowedStringVoice*>(synthesiser.getVoice(0)))
          setLatencySamples(static_cast<int>(std::ceil(voice->getOversamplingLatency())));
      ```
    - Note: add `float getOversamplingLatency() const` to BowedStringVoice (returns `oversampling.getLatencyInSamples()`)

  - `processBlock()`:
    - Wire tuning engine params (add after existing param reads):
      ```cpp
      tuningEngine.setMasterTune(static_cast<double>(refPitch));
      int tuningSystemIdx = static_cast<int>(parameters.getRawParameterValue("tuningSystem")->load());
      // Map: 0=Scala/TUN, 1=MTS-ESP, 2=12-TET
      switch (tuningSystemIdx) {
          case 0: tuningEngine.setMode(TuningEngine::Mode::Scala); break;
          case 1: tuningEngine.setMode(TuningEngine::Mode::MTSESP); break;
          default: tuningEngine.setMode(TuningEngine::Mode::TwelveTET); break;
      }
      ```
    - Remove `canPlaySound` references/checks if any (MPE voices don't use Sound)
    - Voice dynamic_cast changes: `BowedStringVoice*` is still valid (it's just MPESynthesiserVoice now)

  - Remove `#include "BowedStringSound.h"` from PluginProcessor.cpp

### Task 6: Optimization + Cleanup + CMakeLists

- **Modify:** `Source/DSP/SympatheticStringEngine.h` or `.cpp`, `CMakeLists.txt`
- **Depends on:** Task 5
- **Details:**

  **Sympathetic energy threshold:**
  - Raise gating threshold from `1e-7f` to `1e-5f` (-100 dBFS, still 40 dB below 16-bit noise floor)
  - Strings settle to gated state ~30% faster, meaningful CPU savings with 12 strings

  **MPE normalization constants:**
  - Add `constexpr float inv127 = 1.0f / 127.0f` and `constexpr float inv8192 = 1.0f / 8192.0f` in BowedMPESynthesiser.h for CC normalization (avoids per-sample division)

  **Remove BowedStringSound.h:**
  - Delete `Source/BowedStringSound.h` (no longer used after MPE migration)
  - Remove from CMakeLists.txt target_sources

  **CMakeLists.txt update:**
  - Remove: `Source/BowedStringSound.h`
  - Add: `Source/DSP/BowNoiseGenerator.h`, `Source/BowedMPESynthesiser.h`

  **ScopedNoDenormals verification:**
  - Confirm `ScopedNoDenormals` in processBlock covers oversampled path (it does -- same thread, MXCSR register is per-thread RAII)

---

## Execution Waves

**Wave 1** (independent -- can execute in parallel):
- Task 1: BowNoiseGenerator.h
- Task 2: BowedMPESynthesiser.h
- Task 3: bowNoise parameter in createParameterLayout

**Wave 2** (depends on Wave 1):
- Task 4: BowedStringVoice migration (MPE + oversampling + tuning + noise)

**Wave 3** (depends on Wave 2):
- Task 5: PluginProcessor migration

**Wave 4** (depends on Wave 3):
- Task 6: Optimization + cleanup + CMakeLists + build

---

## Files Summary

### Create (2)
| File | Lines | Description |
|------|-------|-------------|
| `Source/DSP/BowNoiseGenerator.h` | ~50 | Per-voice bandpass noise, 3464 Hz, Q=0.87 |
| `Source/BowedMPESynthesiser.h` | ~40 | MPESynthesiser subclass with CC11 handling |

### Modify (6)
| File | Changes |
|------|---------|
| `Source/BowedStringVoice.h` | MPESynthesiserVoice base, oversampling/tuning/noise/MPE members |
| `Source/BowedStringVoice.cpp` | Rewrite: MPE callbacks, block-based oversampling, tuning engine, bow noise |
| `Source/PluginProcessor.h` | BowedMPESynthesiser member, remove BowedStringSound include |
| `Source/PluginProcessor.cpp` | MPE constructor, tuning engine wiring, latency report, bowNoise param |
| `Source/DSP/SympatheticStringEngine.h` or `.cpp` | Raise energy threshold 1e-7 -> 1e-5 |
| `CMakeLists.txt` | Remove BowedStringSound.h, add BowNoiseGenerator.h + BowedMPESynthesiser.h |

### Delete (1)
| File | Reason |
|------|--------|
| `Source/BowedStringSound.h` | MPESynthesiser has no Sound concept |

---

## Parameters After Phase 3.5

**Newly connected (3 + 1 new):**
- referencePitch -> TuningEngine.setMasterTune() + DroneStringEngine (already partial)
- tuningSystem -> TuningEngine.setMode()
- bowNoise -> BowNoiseGenerator per-voice (NEW APVTS parameter)

**Total connected: 24/24** (bowNoise is new, all parameters now wired)

---

## Success Criteria

- [ ] Build compiles with 0 errors
- [ ] pluginval passes at strictness level 5
- [ ] Oversampling active: friction/waveguide runs at 2x internal rate (verify via sample count in oversampled block)
- [ ] Latency reported to host (setLatencySamples, ~1 sample for IIR 2x)
- [ ] Non-MPE MIDI: notes play correctly via legacy mode (standard keyboard)
- [ ] MPE pitch bend: smooth per-note pitch changes (±48 semitones range)
- [ ] MPE pressure (Z): modulates bow pressure (0.5x at rest, 2.0x at max)
- [ ] MPE timbre (Y/CC74): offsets bow position
- [ ] MPE expression (CC11): scales bow speed
- [ ] TuningEngine: 12-TET produces standard tuning (matches Phase 3.4 baseline)
- [ ] TuningEngine: reference pitch shift heard (change from 440 to 432 Hz)
- [ ] TuningEngine: Scala mode applies custom intervals
- [ ] Bow noise: audible friction texture when bowNoise > 0 (bandpass 2-6 kHz character)
- [ ] Bow noise: scales with pressure × speed (louder with aggressive bowing)
- [ ] Bow noise: 0% = completely silent (no residual noise)
- [ ] Drone strings: unaffected (still use core tier, native rate, no oversampling)
- [ ] Core tier regression: A/B with Phase 3.4 build -- identical at bowNoise=0 + 12-TET + no MPE
- [ ] CPU: 2 strings core tier < 6%, max config < 25%
- [ ] No audio glitches, clicks, or denormals

---

## Risk Mitigations

| Risk | Mitigation |
|------|------------|
| MPE migration breaks non-MPE playback | `enableLegacyMode(2)` in constructor -- standard keyboards work as before |
| Oversampling block size mismatch | Use `oversampling.initProcessing(maxBlockSize)` -- class handles 2x internally |
| WaveguideString delay sizing at 2x | Pass `sampleRate * 2` to `prepare()` -- delay lengths auto-scale |
| BowModel envelope timing at 2x rate | Pass `sampleRate * 2` to `prepare()` -- attack/release in seconds stay correct |
| TuningEngine + MPE pitch conflict | Manually compute: `tuningEngine->getFrequency(note) * pow(2, bendSemitones/12)` -- don't use MPE's pre-computed 12-TET frequency |
| Oversampling latency value | IIR 2x = ~1 sample (fractional), report `ceil()` -- minimal perceptual impact |
| DroneStringEngine regression | Drones use `processSample()` path (unchanged), not junction split, not oversampled |
| Missing bowNoise parameter | Task 3 adds it to APVTS before voice reads it -- coordinated via Wave ordering |
