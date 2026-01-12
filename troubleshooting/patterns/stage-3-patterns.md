# Stage 3 Critical Patterns - GUI & WebView Integration

**Purpose:** Prevent repeat mistakes during Stage 3 (GUI) implementation.

**When to read:** Before implementing WebView UI and parameter bindings.

**Patterns included:** 14 of 22 total patterns (WebView, UI bindings, JavaScript integration)

---

## 1. WebView UI - Module Requirements

### Required CMake Configuration
```cmake
target_link_libraries(MyPlugin
    PRIVATE
        juce::juce_gui_extra  # REQUIRED for WebBrowserComponent
)

target_compile_definitions(MyPlugin
    PUBLIC
        JUCE_WEB_BROWSER=1     # Enable WebView
        JUCE_USE_CURL=0        # Disable CURL (not needed for local HTML)
)
```

### Required Includes
```cpp
#include <juce_gui_extra/juce_gui_extra.h>  // For WebBrowserComponent
```

---

## 2. Threading - UI ↔ Audio Thread

### ❌ WRONG (Thread violation - will crash)
```cpp
// In PluginEditor (UI thread)
button.onClick = [this] {
    audioProcessor.processBlock(...);  // ILLEGAL
};
```

### ✅ CORRECT
```cpp
// Use APVTS for safe UI ↔ Audio communication
button.onClick = [this] {
    audioProcessor.getAPVTS().getParameter("trigger")->setValueNotifyingHost(1.0f);
};
```

**Rule:** NEVER call audio processing code from UI thread. Use AudioProcessorValueTreeState (APVTS) for communication.

---

## 3. Rotary Sliders - Bounds vs setBounds

### ❌ WRONG (Non-interactive sliders)
```cpp
slider.setBounds(x, y, w, h);  // Absolute positioning breaks mouse tracking
```

### ✅ CORRECT
```cpp
addAndMakeVisible(slider);
// In resized():
slider.setBounds(x, y, w, h);  // OK in resized()

// OR use FlexBox/Grid layout
```

**Rule:** Sliders need proper parent component hierarchy for mouse events.

---

## 4. WebView ↔ Parameter Binding

### ❌ WRONG (Parameters not updating)
```cpp
// JavaScript sends:
{ type: "parameterChanged", id: "gain", value: 0.5 }

// C++ expects:
{ type: "parameter_change", ... }
```

### ✅ CORRECT
```cpp
// Standardized event format:
// JS → C++: { type: "parameter_change", id: "gain", value: 0.5 }
// C++ → JS: { type: "parameter_update", id: "gain", value: 0.5 }
```

**Rule:** WebView integration requires exact event type matching between JS and C++.

---

## 5. WebView Resource Provider - Explicit URL Mapping (ALWAYS REQUIRED)

### ❌ WRONG (Generic loop - breaks resource loading)
```cpp
std::optional<juce::WebBrowserComponent::Resource>
getResource(const juce::String& url)
{
    // Generic loop - hard to debug, easy to break
    for (int i = 0; i < BinaryData::namedResourceListSize; ++i) {
        if (path == BinaryData::namedResourceList[i]) {
            // ... conversion logic ...
        }
    }
    return std::nullopt;
}
```

### ✅ CORRECT (Explicit URL mapping)
```cpp
std::optional<juce::WebBrowserComponent::Resource>
getResource(const juce::String& url)
{
    auto makeVector = [](const char* data, int size) {
        return std::vector<std::byte>(
            reinterpret_cast<const std::byte*>(data),
            reinterpret_cast<const std::byte*>(data) + size
        );
    };

    // Explicit mapping - clear, debuggable, reliable
    if (url == "/" || url == "/index.html") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::index_html, BinaryData::index_htmlSize),
            juce::String("text/html")
        };
    }

    if (url == "/js/juce/index.js") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::index_js, BinaryData::index_jsSize),
            juce::String("text/javascript")
        };
    }

    return std::nullopt;  // 404
}
```

**Why:** BinaryData converts paths to C++ identifiers (`index.js` → `index_js`), but HTML/JS use original paths.

---

## 6. Testing GUI Changes - Always Install to System (CRITICAL WORKFLOW)

### ❌ WRONG (Tests stale cached builds)
```bash
./scripts/build-and-install.sh MyPlugin --no-install
# Test in DAW → loads OLD version from system folders
```

### ✅ CORRECT
```bash
# Build AND install to system folders
./scripts/build-and-install.sh MyPlugin
# Then restart DAW - Required for plugin rescan
```

**Why:** DAWs load plugins from `~/Library/Audio/Plug-Ins/`, NOT build directories.

---

## 7. WebView Member Initialization - Use std::unique_ptr (REQUIRED)

### ❌ WRONG (Raw members - initialization order issues)
```cpp
class MyPluginEditor : public juce::AudioProcessorEditor {
private:
    juce::WebSliderRelay gainRelay;
    juce::WebBrowserComponent webView;
    juce::WebSliderParameterAttachment gainAttachment;
};
```

### ✅ CORRECT
```cpp
class MyPluginEditor : public juce::AudioProcessorEditor {
private:
    // Order: Relays → WebView → Attachments
    std::unique_ptr<juce::WebSliderRelay> gainRelay;
    std::unique_ptr<juce::WebBrowserComponent> webView;
    std::unique_ptr<juce::WebSliderParameterAttachment> gainAttachment;
};
```

**Constructor:**
```cpp
MyPluginEditor::MyPluginEditor(MyProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    // 1. Create relays FIRST
    gainRelay = std::make_unique<juce::WebSliderRelay>("GAIN");

    // 2. Create WebView with relay options
    webView = std::make_unique<juce::WebBrowserComponent>(
        juce::WebBrowserComponent::Options{}
            .withNativeIntegrationEnabled()
            .withResourceProvider([this](auto& url) { return getResource(url); })
            .withOptionsFrom(*gainRelay)
    );

    // 3. Create attachments LAST
    gainAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("GAIN"), *gainRelay, nullptr
    );

    addAndMakeVisible(*webView);
    webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());
}
```

---

## 8. WebSliderParameterAttachment - Three Parameters Required (JUCE 8)

### ❌ WRONG (Knobs frozen - no parameter updates)
```cpp
// JUCE 7 style (2 parameters) - FAILS silently in JUCE 8
driveAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
    *processorRef.parameters.getParameter("drive"), *driveRelay);
```

### ✅ CORRECT
```cpp
// JUCE 8 requires 3 parameters (added undoManager)
driveAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
    *processorRef.parameters.getParameter("drive"), *driveRelay, nullptr);
```

**Why:** JUCE 8 changed constructor signature. Missing third parameter causes **silent failure**.

---

## 9. check_native_interop.js - Required for WebView (CRITICAL)

### ❌ WRONG (Missing file - UI may freeze)
```cmake
juce_add_binary_data(PluginName_UIResources
    SOURCES
        Source/ui/public/index.html
        Source/ui/public/js/juce/index.js
        # Missing: check_native_interop.js
)
```

### ✅ CORRECT
```cmake
juce_add_binary_data(PluginName_UIResources
    SOURCES
        Source/ui/public/index.html
        Source/ui/public/js/juce/index.js
        Source/ui/public/js/juce/check_native_interop.js  # Required
)
```

**Why:** `check_native_interop.js` verifies native C++ ↔ JavaScript bridge is working.

---

## 10. WebView valueChangedEvent Callback - No Parameters Passed (CRITICAL)

### ❌ WRONG (Parameters undefined, knobs don't update)
```javascript
driveState.valueChangedEvent.addListener((newValue) => {
    // newValue is UNDEFINED! JUCE doesn't pass callback parameters
    updateKnobVisual(driveRotatable, newValue);  // angle = NaN
});
```

### ✅ CORRECT
```javascript
driveState.valueChangedEvent.addListener(() => {
    const value = driveState.getNormalisedValue();  // Read from state
    updateKnobVisual(driveRotatable, value);
});
```

**Why:** JUCE's `valueChangedEvent` is a notification event, not value-passing.

---

## 11. WebView Knob Interaction - Relative Drag (ALWAYS REQUIRED)

### ❌ WRONG (Absolute positioning - knob jumps to cursor)
```javascript
document.addEventListener('mousemove', (e) => {
    const deltaY = startY - e.clientY;  // Total from START
    const newRotation = startRotation + deltaY;  // Absolute
    setRotation(newRotation);
});
```

### ✅ CORRECT (Relative drag - industry standard)
```javascript
let lastY = 0;

knob.addEventListener('mousedown', (e) => {
    isDragging = true;
    lastY = e.clientY;  // Store CURRENT position
});

document.addEventListener('mousemove', (e) => {
    if (!isDragging) return;

    const deltaY = lastY - e.clientY;  // Distance since LAST FRAME
    rotation += deltaY * 0.5;  // Increment
    rotation = Math.max(-135, Math.min(135, rotation));

    setRotation(rotation);
    lastY = e.clientY;  // Update for next frame
});
```

**Key:** Use `lastY` (previous frame), not `startY` (initial click).

---

## 12. WebView Boolean Parameters - Use getToggleState (ALWAYS REQUIRED)

### ❌ WRONG (Works but semantically incorrect)
```javascript
const modModeState = Juce.getSliderState("MOD_MODE");
modModeToggle.addEventListener("click", () => {
    const currentValue = modModeState.getValue();
    const newValue = currentValue < 0.5 ? 1.0 : 0.0;
    modModeState.setValue(newValue);
});
```

### ✅ CORRECT
```javascript
const modModeState = Juce.getToggleState("MOD_MODE");
modModeToggle.addEventListener("click", () => {
    modModeState.setValue(!modModeState.getValue());  // Clean boolean
});
```

---

## 13. WebView VU Meters - requestAnimationFrame Loop (ALWAYS REQUIRED)

### ❌ WRONG (Updates but jerky, no smooth motion)
```javascript
function updateVUMeter(dbLevel) {
    vuNeedle.style.transform = `rotate(${angle}deg)`;  // Instant jump
}
Juce.addEventListener('VU_LEVEL', updateVUMeter);
```

### ✅ CORRECT (Smooth ballistic motion)
```javascript
let currentAngle = -45;
let targetAngle = -45;
const ATTACK_SPEED = 0.4;
const DECAY_SPEED = 0.15;

function updateVUMeter(dbLevel) {
    targetAngle = -45 + (mapDBToNormalized(dbLevel) * 90);
}

function animateVUMeter() {
    const speed = currentAngle < targetAngle ? ATTACK_SPEED : DECAY_SPEED;
    currentAngle += (targetAngle - currentAngle) * speed;
    vuNeedle.style.transform = `rotate(${currentAngle}deg)`;
    requestAnimationFrame(animateVUMeter);
}

Juce.addEventListener('VU_LEVEL', updateVUMeter);
animateVUMeter();  // Start loop
```

---

## 14. WebView ES6 Module Loading - type="module" Required (CRITICAL)

### ❌ WRONG (Knobs freeze - getSliderState undefined)
```html
<script src="js/juce/index.js"></script>
<script>
    let sliderState = window.__JUCE__.backend.getSliderState(paramId);
    // Returns null - ES6 exports not accessible
</script>
```

### ✅ CORRECT
```html
<script type="module" src="js/juce/index.js"></script>
<script type="module">
    import { getSliderState } from './js/juce/index.js';
    let sliderState = getSliderState(paramId);  // Works!
</script>
```

**Why:** JUCE 8's `index.js` uses ES6 `export` syntax, requires `type="module"`.

---

## Quick Reference

| Pattern | What It Prevents |
|---------|-----------------|
| Module Requirements | Missing WebView support |
| Threading Safety | UI crashes |
| Slider Bounds | Non-interactive controls |
| Parameter Binding | Mismatched event types |
| Resource Provider | 404 errors, broken UI |
| Install to System | Testing stale builds |
| Member Initialization | Initialization order crashes |
| 3-Param Attachment | Frozen knobs (silent failure) |
| check_native_interop | Broken JS ↔ C++ bridge |
| valueChangedEvent | NaN values, broken visuals |
| Relative Drag | Jumpy knob behavior |
| getToggleState | Wrong API for booleans |
| VU Animation Loop | Jerky meter movement |
| ES6 Modules | getSliderState undefined |

---

**Full patterns file:** `troubleshooting/patterns/juce8-critical-patterns.md`
