# Stage 3: GUI - Research

**Plugin:** O-IntonationPad
**Stage:** 3 - GUI Implementation
**Research Date:** 2026-01-29
**Status:** Complete

---

## Research Summary

This document captures technical research findings for implementing O-IntonationPad's WebView-based GUI with 15 parameters, tabbed layout, and interactive pitch circle visualization.

---

## 1. Pitch Circle Implementation

### Research Question
Best approach for real-time pitch visualization in WebView - Canvas vs SVG?

### Findings

**Existing Module Found:** `modules/tuning/scala-tuning-engine/js/pitch-circle.js`

The codebase already has a pitch circle component that can be reused:

```javascript
// From pitch-circle.js (lines 21-253)
export class PitchCircle {
    constructor(options = {}) {
        this.container = options.container;
        this.size = options.size || 150;
        this.showLabels = options.showLabels !== false;
        this.lineColor = options.lineColor || '#6B8E4E';  // Naturalist green
        this.activeColor = options.activeColor || '#DC0000';
        // ...
    }

    setIntervals(intervals, tonic = 0) { ... }
    setNoteActive(midiNote, velocity = 0.8) { ... }
    setNoteInactive(midiNote) { ... }
    flashNote(midiNote, velocity = 0.8, durationMs = 150) { ... }
}
```

**Key Features:**
- **SVG-based rendering** - Scalable, clean, integrates well with DOM
- **Already styled for Naturalist aesthetic** - Uses `#6B8E4E` green, `#8B7355` brown
- **Real-time note highlighting** - `setNoteActive()` / `setNoteInactive()` API
- **Tonic transposition support** - `setIntervals(intervals, tonic)` API
- **Configurable size** - Default 150px, can scale to 200-250px for O-IntonationPad

**Integration Strategy:**
1. Copy `pitch-circle.js` to `plugins/O-IntonationPad/Source/ui/public/modules/`
2. Import as ES6 module in main.js
3. Connect to tuningSystem parameter changes
4. Optionally send MIDI note events from C++ to highlight active pitches

**Active Note Communication (Optional - Stage 4 Polish):**
For real-time pitch highlighting during playback, need to send MIDI data from C++ to WebView:
- Option A: Native function called from processBlock (expensive, audio thread concerns)
- Option B: Timer-based polling from editor thread (safer, slight latency)
- Option C: Defer to Stage 4 polish - start with static interval display

**Recommendation:** Start with static interval display based on `tuningSystem` and `keyRoot` parameters. Add active note highlighting in Stage 4 if CPU budget allows.

---

## 2. Tab Component Implementation

### Research Question
Build custom tab component or use lightweight library?

### Findings

**Custom Implementation Recommended**

All existing Ouaricon plugins use custom CSS tabs. The pattern is simple:

```html
<!-- Tab Navigation -->
<div class="tab-nav">
    <button class="tab-btn active" data-tab="voice">Voice</button>
    <button class="tab-btn" data-tab="tuning">Tuning</button>
    <button class="tab-btn" data-tab="modulation">Modulation</button>
    <button class="tab-btn" data-tab="output">Output</button>
</div>

<!-- Tab Content Panels -->
<div class="tab-content active" id="voice">...</div>
<div class="tab-content" id="tuning">...</div>
<div class="tab-content" id="modulation">...</div>
<div class="tab-content" id="output">...</div>
```

**CSS Styling (Naturalist Aesthetic):**
```css
.tab-nav {
    display: flex;
    gap: 2px;
    border-bottom: 2px solid #5C4033;
    margin-bottom: 15px;
}

.tab-btn {
    padding: 8px 16px;
    background: rgba(139, 168, 112, 0.2);
    border: 1px solid #8B7355;
    border-bottom: none;
    border-radius: 4px 4px 0 0;
    font-family: 'Garamond', serif;
    font-size: 11px;
    text-transform: uppercase;
    letter-spacing: 1px;
    color: #5C4033;
    cursor: pointer;
    transition: background 0.15s;
}

.tab-btn:hover {
    background: rgba(139, 168, 112, 0.35);
}

.tab-btn.active {
    background: rgba(107, 142, 35, 0.4);
    color: #2C3E10;
    border-color: #5C4033;
}

.tab-content {
    display: none;
}

.tab-content.active {
    display: block;
}
```

**JavaScript Handler:**
```javascript
function setupTabs() {
    const tabBtns = document.querySelectorAll('.tab-btn');
    const tabContents = document.querySelectorAll('.tab-content');

    tabBtns.forEach(btn => {
        btn.addEventListener('click', () => {
            // Remove active from all
            tabBtns.forEach(b => b.classList.remove('active'));
            tabContents.forEach(c => c.classList.remove('active'));

            // Add active to clicked
            btn.classList.add('active');
            document.getElementById(btn.dataset.tab).classList.add('active');
        });
    });
}
```

**Accessibility:**
- Tab buttons receive focus by default (native button element)
- Add `role="tablist"` and `role="tab"` for screen readers
- Arrow key navigation optional (defer to Stage 4 polish)

---

## 3. Choice Parameter Styling

### Research Question
How to style dropdown vs button row for choice parameters?

### Findings

**O-IntonationPad has 3 choice parameters:**
1. `keyRoot` (12 options: C-B)
2. `keyScale` (10 options: Major through Melodic Minor)
3. `tuningSystem` (5 options: 12-TET, JI, Pythagorean, Historical, Scala)

**Recommendations by Option Count:**

| Parameter | Options | Control Type | Rationale |
|-----------|---------|--------------|-----------|
| keyRoot | 12 | Dropdown | Too many for buttons |
| keyScale | 10 | Dropdown | Too many for buttons |
| tuningSystem | 5 | Button Row | Fits well, quick access |

**Dropdown Styling (Naturalist):**
```css
.dropdown-select {
    width: 100%;
    padding: 8px 28px 8px 12px;
    background: rgba(139, 168, 112, 0.3);
    border: 2px solid #5C4033;
    border-radius: 4px;
    font-family: 'Garamond', serif;
    font-size: 12px;
    color: #3C2F2F;
    cursor: pointer;
    appearance: none;
    /* Custom dropdown arrow */
    background-image: url("data:image/svg+xml,..."); /* Brown triangle */
    background-position: calc(100% - 10px) center;
    background-repeat: no-repeat;
}

.dropdown-select:focus {
    outline: 2px solid #6B8E4E;
    outline-offset: 1px;
}
```

**Button Row for tuningSystem:**
```css
.tuning-buttons {
    display: flex;
    gap: 4px;
    flex-wrap: wrap;
}

.tuning-btn {
    flex: 1;
    min-width: 60px;
    padding: 6px 8px;
    background: rgba(139, 168, 112, 0.25);
    border: 1px solid #8B7355;
    border-radius: 3px;
    font-family: 'Garamond', serif;
    font-size: 9px;
    text-transform: uppercase;
    letter-spacing: 0.5px;
    color: #3C2F2F;
    cursor: pointer;
    transition: background 0.15s;
}

.tuning-btn.active {
    background: rgba(107, 142, 35, 0.5);
    border-color: #3C5C1A;
    color: #1a2a10;
}
```

---

## 4. WebView ↔ APVTS Bidirectional Binding

### Research Question
Pattern for binding 15 parameters between WebView and APVTS?

### Findings

**Established Pattern from O-Tremolo (PluginEditor.cpp):**

```cpp
// Member declaration order is CRITICAL (Pattern #11)
// 1. RELAYS FIRST
std::unique_ptr<juce::WebSliderRelay> voiceCountRelay;
std::unique_ptr<juce::WebSliderRelay> complexityRelay;
// ... (15 total)

// 2. WEBVIEW SECOND
std::unique_ptr<juce::WebBrowserComponent> webView;

// 3. ATTACHMENTS LAST
std::unique_ptr<juce::WebSliderParameterAttachment> voiceCountAttachment;
std::unique_ptr<juce::WebSliderParameterAttachment> complexityAttachment;
// ... (15 total)
```

**Constructor Pattern:**
```cpp
OIntonationPadEditor::OIntonationPadEditor(OIntonationPadProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    // 1. Create all 15 relays
    voiceCountRelay = std::make_unique<juce::WebSliderRelay>("voiceCount");
    // ...

    // 2. Create WebView with all relay options
    webView = std::make_unique<juce::WebBrowserComponent>(
        juce::WebBrowserComponent::Options{}
            .withNativeIntegrationEnabled()
            .withResourceProvider([this](auto& url) { return getResource(url); })
            .withOptionsFrom(*voiceCountRelay)
            .withOptionsFrom(*complexityRelay)
            // ... all 15 relays
    );

    // 3. Create all 15 attachments
    voiceCountAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.getAPVTS().getParameter("voiceCount"), *voiceCountRelay, nullptr);
    // ...

    addAndMakeVisible(*webView);
    setSize(800, 500);
}
```

**JavaScript Binding Pattern:**
```javascript
import { getSliderState } from './js/juce/index.js';

// Get parameter states
const voiceCountState = getSliderState('voiceCount');
const complexityState = getSliderState('complexity');
// ...

// Setup knob with JUCE binding
function setupKnob(paramId, state, displayOptions) {
    const knob = document.getElementById(`${paramId}Knob`);
    const valueDisplay = document.getElementById(`${paramId}Value`);
    let isDragging = false;
    let lastY = 0;

    // JUCE → UI: Listen for changes
    state.valueChangedEvent.addListener(() => updateVisual());

    // UI → JUCE: Mouse drag
    knob.addEventListener('mousedown', (e) => {
        isDragging = true;
        lastY = e.clientY;
        state.sliderDragStarted();
        e.preventDefault();
    });

    document.addEventListener('mousemove', (e) => {
        if (!isDragging) return;
        const deltaY = lastY - e.clientY;
        const currentNorm = state.getNormalisedValue();
        const newNorm = Math.max(0, Math.min(1, currentNorm + (deltaY * 0.005)));
        state.setNormalisedValue(newNorm);
        lastY = e.clientY;
    });

    document.addEventListener('mouseup', () => {
        if (isDragging) {
            state.sliderDragEnded();
            isDragging = false;
        }
    });

    function updateVisual() {
        const norm = state.getNormalisedValue();
        // Update knob rotation and value display
    }

    updateVisual();
}
```

**JUCE 8 WebSliderRelay Patterns:**
- Pattern #15: `valueChangedEvent.addListener()` takes NO parameters
- Pattern #16: Relative drag (delta from last position), not absolute
- Pattern #12: Attachment constructor takes 3 params (parameter, relay, nullptr)
- Pattern #11: Member declaration order prevents crash (relays → webView → attachments)

---

## 5. Parameter-to-Relay Mapping

### All 15 Parameters

| Parameter ID | Relay Type | Control Type | Tab |
|--------------|------------|--------------|-----|
| voiceCount | WebSliderRelay | Int knob (2-12) | Voice |
| complexity | WebSliderRelay | Float knob (0-100%) | Voice |
| keyRoot | WebSliderRelay | Dropdown (12 items) | Voice |
| keyScale | WebSliderRelay | Dropdown (10 items) | Voice |
| inversionRandom | WebSliderRelay | Float knob (0-100%) | Voice |
| tuningSystem | WebSliderRelay | Button row (5 items) | Tuning |
| wavetablePos | WebSliderRelay | Float knob (0-100%) | Modulation |
| lfoRate | WebSliderRelay | Float knob (0.01-20 Hz) | Modulation |
| lfoDepth | WebSliderRelay | Float knob (0-100%) | Modulation |
| timingRandom | WebSliderRelay | Float knob (0-100ms) | Modulation |
| detuneRandom | WebSliderRelay | Float knob (0-50 cents) | Modulation |
| attackTime | WebSliderRelay | Float knob (1-5000ms) | Output |
| releaseTime | WebSliderRelay | Float knob (10-10000ms) | Output |
| filterCutoff | WebSliderRelay | Float knob (20-20kHz) | Output |
| masterVolume | WebSliderRelay | Float knob (-inf to +6dB) | Output |

**Note:** All parameters use `WebSliderRelay` even for choice parameters - JUCE normalizes choice index to 0-1 range automatically.

---

## 6. Resource Provider Structure

### Files to Embed in BinaryData

| File Path | BinaryData Name | MIME Type |
|-----------|-----------------|-----------|
| index.html | index_html | text/html |
| js/juce/index.js | index_js | text/javascript |
| js/juce/check_native_interop.js | check_native_interop_js | text/javascript |
| modules/pitch-circle.js | pitchcircle_js | text/javascript |
| img/paper.jpg | paper_jpg | image/jpeg |
| img/shell_conchologiaiconi12reev_0090.png | shell_png | image/png |

### CMakeLists.txt Addition

```cmake
juce_add_binary_data(OIntonationPad_UIResources
    HEADER_NAME "BinaryData.h"
    NAMESPACE BinaryData
    SOURCES
        Source/ui/public/index.html
        Source/ui/public/js/juce/index.js
        Source/ui/public/js/juce/check_native_interop.js
        Source/ui/public/modules/pitch-circle.js
        Source/ui/public/img/paper.jpg
        Source/ui/public/img/shell_conchologiaiconi12reev_0090.png
)

target_link_libraries(OIntonationPad
    PRIVATE
        OIntonationPad_UIResources
        # ...
)
```

---

## 7. Layout Considerations

### Window Size: 800x500

**Tab Content Area:** ~740x400 (after header and tab nav)

**Tab 1 (Voice) - 5 controls:**
```
[Knob: Voice Count] [Knob: Complexity] [Dropdown: Key] [Dropdown: Scale] [Knob: Inversion]
      60px               60px             140px           160px              60px
```

**Tab 2 (Tuning) - Pitch Circle + Buttons:**
```
[Button Row: 5 tuning systems (320px wide)]

    +------------------+
    |   Pitch Circle   |     Current Key: C Major
    |    (200-250px)   |     System: Just Intonation
    |                  |
    +------------------+
```

**Tab 3 (Modulation) - 5 controls:**
```
[Knob: Position] [Knob: LFO Rate] [Knob: LFO Depth] [Knob: Timing] [Knob: Detune]
     60px            60px              60px             60px          60px
```

**Tab 4 (Output) - 4 controls:**
```
[Knob: Attack] [Knob: Release] [Knob: Filter] [Knob: Volume]
    60px           60px            60px           60px
```

**Botanical Overlay Position:**
- Position: Right side, overlapping tabs
- Height: ~350px (70% of window)
- Opacity: 0.30-0.35 (slightly lower to avoid tab interference)
- Right offset: -30px (allow some bleed)

---

## 8. Aesthetic Module Reuse

### Files to Copy from Existing Plugins

**From O-Tremolo:**
- `js/juce/index.js` - JUCE WebView bridge
- `js/juce/check_native_interop.js` - Native interop checker
- `img/paper.jpg` - Aged paper background texture

**From scala-tuning-engine module:**
- `pitch-circle.js` - Pitch circle visualization

**New Image to Copy:**
```bash
cp "Ouaricon Audio Images/ocean/shell_conchologiaiconi12reev_0090.png" \
   "plugins/O-IntonationPad/Source/ui/public/img/shell.png"
```

---

## 9. Potential Pitfalls

### Known Issues from Troubleshooting Knowledge Base

1. **Member Declaration Order (Pattern #11)**
   - CRITICAL: relays → webView → attachments
   - Wrong order causes crash on plugin close (destructor order issue)

2. **Navigation Timing (parentHierarchyChanged)**
   - Must wait until `isShowing()` before calling `goToURL()`
   - Use instance variable `hasNavigated` (not static)

3. **WebView Constraints**
   - No `vw`/`vh` viewport units - use px or %
   - No `position: fixed` - use `position: absolute`
   - Chromium rendering only

4. **JUCE 8 Breaking Changes**
   - `withOptionsFrom(*relay)` required for relay registration
   - `WebSliderParameterAttachment` takes 3 parameters (param, relay, undo)

5. **Choice Parameter Handling**
   - Choice parameters normalize to 0-1 range
   - Index = round(normValue * (numChoices - 1))
   - Use `getSliderState()` not special choice API

---

## 10. Implementation Complexity Estimate

| Component | Complexity | Notes |
|-----------|------------|-------|
| Tab system | Low | Standard CSS/JS pattern |
| 15 parameter bindings | Medium | Boilerplate but straightforward |
| Pitch circle integration | Low | Module exists, just wire up |
| Naturalist aesthetic | Low | Template exists, apply consistently |
| Choice parameter dropdowns | Medium | Styling and JS handler needed |
| Resource provider | Low | Explicit URL mapping pattern |
| **Total** | **Medium** | No novel components, proven patterns |

---

## 11. Deferred to Stage 4 (Polish)

The following features are out of scope for Stage 3:

1. **Active note highlighting** - Requires MIDI→WebView communication
2. **Wavetable selector UI** - Placeholder for Stage 4
3. **Scala file import dialog** - Requires native file chooser integration
4. **Keyboard navigation for tabs** - Accessibility polish
5. **Preset manager UI** - Standard module integration
6. **Tooltips/help text** - Polish feature

---

## 12. Recommended Implementation Order

1. **Phase 1: WebView Scaffold**
   - Create PluginEditor.h/cpp with WebView pattern
   - Create resource provider with index.html stub
   - Build and verify WebView loads

2. **Phase 2: Tab Structure**
   - Build HTML with 4 tabs
   - Style tabs with Naturalist aesthetic
   - Add tab switching JavaScript

3. **Phase 3: Parameter Controls**
   - Add all 15 relays and attachments
   - Implement knob controls (9 float params)
   - Implement dropdown controls (2 choice params)
   - Implement button row (1 choice param - tuningSystem)

4. **Phase 4: Pitch Circle**
   - Integrate pitch-circle.js module
   - Connect to tuningSystem and keyRoot parameters
   - Style to match Naturalist aesthetic

5. **Phase 5: Polish**
   - Add botanical overlay
   - Fine-tune spacing and layout
   - Test parameter automation
   - Verify bidirectional sync

---

## Appendix: Key File References

| Reference | Path |
|-----------|------|
| Aesthetic Template | `.claude/aesthetics/ouaricon-naturalist-001/aesthetic.md` |
| Pitch Circle Module | `modules/tuning/scala-tuning-engine/js/pitch-circle.js` |
| WebView Relay Manager | `modules/core/webview-relay-manager/cpp/WebViewRelayManager.h` |
| O-Tremolo Editor (pattern) | `plugins/O-Tremolo/Source/PluginEditor.cpp` |
| JUCE Bridge JS | `plugins/O-Tremolo/Source/ui/public/js/juce/index.js` |
| Parameter Spec | `plugins/O-IntonationPad/.planning/parameter-spec.md` |
| Stage 3 Context | `plugins/O-IntonationPad/.planning/stages/3-gui/CONTEXT.md` |

---

## Research Complete

All questions from CONTEXT.md have been answered:
1. Pitch circle: SVG via existing module (pitch-circle.js)
2. Tab component: Custom CSS/JS (proven pattern)
3. Choice parameter styling: Dropdowns for 10+ options, button row for 5
4. WebView binding: 15 WebSliderRelay instances with standard pattern

**Ready for:** `/plugin-plan O-IntonationPad 3-gui`
