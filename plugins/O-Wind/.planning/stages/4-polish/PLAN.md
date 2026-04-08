# Stage 4: Polish - Execution Plan

## Goal

Achieve pluginval level 10 certification for O-Wind (VST3 + AU), fix the critical voice cleanup DSP reset bug, harden instrument preset comparison, add UI placeholders for deferred features, and verify CPU/latency/CC2 requirements.

## Tasks

1. [ ] Fix voice cleanup missing DSP reset
   - Files: Source/FluteSynthVoice.cpp (lines 174-181)
   - Add jetExciter, jetNonlinearity, dcBlocker, boreWaveguide, jetDelay reset calls before `clearCurrentNote()` in the silent-counter path
   - Match the existing reset pattern from `stopNote(velocity, false)` path (lines 84-90)
   - Depends on: none

2. [ ] Harden instrument preset change detection
   - Files: Source/FluteSynthVoice.cpp (lines 317-323), Source/FluteSynthVoice.h
   - Replace fragile jetGain comparison with direct preset index tracking
   - Add `int lastPresetIndex = -1;` member to FluteSynthVoice
   - Compare `presetIdx != lastPresetIndex` instead of float epsilon check
   - Depends on: none

3. [ ] Add UI placeholder elements for deferred features
   - Files: Resources/ui/index.html
   - Add hidden `<div>` for breath/jet visualization (UI-06) in Sound tab
   - Add hidden `<div>` for register indicator (UI-07) in Sound tab
   - Add hidden `<div>` for visual polish hooks (UI-08) in each tab
   - Use `class="future-feature" style="display:none;"` — zero visual impact
   - Depends on: none

4. [ ] Build Release and run pluginval level 10 (DSP-only)
   - Build: `ninja O-Wind_VST3 O-Wind_AU`
   - Install to system plugin folders (clear AU cache first)
   - Run: `pluginval --strictness-level 10 --skip-gui-tests --validate <VST3>`
   - Run: `pluginval --strictness-level 10 --skip-gui-tests --validate <AU>`
   - Fix any failures before proceeding
   - Depends on: Tasks 1, 2

5. [ ] Run pluginval level 10 with GUI tests
   - Run: `pluginval --strictness-level 10 --validate <VST3>`
   - Run: `pluginval --strictness-level 10 --validate <AU>`
   - If GUI tests fail but DSP passed in Task 4, document as known JUCE WebView limitation
   - Depends on: Task 4

6. [ ] Verify CPU per voice (PERF-02)
   - Build Standalone, play 8 simultaneous notes
   - Measure CPU via Activity Monitor or Instruments.app
   - Target: <2.5% per voice at 44.1kHz
   - Expected: well under target (simpler model than O-Bowed)
   - Depends on: Task 4

7. [ ] Verify latency reporting (PERF-03)
   - Confirm `getLatencySamples()` returns non-zero after prepareToPlay
   - Expected: ~8 samples from 2x polyphase IIR oversampling
   - Verify consistent across 44.1k/48k/96k sample rates
   - pluginval level 10 also tests latency reporting
   - Depends on: Task 4

8. [ ] Verify CC2 breath mapping (COMPAT-03)
   - Code path audit: confirm `controllerMoved` dispatches CC2 to `ccBreathPressure`
   - Verify CC2 override logic: CC2 > 0 takes priority over APVTS breathPressure
   - Document CC2=0 behavior (reverts to APVTS floor — acceptable for v1.0)
   - Depends on: none

9. [ ] Update REQUIREMENTS.md with verification results
   - Mark PERF-02, PERF-03, COMPAT-03 with final status
   - Update COMPAT-01 verified level from stage-1 to stage-4 (level 10)
   - Update traceability table
   - Depends on: Tasks 4-8

## Success Criteria

- [ ] pluginval level 10 passes for VST3 (DSP tests)
- [ ] pluginval level 10 passes for AU (DSP tests)
- [ ] GUI pluginval documented (pass or known WebView limitation)
- [ ] Voice cleanup DSP reset prevents stale state artifacts
- [ ] CPU per voice confirmed <2.5% at 44.1kHz
- [ ] Latency correctly reported to host (~8 samples)
- [ ] CC2 breath mapping code path verified
- [ ] UI placeholder elements present for UI-06, UI-07, UI-08
- [ ] Zero build warnings
- [ ] REQUIREMENTS.md updated with all verification results
