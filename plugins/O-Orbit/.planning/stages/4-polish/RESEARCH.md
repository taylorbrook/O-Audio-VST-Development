# Stage 4: Polish - Research

**Date:** 2026-02-11
**Plugin:** O-Orbit
**Stage:** 4 - Polish (Final)

---

## Research Summary

Comprehensive code review and validation research for O-Orbit Stage 4 (Polish). Plugin is in excellent shape from Stages 1-3: all 17 parameters bound, VBAP renderer working with SAF, WebView UI with orbital visualizer and speaker editor, custom layout persistence, auto-downmix. Zero warnings.

Key areas investigated:
1. pluginval validation concerns (multi-channel, WebView, threads)
2. AU validation (auval) for multi-channel effects
3. Factory presets via AudioProcessor Programs API
4. State persistence verification
5. Code review for potential runtime issues

---

## 1. pluginval Validation Analysis

### 1.1 Running pluginval

```bash
# Strictness level 10 (maximum), skip GUI tests for WebView plugins
/Applications/pluginval.app/Contents/MacOS/pluginval \
    --validate "build/plugins/O-Orbit/OuariconOrbit_artefacts/Release/VST3/O-Orbit.vst3" \
    --skip-gui-tests --strictness-level 10 --timeout-ms 180000

# With GUI tests (may be unstable with WebView)
/Applications/pluginval.app/Contents/MacOS/pluginval \
    --validate "build/plugins/O-Orbit/OuariconOrbit_artefacts/Release/VST3/O-Orbit.vst3" \
    --strictness-level 10 --timeout-ms 600000
```

### 1.2 Multi-Channel Bus Concerns

**Issue:** O-Orbit's `isBusesLayoutSupported()` accepts any output from 2-24 channels. pluginval tests bus layouts extensively at strictness 10.

**Current implementation (PluginProcessor.cpp:151-167):**
```cpp
bool OOrbitProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    auto mainInput  = layouts.getMainInputChannelSet();
    auto mainOutput = layouts.getMainOutputChannelSet();
    if (mainInput.isDisabled() || mainOutput.isDisabled()) return false;
    if (mainInput != juce::AudioChannelSet::mono()
        && mainInput != juce::AudioChannelSet::stereo()) return false;
    int numOut = mainOutput.size();
    return numOut >= 2 && numOut <= 24;
}
```

**Potential Issues:**
- pluginval may test with unusual channel sets (e.g., `AudioChannelSet::ambisonic(3)`) that have many channels
- The plugin uses `mainOutput.size()` which returns channel count regardless of the set type — this is correct
- **Risk: LOW** — the implementation is permissive and should handle all valid channel sets

**Recommended Fix:** None needed. The flexible approach is intentional for this plugin.

### 1.3 WebView + pluginval

**Key concern:** pluginval creates/destroys editors rapidly to test for leaks and crashes.

**Analysis of current editor:**
- Timer stopped in destructor before member destruction (correct)
- All members are unique_ptrs (correct — no manual cleanup needed)
- Destruction order: Attachments → WebView → Relays (declared correctly)
- `hasNavigated` flag prevents re-navigation (but flag is per-instance, so fresh editors navigate fresh — correct)
- `withKeepPageLoadedWhenBrowserIsHidden()` may cause issues during rapid destroy — investigate

**Recommendation:** Use `--skip-gui-tests` for initial validation. If full GUI tests are needed, the destruction order and timer cleanup look solid.

### 1.4 Background Thread (VBAPComputeThread)

**Concern:** pluginval calls `prepareToPlay()` multiple times. Each call starts the VBAP compute thread if not running.

**Current behavior (PluginProcessor.cpp:270-271):**
```cpp
if (! vbapThread.isThreadRunning())
    vbapThread.startThread (juce::Thread::Priority::normal);
```

**Thread destruction (~VBAPComputeThread):**
```cpp
~VBAPComputeThread() override {
    signalThreadShouldExit();
    notify();
    stopThread (2000);
}
```

**Analysis:** The thread is a member of the processor, so it's destroyed when the processor is destroyed. The `isThreadRunning()` guard prevents multiple starts. The destructor properly signals and waits. **Risk: LOW.**

**Potential issue:** If `prepareToPlay()` is called, then `releaseResources()`, then `prepareToPlay()` again — the thread stays running the whole time. This is fine since the thread uses `wait()` when idle.

### 1.5 AsyncUpdater Concern

**Issue:** `OOrbitProcessor` inherits `juce::AsyncUpdater`. The `handleAsyncUpdate()` method is called on the message thread and modifies `currentLayout`, `useCustomLayout`, etc.

**Concern:** During pluginval's rapid create/destroy cycle, an async update could fire after partial destruction. However, `AsyncUpdater` is cleaned up in the base class destructor, and `handleAsyncUpdate()` only reads parameter values and modifies member variables — no external resources.

**Risk: LOW.** The callback accesses `speakerLayoutParam` (a raw pointer to APVTS parameter data) which is valid as long as the processor exists. APVTS is declared before the async updater base class, so it's destroyed after.

### 1.6 VBAPDataExchange Timer

**Concern:** `VBAPDataExchange` inherits `juce::Timer` and starts a 10Hz timer in its constructor.

**Analysis:** The timer is stopped in the destructor (`stopTimer()`). Since `VBAPDataExchange` is a member of the processor, it's destroyed when the processor is destroyed. The timer callback only resets a `unique_ptr` — safe.

**Risk: LOW.**

### 1.7 State Save/Restore

pluginval at strictness 10 tests state save/restore round-trips. The current implementation:
- `getStateInformation()`: Serializes APVTS + custom speaker layout as XML
- `setStateInformation()`: Parses XML, extracts custom layout, restores APVTS

**Potential issue:** After `setStateInformation()`, if `useCustomLayout` is true, `applyLayout(customLayout)` is called. This calls `vbapThread.requestRecomputation()` and `downmixEngine.prepare()`. If this happens before `prepareToPlay()`, the downmix engine may have stale output channel counts.

**Fix consideration:** Guard `applyLayout()` to only trigger VBAP recomputation if `prepareToPlay()` has been called (i.e., check if sample rate > 0 or similar).

**Risk: MEDIUM** — This may or may not trigger depending on pluginval's call order.

---

## 2. AU Validation (auval)

### 2.1 Running auval

```bash
# Full validation
auval -v aufx OuOr Ouar

# Quick check — verify AU registration
auval -a | grep -i orbit
```

### 2.2 Multi-Channel AU Concerns

**AU format identification:**
- Type: `aufx` (audio effect)
- Plugin code: `OuOr`
- Manufacturer: `Ouar`

**Channel negotiation:** AU uses `isBusesLayoutSupported()` same as VST3. auval tests common configurations (mono→stereo, stereo→stereo, etc.).

**Potential auval failures:**
- auval tests processing with silence → expects silence out. O-Orbit's motion engine generates position changes even with no input, but with zero-input the output should be zero (VBAP gains applied to zero signal = zero). **OK.**
- auval tests parameter ranges — all 17 parameters have defined ranges. **OK.**
- auval tests state persistence round-trip. Same concerns as pluginval. **OK.**

### 2.3 AU Cache Clearing

Per CLAUDE.md requirements, always clear AU cache after building:
```bash
killall -9 AudioComponentRegistrar 2>/dev/null || true
rm -rf ~/Library/Caches/AudioUnitCache/
rm -rf ~/Library/Caches/com.apple.audiounits.cache
```

---

## 3. Factory Presets

### 3.1 Approach Decision: Programs API vs OuariconPresetManager

**Option A: AudioProcessor Programs API (getNumPrograms/setCurrentProgram)**
- Simplest approach — presets are compiled into the binary
- No file I/O, no directory management
- Works with all DAW preset browsers natively
- Limited: no user preset saving, no categories, no import/export
- **Best for O-Orbit**: the CONTEXT.md specifies "Factory presets implemented via AudioProcessor programs API"

**Option B: OuariconPresetManager module**
- Full preset management with factory/user directories
- JSON serialization, file I/O, navigation
- Requires UI integration (preset bar in WebView)
- Overkill for the specified scope (8-12 factory presets)

**Decision: Option A (Programs API)** — matches the discuss phase decision and keeps the implementation simple.

### 3.2 Programs API Implementation Pattern

```cpp
// In PluginProcessor.h
struct FactoryPreset {
    juce::String name;
    std::map<juce::String, float> values;
};

static const std::vector<FactoryPreset>& getFactoryPresets();

// In PluginProcessor.cpp
int getNumPrograms() override { return (int) getFactoryPresets().size(); }
int getCurrentProgram() override { return currentProgramIndex; }
void setCurrentProgram(int index) override {
    if (index < 0 || index >= getNumPrograms()) return;
    currentProgramIndex = index;
    const auto& preset = getFactoryPresets()[(size_t)index];
    for (const auto& [paramId, value] : preset.values) {
        if (auto* param = parameters.getParameter(paramId))
            param->setValueNotifyingHost(param->convertTo0to1(value));
    }
}
const juce::String getProgramName(int index) override {
    if (index < 0 || index >= getNumPrograms()) return {};
    return getFactoryPresets()[(size_t)index].name;
}
```

### 3.3 Factory Preset Values

**12 presets across stereo, surround, and creative categories:**

#### Stereo Presets (work in any DAW)

| Preset | Path | Speed | Width | Depth | Tilt | Phase | Elev | Sync | Dist | Air | Atten | Diverge | Src | LR Off | Mix |
|--------|------|-------|-------|-------|------|-------|------|------|------|-----|-------|---------|-----|--------|-----|
| **Slow Orbit** | Orbit(0) | 0.5 | 180 | 0 | 0 | 0 | Off | Off | 1.0 | 30 | Inv(1) | 0 | Mono(0) | 180 | 100 |
| **Fast Spiral** | Orbit(0) | 4.0 | 360 | 50 | 15 | 0 | Off | Off | 2.0 | 40 | Inv(1) | 0 | Mono(0) | 180 | 100 |
| **Pendulum Swing** | Pend(1) | 1.0 | 120 | 0 | 0 | 0 | Off | Off | 1.0 | 20 | Lin(0) | 0 | Mono(0) | 180 | 100 |
| **Ambient Drift** | Drift(3) | 0.3 | 90 | 30 | 0 | 0 | Off | Off | 3.0 | 60 | InvSq(2) | 0 | Mono(0) | 180 | 80 |
| **Tempo Quarter** | Orbit(0) | 1.0 | 180 | 0 | 0 | 0 | Off | 1/4(8) | 1.0 | 25 | Inv(1) | 0 | Mono(0) | 180 | 100 |

#### Surround Presets (for multi-channel setups)

| Preset | Path | Speed | Width | Depth | Tilt | Phase | Elev | ElevRange | Sync | Layout | Dist | Air | Atten | Diverge | Src | LR Off | Mix |
|--------|------|-------|-------|-------|------|-------|------|-----------|------|--------|------|-----|-------|---------|-----|--------|-----|
| **5.1 Orbit** | Orbit(0) | 0.8 | 360 | 20 | 0 | 0 | Off | 45 | Off | 5.1(2) | 2.0 | 35 | Inv(1) | 20 | Mono(0) | 180 | 100 |
| **7.1.4 Height** | Lin(2) | 0.3 | 180 | 40 | 45 | 0 | On | 60 | Off | 7.1.4(5) | 3.0 | 50 | Inv(1) | 10 | Mono(0) | 180 | 100 |
| **Quad Drift** | Drift(3) | 0.5 | 120 | 25 | 0 | 0 | Off | 45 | Off | Quad(1) | 2.0 | 40 | InvSq(2) | 0 | Mono(0) | 180 | 90 |

#### Creative Presets

| Preset | Path | Speed | Width | Depth | Tilt | Phase | Elev | Sync | Dist | Air | Atten | Diverge | Src | LR Off | Mix |
|--------|------|-------|-------|-------|------|-------|------|------|------|-----|-------|---------|-----|--------|-----|
| **L+R Split Wide** | Orbit(0) | 0.8 | 360 | 0 | 0 | 0 | Off | Off | 1.5 | 30 | Inv(1) | 0 | Split(1) | 180 | 100 |
| **Deep Space** | Orbit(0) | 0.15 | 360 | 80 | 10 | 0 | Off | Off | 20.0 | 90 | InvSq(2) | 0 | Mono(0) | 180 | 100 |
| **Tight Focus** | Orbit(0) | 1.5 | 30 | 10 | 0 | 0 | Off | Off | 0.5 | 10 | Lin(0) | 50 | Mono(0) | 180 | 100 |
| **Rhythmic Bounce** | Pend(1) | 2.0 | 150 | 30 | 0 | 0 | Off | 1/8(5) | 1.5 | 25 | Inv(1) | 0 | Mono(0) | 180 | 100 |

**Notes on parameter encoding:**
- Path: Orbit=0, Pendulum=1, Linear=2, Drift=3
- Attenuation: Linear=0, Inverse=1, Inverse Square=2
- Tempo Sync: Off=0, 1/16T=1, ..., 1/4=8, ..., 4 Bars=14
- Speaker Layout: Stereo=0, Quad=1, 5.1=2, 7.1=3, 5.1.4=4, 7.1.4=5, Hex=6, Oct=7
- Source Mode: Mono=0, L+R Split=1
- Surround presets set speaker layout but it only takes effect if the DAW supports multi-channel

### 3.4 Preset Application Concerns

**Zipper noise:** When `setCurrentProgram()` changes multiple parameters simultaneously, there should be no zipper noise because:
- The motion engine uses block-based position calculation, not per-sample parameter reads
- VBAP gains are smoothed per-sample (linear ramp between blocks)
- The distance model filter has internal state that ramps naturally
- `SmoothedValue` members handle continuous parameter changes

**Speaker layout presets:** The surround presets set `speaker_layout` via the APVTS parameter, which triggers `handleAsyncUpdate()` on the message thread. This is the same path as the dropdown in the UI — safe.

---

## 4. State Persistence Verification

### 4.1 Current Implementation Review

**getStateInformation (PluginProcessor.cpp:517-540):**
- Copies APVTS state to XML
- If `useCustomLayout` is true, appends `<CustomLayout>` XML child with all speaker data
- Serializes to binary via `copyXmlToBinary`

**setStateInformation (PluginProcessor.cpp:542-581):**
- Parses binary to XML
- Extracts and removes `<CustomLayout>` child before restoring APVTS
- Restores custom layout and applies it
- Calls `parameters.replaceState()` to restore APVTS

**Potential issue:** The order of operations is:
1. Extract custom layout from XML
2. Set `useCustomLayout = true`
3. Remove custom layout XML from tree
4. `parameters.replaceState()` — this may trigger parameter listeners
5. `applyLayout(customLayout)` — VBAP recomputation

Step 4 triggers the speaker layout parameter listener, which calls `triggerAsyncUpdate()`. When `handleAsyncUpdate()` fires (async on message thread), it sets `useCustomLayout = false` and loads a preset layout — **overwriting the custom layout!**

**This is a bug.** The `handleAsyncUpdate()` checks `if (layoutIndex != lastSpeakerLayoutIndex)` in processBlock, but `handleAsyncUpdate()` unconditionally sets `useCustomLayout = false`.

**Fix:** In `handleAsyncUpdate()`, check if `useCustomLayout` is true and skip the preset load:
```cpp
void OOrbitProcessor::handleAsyncUpdate()
{
    if (useCustomLayout) return; // Custom layout takes precedence
    int layoutIndex = static_cast<int>(speakerLayoutParam->load());
    currentLayout = SpeakerPresets::getPreset(layoutIndex);
    lastSpeakerLayoutIndex = layoutIndex;
    applyLayout(currentLayout);
}
```

**Risk: HIGH** — This could cause custom layouts to be lost after state restore. Must fix.

### 4.2 Round-Trip Test Plan

1. Set parameters to non-default values
2. Create a custom speaker layout (add/move speakers)
3. Save state (via DAW project save)
4. Load state (via DAW project load)
5. Verify: all 17 parameters match, custom layout preserved, VBAP rendering correct

---

## 5. Code Review Findings

### 5.1 processBlock Safety

**Potential issue — spatialBuffer size mismatch (PluginProcessor.cpp:405-408):**
```cpp
int spatialChannels = std::min(layoutNumSpeakers, (int)spatialBuffer.getNumChannels());
if (spatialChannels > spatialBuffer.getNumChannels())
    spatialBuffer.setSize(spatialChannels, numSamples, false, false, true);
```

The `if` condition `spatialChannels > spatialBuffer.getNumChannels()` can never be true because `spatialChannels` is already `min(layoutNumSpeakers, spatialBuffer.getNumChannels())`. This means if `layoutNumSpeakers` exceeds the buffer channel count, the buffer is NOT resized and spatial channels are silently clamped.

**Scenario:** If `applyLayout()` is called with a 12-speaker layout (7.1.4) after `prepareToPlay()` was called with stereo output (spatialBuffer sized to 2 channels), `spatialChannels` would be 2 — we'd only render to 2 speakers even though VBAP is computing gains for 12.

**In practice:** `prepareToPlay()` allocates `spatialBuffer` to `max(totalNumOutputChannels, layoutNumSpeakers)`, so this would only be an issue if the layout changes to a larger one after prepare. The `applyLayout()` method doesn't resize `spatialBuffer`.

**Fix:** Either resize `spatialBuffer` in `applyLayout()`, or fix the guard in processBlock:
```cpp
if (layoutNumSpeakers > spatialBuffer.getNumChannels())
    spatialBuffer.setSize(layoutNumSpeakers, numSamples, false, false, true);
int spatialChannels = layoutNumSpeakers;
```

**Risk: MEDIUM** — Only matters for dynamic layout changes (speaker editor), which is a feature of this plugin.

### 5.2 dryBuffer.setSize() in processBlock

**Issue (PluginProcessor.cpp:379):**
```cpp
dryBuffer.setSize(totalNumInputChannels, numSamples, false, false, true);
```

`setSize()` with `avoidReallocating=true` is fine for the buffer if the size stays the same or shrinks. However, calling `setSize()` in processBlock at all is a minor concern because:
- If the buffer needs to grow, it allocates memory on the audio thread
- This should never happen since `prepareToPlay()` already sizes it to `(2, samplesPerBlock)` and inputs are always 1 or 2 channels

**Risk: LOW** — The `true` parameter makes this a no-op in normal operation.

### 5.3 AsyncUpdater from Audio Thread

**Issue (PluginProcessor.cpp:328):**
```cpp
if (layoutIndex != lastSpeakerLayoutIndex)
    triggerAsyncUpdate();
```

`triggerAsyncUpdate()` is called from the audio thread. This is documented as safe in JUCE — `AsyncUpdater` uses an atomic flag internally and doesn't allocate. **OK.**

---

## 6. Patterns from Other Plugins

### 6.1 O-GrainScatter (Stage 4 completed)

- Code review approach: identify audio artifacts, fix them
- Found: freeze manager release click (no crossfade), output clipping (no soft-clip)
- These are DSP-specific issues not applicable to O-Orbit

### 6.2 O-Bells (Stage 4 completed)

- Uses OuariconPresetManager with folder-based categories
- Full preset bar UI with categorized dropdown
- More complex preset system than what O-Orbit needs
- **Not applicable** — O-Orbit uses Programs API per discuss phase decision

### 6.3 Validation Command Pattern

From `.claude/agents/validation-agent.md`:
```bash
# Stage 4: Full validation
/Applications/pluginval.app/Contents/MacOS/pluginval \
    --validate "$VST3_PATH" --strictness-level 10 --timeout-ms 600000
```

---

## 7. Issues Summary

### Must Fix (Before pluginval)

| # | Issue | File | Risk | Description |
|---|-------|------|------|-------------|
| 1 | **Custom layout overwritten on state restore** | PluginProcessor.cpp:182-190 | HIGH | `handleAsyncUpdate()` sets `useCustomLayout = false` unconditionally, overwriting custom layout restored from state |
| 2 | **spatialBuffer not resized on layout change** | PluginProcessor.cpp:405-408 | MEDIUM | Guard condition is always false; dynamic layout changes to larger configs won't work correctly |

### Should Fix (Quality)

| # | Issue | File | Risk | Description |
|---|-------|------|------|-------------|
| 3 | **getNumPrograms returns 1** | PluginProcessor.cpp:139 | LOW | No factory presets — implement Programs API with 12 presets |
| 4 | **No currentProgramIndex member** | PluginProcessor.h | LOW | Need to track current preset index |

### Won't Fix (Acceptable)

| # | Issue | File | Risk | Description |
|---|-------|------|------|-------------|
| 5 | `dryBuffer.setSize()` in processBlock | PluginProcessor.cpp:379 | VERY LOW | No-op in normal operation |
| 6 | Timer runs when editor hidden | PluginEditor.cpp:295 | VERY LOW | Minor CPU waste, uses `emitEventIfBrowserIsVisible` |

---

## 8. Implementation Approach

### Phase 4.1: Bug Fixes + Validation
1. Fix handleAsyncUpdate() to respect useCustomLayout flag
2. Fix spatialBuffer sizing for dynamic layout changes
3. Build Release (VST3 + AU)
4. Run pluginval (strictness 10, --skip-gui-tests)
5. Run auval
6. Fix any validation failures

### Phase 4.2: Factory Presets
1. Add FactoryPreset struct and preset data (12 presets)
2. Implement getNumPrograms/setCurrentProgram/getProgramName
3. Add currentProgramIndex member
4. Test preset switching in standalone
5. Rebuild and re-validate

### Phase 4.3: Final Verification
1. Full pluginval (with GUI tests if stable)
2. auval validation
3. State persistence round-trip test
4. Install to system folders
5. Test in Logic Pro (AU) and Reaper (VST3)

---

## 9. Files to Modify

| File | Changes |
|------|---------|
| `Source/PluginProcessor.h` | Add FactoryPreset struct, getFactoryPresets(), currentProgramIndex |
| `Source/PluginProcessor.cpp` | Fix handleAsyncUpdate(), fix spatialBuffer sizing, implement presets |

**No new files needed.** No UI changes (presets are accessible via DAW's own preset browser).

---

*Research completed: 2026-02-11*
