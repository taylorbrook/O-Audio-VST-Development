# Ouaricon Tremolo - Implementation Plan

## Overview

This plugin has **completed design phase** with fully specified UI mockups and parameter definitions. Implementation follows the standard 3-stage Plugin Freedom System workflow:

- **Stage 1**: Build System Ready (foundation-shell-agent)
- **Stage 2**: Audio Engine Working (dsp-agent)
- **Stage 3**: UI Integrated (gui-agent)

## Complexity Assessment

**Overall Complexity**: ⭐⭐ Low-Medium

**Rationale**:
- DSP: Standard LFO + amplitude modulation (well-documented, straightforward)
- Parameters: 6 parameters, all standard JUCE types (no custom ranges/units)
- UI: Detailed mockup already exists, WebView implementation is defined
- No external dependencies, no sample playback, no convolution
- Tempo sync adds modest complexity but is well-supported by JUCE

**Risk Areas**:
- Waveform visualizer real-time updates (performance consideration)
- Smoothing filter coefficient tuning (perceptual quality)
- Tempo sync edge cases (BPM changes, transport stop/start)

## Stage 1: Build System Ready

**Goal**: CMake project structure, JUCE integration, all 6 parameters in APVTS

**Implementation Tasks**:
1. Create CMakeLists.txt based on Plugin Freedom System template
2. Configure JUCE modules (juce_audio_processors, juce_dsp, juce_gui_basics)
3. Set up PluginProcessor.h/cpp skeleton
4. Implement APVTS with all 6 parameters:
   - SPEED_PARAM (Float, 0.1-20.0 Hz, default 4.5)
   - DEPTH_PARAM (Float, 0-100%, default 75)
   - WAVEFORM_PARAM (Choice, 0-5, default 0)
   - SMOOTHING_PARAM (Float, 0-100%, default 30)
   - PAN_SYNC_PARAM (Bool, default false)
   - TEMPO_SYNC_PARAM (Bool, default false)
5. Set plugin metadata (name, manufacturer, version, formats)
6. Basic processBlock skeleton (passthrough initially)

**Validation**:
- Project builds successfully
- All parameters visible in DAW automation lanes
- Plugin loads in DAW without crashes
- Parameter changes registered (even if not affecting audio yet)

**Estimated Effort**: 30-45 minutes

## Stage 2: Audio Engine Working

**Goal**: Fully functional tremolo DSP with all waveforms and modulation features

### 2.1 Core DSP Components

**LFO Generator**:
- Phase accumulator (member variable: `float lfoPhase`)
- Phase increment calculation in prepareToPlay
- Waveform generation function (switch statement for 6 types)
- Support for mono and stereo (Pan Sync) phase management

**Smoothing Filter**:
- One-pole lowpass IIR implementation
- Coefficient calculation from SMOOTHING_PARAM
- Per-channel state tracking (if Pan Sync enabled)

**Gain Modulation**:
- Convert LFO output (-1 to +1) to gain multiplier (0 to 1)
- Scale by DEPTH_PARAM
- Apply to audio samples

### 2.2 Tempo Sync Implementation

**Host Transport Integration**:
- Query `juce::AudioPlayHead` for BPM and time signature
- Map speed parameter to note divisions when TEMPO_SYNC enabled
- Calculate Hz from BPM and division
- Handle missing playhead gracefully (fallback to free-running mode)

**Note Division Mapping**:
- Speed parameter range becomes discrete steps when synced
- Suggested divisions: 1/1, 1/2, 1/4, 1/8, 1/16, 1/32
- Convert division to Hz: `freq = (BPM / 60) / (division * 4)`

### 2.3 Pan Sync Implementation

**Stereo Phase Offset**:
- Left channel: main phase
- Right channel: (main phase + 0.5) mod 1.0
- Apply when PAN_SYNC_PARAM is true
- Fallback to mono (identical phase) when false

### 2.4 Parameter Handling

**Smoothing Strategy**:
- Use JUCE's built-in parameter smoothing for Speed, Depth, Smoothing
- Avoid zipper noise on parameter automation
- Waveform changes: Crossfade or instant switch (TBD during implementation)

**Edge Cases**:
- Division by zero prevention (speed minimum 0.1 Hz)
- Gain multiplier clamping (0.0 to 1.0)
- Phase wraparound handling

### 2.5 Processing Pipeline

**prepareToPlay**:
- Initialize LFO phase to 0.0
- Calculate initial phase increment from sample rate
- Initialize smoothing filter state
- Allocate any required buffers

**processBlock**:
```
For each buffer:
  1. Check for tempo sync state, update phase increment if needed
  2. Get current parameter values (speed, depth, waveform, smoothing, pan sync)
  3. For each sample:
     a. Update LFO phase(s)
     b. Generate raw waveform value(s)
     c. Apply smoothing filter
     d. Calculate gain multiplier(s)
     e. Multiply input by gain
  4. Write output samples
```

**Validation**:
- Tremolo effect audible and musical
- All 6 waveforms work correctly
- Tempo sync locks to DAW tempo
- Pan Sync creates stereo width modulation
- Smoothing parameter affects waveform as expected
- No clicks, pops, or artifacts
- Works at 44.1kHz, 48kHz, 96kHz, 192kHz

**Estimated Effort**: 1.5-2 hours

## Stage 3: UI Integrated

**Goal**: WebView UI connected to audio parameters with full visual feedback

### 3.1 WebView Setup

**PluginEditor Integration**:
- Create JUCE WebBrowserComponent in PluginEditor.cpp
- Load HTML from `Source/ui/public/index.html`
- Configure WebView size (600×400px fixed)
- Enable JavaScript interop (`juce::WebBrowserComponent::Options`)

**Resource Management**:
- Embed HTML/CSS/JS as binary resources (or load from file path)
- Include image assets (paper.jpg, carrot.png)
- Configure resource path for development vs release builds

### 3.2 Parameter Binding

**JavaScript ↔ C++ Bridge**:
- Implement `juce::WebSliderRelay` pattern for bidirectional parameter sync
- Map each control to APVTS parameter:
  - Speed knob → SPEED_PARAM
  - Depth knob → DEPTH_PARAM
  - Waveform dropdown → WAVEFORM_PARAM
  - Smoothing slider → SMOOTHING_PARAM
  - Pan Sync button → PAN_SYNC_PARAM
  - Tempo Sync button → TEMPO_SYNC_PARAM

**Update Flow**:
- UI change → JavaScript → C++ → APVTS → DSP
- DSP automation → APVTS → C++ → JavaScript → UI update

### 3.3 Visual Feedback

**Waveform Visualizer**:
- Canvas element updates in real-time
- Listen to WAVEFORM_PARAM and SMOOTHING_PARAM changes
- Redraw waveform using JavaScript canvas API
- 4 cycles displayed across visualizer width
- Grid lines for amplitude reference

**Knob Rotation**:
- Speed/Depth knobs rotate indicator based on parameter value
- Range: -140° to +140° (280° total travel)
- Value displays update with unit formatting

**Button States**:
- Pan Sync / Tempo Sync toggle active state CSS class
- Color/border change on enable

### 3.4 High-DPI Support

**Resolution Independence**:
- Canvas scaled for Retina displays (devicePixelRatio)
- SVG or CSS graphics for crisp rendering at any zoom
- Test on standard and high-DPI monitors

### 3.5 Testing Checklist

**Functional**:
- All controls respond to mouse interaction
- Parameter values display correctly with units
- Automation from DAW updates UI controls
- Waveform visualizer shows all 6 waveform types
- Smoothing parameter affects visualizer in real-time
- Buttons toggle active state visually

**Visual Quality**:
- Paper texture background loads correctly
- Carrot botanical overlay positioned properly
- Botanical motifs (❦, ✿, ❧) render at correct positions
- Typography (Garamond) displays correctly
- Colors match specification
- UI readable on light and dark DAW backgrounds

**Performance**:
- No lag when dragging knobs
- Canvas redraws smoothly (<16ms per frame for 60fps)
- No audio dropouts caused by UI updates
- CPU usage remains reasonable with continuous waveform animation

**Validation**:
- Plugin builds with WebView UI
- All controls functional and bound to parameters
- Visual design matches mockup
- High-DPI rendering correct
- No console errors in WebView inspector
- pluginval passes (VST3/AU validation)

**Estimated Effort**: 1.5-2 hours

## Stage 4: Final Validation & Polish

**Automated Testing**:
- Run pluginval on VST3 and AU builds
- Test in multiple DAWs (Logic, Ableton, Reaper)
- Verify preset save/load
- Stress test with automation

**Quality Checks**:
- No audio artifacts (clicks, pops, DC offset)
- No memory leaks (run for extended periods)
- Proper handling of sample rate changes
- Tempo sync responds to BPM changes immediately
- UI scales correctly on different monitor resolutions

**Documentation**:
- Update CHANGELOG.md
- Create presets (8 factory presets suggested in parameter-spec.md)
- Test preset compatibility across DAWs

**Installation**:
- Use `/install-plugin OuariconTremolo` to deploy to system folders
- Test in real DAW session
- Verify AU and VST3 formats both work

**Estimated Effort**: 30-45 minutes

## Total Estimated Time

- Stage 1: 30-45 min
- Stage 2: 1.5-2 hrs
- Stage 3: 1.5-2 hrs
- Stage 4: 30-45 min

**Total**: ~4-5 hours for complete implementation

## Critical Dependencies

**JUCE Modules Required**:
- juce_audio_basics
- juce_audio_processors
- juce_audio_devices
- juce_gui_basics
- juce_gui_extra (for WebBrowserComponent)
- juce_dsp (optional, for filter implementations)

**External Libraries**: None (pure JUCE implementation)

**Platform Requirements**:
- macOS 13+ for WebView support
- Xcode Command Line Tools or Xcode
- CMake 3.15+
- JUCE 8.0.0+

## Risk Mitigation

### Waveform Visualizer Performance
**Risk**: Canvas redraws on every parameter change could impact UI responsiveness
**Mitigation**:
- Throttle redraws to 30fps (every 33ms)
- Use requestAnimationFrame for smooth updates
- Only redraw when waveform or smoothing changes (not on speed/depth)

### Tempo Sync Edge Cases
**Risk**: BPM changes or missing playhead info could cause incorrect LFO speed
**Mitigation**:
- Graceful fallback to free-running mode if playhead unavailable
- Smooth transitions when switching sync on/off
- Test with various DAWs (different playhead implementations)

### Smoothing Filter Tuning
**Risk**: Coefficient mapping from 0-100% parameter might not feel perceptually linear
**Mitigation**:
- Test with all waveforms during Stage 2
- Adjust mapping curve if needed (logarithmic/exponential)
- Document final mapping in code comments

## Success Criteria

Plugin is considered complete when:
1. ✅ All 6 parameters functional and automatable
2. ✅ All 6 waveforms generate correct tremolo effect
3. ✅ Tempo sync locks accurately to DAW BPM
4. ✅ Pan Sync creates convincing stereo width modulation
5. ✅ Smoothing parameter affects waveform character noticeably
6. ✅ UI matches botanical design specification exactly
7. ✅ Waveform visualizer updates in real-time
8. ✅ No audio artifacts or performance issues
9. ✅ Passes pluginval for VST3 and AU
10. ✅ Works in at least 3 different DAWs

## Next Steps

After implementation plan is approved:
1. Run `/implement OuariconTremolo` to start Stage 1
2. foundation-shell-agent creates project structure and parameters
3. validation-agent verifies Stage 1 completion
4. Proceed to Stage 2 (DSP implementation)
5. Continue through stages with automatic validation at each checkpoint

**The plugin is ready for implementation. All design work is complete.**
