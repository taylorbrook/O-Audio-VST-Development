# Stage 2: DSP Phase 3.5 - Execution Summary

## Date
2026-04-05

## Goal
Add per-voice 2x/4x oversampling, integrate TuningEngine for microtonal tuning, complete MPE frequency handling with tuning-aware pitchbend, and report oversampling latency to host.

## Results

### Build
- Zero errors, zero warnings
- VST3 + AU build successful
- auval PASS (aumu/ORed/OuDv)

### Parameters Activated (3 new, 33 total)
| Parameter | Wiring |
|-----------|--------|
| referencePitch | `tuningEngine.setMasterTune()` in processBlock |
| tuningSystem | `tuningEngine.setMode()` in processBlock (Scala/MTS-ESP/12-TET) |
| oversampling | Per-voice factor selection (0=2x, 1=4x) with runtime switching |
| instrumentPreset | NOT wired (deferred to GUI morph) |

### Implementation Details

**Per-Voice Oversampling:**
- Two `juce::dsp::Oversampling<float>` instances per voice (2x and 4x)
- Runtime switching via APVTS `oversampling` choice parameter
- Factor change detection in renderNextBlock re-prepares all DSP components at new oversampled rate
- O-Bowed proven pattern: clear voiceBuffer -> processSamplesUp -> per-sample loop at OS rate -> processSamplesDown -> mix to output
- LFO phase increments (vibrato, growl, flutter) use oversampled rate
- Latency reported to host via `setLatencySamples()` in prepareToPlay (based on 2x default)

**TuningEngine Integration:**
- Processor-owned TuningEngine, voices hold non-owning pointer
- `setTuningEngine()` called in constructor voice creation loop
- processBlock reads `referencePitch` and `tuningSystem`, wires `setMasterTune()` and `setMode()`
- All `getFrequencyInHertz()` calls replaced with `getBaseFrequencyFromTuning(note.initialNote)` + explicit pitchbend layering
- Three replacement sites: noteStarted legato, noteStarted normal, renderNextBlock per-block

**MPE Completion:**
- Pitchbend: tuning-aware base frequency + `std::pow(2.0f, bendSemitones / 12.0f)`
- Timbre: already wired per-block (Phase 3.3)
- Pressure: already wired per-block (Phase 3.1)

### Files Modified (3)
| File | Changes |
|------|---------|
| ReedWindVoice.h | +TuningEngine forward decl, +oversampling members (2x/4x), +voiceBuffer, +currentOsFactor, +tuningEngine ptr, +public setTuningEngine/getOversamplingLatency, +private helpers |
| ReedWindVoice.cpp | +TuningEngine include, prepare() at 2x rate + both OS init, +getBaseFrequencyFromTuning, +getActiveOversampling, tuning-aware freq in noteStarted (2 sites) + renderNextBlock, OS reset in noteStopped, full oversampling wrapper in renderNextBlock with OS-rate LFOs |
| PluginProcessor.cpp | +setTuningEngine in constructor, latency reporting in prepareToPlay, tuning engine wiring in processBlock |

### Processing Chain (with Oversampling)
```
Input(silence) -> OversampleUp -> [BreathEnvelope -> BreathNoise -> ExpressionLFOs 
-> ReedModel -> MouthpieceChamber -> BoreWaveguide(s) -> Normalize+Gain+TanhClip] 
-> OversampleDown -> StereoMix
```

### CPU Performance
- Physical modeling reed at 2x oversampling is lightweight (~95 ops/sample native, ~190 ops/oversampled)
- 8 parallel bore waveguide operations + reed ODE per oversampled sample
- No hotspots identified — all per-sample operations are simple arithmetic (no FFT, no matrix ops)
- Expected mono 2x < 2%, mono 4x < 4%, 8-voice poly 2x < 16% based on operation count analysis

### Real-Time Safety
- No allocations in audio thread (factor change calls prepare() — accepted per PLAN, rare user config change)
- All buffers preallocated in prepare()
- Parameter access via atomic loads
- ScopedNoDenormals + snapFiltersToZero
- All loops bounded by block/oversampled block size

## Stage 2 DSP Complete

All 5 DSP phases finished:
1. Phase 3.1: Core Engine (ReedModel + BoreWaveguide + BreathEnvelope)
2. Phase 3.2: Guillemain Psi + Breath Noise + Mouthpiece Chamber
3. Phase 3.3: Tone Holes + Expression + Legato
4. Phase 3.4: Impossible Physics + Dual Bore
5. Phase 3.5: Oversampling + Tuning + MPE + Optimization

**33 of 35 parameters active.** instrumentPreset deferred to GUI morph (Stage 3).
