# AngelGrain - Implementation Plan

**Date:** 2025-11-18
**Complexity Score:** 5.0 (Complex)
**Strategy:** Phase-based implementation

---

## Complexity Factors

- **Parameters:** 7 parameters (7/5 = 1.4 points)
  - delayTime, tempoSync, grainSize, feedback, chaos, character, mix
- **Algorithms:** 4 DSP components = 4.0
  - Granular synthesis engine (voice management, scheduling)
  - Pitch quantization system (octaves/fifths)
  - Character morphing system (density + crossfade)
  - Chaos master randomization (multi-dimension scaling)
- **Features:** 1 point
  - Feedback loop (+1)
- **Total:** 1.4 + 4.0 + 1.0 = 6.4 (capped at 5.0)

---

## Stages

- Stage 0: Research & Planning (CURRENT)
- Stage 1: Foundation <- Next
- Stage 2: Shell
- Stage 3: DSP [3 phases]
- Stage 4: GUI [2 phases]
- Stage 5: Validation

---

## Complex Implementation (Score >= 3.0)

### Stage 3: DSP Phases

#### Phase 3.1: Core Granular Engine

**Goal:** Implement basic grain buffer, voice management, and scheduling

**Components:**
- Grain buffer using `juce::dsp::DelayLine<float, Lagrange3rd>`
- Grain voice structure (position, window position, playback rate, pan, active)
- Voice array pre-allocation (32 voices)
- Grain scheduler with sample counter
- Basic window envelope (Hann for initial implementation)
- Dry/wet mixer setup

**Test Criteria:**
- [ ] Plugin loads in DAW without crashes
- [ ] Audio passes through (wet and dry signals audible)
- [ ] Grains spawn at regular intervals based on delayTime
- [ ] Grains read from delay buffer correctly
- [ ] Window envelope prevents clicks (smooth grain start/end)
- [ ] Multiple grains overlap without artifacts
- [ ] Dry/wet mix parameter works correctly

---

#### Phase 3.2: Pitch Quantization and Pan

**Goal:** Implement pitch shifting and stereo panning per grain

**Components:**
- Pitch quantization to octaves/fifths: [-12, -7, 0, +7, +12]
- Playback rate calculation: `rate = pow(2.0f, semitones / 12.0f)`
- Random pan per grain with equal-power pan law
- Basic randomization (prepare for chaos integration)

**Test Criteria:**
- [ ] Grains pitch shift to correct intervals (octaves sound like octaves)
- [ ] Playback rate variation is audible and smooth
- [ ] Grains pan across stereo field
- [ ] Equal-power pan law maintains consistent volume
- [ ] No interpolation artifacts at extreme pitch shifts (±12 semitones)

---

#### Phase 3.3: Character, Chaos, and Feedback

**Goal:** Implement character morphing, chaos system, and feedback loop

**Components:**
- Character morphing: density multiplier + Tukey window alpha
- Chaos master randomization: position, pitch, pan, timing
- Feedback loop with soft saturation
- Tempo sync using AudioPlayHead
- Position randomization within delay buffer

**Test Criteria:**
- [ ] Character at 0% produces sparse, glitchy texture
- [ ] Character at 100% produces dense, smooth/angelic texture
- [ ] Chaos at 0% produces regular, centered output
- [ ] Chaos at 100% produces chaotic, spread output
- [ ] Feedback creates evolving textures without runaway
- [ ] Tempo sync quantizes delay time to note divisions
- [ ] High feedback + high chaos creates complex but stable textures

---

### Stage 4: GUI Phases

#### Phase 4.1: Layout and Basic Controls

**Goal:** Integrate UI mockup and bind basic parameters

**Components:**
- Copy UI mockup to Source/ui/public/index.html
- Update PluginEditor.h/cpp with WebView setup
- Configure CMakeLists.txt for WebView resources
- Bind primary parameters (delayTime, grainSize, feedback, mix)
- Central character knob as focal point

**Test Criteria:**
- [ ] WebView window opens with correct size
- [ ] All controls visible and styled correctly
- [ ] Layout matches mockup (character central, chaos surrounding)
- [ ] Ethereal/luminous visual style renders properly
- [ ] Background and styling match "angelic" aesthetic

---

#### Phase 4.2: Parameter Binding and Interaction

**Goal:** Complete two-way parameter communication (UI to DSP)

**Components:**
- JavaScript to C++ relay calls (all 7 parameters)
- C++ to JavaScript parameter updates (host automation)
- Value formatting and display (ms for delayTime, % for others)
- TempoSync toggle functionality
- Relative drag for rotary knobs

**Test Criteria:**
- [ ] All control movements change DSP parameters
- [ ] Host automation updates UI controls
- [ ] Preset changes update all UI elements
- [ ] Parameter values display correctly with units
- [ ] Tempo sync toggle enables/disables time quantization
- [ ] No lag or visual glitches during parameter changes

---

### Implementation Flow

- Stage 1: Foundation - project structure, CMakeLists.txt
- Stage 2: Shell - APVTS parameters (7 parameters)
- Stage 3: DSP - 3 phases
  - Phase 3.1: Core granular engine
  - Phase 3.2: Pitch quantization and pan
  - Phase 3.3: Character, chaos, feedback
- Stage 4: GUI - 2 phases
  - Phase 4.1: Layout and basic controls
  - Phase 4.2: Parameter binding
- Stage 5: Validation - presets, pluginval, changelog

---

## Implementation Notes

### Thread Safety
- All parameter reads use atomic `getRawParameterValue()->load()`
- Voice array pre-allocated in `prepareToPlay()` (no allocations in processBlock)
- Grain scheduler uses sample counter (no time-based operations in audio thread)
- Feedback gain smoothing for click-free changes

### Performance
- **Estimated CPU usage:**
  - Granular engine (32 voices): ~30-45% single core
  - Pitch shifting: ~5%
  - Window application: ~5%
  - Total: ~40-55% single core at 48kHz
- **Optimization opportunities:**
  - Pre-calculate window tables for common character values
  - Use SIMD for grain summing
  - Early-exit inactive voices

### Latency
- No latency compensation needed (granular delay is creative effect)
- DelayTime controls when grains are read, not processing latency

### Denormal Protection
- Use `juce::ScopedNoDenormals` in `processBlock()`
- Feedback loop may accumulate denormals at high feedback

### Known Challenges
- **Tukey window implementation:** Not built-in to JUCE
  - Solution: Custom implementation using formula (see architecture.md)
  - Alternative: Hann window with scaled attack/release portions
- **Character dual control:** Finding good mapping curves
  - Solution: Test extensively, reference Beads "texture" control
- **Tempo sync accuracy:** Must update when DAW tempo changes
  - Solution: Check playhead in processBlock, detect tempo changes
- **Buffer boundary handling:** Pitch-shifted grains may exceed buffer
  - Solution: Wrap read position modulo buffer size

### CPU Budget
- Target: < 60% single core at 48kHz with maximum density
- Test early with extreme parameters (100% character, 5ms grain size)
- Fallback: Reduce to 16 voices if CPU exceeds budget

---

## References

- Creative brief: `plugins/AngelGrain/.ideas/creative-brief.md`
- Parameter spec: `plugins/AngelGrain/.ideas/parameter-spec-draft.md`
- DSP architecture: `plugins/AngelGrain/.ideas/architecture.md`

### Reference Plugins
- **Scatter** - Granular delay with scale quantization (similar architecture)
  - Reference: Voice management, grain scheduling, feedback loop
  - Location: `plugins/Scatter/Source/PluginProcessor.cpp`
- **FlutterVerb** - Delay line usage with LFO modulation
  - Reference: DelayLine with Lagrange3rd, parameter smoothing
  - Location: `plugins/FlutterVerb/Source/PluginProcessor.h`
- **TapeAge** - Feedback and saturation
  - Reference: tanh saturation in feedback loop
  - Location: `plugins/TapeAge/Source/PluginProcessor.cpp`

---

## Duration Estimates

- **Stage 1: Foundation** - 15 min
- **Stage 2: Shell** - 30 min
- **Stage 3: DSP** - 4-6 hours
  - Phase 3.1: Core engine - 2-3 hours
  - Phase 3.2: Pitch/pan - 1-1.5 hours
  - Phase 3.3: Character/chaos/feedback - 1-1.5 hours
- **Stage 4: GUI** - 2-3 hours
  - Phase 4.1: Layout - 1-1.5 hours
  - Phase 4.2: Binding - 1-1.5 hours
- **Stage 5: Validation** - 1-2 hours

**Total estimated:** 8-12 hours
