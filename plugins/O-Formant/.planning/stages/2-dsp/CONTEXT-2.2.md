# Stage 2: DSP - Phase 2.2 Context (Modulation & Expression)

## Discussion Summary

**Date:** 2026-04-05
**Prerequisite:** Phase 2.1 verified -- core vocal engine operational

## Phase 2.2 Scope

4 new DSP components + MPE integration into existing FormantVoice.

### 1. VibratoLFO
- Sine LFO with per-voice phase accumulator
- Parameters: vibratoRate (0.5-12 Hz), vibratoDepth (0-100 cents), vibratoDelay (0-2000ms)
- Onset delay: linear ramp from 0 to full depth over vibratoDelay ms after note-on
- **Micro-jitter enabled:** +/-0.5% random F0 perturbation per LFO cycle for naturalness
- Applied as: `f0 * pow(2.0, depthCents / 1200.0 * sin(phase))`

### 2. PitchGlide
- **Per-voice glide** (deviation from architecture's "monophonic last-note")
- Exponential one-pole smoother on target frequency
- Time constant from pitchGlide parameter (0-1000ms)
- When pitchGlide == 0, frequency snaps immediately
- On voice steal/reassign, glides from old pitch to new pitch

### 3. ConsonantEngine
- KLATT-derived parallel noise branch
- Noise source: white noise from juce::Random (per-voice, decorrelated)
- Tone control: consonantTone 0.0 = LP ~2kHz (dark /f/), 1.0 = HP ~6kHz (bright /s/)
  - Implemented as crossfade between LP and HP filtered noise
- Sibilance: peaked resonance at 4-8kHz (bandpass, Q from sibilance param)
- **Auto-consonant plosive burst:**
  - Fires on every noteStarted() when autoConsonant enabled
  - **Velocity scales burst amplitude** (soft = subtle, hard = pronounced)
  - 10-25ms noise burst with falling amplitude envelope
  - Burst spectrum shaped by consonantTone
  - Crossfades into vowel ADSR attack
- Output: `final = formantOut + consonantLevel * consonantOut`
- Early-out: skip entire branch when consonantLevel == 0 AND autoConsonant off

### 4. MPE Integration
- **Pressure -> Breathiness (additive):**
  - Effective breathiness = knob_value + pressure * (1.0 - knob_value)
  - Knob sets baseline, pressure adds per-note expression up to 1.0
  - Updated in notePressureChanged() callback
- **Slide -> Vowel Y offset:**
  - Effective vowelY = knob_value + timbre * range (clamped 0-1)
  - Updated in noteTimbreChanged() callback
- **Velocity -> Attack character:**
  - Scales auto-consonant burst amplitude
  - Optionally shortens attack time (velocity 1.0 = fast attack)

## Integration Points (FormantVoice)

- New member variables: VibratoLFO, PitchGlide, ConsonantEngine
- noteStarted(): init vibrato delay ramp, trigger plosive burst, store velocity
- notePressureChanged(): update per-voice breathiness offset
- noteTimbreChanged(): update per-voice vowelY offset
- renderNextBlock(): insert vibrato + glide before glottalSource.setFrequency(), add consonant branch after formant filtering

## Signal Flow Update

```
F0 from MPENote
  -> PitchGlide (portamento)
  -> VibratoLFO (pitch mod + micro-jitter)
  -> Final F0 -> glottalSource

Source (glottal + aspiration)
  -> FormantFilterBank -> formantOut
  
ConsonantEngine (noise + tone + sibilance + burst)
  -> consonantOut

Mix: formantOut + consonantLevel * consonantOut
  -> ADSR envelope
  -> voice output
```

## Files to Create
- `Source/dsp/VibratoLFO.h`
- `Source/dsp/PitchGlide.h`
- `Source/dsp/ConsonantEngine.h`

## Files to Modify
- `Source/FormantVoice.h` (add new DSP members, per-voice state)
- `Source/FormantVoice.cpp` (integrate all 4 components)
- `Source/CMakeLists.txt` (add new source files if needed)

## Requirements Addressed
- FUNC-05: Consonant noise injection (must)
- FUNC-07: MPE support (should)
- FUNC-09: Auto-consonant plosive burst (should)
- FUNC-10: Vibrato LFO (should)
- FUNC-11: Portamento/pitch glide (nice)
- DSP-08: Consonant tone shaping + sibilance (should)

## Next Phase
Ready for: **research** phase
