# AutoClip - Implementation Plan

**Date:** 2025-11-13
**Complexity Score:** 4.4 (Complex)
**Strategy:** Phase-based implementation

---

## Complexity Factors

- **Parameters:** 2 parameters (2/5 = 0.4 points, capped at 2.0) = 0.4
- **Algorithms:** 4 DSP components = 4
  - Hard Clipper
  - Lookahead Buffer
  - Peak-Based Gain Matching
  - Clip Solo (Delta Monitoring)
- **Features:** 0 points
  - No feedback loops
  - No FFT processing
  - No multiband processing
  - No modulation systems
  - No external MIDI control
- **Total:** 4.4 (capped at 5.0)

**Rationale for phased approach:**
Despite simple individual components, peak-based gain matching requires tuning (smoothing time, musical feel) and represents MEDIUM risk. Phased implementation provides checkpoints for testing and tuning before adding complexity.

---

## Stages

- Stage 0: Research ✓
- Stage 1: Planning ✓
- Stage 2: Foundation ← Next
- Stage 3: Shell
- Stage 4: DSP [phased]
- Stage 5: GUI [phased]
- Stage 6: Validation

---

## Stage 4: DSP Phases

### Phase 4.1: Core Processing (Hard Clipping + Lookahead)

**Goal:** Implement basic hard clipping with lookahead buffer

**Components:**
- Hard clipper algorithm (threshold-based clamping)
- Lookahead buffer (5ms delay using juce::dsp::DelayLine)
- Threshold parameter connection
- Input → Lookahead → Clip → Output path

**Implementation details:**
- Setup DelayLine with 5ms fixed delay
- Implement per-sample clipping: `juce::jlimit(-threshold, threshold, sample)`
- Process: Push input to lookahead → Pop delayed signal → Clip → Output
- Report latency via getLatencySamples() (5ms)

**Test Criteria:**
- [ ] Plugin loads in DAW without crashes
- [ ] Audio passes through with 5ms latency
- [ ] Threshold parameter works (0% = no clipping, 100% = heavy clipping)
- [ ] Clipping artifacts audible when threshold lowered
- [ ] No pops, clicks, or discontinuities
- [ ] Latency reported correctly to host (verify alignment with other tracks)

**Duration:** 30 min

---

### Phase 4.2: Automatic Gain Matching

**Goal:** Implement peak-based gain compensation

**Components:**
- Input peak detection (from lookahead buffer)
- Output peak detection (from clipped signal)
- Gain calculation (input_peak / output_peak)
- Gain smoothing (juce::SmoothedValue with 50ms time constant)
- Gain application to clipped output

**Implementation details:**
- Peak detection: Iterate buffer, find max absolute value
- Gain calculation with epsilon: `(outputPeak > 0.001f) ? (inputPeak / outputPeak) : 1.0f`
- SmoothedValue setup: `reset(sampleRate, 0.05)` for 50ms smoothing
- Apply smoothed gain to clipped signal

**Test Criteria:**
- [ ] Output peak level matches input peak level (verify with meters)
- [ ] Gain compensation increases as threshold lowers
- [ ] No audible zipper noise or gain pumping
- [ ] Smooth gain transitions on transient material (drums)
- [ ] Edge cases handled: silent input (gain = 1.0), no clipping (gain ≈ 1.0)
- [ ] A/B test: Clipped signal at same peak level as clean input

**Tuning parameters:**
- Smoothing time: Start at 50ms, adjust if gain pumping or sluggishness occurs (range: 20-100ms)
- Epsilon value: 0.001f to prevent division by zero (adjust if needed)

**Duration:** 45 min

---

### Phase 4.3: Clip Solo (Delta Monitoring)

**Goal:** Add difference signal monitoring

**Components:**
- Store original sample before clipping
- Calculate difference signal (original - clipped_with_gain)
- Clip solo boolean parameter
- Conditional output routing (clipped or difference)

**Implementation details:**
- Store original: `float original = inputSample;` before processing
- Calculate difference: `float diff = original - (clipped * gain);`
- Route based on parameter: `output = soloClipped ? diff : (clipped * gain);`

**Test Criteria:**
- [ ] Clip solo toggle switches between normal and difference output
- [ ] Difference signal is silent when no clipping occurs (threshold at 0%)
- [ ] Difference signal contains clipped harmonics when threshold is high
- [ ] Solo mode useful for hearing clipping artifacts
- [ ] Parameter is automatable in DAW
- [ ] No audible pops when toggling solo on/off

**Duration:** 20 min

---

## Stage 5: GUI Phases

### Phase 5.1: Layout and Basic Controls

**Goal:** Integrate mockup HTML and bind basic parameters

**Components:**
- Copy v1-ui.html to Source/ui/public/index.html (when mockup exists)
- Update PluginEditor.h/cpp with WebView setup
- Configure CMakeLists.txt for WebView resources
- Bind clipThreshold parameter via WebSliderRelay
- Bind soloClipped parameter via WebToggleButtonRelay

**Implementation details:**
- WebView setup with resource provider (explicit URL mapping pattern)
- Create relay for threshold: `WebSliderRelay("clipThreshold")`
- Create relay for solo: `WebToggleButtonRelay("soloClipped")`
- Create attachments with nullptr for undoManager (3-parameter pattern)
- Add check_native_interop.js to resources

**Test Criteria:**
- [ ] WebView window opens with correct size
- [ ] Threshold control visible and styled correctly
- [ ] Solo toggle button visible and styled correctly
- [ ] Layout matches mockup design
- [ ] Background and styling render properly

**Duration:** 20 min

---

### Phase 5.2: Parameter Binding and Interaction

**Goal:** Two-way parameter communication (UI ↔ DSP)

**Components:**
- JavaScript → C++ relay calls (threshold slider, solo toggle)
- C++ → JavaScript parameter updates (host automation)
- Value formatting and display
- Real-time parameter updates during playback

**Implementation details:**
- Use ES6 module imports: `import { getSliderState, getToggleState } from './js/juce/index.js'`
- Threshold binding: `const thresholdState = getSliderState("clipThreshold");`
- Solo binding: `const soloState = getToggleState("soloClipped");`
- Implement relative drag for threshold knob (frame-delta pattern, not absolute positioning)
- Add valueChangedEvent listeners (call getNormalisedValue() inside, no parameters passed to callback)

**Test Criteria:**
- [ ] Threshold knob rotation changes DSP clipping amount
- [ ] Solo toggle changes DSP output routing
- [ ] Host automation updates threshold knob position
- [ ] Host automation updates solo toggle state
- [ ] Preset changes update all UI elements
- [ ] Parameter values display correctly (e.g., "50%" for threshold)
- [ ] No lag or visual glitches during parameter changes
- [ ] Knob uses relative drag (incremental), not absolute positioning

**Duration:** 25 min

---

### Phase 5.3: Metering and Visual Feedback

**Goal:** Add input/output meters and clipping indicator

**Components:**
- Input level meter (displays input peak)
- Output level meter (displays output peak, shows gain matching in action)
- Clipping indicator (visual representation of clipping amount)
- Real-time data flow from DSP to UI

**Implementation details:**
- C++ sends meter data via custom event: `webView->emitEventIfBrowserIsVisible("meterUpdate", {inputPeak, outputPeak, clippingAmount})`
- JavaScript receives events and updates meter visuals
- Use requestAnimationFrame loop for smooth meter animation (ballistic motion: fast attack 0.4, slow decay 0.15)
- Clipping indicator shows percentage or visual bar based on threshold

**Test Criteria:**
- [ ] Input meter displays input signal level
- [ ] Output meter displays output signal level (matches input due to gain matching)
- [ ] Meters demonstrate gain matching: Output stays constant as threshold changes
- [ ] Clipping indicator shows amount of clipping occurring
- [ ] Meters animate smoothly (no jitter, ballistic motion feels natural)
- [ ] Performance acceptable (no CPU spikes from meter updates)
- [ ] Meters update at reasonable rate (~30-60 fps, not per-sample)

**Duration:** 30 min

---

## Estimated Duration

Total: ~3 hours

- Stage 2: 5 min (Foundation - project structure)
- Stage 3: 5 min (Shell - APVTS parameters)
- Stage 4: 95 min (DSP - 3 phases)
  - Phase 4.1: 30 min (Core processing)
  - Phase 4.2: 45 min (Gain matching + tuning)
  - Phase 4.3: 20 min (Clip solo)
- Stage 5: 75 min (GUI - 3 phases)
  - Phase 5.1: 20 min (Layout and controls)
  - Phase 5.2: 25 min (Parameter binding)
  - Phase 5.3: 30 min (Metering and visual feedback)
- Stage 6: 20 min (Validation - presets, pluginval, changelog)

**Note:** Estimated 1.5-2.5 hours in architecture.md was for DSP only. GUI adds ~1.25 hours. Total aligns with 3-hour estimate.

---

## Implementation Notes

### Thread Safety

- All parameter reads use atomic `getRawParameterValue()->load()` via APVTS
- Peak detection and gain calculation happen entirely in audio thread (no shared state)
- Lookahead buffer is per-channel (no inter-channel dependencies)
- SmoothedValue uses internal state (no cross-thread access)
- Meter data sent to UI via event emission (non-blocking, async)
- No locks, no allocations in processBlock()

### Performance

- Estimated CPU usage:
  - Hard clipping: <1% (trivial per-sample operation)
  - Peak detection: ~5% (two max-finding loops per buffer)
  - Lookahead buffer: ~2% (circular buffer operations)
  - Gain smoothing: ~1% (exponential smoothing per sample)
  - **Total estimated:** ~10% single core at 48kHz, 512 sample buffer
- Hot paths: Peak detection loops (could optimize with SIMD if needed, but unlikely to be bottleneck)
- Meter updates: Throttled to ~30-60 fps (not per-sample) to avoid UI thread starvation

### Latency

- Fixed latency: 5ms (lookahead buffer)
- Sample count: `getLatencySamples()` returns `static_cast<int>(0.005 * getSampleRate())`
  - 48kHz: 240 samples
  - 96kHz: 480 samples
  - 192kHz: 960 samples
- Host compensation: DAW automatically delays other tracks by reported latency for alignment
- User-facing: Document that 5ms latency is inherent to automatic gain matching design

### Denormal Protection

- Use `juce::ScopedNoDenormals` in processBlock() (standard practice)
- Hard clipping naturally avoids denormals (output bounded by threshold)
- Gain smoothing uses juce::SmoothedValue (handles denormals internally)
- Epsilon in gain calculation (0.001f) prevents denormal divisions

### Known Challenges

1. **Gain matching tuning:**
   - Smoothing time (50ms starting point) may need adjustment
   - Test on drums: fast transients require responsive gain (20-30ms may be better)
   - Test on sustained material: slow changes need smooth gain (70-100ms may be better)
   - Solution: Start at 50ms, tune during Phase 4.2 testing

2. **Peak vs. RMS perception:**
   - Peak matching may not match perceived loudness
   - RMS-based approach is fallback if peak-based doesn't feel natural
   - Test plan: A/B clipped vs clean, verify peaks match but loudness may differ
   - If needed: Switch to RMS calculation (minimal code change)

3. **Latency reporting:**
   - Must implement getLatencySamples() correctly for host compensation
   - Test in multiple DAWs (Logic, Ableton, Reaper) to verify timing alignment
   - Some DAWs may not handle latency reporting perfectly (document if found)

4. **WebView parameter binding:**
   - Follow JUCE 8 patterns: ES6 modules with type="module"
   - Use getToggleState() for boolean solo parameter (not getSliderState())
   - Use 3-parameter WebSliderParameterAttachment (include nullptr for undoManager)
   - Implement relative drag for knob (frame-delta, not absolute positioning)

5. **Meter performance:**
   - Don't send meter data per-sample (too much overhead)
   - Throttle to ~30-60 fps (e.g., send every 512 samples at 48kHz = ~93 fps, acceptable)
   - Use requestAnimationFrame for smooth meter animation in JavaScript
   - Implement ballistic motion (fast attack, slow decay) for natural feel

---

## References

- Creative brief: `plugins/AutoClip/.ideas/creative-brief.md`
- Parameter spec: `plugins/AutoClip/.ideas/parameter-spec-draft.md`
- DSP architecture: `plugins/AutoClip/.ideas/architecture.md`
- UI mockup: `plugins/AutoClip/.ideas/mockups/v[N]-ui.yaml` (when created)

**Similar plugins for reference:**
- GainKnob - WebView parameter binding patterns, knob interaction
- TapeAge - SmoothedValue usage, gain smoothing implementation
- StandardClip - Hard clipping algorithm inspiration (professional reference)

**JUCE 8 critical patterns:**
- Pattern #1: CMakeLists.txt - juce_generate_juce_header() after target_link_libraries
- Pattern #5: Threading - UI ↔ Audio thread (use APVTS for communication)
- Pattern #11: WebView member initialization (std::unique_ptr pattern)
- Pattern #12: WebSliderParameterAttachment (3 parameters including nullptr)
- Pattern #15: WebView valueChangedEvent callback (no parameters passed, call getNormalisedValue() inside)
- Pattern #16: WebView knob interaction (relative drag, frame-delta pattern)
- Pattern #17: juce::dsp API (DelayLine uses prepare(spec) and process(context))
- Pattern #19: WebView Boolean parameters (use getToggleState, not getSliderState)
- Pattern #20: WebView VU meters (requestAnimationFrame loop with ballistic motion)
- Pattern #21: WebView ES6 module loading (type="module" required)
