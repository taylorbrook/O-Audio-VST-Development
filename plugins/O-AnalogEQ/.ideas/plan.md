# OuariconAnalogEQ - Implementation Plan

**Date:** 2026-01-11
**Complexity Score:** 4.0 (Complex)
**Strategy:** Phase-based implementation

---

## Complexity Factors

**Calculation breakdown:**

- **Parameters:** 16 parameters (16/5 = 3.2, capped at 2.0) = **2.0**
- **Algorithms:** 2 DSP algorithm types = **2**
  - IIR biquad filtering (shelving + parametric, all 4 bands use same JUCE API)
  - Waveshaper saturation (tanh soft clipping)
- **Features:** 0 points (no complex features)
  - No feedback loops
  - No FFT/frequency domain processing
  - No multiband (sequential, not parallel split/combine)
  - No modulation systems (LFO/envelope)
  - No external MIDI control
- **Total:** 2.0 + 2 + 0 = **4.0** (no cap applied)

**Classification:** Complex (score ≥ 3.0) → Phase-based implementation

---

## Stages

- Stage 0: Research ✓
- Stage 1: Planning ✓
- Stage 1: Foundation ← Next
- Stage 2: Shell
- Stage 3: DSP (Phase 4.1)
- Stage 3: DSP (Phase 4.2)
- Stage 3: GUI (Phase 5.1)
- Stage 3: GUI (Phase 5.2)
- Stage 3: Validation

---

## Stage 3: DSP Phases

### Phase 4.1: Core EQ Processing (Single Band Validation)

**Goal:** Implement and validate one complete EQ band (LF shelf) with parameter control and bypass

**Components:**
- LF shelf IIR filter with coefficient calculation
- Parameter reads from APVTS (`lf_freq`, `lf_gain`, `lf_on`)
- Hard bypass logic (skip processing when `lf_on = false`)
- Output gain (post-EQ trim)
- Basic signal flow: Input → LF Filter → Output Gain → Output

**Test Criteria:**
- [ ] Plugin loads in DAW without crashes
- [ ] Audio passes through cleanly (no artifacts, clicks, pops)
- [ ] LF frequency parameter (30-500 Hz) changes filter corner frequency audibly
- [ ] LF gain parameter (±12 dB) boosts/cuts low frequencies correctly
- [ ] LF bypass toggle (`lf_on`) enables/disables band without clicks
- [ ] Output gain parameter (±12 dB) adjusts final level correctly
- [ ] No denormal CPU spikes (verify with ScopedNoDenormals)
- [ ] Stable across all sample rates (44.1kHz, 48kHz, 96kHz, 192kHz)

**Git commit after this phase:** "feat(OuariconAnalogEQ): Phase 4.1 - LF shelf filter and output gain implemented"

---

### Phase 4.2: Full EQ Chain (Remaining 3 Bands)

**Goal:** Add LMF, HMF, HF bands and verify sequential processing

**Components:**
- LMF bell filter with Q parameter (`lmf_freq`, `lmf_gain`, `lmf_q`, `lmf_on`)
- HMF bell filter with Q parameter (`hmf_freq`, `hmf_gain`, `hmf_q`, `hmf_on`)
- HF shelf filter (`hf_freq`, `hf_gain`, `hf_on`)
- Sequential processing chain: LF → LMF → HMF → HF → Output Gain
- Q mapping: Choice parameter (0=0.5, 1=1.0, 2=2.0)

**Test Criteria:**
- [ ] All 4 EQ bands process correctly (LF, LMF, HMF, HF)
- [ ] Q parameter on LMF/HMF changes filter bandwidth audibly
  - [ ] WIDE (Q=0.5): Broad, gentle curves
  - [ ] MED (Q=1.0): Balanced, musical curves
  - [ ] TIGHT (Q=2.0): Focused, surgical curves
- [ ] Sequential processing order is correct (low-to-high frequency)
- [ ] All bypass combinations work without clicks
  - [ ] All bands on
  - [ ] Individual bands bypassed (test all 4 combinations)
  - [ ] All bands off (unity gain passthrough)
- [ ] No frequency masking or band interaction issues
- [ ] Extreme settings are stable (high Q + high gain, all frequencies)
- [ ] Cumulative boost doesn't cause clipping (test +12dB on all bands)

**Git commit after this phase:** "feat(OuariconAnalogEQ): Phase 4.2 - Full 4-band EQ chain implemented"

---

### Phase 4.3: Analog Saturation

**Goal:** Add analog warmth circuit and validate post-EQ saturation

**Components:**
- Waveshaper with tanh transfer function (`juce::dsp::WaveShaper`)
- Transfer function: `tanh(input * 1.5) * 1.1`
- Analog bypass toggle (`analog` parameter)
- Processing order: EQ Section → Saturation → Output Gain

**Test Criteria:**
- [ ] Saturation adds harmonic warmth (audible on sine wave test)
- [ ] Saturation is subtle at nominal levels (not aggressive distortion)
- [ ] Saturation intensifies with EQ boost (high gain → more saturation)
- [ ] Analog bypass works without clicks or level jumps
- [ ] No aliasing artifacts (test with high-frequency content)
  - [ ] Listen for "graininess" or high-frequency "hash"
  - [ ] Test with 0dBFS square wave input
- [ ] CPU usage acceptable (< 15% single core @ 48kHz)
- [ ] A/B against Waves V-EQ4 saturation (reference comparison)

**Git commit after this phase:** "feat(OuariconAnalogEQ): Phase 4.3 - Analog saturation circuit implemented"

---

## Stage 3: GUI Phases

### Phase 5.1: Layout and Basic Controls

**Goal:** Integrate v3 mockup HTML and render all UI elements

**Components:**
- Copy `v3-ui.yaml` mockup to `Source/ui/public/index.html` (convert from YAML spec)
- Implement WebView setup in PluginEditor.h/cpp
- Configure CMakeLists.txt for WebView resources (BinaryData)
  - Add `paper1.jpg` background texture
  - Add `flower_ferdinandibauer00baue_0021.png` botanical overlay
  - Add HTML/CSS/JS files
- Render dual-layer knobs (seed cross-section pattern)
- Render Q toggles (3-way switches for LMF/HMF)
- Render band enable toggles
- Render output gain knob (standard single-layer)
- Render analog toggle (large botanical toggle)
- Render VU meter (circular meter, far right)

**Test Criteria:**
- [ ] WebView window opens with correct size (920×220px)
- [ ] Paper texture background renders correctly
- [ ] Large rotated botanical overlay displays at 45% opacity
- [ ] All 4 dual-layer knobs visible and styled correctly
  - [ ] Outer ring: 10-segment conic gradient (aged paper colors)
  - [ ] Inner dial: seed cross-section pattern with center core
- [ ] Q toggles render as 3-position switches (WIDE/MED/TIGHT)
- [ ] Band enable toggles render with correct colors (active = green, inactive = walnut)
- [ ] Output gain knob renders as standard single-layer knob
- [ ] Analog toggle renders as large botanical toggle
- [ ] VU meter renders as circular meter with scale marks
- [ ] Layout matches mockup design (920×220px compact rack-unit)
- [ ] No console errors (check browser dev tools in WebView)

**Git commit after this phase:** "feat(OuariconAnalogEQ): Phase 5.1 - WebView UI layout implemented (v3 mockup)"

---

### Phase 5.2: Parameter Binding and Interaction

**Goal:** Two-way parameter communication (UI ↔ DSP)

**Components:**
- Create WebSliderRelay for each frequency/gain/output knob (11 relays)
  - LF: `lf_freq`, `lf_gain`
  - LMF: `lmf_freq`, `lmf_gain`
  - HMF: `hmf_freq`, `hmf_gain`
  - HF: `hf_freq`, `hf_gain`
  - Output: `output_gain`
- Create WebToggleState for each boolean parameter (5 toggles)
  - Band enables: `lf_on`, `lmf_on`, `hmf_on`, `hf_on`
  - Analog: `analog`
- Create WebComboBoxState for Q parameters (2 choice params)
  - `lmf_q`, `hmf_q`
- Implement JavaScript drag handlers for dual-layer knobs
  - Outer ring: Frequency control (relative drag, log scale)
  - Inner dial: Gain control (relative drag, linear dB scale)
  - Independent rotation for each layer
- Implement JavaScript click handlers for toggles and Q selectors
- C++ → JS parameter updates (host automation, preset changes)
- JS → C++ parameter changes (UI control movements)

**Test Criteria:**
- [ ] Dual-layer knob interactions work correctly
  - [ ] Outer ring drag changes frequency parameter (log scale feels natural)
  - [ ] Inner dial drag changes gain parameter (smooth, proportional)
  - [ ] Both layers rotate independently (no interference)
  - [ ] Click and drag on correct zone (outer vs inner) activates correct control
- [ ] Q toggles change parameter value (0 → 1 → 2 → 0 cycle)
- [ ] Band enable toggles change boolean parameters (on/off)
- [ ] Analog toggle changes boolean parameter (on/off)
- [ ] Host automation updates UI controls in real-time
  - [ ] Automate frequency parameter → outer ring rotates
  - [ ] Automate gain parameter → inner dial rotates
  - [ ] Automate boolean → toggle visual state changes
- [ ] Preset changes update all UI elements instantly
- [ ] Parameter values display correctly (tooltips or value labels)
  - [ ] Frequency: Display Hz value (e.g., "100 Hz", "2.5 kHz", "10 kHz")
  - [ ] Gain: Display dB value (e.g., "+3.5 dB", "-6.0 dB")
  - [ ] Q: Display text (WIDE/MED/TIGHT)
- [ ] No lag or visual glitches during parameter changes
- [ ] No clicks/pops when changing parameters during playback

**Git commit after this phase:** "feat(OuariconAnalogEQ): Phase 5.2 - WebView parameter binding implemented"

---

## Implementation Flow

- Stage 1: Foundation - Project structure and CMake setup
- Stage 2: Shell - APVTS parameters (16 params)
- Stage 3: DSP - 3 phases
  - **Phase 4.1:** Core EQ Processing (LF shelf + output gain)
  - **Phase 4.2:** Full EQ Chain (LMF, HMF, HF bands)
  - **Phase 4.3:** Analog Saturation (tanh waveshaping)
- Stage 3: GUI - 2 phases
  - **Phase 5.1:** Layout and Basic Controls (WebView rendering)
  - **Phase 5.2:** Parameter Binding and Interaction (UI ↔ DSP communication)
- Stage 3: Validation - Presets, pluginval, changelog

---

## Implementation Notes

### Thread Safety

**Parameter access pattern:**
```cpp
// Audio thread - atomic reads
float lfFreq = *apvts.getRawParameterValue("lf_freq");
float lfGain = *apvts.getRawParameterValue("lf_gain");
bool lfOn = *apvts.getRawParameterValue("lf_on") > 0.5f;
```

**Coefficient updates (non-allocating):**
```cpp
// JUCE uses ref-counted pointers - no allocations
if (parameterChanged) {
    float gainFactor = std::pow(10.0f, gainDB / 20.0f);
    *lfFilter.coefficients = *juce::dsp::IIR::Coefficients<float>::makeLowShelf(
        sampleRate, lfFreq, 0.707f, gainFactor);
}
```

**No locks needed:**
- APVTS provides atomic parameter reads
- Filter coefficient assignment is ref-counted (no allocations)
- All state is per-channel (L/R independent)

---

### Performance

**Estimated CPU usage per component (48kHz, buffer size 512):**
- LF Shelf Filter: ~2% single core
- LMF Bell Filter: ~2% single core
- HMF Bell Filter: ~2% single core
- HF Shelf Filter: ~2% single core
- Analog Saturation: ~3% single core (tanh + multiply)
- Output Gain: ~0.5% single core
- **Total estimated:** ~11-12% single core at 48kHz

**Optimization notes:**
- JUCE IIR filters use SIMD vectorization on supported platforms
- All processing is in-place (no buffer copies)
- No memory allocation in audio thread
- `tanh()` is relatively expensive (~10-15 CPU cycles) but called once per sample

**If CPU is too high:**
- Remove saturation (saves ~3% CPU)
- Use lookup table for `tanh()` (saves ~2% CPU)

---

### Latency

**Total latency:** < 5 samples (~0.1ms @ 48kHz)
- IIR biquad filters: 1-2 samples group delay (minimum-phase)
- Saturation: 0 samples (memoryless function)
- Output gain: 0 samples

**Host compensation:** Not needed (latency too small to report)

```cpp
int getLatencySamples() const override {
    return 0; // Negligible latency
}
```

---

### Denormal Protection

**Strategy:** Use `juce::ScopedNoDenormals` in `processBlock()`

```cpp
void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override
{
    juce::ScopedNoDenormals noDenormals;

    // ... DSP processing
}
```

**Why needed:**
- IIR filters can produce denormal numbers (values near zero)
- Denormals cause severe CPU slowdown (100-1000x)
- `ScopedNoDenormals` sets CPU flush-to-zero mode

**Components that need protection:**
- All IIR filters (filter state can drift to denormals)
- Saturation waveshaper (input near zero)

---

### Known Challenges

**Dual-layer knob interaction (UI):**
- Challenge: Outer ring vs inner dial hit detection
- Solution: Use z-index layering + precise hit testing
- Reference: Similar to seed-style knobs in other Ouaricon plugins
- Test: Click on border → outer ring, click on center → inner dial

**Filter state reset on bypass:**
- Challenge: Bypassing filter then re-enabling can cause burst
- Solution: Don't reset filter state on bypass (preserve continuity)
- Pattern: Hard bypass (skip processing) but maintain filter state
- Test: Bypass → re-enable during playback (should be click-free)

**Q parameter mapping:**
- Challenge: Choice parameter (0/1/2) → float Q value (0.5/1.0/2.0)
- Solution: Simple switch statement in coefficient update
- Reference: architecture.md Algorithm Details section

**Saturation aliasing:**
- Challenge: Waveshaping can create high-frequency aliasing
- Mitigation: Gentle drive (1.5x), band-limited by EQ
- Test: Listen for "graininess" with extreme input levels
- Fallback: Add 2x oversampling if aliasing is audible

**VU meter visualization (UI):**
- Challenge: VU meter responds to output level (real-time audio data)
- Solution: Send level data from C++ → JS via custom event
- Pattern: Similar to VU meters in FlutterVerb, TapeAge
- Reference: juce8-critical-patterns.md Pattern 20 (requestAnimationFrame loop)

---

## References

**Contract files:**
- Creative brief: `plugins/OuariconAnalogEQ/.ideas/creative-brief.md`
- Parameter spec: `plugins/OuariconAnalogEQ/.ideas/parameter-spec.md`
- DSP architecture: `plugins/OuariconAnalogEQ/.ideas/architecture.md`
- UI mockup: `plugins/OuariconAnalogEQ/.ideas/mockups/v3-ui.yaml`

**Similar plugins for reference:**
- **FlutterVerb:** WebView dual-layer knobs (seed pattern), VU meter visualization
- **TapeAge:** WebView parameter binding, toggle switches, saturation waveshaping
- **LushVerb:** WebView ES6 module loading, relative drag knob interaction

**JUCE patterns:**
- `troubleshooting/patterns/juce8-critical-patterns.md` - Required reading before implementation
  - Pattern 1: CMake header generation (juce_generate_juce_header)
  - Pattern 8: WebView resource provider (explicit URL mapping)
  - Pattern 9: CMake NEEDS_WEB_BROWSER for VST3
  - Pattern 11: WebView member initialization (std::unique_ptr)
  - Pattern 12: WebSliderParameterAttachment (3 parameters required in JUCE 8)
  - Pattern 13: check_native_interop.js required
  - Pattern 15: valueChangedEvent callback (no parameters passed)
  - Pattern 16: Relative drag for knob interaction
  - Pattern 20: VU meter requestAnimationFrame loop
  - Pattern 21: ES6 module loading (type="module")

---

**Plan completed:** 2026-01-11
**Ready for:** Stage 1 Foundation implementation
