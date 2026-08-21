# Stage 2: DSP - Context

## Discussion Summary

**Date:** 2026-08-20
**Participants:** User, Claude

## Sequencing Note (deviation)

This discuss phase ran **ahead of sequence**: Stage 1 (Foundation) has no artifacts yet
(`stages/1-foundation/` absent, no VERIFICATION.md), and the UI mockup + parameter-spec.md
finalization listed in STATUS.md are still pending. The DSP context below does not depend on
the mockup, so the discussion is valid — but **Stage 2 execution must not start until Stage 1
is complete and verified.** Re-check this note at `/plugin-plan O-Emulator 2-dsp` time.

## Requirements Confirmed

- Stage 2 implements ARCHITECTURE.md exactly (immutable contract): 5 codec engines, console-domain
  resampling, SPU reverb @ 22.05 kHz, output-stage models, age model, crush macro, latency-compensated
  mix, click-safe console crossfade, fixed-chunk block-size-invariant infrastructure.
- Four DSP phases per ROADMAP.md, in order: 2.1 SNES end-to-end → 2.2 PS1 + SPU reverb →
  2.3 NES/GB/Genesis + switching → 2.4 Age + crush polish. Git commit per phase.
- Performance target: < 10% single core @ 48 kHz (not a gating concern; correctness/invariance are).
- Edge cases already specified in ARCHITECTURE.md: pathological input probe (silence, DC, full-scale
  noise, denormals), NES DC offset blocking, reverb isfinite guard with non-sticky reset,
  host rates 44.1–192 kHz, block sizes 64/512/4096 digest-identical.

## Constraints Identified

- **Stage 1 must complete first** (see Sequencing Note). Mockup finalization of parameter-spec.md
  precedes Stage 1 per STATUS.md.
- GPL hygiene: implement codecs/tables from published specs (psx-spx, S-DSP docs, NESdev); never
  port blargg/Nuked GPL code into this AGPL-3.0 repo.
- Constant worst-case latency across console modes; `setLatencySamples(N)` figure mirrored exactly
  into `DryWetMixer::setWetLatency(N)`.
- No allocation/locks/file-I/O on the audio thread; all five engines pre-allocated in `prepareToPlay`.
- Render harness is the Stage-2 correctness gate (house pattern) — build it in Phase 2.1 alongside
  the engine skeleton.

## Approach Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| SPU reverb implementation order | Register-model port first; Schroeder network only as failure fallback | Authentic half-rate murk is the point; fallback stays documented in ARCHITECTURE.md if the port fails validation |
| BRR/SPU-ADPCM encoder loop | Closed-loop from the start (encode against decoded history) | Core product differentiator; open-loop remains the documented fallback only if artifacts prove wrong |
| Verification cadence | Render-harness gate per phase; formal `/plugin-verify` once at end of Stage 2 | Each phase commits only after its ROADMAP test criteria pass; one goal-backward verification pass covers the stage |
| External-reference validation depth | Internal probes + spot-check against published specs by inspection | Harness spectral/digest probes per mode suffice; no brr_encoder/lv2-psx-reverb tool-render comparison tooling |

## Open Questions

- Crush integer-step UI detents (NES rate table, GB level steps) — deferred to mockup/Stage 3;
  DSP supports continuous knob with internal stepping + 5 ms micro-fades either way.
- Exact latency constant (nominal 100–130 samples @ 48 kHz) — computed during Phase 2.1
  implementation, not decidable now.

## Next Phase

Ready for: research phase (after Stage 1 completes; research may proceed earlier since it is
read-only against ARCHITECTURE.md).
