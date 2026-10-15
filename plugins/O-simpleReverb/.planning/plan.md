# OuariconSimpleReverb - Implementation Plan

**Date:** 2026-01-13
**Complexity Score:** 4.2 (Complex)
**Strategy:** Phase-based implementation

---

## Complexity Factors

**Calculation breakdown:**

- **Parameters:** 6 parameters (6/5 = 1.2 points, capped at 2.0) = **1.2**
  - TYPE (choice)
  - CHARACTER (float)
  - WET (float)
  - DRY (float)
  - DECAY (float)
  - SIZE (float)

- **Algorithms:** 3 DSP components = **3**
  - Reverb Engine (juce::dsp::Reverb)
  - Character Filter (juce::dsp::IIR::Filter)
  - Dry/Wet Mixer (juce::dsp::DryWetMixer)

- **Features:** 0 points (no feedback loops, FFT, multiband, modulation, MIDI)

- **Total:** 1.2 + 3 + 0 = **4.2** (capped at 5.0)

**Classification:** Complex (score ≥ 3.0)

---

## Stages

- Stage 0: Research ✓
- Stage 0: Planning ← Current
- Stage 1: Foundation ← Next
- Stage 2: Shell
- Stage 3: DSP [3 phases]
- Stage 4: GUI [2 phases]
- Stage 5: Validation

---

## Complex Implementation (Score = 4.2)

### Stage 3: DSP Phases

#### Phase 3.1: Core Processing

**Goal:** Implement basic reverb engine with single type, dry/wet mixing, and parameter control

**Components:**
- juce::dsp::Reverb with Room type preset (roomSize=0.5, damping=0.5, width=1.0)
- SIZE parameter (0-100%) scales roomSize (0.5-1.5x base)
- DECAY parameter (0.1-10s) controls damping inverse
- juce::dsp::DryWetMixer for dry/wet blending
- Separate DRY (0-100%) and WET (0-100%) parameters

**Signal Flow:**
```
Input → DryWetMixer.pushDrySamples → Reverb.process → Wet scaling → Blend with Dry → Output
```

**Test Criteria:**
- [ ] Plugin loads in DAW without crashes
- [ ] Audio passes through (dry signal audible at 100% DRY, 0% WET)
- [ ] Reverb audible at 0% DRY, 100% WET
- [ ] SIZE parameter changes room size (larger = more spacious)
- [ ] DECAY parameter changes tail length (longer = more sustain)
- [ ] DRY and WET parameters work independently
- [ ] No clicks, pops, or artifacts during playback
- [ ] Reverb sounds natural and musical

---

#### Phase 3.2: Type Switching

**Goal:** Implement all six reverb types with distinct character

**Components:**
- AudioParameterChoice for TYPE parameter (6 options)
- Parameter mapping table (TYPE → roomSize/damping/width)
- TYPE presets:
  - Booth: roomSize=0.15, damping=0.85, width=1.0
  - Room: roomSize=0.50, damping=0.50, width=1.0
  - Hall: roomSize=0.85, damping=0.20, width=1.0
  - Spring: roomSize=0.30, damping=0.45, width=0.8
  - Plate: roomSize=0.60, damping=0.30, width=1.0
  - Ambient: roomSize=0.95, damping=0.10, width=1.0

**Test Criteria:**
- [ ] TYPE dropdown works in DAW (6 options visible)
- [ ] Each type sounds distinct from others
- [ ] Booth: Tight, intimate, short reflections
- [ ] Room: Natural, versatile, balanced
- [ ] Hall: Large, long tail, diffuse
- [ ] Spring: Narrower stereo, spring-like character
- [ ] Plate: Dense, smooth, bright
- [ ] Ambient: Washy, ethereal, long diffuse tail
- [ ] TYPE changes don't cause clicks or artifacts
- [ ] SIZE and DECAY still work correctly with all types

---

#### Phase 3.3: Character Control

**Goal:** Add tonal shaping (warm/bright/neutral) to reverb tail

**Components:**
- juce::dsp::IIR::Filter with coefficient switching
- CHARACTER parameter (-100% to +100%, bipolar)
- **Warm mode (-100% to -1%):** Low-pass filter
  - Cutoff: 2kHz to 20kHz (exponential)
  - Formula: `cutoff = 20000.0f * pow(10.0f, warmValue * log10(10.0f))`
- **Neutral mode (±0.5%):** Filter bypassed
- **Bright mode (+1% to +100%):** High-shelf boost at 4kHz
  - Gain: 0dB to +6dB (linear)
  - Formula: `gainDb = brightValue * 6.0f`
- Filter state reset on mode transitions (prevent clicks)

**Test Criteria:**
- [ ] CHARACTER knob works in DAW
- [ ] Warm mode (-100%): Very dark reverb tail (2kHz cutoff)
- [ ] Warm mode (-50%): Moderate warmth (~6kHz cutoff)
- [ ] Warm mode (-10%): Subtle warmth (~12kHz cutoff)
- [ ] Neutral mode (0%): No tonal coloration (filter bypassed)
- [ ] Bright mode (+10%): Subtle airy quality
- [ ] Bright mode (+50%): Moderate brightness
- [ ] Bright mode (+100%): Very bright, airy tail (+6dB @ 4kHz)
- [ ] No clicks when sweeping through neutral zone
- [ ] No clicks when switching warm ↔ bright
- [ ] Character filter affects wet signal only (dry unchanged)

---

### Stage 4: GUI Phases

#### Phase 4.1: Layout and Basic Controls

**Goal:** Create Ouaricon Naturalist UI with all parameter controls

**Components:**
- HTML mockup (to be created during UI design)
- Ouaricon Naturalist aesthetic:
  - Aged paper background
  - Botanical illustration overlay (flora category - ethereal/flowing)
  - Seed cross-section knobs (medium 60px)
  - Garamond serif typography
  - Warm earth-tone palette with green accents
- TYPE dropdown (6 options)
- 5 knobs: CHARACTER, WET, DRY, DECAY, SIZE
- Layout suggestion:
  - TYPE dropdown at top or grouped with controls
  - Knobs in balanced arrangement (3 top, 2 bottom OR 2 top, 3 bottom)
  - Botanical overlay on right side

**Test Criteria:**
- [ ] WebView window opens with correct size
- [ ] All controls visible and styled correctly (dropdown + 5 knobs)
- [ ] Layout matches Ouaricon Naturalist brand
- [ ] Background (aged paper) renders properly
- [ ] Botanical overlay (flora) positioned correctly
- [ ] Typography (Garamond) loads correctly
- [ ] Color palette (earth tones + green accents) matches brand

---

#### Phase 4.2: Parameter Binding and Interaction

**Goal:** Two-way parameter communication (UI ↔ DSP)

**Components:**
- WebSliderRelay for 5 float parameters (CHARACTER, WET, DRY, DECAY, SIZE)
- WebComboBoxRelay for TYPE parameter (choice)
- JavaScript → C++ relay calls (control changes)
- C++ → JavaScript parameter updates (host automation)
- Value formatting and display
- Real-time parameter updates during playback

**Test Criteria:**
- [ ] TYPE dropdown changes reverb type in real-time
- [ ] All 5 knobs change DSP parameters (verified via audio)
- [ ] Host automation updates UI controls (move knobs in DAW, UI follows)
- [ ] Preset changes update all UI elements (TYPE + 5 knobs)
- [ ] Parameter values display correctly (units: %, s, etc.)
- [ ] No lag or visual glitches during parameter changes
- [ ] Knobs use relative drag (not absolute positioning)

---

### Implementation Flow

1. **Stage 0: Research** ✓ Complete (architecture.md + plan.md)
2. **Stage 0: Planning** ✓ Current (this file)
3. **Stage 1: Foundation** - CMakeLists.txt, project structure
4. **Stage 2: Shell** - APVTS parameters (6 parameters)
5. **Stage 3: DSP** - 3 phases
   - Phase 3.1: Core Processing (Room reverb + dry/wet)
   - Phase 3.2: Type Switching (6 types)
   - Phase 3.3: Character Control (warm/bright filter)
6. **Stage 4: GUI** - 2 phases
   - Phase 4.1: Layout and Basic Controls
   - Phase 4.2: Parameter Binding
7. **Stage 5: Validation** - Presets, pluginval, changelog

---

## Implementation Notes

### Thread Safety
- All parameter reads use atomic `getRawParameterValue()->load()`
- Reverb parameter updates in audio thread (no allocations)
- Filter coefficient updates in audio thread (JUCE handles efficiently)
- No shared state between channels (reverb is stereo, filter duplicated)

### Performance
**Estimated CPU usage:**
- Reverb: ~15-25% CPU (single core, 48kHz, 512 samples)
  - JUCE dsp::Reverb is highly optimized
- Character filter: ~2-5% CPU (IIR biquad, very efficient)
- Total estimated: ~20-30% single core at 48kHz

**Optimization opportunities:**
- None needed initially (well within "lightweight" target)
- If CPU exceeds target: Reduce reverb quality (shorter delay lines)

### Latency
**Estimated latency:**
- Reverb: ~10-20ms internal latency
- Character filter: <1ms (IIR is non-latent)
- Total: ~10-20ms

**Host compensation:**
- Report via `getLatencySamples()` if measurable
- DryWetMixer handles latency compensation automatically

### Denormal Protection
- Use `juce::ScopedNoDenormals` in processBlock()
- All JUCE DSP components handle denormals internally
- Filter state managed by JUCE (denormal protection built-in)

### Known Challenges

**Type preset tuning:**
- Six types must sound distinct and musical
- May require iterative A/B testing and parameter tweaking
- Reference: Professional reverb plugins for character differences
- Solution: Start with research-based presets, adjust by ear

**Character filter mode transitions:**
- Switching warm ↔ bright can cause clicks if filter state not reset
- Pattern: Reset filter state on mode change (see FlutterVerb TONE control)
- Reference: troubleshooting/patterns/juce8-critical-patterns.md
- Solution: Check FlutterVerb PluginProcessor.cpp lines 86-130

**Independent DRY/WET control:**
- DryWetMixer provides wet mix proportion, not independent dry gain
- Need manual dry/wet blend: `output = (dry * dryGain) + (wet * wetGain)`
- Solution: Capture dry via pushDrySamples, apply manual gain scaling

**Spring type character:**
- May need subtle modulation for authentic spring reverb metallic sound
- Current approach: Narrower stereo width (0.8) + specific roomSize/damping
- Fallback: If not convincing, add LFO modulation in Phase 3.2 (optional)

---

## References

**Contract files:**
- Creative brief: `plugins/OuariconSimpleReverb/.ideas/creative-brief.md`
- DSP architecture: `plugins/OuariconSimpleReverb/.ideas/architecture.md`
- Implementation plan: `plugins/OuariconSimpleReverb/.ideas/plan.md` (this file)

**Reference plugins:**
- **FlutterVerb** - Reverb engine setup, TONE control (character filter), dry/wet mixing
- **DriveVerb** - Additional reverb reference
- **LushVerb** - Reverb parameter mapping

**Critical patterns:**
- `troubleshooting/patterns/juce8-critical-patterns.md`
  - Pattern #17: juce::dsp::Reverb API (prepare/process, not setSampleRate)
  - Filter state management on mode transitions

**JUCE documentation:**
- juce::dsp::Reverb API: https://docs.juce.com/master/classdsp_1_1Reverb.html
- juce::dsp::IIR::Filter: Biquad filter with coefficients
- juce::dsp::DryWetMixer: Latency-compensated mixing
