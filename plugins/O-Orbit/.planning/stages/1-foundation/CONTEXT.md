# Stage 1: Foundation - Context

## Discussion Summary

**Date:** 2026-02-09
**Participants:** User, Claude

## Requirements Confirmed

- Integrate SAF (Spatial Audio Framework) as a git submodule in Stage 1 (not deferred to Stage 2.2)
- SAF submodule location: `libs/SAF/` within the O-Orbit plugin directory
- SPEAKER_LAYOUT (P10) as APVTS AudioParameterChoice for preset save/recall (non-automatable but stored in APVTS)
- Default output bus: stereo (safe for maximum DAW compatibility)
- Users select larger layouts explicitly; auto-downmix handles fallback
- Subfolder source organization: Source/DSP/, Source/Data/, Source/ (processor + editor)

## Constraints Identified

- SAF requires BLAS/LAPACK: Apple Accelerate on macOS (auto-detected), Intel MKL or OpenBLAS on Windows
- Git submodule requires `git submodule update --init` after clone
- Multi-channel bus negotiation via `isBusesLayoutSupported()`, NOT `PLUGIN_CHANNEL_CONFIGURATIONS`
- JUCE 8.0.4 patterns: `juce_generate_juce_header()` after `target_link_libraries()`
- WebView: `NEEDS_WEB_BROWSER TRUE` + `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`

## Approach Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| SAF integration timing | Stage 1 (now) | Verify build chain early; catch integration issues before DSP work |
| SAF integration method | Git submodule at libs/SAF/ | Pinned version, offline builds, full control |
| Speaker layout param | APVTS AudioParameterChoice | Easy preset save/recall; hosts may show in automation lanes |
| Default output bus | Stereo | Maximum DAW compatibility; users select larger layouts explicitly |
| Source file layout | Subfolder (DSP/, Data/) | Matches component architecture; easier navigation with 10+ files |
| Tempo sync divisions | Extended (dotted + triplet) | More musical flexibility: Off, 1/16T, 1/16, 1/16D, 1/8T, 1/8, 1/8D, 1/4T, 1/4, 1/4D, 1/2, 1/2D, 1, 2, 4 bars |
| Azimuth convention | Counter-clockwise (0=front, +90=left) | Matches SAF convention and academic VBAP literature |

## Parameter Definitions (Stage 1 Scope)

All 17 parameters defined in APVTS:

### Motion Engine (9 params)
| ID | Name | Type | Range | Default | Skew |
|----|------|------|-------|---------|------|
| PATH | Path | Choice | Orbit/Pendulum/Linear/Drift | 0 (Orbit) | Linear |
| SPEED | Speed | Float | 0.01-20 Hz | 1.0 | Exponential (0.5) |
| WIDTH | Width | Float | 0-360 deg | 180 | Linear |
| DEPTH | Depth | Float | 0-100% | 0 | Linear |
| TILT | Tilt | Float | -90 to +90 deg | 0 | Linear |
| PHASE | Phase | Float | 0-360 deg | 0 | Linear |
| ELEVATION_ENABLE | Elevation | Bool | Off/On | Off | N/A |
| ELEVATION_RANGE | Elev Range | Float | 0-90 deg | 45 | Linear |
| TEMPO_SYNC | Tempo Sync | Choice | Off/1/16T/1/16/1/16D/1/8T/1/8/1/8D/1/4T/1/4/1/4D/1/2/1/2D/1/2/4 bars | 0 (Off) | Linear |

### Spatial Rendering (5 params)
| ID | Name | Type | Range | Default | Skew |
|----|------|------|-------|---------|------|
| SPEAKER_LAYOUT | Speaker Layout | Choice | Stereo/Quad/5.1/7.1/5.1.4/7.1.4/Hex/Oct/Custom | 0 (Stereo) | Linear |
| DISTANCE | Distance | Float | 0.1-30 m | 1.0 | Exponential (0.5) |
| AIR_ABSORPTION | Air Absorption | Float | 0-100% | 50 | Linear |
| ATTENUATION_CURVE | Atten Curve | Choice | Linear/Inverse/InvSquare | 1 (Inverse) | Linear |
| CENTER_DIVERGE | Center Diverge | Float | 0-100% | 0 | Linear |

### Mix / Source (3 params)
| ID | Name | Type | Range | Default | Skew |
|----|------|------|-------|---------|------|
| SOURCE_MODE | Source Mode | Choice | Mono/L+R Split | 0 (Mono) | Linear |
| LR_OFFSET | L/R Offset | Float | 0-360 deg | 180 | Linear |
| MIX | Mix | Float | 0-100% | 100 | Linear |

## Source File Organization

```
plugins/O-Orbit/
  CMakeLists.txt
  libs/
    SAF/              (git submodule)
  Source/
    PluginProcessor.h
    PluginProcessor.cpp
    PluginEditor.h
    PluginEditor.cpp
    DSP/
      MotionEngine.h      (stub - implemented in Stage 2.1)
      MotionEngine.cpp
      VBAPRenderer.h       (stub - implemented in Stage 2.2)
      VBAPRenderer.cpp
      DistanceModel.h      (stub - implemented in Stage 2.1)
      DistanceModel.cpp
    Data/
      SpeakerLayout.h      (struct definitions)
      SpeakerLayout.cpp
      SpeakerPresets.h     (preset layout data)
  Resources/
    ui/                    (empty - populated in Stage 3)
```

## Open Questions

- SAF submodule: exact commit/tag to pin (use latest stable release)
- Windows CI setup for SAF (MKL vs OpenBLAS) - deferred to Stage 4 or CI setup

## Next Phase

Ready for: **research** phase (investigate SAF submodule setup, CMake integration patterns)
