---
phase: O-SimpleReverb-code-review
reviewed: 2026-07-05T00:00:00Z
depth: standard
files_reviewed: 7
files_reviewed_list:
  - plugins/O-SimpleReverb/Source/PluginProcessor.h
  - plugins/O-SimpleReverb/Source/PluginProcessor.cpp
  - plugins/O-SimpleReverb/Source/PluginEditor.h
  - plugins/O-SimpleReverb/Source/PluginEditor.cpp
  - plugins/O-SimpleReverb/Source/ui/public/index.html
  - plugins/O-SimpleReverb/Source/ui/public/modules/preset-manager.js
  - plugins/O-SimpleReverb/CMakeLists.txt
findings:
  critical: 4
  warning: 5
  info: 3
  total: 12
status: issues
---

# O-SimpleReverb v1.5.5: Code Review Report

**Reviewed:** 2026-07-05
**Depth:** standard
**Files Reviewed:** 7
**Status:** issues_found

## Summary

JUCE 8 reverb plugin with WebView UI. Structure is sound: relay/webview/attachment ordering is correct, `ScopedNoDenormals` present, `numSamples == 0` guarded, all JS `getNativeFunction` calls have matching C++ `withNativeFunction` registrations, resource provider correctly compares bare paths, and the CMake config has both `NEEDS_WEBVIEW2 TRUE` and `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` (rare in this repo).

Four Critical findings: two async FileChooser use-after-free paths (known shipped-bug class in this repo), an inverted DECAY skew that makes the parameter, its UI readout, and all 24 factory presets disagree, and two classes of audio-thread heap allocation (`IIR::Coefficients::makeXXX` and unprepared work buffers).

## Critical Issues

### CR-01 — FileChooser `launchAsync` completions capture `this` and `complete` without SafePointer (use-after-free)

**File:** `plugins/O-SimpleReverb/Source/PluginEditor.cpp:97-115` (`savePresetWithDialog`) and `plugins/O-SimpleReverb/Source/PluginEditor.cpp:124-142` (`loadPresetFromFile`)

**Issue:** Both native functions launch a system file dialog with a completion lambda capturing raw `this` and the WebView-owned `complete` callback:

```cpp
fileChooser->launchAsync(..., [this, complete](const juce::FileChooser& fc) {
    ...
    bool success = processorRef.presetManager.savePreset(name);  // `this` may be dead
    complete(juce::var(result));                                 // `complete` owned by dead WebView Impl
});
```

If the user closes the plugin window (or the host destroys the editor) while the dialog is open, the completion can still fire against the destroyed editor: `processorRef` access is a dangling-reference dereference, and invoking `complete` is itself a UAF because the callback's owner (the WebView `Impl`) is gone. This exact class shipped as a real crash and was fixed in O-MicrotonalSampler v1.23.5 (W12); the fix note explicitly calls for auditing all WebView editors.

**Fix:** Capture a `juce::Component::SafePointer` and bail with a bare `return` on the null path — do NOT call `complete(...)` after teardown:

```cpp
juce::Component::SafePointer<OSimpleReverbAudioProcessorEditor> safeThis(this);
fileChooser->launchAsync(flags, [safeThis, complete](const juce::FileChooser& fc) {
    if (safeThis == nullptr)
        return;  // bare return — complete() is owned by the dead WebView
    auto file = fc.getResult();
    ...
});
```

Apply to both `savePresetWithDialog` and `loadPresetFromFile`.

### CR-02 — DECAY skew is inverted: knob center is 1.47x, not 1.0x; UI readout and all 24 factory presets are wrong

**File:** `plugins/O-SimpleReverb/Source/PluginProcessor.cpp:166-172` (parameter), `plugins/O-SimpleReverb/Source/ui/public/index.html:798` (JS formatter), `plugins/O-SimpleReverb/Source/PluginProcessor.cpp:601-706` (factory preset norms)

**Issue:** The comment claims "skew 1.585 puts 1.0 at center", but JUCE's `NormalisableRange::convertFrom0to1` maps `value = min + (max-min) * norm^(1/skew)`. With skew 1.585, normalized 0.5 → `0.5 + 1.5 * 0.5^0.631` = **1.47x**. To put 1.0x at center the skew must be `log(0.5)/log((1.0-0.5)/(2.0-0.5))` ≈ **0.631** (the reciprocal). Three downstream consumers were all authored against the intended (0.631) mapping and therefore disagree with the actual DSP:

1. **JS readout** (index.html:798): `0.5 + 1.5 * Math.pow(norm, 1.585)` — this is the *intended* curve, so the UI shows 1.0x at center while the processor actually applies 1.47x. The generic DAW parameter view shows 1.47 while the plugin UI shows 1.0.
2. **Factory presets**: e.g. "Booth - Drum Close" stores `DECAY 0.20` — intended 0.62x (tight), actual `0.5 + 1.5*0.2^0.631` ≈ **1.04x** (neutral). Every "short decay" factory preset actually has medium-to-long decay; the presets audibly do not match their names.
3. **Header comment** (line 166) documents the wrong behavior.

This is the exact `pattern_webview_knob_readout_scaled_value` defect class (O-MicrotonalSampler read 2x wrong for ~20 versions).

**Fix:** Change the skew to 0.631:

```cpp
juce::NormalisableRange<float>(0.5f, 2.0f, 0.01f, 0.6309f),  // puts 1.0 at knob center
```

With that change the existing JS formatter (`norm^1.585`) and the factory preset norms become exactly correct — no other edits needed. More robustly, also switch the JS readout to `state.getScaledValue()` so it can never drift from the C++ range again. Note: changing skew does not break saved sessions (normalized values are reinterpreted), but existing user presets saved under v1.5.5 will load with a different audible decay — flag in CHANGELOG.

### CR-03 — Audio-thread heap allocation: `IIR::Coefficients::makeXXX` called in processBlock paths

**File:** `plugins/O-SimpleReverb/Source/PluginProcessor.cpp:528, 539, 549` (character/LP filters, directly in `processBlock`) and `plugins/O-SimpleReverb/Source/PluginProcessor.cpp:312-335` (`updateTypeSpecificDSP`, called from `processBlock` line 381 on type change)

**Issue:** `juce::dsp::IIR::Coefficients<float>::makeLowPass/makeHighShelf/makeHighPass/makePeakFilter/makeAllPass/makeLowShelf` each heap-allocate a ref-counted `Coefficients` object, and the temporary is also freed on the audio thread. These run whenever CHARACTER moves > 0.1, LPFREQ moves > 0.5 Hz (i.e., continuously during automation/knob drags), or TYPE changes. Allocation on the audio thread can block on the allocator lock and cause dropouts under load. This repo's established RT-safe fix is `ArrayCoefficients` (see `pattern_arraycoefficients_rt_safe_iir`, shipped in O-Formant v1.25.1).

**Fix:** Replace with the stack-based equivalents, assigning coefficients in place:

```cpp
auto coeffs = juce::dsp::IIR::ArrayCoefficients<float>::makeLowPass(currentSampleRate, cutoffHz);
characterFilter.state->coefficients = juce::Array<float>(coeffs.data(), (int) coeffs.size());
```

(Identical math, no allocation.) Apply to all six `makeXXX` call sites reachable from `processBlock`. The `prepareToPlay`-only sites (lines 180, 257) may stay as-is.

### CR-04 — dryBuffer/wetBuffer are never pre-allocated; `setSize` allocates inside processBlock

**File:** `plugins/O-SimpleReverb/Source/PluginProcessor.cpp:404, 422` (allocation sites), `plugins/O-SimpleReverb/Source/PluginProcessor.cpp:235-278` (`prepareToPlay` — missing pre-allocation)

**Issue:** The header comment (PluginProcessor.h:123) claims "Pre-allocated buffers (avoids audio-thread allocation)", but `prepareToPlay` never sizes `dryBuffer` or `wetBuffer`. The `setSize(..., avoidReallocating=true)` calls in `processBlock` therefore heap-allocate **guaranteed on the first audio callback** of every session (buffers start empty), and again any time the host delivers a larger block than previously seen (variable-block hosts like FL Studio, and hosts that probe with small blocks then run large ones, do this routinely).

**Fix:** Pre-allocate in `prepareToPlay`:

```cpp
dryBuffer.setSize(getTotalNumOutputChannels(), samplesPerBlock);
wetBuffer.setSize(getTotalNumOutputChannels(), samplesPerBlock);
```

The existing `setSize(..., true)` calls in `processBlock` then become no-op shrink/reuse for conforming hosts. Optionally clamp per-block processing to the prepared size to be fully robust against hosts that exceed `samplesPerBlock`.

## Warnings

### WR-01 — `applyPresetJson` does not reset parameters to defaults before applying (stale-state inheritance)

**File:** `modules/persistence/preset-manager/cpp/OuariconPresetManager.h:274-306` (shared module compiled into this plugin via `ouaricon_add_module`, CMakeLists.txt:57)

**Issue:** `applyPresetJson` only sets parameters present in the JSON. Any preset missing a key silently inherits the current live value for that parameter. Concrete scenario for this plugin: a user preset saved by a pre-LP-filter build (before LPFREQ/LPON existed) loads with whatever LP state is currently active — load it after an "Ambient - Pad Wash" (LPON=1) and the old preset unexpectedly has the low-cut engaged. This is the known `pattern_preset_apply_needs_reset_to_defaults` defect (fixed as WR-08 in O-Polystutter v1.12.3, flagged as a candidate for all OuariconPresetManager plugins).

**Fix:** In `applyPresetJson`, before the restore loop:

```cpp
for (auto* p : parameters.processor.getParameters())
    if (auto* rp = dynamic_cast<juce::RangedAudioParameter*>(p))
        rp->setValueNotifyingHost(rp->getDefaultValue());
```

Fix belongs in the shared module (version bump) and syncs into this plugin.

### WR-02 — CHARACTER UI label zones (±50) don't match DSP engagement threshold (±0.5): audible filtering while UI reads "neutral"

**File:** `plugins/O-SimpleReverb/Source/PluginProcessor.cpp:509-514` (DSP thresholds) vs `plugins/O-SimpleReverb/Source/ui/public/index.html:790-795` (JS formatter)

**Issue:** The DSP switches to Warm/Bright at `characterValue < -0.5f` / `> 0.5f` (i.e., essentially any deviation from exact 0 on a -100..+100 range), but the UI formatter labels the knob "neutral" for the entire middle half of the range (normalized 25%–75% = actual -50..+50). At `CHARACTER = -50` the DSP applies a low-pass at ~11.1 kHz (`warmValue = 50/99 = 0.505 → cutoff = 2000 + 18000*0.505`); at `+50` a +3 dB high shelf at 4 kHz. Both are clearly audible while the knob reads "neutral". The user cannot tell from the UI whether the character filter is active.

**Fix:** Align the boundaries — either widen the DSP neutral dead zone to ±50 to match the labels, or (better, preserving the continuous sweep) change the formatter zones to match the DSP (`percent < 49.75 → "warm"`, `> 50.25 → "bright"`), or show a continuous value (e.g. "warm 62%") instead of three coarse labels.

### WR-03 — Wet/dry gains applied per-block without smoothing (zipper noise)

**File:** `plugins/O-SimpleReverb/Source/PluginProcessor.cpp:400-401, 556-564`

**Issue:** `wetGain`/`dryGain` are raw atomic loads applied as constant gains for the whole block. Dragging the Wet or Dry knob (or host automation) produces stepped gain changes at block-rate — audible zipper noise on sustained material, which a reverb is by definition processing. (The internal `juce::dsp::Reverb` smooths its own levels, but wet/dry mixing here is manual and unsmoothed.)

**Fix:** Use `juce::SmoothedValue<float>` for both gains — `reset(sampleRate, 0.02)` in `prepareToPlay`, `setTargetValue()` per block, `getNextValue()` per sample in the mix loop (or `applyGainRamp` with previous/current block values as a cheaper approximation).

### WR-04 — Factory presets rewritten to disk on every processor construction

**File:** `plugins/O-SimpleReverb/Source/PluginProcessor.cpp:219` (constructor call), `modules/persistence/preset-manager/cpp/OuariconPresetManager.h:543-576`

**Issue:** `initializeFactoryPresets()` is called unconditionally in the processor constructor and unconditionally writes all 24 factory `.json` files via `replaceWithText`. The module docstring says "Call this once at plugin startup **if factory presets don't exist**" — there is no existence check on either side. Every instantiation (including every `auval`/pluginval scan pass, and every instance added to a session) performs 24 synchronous file writes on the message thread. This directly defeats the module's own v1.5.0 design note ("Directory creation deferred to first use ... to avoid file I/O during AU validation", line 216-217). Two instances constructing concurrently also race on the same files.

**Fix:** Guard the call: `if (!presetManager.getFactoryPresetsDirectory().exists()) initializeFactoryPresets();` — or better, add a version-stamped sentinel check inside the module so factory presets are only rewritten when the plugin version changes.

### WR-05 — No `isBusesLayoutSupported` override: host can negotiate >2-channel layouts the DSP silently drops

**File:** `plugins/O-SimpleReverb/Source/PluginProcessor.h:18-184` (override absent)

**Issue:** `AudioProcessor::isBusesLayoutSupported` defaults to `return true`, so hosts with multichannel tracks (Logic/Reaper surround) may run the plugin at 4/6 channels. The processing chain only ever writes wet content to channels 0/1 (`processBlock` line 483-485) and `juce::dsp::Reverb` handles at most 2 channels — channels ≥ 2 receive dry-only output at `dryGain`, i.e. the reverb silently does nothing on those channels and `wetBuffer.setSize` grows allocations for channels that are never used.

**Fix:** Constrain layouts explicitly:

```cpp
bool isBusesLayoutSupported(const BusesLayout& layouts) const override
{
    auto out = layouts.getMainOutputChannelSet();
    return (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo())
        && layouts.getMainInputChannelSet() == out;
}
```

## Info

### IN-01 — `preset-manager.js` is dead code; preset UI logic duplicated inline; three native functions registered with no UI consumer

**File:** `plugins/O-SimpleReverb/Source/ui/public/modules/preset-manager.js` (entire file), `plugins/O-SimpleReverb/Source/ui/public/index.html:595-767`, `plugins/O-SimpleReverb/CMakeLists.txt:60-67`, `plugins/O-SimpleReverb/Source/PluginEditor.cpp:48-89`

**Issue:** The module's JS file is copied into the plugin tree but is never imported by index.html, never embedded by `juce_add_binary_data`, and never served by the resource provider — index.html reimplements the preset bar inline instead. Consequences: (a) the file will silently drift from the shared module's fixes (the shared C++ side already carries WR-04/IN-02/IN-03 fixes; the JS class is bypassed entirely); (b) the C++ side registers `savePreset`, `deletePreset`, and `isFactoryPreset` native functions that no UI code path calls — notably there is **no way to delete a user preset from the UI** despite the backend supporting it.

**Fix:** Either wire index.html to use the `PresetManager` class from the module (and embed/serve the JS), or remove the dead file and the three unused native-function registrations. If delete-preset is wanted, add the UI affordance.

### IN-02 — "LP Filter" parameter names describe a low-pass but the DSP is a high-pass (low cut)

**File:** `plugins/O-SimpleReverb/Source/PluginProcessor.cpp:182-196` (param names "LP Filter Freq"/"LP Filter On"), `plugins/O-SimpleReverb/Source/PluginProcessor.cpp:257, 549` (`makeHighPass`)

**Issue:** LPFREQ/LPON and the host-visible names "LP Filter Freq"/"LP Filter On" say low-pass, but the filter is `makeHighPass` (a 20–400 Hz low cut — the UI correctly labels it "Low Cut"). In DAW automation lanes and generic editors the parameter name actively misleads. Parameter IDs can't be renamed without breaking sessions, but the display names can.

**Fix:** Change the display names to "Low Cut Freq" / "Low Cut On" (keep the `LPFREQ`/`LPON` IDs for session compatibility) and correct the source comments (PluginProcessor.h:115 also says "Lowpass filter for wet signal").

### IN-03 — WebView2 user-data folder is the shared temp root, not a plugin-scoped subfolder

**File:** `plugins/O-SimpleReverb/Source/PluginEditor.cpp:33-36`

**Issue:** `withUserDataFolder` is set to the bare OS temp directory. Repo convention (and the WebView2 pattern note) is a plugin-specific child, e.g. `tempDirectory.getChildFile("O-SimpleReverb_WebView")`. Sharing one UDF across multiple Ouaricon plugins (and potentially different WebView2 runtime versions) in the same host process risks profile-lock contention and cache corruption on Windows.

**Fix:**

```cpp
.withUserDataFolder(juce::File::getSpecialLocation(juce::File::tempDirectory)
                        .getChildFile("O-SimpleReverb_WebView"))
```

---

## Verified clean (no finding)

- JS↔C++ native-function bridge: all 7 functions used by index.html are registered; no gaps.
- Resource provider compares bare paths (`url == "/"`) — correct, no scheme stripping.
- CMake: `NEEDS_WEBVIEW2 TRUE` + `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` both present; single `BinaryData` namespace (module system adds no second binary-data target).
- Editor teardown order: `stopTimer()` before member destruction; relay → webview → attachment declaration order correct.
- Shared `OuariconPresetManager.h` carries the v1.0.x fixes: slash-in-name sanitization (WR-04), real version metadata (IN-02), prev/next anchor after out-of-list load (IN-03).
- `ScopedNoDenormals`, zero-sample guard, mono-buffer guards, `jlimit` on type index all present.

_Reviewed: 2026-07-05_
_Reviewer: Claude (gsd-code-reviewer)_
_Depth: standard_
