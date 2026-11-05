# Stage 2 — DSP · Phase 2.1 (Ring + pass engine + one stream) — Context

**Plugin:** O-Granulator
**Stage:** 2 of 4 (DSP)
**Phase:** 2.1 of 6 (Ring + pass engine + one stream)
**GSD phase:** discuss
**Date:** 2026-11-04
**Branch:** `main` (trunk-based) @ `3e161927`
**Participants:** Taylor Brook, Claude

---

## Entry Check — contract checksums at the stage boundary

All six byte-exact against STATUS.md frontmatter (re-hashed in this session):

| Contract | SHA-256 | Result |
|---|---|---|
| `BRIEF.md` | `f9c748a8…6559d7` | ✅ |
| `REQUIREMENTS.md` | `b9ba9106…36495d` | ✅ |
| `parameter-spec-draft.md` | `d0b1457e…cbbce3` | ✅ |
| `parameter-spec.md` | `f5ce3595…b3688` | ✅ |
| `research/ARCHITECTURE.md` | `f0cf753b…d9d9` | ✅ |
| `ROADMAP.md` | `79d69576…4b20` | ✅ |

No contract drift. Phase 2.1 plans against these documents.

---

## Discussion Summary

ARCHITECTURE.md specifies 2.1's algorithm completely: the per-pass order (§Processing Chain), the ring
(§2), the scheduler with telescoping jitter (§4), the spawner and reach clamp (§5), the pool (§6), the
renderer (§7), the output law (§10), dry/wet (§14) and the control grid (§16). Discuss did **not**
reopen any of it. It settled three policy questions the architecture left open. Execution defaults
for the rest are recorded below.

Per-phase convention (as O-Octagon / O-simpleAmbisonics): each Stage-2 phase gets its own
`CONTEXT-2.N.md → RESEARCH-2.N.md → PLAN-2.N.md → SUMMARY-2.N.md → VERIFICATION-2.N.md`.

---

## Approach Decisions (locked this session)

| # | Decision | Choice | Rationale |
|---|---|---|---|
| D1 | grain-engine gaps G2 (`GrainPool::fadeOut`) and G3 (`GrainScheduler::rescheduleIn`) | **Plugin-local workarounds for the whole of Stage 2.** Use a Grain `killGain/killStep` field (5 ms) multiplied with `fade.next()`, and a scheduler wrapper that tracks `sinceOnset` itself. grain-engine 1.1.0 is consumed **unmodified**. G1–G3 are promoted to grain-engine 1.2.0 after Granulator ships, as a separate `/module-*` task. | grain-engine has 5 shipped consumers (RD, GS, simpleGrain, TextureForge, Polystutter). A mid-stage module change would bring their regression gates into this stage. |
| D2 | Ring capacity at high sample rates | **Full `kRingSeconds = 41.0` at every rate.** That is 27.6 MB @ 48 k, 55 MB @ 96 k and 110 MB @ 192 k per instance. `buffer_length` stays 1–30 s at every rate. No UI limit state. | Simplest option, and position-knob meaning stays the same at every rate. 192 k is rare. Drops the ARCHITECTURE "≥ 96 k → 20 s" fallback. |
| D3 | Human listening checkpoints in Stage 2 | **End of 2.1 and end of 2.4 only.** Both use the Generic editor in Standalone and/or a DAW. All other phases rely on automated gates only. | 2.1 checks that the skeleton sounds right. 2.4 checks the freeze and feedback semantics, which are the ones flagged for hands-on A/B. |

D3 checkpoint for **2.1** (human, at verify): with one stream on live input, check that position scans the buffer, pitch ±24 st sounds clean, reverse prob works, `buffer_length` moves without clicks, and mix 0/50/100 behaves as expected.

D2 is a cross-phase decision: 2.6's "96 k and 192 k smoke renders" become an **allocation-size check** (prepare at 192 k allocates ~110 MB on the message thread, with no audio-thread allocation), not a clamp check.

---

## Requirements Confirmed (Phase 2.1 scope — from ROADMAP § Phase 2.1)

**Components:** I/O adapter, capture ring `MipRing<2>` (41 s, with the derivation in a comment plus a `static_assert`), write arbiter (live only, so no freeze yet), StreamScheduler (Free, telescoping jitter), spawner (position / spray / size / pitch / reverse, reach clamp, double read positions), `GrainPool<Grain,128,16>`, renderer (`readStereo`, window last), output law (incoherent, cap-aware N_eff), equal-power dry/wet + output gain, and the 32-sample control grid with base ramps.

**Not in 2.1:** feedback (the `fbDelay` z⁻³² buffer exists and is all zero), freeze, FX, window morph (Hann only), scale quantiser, streams 2–4, matrix/sources, tempo.

**Test criteria (gate):**
- [ ] Input audible within one onset at position 0 (FUNC-01)
- [ ] `buffer_length` change: no click, no stale-region read (FUNC-01)
- [ ] Mix 0 % is a bitwise dry null; mix 100 % has no dry (FUNC-07)
- [ ] +24 st swept sine: aliases ≥ 60 dB down (DSP-02)
- [ ] Long-buffer pitched read: periodic-corpus null + double reference at 25 s (DSP-01)
- [ ] Block-size null at 32 / 441 / 512 / 4096 / variable, randomisation ON, seeded, plus a different-seed control (PERF-03)
- [ ] Large-block probe: host block ≥ the minimum grain delay; oversized block (8192 after prepare 512) does not overrun
- [ ] Allocation gate across spawn and steal (PERF-01)
- [ ] D3 human listening checkpoint

---

## Execution Defaults (Claude's call, recorded for plan)

- **4-stream shape from day one, 1 stream active.** Engine arrays, buses and the scheduler table are sized `[4]` now. In 2.1 only stream 1 is honoured; `s2–s4_enable` are forced off inside the engine (no draws, no spawns). 2.2 then removes a mask and does not refactor anything.
- **Grain POD declared at its final layout** (FX state fields present and value-initialised, but unused). `activate()` already value-initialises the slot, so 2.3 adds no new bleed risk.
- **Sinc reads everywhere** (16-tap `SincBank`). The Hermite lever waits for 2.3's CPU measurement. 2.1 records a **non-blocking** CPU baseline (128 grains, no FX, 48 k/128, as a ratio vs engine-bypass) so 2.3 starts with data.
- **Stream-1 parameters honoured in 2.1:** `s1_enable`, `s1_level`, `s1_pan`, `s1_position`, `s1_spray`, `s1_size`, `s1_size_rand`, `s1_density`, `s1_jitter`, `s1_pitch`, `s1_pitch_rand`, `s1_reverse_prob`, plus globals `input_gain`, `buffer_length`, `grain_limit`, `mix`, `output_gain`. Quota water-filling runs with one stream, so the quota is just `grain_limit`.
- **Source layout:** `Source/dsp/` headers (Engine, CaptureWriter, StreamScheduler, Spawner, Grain, OutputLaw, ControlGrid), owned by `PluginProcessor`. Tests extend `tests/render-harness/main.cpp`.
- **Seed:** the per-instance clock seed is unchanged (Stage 1). The harness injects seeds. Null tests use seeded pairs (memory: null test with clock-seeded Random).

## Constraints Identified

- **Stage 1 harness B1–B3 assume pass-through at defaults, but `mix` defaults to 50 %.** Once 2.1 lands, defaults are no longer bitwise dry. B1–B3 must be **rescoped to mix 0 %** (still bitwise across 3 layouts × 6 block patterns), and the defaults row is replaced by "wet present". This is a deliberate gate change, not a regression. The plan must say so explicitly.
- Mono→mono: engine input L=R; output `(L+R)/2` of the final mix. Channel counts come from the **bus**, not the buffer.
- Every per-pass array is a fixed 32 samples. `static_assert(kCell <= kFbDelay)`.
- Reach-clamp floor derives from parameters (r, len), never from pass or block size.
- Draw before obtain: refused or stolen spawns consume the same draws as a granted one.
- Ties in onset order: (offset, stream index).
- Gain laws come from latched parameters at the grid, never from `liveCount` / `liveWeight()`.
- Custom ramps, not `SmoothedValue::reset()`, because reset copies a NaN target.
- No APVTS writes from the audio thread or from `parameterChanged`.
- `ScopedNoDenormals` in `processBlock`.

## Carry-ins from Stage 1 (scheduled)

| Item | Phase |
|---|---|
| Sort the active pitch table (pitch-collections `JustSevenLimit` unsorted at source) | 2.2 (scale quantiser) |
| Audio thread owns its own copy of the custom scale table | 2.2 |
| Never derive unity gain from `get()` on skewed ranges | 2.1 onward (output_gain, levels): use `Decibels::decibelsToGain` on the denormalised value; `0 dB → 1.0f` exact |

## Open Questions (for research)

- `MipRing<2>` write path: confirm the cost of per-sample `push` vs a per-pass block write at 41 s capacity, and that `getWriteCount()` is 64-bit (it overflows at 192 k only after centuries if 64-bit, but must be checked).
- `buffer_length` change mid-flight: the spawner reads `span` at spawn time, so sounding grains keep their latched D. Confirm that this alone satisfies "no stale-region read" after a shrink, i.e. that no new grain draws beyond the new span and old grains are allowed to finish.
- Pin the exact output-law form for a single stream at the cap (N_eff with `quota = grain_limit`) against the density-norm memory (power = cap × mean w² over [0, cap/N]).

## Next Phase

Ready for: research (`/plugin-research O-Granulator 2-dsp`, Phase 2.1). Research can be thin because the architecture is closed. Skipping straight to plan is acceptable.
