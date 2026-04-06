# Stage 3: GUI - Research

**Date:** 2026-04-05
**Phase:** 3.1 (Layout + Controls + Parameter Binding)
**Requirements:** UI-01 (must), UI-03 (nice)

---

## 1. WebView Relay Pattern (Proven)

### Member Declaration Order (CRITICAL)

C++ destroys members in reverse declaration order. Attachments call `evaluateJavascript()` during destruction -- if WebView is already dead, crash (EXC_BAD_ACCESS).

```
1. RELAYS (destroyed last)
2. WEBVIEW (destroyed second)  
3. ATTACHMENTS (destroyed first -- WebView still alive, safe)
```

**Reference:** `modules/core/webview-relay-manager/cpp/WebViewRelayManager.h`

### O-Formant Relay Inventory (21 parameters)

| Type | Count | Parameter IDs |
|------|-------|---------------|
| WebSliderRelay | 20 | vowelX, vowelY, vowelFocus, glottalRd, breathiness, vibratoRate, vibratoDepth, vibratoDelay, consonantLevel, consonantTone, sibilance, formantShift, formantSpread, pitchGlide, attack, decay, sustain, release, outputGain, stereoWidth |
| WebToggleButtonRelay | 1 | autoConsonant |

### Approach Decision: Manual Pattern (not WebViewRelayManager)

With 20 sliders + 1 toggle, using the managed class means no direct relay pointers for `.withOptionsFrom()`. The manual pattern (as in O-Prism, O-Texture) gives explicit control and is the established pattern across 27+ plugins.

**Pattern:** Store relays in `std::vector<std::unique_ptr<WebSliderRelay>>` + single toggle relay. Create attachments in matching vector after WebView initialization.

### Reference Implementation: O-Texture PluginEditor.h

```
plugins/O-Texture/Source/PluginEditor.h
```

- 7 slider relays + 1 combo + 1 toggle (XY pad plugin)
- Same relay->webView->attachment order
- XY pad canvas binding to xRelay + yRelay

### Reference Implementation: O-Prism PluginEditor.h

```
plugins/O-Prism/Source/PluginEditor.h
```

- Complex synth with vector-based relays
- `addNativeFunctions()` method for native bridge registration
- Timer-based data push to WebView

---

## 2. XY Pad Implementation (from O-Texture)

### JavaScript Pattern (Proven)

```
plugins/O-Texture/Source/ui/public/js/main.js (lines 117-156)
```

**Canvas pointer events:**
- `pointerdown` -> `setPointerCapture()`, `sliderDragStarted()` on both X and Y relays
- `pointermove` -> `setNormalisedValue()` on both relays
- `pointerup` -> `sliderDragEnded()` on both relays
- `pointercancel` -> safety cleanup

**Coordinate calculation:**
```js
const normX = Math.max(0, Math.min(1, (e.clientX - rect.left) / rect.width));
const normY = Math.max(0, Math.min(1, 1.0 - (e.clientY - rect.top) / rect.height));
```
Y is inverted (screen Y down, parameter Y up).

**Host automation response:**
```js
xState.valueChangedEvent.addListener(() => updateXYPadVisual());
yState.valueChangedEvent.addListener(() => updateXYPadVisual());
```

**JUCE bridge API:**
- `getSliderState(relayId)` -- returns state object
- `state.getNormalisedValue()` -- read current normalized value
- `state.setNormalisedValue(v)` -- set value (0-1)
- `state.sliderDragStarted()` / `state.sliderDragEnded()` -- DAW automation recording brackets
- `state.valueChangedEvent.addListener(fn)` -- respond to backend changes

### O-Formant XY Pad Additions (vs O-Texture)

1. **Vowel labels at fixed positions** -- IPA symbols (i, e, ɑ, o, u) drawn on canvas at acoustic coordinates
2. **No orbital trails** -- simpler visualization, just cursor dot + vowel markers
3. **Background grid/crosshair** (optional) -- subtle reference lines

### Vowel Positions (normalized, from VowelData.h)

| Vowel | IPA | X | Y | Canvas X (350px) | Canvas Y (350px, inverted) |
|-------|-----|---|---|-------------------|---------------------------|
| I | i | 0.00 | 1.00 | 0 | 0 |
| E | e | 0.31 | 0.43 | 108 | 200 |
| A | ɑ | 0.83 | 0.00 | 291 | 350 |
| O | o | 1.00 | 0.35 | 350 | 228 |
| U | u | 0.98 | 0.93 | 343 | 25 |

---

## 3. CMake Changes Required

### Current State (Stage 1)

```cmake
NEEDS_WEB_BROWSER FALSE
JUCE_WEB_BROWSER=0
```

### Required Changes

```cmake
# In juce_add_plugin():
NEEDS_WEB_BROWSER TRUE
NEEDS_WEBVIEW2 TRUE          # Windows WebView2 static linking

# Binary data target for UI resources:
juce_add_binary_data(O-Formant_UIResources
    SOURCES
        Source/ui/public/index.html
        Source/ui/public/js/juce/index.js
        Source/ui/public/js/juce/check_native_interop.js
        Source/ui/public/js/main.js
        Source/ui/public/img/[botanical-image].png
)

# Link UI resources:
target_link_libraries(O-Formant PRIVATE O-Formant_UIResources)

# Compile definitions:
JUCE_WEB_BROWSER=1                              # Was 0
JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1     # NEW
# Remove: JUCE_WEB_BROWSER=0
```

### Windows WebView2 User Data Folder

Must set temp directory for DAW plugin hosts:
```cpp
#if JUCE_WINDOWS
options = options.withWinWebView2Options(
    Options::WinWebView2{}.withUserDataFolder(
        File::getSpecialLocation(File::tempDirectory)
            .getChildFile("OFormant_WebView")));
#endif
```

---

## 4. Resource Provider Pattern

### Binary Resource Helper (from O-Prism)

```cpp
static auto makeBinaryResource(const char* data, int size, const char* mimeType)
    -> std::optional<juce::WebBrowserComponent::Resource>
{
    auto* byteData = reinterpret_cast<const std::byte*>(data);
    return juce::WebBrowserComponent::Resource {
        std::vector<std::byte>(byteData, byteData + size),
        juce::String(mimeType)
    };
}
```

### URL Matching (bare paths, NOT full URLs)

```cpp
if (url == "/" || url == "/index.html")
    return makeBinaryResource(BinaryData::index_html, BinaryData::index_htmlSize, "text/html");
if (url == "/js/juce/index.js")
    return makeBinaryResource(BinaryData::index_js, BinaryData::index_jsSize, "application/javascript");
// ... etc
```

**CRITICAL:** Resource provider receives bare paths only (e.g., `/`, `/index.html`). Do NOT strip scheme/host -- it's already a bare path. Match directly with equality checks.

### JUCE Bridge Files

Copy from any existing plugin (identical across all):
```
plugins/O-Texture/Source/ui/public/js/juce/index.js
plugins/O-Texture/Source/ui/public/js/juce/check_native_interop.js
```

These provide: `getSliderState()`, `getToggleState()`, `getComboBoxState()`, `getBackendResourceAddress()`, `getNativeFunction()`.

---

## 5. Botanical Image

### CONTEXT.md Decision: Songbird / Throat Anatomy

Available candidates:
- `birds/birds.png` -- multiple birds
- `birds/birdsEuropeIIIGoul_0022.png` through `_0310.png` -- individual European bird illustrations (6 options)
- `anatomy/muscles_histoirephysiqu911875gran_0161.png` -- throat/muscle anatomy

**Recommendation:** A single songbird illustration from `birds/` category. The bird = voice = vocal synthesis metaphor. Copy selected image to `plugins/O-Formant/Source/ui/public/img/`.

### Placement (per aesthetic spec)

```css
.botanical-overlay {
    position: absolute;
    right: -20px;
    top: 50%;
    transform: translateY(-50%);
    height: 71%;
    opacity: 0.35;
    pointer-events: none;
}
```

---

## 6. Layout Architecture (800x600)

### Two-Column + Bottom Row

```
+------------------------------------------+
|  O-FORMANT                               |
+------------------------------------------+
|                    |  GLOTTAL SOURCE      |
|    XY VOWEL        |  [Rd][Breath][VibR]  |
|    MORPH PAD       |  [VibD][VibDly]      |
|    (~350x350)      |                      |
|                    |  CONSONANT           |
|    i       u       |  [Level][Tone]       |
|      e   o         |  [Sibil][Auto]       |
|        a           |                      |
|                    |  CHARACTER           |
|                    |  [Shift][Spread]     |
|                    |  [Glide][Focus]      |
+--------------------+---------------------+
|  ENVELOPE            |  OUTPUT            |
|  [A][D][S][R]        |  [Gain][Width]     |
+------------------------------------------+
```

### CSS Layout Strategy

- **Outer container:** CSS Grid with 2 columns + bottom row
- **Left column:** XY pad canvas (aspect-ratio or fixed px)
- **Right column:** Flexbox sections stacked vertically
- **Bottom row:** Spans full width, flex row for envelope + output
- **No viewport units** (WebView limitation)
- **Fixed dimensions:** 800x600px plugin window

### Parameter Group Counts

| Group | Parameters | Knobs | Toggles |
|-------|-----------|-------|---------|
| XY Pad | vowelX, vowelY | 0 (canvas) | 0 |
| Glottal Source | glottalRd, breathiness, vibratoRate, vibratoDepth, vibratoDelay | 5 | 0 |
| Consonant | consonantLevel, consonantTone, sibilance, autoConsonant | 3 | 1 |
| Character | formantShift, formantSpread, pitchGlide, vowelFocus | 4 | 0 |
| Envelope | attack, decay, sustain, release | 4 | 0 |
| Output | outputGain, stereoWidth | 2 | 0 |

**Total:** 18 knobs + 1 toggle + 2 canvas-bound params = 21

### Knob Size

With 9+ parameters, aesthetic spec recommends 55px knobs with 25px gaps. Right column has max 5 knobs per row (Glottal) -- at 55px + 25px gaps = 375px, fits in ~430px right column.

---

## 7. Ouaricon Naturalist Aesthetic (Key Values)

### CSS Variables (from aesthetic.md)

```css
--bg-paper: #F5E6D3;
--brown-text: #3C2F2F;
--brown-border: #8B7355;
--green-light: #8BA870;
--green-dark: #3C5C1A;
```

### Knob Pattern

10-segment botanical seed cross-section via `conic-gradient`. Size: 55px for this parameter density. Indicator as rotated line element.

### Typography

- Font: `'Garamond', 'Times New Roman', serif`
- Plugin title: 22-26px, letter-spacing 2-3px
- Section labels: 12-14px, uppercase, 1px spacing
- Param labels: 9-11px, uppercase, 0.5-1px spacing

### Toggle (autoConsonant)

- Default: `rgba(139, 168, 112, 0.3)` background, `#3C5C1A` border
- Active: `rgba(107, 142, 35, 0.6)`, `#2C3E10` border

---

## 8. Pitfalls & Constraints

| Pitfall | Mitigation |
|---------|-----------|
| Member destruction order crash | Relays -> WebView -> Attachments (proven pattern) |
| Resource provider receives full URLs | It doesn't -- receives bare paths. Match with `==` |
| `<canvas>` doesn't stretch with CSS position tricks | Use explicit `width/height` in px, not `right/bottom` |
| Windows WebView2 user data folder denied | Set `withUserDataFolder()` to temp directory |
| No viewport units in WebView CSS | Use px/% only |
| WebView2 silent fallback to IE | NEEDS_WEBVIEW2 TRUE + static linking prevents this |
| Canvas DPR for Retina | Set `canvas.width = clientWidth * dpr`, use `ctx.setTransform(dpr,0,0,dpr,0,0)` |

---

## 9. File Structure (Phase 3.1 Deliverables)

```
plugins/O-Formant/
  Source/
    PluginEditor.h          -- Rewrite (WebView editor with 21 relays)
    PluginEditor.cpp        -- Rewrite (resource provider, relay init, attachments)
    ui/
      public/
        index.html          -- Full UI layout with Naturalist styling
        js/
          juce/
            index.js        -- JUCE bridge (copy from O-Texture)
            check_native_interop.js  -- Bridge check (copy from O-Texture)
          main.js           -- XY pad, knob binding, relay init
        img/
          [songbird].png    -- Botanical overlay image
  CMakeLists.txt            -- Updated: WebView flags, binary data target
```

---

## 10. Module Reuse

### webview-relay-manager (Available, NOT recommended for this plugin)

The module provides `WebViewRelayManager` class and convenience macros. However, with 20 slider relays, the manual vector pattern (as in O-Prism) is cleaner and matches the established codebase convention. The module is useful for smaller plugins (1-5 params).

### No other module dependencies for Stage 3

All UI code is custom HTML/CSS/JS + standard JUCE WebView API.
