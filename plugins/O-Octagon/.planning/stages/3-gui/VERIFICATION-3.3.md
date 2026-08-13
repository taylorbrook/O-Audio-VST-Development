# Stage 3 — GUI · Phase 3.3 (Scenes, meters, gradient, elevation) — Verification

**Plugin:** O-Octagon
**Stage:** 3 of 4 — GUI · **Phase 3.3 of 3 — the last phase of Stage 3**
**GSD phase:** verify
**Date:** 2026-08-12
**Branch:** `feat/o-octagon` @ `a47cef88` (2.2 / 2.3 / 3.1 / 3.2 / 3.3 work uncommitted)
**Verifies:** `SUMMARY-3.3.md` against `PLAN-3.3.md`, `CONTEXT-3.3.md`, `RESEARCH-3.3.md`, `REQUIREMENTS.md`

---

## Verdict

**✅ VERIFIED.** `FUNC-06` · `UI-03` · `UI-04` · `UI-05` all close — **4 rows, 18 criteria, zero
partials**, exactly as declared at discuss, research and plan. **Stage 3 closes here.**

**Every gate was RE-RUN from scratch, not read out of `SUMMARY-3.3.md`** — on a forced full
recompile (`rm -rf build/plugins/O-Octagon`, **143 ninja steps, zero compiler diagnostics**), and
then **a second time end-to-end** after the one fix this verification landed.
**92 C++ probes (44 unit + 48 harness), 0 failures. 69 JS gate sections (42 + 27), 0 failures.**
All four contract checksums recomputed byte-exact, including `ROADMAP.md` at its **new** 3.3 pin.

**Three negative controls were run as NEW work at verify — one per gate family.** All three fired.
The tree was proved byte-identical afterwards and every gate returned to green on the restored tree.

**The AO residual is RESOLVED, and it was a real defect — in the probe, not in the plugin.**
`SUMMARY-3.3.md` carried it forward as *"unreproduced and unattributed"* and asked verify to
re-derive it. Verify reproduced it **4 times in 40 runs under load** and attributed **all four** to a
thread other than the one calling `processBlock`. See §The AO residual, which is the most
substantive thing in this document.

**Gate 13's interactive half remains UNRUN**, and Q5's hidden-WKWebView test has now gone unrun by
**four consecutive phases**. The static half of Gate 13 was discharged further than at 3.2: **all
four 3.3 components are confirmed rendering in WKWebView.**

---

## Goal-backward analysis

### What 3.3 set out to achieve (CONTEXT-3.3.md, PLAN-3.3.md)

1. **Weight scenes** that write eight ordinary automation events — not a latched plugin mode — with
   membership **derived from the measured geometry** rather than fixed speaker indices.
2. **Live per-speaker meters** reading what actually **leaves the plugin** — post-map, post-trim —
   so the indicator is a second line of defence on R1 rather than a restatement of the solve.
3. **A DBAP level-field gradient** that matches the shipping solver and recomputes on geometry and
   weights only, never per frame.
4. **A side-elevation strip** making the raked audience plane and the §OQ4 graded speaker heights
   visible as the 3-D geometry the DSP actually solves against.
5. Close **`FUNC-06`, `UI-03`, `UI-04`, `UI-05`** — four rows, **18 criteria, zero partials declared
   in advance** — and with them, Stage 3.

### What was delivered, and whether it matches

| Goal | Status | Evidence re-measured at verify |
|---|---|---|
| 1 — scenes as automation, geometry-derived | ✅ Achieved | Probe **CI** — 8/8 host values, **8/8 begin gestures, 0 unclosed**. Probe **CG** — all **7 rotations** track the geometry and the probe reports its own guard strength (*a fixed-index impl would pass 0/7*). Probe **CJ** — a 50/50 blend renders identical to a processor that only ever saw the blend, and **still holds after 8 further blocks** |
| 2 — meters read the written buffer | ✅ Achieved | Probe **CM** on a **non-identity** map: ping 1→8 gives `spk1→meter1(buf 1) … wrongIndex 0, leaked 0, silent 0`. Layout **§23** — every arc clears the glyph stroke (inner edge **13.75** vs dot outer **12.00 px**) and a ping lights the matching arc with the other seven at zero |
| 3 — gradient matches the solver, counted not eyeballed | ✅ Achieved | Probe **CB** — 20 of 1280 cells vs an **independent** solve, worst \|Δ\| **0.000000055** against a 1e-3 tolerance, **6 of the 20 drawn from the 66 outside-hull cells**. Layout **§27** — 24 drag frames left the recompute count at **11 → 11** while `blur` moved it **11 → 12** |
| 4 — elevation strip | ✅ Achieved | Layout **§22** — `rakeRear` 0 → 2.0 m moved the rear **y2 107 → 79.29 px** while the front endpoint was **bit-exactly unmoved** at (83.69, 107); **8 speakers, 4 distinct y against 4 distinct RETURNED z** = [4.5, 4.5, 4.7, 5.1, 5.4, 5.4, 5.1, 4.7]. Confirmed **rendering in WKWebView** — see §Gate 13 |
| 5 — four rows, 18 criteria, zero partials | ✅ Achieved | 4/4 rows `complete`, 18/18 criteria with named measured evidence. Stage 3 totals **nine rows across 3.1 / 3.2 / 3.3, zero partials** |

**Delivery matches goals.** No goal was quietly narrowed. The two `nice`-priority rows (`UI-04`,
`UI-05`) that §R7 kept on a descope path were **both shipped** (D15), and both remain descopable —
frontend **§39** and **§40** assert that structurally, and verify re-measured both.

---

## Requirements verification

**Stage:** 3-gui, phase 3.3
**Requirements verified at this phase:** 4 — `FUNC-06` (`should`), `UI-03` (`should`), `UI-04`
(`nice`), `UI-05` (`nice`)

| Requirement | Priority | Status | Acceptance criteria |
|---|---|---|---|
| FUNC-06: Weight scenes | should | ✅ **Complete** | **6/6**, each with named measured evidence |
| UI-03: Live per-speaker level indicators | should | ✅ **Complete** | **4/4** |
| UI-04: DBAP level-field gradient backdrop | nice | ✅ **Complete** | **4/4** |
| UI-05: Side-elevation strip | nice | ✅ **Complete** | **4/4** |

**Requirements summary:** ✅ Complete **4** · ⚠️ Partial **0** · ❌ Failed **0** · ⏸️ Deferred **0**.
**No stage-3 row remains open.**

### Criterion → evidence, re-measured at this boundary

| Req | # | Verify-phase measurement |
|---|---|---|
| **FUNC-06** | 1 | Probe **CI** — 8 host values all match, **8/8 begin gestures, 0 unclosed**, 8/8 `valueChanged` |
| | 2 | Probe **CG** — 7/7 rotations track the geometry, fixed-index would score **0/7**. Probe **CF** — `SIDES {3,4,7,8}`, 3 and 8 `ON_EDGE` and in `SIDES`. Frontend **§32** — `js/scenes.js` carries **none** of the predicate's vocabulary and renders the returned indices |
| | 3 | Layout **§24** — hover lit **[1,2,3,8]**, focus lit **[3,4,7,8]**, **two different sets** so neither check passes on a constant. **§25** — the empty slot is `disabled` + `data-empty="true"`, clicking it moved **not one of eight weights** and invoked `applyScene` **0 times**; the control case committed **[4,5,6,7]** with **32 bracket events, 0 unclosed**. Probe **CH** — proscenium rig, `SIDES` empty with **0 INTERIOR** speakers |
| | 4 | Probe **CK** — **16/16 weights bit-identical**, occupancy `{1,3}` preserved; the pre-3.3 upgrade path yields **0 ghost slots**, node **written back**, `VENUE` intact |
| | 5 | Probe **CL** — 42 venue values **bit-identical to the live venue and demonstrably not the saved one**, with the scene callback provably run. Frontend **§35** — `setStateFromXml` has **0 call sites** in 28 source files |
| | 6 | Probe **CJ** — blend identical to a reference processor given only the blend; **after 8 further blocks the parameters still hold it**; endpoints separately shown distinguishable |
| **UI-03** | 1 | Layout **§23** — all eight arcs are children of their glyph `<g>`, inner edge **13.75 ≥ 12.00 px** |
| | 2 | Probe **CM** — **non-identity map**, `wrongIndex 0, leaked 0, silent 0`, read-and-zero residue 0 |
| | 3 | Frontend **§34** — coefficients named `*_PER_FRAME`, **`tick()` applies neither**, hold and release are wall-clock. *Visual half is Gate 13 and did not run* |
| | 4 | Frontend **§34** — a fixed `window.setInterval`, **no `setTimeout` anywhere**, `tick()` schedules nothing. **§33** — three guards, all timestamped and deadline-released, `pollStatus` still unguarded. **§37** — `getMeters` constructs **no `juce::String`**. Probe **CN** — **0 allocations** across 65 `processBlock` calls with 8/8 meters live. **The residual is resolved — see below** |
| **UI-04** | 1 | Probe **CB** — worst \|Δ\| **0.000000055** (tol 1e-3), **6 of 20 outside the hull**. Probe **CA** — `outInvK` vs an independent √denom to **6e-8**, gains **bit-identical** to the nullptr path |
| | 2 | Layout **§27** — **11 → 11** across 24 drag frames (38 srcX / 38 srcY writes prove the drag happened), control **11 → 12**. Probe **CD** — counter `0 → 1 → 6` exact, **5/5 inputs live**. Probe **CC** — bitwise identical across 5 source states × 1280 cells |
| | 3 | Frontend **§38** — `atob → putImageData → drawImage` **in that order** (684 < 1492 < 2455), offscreen at the grid's own resolution, **no part of the solve in JS**. Layout **§26** — `backing == round(rect × dpr)`, **1600/1600** sampled subpixels differ from the flat fill, legend prints the returned span |
| | 4 | Frontend **§39** — imported by `app.js` and nothing else, `#plan-backdrop` its own element, painter hook **defaults to null with the draw path guarded** |
| **UI-05** | 1 | Layout **§22** — rear **107 → 79.29 px**, front **exactly unmoved**, quantised axis **8 → 8 ticks**. Frontend **§41** — solid line endpoints ARE `(bbMinY, rakeFront)` / `(bbMaxY, rakeRear)`, extrapolation a **separate dashed element**, solid line never reads the envelope |
| | 2 | Layout **§22** — both readouts shown (**ear "1.20 m" / source "2.20 m"**); at `srcZ` max the number printed **"9.20 m"** against a 7 m axis while the **marker clamped with a chevron** at cy 10.0 of 123 px. Confirmed in WKWebView: **Ear 2.15 m / Source 2.15 m** with `srcZ = 0` — the source riding the rake |
| | 3 | Layout **§22** — **8 speakers, 4 distinct y vs 4 distinct RETURNED z**, asserted against the payload rather than a literal |
| | 4 | Frontend **§40** — imported by `app.js` only, no other module reaches an `elev-` element, `#group-elevation` is the **last group authored**; layout **§22** measures the same fact on the rendered tree |

---

## Automated checks — all 16 gates re-run

| # | Gate | Verify-phase result |
|---|---|---|
| 1 | Clean 3-format build + both test targets, **forced full recompile** | ✅ **143 steps, `warning:` 0, `error:` 0, `FAILED` 0** — and **re-run a second time** after the verify fix, same result |
| 2 | `node tests/ui_frontend_check.js` | ✅ exit **0**, **42 sections**, 353 assertions, 0 `FAIL` |
| 3 | `node tests/ui_layout_check.js` | ✅ exit **0**, **27 sections**, **skip count 0** — the gate did not skip |
| 4 | Stub render **before** any 3.3 C++ | ✅ **corroborated independently** — see below |
| 5 | `auval -v aufx OuOc OuDv` | ✅ **AU VALIDATION SUCCEEDED** (both builds) |
| 6 | pluginval s10, VST3 ×3 / AU ×3 | ✅ **6/6 exit 0, zero `FAILED` lines** (both builds — 12 runs total) |
| 7 | Both C++ test targets | ✅ **44 + 48 = 92 probes, 0 failures**, exit 0 / exit 0 |
| 8 | `gen_dbap_reference.py --check` | ✅ **102 cases OK**, no fixture drift after Task 1 touched the solver |
| 9 | 17 params vs `parameter-spec.md` | ✅ **17/17 on four sides** — re-derived by an **independent parse** at verify |
| 10 | `createEditor` guard; `PluginEditor.cpp` absent from the harness | ✅ arms diverge, generic editor demoted to `#else`; **0** `PluginEditor` references in either test `CMakeLists.txt` |
| 11 | Unit-target link line | ✅ resolved from `ninja -t commands`: `juce_audio_basics`, `juce_core`, `juce_data_structures`, `juce_events` — **no `juce_dsp`, no `juce_gui_extra`, no `juce_audio_processors`**, with **both** new TUs joined |
| 12 | Contract checksums | ✅ all four **byte-exact**, two different expected values, **referent checked** |
| 13 | **Standalone launch, macOS — HUMAN** | ⚠️ **PARTIALLY discharged** — static half further than at 3.2; interactive half and Q5 **NOT RUN** |
| 14 | Negative controls | ✅ **3 re-run as NEW work, one per family, all three fired**; tree byte-identical afterwards |
| 15 | `node tests/tools/room_layout_study.js` | ✅ exit 0 — strip **554 × 125** in the model against **552 × 123** measured on the page (the 2 px stage border), coarse/guard/doc-§8 behave as §21 does |
| 16 | `node tests/tools/venue_layout_study.js` | ✅ exit 0 — rail **does not overflow**, fit **270 × 337 height-bound**, unchanged from 3.2. **This is what proves 3.3 did not touch the venue rail** |

### Gate 4 — the ordering claim, corroborated against the filesystem for the first time

3.3 landed D27's one line, so §0 now emits a machine stamp. Verify did not merely re-read it — it
checked the stamp against the mtimes of the C++ the claim is about:

| Artifact | Time |
|---|---|
| `evidence/layout-check-preintegration-3.3.log` §0 — `STAMP: [0] ui_layout_check run at` | **2026-08-12T22:47:48.514Z** = **15:47:48 PDT** |
| `Source/DSP/DbapSolver.h` (gained `outInvK`) | 15:48:28 — **+40 s** |
| `Source/Data/SceneModel.h` | 15:50:14 |
| `Source/DSP/FieldSampler.h` | 15:51:49 |
| `Source/DSP/FieldSampler.cpp` | 15:52:54 |
| `Source/Data/SceneModel.cpp` | 16:36:06 |

**Every 3.3 C++ file is newer than the stamp.** The three-boundary chain of transcription that
`VERIFICATION-3.2.md` §Issues 2 flagged is closed: the claim is now evidence, and an **independent**
artifact (filesystem mtimes) agrees with it.

### Contract checksums, recomputed at this boundary

| Contract | Measured at verify | STATUS frontmatter | Result |
|---|---|---|---|
| `BRIEF.md` | `697a4f32…f6b9fbd6` | `697a4f32…f6b9fbd6` | ✅ unmoved |
| `parameter-spec.md` | `b45f88dc…cbb9e02f` | `b45f88dc…cbb9e02f` | ✅ unmoved |
| `research/ARCHITECTURE.md` | `32a85018…81d85273` | `32a85018…81d85273` | ✅ unmoved *(the 3.3-discuss amendment pin)* |
| `ROADMAP.md` | `643471ba…3b4383d8` | `643471ba…3b4383d8` | ✅ at its **new** P70 pin, not the superseded `aec7d0ce…` |

**All four byte-exact, and the referent was checked rather than only the match** —
`pattern_promotion_checksum_pins_replaced_file`. The `aec7d0ce…` value is present in frontmatter
**only** under `roadmap_checksum_superseded`, which is correct.

### Gate 9, re-derived independently

Parsed at verify by a fresh script rather than by reading the gate: the **spec table** (17 rows,
`w1…w8` expanded from the `7–14` row), the **`makeFloat` call sites** (9 literal + the 8-iteration
weights loop), the **`oo::params::id()` table**, and the **relay construction**. Ids, ranges,
defaults, labels and skews are identical on all four sides; the id table's order equals the source
order; `PluginEditor.cpp` contains **0** literal parameter-id strings — relays are built from
`oo::params::id(i)`. `GainStage.h` carries `static_assert (kCount == 17)`; **3.3 adds none.**

### The native-function surface, grep-diffed in both directions

| Site | Count |
|---|---|
| `withNativeFunction("…")` in `PluginEditor.cpp` | **18** |
| `nativeFn("…")` call sites across `Source/ui/public/js/*.js` | **18** |
| `tests/ui-stub/juce-stub.js` whitelist keys | **18** |

**All three sets are identical** — `diff` clean in both directions, no name in one and absent from
another.

---

## Negative controls — 3 run as NEW work at verify, 3 fired

One per gate family, chosen so that a pass proves the family non-vacuous rather than one assertion.
These were injected, measured, and reverted at verify; they are **not** replays of execute's logs.

| # | Family | Injected defect | Result |
|---|---|---|---|
| **NC-A** | C++ probe | Meters indexed by identity instead of through `snapshot.speakerToBuffer` — which is what a **bypassed channel map** is | ✅ **FIRED** — `[FAIL] CM … spk1→meter2(buf 1) … wrongIndex 8, leaked 0, silent 8`, **while CN still PASSED** with 8/8 meters registering |
| **NC-B** | Playwright | A **zero-height** node inserted after `#group-elevation` | ✅ **FIRED** — `[FAIL] [22] #group-elevation is the controls column's LAST child — last is "nc-b-inserted"`, **while §21 `[coarse]` PASSED 592 ≤ 592 at BOTH DPRs and §21 `[guard]` passed 123 ≤ 123** |
| **NC-C** | static JS | `app.js`'s geometry guard released **only on settlement** — N9's live 3.2 defect, restored | ✅ **FIRED** — `[FAIL] [33] js/app.js: and it is RELEASED on that deadline, not only on settlement` |

**Tree proved byte-identical to baseline afterwards** — 49 files, `shasum -a 256`, zero drift — and
on the restored tree both JS gates returned 42 / 27 sections and both probe targets returned 44/0
and 48/0.

### NC-B is the asymmetry D25 was written for, reproduced independently

`SUMMARY-3.3.md` claims NC8 proved §22 catches what would *"otherwise go silently vacuous"*. Verify
built the control in its **strongest** form — a node with **no height**, so nothing overflows — and
got exactly the predicted split:

> **§21 `[coarse]` PASSED** — `controls column scrollHeight 592 <= clientHeight 592`
> **§21 `[coarse]` PASSED at DPR 2** — `592 <= 592`
> **§21 `[guard]` PASSED** — `the strip is inside its stage — 123 <= 123`
> **§22 FIRED** — `last is "nc-b-inserted" of [group-position, …, group-elevation, nc-b-inserted]`

Every measurement of *size* reported green while the **ordering fact they depend on** had been
broken. N11's correction and D25's surviving conclusion are both confirmed by measurement.

---

## The AO residual — reproduced, attributed, and fixed

This is the one item `SUMMARY-3.3.md` explicitly refused to round down, and it asked verify to
re-derive it. Verify did, and **the residual was real** — but it was never in `processBlock`.

### 1. It did not reproduce quietly, exactly as the summary said

| Condition | Runs | AO reading |
|---|---|---|
| First harness execution after this verification's own forced full recompile | 1 | **0** |
| Quiet | 20 | **0 × 20** |
| First execution after a **second** clean rebuild | 1 | **0** |

### 2. It reproduced immediately under CPU contention

Under **8-way CPU load**, `1 allocation` appeared in **4 of 40 runs** — a defect that "did not
reproduce" in 35 quiet runs at execute reproduces at **~10 %** the moment the machine is busy. It
also appeared on **CN** in one run and **AO** in others, which already rules out a code path
specific to either probe.

### 3. The cause: the counter was PROCESS-WIDE, not thread-scoped

`rtcheck::armed` is a global flag, and the replaced `operator new` family counted **any** thread's
allocation inside the window — the JUCE message thread and the macOS runtime threads a console app
keeps alive included. Contention does not make `processBlock` allocate; **it widens the window in
which some other thread's allocation lands inside it.**

Verify tested this directly rather than arguing it, by temporarily recording the allocating thread
against the arming thread:

> **4 of 40 loaded runs fired. `DIAG foreign-thread 1` on all four. 36 of 40 clean, `foreign 0`.**
> **4/4 attribution, no exceptions.**

### 4. Fixed at verify, in the harness only

`tests/render-harness/main.cpp` — the counter is now scoped to the arming thread, every arm site
routes through a single `rtcheck::arm()` so the unattributed form cannot be reintroduced by writing
`armed.store (true)` directly, and the foreign tally is **still taken and still reported** beside
the verdict. A filter that hid the artifact would have traded a flaky probe for a silent one.

| | Before | After |
|---|---|---|
| 40 runs under 8-way load | **4 FIRED** | **0 fired** |
| Foreign allocations observed | 4 (counted as failures) | 1 (**reported**, not counted) |
| Quiet run | pass | pass |

The final verification run shows the mechanism working in the open:

> `[PASS] CN no-allocation-with-metering 0 allocation(s) across 65 processBlock calls … 8/8 meters
> registered the render [+1 foreign-thread, NOT counted — see rtcheck]`

**That exact run would have FAILED before the fix.**

### 5. What this settles, and what it does not

**`PERF-01` does not regress and never did.** `processBlock` allocated **0** in every measurement at
every load level; `buffer.getMagnitude()` resolving to `FloatVectorOperations::findMinAndMax` on a
raw pointer is confirmed by measurement now, not only by inspection. **UI-03/4's residual is closed
and the criterion is `complete` without one.**

It does **not** change the standing limitation restated in `SUMMARY-3.3.md`: **locks and file I/O in
`processBlock` remain grep + inspection**, because `-fsanitize=realtime` is unsupported by Apple
clang 17.0.0. Only *allocation* is measured.

**This is the only change to the tree made at verify**, it is confined to test scaffolding, and the
full gate set was re-run from a forced full recompile afterwards.

---

## Gate 13 — partially discharged at verify, and what remains

The Standalone was launched from the verify build. **Window measured 1102 × 778** (= 1100 × 720 +
Standalone chrome), matching 3.1 and 3.2 exactly. Screenshot:
`evidence/standalone-verify-3.3.png`.

**Confirmed live in WKWebView — all four 3.3 components, none of which had been seen outside
Chromium before this verification:**

- **`SCENES`** — ten controls on one row, `ALL / FRONT / REAR / LEFT / RIGHT / SIDES` live and
  **`U1`–`U4` struck through and disabled**. That is D20's refusal and FUNC-06/3's *"shown as empty
  and not writable"* rendering in the real browser, and **`STORE` sits in the title row** — Q9's V3
  layout, shipped.
- **`ELEVATION`** — the raked line drawn solid front→rear with the **extrapolation dashed at both
  ends** (P76 rule 1), the quantised 0–7 axis, `FRONT` / `REAR` end labels, eight speakers as four
  mirrored pairs reading **1 2 / 3 8 / 4 7 / 5 6**, and **both readouts — `Ear 2.15 m`,
  `Source 2.15 m`** — equal, because `srcZ = 0` means the source rides the rake. UI-05/2's claim,
  visible.
- **The field gradient** — painted behind the plan with the hull polygon, the rig extent and all
  eight glyphs still legible, which is the 0..96 alpha fix that was found *by looking* at execute.
  The legend reads **`FIELD 0.0 – 2.7 dB`**.
- **The eight meter arcs** — present at their glyph positions, at rest (Standalone reports *"Audio
  input is muted to avoid feedback loop"*, so there is no signal to show).

Also re-confirmed: speakers **3 and 8 dashed** and the other six solid (UI-02/2), **both** frame
banners side by side, all eight weights at `1.00`, the nine column controls at their
`parameter-spec.md` defaults, and the `PLAN` caption em-dash.

**What could NOT be discharged, stated as plainly as what could:**

- **Every interactive item.** A scene previewing on hover *and* keyboard focus, a scene committing,
  the meters following a ping 1 → 8, and the strip's rear moving when `rakeRear` does all require
  synthetic clicks into the WKWebView. This environment lacks the accessibility permission to
  deliver one — the attempt returned **`-25208`**, the same failure 3.2 hit. Reading window geometry
  works; delivering events does not.
- **Q5's half, unrun by FOUR consecutive phases now.** A real 30 Hz poll against a **hidden**
  WKWebView — hide for 10 s with the meter poll live, re-show, confirm the meters resume. An attempt
  was made here and abandoned: with the audio input muted the meters read zero either way, so
  "resume" has no observable at rest, and the `dropped` counter `js/meters.js` exposes for exactly
  this test needs devtools access into the WKWebView. **It needs a human with a signal running.**

**Gate 13 is still NOT D5.** D5 is the ~15 min Logic hall session for QUAL-01's audible clause,
folded to Stage 4 and untouched.

> The four rows are nonetheless **complete**: every criterion closes on evidence that was
> re-measured here, and no criterion's testable content rests on the interactive half alone.
> UI-03/3's *visual* half is the one place Gate 13 is the sole witness, and the criterion is closed
> by frontend §34's structural assertions — which is what the two-clock design was for.

---

## Issues found at verify

### 1. The AO residual was a probe defect, not a plugin defect (FIXED — see above)

The only code change made at verify. Nothing in the delivered plugin was wrong.

### 2. `SUMMARY-3.3.md` reports 141 build steps; the verify build is 143 (recorded)

Both forced full recompiles at verify reported **143** ninja steps for the same five targets. The
difference is two steps and no diagnostic; it does not affect any claim. Recorded only because the
summary quotes a step count as a measurement, and a future phase comparing against 141 would see a
drift that is not one.

### 3. Layout §27's counter baseline is run-dependent (recorded, not a defect)

`SUMMARY-3.3.md` quotes §27 as **12 → 12** with the control at **12 → 13**; verify measured
**11 → 11** and **11 → 12**. The baseline is however many 2 Hz status ticks elapsed before the drag
began, so the absolute number is not reproducible and **was never the assertion** — the assertion is
that the drag moves it by **0** and the control moves it by **1**, and both held exactly. Noted so
that the quoted figures are not read as fixed.

### 4. Gate 11's three-module prose has now survived THREE boundaries (recorded)

`VERIFICATION-3.1.md` and `VERIFICATION-3.2.md` both recorded that the phrase *"`juce_audio_basics`
+ `juce_core` + `juce_data_structures` only"* omits `juce_events`, a transitive dependency of
`juce_data_structures`. `SUMMARY-3.3.md` repeats it a third time. The **substantive** claim holds
and was re-verified from `ninja -t commands` with both new TUs joined — no `juce_dsp`, no
`juce_gui_extra`, no `juce_audio_processors`. The asserted invariant is the *absence* of those
three, never the presence of exactly three.

---

## Carried forward to Stage 4

| Item | State |
|---|---|
| **Gate 13's interactive half + Q5's hidden-WKWebView test** | **Open.** ~15 min human. Q5 unrun by four phases |
| **D5 — the Logic hall session** (QUAL-01's audible clause) | **Open.** Stage 4, unchanged since 3.1 discuss |
| **CI wiring** | **Open, and 3.3 is the third phase to widen it.** Neither test target is in `build-and-release.yml`; a JUCE bump performed without them still ships silently. The AO fix makes this *more* urgent, not less — the probe that would have been flaky in CI is now sound, and nothing runs it |
| **Locks and file I/O in `processBlock`** | grep + inspection only. `-fsanitize=realtime` unsupported by Apple clang 17.0.0 |
| **Windows** | No Windows compiler has seen this code. MSVC habits are authored and asserted (§20 / §31) |
| **`COMPAT-04` retroactive criteria** | Still the only summary row without a criteria section |

---

## Stage verdict

**Status:** ✅ **VERIFIED**

**Ready for Stage 4:** **Yes.** No blockers.

**Phase 3.3 closes `FUNC-06`, `UI-03`, `UI-04`, `UI-05` — 18 criteria, zero partials — and with them
Stage 3.** See `VERIFICATION.md` in this directory for the stage-level roll-up.
