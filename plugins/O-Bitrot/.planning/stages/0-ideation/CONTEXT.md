# O-Bitrot — Stage 0 Context (Discuss Phase Findings)

**Date:** 2026-08-15
**Phase:** Stage 0 Research & Planning (research-planning-agent)

## What was decided

1. **Architecture locked:** one shared circular buffer (2.5 s span, static_assert vs 1.8 s vinyl
   revolution + 0.5 s ramp + margin) + per-channel variable-rate read heads whose *behavior* is the
   current failure state. Transport families (tape/CD/vinyl) are mutually exclusive read-head states
   arbitrated per clock tick; Packet/Codec/Crush are serial post stages.
2. **Packet loss runs on its own 20 ms grid**, not the MediaClock — GE burst statistics (DSP-04
   acceptance) only hold on the packet grid. It is not part of transport arbitration.
3. **Constant reported latency = one GSM frame (20 ms)** in all modes, all paths delay-aligned;
   avoids dynamic latency renegotiation when CODEC_MODE automates. Fallback documented (report 0,
   GSM internally delayed) if the constant cost is rejected at verify.
4. **Chain order:** MediaPlayer → Packet → Codec → Crush → mix (physical model: player fails →
   transmission conceals → phone line colors → output converter crushes).
5. **Determinism contract:** RNG streams (8, splitmix-derived from SEED) reseed in prepareToPlay and
   on SEED change; RNG consumed only at ticks/packets. Offline renders bit-identical per seed;
   mid-session realtime not required to match a from-zero bounce.
6. **Parameter deltas vs draft:** CRUSH_RATE fixed to fs-independent 500 Hz–20 kHz (runtime clamp
   fs/2); CLOCK_SYNC_DIV enumerated to 7 divisions; GE mapping formalized
   (PACKET_LOSS → π_B, PACKET_BURST → E[B] 1–8 packets). Carry into parameter-spec.md at mockup
   finalization.
7. **libgsm vendored independently** from upstream (permissive TU-Berlin license; verify license
   file at vendoring). RSBrokenMedia (GPL-3.0) is patterns-only — firewall documented in
   ARCHITECTURE.md.
8. **Phasing:** engine core + tape first (infrastructure proof), codec last (isolated vendoring
   risk). 5 DSP phases, 3 GUI phases. Complexity 5.0 (capped) → staged implementation.

## Constraints carried forward

- Anti-zipper rules (deep-dive §2.5): smooth targets not outputs, fractional-crossing latch, never
  reset phase accumulators, crossfade all jumps 1–5 ms unless HARD_EDGES.
- Per-sample envelope follower (offline-bounce invariance).
- Sample-accurate split-block tick processing; write-ring-before-read (QUAL-02).
- `ArrayCoefficients` only for IIR; TPT filters for swept cutoffs.
- No allocations/locks/logging in processBlock; gsm_create in prepareToPlay only.

## Open items for later stages

- Mockup phase: six-panel + global-strip layout; CD_SEGMENT gray-out below severity ~0.5 (UI
  decision); dice button + seed readout; sync/free control swap.
- parameter-spec.md finalization after mockup (mockup = source of truth).
- Verify libgsm MSVC build (separate C target, relaxed warnings) during Phase 2.5.
- Substitute concealment ships with built-in Decay fallback; if AMDF quality poor, re-scope DSP-04
  "4 audibly distinct modes" at verify.

## Inputs consumed

- `BRIEF.md`, `REQUIREMENTS.md` (18 reqs), `parameter-spec-draft.md` (31 params)
- `research/glitch-effects/degradation-dsp-deep-dive.md` (primary — Level 3, built upon, not
  re-researched)
- `research/glitch-effects/multi-effect-sequencer-reuse-audit.md` (reuse map)
- `troubleshooting/patterns/juce8-critical-patterns.md`
- Local JUCE 8.0.14 source (API verification — Context7 doc-fetch unavailable this session)
