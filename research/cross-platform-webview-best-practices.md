---
title: "Cross-Platform WebView UI for JUCE Plugins: Best Practices Guide"
created: 2026-02-05
last_verified: 2026-02-06
juce_version: "8.0.4"
summary: "Best practices guide for cross-platform WebView UI in JUCE 8 plugins, covering WebView2 static linking on Windows, URL scheme differences, user data folder configuration, resource provider patterns, and CMake configuration for macOS/Windows/Linux."
domain: ui
type: guide
keywords:
  - webview
  - cross-platform
  - webview2
  - juce-gui
  - windows
  - macos
  - resource-provider
  - cmake
stages: [1, 3]
agents: [ui, build]
---

# Cross-Platform WebView UI for JUCE Plugins: Best Practices Guide

**Date:** 2026-02-06
**Purpose:** Inform the gui-agent about cross-platform WebView best practices for Windows + macOS compatibility
**JUCE Version:** 8.0.4
**Research Level:** Deep (Level 3 - JUCE source code analysis + Context7 docs + forum research)

---

## Executive Summary

JUCE's `WebBrowserComponent` uses different native engines on each platform:
- **macOS/iOS**: WKWebView (WebKit)
- **Windows**: Microsoft Edge WebView2 (Chromium) - requires explicit opt-in
- **Linux**: WebKit2GTK

The resource provider API (`withResourceProvider()`) provides a unified abstraction, but there are critical platform-specific configuration requirements that must be met, particularly on Windows where WebView2 requires both CMake flags AND compile definitions. **34 of 35 plugins in this project are currently missing Windows WebView2 configuration.**

---

## 1. Required CMake Configuration (Per Platform)

### The Complete Cross-Platform CMake Template

```cmake
juce_add_plugin(MyPlugin
    COMPANY_NAME "Ouaricon"
    PLUGIN_MANUFACTURER_CODE Ouar
    PLUGIN_CODE Xxxx
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "My Plugin"
    NEEDS_WEB_BROWSER TRUE      # Links webkit on Linux
    NEEDS_WEBVIEW2 TRUE         # Links WebView2LoaderStatic.lib on Windows
)

target_compile_definitions(MyPlugin
    PUBLIC
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=1
        JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1  # CRITICAL for Windows
        JUCE_USE_CURL=0
)

target_link_libraries(MyPlugin
    PRIVATE
        juce::juce_audio_processors
        juce::juce_gui_extra          # REQUIRED for WebBrowserComponent
)
```

### What Each Flag Does

| Flag | Layer | Platform | Effect |
|------|-------|----------|--------|
| `NEEDS_WEB_BROWSER TRUE` | CMake | Linux | Links `webkit2gtk` via pkg-config |
| `NEEDS_WEBVIEW2 TRUE` | CMake | Windows | Links `WebView2LoaderStatic.lib`, adds WebView2 headers |
| `JUCE_WEB_BROWSER=1` | C++ | All | Enables `WebBrowserComponent` class compilation |
| `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` | C++ | Windows | Uses static linking; auto-defines `JUCE_USE_WIN_WEBVIEW2=1` |
| `JUCE_USE_CURL=0` | C++ | All | Disables CURL (not needed for local HTML serving) |

### CRITICAL: The Static Linking Requirement

On Windows, there are two linking modes:

```
JUCE_USE_WIN_WEBVIEW2=1
  -> Dynamic loading: tries LoadLibraryA("WebView2Loader.dll") at runtime
  -> DLL must be distributed alongside plugin or found in system PATH
  -> If DLL not found: WebView silently shows BLANK (no error!)

JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
  -> Static linking: uses WebView2LoaderStatic.lib at link time
  -> No runtime DLL dependency
  -> Auto-defines JUCE_USE_WIN_WEBVIEW2=1 (see juce_gui_extra.h:97)
  -> RECOMMENDED for plugin distribution
```

**MUST use `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` when `NEEDS_WEBVIEW2 TRUE` is set.**
Source: `JUCE/modules/juce_gui_extra/native/juce_WebBrowserComponent_windows.cpp` lines 646-659

### macOS - No Special Flags Needed

macOS uses WKWebView which is always available. The `WebKit` framework is automatically linked by JUCE's module system (declared as `OSXFrameworks: WebKit` in the module header). No additional CMake flags are needed beyond `JUCE_WEB_BROWSER=1`.

---

## 2. URL Scheme Differences Between Platforms

The `getResourceProviderRoot()` function returns different URL schemes per platform:

| Platform | Resource Provider Root URL | Why |
|----------|---------------------------|-----|
| **macOS** | `juce://juce.backend/` | Custom URL scheme via `WKURLSchemeHandler` |
| **iOS** | `juce://juce.backend/` | Same as macOS |
| **Linux** | `juce://juce.backend/` | Custom scheme via `webkit_web_context_register_uri_scheme` |
| **Windows (WebView2)** | `https://juce.backend/` | WebView2 intercepts `https://juce.backend` via `WebResourceRequested` |
| **Android** | `https://juce.backend/` | Same as Windows |

**Impact on code:** This difference is handled transparently by JUCE. When you call `webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot())`, it automatically uses the correct scheme. The JavaScript `index.js` also handles this via `getBackendResourceAddress()`:

```javascript
function getBackendResourceAddress(path) {
  const platform = window.__JUCE__.initialisationData.__juce__platform[0];
  if (platform == "windows" || platform == "android")
    return "https://juce.backend/" + path;
  if (platform == "macos" || platform == "ios" || platform == "linux")
    return "juce://juce.backend/" + path;
}
```

**Best Practice:** Never hard-code URL schemes. Always use `getResourceProviderRoot()` in C++ and `getBackendResourceAddress()` in JavaScript.

---

## 3. Resource Provider Implementation

The resource provider pattern is cross-platform but has platform-specific internals:

### How Resources Are Served on Each Platform

**macOS (WKWebView):**
- Implements `WKURLSchemeHandler` protocol
- Registers `juce://` scheme via `setURLSchemeHandler:forURLScheme:@"juce"`
- Requires macOS 10.13+
- Returns `NSHTTPURLResponse` with Content-Type/Content-Length headers

**Windows (WebView2):**
- Uses `ICoreWebView2::add_WebResourceRequested` event filter
- Intercepts requests to `https://juce.backend/*`
- Creates response via `ICoreWebView2Environment::CreateWebResourceResponse()`
- Returns `SHCreateMemStream` for body data

**Linux (WebKit2GTK):**
- Registers `juce://` scheme via `webkit_web_context_register_uri_scheme()`
- Uses `WebKitURISchemeRequest` / `WebKitURISchemeResponse`

### Cross-Platform Resource Provider Code (Correct Pattern)

```cpp
std::optional<juce::WebBrowserComponent::Resource>
MyPluginEditor::getResource(const juce::String& url)
{
    auto makeVector = [](const char* data, int size) {
        return std::vector<std::byte>(
            reinterpret_cast<const std::byte*>(data),
            reinterpret_cast<const std::byte*>(data) + size
        );
    };

    // Root URL
    if (url == "/" || url == "/index.html") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::index_html, BinaryData::index_htmlSize),
            juce::String("text/html")
        };
    }

    // JUCE frontend library
    if (url == "/js/juce/index.js") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::index_js, BinaryData::index_jsSize),
            juce::String("text/javascript")
        };
    }

    // Native interop check (REQUIRED)
    if (url == "/js/juce/check_native_interop.js") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::check_native_interop_js,
                      BinaryData::check_native_interop_jsSize),
            juce::String("text/javascript")
        };
    }

    return std::nullopt;  // 404
}
```

**IMPORTANT:** The resource provider is NOT available on the IE backend (Windows without WebView2). The macro `JUCE_WEB_BROWSER_RESOURCE_PROVIDER_AVAILABLE` is only `1` when WebView2 is enabled on Windows.

---

## 4. JavaScript Bridge (Platform-Specific Integration)

### Layer 1: Platform-Specific `postMessage` (Injected Automatically)

Each platform injects a different `window.__JUCE__.postMessage` implementation:

**macOS/iOS/Linux:**
```javascript
window.__JUCE__ = {
  postMessage: function (object) {
    window.webkit.messageHandlers.__JUCE__.postMessage(object);
  },
};
```

**Windows (WebView2):**
```javascript
window.__JUCE__ = {
  postMessage: function(object) {
    window.chrome.webview.postMessage(object);
  },
};
```

**Impact:** This is completely transparent to plugin developers. The higher-level APIs (`getSliderState`, `getToggleState`, `emitEvent`) use `postMessage` internally.

### Layer 2: `check_native_interop.js` (Bootstrap)
Sets up `window.__JUCE__.backend` with `addEventListener()`, `removeEventListener()`, `emitEvent()`, and `emitByBackend()`. Also initializes `window.__JUCE__.initialisationData`.

### Layer 3: `index.js` (Public API)
Provides the user-facing API: `getSliderState()`, `getToggleState()`, `getComboBoxState()`, `getNativeFunction()`, `getBackendResourceAddress()`.

**Best Practice:** Always use the ES6 module API from `index.js`. Never directly call `window.__JUCE__.postMessage()` or access `window.chrome.webview` / `window.webkit.messageHandlers`.

---

## 5. Platform-Specific Options

### Windows-Only: `Options::WinWebView2`

```cpp
juce::WebBrowserComponent::Options{}
    .withWinWebView2Options(
        juce::WebBrowserComponent::Options::WinWebView2{}
            .withUserDataFolder(juce::File::getSpecialLocation(
                juce::File::tempDirectory).getChildFile("MyPlugin"))
            .withStatusBarDisabled()
            .withBuiltInErrorPageDisabled()
            .withBackgroundColour(juce::Colour(0xFF1a1a2e))  // Must be fully opaque or transparent
    )
```

**Critical: `withUserDataFolder()` for plugins.** WebView2 may be denied access to the default user data location in plugin hosts. Always specify a writable temp directory.

### Cross-Platform Options (Use These)

```cpp
juce::WebBrowserComponent::Options{}
    .withNativeIntegrationEnabled()
    .withResourceProvider([this](const auto& url) { return getResource(url); })
    .withKeepPageLoadedWhenBrowserIsHidden()  // Prevents about:blank on hide
    .withOptionsFrom(*gainRelay)
    .withOptionsFrom(*mixRelay)
```

### macOS-Only: `Options::AppleWkWebView`

```cpp
juce::WebBrowserComponent::Options{}
    .withAppleWkWebViewOptions(
        juce::WebBrowserComponent::Options::AppleWkWebView{}
            .withDisabledAcceptsFirstMouse()  // Disable click-through on unfocused windows
    )
```

---

## 6. Critical Cross-Platform Gotchas

### Gotcha 1: Silent Fallback to IE on Windows
If WebView2 construction fails (runtime not installed, environment creation fails, permission denied), JUCE **silently falls back to the ancient IE backend**. No error, no warning. The resource provider API is NOT available on the IE backend, so your plugin will show nothing.

**Mitigation:** Always use `NEEDS_WEBVIEW2 TRUE` + `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` to ensure WebView2 is properly linked.

### Gotcha 2: WebView2 User Data Folder Permissions
WebView2 creates a user data folder for caches, cookies, etc. In plugin contexts, the default location (`%LOCALAPPDATA%`) may not be writable. Always specify a custom location.

### Gotcha 3: Evaluation Error Handling Asymmetry
- **macOS, iOS, Linux:** `evaluateJavascript()` returns errors with type, message, source URL, line/column
- **Windows, Android:** Errors are **indistinguishable from success returning null**. `getError()` always returns `nullptr`.

**Mitigation:** Don't rely on JS evaluation error handling. Use defensive JavaScript code with try/catch and console logging.

### Gotcha 4: WebView2 Serial Construction
WebView2 instances are created serially on Windows (one at a time). If multiple `WebBrowserComponent`s are constructed simultaneously, later ones queue. This can cause startup delays.

### Gotcha 5: macOS Keyboard Shortcuts
macOS WKWebView in JUCE requires a custom `WebViewKeyEquivalentResponder` for Cmd+X/C/V/A. This is handled internally, but be aware that custom keyboard shortcuts may need special handling.

### Gotcha 6: `withKeepPageLoadedWhenBrowserIsHidden()`
Without this option, JUCE navigates to `about:blank` when the component is hidden (e.g., when the plugin editor is closed). This is particularly relevant for FL Studio which hides/shows plugin windows frequently. **Always use this option.**

### Gotcha 7: WebView2 NuGet Package Version
The CI workflow installs `Microsoft.Web.WebView2` version `1.0.1901.177`. The JUCE `FindWebView2.cmake` script searches for `*Microsoft.Web.WebView2*` in the NuGet packages directory. Ensure version compatibility.

### Gotcha 8: Resource Provider Requires macOS 10.13+
The custom URL scheme handler (`setURLSchemeHandler:forURLScheme:`) requires macOS 10.13 (High Sierra) or later. This is unlikely to be an issue but worth noting.

### Gotcha 9: evaluateJavascript() Before Page Load = Infinite Loop (Windows)
Calling `evaluateJavascript()` or `emitEventIfBrowserIsVisible()` before `pageFinishedLoading()` causes an infinite loop when multiple `WebBrowserComponent` instances exist on Windows. Fixed on JUCE `develop` branch (Jan 2025).
**Mitigation:** Never call JS evaluation until after the page has loaded. Use `pageFinishedLoading()` callback.
Source: [Forum: WebView Freezes DAW](https://forum.juce.com/t/br-webview-freezes-daw/64917)

### Gotcha 10: Rapid Open/Close Crashes PluginVal (macOS)
Asynchronous URL request callbacks can execute after WebView component destruction during rapid open/close cycles (PluginVal testing). Fixed in JUCE (Jan 7, 2025).
**Mitigation:** Add a 1-second throttle on WebView creation for PluginVal compatibility if on older JUCE.
Source: [Forum: WebView crashes PluginVal](https://forum.juce.com/t/webview-crashes-pluginval/64703)

### Gotcha 11: Options Builder Returns New Objects (Discards Silently)
The `Options` builder methods return new objects; they do NOT modify in-place. This is a common C++ error:
```cpp
// WRONG - setting is silently discarded!
auto options = WebBrowserComponent::Options{};
options.withBackend(Options::Backend::webview2);  // Return value ignored!

// CORRECT - chain calls or capture return value
auto options = WebBrowserComponent::Options{}
    .withBackend(Options::Backend::webview2);
```
**Verification:** Use `WebBrowserComponent::areOptionsSupported(options)` before instantiation.
Source: [Forum: Force WebView2 Backend](https://forum.juce.com/t/how-to-force-webview2-backend/65547)

### Gotcha 12: NuGet Auto-Install Broken (Windows CI)
The embedded NuGet installation in JUCE's CMake find module stopped working around May 2024. Manually install the package in CI:
```yaml
- name: Install WebView2 NuGet Package
  run: |
    Install-Package Microsoft.Web.WebView2 -RequiredVersion 1.0.1901.177 -Force
```
Source: [Forum: JUCE8 Windows WebView2](https://forum.juce.com/t/juce8-windows-webview2/61576)

---

## 7. Current Project Status: What's Missing

### Audit Results: 34 of 35 Plugins Missing Windows Config

Only **O-AnalogEQ** has the complete cross-platform configuration:

```cmake
# O-AnalogEQ (CORRECT - the template to follow)
NEEDS_WEB_BROWSER TRUE
NEEDS_WEBVIEW2 TRUE
JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
```

All other 34 plugins have:
```cmake
# All other plugins (INCOMPLETE - missing Windows support)
NEEDS_WEB_BROWSER TRUE
# MISSING: NEEDS_WEBVIEW2 TRUE
# MISSING: JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
```

### Impact on Windows Builds
Without these flags, plugins on Windows will:
1. Build successfully (no compiler/linker errors)
2. Load in DAWs (no scan errors)
3. Show a **blank white rectangle** where the WebView should be (silent failure)
4. Or fall back to the IE backend (which doesn't support resource providers)

### CI/CD Status
The GitHub Actions workflow (`build-and-release.yml`) correctly installs the WebView2 NuGet package, but the plugins themselves must declare `NEEDS_WEBVIEW2 TRUE` and `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`.

---

## 8. Recommended WebBrowserComponent Construction Pattern

### Cross-Platform Constructor (Complete)

```cpp
// PluginEditor.h
class MyPluginEditor : public juce::AudioProcessorEditor
{
private:
    MyProcessor& processorRef;

    // Order: Relays -> WebView -> Attachments (ALWAYS use unique_ptr)
    std::unique_ptr<juce::WebSliderRelay> gainRelay;
    std::unique_ptr<juce::WebSliderRelay> mixRelay;
    std::unique_ptr<juce::WebBrowserComponent> webView;
    std::unique_ptr<juce::WebSliderParameterAttachment> gainAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> mixAttachment;

    std::optional<juce::WebBrowserComponent::Resource>
    getResource(const juce::String& url);
};
```

```cpp
// PluginEditor.cpp
MyPluginEditor::MyPluginEditor(MyProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    // 1. Create relays FIRST
    gainRelay = std::make_unique<juce::WebSliderRelay>("GAIN");
    mixRelay = std::make_unique<juce::WebSliderRelay>("MIX");

    // 2. Create WebView with ALL cross-platform options
    auto options = juce::WebBrowserComponent::Options{}
        .withNativeIntegrationEnabled()
        .withResourceProvider([this](const auto& url) { return getResource(url); })
        .withKeepPageLoadedWhenBrowserIsHidden()  // FL Studio compat
        .withOptionsFrom(*gainRelay)
        .withOptionsFrom(*mixRelay);

    // Windows-specific: set user data folder for plugin context
   #if JUCE_WINDOWS
    options = options.withWinWebView2Options(
        juce::WebBrowserComponent::Options::WinWebView2{}
            .withUserDataFolder(
                juce::File::getSpecialLocation(juce::File::tempDirectory)
                    .getChildFile("MyPlugin_WebView"))
            .withStatusBarDisabled()
            .withBuiltInErrorPageDisabled()
    );
   #endif

    webView = std::make_unique<juce::WebBrowserComponent>(options);

    // 3. Create attachments LAST (3 params required in JUCE 8)
    gainAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("GAIN"), *gainRelay, nullptr);
    mixAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("MIX"), *mixRelay, nullptr);

    addAndMakeVisible(*webView);
    webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());
    setSize(800, 500);
}
```

### Key Points:
1. **`unique_ptr` for all WebView members** - prevents initialization order issues
2. **Construction order: Relays -> WebView -> Attachments** - WebView needs relays at construction, attachments need both
3. **3 parameters for `WebSliderParameterAttachment`** - JUCE 8 requires undoManager param (can be `nullptr`)
4. **`withKeepPageLoadedWhenBrowserIsHidden()`** - prevents blank page in FL Studio
5. **Windows `withUserDataFolder()`** - prevents permission issues in plugin hosts
6. **`withStatusBarDisabled()`** on Windows - removes distracting status bar

---

## 9. HTML/JavaScript Best Practices

### ES6 Module Loading (Required)
```html
<!-- CORRECT: ES6 module imports -->
<script type="module" src="./js/juce/index.js"></script>
<script type="module">
    import { getSliderState, getToggleState } from './js/juce/index.js';

    // Use imported functions
    const gainState = getSliderState('GAIN');
    gainState.valueChangedEvent.addListener(() => {
        const value = gainState.getNormalisedValue();
        updateKnob(value);
    });
</script>
```

### Cross-Platform JavaScript Considerations
- Never reference `window.chrome.webview` directly (Windows-only)
- Never reference `window.webkit.messageHandlers` directly (macOS/Linux-only)
- Always use the JUCE API: `getSliderState()`, `getToggleState()`, `getNativeFunction()`
- Use `getBackendResourceAddress()` for loading additional resources from the resource provider
- Event callbacks receive NO parameters - always call `getNormalisedValue()` inside the callback

---

## 10. CI/CD Configuration

### GitHub Actions Windows Build Steps (Already Configured)

```yaml
# Install WebView2 NuGet Package (REQUIRED for Windows)
- name: Install WebView2 NuGet Package
  run: |
    Register-PackageSource -provider NuGet -name nugetRepository \
      -location https://www.nuget.org/api/v2 -Force
    Install-Package Microsoft.Web.WebView2 \
      -Scope CurrentUser -RequiredVersion 1.0.1901.177 \
      -Source nugetRepository -Force
```

### NuGet Package Location
JUCE searches for the WebView2 NuGet package at:
- Default: `%USERPROFILE%\AppData\Local\PackageManagement\NuGet\Packages`
- Override: Set `JUCE_WEBVIEW2_PACKAGE_LOCATION` CMake variable

---

## 11. Action Items for gui-agent

When implementing Stage 3 (GUI) for any plugin, the gui-agent MUST:

1. **Always include both CMake flags:**
   - `NEEDS_WEB_BROWSER TRUE` in `juce_add_plugin()`
   - `NEEDS_WEBVIEW2 TRUE` in `juce_add_plugin()`

2. **Always include the compile definition:**
   - `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` in `target_compile_definitions()`

3. **Use `unique_ptr` for all WebView members** (relays, webView, attachments)

4. **Include `withKeepPageLoadedWhenBrowserIsHidden()`** in Options

5. **Include Windows-specific `withUserDataFolder()`** using `#if JUCE_WINDOWS`

6. **Use ES6 module loading** (`type="module"` on script tags)

7. **Include `check_native_interop.js`** in binary data and resource provider

8. **Use 3-parameter `WebSliderParameterAttachment`** constructor (add `nullptr` for undoManager)

9. **Use explicit URL mapping** in `getResource()` (never generic loops)

10. **Never hard-code URL schemes** - use `getResourceProviderRoot()` and `getBackendResourceAddress()`

---

## References

### JUCE Source Files Analyzed
- `JUCE/modules/juce_gui_extra/native/juce_WebBrowserComponent_windows.cpp` - Windows WebView2 implementation
- `JUCE/modules/juce_gui_extra/native/juce_WebBrowserComponent_mac.mm` - macOS WKWebView implementation
- `JUCE/modules/juce_gui_extra/native/juce_WebBrowserComponent_linux.cpp` - Linux WebKit2GTK implementation
- `JUCE/modules/juce_gui_extra/misc/juce_WebBrowserComponent.h` - Cross-platform interface
- `JUCE/modules/juce_gui_extra/misc/juce_WebBrowserComponent.cpp` - Shared implementation
- `JUCE/modules/juce_gui_extra/juce_gui_extra.h` - Module configuration defines
- `JUCE/modules/juce_gui_extra/native/javascript/index.js` - Frontend API
- `JUCE/modules/juce_gui_extra/native/javascript/check_native_interop.js` - Bootstrap script
- `JUCE/extras/Build/CMake/JUCEUtils.cmake` - CMake integration
- `JUCE/extras/Build/CMake/FindWebView2.cmake` - WebView2 NuGet finder

### External Sources
- [JUCE 8 Feature Overview: WebView UIs](https://juce.com/blog/juce-8-feature-overview-webview-uis/)
- [JUCE CMake API Documentation](https://github.com/juce-framework/JUCE/blob/master/docs/CMake%20API.md)
- [JUCE Forum: WebView2 Setup](https://forum.juce.com/t/webview2-setup/44294)
- [JUCE Forum: JUCE8 Windows WebView2](https://forum.juce.com/t/juce8-windows-webview2/61576)
- [JUCE Forum: CMake and WebView2 Support](https://forum.juce.com/t/juce-with-cmake-and-webview2-support/51642)
- [JUCE Forum: WebView Freezes DAW](https://forum.juce.com/t/br-webview-freezes-daw/64917) - Multiple instances infinite loop (fixed Jan 2025)
- [JUCE Forum: WebView crashes PluginVal](https://forum.juce.com/t/webview-crashes-pluginval/64703) - Use-after-free on rapid open/close (fixed Jan 2025)
- [JUCE Forum: Force WebView2 Backend](https://forum.juce.com/t/how-to-force-webview2-backend/65547) - Options builder gotcha
- [JUCE Forum: WebView2 not Edge but IE](https://forum.juce.com/t/webview2-does-not-use-edge-but-ie/50492) - Silent IE fallback

### Local Knowledge Base
- `troubleshooting/patterns/juce8-critical-patterns.md` - 22 critical patterns
- `troubleshooting/patterns/stage-3-patterns.md` - GUI stage patterns
- `troubleshooting/runtime-issues/webbrowser-local-html-url-cant-be-shown-JUCE-20251107.md`
- `troubleshooting/gui-issues/webview-frame-load-interrupted-TapeAge-20251111.md`
- MEMORY.md - WebView2 static vs dynamic linking critical note
