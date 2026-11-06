# Stage 2 — DSP · Phase 2.5a (Tempo sync, LFO/env/macro sources, modulation matrix) — Context

**Plugin:** O-Granulator
**Stage:** 2 of 4 (DSP)
**Phase:** 2.5a of 7 (2.5 split into 2.5a / 2.5b at this discuss)
**GSD phase:** discuss
**Date:** 2026-11-05
**Branch:** `main` (trunk-based) @ `42d30b01`
**Participants:** Taylor Brook, Claude

---

## Entry Check — contract checksums

All six were re-hashed this session and matched the STATUS.md frontmatter. After that, ROADMAP.md was amended (D13) and re-hashed.

| Contract | SHA-256 (prefix) | Result |
|---|---|---|
| `BRIEF.md` | `f9c748a8…` | ✅ |
| `REQUIREMENTS.md` | `418ffb86…` | ✅ (2.4-verify revision) |
| `parameter-spec-draft.md` | `d0b1457e…` | ✅ |
| `parameter-spec.md` | `f5ce3595…` | ✅ |
| `research/ARCHITECTURE.md` | `f0cf753b…` | ✅ |
| `ROADMAP.md` | `79d69576…` → **`1060dae0…`** | ✅, then amended by D13 |

---

## Discussion Summary

ARCHITECTURE already specifies most of 2.5:
- the TempoTracker port and grid lock (§17, Algorithm §6)
- the sources (§15)
- the matrix in normalised space, 20 ms slot ramps, shadow crossfades, ramped vs spawn-latched destination classes, power-of-two Tempo density modulation, and the append-only 113-entry table (§16)
- the telemetry atomics (Telemetry)
- no APVTS write-back (Decisions)

Discuss reopened four points: phase size, burst semantics, the LFO phase model under rate modulation, and a human checkpoint.

Existing hooks in the code:
- `Engine.cpp:573` — Free interval only ("2.5: Tempo")
- `Engine.cpp:439` — the reverb atomics are written with base values ("2.5's matrix modulates them here")
- `Engine.h:51` — "analysis — 2.5"
- `ScaleQuantiser.h:47` — the key becomes a mod destination
- `StreamScheduler.h:31` — Free only

---

## Approach Decisions (locked this session)

| # | Decision | Choice | Rationale |
|---|---|---|---|
| D13 | Phase scope | **Split 2.5 → 2.5a / 2.5b.** **2.5a:** TempoTracker + sync divisions + Tempo density/size modulation, LFO ×4, env follower, macros, the 16-slot matrix, telemetry, and the matrix → reverb atomics. **2.5b:** onset detector + burst, decimator + MPM, absolute pitch follow with `scale_root`, and the Pitch and Onset sources going live. ROADMAP is amended (`1060dae0…`) and Stage 2 now has 7 phases. 2.6 stays Hardening. | 2.5 had 9 components, about twice the size of 2.4. The matrix is the hub every source plugs into, so it lands and gets gated first with the simple sources. Then 2.5b adds the two analysis-heavy sources to a matrix that is already proven. |
| D14 | `sN_onset_trigger` semantics (built in 2.5b, locked now) | **Re-phase, and the clock keeps running (ARCH §4 as written).** At the detected sample i: `process(i)` → `rescheduleNext(0)`, so one grain fires at i and the stream's clock re-phases to i. Normal density continues from there. This is additive: with no transients, the stream behaves exactly as it does with the trigger off. | It locks the cloud rhythmically to the input without a silent "gated" mode. The 50 ms refractory bounds the extra spawns, and the 16 fade slots already cover it. |
| D15 | LFO phase model | **Hybrid.** With the rate **unmodulated**, the phase is computed directly: Free = `frac(φ0 + n·inc)` on the absolute sample counter, and Tempo+playing = `frac(ppq/divBeats + φ°/360)`, which is exactly grid-locked with no accumulated ppq. With the rate **modulated** (any active slot targeting 109–112 with a non-zero ramped depth), a per-LFO **double** phase accumulator advances once per absolute 32-sample cell by the modulated increment. At transport start, locate, or loop (the ppq discontinuities that the tempo tracker already detects), the accumulator re-locks to the direct formula. The handover between the two modes is seamless: the accumulator is seeded from the direct phase on every cell while the rate is unmodulated, so it starts from the current phase when modulation begins. When modulation ends, the phase holds its accumulated value until the next re-lock, with no jump back. Tempo-mode rate modulation is a **continuous** multiplier `2^(4·offset)`. It is not rounded, because an LFO has no grid to land on. | A purely stateless phase jumps under rate modulation. That would break FUNC-06 "any destination" for 109–112 and QUAL-03. Both modes are grid-driven, so block-size invariance holds. The direct path keeps synced LFOs exactly on the bar when nothing modulates them. |
| D16 | Human checkpoint | **DAW tempo check at verify (amends D3).** Logic or Reaper, monitors at normal level. 2.5a checks: synced grains against the click, re-timing on start/loop/locate, a tempo ramp, LFO sweeps of position/pitch/cutoff/mix for zipper, and LFO sync across a loop point. 2.5b checks: onset bursts on a drum loop and pitch-follow harmonisation on a monophonic line. | Tempo sync only proves itself against a real host transport. The synthetic-playhead rows can't reproduce host-specific playhead quirks such as pre-roll, loop wrap, or a Logic tempo ramp. |

Unchanged: D1–D12, except that D16 amends D3 (listening now also at 2.5a/2.5b verify).

---

## Requirements Confirmed (Phase 2.5a scope)

**Goal:** FUNC-04, FUNC-06 (LFO/Env/Macro sources; Pitch/Onset complete in 2.5b), QUAL-03. Re-gates QUAL-01, PERF-01 and PERF-03 under modulation (carried from VERIFICATION-2.4).

**Components:**

- **TempoTracker (ARCH §17):**
  - Port from O-GrainScatter `dsp/TempoTracker.h`: `ppqAt(i) = ppqOrigin + (sampleOffset + i)·ppqPerSample`. It works from integer samples, never an accumulated ppq.
  - At every grid point in Tempo mode while the transport is playing, compute the next grid onset `ceil(ppq/div)·div`. If the pending onset is off by more than 0.5 sample (after start, loop, locate or a tempo change), re-time it.
  - When the transport is not playing or there is no playhead, free-run at the host bpm (120 if absent).
- **Sync divisions:**
  - `sN_size_sync` (20), `sN_density_sync` (19) and `lfoN_rate_sync` (22) map to quarter-note beats.
  - T = ×2/3, D = ×3/2.
  - Interval in samples = `div_beats·60/bpm·fs`, computed in double.
  - `sync_mode` swaps which parameter is read. Both stay registered, and nothing is written.
- **Tempo-mode modulation of size/density:**
  - The offset becomes a multiplier `2^(4·offset)` on the synced value.
  - Density is rounded to the nearest power of two, so onsets stay on the grid.
  - Size is continuous.
- **LFO ×4:**
  - Six shapes, bipolar [−1, 1].
  - S&H and smooth random use `hash(cycleIndex)` via CounterRng, which needs a reserved draw slot (no 2.2–2.4 draw moves). Smooth random interpolates between `hash(c)` and `hash(c+1)`.
  - Phase follows D15. `lfoN_phase` re-locks on transport start and in `prepareToPlay`.
- **Envelope follower:**
  - A per-sample one-pole on `|x·envGain|`. Attack/release coefficients are computed at grid ticks.
  - Output maps −60..0 dB → 0..1 (unipolar).
  - NaN-sticky guard.
- **Macros ×4:** APVTS floats, smoothed on the control grid, unipolar.
- **Matrix (ARCH §16):**
  - 16 slots. `v = range.convertFrom0to1(clamp(base01 + Σ depth·src, 0, 1))`, so skew is respected.
  - Multiple slots on one destination sum **before** a single clamp.
  - Each slot's depth ramps over 20 ms. A change of source or destination cross-fades a 20 ms shadow of the old route to 0.
  - Ramped destinations interpolate inside the cell (`CellLerp`). Spawn-latched destinations read the interpolated value at the spawn sample.
  - The matrix writes the reverb atomics (D11). This closes the carry from VERIFICATION-2.4.
  - Destinations whose parameter is a scale key (ScaleQuantiser) follow the existing note in `ScaleQuantiser.h:47`.
- **Pitch and Onset sources (indices 6, 7):** present in the matrix and output 0 in 2.5a. A route to them is valid and inert. They go live in 2.5b.
- **Telemetry:**
  - `std::array<std::atomic<float>, 113>` holds the modulated normalised destination values, written per cell with relaxed ordering.
  - There are 11 source-value atomics.
  - The engine **never** writes APVTS.
- **Append-only destination table:** a golden test pins the 113-entry ID table (index ↔ param ID ↔ host string).

**Test criteria (gate):**

- [ ] **Neutral null first (N2):** every slot None, `sync_mode` Free, all macros 0. The render must be bit-identical to the 2.4 tree (out-of-tree vs `42d30b01`, N0/N1 style), with the 225 carried rows unchanged. This has to hold before any 2.5a row counts. Negative controls, each of which must DIFFER:
  - one slot LFO→level at depth 1 %
  - `sync_mode` Tempo
  - macro1 routed at 1 %
  - a slot assigned at depth 0. It must NOT differ: a zero-depth route is identity. This one is a positive control.
- [ ] **FUNC-04a:** with a synthetic playhead (constant bpm, then a tempo change, a loop and a locate), synced onsets land within 1 sample of the host grid at jitter 0 for every density division.
- [ ] **FUNC-04b:** Free density is within 2 % over 10 s at jitter 0 (the existing 2.1 row re-runs under the 2.5a tree).
- [ ] **FUNC-06:** each 2.5a source (LFO 1–4, Env, Macro 1–4) modulates a sample of destinations across every class: ramped, spawn-latched, skewed range, Choice-backed if any, and the reverb atomics. Both depth signs. Removing a route returns to base with no step, judged with a same-motion control.
- [ ] **D15:** LFO rate modulation produces no phase discontinuity (max |Δphase| per sample ≤ the modulated increment bound). An unmodulated synced LFO stays exactly ppq-locked over 64 bars, with a bitwise phase match to the direct formula. Re-lock happens at loop/locate.
- [ ] **QUAL-03:** LFO sweeps of position, pitch, cutoff and mix are free of zipper. Check this with the step-click gate against a same-motion reference (memory: step click gates), with a tonal source.
- [ ] **Host automation:** the host cache matches the plugin for `modM_*`, `lfoN_*`, `sync_mode` and `macroN` (pedalboard + CFRunLoop pump). No nested writes.
- [ ] **PERF-03:** block-size null at 32/441/512/4096/variable with LFO (all six shapes) + env + macro routings active, tempo synced through a loop and a tempo change, feedback 80 % + freeze toggle, seeded, plus a different-seed control.
- [ ] **PERF-01:** an allocation gate over matrix route edits, source/destination swaps, sync-mode toggles and transport jumps.
- [ ] **QUAL-02 (partial):** a matrix stress render with all 16 slots at depth ±100 % onto the hottest destinations: no NaN/Inf/denormal, peak ≤ +6 dBFS.
- [ ] **Telemetry:** the destination atomics equal the engine's modulated values at the cell; the source atomics are in range.
- [ ] **CPU (INFO):** C5 = C4 + 16 slots active + 4 LFOs + env, against the 30 % budget.
- [ ] **D16 DAW check at verify** (2.5a items).

---

## Carry-ins

| Item | Handling |
|---|---|
| The matrix writes the reverb atomics (VERIFICATION-2.4) | Matrix component above. |
| QUAL-01 / PERF-01 / PERF-03 re-gate under modulation | Gate rows above. |
| W2o margin (0.017 dB) | The N2 neutral null keeps it untouched. If a row flips, widen the seeds. Never loosen the threshold. |
| Unity never derived from `get()` on skewed ranges | Matrix eval goes through `convertFrom0to1`. Gates read denormalised values. |
| `Engine.h` / `StreamScheduler.h` stage comments | Update them when this code is opened ("Tempo — 2.5a", "analysis — 2.5b"). |

## Execution Defaults (Claude's call, recorded for plan)

- The env follower (and, in 2.5b, the onset detector and MPM) analyses the **conditioned input**: post `input_gain`, the mono sum, pre-ring. It never sees the feedback return or the dry path.
- Source values are sampled at grid points only. Every source is a function of the absolute counter and the per-sample state before the grid point, so the design is block-size invariant by construction.
- Slot state (current source/dest, ramped depth, shadow route) lives in fixed `std::array`s. Route changes are detected by comparing the latched choice indices at ticks, with no listeners.
- Tempo-mode density multipliers are rounded in log2 space, with ties going up.
- The tempo-change re-time threshold is 0.5 sample. Telescoping jitter is preserved after a re-time (the epoch is unchanged and onsets are re-anchored to the grid).
- Reserved CounterRng slots: `LfoSH` per LFO, keyed on the cycle index. No existing draw slot moves (the N2 null depends on it).

## Constraints Identified

- No APVTS write-back. Listeners never write parameters.
- The destination table is append-only (stored choice index).
- No allocation on the audio thread. Every matrix and LFO structure is fixed-size and sized at compile time.
- There is no accumulated ppq anywhere. The D15 accumulator is a phase accumulator re-locked from ppq, not a ppq accumulator.
- Per-pass arrays stay at 32 samples, and the matrix is evaluated once per cell.

## Open Questions (for research)

- **TempoTracker port delta:** how GrainScatter handles the playhead being absent mid-session, `getPpqPosition` with no `getBpm`, and Logic pre-roll (negative ppq). Also its loop-wrap detection threshold.
- **Tempo re-time vs telescoping jitter:** confirm that re-anchoring keeps the mean rate exact and that jitter 0 is exactly on the grid after a locate.
- **D15 "rate modulated" predicate:** decide whether to key it on the ramped depth (a 20 ms tail after removal) or on slot occupancy. Then check that the handover is seamless when a route is removed mid-ramp.
- **Skewed-range destinations under bipolar depth:** the asymmetry in perceived swing, e.g. `sN_size` skew. Decide whether that is acceptable (ARCH says the skew is respected) or needs a note for the UI rings in 3.3.
- **Spawn-latched destinations at high LFO rates (20 Hz):** latch-at-spawn aliasing at low density. That is expected behaviour, but it needs documenting so QUAL-03 doesn't flag it.
- **Matrix cost:** 16 slots × per-cell evaluation plus `CellLerp` for the ramped destinations that actually have routes (sparse) vs all 113 (dense). Pick the sparse form if C5 needs it.
- **Choice-typed destinations:** whether any of the 113 are backed by Choice/Bool. If so, define the rounding.

## Next Phase

Ready for: research (`/plugin-research O-Granulator 2-dsp`, Phase 2.5a).
