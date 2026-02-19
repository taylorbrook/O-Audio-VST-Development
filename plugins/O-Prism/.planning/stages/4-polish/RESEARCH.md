# Stage 4: Polish - Research

**Date:** 2026-02-18
**Plugin:** O-Prism (Microtonal Wavetable Synthesizer)
**Scope:** 4 bugs, version bump, effects layout change, CPU profiling

---

## Bug 1: Wavetable Canvas Displays Blank

### Code Flow Analysis

**JS side** (`index.html` lines 953-1062):
1. `WavetableDisplay` constructor calls `Juce.getNativeFunction('getWavetableFrameForPosition')` to get a function reference
2. `wireWavetableDisplay()` registers listeners on `oscAPos` and `oscATable` state changes
3. `setTimeout(refresh, 200)` triggers initial draw
4. `refresh()` computes `tableIdx = Math.round(tableState.getNormalisedValue() * 3)` and calls `display.fetchAndDraw(tableIdx, pos)`
5. `fetchAndDraw()` awaits the native function, parses JSON result into samples array, calls `draw()`

**C++ side** (`PluginEditor.cpp` lines 398-422):
1. `getWavetableFrameForPosition` receives `(oscId, normalizedPos)`
2. Gets factory table via `processorRef.getFactoryTable(oscId)`
3. Factory tables exist (Saw/Square/Triangle/Sine, each 1 frame, generated in processor constructor)
4. Downsamples 2048 -> 256 samples, returns JSON array string
5. Calls `complete(json)` or `complete(juce::var())` on error

### Root Cause Analysis

**Most likely cause: Silent error in catch block**

The `fetchAndDraw()` catch block at line 981 silently swallows ALL errors:
```js
catch(e) { /* ignore fetch errors */ }
```

Any of these failures would be invisible:
- `getNativeFunction()` returning undefined (function not found)
- Promise rejection from the native bridge
- JSON parse failure
- TypeError from unexpected result format

**Secondary possibility: Timing issue with `setTimeout(refresh, 200)`**

The initial draw fires 200ms after module script execution. If the WebView native bridge isn't fully initialized by then, `getNativeFunction()` might return a function that doesn't actually reach the C++ side. The tuning panel uses a similar pattern but initializes asynchronously with `await tuningPanel.init()`.

**Third possibility: Parameter normalization mismatch**

The `oscATable` parameter is `AudioParameterInt(0, 15)` (range 0-15) but the JS maps it as if there are only 4 choices. At default (value 0, norm 0), `tableIdx = 0` which is correct. But if the parameter is ever at non-zero default, the mapping breaks. This wouldn't cause a blank canvas at startup but would cause wrong table selection after user interaction.

### Proposed Fix

1. **Add error logging** to the catch block: `console.error('Wavetable fetch error:', e)`
2. **Add null check** before calling: `if (typeof this.getFrame !== 'function') return;`
3. **Increase timeout or use retry**: Replace `setTimeout(refresh, 200)` with a retry loop that checks if the native function returns data
4. **Validate result**: Add `console.log('WT result:', result)` to verify what the native function returns
5. **Fix parameter range mismatch**: The `oscATable` param range 0-15 should ideally be 0-3 since there are only 4 factory tables. The 0-15 range was reserved for future user wavetable slots but creates a normalization mismatch with the 4-option dropdown.

### Risk: Low-Medium
- Fix is JS-only (error logging + retry)
- Parameter range change requires C++ modification but is backward-compatible (clamped by `updateWavetableAssignments()`)
- No audio thread changes

---

## Bug 2: Filter Routing Dropdown Misaligned

### Current Layout

`index.html` lines 499-509:
```html
<div class="section" style="margin-top:4px; margin-bottom:4px;">
    <div class="param-row" style="justify-content:center;">
        <div class="dropdown-group">
            <span class="dropdown-label">Filter Routing</span>
            <select id="select-filtRouting" class="param-select">
                <option>Serial</option><option>Parallel</option>
            </select>
        </div>
    </div>
</div>
```

The `justify-content:center` centers the dropdown on the full page width. The Filter A/B sections above are in `inline-sections` flex containers (`flex: 1` each), so they occupy left/right halves. The routing dropdown visually floats alone in the center below, disconnected from the filter sections.

### Proposed Fix

Move the Filter Routing dropdown into the `.inline-sections` wrapper, between Filter A and Filter B, or make it part of the filter row layout. Options:

**Option A (recommended):** Place routing dropdown centered between the two filter sections by wrapping all three in the same `.inline-sections` flex container, with routing as a narrow center element:
```html
<div class="inline-sections">
    <div class="inline-section"> <!-- Filter A --> </div>
    <div style="display:flex; align-items:center; padding-top:20px;">
        <div class="dropdown-group">...</div>
    </div>
    <div class="inline-section"> <!-- Filter B --> </div>
</div>
```

**Option B:** Left-align the routing dropdown under Filter A's section.

### Risk: Low
- HTML/CSS only change
- No C++ changes needed
- No parameter binding changes

---

## Bug 3: Noise Generator Sounds Crackly

### Gain Analysis

Examined `NoiseGenerator.cpp` output levels for each type:

| Type | Output Expression | Peak Range | Issue |
|------|------------------|------------|-------|
| White (0) | `white` | [-1, +1] | OK |
| Pink (1) | `(b0+b1+b2+white*0.1848)*0.11` | ~[-1, +1] | OK (Kellet coefficients self-normalize) |
| Brown (2) | `brownState * 3.5` | **[-3.5, +3.5]** | CLIPPING |
| Digital (3) | `digitalHoldValue` | [-1, +1] | OK (but harsh by design) |
| Vinyl (4) | `vinylBP2*2.0 + crackleDecay` | ~[-2.5, +2.5] | CLIPPING |
| Wind (5) | `windLPState * 5.0` | **[-5.0, +5.0]** | CLIPPING |

### Brown Noise Analysis (line 61)

```cpp
brownState += white * 0.02 * rateScale;
brownState *= 0.998;
return brownState * 3.5;
```

The leaky integrator with coefficient 0.998 has a steady-state standard deviation of:
- `sigma = 0.02 / sqrt(1 - 0.998^2) = 0.02 / 0.0632 = ~0.316`
- 3-sigma peak: ~0.95
- With 3.5x multiplier: peaks at ~3.3 (well above 1.0)

### Wind Noise Analysis (line 109)

```cpp
windLPState += alpha * (windBrownState - windLPState);
return windLPState * 5.0;
```

Same brown noise source, then lowpass filtered (reduces amplitude), then 5.0x multiplier. The lowpass cuts amplitude but 5.0x overcompensates. Peaks exceed 1.0.

### Vinyl Noise Analysis (lines 83-89)

```cpp
double vinyl = vinylBP2 * 2.0;
crackleDecay = (random * 0.5 + 0.5) * polarity;  // +-[0.5, 1.0]
crackleDecay *= 0.95;  // decay per sample
vinyl += crackleDecay;
```

The bandpass output plus crackle impulses can exceed 1.0.

### Proposed Fix

**Approach: Normalize all noise types to peak ~[-1.0, +1.0], then apply soft clip safety net**

```cpp
case 2: // Brown
{
    brownState += white * 0.02 * rateScale;
    brownState *= 0.998;
    return std::tanh(brownState * 3.5);  // Soft clip to [-1, 1]
}

case 4: // Vinyl
{
    // ... existing code ...
    return std::tanh(vinyl);  // Soft clip
}

case 5: // Wind
{
    // ... existing code ...
    return std::tanh(windLPState * 5.0);  // Soft clip
}
```

Using `std::tanh()` provides smooth saturation that:
1. Preserves quiet signals (tanh(x) ~ x for small x)
2. Smoothly compresses peaks above ~0.7
3. Hard-limits to [-1, +1]
4. No discontinuities (no crackle/pop from hard clipping)

**Alternative**: Reduce gain multipliers instead of soft clipping:
- Brown: `brownState * 1.5` (reduce from 3.5)
- Wind: `windLPState * 2.0` (reduce from 5.0)
- Vinyl: `vinyl * 0.7` (reduce from implicit 1.0)

**Recommendation**: Use `tanh()` soft clipping. It's more robust than tuning gain values, and gives a subtle warmth that suits the noise character. The tanh per-sample cost is negligible.

### Risk: Low
- DSP-only change in NoiseGenerator.cpp
- No parameter changes
- No UI changes
- `std::tanh` is SIMD-friendly, negligible CPU impact

---

## Bug 4: Effects Sub-Tabs -> Show All at Once

### Current Implementation

`index.html` lines 552-629:
- `.effect-tab-bar` div with 5 tabs (Reverb, Delay, Chorus, Distortion, EQ)
- 5 `.effect-panel` divs, each `display: none` except active one
- `switchEffectTab()` JS function toggles visibility

### Proposed Fix

1. **Remove** the `.effect-tab-bar` div entirely
2. **Remove** the `.effect-panel` class from each effect section (or override to `display: block`)
3. **Remove** the `switchEffectTab()` JS function
4. **Remove** CSS rules for `.effect-tab-bar`, `.effect-tab`, `.effect-panel`
5. Each effect section becomes a normal `<div>` with `.section-header` + `.param-row`
6. Effects tab becomes scrollable (already has `overflow-y: auto` on `.tab-content`)

### Layout After Fix

```
EFFECTS TAB (scrollable)
+----------------------------------+
| REVERB                           |
| [Size] [Damp] [Pre-Dly] [Mix]   |
|                                  |
| DELAY                            |
| [Time] [FB] [Mode] [Sync] [Mix] |
|                                  |
| CHORUS                           |
| [Rate] [Depth] [Mix]            |
|                                  |
| DISTORTION                       |
| [Type] [Drive] [Mix]            |
|                                  |
| 3-BAND EQ                        |
| [Low] [Mid] [Mid Freq] [High]   |
+----------------------------------+
```

Add `margin-bottom: 12px` to each effect section for visual separation.

### Risk: Low
- HTML/CSS/JS only
- No C++ changes
- No parameter binding changes
- All knob bindings remain identical

---

## Version Bump: 0.1.0 -> 0.9.0

### Files to Change

1. **CMakeLists.txt** line 12: `VERSION 0.1.0` -> `VERSION 0.9.0`

No UI version display exists currently (no version number shown in the UI), so only the CMake version needs updating.

---

## CPU Profiling Strategy

### Worst-Case Scenario

16 voices x 8 unison x 2 oscillators = **256 wavetable oscillator instances**
+ 16 x 2 SVF filters (32 filter instances, some with 2 cascaded SVFs = 64 SVF stages)
+ 16 x 2 ADSR envelopes
+ 16 x 1 noise generator + 1 sub oscillator
+ 5 global effects

### Profiling Approach

1. **Build Release** with optimizations: `cmake -DCMAKE_BUILD_TYPE=Release`
2. **Standalone app** for isolated measurement
3. **Test scenarios**:
   - Minimal: 1 voice, 1 unison, 1 osc -> baseline
   - Typical: 8 voices, 2 unison, both osc -> target <15%
   - Stress: 16 voices, 8 unison, both osc + all effects -> target <50%
4. **Measurement**: Use Instruments.app Time Profiler on macOS
5. **Key optimization targets** (if needed):
   - Wavetable interpolation (most called code path)
   - Mipmap level selection (per-sample log2 computation)
   - SVF filter `processSample()` calls
   - Unison spread calculations

### Optimization Options (if needed)

- Reduce max polyphony dynamically when unison > 4
- Use float for wavetable oscillator (double only for filters)
- Cache mipmap level per block instead of per sample
- SIMD for wavetable linear interpolation

### Risk: Low
- Profiling is non-destructive
- Optimizations only applied if CPU exceeds 40% at typical use

---

## Release Checklist (v0.9.0 beta)

1. Fix 4 bugs (canvas, noise, effects layout, filter routing alignment)
2. Version bump to 0.9.0
3. pluginval strictness 10
4. Ableton Live (VST3) verification
5. Logic Pro (AU) verification
6. State save/restore test
7. CHANGELOG.md creation

---

## Summary of Findings

| Item | Type | Complexity | Risk | C++ Changes | JS/CSS Changes |
|------|------|-----------|------|-------------|----------------|
| Wavetable canvas blank | Bug | Medium | Low-Med | None (maybe param range) | Error logging + retry |
| Filter routing alignment | Bug | Low | Low | None | CSS layout |
| Noise generator crackly | Bug | Low | Low | tanh() in 3 noise types | None |
| Effects sub-tabs removal | Enhancement | Low | Low | None | Remove tab bar + show all |
| Version bump | Release | Trivial | None | CMakeLists.txt | None |
| CPU profiling | Investigation | Medium | Low | Only if optimization needed | None |
| CHANGELOG.md | Documentation | Low | None | None | None |
