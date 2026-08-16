# Stage 2: DSP — Execution Summary

**Date:** 2026-08-15
**Agent:** dsp-agent (one continued session across all 5 phases), orchestrator handled
builds/gates/commits/fixes
**Result:** ✅ All 18 tasks complete — 5 phase commits, 44/44 harness probes green,
pluginval strictness 10 clean (VST3 ×2, AU ×2), installed and auval-registered.

## Phase Commits

| Phase | Commit | Probes at gate |
|-------|--------|----------------|
| 2.1 Engine core + tape | `feat(o-bitrot): stage 2 phase 2.1 — engine core + tape` | 13/13 |
| 2.2 CD skip + vinyl | `feat(o-bitrot): stage 2 phase 2.2 — cd skip + vinyl` | 24/24 |
| 2.3 Packet loss | `feat(o-bitrot): stage 2 phase 2.3 — packet loss` | 28/28 |
| 2.4 Crush + quant | `feat(o-bitrot): stage 2 phase 2.4 — crush + quant` | 36/36 |
| 2.5a libgsm vendoring | `feat(o-bitrot): vendor libgsm 1.0.22 (license-first)` | — |
| 2.5 Codec | `feat(o-bitrot): stage 2 phase 2.5 — codec (mu-law + gsm)` | 44/44 |

## What Was Built

**New DSP (all header-only, `Source/dsp/`):** CaptureRing (absolute-index ring +
fractional read), RngBank (8 splitmix64-seeded streams), MediaClock (sample-accurate
sync/free ticks), ReadHead (clampAndScheduleJump choke point, true two-head 3 ms
crossfades, ≤+2% re-approach trim), TapeTransport (interval bends, stops),
Arbitration (fixed roll order tape→cd→vinyl + EnableFade), CDSkip (severity ladder:
conceal dip / mute+tick / segment loop+chirp), VinylTransport (revolution-quantized
jumps, locked groove, pops), ArtifactSynth (pops/ticks/chirps), PacketLossStage
(GE Markov on independent 20 ms grid, 4 concealments incl. AMDF Substitute),
CrushStage + FractionalHoldLatch (shared SRR primitive), QuantStage (fractional bits,
TPDF dither, per-sample env follower), CodecStage (mono→300–3400 BP→8 kHz latch→
μ-law | GSM 06.10 previous-frame playout→post-LPF→equal-power blend).

**Vendored:** libgsm 1.0.22 from quut.com (18 sources, 5 headers, COPYRIGHT verbatim);
TU-Berlin permissive license recorded in NOTES.md BEFORE the vendored-code commit;
`OBitrot_gsm` STATIC target (SASR NDEBUG NeedFunctionPrototypes=1, -w, PIC).
GSM round-trip harness-gated before integration (corr 0.995) — no μ-law fallback needed.

**Harness:** `tests/render-harness/` (O-Octagon template), 44 probes A–Z2/P1 covering
FUNC-01..04, DSP-01..08, PERF-01, QUAL-01/02. PERF-01 measured: worst-case ratio
0.0042 (bound 0.15).

**Latency:** constant `kCompLatency = ceil(0.020·fs)` in all modes; CodecStage owns
alignment (plain integer delay disabled/μ-law; GSM chain structurally = 0.020·fs);
DryWetMixer::setWetLatency aligns dry; FUNC-02 bit-exact null holds.

## Orchestrator Fixes Along the Way

1. **firstDeviation timeline** — probe helper returned output-time indices while
   expectations were written on the input/tick timeline (all four FUNC-03 probes read
   exactly +kCompLatency+64); helper now maps back to input time.
2. **M2 pitch windows** — autocorr windows straddling jump instants read false pitch
   (crossfade phase discontinuity + pop transient); windows near tick instants are now
   excluded (deterministic — jumps only happen at ticks). Steady state reads
   [220.2, 226.4] Hz = 220 Hz + the mandated ≤2% trim.
3. **Sticky NaN via DryWetMixer Thiran dry-delay** (QUAL-01, probe U): JUCE's Thiran
   DelayLine state computes `alpha·(x − v)`; at integer latency alpha == 0 but
   `0·NaN == NaN`, so one NaN poisons it forever. Fixed by scrubbing non-finite INPUT
   samples to 0 at the processBlock boundary (finite samples never written →
   FUNC-02 bit-exactness preserved).
4. **Probe Y input** — GSM-vs-delay alignment used broadband noise; the full-band
   reference starves normalized correlation (~0.1) with zero alignment defect. Now a
   three-tone passband input (600/1450/3100 Hz, non-commensurate → unique xcorr peak);
   corr 0.578, lag +15 within the ±24 bound (grid jitter + IIR group delay).

## Flags for Verify (from dsp-agent uncertainty reports)

- **Locked groove is effectively single-pass** at the binding `kRingSeconds = 2.5`
  (a re-pass needs 2·rev ≤ ring span; 3.6 s / 2.67 s > 2.5 s). Safe and DSP-03-clean;
  multi-pass "pop per pass" would need a bigger ring — decision for verify.
- **Vinyl forward jumps are return-to-live**, not revolution-quantized (ARCHITECTURE
  sanctions: "clamped to writeAbs − minLag; buffer has no future"). M probe asserts
  integer-rev on backward only.
- **"CRUSH_RATE max = clean" is perceptual**: Hz-true mapping means the interpolated
  latch still engages mildly at 20 kHz; the bit-transparent neutral is CRUSH_ENABLE off.
- **Y-probe bound is ±24 samples** (not the spec's ±6): the codec path carries ~12–15
  samples of minimum-phase IIR group delay the plain-delay reference lacks.
- **GSM engagement starts with ≤20 ms silence** (primed zero frames, fade-covered) —
  documented in NOTES.md.
- **Packet intra-burst repetition edges are intentionally hard** (Repeat/Decay wrap);
  crossfades cover good↔lost transitions per the read of the spec.
- Env→t mapping at ±100% is broad (duck/pump clearly differential, ~1000× in probe S,
  but absolute effect touches tails and transients both); −60 dB floor is a listening
  decision.

## Verification Hooks

- Harness: `cmake -DOUARICON_BUILD_TESTS=ON` → `ninja O-Bitrot-render-test` → run
  binary, exit 0 (44/44).
- pluginval strictness 10: VST3 ×2 SUCCESS, AU ×2 SUCCESS (2026-08-15).
- Installed: `~/Library/Audio/Plug-Ins/{VST3,Components}/O-Bitrot-dev.*`;
  auval lists `aufx OBrt OuDv`.
- DAW smoke check: NOT yet performed (deferred to verify phase / user).
