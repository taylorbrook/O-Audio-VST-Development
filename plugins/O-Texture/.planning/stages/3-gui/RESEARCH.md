# Stage 3: GUI - Research

**Researched:** 2026-02-14
**Domain:** JUCE 8 WebBrowserComponent Relay/Attachment API, WebView Plugin GUI
**Confidence:** HIGH

## Summary

This research covers the JUCE 8.0.4 WebBrowserComponent relay and attachment system for building a WebView-based plugin GUI for O-Texture. All findings are sourced directly from the JUCE 8.0.4 source code at `/Users/taylorbrook/JUCE` and from working plugin implementations in this codebase (O-TextureForge, O-Bells, O-Chorus, etc.).

The relay/attachment pattern is a three-layer architecture: C++ relay objects bridge between APVTS parameters and JavaScript state objects inside the WebView. The JUCE frontend JavaScript module (`index.js` + `check_native_interop.js`) provides `getSliderState()`, `getToggleState()`, and `getComboBoxState()` functions that return state objects synchronized with the C++ relays. Parameter attachments bind relays to APVTS parameters, handling normalization and gesture management automatically.

The O-Texture plugin requires 10 parameter bindings across 3 relay types: 7 WebSliderRelays (X, Y, CHARACTER_A, CHARACTER_B, EVOLVE, BRIGHTNESS, MIX), 2 WebComboBoxRelays (SOURCE, MODE), and 1 WebToggleButtonRelay (FREEZE). The XY pad requires custom JavaScript pointer event handling that drives two SliderState objects (X and Y) simultaneously.

**Primary recommendation:** Follow the unique_ptr relay/attachment pattern established in O-TextureForge, with JUCE frontend JS module served via resource provider, and a Canvas-based XY pad driving two WebSliderRelays via `setNormalisedValue()` + `sliderDragStarted()`/`sliderDragEnded()`.

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
- Ouaricon Naturalist aesthetic (aged paper, earth tones, serif typography, botanical motifs)
- XY pad dominant layout matching BRIEF ASCII mockup: XY pad ~50% left, vertical sliders right (CharA, CharB, Evolve), source selector below, Brightness/Mix/Freeze at bottom
- Orbital trail visualization in XY pad: cursor leaves fading trail, Evolve creates orbital motion
- Dark inset parchment XY pad surface (#D4C4B0 or #C8B8A0) with inset shadow, botanical green trails (#6B8E4E)
- Ice crystal overlay for Freeze state
- Naturalist line art icons for 6 source categories (brown ink line drawings)
- Icon buttons for source selector (6 square buttons)
- Lichen/fungi botanical illustration as decorative overlay (right side, 0.3-0.4 opacity)
- 800x600 window
- 10 parameters all bound via WebView relays + APVTS attachments
- Canvas-based XY pad rendering (HTML5 Canvas 2D)
- requestAnimationFrame throttled to 30fps

### Claude's Discretion
- Exact CSS/JS file organization within Source/ui/public/
- Specific Canvas rendering implementation details for trails
- How to structure the JavaScript module imports
- Whether to inline SVG icons or serve as separate files

### Deferred Ideas (OUT OF SCOPE)
- None explicitly deferred
</user_constraints>

## Standard Stack

The JUCE 8.0.4 WebBrowserComponent relay system is the established pattern. No external libraries needed.

### Core (C++ Side)
| Class | Header | Purpose | Why Standard |
|-------|--------|---------|--------------|
| `WebSliderRelay` | `juce_WebControlRelays.h` | Bridges float parameters to JS SliderState | JUCE built-in, handles bidirectional sync |
| `WebToggleButtonRelay` | `juce_WebControlRelays.h` | Bridges bool parameters to JS ToggleState | JUCE built-in, handles bidirectional sync |
| `WebComboBoxRelay` | `juce_WebControlRelays.h` | Bridges choice parameters to JS ComboBoxState | JUCE built-in, handles bidirectional sync |
| `WebSliderParameterAttachment` | `juce_ParameterAttachments.h` | Binds WebSliderRelay to RangedAudioParameter | JUCE built-in, handles normalization/gestures |
| `WebToggleButtonParameterAttachment` | `juce_ParameterAttachments.h` | Binds WebToggleButtonRelay to RangedAudioParameter | JUCE built-in, handles normalization/gestures |
| `WebComboBoxParameterAttachment` | `juce_ParameterAttachments.h` | Binds WebComboBoxRelay to RangedAudioParameter | JUCE built-in, handles normalization/gestures |
| `WebControlParameterIndexReceiver` | `juce_WebControlParameterIndexReceiver.h` | Receives parameter index from JS for DAW hover | JUCE built-in, enables DAW parameter highlighting |

### Core (JS Side)
| File | Purpose | Why Standard |
|------|---------|--------------|
| `index.js` (JUCE frontend module) | Exports `getSliderState`, `getToggleState`, `getComboBoxState`, `getBackendResourceAddress`, `getNativeFunction` | JUCE-provided, must be served verbatim from `juce_gui_extra/native/javascript/` |
| `check_native_interop.js` | Bootstraps `window.__JUCE__` backend object | Required dependency of index.js, must be served alongside it |

### Supporting
| Library | Purpose | When to Use |
|---------|---------|-------------|
| BinaryData (JUCE) | Embed HTML/CSS/JS/PNG into plugin binary | Always -- no external file dependencies |
| `WebBrowserComponent::Options` | Configure WebView with relays, resource provider, native functions | Always -- single Options chain in constructor |

## Architecture Patterns

### Critical: C++ Member Declaration Order

**This is the single most important pattern.** Relays, WebView, and attachments MUST be declared in specific order because C++ destroys members in reverse declaration order. Attachments call into relays during destruction, and relays reference the WebView.

```
class PluginEditor {
    // 1. RELAYS -- destroyed LAST (after WebView)
    std::unique_ptr<juce::WebSliderRelay> xRelay;
    // ...

    // 2. WEBVIEW -- destroyed SECOND
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // 3. ATTACHMENTS -- destroyed FIRST (while WebView and relays still alive)
    std::unique_ptr<juce::WebSliderParameterAttachment> xAttachment;
    // ...
};
```

**Why unique_ptr:** This project uses `std::unique_ptr` for all relays and attachments (verified in O-TextureForge, O-Bells, O-Chorus, etc.). This allows explicit destruction order control in the destructor. The JUCE demo uses stack-allocated members with member initializer lists (which also works because declaration order = destruction reverse order), but `unique_ptr` is more explicit and matches our codebase convention.

### Pattern 1: Relay Declaration and WebView Options Chain

**Source:** JUCE 8.0.4 `WebViewPluginDemo.h` and O-TextureForge `PluginEditor.cpp`

```cpp
// In constructor:

// 1. Create relays
xRelay = std::make_unique<juce::WebSliderRelay>("xSlider");
yRelay = std::make_unique<juce::WebSliderRelay>("ySlider");
sourceRelay = std::make_unique<juce::WebComboBoxRelay>("sourceCombo");
freezeRelay = std::make_unique<juce::WebToggleButtonRelay>("freezeToggle");

// 2. Create WebView with relays chained into options
webView = std::make_unique<juce::WebBrowserComponent>(
    juce::WebBrowserComponent::Options{}
        .withBackend(juce::WebBrowserComponent::Options::Backend::webview2)
        .withWinWebView2Options(
            juce::WebBrowserComponent::Options::WinWebView2{}
                .withUserDataFolder(juce::File::getSpecialLocation(
                    juce::File::SpecialLocationType::tempDirectory)
                        .getChildFile("OTexture_WebView")))
        .withNativeIntegrationEnabled()
        .withResourceProvider([this](const auto& url) { return getResource(url); })
        // Chain ALL relays here
        .withOptionsFrom(*xRelay)
        .withOptionsFrom(*yRelay)
        .withOptionsFrom(*sourceRelay)
        .withOptionsFrom(*freezeRelay)
        // ... all other relays
);

addAndMakeVisible(*webView);

// 3. Create attachments AFTER webView
xAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
    *processorRef.parameters.getParameter("X"), *xRelay);
yAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
    *processorRef.parameters.getParameter("Y"), *yRelay);
sourceAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
    *processorRef.parameters.getParameter("SOURCE"), *sourceRelay);
freezeAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
    *processorRef.parameters.getParameter("FREEZE"), *freezeRelay);
```

### Pattern 2: Explicit Destructor Ordering

**Source:** O-TextureForge `PluginEditor.cpp` lines 134-166

```cpp
TextureEditor::~TextureEditor()
{
    // Destroy in REVERSE: attachments first, then webView, then relays
    mixAttachment.reset();
    brightnessAttachment.reset();
    freezeAttachment.reset();
    // ... all attachments

    webView.reset();

    mixRelay.reset();
    brightnessRelay.reset();
    freezeRelay.reset();
    // ... all relays
}
```

### Pattern 3: Resource Provider for Multiple Files

**Source:** O-TextureForge `PluginEditor.cpp` lines 215-268

The resource provider maps URL paths to BinaryData entries. Each file added to `juce_add_binary_data()` in CMakeLists.txt gets a sanitized identifier:

```cpp
std::optional<juce::WebBrowserComponent::Resource>
TextureEditor::getResource(const juce::String& url)
{
    auto makeVector = [](const char* data, int size) {
        return std::vector<std::byte>(
            reinterpret_cast<const std::byte*>(data),
            reinterpret_cast<const std::byte*>(data) + size);
    };

    if (url == "/" || url == "/index.html")
        return juce::WebBrowserComponent::Resource{
            makeVector(BinaryData::index_html, BinaryData::index_htmlSize),
            juce::String("text/html")};

    if (url == "/css/ouaricon-naturalist.css")
        return juce::WebBrowserComponent::Resource{
            makeVector(BinaryData::ouariconnaturalist_css, BinaryData::ouariconnaturalist_cssSize),
            juce::String("text/css")};

    if (url == "/js/juce/index.js")
        return juce::WebBrowserComponent::Resource{
            makeVector(BinaryData::index_js, BinaryData::index_jsSize),
            juce::String("application/javascript")};

    if (url == "/js/juce/check_native_interop.js")
        return juce::WebBrowserComponent::Resource{
            makeVector(BinaryData::check_native_interop_js, BinaryData::check_native_interop_jsSize),
            juce::String("application/javascript")};

    if (url == "/js/main.js")
        return juce::WebBrowserComponent::Resource{
            makeVector(BinaryData::main_js, BinaryData::main_jsSize),
            juce::String("application/javascript")};

    if (url == "/img/lichen.png")
        return juce::WebBrowserComponent::Resource{
            makeVector(BinaryData::lichen_png, BinaryData::lichen_pngSize),
            juce::String("image/png")};

    juce::Logger::writeToLog("Resource not found: " + url);
    return std::nullopt;
}
```

### Pattern 4: CMakeLists.txt BinaryData Setup for UI Resources

```cmake
juce_add_binary_data(${PROJECT_NAME}_UIResources
    SOURCES
        Source/ui/public/index.html
        Source/ui/public/css/ouaricon-naturalist.css
        Source/ui/public/js/juce/index.js
        Source/ui/public/js/juce/check_native_interop.js
        Source/ui/public/js/main.js
        Source/ui/public/img/lichen.png
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        ${PROJECT_NAME}_UIResources
)
```

**BinaryData identifier rules:**
- Dots become underscores: `index.html` -> `index_html`
- Hyphens become underscores: `ouaricon-naturalist.css` -> `ouariconnaturalist_css`
- Path separators stripped: just the filename becomes the identifier
- Size companion: `BinaryData::index_html` + `BinaryData::index_htmlSize`

### Pattern 5: JavaScript Frontend Module Loading

**Source:** O-TextureForge `index.html` and JUCE `index.js`

The HTML must load the JUCE frontend JS module. Two proven approaches:

**Approach A (O-TextureForge pattern -- import then expose globally):**
```html
<script type="module" src="./js/juce/index.js"></script>
<script>
    import('./js/juce/index.js').then(m => {
        window.getSliderState = m.getSliderState;
        window.getComboBoxState = m.getComboBoxState;
        window.getToggleState = m.getToggleState;
        window.getBackendResourceAddress = m.getBackendResourceAddress;
    });
</script>
```

**Approach B (single-file -- inline import in main.js):**
```html
<script type="module" src="./js/main.js"></script>
```
Where main.js starts with:
```javascript
import { getSliderState, getToggleState, getComboBoxState, getBackendResourceAddress } from './juce/index.js';
```

**Recommendation:** Use Approach B -- cleaner, single entry point, no global pollution.

### Recommended File Structure
```
Source/ui/public/
  index.html              -- Main HTML page, links CSS and JS
  css/
    ouaricon-naturalist.css  -- Naturalist aesthetic CSS
  js/
    juce/
      index.js              -- JUCE frontend module (copied from JUCE source)
      check_native_interop.js  -- JUCE interop bootstrap (copied from JUCE source)
    main.js                 -- App logic: relay binding, XY pad, animations, knobs, sliders
  img/
    lichen.png              -- Botanical overlay image (transparent PNG)
```

### Anti-Patterns to Avoid
- **Declaring attachments before webView:** Will crash. C++ destroys in reverse order; attachments must be declared AFTER webView.
- **Forgetting `.withOptionsFrom()` for a relay:** The relay will never connect. Each relay MUST be chained into Options.
- **Forgetting `.withNativeIntegrationEnabled()`:** The `window.__JUCE__` object will not be injected. Required for relays.
- **Using `goToURL()` before `addAndMakeVisible()`:** WebView may not be ready. Call `goToURL()` after adding to component.
- **Not serving `check_native_interop.js`:** The `index.js` module imports it with `import "./check_native_interop.js"`. If missing, the module fails to load silently.
- **Modifying JUCE's `index.js` or `check_native_interop.js`:** Copy verbatim from JUCE source. Any modification breaks the relay protocol.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Parameter-to-JS sync | Custom event listeners | WebSliderRelay + WebSliderParameterAttachment | Handles normalization, gestures, thread safety |
| Choice parameter sync | Custom string events | WebComboBoxRelay + WebComboBoxParameterAttachment | Handles choice index mapping, properties push |
| Bool parameter sync | Custom boolean events | WebToggleButtonRelay + WebToggleButtonParameterAttachment | Handles toggle state + properties |
| JS frontend bridge | Custom postMessage protocol | JUCE `index.js` module (`getSliderState`, etc.) | Proven protocol, handles init data, events, promises |
| Resource serving | File system paths or fetch() | `withResourceProvider()` + BinaryData | Cross-platform, no file system access needed |
| Parameter index for DAW | Custom event handling | WebControlParameterIndexReceiver + ControlParameterIndexUpdater | JUCE built-in, works with DAW automation overlay |

**Key insight:** The entire relay/attachment/JS-module system is designed as a cohesive unit. Using part of it custom and part JUCE will break the synchronization protocol. Use the full stack or none of it.

## Common Pitfalls

### Pitfall 1: Member Declaration Order Crash
**What goes wrong:** Plugin crashes on editor close with access violation in WebSliderRelay::removeListener or similar
**Why it happens:** Attachment destructor calls into relay which references destroyed WebView
**How to avoid:** Strict declaration order: relays -> webView -> attachments. Use explicit `.reset()` in destructor in reverse order.
**Warning signs:** Intermittent crashes when closing plugin editor, especially on Windows

### Pitfall 2: Relay Not Connected (Silent Failure)
**What goes wrong:** JS UI controls don't sync with C++ parameters -- no errors shown
**Why it happens:** Relay not passed to `.withOptionsFrom()` in Options chain, or `withNativeIntegrationEnabled()` missing
**How to avoid:** Every relay must appear in the Options chain. Always enable native integration.
**Warning signs:** Console warning in WebView: "Creating SliderState for 'X', which is unknown to the backend"

### Pitfall 3: BinaryData Identifier Mismatch
**What goes wrong:** Compiler error: `BinaryData::main_js` not found, or resource returns 404
**Why it happens:** BinaryData identifiers are auto-generated from filenames with specific sanitization rules
**How to avoid:** Check generated `BinaryData.h` after CMake configure to see exact identifiers
**Warning signs:** Build error or "Resource not found" in JUCE log

### Pitfall 4: check_native_interop.js Not Served
**What goes wrong:** WebView shows blank page, no JS errors visible
**Why it happens:** `index.js` (JUCE frontend module) starts with `import "./check_native_interop.js"`. If not served at the correct relative path, the module import fails.
**How to avoid:** Always serve both files at `/js/juce/index.js` and `/js/juce/check_native_interop.js`
**Warning signs:** Blank WebView with no visible error

### Pitfall 5: Slider Value Domain Confusion (Scaled vs Normalized)
**What goes wrong:** Slider shows wrong range, or parameter doesn't respond to full range
**Why it happens:** Confusion between normalized [0,1] values and scaled (parameter range) values
**How to avoid:** On the JS side, use `sliderState.setNormalisedValue(v)` where v is [0,1]. The SliderState handles conversion using the properties (start, end, skew) pushed from C++. The `sliderState.getScaledValue()` returns the actual parameter value. `sliderState.getNormalisedValue()` returns [0,1].
**Warning signs:** Controls only work in a tiny range, or parameter jumps to extreme values

### Pitfall 6: Missing sliderDragStarted/sliderDragEnded for Gesture Management
**What goes wrong:** DAW undo doesn't group slider movements, or automation recording is granular
**Why it happens:** Not calling `sliderState.sliderDragStarted()` on pointer down and `sliderState.sliderDragEnded()` on pointer up
**How to avoid:** Always bracket continuous interactions with `sliderDragStarted()` / `sliderDragEnded()` pairs
**Warning signs:** Each tiny slider movement creates a separate undo step in the DAW

## Code Examples

### Complete O-Texture Parameter Relay Mapping

Based on the existing parameter layout in `PluginProcessor.cpp` (lines 262-306):

| Parameter ID | Relay Name | Relay Type | JS Function | JS State Type |
|-------------|------------|------------|-------------|---------------|
| `X` | `"xSlider"` | `WebSliderRelay` | `getSliderState("xSlider")` | `SliderState` |
| `Y` | `"ySlider"` | `WebSliderRelay` | `getSliderState("ySlider")` | `SliderState` |
| `CHARACTER_A` | `"characterASlider"` | `WebSliderRelay` | `getSliderState("characterASlider")` | `SliderState` |
| `CHARACTER_B` | `"characterBSlider"` | `WebSliderRelay` | `getSliderState("characterBSlider")` | `SliderState` |
| `EVOLVE` | `"evolveSlider"` | `WebSliderRelay` | `getSliderState("evolveSlider")` | `SliderState` |
| `BRIGHTNESS` | `"brightnessSlider"` | `WebSliderRelay` | `getSliderState("brightnessSlider")` | `SliderState` |
| `MIX` | `"mixSlider"` | `WebSliderRelay` | `getSliderState("mixSlider")` | `SliderState` |
| `SOURCE` | `"sourceCombo"` | `WebComboBoxRelay` | `getComboBoxState("sourceCombo")` | `ComboBoxState` |
| `MODE` | `"modeCombo"` | `WebComboBoxRelay` | `getComboBoxState("modeCombo")` | `ComboBoxState` |
| `FREEZE` | `"freezeToggle"` | `WebToggleButtonRelay` | `getToggleState("freezeToggle")` | `ToggleState` |

### JavaScript: XY Pad with Two WebSliderRelays

**Source:** Derived from JUCE 8.0.4 `index.js` SliderState API (lines 135-271)

```javascript
import { getSliderState } from './juce/index.js';

const xState = getSliderState("xSlider");
const yState = getSliderState("ySlider");

const canvas = document.getElementById('xy-pad');
let isDragging = false;

canvas.addEventListener('pointerdown', (e) => {
    isDragging = true;
    canvas.setPointerCapture(e.pointerId);

    // Begin gesture for BOTH parameters simultaneously
    xState.sliderDragStarted();
    yState.sliderDragStarted();

    updateXY(e);
});

canvas.addEventListener('pointermove', (e) => {
    if (!isDragging) return;
    updateXY(e);
});

canvas.addEventListener('pointerup', (e) => {
    if (!isDragging) return;
    isDragging = false;

    // End gesture for BOTH parameters
    xState.sliderDragEnded();
    yState.sliderDragEnded();
});

function updateXY(e) {
    const rect = canvas.getBoundingClientRect();
    const normX = Math.max(0, Math.min(1, (e.clientX - rect.left) / rect.width));
    const normY = Math.max(0, Math.min(1, 1.0 - (e.clientY - rect.top) / rect.height)); // Y inverted

    xState.setNormalisedValue(normX);
    yState.setNormalisedValue(normY);
}

// Listen for C++ parameter changes (e.g., from automation or Evolve modulation)
xState.valueChangedEvent.addListener(() => {
    // Redraw XY pad cursor at new position
    drawCursor(xState.getNormalisedValue(), yState.getNormalisedValue());
});

yState.valueChangedEvent.addListener(() => {
    drawCursor(xState.getNormalisedValue(), yState.getNormalisedValue());
});
```

### JavaScript: Vertical Slider with WebSliderRelay

```javascript
import { getSliderState } from './juce/index.js';

const charAState = getSliderState("characterASlider");

// Create a vertical slider interaction
function createVerticalSlider(element, sliderState) {
    let isDragging = false;
    let startY, startValue;

    element.addEventListener('pointerdown', (e) => {
        isDragging = true;
        startY = e.clientY;
        startValue = sliderState.getNormalisedValue();
        element.setPointerCapture(e.pointerId);
        sliderState.sliderDragStarted();
    });

    element.addEventListener('pointermove', (e) => {
        if (!isDragging) return;
        const deltaY = startY - e.clientY; // Up = positive
        const sensitivity = 1.0 / element.clientHeight;
        const newValue = Math.max(0, Math.min(1, startValue + deltaY * sensitivity));
        sliderState.setNormalisedValue(newValue);
    });

    element.addEventListener('pointerup', (e) => {
        if (!isDragging) return;
        isDragging = false;
        sliderState.sliderDragEnded();
    });

    // React to backend changes
    sliderState.valueChangedEvent.addListener(() => {
        updateSliderVisual(element, sliderState.getNormalisedValue());
    });

    // React to property changes (range, name, etc.)
    sliderState.propertiesChangedEvent.addListener(() => {
        element.querySelector('.label').textContent = sliderState.properties.name;
    });
}
```

### JavaScript: ComboBox with WebComboBoxRelay (Source Selector)

```javascript
import { getComboBoxState } from './juce/index.js';

const sourceState = getComboBoxState("sourceCombo");

// React to backend pushing choice list and initial value
sourceState.propertiesChangedEvent.addListener(() => {
    // sourceState.properties.choices is a JS array: ["Rain", "Metal", "Wind", "Crowd", "Synth", "Organic"]
    renderSourceButtons(sourceState.properties.choices);
    highlightButton(sourceState.getChoiceIndex());
});

sourceState.valueChangedEvent.addListener(() => {
    highlightButton(sourceState.getChoiceIndex());
});

function onSourceButtonClick(index) {
    sourceState.setChoiceIndex(index);
}
```

**Key detail:** `ComboBoxState.getChoiceIndex()` returns the integer index (0-5). `setChoiceIndex(index)` takes the integer index. The internal value is normalized to [0, 1] but the JS API abstracts this away with `getChoiceIndex`/`setChoiceIndex`.

### JavaScript: Toggle with WebToggleButtonRelay (Freeze)

```javascript
import { getToggleState } from './juce/index.js';

const freezeState = getToggleState("freezeToggle");

freezeState.valueChangedEvent.addListener(() => {
    const isActive = freezeState.getValue(); // boolean
    toggleFreezeVisual(isActive);
});

function onFreezeButtonClick() {
    freezeState.setValue(!freezeState.getValue());
}
```

### JavaScript: Rotary Knob Interaction

```javascript
function createRotaryKnob(element, sliderState) {
    let isDragging = false;
    let startY, startValue;

    element.addEventListener('pointerdown', (e) => {
        isDragging = true;
        startY = e.clientY;
        startValue = sliderState.getNormalisedValue();
        element.setPointerCapture(e.pointerId);
        sliderState.sliderDragStarted();
        e.preventDefault();
    });

    element.addEventListener('pointermove', (e) => {
        if (!isDragging) return;
        const deltaY = startY - e.clientY;
        const sensitivity = 0.005; // Tunable: smaller = slower
        const newValue = Math.max(0, Math.min(1, startValue + deltaY * sensitivity));
        sliderState.setNormalisedValue(newValue);
    });

    element.addEventListener('pointerup', () => {
        if (!isDragging) return;
        isDragging = false;
        sliderState.sliderDragEnded();
    });

    // Double-click to reset
    element.addEventListener('dblclick', () => {
        // Default values are baked into the parameter range
        // getNormalisedValue of default: for 0-1 params default is center
        sliderState.sliderDragStarted();
        sliderState.setNormalisedValue(0.5); // or look up from properties
        sliderState.sliderDragEnded();
    });

    sliderState.valueChangedEvent.addListener(() => {
        updateKnobVisual(element, sliderState.getNormalisedValue());
    });
}
```

### JavaScript: Canvas Orbital Trail Animation (30fps throttled)

```javascript
const trailPoints = [];
const MAX_TRAIL_LENGTH = 60; // ~2 seconds at 30fps
let lastFrameTime = 0;
const FRAME_INTERVAL = 1000 / 30; // 30fps cap

function animationLoop(timestamp) {
    if (timestamp - lastFrameTime >= FRAME_INTERVAL) {
        lastFrameTime = timestamp;

        const normX = xState.getNormalisedValue();
        const normY = yState.getNormalisedValue();

        trailPoints.push({ x: normX, y: normY, age: 0 });
        if (trailPoints.length > MAX_TRAIL_LENGTH) {
            trailPoints.shift();
        }

        // Age all points
        for (const p of trailPoints) {
            p.age++;
        }

        drawTrails();
    }
    requestAnimationFrame(animationLoop);
}

function drawTrails() {
    const ctx = canvas.getContext('2d');
    const w = canvas.width;
    const h = canvas.height;

    ctx.clearRect(0, 0, w, h);

    // Draw trail
    for (let i = 0; i < trailPoints.length; i++) {
        const p = trailPoints[i];
        const alpha = 1.0 - (p.age / MAX_TRAIL_LENGTH);
        const radius = 2 + alpha * 3;

        ctx.beginPath();
        ctx.arc(p.x * w, (1 - p.y) * h, radius, 0, Math.PI * 2);
        ctx.fillStyle = `rgba(107, 142, 78, ${alpha * 0.8})`; // #6B8E4E botanical green
        ctx.fill();
    }

    // Draw current cursor
    const normX = xState.getNormalisedValue();
    const normY = yState.getNormalisedValue();
    ctx.beginPath();
    ctx.arc(normX * w, (1 - normY) * h, 6, 0, Math.PI * 2);
    ctx.fillStyle = '#6B8E4E';
    ctx.fill();
    ctx.strokeStyle = '#4A6B35';
    ctx.lineWidth = 2;
    ctx.stroke();
}

requestAnimationFrame(animationLoop);
```

### C++ PluginEditor.h Complete Member Layout for O-Texture

```cpp
class TextureEditor : public juce::AudioProcessorEditor
{
public:
    explicit TextureEditor(TextureProcessor&);
    ~TextureEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    std::optional<juce::WebBrowserComponent::Resource> getResource(const juce::String& url);

    TextureProcessor& processorRef;

    // ====================================================================
    // CRITICAL MEMBER DECLARATION ORDER: Relays -> WebView -> Attachments
    // ====================================================================

    // 1. RELAYS (destroyed last)
    std::unique_ptr<juce::WebSliderRelay> xRelay;
    std::unique_ptr<juce::WebSliderRelay> yRelay;
    std::unique_ptr<juce::WebSliderRelay> characterARelay;
    std::unique_ptr<juce::WebSliderRelay> characterBRelay;
    std::unique_ptr<juce::WebSliderRelay> evolveRelay;
    std::unique_ptr<juce::WebSliderRelay> brightnessRelay;
    std::unique_ptr<juce::WebSliderRelay> mixRelay;
    std::unique_ptr<juce::WebComboBoxRelay> sourceRelay;
    std::unique_ptr<juce::WebComboBoxRelay> modeRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> freezeRelay;

    // 2. WEBVIEW (destroyed second)
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // 3. ATTACHMENTS (destroyed first)
    std::unique_ptr<juce::WebSliderParameterAttachment> xAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> yAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> characterAAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> characterBAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> evolveAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> brightnessAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> mixAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> sourceAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> modeAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> freezeAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TextureEditor)
};
```

### C++ PluginEditor.cpp Constructor Pattern

```cpp
TextureEditor::TextureEditor(TextureProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    // 1. Create relays
    xRelay          = std::make_unique<juce::WebSliderRelay>("xSlider");
    yRelay          = std::make_unique<juce::WebSliderRelay>("ySlider");
    characterARelay = std::make_unique<juce::WebSliderRelay>("characterASlider");
    characterBRelay = std::make_unique<juce::WebSliderRelay>("characterBSlider");
    evolveRelay     = std::make_unique<juce::WebSliderRelay>("evolveSlider");
    brightnessRelay = std::make_unique<juce::WebSliderRelay>("brightnessSlider");
    mixRelay        = std::make_unique<juce::WebSliderRelay>("mixSlider");
    sourceRelay     = std::make_unique<juce::WebComboBoxRelay>("sourceCombo");
    modeRelay       = std::make_unique<juce::WebComboBoxRelay>("modeCombo");
    freezeRelay     = std::make_unique<juce::WebToggleButtonRelay>("freezeToggle");

    // 2. Create WebView with all relay options
    webView = std::make_unique<juce::WebBrowserComponent>(
        juce::WebBrowserComponent::Options{}
            .withBackend(juce::WebBrowserComponent::Options::Backend::webview2)
            .withWinWebView2Options(
                juce::WebBrowserComponent::Options::WinWebView2{}
                    .withUserDataFolder(juce::File::getSpecialLocation(
                        juce::File::SpecialLocationType::tempDirectory)
                            .getChildFile("OTexture_WebView")))
            .withNativeIntegrationEnabled()
            .withKeepPageLoadedWhenBrowserIsHidden()
#if JUCE_WEB_BROWSER_RESOURCE_PROVIDER_AVAILABLE
            .withResourceProvider([this](const auto& url) { return getResource(url); })
#endif
            .withOptionsFrom(*xRelay)
            .withOptionsFrom(*yRelay)
            .withOptionsFrom(*characterARelay)
            .withOptionsFrom(*characterBRelay)
            .withOptionsFrom(*evolveRelay)
            .withOptionsFrom(*brightnessRelay)
            .withOptionsFrom(*mixRelay)
            .withOptionsFrom(*sourceRelay)
            .withOptionsFrom(*modeRelay)
            .withOptionsFrom(*freezeRelay)
    );

    addAndMakeVisible(*webView);

    // 3. Create attachments
    xAttachment          = std::make_unique<juce::WebSliderParameterAttachment>(*processorRef.parameters.getParameter("X"), *xRelay);
    yAttachment          = std::make_unique<juce::WebSliderParameterAttachment>(*processorRef.parameters.getParameter("Y"), *yRelay);
    characterAAttachment = std::make_unique<juce::WebSliderParameterAttachment>(*processorRef.parameters.getParameter("CHARACTER_A"), *characterARelay);
    characterBAttachment = std::make_unique<juce::WebSliderParameterAttachment>(*processorRef.parameters.getParameter("CHARACTER_B"), *characterBRelay);
    evolveAttachment     = std::make_unique<juce::WebSliderParameterAttachment>(*processorRef.parameters.getParameter("EVOLVE"), *evolveRelay);
    brightnessAttachment = std::make_unique<juce::WebSliderParameterAttachment>(*processorRef.parameters.getParameter("BRIGHTNESS"), *brightnessRelay);
    mixAttachment        = std::make_unique<juce::WebSliderParameterAttachment>(*processorRef.parameters.getParameter("MIX"), *mixRelay);
    sourceAttachment     = std::make_unique<juce::WebComboBoxParameterAttachment>(*processorRef.parameters.getParameter("SOURCE"), *sourceRelay);
    modeAttachment       = std::make_unique<juce::WebComboBoxParameterAttachment>(*processorRef.parameters.getParameter("MODE"), *modeRelay);
    freezeAttachment     = std::make_unique<juce::WebToggleButtonParameterAttachment>(*processorRef.parameters.getParameter("FREEZE"), *freezeRelay);

#if JUCE_WEB_BROWSER_RESOURCE_PROVIDER_AVAILABLE
    webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());
#endif

    setSize(800, 600);
}
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| `evaluateJavascript()` for param sync | WebSliderRelay / WebComboBoxRelay / WebToggleButtonRelay | JUCE 7.x -> 8.x | Type-safe, bidirectional, automatic normalization |
| Custom `postMessage()` protocol | `window.__JUCE__.backend.emitEvent()` + `addEventListener()` | JUCE 8.x | Structured event system with init data injection |
| Loading files from disk | `withResourceProvider()` + BinaryData | JUCE 8.x | Cross-platform, no file system access, single binary |

## JUCE Frontend JS Module Internal Details

**Source:** `/Users/taylorbrook/JUCE/modules/juce_gui_extra/native/javascript/index.js`

### SliderState Properties (pushed from C++ WebSliderParameterAttachment::sendInitialUpdate)

```javascript
sliderState.properties = {
    start: 0,       // NormalisableRange start (e.g., 0.0, -1.0)
    end: 1,         // NormalisableRange end (e.g., 1.0)
    skew: 1,        // NormalisableRange skew factor
    name: "",       // Parameter display name
    label: "",      // Parameter label (e.g., "Hz")
    numSteps: 100,  // Number of discrete steps
    interval: 0,    // Step interval (0 = continuous)
    parameterIndex: -1  // Index for DAW automation
};
```

### ComboBoxState Properties

```javascript
comboBoxState.properties = {
    name: "",              // Parameter display name
    parameterIndex: -1,    // Index for DAW automation
    choices: []            // Array of choice strings: ["Rain", "Metal", "Wind", ...]
};
```

### ToggleState Properties

```javascript
toggleState.properties = {
    name: "",              // Parameter display name
    parameterIndex: -1     // Index for DAW automation
};
```

### Key Methods

**SliderState:**
- `getScaledValue()` -- returns the actual parameter value in its native range (e.g., -1.0 to 1.0 for BRIGHTNESS)
- `getNormalisedValue()` -- returns [0, 1] normalized value
- `setNormalisedValue(v)` -- sets from [0, 1] normalized value; handles skew, snapping internally
- `sliderDragStarted()` -- MUST call on pointer down
- `sliderDragEnded()` -- MUST call on pointer up
- `valueChangedEvent.addListener(callback)` -- fires when C++ pushes new value
- `propertiesChangedEvent.addListener(callback)` -- fires on initial update with range/name info

**ComboBoxState:**
- `getChoiceIndex()` -- returns integer index into choices array
- `setChoiceIndex(index)` -- sets by integer index
- `valueChangedEvent.addListener(callback)` -- fires on value change
- `propertiesChangedEvent.addListener(callback)` -- fires with choices array

**ToggleState:**
- `getValue()` -- returns boolean
- `setValue(bool)` -- sets boolean
- `valueChangedEvent.addListener(callback)` -- fires on toggle change
- `propertiesChangedEvent.addListener(callback)` -- fires with name info

## Open Questions

1. **Lichen/fungi PNG source**
   - What we know: O-TextureForge uses `fern.png` as its botanical overlay, served at `/images/fern.png`
   - What's unclear: Whether to reuse that asset or create a new one
   - Recommendation: Create or source a lichen-specific transparent PNG for thematic accuracy. Can use fern.png as placeholder during development.

2. **Canvas vs SVG for XY pad trails**
   - What we know: Canvas is better for frequent redraws (30fps animation loop). SVG is better for static/declarative UIs.
   - What's unclear: Performance impact in WebView2/WKWebView plugin context
   - Recommendation: Use Canvas. O-TextureForge uses Canvas for its scatter plot with a 30Hz timer and it performs well. SVG DOM manipulation at 30fps would be heavier.

3. **ControlParameterIndexUpdater for DAW automation highlighting**
   - What we know: JUCE provides `WebControlParameterIndexReceiver` (C++) and `ControlParameterIndexUpdater` (JS) for reporting which parameter the mouse is hovering over. This enables DAW features like "last touched parameter" display.
   - What's unclear: Whether this is strictly needed for v1.0
   - Recommendation: Implement it. Add `data-parameter-index` attributes to controls and use `ControlParameterIndexUpdater` in the mousemove handler. Small effort, nice DAW integration.

## Sources

### Primary (HIGH confidence)
- `/Users/taylorbrook/JUCE/modules/juce_gui_extra/misc/juce_WebControlRelays.h` -- Relay class declarations
- `/Users/taylorbrook/JUCE/modules/juce_gui_extra/misc/juce_WebControlRelays.cpp` -- Relay implementations
- `/Users/taylorbrook/JUCE/modules/juce_gui_extra/detail/juce_WebControlRelayEvents.h` -- Event protocol
- `/Users/taylorbrook/JUCE/modules/juce_audio_processors/utilities/juce_ParameterAttachments.h` -- Attachment declarations
- `/Users/taylorbrook/JUCE/modules/juce_audio_processors/utilities/juce_ParameterAttachments.cpp` -- Attachment implementations
- `/Users/taylorbrook/JUCE/modules/juce_gui_extra/misc/juce_WebBrowserComponent.h` -- Options API
- `/Users/taylorbrook/JUCE/modules/juce_gui_extra/native/javascript/index.js` -- JS frontend module
- `/Users/taylorbrook/JUCE/modules/juce_gui_extra/native/javascript/check_native_interop.js` -- JS interop bootstrap
- `/Users/taylorbrook/JUCE/examples/Plugins/WebViewPluginDemo.h` -- Official JUCE example
- `/Users/taylorbrook/Dev/VST-development/plugins/O-TextureForge/Source/PluginEditor.h` -- Working production plugin
- `/Users/taylorbrook/Dev/VST-development/plugins/O-TextureForge/Source/PluginEditor.cpp` -- Working production plugin
- `/Users/taylorbrook/Dev/VST-development/plugins/O-Bells/Source/PluginEditor.h` -- Working production plugin
- `/Users/taylorbrook/Dev/VST-development/plugins/O-Texture/Source/PluginProcessor.cpp` -- Existing parameter layout

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH -- All from JUCE 8.0.4 source code
- Architecture: HIGH -- Verified against 3+ working production plugins and JUCE example
- Member declaration order: HIGH -- Verified in JUCE source (relay lifecycle) and working plugins
- JS API: HIGH -- Read directly from JUCE 8.0.4 index.js source
- Pitfalls: HIGH -- Based on actual JUCE source + established patterns in this codebase
- XY pad pattern: MEDIUM -- Derived from SliderState API docs; no JUCE example with XY pad specifically, but the API supports it cleanly

**Research date:** 2026-02-14
**Valid until:** Indefinite (JUCE 8.0.4 is the locked version in this project)
