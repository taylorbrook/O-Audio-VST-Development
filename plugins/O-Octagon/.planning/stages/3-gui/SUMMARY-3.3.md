# Stage 3 — GUI · Phase 3.3 (Scenes, meters, gradient, elevation) — Summary

**Plugin:** O-Octagon
**Stage:** 3 of 4 — GUI · **Phase 3.3 of 3 — the last phase of Stage 3**
**GSD phase:** execute
**Date:** 2026-08-12
**Branch:** `feat/o-octagon`
**Closes:** `FUNC-06`, `UI-03`, `UI-04`, `UI-05` — four rows, **18 criteria, zero partials**

---

## Result

**Stage 3 is complete.** Nine requirement rows closed across 3.1 / 3.2 / 3.3 with no partials:
`UI-02` (3.1), `FUNC-02` / `FUNC-04` / `FUNC-05` / `UI-01` (3.2), and these four.

| Measure | Plan target | Delivered |
|---|---|---|
| C++ probes | 92, 0 failures | **92 (44 unit + 48 harness), 0 failures** |
| JS gate sections | 69 (42 + 27) | **69 (42 + 27), both green, layout did NOT skip** |
| Native-function surface | 18 | **18**, grep-diffed three ways in both directions |
| Negative controls | 8, declared at plan | **8 fired, tree byte-identical afterwards** |
| Contract checksums | 3 unmoved + `ROADMAP` at its new pin | **all four correct** |
| Build | 3 formats + 2 test targets, forced full recompile | **zero `warning:` / `error:` / `FAILED`** |

**Gate 13 — the human Standalone launch, including Q5's hidden-editor test — HAS NOT RUN.** It is
the one gate no automation in this repo can stand in for, and it is stated here as plainly as the
gates that did run. See *What did not run*.

---

## What was built

**Four new page modules**, all four picked up automatically by P51's derived registry — the seventh
time that enumeration hole would have bitten and the first time closing it cost nothing:

- `js/scenes.js` — ten controls, hover **and** keyboard preview, `STORE` arm/auto-disarm
- `js/meters.js` — 30 Hz poll, per-frame ballistics, deadline-released guard
- `js/field.js` — `atob` → `putImageData` → `drawImage`, offscreen 32 × 40
- `js/elevation.js` — the section strip, three construction rules

**Three new C++ TUs**, two of which joined the **fast unit target** without widening its link line
(verified: `juce_audio_basics + juce_core + juce_data_structures`, no `juce_dsp`, no
`juce_gui_extra`, no `juce_audio_processors`):

- `Source/Data/SceneModel.{h,cpp}` — D16's predicate as a pure function, plus the `SCENES` store
- `Source/DSP/FieldSampler.{h,cpp}` — the full chain, sampled on the message thread
- `dbap::solve` gained `float* outInvK = nullptr` — the P54 precedent, **no call site touched**

**Eighteen native functions** (13 → 18): `getMeters`, `getScenes`, `applyScene`, `storeScene`,
`getFieldGrid`. UI-05 needed none — `getVenueGeometry` already carried per-speaker `z`, both rake
values, the bbox and the centroid from 3.2's P55, and named-scene membership rides that same payload.

---

## The two probes that carry the phase

Both were named at plan as the only assertions a plausible-looking wrong implementation fails, and
both were driven by a negative control to prove it rather than argued.

**CG — the permutation.** Rotate the eight indices against the same eight physical positions; `FRONT`
must return the indices that *now* hold `y < cy`. All seven rotations track the geometry, and the
probe reports its own guard strength: **a fixed-index implementation would pass 0/7**. **NC3**
injected exactly that defect — CG fired on all seven rotations while **CF and CH still passed**.

**CM — post-map meters on a non-identity map.** The probe asserts the permutation *first*, because
all three accepted 8-channel containers have initializer order == enum-bit order and the shipped
default map **is** the identity. Ping stepping 1 → 8: **wrongIndex 0, leaked 0, silent 0**. **NC4**
made the meters read identity-indexed — which is what a bypassed map *is* — and CM fired with
**wrongIndex 8** while probe CN still saw 8/8 meters register.

---

## The five findings the plan said must not be lost

1. **N9 — the dropped completion.** `refreshGeometry` is **repaired**, and the defect was live in
   shipped 3.2 code. Every in-flight guard on the page now carries a timestamp and releases on a
   **deadline**; frontend §33 asserts the shape for all three, **NC5** fired it, and `pollStatus`
   still has **no guard at all** — P71 rule 3, because it is the one poll that self-heals.
2. **N10 — `max_i v_i²` is degenerate.** The field is `1/k = √denom`. Probe **CA** confirms `outInvK`
   equals an independent √denom to **6e-8** with the eight gains **bit-identical** to the nullptr
   path; **NC6** restored the disqualified formula and CB fired at |Δ| **0.246**.
3. **N11 — D25's premise was over-attributed.** §21 asserts the fitted box against its stage *and*
   the coarse column check; **§22 asserts the ordering fact** the coarse one depends on. **NC1** gave
   the asymmetry (§21 fires at 243≤123 while document §8 passes at 720≤720) and **NC8** proved §22
   catches what would otherwise go silently vacuous.
4. **N12 — the field has five inputs.** Probe **CD** drives all five individually (**5/5 live**) and
   the recompute is coalesced to at most one per 2 Hz tick; layout **§27** counts **24 frames of
   puck drag → 12 → 12 invocations**, with `blur` as the positive control at 12 → 13.
5. **N13 — `SCENES` rides `copyState()` but needs normalisation.** Probe **CK** drives a **pre-3.3
   session explicitly** — the `SCENES` element stripped from a real serialised state — and the
   restore yields **0 ghost slots**, the node **written back**, and `VENUE` intact.

---

## Measured layout (the 278 px budget)

Measured on the rendered page, not computed. The plan's own premise correction held: 3.3 touched the
**ROOM controls column**, not the venue rail, and Gate 16 re-ran the venue study to prove it.

| | Measured |
|---|---|
| Controls column | **586 × 592**, four 3.1/3.2 groups ending at y = 386 |
| Scenes group | **76 px** (plan modelled 82) |
| Elevation group | **173 px** (plan modelled 172) |
| Elevation strip | **552 × 123** inside a 552 × 123 stage |
| Scene buttons | **48.2 px**, widest label ink 33.1 px, `scrollWidth == clientWidth` on all ten |
| Bottom edge | groups end at **y = 659** against a column bottom of 664 — **5 px spare** |

Two components + two 12 px gaps = **273 px of the 278 px budget**. No deviation to report.

---

## Deviations, each with its reason

1. **Task order: 6–15 ran BEFORE 1–5.** The plan numbered the C++ first but Gate 4 requires the stub
   render to precede *any* 3.3 C++; both could not hold. The gate won. **Gate 4's stamp is
   `2026-08-12T22:47:48.514Z`, machine-emitted by §0 (P84), recorded with `FieldSampler`/`SceneModel`
   absent from disk and `outInvK` absent from `DbapSolver.h`.** This is the first phase in the
   project whose ordering claim is evidence rather than transcription.
2. **`applySceneWeights` lives on the PROCESSOR, not at the editor's call site (P78).** P78's actual
   argument is C++-vs-JS — eight `SliderState` writes would scatter D18's obligation across 24 bridge
   messages — and that is fully honoured by one C++ function with one call site. **PLAN-3.3's own
   probe table makes CI a HARNESS probe**, and the harness never compiles `PluginEditor.cpp`; on the
   editor the brackets could only ever have been grepped. They are now **measured** through real
   parameter listeners.
3. **`FieldSampler.cpp` includes `ConvexHull2D.h` and `VenueGeometry.h`** beyond Task 2's named list.
   Both are inside the chain the plan itself specifies (the hull projection; the audience plane), both
   were already in the target, and **neither widens the link line** — Gate 11 re-verified.
4. **`SceneModel.cpp` joined the render-harness target.** `PluginProcessor.cpp` owns the store, so it
   is a link requirement, not an optional dependency — found by the harness failing to link.
   `FieldSampler.cpp` is deliberately **not** there.
5. **NC4 was injected in its executable form.** The literal "meter `v_i`" does not compile in the
   shipping build — `GainStage::currentSmoothedValues()` exists only under `OOCTAGON_INSTRUMENT`,
   verified by the compiler error. Identity indexing is the same defect class and is the one the
   criterion names ("would light correctly under a bypassed channel map"), because a bypassed map
   *is* identity indexing.
6. **CB's twenty points are 14 strided + 6 outside-hull.** The bare stride **failed CB's own
   non-vacuity guard**: 0 of 20 cells landed outside the §OQ4 hexagonal hull, so `hullTrimGain` was
   multiplying by bit-exact unity everywhere and half the chain was untested. Same twenty points,
   both arms covered.

---

## Gate-caught defects, and one gate repaired

- **CB failed first**, on its own non-vacuity clause rather than its arithmetic (which agreed to
  4e-8). Recorded because it is the clause working as designed.
- **Frontend §18 had a latent bug this phase exposed.** Its per-class regex was unanchored, so the
  new `.elev-readouts .cell-value { … }` rule was harvested *instead of* `.cell-value`'s own — the
  gate reported the mono stack and tabular-nums missing from a stylesheet that has both. **The
  selector is now anchored at a rule boundary**; a descendant selector ending in the class is no
  longer mistaken for the class's rule. This would have bitten any future phase adding a scoped
  override.
- **Two visual defects were found by looking at the rendered page**, not by a gate:
  the field gradient at alpha 30..180 washed out the hull polygon and the rig extent (now 0..96, so
  the panel shows through where the field has nothing to say), and the elevation strip's four
  mirrored speaker pairs stacked their numerals illegibly (the **dots stay exact** — moving one would
  make the strip lie about a depth — and only the **label** steps aside; it now reads 1 2 / 3 8 /
  4 7 / 5 6).

---

## Gate results

| # | Gate | Result |
|---|---|---|
| 1 | Clean 3-format build + both test targets, **forced full recompile** | ✅ exit 0, **0 warnings / errors / FAILED** across 141 build steps |
| 2 | `ui_frontend_check.js` | ✅ **42 sections**, all pass |
| 3 | `ui_layout_check.js` | ✅ **27 sections**, all pass, **did not SKIP** |
| 4 | Stub render before any 3.3 C++ | ✅ **`2026-08-12T22:47:48.514Z`**, machine-emitted (P84) |
| 5 | `auval -v aufx OuOc OuDv` | ✅ **AU VALIDATION SUCCEEDED** |
| 6 | pluginval s10, VST3 ×3 / AU ×3 | ✅ **all six exit 0, zero `FAILED`** |
| 7 | Both C++ test targets | ✅ **92 probes, 0 failures** (44 + 48) |
| 8 | `gen_dbap_reference.py --check` | ✅ **102 cases OK** — re-run because Task 1 touched the solver |
| 9 | 17 params vs `parameter-spec.md`, three sides | ✅ **17/17**; 3.3 adds none |
| 10 | `createEditor` guard; editor absent from harness target | ✅ both |
| 11 | Unit-target link line | ✅ **no `juce_dsp`, no `juce_gui_extra`** — re-checked with both new TUs joined |
| 12 | Contract checksums | ✅ three unmoved, `ROADMAP.md` at its **new** pin `643471ba…3b4383d8` |
| 13 | **Standalone launch, macOS — HUMAN, ~15 min** | ⛔ **NOT RUN** |
| 14 | The eight negative controls | ✅ **all eight fired**, tree byte-identical |
| 15 | `room_layout_study.js` | ✅ re-run, agrees with layout §19/§21 within the model/implementation delta (2 px on the strip, the stage border) |
| 16 | `venue_layout_study.js` | ✅ re-run — the venue rail is unchanged, which is what proves 3.3 did not touch it |

---

## What did not run

**Stated as plainly as what did.**

- **Gate 13 in full.** A human Standalone launch: the three new screens at 1100 × 720, a scene
  previewing on hover *and* keyboard focus and committing, the meters following a ping 1 → 8, and
  the strip's rear moving when `rakeRear` does. Every one of those has automated evidence against a
  **Chromium** render and against the **C++**; none has been seen in **WKWebView**.
- **Q5's half, and it has now been unrun by three consecutive phases.** A real 30 Hz poll against a
  **hidden WKWebView** — hide the editor for 10 s with the meter poll live, re-show, confirm the
  meters resume. The JS half is measured (N9), the JUCE drop is read from source, and the two
  together have never been executed. `js/meters.js` exposes a `dropped` counter specifically so this
  test has something to read.
- **Locks and file I/O in `processBlock`** remain grep + inspection. Only *allocation* is measured;
  `-fsanitize=realtime` is unsupported by Apple clang 17.0.0. Unchanged since 2.2 and restated
  because probe CN is easy to mistake for an RT-safety pass.
- **Windows.** MSVC habits are authored (§20/§31 green); no Windows compiler has seen this code.
- **CI wiring.** Stage 4. **This is the third phase to widen that gap** — the test targets are not in
  `build-and-release.yml` and a JUCE bump performed without running them still ships silently.

### One residual, unattributed

Probe **AO** reported **1 allocation, exactly once in 35 runs** — on the first execution after a full
clean rebuild. All 34 other runs read 0: 20 quiet, 12 under 8-way CPU load, and 2 after a **second**
clean rebuild that deliberately reproduced the original condition. **It did not reproduce.**

Attribution to 3.3 is **not established and not ruled out**. `buffer.getMagnitude()` resolves to
`FloatVectorOperations::findMinAndMax` on a raw pointer and allocates nothing; probe **CN** — the same
measurement with metering explicitly live and a non-vacuity clause proving the loop ran — read **0 in
that same run**. Recorded here rather than absorbed, because a PERF-01 gate that failed once and
cannot be reproduced is exactly the thing a summary should not round down.

---

## Next Phase

**Ready for:** `verify`

Every gate is **re-run from scratch at verify**, never read out of this file. That discipline caught
four mis-attributions at 2.3 and six more across 3.1 / 3.2, and this summary contains at least three
claims worth re-deriving: the 92-probe count, Gate 4's stamp against the actual C++ mtimes, and the
AO residual's run count.
