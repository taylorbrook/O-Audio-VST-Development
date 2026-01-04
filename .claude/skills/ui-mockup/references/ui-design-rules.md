# WebView UI Design Rules

**Purpose:** Non-negotiable constraints for WebView-based plugin UIs that prevent runtime failures and ensure professional behavior.

**When to enforce:** Every mockup generation, validation before finalization, gui-agent Stage 3 implementation.

---

## Table of Contents

- [CSS Constraints (Critical)](#css-constraints-critical)
- [Native Application Feel (Required)](#native-application-feel-required)
- [Sizing Strategies](#sizing-strategies)
- [Resource Provider Requirements](#resource-provider-requirements)
- [Parameter Binding Patterns](#parameter-binding-patterns)
- [Interactive Control Patterns](#interactive-control-patterns)
- [Performance Guidelines](#performance-guidelines)
- [Platform-Specific Considerations](#platform-specific-considerations)

---

## CSS Constraints (Critical)

### Rule 1: Never Use Viewport Units

**❌ FORBIDDEN:**

```css
/* These cause blank display on first load in JUCE WebView */
body {
    height: 100vh;
}

.container {
    min-height: 100vh;
}

.fullscreen {
    width: 100vw;
    height: 100dvh;  /* Dynamic viewport height */
}
```

**✅ CORRECT:**

```css
/* Use percentage units with explicit html/body height */
html, body {
    height: 100%;  /* Required foundation */
    margin: 0;
    padding: 0;
}

body {
    height: 100%;  /* Now 100% works */
}

.container {
    height: 100%;  /* Inherits from body */
}
```

**Why this matters:**

JUCE WebView doesn't initialize viewport units (`vh`, `vw`, `dvh`, `svh`) until the first resize event. Using them causes:
- Blank screen on initial plugin load
- UI appears only after window resize
- Intermittent rendering failures in some DAWs

**Rationale:** Viewport units depend on browser viewport size, which JUCE WebView calculates asynchronously. Percentage units are resolved immediately against parent dimensions.

**Enforcement:**
- ❌ Reject any mockup containing: `100vh`, `100vw`, `100dvh`, `100svh`, `100lvh`, `100dvw`
- ✅ Require: `html, body { height: 100%; }` in every stylesheet
- ⚠️ Exception: `vmin`/`vmax` for responsive elements are acceptable if not used for layout height

---

### Rule 2: Box Model Consistency

**✅ CORRECT:**

```css
*, *::before, *::after {
    box-sizing: border-box;  /* Prevent size calculation surprises */
}

html, body {
    margin: 0;
    padding: 0;
    height: 100%;
}
```

**Why:**
- Prevents controls from overflowing container
- Makes sizing calculations predictable
- Ensures fixed-size plugins don't scroll

---

## Native Application Feel (Required)

### Rule 3: Disable Web Behaviors

**✅ REQUIRED CSS:**

```css
body {
    /* Disable text selection (not a document) */
    user-select: none;
    -webkit-user-select: none;
    -moz-user-select: none;
    -ms-user-select: none;

    /* Disable touch callouts on mobile */
    -webkit-touch-callout: none;

    /* Default cursor (not text cursor) */
    cursor: default;

    /* Disable scrolling (for fixed-size plugins) */
    overflow: hidden;

    /* Disable text highlighting */
    -webkit-tap-highlight-color: transparent;
}

/* Re-enable selection for specific elements if needed */
input[type="text"], textarea {
    user-select: text;
    cursor: text;
}
```

**✅ REQUIRED JavaScript:**

```javascript
// Disable right-click context menu
document.addEventListener("contextmenu", (e) => {
    e.preventDefault();
    return false;
});

// Disable default drag behaviors
document.addEventListener("dragstart", (e) => {
    e.preventDefault();
});
```

**Why:**
- Plugins should feel like native applications, not web pages
- Users expect audio plugin behaviors (no text selection, no right-click menu)
- Professional appearance (no browser UI artifacts)

**Exception:**
- Text inputs for preset names, search boxes: re-enable `user-select: text`

---

## Sizing Strategies

### Rule 4: Choose Fixed or Resizable Based on Complexity

**Decision criteria:**

| Parameter Count | Layout Complexity | Recommended |
|----------------|-------------------|-------------|
| 1-5 parameters | Simple | **Fixed size** |
| 6-12 parameters | Moderate | **Fixed or Resizable** (user preference) |
| 13+ parameters | Complex | **Resizable** |

**Special cases:**
- Visualizers (spectrum, waveform): **Resizable** (benefits from larger size)
- Utility plugins (tuner, meter): **Fixed** (single purpose)
- Multi-page UIs: **Resizable** (more screen real estate helpful)

---

### Fixed Size Pattern

**C++ (PluginEditor constructor):**

```cpp
// Fixed size - not resizable
setSize(600, 400);
setResizable(false, false);
```

**CSS:**

```css
body {
    width: 600px;
    height: 400px;
    overflow: hidden;  /* No scrolling */
}

.container {
    width: 100%;
    height: 100%;
}
```

**When to use:**
- Simple plugins (≤5 parameters)
- Single-purpose tools
- Skeuomorphic designs (replicating hardware)

---

### Resizable Pattern

**C++ (PluginEditor constructor):**

```cpp
// Resizable with aspect ratio lock
setSize(800, 600);
setResizable(true, true);

// Optional: Set size limits
setResizeLimits(600, 400, 1920, 1080);

// Optional: Constrain aspect ratio
getConstrainer()->setFixedAspectRatio(4.0 / 3.0);  // 4:3 ratio
```

**CSS (responsive with constraints):**

```css
body {
    min-width: 600px;
    max-width: 1920px;
    min-height: 400px;
    max-height: 1080px;
    width: 100%;
    height: 100%;
}

.control-group {
    /* Use clamp() for responsive sizing */
    font-size: clamp(12px, 1.5vw, 18px);
    gap: clamp(10px, 2vw, 30px);
}

.knob {
    /* Scale with window size */
    width: clamp(60px, 8vw, 100px);
    height: clamp(60px, 8vw, 100px);
}
```

**When to use:**
- Complex plugins (>5 parameters)
- Multi-section layouts
- Visualizers and analysis tools
- Accessibility (users want larger UI)

**Testing requirements:**
- Test at minimum size (controls don't overlap)
- Test at maximum size (no pixelation, acceptable spacing)
- Test aspect ratio lock (no distortion)

---

## Resource Provider Requirements

### Rule 5: All Files Must Be Embedded

**✅ CORRECT (Production):**

```cmake
# CMakeLists.txt - Embed all UI files
juce_add_binary_data(${PLUGIN_NAME}_UIResources
    SOURCES
        Source/ui/public/index.html
        Source/ui/public/js/juce/index.js
        Source/ui/public/css/styles.css
        Source/ui/public/assets/logo.svg
)

target_link_libraries(${PLUGIN_NAME} PRIVATE ${PLUGIN_NAME}_UIResources)
```

**❌ WRONG (External files):**

```html
<!-- Don't load external resources in production -->
<link rel="stylesheet" href="https://cdn.example.com/styles.css">
<script src="https://unpkg.com/library@1.0.0"></script>
```

**Why:**
- JUCE 8 requires resource provider pattern (replaces JUCE 7 data URLs)
- Plugins must work offline (no internet dependency)
- Faster load times (no network requests)
- Version stability (no CDN changes breaking plugin)

**Development exception:**
- Local dev server acceptable for hot reloading during development
- Must switch to embedded files for production builds

---

### Rule 6: Resource Naming Convention

**File path → BinaryData symbol:**

```
Source/ui/public/index.html       → BinaryData::index_html
Source/ui/public/js/juce/index.js → BinaryData::juce_index_js
Source/ui/public/css/styles.css   → BinaryData::styles_css
Source/ui/public/assets/logo.svg  → BinaryData::logo_svg
```

**Pattern:**
1. Remove `Source/ui/public/` prefix
2. Replace `/` with `_`
3. Replace `.` with `_`
4. Convert to valid C++ identifier

**Example resource provider:**

```cpp
std::optional<juce::WebBrowserComponent::Resource> getResource(
    const juce::String& url
) {
    // Handle root
    if (url == "/" || url == "/index.html") {
        return juce::WebBrowserComponent::Resource {
            BinaryData::index_html,
            BinaryData::index_htmlSize,
            "text/html"
        };
    }

    // Handle JavaScript
    if (url == "/js/juce/index.js") {
        return juce::WebBrowserComponent::Resource {
            BinaryData::juce_index_js,
            BinaryData::juce_index_jsSize,
            "text/javascript"
        };
    }

    return std::nullopt;  // 404
}
```

---

## Parameter Binding Patterns

### Rule 7: Use JUCE Relay Pattern

**Architecture:**

```
C++ Parameter (APVTS)
    ↕ (normalized 0-1)
ParameterAttachment
    ↕
WebSliderRelay / WebToggleButtonRelay / WebComboBoxRelay
    ↕ (JavaScript events)
WebBrowserComponent
    ↕ (JavaScript state object)
HTML Control (<input>, <select>, <button>)
```

**Parameter type mapping:**

| Parameter Type | C++ Relay | C++ Attachment | JavaScript State |
|---------------|-----------|----------------|------------------|
| Float (slider/knob) | `WebSliderRelay` | `WebSliderParameterAttachment` | `Juce.getSliderState()` |
| Bool (toggle) | `WebToggleButtonRelay` | `WebToggleButtonParameterAttachment` | `Juce.getToggleButtonState()` |
| Choice (dropdown) | `WebComboBoxRelay` | `WebComboBoxParameterAttachment` | `Juce.getComboBoxState()` |

**Example JavaScript binding:**

```javascript
import * as Juce from "./js/juce/index.js";

// Get state object from C++
const gainState = Juce.getSliderState("GAIN");  // "GAIN" = parameter ID

// Get HTML element
const gainSlider = document.getElementById("gain-slider");

// Bind HTML → C++ (user interaction)
gainSlider.addEventListener("input", (e) => {
    const value = parseFloat(e.target.value);
    gainState.setNormalisedValue(value);  // Normalized 0-1
});

// Bind C++ → HTML (automation, preset recall)
gainState.valueChangedEvent.addListener((newValue) => {
    gainSlider.value = newValue;
    updateGainDisplay(newValue);
});

// Initialize from current parameter value
gainSlider.value = gainState.getNormalisedValue();
```

**Critical requirements:**
- Parameter IDs in JavaScript must match C++ exactly (case-sensitive)
- Always initialize HTML controls from `getNormalisedValue()` (handles preset recall)
- Always listen to `valueChangedEvent` (handles DAW automation)

---

## Interactive Control Patterns

### Rule 8: Rotary Control Rotation

**Principle:** Rotate the parent knob element, not child indicator.

**❌ WRONG (Clock hand effect):**

```css
.knob-indicator {
    transform-origin: center bottom;
    transition: transform 50ms;
}
```

```javascript
// Only indicator rotates, knob stays fixed
indicator.style.transform = `rotate(${degrees}deg)`;
```

**✅ CORRECT (Entire control rotates):**

```css
.knob {
    transform-origin: center center;
    transition: transform 50ms;
}

.knob-indicator {
    /* No transform - inherits parent rotation */
    position: absolute;
    top: 8px;
    left: 50%;
    transform: translateX(-50%);  /* Only for centering */
}
```

```javascript
// Rotate parent - indicator follows automatically
knob.style.transform = `rotate(${degrees}deg)`;
```

**Rationale:** The entire knob (body + indicator) should rotate as a single unit. Rotating only the indicator creates a "clock hand" effect where the line spins around a pivot point while the knob body stays fixed.

**Standard rotation range:** -135° to +135° (270° total range)

```javascript
// Standard knob rotation calculation
const normalized = (value - min) / (max - min);  // 0 to 1
const degrees = -135 + (normalized * 270);       // -135° to +135°
knob.style.transform = `rotate(${degrees}deg)`;
```

---

### Rule 9: Skeuomorphic Lighting Consistency

**Principle:** Light and shadow remain fixed; only texture and indicator rotate.

**Implementation:**

```html
<div class="knob">
    <!-- Layer 1: Fixed lighting (doesn't rotate) -->
    <div class="knob-lighting"></div>

    <!-- Layer 2: Rotating texture + indicator (rotates with knob) -->
    <div class="knob-body">
        <div class="knob-texture"></div>
        <div class="knob-indicator"></div>
    </div>
</div>
```

```css
.knob {
    position: relative;
}

.knob-lighting {
    /* Fixed lighting layer - doesn't rotate */
    position: absolute;
    inset: 0;
    border-radius: 50%;
    box-shadow:
        inset -2px -2px 4px rgba(0,0,0,0.4),  /* Bottom-right shadow */
        inset 2px 2px 4px rgba(255,255,255,0.2);  /* Top-left highlight */
    pointer-events: none;
    z-index: 2;
}

.knob-body {
    /* Rotating layer - texture + indicator */
    transform-origin: center center;
    transition: transform 50ms;
    position: relative;
    z-index: 1;
}
```

```javascript
// Rotate only .knob-body (lighting stays fixed)
knobBody.style.transform = `rotate(${degrees}deg)`;
```

**Rationale:** In physical hardware, light source position is fixed. When you turn a knob, the ridges/texture rotate under fixed lighting, creating realistic shadow movement. Rotating the lighting itself breaks physical realism.

**Examples:**
- ✅ Knob texture/ridges rotate, highlights/shadows stay fixed
- ✅ Indicator line rotates with knob body
- ❌ Shadow rotates with knob (unrealistic - light source doesn't follow knob)

---

### Rule 10: Test HTML Preview Frame

**Principle:** Test HTML must show plugin boundaries with fixed-size preview frame.

**❌ WRONG (Fills entire browser window):**

```css
html, body {
    height: 100%;
}

.container {
    width: 100%;   /* Fills browser window */
    height: 100%;  /* No visible boundaries */
}
```

**✅ CORRECT (Fixed preview frame):**

```css
html, body {
    height: 100%;
    margin: 0;
    background: #000;  /* Black beyond plugin */
}

body {
    display: flex;
    align-items: center;
    justify-content: center;
    min-height: 100vh;
}

/* Fixed-size frame matching plugin spec */
.plugin-frame {
    width: 600px;   /* FIXED - from creative brief */
    height: 300px;  /* FIXED - from creative brief */
    border: 2px solid #444;  /* Visible edge */
    box-shadow: 0 10px 40px rgba(0,0,0,0.5);
    overflow: hidden;
    position: relative;
}

/* Plugin UI fills frame */
.container {
    width: 100%;   /* Now 100% of 600px frame */
    height: 100%;  /* Now 100% of 300px frame */
}
```

**Rationale:** Browser testing must show actual plugin dimensions. Without a fixed frame, designers can't verify if content fits the target size - UI appears to "work" by filling the browser window, then overflows in actual plugin.

**Requirements:**
- Frame dimensions match `window.width` × `window.height` from YAML spec
- Visible border (2px solid, distinguishable color)
- Black background outside frame (shows bounds clearly)
- Centered in viewport (flexbox)
- Apply this to **test HTML only** - production HTML still uses `width: 100%; height: 100%;`

---

### Rule 11: Debug Parameter Monitor (Test HTML Only)

**Principle:** Test HTML must include a live parameter monitor showing current adjustments.

**Implementation:**

```html
<!-- Bottom-right debug monitor -->
<div class="debug-monitor">
    <div class="debug-label">PARAMETER MONITOR</div>
    <div class="debug-param" id="debugParam">—</div>
    <div class="debug-value" id="debugValue">—</div>
    <div class="debug-normalized" id="debugNormalized">—</div>
</div>
```

```css
.debug-monitor {
    position: fixed;
    bottom: 10px;
    right: 10px;
    background: rgba(0, 0, 0, 0.85);
    color: #0f0;
    font-family: 'Courier New', monospace;
    font-size: 11px;
    padding: 8px 12px;
    border: 1px solid #333;
    border-radius: 4px;
    min-width: 150px;
    pointer-events: none;  /* Don't block clicks */
    z-index: 10000;
}

.debug-label {
    color: #666;
    font-size: 9px;
    margin-bottom: 4px;
    letter-spacing: 1px;
}

.debug-param {
    font-weight: bold;
    color: #0f0;
}

.debug-value {
    color: #fff;
    margin-top: 2px;
}

.debug-normalized {
    color: #888;
    font-size: 10px;
}
```

```javascript
// Update debug monitor on parameter change
function updateDebugMonitor(paramId, value, normalizedValue) {
    document.getElementById('debugParam').textContent = paramId;
    document.getElementById('debugValue').textContent = formatValue(paramId, value);
    document.getElementById('debugNormalized').textContent = `${(normalizedValue * 100).toFixed(1)}%`;
}

// Call in parameter update handlers
function updateKnob(paramId, value, min, max) {
    // ... normal parameter update logic ...

    const normalized = (value - min) / (max - min);
    updateDebugMonitor(paramId, value, normalized);
}
```

**Requirements:**
- Position: `bottom: 10px; right: 10px` (fixed)
- Shows: Parameter ID, formatted value, normalized percentage
- Updates in real-time during drag/scroll/interaction
- Non-interactive (`pointer-events: none`)
- High z-index (appears above all UI elements)
- Monospace terminal aesthetic (green on black)
- **Test HTML only** - never in production UI

**Rationale:** Provides instant visual feedback during browser testing that parameter bindings are working correctly, values are in correct ranges, and formatting matches expectations. Essential for debugging parameter behavior without opening dev console.

---

## Performance Guidelines

### Rule 12: Optimize for Real-Time Audio Context

**✅ GOOD:**

```javascript
// Throttle expensive operations
let lastUpdate = 0;
const UPDATE_INTERVAL = 16;  // ~60 FPS

window.__JUCE__.backend.addEventListener("meterUpdate", (data) => {
    const now = performance.now();
    if (now - lastUpdate < UPDATE_INTERVAL) return;
    lastUpdate = now;

    updateMeter(data.level);
});
```

**❌ BAD:**

```javascript
// Unthrottled updates cause UI stutter
window.__JUCE__.backend.addEventListener("meterUpdate", (data) => {
    // Expensive DOM manipulation on every event (100+ times/sec)
    document.querySelector('.meter').style.height = (data.level * 100) + '%';
});
```

**Performance rules:**
- **Throttle updates:** Max 60 FPS (16ms interval) for animations
- **Batch DOM updates:** Use `requestAnimationFrame()` to batch changes
- **Minimize reflows:** Cache element references, avoid layout thrashing
- **Use CSS transforms:** `transform: translateX()` instead of `left` (GPU-accelerated)

---

### Rule 13: Timer-Based Updates

**C++ pattern (PluginEditor):**

```cpp
class PluginEditor : public juce::AudioProcessorEditor,
                     private juce::Timer  // Inherit Timer
{
private:
    void timerCallback() override {
        // Read atomics from audio thread
        float level = audioProcessor.getCurrentLevel();

        // Emit event to JavaScript
        webView->emitEventIfBrowserIsVisible("meterUpdate", {
            { "level", level },
            { "timestamp", juce::Time::currentTimeMillis() }
        });
    }
};

// In constructor:
startTimerHz(16);  // 60ms = ~16 FPS (good for visualizations)
```

**Timer interval guidelines:**

| Update Type | Interval | FPS | Use Case |
|------------|----------|-----|----------|
| Smooth animations | 16ms | 60 FPS | Spectrum analyzer, waveform |
| Meter updates | 60ms | 16 FPS | VU meters, level displays |
| Slow polling | 100ms | 10 FPS | Tempo sync, transport state |
| Rare updates | 1000ms | 1 FPS | License check, version info |

---

## Platform-Specific Considerations

### Rule 14: Handle Platform Differences

**Windows-specific (omit on macOS):**

```cmake
# CMakeLists.txt
if (WIN32)
    target_compile_definitions(${PLUGIN_NAME} PRIVATE
        JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
    )
endif()
```

```cpp
// PluginEditor.cpp
webView = std::make_unique<juce::WebBrowserComponent>(
    juce::WebBrowserComponent::Options{}
        #ifdef _WIN32
        .withBackend(juce::WebBrowserComponent::Options::Backend::webview2)
        .withWinWebView2Options(
            juce::WebBrowserComponent::Options::WinWebView2{}
                .withBackgroundColour(juce::Colours::white)
                // CRITICAL: Prevents permission issues in some DAWs
                .withUserDataFolder(juce::File::getSpecialLocation(
                    juce::File::SpecialLocationType::tempDirectory))
        )
        #endif
        .withNativeIntegrationEnabled()
        .withResourceProvider([this](const auto& url) { return getResource(url); })
);
```

**macOS-specific:**

```cpp
// macOS uses WebKit (built-in, no additional setup)
webView = std::make_unique<juce::WebBrowserComponent>(
    juce::WebBrowserComponent::Options{}
        .withNativeIntegrationEnabled()
        .withResourceProvider([this](const auto& url) { return getResource(url); })
        // FL Studio fix: prevent blank screen on focus loss
        .withKeepPageLoadedWhenBrowserIsHidden()
);
```

**Linux-specific:**

```cmake
# CMakeLists.txt - Linux requires WebKit2GTK
if (UNIX AND NOT APPLE)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(WEBKIT2 REQUIRED webkit2gtk-4.0)
    target_link_libraries(${PLUGIN_NAME} PRIVATE ${WEBKIT2_LIBRARIES})
    target_include_directories(${PLUGIN_NAME} PRIVATE ${WEBKIT2_INCLUDE_DIRS})
endif()
```

**Platform testing requirements:**
- Test on all target platforms (Windows, macOS, Linux)
- Windows: Verify WebView2 runtime detection
- macOS: Test in Logic Pro (right-click crash bug), Ableton (Escape key crash)
- Linux: Verify webkit2gtk dependency message if missing

---

## Summary: Validation Checklist

Before finalizing any mockup, validate against these rules:

### Critical (Will Cause Failures)

- [ ] **No viewport units:** CSS does not contain `100vh`, `100vw`, `100dvh`, `100svh`
- [ ] **HTML/body height:** CSS includes `html, body { height: 100%; }`
- [ ] **Native integration:** All resources embedded via `juce_add_binary_data`
- [ ] **Parameter IDs match:** JavaScript parameter IDs match parameter-spec.md exactly

### Required (Professional Behavior)

- [ ] **Native feel CSS:** `user-select: none`, `cursor: default`, `overflow: hidden`
- [ ] **Native feel JS:** Context menu disabled via `contextmenu` event
- [ ] **Sizing strategy:** Fixed or resizable chosen based on complexity
- [ ] **Performance:** Updates throttled to ≤60 FPS

### Recommended (Best Practices)

- [ ] **Box model:** `box-sizing: border-box` for all elements
- [ ] **Error handling:** JavaScript error handlers for `error` and `unhandledrejection`
- [ ] **Platform support:** Platform-specific options included if cross-platform
- [ ] **Testing:** Tested in Debug and Release builds, tested reload 10+ times

### Interactive Controls (Test HTML)

- [ ] **Rotary rotation:** Parent knob rotates (not child indicator)
- [ ] **Skeuomorphic lighting:** Light/shadow fixed, only texture rotates
- [ ] **Preview frame:** Fixed-size frame with visible border (test HTML only)
- [ ] **Debug monitor:** Parameter monitor bottom-right (test HTML only)

---

## Related Documentation

- **Architecture:** `architecture/12-webview-integration-design.md` - Complete WebView architecture
- **Best practices:** `procedures/webview/best-practices.md` - Critical safety patterns
- **Communication:** `procedures/webview/03-communication-patterns.md` - C++ ↔ JavaScript patterns
- **Parameter binding:** `procedures/webview/04-parameter-binding.md` - Relay pattern details
- **Common problems:** `procedures/webview/common-problems.md` - Troubleshooting guide
