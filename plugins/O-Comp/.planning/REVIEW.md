---
phase: O-Comp-code-review
reviewed: 2026-07-01T05:37:26Z
depth: deep
files_reviewed: 7
files_reviewed_list:
  - plugins/O-Comp/Source/PluginProcessor.cpp
  - plugins/O-Comp/Source/PluginProcessor.h
  - plugins/O-Comp/Source/PluginEditor.cpp
  - plugins/O-Comp/Source/PluginEditor.h
  - plugins/O-Comp/Source/OuariconPresetManager.h
  - plugins/O-Comp/Source/ui/public/index.html
  - plugins/O-Comp/Source/ui/public/modules/preset-manager.js
findings:
  critical: 1
  warning: 3
  info: 4
  total: 8
status: issues_found
---

# O-Comp: Code Review Report

**Reviewed:** 2026-07-01T05:37:26Z
**Depth:** deep
**Files Reviewed:** 7
**Status:** issues_found

## Summary

Reviewed the O-Comp compressor (v1.4.x) DSP path, WebView↔C++ bridge, and preset
manager at deep depth. The WebView bridge is sound: all 10 preset native functions
have matching `getNativeFunction`/`withNativeFunction` pairs, all 6 slider relays +
1 toggle relay are wired with the correct 3-arg attachments, `getResource` handles
bare paths correctly (no scheme stripping) and covers every referenced asset. The
auto-gain "overcompensation" fix (the `* 0.5f` factor at line 224) is present and
mathematically safe (ratio ≥ 1 so no divide-by-zero there).

The one real defect that matters: a **divide-by-zero in the soft-knee gain formula
that produces NaN in the audio buffer** when `knee == 0` — and `knee == 0` is a
shipped factory preset ("Parallel Crush"). That is the headline finding. Secondary
findings are a channel-pointer bound mismatch (latent OOB if a non-stereo layout is
ever negotiated, since there is no `isBusesLayoutSupported` guard) and per-block
coefficient/gain stepping. The remaining items are quality/robustness.

## Critical Issues

### CR-01: Divide-by-zero → NaN in soft-knee gain formula when `knee == 0`

**File:** `plugins/O-Comp/Source/PluginProcessor.cpp:329-330`
**Issue:**
`calculateGainReduction` divides by `2.0f * kneeDB` in the "inside knee" branch:

```cpp
float kneeInput = x + kneeDB / 2.0f;
return (1.0f - 1.0f / ratio) * (kneeInput * kneeInput) / (2.0f * kneeDB);
```

When `kneeDB == 0.0f`, the two guard branches are `x < 0` and `x > 0`, so the else
branch is entered whenever `x == 0.0f` (envelope level exactly equals threshold).
The formula then evaluates `(...) * 0 / 0` → **NaN**. That NaN flows straight into
the audio path: `gainLinear = decibelsToGain(-NaN) * makeup` (line 270) →
`channelPtrs[ch][sample] *= gainLinear` (line 276) writes NaN samples to the output
buffer, which many hosts propagate/latch.

`knee == 0.0f` is a valid parameter value (range starts at 0.0) **and is a shipped
factory preset** — "Parallel Crush" sets `knee = 0.0f` (PluginProcessor.cpp:176).
The extra condition (`x == 0.0f` exactly) is reached whenever the dB-domain envelope
lands on the threshold as it sweeps across it, which happens routinely for sustained
signals near the threshold level over long runs. A single NaN sample is an audible
click at best and a stuck-NaN channel at worst.

**Fix:** Guard the denominator (treat a zero/near-zero knee as a hard knee):

```cpp
float calculateGainReduction(float inputLevel, float thresholdDB,
                             float ratio, float kneeDB)
{
    float x = inputLevel - thresholdDB;

    // Hard knee (or numerically negligible knee): avoid /0 in the soft-knee term
    if (kneeDB <= 1.0e-6f)
        return x > 0.0f ? x - (x / ratio) : 0.0f;

    if (x < -kneeDB / 2.0f)
        return 0.0f;
    else if (x > kneeDB / 2.0f)
        return x - (x / ratio);

    float kneeInput = x + kneeDB / 2.0f;
    return (1.0f - 1.0f / ratio) * (kneeInput * kneeInput) / (2.0f * kneeDB);
}
```

## Warnings

### WR-01: Channel-pointer array capped at 2 but detection/apply loops are unbounded — latent OOB

**File:** `plugins/O-Comp/Source/PluginProcessor.cpp:239-278`
**Issue:** `channelPtrs` is a fixed `float*[2]` populated only for `ch < numChannels && ch < 2`
(line 240), but the detection loop (line 247) and the gain-apply loop (line 274)
iterate `ch < numChannels` with **no `< 2` cap**. If `numChannels > 2`, `channelPtrs[2+]`
are null-initialized → `channelPtrs[ch][sample]` dereferences nullptr → crash. There is
no `isBusesLayoutSupported` override in the processor, so the plugin does not actively
reject a >2-channel negotiated layout; the only thing keeping this safe today is the
stereo-only `BusesProperties` declaration. This is a defensive gap, not a currently
reproducing crash.
**Fix:** Cap the loops to the array size (and/or add `isBusesLayoutSupported`):

```cpp
const int nCh = std::min(numChannels, 2);
// ...use nCh in both the detection loop and the apply loop...
```
or add:
```cpp
bool isBusesLayoutSupported(const BusesLayout& l) const override {
    return l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
        || l.getMainOutputChannelSet() == juce::AudioChannelSet::mono();
}
```

### WR-02: Makeup/output gain applied per-sample with no smoothing — zipper on automation

**File:** `plugins/O-Comp/Source/PluginProcessor.cpp:220-227, 270`
**Issue:** `makeupGainLinear` (auto-gain + output_gain) is computed once per block from
the raw atomic parameter values and applied to every sample as a hard multiply. When a
host automates `output_gain` (or toggles `auto_gain`, or sweeps `threshold`/`ratio` which
feed auto-gain), the gain steps discontinuously at block boundaries, producing audible
zipper/clicks. The attack/release coefficients are likewise recomputed from un-smoothed
params each block. (Note: performance of the per-block `std::exp` is out of scope; the
concern here is the discontinuity, i.e. correctness of the audible result.)
**Fix:** Wrap the makeup gain in a `juce::SmoothedValue<float>` initialized in
`prepareToPlay` and advanced per sample, e.g.
`smoothedMakeup.setTargetValue(makeupGainLinear);` then
`channelPtrs[ch][sample] *= smoothedMakeup.getNextValue();`.

### WR-03: `getNextPreset`/`getPreviousPreset` silently jump to index 0 when current preset isn't in the list

**File:** `plugins/O-Comp/Source/OuariconPresetManager.h:436-441, 450-455`
**Issue:** After `loadPresetFromFile(...)`, `currentPresetName` is set to the imported
file's basename (line 365), which need not exist in the Factory/User list. On the next
`getNextPreset()`/`getPreviousPreset()`, `presets.indexOf(currentPresetName)` returns -1
and the function returns `presets[0]` — so "Next" from an imported preset lands on the
alphabetically-first preset instead of the neighbor of where the user actually is. Same
happens after `deletePreset` sets the name to "Default" when no "Default" preset file
exists. Confusing navigation, not a crash.
**Fix:** When `currentIndex < 0`, treat it as "before the list" for next (return
`presets[0]`) but for previous return `presets.getLast()`, or persist the last known
list index rather than resolving by name each call.

## Info

### IN-01: Direct `savePreset(name)` uses the name verbatim as a filename — path-separator names silently drop

**File:** `plugins/O-Comp/Source/OuariconPresetManager.h:306`
**Issue:** `getUserPresetsDirectory().getChildFile(presetName + ".json")` treats `/` (and
on Windows `\`) in `presetName` as a path separator, so a name like `"Bus / Glue"` writes
to an unexpected/nonexistent subdir and the save silently fails (known repo failure mode —
see O-simplePhysicalModelSynth "Koto / Harp"). The interactive path is currently safe
because the UI's Save button routes through `saveWithDialog` → `getFileNameWithoutExtension`,
but the exported programmatic `savePreset` (also exposed as a native fn at PluginEditor.cpp:56)
has no sanitization.
**Fix:** Sanitize before building the file: `auto safe = juce::File::createLegalFileName(presetName);`
and reject/return false if `safe != presetName` (or use `safe`).

### IN-02: Gain-reduction histogram bars overflow the panel above 30 dB GR

**File:** `plugins/O-Comp/Source/ui/public/index.html:916`
**Issue:** `barHeight = (grHistory[ri] / 30) * (...)` hard-codes a 30 dB full-scale. With
the "Parallel Crush"/"Aggressive Smash" presets, GR can exceed 30 dB, so bars draw past
the allotted half-panel with no clamp. Cosmetic only.
**Fix:** Clamp: `const norm = Math.min(1, grHistory[ri] / 30);` and extract `30` to a named
`GR_FULL_SCALE_DB` constant shared with the `GR:` text.

### IN-03: Transfer-curve canvas has no devicePixelRatio scaling — blurry on Retina

**File:** `plugins/O-Comp/Source/ui/public/index.html:514, 756-758`
**Issue:** `#transferCurveCanvas` (and `#envelopeCanvas`) use a fixed backing store
(`width="210" height="240"`) stretched to a CSS 100% box. On Retina/HiDPI the 3.5px vine
curve renders soft. Per the repo's canvas-DPR note, set
`canvas.width = clientWidth * devicePixelRatio` and `ctx.setTransform(dpr,0,0,dpr,0,0)` for
crisp output. Visual polish only.

### IN-04: Static placeholder values in HTML don't match processor defaults

**File:** `plugins/O-Comp/Source/ui/public/index.html:421, 466`
**Issue:** The initial `ratio-value` text is `4.0:1` and `knee-value` is `6.0 dB`, but the
processor default ratio is 2:1 (PluginProcessor.cpp:32). These placeholders are overwritten
by `updateKnob` on load, so it's a momentary flash of a wrong number at most. Align the
static text with the real defaults (2:1) to avoid confusion when reading the source.

---

_Reviewed: 2026-07-01T05:37:26Z_
_Reviewer: Claude (gsd-code-reviewer)_
_Depth: deep_
