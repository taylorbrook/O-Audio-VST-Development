# Stage 2: DSP - Context

## Discussion Summary

**Date:** 2026-02-01
**Participants:** User, Claude
**Stage Goal:** Implement granular freeze engine with threshold gate and dry/wet mixing

## Requirements Confirmed

### Implementation Approach

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Build approach | **Phased** | Buffer loop first → validate mechanics → upgrade to granular |
| Grain count | **8 grains** | 87.5% overlap, ultra-smooth texture (user preference over 4-grain default) |
| Dry/Wet mixing | **juce::dsp::DryWetMixer** | JUCE built-in, latency-compensated |

### DSP Components (from ARCHITECTURE.md)

1. **Circular Freeze Buffer**
   - 2-second capacity (samples = 2 * sampleRate * numChannels)
   - Continuous write (pre-freeze), locked write (during freeze)
   - Pre-allocated in prepareToPlay()

2. **Granular Engine**
   - Overlap-add synthesis with Hann windowing
   - 8 simultaneous grains (87.5% overlap)
   - Grain size: 200ms (scales with sample rate)
   - Grain trigger interval: grain_size / 8 = 25ms
   - Pre-computed Hann window table

3. **Threshold Gate**
   - RMS level detection (20ms window)
   - 3dB hysteresis (engage vs release threshold)
   - State machine: IDLE → FROZEN → IDLE

4. **Drift Modulation**
   - Random grain start position offset
   - Range: 0% (static) to 100% (full buffer range)
   - Per-grain randomization using juce::Random

5. **Crossfade System**
   - juce::LinearSmoothedValue for freeze_gain
   - 50ms fade-in on freeze engage
   - 100ms fade-out on freeze release

6. **Dry/Wet Mixer**
   - juce::dsp::DryWetMixer
   - MIX parameter 0-100%

### Phased Implementation Plan

**Phase A: Simple Buffer Loop**
- Circular buffer write/read
- FREEZE button locks buffer
- Simple buffer loop playback (no grains yet)
- Crossfade engage/disengage
- DryWetMixer integration
- **Validates:** Buffer mechanics, freeze trigger, crossfade, mixing

**Phase B: Threshold Gate**
- RMS level detection
- State machine with hysteresis
- MODE parameter switches trigger source
- **Validates:** Automatic freeze triggering

**Phase C: Granular Engine**
- Replace buffer loop with granular synthesis
- 8-grain overlap-add
- Hann windowing
- DRIFT parameter integration
- **Validates:** Smooth freeze texture, no artifacts

## Constraints Identified

1. **No real-time allocation:** All buffers pre-allocated in prepareToPlay()
2. **Thread safety:** Atomic parameter reads, std::atomic<bool> for freeze state
3. **Sample rate independence:** All timing values recalculated on rate change
4. **Denormal protection:** ScopedNoDenormals in processBlock()

## Parameters Used

| ID | DSP Component | Usage |
|----|---------------|-------|
| FREEZE | Freeze Trigger | Manual freeze button (MODE=Manual) |
| THRESHOLD | Threshold Gate | Auto-freeze level (MODE=Threshold) |
| MODE | Trigger Selection | Manual vs Threshold |
| DRIFT | Granular Engine | Grain position randomization |
| MIX | DryWetMixer | Dry/wet blend |

## Open Questions

None - all key decisions resolved.

## Next Phase

Ready for: **RESEARCH** phase (optional) or **PLAN** phase
- Research existing granular implementations in codebase (if any)
- Create detailed execution plan with file changes
