# compressor-unit

Compact dynamics compressor module for embedding in Ouaricon VST instruments.

## Overview

A 400x100px rack-mount style compressor strip with the essential controls from Ouaricon Compressor. Same high-quality DSP with a simplified interface optimized for effects tabs and channel strips.

## Features

- **4 Essential Controls**: Threshold, Ratio, Attack, Release
- **Fixed 6dB Soft Knee**: Musical response without parameter overload
- **Clickable Bypass**: Title acts as ON/OFF toggle
- **GR Meter**: Vertical LED strip showing gain reduction
- **Naturalist Aesthetic**: Seed-style knobs matching Ouaricon design language

## Parameters

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| `comp_enabled` | on/off | ON | Master bypass toggle |
| `comp_threshold` | -60 to 0 dB | -20 dB | Compression threshold |
| `comp_ratio` | 1:1 to 20:1 | 2:1 | Compression ratio |
| `comp_attack` | 0.1-100 ms | 10 ms | Attack time |
| `comp_release` | 10-1000 ms | 100 ms | Release time |

## Integration

### C++ (PluginProcessor)

```cpp
#include "modules/effects/compressor-unit/cpp/CompressorUnit.h"

class MyPluginProcessor : public juce::AudioProcessor
{
    CompressorUnit compressor;

    // In createParameterLayout():
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
    {
        juce::AudioProcessorValueTreeState::ParameterLayout layout;

        // Add your other parameters...

        // Add compressor parameters
        CompressorUnit::addParameters(layout);

        return layout;
    }

    // In prepareToPlay():
    void prepareToPlay(double sampleRate, int samplesPerBlock) override
    {
        compressor.prepare(sampleRate, samplesPerBlock);
    }

    // In processBlock():
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override
    {
        // Your other processing...

        // Process compression
        compressor.process(buffer, parameters);
    }

    // For UI metering:
    float getCompressorGR() const { return compressor.getGainReductionDB(); }
};
```

### JavaScript (WebView UI)

```javascript
import { CompressorUnit } from './modules/compressor-unit.js';
import { getSliderState, getToggleState } from './js/juce/index.js';

// Create instance
const compressor = new CompressorUnit({
  container: document.getElementById('compressor-container'),
  paramPrefix: 'comp_',
  getSliderState: getSliderState,
  getToggleState: getToggleState
});

// Initialize (renders UI and binds parameters)
compressor.initialize();

// Store reference for GR meter updates
window.compressorUnitInstance = compressor;
```

### Meter Updates from C++

In your PluginEditor, poll the GR value and send to WebView:

```cpp
// In timer callback (30Hz recommended):
void timerCallback() override
{
    float gr = processorRef.getCompressorGR();
    juce::String js = "updateCompressorGR(" + juce::String(gr) + ");";
    webView.evaluateJavascript(js);
}
```

## Layout

```
┌─────────────────────────────────────────────────────────────┐
│ COMPRESSOR [ON]   [○]   [○]   [○]   [○]        ▌█▌         │
│                  Thresh Ratio Attack Release    ▌█▌ GR     │
│                  -20dB  2:1   10ms   100ms      ▌ ▌         │
└─────────────────────────────────────────────────────────────┘
        400px × 100px
```

## DSP Algorithm

Uses the same compression algorithm as Ouaricon Compressor:

1. **Stereo-linked Detection**: Max of all channels for consistent stereo image
2. **Envelope Follower**: Per-sample attack/release with exponential coefficients
3. **Soft Knee**: Quadratic curve through knee region for musical transition
4. **Feed-forward Topology**: Predictable, clean compression character

## Changelog

### v1.0.0 (2026-01-14)
- Initial release
- Extracted from Ouaricon Compressor
- Compact 400x100px layout
- 4 essential parameters
- Vertical GR LED meter
