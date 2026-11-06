# Stage 2 — DSP · Phase 2.6 (Hardening) — Context

**Plugin:** O-Granulator
**Stage:** 2 of 4 (DSP)
**Phase:** 2.6 of 7 (last Stage 2 phase)
**GSD phase:** discuss
**Date:** 2026-11-06
**Branch:** `main` (trunk-based) @ `454c8a31`
**Participants:** Taylor Brook, Claude

---

## Entry Check — contract checksums

All six were re-hashed this session and match the STATUS.md frontmatter. No contract is amended at this discuss.

| Contract | SHA-256 (prefix) | Result |
|---|---|---|
| `BRIEF.md` | `f9c748a8…` | ✅ |
| `REQUIREMENTS.md` | `5f481caa…` | ✅ (2.5b-verify revision) |
| `parameter-spec-draft.md` | `d0b1457e…` | ✅ |
| `parameter-spec.md` | `f5ce3595…` | ✅ |
| `research/ARCHITECTURE.md` | `f0cf753b…` | ✅ |
| `ROADMAP.md` | `1060dae0…` | ✅ |

---

## Discussion Summary

2.6 adds no features. It closes the five open requirements across the whole feature set built in 2.1–2.5b: QUAL-01, QUAL-02, PERF-01, PERF-02 and PERF-03. The ROADMAP test list plus the items carried from VERIFICATION-2.5b define the scope.

**Facts established from the tree:**
- `getTailLengthSeconds()` returns `0.0` (`Source/PluginProcessor.h:65`). This is wrong for freeze and for feedback.
- `Engine::prepare()` ignores `maxBlockSize` ("nothing in process() scales with the host block", `Engine.cpp:32`). The reverb is prepared at `kCell` (`Engine.cpp:119`). The oversized host block (8192 after prepare 512) should therefore be structurally safe. This is the case named in the memory pattern *oversampler overruns on oversized host block*, and it still gets a gate. "Should be safe by reading" is not a gate.
- `processBlock` already holds `juce::ScopedNoDenormals` (`PluginProcessor.cpp:253`). QUAL-02 tests the result; it is not a missing guard.
- The CPU readings so far are C6 15.30 % (verify) and 15.77 % (execute). These were measured on phase stimuli, not on a maxed-out worst case.

Three points were reopened. All three were decided on the recommended option.

---

## Requirements Confirmed

| Req | 2.6 closes it with |
|---|---|
| **QUAL-01** No artifacts | Click sweep: freeze toggle, grain steal in a capped pool (incl. burst steal), stream enable / mute / solo, matrix route add / remove / depth edit under **every** source (LFO ×4, env, macros, Pitch, Onset). Each verdict is taken against a same-motion reference (memory: *step click gate needs same-motion control*), with a tonal stimulus. Noise channels make curvature probes vacuous. |
| **QUAL-02** No NaN / denormal | Extreme-settings stress render: every Float param at its max (and min where that is the hazard: res, cutoff corners), feedback 100 %, all FX, reverb + shimmer, 16 slots routed at full depth from every source. Gate: no NaN / Inf; no subnormal in output, ring, feedback state, reverb tank or filter states. Run at 44.1 / 48 / 96 / 192 k. |
| **PERF-01** RT safety | Whole-feature allocation gate: zero allocations over spawn, steal, freeze toggle, matrix edits, transport jumps, tempo changes, onset bursts, MPM hops, and preset-like mass parameter changes. M5 and M6 are subsumed; the gate has one scope, the whole feature (memory: *gate inherits the review finding's scope*). |
| **PERF-02** CPU | **D22** worst case at 48 k / 128 ≤ 30 % of one core. `grain_limit` engages deterministically: the same seed gives the same steal sequence, and the live count never exceeds the cap. |
| **PERF-03** Block-size invariance | Full-feature seeded null at 32 / 441 / 512 / 4096 / variable, **plus** the oversized host block: 8192 after `prepare(…, 512)`. Every 2.5a / 2.5b element is active, as in N9. |
| — | 96 k and 192 k smoke renders: filters clamp below Nyquist (memory: *SC filter needs Nyquist clamp*), ring memory matches `kRingSamples(fs)` as documented, and no NaN. |
| — | **D21** `getTailLengthSeconds()` reports infinite. |

---

## Approach Decisions

| # | Decision | Choice | Rationale |
|---|---|---|---|
| **D21** | Tail length | `getTailLengthSeconds()` returns `std::numeric_limits<double>::infinity()`. JUCE maps this to VST3 `kInfiniteTail` and an AU tail of ∞. | Freeze holds content forever, and feedback near 100 % decays very slowly. Any finite value either lies or pads every bounce. Hosts must not cut the tail. Offline bounces rely on the user's bounce length. Research confirms the JUCE VST3 / AU mapping and pluginval / auval behaviour with ∞ (no test may treat ∞ as a failure). |
| **D22** | PERF-02 worst case | **Everything maxed:** 4 streams enabled, density maxed, `grain_limit` 128 saturated (steals every pass), every per-grain FX on (filter at max res, drive, crush, ring), feedback 100 % + diffusion + saturation, reverb + shimmer + mod, 16 matrix slots routed from every source, onset + MPM live on a tonal + transient input. Budget **≤ 30 % of one core at 48 k / 128**. Phase-fail > 30 %. 96 k and 192 k are measured and reported as **INFO** with no budget. | This is the ARCHITECTURE budget, stated as a hard gate on the hardest config the parameter set allows. High-rate cost is reported, not gated. The smoke renders cover correctness there. |
| **D23** | Verify mode | **Fully automated.** No LISTENING-2.6.md. | The real-material ear checks are already scheduled: the Stage 3 DAW pass (DAW-2.5a items 1–8, DAW-2.5b items 1–5) and Stage 4 QUAL-04. 2.6 gates are measurable (click ratios, NaN / subnormal census, alloc count, bitwise null, CPU %). |
| **D24** | Regression baseline | 2.6 opens with the **388-row harness re-run on the 454c8a31 tree → baseline A″**, plus N3 / N2 / N1 / N0 `--strict` and R12. Every carried row must diff empty against A″ at every task. Any re-baseline needs an attributed cause, as P74 did in 2.5b. | 2.6 is hardening. A carried-row move means a fix changed audio, and it has to be named. D21 changes no audio. |
| **D25** | Fix policy | If a gate finds a defect, fix it in 2.6 and record it as an execution deviation. The fix is scoped to the class, not the single instance (sweep every site of the same pattern). A fix that changes neutral audio must be signed off at verify. | Hardening exists to find defects. This prevents a gate written to one instance from printing green over its siblings. |
| — | W2o | Carried rule, unchanged: if W2o (+0.483 dB, margin 0.017 dB) moves, widen the seeds. **Never** widen the threshold. | Set in 2.2 verify. |

---

## Constraints Identified

- **No new features and no parameter changes.** The parameter contract stays LOCKED at 220. `tail` is a processor override, not a parameter.
- **No module change expected.** If a defect is in grain-engine or reverb-unit, stop and ask before bumping a module version. Siblings consume them.
- Reported latency stays 0.
- Harness wall time is already 267 s. Long renders (extreme × 4 rates, CPU sweeps) run in the background (memory: *600 s executor watchdog*).
- CPU readings are noisy. Use a median of N runs on a quiet machine and report the spread. A single reading above 30 % with a median below 30 % is flagged, not a fail. Research sets N.
- Click probes: tonal source, linear wet sum, and a same-motion reference per verdict. A burst-steal probe without a steal-free control is vacuous (memory: *grain steal fade*).
- Allocation gate: count allocations, not distinct buffer addresses (memory: *distinct buffer addresses are not an allocation bound*).
- Oversized-block gate runs on a **fresh instance**, not one primed by smaller blocks (memory: *stale host instance vs offline repro*).

---

## Open Questions (for research)

1. **D21 mapping:** confirm the JUCE 8.0.15 VST3 wrapper → `kInfiniteTail`, and the AU wrapper → `kAudioUnitProperty_TailTime` = ∞. Check whether pluginval 10 or auval reject or warn on an ∞ tail.
2. **D22 stimulus:** which input makes the worst case honest (MPM confident every hop, onset firing near its refractory rate, steals every pass)? Which parameters are actually the costliest at max? For example, is max density with the minimum size cheaper than max size? Research picks the true argmax: a small sweep of the expensive axes, not a guess.
3. **CPU method:** how many runs, warm-up, wall vs thread time (C3w / C3t precedent), and how to keep the measurement block-size honest at 128.
4. **QUAL-01 sweep matrix:** sources × destination classes (ramped / latched / tick) × edit type (add / remove / depth step). Pick a representative subset with the reasoning, or run the full cross if it fits the wall budget. Thresholds follow the QB1 precedent (ratio vs same-motion reference).
5. **QUAL-02 subnormal census:** which internal states can be probed through the test accessors (ring, fb chain, SVF states, reverb tank, MPM buffers), and how long the decay-to-silence tail must run to catch late subnormals after the input stops (freeze off, feedback 100 %).
6. **PERF-03 variable block:** reuse the 2.5a MockPlayHead v2 + event splits. The oversized 8192 case needs the transport and event schedule to stay identical to the 512 reference.
7. **192 k:** ring memory at 41 s × 192 k × 2 ch. Confirm the documented figure, the `prepare` time, and that the 64-tap decimator watch (MP1h, worst 0.76 ¢) still holds under the full-feature load.

---

## Next Phase

Ready for: **research** (`/plugin-research O-Granulator 2-dsp`).
