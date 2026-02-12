# Stage 3: GUI - Research

> **Date:** 2026-02-11
> **Plugin:** O-Orbit
> **Stage:** 3-gui (WebView UI, Orbital Visualizer, Speaker Layout Editor)

---

## 1. WebView Infrastructure (C++ Side)

### 1.1 Member Declaration Order (CRITICAL)

C++ destroys members in reverse declaration order. `WebSliderParameterAttachment::~()` calls `evaluateJavascript()` on the WebView during destruction. If WebView is destroyed first, the application crashes.

**Correct Order:**
1. **Relays** (destroyed last - safe, no dependencies)
2. **WebBrowserComponent** (destroyed second - still alive when attachments die)
3. **Attachments** (destroyed first - can safely call evaluateJavascript)

**Reference:** `modules/core/webview-relay-manager/cpp/WebViewRelayManager.h`

### 1.2 Relay Types Needed for O-Orbit

O-Orbit has 17 parameters across 3 groups:

| Parameter | ID | Type | Relay Type |
|-----------|---|------|-----------|
| Path | `path` | Choice (4 options) | `WebComboBoxRelay` |
| Speed | `speed` | Float | `WebSliderRelay` |
| Width | `width` | Float | `WebSliderRelay` |
| Depth | `depth` | Float | `WebSliderRelay` |
| Tilt | `tilt` | Float | `WebSliderRelay` |
| Phase | `phase` | Float | `WebSliderRelay` |
| Elevation Enable | `elevation_enable` | Bool | `WebToggleButtonRelay` |
| Elevation Range | `elevation_range` | Float | `WebSliderRelay` |
| Tempo Sync | `tempo_sync` | Choice (15 options) | `WebComboBoxRelay` |
| Speaker Layout | `speaker_layout` | Choice (8 options) | `WebComboBoxRelay` |
| Distance | `distance` | Float | `WebSliderRelay` |
| Air Absorption | `air_absorption` | Float | `WebSliderRelay` |
| Attenuation Curve | `attenuation_curve` | Choice (3 options) | `WebComboBoxRelay` |
| Center Diverge | `center_diverge` | Float | `WebSliderRelay` |
| Source Mode | `source_mode` | Choice (2 options) | `WebComboBoxRelay` |
| L/R Offset | `lr_offset` | Float | `WebSliderRelay` |
| Mix | `mix` | Float | `WebSliderRelay` |

**Totals:** 11 WebSliderRelays + 5 WebComboBoxRelays + 1 WebToggleButtonRelay = 17 relays

### 1.3 WebView Setup Pattern

From established codebase pattern (O-GrainScatter, O-SpectralShaper, O-Bells):

```cpp
// Constructor: Create relays → Build options → Create WebView → Create attachments

// Step 1: Create relays with parameter IDs matching APVTS
speedRelay = std::make_unique<juce::WebSliderRelay>("speed");
pathRelay = std::make_unique<juce::WebComboBoxRelay>("path");
elevEnableRelay = std::make_unique<juce::WebToggleButtonRelay>("elevation_enable");

// Step 2: Build WebView options with all relays registered
auto options = juce::WebBrowserComponent::Options{}
    .withNativeIntegrationEnabled()
    .withResourceProvider([this](auto& url) { return getResource(url); })
    .withOptionsFrom(*speedRelay)
    .withOptionsFrom(*pathRelay)
    .withOptionsFrom(*elevEnableRelay)
    .withNativeFunction("getMotionState", ...)   // Custom relay for visualizer
    .withNativeFunction("getSpeakerLayout", ...) // Custom relay for editor
    .withNativeFunction("saveSpeakerLayout", ...)
    .withNativeFunction("loadSpeakerLayout", ...)
    .withNativeFunction("exportLayout", ...)
    .withNativeFunction("importLayout", ...);

// Step 3: Create attachments connecting APVTS parameters to relays
speedAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
    *processorRef.parameters.getParameter("speed"), *speedRelay, nullptr);
```

### 1.4 Resource Provider Pattern

Explicit URL-to-BinaryData mapping (NOT generic loops):

```cpp
std::optional<juce::WebBrowserComponent::Resource> getResource(const juce::String& url) {
    auto makeVec = [](const char* d, int s) {
        return std::vector<std::byte>(reinterpret_cast<const std::byte*>(d),
                                      reinterpret_cast<const std::byte*>(d) + s);
    };
    if (url == "/" || url == "/index.html")
        return Resource { makeVec(BinaryData::index_html, BinaryData::index_htmlSize), "text/html" };
    if (url == "/css/styles.css")
        return Resource { makeVec(BinaryData::styles_css, BinaryData::styles_cssSize), "text/css" };
    // ... etc for each resource
    return std::nullopt;
}
```

**BinaryData naming:** Slashes and dots become underscores. `js/app.js` → `BinaryData::app_js`.

### 1.5 Lazy Navigation Pattern

Defer WebView navigation until editor is visible (prevents plugin scanner crashes):

```cpp
void parentHierarchyChanged() override {
    if (isShowing() && webView != nullptr && !hasNavigated) {
        webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());
        hasNavigated = true;
    }
}
```

### 1.6 Timer-Based Data Push (C++ → JS)

For real-time motion state updates, use `juce::Timer` + `evaluateJavascript()`:

```cpp
class OOrbitEditor : public juce::AudioProcessorEditor, private juce::Timer {
    void timerCallback() override {
        auto state = processorRef.getMotionSnapshot();
        juce::String js = juce::String::formatted(
            "if(window.updateMotion) window.updateMotion(%f,%f,%f);",
            state.azimuth, state.elevation, state.distance);
        webView->evaluateJavascript(js);
    }
};
// Start timer: startTimerHz(30); // 30fps is sufficient, canvas interpolates to 60
```

**Alternative: Native function relay** — JS calls `getMotionState()` on each animation frame. This is pull-based rather than push-based. Both patterns work; timer-based push is simpler for continuous updates.

---

## 2. Motion State Relay (Audio Thread → UI)

### 2.1 Current MotionState Structure

From `MotionEngine.h`:
```cpp
struct MotionState {
    float azimuth   = 0.0f;   // degrees
    float elevation = 0.0f;   // degrees
    float distance  = 1.0f;   // normalized
};
```

Accessible via `MotionEngine::getCurrentState()`.

### 2.2 Thread-Safe Snapshot

The processor already reads motion state per-block. For UI relay, add an atomic snapshot:

```cpp
// In PluginProcessor.h:
struct MotionSnapshot {
    float azimuthL  = 0.0f;
    float elevationL = 0.0f;
    float azimuthR  = 0.0f;  // For L+R Split mode
    float elevationR = 0.0f;
    float distance  = 1.0f;
    bool  isLRSplit = false;
};

std::atomic<MotionSnapshot> motionSnapshotForUI;
```

`std::atomic` on a POD struct ≤16 bytes is lock-free on most platforms. The `MotionSnapshot` at 24 bytes might need a SpinLock or `std::atomic_ref`. Alternatively, use two separate atomics or a relaxed copy in `processBlock()`.

**Simpler approach:** Use individual `std::atomic<float>` members:
```cpp
std::atomic<float> uiAzimuthL { 0.0f };
std::atomic<float> uiElevationL { 0.0f };
std::atomic<float> uiAzimuthR { 0.0f };
std::atomic<float> uiElevationR { 0.0f };
std::atomic<float> uiDistance { 1.0f };
```

Updated at end of `processBlock()`, read by timer on message thread.

### 2.3 Downmix Status Relay

`DownmixEngine` has `isActive()`, `getSourceChannels()`, `getTargetChannels()`. Relay as native function:

```cpp
.withNativeFunction("getDownmixStatus", [this](auto, auto complete) {
    auto& dm = processorRef.getDownmixEngine();
    juce::DynamicObject::Ptr obj = new juce::DynamicObject();
    obj->setProperty("active", dm.isActive());
    obj->setProperty("sourceChannels", dm.getSourceChannels());
    obj->setProperty("targetChannels", dm.getTargetChannels());
    complete(juce::var(obj.get()));
});
```

---

## 3. Speaker Layout Relay & Editor

### 3.1 Speaker Data Structure

From `SpeakerLayout.h`:
```cpp
struct Speaker {
    float azimuth   = 0.0f;   // degrees, 0=front, +90=left
    float elevation = 0.0f;   // degrees, 0=horizon
    float distance  = 1.0f;   // meters
    juce::String label;
    bool isLFE      = false;
};
```

### 3.2 Speaker Layout Relay to JS

Serialize speaker layout as JSON array for JavaScript:

```cpp
.withNativeFunction("getSpeakerLayout", [this](auto, auto complete) {
    auto& layout = processorRef.getCurrentLayout();
    juce::Array<juce::var> speakers;
    for (const auto& spk : layout.speakers) {
        juce::DynamicObject::Ptr obj = new juce::DynamicObject();
        obj->setProperty("azimuth", spk.azimuth);
        obj->setProperty("elevation", spk.elevation);
        obj->setProperty("distance", spk.distance);
        obj->setProperty("label", spk.label);
        obj->setProperty("isLFE", spk.isLFE);
        speakers.add(juce::var(obj.get()));
    }
    complete(juce::var(speakers));
});
```

### 3.3 Custom Speaker Layout Modifications

For the speaker editor (Phase 3.3), native functions needed:

- `addSpeaker(azimuth, elevation, distance, label)` — Add speaker to custom layout
- `removeSpeaker(index)` — Remove speaker by index
- `moveSpeaker(index, azimuth, elevation)` — Reposition speaker
- `setCustomLayout(speakersJSON)` — Replace entire layout
- `saveSpeakerLayout(name)` — Save to user presets directory
- `loadSpeakerLayout()` — Open FileChooser, load layout file
- `exportLayout()` — Save to user-chosen file (.json)
- `importLayout()` — Open FileChooser, import layout file

Each modification triggers `VBAPComputeThread::requestRecomputation()` on the background thread.

### 3.4 File I/O via Native Function

FileChooser requires async operation in JUCE 8:

```cpp
.withNativeFunction("exportLayout", [this](auto, auto complete) {
    fileChooser = std::make_shared<juce::FileChooser>(
        "Export Speaker Layout", juce::File{}, "*.json");
    fileChooser->launchAsync(
        juce::FileBrowserComponent::saveMode,
        [this, complete](const juce::FileChooser& fc) {
            auto file = fc.getResult();
            if (file == juce::File{}) { complete(juce::var(false)); return; }
            // Serialize current layout to JSON and write
            auto json = layoutToJSON(processorRef.getCurrentLayout());
            file.replaceWithText(json);
            complete(juce::var(true));
        });
});
```

---

## 4. Canvas Animation Architecture

### 4.1 Orbital Visualizer Design

**Canvas approach:** HTML5 Canvas 2D (not WebGL — overkill for simple 2D path visualization).

**Animation loop:**
```javascript
let animFrame;
function renderLoop() {
    drawVisualizerFrame();
    animFrame = requestAnimationFrame(renderLoop);
}
// Start: renderLoop();
// Stop: cancelAnimationFrame(animFrame);
```

**Data flow:**
1. C++ timer pushes motion state at 30Hz via `evaluateJavascript("window.updateMotion(...)")`
2. JS stores latest state in module-level variable
3. Canvas renders at 60fps, interpolating between received states for smoothness

### 4.2 Visualizer Drawing Requirements

Per CONTEXT.md:
- **Path trails:** Warm brown/amber fade lines — store last N positions, draw with decreasing opacity
- **Source dot:** Muted green (#8BA870) glowing dot, 8-10px radius with radial gradient glow
- **L+R split:** Two dots (green for L, amber for R)
- **Speaker icons:** Cream/brown circles with channel labels around perimeter
- **Background:** Slightly darker area ("naturalist's diagram plate") — could use rgba(0,0,0,0.05) overlay on aged paper

**Coordinate mapping:**
- Azimuth 0° = top (12 o'clock), positive = clockwise? Need to confirm convention.
- From SpeakerLayout.h: 0=front, +90=left (counter-clockwise). Canvas: 0° = top, map counter-clockwise.
- Radius represents distance (center = 0, edge = max distance).

### 4.3 High-DPI Canvas Handling

Critical for Retina/HiDPI displays:
```javascript
const dpr = window.devicePixelRatio || 1;
canvas.width = canvas.clientWidth * dpr;
canvas.height = canvas.clientHeight * dpr;
ctx.scale(dpr, dpr);
```

### 4.4 Performance Considerations

- Trail buffer: Store last 120 positions (2 seconds at 60fps) — circular buffer
- Clear and redraw each frame (simpler than incremental) — Canvas 2D is fast enough for this
- Avoid DOM manipulation during animation — all rendering in Canvas
- No heavy allocations in render loop
- Consider throttling to 30fps if CPU is a concern (unlikely at this complexity)

---

## 5. Speaker Layout Editor Architecture

### 5.1 Toggle View Pattern

Same canvas area switches between "Motion View" and "Speaker Editor" via toggle button:

```javascript
let editorMode = 'motion'; // or 'speaker'
function toggleView() {
    editorMode = editorMode === 'motion' ? 'speaker' : 'motion';
    // Show/hide editor controls (preset buttons, add/remove, text inputs)
}
```

### 5.2 Drag-to-Reposition

Canvas mouse events → coordinate mapping:

```javascript
canvas.addEventListener('mousedown', (e) => {
    const rect = canvas.getBoundingClientRect();
    const x = e.clientX - rect.left;
    const y = e.clientY - rect.top;
    // Convert (x, y) to (azimuth, elevation) based on polar mapping
    // Find nearest speaker, begin drag
});
```

**Coordinate mapping (top-down polar):**
- Center of canvas = (0°, 0° azimuth at center)
- Radius from center = distance (or fixed at 1.0, only azimuth matters)
- Angle from top = azimuth (counter-clockwise for left speakers)
- Canvas x = centerX + r * sin(azimuthRad)
- Canvas y = centerY - r * cos(azimuthRad)

### 5.3 Right-Click to Remove

```javascript
canvas.addEventListener('contextmenu', (e) => {
    e.preventDefault();
    // Find speaker under cursor, confirm removal
    // Call native removeSpeaker(index)
});
```

### 5.4 Preset Layout Buttons

Row of buttons above editor: "Stereo", "Quad", "5.1", "7.1", "5.1.4", "7.1.4", "Hex", "Oct"

Clicking sets the `speaker_layout` ComboBox state, which triggers APVTS update, which triggers VBAP recomputation.

---

## 6. Existing Module & Pattern Reuse

### 6.1 WebViewRelayManager Module

**Location:** `modules/core/webview-relay-manager/`

Could use `WebViewRelayManager` class for simplified relay lifecycle, but the manual pattern (used by O-GrainScatter with 30+ params) gives more control over native functions. **Recommendation: Use manual pattern** for O-Orbit since we need several native functions beyond parameter relays.

### 6.2 JUCE Bridge JS Files

Every WebView plugin includes:
- `js/juce/index.js` — JUCE bridge (PromiseHandler, getNativeFunction, getSliderState, getComboBoxState, getToggleState)
- `js/juce/check_native_interop.js` — Platform detection

These can be copied from any existing plugin (e.g., O-GrainScatter).

### 6.3 Rotary Knob JS Component

**Reference:** `plugins/O-SpectralShaper/Resources/ui/js/components/RotaryKnob.js`

Relative drag pattern:
- Track `lastY` on mousedown
- Delta = `lastY - e.clientY` per mousemove
- Sensitivity-based rotation mapping
- Double-click to reset to default

This can be adapted or re-implemented for O-Orbit's botanical seed knobs.

### 6.4 Botanical Overlay Pattern

Standard CSS pattern from aesthetic template:
```css
.botanical-overlay {
    position: absolute;
    right: -20px;
    top: 50%;
    transform: translateY(-50%);
    height: 71.25%;
    opacity: 0.35;
    pointer-events: none;
    z-index: 1;
}
```

O-Orbit uses "Shell (ocean)" image — a spiral shell/nautilus.

---

## 7. CMakeLists.txt Considerations

### 7.1 Current State

Already has:
- `NEEDS_WEB_BROWSER TRUE`
- `NEEDS_WEBVIEW2 TRUE`
- `JUCE_WEB_BROWSER=1`
- `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`
- Links `juce::juce_gui_extra`

### 7.2 BinaryData for UI Resources

Need to add `juce_add_binary_data()` for HTML/CSS/JS/image files:

```cmake
juce_add_binary_data(OuariconOrbit_WebUI
    SOURCES
        Resources/ui/index.html
        Resources/ui/css/styles.css
        Resources/ui/js/app.js
        Resources/ui/js/juce/index.js
        Resources/ui/js/juce/check_native_interop.js
        Resources/ui/img/shell.png
        Resources/ui/img/paper-bg.jpg
)

target_link_libraries(OuariconOrbit PRIVATE OuariconOrbit_WebUI)
```

**Alternative:** Use `JUCE_BINARY_RESOURCES` in `target_sources()` — but `juce_add_binary_data()` is the established pattern in this codebase.

**Important:** BinaryData target name must not conflict with the plugin target name.

---

## 8. UI Layout Specification (800x600)

### 8.1 Vertical Space Allocation

| Section | Height | Content |
|---------|--------|---------|
| Header | ~30px | Plugin title "O-ORBIT" + view toggle button |
| Visualizer | ~320px | Orbital visualizer / Speaker editor canvas |
| Controls | ~230px | 3 parameter groups in columns |
| Footer | ~20px | Downmix badge (if active) |

### 8.2 Parameter Control Layout (3 Columns)

**Motion (left, 8 params):**
- Row 1: Path (dropdown), Tempo Sync (dropdown)
- Row 2: Speed (knob), Width (knob), Depth (knob)
- Row 3: Tilt (knob), Phase (knob), Elevation (toggle) + Range (knob)

**Spatial (center, 5 params):**
- Row 1: Speaker Layout (dropdown)
- Row 2: Distance (knob), Air Absorption (knob)
- Row 3: Atten Curve (dropdown), Center Diverge (knob)

**Source (right, 3 params):**
- Row 1: Source Mode (dropdown)
- Row 2: L/R Offset (knob), Mix (knob)

### 8.3 Knob Specifications (from aesthetic template)

- **Size:** 55px diameter (compact tier for dense layouts)
- **Design:** Botanical seed cross-section
  - 10-segment conic gradient (alternating cream tones)
  - Dark brown radial divider lines
  - Central core circle (20% radius, lightest cream)
  - 2px walnut border
- **Indicator:** Small green dot or line at current position
- **Label:** Below knob, 9-10px uppercase Garamond
- **Value:** Below label, 10px regular case

---

## 9. Pitfalls & Known Issues

### 9.1 WebView on macOS Plugin Scanner

JUCE WebBrowserComponent can crash during AU validation (`auval`) if navigation is triggered before the editor is visible. **Solution:** Use lazy navigation pattern (parentHierarchyChanged).

### 9.2 Cross-Platform URL Schemes

- macOS/Linux: `juce://juce.backend/` (custom scheme)
- Windows: `https://juce.backend/` (intercepted by WebView2)

**Never hard-code** — use `juce::WebBrowserComponent::getResourceProviderRoot()` in C++ and `window.__JUCE__.backend.getResourceAddress()` in JS.

### 9.3 WebView Memory in DAW

Multiple instances of O-Orbit each create their own WebView. Keep JS memory footprint low:
- Don't allocate large typed arrays unnecessarily
- Clear trail buffers when plugin window is hidden
- Consider stopping animation when not visible

### 9.4 Timer Frequency

30Hz timer for motion state push is sufficient. Canvas interpolates at 60fps. Higher timer rates waste CPU on evaluateJavascript calls.

### 9.5 FileChooser Async Requirement

JUCE 8 requires `launchAsync()` for file dialogs. Store the `FileChooser` as a member to prevent premature destruction:
```cpp
std::shared_ptr<juce::FileChooser> fileChooser; // Member, not local
```

### 9.6 Speaker Layout State Persistence

Current implementation stores speaker layout as APVTS Choice parameter (preset index only). Custom layouts need separate persistence in `getStateInformation()` / `setStateInformation()` via ValueTree or XML child element.

---

## 10. Phased Implementation Summary

### Phase 3.1: WebView UI + Parameter Controls
**C++ work:**
- Rewrite PluginEditor.h/cpp with WebView setup
- 17 relays (11 slider + 5 combo + 1 toggle)
- Resource provider for BinaryData
- Lazy navigation

**Web work:**
- index.html with full layout (800x600)
- styles.css with naturalist aesthetic (paper bg, seed knobs, Garamond)
- app.js with parameter binding (getSliderState, getComboBoxState, getToggleState)
- JUCE bridge files (index.js, check_native_interop.js)
- Shell botanical overlay image
- Paper background texture

**CMake work:**
- Add BinaryData target for UI resources
- Link BinaryData to plugin target

### Phase 3.2: Orbital Visualizer
**C++ work:**
- Add atomic motion snapshot to processor
- Add Timer to editor for motion state push
- Add native function for speaker positions

**Web work:**
- Canvas-based orbital visualizer
- 60fps animation loop with requestAnimationFrame
- Source dot, path trails, speaker icons
- L+R split dual-dot display
- Smooth interpolation between state updates

### Phase 3.3: Speaker Layout Editor + File I/O
**C++ work:**
- Native functions: addSpeaker, removeSpeaker, moveSpeaker
- Native functions: saveSpeakerLayout, loadSpeakerLayout, exportLayout, importLayout
- FileChooser integration (async)
- Custom layout persistence in state

**Web work:**
- Toggle view (motion ↔ speaker editor)
- Drag-to-reposition canvas interaction
- Click-to-add, right-click-to-remove
- Text inputs for precise az/el/dist
- Preset buttons
- Save/Load/Import/Export buttons
- Downmix warning badge

---

## 11. Key Reference Files

| Reference | Path |
|-----------|------|
| Aesthetic template | `.claude/aesthetics/ouaricon-naturalist-001/aesthetic.md` |
| WebView relay manager | `modules/core/webview-relay-manager/cpp/WebViewRelayManager.h` |
| Complex editor (30 params) | `plugins/O-GrainScatter/Source/PluginEditor.h` |
| Canvas visualizer | `plugins/O-GrainScatter/Source/ui/public/js/app.js` |
| Rotary knob component | `plugins/O-SpectralShaper/Resources/ui/js/components/RotaryKnob.js` |
| Resource provider pattern | `plugins/O-SpectralShaper/Source/PluginEditor.cpp` |
| evaluateJavascript push | `plugins/O-Bells/Source/PluginEditor.cpp` |
| JUCE bridge JS | `plugins/O-Bells/Resources/ui/js/juce/index.js` |
| Botanical overlay CSS | `.claude/aesthetics/ouaricon-naturalist-001/aesthetic.md` |
| Lazy navigation template | `.claude/templates/code-snippets/webview/lazy-navigation.yaml` |
| Resource provider template | `.claude/templates/code-snippets/webview/resource-provider.yaml` |

---

**End of Research**
