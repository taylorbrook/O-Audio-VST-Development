# Stage 3 — GUI · Phase 3.3 (Scenes, meters, gradient, elevation) — Plan

**Plugin:** O-Octagon
**Stage:** 3 of 4 — GUI · **Phase 3.3 of 3 — the last phase of Stage 3**
**GSD phase:** plan
**Date:** 2026-08-12
**Branch:** `feat/o-octagon` @ `a47cef88` (2.2 / 2.3 / 3.1 / 3.2 work uncommitted)
**Inputs:** `CONTEXT-3.3.md` (D15–D28), `RESEARCH-3.3.md` (N9–N13, all eleven questions answered)
**Closes:** `FUNC-06`, `UI-03`, `UI-04`, `UI-05` — four rows, 18 criteria, **zero partials declared**

---

## Entry Check — contract checksums

Re-run at this boundary before anything else (`pattern_promotion_checksum_pins_replaced_file`).
**All four byte-exact on arrival:**

| Contract | SHA-256 on arrival | STATUS frontmatter | Result |
|---|---|---|---|
| `BRIEF.md` | `697a4f32…f6b9fbd6` | `697a4f32…f6b9fbd6` | ✅ |
| `parameter-spec.md` | `b45f88dc…cbb9e02f` | `b45f88dc…cbb9e02f` | ✅ |
| `research/ARCHITECTURE.md` | `32a85018…81d85273` | `32a85018…81d85273` | ✅ (the 3.3-discuss pin) |
| `ROADMAP.md` | `aec7d0ce…0137ee29` | `aec7d0ce…0137ee29` | ✅ |

**`ROADMAP.md` is then amended at this boundary and re-pinned** — the amendment RESEARCH-3.3 N10
scheduled, plus a second instance of the same class found while taking it. See **P70**.

| Contract | Superseded | New pin |
|---|---|---|
| `ROADMAP.md` | `aec7d0ce…0137ee29` | **`643471ba…3b4383d8`** |

**The other three are untouched and re-measured byte-exact after the edit:** `BRIEF.md`
`697a4f32…f6b9fbd6`, `parameter-spec.md` `b45f88dc…cbb9e02f`, `research/ARCHITECTURE.md`
`32a85018…81d85273`.

This is the **first `ROADMAP.md` re-pin in the project** — the previous three re-pins (2.2 D2,
2.3 D2, 3.3 discuss) all moved `ARCHITECTURE.md`. Gate 12 must therefore check *four* pins against
*two different* expected values, which is exactly the state in which a checksum gate silently checks
the wrong referent. It is called out in the gate row rather than left to be noticed.

### Numbering

The P-series continues from **P68**; 3.3 holds **P69–P85**. C++ probe letters continue from **BZ**;
3.3 holds **CA–CN** (14 probes, 78 → **92**). JS gate sections go **49 → 69**
(`ui_frontend_check` 31 → 42, `ui_layout_check` 18 → 27).

---

## Goal

**Make the plugin readable across a dark hall, and make every reading falsifiable.**

Four requirements close here and they are not four of a kind. `UI-03` is a safety feature wearing a
visualisation costume — §R7 names it a second human line of defence on **R1**, the highest risk in
the project, and it is **not descopable**. `FUNC-06` is the phase's only write path and it writes to
the eight parameters whose all-zero state is DSP-05's silence. `UI-04` and `UI-05` are `nice`, and
**both ship** (D15).

### What "works" means concretely at 3.3

- Eight meters on the plan, driven by the **written buffer post-map and post-trim** — so a
  channel-map error shows up as the **wrong speaker lighting**, cross-checked against verify-ping
  stepping 1 → 8.
- Ten scene controls that write `w1..w8` as **eight bracketed host gestures**, with membership
  **derived in C++ from the measured geometry** and shown on the plan **before** commit.
- A backdrop gradient that is **the field the shipping solver produces**, compared at 20 grid points
  to 1e-3, recomputed on a **counter** that a puck drag must not move.
- A side-elevation strip in which moving `rakeRear` moves **the rear of the line and not the front**.

### The five findings this plan must not lose

1. **N9 — a dropped completion latches the guard permanently, and it is live in shipped 3.2 code.**
   Measured: after one dropped `getVenueGeometry` the envelope readout stays `15.60 × 19.50 m`
   against a real `39.00 × 52.00 m` and never recovers. 3.3 **repairs `refreshGeometry`** as well as
   building the meter poll correctly (**P71**).
2. **N10 — `max_i v_i²` is degenerate**, measured identically `1.0000` everywhere with one active
   weight. The ROADMAP formula is disqualified (**P69, P70**).
3. **N11 — D25's premise was over-attributed.** The column-level coarse assertion **fires** on this
   column; the genuinely vacuous one is the document-level §8. D25's *conclusion* survives on a
   better justification (**P75**).
4. **N12 — the field has five inputs**, three of them automatable at audio rate (**P73**).
5. **N13 — `SCENES` rides `copyState()` free but needs `VENUE`'s normalisation** (**P80**).

### The premise correction that shapes the layout

`VERIFICATION-3.2.md`'s residual reads *"3.3 adds meters and an elevation strip to that same rail."*
**It does not.** The meters go on the ROOM plan and the strip goes in the ROOM controls column;
**3.3 does not touch the venue rail at all**, and the 251 px carried in from 3.2 is the *venue* main
column's slack. 3.3's budget is the ROOM controls column's **278 px at 582 px wide** — measured
twice, independently, at discuss and at research.

---

## Requirement staging — read this before writing the verify report

Four rows close here. **Nothing is staged, nothing is partial, and no criterion is narrowed.**
Closing these completes Stage 3: `UI-02` (3.1), `FUNC-02`/`FUNC-04`/`FUNC-05`/`UI-01` (3.2), these
four (3.3) — **nine rows**.

### FUNC-06 — Weight scenes *(6 criteria)*

| # | Criterion | Closed by |
|---|---|---|
| 1 | All 8 weights written in one gesture via `setValueNotifyingHost`, read back host-side | **CI** |
| 2 | Membership **geometry-derived**; the **permutation** probe a fixed-index impl must fail | **CG** (+ CF) |
| 3 | The plan shows the set **before commit**; an empty set is legible and **not writable** | **CH** + §24/§25 |
| 4 | 4 user slots capture/recall, persisted in a **`SCENES` child**, round-trip through session state | **CK** |
| 5 | FUNC-05's guarantee **re-measured** against the new tree shape — 42 values bit-identical | **CL** |
| 6 | Two scenes can be **faded** between — the gate that catches a latching implementation | **CJ** |

### UI-03 — Live per-speaker level indicators *(4 criteria)*

| # | Criterion | Closed by |
|---|---|---|
| 1 | Eight indicators at plan positions, driven by the atomic array metered **post-map, post-trim** | **CM** + §23 |
| 2 | **The speaker that lights is the speaker that sounds**, cross-checked against ping 1 → 8 | **CM** |
| 3 | Ballistics: attack 0.5 / decay 0.12 **per rAF frame**, −60..0 dBFS, 1.5 s hold, 20 dB/s release | §37 + Gate 13 |
| 4 | ~30 Hz read-and-zero; **PERF-01 does not regress** — probe AO re-run with metering live | **CN** |

> **Criterion 4's mechanism clause was corrected at this boundary (P70).** It said *"on a Timer"*,
> inherited from §4.3 before the 3.3-discuss amendment. The rate, the zeroing and the PERF-01 clause
> are unchanged and are the testable content.

### UI-04 — DBAP gradient backdrop *(4 criteria)*

| # | Criterion | Closed by |
|---|---|---|
| 1 | 20 grid points vs a **direct solve to 1e-3**, against the shipping solver, never JS | **CB** |
| 2 | Message-thread, on geometry/weight change only — **asserted by a counter**; a puck drag must not move it | **CD** + **CC** + §27 |
| 3 | Offscreen canvas and blitted; no CPU spike during drag | §38 + §26 |
| 4 | Descopable **without touching any other component** — structural, a separate draw layer | §39 |

### UI-05 — Side-elevation strip *(4 criteria)*

| # | Criterion | Closed by |
|---|---|---|
| 1 | Rake line `rakeFront@bbMinY` → `rakeRear@bbMaxY`; **`rakeRear` alone visibly moves the rear** | §28 + NC7 |
| 2 | `srcZ = 0` rides the rake front-to-back; **both readings shown** | §29 |
| 3 | Speaker heights at their y positions — the §OQ4 grading visible | §30 |
| 4 | Descopable without touching any other component | §40 |

> **UI-05/1's negative half is the coverage**, exactly as DSP-04/3's was: assert the rear endpoint
> **moved** *and* the front endpoint **did not**. A rescaling height axis passes the first half and
> fails the second, which is why P76 quantises the axis.

---

## Plan Decisions

### P69 — UI-04's field quantity is **`1/k = √denom`**, reached by a **defaulted out-param** *(N10)*

`max_i v_i²` is disqualified by measurement, not by preference. DBAP normalises to `Σ v_i² = 1`, so
it measures **concentration**, not level: `1/8` when energy is spread, `1` when it is in one speaker
— and therefore **identically `1.0000` at every point in the room** when exactly one weight is
non-zero. The gradient would go blank precisely when the spatial situation is most extreme.

`1/k` is what the solver **already computes** as `denom` before normalising
(`DbapSolver.cpp:76, 90`), gives **1.3–10.4 dB** with correct radial structure, and never degenerates.

```cpp
void solve (const Vec3 spk[kNumSpeakers], const float w[kNumSpeakers], Vec3 src,
            float a, float rs, float outV[kNumSpeakers],
            float* outInvK = nullptr) noexcept;   // √denom; nullptr on the audio path
```

The precedent is **P54's `MapDiagnosis* whyNot = nullptr`**, set in this codebase at 3.2. Every
existing call site compiles unchanged, **no new `pow` is introduced**, `powCalls == 16` is untouched,
and the all-zero-weight early return writes `0.0f` — the correct field value for a rig with no active
speakers. **Rejected:** recomputing `denom` in the field sampler — a mirrored fixture over the
highest-risk arithmetic in the plugin (`pattern_test_fixture_mirrors_drift_silently`), and it would
make UI-04/1's *"compared against a direct solve"* a comparison of two copies of the same mistake.

**The colour mapping normalises to the per-recompute observed min/max and prints the dB span in a
legend.** The field over a raked audience plane is **genuinely flat** — every grid point is at `z=0`
while the speakers are 4.50–5.40 m up, so the minimum 3-D distance is ≥ 4.5 m in a 12 × 15 m hall.
An absolute 0..1 colour map renders a uniform wash **while looking as though it carries
information**, which is `CONTEXT-3.3`'s "beautiful and wrong" risk arriving from an unexpected
direction.

### P70 — Two documents are amended here and **`ROADMAP.md` is re-pinned** *(N10, and one more found while taking it)*

Precedent: 2.2 D2, 2.3 D2 and 3.3-discuss all amended a contract and re-pinned at a phase boundary.
Research explicitly routed N10's amendment **to the plan boundary** rather than taking it.

| # | Document | Amendment | Source |
|---|---|---|---|
| 1 | `ROADMAP.md` Phase 3.3, gradient bullet | `max_i v_i²` → **`1/k = √denom`**, with the measured degeneracy and the flat-field consequence stated | N10, scheduled |
| 2 | `ROADMAP.md` Phase 3.3, meter bullet | **Found here.** *"~30 Hz Timer read"* → fixed-interval JS pull with a deadline-released guard | New |
| 3 | `REQUIREMENTS.md` UI-03 criterion 4 | Same mechanism phrase, same correction | New |

**Amendment 2 is worth naming separately.** `ARCHITECTURE.md` §4.3 carried the identical *"on a
Timer"* error and was corrected at the 3.3 **discuss** boundary — but nobody checked whether the
**second** document said the same thing. It did, and so did the acceptance criterion derived from it.
Honouring it literally would have undone 3.1's deliberate choice to keep `PluginEditor` `Timer`-free,
which is what lets `tests/ui-stub/` render the whole UI and makes the pre-integration half of every
layout gate possible. **The rule this leaves behind: when an amendment corrects a claim, grep the
other contracts for the same claim before closing the boundary.**

`REQUIREMENTS.md` is not a pinned contract and is not re-pinned; the correction is a dated in-place
note in the same style as the sections added at the Stage 3 discuss boundary. **No criterion's
testable content changed** — only a mechanism phrase, and it moved *toward* the pinned contract.

### P71 — **Every in-flight guard on this page releases on a DEADLINE**, and `refreshGeometry` is repaired at 3.3 *(N9)*

N4 established that a completion is **dropped, not rejected**, when the browser is hidden. A dropped
completion is not an exception and not a rejection, so **neither `catch` nor `finally` runs**, the
coroutine suspends forever, and a `finally`-released flag stays `true` for the life of the page.

**This is measured, in shipped 3.2 code**, and it is worse than "the meters freeze": hiding the
editor once during a venue change leaves the Room plan, the Venue table, the envelope readout and the
metres readout permanently frozen on a stale venue **with no error anywhere**.

Three rules, and the first is 3.2 debt discharged here:

1. **`refreshGeometry` is repaired.** Not caused by 3.3; repaired by 3.3, because 3.3 is what makes
   the same shape load-bearing at 30 Hz.
2. **The guard carries a deadline.** Release it when the outstanding request is older than *N*
   intervals. D20's *"fixed interval + in-flight guard"* is **necessary but not sufficient as
   written** — released only on settlement it is the same latch with a shorter fuse. The shape is a
   companion timestamp, never a `finally`-only clear.
3. **Do NOT add a guard to `pollStatus`.** It has none (`app.js:426-432`), which is precisely why it
   is the one poll already safe: `setInterval` fires the next one regardless, so it leaks a pending
   promise per dropped tick and **self-heals**. That leak is bounded and acceptable; the latch is
   not. "Tidying" it would convert the safe path into the broken one.

§33 asserts the shape statically for every guard on the page. **NC5** proves it fires.

### P72 — Scene layout is **V3**: one row of ten, `STORE` in the group title row *(Q9, measured)*

| Variant | Scenes | Elevation | Strip | Button | Labels fit? |
|---|---|---|---|---|---|
| V1 — two rows | 115 px | 139 px | 552 × 92 | 85.3 px | ✅ |
| V2 — one row of 11 | 77 px | 177 px | 552 × 130 | 42.9 px | ❌ **clips by 0.2 px** |
| **V3 — one row of 10, `STORE` in the title row** | **82 px** | **172 px** | **552 × 125** | **48 px** | ✅ **4.9 px spare** |

Widest label ink is **33.1 px** (`FRONT` / `RIGHT` / `SIDES` / `STORE`) at the page's existing 10 px
mono. V2 misses by 0.2 px and would clip **silently, and only on those four labels** — the kind of
defect that ships. V3 fits every label and buys the strip 33 px over V1.

`STORE` in the title row costs **5 px, not 30**, because the title row already exists; it is a 20 px
toggle beside the `Scenes` heading. That placement also suits **D22** — an armed two-step control
reads better as a mode toggle on the group than as an eleventh button in a row of recall actions.

**The strip's real box is 552 × 125**, not Q8's assumed `582 × ~160`: 582 is the *column*, and the
group padding takes 30 px of it. Every 3.3 layout assertion is written against 552 × 125.

### P73 — Field sampling: **full chain, 32 × 40, base64 u8, one call, coalesced to one recompute per poll tick** *(Q1, Q2, N12)*

**The field follows the full chain** — `shaper::shape` → hull-project if outside → `dbap::solve` →
`hullTrimGain`. If it did not, it would match `dbap::solve` alone and **diverge from the plugin
outside the hull, where `hullAtten` is audible** — and the risk UI-04 exists to avoid is a picture
the solver does not produce. Either way the comparison is against shipping functions.

**The input set is five, not two.** UI-04/2 says *"geometry/weight change"*; `GainStage::updateControl`
shows three more:

| Input | Moves the field? | Automatable at audio rate? |
|---|---|---|
| speaker positions (venue) | yes | no — message thread only |
| `w1..w8` | yes | **yes** |
| `rolloff` → `a` | yes | **yes** |
| `blur` → `r_s` | yes | **yes** |
| `hullAtten` → hull trim | yes | **yes** |
| `srcX` / `srcY` / `srcZ` / `width` | **no** | — |

A literal "recompute on change" therefore makes a `blur` automation ramp recompute **every block**.
**Recompute is coalesced to at most one per poll tick**, and **UI-04/2's assertion stays on the
puck** — which is exactly right, because `srcX/srcY/srcZ/width` are genuinely not inputs.

**32 × 40, quantised to 8 bits, base64.** Measured against the shipping solver: `pow` is **not** the
constraint (112 × 140 = 125,440 `pow` in 660 µs), **the payload is** — 61 kB of JSON per recompute
through a bridge that serialises every value. 32 × 40 is **183 µs and 1.7 kB**, decoded with `atob`
into a `Uint8Array`, `putImageData` onto a 32 × 40 offscreen canvas, then `drawImage`-scaled onto
`#plan-backdrop`. That satisfies UI-04/3's *"offscreen canvas and blitted"* **directly rather than by
argument**, and the browser's own smoothing is what turns a coarse grid into a gradient.

**8-bit quantisation does not weaken UI-04/1.** The 1e-3 comparison is a **C++ unit probe** on the
sampler's **float** output, strictly upstream of transport. Asserting anywhere downstream would be
asserting against a JS re-derivation, which the criterion forbids.

### P74 — `FieldSampler.{h,cpp}` lands in the **fast unit target**, and field probes call `instr::resetCounters()` *(Q1)*

`dbap::solve` is a free function — no instance, no state, `noexcept`, no allocation, no JUCE. **It is
message-thread safe and needs no second instance.** `Source/DSP/FieldSampler.{h,cpp}` depends only on
`DbapSolver.h`, `Vec.h`, `HullProcessor.h` and `VenueSnapshot.h`, so it joins the unit target whose
link line is `juce_audio_basics + juce_core + juce_data_structures` and which already compiles
`DbapSolver.cpp`. UI-04/1's twenty-point comparison then runs in the **seconds-to-build** target
rather than behind a plugin — the same move **P56** made for `VenueFile.cpp` at 3.2.

**Two qualifications, both real:**

1. **Under `OOCTAGON_INSTRUMENT` a field sample pollutes `powCalls`**, and probe **AE** asserts
   `powCalls == 16` **exactly** per control block. A field probe sharing a process with AE **must**
   call `instr::resetCounters()` between them — the render harness already does this at eleven sites,
   so it is a convention to follow, not one to invent. **Probe CE asserts the discipline holds** so
   that a future probe added between them fails loudly instead of silently inflating AE.
   `solveRuns` is **not** at risk: `countSolveRun()` is in `GainStage::updateControl:287`, not inside
   `solve`.
2. **The recompute counter is C++-side**, because UI-04/2 requires a counter rather than an eye, and
   because D19's rule — assert against what C++ returned — applies here too.

**Gate 11 re-verifies the link line** rather than trusting this paragraph. Two TUs join the unit
target at 3.3 (`FieldSampler.cpp`, `SceneModel.cpp`), which is the state in which a narrow link line
quietly widens.

### P75 — The elevation guard is **fitted-box-vs-stage PLUS the ordering fact it depends on** *(N11 — D25 corrected)*

**D25's premise does not survive measurement; its conclusion does.** `CONTEXT-3.3` read
`controls.scrollHeight === clientHeight === 592` on an **unmodified** tree as proof the vacuous shape
was present. An unmodified tree has **nothing to overflow**, so both numbers read equal either way.
Measured with a real 120 px overflow, against all three candidate stage constructions:

| Stage construction | Column coarse | Fitted-box guard | Document §8 |
|---|---|---|---|
| A — plain `flex:1` | **FIRES** `699<=592` | FIRES `245<=125` | **PASSES** `720<=720` |
| B — `.plan-stage` clone | **FIRES** `639<=592` | FIRES `245<=125` | **PASSES** |
| C — absolute z-stack | **FIRES** `699<=592` | FIRES `245<=125` | **PASSES** |

**The method was controlled before the conclusion was drawn** — Part D of the study replays 3.2's NC3
on the venue rail through the same code path and reproduces `VERIFICATION-3.2` exactly (rail coarse
**PASSES** `592<=592` while the guard **FIRES** `375<=213`), so the difference is structural, not a
measurement artifact. The structural fact:

> **A flex container's `scrollHeight` grows only for overflow past its LAST child's margin edge.**
> `.miniplan` is child 2 of 5 in the rail; `#group-elevation` **is** the controls column's last child.

**Therefore the guard is required for a different and more durable reason.** The column-level
assertion is non-vacuous **only while the elevation group remains the last child** — insert anything
after it, at any future phase, and it silently becomes vacuous. So §21 asserts the fitted box against
its stage **and §22 asserts the ordering fact it depends on**. The NC asymmetry that proves the new
section non-redundant is **`[§8 passes] while [§21 fires]`** — one level up from where 3.2 found it.

### P76 — Elevation construction: three rules, each closing a specific trap *(Q8, measured)*

Derived from the measured 552 × 125 box and the default venue: depth **28.31 px/m**, height
**19.23 px/m**, ratio **0.68**. **No exaggeration factor is needed and none is applied** — two scales
is ordinary for a section drawing; what it requires is that the height axis be **labelled**, not
faked. The §OQ4 grading `4.50 → 5.40 m` spans **17.3 px** (legible), and `rakeRear` moves the rear at
**19.23 px/m** — a 0.5 m edit moves it **9.6 px**, an order of magnitude more than a probe needs.

1. **Draw the rake line only between `bbMinY` and `bbMaxY`.** `earHeight` **extrapolates linearly**
   outside that span (`VenueModel.h:173-177`), so a line across the whole envelope has **both** ends
   move when `rakeRear` moves — which breaks UI-05/1's negative half, whose whole point is that
   `earHeight(bbMinY) == rakeFront` for any `rakeRear` (RESEARCH-2.2 H5). Dash the extrapolated
   continuation into the margins if it is drawn at all. **NC7.**
2. **Derive the height axis from the venue and QUANTISE it to a 1 m step.** An axis that auto-fits
   `rakeRear` **rescales** when `rakeRear` changes, and UI-05/1 would then measure a rescale instead
   of a move. Quantising means an ordinary rake edit never moves the axis at all, and **the
   criterion's own "front endpoint unchanged" half is the guard**.
3. **The marker clamps; the numbers never do.** `srcZ` spans −2.0 … 8.0 m (`parameter-spec.md` row
   3), so absolute source height reaches ≈ 11.5 m — far above a 6.5 m axis. The marker clamps to the
   axis edge **with a chevron**; both numeric readouts stay exact. UI-05/2 already requires both
   readings shown, so the number is never the thing that is clamped.

**D26 stands:** depth horizontal (front left → rear right), height vertical, `FRONT`/`REAR` labelled
at the ends — because the plan's depth axis runs **top-to-bottom** and the strip's runs
**left-to-right**, rotated 90°, so the ends must be labelled rather than relying on a shared
orientation the two views do not have.

### P77 — Meters: sited in `processBlock` after `gainStage.process`, indexed through the **snapshot** *(Q4)*

The read is the **last statement in `processBlock`**, so it is genuinely "what leaves the plugin", and
it sits **after the ping's post-write overwrite** — which is what makes UI-03/2's cross-check against
verify-ping stepping 1 → 8 possible **at all**. `GainStage` is not touched, so **P24**'s *"this class
does not ask the processor anything"* survives.

```cpp
// after gainStage.process(...), still inside processBlock
for (int i = 0; i < 8; ++i)
{
    const int ch = mapped ? snapshot.speakerToBuffer[i] : i;   // the SNAPSHOT's map, not the member
    if (ch >= numOut) { continue; }
    const float pk = buffer.getMagnitude (ch, 0, buffer.getNumSamples());
    if (pk > meterPeak[i].load (std::memory_order_relaxed))
        meterPeak[i].store (pk, std::memory_order_relaxed);
}
```

- **`snapshot.speakerToBuffer`, never the processor member** — the block was rendered against the
  snapshot, and a venue edit can land between the two.
- **Identity attribution when unmapped is CORRECT and must not be "fixed."** Under `mapInvalid`,
  `GainStage`'s `else` arm writes `out[ch][n] = ch == 0 ? sL : sR`, so the meters show speaker 1 lit
  from L and speakers **2–8 all lit from R**. **That is the fold being visible**, which is the entire
  point of metering the output. Any 3.3 assertion about what an invalid map "retains" is made against
  the **snapshot**, never the output buffer (**N8**, second consumer).
- **`static_assert (std::atomic<float>::is_always_lock_free)` beside the array.** This project does
  not leave an invariant in prose (`pattern_ring_invariant_needs_static_assert`).
- **The load/compare/store race is benign and is documented rather than hardened.** With one
  audio-thread writer and a message-thread `exchange(0)` reader, the only reachable interleaving
  **re-publishes a peak that was already reported** — a duplicate on a max-hold display, never a lost
  peak. A CAS loop would buy nothing.
- **Two polls, not one.** `getStatus` stays at 2 Hz; a new `getMeters` runs at ~30 Hz. Separate for a
  measurable reason: `getStatus` builds a `juce::String` from
  `getBus(false,0)->getCurrentLayout().getDescription()` on **every call**
  (`PluginEditor.cpp:382-385`) — at 30 Hz that is 30 string constructions a second on the message
  thread for a value that changes only on renegotiation.
- **Linear peaks, not dB.** The −60..0 dBFS mapping and the ballistics both live in JS already;
  sending the raw measurement keeps the transform in one place.
- **`exchange(0.0f)` per speaker inside the native function**, per §4.3 amendment 2 — a dropped frame
  then **widens the measurement window instead of losing the peak**.
- **The ballistics coefficients are per-`requestAnimationFrame` frame** (attack 0.5, decay 0.12); the
  ~30 Hz poll refreshes only the **target**. The 1.5 s hold and 20 dB/s release are wall-clock from
  timestamps. Applying a per-frame coefficient on the poll clock is
  `pattern_block_rate_envelope_breaks_blocksize_invariance` in UI form.

### P78 — The scene write is **one C++ native function with eight gesture brackets** *(D18, Q6)*

`beginChangeGesture()` → `setValueNotifyingHost()` → `endChangeGesture()` on **each** of `w1..w8`.
`setValueNotifyingHost` **opens no gesture**, so without brackets Logic's Touch/Latch may move the
sound and **not record it** — invisible to build, `auval` and `pluginval` alike. This is the **third
and final site**, and it **closes `gesture_bracket_obligation`** after the 3.1 puck and the 3.2
preset load.

**In C++, in one function.** `PluginEditor.cpp:635-659` already does exactly this shape for
`loadPreset` across all 17 parameters, including the *closed on both paths* discipline. A JS-side
write through eight `SliderState`s would work, but it **scatters the bracket obligation across 24
messages** and puts D18's correctness somewhere no single grep can confirm. One `applyScene` call
site keeps it in one place, next to its precedent. The parameter echo still reaches the page —
`WebSliderParameterAttachment` listens to the parameter, so a C++-side `setValueNotifyingHost`
repaints the eight in-plan weight cells with **no extra plumbing**.

**D20's refusal lives in C++ too**, not only in the disabled control: `applyScene` returns
`{ok:false, reason:"emptyScene"}` — the same defence-in-depth `startPing`'s `mapInvalid` refusal
established at 3.2. The UI disabling the control is the **affordance**; the C++ refusal is the
**guarantee**.

**Q6's fade gate, as an operational definition:**

1. Apply scene A; read all 8 host-side values → `a`.
2. Apply scene B; read all 8 → `b`.
3. Write `w_i = 0.5·(a_i + b_i)` **directly to the parameters, bypassing the scene path entirely**.
4. Render a block; assert the eight per-speaker gains equal a **direct solve** for the blended vector
   — **and that no subsequent block re-asserts `a` or `b`**.

An implementation that stores "current scene" and re-applies it in `updateControl` **fails step 4**;
one that only writes parameters passes by construction. It runs entirely in the render harness and
needs no host. **This is the shape FUNC-06/6's *"the gate that catches an implementation that
latches"* was asking for.**

### P79 — Membership is **computed in C++ and returned whole**; the page performs no speaker arithmetic *(D19, Q10)*

D16's predicate — `classify(i) != INTERIOR ∧ |x−cx|/hx > |y−cy|/hy`, with `(cx,cy)` the **speaker
centroid** and `(hx,hy)` the **bbox half-spans** — evaluated exactly on the default venue:

```
ALL   {1,2,3,4,5,6,7,8}      LEFT  {1,6,7,8}
FRONT {1,2,3,8}              RIGHT {2,3,4,5}
REAR  {4,5,6,7}              SIDES {3,4,7,8}
```

`SIDES = {3,4,7,8}` reproduces D16, and **speakers 1 and 2 miss by 6.2 %** (`1.0617` vs `1.0000`) — a
property of this hall, not a defect, and FUNC-06/3's show-before-commit is what makes a 6 % margin
**visible instead of silent**. Speakers 3 and 8 are `ON_EDGE`, which is why the predicate must read
`!= INTERIOR` and **not** `== VERTEX`.

**A JS re-derivation is forbidden.** It is a mirrored fixture over **R1**, the highest-risk component
in the project, and it is what makes FUNC-06/2's permutation probe meaningful: a fixed-index
implementation must **fail** it. §32 asserts the absence statically (**NC2**).

**Membership rides `getVenueGeometry`** rather than taking a nineteenth native function — it is a
**pure function of the venue** and that payload already refreshes on `venueGen`, which removes a
whole staleness class. **User slots are not a venue function** and need their own read; **`scenesGen`
joins `getStatus`**, mirroring `venueGen`, so the page knows when to refetch.

`Source/Data/SceneModel.{h,cpp}` holds the predicate as a pure function so the **unit target** can
test it (probes CF, CG, CH) and the editor's `applyScene` consults the same function. One
implementation, two consumers.

### P80 — `SCENES` is a **sibling** of `VENUE`, normalised at `VENUE`'s two points *(D17, N13)*

**Sibling, never child**, is what makes FUNC-05 structural rather than disciplined.
`getStateInformation` is `apvts.copyState()` → XML, so a `SCENES` child of `apvts.state` is persisted
and restored with **no new code** — FUNC-06/4's session round-trip is structural.

**Two things are not free, and both have a `VENUE` precedent to copy rather than invent:**

1. **Normalisation.** `setStateInformation` calls `venue.writeToState (apvts.state)` after restoring,
   so a missing or partial node is written back complete and an older session is upgraded **exactly
   once**. `SCENES` needs the **identical treatment at the identical two points**, or every session
   written before 3.3 restores with no `SCENES` node and the four slots read as **absent rather than
   empty**. Probe **CK** drives a pre-3.3 session explicitly.
2. **The preset path is separate, and it is what D17 is about.** `applyPresetJson` iterates
   `processor.getParameters()` only and can never reach `apvts.state`'s children
   (`OuariconPresetManager.h:298-350`) — which is why FUNC-05 holds by construction. The **only**
   route from a preset to non-parameter state is `setCustomStateCallbacks`, and 3.3 makes the
   plugin's **first and only** such registration. **§27's assertion therefore changes shape**: from
   *"the symbol appears in zero of 24 files"* to *"**exactly one** registration exists and its body
   touches only `SCENES`."* A preset without scenes leaves the slots untouched rather than clearing
   them (`:346-349` calls `customLoad` **only when the property exists**) — verified in module source.

> **One trap in the same header.** `setStateFromXml` (`:592-604`) calls `customLoad` on a *different*
> condition and, above it, does `parameters.replaceState(...)` — which would replace the whole tree,
> **`VENUE` included**. O-Octagon does not use that path and **must not start**. §35 is a one-line
> gate: `PluginEditor.cpp` and `PluginProcessor.cpp` contain no `setStateFromXml` / `getStateAsXml`
> call site.

**Only the four user slots persist.** Named scenes are derived on demand, so there is nothing to go
stale when the venue moves.

### P81 — The native surface is exactly **EIGHTEEN**, and the stub gains a **call counter**

| # | Function | New? | Notes |
|---|---|---|---|
| 1–13 | the 3.2 surface | | unchanged |
| 14 | `getMeters` | ✚ | 8 linear peaks + `gen`; `exchange(0)` C++-side; ~30 Hz |
| 15 | `getScenes` | ✚ | the **4 user slots** only — 8 weights + occupied flag each — plus `scenesGen` |
| 16 | `applyScene` | ✚ | 8 gesture brackets; refuses an empty set |
| 17 | `storeScene` | ✚ | D22's capture into a slot |
| 18 | `getFieldGrid` | ✚ | `{cols, rows, minDb, maxDb, data: <base64>, computeCount}` |

**Named-scene membership rides `getVenueGeometry`** (P79). **UI-05 needs no new function** —
`getVenueGeometry` already carries per-speaker `z`, `rake.front`, `rake.rear`, `bbox.minY/maxY` and
the centroid, all landed by 3.2's **P55**. D15's affordability argument is confirmed in source.

**Three places must move together or §3 fails loudly**, which is the intended behaviour:
`ui_frontend_check.js:211`'s literal `registered.size === 13` → **`18`**; the stub's `NATIVE_FNS` map
(diffed **as a set**, both directions); and `getResource()` + `juce_add_binary_data` SOURCES for each
new page module. It will fail until all eighteen exist in all three, **exactly as the 3 → 13 move did
at 3.2** — a count that silently tracked whatever was registered would assert nothing at all.

**The enumeration hole is already closed and needs no widening.** P51 derives `PAGE_MODULES` from
`Source/ui/public/js/*.js` and §21 asserts set-equality against the CMake SOURCES, so 3.3's four new
modules land **automatically**; a file added on disk and forgotten in CMake fails §21 rather than
404-ing as a missing panel. **This is the seventh time this vacuity class would have bitten and the
first time it costs nothing.**

**One gap the stub does not model:** UI-04/2's puck-drag assertion needs the Playwright side to count
`getFieldGrid` **invocations**. The stub records `WRITES` and `GESTURES` but has **no call counter**.
Adding one to `getNativeFunction` is a few lines and belongs in the task that adds `getFieldGrid`.

### P82 — Probe accounting: **CA–CN**, fourteen new → **92**; JS gates **49 → 69 sections**

| Probe | Target | Requirement | What it measures |
|---|---|---|---|
| **CA** | unit | UI-04/1 | `outInvK` equals `√denom`; the 8 gains are **bit-identical** to the nullptr path |
| **CB** | unit | UI-04/1 | **20 grid points** vs a direct `solveSubPoint` + `hullTrimGain` chain, **to 1e-3** |
| **CC** | unit | UI-04/2 | Field **bitwise unchanged** across `srcX/srcY/srcZ/width` sweeps |
| **CD** | unit | UI-04/2 | Recompute counter increments on **all five** inputs and **not** on source moves |
| **CE** | unit | PERF-02 | `instr::resetCounters()` discipline: a field sample then AE still reads `powCalls == 16` |
| **CF** | unit | FUNC-06/2 | The six named sets on the default venue equal the D16 table **exactly** |
| **CG** | unit | FUNC-06/2 | **PERMUTATION** — indices rotated by `k`; `FRONT` returns the *now*-front speakers |
| **CH** | unit | FUNC-06/3 | **Proscenium fixture** → `SIDES == {}`, and the resolve reports it empty |
| **CI** | harness | FUNC-06/1 | 8 host-side values written; **8 begin/end gesture pairs** observed, none unclosed |
| **CJ** | harness | FUNC-06/6 | **The fade gate** — Q6's four steps; a latching implementation fails step 4 |
| **CK** | harness | FUNC-06/4 | `SCENES` round-trips; a **pre-3.3 session** normalises to a complete node once |
| **CL** | harness | FUNC-06/5 | The **42-value bit-compare re-run** after the `SCENES` node exists |
| **CM** | harness | UI-03/1,2 | Meters on a **NON-IDENTITY** map; ping 1 → 8 lights the matching index, 7 others zero |
| **CN** | harness | UI-03/4 | Probe **AO** re-run with metering live — **0 allocations** in `processBlock` |

**CG and CM are the two that carry the phase.** CG is the only probe a fixed-index implementation
fails; CM is the only probe a `v_i` meter fails. Every other scene and meter probe passes under both
defects — which is what makes these two the non-vacuity guards rather than additional coverage.

**JS sections:** `ui_frontend_check` **31 → 42** (new §32–§42; §3's literal and §27's shape are
**edits** to existing sections, not additions). `ui_layout_check` **18 → 27** (new §19–§30 consolidated
to nine sections; §0's timestamp is an edit).

| New section | Gate | Asserts |
|---|---|---|
| §32 | frontend | **The page performs no speaker arithmetic** — D19's executable form |
| §33 | frontend | Every in-flight guard releases on a **deadline**; no `finally`-only clear |
| §34 | frontend | The meter poll is a **fixed interval**, never `poll().then(poll)` |
| §35 | frontend | **No `setStateFromXml` / `getStateAsXml`** call site anywhere in O-Octagon source |
| §36 | frontend | Scene labels are **HTML-authored and rendered**; state via `data-*` + `aria-pressed` |
| §37 | frontend | Two polls at two rates; `getStatus` is **not** called at 30 Hz |
| §38 | frontend | `atob` → `putImageData` → `drawImage` — UI-04/3 structurally |
| §39 | frontend | UI-04 descope is a flag: backdrop is a separate layer, imported by nothing else |
| §40 | frontend | UI-05 descope is a flag: same shape |
| §41 | frontend | The rake line is drawn **only** `bbMinY` → `bbMaxY` |
| §42 | frontend | The height axis is **venue-derived and quantised** |
| §19 | layout | Ten scene buttons in one row; **every label's ink fits its box** |
| §20 | layout | `STORE` is in the **title row**, an arm toggle with `aria-pressed` |
| §21 | layout | **Fitted box vs its stage**, DPR 1 and DPR 2, backing store doubles |
| §22 | layout | **`#group-elevation` is the column's last child** — the ordering fact §21 depends on |
| §23 | layout | Eight meter arcs sited at their glyph positions, **outside** the glyph stroke |
| §24 | layout | Scene preview responds to **hover AND keyboard focus** (D21) |
| §25 | layout | An empty scene's control is **disabled and does not commit** (D20) |
| §26 | layout | `#plan-backdrop` has explicit `width`/`height` with a **DPR backing store** |
| §27 | layout | `getFieldGrid` invocation count **unchanged across N frames of puck drag** |

> §28/§29/§30 in the requirement-staging table above are the UI-05 assertions, folded into layout
> §21–§23's file as sub-checks of the elevation section rather than three further top-level sections.
> **The section count that gates is 27**, and it is the number Gate 3 asserts.

### P83 — **Eight negative controls, declared at plan**, each naming the gate it must make fire

Precedent P67 (six at 3.2). A negative control declared *after* the gate passes is a rationalisation.

| # | Injected defect | Must **FIRE** | Must still **PASS** |
|---|---|---|---|
| **NC1** | Oversize the elevation strip **120 px** past its stage | layout **§21** `245<=125` | **document §8** at `720<=720` — the asymmetry that proves §21 non-redundant |
| **NC2** | Re-derive scene membership in JS (a centroid/bbox comparison in the scene module) | frontend **§32** | frontend §19's single-projection rule, which such a re-derivation would not trip |
| **NC3** | Replace derived membership with fixed indices `{1,2,3,8}` | **CG** (permutation) | **every non-permuted scene probe** — which is exactly why CG is the guard |
| **NC4** | Meter `v_i` instead of the written buffer | **CM** on a non-identity map | UI-03/1's "eight indicators respond" — the 2.2 NC3 failure repeated |
| **NC5** | Remove the deadline from the meter guard; drop one completion | frontend **§33** + the runtime latch reproduces | the poll's own happy path, which never notices |
| **NC6** | Restore `max_i v_i²` as the field | **CB** | — and the single-weight case reads **`1.0000` everywhere**, reproducing N10 as a control |
| **NC7** | Draw the rake line across the whole envelope | **UI-05/1's negative half** — the front endpoint moves | UI-05/1's positive half, which a whole-envelope line passes |
| **NC8** | Insert a node **after** `#group-elevation` | layout **§22** | layout §21 — proving the coarse column assertion **would have gone vacuous** and §22 is what catches it |

**NC1, NC6 and NC8 are the three that could not have been written before research.** NC1's asymmetry
moved one level up (N11), NC6 exists only because the formula was measured (N10), and NC8 exists only
because the last-child rule was found (N11). The tree is **byte-identical** after all eight.

### P84 — `ui_layout_check.js` §0 emits a **machine-produced ISO-8601 UTC timestamp** *(D27)*

3.2 verify found that Gate 4's ordering claim — *the stub render ran before any C++* — rests on a
stamp **transcribed from console output**: a repo-wide search for `14:22:05` hits only planning prose,
never a machine-produced artifact. **One line makes the claim self-evidencing for every phase after
it.** It cannot repair 3.2's record and does not pretend to; Gate 4 at 3.3 is the first that carries
real evidence.

### P85 — The §R7 descope path stays live, and its cost is stated as a number

D15 ships both `nice` rows. If 3.3 runs long, the descope is still available and **costs a flag**:

- **UI-04** — `#plan-backdrop` is already a separate canvas layer (`index.html:119`, authored at 3.1
  for exactly this). Descoping means not calling the field module. §39 asserts nothing else imports
  it, so the cost is structural today and needs nothing built to make it true.
- **UI-05** — the strip is the controls column's last child and imported by nothing else (§40).
  Removing it returns 172 px of slack and touches no other component.

**`UI-03` is NOT on this path.** §R7 names it a defence on R1 and `REQUIREMENTS.md` marks it
explicitly not descopable. **`FUNC-06` is not either** — it is `should` and it is the phase's only
write path.

---

## Tasks

**The ordering is load-bearing in one place:** Tasks 6–15 are JS and run **against the stub, before
any 3.3 C++ exists**. That is what makes Gate 4 meaningful, and it is why P84's timestamp lands in
Task 15's file rather than at the end.

### Task 1 — `dbap::solve` gains `float* outInvK = nullptr` *(P69)*

- **Files:** `Source/DSP/DbapSolver.h`, `Source/DSP/DbapSolver.cpp`
- **Depends on:** none
- Write `std::sqrt(denom)` through the pointer when non-null; write `0.0f` on the all-zero-weight
  early return. **No new `pow`.** Document the P54 precedent in the header.
- **Every existing call site compiles unchanged** — do not touch one.

### Task 2 — `Source/DSP/FieldSampler.{h,cpp}` *(P73, P74)*

- **Depends on:** Task 1
- 32 × 40 grid over the venue envelope at `z = 0`; **full chain** (shape → project → solve → hull
  trim); float output; observed min/max returned alongside; C++-side **recompute counter**.
- Includes **only** `DbapSolver.h`, `Vec.h`, `HullProcessor.h`, `VenueSnapshot.h` — the narrow link
  line is the point.
- 8-bit quantisation + base64 is a **separate function**, downstream of the float output, so UI-04/1
  asserts upstream of it.

### Task 3 — `Source/Data/SceneModel.{h,cpp}` *(P79)*

- **Depends on:** none
- The six named predicates as **pure functions** of a venue: centroid, bbox half-spans,
  `classify(i) != INTERIOR ∧ |dx|/hx > |dy|/hy`. Returns a mask plus an `isEmpty` flag.
- One implementation, two consumers (unit probes and the editor's `applyScene`).

### Task 4 — Unit target CMake *(P74)*

- **Files:** `tests/unit/CMakeLists.txt`
- **Depends on:** Tasks 2, 3
- Add `FieldSampler.cpp` and `SceneModel.cpp`. **Gate 11 re-checks the link line** — no `juce_dsp`,
  no `juce_gui_extra` — because two TUs joining is when a narrow link line quietly widens.

### Task 5 — Probes **CA–CH** in `tests/unit/main.cpp` *(P82)*

- **Depends on:** Task 4
- Includes the **permutation fixture** (indices rotated by `k`, same eight physical positions) and
  the **proscenium fixture** (4 corners + 2 points on each of the front and rear edges — a
  *physically plausible* rig, which is what makes it a fair test of D20 rather than a contrived one).
- **CE** asserts the `resetCounters()` discipline, so a probe inserted between a field sample and AE
  fails loudly instead of silently inflating AE.

### Task 6 — `tests/ui-stub/juce-stub.js`: the 18-function surface **+ a call counter** *(P81)*

- **Depends on:** none
- Five new entries in `NATIVE_FNS`; a `CALLS` counter in `getNativeFunction` exposed alongside
  `WRITES`/`GESTURES`. §27 (layout) cannot be written without it.

### Task 7 — `index.html`: scenes group, elevation group, meter arcs *(P72, P75, P76)*

- **Depends on:** none
- V3: ten buttons in one row, `STORE` a toggle in the group **title row**. `#group-elevation` is the
  controls column's **last child** (§22 depends on it — and the comment must say so, or a future
  phase inserts after it and re-vacuums the coarse assertion).
- **Labels are authored in HTML.** State goes to `data-*` + `aria-pressed`, never `textContent`
  (`pattern_js_state_updater_overwrites_html_labels` — a ROADMAP test criterion in its own right).

### Task 8 — `css/styles.css` *(P72, P76)*

- **Depends on:** Task 7
- 48 px scene buttons, the 552 × 125 strip, the concentric meter arcs **outside** the glyph stroke —
  the glyph's own stroke already renders hull classification (dashed vs solid) and the two readings
  must not collide (**D23**).

### Task 9 — `Source/ui/public/js/scenes.js` *(P79, P71, D21, D22)*

- **Depends on:** Tasks 6, 7
- Hover **or keyboard focus** previews; click commits (**D21** — keyboard focus gives the identical
  preview, so FUNC-06/3 is not satisfied by a hover the user may never perform).
- `STORE` **arms**, the next slot click captures, and it **auto-disarms** after one capture (**D22**
  — deliberately asymmetric with D21, because recalling is reversible and overwriting a slot is not).
- **No speaker arithmetic.** Membership comes from the `getVenueGeometry` payload, whole.

### Task 10 — `Source/ui/public/js/meters.js` *(P77, P71)*

- **Depends on:** Tasks 6, 7
- Fixed 30 Hz interval, **deadline-released** in-flight guard, `requestAnimationFrame` ballistics
  with **separate current/target**, per-frame coefficients, wall-clock hold and release.

### Task 11 — `Source/ui/public/js/field.js` *(P73, P71)*

- **Depends on:** Task 6
- `atob` → `Uint8Array` → `putImageData` on a 32 × 40 **offscreen** canvas → `drawImage` onto
  `#plan-backdrop`. Explicit `width`/`height` with a **DPR backing store** — canvas is a replaced
  element (`o-textureforge-cursor-bug`).
- Legend prints the **actual dB span** (P69).

### Task 12 — `Source/ui/public/js/elevation.js` *(P76)*

- **Depends on:** Tasks 6, 7
- The three construction rules, each with the trap it closes named in a comment.

### Task 13 — `roomplan.js`: meter-arc hook, backdrop blit hook, scene-preview highlight

- **Depends on:** Tasks 10, 11
- Hooks only. The plan module stays the owner of the view transform (`metresToPx`, `makeView`).

### Task 14 — `app.js`: **repair `refreshGeometry`**, wire four modules, two polls *(P71, P81)*

- **Depends on:** Tasks 9–13
- **The repair is not optional and is not a 3.3 feature** — it is a live defect in shipped 3.2 code
  (N9). Deadline-released guard.
- **Do not add a guard to `pollStatus`.**

### Task 15 — `tests/ui_layout_check.js`: §0 timestamp + sections 19–27 *(P84, P75, P82)*

- **Depends on:** Tasks 7–14
- **RUN AGAINST THE STUB, BEFORE ANY 3.3 C++.** Record the machine-emitted timestamp — Gate 4.

### Task 16 — `PluginProcessor.{h,cpp}`: meter atomics + the `SCENES` node *(P77, P80)*

- **Depends on:** Task 3
- The eight `std::atomic<float>` with the `static_assert`; the read sited **after**
  `gainStage.process(...)`, indexed through `snapshot.speakerToBuffer`.
- `SCENES` normalisation at the **identical two points** `VENUE` uses.

### Task 17 — `PluginEditor.{h,cpp}`: the five new native functions *(P81, P78, P79)*

- **Depends on:** Tasks 2, 3, 16
- `applyScene`'s eight brackets, closed on **both** paths; the empty-set refusal; membership folded
  into `getVenueGeometry`; `scenesGen` onto `getStatus`.
- The **first and only** `setCustomStateCallbacks` registration, touching **only** `SCENES`.

### Task 18 — `CMakeLists.txt` + `getResource()` *(P81)*

- **Depends on:** Tasks 9–12
- Four new page modules in `juce_add_binary_data` SOURCES and four new `getResource` arms.
  **No filename contains a hyphen** (`critical_binary_data_strips_hyphens`).

### Task 19 — `tests/ui_frontend_check.js`: §3 → 18, §27's new shape, sections 32–42 *(P81, P80, P82)*

- **Depends on:** Tasks 17, 18

### Task 20 — Probes **CI–CN** in the render harness *(P82)*

- **Depends on:** Tasks 16, 17

### Task 21 — Re-run `ui_layout_check.js` against the **integrated** page

- **Depends on:** Tasks 18, 19
- The pre-integration run (Task 15) and this one must agree. A divergence means the stub and the real
  page disagree, which is the one thing the stub exists to prevent.

### Task 22 — The **eight negative controls** *(P83)*

- **Depends on:** Tasks 20, 21
- Each fired individually, each reverted, **tree byte-identical afterwards**.

### Task 23 — `REQUIREMENTS.md`: tick the four rows with named evidence

- **Depends on:** Task 22
- Every criterion gets its probe letter or section number **and its measured figure**, in the row it
  belongs to. `pattern_evidence_line_orphaned_past_next_heading`: **count `[x]` against `→ **` per
  section** before closing — an evidence line written past the next heading orphans itself and the
  ticked criterion reads as bare.

### Task 24 — Gates

*(see the gate table below)*

### Task 25 — `SUMMARY-3.3.md` + `STATUS.md`

- **Depends on:** Task 24
- Stage 3 closes here. The summary states **what did not run** as plainly as what did — specifically
  Q5's WKWebView half, which is Gate 13 and has been executed by **no one**.

---

## Gates

Every gate is **run at execute and RE-RUN FROM SCRATCH at verify**, never read out of
`SUMMARY-3.3.md`. This is the 2.3 discipline that caught four mis-attributions and the 3.1/3.2
discipline that caught six more.

| # | Gate | Pass condition |
|---|---|---|
| 1 | Clean 3-format build + both test targets, **forced full recompile** | exit 0, **zero `warning:` / `error:` / `FAILED`** |
| 2 | `node tests/ui_frontend_check.js` | exit 0, **42 sections** |
| 3 | `node tests/ui_layout_check.js` | exit 0, **27 sections** — and it must **not SKIP** |
| 4 | Stub render (Task 15) ran **before** any 3.3 C++ | the **machine-emitted** ISO-8601 stamp, in the artifact (P84) |
| 5 | `auval -v aufx OuOc OuDv` | **AU VALIDATION SUCCEEDED** |
| 6 | pluginval s10, VST3 ×3 / AU ×3 | all six exit 0, zero `FAILED` |
| 7 | Both C++ test targets | **92 probes, 0 failures**, exit 0 / exit 0 |
| 8 | `gen_dbap_reference.py --check` | exit 0, 102 cases — **re-run because Task 1 touched the solver** |
| 9 | 17 params vs `parameter-spec.md`, three sides | **17/17**; **3.3 adds none** |
| 10 | `createEditor` guard present; `PluginEditor.cpp` absent from the harness target | both ✓ |
| 11 | Unit-target link line — **no `juce_dsp`, no `juce_gui_extra`** | ✓ — **re-checked: `FieldSampler.cpp` and `SceneModel.cpp` joined it** |
| 12 | Contract checksums | **BRIEF / parameter-spec / ARCHITECTURE unmoved**; **`ROADMAP.md` at its NEW pin `643471ba…`**. Four pins, **two different** expected values — check the referent, not just the match |
| 13 | **Standalone launch, macOS — HUMAN, ~15 min** | the three new screens render at 1100 × 720; a scene previews on hover **and on keyboard focus** and commits; the meters follow a ping 1 → 8; the strip's rear moves when `rakeRear` does. **PLUS Q5's unrun half: hide the editor for 10 s with the meter poll live, re-show, and confirm the meters RESUME** |
| 14 | The **eight** negative controls | **all eight fired**; tree byte-identical afterwards |
| 15 | `node tests/tools/room_layout_study.js` | **re-run**; its numbers still agree with layout §19/§21 |
| 16 | `node tests/tools/venue_layout_study.js` | **re-run** — 3.3 does not touch the venue rail, and this is what proves it |

**Gate 13 grew from ~8 min to ~15 min, and the growth is Q5.** Research states plainly that a real
30 Hz poll against a hidden WKWebView **has been executed by no one** — the JS half is measured (N9)
and the JUCE half is read from source. It is a named gate item rather than a summary line that reads
as though it ran.

**Gate 13 is still not D5.** D5 is the ~15 min Logic hall session for QUAL-01's audible clause,
folded to Stage 4 and untouched here.

---

## Execution Constraints

1. **The meters measure the written buffer, never the solve.** A probe that meters `v_i` would light
   correctly under a **bypassed channel map** — the exact NC3 failure caught at 2.2.
2. **`mapInvalid`'s seven-lit state is correct and must not be "fixed."** Any assertion about what an
   invalid map retains is made against the **snapshot**, never the output buffer.
3. **`PERF-01` must not regress.** `buffer.getMagnitude()` allocates nothing, but UI-03/4 is a
   **measurement, not an argument** — probe AO re-runs with metering live.
4. **The gradient never recomputes per frame**, and the assertion is a **counter**, not an eye.
5. **The gradient is compared against the shipping `DbapSolver`**, never a JS re-derivation.
6. **Canvas is a replaced element** — explicit `width`/`height` with a DPR backing store, never
   `left` + `right` stretch. The layout gate already runs at both DPRs; the strip and the backdrop
   join it.
7. **No UI state may depend solely on a promise resolving** — and no guard may release solely on
   settlement (**P71**, which is the stronger form N9 forced).
8. **Every HTML-authored label stays authored in HTML.**
9. **The page-module registry is derived, not enumerated.** 3.3's four modules land automatically.
10. **The native-function surface grows to 18**, grep-diffed in both directions across all three
    sites.
11. **`FUNC-05`'s guarantee is re-measured, not inherited** (CL).
12. **278 px at 582 px wide**, and the strip's box is **552 × 125**. Both new components fit inside
    it or the phase reports a deviation.
13. **`juce::String` construction:** any new user-facing string with non-ASCII content is built with
    `<<` onto a **named local** — `juce::String(const char*)` is ASCII-only and
    `juce::String("...") << x` does not compile (it binds the private `operator bool`).
14. **MSVC:** any `constexpr` inside a lambda is `static`; no `SafePointer(this)` init-capture in a
    nested lambda. Windows CI is Stage 4, but the code is written now.

---

## Non-goals for Phase 3.3 — must not appear

- **Moving the venue-screen mini-plan** (D24). v1.1 backlog.
- **Repairing 3.2's Gate 4 ordering record.** P84 lands the timestamp; it cannot create a past record.
- **Seeding `VerifyPing`** (D28). No 3.3 probe needs a reproducible ping stream; FUNC-04/3 holds by
  construction via the `jlimit` hard clamp.
- **"Fixing" `pollStatus`** by adding an in-flight guard (P71 rule 3).
- **CI wiring.** Stage 4. 3.3 widens the gap a third time and says so.
- **The Logic hall session / D5 / `QUAL-01`'s audible clause.** Stage 4.
- **`COMPAT-02`, `COMPAT-04`.** Stage 4.
- **Any change to the 17 musical parameters or the 42 venue values.** `parameter-spec.md` is pinned
  and unmoved; scenes write **existing** parameters and store nothing new in `VENUE`.
- **A second `DbapSolver` instance.** Q1 answered: none is needed and none exists to need.
- **A JS re-derivation of anything the C++ side can return.**

---

## Success Criteria

- [ ] **`FUNC-06` closed** — all 6 criteria, including CG's permutation probe and CJ's fade gate
- [ ] **`UI-03` closed** — all 4 criteria, including CM's ping cross-check on a **non-identity** map
- [ ] **`UI-04` closed** — all 4 criteria, against `1/k` and the shipping solver to 1e-3
- [ ] **`UI-05` closed** — all 4 criteria, including UI-05/1's **negative half**
- [ ] **Stage 3 complete** — nine rows closed across 3.1 / 3.2 / 3.3, **zero partials**
- [ ] **92 probes, 0 failures**, both targets
- [ ] **69 JS gate sections**, both gates green, layout not SKIPped
- [ ] **Eight negative controls fired**, tree byte-identical afterwards
- [ ] **N9's live 3.2 defect repaired** and proven repaired by NC5
- [ ] **`gesture_bracket_obligation` closed** — third and final site
- [ ] **Contract checksums:** three unmoved, `ROADMAP.md` at its new pin
- [ ] **Gate 13 run by a human**, including Q5's hidden-editor test

---

## Risks Active in This Phase

| Risk | Signature if it lands | What catches it |
|---|---|---|
| **A meter lights the wrong speaker** | Entirely plausible on screen | **CM** on a non-identity map + the ping cross-check. **NC4** proves CM is the guard |
| **A scene selects the wrong four** | On the traced layout a fixed-index impl looks **correct** | **CG**. **NC3** proves it is the only probe that fails |
| **The gradient draws a field the solver does not produce** | Beautiful and wrong | **CB** at 1e-3. **NC6** reproduces N10 as a control |
| **The gradient is an information-free wash** | Looks like a gradient, carries nothing | P69's per-recompute min/max normalisation + a dB legend |
| **The strip's height axis rescales instead of moving** | UI-05/1's positive half passes | P76's quantised axis + the criterion's **own negative half**. **NC7** |
| **The 278 px does not hold** | The strip overflows | §21's fitted-box guard. **NC1**'s asymmetry proves it non-redundant |
| **A later phase inserts after `#group-elevation`** | The column assertion **silently goes vacuous** | **§22** asserts the ordering fact. **NC8** |
| **The meter poll dies silently when the editor is hidden** | Meters frozen; no error anywhere | P71's deadline guard. §33 + **NC5** + Gate 13's Q5 item |
| **A pre-3.3 session's slots read as absent, not empty** | Silent, and only on upgrade | **CK** drives a pre-3.3 session explicitly |
| **Stage 3 runs long** | The hall test slips | **P85** — the descope path stays live and costs a flag |

---

## Next Phase

**Ready for:** `execute`

**25 tasks. The two that carry the phase are Task 5 (CG) and Task 20 (CM)** — the permutation probe
and the post-map meter cross-check are the only two assertions in 3.3 that a plausible-looking wrong
implementation fails. Everything else passes under both defects.

**The one ordering constraint:** Tasks 6–15 run against the stub **before** any 3.3 C++ exists, and
Task 15 emits the machine-produced timestamp that makes Gate 4 evidence rather than transcription.

**Closing `FUNC-06`, `UI-03`, `UI-04` and `UI-05` completes Stage 3.**
