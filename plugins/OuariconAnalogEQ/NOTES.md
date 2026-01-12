# OuariconAnalogEQ Notes

## Status
- **Current Status:** 📦 Installed
- **Version:** 1.0.3
- **Type:** Audio Effect (4-Band Parametric/Shelving EQ)
- **Complexity Score:** 4.0 (Complex - Phase-based implementation)

## Lifecycle Timeline

- **2026-01-11 (Ideation):** Creative brief created - Neve 1081-inspired analog EQ
- **2026-01-11 (UI Mockup v1):** Initial design iteration - dual-layer knobs
- **2026-01-11 (UI Mockup v2):** Second design iteration - layout refinements
- **2026-01-11 (UI Mockup v3):** Finalized mockup - paper texture + large rotated botanical overlay
- **2026-01-11 (Parameter Spec):** Finalized 16 parameters (10 float, 2 choice, 5 bool)
- **2026-01-11 (Stage 0):** Research & Planning complete - Architecture and plan documented (Complexity 4.0)
- **2026-01-11 (Stage 1):** Foundation + Shell complete
- **2026-01-11 (Stage 2):** DSP Implementation complete
- **2026-01-11 (Stage 3):** GUI Integration complete
- **2026-01-11 (v1.0.0):** Initial release - Installed to system folders
- **2026-01-11 (v1.0.1):** Fixed GUI connectivity - Q parameters now use correct WebComboBoxRelay
- **2026-01-11 (v1.0.2):** Added missing check_native_interop.js - WebView bridge now functional
- **2026-01-11 (v1.0.3):** Fixed dual-layer knobs + live VU meter with Marimba-style C++ events

## Known Issues

None

## Additional Notes

### Description

A lightweight, knob-based 4-band EQ plugin inspired by the Waves V-EQ4 and Neve 1081 console module. Emphasizes simplicity and musicality with dual-layer knob controls and subtle analog warmth.

### Key Features

**EQ Bands:**
- **LF (Low Frequency):** Shelving filter, 30-500 Hz, ±12 dB
- **LMF (Low-Mid Frequency):** Parametric bell filter, 100-2000 Hz, ±12 dB, 3 Q settings (WIDE/MED/TIGHT)
- **HMF (High-Mid Frequency):** Parametric bell filter, 500-8000 Hz, ±12 dB, 3 Q settings (WIDE/MED/TIGHT)
- **HF (High Frequency):** Shelving filter, 2-20 kHz, ±12 dB

**Global Controls:**
- **Output Gain:** ±12 dB post-EQ trim
- **Analog Toggle:** Enable/disable subtle harmonic saturation

**Sound Character:**
- Neve-inspired musical EQ curves (minimum-phase IIR filters)
- Subtle analog warmth via tanh waveshaping
- Gentle harmonic saturation (0.5-5% THD depending on level)
- Console-style signal flow (LF → HF → Saturation → Output)

### Parameters

Total: 16 parameters (10 float, 2 choice, 5 bool)

**Frequency Parameters (4):**
- `lf_freq`: 30-500 Hz (log scale)
- `lmf_freq`: 100-2000 Hz (log scale)
- `hmf_freq`: 500-8000 Hz (log scale)
- `hf_freq`: 2000-20000 Hz (log scale)

**Gain Parameters (5):**
- `lf_gain`: ±12 dB (linear dB)
- `lmf_gain`: ±12 dB (linear dB)
- `hmf_gain`: ±12 dB (linear dB)
- `hf_gain`: ±12 dB (linear dB)
- `output_gain`: ±12 dB (linear dB)

**Q Parameters (2):**
- `lmf_q`: Choice (0=WIDE/0.5, 1=MED/1.0, 2=TIGHT/2.0)
- `hmf_q`: Choice (0=WIDE/0.5, 1=MED/1.0, 2=TIGHT/2.0)

**Toggle Parameters (5):**
- `lf_on`: Band enable (default: On)
- `lmf_on`: Band enable (default: On)
- `hmf_on`: Band enable (default: On)
- `hf_on`: Band enable (default: On)
- `analog`: Saturation enable (default: On)

### DSP Architecture

**Processing Chain:**
```
Input → LF Shelf → LMF Bell → HMF Bell → HF Shelf → Saturation → Output Gain → Output
```

**JUCE Components:**
- `juce::dsp::IIR::Filter` - All 4 EQ bands (shelving + parametric)
  - Coefficients: `makeLowShelf()`, `makeHighShelf()`, `makePeakFilter()`
- `juce::dsp::WaveShaper` - Analog saturation (`tanh(x * 1.5) * 1.1`)
- `juce::dsp::Gain` - Output trim

**Filter Details:**
- LF/HF shelving: Fixed Q = 0.707 (Butterworth, musical slope)
- LMF/HMF parametric: Variable Q = 0.5/1.0/2.0 (user selectable)
- All filters are IIR biquads (minimum-phase, analog-like)
- Hard bypass when band disabled (skip processing, not mute)

**Saturation:**
- Transfer function: `tanh(x * 1.5) * 1.1`
- Drive: 1.5x (gentle, not aggressive)
- Post-gain: 1.1x (compensate tanh output range)
- Position: Post-EQ (saturate EQ'd signal, console workflow)
- No oversampling (gentle drive, band-limited by EQ)

**Performance:**
- Estimated CPU: ~11-12% single core @ 48kHz
- Latency: < 5 samples (~0.1ms, negligible)
- Denormal protection: `ScopedNoDenormals` in processBlock

### GUI Design

**Layout:** Compact rack-unit (920×220px)
- Paper texture background (`paper1.jpg`)
- Large rotated botanical overlay (90° clockwise, 45% opacity)
- Single horizontal row with tight vertical spacing

**Controls:**
- **4 Dual-Layer Knobs:** Seed cross-section pattern
  - Outer ring: Frequency (10-segment conic gradient)
  - Inner dial: Gain (center core + radial segments)
  - Independent rotation for each layer
- **2 Q Toggles:** 3-way switches (WIDE/MED/TIGHT) for LMF/HMF
- **5 Band Toggles:** Enable/disable per band (botanical green when active)
- **1 Output Gain Knob:** Standard single-layer knob
- **1 Analog Toggle:** Large botanical toggle
- **1 VU Meter:** Circular meter (far right, responds to output level)

**Color Palette:** Aged paper (walnut brown, cream, botanical green)

### Implementation Strategy

**Complexity:** 4.0 (Complex) → Phase-based implementation

**DSP Phases:**
- **Phase 4.1:** Core EQ Processing (LF shelf + output gain validation)
- **Phase 4.2:** Full EQ Chain (LMF, HMF, HF bands)
- **Phase 4.3:** Analog Saturation (tanh waveshaping)

**GUI Phases:**
- **Phase 5.1:** Layout and Basic Controls (WebView rendering, v3 mockup)
- **Phase 5.2:** Parameter Binding (dual-layer knobs, toggles, VU meter)

### Professional References

- **UAD Neve 1081 Classic Console EQ:** Primary inspiration (shelving filters, switchable to bell)
- **Waves V-EQ4:** Neve 1081 emulation (±18dB shelving, analog saturation)
- **FabFilter Pro-Q:** Surgical EQ (opposite design goal - too clean/transparent)

### Technical Specifications

- **Plugin Formats:** VST3, AU (planned)
- **Sample Rates:** 44.1kHz - 192kHz
- **Latency:** Zero (< 5 samples, not reported to host)
- **CPU:** Lightweight (~11-12% single core @ 48kHz)
- **Thread Safety:** Atomic parameter reads, non-allocating coefficient updates

### Validation Checklist (Stage 6)

- [ ] Build successful (CMake + JUCE 8)
- [ ] VST3 and AU formats built
- [ ] Pluginval passes (all tests green)
- [ ] No memory leaks (Valgrind or Instruments)
- [ ] Presets created (at least 5 factory presets)
- [ ] Changelog created (CHANGELOG.md)
- [ ] Installation successful (VST3 + AU in system folders)
- [ ] DAW compatibility tested (Logic Pro, Ableton Live)

### Contract Files

- Creative brief: `plugins/OuariconAnalogEQ/.ideas/creative-brief.md`
- Parameter spec: `plugins/OuariconAnalogEQ/.ideas/parameter-spec.md`
- DSP architecture: `plugins/OuariconAnalogEQ/.ideas/architecture.md`
- Implementation plan: `plugins/OuariconAnalogEQ/.ideas/plan.md`
- UI mockup: `plugins/OuariconAnalogEQ/.ideas/mockups/v3-ui.yaml`

### Next Steps

1. Run `/implement OuariconAnalogEQ` to start Stage 1 (Foundation + Shell)
2. Implement DSP in 3 phases (4.1, 4.2, 4.3)
3. Implement GUI in 2 phases (5.1, 5.2)
4. Validate with pluginval, create presets, changelog
5. Install and test in DAW

---

**Last Updated:** 2026-01-11
**Version:** 1.0.2
**Status:** 📦 Installed
