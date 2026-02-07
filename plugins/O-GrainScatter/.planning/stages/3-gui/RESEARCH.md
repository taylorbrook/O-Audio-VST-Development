# Stage 3: GUI - Research

**Date:** 2026-02-07
**Plugin:** O-GrainScatter
**Stage:** 3-gui (Research phase)

---

## 1. JUCE WebView Data Flow: C++ → JavaScript

### Primary Mechanism: `emitEventIfBrowserIsVisible()`

The recommended pattern for real-time visualization updates (30-60 FPS). Used by O-SpectralShaper for FFT spectrogram data.

**C++ side (timerCallback):**
```cpp
void GrainScatterEditor::timerCallback()
{
    juce::String json = "{\"grains\":[";
    // ... build JSON from grain state ...
    json += "]}";
    webView->emitEventIfBrowserIsVisible("grainUpdate", json);
}
```

**JavaScript side:**
```javascript
window.__JUCE__.backend.addEventListener('grainUpdate', (event) => {
    const data = JSON.parse(event);
    grainVisualizer.update(data.grains);
});
```

**How it works internally** (from `juce_WebBrowserComponent.cpp:415-430`):
- Serializes `var` to JSON string
- Escapes for JS parameter substitution
- Calls `evaluateJavascript("window.__JUCE__.backend.emitByBackend(eventId, data)")`
- JS `check_native_interop.js:139-141` fires `addEventListener` callbacks

**Performance:** O-SpectralShaper pushes 512-bin FFT arrays at 60 Hz successfully. 64 grain structs at 30 Hz will be lighter.

### Secondary: `evaluateJavascript()`

Direct JS execution from C++. Used by O-FreqPulse for playhead position:
```cpp
juce::String js = juce::String::formatted(
    "if (window.updatePlayhead) window.updatePlayhead(%d, %s);",
    step, hasSignal ? "true" : "false"
);
webView->evaluateJavascript(js);
```

Simpler for small data but less structured than emitEvent.

### Tertiary: `withNativeFunction()` (JS → C++ polling)

For lower-frequency queries. JS calls C++ and gets a Promise back:
```cpp
.withNativeFunction("getVoiceCount", [this](const juce::Array<juce::var>&,
    std::function<void(juce::var)> complete) {
    complete(juce::var(processorRef.getActiveVoiceCount()));
})
```
```javascript
const getVoiceCount = Juce.getNativeFunction('getVoiceCount');
const count = await getVoiceCount(); // Returns Promise
```

**Decision:** Use `emitEventIfBrowserIsVisible()` for grain + Euclidean visualization (30 Hz push). No native functions needed for O-GrainScatter.

---

## 2. Lock-Free Grain State for Visualization

### Current State
`GrainPool.h` has a `std::array<GrainVoice, 64> voices` — not thread-safe for GUI reads. The pool is modified on the audio thread.

### Approach: Snapshot Array on Audio Thread

Add a visualization snapshot struct to `PluginProcessor.h`:

```cpp
struct GrainVizSnapshot {
    struct Voice {
        bool active = false;
        float positionNorm = 0.0f;   // 0-1, position in delay buffer normalized to 2s
        float pitchSemitones = 0.0f; // Semitone offset from original (log2(playbackRate) * 12)
        float pan = 0.5f;            // 0=left, 1=right
        float envelope = 0.0f;       // 0-1, current envelope phase for fade
        bool reverse = false;
        bool frozen = false;
    };
    std::array<Voice, 64> voices {};
    int activeCount = 0;
};
```

**Two options for thread safety:**

**Option A: Double-buffer with atomic flag (recommended)**
```cpp
std::array<GrainVizSnapshot, 2> vizSnapshots;
std::atomic<int> vizWriteIndex { 0 };
// Audio thread writes to vizSnapshots[vizWriteIndex], then flips
// GUI thread reads vizSnapshots[1 - vizWriteIndex.load()]
```

**Option B: SpinLock copy**
```cpp
juce::SpinLock vizLock;
GrainVizSnapshot vizSnapshot;
// Audio thread: SpinLock::ScopedLockType lock(vizLock); copy data
// GUI thread: SpinLock::ScopedLockType lock(vizLock); read data
```

**Decision:** Option A (double buffer). No contention, no lock. Audio thread writes snapshot at end of processBlock, flips index. Timer reads the non-active buffer. O-SpectralShaper uses a similar FIFO approach.

### Euclidean Pattern Data

Already cached in `PluginProcessor.h`:
```cpp
std::array<bool, 16> euclideanPattern {};
int cachedEuclideanSteps = 0;
int cachedEuclideanPulses = 0;
```

Need to add:
- `std::atomic<int> currentEuclideanStep { 0 }` — updated in processBlock when a step advances
- Expose pattern + step to editor via getters

---

## 3. Parameter Binding Patterns

### JUCE 8 Web Relay System

**Critical Pattern:** Event callbacks have NO parameters. Must fetch inside callback.

```javascript
// CORRECT (JUCE 8)
state.valueChangedEvent.addListener(() => {
    const value = state.getNormalisedValue(); // Fetch inside
    updateKnob(value);
});

// WRONG (would fail silently)
state.valueChangedEvent.addListener((value) => { ... });
```

### Relay Types and JS API

| Relay Type | JS API | Methods |
|------------|--------|---------|
| `WebSliderRelay` | `Juce.getSliderState(id)` | `getNormalisedValue()`, `setNormalisedValue(v)`, `sliderDragStarted()`, `sliderDragEnded()` |
| `WebComboBoxRelay` | `Juce.getComboBoxState(id)` | `getChoiceIndex()`, `setChoiceIndex(i)`, `getProperties()` (for option list) |
| `WebToggleButtonRelay` | `Juce.getToggleState(id)` | `getValue()`, `setValue(bool)` |

### Existing Relay IDs (from PluginEditor.cpp)

| ID | Type | Control |
|----|------|---------|
| `grain_size` | Slider | Knob |
| `density` | Slider | Knob |
| `pitch_random` | Slider | Knob |
| `pan_random` | Slider | Knob |
| `scale` | ComboBox | Dropdown |
| `root_note` | ComboBox | Dropdown |
| `reverse` | Slider | Knob |
| `feedback` | Slider | Knob |
| `dry_wet` | Slider | Knob |
| `sync_mode` | ComboBox | Dropdown |
| `probability` | Slider | Knob |
| `repeats` | Slider | Knob |
| `spread` | Slider | Knob |
| `pitch_mode` | ComboBox | Dropdown |
| `freeze` | Toggle | Toggle button |
| `stutter_gate` | Toggle | Toggle button |
| `euclidean_pulses` | Slider | Knob |
| `euclidean_steps` | Slider | Knob |

**Total:** 12 sliders, 4 comboboxes, 2 toggles = 18 parameters

---

## 4. Naturalist Knob Implementation

### Reference: O-IntonationPad Seed Cross-Section Knob

**CSS Pattern:**
```css
.knob {
    width: 55px;
    height: 55px;
    border-radius: 50%;
    border: 2px solid #8B7355;
    background:
        radial-gradient(circle, transparent 88%, #C9A27B 88%, #C9A27B 92%,
                        #8B7355 92%, #8B7355 94%, transparent 94%),
        conic-gradient(from 0deg,
            #F5DEB3 0deg, #F5DEB3 18deg, #8B7355 18deg, #8B7355 19deg,
            #E8D5B7 19deg, #E8D5B7 36deg, #8B7355 36deg, #8B7355 37deg,
            /* ... repeats 10 segments ... */
        ),
        radial-gradient(circle, #FFF8DC 0%, #FFF8DC 20%, transparent 20%);
    box-shadow: inset 1px 1px 3px rgba(0,0,0,0.3),
                inset -1px -1px 2px rgba(255,248,220,0.5),
                2px 2px 6px rgba(0,0,0,0.25);
}
```

**Indicator:** Triangle pointer at top, rotated via CSS transform:
```css
.knob-indicator {
    border-left: 5px solid transparent;
    border-right: 5px solid transparent;
    border-top: 22px solid #8B6914;
    transform-origin: center 30px; /* Rotate from knob center */
}
```

**Drag Logic (from O-IntonationPad):**
```javascript
function setupKnob(paramId, state, min, max, unit, formatter) {
    let isDragging = false;
    let lastY = 0;

    knob.addEventListener('mousedown', (e) => {
        isDragging = true;
        lastY = e.clientY;
        state.sliderDragStarted();
    });

    document.addEventListener('mousemove', (e) => {
        if (!isDragging) return;
        const deltaY = lastY - e.clientY;
        const currentNorm = state.getNormalisedValue();
        const sensitivity = 0.005;
        const newNorm = Math.max(0, Math.min(1, currentNorm + deltaY * sensitivity));
        state.setNormalisedValue(newNorm);

        const angle = -140 + (newNorm * 280);
        indicator.style.transform = `translateX(-50%) rotate(${angle}deg)`;
        indicator.style.transformOrigin = 'center 30px';
        lastY = e.clientY;
    });

    document.addEventListener('mouseup', () => {
        if (isDragging) {
            state.sliderDragEnded();
            isDragging = false;
        }
    });
}
```

**Key detail:** Must call `sliderDragStarted()` on mousedown and `sliderDragEnded()` on mouseup for proper JUCE automation recording.

---

## 5. Dropdown (ComboBox) Styling

### Reference Pattern (O-FreqPulse)

```css
.dropdown {
    background: #E8D5B7;
    border: 1px solid #8B7355;
    font-family: 'Georgia', 'Times New Roman', serif;
    font-size: 11px;
    color: #3C2F2F;
    padding: 4px 8px;
    border-radius: 3px;
    appearance: none;
    cursor: pointer;
}
```

**JS Binding (ComboBox relay):**
```javascript
const state = Juce.getComboBoxState('scale');

dropdown.addEventListener('change', (e) => {
    state.setChoiceIndex(e.target.selectedIndex);
});

state.valueChangedEvent.addListener(() => {
    dropdown.selectedIndex = state.getChoiceIndex();
});
```

**Getting option names from JUCE:** `state.getProperties()` returns metadata including choice labels, but the O-IntonationPad/O-Bells pattern is to hardcode option labels in HTML matching the C++ parameter order.

---

## 6. Toggle Button Styling

### Reference Pattern (Aesthetic spec)

```css
.toggle {
    background: rgba(139, 168, 112, 0.3);
    border: 2px solid #3C5C1A;
    border-radius: 4px;
    padding: 8px 12px;
    font-family: 'Garamond', serif;
    font-size: 10px;
    text-transform: uppercase;
    letter-spacing: 0.5px;
    color: #2C3E10;
    cursor: pointer;
}

.toggle.active {
    background: rgba(107, 142, 35, 0.6);
    border-color: #2C3E10;
}
```

**Freeze glow effect (CONTEXT.md requirement):**
```css
.toggle.freeze.active {
    box-shadow: 0 0 12px rgba(107, 142, 78, 0.5),
                0 0 24px rgba(107, 142, 78, 0.2);
    animation: freezePulse 2s ease-in-out infinite;
}

@keyframes freezePulse {
    0%, 100% { box-shadow: 0 0 12px rgba(107, 142, 78, 0.5); }
    50% { box-shadow: 0 0 20px rgba(107, 142, 78, 0.7); }
}
```

**JS Binding (Toggle relay):**
```javascript
const state = Juce.getToggleState('freeze');

button.addEventListener('click', () => {
    state.setValue(!state.getValue());
});

state.valueChangedEvent.addListener(() => {
    const isActive = state.getValue();
    button.classList.toggle('active', isActive);
});
```

---

## 7. Grain Scatter Visualization (Canvas 2D)

### Design

2D scatter plot: X = time position in delay buffer (0-2s), Y = pitch offset (semitones).

Each active grain rendered as a circle:
- Position: X from `positionNorm`, Y from `pitchSemitones`
- Size: Proportional to grain length
- Opacity: Follows Hann envelope phase (fades as grain dies)
- Color: Earth-tone palette, possibly indicating pan position
- Reverse grains: Different shape or color tint

### Canvas 2D Implementation Pattern

From O-MultiBandCompressor and O-SpectralShaper patterns:

```javascript
class GrainScatterViz {
    constructor(canvas) {
        this.canvas = canvas;
        this.ctx = canvas.getContext('2d');
        this.grains = [];
        this.frozen = false;

        // Retina scaling
        const dpr = window.devicePixelRatio || 1;
        canvas.width = canvas.offsetWidth * dpr;
        canvas.height = canvas.offsetHeight * dpr;
        this.ctx.scale(dpr, dpr);

        this.width = canvas.offsetWidth;
        this.height = canvas.offsetHeight;
    }

    update(grainData) {
        this.grains = grainData;
    }

    draw() {
        const ctx = this.ctx;
        ctx.clearRect(0, 0, this.width, this.height);

        // Draw grid
        this.drawGrid();

        // Draw grains
        for (const grain of this.grains) {
            if (!grain.active) continue;

            const x = grain.positionNorm * this.width;
            const y = this.height / 2 - (grain.pitchSemitones / 24) * (this.height / 2);
            const radius = 3 + grain.envelope * 5;
            const alpha = grain.envelope * 0.9 + 0.1;

            ctx.fillStyle = grain.frozen
                ? `rgba(107, 142, 78, ${alpha})`    // Green for frozen
                : `rgba(139, 115, 85, ${alpha})`;   // Brown for live

            ctx.beginPath();
            ctx.arc(x, y, radius, 0, Math.PI * 2);
            ctx.fill();
        }
    }
}
```

### Animation Loop

```javascript
function renderLoop() {
    grainViz.draw();
    euclideanViz.draw();
    requestAnimationFrame(renderLoop);
}
requestAnimationFrame(renderLoop);
```

Data arrives via `emitEventIfBrowserIsVisible` at 30 Hz; rendering runs at 60 FPS via requestAnimationFrame. Smooth because render interpolates/fades using last-known grain state.

---

## 8. Euclidean Circle Visualization (Canvas 2D)

### Design

Circular display showing step distribution. Each step is a dot on a circle. Active pulses are highlighted. Current step has a bright indicator.

### Implementation Pattern

Adapted from O-IntonationPad's pitch circle (SVG → Canvas 2D for consistency with grain scatter):

```javascript
class EuclideanCircleViz {
    constructor(canvas) {
        this.canvas = canvas;
        this.ctx = canvas.getContext('2d');
        this.pattern = new Array(16).fill(false);
        this.currentStep = 0;
        this.steps = 8;
    }

    update(pattern, currentStep, steps) {
        this.pattern = pattern;
        this.currentStep = currentStep;
        this.steps = steps;
    }

    draw() {
        const ctx = this.ctx;
        const cx = this.width / 2;
        const cy = this.height / 2;
        const radius = Math.min(cx, cy) - 15;

        ctx.clearRect(0, 0, this.width, this.height);

        // Draw circle outline
        ctx.strokeStyle = 'rgba(139, 115, 85, 0.3)';
        ctx.lineWidth = 1;
        ctx.beginPath();
        ctx.arc(cx, cy, radius, 0, Math.PI * 2);
        ctx.stroke();

        // Draw steps
        for (let i = 0; i < this.steps; i++) {
            const angle = (i / this.steps) * Math.PI * 2 - Math.PI / 2;
            const x = cx + radius * Math.cos(angle);
            const y = cy + radius * Math.sin(angle);

            const isCurrent = i === this.currentStep;
            const isActive = this.pattern[i];

            ctx.beginPath();
            ctx.arc(x, y, isCurrent ? 8 : 5, 0, Math.PI * 2);

            if (isCurrent && isActive) {
                ctx.fillStyle = '#6B8E4E';  // Bright green
            } else if (isActive) {
                ctx.fillStyle = '#8B7355';  // Brown
            } else {
                ctx.fillStyle = 'rgba(139, 115, 85, 0.2)'; // Faded
            }
            ctx.fill();

            // Connect consecutive active steps with arcs
            if (isActive) {
                ctx.strokeStyle = 'rgba(107, 142, 78, 0.4)';
                ctx.lineWidth = 1.5;
                // Draw arc or line to next active step
            }
        }
    }
}
```

---

## 9. Resource Provider / getResource() Pattern

### Current State

O-GrainScatter serves 3 files:
- `/index.html`
- `/js/juce/index.js`
- `/js/juce/check_native_interop.js`

### Files Needed for Full UI

New files to add to BinaryData and getResource():
- `/css/styles.css` — All CSS
- `/js/app.js` — Main app logic, parameter binding, visualization

**Alternatively:** Single-file approach (all inline in index.html) like O-IntonationPad. Reduces getResource complexity but harder to maintain.

**Decision:** Two-file approach: `index.html` (structure + inline CSS) + `js/app.js` (all logic). Keeps it manageable without excessive BinaryData entries. CSS inline in HTML avoids a separate file/route.

### CMakeLists.txt Update

```cmake
juce_add_binary_data(OuariconGrainScatter_UIResources
    SOURCES
        Source/ui/public/index.html
        Source/ui/public/js/app.js
        Source/ui/public/js/juce/index.js
        Source/ui/public/js/juce/check_native_interop.js
        Source/ui/public/img/botanical.png
)
```

### getResource() Routes to Add

```cpp
if (url == "/js/app.js")
    return Resource{ makeVector(BinaryData::app_js, BinaryData::app_jsSize), "application/javascript" };

if (url == "/img/botanical.png")
    return Resource{ makeVector(BinaryData::botanical_png, BinaryData::botanical_pngSize), "image/png" };
```

---

## 10. Botanical Image Selection

### Plugin Character: Granular Scatter/Stutter

Per aesthetic spec:
- **Effects (delay, modulation)**: Flora, insects, birds — organic, flowing
- **Experimental**: Unusual specimens

**Candidates:**
- `insects/` — butterflies/bugs = geometric, scattered (matches "scatter")
- `flora/` — flowers/plants = organic, generative (matches "grain" metaphor)
- `birds/` — flock scattering = literal scatter metaphor

**Decision:** Insects category preferred — geometric wing patterns mirror the algorithmic scatter of grains. A butterfly with wings spread suggests transformation and dispersal.

---

## 11. Layout Architecture (900x700)

### From CONTEXT.md

```
┌─────────────────────────────────────────────────┐
│  Header: O-GrainScatter title                   │
├───────────────────────────────┬─────────────────┤
│                               │                 │
│  Grain Scatter Viz (Canvas)   │  Euclidean      │
│  ~65% width                   │  Circle (Canvas)│
│                               │  ~35% width     │
│  ~40% height                  │                 │
├───────────────────────────────┴─────────────────┤
│  Group 1: Core Engine  │  Group 2: Pitch/Scale  │
│  6 knobs               │  4 knobs + 1 dropdown  │
│                        │  + 1 dropdown + 1 drop  │
├────────────────────────┼────────────────────────┤
│  Group 3: Beat Sync    │  Group 4: Euclidean    │
│  3 knobs + 1 dropdown  │  2 knobs               │
│  + 1 toggle            │                        │
├────────────────────────┴────────────────────────┤
│  Freeze (prominent toggle with glow)            │
└─────────────────────────────────────────────────┘
```

**Botanical overlay:** Positioned absolute, right side, spanning visualization + controls area, opacity 0.35, pointer-events: none.

---

## 12. Window Size Update

CONTEXT.md specifies 900x700. Current `setSize(800, 500)` in PluginEditor.cpp must be updated.

---

## 13. Pitfalls & Constraints

1. **No viewport units (vh/vw)** — WebView constraint. Use percentages or fixed px.
2. **JUCE 8 callback pattern** — No parameters in `valueChangedEvent.addListener(() => {})`.
3. **sliderDragStarted/Ended** — Must call for proper DAW automation recording.
4. **Retina/HiDPI** — Canvas must scale by `devicePixelRatio` or grains look blurry.
5. **emitEventIfBrowserIsVisible** — Only fires when visible; safe for background tabs.
6. **JSON string size** — 64 grains × ~80 bytes ≈ 5KB per frame at 30Hz. No concern.
7. **Timer vs requestAnimationFrame** — C++ pushes at 30Hz, JS renders at 60 FPS. Decouple data from rendering.
8. **Double-buffer race** — Atomic index flip ensures audio thread never blocks. GUI may read slightly stale data (1 frame) — acceptable for visualization.

---

## 14. Existing Infrastructure Summary

### Already Done (Stage 1)
- 18 relays created in PluginEditor.cpp constructor
- 18 attachments created in PluginEditor.cpp constructor
- WebBrowserComponent with resource provider
- timerCallback at 30 Hz (empty, ready for viz data push)
- getResource() serving index.html + JUCE interop JS
- Window size set (needs update to 900x700)

### Needs Implementation (Stage 3)
1. **C++:** Viz snapshot struct + double buffer in Processor
2. **C++:** Populate snapshot at end of processBlock
3. **C++:** Euclidean current step atomic
4. **C++:** timerCallback emits grainUpdate + euclideanUpdate events
5. **C++:** getResource() routes for new files
6. **C++:** Update setSize to 900x700
7. **CMake:** Add new UI files to BinaryData
8. **HTML:** Full Naturalist-styled UI layout
9. **CSS:** Seed cross-section knobs, toggles, dropdowns, layout
10. **JS:** Knob drag logic, parameter binding for all 18 params
11. **JS:** GrainScatter Canvas 2D visualization
12. **JS:** Euclidean Circle Canvas 2D visualization
13. **JS:** Event listeners for C++ data push
14. **Assets:** Botanical overlay image

---

## References

| Reference | Location |
|-----------|----------|
| O-FreqPulse (Naturalist CSS) | `plugins/O-FreqPulse/Resources/ui/` |
| O-SpectralShaper (emitEvent viz) | `plugins/O-SpectralShaper/Source/PluginEditor.cpp` |
| O-IntonationPad (knob + SVG circle) | `plugins/O-IntonationPad/Source/ui/public/` |
| O-Bells (complex layout) | `plugins/O-Bells/Resources/ui/` |
| Naturalist Aesthetic | `.claude/aesthetics/ouaricon-naturalist-001/aesthetic.md` |
| JUCE WebBrowserComponent | `JUCE/modules/juce_gui_extra/misc/juce_WebBrowserComponent.h` |
| GrainPool DSP | `plugins/O-GrainScatter/Source/dsp/GrainPool.h` |
| EuclideanGenerator | `plugins/O-GrainScatter/Source/dsp/EuclideanGenerator.h` |
