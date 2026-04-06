# Stage 3 Phase 3.1: Layout + Controls + Parameter Binding — Summary

**Date:** 2026-04-05
**Status:** Complete
**Requirements:** UI-01 (must), UI-03 (nice)

## What Was Built

Replaced GenericAudioProcessorEditor with full WebView UI using the Ouaricon Naturalist aesthetic.

### Files Created (5)
- `Source/ui/public/index.html` — Full layout (800x600 grid, XY pad, 18 knobs, 1 toggle, botanical overlay)
- `Source/ui/public/js/main.js` — 21 relay bindings, XY pad drag, knob drag, toggle, host automation sync
- `Source/ui/public/js/juce/index.js` — JUCE bridge (copied from O-Texture)
- `Source/ui/public/js/juce/check_native_interop.js` — Native interop (copied from O-Texture)
- `Source/ui/public/img/flora.png` — Botanical overlay (copied from O-SimpleReverb)

### Files Modified (4)
- `CMakeLists.txt` — NEEDS_WEB_BROWSER TRUE, NEEDS_WEBVIEW2 TRUE, binary data target, JUCE_WEB_BROWSER=1, JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
- `Source/PluginEditor.h` — Full WebView editor with 21 relays + 21 attachments (correct destruction order)
- `Source/PluginEditor.cpp` — Resource provider (5 bare-path matches), relay init, attachment wiring
- `Source/PluginProcessor.cpp` — createEditor() returns OFormantEditor

## Parameter Binding (21 total)

| Type | Count | Bound |
|------|-------|-------|
| WebSliderRelay | 20 | All 20 connected via WebSliderParameterAttachment |
| WebToggleButtonRelay | 1 | autoConsonant connected via WebToggleButtonParameterAttachment |

## Key Implementation Details

- **Member destruction order:** Relays → WebView → Attachments (C++ reverse declaration)
- **Resource provider:** Bare path matching with == (not URL stripping)
- **XY Pad:** pointer events with capture, Y-inverted, DPR-aware canvas rendering
- **Knobs:** Vertical drag with pointer capture, -135 to +135 degree rotation
- **Vowel labels:** 5 IPA symbols at acoustic positions on canvas
- **Botanical overlay:** flora.png at 0.35 opacity, pointer-events none

## Build Result

- VST3 + AU compiled successfully
- Installed to ~/Library/Audio/Plug-Ins/
