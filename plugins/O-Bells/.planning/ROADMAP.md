# O-Bells - Implementation Plan

**Date:** 2026-02-01
**Complexity Score:** 5.0 (Complex - Maximum complexity)
**Strategy:** Phase-based implementation (staged DSP and GUI)

---

## Complexity Factors

- **Parameters:** 21 parameters (21/5 = 4.2, capped at 2.0) = **2.0**
- **Algorithms:** 8 DSP components = **8**
  - Modal Synthesis Engine
  - Voice Manager (8-voice polyphony)
  - Ensemble Voicing System
  - Strike Dynamics Processor
  - Material Morphing System
  - Partial Envelope Generator
  - Pitch Envelope
  - Sympathetic Resonance
- **Features:** 2 points
  - External MIDI control (+1) - Synthesizer with MIDI input
  - Modulation systems (+1) - Partial envelopes, strike dynamics, material morphing
- **Total:** 2.0 + 8 + 2 = **12.0** (capped at 5.0) = **5.0**

**Complexity Tier:** 6 (Synthesizer with extensive modal synthesis, ensemble voicing, 8-voice polyphony)

---

## Stages

- Stage 0: Research ✓
- Stage 1: Planning ← Next
- Stage 1: Foundation (after planning approval)
- Stage 2: Shell
- Stage 3: DSP [PHASED - 3 phases]
- Stage 4: GUI [PHASED - 3 phases]
- Stage 4: Validation

---

## Complex Implementation (Score = 5.0)

### Stage 3: DSP Phases

#### Phase 3.1: Core Modal Synthesis Engine

**Goal:** Implement single-voice modal synthesis with basic bell partials

**Components:**
- Custom `BellVoice` class (inherits from `juce::SynthesiserVoice`)
- Modal partial generator using `juce::dsp::Oscillator` (8 sine oscillators)
- Partial frequency ratio calculation (church bell ratios)
- Basic ADSR envelope per partial (decay-focused)
- MIDI note-on/note-off handling
- Fundamental frequency calculation from MIDI note

**Parameters Active in This Phase:**
- BELL_SIZE (scales fundamental frequency)
- DAMPING (controls decay time)
- INHARMONICITY (0-100%, church bell ratios at 50%)

**Test Criteria:**
- [ ] Plugin loads in DAW without crashes
- [ ] MIDI note-on triggers bell sound (single voice)
- [ ] Bell has characteristic inharmonic timbre (minor third audible)
- [ ] DAMPING parameter controls decay length (short to long)
- [ ] INHARMONICITY morphs from harmonic (0%) to gamelan (100%)
- [ ] No clicks or pops on note-on/note-off
- [ ] Frequency follows MIDI note correctly across full range (A0-C8)

**Git Commit:** "feat(O-Bells): Phase 3.1 - core modal synthesis engine"

---

#### Phase 3.2: Polyphony and Strike Dynamics

**Goal:** Add 8-voice polyphony, voice stealing, and strike parameter control

**Components:**
- `juce::Synthesiser` setup with 8 `BellVoice` instances
- Voice stealing (oldest-first with 5ms fade-out)
- Strike Dynamics Processor (MALLET_HARDNESS, STRIKE_POSITION, velocity response)
- Strike transient generation (noise burst + filter)
- Partial amplitude scaling based on strike position (comb filter effect)
- Spectral tilt based on mallet hardness (high-shelf filter)

**Parameters Active in This Phase:**
- STRIKE_POSITION (0-100%, center to edge)
- MALLET_HARDNESS (0-100%, soft to hard)
- BRIGHTNESS (global high-frequency emphasis)
- VELOCITY_CURVE (linear/exp/log)
- STRIKE_NOISE_CHARACTER (click/thud/ping)

**Test Criteria:**
- [ ] 8-note polyphonic chords play without dropouts
- [ ] Voice stealing occurs smoothly when >8 notes played (no clicks)
- [ ] STRIKE_POSITION affects timbre (center = warm, edge = bright)
- [ ] MALLET_HARDNESS affects attack sharpness and brightness
- [ ] MIDI velocity scales amplitude and brightness appropriately
- [ ] Strike transient audible on note attack (character selectable)
- [ ] Rapid arpeggios don't cause artifacts (stress test voice allocation)

**Git Commit:** "feat(O-Bells): Phase 3.2 - polyphony and strike dynamics"

---

#### Phase 3.3: Ensemble Voicing and Advanced Features

**Goal:** Implement unison layering, octave blending, material morphing, and optional features

**Components:**
- Ensemble Voicing System (unison detune, octave layering, stereo spread)
- Material Morphing System (Bronze → Steel → Glass → Crystal interpolation)
- Pitch Envelope (initial pitch drop for large bells)
- Sympathetic Resonance (cross-voice coupling, optional)
- Partial Tuning (adjust minor-third partial)
- Nonlinear Effects (waveshaping at high velocity)
- Decay Shape selection (linear/exponential/multi-stage)

**Parameters Active in This Phase:**
- MATERIAL (0-100%, continuous morph)
- UNISON_COUNT (1-4 voices)
- UNISON_DETUNE (0-50 cents)
- OCTAVE_BLEND_SUB (0-100%)
- OCTAVE_BLEND_OCT (0-100%)
- STEREO_SPREAD (0-100%)
- PARTIAL_TUNING (±100 cents on minor-third partial)
- PITCH_ENVELOPE (0-100% pitch drop amount)
- SYMPATHETIC_RESONANCE (0-100% coupling strength)
- NONLINEAR_EFFECTS (0-100% waveshaping)
- DECAY_SHAPE (linear/exp/multi choice)

**Test Criteria:**
- [ ] Unison layering creates rich, chorused bell ensemble
- [ ] Detune spreads unison voices smoothly (0 = tight, 50 cents = wide)
- [ ] Octave blend adds sub/octave layers (huge, layered sound)
- [ ] Stereo spread pans ensemble across stereo field
- [ ] Material morphing smoothly transitions between Bronze/Steel/Glass/Crystal
- [ ] Pitch envelope adds initial pitch drop on large bells
- [ ] Sympathetic resonance couples harmonically related voices (optional)
- [ ] CPU usage remains <70% on single core (worst case: 8 voices, 4 unison, 100% octave blend)
- [ ] No phase cancellation or thinning when unison voices sum

**Git Commit:** "feat(O-Bells): Phase 3.3 - ensemble voicing and advanced features"

---

### Stage 4: GUI Phases

#### Phase 4.1: Layout and Basic Controls (Main Panel)

**Goal:** Integrate WebView UI with main panel parameters

**Components:**
- Create HTML mockup for main panel (will be done during mockup creation stage)
- Copy v1-ui.html to Source/ui/public/index.html
- Update PluginEditor.h/cpp with WebView setup
- Configure CMakeLists.txt for WebView resources (NEEDS_WEB_BROWSER TRUE)
- Implement resource provider (index.html, juce/index.js, check_native_interop.js)
- Bind 7 main panel parameters via WebSliderRelay/WebSliderParameterAttachment
- Apply Ouaricon Botanical theme (snail motif, warm amber/bronze colors)

**Parameters Bound in This Phase:**
- STRIKE_POSITION (rotary knob)
- MALLET_HARDNESS (rotary knob)
- BELL_SIZE (rotary knob)
- DAMPING (rotary knob)
- BRIGHTNESS (rotary knob)
- MATERIAL (rotary knob or slider)
- INHARMONICITY (rotary knob)

**Test Criteria:**
- [ ] WebView window opens with correct size (800×600 typical)
- [ ] Main panel layout matches botanical aesthetic (snail hero image, warm colors)
- [ ] All 7 main knobs visible and styled correctly
- [ ] Background renders properly (cream/amber tones)
- [ ] Knobs use relative drag (not absolute positioning - see juce8-critical-patterns.md)
- [ ] Plugin appears in both VST3 and AU formats (NEEDS_WEB_BROWSER flag set)

**Git Commit:** "feat(O-Bells): Phase 4.1 - main panel layout and basic controls"

---

#### Phase 4.2: Parameter Binding and Interaction (Main + Ensemble)

**Goal:** Two-way parameter communication and ensemble section UI

**Components:**
- JavaScript → C++ parameter relay (knob drag updates APVTS)
- C++ → JavaScript parameter updates (host automation, preset changes)
- Ensemble section UI (Unison Count, Detune, Octave Blend sliders, Stereo Spread)
- Value formatting and display (show parameter values as text)
- Real-time updates during playback (host automation visible in UI)
- Main/Advanced panel tabs (toggle between views)

**Parameters Bound in This Phase:**
- UNISON_COUNT (integer selector or knob 1-4)
- UNISON_DETUNE (rotary knob 0-50 cents)
- OCTAVE_BLEND_SUB (slider 0-100%)
- OCTAVE_BLEND_OCT (slider 0-100%)
- STEREO_SPREAD (rotary knob 0-100%)

**Test Criteria:**
- [ ] Knob movements change DSP parameters (audible effect)
- [ ] Host automation updates UI knobs in real-time (no lag)
- [ ] Preset changes update all UI elements correctly
- [ ] Parameter values display as text (e.g., "2.4×", "50 cents", "100%")
- [ ] Ensemble section controls work (unison count, detune, octave blend)
- [ ] Main/Advanced tab switching works (toggle between panels)
- [ ] No visual glitches or frozen knobs (see juce8-critical-patterns: ES6 module loading, valueChangedEvent)

**Git Commit:** "feat(O-Bells): Phase 4.2 - parameter binding and ensemble section"

---

#### Phase 4.3: Advanced Panel and Polish

**Goal:** Advanced parameter controls and visual polish

**Components:**
- Advanced panel layout (Partial Tuning, Pitch Envelope, Sympathetic Resonance, etc.)
- Choice parameters (STRIKE_NOISE_CHARACTER, DECAY_SHAPE, VELOCITY_CURVE) as dropdowns
- Visual feedback for material morphing (color shift or icon change)
- Optional: VU meter or waveform display (if time permits)
- Tooltips for advanced parameters (explain technical terms)
- Final aesthetic polish (animations, hover states, botanical motifs)

**Parameters Bound in This Phase:**
- PARTIAL_TUNING (±100 cents, rotary knob)
- PITCH_ENVELOPE (0-100%, rotary knob)
- SYMPATHETIC_RESONANCE (0-100%, rotary knob)
- NONLINEAR_EFFECTS (0-100%, rotary knob)
- STRIKE_NOISE_CHARACTER (dropdown: Click/Thud/Ping)
- DECAY_SHAPE (dropdown: Linear/Exp/Multi)
- VELOCITY_CURVE (dropdown: Linear/Exp/Log)

**Test Criteria:**
- [ ] Advanced panel accessible via tab/button
- [ ] All advanced parameters work (audible DSP effect)
- [ ] Choice parameters show correct options in dropdown
- [ ] Material morphing shows visual feedback (color/icon change)
- [ ] Tooltips explain technical parameters (e.g., "Partial Tuning: Adjust minor-third overtone")
- [ ] Animations are smooth (no jank, 60fps)
- [ ] Botanical aesthetic is cohesive (snail motif, warm colors, golden ratio)
- [ ] No CPU spikes from UI rendering (profile with Activity Monitor)

**Git Commit:** "feat(O-Bells): Phase 4.3 - advanced panel and visual polish"

---

### Implementation Flow

- Stage 0: Research & Planning ✓
- Stage 1: Planning (create parameter spec from BRIEF) ← Next
- Stage 1: Foundation (CMakeLists.txt, project structure, APVTS stub)
- Stage 2: Shell (APVTS parameters defined)
- Stage 3: DSP
  - Phase 3.1: Core Modal Synthesis
  - Phase 3.2: Polyphony and Strike Dynamics
  - Phase 3.3: Ensemble Voicing and Advanced Features
- Stage 4: GUI
  - Phase 4.1: Layout and Basic Controls
  - Phase 4.2: Parameter Binding and Interaction
  - Phase 4.3: Advanced Panel and Polish
- Stage 4: Validation (presets, pluginval, changelog)

---

## Implementation Notes

### Thread Safety

- All parameter reads use atomic `APVTS::getRawParameterValue()->load()` (JUCE handles this)
- No shared mutable state between voices (each voice has independent oscillators, envelopes)
- Voice allocation uses `juce::Synthesiser` built-in thread safety (no custom locks needed)
- Sympathetic resonance reads other voice states (read-only) → safe without locks

**No custom thread safety code needed** - JUCE Synthesiser and APVTS handle all synchronization.

---

### Performance

**Estimated CPU usage per component (single voice, 8 partials):**
- Modal synthesis (8 sine oscillators): ~0.5% CPU
- Envelope generation (8 partials): ~0.05% CPU
- Strike dynamics (noise + filter): ~0.02% CPU
- **Total per voice:** ~0.6% CPU

**Worst-case scenario (8 voices, 4 unison, 3 octave layers):**
- Base voices: 8 × 0.6% = 4.8%
- Unison multiplication: 4.8% × 4 = 19.2%
- Octave layers: 19.2% × 3 = **57.6% CPU**

**Target:** <60% CPU on single core at 48kHz, 256 sample buffer

**Optimization strategies:**
- Use `juce::dsp::Oscillator` with SIMD optimization (JUCE provides this)
- Disable inactive octave layers when OCTAVE_BLEND = 0% (skip processing)
- Early-exit for silent voices (amplitude < -60dB)
- Consider quality settings if CPU exceeds 70%: Low (4 partials), Med (6 partials), High (8 partials)

---

### Latency

- **Processing latency:** Zero (no lookahead, no FFT, no delay compensation)
- **No need to report latency:** Real-time synthesis with no buffering
- If global reverb added later: Report via `getLatencySamples()`

---

### Denormal Protection

- Use `juce::ScopedNoDenormals` in processBlock() (prevents CPU spikes)
- All JUCE dsp components (Oscillator, IIR filters) handle denormals internally
- Envelope decay gates at -60dB (prevents infinite tail)

---

### Known Challenges

**1. CPU Management (Ensemble Multiplication)**
- Challenge: 4 unison × 3 octave layers × 8 voices = 96 effective voices (768 oscillators)
- Solution: Implement quality settings (partial count), disable unused octave layers
- Reference: Pianoteq uses <5% CPU for tubular bells (target for optimization)

**2. Voice Stealing Artifacts**
- Challenge: Long bell decay times make voice stealing audible
- Solution: 5ms linear fade-out on stolen voice (industry standard)
- Alternative: Prioritize release-phase voices for stealing (see architecture.md)

**3. Inharmonic Partial Tuning Stability**
- Challenge: Extreme inharmonicity (100%) may cause beating/dissonance
- Solution: Limit gamelan ratios to musically usable range, document parameter behavior
- Reference: Research papers validate ratios (see architecture.md sources)

**4. WebView Parameter Binding (JUCE 8)**
- Challenge: JUCE 8 requires ES6 modules, 3-parameter WebSliderParameterAttachment
- Solution: Follow juce8-critical-patterns.md (type="module", nullptr as 3rd parameter)
- Reference: Pattern #21 (ES6 module loading) and #12 (3-parameter attachment)

**5. Sympathetic Resonance Feedback Loops**
- Challenge: Cross-voice coupling can cause runaway resonance if not limited
- Solution: Limit coupling strength (max 15% of strike energy), tolerance threshold (±10 cents)
- Implementation: Make optional (disabled by default), test with dense harmonic chords

---

## References

**Contract Files:**
- Creative brief: `plugins/O-Bells/.planning/BRIEF.md`
- Parameter spec: To be created in Stage 1 Planning (extract from BRIEF)
- DSP architecture: `plugins/O-Bells/.planning/research/ARCHITECTURE.md`
- UI mockup: To be created during mockup stage (v1-ui.yaml)

**Similar Plugins for Reference:**
- **O-Lyrica** - Physical modeling synth (waveguide synthesis, polyphony, MIDI handling)
  - Reference for: Synthesiser setup, MIDI processing, voice management
  - Difference: Waveguide (strings) vs. modal (bells)

- **Pianoteq Tubular Bells** - Industry reference for bell physical modeling
  - Reference for: Parameter ranges, CPU efficiency, bell timbre
  - Difference: Commercial closed-source vs. our open approach

- **Ableton Collision** - Modal synthesis mallet instrument
  - Reference for: Modal synthesis architecture, partial count, performance
  - Difference: Built-in DAW instrument vs. standalone VST/AU plugin

**Critical Patterns:**
- JUCE 8 patterns: `/Users/taylorbrook/Dev/VST-development/troubleshooting/patterns/juce8-critical-patterns.md`
  - Pattern #22: IS_SYNTH flag (REQUIRED for instruments)
  - Pattern #21: ES6 module loading for WebView
  - Pattern #12: WebSliderParameterAttachment 3-parameter constructor
  - Pattern #11: std::unique_ptr for WebView members

---

## Stage 1 Planning Requirements

Before proceeding to Foundation stage, Stage 1 Planning must create:

1. **parameter-spec.md** - Extract all 21 parameters from BRIEF.md
   - 7 main panel parameters
   - 10+ advanced parameters
   - 4 ensemble parameters
   - Define: ID, type, range, default, units, description

2. **Mockup creation** (optional, can be deferred to GUI stage)
   - Main panel layout (7 knobs + ensemble section)
   - Advanced panel layout (10 parameters)
   - Ouaricon Botanical aesthetic (snail motif, amber/bronze colors)

3. **Review ARCHITECTURE.md** - Verify DSP approach aligns with BRIEF vision
   - Confirm modal synthesis is acceptable (vs. sampling or other approaches)
   - Validate complexity score and phase breakdown
   - Approve risk mitigations (CPU management, voice stealing, etc.)

**After approval:** Proceed to Stage 1 Foundation (create CMakeLists.txt, PluginProcessor stub, APVTS setup)

---

**Next Action:** Run `/implement O-Bells` to begin Stage 1 Foundation (or create parameter-spec.md manually if needed first)
