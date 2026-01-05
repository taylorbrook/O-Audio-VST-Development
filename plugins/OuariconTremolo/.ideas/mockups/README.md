# OuariconTremolo UI Mockups

## Mockup Versions

### v1 - Initial Design (Standalone Prototype)
- **v1-ui.yaml**: Complete UI specification (600+ lines)
- **v1-ui-test.html**: Standalone interactive prototype (no JUCE integration)
- **v1-ui-mockup.jpg**: Visual reference screenshot

This version is a fully functional visual prototype but uses standalone JavaScript. Not production-ready for JUCE integration.

### v2 - JUCE WebView Integration (Production Ready) ✅
- **v2-ui-test.html**: Production-ready mockup with proper JUCE WebView interop
- **js/juce/index.js**: JUCE WebView bridge (SliderState, ToggleState, ComboBoxState)
- **js/juce/check_native_interop.js**: JUCE connection verification
- **img/**: Botanical assets (paper.jpg, carrot.png)

This version uses the official JUCE WebView relay pattern and is ready for Stage 3 integration.

## JUCE WebView Interop Pattern

### Parameter Bindings

All 6 parameters use WebView relays for bidirectional C++ ↔ JavaScript communication:

1. **Speed Knob** → `WebSliderRelay("speed")`
   - JavaScript: `getSliderState('speed')`
   - Range: 0.1-20.0 Hz (normalized to 0.0-1.0)
   - Interaction: Vertical drag with `sliderDragStarted()` / `sliderDragEnded()`

2. **Depth Knob** → `WebSliderRelay("depth")`
   - JavaScript: `getSliderState('depth')`
   - Range: 0-100% (normalized to 0.0-1.0)
   - Interaction: Vertical drag with gesture tracking

3. **Waveform Selector** → `WebComboBoxRelay("waveform")`
   - JavaScript: `getComboBoxState('waveform')`
   - Choices: Sine, Triangle, Phasor, Noise, Square, Pulse (indices 0-5)
   - Interaction: Dropdown selection, updates visualizer immediately

4. **Smoothing Slider** → `WebSliderRelay("smoothing")`
   - JavaScript: `getSliderState('smoothing')`
   - Range: 0-100% (normalized to 0.0-1.0)
   - Interaction: Horizontal drag, updates visualizer

5. **Pan Sync Button** → `WebToggleButtonRelay("panSync")`
   - JavaScript: `getToggleState('panSync')`
   - Values: true/false
   - Interaction: Click to toggle, visual active state

6. **Tempo Sync Button** → `WebToggleButtonRelay("tempoSync")`
   - JavaScript: `getToggleState('tempoSync')`
   - Values: true/false
   - Interaction: Click to toggle, visual active state

### Communication Flow

**C++ → JavaScript (Parameter Automation)**
```
APVTS parameter changes → WebSliderRelay → emitEvent() →
JavaScript valueChangedEvent listener → updateVisual()
```

**JavaScript → C++ (User Interaction)**
```
User drags knob → setNormalisedValue() → emitEvent() →
WebSliderRelay → APVTS parameter update
```

### Key Functions

From `js/juce/index.js`:
- `getSliderState(name)` - Continuous parameters (knobs, sliders)
- `getToggleState(name)` - Boolean parameters (toggle buttons)
- `getComboBoxState(name)` - Choice parameters (dropdowns)

### Automation Gesture Tracking

For proper DAW automation recording, slider interactions use:
```javascript
speedKnob.addEventListener('mousedown', () => {
    speedState.sliderDragStarted(); // Begin automation gesture
});

document.addEventListener('mouseup', () => {
    speedState.sliderDragEnded(); // End automation gesture
});
```

This ensures DAW automation lanes record parameter changes as continuous gestures, not individual steps.

## Integration Checklist for gui-agent (Stage 3)

When implementing WebView UI in Stage 3, gui-agent should:

1. ✅ Create `WebSliderRelay` objects for: "speed", "depth", "smoothing"
2. ✅ Create `WebToggleButtonRelay` objects for: "panSync", "tempoSync"
3. ✅ Create `WebComboBoxRelay` object for: "waveform"
4. ✅ Add all relays to `WebBrowserComponent::Options` via `withOptionsFrom()`
5. ✅ Ensure relay names match JavaScript `getSliderState()` / `getToggleState()` / `getComboBoxState()` calls
6. ✅ Copy `v2-ui-test.html` to `Source/ui/public/index.html`
7. ✅ Copy `js/juce/` directory to `Source/ui/public/js/juce/`
8. ✅ Copy `img/` assets to `Source/ui/public/img/`
9. ✅ Configure WebBrowserComponent to load `index.html`
10. ✅ Test bidirectional parameter sync (automation → UI, UI → parameter)

## Directory Structure (Production)

After Stage 3 implementation:
```
OuariconTremolo/
├── Source/
│   ├── PluginProcessor.h/cpp    # APVTS + DSP
│   ├── PluginEditor.h/cpp       # WebBrowserComponent + Relays
│   └── ui/
│       └── public/
│           ├── index.html       # Main UI (from v2-ui-test.html)
│           ├── js/
│           │   └── juce/
│           │       ├── index.js           # WebView bridge
│           │       └── check_native_interop.js
│           └── img/
│               ├── paper.jpg    # Background texture
│               └── carrot.png   # Botanical overlay
├── CMakeLists.txt
└── .ideas/
    └── mockups/                 # Design references (not compiled)
```

## Testing the Mockup

### Standalone Browser Test
Open `v2-ui-test.html` in a browser:
- ✅ Visual design renders correctly
- ✅ Botanical aesthetics (paper texture, carrot overlay, motifs)
- ✅ Canvas waveform visualizer draws all 6 waveform types
- ⚠️ Parameter bindings will show console warnings (expected - no JUCE backend)

### JUCE Integration Test
After Stage 3 implementation in plugin:
- ✅ All controls respond to mouse interaction
- ✅ Parameter automation from DAW updates UI
- ✅ UI changes update DAW automation lanes
- ✅ Gesture tracking works (smooth automation recording)
- ✅ Preset save/load updates UI correctly
- ✅ High-DPI displays render crisp (canvas scaling)

## Design Notes

**Visual Theme**: Botanical Scientific
- Paper texture creates vintage manuscript aesthetic
- Carrot botanical illustration (71.25% height, 35% opacity)
- Unicode botanical motifs: ❦ (fleuron), ✿ (floral), ❧ (leaf)
- Earthy botanical green palette (#8BA870, #6B8E23, #3C5C1A)
- Baskerville typeface (1757, authentic 18th-century botanical publication typography)

**Interaction Design**:
- Knobs: Vertical drag, -140° to +140° rotation (280° total travel)
- Hover states: Subtle scale (1.05x) and shadow lift
- Active toggles: Darker green background, border color shift
- Real-time feedback: Waveform visualizer updates on every parameter change

**Performance**:
- Canvas uses `devicePixelRatio` scaling for Retina displays
- Waveform redraw throttled to parameter changes (not continuous)
- Smoothing interpolation applied in visualization, matches DSP algorithm

## References

- JUCE WebView documentation: `juce::WebBrowserComponent`
- WebView relay examples: `plugins/tache_plugins/*/Source/ui/public/`
- JUCE 8 WebView API: `juce_gui_extra/native/juce_WebBrowserComponent.h`
