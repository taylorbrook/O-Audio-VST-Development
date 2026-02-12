# Stage 4: Polish - Execution Plan

**Plugin:** O-Orbit
**Date:** 2026-02-11
**Based on:** CONTEXT.md, RESEARCH.md

---

## Goal

Fix identified bugs, implement 12 factory presets via Programs API, pass pluginval (strictness 10) and auval validation, and verify state persistence round-trip correctness.

---

## Phase 4.1: Bug Fixes (2 tasks)

### Task 1: Fix handleAsyncUpdate() overwriting custom layouts on state restore

**Files:** `Source/PluginProcessor.cpp` (handleAsyncUpdate, ~line 182)
**Depends on:** None
**Risk found in research:** HIGH

**Problem:** `handleAsyncUpdate()` unconditionally sets `useCustomLayout = false` and loads a preset layout. When `setStateInformation()` restores a custom layout, `parameters.replaceState()` triggers the speaker layout parameter listener, which calls `triggerAsyncUpdate()`. The async callback then overwrites the just-restored custom layout with a preset.

**Fix:** Guard `handleAsyncUpdate()` to skip preset load when `useCustomLayout` is true:

```cpp
void OOrbitProcessor::handleAsyncUpdate()
{
    if (useCustomLayout) return;
    int layoutIndex = static_cast<int>(speakerLayoutParam->load());
    currentLayout = SpeakerPresets::getPreset(layoutIndex);
    lastSpeakerLayoutIndex = layoutIndex;
    applyLayout(currentLayout);
}
```

---

### Task 2: Fix spatialBuffer not resized on dynamic layout change

**Files:** `Source/PluginProcessor.cpp` (processBlock, ~line 405)
**Depends on:** None
**Risk found in research:** MEDIUM

**Problem:** The guard condition `if (spatialChannels > spatialBuffer.getNumChannels())` can never be true because `spatialChannels` is already `min(layoutNumSpeakers, spatialBuffer.getNumChannels())`. If the speaker editor adds speakers beyond the buffer's channel count, spatial rendering is silently clamped.

**Fix:** Check `layoutNumSpeakers` directly against buffer size, then set `spatialChannels` to the actual speaker count:

```cpp
if (layoutNumSpeakers > spatialBuffer.getNumChannels())
    spatialBuffer.setSize(layoutNumSpeakers, numSamples, false, false, true);
int spatialChannels = layoutNumSpeakers;
```

---

## Phase 4.2: Factory Presets (3 tasks)

### Task 3: Add FactoryPreset struct and preset data

**Files:** `Source/PluginProcessor.h`, `Source/PluginProcessor.cpp`
**Depends on:** None

Add to header:
- `struct FactoryPreset` with `juce::String name` and `std::vector<std::pair<juce::String, float>> values`
- `static const std::vector<FactoryPreset>& getFactoryPresets()`
- `int currentProgramIndex = 0` member variable

Add to cpp:
- Static function returning 12 factory presets with parameter values from RESEARCH.md Section 3.3:

**Stereo (5):** Slow Orbit, Fast Spiral, Pendulum Swing, Ambient Drift, Tempo Quarter
**Surround (3):** 5.1 Orbit, 7.1.4 Height Sweep, Quad Drift
**Creative (4):** L+R Split Wide, Deep Space, Tight Focus, Rhythmic Bounce

Parameter IDs: `path`, `speed`, `width`, `depth`, `tilt`, `phase`, `elev_enable`, `elev_range`, `tempo_sync`, `speaker_layout`, `distance`, `air_absorption`, `atten_curve`, `center_diverge`, `source_mode`, `lr_offset`, `mix`

---

### Task 4: Implement Programs API methods

**Files:** `Source/PluginProcessor.cpp` (~lines 139-143)
**Depends on:** Task 3

Replace stub implementations:
- `getNumPrograms()` → return `(int) getFactoryPresets().size()`
- `getCurrentProgram()` → return `currentProgramIndex`
- `setCurrentProgram(int index)` → validate index, iterate preset values, call `param->setValueNotifyingHost(param->convertTo0to1(value))` for each parameter
- `getProgramName(int index)` → return preset name

---

### Task 5: Verify preset parameter IDs match APVTS

**Files:** `Source/PluginProcessor.cpp` (createParameterLayout)
**Depends on:** Tasks 3, 4

Cross-reference the parameter IDs used in factory presets against the actual IDs in `createParameterLayout()`. Ensure:
- All 17 parameter IDs used in presets exist in APVTS
- Value ranges in presets are within declared parameter ranges
- Surround presets' speaker_layout values (0-7) match the parameter's choice index

---

## Phase 4.3: Validation (5 tasks)

### Task 6: Build Release targets

**Files:** None (build system)
**Depends on:** Tasks 1, 2, 4

```bash
cd /Users/taylorbrook/Dev/VST-development/build
cmake --build . --target OuariconOrbit_VST3 OuariconOrbit_AU --config Release
```

Or with ninja:
```bash
ninja OuariconOrbit_VST3 OuariconOrbit_AU
```

Verify zero warnings from O-Orbit source files.

---

### Task 7: Run pluginval (VST3, strictness 10)

**Files:** None (validation)
**Depends on:** Task 6

```bash
/Applications/pluginval.app/Contents/MacOS/pluginval \
    --validate "build/plugins/O-Orbit/OuariconOrbit_artefacts/Release/VST3/O-Orbit.vst3" \
    --skip-gui-tests --strictness-level 10 --timeout-ms 180000
```

If failures occur:
- Analyze failure output
- Fix issues in source
- Rebuild and re-run

---

### Task 8: Run auval (AU validation)

**Files:** None (validation)
**Depends on:** Task 6

```bash
# Clear AU cache and install
killall -9 AudioComponentRegistrar 2>/dev/null || true
rm -rf ~/Library/Caches/AudioUnitCache/
rm -rf ~/Library/Caches/com.apple.audiounits.cache
rm -rf ~/Library/Audio/Plug-Ins/Components/O-Orbit.component
cp -R build/plugins/O-Orbit/OuariconOrbit_artefacts/Release/AU/O-Orbit.component ~/Library/Audio/Plug-Ins/Components/

# Run auval
auval -v aufx OuOr Ouar
```

If failures occur:
- Analyze auval output
- Fix issues
- Rebuild and re-run

---

### Task 9: Install to system folders

**Files:** None (installation)
**Depends on:** Tasks 7, 8

```bash
killall -9 AudioComponentRegistrar 2>/dev/null || true
rm -rf ~/Library/Caches/AudioUnitCache/
rm -rf ~/Library/Caches/com.apple.audiounits.cache
rm -rf ~/Library/Audio/Plug-Ins/VST3/O-Orbit.vst3
rm -rf ~/Library/Audio/Plug-Ins/Components/O-Orbit.component
cp -R build/plugins/O-Orbit/OuariconOrbit_artefacts/Release/VST3/O-Orbit.vst3 ~/Library/Audio/Plug-Ins/VST3/
cp -R build/plugins/O-Orbit/OuariconOrbit_artefacts/Release/AU/O-Orbit.component ~/Library/Audio/Plug-Ins/Components/
```

---

### Task 10: Verify state persistence round-trip

**Files:** None (manual verification)
**Depends on:** Task 9

Test in standalone:
1. Change all 17 parameters to non-default values
2. Switch to a custom speaker layout (add/move speakers)
3. Save state
4. Close and reopen
5. Verify all parameters and custom layout restored correctly

---

## Success Criteria

- [ ] handleAsyncUpdate() respects useCustomLayout flag (Task 1)
- [ ] spatialBuffer dynamically resizes for larger layouts (Task 2)
- [ ] 12 factory presets accessible via Programs API (Tasks 3-4)
- [ ] Preset parameter IDs and values validated (Task 5)
- [ ] Zero O-Orbit source warnings (Task 6)
- [ ] pluginval passes strictness level 10 (Task 7)
- [ ] auval passes AU validation (Task 8)
- [ ] Plugin installed to system folders (Task 9)
- [ ] State persistence round-trip works (Task 10)

---

## Files to Modify

| File | Changes |
|------|---------|
| `Source/PluginProcessor.h` | Add FactoryPreset struct, getFactoryPresets(), currentProgramIndex member |
| `Source/PluginProcessor.cpp` | Fix handleAsyncUpdate(), fix spatialBuffer sizing, implement 12 presets + Programs API |

**No new files needed. No UI changes.**

---

*Plan created: 2026-02-11*
