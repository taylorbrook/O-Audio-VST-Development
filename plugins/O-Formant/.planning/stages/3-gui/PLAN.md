# Stage 3 Phase 3.1: Layout + Controls + Parameter Binding

**Date:** 2026-04-05
**Goal:** Replace GenericAudioProcessorEditor with full WebView UI — Ouaricon Naturalist aesthetic, 2D XY vowel morph pad, all 21 parameters bound via relays.
**Requirements:** UI-01 (must), UI-03 (nice)

---

## Tasks

### 1. [ ] Update CMakeLists.txt for WebView + Binary Data

**Files:** `CMakeLists.txt`
**Depends on:** none

Changes:
- `NEEDS_WEB_BROWSER TRUE` (was FALSE)
- Add `NEEDS_WEBVIEW2 TRUE`
- Add `juce_add_binary_data(O-Formant_UIResources ...)` target for all UI files
- Link `O-Formant_UIResources` to plugin target
- Change `JUCE_WEB_BROWSER=0` → `JUCE_WEB_BROWSER=1`
- Add `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`

Reference: `plugins/O-Texture/CMakeLists.txt` (binary data pattern)

### 2. [ ] Copy JUCE bridge files from O-Texture

**Files create:**
- `Source/ui/public/js/juce/index.js`
- `Source/ui/public/js/juce/check_native_interop.js`

**Depends on:** none

Verbatim copy from `plugins/O-Texture/Source/ui/public/js/juce/`. These are identical across all plugins — provide `getSliderState()`, `getToggleState()`, `getBackendResourceAddress()`.

### 3. [ ] Source and copy botanical image

**Files create:** `Source/ui/public/img/songbird.png`
**Depends on:** none

Select a songbird illustration from an existing plugin's botanical assets or the project image library. Songbird = voice = vocal synthesis metaphor. Copy to `Source/ui/public/img/`. If no suitable bird exists, use a nature illustration and rename.

### 4. [ ] Create index.html — Full Naturalist Layout

**Files create:** `Source/ui/public/index.html`
**Depends on:** Tasks 2, 3

Complete HTML document with:
- **Embedded CSS** (Ouaricon Naturalist aesthetic — `#F5E6D3` paper, `#3C2F2F` brown text, `#8B7355` borders, `#8BA870` moss accents, Garamond serif)
- **Two-column CSS Grid layout** (800x600 fixed):
  - Left: XY vowel morph pad canvas (~350x350) with IPA vowel labels
  - Right: 3 stacked parameter groups (Glottal Source, Consonant, Character)
  - Bottom row: Envelope (ADSR) + Output (Gain, Width)
- **Knob CSS:** 10-segment botanical seed cross-section (`conic-gradient`), 55px diameter, rotational indicator
- **Toggle CSS:** For `autoConsonant` — moss green active state
- **Botanical overlay:** Songbird image, right side, 0.35 opacity, pointer-events none
- **Canvas element** for XY pad with DPR-aware sizing (`canvas.width = clientWidth * dpr`)
- Script tags loading `js/juce/index.js`, `js/juce/check_native_interop.js`, `js/main.js`

Key constraints:
- No viewport units (WebView limitation)
- All dimensions in px or %
- 55px knobs with 25px gaps for right column density (5 per row max)

### 5. [ ] Create main.js — XY Pad + Knob Binding + Relay Init

**Files create:** `Source/ui/public/js/main.js`
**Depends on:** Task 4

JavaScript responsibilities:

**Relay initialization (21 total):**
- 20 slider states via `__JUCE__.getSliderState("paramIdSlider")`
- 1 toggle state via `__JUCE__.getToggleState("autoConsonantToggle")`

**XY Pad (canvas):**
- `pointerdown` → `setPointerCapture()`, call `sliderDragStarted()` on vowelX + vowelY relays
- `pointermove` → calculate `normX`, `normY` (Y inverted), call `setNormalisedValue()` on both
- `pointerup` → `sliderDragEnded()` on both relays
- `pointercancel` → safety cleanup
- Draw 5 IPA vowel labels (i, e, ɑ, o, u) at fixed acoustic positions
- Draw cursor dot at current position
- Host automation response: `valueChangedEvent.addListener()` on both X/Y states → update cursor
- DPR-aware canvas: `canvas.width = clientWidth * dpr`, `ctx.setTransform(dpr, ...)`

**Knobs (18 rotational):**
- Each knob element: mousedown/touchstart → `sliderDragStarted()`, track vertical drag delta → `setNormalisedValue()`, mouseup → `sliderDragEnded()`
- Visual rotation: map 0-1 normalized value to -135° to +135° indicator rotation
- `valueChangedEvent.addListener()` for host automation sync
- Display current value as tooltip or label beneath knob

**Toggle (autoConsonant):**
- Click → `setState(!currentState)`
- `stateChangedEvent.addListener()` for visual sync

Reference: `plugins/O-Texture/Source/ui/public/js/main.js` (XY pad pattern, lines 117-156)

### 6. [ ] Rewrite PluginEditor.h — WebView Editor with 21 Relays

**Files modify:** `Source/PluginEditor.h`
**Depends on:** none

Replace placeholder with full WebView editor class:

```
class OFormantEditor : public juce::AudioProcessorEditor
```

Member declaration order (CRITICAL — destruction order):
1. **RELAYS** (destroyed last): 20 `std::unique_ptr<WebSliderRelay>` + 1 `std::unique_ptr<WebToggleButtonRelay>`
2. **WEBVIEW** (destroyed second): `std::unique_ptr<WebBrowserComponent>`
3. **ATTACHMENTS** (destroyed first): 20 `std::unique_ptr<WebSliderParameterAttachment>` + 1 `std::unique_ptr<WebToggleButtonParameterAttachment>`

Private method: `getResource(const juce::String& url)` for resource provider.

Relay naming convention: `{paramId}Relay` for members, `"{paramId}Slider"` / `"{paramId}Toggle"` for relay IDs (must match JS).

### 7. [ ] Rewrite PluginEditor.cpp — Resource Provider + Relay Init

**Files modify:** `Source/PluginEditor.cpp`
**Depends on:** Tasks 1-6

Constructor sequence:
1. Create all 21 relays with matching JS names
2. Build `WebBrowserComponent::Options`:
   - `.withBackend(webview2)`
   - `.withWinWebView2Options(...)` with temp user data folder `"OFormant_WebView"`
   - `.withNativeIntegrationEnabled()`
   - `.withKeepPageLoadedWhenBrowserIsHidden()`
   - `.withResourceProvider(...)` (guarded by `JUCE_WEB_BROWSER_RESOURCE_PROVIDER_AVAILABLE`)
   - `.withOptionsFrom(...)` for all 21 relays
3. `addAndMakeVisible(*webView)`
4. Create all 21 attachments linking `parameters.getParameter("paramId")` → relay
5. `webView->goToURL(getResourceProviderRoot())`
6. `setSize(800, 600)`

Destructor: explicit `.reset()` in reverse order (attachments → webView → relays).

`getResource()`: bare path matching with `==` (NOT URL stripping):
- `/` or `/index.html` → `index_html`
- `/js/juce/index.js` → `index_js`
- `/js/juce/check_native_interop.js` → `check_native_interop_js`
- `/js/main.js` → `main_js`
- `/img/songbird.png` → `songbird_png`

`paint()`: Fill with `#F5E6D3` (paper background, visible during WebView load).
`resized()`: `webView->setBounds(getLocalBounds())`

### 8. [ ] Update PluginProcessor.cpp — Return WebView Editor

**Files modify:** `Source/PluginProcessor.cpp`
**Depends on:** Task 6

Change:
```cpp
// FROM:
return new juce::GenericAudioProcessorEditor(*this);
// TO:
return new OFormantEditor(*this);
```

Also add `#include "PluginEditor.h"` if not already present (verify — it likely already includes it).

### 9. [ ] Build, Verify, Install

**Depends on:** Tasks 1-8

```bash
cd build && cmake .. -G Ninja && ninja O-Formant_VST3 O-Formant_AU
```

Verification:
- [ ] Compiles without errors or warnings
- [ ] Binary data symbols resolve (no undefined `BinaryData::` references)
- [ ] Plugin loads in Standalone — WebView renders Naturalist UI
- [ ] XY pad responds to drag — vowelX + vowelY parameters update in host
- [ ] Host automation of vowelX/vowelY moves cursor on pad
- [ ] All 18 knobs respond to drag — parameter values update
- [ ] autoConsonant toggle clicks between on/off
- [ ] Vowel labels visible at correct IPA positions on XY pad
- [ ] Ouaricon Naturalist aesthetic renders correctly (paper bg, Garamond text, seed knobs)
- [ ] Botanical songbird overlay visible at 0.35 opacity
- [ ] No crashes on close (destruction order correct)

Install:
```bash
killall -9 AudioComponentRegistrar 2>/dev/null || true
rm -rf ~/Library/Caches/AudioUnitCache/ ~/Library/Caches/com.apple.audiounits.cache
rm -rf ~/Library/Audio/Plug-Ins/VST3/O-Formant.vst3 ~/Library/Audio/Plug-Ins/Components/O-Formant.component
cp -R build/plugins/O-Formant/O-Formant_artefacts/Release/VST3/O-Formant.vst3 ~/Library/Audio/Plug-Ins/VST3/
cp -R build/plugins/O-Formant/O-Formant_artefacts/Release/AU/O-Formant.component ~/Library/Audio/Plug-Ins/Components/
```

---

## Success Criteria

- [ ] WebView UI renders with Ouaricon Naturalist aesthetic (paper, Garamond, seed knobs)
- [ ] XY vowel morph pad functional — drag updates vowelX + vowelY params (UI-01)
- [ ] 5 IPA vowel labels at acoustic positions on XY pad (UI-01)
- [ ] Host automation of vowelX/vowelY reflects on cursor position (UI-01)
- [ ] All 21 parameters bound and functional through WebView controls (UI-03)
- [ ] Parameter groups organized: Glottal, Consonant, Character, Envelope, Output (UI-03)
- [ ] Plugin opens/closes without crash (destruction order correct)
- [ ] Builds on macOS (VST3 + AU)

## APVTS Parameter IDs (verified)

| # | Parameter ID | Relay Type | JS Name | Group |
|---|-------------|------------|---------|-------|
| 1 | vowelX | Slider | vowelXSlider | XY Pad |
| 2 | vowelY | Slider | vowelYSlider | XY Pad |
| 3 | vowelFocus | Slider | vowelFocusSlider | Character |
| 4 | glottalRd | Slider | glottalRdSlider | Glottal |
| 5 | breathiness | Slider | breathinessSlider | Glottal |
| 6 | vibratoRate | Slider | vibratoRateSlider | Glottal |
| 7 | vibratoDepth | Slider | vibratoDepthSlider | Glottal |
| 8 | vibratoDelay | Slider | vibratoDelaySlider | Glottal |
| 9 | consonantLevel | Slider | consonantLevelSlider | Consonant |
| 10 | consonantTone | Slider | consonantToneSlider | Consonant |
| 11 | sibilance | Slider | sibilanceSlider | Consonant |
| 12 | autoConsonant | Toggle | autoConsonantToggle | Consonant |
| 13 | attack | Slider | attackSlider | Envelope |
| 14 | decay | Slider | decaySlider | Envelope |
| 15 | sustain | Slider | sustainSlider | Envelope |
| 16 | release | Slider | releaseSlider | Envelope |
| 17 | formantShift | Slider | formantShiftSlider | Character |
| 18 | formantSpread | Slider | formantSpreadSlider | Character |
| 19 | pitchGlide | Slider | pitchGlideSlider | Character |
| 20 | outputGain | Slider | outputGainSlider | Output |
| 21 | stereoWidth | Slider | stereoWidthSlider | Output |

---

*Phase 3.1 — 9 tasks, 7 files created, 4 files modified*
