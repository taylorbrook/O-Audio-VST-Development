# Stage 2 DSP: Phase 2.1 Core Vocal Engine - Plan

**Created:** 2026-04-04
**Phase:** 2.1 of 3
**Goal:** Playable vocal synth -- LF glottal wavetable through formant filter bank with vowel morphing, aspiration noise, and ADSR envelope

---

## Architecture Deviation

**IMPORTANT:** This plan implements **mipmapped wavetable** for the glottal source, deviating from ARCHITECTURE.md's "Direct LF + PolyBLEP" spec. This was explicitly chosen by the user in the discuss phase (see CONTEXT.md).

---

## Tasks

### 1. [ ] Create GlottalWavetable data structure
- **Files:** `Source/dsp/GlottalWavetable.h` (new)
- **Depends on:** none
- **Details:**
  - Flat `std::vector<float>` storage: `[level][rdStep][sample+guard]`
  - Constants: `kTableSize=2048`, `kGuardSamples=1`, `kNumRdSteps=128`, `kNumMipmapLevels=10`
  - `allocate()`, `getSample(level, rdStep, sampleIndex)`, `setSample(...)`, `getFrameData(level, rdStep)`, `setGuardSamples()`
  - Adapted from O-Prism's `WavetableData` pattern
  - Memory: ~10 MB total, shared read-only across all voices

### 2. [ ] Create GlottalTableGenerator
- **Files:** `Source/dsp/GlottalTableGenerator.h`, `Source/dsp/GlottalTableGenerator.cpp` (new)
- **Depends on:** Task 1
- **Details:**
  - `static void generate(GlottalWavetable& table, double sampleRate)` -- runs once at init
  - 128 log-spaced Rd values from 0.3 to 2.7
  - For each Rd: Fant 1995 regression (Ra, Rk, OQ, Rg, Tp, Te, Ta)
  - Newton-Raphson solver for alpha (open-phase zero-integral constraint, ~30 iterations)
  - Newton-Raphson solver for epsilon (return-phase amplitude matching, ~20 iterations)
  - Render one normalized period of LF derivative into 2048 samples (level 0)
  - Normalize each Rd step to peak=1.0 independently
  - FFT-based mipmap generation (adapted from O-Prism WavetableGenerator pattern)
  - Uses `juce::dsp::FFT(11)` for spectral truncation
  - Set guard samples for wrap-around interpolation

### 3. [ ] Create LFGlottalSource per-voice oscillator
- **Files:** `Source/dsp/LFGlottalSource.h` (new)
- **Depends on:** Task 1
- **Details:**
  - Per-voice instance, holds pointer to shared `GlottalWavetable`
  - `prepare(double sampleRate)`, `setFrequency(float f0)`, `setRd(float rd)`, `getNextSample()`
  - Phase accumulator `[0, 1)`, wraps per period
  - Mipmap level selection from frequency: `log2(freq / baseFreq)`
  - Rd index in log space: `(log(Rd) - log(0.3)) / (log(2.7) - log(0.3))`
  - Bilinear interpolation: 4 table lookups (2 Rd x 2 mipmap levels), linear sample interp each
  - Inline `getNextSample()` for hot path performance

### 4. [ ] Create AspirationNoise mixer
- **Files:** `Source/dsp/AspirationNoise.h` (new)
- **Depends on:** none
- **Details:**
  - Per-voice `juce::Random` instance (seeded per voice index for decorrelation)
  - Single-pole IIR lowpass at ~4kHz: `a = exp(-2*pi*4000/sampleRate)`
  - `prepare(double sampleRate)`, `process(float glottalSample, float breathiness) -> float`
  - `SmoothedValue<float>` for breathiness (~20ms ramp)
  - Mix: `(1 - breath) * glottal + breath * filteredNoise`
  - `reset()` to clear filter state

### 5. [ ] Create VowelData formant tables
- **Files:** `Source/dsp/VowelData.h` (new)
- **Depends on:** none
- **Details:**
  - `constexpr` struct with F1-F5 frequencies, bandwidths, and gains for 5 cardinal vowels (A, E, I, O, U)
  - Csound bass voice data (verified in RESEARCH.md)
  - XY positions from ARCHITECTURE.md: I(0.00,1.00), E(0.31,0.43), A(0.83,0.00), O(1.00,0.35), U(0.98,0.93)
  - Gains stored as linear (pre-converted from dB via `Decibels::decibelsToGain`)
  - Static array of `VowelData` structs

### 6. [ ] Create FormantBiquad struct
- **Files:** `Source/dsp/FormantBiquad.h` (new)
- **Depends on:** none
- **Details:**
  - 32-byte cache-friendly struct: `b0, b1, b2, a1, a2, z1, z2, gain`
  - Direct Form II Transposed topology
  - Inline `processSample(float input) -> float` (returns `output * gain`)
  - `setCoefficients(const std::array<float, 6>& coeffs)` -- from `ArrayCoefficients::makeBandPass`
  - `reset()` to clear z1/z2 state

### 7. [ ] Create FormantFilterBank (5 parallel BPFs)
- **Files:** `Source/dsp/FormantFilterBank.h` (new)
- **Depends on:** Task 6
- **Details:**
  - Array of 5 `FormantBiquad` filters
  - `prepare(double sampleRate)`, `process(float input) -> float` (sum of 5 filter outputs)
  - `updateCoefficients(float freq[5], float bw[5], float gain[5], float shift, float spread, double sampleRate)`
  - Formant shift: `freq *= pow(2, shift/12)` semitone-based
  - Formant spread: scale distance from center-of-mass
  - Frequency clamping to `[20, sampleRate/2 - 100]` before `makeBandPass`
  - Q computed as `freq / bandwidth`, clamped min 0.5
  - `reset()` to clear all filter states

### 8. [ ] Create VowelMorpher (Shepard interpolation)
- **Files:** `Source/dsp/VowelMorpher.h` (new)
- **Depends on:** Task 5
- **Details:**
  - `compute(float cursorX, float cursorY, float focus, float outFreq[5], float outBW[5], float outGain[5])`
  - IDW weights: `w_i = 1 / dist^focus`, normalized
  - Epsilon guard: if `dist < 1e-6`, snap directly to that vowel
  - Frequencies interpolated in log domain: `exp(sum(w * log(f)))`
  - Bandwidths and gains interpolated linearly
  - Runs at block rate (every 32 samples) -- trivial cost

### 9. [ ] Integrate DSP into FormantVoice
- **Files:** `Source/FormantVoice.h` (modify), `Source/FormantVoice.cpp` (modify)
- **Depends on:** Tasks 3, 4, 7, 8
- **Details:**
  - Add member instances: `LFGlottalSource`, `AspirationNoise`, `FormantFilterBank`, `VowelMorpher`, `juce::ADSR`
  - Add `setWavetable(const GlottalWavetable*)` method
  - Add `prepare(double sampleRate)` method to initialize DSP components
  - `noteStarted()`: set ADSR params from APVTS, call `adsr.noteOn()`, set frequency from MPE note
  - `noteStopped(allowTailOff)`: `adsr.noteOff()` or `adsr.reset() + clearCurrentNote()`
  - `renderNextBlock()`: full per-sample loop:
    - Block-rate (every 32 samples): read vowelX/Y/focus/shift/spread, compute morph, update filter coefficients
    - Per sample: read f0 from MPE note, set glottal frequency + Rd, get glottal sample, mix aspiration noise, filter through formant bank, apply ADSR envelope, write to output buffer (mono to both L/R)
    - After block: check `adsr.isActive()`, call `clearCurrentNote()` if inactive
  - Add `sampleCounter` for block-rate updates
  - ADSR params updated per-block (safe per RESEARCH.md)

### 10. [ ] Integrate wavetable into PluginProcessor
- **Files:** `Source/PluginProcessor.h` (modify), `Source/PluginProcessor.cpp` (modify)
- **Depends on:** Tasks 2, 9
- **Details:**
  - Add `GlottalWavetable` member (owned, shared read-only)
  - In constructor: call `GlottalTableGenerator::generate(wavetable, 44100.0)` (initial generation)
  - In `prepareToPlay()`: regenerate wavetable if sample rate changed, pass pointer to all 16 voices, call `voice->prepare(sampleRate)` on each
  - Cast voices to `FormantVoice*` via `dynamic_cast` to access custom methods
  - Include new DSP headers

### 11. [ ] Update CMakeLists.txt with new source files
- **Files:** `CMakeLists.txt` (modify)
- **Depends on:** Tasks 1-8
- **Details:**
  - Add all new files to `target_sources`:
    - `Source/dsp/GlottalWavetable.h`
    - `Source/dsp/GlottalTableGenerator.h`
    - `Source/dsp/GlottalTableGenerator.cpp`
    - `Source/dsp/LFGlottalSource.h`
    - `Source/dsp/AspirationNoise.h`
    - `Source/dsp/VowelData.h`
    - `Source/dsp/FormantBiquad.h`
    - `Source/dsp/FormantFilterBank.h`
    - `Source/dsp/VowelMorpher.h`
  - Add `Source/dsp` to include directories

### 12. [ ] Build and validate
- **Files:** none (build + test)
- **Depends on:** Tasks 10, 11
- **Details:**
  - `cmake --build build --target O-Formant_VST3 O-Formant_AU` (or ninja equivalent)
  - Install to system plugin folders with AU cache clearing
  - Verify with pluginval basic scan
  - Test in DAW: MIDI notes produce vowel sound, Rd changes timbre, XY morphs vowels, ADSR shapes amplitude

---

## File Summary

| Action | File |
|--------|------|
| CREATE | `Source/dsp/GlottalWavetable.h` |
| CREATE | `Source/dsp/GlottalTableGenerator.h` |
| CREATE | `Source/dsp/GlottalTableGenerator.cpp` |
| CREATE | `Source/dsp/LFGlottalSource.h` |
| CREATE | `Source/dsp/AspirationNoise.h` |
| CREATE | `Source/dsp/VowelData.h` |
| CREATE | `Source/dsp/FormantBiquad.h` |
| CREATE | `Source/dsp/FormantFilterBank.h` |
| CREATE | `Source/dsp/VowelMorpher.h` |
| MODIFY | `Source/FormantVoice.h` |
| MODIFY | `Source/FormantVoice.cpp` |
| MODIFY | `Source/PluginProcessor.h` |
| MODIFY | `Source/PluginProcessor.cpp` |
| MODIFY | `CMakeLists.txt` |

**New files:** 9 | **Modified files:** 5

---

## Dependency Graph

```
Task 1 (GlottalWavetable) ──┬──> Task 2 (TableGenerator) ──> Task 10 (Processor integration)
                             └──> Task 3 (LFGlottalSource) ──┐
Task 4 (AspirationNoise) ────────────────────────────────────┤
Task 5 (VowelData) ──> Task 8 (VowelMorpher) ───────────────┤
Task 6 (FormantBiquad) ──> Task 7 (FormantFilterBank) ──────┤
                                                             └──> Task 9 (FormantVoice integration)
                                                                       │
Tasks 1-8 ──> Task 11 (CMakeLists.txt) ──────────────────────────────┤
                                                                       └──> Task 10 ──> Task 12 (Build)
```

**Parallelizable:** Tasks 1, 4, 5, 6 can all be built independently. Tasks 3, 7, 8 depend on their data structures but not on each other.

---

## Success Criteria (from ROADMAP.md)

- [ ] Playing MIDI notes produces voiced "aah" sound (formant A)
- [ ] glottalRd parameter audibly changes voice quality (pressed to breathy)
- [ ] breathiness parameter adds noise to glottal source
- [ ] ADSR envelope shapes amplitude correctly (verify attack/release)
- [ ] vowelX/Y parameters morph between vowels (hear I/E/A/O/U)
- [ ] vowelFocus parameter changes interpolation sharpness
- [ ] formantShift shifts gender (male to female)
- [ ] No clicks, pops, or NaN at any parameter setting
- [ ] Full MIDI range (C0-C8) produces output without aliasing artifacts below C5
- [ ] 16-voice polyphony works without voice stealing artifacts
