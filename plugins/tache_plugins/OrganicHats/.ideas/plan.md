# OrganicHats - Implementation Plan

**Date:** 2025-11-12
**Complexity Score:** 5.0 (Complex)
**Strategy:** Phase-based implementation

---

## Complexity Factors

- **Parameters:** 6 parameters (1.2 points, capped at 2.0) = 1.2
- **Algorithms:** 7 DSP components = 7
  - Noise Generator (juce::Random)
  - Tone Filter (juce::dsp::IIR::Filter) - 2 instances
  - Noise Color Filter (juce::dsp::IIR::Filter) - 2 instances
  - Resonators (juce::dsp::IIR::Filter) - 3 fixed peaks
  - Envelope Generator (juce::ADSR) - 2 instances
  - Velocity Processor (custom)
  - Choke Logic (custom state-based)
- **Features:** 2 points
  - ADSR envelopes (+1)
  - External MIDI control (note triggers + velocity) (+1)
- **Total:** 10.2 (capped at 5.0)

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

#### Phase 4.1: Voice Architecture and Noise Generation

**Goal:** Establish Synthesiser/SynthesiserVoice architecture with MIDI note routing and basic noise generation

**Components:**
- Implement SynthesiserSound subclass (hi-hat sound descriptor)
- Implement SynthesiserVoice subclass with per-voice state
- MIDI note routing (C1 = closed, D1 = open)
- Noise generator (juce::Random per voice)
- Basic ADSR envelopes (closed decay, open release)
- Voice allocation and polyphony management (16 voices max)

**Test Criteria:**
- [ ] Plugin loads in DAW without crashes
- [ ] MIDI notes C1 and D1 trigger separate voices
- [ ] White noise generated per voice (audible output)
- [ ] Velocity affects volume (linear scaling 0.0-1.0)
- [ ] ADSR envelopes shape amplitude correctly
- [ ] Multiple voices playable simultaneously (polyphony works)

**Duration:** 45 min

---

#### Phase 4.2: Filtering and Tone Shaping

**Goal:** Implement dual filter stages (Tone + Noise Color) for brightness and warmth control

**Components:**
- Tone Filter (juce::dsp::IIR::Filter) - Butterworth HP/LP per voice
  - Exponential frequency mapping (3kHz-15kHz)
  - Separate filter instances for closed and open voices
- Noise Color Filter (juce::dsp::IIR::Filter) - Butterworth HP/LP per voice
  - Exponential frequency mapping (5kHz-10kHz)
  - Bypass zone at 50% ±2%
- Velocity-to-tone modulation (+30% cutoff max)
- Parameter updates in audio thread (coefficient recalculation)

**Test Criteria:**
- [ ] CLOSED_TONE parameter controls brightness (3kHz-15kHz)
- [ ] OPEN_TONE parameter controls brightness independently
- [ ] CLOSED_NOISE_COLOR parameter controls warmth/brightness
- [ ] OPEN_NOISE_COLOR parameter controls warmth independently
- [ ] Noise Color bypass at 50% (no filtering)
- [ ] Higher velocity produces brighter tone
- [ ] No clicks or artifacts when sweeping parameters
- [ ] Filter coefficient updates smooth and real-time

**Duration:** 40 min

---

#### Phase 4.3: Resonators and Choke Logic

**Goal:** Add fixed resonance peaks for organic body and implement closed-chokes-open behavior

**Components:**
- Three resonant peak filters per voice (7kHz, 10kHz, 13kHz)
  - Fixed Q factor (3.0-5.0)
  - Fixed gain (-6dB per peak)
  - Initialized in prepareToPlay(), no runtime updates
- Choke logic (state-based)
  - Closed hi-hat note-on checks for active open voices
  - Force open voice ADSR to release phase (<5ms cutoff)
  - Voice state tracking via isActive() or custom flag

**Test Criteria:**
- [ ] Resonators add organic body to hi-hat sound
- [ ] Resonance peaks audible but not harsh
- [ ] Closed hi-hat trigger instantly cuts open hi-hat (<5ms)
- [ ] Choke only applies when open voice is active
- [ ] No choke if open voice already silent
- [ ] Multiple open voices choked simultaneously if needed
- [ ] Overall sound is warm and non-metallic

**Duration:** 35 min

---

### Stage 5: GUI Phases

#### Phase 5.1: WebView Layout and Basic Integration

**Goal:** Integrate mockup HTML and establish WebView infrastructure

**Components:**
- Copy v2-ui.html to Source/ui/public/index.html
- Update PluginEditor.h/cpp with WebView setup (from v2-PluginEditor.h/cpp)
- Configure CMakeLists.txt for WebView resources (from v2-CMakeLists.txt)
- Establish relay system for JavaScript ↔ C++ communication
- Render studio hardware aesthetic (brushed metal, yellow-gold accents)

**Test Criteria:**
- [ ] WebView window opens with correct size (600x590px)
- [ ] Dual-panel layout renders correctly (closed left, open right)
- [ ] All 6 knobs visible and styled correctly
- [ ] Background texture and styling render properly
- [ ] Mounting holes and power LED render
- [ ] Section headers display correctly

**Duration:** 25 min

---

#### Phase 5.2: Parameter Binding (6 parameters)

**Goal:** Two-way parameter communication (UI ↔ DSP)

**Components:**
- JavaScript → C++ relay calls (6 parameters)
  - CLOSED_TONE, CLOSED_DECAY, CLOSED_NOISE_COLOR
  - OPEN_TONE, OPEN_RELEASE, OPEN_NOISE_COLOR
- C++ → JavaScript parameter updates (host automation)
- Value formatting and display (digital readouts below knobs)
- Real-time parameter updates during playback

**Test Criteria:**
- [ ] CLOSED_TONE knob changes DSP brightness (closed)
- [ ] CLOSED_DECAY knob changes envelope decay time
- [ ] CLOSED_NOISE_COLOR knob changes warmth/brightness
- [ ] OPEN_TONE knob changes DSP brightness (open)
- [ ] OPEN_RELEASE knob changes envelope release time
- [ ] OPEN_NOISE_COLOR knob changes warmth/brightness
- [ ] Host automation updates all UI controls
- [ ] Preset changes update all UI elements
- [ ] Digital readouts display correct values with units
- [ ] No lag or visual glitches

**Duration:** 30 min

---

#### Phase 5.3: Power LED and Polish

**Goal:** Add power LED indicator and final UI polish

**Components:**
- Power LED indicator (top-right, pulse animation)
- LED shows plugin active state
- Final styling polish (mounting holes, borders, shadows)
- Test UI responsiveness and smoothness

**Test Criteria:**
- [ ] Power LED renders with pulse animation
- [ ] LED indicates plugin is active
- [ ] All visual elements render smoothly
- [ ] No UI performance issues or CPU spikes
- [ ] Studio hardware aesthetic fully realized
- [ ] Design matches v2 mockup

**Duration:** 15 min

---

### Estimated Duration

Total: ~3.2 hours

- Stage 2: 5 min (Foundation - project structure)
- Stage 3: 5 min (Shell - APVTS parameters, 6 parameters)
- Stage 4: 120 min (DSP - 3 phases)
  - Phase 4.1: 45 min (Voice architecture + noise)
  - Phase 4.2: 40 min (Filtering + tone shaping)
  - Phase 4.3: 35 min (Resonators + choke logic)
- Stage 5: 70 min (GUI - 3 phases)
  - Phase 5.1: 25 min (WebView layout)
  - Phase 5.2: 30 min (Parameter binding)
  - Phase 5.3: 15 min (Power LED + polish)
- Stage 6: 20 min (Validation - presets, pluginval, changelog)

---

## Implementation Notes

### Thread Safety
- All parameter reads use atomic `getRawParameterValue()->load()`
- Filter coefficient updates happen in audio thread (no allocations)
- ADSR parameters updated via atomic `setParameters()` call
- Per-voice Random instance (no shared state between voices)
- Choke logic operates on voice state (no mutex required)

### Performance
- Noise generation: ~1% CPU (lightweight)
- 2x Tone filters: ~5% CPU per voice
- 2x Noise Color filters: ~5% CPU per voice
- 3x Resonators: ~8% CPU per voice
- 2x ADSR envelopes: ~2% CPU per voice
- Total per voice: ~25% single core at 48kHz
- Typical usage: 2-4 active voices → 50-100% single core

### Latency
- Zero-latency processing (real-time synthesis)
- No lookahead or delay buffers required
- Report via getLatencySamples(): return 0

### Denormal Protection
- Use `juce::ScopedNoDenormals` in processBlock()
- All JUCE DSP components handle denormals internally
- ADSR class handles denormals via internal clamping

### Known Challenges

**Voice Architecture:**
- Must subclass both SynthesiserSound and SynthesiserVoice
- Per-voice state isolation critical for thread safety
- MIDI note routing: C1 (36) = closed, D1 (38) = open
- Reference: JUCE Synthesiser tutorial and ClapMachine (planned synth)

**Choke Logic:**
- Closed hi-hat must detect active open voices
- Force open voice ADSR to release with <5ms time
- Use `adsr.noteOff()` or custom fast release
- Track voice state via `isActive()` or custom flag
- Reference: CR-78/808 drum machine behavior

**Filter Coefficient Updates:**
- Exponential frequency mapping for musical response
- Update coefficients on parameter change (audio thread safe)
- Avoid clicks when sweeping (use smoothing if needed)
- Noise Color bypass zone (50% ±2%) prevents clicking
- Reference: GainKnob filter implementation

**Velocity Modulation:**
- Velocity affects both volume and tone brightness
- Volume: linear scaling (velocity / 127.0)
- Tone: +30% cutoff max at full velocity
- Apply to filter cutoff before coefficient calculation

**Resonators:**
- Fixed peak frequencies (7kHz, 10kHz, 13kHz)
- Moderate Q (3.0-5.0) for organic body
- Initialize in prepareToPlay(), no runtime updates
- Series or parallel routing? Test both for best sound

**WebView Integration:**
- Studio Hardware aesthetic requires custom CSS
- Digital readouts below knobs (Courier New monospace)
- Power LED pulse animation (CSS keyframes)
- 6 parameter bindings (3 closed + 3 open)
- Reference: TapeAge, DriveVerb, FlutterVerb for WebView patterns

---

## References

**Contract Files:**
- Creative brief: `plugins/OrganicHats/.ideas/creative-brief.md`
- Parameter spec: `plugins/OrganicHats/.ideas/parameter-spec.md`
- DSP architecture: `plugins/OrganicHats/.ideas/architecture.md`
- UI mockup: `plugins/OrganicHats/.ideas/mockups/v2-ui.yaml`

**Similar Plugins for Reference:**
- **GainKnob** - Filter implementation (Butterworth IIR, exponential mapping)
- **TapeAge** - WebView UI patterns, parameter binding
- **DriveVerb** - Dual-panel layout, knob styling
- **FlutterVerb** - ADSR envelope usage, VU meter animation
- **ClapMachine** (planned) - Synth architecture reference for voice management
