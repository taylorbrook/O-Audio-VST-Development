# C++ Boilerplate Generation for WebView UI

## Header Generation (v[N]-PluginEditor.h)

**Template replacement strategy:**

1. **Include guards:** `#ifndef [PLUGIN_NAME_UPPER]_PLUGINEDITOR_H`
2. **WebView integration:** Include `<juce_gui_extra/juce_gui_extra.h>`
3. **Member variables:**
   - `std::unique_ptr<juce::WebBrowserComponent> webView;`
   - `juce::AudioProcessorValueTreeState& apvts;`
4. **Constructor:** `PluginEditor(AudioProcessor& p, juce::AudioProcessorValueTreeState& vts);`
5. **Destructor:** `~PluginEditor() override;`
6. **JUCE overrides:**
   - `void paint(juce::Graphics&) override;`
   - `void resized() override;`

**Complete template:**

```cpp
#ifndef [PLUGIN_NAME_UPPER]_PLUGINEDITOR_H
#define [PLUGIN_NAME_UPPER]_PLUGINEDITOR_H

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include "PluginProcessor.h"

class [PluginName]Editor : public juce::AudioProcessorEditor
{
public:
    [PluginName]Editor([PluginName]Processor& p, juce::AudioProcessorValueTreeState& vts);
    ~[PluginName]Editor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    [PluginName]Processor& audioProcessor;
    juce::AudioProcessorValueTreeState& apvts;
    std::unique_ptr<juce::WebBrowserComponent> webView;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR([PluginName]Editor)
};

#endif // [PLUGIN_NAME_UPPER]_PLUGINEDITOR_H
```

## Implementation Generation (v[N]-PluginEditor.cpp)

**Template replacement strategy:**

1. **Constructor implementation:**
   - Call `setSize(width, height)` from YAML
   - Initialize APVTS reference
   - Create WebBrowserComponent
   - Load HTML file: `webView->goToURL(juce::File::getCurrentWorkingDirectory().getChildFile("v[N]-ui.html").getFullPathName())`
   - Add parameter listeners for each YAML control

2. **Destructor:** Empty or cleanup listeners

3. **paint():** Fallback background (webView handles UI)

4. **resized():** `webView->setBounds(getLocalBounds())`

**Complete template:**

```cpp
#include "PluginEditor.h"

[PluginName]Editor::[PluginName]Editor([PluginName]Processor& p, juce::AudioProcessorValueTreeState& vts)
    : AudioProcessorEditor(&p), audioProcessor(p), apvts(vts)
{
    setSize([WIDTH_FROM_YAML], [HEIGHT_FROM_YAML]);

    webView = std::make_unique<juce::WebBrowserComponent>();
    addAndMakeVisible(webView.get());

    // Load HTML
    auto htmlFile = juce::File::getCurrentWorkingDirectory().getChildFile("v[N]-ui.html");
    webView->goToURL(htmlFile.getFullPathName());

    // Parameter listeners (generate for each control in YAML)
    // [FOR_EACH_CONTROL]
}

[PluginName]Editor::~[PluginName]Editor()
{
}

void [PluginName]Editor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black); // Fallback
}

void [PluginName]Editor::resized()
{
    webView->setBounds(getLocalBounds());
}
```

## Replacement Variable Mapping

**Extract from context:**
- `[PLUGIN_NAME_UPPER]` - Plugin name in uppercase (e.g., LUSHVERB)
- `[PluginName]` - Plugin name in PascalCase (e.g., LushVerb)
- `[WIDTH_FROM_YAML]` - window.width from YAML
- `[HEIGHT_FROM_YAML]` - window.height from YAML
- `[N]` - Current version number (e.g., 1, 2, 3)
- `[FOR_EACH_CONTROL]` - Loop through YAML controls array

**Parameter listener generation:**
For each control in YAML `controls` array:
```cpp
apvts.addParameterListener("[control.id]", this);
```
