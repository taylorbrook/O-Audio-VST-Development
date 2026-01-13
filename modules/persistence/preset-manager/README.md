# preset-manager

JSON-based preset persistence for Ouaricon plugins.

## Features

- Factory and user preset separation
- Next/previous navigation
- DAW session save/restore
- Custom state callbacks for tuning data, etc.
- JavaScript UI module with ready-made components

## Installation

### CMake Integration

In your plugin's `CMakeLists.txt`:

```cmake
include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)

juce_add_plugin(MyPlugin ...)

ouaricon_add_module(MyPlugin preset-manager)
```

### Manual Integration

1. Copy `cpp/OuariconPresetManager.h` to your Source directory
2. Copy `js/preset-manager.js` to your `ui/public/modules/` directory

## C++ Usage

### Basic Setup

```cpp
#include "OuariconPresetManager.h"

class MyProcessor : public juce::AudioProcessor
{
public:
    MyProcessor()
        : parameters(*this, nullptr, "PARAMS", createLayout())
        , presetManager(parameters, "My Plugin Name")
    {
    }

    void getStateInformation(juce::MemoryBlock& destData) override
    {
        if (auto xml = presetManager.getStateAsXml())
            copyXmlToBinary(*xml, destData);
    }

    void setStateInformation(const void* data, int size) override
    {
        if (auto xml = getXmlFromBinary(data, size))
            presetManager.setStateFromXml(xml.get());
    }

private:
    juce::AudioProcessorValueTreeState parameters;
    OuariconPresetManager presetManager;
};
```

### With Custom State (e.g., Tuning)

```cpp
// In constructor:
presetManager.setCustomStateCallbacks(
    // Save callback
    [this]() -> juce::var {
        auto* obj = new juce::DynamicObject();
        obj->setProperty("tuningMode", tuningMode);
        obj->setProperty("referencePitch", referencePitch);
        return juce::var(obj);
    },
    // Load callback
    [this](const juce::var& data) {
        if (auto* obj = data.getDynamicObject())
        {
            tuningMode = obj->getProperty("tuningMode");
            referencePitch = obj->getProperty("referencePitch");
        }
    }
);
```

### Register Native Functions (WebView)

```cpp
// In PluginEditor constructor:
webView = std::make_unique<juce::WebBrowserComponent>(
    juce::WebBrowserComponent::Options{}
        .withNativeIntegrationEnabled()
        .withNativeFunction("savePreset", [this](auto& args, auto complete) {
            if (args.size() > 0)
                complete(processorRef.presetManager.savePreset(args[0].toString()));
            else
                complete(false);
        })
        .withNativeFunction("loadPreset", [this](auto& args, auto complete) {
            if (args.size() > 0)
                complete(processorRef.presetManager.loadPreset(args[0].toString()));
            else
                complete(false);
        })
        .withNativeFunction("getPresetList", [this](auto&, auto complete) {
            auto list = processorRef.presetManager.getPresetList();
            juce::Array<juce::var> arr;
            for (const auto& name : list)
                arr.add(name);
            complete(juce::var(arr));
        })
        .withNativeFunction("getCurrentPreset", [this](auto&, auto complete) {
            complete(processorRef.presetManager.getCurrentPresetName());
        })
        .withNativeFunction("selectNextPreset", [this](auto&, auto complete) {
            auto next = processorRef.presetManager.getNextPreset();
            complete(next);
        })
        .withNativeFunction("selectPreviousPreset", [this](auto&, auto complete) {
            auto prev = processorRef.presetManager.getPreviousPreset();
            complete(prev);
        })
        .withNativeFunction("deletePreset", [this](auto& args, auto complete) {
            if (args.size() > 0)
                complete(processorRef.presetManager.deletePreset(args[0].toString()));
            else
                complete(false);
        })
        .withNativeFunction("isFactoryPreset", [this](auto& args, auto complete) {
            if (args.size() > 0)
                complete(processorRef.presetManager.isFactoryPreset(args[0].toString()));
            else
                complete(false);
        })
        // ... other options
);
```

### Initialize Factory Presets

```cpp
// In processor constructor (once):
std::vector<OuariconPresetManager::FactoryPresetDef> factoryPresets = {
    {
        "Default",
        {{"GAIN", 0.5f}, {"FREQUENCY", 0.3f}},
        juce::var()  // No custom state
    },
    {
        "Bright",
        {{"GAIN", 0.7f}, {"FREQUENCY", 0.8f}},
        juce::var()
    }
};

presetManager.initializeFactoryPresets(factoryPresets);
```

## JavaScript Usage

### Module Import

```html
<script type="module">
    import { PresetManager, createPresetBar } from './modules/preset-manager.js';

    // Option 1: Create full preset bar automatically
    const manager = createPresetBar('preset-container');

    // Option 2: Custom setup
    const presets = new PresetManager({
        displayElement: document.getElementById('preset-name'),
        prevButton: document.getElementById('prev-btn'),
        nextButton: document.getElementById('next-btn'),
        onPresetChanged: (name) => console.log('Loaded:', name)
    });
    presets.initialize();
</script>
```

### Global Usage (Non-module)

```html
<script src="./modules/preset-manager.js"></script>
<script>
    const manager = new OuariconPresetManager({
        displayElement: document.getElementById('preset-name')
    });
    manager.initialize();
</script>
```

## Directory Structure

Presets are stored in:
```
~/Library/Application Support/{Plugin Name}/Presets/
├── Factory/
│   ├── Default.json
│   └── Bright.json
└── User/
    ├── My Sound.json
    └── Lead Patch.json
```

## Version History

### 1.0.0 (2026-01-12)
- Initial extraction from OuariconMarimba
- Genericized for multi-plugin use
- Added callback interface for custom state
- JavaScript UI module with ready-made components
