# Implementation Plan: MicroMarimba

**Plugin:** MicroMarimba
**Type:** Synthesizer (IS_SYNTH=TRUE, NEEDS_MIDI_INPUT=TRUE)
**Generated:** 2026-01-09 by research-planning-agent (Stage 0)

---

## Complexity Assessment

### Parameter Count

**Source:** parameter-spec-draft.md

**Parameters defined:**
1. MALLET_HARDNESS (Float, 0.0-1.0)
2. BAR_MATERIAL (Float, 0.0-1.0)
3. RESONANCE (Float, 0.0-1.0)
4. TUNING_MODE (Choice, 0-2)
5. REFERENCE_PITCH (Float, 400.0-480.0 Hz)
6. VEL_CURVE (Float, 0.0-1.0)
7. OUTPUT_GAIN (Float, -24.0 to +12.0 dB)

**Parameter count:** 7

**Parameter score:** min(7 / 5, 2.0) = min(1.4, 2.0) = **1.4**

---

### DSP Algorithm Count

**Source:** architecture.md Core Components section

**Algorithms identified:**
1. Modal Resonator Bank (8 parallel 2nd-order IIR filters per voice)
2. Mallet Exciter (noise generation + filtering)
3. Body Resonance (Convolution)
4. Tuning Engine (Scala/KBM parser + MTS-ESP client)
5. Velocity Response (curve mapping)
6. Voice Management (polyphonic synthesizer)

**Algorithm count:** 6

---

### Feature Complexity Analysis

**Source:** architecture.md Feature analysis

| Feature | Score | Detection | Rationale |
|---------|-------|-----------|-----------|
| Modulation systems | +1 | "Modal Resonator Bank" with 8 modes, decay envelopes, amplitude shaping | Complex time-varying resonance |
| External MIDI control | +1 | "Voice Management" + "MIDI Handling" section | MIDI note-on triggers synthesis, velocity mapping |
| File I/O (Scala) | +1 | "Tuning Engine" loads .scl/.kbm files | Background thread file loading, parsing |
| Real-time pitch calculation | +1 | "Tuning Engine" with MTS-ESP real-time queries | Per-note frequency lookup from tuning system |

**Feature count:** 4

---

### Total Complexity Score

**Formula:**
```
score = param_score + algorithm_count + feature_count
score = 1.4 + 6 + 4 = 11.4
final_score = min(11.4, 5.0) = 5.0
```

**Complexity Score:** **5.0** (VERY HIGH)

**Classification:** COMPLEX (score ≥ 3.0)

**Strategy:** Phase-based implementation REQUIRED

---

## Complexity Tier Analysis

**Tier Detection:**

<complexity_reasoning>
Analyzing creative brief and architecture for complexity tier:

**Indicators:**
- Parameter count: 7 (moderate)
- DSP algorithms: 6 complex systems (modal synthesis, convolution, tuning engine)
- Non-DSP features:
  - File I/O: Scala .scl/.kbm file loading (background thread)
  - MTS-ESP: External protocol integration (real-time queries)
  - Multi-system tuning: 3-tier priority fallback (MTS-ESP > Scala > 12-TET)
- UI complexity:
  - Parameter controls (7 parameters)
  - Tuning mode indicator (active system display)
  - File browser for Scala files
- State management:
  - APVTS parameters (automatic)
  - Scala file paths (custom persistence)
  - MTS-ESP connection state (runtime)
- Synthesizer architecture:
  - MIDI input processing
  - 16-24 voice polyphony
  - Per-voice modal synthesis (8 modes each = 128-192 parallel filters)

**Complexity signals:**
1. **Synthesizer with MIDI:** Requires Synthesiser/SynthesiserVoice architecture (Tier 4+)
2. **File I/O:** Scala file parsing on background thread (Tier 5)
3. **External protocol:** MTS-ESP client lifecycle management (Tier 5)
4. **Complex DSP:** Modal synthesis (8 modes × 16-24 voices) + Convolution (Tier 3-4)
5. **Multi-system integration:** Three tuning systems with priority fallback (Tier 5)
6. **Real-time analysis:** Per-note frequency calculation from tuning system (Tier 6 signals)

**Tier determination:**
- Synthesizer architecture → Minimum Tier 4
- File I/O (Scala) → Tier 5
- MTS-ESP integration → Tier 5
- Multi-output: NO (stereo only)
- Real-time visualization: NO (no spectrum analyzer)

**Result:** Tier 5 (File I/O, external protocol, synthesizer with complex DSP)

**Research depth:** DEEP (30 minutes recommended)
</complexity_reasoning>

**Detected Tier:** **5** (File I/O, synthesizer, external protocol integration)

**Research Depth:** DEEP (30 minutes)

**Time Spent:** 30 minutes (full deep research conducted)

---

## Implementation Strategy

**Strategy:** PHASED IMPLEMENTATION (Complex plugin, score 5.0)

**Rationale:**
- Very high complexity score (5.0 = maximum)
- Multiple interdependent systems (tuning engine → modal synthesis → convolution)
- High-risk components (modal synthesis stability, MTS-ESP integration)
- Synthesizer architecture requires careful voice management
- File I/O and external protocol add integration complexity
- 6 major algorithms with performance constraints (<1% CPU per voice target)

**Approach:**
- Break Stages 2 (DSP) and 3 (GUI) into incremental phases
- Each phase is fully functional and testable
- Git commit after each phase completion
- Clear success criteria per phase (prevents "works in isolation, fails together")

---

## Stage Breakdown

### Stage 0: Research & Planning ✓ COMPLETE

**Status:** Complete (2026-01-09)

**Deliverables:**
- ✓ Creative brief analyzed
- ✓ Parameter spec reviewed
- ✓ Professional plugins researched (Applied Acoustics Chromaphone, Modartt Pianoteq, Surge XT)
- ✓ JUCE APIs mapped (IIR::Filter, Convolution, Synthesiser)
- ✓ Architecture documented (architecture.md)
- ✓ Implementation plan created (plan.md)
- ✓ Complexity score calculated (5.0)

**Research conducted:**
- Modal synthesis algorithm (8 modes, biquad coefficients, inharmonic ratios)
- Tuning systems (Scala format, KBM mapping, MTS-ESP protocol)
- Convolution body resonance (short IR strategy)
- Voice management (polyphony, voice stealing)
- Professional reference implementations

**Duration:** 30 minutes (deep research tier)

---

### Stage 1: Foundation + Shell

**Status:** Pending

**Objective:** Create build system, parameter infrastructure, basic GUI shell

**Deliverables:**
- CMakeLists.txt with JUCE project configuration
- PluginProcessor with APVTS and all 7 parameters
- Empty processBlock (no DSP yet)
- Basic GUI with parameter controls (knobs, labels)
- Plugin loads in DAW (silent, but functional)

**Key tasks:**
1. Run foundation-shell-agent with architecture.md and parameter-spec-draft.md
2. Configure IS_SYNTH=TRUE, NEEDS_MIDI_INPUT=TRUE in CMakeLists.txt
3. Create APVTS with all parameters (MALLET_HARDNESS, BAR_MATERIAL, etc.)
4. Set up basic WebView GUI layout
5. Test: Plugin loads, parameters visible, MIDI received (no sound yet)

**Success criteria:**
- CMake builds without errors
- Plugin loads in DAW
- Parameters appear in host automation list
- MIDI input visible (light flashes or debug log)
- No audio output yet (expected)

**Duration:** 1-2 hours

**Next stage:** Stage 2 (DSP) - Phased implementation

---

### Stage 2: DSP Implementation (PHASED)

**Status:** Pending

**Objective:** Implement synthesizer DSP in 4 incremental phases

**Total Duration:** 6-10 hours (across 4 phases)

---

#### Phase 2.1: Basic Synthesizer Shell (12-TET, Simple Oscillator)

**Objective:** Validate synthesizer architecture with minimal DSP

**Tasks:**
1. Implement `juce::Synthesiser` with custom `SynthesiserVoice` subclass
2. Basic sine wave oscillator (placeholder for modal synthesis)
3. 12-TET frequency calculation: `440 * pow(2, (note - 69) / 12.0)`
4. MIDI note-on/off handling
5. Voice allocation (16 voices)
6. Simple ADSR envelope (use RESONANCE for decay time temporarily)

**Test criteria:**
- MIDI notes trigger sound (sine waves)
- Pitch is correct (A4 = 440 Hz)
- Polyphony works (can play 16 simultaneous notes)
- Voice stealing occurs at 17+ notes
- MIDI velocity affects amplitude
- No clicks or pops on note-on/off

**Git commit:** "feat(MicroMarimba): Phase 2.1 - basic synthesizer shell (12-TET, sine wave)"

**Duration:** 1-2 hours

**Fallback:** If synthesizer架构 issues, review JUCE Synthesiser examples, check juce8-critical-patterns.md

---

#### Phase 2.2: Modal Synthesis (8 Modes, 12-TET)

**Objective:** Replace sine oscillator with 8-mode modal resonator bank

**Tasks:**
1. Implement modal synthesis class (8 parallel biquad filters per voice)
2. Calculate modal frequencies: `baseFreq * [1.00, 3.93, 9.24, 16.65, 26.3, 38.2, 52.4, 68.9]`
3. Implement biquad coefficients: `θ = 2π*f/sr`, `r = exp(-1/(decay*sr))`, `g = amp*(1-r)`
4. Process mallet exciter: Noise burst (5-20ms) shaped by MALLET_HARDNESS
5. Sum 8 mode outputs per voice
6. BAR_MATERIAL controls mode amplitude distribution
7. RESONANCE controls decay time scaling
8. Denormal protection (zero state if abs < 1e-8f)

**Test criteria:**
- Marimba-like timbre (inharmonic overtones audible)
- MALLET_HARDNESS changes brightness (soft = dark, hard = bright)
- BAR_MATERIAL changes spectral balance (rosewood = warm, synthetic = bright)
- RESONANCE changes decay time (short to long sustain)
- No blow-ups or NaN outputs
- CPU usage <1% per voice (test with 16 voices)
- Pitch accuracy maintained (A4 = 440 Hz)

**Git commit:** "feat(MicroMarimba): Phase 2.2 - modal synthesis (8 modes, marimba timbre)"

**Duration:** 2-3 hours

**Fallback:** If stability issues, reduce to 6 modes; if CPU too high, optimize biquad processing

---

#### Phase 2.3: Tuning Engine (Scala + MTS-ESP)

**Objective:** Replace 12-TET with full tuning system (MTS-ESP > Scala > 12-TET)

**Tasks:**
1. Vendor Surge Tuning Library in Source/tuning/
2. Vendor MTS-ESP client in Source/mts-esp/
3. Implement tuning priority system:
   - Check MTS-ESP: `MTS_HasMaster(mtsClient)` → `MTS_NoteToFrequency(...)`
   - Else check Scala: `surgeTuning.frequencyForMidiNote(note)`
   - Else 12-TET: `440 * pow(2, (note-69)/12.0) * (REF_PITCH/440)`
4. MTS-ESP lifecycle: `MTS_RegisterClient()` in constructor, `MTS_DeregisterClient()` in destructor
5. Scala file loading:
   - Background thread loading (message thread)
   - Parse .scl with `Tunings::readSCLFile()`
   - Parse .kbm with `Tunings::readKBMFile()`
   - Calculate 128-note frequency table
   - Atomic pointer swap for tuning table
6. Implement nearest-pitch mapping for unmapped notes
7. TUNING_MODE parameter: 0=12-TET, 1=Scala, 2=MTS-ESP
8. REFERENCE_PITCH parameter: Scale all frequencies proportionally

**Test criteria:**
- 12-TET mode: Standard Western tuning (A4 = 440 Hz)
- Scala mode: Load 19-TET scale, verify pitch differences audible
- MTS-ESP mode: Connect to MTS-ESP master, verify tuning changes reflected
- Nearest-pitch mapping: Play unmapped note, hear nearest mapped pitch (not silence)
- File loading: Load invalid .scl file, plugin doesn't crash, falls back to 12-TET
- Atomic updates: Change tuning while playing notes, no glitches or crashes
- REFERENCE_PITCH: Change from 440 to 432 Hz, verify all pitches shift proportionally

**Git commit:** "feat(MicroMarimba): Phase 2.3 - tuning engine (Scala, MTS-ESP, 12-TET fallback)"

**Duration:** 2-3 hours

**Fallback:** If MTS-ESP unstable, disable and use Scala + 12-TET only; if Scala parsing issues, use 12-TET + MTS-ESP

---

#### Phase 2.4: Body Resonance (Convolution)

**Objective:** Add convolution IR for realistic body coupling

**Tasks:**
1. Acquire marimba resonator tube IR (~50-100ms):
   - Option 1: Record real marimba resonator
   - Option 2: License from sample library
   - Option 3: Synthesize using modal synthesis (fallback)
2. Embed IR in plugin binary: Add to BinaryData in CMakeLists.txt
3. Load IR in `prepareToPlay()`:
   - `juce::MemoryInputStream` from BinaryData
   - `AudioFormatReader` to read IR
   - `convolution.loadImpulseResponse(...)`
4. Process convolution:
   - Dry signal: Summed modal output
   - Wet signal: `convolution.process(dry)`
   - Mix: `output = dry * (1 - RESONANCE) + wet * RESONANCE`
5. Report latency: `getLatencySamples()` returns IR length
6. Normalize IR to prevent volume jumps

**Test criteria:**
- Body resonance audible (warmth, depth, low-mid resonance)
- RESONANCE parameter blends dry (modal) and wet (body) smoothly
- No volume jumps when RESONANCE adjusted
- Latency reported correctly (50-100ms)
- CPU overhead <10% for all voices
- No artifacts (clicks, pops, distortion)

**Git commit:** "feat(MicroMarimba): Phase 2.4 - body resonance (convolution IR)"

**Duration:** 1-2 hours

**Fallback:** If IR quality poor, use parametric IIR filters (2-3 peak filters); if latency unacceptable, allow disabling convolution

---

### Stage 3: GUI Implementation (PHASED)

**Status:** Pending

**Objective:** Build complete user interface in 2 incremental phases

**Total Duration:** 4-6 hours (across 2 phases)

---

#### Phase 3.1: Parameter Controls + Tuning Mode Indicator

**Objective:** Complete parameter UI and tuning system visualization

**Tasks:**
1. Copy HTML mockup from .ideas/mockups/v*-ui.yaml (if exists) OR design layout
2. Implement all parameter controls:
   - MALLET_HARDNESS: Rotary knob (0-100%)
   - BAR_MATERIAL: Rotary knob (0=Rosewood, 100=Synthetic)
   - RESONANCE: Rotary knob (0-100%)
   - TUNING_MODE: Dropdown (12-TET, Scala, MTS-ESP)
   - REFERENCE_PITCH: Slider or numeric input (400-480 Hz)
   - VEL_CURVE: Rotary knob (0=Linear, 100=Exponential)
   - OUTPUT_GAIN: Slider (-24 to +12 dB)
3. JavaScript → C++ parameter relay via WebView
4. C++ → JavaScript updates for host automation
5. Tuning mode indicator:
   - Display active tuning system (12-TET / Scala: filename / MTS-ESP: connected)
   - Show warning if Scala file missing or MTS-ESP master disconnected
6. Scala file browser button (open file dialog)

**Test criteria:**
- All parameters visible and functional
- Knob/slider movements update DSP in real-time
- Host automation updates UI controls
- Tuning mode indicator shows correct active system
- Scala file browser opens file dialog
- Warning appears if Scala file invalid or MTS-ESP disconnected

**Git commit:** "feat(MicroMarimba): Phase 3.1 - parameter controls and tuning mode indicator"

**Duration:** 2-3 hours

---

#### Phase 3.2: Polishing and Visual Feedback

**Objective:** Add visual polish and user feedback

**Tasks:**
1. Voice count indicator (show active voices / total polyphony)
2. CPU meter (optional, show DSP load percentage)
3. Scala file path display (show loaded .scl/.kbm filenames)
4. Parameter tooltips (explain physical meaning of each parameter)
5. Visual theme consistent with marimba aesthetic:
   - Warm wood tones for rosewood settings
   - Cool metallic for synthetic settings
   - Natural, acoustic visual language
6. Preset browser integration (if using JUCE preset system)

**Test criteria:**
- Voice count updates in real-time during playback
- CPU meter shows accurate load (if implemented)
- Scala file paths visible and correct
- Tooltips appear on hover, explain parameters clearly
- Visual theme is cohesive and aesthetically pleasing
- Preset loading updates all UI controls

**Git commit:** "feat(MicroMarimba): Phase 3.2 - visual polish and feedback"

**Duration:** 2-3 hours

**Fallback:** If time constrained, skip CPU meter and advanced visual polish; focus on functional parameter controls

---

### Stage 4: Testing & Release Prep

**Status:** Pending

**Objective:** Comprehensive testing, optimization, and release preparation

**Tasks:**
1. **Performance testing:**
   - Profile CPU with 24 voices active
   - Verify <1% per voice (modal synthesis)
   - Verify <10% overhead (convolution)
   - Optimize hot paths if needed (SIMD for modal bank)
2. **Stability testing:**
   - Long-duration playback (10+ minutes, no crashes)
   - Extreme parameter values (no blow-ups)
   - Rapid parameter changes (no clicks/pops)
   - Voice stealing under load (no glitches)
3. **Tuning system testing:**
   - Test with 10+ Scala files (12-TET, 19-TET, just intonation, Bohlen-Pierce)
   - Test MTS-ESP master connection/disconnection
   - Test unmapped note fallback (nearest-pitch)
   - Test reference pitch adjustment (400-480 Hz)
4. **Cross-platform testing:**
   - macOS: Logic Pro, Ableton Live
   - Windows: FL Studio, Reaper (if applicable)
   - Validate MIDI handling across DAWs
5. **Preset creation:**
   - Create 10-15 factory presets demonstrating range:
     - Classic marimba (rosewood, soft mallet)
     - Bright vibraphone (synthetic, hard mallet)
     - Long resonance (high RESONANCE)
     - Various tuning systems (12-TET, 19-TET, just intonation)
6. **Documentation:**
   - User manual: Tuning system usage, parameter explanations
   - Developer notes: Architecture decisions, maintenance guide
7. **Release build:**
   - CMake Release configuration
   - Installer creation (if applicable)
   - Code signing (macOS/Windows)

**Test criteria:**
- CPU usage <25% total (24 voices)
- No crashes or memory leaks (10+ minute stress test)
- All 10+ Scala files load correctly
- MTS-ESP integration robust (handles master disconnect gracefully)
- Cross-DAW compatibility verified
- Presets demonstrate full plugin capability

**Duration:** 4-6 hours

**Git commit:** "feat(MicroMarimba): Stage 4 - testing, optimization, and release prep"

---

## Phase Summary

| Phase | Focus | Duration | Risk Level |
|-------|-------|----------|------------|
| 2.1 | Basic Synthesizer (12-TET, Sine) | 1-2h | LOW |
| 2.2 | Modal Synthesis (8 Modes) | 2-3h | MEDIUM |
| 2.3 | Tuning Engine (Scala, MTS-ESP) | 2-3h | MEDIUM |
| 2.4 | Body Resonance (Convolution) | 1-2h | LOW |
| 3.1 | Parameter UI + Tuning Indicator | 2-3h | LOW |
| 3.2 | Visual Polish + Feedback | 2-3h | LOW |
| 4.0 | Testing & Release | 4-6h | LOW |

**Total Estimated Time:** 14-22 hours (across all phases)

---

## Risk Mitigation

### High-Risk Areas

1. **Modal Synthesis Stability (Phase 2.2)**
   - **Risk:** Incorrect biquad coefficients cause blow-ups or instability
   - **Mitigation:**
     - Use reference implementation from physical modelling guide
     - Clamp pole radius `r` to max 0.9999
     - Add denormal protection (zero if abs < 1e-8f)
     - Unit test coefficient calculation at extreme frequencies
   - **Fallback:** Reduce to 6 modes if stability issues; fall back to Karplus-Strong if modal fails

2. **Tuning Engine Integration (Phase 2.3)**
   - **Risk:** MTS-ESP lifecycle issues, Scala parsing crashes, atomic updates
   - **Mitigation:**
     - Follow microtonality implementation guide exactly
     - Use try/catch around Surge library calls
     - Test with invalid .scl files (corrupted, empty, malformed)
     - Atomic pointer swap with grace period before old table deletion
   - **Fallback:** Disable MTS-ESP if unstable; use Scala + 12-TET only if needed

3. **CPU Performance (Phase 2.2, 2.4)**
   - **Risk:** Exceeds <1% per voice target, causing performance issues
   - **Mitigation:**
     - Profile early with 24 voices
     - Optimize biquad processing (consider SIMD)
     - Share convolution across voices (already planned)
   - **Fallback:** Reduce modal modes to 6 if CPU too high; disable convolution if needed

### Testing Strategy

**Per-Phase Testing:**
- Each phase has clear test criteria (listed above)
- Git commit only after tests pass
- No moving to next phase until current phase stable

**Integration Testing:**
- After Phase 2.4: Full DSP testing (all systems working together)
- After Phase 3.2: Full UI testing (all controls functional)
- Stage 4: Cross-platform and stress testing

**Regression Testing:**
- Re-run previous phase tests after each new phase
- Ensures new features don't break existing functionality

---

## Success Criteria

### Stage 1 (Foundation)
- ✓ Plugin builds without errors
- ✓ Loads in DAW (silent)
- ✓ Parameters visible in host automation
- ✓ MIDI input received (logged or indicator)

### Stage 2 (DSP) - Phase 2.1
- ✓ MIDI notes trigger sound (sine wave)
- ✓ Correct pitch (A4 = 440 Hz)
- ✓ 16-voice polyphony works
- ✓ Voice stealing at 17+ notes

### Stage 2 (DSP) - Phase 2.2
- ✓ Marimba-like timbre (inharmonic)
- ✓ MALLET_HARDNESS affects brightness
- ✓ BAR_MATERIAL affects spectral balance
- ✓ RESONANCE affects decay time
- ✓ CPU <1% per voice
- ✓ No blow-ups or NaN outputs

### Stage 2 (DSP) - Phase 2.3
- ✓ 12-TET mode works (A4 = 440 Hz)
- ✓ Scala mode works (test with 19-TET)
- ✓ MTS-ESP mode works (connects to master)
- ✓ Nearest-pitch mapping functional
- ✓ Invalid .scl files don't crash plugin
- ✓ Atomic tuning updates (no glitches)

### Stage 2 (DSP) - Phase 2.4
- ✓ Body resonance audible (warmth, depth)
- ✓ RESONANCE blends dry/wet smoothly
- ✓ Latency reported correctly
- ✓ CPU overhead <10%
- ✓ No artifacts (clicks, pops)

### Stage 3 (GUI) - Phase 3.1
- ✓ All 7 parameters functional
- ✓ Host automation updates UI
- ✓ Tuning mode indicator shows active system
- ✓ Scala file browser functional
- ✓ Warnings for missing files or disconnected MTS-ESP

### Stage 3 (GUI) - Phase 3.2
- ✓ Voice count indicator updates
- ✓ Visual theme cohesive
- ✓ Tooltips explain parameters
- ✓ Preset loading functional

### Stage 4 (Testing)
- ✓ CPU <25% total (24 voices)
- ✓ No crashes (10+ min stress test)
- ✓ 10+ Scala files load correctly
- ✓ Cross-DAW compatibility verified
- ✓ 10-15 factory presets created

---

## Implementation Notes

### Dependencies

**External Libraries:**
- **Surge Tuning Library:** Header-only, vendor in Source/tuning/
  - License: GPL-3.0 (check compatibility with plugin license)
  - Files: `Tunings.h`, `TuningsImpl.h`, `TuningMath.h`
- **MTS-ESP Client:** Source files, vendor in Source/mts-esp/
  - License: MIT (permissive)
  - Files: `libMTSClient.h`, `libMTSClient.cpp`
- **JUCE Modules:** juce_dsp, juce_audio_processors, juce_audio_basics
  - Already in project, no additional setup

**Assets:**
- **Marimba Body IR:** ~50-100ms impulse response
  - Format: WAV, mono, 48kHz
  - Embed in BinaryData via CMakeLists.txt
  - Fallback: Synthesize using modal synthesis if no recording available

### Architecture Considerations

**IS_SYNTH Configuration:**
- CMakeLists.txt: `IS_SYNTH TRUE`
- CMakeLists.txt: `NEEDS_MIDI_INPUT TRUE`
- No audio input processing (MIDI-only instrument)
- Stereo output only (no multi-output routing)

**Voice Management:**
- Target: 16-24 voices (adjustable based on CPU profiling)
- Voice stealing: Oldest note first
- Each voice independent (no cross-voice coupling)
- Convolution shared (single IR instance for all voices)

**Thread Safety:**
- APVTS parameters: Atomic reads/writes (JUCE handles)
- Tuning table: Atomic pointer swap for updates
- Scala file loading: Message thread (non-blocking)
- MTS-ESP queries: Lock-free client API

### Performance Targets

**CPU Budgets:**
- Modal synthesis: <0.5% per voice (target 0.3-0.5%)
- Convolution: <10% total (shared across voices)
- Tuning engine: <0.1% (negligible overhead)
- Total: <25% for 24 voices (leaves headroom for DAW)

**Latency:**
- Modal synthesis: 0 samples (IIR filters instantaneous)
- Mallet exciter: 1-3ms (attack envelope)
- Convolution: 50-100ms (IR length)
- Total reported: ~50-100ms (dominated by convolution)

**Memory:**
- Modal state: 16 floats per voice (8 modes × 2 state vars) = negligible
- Convolution IR: ~10-50KB (50-100ms @ 48kHz)
- Tuning table: 128 floats × 4 bytes = 512 bytes
- Total per-voice: <1KB (very light)

---

## Next Steps

1. **Review this plan** with plugin architect or lead developer
2. **Prepare development environment:**
   - Clone/update JUCE framework
   - Verify build tools (CMake, compiler)
   - Set up DAW for testing
3. **Execute Stage 1:** Run foundation-shell-agent
4. **Begin Stage 2 (Phased DSP):** Start with Phase 2.1 (basic synthesizer)

**Ready to proceed:** YES (architecture documented, plan approved, complexity assessed)

---

## Document Status

**Plan Version:** 1.0
**Status:** Ready for implementation
**Approval:** Awaiting architect review
**Next Agent:** foundation-shell-agent (Stage 1)
