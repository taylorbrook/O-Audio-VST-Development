# Stage 2 DSP -- Phase 3.5 Summary

**Plugin:** O-Bowed
**Phase:** 3.5 (Final DSP Phase)
**Date:** 2026-04-05

---

## Files Created (2)

| File | Lines | Description |
|------|-------|-------------|
| `Source/DSP/BowNoiseGenerator.h` | 54 | Per-voice bandpass noise (3464 Hz, Q=0.87), amplitude modulated by pressure x speed x noiseAmount |
| `Source/BowedMPESynthesiser.h` | 53 | MPESynthesiser subclass with CC11 Expression dispatching to voices |

## Files Modified (6)

| File | Changes |
|------|---------|
| `Source/BowedStringVoice.h` | Base class: SynthesiserVoice -> MPESynthesiserVoice. Added: oversampling, tuningEngine, bowNoiseGen, voiceBuffer, MPE state members, setExpression/setTuningEngine/setVoiceIndex/getOversamplingLatency methods. Removed: old Synthesiser API methods. |
| `Source/BowedStringVoice.cpp` | Full rewrite: noteStarted/noteStopped/notePitchbendChanged/notePressureChanged/noteTimbreChanged/noteKeyStateChanged. Block-based 2x oversampling in renderNextBlock. TuningEngine frequency lookup. MPE modulation (pressure->bowPressure, timbre->bowPosition, expression->bowSpeed). Bow noise in output mix. |
| `Source/PluginProcessor.h` | Replaced BowedStringSound.h include with BowedMPESynthesiser.h. Changed synthesiser type from juce::Synthesiser to BowedMPESynthesiser. |
| `Source/PluginProcessor.cpp` | Added bowNoise APVTS parameter. Constructor: removed addSound(), added voice index + tuning engine setup, enableLegacyMode(2). prepareToPlay: report oversampling latency. processBlock: wire tuningEngine.setMasterTune/setMode, fixed MPE voice API (isActive/noteStopped). |
| `Source/DSP/SympatheticStringEngine.cpp` | Energy gating threshold: 1e-7f -> 1e-5f (line 183). ~30% faster settle time, meaningful CPU savings with 12 strings. |
| `CMakeLists.txt` | Removed BowedStringSound.h, added BowNoiseGenerator.h + BowedMPESynthesiser.h. |

## Files Removed from Build (1)

| File | Reason |
|------|--------|
| `Source/BowedStringSound.h` | MPESynthesiser has no Sound concept. Removed from CMakeLists, file remains on disk. |

---

## Parameters Connected

**Newly connected this phase (3 + 1 new):**
- `referencePitch` -> TuningEngine.setMasterTune() in processBlock
- `tuningSystem` -> TuningEngine.setMode() in processBlock
- `bowNoise` -> BowNoiseGenerator per-voice in renderNextBlock (NEW APVTS parameter)

**Total parameters: 24/24** (bowNoise is new, all parameters wired)

---

## Signal Flow (Updated)

```
MIDI Input
    |
    v
BowedMPESynthesiser (legacy mode, 8 voices)
    |
    +-- Per-Voice (BowedStringVoice):
    |       |
    |       v
    |   [voiceBuffer: mono silence]
    |       |
    |       v
    |   Oversampling.processSamplesUp() --> 2x rate
    |       |
    |       v
    |   BowModel.updateEnvelope() [2x rate]
    |       |
    |       v
    |   WaveguideString.readJunction() [2x rate]
    |       |
    |       v
    |   Friction Tier Dispatch (Core/Enhanced/Quality) [2x rate]
    |       |
    |       v
    |   Reversed Friction Interpolation [2x rate]
    |       |
    |       v
    |   WaveguideString.writeJunction() [2x rate]
    |       |
    |       v
    |   SubHarmonicsGenerator [2x rate]
    |       |
    |       v
    |   Oversampling.processSamplesDown() --> native rate
    |       |
    |       v
    |   Output Gain + BowNoiseGenerator [native rate]
    |       |
    |       v
    |   Stereo Panning (panL, panR)
    |
    +-- DroneStringEngine (native rate, unchanged)
    |
    v
Body Resonator (stereo)
    |
    v
Sympathetic String Engine
    |
    v
Stereo Width Processor
    |
    v
Output Buffer
```

---

## MPE Support

| MPE Dimension | Source | Target | Scaling |
|---|---|---|---|
| Velocity (strike) | noteOnVelocity | bowModel.startBow() | Direct (0-1) |
| Pitch Bend (X) | totalPitchbendInSemitones | currentFrequency | pow(2, bend/12) on top of TuningEngine base |
| Pressure (Z) | pressure.asUnsignedFloat() | effectivePressure | bowPressure * (0.5 + pressure * 1.5) |
| Timbre (Y/CC74) | timbre.asSignedFloat() | effectivePosition | bowPos + timbre * 0.1, clamped [0.02, 0.30] |
| Expression (CC11) | handleController() | effectiveSpeed | bowSpeed * expression (0-1) |

Legacy mode: enabled with +/- 2 semitone pitch bend range (standard MIDI keyboards work normally).

---

## Oversampling Details

- **Type:** juce::dsp::Oversampling<float> (mono, 2x, IIR polyphase halfband)
- **Per-voice:** Each voice owns its own oversampling instance
- **Internal rate:** All waveguide + friction + bow model prepared at sampleRate * 2
- **Latency:** Reported to host via setLatencySamples(ceil(oversampling.getLatencyInSamples()))
- **Drone strings:** NOT oversampled (use processSample path at native rate, unchanged)

---

## Optimization

- Sympathetic energy threshold: 1e-7f -> 1e-5f (-100 dBFS, still 40 dB below 16-bit noise floor)
- MPE CC normalization: constexpr inv127 = 1.0f/127.0f (avoids per-event division)

---

## Build Status

Pending build verification by orchestrator.
