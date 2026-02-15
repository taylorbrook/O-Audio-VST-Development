# Stage 1: Foundation - Context

## Discussion Summary

**Date:** 2026-02-13
**Participants:** User, Claude

## Requirements Confirmed

- **Bus configuration:** Output-only (stereo). No audio input bus. Standard instrument config -- audio comes from loaded files only.
- **Plugin formats:** VST3 + AU + Standalone (Standalone for easy testing without a DAW)
- **IS_SYNTH TRUE:** Instrument plugin, appears in DAW instrument browser
- **NEEDS_MIDI_INPUT TRUE:** Required for three MIDI modes
- **NEEDS_WEB_BROWSER TRUE:** WebView-based scatter plot UI
- **NEEDS_WEBVIEW2 TRUE:** Windows WebView2 support (with static linking)
- **Grain Size skew:** Logarithmic (skewed) range for 10-500ms parameter, giving finer control in the 10-100ms range
- **Sample rate:** Store currentSampleRate in prepareToPlay() during Stage 1 as a member variable, ready for Stage 2 re-segmentation logic

## Parameters (12 total)

| Parameter ID | Type | Range | Default | Skew | Notes |
|--------------|------|-------|---------|------|-------|
| `ENERGY` | Float | 0.0-1.0 | 0.5 | Linear | Macro: bias grain selection quiet-loud |
| `BRIGHTNESS` | Float | 0.0-1.0 | 0.5 | Linear | Macro: bias dark-bright |
| `TEXTURE` | Float | 0.0-1.0 | 0.5 | Linear | Macro: bias smooth-rough |
| `POSITION` | Float | 0.0-1.0 | 0.0 | Linear | Temporal position in source file |
| `GRAIN_DENSITY` | Int | 1-64 | 8 | Linear | Simultaneous grain count |
| `GRAIN_SIZE` | Float | 10-500 | 50 | **Log skew** | Grain length in ms (log for fine 10-100ms control) |
| `SCATTER_X` | Float | 0.0-1.0 | 0.5 | Linear | Cursor X on scatter plot |
| `SCATTER_Y` | Float | 0.0-1.0 | 0.5 | Linear | Cursor Y on scatter plot |
| `VARIATION` | Float | 0.0-1.0 | 0.2 | Linear | Randomization radius |
| `CROSSFADE` | Float | 0-100 | 50 | Linear | Grain overlap % |
| `OUTPUT_GAIN` | Float | -60 to +12 | 0 | Linear | Master output in dB |
| `MIDI_MODE` | Choice | 0-2 | 2 (Drone) | N/A | Pitch/Trigger/Drone |

## Constraints Identified

- **Output-only bus:** No `isBusesLayoutSupported` needed for input. Output must support stereo.
- **No DSP in Stage 1:** processBlock is an empty stub (silence output). DSP comes in Stage 2.
- **WebView placeholder only:** PluginEditor creates WebBrowserComponent but loads a simple "Loading UI..." placeholder page. No WebGL/scatter plot yet.
- **PLUGIN_CODE:** Must be unique 4-char code (not `OuGS` which is O-GrainScatter). Suggest `OuTF`.
- **Version:** Start at 1.0.0
- **Plugin target name:** `OuariconTextureForge` (follows Ouaricon naming convention)
- **PRODUCT_NAME:** `O-TextureForge${OUARICON_DEV_SUFFIX}`

## Approach Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Bus config | Output-only stereo | Instrument (synthesizer), no audio input needed for v1 |
| Formats | VST3 + AU + Standalone | Standalone useful for quick testing |
| Grain Size skew | Logarithmic | Most useful grain sizes (20-80ms) need fine control |
| Sample rate handling | Store in Stage 1 | Minimal overhead, prepares for Stage 2 re-segmentation |
| Reference plugin | O-GrainScatter | CMakeLists pattern, APVTS layout, viz snapshot double-buffer |
| WebView UI resources | BinaryData (juce_add_binary_data) | Matches O-GrainScatter pattern |
| Compile definitions | JUCE_WEB_BROWSER=1, JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1, JUCE_VST3_CAN_REPLACE_VST2=0, JUCE_USE_CURL=0 | Standard Ouaricon WebView plugin config |

## CMakeLists.txt Pattern (from O-GrainScatter)

Key elements to replicate:
1. `include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)`
2. `juce_add_plugin()` with IS_SYNTH, NEEDS_WEB_BROWSER, NEEDS_WEBVIEW2, NEEDS_MIDI_INPUT
3. `juce_add_binary_data()` for WebView UI resources
4. Standard JUCE module links (juce_audio_basics through juce_gui_extra)
5. `juce_generate_juce_header()`
6. Compile definitions for WebView2 static linking

## File Structure (Stage 1)

```
plugins/O-TextureForge/
├── CMakeLists.txt
└── Source/
    ├── PluginProcessor.h
    ├── PluginProcessor.cpp
    ├── PluginEditor.h
    ├── PluginEditor.cpp
    └── ui/public/
        ├── index.html (placeholder)
        └── js/
            └── juce/
                ├── index.js
                └── check_native_interop.js
```

## Open Questions

- None. All Stage 1 requirements are clear from ROADMAP + discussion.

## Test Criteria (from ROADMAP)

- Plugin appears in DAW instrument browser
- GUI window opens without crash
- All 12 parameters visible in DAW automation list
- Parameter changes from DAW reflected in plugin (APVTS working)
- macOS: AU validation with `auval -v`, VST3 scan
- Standalone opens and shows placeholder UI

## Next Phase

Ready for: **research** phase (or skip to **plan** if no unknowns)
