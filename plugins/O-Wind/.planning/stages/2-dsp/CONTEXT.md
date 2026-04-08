# Stage 2: DSP - Context

## Discussion Summary

**Date:** 2026-04-05
**Participants:** User, Claude

## Scope

**All 4 DSP phases included (3.1-3.4):**
- Phase 3.1: Minimal oscillating model (jet + bore + feedback, monophonic)
- Phase 3.2: Expression + oversampling (noise, vibrato, 2x OS, polyphony, CC mapping)
- Phase 3.3: Impossible physics + instrument presets (infinite sustain, reversed jet, sub-harmonics, 4 core presets)
- Phase 3.4: Advanced features (Tier 2 tone holes, expansion presets, full MPE, tuning integration)

User chose to include Phase 3.4 (Tier 2 Keefe tone holes + expansion presets) in this stage rather than deferring to a future milestone.

## Requirements Confirmed

- FUNC-01 through FUNC-14 all targeted for this stage
- DSP-01 through DSP-05 all targeted for this stage
- PERF-01, PERF-02, PERF-03 targeted for this stage
- QUAL-01, QUAL-02 targeted for this stage
- COMPAT-02 (MIDI/MPE support) targeted for this stage
- FUNC-12 (Tier 2 tone holes) now IN SCOPE (was marked nice-to-have, user confirmed include)

## Constraints Identified

- Must build on Stage 1 foundation (PluginProcessor with 16 APVTS params, Synthesiser, TuningEngine)
- O-Bowed DSP patterns are the reference (split DSP components, voice architecture)
- JUCE 8.0.4 APIs only
- No allocations on audio thread
- CPU budget: <2.5% per voice simple, <3.5% per voice with Tier 2 tone holes

## Approach Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Phase 3.4 scope | Include all (Tier 2 + expansion presets) | User decision — ship complete feature set |
| Voice architecture | `juce::Synthesiser` + `juce::SynthesiserVoice` with manual CC routing | CC2 (breath) is primary wind input, not an MPE dimension. Manual per-channel routing for aftertouch/slide/pitch bend proven in O-Bowed. Same quality, less risk |
| Oversampling | Per-voice `juce::dsp::Oversampling<float>` instances | Clean feedback loop isolation per voice, trivial memory cost (8 voices max) |
| File structure | Split DSP components (like O-Bowed) | `DSP/JetExciter.h`, `DSP/BoreWaveguide.h`, `DSP/ToneHoleSystem.h`, etc. + `FluteSynthVoice.h/.cpp` |
| Phase 3.1 priority | Get it oscillating and pitch-tracking first | Rough flute sound acceptable — refine in subsequent phases |

## File Structure Plan

```
Source/
  PluginProcessor.h/cpp      (existing — add voice setup, prepareToPlay wiring)
  PluginEditor.h/cpp          (existing — no changes in Stage 2)
  FluteSynthSound.h           (existing — trivial sound class)
  FluteSynthVoice.h/cpp       (NEW — SynthesiserVoice subclass, orchestrates DSP)
  DSP/
    JetExciter.h              (NEW — breath model, Bernoulli, turbulence noise, vibrato LFO)
    JetNonlinearity.h         (NEW — tanh saturation, reversed jet blend)
    BoreWaveguide.h           (NEW — bidirectional Thiran delay lines, bore loss, end reflection, radiation)
    DCBlocker.h               (NEW — inline DC blocking filter)
    ToneHoleSystem.h          (NEW — Tier 1 bore-length switching + Tier 2 Keefe scattering)
    SubHarmonics.h            (NEW — nonlinear feedback sub-octave generator)
    StereoWidth.h             (NEW — mid-side decorrelation, shared pattern with O-Bowed)
    InstrumentPresets.h       (NEW — parameter sets for Concert Flute, Shakuhachi, Bansuri, NAF, expansions)
```

## Architecture Reference

All DSP components, signal flow, and JUCE class mappings defined in:
- `plugins/O-Wind/.planning/research/ARCHITECTURE.md` (immutable contract)

Key signal chain (per voice):
```
Breath -> Bernoulli -> + Noise -> Embouchure Sum (<- bore feedback)
  -> Jet Delay (Lagrange3rd) -> tanh Nonlinearity -> DC Blocker
  -> Bore Waveguide (Thiran, bidirectional) -> Tone Holes
  -> Bore Loss Filter -> End Reflection -> feedback loop
  -> Radiation Filter -> voice output
```

Post-voice-summation: Output Level -> Stereo Width -> final output

## Phase Execution Order

1. **Phase 3.1** — Minimal oscillating model (monophonic, no oversampling, 6 params active)
2. **Phase 3.2** — Expression + oversampling (noise, vibrato, 2x OS, 4-voice polyphony, CC mapping)
3. **Phase 3.3** — Impossible physics + 4 core instrument presets + stereo width
4. **Phase 3.4** — Tier 2 tone holes (Keefe scattering), expansion presets (Recorder, Pan Flute, Piccolo, Ocarina), full MPE, tuning integration

## Open Questions

- Tier 2 tone hole stability during rapid transitions — research during Phase 3.4 planning
- Expansion preset parameter tuning (Recorder, Pan Flute, Piccolo, Ocarina) — requires experimentation
- Native American Flute dual-chamber modeling — may need simplified approach (short bore stub)

## Reference Plugins

- **O-Bowed** — split DSP pattern (`DSP/WaveguideString.h`, `DSP/BowModel.h`, `BowedStringVoice.h`)
- **O-Formant** — standard Synthesiser with manual CC74/Aftertouch routing
- **O-Lyrica** — SynthesiserVoice pattern with TuningEngine pointer

## Next Phase

Ready for: **research** phase (Phase 3.1 — minimal oscillating model)
