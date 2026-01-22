# Coding Conventions

**Analysis Date:** 2026-01-22

## Language & Framework

**Primary Language:** C++ (C++17 standard)
- JUCE framework 8.x for VST/AU plugin development
- All source files use `.h` (headers) and `.cpp` (implementation) convention

**File Headers:**
All source files include standardized header blocks:
```cpp
/*
  ==============================================================================

    FileName.h/cpp
    [Purpose/Phase]
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/
```

## Naming Patterns

**Files:**
- PascalCase for class-based files: `WaveguideString.h`, `TuningEngine.cpp`
- Lowercase for utility/module directories: `DSP/`, `modules/`
- Plugin-specific directories follow pattern: `OuariconLyrica/`, `Source/`

**Classes:**
- PascalCase: `OuariconLyricaAudioProcessor`, `WaveguideString`, `TuningEngine`
- Suffix patterns: `Engine`, `Voice`, `Sound`, `Controller`, `Manager`
- JUCE-derived classes inherit naming: `AudioProcessor`, `SynthesiserVoice`

**Functions/Methods:**
- camelCase: `getFrequency()`, `setMasterTune()`, `processSample()`, `prepareToPlay()`
- Getter pattern: `get` + PascalCase: `getAPVTS()`, `getSympatheticEngine()`, `getMaterial()`
- Setter pattern: `set` + PascalCase: `setMasterTune()`, `setBrightness()`, `setDamping()`
- Query pattern (bool return): `isActive()`, `canPlaySound()`, `isNoteMapped()`

**Member Variables:**
- camelCase: `currentFrequency`, `pluckPosition`, `feedbackCoefficient`
- Private members (no prefix): `currentSampleRate`, `currentEnergy`, `dampingAmount`
- Static constants: `UPPERCASE_WITH_UNDERSCORES`: `ENERGY_THRESHOLD = 0.0001f`, `NUM_STAGES = 4`, `NO_BEND = 2.0f`

**Enums:**
- PascalCase class names: `enum class Mode`, `enum class BuiltInPreset`, `enum class PlayingTechnique`
- UPPERCASE enum values: `Mode::TwelveTET`, `BuiltInPreset::PythagoreanMode`, `WoodType::Spruce`

**Type Aliases:**
- Not commonly used; JUCE types preferred directly (e.g., `float`, `double`, `int`)

## Code Style

**Formatting:**
- Tab indentation (4 spaces equivalent) - follows JUCE conventions
- Opening braces on same line: `if (condition) {`
- Closing braces on separate lines
- Line length: No strict limit observed (some lines exceed 100 chars)
- Space after control keywords: `if (condition)`, `for (int i = 0)`

**Braces & Control Flow:**
- Single-line bodies still use braces:
  ```cpp
  if (value > max) { return false; }
  ```
- Multi-statement blocks always use braces
- Early returns preferred for guard clauses:
  ```cpp
  if (condition) return;
  ```

## Import Organization

**Header Includes Order:**
1. JUCE header: `#include <JuceHeader.h>`
2. Standard library: `#include <vector>`, `#include <atomic>`, `#include <mutex>`
3. Local/relative: `#include "HarpSynthSound.h"`, `#include "DSP/TuningEngine.h"`

**Include Guards:**
All headers use modern C++ approach:
```cpp
#pragma once
```

**Example** (`TuningEngine.h`):
```cpp
#pragma once
#include <JuceHeader.h>
#include <vector>
#include <array>
#include <atomic>
#include <mutex>
```

## Documentation

**Comment Style:**
- Doxygen-style JSDoc/TSDoc comments for public functions
- Line comments (`//`) for implementation details
- Block comments (`/* */`) for headers and major sections

**Public Function Documentation:**
Comprehensive JSDoc-style comments with tags:
```cpp
/**
 * Get frequency for a MIDI note with optional pitch bend
 * @param midiNote MIDI note number (0-127)
 * @param midiChannel MIDI channel (0-15) - reserved for future use
 * @return Frequency in Hz
 */
double getFrequency(int midiNote, int midiChannel = 0);
```

**Version Annotations:**
Comments include version/phase information:
- Phase markers: `// Phase 2.7: Sympathetic Resonance Engine`
- Version markers: `// v1.3.2: Static atomic counter`
- Release notes inline: `// v1.11.1: Now initializes 12-TET if intervals are empty`

**Section Headers:**
Major sections delimited with visual separators:
```cpp
// ═══════════════════════════════════════════════════════════════════
// Core Settings
// ═══════════════════════════════════════════════════════════════════
```

## Error Handling

**Validation Pattern:**
Using JUCE's `juce::jlimit()` for parameter bounds checking:
```cpp
double newFreq = juce::jlimit(400.0, 480.0, freqHz);
pitchBendRange = juce::jlimit(1.0f, 48.0f, semitones);
pluckPosition = juce::jlimit(0.0f, 1.0f, position);
```

**Boolean Returns for Failure:**
File I/O and validation methods return `bool`:
```cpp
bool loadScalaFile(const juce::File& sclFile);
bool isNoteMapped(int midiNote) const;
bool popMidiEvent(MidiNoteEvent& event);
```

**Assertions & Null Checks:**
- Minimal assertion use; relies on constructor initialization
- Null pointer checks before dereferencing optional pointers:
  ```cpp
  if (tuningEngine != nullptr)
  {
      currentFrequency = tuningEngine->getFrequency(midiNoteNumber);
  }
  else
  {
      currentFrequency = juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber);
  }
  ```

**Error Prevention:**
- Thread-safe guard patterns (spinlocks, atomics):
  ```cpp
  bool expected = false;
  if (!pushLock.compare_exchange_strong(expected, true, std::memory_order_acquire))
      return;  // Another thread is pushing, drop this event
  ```

**Exception Handling:**
- No exceptions observed in codebase
- JUCE framework handles errors internally
- Silent fallbacks used (e.g., tuning engine defaults)

## Thread Safety

**Atomic Variables:**
Used extensively for lock-free synchronization on audio thread:
```cpp
std::atomic<size_t> writePos { 0 };
std::atomic<bool> pushLock { false };
std::atomic<Mode> currentMode { Mode::TwelveTET };
std::atomic<float> notePitchBends;
```

**Memory Ordering:**
Annotations used to optimize atomic operations:
```cpp
size_t currentWrite = writePos.load(std::memory_order_relaxed);
pushLock.store(false, std::memory_order_release);
```

**Double-Buffering:**
Used for lock-free updates in sympathetic resonance engine and filter coefficients

**Mutex Protection:**
Standard `std::mutex` for non-audio-thread operations:
```cpp
mutable std::mutex intervalMutex;
```

## Numeric Constants

**Floating Point:**
- Default literal suffix: `0.5f` for float
- No suffix for double in expressions: `440.0`, `0.99999f`
- Magic numbers documented with context:
  ```cpp
  float effectiveDecayTime = decayTimeSeconds * lengthDecayModifier;
  float perCycleDecay = std::pow(10.0f, -3.0f / cyclesForDecay);
  return juce::jlimit(0.9f, 0.99999f, perCycleDecay);
  ```

**Frequency Ranges:**
- MIDI note frequencies computed from base: A4 = 440.0 Hz (configurable)
- Low bound: 20.0 Hz
- High bound: `sampleRate * 0.45` (Nyquist-based)

## DSP Conventions

**Physical Modeling Patterns:**
Classes follow consistent DSP interface:
```cpp
void prepare(double sampleRate, int maxBlockSize);  // Initialization
void processSample();  // Per-sample processing
void reset();  // State clearing
void setParameter(float value);  // Parameter updates
```

**Delay Line Usage:**
JUCE's built-in interpolating delay lines:
```cpp
juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> upperRail;
```

**Filter Implementation:**
JUCE IIR filter classes with coefficient updates:
```cpp
juce::dsp::IIR::Filter<float> bridgeFilter;
```

## Resource Management

**JUCE Declarations:**
All plugin classes use JUCE macros:
```cpp
JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveguideString)
```

**Smart Pointers:**
Unique pointers for UI components and relays:
```cpp
std::unique_ptr<juce::WebSliderRelay> masterVolumeRelay;
std::shared_ptr<juce::FileChooser> fileChooser;
```

**Manual Deletion Prevention:**
Destructors marked `= default` when no cleanup needed:
```cpp
~StiffnessFilter();  // Declared, not defined
~TuningEngine() = default;  // Auto-generated
```

## Module & Namespace Conventions

**No Namespaces:**
Code exists in global scope; JUCE framework doesn't encourage explicit namespacing

**Self-Contained Modules:**
Each DSP component (`WaveguideString`, `TuningEngine`, etc.) is independent
- Clear public interface (header)
- Private implementation details
- No cross-module dependencies (except upward to processor)

## Initialization Patterns

**Constructor Defaults:**
Member variables initialized inline:
```cpp
double currentSampleRate = 44100.0;
float currentFrequency = 440.0;
```

**Factory Methods:**
Parameter layouts created via static factories:
```cpp
static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
```

**Lazy Initialization:**
Tuning modes support deferred loading:
```cpp
bool loadScalaFile(const juce::File& sclFile);
void setBuiltInPreset(BuiltInPreset preset);
```

---

*Convention analysis: 2026-01-22*
