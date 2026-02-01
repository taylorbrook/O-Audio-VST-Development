# Stage 2: DSP - Context

**Stage:** 2 - DSP Implementation
**Date:** 2026-01-29
**Participants:** User, Claude
**Status:** Discuss phase complete

---

## Discussion Summary

Stage 2 DSP for O-IntonationPad will implement the complete audio synthesis engine through 4 phases:

1. **Phase 2.1:** Basic wavetable oscillator (validation prototype)
2. **Phase 2.2:** Chord generation system
3. **Phase 2.3:** Tuning system integration (scala-tuning-engine)
4. **Phase 2.4:** Modulation, filtering, scale to 96 oscillators

This is a maximum complexity stage (5.0/5.0) requiring incremental validation at each phase.

---

## Requirements Confirmed

### From Stage 0/1 (Carry Forward)

- 15 APVTS parameters already implemented in PluginProcessor
- Output-only bus configuration (synth, no audio input)
- IS_SYNTH TRUE flag set in CMakeLists.txt
- Plugin verified loading as instrument in DAW

### DSP Requirements

- **Polyphony:** 8 voices minimum (can be reduced to 6 as fallback)
- **Chord voices:** 2-12 sub-voices per main voice (voiceCount parameter)
- **Total oscillators:** 96 target (12 voices × 8 polyphony), 72 fallback
- **Wavetables:** 8 built-in wavetables with frame interpolation
- **Tuning systems:** 5 systems via scala-tuning-engine module
- **CPU target:** <80% single core @ 48kHz (strict), <70% target (comfortable)

---

## Constraints Identified

### Performance Constraints

| Metric | Target | Hard Limit | Fallback |
|--------|--------|------------|----------|
| CPU usage | <70% | <80% | Reduce polyphony to 6 |
| Latency | <5ms | <10ms | N/A |
| Voice stealing | Glitch-free | 10-20ms crossfade | Extend crossfade time |

### Technical Constraints

- No audio thread allocations (pre-allocate all voices in constructor)
- No locks on audio thread (atomic operations only for parameter reads)
- Band-limited wavetables required for anti-aliasing
- Global LFO (not per-voice) for unified pad movement

---

## Approach Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Wavetable validation | Sine wave first | Easier to verify oscillator math and anti-aliasing before adding harmonic complexity |
| Band-limiting strategy | Pre-generate at build time | Static mip-map levels avoid runtime startup cost; sample rate independence |
| Off-scale MIDI handling | Chromatic pass-through | User hears single note for off-scale input - respects original pitch intent |
| CPU fallback priority | Reduce polyphony to 6 | Maintains full chord richness (12 voices), only affects simultaneous note count |
| LFO architecture | Global (shared) | Unified pad movement, lower CPU, matches professional synth behavior |
| Chord generation | Scale-degree analysis | Musically intelligent, dynamic key changes, per-degree chord quality |

---

## Phase-Specific Details

### Phase 2.1: Basic Wavetable Oscillator

**Goal:** Single-voice wavetable synth (no chords) to validate oscillator engine

**Implementation Approach:**
1. Create WavetableVoice class (subclass of juce::SynthesiserVoice)
2. Start with pure sine wave wavetable (256 frames × 2048 samples, all frames identical)
3. Implement linear interpolation for frame and sample positions
4. Add juce::ADSR envelope with attackTime/releaseTime parameters
5. Integrate with juce::Synthesiser (8 voices)

**Validation Criteria:**
- Single notes produce clean sine wave output
- 8-voice polyphony works without glitches
- ADSR envelope shapes amplitude correctly
- No audible aliasing (spectrum analysis: <-60dB above 20kHz)

### Phase 2.2: Chord Generation

**Goal:** Add chord generation algorithm with scale-degree analysis

**Implementation Approach:**
1. Create ChordGenerator class with scale pattern lookup tables
2. Implement pitch-class to scale-degree mapping
3. Select chord type based on scale degree (I=Major, ii=minor, vii°=dim)
4. Implement voice distribution based on complexity and voiceCount
5. Handle off-scale notes with **chromatic pass-through** (single note, no chord)

**Scale Patterns (intervals from root):**
- Major: [0, 2, 4, 5, 7, 9, 11]
- Minor: [0, 2, 3, 5, 7, 8, 10]
- Dorian: [0, 2, 3, 5, 7, 9, 10]
- (etc. for all 10 scales)

**Validation Criteria:**
- Single note triggers chord (multiple pitches audible)
- Chord types correct for scale degrees
- Off-scale notes play as single notes (pass-through)
- CPU usage with 12 voices × 1 polyphony < 20%

### Phase 2.3: Tuning System Integration

**Goal:** Integrate scala-tuning-engine for just intonation and Scala support

**Implementation Approach:**
1. Add scala-tuning-engine module via `/module:add O-IntonationPad scala-tuning-engine`
2. Initialize OuariconTuningEngine in PluginProcessor
3. Hook up tuningSystem and keyRoot parameters
4. Modify chord generation to use `tuning.getFrequency(midiNote)`
5. Test 12-TET, JI, and Pythagorean modes

**Validation Criteria:**
- 12-TET mode produces standard equal temperament
- JI mode produces audibly different intervals (less beating)
- Pythagorean mode produces characteristic sharp thirds
- Frequency accuracy: ±1 cent for all tuning systems

### Phase 2.4: Modulation, Filtering, Polish

**Goal:** Add LFO, filter, randomization, scale to 96 oscillators

**Implementation Approach:**
1. Implement global LFO (juce::dsp::Oscillator<float>) modulating wavetablePos
2. Add StateVariableTPTFilter (low-pass, 12dB/oct) to mixed output
3. Implement randomization (inversion, timing, detune) per note-on
4. Scale to 96 oscillators (12 × 8) with CPU profiling
5. **If CPU >80%:** Apply fallback - reduce polyphony to 6 (72 oscillators)

**Validation Criteria:**
- LFO modulates wavetable position (audible sweeping)
- Filter shapes tonal brightness
- Randomization adds organic variation
- **CPU usage with 96 oscillators < 80% single core @ 48kHz**
- Voice stealing glitch-free (no pops/clicks)

---

## Module Dependencies

### Required for Stage 2

| Module | Version | Purpose |
|--------|---------|---------|
| scala-tuning-engine | v1.13.0 | Just intonation and Scala file support |

**Add with:** `/module:add O-IntonationPad scala-tuning-engine`

### JUCE Components

- `juce::Synthesiser` / `juce::SynthesiserVoice` - Voice management
- `juce::ADSR` - Envelope generation
- `juce::dsp::Oscillator<float>` - LFO implementation
- `juce::dsp::StateVariableTPTFilter` - Low-pass filter
- `juce::FloatVectorOperations` - SIMD optimization (if needed)

---

## Open Questions

None - all decisions confirmed in discuss phase.

---

## Risk Assessment

### Primary Risk: 96 Oscillator CPU Usage

**Likelihood:** MEDIUM (60% project risk)
**Impact:** HIGH (plugin unusable if exceeded)

**Mitigation:**
1. Profile CPU at each phase (2.1: 8 osc, 2.2: 96 osc, 2.4: final)
2. Optimize hot paths (phase advancement, interpolation)
3. **First fallback:** Reduce polyphony to 6 (72 oscillators)
4. **Second fallback:** Reduce max chord voices to 8 (64 oscillators)

### Secondary Risk: Chord Generation Bugs

**Likelihood:** LOW-MEDIUM
**Impact:** MEDIUM (wrong chords sound bad but plugin still works)

**Mitigation:**
1. Unit test chord types for each scale degree
2. Test edge cases: voiceCount=2, complexity=0%/100%
3. **Fallback:** Fixed chord tables if scale-degree logic fails

---

## Next Phase

Ready for: **research** phase

The discuss phase is complete. Next steps:
1. Run `/plugin:research O-IntonationPad --stage 2` to investigate implementation details
2. Or run `/plugin:plan O-IntonationPad --stage 2` to create execution plan

---

## Context Preservation

**Key decisions for implementation:**
- Start wavetable with sine wave (validation first)
- Pre-generate band-limited mip-maps at build time
- Off-scale MIDI notes: chromatic pass-through (single note)
- CPU fallback: reduce polyphony to 6 first
- Global LFO, not per-voice
- Scale-degree chord generation with scale pattern lookup tables
