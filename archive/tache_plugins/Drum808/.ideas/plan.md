# Drum808 - Implementation Plan

**Date:** 2025-11-13
**Complexity Score:** 5.0 (Complex - Maximum)
**Strategy:** Phase-based implementation

---

## Complexity Factors

- **Parameters:** 24 parameters (24/5 = 4.8, capped at 2.0) = **2.0**
- **Algorithms:** 8 DSP components = **8**
  - Kick Drum Voice (custom bridged-T with pitch envelope)
  - Low Tom Voice (bridged-T + bandpass filter)
  - Mid Tom Voice (same as low tom, separate instance)
  - Clap Voice (multi-trigger envelope + noise + bandpass)
  - Closed Hi-Hat Voice (6-oscillator synthesis + bandpass)
  - Open Hi-Hat Voice (same as closed hat, separate instance)
  - MIDI Note Router (custom routing logic)
  - Voice Choking System (state tracking)
- **Features:** 2 points
  - External MIDI control (+1): MIDI note-on triggers, velocity sensitivity
  - Multi-output routing (+1): 7 stereo buses (14 channels total)
- **Total:** 12.0 (capped at 5.0) = **5.0**

**Tier:** 5 (Synthesizers + Multi-output routing)
**Research Depth:** DEEP (30 minutes)

---

## Stages

- Stage 0: Research ✓
- Stage 1: Planning ✓
- Stage 2: Foundation ← Next
- Stage 3: Shell
- Stage 4: DSP (phased - 3 phases)
- Stage 5: GUI (phased - 3 phases)
- Stage 6: Validation

---

## Complex Implementation (Score = 5.0)

### Stage 4: DSP Phases

#### Phase 4.1: Foundation + Simple Voices (Low/Mid Tom)

**Goal:** Establish MIDI routing, multi-output architecture, and implement simple bridged-T voices without pitch envelope

**Components:**
- MIDI note routing (note number → voice index mapping)
- Multi-output bus configuration (7 stereo buses: main + 6 individual)
- Basic voice architecture (trigger, envelope state, output routing)
- Low Tom synthesis:
  - `juce::dsp::Oscillator<float>` (sine wave)
  - `juce::dsp::StateVariableTPTFilter<float>` (bandpass, Q controlled by tone)
  - Exponential amplitude envelope (no pitch envelope)
  - Base frequency ~150 Hz (tunable ±12 semitones)
- Mid Tom synthesis:
  - Same architecture as Low Tom
  - Base frequency ~220 Hz (default +5 semitones, tunable ±12 semitones)
- Output routing: Each voice writes to both main mix (bus 0) AND individual output (bus 1-3)

**Test Criteria:**
- [ ] Plugin loads in DAW without crashes
- [ ] MIDI note 41 (F1) triggers Low Tom
- [ ] MIDI note 45 (A1) triggers Mid Tom
- [ ] Low Tom individual output (bus 2) isolated correctly
- [ ] Mid Tom individual output (bus 3) isolated correctly
- [ ] Main output (bus 0) contains mixed low tom + mid tom
- [ ] Velocity sensitivity works (soft/hard hits produce different volumes)
- [ ] Tuning parameter shifts pitch (test ±12 semitones)
- [ ] Tone parameter changes filter resonance (audible brightness change)
- [ ] Decay parameter changes envelope length (20ms vs 1000ms audible)

**Duration:** 4-6 hours

---

#### Phase 4.2: Complex Voices (Kick, Hi-Hats, Choking)

**Goal:** Implement kick with pitch envelope, six-oscillator hi-hats, and voice choking logic

**Components:**
- Kick synthesis:
  - Body tone: `juce::dsp::Oscillator<float>` (sine wave)
  - Attack transient: Noise burst scaled by tone parameter
  - Exponential pitch envelope: freq(t) = baseFreq × (1.0 + exp(-t / 0.02))
  - Exponential amplitude envelope
  - Base frequency ~60 Hz (tunable ±12 semitones)
- Closed Hi-Hat synthesis:
  - 6× `juce::dsp::Oscillator<float>` (square wave)
  - Frequency ratios: {1.0, 1.4, 1.7, 2.1, 2.5, 3.0}
  - Base frequency ~3500 Hz (tunable ±12 semitones)
  - Bandpass filter (6-12 kHz, controlled by tone parameter)
  - Short exponential decay (20-200ms)
- Open Hi-Hat synthesis:
  - Same architecture as Closed Hi-Hat
  - Longer exponential decay (100-1000ms)
- Voice choking logic:
  - Track open hat voice state (playing/stopped flag)
  - When closed hat triggers (note 42), immediately stop open hat
  - Reset open hat envelope to zero (prevent clicks)
  - Sample-accurate choking (handle both notes in same buffer)

**Test Criteria:**
- [ ] MIDI note 36 (C1) triggers Kick
- [ ] MIDI note 42 (F#1) triggers Closed Hi-Hat
- [ ] MIDI note 46 (A#1) triggers Open Hi-Hat
- [ ] Kick pitch envelope audible (frequency sweeps down)
- [ ] Kick attack transient controlled by tone parameter (click amount)
- [ ] Hi-hats have metallic character (6 oscillators create inharmonic spectrum)
- [ ] Hi-hat tone parameter shifts brightness (6-12 kHz bandpass center)
- [ ] Voice choking works: Closed hat stops open hat immediately
- [ ] Edge case: Simultaneous closed + open hat → closed hat wins
- [ ] Kick individual output (bus 1) isolated correctly
- [ ] Closed Hat individual output (bus 5) isolated correctly
- [ ] Open Hat individual output (bus 6) isolated correctly

**Duration:** 6-8 hours

---

#### Phase 4.3: Clap Voice (Multi-Trigger Envelope)

**Goal:** Implement clap synthesis with 4-state multi-trigger envelope

**Components:**
- Noise generator: `juce::Random::getSystemRandom().nextFloat() * 2.0f - 1.0f`
- Bandpass filter:
  - Center frequency ~1000 Hz (tunable ±12 semitones via tuning parameter)
  - Q factor 2.0-5.0 (controlled by tone parameter)
- Multi-trigger envelope state machine:
  - State 1 (Spike 1): t=0-10ms, amplitude = snap × 1.0, exp(-t / 0.003)
  - State 2 (Spike 2): t=10-20ms, amplitude = snap × 0.6, exp(-(t-0.01) / 0.003)
  - State 3 (Spike 3): t=20-30ms, amplitude = snap × 0.3, exp(-(t-0.02) / 0.003)
  - State 4 (Decay): t=30-1964ms, amplitude = exp(-(t-0.03) / 1.934)
- Spike timing: Sample-rate independent (calculate transition samples in prepareToPlay)
- Snap parameter controls initial spike amplitude (attack sharpness)

**Test Criteria:**
- [ ] MIDI note 38 (D1) triggers Clap
- [ ] Multi-spike character audible (3 distinct transients)
- [ ] Spike timing correct at 44.1kHz, 48kHz, 96kHz (10ms spacing verified)
- [ ] Snap parameter controls attack sharpness (0% = soft, 100% = sharp)
- [ ] Tone parameter controls brightness (bandpass Q factor)
- [ ] Tuning parameter shifts resonance frequency (audible pitch change)
- [ ] Decay tail follows spike sequence (long 1964ms decay)
- [ ] Clap individual output (bus 4) isolated correctly
- [ ] State machine handles retriggering correctly (reset to spike 1)

**Duration:** 4-6 hours

---

### Stage 5: GUI Phases

#### Phase 5.1: Layout and Basic Controls

**Goal:** Integrate mockup HTML and establish 6-channel strip layout with 24 parameter bindings

**Components:**
- Copy mockup HTML to `Source/ui/public/index.html`
- WebView setup in PluginEditor.h/cpp:
  - Resource provider for HTML/CSS/JS files
  - Explicit URL mapping (per juce8-critical-patterns.md Pattern 8)
  - NEEDS_WEB_BROWSER TRUE in CMakeLists.txt (per Pattern 9)
- CMakeLists.txt configuration:
  - `juce_add_binary_data()` for UI resources
  - `juce_gui_extra` module linked
  - `JUCE_WEB_BROWSER=1` compile definition
- Layout: 6 vertical channel strips (Kick, Low Tom, Mid Tom, Clap, Closed Hat, Open Hat)
- Each strip: 4 vertical sliders (Level, Tone, Decay/Snap, Tuning)
- Total: 24 parameter controls

**Test Criteria:**
- [ ] WebView window opens with correct size (~1000×500px)
- [ ] All 6 channel strips visible and labeled correctly
- [ ] 24 sliders visible (6 voices × 4 parameters each)
- [ ] Layout matches mockup design (vertical strips, horizontal arrangement)
- [ ] Background and styling render properly (808 aesthetic)
- [ ] No console errors in WebView (check resource loading)

**Duration:** 3-4 hours

---

#### Phase 5.2: Parameter Binding and Interaction

**Goal:** Establish two-way parameter communication (UI ↔ DSP) for all 24 parameters

**Components:**
- 24× `WebSliderRelay` instances (one per parameter)
  - kick_level, kick_tone, kick_decay, kick_tuning
  - lowtom_level, lowtom_tone, lowtom_decay, lowtom_tuning
  - midtom_level, midtom_tone, midtom_decay, midtom_tuning
  - clap_level, clap_tone, clap_snap, clap_tuning
  - closedhat_level, closedhat_tone, closedhat_decay, closedhat_tuning
  - openhat_level, openhat_tone, openhat_decay, openhat_tuning
- 24× `WebSliderParameterAttachment` instances (per Pattern 12: 3 parameters including nullptr)
- JavaScript parameter binding:
  - ES6 module loading with `type="module"` (per Pattern 21)
  - `import { getSliderState }` from JUCE index.js
  - Relative drag interaction (per Pattern 16)
  - `valueChangedEvent` callback with no parameters (per Pattern 15)
- Value formatting: Display ranges (0-100%, 50-1000ms, -12 to +12 st)

**Test Criteria:**
- [ ] All 24 sliders respond to mouse drag (relative drag, not absolute)
- [ ] Dragging slider updates DSP parameter (audio changes)
- [ ] Host automation updates UI sliders (preset changes reflect correctly)
- [ ] Parameter values display correctly (formatted ranges)
- [ ] No frozen knobs (verify 3-parameter attachment signature)
- [ ] Smooth interaction (no lag or visual glitches)
- [ ] Knobs initialize to correct positions on load (sendInitialUpdate works)

**Duration:** 3-4 hours

---

#### Phase 5.3: Advanced UI Elements (Trigger LEDs)

**Goal:** Implement per-voice trigger LEDs with real-time MIDI note-on visualization

**Components:**
- 6× atomic bool flags (one per voice) for audio → UI communication
  - `std::atomic<bool> kickTriggered`, `lowTomTriggered`, etc.
- Audio thread sets flags on MIDI note-on (sample-accurate)
- Message thread timer callback (60fps) reads flags and updates LED visuals
- LED animation:
  - Light up on trigger (full brightness)
  - Exponential decay over 200-500ms (visual sustain)
  - CSS transitions or JavaScript animation
- Visual style: Red/orange LED indicators (authentic 808 aesthetic)

**Test Criteria:**
- [ ] All 6 trigger LEDs visible in UI (one per voice)
- [ ] MIDI note-on causes corresponding LED to light up
- [ ] LEDs respond in real-time (no visible lag)
- [ ] LED decay animation smooth (exponential falloff)
- [ ] Multiple simultaneous triggers work correctly (independent LEDs)
- [ ] LEDs work at high trigger rates (rapid MIDI notes don't cause flicker)
- [ ] Visual polish: Colors, brightness, animations match 808 aesthetic

**Duration:** 2-3 hours

---

### Estimated Duration

Total: ~22-31 hours

- Stage 2: 5 min (Foundation - project structure)
- Stage 3: 5 min (Shell - APVTS parameters)
- Stage 4: 14-20 hours (DSP - 3 phases)
  - Phase 4.1: 4-6 hours (Foundation + Simple Voices)
  - Phase 4.2: 6-8 hours (Complex Voices + Choking)
  - Phase 4.3: 4-6 hours (Clap Multi-Trigger)
- Stage 5: 8-11 hours (GUI - 3 phases)
  - Phase 5.1: 3-4 hours (Layout + Basic Controls)
  - Phase 5.2: 3-4 hours (Parameter Binding)
  - Phase 5.3: 2-3 hours (Trigger LEDs)
- Stage 6: 20 min (Validation - presets, pluginval, changelog)

---

## Implementation Notes

### Thread Safety

- All parameter reads use `APVTS::getRawParameterValue()->load()` (atomic)
- Voice trigger flags: 6× `std::atomic<bool>` for LED communication
- No shared state between voices (except open hat choking flag - bool, audio thread only)
- MIDI processing happens entirely on audio thread (no cross-thread communication)
- UI trigger LEDs read atomic flags on timer callback (message thread)

### Performance

Estimated CPU usage per component (at 48kHz):
- Kick voice: ~8% (2 oscillators + 2 exponential envelopes + pitch envelope)
- Low Tom voice: ~6% (1 oscillator + 1 bandpass filter + 1 envelope)
- Mid Tom voice: ~6% (same as Low Tom)
- Clap voice: ~10% (noise generator + bandpass filter + 4-state envelope)
- Closed Hat voice: ~15% (6 square oscillators + bandpass filter + envelope)
- Open Hat voice: ~15% (same as Closed Hat)
- MIDI routing: <1%
- Multi-output routing: <1%
- **Total estimated: ~60% single core** (high but acceptable for drum synth)

Optimization opportunities:
- SIMD for hi-hat oscillator mixing (6 oscillators → vectorized)
- Lookup tables for exponential envelopes (trade memory for CPU)
- Voice allocation: Only process active voices (skip silent voices)

### Latency

- Zero latency (no lookahead, no FFT, no algorithmic delay)
- Sample-accurate MIDI triggering
- Report via `getLatencySamples()`: return 0

### Denormal Protection

- Use `juce::ScopedNoDenormals` in processBlock()
- All JUCE DSP components handle denormals internally
- Custom exponential envelopes: Add threshold (output = 0 if envelope < 1e-8)
- Oscillator phase wrapping prevents denormal accumulation

### Known Challenges

**Bridged-T Tuning:**
- Pitch envelope decay constant (20ms) may need empirical tuning
- Reference: Compare against Roland TR-808 samples for authenticity
- Fallback: If pitch envelope doesn't sound right, use fixed-frequency sine

**Clap Multi-Trigger Timing:**
- State machine must be sample-rate independent
- Calculate transition samples in prepareToPlay(): `spike2Samples = sampleRate × 0.01`
- Test at 44.1kHz, 48kHz, 96kHz, 192kHz to verify 10ms spacing

**Hi-Hat CPU Cost:**
- 12 oscillators total (6 closed + 6 open) may exceed 60% CPU estimate
- Profile early in Phase 4.2
- If CPU >70%, consider ring modulation fallback (3 oscillators × ring mod)

**Multi-Output DAW Compatibility:**
- Not all DAWs support 14 outputs
- Test with Logic Pro (full support), Ableton (requires "Configure"), FL Studio ("Split mixer tracks")
- Graceful fallback: If DAW doesn't enable individual outputs, main mix still works

**Voice Choking Edge Cases:**
- Simultaneous closed + open hat in same buffer (sample position collision)
- Solution: Process closed hat BEFORE open hat in MIDI routing (deterministic order)
- Test: Send both MIDI notes at exact same sample position, verify closed hat wins

### Module Dependencies

- `juce_dsp`: Oscillator, StateVariableTPTFilter (for DSP synthesis)
- `juce_audio_processors`: AudioProcessor, APVTS (for parameters and bus config)
- `juce_gui_extra`: WebBrowserComponent (for WebView UI)
- `juce_audio_basics`: MidiBuffer, MidiMessage (for MIDI routing)

### Critical Patterns to Follow

- **Pattern 4:** BusesProperties MUST be in constructor initializer list (NOT prepareToPlay)
- **Pattern 8:** WebView resource provider uses explicit URL mapping (NOT generic loop)
- **Pattern 9:** CMakeLists.txt requires `NEEDS_WEB_BROWSER TRUE` for VST3
- **Pattern 10:** Always install to system after build (DAWs load from system folders)
- **Pattern 11:** Use `std::unique_ptr` for WebView members (proper initialization order)
- **Pattern 12:** WebSliderParameterAttachment requires 3 parameters (parameter, relay, nullptr)
- **Pattern 16:** Knob interaction uses relative drag (frame-delta, not absolute positioning)
- **Pattern 21:** WebView requires `type="module"` for ES6 imports

---

## References

- Creative brief: `plugins/Drum808/.ideas/creative-brief.md`
- Parameter spec: `plugins/Drum808/.ideas/parameter-spec.md`
- DSP architecture: `plugins/Drum808/.ideas/architecture.md`
- UI mockup: (to be created in ideation)

Similar plugins for reference:
- **MinimalKick:** Simple drum synthesis (reference for voice architecture)
- **GainKnob:** WebView parameter binding (reference for relay setup)
- **TapeAge:** WebView knob interaction (reference for relative drag pattern)
- **AutoClip:** Multi-parameter UI (reference for managing 24 parameter bindings)
