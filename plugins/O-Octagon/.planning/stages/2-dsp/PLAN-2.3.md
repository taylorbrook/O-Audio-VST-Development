# Stage 2 — DSP · Phase 2.3 (Source Shaping and Outside-Hull Processing) — Plan

**Plugin:** O-Octagon
**Stage:** 2 of 4 — DSP
**Phase:** 2.3 of 3 — Source Shaping and Outside-Hull Processing
**GSD phase:** plan
**Date:** 2026-08-11
**Branch:** `feat/o-octagon` @ `a47cef88` (2.2 work uncommitted)
**Inputs:** `CONTEXT-2.3.md`, `RESEARCH-2.3.md`, `research/ARCHITECTURE.md` §3.3.2/§3.4/§3.5/§5/§OQ4,
`ROADMAP.md` Phase 2.3, `REQUIREMENTS.md`, `PLAN-2.2.md` (P14–P24 inherited)

---

## Entry Check — contract checksums

Re-computed at this boundary, not read out of `RESEARCH-2.3.md`
(`pattern_promotion_checksum_pins_replaced_file`). All four byte-exact against `STATUS.md`
frontmatter:

| Contract | SHA-256 | Result |
|---|---|---|
| `BRIEF.md` | `697a4f32…f6b9fbd6` | ✅ |
| `parameter-spec.md` | `b45f88dc…cbb9e02f` | ✅ |
| `research/ARCHITECTURE.md` | `a8a358f4…f0429b6d4408` | ✅ matches the **2.3 D2 re-pin** |
| `ROADMAP.md` | `aec7d0ce…0137ee29` | ✅ |

**No pin moves at 2.3.** Two superseded architecture hashes (`bff8a83b…`, `cd881a10…`) stay in
frontmatter and match nothing from here on. See **P36** for why H2's figure correction is an erratum
rather than a third re-pin.

**Marker state, verified rather than assumed:** `PHASE-2.3-WIDTH`, `PHASE-2.3-AIR` and
`PHASE-2.3-TRIM` each appear **exactly once** in `Source/`, all three in `GainStage.cpp`.
`PHASE-2.2-REPLACE` is **0**. **Probe inventory:** 29 unit + 17 harness = **46 (A–AT)**, reconciled
against 48 `check (` occurrences less the two `void check (…)` definitions.

---

## Goal

**The chain closes.** `width` goes live, the hull distance that `solveSubPoint` computes and throws
away starts driving a trim and a filter, and the per-speaker trims that have ridden in the snapshot
since 2.1 finally get multiplied in. At the end of this phase every one of `ARCHITECTURE §5` steps
1–7 is live and Stage 2 has no DSP requirement outstanding.

The functional diff is three marker sites plus a filter pair. **The work is in the measurement**, and
research already found the two places where the obvious measurement is vacuous (Q6: the 1 kHz
differential slew probe reads zero excess to nine decimals; H8: a `λ = 2, blur = 1` invariance probe
sits 0.9 % from a clamp) and the two places where the obvious *implementation* ships a defect (H1: a
click 5.2× the signal's own maximum slew; H5: a permanent-silence NaN latch armed by FUNC-07's
multiply).

### §5 step map — what this phase completes

| §5 step | 2.2 | 2.3 | Note |
|---|---|---|---|
| 1. Snapshot 17 atomics, dirty check vs generation | ✅ | — | unchanged |
| 2. Bbox-denormalise `srcX`/`srcY` → metres | ✅ | — | unchanged |
| 3. Sub-points, `rFade`, per-sub-point `earHeight` | ✅ (degenerate) | ✅ **live** | `PHASE-2.3-WIDTH` |
| 4. Hull inside/outside + projection | ✅ | ✅ **returns `d_hull`** | it was computed and discarded |
| 5. DBAP solve per sub-point | ✅ | — | unchanged |
| 6. Hull gain trim + air LPF | ❌ | ✅ **new** | `PHASE-2.3-AIR`, `hullproc` + 2 filters |
| 7. Fold venue trim, set 17 targets | partial | ✅ **complete** | `PHASE-2.3-TRIM` |
| Per-sample inner loop, exactly-once invariant | ✅ | ✅ + air | the skip sits **outside** the 17-smoother rule |
| Verify-ping override, metering | ❌ | ❌ **Stage 3** | — |

**All three markers must read ZERO in `Source/` at the end of this phase, and prose must not
resurrect them.** At 2.2 the first pass had each token appearing twice because comments quoted it.
When retiring a marker, delete the token — do not write "formerly PHASE-2.3-AIR".

---

## Requirement staging — read this before writing the verify report

`ROADMAP.md` Phase 2.3 assigns **FUNC-07, DSP-06, DSP-07, DSP-08, QUAL-01**, plus **QUAL-04
criterion 3** carried from 2.2. All four criteria sections that were missing at the 2.2 boundary were
written into `REQUIREMENTS.md` at 2.3 discuss, so this phase verifies against something.

| Req | Criterion | Closes at | Probe |
|---|---|---|---|
| **FUNC-07** | 1. −12 dB at speaker *i*, no change at the other seven | 2.3 | BF |
| | 2. +6 dB likewise; both ends reachable from the venue store | 2.3 | BF |
| | 3. Applied after the solve, folded into the smoothed targets | 2.3 | BF + BG |
| | 4. Venue-scoped: a musical preset load leaves all 8 trims bit-identical | 2.3 | BF |
| **DSP-06** | 1. `width = 0` → `v_L ≡ v_R` bitwise, **no branch** (regression gate) | 2.3 | AY |
| | 2. `width > 0` → spread ⟂ bearing **from the centroid**, R on audience right | 2.3 | AX |
| | 3. Per-sub-point `z` against the sloped plane at its **own** `y` | 2.3 | AX |
| | 4. Sweep through the centroid at `width = 6` → no discontinuity | 2.3 | BA |
| | 5. `(0,−1)` fallback at `\|b\| < 1e-6`, finite and symmetric | 2.3 | AX |
| **DSP-07** | 1. Trim linear in dB/m, floored at −24 dB, per sub-point, pre-smoothing | 2.3 | AV + BD |
| | 2. `hullAtten = 0` → bit-identical to the trim removed | 2.3 | AV + BD **(P33 method)** |
| | 3. No-op inside the hull regardless of setting | 2.3 | AV + BD |
| | 4. Cutoff matches the §3.5.2 table at {0.35, 1.0} × {5, 15} m | 2.3 | AU |
| | 5. `airAmount = 0` → bit-identical to the filter absent | 2.3 | BD **(P33 method)** |
| | 6. `d_hull = 0` → bit-identical at **any** `airAmount` — the D2 amendment | 2.3 | BD |
| | 7. `reset()` on the `airAmount → 0` transition, **not** on every `d_hull == 0` block | 2.3 | BD + BE |
| | 8. Non-finite sample recovers within one block | 2.3 | BE |
| **DSP-08** | 1–4. `r_s` mapping, **invariance** under λ, both caps, `blur = 0` finite | 2.3 | AW (implemented at 2.2) |
| **QUAL-01** | 1. Full-range sweeps of all 17 → no clicks/steps/level jumps | 2.3 | AS (11) + AZ (5) + BC (`airAmount`) |
| | 2. Rapid puck movement across the hull → no audible discontinuity | 2.3 | BB (DC + 1 kHz + 8 kHz) + **D5** |
| | *scope:* one representative live venue edit during playback (D4) | 2.3 | BG |
| **QUAL-04** | 3. `width` sweep → no zipper — **clears 2.2's PARTIAL** | 2.3 | AZ |

**Nothing carries past 2.3 except the three residuals named at discuss** — the CI gap (Stage 4),
`COMPAT-04`'s retroactive criteria (Stage 4), and `FUNC-06` / `UI-02..05` criteria (owed at Stage 3
discuss, before Stage 3 plan). **Stage 2 then closes with a stage-level `VERIFICATION.md`**; 2.1 and
2.2 wrote phase-suffixed ones only.

**Two criteria carry a method statement rather than a bare pass/fail, and the plan says so up front
so verify discovers nothing (P33, P34):**

- **DSP-07/2 and /5** — there is no "filter absent" or "trim absent" build to compare against, and
  fabricating one would be a second arithmetic path, which is the class of thing §3.4.3 forbids.
  Bit-transparency is proven **structurally**: a branch counted as never taken, and a multiply by a
  value asserted bit-equal to `1.0f`. Each half carries a non-vacuity control showing the stage
  *does* change the output when enabled. SUMMARY-2.3 must state this, not "bit-transparency ✓".
- **QUAL-01/2** — the accepted cost of D2 is a one-sample step, and its size is `A·|H_20k(f) − 1|`,
  not the magnitude-only figure the architecture quotes (H2). Measurement bounds it; only **D5's
  listening session** settles whether "no *audible* discontinuity" is met on HF-rich material.

---

## Plan Decisions

Continuing the P-series (Stage 1: P1–P4; 2.1: P5–P13; 2.2: P14–P24). Each of RESEARCH-2.3's five open
items is resolved below, plus seven decisions the research findings forced.

### P25 — `hullproc` is a **header-only** `Source/DSP/HullProcessor.h` *(D1, research item 5 — file shape)*

Two `inline` free functions in `namespace hullproc`, no state, no class. The `VenueGeometry.h`
precedent (P14) applied a second time: trivial stateless free functions do not earn a `.cpp`, and
header-only means **zero CMake churn** — nothing to add to `target_sources` in three CMakeLists.

```cpp
namespace hullproc
{
    inline constexpr float kTrimFloorDb   = -24.0f;   // §3.5.1
    inline constexpr float kAirRefMetres  =   3.0f;   // §3.5.2 dRef
    inline constexpr float kAirCeilingHz  = 20000.0f;
    inline constexpr float kAirFloorHz    =   500.0f;
    inline constexpr float kNyquistMargin =   0.45f;  // H4

    /** §3.5.1. Returns EXACTLY 1.0f when hullAtten·dHull == 0 — see P33. */
    inline float hullTrimGain (float hullAtten, float dHull) noexcept;

    /** §3.5.2, with H4's Nyquist-safe bounds. */
    inline float airCutoffHz (float airAmount, float dHull, double sampleRate) noexcept;
}
```

**D1's constraint is "no `juce_dsp`, no `juce_audio_processors`", and that is what is preserved.**
`hullTrimGain` uses `juce::Decibels::decibelsToGain` from `juce_audio_basics`, which the unit target
**already links** (`tests/unit/CMakeLists.txt:93`) and which is the *same function* `VenueModel::trimLin`
uses — so the two conversions in FUNC-07's product line are one function, not two, and Q5's
bit-transparency proof (which was written against `decibelsToGain`) transfers verbatim. Writing
`std::pow (10.0f, dB * 0.05f)` by hand to satisfy a literal reading of "JUCE-free" would fork the
conversion for no gain.

**Deviation from `ROADMAP.md`, recorded not slipped:** ROADMAP names `HullProcessor.{h,cpp}`. This is
a file-shape deviation of the same class as P14 and goes in `SUMMARY-2.3.md`.

### P26 — Keep `juce::dsp::FirstOrderTPTFilter<float>`. Do **not** hand-roll `hullproc::OnePoleTPT` *(research item 5 — the open call)*

RESEARCH offered no recommendation and both answers were defensible. The call is **keep the JUCE
class**, for three reasons in descending weight:

1. **The one unique advantage of a hand-rolled struct is already neutralised.** Its selling point was
   direct state inspection for DSP-07/8 — but **Q1 proved from source that the output check is exactly
   equivalent and *immediate***, not one sample late: `y = G·x + (1−G)·s` is affine in `s` on the same
   sample. An equivalence that is proven is not worth a new numeric surface.
2. **The "move DSP-07 into the fast unit target" win is smaller than it looks.** The criteria that are
   pure arithmetic — the cutoff curve (4), the trim law (1, 2, 3) — are *already* in `hullproc` free
   functions under P25 and run in the fast target regardless. Every remaining filter criterion (5, 6,
   7, 8) is about the filter **as `GainStage` drives it**, and `GainStage.cpp` is deliberately absent
   from the unit target and will stay absent. Those probes land in the render harness either way.
3. **§3.5.2 names the class.** Deviating from a named architecture choice needs to buy something, and
   after (1) and (2) it buys a Debug `jassert` we must design around anyway.

**Consequence, and it is not optional: H4 and H6 become mandatory tasks, not nice-to-haves.** Keeping
the JUCE class means inheriting its Nyquist `jassert` (H4) and its `s1 { 2 }` initializer-list quirk
(H6). Both are handled — P28 and P30 respectively.

### P27 — Adopt H1: `reset (x)` on the `airActive` false→true edge *(RESEARCH item 1)*

D2 considered two re-entry policies and picked the worse one. H1's third option is strictly better
than both and satisfies D2's own stated rationale more completely than D2's own choice:

| policy | 1 kHz step at re-entry | vs. the signal's own max per-sample slew |
|---|---|---|
| resident (D2 as written) | 3.407e-01 | **520.9 %** |
| `reset (0)` (original §3.5.2) | 2.810e-01 | 429.6 % |
| **`reset (x)`** | **0** | **0.0 %** |

`processSample` computes `v = G·(x − s)`; seeding `s = x` makes `v` exactly `0.0f` and `y = 0.0f + x`,
so `y == x` **bit-exactly** on every toolchain, at every cutoff, at every entry speed. (The algebraic
form `G·x + (1−G)·x` *would* round; the code's form does not.) D2 objected to *discarding* continuity
by re-zeroing — `reset (x)` discards nothing, it seeds the state with the value the filter would have
converged to, so the oscillating-puck hazard D2 named disappears rather than being traded away.

**Third payoff, free:** it closes Q1's blind spot. A NaN parked in a filter that is currently
*skipped* is invisible to a per-block output check, and under D2 a filter can be skipped
indefinitely. An unconditional re-seed at the edge overwrites it before it can be used.

**Implementation shape, and the two traps:**

- The seed is **hoisted out of the per-sample loop**. `renderChunk` is called immediately after the
  control boundary that set the flag, so the first sample of the chunk *is* the edge sample: seed
  from `0.5f * in0[start]` (matching the `sL` expression exactly, including the `nullptr → 0.0f`
  case) before the loop. No per-sample branch is added.
- **The pending flag is consumed in REAL mode only and left pending in SAFE mode** (H10). In SAFE
  mode the filter is not applied at all, so consuming the flag there would leave a mode flip
  mid-render with a stale pending reset — exactly the F3 window `GainStage.cpp:252-256` already warns
  about.

Recorded as a deviation in `SUMMARY-2.3.md`. It is an implementation choice **inside** §3.5.2's
stated intent, so no checksum moves (contrast: D2 itself, which changed the specification).

### P28 — Nyquist-safe cutoff bounds, and `prepareToPlay` re-instates its second parameter *(H4, Q4)*

`setCutoffFrequency` asserts `isPositiveAndBelow (fc, sampleRate * 0.5)`, and past Nyquist
`tan (π·fc/fs)` goes **negative**, so `G = g/(1+g)` is negative or singular and the one-pole is not a
lowpass at all. A Debug crash and a Release nonsense at 22.05 and 32 kHz — rates hosts do offer and
pluginval at strictness 10 exercises.

```
ceiling = min (kAirCeilingHz, kNyquistMargin · fs)     // 0.45·fs
floor   = min (kAirFloorHz,   ceiling)                 // so the clamp cannot invert
fc      = clamp (kAirCeilingHz · exp2 (−airAmount · dHull / kAirRefMetres), floor, ceiling)
```

| fs | ceiling |
|---|---|
| 22 050 | 9 922.5 |
| 32 000 | 14 400.0 |
| 44 100 | 19 845.0 |
| **48 000** | **20 000.0** |

At 44.1 kHz this clips 20 000 → 19 845 Hz: a 0.07 dB change at 10 kHz, inaudible, and it makes the
plugin correct everywhere rather than correct above 40 kHz. Note the *numerator* stays the literal
`kAirCeilingHz` — the musical curve is anchored at 20 kHz (§3.5.2's four-row table re-derives exactly:
13 348 / 5 946 / 6 300 / 625 Hz) and only the clamp is rate-aware.

**`GainStage` therefore needs the sample rate as a member**, set in `prepare` before step 3's
`updateControl`. And per Q4, `OOctagonProcessor::prepareToPlay (double, int)` **re-instates its second
parameter name and forwards it**: `FirstOrderTPTFilter::prepare` takes a `juce::dsp::ProcessSpec`, and
fabricating a `maximumBlockSize` is a small lie in a codebase that has been careful not to tell any.
The `-Wunused-parameter` comment at `PluginProcessor.cpp:191-193` then simply goes away because the
parameter is used. `GainStage::prepare` gains an `int samplesPerBlock` argument.

### P29 — Sanitise at `publishSnapshot()` *(H5 — a defect this phase would otherwise create)*

The 17 musical parameters are sanitised at ingestion (P17). **The 42 venue values are not** — there is
no `jlimit`, no `clamp` and no `isfinite` anywhere in `VenueModel.cpp`. That is latent today because
`trimLin` is carried and never used. **FUNC-07's multiply is what arms it:**

```
trimDb = 1e30  →  decibelsToGain → +inf  →  trimLin = inf
v_i = 0.0f     (exactly, whenever w_i == 0 — DSP-05 criterion 1)
0.0f * inf     =  NaN  →  setTargetValue (NaN)  →  SmoothedValue latches  →  PERMANENT SILENCE
```

That is RESEARCH-2.2's H2 latch reached through a new door, and the door is reachable: venue values
come from `setStateInformation` (host session data) and, from Stage 3.2, from a UI where a user types
coordinates. `NaN` itself happens to be benign (`NaN > -100.0f` is false, so `decibelsToGain` returns
`0.0f`) — **do not rely on that**; it is one refactor from changing.

**`publishSnapshot()` is the single funnel for everything the audio thread ever reads about the
room**, which makes it the exact analogue of `snapshotParameters()`: one site, stated once,
structurally impossible to bypass.

- **`trimDb` clamped to ±24 dB before conversion.** In scope without argument — the multiply is what
  makes it exploitable. ±24 dB matches the hull trim's own −24 dB floor and comfortably contains
  FUNC-07's criteria (−12 dB and +6 dB must both be reachable).
- **Every other float `isfinite`-guarded to its §OQ4 default** — positions, rake, bbox, centroid,
  `rigScale`, `hullEpsCross`. This closes a *pre-existing* 2.2 hazard: a NaN speaker coordinate
  already reaches `dbap::solve`, where `dRaw < kMinDistance` is false for NaN and
  `denom < kDenomEpsilon` is false for NaN, so it falls straight through to `setTargetValue`. Same
  one-line loop, same site, so folding it in costs nothing — but it **is** a scope addition and goes
  in `SUMMARY-2.3.md` as a recorded deviation, not slipped in.

### P30 — Filter `prepare()` goes in **step 2** of `GainStage::prepare` *(Q4, H6)*

P23's rule is that `SmoothedValue::reset()` is a **state teleport** with exactly one call site.
`filter.prepare()` performs the analogous teleport (it does `sampleRate = …; s1.resize (n); update();
reset();`). Putting it beside the seventeen `s.reset (sampleRate, 0.005)` calls makes the rule read
**"step 2 is the only place any DSP state is initialised, ever"** — one site, one step, stated once.

Two things this settles:

- **`prepare()` is mandatory, not advisable (H6).** `std::vector<SampleType> s1 { 2 }` is the
  *initializer-list* constructor: the default vector has **size 1 holding the value 2.0f**. An
  unprepared filter decays from 2.0 on its first outputs, and `processSample (1, …)` on one is out of
  bounds. Step 2's `prepare()` resizes and zeroes it.
- **Two mono instances, each prepared with `numChannels = 1` — mandatory, not stylistic (H6).** `G` is
  a **per-filter** member, not per-channel. One 2-channel instance would carry two states but **one
  shared cutoff**, and the two sub-points have different `d_hull` whenever `width > 0` and the source
  straddles the hull. That is silently wrong in exactly the configuration Q9 asks about.

`s1.resize()` can allocate, but does not here (size 1 → `numChannels` 1) and `prepareToPlay` is not a
real-time context anyway; probe AO arms its counter *after* `negotiate()`, so filter preparation is
outside the measured window. **No task is spent on this** — recorded so the executor does not spend
one.

### P31 — The DSP-07 NaN guard is written against the filter **output**, once per block *(Q1)*

`s1` is private with no accessor (`juce_FirstOrderTPTFilter.h:149`). The output check is equivalent
and immediate (Q1), and the state is **sticky by construction** — `s = y + v` re-derives `s` from a
value that is already NaN, so once poisoned it never recovers
(`pattern_envelope_follower_state_sticky_nan`).

**`reset()` alone fully restores the filter, and the header comment must say why**:
`pattern_biquad_nan_guard_sticky_silence` does **not** apply verbatim here, because `G` is recomputed
from `cutoffFrequency` at every control block and is never derived from the state. There is no
last-known-good coefficient to preserve. Without that note, the next reader adds coefficient
preservation that does nothing and looks prudent.

**This `reset()` is a second call site by necessity and that is fine** — it is a *recovery* path, not
an initialisation path. Label it as such in the header, next to P30's rule, or someone will "fix" it.

### P32 — Two new instrumentation counters. Neither routes through `countedPow` *(Q2)*

`instr::` gains `airCutoffUpdates` and `airSamplesFiltered` (counters 5 and 6), both added to
`resetCounters()`.

- **`airCutoffUpdates == solveRuns · 2`** is the executable form of *"`setCutoffFrequency` stays out
  of the per-sample loop"* (Q2). The number that can silently regress is not the `tan` count — it is
  the placement, and this asserts the placement. PERF-01 stays a number rather than an argument.
- **`airSamplesFiltered`** is what makes P33's structural bit-transparency proof measurable: `== 0`
  when the stage is defeated, `== samplesRendered · 2` when it is active.

**Do not route the new `pow`/`exp2` calls through `countedPow`.** Probe AE asserts `powCalls == 16`
*exactly* and that assertion is load-bearing for §3.3.5's budget. `juce::Decibels::decibelsToGain`
calls `std::pow` directly and is **not** counted (`juce_Decibels.h:54-59`), so the existing gate is
unaffected — confirm this at execute rather than assume it, because it is exactly the kind of thing
that turns a green gate red for a reason unrelated to the change.

Cost added, all at control rate, none per sample: 2 `tan` + 2 `exp2` + 2 `pow` per control block =
**4 500 transcendental calls/second at 48 kHz** on top of 12 750. About 35 % more control-rate work;
**PERF-01's per-sample budget is untouched.**

### P33 — DSP-07/2 and /5 are proven **structurally**, with a non-vacuity control on each half *(Q5)*

There is no "filter absent" build and no "trim absent" build. Building one would mean a second
arithmetic path selected by a compile flag — the thing §3.4.3 forbids, and a path the shipping binary
would never take. So the claim is made the honest way:

| claim | proof | non-vacuity control |
|---|---|---|
| `hullAtten = 0` → trim is a no-op | `bitExact (hullTrimGain (0, d), 1.0f)` over a swept `d` (AV) + `v * 1.0f == v` for every finite `v` | `hullAtten = 0.5` outside the hull **changes** the render (BD) |
| `airAmount = 0` → filter is absent | `airSamplesFiltered == 0` — the branch is *counted* as never taken (BD) | `airAmount = 0.35` outside the hull **changes** the render, counter `== samples·2` (BD) |
| `d_hull = 0` → filter is absent at any `airAmount` | same counter, plus renders at `airAmount ∈ {0, 0.35, 1.0}` **bit-identical** inside the hull (BD) | the same three settings **differ** outside the hull |

`std::pow (10.0f, -0.0f)` is **exactly `1.0f`** — C99 Annex F / IEEE 754 mandate `pow (x, ±0) == 1`,
and `-0.0f > -100.0f` is true so `decibelsToGain` takes the `pow` branch. No `-ffast-math` anywhere on
the line (`juce_recommended_config_flags` adds `-O3` in Release and `-g -O0` in Debug, nothing else),
so IEEE semantics hold and this is a guarantee rather than a rounding accident.

**One precondition, stated in the probe's comment rather than left implicit.** `processBlock` runs
under `juce::ScopedNoDenormals`, so if a solved `v_i` were **denormal** (< 1.18e-38), `v_i * 1.0f`
would flush to zero where 2.2 stored the denormal. Reaching one requires a weight around 1e-38 —
unreachable from the exposed 0–1 range with any non-degenerate geometry, and the probe uses default
weights. An unstated precondition is how a bit-identity claim quietly becomes false.

### P34 — QUAL-01's two criteria, with derived bounds rather than tuned tolerances *(Q6, Q7, H2, H9)*

**D3's method as written does not work for `airAmount`, and the reason is not a tolerance problem.**
At 1 kHz the excess slew of a swept render over a held one is **+0.00000 %** — identically zero to
nine decimals, because the render's max slew is set by the sine's own zero crossing, which occurs
early where both renders still share a cutoff. The probe would pass for a reason unrelated to the
code.

**Criterion 1 (`airAmount`) — the two-render differential (Q6 option 1).** Render the same sweep with
the cutoff updated on the 64-sample control grid and again on a 4096-sample grid, **subtract**, and
the sine cancels to the precision of the two gain trajectories. Assert

```
max |Δy|  ≤  max |ΔG| · 2 · peak
```

with `max |ΔG|` computed **in-probe from the sweep schedule the probe itself drives** (Q3 gives the
step in closed form: `Δy = (G_new − G_old)·(x − s)`). **There is no tolerance to pick and nothing
mirrors a constant** (`pattern_test_fixture_mirrors_drift_silently`). The 4096-sample negative control
exceeds the 64-sample bound by **46–50×**, not by a hair.

**Criterion 2 (hull crossing) — the bound is `A·|H_20k(f) − 1|`, predicted and asserted.** Q7's answer
flips the intuition: 1 kHz *is* sensitive enough, but not for the reason D3 assumed. The step is the
complex `H(f) − 1`, and the **phase lag dominates** below ~15 kHz:

| tone (fs = 48 kHz) | magnitude-only | **full \|H − 1\|** | phase term dominates by |
|---|---|---|---|
| 1 kHz | 1.542e-04 | **1.756e-02** | **114×** |
| 8 kHz | 1.176e-02 | **1.529e-01** | 13.0× |

At amplitude 0.5 the 1 kHz step is a one-sample jump of 8.8e-3 against a natural slew cap of 6.5e-2 —
**13 % of the signal's fastest legitimate move**, comfortably measurable. Asserting the *predicted*
number rather than "under some bound" makes the probe fail loudly if P27's `reset (x)` is ever
dropped, because the resident-state edge is ~20× larger at 1 kHz.

**Three rules that apply to every filter-path probe:**

1. Both excitations, always: **DC for the gain vector, sine for the filter.** "0 dB at DC" is still
   true — the phase lag also goes to zero at DC — so a DC hull-crossing probe is *exactly blind* to
   the one discontinuity D2 introduced.
2. **Two tones on criterion 2:** 1 kHz and 8 kHz. 8 kHz is where the cost is large enough to matter
   musically and is the tone D5's listening check should use.
3. **Discard a ≥ 2000-sample lead-in (H9).** A filter starting from `s = 0` produces a step at n = 2
   *larger than the signal's steady-state maximum slew* (0.067264 vs 0.065304). This produced a 3 %
   over-reading during research before it was traced. Probe AS is unaffected (DC input); every new
   2.3 filter-path probe is affected.

### P35 — DSP-08's invariance probe: λ ∈ {0.5, 2.0} at `blur = 0.25`, scaling the **source** too *(H8)*

The invariance is exact, not approximate: `v_i` is homogeneous of degree 0 in λ. **The only
scale-breaking terms are the two clamps**, and the margin on `kMaxBlurMetres = 8.0` is thin:

| λ | blur | `r_s` wanted | `r_s` actual | |
|---|---|---|---|---|
| 2.0 | 1.00 | 7.932 m | 7.932 m | ok **by 0.9 %** |
| 2.1 | 1.00 | 8.328 m | 8.000 m | **clamped — invariance breaks** |
| 2.0 | 0.25 | 1.983 m | 1.983 m | ok, 4× margin |

A probe written as "λ = 2, blur = 1" passes today and fails the moment `rigScale` moves — and
`rigScale` has already been corrected twice (7.95 → 7.93165). `blur = 0.25` leaves a 4× margin.
**Assert the invariance, never the constant.** And **scale the source position as well as the
speakers**: scaling only the speakers changes the geometry rather than its scale, and the probe would
fail against a correct implementation.

The probe also carries a **positive control**: at `blur = 1, λ = 2.1` the invariance **must break**,
which proves the probe can see the clamp at all.

### P36 — H2's figure correction is an **erratum**, not a third re-pin *(research item 4)*

§3.5.2's amendment paragraph and `REQUIREMENTS.md`'s QUAL-01 scope note both quote the accepted cost
as *"3 dB @ 20 kHz, 0.7 dB @ 10 kHz, 0 dB at DC"*. The 20 kHz figure is right (−3.0103 dB **is** the
cutoff definition). **The 10 kHz figure is the analog one-pole's −0.969 dB**; the digital filter at
`fc = 20 kHz` and fs = 48 kHz is **−0.1798 dB** there, far flatter, because bilinear prewarping
compresses the passband when `fc` sits at 0.83 × Nyquist. The error is in the *conservative*
direction, so **D2's decision stands a fortiori** — and separately the whole figure is
magnitude-only, understating the real step by 5–190× (H2, P34).

- **`ARCHITECTURE.md` is not re-edited and not re-pinned.** The figure is descriptive prose about a
  cost, not a specification the code implements, and re-pinning a contract mid-phase is the exact
  thing the discipline exists to avoid. The pin has already moved twice; a third move for a prose
  correction devalues the mechanism.
- **`REQUIREMENTS.md` is corrected in place, at plan** (Task 1). It is not checksummed, and its
  QUAL-01 note currently prescribes a **measurement method that Q6 proved vacuous**. Leaving that
  standing until verify would put the plan's probes in direct conflict with the requirement document.
  Correcting a *method* is not the same act as ticking a *verdict* — verdicts still land at verify.
- **`SUMMARY-2.3.md` records the erratum** with both the corrected response table and the reason the
  original figure was wrong.

---

## Tasks

### Task 1 — `REQUIREMENTS.md` method corrections *(P34, P36)*

Do this **first**, so nothing downstream contradicts the requirement document.

- QUAL-01's scope note: replace *"3 dB @ 20 kHz, 0.7 dB @ 10 kHz, 0 dB at DC"* with the corrected
  exact-response table (H2) and the `|H − 1|` framing. Keep "0 dB at DC" — it is true and it is why
  the DC probe is blind.
- QUAL-01's `airAmount` method: replace the `max |out[n] − out[n−1]|` differential-against-held
  construction with P34's **two-render differential** and its derived bound. State plainly that the
  original method was measured at **+0.00000 % excess** and would have passed vacuously.
- QUAL-01 criterion 2's method: DC **and** sine at 1 kHz **and** 8 kHz; bound `A·|H_20k(f) − 1|`;
  entry edge asserted **bit-exact** under P27.
- Add a one-line note under DSP-07 criteria 2 and 5 pointing at P33's structural method.

**No verdicts, no tick marks** — those land at verify, as at 2.2.

**Files:** `.planning/REQUIREMENTS.md`
**Depends on:** nothing

---

### Task 2 — `Source/DSP/HullProcessor.h` *(P25, P28)*

Header-only, `inline`, `namespace hullproc`. Includes: `<cmath>` and
`<juce_audio_basics/juce_audio_basics.h>` (for `juce::Decibels`) — **not** `juce_dsp`.

- The five constants above, each with a `static_assert` where it carries a claim
  (`kNyquistMargin < 0.5f`; `kAirFloorHz < kAirCeilingHz`).
- `hullTrimGain (hullAtten, dHull)` = `decibelsToGain (max (−hullAtten·dHull, kTrimFloorDb))`.
  Comment must record the P33 claim: at `hullAtten·dHull == 0` the argument is `-0.0f`,
  `-0.0f > -100.0f` is true, and `pow (10, -0.0f)` is **exactly** `1.0f`.
- `airCutoffHz (airAmount, dHull, sampleRate)` per P28 — ceiling, floor derived from the ceiling, then
  clamp. `std::exp2`, not `std::pow`.
- Comment the H4 finding at the ceiling line: past Nyquist `tan` goes negative and the one-pole stops
  being a lowpass, so this is correctness, not defensiveness.

**Files:** `Source/DSP/HullProcessor.h` (new)
**Depends on:** nothing

---

### Task 3 — Two instrumentation counters *(P32)*

`airCutoffUpdates` and `airSamplesFiltered` in the `#if OOCTAGON_INSTRUMENT` block of
`DbapSolver.h`, plus `countAirCutoffUpdate()` / `countAirSampleFiltered()` unconditional helpers, plus
both added to `resetCounters()`. Same shape as the existing four.

Comment on `airCutoffUpdates`: it asserts **placement**, not cost — `== solveRuns · 2` is the
executable form of "not called per sample".

**Files:** `Source/DSP/DbapSolver.h`
**Depends on:** nothing

---

### Task 4 — `solveSubPoint` returns `d_hull` *(constraint 2)*

Currently `projection.distance` is computed inside the outside-hull branch and discarded at a
documented line. Return it.

- Signature returns `float`. **`0.0f` on the inside path — returned explicitly, never left
  uninitialised.**
- Delete the "deliberately NOT consumed at Phase 2.2" comment block **including its reference to the
  marker tokens**. The tokens themselves are retired in Task 5; this comment must not outlive them.
- `hull::project().distance` is always ≥ 0, always finite, and `0.0f` on every degenerate count
  (`ConvexHull2D.cpp:86-119`) — nothing new to compute and no guard to add.

**Files:** `Source/DSP/GainStage.cpp`
**Depends on:** nothing

---

### Task 5 — `GainStage`: the three markers retire *(P26, P27, P28, P30, P31, P32)*

The core of the phase. Every sub-item below is a line or two; the care is in the ordering and the
comments.

**`GainStage.h`:**
- `#include <juce_dsp/juce_dsp.h>` (H7 — the link is already there in all three targets; **the include
  is what is missing**). Verified safe: nothing the unit target compiles includes `GainStage.h`.
- `#include "HullProcessor.h"`.
- Two `juce::dsp::FirstOrderTPTFilter<float> airL, airR;` beside the smoothers. **Two mono instances**
  — P30 says why one 2-channel instance is silently wrong.
- `double sampleRate { 0.0 };` member (P28), `bool airActiveL/R { false };`, `bool airSeedPendingL/R
  { false };`.
- `prepare()` gains `int samplesPerBlock`. Update the doc comment's four-step list to name filter
  preparation in step 2, and add P31's note that the NaN guard's `reset()` is a **recovery** site by
  necessity — labelled, so the single-reset-site rule is not read as broken.

**`GainStage::prepare` — step 2 only** (P30): store `sampleRate`; `airL.prepare ({ sampleRate,
(uint32) samplesPerBlock, 1 })` and the same for `airR`. Nothing in steps 1, 3 or 4 changes.

**`GainStage::updateControl`:**
- **`PHASE-2.3-WIDTH` retires** — `const float widthMetres = p[params::width];`. Delete the marker and
  the whole degeneracy comment; keep §3.4.3's "both solves run unconditionally" comment, which is
  still load-bearing (constraint 4).
- Capture `dHullL` / `dHullR` from Task 4's return values.
- **`PHASE-2.3-AIR` retires** — §5 step 6, per sub-point:
  - `const float trimL = hullproc::hullTrimGain (p[params::hullAtten], dHullL);` then
    `for (i) vL[i] *= trimL;`. §5 step 6 folds the trim into `v` **before** step 7, and doing it here
    rather than at the target line is what makes DSP-07/1's "folded into that sub-point's gain vector"
    literally true.
  - `airL.setCutoffFrequency (hullproc::airCutoffHz (p[params::airAmount], dHullL, sampleRate));` —
    **unconditionally**, then `instr::countAirCutoffUpdate();`. Setting the cutoff on a skipped filter
    is free and keeps the counter identity `== solveRuns · 2` exact.
  - `const bool wasActive = airActiveL; airActiveL = (p[params::airAmount] > 0.0f && dHullL > 0.0f);
    if (airActiveL && ! wasActive) airSeedPendingL = true;` — P27's edge.
  - **`reset()` on the `airAmount → 0` transition only** (DSP-07/7) — *not* on a `d_hull == 0` block.
    Comment the distinction; it is the single most likely thing to be "simplified" later.
- **`PHASE-2.3-TRIM` retires** — `gL[i].setTargetValue (vL[i] * snapshot.trimLin[i]);` and the same
  for R. Delete the marker comment entirely.

**`GainStage::renderChunk`, REAL branch only:**
- Before the loop: consume the pending seeds — `if (airSeedPendingL) { airL.reset (in0 != nullptr ?
  0.5f * in0[start] : 0.0f); airSeedPendingL = false; }`, same for R with `in1`. The expression must
  match the `sL`/`sR` expression **exactly**, `nullptr` case included, or P27's bit-exactness claim
  is false at the one edge it exists to protect.
- In the loop, after `sL`/`sR` are read and **before** the gain matrix:
  `const float fL = airActiveL ? airL.processSample (0, sL) : sL;` (same for R), and
  `if (airActiveL) instr::countAirSampleFiltered();`. **H7's aliasing rule is unchanged** — the filter
  inserts after the read of `in0[n]`, so no read pointer is hoisted and no output is written before
  the read.
- Use `fL`/`fR` in the gain matrix. **All 17 `getNextValue()` calls stay exactly once, unconditional,
  in both modes** — the air skip gates `processSample`, nothing else (constraint 6, H10).
- **After the loop, once per block:** the P31 NaN guard. Track the block's max `|fL|`/`|fR|` (or check
  the last output of each filter) with `std::isfinite`; on failure `reset()` that filter. Header
  comment records that `G` is recomputed every control block so no coefficient preservation is needed.

**`GainStage::renderChunk`, SAFE branch:** unchanged except that the pending seed flags are **not**
consumed (H10). Add the one-line comment saying so and naming the F3 mode-flip window.

**Files:** `Source/DSP/GainStage.h`, `Source/DSP/GainStage.cpp`
**Depends on:** Tasks 2, 3, 4

---

### Task 6 — `PluginProcessor`: `ProcessSpec` forwarding and `publishSnapshot()` sanitisation *(P28, P29)*

- `prepareToPlay (double sampleRate, int samplesPerBlock)` — re-instate the parameter name, delete the
  `-Wunused-parameter` explanation comment, forward it to `gainStage.prepare (...)`. Order is
  otherwise unchanged: `readVenueFromState()`, `rebuildChannelMap()`, `preparedYet = true`, then the
  gain stage **last** (P23).
- `publishSnapshot()` — P29. A local `sane (float v, float fallback)` helper
  (`std::isfinite (v) ? v : fallback`), applied to every float copied in; `trimDb` additionally
  `jlimit (-24.0f, 24.0f, …)` **before** `decibelsToGain`. Fallbacks come from §OQ4 / `VenueModel.cpp:166`.
- Comment must state the P29 scope split explicitly: the **trim** guard is 2.3's (FUNC-07's multiply
  arms it), the **position** guard closes a pre-existing 2.2 hazard folded in at the same site.

**Files:** `Source/PluginProcessor.h`, `Source/PluginProcessor.cpp`
**Depends on:** Task 5

---

### Task 7 — Unit-target probes **AU–AX** *(fast target; `hullproc` + `shaper` arithmetic)*

29 existing probes must still pass alongside these four.

**AU — `air-cutoff-curve-and-nyquist-bounds`** (DSP-07/4, H4)
- §3.5.2's four-row table at fs = 48 kHz: `airAmount` ∈ {0.35, 1.0} × `d_hull` ∈ {5, 15} m →
  13 348 / 5 946 / 6 300 / 625 Hz, to a **stated** tolerance (1 Hz is ample; the arithmetic is exact).
- The H4 ceiling table at fs ∈ {22 050, 32 000, 44 100, 48 000} → 9 922.5 / 14 400 / 19 845 / 20 000.
- **`fc < fs/2` at every rate for every `(airAmount, d_hull)` in range** — the assertion JUCE would
  have made, made here instead where it also holds in Release.
- The floor binds only at extremes: at `airAmount = 1.0` it is reached at `d_hull = 15.97 m`. Assert
  the floor **and** that it never inverts the ceiling at low rates.

**AV — `hull-trim-law-and-unity`** (DSP-07/1, /2, /3)
- Linear in dB per metre over a swept `d_hull`, floored at exactly −24 dB.
- **`bitExact (hullTrimGain (0.0f, d), 1.0f)`** over the whole sweep — P33's first half, `memcmp`, not
  `near()`.
- `hullTrimGain (hullAtten, 0.0f)` is bit-exactly `1.0f` for every `hullAtten` in range — DSP-07/3.

**AW — `blur-invariance-under-room-scale`** (DSP-08, P35)
- λ ∈ {0.5, 2.0} at `blur = 0.25`, scaling **speakers and source** about the centroid; gain vector
  invariant to bit-tight tolerance.
- **Positive control:** `blur = 1, λ = 2.1` → invariance **must break** (the `kMaxBlurMetres` clamp).
  A probe that cannot see the clamp cannot claim the invariance is a property of the code.
- `blur = 0` with the source at a speaker's floor coordinate → finite, non-degenerate (DSP-08/4).
- Both caps `static_assert`ed (DSP-08/3) — confirm the 2.2 asserts are present rather than adding
  duplicates.

**AX — `sub-point-geometry-live`** (DSP-06/2, /3, /5)
- Handedness: puck downstage of the centroid, `width > 0` → **R on the audience's right** (larger `x`).
  Asserted, not reasoned.
- `z_L ≠ z_R` whenever the spread has a `y` component and `rakeFront ≠ rakeRear`; equal when the rake
  is flat. Both halves, or the probe passes on a bug that ignores `y`.
- `|b| < 1e-6` → the `(0, −1)` fallback: finite, symmetric about the puck, spread along the room's
  left–right axis.
- The `rFade` collapse as an arithmetic property: `wEff → 0` as `|b| → 0`, monotone, reaching exactly
  `width` at `|b| ≥ rFade`. (The *continuity* half is BA, in the harness.)

**Files:** `tests/unit/main.cpp`
**Depends on:** Tasks 2, 5

---

### Task 8 — Render-harness probes **AY–BJ** *(integration)*

17 existing probes must still pass. Every filter-path probe **discards a ≥ 2000-sample lead-in** (H9)
and states the discard in its detail string.

**AY — `width-zero-is-bit-identical`** (DSP-06/1 — the regression gate)
`width = 0`, settle 240 samples, read `currentSmoothedValues()`: `bitExact (gL[i], gR[i])` for all 8.
**Negative control in the same probe:** off-centre puck at `width = 4` → the eight pairs must
**differ**. This is the criterion most likely to break and least likely to be noticed; `width`'s
default is `0.0`, so it is also the default-patch gate.

**AZ — `no-zipper-on-remaining-parameters`** (QUAL-04/3, QUAL-01/1)
AS's proven DC construction applied to the five parameters AS does not cover: **`width`** (clears
2.2's PARTIAL), `rolloff`, `blur`, `hullAtten`, `outputGain`. Same bound (5 ms smoother max
per-sample delta, 1/240 + 1e-6 at 48 kHz), same negative control (sample the series at 64-sample
control boundaries — must exceed the bound). With AS's eleven and BC's `airAmount`, **all 17 are
covered; state the 11 + 5 + 1 arithmetic in the detail string.**

**BA — `centroid-crossing-at-width-6`** (DSP-06/4, risk R5)
Puck swept through the rig centroid at `width = 6`, DC excitation, step bound = the smoother's max
per-sample delta. This is the gate on the design-time `rFade` fix; without it §3.4.2 is an argument.

**BB — `hull-crossing-continuity`** (QUAL-01/2, P27, P34)
- **DC excitation** → the gain vector's step, bounded by the smoother delta.
- **Sine at 1 kHz and 8 kHz**, bound `A·|H_20k(f) − 1|` computed in-probe from the transfer function
  at the actual `fs`: 1.756e-2 and 1.529e-1 per unit amplitude at 48 kHz.
- **Entry edge (inside → outside) asserts bit-exact continuity** — P27's `reset (x)` makes `y == x`
  by the arithmetic. This is the probe that fails loudly if the seed is ever dropped.
- **Exit edge (outside → inside) asserts ≤ the predicted bound.** H3: the exit step is inherent to D2
  and cannot be removed by any state manipulation — there is no state on the dry path to seed. Report
  the measured value; it is the number D5 listens to.
- Q9 is **not** probed empirically: straddling sub-points are provably *less* severe than a coincident
  crossing (each feed enters at 0.5, so the worst-case step is half), and the `wEff` collapse region
  is a disc interior to the hull, so it cannot meet the boundary on a non-degenerate rig. Record the
  argument in the probe comment; do not spend a probe proving it.

**BC — `air-sweep-differential`** (QUAL-01/1 for `airAmount`, P34)
Two renders of the same 1 s `0.02 → 1.0` sweep — cutoff on the 64-sample grid and on a 4096-sample
grid — **subtracted**. Assert `max |Δy| ≤ max|ΔG| · 2 · peak`, `max |ΔG|` computed in-probe from the
sweep schedule. **Negative control is the 4096 render itself**, which exceeds the 64-sample bound by
~48×. Nothing tuned, nothing mirrored.

**BD — `air-and-trim-bit-transparency`** (DSP-07/2, /5, /6, /7 — P33)
- Source **inside** the hull: renders at `airAmount ∈ {0, 0.35, 1.0}` **bit-identical by `memcmp`**,
  `airSamplesFiltered == 0` in all three. This is the D2 amendment's whole point: the shipping default
  patch is bit-transparent.
- Source **outside** the hull, `airAmount = 0`: `airSamplesFiltered == 0`.
- **Non-vacuity:** outside the hull at `airAmount = 0.35`, `airSamplesFiltered == samples · 2` and the
  render **differs** from the `airAmount = 0` render.
- `hullAtten = 0` vs `0.5` outside the hull → renders **differ** (the trim's non-vacuity half; the
  identity half is AV's `bitExact (…, 1.0f)`).
- DSP-07/7: oscillate the puck across the hull edge and assert the filter is **not** re-zeroed per
  block — the resident state produces the bit-exact entry BB measures, which a per-block `reset (0)`
  could not.
- The denormal precondition (P33) goes in this probe's comment.

**BE — `air-filter-nan-recovery`** (DSP-07/8, risk R6)
- Inject a non-finite sample with the filter **active** (outside hull, `airAmount = 0.35`) → finite
  output within one block via the P31 guard.
- **The skipped-filter hole:** poison the filter, move the source **inside** the hull (filter skipped,
  guard cannot see it), move back **outside** → finite, and the entry edge still bit-exact. This is
  the third payoff of P27 and it needs its own assertion or nobody will know it works.

**BF — `per-speaker-trim`** (FUNC-07/1–4)
- −12 dB at speaker *i* → exactly −12 dB on lane `speakerToBuffer[i]`, and the other seven **bitwise
  unchanged**. Then +6 dB. Both through a **non-identity label map** (C1: a container-only test is
  vacuous — `critical_audiochannelset_is_a_bitset_not_an_order`).
- The trim **step** obeys the 5 ms smoother bound (FUNC-07/3 + QUAL-01's D4 scope).
- FUNC-07/4: load a musical preset → all 8 trims bit-identical (the FUNC-05 store separation,
  exercised on this field specifically).

**BG — `live-venue-edit-during-playback`** (QUAL-01, D4 scope)
Probe AQ's mid-stream edit rig, unmodified (Q10: it works as-is). Edit a trim between two
`processBlock` calls; assert the smoother bound. **Window: start at the block boundary and run at
least 64 + 240 samples** — the edit takes effect at the next control boundary, up to 63 samples later,
and a narrower window can miss the event entirely and pass vacuously. Alignment is otherwise free
(the effect is always control-grid-aligned), which is why this needs less care than QUAL-03's
protocol.

**BH — `venue-value-sanitisation`** (P29 / H5)
- `trimDb` ∈ {1e30, −1e30, NaN} in the venue store → snapshot `trimLin` finite and within ±24 dB;
  render finite; **no permanent silence** — drive `w_i = 0` at that speaker, which is the exact
  `0.0f * inf = NaN` path.
- A NaN speaker **coordinate** → finite output (the folded-in pre-existing guard).
- Both must be shown to have been *live* before the fix: state in the detail string that the
  unsanitised path reaches `setTargetValue` through `dRaw < kMinDistance` and
  `denom < kDenomEpsilon`, both of which are **false for NaN**.

**BI — `blocksize-invariance-with-everything-live`** (ROADMAP criterion, QUAL-03 regression)
AL's 512/4096 pair **and** AM's ragged `1,7,64,333,4096` sequence, re-run with `width = 4`,
`hullAtten = 1.0`, `airAmount = 0.35` and the source **outside** the hull, so the trim, the filter and
the two distinct sub-point distances are all active. `memcmp`, never a tolerance. H10 is the argument
that this holds; this probe is the evidence.

**BJ — `control-rate-counters`** (Q2 / P32, PERF-01 regression)
`airCutoffUpdates == solveRuns · 2` exactly, `airSamplesFiltered` matching the expected
active-sample count, and **`powCalls == 16` still exact** (probe AE's assertion must not have been
disturbed by the new `pow`/`exp2` calls). Also re-confirm `sampleAdvances == totalSamples` in both
modes.

**Files:** `tests/render-harness/main.cpp`
**Depends on:** Tasks 5, 6

---

### Task 9 — Gates

Run every one; record **actual output**. Do not read a result out of a prior document.

1. **Clean 3-format build** (VST3 + AU + Standalone), forced TU recompile — **zero warnings, zero
   errors** across the whole log.
2. **Marker retirement:** `PHASE-2.3-WIDTH`, `PHASE-2.3-AIR`, `PHASE-2.3-TRIM` each appear **ZERO**
   times in `Source/`; `PHASE-2.2-REPLACE` still 0. **Count occurrences, not lines** — at 2.2 the
   first pass had each token twice because a comment quoted it.
3. `grep -rn` for hardcoded output channel indices outside `ChannelMap` — expected **zero**.
4. `auval -a | grep -i octagon`, then `auval -v aufx OuOc OuDv` → **AU VALIDATION SUCCEEDED**.
5. **pluginval strictness 10, VST3 and AU, ×3 each** (`pattern_ci_pluginval10_catches_latent_nan` —
   this phase adds the plugin's only recursive element **and** H4's sub-40 kHz rate hazard, which
   strictness 10 exercises).
6. Configure with `-DOUARICON_BUILD_TESTS=ON`; build and run **both** test targets → **exit 0**. All
   46 of A–AT still pass alongside AU–BJ → **62 probes**.
7. `gen_dbap_reference.py --check` → exit 0.
8. **17 parameters unchanged** against `parameter-spec.md` — 2.3 adds none; `width`, `hullAtten` and
   `airAmount` are already #4, #15, #16.
9. `setLatencySamples` still appears nowhere; no `switch` on `ChannelType`; `createEditor`'s
   `#if JUCE_WEB_BROWSER` guard still present.
10. `grep` the shipping TUs for `OOCTAGON_INSTRUMENT` — the plugin target must not define it; confirm
    from the build log that only the two test targets do. **Six** counters now, not four.
11. **Unit-target link line unchanged** — `juce_dsp` must **not** appear in
    `tests/unit/CMakeLists.txt`, and `GainStage.h`'s new `juce_dsp` include must not have leaked into
    anything the fast target compiles (H7 verified this holds; re-verify, because Task 5 is what could
    break it).

**Files:** none (verification)
**Depends on:** Tasks 7, 8

---

### Task 10 — Manual Logic gate (D5) ⚠️ HUMAN, before verify closes

**One combined ~15-minute session**, folding in 2.2's carried Task 12. Not blocking execute.

Carried from 2.2 (corroboration; FUNC-03/3's gate is probe AJ, shown non-vacuous by VERIFICATION-2.2
NC3):
- Automate `srcX` across the room — the 8 surround-meter lanes **no longer move in lockstep**.
- Set `w3 = 0` — that lane goes **silent** while the others compensate.

New at 2.3:
- **Width audibly spreads** the source as `width` goes 0 → 6.
- **Air audibly dulls** the source outside the hull, and is **inaudible inside** it (the D2 default-
  patch claim, by ear).
- **A per-speaker trim moves one lane only.**
- **H2's addition — cross the hull boundary with HF-rich material, not a sine.** The accepted cost of
  D2 is ~15 % of an 8 kHz component as a one-sample step. Measurement bounds it; this is the one claim
  in the phase that only listening can settle. If it ticks audibly, that is a **discuss-boundary**
  finding for Stage 3 (H3 names the lever: raising `fc(d_hull = 0)` toward Nyquist re-tunes the whole
  musical curve, so it is not a plan-phase change).

Record every observation verbatim in `SUMMARY-2.3.md`, positive or negative.

**Files:** none. **Depends on:** Task 9

---

### Task 11 — `SUMMARY-2.3.md` + `STATUS.md`

- **Three deviations, recorded explicitly:** P27 (`reset (x)` at the air edge, beyond §3.5.2's two
  named policies), P29's *position* guard (a pre-existing 2.2 hazard folded into a 2.3 site), and P25
  (`HullProcessor.h` header-only against ROADMAP's `.{h,cpp}`).
- **The H2 erratum in full** (P36): the corrected exact-response table, why the −0.7 dB figure was the
  analog filter's, why the magnitude-only framing understated the step by 5–190×, and why
  `ARCHITECTURE.md` was **not** re-pinned.
- **P33's method stated as a method**, not as "bit-transparency ✓".
- **Q6's negative result recorded**: D3's original 1 kHz differential slew probe measures +0.00000 %
  excess and would have passed vacuously. This is the kind of finding that must survive into the
  record, or the method gets re-proposed at Stage 3.
- Measured values for BB's two edges, BC's ratio, and AW's positive control.
- `REQUIREMENTS.md` **verdicts** are written at verify, not here. Task 1's method corrections already
  landed.

**Files:** `stages/2-dsp/SUMMARY-2.3.md`, `.planning/STATUS.md`
**Depends on:** Task 10

---

## Execution Constraints

- **DO NOT execute this phase in an isolated worktree.** `stages/2-dsp/` is untracked and so are eight
  `Source/` files; the scope would vanish and every gate would pass vacuously
  (`pattern_worktree_isolation_wrong_for_untracked_scope`).
- Plugin CMake target is **`OuariconOctagon`**; the folder is `O-Octagon`
  (`build_script_target_name_vs_folder`).
- Read `parameter-spec.md`, **never** `parameter-spec-draft.md`.
- **Nothing may be elided under a `width == 0` branch** (§3.4.3, constraint 4). Both solves stay
  unconditional and `powCalls == 16` must still hold **exactly**.
- **The air filter is per sub-point, not per speaker** — 2 instances, applied to the source signal
  before the gain matrix. Per-speaker would be 4× the cost and wrong (§3.5.2).
- **The per-sample invariant survives:** all 17 `getNextValue()` called exactly once, unconditionally,
  in both SAFE and REAL modes. The air skip gates `processSample` and nothing else.
- **H7's input aliasing is unchanged and still load-bearing.** `out[0]` and `in[0]` are the same
  memory; `s_L`/`s_R` are read at the top of each sample's iteration before any output write. The
  filter inserts *after* that read.
- No new APVTS parameters. The 17/17 programmatic gate must still pass.
- No `switch` on `AudioChannelSet::ChannelType`. No `setLatencySamples()`.
- Never call a parameter setter from the audio thread. In the harness write parameters with
  `setValueNotifyingHost (range.convertTo0to1 (real))` — `setValue()` alone leaves the cached atomic
  stale. Never assert against `apvts.state` in a console app.
- Clear the AU cache and sweep **both** `-dev` and unsuffixed bundles before any install
  (`./scripts/build-and-install.sh O-Octagon`).

## Non-goals for Phase 2.3 — must not appear

A `HullProcessor` **class**; a hand-rolled `OnePoleTPT` (P26); any per-speaker filter; any new APVTS
parameter; any change to the `fc(d_hull = 0) = 20 kHz` anchor (H3 — a discuss-boundary change); any
re-pin of `ARCHITECTURE.md` (P36); `VerifyPing`; metering; any WebView editor; any
`juce_add_binary_data` target; any change to `build-and-release.yml`; any substitution of
`std::atomic<std::shared_ptr>` for the snapshot double-buffer; any `width == 0` fast path.

---

## Success Criteria

**ROADMAP Phase 2.3 test criteria — all twelve:**

- [ ] `width = 0` produces gain vectors bit-identical to a single mono-summed source point *(AY)*
- [ ] `width > 0` produces two sub-points ⟂ the bearing from the centroid, correct handedness *(AX)*
- [ ] Sweeping the puck through the centroid at `width = 6` produces no discontinuity *(BA)*
- [ ] Sub-points straddling the hull boundary behave correctly and continuously *(BB + the Q9
      argument, recorded rather than probed)*
- [ ] `hullAtten = 0` → bit-identical to hull processing removed *(AV + BD, P33 method)*
- [ ] `airAmount = 0` → **bit-identical** to the filter absent *(BD, P33 method)*
- [ ] Air LPF cutoff matches the specified curve at {0.35, 1.0} × {5, 15} m *(AU)*
- [ ] Injecting a non-finite sample recovers within one block *(BE)*
- [ ] Per-speaker trim of −12/+6 dB — that level change at that speaker and nowhere else *(BF)*
- [ ] Full-range sweeps of every parameter produce no clicks, discontinuities or level jumps
      *(AS + AZ + BC = 11 + 5 + 1 = 17)*
- [ ] Block-size invariance still holds with width, hull and air all active *(BI)*
- [ ] **DSP-06, DSP-07, DSP-08, FUNC-07, QUAL-01 verified**

**Added by this plan:**

- [ ] **`d_hull = 0` is bit-transparent at any `airAmount`** — the D2 amendment's actual payoff, and
      not in ROADMAP's list *(BD)*
- [ ] The **entry edge is bit-exact** under P27's `reset (x)`, and the exit step is reported as a
      measured number rather than asserted away *(BB)*
- [ ] A NaN parked in a **skipped** filter is cleared at re-entry *(BE)* — the hole a per-block output
      guard structurally cannot see
- [ ] `trimDb = 1e30` with `w_i = 0` does **not** latch permanent silence *(BH)* — the defect FUNC-07's
      multiply would have created
- [ ] The cutoff is **below Nyquist at 22.05 and 32 kHz** *(AU)*, where the literal `20000` asserts in
      Debug and inverts the filter in Release
- [ ] DSP-08's invariance carries a **positive control** proving the probe can see the clamp *(AW)*
- [ ] QUAL-01's `airAmount` bound is **computed in-probe**, with a negative control separating by ~48×
      *(BC)*
- [ ] Every filter-path probe discards a ≥ 2000-sample lead-in *(H9)*
- [ ] `airCutoffUpdates == solveRuns · 2`; `powCalls == 16` **still exact** *(BJ)*
- [ ] Three `PHASE-2.3-*` markers at **zero occurrences**, and no prose resurrects them
- [ ] The unit target's link line still has **no `juce_dsp`** *(gate 11)*
- [ ] 62 probes, both targets exit 0; clean 3-format build, zero warnings; pluginval s10 ×3 each;
      `auval` SUCCEEDED
- [ ] Contract checksums re-verified at 2.3 verify, **and no pin has moved**

**Requirement outcomes expected at verify:** FUNC-07 ✅ · DSP-06 ✅ · DSP-07 ✅ *(under §3.5.2 as
amended, with P36's erratum)* · DSP-08 ✅ · QUAL-01 ✅ *(under the D3/D4 scope as corrected by P34)* ·
**QUAL-04 ✅ — criterion 3 closes, clearing 2.2's PARTIAL.**

**Stage 2 then closes with a stage-level `VERIFICATION.md`.**

---

## Risks Active in This Phase

| Risk | Severity | Mitigation in this plan |
|---|---|---|
| **H1 — a re-entry click 5.2× the signal's own max slew** | HIGH | P27's `reset (x)`; BB asserts the entry edge **bit-exact**, so dropping the seed fails loudly rather than quietly |
| **H5 — FUNC-07's multiply arms a permanent-silence NaN latch** | HIGH | P29 sanitises at `publishSnapshot()`, the single funnel; BH drives `trimDb = 1e30` **with `w_i = 0`**, the exact `0.0f * inf` path |
| **DSP-06/1 regresses when `width` goes live** | HIGH | AY, with a built-in negative control. `width`'s default is 0.0, so this is the default patch |
| **R6 — sticky NaN in the only recursive element** | MEDIUM | P31's per-block output guard (Q1: equivalent and immediate), **plus** P27's edge re-seed for the skipped-filter hole; BE probes both |
| **H4 — `fc = 20 000` asserts in Debug and inverts the filter in Release below 40 kHz** | MEDIUM | P28's `min (20000, 0.45·fs)`; AU asserts `fc < fs/2` at four rates; pluginval s10 exercises unusual rates |
| **Q6 — the specified QUAL-01 method is vacuous (+0.00000 %)** | MEDIUM | P34's two-render differential with an in-probe bound; Task 1 corrects `REQUIREMENTS.md` **before** execute so the docs cannot disagree |
| **H8 — a `λ = 2, blur = 1` invariance probe sits 0.9 % from a clamp** | MEDIUM | P35's `blur = 0.25` (4× margin), source scaled too, plus a positive control at λ = 2.1 |
| **H2 — the accepted cost of D2 is 5–190× the quoted figure** | MEDIUM | P36's erratum; BB asserts the *predicted* `A·\|H − 1\|`; D5 listens to it on HF-rich material |
| **A marker survives in prose and grandfathers itself** | MEDIUM | Gate 2 counts **occurrences**, not lines; Task 4 deletes the comment that references the tokens |
| **`juce_dsp` leaks into the fast unit target** | LOW | Gate 11. H7 verified the include direction runs one way; Task 5 is what could break it |
| **PERF-01 regresses at control rate** | LOW | P32's `airCutoffUpdates == solveRuns · 2` makes placement a number; BJ re-asserts `powCalls == 16` exactly |

---

## Next Phase

**Ready for:** execute phase — Phase 2.3 (Source Shaping and Outside-Hull Processing).

Writes `stages/2-dsp/SUMMARY-2.3.md`, then **`stages/2-dsp/VERIFICATION.md`** at verify — the
stage-level report that closes Stage 2. 2.1 and 2.2 wrote phase-suffixed ones only.

**Owed at Stage 3 discuss, before Stage 3 plan:** `FUNC-06` and `UI-02..05` have summary rows in
`REQUIREMENTS.md` and **no acceptance criteria** — the same defect this stage repaired three times
(PERF-02 and QUAL-04 at the 2.2 boundary; FUNC-07, DSP-06, DSP-07 and DSP-08 at the 2.3 boundary).
`COMPAT-04`'s criteria are owed retroactively at Stage 4, along with the CI gap
(`.planning/todos/pending/` — no test target in this repo has ever run in CI, so all 62 probes fire
only under `-DOUARICON_BUILD_TESTS=ON` locally).
