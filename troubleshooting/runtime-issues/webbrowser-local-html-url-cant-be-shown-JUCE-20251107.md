---
plugin: JUCE
date: 2025-11-07
problem_type: runtime_error
component: webview
symptoms:
  - 'WebBrowserComponent shows "The URL can''t be shown" error'
  - 'file:// URLs fail to load in WKWebView on macOS'
  - HTML files never display despite successful build
  - WebView window appears blank/black
root_cause: url_protocol
juce_version: 8.0.9
resolution_type: api_migration
severity: critical
tags: [webview, webbrowser, wkwebview, resource-provider, file-url, juce8]
---

# Troubleshooting: WebBrowserComponent Shows "The URL Can't Be Shown"

## Problem
WebBrowserComponent in JUCE 8 displays "The URL can't be shown" error when attempting to load local HTML files using file:// URLs. The HTML never renders despite files existing and being readable.

## Environment
- JUCE Version: 8.0.9
- Affected: WebBrowserComponent (juce_gui_basics), PluginEditor
- Platform: macOS (WKWebView backend)
- Date: 2025-11-07

## Symptoms
- WebBrowserComponent shows "The URL can't be shown" system error
- file:// URL loading fails silently (no logs, no exceptions)
- Plugin builds successfully (VST3/AU/Standalone)
- HTML files exist and are readable from disk
- WebView window appears blank or black
- Works on older JUCE versions but breaks in JUCE 8

## What Didn't Work

**Attempted Solution 1:** Load HTML from app Resources folder
- **Why it failed:** macOS sandboxing and WKWebView security model block direct file:// access to bundle resources. WKWebView runs in a separate process with strict security restrictions.

**Attempted Solution 2:** Copy HTML files to temp directory and use file:// URLs
- **Why it failed:** WKWebView on macOS blocks file:// URLs by default due to security policies. Using `juce::URL(juce::File).toString(false)` + `goToURL()` is the JUCE 7 pattern and doesn't work with WKWebView's security model in JUCE 8.

**Attempted Solution 3:** Use `loadFileURL:allowingReadAccessTo:` (native WKWebView method)
- **Why it failed:** This is the correct native API, but JUCE 8's WebBrowserComponent doesn't expose it directly. JUCE provides a different pattern: resource providers.

## Solution

JUCE 8 introduced the **resource provider pattern** for serving local files to WebBrowserComponent. Instead of file:// URLs, use `Options::withResourceProvider()` to serve HTML/CSS/JS from memory.

**Code changes:**

```cpp
// PluginEditor.h

class ShakeAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    ShakeAudioProcessorEditor(ShakeAudioProcessor& p);
    // ...

private:
    // Resource provider callback
    std::optional<juce::WebBrowserComponent::Resource>
    handleResourceRequest(const juce::String& path);

    // WebBrowserComponent with Options (JUCE 8 API)
    juce::WebBrowserComponent webBrowser{
        juce::WebBrowserComponent::Options{}
            .withResourceProvider(
                [this](const juce::String& path) {
                    return handleResourceRequest(path);
                }
            )
    };

    // UI content stored in memory
    juce::String indexHTML;
    juce::String stylesCSS;
    juce::String scriptJS;
};
```

```cpp
// PluginEditor.cpp

ShakeAudioProcessorEditor::ShakeAudioProcessorEditor(ShakeAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    // Load UI files into memory at startup
    juce::File uiDir = juce::File(__FILE__).getParentDirectory().getChildFile("ui");

    if (uiDir.exists())
    {
        indexHTML = uiDir.getChildFile("index.html").loadFileAsString();
        stylesCSS = uiDir.getChildFile("styles.css").loadFileAsString();
        scriptJS = uiDir.getChildFile("script.js").loadFileAsString();
    }

    addAndMakeVisible(webBrowser);

    // Navigate to resource provider root (triggers "/" request → index.html)
    webBrowser.goToURL(juce::WebBrowserComponent::getResourceProviderRoot());

    setSize(600, 400);
}

// Resource provider callback - serves content from memory
std::optional<juce::WebBrowserComponent::Resource>
ShakeAudioProcessorEditor::handleResourceRequest(const juce::String& path)
{
    DBG("Resource request: " + path);

    // Helper to convert String to std::vector<std::byte>
    auto stringToBytes = [](const juce::String& str) -> std::vector<std::byte>
    {
        auto utf8 = str.toUTF8();
        std::vector<std::byte> bytes;
        bytes.reserve(utf8.sizeInBytes());

        for (size_t i = 0; i < utf8.sizeInBytes(); ++i)
            bytes.push_back(static_cast<std::byte>(utf8[i]));

        return bytes;
    };

    // Root path → serve index.html
    if (path == "/")
    {
        return juce::WebBrowserComponent::Resource{
            stringToBytes(indexHTML),
            "text/html"
        };
    }

    // Serve CSS
    if (path == "/styles.css")
    {
        return juce::WebBrowserComponent::Resource{
            stringToBytes(stylesCSS),
            "text/css"
        };
    }

    // Serve JavaScript
    if (path == "/script.js")
    {
        return juce::WebBrowserComponent::Resource{
            stringToBytes(scriptJS),
            "application/javascript"
        };
    }

    // Resource not found
    return std::nullopt;
}
```

**Key changes from JUCE 7 pattern:**
1. **Constructor:** WebBrowserComponent now uses `Options` builder pattern with `withResourceProvider()`
2. **No file:// URLs:** Remove all `juce::URL(juce::File)` code
3. **No temp directory:** Files load into memory, no filesystem access at runtime
4. **Navigation:** Use `getResourceProviderRoot()` instead of file URLs
5. **Resource callback:** Implement callback to serve content based on path

## Why This Works

### Root Cause
WKWebView (macOS WebView backend) has strict security policies that prevent direct file:// URL loading. This is by design - WKWebView runs in a separate process with sandboxing enabled.

### Why Resource Provider Works
1. **juce:// scheme:** Resource provider uses `juce://` scheme (macOS) or `https://` (Windows) instead of `file://`, which WKWebView accepts
2. **No filesystem access:** Resources served from memory via callback - no runtime file I/O that sandbox blocks
3. **JUCE 8 API:** The `Options` pattern with `withResourceProvider()` is the official JUCE 8 way to serve local content
4. **Callback pattern:** Acts like a lightweight web server - WebView requests paths, callback returns content
5. **Type safety:** Uses `std::vector<std::byte>` for binary data (JUCE 8 requirement)

### Technical Details
- `getResourceProviderRoot()` returns the base URL (e.g., `juce://` on macOS)
- When WebView navigates to this URL, it triggers a request for "/"
- Your callback receives "/", returns HTML with `text/html` MIME type
- HTML references CSS/JS with relative paths (`<link href="styles.css">`)
- WebView makes additional requests for `/styles.css`, `/script.js`
- Callback serves each resource with appropriate MIME type

## Prevention

### When Using WebBrowserComponent in JUCE 8:

1. **Always use resource provider pattern** for local HTML/CSS/JS:
   ```cpp
   juce::WebBrowserComponent webBrowser{
       juce::WebBrowserComponent::Options{}
           .withResourceProvider([this](auto path) {
               return handleRequest(path);
           })
   };
   ```

2. **Never use file:// URLs** - they don't work with WKWebView security model

3. **Load files at startup** into `juce::String` members, serve from memory in callback

4. **For production builds**, use `juce_add_binary_data()` to embed files at compile time:
   ```cmake
   juce_add_binary_data(MyPluginUIData
       NAMESPACE MyPluginUI
       SOURCES
           Source/ui/index.html
           Source/ui/styles.css
           Source/ui/script.js)

   target_link_libraries(MyPlugin PRIVATE MyPluginUIData)
   ```
   Then replace `loadFileAsString()` with `BinaryData::index_html`, etc.

5. **Return `std::vector<std::byte>`** from resource provider (not `juce::String`)

6. **Set correct MIME types:**
   - HTML: `"text/html"`
   - CSS: `"text/css"`
   - JavaScript: `"application/javascript"`
   - Images: `"image/png"`, `"image/jpeg"`, etc.

7. **Test early** - verify HTML loads in Standalone app before testing in DAWs

### Migration Checklist from JUCE 7

If migrating existing WebView code:
- [ ] Replace default constructor with `Options{}` builder
- [ ] Add `withResourceProvider()` callback
- [ ] Remove all `juce::URL(juce::File)` code
- [ ] Remove temp directory extraction code
- [ ] Implement resource callback with path-based routing
- [ ] Change `goToURL()` to use `getResourceProviderRoot()`
- [ ] Convert strings to `std::vector<std::byte>` in callback
- [ ] Add MIME type for each resource type

## Related Issues

- See official JUCE blog: "JUCE 8 Feature Overview: WebView UIs" (https://juce.com/blog/juce-8-feature-overview-webview-uis/)
- See JUCE API docs: `WebBrowserComponent::Options::withResourceProvider()`

No other troubleshooting docs related to WebView yet.
