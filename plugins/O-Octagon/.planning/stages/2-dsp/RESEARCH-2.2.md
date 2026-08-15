# Stage 2 — DSP · Phase 2.2 (DBAP Solve and Gain Application) — Research

**Plugin:** O-Octagon
**Stage:** 2 of 4 — DSP
**Phase:** 2.2 of 3 — DBAP Solve and Gain Application
**GSD phase:** research
**Date:** 2026-08-11
**Branch:** `feat/o-octagon` @ `a47cef88`
**Depth:** DEEP (complexity tier 6)

---

## Entry Check — contract checksums

Re-computed at this boundary, not read out of `CONTEXT-2.2.md`
(`pattern_promotion_checksum_pins_replaced_file`). All four byte-exact against `STATUS.md`
frontmatter, **including the architecture pin re-issued at discuss by decision D2**:

| Contract | SHA-256 on arrival | Result |
|---|---|---|
| `BRIEF.md` | `697a4f32…f6b9fbd6` | ✅ matches |
| `parameter-spec.md` | `b45f88dc…cbb9e02f` | ✅ matches |
| `research/ARCHITECTURE.md` | `cd881a10…4b10861b` | ✅ matches the **new** pin |
| `ROADMAP.md` | `aec7d0ce…0137ee29` | ✅ matches |

No drift. The superseded architecture hash `bff8a83b…c406cfe` is recorded in frontmatter and is not
expected to match anything from here on.

---

## Scope discipline

Per `CONTEXT-2.2.md`, this research did **not** re-open §3.3 (constants, blur mapping, distance
floor, all-zero guard, `pow` budget), §3.4 (sub-point geometry), §3.6 (control grid, inner loop,
smoothing time) or §5 (processing order). Everything below is either (a) an answer to Q1–Q8, or
(b) a defect or hazard found *in the specification or in a Phase 2.1 deliverable* while verifying
those sections against JUCE 8.0.14 source and the code as it actually stands.

All JUCE claims are verified against the local tree at `/Users/taylorbrook/JUCE` (8.0.14) with
file:line references. Context7 was not consulted — local source is the stronger authority, matching
Stage 0/1/2.1 practice. Toolchain claims are verified by **running** the toolchain, not by recall.

---

## Answers to the Open Questions

### Q1 — How does the harness write a parameter at an exact absolute sample offset, synchronously?

**Answer: `apvts.getParameter(id)->setValueNotifyingHost (range.convertTo0to1 (realValue))`. It is
fully synchronous on the calling thread. No message loop is involved, no `AsyncUpdater`, no
`Timer`.**

The chain, verified end to end:

| Step | Location | What happens |
|---|---|---|
| 1 | `juce_AudioProcessorParameter.cpp:59-63` | `setValueNotifyingHost()` calls `setValue()`, then `sendValueChangedMessageToListeners()` |
| 2 | `juce_AudioProcessorParameter.cpp:111-121` | that iterates the listener list **synchronously**, under a `ScopedLock`, calling `parameterValueChanged()` on each |
| 3 | `juce_AudioProcessorValueTreeState.cpp:148-159` | `APVTS::ParameterAdapter::parameterValueChanged()` writes `unnormalisedValue` |
| 4 | `juce_AudioProcessorValueTreeState.cpp:118`, `:381-387` | `unnormalisedValue` **is** the object `getRawParameterValue()` returns a pointer to |

So the write lands in `srcXParam` *et al.* before `setValueNotifyingHost()` returns. The §3.6.3
protocol is deterministic.

**The trap, and it is the one that would make QUAL-03 pass vacuously.** `AudioProcessorParameter::
setValue()` on its own does **not** notify. `AudioParameterFloat::setValue()` assigns its own `value`
member and calls the empty `valueChanged()` hook (`juce_AudioParameterFloat.cpp`, one-liner). It never
reaches step 2, so **the cached atomic stays stale**. A harness written against `setValue()` would
render both block sizes against the default gain vector, get `memcmp` equality trivially, and report
QUAL-03 green having tested nothing. This is not hypothetical — `setValue()` is public and is the
more obvious-looking name of the two.

Also viable: `*floatParam = denormalisedValue` (`operator=`) routes through `setValueNotifyingHost`,
but it takes engineering units and early-outs on `approximatelyEqual`. Prefer the explicit form so
the normalisation is visible at the call site.

**What *is* deferred, and the harness must not read it.** The `ValueTree` property mirror is
timer-driven: `parameterValueChanged` only sets `needsUpdate` (`:158`), and `flushToTree` (`:120-141`)
runs from `APVTS::timerCallback` (`:472`), started by `startTimerHz (10)` (`:274`). In a console app
with no message loop **that timer never fires**, so `apvts.state.getProperty ("srcX")` returns a stale
value indefinitely. `copyState()` (`:389-393`) and therefore `getStateInformation()` call
`flushParameterValuesToValueTree()` explicitly, which is why 2.1's probes T and U are correct as
written. Rule for 2.2: assert against `getRawParameterValue()`, never against the raw tree.

**One second-order note.** Step 2 takes `listenerLock`, and step 3 calls its own listener list under a
`CriticalSection` (`LockedListeners`, `:179-204`). Two locks per write. Harmless between
`processBlock` calls; it is a hard prohibition on calling the setter from the audio thread.

**Repo precedent — reuse it verbatim.** `O-ReverseDelay/tests/render-harness/main.cpp:289-293`:

```cpp
static void setParam (juce::AudioProcessorValueTreeState& apvts, const char* id, float real)
{
    if (auto* p = apvts.getParameter (id))
        p->setValueNotifyingHost (p->getNormalisableRange().convertTo0to1 (real));
}
```

and its probe O (`:1689-1752`) drives exactly this pattern in a console app. That is empirical
confirmation on top of the source trace.

---

### Q2 — `earHeight()` on the audio thread

**Answer: free functions over the four already-published values, with `VenueModel` reduced to a thin
delegate. No new snapshot fields. This is the `hull::` precedent from 2.1, applied a second time.**

`ConvexHull2D.h:29-72` already established the shape and stated its own justification: *"Writing the
tests here and the members as thin delegates means the message-thread diagnostic and the audio-thread
solve provably run the SAME geometry — there is no second implementation to drift."* That argument
transfers without modification.

**Q2 understates the scope, and the missing half matters.** §5 step 2 needs the **bbox
denormalisation** on the audio thread too — `srcX`/`srcY` are normalised 0–1 (`PluginProcessor.cpp:69`)
and DBAP needs metres. `VenueModel::normToMetres()` is a message-thread method carrying its **own**
`kMinSpan` guard, deliberately separate from `earHeight()`'s (`VenueModel.cpp`, both guards commented
as independent: a rig with all eight speakers at one *x* has a fine rake and a zero-width bbox). So
2.2 faces **two** second-derivations, each with its own guard, not one.

Recommended shape — `Source/Data/VenueGeometry.h`, header-only, no JUCE dependency:

```cpp
namespace oo::plane
{
    inline constexpr float kMinSpan = 1.0e-6f;

    float earHeight    (float rakeFront, float rakeRear, float bbMinY, float bbMaxY, float y) noexcept;
    Vec2  normToMetres (float bbMinX, float bbMaxX, float bbMinY, float bbMaxY,
                        float nx, float ny) noexcept;
}
```

- `VenueModel::kMinSpan` becomes an alias of `oo::plane::kMinSpan`, so the public API and the on-disk
  contract are unchanged.
- `VenueModel::earHeight()` / `normToMetres()` become one-line delegates.
- A unit probe asserts member == free function over a swept set of `y` and `(nx, ny)` on a
  non-default venue. The "no second implementation" property becomes a **test**, not a claim.

**Rejected: precomputed slope/intercept in the snapshot.** It adds two fields whose consistency with
`rakeFront`/`rakeRear` must then be maintained, it encodes the zero-span guard implicitly (as
`slope = 0, intercept = rakeFront`) so the audio thread can no longer distinguish a flat rake from a
degenerate room, and — decisively — it does nothing for `normToMetres`, which needs the four bbox
values anyway. Half the problem would remain unsolved by a change that grows the snapshot.

---

### Q3 — `absoluteSampleCounter` lifecycle

**Answer: `std::uint64_t`, plugin-local, monotonic, reset in `prepareToPlay()` and nowhere else.
Never derived from the playhead.**

- **Why not the playhead.** A host locate would jump the grid and a loop would rewind it, making the
  control-update positions a function of the host's transport rather than of the plugin. QUAL-03's
  bit-identity would become both untestable and untrue, and hosts call `processBlock` while stopped
  anyway. The grid's whole purpose (§3.6.2) is to be decoupled from anything the host chooses
  arbitrarily; the transport is exactly such a thing.
- **Not reset in `releaseResources()`** (which a host may call and re-`prepareToPlay` around), **not
  touched by `setStateInformation()`** (it currently is not — keep it that way).
- **Overflow is a non-issue twice over.** 2⁶⁴ samples ≈ 12 million years at 48 kHz; and because 64
  divides 2⁶⁴ exactly, even the wrap preserves grid phase. Worth stating as a comment on the type
  choice so nobody "fixes" it to `int`.
- **`kControlBlock = 64` must be asserted a power of two**, per `pattern_ring_invariant_needs_static_assert`:
  `static_assert ((kControlBlock & (kControlBlock - 1)) == 0)`. Then `% kControlBlock` is
  `& (kControlBlock - 1)` and the modulo disappears.
- **QUAL-03 consequence:** both renders must `prepareToPlay()` first so both start at counter 0. The
  existing `negotiate()` helper (`tests/render-harness/main.cpp:82-96`) already does, and must be
  re-invoked per block size — the same thing O-ReverseDelay probe O does at `:1719-1721`.
- **Degenerate block sizes are safe.** `numSamples == 0` (pluginval issues these) leaves the `while`
  body unexecuted and the counter unchanged. A buffer larger than the prepared `samplesPerBlock`
  (pluginval strictness 10 issues these) is handled by the chunk loop with no allocation.

**A consequence of the reset point that changes every probe's first 240 samples.**
`SmoothedValue::reset (int numSteps)` calls `setCurrentAndTargetValue (target)`
(`juce_SmoothedValue.h:274-278`) — on a fresh object `target` is 0. Without an explicit initial solve,
**every render begins with a 5 ms fade-in from silence**. That is deterministic, so QUAL-03 still
passes, but it corrupts the lead-in of the unity, Layer-3 FFT and Σv²=1 probes unless each one
separately discards it.

Recommended `prepareToPlay` order: reset the counter → `reset (sr, 0.005)` on all 17 → run one
`updateControl()` → `setCurrentAndTargetValue()` on all 17 from that solve. Sample 0 is then already
correct, and no probe needs a lead-in for this reason (the tone's own start transient is a separate
matter — see Q7).

---

### Q4 — The dirty check's comparison

**Answer: `std::memcmp` over a `std::array<float, 17>`, not element-wise `!=`. And the NaN question
has a sharper answer than Q4 anticipated — see H2.**

- **`memcmp`, and the NaN behaviour is the reason, not a caveat.** Element-wise `!=` is true for NaN
  against itself, so once a host writes a NaN the solve would run every single control block forever.
  `memcmp` compares bit patterns, so NaN equals NaN and the skip keeps working. `-0.0f` vs `+0.0f`
  differ bitwise and cause one spurious re-solve — harmless, and worth one comment so it is not
  "fixed" later. The idiom already exists in both O-Octagon test mains as `bitExact()`
  (`tests/render-harness/main.cpp:72-75`, `tests/unit/main.cpp:123`), where it also serves to avoid
  `-Wfloat-equal`.
- **A host really can write a NaN — verified, not assumed.** `NormalisableRange::convertFrom0to1`
  clamps through `clampTo0To1` (`juce_core/maths/juce_NormalisableRange.h:162`, `:259-268`), which is
  `jlimit`. `jlimit` with NaN takes neither branch (`NaN < 0` is false, `1 < NaN` is false) and
  returns NaN (`juce_MathsFunctions.h:520-527`). The `jassert (exactlyEqual (clamped, value))` inside
  `clampTo0To1` fires in Debug and is compiled out in Release. So in a Release plugin a NaN reaches
  `AudioParameterFloat::value` and then the raw atomic unmodified.
- **Denormals** are ordinary distinct bit patterns; `memcmp` handles them and `ScopedNoDenormals`
  affects the arithmetic, not the comparison.
- **The venue generation counter must be part of the same check — but the current two-atomic form is
  racy. See H1.** The fix (stamp the generation inside the snapshot) also collapses the dirty check to
  a single acquire.

The mitigation for the NaN half is not in the dirty check at all: **sanitise the 17-float snapshot at
ingestion** (`p = std::isfinite (raw) ? raw : fallback`). 17 branches per 64 samples. It removes the
latch described in H2, removes any NaN/denormal anomaly from the dirty check, and leaves §3.3.4's
all-zero-weight guard doing the job it was actually designed for.

---

### Q5 — SAFE mode at 2.2

**Answer: run steps 1–7 in full in both modes, and advance all 17 `getNextValue()` calls exactly once
per sample in both modes. Only the per-sample *write* differs.**

Four reasons, in order of weight:

1. **§3.6.4's invariant is written "unconditionally".** A mode branch that skips `getNextValue()` is
   precisely the branch the section forbids. Making the invariant true only in REAL mode makes it a
   convention rather than an invariant, and the failure it guards against is a slow,
   position-dependent gain error that no single-parameter test finds.
2. **The F3 hazard can flip `mappedOutputAvailable()` between blocks** without an intervening
   `prepareToPlay()` — that is exactly the state probe S constructs (`tests/render-harness/main.cpp`
   probe S). Smoothers frozen for N blocks would resume from a stale `currentValue`. With the
   unconditional advance there is no stale state to resume from.
3. **auval exercises (1,1), (1,2), (2,1) and (2,2)** — JUCE derives those AU configs from
   `isBusesLayoutSupported()` (`PluginProcessor.cpp:152-182`, RESEARCH F2). RT-safety, the `pow`
   budget and the no-NaN property must hold there, and they are only *tested* there if the same code
   runs.
4. Stage 3's meters read the solve. Skipping it now means putting the branch back at 3.x.

Cost: 16 `pow` per control block and ~50 flops/sample discarded in a degenerate mode. Nothing.

**One thing to state rather than discover:** §5's SAFE-mode note says the per-sample stage writes *"the
dry input at unity"*. Taken literally, `outGain` is advanced but **not applied**, so the Output knob is
inert on a mono/stereo output bus. That is the contract as written and 2.2 should follow it; if it is
to change, it changes at a discuss boundary, not silently in the gain stage.

**The input-aliasing discipline is sharper at 2.2 than it was at 2.1 — see H7.**

---

### Q6 — `pow` instrumentation without polluting the shipping binary

**Answer: an `inline` counter plus a forwarding wrapper in `DbapSolver.h`, behind a compile-time
switch that only the test targets define. No macros needed.**

```cpp
namespace oo::instr
{
   #if OOCTAGON_INSTRUMENT
    inline std::atomic<std::uint32_t> powCalls  { 0 };
    inline std::atomic<std::uint32_t> solveRuns { 0 };
    inline std::atomic<std::uint32_t> hullProjections { 0 };
    inline float countedPow (float b, float e) noexcept
    { powCalls.fetch_add (1, std::memory_order_relaxed); return std::pow (b, e); }
   #else
    inline float countedPow (float b, float e) noexcept { return std::pow (b, e); }
   #endif
}
```

- The plugin target never defines `OOCTAGON_INSTRUMENT`, so **the counter objects do not exist** in
  the shipping binary and `countedPow` collapses to `std::pow` at the call site.
- The lever is already in use: both test targets compile the same plugin TUs with different
  definitions (`JUCE_WEB_BROWSER=0` at `tests/render-harness/CMakeLists.txt:80`, `tests/unit/
  CMakeLists.txt:71`). Adding `OOCTAGON_INSTRUMENT=1` to the two `target_compile_definitions` blocks is
  the whole mechanism.
- `inline` variables are C++17 and the repo is already there. No `constexpr`-inside-lambda, so
  `critical_msvc_constexpr_lambda_capture` is not in play.

**Three counters, not one.** `powCalls` closes PERF-02's ≤ 32 criterion (16 expected via the
`t = pow(d, −a)` reuse of §3.3.5). `solveRuns` turns PERF-02's *"skipped when unchanged — measured by
instrumentation, not asserted in prose"* into an actual measurement. `hullProjections` turns PERF-01
criterion 3 (*"hull projection only when outside"*) from a placement argument into a number.

---

### Q7 — FFT for channel-map Layer 3

**`juce_dsp` is already linked** — `tests/render-harness/CMakeLists.txt:109`, and by the plugin at
`CMakeLists.txt:58`. Nothing to add. The geometry unit target deliberately does not link it
(`tests/unit/CMakeLists.txt:74-85`) and should not start.

API: `juce::dsp::FFT fft (order)` (`juce_FFT.h:58`) and `performFrequencyOnlyForwardTransform (float*)`
(`:114`), which wants an array of `2 × size` floats.

**Making "the dominant bin is *exactly* its speaker's frequency" a true statement.** Use
**bin-centred** tones and a **rectangular** window:

- N = 4096 at fs = 48 000 → bin spacing 11.71875 Hz.
- `k ∈ {64, 128, 192, 256, 320, 384, 448, 512}` → 750, 1500, 2250, 3000, 3750, 4500, 5250, 6000 Hz.
- A bin-centred sinusoid is periodic in an N-sample rectangular window, so leakage is zero to float
  precision and the peak bin is exact. **Do not apply a Hann window** — it spreads energy into k±1 and
  turns "the dominant bin" into a three-bin argument.

**The level is exactly unity and should be asserted too.** With `w_j = δ_ij`,
`denom = w_j²·d_j^(−2a)`, so `k = d_j^a` and `v_j = k·w_j/d_j^a = 1` *exactly*. With a mono input and
two coincident sub-points (D1, `wEff = 0`), `out = v·0.5·(L + R) = 1·in`. So Layer 3 can assert
magnitude ≈ input magnitude, not merely "loudest bin".

Recommended structure — it satisfies ROADMAP's wording literally *and* adds the stronger half:

1. Eight renders, one per speaker *j*, each with `w = δ_ij` and the input tone at `f_j`.
2. **Per render:** assert lane `speakerToBuffer[j]` is loud and the other seven are below a floor.
   (This is the assertion that actually catches a permutation error.)
3. Accumulate the eight renders into one 8-channel buffer, then one FFT per lane: the dominant bin
   must equal the expected speaker's `k`. (This is ROADMAP's literal sentence.)

**C1 is binding here.** The venue must carry a **non-identity** label map, or `speakerToBuffer` is
`{0,1,2,3,4,5,6,7}` and the probe is byte-identical to a hardcoded map
(`critical_audiochannelset_is_a_bitset_not_an_order`). Reuse the permuted-label venue that probes D
and T already construct.

Discard the tone's start transient before the analysis window (≥ one control block); with Q3's
prepare-snap in place that is the only lead-in still required.

---

### Q8 — Fixture format and dimensions for D3

**Answer: a committed, generated C++ **header** is the source of truth. Confirmed — and the divergence
from Layer 2's build-time generation is a policy difference, not an inconsistency.**

| | Layer 2 golden | DBAP reference |
|---|---|---|
| Tracks | an **external moving target** (the JUCE tree) | a **fixed published equation set** (2011-04-14 revision) |
| Must regenerate when | JUCE changes — that *is* the gate | never |
| Generated | at build time, `DEPENDS` on the two JUCE files (`tests/unit/CMakeLists.txt:38-50`) | once, reviewed, committed |

Regenerating the oracle every build would make it a *build product*: a change to the solver plus a
matching change to the generator would agree silently, with nothing in the diff. Committed, it shows
up in review. And per 2.1's Q3 finding — **no test target in this repo has ever run in CI** — a
build-time regeneration would only ever execute for someone already running the tests, so it buys
nothing and costs a second `find_package(Python3 REQUIRED)`.

- Ship the generator beside it as `tests/tools/gen_dbap_reference.py`, modelled on
  `gen_juce_channel_order.py` (argument-driven, fails loud, deterministic output — its failure paths
  are already proven at SUMMARY-2.1 F5). Give it a `--check` mode and, optionally, a CMake target
  `O-Octagon-dbap-fixture-check` **excluded from `all`**.
- **A header, not a data file**: no runtime file I/O in the probe, no working-directory dependence in
  a console app whose cwd is whatever the caller had.
- **Contents (self-contained, per D3):** fixture version; the 8 speaker positions in metres;
  `rigScale` **as a literal in the fixture**; and per case `w[8]`, `rolloff`, `blur`, the source
  `(x, y, z)` in metres, and the expected `v[8]`. Store expected gains as **doubles** (`%.17g`) and
  compare the float solver's output promoted to double.

**A simplification worth taking.** If `DbapSolver` takes raw inputs —
`solve (const Vec3 spk[8], const float w[8], Vec3 src, float a, float rs, float outV[8])` — the probe
calls it directly and needs **no `VenueModel` at all**. D3's stated intent (*zero coupling to
`VenueModel`'s defaults*) is then satisfied more strongly than by constructing a `VenueModel` from the
fixture, and the DBAP probes can live in the fast `tests/unit/` target, whose link line has no
`juce_audio_processors` (`tests/unit/CMakeLists.txt:77-85`). `DbapSolver.cpp` would need only
`<cmath>` and `Vec.h`, so that narrow link line survives intact.

**Independence discipline.** The Python must implement eqs 9–10 from the revision, not transcribe the
C++. Concretely: compute `d^(−a)` as `exp (−a · log d)` rather than reusing the `t`/`t²` trick, and
normalise by explicitly forming `Σ v²` and dividing by its square root rather than by the
`k = 1/√denom` shortcut. An oracle that re-runs the implementation's own expression reproduces its
errors and passes forever — the same argument 2.1 F2 made for the hull oracle's ternary search.

**Tolerance — follow 2.1 F2 exactly.** Hard-assert at **1e-5**, print the measured worst deviation on
every run, and record in SUMMARY whether DSP-01's 1e-6 was met. The solver is single-precision, Clang
defaults to `-ffp-contract=on` for C++, and a hard gate pinned at 1e-6 invites a "fix" that is really
a tolerance edit.

**Case list** (all written into the fixture, positions from a pinned grid plus a pinned-seed
pseudorandom set): inside the hull; outside the hull; at hull vertices; at exact speaker coordinates
with `blur = 0`; both rolloff ends (3 and 6); both blur ends (0 and 1); one non-zero weight; two
non-zero weights; and all-zero weights.

---

## Findings beyond Q1–Q8

Numbered H1… continuing the F- (Stage 1) and G- (Phase 2.1) series.

### H1 — The venue generation counter and the snapshot are two separate atomics, so the dirty check can go permanently stale *(HIGH — defect in a Phase 2.1 deliverable)*

`VenueSnapshotPublisher::publish()` (`Source/Data/VenueSnapshot.h:92-102`) does:

```cpp
slots[target] = newSnapshot;
activeSlot.store (target, std::memory_order_release);
generation.fetch_add (1, std::memory_order_release);
```

and the audio thread reads them through **two separate acquires** — `read()` (`:106-109`) and
`getGeneration()` (`:112-115`).

If a `publish()` lands between those two reads, the control block observes the **new** geometry with
the **old** generation. It solves, stores that generation as `lastSolvedGeneration`, and from then on
every block compares equal: the venue edit is present in the snapshot and the solve never runs against
it. The gain vector stays wired to the pre-edit geometry until some unrelated *parameter* happens to
change. Reversing the read order swaps which half is stale (old snapshot, new generation → one wasted
solve, then a permanent skip against a snapshot that has since moved) and does not fix it.

**Fix:** stamp the generation **inside** `VenueSnapshot`. `publish()` writes `slot.generation` before
the release store on `activeSlot`; the audio thread's single acquire then delivers data and generation
together, and the dirty check reads `snapshot.generation`. Four lines, and it collapses two acquires to
one.

This deviates from ARCHITECTURE §3.6.6's field list — the same class of deviation as 2.1's
`hullEpsCross` (SUMMARY-2.1 deviation 1), accepted at 2.1 verify for the identical reason: the
alternative is an unsynchronised second read of a message-thread quantity.

**Detection matters as much as the fix.** This is invisible to any probe that edits the venue while
audio is stopped. The probe must publish a venue edit **between two `processBlock` calls** and assert
the gain vector has changed by the next control block.

### H2 — QUAL-02's sticky NaN has a second latch site, and it is reachable from a *parameter* *(HIGH — the architecture names the wrong element)*

ARCHITECTURE §3.5.2 states the TPT filter is *"the only recursive element in the plugin"*. It is not.
`SmoothedValue<float, Linear>` is recursive: `currentValue += step`
(`juce_SmoothedValue.h:383-390`). `setTargetValue (NaN)` sets `target = NaN` and, via `setStepSize()`
(`:372-375`), `step = NaN`. After that `currentValue` is NaN for the life of the object, with no
self-healing path — and unlike the air filter, this latch is reachable **before Phase 2.3 exists**.

The path is complete and every link is verified:

1. A host writes NaN → `clampTo0To1`/`jlimit` pass it through (Q4) → the raw atomic is NaN.
2. §3.3.4's guard does **not** catch it: `denom` is NaN and `NaN < kDenomEpsilon` is **false**, so the
   silence branch is skipped.
3. `k = 1/√NaN = NaN` → all 16 gain targets are NaN → 16 smoothers latch.

So QUAL-02's *"pathological input → no sticky NaN"* criterion must be tested against a **pathological
parameter**, not only a pathological input sample, and the mitigation belongs at parameter ingestion
(Q4), not in the solver.

### H3 — §5 step 2 is assigned to Phase 2.3, but 2.2 physically cannot solve without it *(MEDIUM — scope)*

ROADMAP puts `SourceShaper` (bbox denormalisation, bearing from the centroid, `rFade`, sub-point
construction) in Phase **2.3**. But DBAP needs the source position in **metres** and `srcX`/`srcY` are
normalised 0–1, so 2.2 must implement the bbox denormalisation and the per-sub-point rake resolution
regardless of which file they live in.

Recommend creating `Source/DSP/SourceShaper.{h,cpp}` **at 2.2**, in its D1-degenerate configuration
(`wEff` hardwired to 0, `width` unread), rather than inlining the two zero-span guards into
`GainStage` and extracting them at 2.3. This is D1's own argument applied one level up: the thing that
must not be written twice is the **guard**, not just the inner loop. 2.3's diff then really is *"make
`wEff` live"*, which is what D1 promises.

### H4 — The §OQ4 default venue is exactly mirror-symmetric, so a naive FUNC-01/3 probe fails at the default position *(MEDIUM — test design)*

From the §OQ4 table: (1, 2), (3, 8), (4, 7) and (5, 6) are exact mirrors about **x = 6.5 m**, which is
also `bbMinX + 0.5·(bbMaxX − bbMinX)` and the centroid *x*. At the default `srcX = 0.5` the puck lands
at exactly 6.5 m, so **those four pairs receive identical gains** — for a correct implementation. A
probe phrased *"all 8 channels differ"* would report FUNC-01/3 as failing.

Two consequences, and the second is a gift:

- The independence probe must use an off-centre source. `srcX = 0.18, srcY = 0.72` avoids every
  symmetry axis and every speaker coordinate.
- **The symmetry itself is a strong, non-vacuous correctness probe.** Assert the four pairs are
  bit-identical at `srcX = 0.5` and pairwise distinct off-centre. A dropped `z` term, a transposed
  speaker index or a sign error in the perpendicular breaks it, and unlike a distinctness check it
  cannot be satisfied by noise.

### H5 — DSP-04 criterion 3 needs its negative half or it cannot distinguish "wired in" from "something moved" *(MEDIUM — test design)*

`earHeight (bbMinY) = rakeFront` for **any** `rakeRear` (`VenueModel.cpp`, the linear form), so a
front-of-room source is provably invariant to `rakeRear`. Assert both halves: a **rear** source's gain
vector changes when `rakeRear` alone moves, **and** a source at `srcY = 0` does not. The first half
alone passes if any unrelated recompute happened to be triggered.

### H6 — QUAL-03's protocol is only executable if the automation offsets are multiples of the larger block size — and a stronger probe is nearly free *(MEDIUM)*

§3.6.3 says *"control-grid-aligned absolute sample offsets"*. A write at absolute sample 1024 cannot be
performed between `processBlock` calls in a fixed-4096 render without splitting the call — and
splitting it changes the block size, which is the variable under test. Since 4096 is a multiple of both
512 and 64, **restricting the automation events to multiples of 4096 makes the protocol literally
executable at both sizes.**

The test remains meaningful at that spacing: under a per-block solve the 512 render sets targets 8×
more often *between* events, which is exactly the divergence QUAL-03 exists to catch
(`pattern_block_rate_envelope_breaks_blocksize_invariance`).

**A strictly stronger second probe costs almost nothing.** Drive one render with a **ragged block-size
sequence** (e.g. repeating 1, 7, 64, 333, 4096) against a fixed-4096 render, with events at arbitrary
offsets. Because the harness chooses where each call ends, any offset is reachable — and the grid walk
makes the result invariant even for **non**-64-aligned events, since the first control boundary at or
after absolute sample S is the same absolute sample in both renders. Passing that demonstrates
block-size invariance over a far wider space than two fixed sizes.

Recommend both: 512-vs-4096 with events at 4096 multiples for literal ROADMAP compliance, and the
ragged sequence as the real gate.

**The excitation must be position-deterministic** — a function of the absolute sample index, not a
sequential generator. `renderEffect`-style whole-block fills consume a sequential RNG a different
number of times at a different block size, and the two runs then do not share an input signal at all.
O-ReverseDelay makes exactly this point at `tests/render-harness/main.cpp:1706-1712` and supplies a
usable hash-based `noiseAt(t)`.

### H7 — The input-aliasing discipline is sharper at 2.2 than at 2.1 *(MEDIUM)*

With a mono input bus and a 7.1 output bus the buffer has 8 channels and `out[0]` aliases `in[0]`. At
2.1 every lane received the same value, so an accidental channel-major write was survivable. At 2.2
`out[speakerToBuffer[i]]` differs per *i*, and no scratch buffer is available (PERF-01 forbids the
allocation), so **`s_L` and `s_R` for sample *n* must be read before any output is written for sample
*n***. §3.6.4's loop already has this shape; the hazard is a plausible-looking "optimisation" that
hoists `const float* in0 = buffer.getReadPointer (0)` out and writes channel-major. Same family as
`pattern_grain_read_before_capture_write_blocksize`, and it would pass at blockSize 1.

### H8 — PERF-01's "no allocation" can be measured here; RTSan cannot *(MEDIUM)*

`clang++ -fsanitize=realtime` is **rejected** by the toolchain in use — Apple clang 17.0.0 (Xcode 26.3)
reports `unsupported argument 'realtime'`. Verified by running it, not by recall. RealtimeSanitizer is
not an option on this machine.

The portable alternative is a **replaced global allocation function** in the harness TU: replace
`operator new`, `operator new[]`, both `std::align_val_t` overloads and every matching `operator
delete` with `std::malloc`/`std::free` wrappers that bump a counter; arm the counter immediately before
a `processBlock` call and assert it did not move.

Two disciplines make it honest rather than decorative:

- **Replace every variant.** An un-replaced aligned-new is silently uncounted, and a probe that counts
  nothing passes.
- **Warm up first.** Call `processBlock` once before arming, so first-touch lazy initialisation inside
  libc++ or JUCE is not attributed to the audio path.

**Locks and file I/O stay grep + inspection at 2.2.** Say that plainly in SUMMARY rather than implying
the whole of PERF-01 criterion 1 is instrumented.

### H9 — `SmoothedValue::reset()` is a state reset, not a ramp-length setter *(LOW, but it silently defeats QUAL-01)*

`reset (int numSteps)` calls `setCurrentAndTargetValue (this->target)` (`juce_SmoothedValue.h:274-278`),
teleporting `currentValue` to `target` and zeroing the countdown. Calling `reset (sr, 0.005)` anywhere
other than `prepareToPlay` would jump all 17 gains — and **QUAL-03 would still pass**, because both
renders teleport identically. Only QUAL-01's "no discontinuity" would catch it, and only if a probe
happened to straddle the call. One reset site, in `prepareToPlay`, and nowhere else.

### H10 — The 5 ms ramp never completes during motion; do not write a probe that assumes it does *(LOW)*

`stepsToTarget = (int) floor (0.005 × sr)` = 240 at 48 kHz (`juce_SmoothedValue.h:265-269`), while a new
target arrives every 64 samples. `setTargetValue` resets `countdown` to 240 and recomputes
`step = (target − currentValue)/240` (`:284-303`, `:372-375`), so during a sweep the gain **chases**
the solved value rather than reaching it — an exponential-ish lag of roughly the ramp length. This is
deterministic and is exactly what §3.6.5 intends (*"comfortably longer than the 1.33 ms control
grid"*), but it means a probe must not assert that the rendered gain equals the solved gain **while a
parameter is moving**. Compare only after ≥ 240 samples of stillness. QUAL-04's max-per-sample-delta
bound is the right shape and is unaffected.

---

## Reuse — existing code and precedent

| Source | Reuse | Notes |
|---|---|---|
| `O-ReverseDelay/tests/render-harness/main.cpp:289-293` | `setParam()` verbatim | The only synchronous write path (Q1) |
| `O-ReverseDelay/tests/render-harness/main.cpp:1689-1752` | Block-size probe skeleton | Note it uses a **tolerance**; O-Octagon's QUAL-03 demands `memcmp` |
| `O-ReverseDelay/tests/render-harness/main.cpp:1706-1712` | Position-deterministic `noiseAt(t)` | Load-bearing for H6 |
| `O-ReverseDelay/Source/PluginProcessor.cpp:2080-2093` | The chunked control-grid loop idiom | Countdown form; provably equivalent to the contract's modulo form given a fixed *K* and one reset site. **Use the contract's form** — it is self-resynchronising and lets the harness compute the next boundary |
| `Source/DSP/ConvexHull2D.h:29-72` (`hull::` namespace) | The free-function-over-raw-storage pattern | Q2 applies it a second time; `hull::isInside`/`project` are called directly by the solver |
| `Source/PluginProcessor.h:163-178` (`mappedOutputAvailable`) | Called, never re-derived | G1 carried forward verbatim |
| `tests/{unit,render-harness}/main.cpp` `check()` / `bitExact()` | Probe idiom | No framework — `project_no_unit_test_framework_ci_never_runs_tests` |
| `tests/tools/gen_juce_channel_order.py` | Template for the DBAP generator | Fail-loud, argument-driven, byte-deterministic (SUMMARY-2.1 F5) |
| `modules/registry.yaml` | **No applicable module** | Re-checked all eight categories (core, persistence, metering, tuning, modulation, synthesis, effects, ui). Nothing covers spatialisation, DBAP, or a control grid. Nothing to `/module-add` |

`juce_dsp` is already linked by the plugin (`CMakeLists.txt:58`) and the render harness
(`tests/render-harness/CMakeLists.txt:109`). **No new third-party dependency, no new module, and no
change to `build-and-release.yml` is required by this phase.**

---

## Pitfalls carried from the knowledge base

| Pattern | Bites where |
|---|---|
| `pattern_block_rate_envelope_breaks_blocksize_invariance` | The whole of §3.6. H6 is its executable form |
| `pattern_envelope_follower_state_sticky_nan` | **H2** — extended: the latch is a `SmoothedValue`, not the filter, and the trigger is a parameter |
| `pattern_test_fixture_mirrors_drift_silently` | Q8 — the oracle must not transcribe the C++, and the fixture must stay committed and reviewable |
| `pattern_ring_invariant_needs_static_assert` | `kControlBlock` power-of-two; §3.3.1's four constants |
| `pattern_grain_read_before_capture_write_blocksize` | **H7** — read-before-write per sample, no hoisted read pointer |
| `critical_audiochannelset_is_a_bitset_not_an_order` (C1) | Q7 — Layer 3 must drive a **non-identity** label map or it is vacuous |
| `pattern_standalone_canonical_channelset_oob` (G1) | `GainStage` **calls** `mappedOutputAvailable()`; never bounds by `getTotalNumOutputChannels()` |
| `pattern_dbap_not_vbap_for_irregular_arrays` | The 2011-04-14 revision only. The original's eqs 3–6 and 9–10 are wrong |
| `pattern_worktree_isolation_wrong_for_untracked_scope` | `stages/2-dsp/` is untracked. **Do not execute 2.2 in an isolated worktree** — every gate would pass vacuously |
| `pattern_rng_stream_interleave_blocksize` | No RNG in the DSP at 2.2, but the harness excitation must be a function of the absolute sample index (H6) |
| `pattern_render_harness_breaks_on_webview_editor` | Guard already in place at `PluginProcessor.cpp:383-387`; do not remove it |
| `build_script_target_name_vs_folder` | Target is **`OuariconOctagon`**, folder is `O-Octagon` |
| `critical_msvc_constexpr_lambda_capture` | Not exercised by Q6's design; flag on first Windows CI add |

---

## Open items handed to the plan phase

1. **`Source/Data/VenueGeometry.h`** — free `earHeight` / `normToMetres` plus the shared `kMinSpan`,
   with `VenueModel` delegating and a unit probe asserting member == free function (Q2).
2. **Create `SourceShaper` at 2.2 in D1-degenerate form**, or inline the two zero-span guards into
   `GainStage` and extract at 2.3. Recommendation: **create it** (H3).
3. **Stamp the generation inside `VenueSnapshot`** (H1). This edits a Phase 2.1 file; confirm it is in
   2.2's scope rather than a deferred repair. Recommendation: **in scope** — 2.2 is the first phase
   whose correctness depends on it.
4. **Parameter sanitiser at ingestion** (Q4 / H2) — confirm the fallback: the parameter's declared
   default, or a clamp into its declared range. Recommendation: **default**, because a NaN carries no
   information about which end to clamp to.
5. **SAFE-mode `outGain`** — the contract says the dry input is written at unity, so the Output knob is
   inert on a mono/stereo bus. Confirm, or escalate to a discuss decision (Q5).
6. **Where the DBAP probes live** — `tests/unit/` (fast, no `juce_audio_processors`) requires
   `DbapSolver` to take raw inputs. Recommendation: **unit target**, and shape the solver accordingly
   (Q8).
7. **`OOCTAGON_INSTRUMENT=1`** on both test targets, and the three counters (`powCalls`, `solveRuns`,
   `hullProjections`) (Q6).
8. **Block-size probe matrix** — 512 vs 4096 with events at multiples of 4096 (mandated), plus the
   ragged-sequence probe (recommended) (H6).
9. **The fixture's committed location and case list**, and whether `--check` gets an
   excluded-from-`all` CMake target (Q8).
10. **`prepareToPlay` ordering** — counter reset, `reset(sr, 0.005)`, one `updateControl()`, then
    `setCurrentAndTargetValue()` on all 17 (Q3).
11. **Do not execute 2.2 in an isolated worktree.**

## Still-open manual gate

**The D4 Logic surround check (~10 min) runs before 2.2 verify closes**, as Task 13 did for Stage 1:
(a) automate `srcX` and confirm the 8 surround-meter lanes no longer move in lockstep; (b) set
`w3 = 0` and confirm that lane goes silent while the others compensate. Nothing in this research
depends on its result, and it is corroboration for FUNC-01/3 — the gate itself is Layer 3 (D4
consequence, CONTEXT-2.2).
