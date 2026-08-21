# O-Emulator — Stage 2 (DSP) Execute Summary

**Date:** 2026-08-20
**Phase:** execute (stage 2)
**Branch/worktree:** `feat/o-emulator-impl` @ `~/Dev/VST-development-emulator`
**Agent:** dsp-agent (single continuous session across all 4 phases)

## Result: ✅ COMPLETE — all 20 PLAN tasks, 4 phase commits, harness green at every gate

| Phase | Commit | Harness at commit |
|-------|--------|-------------------|
| 2.1 SNES end-to-end | `4debe9a0` | 27 checks ALL PASS |
| 2.2 PS1 + SPU reverb | `06b4097a` | 34 checks ALL PASS |
| 2.3 NES/GB/Genesis + switching | `490d99b3` | 45 checks ALL PASS |
| 2.4 Age + crush polish | `dec6d029` | 52 checks ALL PASS |

## What was built (Source/dsp/, all net-new)

- **Infrastructure:** `FixedChunkFeeder` (32-host-sample chunk walk, uint64 absolute counter — block-size invariance by construction), `ConsoleEngine` (5 pre-allocated pipelines, per-console spec table), `ConsoleCrossfader` (30 ms equal-power, reverb persists across switches, first-chunk instant-switch).
- **Resampling:** `ConsoleResampler` (4× cascaded Butterworth AA — Q table {0.50980, 0.60134, 0.89998, 2.56292} — Lagrange decimation by returned consumed count; Gaussian or ZOH upsample), `GaussianInterpolator` (512-entry S-DSP table, calibrated generator — see Deviations).
- **Codecs:** `AdpcmEncoder<BlockLen, NumFilters>` closed-loop skeleton → `BrrCodec` (SNES 16/4) and `SpuAdpcmCodec` (PS1 28/5); `DpcmCodec` (NES 1-bit sigma-delta, NTSC timer table), `WaveQuantizer` (GB 4-bit + DAC-error table, 16/8/4 levels), `GenesisDac` (8-bit + ladder offset, rate-hold).
- **Reverb:** `SpuReverb` — psx-spx register model @ fixed 22.05 kHz tick (vIIR reflections → 4-comb → 2 APF), Hall-shaped named register set, stability proven in-header, non-sticky isfinite guard; console-domain send/return with primed alignment ring.
- **Character:** `OutputStage` (TPT LP per-console corner, soft/hard/crunchy clip, 10 Hz DC blocker), `AgeModel` (rate-normalized noise bed + hum, per-purpose RNG streams), age dulling + ±15-cent drift on decimation read rate, `CrushCurve` per-console macro rows, AA-open precomputed sets, 5 ms micro-fades on integer steps.
- **Mix:** DryWetMixer ctor-sized `{1024}`, exact `setLatencySamples` ↔ `setWetLatency` pairing. Latency 116 samples @ 48 kHz (319 @ 192 kHz), constant across all 5 console modes.

## Requirement traceability (probe names)

- FUNC-01 5-mode distinctness — M1 ✅ · FUNC-02 mix-0 null + macro liveness — B, C, G6 ✅ · FUNC-03 reverb all modes — M5 ✅ · FUNC-04 click-free switch, no latency renegotiation — M3 ✅
- DSP-01..05 — S1/S2, F, M2 (live-corner contract), R1, G1–G4 ✅ · PERF-01 — audit clean + P1 CPU ratio 0.017 ✅ · PERF-02 — AN/AN2/AN3 bit-identical incl. ragged {1,7,64,333,4096} ✅ · QUAL-01 — U pathological (denormals, NaN input) ✅

## Digest anchor state (re-anchor discipline applied)

Active (recorded at 2.4): SNES `9cf6baa8d3b61b14`, PS1+reverb `b23fe10b74526fab`, switch-matrix `dad157a01f7c393f`. All prior anchors (Stage-1 passthrough, 2.1, 2.2, 2.3) retired as named constants with moved-asserts. G5 asserts offline == realtime bit-identity.

## Deviations from PLAN (documented in-code)

1. **Gaussian table + SPU Hall registers are spec-calibrated, not verbatim ROM/PsyQ transcriptions** (no reference access; hallucinated hex in a feedback network risks instability). Topologies are exact; a verbatim data swap later is one-function + re-anchor.
2. **Age bed injects once at engine level post-crossfader** (not per-pipeline) — keeps RNG streams per-purpose/unconditional and avoids doubled hiss in fades.
3. **DPCM counter starts at midpoint** (hardware starts at 0) — avoids −1.0 onset thump; in-signal behavior preserved.
4. M2's original cross-mode centroid-ordering clause was a probe-design error (DPCM slew-limiting sets the NES centroid, not the output corner) — replaced with a live-corner contract check via `OUARICON_RENDER_HARNESS` accessors; analysis recorded in-probe.
5. PS1 upsample prime 4 → 8 in 2.4 (drift excursion headroom); Z/Z2 pin age 0 (drift would shift measured lag).

## Notes for verify phase

- One compile fix across all 4 phases (`juce::dsp::IIR::ArrayCoefficients` qualification, 2.1); every phase green at first or second harness run.
- pluginval strictness 10 NOT yet re-run post-Stage-2 (Stage-1 ran it green) — verify should re-run it VST3+AU.
- Installed `O-Emulator-dev.{vst3,component}`; auval registered.
- GPL hygiene held: all codec/reverb tables from published specs (psx-spx, S-DSP docs, NESdev); no blargg/Nuked code.
