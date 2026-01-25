# Ouaricon Saturation Modeling - Implementation Plan

**Date:** 2026-01-09
**Complexity Score:** 3.8 (Complex)
**Strategy:** Phase-based implementation

---

## Complexity Factors

- **Parameters:** 4 parameters (4/5 = 0.8 points) = 0.8
- **Algorithms:** 7 DSP components = 7
  - Jiles-Atherton Hysteresis (MAGNETIC)
  - Koren Triode Model (TUBE)
  - Transformer Core Saturation (TRANSFORMER)
  - Newton-Raphson Diode Clipper (DIODE)
  - Oversampling System
  - Auto-Gain RMS Follower
  - Model-Specific Frequency Response Filters (3 sets)
- **Features:** -4 points (no complex features that add to base complexity)
  - No feedback loops beyond model-internal state
  - No FFT/frequency domain processing
  - No multiband processing
  - No modulation systems
  - No external MIDI control
- **Total:** 0.8 + 7 - 4 = 3.8 (capped at 5.0)

**Rationale for 3.8 score:**
- 4 physically-modeled saturation algorithms with iterative solvers (HIGH complexity)
- Oversampling system with quality switching (MEDIUM complexity)
- Auto-gain system with RMS detection (LOW complexity)
- But: Simple parameter count (only 4), no advanced DSP features
- Classification: Complex (≥ 3.0) → Requires phase-based implementation

---

## Stages

- Stage 0: Research ✓
- Stage 1: Planning ← Next
- Stage 1: Foundation
- Stage 2: Shell
- Stage 3: DSP (4 phases)
- Stage 3: GUI (1 phase, simple controls)
- Stage 3: Validation

---

## Complex Implementation (Score ≥ 3.0)

### Stage 3: DSP Phases

#### Phase 4.1: Foundation - Oversampling + DIODE Model

**Goal:** Implement simplest saturation model (DIODE) with oversampling system

**Components:**
- Oversampling system (`juce::dsp::Oversampling`) with 3 quality modes
- DIODE model (Newton-Raphson solver for Shockley equation)
- Model routing switch (prepare for 4 models)
- Basic signal flow (input → upsample → saturation → downsample → output)

**Test Criteria:**
- [ ] Plugin loads in DAW without crashes
- [ ] Audio passes through (dry signal audible when INTENSITY=0)
- [ ] QUALITY parameter switches oversampling (0=none, 1=2x, 2=4x)
- [ ] Latency reported correctly per quality mode
- [ ] DIODE saturation audible and smooth (no clicks or artifacts)
- [ ] INTENSITY parameter controls saturation amount (0-100%)
- [ ] Sine sweep shows expected harmonic generation
- [ ] No aliasing artifacts in HIGH quality mode

**Duration estimate:** 2-3 days

---

#### Phase 4.2: TRANSFORMER Model

**Goal:** Add TRANSFORMER model with core saturation and frequency response

**Components:**
- Transformer core saturation (tanh-based soft saturation)
- LF bump filter (60Hz peak, Q=0.7, +2.0dB)
- HF sheen filter (8kHz shelf, +1.0dB)
- MODEL parameter routing (DIODE=3, TRANSFORMER=2)

**Test Criteria:**
- [ ] MODEL parameter switches between DIODE and TRANSFORMER
- [ ] TRANSFORMER produces low-frequency thickening
- [ ] LF bump audible at 60Hz (spectrum analyzer verification)
- [ ] HF sheen audible at 8kHz (subtle brightness)
- [ ] INTENSITY scales saturation smoothly
- [ ] Model switching doesn't cause clicks or artifacts
- [ ] State reset on model change prevents residual artifacts

**Duration estimate:** 1-2 days

---

#### Phase 4.3: TUBE Model

**Goal:** Add TUBE model with Koren triode equations

**Components:**
- Koren triode model (Newton-Raphson solver for plate current)
- Presence filter (3kHz peak, Q=0.7, +1.5dB)
- Grid bias and plate voltage mapping from INTENSITY
- Iteration count tied to QUALITY (4/6/8 iterations)

**Test Criteria:**
- [ ] TUBE model produces asymmetric clipping (triode characteristic)
- [ ] Presence boost audible at 3kHz (spectrum analyzer verification)
- [ ] INTENSITY parameter controls grid bias smoothly
- [ ] Newton-Raphson converges reliably (no divergence or NaNs)
- [ ] QUALITY parameter affects iteration count (verify CPU difference)
- [ ] Tube warmth and harmonic character distinct from DIODE/TRANSFORMER

**Duration estimate:** 2-3 days

---

#### Phase 4.4: MAGNETIC Model + Auto-Gain

**Goal:** Add highest-complexity MAGNETIC model and finalize auto-gain system

**Components:**
- Jiles-Atherton hysteresis model (Langevin function, differential equations)
- Head bump filter (80Hz peak, Q=0.7, +2.5dB)
- HF rolloff filter (12kHz lowpass, Q=0.707)
- Auto-gain system (RMS envelope follower, level matching)
- Magnetization state management (M, H_prev)

**Test Criteria:**
- [ ] MAGNETIC model produces hysteresis effect (memory/history)
- [ ] Sine sweep shows asymmetric saturation curve
- [ ] Head bump audible at 80Hz (tape character)
- [ ] HF rolloff audible above 12kHz (tape head loss)
- [ ] Langevin function handles singularities (no NaN or division by zero)
- [ ] State management stable (no runaway magnetization)
- [ ] Auto-gain maintains perceived loudness across INTENSITY range
- [ ] AUTOGAIN toggle works correctly (on/off comparison)
- [ ] All 4 models functional and distinct sonic character

**Duration estimate:** 3-4 days

---

### Stage 3: GUI Phase

#### Phase 5.1: UI Integration (Simple Controls)

**Goal:** Integrate WebView UI with parameter binding (or native JUCE GUI if WebView not used)

**Components:**
- INTENSITY knob (large, central)
- MODEL buttons (4 buttons: MAGNETIC, TUBE, TRANSFORMER, DIODE)
- QUALITY buttons (3 buttons: LOW, MID, HIGH)
- AUTO button (toggle for auto-gain)
- Parameter binding via WebView relay system or JUCE native attachments
- Ouaricon botanical aesthetic (consistent with Tremolo)

**Test Criteria:**
- [ ] UI loads and displays correctly
- [ ] INTENSITY knob updates DSP parameter smoothly
- [ ] MODEL buttons switch saturation algorithm
- [ ] QUALITY buttons change oversampling mode
- [ ] AUTO button toggles auto-gain on/off
- [ ] Host automation updates UI controls
- [ ] Preset changes update all UI elements
- [ ] Visual feedback matches Ouaricon aesthetic
- [ ] No lag or visual glitches during parameter changes

**Duration estimate:** 2-3 days

**Note:** UI is simple (4 parameters, no advanced visualizations), so single phase sufficient

---

### Implementation Flow

- Stage 1: Foundation - project structure (CMakeLists.txt, folders, JUCE setup)
- Stage 2: Shell - APVTS parameters (4 parameters: INTENSITY, MODEL, QUALITY, AUTOGAIN)
- Stage 3: DSP - 4 phases
  - Phase 4.1: Oversampling + DIODE (foundation, 2-3 days)
  - Phase 4.2: TRANSFORMER (1-2 days)
  - Phase 4.3: TUBE (2-3 days)
  - Phase 4.4: MAGNETIC + Auto-Gain (3-4 days)
- Stage 3: GUI - 1 phase
  - Phase 5.1: UI Integration (2-3 days)
- Stage 3: Validation - presets, pluginval, changelog (1-2 days)

**Total estimated duration:** 13-18 days (2.5-3.5 weeks)

---

## Implementation Notes

### Thread Safety
- All parameter reads use atomic `getRawParameterValue()->load()`
- No shared state between models (each model has own state variables)
- Model switching resets state in audio thread (no allocations, just zero initialization)
- Oversampling buffer managed by JUCE (thread-safe)
- RMS buffers are per-channel (no cross-channel shared state)
- State variables (M, H_prev, envelopes) are local to audio thread
- No locks in audio thread (all communication via atomics)

### Performance

**Target CPU budgets (48kHz, single instance):**
- **LOW quality (no oversampling):**
  - DIODE: ~1% (4 iterations Newton-Raphson)
  - TRANSFORMER: ~1.5% (tanh + 3 biquad filters)
  - TUBE: ~2% (4 iterations Newton-Raphson + 1 filter)
  - MAGNETIC: ~3% (Langevin + 2 filters)
- **MID quality (2x oversampling):**
  - 2x CPU multiplier (2-6% range)
- **HIGH quality (4x oversampling):**
  - 4x CPU multiplier (4-12% range)
- **Auto-gain:** ~0.5% (RMS + envelope follower)

**Optimization strategies:**
- Use previous sample as Newton-Raphson initial guess (warm start → faster convergence)
- Clamp exponential functions to prevent overflow (exp(x) → exp(clamp(x, -30, 30)))
- Pre-calculate constants outside sample loop (e.g., RMS window size, time constants)
- Update filter coefficients only on parameter change (not per-sample)
- Use `juce::ScopedNoDenormals` for denormal protection

**Hot paths:**
1. Newton-Raphson iteration loops (TUBE, DIODE) - Profile iteration count vs. quality
2. Langevin function evaluation (MAGNETIC) - Consider lookup table if too slow
3. Oversampling up/down (2x-4x processing load)
4. RMS calculation (moving average per sample)

### Latency

**Per quality mode (at 48kHz):**
- **LOW:** 0 samples (no oversampling)
- **MID:** ~10-20 samples (2x oversampling filter group delay, ~0.2-0.4ms)
- **HIGH:** ~20-40 samples (4x oversampling filter group delay, ~0.4-0.8ms)

**Implementation:**
- Query latency: `oversampler.getLatencyInSamples()`
- Report to host: `setLatencySamples(oversampler.getLatencyInSamples())`
- Update on quality change (QUALITY parameter listener)
- Host compensates via plugin delay compensation (PDC)

### Denormal Protection

**Strategy:**
- `juce::ScopedNoDenormals` in `processBlock()` (CPU flush-to-zero mode)
- Threshold state variables:
  - Magnetization M: if `|M| < 1e-8`, set to 0.0
  - RMS buffers: if `rms < 1e-8`, set to 0.0
  - Envelope followers: if `envelope < 1e-8`, set to 0.0
- Clamp exponentials: `exp(clamp(x, -30, 30))` prevents overflow/underflow
- JUCE DSP components (IIR filters, Oversampling) handle denormals internally

### Known Challenges

**1. Jiles-Atherton numerical stability (MAGNETIC):**
- **Challenge:** Langevin function has singularity at x=0, differential equations can diverge
- **Solution:** Use Taylor series approximation for |x| < 1e-6, clamp M to [-Ms, Ms]
- **Reference:** circuit-modeling-fundamentals.md Section 5.1 (lines 870-879)

**2. Newton-Raphson convergence (TUBE, DIODE):**
- **Challenge:** Poor initial guess can cause divergence or slow convergence
- **Solution:** Use previous sample value as initial guess (warm start), limit iterations to 4-8
- **Reference:** circuit-modeling-fundamentals.md Section 3.3 (lines 419-454)

**3. Oversampling latency reporting:**
- **Challenge:** Latency changes with QUALITY parameter (0/10-20/20-40 samples)
- **Solution:** Update `setLatencySamples()` in parameter change callback, reset oversampler on switch
- **Reference:** physical-modelling-synthesis-complete-guide.md Section 2.1.3 (lines 356-382)

**4. Auto-gain pumping artifacts:**
- **Challenge:** 100ms time constant may pump on percussive material (drums, transients)
- **Solution:** Balance responsiveness vs. smoothness (100ms standard), add gain limiting (0.1-10x)
- **Fallback:** If pumping unacceptable, reduce time constant to 50ms or switch to peak-based gain

**5. Model switching clicks:**
- **Challenge:** Switching MODEL parameter may cause audio discontinuity (state reset)
- **Solution:** Reset state (M=0, H_prev=0, filter states=0) on model change, crossfade if needed
- **Note:** Brief glitch acceptable (documented in manual, users typically don't switch models mid-playback)

**6. Quality switching glitches:**
- **Challenge:** Changing QUALITY during playback resets oversampler (brief dropout)
- **Solution:** Reset oversampler in `prepareToPlay()` or parameter callback, document behavior
- **Note:** Most users set quality once (session start), not during playback

---

## References

**Contract files:**
- Creative brief: `plugins/OuariconSaturationModeling/.ideas/creative-brief.md`
- Parameter spec: `plugins/OuariconSaturationModeling/.ideas/parameter-spec-draft.md`
- DSP architecture: `plugins/OuariconSaturationModeling/.ideas/architecture.md`

**Technical documentation:**
- `research/circuit-modeling-fundamentals.md` - All 4 saturation model implementations
  - Section 4.1: Diode clipping (Newton-Raphson)
  - Section 4.3: Tube saturation (Koren triode)
  - Section 5.1: Tape saturation (Jiles-Atherton)
  - Section 6.3: Transformer modeling
- `troubleshooting/dsp-issues/physical-modelling-synthesis-complete-guide.md` - JUCE DSP patterns
  - Section 2.1: Essential JUCE classes (Oversampling, IIR filters)
  - Section 3: Performance optimization
- `troubleshooting/patterns/juce8-critical-patterns.md` - JUCE 8 requirements

**Similar plugins for reference:**
- OuariconTremolo - Ouaricon aesthetic, WebView UI patterns
- GainKnob - Parameter binding, simple controls
- TapeAge - Saturation + filtering architecture

**Professional reference products:**
- UAD Neve 1073 - Transformer saturation + frequency response
- Arturia 1973-Pre - Triode tube modeling
- Soundtoys Decapitator - Multiple saturation models with single Drive knob
- Applied Acoustics Objeq - Physical modeling architecture

---

## Phase-Specific Notes

### Phase 4.1 (Oversampling + DIODE)
**Focus:** Establish foundation and verify oversampling system
- Start with DIODE (simplest model) to validate architecture
- Test oversampling latency reporting early (DAW compatibility)
- Verify quality switching doesn't crash or cause severe glitches
- Profile CPU per quality mode (verify 1% → 2% → 4% scaling)

### Phase 4.2 (TRANSFORMER)
**Focus:** Add frequency response filters and validate model routing
- Verify biquad filter design (peak filters, shelf filters)
- Test model switching between DIODE and TRANSFORMER (no clicks)
- Compare frequency response to professional transformer emulations
- Ensure state reset on model change prevents artifacts

### Phase 4.3 (TUBE)
**Focus:** Validate Newton-Raphson convergence for complex model
- Monitor convergence behavior (log iteration count, check for NaN/inf)
- Test extreme INTENSITY values (0%, 100%) for stability
- Profile CPU with different iteration counts (4/6/8)
- Compare harmonic structure to reference tube emulations (UAD, Arturia)

### Phase 4.4 (MAGNETIC + Auto-Gain)
**Focus:** High-risk features with fallback readiness
- Implement Jiles-Atherton carefully (follow reference implementation exactly)
- Add extensive logging for numerical stability debugging
- Test auto-gain with various source material (drums, vocals, mix bus)
- Have simplified tanh-based hysteresis fallback ready if J-A fails
- Verify all 4 models produce distinct sonic character (A/B testing)

### Phase 5.1 (UI Integration)
**Focus:** Clean, simple parameter binding
- If WebView: Follow OuariconTremolo WebView patterns
- If native JUCE: Use standard slider/button attachments
- Verify botanical aesthetic consistency (colors, fonts, layout)
- Test host automation (knob moves when DAW automates parameters)
- Test preset switching (all controls update correctly)

---

## Success Criteria

**Stage 3 (DSP) complete when:**
- All 4 saturation models implemented and functional
- Model switching works smoothly (no crashes, minimal artifacts)
- Quality tiers meet CPU targets (1% / 2-3% / 5-8%)
- Auto-gain maintains perceived loudness across INTENSITY range
- No numerical instability (NaN, inf, runaway values)
- Latency reported correctly per quality mode
- All parameters smooth (no zipper noise or clicks)

**Stage 3 (GUI) complete when:**
- All 4 parameters bound to UI controls
- Host automation updates UI (two-way communication)
- Preset switching updates all controls
- Visual design matches Ouaricon aesthetic
- No UI lag or visual glitches

**Plugin release-ready when:**
- Passes pluginval (VST3, AU formats)
- No crashes in major DAWs (Logic Pro, Ableton, Reaper)
- Sonic quality matches professional analog emulations
- Factory presets demonstrate all 4 models
- CPU usage within targets (< 10% single core in HIGH mode)
- User manual documents model characteristics and quality modes
- Changelog reflects v1.0.0 release
