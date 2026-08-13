# Stage 2 — DSP · Phase 2.2 (DBAP Solve and Gain Application) — Plan

**Plugin:** O-Octagon
**Stage:** 2 of 4 — DSP
**Phase:** 2.2 of 3 — DBAP Solve and Gain Application
**GSD phase:** plan
**Date:** 2026-08-11
**Branch:** `feat/o-octagon` @ `a47cef88`
**Inputs:** `CONTEXT-2.2.md`, `RESEARCH-2.2.md`, `research/ARCHITECTURE.md` §2/§3.3/§3.4/§3.6/§5/§OQ3/§OQ4,
`ROADMAP.md` Phase 2.2, `REQUIREMENTS.md`, `PLAN-2.1.md` (P5–P13 inherited)

---

## Entry Check — contract checksums

Re-computed at this boundary, not read out of `RESEARCH-2.2.md` (C6,
`pattern_promotion_checksum_pins_replaced_file`). All four byte-exact against `STATUS.md` frontmatter:

| Contract | SHA-256 | Result |
|---|---|---|
| `BRIEF.md` | `697a4f32…f6b9fbd6` | ✅ |
| `parameter-spec.md` | `b45f88dc…cbb9e02f` | ✅ |
| `research/ARCHITECTURE.md` | `cd881a10…4b10861b` | ✅ matches the **D2 re-pin** |
| `ROADMAP.md` | `aec7d0ce…0137ee29` | ✅ |

No drift. This plan is written against these exact documents. The superseded architecture hash
`bff8a83b…c406cfe` is expected to match nothing from here on.

---

## Goal

**Audio spatialises.** The DBAP solver, the 64-sample absolute-sample-aligned control grid, the 17
smoothed gains and the per-sample inner loop land together, so that for the first time the 8 output
lanes carry *different* signal — and every one of them is written through `speakerToBuffer`.

At the end of this phase §5 steps 1, 2, 3, 4, 5 and 7 are live. Step 6 (hull trim, air LPF) and the
trim fold are 2.3. `width` is present but not read (D1): the two sub-points coincide, so the plugin
ships the **general path in its degenerate configuration**, not a stub.

### §5 step map for this phase — what is live and what is deliberately absent

| §5 step | 2.2 | Note |
|---|---|---|
| 1. Snapshot 17 atomics, dirty check vs venue generation | ✅ | + parameter sanitiser (P17) |
| 2. Bbox-denormalise `srcX`/`srcY` → metres | ✅ | H3 — `SourceShaper`, pulled forward (P15) |
| 3. Sub-points, `rFade`, per-sub-point `earHeight` | ✅ | `wEff` forced to 0 by D1 |
| 4. Hull inside/outside per sub-point + projection | ✅ | calls 2.1's `hull::` free functions |
| 5. DBAP solve per sub-point (3D) | ✅ | `DbapSolver` |
| 6. Hull gain trim + air LPF | ❌ **2.3** | `PHASE-2.3-AIR` marker at the insertion point |
| 7. Fold venue trim, set 17 targets | **partial** | targets set; **trim NOT folded** (FUNC-07 → 2.3), `PHASE-2.3-TRIM` marker |
| Per-sample inner loop, exactly-once invariant | ✅ | §3.6.4 verbatim, written **once** |
| Verify-ping override, metering | ❌ Stage 3 | — |

---

## Requirement staging — read this before writing the verify report

CONTEXT-2.2 lists eleven requirement lines for this phase. **One cannot fully close here**, and it
was named at *discuss*, one phase earlier than 2.1 managed. Nothing below may be discovered at verify.

| Req | Criterion | Closes at |
|---|---|---|
| **FUNC-01** | 1–2 met at stage-1 | — |
| | 3. All 8 outputs carry independent, non-duplicated signal for an off-centre source | **2.2** (probes AI + AJ) |
| | → **FUNC-01 closes ✅ complete at 2.2** | |
| **DSP-01** | 1. `a = R/(20·log10 2)` at R = 3, 4, 6 | **2.2** (X) |
| | 2. `d_i` carries `(z_i − z_s)²` — `srcZ` alone changes the gain vector | **2.2** (Z) |
| | 3. Matches an independent reference of eqs 9–10 to 1e-6 | **2.2** (Y) |
| **DSP-02** | 1–3. `Σ v_i² = 1 ± 1e-6` inside / outside / vertices / speaker positions, over rolloff × blur | **2.2** (AA) |
| **DSP-05** | 1–3. `w_i = 0` → exact zero; 2-speaker subset holds `Σ v² = 1`; all-zero → silence | **2.2** (AB, AC) |
| **PERF-01** | 1. No allocation, lock or file I/O in `processBlock` | **2.2** (AO) — **see the H8 honesty note below** |
| | 2. `pow` on parameter change, not per sample | **2.2** (AE, AP) |
| | 3. Hull projection only when outside | **2.2** (AP) |
| **PERF-02** | 1–4. Grid, skip-when-unchanged, ≤ 32 `pow`, exactly-once `getNextValue()` | **2.2** (AE, AP, AT) |
| **QUAL-02** | 1–4. Finite everywhere; no sticky NaN under pathological **input and parameter** | **2.2** (AD, AR) |
| **QUAL-03** | 1–3. Bit-identity under the §3.6.3 protocol, `memcmp` | **2.2** (AL, AM, AN) |
| **QUAL-04** | 1. Position sweep ≤ smoother max per-sample delta | **2.2** (AS) |
| | 2. Weight sweep, including through exact zero | **2.2** (AS) |
| | 3. *`width` sweep* | **2.3 — `width` is wired to nothing at 2.2 (D1); the probe would pass vacuously** |
| | → **QUAL-04 is ⚠️ PARTIAL at 2.2**, declared at discuss | |
| **DSP-04 /3** | Changing `rakeRear` alone changes a **rear** source's gain vector | **2.2** (AK) → DSP-04 ✅ complete |
| **FUNC-03 /3** | Changing a label row moves audio to that physical output | **2.2** (AJ) → FUNC-03 ✅ complete |

**PERF-01 criterion 1 — state the method, not a verdict (H8).** `-fsanitize=realtime` is
**unsupported** by Apple clang 17.0.0 (verified by running it). Allocation is *measured* by a
replaced global `operator new` family in the harness TU. **Locks and file I/O are grep + inspection.**
SUMMARY-2.2 must say exactly that; writing "RT-safety harness pass ✓" without the qualifier would
overstate what ran.

**DSP-08 is implemented here but does NOT close here.** ROADMAP 2.2 builds the blur→`r_s` mapping and
DSP-02 needs it, but the traceability table assigns DSP-08 to 2.3. Probe AG exercises it as
supporting evidence; the requirement stays `pending` until 2.3. Do not tick it.

**Action for the executor:** do not attempt to close QUAL-04/3, and do not report it as closed.
`REQUIREMENTS.md` is updated at *verify*.

---

## Plan Decisions

Continuing the P-series (Stage 1: P1–P4; Phase 2.1: P5–P13). Each of RESEARCH-2.2's eleven open items
is resolved below; item 11 (worktree) is an execution constraint rather than a decision.

### P14 — `Source/Data/VenueGeometry.h`: free functions, `VenueModel` delegates *(Q2, item 1)*

Header-only, `inline`, no JUCE dependency. The `hull::` precedent (`ConvexHull2D.h:29-72`) applied a
second time — **and Q2 understated the scope: `normToMetres` needs it too, with its own guard.**

```cpp
namespace oo::plane
{
    inline constexpr float kMinSpan = 1.0e-6f;      // the single definition

    inline float earHeight    (float rakeFront, float rakeRear,
                               float bbMinY, float bbMaxY, float y) noexcept;
    inline Vec2  normToMetres (float bbMinX, float bbMaxX, float bbMinY, float bbMaxY,
                               float nx, float ny) noexcept;
}
```

- `VenueModel::kMinSpan` becomes `= oo::plane::kMinSpan`. The public API and the on-disk contract are
  unchanged; there is now one definition instead of two guards that *happen* to agree.
- `VenueModel::earHeight()` / `normToMetres()` / `absoluteHeight()` become one-line delegates.
- **Both zero-span guards survive independently** — a rig with all eight speakers at one *x* has a
  fine rake and a zero-width bbox, so collapsing them into one is wrong.
- Probes V and W assert **member == free function** over a swept set on a *non-default* venue. "No
  second implementation to drift" becomes a test rather than a claim.

**Rejected: precomputed slope/intercept in the snapshot.** It adds two fields whose consistency with
`rakeFront`/`rakeRear` must be maintained, encodes the zero-span guard implicitly so the audio thread
can no longer distinguish a flat rake from a degenerate room, and does nothing at all for
`normToMetres` — half the problem left unsolved by a change that grows the snapshot.

### P15 — `SourceShaper` is created **at 2.2**, in D1-degenerate form, driven by a literal `0.0f` *(H3, item 2)*

ROADMAP assigns `SourceShaper` to 2.3, but DBAP needs metres and `srcX`/`srcY` are normalised, so 2.2
must implement §5 step 2 and the per-sub-point rake resolution regardless of which file they live in.
Inlining them into `GainStage` and extracting at 2.3 would write the two zero-span guards twice —
D1's own argument, one level up.

**`SourceShaper::shape()` implements §3.4.1 in full, steps 1–6, including `rFade` and the `(0,−1)`
fallback bearing.** What makes it degenerate is the *caller*: `GainStage::updateControl()` passes a
literal `0.0f` for `width` and never reads `widthParam`, at a line carrying the greppable token:

```cpp
const float widthMetres = 0.0f;   // PHASE-2.3-WIDTH: read widthParam here; DSP-06 / QUAL-04 crit 3
```

This is the Stage-1 `PHASE-2.2-REPLACE` mechanism, which worked: 2.2's gates assert the token appears
**exactly once**, and 2.3's gates assert it appears **zero** times — retired, never grandfathered.

Consequence worth stating: at `width = 0`, `wEff = 0`, the sub-points coincide, `v_L ≡ v_R`
bit-for-bit, and the sum degenerates to `v_i · 0.5·(L+R)` with **no branch** (§3.4.3). The 2.3 diff
really is one line plus the QUAL-04/3 probe.

Because `shape()` is complete, probe AH can drive `width > 0` **directly** in the unit target. That is
coverage for code this phase ships inert — it is **not** a claim on DSP-06, which closes at 2.3.

### P16 — Stamp the generation **inside** `VenueSnapshot` *(H1, item 3 — IN SCOPE, edits a 2.1 file)*

`VenueSnapshot.h:92-115` publishes the slot and the generation as **two separate atomics**, read
through **two separate acquires**. A `publish()` landing between those reads gives the control block
the **new** geometry with the **old** generation; that generation is stored as `lastSolvedGeneration`
and every subsequent block compares equal — **the venue edit is present in the snapshot and the solve
never runs against it, permanently.** Reversing the read order only swaps which half is stale.

```cpp
struct VenueSnapshot { … std::uint32_t generation { 0 }; };   // stamped by publish()

void publish (const VenueSnapshot& s) noexcept
{
    const int target = 1 - activeSlot.load (std::memory_order_relaxed);
    slots[(size_t) target] = s;
    slots[(size_t) target].generation = ++publishCounter;      // message thread only, not atomic
    activeSlot.store (target, std::memory_order_release);      // ONE release
}
```

The audio thread's single acquire in `read()` now delivers data and generation together and the dirty
check reads `snapshot.generation`. Two acquires collapse to one. `getGeneration()` is retained for the
message thread / diagnostics or deleted — it must **not** be what the dirty check reads.

This deviates from ARCHITECTURE §3.6.6's field list, the same class of deviation as 2.1's
`hullEpsCross` and accepted at 2.1 verify for the identical reason. Record it in SUMMARY-2.2 as a
deviation, not as an unremarked edit.

**Detection matters as much as the fix.** This is invisible to any probe that edits the venue while
audio is stopped. Probe AQ publishes a venue edit **between two `processBlock` calls** and asserts the
gain vector has moved by the next control block.

### P17 — Sanitise the 17-float snapshot at ingestion; the fallback is the parameter's **declared default** *(Q4 / H2, item 4)*

H2's latch is real and reachable **from a parameter**, before 2.3 exists: `jlimit` passes NaN through
(`juce_MathsFunctions.h:520-527`), the `jassert` inside `clampTo0To1` is Debug-only, §3.3.4's guard
misses it (`NaN < kDenomEpsilon` is **false**), and `SmoothedValue::setTargetValue(NaN)` sets
`step = NaN` — after which `currentValue` is NaN for the life of the object with no self-healing path.
ARCHITECTURE §3.5.2's claim that the TPT filter is *"the only recursive element"* is wrong.

```cpp
p[k] = std::isfinite (raw[k]) ? raw[k] : paramDefaults[k];    // 17 branches per 64 samples
```

**`paramDefaults` is derived, never transcribed.** Populate it in the constructor beside the atomic
cache, from the parameter objects themselves:

```cpp
paramDefaults[k] = range.convertFrom0to1 (apvts.getParameter (id)->getDefaultValue());
```

A hand-written table of 17 defaults is `pattern_test_fixture_mirrors_drift_silently` with a
`parameter-spec.md` gate that would not catch it (that gate compares the *parameters*, not this array).
The default is the right fallback because **a NaN carries no information about which end to clamp to**.

This also removes any NaN/denormal anomaly from the dirty check and leaves §3.3.4's all-zero guard
doing the job it was designed for.

### P18 — SAFE mode: steps 1–7 run, all 17 advance, `outGain` is **not** applied *(Q5, item 5)*

Confirmed as the contract reads — not escalated. §5's SAFE-mode note says the per-sample stage writes
*"the dry input at unity"*, so on a mono/stereo output bus the Output knob is **inert**. That surprises
a reader, so it gets a comment naming it as contract-mandated, and probe AT asserts it. If it is to
change it changes at a discuss boundary, not silently in the gain stage.

Everything else runs in both modes, per §3.6.4's *unconditionally*:

1. A mode branch that skips `getNextValue()` is precisely the branch the section forbids.
2. The F3 hazard flips `mappedOutputAvailable()` between blocks with no intervening `prepareToPlay()`
   — the state probe S already constructs. Frozen smoothers would resume from a stale `currentValue`.
3. `auval` exercises (1,1), (1,2), (2,1), (2,2), so the `pow` budget and the no-NaN property are only
   *tested* there if the same code runs. This is load-bearing for COMPAT-01.

Cost: 16 `pow` per control block and ~50 flops/sample discarded in a degenerate mode.

### P19 — `DbapSolver` takes **raw inputs**; the DBAP probes live in the fast unit target *(Q8, item 6)*

```cpp
namespace oo::dbap
{
    void solve (const Vec3 spk[8], const float w[8], Vec3 src,
                float a, float rs, float outV[8]) noexcept;
}
```

No `VenueModel`, no `VenueSnapshot`, no JUCE — `DbapSolver.cpp` includes only `<cmath>`, `<atomic>`
and `Vec.h`. The probe then calls it directly against the fixture, which satisfies D3's stated intent
(*zero coupling to `VenueModel`'s defaults*) **more strongly** than constructing a `VenueModel` from
the fixture would, and `tests/unit/`'s narrow link line (no `juce_audio_processors`) survives intact.

§3.3 is implemented verbatim: the four `static_assert`ed constants, the unconditional `kMinDistance`
floor on every path, the `denom < kDenomEpsilon` → **explicit zeros** silence branch, and the
`t = pow(d, −a)` reuse so it is 8 `pow` per sub-point rather than 16.

### P20 — `OOCTAGON_INSTRUMENT=1` on both test targets; **four** counters *(Q6, item 7)*

`inline` counters plus a forwarding `countedPow` in `DbapSolver.h`, behind a compile-time switch only
the test targets define. The plugin target never defines it, so the counter objects **do not exist**
in the shipping binary and `countedPow` collapses to `std::pow` at the call site. The lever is already
in use — both test CMakeLists already compile the plugin TUs with `JUCE_WEB_BROWSER=0`.

| Counter | Closes |
|---|---|
| `powCalls` | PERF-02/3 — ≤ 32 per control block (16 expected) |
| `solveRuns` | PERF-02/2 — *"skipped when unchanged, measured by instrumentation, not asserted in prose"* |
| `hullProjections` | PERF-01/3 — *"hull projection only when outside"* becomes a number, not a placement argument |
| **`sampleAdvances`** | **PERF-02/4** — incremented once per iteration of the §3.6.4 inner loop |

Research named three. The fourth is added because **PERF-02 criterion 4 is otherwise unmeasurable**:
`sampleAdvances == totalSamplesRendered` in **both** modes is the executable form of *"exactly once
per sample, unconditionally"*. It is paired with an instrumentation-gated accessor returning the 17
`getCurrentValue()`s, so probe AT can assert all 17 have *arrived* after 240 still samples — a
desynchronised smoother has not.

### P21 — Block-size matrix: the mandated pair **plus** the ragged sequence *(H6, item 8)*

§3.6.3 requires control-grid-aligned absolute offsets. A write at absolute sample 1024 cannot be
performed between `processBlock` calls in a fixed-4096 render without splitting the call — and
splitting it changes the variable under test. Since 4096 is a multiple of both 512 and 64:

- **AL (mandated, literal ROADMAP compliance):** 512 vs 4096, automation events at multiples of 4096,
  `memcmp` — not a tolerance. Still meaningful: the 512 render sets targets 8× more often *between*
  events, which is exactly the divergence QUAL-03 exists to catch.
- **AM (the real gate):** a **ragged** block-size sequence — repeating `1, 7, 64, 333, 4096` — against
  a fixed 4096 render, with events at **arbitrary** offsets. The harness chooses where each call ends,
  so any offset is reachable, and the grid walk makes the result invariant even for non-64-aligned
  events because the first control boundary at or after absolute sample *S* is the same absolute
  sample in both renders. This demonstrates invariance over a far wider space than two fixed sizes.

**The excitation must be position-deterministic** — a function of the absolute sample index, never a
sequential generator, or the two runs do not share an input signal at all
(`pattern_rng_stream_interleave_blocksize`). The harness's existing `testSample(n)` already has this
shape; O-ReverseDelay's hash-based `noiseAt(t)`
(`tests/render-harness/main.cpp:1706-1712`) is the broadband variant and is the better excitation here.

### P22 — The DBAP oracle is a **committed generated header**; the generator ships beside it *(Q8, item 9)*

| | Layer 2 golden (2.1) | DBAP reference (2.2) |
|---|---|---|
| Tracks | an external moving target (JUCE) | a fixed published equation set (2011-04-14) |
| Must regenerate when | JUCE changes — that *is* the gate | never |
| Generated | at build time | **once, reviewed, committed** |

The divergence is a policy difference, not an inconsistency. Regenerating every build makes the oracle
a *build product*: a change to the solver plus a matching change to the generator would agree
silently, with nothing in the diff. Committed, it shows up in review. And since no test target in this
repo has ever run in CI, build-time regeneration would only execute for someone already running the
tests.

- Fixture: **`tests/fixtures/DbapReferenceFixture.h`**, committed. A header, not a data file — no
  runtime file I/O and no working-directory dependence in a console app.
- Generator: `tests/tools/gen_dbap_reference.py`, modelled on `gen_juce_channel_order.py`
  (argument-driven, fails loud, byte-deterministic; its failure paths are already proven at
  SUMMARY-2.1 F5). It gets a `--check` mode and a CMake target `O-Octagon-dbap-fixture-check`
  **excluded from `all`**.
- Self-contained per D3: fixture version, the 8 speaker positions in metres, `rigScale` **as a literal
  in the fixture**, and per case `w[8]`, `rolloff`, `blur`, source `(x, y, z)` in metres, and expected
  `v[8]` as **doubles** (`%.17g`). The C++ compares the float solver's output promoted to double.
- **Independence discipline — the Python must not transcribe the C++.** Compute `d^(−a)` as
  `exp(−a·log d)`, not via the `t`/`t²` trick; normalise by explicitly forming `Σ v²` and dividing by
  its square root, not via the `k = 1/√denom` shortcut. An oracle that re-runs the implementation's own
  expression reproduces its errors and passes forever — the argument 2.1 F2 made for the hull oracle.
- **Tolerance follows 2.1 F2 exactly:** hard-assert at **1e-5**, print the measured worst deviation on
  every run, and record in SUMMARY whether DSP-01's 1e-6 was met. The solver is single-precision and
  Clang defaults to `-ffp-contract=on`; a hard gate pinned at 1e-6 invites a "fix" that is a tolerance
  edit.
- Case list: inside the hull; outside; at hull vertices; at exact speaker coordinates with `blur = 0`;
  both rolloff ends (3 and 6); both blur ends (0 and 1); one non-zero weight; two non-zero weights;
  all-zero weights. Positions from a pinned grid plus a pinned-seed pseudorandom set.

### P23 — `prepareToPlay` ordering, and the counter's lifecycle *(Q3, item 10)*

`absoluteSampleCounter` is `std::uint64_t`, plugin-local, monotonic, **reset in `prepareToPlay()` and
nowhere else** — never from the playhead (a host locate would jump the grid and a loop would rewind
it, making QUAL-03 both untestable and untrue), not in `releaseResources()`, not in
`setStateInformation()`. Overflow is a non-issue twice over: 2⁶⁴ samples ≈ 12 My at 48 kHz, and 64
divides 2⁶⁴ so even the wrap preserves grid phase. Say so in a comment on the type so nobody "fixes"
it to `int`.

```cpp
static_assert ((kControlBlock & (kControlBlock - 1)) == 0, "…");   // pattern_ring_invariant_needs_static_assert
```

so `% kControlBlock` becomes `& (kControlBlock - 1)`.

**Order in `prepareToPlay()`, and it changes the first 240 samples of every probe:**

1. `absoluteSampleCounter = 0`
2. `reset (sr, 0.005)` on all 17
3. one `updateControl()`
4. `setCurrentAndTargetValue()` on all 17 from that solve

`SmoothedValue::reset(int)` calls `setCurrentAndTargetValue(target)`, and on a fresh object `target`
is 0 — so without step 4 **every render begins with a 5 ms fade-in from silence**. Deterministic, so
QUAL-03 would still pass, but it corrupts the lead-in of the unity, Layer-3 and Σv²=1 probes unless
each separately discards it. With step 4, sample 0 is already correct.

**One reset site only (H9).** `reset()` is a *state* reset that teleports all 17 to target; calling it
anywhere else would jump every gain, and **QUAL-03 would still pass** because both renders teleport
identically. Only QUAL-01 would catch it, and only by luck.

### P24 — `GainStage` owns the grid and the smoothers; the processor passes G1 **in**

`GainStage` never calls a processor accessor and never calls `getTotalNumOutputChannels()`. The
processor reads `buffer.getNumChannels()` once, evaluates the existing
`mappedOutputAvailable (numOut)` helper (**P6, inherited verbatim — not re-derived**), and passes the
`bool` and the `int` into `GainStage::process()`. G1 stays stated in exactly one place.

The buffer's channel count is fixed for the duration of a block, so the mode cannot change mid-block;
it *can* change **between** blocks (probe S), which is why P18's unconditional advance matters.

---

## Tasks

### Task 1 — `Source/Data/VenueGeometry.h`; `VenueModel` delegates

Per P14. Header-only, `inline`, `<cmath>` + `Vec.h` only.

- `oo::plane::kMinSpan` is the single definition; `VenueModel::kMinSpan` aliases it.
- `earHeight (rakeFront, rakeRear, bbMinY, bbMaxY, y)` — linear, extrapolated outside the range, with
  the span guard collapsing to `rakeFront`.
- `normToMetres (bbMinX, bbMaxX, bbMinY, bbMaxY, nx, ny)` — **its own** span guard, per axis,
  collapsing to `bbMinX` / `bbMinY`.
- `VenueModel::earHeight()`, `absoluteHeight()` and `normToMetres()` become one-line delegates. Public
  signatures and the on-disk contract are unchanged.

Header-only → nothing added to `target_sources`.

**Files:** `Source/Data/VenueGeometry.h` (new), `Source/Data/VenueModel.h`, `Source/Data/VenueModel.cpp` (modify)
**Depends on:** nothing

---

### Task 2 — Stamp the generation inside `VenueSnapshot`

Per P16. Four lines plus a comment recording the deviation from §3.6.6 and why (H1: two acquires make
the dirty check go **permanently** stale, not transiently wrong).

- `std::uint32_t generation` becomes a `VenueSnapshot` field, written by `publish()` **before** the
  release store on `activeSlot`.
- The `std::atomic<std::uint32_t> generation` member is either removed or demoted to a message-thread
  counter. **Nothing on the audio thread may read it.**
- `static_assert (std::is_trivially_copyable_v<VenueSnapshot>)` still holds — check it does.

**Files:** `Source/Data/VenueSnapshot.h` (modify)
**Depends on:** nothing

---

### Task 3 — `Source/DSP/DbapSolver.{h,cpp}` + the four instrumentation counters

§3.3 verbatim, per P19 and P20.

- Constants and their `static_assert`s (§3.3.1) — `kInvTwentyLog10Two`, `kMinDistance = 0.05f`,
  `kMaxBlurMetres = 8.0f`, `kBlurScale = 0.5f`, `kDenomEpsilon = 1e-20f`.
- `blurToRadius (blur, rigScale) = min (blur · kBlurScale · rigScale, kMaxBlurMetres)` (§3.3.2).
- `rolloffToAlpha (R) = R · kInvTwentyLog10Two` (§3.3.1 / eq 4).
- `solve()` per P19's raw-input signature. `d = max (sqrt (dx²+dy²+dz²+rs²), kMinDistance)` applied
  **unconditionally on every path**, including `blur = 0` with the source exactly on a speaker.
- `t = countedPow (d, −a)`; `denom = Σ w_i²·t_i²`; `denom < kDenomEpsilon` → **write explicit zeros
  and return** (not a NaN, not full-scale); else `k = 1/sqrt(denom)`, `v_i = k·w_i·t_i`.
- `oo::instr` block behind `#if OOCTAGON_INSTRUMENT` with `powCalls`, `solveRuns`, `hullProjections`,
  `sampleAdvances` and a `resetCounters()`. In the `#else` arm `countedPow` forwards to `std::pow` and
  the counter objects do not exist.

`DbapSolver.cpp` must include **only** `<cmath>`, `<atomic>` and `Vec.h` — the narrow unit-target link
line depends on it.

**Files:** `Source/DSP/DbapSolver.h`, `Source/DSP/DbapSolver.cpp` (new), `CMakeLists.txt` (modify)
**Depends on:** nothing

---

### Task 4 — `tests/tools/gen_dbap_reference.py` + the committed fixture

Per P22.

1. Independent implementation of eqs 9–10 from the **2011-04-14 revision** — `exp(−a·log d)`, explicit
   `Σ v²` normalisation. **Do not transcribe the C++.**
2. Emits `tests/fixtures/DbapReferenceFixture.h`: version, the 8 speaker positions, `rigScale` as a
   literal, and per case `w[8]`, `rolloff`, `blur`, source `(x,y,z)`, expected `v[8]` as `%.17g`
   doubles.
3. Deterministic output — no timestamps, no absolute paths, no build-directory strings. Two runs
   byte-identical.
4. `--check` mode: regenerate to a temp buffer and diff against the committed file; exit non-zero on
   divergence.
5. **Exit non-zero on any failure and never emit a fixture with zero cases.** A vacuous oracle is
   worse than none — it reports green.

Run it once, **read the diff**, and commit the fixture.

**Files:** `tests/tools/gen_dbap_reference.py`, `tests/fixtures/DbapReferenceFixture.h` (new)
**Depends on:** nothing (independent of Task 3 by design)

---

### Task 5 — `Source/DSP/SourceShaper.{h,cpp}` in D1-degenerate form

Per P15. §3.4.1 steps 1–6 in full; `<cmath>` + `Vec.h` + `VenueGeometry.h` only, no JUCE.

```cpp
struct SubPoints { Vec3 left, right; float wEff; };

SubPoints shape (const VenueSnapshot& v, float srcXNorm, float srcYNorm,
                 float srcZ, float widthMetres) noexcept;
```

- Step 1 via `oo::plane::normToMetres` — **not** a second denormalisation.
- Step 2 bearing from the **rig centroid**, not the bbox centre.
- Step 3 `rFade = 0.15 · rigScale`; `wEff = width · min (1, |b| / rFade)`.
- Step 4 `b̂ = b / max(|b|, 1e-6)` with the **`(0,−1)` fallback** at `|b| < 1e-6`; `n̂ = (−b̂.y, b̂.x)`.
- Step 5 `P_L = P − (wEff/2)n̂`, `P_R = P + (wEff/2)n̂`.
- Step 6 each sub-point resolves **its own** `z = oo::plane::earHeight (…, P.y) + srcZ`. The plane
  slopes in y and the spread has a y component — do not hoist this out of the pair.

Handedness is asserted by probe AH, not by the comment.

**Files:** `Source/DSP/SourceShaper.h`, `Source/DSP/SourceShaper.cpp` (new), `CMakeLists.txt` (modify)
**Depends on:** Tasks 1, 2

---

### Task 6 — `Source/DSP/GainStage.{h,cpp}` — control grid, 17 smoothers, inner loop

The heart of the phase. §3.6.2 and §3.6.4 **verbatim, written once** (D1).

**Members:** `std::array<juce::SmoothedValue<float, ValueSmoothingTypes::Linear>, 8> gL, gR;` plus
`outGain`; `std::uint64_t absoluteSampleCounter`; `std::array<float,17> lastSolvedParams`;
`std::uint32_t lastSolvedGeneration`; `bool haveSolved`.

**`prepare (sr)`** — P23's four-step order, `static_assert` on `kControlBlock`.

**`process (buffer, numOut, mapped, snapshot, params)`** — the §3.6.2 chunk loop:

```
n = 0
while n < numSamples:
    samplesToNextBoundary = kControlBlock - (absoluteSampleCounter & (kControlBlock - 1))
    chunk = min (numSamples - n, samplesToNextBoundary)
    if ((absoluteSampleCounter & (kControlBlock - 1)) == 0)  updateControl (…)
    renderChunk (n, chunk)
    n += chunk;  absoluteSampleCounter += chunk
```

`numSamples == 0` (pluginval issues these) leaves the body unexecuted; a buffer larger than the
prepared `samplesPerBlock` (strictness 10 issues these) is handled by the chunk loop with no
allocation.

**`updateControl()`** — §5 steps 1–5 and 7:

1. Sanitised 17-float snapshot (P17) vs `lastSolvedParams` by **`std::memcmp`**, and
   `snapshot.generation` vs `lastSolvedGeneration`. Unchanged → return. `memcmp` is right *because* of
   NaN, not despite it: element-wise `!=` is true for NaN against itself and would re-solve every
   block forever. `-0.0f` vs `+0.0f` differ bitwise and cost one spurious re-solve — comment it so it
   is not "fixed". The `bitExact()` idiom already exists in both test mains.
2. `SourceShaper::shape (…, /* PHASE-2.3-WIDTH */ 0.0f)`.
3. Per sub-point: `hull::isInside (snapshot.hullPts, snapshot.hullCount, p, snapshot.hullEpsCross)`;
   **only if not inside**, `hull::project(...)` — the `if` is the mechanism for PERF-01/3, and
   `hullProjections` is incremented inside that branch.
4. `dbap::solve()` per sub-point.
5. `PHASE-2.3-AIR` marker at §5 step 6's insertion point. `PHASE-2.3-TRIM` marker where `trimLin` will
   be folded — **`trimLin` is carried in the snapshot and applied nowhere** (FUNC-07 → 2.3).
6. `gL[i].setTargetValue (vL[i])`, `gR[i].setTargetValue (vR[i])`,
   `outGain.setTargetValue (Decibels::decibelsToGain (params.outputGain))`.
7. `++solveRuns`.

**`renderChunk()`** — §3.6.4 verbatim:

```cpp
for (int n = start; n < start + count; ++n)
{
    const float sL = 0.5f * in0[n];          // read BEFORE any output write for THIS sample
    const float sR = 0.5f * in1[n];
    const float g  = outGain.getNextValue();

    for (int i = 0; i < 8; ++i)
        out[snapshot.speakerToBuffer[i]][n] = (gL[i].getNextValue() * sL
                                             + gR[i].getNextValue() * sR) * g;
    ++sampleAdvances;
}
```

- **Invariant, load-bearing:** every `getNextValue()` is called **exactly once per sample,
  unconditionally**. No `continue`, no early exit, no `if (w[i] == 0) skip` — ever. A desynchronised
  smoother produces a slow, position-dependent gain error **no single-parameter test will find**.
- **H7 — read before write, per sample.** `out[0]` aliases `in[0]` and no scratch buffer is available
  (PERF-01 forbids the allocation). At 2.1 every lane got the same value so a channel-major write was
  survivable; at 2.2 it is not. The plausible-looking "optimisation" that hoists a read pointer and
  writes channel-major **would pass at blockSize 1** and fail everywhere else
  (`pattern_grain_read_before_capture_write_blocksize`). `in1` is `in0` when the input bus is mono;
  `numIn == 0` yields silence rather than a read of channel 0.
- **SAFE mode (P18):** the same loop runs, all 17 advance, `sampleAdvances` increments — only the
  write differs: the **dry input at unity** to the `numOut` available channels, `outGain` **not**
  applied. Comment it as contract-mandated (§5).
- **Do not add a width-dependent branch to elide the second solve** (§3.4.3). Under D1 the second
  solve is *already* redundant, and it still must not be optimised away — a branch that changes the
  arithmetic path between block boundary A and block boundary B is the exact class of bug QUAL-03
  exists to catch.

**Files:** `Source/DSP/GainStage.h`, `Source/DSP/GainStage.cpp` (new), `CMakeLists.txt` (modify)
**Depends on:** Tasks 2, 3, 5

---

### Task 7 — Processor integration and the `processBlock` rewrite

- **`paramDefaults`** — the 17 declared defaults, derived from the parameter objects in the
  constructor beside the existing atomic cache (P17). Never hand-transcribed.
- `GainStage gainStage;` member. `prepareToPlay()` calls `gainStage.prepare (sampleRate)` **after**
  `readVenueFromState()` and `rebuildChannelMap()`, so the initial `updateControl()` of P23 step 3
  solves against a published snapshot and a built map.
- `processBlock()` becomes: `ScopedNoDenormals` → `numOut = buffer.getNumChannels()` →
  `mapped = mappedOutputAvailable (numOut)` → `snapshot = venuePublisher.read()` (once, held) →
  sanitised parameter snapshot → `gainStage.process (…)`. The 2.1 mono-sum block is **replaced**, and
  the 1/`numIn` averaging is superseded by §3.4.3's `0.5·L` / `0.5·R` convention (identical result for
  both mono and stereo input — no level change).
- `releaseResources()` does **not** touch the counter (P23).
- `setStateInformation()` is unchanged in ordering; it must not reset the counter.
- **Instrumentation-gated accessor** (P20): under `#if OOCTAGON_INSTRUMENT`, a method returning the 17
  smoothers' `getCurrentValue()`s so probe AT can assert they all *arrived*.

**Files:** `Source/PluginProcessor.h`, `Source/PluginProcessor.cpp` (modify)
**Depends on:** Tasks 6

---

### Task 8 — CMake wiring

- Plugin `CMakeLists.txt`: add `Source/DSP/DbapSolver.cpp`, `Source/DSP/SourceShaper.cpp`,
  `Source/DSP/GainStage.cpp` to `target_sources`. `VenueGeometry.h` is header-only and stays absent,
  like `Vec.h` and `VenueSnapshot.h`.
- `tests/unit/CMakeLists.txt`: add `DbapSolver.cpp` and `SourceShaper.cpp`; add
  `OOCTAGON_INSTRUMENT=1`; add `tests/fixtures` to the include path. **The link line must stay narrow**
  — no `juce_audio_processors`, no `juce_dsp`, no `PluginProcessor.cpp`. If a new TU forces a link
  addition, that is a design error in the TU, not in the CMake.
- `tests/render-harness/CMakeLists.txt`: add the three new plugin TUs; add `OOCTAGON_INSTRUMENT=1`.
  `juce_dsp` is already linked (`:109`) — the FFT needs nothing new.
- `O-Octagon-dbap-fixture-check` custom target running `gen_dbap_reference.py --check`,
  **`EXCLUDE_FROM_ALL`** (P22).
- **Do not touch `.github/workflows/build-and-release.yml`.** It is tag-triggered, secrets-bearing,
  and carries a standing rule against widening its trigger surface. The CI gap is already logged at
  `.planning/todos/pending/2026-08-11-secrets-free-test-ci-workflow.md` and belongs to Stage 4.

**Files:** `CMakeLists.txt`, `tests/unit/CMakeLists.txt`, `tests/render-harness/CMakeLists.txt` (modify)
**Depends on:** Tasks 3, 4, 5, 6

---

### Task 9 — Unit-target probes V–AH

Appended to `tests/unit/main.cpp` (A–P already exist and must all still pass). Same `check()` idiom —
**no framework**.

| # | Probe | Closes |
|---|---|---|
| V | `VenueModel::earHeight()` == `oo::plane::earHeight()` over a swept `y` on a **non-default** venue, incl. the zero-span guard | P14 / Q2 |
| W | `VenueModel::normToMetres()` == `oo::plane::normToMetres()` over swept `(nx,ny)`, incl. **its own** zero-span guard on a zero-width bbox with a live rake | P14 / Q2 |
| X | `a = R/(20·log10 2)` at R = 3, 4, 6 vs hand-computed | DSP-01/1 |
| Y | **Gains vs the committed Python fixture, every case.** Hard gate 1e-5, worst deviation **printed**, DSP-01's 1e-6 recorded in SUMMARY | DSP-01/3, P22 |
| Z | Changing **only** `srcZ` changes the gain vector — the `(z_i − z_s)²` term. *Non-vacuous only because §OQ4's default heights are graded 4.50→5.40 m; do not flatten them* | DSP-01/2 |
| AA | `Σ v_i² = 1 ± 1e-6` — dense sweep inside the hull, outside, at hull vertices, at exact speaker coordinates, across the full rolloff (3–6) × blur (0–1) product. Measured **at the solver output** | DSP-02/1,2,3 |
| AB | `w_i = 0` → **exactly** 0.0f at speaker *i*; a 2-speaker subset still `Σ v² = 1` | DSP-05/1,2 |
| AC | All-zero weights → all 8 exactly 0.0f. **Not NaN, not full-scale.** Assert `std::isfinite` and `== 0.0f` bitwise | DSP-05/3, QUAL-02 |
| AD | Finite at every exact speaker coordinate with `blur = 0`; at both rolloff ends; on the degenerate venues of §3.1.6 | QUAL-02/1,2,3 |
| AE | `powCalls` per solve pair ≤ 32, **and == 16** (the `t = pow(d,−a)` reuse). Assert the exact figure, not just the bound — the bound alone passes if the reuse is dropped | PERF-02/3 |
| AF | **Mirror symmetry (H4).** §OQ4 is exactly symmetric about x = 6.5 m, so pairs (1,2), (3,8), (4,7), (5,6) are **bit-identical** at `srcX = 0.5` — for a *correct* implementation — and pairwise distinct off-centre. A dropped `z` term, a transposed index or a sign error in the perpendicular breaks it, and unlike a distinctness check it cannot be satisfied by noise | DSP-01, FUNC-01 support |
| AG | `r_s = min (blur·0.5·rigScale, 8)`; the §3.3.2 table incl. **1.98 m at blur 0.50**; and the **scaling invariant** — doubling every coordinate doubles `r_s`. *The invariant is the real assertion; a bare constant is a mirrored fixture* | DSP-08 support (closes 2.3) |
| AH | `SourceShaper` driven **directly** at `width > 0`: handedness (puck downstage → `P_R` on audience right), the `rFade` collapse to a point at the centroid, the `(0,−1)` fallback, per-sub-point `earHeight` at differing `y`. **Coverage for code shipped inert — NOT a DSP-06 claim** | P15 |

**Files:** `tests/unit/main.cpp` (modify)
**Depends on:** Tasks 3, 4, 5, 8

---

### Task 10 — Render-harness probes; **re-specify probe Q**

Appended to `tests/render-harness/main.cpp`.

#### Probe Q must be re-specified, or it will fail

**This is predicted here, not to be discovered at execute.** Probe Q asserts unity gain through all 8
outputs (`max |out − in| = 0.000000000` at 2.1). Once DBAP is live, lane *i* carries `v_i · in`, and
`Σ v_i² = 1` means no individual lane is at unity. Q's *original* statement is now false by design.

Re-spec, and it is the stronger statement: with `w = δ_ij`, `denom = w_j²·d_j^(−2a)` so `k = d_j^a`
and `v_j = 1` **analytically**. Q becomes *"with `w = δ_ij` and a mono input, lane
`speakerToBuffer[j]` reproduces the input at unity"*. Assert to **1e-6 relative, not bit-exact** — the
`k = 1/sqrt(t²)` round-trip is not guaranteed exact in single precision, and a bit-exact gate here
would be flaky rather than strict. Probes R, S, T and U are unaffected and must still pass.

| # | Probe | Closes |
|---|---|---|
| Q′ | **Re-spec as above** | Stage-1 issue 3 carry, DSP-05/1 |
| AI | **FUNC-01/3 independence.** Off-centre source `srcX = 0.18, srcY = 0.72` — avoids every symmetry axis and every speaker coordinate (H4) — under a **non-identity** label map; all 8 lanes pairwise distinct | **FUNC-01/3** |
| AJ | **Channel-map Layer 3 — mandatory.** Eight renders, `w = δ_ij`, bin-centred tone `f_j`, N = 4096 at 48 kHz, `k ∈ {64,128,…,512}`, **rectangular window** (a Hann spreads into k±1 and turns "the dominant bin" into a three-bin argument). Per render: lane `speakerToBuffer[j]` loud, the other seven below a floor — *this is the assertion that catches a permutation error*. Then accumulate all eight into one 8-channel buffer and FFT per lane: dominant bin == expected `k`, and level ≈ unity. **C1: the venue must carry a non-identity label map** or the probe is byte-identical to a hardcoded map. Discard ≥ one control block of start transient | **FUNC-03/3**, ROADMAP mandatory |
| AK | **DSP-04/3, both halves (H5).** A **rear** source's gain vector changes when `rakeRear` alone moves, **and** a source at `srcY = 0` does **not** — `earHeight(bbMinY) = rakeFront` for any `rakeRear`, so the negative half is provable. The positive half alone passes if any unrelated recompute fired | **DSP-04/3** |
| AL | **QUAL-03 mandated.** 512 vs 4096, automation at multiples of 4096, **`memcmp`** | QUAL-03/1 |
| AM | **QUAL-03 real gate.** Ragged block sizes `1,7,64,333,4096` vs fixed 4096, arbitrary event offsets, `memcmp` (P21) | QUAL-03/1 |
| AN | Parameters held constant, several block-size pairs, `memcmp` | QUAL-03/2,3 |
| AO | **PERF-01/1 allocation.** Replace `operator new`, `operator new[]`, both `std::align_val_t` overloads **and every matching `operator delete`** with counting `malloc`/`free` wrappers. **Warm up with one `processBlock` before arming**, so libc++/JUCE first-touch initialisation is not attributed to the audio path. Assert the counter did not move. *An un-replaced aligned-new is silently uncounted and a probe that counts nothing passes* | PERF-01/1 (allocation half) |
| AP | `hullProjections == 0` for an inside source and `> 0` for an outside one; `solveRuns` does **not** increment across blocks with nothing changed and **does** on a parameter change and on a venue edit | PERF-01/3, PERF-02/1,2 |
| AQ | **H1 regression.** Publish a venue edit **between two `processBlock` calls** and assert the gain vector moves by the next control block. *Invisible to any probe that edits the venue while audio is stopped* | P16 / H1 |
| AR | **QUAL-02/4 + H2.** Write **NaN into a parameter** (not only into the input) → output stays finite and the next block is correct; no sticky NaN. Plus pathological input: silence, DC, full-scale, denormals | QUAL-02/4 |
| AS | **QUAL-04/1,2.** Full-speed sweep of `srcX`/`srcY`/`srcZ`, and of all 8 weights **through exact zero**, with a **DC input** so the rendered sample *is* the gain. Assert per-sample \|Δout\| ≤ 1/240 + 1e-6 on every lane (`v_i ∈ [0,1]` because `Σ v² = 1`, and the 5 ms ramp is 240 steps at 48 kHz). **Negative control:** compute the same sweep with the solved vector held per control block instead of ramped, and assert *that* series **exceeds** the bound — a probe never observed to fail is a probe whose failure path is untested | QUAL-04/1,2 |
| AT | **SAFE mode + the exactly-once invariant.** At (1,1), (1,2), (2,1), (2,2): `solveRuns` increments (steps 1–7 ran), `sampleAdvances == totalSamples` in **both** modes, output == dry input at unity with `outputGain` **not** applied (P18), and after 240 still samples all 17 smoothers have arrived at target in **both** modes | PERF-02/4, P18, COMPAT-01 support |

**Excitation discipline:** the block-size probes use a **position-deterministic** excitation — a
function of the absolute sample index, never a sequential generator, or the two runs do not share an
input signal at all. The existing `testSample(n)` already has this shape; a hash-based `noiseAt(t)`
(broadband) is the better choice for AL/AM/AN.

**H10 — do not assert rendered == solved while a parameter is moving.** `stepsToTarget` is 240 at
48 kHz while a new target arrives every 64 samples, so during a sweep the gain **chases** the solved
value and never reaches it. That is §3.6.5 working as intended. Compare only after ≥ 240 samples of
stillness (AT does exactly this).

**Files:** `tests/render-harness/main.cpp` (modify)
**Depends on:** Tasks 7, 8

---

### Task 11 — Gates

Run every one; record **actual output**. Do not read results out of a prior document (C6).

1. **Clean 3-format build** (VST3 + AU + Standalone), forced TU recompile — **zero warnings, zero
   errors** across the whole log
2. `grep -rn` for hardcoded output channel indices outside `ChannelMap` — expected **zero**. A loop
   variable is not a hardcoded index
3. **Marker uniqueness:** `PHASE-2.3-WIDTH`, `PHASE-2.3-AIR`, `PHASE-2.3-TRIM` each appear **exactly
   once** in `Source/`. `PHASE-2.2-REPLACE` remains **0**
4. `auval -a | grep -i octagon`, then `auval -v aufx OuOc OuDv` → **AU VALIDATION SUCCEEDED**
5. **pluginval strictness 10, VST3 and AU, ×3 each** (`pattern_ci_pluginval10_catches_latent_nan` — a
   latent NaN is exactly what this phase can introduce)
6. Configure with `-DOUARICON_BUILD_TESTS=ON`; build and run **both** test targets → **exit 0**;
   all of A–U still pass alongside V–AT
7. `gen_dbap_reference.py --check` → exit 0 (the committed fixture matches its generator)
8. 17 parameters unchanged against `parameter-spec.md` — **2.2 adds none**
9. `setLatencySamples` still appears nowhere; no `switch` on `ChannelType`; `createEditor`'s
   `#if JUCE_WEB_BROWSER` guard still present
10. `grep` the shipping TUs for `OOCTAGON_INSTRUMENT` — the plugin target must not define it; confirm
    from the build log that only the two test targets do

**Files:** none (verification)
**Depends on:** Tasks 9, 10

---

### Task 12 — Manual Logic gate (D4) ⚠️ HUMAN, before verify closes

~10 minutes, as Task 13 was for Stage 1. **Not blocking execute** — it is corroboration, and the gate
for FUNC-03/3 is probe AJ (verify-ping is FUNC-04, at Stage 3).

- Automate `srcX` across the room and confirm the 8 surround-meter lanes **no longer move in
  lockstep** — the direct contrast with 2.1, where identical signal on all 8 was correct by design
- Set `w3 = 0` and confirm that lane goes **silent** while the others compensate (DSP-05, and the
  audible half of `Σ v_i² = 1`)

Record both observations verbatim in SUMMARY-2.2, positive or negative.

**Files:** none. **Depends on:** Task 11

---

### Task 13 — `SUMMARY-2.2.md` + `STATUS.md`

- Record the **two deviations** explicitly: P16 (`VenueSnapshot.generation`, beyond §3.6.6's field
  list) and P15 (`SourceShaper` created a phase early, beyond ROADMAP's placement).
- Record PERF-01's method honestly per H8: allocation **measured**, locks and file I/O **grep +
  inspection**, RTSan **unavailable on this toolchain**.
- Record probe Y's measured worst deviation and whether DSP-01's 1e-6 was met at the 1e-5 gate.
- Record probe Q's re-specification and why the original statement became false.
- `REQUIREMENTS.md` is updated at **verify**, not here.

**Files:** `stages/2-dsp/SUMMARY-2.2.md`, `.planning/STATUS.md`
**Depends on:** Task 12

---

## Execution Constraints

- **DO NOT execute this phase in an isolated worktree.** `stages/2-dsp/` is untracked; the scope would
  vanish and every gate would pass vacuously
  (`pattern_worktree_isolation_wrong_for_untracked_scope`).
- Plugin CMake target is **`OuariconOctagon`**; the folder is `O-Octagon`
  (`build_script_target_name_vs_folder`).
- Read `parameter-spec.md`, **never** `parameter-spec-draft.md`.
- **DBAP per the 2011-04-14 revised equations only** — the original's eqs 3–6 and 9–10 are wrong
  (`pattern_dbap_not_vbap_for_irregular_arrays`).
- `GainStage` **calls** `mappedOutputAvailable (int)`; it does not re-derive G1 and never calls
  `getTotalNumOutputChannels()`, which is the accessor that lies under F3.
- No `switch` on `AudioChannelSet::ChannelType` (`-Wswitch-enum`). No `setLatencySamples()`.
- Never call a parameter setter from the audio thread — `setValueNotifyingHost` takes two locks
  (RESEARCH-2.2 Q1).
- In the harness, write parameters with
  `apvts.getParameter(id)->setValueNotifyingHost (range.convertTo0to1 (real))`. **`setValue()` alone
  does not notify and leaves the cached atomic stale** — a harness written against it would render
  both block sizes against the default gain vector and report QUAL-03 green having tested nothing.
- Never assert against `apvts.state` in a console app: the `ValueTree` mirror is timer-driven and that
  timer never fires. Assert against `getRawParameterValue()`.
- Clear the AU cache and sweep **both** `-dev` and unsuffixed bundles before any install
  (`./scripts/build-and-install.sh O-Octagon`).

## Non-goals for Phase 2.2 — must not appear

`HullProcessor`, the hull gain trim, the air-absorption LPF, any `FirstOrderTPTFilter`, FUNC-07 trim
**application**, any read of `widthParam`, `VerifyPing`, metering, any WebView editor, any
`juce_add_binary_data` target, any new APVTS parameter, any change to `build-and-release.yml`, any
substitution of `std::atomic<std::shared_ptr>` for the snapshot double-buffer.

---

## Success Criteria

**ROADMAP Phase 2.2 test criteria — all thirteen:**

- [ ] `a = R/(20·log10 2)` matches hand-computed values at R = 3, 4, 6
- [ ] `d_i` includes `(z_i − z_s)²` — changing only `srcZ` changes the gain vector
- [ ] Gain vector matches an independent reference of eqs 9–10 to 1e-6 over a fixture set
- [ ] `Σ v_i² = 1 ± 1e-6` inside the hull, outside, at vertices, at exact speaker positions —
      measured at the **solver output**
- [ ] Holds across the full rolloff (3–6) × blur (0–1) product
- [ ] `w_i = 0` → exactly zero at speaker *i*; a 2-speaker subset preserves `Σ v_i² = 1`
- [ ] **All-zero weights → silence, not NaN, not full-scale**
- [ ] Source at each exact speaker coordinate with `blur = 0` → finite output
- [ ] Pathological input → no sticky NaN
- [ ] **Block-size invariance:** programmatic automation at grid-aligned absolute offsets, 512 vs
      4096, **bit-identical by `memcmp`**; and any pair of sizes with parameters held constant
- [ ] No zipper noise on a full-speed sweep of position or of all 8 weights
- [ ] RT-safety: no allocation, lock or file I/O in `processBlock`; `pow` per block ≤ 32
- [ ] **Channel-map Layer 3** — `w = δ_ij`, unique tone per speaker, each output channel's dominant
      FFT bin exactly its speaker's frequency. **Mandatory**

**Added by this plan:**

- [ ] Layer 3 drives a **non-identity** label map (C1) — a container-only test is vacuous
- [ ] The independence probe uses an **off-centre** source; the §OQ4 mirror symmetry is asserted as a
      correctness property in its own right (H4)
- [ ] DSP-04/3 asserts **both** halves — rear source moves, `srcY = 0` source does not (H5)
- [ ] The **ragged** block-size probe passes, not only the mandated 512/4096 pair (H6)
- [ ] The H1 dirty-check race is **measured** by a venue edit published between two `processBlock`
      calls (P16) — not merely fixed
- [ ] A **parameter** NaN, not only an input NaN, is proven not to latch (H2)
- [ ] `powCalls == 16` exactly, `sampleAdvances == totalSamples` in **both** modes
- [ ] The QUAL-04 probe carries a **negative control** proving it can fail
- [ ] Probe Q is re-specified before it fails, and the reason is recorded
- [ ] `gen_dbap_reference.py --check` passes; the fixture is committed and reviewable
- [ ] Three `PHASE-2.3-*` markers, each exactly once; `PHASE-2.2-REPLACE` still zero
- [ ] Contract checksums re-verified at 2.2 verify (C6)
- [ ] Both test targets exit 0; clean 3-format build with zero warnings; pluginval s10 ×3 each;
      `auval` SUCCEEDED

**Requirement outcomes expected at verify:** FUNC-01 ✅ · FUNC-03 ✅ · DSP-01 ✅ · DSP-02 ✅ ·
DSP-04 ✅ · DSP-05 ✅ · PERF-01 ✅ · PERF-02 ✅ · QUAL-02 ✅ · QUAL-03 ✅ ·
**QUAL-04 ⚠️ partial (criterion 3, `width`, → 2.3)**. DSP-08 implemented, **not** ticked.

---

## Risks Active in This Phase

| Risk | Severity | Mitigation in this plan |
|---|---|---|
| **R1 — speaker→buffer channel map** | CRITICAL | Layer 3 lands here (AJ) and is mandatory. C1 enforced: a non-identity map, or the probe is byte-identical to a hardcoded one. AI adds the independence half |
| **H1 — dirty check goes permanently stale** | HIGH | P16 stamps the generation inside the snapshot; AQ measures it with a publish between two `processBlock` calls |
| **H2 — sticky NaN latches in a `SmoothedValue`, reachable from a parameter** | HIGH | P17 sanitises at ingestion against **derived** defaults; AR drives a parameter NaN, not only an input NaN |
| **R3 — block-size invariance vs per-block solve** | HIGH | The §3.6.2 grid; AL (mandated) + AM (ragged, the real gate) + AN; `memcmp`, never a tolerance |
| **G1 — valid map ≠ 8-channel buffer** | HIGH | `GainStage` **calls** the P6 helper; probe S (2.1) still passes; AT covers SAFE mode at all four auval configs |
| **H7 — channel-major write destroys the aliased input** | MEDIUM | Read-before-write per sample, no hoisted read pointer; AM's ragged sizes catch what blockSize 1 would hide |
| **H4 — a naive independence probe fails on correct code** | MEDIUM | Off-centre source; symmetry asserted as its own probe (AF) |
| **The oracle reproduces the implementation's error** | MEDIUM | P22's independence discipline: `exp(−a·log d)`, explicit `Σ v²`, committed and reviewable, `--check` gate |
| **PERF-01 overstated** | MEDIUM | H8: RTSan unavailable, allocation measured by replaced `operator new` family (**every** variant, warmed up), locks/file I/O grep + inspection, said plainly in SUMMARY |
| **A gate that cannot fail** | MEDIUM | Negative control on AS; `powCalls == 16` exactly rather than ≤ 32; `--check` on the fixture |

---

## Next Phase

**Ready for:** execute phase — Phase 2.2 (DBAP Solve and Gain Application).

Writes `stages/2-dsp/SUMMARY-2.2.md`. The stage-level `VERIFICATION.md` is written only at the close
of 2.3.

**Owed at 2.3 discuss, carried from CONTEXT-2.2:** FUNC-07, DSP-06, DSP-07 and DSP-08 have summary
rows in `REQUIREMENTS.md` and **no acceptance criteria**. They must be written **before 2.3 plan**, or
2.3 verifies against nothing — the defect this phase's discuss found and repaired for PERF-02 and
QUAL-04.
