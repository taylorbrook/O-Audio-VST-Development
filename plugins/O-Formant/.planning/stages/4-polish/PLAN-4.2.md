# Stage 4 Phase 4.2: Validation + Release — Execution Plan

**Date:** 2026-04-05
**Goal:** Achieve pluginval level 10 validation for both VST3 and AU, write CHANGELOG.md, final build and install

---

## Tasks

### 1. [ ] Clean build VST3 + AU
- **Files:** none (build system)
- **Depends on:** none
- **Details:**
  - `cd /Users/taylorbrook/Dev/VST-development/build && ninja O-Formant_VST3 O-Formant_AU`
  - Verify zero errors (4 pre-existing sign-conversion warnings in GlottalTableGenerator.cpp are acceptable)

### 2. [ ] Install to system folders with cache clear
- **Files:** none (system install)
- **Depends on:** Task 1
- **Details:**
  - Kill AudioComponentRegistrar, clear AU caches
  - Remove old VST3 + AU from system folders
  - Copy fresh binaries from build artefacts
  - ```bash
    killall -9 AudioComponentRegistrar 2>/dev/null || true
    rm -rf ~/Library/Caches/AudioUnitCache/ ~/Library/Caches/com.apple.audiounits.cache
    rm -rf ~/Library/Audio/Plug-Ins/VST3/O-Formant.vst3
    rm -rf ~/Library/Audio/Plug-Ins/Components/O-Formant.component
    cp -R build/plugins/O-Formant/O-Formant_artefacts/Release/VST3/O-Formant.vst3 ~/Library/Audio/Plug-Ins/VST3/
    cp -R build/plugins/O-Formant/O-Formant_artefacts/Release/AU/O-Formant.component ~/Library/Audio/Plug-Ins/Components/
    ```

### 3. [ ] Run pluginval level 10 on VST3
- **Files:** none (validation)
- **Depends on:** Task 2
- **Details:**
  - ```bash
    /Applications/pluginval.app/Contents/MacOS/pluginval \
      --strictness-level 10 \
      --validate-in-process \
      --timeout-ms 120000 \
      --validate ~/Library/Audio/Plug-Ins/VST3/O-Formant.vst3
    ```
  - Save random seed from output for reproducibility
  - Expected: PASS (level 5 already passing, DSP has NaN guards + freq clamping)
  - Key risk areas: parameter thread safety (500x concurrent), non-releasing SR switch, state restoration

### 4. [ ] Run pluginval level 10 on AU
- **Files:** none (validation)
- **Depends on:** Task 2
- **Details:**
  - ```bash
    /Applications/pluginval.app/Contents/MacOS/pluginval \
      --strictness-level 10 \
      --validate-in-process \
      --timeout-ms 120000 \
      --validate ~/Library/Audio/Plug-Ins/Components/O-Formant.component
    ```
  - Same risk areas as VST3

### 5. [ ] Fix any pluginval level 10 failures (conditional)
- **Files:** TBD based on failures
- **Depends on:** Tasks 3, 4
- **Details:**
  - If NaN/Inf in audio output: add tanh soft clipping safety net in processBlock (O-Prism pattern)
  - If state restoration fails: check preset name round-trip, verify APVTS XML serialization
  - If subnormal warnings: verify ScopedNoDenormals present (already is), add explicit flush-to-zero on filter output
  - If parameter thread safety crash: verify all audio-thread parameter access uses atomic loads (already does)
  - If this task triggers: rebuild (Task 1), reinstall (Task 2), re-validate (Tasks 3-4)
  - Skip if Tasks 3+4 both pass clean

### 6. [ ] Write CHANGELOG.md
- **Files:** `plugins/O-Formant/CHANGELOG.md` (create)
- **Depends on:** Tasks 3, 4 (need final validation level to document)
- **Details:**
  - Standard project format (Keep a Changelog style)
  - v1.0.0 entry covering all 4 stages:
    - Stage 1: JUCE 8 project, CMake, 21 APVTS parameters, MPESynthesiser
    - Stage 2: LF glottal model, 5-formant filter bank, vowel morpher, consonant engine, vibrato, pitch glide, ADSR
    - Stage 3: WebView UI, XY vowel pad, formant overlay, parameter sections
    - Stage 4: 16 factory presets, preset browser, outputGain + stereoWidth, pluginval level 10
  - Technical notes: domain, parameter count, JUCE version, formats
  - Template in RESEARCH-4.2.md

### 7. [ ] Final verification
- **Files:** none
- **Depends on:** Tasks 3, 4, 6
- **Details:**
  - Confirm both VST3 and AU passed level 10
  - Confirm CHANGELOG.md exists and follows project conventions
  - Confirm plugins installed in system folders
  - Run `auval -a | grep -i formant` to verify AU registered

---

## Success Criteria

- [ ] pluginval level 10 PASSES for VST3
- [ ] pluginval level 10 PASSES for AU
- [ ] CHANGELOG.md created with v1.0.0 entry (all stages documented)
- [ ] VST3 + AU installed to system folders
- [ ] No new code issues introduced (if fixes were needed)

---

## Files Summary

| Action | File |
|--------|------|
| Create | `CHANGELOG.md` |
| Modify | Source files only if pluginval level 10 reveals issues (conditional) |

---

## Requirements Addressed

| Requirement | Task |
|-------------|------|
| COMPAT-01 (pluginval validation — level 10 upgrade) | Tasks 3, 4, 5 |

---

## Risk Mitigation

| Risk | Likelihood | Mitigation |
|------|-----------|------------|
| Biquad instability at extreme param randomization | LOW | Triple-layer NaN protection already in place |
| Non-releasing SR switch crash | LOW | prepareToPlay does full reset, no releaseResources dependency |
| State restoration tolerance failure | LOW | APVTS XML round-trip is deterministic |
| Parameter thread safety crash | LOW | All audio-thread reads are atomic loads via APVTS |
| Wavetable gen timeout | NONE | --timeout-ms 120000 (gen takes <100ms) |

---

## Estimated Scope

- 7 tasks (2 conditional/skip-if-clean)
- Primary work: build + validate + write changelog
- If level 10 passes clean: ~15 minutes execution
- If fixes needed: +30 minutes per fix iteration
