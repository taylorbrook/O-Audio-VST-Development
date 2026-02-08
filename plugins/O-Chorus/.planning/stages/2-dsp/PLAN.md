# Stage 2: DSP — Execution Plan

**Plugin:** O-Chorus
**Stage:** 2-dsp
**Created:** 2026-02-07
**Context:** CONTEXT.md, ARCHITECTURE.md

---

## Goal

Implement the complete multi-voice BBD-style chorus DSP engine: modulated delay lines with Lagrange3rd interpolation, per-voice LFO with phase distribution, tanh saturation with adjustable drive, one-pole tone filter, equal-power stereo panning, and dry/wet mix — all connected to the 7 APVTS parameters.

---

## Tasks

### 1. [ ] Create ChorusEngine class with voice struct and delay lines
- **Files:** `Source/DSP/ChorusEngine.h`, `Source/DSP/ChorusEngine.cpp`
- **Depends on:** none
- **Details:**
  - Define `ChorusVoice` struct: `DelayLine<float, Lagrange3rd>`, `lfoPhaseOffset`, `depthVariation`, `panPosition`
  - Fixed array of 8 voices (only first N active based on voices param)
  - `prepare(double sampleRate, int samplesPerBlock)` — init delay lines (50ms max), generate seeded depth variations (0.85-1.15), compute phase offsets
  - `reset()` — clear all delay lines
  - `setVoiceCount(int)` — recalculate phase offsets and pan positions
  - Store `SmoothedValue<float>` for rate, depth, width, tone, mix, drive params (50ms ramp, 100ms for tone)

### 2. [ ] Implement LFO and per-voice delay modulation
- **Files:** `Source/DSP/ChorusEngine.cpp`
- **Depends on:** Task 1
- **Details:**
  - Global LFO phase accumulator: `phaseIncrement = (rate * 2pi) / sampleRate`
  - Per-voice phase: `globalPhase + voice.lfoPhaseOffset`
  - Per-voice modulated delay: `baseDelay (10ms) + sin(phase) * depth * voice.depthVariation * delayRange (5ms)`
  - Convert ms to samples, use `delayLine.popSample()` then `delayLine.pushSample()`
  - Wrap phase at 2pi to prevent float precision loss

### 3. [ ] Implement tanh saturation with drive parameter
- **Files:** `Source/DSP/ChorusEngine.cpp`
- **Depends on:** Task 1
- **Details:**
  - `saturate(float sample, float drive)` inline function
  - Asymmetric: positive half driven at 1.0x, negative at 0.9x
  - Normalized: `tanh(driven) / tanh(1 + scaledDrive)` for unity gain
  - Bypass when drive < 0.01
  - Applied per-voice AFTER delay read, BEFORE tone filter

### 4. [ ] Implement one-pole tone filter
- **Files:** `Source/DSP/ChorusEngine.cpp`
- **Depends on:** Task 1
- **Details:**
  - `juce::dsp::IIR::Filter<float>` per channel (2 filters for stereo wet bus)
  - Cutoff mapping: tone -1.0 → 2kHz, 0.0 → 8kHz, +1.0 → 20kHz (piecewise linear in log)
  - Update coefficients when tone param changes (via `SmoothedValue` target comparison)
  - `makeLowPass(sampleRate, cutoffFreq)` from JUCE IIR Coefficients
  - Applied to summed wet signal (after all voices accumulated per channel)

### 5. [ ] Implement equal-power stereo panning and mix
- **Files:** `Source/DSP/ChorusEngine.cpp`
- **Depends on:** Task 1
- **Details:**
  - Per-voice pan position: `i / (numVoices - 1)` for stereo spread (centered if 1 voice)
  - Width scales effective pan: `effectivePan = 0.5 + (panPos - 0.5) * width`
  - Equal-power: `leftGain = cos(angle)`, `rightGain = sin(angle)`
  - Accumulate all voices into wet L/R buffers
  - Mix stage: `output = dry * (1 - mix) + wet * mix`

### 6. [ ] Implement voice count crossfade
- **Files:** `Source/DSP/ChorusEngine.cpp`
- **Depends on:** Tasks 1, 2, 5
- **Details:**
  - Track `currentVoiceCount` and `targetVoiceCount`
  - When voices param changes, set target and begin ~50ms crossfade
  - During crossfade: process both old and new voice counts, blend output
  - After crossfade complete: update currentVoiceCount, recalculate phase offsets and pan positions
  - Prevents clicks when changing voice count during playback

### 7. [ ] Wire ChorusEngine into PluginProcessor
- **Files:** `Source/PluginProcessor.h`, `Source/PluginProcessor.cpp`
- **Depends on:** Tasks 1-6
- **Details:**
  - Add `ChorusEngine chorusEngine` member
  - `prepareToPlay()`: call `chorusEngine.prepare(sampleRate, samplesPerBlock)`
  - `processBlock()`: read all 7 params via atomic loads, pass to engine, process buffer
  - Mono sum input: `(L + R) / 2` fed to chorus voices, original L/R preserved for dry path
  - Process sample-by-sample (not block — LFO needs per-sample phase advance)
  - Output: `dry * (1-mix) + wetL/R * mix`

### 8. [ ] Add CMakeLists.txt source entries for new files
- **Files:** `CMakeLists.txt`
- **Depends on:** Task 1
- **Details:**
  - Add `Source/DSP/ChorusEngine.h` and `Source/DSP/ChorusEngine.cpp` to `target_sources`

### 9. [ ] Build and verify
- **Files:** none (build only)
- **Depends on:** Tasks 7, 8
- **Details:**
  - `ninja OuariconChorus_VST3 OuariconChorus_AU`
  - Verify zero compile errors and warnings
  - Install to system plugin folders
  - Clear AU cache
  - Verify AU detection via `auval -a | grep -i chorus`

---

## Processing Signal Flow (Reference)

```
Stereo Input L/R
    |
    +-- [Dry Path: preserve original L/R] --------------------+
    |                                                          |
    +-- [Mono Sum: (L+R)/2] --> chorus input                  |
            |                                                  |
            +-- Voice 0: LFO(phase+0) --> Delay --> Saturate  |
            +-- Voice 1: LFO(phase+offset) --> Delay --> Sat  |
            +-- Voice N: ...                                   |
                    |                                          |
                    [Pan each voice L/R by position * width]   |
                    [Sum all voices into wetL, wetR]           |
                    [Apply tone filter to wetL, wetR]          |
                                                               |
    output L = dryL * (1-mix) + wetL * mix  <-----------------+
    output R = dryR * (1-mix) + wetR * mix
```

---

## Success Criteria

- [ ] Plugin builds (VST3 + AU) with zero errors
- [ ] Chorus effect audible: single MIDI note or audio input produces chorused output
- [ ] Rate parameter sweeps LFO speed smoothly (0.05-5 Hz)
- [ ] Depth parameter controls modulation amount (0 = no effect, 1 = full)
- [ ] Voices parameter changes voice count (1-8) without clicks
- [ ] Width parameter controls stereo spread (0 = mono, 1 = full)
- [ ] Tone parameter sweeps filter (dark to bright)
- [ ] Drive parameter controls saturation amount (0 = clean, 1 = warm)
- [ ] Mix at 0% = dry bypass, Mix at 100% = wet only (vibrato)
- [ ] No audio glitches, clicks, or pops during parameter changes
- [ ] No CPU spikes when processing silence

---

## Files Summary

**Create:**
- `Source/DSP/ChorusEngine.h`
- `Source/DSP/ChorusEngine.cpp`

**Modify:**
- `Source/PluginProcessor.h` (add ChorusEngine member)
- `Source/PluginProcessor.cpp` (wire DSP into prepareToPlay/processBlock)
- `CMakeLists.txt` (add new source files)
