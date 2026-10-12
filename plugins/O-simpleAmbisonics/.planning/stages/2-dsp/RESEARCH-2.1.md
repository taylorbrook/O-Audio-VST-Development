# Stage 2 — DSP · Phase 2.1 (Source → Encoder → Stereo decode) — Research

**Plugin:** O-simpleAmbisonics
**Stage:** 2 of 4 — DSP
**Phase:** 2.1 of 3 — audible chain, no rotation, no HRTF
**GSD phase:** research
**Date:** 2026-10-12
**Branch:** `main` @ `48fa56f8` (CONTEXT-2.1 committed)
**Inputs:** `CONTEXT-2.1.md` (D1–D9, R1–R7), `research/ARCHITECTURE.md` (immutable), `ROADMAP.md` §Phase 2.1, `REQUIREMENTS.md`, Stage-1 source (`Source/`, `tests/render-harness/`), O-GrainScatter `Source/dsp/{SpatialEncoder,TripleBuffer}.h`, O-Octagon + O-Strata harnesses, `research/ambisonics-encoding-deep-dive.md` §2, §3.7, Appendix B, JUCE 8.0.15 local source, project memory (`index_dsp_gate_design` and its leaves)
**Numerics:** every number below was computed, not estimated — script `r3_r4_r5.py` (scratchpad, reproduced inline where it matters)

---

## Entry check

| Item | State |
|---|---|
| Contract drift | None. ARCHITECTURE.md, ROADMAP.md §2.1, REQUIREMENTS.md unchanged since the 2.1 discuss |
| Stage-1 seam compiled against by the harness | `SpatialEngine::{prepare, reset, setParams, process(mono, L, R, n)}`, `kPartition = 64`, `DecoderDelay = FixedDelay<64>` — unchanged |
| Harness build | `build/CMakeCache.txt` has `OUARICON_BUILD_TESTS:BOOL=ON`; `build/plugins/O-simpleAmbisonics/tests/` exists — the 2.1 gates land in the existing target `O-simpleAmbisonics-render-test` |
| Precedent read | O-Octagon `tests/render-harness/main.cpp` (probe AO allocation counter, v1.15.0 `worstCurvature` click metric); O-Strata `tests/render-harness/{main.cpp,README.md}` (same counter with an ASan compile-out, H5 excess-step zipper gate, `storm` click ratio) |

Scope discipline: no algorithm in ARCHITECTURE was re-opened. Findings below are (a) answers to R1–R7, (b) numbers that turn D4/D5/D7/D8 from intent into checkable statements, and (c) hazards found while verifying the discuss decisions against source.

---

## Answers to the open questions

### R1 — Click-gate metric: worst second difference against a matched control, ratio ≤ 2.0, on a 100 Hz tone

ARCHITECTURE's "no sample-to-sample jump > −60 dBFS relative" is unusable on a tone, as CONTEXT suspected. The numbers settle which metric discriminates. For a 1 rev/s azimuth sweep the worst per-block gain step of the stereo decoder (a non-lerped, snap-per-block implementation — the thing the gate must catch) compared with the tone's own first and second difference:

| Tone | Host block | step ÷ tone 1st-diff | step ÷ tone 2nd-diff |
|---|---|---|---|
| 1 kHz | 64 | 0.03 | 0.24 |
| 1 kHz | 1024 | 0.51 | 3.9 |
| 1 kHz | 4096 | 1.95 | 14.9 |
| 100 Hz | 64 | 0.32 | 24 |
| 100 Hz | 1024 | 5.1 | 390 |
| 100 Hz | 4096 | 19.5 | 1490 |

A first-difference metric at 1 kHz cannot see the defect at any block size up to 1024 (ratio < 1). The second difference at 100 Hz sees it at every block size including 64 (ratio 24). This is the same conclusion O-Octagon reached at v1.15.0 (`worstCurvature`, "a low tone exposes a click that a first-difference reading would bury under the programme's own slope").

**Metric (reuse for the order-fade probe and for every 2.2 / 2.3 click gate):**

```
curvature(y)[n] = | y[n] − 2·y[n−1] + y[n−2] |
event   = max curvature over the event window (the sweep / fade), both channels
control = max curvature of a MATCHED static render (same tone, same gain staging,
          parameter parked at the direction of maximum output level — az = +60° for L,
          −60° for R — so the control carries the largest legitimate curvature)
verdict = event / control ≤ 2.0   (O-Octagon kClickRatioCeiling)
```

Stimulus: 100 Hz sine, amplitude 0.5, exactly 480 samples per cycle at 48 kHz (`pattern_exact_cycle_gate_needs_exactly_representable_f0`), 1 s sweep = one full circle, rendered at **64-sample blocks and at 4096-sample blocks** (both; the 4096 run is where a chord/lerp defect would show — see R3). Skip the first 64 + 20 ms of each render (delay fill + any ramp). Print the absolute literals beside the ratio.

**Negative control, with no production switch:** the harness's own double-precision reference model (D8) is run twice over the sweep, once with the per-sample lerp and once snapping the coefficient vector per 64-sample block. The snapped model must FAIL the same detector (ratio ≥ 24 at 100 Hz / 64-sample blocks per the table). A detector that has not been made to fail is not evidence (`pattern_distinct_buffer_addresses_are_not_an_allocation_bound`, last paragraph). This control lives entirely in the harness.

Liveness (`pattern_zipper_sweep_probe_needs_liveness_gate`): the same sweep render must differ from the parked render (L energy over the second half of the circle differs by ≥ 6 dB from the first half) — a dead azimuth path would otherwise pass the ratio at exactly 1.0.

"Spectral flatness unchanged" from REQUIREMENTS DSP-06: a 1 rev/s pan sweep adds ±1 Hz sidebands by construction, so spectral flatness is not an invariant. Replace with: energy outside the 100 Hz bin ±2 bins (1 s window) relative to the bin ≤ −60 dB. Informational, not the verdict.

### R2 — Allocation counter: copy O-Octagon probe AO verbatim (thread-filtered `operator new` family), with the standing coverage statement

Two sibling harnesses carry a counter; the lineage is settled.

| Harness | Mechanism | Notes |
|---|---|---|
| **O-Octagon** `tests/render-harness/main.cpp:111-241` | Replaces all eight `operator new`/`delete` variants incl. both `std::align_val_t` overloads; `rtcheck::arm()` records the arming thread; `note()` tallies the arming thread into `allocations` and every other thread into `foreignAllocations`; `foreignNote()` is appended to every verdict | The thread filter exists because of a reproduced 4/40 false positive under load attributed every time to a non-audio thread (VERIFY-3.3 comment block). **Canonical** |
| O-Strata `main.cpp:52-150` | Verbatim copy of the above wrapped in `#ifdef STRATA_HARNESS_ASAN` (ASan owns the allocator) | Adds `checkFailsAsExpected()` negative-control idiom and a README coverage section |
| O-Bells (`--alloc-check`) | libmalloc `malloc_logger` hook — sees `HeapBlock`/malloc too | Two traps: `volatile` flag (clang folds the armed flag around `malloc()`), process-wide hook needs `pthread_equal` scope (`pattern_malloc_logger_alloc_gate_two_traps`). Not needed here: the 2.1 engine has no JUCE containers |

**Recommendation:** copy O-Octagon lines 111–241 with provenance comment (as O-Strata did); no ASan ifdef (ASan hangs at init on this toolchain anyway — `critical_asan_runtime_hangs_at_init_on_macos26`).

How the harness excludes its own allocations (`pattern_operator_new_counter_blind_to_heapblock_and_first_block_async_post`):

1. Build the `AudioBuffer` at its largest size and `midi.ensureSize(256)` **before** `arm()`.
2. One unarmed warm-up `processBlock` after `prepareToPlay` (absorbs any first-block lazy work — none is expected in this processor, but the warm-up costs nothing and makes the claim about steady state).
3. `arm()` → N `processBlock` calls (orders 1/2/3, both decoder values, an order change and a decoder change mid-window so both fades run armed, one 4096-sample block against a 64-prepared instance) → `disarm()` → assert `allocations == 0`, print `foreignNote()`.
4. **Liveness:** inside a separate armed window do one deliberate `new int` / `delete`; the counter must read exactly 1. Without it an un-replaced overload would pass vacuously (O-Bells found this within minutes).
5. Parameter changes inside the armed window go through `setValueNotifyingHost` on the harness thread — that path allocates nothing in JUCE 8 for a plain `RangedAudioParameter` with no listeners attached (no editor exists in the harness), but keep the changes **between** `processBlock` calls, never from another thread.

Coverage statement for SUMMARY-2.1 (standing wording, O-Strata README §"Allocation counter coverage"): *covered = every new-expression on the audio thread; not covered = `HeapBlock`/malloc paths (`AudioBuffer`, `Array`, `MidiBuffer`, `MemoryBlock`) and the `AsyncUpdater` post path — grep + inspection.* The 2.1 engine is JUCE-free (D3) and its only dynamic storage is `std::vector` sized in `prepare` (which IS `operator new`, so a `resize` in `process` would be counted). The processor's `processBlock` touches `buffer` (host-owned) and the `TripleBuffer` (fixed `std::array<T,3>`). Nothing in scope lives in the uncovered set.

### R3 — Chord error: unacceptable at D5's slice-rate update for long blocks; run the control path on a fixed 64-sample grid instead

D5 as written ("block" = one `process` call ≤ `maxBlock`, k from the slice length, lerp from the previous call's end vector) gives a 2 Hz orbit **61.4° per call** at 4096-sample blocks. Mid-block the lerped vector is the chord midpoint; its norm per order band against the exact midpoint direction:

| Control step | Degrees per step | Band 1 dip | Band 2 dip | Band 3 dip |
|---|---|---|---|---|
| 4096 samples @ 2 Hz (D5 as written) | 61.4° | **−1.31 dB** | **−3.75 dB** | **−5.56 dB** |
| 1024 samples @ 2 Hz | 15.4° | −0.08 dB | −0.23 dB | −0.47 dB |
| 64 samples @ 2 Hz | 0.96° | −0.00 dB | −0.00 dB | −0.00 dB (−0.002) |

(The order-3 band rotates `3·a`; at 61° per step the `sin 3a / cos 3a` terms move 184° — nearly antiphase — which is why the dip grows with order. ARCHITECTURE's "for small angular changes per block, the difference is inaudible" assumed small changes; 61° is not small.) Stereo-mode consequence at 4096 blocks: L mid-block 0.875 vs exact 0.936 (−0.59 dB), a periodic level wobble at the block rate — audible, and it would also make the DSP-04 and DSP-06 renders block-size dependent.

There is a second defect in slice-rate control that the chord number does not show: the output becomes **a function of the host's block slicing** whenever a parameter moves. A lerp over 64 samples to T1 followed by 64 to T2 is not the lerp over 128 samples to T2, so P2 (ragged ≡ flat) and P3 (oversized ≡ prepared) can hold only while every parameter is static and orbit is Off. CONTEXT D5 claims "P2 ragged / P3 oversized gates stay meaningful for the engine" — they stay meaningful only for the static case. The 2.3 convolver already solves the identical problem for the audio path with a 64-sample FIFO.

**Recommendation — D5 amendment (for the plan to adopt):** the engine keeps a control-grid phase counter `gridPos ∈ [0, 64)` that persists across `process` calls. Inside `process`, the per-sample loop runs in runs of `min(remaining, 64 − gridPos)`; whenever `gridPos` wraps to 0 the engine performs one **control step**: read `Params`, advance the orbit by `64/fs`, step every block-rate smoother with the **constant** `k = 1 − exp(−64/(τ·fs))` (0.0645 at 48 kHz; computed once in `prepare`), evaluate the encoder (two trig pairs), the distance law (one `tan`), the order gains, and set the lerp endpoints for the next 64 samples. Consequences:

- Chord error ≤ 0.002 dB at the fastest orbit (table row 3). Settles R3: no sub-block scheme beyond the grid is needed.
- Renders are **bit-identical across any slicing**, with parameters moving and orbit On, because the same operations run in the same order at the same absolute sample positions. P2/P3 can be extended with an orbit-On variant and still demand `memcmp` equality — a far stronger gate than the static one.
- Smoother time constants are block-size-invariant by construction (no `exp` per call).
- Control cost: one encoder evaluation + one `tan` + three `exp`-free one-pole steps per 64 samples ≈ 750 Hz control rate — ~40 flops per sample, below the per-sample lerp cost that exists anyway.
- The 2.3 UPOLS step lands on the same grid (`kPartition`), so the whole engine ticks at one control rate; the stereo `FixedDelay<64>` is unaffected.
- `setParams` is still called once per host block by the processor (D2 unchanged); the engine simply samples the stored `Params` at each grid boundary. A host block shorter than 64 samples may contain no control step — correct: the lerp continues toward the endpoints set at the last step.

What this does **not** change: τ = 20 ms everywhere, the wrap smoother, the per-sample lerp, D2's seam, D8's reference null (the model adopts the same 64-grid — it has to, to be bit-comparable). The lag note in CONTEXT stays true: a one-pole with τ = 20 ms trails a 2 Hz orbit by 14.1° (computed: `atan(2π·2·0.02)`), a constant phase offset.

### R4 — Independent DSP-01 table: generic triple product from §2.1–2.6, verified to 4.7e-16 against the ported constants

Written and run in double from the deep-dive's *general* formula, never from `encodeSH16`'s simplified constants:

```
Y_n^m(a, e) = N_n^|m| · P_n^|m|(sin e) · T_m(a)
N_n^|m|     = sqrt( (2 − δ_{m0}) · (n − |m|)! / (n + |m|)! )        (factorials in code)
P_n^|m|(x)  = explicit polynomials of §2.5 (no Condon–Shortley phase), x = sin e, sqrt(1−x²) = cos e
T_m(a)      = sin(|m|·a) for m < 0, 1 for m = 0, cos(m·a) for m > 0
ACN         = n·(n+1) + m,  n = 0..3, m = −n..n
```

Results on the 13 × 7 (az × el) grid:

| Comparison | max abs error |
|---|---|
| generic reference vs `encodeSH16` formulas (both double) | 4.7e-16 |
| generic reference vs `encodeSH16` computed in float32 | **2.8e-08** (gate 1e-5 — 350× margin) |
| Appendix B spot vectors (front ACN 13/15 = −0.6124 / +0.7906; left ACN 9/11 = −0.7906 / −0.6124) | reproduced |

ACN ↔ (n, m) and the m < 0 → `sin(|m|a)` assignment match AmbiX (Nachbar et al. 2011): no Condon–Shortley phase, SN3D, `sin` for negative m. So the gate is written as above, with its own `legendre(n, m, x)` table and `factorial()`; it shares **nothing** with `ShEncoder.h` but `<cmath>`. Also assert W ≡ 1 at every grid point and, as a sign anchor, ACN 1 (Y) > 0 at az = +90° (AmbiX left-positive — the DSP-04 "L louder at +90°" claim rests on this).

### R5 — DSP-05 "HF/LF ratio": two-tone 200 Hz + 12 kHz, exact-bin DFT, closed-form expectation; monotonic only on (1, 20] m

The gain law cancels in any ratio of two tones through the same `g(d)`, so the ratio isolates the air LP + mix. Expected values from the TPT one-pole's exact bilinear response `|H(f)| = 1/sqrt(1 + (tan(πf/fs)/tan(πf_c/fs))²)` and the D7 mix `|1 + m·(H − 1)|`:

| d (m) | f_c (Hz) | m | HF/LF (dB) | g(d) (dB) |
|---|---|---|---|---|
| 0.2 | 20000 | 0 | 0.000 | +8.87 |
| 1.0 | 20000 | 0 | 0.000 | 0.00 |
| 1.25 | 17889 | 0.25 | −0.298 | −1.58 |
| 1.5 | 16330 | 0.5 | −0.828 | −2.92 |
| 2.0 | 14142 | 1 | −1.949 | −5.11 |
| 4.0 | 10000 | 1 | −4.310 | −10.63 |
| 8.0 | 7071 | 1 | −7.001 | −16.39 |
| 20.0 | 4472 | 1 | **−10.788** | **−24.19** |

Non-increasing over [0.2, 20]: true. Strictly decreasing on (1, 20] (4000-point scan): true, minimum step 0.001 dB per 4.75 mm. **Flat on [0.2, 1]** by design (m = 0). So the DSP-05 criterion must read: *level strictly decreasing over 0.2 → 20 m; HF/LF ratio exactly 0 dB (bit-level: wet mix 0) for d ≤ 1 m and strictly decreasing for 1 < d ≤ 20 m.* "Strictly decreasing HF/LF 0.2 → 20" as written in CONTEXT would fail on the correct implementation.

Stimulus: 200 Hz + 12 kHz, equal amplitude 0.25 each, 1 s window → exactly 200 and 12 000 cycles, so a plain DFT at the two bins is leakage-free; 12 kHz < 0.45 × 44 100 = 19.8 kHz so the probe survives 44.1 kHz. Anchors within 0.1 dB: 0 dB at 1 m (exact), −10.79 dB at 20 m, and level g(d) at 0.2 / 1 / 20 m (+8.87 / 0 / −24.19 dB) measured on the 200 Hz bin. Distance grid for monotonicity: 25 log-spaced points on [0.2, 20] plus 1.0 exactly.

**Correction to D7's motivation (D7 itself stands):** CONTEXT says a first-order pole at the 0.45·fs clamp is "−0.85 dB at 10 kHz". That is the *analog* prototype; the TPT one-pole is a bilinear design whose response is warped to reach zero at Nyquist, and at f_c = 21.6 kHz it reads **−0.06 dB at 10 kHz, −0.11 dB at 12 kHz** (closed form above). The LP at the clamp is nearly transparent on its own. D7's wet mix remains the right implementation because D8's reference null needs d ≤ 1 m to be *exactly* `x` (not −0.06 dB of it) and because `m` gives "1 m = no effect" as a bit-level fact the gate can assert. Record the corrected number so the tooltip/lesson text in Stage 3 does not repeat −0.85 dB.

### R6 — `TripleBuffer.h` is not a registered module; copy it with provenance, flag for extraction after Stage 2

`modules/registry.yaml` (v1.2.1) has no triple-buffer entry; `modules/metering/` holds only `vu-meter`. Two verbatim copies exist: `O-GrainScatter/Source/dsp/TripleBuffer.h` (origin) and `O-simpleGrain/Source/dsp/TripleBuffer.h` (identical body, a six-line provenance comment added). O-simpleAmbisonics becomes the third consumer — the usual extraction threshold — but `/module-create` is Stage-4/repo-level housekeeping and out of 2.1 scope. **Copy verbatim, keep the AGPL header, add the provenance comment in the O-simpleGrain form, and list "TripleBuffer → `core/triple-buffer` module candidate (3 consumers)" in SUMMARY-2.1's deferred items.** Two additions worth making in our copy (they do not change behaviour): `static_assert(std::is_trivially_copyable_v<T>)` (the frame crosses threads by value), and a comment that `read()` must be called from exactly one thread.

The SPSC protocol was re-read: `publish()` swaps the write slot with `middle` (acq_rel) and raises `newData`; `read()` consumes `newData` then swaps `middle` into the read slot. Wait-free, no ABA on indices because each side only ever touches its own index plus the atomic middle. Correct for one writer (audio) and one reader (Timer).

### R7 — Publish once per host block; `shPeak` and `outPeak` are the max over the whole host block; the writer resets its peak accumulators only after the reader has consumed a frame

Siblings publish exactly once per `processBlock` after the render (O-GrainScatter `PluginProcessor.cpp:786-828`, O-simpleGrain `:899-928`) and the editor reads at 30 Hz (`startTimerHz(30)` in O-simpleFM / O-simpleGrain). Neither carries level meters through the frame, so there is no precedent for the peak question; reasoning from the cadence instead:

- At 64-sample host blocks the audio thread publishes 750 frames/s and the UI reads 30: **96 % of frames are never seen**. For `shGain`, `azEff`, `orderActive`, `decoderMix` that is correct (latest state wins). For `shPeak[16]` / `outPeakL/R` it means a 30 Hz meter would display 1 in 25 blocks' peaks — under-reading transients, flickering.
- Fix with two lines on the writer side, still wait-free: before filling the frame, `if (! meters.hasNewData())` the reader has consumed the last published frame → reset the running peak accumulators to the current block's peaks; else keep `max(accumulated, thisBlock)`. The published frame then carries **peak-since-the-UI-last-looked**, which is exactly meter semantics, independent of block size and of the Timer rate. `hasNewData()` is an acquire load on the writer thread — no contention.
- Under the slice loop (host block > `maxBlock`) the processor takes `max` across the slices' `engine.lastFrame().shPeak` and uses the last slice's state fields — confirming CONTEXT's instinct. With R3's control grid this is simply "the engine accumulates peaks until the processor calls `engine.resetPeaks()` at the top of each host block"; simplest ownership: the **engine** owns peak accumulation across `process` calls and exposes `takeFrame()`; the processor's only job is the `hasNewData()` test and `publish()`.

Stage-3 reader contract to write into `MeterFrame.h` now: fields are plain `float`/`int` (trivially copyable), `shPeak` is a hold value already (JS applies decay only), `latencySamples` is constant 64, and a frame with `orderActive` = 4/9/16 is the FUNC-03 meter-count criterion.

---

## Findings beyond R1–R7

### F1 — One-pole smoothers never reach their target exactly; the "exactly 0 / exactly 1" criteria need a linear ramp or a snap *(HIGH — three CONTEXT criteria depend on it)*

`cur += k·(target − cur)` converges asymptotically. With k = 0.0645 per 64-sample step, an order gain fading 1 → 0 reaches 1e-38 (where flush-to-zero finally makes it 0) after **1312 control steps = 1.75 s**, and only if FTZ is on — the engine-direct harness gates run without `ScopedNoDenormals`. Three criteria are stated as exact: unused order bands "reaching exactly 0" (order-fade probe, DSP-07 wording), "LP mix exactly 0 at d ≤ 1 m" after a move (DSP-05), and the P1 anchor (`orderGain[1]` must be exactly 1 — fine when seeded, broken after any order change).

Recommendation, two parts:

1. **Order gains and the decoder mix are linear ramps**, not one-poles: at each control step move by `64/(0.02·fs)` (1/15 at 48 kHz) toward the target and clamp — exactly 0 or 1 after 15 steps (20 ms), matching ARCHITECTURE's "20 ms fade" and the DSP-07 "unused channels zeroed" wording literally. Per-sample lerp of `c_eff` is unchanged.
2. **Every one-pole smoother snaps when within 1e-6·max(1, |target|)** of its target (one compare per control step). Gain `g`, cutoff `G`, mix `m`, elevation and the wrap smoothers then *arrive*, so a static render after a move is bit-identical to a fresh render at the same setting — which is also what makes "fresh instance per render" (D9) and "after a parameter change" renders comparable.

### F2 — The mixer must be written `stereo + mix·(binaural − stereo)` for D6's placeholder to be exact *(MEDIUM)*

With `out = mix·a + (1 − mix)·b` and identical legs `a == b`, float gives `mix·a + (1−mix)·a ≠ a` in general (two roundings). With `b + mix·(a − b)` and `a == b` the result is exactly `b` for any `mix`. This keeps D8's "exactly 0.75·s[n−64]" true in **Binaural** mode (the installed default) and not only in Stereo mode, and removes one rounding from the real 2.3 crossfade. Same form for the air-LP mix (`x + m·(lp − x)`, already D7) and for the per-sample coefficient lerp (`prev + α·(cur − prev)`: exact when `cur == prev`).

### F3 — D8's "exactly 0.75·s[n−64]" is achievable bit-exactly, if the decoder matrix uses exact literals *(MEDIUM — tells the plan how to write the anchor)*

Chain at front / 1 m / order 1 / W = 1: `g(1) = 1.25f/1.25f = 1` exactly; `m = 0` → `y = x` exactly (F2 form); encoder `X = cos 0 · cos 0 = 1`, `Y = 0` exactly; `orderGain[1] = 1` (seeded; F1 keeps it so); lerp endpoints equal → exact. Decoder: write the matrix as `0.5f·W + 0.25f·X ± 0.4330127f·Y` — the `0.25f` (= 0.5·cos 60°) must be the **literal**, not `0.5f * std::cos(...)` (float `cos(π/3)` is 0.49999997, and the anchor would miss by 1 ulp). Then `L = 0.5·s + 0.25·s + 0.433·0`: both products exact (power-of-two scaling), the sum is the correctly rounded `0.75·s`, identical to `0.75f * s` whether or not clang contracts to an FMA. Processor gain at 0 dB is exactly 1.0. **So assert the anchor bit-equal** (`memcmp`), with the general null's < 1e-5 covering the non-anchor settings. If the executor finds a 1-ulp miss, the cause is an inexact literal or a non-F2 lerp form, not a tolerance problem.

### F4 — `wrapTo180` with `floor` returns [−180, 180), ARCHITECTURE writes (−180, 180] *(LOW — pick one, write it down)*

`x − 360·floor((x + 180)/360)` maps +180 → −180. Harmless for the smoother (the delta is what matters) but the published `azEff` for a source parked at exactly +180° will read −180°, and the DSP-06 trajectory check "|wrapTo180(cur − 180)| ≤ 10°" is symmetric so it does not care. Keep `floor` (one call, no branch), document [−180, 180) in `Smoothers.h`, and have the Stage-3 readout format −180 as 180 if Taylor wants the UI to show +180.

### F5 — The engine must snap all smoothers from `Params` inside `prepare`, so `setParams` precedes `prepare` *(MEDIUM — seam order)*

CONTEXT asks for gain seeded from the parameter in `prepare` (no fade-in). That generalises: `prepare` snaps **every** smoother (angles, elevation, g, G, m, order gains, decoder mix, orbit phase = 0) to the current `Params`, so block 0 is at steady state and P5/P4-style "exact at the first block" gates hold for the engine. The processor therefore fills `Params` from the atomics **before** `engine.prepare(fs, maxBlock)` in `prepareToPlay` (the harness `makeProc` pre-setup lambda already sets parameters before `prepareToPlay`, so the existing probe shape carries over). Finite-scrub note: `snapTo` writes finite values, so no NaN can survive a re-prepare (`critical_smoothedvalue_reset_does_not_clear_a_nan` is about `juce::SmoothedValue`, which 2.1 no longer uses in the engine).

### F6 — Engine-direct gates need their own denormal handling *(LOW)*

`juce::ScopedNoDenormals` lives in `processBlock`; the DSP-01/04/05/06 gates drive `SpatialEngine` directly. A TPT pole and the (F1-snapped) smoothers decay quickly, but the harness should wrap engine-direct renders in `juce::ScopedNoDenormals` as well so a denormal tail cannot make an engine-direct render differ from a processor render of the same thing.

### F7 — The stereo decoder is non-negative for every direction including elevation; the DSP-04 margin is 2.9 dB *(confirms the gate thresholds)*

`V = 0.5·W + 0.5·(cos 60°·X + sin 60°·Y)`, with `(X, Y) = cos e·(cos a, sin a)`: `V ≥ 0.5 − 0.5·cos e ≥ 0`. In-phase everywhere, so "L·s and R·s correlation ≥ 0" holds analytically. At az = +90°, e = 0: L = 0.9330, R = 0.0670 → **22.9 dB**, i.e. 2.9 dB above the ≥ 20 dB criterion; with F1 the static render is exact so the margin is real, not slack. At e = ±90°: L = R = 0.5 exactly (no singularity; the "stereo has no height" lesson). L = R at front: 0.75 both (D8).

### F8 — P2/P3 keep their static meaning now and gain a dynamic variant under R3 *(informs the plan)*

Keep P2/P3 as written (defaults, orbit Off) — they must still `memcmp`-match. Add **P2b / P3b**: orbit On at 2 Hz, order 3, decoder Stereo, same ragged / oversized slicings → still `memcmp`-equal to the flat render. These pass only with the 64-sample control grid (R3) and fail on slice-rate control, so they are also the gate that pins the D5 amendment.

### F9 — Harness-side reference model scope *(informs the plan)*

The D8 model is a straight transcription of the chain in double: `g(d)`, bilinear one-pole with lerped G (same TPT difference equations in double), D7 mix, SN3D encode from the **R4 generic table** (not from `ShEncoder.h`), linear order gains, ±60° matrix, 64-sample delay, with the same 64-sample control grid and the same lerp forms. It is used by: P1 (null < 1e-5 at a static non-trivial setting, e.g. az 37°, el −12°, d 3.3 m, order 3, Stereo; plus the bit-exact front anchor), R1's negative control (snap vs lerp), and DSP-06's trajectory expectation. ~120 lines; keep it in a separate `tests/render-harness/ReferenceModel.h` so 2.2 can extend it with a rotation matrix.

---

## Reuse — existing code and precedent

| Source | Reuse | Notes |
|---|---|---|
| `O-GrainScatter/Source/dsp/SpatialEncoder.h` | `encodeSH16` + `AmbiTrigCache` + constants, lifted into `ShEncoder.h` | Verified against the generic formula to 4.7e-16 (R4). Takes **radians**; the engine converts once per control step. `SmoothedSHCoeffs` (per-sample one-pole on coefficients) is **not** lifted — 2.1 uses block-rate smoothing of angles + per-sample lerp of the vector |
| `O-GrainScatter/Source/dsp/TripleBuffer.h` | Verbatim copy + provenance | R6; module extraction deferred |
| `O-Octagon/tests/render-harness/main.cpp:111-241` | Allocation counter (`rtcheck`) verbatim | R2 |
| `O-Octagon/tests/render-harness/main.cpp:967-1031` | `worstCurvature` + `ClickReading` + `kClickRatioCeiling = 2.0f` | R1; the `renderHooked` 64-block renderer is also the right shape for the sweep driver (hook sets azimuth before each 64-block) |
| `O-Strata/tests/render-harness/main.cpp:167-175` | `checkFailsAsExpected()` negative-control idiom | R1 control, F8 |
| `Source/dsp/FixedDelay.h` (Stage 1) | `DecoderDelay` ×2 in `DecoderMixer` | Unchanged |
| `juce::dsp::FirstOrderTPTFilter` (`juce_FirstOrderTPTFilter.cpp:89-96, 120`) | **Read, not used** — confirms D4's topology (`v = G·(x − s); y = v + s; s = y + v`) and `G = g/(1+g)`, `g = tan(π f_c/fs)` | Own 6-line version per D4 so G can be lerped per sample; TPT is the structure that stays stable under per-sample coefficient change |
| `modules/` registry | **No applicable module** | Checked v1.2.1: nothing covers SH encoding, smoothing, or the triple buffer |

No new third-party dependency.

---

## Pitfalls carried from the knowledge base

| Pattern | Bites where in 2.1 |
|---|---|
| `pattern_gate_stimulus_below_threshold_is_vacuous` | R1 — a 1 kHz tone cannot see a 64-block zipper; the stimulus (100 Hz) was chosen from the table, and the snapped-model control proves the detector is live |
| `pattern_zipper_sweep_probe_needs_liveness_gate` | R1 liveness half; also DSP-05 (a dead `distance` path reads a flat 0 dB ratio, which the "exactly 0 at ≤ 1 m" clause would *reward* — the anchors at 20 m are the liveness) |
| `pattern_zipper_gate_absolute_step_is_gain_staging_dependent` | R1 — ratio vs a matched control, never an absolute step |
| `pattern_distinct_buffer_addresses_are_not_an_allocation_bound` | R2 liveness `new` inside an armed window; R1 negative control |
| `pattern_operator_new_counter_blind_to_heapblock_and_first_block_async_post` | R2 steps 1–2 and the coverage statement |
| `pattern_exact_cycle_gate_needs_exactly_representable_f0` | 100 Hz (480 samples), 200 Hz (240), 12 kHz (4) at 48 kHz — all integer |
| `pattern_gain_ramp_without_seeding_fades_in_from_silence` | F5 — every smoother snapped in `prepare` |
| `pattern_sc_filter_needs_nyquist_clamp` / `pattern_downstream_guard_masks_defect_gate_the_clamp_target` | Air LP clamp at 0.45·fs is unreachable with d ≥ 0.2 at 48 kHz (f_c ≤ 20 kHz); at 44.1 kHz it engages for d < 1.016 m — gate: d = 0.2 and d = 1.0 render **bit-identically** at 44.1 kHz (same clamped request; m = 0 anyway) and the 12 kHz tone's ratio is exactly 0 dB |
| `critical_smoothedvalue_reset_does_not_clear_a_nan` | F5 — engine smoothers `snapTo` finite values |
| `pattern_oversampler_overruns_on_oversized_host_block` | Engine scratch sized to `maxBlock`; processor slice loop already present; P3/P3b |
| `pattern_metric_window_vs_modulation_period` | R5 uses a 1 s window on a *static* render; R1's window is the whole sweep — curvature is a per-sample max, not an average, so the window cannot wash it out |
| `pattern_test_fixture_mirrors_drift_silently` | R4 — the reference table is derived from factorials and §2.5 polynomials, not a copy of the 16 constants |
| `critical_asan_runtime_hangs_at_init_on_macos26` | R2 — counters, not ASan |
| `pattern_render_harness_breaks_on_webview_editor` | Harness still `JUCE_WEB_BROWSER=0`, no editor TU — unchanged this phase |

---

## Open items handed to the plan phase

1. **Adopt the D5 amendment (R3):** fixed 64-sample control grid with a persistent `gridPos`, constant `k`, control step at each grid boundary. Plan should name it as the amendment it is and add P2b/P3b (F8) as its gate.
2. **Order gains + decoder mix as linear ramps; one-poles snap at 1e-6 (F1).** Decide whether the ramp step lives at the control grid (recommended, 1/15 per step) or per sample.
3. **Mixer and lerp forms per F2** (`b + t·(a − b)`), decoder matrix with exact literals (F3), anchor asserted bit-equal.
4. **DSP-05 criterion wording (R5):** HF/LF exactly 0 dB on [0.2, 1], strictly decreasing on (1, 20]; anchors 0 / −10.79 dB; level anchors +8.87 / 0 / −24.19 dB; 44.1 kHz clamp-target identity check.
5. **R1 detector placement:** one `clickRatio(event, control)` helper shared by the DSP-06 sweep (64 and 4096 blocks), the order-fade probe (1 → 3 → 1 mid-tone), and later 2.2/2.3 — put it beside `rtcheck` so 2.3 does not re-derive it.
6. **`ReferenceModel.h` (F9)** as its own header under `tests/render-harness/`.
7. **Meter ownership (R7):** engine accumulates peaks and exposes `takeFrame()`; processor does `hasNewData()`-gated reset + `publish()`. `MeterFrame.h` fields fixed now for Stage 3.
8. **Provenance comments** on `ShEncoder.h` (O-GrainScatter, verified R4) and `TripleBuffer.h` (R6); SUMMARY-2.1 deferred list gets the module-extraction candidate and the D6 placeholder obligation.
9. **D7 motivation number** corrected to −0.06 dB at 10 kHz (R5) wherever the lesson text is drafted.
10. **SUMMARY-2.1 coverage statement** for the allocation gate (R2 wording).

## Still-open manual gates

Unchanged from Stage 1 verify: Logic stereo + mono track, Generic editor 12 rows, project save/reopen — formal under COMPAT-02 at Stage 4. Nothing in 2.1 depends on them; the installed-plugin hands-on listed in CONTEXT §Success criteria is the 2.1 verify's human item.
