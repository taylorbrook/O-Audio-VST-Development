# Stage 2: DSP - Phase 2.2 Modulation & Expression - Verification

## Verification Date

2026-04-05

## Goal-Backward Analysis

### Original Goals (from CONTEXT-2.2.md)

1. VibratoLFO: per-voice sine LFO with onset delay and micro-jitter
2. PitchGlide: per-voice exponential one-pole smoother for portamento
3. ConsonantEngine: KLATT-derived parallel noise branch (LP/HP crossfade, sibilance, plosive burst)
4. MPE integration: pressure->breathiness, timbre->vowelY, velocity->burst amplitude
5. Connect remaining 8 parameters + 2 MPE dimensions (19/21 total)

### Deliverables (from SUMMARY-2.2.md)

1. VibratoLFO.h: sine LFO, double-precision phase, onset delay ramp, +/-0.5% micro-jitter per cycle
2. PitchGlide.h: one-pole exponential smoother adapted from O-Prism GlideProcessor
3. ConsonantEngine.h: 3x FormantBiquad (LP 2kHz/HP 6kHz/BP 5.5kHz), tone crossfade, 15ms plosive burst
4. FormantVoice: per-sample F0 chain (glide->vibrato->jitter), consonant mix, MPE callbacks
5. 19 of 21 parameters connected (remaining: outputGain, stereoWidth for Phase 2.3)

### Goal Achievement

| Goal | Status | Evidence |
|------|--------|----------|
| VibratoLFO | ✅ Achieved | VibratoLFO.h -- sine LFO with onset delay ramp (linear counter), micro-jitter via zero-crossing detection |
| PitchGlide | ✅ Achieved | PitchGlide.h -- one-pole smoother, `coeff = exp(-1/(time*sr))`, snapTo/setTarget for first-note/glide |
| ConsonantEngine | ✅ Achieved | ConsonantEngine.h -- 3x FormantBiquad, LP/HP crossfade by cachedTone, sibilance BP Q 2-10, 15ms plosive burst |
| MPE integration | ✅ Achieved | FormantVoice.cpp:148-163 -- notePressureChanged/noteTimbreChanged with asUnsignedFloat, additive breath formula |
| 19/21 params connected | ✅ Achieved | vibratoRate/Depth/Delay, pitchGlide, consonantLevel/Tone, sibilance, autoConsonant all wired |

## Requirements Verification

**Stage:** 2-dsp (Phase 2.2)
**Requirements for this phase:** 6 (previously deferred from Phase 2.1)

| Requirement | Priority | Status | Acceptance Criteria |
|-------------|----------|--------|---------------------|
| FUNC-05: Consonant noise injection | must | ✅ Complete | ConsonantEngine with LP/HP/sibilance filtering, level scaling, early-out optimization |
| FUNC-07: MPE support | should | ✅ Complete | pressure->breathiness (additive), timbre->vowelY (offset), velocity->burst; callbacks verified from JUCE source |
| FUNC-09: Auto-consonant plosive burst | should | ✅ Complete | triggerBurst(velocity), 15ms exponential decay (exp(-5*progress)), velocity-scaled amplitude |
| FUNC-10: Vibrato LFO | should | ✅ Complete | Sine LFO with onset delay ramp, per-cycle micro-jitter, vibratoRate/Depth/Delay params connected |
| FUNC-11: Portamento/pitch glide | nice | ✅ Complete | Per-voice exponential one-pole smoother, wasActive glide detection, snapTo for first note |
| DSP-08: Consonant tone/sibilance | should | ✅ Complete | LP 2kHz / HP 6kHz crossfade by consonantTone, sibilance BP 5.5kHz Q 2-10 |

**Requirements Summary:**
- ✅ Complete: 6
- ⚠️ Partial: 0
- ⏸️ Deferred: 0
- ❌ Failed: 0

## Cumulative Requirements Status (Phase 2.1 + 2.2)

| Status | Count | Requirements |
|--------|-------|-------------|
| ✅ Complete | 20 | FUNC-01-04, FUNC-05-11, DSP-01-05, DSP-07-08, PERF-01-02, QUAL-01 |
| ⚠️ Partial | 1 | DSP-06 (XY smoothing: block-rate 32-sample updates, no explicit 30ms smoother) |
| ⏸️ Deferred | 1 | FUNC-08 already complete (legacy mode), FUNC-12 stage-4 |
| Not yet verified | 4 | UI-01, UI-02, UI-03 (stage-3), FUNC-12, COMPAT-01 re-verify (stage-4) |

## Automated Checks

| Check | Result | Notes |
|-------|--------|-------|
| Build (VST3 + AU) | ✅ Pass | ninja: no work to do (already compiled, clean) |
| pluginval VST3 (level 5) | ✅ Pass | All tests passed, 0 failures |
| auval AU validation | ✅ Pass | All render tests, parameter setting/ramping, MIDI test passed |
| Real-time safety (code review) | ✅ Pass | No heap allocations in renderNextBlock or any DSP getNextSample paths |
| NaN/Inf protection | ✅ Pass | Existing FormantBiquad guard + FormantVoice output guard; consonantEngine.reset() added to NaN recovery |

## Code Review: Implementation Quality

### Signal Flow (verified from FormantVoice.cpp:236-257)
```
F0 -> PitchGlide -> VibratoLFO (cents + jitter) -> glottalSource.setFrequency
GlottalSource -> AspirationNoise -> FormantFilterBank -> formantOut
ConsonantEngine (noise + burst) -> consonantOut
Mix: formantOut + consonantOut -> ADSR envelope -> output
```
Matches PLAN-2.2.md specification exactly.

### Per-Sample vs Block-Rate (correct)
- **Per-sample:** F0 chain (pitchGlide, vibratoLFO, glottalSource), consonant processing, ADSR
- **Block-rate (32 samples):** vowel morph, formant coefficients, consonant coefficients
- **Per-block:** ADSR params, Rd, vibrato rate/depth, consonant level/tone/sibilance, breathiness

### Pattern Reuse
- FormantBiquad reused for ConsonantEngine filters (LP/HP/sibilance) ✅
- Per-voice Random with unique seed (voiceIndex * 37 + 23, different from AspirationNoise) ✅
- O-Prism GlideProcessor pattern adapted for PitchGlide ✅
- LFGlottalSource double-precision phase pattern reused in VibratoLFO ✅

### Edge Case Handling
- PitchGlide: wasActive flag prevents glide from 440Hz on first note ✅
- PitchGlide: coeff=0 early-out when glide disabled ✅
- VibratoLFO: delaySamples=0 guarded with jmax(1, delaySamples) ✅
- ConsonantEngine: early-out when consonantLevel < 0.001 AND no active burst ✅
- ConsonantEngine: burstTotalSamples guarded with jmax(1, ...) ✅
- Nyquist clamping on consonant filter frequencies ✅

## Human Verification

- [ ] Play sustained note -- hear vibrato develop after delay period
- [ ] Sweep vibratoRate 0.5->12 Hz -- hear speed change
- [ ] Sweep vibratoDepth 0->100 cents -- hear depth increase
- [ ] Compare sustained note with Phase 2.1 -- hear micro-jitter liveliness
- [ ] Play rapid notes with pitchGlide > 0 -- hear smooth pitch transition
- [ ] Set pitchGlide = 0 -- confirm instant pitch changes
- [ ] Sweep consonantLevel 0->1 -- hear noise layer increase
- [ ] Sweep consonantTone 0->1 -- hear dark (/f/) to bright (/s/) transition
- [ ] Sweep sibilance 0->1 -- hear 4-8kHz resonance sharpen
- [ ] Enable autoConsonant, play notes -- hear burst transient at onset
- [ ] Play soft vs hard velocity with autoConsonant -- hear burst amplitude difference
- [ ] Send MIDI aftertouch -- hear breathiness increase
- [ ] Send CC74 -- hear vowel Y position shift
- [ ] Play 16-note chord with all features enabled -- verify no dropouts

## Issues Found

None. All planned features implemented as specified.

## Stage Verdict

**Status:** ✅ VERIFIED (Phase 2.2 of 3)

**Ready for Phase 2.3:** Yes

**Phase 2.2 delivers:**
- Per-sample pitch modulation chain (glide -> vibrato -> jitter -> F0)
- KLATT-derived consonant noise engine with tone/sibilance shaping and plosive burst
- MPE expression mapping (pressure, timbre, velocity)
- 19 of 21 parameters connected and functional

**Remaining for Phase 2.3:**
- outputGain (juce::dsp::Gain with dB control)
- stereoWidth (per-voice pan by pitch)
- Parameter smoothing verification
- Performance optimization (skip consonant when level=0, early-out silent voices)
- Filter state reset on voice release
