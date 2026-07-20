# LushPad - Implementation Plan

**Date:** 2025-11-13
**Complexity Score:** 5.0 (Complex)
**Strategy:** Phase-based implementation

---

## Complexity Factors

- **Parameters:** 3 parameters (3/5 = 0.6 points, capped at 2.0) = 0.6
- **Algorithms:** 8 DSP components = 8
  - Voice Management System (custom)
  - Oscillator Bank (3 sine waves per voice)
  - FM Feedback Processor (custom phase modulation)
  - Harmonic Saturation (tanh waveshaping)
  - Low-Pass Filter (juce::dsp::IIR::Filter)
  - ADSR Envelope (juce::ADSR)
  - Random LFO Modulation System (9 LFOs per voice, nested)
  - Reverb Engine (juce::dsp::Reverb)
- **Features:** 1 point
  - Nested modulation system (+1, highly complex routing)
- **Total:** 0.6 + 8 + 1 = 9.6 → capped at 5.0

---

## Stages

- Stage 0: Research ✓
- Stage 1: Planning ✓
- Stage 2: Foundation ← Next
- Stage 3: Shell
- Stage 4: DSP [phased - 3 phases]
- Stage 5: GUI [phased - 3 phases]
- Stage 6: Validation

---

## Complex Implementation (Score ≥ 3.0)

### Stage 3: DSP Phases

#### Phase 3.1: Core Polyphonic Synthesis

**Goal:** Implement basic polyphonic synth with MIDI, voice allocation, oscillators, and envelope

**Components:**
- MIDI input handling (note-on, note-off, velocity)
- Voice allocation system (8 voices, oldest-note-stealing)
- Sine wave oscillators (3 per voice with ±7 cent detuning)
- ADSR envelope per voice (fixed: 300ms attack, 2s release)
- Voice mixing (sum 8 voices to stereo output)
- Basic parameter connections (none yet, using fixed values)

**Signal flow:**
```
MIDI → Voice Allocation → 3 Sine Oscillators (detuned) → Sum → ADSR → Voice Mix → Output
```

**Test Criteria:**
- [ ] Plugin loads in DAW without crashes
- [ ] MIDI note-on triggers sound
- [ ] MIDI note-off releases envelope (2s fade-out)
- [ ] 8-note polyphony works (can hold 8 notes simultaneously)
- [ ] Voice stealing works (9th note steals oldest voice)
- [ ] Detuning creates chorused thickness (3 oscillators audible)
- [ ] ADSR envelope shapes amplitude correctly (slow attack, long release)
- [ ] No clicks or pops on note-on/note-off

**Duration:** 40 min

---

#### Phase 3.2: Timbre Processing (FM + Saturation + Filter)

**Goal:** Add FM feedback, harmonic saturation, and filtering with parameter control

**Components:**
- FM feedback per oscillator (phase modulation by delayed output)
- Harmonic saturation (tanh waveshaping)
- Low-pass filter per voice (12dB/octave, fixed resonance 0.35)
- timbre parameter connection (controls FM depth 0.0-0.4 AND saturation gain 1.0-3.0)
- filter_cutoff parameter connection (20-20000 Hz, logarithmic)
- Velocity → filter cutoff scaling (soft notes darker, hard notes brighter)

**Signal flow:**
```
MIDI → Voice Allocation
  → 3 Sine Oscillators with FM Feedback (timbre)
  → Sum → Saturation (timbre) → Filter (filter_cutoff, velocity)
  → ADSR → Voice Mix → Output
```

**Test Criteria:**
- [ ] timbre parameter affects FM feedback (0.0 = pure sine, 1.0 = rich harmonics)
- [ ] timbre parameter affects saturation (0.0 = clean, 1.0 = warm/saturated)
- [ ] FM feedback stays musical at all timbre settings (no harshness at max)
- [ ] Saturation adds warmth without distortion
- [ ] filter_cutoff parameter sweeps smoothly (20 Hz - 20 kHz)
- [ ] Velocity affects filter brightness (soft = dark, hard = bright)
- [ ] No zipper noise when changing parameters
- [ ] No instability or runaway feedback

**Duration:** 35 min

---

#### Phase 3.3: Generative Modulation + Reverb

**Goal:** Add nested random LFO system and global reverb

**Components:**
- Random LFO system (9 LFOs per voice):
  - Primary LFOs (3): panning, FM depth, saturation amount
  - Secondary LFOs (3): modulate primary LFO speed
  - Tertiary LFOs (3): modulate primary LFO depth
- Smooth interpolation (low-pass filtered random values)
- Global reverb (juce::dsp::Reverb, large hall configuration)
- reverb_amount parameter connection (dry/wet mix 0.0-1.0)
- Pre-delay (30ms), damping (0.4), roomSize (0.9)

**Signal flow:**
```
MIDI → Voice Allocation
  ↓
Update Random LFOs (nested: tertiary → secondary → primary)
  ↓
Calculate modulated parameters:
  - FM depth = timbre + LFO2_modulation
  - Saturation = timbre + LFO3_modulation
  - Panning = center + LFO1_modulation
  ↓
3 Sine Oscillators with modulated FM Feedback
  → Sum → Saturation (modulated) → Filter
  → ADSR → Panning (modulated) → Voice Mix
  ↓
Global Reverb (reverb_amount) → Output
```

**Test Criteria:**
- [ ] Random LFOs create subtle, organic movement (not perceivable as periodic)
- [ ] Panning modulation creates stereo width variation
- [ ] FM depth modulation creates evolving timbre
- [ ] Saturation modulation creates warmth variation
- [ ] Nested modulation creates non-repeating evolution (listen for 2-3 minutes)
- [ ] LFO interpolation is smooth (no clicks or stepping)
- [ ] reverb_amount parameter controls wet/dry mix
- [ ] Reverb adds lush, spacious character
- [ ] CPU usage acceptable (<200% single core for 8 voices)
- [ ] Sound never repeats exactly (generative quality achieved)

**Duration:** 50 min

---

### Stage 4: GUI Phases

#### Phase 4.1: Layout and Basic Controls

**Goal:** Integrate mockup HTML and set up WebView with 3 knobs

**Components:**
- Copy v2-ui.html to Source/ui/public/index.html
- Update PluginEditor.h/cpp with WebView setup
- Configure CMakeLists.txt for WebView resources (index.html, index.js, check_native_interop.js)
- Create WebSliderRelay for each parameter (timbre, filter_cutoff, reverb_amount)
- Resource provider with explicit URL mapping

**Test Criteria:**
- [ ] WebView window opens at 600×300 pixels
- [ ] All 3 knobs visible and styled correctly (neomorphic soft design)
- [ ] Layout matches mockup (Timbre left, Filter center, Reverb right)
- [ ] Background gradient renders (cream #d4c4b0 to #b8a690)
- [ ] Subtle modulation glow visible behind knobs

**Duration:** 15 min

---

#### Phase 4.2: Parameter Binding and Interaction

**Goal:** Two-way parameter communication (UI ↔ DSP)

**Components:**
- JavaScript → C++ relay calls via WebSliderParameterAttachment
- C++ → JavaScript parameter updates (host automation)
- Knob drag interaction (relative drag, NOT absolute positioning)
- Value change event listeners (remember: no parameters passed, must call getNormalisedValue())
- ES6 module imports (type="module" required for getSliderState)

**Test Criteria:**
- [ ] Knob movements change DSP parameters (audible effect)
- [ ] Host automation updates UI knobs (e.g., DAW automation lane)
- [ ] Preset changes update all 3 knobs
- [ ] Knob drag uses relative motion (not absolute cursor position)
- [ ] No knob jumping or frozen controls
- [ ] Value change events fire correctly (using getNormalisedValue() inside callback)
- [ ] WebSliderParameterAttachment uses 3 parameters (parameter, relay, nullptr)

**Duration:** 15 min

---

#### Phase 4.3: Advanced UI - Modulation Glow Animation

**Goal:** Add pulsing glow effect showing hidden LFO activity

**Components:**
- Modulation glow layer behind knobs (CSS radial gradient)
- Pulsing animation (smooth breathing effect, 3-5 second cycles)
- Color shift based on modulation intensity (subtle warm glow)
- requestAnimationFrame loop for smooth 60fps animation
- No C++ → JS data needed (purely decorative, randomized in JS)

**Test Criteria:**
- [ ] Glow pulses smoothly behind all 3 knobs
- [ ] Animation runs at 60fps without jank
- [ ] Glow intensity varies organically (simulating hidden LFO activity)
- [ ] Visual polish matches "lush/organic aesthetic" from brief
- [ ] No performance impact (CPU usage stays acceptable)

**Duration:** 10 min

---

### Estimated Duration

Total: ~165 minutes (~2.75 hours)

- Stage 2: 5 min (Foundation - project structure)
- Stage 3: 5 min (Shell - APVTS parameters)
- Stage 4: 125 min (DSP - 3 phases)
  - Phase 4.1: 40 min (Core polyphonic synthesis)
  - Phase 4.2: 35 min (Timbre processing)
  - Phase 4.3: 50 min (Generative modulation + reverb)
- Stage 5: 40 min (GUI - 3 phases)
  - Phase 5.1: 15 min (Layout and basic controls)
  - Phase 5.2: 15 min (Parameter binding)
  - Phase 5.3: 10 min (Modulation glow animation)
- Stage 6: 20 min (Validation - presets, pluginval, changelog)

---

## Implementation Notes

### Thread Safety

- All parameter reads use atomic `getRawParameterValue()->load()` (APVTS pattern)
- Voice state is per-voice (no shared state, safe for parallel processing)
- LFO phase state is per-voice (independent, no cross-voice dependencies)
- Filter state is per-voice (juce::dsp::IIR::Filter manages internal state)
- Reverb is global but only accessed by audio thread (no contention)
- No manual synchronization needed (APVTS handles parameter thread safety)

### Performance

**Estimated CPU usage:**
- Per-voice: ~19% CPU (oscillators 8%, saturation 2%, filter 3%, ADSR 1%, LFOs 5%)
- 8 voices: ~152% CPU
- Reverb: ~30% CPU
- **Total: ~180% single core** → Requires ~2 cores for full polyphony

**Optimization opportunities if needed:**
- Reduce polyphony to 6 voices (saves ~38% CPU)
- Simplify LFO system to 2 layers instead of 3 (saves ~15% CPU)
- Use SIMD for oscillator bank (potential 2-3x speedup)
- Fall back to single-layer random modulation (saves ~30% LFO CPU)

**Hot paths:**
- Oscillator generation with FM feedback (called 24 times per sample for 8 voices)
- LFO update and cross-modulation (called 72 times per block)

### Latency

- FM feedback: 1 sample delay
- IIR filter: ~1 sample group delay
- Reverb: 30ms pre-delay + ~20ms internal = ~50ms total
- **Total latency: ~50ms (2400 samples @ 48kHz)**
- Report via `getLatencySamples()` for host compensation
- Latency acceptable for pad use case (not percussive)

### Denormal Protection

- Use `juce::ScopedNoDenormals` in `processBlock()` (standard JUCE pattern)
- All JUCE DSP components handle denormals internally (IIR::Filter, Reverb, ADSR)
- Custom oscillator phase wrapping prevents denormals (phase stays in [0, 2π])
- LFO phase wrapping prevents denormals (phase stays in [0, 2π])

### Known Challenges

**Nested LFO Tuning (Phase 3.3):**
- Finding frequency ranges that feel organic requires experimentation
- Too fast: Perceivable wobble (sounds like vibrato/tremolo)
- Too slow: No perceivable evolution (defeats purpose)
- Target: 0.01-0.2 Hz (very slow, imperceptible but effective)
- Reference: Omnisphere Orb modulation, Generate chaotic oscillators
- Fallback: Simplify to 2 layers or single layer if too complex

**FM Feedback Stability (Phase 3.2):**
- Feedback depth >0.5 often becomes harsh/chaotic
- Maximum tuned to 0.4 (when timbre = 1.0) to stay musical
- Random modulation adds ±20%, could push to 0.48 worst-case
- Add soft limiting if instability detected (tanh clipping on feedback signal)
- Test across note ranges (low notes more prone to feedback issues)

**Voice Allocation Edge Cases (Phase 3.1):**
- 9th note triggers while 8 voices busy: Should steal oldest, not crash
- Rapid arpeggios: Voices should cycle smoothly without glitches
- Force ADSR release on stolen voice (prevents pops)
- Test: Hold 8-note chord, trigger 9th note repeatedly

**WebView ES6 Module Loading (Phase 5.2):**
- MUST use `type="module"` on script tags (see juce8-critical-patterns.md #21)
- MUST import getSliderState: `import { getSliderState } from './js/juce/index.js'`
- Frozen knobs = missing type="module" or missing import
- Reference: GainKnob working implementation

**WebView Parameter Binding (Phase 5.2):**
- WebSliderParameterAttachment requires 3 parameters in JUCE 8 (see pattern #12)
- Correct: `(parameter, relay, nullptr)`
- Wrong: `(parameter, relay)` → compiles but doesn't bind (silent failure)
- valueChangedEvent callback receives NO parameters (see pattern #15)
- Must call `getNormalisedValue()` inside callback, not use callback parameter

**Relative Knob Drag (Phase 5.2):**
- Use frame-delta pattern: `lastY - e.clientY` NOT `startY - e.clientY`
- Increment rotation: `rotation += deltaY * sensitivity`
- Update tracking: `lastY = e.clientY` after each frame
- Reference: Pattern #16 in juce8-critical-patterns.md

---

## References

**Contract files:**
- Creative brief: `plugins/LushPad/.ideas/creative-brief.md`
- Parameter spec: `plugins/LushPad/.ideas/parameter-spec.md`
- DSP architecture: `plugins/LushPad/.ideas/architecture.md`
- UI mockup: `plugins/LushPad/.ideas/mockups/v2-ui.yaml`

**Similar plugins for reference:**
- GainKnob - WebView knob interaction, ES6 module imports
- FlutterVerb - juce::dsp::Reverb modern API usage
- TapeAge - WebView parameter binding patterns

**Critical patterns:**
- `troubleshooting/patterns/juce8-critical-patterns.md` - REQUIRED READING before implementation
  - Pattern #4: Bus config (output-only for synth, no input)
  - Pattern #9: NEEDS_WEB_BROWSER TRUE for VST3
  - Pattern #11: WebView member initialization (use std::unique_ptr)
  - Pattern #12: WebSliderParameterAttachment 3-parameter constructor
  - Pattern #15: valueChangedEvent callback (no parameters passed)
  - Pattern #16: Relative knob drag (frame-delta pattern)
  - Pattern #17: juce::dsp::Reverb modern API (prepare/process, not setSampleRate)
  - Pattern #21: ES6 module loading (type="module" required)
