# OuariconTremolo Notes

## Status
- **Current Status:** 💡 Ideated
- **Version:** N/A (not yet implemented)
- **Type:** Audio Effect (Tremolo)

## Lifecycle Timeline

- **2026-01-03:** Plugin properly initialized with complete contract structure
  - Created `.ideas/` directory with all required contracts
  - Moved UI mockups to proper location (`.ideas/mockups/`)
  - Registered in PLUGINS.md
  - Ready for `/implement` workflow

## Description

A tremolo effect plugin featuring a botanical aesthetic inspired by vintage herbarium illustrations. Provides classic amplitude modulation with 6 waveform types, smoothing control, tempo sync, and stereo panning capabilities.

## Design Highlights

**Visual Theme**: Botanical Scientific
- Vintage paper texture background with aged cream tones
- Semi-transparent carrot botanical illustration overlay
- Botanical unicode motifs (❦ fleuron, ✿ floral, ❧ leaf)
- Earthy botanical green color palette
- Garamond typography for classical elegance

**User Interface**:
- Fixed 600×400px window size
- WebView-based (HTML/CSS/JS)
- Left panel: Speed/Depth knobs, Pan/Tempo Sync toggle buttons
- Right panel: Waveform selector, real-time visualizer, Smoothing slider
- All controls feature botanical-themed decorations

## Parameters

1. **Speed** (0.1-20.0 Hz, default 4.5 Hz): Tremolo rate
2. **Depth** (0-100%, default 75%): Modulation intensity
3. **Waveform** (Sine/Triangle/Phasor/Noise/Square/Pulse, default Sine): Modulation shape
4. **Smoothing** (0-100%, default 30%): Waveform curve softness
5. **Pan Sync** (Bool, default OFF): Stereo width modulation (L/R 180° phase offset)
6. **Tempo Sync** (Bool, default OFF): Lock to DAW tempo

## Technical Implementation

**DSP Architecture**:
- Phase accumulator LFO with 6 waveform generators
- One-pole lowpass smoothing filter
- Stereo phase offset for Pan Sync mode
- Host tempo integration for Tempo Sync
- Zero-latency amplitude modulation

**Complexity**: ⭐⭐ Low-Medium
- Standard tremolo DSP (well-documented)
- 6 parameters, all standard JUCE types
- Detailed UI mockup already exists
- No external dependencies

**Estimated Implementation Time**: 4-5 hours total
- Stage 1 (Build System Ready): 30-45 min
- Stage 2 (Audio Engine Working): 1.5-2 hrs
- Stage 3 (UI Integrated): 1.5-2 hrs
- Stage 4 (Validation & Polish): 30-45 min

## Assets

**Images**:
- `paper.jpg`: Vintage paper texture background
- `carrot.png`: Botanical illustration overlay

**Mockups**:
- `.ideas/mockups/v1-ui.yaml`: Complete UI specification (600+ lines)
- `.ideas/mockups/v1-ui-test.html`: Fully functional interactive prototype
- `.ideas/mockups/v1-ui-mockup.jpg`: Visual reference image

## Known Issues

None (plugin not yet implemented)

## Next Steps

1. Run `/implement OuariconTremolo` to begin Stage 1
2. foundation-shell-agent will create CMakeLists.txt, PluginProcessor skeleton, APVTS parameters
3. Automatic validation after each stage
4. Expected completion: All 3 stages complete with working VST3/AU plugin

## Additional Notes

This plugin has exceptional preparation:
- Complete parameter specification with ranges, defaults, units
- Detailed DSP architecture documentation
- Comprehensive implementation plan with time estimates
- Fully functional UI mockup (interactive HTML prototype)
- All design decisions made and documented

**Ready for immediate implementation** - no additional planning required. Can proceed directly to `/implement` workflow.
