# Testing Patterns

**Analysis Date:** 2026-01-22

## Status: Manual Testing Only

**Framework:** Not detected
- No automated test framework (Google Test, Catch2, etc.) found
- No test runner configuration (CMake test targets, ctest)
- No unit test files (*.test.cpp, *.spec.cpp)

**Build Configuration:** CMake (3.15+)
- `CMakeLists.txt` at `/Users/taylorbrook/Dev/VST-development/plugins/OuariconLyrica/CMakeLists.txt`
- No test targets defined
- Focus on plugin compilation for VST3, AU, and Standalone formats

## Manual Testing Artifacts

**Test Scenario Documents:**
Located in `.claude/skills/plugin-testing/references/`:
- `test-specifications.md` - Manual test spec templates
- `manual-testing-guide.md` - Testing procedures
- `regression-testing.md` - Regression test checklists

**UI Test Mockups:**
Manual HTML mockup files in `.ideas/mockups/` directories:
- Pattern: `v{N}-ui-test.html` (multiple versions per plugin)
- Purpose: Visual verification of UI states

**Test Assets:**
- `test-scenarios.md` - Scenario definitions
- `test-plugin-specs.json` - Plugin configuration specs
- `browser-testing.md` - Browser-based UI testing guide

## Testing Categories (Manual)

### Unit Testing Approach

**Currently:** No unit tests exist
- DSP modules (`WaveguideString`, `TuningEngine`, etc.) are component-tested in isolation
- Manual verification through plugin parameter changes
- Direct measurement of output audio via standalone plugin

**Potential Test Targets** (where unit tests would be valuable):
- `TuningEngine` frequency calculations
- `SympatheticResonanceEngine` coupling matrix computations
- `WaveguideString` delay line calculations
- `StiffnessFilter` coefficient updates
- `BodyResonance` modal filter parameters

### Integration Testing Approach

**Current Pattern:**
Manual testing through plugin host interaction:
1. Load plugin in DAW (Logic Pro, Reaper, etc.)
2. Trigger MIDI notes via keyboard/host
3. Monitor visual feedback (tuning circle, UI updates)
4. Listen for audio artifacts, glitches, clicks

**Critical Integration Points Tested Manually:**
- MIDI event processing → Voice triggering → Audio output
- Parameter changes (APVTS) → DSP updates → Audio modulation
- WebView UI → JavaScript relay → Audio parameter binding
- File I/O (Scala file loading) → Tuning mode switching

### Audio Testing Approach

**Listening Tests:**
- Manual playback of test audio files (`.wav` reference files observed):
  - `/Users/taylorbrook/Dev/VST-development/dry.wav`
  - `/Users/taylorbrook/Dev/VST-development/working.wav`
  - `/Users/taylorbrook/Dev/VST-development/stuttering.wav`

**Parameter Sweep Testing:**
Manual variation of parameters to verify:
- Frequency response with `brightness` parameter changes
- Decay behavior with `decayTime` slider modifications
- Pluck characteristics with `pluckPosition` variations
- Material timbres with `stringMaterial` selection

### Regression Testing

**Tracked in Improvement Plans:**
- `improvements/` and `.bugs/` directories document known issues
- `CHANGELOG.md` tracks fixes and regressions
- Version-specific test notes in improvement documentation

**Example Pattern** (from git log):
- v1.11.0 → v1.11.1: "fix(OuariconLyrica): v1.11.1 - Editing interval cents in UI now updates tuning"
- v1.11.0 introduced tonic modal rotation, tested manually for regressions

## Code Coverage

**No Coverage Tracking:** Coverage tools not configured

**Critical Code Sections** (high priority for testing if framework adopted):
- Thread-safe audio processing paths (no synchronization bugs)
- MIDI note-to-frequency conversion with custom tunings
- File I/O error handling (Scala file parsing)
- Edge cases in pitch bend calculations
- Sympathetic coupling matrix calculations

## Testing Without Framework

### Manual Verification Checklist Pattern

**Example from codebase** (`WaveguideString::calculateFeedbackCoefficient()`):
```cpp
float calculateFeedbackCoefficient() const
{
    // v1.2.0: Length modifier - longer strings (high length) have slower decay
    // Length=0 → 0.7x decay time (faster), Length=0.5 → 1.0x, Length=1.0 → 1.6x (slower)
    // This creates audible difference: short strings = punchy/quick, long strings = sustained/diffuse
    float lengthDecayModifier = 0.7f + lengthAmount * 0.9f;
    // ... calculation logic with expected behavior documented
}
```

**Verification Methods Embedded in Code:**
- Mathematical formulas documented with test cases
- Inline comments describe expected behavior ranges
- Version annotations track when behavior changed

### Parameter Boundary Testing

**Convention:** All parameters use `juce::jlimit()` for bounds checking
```cpp
double newFreq = juce::jlimit(400.0, 480.0, freqHz);  // Master tune limited to 400-480 Hz
pitchBendRange = juce::jlimit(1.0f, 48.0f, semitones);  // Pitch bend 1-48 semitones
pluckPosition = juce::jlimit(0.0f, 1.0f, position);  // Normalized 0-1 range
```

**Manual Testing Pattern:**
- Verify extreme values (0.0, 1.0) produce expected behavior
- Test just-outside-bounds values get clamped correctly
- Listen for artifacts at boundary transitions

## Thread Safety Testing

**Critical Audio Thread Code:**
Located in `/Users/taylorbrook/Dev/VST-development/plugins/OuariconLyrica/Source/`:

**Lock-Free MPSC Queue** (`PluginProcessor.h`):
```cpp
class MidiEventQueue {
    void push(const MidiNoteEvent& event) { /* try-lock pattern */ }
    bool pop(MidiNoteEvent& event) { /* lock-free read */ }
};
```

**Manual Testing:**
- Trigger MIDI notes rapidly (stress test)
- Monitor for dropped events in UI visualization
- Verify no audio glitches on parameter changes
- Check for race conditions when loading Scala files mid-playback

**Atomic Updates:**
Double-buffered coupling matrix in `SympatheticResonanceEngine`:
```cpp
std::atomic<Mode> currentMode { Mode::TwelveTET };
std::atomic<float> notePitchBends;
```

Manual verification: Parameter updates mid-note don't cause clicks or pops

## Platform-Specific Testing

**Target Platforms:**
- macOS (AU, VST3, Standalone) - primary development platform
- Windows (VST3) - supported via build
- Linux - not targeted

**Manual Testing Checklist:**
- macOS: Verified with Logic Pro, Reaper
- Cross-plugin compatibility: MIDI controller input routing
- Standalone mode: File save/load, preset management

## Known Testing Gaps

**1. No Automated Test Suite:**
- Risk: Regressions introduced with DSP parameter changes
- Impact: Physical modeling accuracy untested
- Priority: High (Phase 3 candidate)

**2. No Floating-Point Precision Tests:**
- Current: Manual listening verification
- Risk: Frequency calculation drift over long playback
- Impact: Tuning accuracy degradation over session

**3. No MIDI Event Coverage:**
- Current: Manual note triggering
- Risk: Edge cases in velocity/channel handling missed
- Impact: Polyphonic note handling bugs

**4. No File I/O Error Tests:**
- Current: Happy-path only (valid Scala files)
- Risk: Malformed files crash or corrupt state
- Impact: User-provided tuning files cause instability

**5. No Performance Benchmarks:**
- Current: Subjective "smooth" verification
- Risk: CPU load increases unnoticed
- Impact: Plugin unusable on low-spec machines

## Recommended Testing Framework

**If Adopting Unit Tests**, follow these conventions:

**Test File Locations:**
```
OuariconLyrica/
├── Source/
│   ├── TuningEngine.h
│   ├── TuningEngine.cpp
│   └── Tests/           ← NEW
│       └── TuningEngineTests.cpp
├── CMakeLists.txt
└── CMakeListsTest.txt   ← NEW (test target)
```

**Test Structure Example** (recommended pattern):
```cpp
// TuningEngineTests.cpp - hypothetical
#include <catch2/catch.hpp>
#include "../DSP/TuningEngine.h"

TEST_CASE("TuningEngine: 12-TET frequency calculation") {
    TuningEngine engine;

    SECTION("A4 at standard 440 Hz") {
        double freq = engine.getFrequency(69);  // MIDI note 69 = A4
        REQUIRE(std::abs(freq - 440.0) < 0.01);
    }

    SECTION("Master tune 432 Hz") {
        engine.setMasterTune(432.0);
        double freq = engine.getFrequency(69);
        REQUIRE(std::abs(freq - 432.0) < 0.01);
    }
}

TEST_CASE("TuningEngine: Scala file loading") {
    TuningEngine engine;
    juce::File scalaFile("test-tunings/just-intonation.scl");

    REQUIRE(engine.loadScalaFile(scalaFile) == true);
    REQUIRE(engine.getMode() == TuningEngine::Mode::Scala);
}
```

**Test Execution Command** (projected):
```bash
cmake .. -DBUILD_TESTING=ON
cmake --build . --target OuariconLyricaTests
ctest --output-on-failure
```

## Manual Testing Best Practices (Current)

**Systematic Approach:**
1. Document test scenario in `.claude/skills/plugin-testing/`
2. Create HTML mockup of expected UI state in `.ideas/mockups/`
3. Manually test in DAW
4. Record audio output for archival
5. Update regression test checklist in improvement docs

**Version Control for Tests:**
- Manual test results recorded in version tags
- Backup versions in `/backups/` directory before major changes
- Improvement notes link to specific commits

---

*Testing analysis: 2026-01-22*
