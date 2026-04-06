# Stage 3 Phase 3.2: Visual Polish — Formant Overlay, Cursor Glow, ADSR Display — Summary

**Date:** 2026-04-05
**Status:** Complete
**Requirements:** UI-02 (nice — formant peaks overlay)

## What Was Built

Three visual polish features added to the WebView UI, all JS-side with no C++ changes.

### 1. Cursor Glow
- Radial gradient (28px radius) behind XY pad cursor
- Moss green at 0.3 opacity fading to transparent
- Draws behind crosshair and dot for depth effect

### 2. Formant Dot Overlay (F1-F5)
- VOWELS constant ported from VowelData.h (5 vowels, F1-F5 freq/bw/gain)
- `computeFormants()` — Shepard IDW interpolation matching VowelMorpher.h algorithm
- `applyShiftSpread()` — semitone shift + spread from center of mass matching FormantFilterBank.h
- 5 labeled dots (F1-F5) in lower portion of XY pad
- X: log-frequency mapping (200-5000 Hz range)
- Y: gain-based height in bottom 30% of pad (avoids vowel labels)
- Dots update reactively on vowelX/vowelY/vowelFocus/formantShift/formantSpread changes

### 3. ADSR Curve Display
- Canvas element added below Envelope knobs (100% width, 50px height)
- DPR-aware rendering matching existing setupCanvas() pattern
- Linear segments (matching juce::ADSR): attack→peak, decay→sustain, sustain hold (20%), release→0
- Moss green (#8BA870) stroke with subtle area fill
- Updates reactively on attack/decay/sustain/release changes

## Files Modified (2)

| File | Changes |
|------|---------|
| `Source/ui/public/js/main.js` | +VOWELS constant, +computeFormants(), +applyShiftSpread(), +cursor glow in drawXYPad(), +formant dots in drawXYPad(), +setupADSRCanvas(), +drawADSR(), +7 relay listeners |
| `Source/ui/public/index.html` | +`<canvas id="adsr-canvas">` in Envelope group, +CSS for #adsr-canvas |

**No C++ changes. No new relays. No binary data changes.**

## Build Result

- VST3 + AU compiled successfully (pre-existing sign-conversion warnings in GlottalTableGenerator.cpp only)
- Installed to ~/Library/Audio/Plug-Ins/
