---
title: "Glitch Effects Research — Index"
created: 2026-08-14
last_verified: 2026-08-15
juce_version: "8.0.14"
summary: "Index of the Level-3 glitch-effects research pass: degradation DSP deep dive, spectral glitch/datamosh techniques, and the suite reuse audit for multi-effect glitch sequencing."
domain: dsp
type: guide
keywords:
  - glitch
  - degradation
  - bitrot
  - spectral
  - reuse-audit
stages: [0, 2]
agents: [dsp, research]
---

# Glitch Effects Research (Level 3, 2026-08-14)

Deep-research output on glitch audio effects, extending `research/stutter-effects/` (2026-01). Three parallel investigation threads plus a market sweep.

## Files

| File | Contents |
|---|---|
| `degradation-dsp-deep-dive.md` | Bitcrush/SRR/codec/broken-media DSP: formulas, anti-zipper techniques, open-source references |
| `spectral-glitch-datamosh.md` | STFT glitch primitives, spectral freeze state of the art, datamosh product analysis, JUCE FFT patterns |
| `multi-effect-sequencer-reuse-audit.md` | O-Polystutter/O-ReverseDelay/O-Freeze code audit + recommended O-Glitch architecture |

## Proposed plugins (ranked by leverage)

1. **O-Glitch** — multi-effect step sequencer (dBlue Glitch 2 successor). 16 steps × ~7 effects (stutter, reverser, tapestop, pitch, bitcrush, gate, freeze), Euclidean patterns, MIDI scenes, TapeDegrader post-chain. ~60% reuse from the suite; ~14–17 dev-days. See `multi-effect-sequencer-reuse-audit.md`.
2. **O-Bitrot** — broken-media degradation box. Clocked stochastic state machine over a shared circular buffer (RSBrokenMedia pattern): tape bends from a musical-interval table, CD-skip segment stutters + synthesized chirps, vinyl revolution-quantized jumps + pops, Gilbert–Elliott packet loss, real GSM/μ-law codec chain, DeRez-style sweepable crush with envelope-driven dynamic bit depth. ~2–3 weeks. See `degradation-dsp-deep-dive.md`.
3. **O-Lossy** — spectral codec-artifact plugin (Goodhertz Lossy territory): bit-budget spectral gating (holes + birdies), packet repeat/loss, phase jitter, Inverse residual mode. Unclaimed marquee feature: stale-frame magnitudes driven by live phase-deltas — the true audio analog of datamosh motion vectors (no shipping product does this). ~3–4 weeks. See `spectral-glitch-datamosh.md`.
4. **O-Tapestop** — focused varispeed/playhead plugin (Path C lite): tapestop/start with curve control, scratch. Tape-start uses the Signalsmith fall-behind-then-crossfade-skip resync. Reuses O-ReverseDelay's grain/direction engine; ~1 week.

## Market snapshot (2026-08)

- Free tier owns sequenced glitch: Illformed Glitch 2 (now free), Glitchmachines Fracture/Hysteresis.
- Paid tier ($36–$149, routinely 50% off) wins on specialization: Effectrix 2 (paint-on-grid), Stutter Edit 2 (MIDI gestures; complained-about UI), ShaperBox 3 (musical, not chaotic), Portal (granular + scale lock), RC-20 (lo-fi character), **Aberrant DSP Digitalis $36** (closest modern dBlue successor: spectral PaintBox / decimate+bitrot / repeater, 16-step 4-slot sequencer).
- Gaps: no freeware gesture-triggered tool; aesthetic specialization beats feature count; true codec-corruption datamosh is unclaimed.

## Related

- `research/stutter-effects/` — Path A/B/C implementation guides (Path B shipped as O-Polystutter v1.12.x)
- Side finding filed 2026-08-14: O-Polystutter `lane1..4_filter` params are shipped no-ops (declared + UI-bound, no DSP) — /improve in progress
