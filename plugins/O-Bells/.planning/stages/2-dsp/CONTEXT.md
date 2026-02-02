# Stage 2: DSP Implementation - Context

**Phase:** Discuss
**Date:** 2026-02-01
**Status:** Complete

## Decisions Made

### Implementation Scope
- **Decision:** Complete all 3 DSP phases before Stage 3 (GUI)
- **Phases:**
  - Phase 2.1: Core modal synthesis (8 partials, single voice)
  - Phase 2.2: Polyphony (8 voices) + strike dynamics
  - Phase 2.3: Ensemble voicing + advanced features (including sympathetic resonance)

### Partial Count
- **Decision:** Start with full 8 partials from the beginning
- **Rationale:** Architecture is well-defined, no need to iterate. 8 partials is the target per ARCHITECTURE.md.

### Advanced Features
- **Decision:** Include sympathetic resonance in Phase 2.3
- **Rationale:** User preference to have complete DSP before GUI phase

## DSP Requirements (from ARCHITECTURE.md)

### Phase 2.1: Core Modal Synthesis
- BellVoice class (juce::SynthesiserVoice subclass)
- 8 sine oscillators per voice (modal partials)
- Church bell partial ratios: 0.5x, 1.0x, 2.4x, 3.0x, 4.0x, 5.2x, 6.0x, 8.0x
- INHARMONICITY parameter interpolates between harmonic/bell/gamelan ratios
- Exponential decay envelopes per partial (higher = faster decay)
- Parameters: bellSize, damping, brightness, material, inharmonicity

### Phase 2.2: Polyphony + Strike Dynamics
- juce::Synthesiser with 8 BellVoice instances
- Voice stealing: oldest-first with 5ms fade-out
- Strike dynamics processor:
  - MALLET_HARDNESS → spectral tilt (soft=dark, hard=bright)
  - STRIKE_POSITION → comb filter on partials
  - Velocity → amplitude + brightness scaling
- Strike transient: noise burst filtered by STRIKE_NOISE_CHARACTER (Click/Thud/Ping)
- Parameters: strikePosition, malletHardness, strikeNoiseChar, velocityCurve

### Phase 2.3: Ensemble + Advanced
- Unison layering: 1-4 detuned copies per voice
- Detune distribution: symmetric spread (±cents)
- Octave layers: sub (-12), fundamental, octave (+12)
- Stereo spread for ensemble panning
- Sympathetic resonance: cross-voice coupling for harmonically related notes
- Parameters: unisonCount, unisonDetune, octaveBlendSub, octaveBlendOct, stereoSpread, sympatheticResonance

## Key Algorithms

### Partial Frequency (from ARCHITECTURE.md)
```cpp
// Interpolate ratios based on inharmonicity (0-50%: harmonic→bell, 50-100%: bell→gamelan)
const float harmonicRatios[] = {0.5, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0};
const float bellRatios[] = {0.5, 1.0, 2.4, 3.0, 4.0, 5.2, 6.0, 8.0};
const float gamelanRatios[] = {0.5, 1.0, 2.1, 3.5, 4.8, 5.8, 7.2, 9.5};
```

### Strike Position (comb filter)
```cpp
float strikePositionGain(int partialIndex, float position) {
    float phase = juce::MathConstants<float>::pi * position * (partialIndex + 1);
    return std::abs(std::sin(phase));
}
```

### Unison Detune Distribution
```cpp
// Symmetric spread: e.g., 4 voices at 50 cents → [-50, -16.7, +16.7, +50]
```

## Files to Create

- `Source/BellVoice.h` - Voice class header
- `Source/BellVoice.cpp` - Voice implementation
- Modify `Source/PluginProcessor.cpp` - Add Synthesiser setup

## CPU Target

- <60% single core worst case
- 8 voices × 4 unison × 8 partials × 3 octaves = 768 oscillators max
- Mitigations: SIMD (juce::dsp::Oscillator), quality settings, disable unused layers

## Test Criteria

After Stage 2:
- Plugin produces bell sounds from MIDI input
- All main panel parameters affect timbre
- Ensemble creates chorus/width effect
- Voice stealing works without clicks

---

*Generated during discuss phase*
