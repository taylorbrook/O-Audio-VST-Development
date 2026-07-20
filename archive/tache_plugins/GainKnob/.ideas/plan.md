# GainKnob - Implementation Plan

**Date:** 2025-11-10
**Complexity Score:** 1.2 (Simple)
**Strategy:** Single-pass implementation

## Complexity Factors

- Parameters: 1 (0.2 points)
- Algorithms: 1 (gain multiplication with dB conversion)
- Features: 0 (no advanced features)

## Stages

- Stage 0: Research ✓
- Stage 1: Planning ← Current
- Stage 2: Foundation
- Stage 3: Shell
- Stage 4: DSP
- Stage 5: GUI
- Stage 6: Validation

## Stage Details

### Stage 2: Foundation (5 min)
- Create CMakeLists.txt with JUCE 8 configuration
- Generate minimal PluginProcessor and PluginEditor classes
- Verify compilation (VST3, AU, Standalone)

### Stage 3: Shell (5 min)
- Implement APVTS with single GAIN parameter
- Add state management (getStateInformation/setStateInformation)
- Update processBlock stub

### Stage 4: DSP (10 min)
- Implement gain conversion (dB to linear)
- Add special case handling (≤ -59.9 dB → 0.0)
- Apply gain to audio buffer
- Add denormal protection (optional)

### Stage 5: GUI (15 min)
- Copy finalized UI mockup to ui/public/index.html
- Download JUCE frontend library
- Create WebView infrastructure (relay, attachment)
- Update CMakeLists.txt for WebView support
- Verify member order (Relays → WebView → Attachments)

### Stage 6: Validation (15 min)
- Create 3-5 factory presets
- Run automated tests
- Generate CHANGELOG.md
- Mark ready for installation

## Estimated Duration

**Total: ~50 minutes**

- Stage 2: 5 min
- Stage 3: 5 min
- Stage 4: 10 min
- Stage 5: 15 min
- Stage 6: 15 min

## Implementation Notes

- **Simplicity:** This is the simplest possible plugin type
- **No phases needed:** Single-pass implementation for all stages
- **UI already finalized:** v1 mockup ready for Stage 5
- **Test focus:** Verify parameter automation and preset recall
