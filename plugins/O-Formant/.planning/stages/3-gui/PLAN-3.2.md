# Stage 3 Phase 3.2: Visual Polish — Formant Overlay, Cursor Glow, ADSR Display

**Date:** 2026-04-05
**Goal:** Add formant peaks overlay (F1-F5 dot markers), cursor glow, and ADSR curve visualization to the WebView UI. All JS-side — no C++ changes.
**Requirements:** UI-02 (nice — formant peaks overlay)

---

## Tasks

### 1. [ ] Add VowelData constants and formant computation to main.js

**Files:** `Source/ui/public/js/main.js`
**Depends on:** none

Add at top of file (after relay declarations):
- `VOWELS` array: 5 vowels with name, x, y, freq[5], bw[5], gain[5] (from VowelData.h / RESEARCH-3.2.md)
- `computeFormants(cursorX, cursorY, focus)` — Shepard IDW interpolation, log-domain frequency blending (matches VowelMorpher.h algorithm)
- `applyShiftSpread(freq, shift, spread)` — semitone pitch shift + spread from center of mass (matches FormantFilterBank.h lines 42-61)

Constants must match VowelData.h exactly:
```
A: x=0.83 y=0.00, freq=[600,1040,2250,2450,2750], gain=[1.0,0.4467,0.3548,0.3548,0.1000]
E: x=0.31 y=0.43, freq=[400,1620,2400,2800,3100], gain=[1.0,0.2512,0.3548,0.2512,0.1259]
I: x=0.00 y=1.00, freq=[250,1750,2600,3050,3340], gain=[1.0,0.0316,0.1585,0.0794,0.0398]
O: x=1.00 y=0.35, freq=[400,750,2400,2600,2900],  gain=[1.0,0.2818,0.0891,0.1000,0.0100]
U: x=0.98 y=0.93, freq=[350,600,2400,2675,2950],  gain=[1.0,0.1000,0.0251,0.0398,0.0158]
```

### 2. [ ] Add cursor glow to drawXYPad()

**Files:** `Source/ui/public/js/main.js`
**Depends on:** none

Insert **before** the crosshair drawing (line ~200, after `cx`/`cy` are computed, before `ctx.strokeStyle = 'rgba(139,163,112,0.4)'`):
- Create radial gradient centered at (cx, cy), radius 28px
- Inner stop: `rgba(139, 168, 112, 0.3)` (moss green)
- Outer stop: `rgba(139, 168, 112, 0.0)` (transparent)
- Fill arc at cursor position

### 3. [ ] Add formant dot overlay to drawXYPad()

**Files:** `Source/ui/public/js/main.js`
**Depends on:** Task 1

Insert at **end** of `drawXYPad()` (after inner dot), so formant markers draw on top:
- Get scaled values: `formantShiftState.getScaledValue()`, `formantSpreadState.getScaledValue()`, `vowelFocusState.getScaledValue()`
- Call `computeFormants(normX, normY, focus)` then `applyShiftSpread(result.freq, shift, spread)`
- Map F1-F5 to XY pad coordinates:
  - X: `log-frequency mapping` — `(Math.log(f) - Math.log(200)) / (Math.log(5000) - Math.log(200))`
  - Y: gain-based height in bottom 30% of pad — `1.0 - (gain * 0.3)` (lower portion, avoids vowel labels)
- Draw 5 dots: 3.5px radius circles, fill `rgba(139, 168, 112, 0.5)`, stroke `rgba(139, 168, 112, 0.8)` 1px
- Draw labels "F1"-"F5" in 8px font below each dot, same color at 0.6 opacity

### 4. [ ] Add relay listeners for formant-affecting parameters

**Files:** `Source/ui/public/js/main.js`
**Depends on:** none

In `initRelays()`, add `valueChangedEvent.addListener(() => drawXYPad())` for:
- `formantShiftState`
- `formantSpreadState`
- `vowelFocusState`

These 3 relays affect formant dot positions but currently don't trigger XY pad redraw. (vowelX/vowelY already have listeners.)

### 5. [ ] Add ADSR canvas element to index.html

**Files:** `Source/ui/public/index.html`
**Depends on:** none

In the Envelope `.bottom-group` (line ~356-379), add after the `.param-row` div:
```html
<canvas id="adsr-canvas"></canvas>
```

Add CSS:
```css
#adsr-canvas {
  width: 100%;
  height: 50px;
  margin-top: 4px;
}
```

### 6. [ ] Add ADSR canvas drawing logic to main.js

**Files:** `Source/ui/public/js/main.js`
**Depends on:** Tasks 4, 5

Add functions:
- `setupADSRCanvas()` — called from DOMContentLoaded, sets up DPR-aware canvas (same pattern as `setupCanvas()`)
- `drawADSR()` — reads scaled values from attackState, decayState, sustainState, releaseState:
  - A+D+R time proportionally fill 80% of canvas width; sustain hold gets 20%
  - Linear segments (matching juce::ADSR): attack 0→1, decay 1→sustain, sustain hold, release sustain→0
  - Stroke: `#8BA870` moss green, 1.5px line width
  - 4px padding on all sides
  - Fill underneath curve with `rgba(139, 168, 112, 0.08)` for subtle area fill

Add relay listeners in `initRelays()`:
- `attackState.valueChangedEvent.addListener(() => drawADSR())`
- `decayState.valueChangedEvent.addListener(() => drawADSR())`
- `sustainState.valueChangedEvent.addListener(() => drawADSR())`
- `releaseState.valueChangedEvent.addListener(() => drawADSR())`

Call `setupADSRCanvas()` and initial `drawADSR()` from DOMContentLoaded (after `setupCanvas()`).

### 7. [ ] Build and verify

**Depends on:** Tasks 1-6

```bash
cd build && cmake .. -G Ninja && ninja O-Formant_VST3 O-Formant_AU
```

Verification checklist:
- [ ] Compiles without errors
- [ ] Cursor has soft moss-green glow behind it on XY pad
- [ ] F1-F5 dots appear in lower portion of XY pad
- [ ] Formant dots move when dragging cursor (vowelX/vowelY change)
- [ ] Formant dots respond to Shift/Spread/Focus knob changes
- [ ] Formant dots don't obscure vowel labels or cursor
- [ ] ADSR canvas renders below envelope knobs
- [ ] ADSR curve updates when turning attack/decay/sustain/release knobs
- [ ] ADSR curve shape matches expectations (linear segments)
- [ ] No visual regressions (knobs, toggle, botanical overlay still correct)
- [ ] DPR scaling correct on Retina (no blurry canvas elements)

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

- [ ] Cursor glow: soft radial moss-green glow behind XY pad cursor
- [ ] Formant overlay: F1-F5 labeled dots in lower portion of XY pad (UI-02)
- [ ] Formant dots update reactively on vowelX/vowelY/vowelFocus/formantShift/formantSpread changes
- [ ] ADSR curve: linear envelope visualization below envelope knobs
- [ ] ADSR updates reactively on attack/decay/sustain/release changes
- [ ] No visual regressions from Phase 3.1
- [ ] Builds on macOS (VST3 + AU)

## Files Modified

| File | Changes |
|------|---------|
| `Source/ui/public/js/main.js` | VOWELS constant, computeFormants(), applyShiftSpread(), cursor glow in drawXYPad(), formant dots in drawXYPad(), ADSR canvas setup/draw, 7 new relay listeners |
| `Source/ui/public/index.html` | `<canvas id="adsr-canvas">` in Envelope group + CSS |

**No C++ changes. No new relays. No binary data changes (existing resources only).**

---

*Phase 3.2 -- 7 tasks, 2 files modified*
