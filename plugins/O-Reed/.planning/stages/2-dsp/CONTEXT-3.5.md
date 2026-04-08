# Stage 2: DSP Phase 3.5 - Context

## Discussion Summary

**Date:** 2026-04-05
**Participants:** User, Claude

## Requirements Confirmed

- **Oversampling:** Per-voice pattern (O-Bowed), 2x default / 4x option via APVTS choice param
- **4x mode:** Same polynomial solver, no Newton-Raphson. Higher oversampling factor only.
- **Instrument morph:** Deferred to Stage 3 (GUI). DSP already responds to all parameter changes smoothly — morph is a UI-side macro that sweeps APVTS values.
- **Tuning:** Standard TuningEngine integration (Scala/MTS-ESP/12TET). O-Bowed pattern: setMasterTune + setMode in processBlock, voices use tuningEngine.getFrequencyForNote() instead of getFrequencyInHertz()
- **MPE:** Wire remaining handlers — timbre/slide -> embouchure blend (same pattern as pressure -> breath). Pitchbend already incorporated via getFrequencyInHertz(), just needs tuning-aware replacement.
- **Optimization:** Measurement + targeted fixes only. Profile CPU per voice, document hotspots, fix obvious issues. No speculative SIMD or lookup table rewrites.
- **Latency reporting:** setLatencySamples() in prepareToPlay using oversampling.getLatencyInSamples()

## Constraints Identified

- Per-voice oversampling means 8 Oversampling instances in poly mode (acceptable — O-Bowed proves this)
- Oversampling choice change requires re-calling prepareToPlay (or lazy re-init in renderNextBlock)
- TuningEngine frequency lookup must replace ALL getFrequencyInHertz() calls in voice (noteStarted, legato, renderNextBlock)
- instrumentPreset param pointer is cached but NOT wired to DSP — remains unwired (deferred to GUI morph)

## Approach Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Oversampling architecture | Per-voice | Proven O-Bowed pattern, simple, reed instruments are low-poly |
| 4x solver | Same polynomial (no NR) | Current solver stable through Phase 3.4, NR adds complexity for marginal gain |
| Instrument morph | Defer to GUI | DSP already handles smooth param changes; morph is a UI macro |
| Tuning integration | O-Bowed pattern | TuningEngine module already linked, proven API |
| MPE completion | Wire timbre->embouchure, use tuning-aware freq | Stubs already exist, straightforward |
| Optimization scope | Measure + targeted fixes | Don't over-optimize before GUI stage |

## Parameters Activated This Phase

| Parameter | Wiring |
|-----------|--------|
| referencePitch | -> tuningEngine.setMasterTune() |
| tuningSystem | -> tuningEngine.setMode() |
| oversampling | -> per-voice Oversampling factor (2x=index 0, 4x=index 1) |
| instrumentPreset | NOT wired (deferred to GUI morph system) |

**Total active after Phase 3.5:** 33 of 35 (instrumentPreset deferred, outputGain already active)

## Implementation Components

1. **Per-voice juce::dsp::Oversampling<float>** — 1 channel, factor from APVTS choice (1=2x, 2=4x), halfband polyphase IIR
2. **TuningEngine wiring in PluginProcessor::processBlock** — read referencePitch + tuningSystem, call setMasterTune/setMode
3. **Voice frequency lookup** — replace getFrequencyInHertz() with tuningEngine->getFrequencyForNote(midiNote) + pitchbend offset
4. **MPE timbre -> embouchure** — same blend pattern as pressure -> breath in notePressureChanged
5. **Latency reporting** — setLatencySamples(oversampling.getLatencyInSamples()) in prepareToPlay
6. **CPU profiling** — measure per-voice cost at 2x and 4x, document in SUMMARY

## Open Questions

None. All decisions resolved.

## Next Phase

Ready for: research phase
