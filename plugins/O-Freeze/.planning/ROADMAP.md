# O-Freeze - Implementation Plan

**Date:** 2026-02-01
**Complexity Score:** 5.0 (Complex - capped at maximum)
**Strategy:** Phase-based implementation

---

## Complexity Factors

- **Parameters:** 5 parameters (5/5 = 1.0 points, capped at 2.0) = 1.0
- **Algorithms:** 3 DSP components = 3
  - Granular Engine (custom overlap-add with Hann windowing)
  - Threshold Gate (RMS detection + state machine with hysteresis)
  - Crossfade System (freeze engage/disengage smoothing)
- **Features:** 2 points
  - Custom granular synthesis (+1, no JUCE built-in class)
  - Modulation system (drift randomization) (+1)
- **Total:** 1.0 + 3 + 2 = 6.0 (capped at 5.0)

**Classification:** Complex (≥ 3.0) → Phase-based implementation required

---

## Stages

- Stage 0: Research ✓
- Stage 1: Planning ✓
- Stage 1: Foundation ← Next
- Stage 2: Shell
- Stage 3: DSP (3 phases)
- Stage 3: GUI (2 phases)
- Stage 3: Validation

---

## Complex Implementation (Score = 5.0)

### Stage 3: DSP Phases

#### Phase 4.1: Core Processing (Freeze Buffer + Simple Playback)

**Goal:** Implement freeze trigger, circular buffer capture, and simple buffer loop playback (no granular yet)

**Components:**
- Circular freeze buffer (juce::AudioBuffer, 2-second capacity)
- Freeze trigger detection (FREEZE button in Manual mode)
- Simple buffer loop playback (loop through frozen buffer, no windowing)
- Basic crossfade system (freeze engage/disengage smoothing)
- Dry/wet mixer (MIX parameter)

**Test Criteria:**
- [ ] Plugin loads in DAW without crashes
- [ ] Audio passes through when freeze inactive (dry signal audible)
- [ ] FREEZE button triggers buffer capture (locks buffer write position)
- [ ] Frozen buffer loops audibly (expect loop seam/click - acceptable for Phase 4.1)
- [ ] Crossfade system prevents hard clicks on freeze engage/disengage
- [ ] MIX parameter blends dry and frozen signals correctly
- [ ] No memory leaks or buffer overruns

**Implementation notes:**
- Start with simple loop to validate freeze trigger and buffer management
- Loop seam artifact is expected (will be fixed in Phase 4.2 with granular)
- Focus on correct buffer indexing and wraparound logic

---

#### Phase 4.2: Granular Engine (Overlap-Add Synthesis)

**Goal:** Replace simple buffer loop with granular engine (Hann windowing, overlap-add)

**Components:**
- Grain generator (50ms grain size, 4 simultaneous grains)
- Hann window (pre-computed lookup table)
- Grain scheduler (trigger new grain every 12.5ms for 75% overlap)
- Overlap-add summing (normalize by grain count)
- Drift modulation (DRIFT parameter, randomize grain read positions)

**Test Criteria:**
- [ ] Frozen texture is smooth (no audible loop seam or grain boundaries)
- [ ] No amplitude modulation artifacts (tremolo effect)
- [ ] DRIFT parameter works correctly:
  - [ ] 0% = perfectly static freeze (all grains read same position)
  - [ ] 100% = maximum drift (grains read across entire buffer)
  - [ ] 50% = balanced drift (subtle movement without chaos)
- [ ] No clicks or pops when grains start/end (Hann window zero-crossing verified)
- [ ] CPU usage acceptable (~15-20% single core at 48kHz)
- [ ] Grain timing is stable (no jitter or irregular texture)

**Implementation notes:**
- Pre-compute Hann window in prepareToPlay() (no runtime calculation)
- Use fixed grain size (50ms = 2400 samples @ 48kHz) for simplicity
- Start with 4 grains (75% overlap), test 2 and 8 grains if issues
- Verify overlap-add normalization prevents clipping

---

#### Phase 4.3: Threshold Gate (Auto-Freeze Mode)

**Goal:** Add threshold-based auto-freeze trigger (Threshold mode)

**Components:**
- RMS level detection (20ms rolling window)
- Threshold comparison (user-adjustable -60dB to 0dB)
- State machine with hysteresis (3dB gap between engage/release)
- MODE parameter (Manual vs. Threshold)
- Threshold gate crossfade integration (same crossfade system as manual freeze)

**Test Criteria:**
- [ ] MODE parameter switches between Manual and Threshold modes correctly
- [ ] Threshold mode triggers freeze when input level drops below THRESHOLD
- [ ] Freeze releases when input level rises above THRESHOLD + 3dB (hysteresis)
- [ ] No rapid on/off cycling (fluttering) when input hovers near threshold
- [ ] RMS window (20ms) averages out transients (doesn't false-trigger on drum hits)
- [ ] Crossfade system smoothly engages/disengages freeze in both modes
- [ ] THRESHOLD parameter range (-60dB to 0dB) works correctly

**Implementation notes:**
- Implement peak detection first, then replace with RMS if needed
- Test hysteresis with various input material (drums, pads, ambient)
- Document recommended threshold settings (-40dB to -20dB for most material)
- UI should disable inactive controls based on MODE (FREEZE button disabled in Threshold mode, THRESHOLD knob disabled in Manual mode)

---

### Stage 3: GUI Phases

#### Phase 5.1: Layout and Basic Controls

**Goal:** Integrate mockup HTML and bind basic parameters (THRESHOLD, DRIFT, MIX)

**Components:**
- Copy v[N]-ui.html to Source/ui/public/index.html (if mockup exists)
- Update PluginEditor.h/cpp with WebView setup
- Configure CMakeLists.txt for WebView resources (BinaryData)
- Bind THRESHOLD, DRIFT, MIX parameters via WebSliderRelay
- Implement MODE parameter toggle (Manual/Threshold button)
- Implement FREEZE button (Manual mode only)

**Test Criteria:**
- [ ] WebView window opens with correct size (400x300 typical)
- [ ] All basic controls visible and styled correctly
- [ ] Layout matches mockup design (if mockup exists)
- [ ] Background and styling render properly
- [ ] Knobs/sliders respond to mouse drag (parameter binding verified)
- [ ] MODE toggle switches between Manual and Threshold correctly
- [ ] FREEZE button is disabled in Threshold mode (UI feedback)

**Implementation notes:**
- If no mockup exists, create minimal HTML layout (3 knobs, 1 toggle, 1 button)
- Use WebSliderRelay for continuous parameters (THRESHOLD, DRIFT, MIX)
- Use WebToggleRelay or custom button relay for FREEZE and MODE
- Ensure FREEZE button shows visual state (active/inactive, enabled/disabled)

---

#### Phase 5.2: Parameter Binding and Interaction

**Goal:** Two-way parameter communication (UI ↔ DSP) and visual polish

**Components:**
- JavaScript → C++ relay calls (knob/slider movements update DSP)
- C++ → JavaScript parameter updates (host automation updates UI)
- FREEZE button state synchronization (UI reflects freeze active/inactive)
- MODE parameter UI state (show/hide THRESHOLD knob vs. FREEZE button)
- Value formatting and display (dB for THRESHOLD, % for DRIFT/MIX)
- Real-time parameter updates during playback

**Test Criteria:**
- [ ] Control movements change DSP parameters (hear audio change)
- [ ] Host automation updates UI controls (knobs rotate, button state changes)
- [ ] Preset changes update all UI elements instantly
- [ ] Parameter values display correctly (THRESHOLD in dB, DRIFT/MIX in %)
- [ ] No lag or visual glitches (smooth knob rotation, no jumps)
- [ ] FREEZE button shows active state when frozen (visual feedback)
- [ ] MODE toggle shows/hides appropriate controls (conditional UI)

**Implementation notes:**
- Use `valueChangedEvent` listeners for C++ → JS updates (see juce8-critical-patterns.md #15)
- Ensure FREEZE button state is NOT persisted on preset save (always starts disengaged)
- Test host automation with various DAWs (Logic, Ableton, Reaper)
- Add tooltips or value displays for knobs (user knows exact parameter values)

---

### Implementation Flow

- Stage 1: Foundation - project structure (CMakeLists.txt, PluginProcessor skeleton)
- Stage 2: Shell - APVTS parameters (THRESHOLD, DRIFT, MIX, MODE, FREEZE)
- Stage 3: DSP - 3 phases
  - Phase 4.1: Core Processing (freeze buffer + simple loop)
  - Phase 4.2: Granular Engine (overlap-add + drift modulation)
  - Phase 4.3: Threshold Gate (auto-freeze mode)
- Stage 3: GUI - 2 phases
  - Phase 5.1: Layout and Basic Controls
  - Phase 5.2: Parameter Binding and Interaction
- Stage 3: Validation - presets, pluginval, changelog

---

## Implementation Notes

### Thread Safety

- All parameter reads use atomic `getRawParameterValue()->load()` (THRESHOLD, DRIFT, MIX)
- FREEZE button state: `std::atomic<bool>` (Manual mode only, checked on audio thread)
- Freeze buffer: Pre-allocated in prepareToPlay (no real-time allocation in processBlock)
- Granular engine: Grain state managed per-sample (no shared state across channels)
- Threshold gate: RMS calculation per-channel (independent stereo processing)

**Key pattern:**
```cpp
// Audio thread (processBlock)
auto thresholdParam = apvts.getRawParameterValue("THRESHOLD");
float threshold_dB = thresholdParam->load();  // Atomic read

// Message thread (UI button click)
freezeButtonState.store(true);  // Atomic write
```

---

### Performance

- **Granular engine:** ~15% CPU (4 grains @ 48kHz, 50ms grain size) - Most expensive component
- **Threshold gate:** ~2% CPU (RMS averaging over 20ms window)
- **Crossfade system:** ~1% CPU (linear interpolation per-sample)
- **Dry/Wet mixer:** ~2% CPU (JUCE DryWetMixer overhead)
- **Total estimated:** ~20% single core at 48kHz
- **Passthrough mode (freeze inactive):** ~0% CPU (granular engine bypassed)

**Optimization opportunities:**
- SIMD for grain summing (4 grains = vectorizable with SSE/NEON)
- Pre-compute Hann window (lookup table, zero runtime calculation)
- Skip granular engine when freeze inactive (zero CPU overhead)
- Consider 2-grain mode for low-CPU scenarios (acceptable quality, ~8% CPU)

---

### Latency

- **Granular engine:** ~50ms latency (grain size) when freeze active
- **Threshold gate:** ~20ms latency (RMS averaging window)
- **Total latency:** ~70ms (3360 samples @ 48kHz)
- **Passthrough mode:** 0ms latency (freeze inactive)
- **Report via:** `getLatencySamples()` for host compensation (only when freeze active)

**Implementation note:**
- Latency is dynamic (changes when freeze engages/disengages)
- May need to report worst-case latency (70ms) always to avoid host re-routing
- Test with various DAWs to verify latency compensation works correctly

---

### Denormal Protection

- Use `juce::ScopedNoDenormals` at start of processBlock() (prevents denormal slowdown)
- Threshold gate: Add 1e-6 epsilon to RMS before `log10()` (prevents log(0) denormals)
- Granular engine: No denormal risk (reading from buffer, not recursive filters)
- Crossfade system: LinearSmoothedValue handles denormals internally

**Pattern:**
```cpp
void processBlock(AudioBuffer<float>& buffer, MidiBuffer&) override {
    juce::ScopedNoDenormals noDenormals;  // Enable flush-to-zero mode
    // ... processing ...
}
```

---

### Known Challenges

1. **Grain overlap-add normalization:**
   - Challenge: Summing 4 grains can cause clipping if not normalized
   - Solution: Divide output by active grain count (`output /= num_active_grains`)
   - Test: Verify frozen output stays within [-1.0, 1.0] range

2. **Freeze buffer wraparound indexing:**
   - Challenge: Circular buffer read/write requires modulo arithmetic (easy to get wrong)
   - Solution: Use `index % buffer_size` for all buffer accesses, test with unit tests
   - Test: Verify no crashes on buffer loop boundary (write pointer wraps correctly)

3. **Threshold gate hysteresis tuning:**
   - Challenge: 3dB hysteresis may be too tight (rapid on/off cycling) or too loose (slow response)
   - Solution: Start with 3dB, test with various material, adjust to 6dB if needed
   - Test: Record drums, pads, ambient noise - verify no fluttering

4. **FREEZE button state persistence:**
   - Challenge: Should freeze state be saved with preset? (Probably NOT - always start disengaged)
   - Solution: Don't persist FREEZE button state in `getStateInformation()`
   - Test: Save preset while frozen, reload preset - verify freeze is disengaged on load

5. **MODE parameter UI state management:**
   - Challenge: UI must show/hide controls based on MODE (FREEZE button vs. THRESHOLD knob)
   - Solution: JavaScript listener on MODE parameter, conditional CSS display
   - Test: Switch MODE - verify inactive controls are disabled/hidden

---

## References

- Creative brief: `plugins/O-Freeze/.planning/BRIEF.md`
- DSP architecture: `plugins/O-Freeze/.planning/research/ARCHITECTURE.md`
- Parameter spec: (Will be created in Stage 2 Shell, extracted from BRIEF.md)
- UI mockup: (Not created yet - minimal HTML layout in Phase 5.1)

**Similar plugins for reference:**
- **GainKnob** - WebView parameter binding pattern (WebSliderRelay, valueChangedEvent)
- **TapeAge** - Crossfade system for mode switching (wet-only vs. wet+dry)
- **FlutterVerb** - Threshold parameter with dB range, RMS level detection (for VU meter)
- **LushPad** - Button state management (trigger buttons, not persisted)

**Technical references:**
- `troubleshooting/patterns/juce8-critical-patterns.md` - Critical patterns #11, #12, #15, #16
- Curtis Roads, "Microsound" - Granular synthesis theory
- JUCE documentation (/juce-framework/juce) - AudioBuffer, LinearSmoothedValue, DryWetMixer
