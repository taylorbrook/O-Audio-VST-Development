# Analog EQ Unit

A 4-band analog-style parametric EQ module designed for embedding in VST instruments as an effects tab. Based on the DSP from Ouaricon Analog Equalizer.

## Features

- **LF Shelf** (30-500 Hz) - Low frequency boost/cut
- **LMF Bell** (100-2000 Hz) - Low-mid parametric with variable Q
- **HMF Bell** (500-8000 Hz) - High-mid parametric with variable Q
- **HF Shelf** (2000-20000 Hz) - High frequency boost/cut
- **Analog Saturation** - Subtle `tanh` warmth circuit
- **Output Gain** - ±12 dB trim

## Dual-Ring Knob Design

Each EQ band features a distinctive dual-ring knob that controls two parameters:

```
    ╭─────────────╮
    │  ┌───────┐  │  ← Outer Ring: FREQUENCY
    │  │ ┌───┐ │  │     - Green gradient border
    │  │ │   │ │  │     - SVG notches for visual reference
    │  │ │ ● │ │  │  ← Inner Dial: GAIN
    │  │ │   │ │  │     - Seed/wheat conic pattern
    │  │ └───┘ │  │     - Dark pointer indicator
    │  └───────┘  │
    ╰─────────────╯
```

**Interaction:**
- Click **outer ring** (>60% from center) → adjust frequency
- Click **inner dial** (≤60% from center) → adjust gain
- **Drag vertically** to change value
- **Double-click** to reset to center position
- **Tooltip** shows both values: "500 Hz / +3.0 dB"

## Quick Start

### 1. Add to Your Plugin's Parameter Layout

```cpp
#include "AnalogEQUnit.h"

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // Your existing parameters...
    layout.add(std::make_unique<juce::AudioParameterFloat>(...));

    // Add EQ parameters with prefix to avoid conflicts
    auto eqParams = AnalogEQUnit::createParameterLayout("fx_eq_");
    for (auto& param : eqParams)
        layout.add(std::move(param));

    return layout;
}
```

### 2. Add to Your Processor

```cpp
// PluginProcessor.h
#include "AnalogEQUnit.h"

class MyPluginProcessor : public juce::AudioProcessor
{
public:
    juce::AudioProcessorValueTreeState parameters;

private:
    AnalogEQUnit eqUnit;  // Declare after parameters
};

// PluginProcessor.cpp
MyPluginProcessor::MyPluginProcessor()
    : parameters(*this, nullptr, "PARAMS", createParameterLayout())
    , eqUnit(parameters, "fx_eq_")  // Same prefix as createParameterLayout
{
}

void MyPluginProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    eqUnit.prepare(sampleRate, samplesPerBlock, getTotalNumOutputChannels());
}

void MyPluginProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    // Your synth/effect processing...

    // Then apply EQ
    eqUnit.process(buffer);
}
```

### 3. Add the WebView UI

```html
<!-- In your index.html effects tab -->
<div id="eq-panel"></div>

<script type="module">
import { AnalogEQUnitUI } from './modules/analog-eq-unit.js';
import { getSliderState, getToggleState, getComboBoxState } from './js/juce/index.js';

const eqUI = new AnalogEQUnitUI({
    container: document.getElementById('eq-panel'),
    paramPrefix: 'fx_eq_',
    getSliderState: getSliderState,
    getToggleState: getToggleState,
    getComboBoxState: getComboBoxState,
    showMeter: true,
    onReady: () => console.log('EQ UI ready')
});

eqUI.initialize();
</script>
```

## Parameter Reference

All parameters use a configurable prefix (default: `eq_`).

| Parameter | Type | Range | Default | Description |
|-----------|------|-------|---------|-------------|
| `{prefix}lf_freq` | float | 30-500 Hz | 100 Hz | LF shelf corner frequency |
| `{prefix}lf_gain` | float | ±12 dB | 0 dB | LF shelf boost/cut |
| `{prefix}lf_on` | bool | - | true | LF band enable |
| `{prefix}lmf_freq` | float | 100-2000 Hz | 500 Hz | LMF bell center frequency |
| `{prefix}lmf_gain` | float | ±12 dB | 0 dB | LMF bell boost/cut |
| `{prefix}lmf_q` | choice | WIDE/MED/TIGHT | MED | LMF Q factor |
| `{prefix}lmf_on` | bool | - | true | LMF band enable |
| `{prefix}hmf_freq` | float | 500-8000 Hz | 2000 Hz | HMF bell center frequency |
| `{prefix}hmf_gain` | float | ±12 dB | 0 dB | HMF bell boost/cut |
| `{prefix}hmf_q` | choice | WIDE/MED/TIGHT | MED | HMF Q factor |
| `{prefix}hmf_on` | bool | - | true | HMF band enable |
| `{prefix}hf_freq` | float | 2000-20000 Hz | 8000 Hz | HF shelf corner frequency |
| `{prefix}hf_gain` | float | ±12 dB | 0 dB | HF shelf boost/cut |
| `{prefix}hf_on` | bool | - | true | HF band enable |
| `{prefix}output_gain` | float | ±12 dB | 0 dB | Master output trim |
| `{prefix}analog` | bool | - | true | Analog saturation enable |

## WebView Relay Setup

For the UI to work, you need to create WebView relays and attachments in your editor:

```cpp
// PluginEditor.h
class MyPluginEditor : public juce::AudioProcessorEditor
{
private:
    // Create relays for each EQ parameter
    std::unique_ptr<juce::WebSliderRelay> eqLfFreqRelay;
    std::unique_ptr<juce::WebSliderRelay> eqLfGainRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> eqLfOnRelay;
    // ... etc for all 16 parameters

    // Attachments
    std::unique_ptr<juce::WebSliderParameterAttachment> eqLfFreqAttachment;
    // ... etc
};

// PluginEditor.cpp - Constructor
MyPluginEditor::MyPluginEditor(MyPluginProcessor& p)
    : AudioProcessorEditor(p)
{
    // 1. Create relays FIRST
    eqLfFreqRelay = std::make_unique<juce::WebSliderRelay>("fx_eq_lf_freq");
    eqLfGainRelay = std::make_unique<juce::WebSliderRelay>("fx_eq_lf_gain");
    eqLfOnRelay = std::make_unique<juce::WebToggleButtonRelay>("fx_eq_lf_on");
    // ... create all relays

    // 2. Create WebView with relay options
    webView = std::make_unique<juce::WebBrowserComponent>(
        juce::WebBrowserComponent::Options{}
            .withOptionsFrom(*eqLfFreqRelay)
            .withOptionsFrom(*eqLfGainRelay)
            .withOptionsFrom(*eqLfOnRelay)
            // ... add all relays
    );

    // 3. Create attachments LAST
    eqLfFreqAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *p.parameters.getParameter("fx_eq_lf_freq"), *eqLfFreqRelay, nullptr);
    // ... create all attachments
}
```

### Helper: Creating All Relays

Use the static helper to get all parameter IDs:

```cpp
auto eqParamIDs = AnalogEQUnit::getParameterIDs("fx_eq_");
// Returns: ["fx_eq_lf_freq", "fx_eq_lf_gain", "fx_eq_lf_on", ...]
```

## DSP Signal Flow

```
Input
  │
  ├──► LF Shelf Filter (if lf_on)
  │
  ├──► LMF Bell Filter (if lmf_on)
  │
  ├──► HMF Bell Filter (if hmf_on)
  │
  ├──► HF Shelf Filter (if hf_on)
  │
  ├──► Analog Saturation (if analog_on)
  │    └── tanh(x * 0.5) * 2.0
  │
  └──► Output Gain
       │
       ▼
    Output
```

## Q Factor Values

| Setting | Q Value | Character |
|---------|---------|-----------|
| WIDE | 0.5 | Broad, gentle curves |
| MED | 1.0 | Balanced, musical |
| TIGHT | 2.0 | Focused, surgical |

## Thread Safety

- All parameter reads use atomic operations
- Filter coefficient updates only occur when parameters change
- Output level meter uses relaxed atomic store/load
- Safe for real-time audio thread usage

## UI Customization

The JavaScript UI uses CSS custom properties for easy theming:

```css
.eq-unit {
    --eq-bg: #1a1a1a;
    --eq-accent: #5a9c4f;
    --eq-text: #ccc;
}
```

## VU Meter Integration

To use the optional VU meter, poll the output level from a timer:

```cpp
void MyPluginEditor::timerCallback()
{
    float levelDB = audioProcessor.eqUnit.getOutputLevelDB();
    webView->emitEventIfBrowserIsVisible("eqOutputLevel", levelDB);
}
```

```javascript
// In your WebView
window.__JUCE__.backend.addEventListener('eqOutputLevel', (level) => {
    eqUI.updateMeter(level);
});
```

## Version History

### 1.0.0 (2026-01-13)
- Initial release
- 4-band EQ with analog saturation
- Compact WebView UI component
- Based on Ouaricon Analog Equalizer DSP
