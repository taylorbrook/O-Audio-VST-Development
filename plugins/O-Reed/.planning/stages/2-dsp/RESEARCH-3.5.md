# Stage 2: DSP Phase 3.5 - Research

## Research Date

2026-04-05

## Scope

Oversampling (per-voice), TuningEngine integration, MPE completion, optimization profiling.

---

## 1. Per-Voice Oversampling

### O-Bowed Reference Pattern (proven)

**Declaration** — one `juce::dsp::Oversampling<float>` per voice + mono voiceBuffer:
```cpp
// BowedStringVoice.h:85-87
juce::dsp::Oversampling<float> oversampling { 1, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR };
juce::AudioBuffer<float> voiceBuffer;
```
- `numChannels=1` (mono per-voice)
- `factor=1` means 2^1 = 2x oversampling
- `filterHalfBandPolyphaseIIR` — efficient anti-aliasing

**Prepare** — DSP components at oversampled rate, oversampling at native rate:
```cpp
// BowedStringVoice.cpp:94-113
waveguideString.prepare(sampleRate * 2.0, maxBlockSize * 2);
bowModel.prepare(sampleRate * 2.0);
oversampling.initProcessing(static_cast<size_t>(maxBlockSize));  // native block size
voiceBuffer.setSize(1, maxBlockSize);
```

**renderNextBlock wrapper:**
1. Clear voiceBuffer, create AudioBlock subblock of `numSamples`
2. `processSamplesUp(block)` → returns oversampled AudioBlock
3. Per-sample loop at 2x rate writing to `oversampledData`
4. `processSamplesDown(block)` → downsampled audio back in voiceBuffer
5. Post-downsample: mix from voiceBuffer into outputBuffer

**Latency reporting** in processor's `prepareToPlay()`:
```cpp
if (auto* voice = dynamic_cast<BowedStringVoice*>(synthesiser.getVoice(0)))
    setLatencySamples(static_cast<int>(std::ceil(voice->getOversamplingLatency())));
```

### Adaptation for O-Reed

**Key difference:** O-Reed needs runtime 2x/4x switching via APVTS `oversampling` choice param.

**Approach:** Two `Oversampling` instances per voice (one 2x, one 4x), select active one based on param. Avoids re-init in audio thread.

```cpp
// Voice members:
juce::dsp::Oversampling<float> oversampling2x { 1, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR };
juce::dsp::Oversampling<float> oversampling4x { 1, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR };
juce::AudioBuffer<float> voiceBuffer;
int currentOsFactor = 1;  // 1=2x, 2=4x (matches Oversampling factor param)
```

**Prepare:** Init both at native block size. Prepare all DSP at 4x rate (highest possible) — wasteful for 2x but safe. Alternative: prepare at max rate, inner loop uses actual OS rate. O-Bowed uses fixed 2x so this wasn't needed there.

**Better approach:** Prepare DSP at actual oversampled rate. On factor change (detected per-block), re-prepare DSP components — this only happens on user parameter change, not per-note.

**Factor change detection in renderNextBlock:**
```cpp
int osChoice = static_cast<int>(pOversampling->load());  // 0=2x, 1=4x
int osFactor = osChoice + 1;  // 1=2x, 2=4x
if (osFactor != currentOsFactor) {
    currentOsFactor = osFactor;
    double osRate = sr * std::pow(2.0, osFactor);
    // Re-prepare DSP components at new oversampled rate
    reedModel.prepare(osRate);
    bore.prepare(osRate, numSamples * (1 << osFactor));
    bore2.prepare(osRate, numSamples * (1 << osFactor));
    // Reset oversampling state
    getActiveOversampling().reset();
}
```

**Helper:**
```cpp
juce::dsp::Oversampling<float>& getActiveOversampling() {
    return (currentOsFactor == 2) ? oversampling4x : oversampling2x;
}
```

**Latency:** Changes with oversampling factor. Report in `prepareToPlay()` for default (2x). Update when factor changes would require host notification — but hosts typically don't re-query latency mid-playback. Report worst-case (4x) or default (2x) — O-Bowed reports fixed 2x. We'll report based on current setting, re-set in processBlock if changed.

**Decision:** Report latency in prepareToPlay based on initial 2x. If user switches to 4x, the latency difference (~8 samples) is inaudible in practice. This matches O-Bowed simplicity.

### What Gets Oversampled

The entire per-sample DSP chain (reed+junction+bore) runs at oversampled rate:
- BreathEnvelope → ReedModel → MouthpieceChamber → BoreWaveguide(s)
- Expression LFOs (vibrato, growl, flutter) — their rates need `sr` to be oversampled rate
- BreathNoise — runs at oversampled rate

**Not oversampled:** Output gain, safety clip, addSample to outputBuffer — these run at native rate after downsample.

---

## 2. TuningEngine Integration

### O-Bowed Reference Pattern (proven)

**Processor owns TuningEngine, voices hold non-owning pointer:**
```cpp
// Processor header:
TuningEngine tuningEngine;

// Voice header:
TuningEngine* tuningEngine = nullptr;
void setTuningEngine(TuningEngine* engine) noexcept { tuningEngine = engine; }
```

**Processor constructor** injects pointer:
```cpp
voice->setTuningEngine(&tuningEngine);
```

**Processor processBlock** updates tuning per-block:
```cpp
tuningEngine.setMasterTune(static_cast<double>(refPitch));
int tuningSystemIdx = static_cast<int>(parameters.getRawParameterValue("tuningSystem")->load());
switch (tuningSystemIdx) {
    case 0:  tuningEngine.setMode(TuningEngine::Mode::Scala); break;
    case 1:  tuningEngine.setMode(TuningEngine::Mode::MTSESP); break;
    default: tuningEngine.setMode(TuningEngine::Mode::TwelveTET); break;
}
```

**Voice frequency lookup** replaces `getFrequencyInHertz()`:
```cpp
float getBaseFrequencyFromTuning(int midiNote) const {
    if (tuningEngine != nullptr)
        return static_cast<float>(tuningEngine->getFrequency(midiNote));
    return static_cast<float>(juce::MidiMessage::getMidiNoteInHertz(midiNote));
}
```

**Usage in noteStarted/pitchbend:**
```cpp
float freq = getBaseFrequencyFromTuning(midiNote);
float bendSemitones = static_cast<float>(note.totalPitchbendInSemitones);
if (std::abs(bendSemitones) > 0.001f)
    freq *= std::pow(2.0f, bendSemitones / 12.0f);
```

### Adaptation for O-Reed

**Straightforward port.** Changes needed:

1. **PluginProcessor.h** — already has `TuningEngine tuningEngine` member
2. **ReedWindVoice.h** — add `TuningEngine* tuningEngine = nullptr` + setter
3. **PluginProcessor constructor** — add `voice->setTuningEngine(&tuningEngine)` to voice creation loop
4. **PluginProcessor::processBlock** — add tuning engine wiring before `synthesiser.renderNextBlock()`
5. **ReedWindVoice** — add `getBaseFrequencyFromTuning()` helper
6. **Replace ALL `getFrequencyInHertz()` calls:**
   - `noteStarted()` line 107 (legato path)
   - `noteStarted()` line 163 (normal onset)
   - `renderNextBlock()` line 327 (per-block frequency update)

**Thread safety:** `tuningEngine.setMasterTune()` and `setMode()` called on audio thread in processBlock before voices render — same thread, no contention. `getFrequency()` called from voices on same audio thread. Safe.

**API:** `TuningEngine::getFrequency(int midiNote, int midiChannel = 0)` — returns `double`. Cast to `float` for voice use.

---

## 3. MPE Completion

### Current State

| Handler | Status | Implementation |
|---------|--------|----------------|
| `notePressureChanged()` | Done (Phase 3.1) | MPE pressure → breath target |
| `notePitchbendChanged()` | Stub (empty) | Per-block via `getFrequencyInHertz()` |
| `noteTimbreChanged()` | Stub (comment only) | Per-block via `note.timbre` |

**Per-block MPE reads (renderNextBlock lines 304-311):**
- `note.timbre.asUnsignedFloat()` → embouchure override (already implemented)
- `note.pressure.asUnsignedFloat()` → breath override (already implemented)

### Changes Needed

1. **Pitchbend** — currently uses `note.getFrequencyInHertz()` which already incorporates MPE pitchbend. When switching to TuningEngine, need to explicitly layer pitchbend:
   ```cpp
   float freq = getBaseFrequencyFromTuning(note.initialNote);
   float bendSemitones = static_cast<float>(note.totalPitchbendInSemitones);
   if (std::abs(bendSemitones) > 0.001f)
       freq *= std::pow(2.0f, bendSemitones / 12.0f);
   ```
   This applies everywhere `getFrequencyInHertz()` was used.

2. **Timbre** — already wired per-block at line 310-311. No change needed.

3. **Pressure** — already wired per-block at line 314. No change needed.

### O-Bowed Timbre Pattern

O-Bowed reads timbre per-block as `note.timbre.asSignedFloat()` (-1 to +1) and adds to bow position.

O-Reed already reads timbre per-block at line 306 as `note.timbre.asUnsignedFloat()` (0 to 1) and uses it to override embouchure. This is functionally complete. The CONTEXT-3.5.md says "wire timbre->embouchure blend (same pattern as pressure -> breath)" — this is already done.

**Conclusion:** MPE is already mostly wired. The only real change is replacing `getFrequencyInHertz()` with tuning-engine-aware frequency + explicit pitchbend layering.

---

## 4. Optimization

### CONTEXT-3.5 Decision

"Measurement + targeted fixes only. Profile CPU per voice, document hotspots, fix obvious issues. No speculative SIMD or lookup table rewrites."

### Profiling Approach

Use `std::chrono::high_resolution_clock` around the voice render loop to measure per-voice cost:
```cpp
auto start = std::chrono::high_resolution_clock::now();
// ... voice render ...
auto end = std::chrono::high_resolution_clock::now();
float voiceCostUs = std::chrono::duration<float, std::micro>(end - start).count();
```

Measure at 2x and 4x for mono and 8-voice poly. Report in SUMMARY.

### Known Hotspots (from ARCHITECTURE.md)

- **Reed ODE** — 2 adds, 3 muls per sample (symplectic Euler) — negligible
- **Bore waveguide** — Thiran allpass (6 ops × 2 lines) + viscothermal IIR (5 ops) + bell IIR (5 ops) + tone holes (4 × ~10 ops) = ~62 ops/sample — moderate
- **Breath noise** — IIR filter + random + scale — ~15 ops/sample — light
- **Mouthpiece chamber** — 1 IIR step — ~5 ops/sample — negligible
- **Expression LFOs** — sin() calls (3 possible) — ~10 ops/sample — light

**Total estimate at native rate:** ~95 ops/sample/voice
**At 2x:** ~190 ops/output sample/voice
**At 4x:** ~380 ops/output sample/voice

**8-voice poly at 2x, 48kHz:** 190 × 8 × 48000 = 73M ops/sec — well within budget
**Mono 4x at 48kHz:** 380 × 48000 = 18.2M ops/sec — trivial

### Likely Non-Issues

- `std::pow(2.0f, cents/1200.0f)` — called once per block, not per sample
- `std::sin()` for LFOs — 3 calls per sample max, modern CPUs handle this in <10ns
- `std::tanh()` — 1 call per sample for safety clip — ~5ns

### Potential Optimizations (if profiling reveals need)

1. **BoreWaveguide tone hole scattering** — 4 three-port junctions at ~10 ops each = 40 ops. Could unroll or SIMD but unlikely bottleneck.
2. **Dual bore mode** — doubles bore processing. Already gated by `if (dualBoreActive)` so no cost when off.
3. **Denormal cost** — `ScopedNoDenormals` already in place. Bore `snapFiltersToZero()` prevents denormal tail.

**Decision:** Measure first, then fix only if any voice exceeds 2% CPU mono at 2x.

---

## 5. Implementation Sequence

1. Add `voiceBuffer` + two `Oversampling` instances to voice, `prepare()` init
2. Add `TuningEngine*` to voice, setter, `getBaseFrequencyFromTuning()` helper
3. Wire TuningEngine in processor constructor + processBlock
4. Replace all `getFrequencyInHertz()` with tuning-aware frequency + pitchbend layering
5. Wrap renderNextBlock inner loop with oversampling (up → process at OS rate → down)
6. Update `sr` usage in LFOs to use oversampled rate inside the loop
7. Report latency in prepareToPlay
8. Add CPU profiling measurement, document results
9. Build, test, verify regression at 2x with 12-TET (should sound identical to Phase 3.4)

---

## 6. Files Modified

| File | Changes |
|------|---------|
| `ReedWindVoice.h` | Add Oversampling members, voiceBuffer, TuningEngine pointer, helper method |
| `ReedWindVoice.cpp` | Oversampling wrapper in renderNextBlock, tuning-aware frequency, prepare changes |
| `PluginProcessor.h` | No changes needed (already has TuningEngine) |
| `PluginProcessor.cpp` | Wire TuningEngine in constructor + processBlock, latency reporting |

No new files needed. No DSP class changes — oversampling wraps the existing chain transparently.

---

## 7. Risk Assessment

| Risk | Likelihood | Mitigation |
|------|-----------|------------|
| Oversampling adds audible latency | Low | ~8-16 samples, report to host |
| Factor switch causes click | Medium | Reset oversampling state + re-prepare DSP on change |
| TuningEngine nullptr in voice | Low | Null check in helper with 12-TET fallback |
| CPU increase from 4x | Low | Reed instruments are low-poly, 4x still well within budget |
| Regression at 2x/12-TET | Low | No algorithmic change, just wrapper around existing chain |

---

## References

- O-Bowed oversampling: `plugins/O-Bowed/Source/BowedStringVoice.h:85-87`, `.cpp:94-200`
- O-Bowed tuning: `plugins/O-Bowed/Source/PluginProcessor.cpp:237-314`, `BowedStringVoice.cpp:25-73,261-267`
- TuningEngine API: `modules/tuning/scala-tuning-engine/cpp/TuningEngine.h:243`
- JUCE Oversampling: `juce::dsp::Oversampling<float>` — `initProcessing()`, `processSamplesUp()`, `processSamplesDown()`, `getLatencyInSamples()`, `reset()`
