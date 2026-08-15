# Stage 3 — GUI · Phase 3.3 (Scenes, meters, gradient, elevation) — Research

**Plugin:** O-Octagon
**Stage:** 3 of 3 — GUI · **Phase 3.3 of 3**
**GSD phase:** research
**Date:** 2026-08-12
**Branch:** `feat/o-octagon` @ `a47cef88` (2.2 / 2.3 / 3.1 / 3.2 work uncommitted)
**Answers:** the eleven questions in `CONTEXT-3.3.md`
**Sources:** O-Octagon `Source/`, JUCE 8.0.14 in-tree, `modules/persistence/preset-manager` v1.0.5,
one **measured** Chromium render at 1100 × 720 (`tests/tools/room_layout_study.js`), and two
**measured** C++ benchmarks against the shipping `DbapSolver.cpp`

---

## Entry Check — the four contracts

Re-run at this boundary, before anything else (`pattern_promotion_checksum_pins_replaced_file`):

| Contract | SHA-256 measured now | STATUS frontmatter | Result |
|---|---|---|---|
| `BRIEF.md` | `697a4f32…f6b9fbd6` | `697a4f32…f6b9fbd6` | ✅ |
| `parameter-spec.md` | `b45f88dc…cbb9e02f` | `b45f88dc…cbb9e02f` | ✅ |
| `research/ARCHITECTURE.md` | `32a85018…81d85273` | `32a85018…81d85273` | ✅ **the new 3.3-discuss pin** |
| `ROADMAP.md` | `aec7d0ce…0137ee29` | `aec7d0ce…0137ee29` | ✅ |

**All four byte-exact.** The architecture pin moved at 3.3 discuss (four amendments) and is measured
here at its **new** value, which is the check that amendment actually landed. **No contract is
amended and no pin moves at this boundary** — but **N10 below requires a `ROADMAP.md` amendment at
the plan boundary**, and it is flagged rather than taken here, exactly as 3.1 research scheduled the
§8 re-pins rather than taking them.

---

## Executive summary — what this research changes

Five findings could not have been known at discuss. **Three of them change a decision, and one of
those is a live defect in shipped 3.2 code.** Two correct a premise carried in from discuss.

| # | Finding | Consequence |
|---|---|---|
| **N9** | **A single dropped completion permanently latches `app.js`'s in-flight guard.** `refreshGeometry()` clears `geometryFetchInFlight` in a `finally` (`app.js:379-399`); a promise that never settles never runs its `finally`. **MEASURED on the shipping page**: after one dropped `getVenueGeometry`, the envelope readout stays at `15.60 × 19.50 m` while the venue is `39.00 × 52.00 m`, and it never recovers — through five further poll ticks with the transport restored | **The N4 hazard is already live, in 3.2 code, and it is worse than "the meters freeze."** D20's "fixed interval + in-flight guard" is **not sufficient as stated** — the guard must be released by something other than settlement. 3.3 must fix `refreshGeometry` as well as build the meter poll correctly |
| **N10** | **`max_i v_i²` is degenerate.** MEASURED across the envelope with the shipping solver: **identically 1.0000 everywhere when one weight is non-zero**, and only 3.2–5.4 dB of contrast otherwise | **The `ROADMAP.md` formula for UI-04 is disqualified** — the picture goes blank exactly when the spatial situation is most extreme. The un-normalised DBAP field `1/k` gives 1.3–10.4 dB with correct radial structure and **never** degenerates. Needs a `ROADMAP` amendment at plan and one defaulted out-param on `dbap::solve` |
| **N11** | **D25's premise is over-attributed.** `controls.scrollHeight === clientHeight === 592` on an *unmodified* tree is not evidence of vacuity — nothing overflows, so both read equal. MEASURED with a real 120 px overflow, the coarse column assertion **FIRES** (`699<=592`) in all three candidate stage constructions | **The NC3 asymmetry does not reproduce on this column.** The genuinely vacuous assertion here is the **document-level §8**, which passed at `720<=720` in every run. D25's *conclusion* survives; the negative control that proves it must be built against §8, not against the column |
| **N12** | The field depends on **`rolloff`, `blur` and `hullAtten`** as well as geometry and weights (`GainStage.cpp` `updateControl`: `a = rolloffToAlpha(rolloff)`, `rs = blurToRadius(blur, rigScale)`, then `hullTrimGain(hullAtten, d_hull)`) | **UI-04/2 names only two of five inputs.** Three of the five are automatable at audio rate, so "recompute on change" needs a coalescing rule or a rolloff ramp recomputes every block |
| **N13** | `SCENES` as a **sibling** of `VENUE` rides `apvts.copyState()` for free (`PluginProcessor.cpp` `getStateInformation`) | **FUNC-06/4's session round-trip needs no new code** in `get`/`setStateInformation` — but it does need the `writeToState` **normalisation** `VENUE` already has, or every session written before 3.3 leaves the four slots unreadable |

**Measured, not estimated (Q8 / Q9):** the discuss budget of **278 px** is confirmed on the rendered
page, and the strip's real box is **552 × 125 px** — *not* the `582 × ~160` Q8 assumed. One row of
ten scene buttons with `STORE` in the group title row is the only arrangement that both fits its
labels and leaves the strip above 100 px. **A true-scale strip needs no exaggeration**, and the
criterion's own negative half is what guards the height axis against rescaling.

**What was NOT run, and is not claimed:** Q5's WKWebView half. The JS half of N4 is now *measured*
(N9) and the JUCE half is *read from source* (3.2 N4, re-verified). Confirming that a real WKWebView
drops a 30 Hz completion when hidden requires a running plugin with the meter poll in it, which does
not exist until execute. **It is specified below as a named execute-phase item, not answered here.**

---

## 1. Findings verified in source and by measurement

### N9 — one dropped completion latches the guard permanently (Q5)

`app.js`'s geometry cache is the pattern D20 proposes to reuse for the meters:

```js
// app.js:379-399
async function refreshGeometry() {
  if (geometryFetchInFlight) return;
  geometryFetchInFlight = true;
  try {
    const payload = await nativeFn("getVenueGeometry")();
    …
  } catch (err) {
    console.error("getVenueGeometry failed", err);
  } finally {
    geometryFetchInFlight = false;      // <-- never runs if the await never settles
  }
}
```

N4 established that a completion is **dropped, not rejected**, when the browser is hidden
(`juce_WebBrowserComponent.cpp:336-344, 607-611`). A dropped completion is not an exception and not a
rejection, so **neither `catch` nor `finally` runs** — the coroutine is suspended forever and
`geometryFetchInFlight` stays `true` for the life of the page.

**This was measured, not argued.** The shipping page was served with the real stub, the
`getVenueGeometry` transport was made to return a never-settling promise (precisely what JUCE does
when hidden), the venue was moved, and the transport was then restored:

| Run | After the venue moves to 39.00 × 52.00 m | 5 poll ticks after the transport is restored |
|---|---|---|
| **control** (no drop) | `39.00 × 52.00 m` ✅ | `39.00 × 52.00 m` |
| **one dropped completion** | `15.60 × 19.50 m` ❌ | `15.60 × 19.50 m` — **never recovers** |

Three consequences, and the first is not a 3.3 design question at all:

1. **This is a live defect in shipped 3.2 code.** Hiding the editor once during a venue change leaves
   the Room plan, the Venue table, the envelope readout and the metres readout permanently frozen on
   a stale venue, with no error anywhere. It is not caused by 3.3 and it must be repaired by 3.3,
   because 3.3 is what makes the same shape load-bearing at 30 Hz.
2. **D20's rule is necessary but not sufficient as written.** A fixed interval with an in-flight
   guard degrades to dropped frames *only if the guard is released independently of settlement*. The
   guard must carry a **deadline** — release it when the outstanding request is older than N
   intervals — or it is the same latch with a shorter fuse.
3. **The 2 Hz `getStatus` poll is the one place that is already safe**, and now for a stated reason:
   `pollStatus()` has no in-flight guard at all (`app.js:426-432`), so `setInterval` fires the next
   one regardless. It leaks a pending promise per dropped tick and self-heals. **That leak is
   bounded and acceptable; the latch is not.** Do not "fix" `pollStatus` by adding a bare guard.

**Rule for plan:** *every* in-flight guard on this page releases on a deadline, never on settlement
alone. That covers `refreshGeometry`, the new meter poll, and the field-grid fetch.

### N10 — `max_i v_i²` is the wrong field, and it is measurably wrong (Q3)

`ROADMAP.md` Phase 3.3 specifies *"per-pixel `max_i v_i²` over a coarse grid"*. Measured over the
default envelope on a 32 × 40 grid with the shipping `dbap::solve`:

| Configuration | min | max | contrast |
|---|---|---|---|
| ALL (default patch) | 0.1466 | 0.4135 | 4.5 dB |
| FRONT `{1,2,3,8}` | 0.2589 | 0.5469 | 3.2 dB |
| SIDES `{3,4,7,8}` | 0.2583 | 0.5552 | 3.3 dB |
| rolloff 6.0 (max) | 0.1535 | 0.5337 | 5.4 dB |
| rolloff 3.0 (min) | 0.1396 | 0.2979 | 3.3 dB |
| **single speaker, `w1` only** | **1.0000** | **1.0000** | **0.0 dB** |

The last row is the disqualifying one, and it is structural rather than a tuning problem. DBAP
normalises to `Σ v_i² = 1`, so `max_i v_i²` measures only *how concentrated* the image is —
`1/8` when energy is spread evenly, `1` when it is all in one speaker. With one active weight it is
`1` at **every point in the room**, carrying no information about where that speaker is. The
gradient would go blank precisely when the operator most needs it.

**The un-normalised DBAP field is the right quantity.** `dbap::solve` already computes it as
`denom` before normalising (`DbapSolver.cpp:76, 90`): `1/k = sqrt(Σ_i (w_i · d_i^{-a})²)`. Measured
over the same grid:

| Configuration | min | max | contrast |
|---|---|---|---|
| ALL (default patch) | 0.44619 | 0.55445 | 1.9 dB |
| FRONT `{1,2,3,8}` | 0.24214 | 0.45682 | **5.5 dB** |
| REAR `{4,5,6,7}` | 0.23110 | 0.45998 | **6.0 dB** |
| SIDES `{3,4,7,8}` | 0.28950 | 0.43607 | 3.6 dB |
| **single speaker, `w1` only** | 0.09853 | 0.32484 | **10.4 dB**, clean radial falloff |
| rolloff 6.0 / 3.0 | — | — | 2.5 dB / 1.3 dB |

It is source-independent (satisfying UI-04/2), weight- and geometry-dependent, a genuine **level**
field in dB, and it never degenerates. **It is also exactly what the shipping solver produces**,
which is what makes UI-04/1's *"compared against a direct solve to 1e-3"* literally satisfiable
rather than approximately.

**How to reach it without a second implementation.** Recomputing `denom` in a field sampler is a
mirrored fixture over the highest-risk arithmetic in the plugin
(`pattern_test_fixture_mirrors_drift_silently`). Instead, add a **defaulted out-param** to the
solver — the exact `MapDiagnosis* whyNot = nullptr` precedent P54 already set in this codebase:

```cpp
void solve (const Vec3 spk[kNumSpeakers], const float w[kNumSpeakers], Vec3 src,
            float a, float rs, float outV[kNumSpeakers],
            float* outInvK = nullptr) noexcept;   // sqrt(denom); nullptr on the audio path
```

Every existing call site compiles unchanged, no new `pow` is introduced, `powCalls == 16` is
untouched, and the all-zero-weight early return writes `0.0f` — which is the correct field value for
a rig with no active speakers.

> **Both figures share a cause worth stating.** Every grid point sits at `z = 0` while the speakers
> are 4.50–5.40 m up, so the minimum 3-D distance from any audience point to any speaker is ≥ 4.5 m
> in a 12 × 15 m hall. **The DBAP field over a real raked audience plane genuinely is flat.** That is
> a property of the rig, not of either formula, and it means **any absolute 0..1 colour mapping
> renders as a near-uniform wash.** The backdrop must normalise to the per-recompute observed
> min/max and print the actual dB span in a legend — otherwise the picture carries no information
> while looking as though it does, which is the "beautiful and wrong" risk `CONTEXT-3.3` named,
> arriving from an unexpected direction.

**Disposition:** this is a `ROADMAP.md` deviation. `REQUIREMENTS.md`'s four UI-04 criteria never name
the formula, and the ROADMAP's own test criterion (*"compare against a direct solve to 1e-3"*) is
formula-agnostic — **only the ROADMAP's component bullet names `max_i v_i²`.** Research moves no pin.
**Plan takes the decision and amends `ROADMAP.md` with a re-pin at the plan boundary**, precedent
2.2 D2 / 2.3 D2 / 3.3-discuss.

### N11 — D25's premise is over-attributed; the vacuous assertion is §8, not the column (Q11)

`CONTEXT-3.3` reports that the discuss measurement returned
`controls.scrollHeight === controls.clientHeight === 592` and reads that as *"D-2's exact signature,
confirmed PRESENT on 3.3's own target column TODAY."*

**An unmodified tree has nothing to overflow, so both numbers read equal whether or not the
assertion is vacuous.** The measurement that distinguishes them is the one with an actual overflow
in it, and it was not taken at discuss. Taken here — the strip forced 120 px past its stage, against
all three candidate stage constructions:

| Stage construction | Overflow | Column `scrollHeight <= clientHeight` | Fitted-box guard | Document §8 |
|---|---|---|---|---|
| A — plain `flex:1`, canvas in flow | +120 px | **FIRES** `699<=592` | FIRES `245<=125` | **PASSES** `720<=720` |
| B — exact `.plan-stage` clone (centred flex) | +120 px | **FIRES** `639<=592` | FIRES `245<=125` | **PASSES** |
| C — relative stage, absolutely-positioned canvas | +120 px | **FIRES** `699<=592` | FIRES `245<=125` | **PASSES** |

**The coarse column assertion is not vacuous on this column.** What *is* vacuous — in every run, at
both DPRs — is the **document-level** assertion, which sat at `720<=720` throughout.

**The method was controlled before the conclusion was drawn.** Part D of the study replays 3.2's NC3
on the venue rail against the same code path, and reproduces `VERIFICATION-3.2`'s verified result
exactly: rail coarse **PASSES** `592<=592` while the guard **FIRES** `375<=213`. So the difference
between the rail and the column is structural, not a measurement artifact — and the study reports
the structural fact that explains it:

> **A flex container's `scrollHeight` grows only for overflow past its LAST child's margin edge.**
> `.miniplan` is child 2 of 5 in the rail (measured: `miniIsLastChild: false`), so its overflow
> spills into space the scroll box already covers and `scrollHeight` never moves.
> `#group-elevation` **is** the controls column's last child, so its overflow extends past the
> content edge and `scrollHeight` does move.

**D25's conclusion survives and its justification changes.** The fitted-box-against-its-stage guard
is still required — but for a different and more durable reason: the column-level assertion is
non-vacuous **only while the elevation group remains the last child**. Insert anything after it, at
any future phase, and it silently becomes vacuous. So the gate must assert **the ordering fact it
depends on**, not merely the measurement — and the NC3-style asymmetry that proves the new guard
non-redundant is `[§8 passes] vs [the new section fires]`, one level up from where 3.2 found it.

### N12 — the field has five inputs, and UI-04/2 names two

Read from `GainStage::updateControl`, the solve chain per sub-point is
`shaper::shape` → `solveSubPoint` (hull-project if outside, then `dbap::solve`) → `hullTrimGain` →
`trimLin`. The inputs that move the **field** — as opposed to the source position — are therefore:

| Input | Moves the field? | Automatable? |
|---|---|---|
| speaker positions (venue generation) | yes | no — message thread only |
| `w1..w8` | yes | **yes** |
| `rolloff` → `a` | yes | **yes** |
| `blur` → `r_s` | yes | **yes** |
| `hullAtten` → hull trim | yes, if the field follows the full chain | **yes** |
| `srcX` / `srcY` / `srcZ` / `width` | **no** | — |

UI-04/2 says *"on geometry/weight change only"*. Three further inputs exist and all three are
automatable at audio rate, so a literal "recompute on change" makes a `blur` automation ramp
recompute the field every block. **The criterion's testable claim is the puck one** — *"dragging the
puck for N frames must leave the recompute count unchanged"* — and that is exactly right, because
`srcX/srcY` are genuinely not inputs. Plan should specify the input set as the five above, with the
recompute **coalesced to at most one per poll tick**, and keep the assertion on the puck.

**The sampling contract must also be pinned.** If the field applies hull projection and the hull
trim, it matches what the audience hears and UI-04/1 compares against `solveSubPoint` +
`hullTrimGain`; if it does not, it matches `dbap::solve` alone and diverges from the plugin outside
the hull, where `hullAtten` is audible. **Recommendation: follow the full chain**, because the risk
UI-04 exists to avoid is a picture the solver does not produce — and either way the comparison is
against shipping functions, never a re-implementation.

### N13 — `SCENES` rides `copyState()`, but needs `VENUE`'s normalisation

`getStateInformation` is `apvts.copyState()` → XML (`PluginProcessor.cpp`), so a `SCENES` child of
`apvts.state` is persisted and restored with **no new code** — the same property the comment there
already claims for `VENUE`. FUNC-06/4's `getStateInformation`/`setStateInformation` round-trip is
therefore structural.

Two things are **not** free, and both have a `VENUE` precedent to copy rather than invent:

1. **Normalisation.** `setStateInformation` calls `venue.writeToState (apvts.state)` after restoring,
   so a missing or partial `VENUE` node is written back complete and a pre-2.1 session is upgraded
   exactly once. `SCENES` needs the identical treatment at the identical two points, or every
   session written before 3.3 restores with no `SCENES` node and the four slots read as absent
   rather than as empty.
2. **The preset path is separate and is the one D17 is about.** `applyPresetJson` iterates
   `processor.getParameters()` only and can never reach `apvts.state`'s children
   (`OuariconPresetManager.h:298-350`) — which is why FUNC-05 holds by construction. The **only**
   route from a preset to non-parameter state is `setCustomStateCallbacks`, verified in module source:
   `savePresetJson` writes `customSave()` under `"customState"` (`:287-290`), and `applyPresetJson`
   calls `customLoad` **only when that property exists** (`:346-349`) — so a preset without scenes
   leaves the slots untouched rather than clearing them. Good.

> **One trap in the same header.** `setStateFromXml` (`:592-604`) calls `customLoad` on a *different*
> condition and, above it, does `parameters.replaceState(...)` — which would replace the whole tree,
> `VENUE` included. **O-Octagon does not use that path** (it implements `get`/`setStateInformation`
> itself) and must not start. Worth a one-line gate: `PluginEditor.cpp` and `PluginProcessor.cpp`
> contain no `setStateFromXml` / `getStateAsXml` call site.

---

## 2. The layout, MEASURED (Q8 / Q9)

Tool: `tests/tools/room_layout_study.js` — precedent `tests/tools/venue_layout_study.js` (3.2 Q11).
It serves the **real** `Source/ui/public` with the bridge stub swapped in, byte-identical to what
`ui_layout_check.js` serves, lets the shipping modules run, and only then injects the candidates.
No row arithmetic (`pattern_flex1_container_slack_invisible_to_row_sum`).

**Baseline reproduced:** `.controls-column` 582 × 592, four groups end at y = 386, **slack 278 px** —
`CONTEXT-3.3`'s figure, confirmed independently.

### Q9 — where the ten scene buttons go

| Variant | Scenes group | Elevation group | **Strip** | Button | Labels fit? |
|---|---|---|---|---|---|
| V1 — 2 rows: 6 named / 4 user + STORE | 115 px | 139 px | **552 × 92** | 85.3 px | ✅ |
| V2 — 1 row of 11 | 77 px | 177 px | **552 × 130** | 42.9 px (content 32.9) | ❌ **clips** |
| **V3 — 1 row of 10, `STORE` in the title row** | **82 px** | **172 px** | **552 × 125** | **48 px** (content 38) | ✅ |

Widest label ink is **33.1 px** (`FRONT` / `RIGHT` / `SIDES` / `STORE`) at the page's existing 10 px
mono. V2 misses by 0.2 px — it would clip, silently and only on those four labels. **V3 is the
recommendation:** it fits every label with 4.9 px to spare and buys the strip 33 px over V1.

`STORE` in the group title row costs **5 px**, not 30, because the title row already exists; it is a
20 px toggle beside the `Scenes` heading. That placement also happens to suit D22 — an armed
two-step control reads better as a mode toggle on the group than as an eleventh button in a row of
recall actions.

### Q8 — does the elevation strip fit, and is true scale legible?

**Q8's premise was wrong in the helpful direction on width and the unhelpful one on height.** The
real box is **552 × 125 px**, not `582 × ~160`: 582 is the *column*, and the group's padding takes
30 px of it.

Derived from the measured box and the default venue (envelope `15.60 × 19.50 m`, bbox
y `[4.50, 19.50]`, `rakeFront 1.10`, `rakeRear 3.20`, speaker z `4.50 → 5.40`):

| Quantity | Value |
|---|---|
| Depth scale (envelope y-span 19.50 m over 552 px) | **28.31 px/m** |
| Height scale (axis 0 … 6.5 m over 125 px) | **19.23 px/m** |
| Height : depth | **0.68** |
| Rake line, `bbMinY` → `bbMaxY` | x = 63.7 → 488.4 px — **77 % of the strip** |
| `rakeFront 1.10` / `rakeRear 3.20` | 21.2 px / 61.5 px above the baseline |
| Speaker heights 4.50 → 5.40 m | 86.5 → 103.8 px; the §OQ4 grading spans **17.3 px** |
| **`rakeRear` sensitivity** | **19.23 px per metre** — a 0.5 m edit moves the rear **9.6 px** |

**No exaggeration factor is needed and none should be applied.** The two axes simply carry different
scales, which is ordinary for a section drawing; what it requires is that the height axis be
**labelled**, not that it be faked. The §OQ4 graded heights are legible at 17.3 px, and UI-05/1's
`rakeRear` change is visible by an order of magnitude more than the 1 px a probe needs.

Three construction rules fall out, and each closes a specific trap:

1. **Draw the rake line only between `bbMinY` and `bbMaxY`.** `earHeight` extrapolates linearly
   outside that span (`VenueModel.h:173-177`), so a line drawn across the whole envelope has **both**
   ends move when `rakeRear` moves — which breaks UI-05/1's negative half, whose whole point is that
   `earHeight(bbMinY) == rakeFront` for any `rakeRear` (RESEARCH-2.2 H5). Dash the extrapolated
   continuation into the margins if it is drawn at all.
2. **Derive the height axis from the venue, and quantise it.** An axis that auto-fits `rakeRear`
   rescales when `rakeRear` changes, and UI-05/1 would then measure a rescale instead of a move.
   Quantising the top to a 1 m step means an ordinary rake edit never moves the axis at all.
   **The criterion's own negative half is the guard**: assert the rear endpoint moved **and** the
   front endpoint did not — a rescaling axis fails the second half.
3. **`srcZ` spans −2.0 … 8.0 m** (`parameter-spec.md` row 3), so absolute source height reaches
   ≈ 11.5 m — far above a 6.5 m axis. The marker **clamps to the axis edge with a chevron**, and both
   numeric readouts stay exact. UI-05/2 already requires both readings shown, so the number is never
   the thing that is clamped.

### Q11 — the negative control, and what it must be built against

Measured, at **DPR 1 and DPR 2**, for the V3 candidate:

| | Strip | Backing store | Column coarse | Fitted-box guard | Document §8 |
|---|---|---|---|---|---|
| as designed, DPR 1 | 552 × 125 | 552 × 125 | PASS `592<=592` | PASS `125<=125` | PASS `720<=720` |
| **+120 px, DPR 1** | 552 × 245 | 552 × 245 | **FIRE** `699<=592` | **FIRE** `245<=125` | **PASS** |
| as designed, DPR 2 | 552 × 125 | **1104 × 250** | PASS | PASS | PASS |
| **+120 px, DPR 2** | 552 × 245 | **1104 × 490** | **FIRE** | **FIRE** | **PASS** |

The DPR-2 backing store doubles correctly, which is the `o-textureforge-cursor-bug` check the
existing gate already runs at both DPRs (§6) and which the strip must join.

**The asymmetry that proves the new section non-redundant is `[§8 passes] while [the new section
fires]`** — see N11. Per gate family:

| Family | Negative control | What must fire | What must still pass |
|---|---|---|---|
| Playwright (`ui_layout_check`) | oversize the elevation strip past its stage by 120 px | the new fitted-box section | **§8** at `720<=720` — measured, in all six runs |
| Static (`ui_frontend_check`) | re-derive scene membership in JS (a `centroid`/`bbox` comparison in the scene module) | a new "the page performs no speaker arithmetic" section — the executable form of **D19** | §19's single-projection rule, which such a re-derivation would not trip |
| C++ probes | replace derived membership with fixed indices `{1,2,3,8}` | FUNC-06/2's **permutation** probe | every non-permuted scene probe — which is exactly why the permutation fixture is the non-vacuity guard |
| C++ probes | meter `v_i` instead of the written buffer | UI-03/2's ping cross-check on a **non-identity** map | UI-03/1's "eight indicators respond" — which a `v_i` meter passes, and which is the NC3 failure repeated |

---

## 3. The six named scenes, computed on the default venue (Q7 / Q10)

D16's predicate — `classify(i) != INTERIOR ∧ |x−cx|/hx > |y−cy|/hy`, with `(cx, cy)` the **speaker
centroid** and `(hx, hy)` the **bbox half-spans** — evaluated exactly:

`centroid (6.5000, 12.4625)`, `hx = 6.000`, `hy = 7.500`, bbox `x[0.5, 12.5] y[4.5, 19.5]`

| n | x | y | \|dx\|/hx | \|dy\|/hy | SIDES |
|---|---|---|---|---|---|
| 1 | 0.50 | 4.50 | 1.0000 | 1.0617 | — |
| 2 | 12.50 | 4.50 | 1.0000 | 1.0617 | — |
| 3 | 12.50 | 9.85 | 1.0000 | 0.3483 | **YES** |
| 4 | 12.50 | 16.00 | 1.0000 | 0.4717 | **YES** |
| 5 | 9.80 | 19.50 | 0.5500 | 0.9383 | — |
| 6 | 3.20 | 19.50 | 0.5500 | 0.9383 | — |
| 7 | 0.50 | 16.00 | 1.0000 | 0.4717 | **YES** |
| 8 | 0.50 | 9.85 | 1.0000 | 0.3483 | **YES** |

```
ALL   {1,2,3,4,5,6,7,8}      LEFT  {1,6,7,8}
FRONT {1,2,3,8}              RIGHT {2,3,4,5}
REAR  {4,5,6,7}              SIDES {3,4,7,8}
```

**`SIDES = {3,4,7,8}` ✅ reproduces D16, and speakers 1 and 2 miss by 6.2 %** (`1.0617` against
`1.0000`) — `CONTEXT-3.3`'s figure, confirmed. Speakers 3 and 8 are `ON_EDGE` on this venue (they lie
on the `x = 12.5` and `x = 0.5` hull edges), which is why they render dashed today and why the
`!= INTERIOR` half of the predicate must be `!= INTERIOR` and not `== VERTEX`.

**Q10 answered: no change to `getVenueGeometry`'s payload is required for the predicate itself,** and
D19 forbids the page deriving membership regardless. But membership **is** a pure function of the
venue, so the cleanest home for the six named sets is **inside the `getVenueGeometry` payload** — it
already refreshes exactly when the venue moves, on `venueGen`, which removes a whole staleness class
and saves a native function. User slots are *not* a venue function and need their own read.

### The two fixtures (Q7)

**Permutation fixture** — the same eight physical positions, indices rotated by `k` (speaker `n`
holds default position `((n + k − 1) mod 8) + 1`). `FRONT` must return the indices that *now* hold
`y < cy`; a fixed-index implementation returns `{1,2,3,8}` and **fails**. Without it the probe is
vacuous against the traced layout, where 1 and 2 happen to be front.

**Empty-set fixture for D20** — a *proscenium* rig: four corners plus two points on each of the
front and rear edges, no side fills.

```
(0,0) (4,0) (8,0) (12,0)  (0,16) (4,16) (8,16) (12,16)
centroid (6.0, 8.0)   hx = 6.0   hy = 8.0
corners:      |dx|/hx = 1.000, |dy|/hy = 1.000  ->  1 > 1 is FALSE
edge points:  |dx|/hx = 0.333, |dy|/hy = 1.000  ->  FALSE
SIDES = {}                                          FRONT = {1,2,3,4}
```

Every speaker is non-`INTERIOR` (corners `VERTEX`, edge points `ON_EDGE`), the venue is
non-degenerate, and it is a **physically plausible rig** rather than a contrived one — a hall with
front and rear arrays and no side fills. That is what makes it a fair test of D20's
*"a degenerate venue can legitimately empty a named scene"* rather than a synthetic edge case.

---

## 4. The meters (Q4)

**Site the read in `processBlock`, after `gainStage.process(...)`.** It is the last statement in the
function, so this is genuinely "what leaves the plugin", and it sits **after** the ping's post-write
overwrite — which is what makes UI-03/2's cross-check against verify-ping stepping 1 → 8 possible at
all. `GainStage` is not touched, so P24's "this class does not ask the processor anything" survives.

```cpp
// after gainStage.process(...), still inside processBlock
for (int i = 0; i < 8; ++i)
{
    const int ch = mapped ? snapshot.speakerToBuffer[i] : i;   // the snapshot's map, not the member
    if (ch >= numOut) { continue; }
    const float pk = buffer.getMagnitude (ch, 0, buffer.getNumSamples());
    if (pk > meterPeak[i].load (std::memory_order_relaxed))
        meterPeak[i].store (pk, std::memory_order_relaxed);
}
```

- **`snapshot.speakerToBuffer`, never the processor member** — the block was rendered against the
  snapshot, and a venue edit can land between the two.
- **Identity attribution when unmapped is correct and must not be "fixed."** Constraint 2: under
  `mapInvalid` the `else` arm writes `out[ch][n] = ch == 0 ? sL : sR`, so the meters show speaker 1
  lit from L and speakers 2–8 lit from R. **That is the fold being visible**, which is the entire
  point of metering the output.
- **PERF-01 is safe by construction and still gets measured.** `AudioBuffer::getMagnitude(channel,
  start, num)` resolves to `FloatVectorOperations::findMinAndMax` on a raw pointer — no allocation,
  no lock. UI-03/4 re-runs probe **AO** with metering live anyway, because the criterion is a
  measurement and not an argument.
- **The load/compare/store race is benign and should be documented rather than hardened.** With one
  audio-thread writer and a message-thread `exchange(0)` reader, the only reachable interleaving
  re-publishes a peak that was already reported — a duplicate on a max-hold display, never a lost
  peak. A CAS loop would buy nothing.
- **Add `static_assert (std::atomic<float>::is_always_lock_free)`** beside the array. This project
  does not leave an invariant in prose (`pattern_ring_invariant_needs_static_assert`).

**Payload and transport — two intervals, not one.** `getStatus` stays at 2 Hz and a new `getMeters`
runs at ~30 Hz, returning **linear peaks**:

```
getMeters -> { peak: [8 floats, linear], gen: <int> }
```

- **Separate, not folded into `getStatus`,** for a measurable reason: `getStatus` builds a
  `juce::String` from `getBus(false,0)->getCurrentLayout().getDescription()` on every call
  (`PluginEditor.cpp:382-385`). At 30 Hz that is 30 string constructions a second on the message
  thread for a value that changes only on renegotiation.
- **Linear, not dB.** The `−60..0 dBFS` mapping and the ballistics both live in JS already; sending
  the raw measurement keeps the transform in one place.
- **`exchange(0.0f)` per speaker, inside the native function**, per amendment 2 — a dropped frame
  then widens the measurement window instead of losing the peak.
- **The ballistics coefficients are per-`requestAnimationFrame` frame** (attack 0.5, decay 0.12); the
  ~30 Hz poll refreshes only the *target*. The 1.5 s hold and 20 dB/s release are wall-clock from
  timestamps. Amendment 2 states this; it is repeated here because applying a per-frame coefficient
  on the poll clock is `pattern_block_rate_envelope_breaks_blocksize_invariance` in UI form.
- **The poll's in-flight guard releases on a deadline** — N9.

---

## 5. The gradient (Q1 / Q2)

**Q1 — the solver can be called on the message thread, with two qualifications.** `dbap::solve` is a
free function with no instance and no state: raw arrays in, eight floats out, `noexcept`, no
allocation, no JUCE (`DbapSolver.cpp:22-25` forbids any further include). Nothing about it is bound
to the audio thread, and the shipping binary carries no counter objects at all
(`OOCTAGON_INSTRUMENT` is defined by the two **test** targets only). **No second instance is needed
and none exists to need.**

1. **Under instrumentation, a field sample pollutes `powCalls`.** Both test targets define
   `OOCTAGON_INSTRUMENT=1`, and probe AE asserts `powCalls == 16` **exactly** per control block. A
   field-sampler probe sharing a process with AE must call `instr::resetCounters()` between them —
   the render harness already does this at eleven sites, so it is a convention to follow, not a new
   one. `solveRuns` is **not** at risk: `countSolveRun()` is called in `GainStage::updateControl`
   (`GainStage.cpp:287`), not inside `solve`.
2. **The recompute counter must be C++-side**, because UI-04/2 requires a counter rather than an eye,
   and because D19's rule — assert against what C++ returned — applies here as much as to scene
   membership.

**Where the code goes.** A new `Source/DSP/FieldSampler.{h,cpp}` depending only on `DbapSolver.h`,
`Vec.h` and `VenueSnapshot.h` lands in the **fast unit target**, whose link line is
`juce_audio_basics + juce_core + juce_data_structures` and which already compiles `DbapSolver.cpp`.
UI-04/1's twenty-point comparison then runs in the seconds-to-build target rather than behind a
plugin — the same move P56 made for `VenueFile.cpp` at 3.2. **Gate 11 re-verifies the link line
rather than trusting this paragraph.**

**Q2 — measured, with the shipping solver, on this machine:**

| Grid | Points | `pow` calls | **Time per recompute** | Payload as f32 | as base64 u8 |
|---|---|---|---|---|---|
| **32 × 40** | 1 280 | 10 240 | **183 µs** | 5.0 kB | **1.7 kB** |
| 48 × 60 | 2 880 | 23 040 | 316 µs | 11.2 kB | 3.8 kB |
| 56 × 70 | 3 920 | 31 360 | 243 µs | 15.3 kB | 5.1 kB |
| 64 × 80 | 5 120 | 40 960 | 282 µs | 20.0 kB | 6.7 kB |
| 112 × 140 | 15 680 | 125 440 | 660 µs | 61.2 kB | 20.4 kB |

**`pow` is not the constraint; the payload is.** Even 112 × 140 costs well under a millisecond on the
message thread, but 61 kB of JSON per recompute is not something to send through a bridge that
serialises every value. **Recommendation: 32 × 40, one native call, quantised to 8 bits and sent as
base64** — 1.7 kB, decoded with `atob` into a `Uint8Array`, `putImageData` onto a 32 × 40 offscreen
canvas, then `drawImage`-scaled onto `#plan-backdrop`. That satisfies UI-04/3's *"offscreen canvas
and blitted"* directly rather than by argument, and the browser's own smoothing is what turns a
coarse grid into a gradient.

**8-bit quantisation does not weaken UI-04/1.** The 1e-3 comparison is a **C++ unit probe** against
`dbap::solve` on the sampler's float output; quantisation is a transport detail strictly downstream
of the assertion. Putting the assertion anywhere else would be asserting against a JS re-derivation,
which is what the criterion forbids.

**`#plan-backdrop` already exists** as a separate canvas layer (`index.html:119`), authored for this
at 3.1 — *"a raster surface, because 3.3's level-gradient field needs per-pixel work."* UI-04/4's
descope-costs-a-flag property is therefore structural today and needs nothing built to make it true.

---

## 6. Scenes: the write path (Q6)

**Do the eight writes in C++, in one native function.** D18 requires
`beginChangeGesture()` → `setValueNotifyingHost()` → `endChangeGesture()` on each of `w1..w8`, and
`PluginEditor.cpp:635-659` already does exactly this shape for `loadPreset` across all 17 parameters,
including the *closed on both paths* discipline. A JS-side write through eight `SliderState`s would
work, but it scatters the bracket obligation across 24 messages and puts the correctness of D18
somewhere no single grep can confirm. One `applyScene` call site keeps it in one place, next to its
precedent.

The parameter echo still reaches the page: `WebSliderParameterAttachment` listens to the parameter,
so a C++-side `setValueNotifyingHost` repaints the eight in-plan weight cells with no extra plumbing.

**D20's refusal belongs in C++ too**, not only in the disabled control. The UI disabling an empty
scene is the affordance; `applyScene` returning `{ok:false, reason:"emptyScene"}` is the guarantee —
the same defence-in-depth `startPing`'s `mapInvalid` refusal established at 3.2.

**Q6 — the operational definition of "two scenes can be faded between."** The criterion is a property
of *writing parameters rather than latching a mode*, so the gate is:

1. Apply scene A; read all 8 host-side values → vector `a`.
2. Apply scene B; read all 8 → vector `b`.
3. Write `w_i = 0.5·(a_i + b_i)` directly to the parameters, bypassing the scene path entirely.
4. Render a block and assert the eight per-speaker gains equal a **direct solve** for the blended
   weight vector — and that **no subsequent block re-asserts `a` or `b`**.

An implementation that stores "current scene" and re-applies it in `updateControl` fails step 4;
one that only writes parameters passes by construction. It runs entirely in the render harness,
which already has APVTS access, and it needs no host. **This is the shape the criterion's
*"the gate that catches an implementation that latches"* was asking for.**

---

## 7. The native-function surface: 13 → 18

| # | Function | New? | Notes |
|---|---|---|---|
| 1–13 | the 3.2 surface | | unchanged |
| 14 | `getMeters` | ✚ | 8 linear peaks + `gen`; `exchange(0)` C++-side; ~30 Hz |
| 15 | `getScenes` | ✚ | the **4 user slots** only — 8 weights + occupied flag each — plus `scenesGen` |
| 16 | `applyScene` | ✚ | 8 gesture brackets; refuses an empty set |
| 17 | `storeScene` | ✚ | D22's capture into a slot |
| 18 | `getFieldGrid` | ✚ | `{cols, rows, minDb, maxDb, data: <base64>, computeCount}` |

**Named-scene membership rides `getVenueGeometry`** rather than taking a nineteenth entry — it is a
pure function of the venue and that payload already refreshes on `venueGen`. **`scenesGen` joins
`getStatus`**, mirroring `venueGen`, so the page knows when to refetch slots.

**UI-05 needs no new function.** `getVenueGeometry` already carries per-speaker `z`, `rake.front`,
`rake.rear`, `bbox.minY/maxY` and the centroid — all landed by 3.2's P55. D15's affordability
argument is confirmed in source.

**Three places must move together or a gate fails loudly**, which is the intended behaviour:

- `ui_frontend_check.js:211`'s literal `registered.size === 13` → `18`. It will **fail until all
  eighteen exist in the C++ registrations, the derived call sites and the stub whitelist**, exactly
  as the `3 → 13` move did at 3.2.
- The stub's `NATIVE_FNS` map (`juce-stub.js`) — §3 diffs it against the C++ **as a set**.
- `getResource()` and `juce_add_binary_data` SOURCES for each new page module.

**The enumeration hole is already closed and needs no widening.** P51 derives `PAGE_MODULES` from
`Source/ui/public/js/*.js` (`ui_frontend_check.js:122`) and §1/§3/§6/§12/§14/§19/§21 all iterate it,
with §21 asserting set-equality against the CMake SOURCES. New 3.3 modules land automatically; a file
added on disk and forgotten in CMake fails §21 rather than 404-ing as a missing panel. **This is the
seventh time this vacuity class would have bitten, and it is the first time it costs nothing.**

**One gap the stub does not yet model:** UI-04/2's puck-drag assertion needs the Playwright side to
count `getFieldGrid` **invocations**. The stub records `WRITES` and `GESTURES` but has no call
counter. Adding one to `getNativeFunction` is a few lines and belongs in the task that adds
`getFieldGrid`.

---

## 8. Handoff to plan

**Answered:** all eleven questions. **Five from source or measurement that discuss could not have
had:** N9 (measured on the shipping page), N10 (measured against the shipping solver), N11 (measured,
with a method control), N12 and N13 (read in source).

**Decisions plan must take, in descending order of blast radius:**

| # | Decision | Research recommends |
|---|---|---|
| 1 | UI-04's field quantity | **`1/k`, the un-normalised DBAP field**, via a defaulted `float* outInvK` out-param on `dbap::solve`. **Amend `ROADMAP.md` and re-pin at the plan boundary** |
| 2 | N9's repair | Deadline-released in-flight guards everywhere, **including a fix to the shipped `refreshGeometry`**. This is 3.2 debt discharged at 3.3 |
| 3 | Scene layout | **V3** — one row of 10, `STORE` in the group title row. Strip **552 × 125** |
| 4 | Field sampling contract | Full chain (project → solve → hull trim), 32 × 40, base64 u8, one native call, coalesced to one recompute per poll tick |
| 5 | The D25 guard's justification | Fitted-box-vs-stage **plus an assertion that `#group-elevation` is the column's last child**; the NC asymmetry is against **§8**, not the column |
| 6 | Height-axis rule | Venue-derived and **quantised**; the criterion's own "front endpoint unchanged" half is the guard against a rescaling axis |

**Carried forward untouched:** Gate 13's Venue/modal half (~8 min human, now extended by the three
new screens' launch-and-look); D5 / QUAL-01's audible clause (Stage 4); the CI gap, widened a third
time here; `COMPAT-04`'s retroactive criteria (Stage 4).

**Not run, and not claimed:** Q5's WKWebView half. The JS latch is measured and the JUCE drop is
read from source; **a real 30 Hz poll against a hidden WKWebView has not been executed by anyone**,
and it should become a named item in 3.3's Gate 13 rather than a line in a summary that reads as
though it had.

**Verification target for 3.3 is unchanged:** `FUNC-06`, `UI-03`, `UI-04`, `UI-05` — four rows,
18 criteria, **zero partials declared in advance**. Closing them completes Stage 3.

---

## Next Phase

**Ready for:** `plan`
