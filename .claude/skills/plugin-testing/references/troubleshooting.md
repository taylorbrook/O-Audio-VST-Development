# Testing Troubleshooting Guide

## Common Issues and Solutions

### Issue: Automated Tests Fail to Compile

**Error:**

```
Tests failed to compile:

Error: 'Catch2/catch_test_macros.hpp' file not found
```

**Cause:** Catch2 testing framework not installed

**Solutions:**

```
Fix options:
1. Skip automated tests, use pluginval instead (recommended)
2. Install Catch2: brew install catch2
3. Add Catch2 to CMakeLists.txt as FetchContent

Choose (1-3): _
```

### Issue: Pluginval Not Found

**Error:**

```
Pluginval not found at /Applications/pluginval.app or in PATH
```

**Cause:** Pluginval not installed at expected location

**Solutions:**

```
Install options:
1. Via Homebrew: brew install --cask pluginval
   (Installs to /Applications/pluginval.app - RECOMMENDED)
2. Manual download: https://github.com/Tracktion/pluginval/releases
   (Download and place pluginval.app in /Applications/ folder)
3. Skip pluginval, run automated tests instead

After installing:
- Verify installation: ls -la /Applications/pluginval.app/Contents/MacOS/pluginval
- No need to add to PATH or create symlinks
- Rerun: /test [PluginName] build

Choose (1-3): _
```

### Issue: State Save/Load Test Failed

**Error:**

```
State save/load test failed: Parameters not restoring correctly
```

**Root cause:** Using raw AudioProcessor parameters instead of APVTS

**Fix:**

```
1. Open PluginProcessor.h
2. Verify you have: juce::AudioProcessorValueTreeState apvts;
3. Open PluginProcessor.cpp
4. In getStateInformation():
   auto state = apvts.copyState();
   std::unique_ptr<juce::XmlElement> xml(state.createXml());
   copyXmlToBinary(*xml, destData);

5. In setStateInformation():
   std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
   if (xml.get() != nullptr)
       apvts.replaceState(juce::ValueTree::fromXml(*xml));

This ensures APVTS handles all state management automatically.
```

### Issue: Thread Safety Failure

**Error:**

```
Thread safety test failed: Memory allocation detected in processBlock()

Location: PluginProcessor.cpp:147
Culprit: std::vector::resize()
```

**Root cause:** Allocating memory in real-time audio thread

**Fix:**

```
Real-time audio rule: NEVER allocate memory in processBlock()

Fix:
1. Move all allocations to prepareToPlay():
   void prepareToPlay(double sampleRate, int samplesPerBlock) override {
       delayBuffer.setSize(2, 48000);  // Pre-allocate max size
   }

2. In processBlock(), only access pre-allocated buffers:
   void processBlock(juce::AudioBuffer<float>& buffer, ...) override {
       // No resize(), no new, no malloc()
       // Only use delayBuffer (already allocated)
   }

Common allocation sources:
- std::vector::resize()
- juce::String constructor
- std::make_unique()
- juce::AudioBuffer::setSize()

Rule: If it might allocate, move it to prepareToPlay().
```

### Issue: Multiple Test Failures (Overwhelming)

**Error:**

```
Multiple test failures detected (8/50 tests failed)
```

**Approach:** Prioritize fixes

**Recommendation:**

```
This is normal for initial testing - don't be discouraged.

Recommendation: Fix failures in priority order:

High priority (will cause crashes):
1. Thread safety failures → Fix first (real-time audio rule)
2. State save/load failures → Fix second (user data loss)

Medium priority (will cause issues):
3. Parameter automation failures → Fix third (workflow issues)
4. Audio validation failures → Fix fourth (sonic issues)

Low priority (nice to have):
5. Preset handling → Fix last (convenience)

Let's tackle them one at a time. Start with thread safety?

1. Yes, investigate thread safety failure
2. No, I'll fix manually
3. Show me all failures, I'll prioritize

Choose (1-3): _
```

### Issue: Continue Anyway Warning

**User chooses:** "Continue anyway" despite failures

**Response:**

```
⚠️  WARNING: Continuing with pluginval failures is NOT RECOMMENDED

These failures may cause:
- Crashes in DAWs (Ableton, Logic, etc.)
- Rejected from plugin marketplaces
- Poor user experience (state loss, automation issues)

Recommended: Fix failures before installation.

Proceed anyway? (y/n): _
```

## Test-Specific Troubleshooting

### Parameter Response Test Failures

**Common causes:**

- Parameter declared but not read in DSP code
- Wrong parameter ID in processBlock
- Parameter smoothing set to infinity (never reaches target)

**Debug approach:**

1. Check parameter is in APVTS layout
2. Verify `getRawParameterValue("paramID")` used in processBlock
3. Check parameter ID spelling (case-sensitive)
4. Verify smoothing parameters are reasonable

### Silent Input Test Failures

**Common causes:**

- Uninitialized delay buffers containing garbage
- Denormal numbers in filters/reverbs
- Missing `clear()` in `prepareToPlay()`

**Debug approach:**

1. Add `buffer.clear()` at start of prepareToPlay()
2. Add denormal protection: `juce::FloatVectorOperations::disableDenormalisedNumberSupport()`
3. Initialize all delay/buffer variables to zero

### Feedback Loop Test Failures

**Common causes by plugin type:**

- **Reverbs/delays**: Feedback parameter > 1.0 or summing without attenuation
- **Filters**: Unstable coefficients (resonance too high, wrong math)
- **Compressors**: Negative attack/release times, wrong gain reduction sign
- **General**: Accumulating values without bounds checking

**Debug approach:**

1. Verify feedback coefficients < 1.0
2. Check filter stability (poles inside unit circle)
3. Add hard clipping as safety net: `juce::jlimit(-10.0f, 10.0f, sample)`
4. Test with extreme parameter values

### CPU Performance Test Failures

**Common causes:**

- Inefficient algorithms (no SIMD, excessive branching)
- Expensive functions in inner loops (sin, cos, exp, sqrt)
- Unnecessary memory operations
- Non-optimized build (Debug mode)

**Debug approach:**

1. Profile with Instruments (macOS) or perf (Linux)
2. Use SIMD where possible (juce::dsp classes)
3. Replace expensive functions with lookup tables
4. Ensure Release build with optimizations enabled
5. Consider reducing filter order or algorithm complexity
