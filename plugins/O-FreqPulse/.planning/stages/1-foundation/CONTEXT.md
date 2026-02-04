# Stage 1: Foundation - Context

**Phase:** Discuss Complete
**Date:** 2026-02-03

## Stage Goal
Build system setup, 165 APVTS parameters, plugin shell with passthrough audio.

## Decisions Made

### 1. Module Strategy
**Decision:** Fresh build (no shared Ouaricon modules)
**Rationale:** Custom FFT processing requires unique code paths. WebView boilerplate will be implemented in Stage 3.

### 2. Parameter Organization
**Decision:** Grouped by band in DAW automation
**Implementation:** Use JUCE parameter groups:
- Global (mix, steps, rate, swing, smoothing)
- Band 1 Sub (enable, low, high, depth, euc_on, euc_steps, euc_pulses, euc_offset, steps 1-32)
- Band 2 Low (same pattern)
- Band 3 Mid (same pattern)
- Band 4 High (same pattern)

### 3. Audio Path Verification
**Decision:** Passthrough only (no test tone)
**Rationale:** Standard approach - rely on input signal for verification.

## Requirements Summary

From ARCHITECTURE.md and REQUIREMENTS.md:
- **165 total parameters:** 5 global + 32 band (8×4) + 128 step (32×4)
- **JUCE modules:** juce_audio_processors, juce_dsp, juce_gui_extra
- **Formats:** VST3, AU
- **Latency:** Will be set in Stage 2 (2048 samples for FFT)

## Success Criteria

- [ ] Plugin compiles for VST3 and AU
- [ ] Plugin loads in DAW without crash
- [ ] All 165 parameters visible in DAW automation
- [ ] Parameters grouped logically in automation list
- [ ] Audio passes through unchanged (passthrough)
- [ ] State save/load works (preset recall)

## Constraints

- No DSP processing in Stage 1 (defer to Stage 2)
- No UI implementation in Stage 1 (defer to Stage 3)
- Editor can be minimal placeholder

## Files to Create

```
plugins/O-FreqPulse/
├── CMakeLists.txt
├── Source/
│   ├── PluginProcessor.h
│   ├── PluginProcessor.cpp
│   ├── PluginEditor.h
│   └── PluginEditor.cpp
```
