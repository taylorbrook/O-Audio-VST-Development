# Stage 2: DSP Phase 3.5 - Research

**Date:** 2026-04-05
**Scope:** Oversampling, MPE, Tuning Engine, Bow Noise, Optimization

---

## 1. Per-Voice Oversampling

### Filter Type: `filterHalfBandPolyphaseIIR`

**Choice:** IIR over FIR. Confirmed from JUCE 8 source.

| Property | IIR (Polyphase) | FIR (Equiripple) |
|----------|-----------------|-------------------|
| Latency | ~0.5-1.5 samples | ~10-20+ samples |
| Phase | Nonlinear near Nyquist | Linear |
| CPU | Lower | Higher |

Phase linearity near Nyquist is irrelevant -- that's the inaudible band being discarded. Minimum latency matters for real-time feel. The "~8 samples" in the architecture doc was wrong (FIR figure).

### Per-Voice Ownership

8 separate `Oversampling<float>(1, 1, filterHalfBandPolyphaseIIR)` instances are safe.

Memory per instance: ~`(maxBlockSize * 2 + 50)` floats = ~4KB at 512 block size. 32KB total for 8 voices. Negligible.

Each is `JUCE_DECLARE_NON_COPYABLE` -- own by value in voice, call `initProcessing()` in `prepareToPlay()`.

### Block Processing (RESTRUCTURE REQUIRED)

`juce::dsp::Oversampling` has **no per-sample API**. The interface is:

```cpp
auto oversampledBlock = oversampling.processSamplesUp(inputBlock);
// ... process oversampledBlock at 2x rate (length = numSamples * 2) ...
oversampling.processSamplesDown(outputBlock);
```

**Current `renderNextBlock` must be restructured:**

1. Allocate a mono `voiceBuffer` (owned by voice, sized in `prepareToPlay`)
2. `processSamplesUp(silence)` -- gets correctly-sized internal buffer reference
3. Run friction + waveguide per-sample loop at 2x rate into the oversampled buffer
4. `processSamplesDown(voiceBuffer)` -- anti-aliasing decimation
5. Mix voiceBuffer into output with panning

### WaveguideString at 2x Rate

`WaveguideString::prepare()` must receive `sampleRate * 2` so delay line lengths and bridge filter coefficients are correct at the internal rate.

BowModel envelope rate also needs adjustment -- envelope runs at 2x, so attack/release times double in samples.

### initProcessing

Pass **host block size** (pre-oversampling). Class multiplies internally:
```cpp
oversampling.initProcessing((size_t)maxBlockSize); // e.g. 512 -> allocates 1024 internally
```

### Latency Reporting

IIR 2x latency is ~1 sample (fractional). Report via:
```cpp
setLatencySamples(static_cast<int>(std::ceil(oversampling.getLatencyInSamples())));
```

Note: `getLatencySamples()` is non-virtual in JUCE 8 (from memory). Use `setLatencySamples()` in `prepareToPlay()`.

---

## 2. MPE Implementation

### Decision: Migrate to `MPESynthesiser` + `MPESynthesiserVoice`

**Standard `juce::Synthesiser` has no MPE awareness.** Pitch bend routing happens to work per-channel (by accident), but:
- No zone management (master vs member channels)
- No pitchbend range tracking (±48 semitone default)
- Master channel messages don't broadcast to member channels
- Channel pressure routing requires manual override

**`MPESynthesiser` gives all of this for free** with moderate migration cost.

### Migration Scope

**Voice class:** `SynthesiserVoice` -> `MPESynthesiserVoice`

| Old Method | New Method |
|------------|------------|
| `startNote(note, vel, sound, pitchWheel)` | `noteStarted()` -- read `getCurrentlyPlayingNote()` |
| `stopNote(vel, tailOff)` | `noteStopped(bool allowTailOff)` |
| `pitchWheelMoved(int)` | `notePitchbendChanged()` -- `currentlyPlayingNote.totalPitchbendInSemitones` |
| `controllerMoved(int, int)` | `noteTimbreChanged()` for CC74 |
| `channelPressureChanged(int)` | `notePressureChanged()` -- `currentlyPlayingNote.pressure` |
| `canPlaySound(Sound*)` | (removed -- no Sound concept) |
| `renderNextBlock(...)` | Same signature |

**Processor class:** `Synthesiser` -> custom `BowedMPESynthesiser : MPESynthesiser`

- Remove `addSound()` call (no Sound concept in MPE)
- Default constructor creates lower zone with 15 member channels (2-16) -- standard MPE layout
- Override `handleController()` for CC11 (Expression -> bow speed multiply)

### MPE Dimension Mapping

| MPE Dimension | Voice Callback | Target | Mapping |
|---------------|----------------|--------|---------|
| Pitch Bend | `notePitchbendChanged()` | Frequency | `currentlyPlayingNote.getFrequencyInHertz()` (pre-computed) |
| Pressure (Z) | `notePressureChanged()` | Bow Pressure | `0.5 + pressure * 1.5` (0.5x-2.0x multiply) |
| Timbre (Y/CC74) | `noteTimbreChanged()` | Bow Position | `base + timbre * range` (signed float offset) |
| CC11 (Expression) | Custom `handleController()` | Bow Speed | `0.5 + (cc/127) * 1.5` (0.5x-2.0x multiply) |

### Thread Safety

All MPE callbacks execute on the audio thread during `processBlock()`. Direct member variable updates are safe. No atomics needed within the voice.

### CC11 Handling

CC11 is NOT an MPE dimension -- `MPEInstrument` ignores it. Custom synthesiser override required:

```cpp
class BowedMPESynthesiser : public juce::MPESynthesiser {
    void handleController(int ch, int cc, int val) override {
        if (cc == 11) { /* dispatch to voices */ }
        // No super call needed -- base implementation is empty
        // CC64/CC66 are handled separately by MPEInstrument
    }
};
```

### Legacy Mode

`MPESynthesiser` supports legacy mode for non-MPE controllers:
```cpp
synthesiser.enableLegacyMode(2); // standard ±2 semitone bend
```

This should be default, with MPE auto-detected from zone configuration messages.

---

## 3. Tuning Engine Integration

### Pattern (from O-Lyrica, verified in source)

1. **Processor** owns `TuningEngine tuningEngine` (already done in O-Bowed)
2. **Voice** gets pointer: `voice->setTuningEngine(&tuningEngine)` during voice setup in constructor
3. **`noteStarted()`**: `currentFrequency = tuningEngine->getFrequency(midiNote)` instead of `MidiMessage::getMidiNoteInHertz()`
4. **`referencePitch`** parameter: `tuningEngine.setMasterTune(refPitch)` each processBlock
5. **`tuningSystem`** parameter: `tuningEngine.setMode()` on change

### TuningEngine API (from module header)

- `getFrequency(int midiNote, int midiChannel = 0)` -- audio-thread safe, reads pre-computed frequency table
- `setMasterTune(double freqHz)` -- sets A4 reference, rebuilds frequency table
- `setMode(Mode mode)` -- `TwelveTET`, `Scala`, `MTSESP`
- `setBuiltInPreset(BuiltInPreset)` -- Pythagorean, Zarlino, Meantone, Werckmeister, etc.
- `loadScalaFile(File)` / `loadKBMFile(File)` -- file I/O (message thread only)
- `setPitchBend(int midiNote, float amount)` / `clearPitchBend()` -- per-note bend

### Thread Safety

`getFrequency()` reads from `std::array<std::atomic<double>, 128> frequencyTable` -- lock-free, audio-thread safe.

`setMode()`, `setCustomIntervals()`, `loadScalaFile()` use `std::mutex intervalMutex` -- call from message thread or processBlock (acceptable if rare, mutex contention minimal for block-rate updates).

`setMasterTune()` triggers `rebuildFrequencyTable()` which locks `intervalMutex` -- safe to call from processBlock at block rate.

### Integration with MPESynthesiserVoice

With MPE, `getCurrentlyPlayingNote().getFrequencyInHertz()` computes frequency from 12-TET + pitchbend. For microtonal tuning, we need to:

1. Get the MIDI note from `getCurrentlyPlayingNote().initialNote`
2. Get base frequency from `tuningEngine->getFrequency(midiNote)`
3. Apply MPE pitch bend on top: `baseFreq * pow(2, bendSemitones / 12.0f)`

The MPE pre-computed frequency can't be used directly since it assumes 12-TET base.

### DroneStringEngine

Already uses `setReferencePitch(refPitch)` in processBlock. This will remain as-is -- drones don't go through TuningEngine.

---

## 4. Bow Noise Generator

### Filter Design

**Single bandpass biquad** at 3464 Hz (geometric mean of 2-6 kHz range), Q = 0.87.

```cpp
juce::dsp::IIR::Coefficients<float>::makeBandPass(sampleRate, 3464.0f, 0.87f)
```

One biquad = half the CPU of HP+LP cascade. Q gives direct bandwidth control. Sufficient for broadband friction noise.

### Fixed Center Frequency

Do NOT track fundamental. Bow noise is broadband friction noise whose spectrum depends on rosin/hair mechanics, not string pitch. Guettler 2011 confirms this. Tracking would be acoustically wrong.

### Random Source

`juce::Random::nextFloat()` is real-time safe (LCG, no syscalls). Each voice MUST own its own `juce::Random` instance seeded by voice index to avoid correlated noise across voices.

```cpp
juce::Random noiseRandom { static_cast<juce::int64>(voiceIndex * 31337) };
```

### Amplitude Model

```cpp
float noiseAmp = bowPressure * bowSpeed * bowNoiseParam * 0.03f;
```

Acoustically grounded -- Guettler 2011 confirms noise scales with force x velocity. Real violins: ~1% output energy as noise. One-pole smooth the amplitude at block-rate to avoid clicks.

### Signal Chain Position

```
BodyResonator -> + BowNoiseGenerator -> SympatheticStringEngine -> StereoWidth
```

Post-body, additive. NOT in waveguide feedback loop.

---

## 5. CPU Optimization

### ScopedNoDenormals Coverage

The `ScopedNoDenormals` in `processBlock()` modifies the thread's MXCSR register via RAII. It covers everything called from that thread:
- `Synthesiser::renderNextBlock()` -> all voice rendering
- Oversampled inner loop (same thread)
- Body resonator, sympathetic engine, etc.

**No per-voice instances needed.**

### Energy Gating Threshold

Current sympathetic string threshold `1e-7f` (-140 dBFS) is overly conservative. Raise to `1e-5f` (-100 dBFS):
- Still 40 dB below 16-bit noise floor
- Strings settle to gated state ~30% faster
- Meaningful CPU savings with 12 sympathetic strings

### Division Avoidance

Current codebase is clean -- no per-sample divisions in inner loops. For Phase 3.5, precompute:
```cpp
constexpr float inv127 = 1.0f / 127.0f; // MPE CC normalization
constexpr float inv8192 = 1.0f / 8192.0f; // pitch bend normalization
```

### Body Resonator

16 scalar biquads = ~0.07% CPU at 48 kHz. SIMD optimization not worth the complexity. Real CPU budget concern is per-voice oversampled friction+waveguide.

### CPU Budget Estimate

| Component | CPU @ 48kHz (est.) |
|-----------|-------------------|
| 1 voice (core tier, 2x OS) | ~1.5% |
| 1 voice (quality tier, 2x OS) | ~4% |
| 4 drone strings (core, no OS) | ~2% |
| Body resonator (stereo) | ~0.1% |
| 12 sympathetic strings | ~1-2% (with gating) |
| Stereo width | ~0.01% |
| **Max config (4 voice quality + 4 drone + 12 symp + OS)** | **~20%** |

Under the 25% target.

---

## 6. Open Questions Resolved

| Question | Answer |
|----------|--------|
| IIR vs FIR filter? | IIR -- ~1 sample latency vs ~15+ |
| Block vs per-sample oversampling? | Block (must restructure renderNextBlock) |
| MPE zone config? | Migrate to MPESynthesiser (handles zones, bend range, pressure) |
| Single bandpass or HP+LP cascade? | Single bandpass (half CPU, sufficient) |
| TuningEngine thread safety? | `getFrequency()` is lock-free; `setMode()` uses mutex (safe at block rate) |
| Bow noise center fixed or tracking? | Fixed at 3464 Hz (does not track fundamental) |

---

## 7. Implementation Risks

1. **MPESynthesiser migration** is the highest-risk change -- touches voice lifecycle, processor setup, and MIDI routing. Must verify non-MPE keyboards still work via legacy mode.

2. **Block-based oversampling** requires restructuring the per-sample voice loop. BowModel envelope timing must account for 2x rate.

3. **TuningEngine + MPE interaction**: MPE's `getFrequencyInHertz()` assumes 12-TET base. Must manually apply pitch bend on top of tuning engine frequency instead of using the pre-computed value.

4. **bowNoise parameter doesn't exist yet** in APVTS -- must be added to `createParameterLayout()`.
