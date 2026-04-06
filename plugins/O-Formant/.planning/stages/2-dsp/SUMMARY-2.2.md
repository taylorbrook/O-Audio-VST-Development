# Stage 2 DSP: Phase 2.2 Modulation & Expression - Summary

**Completed:** 2026-04-05
**Phase:** 2.2 of 3
**Result:** SUCCESS - pluginval strictness 5 passed

---

## What Was Built

3 DSP files created, 2 files modified. Modulation, consonant noise, and MPE expression:

**PitchGlide -> VibratoLFO -> F0 | ConsonantEngine -> mix with formant -> ADSR -> output**

### New DSP Components (Source/dsp/)

| File | Purpose |
|------|---------|
| VibratoLFO.h | Sine LFO with onset delay ramp, micro-jitter (+/-0.5% per cycle), double-precision phase |
| PitchGlide.h | One-pole exponential smoother for portamento, adapted from O-Prism GlideProcessor |
| ConsonantEngine.h | KLATT parallel noise: LP/HP crossfade (tone), sibilance BP resonance, auto-consonant plosive burst |

### Modified Files

| File | Changes |
|------|---------|
| FormantVoice.h | Added VibratoLFO, PitchGlide, ConsonantEngine members; MPE state (mpeBreathOffset, mpeVowelYOffset, noteVelocity); wasActive for glide detection |
| FormantVoice.cpp | Per-sample F0 chain (glide->vibrato->jitter); consonant branch after formant filtering; MPE pressure->breathiness, timbre->vowelY; auto-consonant burst on noteStarted |

## Parameters Connected (19 of 21)

### Newly connected in Phase 2.2 (8 + 2 MPE dimensions):
- vibratoRate, vibratoDepth, vibratoDelay -> VibratoLFO
- pitchGlide -> PitchGlide
- consonantLevel, consonantTone, sibilance -> ConsonantEngine
- autoConsonant -> ConsonantEngine plosive burst trigger
- MPE pressure -> breathiness offset (additive above knob baseline)
- MPE timbre -> vowelY offset (centered, clamped 0-1)

### Remaining for Phase 2.3: outputGain, stereoWidth

## Signal Flow (Updated)

```
F0 from MPENote
  -> PitchGlide (portamento smoother)
  -> VibratoLFO (pitch mod in cents + micro-jitter)
  -> Final F0 -> LFGlottalSource

GlottalSource -> AspirationNoise (breathiness + MPE pressure) -> FormantFilterBank -> formantOut

ConsonantEngine (noise LP/HP crossfade + sibilance + burst) -> consonantOut

Mix: formantOut + consonantOut
  -> ADSR envelope
  -> mono to both channels
```

## Validation

- Build: clean (no new warnings)
- pluginval: PASSED at strictness level 5
- NaN protection: inherited from Phase 2.1 (FormantBiquad + output guard), added consonantEngine.reset() in guard

## Requirements Addressed

| Requirement | Priority | Status |
|-------------|----------|--------|
| FUNC-05 | must | ConsonantEngine with LP/HP/sibilance filtering |
| FUNC-07 | should | MPE pressure->breathiness, timbre->vowelY |
| FUNC-09 | should | Auto-consonant burst with velocity scaling (15ms exponential decay) |
| FUNC-10 | should | VibratoLFO with onset delay and micro-jitter |
| FUNC-11 | nice | Per-voice pitch glide (exponential one-pole smoother) |
| DSP-08 | should | Consonant tone shaping (LP/HP crossfade) + sibilance (BP Q 2-10) |
