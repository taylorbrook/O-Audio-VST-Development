# Stage 0 Context: O-IntonationPad Research & Planning

**Stage:** 0 - Ideation (Research & Planning)
**Date:** 2026-01-29
**Agent:** research-planning-agent
**Status:** ✅ Complete

---

## Phase Summary

Stage 0 research and planning for O-IntonationPad (wavetable pad synthesizer with just intonation chord generation) has been completed. This plugin represents maximum complexity (5.0/5.0) and requires multi-phase staged implementation.

---

## Key Findings

### Complexity Assessment

**Final Score: 5.0 / 5.0** (Maximum complexity - staged implementation required)

**Breakdown:**
- **Parameter Score:** 2.0 (15 parameters / 5 = 3.0, capped at 2.0)
- **Algorithm Score:** 3.0 (wavetable engine, JI tuning, chord generation)
- **Feature Score:** 3.0 (MIDI synth architecture, Scala file I/O, 96 oscillator management)

**Total:** 2.0 + 3.0 + 3.0 = 7.0 → capped at 5.0

This is the highest complexity plugin in the codebase to date. Complexity tier: **Tier 6 (DEEP research)**.

---

## Research Depth

**Tier 6 Indicators:**
1. **MIDI synthesizer with voice management** (Tier 4)
2. **File I/O (Scala .scl import)** (Tier 5)
3. **96 simultaneous oscillators** (performance critical)
4. **Complex algorithmic features** (chord generation, JI calculations)

**Research Duration:** ~90 minutes (extended deep research)

**Features Researched:** 10 major features
1. Wavetable oscillator engine
2. Just intonation ratio calculation (5 tuning systems)
3. Chord generation algorithm
4. Voice management system (96 oscillators)
5. MIDI input processing
6. Scala file parser
7. LFO modulation
8. Envelope system (ADSR)
9. Filter (low-pass)
10. Randomization system (inversions, timing, detuning)

---

## Critical Decisions

### 1. Staged DSP Implementation (4 Phases)

**Decision:** Break Stage 2 (DSP) into 4 phases with validation checkpoints

**Rationale:**
- 96 oscillators (12 voices × 8 polyphony) is near real-time CPU limit
- Risk of CPU usage exceeding 80% single core
- Need incremental validation to catch performance issues early

**Phases:**
- **Phase 2.1:** Basic wavetable oscillator (single voice, validation prototype)
- **Phase 2.2:** Chord generation system (12 sub-voices per main voice)
- **Phase 2.3:** Tuning system integration (JI, Scala support)
- **Phase 2.4:** Modulation, filtering, scaling to 96 oscillators (with CPU profiling)

Each phase has validation criteria and fallback strategies.

---

### 2. Use Existing scala-tuning-engine Module

**Decision:** Integrate existing `modules/tuning/scala-tuning-engine` (v1.13.0)

**Rationale:**
- Proven implementation from O-Lyrica v1.17.0
- Thread-safe atomic frequency table (lock-free audio thread reads)
- Supports any scale size (7, 12, 19, 31 notes) with linear mapping
- Handles Scala .scl parsing, JI/Pythagorean/Historical temperaments
- **Saves 10-15 hours of development time**

**Alternative Rejected:** Custom tuning system implementation
- Why rejected: Redundant effort, module already validated and tested

---

### 3. Global LFO (Not Per-Voice)

**Decision:** Use single global LFO shared across all voices

**Rationale:**
- Pad synths benefit from unified movement (all notes modulate in sync)
- Lower CPU (1 LFO vs. 8 LFOs for 8 polyphony)
- Matches professional synth behavior (Serum, Vital use global LFO for pads)
- Prevents phasiness from per-voice LFO detuning

**Alternative Rejected:** Per-voice LFO with independent phases
- Why rejected: Causes undesirable phasiness, higher CPU, less unified pad movement
- When to reconsider: If user testing reveals desire for "detuned LFO" width effect (can add as v1.1 feature)

---

### 4. Band-Limited Wavetables (Not Oversampling)

**Decision:** Use pre-calculated band-limited wavetables for anti-aliasing

**Rationale:**
- Industry standard approach (Serum, Vital use band-limited tables)
- Lower CPU than 2x or 4x oversampling
- Generate multiple versions of each wavetable with progressively fewer harmonics at higher frequencies

**Alternative Rejected:** Real-time oversampling (2x or 4x internal sample rate)
- Why rejected: Higher CPU cost (2x sample rate = 2x processing)
- When to reconsider: If band-limited tables don't prevent aliasing (Phase 2.1 validation)

---

### 5. Scale-Degree Chord Generation (Not Fixed Tables)

**Decision:** Implement scale-degree analysis for chord generation

**Rationale:**
- Musically intelligent: Follows scale/key changes dynamically
- Matches creative brief: "scale-degree aware chord construction"
- Enables realistic progressions (I-IV-V sounds correct in any key)

**Alternative Rejected:** Fixed chord lookup tables
- Why rejected: Not scale-aware, requires manual key change updates
- When to reconsider: If scale-degree logic proves too complex or buggy (Phase 2.2 validation)

---

## Primary Risks Identified

### 1. Voice Management (96 Oscillators) - MEDIUM Risk

**Risk:** CPU usage exceeds 80% single core, making plugin unusable

**Likelihood:** MEDIUM (60% project risk)
- 96 oscillators is near upper limit for real-time processing
- Wavetable interpolation is CPU-intensive (likely 50-60% of total CPU)

**Mitigation Strategy:**
1. Profile CPU at each phase (Phase 2.1: 1 voice, Phase 2.2: 12 voices, Phase 2.4: 96 voices)
2. Optimize hot paths (phase advancement, interpolation)
3. Consider SIMD vectorization (juce::FloatVectorOperations)

**Fallback Plans:**
- **Fallback 1:** Reduce polyphony to 6 (72 oscillators) - 25% CPU reduction
- **Fallback 2:** Reduce max chord voices to 8 (64 oscillators) - maintains core feature
- **Fallback 3:** Dynamic voice allocation (fewer sub-voices for simple chords)

**Validation Checkpoint:** Phase 2.4 (CPU profiling with 96 oscillators)

---

### 2. Chord Generation Algorithm - MEDIUM Risk

**Risk:** Incorrect chord types or voice distribution bugs

**Likelihood:** LOW-MEDIUM
- Music theory logic is complex but deterministic
- Edge cases: Non-scale notes, voiceCount > intervals, complexity extremes

**Mitigation Strategy:**
1. Implement lookup tables for common scales first (Major, Minor, Dorian)
2. Unit test chord generation for each scale degree (I-vii)
3. Test edge cases: Non-scale notes, voiceCount=2, complexity=0% and 100%

**Fallback Plans:**
- **Fallback 1:** Fixed chord tables (pre-calculated voicings per root)
- **Fallback 2:** Simple MIDI harmonizer (fixed +3, +7 semitone offsets)

**Validation Checkpoint:** Phase 2.2 (chord type correctness testing)

---

### 3. Wavetable Aliasing - LOW Risk

**Risk:** Audible aliasing artifacts above 15kHz

**Likelihood:** LOW
- Band-limited wavetables are proven approach
- Mitigated by proper wavetable generation

**Mitigation Strategy:**
1. Generate band-limited versions of each wavetable (progressively fewer harmonics at high frequencies)
2. Spectrum analysis during Phase 2.1 validation (verify <-60dB above 20kHz)

**Fallback Plans:**
- **Fallback 1:** Add 2x oversampling (doubles CPU but eliminates aliasing)
- **Fallback 2:** Reduce wavetable frame count (256 → 128) for faster interpolation

**Validation Checkpoint:** Phase 2.1 (spectrum analysis)

---

## Technical Constraints

### Performance Targets

- **CPU Usage:** <80% single core @ 48kHz with 96 oscillators (strict requirement)
- **Target:** <70% single core (comfortable headroom)
- **Latency:** <5ms (<240 samples @ 48kHz)
- **Polyphony:** 8 voices minimum, 96 oscillators total (12 × 8)

### Memory Constraints

- **Wavetable Storage:** ~16MB for 8 wavetables (256 frames × 2048 samples × 4 bytes × 8 tables)
- **Voice Memory:** Pre-allocated in constructor (no audio thread allocations)

### Platform Constraints

- **macOS:** Code signing required, AudioComponentRegistrar cache clearing
- **DAW Compatibility:** Test in Logic Pro, Ableton, FL Studio, Reaper
- **Plugin Formats:** VST3 and AU (both required)

---

## Module Dependencies

### JUCE Modules

- `juce_audio_basics` - AudioBuffer, MIDI processing
- `juce_audio_processors` - AudioProcessor, APVTS, Synthesiser, SynthesiserVoice
- `juce_dsp` - StateVariableTPTFilter (low-pass), Oscillator (LFO)
- `juce_gui_basics` - Basic GUI components
- `juce_gui_extra` - WebBrowserComponent (WebView UI)

### Ouaricon Modules

- `scala-tuning-engine` (v1.13.0) - Just intonation and Scala file support
  - Thread-safe atomic frequency table
  - Supports any scale size (linear mapping)
  - Pre-calculated JI, Pythagorean, Historical temperament tables

---

## JUCE 8 Critical Patterns Applied

### Pattern 22: IS_SYNTH Flag (CRITICAL)

**Application:** Set `IS_SYNTH TRUE` in juce_add_plugin()

**Why Critical:**
- Without IS_SYNTH flag, plugin appears in Effects category (not Instruments)
- DAW doesn't route MIDI to plugin
- Plugin loads but produces no audio

**CMakeLists.txt:**
```cmake
juce_add_plugin(O-IntonationPad
    IS_SYNTH TRUE           # REQUIRED for instrument plugins
    NEEDS_MIDI_INPUT TRUE   # Explicit MIDI requirement
    ...
)
```

---

### Pattern 4: Bus Configuration (Synth = Output-Only)

**Application:** Use output-only BusesProperties (no input bus)

**Why Critical:**
- Synths create audio from scratch (no audio input needed)
- Adding input bus causes "missing input" errors in DAWs

**PluginProcessor Constructor:**
```cpp
AudioProcessor(BusesProperties()
    .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    // No .withInput() - synths have no audio input
```

---

### Pattern 17: juce::dsp API (Modern DSP Pipeline)

**Application:** Use ProcessSpec + ProcessContextReplacing for filter

**PluginProcessor::prepareToPlay:**
```cpp
juce::dsp::ProcessSpec spec;
spec.sampleRate = sampleRate;
spec.maximumBlockSize = samplesPerBlock;
spec.numChannels = 2;
filter.prepare(spec);  // NOT filter.setSampleRate()
```

**PluginProcessor::processBlock:**
```cpp
juce::dsp::AudioBlock<float> block(buffer);
juce::dsp::ProcessContextReplacing<float> context(block);
filter.process(context);  // NOT filter.processMono() or processStereo()
```

---

## Research Sources

### Professional Plugins Researched

1. **Xfer Serum 2** - Wavetable synthesis gold standard
   - 256-frame wavetables with smooth morphing
   - Band-limited tables for anti-aliasing
   - Observation: Deep modulation routing (LFO → wavetable position)

2. **Vital** - Free wavetable synth matching Serum
   - High-resolution wavetable editor
   - Open-source (can inspect implementation)
   - Observation: Efficient wavetable engine, <70% CPU with 16 voices

3. **Spectrasonics Omnisphere** - Pad synthesis reference
   - Extensive wavetable library
   - LFO modulation for evolving pads
   - Observation: Focus on evolving textures (matches O-IntonationPad use case)

4. **U-he Zebra** - Modular synthesis with wavetable oscillators
   - Known for lush pad character
   - Observation: Uses oscillator stacking for rich pads (similar to 12-voice chord approach)

### JUCE Documentation

- **juce::Synthesiser / SynthesiserVoice** - Voice management and polyphony
- **Wavetable synthesis tutorial** - Phase-driven lookup with linear interpolation
- **juce::ADSR** - Envelope generation
- **juce::dsp::StateVariableTPTFilter** - Modern TPT filter (low CPU, stable)
- **juce::dsp::Oscillator** - Template-based oscillator (used for LFO)

### Microtonal Tuning Resources

- **Scala Software & Archive** - 3000+ .scl files, official format specification
- **Surge Synth Team Tuning Guide** - 182 microtonal tables (JI, Pythagorean, historical)
- **Scale Workshop** - Browser-based scale editor
- **Just Intonation Theory** - 5-limit interval tables (Wikipedia)
- **Existing Module:** `scala-tuning-engine` v1.13.0 (O-Lyrica implementation)

### Chord Generation Research

- **AutoHarmonizer (2024)** - Harmonic density-controllable melody harmonization
- **Music Theory Chord Progressions** - Recursive enumeration of chord progressions
- **Automatic SATB Part-Writer** - Voice distribution algorithm

---

## Implementation Constraints

### Must-Have Features (v1.0)

1. ✅ Wavetable oscillator with 8 built-in wavetables
2. ✅ 1-note chord mode (2-12 voices)
3. ✅ 5 tuning systems (JI, Pythagorean, Historical, Scala, Manual)
4. ✅ LFO modulation to wavetable position
5. ✅ ADSR envelope
6. ✅ Low-pass filter
7. ✅ Randomization (inversions, timing, detuning)
8. ✅ 8 polyphony minimum

### Deferred Features (v1.1+)

- Custom wavetable import (.wav format)
- Per-voice LFO (if user requests)
- MPE support (polyphonic expression)
- Additional historical temperaments
- MIDI learn for parameters

---

## Next Steps

**Stage 1 (Foundation):** Create build system and parameter definitions
- Run `/implement O-IntonationPad` to start Stage 1
- Generate CMakeLists.txt with IS_SYNTH TRUE
- Create PluginProcessor with output-only bus configuration
- Define 15 APVTS parameters
- Verify plugin loads as instrument in DAW

**Stage 2 (DSP):** 4-phase implementation with validation checkpoints
- Phase 2.1: Basic wavetable oscillator (validation prototype)
- Phase 2.2: Chord generation system
- Phase 2.3: Tuning system integration
- Phase 2.4: Modulation, filtering, scaling to 96 oscillators (CPU profiling)

**Stage 3 (GUI):** WebView UI with scala-tuning-engine components

**Stage 4 (Polish):** Performance optimization, testing, documentation

**Expected Timeline:** 46-62 hours (6-8 full working days)

---

## Conclusion

O-IntonationPad is the most complex plugin in the codebase to date (5.0/5.0 complexity). The primary risk is CPU performance with 96 oscillators, mitigated by:

1. **Incremental development:** 4-phase DSP implementation with validation checkpoints
2. **Fallback strategies:** Pre-planned for high-risk features (polyphony reduction, voice count reduction)
3. **Module reuse:** scala-tuning-engine saves 10-15 hours of development time
4. **Professional research:** Reference implementations (Serum, Vital) validate architectural decisions

All Stage 0 research is complete. Architecture documented in ARCHITECTURE.md with complete JUCE class mappings, algorithm details, and integration points. Implementation roadmap documented in ROADMAP.md with phase breakdown and risk mitigation.

**Ready for Stage 1 implementation.**
