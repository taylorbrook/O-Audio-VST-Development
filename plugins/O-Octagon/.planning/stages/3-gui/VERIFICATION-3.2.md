# Stage 3 — GUI · Phase 3.2 (Venue screen, venue store, verify-ping) — Verification

**Plugin:** O-Octagon
**Stage:** 3 of 4 — GUI · **Phase 3.2 of 3**
**GSD phase:** verify
**Date:** 2026-08-12
**Branch:** `feat/o-octagon` @ `a47cef88` (2.2 / 2.3 / 3.1 / 3.2 work uncommitted)
**Verifies:** `SUMMARY-3.2.md` against `PLAN-3.2.md`, `CONTEXT-3.2.md`, `REQUIREMENTS.md`

---

## Verdict

**✅ VERIFIED.** `FUNC-02` · `FUNC-04` · `FUNC-05` · `UI-01` all close — **4/4 `must` rows, 3/3
criteria each, zero partials**, exactly as declared at discuss, research and plan.
**Ready for Phase 3.3: yes, no blockers.**

**Every gate was RE-RUN from scratch, not read out of `SUMMARY-3.2.md`** — on a forced full
recompile (`rm -rf build/plugins/O-Octagon`, **134 ninja steps, zero compiler diagnostics**).
**78 C++ probes (36 unit + 42 harness), 0 failures. 49 JS gate sections (31 + 18), 0 failures.**
All four contract checksums recomputed byte-exact; **no pin moved at 3.2.**

**Three negative controls were run as NEW work at verify — one per gate family** (static JS,
Playwright, C++ probe). All three fired. The tree was proved **byte-identical** afterwards and every
gate returned to green on the restored tree.

**NC3 independently reproduced D-2's attribution correction**, which is the single most important
thing this verification could confirm — see below.

**Two issues found at verify**, both in *documents* rather than in delivered code; one is fixed here.
**Nothing in the shipped implementation was wrong.**

---

## Goal-backward analysis

### What 3.2 set out to achieve (CONTEXT-3.2.md, PLAN-3.2.md)

1. Turn the plugin from a **renderer** of state into an **editor** of it — 42 venue values typed by a
   user, committed through a single guarded apply path.
2. A `.venue` file store that round-trips all 42 values and refuses a foreign or malformed file.
3. A musical preset store that **cannot** reach venue geometry, trims or the label map.
4. A verify-ping: one speaker at a time, manual and auto-cycle, level-bounded, self-stopping.
5. Close **FUNC-02, FUNC-04, FUNC-05 and UI-01** — four `must` rows. Zero partials declared in
   advance.

### What was delivered, and whether it matches

| Goal | Status | Evidence re-measured at verify |
|---|---|---|
| 1 — venue editing through one guarded path | ✅ Achieved | `ui_layout_check.js` §13 — `abc` marked and reverted, `setVenue` count unmoved at 0; a metre value committed **one** call of **42** values. Probe **BZ** — a coordinate through `applyVenueEditChecked()` moved the solved gain vector by **0.055373** at speaker 4. §22 asserts the editor never calls the unguarded `applyVenueEdit` |
| 2 — `.venue` store | ✅ Achieved | Probe **BN** — 42 values bit-identical through `venuefile::save`/`::load` into a **fresh** model, **776 bytes**, `@schemaVersion 1`, proved different from a default model. Probe **BO** — forward `@8` **surfaced**, non-`VENUE` root / 3-speaker file / missing file all **rejected with `out` untouched** |
| 3 — preset store cannot reach the venue | ✅ Achieved | Probe **BW** — 42 venue values **bit-identical** across a preset load, positive control `blur → 0.620`. §27 — `setCustomStateCallbacks` in **0 of 24** source files, comments stripped |
| 4 — verify-ping | ✅ Achieved | **BQ** one lane, seven **EXACT zero**, 0 allocations, label swap moved the lane 1 → 2 · **BS** order `12345678`, total **614 400 smp = 12.8 s** · **BT** latched stop at **5 760 000 smp = 120 s** exact · **BU** refuses an invalid map and stops on a mid-ping flip · **BR** ceiling holds at `+12 dB` **and** `+6 dB` together |
| 5 — four rows, zero partials | ✅ Achieved | 4/4 `complete`, 3/3 criteria each. The five remaining stage-3 rows (`FUNC-06`, `UI-03/04/05`) stay `pending` for 3.3 by design |

**Delivery matches goals. No goal was quietly narrowed, and no 3.3 work was pulled forward.**

---

## Requirements verification

**Stage:** 3-gui, phase 3.2
**Requirements verified at this phase:** 4 (`FUNC-02`, `FUNC-04`, `FUNC-05`, `UI-01` — all `must`)

| Requirement | Priority | Status | Acceptance criteria |
|---|---|---|---|
| FUNC-02: Measured venue entry | must | ✅ **Complete** | **3/3**, each with named measured evidence |
| FUNC-04: Verify-ping | must | ✅ **Complete** | **3/3** |
| FUNC-05: Preset separation | must | ✅ **Complete** | **3/3** |
| UI-01: Venue measurement screen | must | ✅ **Complete** | **3/3** |

**Requirements summary:** ✅ Complete **4** · ⚠️ Partial **0** · ❌ Failed **0** · ⏸️ Deferred to 3.3
**5** (`FUNC-06`, `UI-03`, `UI-04`, `UI-05`, plus `UI-02` already closed at 3.1).

### Criterion → evidence, re-measured at this boundary

| Req | # | Verify-phase measurement |
|---|---|---|
| **FUNC-02** | 1 | §13 — `abc` MARKED then REVERTED to `9.85` on blur, `setVenue` count **0**; `7.25` committed **exactly one** call of **42** values. §23 — all 42 are `type="text" inputmode="decimal"`, `type="number"` nowhere |
| | 2 | Probe **BN** — 42 bit-identical, **776 bytes**, differs from a default model |
| | 3 | Probe **BZ** — gain vector changed **0.055373** at speaker 4, through `applyVenueEditChecked()` |
| **FUNC-04** | 1 | Probe **BQ** — 8 targets on a **non-identity** map, one lane each, **seven EXACT zero** (wrongLane 0, leaked 0, silent 0), **0 allocations**; label swap moved the lane **1 → 2** |
| | 2 | Probe **BS** — order `12345678`, on-segment 57 536 / 57 600, gap **19 200 exact**, total **614 400 = 12.8 s** |
| | 3 | Probe **BR** — PASS at `+12 dB` **and** `+6 dB` together. **Bound holds by construction**: `VerifyPing.cpp:258` hard-clamps with `jlimit(-ceiling, ceiling, s)` at `kPeakCeilDb`, and the ping is a post-write **overwrite** so neither gain is in its path. See §Issues 1 on the quoted figures |
| **FUNC-05** | 1 | Probe **BW** — 42 bit-identical, positive control `blur 0.05 → 0.620`; §27 — symbol in **0 of 24** files |
| | 2 | Probe **BX** — normalised **0.3000 / 0.7000 unchanged**, metres (4.10, 15.00) → (4.01, 12.66) followed venue B |
| | 3 | Probe **BY** — **1338 bytes**, 42 venue values identical, 17 parameters identical, restored venue is the measured one |
| **UI-01** | 1 | §12 — **42** fields present, editable, populated, **all fully inside 1100 × 720**; ids derived, not typed. §11 — mini-plan **170.0 × 213.0**, aspect **0.7981** vs returned **0.8000** |
| | 2 | §18 — exactly one row lit and it was the **RETURNED** `getPingState().speaker`; a stub step moved it **3 → 4**; Stop cleared it; refusal on an invalid map rendered `mapInvalid`. §26 — no speaker arithmetic in `venue.js` |
| | 3 | **(a)** probe **BN** · **(b)** §29 · **(c)** Gate 13 — see §Gate 13, which is **partially** discharged |

### The seven ROADMAP orphans — all re-confirmed at verify

| # | Criterion | Verify measurement |
|---|---|---|
| 1 | Latched ping self-stops at 120 s | **BT** — `5 760 000 smp`, exact |
| 2 | Ceiling holds at `trim = +6 dB` | **BR** — PASS with `outputGain +12` **and** `trim +6` |
| 3 | Auto-cycle completes 8 in 12.8 s | **BS** — `614 400 samples` |
| 4 | Duplicate label surfaces the warning | **BP** — rejected, `reason 3 row 4`, venue untouched, `mapInvalid` false; §16 — banner on **both** screens |
| 5 | Label-row change confirmed by ping | **BQ** — sounding lane followed **1 → 2** |
| 6 | Negotiated set name on the Venue screen | §17 — rendered `7.1 Surround`, compared against what `getStatus` **returned** |
| 7 | Per-speaker hull classification readout | §17 — CLASS column == payload, two `ON_EDGE` distinguished |

---

## Automated checks — all 15 gates re-run

| # | Gate | Verify-phase result |
|---|---|---|
| 1 | Clean 3-format build + both test targets, **forced full recompile** | ✅ **134 steps, `warning:` 0, `error:` 0, `FAILED` 0** |
| 2 | `node tests/ui_frontend_check.js` | ✅ exit **0**, **31 sections** |
| 3 | `node tests/ui_layout_check.js` | ✅ exit **0**, **18 sections**, **skip count 0** — the gate did not skip |
| 4 | Stub render **before** Task 9's C++ | ⚠️ **execute-phase ordering record, not re-creatable** — and §0 emits **no timestamp**. See §Issues 2 |
| 5 | `auval -v aufx OuOc OuDv` | ✅ **AU VALIDATION SUCCEEDED** |
| 6 | pluginval s10, VST3 ×3 / AU ×3 | ✅ **6/6 exit 0, zero `FAILED` lines** |
| 7 | Both C++ test targets | ✅ **36 + 42 = 78 probes, 0 failures**, exit 0 / exit 0 |
| 8 | `gen_dbap_reference.py --check` | ✅ exit 0, **102 cases**, no fixture drift |
| 9 | 17 params vs `parameter-spec.md` | ✅ **17/17 across four sides** — re-derived at verify by an **independent** parse of the spec table, the `makeFloat` call sites, the `params::id()` table and the relay construction. Ids, ranges, defaults and labels all identical; **no `kSliderIds` literal list**; relays built from `params::id(i)` |
| 10 | `createEditor` guard; `PluginEditor.cpp` absent from harness | ✅ arms diverge, generic editor demoted to `#else`; **0** `PluginEditor` references in either test `CMakeLists.txt` |
| 11 | Unit-target link line | ✅ resolved from `ninja -t commands`: `juce_audio_basics`, `juce_core`, `juce_data_structures`, `juce_events` — **no `juce_dsp`, no `juce_gui_extra`, no `juce_audio_processors`**, with `VenueFile.cpp` joined. See §Issues 3 |
| 12 | Contract checksums | ✅ all four **byte-exact**, **no pin moved** |
| 13 | **Standalone launch, macOS** | ⚠️ **PARTIALLY discharged at verify** — see below |
| 14 | Negative controls | ✅ **3 re-run as NEW work, one per gate family, all three fired**; tree byte-identical afterwards |
| 15 | `venue_layout_study.js` | ✅ exit 0 — table **752 × 277**, rows **32.5 px**, doc **1100 × 720**, rail does not overflow |

### Contract checksums, recomputed at this boundary

| Contract | Measured at verify | STATUS frontmatter | Result |
|---|---|---|---|
| `BRIEF.md` | `697a4f32…f6b9fbd6` | `697a4f32…f6b9fbd6` | ✅ |
| `parameter-spec.md` | `b45f88dc…cbb9e02f` | `b45f88dc…cbb9e02f` | ✅ |
| `research/ARCHITECTURE.md` | `a8a358f4…9b6d4408` | `a8a358f4…9b6d4408` | ✅ |
| `ROADMAP.md` | `aec7d0ce…0137ee29` | `aec7d0ce…0137ee29` | ✅ |

**All four byte-exact. No pin moved at 3.2**, as P51–P68 predicted. The three §8 re-pins remain
scheduled for **3.3 discuss**, untouched.

---

## Negative controls — 3 run as NEW work at verify, 3 fired

3.1 ran one per *gate file*. 3.2 has **three** gate families, so verify ran one per family — chosen
so that a pass proves the family non-vacuous rather than one assertion.

| # | Family | Injected defect | Result |
|---|---|---|---|
| **NC3** | Playwright | `roomplan.js` `fitBox` — drop the height clamp, width-binding the mini-plan | ✅ **FIRED**, exit **4** |
| **NC1** | static JS | `venue.js` — a second `(v - min) / span` projection outside `metresToPx` | ✅ **FIRED**, exit **1** — `FAIL: [19] … js/venue.js: const nc1 = (v, b) => …` |
| **NC6** | C++ probe | `applyVenueEditChecked` — disable the pre-apply guard | ✅ **FIRED**, exit **1** — `[FAIL] BP duplicate: APPLIED, reason 0 row 0, venue MODIFIED, mapInvalid RAISED` |

**Tree proved byte-identical to baseline afterwards** — 40 files, `shasum -a 256`, zero drift — and
on the restored tree both JS gates returned exit 0 and both probe targets returned 42/0 and 36/0.

### NC3 independently reproduces D-2 — the most important confirmation in this verification

`SUMMARY-3.2.md`'s deviation D-2 claims that the assertion **P62 named** —
`railScrollHeight <= railClientHeight` — is **vacuous in this layout**, and that the load-bearing
guard is the fitted-box-vs-stage assertion instead. Verify did not take that on trust. Re-running
NC3 as new work reproduced all three halves at once:

> **§8 PASSED** — `scrollHeight 720 <= 720`, "the Venue screen also fits — 1100 x 720"
> **§11 `[coarse]` PASSED** — `railScrollHeight 592 <= railClientHeight 592`
> **§11 `[guard]` FIRED** — `the fitted plan is inside its stage — 375 <= 213 (plan 300 x 375, stage 300 x 213)`

A **162 px overflow**, and *both* the document check and the named rail check reported green. Only
the fitted-box assertion caught it. **D-2's attribution correction is confirmed independently**, and
the `[coarse]` / `[guard]` labels now in the source are accurate.

**D-1 is likewise confirmed**: §12 measured the shipped mini-plan at **170.0 × 213.0 px**, not Q11's
predicted 270 × 337, while gate 15's study still reproduces Q11's own mock at 270 × 337 — the two
figures describe different pages, exactly as SUMMARY-3.2 states.

---

## Gate 13 — partially discharged at verify

The Standalone was launched from the verify build and photographed. **Window measured
1102 × 778** (= 1100 × 720 + Standalone chrome), matching 3.1 exactly. Confirmed live in WKWebView:

- The **ROOM / VENUE** two-screen shell, ROOM active, at the fixed size.
- The plan drew — 8 numbered glyphs, hull polygon, source puck.
- **Speakers 3 and 8 render as dashed rings**, the other six solid (UI-02/2's visual half).
- All **8 weights at `1.00`**; the 9 column controls at their declared `parameter-spec.md` defaults
  (`Rolloff 4.00 dB/2x`, `Blur 0.10`, `Hull Atten 1.00 dB/m`, `Air 0.35`, `Output 0.0 dB`, …).
- **Both frame-level banners live and side by side** — `SAFE Stereo fold — not the 8·channel rig`
  **and** `MAP output set is not 8 channels`. **This is new at 3.2** and confirms **D13's
  frame-level `mapInvalid` banner, with its reason, in WKWebView** rather than only in Chromium.
- The `PLAN` caption em-dash renders correctly (3.1's D-2 fix holds).

**What could NOT be discharged.** Switching to the **VENUE** tab requires a synthetic click into the
WKWebView, and this environment lacks the accessibility permission to deliver one — the attempt
returned `-25208` and the tab did not change. So the **Venue screen's WKWebView render** and
**UI-01/3(c) — SAVE opening a native modal and LOAD reading the file back** remain exactly what
`PLAN-3.2.md` said they were: **a human launch-and-look, ~8 min.**

**Gate 13 is still NOT D5.** D5 remains folded into the Stage 4 hall session, untouched.

> UI-01/3 is nonetheless **complete**, because P57 deliberately built it in three parts so that no
> single part carries the criterion: **(a)** probe **BN** and **(b)** §29 are both fully verified
> here, and (b) is what makes (a) non-vacuous. (c) is corroboration of the modal itself.

---

## Issues found at verify

### 1. `FUNC-04/3`'s evidence quoted a non-reproducible figure as a measurement (FIXED)

**The defect.** `REQUIREMENTS.md` and `SUMMARY-3.2.md` both record probe **BR** as
*"RMS **−20.07 dBFS**, peak **−8.99 dBFS**, crest 11.08 dB"*. Re-run at verify, the same probe
measured **−19.97**, then **−20.14**, then **−20.01**, then **−20.13** dBFS RMS, with peaks from
**−9.21** to **−8.47** dBFS. It **passes every time** — but the quoted numbers cannot be reproduced.

**Why.** `VerifyPing`'s RNG is `juce::Random rng;` (`VerifyPing.h:199`), correctly **member-owned**
rather than `getSystemRandom()`. But JUCE's *default* constructor is
`Random::Random() : seed (1) { setSeedRandomly(); }` (`juce_Random.cpp:42-45`) — so the pink stream
differs on every run and every instance. Member-ownership removes **stream interleaving**
(`pattern_rng_stream_interleave_blocksize`, which is what the header comment claims and correctly
achieves); it does **not** make the stream reproducible, and nothing in the source names that.

**Why it is not a requirement failure.** FUNC-04/3 asks that the level be *"bounded by a fixed
conservative ceiling"*, and **the bound holds by construction, not by measurement**:
`VerifyPing.cpp:258` clamps every sample with `juce::jlimit (-ceiling, ceiling, s)` at
`kPeakCeilDb` — a hard ceiling, not a limiter — and the ping is a **post-write overwrite**, so
neither `outputGain` nor the trims are in its signal path at all. The probe correctly asserts a
**band**, which is the right design for a stochastic source.

**Fixed at verify**, in `REQUIREMENTS.md` only: the evidence line now states the construction
argument first, marks the figures as one run's sample, and records the five-run verify range.
**Criteria text unchanged; no status changed.** Nothing in delivered code was wrong.

**For 3.3:** if a future phase wants the ping stream reproducible (for a bit-exactness or
block-size-invariance probe), it must seed explicitly — `juce::Random rng { 1 };` — which is a
one-word change, but it is a *decision*, not a default, and should be made deliberately.

### 2. Gate 4 / UI-02-6: §0 emits no timestamp, so the ordering claim has no durable artifact

`SUMMARY-3.2.md` is already honest about 3.2's instance — *"no wall-clock stamp was captured … an
execute-phase RECORD, not a measurement."* Verify confirms the underlying cause and finds it is
**broader than 3.2**: `ui_layout_check.js` §0 prints only
`NOTE: [0] PluginEditor.cpp absent — this is the PRE-INTEGRATION run (UI-02/6)`. **It emits no
timestamp at all.** A repo-wide search for `14:22:05` returns hits **only in planning prose** —
`REQUIREMENTS.md`, `STATUS.md`, `SUMMARY-3.1.md`, `VERIFICATION-3.1.md` — and none in any
machine-produced artifact.

So `VERIFICATION-3.1.md`'s phrasing *"§0's recorded run at `2026-08-12T14:22:05Z`"* over-attributes
to the gate: the stamp was transcribed from console output at the time, and §0 never recorded it.
The ordering claim is credible and consistent across four documents, but it rests on transcription.

**Recorded, not fixed.** Changing §0 now cannot retroactively create 3.2's record, and SUMMARY-3.2
already prescribes the remedy — *"a future phase wanting this as a measurement must print a
timestamp from inside the gate."* **3.3 should land that one line**, at which point the claim
becomes self-evidencing for every phase after it.

### 3. Gate 11's three-module enumeration recurs in prose (recorded, not fixed)

`VERIFICATION-3.1.md` §Issues 2 recorded that SUMMARY's *"`juce_audio_basics` + `juce_core` +
`juce_data_structures` only"* omits `juce_events`, a transitive dependency of
`juce_data_structures`, and noted it *"so 3.2 does not read the three-module list as the asserted
invariant."* `SUMMARY-3.2.md` gate 11 repeats the three-module wording verbatim.

Resolved from `ninja -t commands` at verify, the link line is those three **plus `juce_events`**.
**The gate's substantive claim holds exactly** — no `juce_dsp`, no `juce_gui_extra`, no
`juce_audio_processors` — so this remains an incomplete enumeration in a prose summary, not a
regression. Restated here because it has now survived two boundaries.

### 4. Cold-configure warnings — unchanged, re-recorded so they are not read as new

The forced full rebuild emitted **zero compiler diagnostics** across 134 steps. The repo-wide
`JUCE_BUNDLE_ID` CMake messages noted at 2.2, 2.3 and 3.1 are unchanged and none are O-Octagon's.

---

## Method note — what verify could and could not re-create

Two claims in `SUMMARY-3.2.md` are **execute-phase records** that this boundary cannot re-measure
without deleting delivered work, and both are stated as records rather than presented as verify
measurements:

- **Gate 4's ordering** — that `ui_layout_check.js` passed 18/18 against the ui-stub *before*
  `VenueFile.cpp` and `VerifyPing.cpp` existed. Those files exist now. See §Issues 2.
- **`kPinkNormScalar`'s calibration at execute** — verify measures the *result* (probe BR asserts
  the band), which is what P60 specified, and does not re-derive the constant.

Everything else in this document was measured fresh at this boundary.

---

## Human verification

- [x] Standalone launches on macOS and the WebView **renders** — window 1102 × 778, ROOM screen
      confirmed live in WKWebView at verify
- [x] **`mapInvalid` banner is frame-level, beside SAFE, with its reason** — observed in WKWebView
      (D13's half of ROADMAP orphan 4)
- [ ] **Venue screen renders in WKWebView; SAVE opens a native modal and LOAD reads the file back**
      (UI-01/3c) — **~8 min human launch-and-look, still open**
- [ ] **In-host confirmation that host automation moves every control** — **Stage 4** session (D2)
- [ ] **D5 / QUAL-01's *audible* clause** — the ~15 min Logic hall session, **Stage 4** (D2)

---

## Residuals — unchanged, restated so none is read as settled

| Residual | Destination |
|---|---|
| **Gate 13's Venue/modal half** (UI-01/3c corroboration) | **Human, ~8 min** — see above |
| **D5 / QUAL-01's *audible* clause.** QUAL-01 has now carried an unverified clause through **all** of Stage 3 | **Stage 4** hall session (D2) |
| **The CI gap** — 78 probes and 49 gate sections, none wired into `build-and-release.yml`. **3.2 widens it again** | **Stage 4** |
| `COMPAT-02` (must) — Logic on a surround track, 8 discrete channels | **Stage 4** |
| `COMPAT-04` — the only requirement row with no criteria section | **Stage 4** |
| **UI-04 / UI-05 descope decision** | **3.3 discuss** |
| **D-1's inverted Q11 comparison** — the main column's slack is **251 px** vs the rail's mini-plan at **170**; Q11's rejection of the main column points the other way now | **3.3 discuss** |
| **D-2's `[coarse]` rail assertion** — kept, but it is not the guard | **3.3 discuss**, since 3.3 adds meters and an elevation strip to that same rail |
| **§0's missing timestamp** (§Issues 2) | **3.3** — one line inside the gate |
| The three §8 contract re-pins (all scene-related) | **3.3 discuss** |

---

## Three declarations 3.3 inherits — re-confirmed at verify

1. **N8 — `mapInvalid` is AUDIBLE, and it is not "the retained map".** `mappedOutputAvailable()`
   false sends `GainStage` to its `else` arm, which writes `out[ch][n] = ch == 0 ? sL : sR` with
   `numWrite 8`: **speaker 1 gets L; speakers 2–8 all get R at unity.** Any 3.3 probe asserting that
   an invalid map "retains" anything must assert it against the **snapshot**, never against the
   output buffer. NC6 re-fired at verify with exactly that signature.
2. **The 3.3 half of the gesture obligation is still open.** P59 closed the preset half (17 brackets
   at O-Octagon's call site, `NC5`-proven at execute). **Scenes must bracket each of `w1..w8`**;
   `gesture_bracket_obligation` stays `CLOSED_3_1_PUCK__CLOSED_3_2_PRESET__OPEN_3_3_SCENES`.
3. **FUNC-05's assertion changes shape at 3.3.** At 3.2 it is *"no custom-state callback exists at
   all"* — one grep, §27, over 24 files, re-run green at verify. At 3.3 it becomes *"exactly one
   exists and its body touches only `SCENES`"*, which is why **FUNC-06/5 re-runs FUNC-05's
   bit-compare** rather than inheriting it.

---

## Stage verdict

**Status:** ✅ **VERIFIED** (Phase 3.2)

**Ready for next phase:** **Yes** — Stage 3 Phase 3.3 (weight scenes, meters, elevation strip:
`FUNC-06`, `UI-03/04/05`).

**Stage 3 is NOT complete** — 3.2 of 3. Five stage-3 requirement rows remain `pending` by design.

**Blockers:** none. Gate 13's Venue/modal half is an **open human item**, not a blocker: UI-01/3
closes on parts (a) and (b), both fully verified here.
