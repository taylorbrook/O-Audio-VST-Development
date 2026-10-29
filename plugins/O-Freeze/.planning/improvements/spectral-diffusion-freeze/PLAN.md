---
milestone: spectral-diffusion-freeze
domain: mixed
execute_agent: general-purpose
version_bump: minor
base_version: 2.8.0
target_version: 2.9.0
created: 2026-10-28
---

# Plan: Spectral Diffusion Freeze

**Plugin:** O-Freeze
**Milestone:** spectral-diffusion-freeze
**Created:** 2026-10-28
**Phase:** Plan

---

## Summary

**Improvement:** A second freeze engine, `ENGINE` = Granular / Spectral. Spectral is an STFT freeze (8192 / 4×, Hann×Hann). It averages magnitudes over `AVG_FRAMES` frames at capture, drifts each bin's level with a random walk (Drift), and advances phase by ω̂H + β·U(−π,π) with β = `DIFFUSION`. Phase locking applies near β = 0. Detune becomes a per-bin spread, and the LFO modulates Diffusion.
**Approach:** A self-contained `SpectralFreeze` class driven per sample from the existing loop. Analysis runs on the write-head clock, resynthesis on the synth clock, and every draw is keyed on a hop or engage counter with its own `CounterRng` slots. Granular stays byte-identical because of an `e == 0` branch and a draw-for-draw extraction of `startGranularVoices`.
**Version:** 2.8.0 → 2.9.0 (MINOR)

### Decisions locked at plan (RESEARCH open questions)

| # | Question | Decision |
|---|---|---|
| D1 | UI layout | **Swap slots.** In Spectral, the Size slot becomes Diffusion, the Grains slot becomes Avg, and Reverse dims. No relayout. The Engine toggle is a second segmented toggle styled like `#mode-toggle`. |
| D2 | β makeup | **Half-compensation.** `makeup(β) = 1/sqrt(1 − 0.5·(1 − sinc²(β)))`. Execute refits the constant until chord and pink noise both stay within ±3 dB of β = 0. |
| D3 | AVG_FRAMES type | **`AudioParameterInt` 1–16, default 8** (consistent with Grains). It takes effect at the next capture. |

### Reference facts (verified at plan)

- The v2.8.0 baseline is commit **`26bbaeb6`**. There is no `v2.8.0-O-Freeze` tag, so build the null's out-of-tree reference from `git archive 26bbaeb6 -- plugins/O-Freeze` (memory: *null test w/ seeded out-of-tree pair*). Don't create a tag (memory: *never tag unless /publish*).
- `RngSlot` (PluginProcessor.h:184) = `kSlotJitter, kSlotDetune, kSlotInterval, kSlotDrift, kSlotHold`. The new slots get appended after `kSlotHold`.
- The layout's last param is `DETUNE` (PluginProcessor.cpp:121). ENGINE / DIFFUSION / AVG_FRAMES get appended after it with version hint 1.
- O-Freeze has **no alloc harness**. Port the hook from `plugins/O-Bells/tests/render-harness/main.cpp`, which counts allocations on the processBlock thread only and proves the hook is live first.

---

## Task Breakdown

### Task 1: Params + engine wiring scaffold (no spectral DSP yet)

**Outcome:**
- ENGINE (Choice: Granular / Spectral, default 0), DIFFUSION (Float 0–100 %, default 0) and AVG_FRAMES (Int 1–16, default 8) are appended to the layout.
- 3 RNG slots are appended (`kSlotSpecPhase`, `kSlotSpecDrift`, `kSlotSpecEngage`, with detune/rate sub-streams split by counter offset).
- An engine smoother `e` (50 ms linear) is added.
- `startGranularVoices(reverse, drift, detune)` is extracted from `setFrozen` (PluginProcessor.cpp:358–383) **draw-for-draw**.
- The per-sample mix takes the form `if (e == 0) frozen = granularSum * densityGain; else …`, with a stub `SpectralFreeze` that outputs 0.
- The release fade per engine is `max(granular remaining + 50 ms, spectral 100 ms)` while both engines are live.
- `SpectralFreeze.h/.cpp` exists as an empty-shell class and is added to CMakeLists.

**Verification:**
- [x] Builds clean (no new warnings).
- [x] The existing `tests/blocksize-null/gate.py` still PASSES (Granular, all configs).
- [x] An old (v2.8.0) state loads with Engine = Granular, Diffusion 0, Avg 8.

**Dependencies:** None
**Estimated effort:** Medium

---

### Task 2: v2.8.0 Granular null gate (regression wall)

**Outcome:** `tests/granular-null/gate.py` renders a v2.8.0 out-of-tree build (`git archive 26bbaeb6`) and the current build. Both use the same template states (seeds), Engine = Granular, and all existing blocksize-null configs. VST3 params are matched by name.

**Verification:**
- [x] Every config is **bit-identical** (or ≤ −140 dBFS) to v2.8.0.
- [x] Negative control: a deliberate one-ulp change (e.g. `densityGain * 1.0000001f`) FAILS the gate, then gets reverted.
- [x] The frozen section is non-vacuous (RMS above the gate floor).

**Dependencies:** Task 1
**Estimated effort:** Medium
**Gate rule:** Task 3 doesn't start until this passes. Re-run it after Tasks 4, 5 and 7.

---

### Task 3: SpectralFreeze — analysis + capture

**Outcome:**
- `prepare(fs, maxCh)` preallocates everything: the FFT order-13 object is built in the ctor, plus the Hann tables, a 16 × 4097 × 2 power ring, the last-2-frame phases, 2·N work buffers, the per-bin state arrays and a 2W OLA ring.
- `analyseFrame(endPos)` is the single function behind both the rolling analysis (every H = 2048 write-head advances, gated on Engine = Spectral) and the on-demand fallback (fewer than N valid frames).
- `capture(N)` performs:
  - the RMS magnitude average;
  - an IF estimate `ω̂_k = ω_k + princarg(Δφ − ω_k H)/H`;
  - peak picking (local max over ±2 bins, > −60 dB re max) with regions bounded at the magnitude minimum, storing `peakOf[k]` and `Δ_k`;
  - per-engage draws (detune r_k, walk rate ρ_k log-uniform 0.2–3 Hz) from `specEngageCounter`.
- DC and Nyquist bins are kept real.
- An identity round-trip test confirms JUCE's inverse-FFT scaling before 1/1.5 is relied on.

**Verification:**
- [x] A unit render of FFT → IFFT identity returns the input (documents the scale factor).
- [x] The hop counter advances only with `writePosition` (it stops during the WR-03 tail hold).
- [x] Fallback and rolling captures both fire in a scripted test (engage < N·H after an engine switch).

**Dependencies:** Task 2
**Estimated effort:** Large

---

### Task 4: SpectralFreeze — resynthesis, β, locking, drift, detune

**Outcome:** `synthesiseFrame()` (one per H on the synth clock, a single function so that RESEARCH Alt 2 stays a contained change) does the following:
- β = clamp(DIFFUSION + lfo·depth, 0, 1), sampled at the hop sample.
- Free phase `φᶠ += ω̂H·2^(r_k·cents/1200) + β·π·(2u−1)`.
- Soft identity lock `λ(β) = max(0, 1−β/0.3)²`.
- OU drift `d ← a d + sqrt(1−a²)·√3·(2u−1)`, with σ = Drift % × 1.5 dB.
- The phase noise, drift and detune factors are **shared across L/R**.
- IFFT × Hann × (1/1.5) × makeup(β) (D2) × spectral trim, overlap-added into the ring.
- On capture, the OLA is primed with 4 frames.
- Engine-switch crossfades in both directions; a mid-freeze switch to Granular uses `startGranularVoices`.
- Re-engage during a spectral tail continues without recapture.
- A fixed spectral trim makes spectral β = 0 match the granular density-law RMS within 1 dB.

**Verification:**
- [x] Frozen output carries signal on chord + pink noise at 48 k and 192 k.
- [x] D2 refit: steady RMS within ±3 dB of β = 0 over β ∈ {0, .25, .5, .75, 1} for chord and pink noise. The constant is recorded in SUMMARY.
- [x] Granular-vs-Spectral (β = 0) RMS within 1 dB on the gate stimulus.

**Dependencies:** Task 3
**Estimated effort:** Large

---

### Task 5: Spectral gates + alloc / worst-block harness

**Outcome:** `tests/spectral/gate.py` covers the RESEARCH metrics table, and `blocksize-null/gate.py` gets Spectral configs (manual + threshold; β 0/0.6/1; drift; detune; LFO). A C++ render harness (ported from O-Bells) counts allocations on the processBlock thread and records the worst block time.

**Verification (thresholds from RESEARCH § Objective Metrics):**
- [x] Block-size null 32 / 441 / 7 vs 512 ≤ −120 dBFS; seed-distinct templates must NOT null.
- [x] β monotone: chord peak-to-floor strictly ↓, hop-phase circular variance strictly ↑; β = 0 CV < 0.05.
- [x] Metallic: on pink noise, β = 1, drift 0, residual std (PSD_dB − ⅓-oct smoothed) is N = 8 ≤ 0.6 × N = 1.
- [x] Hop-rate AM < 0.5 dB at fs/H and 2fs/H (β = 0 chord).
- [x] Clicks on engage, release and engine switch while frozen: ≤ 1.5× the same-motion Mix-0 control (memory: *step click gate*).
- [x] 0 allocations with Spectral engaged, including the fallback path and both switch directions. The hook is proven live by a deliberate malloc.
- [x] Worst processBlock at block 32 / 48 k over 20 s frozen is < 333 µs. **If it fails → implement Alt 2 (amortised bins) inside `synthesiseFrame`, then re-gate.**
- [x] Task 2 Granular null re-run: PASS.

**Dependencies:** Task 4
**Estimated effort:** Large

---

### Task 6: UI + i18n

**Outcome:**
- `PluginEditor`: a combo relay for ENGINE and slider relays for DIFFUSION and AVG_FRAMES. They are **appended**, and every relay is declared before the attachments.
- `index.html`:
  - an Engine segmented toggle (captions are the choice strings verbatim, untranslated per the Mode precedent);
  - the Size knob swaps to Diffusion and the Grains knob to Avg in Spectral (D1), and Reverse dims;
  - the LFO group label is engine-dependent ("Drift LFO" ↔ "Diffusion LFO", new key).
- `i18n.js`: EN/FR/zh labels and tooltips. These include the Spectral variants of Drift ("per-bin level walk") and Detune ("spread / shimmer", not pitch — F2), and Avg ("applies at next freeze").
- Readouts use `getScaledValue()` (memory).
- Per memory, check the French width budget and pin content-sized boxes at the widest language.

**Verification:**
- [x] check-i18n, fr/zh lint and check-ui-labels PASS.
- [x] `tests/ui_tip_render_check.js` PASS, with i18n-states.json extended for Spectral.
- [x] Headless capture in both engines × EN/FR/zh: no overflow, and the swapped knobs bind to the right params (moving the Diffusion knob changes DIFFUSION, not GRAIN_SIZE).
- [x] Preset load / engine automation updates the swap live, so no stale one-shot state push happens (memory: *one-shot push stale on preset*).

**Dependencies:** Task 1 (param IDs). It can run in parallel with Tasks 3–5, but it lands after Task 5.
**Estimated effort:** Medium

---

### Task 7: Build, validate, docs

**Outcome:**
- CMake version set to 2.9.0.
- Build and install with `./scripts/build-and-install.sh O-Freeze`.
- pluginval strictness 10 and auval.
- Granular null + spectral gates re-run on the **installed** binary (memory: *measure the INSTALLED VST3*).
- CHANGELOG `## [2.9.0]` entry in bracket form.
- README/USER_GUIDE Spectral section; PLUGINS.md row.

**Verification:**
- [x] pluginval 10 PASS, `auval -a | grep -i freeze` + auval validate PASS.
- [x] All gates PASS on the installed build.
- [x] Build has no new warnings.

**Dependencies:** Tasks 5, 6
**Estimated effort:** Small

---

## Dependency Graph

```
Task 1 ──> Task 2 ──> Task 3 ──> Task 4 ──> Task 5 ──┐
   │                                                  ├──> Task 7
   └────────────────────> Task 6 ─────────────────────┘
```

**Parallelizable:** Task 6 (UI) can proceed alongside Tasks 3–5 once Task 1 fixes the param IDs.

## Execution Order

| Order | Task | Dependencies | Parallelizable With |
|-------|------|--------------|---------------------|
| 1 | T1 Params + wiring scaffold | — | — |
| 2 | T2 v2.8.0 Granular null | T1 | T6 |
| 3 | T3 Analysis + capture | T2 | T6 |
| 4 | T4 Resynthesis | T3 | T6 |
| 5 | T5 Gates + alloc/worst-block | T4 | T6 |
| 6 | T6 UI + i18n | T1 | T2–T5 |
| 7 | T7 Build, validate, docs | T5, T6 | — |

---

## Risk Notes

1. **Granular regression.** If it happens, the v2.8.0 null fails and existing sessions change sound.
   - Mitigation: the `e == 0` branch, the draw-for-draw extraction, and the T2 gate as a wall before any DSP lands, re-run after T4, T5 and T7.
2. **Level step between engines / across β (F1).** If it happens, an engine switch or a β sweep jumps in level.
   - Mitigation: D2 refit and the spectral trim in T4, gated in T4/T5.
3. **Per-hop CPU burst.** If it happens, a dropout at a 32-sample buffer.
   - Mitigation: the worst-block gate in T5; Alt 2 is pre-scoped inside `synthesiseFrame()`.
4. **Phase-lock region errors on chords.** If they happen, side bins jump and the sound warbles at β = 0.
   - Mitigation: regions bounded by the magnitude minimum, covered by the chord CV < 0.05 gate in T5.
5. **Param order / automation.** If it happens, old sessions misload.
   - Mitigation: append only (T1), with an old-state load check.
6. **Swapped-knob binding (D1).** If it happens, the UI writes GRAIN_SIZE while showing Diffusion.
   - Mitigation: the separate relays + attachments and the T6 binding check.

---

## Domain Agent Instructions

**Execute Agent:** general-purpose. Do the DSP first (T1–T5), then the GUI (T6), then T7.

**Domain-specific rules:**
- No allocation, lock or FFT-object construction in processBlock. All buffers are sized in `prepareToPlay` for 192 k.
- All scheduling (analysis hop, synth hop, RNG counters) runs on sample counters reset in `prepareToPlay` and on seed change. Nothing is keyed on host block boundaries.
- Never touch the Granular path's `grainCounter` / `engageCounter` increments. Spectral owns `specHopCounter` / `specEngageCounter`.
- Branch on `e == 0`; never multiply the granular sum by 1.0f.
- Relays are declared before attachments, and new members are appended.
- Path-scoped commits only (`git commit -- plugins/O-Freeze PLUGINS.md`). Re-check the branch and status immediately before committing. No tags.
- Run long renders and builds in the background (600 s watchdog).

**Files to read first:**
- `Source/PluginProcessor.h/.cpp`: the per-sample loop, `setFrozen` (358–383), control tick (412–457), seed reset (471–477), WR-03 hold (725, 819).
- `Source/CounterRng.h`: the counter-RNG API.
- `tests/blocksize-null/gate.py`: the template/seed harness to extend.
- `plugins/O-Bells/tests/render-harness/main.cpp`: the alloc hook to port.
- `~/JUCE/modules/juce_dsp/frequency/juce_FFT.h:85–115`: the real-only buffer contract.
- `RESEARCH.md` § Recommended Approach + § Objective Metrics.

---

## Success Criteria

1. [x] Engine = Granular bit-exact vs v2.8.0 (`26bbaeb6`)
2. [x] Spectral bit-exact across block sizes 32/441/7 vs 512; reproducible from seed
3. [x] β sweep monotone; β = 0 CV < 0.05
4. [x] Avg N = 8 ≤ 0.6 × N = 1 metallic residual
5. [x] 0 allocations in processBlock with Spectral engaged
6. [x] No click on engage / release / engine switch
7. [x] check-i18n, fr/zh lint, check-ui-labels PASS
8. [x] Build without warnings; pluginval 10, auval PASS
9. [x] All tasks completed and verified

---

*Generated by improve-milestone plan phase*
