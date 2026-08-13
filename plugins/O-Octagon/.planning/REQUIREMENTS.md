# O-Octagon - Requirements

---
version: 1.0.0
plugin: O-Octagon
created: 2026-08-10
lastUpdated: 2026-08-13
lastVerified: stage-4 phase 4.1 (machine gates) — 29 of 30 complete, 0 partial, 1 pending
openRows: COMPAT-02 (pending, stage-4 phase 4.2 — the ONLY open row; three criteria, all needing a host)
---

## Overview

**Target Milestone:** v1.0
**Total Requirements:** 30
**Coverage:** must: 18 | should: 10 | nice: 2

---

## Requirements

### Functional (FUNC)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| FUNC-01 | Accepts mono or stereo input and renders 8 discrete speaker feeds through an `AudioChannelSet::create7point1()` output bus used purely as an 8-channel carrier | must | complete | stage-2 *(2.2)* |
| FUNC-02 | User can type real measured coordinates in metres (x, y, z) for all 8 speakers plus front/rear rake heights, and save/load them as a venue store | must | complete | stage-3 *(3.2)* |
| FUNC-03 | User-configurable 8-row mapping table assigning speaker 1-8 to a 7.1 channel label, with a sane shipped default | must | complete | stage-2 *(2.1 + 2.2)* |
| FUNC-04 | Verify mode solo-pings each speaker in turn (manual step and auto-cycle) so physical wiring is confirmable in under a minute | must | complete | stage-3 *(3.2)* |
| FUNC-05 | Venue data and musical state are separate preset stores; loading a musical preset never writes venue geometry, trims, or the label map | must | complete | stage-3 *(3.2)* |
| FUNC-06 | Weight scenes (ALL / FRONT / REAR / LEFT / RIGHT / SIDES + 4 user slots) write all 8 weight parameters at once so they record as ordinary automation | should | complete | stage-3 *(3.3)* |
| FUNC-07 | Per-speaker calibration trim (-12 to +6 dB), venue-scoped, applied after the DBAP solve | should | complete | stage-2 *(2.3)* |

### DSP (DSP)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| DSP-01 | DBAP implemented per the 2011-04-14 revised equations, in three dimensions (`d_i` includes the `(z_i-z_s)²` term) | must | complete | stage-2 *(2.2)* |
| DSP-02 | Constant intensity: `Σ v_i² = 1` holds within tolerance for every source position, inside and outside the hull, at every rolloff and blur setting | must | complete | stage-2 *(2.2)* |
| DSP-03 | Convex hull computed by a proper algorithm with explicit collinearity handling — no rectangle assumption, no assumption that all 8 speakers are vertices; sources outside are projected to the nearest hull point for gain computation | must | complete | stage-2 *(2.1)* |
| DSP-04 | Audience plane modelled as sloped from front-row to rear-row ear height; `srcZ` is height above that plane, so `srcZ = 0` tracks ear level front-to-back | must | complete | stage-2 *(2.1 + 2.2)* |
| DSP-05 | Per-speaker weights `w_1..w_8` (0-1) restrict a source to a subset of speakers and redistribute rather than reduce level | must | complete | stage-2 *(2.2)* |
| DSP-06 | Stereo input with `width > 0` renders as two sub-points straddling the puck, spread perpendicular to the puck's bearing from room centre; `width = 0` collapses to a mono-summed single point | should | complete | stage-2 *(2.3)* |
| DSP-07 | Source→hull distance drives a gain trim (0-3 dB/m) and a 1-pole air-absorption LPF, each independently defeatable | should | complete | stage-2 *(2.3)* |
| DSP-08 | Spatial blur `r_s` is normalised against the covariance of speaker distances from rig centre (paper §3.1) so it is room-size independent, and is capped; it is additional to the physical floor supplied by speaker height | should | complete | stage-2 *(2.3)* |

### UI (UI)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| UI-01 | Venue measurement screen: 8-row x/y/z entry in metres, rake front/rear, 8-row label map, 8 trims, verify-ping controls, venue save/load | must | complete | stage-3 *(3.2)* |
| UI-02 | Room screen: top-down plan proportioned to the measured geometry, draggable source puck, convex hull drawn explicitly | should | complete | stage-3 *(3.1)* |
| UI-03 | Live per-speaker level indicators at each of the 8 speaker positions on the plan | should | complete | stage-3 *(3.3)* |
| UI-04 | DBAP level-field gradient rendered as the plan backdrop (paper figs 1-3) | nice | complete | stage-3 *(3.3)* |
| UI-05 | Height control presented as a side-elevation strip showing the raked audience line and the source's height above it | nice | complete | stage-3 *(3.3)* |

### Performance (PERF)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| PERF-01 | Real-time safe audio processing — no allocations, locks, or file I/O in `processBlock` | must | complete | stage-2 *(2.2)* |
| PERF-02 | Gain vectors recomputed only when position, weights, rolloff, blur, or venue change — not per sample — and the resulting 8 gains are smoothed | should | complete | stage-2 *(2.2)* |

### Compatibility (COMPAT)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| COMPAT-01 | Passes pluginval validation (VST3 and AU), including strictness level 10 | must | complete | stage-1 |
| COMPAT-02 | Instantiates in Logic Pro on a surround track and outputs 8 discrete channels | must | pending | stage-4 |
| COMPAT-03 | Speaker→buffer index map is built once in `prepareToPlay()` via `getChannelIndexForType()`; no hardcoded channel indices anywhere in the codebase | must | complete | stage-2 *(2.1)* |
| COMPAT-04 | Defined, non-crashing behaviour when instantiated on a stereo track (exact policy resolved at Stage 0) | should | ✅ **complete** — 3 of 3 | stage-1 *(criteria 1, 2)* → **stage-4 (4.1)** *(criterion 3 via CO + BM + NC1; criterion 2's render clause via CQ; criterion 1 re-run on the final binary)* |

### Quality (QUAL)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| QUAL-01 | No audio artifacts across the full parameter ranges | must | complete | stage-2 *(2.3)* |
| QUAL-02 | No NaN or Inf for any source position, any weight state including all-zero, any blur value, or any degenerate venue geometry | must | complete | stage-2 *(2.2)* |
| QUAL-03 | Block-size invariant — an offline bounce is bit-identical to a real-time pass for identical automation | must | complete | stage-2 *(2.2)* |
| QUAL-04 | No zipper noise on fast position, width, or weight automation | should | complete | stage-2 *(2.2 + 2.3)* |

---

## Acceptance Criteria Details

### FUNC-01: 8-channel transport

- [x] `isBusesLayoutSupported()` accepts mono-in/7.1-out and stereo-in/7.1-out — *met at stage-1;
      `auval` reports `[1, 8]` and `[2, 8]` in the derived AU config set*
- [x] Rejects layouts whose output channel count is not 8 (except the Stage-0 stereo-fallback policy)
      — *met at stage-1; the derived AU set is exactly six configs and admits nothing else*
- [x] All 8 output channels carry independent, non-duplicated signal for an off-centre source —
      *met at stage-2 phase 2.2; probe AI, **0 duplicate pairs of 28**, closest lanes differ by
      0.00398982, on a non-identity map. Note AI is invariant under a channel-map permutation — it
      evidences independence, **not** routing (VERIFICATION-2.2 NC3). Routing is FUNC-03/3*

> **Re-mapped stage-1 → stage-2 at Stage 1 verify (2026-08-11).** Two of three criteria were met by
> the foundation shell; the third cannot be met until Stage 2 exists. See
> `stages/1-foundation/VERIFICATION.md` §Issues 1.

### FUNC-02: Measured venue entry

- [x] All 24 coordinate fields and both rake heights accept typed metre values and reject non-numeric input
      → **`ui_layout_check.js` §13 — typing `abc` into `vf-3-y` MARKED the field and REVERTED it on blur to "9.85", with `setVenue` call count unmoved at 0; typing `7.25` committed EXACTLY ONE `setVenue` carrying 42 values, and `speakers[2].y == 7.25` in the payload. Testable only because **D12** chose `type="text"` — §23 asserts all 42 are `type="text" inputmode="decimal"` and that `type="number"` appears nowhere. **NC4**: making the label column revert like the numeric ones fails §15.**
- [x] Saving then reloading a venue reproduces all 42 values exactly
      → **Probe BN — `venuefile::save()` then `venuefile::load()` into a FRESH model, all 42 bit-compared through the object representation: identical, 776 bytes, `@schemaVersion 1`, and PROVED different from a default-constructed model so an empty file could not pass. These are the same two functions the chooser completions call; §29 asserts that statically and that no parallel serialisation path exists outside `VenueFile.cpp`.**
- [x] The DBAP solve uses the entered values, verified by a changed gain vector after a coordinate edit
      → **Probe BZ — one coordinate moved through **`applyVenueEditChecked()`**, the function `setVenue` calls: applied, and the solved gain vector CHANGED by 0.055373 at speaker 4. Distinct from probe AQ/BL, which drive `applyVenueEdit` directly and therefore do not exercise 3.2's guarded path at all.**

### FUNC-03: Label map

- [x] Each of the 8 rows can be assigned any of the 8 7.1 channel labels — *met at stage-2 phase 2.1;
      probe D drives a **non-identity** map (built `{1,2,3,4,5,6,7,0}`), so a hardcoded 0..7 fails
      immediately (C1)*
- [x] A duplicate or missing assignment is detected and surfaced rather than silently routed —
      *met at stage-2 phase 2.1; probe E (missing label → rejected, output completely untouched),
      probe F (duplicate → rejected, last valid map retained), probe G (a label of `"7"` resolves to
      type 134, a plausible discrete channel **not** `unknown`, and is rejected by the permutation
      check)*
- [x] Changing a row moves audio to the corresponding physical output — *met at stage-2 phase 2.2
      by probe **AJ** (Layer 3): 8 renders on a non-identity map, each speaker's tone lands ONLY in
      its own lane and all 8 dominant bins are exact. Verify-ping itself remains FUNC-04 at stage-3.
      **Proven non-vacuous at verify (NC3):** bypassing `speakerToBuffer[i]` for a literal `i` makes
      AJ fail with every lane misrouted and every dominant bin off by one position*

> **Closed complete at stage-2 phase 2.2 (2026-08-11).** Partial at 2.1 by plan, not by shortfall.
> See `stages/2-dsp/VERIFICATION-2.1.md` and `stages/2-dsp/VERIFICATION-2.2.md`.

### FUNC-04: Verify-ping

- [x] Ping plays from exactly one speaker at a time, with all others silent
      → **Probe BQ — all 8 targets on a **NON-IDENTITY** map: one lane sounding each time, the other seven **EXACT zero** compared through the object representation, never a threshold (wrongLane 0, leaked 0, silent 0). 0 allocations measured on the audio thread while pinging. Also carries **ROADMAP orphan 5**: swapping two label rows moved the sounding lane 1 → 2, which is the ping-confirmed half of FUNC-03's "changing a row moves audio" (AJ closed the rendered-tone half at 2.2).**
- [x] Manual step advances 1→8 and auto-cycle completes all 8 unattended
      → **Probe BS — auto-cycle order measured as `12345678`, on-segment 57536 smp against an expected 57600 (= `kOnSeconds × sampleRate`), gap 19200 exact, **total 614400 samples == 12.8 s**, all derived from the prepared rate rather than transcribed. Manual `startVerifyPing(1..8)` reported each speaker in turn. Carries **ROADMAP orphan 3**.**
- [x] Ping level is bounded by a fixed conservative ceiling regardless of `outputGain`
      → **Probe BR — measured at `outputGain = +12 dB` **AND** `trim = +6 dB` SIMULTANEOUSLY (either alone leaves the other multiply untested). The bound itself holds BY CONSTRUCTION: `VerifyPing.cpp:258` clamps every sample with `juce::jlimit (-ceiling, ceiling, s)` at `kPeakCeilDb`, a HARD ceiling rather than a limiter, and the ping is a post-write OVERWRITE so neither `outputGain` nor the trims are in its path at all. The probe asserts the measured RESULT against a band, not `kPinkNormScalar`. **The figures are one run's sample, not a reproducible constant** — `VerifyPing`'s member-owned `juce::Random` is default-constructed, and JUCE's default ctor calls `setSeedRandomly()`, so the pink stream differs every run. Five runs at verify (2026-08-12): RMS **−20.14 … −19.97 dBFS** against a −20.0 target, peak **−9.21 … −8.47 dBFS** against the −6.0 ceiling, crest 10.76 … 11.67 dB. PASS on every run. Carries **ROADMAP orphan 2**, which the criterion's own "`outputGain`" wording does not reach — the trims are a separate multiply, live since 2.3.**

### FUNC-05: Preset separation

- [x] Loading a musical preset leaves all 42 venue values bit-identical
      → **Probe BW — a preset save/load across a measured venue left all 42 values **BIT-IDENTICAL**, with a positive control proving the load actually did something (blur restored to 0.620 from 0.05). Plus §27, the one greppable assertion: **`setCustomStateCallbacks` appears in NONE of the 24 O-Octagon source files** (comments stripped) and `preset-manager.js` is not vendored. `applyPresetJson` iterates `processor.getParameters()` only and can never walk `apvts.state`'s children, so the criterion holds by construction and the probe measures it anyway.**
- [x] A musical preset saved under venue A recalls correctly under venue B, with position resolved against venue B's bounding box
      → **Probe BX — saved under the default venue at `srcX/srcY = 0.3/0.7`, applied a venue with a different bbox, reloaded: normalised values **UNCHANGED** at 0.3000/0.7000 while the resolved metres followed venue B, (4.10, 15.00) → (4.01, 12.66). The musical gesture is "30 % across the rig", not "at 4.10 m".**
- [x] Session state (`getStateInformation` / `setStateInformation`) round-trips both stores together
      → **Probe BY — 1338 bytes across a modified venue AND modified parameters: 42 venue values identical, 17 parameters identical, and the restored venue proved to be the measured one rather than the default. **N2 honoured**: O-Octagon keeps its own state functions, because `OuariconPresetManager::setStateFromXml` calls `replaceState()` and nothing else, which would bypass §4.1's `readVenueFromState()` → `rebuildChannelMap()` ordering.**

### FUNC-06: Weight scenes

> **Section added 2026-08-12 at the Stage 3 discuss boundary.** FUNC-06 carried a summary-table row
> and no acceptance criteria at all — it would have been "verified" at 3.3 against nothing. Criteria
> derived from `ROADMAP.md` Phase 3.3, `ARCHITECTURE.md` §6.3 / §4.1, and decisions **D6** (scene
> membership is geometry-derived) and **D7** (user slots persist in a `SCENES` child) recorded in
> `stages/3-gui/CONTEXT-3.1.md`. Neither contract defined either of those; the gap was found here.

- [x] Each of the six named scenes writes **all 8** weight parameters in one gesture via
      `setValueNotifyingHost`, so the host records eight ordinary automation events — not a latched
      mode, not a plugin-private state. Verified by reading back all 8 host-side parameter values —
      *met at stage-3 phase 3.3; probe **CI**, driven through real `AudioProcessorParameter::Listener`
      instances: **8/8 host values match**, **8/8 begin gestures**, **0 unclosed**, 8/8
      valueChanged notifications. The write is `OOctagonProcessor::applySceneWeights`, one C++
      function with one call site — the **third and final** site of `gesture_bracket_obligation`
      after the 3.1 puck and the 3.2 preset load*
- [x] **Named scene membership is derived from the measured geometry, not from fixed speaker
      indices** (D6): `FRONT`/`REAR` split on the centroid's y, `LEFT`/`RIGHT` on x, `SIDES` selects
      hull speakers off both axes, `ALL` sets 8×1.0. The probe **re-measures a venue with permuted
      speaker numbering** and asserts `FRONT` still selects the physically-front speakers — *a
      fixed-index implementation must FAIL this criterion.* Without the permutation the probe is
      vacuous against the traced layout, where 1,2 happen to be front (the R1 failure class:
      `critical_audiochannelset_is_a_bitset_not_an_order`) — *met at stage-3 phase 3.3; probe **CG**
      rotates the eight indices against the same eight physical positions for **k = 1..7** and all
      seven track the geometry, and the probe REPORTS ITS OWN GUARD STRENGTH: **a fixed-index
      implementation would pass 0/7**. **NC3** injected exactly that defect and CG fired on all
      seven rotations while **CF and CH still passed** — which is what makes CG the guard rather
      than additional coverage. Probe **CF** pins the six sets on the default venue to D16's table
      exactly: ALL {1..8}, FRONT {1,2,3,8}, REAR {4,5,6,7}, LEFT {1,6,7,8}, RIGHT {2,3,4,5},
      **SIDES {3,4,7,8}** — speakers 1 and 2 miss by 6.2 %, a property of this hall. The predicate
      lives once, in `oo::scenes::resolve`, and frontend **§32** asserts the page carries no copy*
- [x] The Room plan indicates **which speakers a scene will select before it is committed**, and the
      degenerate cases are legible rather than silent: a scene resolving to an empty set must be
      shown as empty and must **not** be writable (all-zero weights are DSP-05's silence path, and
      reaching it by a mis-derived scene click mid-concert is unrecoverable) — *met at stage-3 phase
      3.3; layout **§24** proves the preview answers **hover AND keyboard focus** with the set the
      plugin returned — hover FRONT lit **[1,2,3,8]**, focus SIDES lit **[3,4,7,8]**, two different
      sets so neither check passes on a constant. Layout **§25**: an uncaptured slot is `disabled`
      with `data-empty="true"`, clicking it moved **not one of the eight weights** and invoked
      `applyScene` **0 times** — "the page did not ask" is a stronger claim than "the plugin said
      no", and both exist. Probe **CH** supplies the geometry half on a physically plausible
      PROSCENIUM rig — 4 corners plus 2 fills on each of the front and rear edges — where
      **SIDES = {} with 0 INTERIOR speakers**, so the set is empty for the right reason*
- [x] The 4 user slots each capture and recall all 8 weights, persisted in a **`SCENES` child of
      `apvts.state`** (D7), and round-trip through `getStateInformation`/`setStateInformation` —
      *met at stage-3 phase 3.3; probe **CK**: **16/16 weights bit-identical** across a full
      state round trip with occupancy {1,3} preserved. The upgrade half is driven EXPLICITLY rather
      than argued — the `SCENES` element is stripped from a real serialised state and fed back in,
      and the restore yields **0 ghost slots** with the node **written back** and `VENUE` intact.
      Without that write-back every pre-3.3 session would restore with the four slots reading
      ABSENT rather than EMPTY (RESEARCH-3.3 N13)*
- [x] **FUNC-05's structural guarantee survives the addition**: `SCENES` is a *sibling* of `VENUE`,
      never a child. Loading a musical preset may carry scenes; it must still leave all 42 venue
      values **bit-identical** — asserted by the same bit-compare FUNC-05 uses, re-run after the
      `SCENES` node exists, so the guarantee is measured against the new tree shape and not assumed
      to have been inherited — *met at stage-3 phase 3.3; probe **CL** re-runs the 42-value compare
      **with the `setCustomStateCallbacks` registration LIVE** — the same one the editor makes — and
      the venue is bit-identical to the live one and **demonstrably not** the saved one, while
      `blur` restored to 0.44 and the scene callback provably ran. Frontend **§27 changed shape**
      as planned, from "the symbol appears nowhere" to "**exactly one** registration, and neither
      `scenesToVar` nor `scenesFromVar` names the venue". Frontend **§35** closes the remaining
      door: `setStateFromXml` — which does `replaceState` on the whole tree — has **0 call sites**
      in O-Octagon*
- [x] Two scenes can be **faded** between — a direct consequence of writing parameters rather than
      applying a snapshot, and the gate that catches an implementation that latches — *met at
      stage-3 phase 3.3; probe **CJ**, Q6's four steps: apply A, apply B, then write
      `w_i = 0.5(a_i+b_i)` DIRECTLY, bypassing the scene path. The render is **identical to a
      reference processor that only ever saw the blend**, and after **8 further control blocks**
      the parameters **still hold the blend** — an implementation storing "the current scene" and
      re-applying it would have snapped back to A or B here. The endpoints are separately shown
      distinguishable from the blend, so the no-drift clause is not vacuous*

### FUNC-07: Per-speaker calibration trim

> **Section added 2026-08-11 at the Phase 2.3 discuss boundary.** FUNC-07 had a summary-table row and
> no criteria — the same defect repaired for PERF-02 and QUAL-04 at the 2.2 boundary, and for
> COMPAT-04 (retroactively, at Stage 4) at 2.2 verify. Criteria derived from `ROADMAP.md` Phase 2.3,
> `ARCHITECTURE.md` §5 step 7 and §6.2, and the `PHASE-2.3-TRIM` marker at `GainStage.cpp:182`.

- [x] A trim of −12 dB at speaker `i` produces exactly −12 dB at that speaker's output lane and **no
      level change at any of the other seven**, measured through `speakerToBuffer[i]` — *met at
      stage-2 phase 2.3; probe BF, lane 4 **×0.25119** against a wanted ×0.25119, **7/7 others
      bitwise unchanged***
- [x] The same holds at +6 dB, and both range ends are reachable from the venue store — *met at
      stage-2 phase 2.3; probe BF, **×1.99526** against a wanted ×1.99526, 7/7 others bitwise
      unchanged*
- [x] Trims are applied **after** the DBAP solve and folded into the smoothed targets, so they cost
      nothing per sample and are subject to the same 5 ms ramp as every other gain change — *met at
      stage-2 phase 2.3; `GainStage.cpp:279-280` multiplies into `setTargetValue`, and probe BF
      measures the trim **step** at 0.0008819 against the 0.0041677 smoother bound*
- [x] Trims are venue-scoped, not automatable: loading a musical preset leaves all 8 trims
      bit-identical (the FUNC-05 store separation, exercised on this field specifically) — *met at
      stage-2 phase 2.3; probe BF, venue-scoped ratio **0.251189 → 0.251189** across a musical preset
      load*

> **Proven non-vacuous at verify (NC3).** Dropping `* snapshot.trimLin[i]` makes BF fail on all four
> clauses at once — `x1.00000 (want x0.25119) WRONG`, and
> `venue-scoped: ratio 1.000000 -> 1.000000 — THE TRIM MOVED WITH THE MUSICAL STATE`.
>
> **The multiply arms a permanent-silence latch, and P29 is what closes it** (RESEARCH-2.3 H5).
> `v_i` is exactly `0.0f` whenever `w_i == 0` (DSP-05/1), so an unsanitised `trimDb = 1e30` gives
> `0.0f * inf = NaN` → `setTargetValue(NaN)` → the `SmoothedValue` latches. Sanitised at
> `publishSnapshot()`, the single funnel. **Verified at 2.3 verify by NC2:** removing the guard makes
> probe BH report `NON-FINITE` and `clamp: xinf — NOT CLAMPED`. Two of the three pathological inputs
> are benign unguarded (`NaN > -100.0f` is false so `decibelsToGain` returns 0; `-1e30` likewise) —
> **only the `+inf` path kills the plugin**, so the guard is the only thing standing on it.

### DSP-01: DBAP, 3D, revised equations

- [x] `a = R / (20·log10 2)` — *met at stage-2 phase 2.2; probe X, R=3 → 0.4982892 (err 1.33e-8),
      R=4 → 0.6643856 (err 2.2e-9), R=6 → 0.9965785 (err 2.65e-8)*
- [x] `d_i` includes the `(z_i - z_s)²` term; changing only `srcZ` changes the gain vector — *met at
      stage-2 phase 2.2; probe Z, 8/8 lanes moved, max delta 0.017464 over a 0.900 m rig z-spread.
      **NC1 at verify:** forcing `dz = 0` drops this to 0/8 lanes moved*
- [x] Gain vector matches an independent reference implementation of eqs 9-10 to within 1e-6 — *met
      at stage-2 phase 2.2; probe Y, **max |impl − oracle| = 1.0236e-7** over the 102 committed
      fixture cases (worst `random-22`), so the 1e-6 criterion is met with the hard gate set at 1e-5.
      The oracle is genuinely independent: `exp(−a·log d)` not `pow`, explicit `Σv²` not the C++'s
      `k = 1/√denom`, double precision, constants restated rather than parsed from the C++.
      **Proven load-bearing at verify (NC1, NC2):** a dropped z term moves Y to 0.39153065, and a 1e-4
      hand-edit of the committed fixture both fails `--check` and makes Y report exactly 1.0000e-4*

### DSP-02: Constant intensity

- [x] `Σ v_i² = 1 ± 1e-6` across a dense sweep of source positions inside the hull — *met at
      stage-2 phase 2.2; probe AA, 61 inside positions per parameter pair*
- [x] Holds for sources outside the hull, at hull vertices, and at speaker positions — *met at
      stage-2 phase 2.2; probe AA, 122 outside positions per pair, plus AD (on-speaker at blur = 0)*
- [x] Holds across the full rolloff (3-6) and blur (0-1) ranges — *met at stage-2 phase 2.2; probe
      AA sweeps the full product: **7686 solves, max |Σv²−1| = 3.2590e-7***

> **What DSP-02 does NOT cover, established by negative control at 2.2 verify (NC1).** `Σ v_i² = 1`
> is a *normalisation* invariant — `k = 1/√denom` is computed from the same distances it normalises,
> so the identity holds whatever those distances are. With the `(z_i − z_s)²` term deleted from the
> solver, AA still passes at 3.5154e-7 while DSP-01's probes Y and Z both fail. The 7686-solve figure
> is not broad correctness coverage, and DSP-02 cannot backstop DSP-01.

### DSP-03: Convex hull

- [x] Hull of the traced layout yields vertices 1, 2, 4, 5, 6, 7, correctly classifying collinear speakers 3 and 8 as on-edge rather than interior or vertex — *met at stage-2 phase 2.1 by probes **H and I together**. H alone is not the coverage: the §OQ4 side walls are dead straight, so speakers 3 and 8 have exactly zero cross product and pop for any non-negative epsilon — H would pass with `EPS_CROSS = 0`. Probe I exercises the epsilon (1 µm popped, 1 mm kept, against a measured `EPS_CROSS` of 1.0e-4)*
- [x] A source at a physical rear corner of the room is correctly classified as outside the hull — *met at stage-2 phase 2.1; probe J, `d_hull = 4.0608 m`*
- [x] Hull projection returns the nearest point on the boundary, verified against brute-force nearest-point-on-segment for a fixture set — *met at stage-2 phase 2.1; probe K, `max |impl − oracle| = 8.67e-7 m` over 200 points, 147 outside. The oracle is genuinely independent — ternary search on a convex 1-D function in double precision, not the closed-form dot-product projection the implementation uses*
- [x] Degenerate venues (all speakers collinear, duplicate coordinates) do not crash or produce NaN — *met at stage-2 phase 2.1; probe L, the full §3.1.6 matrix*

### DSP-04: Sloped audience plane

- [x] With `srcZ = 0`, the resolved absolute source height varies linearly from `rakeFront` at the front of the room to `rakeRear` at the rear — *met at stage-2 phase 2.1; probe M (`front 1.100, mid 3.550, rear 6.000`)*
- [x] A source at `srcZ = 0` at the rear of a steeply raked room is never below the rear-row ear height — *met at stage-2 phase 2.1; probe N, worst deficit 0.000000*
- [x] Changing `rakeRear` alone changes the gain vector for a rear-positioned source — *met at
      stage-2 phase 2.2; probe AK, **both halves**: rear source delta 0.00522166 (moved), front
      source delta 0.0000000000 (unchanged, as a plane hinged at the front row requires). The
      negative half is the coverage — `earHeight(bbMinY) = rakeFront` for any `rakeRear`, so a
      rear-only probe would not distinguish a correct plane from a global offset (RESEARCH H5)*

> **Closed complete at stage-2 phase 2.2 (2026-08-11).** Partial at 2.1 by plan, not by shortfall.
> See `stages/2-dsp/VERIFICATION-2.1.md` and `stages/2-dsp/VERIFICATION-2.2.md`.

### DSP-05: Speaker weights

- [x] Setting `w_i = 0` produces exactly zero gain at speaker `i` — *met at stage-2 phase 2.2;
      probe AB, each muted lane is exactly `0.0f`, not merely small*
- [x] Restricting to a 2-speaker subset preserves `Σ v_i² = 1` — *met at stage-2 phase 2.2; probe AB,
      7-live max |Σv²−1| = 1.391e-7, 2-speaker subset Σv² = 0.9999999754*
- [x] All-zero weights produce silence, not NaN or full-scale output — *met at stage-2 phase 2.2;
      probe AC, 4 configurations, all 8 gains exactly `0.0f`. The §3.3.4 `denom < kDenomEpsilon`
      guard writes silence explicitly; without it `k = inf` and `v_i = inf · 0 = NaN`, which would
      latch permanently in the SmoothedValue targets*

### DSP-06: Stereo sub-point geometry

> **Section added 2026-08-11 at the Phase 2.3 discuss boundary** (see the FUNC-07 note above).
> Criteria derived from `ROADMAP.md` Phase 2.3 and `ARCHITECTURE.md` §3.4.1–§3.4.3.

- [x] `width = 0` produces gain vectors **bit-identical** to a single mono-summed source point —
      i.e. `v_L ≡ v_R` by `memcmp`, reached with **no branch** (§3.4.3). This is the 2.2 shipping
      configuration, so it must not regress when `width` goes live — *met at stage-2 phase 2.3; probe
      AY, **8/8 `gL == gR` bitwise** at `width = 0`, and the same probe's non-vacuity half separates
      **8/8 pairs, worst 0.1636034**, at `width = 4`*
- [x] `width > 0` produces two sub-points spread perpendicular to the puck's bearing **from the rig
      centroid** (not the bbox centre), with **R on the audience's right** when the puck is downstage
      — the §3.4.1 handedness check, asserted rather than reasoned — *met at stage-2 phase 2.3; probe
      AX, **span 4.000 m ⟂ bearing, R on audience right***
- [x] Each sub-point resolves its **own** absolute height against the sloped audience plane at its
      **own** y — `z_L = earHeight(P_L.y) + srcZ`, and `z_L ≠ z_R` whenever the spread has a y
      component and `rakeFront ≠ rakeRear` — *met at stage-2 phase 2.3; probe AX, **dy 3.55 m → dz
      1.6349 m**, and equal z on a flat rake (the negative half)*
- [x] Sweeping the puck through the centroid at `width = 6` produces **no discontinuity**: the
      `rFade = 0.15·rigScale` collapse holds `wEff → 0` before the bearing flips, so both sub-points
      are coincident at the moment L and R swap (§3.4.2, design-time fix for risk R5) — *met at
      stage-2 phase 2.3; probe BA, worst per-sample step **0.0003724** against the 0.0041677 bound,
      **negative control 0.02383 fires**. AX measures the collapse geometrically: `rFade` **1.190 m**,
      monotone, full width at and beyond*
- [x] The `(0, −1)` fallback bearing is taken when `|b| < 1e-6` and yields a finite, symmetric result
      — *met at stage-2 phase 2.3; probe AX, finite and coincident on the puck*

> **Proven non-vacuous at verify (NC6), and the attribution corrected.** Reverting `widthMetres` to
> the 2.2 literal `0.0f` makes AY fail with
> `width=4: 0/8 pairs separated, worst 0.0000000 — WIDTH IS NOT REACHING THE SHAPER`. **AY is the
> liveness gate for `width` across DSP-06 and QUAL-04/3 alike** — see QUAL-04 criterion 3, where a
> sweep probe passes vacuously without it.

### DSP-07: Outside-hull distance processing

> **Section added 2026-08-11 at the Phase 2.3 discuss boundary** (see the FUNC-07 note above).
> Criteria derived from `ROADMAP.md` Phase 2.3 and `ARCHITECTURE.md` §3.5.1–§3.5.2 **as amended at
> this boundary** (decision D2 — the air skip condition is `airAmount · d_hull == 0`).

- [x] Gain trim is linear in dB per metre of `d_hull`, **floored at −24 dB**, applied per sub-point
      and folded into that sub-point's gain vector before smoothing — *met at stage-2 phase 2.3;
      probe AV, dB/m law linear over 6 × 20, floor exactly **−24.0000 dB**. Folded into `v` before
      §5 step 7 (`GainStage.cpp:211-218`), which is what makes "folded into that sub-point's gain
      vector" literally rather than approximately true*
- [x] `hullAtten = 0` → output **bit-identical** to hull trim removed (`dbToGain(-0.0) == 1.0f`
      exactly, so the multiply is a provable no-op) — DSP-07's "independently defeatable", first half
      — *met at stage-2 phase 2.3 **by the structural method below**; probe AV, `bitExact
      (hullTrimGain (0, d), 1.0f)` over **201 swept `d`**; non-vacuity by probe BD, `hullAtten` 0 vs
      0.5 outside the hull **changes** the render*
- [x] The trim is a no-op **inside** the hull regardless of setting, because `d_hull = 0` there —
      *met at stage-2 phase 2.3; probe AV, bit-exact `1.0f` over **13 `hullAtten` values** at
      `d = 0`*
- [x] Air LPF cutoff matches `clamp(20000·2^(−airAmount·d_hull/3), 500, 20000)` at `airAmount` ∈
      {0.35, 1.0} × `d_hull` ∈ {5, 15} m — the §3.5.2 table, to a stated tolerance — *met at stage-2
      phase 2.3; probe AU, the four-row table **exact to 1 Hz** at 48 kHz. AU also asserts H4's
      rate-aware ceiling — **9922.5 / 14400 / 19845 / 20000 Hz** — and `fc < fs/2` over 5 rates × 21
      `airAmount` × 51 `d_hull`. **Proven non-vacuous at verify (NC5):** restoring the literal 20000
      makes AU report `ceilings WRONG; PAST NYQUIST`*
- [x] `airAmount = 0` → **bit-identical** to the filter absent — DSP-07's "independently defeatable",
      second half. A 20 kHz cutoff would fail this — *met at stage-2 phase 2.3 **by the structural
      method below**; probe BD, `airSamplesFiltered == 0` — the branch is **counted** as never taken;
      non-vacuity in the same probe, **16384/16384** filtered at `airAmount = 0.35` outside the hull
      with a differing render*
- [x] **`d_hull = 0` (source inside the hull) → bit-identical to the filter absent, at any
      `airAmount`** — the D2 amendment. The shipping default patch is therefore bit-transparent —
      *met at stage-2 phase 2.3; probe BD, renders at `airAmount ∈ {0, 0.35, 1.0}` inside the hull
      are **bit-identical by `memcmp`**, counter 0 in all three. **Proven non-vacuous at verify
      (NC4):** removing the `d_hull > 0` conjunct makes BD report `FILTER RAN INSIDE THE HULL` —
      **and makes 2.2's probe Q′ fail at 0.319313765**, so the amendment is load-bearing for FUNC-01
      as well*
- [x] Filter state is `reset()` on the `airAmount → 0` transition but **not** on every `d_hull == 0`
      block, so a puck oscillating across the hull edge does not repeatedly re-zero it — *met at
      stage-2 phase 2.3 **structurally**: `GainStage.cpp:261-269` guards the reset on an `airAmount`
      edge, never a `d_hull` test. Under P27's entry re-seed the two policies §3.5.2 debated are
      **observationally equivalent at the boundary** — a spurious reset taken while the filter is
      inactive is overwritten before a single sample passes through it — so no measurement can
      distinguish them and claiming one does would be false. Probe BD measures the property the
      criterion exists to protect: **12 crossings, worst DC step 0.0009319 against a 0.0041677
      bound**, no click, state intact*
- [x] Injecting a non-finite sample recovers within one block — the per-block `std::isfinite` state
      check with `reset()` on failure (risk R6, `pattern_envelope_follower_state_sticky_nan`) —
      *met at stage-2 phase 2.3; probe BE half 1, active-filter NaN recovers in one block. **Proven
      non-vacuous at verify (NC7):** removing the P31 guard makes BE report
      `active-filter NaN LATCHED`*

> **BE's second half — what a green result does and does not attribute** *(added 2026-08-11 at 2.3
> verify; NC1, NC7, NC8)*. Half 2 poisons the filter, moves the source **inside** so the filter is
> skipped and the per-block output guard has nothing to inspect, then re-enters. It reported
> `re-entry clean` with the seed removed (NC1) **and** with the guard removed (NC7); it only failed —
> `RE-ENTERED POISONED` — with **both** removed (NC8). The construction is sound and the hole is real,
> but because half 2 poisons the filter while it is *active*, the P31 guard cleans the state at the
> end of that very block in the shipping build. **Two independent mechanisms close the same hole and
> a green half 2 cannot attribute the closure to either.** DSP-07/8's coverage is **BE half 1 + NC7**;
> P27's third payoff is real and is evidenced by **NC8**, not by a green run.

> **Method for criteria 2 and 5 — structural, not comparative** *(added 2026-08-11 at the Phase 2.3
> plan boundary; PLAN-2.3 P33, RESEARCH-2.3 Q5)*. There is **no "filter absent" build and no "trim
> absent" build** to compare against, and fabricating one would mean a second arithmetic path selected
> by a compile flag — the class of thing §3.4.3 forbids, and a path the shipping binary would never
> take. Bit-transparency is therefore proven **structurally**: the trim by `bitExact(hullTrimGain(0, d),
> 1.0f)` over a swept `d` plus `v * 1.0f == v` for every finite `v`; the filter by an instrumentation
> counter showing the branch is **never taken** (`airSamplesFiltered == 0`). **Each half carries a
> non-vacuity control** showing the stage *does* change the output when enabled. `SUMMARY-2.3.md`
> states this as a method — not as "bit-transparency ✓".
>
> **One stated precondition.** `processBlock` runs under `juce::ScopedNoDenormals`, so a **denormal**
> `v_i` (< 1.18e-38) would flush to zero under `v_i * 1.0f`. Reaching one requires a weight around
> 1e-38 — unreachable from the exposed 0–1 range with any non-degenerate geometry — and the probes use
> default weights. An unstated precondition is how a bit-identity claim quietly becomes false.

### DSP-08: Room-size-independent spatial blur

> **Section added 2026-08-11 at the Phase 2.3 discuss boundary** (see the FUNC-07 note above).
> Criteria derived from `ROADMAP.md` Phase 2.3 and `ARCHITECTURE.md` §3.3.2 / §OQ4.
> **Implemented at Phase 2.2** — probe AG is supporting evidence; the requirement ticks here.

- [x] `r_s = min(blur · 0.5 · rigScale, 8.0)`, where `rigScale = sqrt((1/8) Σ ‖p_i − c‖²)` is the RMS
      3-D speaker radius from the centroid — the paper's §3.1 "covariance of speaker distances from
      rig centre" made dimensional — *met at stage-2 phase 2.3; probe AG, `blur = 0.50 → 1.9829 m`
      against §3.3.2's 1.98 (the value the 2.2 D2 re-pin corrected from 1.99)*
- [x] **Room-size independence asserted as an invariant, not a constant**: uniformly scaling every
      speaker coordinate about the centroid by λ scales `r_s` by λ at fixed `blur`, so the gain vector
      is unchanged. A bare `rigScale ≈ 7.93` assertion is a mirrored fixture
      (`pattern_test_fixture_mirrors_drift_silently`) — this is the probe-O discipline from 2.1 —
      *met at stage-2 phase 2.3; probe AW, **Δ 3.0e-8 at λ = 0.5 and 6.0e-8 at λ = 2.0**, scaling the
      **source** as well as the speakers, at `blur = 0.25`. **The probe carries a positive control
      proving it can see the clamp:** at `blur = 1, λ = 2.1` it reads **Δ 1.844e-3**. `blur = 0.25`
      rather than 1.0 is deliberate (RESEARCH-2.3 H8) — at `blur = 1, λ = 2` the default rig lands
      7.932 m against the 8 m clamp, passing by **0.9 %**, and `rigScale` has already moved twice*
- [x] Both caps hold: the exposed `0–1` range **is** the primary cap (`blur = 1` → `0.5·rigScale`),
      and `kMaxBlurMetres = 8.0` is an absolute backstop so a mistyped coordinate cannot swamp the
      array. Both `static_assert`ed — *met at stage-2 phase 2.3; probe AG, 8 m cap enforced*
- [x] Blur is **additional** to the physical floor supplied by flown speaker height — at `blur = 0`
      the 3-D model still yields a finite, non-degenerate gain vector for a source at a speaker's
      floor coordinate — *met at stage-2 phase 2.3; probe AW, `blur = 0` at a speaker's floor point
      finite with Σv² = 1 (corroborated by probe AD at 2.2)*

### UI-01: Venue measurement screen

- [x] All 42 venue values are viewable and editable without leaving the screen
      → **`ui_layout_check.js` §12 — all **42** fields present, editable, POPULATED from the payload and **fully inside the viewport at exactly 1100 × 720**, with the id list DERIVED (8 × label/x/y/z/trim + 2 rake) rather than typed. §11 proves the rail itself does not overflow; §22 closes the 42 ids four ways — DOM == `venue.js`'s derived table == the payload shape == the four `VenueModel` setters. Measured table 752 × 277 px, rows at 32.5 px, matching Q11 exactly.**
- [x] Verify-ping is reachable from this screen and its active speaker is visually indicated
      → **`ui_layout_check.js` §18 — Start lit exactly one row and it was the **RETURNED** `getPingState().speaker`; a stub-driven `stepPing()` moved the indicator to the new returned speaker (3 → 4) and the "it actually moved" control confirms a frozen indicator would not have passed; Stop cleared it; a refusal on an invalid map rendered its reason. §26 asserts statically that **`venue.js` does no arithmetic on a speaker index at all** — C++ owns the cycle (**D14**), and the 100 ms poll is faster than the 400 ms gap the 500 ms status poll cannot resolve.**
- [x] Venue save/load round-trips through a named file
      → **THREE PARTS (P57), because a native modal is drivable by no gate class in this repo. **(a)** Probe **BN**, the 42-value bit-compare through `venuefile::save`/`::load`. **(b)** §29 — the completions hoist a `SafePointer` LOCAL (never an init-capture, MSVC), `return` **BARE** on a dead pointer, call exactly those two functions, and **no `createXml`/`parseXML` exists outside `VenueFile.cpp`** — the processor's single site is the session path N2 requires stay untouched. (b) is what makes (a) non-vacuous. **(c)** Gate 13, a Standalone launch-and-look for the modal itself. Probe **BO** additionally proves a forward `@schemaVersion` is SURFACED and a malformed or 3-speaker root REJECTED with `out` untouched.**

### UI-02: Room screen plan

> **Section added 2026-08-12 at the Stage 3 discuss boundary.** UI-02 carried a summary-table row and
> no acceptance criteria. Criteria derived from `ROADMAP.md` Phase 3.1, `ARCHITECTURE.md` §3.1.6, and
> the repo patterns each line names. Verified at **3.1**.

- [x] Plan proportions match the venue's **derived envelope** (speaker bbox + 15 % margin), asserted
      against the envelope the plugin computes — never a hardcoded aspect ratio. A probe edits a
      coordinate and asserts the plan's aspect follows it
      (`pattern_test_fixture_mirrors_drift_silently`)
      → **`ui_layout_check.js` §2 — rendered plan box 448.0 × 560.0 px, aspect 0.8000 == the RETURNED envelope's 0.8000; stub bbox mutated to a landscape venue, re-measured 3.0128 vs returned 3.0000. **NC8**: hardcoding the aspect fails §2.**
- [x] The hull overlay matches the computed hull, **including that speakers 3 and 8 render as
      `ON_EDGE` and not as vertices**. This is the visual half of DSP-03's §3.1.6 classification and
      it is the one hull property a glance can check in the hall
      → **`ui_layout_check.js` §3 — `polygon.points.numberOfItems` 6 == returned `hullCount` 6; `glyph-3`/`glyph-8` carry `is-onedge`, the other six `is-vertex`. **C++ half, probe BK**: `getNumHullPoints() == 6`, measured classes `1:V 2:V 3:E 4:V 5:V 6:V 7:V 8:E`. Confirmed in WKWebView at Gate 13.**
- [x] Puck drag is **relative-delta, not absolute cursor tracking** (juce8-critical-patterns §16)
      → **`ui_layout_check.js` §4 — grabbed **8 px off-centre**: 0.00 px jump on pointerdown, then 40.00 px of puck travel for a 40 px pointer move. `ui_frontend_check.js` §19 asserts the accumulator is clamped and stored back. **NC5**: absolute tracking fails §4 (33.00 px). **NC6**: clamping only at the write fails §5 (0.00 px on reversal).**
- [x] The canvas is sized with explicit `width`/`height` in `calc()` plus a DPR backing store — never
      left+right stretch (`o-textureforge-cursor-bug`: canvas is a CSS *replaced* element, so
      left+right does not stretch it and the bug is invisible at one DPR)
      → **`ui_layout_check.js` §6 — DPR 1: backing store 448×560 == round(rect·dpr); DPR 2: 896×1120 == round(rect·dpr), and the two differ. `ui_frontend_check.js` §17 asserts `calc()` width/height and the absence of left+right. **NC7**: dropping the backing store fails §6.**
- [x] `srcX`/`srcY` read out in **metres resolved against the live venue** through a
      `getNativeFunction` call. **Non-vacuity gate:** with the puck stationary, editing a venue
      coordinate must change the readout. A JS-side min/max map passes a static check and fails this
      one (`pattern_webview_knob_readout_scaled_value`)
      → **Three parts (P45). **(a)** `ui_layout_check.js` §7 — puck stationary (srcX/srcY unmoved to 1e-6), stub bbox mutated, readout moved `12.36 × 12.00 m` → `129.65 × 220.00 m`. **(b)** probe **BL** — `applyVenueEdit` moved `bbMaxX` 12.500→15.500 and `bbMinY` 4.500→2.500, generation 3→4. **(c)** `ui_frontend_check.js` §13 — the `getVenueGeometry` body reads `getVenue()`/`getHull()` and contains **zero** float literals. **NC2**: one literal bound fails §13. *The end-to-end half is a declared **3.2 gate**.***
- [x] Rendered against `tests/ui-stub/juce-stub.js` **before** C++ integration, so a top-level TDZ
      throw cannot silently kill every later initializer (`pattern_module_toplevel_init_tdz`)
      → **`ui_layout_check.js` ran to **10/10 PASS at 2026-08-12T14:22:05Z**, when `Source/PluginEditor.cpp` did not exist — its §0 recorded the pre-integration run explicitly. Zero console errors, 17/17 controls present, 17/17 readouts populated. Re-run post-integration (Task 10): 10/10, §0 then cross-checked `setSize`.**
- [x] Every `getNativeFunction` in JS has a matching `withNativeFunction` in C++, asserted by
      grep-diff — zero gaps in **both** directions (`pattern_webview_native_fn_bridge_gap`)
      → **`ui_frontend_check.js` §3 — surface is **exactly 3** (`getParameterDefaults`, `getStatus`, `getVenueGeometry`); JS-called == C++-registered == stub-whitelisted, diffed in both directions; an unknown name REJECTS in the stub. Re-confirmed independently at 3.1 verify by a hand-run grep-diff: the three `nativeFn("…")` call sites in `app.js`/`roomplan.js` == the three `withNativeFunction` registrations in `PluginEditor.cpp`, zero gaps either way.**

### UI-03: Live per-speaker level indicators

> **Section added 2026-08-12 at the Stage 3 discuss boundary.** UI-03 carried a summary-table row and
> no acceptance criteria. Criteria derived from `ARCHITECTURE.md` §4.3 and `ROADMAP.md` Phase 3.3.
> Verified at **3.3**. `should` priority and explicitly **not** descopable — §R7 names it a defence
> on R1.

- [x] Eight indicators sited at their plan positions, driven by the `std::atomic<float>[8]` metered
      **post-map and post-trim** (§4.3) — i.e. what actually leaves the plugin, not the computed `v_i`
      — *met at stage-3 phase 3.3; the read is the **last statement in `processBlock`**, after
      `gainStage.process` and after the ping's post-write overwrite, indexed through
      **`snapshot.speakerToBuffer`** and never the processor member. Layout **§23**: all eight arcs
      are children of their glyph `<g>` and every one clears the glyph stroke — inner edge
      **13.75 px** against a dot outer edge of **12.00 px**. The array carries
      `static_assert (std::atomic<float>::is_always_lock_free)` beside it*
- [x] **The speaker that lights is the speaker the sound comes from**, cross-checked against
      verify-ping stepping 1→8. This is the second, human line of defence on R1 and the reason §4.3
      meters the written buffer rather than the solve. A probe that meters `v_i` would light
      correctly under a bypassed channel map — the exact NC3 failure from 2.2 — *met at stage-3
      phase 3.3; probe **CM**, **on a NON-IDENTITY map** — the probe asserts the permutation FIRST,
      because all three accepted 8-channel containers have initializer order == enum-bit order and
      the shipped default map IS the identity. Ping stepping 1→8: **wrongIndex 0, leaked 0,
      silent 0**, with speaker 1 lighting meter 1 while its audio is in buffer lane 1, speaker 2 →
      meter 2 / lane 2, and so on across all eight. **NC4** injected the defect in its executable
      form — index by identity instead of through the map, which is what a bypassed map IS — and CM
      fired with **wrongIndex 8** (spk1→meter2) while CN still saw 8/8 meters register. That
      asymmetry is what makes CM the guard*
- [x] Ballistics per §4.3: attack **0.5**, decay **0.12**, scale **−60..0 dBFS**, 1.5 s peak-hold
      releasing at 20 dB/s, `requestAnimationFrame` loop with separate current/target
      (juce8-critical-patterns §20) — *met at stage-3 phase 3.3; frontend **§34** asserts the
      coefficients are named `ATTACK_PER_FRAME` / `DECAY_PER_FRAME`, that **`tick()` — the poll —
      applies neither of them**, and that the hold and release are wall-clock constants
      (`PEAK_HOLD_MS`, `PEAK_RELEASE_DB_PER_S`). Two clocks, deliberately: a per-frame coefficient
      driven on the poll clock is `pattern_block_rate_envelope_breaks_blocksize_invariance` in UI
      form, where a throttled tab would change the attack time. **The visual half is Gate 13 and
      HAS NOT RUN** — see the residual note below*
- [x] UI reads and zeroes at ~30 Hz — by a **fixed-interval JS pull whose in-flight guard releases
      on a deadline**, not a `juce::Timer` and not a `poll().then(poll)` recursion; **PERF-01 does
      not regress** — probe AO's replaced `operator new` family re-run with metering live must still
      read **0 allocations** in `processBlock` — *met at stage-3 phase 3.3, WITH ONE RESIDUAL.
      Frontend **§34**: a fixed `window.setInterval` at **33 ms**, no `setTimeout` anywhere in the
      module, and `tick()` schedules nothing. Frontend **§33**: every in-flight guard on the page —
      three of them — is timestamped and released on a DEADLINE, and `pollStatus` still has **no
      guard at all**, which is why it is the one poll that self-heals (P71 rule 3). Frontend **§37**
      proves the two rates are separate and that `getMeters` constructs no `juce::String`. Probe
      **CN**: **0 allocations across 65 `processBlock` calls** including an 8192-sample over-size
      block, with **8/8 meters registering the render** so the measurement is not of code that never
      ran.*

      > **RESIDUAL RESOLVED at the 3.3 verify boundary — it was a defect in the PROBE, not in the
      > plugin.** Execute carried forward that probe AO had reported **1 allocation once in 35
      > runs**, unreproduced and unattributed. Verify reproduced it **4 times in 40 runs under
      > 8-way CPU load** and attributed **all four** to a thread other than the one calling
      > `processBlock`: `rtcheck::armed` was a **process-wide** flag, so the replaced
      > `operator new` counted any thread's allocation — the JUCE message thread and the macOS
      > runtime threads a console app keeps alive included. Contention does not make `processBlock`
      > allocate; it widens the window in which another thread's allocation lands inside it. The
      > counter is now scoped to the arming thread, every arm site routes through one
      > `rtcheck::arm()`, and the foreign tally is still taken and **still reported** beside the
      > verdict rather than hidden. **Before: 4 of 40 loaded runs failed. After: 0 of 40**, with the
      > one foreign allocation observed printed as `[+1 foreign-thread, NOT counted]`.
      > **`PERF-01` never regressed** — `processBlock` measured 0 at every load level. Unchanged:
      > locks and file I/O remain grep + inspection, `-fsanitize=realtime` being unsupported by
      > Apple clang 17.0.0.

      > **Mechanism clause corrected 2026-08-12 at the Phase 3.3 plan boundary (P70).** As written
      > on 2026-08-12 at the Stage 3 discuss boundary this criterion said *"at ~30 Hz on a Timer"*.
      > It was derived from `ARCHITECTURE.md` §4.3, which carried the same error and **was corrected
      > at the 3.3 discuss boundary** (amendment 2) — so this criterion inherited a mechanism the
      > pinned contract now forbids, and verifying it literally would have required undoing 3.1's
      > `Timer`-free editor. **The rate, the zeroing and the PERF-01 clause are unchanged and are
      > the testable content**; only the mechanism phrase moved, and it moved to match the contract
      > rather than away from it. The deadline half is RESEARCH-3.3 N9, measured on the shipping
      > page: a guard released only on settlement latches permanently on the first dropped
      > completion.

### UI-04: DBAP level-field gradient backdrop

> **Section added 2026-08-12 at the Stage 3 discuss boundary.** UI-04 carried a summary-table row and
> no acceptance criteria. Criteria derived from `ROADMAP.md` Phase 3.3 and the DBAP paper figs 1-3.
> Verified at **3.3**. `nice` priority — first on §R7's descope path to v1.1.

- [x] The gradient matches the solver: **20 grid points sampled and compared against a direct solve
      to 1e-3**. Asserted against the shipping `DbapSolver`, never a re-implementation in JS — a
      JS re-derivation is a mirrored fixture and would drift silently — *met at stage-3 phase 3.3;
      probe **CB**, **worst |Δ| 0.000000055 against a tolerance of 1e-3**, over 20 cells of the
      32 × 40 grid. THE ORACLE IS INDEPENDENT: the distances, the pow loop and the −24 dB trim law
      are written out in double at the probe site, not routed back through `dbap::solve`. The first
      version of this probe FAILED on its own non-vacuity guard — 0 of 20 strided cells landed
      outside the hull, so `hullTrimGain` was multiplying by bit-exact unity everywhere and half the
      chain was untested; the sample set now draws 6 of its 20 from the **66 outside-hull cells**.
      The quantity is **`1/k = √denom`** and not `max_i v_i²` (P69 / N10) — probe **CA** confirms
      `outInvK` equals an independent √denom to 6e-8 and that the eight gains are **bit-identical**
      to the nullptr path. **NC6** restored `max_i v_i²` and CB fired at |Δ| 0.246*
- [x] Recomputed on the **message thread, on geometry/weight change only** — never per frame.
      Asserted by a **counter**, not by eye: dragging the puck for N frames must leave the recompute
      count unchanged, because the field depends on speakers and weights, not on source position —
      *met at stage-3 phase 3.3; layout **§27**: **24 frames of an off-centre puck drag** (38 srcX
      and 38 srcY writes, so the drag demonstrably happened) left the `getFieldGrid` INVOCATION
      count at **12 → 12**, and the positive control — moving `blur` — took it **12 → 13**. Probe
      **CD** carries the C++ half: the counter reads 0 → 1 → 6 exactly, and **all five real inputs
      move the field** (geometry, the eight weights, rolloff, blur, hullAtten) — three of which
      UI-04/2's own wording omits and all three of which are automatable at audio rate (N12), which
      is why the recompute is coalesced to at most one per 2 Hz status tick. Probe **CC**: the field
      is **bitwise identical across 5 source states × 1280 cells**, with the sub-points measurably
      moving 4/4 times so the sweep is not inert*
- [x] Drawn to an offscreen canvas and blitted; no CPU spike during puck drag — *met at stage-3
      phase 3.3; frontend **§38** asserts the chain **`atob` → `putImageData` → `drawImage` in that
      order** onto a `document.createElement("canvas")` at the grid's own 32 × 40 resolution, with
      the upscale left to the browser's bilinear smoothing rather than a JS loop — and that
      `js/field.js` contains **no part of the solve** (no `Math.pow`, `Math.sqrt`, `Math.hypot`,
      `rolloff` or `denom`). The payload is **1.7 kB** against 61 kB for float-per-cell JSON (Q2).
      Layout **§26**: the backdrop carries an explicit CSS box with `backing == round(rect × dpr)`,
      the field is measurably PAINTED (**1600/1600 sampled subpixels differ from the flat fill**),
      and the legend prints the returned span **"1.3 – 10.4 dB"**. No CPU spike follows from §27:
      the drag triggers no recompute at all*
- [x] Descopable to v1.1 **without touching any other component** — asserted structurally by the
      backdrop being a separate draw layer, so the descope decision at 3.3 costs a flag and not a
      refactor — *met at stage-3 phase 3.3; frontend **§39**: `js/field.js` is imported by
      **app.js and nothing else**, `#plan-backdrop` is its own `<canvas>` element (authored at 3.1
      for exactly this), and `roomplan.js`'s painter hook **defaults to null with the draw path
      guarded** — so not installing the painter IS the descope and leaves the 3.1 backdrop exactly
      as it was. D15 shipped it rather than descoping, and this criterion is what keeps that
      reversible*

### UI-05: Side-elevation strip

> **Section added 2026-08-12 at the Stage 3 discuss boundary.** UI-05 carried a summary-table row and
> no acceptance criteria. Criteria derived from `ROADMAP.md` Phase 3.3 and DSP-04 / `ARCHITECTURE.md`
> §OQ4. Verified at **3.3**. `nice` priority — second on §R7's descope path to v1.1.

- [x] The raked audience line is drawn from `rakeFront` at `bbMinY` to `rakeRear` at `bbMaxY`, and
      **changing `rakeRear` alone visibly changes the rear of the line** — the visual analogue of
      DSP-04's criterion, whose negative half exists because `earHeight(bbMinY) = rakeFront` for any
      `rakeRear` (RESEARCH-2.2 H5) — *met at stage-3 phase 3.3, **BOTH HALVES**; layout **§22**,
      `rakeRear` 0 → 2.0 m moved the rear endpoint **y2 107 → 79.29 px** while the front endpoint
      **did not move at all** — (83.69, 107) → (83.69, 107), exact. The quantised axis also did not
      rescale: **8 ticks → 8 ticks**. Frontend **§41** asserts the construction statically: the
      solid line's endpoints ARE `(bbMinY, rakeFront)` and `(bbMaxY, rakeRear)`, the extrapolation
      is a **separate dashed element**, and the solid line never reads the envelope. **NC7** drew
      one line across the whole envelope and the NEGATIVE half fired — front moved 107 → 111.16 —
      **while the positive half still passed**, which is exactly the asymmetry the criterion was
      written for*
- [x] With `srcZ = 0`, moving the puck front-to-back keeps the source marker **riding the rake** —
      its absolute height changes while its height *above the plane* stays zero. Both readings shown,
      because the parameter is the second one and the confusing one — *met at stage-3 phase 3.3;
      layout **§22** reads both — ear **"1.20 m"** and source **"2.20 m"** — in the group title row,
      which costs ~0 px against the 172 px budget (P72's trick reused). The marker is drawn as a
      stem from the ear height to the absolute height, so `srcZ = 0` renders as a zero-length stem
      ON the rake by construction. **THE MARKER CLAMPS, THE NUMBERS NEVER DO** (P76 rule 3): driving
      `srcZ` to its +8.00 m maximum printed **"9.20 m"** against a 7 m axis while the dot pinned to
      the edge with a chevron and stayed inside the strip at cy 10.0 of 123 px. `earHeight` is
      imported from `roomplan.js`, not re-implemented — it is computed BY `metresToPx`, so §19's
      one-projection rule is honoured rather than argued around*
- [x] Speaker heights are shown at their y positions, so the §OQ4 graded defaults are visible as the
      3-D geometry DSP-01 actually solves against — *met at stage-3 phase 3.3; layout **§22**:
      **8 speakers drawn**, with **4 distinct y positions against 4 distinct RETURNED z values**
      (z = [4.5, 4.5, 4.7, 5.1, 5.4, 5.4, 5.1, 4.7]) — asserted against the payload rather than a
      literal, so a strip that drew all eight flat would fail. A section collapses the lateral axis,
      so the four mirrored pairs land on one point each; the DOTS stay exact and only the NUMERAL
      steps aside, which was found by looking at the rendered page and reads **1 2 / 3 8 / 4 7 /
      5 6**. The grading spans 17.3 px on this box and is legible*
- [x] Descopable to v1.1 without touching any other component — *met at stage-3 phase 3.3;
      frontend **§40**: `js/elevation.js` is imported by **app.js and nothing else**, no other
      module reaches for an `elev-` element, and `#group-elevation` is the **last group authored in
      the controls column** — asserted statically here and MEASURED on the rendered tree by layout
      **§22**, because a future phase can break either one alone. Removing it returns 172 px of
      slack and touches nothing. **NC8** inserted a node after it: §22 fired while §21's fitted-box
      guard still passed, proving the coarse column assertion would have gone silently vacuous and
      that §22 is what catches it*

### PERF-01: Real-time safety

- [x] No heap allocation, lock, or file I/O in `processBlock` — *met at stage-2 phase 2.2, **with
      the method stated** (RESEARCH H8). **Allocation is MEASURED:** probe AO replaces the entire
      global `operator new` family — plain, `[]`, both `std::align_val_t` overloads and every matching
      `operator delete` — with counting `malloc`/`posix_memalign` wrappers, warmed up with one
      un-counted block so libc++/JUCE first-touch is not attributed to the audio path. **0 allocations
      across 66 `processBlock` calls**, including an 8192-sample over-size block and a mid-stream
      venue edit. **Locks and file I/O are grep + inspection ONLY, not measured.**
      `-fsanitize=realtime` is unsupported by Apple clang 17.0.0, verified by running it. Do not
      restate this as "RT-safety harness pass"*
- [x] `pow()` calls occur on parameter change, not per sample — *met at stage-2 phase 2.2; probe AP,
      idle blocks 0 solves and **0 pow**, a parameter change 1 solve and **16 pow***
- [x] Hull projection executes only when the source is outside the hull — *met at stage-2 phase 2.2;
      probe AP, inside projections **0**, outside projections 2. AP **searches** for a normalised
      position the plugin's own hull calls outside and asserts it found one first — `srcX=0, srcY=0`
      denormalises to exactly speaker 1, a hull **vertex**, which the inside-or-on test accepts
      (SUMMARY-2.2 F3)*

### PERF-02: Solve scheduling and smoothing

> **Section added 2026-08-11 at the Phase 2.2 discuss boundary.** PERF-02 carried a summary-table row
> but no acceptance criteria — it would have been "verified" at 2.2 against nothing. Criteria are
> derived from `ROADMAP.md` Phase 2.2 and `ARCHITECTURE.md` §3.6.2 / §3.6.4 / §3.3.5, not invented.

- [x] The solve runs on the **64-sample absolute-sample-aligned control grid** — *met at stage-2
      phase 2.2; probes AL / AM / AN, bit-identical by `memcmp` across 512-vs-4096 and the ragged
      `1,7,64,333,4096` sequence, which is only possible if the grid is absolute-sample-aligned and
      independent of the host's block boundary*
- [x] With the parameter snapshot **and** the venue generation both unchanged, the solve is skipped
      — *met at stage-2 phase 2.2; probe AP, idle solves **0**, pow **0**. Venue-edit solves **1**
      (probe AQ). **Proven non-vacuous at verify (NC4):** removing the generation term from the dirty
      check makes AQ fail with "THE DIRTY CHECK IS STALE" and drops AP's venue-edit solves to 0*
- [x] `pow()` count per control block confirmed ≤ 32 — *met at stage-2 phase 2.2; probe AE asserts
      **exactly 16**, not merely under the bound: the bound alone still passes if the
      `t = pow(d, −a)` / `t²` reuse is dropped*
- [x] Every gain reaching the output is a per-sample `SmoothedValue` ramp with `getNextValue()` called
      exactly once per sample per smoother, unconditionally — *met at stage-2 phase 2.2; probe AT,
      `sampleAdvances == totalSamples` across 5 configurations including all four `auval` pairs, in
      **both** real and SAFE mode*

### COMPAT-01: pluginval

- [x] VST3 passes pluginval strictness 10 — *SUCCESS ×3, re-run at Stage 1 verify*
- [x] AU passes pluginval strictness 10 and `auval` — *SUCCESS ×3; `AU VALIDATION SUCCEEDED`*
- [x] Run 2-3 times locally before any publish (per known latent-NaN pattern) — *3 runs per format*

### COMPAT-02: Logic Pro

- [ ] Instantiates on a surround track with 7.1 output — *observed working at the stage-2 phase 2.1
      manual gate: Logic negotiated **`7.1`** (`create7point1()`), all 8 surround-meter lanes moved.
      **This contradicts the Stage-0 R2 prediction of 7.1-SDDS**, which is now retired in favour of
      the observation. Left open here because COMPAT-02 is verified at stage-4*
- [ ] Verify-ping confirms all 8 outputs reach distinct physical channels — *needs FUNC-04 (stage-3).
      The 2.1 meter check proves negotiation and writability only: all 8 lanes carry identical
      signal at that phase. Do not over-read it.*
      **Scope stated 2026-08-13 at the 4.2 discuss boundary (D11); the criterion wording above is
      deliberately left unchanged.** *No physical 8-out interface is attached, so this closes
      against the **CoreAudio device boundary** — BlackHole 64ch, evidenced by **per-channel
      capture and script analysis**, which exercises Logic's surround assignment, I/O routing and
      the driver boundary, and is stronger evidence than eight moving meters. The residual is
      **one specific hardware driver**, recorded with **owner: none** — a property of hardware that
      would not generalise across interfaces even if one were present. A criterion is never
      re-worded to fit its rig; see ROADMAP §Stage 4 D11*
- [ ] Automation of `srcX`/`srcY`/`srcZ` and `w1..w8` is visible and writable in Logic's automation lanes
      — *visibility observed at the 2.1 manual gate: 17 parameters under 5 groups, matching the
      `auval` clump dump. Writability not yet exercised per-parameter*

### COMPAT-03: Channel map

- [x] Grep confirms zero hardcoded channel indices in the output path — *met at stage-2 phase 2.1;
      re-run at verify, 3 hits all doc-comment prose, plus a line-by-line read of `processBlock`*
- [x] Unit test asserts the map built by `getChannelIndexForType()` against known JUCE enum-bit order
      for 7.1 — *met at stage-2 phase 2.1; probes A and B, all 3 accepted 8-channel sets, scan bound
      256 and the `expected.size() == set.size()` assertion (G2)*
- [x] Test fails loudly if JUCE's enum-bit order changes (asserted against parsed source, not a
      mirrored constant) — *met at stage-2 phase 2.1, and **proven by negative control at verify**:
      mutating `leftSurroundRear = 20 → 90` in a copied JUCE tree moved the generated SHA
      `5cd774cb…` → `39298098…`, and substituting that value for the committed constant produced
      `error: static assertion failed` with `BUILD_EXIT=1`. The gate fails the **build**, per
      ROADMAP:131 — not merely a test run*

### COMPAT-04: Defined behaviour on a stereo track

> **Section added 2026-08-12 at the Stage 4 phase 4.1 discuss boundary (D8).** COMPAT-04 carried a
> summary-table row and **no criteria** since Stage 1 — the last of this project's 30 rows in that
> state, and owed since Phase 2.2 verify. It was marked ✅ complete at stage-1 against prose, which is
> exactly the defect repaired for PERF-02 and QUAL-04 at the 2.2 boundary and for `FUNC-06` /
> `UI-02..05` at the Stage 3 phase boundaries. Criteria derived here **from shipped source**
> (`PluginProcessor.cpp:171-197`, `:226-239`) and from `ARCHITECTURE.md`, **before any 4.1
> implementation exists to shape them.** Verified at stage-4 phase 4.1.
>
> **Two properties of *how the policy is written* are load-bearing, and either could regress without
> a compile error.** The criteria assert both, not just today's behaviour:
>
> 1. `isBusesLayoutSupported()`'s mono/stereo-out arm is **load-bearing for AU, not only for
>    Standalone.** JUCE derives `AUChannelInfo = {(1,1),(1,2),(1,8),(2,1),(2,2),(2,8)}` from the
>    predicate and `auval` exercises all six — narrowing it silently changes what `auval` tests.
> 2. `safeMode` is derived in `prepareToPlay()` as the **complement of the three real containers**,
>    never as `== mono() || == stereo()`. Written the second way, a fourth 8-channel container
>    admitted later would silently stop raising the banner. **A test of today's five sets passes for
>    both spellings** — hence criterion 3's negative control, which is the non-vacuity clause.

> **Status on derivation: ⚠️ partial, 2 of 3.** Criteria 1 and 2 carry **real stage-1 evidence** and
> are ticked against it, not against prose — the tick was under-documented, not unearned. **Criterion
> 3 is genuinely new**, because `safeMode` did not exist when the row was ticked: it was added at
> Phase 3.1 (P43), *after* Stage 1 closed. A row completed before the code it now describes was
> written is the one honest way this project has been ticking things it had not measured, and it is
> worth naming as its own shape.

- [x] `isBusesLayoutSupported()` accepts `(1,1)`, `(1,2)`, `(2,1)` and `(2,2)`; `auval` reports all
      six `AUChannelInfo` configs and passes — *met at stage-1;
      `stages/1-foundation/VERIFICATION.md` records `auval` exercising all four configs against
      `PluginProcessor.cpp:162-163`. **Re-run at 4.1 on the final binary**, since the predicate is
      byte-identical to `a47cef88` but the plugin around it is not*
      → **RE-CONFIRMED at 4.1 on the final binary.** `auval -v aufx OuOc OuDv` exit 0, **AU
      VALIDATION SUCCEEDED**, and the reported channel capabilities are exactly the six:
      `[1,1] [1,2] [1,8] [2,1] [2,2] [2,8]`. The predicate is no longer byte-identical to
      `a47cef88` — 4.1 Task 3 routed its three-container arm through `oo::rig::isRealRig`
      (P91) — and the six configs are unchanged across that edit, which is what makes the
      extraction provably behaviour-neutral on the AU side as well as through probe BM.
- [x] Instantiation on a 2-channel output is **defined and non-destructive**: Standalone launched on
      a 2-channel device opens and stays running — *met at stage-1 on MacBook Pro Speakers (2 out).
      **4.1 adds the render clause**: a pass through the SAFE fold produces finite samples with no
      NaN/Inf, which stage-1 had no DSP to exercise*
      → **RENDER CLAUSE MET at 4.1 — probe CQ** (`tests/render-harness/main.cpp`). Full-scale sine
      through **both** SAFE widths (mono-in/stereo-out and mono-in/mono-out), all nine ranged
      parameters driven to a range endpoint read off the **live** `NormalisableRange`, weights split
      across both rails, 8 blocks of 512. Every output sample finite; peak 1.000 on both. The probe
      also asserts `isSafeMode()` is true before rendering, so it cannot pass by silently having
      tested a real rig.
- [x] The SAFE banner is raised on mono/stereo out and **not** on any of the three real containers,
      asserted through the **complement predicate** — with a negative control on a **fourth
      8-channel set** confirming the banner **is raised**, which is what discriminates the complement
      spelling from `== mono() || == stereo()`, under which it would stay down. **NEW at 4.1**:
      `safeMode` landed at Phase 3.1 (P43), after this row was ticked, so no existing evidence
      touches it
      > **Wording corrected 2026-08-12 at the Stage 4 phase 4.1 plan boundary (A3 / P90).** This
      > criterion read "confirms the banner **stays down**" — the outcome of the `== mono || ==
      > stereo` spelling that note 2 above explicitly rejects, not of the complement form the plugin
      > ships. Under `PluginProcessor.cpp:230-238` a fourth container is not one of the three real
      > rigs, so `realRig` is false, `safeMode` is true, and the banner goes **up**. **Verified as
      > written, this criterion passed only if the code carried the defect it was written to
      > exclude.** Found by applying Stage 3's parting rule at the plan boundary; the same sentence
      > in `ROADMAP.md` §4.1 was corrected in the same edit, and a third site — the shipped comment
      > at `PluginProcessor.cpp:229-232` — states the same direction backwards and is corrected in
      > code at 4.1 Task 3. **Two-thirds of this criterion turns out to be already met**: probe BM
      > (`tests/render-harness/main.cpp:3807-3850`, Phase 3.1) drives all five layouts through a
      > real `prepareToPlay` and asserts `isSafeMode()` against each. It was invisible to this
      > ledger because the row had no criteria section until 2026-08-12 — the mirror of this stage's
      > declared hazard: **evidence nobody claimed**, rather than a residual nobody owned.

      → **MET at 4.1 by a PAIR of probes, neither of which is sufficient alone.**

      → **CO** (`tests/unit/main.cpp`, 45 probes) calls `oo::rig::isRealRig` **directly** and asserts
      the partition as a *form*: true for exactly `create7point1()`, `create7point1SDDS()`,
      `create5point1point2()`; false for `create7point1point4()` (12 ch), `octagonal()` (8 ch),
      `quadraphonic()`, `mono()`, `stereo()`. **The negative control this criterion asks for is rows
      4 and 5**: both are ≥ 8-channel sets that are not real rigs, so the shipped complement form
      reports them SAFE and **the banner goes up**. CO additionally asserts both are actually ≥ 8
      channels, so the discriminating rows cannot quietly become vacuous if JUCE stops offering them.

      → **BM** (`tests/render-harness/main.cpp`, Phase 3.1) drives all five reachable layouts through
      a real `prepareToPlay` and asserts `isSafeMode()` — the *wiring*. It structurally cannot reach
      the discriminating sets, because it arrives through the host negotiation that filters them out.

      → **NC1 is what makes the pairing a measurement rather than an assertion.** `isRealRig` was
      rewritten as the rejected `!= mono() && != stereo()` spelling and both targets re-run: **CO
      failed**, naming 7.1.4 and octagonal as "READ AS REAL — BANNER DOWN ON AN UNMAPPED RIG", while
      **BM passed, 50 probes / 0 failures**. The mutation was reverted and `RigPolicy.h` confirmed
      byte-identical (`33e33845…`). BM alone cannot see the defect this criterion exists to exclude;
      CO alone cannot see a broken derivation site. Both, or neither.

      → **P90's third site is also closed.** The comment at `PluginProcessor.cpp:229-232` stated the
      same outcome backwards and was restated in the same edit that extracted the predicate — which
      mattered here because Task 3 *moved those exact lines*, and an unfixed sentence would have been
      promoted to the doc-comment on the function CO pins.

### QUAL-01: No artifacts

- [x] Full-range sweeps of every parameter produce no clicks, discontinuities, or level jumps —
      *met at stage-2 phase 2.3 across **17 of 17** automatable parameters, coverage being arithmetic
      rather than assertion: **AS (11) + AZ (5) + BC (1) = 17**. AS: position 0.0008203, weights
      0.0016529 against a 0.0041677 bound. AZ: `width` 0.0004728, `rolloff` 0.0003437, `blur`
      0.0004737, `hullAtten` 0.0011232, `outputGain` 0.0019509 — **all five under bound and all five
      negative controls fire**, sweep endpoints read from `getAPVTS().getParameterRange(id)` rather
      than hand-written. BC: the `airAmount` two-render differential, `|D| 0.006904 ≤ 0.014221` at
      8 kHz, **28.3×** the 64-grid bound. One live venue edit during playback (BG, worst step
      0.0009429) and FUNC-07's trim step (BF, 0.0008819) cover the 42 message-thread venue values,
      per the D4 scope below*
- [x] Rapid puck movement across the hull boundary produces no audible discontinuity — *met at
      stage-2 phase 2.3 on **both** excitations, per the D3 method below. **DC:** probe BD, 12
      crossings, worst step 0.0009319 against 0.0041677; probe BB, 0.0008494. **Sine:** probe BB, the
      **entry edge is BIT-EXACT at both 1 kHz and 8 kHz**, and the exit step matches the predicted
      `A·|H_20k(f) − 1|` to **0.000 %** — 0.005328 at 1 kHz and 0.046267 at 8 kHz, asserted as an
      equality against a prediction rather than as a ceiling. **Proven non-vacuous at verify (NC1):**
      removing P27's entry re-seed makes BB report `entry NOT BIT-EXACT — P27's SEED IS GONE` and
      blows the DC step to 0.1367314, 33× the bound. **The audibility clause is bounded but not
      concluded — see the note below.***

> **The *audible* clause of criterion 2 is measured, not listened to** *(recorded 2026-08-11 at 2.3
> verify)*. Everything measurement can settle is settled: the entry edge is bit-exact, the exit step
> equals its prediction to 0.000 %, and the DC path is continuous across 12 crossings. What
> measurement cannot conclude is whether **~15 % of an 8 kHz component, as a one-sample step on a
> deliberate gesture**, ticks audibly on HF-rich material. That is the **D5 listening session's H2
> item**, carried OPEN into Stage 3 discuss. The requirement is ticked ✅ because every measurable
> clause is met and the residual is a named, bounded, single-gesture artifact with a documented lever
> (RESEARCH-2.3 H3 — raising `fc(d_hull = 0)` toward Nyquist, which re-tunes the whole musical curve
> and is therefore a discuss-boundary change, not a fix).
>
> **Probe AS's position figure moved between 2.2 and 2.3, and it is a real behavioural change.**
> 2.2 verify recorded **0.0008846**; 2.3 verify measures **0.0008203** (negative control 0.0564730 →
> 0.0524102). The weights half is unchanged to the last digit. `hullAtten` defaults to 1.0 dB/m, so a
> full-range position sweep now crosses the hull and picks up the trim and the air stage. **From 2.3
> onward QUAL-04 criteria 1 and 2 depend on a Phase 2.3 mechanism** — NC1 drives AS to 0.3097257 when
> P27's seed is removed.

> **Scope and method, fixed at the Phase 2.3 discuss boundary (decisions D3 and D4).** Stated here so
> verify discovers nothing the plan did not predict.
>
> **Scope (D4).** "Every parameter" means the **17 automatable musical parameters** — QUAL-01 is a
> statement about automation, and only APVTS parameters ride an automation lane. Venue values are
> message-thread edits, not sweeps; they are covered by **one representative live-edit probe during
> playback** plus FUNC-07's ±dB trim **step**, since everything a venue edit touches lands on the same
> smoothed targets. A full-speed sweep of all 42 stresses a path no user can drive.
>
> **Method, 16 of 17 (the gain path).** QUAL-04's proven DC construction: the rendered sample **is**
> the gain, bounded by the 5 ms smoother's maximum per-sample delta (1/240 + 1e-6 at 48 kHz), with a
> negative control that must exceed the bound (probe AS, which fires at 13–25×).
>
> **Method, `airAmount` (the filter path) — the DC method is EXACTLY BLIND to it.** A one-pole passes
> DC unchanged in steady state, so an air sweep moves a DC output by literally zero and would pass
> vacuously.
>
> **The method originally specified here was ALSO vacuous, and is replaced** *(corrected 2026-08-11 at
> the Phase 2.3 plan boundary; RESEARCH-2.3 Q6, PLAN-2.3 P34)*. It read: with a 1 kHz sine,
> `max |out[n] − out[n−1]|` under a full-speed sweep must not exceed the same measurement with
> `airAmount` held, plus tolerance. **Simulated, that excess is `+0.00000 %`** — identically zero to
> nine decimals — because the render's maximum slew is set by the sine's own zero crossing, which
> occurs early, where the swept and held renders still share a cutoff. The probe would have passed for
> a reason unrelated to the code.
>
> **Replacement — the two-render differential, with a bound derived rather than tuned.** Render the
> same 1 s `0.02 → 1.0` sweep twice, with the cutoff updated on the 64-sample control grid and again
> on a 4096-sample grid, and **subtract**: the sine cancels to the precision of the two gain
> trajectories. The coefficient step is analytic — `Δy = (G_new − G_old)·(x − s)` read straight off
> `processSample` — so the assertion is
>
> ```
> max |Δy|  ≤  max |ΔG| · 2 · peak
> ```
>
> with `max |ΔG|` computed **in-probe from the sweep schedule the probe itself drives**. There is no
> tolerance to pick and nothing mirrors a constant
> (`pattern_test_fixture_mirrors_drift_silently`). **Negative control:** the 4096-sample render itself,
> which exceeds the 64-sample bound by ~48× — a separation, not a hair.
>
> **Criterion 2 requires the same treatment, against a corrected number.** Under the D2 amendment to
> §3.5.2 the hull boundary steps the air filter's transfer function, so a DC hull-crossing probe cannot
> see the one discontinuity the amendment introduced. Criterion 2 is measured on **both** excitations:
> DC for the gain vector, and sine at **1 kHz *and* 8 kHz** for the filter.
>
> **The quoted cost — "3 dB @ 20 kHz, 0.7 dB @ 10 kHz, 0 dB at DC" — is corrected here**
> *(RESEARCH-2.3 H2; carried as an erratum in `SUMMARY-2.3.md`, `ARCHITECTURE.md` deliberately NOT
> re-pinned — PLAN-2.3 P36)*. Two errors, both understating the cost:
>
> 1. **The 10 kHz figure is the ANALOG one-pole's** (−0.969 dB at `f/fc = 0.5`). The digital TPT filter
>    at `fc = 20 kHz` is far flatter, because 20 kHz sits at 0.83 × Nyquist and bilinear prewarping
>    compresses the passband:
>
>    | fs | @1 kHz | @4 kHz | @8 kHz | @10 kHz | @15 kHz | @20 kHz |
>    |---|---|---|---|---|---|---|
>    | 44 100 | −0.0005 dB | −0.0081 | −0.0384 | **−0.0695** | −0.3008 | −3.0103 |
>    | 48 000 | −0.0013 dB | −0.0223 | −0.1027 | **−0.1798** | −0.6476 | −3.0103 |
>
>    The 20 kHz figure is right — −3.0103 dB **is** the definition of the cutoff.
>
> 2. **Magnitude is the wrong quantity.** The step at the crossing is the difference between two signal
>    *paths*, so it is the complex `H(f) − 1`, and the filter's **phase lag dominates everywhere below
>    ~15 kHz** — by **114×** at 1 kHz (48 kHz) and **190×** at 44.1 kHz. The real accepted cost is
>    **1.8 % of a 1 kHz component and 15 % of an 8 kHz component**, as a one-sample step.
>
> **Criterion 2's bound is therefore `A · |H_20k(f) − 1|`**, computed in-probe from the transfer
> function at the actual `fs` — 1.756e-2 and 1.529e-1 per unit amplitude at 48 kHz. Asserting the
> *predicted* number rather than "under some bound" makes the probe fail loudly if the entry-edge
> re-seed is ever dropped, because a resident-state edge is ~20× larger at 1 kHz. The **entry** edge
> (inside → outside) is asserted **bit-exact**: seeding the filter state with the incoming sample makes
> `v = G·(x − s) = 0.0f` and `y == x` by the arithmetic. The **exit** edge is inherent to D2 and is
> reported as a measured number rather than asserted away.
>
> **"0 dB at DC" is retained and is still true** — the phase lag also goes to zero at DC — which is
> precisely why the DC probe is blind to the filter and both excitations are required.
>
> **Whether the bounded step is *audible* is settled only by the D5 listening session**, on HF-rich
> material at the hull boundary. Measurement bounds it; it cannot conclude the "no *audible*
> discontinuity" clause on its own.

### QUAL-02: No NaN or Inf

- [x] Source at each exact speaker coordinate — finite output at blur = 0 — *met at stage-2 phase
      2.2; probe AD. The §3.3.3 distance floor is applied **unconditionally**, on every path*
- [x] All-zero weights, single non-zero weight, rolloff at both range ends — finite output — *met at
      stage-2 phase 2.2; probes AC and AB*
- [x] Degenerate venue geometry (coincident speakers, zero rake span) — finite output — *met at
      stage-2 phase 2.2; probe AD, coincident rig finite with Σv²=1, collinear rig finite*
- [x] Pathological input produces no sticky NaN state — *met at stage-2 phase 2.2; probe AR covers
      **parameter** NaN as well as input NaN. This is the RESEARCH H2 second latch site: the 17
      `SmoothedValue`s are recursive too and §3.3.4's guard misses NaN (`NaN < eps` is false), so a
      host writing NaN to a parameter — which `jlimit` passes and the `jassert` only catches in Debug
      — would latch before any input NaN could. Mitigated by sanitising the 17-float snapshot at
      ingestion (P17), with the fallback read from the parameter object's declared default*

### QUAL-03: Block-size invariance

- [x] Rendering identical automation at blockSize 512 and 4096 produces bit-identical output — *met
      at stage-2 phase 2.2; probe AL, **bit-identical by `memcmp`** over 24576 samples × 8 lanes*
- [x] An offline bounce is bit-identical to a real-time pass — *met at stage-2 phase 2.2; probe AM,
      the ragged `1,7,64,333,4096` sequence against a fixed 4096 with 7 events at non-aligned
      offsets — a strictly stronger gate than the mandated pair, since an offline bounce differs from
      a real-time pass precisely by presenting arbitrary block sizes (RESEARCH H6). Probe AN adds 6
      further regimes with parameters held constant*
- [x] Any envelope or smoothing runs per sample, not per block — *met at stage-2 phase 2.2; probe AT,
      `sampleAdvances == totalSamples`. Every comparison above is `memcmp`, never a tolerance*

> **Verified under the protocol of `ARCHITECTURE.md` §3.6.3**, which is a refinement of these three
> criteria and not a weakening: parameters are driven **programmatically** by the render harness at
> control-grid-aligned absolute sample offsets, and the comparison is `memcmp`, not a tolerance. A
> strict unqualified reading is untestable — when a host's message-thread automation writes land
> relative to the audio thread is not a property of this plugin.

### QUAL-04: No zipper noise

> **Section added 2026-08-11 at the Phase 2.2 discuss boundary.** As with PERF-02, QUAL-04 had a
> summary-table row and no criteria. Derived from `ROADMAP.md` Phase 2.2 and `ARCHITECTURE.md` §3.6.5.

- [x] A full-speed sweep of `srcX` / `srcY` / `srcZ` produces no sample-to-sample step exceeding the
      5 ms smoother's maximum per-sample delta — *met at stage-2 phase 2.2; probe AS, bound
      0.0041677, measured **0.0008846***
- [x] A full-speed sweep of all 8 weights, including through exact zero, produces the same — *met at
      stage-2 phase 2.2; probe AS, measured **0.0016529** against the same bound. **AS carries a
      working negative control and it fires:** sampling the same rendered series at 64-sample control
      boundaries — what the output would look like with the vector held per control block instead of
      ramped — gives 0.0564730 and 0.1057403, exceeding the bound by 13× and 25×*
- [x] A full-speed sweep of `width` produces the same — *met at stage-2 phase 2.3, and it takes
      **two** probes, not one. **AY** establishes that the control is live: `width = 4` separates 8/8
      sub-point pairs, worst **0.1636034**. **AZ** establishes that sweeping it does not zipper:
      `width [0.0..6.0]` measures **0.0004728** against the 0.0041677 bound, with a negative control
      at 0.03015 that fires.*
      *Staged at the 2.2 discuss phase, not discovered at verify — and the premise was confirmed at
      2.2 verify by reading `GainStage.cpp:147`: `const float widthMetres = 0.0f;`*

> **AZ alone is vacuous for this criterion, and that was established by measurement, not argument**
> *(corrected 2026-08-11 at 2.3 verify; NC6)*. Reverting `widthMetres` to the 2.2 literal `0.0f` makes
> **AY fail** — `width=4: 0/8 pairs separated, worst 0.0000000 — WIDTH IS NOT REACHING THE SHAPER` —
> while **AZ still passes**, because with `width` wired to nothing the swept render is bit-identical
> to the held one and a measured step of zero is under any bound. `SUMMARY-2.3.md` proposed this
> criterion on "AZ's `width` sweep"; that is half the evidence. **QUAL-04/3 = AY (the control is
> live) + AZ (and it does not zipper).** Same class of correction as 2.2 verify's NC3 (probe AI is
> not probe AJ): two probes that look interchangeable in a results table and are not.

---

## Traceability

| Stage | Requirements Verified |
|-------|----------------------|
| stage-1 | COMPAT-01 ✅, COMPAT-04 ✅ *(FUNC-01 partial — re-mapped to stage-2)* |
| stage-2 phase 2.1 | COMPAT-03 ✅, DSP-03 ✅, DSP-04 ⚠️ partial *(criterion 3 → 2.2)*, FUNC-03 ⚠️ partial *(criterion 3 → 2.2)* |
| stage-2 phase 2.2 | **FUNC-01** ✅, DSP-01 ✅, DSP-02 ✅, DSP-05 ✅, PERF-01 ✅, PERF-02 ✅, QUAL-02 ✅, QUAL-03 ✅, QUAL-04 ⚠️ partial *(criterion 3, `width`, → 2.3)*, **+ DSP-04/3 ✅ and FUNC-03/3 ✅** |
| stage-2 phase 2.3 | **FUNC-07** ✅, **DSP-06** ✅, **DSP-07** ✅, **DSP-08** ✅, **QUAL-01** ✅, **+ QUAL-04/3** ✅ — *stage 2 closes, 18/18 rows complete, 0 partial, 0 failed* |
| stage-3 phase 3.1 | **UI-02** ✅ *(7/7 criteria, derived at the 3.1 discuss boundary — the row had none)* |
| stage-3 phase 3.2 | **FUNC-02** ✅, **FUNC-04** ✅, **FUNC-05** ✅, **UI-01** ✅ — 4 `must` rows, 3/3 criteria each |
| stage-3 phase 3.3 | **FUNC-06** ✅, **UI-03** ✅, **UI-04** ✅, **UI-05** ✅ — *stage 3 closes, 9/9 rows complete, 40 criteria, 0 partial, 0 failed* |
| stage-4 phase 4.1 | **COMPAT-04** ✅ *(criterion 3 via CO + BM + NC1; criterion 2's render clause via CQ; criterion 1 re-run on the final binary — criteria derived at the 4.1 discuss boundary, the row had none, and `safeMode` postdates its stage-1 tick)*, **COMPAT-01** ✅ **re-confirmed** on the final binary *(pluginval s10 VST3 ×3 / AU ×3 all exit 0; `auval` SUCCEEDED with all six `AUChannelInfo` configs)* |
| stage-4 phase 4.2 | **COMPAT-02** *(3 criteria, the project's only `pending` row)*, **QUAL-01** criterion 2's audible clause |

> **Ledger entering stage-4** *(recorded 2026-08-12 at the 4.1 discuss boundary)*: **28 complete ·
> 1 partial (`COMPAT-04`) · 1 pending (`COMPAT-02`)**. This corrects the Stage 3 close's "29 of 30",
> which counted `COMPAT-04` complete from its summary row. Reading the stage-1 evidence shows
> criteria 1 and 2 **were** genuinely measured — the tick was under-documented, not unearned — while
> criterion 3 covers `safeMode`, **added at Phase 3.1, two stages after the row closed.** A completed
> row whose subject matter later grew: a distinct shape from the five vacuity defects Stage 3
> catalogued, and the reason 4.1 re-derives rather than merely re-runs.

> **Ledger at the 4.1 close** *(recorded 2026-08-12)*: **29 complete · 0 partial · 1 pending
> (`COMPAT-02`)**. `COMPAT-04` closes 3 of 3 and `COMPAT-01` is re-confirmed against the binary 4.2
> will run, not against the Stage-1 one. **`COMPAT-02` is the only row left**, it carries three
> criteria, and every one of them needs a host — which is exactly why Stage 4 was split: 4.2 runs
> against a binary 4.1 has frozen by commit rather than against whatever was on disk that afternoon.
>
> The three items 4.1 **did not** close, stated as deferrals rather than left as prose: the two JS
> UI gates do not run in CI *(owner none, blocked on headless-render determinism — 69 sections, green
> today, gated by a human running `node`)*; Windows **UI correctness** is unproven *(owner none,
> blocked on hardware — CI's ceiling is that pluginval 10 opens the editor without timing out)*; and
> RT-safety beyond allocation remains grep plus inspection *(`-fsanitize=realtime` is unsupported by
> Apple clang 17.0.0; allocation is now measured soundly, which makes this gap **sharper, not
> smaller**)*.

---

## Out of Scope (v1.0)

| Feature | Reason | Future Version |
|---------|--------|----------------|
| VBAP secondary mode (A/B) | Needs a genuine 3D triangulation of the 8 speakers — Stage-2 work, not a toggle | v1.1+ |
| Binaural / stereo fold-down | Output bus is fixed at 8 channels; fold-down must write into 2 of 8 and mute 6. True binaural needs HRTF data | v1.1+ |
| Quadraphonic (4ch) container variant | Doubles the bus-layout and channel-map test matrix — the exact area where silent channel-order bugs live | v1.1+ |
| Internal diffuse reverb driven by hull distance | Cannot be an external send (`aufx` gets one bus); an 8-out diffuse network is a significant DSP add | v1.1+ |
| Motion engine (tempo-synced paths, gesture record/playback) | v1.0 is host-automation only, deliberately | v1.1+ |
| Multiple simultaneous sources per instance | v1.0 is one puck plus stereo width | v1.1+ |

---
*Generated from BRIEF.md on 2026-08-10*
*Schema: .planning/workflow/schemas/plugin-requirements.schema.json*
