# Stage 3: GUI - Execution Plan

**Plugin:** O-SpectralShaper
**Stage:** 3 (GUI Implementation)
**Created:** 2026-02-04
**Status:** Ready for Execution

---

## Goal

Implement the complete WebView GUI for O-SpectralShaper: dark botanical theme with 6 parameter knobs, drawable attack/sustain curve editors (freehand + node modes), and a real-time WebGL spectrogram with transient heat overlay.

---

## Phase Breakdown

This stage is divided into 3 phases per ROADMAP.md:

| Phase | Focus | Key Deliverables |
|-------|-------|------------------|
| 3.1 | Layout & Controls | Dark botanical theme, 6 rotary knobs, parameter binding |
| 3.2 | Curve Editors | Freehand + node modes, C++ communication, curve sync |
| 3.3 | Spectrogram | AbstractFifo pipeline, WebGL renderer, transient overlay |

---

## Phase 3.1: Layout and Basic Controls

### Tasks

1. [ ] **Process and bundle image assets**
   - Files: Resources/ui/images/paper-bg.webp, slug-overlay.webp
   - Process paper1.jpg → darkened/inverted WebP
   - Process slug image → inverted/ethereal WebP
   - Add to CMakeLists.txt binary data
   - Depends on: none

2. [ ] **Create dark botanical CSS theme**
   - Files: Resources/ui/css/styles.css
   - Color palette from CONTEXT.md (#1A1A1A, #E8E0D4, etc.)
   - Paper texture overlay (10% opacity)
   - Slug illustration overlay (5-15% opacity)
   - Serif typography (Georgia/Times)
   - Depends on: Task 1

3. [ ] **Build stacked HTML layout (700×500)**
   - Files: Resources/ui/index.html
   - Header: "O-SpectralShaper" branding
   - Main area: Spectrogram placeholder (will be WebGL canvas)
   - Middle: Attack curve editor placeholder
   - Lower: Sustain curve editor placeholder
   - Right sidebar: 6 rotary knob containers
   - Dimensions: 700×500 fixed
   - Depends on: Task 2

4. [ ] **Implement rotary knob component (JavaScript)**
   - Files: Resources/ui/js/components/RotaryKnob.js
   - Relative drag pattern (deltaY from lastY)
   - Rotation range: -135° to +135°
   - Visual rotation via CSS transform
   - Value display (below or inside knob)
   - Depends on: Task 3

5. [ ] **Wire existing relays to JavaScript knobs**
   - Files: Resources/ui/index.html, Resources/ui/js/app.js
   - Bind: MIX, ATTACK_TIME, SUSTAIN_TIME, SENSITIVITY, LOOKAHEAD_ENABLED, LOOKAHEAD_TIME, OUTPUT_GAIN
   - Use Juce.getSliderState() / Juce.getToggleState()
   - valueChangedEvent pattern (no callback params in JUCE 8)
   - Depends on: Task 4

6. [ ] **Update PluginEditor dimensions**
   - Files: Source/PluginEditor.cpp
   - setSize(700, 500)
   - Depends on: none

### Phase 3.1 Success Criteria

- [ ] WebView loads without console errors
- [ ] Dark botanical theme visible (paper texture, slug overlay)
- [ ] All 6 parameter knobs visible and styled
- [ ] Dragging knobs changes DSP parameters (audible effect)
- [ ] DAW automation moves knobs in UI
- [ ] Preset changes update knobs correctly

### Phase 3.1 Commit

```
feat(O-SpectralShaper): Phase 3.1 - WebView layout with botanical theme and parameter controls
```

---

## Phase 3.2: Drawable Curve Editors

### Tasks

7. [ ] **Create CurveEditor base class**
   - Files: Resources/ui/js/components/CurveEditor.js
   - HTML5 Canvas setup (fills container)
   - Logarithmic X-axis (20Hz-22050Hz)
   - Y-axis: -1.0 to +1.0 (displayed as ±12dB)
   - Grid overlay with frequency labels
   - Abstract draw() method
   - Depends on: Phase 3.1 complete

8. [ ] **Implement Freehand mode**
   - Files: Resources/ui/js/components/FreehandCurve.js
   - Mouse drag captures raw points
   - Catmull-Rom spline smoothing (tension 0.5)
   - Sample curve at 32 band centers (log spacing)
   - Real-time redraw during drag
   - Depends on: Task 7

9. [ ] **Implement Node mode**
   - Files: Resources/ui/js/components/NodeCurve.js
   - Double-click to add control point
   - Drag nodes to move
   - Bezier interpolation between nodes
   - 10px hit radius for selection
   - Delete key removes selected node
   - Depends on: Task 7

10. [ ] **Add mode toggle button**
    - Files: Resources/ui/index.html, Resources/ui/js/app.js
    - Toggle button: "Freehand" / "Node"
    - One toggle per curve area (attack, sustain)
    - Persist mode preference
    - Depends on: Tasks 8, 9

11. [ ] **Register C++ native functions for curve updates**
    - Files: Source/PluginEditor.h, Source/PluginEditor.cpp
    - addNativeFunction("setAttackCurve", ...)
    - addNativeFunction("setSustainCurve", ...)
    - Accept juce::Array<juce::var> (32 values)
    - Call processorRef.setAttackCurve/setSustainCurve
    - Depends on: none (C++ side)

12. [ ] **Connect JavaScript curve editors to C++ native functions**
    - Files: Resources/ui/js/app.js, CurveEditor.js
    - Get native function via Juce.getNativeFunction("setAttackCurve")
    - Call on finishDrawing() or node release
    - Throttle to max 30 updates/sec during drag
    - Depends on: Tasks 8, 9, 10, 11

13. [ ] **Add curve initialization from C++ state**
    - Files: Source/PluginEditor.cpp, Resources/ui/js/app.js
    - addNativeFunction("getAttackCurve") / "getSustainCurve"
    - JavaScript calls on DOMContentLoaded
    - Initialize curve display from saved state
    - Depends on: Task 12

### Phase 3.2 Success Criteria

- [ ] Attack curve editor draws smooth freehand curves
- [ ] Sustain curve editor draws smooth freehand curves
- [ ] Node mode allows precise control point placement
- [ ] Mode toggle switches between freehand and node
- [ ] Drawing curves produces audible change (transient shaping)
- [ ] Curves persist across preset save/load
- [ ] No audio glitches when drawing curves
- [ ] Frequency labels visible on grid (50Hz, 100Hz, 1kHz, 10kHz, etc.)

### Phase 3.2 Commit

```
feat(O-SpectralShaper): Phase 3.2 - Drawable attack/sustain curve editors with freehand and node modes
```

---

## Phase 3.3: Real-Time Spectrogram + Transient Overlay

### Tasks

14. [ ] **Add visualization data structures to Processor**
    - Files: Source/PluginProcessor.h
    - struct VisualizationFrame { fftMagnitudes[257], transientActivity[32] }
    - juce::AbstractFifo visualizationFifo { 60 }
    - std::vector<VisualizationFrame> visualizationBuffer { 60 }
    - writeVisualizationFrame() method
    - Depends on: none

15. [ ] **Push FFT data to FIFO in processBlock**
    - Files: Source/PluginProcessor.cpp, Source/STFTProcessor.h/.cpp
    - Extract current FFT magnitudes from STFTProcessor
    - Add getLastMagnitudes() accessor to STFTProcessor
    - Call writeVisualizationFrame() once per FFT hop
    - Depends on: Task 14

16. [ ] **Add Timer to PluginEditor for 60fps FIFO reads**
    - Files: Source/PluginEditor.h, Source/PluginEditor.cpp
    - Inherit from juce::Timer
    - startTimerHz(60) in constructor
    - timerCallback() reads from FIFO
    - Depends on: Task 14

17. [ ] **Emit visualization data to WebView**
    - Files: Source/PluginEditor.cpp
    - Build JSON strings for fft[] and transients[]
    - webView->emitEventIfBrowserIsVisible("visualizationUpdate", data)
    - Add 100ms delay after pageFinishedLoading (JUCE 8 pattern)
    - Depends on: Task 16

18. [ ] **Create WebGL spectrogram renderer**
    - Files: Resources/ui/js/components/Spectrogram.js
    - WebGL2 context (fallback to WebGL1)
    - Circular buffer texture (512×257, R32F or RGBA8)
    - texSubImage2D for column updates
    - Uniform for writeOffset (circular buffer position)
    - Depends on: Phase 3.1 canvas placeholder

19. [ ] **Implement fragment shader with inferno colormap**
    - Files: Resources/ui/shaders/spectrogram.frag (inline in JS)
    - Logarithmic frequency mapping (Y-axis)
    - dB scaling (0-60dB range)
    - Inferno colormap (perceptually uniform)
    - Depends on: Task 18

20. [ ] **Add transient heat overlay**
    - Files: Resources/ui/js/components/Spectrogram.js, shader
    - Second texture for transient data (32 bands, interpolated to 257)
    - Heat colormap (black → red → orange → yellow)
    - Additive blend with spectrogram
    - heatIntensity uniform for blend control
    - Depends on: Tasks 18, 19

21. [ ] **Listen for visualization events in JavaScript**
    - Files: Resources/ui/js/app.js
    - window.__JUCE__.backend.addEventListener("visualizationUpdate", ...)
    - Parse JSON fft/transient arrays
    - Call spectrogram.addFrame()
    - Depends on: Tasks 17, 18, 20

22. [ ] **Implement requestAnimationFrame render loop**
    - Files: Resources/ui/js/app.js
    - render() calls spectrogram.draw()
    - requestAnimationFrame(render)
    - Start after WebView ready
    - Depends on: Task 21

23. [ ] **Add WebGL context loss handling**
    - Files: Resources/ui/js/components/Spectrogram.js
    - Handle webglcontextlost event
    - Reinitialize on webglcontextrestored
    - Depends on: Task 18

### Phase 3.3 Success Criteria

- [ ] Spectrogram scrolls smoothly at 60fps
- [ ] Audio input produces visible spectrogram display
- [ ] Frequency axis is logarithmic (kick at bottom, cymbals at top)
- [ ] Transient heat overlay shows red on drum attacks
- [ ] No audio dropouts during visualization
- [ ] UI remains responsive while audio plays
- [ ] WebGL context loss recovers gracefully

### Phase 3.3 Commit

```
feat(O-SpectralShaper): Phase 3.3 - Real-time WebGL spectrogram with transient heat overlay
```

---

## File Summary

### New Files

| File | Phase | Purpose |
|------|-------|---------|
| Resources/ui/images/paper-bg.webp | 3.1 | Darkened paper texture |
| Resources/ui/images/slug-overlay.webp | 3.1 | Ethereal slug illustration |
| Resources/ui/css/styles.css | 3.1 | Dark botanical theme |
| Resources/ui/js/components/RotaryKnob.js | 3.1 | Rotary knob component |
| Resources/ui/js/app.js | 3.1 | Main JavaScript entry |
| Resources/ui/js/components/CurveEditor.js | 3.2 | Base curve editor |
| Resources/ui/js/components/FreehandCurve.js | 3.2 | Freehand drawing mode |
| Resources/ui/js/components/NodeCurve.js | 3.2 | Node editing mode |
| Resources/ui/js/components/Spectrogram.js | 3.3 | WebGL spectrogram |

### Modified Files

| File | Phase | Changes |
|------|-------|---------|
| CMakeLists.txt | 3.1 | Add binary data for images |
| Resources/ui/index.html | 3.1, 3.2 | Replace placeholder with full UI |
| Source/PluginEditor.h | 3.1, 3.2, 3.3 | Timer, native functions |
| Source/PluginEditor.cpp | 3.1, 3.2, 3.3 | Native functions, Timer callback |
| Source/PluginProcessor.h | 3.3 | Visualization structures |
| Source/PluginProcessor.cpp | 3.3 | FIFO write in processBlock |
| Source/STFTProcessor.h | 3.3 | getLastMagnitudes() accessor |
| Source/STFTProcessor.cpp | 3.3 | Store last magnitudes |

---

## Dependencies Graph

```
Phase 3.1 (Layout)
├── Task 1 (Assets)
│   └── Task 2 (CSS)
│       └── Task 3 (HTML Layout)
│           └── Task 4 (Knob Component)
│               └── Task 5 (Wire Relays)
└── Task 6 (Editor Dimensions) [parallel]

Phase 3.2 (Curves) [depends on Phase 3.1]
├── Task 7 (CurveEditor Base)
│   ├── Task 8 (Freehand)
│   └── Task 9 (Node)
├── Task 10 (Mode Toggle) [depends on 8, 9]
├── Task 11 (C++ Native Functions) [parallel with 7-10]
├── Task 12 (JS→C++ Connection) [depends on 8, 9, 10, 11]
└── Task 13 (C++→JS Init) [depends on 12]

Phase 3.3 (Spectrogram) [depends on Phase 3.2]
├── Task 14 (Processor Viz Structures)
│   └── Task 15 (FIFO Write)
│       └── Task 16 (Editor Timer)
│           └── Task 17 (Emit to WebView)
├── Task 18 (WebGL Renderer) [parallel]
│   ├── Task 19 (Colormap Shader)
│   └── Task 20 (Heat Overlay)
├── Task 21 (Event Listener) [depends on 17, 18, 20]
├── Task 22 (RAF Loop) [depends on 21]
└── Task 23 (Context Loss) [depends on 18]
```

---

## Risk Mitigations

| Risk | Mitigation |
|------|------------|
| WebGL not supported | Detect via canvas.getContext('webgl2') and fallback to Canvas 2D |
| 60fps stuttering | Reduce to 30fps, downsample FFT from 257 to 64 bins |
| Curve sync glitches | Throttle JS→C++ updates to 30/sec during drag |
| Memory leaks | Delete WebGL textures in cleanup, use weak references |
| Large JSON payloads | Pre-serialize in C++, consider binary ArrayBuffer |

---

## Estimated Effort

| Phase | Complexity | Tasks | Notes |
|-------|------------|-------|-------|
| 3.1 | Medium | 6 | Foundation already in place from Stage 1 |
| 3.2 | High | 7 | Novel curve editor with C++ sync |
| 3.3 | High | 10 | WebGL + real-time data pipeline |

---

## Next Steps

Run `/plugin-execute O-SpectralShaper 3-gui` to begin Phase 3.1 execution.

---

*Plan created: 2026-02-04*
*Ready for execution*
