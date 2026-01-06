# OuariconTremolo Notes

## Status
- **Current Status:** 📦 Installed
- **Version:** 1.1.1
- **Type:** Audio Effect (Tremolo)

## Lifecycle Timeline

- **2026-01-03:** Plugin properly initialized with complete contract structure
  - Created `.ideas/` directory with all required contracts
  - Moved UI mockups to proper location (`.ideas/mockups/`)
  - Registered in PLUGINS.md
  - Ready for `/implement` workflow

- **2026-01-04 (Stage 1):** Foundation complete
  - Build system created (CMakeLists.txt)
  - All 6 parameters implemented in APVTS
  - PluginProcessor and PluginEditor skeleton files

- **2026-01-04 (Stage 2):** Audio Engine Working - DSP implementation complete
  - LFO generator with 6 waveforms (Sine, Triangle, Phasor, Noise, Square, Pulse)
  - One-pole lowpass smoothing filter (0-100% parameter control)
  - Pan Sync feature (stereo modulation with 180° L/R phase offset)
  - Tempo Sync integration (locks to DAW BPM with note division quantization)
  - Gain modulation with depth control (0-100%)
  - Real-time safe implementation (atomic parameter reads, ScopedNoDenormals)
  - All processing pipelines optimized for mono and stereo modes

- **2026-01-04 (v1.0.0):** Initial release - plugin completed but crashes on load in all DAWs

- **2026-01-05 (v1.0.1):** Critical crash fix
  - Fixed WebView navigation crash that prevented plugin loading
  - Root cause: goToURL() called in constructor before window context existed
  - Solution: Moved navigation to parentHierarchyChanged() callback
  - Plugin now loads successfully in Logic Pro, Ableton, Reaper, etc.

- **2026-01-05 (v1.1.0):** Musical division display enhancement
  - Added musical rhythmic value display when tempo sync is enabled
  - Expanded from 6 to 16 musical divisions (straight, triplets, quintuplets)
  - Speed dial now shows "1/8T", "1/4Q", etc. when synced to tempo
  - Preserves musical relationship across tempo changes
  - Hz display retained when tempo sync is OFF

- **2026-01-05 (v1.1.1):** UI polish improvements
  - Centered text in waveform dropdown menu
  - Tightened knob label/value spacing (8px → 4px gap)
  - Adjusted depth dial position up by 5px
  - Waveform visualizer now responds to depth parameter (amplitude scales 0-100%)

- **2026-01-04:** Design refinement session
  - **Typography updated**: Changed from Garamond to Baskerville (1757, authentic 18th-century botanical publication typeface)
  - **Retro control style chosen**: Bakelite Radio (1930s-1950s Art Deco aesthetic)
    - Evaluated 3 styles: Brass Laboratory, Bakelite Radio, Vintage Chrome Amp
    - Selected Bakelite for warm, approachable vintage feel with dark amber/brown tones
  - **Layout improvement**: Pan/Tempo Sync buttons changed from vertical to horizontal arrangement
  - **Knob refinement**: 70px Bakelite knobs with ribbed grip texture, cream pointer notches
  - **Button refinement**: 75px Art Deco styled buttons with minimal decoration
  - Production mockup: `v3-bakelite-radio.html` (ready for Stage 3 integration)

## Description

A tremolo effect plugin featuring a botanical aesthetic inspired by vintage herbarium illustrations. Provides classic amplitude modulation with 6 waveform types, smoothing control, tempo sync, and stereo panning capabilities.

## Design Highlights

**Visual Theme**: Botanical Scientific + Bakelite Radio Retro
- Vintage paper texture background with aged cream tones
- Semi-transparent carrot botanical illustration overlay
- Baskerville typeface (1757, authentic botanical publication typography)
- **Retro controls**: 1930s-1950s Bakelite Radio aesthetic
  - Dark amber/brown color palette (#6B3410, #4A2511, #3A1A08)
  - Warm, nostalgic Art Deco styling
  - Ribbed knobs with cream pointer notches
  - Minimal decorative accents

**User Interface**:
- Fixed 600×400px window size
- WebView-based (HTML/CSS/JS) with JUCE interop
- **Left panel**:
  - Pan Sync & Tempo Sync buttons (horizontal layout, side-by-side)
  - Speed & Depth knobs (70px Bakelite style, vertically stacked)
- **Right panel**:
  - Waveform selector dropdown
  - Real-time waveform visualizer canvas
  - Smoothing horizontal slider

## Parameters

1. **Speed** (0.1-20.0 Hz, default 4.5 Hz): Tremolo rate
   - Displays Hz when tempo sync is OFF
   - Displays musical divisions (1/1, 1/2, 1/4, 1/8, 1/16, 1/32, triplets, quintuplets) when tempo sync is ON
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
- `.ideas/mockups/v1-ui.yaml`: Original UI specification (600+ lines, reference only)
- `.ideas/mockups/v1-ui-test.html`: Original standalone prototype (reference only)
- `.ideas/mockups/v2-ui-test.html`: JUCE WebView interop version (production base)
- `.ideas/mockups/v3-bakelite-radio.html`: **FINAL DESIGN** - Bakelite Radio style with JUCE interop ⭐
  - Horizontal button layout (Pan/Tempo Sync side-by-side)
  - 70px Bakelite knobs with ribbed texture
  - Cream pointer notches (6px × 20px rectangular)
  - Dark amber Art Deco buttons (75px wide)
- `.ideas/mockups/v3-brass-laboratory.html`: Brass style variation (not chosen)
- `.ideas/mockups/v3-chrome-amp.html`: Chrome amp style variation (not chosen)
- `.ideas/mockups/js/juce/`: JUCE WebView bridge files (index.js, check_native_interop.js)
- `.ideas/mockups/RETRO-STYLE-COMPARISON.md`: Design decision documentation

## Known Issues

None - All known issues resolved in v1.0.1

## Current Design Status

**Phase**: Refinement (still in `/dream` stage)
**Mockup Status**: Finalized - Bakelite Radio style selected
**Next Action Options**:
1. Continue refining UI design (if needed)
2. Run `/implement OuariconTremolo` when ready to build

## Implementation Handoff Notes

When proceeding to `/implement`:
- Use `v3-bakelite-radio.html` as the UI source for Stage 3 (gui-agent)
- Copy to `Source/ui/public/index.html` during WebView integration
- Ensure `js/juce/` directory is copied for JUCE parameter bindings
- All 6 WebView relay names documented in `parameter-spec.md`
- Horizontal button layout provides proper spacing for knob labels

## Future Refinement Options

If continuing design work before implementation:
- Fine-tune knob/button sizes
- Adjust Bakelite color tones
- Refine waveform visualizer appearance
- Test additional layout variations

## Additional Notes

This plugin has exceptional preparation:
- Complete parameter specification with ranges, defaults, units
- Detailed DSP architecture documentation
- Comprehensive implementation plan with time estimates
- Fully functional UI mockup (interactive HTML prototype)
- All design decisions made and documented

**Ready for immediate implementation** - no additional planning required. Can proceed directly to `/implement` workflow.
