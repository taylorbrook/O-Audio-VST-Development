# Stage 3: GUI - Research

**Researched:** 2026-04-05
**Domain:** JUCE 8 WebView UI, Ouaricon Naturalist aesthetic, preset module integration
**Confidence:** HIGH

## Summary

O-Wind Stage 3 GUI builds the full WebView-based UI for a 900x600 flute synthesizer plugin. The existing PluginEditor already has 14 WebSliderRelays + WebSliderParameterAttachments wired, a resource provider serving index.html and tuning panel JS/CSS, and WinWebView2 configured. The work is: (1) replace the shell index.html with a full 3-tab naturalist UI, (2) integrate the shared preset module, (3) add tone hole toggle via WebToggleButtonRelay (confirmed available in JUCE 8), (4) add instrument preset selector via native function, (5) add botanical fern image, and (6) update CMakeLists.txt binary resources.

All patterns are well-established across O-Bells, O-Lyrica, and O-Formant. The slider binding JS pattern, tab switching, preset browser, and toggle patterns are directly reusable with minor adaptation. The tone hole toggle has a clean path via JUCE 8's WebToggleButtonRelay + WebToggleButtonParameterAttachment (used in O-Formant and O-Lyrica). The instrument preset selector is a simple native function dropdown (not APVTS-backed since it controls internal DSP coefficients, not user-facing parameters).

**Primary recommendation:** Follow O-Bells structure exactly -- single index.html with inline CSS and module-pattern JS, preset browser in header bar, tabs below, signal-flow parameter groups on Sound tab. Use WebToggleButtonRelay (not slider hack) for tone hole toggle. Use native function for instrument preset selector.

## User Constraints (from CONTEXT.md)

### Locked Decisions
- Visual aesthetic: Ouaricon Naturalist brand identity
- Window size: 900x600
- Tab structure: 3 tabs (Sound, Tuning, Effects)
- Preset system: Shared preset module (OuariconPresetManager + preset-manager.js)
- Botanical illustration: Fern (fern_naturalistsmisc1Geor_0089.png)
- Tone hole toggle: Visible UI toggle on Sound tab
- Phase merge: 5.1+5.2 combined into Phase 3.1 (Layout + Controls + Binding)
- Mockup step: Skipped
- Effects tab: Blank placeholder

### Phase Structure
| Phase | Goal |
|-------|------|
| 3.1 | Layout + Controls + Parameter Binding (14 params wired, preset module, 3 tabs, tone hole toggle) |
| 3.2 | Breath/jet visualization, register indicator, visual polish |

### Deferred (OUT OF SCOPE for 3.1)
- Breath/jet real-time visualization (Phase 3.2)
- Register indicator (Phase 3.2)
- Visual polish and animation refinement (Phase 3.2)
- Effects tab DSP (future stage)

## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| UI-01 | Signal-flow parameter layout with grouped controls | O-Bells section/param-row HTML pattern; 4 groups across top + Impossible Physics bottom |
| UI-02 | 3-tab interface (Sound, Tuning, Effects) | O-Bells tab-bar + tab-content CSS/JS pattern directly reusable |
| UI-03 | Instrument preset selector (preset module) | Shared OuariconPresetManager + native functions; factory presets from InstrumentPresets.h |
| UI-04 | Tone hole system toggle | WebToggleButtonRelay + WebToggleButtonParameterAttachment (JUCE 8 confirmed) |
| UI-05 | Tuning panel integration (shared module) | Already served by resource provider; needs TuningPanel JS initialization pattern from O-Bells |
| UI-06 | Breath/jet visualization | Phase 3.2 -- not researched |
| UI-07 | Register indicator | Phase 3.2 -- not researched |
| UI-08 | Visual polish | Phase 3.2 -- not researched |

---

## 1. Reference Plugin Analysis

### O-Bells (PRIMARY reference -- closest match)

**Architecture:** Single `index.html` file (~2177 lines) containing:
- All CSS in a `<style>` block (no external CSS file except tuning-panel.css)
- All HTML structure in `<body>`
- All JS in a `<script type="module">` block at end of body
- Imports from `/js/juce/index.js` for relay state management

**Layout structure:**
```
plugin-container (800x600, fixed)
  botanical-overlay (absolute positioned img, right side)
  header (50px, contains title + preset browser)
  tab-bar (40px, below header)
  tab-content (flexible, below tab-bar)
  footer (55px, at bottom -- keyboard + gain)
```

**Key patterns verified:**
- `.plugin-container` uses fixed pixel dimensions, not viewport units
- `html, body { height: 100%; }` -- percentage-based, not vh
- Tab switching: `data-tab` attribute on `.tab` elements, JS toggles `.active` class on both `.tab` and `.tab-content`
- Preset browser: In header bar, uses `Juce.getNativeFunction()` pattern
- Slider binding: `data-param` on `.slider` elements, JS calls `Juce.getSliderState(paramName)` to get state, then binds `setNormalisedValue()` on drag and `valueChangedEvent.addListener()` for automation
- Toggle pattern (for booleans): Uses `Juce.getSliderState()` with `getNormalisedValue() > 0.5` check (O-Bells used slider relays for bool params)
- Image: `<img src="img/snail.png" class="botanical-overlay">`, served as `image/png` MIME type

### O-Formant (Toggle reference)

**Toggle implementation with WebToggleButtonRelay (the proper way):**

C++ side:
```cpp
autoConsonantRelay = std::make_unique<juce::WebToggleButtonRelay>("autoConsonantToggle");
// ... in WebBrowserComponent options:
.withOptionsFrom(*autoConsonantRelay)
// ... attachment:
autoConsonantAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
    *apvts.getParameter("autoConsonant"), *autoConsonantRelay);
```

JS side (from O-Formant main.js):
```javascript
import { getToggleState } from './juce/index.js';

const autoConsonantState = getToggleState('autoConsonantToggle');

// Read value
autoConsonantState.getValue();  // returns boolean

// Set value
autoConsonantState.setValue(!autoConsonantState.getValue());

// Listen for changes
autoConsonantState.valueChangedEvent.addListener(() => {
    updateToggleVisual();
});
```

HTML pattern:
```html
<div class="toggle-control" id="myToggle">
    <div class="toggle-checkbox"></div>
    <span class="toggle-label">TONE HOLES</span>
</div>
```

### O-Lyrica (4-tab + toggle reference)

Confirms the WebToggleButtonRelay pattern works with multiple toggles. O-Lyrica has 5 toggle relays:
- `freeToggleRelay`, `scaleToggleRelay` (glissando modes)
- `chorusBypassRelay`, `delayBypassRelay`, `eqBypassRelay`, `reverbBypassRelay` (effect bypasses)

All use `WebToggleButtonRelay` + `WebToggleButtonParameterAttachment`.

---

## 2. Preset Module Integration

### Step-by-Step (from shared module + O-Bells reference)

**A. PluginProcessor changes:**

1. Add `#include "OuariconPresetManager.h"` to processor header
2. Add member: `OuariconPresetManager presetManager;`
3. Initialize in constructor: `presetManager(parameters, "O-Wind")`
4. Add accessor: `OuariconPresetManager& getPresetManager() { return presetManager; }`
5. Update `getStateInformation()`:
   ```cpp
   if (auto xml = presetManager.getStateAsXml())
       copyXmlToBinary(*xml, destData);
   ```
6. Update `setStateInformation()`:
   ```cpp
   if (auto xml = getXmlFromBinary(data, size))
       presetManager.setStateFromXml(xml.get());
   ```
7. Add factory preset initialization (in constructor, after APVTS):
   ```cpp
   presetManager.initializeFactoryPresets({
       {"Concert Flute", {{"breathPressure", 0.5f}, {"embouchure", 0.5f}, ...}, {}},
       {"Shakuhachi", {{"breathPressure", 0.4f}, ...}, {}},
       // ... 8 presets total
   });
   ```

**B. PluginEditor changes:**

Register native functions on WebBrowserComponent options chain:
- `getPresetList` -- returns flat array of all preset names
- `getCurrentPreset` -- returns current preset name string
- `loadPreset` -- loads by name, returns bool
- `savePreset` -- saves by name, returns bool
- `selectNextPreset` -- navigates to next, returns new name
- `selectPreviousPreset` -- navigates to prev, returns new name
- `savePresetWithDialog` -- opens native file dialog, returns name
- `loadPresetFromFile` -- opens native file dialog, returns name

Add `std::shared_ptr<juce::FileChooser> fileChooser;` member for async dialogs.

**C. JS side:**

Can use either:
1. **Shared module** (`preset-manager.js`) -- provides `PresetManager` class with button bindings
2. **Inline pattern** (like O-Bells) -- call `Juce.getNativeFunction()` directly

**Recommendation:** Use the O-Bells inline pattern for Phase 3.1. It's simpler, avoids an additional module import, and the preset browser is straightforward (prev/next arrows + name display + save/load buttons + dropdown).

**D. CMakeLists.txt:**

Add `OuariconPresetManager.h` to source files. The shared module uses `ouaricon_add_module(O-Wind preset-manager)` but this just copies the header -- manual include also works.

**E. Factory Presets:**

8 factory presets corresponding to InstrumentPresets.h entries. Each preset JSON stores:
- All 14 APVTS parameter values (curated per instrument type)
- The preset name matching the instrument
- Stored at `~/Library/O-Wind/Presets/Factory/*.json`

**NOTE:** `getPresetListWithCategories()` is NOT in the shared module -- it's an O-Bells-specific extension. O-Wind should use `getPresetList()` (flat list) since there's no category hierarchy needed for 8 factory presets.

---

## 3. Toggle Button Pattern (Tone Hole)

### JUCE 8 WebToggleButtonRelay -- CONFIRMED AVAILABLE

**Confidence:** HIGH -- verified by reading JUCE 8 source at `/Users/taylorbrook/JUCE/modules/`

JUCE 8 provides three relay types:
1. `WebSliderRelay` + `WebSliderParameterAttachment` (float/int params)
2. `WebToggleButtonRelay` + `WebToggleButtonParameterAttachment` (bool params)
3. `WebComboBoxRelay` + `WebComboBoxParameterAttachment` (choice params)

**Implementation for tone hole toggle:**

1. **PluginProcessor:** Add `AudioParameterBool` to APVTS:
   ```cpp
   layout.add(std::make_unique<juce::AudioParameterBool>(
       juce::ParameterID{"toneHoleEnabled", 1},
       "Tone Hole Enable",
       false  // default off
   ));
   ```

2. **PluginEditor.h:** Add relay + attachment:
   ```cpp
   std::unique_ptr<juce::WebToggleButtonRelay> toneHoleRelay;
   // ... (in attachments section)
   std::unique_ptr<juce::WebToggleButtonParameterAttachment> toneHoleAttachment;
   ```

3. **PluginEditor.cpp:** Create relay, register with WebView, create attachment:
   ```cpp
   toneHoleRelay = std::make_unique<juce::WebToggleButtonRelay>("toneHoleToggle");
   // In WebBrowserComponent options:
   .withOptionsFrom(*toneHoleRelay)
   // After WebView creation:
   toneHoleAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
       *apvts.getParameter("toneHoleEnabled"), *toneHoleRelay);
   ```

4. **JS:** Use `Juce.getToggleState("toneHoleToggle")`:
   ```javascript
   const toneHoleState = Juce.getToggleState("toneHoleToggle");

   // Initialize visual
   updateToneHoleVisual(toneHoleState.getValue());

   // Click handler
   toggleEl.addEventListener('click', () => {
       toneHoleState.setValue(!toneHoleState.getValue());
   });

   // Automation listener
   toneHoleState.valueChangedEvent.addListener(() => {
       updateToneHoleVisual(toneHoleState.getValue());
   });
   ```

**JS API (from JUCE 8 index.js):**
- `getToggleState(name)` returns a `ToggleState` object
- `ToggleState.getValue()` returns boolean
- `ToggleState.setValue(bool)` sends to C++
- `ToggleState.valueChangedEvent.addListener(callback)` listens for C++ changes

**IMPORTANT:** The relay name in C++ (`"toneHoleToggle"`) must EXACTLY match the name passed to `getToggleState()` in JS.

**DSP Integration:** PluginProcessor must read the parameter in processBlock:
```cpp
bool toneHoleEnabled = apvts.getRawParameterValue("toneHoleEnabled")->load() > 0.5f;
```

---

## 4. Instrument Preset Selector Pattern

### Architecture

The 8 instrument presets (InstrumentPresets.h) configure internal DSP coefficients -- they are NOT APVTS parameters. The `currentPresetIndex` is already an `std::atomic<int>` on the processor. This needs:

1. **Native function approach** (not relay) since it's not an APVTS param:
   ```cpp
   .withNativeFunction("getInstrumentPresets", [this](auto, auto complete) {
       auto* arr = new juce::Array<juce::var>();
       for (int i = 0; i < InstrumentPresets::numTotalPresets; ++i)
           arr->add(juce::String(InstrumentPresets::allPresets[i].name));
       complete(juce::var(*arr));
       delete arr;
   })

   .withNativeFunction("setInstrumentPreset", [this](const auto& args, auto complete) {
       if (args.size() >= 1) {
           int idx = static_cast<int>(args[0]);
           processorRef.currentPresetIndex.store(idx);
           complete(true);
       } else {
           complete(false);
       }
   })

   .withNativeFunction("getInstrumentPreset", [this](auto, auto complete) {
       complete(processorRef.currentPresetIndex.load());
   })
   ```

2. **JS:** Build a dropdown/selector UI:
   ```javascript
   const getPresets = Juce.getNativeFunction('getInstrumentPresets');
   const setPreset = Juce.getNativeFunction('setInstrumentPreset');
   const getPreset = Juce.getNativeFunction('getInstrumentPreset');

   // Initialize
   const presets = await getPresets();
   const currentIdx = await getPreset();
   // Populate dropdown, highlight current
   ```

3. **UI placement:** On Sound tab, likely as a prominent selector near the top or integrated with the parameter groups. Could be a styled `<select>` or custom dropdown matching naturalist aesthetic.

**NOTE:** Instrument preset selection should also be saveable in the preset module's factory presets. When a factory preset is loaded, it should set both APVTS params AND the instrument preset index. This requires either:
- Adding instrument preset index as an APVTS param (cleanest for preset persistence)
- Using the preset manager's custom state callback

**Recommendation:** Add `instrumentPreset` as an `AudioParameterInt` (range 0-7) to APVTS. This lets it survive preset save/load automatically and can use WebComboBoxRelay or WebSliderRelay for automation.

---

## 5. Visual Aesthetic Implementation

### Color System (CSS Variables)
```css
:root {
    --bg-paper: #F5E6D3;
    --bg-paper-mid: #EBD9C7;
    --bg-accent: #D4C4B0;
    --brown-border: #8B7355;
    --brown-frame: #5C4033;
    --brown-text: #3C2F2F;
    --brown-text-secondary: #5C4033;
    --green-light: #8BA870;
    --green-mid: #6B8E4E;
    --green-dark: #3C5C1A;
    --green-darkest: #2C3E10;
    --btn-default: rgba(139, 168, 112, 0.3);
    --btn-active: rgba(107, 142, 35, 0.6);
}
```

### Typography
- Font: `'Garamond', 'Times New Roman', serif`
- Title: 22px, weight 300, letter-spacing 2px
- Section labels: 11px, uppercase, letter-spacing 1px
- Param labels: 10px, uppercase, letter-spacing 0.8px
- Values: 9px, normal case

### Slider Control HTML Pattern (from O-Bells)
```html
<div class="param-control">
    <div class="param-label">Breath Pressure</div>
    <div class="slider" data-param="breathPressure">
        <div class="slider-thumb"></div>
    </div>
    <div class="param-value" data-value="breathPressure">50%</div>
</div>
```

### Slider JS Binding Pattern (from O-Bells)
```javascript
import * as Juce from './js/juce/index.js';

const parameterStates = new Map();

document.querySelectorAll('.slider[data-param]').forEach(slider => {
    const paramName = slider.dataset.param;
    const thumb = slider.querySelector('.slider-thumb');
    const valueDisplay = document.querySelector(`[data-value="${paramName}"]`);
    const state = Juce.getSliderState(paramName);
    parameterStates.set(paramName, state);

    // Initialize position
    function updateVisual(normValue) {
        thumb.style.left = `calc(${normValue * 100}% - 7px)`;
        if (valueDisplay) valueDisplay.textContent = `${Math.round(normValue * 100)}%`;
    }
    updateVisual(state.getNormalisedValue());

    // UI -> JUCE (drag)
    let dragging = false;
    slider.addEventListener('mousedown', (e) => { dragging = true; updateFromMouse(e); });
    document.addEventListener('mousemove', (e) => { if (dragging) updateFromMouse(e); });
    document.addEventListener('mouseup', () => dragging = false);

    function updateFromMouse(e) {
        const rect = slider.getBoundingClientRect();
        const x = Math.max(0, Math.min(rect.width, e.clientX - rect.left));
        const value = x / rect.width;
        state.setNormalisedValue(value);
        updateVisual(value);
    }

    // JUCE -> UI (automation/preset)
    state.valueChangedEvent.addListener(() => {
        updateVisual(state.getNormalisedValue());
    });
});
```

### Parameter Section Group Pattern
```html
<div class="section">
    <div class="section-title">Excitation</div>
    <div class="param-row">
        <!-- 3 param-controls in a flex row -->
    </div>
</div>
```

### Toggle CSS Pattern (from O-Bells/O-Formant)
```css
.toggle-control {
    display: flex;
    align-items: center;
    gap: 8px;
    padding: 6px 10px;
    background: rgba(139, 168, 112, 0.15);
    border-radius: 4px;
    cursor: pointer;
}
.toggle-control:hover { background: rgba(139, 168, 112, 0.25); }
.toggle-control .toggle-checkbox {
    width: 14px; height: 14px;
    border: 2px solid #3C5C1A;
    border-radius: 3px;
    background: #E8D5B7;
    display: flex; align-items: center; justify-content: center;
    font-size: 10px; color: #3C5C1A;
}
.toggle-control.active .toggle-checkbox { background: rgba(107, 142, 35, 0.6); }
.toggle-control .toggle-label {
    font-size: 10px; font-weight: 400;
    letter-spacing: 0.8px; text-transform: uppercase; color: #3C2F2F;
}
```

### Botanical Overlay
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

---

## 6. Resource Provider Patterns

### Current O-Wind Resource Provider

Already serves:
- `/` and `/index.html` -> `text/html`
- `/js/juce/index.js` -> `text/javascript`
- `/js/juce/check_native_interop.js` -> `text/javascript`
- `/js/tuning-panel.js` -> `text/javascript`
- `/css/tuning-panel.css` -> `text/css`

### Additions Needed

| URL Path | BinaryData Name | MIME Type |
|----------|----------------|-----------|
| `/img/fern.png` | `fern_naturalistsmisc1Geor_0089_png` | `image/png` |
| `/modules/preset-manager.js` | `presetmanager_js` | `text/javascript` |

**NOTE on BinaryData naming:** JUCE's `juce_add_binary_data` converts filenames to C++ identifiers by replacing non-alphanumeric characters with underscores. The exact name depends on the file path. For example:
- `fern_naturalistsmisc1Geor_0089.png` -> `fern_naturalistsmisc1Geor_0089_png` / `fern_naturalistsmisc1Geor_0089_pngSize`

**Image serving pattern (from O-Bells):**
```cpp
if (url == "/img/fern.png") {
    return juce::WebBrowserComponent::Resource {
        std::vector<std::byte>(
            reinterpret_cast<const std::byte*>(BinaryData::fern_naturalistsmisc1Geor_0089_png),
            reinterpret_cast<const std::byte*>(BinaryData::fern_naturalistsmisc1Geor_0089_png)
                + BinaryData::fern_naturalistsmisc1Geor_0089_pngSize),
        juce::String("image/png")
    };
}
```

### CMakeLists.txt Binary Resources Update

Current:
```cmake
juce_add_binary_data(O-Wind_UIResources
    SOURCES
        Resources/ui/index.html
        Resources/ui/js/juce/index.js
        Resources/ui/js/juce/check_native_interop.js
        ${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/js/tuning-panel.js
        ${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/snippets/tuning-panel.css
)
```

Add:
```cmake
        Resources/ui/img/fern_naturalistsmisc1Geor_0089.png
```

Optionally add `preset-manager.js` if using the shared module (but inline preset browser is recommended for simplicity).

---

## 7. Tab Implementation

### HTML Pattern (from O-Bells)
```html
<!-- Tab Bar -->
<div class="tab-bar">
    <div class="tab active" data-tab="sound">Sound</div>
    <div class="tab" data-tab="tuning">Tuning</div>
    <div class="tab" data-tab="effects">Effects</div>
</div>

<!-- Tab Contents -->
<div class="tab-content active" id="sound-tab">
    <!-- Sound tab content -->
</div>
<div class="tab-content" id="tuning-tab">
    <div id="tuning-container"></div>
</div>
<div class="tab-content" id="effects-tab">
    <div class="placeholder">Coming Soon</div>
</div>
```

### CSS Pattern (from O-Bells)
```css
.tab-bar {
    position: absolute;
    top: 50px;  /* below header */
    left: 0;
    width: 100%;
    height: 40px;
    display: flex;
    background: #D4C4B0;
    border-bottom: 2px solid #8B7355;
    z-index: 2;
}

.tab {
    flex: 1;
    display: flex;
    justify-content: center;
    align-items: center;
    background: #D4C4B0;
    color: #5C4033;
    font-size: 13px;
    font-weight: 400;
    letter-spacing: 1.5px;
    text-transform: uppercase;
    cursor: pointer;
    border-right: 1px solid #8B7355;
    transition: background 0.2s, color 0.2s;
}

.tab:last-child { border-right: none; }
.tab:hover { background: #C9B69F; }
.tab.active {
    background: #F5E6D3;
    color: #3C2F2F;
    border-bottom: 2px solid #F5E6D3;
    margin-bottom: -2px;
}

.tab-content {
    position: absolute;
    top: 90px;  /* header + tab bar */
    left: 0;
    width: 100%;
    height: calc(100% - 90px);
    display: none;
    padding: 14px 30px;
    z-index: 0;
    overflow-y: auto;
}

.tab-content.active { display: block; }
```

### JS Tab Switching (from O-Bells)
```javascript
const tabs = document.querySelectorAll('.tab');
const tabContents = document.querySelectorAll('.tab-content');
const botanicalOverlay = document.getElementById('botanicalOverlay');

tabs.forEach(tab => {
    tab.addEventListener('click', () => {
        const targetTab = tab.dataset.tab;

        // Update active tab
        tabs.forEach(t => t.classList.remove('active'));
        tab.classList.add('active');

        // Update active content
        tabContents.forEach(content => content.classList.remove('active'));
        document.getElementById(`${targetTab}-tab`).classList.add('active');

        // Optional: adjust botanical overlay position for tuning tab
        if (targetTab === 'tuning') {
            botanicalOverlay.classList.add('tuning-position');
        } else {
            botanicalOverlay.classList.remove('tuning-position');
        }
    });
});
```

### Tuning Panel Initialization (from O-Bells)
```javascript
(async () => {
    const { TuningPanel } = await import('./js/tuning-panel.js');
    const container = document.getElementById('tuning-container');
    const tuningPanel = new TuningPanel(container, Juce);
    await tuningPanel.init();

    // Expose note highlighting for C++ evaluateJavascript calls
    window.tuningNoteOn = (midiNote) => tuningPanel.noteOn(midiNote);
    window.tuningNoteOff = (midiNote) => tuningPanel.noteOff(midiNote);
})();
```

---

## 8. Pitfalls and Gotchas

### Pitfall 1: Resource Provider Receives Bare Paths
**What goes wrong:** Code strips scheme/host from URL, but resource provider already receives bare path (e.g., `/`, `/index.html`, `/img/fern.png`)
**How to avoid:** Compare directly: `if (url == "/img/fern.png")`. Never use `fromFirstOccurrenceOf("://")`.
**Confidence:** HIGH -- documented in project MEMORY.md

### Pitfall 2: No Viewport Units in CSS
**What goes wrong:** `100vh` does not work in JUCE WebView -- results in incorrect sizing
**How to avoid:** Use `100%` on `html, body`. Use `calc()` with pixel offsets for content areas.
**Confidence:** HIGH -- documented in O-Bells CSS comment

### Pitfall 3: Relay/Attachment Destruction Order
**What goes wrong:** If WebView is destroyed before attachments, `evaluateJavascript()` called during attachment destruction crashes.
**How to avoid:** Member declaration order MUST be: Relays -> WebView -> Attachments (destroyed in reverse). Already correct in O-Wind's PluginEditor.h.
**Confidence:** HIGH -- documented in all plugin editors

### Pitfall 4: Preset Module File I/O During AU Validation
**What goes wrong:** AU validation runs the processor constructor but doesn't expect file I/O to happen. Creating preset directories during construction can cause AU validation failure.
**How to avoid:** OuariconPresetManager v1.5.0 defers directory creation to first use (lazy initialization). Verified in source.
**Confidence:** HIGH -- verified in OuariconPresetManager.h

### Pitfall 5: BinaryData Identifier Names
**What goes wrong:** The BinaryData identifier for `fern_naturalistsmisc1Geor_0089.png` may not be what you expect due to JUCE's filename-to-identifier conversion.
**How to avoid:** After updating CMakeLists.txt and rebuilding, check the generated `BinaryData.h` for the exact identifier name. Build will fail at compile time if the name is wrong.
**Confidence:** HIGH

### Pitfall 6: Preset Dropdown z-index Stacking Context
**What goes wrong:** Preset dropdown appears behind tab content or parameter controls due to CSS stacking context.
**How to avoid:** Use `z-index: 9999` on dropdown, and elevate parent container:
```css
.header:has(.preset-dropdown.visible) { z-index: 9999; }
```
**Confidence:** HIGH -- documented in shared module README

### Pitfall 7: Windows WebView2 User Data Folder
**What goes wrong:** WebView2 denied access to default user data location in DAW hosts.
**How to avoid:** Already configured in O-Wind's PluginEditor.cpp -- `withUserDataFolder()` set to temp directory.
**Confidence:** HIGH -- already handled

### Pitfall 8: FileChooser Must Be Shared Pointer
**What goes wrong:** FileChooser goes out of scope before async callback completes, causing crash.
**How to avoid:** Store as `std::shared_ptr<juce::FileChooser>` on the editor class. Capture by value in lambda.
**Confidence:** HIGH -- pattern from O-Bells

---

## 9. Module Dependencies

### Shared Modules to Import
| Module | Path | What It Provides |
|--------|------|-----------------|
| Preset Manager | `modules/persistence/preset-manager/` | `OuariconPresetManager.h` (C++), `preset-manager.js` (JS) |
| Tuning Panel | `modules/tuning/scala-tuning-engine/` | Already integrated -- `tuning-panel.js`, `tuning-panel.css` |
| JUCE Bridge | `Resources/ui/js/juce/index.js` | Already present -- `getSliderState`, `getToggleState`, `getNativeFunction`, `getComboBoxState` |

### Files to Create (New)
| File | Purpose |
|------|---------|
| `Resources/ui/index.html` | Full UI (replace existing shell) |
| `Resources/ui/img/fern_naturalistsmisc1Geor_0089.png` | Botanical overlay image |

### Files to Modify
| File | Changes |
|------|---------|
| `Source/PluginProcessor.h` | Add `OuariconPresetManager` member, `getPresetManager()` accessor, `AudioParameterBool` for tone hole |
| `Source/PluginProcessor.cpp` | Add preset manager init, factory presets, state save/load with preset manager, tone hole parameter, instrument preset APVTS param |
| `Source/PluginEditor.h` | Add `WebToggleButtonRelay` + attachment for tone hole, `FileChooser` member |
| `Source/PluginEditor.cpp` | Add preset native functions, instrument preset native functions, toggle relay/attachment, update resource provider |
| `CMakeLists.txt` | Add fern image to binary resources, add OuariconPresetManager.h to sources |
| `Source/FluteSynthVoice.h/cpp` | Read tone hole enabled param |

---

## 10. Estimated Complexity

### File Count
- **New files:** 2 (index.html, fern.png copied)
- **Modified files:** 6 (Processor .h/.cpp, Editor .h/.cpp, CMakeLists.txt, FluteSynthVoice)
- **Total touched:** 8 files

### Lines of Code Estimates
| File | Estimated Lines |
|------|----------------|
| `index.html` (CSS + HTML + JS) | ~1200-1500 lines |
| `PluginProcessor.h` changes | ~15 lines added |
| `PluginProcessor.cpp` changes | ~80 lines added (factory presets, state, tone hole param) |
| `PluginEditor.h` changes | ~10 lines added |
| `PluginEditor.cpp` changes | ~120 lines added (native functions, relay, resource routes) |
| `CMakeLists.txt` changes | ~5 lines added |
| `FluteSynthVoice` changes | ~5 lines (read tone hole param) |
| **Total new/modified** | **~1400-1700 lines** |

### Complexity Assessment
- **Straightforward:** All patterns are proven across 3+ reference plugins
- **Highest risk area:** index.html layout -- getting 14 knobs + toggle + instrument selector to fit cleanly in 900x600 with the naturalist aesthetic
- **No novel engineering required:** Every component has a working reference implementation

---

## Sources

### Primary (HIGH confidence)
- O-Bells `PluginEditor.h/cpp` -- full preset + tab + slider + toggle pattern
- O-Bells `index.html` -- 2177 lines of working naturalist UI
- O-Formant `PluginEditor.cpp` -- WebToggleButtonRelay + WebToggleButtonParameterAttachment usage
- O-Lyrica `PluginEditor.h` -- multiple WebToggleButtonRelay instances
- O-Wind `PluginEditor.h/cpp` -- existing relay/attachment/resource provider setup
- O-Wind `PluginProcessor.h` -- existing `currentPresetIndex` atomic, `getTuningEngine()`
- O-Wind `InstrumentPresets.h` -- 8 instrument preset definitions
- JUCE 8 source `/modules/juce_gui_extra/misc/juce_WebControlRelays.h` -- WebToggleButtonRelay API
- JUCE 8 source `/modules/juce_gui_extra/native/javascript/index.js` -- ToggleState JS API
- Shared module `modules/persistence/preset-manager/` -- README, module.yaml, OuariconPresetManager.h, preset-manager.js
- Aesthetic doc `.claude/aesthetics/ouaricon-naturalist-001/aesthetic.md` -- full visual specification

### Secondary (MEDIUM confidence)
- O-Wind `CMakeLists.txt` -- current binary resource list
- Project `CLAUDE.md` -- build requirements, cache clearing protocol

## Metadata

**Confidence breakdown:**
- Reference patterns: HIGH -- read actual source code from 4 working plugins
- Toggle implementation: HIGH -- verified JUCE 8 source + 2 working examples
- Preset module: HIGH -- read shared module source + O-Bells integration
- Visual aesthetic: HIGH -- read full aesthetic.md specification
- Instrument selector: MEDIUM -- novel UI element but straightforward native function pattern

**Research date:** 2026-04-05
**Valid until:** 2026-05-05 (stable -- JUCE 8 API, established project patterns)
