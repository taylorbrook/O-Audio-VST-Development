# Stage 2 — DSP · Phase 2.3 (Source Shaping and Outside-Hull Processing) — Research

**Plugin:** O-Octagon
**Stage:** 2 of 4 — DSP
**Phase:** 2.3 of 3 — Source Shaping and Outside-Hull Processing
**GSD phase:** research
**Date:** 2026-08-11
**Branch:** `feat/o-octagon` @ `a47cef88` (2.2 work uncommitted)
**Depth:** DEEP (complexity tier 6)

---

## Entry Check — contract checksums

Re-computed here, not read out of `CONTEXT-2.3.md` (`pattern_promotion_checksum_pins_replaced_file`).

| Contract | SHA-256 on arrival | Result |
|---|---|---|
| `BRIEF.md` | `697a4f32…f6b9fbd6` | ✅ matches `STATUS.md` |
| `parameter-spec.md` | `b45f88dc…cbb9e02f` | ✅ matches |
| `research/ARCHITECTURE.md` | `a8a358f4…f0429b6d4408` | ✅ matches the **D2 re-pin** issued at 2.3 discuss |
| `ROADMAP.md` | `aec7d0ce…0137ee29` | ✅ matches |

No drift. Two superseded architecture hashes (`bff8a83b…`, `cd881a10…`) are recorded in frontmatter
and are not expected to match anything from here on.

**Marker state, verified rather than assumed:** each of `PHASE-2.3-WIDTH`, `PHASE-2.3-AIR`,
`PHASE-2.3-TRIM` appears **exactly once** in `Source/`. The 2.2 retirement gate is intact and the
2.3 target of zero is well-defined.

**Probe inventory, verified:** 46 probes, `A`–`AT`, no gaps in the letter sequence — **29 in the unit
target, 17 in the render harness**, reconciling exactly against the 48 `check (` occurrences once the
two `void check (…)` definitions are excluded.

---

## Scope discipline

2.3 is small in code and sharp in measurement, exactly as `CONTEXT-2.3.md` says. This research does
**not** re-open `shaper::shape()`, `dbap::solve()` or `hull::project()` — all three are complete and
correct, and `projection.distance` **is** `d_hull` (`ConvexHull2D.cpp:86-119`, always ≥ 0, always
finite, `0.0f` on every degenerate count).

What it does do is answer the ten open questions against JUCE 8.0.14 source and against the
toolchain, and then report ten findings the questions did not ask about — one of which (**H1**) would
ship an audible click, and one of which (**H5**) would ship a permanent NaN latch.

The numeric work behind Q6, Q7 and H1–H4/H8/H9 is **reproducible and committed** —
`tests/tools/air_filter_study.py`, run with no arguments, prints every table below. It is a research
artefact, not a build step and not a test, committed for the same reason
`tests/fixtures/DbapReferenceFixture.h` ships with its generator: so the numbers can be re-derived
rather than trusted. Every dB figure below is computed from the filter's exact transfer function,
transcribed from JUCE source, not quoted from the architecture.

---

## Answers to the Open Questions

### Q1 — `FirstOrderTPTFilter` state inspection, and whether an output check is equivalent

**`s1` is private and there is no accessor** — `juce_FirstOrderTPTFilter.h:149`:

```cpp
private:
    SampleType G = 0;
    std::vector<SampleType> s1 { 2 };
    double sampleRate = 44100.0;
```

`reset(SampleType)` (`.h:97`) can **write** the state; nothing can read it.

**The output check is equivalent, and it is *immediate*, not one sample late.** From
`juce_FirstOrderTPTFilter.cpp:89-107`:

```cpp
auto v = G * (inputValue - s);
auto y = v + s;              // y = G*x + (1-G)*s
s = y + v;
```

`y` is an affine function of `s` **on the same sample**. A non-finite `s` makes `y` non-finite
immediately; there is no one-sample latency to reason about, and CONTEXT's parenthetical ("surfaces
on the next sample") is conservative rather than wrong.

It is also **sticky by construction**: `s = y + v` re-derives `s` from a value that is already NaN,
so once poisoned it never recovers. `pattern_envelope_follower_state_sticky_nan` applies exactly.

**Write the DSP-07 guard against the output**, once per block, `reset()` on failure. Unlike
`pattern_biquad_nan_guard_sticky_silence` there is no "last known good coefficient" problem here:
`G` is recomputed from `cutoffFrequency` at every control block and is never derived from the state,
so `reset()` alone fully restores the filter.

> **But see H5.** The guard cannot see a NaN parked in a filter that is currently *skipped*, and
> under D2 a filter can be skipped indefinitely. H1's recommended edge behaviour closes that hole as
> a side effect.

---

### Q2 — `setCutoffFrequency` cost, and whether it needs a counter

**One `std::tan` per call, and nothing else** (`.cpp:52-59` → `update()` at `.cpp:117-122`):

```cpp
void setCutoffFrequency (SampleType newValue) { …; cutoffFrequency = newValue; update(); }
void update() { auto g = tan (pi * cutoffFrequency / sampleRate); G = g / (1 + g); }
```

`kControlBlock = 64` (`GainStage.h:114`), so at 48 kHz there are **750 control blocks/second** and
two calls per block:

| Call site | per block | per second @ 48 kHz |
|---|---|---|
| `std::tan` via `setCutoffFrequency` | 2 | **1 500** |
| `std::exp2` for the two cutoffs | 2 | 1 500 |
| `std::pow` via `decibelsToGain` (hull trim, per sub-point) | 2 | 1 500 |
| `std::pow` via `decibelsToGain` (`outGain`) — already present at 2.2 | 1 | 750 |
| `countedPow` inside `dbap::solve` — unchanged | 16 | 12 000 |

CONTEXT's "~1500 tan/second" is **confirmed exactly**. 2.3 adds 4 500 transcendental calls/second on
top of 12 750 — about 35% more, **all at control rate, none per sample.** PERF-01's per-sample
budget is untouched.

**A counter is worth adding, but not for `tan`.** The number that can silently regress is not the
call count — it is whether `setCutoffFrequency` stays *out of the per-sample loop*. A
`instr::airCutoffUpdates` counter asserted `== solveRuns * 2` is the executable form of that, and it
costs the same as the four counters already live under `OOCTAGON_INSTRUMENT`. It also makes PERF-01
a number rather than an argument, which is the standing discipline (`GainStage.cpp:40-42`).

**Do not route the new `pow`/`exp2` calls through `countedPow`.** Probe AE asserts
`powCalls == 16` exactly, and that assertion is load-bearing for §3.3.5's budget. `decibelsToGain`
calls `std::pow` directly and is not counted — verified at
`juce_Decibels.h:54-59` — so the existing gate is unaffected. Confirm this rather than assume it: it
is the kind of thing that turns a green gate red for a reason unrelated to the change.

---

### Q3 — Does `setCutoffFrequency` reset or perturb state?

**No. It touches `cutoffFrequency` and `G` only** — `s1` is not named anywhere in
`setCutoffFrequency()` or `update()` (`.cpp:52-59`, `.cpp:117-122`). The only writers of `s1` are
`prepare()`, the two `reset()` overloads, `processSample()` and `snapToZero()`.

Two consequences, both load-bearing for D2:

1. **The "skip without reset" design is coherent.** A cutoff change is a *coefficient* change, not a
   state change, so the resident state stays meaningful across an arbitrary number of cutoff moves.
2. **A cutoff change is not itself the discontinuity D3 is trying to measure.** The step it produces
   is exactly

   ```
   Δy = (G_new − G_old) · (x − s)
   ```

   read straight off `processSample`. That is an analytic quantity, and Q6 uses it.

---

### Q4 — `prepare()` on the filters vs. `GainStage::prepare`'s "one reset site, ever" (P23)

`FirstOrderTPTFilter::prepare` (`.cpp:62-73`) does four things:

```cpp
sampleRate = spec.sampleRate;
s1.resize (spec.numChannels);
update();
reset();
```

**`ProcessSpec` is `{ double sampleRate; uint32 maximumBlockSize; uint32 numChannels; }`, and
`maximumBlockSize` is never read by this filter.** Only `sampleRate` and `numChannels` are used.
That matters because `OOctagonProcessor::prepareToPlay (double sampleRate, int)` deliberately drops
its second parameter (`PluginProcessor.cpp:190-193`).

**Recommendation: re-instate the parameter name and pass it.** Constructing a `ProcessSpec` with a
fabricated `maximumBlockSize` is a small lie in a codebase that has been careful not to tell any;
naming the parameter and forwarding it costs one identifier and removes the question. The
`-Wunused-parameter` comment at `PluginProcessor.cpp:191-193` then simply goes away, because the
parameter is used.

**On P23 — this extends the discipline, it does not violate it.** P23's rule is that
`SmoothedValue::reset()` is a *state teleport* and must have exactly one call site. `filter.prepare()`
performs the analogous teleport for the filter. Put it in **step 2** of `GainStage::prepare`,
alongside the seventeen `s.reset (sampleRate, 0.005)` calls, and the rule becomes "step 2 is the only
place any DSP state is reset, ever" — one site, one step, stated once.

Two footnotes:

- `s1.resize()` **can** allocate. It does not here (see H6 — the default vector already has size 1,
  and `numChannels` is 1), and in any case `prepareToPlay` is not a real-time context. Probe AO arms
  its counter *after* `negotiate()` (`main.cpp:1187-1190`), so filter preparation is outside the
  measured window either way. Nothing to do; recorded so the plan does not spend a task on it.
- The **DSP-07 NaN guard's `reset()` is a second site by necessity**, and that is fine — it is a
  recovery path, not an initialisation path. Say so explicitly in the header comment, or the next
  reader will "fix" it.

---

### Q5 — The exact bit-transparency claim

**Confirmed, and the toolchain assumption is checked rather than assumed.**

`juce::Decibels::decibelsToGain` (`juce_Decibels.h:54-59`):

```cpp
return decibels > minusInfinityDb ? std::pow (Type (10.0), decibels * Type (0.05)) : Type();
```

At `d_hull = 0` with any `hullAtten`, the argument is `-0.0f`:

- `-0.0f > -100.0f` → **true** (IEEE compares `-0.0 == 0.0`), so the `pow` branch is taken.
- `-0.0f * 0.05f` → `-0.0f`.
- `std::pow (10.0f, -0.0f)` → **exactly `1.0f`**. C99 Annex F / IEEE 754 mandate `pow(x, ±0) == 1`
  for every `x`, including NaN. This is not an approximation that happens to round well.
- `v * 1.0f == v` bit-exactly for every finite `v`, both zeros, and both infinities.

**No `-ffast-math` anywhere on the line.** `juce_recommended_config_flags` adds `-O3` in Release and
`-g -O0` in Debug and nothing else (`JUCEHelperTargets.cmake:139-146`); the plugin adds no other
compile options. IEEE semantics hold, so the `pow(x, ±0)` guarantee is real.

**`trimLin` is exactly `1.0f` at the default**, by the same route: `trimDb` defaults to `0.0f`
(`VenueModel.cpp:166`) and `trimLin()` is `decibelsToGain (trimDb (i))` (`VenueModel.cpp:327-330`).

**One stated precondition, not a loophole.** `processBlock` runs under `juce::ScopedNoDenormals`
(`PluginProcessor.cpp:330`), so FTZ/DAZ is active. If a solved `v_i` were **denormal**
(< 1.18e-38), `v_i * 1.0f` would flush to zero where 2.2 stored the denormal. Reaching a denormal
`v_i` requires a weight around 1e-38; it is unreachable from the exposed 0–1 weight range with any
non-degenerate geometry, and the DSP-06/1 probe uses default weights. **State the precondition in
the probe's comment** — an unstated precondition is how a bit-identity claim quietly becomes false.

---

### Q6 — The differential sine probe's tolerance, derived

**D3's method as written does not work, and the reason is worth stating precisely rather than
patching around.**

Simulated: `airAmount` swept 0.02 → 1.0 over one second at `d_hull = 5 m`, `fs = 48 kHz`, sine
amplitude 0.5, filter always active. `max |out[n] − out[n−1]|` compared against the same measurement
with `airAmount` held:

| probe tone | held | swept, 64-sample grid | excess |
|---|---|---|---|
| 1 kHz | 0.072583660 | 0.072583660 | **+0.00000%** |
| 8 kHz | 0.464506364 | 0.466542207 | +0.438% |

At 1 kHz the excess is **identically zero to nine decimals**. The max slew of the render is set by
the sine's own zero-crossing, which occurs early, where the swept and held renders still share a
cutoff. The probe would pass, and it would pass for a reason that has nothing to do with the code.

**The quantity that *is* measurable is the coefficient step itself**, and Q3 gives it in closed form:

```
Δy = (G_new − G_old) · (x − s)          ≤   max|ΔG per control block| · 2 · peak
```

Isolated by simulation, and it scales with the update interval **exactly as predicted**:

| tone | 64-sample grid | 4096-sample grid | ratio | step / natural slew (64) |
|---|---|---|---|---|
| 1 kHz | 5.273e-05 | 2.618e-03 | **49.7×** | 7.3e-04 |
| 4 kHz | 2.142e-04 | 1.048e-02 | **48.9×** | 8.4e-04 |
| 8 kHz | 4.507e-04 | 2.086e-02 | **46.3×** | 9.7e-04 |

`max |ΔG|` for a full-speed 0→1 sweep, computed from the sweep the probe itself drives:

| sweep duration | control blocks | max\|ΔG\| | bound at peak 0.5 |
|---|---|---|---|
| 0.25 s | 187 | 5.378e-03 | 5.378e-03 |
| 0.50 s | 375 | 2.685e-03 | 2.685e-03 |
| **1.00 s** | 750 | **1.343e-03** | **1.343e-03** |
| 2.00 s | 1500 | 6.719e-04 | 6.719e-04 |

**So the answer to "what is the right tolerance?" is: there is no tolerance to pick.** The probe
computes `max |ΔG|` from the same sweep it renders — the sweep schedule is already in the probe — and
asserts `max |Δy| ≤ max|ΔG| · 2 · peak`. Nothing is tuned, nothing mirrors a constant
(`pattern_test_fixture_mirrors_drift_silently`), and the 4096-sample negative control exceeds it by
~48× rather than by a hair.

**But the step is 3–4 orders of magnitude below the signal's own per-sample slew at every tone**, so
it cannot be read off `max |out[n] − out[n−1]|` of the raw render. Two ways to get at it, in order of
preference:

1. **Assert the analytic bound on the *difference between two renders that share the signal*** —
   render at the 64-sample grid and at the 4096-sample grid, subtract, and the sine cancels to the
   precision of the two gain trajectories. The 48× separation above is then directly visible.
2. **Give up on measuring the coefficient step through QUAL-01 at all**, and let the analytic bound
   stand as the argument, spending QUAL-01's `airAmount` budget on the edges instead (H1/H3) — which
   is where the audible discontinuity actually lives, by five orders of magnitude.

Recommendation: do **both** — (1) is cheap and non-vacuous, and (2) is where the requirement's
"no audible discontinuity" language actually bites.

---

### Q7 — Is 1 kHz sensitive enough to see the D2 step?

**No — and the reason is that the D2 step is not primarily a magnitude step at all.** See **H2**;
this is the short form.

The step at a hull crossing is the difference between "filter at `fc = 20 kHz`" and "no filter", i.e.
the complex quantity `H(f) − 1`. D2 and §3.5.2 quote only `| |H| − 1 |`:

| tone (fs = 48 kHz) | magnitude only | **full \|H − 1\|** | phase term dominates by |
|---|---|---|---|
| 1 kHz | 1.542e-04 | **1.756e-02** | **114×** |
| 4 kHz | 2.567e-03 | 7.161e-02 | 27.9× |
| 8 kHz | 1.176e-02 | **1.529e-01** | 13.0× |
| 15 kHz | 7.185e-02 | 3.722e-01 | 5.2× |
| 20 kHz | 2.929e-01 | 7.071e-01 | 2.4× |

So the answer flips: **1 kHz is fine, but not for the reason D3 assumed.** The magnitude tilt at
1 kHz is 1.5e-4 of the component and genuinely invisible; the *full* step is 1.8e-2, because a 20 kHz
one-pole imposes a **1.0° phase lag** at 1 kHz and switching it out removes that lag instantly. At
amplitude 0.5 that is a one-sample jump of 8.8e-3 against a natural per-sample slew cap of 6.5e-2 —
**13% of the signal's fastest legitimate move, and comfortably measurable.**

A second tone is still worth having: at **8 kHz** the step is 0.153 of the component, which is where
the cost is large enough to matter musically and is the right tone for D5's listening check.

**Criterion 1 keeps 1 kHz; criterion 2 uses 1 kHz *and* 8 kHz.** D3's method transfers unchanged in
form. Its *bound* does not — see H2.

---

### Q8 — `trimLin` provenance

**The plumbing is complete. FUNC-07 really is one multiply.** The chain, end to end:

```
VenueModel::trims[i]            VenueModel.cpp:166 (default 0.0f), :228 (read), :290 (set)
  → VenueModel::trimDb(i)       VenueModel.cpp:322
  → VenueModel::trimLin(i)      VenueModel.cpp:327   decibelsToGain(trimDb(i))
  → snapshot.trimLin[i]         PluginProcessor.cpp:295   inside publishSnapshot()
  → (applied nowhere)           GainStage.cpp:182   ← PHASE-2.3-TRIM
```

`publishSnapshot()` is called from both `readVenueFromState()` and `rebuildChannelMap()`, so trims
ride the same generation-stamped release/acquire edge as the geometry. Nothing to build.

**No plumbing gap — but a sanitisation gap, and 2.3 is what makes it dangerous.** See **H5**.

---

### Q9 — Sub-points straddling the hull boundary

**Straddling is strictly *less* severe than a coincident crossing, and the `wEff` collapse cannot
interact with it except on a degenerate rig.** Both halves are provable rather than empirical.

**Severity.** The two feeds enter at 0.5 each (`GainStage.cpp:231-232`, §3.4.3's level convention),
and each sub-point's air filter acts on its own feed *before* the gain matrix. When only sub-point L
crosses, the discontinuity appears only in the `gL[i] · s_L` term. The worst-case output step is
therefore **half** the coincident-crossing step, at every speaker. The `width = 0` case — both
sub-points crossing together — is the worst case, and it is the one the probes already drive.

**Interaction with `wEff`'s centroid collapse: unreachable on any non-degenerate rig.**
`rFade = 0.15 · rigScale` (`SourceShaper.h:66`) is measured from the **rig centroid**, and the
floor-projected centroid of eight speakers is a convex combination of the eight floor points — so it
lies inside their convex hull *by definition*, and strictly inside unless all eight are collinear.
The collapse region is a disc of radius ≈ 1.19 m around a point that is interior to the hull; the
hull boundary is the place where crossings happen. They meet only when the hull degenerates to a
segment (`hullCount ≤ 2`), and there `hull::project` returns a distance from a segment or a point and
`d_hull` is still well defined.

**No hazard, no probe needed beyond the coincident case.** Record the argument; do not spend a task
proving it empirically.

---

### Q10 — Venue live-edit probe mechanics (D4)

**Probe AQ's rig works unmodified**, and the reason QUAL-01's live-edit probe needs *less* care than
QUAL-03's is structural.

`proc.applyVenueEdit (v)` lands between two `processBlock` calls (`main.cpp:1359-1371`). But a venue
edit does not take effect when it is published — it takes effect at the **next control boundary**,
because `updateControl()` only runs at `phase == 0` (`GainStage.cpp:108-109`) and the dirty check
keys off `snapshot.generation` (`GainStage.cpp:137-139`).

**So the effect is *always* control-grid-aligned, regardless of where the edit landed.** QUAL-03's
protocol needs automation at multiples of the larger block size because it compares two renders at
different block sizes; QUAL-01's live-edit probe compares one render against a bound, so alignment is
free.

**The one thing the probe must get right** is the measurement window: the edit's effect begins at the
first multiple of 64 at or after the block boundary, i.e. up to **63 samples later** than the
`applyVenueEdit` call. A window that starts at the block boundary and is narrower than 64 samples can
miss the event entirely and pass vacuously. Start the window at the block boundary and run it at
least 64 + 240 samples (one control block plus the 5 ms ramp at 48 kHz).

---

## Findings beyond Q1–Q10

### H1 — D2's "leave the state resident" is the **worst** of the three re-entry policies, and `reset(x)` makes the edge bit-exact *(HIGH — would ship an audible click)*

D2 rejected resetting on every skip, correctly, because re-zeroing a filter every time a puck
oscillates across the hull edge is self-inflicted damage. It then chose the only other option it
considered: leave the state where it is. **There is a third option, and it is strictly better than
both.**

When `airActive` goes false → true, the first sample out of the filter is

```
y = G·x + (1 − G)·s_resident
```

Its error against the dry signal it is replacing is `(1 − G)·|s_resident − x|`. Simulated: a source
sitting at `d_hull = 12 m` (`airAmount = 0.35` → `fc = 7.6 kHz`, `1 − G = 0.649`) that ducks inside
the hull for one control block and comes straight back out:

| tone | re-entry policy | \|y − x\| at the edge | vs. the signal's own max per-sample slew |
|---|---|---|---|
| 1 kHz | **resident (D2 as written)** | 3.407e-01 | **520.9%** |
| 1 kHz | `reset(0)` (original §3.5.2) | 2.810e-01 | 429.6% |
| 1 kHz | **`reset(x)`** | 0 | **0.0%** |
| 8 kHz | resident (D2 as written) | 3.149e-02 | 6.3% |
| 8 kHz | `reset(0)` | 2.810e-01 | 56.2% |
| 8 kHz | **`reset(x)`** | 0 | **0.0%** |

A click **five times larger than the fastest move the signal itself can make** is not a subtle
artefact. QUAL-01 criterion 2 says "rapid puck movement across the hull boundary produces no audible
discontinuity"; rapid movement is precisely the case that leaves `s_resident` furthest from `x`.

**The fix is one float store, and it is exact — not approximately exact.** Seeding `s = x` on the
false→true edge:

```cpp
airL.reset (s_L);          // public: juce_FirstOrderTPTFilter.h:97
```

then `processSample` computes `v = G * (x − s) = G * 0.0f = 0.0f` and `y = 0.0f + x`. **`y == x`
bit-exactly**, on every toolchain, at every cutoff, at every entry speed — by the arithmetic, not by
convergence. (The algebraic form `G·x + (1−G)·x` *would* round; the code's form does not. The
distinction is real and it is in our favour.)

**This satisfies D2's own rationale better than D2's own choice does.** D2's objection was to
re-zeroing — to discarding continuity. `reset(x)` discards nothing; it re-seeds the state with the
value the filter would have converged to. The oscillating-puck hazard D2 named disappears entirely,
because the seeded value *is* the continuous one.

It is the same shape of move as D2 itself: an extension of the section's own reasoning to an axis the
original wording did not consider. **Recommend adopting it. It is an implementation choice inside
§3.5.2's stated intent, so it needs a recorded deviation in `SUMMARY-2.3.md`, not a checksum re-pin.**

> **Third independent reason to adopt it:** it closes the skipped-filter NaN hole in Q1/DSP-07 for
> free. A NaN parked in a filter that is currently skipped is invisible to a per-block output check;
> an unconditional re-seed at the edge overwrites it before it can be used.

---

### H2 — D2's cost figures are magnitude-only, and understate the accepted cost by 5–190× *(HIGH — the number QUAL-01/2 must be measured against is wrong)*

§3.5.2 as amended, and `CONTEXT-2.3.md` D3, both state the accepted cost as "**bounded at 3 dB @
20 kHz, 0.7 dB @ 10 kHz, and 0 dB at DC**", and build the whole QUAL-01/2 measurement plan on the
last figure ("invisible to a DC probe by construction").

**Two problems, one arithmetic and one conceptual.**

**Arithmetic — the 10 kHz figure is wrong.** The exact response of the TPT one-pole at `fc = 20 kHz`:

| fs | @1 kHz | @4 kHz | @8 kHz | @10 kHz | @15 kHz | @20 kHz |
|---|---|---|---|---|---|---|
| 44 100 | −0.0005 dB | −0.0081 | −0.0384 | **−0.0695** | −0.3008 | −3.0103 |
| 48 000 | −0.0013 dB | −0.0223 | −0.1027 | **−0.1798** | −0.6476 | −3.0103 |

The quoted −0.7 dB is the **analog** one-pole at `f/fc = 0.5` (−0.969 dB, rounded down). The digital
filter is far flatter, because `fc = 20 kHz` sits at 0.83 × Nyquist at 48 kHz and bilinear prewarping
compresses the whole passband. **The 20 kHz figure is right** (−3.0103 dB is the definition of the
cutoff). The error is in the conservative direction, so **D2's decision stands a fortiori.**

**Conceptual — and this one changes the measurement.** The step at the crossing is the difference
between two *signal paths*, so the quantity is the complex `H(f) − 1`, not `| |H(f)| − 1 |`. The
filter's **phase lag** is the dominant term everywhere below ~15 kHz:

| tone (fs = 48 kHz) | \|H\| | phase | magnitude-only step | **full \|H − 1\|** | ratio |
|---|---|---|---|---|---|
| 1 kHz | 0.999846 | −1.006° | 1.542e-04 | **1.756e-02** | **114×** |
| 2 kHz | 0.999378 | −2.020° | 6.216e-04 | 3.525e-02 | 56.7× |
| 4 kHz | 0.997433 | −4.107° | 2.567e-03 | 7.161e-02 | 27.9× |
| 8 kHz | 0.988244 | −8.794° | 1.176e-02 | **1.529e-01** | 13.0× |
| 15 kHz | 0.928152 | −21.85° | 7.185e-02 | 3.722e-01 | 5.2× |
| 20 kHz | 0.707107 | −45.00° | 2.929e-01 | 7.071e-01 | 2.4× |

At 44.1 kHz the ratio at 1 kHz is **190×**.

**Consequences, in order of how much they change the plan:**

1. **The accepted cost of D2 is ~1.8% of a 1 kHz component and ~15% of an 8 kHz component, as a
   one-sample step.** Not 0.0013 dB and 0.10 dB. Still bounded, still one sample, still only at a
   deliberate gesture — but it is a real tick on HF-rich material, and **D5's listening session is
   the right place to judge whether it is acceptable**, with 8 kHz-rich material specifically.
2. **QUAL-01 criterion 2's probe bound must be `A · |H_20k(f) − 1|`**, derived from the transfer
   function. That makes the probe *predictive*: it asserts a specific number rather than "under some
   bound", so it fails loudly if H1's `reset(x)` is dropped (the resident-state edge is 20× larger at
   1 kHz).
3. **"0 dB at DC" is still true and still the reason the DC probe is blind** — the phase lag also
   goes to zero at DC. D3's core insight is unaffected. Only the magnitudes move.

**`REQUIREMENTS.md`'s QUAL-01 scope note and `ARCHITECTURE.md` §3.5.2's amendment paragraph both
carry the "0.7 dB @ 10 kHz" figure.** Correcting the architecture is a contract edit; the plan phase
should decide whether to re-pin now or carry the correction as a recorded erratum. **Recommend
carrying it as an erratum in `SUMMARY-2.3.md`** — the figure is descriptive prose about a cost, not a
specification the code implements, and re-pinning a contract mid-phase is the thing the discipline
exists to avoid. `REQUIREMENTS.md` is not checksummed and can be corrected in place.

---

### H3 — The two edges are asymmetric, and only one of them is fixable *(MEDIUM — shapes what criterion 2 can promise)*

With H1's `reset(x)` in place:

| edge | what happens | step |
|---|---|---|
| **entry** (inside → outside, filter switched **in**) | `v = G·(x − x) = 0`, `y = x` | **0, bit-exact** |
| **exit** (outside → inside, filter switched **out**) | dry replaces `H_20k · x` | `A · \|H_20k(f) − 1\|` |

The exit step is **inherent to D2** and cannot be removed by any state manipulation — there is no
state on the dry path to seed. It is bounded by the table in H2.

**One option is worth naming and then not taking.** The exit step exists only because `fc(d_hull = 0)`
is 20 kHz rather than infinite. As `fc → fs/2`, `G → 1` and `H → 1`, so the switch becomes continuous:

| `fc(d_hull = 0)` | \|H − 1\| at 8 kHz |
|---|---|
| 20 000 Hz | 0.15288 |
| 22 000 Hz | 0.07579 |
| 23 000 Hz | 0.03781 |
| 23 800 Hz | 0.00756 |

But 20 kHz is also the anchor of the whole musical curve — it is what makes `0.35 / 5 m → 13.3 kHz`
in §3.5.2's table. Moving it re-tunes the mapping, which is a **discuss-boundary** change, not a
plan-phase one. **Recorded so the option is on the record and nobody rediscovers it at verify.**

---

### H4 — The literal `20000` ceiling trips a JUCE assertion and produces an unstable filter below 40 kHz *(MEDIUM — a Debug crash and a Release nonsense)*

`setCutoffFrequency` asserts (`juce_FirstOrderTPTFilter.cpp:55`):

```cpp
jassert (isPositiveAndBelow (newValue, static_cast<SampleType> (sampleRate * 0.5)));
```

§3.5.2 specifies `clamp(…, 500, 20000)` with 20 000 as a literal.

| fs | Nyquist | `fc = 20 000` |
|---|---|---|
| 22 050 | 11 025 | **asserts** |
| 32 000 | 16 000 | **asserts** |
| 44 100 | 22 050 | ok |
| 48 000 | 24 000 | ok |

And the Release behaviour is worse than the Debug one: past Nyquist, `tan(π·fc/fs)` goes **negative**,
so `G = g/(1+g)` is negative or singular and the one-pole is not a lowpass at all.

44.1 kHz is the lowest rate anyone is likely to run, but hosts do offer 32 kHz, and pluginval at
strictness 10 exercises unusual rates. **The ceiling must be `min (20000, 0.45 · fs)`:**

| fs | ceiling |
|---|---|
| 22 050 | 9 922.5 |
| 32 000 | 14 400.0 |
| 44 100 | 19 845.0 |
| **48 000** | **20 000.0** |

At 44.1 kHz this clips the ceiling from 20 000 to 19 845 Hz — a 0.07 dB change at 10 kHz, inaudible,
and it makes the plugin correct everywhere instead of correct above 40 kHz. **The 500 Hz floor also
needs the ceiling as its own upper bound** so the clamp cannot invert at very low rates.

The floor itself never binds inside the exposed ranges except at extremes: at `airAmount = 1.0` it is
reached at `d_hull = 15.97 m`, which is off the far edge of any realistic hall. §3.5.2's four-row
table re-derives **exactly** (13 348 / 5 946 / 6 300 / 625 Hz vs. 13.3 k / 5.9 k / 6.3 k / 0.62 k) —
the architecture's arithmetic is correct.

---

### H5 — FUNC-07's multiply turns an unsanitised venue value into the H2 permanent NaN latch *(HIGH — a new defect created by this phase)*

**The 17 musical parameters are sanitised. The 42 venue values are not.**

`snapshotParameters()` guards every parameter (`PluginProcessor.cpp:227-229`):

```cpp
p[k] = std::isfinite (raw) ? raw : paramDefaults[k];   // "17 branches per block. The cost is nil"
```

`VenueModel::readFromState` has **no equivalent**. `readFloat` (`VenueModel.cpp:114-120`) returns
whatever the `ValueTree` holds, and there is no `jlimit`, no `clamp`, and no `isfinite` anywhere in
`VenueModel.cpp`. `setSpeakerTrimDb` (`:285-293`) accepts any float.

Today that is latent, because `trimLin` is carried and never used. **The `PHASE-2.3-TRIM` multiply is
what arms it:**

```
trimDb = 1e30   →   decibelsToGain → pow(10, 5e28) = +inf   →   trimLin = inf
v_i = 0.0f      (exactly, whenever w_i == 0 — DSP-05 criterion 1)
v_i * trimLin   =   0.0f * inf   =   NaN
→ setTargetValue (NaN) → SmoothedValue latches → PERMANENT SILENCE
```

That is exactly the H2 latch from RESEARCH-2.2, reached through a new door. And the door is reachable:
venue values come from `setStateInformation` (host session data) and, from Stage 3.2, from a UI where
a user types coordinates.

`NaN` itself is benign here by luck — `NaN > -100.0f` is false, so `decibelsToGain` returns `0.0f` and
the speaker goes silent rather than poisoning anything. **Do not rely on that**; it is one refactor
away from changing.

**Recommendation: sanitise at `publishSnapshot()`.** That function (`PluginProcessor.cpp:283-312`) is
the single funnel for everything the audio thread ever reads about the room, which makes it the exact
analogue of `snapshotParameters()` — one site, stated once, structurally impossible to bypass:

- `isfinite`-guard every float copied into the snapshot (positions, trims, rake, bbox, centroid,
  `rigScale`, `hullEpsCross`), falling back to the §OQ4 default;
- clamp `trimDb` to a stated range before conversion. **±24 dB** is the natural choice: it matches
  the hull trim's own −24 dB floor, and it comfortably contains FUNC-07's own criteria (−12 dB
  and +6 dB must both be reachable).

**Scope note, stated plainly.** The trim guard is unambiguously 2.3's, because FUNC-07's multiply is
what makes it exploitable. The *position* guard closes a pre-existing 2.2 hazard — a NaN speaker
coordinate already reaches `dbap::solve`, where `dRaw < kMinDistance` is false for NaN,
`denom < kDenomEpsilon` is false for NaN, and the NaN falls straight through to `setTargetValue`. It
is the same one-line loop at the same site, so folding it in costs nothing, but it **is** a scope
addition and should be recorded as a deviation rather than slipped in.

---

### H6 — `FirstOrderTPTFilter`'s default state is one element holding `2.0f`, and `G` is per-filter, not per-channel *(MEDIUM — two instances is mandatory, not stylistic)*

`std::vector<SampleType> s1 { 2 };` (`juce_FirstOrderTPTFilter.h:149`) is the **initializer-list**
constructor, not the size constructor. The default vector has **size 1, holding the value 2.0f** —
almost certainly not what upstream intended, and it means:

- an **unprepared** filter has one state element pre-loaded with 2.0f, so its first outputs decay
  from 2.0 rather than starting clean;
- `processSample (1, …)` on an unprepared filter is **out of bounds**.

Neither bites once `prepare()` is called (it does `s1.resize(numChannels)` then `reset()`), which is
the real lesson: **`prepare()` is mandatory, not merely advisable.** Q4's placement in step 2 of
`GainStage::prepare` covers it.

**Separately — two *instances* are required, and not for tidiness.** `G` is a **per-filter** member,
not per-channel. A single `FirstOrderTPTFilter<float>` prepared with `numChannels = 2` carries two
independent *states* but **one shared cutoff**. The two sub-points have different `d_hull` whenever
`width > 0` and the source straddles the hull, so they need different cutoffs. One 2-channel instance
would be silently wrong in exactly the configuration Q9 asks about. **Two mono instances, each
prepared with `numChannels = 1`.**

---

### H7 — `CONTEXT-2.3.md` constraint 9 is factually wrong: `juce_dsp` is already linked *(MEDIUM — a plan task that would be a no-op)*

Constraint 9 says *"`juce::juce_dsp` must be added to `target_link_libraries` — it is not linked
today."*

It is:

| target | line | state |
|---|---|---|
| `OuariconOctagon` (plugin) | `CMakeLists.txt:62` | **already linked**, and committed at `HEAD` (line 58 there) |
| `O-Octagon-render-test` | `tests/render-harness/CMakeLists.txt:118` | **already linked** |
| `O-Octagon-geometry-test` | — | **deliberately absent**, correctly (D1) |

**What is actually missing is the include, not the link.** `GainStage.h` includes only
`juce_audio_basics` (`:22`). Using `FirstOrderTPTFilter` needs `#include <juce_dsp/juce_dsp.h>` there.

**And that include is safe** — verified, not assumed: `tests/unit/main.cpp` does **not** include
`GainStage.h` (its only mention of the class is a prose comment at `:1590`), and nothing the unit
target compiles includes it either — the include direction runs `GainStage.h` → `DbapSolver.h` /
`SourceShaper.h`, never the reverse. D1's narrow link line survives.

---

### H8 — DSP-08's invariance is exact, but both clamps break it, and the default rig sits 0.9% from one of them *(MEDIUM — test design)*

The invariance is not approximate. Scaling every speaker coordinate **and the source position** about
the centroid by λ:

```
d_i → λ·d_i            t_i = d_i^(−a) → λ^(−a)·t_i
k = 1/sqrt(Σ (w_i t_i)²) → λ^(a)·k
v_i = k·w_i·t_i        → INVARIANT, exactly
```

`v_i` is homogeneous of degree 0 in λ. `r_s = blur · 0.5 · rigScale` scales by λ, which is what makes
the `d_i` scaling hold in the first place.

**The only scale-breaking terms are the two clamps** — `kMaxBlurMetres = 8.0` and
`kMinDistance = 0.05`. And the margin on the first one is thin:

| λ | blur | `r_s` wanted | `r_s` actual | |
|---|---|---|---|---|
| 1.0 | 1.00 | 3.966 m | 3.966 m | ok |
| **2.0** | **1.00** | **7.932 m** | **7.932 m** | **ok by 0.9%** |
| 2.1 | 1.00 | 8.328 m | 8.000 m | **clamped — invariance breaks** |
| 2.0 | 0.25 | 1.983 m | 1.983 m | ok, comfortable |

A DSP-08 probe written as "λ = 2, blur = 1" passes today and fails the moment `rigScale` moves —
and `rigScale` has already been corrected twice (7.95 → 7.93165). **Use `blur = 0.25` with
λ ∈ {0.5, 2.0}**, which leaves a 4× margin, and assert the invariance to `memcmp`-tight tolerance
rather than to a constant. `CONTEXT-2.3.md`'s warning about a mirrored `rigScale ≈ 7.93` fixture is
right, and this is the concrete form of it.

**Do not forget to scale the source position too.** Scaling only the speakers changes the geometry,
not its scale, and the probe would fail for a correct implementation.

---

### H9 — A probe that measures from sample 0 measures the filter's cold start, not the automation *(LOW, but it produced a wrong number during this research)*

The filter begins at `s = 0`. Against a sine that is already moving, the first two samples produce a
step **larger than the signal's steady-state maximum slew**:

```
measured max|Δout| from sample 0 : 0.067264   ← occurs at n = 2, the cold start
theoretical steady-state cap     : 0.065304
```

This produced a 3% over-reading in the first pass of this study before it was traced. Every probe
that measures `max |out[n] − out[n−1]|` on a filtered render **must discard a lead-in** — 2000
samples is ample (the pole at `fc = 20 kHz` decays 4.8 dB/sample at 48 kHz). Probe AS is unaffected
because its input is DC, but every new 2.3 probe on the filter path is affected.

---

### H10 — The `airActive` skip does not violate the exactly-once invariant, and QUAL-03 survives *(LOW — confirms a constraint rather than challenging it)*

Checked because it is the invariant most likely to be broken by accident:

- **`airActive` is decided at control rate**, inside `updateControl`, from `d_hull` and `airAmount`.
  Both are deterministic functions of the absolute sample position, so the skip decision at absolute
  sample *n* is identical at blockSize 512 and 4096. **QUAL-03 holds.**
- **The filter advances once per sample when active and not at all when skipped** — also
  deterministic, for the same reason. `CONTEXT-2.3.md` constraint 6 is right that the skip sits
  *outside* the seventeen-smoother rule; it does not create a second rule that can drift.
- **The dirty check cannot desynchronise it.** If `updateControl` short-circuits
  (`GainStage.cpp:137-140`), nothing that feeds `airActive` has changed, so leaving the flag resident
  is correct rather than merely harmless.
- **In SAFE mode the filter is not applied at all** (§5: "the dry input at unity"). The `reset(x)`
  edge flag from H1 must therefore be *consumed* in REAL mode and *left pending* in SAFE mode, or a
  mode flip mid-render leaves a stale pending reset. One line, but it is exactly the F3 window that
  `GainStage.cpp:252-256` already warns about.

---

## Reuse — existing code and precedent

| Thing | Where | Use at 2.3 |
|---|---|---|
| `hull::project().distance` | `ConvexHull2D.cpp:86-119` | **is** `d_hull`; return it from `solveSubPoint`, `0.0f` on the inside path |
| `shaper::shape()` | `SourceShaper.cpp:29-89` | complete; change only its caller's `0.0f` |
| `snapshot.trimLin[8]` | `VenueSnapshot.h:42`, populated `PluginProcessor.cpp:295` | complete; one multiply |
| Probe AS's structure | `render-harness/main.cpp:1498-1608` | the template for every derived-bound + negative-control probe |
| Probe AQ's mid-stream edit rig | `render-harness/main.cpp:1347-1384` | reusable for D4's live-edit probe, unmodified |
| `instr::` counters | `DbapSolver.h:61-77` | a fifth counter costs nothing; see Q2 |
| `bitExact()` helper | `render-harness/main.cpp:159`, `unit/main.cpp:143` | H1's entry edge and DSP-06/1 both need `memcmp`, not `near()` |
| §OQ4 defaults | `VenueModel.cpp:166` | the fallback set for H5's sanitisation |

---

## Pitfalls carried from the knowledge base

| Pattern | How it lands here |
|---|---|
| `pattern_envelope_follower_state_sticky_nan` | The air filter is the plugin's only recursive element. Q1 confirms the latch; H1 gives the second, free clearing path |
| `pattern_biquad_nan_guard_sticky_silence` | Does **not** apply verbatim: `G` is recomputed each control block, so `reset()` alone restores. Say so, or someone adds coefficient-preservation that does nothing |
| `pattern_test_fixture_mirrors_drift_silently` | DSP-08's `rigScale`, and Q6's tolerance. Both are derived in-probe rather than written down |
| `pattern_block_rate_envelope_breaks_blocksize_invariance` | H10 checks the skip against it explicitly |
| `pattern_grain_read_before_capture_write_blocksize` | H7's aliasing rule is unchanged: the filter inserts **after** `s_L`/`s_R` are read and before the gain matrix |
| `pattern_worktree_isolation_wrong_for_untracked_scope` | `stages/2-dsp/` is still untracked. **Do not execute 2.3 in a worktree** |
| `pattern_harness_param_leak_and_decay_window` | H9's lead-in is the "decay measured at the floor" failure in a new costume |
| `pattern_promotion_checksum_pins_replaced_file` | H2 recommends an erratum rather than a mid-phase re-pin, for exactly this reason |

---

## Open items handed to the plan phase

**Decisions the plan must make, with a recommendation attached:**

1. **H1 — adopt `reset(x)` at the `airActive` false→true edge?** *Recommend yes.* One float store;
   turns a 520%-of-slew click into a bit-exact handoff; satisfies D2's stated rationale better than
   D2's own choice; closes the skipped-filter NaN hole. Record as a deviation in `SUMMARY-2.3.md`.
2. **H5 — sanitise venue values at `publishSnapshot()`?** *Recommend yes for `trimDb` (in scope: the
   multiply is what arms it), and yes-with-a-recorded-deviation for the rest (out of scope, same
   line, pre-existing).* Clamp `trimDb` to ±24 dB.
3. **H4 — Nyquist-safe cutoff ceiling `min(20000, 0.45·fs)`?** *Recommend yes.* Not optional below
   40 kHz.
4. **H2 — correct the "0.7 dB @ 10 kHz" figure as an erratum, or re-pin `ARCHITECTURE.md`?**
   *Recommend erratum.* It is descriptive prose about a cost, not a specification the code
   implements. Correct `REQUIREMENTS.md` in place (not checksummed).
5. **`hullproc` vs. `juce::dsp::FirstOrderTPTFilter`.** §3.5.2 names the JUCE class, and D1 puts the
   arithmetic in JUCE-free free functions. A five-line `hullproc::OnePoleTPT` struct would be
   JUCE-free, allocation-free, trivially copyable, **state-inspectable** (closing Q1 directly rather
   than by equivalence), free of H4's `jassert` and H6's `{2}` quirk, and would move every DSP-07
   filter criterion into the **fast** unit target instead of the render harness. Against: it deviates
   from a named architecture choice. *No recommendation — this is a plan-phase call, and both answers
   are defensible.* If the JUCE class is kept, H4 and H6 become mandatory tasks rather than optional
   ones.

**Measurement obligations, now costed:**

6. **QUAL-01 criterion 1 (`airAmount`)** — assert `max|Δy| ≤ max|ΔG| · 2 · peak`, with `max|ΔG|`
   computed in-probe from the sweep schedule. Negative control at a 4096-sample update interval
   separates by ~48×. Measure on the **difference** of two renders, not on the raw render (Q6).
7. **QUAL-01 criterion 2 (hull crossing)** — two excitations as D3 requires, but the sine bound is
   `A · |H_20k(f) − 1|`, **predicted and asserted**, not a tolerance: 1.756e-2 at 1 kHz and 1.529e-1
   at 8 kHz (48 kHz, per unit amplitude). Entry edge asserts **bit-exact** continuity under H1.
8. **DSP-08** — λ ∈ {0.5, 2.0} at `blur = 0.25`, scaling the **source** as well as the speakers (H8).
9. **Every filter-path probe discards a ≥2000-sample lead-in** (H9).
10. **`instr::airCutoffUpdates == solveRuns · 2`** — the executable form of "not called per sample"
    (Q2). Do **not** route through `countedPow`; probe AE's `powCalls == 16` must stay exact.

**Confirmed, no work needed:** `juce_dsp` linkage (H7), `trimLin` plumbing (Q8), straddling
sub-points (Q9), live-edit alignment (Q10), §3.5.2's four-row `fc` table (H4), `d_hull` availability
and degenerate-count behaviour.

---

## Still-open manual gate

**D5's combined Logic session, at 2.3 verify** — 2.2's carried Task 12 (automate `srcX`, 8 lanes out
of lockstep; `w3 = 0` silences that lane) plus 2.3's items (width audibly spreads; air audibly dulls
outside the hull; a per-speaker trim moves one lane only). ~15 minutes, once, at the point where the
whole chain exists.

**One item to add to that session, from H2:** cross the hull boundary with **HF-rich material**, not
just a sine. The accepted cost of D2 is ~15% of an 8 kHz component as a one-sample step, which is the
figure that decides whether QUAL-01 criterion 2's "no *audible* discontinuity" is met. It is the one
claim in this phase that measurement can bound but only listening can settle.
