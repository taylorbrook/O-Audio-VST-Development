# scala-tuning-engine

Complete microtonal tuning system with Scala file support for Ouaricon plugins.

## Features

- **Three Tuning Modes**: 12-TET, Custom (Scala), MTS-ESP (stubbed)
- **Scala File Support**: Load/save .scl and .kbm files
- **Thread-Safe**: Atomic frequency table for real-time audio
- **Tonic Transposition**: Shift scale root to any note
- **Dynamic Scale Sizes**: Support for any number of notes per octave
- **Visual Components**: Pitch circle, interval list, tonic selector

## Installation

```cmake
include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)
ouaricon_add_module(MyPlugin scala-tuning-engine)
```

## C++ Usage

### Basic Setup

```cpp
#include "OuariconTuningEngine.h"

class MySynthProcessor : public juce::AudioProcessor
{
public:
    OuariconTuningEngine tuning;

    // In synthesizer voice:
    void noteOn(int midiNote, float velocity)
    {
        double frequency = tuning.getFrequency(midiNote);
        oscillator.setFrequency(frequency);
    }
};
```

### Register Native Functions

```cpp
// In PluginEditor constructor:
webView = std::make_unique<juce::WebBrowserComponent>(
    juce::WebBrowserComponent::Options{}
        .withNativeIntegrationEnabled()

        .withNativeFunction("loadScalaFile", [this](auto& args, auto complete) {
            // Open file chooser and load .scl file
            fileChooser = std::make_unique<juce::FileChooser>(
                "Load Scala File", juce::File{}, "*.scl");
            fileChooser->launchAsync(juce::FileBrowserComponent::openMode, [this, complete](const auto& fc) {
                if (fc.getResult().existsAsFile()) {
                    bool success = processorRef.tuning.loadScalaFile(fc.getResult());
                    if (success) {
                        // Notify UI
                        auto name = processorRef.tuning.getActiveTuningName();
                        webView->evaluateJavascript(
                            "onScalaLoaded('" + name + "');", nullptr);
                    }
                    complete(success);
                } else {
                    complete(false);
                }
            });
        })

        .withNativeFunction("setTuningIntervals", [this](auto& args, auto complete) {
            if (args.size() >= 2) {
                std::vector<double> cents;
                if (auto* arr = args[0].getArray()) {
                    for (const auto& v : *arr)
                        cents.push_back(static_cast<double>(v));
                }
                juce::String name = args[1].toString();
                processorRef.tuning.setCustomIntervals(cents, name);
                complete(true);
            } else {
                complete(false);
            }
        })

        .withNativeFunction("getTuningIntervals", [this](auto&, auto complete) {
            auto* result = new juce::DynamicObject();
            juce::Array<juce::var> intervals;
            for (double c : processorRef.tuning.getIntervals())
                intervals.add(c);
            result->setProperty("intervals", juce::var(intervals));
            result->setProperty("name", processorRef.tuning.getActiveTuningName());
            complete(juce::var(result));
        })

        .withNativeFunction("setTonicNote", [this](auto& args, auto complete) {
            if (args.size() > 0) {
                processorRef.tuning.setTonicNote(static_cast<int>(args[0]));
                complete(true);
            } else {
                complete(false);
            }
        })

        // ... other options
);
```

### APVTS Parameters

```cpp
auto layout = std::make_unique<juce::AudioProcessorValueTreeState::ParameterLayout>();

layout->add(std::make_unique<juce::AudioParameterChoice>(
    juce::ParameterID { "TUNING_MODE", 1 },
    "Tuning Mode",
    juce::StringArray { "12-TET", "Custom", "MTS-ESP" },
    0));

layout->add(std::make_unique<juce::AudioParameterFloat>(
    juce::ParameterID { "REFERENCE_PITCH", 1 },
    "Reference Pitch",
    juce::NormalisableRange<float>(400.0f, 480.0f),
    440.0f,
    juce::AudioParameterFloatAttributes().withLabel("Hz")));
```

## JavaScript Usage

### Complete Tuning Panel

```html
<div id="tuning-container"></div>
<div id="pitch-circle"></div>

<script type="module">
    import { TuningPanel } from './modules/tuning-panel.js';
    import { PitchCircle } from './modules/pitch-circle.js';

    // Create pitch circle
    const circle = new PitchCircle({
        container: document.getElementById('pitch-circle'),
        size: 150,
        showLabels: true
    });

    // Create tuning panel with pitch circle
    const panel = new TuningPanel({
        container: document.getElementById('tuning-container'),
        pitchCircle: circle,
        onTuningChanged: (intervals, name) => {
            console.log('Tuning changed:', name, intervals);
        }
    });

    panel.initialize();

    // Callback from C++ when Scala file is loaded
    window.onScalaLoaded = async function(scaleName) {
        const result = await window.__JUCE__.backend.getTuningIntervals();
        if (result && result.intervals) {
            panel.onScalaLoaded(scaleName, result.intervals);
        }
    };
</script>
```

### Pitch Circle Only

```javascript
import { PitchCircle } from './modules/pitch-circle.js';

const circle = new PitchCircle({
    container: document.getElementById('circle'),
    size: 150,
    lineColor: '#6B8E4E',
    activeColor: '#DC0000'
});

// Update intervals
circle.setIntervals([0, 100, 200, 300, 400, 500, 600, 700, 800, 900, 1000, 1100]);

// Highlight active notes (called from C++)
window.setNoteActive = (midiNote, velocity) => circle.setNoteActive(midiNote, velocity);
window.setNoteInactive = (midiNote) => circle.setNoteInactive(midiNote);
```

## Built-in Scale Presets

| Preset | Description |
|--------|-------------|
| `12tet` | Standard 12-tone equal temperament |
| `just` | Just Intonation (5-limit) |
| `pythagorean` | Pythagorean tuning |
| `meantone` | Quarter-comma meantone |

## CSS Styling

The module uses these CSS classes that you can customize:

```css
.tuning-panel { /* Main container */ }
.tuning-mode-buttons { /* Mode selection buttons */ }
.scale-name-display { /* Current scale name */ }
.interval-list { /* Interval editing list */ }
.interval-item { /* Single interval row */ }
.interval-input { /* Cents/ratio input */ }
.tonic-selector { /* Tonic transposition control */ }
.file-buttons { /* Scala file load/save buttons */ }
.pitch-circle-svg { /* SVG pitch circle */ }
```

## State Persistence

Save/restore tuning state with presets:

```javascript
// Get current state
const state = panel.getState();
// Returns: { mode, intervals, scaleName, tonic, referencePitch }

// Restore state
panel.setState(savedState);
```

## Common Pitfalls

### 1. Wrong ComboBox API Method Names (Critical)

**Symptom:** Tuning tab appears but intervals don't load, keyboard doesn't play, Scala files don't affect tuning. No visible error - everything just silently fails.

**Root Cause:** JUCE 8 WebComboBoxRelay uses `getChoiceIndex()`/`setChoiceIndex()`, but it's easy to mistakenly use `getChosenIndex()`/`setChosenIndex()`.

```javascript
// WRONG - will silently crash the module
const mode = tuningModeState.getChosenIndex();
tuningModeState.setChosenIndex(1);

// CORRECT - JUCE 8 API
const mode = tuningModeState.getChoiceIndex();
tuningModeState.setChoiceIndex(1);
```

**Why it's dangerous:** ES6 modules abort on uncaught errors with no visible indication. If this error occurs early in the module, everything after it (interval loading, keyboard setup, etc.) never runs.

**Debugging tip:** If native functions aren't being called but other parts of the UI work, add try-catch blocks with native logging to find where the module is aborting:

```javascript
try {
    await Juce.getNativeFunction('debugLog')('Checkpoint 1');
    // ... code ...
    await Juce.getNativeFunction('debugLog')('Checkpoint 2');
} catch (e) {
    await Juce.getNativeFunction('debugLog')('ERROR: ' + e.message);
}
```

### 2. APVTS Parameter Override in processBlock

**Symptom:** Loading a Scala file works initially, but tuning reverts to 12-TET on the next audio block.

**Root Cause:** If `processBlock()` reads the APVTS tuning mode parameter and calls `tuningEngine.setMode()` every block, it will override any mode changes made directly to the TuningEngine.

**Solution:** When loading a Scala file via native function, also update the APVTS parameter:

```cpp
.withNativeFunction("loadScalaFile", [this](auto& args, auto complete) {
    // ... file chooser code ...
    if (processorRef.tuning.loadScalaFile(result)) {
        // CRITICAL: Also update APVTS so processBlock doesn't override
        if (auto* param = processorRef.getAPVTS().getParameter("TUNING_MODE"))
            param->setValueNotifyingHost(0.5f); // Index 1 = Custom (normalized)

        complete(juce::var(processorRef.tuning.getActiveTuningName()));
    }
});
```

### 3. Module Script Execution Order

**Symptom:** Global handlers (like `window.handleLoadSCL`) throw "JUCE not ready" errors.

**Root Cause:** Non-module `<script>` blocks run before `<script type="module">` blocks. If global handlers are defined in non-module scripts but rely on the Juce import from the module, they'll fail.

**Solution:** Expose the Juce API to global scope from the module:

```javascript
// In module script
import * as Juce from '/js/juce/index.js';
window.JuceAPI = Juce;  // Expose to global handlers

// In non-module script (runs first, but handlers called later)
window.handleLoadSCL = async function() {
    if (!window.JuceAPI) { console.error('JUCE not ready'); return; }
    await window.JuceAPI.getNativeFunction('loadScalaFile')();
};
```

## Version History

### 1.0.0 (2026-01-12)
- Initial extraction from OuariconMarimba
- Genericized C++ TuningEngine
- Modular JavaScript components (TuningPanel, PitchCircle)
- Built-in scale presets
