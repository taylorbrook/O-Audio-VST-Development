# Stage 2 DSP: Phase 2.2 Modulation & Expression - Plan

**Created:** 2026-04-05
**Phase:** 2.2 of 3
**Goal:** Add vibrato, pitch glide, consonant noise engine, and MPE expression to the vocal synth -- connecting the remaining 10 parameters and enabling expressive real-time performance

---

## Tasks

### 1. [ ] Create VibratoLFO class
- **Files:** `Source/dsp/VibratoLFO.h` (new)
- **Depends on:** none
- **Details:**
  - Header-only, follows existing DSP class pattern
  - Double-precision phase accumulator (matches LFGlottalSource pattern)
  - `prepare(double sampleRate)`, `noteOn(float delayMs)`, `getNextValue(float rateHz, float depthCents) -> float`
  - Returns pitch modulation in cents: `depthCents * sin(phase * 2pi) * delayGain`
  - Onset delay: linear ramp from 0 to 1 over `vibratoDelay` ms (sample counter)
  - Micro-jitter: +/-0.5% F0 perturbation per LFO cycle (zero-crossing detection)
  - `getJitterOffset() -> float` for separate F0 multiplier application
  - Per-voice `juce::Random` for jitter (no constructor seed needed -- only jitter uses it)

### 2. [ ] Create PitchGlide class
- **Files:** `Source/dsp/PitchGlide.h` (new)
- **Depends on:** none
- **Details:**
  - Header-only, adapted from O-Prism GlideProcessor (simplified: no mode switching)
  - One-pole exponential smoother: `coeff = exp(-1.0 / (timeMs * 0.001 * sampleRate))`
  - `prepare(double sampleRate)`, `setTarget(float freqHz)`, `snapTo(float freqHz)`, `setTime(float timeMs)`
  - `getNextFrequency() -> float`: `current = current * coeff + target * (1 - coeff)`
  - When `coeff == 0` (glide disabled): returns target immediately
  - Convergence threshold: `abs(current - target) < target * 0.00001f` snaps to target

### 3. [ ] Create ConsonantEngine class
- **Files:** `Source/dsp/ConsonantEngine.h` (new)
- **Depends on:** none (reuses FormantBiquad internally)
- **Details:**
  - Header-only, KLATT-derived parallel noise branch
  - 3x `FormantBiquad` instances: LP (~2kHz), HP (~6kHz), sibilance BP (4-8kHz)
  - Per-voice `juce::Random` (seed = `voiceIndex * 37 + 23`, different from AspirationNoise)
  - `prepare(double sampleRate, int voiceIndex)`, `reset()`
  - `updateCoefficients(float consonantTone, float sibilance, double sampleRate)` -- block-rate
  - Tone crossfade: `(1-tone) * lpOut + tone * hpOut + sibilance * bpOut`
  - Auto-consonant plosive burst:
    - `triggerBurst(float velocity)` -- sets `burstSamplesRemaining`, amplitude = velocity
    - 15ms burst duration (~661 samples at 44.1kHz)
    - Falling exponential envelope: `exp(-5.0 * progress) * burstAmplitude`
  - `getNextSample(float consonantLevel, bool autoConsonant) -> float`
  - Early-out: returns 0 when `consonantLevel < 0.001f AND !(autoConsonant && burst active)`
  - LP coefficients: `ArrayCoefficients::makeLowPass(sr, 2000, 0.707)`
  - HP coefficients: `ArrayCoefficients::makeHighPass(sr, 6000, 0.707)`
  - Sibilance: `ArrayCoefficients::makeBandPass(sr, 5500, 2.0 + sibilance * 8.0)`

### 4. [ ] Integrate VibratoLFO + PitchGlide into FormantVoice
- **Files:** `Source/FormantVoice.h` (modify), `Source/FormantVoice.cpp` (modify)
- **Depends on:** Tasks 1, 2
- **Details:**
  - Add members: `VibratoLFO vibratoLFO`, `PitchGlide pitchGlide`
  - Add state: `bool wasActive = false` (for glide first-note detection)
  - `prepare()`: call `vibratoLFO.prepare(sr)`, `pitchGlide.prepare(sr)`
  - `noteStarted()`:
    - Read `vibratoDelay` from APVTS, call `vibratoLFO.noteOn(delayMs)`
    - Read `pitchGlide` from APVTS, call `pitchGlide.setTime(glideMs)`
    - If `glideMs > 0 && wasActive`: `pitchGlide.setTarget(f0)` (glide from old pitch)
    - Else: `pitchGlide.snapTo(f0)` (immediate jump)
    - Set `wasActive = true` at end
  - `noteStopped()`: only reset `wasActive = false` when voice fully clears (non-tail-off path)
  - `renderNextBlock()`: change F0 from block-rate to per-sample:
    - Remove block-level `glottalSource.setFrequency(f0)` line
    - In per-sample loop, BEFORE glottal source:
      ```
      float baseF0 = pitchGlide.getNextFrequency();
      float vibCents = vibratoLFO.getNextValue(vibratoRate, vibratoDepth);
      float jitter = vibratoLFO.getJitterOffset();
      float finalF0 = baseF0 * std::pow(2.0f, vibCents / 1200.0f) * (1.0f + jitter);
      glottalSource.setFrequency(finalF0);
      ```
    - Read vibratoRate/vibratoDepth at block-rate (every 32 samples)

### 5. [ ] Integrate ConsonantEngine into FormantVoice
- **Files:** `Source/FormantVoice.h` (modify), `Source/FormantVoice.cpp` (modify)
- **Depends on:** Task 3, Task 4 (sequential -- both modify same files)
- **Details:**
  - Add member: `ConsonantEngine consonantEngine`
  - Pass `voiceIndex` to ConsonantEngine in constructor
  - `prepare()`: call `consonantEngine.prepare(sr, voiceIndex)`
  - `noteStarted()`: call `consonantEngine.reset()`, then if `autoConsonant`: `consonantEngine.triggerBurst(velocity)`
  - `renderNextBlock()` block-rate: call `consonantEngine.updateCoefficients(consonantTone, sibilance, sr)`
  - `renderNextBlock()` per-sample, AFTER formant filtering:
    ```
    float consonantOut = consonantEngine.getNextSample(consonantLevel, autoConsonant);
    float mixed = filtered + consonantOut;  // consonantLevel scaling is inside getNextSample
    float sample = mixed * env;
    ```
  - Signal flow: consonant output is envelope-gated by same ADSR (mixed before envelope)

### 6. [ ] Integrate MPE expression into FormantVoice
- **Files:** `Source/FormantVoice.h` (modify), `Source/FormantVoice.cpp` (modify)
- **Depends on:** Task 5 (sequential -- modifies same files)
- **Details:**
  - Add per-voice state: `float mpeBreathOffset = 0.0f`, `float mpeVowelYOffset = 0.0f`, `float noteVelocity = 0.0f`
  - `noteStarted()`: store velocity = `getCurrentlyPlayingNote().noteOnVelocity.asUnsignedFloat()`
  - `notePressureChanged()`:
    - `mpeBreathOffset = getCurrentlyPlayingNote().pressure.asUnsignedFloat()`
  - `noteTimbreChanged()`:
    - `mpeVowelYOffset = getCurrentlyPlayingNote().timbre.asUnsignedFloat() - 0.5f` (range -0.5 to +0.5)
  - `renderNextBlock()` block-rate updates:
    - Breathiness: `effectiveBreath = knobBreath + mpeBreathOffset * (1.0 - knobBreath)` -- pressure adds, knob sets floor
    - VowelY: `vowelY = pVowelY->load() + mpeVowelYOffset`, clamped 0-1
  - `noteStarted()` reset: clear `mpeBreathOffset = 0`, `mpeVowelYOffset = 0`

### 7. [ ] Update CMakeLists.txt
- **Files:** `CMakeLists.txt` (modify)
- **Depends on:** Tasks 1-3
- **Details:**
  - No changes needed -- header-only files in `Source/dsp/` are already on the include path
  - Verify: no new .cpp files being added (all 3 new files are header-only)

### 8. [ ] Build, validate, and test
- **Files:** none (build + test)
- **Depends on:** Tasks 4, 5, 6, 7
- **Details:**
  - `ninja O-Formant_VST3 O-Formant_AU`
  - Clear AU cache, install to system plugin folders
  - pluginval at strictness 5
  - DAW test: verify vibrato, glide, consonants, MPE expression

---

## File Summary

| Action | File |
|--------|------|
| CREATE | `Source/dsp/VibratoLFO.h` |
| CREATE | `Source/dsp/PitchGlide.h` |
| CREATE | `Source/dsp/ConsonantEngine.h` |
| MODIFY | `Source/FormantVoice.h` |
| MODIFY | `Source/FormantVoice.cpp` |

**New files:** 3 | **Modified files:** 2

---

## Dependency Graph

```
Task 1 (VibratoLFO) ──────┐
Task 2 (PitchGlide) ──────┤
                           └──> Task 4 (Integrate vibrato + glide into FormantVoice)
Task 3 (ConsonantEngine) ──────> Task 5 (Integrate consonant into FormantVoice)
                                        │
                                        └──> Task 6 (Integrate MPE into FormantVoice)
                                                    │
Tasks 1-3 ──> Task 7 (CMake verify) ───────────────┤
                                                    └──> Task 8 (Build + validate)
```

**Parallelizable:** Tasks 1, 2, 3 are fully independent. Tasks 4, 5, 6 are sequential (all modify FormantVoice).

---

## Parameters Connected by This Phase

| Parameter | Connected To | Integration |
|-----------|-------------|-------------|
| vibratoRate | VibratoLFO | Block-rate read in renderNextBlock |
| vibratoDepth | VibratoLFO | Block-rate read in renderNextBlock |
| vibratoDelay | VibratoLFO | Read in noteStarted() |
| pitchGlide | PitchGlide | Read in noteStarted() |
| consonantLevel | ConsonantEngine | Block-rate read, scales consonant mix |
| consonantTone | ConsonantEngine | Block-rate coefficient update |
| sibilance | ConsonantEngine | Block-rate coefficient update |
| autoConsonant | ConsonantEngine | Read per-sample for burst gating |
| (MPE pressure) | AspirationNoise | Per-note breathiness offset |
| (MPE timbre) | VowelMorpher | Per-note vowelY offset |

**After Phase 2.2:** 19 of 21 parameters connected (remaining: outputGain, stereoWidth -- Phase 2.3)

---

## Success Criteria

- [ ] Vibrato audible: sustained note develops periodic pitch wobble after delay period
- [ ] Vibrato delay works: no vibrato at onset, fades in after vibratoDelay ms
- [ ] Micro-jitter: sustained tones sound slightly alive/organic vs. Phase 2.1's static pitch
- [ ] Pitch glide: rapid note changes glide smoothly when pitchGlide > 0
- [ ] Pitch glide disabled: pitchGlide = 0 produces instant pitch changes (no smearing)
- [ ] Consonant noise: increasing consonantLevel adds noise layer to vowel output
- [ ] Consonant tone sweep: tone 0 = dark fricative (/f/), tone 1 = bright fricative (/s/)
- [ ] Sibilance: increasing sibilance adds peaked resonance in 4-8kHz range
- [ ] Auto-consonant burst: enabling autoConsonant produces brief noise transient at note onset
- [ ] Velocity scaling: harder MIDI velocity = louder consonant burst
- [ ] MPE pressure: aftertouch increases breathiness above knob baseline
- [ ] MPE timbre: CC74/slide shifts vowel Y-axis position
- [ ] No clicks, pops, or NaN at any parameter setting
- [ ] pluginval passes at strictness 5
- [ ] 16-voice polyphony with all features enabled -- no CPU dropouts

---

## Requirements Addressed

| Requirement | Priority | Coverage |
|-------------|----------|----------|
| FUNC-05 | must | Consonant noise injection via ConsonantEngine |
| FUNC-07 | should | MPE pressure -> breathiness, timbre -> vowelY |
| FUNC-09 | should | Auto-consonant plosive burst with velocity scaling |
| FUNC-10 | should | Vibrato LFO with delay and micro-jitter |
| FUNC-11 | nice | Per-voice pitch glide (exponential smoother) |
| DSP-08 | should | Consonant tone shaping (LP/HP crossfade) + sibilance (BP resonance) |
