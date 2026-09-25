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

### Rule 4: Fixed Frame by Default, No Taller Than 800 px

**Suite evidence** (UI design review 260924-nho, §5.6):
- 40 of 44 plugins ship a fixed frame. Only four are resizable: O-MicrotonalSampler (720×480 – 1600×1080, the only one with `@media` breakpoints), O-Orbit, O-simpleBeatmaker and O-simpleSubtractive.
- Complexity has not driven sizing in practice: O-Prism (1200×800, five tabs) and O-Octagon (a visualizer) are both fixed, and work.
- Five shipped frames are 820–980 px tall (O-simpleFM 980, O-simpleAdditive 930, O-simpleBeatmaker 900, O-simplePhysicalModelSynth 860, O-simpleSubtractive 820). A 13-inch laptop at its default scaled resolution leaves about 800 px below the menu bar and a DAW's plugin-window title bar, so those windows fall off-screen.

**The rule:**
- **Parameter count and page count do not decide sizing.**
- **Default: a fixed frame no taller than 800 px.** Width follows the design. Use tabs, pages or a denser layout before growing the frame past 800 px.
- **Only if a design genuinely cannot fit in 800 px**, make it resizable with a fixed aspect ratio and whole-page scaling (review R7) — see "Resizable Pattern" below:
  - `setResizable(true, true)`, plus `getConstrainer()->setFixedAspectRatio(designW / designH)`, plus `setResizeLimits`;
  - the page keeps its fixed design-size composition and scales it as a whole — CSS `zoom`, or a `transform: scale()` on the root, computed from `window.innerWidth / designWidth` in a `resize` listener;
  - it does NOT reflow, and has no breakpoints.
  - No shipped plugin uses this scale approach yet, so the Stage 3 integration checklist carries a hands-on DAW resize check for any resizable frame.

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
html, body {
    height: 100%;
    overflow: hidden;  /* No scrolling */
}

body {
    width: 600px;
    height: 400px;
}

.container {
    width: 100%;
    height: 100%;
}
```

**When to use:**
- Every plugin whose design fits in a frame ≤ 800 px tall — the default
- Multi-page and tabbed UIs (O-Prism: 1200×800, five tabs)
- Visualizers (O-Octagon)
- Skeuomorphic designs (replicating hardware)

---

### Resizable Pattern

**C++ (PluginEditor constructor):**

```cpp
// The design size is the composition the page is authored at.
constexpr int designW = 1000, designH = 760;

setSize(designW, designH);
setResizable(true, true);
getConstrainer()->setFixedAspectRatio(static_cast<double>(designW) / designH);
setResizeLimits(designW * 3 / 4, designH * 3 / 4, designW * 3 / 2, designH * 3 / 2);
```

**CSS + JS (scale the fixed composition; never reflow):**

```css
html, body {
    width: 100%;
    height: 100%;
    overflow: hidden;
}

#stage {
    width: 1000px;            /* designW — the composition never changes size */
    height: 760px;            /* designH */
    transform-origin: 0 0;
}
```

```javascript
// In the module-state block:  const DESIGN_W = 1000;
function applyScale() {
    const s = window.innerWidth / DESIGN_W;       // aspect is locked by the constrainer
    document.getElementById('stage').style.transform = `scale(${s})`;
}
// In init():
applyScale();
window.addEventListener('resize', applyScale);
```

Keep `#tooltip` OUTSIDE `#stage`: a transformed ancestor becomes the containing block for `position: fixed` children, which would break the tooltip's frame clamp. Drag travel is measured in window pixels, so knobs feel the same at every scale.

**When to use:**
- Only when the design cannot fit in a fixed frame ≤ 800 px tall (Rule 4)

**Testing requirements:**
- Test at minimum size (text stays at or above the 9 px floor once scaled)
- Test at maximum size (no pixelation, acceptable spacing)
- Test aspect ratio lock (no distortion)
- Hands-on in a DAW: drag-resize the plugin window and confirm the page scales as a whole

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
- Always listen to `propertiesChangedEvent` too — range, skew and choices can arrive after the first paint
- Readouts come from `getScaledValue()`, never from a JS range map; the C++ `NormalisableRange` is the only range
- The full knob contract (markup, CSS, `bindKnob` lifecycle, readouts) lives in `html-generation.md`

---

## Interactive Control Patterns

### Rule 8: Rotary Control Rotation — Family A stem

**Principle:** The seed ring and its lighting stay fixed. Only the `.knob-stem` rotates. This is the house knob (Family A, O-ReverseDelay) and is consistent with Rule 9: the light source does not follow the knob.

```html
<div class="knob" id="knob-threshold" data-param="threshold"><div class="knob-stem"></div></div>
```

```css
.knob-stem {
  position: absolute;
  left: 50%;
  top: 50%;
  width: 2.5px;
  height: 24px;
  border-radius: 1.5px;
  background: var(--brown-frame);
  transform-origin: 50% 100%;                        /* pivot at the stem's base */
  transform: translate(-50%, -100%) rotate(0deg);    /* base sits on the knob centre */
  pointer-events: none;
}
```

The `translate(-50%, -100%)` puts the stem's base on the knob centre, and `transform-origin: 50% 100%` makes that base the pivot, so the stem reads as a pointer from the centre outwards.

**Angle:** −135° to +135° (270° travel), from the parameter's own normalised value:

```javascript
function normToDeg(n) {
    return KNOB_MIN_DEG + n * (KNOB_MAX_DEG - KNOB_MIN_DEG);   // -135 .. +135
}

stem.style.transform =
    `translate(-50%, -100%) rotate(${normToDeg(st.getNormalisedValue())}deg)`;
```

`getNormalisedValue()` already carries the C++ range and skew. Never compute the angle from mirrored engineering ranges in JS: it matches C++ only on linear parameters.

The full binding — `updateKnobVisual`, `bindKnob`, pointer capture, arrow keys, ARIA, wheel and dblclick reset — is in `html-generation.md` ("Rotary Knob — Family A" and "Knob Interaction").

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
// Update debug monitor on parameter change. Reads what the knob already shows:
// its own aria-valuetext (= the readout text, from getScaledValue()) and the
// normalised value straight from the SliderState. No range math here.
function updateDebugMonitor(id) {
    const st = sliderState[id];
    const knob = document.getElementById(`knob-${id}`);
    if (!st || !knob) return;
    document.getElementById('debugParam').textContent = id;
    document.getElementById('debugValue').textContent = knob.getAttribute('aria-valuetext') || '—';
    document.getElementById('debugNormalized').textContent =
        `${(st.getNormalisedValue() * 100).toFixed(1)}%`;
}

// Call at the end of updateKnobVisual(id), after aria-valuetext is written.
```

**Requirements:**
- Position: `bottom: 10px; right: 10px` (fixed)
- Shows: Parameter ID, the knob's readout text (its `aria-valuetext`), normalised percentage from `getNormalisedValue()`
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
| Rare updates | 1000ms | 1 FPS | Version info, status readout |

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
- [ ] **Sizing strategy:** fixed frame ≤ 800 px tall, or resizable with a fixed-aspect whole-page scale per Rule 4
- [ ] **Performance:** Updates throttled to ≤60 FPS
- [ ] **i18n canon:** the canonical block copied verbatim from `scripts/i18n-canon.js` (never retyped); table module `js/i18n.js` with en / fr / zh-Hans (`html-generation.md` "i18n — the canonical block")
- [ ] **Hover-help switch:** exactly one `#tips-toggle`, in the settings popover beside `#lang-select`, bound in `TIP_BINDINGS`
- [ ] **Tooltip copy:** `data-tip` / `data-tip-title` written only by `applyI18n()` from `TIP_BINDINGS` — none authored in markup, no native `title`
- [ ] **9 px text floor:** no text element below 9 px (`aesthetic.md`: parameter labels 9–11 px)
- [ ] **Reduced motion:** every transition or animation has a counterpart inside `@media (prefers-reduced-motion: reduce)`

### Recommended (Best Practices)

- [ ] **Box model:** `box-sizing: border-box` for all elements
- [ ] **Error handling:** JavaScript error handlers for `error` and `unhandledrejection`
- [ ] **Platform support:** Platform-specific options included if cross-platform
- [ ] **Testing:** Tested in Debug and Release builds, tested reload 10+ times

### Interactive Controls

These apply to test HTML and production HTML alike.

- [ ] **Rotary rotation:** Family A — ring and lighting fixed, only `.knob-stem` rotates, angle from `getNormalisedValue()` (Rule 8)
- [ ] **Pointer lifecycle:** `setPointerCapture` on the knob, with `pointerup`, `pointercancel` and `lostpointercapture` all ending the gesture exactly once
- [ ] **Keyboard + ARIA:** arrow keys nudge, `role="slider"`, `tabindex="0"`, `aria-valuetext` = readout text
- [ ] **Readouts:** from `getScaledValue()` — no JS range map, no mirrored min/max
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
