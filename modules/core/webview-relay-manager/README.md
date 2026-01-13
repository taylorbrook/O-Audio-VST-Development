# webview-relay-manager

WebView relay and attachment lifecycle management for JUCE 8 plugins.

## The Problem

JUCE 8's WebView parameter attachments call `evaluateJavascript()` during destruction. If the WebView is destroyed before the attachments, this causes a crash (EXC_BAD_ACCESS).

C++ destroys class members in **reverse declaration order**. This means:

```cpp
// WRONG - Will crash in release builds!
class MyEditor {
    std::unique_ptr<juce::WebBrowserComponent> webView;      // Declared first
    std::unique_ptr<juce::WebSliderRelay> relay;             // Declared second
    std::unique_ptr<juce::WebSliderParameterAttachment> att; // Declared third
};

// Destruction order: att (calls webView->evaluateJavascript()) → relay → webView
// webView is already destroyed when attachment tries to use it! 💥
```

## The Solution

Declare members in this order:

1. **Relays FIRST** (destroyed last)
2. **WebView SECOND**
3. **Attachments LAST** (destroyed first - WebView still alive)

## Installation

```cmake
include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)
ouaricon_add_module(MyPlugin webview-relay-manager)
```

## Usage

### Option 1: Manual Pattern (Recommended)

```cpp
class MyEditor : public juce::AudioProcessorEditor
{
public:
    MyEditor(MyProcessor& p) : processorRef(p)
    {
        // 1️⃣ Create relays FIRST
        gainRelay = std::make_unique<juce::WebSliderRelay>("gain");
        bypassRelay = std::make_unique<juce::WebToggleButtonRelay>("bypass");

        // 2️⃣ Create WebView SECOND (with relay options)
        webView = std::make_unique<juce::WebBrowserComponent>(
            juce::WebBrowserComponent::Options{}
                .withNativeIntegrationEnabled()
                .withResourceProvider([this](auto& url) { return getResource(url); })
                .withOptionsFrom(*gainRelay)
                .withOptionsFrom(*bypassRelay)
        );
        addAndMakeVisible(*webView);

        // 3️⃣ Create attachments LAST
        // CRITICAL: Pass nullptr as third argument (undoManager)
        gainAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
            *processorRef.parameters.getParameter("gain"), *gainRelay, nullptr);
        bypassAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
            *processorRef.parameters.getParameter("bypass"), *bypassRelay, nullptr);
    }

private:
    MyProcessor& processorRef;

    // ⚠️ CRITICAL: Declaration order determines destruction order!

    // 1️⃣ RELAYS FIRST (destroyed last)
    std::unique_ptr<juce::WebSliderRelay> gainRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> bypassRelay;

    // 2️⃣ WEBVIEW SECOND
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // 3️⃣ ATTACHMENTS LAST (destroyed first - can safely use webView)
    std::unique_ptr<juce::WebSliderParameterAttachment> gainAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> bypassAttachment;
};
```

### Option 2: Using WebViewRelayManager Class

```cpp
#include "WebViewRelayManager.h"

class MyEditor : public juce::AudioProcessorEditor
{
public:
    MyEditor(MyProcessor& p) : processorRef(p)
    {
        // Step 1: Create relays
        relayManager.createSliderRelay("gain");
        relayManager.createToggleRelay("bypass");

        // Step 2: Initialize WebView
        auto* webView = relayManager.initializeWebView(
            [this](const auto& url) { return getResource(url); });
        addAndMakeVisible(*webView);

        // Step 3: Create attachments
        relayManager.createSliderAttachment("gain",
            *processorRef.parameters.getParameter("gain"));
        relayManager.createToggleAttachment("bypass",
            *processorRef.parameters.getParameter("bypass"));

        webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());
    }

private:
    MyProcessor& processorRef;
    WebViewRelayManager relayManager;  // Handles lifecycle automatically
};
```

### Option 3: Using Macros

```cpp
#include "WebViewRelayManager.h"

class MyEditor : public juce::AudioProcessorEditor
{
public:
    MyEditor(MyProcessor& p) : processorRef(p)
    {
        // Create relays
        OUARICON_CREATE_RELAY(gain, "gain");
        OUARICON_CREATE_TOGGLE_RELAY(bypass, "bypass");

        // Create WebView (manually - macros don't help here)
        webView = std::make_unique<juce::WebBrowserComponent>(
            juce::WebBrowserComponent::Options{}
                .withNativeIntegrationEnabled()
                .withOptionsFrom(*gainRelay)
                .withOptionsFrom(*bypassRelay)
        );

        // Create attachments
        OUARICON_CREATE_ATTACHMENT(gain, processorRef.parameters.getParameter("gain"));
        OUARICON_CREATE_TOGGLE_ATTACHMENT(bypass, processorRef.parameters.getParameter("bypass"));
    }

private:
    MyProcessor& processorRef;

    // Declare in correct order using macros
    OUARICON_DECLARE_RELAY(gain);
    OUARICON_DECLARE_TOGGLE_RELAY(bypass);

    std::unique_ptr<juce::WebBrowserComponent> webView;

    OUARICON_DECLARE_ATTACHMENT(gain);
    OUARICON_DECLARE_TOGGLE_ATTACHMENT(bypass);
};
```

## Common Mistakes

### 1. Wrong Declaration Order

```cpp
// ❌ WRONG
std::unique_ptr<juce::WebBrowserComponent> webView;
std::unique_ptr<juce::WebSliderRelay> gainRelay;
std::unique_ptr<juce::WebSliderParameterAttachment> gainAttachment;

// ✅ CORRECT
std::unique_ptr<juce::WebSliderRelay> gainRelay;
std::unique_ptr<juce::WebBrowserComponent> webView;
std::unique_ptr<juce::WebSliderParameterAttachment> gainAttachment;
```

### 2. Forgetting the nullptr Argument

```cpp
// ❌ WRONG - Missing third argument
gainAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
    *param, *relay);  // Will fail silently!

// ✅ CORRECT - Include nullptr for UndoManager
gainAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
    *param, *relay, nullptr);
```

### 3. Using Raw Pointers

```cpp
// ❌ WRONG - Raw pointers don't have deterministic destruction
juce::WebSliderRelay* gainRelay = new juce::WebSliderRelay("gain");

// ✅ CORRECT - Use unique_ptr
std::unique_ptr<juce::WebSliderRelay> gainRelay;
```

## Debugging Crashes

If you get crashes during plugin unload:

1. Check member declaration order in the header file
2. Ensure WebView is declared AFTER relays, BEFORE attachments
3. Verify all attachments include the `nullptr` argument
4. Add logging to destructors to see destruction order

```cpp
~MyEditor()
{
    // Stop any timers first
    stopTimer();

    // Log destruction (remove in production)
    DBG("~MyEditor: Starting destruction");

    // Members will be destroyed in reverse declaration order
}
```

## Version History

### 1.0.0 (2026-01-12)
- Initial creation
- Documented Pattern 11 from juce8-critical-patterns.md
- Added WebViewRelayManager class
- Added convenience macros
