# vu-meter

Thread-safe VU metering for Ouaricon plugins.

## Features

- Lock-free atomic level storage for audio thread safety
- Smooth animated displays with configurable ballistics
- Multiple display styles: LED bar, smooth bar, needle
- Optional peak hold with configurable decay
- Easy integration with WebView via evaluateJavascript

## Installation

### CMake Integration

```cmake
include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)
ouaricon_add_module(MyPlugin vu-meter)
```

## C++ Usage

### In PluginProcessor

```cpp
#include "VUMeterBridge.h"

class MyProcessor : public juce::AudioProcessor
{
public:
    VUMeterBridge meters;

    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override
    {
        // ... your processing ...

        // Store peak levels for metering
        float inputPeak = getPeakMagnitude(buffer, 0, buffer.getNumSamples());
        // ... process audio ...
        float outputPeak = getPeakMagnitude(buffer, 0, buffer.getNumSamples());

        meters.storeLevels(inputPeak, outputPeak);
    }

    // Expose for Editor
    float getInputLevelDB() const { return meters.getInputLevelDB(); }
    float getOutputLevelDB() const { return meters.getOutputLevelDB(); }
};
```

### In PluginEditor

```cpp
class MyEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    MyEditor(MyProcessor& p) : processorRef(p)
    {
        startTimerHz(30);  // 30fps meter updates
    }

private:
    void timerCallback() override
    {
        // Send meter values to WebView
        juce::String script = processorRef.meters.generateMeterScript();
        webView->evaluateJavascript(script, nullptr);
    }
};
```

### With Additional Values (Gain Reduction, Envelope)

```cpp
// In processBlock:
std::vector<float> aux = { gainReductionDB, envelopeDB };
meters.storeAuxLevels(aux);

// In timerCallback:
juce::String script = processorRef.meters.generateMeterScriptWithAux(2);
webView->evaluateJavascript(script, nullptr);
```

## JavaScript Usage

### LED Bar Meter

```html
<div id="input-meter" style="width: 20px; height: 200px;"></div>
<div id="output-meter" style="width: 20px; height: 200px;"></div>

<script type="module">
    import { LEDMeter, createMeterPair } from './modules/vu-meter.js';

    // Option 1: Create a meter pair (registers updateMeters automatically)
    const { inputMeter, outputMeter } = createMeterPair(
        document.getElementById('input-meter'),
        document.getElementById('output-meter'),
        { segments: 14 }
    );

    // Option 2: Manual setup
    const meter = new LEDMeter({
        container: document.getElementById('my-meter'),
        segments: 12,
        minDB: -60,
        maxDB: 0,
        colors: {
            low: '#00FF00',
            mid: '#FFFF00',
            high: '#FF0000',
            inactive: '#333333'
        }
    });

    // Register your own update handler
    window.updateMeters = function(inputDB, outputDB) {
        meter.setLevel(outputDB);
    };
</script>
```

### Smooth Bar Meter

```javascript
import { SmoothMeter } from './modules/vu-meter.js';

const meter = new SmoothMeter({
    container: document.getElementById('meter'),
    gradient: ['#4CAF50', '#8BC34A', '#FFEB3B', '#FF5722'],
    direction: 'vertical'
});
```

### Needle Meter

```javascript
import { NeedleMeter } from './modules/vu-meter.js';

// Default: 180° sweep with dynamic green-to-red color
const meter = new NeedleMeter({
    container: document.getElementById('vu-dial')
});

// Custom colors and angles
const customMeter = new NeedleMeter({
    container: document.getElementById('vu-dial'),
    minAngle: -90,
    maxAngle: 90,
    colorLow: '#00FF00',   // Green at min
    colorHigh: '#FF0000',  // Red at max
    // needleColor: '#2C3E50'  // Set this to disable dynamic color
});
```

### With Auxiliary Values

```javascript
import { LEDMeter, registerMeterUpdate } from './modules/vu-meter.js';

const inputMeter = new LEDMeter({ container: document.getElementById('input') });
const outputMeter = new LEDMeter({ container: document.getElementById('output') });
const grMeter = new LEDMeter({ container: document.getElementById('gr') });

registerMeterUpdate(
    { input: inputMeter, output: outputMeter },
    [
        (gainReduction) => grMeter.setLevel(-gainReduction),  // GR is inverted
        (envelope) => { /* handle envelope */ }
    ]
);
```

## Configuration Options

### C++ (VUMeterBridge)

| Property | Default | Description |
|----------|---------|-------------|
| minDB | -60.0f | Minimum dB level |
| maxDB | 3.0f | Maximum dB level |

### JavaScript (All Meters)

| Property | Default | Description |
|----------|---------|-------------|
| minDB | -60 | Minimum dB level |
| maxDB | 3 | Maximum dB level |
| attackSpeed | 0.4 | Attack interpolation (0-1) |
| releaseSpeed | 0.15 | Release interpolation (0-1) |
| peakHoldMs | 1000 | Peak hold time (0 to disable) |

### LEDMeter Specific

| Property | Default | Description |
|----------|---------|-------------|
| segments | 14 | Number of LED segments |
| colors.low | '#8BA870' | Low level color |
| colors.mid | '#6B8E4E' | Mid level color |
| colors.high | '#C9A27B' | High/warning color |
| colors.inactive | '#8B7355' | Inactive segment color |
| thresholds.mid | 0.5 | Threshold for mid color |
| thresholds.high | 0.85 | Threshold for high color |

### NeedleMeter Specific

| Property | Default | Description |
|----------|---------|-------------|
| minAngle | -90 | Needle angle at minimum level (degrees) |
| maxAngle | 90 | Needle angle at maximum level (degrees) |
| colorLow | '#4CAF50' | Needle color at minimum (green) |
| colorHigh | '#F44336' | Needle color at maximum (red) |
| needleColor | null | Static color (disables gradient if set) |

## Version History

### 1.0.0 (2026-01-12)
- Initial extraction from OuariconComp
- LED bar, smooth bar, and needle display styles
- Configurable attack/release ballistics
- Optional peak hold
