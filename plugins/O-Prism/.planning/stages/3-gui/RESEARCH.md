# Stage 3: GUI - Research

## Research Date

2026-02-17

## Scope

Investigate implementation approach for O-Prism Stage 3 (GUI): WebView-based 3-tab UI at 1200x800, Ouaricon Naturalist aesthetic, 73 slider + 1 toggle parameter bindings, wavetable Canvas display, tuning panel integration, and two bug fixes from Stage 2 carry-over.

---

## 1. Existing Ouaricon UI Patterns (Reuse Opportunities)

### 1.1 Ouaricon Naturalist Design System

**Color tokens** (consistent across all plugins):

| Token | Value | Usage |
|-------|-------|-------|
| Paper background | `#F5E6D3` | Main body background |
| Paper mid | `#EBD9C7` / `#D4C4B0` | Secondary surfaces, tab bar |
| Text primary | `#3C2F2F` | Labels, values |
| Text secondary | `#8B7355` | Section headers, muted labels |
| Border/line | `#8B7355` | Section borders, knob outlines |
| Active accent | `#6B8E4E` | Active states, botanical green |
| Amber accent | `#8B6914` | Knob indicator, highlights |
| Knob wheat A | `#F5DEB3` | Knob conic gradient segment |
| Knob wheat B | `#E8D5B7` | Alternating knob segment |
| Knob cream | `#FFF8DC` | Knob center highlight |
| Amber light | `#C9A27B` | Knob outer ring highlight |

**Typography:** `'Garamond', 'Georgia', 'Times New Roman', serif` throughout. Section headers: `font-size: 8-11px; text-transform: uppercase; letter-spacing: 1.5-2px; color: #8B7355`.

**No standalone CSS file to import** — each plugin embeds styles inline in `index.html`. The `ouaricon-naturalist.css` file in O-TextureForge is specific to that plugin's scatter plot UI. For O-Prism, define all styles inline in `index.html` using the shared design tokens above.

### 1.2 Tab Layout Pattern (from O-Bells, O-Marimba)

Both plugins implement tabs with the same CSS/JS pattern:

**CSS:**
```css
.tab-bar {
    display: flex;
    background: #D4C4B0;
    border-bottom: 2px solid #8B7355;
}
.tab {
    flex: 1;
    text-align: center;
    padding: 10px;
    cursor: pointer;
    text-transform: uppercase;
    letter-spacing: 1.5px;
    font-size: 13px;
    color: #5C4033;
    border-right: 1px solid #8B7355;
}
.tab.active {
    background: #F5E6D3;
    color: #3C2F2F;
    border-bottom: 2px solid #F5E6D3;
    margin-bottom: -2px;
}
.tab-content { display: none; }
.tab-content.active { display: block; }
```

**JS (O-Marimba `switchTab()` pattern):**
```javascript
function switchTab(tabName) {
    document.querySelectorAll('.tab-content').forEach(c => c.classList.remove('active'));
    document.querySelectorAll('.tab').forEach(t => t.classList.remove('active'));
    document.getElementById(tabName + '-tab').classList.add('active');
    document.querySelector(`[data-tab="${tabName}"]`).classList.add('active');
    // Shift botanical overlay
    botanical.className = 'botanical-overlay';
    if (tabName !== 'synth') botanical.classList.add(tabName + '-position');
}
```

**Recommendation:** Adopt the O-Marimba pattern (3-tab: SYNTH/TUNING/EFFECTS, explicit `switchTab()` function, botanical class toggling).

### 1.3 Seed Cross-Section Knob (CSS-only)

All Ouaricon plugins use the same CSS knob design — a `conic-gradient` with 20 alternating wheat-colored segments that resembles a botanical seed cross-section.

```css
.knob {
    width: 50px; height: 50px;
    border-radius: 50%;
    border: 2px solid #8B7355;
    background:
        radial-gradient(circle, transparent 84%, #C9A27B 84%, #C9A27B 88%,
            #8B7355 88%, #8B7355 90%, transparent 90%),
        conic-gradient(from 0deg,
            #F5DEB3 0deg, #F5DEB3 18deg, #8B7355 18deg, #8B7355 19deg,
            #E8D5B7 19deg, #E8D5B7 36deg, #8B7355 36deg, #8B7355 37deg,
            /* repeats 20 segments */),
        radial-gradient(circle, #FFF8DC 0%, #FFF8DC 20%, transparent 20%);
}
.knob-indicator {
    position: absolute;
    width: 3px; height: 14px;
    background: #8B6914;
    top: 4px; left: 50%; margin-left: -1.5px;
    transform-origin: center 21px;
    transform: rotate(-135deg);  /* min position */
}
```

Rotation range: `-135deg` (min) to `+135deg` (max) = 270 degrees of travel.

**JS knob interaction** (O-Marimba / O-TextureForge pattern):
```javascript
function makeKnobDraggable(knobEl, state, onUpdate) {
    let isDragging = false;
    knobEl.addEventListener('mousedown', (e) => {
        isDragging = true;
        state.sliderDragStarted();
        e.preventDefault();
    });
    document.addEventListener('mouseup', () => {
        if (isDragging) { isDragging = false; state.sliderDragEnded(); }
    });
    document.addEventListener('mousemove', (e) => {
        if (!isDragging) return;
        const norm = Math.max(0, Math.min(1, state.getNormalisedValue() - e.movementY * 0.005));
        state.setNormalisedValue(norm);
    });
    state.valueChangedEvent.addListener(() => onUpdate(state.getNormalisedValue()));
    onUpdate(state.getNormalisedValue());
}
```

**At 50px, O-Prism can fit ~12-14 knobs per row** within the 1200px width (accounting for labels and spacing).

### 1.4 Botanical Watermark Pattern

All plugins use the same approach — an absolutely-positioned image at low opacity with CSS classes that shift position per tab.

```css
.botanical-overlay {
    position: absolute;
    right: -30px;
    top: 50%;
    transform: translateY(-50%);
    height: 71%;
    opacity: 0.20;
    pointer-events: none;
    z-index: 1;
    transition: right 0.4s ease-out, opacity 0.4s ease-out;
}
.botanical-overlay.tuning-position { right: -80px; opacity: 0.15; }
.botanical-overlay.effects-position { right: -180px; opacity: 0.12; }
```

O-Prism will use the **snow bunting specimen image** provided by the user. The image file must be:
1. Added to `Source/ui/public/images/` (or similar)
2. Added to `juce_add_binary_data` in CMakeLists.txt
3. Served via the resource provider
4. Referenced as `<img src="/images/snow-bunting.png">`

### 1.5 JS Architecture — Parameter Binding

**JUCE 8 WebView bridge APIs** used across all plugins:

```javascript
import * as Juce from './js/juce/index.js';

// Float/Int parameters (via WebSliderRelay):
const state = Juce.getSliderState('paramId');
state.getNormalisedValue()           // 0.0-1.0
state.setNormalisedValue(0.75)
state.sliderDragStarted()           // begin automation gesture
state.sliderDragEnded()             // end automation gesture
state.valueChangedEvent.addListener(() => { ... })  // C++ -> JS

// Choice parameters (same relay, use normalized mapping or ComboBox):
// Choice params registered as WebSliderRelay work via normalized 0-1 mapping
// For discrete display: denormalize = Math.round(norm * (numChoices - 1))

// Toggle parameters (via WebToggleButtonRelay):
const toggle = Juce.getToggleState('paramId');
toggle.getToggleState()              // bool
toggle.setToggleState(true)
toggle.toggleStateChangedEvent.addListener(() => { ... })

// Native function calls (async):
const fn = Juce.getNativeFunction('functionName');
const result = await fn(arg1, arg2);
```

**Helper pattern** (from O-Lyrica — recommended for O-Prism's 73 params):

```javascript
function bindKnob(paramId, defaultNorm = 0.5) {
    const el = document.getElementById(`knob-${paramId}`);
    const indicator = el.querySelector('.knob-indicator');
    const valueLabel = document.getElementById(`val-${paramId}`);
    const state = Juce.getSliderState(paramId);

    function updateDisplay(norm) {
        const angle = -135 + norm * 270;
        indicator.style.transform = `rotate(${angle}deg)`;
        if (valueLabel) valueLabel.textContent = formatValue(paramId, norm);
    }

    makeKnobDraggable(el, state, updateDisplay);

    el.addEventListener('dblclick', () => state.setNormalisedValue(defaultNorm));
}
```

This scales to 73 parameters — call `bindKnob('oscAPos')` for each.

---

## 2. Tuning Panel Integration

### 2.1 Module Assets

| Asset | Source Path |
|-------|------------|
| TuningPanel JS | `modules/tuning/scala-tuning-engine/js/tuning-panel.js` |
| TuningPanel CSS | `modules/tuning/scala-tuning-engine/snippets/tuning-panel.css` |
| Integration checklist | `modules/tuning/scala-tuning-engine/snippets/INTEGRATION-CHECKLIST.md` |

### 2.2 Integration Pattern (from O-Bells)

1. Copy `tuning-panel.js` and `tuning-panel.css` into O-Prism's UI resources
2. Add both to `juce_add_binary_data` in CMakeLists.txt
3. Serve via resource provider (add URL mappings)
4. HTML container in tuning tab: `<div id="tuning-container"></div>`
5. Initialize in JS:

```javascript
const { TuningPanel } = await import('./js/tuning-panel.js');
const tuningPanel = new TuningPanel(
    document.getElementById('tuning-container'), Juce);
await tuningPanel.init();
```

6. Override CSS variables to match Naturalist aesthetic (tuning panel has its own theming variables — already Naturalist-compatible with defaults)

### 2.3 Already Implemented in O-Prism

O-Prism already has all 23 native tuning functions registered in `PluginEditor.cpp`:
- Tuning data (get/set intervals, name, single interval)
- Tonic/rotation
- Octave stretch, master tune
- Temperament presets
- File I/O (Scala, KBM, HTML export)
- Scale generators (EDO, harmonic, rank-2)
- Embedded tuning library (list, categories, load)

**Missing native functions** (compared to integration checklist):
- `applyGeneratedScale` — needed for scale generator results to take effect
- `saveScalaFile` / `saveKBMFile` — needed for file export

These should be added during implementation if the tuning panel calls them.

---

## 3. Wavetable Canvas Display

### 3.1 Existing Canvas Patterns in Codebase

| Plugin | Visualization | Technology | Data Flow |
|--------|--------------|------------|-----------|
| O-GrainScatter | Grain scatter plot | Canvas 2D | `emitEventIfBrowserIsVisible` at 30Hz |
| O-SpectralShaper | Spectral curve editor | Canvas 2D | `evaluateJavascript` at 60Hz |
| O-MultiBandCompressor | Spectrum analyzer | Canvas 2D | `evaluateJavascript` push |
| O-Marimba | Waveform display | SVG path | `getNativeFunction` polling at rAF |
| O-TextureForge | Corpus scatter | WebGL (regl) | `emitEventIfBrowserIsVisible` |

### 3.2 WavetableData Structure

From `Source/dsp/WavetableData.h`:
```cpp
struct WavetableData {
    static constexpr int kTableSize = 2048;
    static constexpr int kGuardSamples = 1;
    static constexpr int kFrameSize = 2049;  // kTableSize + guard
    static constexpr int kMaxFrames = 256;
    static constexpr int kNumMipmapLevels = 10;
    int numFrames = 0;
    std::vector<float> data;  // flat: [level][frame][sample+guard]
    // index = (level * numFrames + frame) * kFrameSize + sampleIndex
};
```

For display: read level 0 (full resolution), specific frame. 2048 float samples in ~[-1, +1].

### 3.3 Factory Wavetable Shapes

From `Source/dsp/WavetableGenerator.h`:
```cpp
enum class WaveShape { Saw=0, Square=1, Triangle=2, Sine=3 };
```
All factory shapes are single-frame (`numFrames=1`). User-imported wavetables can have up to 256 frames.

### 3.4 Recommended Implementation

**Data flow:** On-demand pull (not continuous push).

Per the CONTEXT.md decision: wavetable display updates on **position parameter change only**, not in real-time during playback. This means:

1. **Add native functions to PluginEditor.cpp:**
   - `getWavetableFrame(oscId, frameIndex)` — returns 256 downsampled float values as JSON
   - `getWavetableInfo(oscId)` — returns `{numFrames, shapeName}`
   - `getWavetableFrameForPosition(oscId, normalizedPos)` — convenience: converts position 0-1 to frame index

2. **JS Canvas class:**

```javascript
class WavetableDisplay {
    constructor(canvasId) {
        this.canvas = document.getElementById(canvasId);
        this.ctx = this.canvas.getContext('2d');
        this.samples = [];
        this.resizeCanvas();
    }

    resizeCanvas() {
        const dpr = window.devicePixelRatio || 1;
        const rect = this.canvas.getBoundingClientRect();
        this.w = rect.width;
        this.h = rect.height;
        this.canvas.width = this.w * dpr;
        this.canvas.height = this.h * dpr;
        this.ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
    }

    draw() {
        const { ctx, w, h, samples } = this;
        if (!samples.length) return;
        const mid = h / 2;

        ctx.clearRect(0, 0, w, h);

        // Aged paper background (matches Naturalist)
        ctx.fillStyle = '#EBD9C7';
        ctx.fillRect(0, 0, w, h);

        // Center line
        ctx.strokeStyle = 'rgba(139, 115, 85, 0.3)';
        ctx.lineWidth = 1;
        ctx.beginPath();
        ctx.moveTo(0, mid);
        ctx.lineTo(w, mid);
        ctx.stroke();

        // Waveform fill (botanical amber gradient)
        const gradient = ctx.createLinearGradient(0, 0, 0, h);
        gradient.addColorStop(0, 'rgba(139, 105, 20, 0.3)');
        gradient.addColorStop(0.5, 'rgba(139, 105, 20, 0.08)');
        gradient.addColorStop(1, 'rgba(139, 105, 20, 0.0)');

        ctx.beginPath();
        for (let i = 0; i < samples.length; i++) {
            const x = (i / (samples.length - 1)) * w;
            const y = mid - samples[i] * mid * 0.85;
            i === 0 ? ctx.moveTo(x, y) : ctx.lineTo(x, y);
        }
        ctx.lineTo(w, mid);
        ctx.lineTo(0, mid);
        ctx.closePath();
        ctx.fillStyle = gradient;
        ctx.fill();

        // Waveform stroke (brown ink)
        ctx.beginPath();
        for (let i = 0; i < samples.length; i++) {
            const x = (i / (samples.length - 1)) * w;
            const y = mid - samples[i] * mid * 0.85;
            i === 0 ? ctx.moveTo(x, y) : ctx.lineTo(x, y);
        }
        ctx.strokeStyle = '#5C4033';
        ctx.lineWidth = 1.5;
        ctx.stroke();
    }
}
```

3. **Trigger updates:** Listen for `oscAPos` / `oscBPos` slider `valueChangedEvent`. On change, fetch frame data and redraw. Also refetch on wavetable shape/table change.

**Downsampling:** Send 256 points (stride=8 over 2048 samples). At typical canvas width of ~350px, this provides ~1.4 samples per pixel — sufficient quality with ~2KB JSON per fetch.

**DPI handling:** Always multiply canvas dimensions by `devicePixelRatio` (proven pattern across O-GrainScatter, O-SpectralShaper).

**Styling as "specimen illustration":** Brown border (`2px solid #8B7355`), aged paper background (`#EBD9C7`), brown waveform stroke — consistent with Naturalist aesthetic.

---

## 4. Bug Fixes Required

### 4.1 Filter Type Parameter — Add BP24

**File:** `Source/PluginProcessor.cpp` lines 159-160 and 182-183

**Current (6 choices):**
```cpp
juce::StringArray { "LP12", "LP24", "HP12", "HP24", "BP", "Notch" }
```

**Fix (7 choices):**
```cpp
juce::StringArray { "LP12", "LP24", "HP12", "HP24", "BP12", "BP24", "Notch" }
```

Also rename "BP" to "BP12" for clarity. DSP code already handles 7 types (indices 0-6).

**Impact:** Fixes the issue where selecting "Notch" (index 5) was actually BP24 (type 5) in SVFFilter. After fix: BP12=4, BP24=5, Notch=6 — matches SVFFilter enum.

### 4.2 numSliderParams Count

**File:** `Source/PluginEditor.h` line 65

**Current:** `static constexpr int numSliderParams = 67;`

**Fix:** Count the actual `sliderParamIds` array entries:
- Osc A: 10
- Osc B: 10
- Sub + Noise: 5
- Amp Env: 4
- Filter Env: 5
- Filter A: 5
- Filter B: 5
- Filter Routing: 1
- Tuning: 7
- Reverb: 4
- Delay: 4
- Chorus: 3
- Distortion: 3
- EQ: 4
- Global: 3
- **Total: 73**

**Fix:** `static constexpr int numSliderParams = 73;`

---

## 5. Resource Provider URL Mapping

### 5.1 Current State

The resource provider in `PluginEditor.cpp` only maps 3 files:
- `/` and `/index.html` → `index_html`
- `/js/juce/index.js` → `index_js`
- `/js/juce/check_native_interop.js` → `check_native_interop_js`

### 5.2 Files to Add

For the complete Stage 3 GUI, the resource provider needs additional mappings:

| URL Path | BinaryData Name | Purpose |
|----------|----------------|---------|
| `/js/tuning-panel.js` | `tuning_panel_js` | TuningPanel module |
| `/css/tuning-panel.css` | `tuning_panel_css` | TuningPanel styles |
| `/images/snow-bunting.png` | `snow_bunting_png` | Botanical watermark |

**Note:** JUCE BinaryData mangles filenames: hyphens → underscores, dots → underscores (except the last one which defines the variable suffix). Verify exact names after build.

### 5.3 CMakeLists.txt Updates

```cmake
juce_add_binary_data(O-Prism_UIResources
    SOURCES
        Source/ui/public/index.html
        Source/ui/public/js/juce/index.js
        Source/ui/public/js/juce/check_native_interop.js
        Source/ui/public/js/tuning-panel.js
        Source/ui/public/css/tuning-panel.css
        Source/ui/public/images/snow-bunting.png
)
```

---

## 6. Cross-Platform WebView Considerations

### 6.1 URL Scheme

Already handled — `webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot())` is used in PluginEditor.cpp. Resource provider works on both macOS (WebKit, `juce://`) and Windows (WebView2, `https://`).

### 6.2 Windows WebView2

Already configured correctly:
- `NEEDS_WEBVIEW2 TRUE` in CMakeLists.txt
- `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` in compile definitions
- User data folder set to temp directory

### 6.3 CSS/JS Compatibility

- Use standard CSS (no `-webkit-` prefixes needed — both WebKit and Chromium/WebView2 support standard properties)
- Use ES modules (`import`/`export`) — supported by both backends
- Canvas 2D — universally supported
- `conic-gradient` — supported in Safari 12.1+ and Chrome 69+ (both WebKit and WebView2 are well past these versions)

---

## 7. Implementation Architecture

### 7.1 File Structure (After Stage 3)

```
Source/ui/public/
├── index.html                    # Complete UI (styles + layout + JS inline)
├── js/
│   ├── juce/
│   │   ├── index.js             # JUCE bridge (existing)
│   │   └── check_native_interop.js  # Interop check (existing)
│   └── tuning-panel.js          # Tuning module (copy from module)
├── css/
│   └── tuning-panel.css         # Tuning panel styles (copy from module)
└── images/
    └── snow-bunting.png         # Botanical specimen
```

### 7.2 HTML Architecture

Single `index.html` with inline styles and module script:

```
<html>
<head>
    <style> /* All Naturalist styles + knob CSS + layout CSS */ </style>
    <link rel="stylesheet" href="/css/tuning-panel.css">
</head>
<body>
    <!-- Header bar: branding, logo -->
    <div class="header-bar">...</div>

    <!-- Tab bar: SYNTH | TUNING | EFFECTS -->
    <div class="tab-bar">...</div>

    <!-- Botanical watermark -->
    <img class="botanical-overlay" src="/images/snow-bunting.png">

    <!-- Tab content: SYNTH -->
    <div id="synth-tab" class="tab-content active">
        <!-- Osc A section with Canvas display -->
        <!-- Osc B section with Canvas display -->
        <!-- Sub + Noise row -->
        <!-- Osc Mix -->
        <!-- Filter A + Filter B -->
        <!-- Filter routing toggle -->
        <!-- Amp Envelope + Filter Envelope -->
    </div>

    <!-- Tab content: TUNING -->
    <div id="tuning-tab" class="tab-content">
        <div id="tuning-container"></div>
    </div>

    <!-- Tab content: EFFECTS -->
    <div id="effects-tab" class="tab-content">
        <!-- Sub-tab bar: Reverb | Delay | Chorus | Distortion | EQ -->
        <!-- Effect panels -->
    </div>

    <!-- Persistent footer: Master Vol, Osc Mix, Polyphony -->
    <div class="footer-bar">...</div>

    <script type="module">
        import * as Juce from './js/juce/index.js';
        // Initialize all 73 knob bindings
        // Initialize tuning panel
        // Initialize wavetable displays
        // Tab switching logic
    </script>
</body>
</html>
```

### 7.3 Parameter Binding Strategy

With 73 slider parameters, use a data-driven approach:

```javascript
// Define all parameters with display formatting
const PARAMS = {
    oscAPos:     { label: 'Position', format: (n) => (n * 100).toFixed(0) + '%' },
    oscALevel:   { label: 'Level',    format: (n) => (n * 100).toFixed(0) + '%' },
    oscACoarse:  { label: 'Coarse',   format: (n) => Math.round(n * 48 - 24) + 'st' },
    // ... 70 more
};

// Batch bind all knobs
for (const [id, config] of Object.entries(PARAMS)) {
    const knob = document.getElementById(`knob-${id}`);
    if (knob) bindKnob(id, config);
}
```

This avoids 73 individual binding blocks and keeps the JS maintainable.

### 7.4 Choice Parameters via Slider Relays

O-Prism registers choice parameters (`filtAType`, `subShape`, `noiseType`, `distType`, `filtRouting`, `glideMode`, `delayMode`, `tuningPreset`, `tonic`) as slider relays (not combo box relays). This means:

- JS receives normalized 0-1 values
- Must denormalize: `choiceIndex = Math.round(norm * (numChoices - 1))`
- Display as dropdown/button group in UI, but bridge via slider state

For dropdowns:
```javascript
function bindDropdown(selectEl, paramId, numChoices) {
    const state = Juce.getSliderState(paramId);
    selectEl.addEventListener('change', () => {
        state.sliderDragStarted();
        state.setNormalisedValue(selectEl.selectedIndex / (numChoices - 1));
        state.sliderDragEnded();
    });
    state.valueChangedEvent.addListener(() => {
        selectEl.selectedIndex = Math.round(state.getNormalisedValue() * (numChoices - 1));
    });
    selectEl.selectedIndex = Math.round(state.getNormalisedValue() * (numChoices - 1));
}
```

---

## 8. Performance Considerations

### 8.1 WebView Rendering

- 1200x800 is large for WebView but well within capability
- Canvas waveform displays (2 canvases, ~350x120 each) are lightweight
- No continuous animation needed — redraws only on parameter changes
- `requestAnimationFrame` loop **not needed** — use event-driven redraws only

### 8.2 Parameter Update Batching

With 73 parameters, initial page load will fire 73 `valueChangedEvent` callbacks. Use `requestAnimationFrame` debouncing for DOM updates:

```javascript
let pendingUpdates = new Set();
function scheduleUpdate(paramId) {
    pendingUpdates.add(paramId);
    if (pendingUpdates.size === 1) {
        requestAnimationFrame(() => {
            for (const id of pendingUpdates) updateDisplay(id);
            pendingUpdates.clear();
        });
    }
}
```

### 8.3 Memory

- Inline HTML with all styles + JS = single file, single resource provider fetch
- Tuning panel + CSS as separate files (lazy-loaded on tuning tab visit is possible but not necessary)
- Snow bunting PNG: keep under 500KB (compress if larger)

---

## 9. Pitfalls and Gotchas

### 9.1 JUCE WebView

| Pitfall | Mitigation |
|---------|-----------|
| `valueChangedEvent` doesn't pass value | Always call `state.getNormalisedValue()` inside listener |
| Choice params via slider relay need denormalization | Use `Math.round(norm * (n-1))` |
| CSS `100vh` doesn't work in embedded WebView | Use `height: 100%` on html/body, `overflow: hidden` |
| BinaryData name mangling | Verify exact names in generated `BinaryData.h` after build |
| Resource provider 404s show blank page | Add catch-all logging in `getResource()` for debugging |

### 9.2 Layout

| Pitfall | Mitigation |
|---------|-----------|
| 46 params on SYNTH tab = visual density | Use clear section headers, consistent spacing, logical grouping |
| Knob labels must be readable at 50px | 9px font, uppercase, truncate if needed |
| Footer must persist across tabs | Use `position: absolute; bottom: 0` outside tab-content containers |

### 9.3 Canvas

| Pitfall | Mitigation |
|---------|-----------|
| Blurry canvas on Retina | Always multiply by `devicePixelRatio` |
| Canvas resize on window resize | Not applicable — fixed 1200x800, but handle DPI changes |
| Empty wavetable data | Check `samples.length > 0` before drawing |

---

## 10. Summary of Research Findings

| Finding | Recommendation |
|---------|---------------|
| Ouaricon Naturalist is inline CSS, not a shared file | Define all styles inline in index.html |
| Tab pattern proven in O-Bells/O-Marimba | Adopt O-Marimba switchTab() pattern |
| Seed cross-section knob is CSS conic-gradient | Reuse exact CSS from O-Bells |
| Botanical watermark shifts per tab | CSS classes with transition animation |
| 73 params need batch binding approach | Data-driven PARAMS object + loop |
| Choice params registered as slider relays | Denormalize with `Math.round(norm * (n-1))` |
| Tuning panel module ready to integrate | Copy JS + CSS, init TuningPanel class |
| Canvas waveform is event-driven, not real-time | Fetch frame data on position change only |
| Two Stage 2 bugs to fix | BP24 filter choice + numSliderParams=73 |
| Cross-platform WebView already configured | No additional work needed |
| Native functions for wavetable display | Need to add getWavetableFrame, getWavetableInfo |

---

## 11. Phase Breakdown (from CONTEXT.md, refined)

### Phase 3.1: Layout + Styling + Bug Fixes
- Fix numSliderParams (67 → 73) and filter type BP24
- Complete index.html with all CSS (Naturalist tokens, knob, tabs, sections)
- 3-tab HTML structure (SYNTH | TUNING | EFFECTS)
- Header bar, footer bar (persistent master strip)
- Snow bunting botanical watermark with tab-shift CSS
- CMakeLists.txt: add all new binary resources
- Resource provider: add URL mappings for new files
- Tab switching JS
- **Deliverables:** Styled layout rendering in WebView, tabs working, botanical visible

### Phase 3.2: Parameter Binding + Tuning Panel
- Data-driven bindKnob() for all 73 slider params
- bindDropdown() for all choice params
- delaySync toggle binding
- Value display formatters per parameter type
- Tuning panel integration (copy module, init, container in tuning tab)
- Effects sub-tab switching
- Double-click reset on knobs
- **Deliverables:** All params controllable from UI, tuning panel functional

### Phase 3.3: Wavetable Display + Polish
- Add native functions: getWavetableFrame, getWavetableInfo
- WavetableDisplay Canvas class (2 instances: Osc A, Osc B)
- Waveform rendering with Naturalist styling (brown ink, aged paper)
- Position indicator / frame counter overlay
- Wavetable selector dropdown per oscillator
- Hover effects, focus states, visual polish
- Cross-platform testing notes
- **Deliverables:** Complete, polished GUI, build passing, pluginval passing
