# Testing Patterns

**Analysis Date:** 2026-01-29

## Test Framework

**Status:** No automated test framework detected

**Infrastructure:**
- No `test/` directory
- No `.cpp/.h` test files found
- No test runner configuration (`CMakeLists.txt` for tests, pytest config, etc.)
- No C++ unit testing library (Google Test, Catch2, Doctest) integrated
- No CI/CD test pipeline configuration

**Testing Approach:** Manual testing in DAW (Logic Pro, Ableton) only

---

## Manual Testing Approach

**Primary Method:** DAW Integration Testing

**Process:**
1. **Build**: Compile plugin using ninja/CMake
2. **Install**: Copy plugin binaries to system plugin folders (VST3, AU)
3. **Cache Clear**: Kill AudioComponentRegistrar, clear caches (per CLAUDE.md requirements)
4. **DAW Launch**: Load plugin in Logic Pro or Ableton
5. **Validation**: Test audio processing, parameter changes, presets, UI interaction
6. **Verification**: Check `auval -a | grep -i [pluginname]` for AU registration

**Example Workflow (from CLAUDE.md):**
```bash
# After ninja build
killall -9 AudioComponentRegistrar 2>/dev/null || true
rm -rf ~/Library/Caches/AudioUnitCache/
rm -rf ~/Library/Caches/com.apple.audiounits.cache

# Install
cp -R build/plugins/[PluginName]/[PluginName]_artefacts/Release/VST3/[PluginName].vst3 \
     ~/Library/Audio/Plug-Ins/VST3/
cp -R build/plugins/[PluginName]/[PluginName]_artefacts/Release/AU/[PluginName].component \
     ~/Library/Audio/Plug-Ins/Components/

# Verify (AU only)
auval -a | grep -i [pluginname]
```

---

## Code Review Points (Implicit Testing)

**Development Validation:**
- DSP algorithms tested conceptually against references (e.g., Linkwitz-Riley filters, Chebyshev polynomials)
- Parameter ranges validated during construction (e.g., `juce::NormalisableRange<float>(40.0f, 200.0f, ...)`)
- Thread safety verified via atomic patterns (no data races on meter updates)
- Known issues documented with explanation (e.g., oversampling disabled due to Logic Pro crashes)

**Code Comments as Tests:**
Several plugins document test outcomes in code:

`/Users/taylorbrook/Dev/VST-development/plugins/O-Bass/Source/DSP/HarmonicGenerator.cpp`:
```cpp
// Logic Pro compatibility note
// Note: Oversampling is disabled due to JUCE compatibility issues with
// Logic Pro. See CODE_REVIEW.md Priority 4 for investigation status.
```

`/Users/taylorbrook/Dev/VST-development/plugins/O-Bass/Source/PluginProcessor.cpp`:
```cpp
// DISABLED: Latency reporting causes Logic Pro crash ("Sample Rate XXXXX" error)
```

---

## Test Data & Fixtures

**Preset System (Built-in Test Data):**
- Factory presets serve as regression test fixtures
- Example from `OBassAudioProcessor::createParameterLayout()`:
  ```cpp
  // Factory presets initialize with known good states
  std::vector<OuariconPresetManager::FactoryPresetDef> factoryPresets = {
      {"Default", {{"crossover_freq", 0.25f}, {"enhance", 0.50f}, {"output", 0.5f}}, juce::var()},
      {"Gentle Bass Guitar", {{"crossover_freq", 0.375f}, {"enhance", 0.30f}, {"output", 0.5f}}, juce::var()},
      {"Punchy 808", {{"crossover_freq", 0.0f}, {"enhance", 0.70f}, {"output", 0.5f}}, juce::var()},
      // ... more presets ...
  };
  ```
- Presets stored as JSON in `~/Library/Application Support/{pluginName}/Presets/Factory/`
- Loading/saving tested implicitly via UI preset menu

**Parameter Ranges as Implicit Tests:**
- Example from `O-AnalogSaturation`:
  ```cpp
  // INTENSITY - 0-100% with 0.1 increment
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID { "INTENSITY", 1 },
      "Intensity",
      juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f, 1.0f),
      50.0f,
      "%"
  ));
  ```
- Each parameter range implicitly tested by DAW parameter automation

---

## Audio Processing Tests (Implicit)

**Compressor Test Pattern (O-MultiBandCompressor):**
```cpp
// processStereo() includes validation and meter output
void processStereo(juce::AudioBuffer<float>& buffer,
                  float thresholdDB, float ratio, float kneeDB, float makeupDB,
                  float peakRmsBlend, bool isBypassed,
                  float scHPFFreq, float scLPFFreq, bool scListen,
                  bool autoMakeupEnabled,
                  std::atomic<float>& gainReductionMeter)  // <-- Output meter for validation
{
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    if (numSamples == 0 || numChannels == 0)
        return;  // <-- Boundary validation

    if (isBypassed)
    {
        gainReductionMeter.store(0.0f, std::memory_order_relaxed);
        return;  // <-- Bypass state validation
    }

    // ... processing ...
    gainReductionMeter.store(smoothedGainReductionDB, std::memory_order_relaxed);  // <-- Meter for UI verification
}
```

**VU Meter Validation:**
- Every plugin exposes meter values via atomic accessors
- UI reads meters on timer for visualization
- Meters validate:
  - Input/output levels are reasonable (not clipping, not silent)
  - Processing is active (gain reduction, limiting indicators change)
  - Bypass state (meters zero when bypassed)

---

## DSP Algorithm Validation

**Crossover Filter Testing (O-Bass):**
- Algorithm: Linkwitz-Riley 24dB/oct (IIR) vs. linear-phase FIR convolution
- Validation: Test in DAW with tone sweep 20Hz-20kHz
- Expected: Smooth -24dB/oct slope at cutoff frequency
- Test artifact: User can switch modes and hear quality difference

**Compressor Algorithm Testing (O-MultiBandCompressor):**
- Algorithm: 4-band crossover + per-band feed-forward compressor with soft knee
- Validation:
  - Run kick drum through low band, verify attenuation matches threshold/ratio
  - Test sidechain filters (HPF/LPF) by enabling "sidechain listen" mode
  - Verify auto-makeup gain maintains perceived volume
- Test artifact: Spectrum analyzer shows band-specific compression

**Saturation Model Testing (O-AnalogSaturation):**
- 4 models: MAGNETIC, TUBE, TRANSFORMER, DIODE
- Validation:
  - Increase intensity from 0-100%, verify no aliasing (quality = HIGH uses 4x oversampling)
  - Compare output to reference analog circuit (subjective)
  - Check input/output levels don't clip unexpectedly
- Test artifact: Each model sounds distinctly different

---

## Thread Safety Testing (Implicit)

**Pattern:**
- Atomic meters are read/written from different threads
- Validation: UI doesn't crash when reading meters while audio processes
- Test in DAW: Adjust parameters while audio plays, verify smooth meters

**Example Atomic Usage (O-Bass):**
```cpp
class OBassAudioProcessor {
public:
    float getLimitIndicator() const { return limitIndicator.load(); }           // UI thread reads
    float getOutputLevelDB() const { return outputLevelDB.load(); }             // UI thread reads

private:
    std::atomic<float> limitIndicator { 0.0f };                                  // Audio thread writes
    std::atomic<float> outputLevelDB { -60.0f };                                 // Audio thread writes
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>
        limitIndicatorSmooth;                                                   // Smooths writes for UI
};
```

**Validation:** No locks = no audio glitches, meters update smoothly

---

## Known Test Gaps

**Critical Testing Needs:**
- **File I/O**: Preset save/load tested manually only (no file-based regression tests)
- **Edge Cases**: Buffer sizes, sample rates, channel counts not unit tested
- **Compatibility**: Only tested in Logic Pro; need Ableton, Reaper, Studio One verification
- **Latency**: Latency reporting disabled in some plugins due to DAW crashes (untested)
- **Oversampling**: Disabled in O-Bass due to Logic Pro incompatibility (never fully validated)

**Untested Algorithms:**
- Complex mode switching (LowLatency ↔ HighFidelity) while audio plays
- Extreme parameter combinations (all settings at min/max simultaneously)
- Rapid preset switching
- Very long buffer sizes or very short buffer sizes

---

## Performance Testing

**Implicit Validation:**
- DAW monitoring: Listen for audio glitches, dropouts
- CPU meter: Check DSP doesn't exceed reasonable limits at 48kHz
- Build warnings: JUCE recommended warning flags enable strict compilation

**Not Formally Measured:**
- Per-plugin CPU usage (benchmarks)
- Memory allocation patterns (no profiler output)
- Startup time
- Preset load time

---

## Documentation of Test Decisions

**Research Directory (Test References):**
- `/Users/taylorbrook/Dev/VST-development/research/` contains algorithm references
- Papers on DSP algorithms validate theoretical correctness
- Used to verify implementations match published formulas

**Example:** Reverb algorithm references
- File: `/Users/taylorbrook/Dev/VST-development/research/reverb-comprehensive-research.md`
- Validates that reverb implementations follow Schroeder/Freeverb architecture

---

## Future Testing Strategy

**Recommended Additions:**
1. **Unit Tests**: C++ test framework (Google Test) for DSP algorithms
2. **Regression Suite**: Preset load/save, parameter automation, bypass states
3. **Integration Tests**: Multi-plugin chains, long-running sessions
4. **Benchmark Suite**: CPU usage per mode, memory allocation profiles
5. **Compatibility Matrix**: Verified DAW/OS/format combinations

**Current Bottleneck:**
- No automated test infrastructure
- All validation happens manually in DAW
- Test coverage unknown; relies on developer attention during integration

---

*Testing analysis: 2026-01-29*
