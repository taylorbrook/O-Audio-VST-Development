# Stage 1: Foundation - Context

## Discussion Summary

**Date:** 2026-02-01
**Participants:** User, Claude
**Stage Goal:** Create project structure with CMakeLists.txt and PluginProcessor skeleton with APVTS parameters

## Requirements Confirmed

### Parameters (5 total)
| ID | Type | Range | Default | Confirmed |
|----|------|-------|---------|-----------|
| FREEZE | Bool | On/Off | Off | Yes |
| THRESHOLD | Float | -60 to 0 dB | -40 dB | Yes |
| MODE | Choice | Manual/Threshold | Manual | Yes |
| DRIFT | Float | 0-100% | 0% | Yes |
| MIX | Float | 0-100% | 100% | **Confirmed: 100% (fully wet)** |

### Buffer Configuration
- **Freeze buffer size:** 2 seconds (confirmed)
- **Memory usage:** ~750KB at 48kHz stereo (acceptable)
- **Rationale:** Industry standard, good balance of texture variety and memory

### Gate Configuration
- **RMS window:** 20ms (confirmed)
- **Hysteresis:** 3dB (per ARCHITECTURE.md)
- **Rationale:** Balanced response - averages transients, responsive to actual endings

### State Persistence
- **FREEZE button:** NOT persisted with presets (confirmed)
- **All other parameters:** Persisted via APVTS
- **Rationale:** Prevents unexpected frozen states on preset load

### Sample Rate Handling
- **Approach:** Auto-scale all timing values (confirmed)
- **Recalculated in prepareToPlay():**
  - Grain size (50ms in samples)
  - Freeze buffer size (2 seconds in samples)
  - RMS window size (20ms in samples)
  - Grain trigger interval
  - Hann window table

## Constraints Identified

1. **No real-time allocation:** All buffers pre-allocated in prepareToPlay()
2. **Atomic parameter access:** All parameter reads use getRawParameterValue()->load()
3. **FREEZE state:** Use std::atomic<bool> for thread-safe audio thread access
4. **Sample rate independence:** All timing must scale with sample rate

## Approach Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Mix default | 100% (wet) | Cleaner for sound design, user can adjust for layering |
| Gate response | 20ms RMS | Balanced - filters transients, responsive to endings |
| Buffer duration | 2 seconds | Industry standard, ~750KB memory (negligible) |
| Freeze persistence | Don't persist | Predictable preset behavior, no surprise frozen states |
| Sample rate | Auto-scale | Professional behavior, works at all rates without issues |

## Open Questions

None - all key decisions resolved.

## Implementation Scope for Stage 1

### Files to Create
1. `CMakeLists.txt` - VST3/AU targets, JUCE module dependencies
2. `Source/PluginProcessor.h` - APVTS declaration, parameter layout
3. `Source/PluginProcessor.cpp` - APVTS initialization, passthrough processBlock
4. `Source/PluginEditor.h` - Empty shell editor
5. `Source/PluginEditor.cpp` - Basic 400x300 window

### APVTS Parameters to Implement
```cpp
// FREEZE - Manual trigger button (AudioParameterBool)
// THRESHOLD - Auto-freeze level (AudioParameterFloat, -60 to 0 dB)
// MODE - Trigger mode selection (AudioParameterChoice, Manual/Threshold)
// DRIFT - Grain randomization (AudioParameterFloat, 0-100%)
// MIX - Dry/Wet blend (AudioParameterFloat, 0-100%)
```

### Build Configuration
- **Plugin code:** OFCR (4 characters for AU)
- **Manufacturer code:** Ouqn
- **Formats:** VST3, AU
- **JUCE modules:** juce_audio_processors, juce_dsp, juce_gui_basics

## Next Phase

Ready for: **RESEARCH** phase
- Research existing plugin patterns (GainKnob, TapeAge) for APVTS setup
- Verify CMakeLists.txt template matches workspace conventions
- Check JUCE 8 parameter patterns in troubleshooting docs
