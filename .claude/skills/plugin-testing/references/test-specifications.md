# Automated Test Specifications

**This document contains detailed implementations for the 5 automated stability tests.**

## Test Infrastructure Required

```cpp
// plugins/[Plugin]/Tests/StabilityTests.cpp
#include <catch2/catch_test_macros.hpp>
#include "../Source/PluginProcessor.h"

TEST_CASE("Parameter Response Test") {
    // Set parameters, process audio, verify output changes
}

TEST_CASE("State Save/Load Test") {
    // Save state, load state, verify parameters restore
}

// ... other tests
```

## Test 1: Parameter Response Test

**Purpose:** Verify all parameters actually affect audio output.

**How it works:**

```cpp
TEST_CASE("Parameter Response Test") {
    [PluginName]Processor processor;
    processor.prepareToPlay(44100, 512);

    // Generate test signal (1kHz sine wave)
    juce::AudioBuffer<float> inputBuffer(2, 512);
    fillWithSineWave(inputBuffer, 1000.0f, 44100.0f);

    // For each parameter
    for (auto* param : processor.getParameters()) {
        // Test at min value
        param->setValueNotifyingHost(0.0f);
        auto outputMin = processBuffer(processor, inputBuffer);

        // Test at max value
        param->setValueNotifyingHost(1.0f);
        auto outputMax = processBuffer(processor, inputBuffer);

        // Verify output changed
        float rmsDifference = calculateRMSDifference(outputMin, outputMax);
        REQUIRE(rmsDifference > 0.001f);  // Some audible change
    }
}
```

**PASS:** All parameters cause measurable output change (RMS difference > 0.001)

**FAIL:** List parameters that don't affect audio

**Example failure:**

```
❌ Parameter Response Test FAILED

Non-responsive parameters:
- bypass (parameter index 5)
  Expected: Output should differ when bypass = 0 vs 1
  Actual: Identical output (RMS diff: 0.0)

  Likely cause: Parameter connected to APVTS but not used in processBlock()
```

**Common causes:**

- Parameter declared but not read in DSP code
- Wrong parameter ID in processBlock
- Parameter smoothing set to infinity (never reaches target)

## Test 2: State Save/Load Test

**Purpose:** Verify plugin state persists correctly (DAW project save/load, preset recall).

**How it works:**

```cpp
TEST_CASE("State Save/Load Test") {
    [PluginName]Processor processor;

    // Set random parameter values
    juce::Random random;
    std::vector<float> originalValues;
    for (auto* param : processor.getParameters()) {
        float randomValue = random.nextFloat();
        param->setValueNotifyingHost(randomValue);
        originalValues.push_back(randomValue);
    }

    // Save state
    juce::MemoryBlock stateData;
    processor.getStateInformation(stateData);

    // Reset to defaults
    for (auto* param : processor.getParameters()) {
        param->setValueNotifyingHost(0.5f);
    }

    // Load state
    processor.setStateInformation(stateData.getData(), stateData.getSize());

    // Verify all parameters restored
    for (size_t i = 0; i < originalValues.size(); ++i) {
        float restored = processor.getParameters()[i]->getValue();
        REQUIRE(std::abs(restored - originalValues[i]) < 0.0001f);
    }
}
```

**PASS:** All parameters restore to exact values (within 0.0001 tolerance)

**FAIL:** List parameters that didn't restore correctly

**Example failure:**

```
❌ State Save/Load Test FAILED

Parameters not restoring:
- bypass (expected: 1.0, actual: 0.5, diff: 0.5)
- mix (expected: 0.73, actual: 0.5, diff: 0.23)

Likely causes:
- Parameters not saved in getStateInformation()
- Parameters not loaded in setStateInformation()
- Wrong parameter IDs in state save/load code
```

**Common causes:**

- Using raw AudioProcessor parameters instead of APVTS
- Typo in parameter ID during state load
- Missing parameters from XML state structure

## Test 3: Silent Input Test

**Purpose:** Verify plugin doesn't add unexpected noise when fed silence.

**How it works:**

```cpp
TEST_CASE("Silent Input Test") {
    [PluginName]Processor processor;
    processor.prepareToPlay(44100, 512);

    // Generate silence
    juce::AudioBuffer<float> silentBuffer(2, 512);
    silentBuffer.clear();

    // Process silence
    processor.processBlock(silentBuffer, juce::MidiBuffer());

    // Check output RMS
    float outputRMS = calculateRMS(silentBuffer);

    // For clean processors (compressors, EQs)
    REQUIRE(outputRMS < 0.0001f);  // Essentially silent

    // For generators/effects that add noise
    REQUIRE(outputRMS < 0.1f);  // Reasonable noise floor
}
```

**PASS:** Output is silent OR reasonable for plugin type

**FAIL:** Unexpected output level

**Example failure:**

```
❌ Silent Input Test FAILED

Output RMS: 0.847 (expected < 0.0001)

This plugin adds significant output when input is silent.

Likely causes:
- Uninitialized audio buffers
- Denormal numbers causing CPU noise
- Oscillator/generator not gated properly
- Missing bypass logic
```

**Plugin type expectations:**

- **Clean processors** (compressor, EQ, limiter): Output should be silent (RMS < 0.0001)
- **Noisy processors** (analog-modeled saturation, tape): Some noise acceptable (RMS < 0.01)
- **Generators** (reverb, delay with feedback): Test disabled or threshold raised

**Common causes:**

- Uninitialized delay buffers containing garbage
- Denormal numbers in filters/reverbs
- Missing `clear()` in `prepareToPlay()`

## Test 4: Feedback Loop Test

**Purpose:** Verify plugin doesn't explode (go to infinity) during processing.

**How it works:**

```cpp
TEST_CASE("Feedback Loop Test") {
    [PluginName]Processor processor;
    processor.prepareToPlay(44100, 512);

    // Generate test signal
    juce::AudioBuffer<float> testBuffer(2, 512);
    fillWithSineWave(testBuffer, 440.0f, 44100.0f);

    // Process 1000 iterations (simulating ~11 seconds of audio)
    float maxOutputSeen = 0.0f;
    for (int i = 0; i < 1000; ++i) {
        processor.processBlock(testBuffer, juce::MidiBuffer());
        maxOutputSeen = std::max(maxOutputSeen, findMaxAbsSample(testBuffer));

        // Early exit if exploding
        if (maxOutputSeen > 100.0f) {
            FAIL("Output exploding: " << maxOutputSeen);
        }

        // Reset input for next iteration
        fillWithSineWave(testBuffer, 440.0f, 44100.0f);
    }

    // Verify reasonable output level
    REQUIRE(maxOutputSeen < 10.0f);  // Allow some headroom above 0dBFS
}
```

**PASS:** Output RMS stays below threshold (< 10.0) over 1000 iterations

**FAIL:** Feedback or instability detected

**Example failure:**

```
❌ Feedback Loop Test FAILED

Iteration 247: Output = 87.3 (threshold: 10.0)

Plugin output is exploding during sustained processing.

Likely causes:
- Positive feedback in delay/reverb
- Unstable filter coefficients (poles outside unit circle)
- Gain multiplication without attenuation
- Missing hard clipping
```

**Common causes in different plugin types:**

- **Reverbs/delays**: Feedback parameter > 1.0 or summing without attenuation
- **Filters**: Unstable coefficients (resonance too high, wrong math)
- **Compressors**: Negative attack/release times, wrong gain reduction sign
- **General**: Accumulating values without bounds checking

## Test 5: CPU Performance Test

**Purpose:** Verify plugin runs in real-time (doesn't exceed available CPU budget).

**How it works:**

```cpp
TEST_CASE("CPU Performance Test") {
    [PluginName]Processor processor;
    processor.prepareToPlay(44100, 512);

    juce::AudioBuffer<float> testBuffer(2, 512);
    fillWithSineWave(testBuffer, 1000.0f, 44100.0f);

    // Warm-up (let caches stabilize)
    for (int i = 0; i < 100; ++i) {
        processor.processBlock(testBuffer, juce::MidiBuffer());
    }

    // Measure processing time
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 1000; ++i) {
        processor.processBlock(testBuffer, juce::MidiBuffer());
    }
    auto end = std::chrono::high_resolution_clock::now();

    double elapsedSeconds = std::chrono::duration<double>(end - start).count();
    double audioSeconds = (1000 * 512) / 44100.0;  // Time represented by 1000 buffers

    // Calculate real-time factor (should be < 1.0 for real-time)
    double realTimeFactor = elapsedSeconds / audioSeconds;

    // Allow 10% CPU usage (0.1 real-time factor)
    REQUIRE(realTimeFactor < 0.1);
}
```

**PASS:** Real-time factor < 0.1 (plugin uses < 10% of available CPU)

**FAIL:** Too CPU intensive

**Example failure:**

```
❌ CPU Performance Test FAILED

Real-time factor: 0.34 (expected < 0.1)

Plugin is too CPU intensive - using 34% of available processing time.

At 10ms buffer size (common in low-latency scenarios):
- Available: 10ms per buffer
- Plugin uses: 3.4ms per buffer
- Remaining for DAW/other plugins: 6.6ms

Performance budget exceeded by 240%.

Optimization suggestions:
- Profile with Instruments/perf to find hotspots
- Reduce filter order
- Use lookup tables for expensive functions (sin, exp, sqrt)
- Optimize inner loops (SIMD, remove branches)
```

**Real-time factor explanation:**

- **< 0.1**: Excellent (< 10% CPU usage)
- **0.1 - 0.3**: Good (10-30% CPU usage)
- **0.3 - 0.5**: Warning (30-50% CPU usage, may cause issues in complex projects)
- **> 0.5**: Fail (> 50% CPU usage, unusable in real-time)

**Performance benchmarks by plugin type:**

- **Simple utility** (gain, pan): < 0.01 real-time factor
- **Dynamics** (compressor, gate): < 0.05
- **EQ** (parametric, 4-8 bands): < 0.08
- **Time-based FX** (delay, chorus): < 0.10
- **Reverb** (algorithmic): < 0.15
- **Complex FX** (convolution reverb, spectral): < 0.30

## Running Automated Tests

**Invoke from command line:**

```bash
cd build
cmake .. -DBUILD_TESTS=ON
cmake --build . --target [PluginName]Tests
./Tests/[PluginName]Tests
```

**Expected output (all passing):**

```
Randomness seeded to: 1234567890

Running tests...
===============================================================================
All tests passed (5 assertions in 5 test cases)
```
