# WebView UI Integration Checklist - v[N]

**Plugin:** [PluginName]
**UI Version:** v[N]
**Date:** [timestamp]

## Prerequisites

- [ ] UI design approved (tested in browser)
- [ ] parameter-spec.md finalized
- [ ] Plugin shell exists (Stage 2 complete)

## Step 1: Copy HTML to Plugin

```bash
# Create UI directory
mkdir -p plugins/[PluginName]/Source/ui

# Copy production HTML
cp plugins/[PluginName]/.ideas/mockups/v[N]-ui.html \
   plugins/[PluginName]/Source/ui/index.html
```

**Verify:**

- [ ] HTML file exists at `Source/ui/index.html`
- [ ] File contains production UI code (not browser test)

## Step 2: Update PluginEditor.h

**Add WebView component:**

```cpp
#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class PluginEditor : public juce::AudioProcessorEditor
{
public:
    PluginEditor(PluginProcessor&);
    ~PluginEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    PluginProcessor& audioProcessor;
    juce::WebBrowserComponent webView;  // Add this

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEditor)
};
```

**Verify:**

- [ ] `juce::WebBrowserComponent webView;` declared
- [ ] `#include <JuceHeader.h>` present

## Step 3: Update PluginEditor.cpp

**Initialize WebView and load HTML:**

```cpp
#include "PluginEditor.h"

PluginEditor::PluginEditor(PluginProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    // Set editor size (match YAML window dimensions)
    setSize(600, 400);

    // Add WebView to editor
    addAndMakeVisible(webView);

    // Load HTML from binary data
    auto html = juce::String(BinaryData::index_html, BinaryData::index_htmlSize);
    webView.goToURL("data:text/html," + html);

    // Setup parameter listeners
    for (auto* param : audioProcessor.getParameters())
    {
        param->addListener(this);
    }
}

PluginEditor::~PluginEditor()
{
    // Remove parameter listeners
    for (auto* param : audioProcessor.getParameters())
    {
        param->removeListener(this);
    }
}

void PluginEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black);
}

void PluginEditor::resized()
{
    webView.setBounds(getLocalBounds());
}

// Parameter change → UI update
void PluginEditor::parameterValueChanged(int parameterIndex, float newValue)
{
    auto* param = audioProcessor.getParameters()[parameterIndex];
    auto paramID = param->getName(100);

    // Send to JavaScript
    juce::String js = "receiveParameterUpdate('" + paramID + "', " + juce::String(newValue) + ");";
    webView.evaluateJavascript(js);
}

// UI change → Parameter update (handle messages from JavaScript)
// (Implement message handler based on JUCE version)
```

**Verify:**

- [ ] WebView initialized in constructor
- [ ] HTML loaded from binary data
- [ ] Parameter listeners added
- [ ] `resized()` sets WebView bounds
- [ ] Parameter changes push updates to UI

## Step 4: Update CMakeLists.txt

**Enable WebView and bundle HTML:**

```cmake
# Add binary data for HTML
juce_add_binary_data([PluginName]Assets
    SOURCES
        Source/ui/index.html
)

# Plugin configuration
juce_add_plugin([PluginName]
    COMPANY_NAME "YourCompany"
    PLUGIN_MANUFACTURER_CODE "Ycom"
    PLUGIN_CODE "Plug"
    FORMATS VST3 AU
    PRODUCT_NAME "[ProductName]"
    NEEDS_WEB_BROWSER TRUE  # Enable WebView
)

# Link libraries
target_link_libraries([PluginName]
    PRIVATE
        juce::juce_audio_plugin_client
        juce::juce_audio_utils
        juce::juce_gui_extra  # WebView support
        [PluginName]Assets    # HTML resources
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
)
```

**Verify:**

- [ ] `NEEDS_WEB_BROWSER TRUE` set
- [ ] Binary data target created
- [ ] `juce::juce_gui_extra` linked
- [ ] HTML assets linked

## Step 5: Build and Test

```bash
cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build . --parallel
```

**Verify:**

- [ ] Build succeeds without errors
- [ ] No WebView-related warnings
- [ ] Binary data embedded correctly

## Step 6: Test in Standalone

```bash
# Run standalone app
./build/plugins/[PluginName]/[PluginName]_artefacts/Debug/Standalone/[ProductName].app/Contents/MacOS/[ProductName]
```

**Check:**

- [ ] Plugin window opens
- [ ] UI loads and displays correctly
- [ ] All controls visible
- [ ] Window size matches spec (600x400 or custom)
- [ ] No console errors

**Interact with controls:**

- [ ] Knobs/sliders respond to mouse
- [ ] Values update in UI
- [ ] Parameters change in audio processing
- [ ] No lag or freezing

## Step 7: Test in DAW

```bash
# Install plugin (macOS)
cp -r ./build/plugins/[PluginName]/[PluginName]_artefacts/Debug/VST3/[ProductName].vst3 \
      ~/Library/Audio/Plug-Ins/VST3/

# Or use /install-plugin command
```

**Load in DAW (Logic, Ableton, Reaper):**

- [ ] Plugin appears in DAW plugin list
- [ ] Plugin loads without crashes
- [ ] UI displays correctly in plugin window
- [ ] All controls functional
- [ ] Parameter automation works
- [ ] Preset recall works
- [ ] Project save/load preserves state

## Troubleshooting

### WebView doesn't load

**Check:**

```cpp
// Verify HTML loaded
auto html = juce::String(BinaryData::index_html, BinaryData::index_htmlSize);
DBG("HTML size: " + juce::String(BinaryData::index_htmlSize));
```

**Solution:** Rebuild CMake to regenerate binary data.

### UI shows blank window

**Check:**

```cpp
// Verify goToURL called
webView.goToURL("data:text/html," + html);
```

**Solution:** Ensure HTML is valid, check browser console for errors.

### Controls don't respond

**Check JavaScript console** (if accessible):

- Parameter bindings set correctly (`data-param` attributes)
- Event listeners attached
- `window.juce` object available

**Solution:** Verify HTML matches v[N]-ui.html exactly.

### Parameters don't update

**Check:**

- Parameter listeners added in constructor
- `parameterValueChanged()` implemented
- JavaScript function `receiveParameterUpdate()` exists

**Solution:** Add debug logging to verify messages sent.

## Success Criteria

UI integration is complete when:

- ✅ Plugin builds without errors
- ✅ UI loads in standalone app
- ✅ All controls visible and functional
- ✅ Parameters update correctly (C++ ↔ JavaScript)
- ✅ Plugin works in DAW
- ✅ Automation and preset recall work

## Next Steps

After successful integration:

1. Continue with automatic validation
2. Run pluginval tests
3. Manual DAW testing
4. Install for production use
