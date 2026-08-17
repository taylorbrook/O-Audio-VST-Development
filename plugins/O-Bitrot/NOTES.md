# O-Bitrot Notes

## Status
- **Current Status:** 📦 Installed
- **Version:** 1.3.0
- **Type:** Audio Effect (Broken-Media Degradation)

## Lifecycle Timeline

- **2026-08-17 (v1.3.0):** Engine quality foundations (improvement brief items 5 + 12) — capture ring 2.5 s -> 10 s, which was the ceiling on every sustained loop: a locked groove re-jumps only while `lag + revolution <= maxLag - 50 ms`, and at 2.5 s that required a NEGATIVE starting lag for a second pass, so the headline "Locked Groove" preset released after one re-pass every time (measured 0 re-passes; now 6, with the CD loop going 24 passes deep against 5). The ring's static_assert was rewritten to actually constrain the constant — the old form was satisfied by 2.5 s and 10 s alike. `readFrac` upgraded from 2-point lerp to 4-point Catmull-Rom (mid-sample retention at 0.4*fs 0.294 -> 0.427; the `frac <= 0` exact-integer fast path kept verbatim, FUNC-02 untouched). CDSkip given a lag-budget self-release mirroring vinyl's, and ReadHead's lag-overflow clamp suppressed while a loop owns the rate — previously it teleported the head forward while CDSkip still read `state == Loop` and the next wrap re-jumped from the teleported position (13 recovery jumps landing 68404 samples behind live -> exactly 1 landing at lag 1). Suppression is additionally gated on tape being idle, because a CD/vinyl win starts a tape release ramp that runs underneath the loop with rate != 1.0. Overflow recovery pinned to a fixed 1.2 s landing rather than `0.5 * maxLag`, which would have become a 4.95 s teleport at the larger ring. Harness 57/57 stable over 3 runs (3 new probes C2/L2/M4, each verified to FAIL against the code it gates; M/M2 re-recorded — the saw marker's "period > 2x max lag" invariant was stated but unenforced and the bigger ring broke it, so the period is now 2^20 with a static_assert and derived thresholds, plus a 12 s ring-fill before measuring). pluginval s10 SUCCESS x3, auval PASS. Installed.
- **2026-08-17 (v1.2.1):** Engine robustness (improvement brief items 9 + 13) — Sync mode falls back to free-run when the host transport is STOPPED (Sync is the default, so the plugin was pure passthrough while parked; playing behaviour unchanged, dead `lastPPQ` deleted); a jump arriving mid-crossfade now FOLDS into the running fade instead of restarting it (the outgoing head was being dropped as a step of up to the full jump discontinuity — reachable in one tick via Arbitration's `cd.release()` → `vinyl.onWin()` pair; measured 0.85 → 0.0145); a tape release landing back on NORMAL with > 250 ms of lag takes ONE intentional crossfaded jump to live rather than leaving the +2% trim to claw it back (fires on ~39% of releases at stock settings, observed max lag at release 2.0–2.3 s — the pre-fix hidden clamp really was being hit). Harness 54/54 (4 new probes, each verified to fail against the pre-fix code); auval SUCCEEDED. Installed.
- **2026-08-16 (v1.2.0):** Packet-loss correctness (improvement brief items 1 + 10) — Gilbert–Elliott remap so PACKET_LOSS spans clean → ~98.6% true loss (was floor ~1%/ceiling ~30%); Good-state floor scales with loss01 (knob-zero clean); Decay concealment −6 dB/rep ramp hard-flooring to silence by ~60 ms, pitch-aligned via the AMDF path; ~1 ms raised-cosine OLA at substitute/decay cycle joints; Dropped Call 45→65, Total Media Failure 55→90. Harness 50/50 (3 new probes O2/O3/P2, probe O bounds re-derived). Installed.
- **2026-08-16 (v1.1.0):** Mono compatibility — mono→mono and mono→stereo bus layouts added (stereo→mono still rejected, no downmix rule). Stereo path bit-identical; harness 47/47 (3 new mono probes incl. mono-vs-stereo engine bit-identity under max degradation), pluginval s10 SUCCESS, auval SUCCEEDED with capabilities exactly [1,1] [1,2] [2,2]. Installed.
- **2026-08-16:** Installed to system folders (VST3 + AU, dev branding `O-Bitrot-dev`). Stages 1–4 complete.
- **2026-08-14:** Ideation complete — creative brief and requirements created from `research/glitch-effects/` (concept 2). Six degradation families (tape/CD/vinyl/packet/codec/crush), per-module controls, tempo-synced stochastic clock, seeded randomness.

## Third-Party Code

### libgsm 1.0.22 (GSM 06.10 full-rate codec)

- **Vendored:** 2026-08-15 into `third_party/libgsm/{src,inc}` + `COPYRIGHT`,
  from the canonical upstream tarball at <https://www.quut.com/gsm/>
  (gsm-1.0-pl22, Jutta Degener & Carsten Bormann, TU Berlin). 18 library
  `.c` files + 5 headers; the `toast` utilities are not vendored.
- **License:** TU-Berlin permissive (ISC-style; the 2009 addendum grants
  "Permission to use, copy, modify, and distribute this software for any
  purpose with or without fee ... provided that this notice is not removed"
  with no warranty). Full text verbatim in `third_party/libgsm/COPYRIGHT`.
  Compatible with this repository's AGPL-3.0 license (permissive → AGPL).
- **Provenance / GPL firewall:** vendored directly from upstream, NOT from
  RSBrokenMedia (GPL-3.0), which remains a patterns-only reference.
- **Build:** compiled as the `OBitrot_gsm` STATIC library with definitions
  `SASR NDEBUG NeedFunctionPrototypes=1` (never `WAV49`/`FAST`), warnings
  suppressed PRIVATE, PIC on; linked PRIVATE into `OBitrot`.

## Latency Scheme (Stage 2)

Constant reported latency of 20 ms in ALL modes:
`setLatencySamples(ceil(0.020 * fs))` once in `prepareToPlay`. CodecStage
owns the alignment: a hand-rolled integer delay of exactly that figure when
the codec is disabled or in Mu-law mode; the GSM 160-frame chain replaces it
structurally in GSM mode (one 8 kHz frame = 0.020 s). `DryWetMixer::
setWetLatency` aligns the dry path. The first 20 ms of GSM output after
engagement is silence (output frame primed with zeros; the 10 ms
CODEC_ENABLE fade covers it). At non-integer `0.020*fs` host rates the GSM
path misaligns by < 1 sample versus the plain delay — accepted.

## Known Issues

None

**Installation Locations:**
- VST3: `~/Library/Audio/Plug-Ins/VST3/O-Bitrot-dev.vst3`
- AU: `~/Library/Audio/Plug-Ins/Components/O-Bitrot-dev.component`
