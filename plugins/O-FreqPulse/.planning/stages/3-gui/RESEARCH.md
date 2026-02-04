# Stage 3: GUI Implementation - Research

**Plugin:** O-FreqPulse
**Stage:** 3 (GUI)
**Phase:** RESEARCH
**Date:** 2026-02-03

---

## 1. Research Scope

This document investigates implementation approaches for O-FreqPulse's WebView GUI, which includes:
- 2D step grid (4 bands × 32 steps = 128 cells)
- Real-time playhead synchronization
- Per-band Euclidean controls (expandable panels)
- Global controls footer (Mix, Rate, Swing, Smoothing, Steps)
- Naturalist aesthetic (O-Detune style)

---

## 2. JUCE WebView Architecture (Proven Patterns)

### 2.1 WebBrowserComponent Setup

From `juce8-critical-patterns.md` and O-Bells implementation:

**Critical Pattern #11 - Use `std::unique_ptr` for member ordering:**
```cpp
class OFreqPulseAudioProcessorEditor : public juce::AudioProcessorEditor {
private:
    // Order matters: Relays → WebView → Attachments

    // 1. Relays (created FIRST)
    std::unique_ptr<juce::WebSliderRelay> mixRelay;
    std::unique_ptr<juce::WebComboBoxRelay> stepsRelay;
    std::unique_ptr<juce::WebComboBoxRelay> rateRelay;
    // ... etc

    // 2. WebView (created with relay options)
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // 3. Attachments (created LAST)
    std::unique_ptr<juce::WebSliderParameterAttachment> mixAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> stepsAttachment;
    // ... etc
};
```

**Critical Pattern #8 - Explicit URL Mapping (not generic loops):**
```cpp
std::optional<juce::WebBrowserComponent::Resource>
getResource(const juce::String& url) {
    if (url == "/" || url == "/index.html")
        return makeResource(BinaryData::index_html, "text/html");
    if (url == "/js/juce/index.js")
        return makeResource(BinaryData::index_js, "text/javascript");
    if (url == "/js/juce/check_native_interop.js")
        return makeResource(BinaryData::check_native_interop_js, "text/javascript");
    if (url == "/css/styles.css")
        return makeResource(BinaryData::styles_css, "text/css");
    return std::nullopt;
}
```

### 2.2 Parameter Relay Types

| Parameter Type | Relay Class | Attachment Class | JS API |
|----------------|-------------|------------------|--------|
| Float (continuous) | `WebSliderRelay` | `WebSliderParameterAttachment` | `getSliderState()` |
| Choice (enum) | `WebComboBoxRelay` | `WebComboBoxParameterAttachment` | `getComboBoxState()` |
| Boolean | `WebToggleRelay` | `WebToggleButtonParameterAttachment` | `getToggleState()` |

**O-FreqPulse parameter mapping:**
- Global sliders: `mix`, `swing`, `smoothing` → `WebSliderRelay`
- Global choices: `steps`, `rate` → `WebComboBoxRelay`
- Per-band enables: `band0_enable` ... `band3_enable` → `WebToggleRelay`
- Per-band sliders: `band0_depth`, `band0_euc_steps`, etc. → `WebSliderRelay`
- Per-band choices: `band0_mode`, etc. (Manual/Euclidean) → `WebComboBoxRelay`
- **Step grid (128 params):** `step_b0_s0` ... `step_b3_s31` → `WebToggleRelay`

### 2.3 Native Functions for Custom Communication

For playhead sync and bulk grid updates, use `withNativeFunction()`:

```cpp
.withNativeFunction("getCurrentStep", [this](const auto& args, auto complete) {
    // Return current step from processor (atomic read)
    complete(juce::var(processorRef.getCurrentStep()));
})

.withNativeFunction("setStepPattern", [this](const auto& args, auto complete) {
    // Accept JSON array for bulk grid update
    if (args.size() >= 2) {
        int bandIndex = static_cast<int>(args[0]);
        auto patternStr = args[1].toString();
        // Parse and update processor
    }
    complete({});
})
```

---

## 3. Step Grid Implementation

### 3.1 Parameter Strategy: Individual vs Bulk

**Option A: Individual Toggle Parameters (128 params)**
- Pros:
  - Standard APVTS pattern
  - DAW automation per-step
  - State save/restore automatic
- Cons:
  - 128 WebToggleRelay objects
  - Heavy initial sync

**Option B: Native Function Bulk Transfer**
- Pros:
  - Single message for all 128 cells
  - Faster grid refresh
- Cons:
  - Custom serialization
  - No per-step DAW automation

**Recommendation:** Hybrid approach
- Use individual bool parameters for the 128 steps (already in APVTS)
- Use native functions for bulk reads (`getGridState`) and playhead sync
- Grid click → individual parameter update via `getToggleState()`

### 3.2 Grid Rendering (JavaScript)

**Structure:**
```html
<div class="grid-container">
    <!-- Band rows (bottom to top for frequency visualization) -->
    <div class="band-row" data-band="3"> <!-- HIGH -->
        <div class="band-label">HIGH<br>4kHz-20kHz</div>
        <div class="steps-container">
            <!-- 32 step cells -->
            <div class="step-cell" data-band="3" data-step="0"></div>
            ...
        </div>
        <div class="band-mode">Manual</div>
        <div class="band-expand-btn">▶</div>
    </div>
    <!-- ... more bands ... -->
    <div class="playhead"></div>
</div>
```

**CSS Considerations (from CONTEXT.md):**
- Grid width: ~700px for 32 steps = ~22px per cell
- Grid height: ~200px for 4 bands = ~50px per band row
- Step on (full gain): `opacity: 1.0`, accent green
- Step on (partial gain): `opacity: 0.3-1.0` (brightness = gain)
- Playhead: CSS transform for GPU-accelerated positioning

### 3.3 Playhead Synchronization

**Critical Pattern #20 - requestAnimationFrame for animations:**

```javascript
let currentStep = 0;

// Processor sends step index via native function polling
async function updatePlayhead() {
    const step = await Juce.getNativeFunction('getCurrentStep')();

    if (step !== currentStep) {
        currentStep = step;
        // Update playhead position (CSS transform for smooth rendering)
        const playhead = document.querySelector('.playhead');
        const stepWidth = 700 / numSteps;  // Dynamic based on steps parameter
        playhead.style.transform = `translateX(${step * stepWidth}px)`;
    }

    requestAnimationFrame(updatePlayhead);
}

updatePlayhead();  // Start loop
```

**Alternative: Timer-based polling from C++:**
```cpp
// In PluginEditor
void timerCallback() override {
    int step = processorRef.getCurrentStep();
    juce::String js = juce::String::formatted(
        "if (window.updatePlayhead) window.updatePlayhead(%d);", step
    );
    webView->evaluateJavascript(js);
}
```

**Trade-off:**
- RAF loop: JavaScript controls polling rate (~60fps), slight latency
- C++ timer: Tighter control, matches processBlock step calculation
- **Recommendation:** C++ timer at 30-60Hz (matches O-Bells meter pattern)

---

## 4. Euclidean Panel (Expandable Accordion)

### 4.1 UI Pattern (from CONTEXT.md)

```
HIGH  ▼  [████░░██░░████░░██░░████░░██░░████░░██░░] E:8/5/0  [▶]
```

Click `[▶]` to expand:
```
┌─ Expanded Euclidean Panel ──────────────────────────────────────────┐
│  Band: HIGH    Mode: [Euclidean ▼]                                   │
│  Steps: ○ 8    Pulses: ○ 5    Offset: ○ 0    Depth: ○ 100%          │
└──────────────────────────────────────────────────────────────────────┘
```

### 4.2 JavaScript State Management

```javascript
let expandedBand = null;  // Only one band expanded at a time

function toggleBandExpanded(bandIndex) {
    if (expandedBand === bandIndex) {
        expandedBand = null;  // Collapse
    } else {
        expandedBand = bandIndex;  // Expand this, collapse others
    }
    updateAccordionVisibility();
}

function updateAccordionVisibility() {
    document.querySelectorAll('.euclidean-panel').forEach((panel, index) => {
        panel.classList.toggle('visible', index === expandedBand);
    });
}
```

### 4.3 Mode Switching (Manual ↔ Euclidean)

When mode changes to Euclidean:
1. Generate Euclidean pattern in JavaScript (for preview)
2. Update C++ parameter (mode toggle)
3. Grid becomes read-only (visual indication)
4. C++ processor generates actual pattern on audio thread

```javascript
const bandModeState = Juce.getComboBoxState(`band${bandIndex}_mode`);

bandModeState.valueChangedEvent.addListener(() => {
    const isEuclidean = bandModeState.getChoiceIndex() === 1;
    setGridEditable(bandIndex, !isEuclidean);

    if (isEuclidean) {
        // Show generated pattern (read from C++ or compute locally)
        showEuclideanPreview(bandIndex);
    }
});
```

---

## 5. Naturalist Aesthetic Implementation

### 5.1 Color Palette (from CONTEXT.md)

```css
:root {
    /* Paper/background */
    --bg-paper: #F5E6D3;
    --bg-header: linear-gradient(to bottom, #EBD9C7, #F5E6D3);

    /* Grid (dark for contrast) */
    --grid-bg: #1a1410;
    --step-off: rgba(60, 47, 47, 0.3);
    --step-on: #5a7a6a;  /* accent green */
    --playhead: #8BA870;  /* lighter green */

    /* Text */
    --text-primary: #3C2F2F;
    --text-secondary: #8b7355;

    /* Borders */
    --border-accent: #8B7355;
}
```

### 5.2 Typography

```css
body {
    font-family: 'Georgia', 'Times New Roman', serif;
    /* Native feel CSS (from juce8-critical-patterns.md) */
    user-select: none;
    -webkit-user-select: none;
    cursor: default;
}
```

### 5.3 Slider Styling (O-Bells pattern)

```css
.slider {
    width: 100%;
    height: 8px;
    background: #E8D5B7;
    border: 1px solid #8B7355;
    border-radius: 4px;
}

.slider-thumb {
    width: 14px;
    height: 16px;
    background: #C9A27B;
    border: 2px solid #8B7355;
    border-radius: 3px;
    cursor: grab;
}
```

---

## 6. Parameter Binding Strategy

### 6.1 Global Parameters (5 params)

| ID | Type | Relay | JS API |
|----|------|-------|--------|
| `mix` | Float 0-1 | WebSliderRelay | `getSliderState('mix')` |
| `steps` | Choice (4,8,16,32) | WebComboBoxRelay | `getComboBoxState('steps')` |
| `rate` | Choice (1/1...1/32T) | WebComboBoxRelay | `getComboBoxState('rate')` |
| `swing` | Float 0-1 | WebSliderRelay | `getSliderState('swing')` |
| `smoothing` | Float 0-100 | WebSliderRelay | `getSliderState('smoothing')` |

### 6.2 Per-Band Parameters (32 params: 8 × 4 bands)

| Pattern | Type | Count | Example |
|---------|------|-------|---------|
| `band{N}_enable` | Bool | 4 | `band0_enable` |
| `band{N}_depth` | Float | 4 | `band0_depth` |
| `band{N}_mode` | Choice | 4 | `band0_mode` (Manual/Euclidean) |
| `band{N}_euc_steps` | Int 1-32 | 4 | `band0_euc_steps` |
| `band{N}_euc_pulses` | Int 1-32 | 4 | `band0_euc_pulses` |
| `band{N}_euc_offset` | Int 0-31 | 4 | `band0_euc_offset` |
| `band{N}_low` | Float | 4 | Fixed for v1.0, not editable |
| `band{N}_high` | Float | 4 | Fixed for v1.0, not editable |

### 6.3 Step Grid Parameters (128 params)

Pattern: `step_b{N}_s{M}` where N=0-3, M=0-31

**Initialization strategy:**
```javascript
const stepStates = [];  // 4 bands × 32 steps

for (let band = 0; band < 4; band++) {
    stepStates[band] = [];
    for (let step = 0; step < 32; step++) {
        const paramId = `step_b${band}_s${step}`;
        stepStates[band][step] = Juce.getToggleState(paramId);

        // Listen for changes (automation, preset load)
        stepStates[band][step].valueChangedEvent.addListener(() => {
            updateStepVisual(band, step, stepStates[band][step].getValue());
        });
    }
}
```

---

## 7. Existing Module Opportunities

### 7.1 JUCE Bridge Files

**Required (copy from O-Bells/other plugins):**
- `js/juce/index.js` - WebView bridge (ES6 exports)
- `js/juce/check_native_interop.js` - Initialization verification

### 7.2 Reusable Patterns

**From O-Bells:**
- Slider binding pattern (mousedown/mousemove/mouseup)
- Choice button binding (click handler → setChoiceIndex)
- Timer-based meter update (adapt for playhead)

**From O-Detune (Naturalist style):**
- Paper texture background
- Slider visual styling
- Header/footer layout

---

## 8. Critical Patterns Checklist

From `troubleshooting/patterns/juce8-critical-patterns.md`:

| Pattern | Relevant? | Implementation |
|---------|-----------|----------------|
| #8 Explicit URL mapping | ✅ | Resource provider with if/else chain |
| #9 NEEDS_WEB_BROWSER | ✅ | Already in CMakeLists.txt |
| #10 Install to system | ✅ | Use build-and-install.sh |
| #11 unique_ptr members | ✅ | Relays → WebView → Attachments |
| #12 Three-param attachment | ✅ | Pass `nullptr` for undoManager |
| #13 check_native_interop.js | ✅ | Include in BinaryData |
| #15 valueChangedEvent no params | ✅ | Call `getNormalisedValue()` inside |
| #16 Relative drag | ✅ | Frame-delta pattern for sliders |
| #19 Boolean → getToggleState | ✅ | For step grid and band enables |
| #21 ES6 type="module" | ✅ | `<script type="module">` |

---

## 9. Risk Assessment

### 9.1 HIGH Risk: 128 Toggle Parameters Performance

**Problem:** Creating 128 WebToggleRelay objects + attachments may cause slow initialization.

**Mitigation:**
1. Batch creation in constructor
2. Consider lazy initialization (create on first grid interaction)
3. Profile startup time; if >2s, switch to native function for grid state

**Fallback:** Use single native function for grid state JSON blob

### 9.2 MEDIUM Risk: Playhead Sync Latency

**Problem:** Playhead may lag behind actual step due to JS↔C++ communication latency.

**Mitigation:**
1. Use C++ timer callback (like O-Bells meters) at 30-60Hz
2. Playhead position is visual feedback, not critical timing
3. Consider predictive positioning based on BPM

### 9.3 LOW Risk: Grid Rendering Performance

**Problem:** 128 cells × frequent updates could cause jank.

**Mitigation:**
1. Only update changed cells (dirty flag per cell)
2. Use CSS transforms (GPU-accelerated)
3. Batch DOM updates in single requestAnimationFrame
4. 32-step mode most common; 128 cells is max

---

## 10. Recommended Implementation Order

1. **WebView scaffolding** (PluginEditor.cpp)
   - Create relays for global params only (5)
   - Resource provider with index.html, index.js, check_native_interop.js
   - Window size 850×550

2. **Basic HTML/CSS** (Naturalist layout)
   - Header with plugin name
   - Grid placeholder (static 4×16 for initial testing)
   - Footer with global controls
   - Paper texture, earthy colors

3. **Global parameter bindings**
   - Mix, Steps, Rate, Swing, Smoothing
   - Slider and dropdown interactions

4. **Band rows with basic grid**
   - 4 band labels with fixed frequencies
   - 32-step cells per band (toggle only, no gain)
   - Step click → toggle state

5. **Playhead sync**
   - Native function `getCurrentStep()`
   - Timer callback in C++
   - CSS transform positioning

6. **Per-band Euclidean panels**
   - Accordion expand/collapse
   - Mode toggle (Manual/Euclidean)
   - Euclidean parameter sliders

7. **Polish**
   - Step gain visualization (brightness)
   - Euclidean pattern preview
   - Band enable toggles
   - Depth knobs

---

## 11. File Structure

```
plugins/O-FreqPulse/
├── Resources/
│   └── ui/
│       ├── index.html
│       ├── css/
│       │   └── styles.css
│       ├── js/
│       │   ├── app.js
│       │   └── juce/
│       │       ├── index.js
│       │       └── check_native_interop.js
│       └── img/
│           └── botanical.png  (optional)
├── Source/
│   ├── PluginEditor.h
│   ├── PluginEditor.cpp
│   ├── PluginProcessor.h
│   └── PluginProcessor.cpp
└── CMakeLists.txt
```

---

## 12. Open Questions for Planning Phase

1. **Botanical illustration:** Should FreqPulse have a unique botanical element (e.g., fern fronds for rhythm)?

2. **Grid gain editing:** Click = toggle only, or should we support drag-to-paint and vertical-drag-for-gain?

3. **Preset browser:** Should v1.0 include preset save/load UI, or defer to v1.1?

4. **Spectrum overlay:** Should we show real-time input spectrum on the grid? (v1.1 feature?)

---

## 13. References

- `troubleshooting/patterns/juce8-critical-patterns.md` - Required reading
- `plugins/O-Bells/Source/PluginEditor.cpp` - WebView pattern reference
- `plugins/O-Bells/Resources/ui/index.html` - Naturalist styling reference
- `plugins/O-Bells/Resources/ui/js/juce/index.js` - JUCE bridge reference
- `plugins/O-FreqPulse/.planning/stages/3-gui/CONTEXT.md` - Discuss phase decisions

---

**Research Status:** ✅ COMPLETE
**Next Phase:** /plugin-plan O-FreqPulse 3-gui
