# O-Gain - Implementation Plan

**Date:** 2026-03-07
**Complexity Score:** 3.0 (Complex)
**Strategy:** Phase-based implementation

---

## Complexity Factors

- **Parameters:** 10 parameters (10/5 = 2.0 points, capped at 2.0) = **2.0**
- **Algorithms:** 4 DSP components = **0** (counted in features below)
  - K-weighted IIR filters (pre-filter + RLB)
  - LUFS gating block accumulator
  - EBU R128 dual-gate system
  - True peak detection (4x polyphase FIR)
- **Features:** 1 point
  - Modulation systems: No (+0)
  - Feedback loops: No (+0)
  - FFT/frequency domain: No (+0)
  - Multiband processing: No (+0)
  - External MIDI control: No (+0)
  - Complex measurement system (K-weighting + dual-gate + true peak): **+1** (custom, non-trivial DSP)
- **Raw Total:** 2.0 (params) + 0 (algorithms counted via features) + 1.0 (features) = **3.0**
- **Final Score:** min(3.0, 5.0) = **3.0**

**Note:** The complexity here is driven by the parameter count (10 APVTS parameters) and the K-weighted LUFS measurement system which requires custom implementation of BS.1770 filters and EBU R128 gating. The actual DSP processing (gain multiplication, channel utilities) is trivially simple. The complexity is concentrated in the measurement subsystem.

---

## Stages

- Stage 0: Research -- Complete
- Stage 1: Foundation (build system + parameters) -- Next
- Stage 2: Shell (APVTS parameter wiring)
- Stage 3: DSP -- 2 phases
  - Phase 3.1: Core Processing (gain + channel utilities + basic metering)
  - Phase 3.2: Learn Mode + LUFS Measurement
- Stage 4: GUI -- 2 phases
  - Phase 4.1: Layout and Basic Controls
  - Phase 4.2: Meter Display and Learn Interaction
- Stage 5: Validation (presets, pluginval, changelog)

---

## Complex Implementation (Score >= 3.0)

### Stage 3: DSP Phases

#### Phase 3.1: Core Processing

**Goal:** Establish the audio processing path with gain application, channel utilities, and basic peak/RMS metering. After this phase, the plugin is a functional gain utility with channel tools.

**Components:**
- Gain stage: `juce::dsp::Gain<float>` applying gain_offset + trim
- Channel utilities: phase inversion (L/R), channel swap, mono sum, M/S encode/decode
- Basic metering: peak detection (input/output), RMS calculation
- Signal chain: Input -> Utilities -> Metering (input) -> Gain -> Metering (output) -> Output
- Atomic float metering values for UI display

**Test Criteria:**
- [ ] Plugin loads in DAW without crashes
- [ ] Audio passes through with unity gain at default settings
- [ ] gain_offset knob applies gain correctly (-40 to +40 dB range)
- [ ] trim knob applies fine gain adjustment (-6 to +6 dB range)
- [ ] Phase invert L/R works correctly (polarity flip)
- [ ] Channel swap works correctly (L and R exchanged)
- [ ] Mono sum works correctly (L+R)/2 on both channels
- [ ] M/S encode and decode work correctly (round-trip preserves signal)
- [ ] Peak metering shows correct levels (input and output)
- [ ] RMS metering shows correct levels
- [ ] No clicks or artifacts when changing gain parameters
- [ ] Zero latency confirmed (DAW reports 0 samples latency)

---

#### Phase 3.2: Learn Mode + LUFS Measurement

**Goal:** Implement the complete Learn workflow including K-weighted LUFS measurement, EBU R128 dual-gate, true peak detection, and gain calculation with safety clamping.

**Components:**
- K-weighting filters: pre-filter (high-shelf) + RLB (high-pass) using `juce::dsp::IIR::Filter<double>`
- LUFS gating block accumulator: 400ms blocks with 100ms hop
- EBU R128 dual-gate: absolute gate (-70 LUFS) + relative gate (-10 LU)
- True peak detection: digital peak for MVP, polyphase FIR for v1.0
- VU meter ballistics: `juce::dsp::BallisticsFilter<float>` (300ms attack/release)
- Learn state machine: idle/learning/complete with atomic flag
- Gain calculation: target - measured, clamped, true peak safety check
- Confidence indicator: based on duration and block count
- Momentary/short-term LUFS for display during Learn

**Test Criteria:**
- [ ] Learn mode starts and stops correctly via atomic flag
- [ ] K-weighting filters produce correct frequency response (verify with test tone)
- [ ] LUFS measurement produces correct values for known test signals
- [ ] Dual-gate correctly removes silence and quiet passages
- [ ] True peak detection catches inter-sample peaks (compare with reference meter)
- [ ] Gain calculation correctly computes target - measured
- [ ] Gain is clamped to parameter range (-40 to +40 dB)
- [ ] True peak safety check prevents gain that would exceed -1 dBTP ceiling
- [ ] gain_offset parameter updates correctly after Learn completes
- [ ] VU meter ballistics display smooth, authentic 300ms response
- [ ] Confidence indicator shows Low/Medium/High correctly based on duration
- [ ] Meter mode switching works: Peak, RMS, VU, LUFS display correctly
- [ ] No crashes or glitches when Learn is started/stopped rapidly
- [ ] Measurement is accurate across different sample rates (44.1k, 48k, 96k)
- [ ] CPU usage during Learn mode is acceptable (< 15% single core per instance)

---

### Stage 4: GUI Phases

#### Phase 4.1: Layout and Basic Controls

**Goal:** Integrate the HTML/WebView UI with basic parameter bindings for all controls.

**Components:**
- WebView setup with resource provider
- Central gain knob (large, -40 to +40 dB display)
- Trim knob (small, -6 to +6 dB)
- Target level selector (dropdown or knob with preset values)
- Measurement mode selector (LUFS/RMS toggle)
- Meter mode selector (Peak/RMS/VU/LUFS)
- Utility buttons: Phase L, Phase R, Swap, Mono, M/S mode
- WebSliderRelay and WebSliderParameterAttachment for all parameters

**Test Criteria:**
- [ ] WebView window opens with correct size (compact, narrow layout)
- [ ] All controls visible and styled correctly
- [ ] Gain knob responds to drag and updates parameter
- [ ] Trim knob responds to drag and updates parameter
- [ ] Target level control works with preset values
- [ ] Utility buttons toggle correctly (Phase L, Phase R, Swap, Mono, M/S)
- [ ] Host automation updates all UI controls correctly
- [ ] Layout is narrow enough for mixer view (target: ~200px wide)

---

#### Phase 4.2: Meter Display and Learn Interaction

**Goal:** Implement real-time meter visualization and Learn mode UI interaction.

**Components:**
- Input/output level meters (animated, mode-switchable)
- Learn button with visual state indicator (idle/learning/complete)
- VU meter display with smooth ballistic animation
- During-Learn display: momentary LUFS, short-term LUFS, integrated LUFS, elapsed time
- Confidence indicator (Low/Medium/High)
- True peak warning indicator
- requestAnimationFrame loop for meter smoothing
- C++ -> JS meter data relay via custom events

**Test Criteria:**
- [ ] Meters animate smoothly at 60fps (requestAnimationFrame)
- [ ] Meter mode switching changes display type correctly
- [ ] VU meter shows authentic ballistic response (300ms rise, smooth decay)
- [ ] Learn button shows visual feedback for idle/learning/complete states
- [ ] During Learn: momentary, short-term, and integrated LUFS display correctly
- [ ] Elapsed time counter works during Learn
- [ ] Confidence indicator updates in real-time during Learn
- [ ] True peak warning appears when calculated gain would exceed ceiling
- [ ] Input and output meters show independent levels
- [ ] Performance acceptable with 10+ instances open (no UI lag)
- [ ] All meter data updates from C++ to JS are smooth and glitch-free

---

### Implementation Flow

- Stage 0: Research -- Complete
- Stage 1: Foundation -- project structure, CMakeLists.txt, PluginProcessor/Editor stubs
- Stage 2: Shell -- APVTS parameters (10 parameters), basic processBlock pass-through
- Stage 3: DSP -- 2 phases
  - Phase 3.1: Core Processing (gain + utilities + basic metering)
  - Phase 3.2: Learn Mode + LUFS Measurement (K-weighting, gating, true peak, VU ballistics)
- Stage 4: GUI -- 2 phases
  - Phase 4.1: Layout and Basic Controls
  - Phase 4.2: Meter Display and Learn Interaction
- Stage 5: Validation -- presets, pluginval, changelog

---

## Implementation Notes

### Thread Safety
- All parameter reads use atomic `getRawParameterValue()->load()`
- Learn active flag: `std::atomic<bool>` (UI sets, audio reads)
- Meter values: `std::atomic<float>` per channel per meter type
- Learn accumulation: single-producer (audio) / single-consumer (UI after stop)
- No mutexes on audio thread
- No allocations on audio thread (pre-allocate block vectors in prepareToPlay)

### Performance
- Channel utilities: < 1% CPU per instance (trivial sample ops)
- Gain application: < 1% CPU (single multiply with smoothing)
- K-weight measurement (during Learn only): ~5% CPU (two biquad filters + accumulation)
- Total idle: < 1% CPU per instance (critical for 40+ instance usage)
- Total during Learn: ~10-12% CPU per instance at 48 kHz
- Designed for 40+ simultaneous instances in a mixing session

### Latency
- Zero latency (0 samples)
- No FFT, no lookahead, no oversampling in signal path
- True peak detection is a measurement side-chain (does not affect signal path)
- Report `setLatencySamples(0)` in prepareToPlay

### Denormal Protection
- Use `juce::ScopedNoDenormals` in processBlock()
- K-weight IIR filters can generate denormals during silence
- BallisticsFilter has internal snap-to-zero

### Known Challenges
- K-weight filter coefficient recalculation for non-48kHz sample rates requires bilinear transform. Mitigation: pre-calculate for common rates (44100, 48000, 88200, 96000).
- Overlapping 400ms gating blocks with 100ms hop require circular buffer management. Mitigation: use `std::vector<double>` with index wrapping.
- LUFS accuracy validation. Mitigation: test against EBU test signals or reference meter (Youlean, dpMeter5).
- UI meter refresh throttling for 40+ instances. Mitigation: timer-based polling at 30Hz, not processBlock-driven.
- M/S encode/decode scaling convention must be consistent (use 0.5 scaling on both encode and decode for unity round-trip).

---

## References

- Creative brief: `plugins/O-Gain/.planning/BRIEF.md`
- Parameter spec: `plugins/O-Gain/.planning/parameter-spec-draft.md`
- DSP architecture: `plugins/O-Gain/.planning/research/ARCHITECTURE.md`
- Market research: `research/gain-staging-plugin-market-research.md`
- Textbook research: `research/gain-staging-metering-loudness-textbook-research.md`

### Similar Plugins for Reference
- **GainKnob** - Reference for basic gain utility pattern (WebView + APVTS + simple DSP)
- **O-Comp** - Reference for metering display in WebView (VU meter with ballistic animation)
- **O-MultiBandCompressor** - Reference for multi-mode processing and parameter organization
