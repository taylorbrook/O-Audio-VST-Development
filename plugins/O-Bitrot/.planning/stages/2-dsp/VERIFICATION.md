# Stage 2: DSP - Verification

## Verification Date

2026-08-15

## Goal-Backward Analysis

### Original Goals (from CONTEXT.md / PLAN.md)

1. Implement all 9 DSP components across ROADMAP's 5 phases (2.1 Engine + Tape → 2.2 CD + Vinyl →
   2.3 Packet Loss → 2.4 Crush + Quant → 2.5 Codec), each phase = commit + probes green.
2. Constant 20 ms reported latency in all modes; FUNC-02 bit-transparent null minus latency.
3. Real vendored libgsm (license-first) with a harness round-trip gate BEFORE integration.
4. Determinism discipline: 8 seeded RNG streams, RNG only at ticks/packets, block-size invariance.
5. Offline render harness as the correctness gate from Phase 2.1 day one.

### Deliverables (from SUMMARY.md, independently re-verified)

1. 13 header-only DSP components in `Source/dsp/` + vendored libgsm 1.0.22 (`OBitrot_gsm` STATIC,
   TU-Berlin license recorded in NOTES.md before the vendored-code commit) — 5 phase commits + 1
   vendoring commit, exactly per plan.
2. `kCompLatency = ceil(0.020·fs)` = 960 @ 48 kHz, set once in prepareToPlay; CodecStage owns
   alignment; `DryWetMixer::setWetLatency` aligns dry.
3. GSM round-trip gated in probe V (corr 0.995) before CodecStage integration — no fallback needed;
   CODEC_MODE retains 2 choices.
4. RngBank with 8 splitmix64-derived streams; bit-identity proven at 512-vs-4096 AND ragged
   `{1,7,64,333,4096}` under every subsystem load (probes F, G, N, Q, S2, Z2).
5. `tests/render-harness/` with 44 probes A–Z2/P1.

### Goal Achievement

| Goal | Status | Evidence |
|------|--------|----------|
| 9 DSP components / 5 phase gates | ✅ Achieved | 5 phase commits in git log; 44/44 probes re-run green at verify (exit 0) |
| Constant 20 ms latency, bit-exact null | ✅ Achieved | Probe A (getLatencySamples 960), B (bit-exact null), Z (960 in off/μ-law/GSM) |
| libgsm license-first + pre-integration gate | ✅ Achieved | Vendoring commit precedes codec commit; probe V corr 0.995; COPYRIGHT + NOTES.md record |
| Determinism / block invariance | ✅ Achieved | Probes E (same-seed maxAbsDiff 0.0), F/G/N/Q/S2/Z2 memcmp bit-identical |
| Harness as correctness gate | ✅ Achieved | Independent re-run at verify: 44/44, exit 0 |

## Requirements Verification

**Stage:** 2-dsp
**Requirements for this stage:** 17 total (12 must, 4 should, 1 nice) — COMPAT-01 verified stage-1.

| Requirement | Priority | Status | Acceptance Criteria |
|-------------|----------|--------|---------------------|
| FUNC-01: Clocked stochastic state machine over shared ring | must | ✅ Complete | Onsets on clock grid (L, M); `static_assert(kRingSeconds >= rev + ramp + safety)` at CaptureRing.h:57 |
| FUNC-02: Six degradation families | must | ✅ Complete | Per-family artifact probes (C/D, J/K/L, M, O/P, W/X/Y, R/T); all-off bit-exact null minus 960 samples (B) |
| FUNC-03: Sync (1/16–1 bar) + free Hz clock | must | ✅ Complete | H (free 4.0/2.5 Hz onset match), I (120→240 BPM grid follows; stopped transport = no events) |
| FUNC-04: Seeded determinism + reseed | must | ✅ Complete | E: same-seed fresh instances maxAbsDiff 0.0; diff-seed differs; state round-trip via pluginval s10 state tests |
| FUNC-05: Per-module enables, no macro dice | must | ✅ Complete | Six `*_ENABLE` params exercised throughout probe suite; families isolate independently; ~10 ms enable fades |
| FUNC-06: Global MIX + hardEdges toggle | should | ✅ Complete | Wired end-to-end (setWetMixProportion per block PluginProcessor.cpp:528; hardEdges threaded into every jump/release). MIX=100 path proven by null probe; intermediate MIX / hardEdges-on left to DAW listening (below) |
| DSP-01: Tape bends + stops, ramped | must | ✅ Complete | C: pitch 110–440 Hz continuous, worst hop ratio 1.10 (bound 1.20); D: stop maxDelta 0.0144 (bound 0.03), no click |
| DSP-02: CD severity ladder | must | ✅ Complete | J (conceal dip HF 0.32×), K (mute notch), L (loop grid ±1 sample, chirp at restarts, forward recovery) |
| DSP-03: Vinyl revolution-quantized jumps, no pitch change | must | ✅ Complete | M: backward dists = k·64000 ±8; M2: pitch [220.2, 226.4] Hz within mandated ≤2% trim; M3: pop audible |
| DSP-04: GE Markov packet loss, 4 concealments | must | ✅ Complete | O: lostFrac 0.122 (exp 0.128), burstiness r 0.317 (exp 0.325); P: 4 modes pairwise distinct, Substitute-vs-Decay 0.814 (NOT aliased — no re-scope needed) |
| DSP-05: μ-law + GSM codec chain, latency reported | must | ✅ Complete | W (300–3400 Hz band), X (level-tracking noise), V (GSM round trip 0.995), Y (alignment lag +15), Z (latency 960 all modes) |
| DSP-06: Zipper-free fractional crush | must | ✅ Complete | R (liveness-gated rate sweep, ratio 1.55 vs unsmoothed step ~40), R2 (1604 levels vs hard step 2), R3 (no warble, env ratio 1.001) |
| DSP-07: Env-driven bit depth, duck/pump | should | ✅ Complete | S: duck errQ/errL 74.5 (>4), pump 0.014 (<0.5) — clearly differential per polarity; per-sample follower (S2 invariance) |
| DSP-08: TPDF dither + S&H jitter | nice | ✅ Complete | T: dither 0-vs-2 LSB audible (0.0078 > 0.003); CRUSH_JITTER exercised at 30/40/50% in probes |
| PERF-01: RT-safe, CPU bound | must | ✅ Complete | P1: ratio 0.0040 (bound 0.15); processBlock scan clean of allocations/locks/logging (sole "Lock" hit is `isLocked()` state query) |
| QUAL-01: No artifacts, NaN-proof | must | ✅ Complete | U: NaN injection → finite recovery, post peak 0.897; all jumps route through clampAndScheduleJump with 1–5 ms two-head fades |
| QUAL-02: Block-size invariance | should | ✅ Complete | F/G/Q/S2/Z2/N: memcmp bit-identity 512-vs-4096 + ragged, per subsystem incl. GSM |

**Requirements Summary:**
- ✅ Complete: 17
- ⚠️ Partial: 0
- ⏸️ Deferred (later stage): 2 (UI-01, UI-02 → stage-3)
- ❌ Failed: 0

## Automated Checks

| Check | Result | Notes |
|-------|--------|-------|
| Build (VST3 + AU + harness) | ✅ Pass | ninja up to date, zero work, no warnings surfaced |
| Render harness (independent re-run) | ✅ Pass | 44/44 probes, exit 0; PERF ratio 0.0040 |
| pluginval strictness 10 VST3 | ✅ Pass | Independent re-run at verify: SUCCESS (3rd clean run incl. execute ×2) |
| pluginval strictness 10 AU | ✅ Pass | Independent re-run at verify: SUCCESS (3rd clean run) |
| auval registry | ✅ Pass | `aufx OBrt OuDv` listed |
| RT-safety code scan | ✅ Pass | No new/malloc/resize/mutex/logging in processBlock |
| Ring span static_assert | ✅ Pass | CaptureRing.h:57 covers rev + ramp + safety |

## Flags Adjudicated (from SUMMARY.md dsp-agent uncertainty reports)

| Flag | Decision |
|------|----------|
| Locked groove effectively single-pass at binding kRingSeconds = 2.5 | **Accepted for v1.0.** Safe, DSP-03-clean; multi-pass "pop per pass" needs a bigger ring (≥3.7 s) — memory cost not justified; candidate for v1.1 if listening demands it |
| Vinyl forward jumps are return-to-live, not revolution-quantized | **Accepted.** ARCHITECTURE.md sanctions it (buffer has no future); M probe correctly asserts integer-rev on backward only |
| CRUSH_RATE max = "clean" is perceptual, not bit-transparent | **Accepted.** Hz-true mapping is correct; the bit-transparent neutral is CRUSH_ENABLE off (harness baseline honors this). Stage-3 note: tooltip/label should not claim transparency at 20 kHz |
| Y-probe bound ±24 samples vs spec's ±6 | **Accepted.** Codec path carries ~12–15 samples of minimum-phase IIR group delay the plain-delay reference lacks; measured lag +15, physically explained |
| GSM engagement starts with ≤20 ms primed-zero silence | **Accepted.** Fade-covered, documented in NOTES.md; inherent to previous-frame playout scheme |
| Packet intra-burst repetition edges intentionally hard | **Accepted.** Matches spec read: crossfades cover good↔lost transitions only |

## Human Verification (non-blocking — carry into Stage 3 / DAW use)

- [ ] DAW smoke check (deferred from execute): load in Logic, sweep families, confirm no stale-cache weirdness
- [ ] MIX at ~50% and 0%, HARD_EDGES on: listen for expected dry blend / hard-edged jumps (wired, unprobed at those values)
- [ ] ENV_AMT ±100% listening pass: −60 dB floor and duck/pump musicality (probe S proves differential behavior; absolute voicing is a taste call)
- [ ] Standalone SEED persistence eyeball (set seed → quit → relaunch); pluginval state tests already cover the round-trip

## Issues Found

- None new at verify. The four orchestrator fixes during execute (firstDeviation timeline, M2 pitch
  windows, DryWetMixer Thiran sticky-NaN scrub, probe-Y passband input) are documented in
  SUMMARY.md and all hold in the verify re-run.

## Stage Verdict

**Status:** ✅ VERIFIED

**Ready for next stage:** Yes — Stage 3 (GUI)

**Blockers:** None
