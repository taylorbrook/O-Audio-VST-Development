# Stage 2: DSP Implementation - Execution Plan

**Phase:** Plan
**Date:** 2026-02-01
**Status:** Ready for Execute

## Overview

Implement modal synthesis bell engine with 8-voice polyphony and ensemble voicing.

## Phase 2.1: Core Modal Synthesis

### Tasks

1. **Create BellSound.h**
   - Simple `juce::SynthesiserSound` subclass
   - `appliesToNote()` returns true for all
   - `appliesToChannel()` returns true for all

2. **Create BellVoice.h**
   - Define `NUM_PARTIALS = 8`
   - Define partial ratio tables (harmonic, bell, gamelan)
   - Define `ModalPartial` struct (phase, amplitude, frequency, envelope)
   - Define `StrikeExciter` struct (noise burst generator)
   - Declare all parameter setters
   - Declare `SynthesiserVoice` interface methods

3. **Create BellVoice.cpp - Core**
   - Implement `canPlaySound()`
   - Implement `startNote()`:
     - Calculate base frequency from MIDI note
     - Calculate partial frequencies using inharmonicity interpolation
     - Set partial amplitudes based on strike position
     - Configure exciter (mallet hardness → filter coefficient)
     - Reset all state
   - Implement `stopNote()`:
     - If allowTailOff: let envelopes decay naturally
     - Else: clearCurrentNote() immediately
   - Implement `renderNextBlock()`:
     - Generate excitation sample
     - Process all partials (sine oscillators with envelopes)
     - Sum partials and write to buffer

4. **Modify PluginProcessor.cpp**
   - Add `juce::Synthesiser synth` member
   - In constructor: add 8 voices and 1 sound
   - In `prepareToPlay()`: set sample rate, prepare voices
   - In `processBlock()`: clear buffer, render synth

### Success Criteria (Phase 2.1)
- [ ] Plugin produces sound from MIDI notes
- [ ] Bell-like timbre with inharmonic partials
- [ ] damping parameter affects decay time
- [ ] inharmonicity parameter morphs partial ratios
- [ ] Single voice plays cleanly

---

## Phase 2.2: Polyphony + Strike Dynamics

### Tasks

1. **Voice Stealing**
   - Implement oldest-first stealing in Synthesiser
   - Add 5ms fade-out on stolen voices

2. **Strike Position Processing**
   - Implement comb filter algorithm for partials
   - `strikePosition` affects partial amplitude distribution

3. **Mallet Hardness**
   - Implement spectral tilt (high-shelf effect)
   - Harder = brighter attack + more upper partials

4. **Velocity Curves**
   - Implement linear, exponential, logarithmic curves
   - Map MIDI velocity to amplitude and brightness

5. **Strike Transient**
   - Implement `StrikeExciter` with filtered noise burst
   - `strikeNoiseChar` selects filter type (Click/Thud/Ping)
   - Duration: 2-25ms based on mallet hardness

6. **Parameter Integration**
   - Cache APVTS parameter pointers in prepareToPlay
   - Pass parameters to voices in processBlock

### Success Criteria (Phase 2.2)
- [ ] 8-voice polyphony with smooth voice stealing
- [ ] strikePosition audibly changes timbre
- [ ] malletHardness affects brightness
- [ ] velocityCurve options work correctly
- [ ] Strike transient audible on note attack

---

## Phase 2.3: Ensemble + Advanced Features

### Tasks

1. **Unison Voicing**
   - Implement per-voice unison engine (1-4 copies)
   - Calculate symmetric detune distribution
   - Sum unison outputs per voice

2. **Octave Layering**
   - Add sub-octave processing (freq × 0.5)
   - Add upper-octave processing (freq × 2.0)
   - Mix based on blend parameters

3. **Stereo Spread**
   - Implement pan calculation for unison voices
   - Apply panning in renderNextBlock

4. **Material Morphing**
   - Implement decay multipliers for Bronze/Steel/Glass/Crystal
   - Apply material to brightness filter cutoff

5. **Sympathetic Resonance**
   - Track active voice frequencies
   - On note-on, check harmonic relationships
   - Apply small excitation to coupled voices

6. **Output Gain**
   - Apply outputGain (dB) to final output
   - Prevent clipping with soft limiter if needed

7. **Quality Setting**
   - Map quality (Low/Med/High) to partial count (4/6/8)
   - Dynamically reduce processing when Low selected

### Success Criteria (Phase 2.3)
- [ ] Unison creates chorus/thickness effect
- [ ] Octave blend adds depth (sub) and shimmer (oct)
- [ ] Stereo spread widens the sound
- [ ] Material audibly changes character
- [ ] Sympathetic resonance adds realism (subtle)
- [ ] CPU stays under 60% worst case

---

## File Changes Summary

### New Files
- `Source/BellSound.h` (~20 lines)
- `Source/BellVoice.h` (~150 lines)
- `Source/BellVoice.cpp` (~400 lines)

### Modified Files
- `Source/PluginProcessor.h` - Add Synthesiser member, parameter pointers
- `Source/PluginProcessor.cpp` - Voice setup, parameter passing
- `CMakeLists.txt` - Add new source files

---

## Risk Mitigation

### CPU Risk
- Start with 8 partials, can reduce via quality setting
- Profile after Phase 2.1 to catch issues early
- Use `juce::ScopedNoDenormals` everywhere

### Voice Stealing Clicks
- 5ms fade-out is industry standard
- If still clicking, increase to 10ms

### Ensemble CPU Multiplication
- Default unison to 1 (user opts in)
- Disable octave layers when blend = 0%

---

## Commit Strategy

- **After Phase 2.1:** `feat(O-Bells): implement core modal synthesis engine`
- **After Phase 2.2:** `feat(O-Bells): add polyphony and strike dynamics`
- **After Phase 2.3:** `feat(O-Bells): complete ensemble voicing and advanced features`

---

*Plan created: 2026-02-01*
*Ready for: Execute phase*
