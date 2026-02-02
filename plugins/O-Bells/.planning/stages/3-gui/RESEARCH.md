# Stage 3: GUI Implementation - Research

**Plugin:** O-Bells
**Stage:** 3 (GUI Implementation)
**Phase:** Research
**Date:** 2026-02-01

---

## Research Summary

This document investigates the implementation approach for O-Bells Stage 3 GUI - a WebView-based UI with 18 parameters, Ouaricon Naturalist aesthetic, tab-based layout, and output level meter.

---

## 1. JUCE 8 WebView Architecture

### Core Pattern: WebSliderRelay + WebSliderParameterAttachment

From `juce8-critical-patterns.md` and O-Lyrica reference implementation:

**C++ Side (PluginEditor.h):**
```cpp
class OBellsAudioProcessorEditor : public juce::AudioProcessorEditor {
private:
    // Order: Relays → WebView → Attachments
    std::unique_ptr<juce::WebSliderRelay> strikePositionRelay;
    std::unique_ptr<juce::WebSliderRelay> malletHardnessRelay;
    // ... 18 total relays

    std::unique_ptr<juce::WebBrowserComponent> webView;

    std::unique_ptr<juce::WebSliderParameterAttachment> strikePositionAttachment;
    // ... 18 total attachments
};
```

**Critical Construction Order (Pattern #11):**
1. Create relays FIRST
2. Create WebView with relay options
3. Create attachments LAST (with nullptr for undoManager - Pattern #12)

**JavaScript Side (ES6 Module Pattern - Pattern #21):**
```html
<script type="module" src="js/juce/index.js"></script>
<script type="module">
    import { getSliderState, getComboBoxState } from './js/juce/index.js';

    // Get parameter state
    const sliderState = getSliderState('strikePosition');

    // Listen for changes (callback receives NO parameters - Pattern #15)
    sliderState.valueChangedEvent.addListener(() => {
        const value = sliderState.getNormalisedValue();
        updateSliderVisual(value);
    });
</script>
```

### Required Files

| File | Purpose | BinaryData Name |
|------|---------|-----------------|
| `Resources/ui/index.html` | Main WebView HTML | `index_html` |
| `Resources/ui/js/juce/index.js` | JUCE ES6 bridge | `index_js` |
| `Resources/ui/js/juce/check_native_interop.js` | Native integration check | `check_native_interop_js` |
| `Resources/ui/img/snail.png` | Botanical overlay | `snail_png` |

### CMake Binary Data Configuration

```cmake
juce_add_binary_data(O-Bells_UIResources
    SOURCES
        Resources/ui/index.html
        Resources/ui/js/juce/index.js
        Resources/ui/js/juce/check_native_interop.js
        Resources/ui/img/snail.png
)

target_link_libraries(O-Bells
    PRIVATE
        O-Bells_UIResources
)
```

### Resource Provider Pattern (Pattern #8)

Explicit URL mapping is required - generic loops break resource loading:

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

    if (url == "/js/juce/check_native_interop.js") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::check_native_interop_js,
                      BinaryData::check_native_interop_jsSize),
            juce::String("text/javascript")
        };
    }

    if (url == "/img/snail.png") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::snail_png, BinaryData::snail_pngSize),
            juce::String("image/png")
        };
    }

    return std::nullopt;  // 404
}
```

---

## 2. Parameter Binding Patterns

### Float Parameters (17 total)

Use `getSliderState()` with `getNormalisedValue()`:

```javascript
const strikeState = getSliderState('strikePosition');

// Initialize visual
updateSlider(strikeSlider, strikeState.getNormalisedValue());

// Listen for changes (no callback params!)
strikeState.valueChangedEvent.addListener(() => {
    updateSlider(strikeSlider, strikeState.getNormalisedValue());
});

// User interaction
strikeSlider.addEventListener('input', (e) => {
    strikeState.setNormalisedValue(parseFloat(e.target.value));
});
```

### Integer Parameter (unisonCount: 1-4)

Use `getSliderState()` but round to integers:

```javascript
const unisonState = getSliderState('unisonCount');

// getNormalisedValue returns 0-1, map to 1-4
function getUnisonValue() {
    const norm = unisonState.getNormalisedValue();
    return Math.round(norm * 3) + 1;  // 0→1, 0.33→2, 0.67→3, 1→4
}

// setNormalisedValue expects 0-1
function setUnisonValue(val) {
    const norm = (val - 1) / 3;  // 1→0, 2→0.33, 3→0.67, 4→1
    unisonState.setNormalisedValue(norm);
}
```

### Choice Parameters (3 total: strikeNoiseChar, decayShape, velocityCurve)

Use `getComboBoxState()`:

```javascript
const noiseState = getComboBoxState('strikeNoiseChar');

// Get current choice (0, 1, or 2)
noiseState.valueChangedEvent.addListener(() => {
    const index = noiseState.getChoiceIndex();
    updateChoiceButtons(noiseButtons, index);
});

// Set choice
noiseButtons.forEach((btn, i) => {
    btn.addEventListener('click', () => {
        noiseState.setChoiceIndex(i);
    });
});
```

---

## 3. O-Lyrica Reference Implementation Analysis

### Slider CSS (from O-Lyrica lines 381-420)

```css
/* Slider track - aged paper aesthetic */
input[type="range"] {
    -webkit-appearance: none;
    appearance: none;
    width: 100%;
    height: 6px;
    background: linear-gradient(to bottom, #D4C4B0 0%, #E8D5B7 50%, #D4C4B0 100%);
    border: 1px solid #8B7355;
    border-radius: 3px;
    cursor: pointer;
    box-shadow: inset 1px 1px 2px rgba(0,0,0,0.15);
}

/* Slider thumb - cream seed-like */
input[type="range"]::-webkit-slider-thumb {
    -webkit-appearance: none;
    appearance: none;
    width: 14px;
    height: 14px;
    background: radial-gradient(circle at 30% 30%, #FFF8DC 0%, #F5DEB3 40%, #D4C4B0 100%);
    border: 2px solid #8B7355;
    border-radius: 50%;
    cursor: pointer;
    box-shadow: 1px 1px 3px rgba(0,0,0,0.25);
    transition: transform 0.1s;
}

input[type="range"]::-webkit-slider-thumb:hover {
    transform: scale(1.1);
}

input[type="range"]::-webkit-slider-thumb:active {
    transform: scale(0.95);
}
```

### Tab Switching Pattern (from O-Lyrica lines 268-325)

```html
<!-- Tab bar -->
<div class="tab-bar">
    <div class="tab active" data-tab="instrument">Instrument</div>
    <div class="tab" data-tab="tuning">Tuning</div>
</div>

<!-- Tab content -->
<div class="tab-content active" id="tab-instrument">...</div>
<div class="tab-content" id="tab-tuning">...</div>
```

```javascript
// Tab switching logic
const tabs = document.querySelectorAll('.tab');
const contents = document.querySelectorAll('.tab-content');
const botanicalOverlay = document.querySelector('.botanical-overlay');

tabs.forEach(tab => {
    tab.addEventListener('click', () => {
        const targetTab = tab.dataset.tab;

        // Update active tab
        tabs.forEach(t => t.classList.remove('active'));
        tab.classList.add('active');

        // Update content visibility
        contents.forEach(c => c.classList.remove('active'));
        document.getElementById(`tab-${targetTab}`).classList.add('active');

        // Shift botanical overlay
        botanicalOverlay.classList.remove('instrument-position', 'tuning-position');
        botanicalOverlay.classList.add(`${targetTab}-position`);
    });
});
```

### Botanical Overlay Animation (O-Lyrica lines 61-86)

```css
.botanical-overlay {
    position: absolute;
    right: -30px;
    top: 50%;
    transform: translateY(-50%);
    height: 480px;
    opacity: 0.25;
    pointer-events: none;
    z-index: 1;
    transition: right 0.4s ease-out, opacity 0.4s ease-out;
}

/* Tab-specific positions */
.botanical-overlay.instrument-position {
    right: -30px;
    opacity: 0.25;
}

.botanical-overlay.tuning-position {
    right: -80px;
    opacity: 0.18;
}
```

---

## 4. Output Level Meter Implementation

### Architecture Decision

For O-Bells (synthesizer), implement a simple stereo peak meter:

1. **C++ Side:** Calculate RMS/peak level in processBlock, expose via atomic floats
2. **JavaScript Side:** Poll via custom native function, animate with requestAnimationFrame

### C++ Level Metering

```cpp
// PluginProcessor.h
class OBellsAudioProcessor : public juce::AudioProcessor {
public:
    // Atomic for thread-safe UI reading
    std::atomic<float> outputLevelL { -60.0f };
    std::atomic<float> outputLevelR { -60.0f };

private:
    float peakL = 0.0f, peakR = 0.0f;
    const float peakDecay = 0.9f;  // ~30ms at 44.1kHz
};

// PluginProcessor.cpp - in processBlock after final output
void OBellsAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, ...)
{
    // ... synthesis code ...

    // Update level meters
    float maxL = 0.0f, maxR = 0.0f;

    for (int i = 0; i < buffer.getNumSamples(); ++i) {
        maxL = std::max(maxL, std::abs(buffer.getSample(0, i)));
        if (buffer.getNumChannels() > 1)
            maxR = std::max(maxR, std::abs(buffer.getSample(1, i)));
    }

    // Apply peak hold with decay
    peakL = std::max(maxL, peakL * peakDecay);
    peakR = std::max(maxR, peakR * peakDecay);

    // Convert to dB and store atomically
    outputLevelL.store(juce::Decibels::gainToDecibels(peakL, -60.0f));
    outputLevelR.store(juce::Decibels::gainToDecibels(peakR, -60.0f));
}
```

### JavaScript Meter Animation (Pattern #20)

```javascript
// Separate current/target with RAF loop
let currentMeterL = -60;
let currentMeterR = -60;
let targetMeterL = -60;
let targetMeterR = -60;

const ATTACK_SPEED = 0.4;   // Fast rise
const DECAY_SPEED = 0.15;   // Slow fall

// Native function to get levels
const getLevelsFn = Juce.getNativeFunction('getLevels');

// Update targets from C++
async function fetchLevels() {
    const levels = await getLevelsFn();
    if (levels) {
        targetMeterL = levels.left;
        targetMeterR = levels.right;
    }
}

// Animation loop
function animateMeters() {
    // Ballistic motion
    const speedL = currentMeterL < targetMeterL ? ATTACK_SPEED : DECAY_SPEED;
    const speedR = currentMeterR < targetMeterR ? ATTACK_SPEED : DECAY_SPEED;

    currentMeterL += (targetMeterL - currentMeterL) * speedL;
    currentMeterR += (targetMeterR - currentMeterR) * speedR;

    // Update visual (map -60 to 0 dB → 0 to 100%)
    const heightL = Math.max(0, (currentMeterL + 60) / 60 * 100);
    const heightR = Math.max(0, (currentMeterR + 60) / 60 * 100);

    meterBarL.style.height = `${heightL}%`;
    meterBarR.style.height = `${heightR}%`;

    // Color zones
    updateMeterColor(meterBarL, heightL);
    updateMeterColor(meterBarR, heightR);

    requestAnimationFrame(animateMeters);
}

// Poll every 50ms (20Hz)
setInterval(fetchLevels, 50);
animateMeters();
```

### Alternative: Simpler Meter Without Native Function

If level metering adds too much complexity, use a CSS-only visual meter driven by output gain parameter (less accurate but simpler):

```css
.output-meter {
    width: 6px;
    height: 80px;
    background: linear-gradient(to top,
        #8BA870 0%,
        #8BA870 60%,
        #D4A574 60%,
        #D4A574 80%,
        #C75050 80%);
    border: 1px solid #8B7355;
    border-radius: 2px;
}
```

**Recommendation:** Start with simple CSS meter, add true level metering in Stage 4 Polish if desired.

---

## 5. Parameter Layout Organization

### Tab 1: Instrument (18 Parameters)

Based on CONTEXT.md layout plan:

```
┌─────────────────────────────────────────────────────────────────────────────┐
│  Header: O-BELLS                                    [Preset Browser]        │
├─────────────────────────────────────────────────────────────────────────────┤
│  [Instrument]  [Tuning]                                                     │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ═══════════════════ SYNTHESIS ═══════════════════                         │
│                                                                             │
│  Strike Position ──●──  Mallet Hardness ──●──  Damping ──●──               │
│  Brightness ──●──       Material ──●──         Inharmonicity ──●──         │
│                                                                             │
│  ═══════════════════ ENSEMBLE ═══════════════════                          │
│                                                                             │
│  Unison [1│2│3│4]      Detune ──●──            Spread ──●──                │
│  Sub ──●──             Oct ──●──                                           │
│                                                                             │
│  ═══════════════════ CHARACTER ═══════════════════                         │
│                                                                             │
│  Strike Noise [Click│Thud│Ping]    Velocity [Lin│Exp│Log]                  │
│  Decay [Linear│Exp│Multi]                                                   │
│                                                                             │
│  ═══════════════════ ADVANCED ═══════════════════                          │
│                                                                             │
│  Partial Tuning ──●──  Pitch Env ──●──  Pitch Env Time ──●──               │
│  Nonlinear ──●──                                                            │
│                                                                             │
├─────────────────────────────────────────────────────────────────────────────┤
│  Footer: Output ──●──  [Meter]                    OUARICON AUDIO           │
└─────────────────────────────────────────────────────────────────────────────┘
```

### Tab 2: Tuning (Placeholder)

```
┌─────────────────────────────────────────────────────────────────────────────┐
│  Header: O-BELLS                                    [Preset Browser]        │
├─────────────────────────────────────────────────────────────────────────────┤
│  [Instrument]  [Tuning]                                                     │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│                                                                             │
│                                                                             │
│                    TUNING MODULE COMING SOON                                │
│                                                                             │
│                    Future integration with                                  │
│                    Ouaricon Tuning System                                   │
│                                                                             │
│                                                                             │
│                                                                             │
├─────────────────────────────────────────────────────────────────────────────┤
│  Footer: Output ──●──  [Meter]                    OUARICON AUDIO           │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## 6. Ouaricon Naturalist Aesthetic Application

### Color Palette (from aesthetic.md)

```css
:root {
    /* Backgrounds */
    --bg-paper-light: #FAF0E6;
    --bg-paper: #F5E6D3;
    --bg-paper-mid: #EBD9C7;
    --bg-accent: #D4C4B0;

    /* Browns */
    --brown-border: #8B7355;
    --brown-frame: #5C4033;
    --brown-text: #3C2F2F;

    /* Greens (Botanical) */
    --green-light: #8BA870;
    --green-mid: #6B8E4E;
    --green-dark: #3C5C1A;
}
```

### Botanical Image

**Source:** `/Users/taylorbrook/Dev/Ouaricon Audio Images/insects/snails_spciesgnra12kiene_0169.png`

**Application:**
- Position: Right side, partially off-screen
- Opacity: 0.25 (Instrument tab) → 0.18 (Tuning tab)
- Height: ~75% of container
- Transition: 0.4s ease-out on tab change

### Typography

```css
body {
    font-family: 'Garamond', 'Times New Roman', serif;
}

.section-header {
    font-size: 9px;
    text-transform: uppercase;
    letter-spacing: 1.5px;
    color: #5C4033;
}

.slider-label {
    font-size: 9px;
    text-transform: uppercase;
    letter-spacing: 0.5px;
    color: #3C2F2F;
}
```

---

## 7. CSS Constraints for JUCE WebView

From `juce8-critical-patterns.md`:

1. **Use `height: 100%`, NOT `100vh`** - JUCE WebView requires percentage-based sizing
2. **Include `box-sizing: border-box`** on all elements
3. **Fixed plugin dimensions** - 800x600px as specified in CONTEXT.md
4. **Native application feel:**
   ```css
   user-select: none;
   -webkit-user-select: none;
   -webkit-touch-callout: none;
   -webkit-tap-highlight-color: transparent;
   cursor: default;
   ```

---

## 8. Files to Create

### Resources/ui/index.html
Main WebView interface with:
- HTML structure (header, tabs, sections, footer)
- CSS styling (Ouaricon Naturalist aesthetic)
- JavaScript parameter bindings (ES6 module pattern)

### Resources/ui/js/juce/index.js
Copy from JUCE examples or working plugin (O-Lyrica)

### Resources/ui/js/juce/check_native_interop.js
Copy from JUCE examples or working plugin

### Resources/ui/img/snail.png
Copy from `/Users/taylorbrook/Dev/Ouaricon Audio Images/insects/snails_spciesgnra12kiene_0169.png`

### Source/PluginEditor.h/.cpp Updates
- WebSliderRelay for each parameter (18 total)
- WebBrowserComponent with resource provider
- WebSliderParameterAttachment for each parameter
- Level meter atomic variables (if implementing true metering)

### CMakeLists.txt Updates
- Add binary data target for UI resources
- Link UI resources to plugin target

---

## 9. Implementation Risks

### Risk 1: 18 Parameters = Many Relays/Attachments
**Mitigation:** Use arrays/vectors to organize relays and attachments systematically

### Risk 2: Choice Parameters Binding
**Mitigation:** Use `getComboBoxState()` not `getSliderState()` for choice parameters

### Risk 3: Integer Parameter (unisonCount)
**Mitigation:** `getSliderState()` works but requires manual normalization mapping

### Risk 4: Level Meter Thread Safety
**Mitigation:** Use `std::atomic<float>` for level values, only read from UI thread

### Risk 5: Image Asset Size
**Mitigation:** Compress PNG before including in binary data

---

## 10. Reference Files

| Purpose | Path |
|---------|------|
| O-Lyrica UI Reference | `plugins/O-Lyrica/Resources/ui/index.html` |
| JUCE 8 Critical Patterns | `troubleshooting/patterns/juce8-critical-patterns.md` |
| Ouaricon Aesthetic | `.claude/aesthetics/ouaricon-naturalist-001/aesthetic.md` |
| O-Bells Parameter Spec | `plugins/O-Bells/.planning/parameter-spec.md` |
| O-Bells CONTEXT.md | `plugins/O-Bells/.planning/stages/3-gui/CONTEXT.md` |
| Botanical Image | `/Users/taylorbrook/Dev/Ouaricon Audio Images/insects/snails_spciesgnra12kiene_0169.png` |

---

## Summary

Stage 3 GUI implementation for O-Bells requires:

1. **WebView UI** with JUCE 8 ES6 module pattern
2. **18 parameter bindings** (17 sliders + 1 combobox selector for unison, 3 comboboxes for choices)
3. **Tab-based layout** (Instrument / Tuning placeholder)
4. **Ouaricon Naturalist aesthetic** with snail botanical overlay
5. **Simple output meter** (CSS-based initially, true level metering optional for Stage 4)
6. **800x600 fixed dimensions**

All critical JUCE 8 patterns from `juce8-critical-patterns.md` must be followed, particularly:
- Pattern #11: unique_ptr member initialization order
- Pattern #12: Three-parameter WebSliderParameterAttachment
- Pattern #15: valueChangedEvent callbacks receive no parameters
- Pattern #21: ES6 module type="module" required

---

*Research completed: 2026-02-01*
*Ready for: Plan phase*
