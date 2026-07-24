# Stage 2: DSP — Execute SUMMARY

**Plugin:** O-ReverseDelay
**Stage:** 2-dsp
**Phase:** execute — complete
**Date:** 2026-07-24
**Agent:** dsp-agent (3 invocations: Phase 2.1, Phase 2.2, Phase 2.3 Tasks 8–9 + Task 10)

## Result

All 11 PLAN.md tasks complete. Full DSP engine implemented: granular reverse smear
over a 3.5 s capture ring (reverse read law D+2n), damped tanh-stable feedback
through the shared capture buffer, tempo sync with COMPAT-02 fallback, per-grain
width spread, equal-power mix. Render harness enforces every acceptance criterion
with hard exit codes — **33 checks, ALL PASS**. pluginval strictness-10 ×3 clean on
installed VST3 + AU.

## Commits

| Commit | Content |
|--------|---------|
| 91c673f | fix: run-gate.sh resolves juce_add_plugin target (gate was blocked by folder-name assumption; target is `OuariconReverseDelay`) |
| 81603a8 | Phase 2.1 — core reverse wet path, harness probes 0+A–E |
| 0ae40e5 | Phase 2.2 — feedback loop + damping + stability, probes F–H |
| (this commit) | Phase 2.3 — tempo sync + width + probes I–M, STATUS/SUMMARY |

## Files

**Created:** `Source/dsp/CaptureBuffer.h` (int64 absolute-position ring, monoSum),
`Source/dsp/WindowLut.h` (Hann 2048 lerp), `Source/dsp/ReverseGrain.h` (POD + 32-pool),
`Source/dsp/GrainScheduler.h` (free countdown), `tests/render-harness/{CMakeLists.txt,main.cpp}`
**Modified:** `Source/PluginProcessor.h/.cpp` (full processBlock), `CMakeLists.txt` (OUARICON_BUILD_TESTS)

## Harness Results (final run, fs=48000 block=512)

- Probes A/B: reversed-ramp slope −0.000014731 (exact), corrRev 1.000/corrFwd −1.000; impulse bloom peak centered
- Probes C/D/E: clicks maxStep 0.0089 < 0.0137; density flatness spread **0.061 dB** (±1 dB budget); single-generation leak 0.0
- Probe F: damping per generation — centroid 10008→4136 Hz, lowFrac 0.0067→0.0019
- Probe G: 60 s @ feedback=100 — peak 0.2404, tail 0.0, all finite
- Probe H: cutoff sweeps click-free
- Probe I: sync 120 BPM 1/4 → latency 24000 samples (exact); free 150/500/1200 ms exact
- Probe J: no-playhead + null-BPM fallback both land at free D (COMPAT-02)
- Probe K: width=0 corr 1.0000 side 0.0; width=100 corr 0.3449
- Probe L: mono→stereo identity Δ0.0000 dB; dry duplication exact
- Probe M: all-10-param sweeps + Sync↔Free mode switch — click-free, finite, bounded

## Validation

- Build warning-clean (float-equal guards switched to `juce::exactlyEqual`)
- `./scripts/build-and-install.sh O-ReverseDelay` — installed `O-ReverseDelay-dev.{vst3,component}`
- pluginval strictness 10: VST3 3/3 PASS, AU 3/3 PASS (latent-NaN class check)
- `auval -a` lists `aufx ORvD OuDv`

## Notable Deviations (agent-documented, probe design only — DSP matches contract)

- Probe I uses energy-centroid latency (bloom centroid = T+D+G sub-sample exact) — raw onset jitters by 2·interval > the ±1-block budget on any legal grain size; onset kept as a sanity window
- Probe D uses seeded noise, not a sine — at 220 Hz several density steps make the copy grid fully coherent (+4.3 dB), measuring coherence instead of compensation
- Probe M two-tier click detector: smooth params kStepFactor 3.0; latched/loop params 8.0 + peak + finite (latched-param sweeps legitimately move grain positions)
- Tuned-then-frozen constants (D5): probe C kStepFactor 1.75, probe H 2.5, probe M 3.0/8.0, kPanBias 0.5

## Open Item for Verify — D6 Audition Finding

The contract topology (feedback tap post-pan + mono-sum grain source) has an
inherent **~−7.3 dB/generation loss at feedback=100 before damping** (−4.3 dB
Hann² duty from RMS-flat compensation, −3 dB pan→monoSum round trip). Probe G
passes (bounded, persistent wash), but 100% feedback decays ~7 dB per D rather
than approaching self-sustain. If the Standalone audition finds the wash too
short, the fix is a **single makeup constant at the feedback tap** — do not
relitigate the topology.

**D6 Standalone audition is outstanding** — required before/at verify: smear
quality, feedback wash, width character.
