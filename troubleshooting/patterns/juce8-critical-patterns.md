# JUCE 8.x Critical Patterns - REQUIRED READING

**Purpose:** Prevent repeat mistakes across all plugin implementations.

**When to read:** Before ANY code generation in Stages 2-5.

---

## 1. CMakeLists.txt - Header Generation (ALWAYS REQUIRED)

### ❌ WRONG (Will fail with "JuceHeader.h not found")
```cmake
target_link_libraries(MyPlugin
    PRIVATE
        juce::juce_audio_processors
)

target_compile_definitions(MyPlugin
    PUBLIC
        JUCE_VST3_CAN_REPLACE_VST2=0
)
```

### ✅ CORRECT
```cmake
target_link_libraries(MyPlugin
    PRIVATE
        juce::juce_audio_processors
)

# CRITICAL: Generate JuceHeader.h (JUCE 8 requirement)
juce_generate_juce_header(MyPlugin)

target_compile_definitions(MyPlugin
    PUBLIC
        JUCE_VST3_CAN_REPLACE_VST2=0
)
```

**Why:** JUCE 8.x does NOT ship a pre-built JuceHeader.h. The `juce_generate_juce_header()` function:
1. Scans linked JUCE modules
2. Auto-generates JuceHeader.h in build artifacts
3. Adds it to compiler include paths

**Placement:** MUST come after `target_link_libraries()`, BEFORE `target_compile_definitions()`

**Documented in:** `troubleshooting/build-failures/juceheader-not-found-cmake-JUCE-20251106.md`

---

## 2. Include Style - Prefer Module Headers

### ✅ PREFERRED (Modern JUCE 8)
```cpp
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
```

### ⚠️ ACCEPTABLE (But requires juce_generate_juce_header())
```cpp
#include <JuceHeader.h>
```

**Recommendation:** Use individual module headers. They're explicit, don't require CMake generation, and match JUCE 8 best practices.

---

## 3. WebView UI - Module Requirements

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

**Documented in:** Stage 5 (GUI integration)

---

## 4. Bus Configuration - Effects vs Instruments

### Effects (Audio In → Audio Out)
```cpp
AudioProcessor(BusesProperties()
    .withInput("Input", juce::AudioChannelSet::stereo(), true)
    .withOutput("Output", juce::AudioChannelSet::stereo(), true))
```

### Instruments (MIDI In → Audio Out)
```cpp
AudioProcessor(BusesProperties()
    .withOutput("Output", juce::AudioChannelSet::stereo(), true))
```

**Common mistake:** Adding input bus to instruments causes "missing input" errors in DAWs.

**Documented in:** `troubleshooting/runtime-issues/vst3-bus-config-effect-missing-input-Shake-20251106.md`

---

## 5. Threading - UI ↔ Audio Thread

### ❌ WRONG (Thread violation - will crash)
```cpp
// In PluginEditor (UI thread)
button.onClick = [this] {
    audioProcessor.processBlock(...);  // ILLEGAL
};
```

### ✅ CORRECT
```cpp
// In PluginEditor (UI thread) - use parameters or atomic flags
button.onClick = [this] {
    audioProcessor.getAPVTS().getParameter("trigger")->setValueNotifyingHost(1.0f);
};
```

**Rule:** NEVER call audio processing code from UI thread. Use AudioProcessorValueTreeState (APVTS) for communication.

**Documented in:** `troubleshooting/build-failures/ui-trigger-button-processblock-invocation-thread-violation-JUCE-20251106.md`

---

## 6. Rotary Sliders - Bounds vs setBounds

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

**Rule:** Sliders need proper parent component hierarchy for mouse events. Absolute positioning works in `resized()` but not during construction.

**Documented in:** `troubleshooting/api-usage/rotary-slider-absolute-positioning-interaction-TapeMachine-20251107.md`

---

## 7. WebView ↔ Parameter Binding

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

**Documented in:** `troubleshooting/api-usage/webview-parameter-not-updating-wrong-event-format-JUCE-20251108.md`

---

## 8. WebView Resource Provider - Explicit URL Mapping (ALWAYS REQUIRED)

### ❌ WRONG (Generic loop - breaks resource loading)
```cpp
std::optional<juce::WebBrowserComponent::Resource>
getResource(const juce::String& url)
{
    auto path = url.substring(1);  // Remove leading slash

    // Generic loop - hard to debug, easy to break
    for (int i = 0; i < BinaryData::namedResourceListSize; ++i) {
        if (path == BinaryData::namedResourceList[i]) {
            // ... conversion logic ...
        }
    }
    return std::nullopt;
}
```

**Problem:** BinaryData flattens paths (`js/juce/index.js` → `index_js`), but HTML requests `/js/juce/index.js`. Generic loop can't match.

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

**Why:**
- BinaryData converts paths to valid C++ identifiers (`index.js` → `index_js`)
- HTML/JS use original paths (`./js/juce/index.js`)
- Explicit mapping bridges this gap clearly
- Easy to debug (can log URL mismatches)

**Placement:** PluginEditor.cpp private method

**Documented in:** `troubleshooting/gui-issues/webview-frame-load-interrupted-TapeAge-20251111.md`

---

## 9. CMakeLists.txt - NEEDS_WEB_BROWSER for VST3 (ALWAYS REQUIRED)

### ❌ WRONG (VST3 won't appear in DAW)
```cmake
juce_add_plugin(MyPlugin
    COMPANY_NAME "YourCompany"
    PLUGIN_CODE Plug
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "My Plugin"
    # Missing NEEDS_WEB_BROWSER - VST3 will be built but won't load
)
```

### ✅ CORRECT
```cmake
juce_add_plugin(MyPlugin
    COMPANY_NAME "YourCompany"
    PLUGIN_CODE Plug
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "My Plugin"
    NEEDS_WEB_BROWSER TRUE  # REQUIRED for VST3 WebView support
)
```

**Why:** VST3 format requires explicit WebView flag even if AU works without it. Missing this flag causes:
- VST3 builds successfully
- Binary exists in build artifacts
- But plugin doesn't appear in DAW VST3 scanner
- Only AU format visible

**When:** ANY plugin using WebBrowserComponent for UI

**Documented in:** `troubleshooting/gui-issues/webview-frame-load-interrupted-TapeAge-20251111.md`

---

## 10. Testing GUI Changes - Always Install to System (CRITICAL WORKFLOW)

### ❌ WRONG (Tests stale cached builds)
```bash
# Build but DON'T install
./scripts/build-and-install.sh MyPlugin --no-install

# Test in DAW → loads OLD version from system folders
```

**Result:** DAW loads stale plugins from `~/Library/Audio/Plug-Ins/`, not fresh builds.

### ✅ CORRECT
```bash
# Build AND install to system folders
./scripts/build-and-install.sh MyPlugin

# Script automatically:
# 1. Builds fresh binaries
# 2. Removes old versions from system
# 3. Installs new versions
# 4. Clears DAW caches (Ableton DB, AU cache)
# 5. Verifies installation
```

**Then restart DAW** - Required for plugin rescan after cache clear.

**Why:**
- DAWs load plugins from system folders (`~/Library/Audio/Plug-Ins/`), NOT build directories
- Using `--no-install` means testing old builds while developing new code
- All "fixes" appear broken when you're testing stale binaries

**When:** ANY time you modify:
- PluginEditor code (UI changes)
- CMakeLists.txt (configuration)
- Resource files (HTML/CSS/JS)
- Parameter bindings

**Documented in:** `troubleshooting/gui-issues/webview-frame-load-interrupted-TapeAge-20251111.md`

---

## 11. WebView Member Initialization - Use std::unique_ptr (REQUIRED)

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
        *processorRef.parameters.getParameter("GAIN"), *gainRelay
    );

    addAndMakeVisible(*webView);
    webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());
}
```

**Why:**
- Ensures correct construction order
- Prevents release build crashes (90% of member order issues)
- Destruction order is automatic (reverse of declaration)
- Matches JUCE examples and GainKnob reference implementation

**When:** ALL WebView-based plugin editors

**Documented in:** `troubleshooting/gui-issues/webview-frame-load-interrupted-TapeAge-20251111.md`

---

## 12. WebSliderParameterAttachment - Three Parameters Required (JUCE 8)

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

**Why:**
- JUCE 8 changed WebSliderParameterAttachment constructor signature
- Old signature: `(parameter, relay)`
- New signature: `(parameter, relay, undoManager)`
- Missing third parameter causes **silent failure** - compiles but doesn't bind
- UI displays correctly but knobs don't respond to mouse input
- No compiler error or warning

**Symptoms:**
- WebView UI loads and displays properly
- Knobs visible but completely frozen (don't respond to drag)
- Parameters don't update when knobs are moved
- Audio processing works (parameters have default values)

**When:** ALL WebView-based plugins using WebSliderParameterAttachment

**Documented in:** Pattern discovered during TapeAge frozen knobs issue (2025-11-11)

---

## 13. check_native_interop.js - Required for WebView (CRITICAL)

### ❌ WRONG (Missing file - UI may freeze)
```cmake
# CMakeLists.txt
juce_add_binary_data(PluginName_UIResources
    SOURCES
        Source/ui/public/index.html
        Source/ui/public/js/juce/index.js
        # Missing: check_native_interop.js
)
```

### ✅ CORRECT
```cmake
# CMakeLists.txt
juce_add_binary_data(PluginName_UIResources
    SOURCES
        Source/ui/public/index.html
        Source/ui/public/js/juce/index.js
        Source/ui/public/js/juce/check_native_interop.js  # Required
)
```

**PluginEditor.cpp resource handler:**
```cpp
// Must serve check_native_interop.js
if (url == "/js/juce/check_native_interop.js") {
    return juce::WebBrowserComponent::Resource {
        makeVector(BinaryData::check_native_interop_js,
                  BinaryData::check_native_interop_jsSize),
        juce::String("text/javascript")
    };
}
```

**Why:**
- `check_native_interop.js` is JUCE's WebView initialization verification script
- Tests that native C++ ↔ JavaScript bridge is working
- Without it, WebView may appear to load but native integration fails silently
- Results in frozen UI, non-responsive controls, broken parameter binding

**Symptoms:**
- UI loads and displays correctly
- Knobs visible but don't respond to interaction
- Similar to missing nullptr, but root cause is missing initialization

**Source:** Copy from working plugin (e.g., GainKnob) or JUCE examples

**When:** ALL WebView-based plugins

**Documented in:** Pattern discovered during TapeAge frozen knobs issue (2025-11-11)

---

## 14. Changing PRODUCT_NAME - Manual Cleanup Required (CRITICAL)

### ❌ WRONG (Leaves orphaned plugins)
```bash
# Change PRODUCT_NAME in CMakeLists.txt
PRODUCT_NAME "TAPE AGE"  →  PRODUCT_NAME "TapeAge"

# Build and install
./scripts/build-and-install.sh PluginName

# Result: Both "TAPE AGE" and "TapeAge" exist in system folders
# DAW shows duplicate plugins, one broken
```

### ✅ CORRECT
```bash
# BEFORE changing PRODUCT_NAME, manually remove old versions:
rm -rf ~/Library/Audio/Plug-Ins/VST3/"TAPE AGE.vst3"
rm -rf ~/Library/Audio/Plug-Ins/Components/"TAPE AGE.component"

# THEN change PRODUCT_NAME in CMakeLists.txt
PRODUCT_NAME "TapeAge"

# THEN build and install
./scripts/build-and-install.sh PluginName

# Clear caches
killall -9 AudioComponentRegistrar
rm ~/Library/Preferences/Ableton/*/PluginDatabase.cfg
```

**Why:**
- Build script's "Phase 4: Remove Old Versions" searches for plugins matching the NEW product name
- When you change PRODUCT_NAME, old plugins have the OLD name, so they aren't found and removed
- DAW sees both versions, creating duplicate entries in plugin list
- Old version is broken (wrong Component ID, missing fixes), new version works

**Prevention:**
- Always manually remove old plugins BEFORE changing PRODUCT_NAME
- Or use `/uninstall [PluginName]` before changing name (searches by directory, not product name)
- After name change, verify only one version exists: `ls ~/Library/Audio/Plug-Ins/{VST3,Components}/ | grep -i PluginName`

**Common scenarios:**
- Removing spaces: "TAPE AGE" → "TapeAge"
- Fixing capitalization: "tapeage" → "TapeAge"
- Renaming plugin: "OldName" → "NewName"
- Fixing typos in product name

**When:** ANY time PRODUCT_NAME is modified in CMakeLists.txt

**Documented in:** Pattern discovered during TapeAge VST3/AU duplicate issue (2025-11-11)

---

## 15. WebView valueChangedEvent Callback - No Parameters Passed (CRITICAL)

### ❌ WRONG (Parameters undefined, knobs don't update)
```javascript
// JavaScript assumes callback receives value parameter
driveState.valueChangedEvent.addListener((newValue) => {
    // newValue is UNDEFINED! JUCE doesn't pass callback parameters
    updateKnobVisual(driveRotatable, newValue);  // angle = NaN, no rotation
});
```

**Result:** Knobs appear at 12 o'clock (0 degrees) or show `NaN` rotation. Parameters don't persist between sessions because visual updates fail.

### ✅ CORRECT
```javascript
// Call getNormalisedValue() INSIDE the callback to read from state
driveState.valueChangedEvent.addListener(() => {
    const value = driveState.getNormalisedValue();  // Read current value from state
    updateKnobVisual(driveRotatable, value);
});
```

**Why:**
- JUCE's WebView `valueChangedEvent` is a **notification event**, not a value-passing event
- The callback receives **no parameters** (or undefined if you declare them)
- Different from typical JavaScript event patterns (e.g., DOM events)
- The correct pattern: callback with no params → call `getNormalisedValue()` inside
- By the time the event fires, the state object has been updated by C++

**Event sequence:**
1. C++ calls `sendInitialUpdate()` on WebSliderParameterAttachment
2. JUCE WebView bridge updates JavaScript SliderState object
3. `valueChangedEvent` fires (no callback parameters)
4. JavaScript callback reads value via `getNormalisedValue()`
5. Visual update receives valid number (0-1 normalized)

**Common mistake:**
Assuming callback works like DOM events: `addEventListener('change', (event) => event.value)`

**Reference implementation:**
GainKnob example shows this pattern:
```javascript
this.paramState.valueChangedEvent.addListener(() => {
    this.render();  // render() internally calls getNormalisedValue()
});
```

**When:** ALL WebView-based plugins using parameter bindings

**Documented in:** `troubleshooting/gui-issues/webview-parameter-undefined-event-callback-TapeAge-20251111.md`

---

## 16. WebView Knob Interaction - Relative Drag (ALWAYS REQUIRED)

### ❌ WRONG (Absolute positioning - knob jumps to cursor)
```javascript
// Absolute positioning: knob tracks mouse Y-coordinate
knob.addEventListener('mousedown', (e) => {
    isDragging = true;
    startY = e.clientY;
    startRotation = getCurrentRotation();
});

document.addEventListener('mousemove', (e) => {
    if (!isDragging) return;

    const deltaY = startY - e.clientY;  // Total distance from START
    const newRotation = startRotation + deltaY;  // Proportional to cursor position
    setRotation(newRotation);
});
```

**Result:** Knob jumps to match cursor Y-position. Unnatural, imprecise interaction. User must move mouse to exact target position instead of dragging incrementally.

### ✅ CORRECT (Relative drag - industry standard)
```javascript
// Relative drag: knob increments based on frame-to-frame movement
let rotation = 0;
let lastY = 0;

knob.addEventListener('mousedown', (e) => {
    isDragging = true;
    lastY = e.clientY;  // Store CURRENT position, not start
});

document.addEventListener('mousemove', (e) => {
    if (!isDragging) return;

    const deltaY = lastY - e.clientY;  // Distance since LAST FRAME
    rotation += deltaY * 0.5;  // Increment rotation (sensitivity factor)
    rotation = Math.max(-135, Math.min(135, rotation));

    setRotation(rotation);
    lastY = e.clientY;  // Update for next frame
});
```

**Why:**
- **Frame-delta pattern:** Calculates movement since last frame, not since initial click
- **Incremental updates:** Adds to current rotation rather than replacing it
- **Natural feel:** Drag up = increase, drag down = decrease (proportional to drag distance)
- **Cursor position irrelevant:** Can drag infinitely, not bound to cursor location
- **Industry standard:** Pro Tools, Logic, Ableton, all professional VST/AU plugins use this

**Key differences:**
- `lastY` (previous frame) NOT `startY` (initial click)
- `rotation += deltaY` (increment) NOT `rotation = startRotation + deltaY` (replace)
- `lastY = e.clientY` (update tracking each frame)

**Common mistake:**
Using total distance from start point: `startY - e.clientY` creates absolute positioning where knob rotation directly maps to mouse Y-coordinate.

**When:** ALL WebView knob/rotary controls

**Documented in:** `troubleshooting/gui-issues/absolute-knob-drag-webview-20251111.md`

---

## 17. juce::dsp::Reverb API - Modern DSP Pipeline (ALWAYS REQUIRED)

### ❌ WRONG (API mismatch - will not compile)
```cpp
// Using old juce::Reverb API with juce::dsp::Reverb class
juce::dsp::Reverb reverb;

void prepareToPlay(double sampleRate, int samplesPerBlock) override {
    reverb.setSampleRate(sampleRate);  // No such method in juce::dsp::Reverb
    reverb.reset();
}

void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override {
    if (buffer.getNumChannels() == 1) {
        reverb.processMono(buffer.getWritePointer(0), buffer.getNumSamples());  // Wrong API
    } else if (buffer.getNumChannels() == 2) {
        reverb.processStereo(buffer.getWritePointer(0), buffer.getWritePointer(1), buffer.getNumSamples());  // Wrong API
    }
}
```

### ✅ CORRECT
```cpp
// Modern juce::dsp::Reverb with ProcessSpec and AudioBlock
juce::dsp::Reverb reverb;

void prepareToPlay(double sampleRate, int samplesPerBlock) override {
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32>(getTotalNumOutputChannels());

    reverb.prepare(spec);
    reverb.reset();
}

void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override {
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    reverb.process(context);  // Handles all channel configurations automatically
}
```

**Why:** JUCE has two different reverb classes with incompatible APIs:
- `juce::Reverb` (old, non-DSP module) - Uses `setSampleRate()`, `processMono()`, `processStereo()`
- `juce::dsp::Reverb` (modern DSP module) - Uses `prepare(spec)` and `process(context)`

**Key differences:**
- Modern DSP uses ProcessSpec (not individual setters) in `prepare()`
- Modern DSP uses AudioBlock/ProcessContext (not raw pointers) in `process()`
- Modern DSP handles mono/stereo/multi-channel automatically via AudioBlock
- Modern DSP integrates seamlessly with other juce::dsp components (DryWetMixer, etc.)

**When:** ANY plugin using `juce::dsp::Reverb` or other `juce::dsp::` components

**Pattern applies to all juce::dsp components:**
- `juce::dsp::Compressor`
- `juce::dsp::Limiter`
- `juce::dsp::Chorus`
- `juce::dsp::Phaser`
- `juce::dsp::Reverb`
- `juce::dsp::DelayLine`
- `juce::dsp::IIR::Filter`

**Documented in:** `troubleshooting/api-usage/juce-dsp-reverb-wrong-api-methods-FlutterVerb-20251111.md`

---

## Usage Instructions

### For Subagents (foundation-agent, shell-agent, dsp-agent, gui-agent)

Add to your prompt:

```markdown
## CRITICAL: Before Implementation

Read and internalize JUCE 8 critical patterns:
@troubleshooting/patterns/juce8-critical-patterns.md

These are non-negotiable requirements. Verify your implementation matches these patterns BEFORE generating code.
```

### For Human Developers

Review this file before:
- Creating new plugins
- Modifying CMakeLists.txt
- Adding WebView UI
- Debugging build/runtime issues

---

## References

## 18. macOS Plugin Code Signing & Cache Management (ALWAYS REQUIRED)

### ❌ WRONG (Will cause plugins to not load or show old code)
```bash
# Building and installing without proper signing/cache clearing
cmake --build build --target MyPlugin_VST3 MyPlugin_AU
cp -R build/.../MyPlugin.vst3 ~/Library/Audio/Plug-Ins/VST3/
cp -R build/.../MyPlugin.component ~/Library/Audio/Plug-Ins/Components/

# OR: Using --deep flag when re-signing (corrupts binary!)
codesign --force --deep --sign - ~/Library/Audio/Plug-Ins/VST3/MyPlugin.vst3

# Expecting DAW to see changes without clearing caches
# (Launch Ableton and wonder why plugin doesn't update)
```

### ✅ CORRECT
```bash
# Step 1: Build plugins
cmake --build build --target MyPlugin_VST3 MyPlugin_AU

# Step 2: Remove old versions
rm -rf ~/Library/Audio/Plug-Ins/VST3/MyPlugin.vst3
rm -rf ~/Library/Audio/Plug-Ins/Components/MyPlugin.component

# Step 3: Install fresh from build artifacts
cp -R build/.../MyPlugin.vst3 ~/Library/Audio/Plug-Ins/VST3/
cp -R build/.../MyPlugin.component ~/Library/Audio/Plug-Ins/Components/

# Step 4: Sign WITHOUT --deep flag (prevents corruption)
codesign --force --sign - ~/Library/Audio/Plug-Ins/VST3/MyPlugin.vst3
codesign --force --sign - ~/Library/Audio/Plug-Ins/Components/MyPlugin.component

# Step 5: Verify signatures are valid
codesign --verify --deep --strict ~/Library/Audio/Plug-Ins/VST3/MyPlugin.vst3
codesign --verify --deep --strict ~/Library/Audio/Plug-Ins/Components/MyPlugin.component

# Step 6: Nuclear cache clear
killall "Ableton Live 12 Suite" "Logic Pro" 2>/dev/null
rm ~/Library/Preferences/Ableton/Live*/PluginScanner.txt 2>/dev/null
rm -rf ~/Library/Caches/AudioUnitCache 2>/dev/null
killall -9 AudioComponentRegistrar 2>/dev/null

# Step 7: System restart (required for AudioComponentRegistrar)
sudo reboot
```

**Why:** macOS has multiple layers of plugin caching and validation:

1. **Code signing validation**: Invalid signatures cause "sealed resource is missing or invalid" error
2. **DAW plugin scanner cache**: Caches plugin info, doesn't reload on simple file change
3. **macOS Audio Unit cache**: System-level cache of AU components
4. **AudioComponentRegistrar**: Process that holds plugin registry, persists across kills

**Critical mistakes:**

- **Using `codesign --deep`**: Modifies nested bundle contents, changing binary hash and corrupting plugin
- **Not clearing caches**: DAW loads cached old version even with new binary
- **Not restarting**: AudioComponentRegistrar cache survives process kills

**Check DAW scanner logs for errors:**
```bash
# Ableton
tail -50 ~/Library/Preferences/Ableton/Live*/PluginScanner.txt | grep -i error

# Logic Pro
tail -50 ~/Library/Logs/AudioUnitHosting/Logic*.log | grep -i error
```

**Documented in:** `troubleshooting/validation-problems/vst3-invalid-signature-plugin-cache-FlutterVerb-20251111.md`

---

## 19. WebView Boolean Parameters - Use getToggleState (ALWAYS REQUIRED)

### ❌ WRONG (Works but semantically incorrect)
```javascript
// Using slider API for boolean parameter (FlutterVerb v1.0.1)
const modModeState = Juce.getSliderState("MOD_MODE");

modModeToggle.addEventListener("click", () => {
    const currentValue = modModeState.getValue();
    const newValue = currentValue < 0.5 ? 1.0 : 0.0;  // Float threshold
    modModeState.setValue(newValue);
});

modModeState.valueChangedEvent.addListener((value) => {
    updateToggleVisual(value >= 0.5);  // Float comparison
});
```

**Problem:** Boolean parameters ARE stored as 0.0/1.0 floats internally, so `getSliderState()` works but requires unnecessary float comparisons and is semantically wrong.

### ✅ CORRECT
```javascript
// Use getToggleState for boolean parameters (FlutterVerb v1.0.2)
const modModeState = Juce.getToggleState("MOD_MODE");

modModeToggle.addEventListener("click", () => {
    const currentValue = modModeState.getValue();
    modModeState.setValue(!currentValue);  // Clean boolean toggle
});

modModeState.valueChangedEvent.addListener(() => {
    updateToggleVisual(modModeState.getValue());  // Direct boolean
});
```

**Why:** JUCE WebView bridge provides TWO distinct APIs:
- `getSliderState()` - For continuous/ranged parameters (returns normalized float 0.0-1.0)
- `getToggleState()` - For boolean parameters (returns true/false)

**Benefits of correct API:**
1. **Type safety:** Works with booleans, not floats pretending to be booleans
2. **Cleaner code:** `!value` instead of `value < 0.5 ? 1.0 : 0.0`
3. **Semantic correctness:** API name matches intent
4. **No threshold ambiguity:** What if someone uses 0.3? With booleans, no confusion.

**Decision tree:**
```
Parameter type in C++?
├─ AudioParameterBool → getToggleState()
├─ AudioParameterFloat → getSliderState()
├─ AudioParameterInt → getSliderState()
└─ AudioParameterChoice → getSliderState()
```

**When:** ALL WebView toggle switches/boolean parameters

**Documented in:** `troubleshooting/gui-issues/toggle-switch-wrong-api-getsliderstate-flutterverb-20251111.md`

---

## 20. WebView VU Meters - requestAnimationFrame Loop (ALWAYS REQUIRED)

### ❌ WRONG (Updates but jerky, no ballistic motion)
```javascript
// Direct CSS update in event callback (FlutterVerb v1.0.1)
function updateVUMeter(dbLevel) {
    const normalizedLevel = mapDBToNormalized(dbLevel);
    const angle = -45 + (normalizedLevel * 90);

    // Instant jump - no smooth animation
    vuNeedle.style.transform = `rotate(${angle}deg)`;
}

Juce.addEventListener('VU_LEVEL', updateVUMeter);
```

**Problem:** Needle jumps instantly to target position. No smooth motion, no ballistic physics (fast attack/slow decay). Visually jarring and hard to read peaks.

### ✅ CORRECT
```javascript
// Separate current/target with RAF loop (FlutterVerb v1.0.2)
let currentNeedleAngle = -45;   // Visual state
let targetNeedleAngle = -45;    // Audio data

const ATTACK_SPEED = 0.4;   // Fast rise (responsive to peaks)
const DECAY_SPEED = 0.15;   // Slow fall (readable fallback)

// Event handler ONLY updates target
function updateVUMeter(dbLevel) {
    const normalizedLevel = mapDBToNormalized(dbLevel);
    targetNeedleAngle = -45 + (normalizedLevel * 90);
}

// Animation loop interpolates current → target
function animateVUMeter() {
    // Ballistic motion: fast attack, slow decay
    const speed = currentNeedleAngle < targetNeedleAngle ? ATTACK_SPEED : DECAY_SPEED;
    currentNeedleAngle += (targetNeedleAngle - currentNeedleAngle) * speed;

    // Update visual
    vuNeedle.style.transform = `rotate(${currentNeedleAngle}deg)`;

    // Update color based on level zones
    const normalized = (currentNeedleAngle + 45) / 90;
    if (normalized > 0.9) {
        vuNeedle.style.background = 'linear-gradient(180deg, #ff6666 0%, #ff4444 100%)';
    } else if (normalized > 0.75) {
        vuNeedle.style.background = 'linear-gradient(180deg, #ffcc66 0%, #ffaa44 100%)';
    } else {
        vuNeedle.style.background = 'linear-gradient(180deg, #d4a574 0%, #c49564 100%)';
    }

    requestAnimationFrame(animateVUMeter);
}

// Start loop once
Juce.addEventListener('VU_LEVEL', updateVUMeter);
animateVUMeter();
```

**Why:** Canvas/DOM animations require continuous redraw loops. Simply updating a value in event callbacks doesn't create smooth motion.

**Key architecture:**
1. **Separate data from view:** Target (audio) vs current (visual)
2. **Events only update target:** Decouples audio rate (~30-60Hz) from display rate (60fps)
3. **Animation loop interpolates:** Smoothly moves current toward target
4. **Ballistic speeds:** Different attack (0.4) vs decay (0.15) mimics analog VU meters
5. **requestAnimationFrame:** Browser-optimized ~60fps, auto-pauses when tab hidden

**Exponential interpolation formula:**
```javascript
current += (target - current) * speed
```
- Creates smooth easing (fast initially, slows near target)
- Never overshoots target
- Naturally converges without threshold checks

**Ballistic motion physics:**
- **Attack = 0.4:** Needle catches peaks quickly (responsive)
- **Decay = 0.15:** Needle falls slowly (easier to read, less jittery)
- **Industry standard:** Mimics physical VU meter with mass/spring/damper

**When:** ALL WebView animated meters, level displays, visualizations

**Speed tuning guide:**
- 0.1 = Very smooth, sluggish (too slow for peaks)
- 0.2 = Smooth decay (good for release)
- 0.4 = Responsive attack (good for peaks)
- 0.6 = Very fast (may feel twitchy)
- 1.0 = Instant (defeats purpose of smoothing)

**Pattern applies to:**
- VU meters
- Peak meters
- Spectral analyzers
- Waveform displays
- Any real-time audio visualization

**Documented in:** `troubleshooting/gui-issues/vu-meter-no-animation-loop-flutterverb-20251111.md`

---

## 21. WebView ES6 Module Loading - type="module" Required (CRITICAL)

### ❌ WRONG (Knobs freeze - getSliderState undefined)
```html
<!-- index.html - Missing type="module" -->
<script src="js/juce/index.js"></script>
<script>
    // JUCE parameter bindings
    const knobs = document.querySelectorAll('.knob');

    knobs.forEach(knob => {
        const paramId = knob.dataset.param;

        // ❌ getSliderState is undefined - ES6 exports not accessible
        let sliderState = window.__JUCE__.backend.getSliderState(paramId);
        // Returns null, early return, no handlers attached

        if (!sliderState) {
            console.error(`Failed to get slider state for ${paramId}`);
            return;  // Knobs frozen - no drag handlers attached
        }
    });
</script>
```

**Result:** Knobs display correctly but don't respond to drag. Console shows "Failed to get slider state" errors.

### ✅ CORRECT
```html
<!-- index.html - ES6 module with proper import -->
<script type="module" src="js/juce/index.js"></script>
<script type="module">
    import { getSliderState } from './js/juce/index.js';

    // JUCE parameter bindings
    const knobs = document.querySelectorAll('.knob');

    knobs.forEach(knob => {
        const paramId = knob.dataset.param;

        // ✅ Use imported function directly
        let sliderState = getSliderState(paramId);

        if (!sliderState) {
            console.error(`Failed to get slider state for ${paramId}`);
            return;
        }

        // Attach drag handlers, listeners (now executes successfully)
        sliderState.valueChangedEvent.addListener(() => {
            const value = sliderState.getNormalisedValue();
            updateKnob(value);
        });

        // Mouse drag handler
        knob.addEventListener('mousedown', e => {
            // ... drag implementation
        });
    });
</script>
```

**Why:**
- JUCE 8's `index.js` uses ES6 `export` syntax (exports `getSliderState`, `getToggleState`, etc.)
- ES6 modules REQUIRE `type="module"` attribute on script tags
- Without `type="module"`, browser ignores `export` statements → functions unavailable
- `getSliderState()` is NOT on `window.__JUCE__.backend` - it's an ES6 module export
- Must use `import { getSliderState }` to access the exported function

**Symptoms:**
- Knobs display correctly but don't rotate when dragged
- Click/depress visual feedback works (CSS `:active`, no JavaScript needed)
- DAW automation works but UI doesn't reflect changes
- Console error: `"Failed to get slider state for [paramId]"`
- No drag handlers or listeners attached (early return due to null check)

**When:** ALL WebView-based plugins using JUCE 8's official `index.js`

**Required changes:**
1. Add `type="module"` to external script tag
2. Add `type="module"` to inline script tag
3. Add `import { getSliderState } from './js/juce/index.js'` at top of inline script
4. Call `getSliderState(paramId)` directly (not via `window.__JUCE__.backend`)

**Also applies to:**
- `getToggleState()` for boolean parameters
- `getComboBoxState()` for dropdown parameters
- `getNativeFunction()` for custom C++ callbacks
- Any other functions exported from JUCE's `index.js`

**Documented in:** `troubleshooting/api-usage/webview-es6-module-loading-frozen-knobs-lushverb-20251111.md`

---

## 22. CMakeLists.txt - IS_SYNTH Flag for Instruments (ALWAYS REQUIRED)

### ❌ WRONG (No audio - MIDI not routed)
```cmake
# Missing IS_SYNTH flag - plugin treated as effect
juce_add_plugin(LushPad
    COMPANY_NAME "YourCompany"
    PLUGIN_MANUFACTURER_CODE Manu
    PLUGIN_CODE Lush
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "LushPad"
    NEEDS_WEB_BROWSER TRUE
)
```

**Result:** Plugin loads successfully but produces no audio. Appears in DAW effects browser instead of instruments. DAW doesn't route MIDI to it.

### ✅ CORRECT
```cmake
# IS_SYNTH TRUE declares plugin as instrument
juce_add_plugin(LushPad
    COMPANY_NAME "YourCompany"
    PLUGIN_MANUFACTURER_CODE Manu
    PLUGIN_CODE Lush
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "LushPad"
    IS_SYNTH TRUE           # REQUIRED for instruments
    NEEDS_MIDI_INPUT TRUE   # Explicit MIDI requirement
    NEEDS_WEB_BROWSER TRUE
)
```

**Why:** JUCE uses `IS_SYNTH TRUE` to generate plugin metadata that tells DAWs this is an instrument:
- Sets `JucePlugin_IsSynth=1` preprocessor define
- Sets VST3 category to "Instrument|Synth" in moduleinfo.json
- Sets AU type to `'aumu'` (Audio Unit Music Device)
- DAWs read this metadata and enable MIDI routing automatically
- Without it, plugin is categorized as effect (no MIDI routing)

**Plugin type decision:**
```
Does plugin CREATE audio from scratch (no audio input needed)?
├─ YES → IS_SYNTH TRUE + output-only BusesProperties
│         Examples: Synth, drum machine, noise generator
└─ NO  → IS_SYNTH FALSE + input+output BusesProperties
          Examples: Delay, reverb, compressor, EQ
```

**Critical:** `IS_SYNTH` flag must match BusesProperties configuration in PluginProcessor.cpp:

```cpp
// For synth (IS_SYNTH TRUE) - output-only bus
AudioProcessor(BusesProperties()
    .withOutput("Output", juce::AudioChannelSet::stereo(), true))

// For effect (IS_SYNTH FALSE) - input + output bus
AudioProcessor(BusesProperties()
    .withInput("Input", juce::AudioChannelSet::stereo(), true)
    .withOutput("Output", juce::AudioChannelSet::stereo(), true))
```

**Symptoms when missing:**
- Plugin loads without errors
- No audio output when playing MIDI notes
- Plugin shows in effects category, not instruments
- Code has correct synth implementation but receives no MIDI

**When:** ALL synth/instrument plugins at Stage 2 (Foundation) configuration

**Documented in:** `troubleshooting/runtime-issues/synth-no-audio-missing-is-synth-LushPad-20251113.md`

---

All patterns documented with full context in:
- `troubleshooting/build-failures/`
- `troubleshooting/runtime-issues/`
- `troubleshooting/api-usage/`
- `troubleshooting/gui-issues/`
- `troubleshooting/validation-problems/`

See individual issue files for:
- Full problem descriptions
- What didn't work
- Why the solution works
- Prevention strategies
