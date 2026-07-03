# Stage 2: DSP — Verification

## Verification Date

2026-06-26

## Goal-Backward Analysis

### Original Goals (from CONTEXT.md / PLAN.md)

1. Fill the silent Stage-1 shell with the real audio engine `EXCITATION → RESONATOR → MATERIAL/DAMPING`.
2. Deliver a tunable, decaying, dynamically-responsive synth across **String (Karplus-Strong)** and **Modal** resonators with three exciters (**Pluck / Strike / Bow**).
3. All 17 wired params drive real DSP; cross-driving (any exciter → any resonator) works.
4. Gate correctness with the offline render-harness using an **autocorrelation** pitch probe (±5 cents), re-run at each of the three phase gates (D3).
5. Authored-in stability (clamps, tanh-sat, DC-blocker, isfinite/Q guards) since the references lack it.
6. Viz taps (loop-energy + modal stems) running, audio-thread copy-only (PERF-01).

### Deliverables (from SUMMARY.md + code inspection)

1. 9 new header-only DSP classes (`OnePoleLPF`, `PositionComb`, `StringResonator`, `PluckExciter`, `StrikeExciter`, `BowExciter`, `BowNoiseGenerator`, `ModalResonator`, `VizTap`) + edits to `PhysicalModelVoice.h`, `PluginProcessor.{h,cpp}`, `CMakeLists.txt`, harness `main.cpp`.
2. KS string loop (Thiran delay + loop LPF + shadow crossfade + exact-phase-delay tuning + DC blocker); 8-mode RBJ constant-skirt modal bank (Fletcher inharmonicity, derived T60→Q, isfinite/Q guards).
3. Single `e[n]` drives whichever resonator `resonatorType` selects → cross-driving for free; Material macro co-moves damping+decay.
4. Harness with `autocorrPitchHz` (parabolic-refined) + 22 acceptance checks across 3 phase gates.
5. Stability authored in: feedback clamp `[0.80, 0.999]`, tanh-sat/jlimit on bow, DC blocker on string output, per-biquad isfinite reset, Q ≤ 500.
6. `VizTap` (lock-free `VizRing` + loop-energy scalar + 8 modal stems), lead-voice publish, copy-only.

### Goal Achievement

| Goal | Status | Evidence |
|------|--------|----------|
| 1. Real engine fills the shell | ✅ Achieved | `makes-sound` rms=0.048; engine routes exciter→resonator in `PhysicalModelVoice::renderNextBlock` |
| 2. Tunable/decaying/dynamic, String + Modal, 3 exciters | ✅ Achieved | Tuning ±5¢; `decay-shortens`; pluck/strike/bow + string/modal all produce sound |
| 3. All params drive DSP; cross-driving | ✅ Achieved | `modal-pluck/strike/bow` all PASS; `stringModel` is the only sound-inert param (KS-only, D1 — deferred v1.1) |
| 4. Autocorr ±5¢ gate, phased | ✅ Achieved | C1 −0.00¢, C3 0.67¢, C5 0.05¢, C7 1.76¢; 3 gates structured in `main.cpp` |
| 5. Authored stability | ✅ Achieved | `bow-bounded-max` peak=0.97 @ maxForce+maxDecay; `no-blowup`; `no-DC` mean=−0.0004 |
| 6. Viz taps copy-only | ✅ Achieved | `VizTap.h` lock-free ring + atomics; `publishViz` copy-only; no alloc/lock in `processBlock` |

## Requirements Verification

**Stage:** stage-2
**Requirements for this stage:** 16 total (12 must, 3 should, 1 nice)

| Requirement | Priority | Status | Evidence / Acceptance |
|-------------|----------|--------|-----------------------|
| FUNC-01: Excitation→resonator→material chain | must | ✅ Complete | `makes-sound`, `decay-shortens`, Material macro co-move |
| FUNC-02: Swappable Pluck/Strike/Bow | must | ✅ Complete | `strike-makes-sound`, `bow-sustains`, pluck `makes-sound` |
| FUNC-03: String/Modal switch | must | ✅ Complete | String tuning + `modal-*` all PASS |
| FUNC-04: Cross-driving (each exciter→each resonator) | must | ✅ Complete | `modal-pluck/strike/bow` PASS (same `e[n]` path) |
| FUNC-05: 16-voice polyphony | should | ✅ Complete | `kNumVoices` voices + note-stealing in ctor; ADSR-tail release (manual DAW chord check recommended) |
| FUNC-06: Velocity scales strength/brightness | should | ✅ Complete | `velLevel`→amp (`triggerExciter`) + `velToBrightness`→cutoff (`exciterCutoff`) |
| DSP-01: Karplus-Strong loop | must | ✅ Complete | `StringResonator::processAdditive` (delay+LPF+feedback) |
| DSP-02: Fractional-delay tuning | must | ✅ Complete | Thiran + exact-phase-delay comp; ±5¢ @ C1/C3/C5/C7 |
| DSP-03: cutoff+feedback, clamped < 1 | must | ✅ Complete | `setDecay` clamps `[0.80,0.999]`; `setDamping` log cutoff |
| DSP-04: Modal bank, higher modes quieter+faster | must | ✅ Complete | `DECAY_MULTIPLIERS`, 1/k amp tilt; `modal-rings` |
| DSP-05: Inharmonicity stretches spacing | must | ✅ Complete | `inharmonicity-stretches` 1760→2340 Hz |
| DSP-06: Waveguide string (dual-rail) | nice | ⏸️ Deferred → v1.1 | Scope decision D1; `stringModel` ships KS-only, no contract break |
| DSP-07: Material macro steel↔nylon co-move | should | ✅ Complete | `parameterChanged` writes back damping+decay (survives pluginval fuzz) |
| DSP-08: DC-safe, band-limited, bow stable+sustaining | must | ✅ Complete | `no-DC`, raised-cosine strike, `bow-sustains`+`bow-bounded-max` |
| PERF-01: Real-time safe + lock-free viz FIFO | must | ✅ Complete | `processBlock` no alloc/lock; `VizRing` lock-free copy-only |
| QUAL-01: No artifacts across ranges | must | ✅ Complete | `no-blowup`, `bow-bounded-max`, `strike-hi-clean`, `output-finite` |

**Requirements Summary:**
- ✅ Complete: 15 (12 must, 3 should)
- ⏸️ Deferred (v1.1, nice): 1 (DSP-06)
- ⚠️ Partial: 0
- ❌ Failed: 0

## Automated Checks

| Check | Result | Notes |
|-------|--------|-------|
| Build (VST3 + AU + harness) | ✅ Pass | Clean compile/link, no warnings |
| Render-harness gate | ✅ Pass | **ALL PASS — 22/22 checks, exit 0** |
| Tuning (autocorr ±5¢) | ✅ Pass | C1 −0.00¢ · C3 0.67¢ · C5 0.05¢ · C7 1.76¢ |
| Bow bounded @ maxForce+maxDecay | ✅ Pass | peak=0.97, finite |
| Cross-driving (modal pluck/strike/bow) | ✅ Pass | rms 0.026–0.177, all finite/bounded |
| Inharmonicity stretch | ✅ Pass | top mode 1760→2340 Hz |
| No-DC / no-blowup / finite | ✅ Pass | mean=−0.0004, peak<1.0 |
| pluginval --strictness-level 10 (VST3) | ✅ Pass | **SUCCESS** (incl. parameter fuzz / Material macro write-back) |

> Note: SUMMARY.md states "23 checks"; the harness as run reports **22**. The discrepancy is a SUMMARY miscount, not a missing check — all gate criteria are covered.

## Human Verification (recommended in DAW — not blocking)

- [ ] Play a chord — confirm 16-voice polyphony and clean voice-stealing
- [ ] Sweep Material 0→100 — confirm steel→nylon and Damping+Decay knobs visibly co-move
- [ ] Hold a Bow note — confirm it sustains, then rings out cleanly on release
- [ ] Swap exciter under each resonator — confirm only attack/drive changes

## Issues Found

- None blocking. One documentation cleanup: REQUIREMENTS.md DSP-06 "Verified At" was `stage-2` but the feature is an explicitly-scoped v1.1 deferral (CONTEXT D1) — status updated to deferred this pass.
- Deviations from PLAN (all justified, recorded in SUMMARY): (a) exact phase-delay tuning replaced the DC group-delay approximation (C7 12.1¢→1.76¢); (b) Bow = friction-weighted noise drive (documented D2 fallback — memoryless friction cannot self-oscillate in a single KS loop); (c) Material macro on the message thread; (d) `stringModel` ships KS-only.

## Stage Verdict

**Status:** ✅ VERIFIED

**Ready for next stage:** Yes

**Blockers:** None. (DSP-06 waveguide is a `nice` deferred to v1.1 by design; not a v1.0 blocker.)
