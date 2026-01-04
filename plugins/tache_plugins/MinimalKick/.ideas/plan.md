# MinimalKick - Implementation Plan

**Date:** 2025-11-13
**Complexity Score:** 5.0 (Complex)
**Strategy:** Phase-based implementation

---

## Complexity Factors

- **Parameters:** 5 parameters (5/5 = 1.0, capped at 2.0) = 1.0
- **Algorithms:** 4 DSP components = 4.0
  - Sine wave oscillator (juce::dsp::Oscillator)
  - Custom pitch envelope (exponential decay)
  - Amplitude envelope (juce::ADSR)
  - Saturation/drive (tanh waveshaping)
- **Features:** 0 points
  - MIDI input (standard synth architecture, no complexity added)
  - Monophonic voice (simpler than polyphonic, no complexity added)
- **Total:** 5.0 (capped at 5.0)

---

## Stages

- Stage 0: Research ✓
- Stage 1: Planning ✓
- Stage 2: Foundation ← Next
- Stage 3: Shell
- Stage 4: DSP (3 phases)
- Stage 5: GUI (3 phases)
- Stage 6: Validation

---

## Complex Implementation (Score = 5.0)

### Stage 4: DSP Phases

#### Phase 4.1: Core Synthesis (Oscillator + MIDI + Amplitude Envelope)

**Goal:** Implement basic kick drum synthesis without pitch modulation

**Components:**
- MIDI input handling (note-on triggers, note-to-Hz conversion)
- Sine wave oscillator (juce::dsp::Oscillator with sine wave)
- Amplitude envelope (juce::ADSR configured as AD with sustain=0)
- Monophonic voice management (single voice, retriggerable)
- Signal flow: MIDI → Oscillator → Amplitude Envelope → Output

**Test Criteria:**
- [ ] Plugin loads in DAW without crashes
- [ ] MIDI notes trigger sine wave output
- [ ] Base pitch follows MIDI note correctly (C1=~32Hz, C3=~130Hz)
- [ ] Attack parameter (0-50ms) shapes note onset
- [ ] Decay parameter (50-2000ms) shapes note release
- [ ] Envelope decays to silence (sustain=0)
- [ ] Retriggering works (new note restarts envelope)
- [ ] No clicks or artifacts on note-on/off

**Duration:** 90 min

**Implementation Notes:**
- Use `juce::MidiMessage::getMidiNoteInHertz()` for MIDI-to-Hz conversion
- Initialize oscillator with sine wave: `osc.initialise([](float x) { return std::sin(x); }, 128);`
- Configure ADSR: attack=parameter, decay=parameter, sustain=0.0, release=0.0
- Phase reset on note-on for consistent attack (call `osc.reset()`)
- Single voice state: track active/inactive, retrigger on new note-on

---

#### Phase 4.2: Pitch Envelope (Custom Exponential Implementation)

**Goal:** Add exponential pitch envelope to oscillator frequency

**Components:**
- Custom pitch envelope generator (exponential decay)
- Sweep parameter (0-24 semitones starting offset)
- Time parameter (5-500ms decay time)
- Frequency modulation (base pitch + envelope offset)
- Phase-continuous frequency changes (no clicks)

**Test Criteria:**
- [ ] Sweep parameter controls starting pitch offset (0 st = no sweep, 24 st = 2 octaves up)
- [ ] Time parameter controls decay speed (5ms = fast click, 500ms = slow laser)
- [ ] Pitch decays exponentially from sweep offset to base pitch
- [ ] No clicks or discontinuities during pitch sweep
- [ ] Different MIDI notes produce different base pitches
- [ ] Envelope retriggers on new note-on (phase reset)
- [ ] Typical 808-style kick (sweep=12st, time=50ms) sounds punchy

**Duration:** 120 min

**Implementation Notes:**
- Exponential decay formula: `offset = sweepSemitones * exp(-decayRate * time)`
- Decay rate calculation: `decayRate = -log(0.001) / decayTimeSeconds` (decay to 0.1%)
- Frequency calculation: `freq = baseFreq * pow(2.0, offset / 12.0)` (semitone formula)
- Update frequency every sample or every block (block update more CPU efficient)
- juce::dsp::Oscillator handles phase continuity internally when setFrequency() called
- Reset envelope on note-on (synchronized with amplitude envelope)

---

#### Phase 4.3: Saturation/Drive (Tanh Waveshaping)

**Goal:** Add warm saturation for harmonic content

**Components:**
- Drive parameter (0-100% controls saturation amount)
- Tanh waveshaping (symmetric, odd harmonics)
- Gain mapping (drive 0% → gain 1.0, drive 100% → gain 10.0)
- Applied after amplitude envelope (saturation on final amplitude)

**Test Criteria:**
- [ ] Drive parameter adds audible warmth and harmonics
- [ ] Drive 0% = clean sine wave (unity gain, no saturation)
- [ ] Drive 100% = heavy saturation (audible harmonics)
- [ ] No aliasing artifacts (kick drum bandwidth limited)
- [ ] No level jumps or discontinuities when changing drive
- [ ] Saturation sounds musical (warm, not harsh)

**Duration:** 60 min

**Implementation Notes:**
- Transfer function: `output = std::tanh(gain * input)`
- Gain mapping: `gain = 1.0 + (driveNormalized * 9.0)` where driveNormalized is 0.0-1.0
- Apply per-sample after amplitude envelope multiplication
- No oversampling needed (kick drum content mostly <200Hz, aliasing inaudible)
- Symmetric waveshaping produces odd harmonics only (3rd, 5th, 7th - warm character)

---

### Stage 5: GUI Phases

#### Phase 5.1: Layout and Basic Controls

**Goal:** Integrate mockup HTML and display all controls

**Components:**
- Copy v2-ui.html to Source/ui/public/index.html
- Update PluginEditor.h/cpp with WebView setup
- Configure CMakeLists.txt for WebView resources (JUCE_COPY_PLUGIN_AFTER_BUILD)
- Set window size to 730×280px (matches mockup)
- Verify all 5 knobs render correctly

**Test Criteria:**
- [ ] WebView window opens with correct size (730×280px)
- [ ] All 5 rotary knobs visible (sweep, time, attack, decay, drive)
- [ ] Layout matches mockup (horizontal row, 50px gaps, 40px edge margins)
- [ ] Background styling renders (dark brown, brass accents, scan lines, vignette)
- [ ] Typography renders correctly (uppercase brass/gold, wide letter-spacing)
- [ ] No JavaScript console errors

**Duration:** 45 min

---

#### Phase 5.2: Parameter Binding and Interaction

**Goal:** Two-way parameter communication (UI ↔ DSP)

**Components:**
- JavaScript → C++ relay calls (knob changes call C++ updateParameter)
- C++ → JavaScript parameter updates (host automation triggers evaluateJavascript)
- APVTS attachments for 5 parameters (sweep, time, attack, decay, drive)
- Value formatting (semitones, milliseconds, percentage)
- Real-time updates during DAW playback

**Test Criteria:**
- [ ] Knob movements change DSP parameters (audible effect)
- [ ] Host automation updates UI knobs (visual sync)
- [ ] Preset changes update all 5 knobs
- [ ] Parameter values display correctly with units (12 st, 50 ms, 20%)
- [ ] No lag between UI interaction and DSP response
- [ ] No visual glitches during rapid parameter changes

**Duration:** 60 min

**Implementation Notes:**
- Use relay pattern from JUCE 8 Critical Patterns (Pattern #15, #16)
- JavaScript calls: `window.webkit.messageHandlers.relay.postMessage({id: "sweep", value: 0.5})`
- C++ receives: `void emitEventFromJavaScript(const juce::String& eventName, const juce::var& args)`
- C++ sends: `webView->evaluateJavascript("updateParameter('sweep', 12.0)")`
- Create ParameterAttachment instances to link APVTS ↔ UI updates

---

#### Phase 5.3: Polish and Value Displays

**Goal:** Final visual polish and parameter readouts

**Components:**
- Real-time parameter value displays (numeric readouts with units)
- Knob hover states (visual feedback)
- Smooth knob rotation animation
- Parameter value formatting (1 decimal for st, 0 decimals for ms/%)
- Visual polish (anti-aliasing, subpixel rendering)

**Test Criteria:**
- [ ] Parameter values update in real-time as knobs move
- [ ] Units display correctly (st, ms, %)
- [ ] Knob rotation is smooth (no jitter or lag)
- [ ] Hover states provide visual feedback
- [ ] Typography is crisp and readable
- [ ] Overall aesthetic matches vintage hardware design

**Duration:** 30 min

---

### Estimated Duration

Total: ~6.5 hours

- Stage 2: 5 min (Foundation - project structure)
- Stage 3: 5 min (Shell - APVTS parameters)
- Stage 4: 270 min (DSP - 3 phases)
  - Phase 4.1: 90 min (Core synthesis)
  - Phase 4.2: 120 min (Pitch envelope - highest complexity)
  - Phase 4.3: 60 min (Saturation)
- Stage 5: 135 min (GUI - 3 phases)
  - Phase 5.1: 45 min (Layout)
  - Phase 5.2: 60 min (Parameter binding)
  - Phase 5.3: 30 min (Polish)
- Stage 6: 20 min (Validation - presets, changelog)

---

## Implementation Notes

### Thread Safety
- All parameter reads use atomic `getRawParameterValue()->load()`
- Oscillator state is per-voice (no shared state)
- Pitch envelope state is per-voice (no shared state)
- Amplitude envelope state is per-voice (no shared state)
- MIDI processing happens entirely in audio thread (processBlock)
- No locks needed (APVTS handles parameter thread safety)

### Performance
- Estimated CPU usage per component (48kHz, single core):
  - Oscillator: ~5% (wavetable lookup)
  - Pitch envelope: ~2% (exponential calculation per-sample)
  - Amplitude envelope: ~1% (ADSR per-sample)
  - Saturation: ~3% (tanh per-sample)
  - **Total:** ~11% single core (very efficient)
- Optimization opportunities:
  - Calculate pitch envelope per-block (not per-sample) for lower CPU
  - Use lookup table for exponential if needed (trade memory for CPU)

### Latency
- Zero algorithmic latency (no lookahead, no FFT, no buffers beyond single sample)
- Processing is sample-accurate
- No latency compensation needed
- No need to implement `getLatencySamples()`

### Denormal Protection
- Use `juce::ScopedNoDenormals` in processBlock()
- juce::dsp::Oscillator handles denormals internally
- Envelope values won't go denormal (decay to zero then stop generating)

### Known Challenges

**Pitch Envelope Phase Continuity (Phase 4.2 - HIGHEST RISK):**
- Problem: Frequency changes can cause phase discontinuities (clicks/pops)
- Solution: juce::dsp::Oscillator handles phase continuity internally when setFrequency() called
- Mitigation: Update frequency smoothly (per-sample or per-block), avoid large jumps
- Reference: TR-808/909 exponential pitch envelope architecture
- Fallback: Linear decay if exponential too complex (less authentic but safer)

**Custom Pitch Envelope Implementation:**
- No JUCE class exists for pitch envelopes (ADSR is for amplitude)
- Need custom exponential decay generator
- Formula: `offset = sweepSemitones * exp(-decayRate * elapsedTime)`
- Decay rate: `decayRate = -log(0.001) / decayTimeSeconds` (decay to 0.1% in specified time)
- Test thoroughly with different sweep/time combinations

**Monophonic Retriggering:**
- New note-on must reset oscillator phase (consistent attack)
- Both envelopes restart simultaneously (pitch + amplitude)
- No glide/portamento (instant frequency change to new note)
- Simple state machine: active (envelope running) / inactive (silent)

---

## References

**Contract files:**
- Creative brief: `plugins/MinimalKick/.ideas/creative-brief.md`
- Parameter spec: `plugins/MinimalKick/.ideas/parameter-spec.md`
- DSP architecture: `plugins/MinimalKick/.ideas/architecture.md`
- UI mockup: `plugins/MinimalKick/.ideas/mockups/v2-ui.yaml`

**Similar plugins for reference:**
- GainKnob - WebView parameter binding patterns, simple DSP architecture
- TapeAge - Custom DSP implementation (saturation, modulation), envelope handling
- OrganicHats - MIDI synth architecture, monophonic voice management, envelope triggering
- FlutterVerb - Complex phased implementation pattern, VU meter animation
