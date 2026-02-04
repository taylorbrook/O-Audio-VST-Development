# scala-tuning-engine v2.1.0

Complete microtonal tuning system for WebView-based JUCE plugins.

## Quick Start

For step-by-step integration, see **`snippets/INTEGRATION-CHECKLIST.md`**.

The `snippets/` directory contains copy-paste ready code for:
- `parameters.cpp` - APVTS parameter definitions
- `processor-header.h` - PluginProcessor.h additions
- `native-functions.cpp` - All 20+ WebView native functions
- `persistence.cpp` - DAW session state save/restore
- `tuning-panel.css` - Complete CSS styles for the UI

## Features

### Tuning Modes
- **12-TET**: Standard 12-tone equal temperament
- **Custom/Scala**: Load .scl files or edit intervals directly
- **MTS-ESP**: Placeholder for future MTS-ESP integration

### Visualizations (5 modes)
- **Circle**: Traditional pitch circle with intervals as spokes
- **Polar**: Radial plot showing interval positions
- **Matrix**: Grid showing interval cents between all scale degrees
- **TrueKeys**: Real-time display of intervals between held notes
- **Rotation**: Modal rotation table for all tonics

### Scale Generator
- **EDO**: Equal Division of Octave (2-72 divisions, custom period)
- **Harmonic Series**: Generate from overtone ratios (harmonics 1-64)
- **Rank-2 Temperaments**: Meantone-style scales with custom generator

### Factory Library (24+ presets)
| Category | Tunings |
|----------|---------|
| Historical | Young 1799, Neidhardt III, Kellner Bach, Bach/Lehman, Valotti |
| Just Intonation | Ptolemy, 5-Limit JI, 7-Limit JI, Partch 43-Tone |
| Equal Divisions | 17-EDO, 19-EDO, 22-EDO, 31-EDO, 41-EDO, 53-EDO |
| Non-Octave | Bohlen-Pierce, Carlos Alpha/Beta/Gamma |
| World | Arabic 24-TET, Turkish Makam, Indian 22-Shruti, Gamelan |

### File I/O
- Load/Save Scala .scl files
- Load/Save Keyboard Mapping .kbm files
- Export HTML documentation with SVG pitch circle

### Additional Features
- Editable intervals table with tonic selector
- Octave stretch (0.95-1.25) for physical modeling
- Reference pitch (A4) control (400-480 Hz)
- Thread-safe frequency table for lock-free audio access
- Per-note pitch bend support

## Installation

### 1. Copy C++ Files

Copy from `cpp/` to your plugin's Source folder:
```
TuningEngine.h / TuningEngine.cpp
ScaleGenerator.h / ScaleGenerator.cpp
TuningExporter.h / TuningExporter.cpp
EmbeddedTunings.h / EmbeddedTunings.cpp
```

### 2. Add to CMakeLists.txt

```cmake
target_sources(${PROJECT_NAME} PRIVATE
    Source/TuningEngine.cpp
    Source/ScaleGenerator.cpp
    Source/TuningExporter.cpp
    Source/EmbeddedTunings.cpp
)
```

### 3. Create APVTS Parameters

```cpp
juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Reference pitch (A4)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "tuning_masterTune", "Master Tune",
        juce::NormalisableRange<float>(400.0f, 480.0f, 0.1f),
        440.0f));

    // Tuning mode
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        "tuning_tuningMode", "Tuning Mode",
        juce::StringArray{"12-TET", "Custom", "MTS-ESP"}, 0));

    // Octave stretch
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "tuning_octaveStretch", "Octave Stretch",
        juce::NormalisableRange<float>(0.95f, 1.25f, 0.01f),
        1.0f));

    // Temperament preset
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        "tuning_temperamentPreset", "Temperament",
        juce::StringArray{"12-TET", "Pythagorean", "Zarlino", "Meantone",
                          "Werckmeister III", "Kirnberger III", "Vallotti",
                          "Well Tempered", "Just Intonation", "Bohlen-Pierce", "Custom"},
        0));

    return { params.begin(), params.end() };
}
```

### 4. Add Native Functions to PluginEditor

```cpp
void PluginEditor::setupNativeFunctions()
{
    // Tuning intervals
    webView->bind("getTuningIntervals", [this]() {
        auto intervals = processor.tuningEngine.getIntervals();
        juce::String json = "[";
        for (size_t i = 0; i < intervals.size(); ++i) {
            if (i > 0) json += ",";
            json += juce::String(intervals[i]);
        }
        json += "]";
        return json;
    });

    webView->bind("setTuningIntervals", [this](const juce::var& args) {
        // Parse JSON array and apply
        // ...
    });

    webView->bind("setSingleInterval", [this](int index, double cents) {
        processor.tuningEngine.setSingleInterval(index, cents);
        return true;
    });

    // ... Add all native functions from module.yaml
}
```

### 5. Include JavaScript Module

Copy `js/tuning-panel.js` to your Resources/ui/js/ folder.

In your index.html:
```html
<div id="tuning-container"></div>

<script type="module">
    import { TuningPanel } from './js/tuning-panel.js';

    const panel = new TuningPanel(
        document.getElementById('tuning-container'),
        window.__JUCE__
    );

    panel.init();
</script>
```

## Usage in Audio Processing

```cpp
void PluginProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    for (const auto metadata : midi) {
        auto msg = metadata.getMessage();

        if (msg.isNoteOn()) {
            int note = msg.getNoteNumber();
            double freq = tuningEngine.getFrequency(note);
            // Use freq for synthesis...
        }
    }
}
```

## State Persistence

The tuning engine state should be saved with your plugin's session:

```cpp
void PluginProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();

    // Add tuning custom state
    auto tuningState = state.getOrCreateChildWithName("tuning", nullptr);

    auto intervals = tuningEngine.getIntervals();
    juce::String intervalsStr;
    for (size_t i = 0; i < intervals.size(); ++i) {
        if (i > 0) intervalsStr += ",";
        intervalsStr += juce::String(intervals[i]);
    }
    tuningState.setProperty("intervals", intervalsStr, nullptr);
    tuningState.setProperty("scaleName", tuningEngine.getActiveTuningName(), nullptr);
    tuningState.setProperty("tonic", tuningEngine.getTonicNote(), nullptr);

    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}
```

## Integration Approach

This module uses the **standalone ES6 class approach** rather than surgical integration. This is appropriate because:

1. **Self-contained panel** - The tuning system is a complete UI panel, not something that integrates into existing structures
2. **No namespace conflicts** - Uses `.tuning-*` CSS prefix, doesn't share selectors with plugin CSS
3. **Complexity** - 880 lines of JS with 5 visualization modes would be unwieldy as inline snippets
4. **API cohesion** - The 20+ native functions form a logical API managed by a single class

For simpler modules that integrate INTO existing UI (like footer keyboards), see `instrument-footer-panel` which uses surgical integration with inline snippets.

## Breaking Changes from v1.0.0

- **New API**: TuningEngine class has expanded methods
- **New classes**: Added ScaleGenerator, TuningExporter, EmbeddedTunings
- **JavaScript**: Rewritten as ES6 module (was inline script)
- **Parameters**: Now use configurable prefix (default `tuning_`)

## License

Part of the Ouaricon Audio module library.
