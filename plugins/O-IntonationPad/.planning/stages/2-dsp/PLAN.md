# Stage 2: DSP - Execution Plan

**Plugin:** O-IntonationPad
**Stage:** 2 - DSP Implementation
**Created:** 2026-01-29
**Status:** Ready for execution

---

## Goal

Implement the complete audio synthesis engine for O-IntonationPad: wavetable oscillators, chord generation with scale-degree analysis, just intonation tuning via scala-tuning-engine module, LFO modulation, filtering, and voice management for up to 96 simultaneous oscillators.

---

## Phase Structure

Stage 2 is divided into 4 phases with validation checkpoints:

| Phase | Goal | Oscillators | CPU Target |
|-------|------|-------------|------------|
| 2.1 | Basic wavetable oscillator | 8 (1 per voice) | <5% |
| 2.2 | Chord generation | 96 (12 × 8) | <30% |
| 2.3 | Tuning system integration | 96 | <35% |
| 2.4 | Modulation, filtering, polish | 96 | <80% |

---

## Tasks

### Phase 2.1: Basic Wavetable Oscillator

#### Task 1: Create WavetableOscillator class
- **Files:** Source/DSP/WavetableOscillator.h, Source/DSP/WavetableOscillator.cpp
- **Depends on:** None
- **Description:**
  - Standalone oscillator class (not a SynthesiserVoice yet)
  - Holds wavetable data: 256 frames × 2048 samples
  - Methods: `setFrequency()`, `setWavetablePosition()`, `getNextSample()`
  - Linear interpolation for both frame position and sample position
  - Start with all frames = sine wave (validation mode)

#### Task 2: Create WavetableVoice class
- **Files:** Source/DSP/WavetableVoice.h, Source/DSP/WavetableVoice.cpp
- **Depends on:** Task 1
- **Description:**
  - Subclass of `juce::SynthesiserVoice`
  - Contains single WavetableOscillator (expanded to 12 in Phase 2.2)
  - Implements `canPlaySound()`, `startNote()`, `stopNote()`, `renderNextBlock()`
  - juce::ADSR envelope with attack/release from parameters
  - Atomic parameter reads (no locks on audio thread)

#### Task 3: Create WavetableSound class
- **Files:** Source/DSP/WavetableSound.h
- **Depends on:** None
- **Description:**
  - Subclass of `juce::SynthesiserSound`
  - Simple implementation: `appliesToNote()` returns true for all notes
  - `appliesToChannel()` returns true for all channels

#### Task 4: Integrate Synthesiser in PluginProcessor
- **Files:** Source/PluginProcessor.h, Source/PluginProcessor.cpp
- **Depends on:** Tasks 2, 3
- **Description:**
  - Add 8 WavetableVoice instances to synthesiser in constructor
  - Add WavetableSound to synthesiser
  - In `prepareToPlay()`: Set sample rate, initialize voices
  - In `processBlock()`: Call `synthesiser.renderNextBlock()`
  - Pass MIDI buffer to synthesiser
  - Connect attackTime/releaseTime parameters to voices

#### Task 5: Generate sine wavetable data
- **Files:** Source/DSP/WavetableData.h
- **Depends on:** Task 1
- **Description:**
  - Static constexpr array with sine wave (2048 samples)
  - Helper function to generate band-limited wavetables (for future use)
  - All 256 frames point to same sine data (validation mode)

#### Task 6: Validation - Phase 2.1
- **Files:** None (testing only)
- **Depends on:** Tasks 1-5
- **Description:**
  - Build and install plugin
  - Test single notes produce clean sine wave
  - Test 8-voice polyphony (play chord)
  - Verify ADSR envelope works
  - CPU profiling: expect <5% single core

**Phase 2.1 Exit Criteria:**
- [ ] Single notes produce clean sine wave output
- [ ] 8-voice polyphony works without glitches
- [ ] ADSR envelope shapes amplitude correctly
- [ ] CPU usage < 5% with 8 voices active

---

### Phase 2.2: Chord Generation System

#### Task 7: Create ChordGenerator class
- **Files:** Source/DSP/ChordGenerator.h, Source/DSP/ChordGenerator.cpp
- **Depends on:** Phase 2.1 complete
- **Description:**
  - Scale pattern lookup tables for all 10 scales (intervals from root)
  - Method: `getScaleDegree(int midiNote, int keyRoot, ScaleType scale)` → 0-6 or -1 for off-scale
  - Method: `getChordType(int scaleDegree, ScaleType scale)` → Major/Minor/Diminished/Augmented
  - Method: `generateChordVoices(int midiNote, int voiceCount, float complexity, ...)` → vector of pitches

#### Task 8: Implement chord voice distribution
- **Files:** Source/DSP/ChordGenerator.cpp
- **Depends on:** Task 7
- **Description:**
  - Complexity 0% = triad only (root, 3rd, 5th)
  - Complexity 50% = 7th chord
  - Complexity 100% = 13th chord (all extensions)
  - Voice count determines octave spreading:
    - voiceCount=2: root + 5th
    - voiceCount=3: root + 3rd + 5th (triad)
    - voiceCount=5: root + 3rd + 5th + 7th + octave
    - voiceCount=12: full voicing with octave doubling
  - Off-scale MIDI notes: chromatic pass-through (single note, no chord)

#### Task 9: Expand WavetableVoice to multi-oscillator
- **Files:** Source/DSP/WavetableVoice.h, Source/DSP/WavetableVoice.cpp
- **Depends on:** Tasks 7, 8
- **Description:**
  - Change from 1 oscillator to array of 12 oscillators (MAX_CHORD_VOICES)
  - Store active voice count per note
  - `startNote()` calls ChordGenerator to get pitches for each sub-oscillator
  - `renderNextBlock()` sums all active sub-oscillators
  - Shared envelope across all sub-voices

#### Task 10: Connect chord parameters to DSP
- **Files:** Source/PluginProcessor.cpp
- **Depends on:** Task 9
- **Description:**
  - Create atomic parameter pointers for: voiceCount, complexity, keyRoot, keyScale
  - Pass to WavetableVoice on note-on
  - ChordGenerator uses current parameter values

#### Task 11: Validation - Phase 2.2
- **Files:** None (testing only)
- **Depends on:** Tasks 7-10
- **Description:**
  - Test single note triggers chord (multiple pitches audible)
  - Verify chord types for each scale degree (I=Major, ii=minor, vii°=dim)
  - Test voiceCount parameter (2 voices vs 12 voices)
  - Test complexity parameter (0%=triad, 100%=13th)
  - Test off-scale notes (chromatic pass-through)
  - CPU profiling: expect <30% with 12 voices × 8 polyphony

**Phase 2.2 Exit Criteria:**
- [ ] Single note triggers chord (multiple pitches audible)
- [ ] Chord types correct for scale degrees
- [ ] voiceCount parameter spreads voices
- [ ] Complexity parameter affects extensions
- [ ] Off-scale notes pass through as single notes
- [ ] CPU usage < 30% with 96 oscillators

---

### Phase 2.3: Tuning System Integration

#### Task 12: Add scala-tuning-engine module
- **Files:** CMakeLists.txt, modules.json
- **Depends on:** Phase 2.2 complete
- **Description:**
  - Run `/module:add O-IntonationPad scala-tuning-engine`
  - Update CMakeLists.txt with `ouaricon_add_module()` call
  - Verify module compiles with plugin

#### Task 13: Initialize OuariconTuningEngine
- **Files:** Source/PluginProcessor.h, Source/PluginProcessor.cpp
- **Depends on:** Task 12
- **Description:**
  - Include scala-tuning-engine headers
  - Add OuariconTuningEngine member to PluginProcessor
  - Initialize with 12-TET mode (default)
  - Store atomic pointers for tuningSystem and keyRoot parameters

#### Task 14: Implement tuning parameter handling
- **Files:** Source/PluginProcessor.cpp
- **Depends on:** Task 13
- **Description:**
  - On prepareToPlay: Initialize tuning engine
  - Create parameter listener or check in processBlock
  - Map tuningSystem choice to tuning mode:
    - "12-TET" → setMode(TwelveTET)
    - "Just Intonation" → setMode(JustIntonation)
    - "Pythagorean" → setMode(Pythagorean)
    - "Historical" → setMode(Historical)
    - "Scala" → setMode(Scala) (file loading in Stage 4)
  - On keyRoot change: `tuning.setTonicNote(keyRoot)`

#### Task 15: Modify ChordGenerator for tuned frequencies
- **Files:** Source/DSP/ChordGenerator.cpp, Source/DSP/WavetableVoice.cpp
- **Depends on:** Tasks 13, 14
- **Description:**
  - ChordGenerator outputs MIDI note numbers (as before)
  - WavetableVoice gets tuned frequency from tuning engine
  - `float freq = tuning.getFrequency(midiNote)`
  - Set oscillator frequency with tuned value

#### Task 16: Validation - Phase 2.3
- **Files:** None (testing only)
- **Depends on:** Tasks 12-15
- **Description:**
  - Test 12-TET mode: verify standard equal temperament
  - Test JI mode: play major chord, hear reduced beating vs 12-TET
  - Test Pythagorean mode: hear characteristic sharp thirds
  - Test keyRoot changes: tuning should shift
  - Frequency accuracy check: ±1 cent tolerance
  - CPU profiling: expect <35% (tuning adds minimal overhead)

**Phase 2.3 Exit Criteria:**
- [ ] 12-TET mode produces standard equal temperament
- [ ] JI mode produces audibly different intervals
- [ ] Pythagorean mode produces sharp thirds
- [ ] keyRoot parameter shifts tuning
- [ ] CPU usage < 35% with 96 oscillators

---

### Phase 2.4: Modulation, Filtering, Polish

#### Task 17: Implement global LFO
- **Files:** Source/PluginProcessor.h, Source/PluginProcessor.cpp
- **Depends on:** Phase 2.3 complete
- **Description:**
  - Add `juce::dsp::Oscillator<float>` for LFO (sine wave)
  - Initialize in prepareToPlay with sample rate
  - Atomic pointers for lfoRate, lfoDepth parameters
  - In processBlock: advance LFO, calculate modulated wavetablePos
  - Pass modulated position to all voices

#### Task 18: Add StateVariableTPTFilter
- **Files:** Source/PluginProcessor.h, Source/PluginProcessor.cpp
- **Depends on:** Task 17
- **Description:**
  - Add `juce::dsp::StateVariableTPTFilter<float>` member
  - Configure as low-pass, 12dB/octave
  - Initialize in prepareToPlay
  - Atomic pointer for filterCutoff parameter
  - Apply filter to mixed output (post-voice summing)

#### Task 19: Implement randomization system
- **Files:** Source/DSP/WavetableVoice.cpp
- **Depends on:** Task 17
- **Description:**
  - In startNote(): Apply randomization per chord voice
  - Inversion random: ±1 octave shift (based on inversionRandom %)
  - Timing random: Stagger voice start (0 to timingRandom ms)
  - Detune random: Gaussian random cents (±detuneRandom cents)
  - Use juce::Random for deterministic randomness

#### Task 20: Add master volume control
- **Files:** Source/PluginProcessor.cpp
- **Depends on:** Task 18
- **Description:**
  - Atomic pointer for masterVolume parameter
  - Apply as final gain stage after filter
  - Linear gain (0.0 to 1.26, where 1.0 = unity)

#### Task 21: Scale up and CPU profiling
- **Files:** Source/PluginProcessor.cpp
- **Depends on:** Tasks 17-20
- **Description:**
  - Ensure 8 polyphony × 12 chord voices = 96 oscillators work
  - Test voice stealing under load (play >8 notes rapidly)
  - Profile CPU at 48kHz sample rate
  - If CPU >80%: Apply fallback (reduce polyphony to 6)

#### Task 22: Validation - Phase 2.4 (Final)
- **Files:** None (testing only)
- **Depends on:** Tasks 17-21
- **Description:**
  - Test LFO modulates wavetable position (audible sweeping)
  - Test filter shapes brightness
  - Test randomization adds organic variation
  - Test master volume controls output level
  - **Critical:** CPU < 80% single core @ 48kHz
  - Test voice stealing (no pops/clicks)

**Phase 2.4 Exit Criteria:**
- [ ] LFO modulates wavetable position
- [ ] Filter shapes tonal brightness
- [ ] Randomization adds organic variation
- [ ] Master volume controls output level
- [ ] CPU usage < 80% with 96 oscillators @ 48kHz
- [ ] Voice stealing is glitch-free

---

## Files to Create/Modify

### New Files (7)
| File | Task | Description |
|------|------|-------------|
| Source/DSP/WavetableOscillator.h | 1 | Oscillator class header |
| Source/DSP/WavetableOscillator.cpp | 1 | Oscillator implementation |
| Source/DSP/WavetableVoice.h | 2 | Synth voice header |
| Source/DSP/WavetableVoice.cpp | 2 | Synth voice implementation |
| Source/DSP/WavetableSound.h | 3 | Sound class (simple) |
| Source/DSP/WavetableData.h | 5 | Wavetable data arrays |
| Source/DSP/ChordGenerator.h | 7 | Chord gen header |
| Source/DSP/ChordGenerator.cpp | 7 | Chord gen implementation |

### Modified Files (3)
| File | Tasks | Changes |
|------|-------|---------|
| Source/PluginProcessor.h | 4, 13, 17, 18 | Add tuning engine, LFO, filter members |
| Source/PluginProcessor.cpp | 4, 10, 14, 17-20 | Synthesiser setup, parameter connections, DSP chain |
| CMakeLists.txt | 12 | Add scala-tuning-engine module |

---

## Dependencies

### Task Dependency Graph

```
Phase 2.1:
  Task 1 (oscillator) ──┬── Task 2 (voice) ──┬── Task 4 (integrate)
  Task 3 (sound) ───────┘                    │
  Task 5 (data) ────────────────────────────┴── Task 6 (validate)

Phase 2.2:
  Task 7 (chord gen) ── Task 8 (distribution) ── Task 9 (multi-osc) ── Task 10 (params) ── Task 11 (validate)

Phase 2.3:
  Task 12 (module) ── Task 13 (init) ── Task 14 (params) ── Task 15 (freq) ── Task 16 (validate)

Phase 2.4:
  Task 17 (LFO) ──┬── Task 19 (random)
  Task 18 (filter)┴── Task 20 (volume) ── Task 21 (scale) ── Task 22 (validate)
```

### Module Dependencies

| Module | Version | Phase Added |
|--------|---------|-------------|
| scala-tuning-engine | v1.13.0 | Phase 2.3 (Task 12) |

### JUCE Components Used

- `juce::Synthesiser` / `juce::SynthesiserVoice` / `juce::SynthesiserSound`
- `juce::ADSR`
- `juce::dsp::Oscillator<float>` (LFO)
- `juce::dsp::StateVariableTPTFilter<float>`
- `juce::Random`

---

## Success Criteria (Stage 2 Complete)

- [ ] Complete audio synthesis working (wavetable + chords + tuning)
- [ ] All 15 DSP parameters functional and connected
- [ ] CPU usage < 80% single core with 96 oscillators (or fallback applied)
- [ ] Voice stealing glitch-free (no pops/clicks)
- [ ] No aliasing artifacts (< -60dB above 20kHz)
- [ ] Just intonation tuning audibly different from 12-TET
- [ ] LFO smoothly modulates wavetable position
- [ ] Filter shapes tonal brightness
- [ ] Randomization adds organic pad variation

---

## Fallback Strategies

### If CPU exceeds 80% (Phase 2.4):
1. **First:** Reduce polyphony to 6 (72 oscillators)
2. **Second:** Reduce max chord voices to 8 (64 oscillators)
3. **Third:** Dynamic voice allocation (fewer sub-voices for simple chords)

### If chord generation incorrect (Phase 2.2):
1. Use fixed chord tables instead of scale-degree analysis

### If aliasing present (Phase 2.1):
1. Add 2x oversampling to oscillator

---

## Execution Notes

- Execute phases sequentially (2.1 → 2.2 → 2.3 → 2.4)
- Validate at each phase checkpoint before proceeding
- Do not skip validation tasks (6, 11, 16, 22)
- Apply fallback strategies if validation fails
- Keep CPU profiling results in validation notes

---

## Next Step

Run `/plugin-execute O-IntonationPad 2-dsp` to begin Phase 2.1 implementation.
