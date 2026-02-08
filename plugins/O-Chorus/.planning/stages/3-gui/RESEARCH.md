# Stage 3: GUI - Research Findings

**Date:** 2026-02-07
**Plugin:** O-Chorus
**Stage:** 3-gui (WebView UI Implementation)

---

## 1. JUCE WebView Slider Integration API

### JavaScript-Side API (`getSliderState`)

The JUCE 8 WebView framework provides `SliderState` objects for bidirectional parameter binding:

```javascript
import * as Juce from "./juce/index.js";

const sliderState = Juce.getSliderState("rate");  // Must match C++ WebSliderRelay name

// Query
sliderState.getNormalisedValue()      // → float [0, 1]
sliderState.getScaledValue()          // → float (actual value, e.g. 1.0 Hz)
sliderState.properties.name           // → "Rate"
sliderState.properties.label          // → "Hz"
sliderState.properties.start          // → 0.05
sliderState.properties.end            // → 5.0
sliderState.properties.skew           // → 0.35 (logarithmic)
sliderState.properties.numSteps       // → int
sliderState.properties.interval       // → float

// Actions
sliderState.setNormalisedValue(0.5)   // Update value (0-1 range)
sliderState.sliderDragStarted()       // Signal gesture begin (for DAW undo)
sliderState.sliderDragEnded()         // Signal gesture end

// Listeners
sliderState.valueChangedEvent.addListener(() => { ... })      // Value changed
sliderState.propertiesChangedEvent.addListener(() => { ... }) // Properties updated
```

### C++ Side (Already Implemented in Stage 1)

O-Chorus already has 7 relays + 7 attachments wired in PluginEditor.cpp:
- `rateRelay` / `rateAttachment`
- `depthRelay` / `depthAttachment`
- `voicesRelay` / `voicesAttachment`
- `widthRelay` / `widthAttachment`
- `toneRelay` / `toneAttachment`
- `mixRelay` / `mixAttachment`
- `driveRelay` / `driveAttachment`

**No C++ changes needed for Stage 3** — only `index.html` replacement + new assets.

### Gesture Support (Critical for DAW Undo/Redo)

Must call `sliderDragStarted()` on mousedown and `sliderDragEnded()` on mouseup:
```javascript
knob.addEventListener('mousedown', (e) => {
  sliderState.sliderDragStarted();
  // ... begin drag
});

document.addEventListener('mouseup', () => {
  if (isDragging) {
    sliderState.sliderDragEnded();
    isDragging = false;
  }
});
```

---

## 2. Established Knob Interaction Pattern

### Vertical Drag (Portfolio Standard)

All Ouaricon plugins use the same interaction model:

```javascript
const SENSITIVITY = 0.005;  // Pixels to normalized value

knob.addEventListener('mousedown', (e) => {
  isDragging = true;
  lastY = e.clientY;
  sliderState.sliderDragStarted();
  e.preventDefault();
});

document.addEventListener('mousemove', (e) => {
  if (!isDragging) return;
  const deltaY = lastY - e.clientY;  // Up = increase
  const current = sliderState.getNormalisedValue();
  const newValue = Math.max(0, Math.min(1, current + deltaY * SENSITIVITY));
  sliderState.setNormalisedValue(newValue);
  lastY = e.clientY;
});

document.addEventListener('mouseup', () => {
  if (isDragging) {
    sliderState.sliderDragEnded();
    isDragging = false;
  }
});
```

### Double-Click to Reset

```javascript
knob.addEventListener('dblclick', (e) => {
  e.preventDefault();
  sliderState.sliderDragStarted();
  sliderState.setNormalisedValue(DEFAULT_VALUES[paramId]);
  sliderState.sliderDragEnded();
});
```

### Rotation Angle Mapping

Standard 270-degree arc:
```javascript
const ANGLE_MIN = -135;  // Bottom-left
const ANGLE_MAX = 135;   // Bottom-right
const ANGLE_RANGE = 270;

function normToAngle(norm) {
  return ANGLE_MIN + norm * ANGLE_RANGE;
}
```

### Sensitivity Values Used in Portfolio

| Plugin | Sensitivity | Feel |
|--------|------------|------|
| O-DigiDelay | `deltaY * 0.5` (on rotation) | Fast |
| O-GrainScatter | `deltaY * 0.005` | Slow/precise |
| O-Comp | `deltaY / 200` (= 0.005) | Medium |
| **O-Chorus (recommended)** | `deltaY * 0.005` | Medium |

---

## 3. Segment Knob CSS Pattern (Naturalist Template)

### 10-Segment Conic Gradient (O-DigiDelay Pattern)

```css
.knob-body {
  border-radius: 50%;
  background: conic-gradient(
    from 0deg,
    #F5DEB3 0deg 36deg,
    #E8D5B7 36deg 72deg,
    #F5DEB3 72deg 108deg,
    #E8D5B7 108deg 144deg,
    #F5DEB3 144deg 180deg,
    #E8D5B7 180deg 216deg,
    #F5DEB3 216deg 252deg,
    #E8D5B7 252deg 288deg,
    #F5DEB3 288deg 324deg,
    #E8D5B7 324deg 360deg
  );
  border: 2px solid #8B7355;
  box-shadow: inset 0 1px 2px rgba(0,0,0,0.15);
}

.knob-indicator {
  position: absolute;
  top: 8px;
  width: 2px;
  height: 16px;
  background: #3C2F2F;
  transform: translateX(-50%);
}
```

### Rotation via `transform: rotate()`

The indicator div rotates with the knob body:
```css
.knob-wrapper {
  transform: rotate(${normToAngle(value)}deg);
}
```

---

## 4. Ouaricon Naturalist Color Palette

| Element | Color | Hex |
|---------|-------|-----|
| Paper background | Warm cream | `#F5E6D3` |
| Knob segment 1 | Wheat | `#F5DEB3` |
| Knob segment 2 | Tan | `#E8D5B7` |
| Knob border | Brown | `#8B7355` |
| Text / labels | Dark brown | `#3C2F2F` |
| Active accent | Sage green | `#6B8E4E` |
| LFO ring / vine | Muted sage | `#5a7a6a` |
| Botanical accent | Dark green | `#3C5C1A` |
| Plugin border | Brown | `#8B7355` |
| Preset bar green | Botanical | `#8BA870` |

### Typography

- **Font:** `'EB Garamond', 'Garamond', 'Georgia', serif`
- **Labels:** Uppercase, letter-spacing 1-2px, `#3C2F2F`
- **Values:** Normal case, lighter weight

---

## 5. LFO Ring Animation

### Approach: JS-Driven SVG Animation

From the discuss phase, the LFO indicator will be an SVG circle with animated `stroke-dasharray`. This pattern exists in O-Comp's vine-arc knobs:

```javascript
// SVG circle with animated dash
const ARC_LENGTH = 138.23;  // 2 * PI * radius

function animateLFO(rateNorm, depthNorm) {
  const rateHz = /* denormalize rate */;
  const period = 1000 / rateHz;  // ms per cycle

  // Rotation animation
  lfoRing.style.transition = `transform ${period}ms linear`;
  lfoRing.style.transform = `rotate(${currentAngle}deg)`;

  // Pulse scale from depth
  const scale = 1.0 + depthNorm * 0.15;
  lfoGlow.style.transform = `scale(${scale})`;
}
```

### Recommended Implementation: `requestAnimationFrame` Loop

More reliable than CSS transitions for continuous rotation:

```javascript
let lfoPhase = 0;

function lfoAnimationLoop() {
  const rateHz = /* get from rate slider state */;
  const depthNorm = /* get from depth slider state */;

  // Advance phase
  lfoPhase += (rateHz * 2 * Math.PI) / 60;  // 60fps assumed
  if (lfoPhase > 2 * Math.PI) lfoPhase -= 2 * Math.PI;

  // Dot position on circle
  const x = centerX + radius * Math.cos(lfoPhase);
  const y = centerY + radius * Math.sin(lfoPhase);
  lfoDot.setAttribute('cx', x);
  lfoDot.setAttribute('cy', y);

  // Pulse/glow from depth
  const pulse = 1 + Math.sin(lfoPhase) * depthNorm * 0.2;
  lfoRing.style.opacity = 0.3 + pulse * 0.3;

  requestAnimationFrame(lfoAnimationLoop);
}
```

### Color: Sage Green `#5a7a6a`

Matching the vine-arc stroke from O-Comp.

---

## 6. Voices Knob: Stepped Integer Snapping

The `voices` parameter is `AudioParameterInt` (1-8). The slider state automatically handles stepping via `properties.numSteps` and `properties.interval`.

However, the visual knob must snap to 8 discrete positions:

```javascript
function setupVoicesKnob(state) {
  // 8 steps across 270-degree range
  const STEPS = 8;

  state.valueChangedEvent.addListener(() => {
    const norm = state.getNormalisedValue();
    // Snap to nearest step for visual
    const step = Math.round(norm * (STEPS - 1));
    const snappedNorm = step / (STEPS - 1);
    const angle = ANGLE_MIN + snappedNorm * ANGLE_RANGE;
    voicesIndicator.style.transform = `translateX(-50%) rotate(${angle}deg)`;
    voicesValue.textContent = Math.round(state.getScaledValue());
  });
}
```

---

## 7. Asset Requirements

### Paper Texture (Reuse)

`paper1.jpg` exists in O-DigiDelay and O-AnalogSaturation (169KB). Copy to O-Chorus:
```
plugins/O-DigiDelay/Source/ui/public/img/paper1.jpg
→ plugins/O-Chorus/Source/ui/public/img/paper1.jpg
```

### Botanical Overlay

Available overlays in portfolio:
- `butterfly2_Black and white.png` (O-DigiDelay)
- `botanical.png` (O-Bass)
- `flora.png` (O-SimpleReverb)
- `botanical-bug.png` (O-Polystutter)
- `shell.png` (O-IntonationPad)

**Recommendation:** Use butterfly overlay at ~15-20% opacity (matches O-DigiDelay's proven aesthetic) or create without botanical for a cleaner look (700x250 is narrow, overlay may feel cramped).

### Google Fonts

EB Garamond can be embedded via `@font-face` from a local WOFF2 file, or use system Garamond/Georgia as fallback (no network in plugin WebViews).

**Recommendation:** Use system fallback stack `'Garamond', 'Georgia', serif` — consistent with O-DigiDelay which does NOT embed custom fonts.

---

## 8. BinaryData / CMakeLists.txt Changes

### Files to Add

```cmake
juce_add_binary_data(OuariconChorus_UIResources
    SOURCES
        Source/ui/public/index.html          # Updated (full GUI)
        Source/ui/public/js/juce/index.js    # Existing
        Source/ui/public/js/juce/check_native_interop.js  # Existing
        Source/ui/public/img/paper1.jpg      # New (copy from O-DigiDelay)
)
```

### Resource Provider Updates Needed

`PluginEditor.cpp::getResource()` must map new asset URLs:

```cpp
if (url == "/img/paper1.jpg")
    return makeResource(BinaryData::paper1_jpg, BinaryData::paper1_jpgSize, "image/jpeg");
```

### PluginEditor.cpp: Window Size Change

```cpp
// Change from 600x400 to 700x250
setSize(700, 250);
```

---

## 9. Value Display Formatting

### Parameter Display Strings

| Parameter | Formatter | Example |
|-----------|-----------|---------|
| Rate | `${value.toFixed(2)} Hz` | "1.00 Hz" |
| Depth | `${Math.round(value * 100)}%` | "50%" |
| Voices | `${Math.round(value)}` | "4" |
| Width | `${Math.round(value * 100)}%` | "70%" |
| Tone | `${value > 0 ? '+' : ''}${Math.round(value * 100)}%` | "+0%" or "-50%" |
| Mix | `${Math.round(value * 100)}%` | "50%" |
| Drive | `${Math.round(value * 100)}%` | "30%" |

### Getting Actual Values

Use `sliderState.getScaledValue()` for display, `sliderState.getNormalisedValue()` for knob position:

```javascript
rateState.valueChangedEvent.addListener(() => {
  const hz = rateState.getScaledValue();
  rateValueEl.textContent = hz.toFixed(2) + ' Hz';
});
```

---

## 10. Cross-Platform Considerations

### URL Scheme

Already handled by existing interop layer:
- macOS: `juce://juce.backend/`
- Windows: `https://juce.backend/`
- Use `getResourceProviderRoot()` in C++ (already done)

### WebView2 Windows Support

Already configured in CMakeLists.txt:
- `NEEDS_WEBVIEW2 TRUE`
- `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`
- `withUserDataFolder()` set to temp directory

### No Network Access

Plugin WebViews cannot fetch external resources. All assets must be in BinaryData:
- No Google Fonts CDN
- No external images
- No fetch() calls to APIs

---

## 11. Pitfalls to Avoid

### From Portfolio Experience

1. **BinaryData name flattening** — `js/juce/index.js` becomes `BinaryData::index_js`, not `BinaryData::js_juce_index_js`. Map URLs to flattened names explicitly.

2. **MIME types matter** — Wrong MIME type causes silent failure. Always use `text/html`, `application/javascript`, `image/jpeg`, `image/png`.

3. **Viewport meta tag** — Use `<meta name="viewport" content="width=700, height=250">` matching the actual window size, NOT device-width.

4. **No vh/vw units** — WebView viewport can be unreliable. Use `height: 100%` on html/body and pixel dimensions for layout.

5. **Module script type** — JUCE interop uses `<script type="module">`. Must match.

6. **Event listener cleanup** — Document-level mousemove/mouseup handlers are shared. Use a single global drag state object to avoid conflicts between knobs.

7. **Double-click prevention** — `e.preventDefault()` on mousedown to avoid text selection during drag.

8. **User-select: none** — Required on body to prevent text selection during knob interaction.

---

## 12. Summary: Implementation Scope

### What Changes

| Component | Action |
|-----------|--------|
| `index.html` | **Replace** — Full GUI with 7 knobs, LFO ring, Naturalist styling |
| `img/paper1.jpg` | **Add** — Copy from O-DigiDelay |
| `CMakeLists.txt` | **Edit** — Add `paper1.jpg` to binary data sources |
| `PluginEditor.cpp` | **Edit** — Add paper1.jpg resource route, change setSize to 700x250 |

### What Stays the Same

| Component | Status |
|-----------|--------|
| `PluginProcessor.h/.cpp` | No changes |
| `DSP/ChorusEngine.h/.cpp` | No changes |
| `PluginEditor.h` | No changes |
| `js/juce/index.js` | No changes |
| `js/juce/check_native_interop.js` | No changes |
| All relay/attachment wiring | Already complete |

### Estimated Complexity

- **index.html:** ~300-400 lines (CSS + HTML + JS inline)
- **CMakeLists.txt:** +1 line (paper1.jpg)
- **PluginEditor.cpp:** +5 lines (resource route + size change)
- **Total:** ~4 files modified, ~1 file copied

---

*Research complete. Ready for plan phase.*
