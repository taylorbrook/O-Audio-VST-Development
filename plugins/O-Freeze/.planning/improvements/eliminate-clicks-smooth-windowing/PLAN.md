# Plan: Eliminate Clicks & Smooth Windowing

**Milestone:** eliminate-clicks-smooth-windowing
**Plugin:** O-Freeze
**Created:** 2026-02-02

---

```yaml
milestone: eliminate-clicks-smooth-windowing
domain: dsp
execute_agent: dsp-agent
version_bump: minor
base_version: 1.0.1
target_version: 1.1.0
```

---

## Summary

This plan implements three targeted changes to eliminate audible clicks in O-Freeze:

1. **Replace asymmetric Blackman-Harris with symmetric Hann window** - Guarantees true zero at grain boundaries
2. **Implement staggered grain activation on freeze engage** - Only activate first grain immediately, let others follow naturally
3. **Implement soft grain deactivation on freeze release** - Let active grains complete their envelope cycle

All changes are confined to `PluginProcessor.cpp` with no header changes required.

---

## Task Breakdown

### Task 1: Replace Window Function
**Priority:** HIGHEST
**Dependencies:** None
**Estimated Impact:** Eliminates 60-70% of clicks

**Current Code (lines 113-141):**
```cpp
// Pre-compute asymmetric Blackman-Harris window
hannWindow.resize(grainSize);
const double PI = juce::MathConstants<double>::pi;
const int attackLen = static_cast<int>(grainSize * 0.6);
const int releaseLen = grainSize - attackLen;

// Blackman-Harris coefficients
const double a0 = 0.35875, a1 = 0.48829, a2 = 0.14128, a3 = 0.01168;
// ... asymmetric generation code ...
```

**Target Code:**
```cpp
// Pre-compute symmetric Hann window (true zero at endpoints, COLA compliant at 87.5% overlap)
hannWindow.resize(grainSize);
const double PI = juce::MathConstants<double>::pi;

for (int i = 0; i < grainSize; ++i)
{
    double phase = static_cast<double>(i) / static_cast<double>(grainSize - 1);
    hannWindow[i] = static_cast<float>(0.5 * (1.0 - std::cos(2.0 * PI * phase)));
}
```

**Verification:**
- [ ] Window starts at 0.0 (verify `hannWindow[0] == 0.0f`)
- [ ] Window ends at 0.0 (verify `hannWindow[grainSize-1] == 0.0f`)
- [ ] Window peak at center (verify `hannWindow[grainSize/2] ≈ 1.0f`)

---

### Task 2: Add Release State Flag
**Priority:** HIGH
**Dependencies:** None (can be done in parallel with Task 1)
**Estimated Impact:** Enables soft deactivation

**Current State:**
- No flag to indicate "stop triggering new grains but let active ones finish"

**Changes Required:**

**2a. Add member variable to header (PluginProcessor.h, after line 80):**
```cpp
bool stopTriggeringNewGrains = false;  // Soft release: let active grains complete
```

**2b. Reset flag in prepareToPlay (PluginProcessor.cpp, around line 152):**
```cpp
stopTriggeringNewGrains = false;
```

**Verification:**
- [ ] Flag declared in header
- [ ] Flag reset in prepareToPlay

---

### Task 3: Implement Staggered Grain Activation
**Priority:** MEDIUM
**Dependencies:** Task 2 (needs flag infrastructure)
**Estimated Impact:** Eliminates engage "burst" sound

**Current Code (lines 245-257):**
```cpp
if (bufferFrozen)
{
    // Immediately trigger all 8 grains with staggered positions
    const int freezeBufLen = freezeBuffer.getNumSamples();
    int startPos = (writePosition - grainSize + freezeBufLen) % freezeBufLen;
    for (int i = 0; i < 8; ++i)
    {
        grains[i].active = true;
        grains[i].startSample = i * (grainSize / 8);
        grains[i].position = (startPos + i * (grainSize / 8)) % freezeBufLen;
    }
    nextGrainIndex = 0;
    grainTriggerCounter = 0;
}
```

**Target Code:**
```cpp
if (bufferFrozen)
{
    // Staggered activation: only activate FIRST grain immediately
    // Subsequent grains will be activated by normal trigger mechanism
    const int freezeBufLen = freezeBuffer.getNumSamples();
    int startPos = (writePosition - grainSize + freezeBufLen) % freezeBufLen;

    // Activate only the first grain
    grains[0].active = true;
    grains[0].startSample = 0;
    grains[0].position = startPos;

    // Deactivate all other grains - they'll be triggered naturally
    for (int i = 1; i < 8; ++i)
    {
        grains[i].active = false;
    }

    nextGrainIndex = 1;  // Next grain to trigger
    grainTriggerCounter = 0;
    stopTriggeringNewGrains = false;  // Ensure we're triggering grains
}
```

**Verification:**
- [ ] Only grain 0 active immediately after freeze engage
- [ ] Full granular texture builds up over ~175ms (7 trigger intervals)
- [ ] No audible "burst" on engage

---

### Task 4: Implement Soft Grain Deactivation
**Priority:** HIGH
**Dependencies:** Task 2 (needs stopTriggeringNewGrains flag)
**Estimated Impact:** Eliminates release clicks entirely

**Current Code (lines 259-267):**
```cpp
else
{
    // Freeze released: fade out frozen signal
    freezeGain.reset(currentSampleRate, 0.100); // 100ms fade-out
    freezeGain.setTargetValue(0.0f);

    // Deactivate all grains
    for (auto& grain : grains)
        grain.active = false;
}
```

**Target Code:**
```cpp
else
{
    // Freeze released: soft deactivation
    // Stop triggering NEW grains, but let active grains complete their cycle
    stopTriggeringNewGrains = true;

    // Extended fade-out to cover grain completion time (grainSize samples ≈ 200ms)
    // Add extra 50ms safety margin
    freezeGain.reset(currentSampleRate, 0.250); // 250ms fade-out
    freezeGain.setTargetValue(0.0f);

    // DON'T deactivate grains here - they'll naturally complete
    // when startSample >= grainSize (already handled in grain advance loop)
}
```

**Verification:**
- [ ] Active grains continue playing after release
- [ ] No new grains triggered after release
- [ ] Grains naturally complete and deactivate
- [ ] No audible click on release

---

### Task 5: Update Grain Trigger Logic
**Priority:** HIGH
**Dependencies:** Task 2, Task 4
**Estimated Impact:** Completes soft deactivation implementation

**Current Code (lines 276-302):**
```cpp
if (bufferFrozen)
{
    if (grainTriggerCounter >= grainTriggerInterval)
    {
        // Activate new grain
        // ...
    }
    // ...
}
```

**Target Code:**
```cpp
// Only trigger new grains if frozen AND not in soft release mode
if (bufferFrozen && !stopTriggeringNewGrains)
{
    if (grainTriggerCounter >= grainTriggerInterval)
    {
        // Activate new grain
        // ... (existing code unchanged)
    }
    else
    {
        grainTriggerCounter++;
    }
}
```

**Verification:**
- [ ] Grains continue triggering during normal freeze
- [ ] Grain triggering stops when `stopTriggeringNewGrains = true`
- [ ] Active grains still process and complete during soft release

---

### Task 6: Verify COLA Compliance (Optional Debug Check)
**Priority:** LOW
**Dependencies:** Task 1
**Estimated Impact:** Confirms window correctness

**Add to prepareToPlay (after window generation, debug builds only):**
```cpp
#if JUCE_DEBUG
// Verify COLA compliance: sum of overlapping windows should be constant
float colaSum = 0.0f;
for (int grainOffset = 0; grainOffset < 8; ++grainOffset)
{
    int windowPos = grainOffset * grainTriggerInterval;
    if (windowPos < grainSize)
        colaSum += hannWindow[windowPos];
}
// At 87.5% overlap with Hann window, sum should be approximately 1.0
jassert(std::abs(colaSum - 1.0f) < 0.1f);
#endif
```

**Verification:**
- [ ] Debug assertion passes
- [ ] Can be removed after verification

---

## Dependency Graph

```
Task 1 (Window)        Task 2 (Flag)
      │                     │
      │              ┌──────┴──────┐
      │              │             │
      │         Task 3        Task 4
      │       (Stagger)    (Soft Deact)
      │              │             │
      │              └──────┬──────┘
      │                     │
      │                 Task 5
      │              (Trigger Logic)
      │                     │
      └─────────┬───────────┘
                │
            Task 6
         (COLA Verify)
```

**Execution Order:**
1. Tasks 1 and 2 can run in parallel (no dependencies)
2. Task 3 requires Task 2
3. Task 4 requires Task 2
4. Task 5 requires Tasks 2 and 4
5. Task 6 requires Task 1 (optional, debug only)

---

## Files Modified

| File | Changes |
|------|---------|
| `PluginProcessor.h` | Add `stopTriggeringNewGrains` member variable |
| `PluginProcessor.cpp` | Replace window, modify engage/release logic, update trigger condition |

---

## Risk Assessment

| Task | Risk | Mitigation |
|------|------|------------|
| Window replacement | Low - may change tonal character slightly | Hann is warmer, aligns with requirements |
| Staggered activation | Medium - freeze engage may sound "building up" | First grain starts immediately, builds naturally |
| Soft deactivation | Low - release may feel slightly longer | Extended fade covers grain completion |

---

## Test Plan

### Manual Tests

1. **Percussive Material Test**
   - Load drum loop or transient-heavy material
   - Engage/release freeze rapidly
   - Listen for clicks at any phase

2. **Sustained Material Test**
   - Load pad or drone
   - Engage freeze, hold for 10+ seconds
   - Release and listen for smooth transition

3. **Threshold Mode Test**
   - Set to threshold mode
   - Feed audio that crosses threshold repeatedly
   - Verify no clicks on automatic engage/release

4. **Drift Interaction Test**
   - Set Drift to 100%
   - Engage freeze
   - Verify no additional clicks from drift variation

5. **Rapid Toggle Test**
   - Rapidly toggle freeze on/off
   - Verify graceful handling without crashes or artifacts

### Automated Tests

1. Build succeeds without warnings
2. Pluginval passes all tests
3. AU validation passes (`auval -v aufx Ofrz Ouar`)

---

## Success Criteria

- [ ] No audible clicks at any point in freeze cycle
- [ ] Warmer, smoother frozen texture
- [ ] Smooth transitions between dry and frozen states
- [ ] No regression in Drift, Mix, or Threshold mode
- [ ] CPU usage within 10% of previous implementation
- [ ] Pluginval passes
- [ ] AU validation passes

---

## Post-Implementation

1. Update CHANGELOG.md with v1.1.0 entry
2. Create git commit: `improve(O-Freeze): v1.1.0 - Click-free granular windowing`
3. Tag release: `v1.1.0`
4. Clear activeMilestone from registry
