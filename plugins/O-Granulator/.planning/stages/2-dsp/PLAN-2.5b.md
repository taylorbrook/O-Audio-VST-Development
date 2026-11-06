# Stage 2 — DSP · Phase 2.5b (Onset detector + burst, MPM pitch analysis, f0 history ring, absolute / interval pitch follow) — Plan

**Plugin:** O-Granulator
**Stage:** 2 of 4 (DSP)
**Phase:** 2.5b of 7
**GSD phase:** plan
**Date:** 2026-11-05
**Branch:** `main` (trunk-based) @ `4761137d`
**Inputs:**
- `CONTEXT-2.5b.md` (D17–D20 locked, the N3 null, carry-ins, execution defaults)
- `RESEARCH-2.5b.md` (R59–R78). **Where it differs from ARCHITECTURE.md, ROADMAP.md or CONTEXT-2.5b, RESEARCH-2.5b wins**, as amended by the sign-offs below.
- `SUMMARY-2.5a.md`, `VERIFICATION-2.5a.md` (harness 299/299; C5 16.26 % at execute, 16.63 % at verify; deviations 1–12 accepted; the item 6 watch; W2o +0.483 dB with a 0.017 dB margin; FUNC-06 partial; the QUAL-01 / PERF-01 / PERF-03 2.5b remainders; D16 DAW check deferred to Stage 3)
- `research/ARCHITECTURE.md` §4, §5, §15, Telemetry, Algorithm §5, §7, §8; `ROADMAP.md` Phase 2.5b; `REQUIREMENTS.md` (FUNC-06, FUNC-08, DSP-08, DSP-01); `parameter-spec.md` (LOCKED: `scale_root`, `sN_onset_trigger`, `pitch_follow_sN`, `onset_threshold`, `sN_scale`, `degreeMasks`, `customScale`)
- The 2.5a engine at `2cf30120`: `Source/dsp/` (`Engine.*`, `StreamScheduler.h`, `Spawner.h`, `ScaleQuantiser.h`, `ModSources.h`, `ModMatrix.h`, `ControlGrid.h`, `EngineConstants.h`), `PluginProcessor.*`, `tests/render-harness/`, `tests/n2/` (the template for n3), `tests/n1/`, `tests/n0/`, `tests/r12/`, `tests/hostcache/run.py`
- grain-engine 1.1.0 `GrainScheduler.h` / `CounterRng.h`, pitch-collections 1.0.0, JUCE 8.0.15 `juce_FFT.cpp` (all read at plan)

---

## Sign-offs (Taylor, 2026-11-05, at plan)

| Item | Decision |
|---|---|
| **R60** — onset trigger (ARCH §8 / §15 erratum) | **Edge-triggered with a +3 dB re-arm.** Fire when the condition (`fast > thrLin` AND `fast > 10^(6/20)·slow`) holds AND the detector is armed AND the refractory has expired. Firing disarms. Re-arm when `fast < 10^(3/20)·slow`. One event per sustained onset (ARCH's level trigger re-fires at 0 / 50 / 100 ms). |
| **R66** — D20 amendment | **Median-of-3, then an asymmetric harmonic-class lookahead confirm with N = 3.** A jump from the accepted note by ±7.02 / ±12 / ±19.02 / ±24 st (each ±50 ct) is accepted only if the next 3 medians agree within ±0.5 st. A return to the accepted note, or a non-harmonic move, is immediate. It replaces the symmetric 2-hop octave hysteresis. |
| **R64** — lookup point (narrows D17 "read start") | f0 is looked up at the **content midpoint** `readStart + dir·rl/2` of a provisional plan. Two passes: provisional interval-mode plan → lookup → re-plan pitch → rate → delay. |
| **R71** — absolute snap | Absolute mode with pitch_rand 0, or with no member inside the span, snaps to the **nearest** member (ties up). Interval mode keeps "offset 0". |

---

## Entry Check — contract checksums

Re-hash all six at the start of execute (`shasum -a 256`) against the STATUS.md frontmatter (ROADMAP `1060dae0…`; REQUIREMENTS `f0b15388…`, the 2.5a-verify revision). Before T1 the executor confirms:
- `git diff --stat 2cf30120 HEAD -- plugins/O-Granulator/Source plugins/O-Granulator/tests plugins/O-Granulator/CMakeLists.txt` is empty (checked at plan @ `4761137d`: empty). So `Source/` at HEAD is the verified 2.5a tree, byte for byte.
- `git diff --quiet 2cf30120 HEAD -- modules scripts/param-dump` is clean (checked at plan). **2.5b changes no module.** This was re-checked against the code at plan:
  - grain-engine 1.1.0: `GrainScheduler::process` / `reset` / `getOnsetCount` carry the R73 re-phase. `CounterRng::hash` mixes the full 64-bit counter (`mix (counter)`), so `2^63 | n` is a different draw from `n`.
  - pitch-collections 1.0.0: `builtIns()[Chromatic]` is `equalDivision (12)` (D19), and `Table::period` exists (R70).
  - `juce::dsp::FFT` is JUCE. `juce_dsp` is already linked by the plugin target and by `ouaricon_add_processor_console` (n0–n3).
  - No consumer compile check is needed.
- Stale stage comments (verified at plan; T2 fixes all of them): `Engine.h:58` "2. (analysis — 2.5b)" and `ModSources.h:64` "Pitch (6) and Onset (7) output exactly 0 in 2.5a (2.5b).". Also stale: `Engine.h:45` "Phases 2.1–2.5a", `Engine.h:126` "Pitch / Onset 0 (2.5b)", `Engine.cpp:701` "Pitch / Onset stay 0 (2.5b)", `EngineConstants.h:33` "Phases 2.1–2.5a", `StreamScheduler.h:31` "Free + Tempo (2.5a)", and the harness banner in `main.cpp` ("2.1–2.5a").

---

## Goal

Make the engine listen to its input:
- **Onset:** an edge-triggered detector on the conditioned input (R60, R75). Its events fire bursts: a re-phase in Free (D14, via R73), and one extra grain in Tempo with the grid untouched (D18). They also drive the Onset source (analytic 150 ms decay).
- **Pitch:** decimator + MPM (the ARCH contract, R68). Hop results are committed at the next grid tick (R59). The D20 guard as amended (R66). The live Pitch source (hold, slew). The D17 f0 history ring in the ring domain (R62–R65, R69).
- **Follow:** absolute pitch follow against `scale_root` (R70–R72, D19). The f0 is looked up at the content midpoint (R64) through a two-pass plan. A miss falls back to interval mode (the D4 path).
- The Pitch and Onset matrix sources go live; telemetry carries them.

At neutral settings (follow off ×4, trigger off ×4, no live route from sources 6 / 7), the engine stays **bit-identical to the verified 2.5a tree while the analysis runs** (N3 vs `2cf30120`). N2, N1, N0 and R12 keep passing. The 299 carried rows keep their results. Exactly one carried row, the 2.5a "MF6 Pitch / Onset … inert until 2.5b", is rewritten to its 2.5b form at T4 (P74). **No audio re-baseline is planned.**

At the end of 2.5b:
- FUNC-08 and DSP-08 are gated objectively. FUNC-06 is complete (Pitch and Onset).
- QUAL-01 (onset bursts), PERF-01 (onset / MPM / follow), PERF-03 (2.5b sources) and QUAL-02 (analysis stress) advance. DSP-01 is re-gated under follow. PERF-02 C6 is measured.
- `DAW-2.5b.md` is ready and joins DAW-2.5a.md in the deferred Stage 3 DAW pass.
- The plugin installs and passes auval, pluginval 10 on VST3 and AU, and the host-automation cache gate.

---

## Plan-phase decisions

- **P60 — N3, the neutral null, lands first on the unmodified tree** (CONTEXT gate). T1 is a separate commit, made before any `Source/` change.
  - `tests/n3/` is a sibling of `tests/n2/` (same `git archive` machinery), with `OLD_SHA = 2cf30120`. The refusal check covers `modules/synthesis/grain-engine modules/tuning/pitch-collections modules/effects/reverb-unit scripts/param-dump`, the list in `tests/n2/run.sh`. (PLAN-2.5a P46 wrote `modules/core/grain-engine`; the real path is `synthesis`.)
  - The driver is the N2 driver: 48 k, 512 blocks, the R12 seed, exact `setP`, 2.3 FX non-peaking, 2.4 dials neutral, and the 2.5a dials non-default but neutral (slots 1–4 source / dest None, 5–8 None / dest, 9–16 None / None, every depth non-zero). It adds **non-default but neutral 2.5b dials:**
    - `onset_threshold` −24 dB; `scale_root` 7 (G); `sN_onset_trigger` Off ×4; `pitch_follow_sN` Off ×4;
    - slot 12 = Pitch → None (depth 40 %), slot 13 = Onset → None (depth −60 %).
    - None of these is a route, so the null must hold.
  - **Stimulus, both sides:** the N2 hash noise ×0.1 (peak 0.05), plus a 0.1 sine at 227 Hz that steps to 301 Hz at 3 s (phase-continuous; off the 12-TET grid; +4.88 st, not a harmonic-class step), plus 2 ms 1 kHz Hann bursts at 0.35 every 250 ms from 0.5 s. Peak ≤ 0.5 (the N2 bound), 6 s. The detector fires and MPM is confident in the neutral case, so the null proves that the analysis never reaches the audio path. If a below-knee or non-vacuity precondition fails, scale the stimulus on both sides, never the check, and record it.
  - Cases `free` and `capped` as N2. A `MockPlayHead` (constant 120 bpm, playing) on both sides.
  - **Analysis liveness:** the driver prints `onsets / hops / confident / f0writes` from the Diag through a generic lambda with a C++20 `requires` probe, so the same source compiles on both sides at every task (absent fields print `n/a`). Each must be > 0 from the task that adds it (onsets T3; hops and confident T5; f0writes T7). `--strict` requires all four.
  - Controls (new side vs the old side's same variant, free case):
    - `N3c`: seed + 1, must differ.
    - **Positive, IDENTICAL at every task:** `N3p-THR` sweeps `onset_threshold` −60 → 0 dB, set per host block across the render (triggers off, no Onset route). `N3p-Z` routes Pitch → s1_level (slot 14) and Onset → mix (slot 15) at depth **0** (R46 for the new sources).
    - **Negative,** each IDENTICAL until its stage lands, then must DIFFER; `--strict` from T8:
      - `N3n-TRG`: `s1_onset_trigger` On (Free; the bursts are the percussive stimulus). Flips at T3.
      - `N3n-ONS`: slot 14 Onset → s1_level +1 %. Flips at T4.
      - `N3n-PIT`: slot 15 Pitch → s1_level +1 %. Flips at T6.
      - `N3n-FOL`: `pitch_follow_s1` On. s1's scale is Off, so D19 applies (chromatic). The tones sit 46 / 42 ct off the grid, so the follow interval is never 0. Flips at T8.
  - N2 (`--strict`), N1 (`--strict`), N0 (`--strict`) and R12 keep running unchanged. The out-of-tree N3 is unrelated to the 2.1 harness row "N3 R1 on …".
- **P61 — `Analysis.h` (new, header-only):** one owner for the detector, decimator, MPM, guard, f0 ring, cell log and the live Pitch / Onset sources (CONTEXT execution default).
  - **Tap:** per sample `a = 0.5f * (inL[i] + inR[i]) * inGain.value (cellPos + i)`. That is the env expression, evaluated by the analysis itself; `env.process` stays verbatim, so the EV rows stay bitwise. The call sits in `Engine::process` right after the env scrub, before `scheduleStreams`. It never reads the ring write, the feedback return or the dry path (R74: the conditioned input is kept).
  - **Always runs** (detector + MPM), whatever the triggers, follow and routes. The Pitch source, telemetry and ring stay warm, and N3 proves isolation.
  - **Sanitise at the tap:** a non-finite `a` → 0 and `analysisScrubs++`, so NaN never enters the FIR, the window or the envelopes. The detector and slew state are checked at each **tick** (absolute, so block-invariant; not per piece). Non-finite → zeroed, armed, `analysisScrubs++`. A non-finite MPM result counts as unconfident.
  - **Fixed capacity, no allocation after construction:** `std::array` for the FIR history, the decimated window ring (2048), the FFT buffer (2·2048 floats), the m(τ) / NSDF scratch (1024), the f0 ring (`kF0RingCap = 8192` entries) and the cell log (`kCellLogCap = 2048`). `prepare (fs)` (message thread) computes D, Hs, N, the log length, the lags and every coefficient, and clears all state. A jassert and FR0 assert N ≤ cap and log ≤ cap from 8 k to 768 k. `juce::dsp::FFT fft { 11 }` is a member, constructed with the Engine on the message thread.
  - No background thread, no lock, no logging (determinism: offline == live).
- **P62 — Onset detector** (R60 signed off, R61, R75).
  - Per sample on `a`: a TPT one-pole HP at 150 Hz (`g = tan (π·150/fs)`, `G = g/(1 + g)`; `v = (a − s)·G`, `lp = v + s`, `s = lp + v`, `hp = a − lp`), `x = |hp|`, a fast envelope (0.5 / 10 ms) and a slow envelope (80 / 80 ms), with `c = 1 − exp (−1/(τ·fs))` in double → float at prepare. Hand-written: no `juce::dsp::IIR` (memory: its default ctor is first order and mallocs).
  - The condition is compared in linear (R75): `fast > thrLin && fast > kOnsetRatio·slow`, with `thrLin = 10^(onset_threshold/20)` latched at the tick. No per-sample log10. Edge + re-arm per the sign-off; the statement order is exactly RESEARCH §2. The refractory is `round (0.05·fs)` samples, decremented per sample (memory: a block timer `+= n` fires early).
  - The event is the absolute sample where the gated condition first holds, stored as `int64 eventAbs` (memory: block-relative scheduling drops events). At most one event per piece (the refractory is ≥ 400 samples > kCell for fs ≥ 8 k); test builds track `maxEventsPerPiece`.
  - A transient inside the refractory can fire at the sample the refractory expires, if the detector was re-armed and the condition still holds. That is the signed-off rule; OD3 records it.
  - Detection lags the true onset by 0.4–5 ms (R61). Gates measure against the **reported** event.
  - Hook `levelTrigger` (OD1n): the ARCH level form (no armed state).
- **P63 — Onset → scheduler** (D14 via R73, D18).
  - `Engine::scheduleStreams (len, eventOffset)`, with `eventOffset = eventAbs − absCount` for an event inside this piece, else −1. Only enabled streams whose `onsetTrigger` is latched On react. A disabled stream does nothing and draws nothing (FUNC-03 c1). A muted or frozen stream fires (frozen geometry: "a live transient retriggers the held buffer").
  - **Free (D14):** `process (i)` → `rephase()` → `process (len − i)`, with the second batch's offsets shifted by i. `rephase()` is a public wrapper of `restart()`: `epoch += getOnsetCount()`, then `GrainScheduler::reset()`. The re-phase grain is index 0 of the new epoch at frac 0 (u₀ = ½, no jitter draw). A grid onset due at i is not emitted twice, because `process (i)` ends before i. No new index space (R73).
  - **Tempo (D18):** `processTempo` is unchanged. Append `{i, 0.0, kBurstBit | burstCount[s]++, s, burst = true}` after every grid onset with offset ≤ i (grid first on ties). `kBurstBit = 1ull << 63`; `burstCount` is zeroed only in `prepare()`. The slot, `tIndex`, `lastEmit` and `epoch` are untouched (D18; the VERIFICATION-2.5a item 6 watch). The draws use `kGrainStream + s` at a counter disjoint from every grid counter (< 2^63). This needs no Spawner signature change.
  - **Capacity:** `streamOnsets[kMaxStreams][kCell + 1]` and `onsets[kMaxStreams·(kCell + 1)]`; `mergeOnsets` takes the new extent. The merge order is unchanged (offset, then stream; in-stream order kept). `StreamOnset` gains `bool burst = false` (records and telemetry). The canaries stay around the resized arrays.
  - Bursts sit outside the R5 law (R78, accepted). Quotas and R8 stealing apply unchanged.
- **P64 — Onset source** (R75). At tick T, `v = exp (−(T − lastEventAbs)/(0.15·fs))` in double → float ("150 ms" is the time constant τ, ARCH wording). It is 0 before the first event and **exactly 0 once `T − lastEventAbs > 16·τ`** (`exp (−16)` = 1.1e-7). An unbounded exp tail goes subnormal after ≈ 87 τ (13 s), and the carried Z5 row scans every source for subnormals. Instant attack = the first tick after the event (≤ 32 samples): sources are sampled at the tick, so an event acts from the next cell.
- **P65 — Decimator + MPM** (the ARCH §15 / Algorithm §7 contract; R68).
  - `D = max (1, round (fs/12000))`, `Hs = 128·D`. A 64-tap Blackman-windowed sinc, cutoff 0.4·fs/D, designed in double at prepare and **normalised to Σh = 1**. Do not use `juce::dsp::FilterDesign` (memory: WindowedSinc DC gain 0.990). Polyphase: the FIR output is computed only at `abs % D == 0` (decimated index `m = abs/D`), 64 MACs per decimated sample, with the history persisting across pieces.
  - A hop fires on the absolute decimated counter when `(m + 1) % 128 == 0 && m + 1 ≥ 1024`. The window is decimated samples `[m − 1023, m]`, the hop sample is `a_hop = m·D`, and the window centre is `c = a_hop − (511.5·D + 31.5)` (R69).
  - Per hop:
    - the level is the window's mean square in double, gated at ≥ `10^(−50/10)` (−50 dBFS RMS);
    - remove the window mean, zero-pad to 2048, `performRealOnlyForwardTransform (buf, true)`, form |X|², `performRealOnlyInverseTransform` → r(τ);
    - compute `m(τ)` as a running difference only up to `τ_max = ⌈fs_d/40⌉ + 1` (R68); NSDF = `2r/m`;
    - pick the peak exactly as the RESEARCH model does: skip the zero-lag lobe, take key maxima between positive-going crossings in `[⌊fs_d/1500⌋, τ_max]`, take the first ≥ 0.9·max, refine parabolically; clarity = the interpolated peak;
    - accept iff clarity ≥ 0.8 and the level gate passes.
    - Output: `{hopIndex, a_hop, note = 69 + 12·log2 ((fs_d/τ)/440) (float), clarity, level, confident}`.
  - The harness carries an independent **double, direct-autocorrelation reference** of the same pipeline (MP0). The FFT scaling and the pick are proven against it, not by reading JUCE.
  - **64 taps at every rate** (CONTEXT keeps the ARCH contract). Measured at plan with the RESEARCH filter design:
    - fold-band attenuation at `fs_d/2`: −27.8 dB at 44.1 / 48 k, −13.9 dB at 96 k, −8.7 dB at 192 k;
    - aliasing into the ≤ 1.5 kHz pitch band: ≈ −104 dB at 48 k, −85 dB at 96 k, −53 dB at 192 k, −16 dB at 384 k.
    - DSP-08 gates 44.1 / 48 / 96 k. 176.4 / 192 k is an INFO row (MP1h). A fallback (taps = 16·D) would change the lag constant and the CONTEXT contract, so it needs a sign-off (Risks).
- **P66 — R59: hop results commit at the next grid tick.**
  - The hop is computed inside the analysis pass at `a_hop`, into a one-entry `pending` slot (Hs ≥ 128 > kCell, so ≤ 1 hop per cell). `gridTick` commits it after `handleFreezeEdges` and before `tickSources`: guard → ring write → live note → Pitch source. Spawns, the Pitch source, telemetry and lookups read committed state only.
  - Hook `commitHopImmediately` (N9n): commit at the hop sample inside the piece. That is the R59 failure class; it must break the block null.
- **P67 — D20 octave guard** (R66 signed off; R67, R69).
  - Committed raw hop i → median `m_{i−1}` over the raw hops `{i − 2, i − 1, i}`, confident only: 3 confident → the median; 2 → the newer of the two (the research model `guard2.py`); < 2 → NaN. Its timestamp is the centre of hop i − 1 (a centred median, R69).
  - **The decision for median j is made at the commit of raw hop j + 3** (once median j + 2 is known). This is a **uniform 3-hop lag for every hop**, so the newest entry always lags the head by exactly `511.5·D + 31.5 + 3·Hs` (+ ≤ 31 commit samples): 3613.5 at 44.1 / 48 k (R69).
  - Rule against the accepted note `acc`:
    - NaN → the output is NaN (acc kept);
    - acc empty, or `|m_j − acc| ≤ 0.5`, or a non-harmonic move → accept;
    - a harmonic-class jump (`||m_j − acc| − h| ≤ 0.5` for h ∈ {7.02, 12, 19.02, 24}) → accept only if `m_j`, `m_{j+1}` and `m_{j+2}` are all confident and within ±0.5 st of `m_j`; otherwise output acc.
  - **acc expires** after `ceil (0.25·fs/Hs)` consecutive NaN medians (the hold length), so a note after a silence is never relabelled with a stale note. This is a plan refinement of R66; it goes into the errata and OG1 gates it.
  - Hook `guardOff` (OG2n): raw hops straight through (no median, no confirm).
- **P68 — Live Pitch source and telemetry.**
  - The live note is the newest decided value. The live confidence (source and telemetry) stays true until `ceil (0.25·fs/Hs)` decided hops have passed since the last confident one, then turns false (the ARCH hold). The hold is **not** written into the ring (R65).
  - Source, once per tick: `target = clamp ((note − 24)/84, 0, 1)` of the last confident note; one-pole `y += a·(target − y)` with `a = 1 − exp (−32/(0.02·fs))` (double → float, at prepare). While unconfident the target stays where it is (the source holds). **0 until the first confident hop, then the slew runs from 0.** That confirms the RESEARCH §8 recommendation; there is no snap, because a step would click a routed ramped destination.
  - Telemetry: the existing `srcTelemetry` Pitch and Onset entries, plus two new relaxed atomics, `pitchNote` (MIDI float) and `pitchConfident` (0 / 1), with processor accessors (ARCH Telemetry: "pitch-follower note + confidence"). The engine never writes APVTS.
- **P69 — f0 history ring + cell log** (D17 via R62, R63, R65).
  - **Cell log:** at each tick, after `handleFreezeEdges`, record `{cellStart = absCount, W = ring.getWriteCount(), live = ! frozen}`. `freezePending` cells are live (the head still moves). Length `ceil ((lagNewest + 32)/32) + 2`, ≤ 2048.
  - **Write:** at the decision for hop j, map the centre `c_j` to its cell (`floor (c_j) / 32`).
    - Live cell: `W = W_cell + (c_j − cellStart)`, `k = floor (W/Hs)`, entry `[k mod N] = {tag = k, note}` with NaN = unconfident, and `kNewest = max (kNewest, k)`.
    - Frozen cell: skip, `f0Skips++`.
    - `N = ceil (kRingSamples (fs)/Hs) + 2`. Tags start at −1 (never written).
  - The ring stores the **post-guard value per hop, with no hold** (R65). Hook `ringHold` (FR7n) writes the held value instead.
  - Frozen: nothing is written for frozen cells; the held span keeps its entries (D17).
- **P70 — Lookup** (R62; R64 signed off). Spawn, follow streams only.
  - Pass 1 is the existing `Spawner::plan` call (interval mode). For a follow-off stream it is the only call (N3).
  - `Wq = (W_on − delay) + dir·rate·len/2`, the content midpoint. `W_on` is the value `spawn` already forms: live `wPiece + offset + 1`, frozen `wPiece`, pending `fz.W0`.
  - `k = floor (Wq/Hs)`. If `k > kNewest` then `k = kNewest` (R62). If `kNewest < 0` or `tag[k mod N] ≠ k` → miss (missing or overwritten). A NaN entry → miss (unconfident). **A miss leaves the pass-1 plan in place (interval mode).**
  - A hit → the P71 target → pass 2: `Spawner::plan (…, &follow)`. Draws are keyed by slot and pure, so nothing moves (FR10).
  - Hooks: `strictNewest` (FR8n: `k > kNewest` → miss, strict D17), `lookupAtStart` (FR9n: `Wq = W_on − delay`), `liveF0Lookup` (FR5n / FR6n: the live tracker note, the ARCH §5 form).
- **P71 — Absolute quantiser** (R70, R71 signed off, R72, D19).
  - `AbsSet` in `ScaleQuantiser.h`, one per stream, with a fixed `cents[kMaxActive]`. Members are `c_d + k·P` (P = `table.period`, in double) for the active degrees over **[−2400 − P, 2400 + 2P]**. It is built for the maximum span, so its key is (table choice, mask, customVersion) only, and a modulated `pitch_rand` never rebuilds it per cell. This refines R70, which keyed on the span; `ActiveSet` keeps its own keys. The mask is tested by source index and the set is then insertion-sorted (R9; memory: 7-Limit Just is unsorted at source). Outward truncation at kMaxActive works as in ActiveSet (`wasTruncated`). Rebuilt at ticks only, counted in `absSetRebuilds`.
  - D19: scale Off with follow on → `builtIns()[Chromatic]` (= `equalDivision (12)`), all degrees (masks ignored).
  - Per grain, in double, MIDI cents:
    - `R = 6000 + 100·root` (1/1 at C4 + root, R70) and `T = 100·(f0 + pitch)`, where pitch is the spawn-latched, matrix-modulated `sp.pitch`;
    - `o = T − R`, `k0 = floor (o/P)`, `q = o − k0·P`;
    - clamp window `[100·f0 − 2400, 100·f0 + 2400] − R − k0·P` (|interval| ≤ 24 st); span = 100·pitch_rand.
    - span > 0 and members in `[q − span, q + span] ∩ clamp` → a uniform pick, `draw.nextInt (Slot::Pitch, n)` (R72);
    - otherwise → the **nearest** member to q inside the clamp window, ties up (R71);
    - no member inside the clamp window, or an empty set (empty mask) → miss → interval mode. That matches the spec's "empty mask: grains stay at the stream's base Pitch" (offset 0, no draw).
  - `t = R + k0·P + member`; `follow.st = (t − 100·f0)/100` (st, double); `follow.pitchCents = t − T`.
  - Test builds add `rec.f0`, `rec.target`, `rec.followHit`, `rec.followMiss` (0 none, 1 unconfident, 2 missing / overwritten, 3 empty set, 4 no member), `rec.lookupW` and `rec.burst`. The D19 and quantisation rows gate the **recorded target** (R72); the sounding pitch is INFO.
- **P72 — `Spawner::plan` gains a trailing `const FollowPitch* follow = nullptr`** (`{double st; float pitchCents;}`). Non-null → `st = follow->st` and `pitchCents = follow->pitchCents`, then the existing ±24 clamp, `rate = exp2 (st/12)` and the rest of `plan`, unchanged. nullptr → the 2.5a code, verbatim. Tick consumers (quotas, R5 coherence) keep the interval-mode values: follow does not feed the output law, the same treatment as R78.
- **P73 — Plumbing.**
  - `ParamPointers` + `GridParams` gain `scaleRoot` (choice 0–11, `lround`, clamped), `onsetThresholdDb` (−60..0), and per stream `s[n].onsetTrigger` and `s[n].follow` (bools). The processor caches 10 pointers: `scale_root`, `onset_threshold`, `s1..4_onset_trigger`, `pitch_follow_s1..4`.
  - Diag gains `onsetEvents`, `maxEventsPerPiece`, `freeBursts`, `tempoBursts`, `mpmHops`, `mpmConfident`, `guardHeld`, `f0Writes`, `f0Skips`, `followLookups`, `followHits`, `followMiss[5]`, `absSetRebuilds`, `analysisScrubs`.
  - TestHooks gain `analysisOff` (OD0), `levelTrigger`, `commitHopImmediately`, `guardOff`, `ringHold`, `strictNewest`, `lookupAtStart`, `liveF0Lookup` and `injectAnalysis {int64 at; float value; bool done}`.
  - Test logs: `eventLog` and `hopLog`, 4096 entries each (hop abs, raw note, clarity, level, confident, decided index, decided note).
- **P74 — Carried rows.**
  - Baseline A is the harness at T1 (299/299, run twice, identical).
  - **No audio re-baseline is planned.**
  - **One planned, attributed carried-row change:** at T4 (Onset live), the 2.5a row "MF6 Pitch / Onset routed at ±100 %: bitwise == unrouted (inert until 2.5b)" is rewritten in place in `engine_gates_25ab.h` as "MF6 Pitch / Onset routed at ±100 %: live (2.5b) — differs from unrouted", on the P75 tonal + burst stimulus, with `routedDests ≥ 1`. Baseline A′ is A with that one line replaced. The carried diff runs against A through T3 and against A′ from T4. The count stays at 299 carried.
  - Every other carried row must keep its line. Checked by grep at plan: no other carried row routes source 6 or 7, turns a trigger on or turns follow on.
  - W2o (+0.483 dB, 0.017 dB margin) is unchanged by construction. If it flips, widen its seeds; never touch the threshold.
- **P75 — Stimulus conventions** (memory: a stimulus below threshold makes a gate vacuous; click probes need a tonal source and a same-motion control).
  - Detection stimuli are **≥ 1 ms tone bursts** (R61), never single-sample impulses. An impulse is INFO only: detected at 44.1 / 48 k, missed at 96 / 192 k.
  - FUNC-08 hit-rate trains use equal-level bursts at IOI ≥ 60 ms. A ghost note under an accent's slow envelope is ARCH behaviour, not a miss (RESEARCH §2).
  - Follow stimuli use tones off the 12-TET grid (227 Hz, 233 Hz, 301 Hz, 311 Hz) with non-harmonic-class steps, so neither an interval of 0 nor a guard confirmation hides a result.
  - D20 audio uses the near-boundary H2/H1 = 4.36 + white noise at SNR 20 dB (seeded). Breathy tones, persistent H2/H1 = 8 and H3/H1 = 4 are INFO (R67).
  - Click probes (QB1): a 440 Hz carrier with 1 ms-attack +18 dB swells every 125 ms. The swells fire the detector while the ring content stays tonal. The reference has the **same grain count** (R78).
  - Rates: 44.1 / 48 / 96 k for DSP-08; 44.1 / 48 / 96 / 192 k for the detector.
- **P76 — Row files.**
  - `tests/render-harness/engine_gates_25b.h` holds the OD, OB, MP, OG and PS rows, plus the stimulus helpers and the double references (MPM, detector). `engine_gates_25bb.h` holds the FR, AQ, MX, QB, N9, M6, Z6 and C6 rows.
  - Carried row files are not edited, except the P74 MF6 block.
  - New prefixes: OD, OB, MP, OG, PS, FR, AQ, MX, QB, N9, M6, Z6, C6. None of them matches a carried row token.
  - `main.cpp` gains `--only25b`, two `#include`s and the section calls.
  - Expected total **≈ 370** (299 carried + ≈ 70 new); record the actual count.
- **P77 — CPU.**
  - **C6 (INFO)** = the C5 recipe + the detector and MPM (always on) + 4 follow streams + triggers on ×4, on a tonal + burst input. Warm-up 4.5 s, median of 3 × 10 s, % of one core, Δ vs a C5 re-run. R77 projects ≈ 17 %. **Phase-fail ceiling: 25 %.** P26 set 30 % for 2.3; here, an analysis cost more than ≈ 8 points above C5 means the hop or the decimator is wrong (for example an FFT per sample). Between 20 and 25 %: record it and flag it for 2.6.
  - **C6s (INFO):** host blocks of 32 at 48 k, per-block wall time; median and worst over blocks that contain a hop vs blocks that don't. Flag for 2.6 if the median hop-block excess is over 10 % of the block budget (66.7 µs). The hop is not spread across passes (R77).
- **P78 — Host-cache gate and DAW sheet.**
  - `tests/hostcache/run.py` adds the 10 2.5b parameters (HA1 69 → 79; HA2 untouched 152 → 142).
  - T12 writes `stages/2-dsp/DAW-2.5b.md` (items in T12). Those items join DAW-2.5a.md in the deferred Stage 3 DAW pass. A 2.5b DAW FAIL reopens FUNC-08 / DSP-08 (CONTEXT).

---

## Source layout (deltas against 2.5a)

| File | Change |
|---|---|
| `Source/dsp/EngineConstants.h` | Detector: `kOnsetHpHz = 150`, `kOnsetFastAttMs = 0.5`, `kOnsetFastRelMs = 10`, `kOnsetSlowMs = 80`, `kOnsetRatio = 1.9952623149688795` (10^(6/20)), `kOnsetRearm = 1.4125375446227544` (10^(3/20)), `kOnsetRefractS = 0.05`, `kOnsetTauS = 0.15`, `kOnsetZeroTaus = 16`. Decimator / MPM: `kDecimTargetHz = 12000`, `kDecimTaps = 64`, `kDecimCutoff = 0.4`, `kMpmWindow = 1024`, `kMpmFftOrder = 11`, `kMpmHop = 128`, `kMpmKeyMax = 0.9`, `kMpmClarity = 0.8`, `kMpmGateDb = −50`, `kMpmMinHz = 40`, `kMpmMaxHz = 1500`. Guard: `kGuardHarm[4] {7.02, 12, 19.02, 24}`, `kGuardTolSt = 0.5`, `kGuardN = 3`. Pitch source: `kPitchHoldS = 0.25`, `kPitchSlewS = 0.02`, `kPitchSrcLo = 24`, `kPitchSrcSpan = 84`. Capacities: `kF0RingCap = 8192`, `kCellLogCap = 2048`. `kBurstBit = 1ull << 63`. `kAbsRefCents = 6000`, `kAbsMaxSpanCents = 2400`. Banner "Phases 2.1–2.5b" |
| `Source/dsp/Analysis.h` **(new)** | P61–P69: `OnsetDetector`, `Decimator`, `Mpm`, `OctaveGuard`, `CellLog`, `F0Ring`, `PitchTracker` (hold, slew) and `Analysis` (tap, per-piece `process` → event, the pending hop, `tick (…)` commit, Pitch / Onset sources, telemetry values, `lookup`) |
| `Source/dsp/ScaleQuantiser.h` | `AbsSet` (P71) next to `ActiveSet` (unchanged) |
| `Source/dsp/Spawner.h` | `FollowPitch`; `plan (…, const FollowPitch* follow = nullptr)` (P72) |
| `Source/dsp/StreamScheduler.h` | `rephase()`; `StreamOnset::burst`; `kOnsetCap = kCell + 1`; `mergeOnsets` extent; header comment "Free + Tempo (2.5a) + onset bursts (2.5b)" |
| `Source/dsp/ModSources.h` | Comment at :64 (Pitch / Onset live from 2.5b, computed in `Analysis.h`). No code change |
| `Source/dsp/ControlGrid.h` | P73 fields (pointers + latch) |
| `Source/dsp/Engine.h/.cpp` | The analysis member + prepare. The per-piece analysis after the env scrub. At the tick: cell log + commit + `thrLin`. `scheduleStreams (len, eventOffset)`: the Free split, the Tempo insertion, `burstCount`. Abs sets in `updateScaleState`; lookup + pass 2 in `spawn`. Pitch / Onset in `tickSources`. Diag, TestHooks, SpawnRecord, event and hop logs. Banner and comments (`Engine.h:45`, `:58`, `:126`; `Engine.cpp:701`) |
| `Source/PluginProcessor.h/.cpp` | 10 pointers; `getPitchNote()`, `getPitchConfident()` |
| `tests/n3/run.sh`, `tests/n3/n3_render.cpp` **(new)** | P60 |
| `tests/render-harness/main.cpp` | `--only25b`; two `#include`s + section calls; banner "2.1–2.5b" |
| `tests/render-harness/engine_gates_25b.h`, `engine_gates_25bb.h` **(new)** | P76 |
| `tests/render-harness/engine_gates_25ab.h` | The MF6 inert block → its 2.5b form (T4, P74). The only edit to a carried file |
| `tests/hostcache/run.py` | + the 10 2.5b parameters (P78) |

---

## Tasks

Build commands (from `build/`, Release, `OUARICON_BUILD_TESTS=ON`): `ninja O-Granulator-render-harness`, then `ninja OuariconGranulator_VST3 OuariconGranulator_AU`. The harness binary is `build/plugins/O-Granulator/O-Granulator-render-harness_artefacts/Release/O-Granulator-render-harness`; exit 0 means ALL PASS.

**Run the harness and every out-of-tree script (`tests/n3`, `n2`, `n1`, `n0`, `r12`) in the background** (memory: 600 s watchdog). `--only25b` is the iteration aid; every acceptance uses the full run.

**The carried-row diff:** sorted `[PASS]` / `[FAIL]` lines, dropping the new prefixes (`OD`, `OB`, `MP`, `OG`, `PS`, `FR`, `AQ`, `MX`, `QB`, `N9`, `M6`, `Z6`, `C6`) and the INFO / timing rows, as in 2.5a. Match the whole token, not a leading letter: carried rows start with `M`, `F`, `P`, `A`, `Q`, `N` and `Z`. T1–T3 compare against **baseline A** (T1); T4 onward against **A′** (P74). Apart from that one attributed line, the diff must be empty at every task. A non-empty diff is a regression: stop and bisect.

### Wave 0 — the precondition, on the 2.5a tree (P60)

**T1. N3 neutral null vs `2cf30120`; baseline A**
- Files: `tests/n3/run.sh` (new), `tests/n3/n3_render.cpp` (new). **No `Source/` change.**
- `run.sh` copies `tests/n2/run.sh`: the P60 refusal list; brace-form `"${OLD_SHA}"` revpaths (memory: the zsh `:` modifier); `--strict`; WORK `${TMPDIR:-/tmp}/ogran-n3`. Its controls are N3c, N3p-THR, N3p-Z and N3n-TRG / ONS / PIT / FOL. It prints the analysis liveness line and, with `--strict`, requires it.
- `n3_render.cpp` extends `n2_render.cpp` per P60. It keeps the landed / raw checksums (now over the 2.5b params too), planCalls > 0, free 0 steals / kills, capped kills > 0 and the below-knee print. Variants: `base`, `seed1`, `THR`, `Z`, `TRG`, `ONS`, `PIT`, `FOL`.
- Capture **baseline A**: harness stdout → `${TMPDIR:-/tmp}/ogran-25b/harness-A.txt`. Run it twice and diff (empty: determinism).
- Acceptance:
  - `tests/n3/run.sh` exit 0: both cases max|Δ| == 0; N3c differs; below knee; **N3p-THR and N3p-Z IDENTICAL**.
  - All four N3n print **IDENTICAL**; the liveness line prints `n/a`.
  - Harness 299/299. N2 `--strict`, N1 `--strict`, N0 `--strict` and R12 PASS.
- Commit **alone**: re-check `git branch --show-current` + `git status --short`, then `git commit -- plugins/O-Granulator/tests`.
- Depends on: none.

### Wave 1 — plumbing (bitwise no-ops)

**T2. Plumbing: constants, ControlGrid + processor pointers, the `Analysis.h` shell, onset-list capacity, the `Spawner::plan` follow parameter, Diag / TestHooks / records, stale comments; harness scaffolding**
- Files:
  - `EngineConstants.h`; `ControlGrid.h`.
  - `Analysis.h` (new: types and `prepare`, which computes D, Hs, N, the log length, the R69 lags and the coefficients; no per-sample work yet).
  - `StreamScheduler.h` (`StreamOnset::burst`, `kCell + 1` capacity, the `mergeOnsets` extent, `rephase()`, comment); `Spawner.h` (`FollowPitch` and the defaulted parameter, unused); `ScaleQuantiser.h` (`AbsSet` declared); `ModSources.h` (comment).
  - `Engine.h/.cpp` (member, prepare call, Diag / TestHooks / SpawnRecord fields, `Diag::reset()`, banner + comments).
  - `PluginProcessor.h/.cpp` (10 pointers; telemetry accessors returning 0).
  - `main.cpp` (`--only25b`, includes, section calls, banner); `engine_gates_25b.h` and `engine_gates_25bb.h` (new: the stimulus helpers and FR0).
- **FR0 — capacities and constants:** at 8 / 11.025 / 16 / 22.05 / 32 / 44.1 / 48 / 88.2 / 96 / 176.4 / 192 / 352.8 / 384 / 705.6 / 768 kHz, N ≤ `kF0RingCap`, the log length ≤ `kCellLogCap` and `τ_max` ≤ 1022. The window-centre lag `511.5·D + 31.5` and the newest-entry lag `+ 3·Hs` equal the RESEARCH §3 table at 44.1 / 48 / 88.2 / 96 / 192 k (2077.5 / 3613.5 … 8215.5 / 14 359.5).
- Acceptance: FR0 PASS; 0 new warnings; carried diff vs A empty; N3 PASS (both N3p IDENTICAL, all N3n IDENTICAL); the `Grain` static_assert and the canaries hold.
- Commit T2 (path-scoped).
- Depends on: T1.

### Wave 2 — onset (tracer: a detected transient spawns a grain, end to end, before any pitch work)

**T3. Onset detector + Free burst**
- Files: `Analysis.h` (`OnsetDetector`, the tap, `eventAbs`, the tick scrub, `thrLin`); `Engine.cpp` (the analysis call after the env scrub; `thrLin` at the tick; `scheduleStreams (len, eventOffset)` with the Free split); `StreamScheduler.h` (`rephase()`); `engine_gates_25b.h`.
- Hooks wired: `analysisOff`, `levelTrigger`, `injectAnalysis`; the test log `eventLog`.
- Write each unit against its double reference first and watch it fail on a stub. The negative controls still carry non-vacuity (2.5a deviation 12).
  - **OD0 — isolation in tree:** a neutral render (follow off ×4, trigger off ×4, no 6 / 7 route; the P60 composite; 10 s; 512 blocks) with `analysisOff` vs with the analysis on: bitwise identical, while the on-render shows events > 0. **OD0n:** the same pair with s1's trigger on differs.
  - **OD1 — edge trigger (R60):** sustained 440 Hz (5 ms attack) and hard-gated 200 Hz tones at −12 dBFS, at 44.1 / 48 / 96 / 192 k: exactly **1** event each (≈ 1.8 / 0.4 ms after the onset). **OD1n:** `levelTrigger` fires 3 times (0 / 50 / 100 ms).
  - **OD2 — bursts (R61):** a 1 ms 2 kHz burst at −6 dBFS is detected at all four rates. **OD2i (INFO):** a 0 dBFS single-sample impulse — detected at 44.1 / 48 k, missed at 96 / 192 k (documented).
  - **OD3 — refractory:** across every OD stimulus and a burst-pair sweep (30 / 45 / 55 ms apart), no two events are closer than `round (0.05·fs)` samples. A pair 55 ms apart gives 2 events. For a pair inside the refractory, the second event is never earlier than `e₁ + refr` (a fire at exactly `e₁ + refr` follows the P62 rule; record it).
  - **OD4 — threshold:** threshold −30 dB: a −35 dBFS burst train gives 0 events, a −25 dBFS train fires on every burst. A threshold change takes effect at the next tick. **OD4b (R75):** on 10 s of the RESEARCH drum loop, the linear-compare events equal a double dB-domain reference (`20·log10` on both conditions) sample for sample. Any event within 1e-6 dB of a threshold is counted and reported.
  - **OD5 — tap point (R74):** feedback 100 % with the input removed → 0 events (the return is not tapped); `input_gain` −24 dB removes the events of a −20 dBFS train (threshold −30); freeze on: the detector still fires on the live input.
  - **OD6 — NaN at the tap:** `injectAnalysis` NaN → `analysisScrubs == 1`, no event from it, finite detector state, and the next burst fires. Audio bitwise == the un-injected render.
  - **OB1 — Free burst (D14 / R73):** Free, s1 and s2 with the trigger on, s3 with the trigger off, s4 disabled with the trigger on; density 4 g/s, jitter 0; a burst train at IOI 180 ms for 20 s.
    - Each trigger-on stream spawns at exactly the reported event sample (Δ 0), at frac 0, with `globalIndex` == the stream's new epoch (index 0).
    - The next grid onset sits at `event + fs/density` exactly.
    - The `globalIndex` set is unique over the render (no replay).
    - s3 and s4 spawn nothing at the events, and s4's `planCalls` / `schedCalls` equal those of a trigger-off render.
- Acceptance: OD0–OD6 and OB1 PASS (OD0n differs; OD1n fires 3 times); **N3n-TRG DIFFERS**; N3 PASS (N3p-THR IDENTICAL with the detector live: events change with the threshold, but nothing consumes them); carried diff vs A empty; 0 new warnings.
- Depends on: T2.

**T4. Tempo extra grain (D18), Onset source, telemetry; the click-train hit rate; the MF6 rewrite**
- Files: `Engine.cpp` (the Tempo insertion + `burstCount`; Onset in `tickSources`; telemetry); `Analysis.h` (`onsetSource (T)`); `engine_gates_25b.h`; `engine_gates_25ab.h` (the MF6 inert block only, P74).
- **OB2 — Tempo (D18):** Tempo 1/16 at 120 bpm, jitter 30 %; triggers on for s1 and s2, s3 trigger off, s4 disabled; a burst train at IOI 180 ms; 20 s.
  - Every grid record (`onsetSample`, `frac`, `globalIndex` and the plan fields) is bitwise == the trigger-off render.
  - The burst records sit exactly at the events, at frac 0, with `globalIndex = 2^63 | n` (n = 0, 1, 2 … per stream).
  - The `getSlotNext()` / `getEpoch()` traces are identical to the trigger-off render.
  - s3 and s4: no burst.
- **OB2b — item 6 watch:** TS1 re-run with s1's trigger on over a burst train: 4 divisions (1/16, 1/8 T, 1/4 D, 1 bar) × the 8 TS1 transports. Grid onsets == the analytic grid (worst |Δ| ≤ 1e-6 sample, expected 0), with exact counts.
- **OB3 — coincidence:** an event on the sample of a grid onset. Tempo → two grains at that sample (grid first, burst second, merge order kept). Free → exactly one grain (the re-phase; `process (i)` stops before i).
- **OB4 — frozen:** freeze engaged plus a live burst → a grain at the event with frozen geometry (`rec.writeHead == W0`, delay inside `heldSpan`).
- **OB5 — edges:**
  - A muted stream fires.
  - An enable edge and an event in the same piece give 2 grains: the reset onset at the piece start, and the re-phase at the event.
  - An event at i = 0 right after an edge gives 1 grain, with the epoch unchanged (still no replay).
  - `maxEventsPerPiece == 1`.
- **PS1 — Onset source (R75, P64):** at every tick, the source is bitwise `(float) exp (−(T − e)/(0.15·fs))` against the last event; 0 before the first event; ≈ 1 at the first tick after a re-trigger; exactly 0 beyond 16 τ. 60 s of silence after one event leaves no subnormal source value.
- **OD7 — FUNC-08 click train:** equal-level 2 ms 1 kHz Hann bursts at −12 dBFS, IOI 60 / 125 / 250 ms, 30 s, at 44.1 / 48 / 96 / 192 k, threshold −30. Events == bursts (one per burst, within 5 ms after its start), 0 double fires. The same train at −36 dBFS gives 0 events.
- **MF6 rewrite (P74):** the carried inert block is rewritten to its 2.5b form (Pitch → mix and Onset → s1_pitch at ±100 % on the P75 tonal + burst stimulus → differs from unrouted, `routedDests ≥ 1`). Capture **A′** (run twice, identical).
- Acceptance: OB2–OB5, PS1 and OD7 PASS; **N3n-ONS DIFFERS**; N3p-Z IDENTICAL; carried diff vs **A′** empty (vs A it is exactly the MF6 line); N2 `--strict`, N1 `--strict`, N0 `--strict` and R12 PASS; `srcTelemetry` Onset is in range.
- Commit T3–T4 (path-scoped).
- Depends on: T3.

### Wave 3 — pitch analysis

**T5. Decimator + MPM; the R59 commit at the tick**
- Files: `Analysis.h` (`Decimator`, `Mpm`, `pending`, commit → `hopLog`); `Engine.cpp` (commit after `handleFreezeEdges`); `engine_gates_25b.h` (+ the double direct MPM reference).
- Hook wired: `commitHopImmediately`; the test log `hopLog`.
- **MP0 — FFT vs the direct reference:** 100 windows (sines 40–1500 Hz, harmonic tones, noise, mixtures) at fs_d 11 025 and 12 000. The engine's r(τ) vs a double direct autocorrelation: max |Δr| ≤ 1e-4·r(0) for τ ≤ τ_max. The picked τ is within 1e-3 lag, clarity within 1e-4, and the accept decisions are identical, except windows within 1e-4 of a threshold (counted and reported).
- **MP1 — DSP-08 accuracy:** sines at −12 dBFS of 55 / 82.41 / 110 / 220 / 440 / 700 / 1000 Hz × 44.1 / 48 / 96 k, 0.6 s each. Every hop after the first full window is confident, and the worst |¢| of the raw hop f0 is **≤ 10 ¢**. Expected ≤ 2.8 ¢ (R68): anything > 3 ¢ is investigated before the PASS is accepted. **MP1i (INFO):** the 40 and 1500 Hz edges. **MP1h (INFO):** 176.4 / 192 k, a sine and a 220 Hz sawtooth (partials to 20 kHz): the confident fraction and the worst ¢ (P65 fold attenuation).
- **MP2 — clarity:** 0 confident hops on silence, white noise at −20 and −60 dBFS, brown noise at −20 dBFS (seeded) and a −55 dBFS sine (below the gate). A −45 dBFS sine is confident (the gate is not vacuous).
- **MP4 — hop schedule:** the hop samples in `hopLog` == `m·D` for `(m + 1) % 128 == 0`, `m ≥ 1023`, identical at blocks 32 / 441 / 4096; centres == `a_hop − (511.5·D + 31.5)`.
- Acceptance: MP0–MP4 PASS; N3 PASS with hops and confident > 0 printed (N3n-PIT still IDENTICAL: nothing consumes hops yet); carried diff vs A′; 0 new warnings.
- Depends on: T4.

**T6. D20 guard, live Pitch source, telemetry**
- Files: `Analysis.h` (`OctaveGuard`, `PitchTracker`); `Engine.cpp` (Pitch in `tickSources`; `pitchNote` / `pitchConfident`); `PluginProcessor.h/.cpp` (accessors); `engine_gates_25b.h`.
- Hook wired: `guardOff`.
- **OG1 — injected sequences (unit on `OctaveGuard`):**
  - Flips of length L of +12, −12, +19.02, −19.02, +7.02, −7.02 and ±24 st on a steady 57.0: L = 1 and 2 → 0 wrong outputs; L = 3, 4, 5 → pass through (L wrong; the guard does not claim them).
  - Legit leaps 57 → 69, 57 → 62 and 57 → 76.02: the output switches exactly at the leap hop (exact in the ring).
  - A return to acc and a non-harmonic move (±1, ±5 st, 12.6 st) are immediate.
  - 2 of 3 confident → the newer one; 1 of 3 → NaN.
  - acc expiry: NaN for ≥ the hold length, then +12 → accepted at its first median.
  - The decided index is always the raw index − 3.
- **OG2 — near boundary (R67):** 220 Hz, H2/H1 = 4.36, white noise at SNR 20 dB (seeded), 6 s at 48 k. The decided sequence has **0** wrong runs shorter than 3 hops (wrong = > 0.5 st from 57.0). **OG2n:** `guardOff` gives ≥ 10 such runs (the R67 model gives 39). If OG2n shows fewer than 10 at 4.36, scan H2/H1 from 4.30 to 4.45 for the near-boundary point and record it.
- **OG3 (INFO):** persistent H2/H1 = 8 → +12 on ≥ 95 % of hops (tracks the octave, documented); H3/H1 = 4 → +19.02; a breathy sine + white noise at SNR 20 / 10 / 0 dB → 0 wrong hops (R67: vacuous for the guard).
- **MP3 — hold (DSP-08):** a 220 Hz sine for 2 s, then silence. The live confidence stays true for exactly `ceil (0.25·fs/Hs)` decided hops after the last confident one, then turns false. The Pitch source converges to the last target and holds it through 3 s of silence. The ring entries for the silence are NaN (R65).
- **PS2 — Pitch source law:** on a 233 → 311 Hz step, the source at every tick == the float reference `y += a·(clamp ((n − 24)/84, 0, 1) − y)` driven by the decided notes in `hopLog`, bitwise. 0 until the first confident hop; unchanged while unconfident. Unit: MIDI 24 → 0, 108 → 1.
- **PS3 — telemetry:** `srcTelemetry` Pitch and Onset == the engine source values at every cell, in [0, 1]; `pitchNote` / `pitchConfident` == the live tracker.
- **PS4 (INFO):** after a note step, the time until the Pitch source crosses 50 % of the step. Expected ≈ 75 ms + the 20 ms slew (R69); printed.
- Acceptance: OG1, OG2 (OG2n ≥ 10 short runs), MP3 and PS2–PS3 PASS; **N3n-PIT DIFFERS**; N3p-Z IDENTICAL; carried diff vs A′.
- Commit T5–T6 (path-scoped).
- Depends on: T5.

### Wave 4 — f0 ring + follow

**T7. f0 history ring + cell log**
- Files: `Analysis.h` (`CellLog`, `F0Ring`, the write at decision); `Engine.cpp` (the log record at the tick); `engine_gates_25bb.h`.
- Hook declared: `ringHold` (consumed at T8).
- **FR1 — tags (unit):** a never-written slot → miss; write k, then k + N → lookup k misses (tag), lookup k + N hits; a NaN entry → miss.
- **FR2 — newest rule (unit):** `k > kNewest` → `kNewest`; `kNewest < 0` → miss.
- **FR3 — freeze edge (unit, synthetic cell log):** a centre in the last live cell is written at `W_cell + offset`; a centre in the first frozen cell is skipped (`f0Skips++`); after release, W continues from W0 (no gap in the W domain).
- **FR4 — alignment (R69):** a 233 Hz tone, a 311 Hz region from S₁ to S₂ (1 s, a non-harmonic +5.0 st), then 233 Hz again; never frozen (W = abs). The centre of the B region in the ring, taken as the mean of the first and last B entry centres, is within ±0.75·Hs of (S₁ + S₂)/2. The up / down step biases of a mixed window cancel. **FR4n (computed from the same hopLog):** timestamps at the decision hop (the centre of hop j + 3) put the region ≥ 2.25·Hs late, so the constant is what places it.
- Acceptance: FR0–FR4 PASS (FR4n off); N3 PASS (f0writes > 0); carried diff vs A′ (nothing reads the ring yet).
- Depends on: T6.

**T8. Absolute quantiser + follow (two-pass) + spawn record**
- Files: `ScaleQuantiser.h` (`AbsSet`); `Spawner.h` (consume `FollowPitch`); `Engine.h/.cpp` (abs sets in `updateScaleState`; lookup + pass 2 in `spawn`; records; Diag); `engine_gates_25bb.h`.
- Hooks wired: `strictNewest`, `lookupAtStart`, `liveF0Lookup`, `ringHold`.
- D17 rows (stimulus A = 233 Hz from 0 to 2 s, B = 311 Hz from 2 to 4 s; `buffer_length` 8 s; size 50 ms; spray 0; density 20 g/s; scale Off → D19):
  - **FR5 — content f0, live:** grains spawned in [2.3, 2.9] s at position 12.5 % (1 s behind) read A: every `rec.f0` is within 10 ¢ of A and none is within 1 st of B. At position 0 they read B (the first 150 ms after the step excluded). **FR5n:** `liveF0Lookup` gives B for the 1 s-behind grains.
  - **FR6 — frozen:** freeze at 1.5 s; the input plays A until 2 s, then B. Grains in [2.3, 2.9] s at positions 0 / 25 / 50 % all read A; `f0Skips > 0`. **FR6n:** `liveF0Lookup` → B.
  - **FR7 — unconfident gap (R65):** A, then a 200 ms −20 dBFS white-noise gap (shorter than the hold), then A. Grains whose content midpoint lies over the gap's NaN entries → `followMiss == 1` and interval mode (`rec.target` NaN; the D4 pitch). **FR7n:** `ringHold` → those grains hit with A.
  - **FR8 — position 0 (R62):** a 227 Hz sine, follow on, position 0: every grain after 0.2 s has `followHit` and f0 within 10 ¢ of 227 Hz. **FR8n:** `strictNewest` → 0 hits.
  - **FR9 — midpoint (R64):** an 800 ms grain whose content starts 300 ms before the A → B boundary (midpoint 100 ms after it) reads B. **FR9n:** `lookupAtStart` → A.
  - **FR10 — two-pass draws:** follow on vs follow off at the same seed (feedback 0, no triggers, not capped). Per `globalIndex`, `dir`, `gainL/R`, `windowStep`, `tiltT`, `len` and the FX latch (`fxActive`, `svfMode`, `fcHz`, `ringHz`, `crushed`, `driveD`) are bitwise equal; only pitch, rate, level, set and delay may differ.
- Absolute quantisation rows (`rec.target` is MIDI in double; members checked in double):
  - **AQ1 — D19:** follow on, scale Off, roots {0, 7, 11} × pitch_rand {0, 3 st} × pitch {0, +7, −5}. Every hit's target is within **0.001 ¢** of `60 + root + integer`; |interval| ≤ 24 st; ≥ 100 hits per case. Follow off with scale Off: `followLookups == 0`, and the records are identical to a render with every follow param off.
  - **AQ2 — scales with masks:** Major (12-EDO collection, mask {0, 2, 4}), 19-EDO (every other degree), Bohlen-Pierce (P = 1901.955, mask {0, 3, 6, 9}) and Custom (a non-1200 period, e.g. `edo 9 1800`, masked), at root 3.
    - Every target ∈ A = {6000 + 300 + c_d + k·P | d active} within 1e-6 ¢.
    - pitch_rand 0 → the nearest member to T (ties up), recomputed in the row from the table.
    - pitch_rand 5 st → every target in `[T ± span] ∩ clamp`, and the pick distribution over the window members passes χ² (p > 0.001, ≥ 2000 grains).
  - **AQ3 — ±24 clamp:** an `AbsSet` unit with f0 ∈ {28, 100} MIDI × pitch ±24 × pitch_rand {0, 24} × {Chromatic, Major masked, BP} → |interval| ≤ 24 st and member ∈ A. MIDI 100 (2637 Hz) is above the 1500 Hz tracker range, so it can only be a unit (RESEARCH §6 asked for an engine row). Engine row: f0 = 41.2 Hz (MIDI 28) with pitch −24 → every recorded interval ≥ −24 st.
  - **AQ4 — nearest (R71):** pitch_rand 0 with T exactly between two members → the upper one; T on a member → that member. Follow off with the same settings: offset 0 (interval mode unchanged).
  - **AQ5 — fallbacks:** empty mask → `followMiss == 3`, `st == pitch` exactly, no `Slot::Pitch` draw. Unconfident → 1. An `AbsSet` unit with a hand-built 6000 ¢ period and one degree → `followMiss == 4` for an f0 with no member within ±24 st. Each falls back to interval mode.
  - **AQ6 — register (R70):** with BP at roots 0 and 5, the member sets differ by exactly +500 ¢ and equal `{6000 + 100·root + c_d + k·1901.955}`.
  - **AQ7 — rebuild keys:** over a render with `pitch_rand` modulated by an LFO, `absSetRebuilds` == the number of key changes (0 extra rebuilds). A mask change and a custom apply each add exactly 1.
  - **AQ8 (INFO):** the sounding pitch of a single follow stream on a sine, measured with the harness MPM reference, lands within 3 ¢ of the target note.
- Acceptance: FR5–FR10 and AQ1–AQ7 PASS (every FR n-control fails its subject's gate); **N3n-FOL DIFFERS → N3 `--strict` from here on** (all four N3n differ, both N3p identical, all liveness counters > 0); carried diff vs A′; N2 `--strict`, N1 `--strict`, N0 `--strict` and R12 PASS; 0 new warnings.
- Commit T7–T8 (path-scoped).
- Depends on: T7.

### Wave 5 — gates (`engine_gates_25bb.h`; no `Source/` change)

**T9. FUNC-06 completion, QUAL-01 bursts, burst level**
- **MX1 — Pitch / Onset × class:** {Pitch, Onset} × {mix (ramped), s1_spray (latched), s1_size (skewed), reverb_size, lfo1 rate} × depth {+100 %, −100 %}, on a stepped off-grid tone + 2 ms bursts at IOI 250 ms. The destination moves with sign(depth·src) and stays in range (the MF6 method; both sources are unipolar).
- **MX2 — removal without a step:** for Pitch and Onset, remove the route mid-note or mid-pulse four ways (source → None, dest → None, depth → 0, dest swap). The traced |Δv| per sample stays ≤ the P56 removal bound (the residual slope `|c|/(20 ms·fs)`) through removal and the 20 ms tail, and the value is then exactly the raw path. **MX2n:** `noResidual` exceeds the bound. **MX2a (audio):** removal step-click κ ≤ 2.0× the held-route reference (440 Hz carrier + bursts).
- The Onset source's instant attack (a 0.67 ms cell ramp on a routed ramped destination) is excluded by design, like the square / saw / S&H edges in P56.
- **QB1 — QUAL-01 bursts:** Free and Tempo; s1 trigger on, window 0 (Hann), size 60 ms; density 20 g/s (Free) or 1/16 at 120 bpm (Tempo); the P75 swell carrier. The reference has the trigger off and a Free density set to the trigger render's measured spawns/s (same grain count). The existing step-click κ is **≤ 2.0×** the reference (memory: same-motion reference, tonal source). **QB1n:** the same trigger render at s1_window 1 (percussive decay: an instant attack) exceeds 2.0×, so the gate sees a click at this stimulus.
- **QB2 (INFO, R78):** RMS of Tempo trigger-on vs trigger-off on a busy burst train, in dB. Expected ≤ +5.4 dB.
- Acceptance: MX1, MX2 and MX2a PASS; MX2n fails; QB1 PASS; QB1n fails; QB2 printed.
- Depends on: T8.

**T10. PERF / QUAL rows: N9, M6, Z6, C6**
- **N9 — PERF-03 with every 2.5b element on:** two recipes, 12 s at 48 k each.
  - N9-F is Free. N9-T is Tempo on the v2 mock, with a tempo step, a loop and a locate.
  - Both: 4 streams; triggers on ×4; follow on ×4 (scales Off, Major, 19-EDO, BP); Pitch → s1_level, Onset → mix, Pitch → s2_pitch (latched), Onset → s3_density, LFO1 → s4_position, Env → fb_saturation; feedback 80 %; freeze toggles at shared boundaries and at block-straddling samples; seeded.
  - Input: stepped off-grid tones, 2 ms bursts every 180 ms, a 300 ms noise segment and 200 ms of silence.
  - Blocks 32 / 441 / 512 / 4096 / variable: **value compare max|Δ| == 0** (R35). The event list, the hop schedule and the f0 ring snapshot (tags + notes) are bitwise equal.
  - **N9c:** seed + 1 differs. **N9n:** `commitHopImmediately` on a tonal recipe (follow on, position 0, density 100 g/s) breaks the 441 vs 32 null (R59).
  - Non-vacuity: events > 0, freeBursts > 0 (N9-F), tempoBursts > 0 (N9-T), confident hops > 0, followHits > 0, followMiss > 0, f0Skips > 0, routed 6 / 7 dests > 0, re-anchors and re-locks > 0 (N9-T).
- **M6 — PERF-01 allocations:** M5 coverage plus:
  - detector events, Free re-phases and Tempo bursts;
  - MPM hops, including the first hop after prepare;
  - follow lookups and pass-2 plans; abs-set rebuilds (a scale, mask or custom apply with follow on);
  - Pitch / Onset route add, remove and swap; freeze toggles (ring skips); `onset_threshold` and `scale_root` automation; sync toggles.
  - 0 allocations; liveness 1 (separate row, as M5).
- **Z6a — QUAL-02 analysis stress:** 60 s at 48 / 44.1 / 192 k.
  - Triggers on ×4 at −60 dB on 0 dBFS noise + 2 ms 0 dBFS bursts (the maximum event rate).
  - Follow ×4 with pitch ±24 and pitch_rand 24; Pitch / Onset routed at ±100 % onto the Z5 hot list; feedback 100 % + freeze toggles.
  - Every sample finite. No subnormal in the analysis state (HP, envelopes, FIR history, decimated window, NSDF scratch, guard, slew, sources) at section ends.
  - Wet ≤ float(A) and output ≤ its gain bound (the Z5 split gate, 2.5a deviation 8).
  - `fbScrubs`, `nanKills`, `envScrubs` and `analysisScrubs` == 0.
- **Z6b — injection:** NaN, +Inf and a 1e30 sample at the tap (`injectAnalysis`). Scrubs as P61 gives (record the counts), no event from them, every hop finite or unconfident, the detector re-arms and fires on the next burst, and the audio is bitwise == the un-injected render (no trigger, no follow, no 6 / 7 route).
- **C6 / C6s (INFO):** P77. C6 > 25 % is a phase fail: stop and investigate.
- Acceptance: N9 (N9c and N9n differ), M6, Z6a and Z6b PASS; C6 ≤ 25 %; C6s printed.
- Depends on: T9.

### Wave 6 — validate + docs

**T11. Static checks**
- Grep only non-comment lines, confirming the baseline at HEAD first:
  - `std::vector|new |malloc|make_unique|std::function|std::thread|std::async` in `Analysis.h` → 0. In `ScaleQuantiser.h`, `StreamScheduler.h` and `Spawner.h` → no increase over HEAD.
  - `fmod` in `Analysis.h` and `ScaleQuantiser.h` → 0 (`o = T − R` is negative below the root).
  - `log10|pow` inside `OnsetDetector`'s per-sample function → 0 (R75; by grep on that function's line range).
  - `juce::dsp::FFT` constructed once (the `Analysis` member); `perform*Transform` called only from `Mpm`.
  - `->setValue|setValueNotifyingHost|beginChangeGesture` in `Source/dsp/` → 0.
  - `getPosition\|getPlayHead` in `Source/` → exactly one call site (`processBlock`).
  - Reported latency is still 0 (`setLatencySamples` untouched).
  - `git diff --stat 2cf30120 -- modules scripts/param-dump` empty.
- Depends on: T10.

**T12. Build, install, validate; host-cache gate; DAW sheet**
- Harness ALL PASS (record the count, ≈ 370) with 0 new warnings; carried diff vs A′ empty.
- `tests/n3/run.sh --strict`, `tests/n2/run.sh --strict`, `tests/n1/run.sh --strict`, `tests/n0/run.sh --strict` and `tests/r12/run.sh` PASS.
- `./scripts/build-and-install.sh O-Granulator` (dual-variant sweep, AU cache clear). `auval -v aufx OuGn OuDv` PASS. pluginval strictness 10 on VST3 + AU (randomised params now hit triggers, follow, the threshold and `scale_root`; memory: pluginval-10 catches a latent NaN).
- **HA (P78):** `uv run --with pedalboard tests/hostcache/run.py` on the installed VST3, with the 10 2.5b params added: they round-trip on decoded values after the CFRunLoop pump; no nested writes. Exit 0.
- Write `stages/2-dsp/DAW-2.5b.md`: per item, a session recipe (Logic and/or Reaper; the generic editor, or the WebView UI once Stage 3 lands; monitors at normal level) and what to listen for.
  1. **Onset bursts on drums, Free and Tempo:** a drum loop with s1's trigger on and the threshold swept from −40 to −20 dB. Free: grains land on the hits and the cloud re-phases. Tempo 1/16 at 120: the grid stays on the click and the hits add grains. Listen for flams, double hits and misses (ghost notes under accents are expected, RESEARCH §2).
  2. **Follow on a monophonic line, at position 0 and at 2 s** (buffer 8 s, position 25 %): Major from the song's key, s2 / s3 at pitch +4 / +7. The harmonies stay in key. At position 0, note changes arrive ≈ 75 ms late (R62 / R69). At 2 s the harmony follows the delayed content, not the live note (D17).
  3. **Frozen-chord harmonisation:** freeze on a held note and keep playing other notes live. The frozen grains keep the held note's harmony (D17); the Pitch source follows the live input.
  4. **Feedback-heavy follow (R74):** feedback 80 % with follow on. It harmonises against the source note; listen for pitch runaway or smearing (feedback content carries the input f0, documented).
  5. **Tempo burst level (R78):** Tempo 1/16 at 120, triggers on, a busy drum loop. A level rise of up to +5.4 dB is accepted (the D9 knee bounds the wet); flag it if it reads as pumping or overload.
- These join DAW-2.5a.md items 1–8 in the Stage 3 DAW pass (VERIFICATION-2.5a). Execute does not wait on the DAW.
- Depends on: T11.

**T13. Docs + state**
- Write SUMMARY-2.5b.md: the gate table with measured values; N3 / N2 / N1 / N0 / R12; HA; C6 / C6s; the A′ attribution; the errata list below; carries to 2.6 / Stage 3 / 3.x; execution deviations.
- STATUS.md → `stage_2_phase_2_5b_execute_complete` (next: verify). REQUIREMENTS stays untouched until verify.
- Commit path-scoped after the re-check: `git commit -- plugins/O-Granulator`.
- Depends on: T12.

---

## Dependency graph

```
T1 ─ T2 ─ T3 ─ T4 ─ T5 ─ T6 ─ T7 ─ T8 ─ T9 ─ T10 ─ T11 ─ T12 ─ T13
(N3) (plumb) └ onset ┘ └ pitch ┘ └ follow ┘ └ gates ┘ └── validate ──┘
```

Strictly sequential. Every task touches `Engine.cpp` / `Analysis.h`, a harness row file or the N3 control set, and each N3n flips at its own task (TRG T3, ONS T4, PIT T6, FOL T8 → strict). T9–T10 only author rows and could overlap T11, but they stay sequential for clean diffs.

---

## Gate criteria (new rows)

≈ 74 harness rows (9 of them INFO), 9 N3 script rows plus the liveness line, the HA script and the T12 validation. INFO rows: OD2i, MP1i, MP1h, OG3, PS4, AQ8, QB2, C6, C6s.

| Row | Measures | Pass | Negative control | Req | Task |
|---|---|---|---|---|---|
| N3 free / capped | out-of-tree null vs `2cf30120`, neutral 2.5b dials, analysis running | max\|Δ\| == 0; below knee; liveness > 0 (strict) | N3c | COMPAT-01 | T1 |
| N3p-THR / N3p-Z | threshold sweep; zero-depth 6 / 7 routes | IDENTICAL at every task | — | FUNC-08, FUNC-06 (R46) | T1–T8 |
| N3n-TRG / ONS / PIT / FOL | one 2.5b element minimally on | each differs (strict from T8) | — | — | T3, T4, T6, T8 |
| FR0 | capacities 8–768 k; R69 constants | ≤ caps; == table | — | PERF-01, DSP-08 | T2 |
| OD0 / OD0n | in-tree analysis isolation | analysisOff == on, bitwise | trigger on differs | FUNC-08 | T3 |
| OD1 / OD1n | sustained tones, 4 rates | exactly 1 event | levelTrigger → 3 | FUNC-08 (R60) | T3 |
| OD2 (+OD2i) | 1 ms bursts, 4 rates | detected at every rate | — | FUNC-08 (R61) | T3 |
| OD3 | refractory | spacing ≥ round (0.05·fs); 55 ms pair → 2 | — | FUNC-08 | T3 |
| OD4 / OD4b | threshold; linear vs dB reference | 0 / all; tick latch; equal event sets | — | FUNC-08 (R75) | T3 |
| OD5 | tap point | return not tapped; input_gain applied; frozen fires | — | FUNC-08 (R74) | T3 |
| OD6 | NaN at the tap | scrub 1, no event, audio bitwise | — | QUAL-02 | T3 |
| OD7 | click-train hit rate, 4 rates | hits == bursts, 0 doubles, −36 dBFS → 0 | — | FUNC-08 | T4 |
| OB1 | Free re-phase (D14 / R73) | grain at the event (Δ 0), frac 0, new epoch; next at +I; disabled / off → 0 | — | FUNC-08, FUNC-03 | T3 |
| OB2 / OB2b | Tempo extra grain (D18); TS1 with triggers on | grid bitwise == trigger off; bursts at `2^63 \| n`; ≤ 1e-6 sample | — | FUNC-08, FUNC-04 | T4 |
| OB3–OB5 | coincidence, frozen, mute / enable edges | see T4 | — | FUNC-08 | T4 |
| PS1–PS3 (+PS4) | Onset law + flush; Pitch law; telemetry | bitwise vs reference; no subnormal; == engine | — | FUNC-06 | T4, T6 |
| MP0 | FFT NSDF vs a double direct reference | Δr ≤ 1e-4·r(0); τ 1e-3; same decisions | — | DSP-08 | T5 |
| MP1 (+MP1i, MP1h) | sines 55–1000 Hz × 44.1 / 48 / 96 k | all confident; ≤ 10 ¢ (expected ≤ 2.8) | — | DSP-08 | T5 |
| MP2 | silence, noises, −55 dBFS sine | 0 confident; −45 dBFS confident | — | DSP-08 | T5 |
| MP3 | hold | exactly the hold length, then unconfident; value held; ring NaN | — | DSP-08 | T6 |
| MP4 | hop schedule | == formula at 3 block sizes | — | PERF-03 | T5 |
| OG1 | injected sequences | L ≤ 2 → 0; leaps exact; returns / non-harmonic immediate; expiry | — | DSP-08 (R66) | T6 |
| OG2 / OG2n (+OG3) | near boundary H2/H1 4.36 + SNR 20 dB | 0 runs < 3 hops | guardOff ≥ 10 short runs | DSP-08 (R67) | T6 |
| FR1–FR4 / FR4n | tags, newest, freeze edge, alignment | see T7 | decision-hop timestamps ≥ 2.25·Hs | D17 (R63, R69) | T7 |
| FR5–FR9 (+n) | content f0, frozen, unconfident gap, position 0, midpoint | f0 from the content; interval mode on a miss | liveF0Lookup, ringHold, strictNewest, lookupAtStart | D17, DSP-08 | T8 |
| FR10 | two-pass draws | non-pitch fields bitwise == follow off | — | D17 (R64) | T8 |
| AQ1 | D19 | 12-TET from the root ≤ 0.001 ¢; follow off unchanged | — | DSP-01 (D19) | T8 |
| AQ2–AQ7 (+AQ8) | masked 12-EDO / 19-EDO / BP / Custom; ±24; nearest; fallbacks; register; rebuild keys | see T8 | — | DSP-01 (R70–R72) | T8 |
| MX1 / MX2 / MX2n / MX2a | Pitch and Onset × classes × ±; removal | direction + range; ≤ bound; κ ≤ 2.0 | MX2n noResidual | FUNC-06 | T9 |
| QB1 / QB1n (+QB2) | burst clicks vs a same-grain-count reference; level | κ ≤ 2.0× | percussive window | QUAL-01 | T9 |
| N9 / N9c / N9n | block null, Free + Tempo, every 2.5b element | max\|Δ\| == 0; events / hops / ring equal | N9c seed; N9n commitHopImmediately | PERF-03 | T10 |
| M6 | allocations | 0; liveness 1 | liveness | PERF-01 | T10 |
| Z6a / Z6b | analysis stress at 3 rates; injection | finite, no subnormal, ≤ bounds; scrubs isolated | — | QUAL-02 | T10 |
| C6 / C6s | CPU, INFO | ≤ 25 % phase fail; spike flag | C5 re-run | PERF-02 | T10 |
| MF6 (carried, rewritten) | Pitch / Onset routed ±100 % | differs from unrouted | — | FUNC-06 | T4 |
| HA | host cache incl. the 2.5b params, installed VST3 | equal | — | COMPAT-01 | T12 |

---

## Success criteria (gate for verify)

| # | Criterion | Req | Task |
|---|---|---|---|
| 1 | N3 null vs `2cf30120` bitwise (free + capped) with the analysis running; N3p-THR / N3p-Z identical; all four N3n differ (strict); N2 / N1 / N0 `--strict` + R12 PASS | COMPAT-01 | T1, T8 |
| 2 | Carried 299: diff vs A empty through T3, vs A′ (the one MF6 line) from T4; W2o unchanged | COMPAT-01 | all |
| 3 | Detector: one event per onset (edge + re-arm), ≥ 1 ms bursts at every rate, refractory, threshold, tap point, NaN isolated | FUNC-08 | T3 |
| 4 | Bursts at the detected sample (Δ 0) on every enabled trigger-on stream, none on disabled / trigger-off streams; Free re-phases; the Tempo grid is bitwise == trigger off; click-train hit rate | FUNC-08 | T3, T4 |
| 5 | MPM ≤ 10 ¢ from 55 to 1000 Hz at 44.1 / 48 / 96 k; 0 confident on silence / noise; hold → unconfident | DSP-08 | T5, T6 |
| 6 | D20: no flip of ≤ 2 hops survives (injected + near boundary; the guard-off control shows them); legit leaps exact | DSP-08 | T6 |
| 7 | D17: content f0 at position > 0 and while frozen; unconfident → interval mode; position 0 follows (R62); midpoint (R64); two-pass draws unchanged | DSP-08 | T7, T8 |
| 8 | D19 + absolute quantisation (masked 12-EDO, 19-EDO, BP, Custom), ±24, nearest, fallbacks, on the recorded target | DSP-01 | T8 |
| 9 | Pitch / Onset modulate every class, bipolar; removal without a step; telemetry == engine | FUNC-06 | T4, T6, T9 |
| 10 | Burst click κ ≤ 2.0× the same-grain-count reference (non-vacuous); burst level reported | QUAL-01 | T9 |
| 11 | Block null with every 2.5b source, trigger and follow on; the seed control differs; the R59 control breaks it | PERF-03 | T10 |
| 12 | 0 allocations over detector events, hops, follow and bursts | PERF-01 | T10 |
| 13 | Analysis stress finite / no subnormal at 3 rates; tap injection isolated | QUAL-02 | T10 |
| 14 | Install; auval; pluginval 10 VST3 + AU; HA; static checks; 0 new warnings; latency 0 | COMPAT-01 | T11, T12 |
| 15 | C6 ≤ 25 % (INFO with a phase-fail ceiling); C6s reported; DAW-2.5b.md ready (items 1–5) | PERF-02, D16 | T10, T12 |

Requirement status expected at verify:
- **Complete:** FUNC-08 and DSP-08 (harness; the DAW confirmation is deferred to Stage 3 and reopens them on a FAIL); FUNC-06 (every source).
- **DSP-01** stays complete (re-gated under follow, AQ rows).
- **QUAL-01 / QUAL-02 / PERF-01 / PERF-03** advanced: the 2.5b remainders close; full-feature hardening is 2.6.
- **PERF-02:** C6 measured; the budget gate is 2.6.

---

## Errata for SUMMARY-2.5b (record at T13; ARCHITECTURE / ROADMAP / CONTEXT not edited — checksums)

1. **R59** hop commit at the next gridTick. ARCH §15 "runs at hop boundaries": computed there, committed at the tick.
2. **R60** (signed off) edge-triggered detector with a +3 dB re-arm, vs ARCH §8 / §15 level trigger.
3. **R61** detection stimuli are ≥ 1 ms bursts; impulses are rate dependent; FUNC-08 offsets are measured against the reported event.
4. **R62** a read point newer than the newest entry uses the newest entry (D17).
5. **R63** ring-domain f0 ring via the per-cell log, with tagged entries. N ≈ 3.5–4.0 k entries (41 s ring), not CONTEXT's "≈ 2.8 k at 30 s".
6. **R64** (signed off) content-midpoint lookup through a two-pass plan, narrowing D17's "read start".
7. **R65** no hold in the ring; the hold belongs to the live source only.
8. **R66** (signed off) D20 asymmetric harmonic-class lookahead, N = 3, replacing the symmetric 2-hop octave hysteresis. Uniform 3-hop decision lag. P67's acc expiry after the hold is a plan refinement.
9. **R67** D20 rows are injected sequences plus near-boundary audio with a guard-off control. Breathy tones (never flip) and a persistent strong H2 (tracks the octave) are INFO. This replaces CONTEXT's D20 stimuli.
10. **R68** m(τ) computed only up to τ_max. **R69** lag constants (D = 4: 2077.5 / 3613.5).
11. **R70** absolute set with 1/1 at C4 + root (no GrainScatter precedent). P71 builds it over the maximum span (key without the span).
12. **R71** (signed off) nearest member at pitch_rand 0. An empty set or no member → interval mode (P71).
13. **R72** D19 and the quantisation rows gate the recorded target; the sounding pitch is INFO. RESEARCH §6's MIDI 100 clamp case is a unit row (it is outside the 40–1500 Hz tracker range).
14. **R73** Free re-phase via `restart()` (ARCH §4's `rescheduleNext (0)` is not used); Tempo bursts on the `2^63 | n` counter. That is how CONTEXT's "reserved stream / slot" is realised.
15. **R74** the conditioned input is kept: feedback-heavy content gets the input's f0.
16. **R75** linear compare; analytic Onset source with τ = 150 ms. P64 flushes it to 0 after 16 τ.
17. **R76** no-heap FFT; f0 is not bitwise across platforms.
18. **R77** the hop is not spread (C6 / C6s).
19. **R78** Tempo bursts sit outside the R5 law (≤ +5.4 dB, accepted). Follow doesn't feed the law either (P72).
20. **P65** the 64-tap decimator is kept at every rate (fold attenuation −8.7 dB at 192 k; MP1h INFO).
21. **P68** Pitch source starts at 0 and slews from 0 (RESEARCH §8 recommendation confirmed).
22. **P74** the MF6 inert carried row is rewritten (A′).
23. Any execution deviation, with the measured reason, and the final harness count.

## Carried (not implemented in 2.5b)

- **2.6:** the full PERF-02 budget gate; `getTailLengthSeconds()`; the full QUAL-01 click sweep, including route edits under all sources; the full-feature PERF-03 null, including the oversized host block (8192 after prepare 512); 96 / 192 k smoke renders; the decimator taps above 96 k if MP1h flags them.
- **Stage 3 DAW pass:** DAW-2.5a.md items 1–8 (watch item 6) + DAW-2.5b.md items 1–5.
- **3.x UI copy:** Pitch source latency ≈ 75 ms + the 20 ms slew (R69); the 40–1500 Hz range covers 0.04–0.79 of the Pitch source (RESEARCH §8); follow "harmonises against the source note" (R74); the R45 size-sync tooltip. `getPitchNote` / `getPitchConfident` and the telemetry accessors feed the UI.
- **Out of scope:** the O-GrainScatter stopped-transport re-fire (R43), a separate GrainScatter issue.
- W2o rule unchanged: widen seeds, never the threshold.

---

## Risks / watch items

- **FP contraction / reordering breaks N3.** The analysis must touch no audio statement: env stays verbatim; the onset-array resize and `mergeOnsets` must keep the order; the second `Spawner::plan` call runs only for follow streams; the Tempo insertion runs only for trigger-on streams. Bisect to the statement; never loosen N3.
- **N3p-THR or N3p-Z fails** → the analysis leaks into the matrix: a zero-valued Pitch / Onset source makes a destination look routed (−0.0f, or a residual stuck at a subnormal), or the Onset flush disturbs a residual. Treat −0.0f as 0, as in 2.5a.
- **Vacuous negative controls** (memory: a stimulus below threshold). OD0n, OD1n, OG2n, FR4n, FR5n–FR9n, MX2n, QB1n, N9n and the four N3n must each fail their gate **before** the subject row's PASS is trusted. If one doesn't, re-derive the stimulus and record it.
- **MPM model mismatch.** MP0 is the oracle. R68's ≤ 2.78 ¢ is the expectation; > 3 ¢ anywhere means the port differs from the model: investigate, don't accept a pass on the 10 ¢ margin.
- **64 taps above 96 k** (P65). If MP1h shows confident wrong hops at 176.4 / 192 k, **stop and ask Taylor.** The fallback (taps = 16·D) changes the lag constant to `(16·D − 1)/2` and the CONTEXT contract.
- **Follow latency.** At position 0, follow uses an entry ≈ 75 ms old (R62 / R69), so a note change reaches follow grains late. Accepted; DAW item 2 and the Stage 3 copy cover it.
- **Refractory-edge fire.** Under the signed-off R60 rule, a second transient inside the refractory can fire at the refractory's last sample. OD3 records it. If DAW item 1 hears late flams, the remedy (disarm on any in-refractory condition) needs a new sign-off.
- **Onset flush and the R46 raw path.** When the Onset source reaches exactly 0, a routed destination returns to the raw path (a ≤ 1-ulp round-trip step). Inaudible; MX2 and Z6 cover it.
- **Bursts in a capped pool.** Tempo extra grains steal under R8. QB1 runs at grain_limit 128; the capped case is covered by Z6a's finiteness only. The burst-steal click belongs to 2.6's full QUAL-01 sweep.
- **R4 epoch at an event on i = 0 after an edge.** The epoch is unchanged, and the re-phase grain is the reset's own index 0, not a replay. OB5 covers it.
- **FFT backends.** AppleFFT (vDSP) on macOS; the Windows FFTFallback uses a 16.4 KB `alloca` (stack). M6 covers macOS only; Windows CI pluginval exercises the fallback. Never assert a bitwise f0 across platforms (R76).
- **Harness wall time** grows: MP1 7 freqs × 3 rates, OD rows × 4 rates, OG2 6 s, OB2b 32 TS1 cases, N9 2 recipes × 5 variants × 12 s, Z6a 60 s × 3 rates. Use the render pool and run in the background; don't thin the grids.
- **Carried C rows** (C2–C5) rise by the always-on analysis (≈ 0.3 %). They are INFO / timing lines, dropped from the carried diff; C3's ceilings have ample margin.
- **Do not touch any module** (grain-engine 1.1.0, pitch-collections 1.0.0, reverb-unit 1.2.0). The N3 refusal enforces it.
- **Ephemeral references.** The research scratch (`/private/tmp/claude-501/…/55810a67-…/scratchpad/r25b/`: `mpm.py`, `guard2.py`, `onset.py`, `drums.py`) may be gone. This plan and RESEARCH-2.5b are normative; if the scripts survive, they are the reference models for MP0 / OG1 / OD.
- **Shared checkout:** path-scoped commits only. Re-check the branch and staging immediately before each commit. Other sessions' changes under `.claude/` and `plugins/O-GrainScatter/` are in the tree.
