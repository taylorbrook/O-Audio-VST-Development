# Stage 3: GUI - Context

## Discussion Summary

**Date:** 2026-02-03
**Participants:** User, Claude

## Requirements Confirmed

### Visual Style: Dark Botanical

- **Base Theme:** Dark background with botanical/scientific illustration overlay
- **Paper Texture:** `/Users/taylorbrook/Dev/Ouaricon Audio Images/paper/paper1.jpg` - darkened/inverted
- **Illustration:** `/Users/taylorbrook/Dev/Ouaricon Audio Images/RAW/insects/slug_olididae118771879trin_0119.jpg` - sea slug with green leaf-like cerata, inverted/darkened for ethereal effect
- **Overall Aesthetic:** Scientific journal meets dark mode - technical precision with organic warmth

### Assets

| Asset | Path | Usage |
|-------|------|-------|
| Paper texture | `/Users/taylorbrook/Dev/Ouaricon Audio Images/paper/paper1.jpg` | Darkened/inverted as subtle background texture |
| Sea slug illustration | `/Users/taylorbrook/Dev/Ouaricon Audio Images/RAW/insects/slug_olididae118771879trin_0119.jpg` | Inverted/ethereal overlay - ghostly botanical element |

### Layout

- **Structure:** Stacked vertical layout
  - Spectrogram at top (hero element)
  - Attack curve editor below spectrogram
  - Sustain curve editor below attack curve
  - Parameter knobs in right sidebar
- **Dimensions:** 700×500 pixels (wide rectangle)
- **Header:** Top bar with "O-SpectralShaper" branding

### Controls (6 Parameters + 2 Curve Areas)

| Parameter | UI Element | Style | Notes |
|-----------|------------|-------|-------|
| Mix | Rotary knob | Botanical-styled | Right sidebar |
| Attack Time | Rotary knob | Botanical-styled | Right sidebar |
| Sustain Time | Rotary knob | Botanical-styled | Right sidebar |
| Sensitivity | Rotary knob | Botanical-styled | Right sidebar |
| Lookahead | Rotary knob | Botanical-styled | Right sidebar |
| Output Gain | Rotary knob | Botanical-styled | Right sidebar |
| Attack Curve | Drawable canvas | Grid overlay, blue accent | 32-band logarithmic |
| Sustain Curve | Drawable canvas | Grid overlay, orange accent | 32-band logarithmic |

### Spectrogram Specification

- **Type:** Scrolling time × frequency (WebGL)
- **Axis:** Logarithmic frequency (20Hz-20kHz on Y), time scrolling on X
- **Colormap:** Standard spectrogram palette (dark blue → cyan → yellow → white for magnitude)
- **Transient Overlay:** Heat overlay - red/orange blended on top of spectrogram where transient activity is high
- **Update Rate:** 60fps via requestAnimationFrame

### Curve Editor Specification

- **Mode Switching:** Explicit toggle button (Freehand vs Node)
- **Freehand Mode:** Mouse drag draws curve, Catmull-Rom spline smoothing
- **Node Mode:** Click to place control points, drag for precision
- **X-Axis:** Frequency (logarithmic scale, 32 bands: 20Hz-20kHz)
- **Y-Axis:** Boost/Cut (-1.0 to +1.0, displayed as ±100% or ±12dB)
- **Grid:** Frequency labels, dB scale lines
- **Colors:**
  - Attack curve: Blue accent (#4A90D9)
  - Sustain curve: Orange accent (#D9944A)
- **Persistence:** Curves save/load with plugin state (hex-encoded in XML)

### Typography

- **Font Family:** Serif for labels (scientific aesthetic), matches Ouaricon branding
- **Header:** "O-SpectralShaper" - prominent in top bar
- **Labels:** Parameter names below knobs
- **Values:** Current parameter values displayed on/near knobs
- **Curve Labels:** "ATTACK" and "SUSTAIN" labels on curve editors

### Color Palette (Dark Botanical)

| Element | Color | Hex |
|---------|-------|-----|
| Background | Dark charcoal | #1A1A1A |
| Paper overlay | Sepia tint (10% opacity) | Darkened paper1.jpg |
| Slug overlay | Inverted/ghostly (5-15% opacity) | Inverted slug image |
| Text primary | Warm cream | #E8E0D4 |
| Text secondary | Muted sepia | #A89888 |
| Attack accent | Botanical blue | #4A90D9 |
| Sustain accent | Botanical orange | #D9944A |
| Transient heat | Red-orange gradient | #FF4444 → #FF8844 |
| Spectrogram cold | Deep blue | #1A2440 |
| Spectrogram hot | Bright cyan/white | #44FFFF → #FFFFFF |
| Knob indicator | Warm cream | #E8E0D4 |

## Constraints Identified

1. **WebGL Requirement:** Spectrogram needs GPU rendering for 60fps scrolling
2. **Thread Safety:** Visualization data via lock-free FIFO (already in DSP)
3. **Curve Sync:** 32-value arrays must update atomically between JS and C++
4. **Real-Time Performance:** UI updates must not affect audio thread
5. **State Persistence:** Curve arrays saved as hex-encoded data in XML
6. **Image Assets:** Need to bundle inverted/processed versions, not load at runtime

## Approach Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Layout | Stacked vertical | Spectrogram needs width; curves stack naturally below |
| Dimensions | 700×500 | Balance of detail and DAW screen fit |
| Curve mode switching | Toggle button | User explicitly chose over modifier key |
| Transient visualization | Heat overlay | Integrated with spectrogram, matches BRIEF |
| Botanical integration | Inverted/ethereal | Dark theme with ghostly slug overlay |
| Knob placement | Right sidebar | Keeps main area for visualization, common pattern |
| WebGL spectrogram | Yes | HTML5 Canvas can't handle 60fps scrolling |

## Data Flow (JavaScript ↔ C++)

### C++ → JavaScript (Visualization Data)

```
Audio Thread                    GUI Thread                  JavaScript
     │                               │                           │
     │ ──FIFO push──►                │                           │
     │ (FFT mags + transients)       │                           │
     │                               │ ◄──Timer 60fps──          │
     │                               │ ──FIFO pop──►             │
     │                               │                           │
     │                               │ ──evaluateJS()──►         │
     │                               │                  updateSpectrogram(data)
```

### JavaScript → C++ (Curve Updates)

```
JavaScript                      C++ GUI Thread              Audio Thread
     │                               │                           │
     │ ──Juce.getNativeFunction()──► │                           │
     │   "setAttackCurve([32 vals])" │                           │
     │                               │ ──atomic swap──►          │
     │                               │               newCurveBuffer
```

## Phase Breakdown (from ROADMAP.md)

### Phase 3.1: Layout and Basic Controls
- WebView setup with resource provider
- 6 parameter knobs in right sidebar
- HTML/CSS dark botanical theme
- Parameter binding (WebSliderRelay + Attachment)
- Placeholder areas for spectrogram and curves

### Phase 3.2: Drawable Curve Editors
- HTML5 Canvas curve editors (Attack + Sustain)
- Freehand mode with Catmull-Rom smoothing
- Node mode with draggable control points
- Mode toggle button
- C++ native function communication
- Atomic curve buffer updates

### Phase 3.3: Real-Time Spectrogram + Transient Overlay
- juce::AbstractFifo data pipeline
- WebGL spectrogram renderer
- Fragment shader colormap
- Transient heat overlay blend
- 60fps requestAnimationFrame loop
- GPU texture scrolling

## Parameter-UI Mapping

| Parameter ID | Type | Range | Default | UI Element | Relay Type |
|-------------|------|-------|---------|------------|------------|
| MIX | Float | 0-100% | 100% | Rotary knob | WebSliderRelay |
| ATTACK_TIME | Float | 0.1-50ms | 10ms | Rotary knob | WebSliderRelay |
| SUSTAIN_TIME | Float | 10-500ms | 100ms | Rotary knob | WebSliderRelay |
| SENSITIVITY | Float | 0-100% | 50% | Rotary knob | WebSliderRelay |
| LOOKAHEAD | Float | 0-10ms | 2ms | Rotary knob | WebSliderRelay |
| OUTPUT_GAIN | Float | -12 to +12dB | 0dB | Rotary knob | WebSliderRelay |

### Curve Data (Non-APVTS, Custom Handling)

| Data | Type | Values | Communication |
|------|------|--------|---------------|
| Attack Curve | float[32] | -1.0 to +1.0 | Juce.getNativeFunction("setAttackCurve") |
| Sustain Curve | float[32] | -1.0 to +1.0 | Juce.getNativeFunction("setSustainCurve") |

## Open Questions

None - all requirements clarified during discussion.

## Next Phase

Ready for: **Research** phase (investigate WebGL spectrogram implementation patterns) or **Plan** phase (create detailed execution plan)

Recommended path: `/plugin-research O-SpectralShaper 3-gui` or `/plugin-plan O-SpectralShaper 3-gui`

---

*Context gathered: 2026-02-03*
*Discuss phase complete*
