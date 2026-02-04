# Stage 1: Foundation - Context

## Discussion Summary

**Date:** 2026-02-03
**Participants:** User, Claude

## Requirements Confirmed

### Parameters (7 Total - Updated from 6)

| Parameter | Type | Range | Default | Skew | Notes |
|-----------|------|-------|---------|------|-------|
| MIX | Float | 0.0-1.0 | 1.0 | Linear | Wet/dry blend |
| ATTACK_TIME | Float | 0.1-50.0ms | 10.0ms | 0.3 (Log) | Global attack speed |
| SUSTAIN_TIME | Float | 10.0-500.0ms | 100.0ms | 0.3 (Log) | Global sustain release |
| SENSITIVITY | Float | 0.0-1.0 | 0.5 | Linear | Detection threshold |
| LOOKAHEAD_ENABLED | Bool | on/off | **off** | N/A | **New: toggle for lookahead** |
| LOOKAHEAD_TIME | Float | 0.1-10.0ms | 2.0ms | Linear | Only active when enabled |
| OUTPUT_GAIN | Float | -12.0-12.0dB | 0.0dB | Linear | Level compensation |

**Key Change:** Lookahead is now a separate toggle (default OFF) plus time parameter, instead of a single parameter.

### Sample Rate Support

- Standard range: 44.1kHz to 192kHz
- FFT size adapts automatically for consistent frequency resolution
- No special optimizations needed for specific rates

### Latency Reporting

- **Fixed 512 samples** regardless of lookahead toggle state
- Rationale: FFT processing requires the buffer anyway; dynamic latency causes DAW issues
- ~11.6ms @ 44.1kHz, ~10.7ms @ 48kHz

## Constraints Identified

- **C1:** JUCE 8 requires `ParameterID{"NAME", 1}` format (version 1)
- **C2:** WebView requires `NEEDS_WEB_BROWSER TRUE` in juce_add_plugin
- **C3:** Must include `juce_dsp` module for FFT (even though DSP implemented in Stage 2)
- **C4:** Curve data (64 floats) stored in state, NOT as APVTS parameters

## Approach Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Lookahead control | Separate toggle + time | User preference; clearer UI intent |
| Latency reporting | Fixed 512 samples | Best practice; avoids DAW compensation issues |
| Parameter version | 1 | JUCE 8 standard |
| APVTS param count | 7 | Added LOOKAHEAD_ENABLED bool |
| WebView enabled | Yes | Required for Stage 3 GUI |

## JUCE Modules Required

```cmake
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_dsp           # For FFT in Stage 2
        juce::juce_gui_extra     # For WebView in Stage 3
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)
```

## CMake Configuration

```cmake
juce_add_plugin(${PROJECT_NAME}
    COMPANY_NAME "Ouaricon"
    PLUGIN_MANUFACTURER_CODE Ouar
    PLUGIN_CODE OSpS
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "O-SpectralShaper"
    NEEDS_WEB_BROWSER TRUE      # Required for WebView
)
```

## Updated Parameter Spec

The `parameter-spec.md` needs updating to reflect the new LOOKAHEAD_ENABLED bool parameter:

```cpp
// Bool parameter for lookahead toggle
layout.add(std::make_unique<AudioParameterBool>(
    ParameterID{"LOOKAHEAD_ENABLED", 1}, "Lookahead Enabled",
    false  // Default OFF
));
```

## Open Questions

None - all requirements clarified.

## Next Phase

Ready for: **research** phase (brief - verify JUCE 8 patterns) then **plan** phase

## Implementation Checklist (Stage 1)

- [ ] CMakeLists.txt with correct modules and WebView flag
- [ ] PluginProcessor.h/cpp with APVTS (7 parameters)
- [ ] PluginEditor.h/cpp (placeholder WebView)
- [ ] `getLatencyInSamples()` returns 512
- [ ] Build verification (VST3 + AU)
- [ ] Load in DAW verification
- [ ] Parameter automation test
