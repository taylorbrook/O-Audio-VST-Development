# O-Detune - Implementation Plan

**Date:** 2026-02-01
**Complexity Score:** 5.0 (Complex - Maximum)
**Strategy:** Phase-based implementation with incremental feature integration

---

## Complexity Factors

**Breakdown:**

- **Parameters:** 21 parameters (21/5 = 4.2, capped at 2.0) = **2.0 points**
- **Algorithms:** 11 DSP components = **11 points**
  - Delay-Based Pitch Modulation (Wobble)
  - Multi-Voice Unison (2-7 parallel delay lines)
  - LFO System (multi-LFO + noise modulation)
  - Drive/Saturation (waveshaping)
  - Color Filter (biquad)
  - Age/Degradation (noise + drift)
  - Stereo Width Processor (mid-side)
  - Mono-Safe Mode (all-pass/comb filters)
  - Focus Filter (dual biquad)
  - Feedback Loop
  - Dry/Wet Mixer
- **Features:** 2 points
  - Feedback loops (+1)
  - Modulation systems (+1) - LFO with multiple shapes, non-repeating patterns
- **Total:** 2.0 + 11 + 2 = **15.0** (capped at 5.0) = **5.0**

**Complexity tier:** 5 (maximum)

**Complexity drivers:**
- 21 parameters (very high parameter count)
- Dual-engine architecture (Wobble + Unison with crossfade blend)
- Multi-voice processing (up to 7 parallel pitch shifters)
- Modulation system (multi-LFO, noise modulation, non-repeating patterns)
- Mono-safe mode (all-pass/comb filter array - algorithmically complex)
- Feedback loops (signal routing complexity)

---

## Stages

- Stage 0: Research ✓ (Complete)
- Stage 1: Planning ← Next
- Stage 1: Foundation
- Stage 2: Shell
- Stage 3: DSP (3 phases - see below)
- Stage 3: GUI (3 phases - see below)
- Stage 3: Validation

---

## Complex Implementation (Score = 5.0)

### Stage 3: DSP Phases

#### Phase 4.1: Core Processing (Dual Engines + Basic Signal Flow)

**Goal:** Implement wobble and unison engines with basic signal path (no modulation, no character processing)

**Components:**
- Delay-based pitch modulation (wobble engine) with simple sine LFO
- Multi-voice unison (3 voices initially, linear distribution)
- Dual-engine blend control
- Basic dry/wet mixing
- Focus filter (frequency-selective processing)

**Implementation details:**
- Wobble: Single sine LFO modulating delay time (±100 cents range)
- Unison: 3 parallel delay lines with static detune offsets (-15, 0, +15 cents)
- Blend: Linear crossfade between wobble and unison outputs
- Focus: Dual biquad (high-pass + low-pass) on wet signal

**Test Criteria:**
- [ ] Plugin loads in DAW without crashes
- [ ] Wobble engine produces audible pitch modulation (sine LFO)
- [ ] Unison engine produces 3-voice detuning effect
- [ ] Blend parameter smoothly crossfades between engines
- [ ] Focus filter limits processing to selected frequency range
- [ ] Dry/wet mix works correctly (0% = bypass, 100% = fully processed)
- [ ] No clicks, pops, or artifacts during parameter changes
- [ ] CPU usage < 20% with all Phase 4.1 features active

**Git commit:** "feat(O-Detune): Phase 4.1 - core dual-engine processing"

---

#### Phase 4.2: Modulation & Character Processing

**Goal:** Add advanced modulation (multi-LFO, noise), character processing (saturation, color, age), and feedback loop

**Components:**
- Multi-LFO system (primary + secondary LFOs with noise modulation)
- Triangle and random LFO shapes
- Tempo sync
- Drive/saturation (hyperbolic tangent waveshaping)
- Color filter (tone shaping from dark to bright)
- Age/degradation (noise injection + filter drift)
- Feedback loop (recirculation before saturation)

**Implementation details:**
- Multi-LFO: Primary LFO (user rate) + secondary LFO (0.05-0.2 Hz) for non-repeating patterns
- Random shape: Low-pass filtered white noise (bandwidth ~1-5 Hz)
- Saturation: `output = tanh(gain * input)` with gain 1.0-10.0
- Color: Negative = low-pass (2k-20k cutoff), Positive = high-shelf (+0 to +6dB @ 5kHz)
- Age: Bandpass-filtered noise (2k-8k) + subtle filter drift (±5% cutoff modulation)
- Feedback: Tap after blend mixer, before saturation (creates harmonic buildup)

**Test Criteria:**
- [ ] Multi-LFO produces non-repeating wobble patterns
- [ ] Triangle LFO shape works correctly
- [ ] Random LFO shape creates authentic tape-like variation
- [ ] Tempo sync locks wobble rate to DAW BPM
- [ ] Saturation adds warmth and harmonics (audible at 50%+ drive)
- [ ] Color filter shapes tone correctly (dark = muffled, bright = airy)
- [ ] Age parameter adds hiss and instability
- [ ] Feedback loop creates buildup effect without runaway (safe at 80%)
- [ ] No crashes or artifacts with all modulation/character features active
- [ ] CPU usage < 35% with Phase 4.1 + 4.2 features

**Git commit:** "feat(O-Detune): Phase 4.2 - modulation system and character processing"

---

#### Phase 4.3: Advanced Features (Unison Expansion + Stereo + Mono-Safe)

**Goal:** Expand unison to 2/4/5/7 voices, add exponential/random distribution, implement stereo width and mono-safe mode

**Components:**
- Unison voice expansion (2, 4, 5, 7 voices)
- Exponential and random distribution modes
- Per-voice stereo panning (unison_spread parameter)
- Stereo width processing (mid-side, 0-200%)
- Mono-safe mode (all-pass/comb filter array)
- Era-specific frequency response (60s/70s/80s presets)

**Implementation details:**
- Voice expansion: Instantiate N delay lines based on unison_voices parameter
- Distribution modes:
  - Exponential: `detune[i] = sign(i) * (totalDetune/2) * ((i / maxI)^2)`
  - Random: `detune[i] = random(-totalDetune/2, +totalDetune/2)` per-voice
- Panning: `pan[i] = -spread/2 + (i / (voices-1)) * spread`
- Stereo width: Mid-side processing with Side gain (0-200%)
- Mono-safe: All-pass cascade (4 stages) + comb filters (complementary L/R delays)
- Era presets: Biquad coefficient sets for 60s/70s/80s frequency response curves

**Test Criteria:**
- [ ] Unison supports 2/3/4/5/7 voices correctly
- [ ] Exponential distribution creates more voices near center
- [ ] Random distribution varies per-voice detune within range
- [ ] Stereo panning spreads voices across stereo field
- [ ] Width parameter controls stereo spread (0% = mono, 200% = extra-wide)
- [ ] Mono-safe mode cancels perfectly when summed to mono (L + R = 0 verification)
- [ ] Era presets apply correct frequency response (60s = Ampex, 70s = Teac, 80s = Cassette)
- [ ] No artifacts when switching voice counts or distribution modes
- [ ] CPU usage acceptable with 7 voices + all features (<50% single core @ 48kHz)

**Git commit:** "feat(O-Detune): Phase 4.3 - unison expansion, stereo processing, mono-safe mode"

---

### Stage 3: GUI Phases

#### Phase 5.1: Layout and Basic Controls

**Goal:** Integrate WebView mockup and bind primary dual-engine controls

**Components:**
- Copy finalized mockup HTML to Source/ui/public/index.html
- Configure CMakeLists.txt for WebView resources (juce_add_binary_data)
- Update PluginEditor.h/cpp with WebView setup (check_native_interop.js required)
- Bind primary parameters:
  - Mode blend (wobble ↔ unison)
  - Wobble: era, rate, depth, shape
  - Unison: voices, detune, distribution, spread
  - Mix (dry/wet)

**Test Criteria:**
- [ ] WebView window opens with correct size (match mockup dimensions)
- [ ] Dual-engine layout visible (wobble left, unison right, blend center)
- [ ] All basic controls render correctly (knobs, sliders, dropdowns)
- [ ] Background, colors, and styling match mockup aesthetic
- [ ] Layout is responsive and properly scaled

**Git commit:** "feat(O-Detune): Phase 5.1 - WebView layout and basic controls"

---

#### Phase 5.2: Parameter Binding and Interaction

**Goal:** Two-way parameter communication (UI ↔ DSP) for all controls

**Components:**
- JavaScript → C++ relay calls (user control changes)
- C++ → JavaScript parameter updates (host automation, preset changes)
- Value formatting and display (Hz, cents, %, era names)
- Real-time parameter updates during playback
- Bind remaining parameters:
  - Drive, color, age (character section)
  - Width, focus (output section)
  - Mono-safe toggle
  - Advanced: delay, feedback, randomization

**Test Criteria:**
- [ ] All control movements update DSP parameters correctly
- [ ] Host automation updates UI controls in real-time
- [ ] Preset changes update all UI elements instantly
- [ ] Parameter values display with correct units (Hz, cents, %, dB)
- [ ] No lag or visual glitches during parameter sweeps
- [ ] Boolean parameters (mono-safe, tempo sync) toggle correctly
- [ ] Choice parameters (era, shape, voices, distribution) show correct labels

**Git commit:** "feat(O-Detune): Phase 5.2 - complete parameter binding"

---

#### Phase 5.3: Advanced UI Elements (Optional - if mockup includes)

**Goal:** Implement advanced UI features (visualizations, animations, mode indicators)

**Components:**
- Wobble visualization (animated pitch deviation graph)
- Unison voice spread indicator (visual representation of voice distribution)
- Mono-safe indicator (green = safe, yellow = potential issues)
- Era preset visual feedback (60s/70s/80s branding)
- VU meters or level displays (if included in mockup)

**Implementation details:**
- Wobble visualization: Canvas-based waveform displaying LFO shape in real-time
- Unison indicator: Visual dots/bars representing voice positions across detune range
- Mono-safe: Color-coded indicator (green when enabled and working)
- Era visuals: SVG icons or text labels matching era aesthetic
- VU meters: requestAnimationFrame loop with ballistic motion (fast attack, slow decay)

**Test Criteria:**
- [ ] Wobble visualization animates smoothly at 60fps
- [ ] Unison indicator updates when voices/detune/distribution changes
- [ ] Mono-safe indicator shows correct state (green when enabled)
- [ ] Era visuals change when era preset is selected
- [ ] VU meters (if present) respond to audio level with ballistic motion
- [ ] Performance acceptable (no CPU spikes, smooth animations)
- [ ] Visual polish matches mockup quality

**Git commit:** "feat(O-Detune): Phase 5.3 - advanced UI visualizations"

**Note:** This phase is OPTIONAL and depends on mockup finalization. Skip if mockup is minimal (controls only, no visualizations).

---

### Implementation Flow

**Complete workflow:**

1. **Stage 0:** Research ✓ (Complete - ARCHITECTURE.md created)
2. **Stage 1:** Planning (create execution plan with task breakdown)
3. **Stage 1:** Foundation (CMakeLists.txt, project structure, build system)
4. **Stage 2:** Shell (APVTS parameters, PluginProcessor skeleton)
5. **Stage 3: DSP** - 3 phases
   - **Phase 4.1:** Core dual-engine processing (wobble + unison + blend + focus)
   - **Phase 4.2:** Modulation system and character processing (multi-LFO + saturation + color + age + feedback)
   - **Phase 4.3:** Advanced features (unison expansion + stereo + mono-safe + era presets)
6. **Stage 3: GUI** - 3 phases (or 2 if Phase 5.3 skipped)
   - **Phase 5.1:** WebView layout and basic controls
   - **Phase 5.2:** Complete parameter binding
   - **Phase 5.3:** Advanced UI visualizations (OPTIONAL)
7. **Stage 3:** Validation (factory presets, pluginval, CHANGELOG.md)

**Total phases:** 3 (DSP) + 2-3 (GUI) + validation = **6-7 phases**

---

## Implementation Notes

### Thread Safety

- All parameter reads use `APVTS::getRawParameterValue()->load()` (atomic)
- LFO phase state is per-channel (no shared state between stereo L/R)
- Feedback buffer: Audio thread only (no cross-thread access)
- Era preset coefficients: Atomic pointer swap (message thread computes, audio thread reads)
- No mutex/locks in audio thread (real-time safe)

### Performance

**Estimated CPU usage per component:**
- Wobble engine: ~5% (1 delay line + LFO)
- Unison engine: ~5% per voice × voice count
  - 3 voices: ~15%
  - 7 voices: ~35% (worst case)
- Saturation: <1% (simple tanh)
- Filters (color + focus + era): ~2% per biquad × 4 filters = ~8%
- Feedback loop: <1% (buffer mixing)
- Total estimated (7 voices + all features): **~50% single core @ 48kHz**

**Optimization opportunities:**
- Skip processing inactive engine when blend = 0% or 100% (conditional)
- SIMD for parallel voice processing (juce::dsp supports SIMD)
- Reduce unison voice count in low-CPU mode (user setting)

### Latency

- **Base delay:** 50ms center delay (2400 samples @ 48kHz)
- **Unison delay:** 50ms per voice (same as wobble)
- **Total latency:** 50ms reported via `getLatencySamples()` for host compensation
- **DryWetMixer:** Automatic latency compensation for dry signal alignment

### Denormal Protection

- Use `juce::ScopedNoDenormals` in processBlock()
- All JUCE DSP components handle denormals internally (DelayLine, Oscillator, IIR::Filter)
- LFO uses phase wrapping to avoid denormals
- Delay lines flush to zero on silence (JUCE built-in)

### Known Challenges

**1. Mono-safe mode (all-pass/comb filters):**
- **Challenge:** Algorithm not publicly documented, requires reverse-engineering Polyverse Wider
- **Solution:** Research open-source implementations, test phase coherence with unit tests
- **Fallback:** Use mid-side processing with careful phase management if all-pass proves too complex
- **Reference:** Polyverse Wider documentation, DAFX book on phase filters

**2. Multi-LFO non-repeating patterns:**
- **Challenge:** Simple LFO sounds artificial, need noise modulation for authenticity
- **Solution:** Low-pass filtered white noise (1-5 Hz bandwidth) mixed with primary LFO
- **Fallback:** Use single noise-modulated LFO if dual-LFO CPU cost too high
- **Reference:** Goodhertz Wow Control (multi-LFO architecture)

**3. Unison voice distribution edge cases:**
- **Challenge:** 2 voices vs 7 voices have different distribution formulas
- **Solution:** Test all voice counts (2, 3, 4, 5, 7), ensure central voice always at 0 cents
- **Reference:** Supersaw algorithm (linear distribution), chorus plugins (random distribution)

**4. Feedback loop stability:**
- **Challenge:** High feedback + high drive can cause runaway saturation
- **Solution:** Scale feedback gain by (1 - feedback) to prevent runaway, limit feedback to 80% max
- **Reference:** Delay plugins with feedback (Valhalla Delay, EchoBoy)

**5. Era preset frequency response:**
- **Challenge:** Authentic tape curves not standardized, requires research
- **Solution:** Research Ampex/Teac/Cassette frequency response graphs, approximate with biquad shelves
- **Fallback:** Use generic rolloff curves if authentic curves unavailable
- **Reference:** Tape machine specifications, analog tape modeling literature

### Phase Dependencies

**Critical ordering:**
- **Phase 4.1 MUST complete before 4.2:** Feedback loop depends on working dual-engine architecture
- **Phase 4.2 MUST complete before 4.3:** Mono-safe mode integrates with existing signal flow
- **Phase 5.1 MUST complete before 5.2:** Parameter binding requires working WebView layout
- **Phase 5.2 MUST complete before 5.3:** Visualizations depend on parameter binding for data

**Independent phases (could parallelize if multiple developers):**
- Phase 4.3 (stereo processing) is independent from Phase 5.1-5.2 (GUI layout/binding)
- Phase 5.3 (visualizations) is independent from DSP phases

### Verification Strategy

**Per-phase verification:**
- Each phase ends with git commit only after ALL test criteria met
- Test in DAW (Logic Pro or Ableton) after each phase commit
- Run pluginval after Phase 4.3 (DSP complete) and Phase 5.2 (GUI complete)
- Create 1-2 test presets per phase to validate new features

**Final verification (Stage 3: Validation):**
- Create factory presets covering all dual-engine modes (wobble only, unison only, blend midpoints)
- Run full pluginval suite (strict mode)
- Test mono-safe mode verification (sum to mono and verify cancellation)
- Generate CHANGELOG.md with all implemented features

---

## References

**Contract files:**
- Creative brief: `plugins/O-Detune/.planning/BRIEF.md`
- Parameter spec: `plugins/O-Detune/.planning/parameter-spec-draft.md`
- DSP architecture: `plugins/O-Detune/.planning/research/ARCHITECTURE.md`
- UI mockup: `plugins/O-Detune/.planning/mockups/v[N]-ui.yaml` (to be created)

**Similar plugins for reference:**
- **GainKnob:** WebView parameter binding patterns, simple UI integration
- **TapeAge:** Multi-parameter character processing, era presets
- **FlutterVerb:** Modulation LFO implementation, VU meters, WebView visualizations
- **LushPad:** Multi-voice architecture (oscillators vs delay lines, but similar parallel processing)

**Professional plugin research:**
- Goodhertz Wow Control - Multi-LFO tape wow/flutter
- XLN RC-20 Retro Color - Dual-LFO wobble module
- Soundtoys MicroShift - Multi-voice unison detuning
- Polyverse Wider - Mono-safe stereo widening (all-pass/comb filters)
- Valhalla Delay - Delay-based pitch shifting, feedback loops

**JUCE critical patterns:**
- Pattern #3: DelayLine interpolation (Lagrange3rd for pitch modulation)
- Pattern #17: juce::dsp API (prepare() with ProcessSpec)
- Pattern #21: WebView ES6 module loading (type="module" required)
- Pattern #12: WebSliderParameterAttachment (3 parameters in JUCE 8)

---

*Generated: 2026-02-01*
*Complexity: 5.0 (Maximum)*
*Strategy: Phased implementation with 3 DSP phases + 2-3 GUI phases*
*Total phases: 6-7 phases*
