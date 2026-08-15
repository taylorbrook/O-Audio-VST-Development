# Stage 2: DSP - Context

## Discussion Summary

**Date:** 2026-08-15
**Participants:** User, Claude

## Requirements Confirmed

- Stage 2 implements all 9 DSP components across ROADMAP.md's 5 phases (2.1 Engine Core + Tape →
  2.2 CD Skip + Vinyl → 2.3 Packet Loss → 2.4 Crush + Quant → 2.5 Codec), each phase = git commit
  + harness probes green.
- Requirements in scope: FUNC-01..04, DSP-01..08, PERF-01, QUAL-01/02 (per REQUIREMENTS.md
  traceability; COMPAT-01 already verified at Stage 1).
- ARCHITECTURE.md is the BINDING contract — chain MediaPlayer → Packet → Codec → Crush → mix,
  one shared ring + clocked stochastic read heads, 8 seeded RNG streams, parameter-spec.md IDs
  locked (31 params already live in APVTS from Stage 1).
- Offline DSP render harness is the correctness gate from Phase 2.1 day one (repo Stage-2 pattern),
  including the 512-vs-4096 bit-identity probe and seeded-determinism probe.

## Constraints Identified

- Foundation shell is verified bit-transparent with latency 0; Phase 2.1 introduces the 20 ms
  latency scheme — FUNC-02 probe becomes "bit-transparent minus reported latency."
- RSBrokenMedia is GPL-3.0: patterns only, zero code copying (GPL firewall documented in
  ARCHITECTURE.md). Airwindows DeRez is MIT — code adaptation permitted.
- libgsm license file must be vendored and recorded; compile as separate C static-lib target with
  relaxed warnings (MSVC C89 quirks).
- Determinism discipline: RNG consumed only at ticks/packets, one stream per subsystem, streams
  reseeded in prepareToPlay + on SEED change; split-block processing at tick offsets.
- Ring span static_assert must cover max revolution (1.8 s) + ramp + safety headroom.
- All read-head position changes route through the single `clampAndScheduleJump()` choke point;
  1–5 ms crossfades unless HARD_EDGES.

## Approach Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| GSM codec (Phase 2.5) | Real vendored libgsm | Static lib, relaxed warnings; harness-verify a 160-sample frame round trip BEFORE integration. Fallback to μ-law+extra-decimation approximation only if the harness gate fails — fallback decision made inside Phase 2.5, not deferred to v1.1 planning. CODEC_MODE keeps ≥2 choices either way. |
| Latency scheme | Constant 20 ms all modes | `setLatencySamples(ceil(0.020·fs))` once in prepareToPlay; every wet path delay-aligned; `DryWetMixer::setWetLatency` aligns dry. FUNC-02 provable in every mode; no dynamic renegotiation on CODEC_MODE automation. |
| CPU budget | ≤15% single core @ 48 kHz worst case | Matches architecture estimate (all families + GSM). PERF-01 measured in harness at Phase 2.5. No forced interpolation downgrades. |
| Substitute concealment fallback | Auto-degrade to Decay on low AMDF confidence (built-in) | Per ARCHITECTURE.md; if Substitute ends up aliased to Decay entirely, DSP-04's "4 audibly distinct modes" is re-scoped at verify — flagged, not silently passed. |
| Phase order | 2.1 → 2.5 as ROADMAP.md | Infrastructure proven with Tape first; codec last isolates the vendoring risk behind a proven engine. |

## Open Questions

- None blocking. Research phase should nail down: libgsm CMake integration specifics (target
  setup, MSVC flags), exact delay-trim bookkeeping between the 8 kHz codec grid and host-fs
  sample counts (the "subtle off-by-frames" risk), and the O-Polystutter/O-ReverseDelay reuse
  points (CaptureBuffer absolute-index ring, varispeed read + anti-click crossfade stack,
  updateBeatSync port).

## Next Phase

Ready for: research phase
