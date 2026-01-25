# Phase 5: WebView UI - Research

**Researched:** 2026-01-25
**Domain:** JUCE 8 WebView integration for audio plugin GUIs
**Confidence:** HIGH

## Summary

Phase 5 requires implementing a WebView-based UI for O-Bass that exposes four controls (Frequency, Enhance, Output, Mode toggle) and matches the Ouaricon botanical visual language. This research establishes the standard patterns already proven in this codebase through O-Tremolo and other Ouaricon plugins.

The implementation follows a well-documented pattern in the project's `juce8-critical-patterns.md` reference. The O-Tremolo plugin serves as the canonical reference implementation, demonstrating the exact file structure, relay patterns, and parameter binding approach required.

**Primary recommendation:** Copy O-Tremolo's WebView architecture verbatim, adapting only the parameter IDs (`crossover_freq`, `enhance`, `enhanceMode`, `output`), control types (3 sliders + 1 toggle vs O-Tremolo's 4 sliders + 2 toggles), and visual assets for the 2x2 grid layout specified in CONTEXT.md.

## Standard Stack

The established libraries/tools for this domain:

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| JUCE 8 WebBrowserComponent | 8.x | Embedded WebView for plugin UI | Built into JUCE, native integration with parameter system |
| WebSliderRelay | 8.x | Bidirectional slider <-> JS binding | JUCE official pattern for continuous parameters |
| WebToggleButtonRelay | 8.x | Bidirectional toggle <-> JS binding | JUCE official pattern for boolean/choice parameters |
| WebSliderParameterAttachment | 8.x | Connects relay to APVTS parameter | JUCE official pattern, handles automation reflection |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| juce_gui_extra | 8.x | Contains WebBrowserComponent | Always required for WebView |
| BinaryData (CMake generated) | N/A | Embeds HTML/JS/CSS/images in binary | Always - avoids external file dependencies |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| WebView UI | Native JUCE Components | Would work but doesn't match Ouaricon botanical aesthetic; harder to achieve illustrated style |
| Embedded BinaryData | External file loading | Would work but creates deployment issues; BinaryData is the established pattern |

**No installation required** - JUCE 8 includes all WebView components. CMake handles BinaryData generation.

## Architecture Patterns

### Recommended Project Structure
```
plugins/OBass/
├── Source/
│   ├── PluginProcessor.cpp    # DSP (already complete)
│   ├── PluginProcessor.h
│   ├── PluginEditor.cpp       # WebView setup (REPLACE generic editor)
│   ├── PluginEditor.h
│   └── ui/
│       └── public/
│           ├── index.html     # Main UI with inline CSS/JS
│           ├── js/
│           │   └── juce/
│           │       ├── index.js              # JUCE bridge (copy from O-Tremolo)
│           │       └── check_native_interop.js  # Required for WebView init
│           └── img/
│               ├── paper.jpg      # Paper texture background
│               └── botanical.png  # Botanical illustration overlay
├── CMakeLists.txt             # Add juce_add_binary_data for UI resources
└── ...
```

### Pattern 1: Member Declaration Order (CRITICAL)
**What:** WebView components must be declared in specific order for correct destruction
**When to use:** ALL WebView editors
**Example:**
```cpp
// Source: juce8-critical-patterns.md Pattern #11
class OBassAudioProcessorEditor : public juce::AudioProcessorEditor {
private:
    OBassAudioProcessor& processorRef;

    // 1. Relays FIRST (no dependencies)
    std::unique_ptr<juce::WebSliderRelay> frequencyRelay;
    std::unique_ptr<juce::WebSliderRelay> enhanceRelay;
    std::unique_ptr<juce::WebSliderRelay> outputRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> modeRelay;

    // 2. WebView SECOND (depends on relays via withOptionsFrom)
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // 3. Attachments LAST (depend on both relays and parameters)
    std::unique_ptr<juce::WebSliderParameterAttachment> frequencyAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> enhanceAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> outputAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> modeAttachment;
};
```

### Pattern 2: Constructor Initialization Order
**What:** Create components in matching order: relays, then WebView with options, then attachments
**When to use:** ALL WebView editors
**Example:**
```cpp
// Source: O-Tremolo/Source/PluginEditor.cpp (verified codebase pattern)
OBassAudioProcessorEditor::OBassAudioProcessorEditor(OBassAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    // 1. Create relays FIRST (with relay IDs matching HTML)
    frequencyRelay = std::make_unique<juce::WebSliderRelay>("frequency");
    enhanceRelay = std::make_unique<juce::WebSliderRelay>("enhance");
    outputRelay = std::make_unique<juce::WebSliderRelay>("output");
    modeRelay = std::make_unique<juce::WebToggleButtonRelay>("mode");

    // 2. Create WebView SECOND with all relay options
    webView = std::make_unique<juce::WebBrowserComponent>(
        juce::WebBrowserComponent::Options{}
            .withNativeIntegrationEnabled()
            .withResourceProvider([this](auto& url) { return getResource(url); })
            .withOptionsFrom(*frequencyRelay)
            .withOptionsFrom(*enhanceRelay)
            .withOptionsFrom(*outputRelay)
            .withOptionsFrom(*modeRelay)
    );

    // 3. Create attachments LAST (Pattern #12: 3 parameters required)
    frequencyAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("crossover_freq"), *frequencyRelay, nullptr);
    enhanceAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("enhance"), *enhanceRelay, nullptr);
    outputAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("output"), *outputRelay, nullptr);
    modeAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.parameters.getParameter("enhanceMode"), *modeRelay, nullptr);

    addAndMakeVisible(*webView);
    setSize(500, 450);  // Per CONTEXT.md decision
}
```

### Pattern 3: Explicit URL Mapping (Resource Provider)
**What:** Map HTML request URLs to BinaryData identifiers explicitly
**When to use:** ALL WebView editors
**Example:**
```cpp
// Source: juce8-critical-patterns.md Pattern #8
std::optional<juce::WebBrowserComponent::Resource>
OBassAudioProcessorEditor::getResource(const juce::String& url)
{
    auto makeVector = [](const char* data, int size) {
        return std::vector<std::byte>(
            reinterpret_cast<const std::byte*>(data),
            reinterpret_cast<const std::byte*>(data) + size
        );
    };

    // Root "/" -> index.html
    if (url == "/" || url == "/index.html") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::index_html, BinaryData::index_htmlSize),
            juce::String("text/html")
        };
    }

    // JUCE JavaScript bridge
    if (url == "/js/juce/index.js") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::index_js, BinaryData::index_jsSize),
            juce::String("text/javascript")
        };
    }

    // JUCE interop checker (Pattern #13: REQUIRED)
    if (url == "/js/juce/check_native_interop.js") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::check_native_interop_js,
                      BinaryData::check_native_interop_jsSize),
            juce::String("text/javascript")
        };
    }

    // Background image
    if (url == "/img/paper.jpg") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::paper_jpg, BinaryData::paper_jpgSize),
            juce::String("image/jpeg")
        };
    }

    // Botanical overlay (new for O-Bass)
    if (url == "/img/botanical.png") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::botanical_png, BinaryData::botanical_pngSize),
            juce::String("image/png")
        };
    }

    juce::Logger::writeToLog("Resource not found: " + url);
    return std::nullopt;
}
```

### Pattern 4: parentHierarchyChanged Navigation
**What:** Navigate WebView only after editor is attached to window
**When to use:** ALL WebView editors (prevents crash during plugin scanning)
**Example:**
```cpp
// Source: O-Tremolo/Source/PluginEditor.cpp
void OBassAudioProcessorEditor::parentHierarchyChanged()
{
    if (isShowing() && webView != nullptr && !hasNavigated)
    {
        webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());
        hasNavigated = true;
    }
}
```

### Pattern 5: ES6 Module Import in JavaScript
**What:** Use `type="module"` and import JUCE functions
**When to use:** ALL WebView HTML files
**Example:**
```html
<!-- Source: juce8-critical-patterns.md Pattern #21 -->
<script type="module" src="js/juce/check_native_interop.js"></script>
<script type="module">
    import { getSliderState, getToggleState } from './js/juce/index.js';

    // Parameter binding
    const frequencyState = getSliderState('frequency');
    const modeState = getToggleState('mode');

    // Setup knob/toggle handlers...
</script>
```

### Pattern 6: Relative Drag for Knobs
**What:** Use frame-delta drag, not absolute positioning
**When to use:** ALL rotary knob implementations
**Example:**
```javascript
// Source: juce8-critical-patterns.md Pattern #16
let lastY = 0;
let isDragging = false;

knob.addEventListener('mousedown', (e) => {
    isDragging = true;
    lastY = e.clientY;
    state.sliderDragStarted();
});

document.addEventListener('mousemove', (e) => {
    if (!isDragging) return;

    const deltaY = lastY - e.clientY;  // Frame delta, not total
    const currentNorm = state.getNormalisedValue();
    const sensitivity = 0.005;
    const newNorm = Math.max(0, Math.min(1, currentNorm + deltaY * sensitivity));

    state.setNormalisedValue(newNorm);
    lastY = e.clientY;  // Update for next frame
});

document.addEventListener('mouseup', () => {
    if (isDragging) {
        state.sliderDragEnded();
        isDragging = false;
    }
});
```

### Anti-Patterns to Avoid
- **Generic URL loop:** Don't iterate BinaryData to match URLs - use explicit mapping
- **Raw member variables:** Use `std::unique_ptr` for all WebView components
- **2-param attachment constructor:** Always pass `nullptr` as third parameter to WebSliderParameterAttachment
- **Missing check_native_interop.js:** Will cause silent initialization failure
- **Absolute knob positioning:** Don't map mouse Y directly to rotation

## Don't Hand-Roll

Problems that look simple but have existing solutions:

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Parameter <-> JS binding | Custom message passing | WebSliderRelay + WebSliderParameterAttachment | JUCE handles all synchronization, undo, automation |
| Value smoothing | JavaScript smoothing | JUCE's built-in smoothed parameters | Already implemented in DSP, UI just reflects state |
| Double-click reset | Custom gesture detection | JUCE parameter default value + event handling | Pattern in O-Tremolo shows how |
| File embedding | External file loading | juce_add_binary_data() | Standard JUCE pattern, avoids deployment issues |

**Key insight:** The JUCE 8 WebView bridge handles all the hard problems (thread safety, synchronization, automation). The UI just needs to read/write normalized values and update visuals.

## Common Pitfalls

### Pitfall 1: Wrong Attachment Constructor Signature
**What goes wrong:** Knobs display but don't respond to drag
**Why it happens:** JUCE 8 changed WebSliderParameterAttachment to require 3 parameters (was 2 in JUCE 7)
**How to avoid:** Always pass `nullptr` as third parameter
**Warning signs:** UI loads, knobs visible, but completely frozen

### Pitfall 2: Missing check_native_interop.js
**What goes wrong:** Native bridge fails silently
**Why it happens:** File not included in BinaryData or not served by resource provider
**How to avoid:** Always include in CMakeLists.txt and resource handler
**Warning signs:** Similar to wrong attachment signature - frozen UI

### Pitfall 3: Wrong Script Type Attribute
**What goes wrong:** getSliderState/getToggleState undefined
**Why it happens:** ES6 modules require `type="module"` attribute
**How to avoid:** Use `<script type="module">` for all JUCE-interacting scripts
**Warning signs:** Console errors about undefined functions

### Pitfall 4: Callback Parameters Expected
**What goes wrong:** Values undefined in event handlers
**Why it happens:** JUCE's valueChangedEvent passes no parameters to callback
**How to avoid:** Call `state.getNormalisedValue()` inside callback, not use callback parameter
**Warning signs:** `NaN` rotation, knobs stuck at 12 o'clock

### Pitfall 5: Navigation Before Window Attachment
**What goes wrong:** Crash during plugin scanning
**Why it happens:** goToURL() called in constructor before WebView has window context
**How to avoid:** Navigate in parentHierarchyChanged() with isShowing() guard
**Warning signs:** Plugin crashes during DAW scan, works in standalone

## Code Examples

### CMakeLists.txt Addition (BinaryData)
```cmake
# Source: O-Tremolo/CMakeLists.txt pattern
# WebView UI Resources
juce_add_binary_data(OBass_UIResources
    SOURCES
        Source/ui/public/index.html
        Source/ui/public/js/juce/index.js
        Source/ui/public/js/juce/check_native_interop.js
        Source/ui/public/img/paper.jpg
        Source/ui/public/img/botanical.png
)

target_link_libraries(OBass
    PRIVATE
        OBass_UIResources
)
```

### HTML Structure (index.html skeleton)
```html
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>O-Bass</title>
    <style>
        /* Inline CSS for performance */
        * { margin: 0; padding: 0; box-sizing: border-box; user-select: none; }
        html, body { width: 100%; height: 100%; overflow: hidden; }
        body { font-family: 'Garamond', serif; }

        .plugin-container {
            width: 500px;
            height: 450px;
            position: relative;
        }

        .background {
            position: absolute;
            width: 100%;
            height: 100%;
            object-fit: cover;
        }

        /* 2x2 grid for controls per CONTEXT.md */
        .controls-grid {
            display: grid;
            grid-template-columns: 1fr 1fr;
            grid-template-rows: 1fr 1fr;
            gap: 20px;
            padding: 20px;
        }

        /* Botanical knob style matching O-Tremolo */
        .knob { /* ... */ }
    </style>
</head>
<body>
    <div class="plugin-container">
        <img src="img/paper.jpg" class="background">
        <img src="img/botanical.png" class="botanical-overlay">

        <div class="content">
            <div class="header">
                <h1 class="title">O-Bass</h1>
            </div>

            <div class="controls-grid">
                <!-- Top row: Frequency + Enhance -->
                <div class="knob-container" data-param="frequency">
                    <div class="knob"><div class="knob-indicator"></div></div>
                    <div class="knob-label">FREQUENCY</div>
                    <div class="knob-value">80 Hz</div>
                </div>
                <div class="knob-container" data-param="enhance">
                    <div class="knob"><div class="knob-indicator"></div></div>
                    <div class="knob-label">ENHANCE</div>
                    <div class="knob-value">50%</div>
                </div>

                <!-- Bottom row: Output + Mode -->
                <div class="knob-container" data-param="output">
                    <div class="knob"><div class="knob-indicator"></div></div>
                    <div class="knob-label">OUTPUT</div>
                    <div class="knob-value">0 dB</div>
                </div>
                <div class="toggle-container" data-param="mode">
                    <div class="toggle"></div>
                    <div class="toggle-label">MODE</div>
                </div>
            </div>

            <!-- Limit indicator LED -->
            <div class="limit-led"></div>
        </div>
    </div>

    <script type="module" src="js/juce/check_native_interop.js"></script>
    <script type="module">
        import { getSliderState, getToggleState } from './js/juce/index.js';
        // ... parameter binding code
    </script>
</body>
</html>
```

### Toggle State Usage for Mode
```javascript
// Source: juce8-critical-patterns.md Pattern #19
const modeState = getToggleState('mode');

function updateToggleVisual() {
    const isColored = modeState.getValue();  // true = Colored, false = Clean
    toggleElement.classList.toggle('colored', isColored);
    modeLabel.textContent = isColored ? 'COLORED' : 'CLEAN';
}

// JUCE -> UI
modeState.valueChangedEvent.addListener(() => {
    updateToggleVisual();
});

// UI -> JUCE
toggleElement.addEventListener('click', () => {
    modeState.setValue(!modeState.getValue());
});

// Initial state
updateToggleVisual();
```

### Limit Indicator from Processor
```cpp
// PluginProcessor already has: float getLimitIndicator() const
// Need to expose via native function for UI polling

// In PluginEditor.cpp, add native function:
webView = std::make_unique<juce::WebBrowserComponent>(
    juce::WebBrowserComponent::Options{}
        // ... relay options ...
        .withNativeFunction("getLimitIndicator", [this](auto&, auto complete) {
            complete(processorRef.getLimitIndicator());
        })
);
```

```javascript
// In JavaScript, poll periodically
import { getNativeFunction } from './js/juce/index.js';

const getLimitIndicator = getNativeFunction('getLimitIndicator');

function updateLimitLED() {
    getLimitIndicator().then(value => {
        limitLED.style.opacity = 0.3 + value * 0.7;  // 0.3 = dim, 1.0 = full glow
        limitLED.style.boxShadow = `0 0 ${value * 10}px rgba(255, 100, 100, ${value})`;
    });

    requestAnimationFrame(updateLimitLED);
}

updateLimitLED();
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Native JUCE sliders | WebView UI | JUCE 7+ | Enables rich CSS-based visuals |
| 2-param attachment | 3-param attachment (with nullptr) | JUCE 8 | Silent failure if using old signature |
| Regular script tags | ES6 modules | JUCE 8 | Must use `type="module"` |
| External JS files | Embedded BinaryData | Always | Standard JUCE pattern for deployment |

**Deprecated/outdated:**
- `juce::Slider` with `setLookAndFeel()`: Still works but doesn't match Ouaricon botanical aesthetic
- Window manager event for navigation: Use `parentHierarchyChanged()` instead

## Open Questions

Things that couldn't be fully resolved:

1. **Limit indicator polling rate**
   - What we know: O-Tremolo uses requestAnimationFrame for waveform, works at ~60fps
   - What's unclear: Optimal polling rate for CPU/visual balance
   - Recommendation: Start with RAF loop, profile if needed

2. **Botanical image source for O-Bass**
   - What we know: O-Tremolo uses carrot.png, need different botanical for O-Bass
   - What's unclear: Whether to create new asset or reuse existing
   - Recommendation: Claude's discretion per CONTEXT.md; can reuse or create thematic bass-related botanical

## Sources

### Primary (HIGH confidence)
- `/Users/taylorbrook/Dev/VST-development/troubleshooting/patterns/juce8-critical-patterns.md` - Authoritative pattern reference for this codebase
- `/Users/taylorbrook/Dev/VST-development/plugins/O-Tremolo/Source/PluginEditor.cpp` - Working reference implementation
- `/Users/taylorbrook/Dev/VST-development/plugins/O-Tremolo/Source/ui/public/index.html` - Full UI implementation

### Secondary (MEDIUM confidence)
- `/Users/taylorbrook/Dev/VST-development/plugins/OBass/Source/PluginProcessor.cpp` - Current parameter definitions (crossover_freq, enhance, enhanceMode, output)

### Tertiary (LOW confidence)
- None - all patterns verified against working codebase

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - Verified against working O-Tremolo implementation
- Architecture: HIGH - Documented in juce8-critical-patterns.md with multiple working examples
- Pitfalls: HIGH - All patterns documented from actual bugs encountered in this codebase

**Research date:** 2026-01-25
**Valid until:** Indefinite (patterns are codebase-specific, not library-version-dependent)
