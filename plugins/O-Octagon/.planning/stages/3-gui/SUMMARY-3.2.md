# Stage 3 — GUI · Phase 3.2 (Venue screen, venue store, verify-ping) — Execute Summary

**Plugin:** O-Octagon
**Stage:** 3 of 4 — GUI · **Phase:** 3.2 of 3 · **GSD phase:** execute
**Date:** 2026-08-12
**Branch:** `feat/o-octagon`
**Plan:** `PLAN-3.2.md` (P51–P68, 20 tasks)

---

## Outcome

**All 20 tasks complete. `FUNC-02` ✅ · `FUNC-04` ✅ · `FUNC-05` ✅ · `UI-01` ✅ — all four `must`,
zero partials, exactly as predicted at discuss, research and plan.** The sixth consecutive phase
under that discipline.

| Measure | At 3.1 | At 3.2 | Result |
|---|---|---|---|
| C++ probes | 65 (33 unit + 32 harness) | **78** (36 unit + 42 harness) | 0 failures, both targets exit 0 |
| JS gate sections | 30 (20 static + 10 Playwright) | **49** (31 + 18) | both exit 0 |
| Native-fn surface | 3 | **13** | closed three ways |
| Negative controls (cumulative) | 13 | **19** | all six of 3.2's fired |
| Compiler diagnostics, forced full recompile | 0 | **0** | 132 steps |

**All four contract checksums byte-exact at the execute boundary. No pin moved, no contract
amended.** `REQUIREMENTS.md` edited only to tick the four rows and attach evidence (P68).

---

## The three findings the plan said must not be lost

1. **N8 — `mapInvalid` is AUDIBLE.** Landed as `applyVenueEditChecked()`, whose predicate is
   `ochan::buildSpeakerToBuffer()` itself, built into a scratch array. Probe **BP** confirms a
   duplicate label is rejected with the row named, the venue untouched and `mapInvalid` never
   raised. **NC6** removed the guard and BP fired with exactly the audible signature:
   `duplicate: APPLIED, venue MODIFIED, mapInvalid RAISED`.
2. **The enumeration hole.** Closed by deriving the page-module registry from disk in **Task 1,
   before `venue.js` existed** (P51). **NC1** and **NC2** both fired — and they could only fire
   because §19 and §3 now iterate that registry. Under the 3.1 arrays they would have passed by not
   looking, for the sixth time.
3. **N4 — a dropped completion.** Every write treats its completion as advisory; the authoritative
   state converges on the `venueGen` poll. §25 asserts no `.then()` handler establishes venue state
   and that `startPingPoll()` runs **before** `startPing` is called, not inside its `.then()`.

---

## How the 18 plan decisions landed

| P | Landed | Note |
|---|---|---|
| **P51** | ✅ as planned | `PAGE_MODULES` read from disk; §1/§3/§6/§7/§12/§14/§19 iterate it; §21 asserts `>= 3`, ⊆ SOURCES, and set equality. **§7 generalised too** — a seventh section of the same class the plan did not enumerate (deviation D-12) |
| **P52** | ✅ as planned | `applyVenueEditChecked` validates into a scratch array through `buildSpeakerToBuffer` itself. `applyVenueEdit` stays public for probe BL; §22 asserts the editor never calls it |
| **P53** | ✅ as planned | Label holds-and-marks, numerics revert. **NC4 proves the reachability argument**: with a reverting label the swap *never completes* — §15 reported `row1 "L", row2 "R"` and `committed exactly once — 0` |
| **P54** | ✅ as planned | `MapDiagnosis` through a defaulted out-param; every existing call site compiled unchanged. No new atomic. Probe **BV** distinguishes all three reasons and their rows |
| **P55** | ✅ as planned | `trimDb` inside each speaker object, `rake` its own. 42 representable from one call |
| **P56** | ✅ as planned | `Source/Data/VenueFile.{h,cpp}`. **Gate 11 re-verified: the unit link line is still `juce_audio_basics` + `juce_core` + `juce_data_structures` only** |
| **P57** | ✅ as planned | Three parts: probe BN (a), §29 (b), Gate 13 (c). (b) is what makes (a) non-vacuous |
| **P58** | ✅ as planned | Header only, four functions. §27: `setCustomStateCallbacks` in **none** of 24 source files |
| **P59** | ✅ as planned | 17 gestures at O-Octagon's call site, bounded by `oo::params::kCount`. Shared module untouched. **NC5** fired §28 |
| **P60** | ✅ as planned | Post-write overwrite, no `reset()` anywhere in `VerifyPing.cpp`, both clocks in samples. **`kPinkNormScalar` calibrated: 0.09116**, and probe BR measured the *result* at **−20.07 dBFS** |
| **P61** | ✅ as planned | 100 ms poll while pinging only; no push path; §26 asserts `venue.js` does no arithmetic on a speaker index |
| **P62** | ⚠️ **landed, but the named assertion was wrong** | `fitBox` stated once and exported once ✅. **But `railScrollHeight <= railClientHeight` is VACUOUS in this layout** — see deviation D-2 |
| **P63** | ✅ as planned | Table-left / rail-right. Table measured **752 × 277 px, rows 32.5 px — Q11 reproduced exactly**. Mini-plan differs; see D-1 |
| **P64** | ✅ as planned | §25 asserts the convergence; §29 the SafePointer form |
| **P65** | ✅ as planned | Thirteen, and §3's literal moved 3 → 13 and failed loudly until every one existed in all three places |
| **P66** | ✅ as planned | BN–BZ, 78 probes, 0 failures. None of A–BM regressed |
| **P67** | ✅ **all six fired** | And NC3 corrected an attribution — see below |
| **P68** | ✅ as planned | `REQUIREMENTS.md` criteria unchanged; only ticks and evidence added |

---

## Deviations

Each is a thing the plan did not predict. None changes a contract.

### D-1 — the mini-plan lands at **170 × 213**, not Q11's predicted 270 × 337

Measured on the rendered page. The rail's 592 px divides as `16 (set name) + 213 (mini-plan stage)
+ 65 + 116 + 142 + 4 × 10 gaps`. **Q11's mock had four rail items and no preset bar**; the shipped
rail has five, and the preset group alone is 116 px — `213 + 116 = 329`, which is where Q11's 337
came from.

Nothing in the gate or the fit rule is affected: §11 passes, §12 confirms the aspect follows the
returned envelope (0.7981 vs 0.8000), and NC3 still fires.

**But Q11's other conclusion INVERTS at the real content, and 3.3 must know.** Q11 rejected the main
column as a home for the mini-plan because "height-bound there it would be ~240 px wide, narrower
than the rail's 270". The rail's is **170**, and the main column's measured slack is **251 px**. The
comparison now points the other way. **Not acted on here** — D9/P63 fixes the layout and changing it
at execute would be a larger deviation than accepting a smaller plan. **Carried to 3.3 discuss**,
where the meters and the elevation strip re-open the rail budget anyway.

### D-2 — §11's named assertion is **vacuous**; a different one is the guard

P62 named `railScrollHeight <= railClientHeight`. **NC3 proved it is not a guard in this layout.**
Width-binding the mini-plan produced **300 × 375 inside a 300 × 213 stage — a 162 px overflow — and
that assertion still reported `592 <= 592` and PASSED.**

The reason is the box tree. Q11's mock made the plan a direct flex *child* of the rail, so an
oversized plan grew the rail's own content. The shipped layout makes `.miniplan` a
`flex: 1 1 auto` **stage** that absorbs the residual height, and the `<svg>` overflows the *stage* —
which Chromium does not propagate into the rail's `scrollHeight`.

**The load-bearing assertion is the fitted box against the stage that was measured to produce it**,
which is what fired. Both are kept and now labelled `[coarse]` and `[guard]` in the source, because
the rail check does cover a different shape (a rail *group* growing past the stack). This is the
same class of error Q11 itself caught — an assertion measuring the wrong box — one layer down, and
**it was found only because NC3 was run rather than assumed.**

### D-3 — the layout gate needed `force: true` on its tab clicks

NC3's overflow was severe enough that the `<svg>` physically covered `#tab-room`, so Playwright
retried for 30 s and the whole gate died with a `TimeoutError` **in section 8's trailing click, 200
lines before §11 could name the fault**. A layout overflow must produce a *named* failure, not a
click timeout somewhere else. Hardened, with the reason recorded in-source. **A gate improvement
discovered by a negative control, which is what negative controls are for.**

### D-4 — four sections were scanning **raw** source for a token their own comment contains

`ui_frontend_check.js` states this rule at the top of the file and §20 was the one place not
following it. 3.2 writes the comments that expose it:

| Section | Token | Where the comment lives |
|---|---|---|
| §20, §31 | `[safeThis = juce::Component::SafePointer` | `PluginEditor.cpp`, explaining why it is banned |
| §27 | `setCustomStateCallbacks` | `PluginEditor.h`, explaining why it is absent |
| §30 | `juce::Timer`, `getSystemRandom` | `VerifyPing.h`, explaining why neither is used |

All now scan comment-stripped code. The alternative — deleting the explanations — is exactly the
"fix" the file's header warns about.

### D-5 — §31 must **not** scan `ChannelMap.cpp`

The plan named `VenueFile.cpp`, `VerifyPing.cpp` and the new `PluginEditor.cpp` lines. Scanning
`ChannelMap.cpp` as well flagged a pre-existing em-dash at `:133` — a string built with `+` on an
already-constructed `juce::String`, which does **not** go through the `CharPointer_ASCII` conversion
and was adjudicated safe at 3.1 when D-2's `String(const char*)` form was fixed. The plan's list was
right; the over-reach was mine. A gate that cries wolf gets edited rather than obeyed.

### D-6 — §29's "no parallel serialisation path" had to be scoped

`PluginProcessor.cpp:582` legitimately calls `state.createXml()` in `getStateInformation` — the
session path **N2 requires stay exactly as it is**. Narrowed to: `PluginEditor.cpp` serialises
nothing itself, the processor has **exactly one** site and it is the session path, and
`VenueFile.cpp` *does* serialise so the checks are not vacuous.

### D-7 — three consequential gate widenings

- **§12** changed from `starts > 0 && starts === ends` per module to per-module *balance* plus one
  global non-vacuity check. `venue.js` writes 42 VENUE values and **zero** parameters, so it
  balances at 0/0, which is correct and must not fail.
- **§6** `el`-binding whitelist gained `vset-name` and `map-invalid-copy`, the two frame-level value
  nodes `app.js` writes. Still an explicit reviewed list, not a prefix rule.
- **§18** value-node scan **20 → 75** and `VALUE_CLASSES` gained `.vcell-value`, `.vfield`,
  `.map-copy`. The 42 measured metre values in a column are what that rule always existed for.

### D-8 — a CMake **comment** broke §9

The comment explaining the derived registry contained the literal `Source/ui/public/js/*.js`, which
§9's SOURCES regex harvested as an eighth embedded file. Reworded. Same class as D-4, one file over.

### D-9 — `juce::String("…") << x` does not compile

`String::operator<<` takes an lvalue reference, so the temporary binds to the **private**
`operator bool` and the error names neither. Built onto a named local instead, keeping the `<<`
habit.

### D-10 — probes BW / BX use `savePresetToFile` / `loadPresetFromFile`

Rather than the named `savePreset` / `loadPreset`, so the probes do not write into
`~/Library/O-Octagon/Presets`. Both pairs funnel into the same `createPresetJson` /
`applyPresetJson` bodies, which is where the FUNC-05 property lives. §27 carries the other half
statically.

### D-11 — the harness CMake gained the preset-manager include directory

Task 13 named only the plugin target. BW and BX drive `OuariconPresetManager` directly because the
harness has no editor.

### D-12 / D-13 — two additive strengthenings

§7 iterates `PAGE_MODULES` (a seventh section of P51's class); §11 gained a horizontal containment
assertion beside the vertical one.

### Design note — the venue **name** is a display, not a 43rd field

`setVenue` starts from the live venue, so the name survives a table commit untouched. Q11's
"truncates at 220 px" incidental is discharged by giving it a full-width row on the Venue screen
rather than a narrow field at the end of the rake row.

---

## The 15 gates, as run

| # | Gate | Result |
|---|---|---|
| 1 | Clean 3-format build + both test targets, **forced full recompile** | ✅ **132 steps, 0 `warning:`, 0 `error:`, 0 `FAILED`** |
| 2 | `node tests/ui_frontend_check.js` | ✅ exit 0, **31 sections** |
| 3 | `node tests/ui_layout_check.js` | ✅ exit 0, **18 sections**, did **not** skip |
| 4 | Stub render ran **before** Task 9's C++ | ✅ ordering held — **see the note below on its timestamp** |
| 5 | `auval -v aufx OuOc OuDv` | ✅ **AU VALIDATION SUCCEEDED** |
| 6 | pluginval s10, VST3 ×3 / AU ×3 | ✅ **all six exit 0, zero `FAILED`** |
| 7 | Both C++ test targets | ✅ **78 probes, 0 failures** (36 unit + 42 harness), exit 0 / exit 0 |
| 8 | `gen_dbap_reference.py --check` | ✅ exit 0, **102 cases** |
| 9 | 17 params vs `parameter-spec.md` | ✅ **17/17 across four sides**, identical sets; no `kSliderIds` list; relays from `id(i)`; **3.2 adds none** |
| 10 | `createEditor` guard; `PluginEditor.cpp` absent from the harness | ✅ both ✓ (2 guard sites, 0 harness references) |
| 11 | Unit-target link line | ✅ `juce_audio_basics` + `juce_core` + `juce_data_structures` **only** — no `juce_dsp`, no `juce_gui_extra`, no `juce_audio_processors`, with `VenueFile.cpp` joined |
| 12 | Contract checksums | ✅ **all four byte-exact, no pin moved** |
| 13 | **Standalone launch, macOS — HUMAN, ~8 min** | ⏳ **OPEN — the only outstanding item** |
| 14 | The six negative controls | ✅ **all six fired**; tree byte-identical afterwards; both gates exit 0 on the restored tree |
| 15 | `venue_layout_study.js` | ✅ re-run: table **752 × 277**, rows **32.5 px**, doc 1100 × 720 — agrees with §11/§12 |

### Gate 4's timestamp — stated precisely rather than fabricated

The **ordering** held: `ui_layout_check.js` passed **18/18 against the ui-stub** at Task 8, before
`Source/Data/VenueFile.cpp` and `Source/DSP/VerifyPing.cpp` existed, and Task 9 did not begin until
it did. **No wall-clock stamp was captured at the moment of that run**, and file mtimes cannot
corroborate it after the fact — the negative-control restores and the §11 rewrite have since
rewritten `venue.js` and `ui_layout_check.js`. Recorded as an **execute-phase ordering fact**, the
same disposition 3.1 gave UI-02 criterion 6. A future phase wanting this as a *measurement* must
print a timestamp from inside the gate.

---

## Criterion → evidence, as run

Every line below is in `REQUIREMENTS.md` under its own criterion; a per-section `[x]`-vs-`→ **`
count was run programmatically afterwards and all four sections are **`XeXeXe`** — no orphaned
evidence anywhere in the file, and no evidence line attached to a still-open criterion
(`pattern_evidence_line_orphaned_past_next_heading`).

| Requirement | # | Evidence |
|---|---|---|
| **FUNC-02** | 1 | §13 — `abc` marked and reverted to "9.85", `setVenue` count unmoved at 0; `7.25` committed **one** call of **42 values** |
| | 2 | Probe **BN** — 42 bit-identical, 776 bytes, proved different from a default model |
| | 3 | Probe **BZ** — gain vector changed by **0.055373** at speaker 4, through `applyVenueEditChecked` |
| **FUNC-04** | 1 | Probe **BQ** — 8 targets, non-identity map, one lane each, **seven EXACT zero**, 0 allocations; label swap moved the lane 1 → 2 |
| | 2 | Probe **BS** — order `12345678`, on 57536/57600, gap 19200 exact, **total 614400 = 12.8 s** |
| | 3 | Probe **BR** — at `+12 dB` **and** `+6 dB`: RMS **−20.07 dBFS**, peak **−8.99 dBFS** under a −6.0 ceiling |
| **FUNC-05** | 1 | Probe **BW** — 42 bit-identical, positive control (blur 0.05 → 0.620); §27 — symbol in **0 of 24** files |
| | 2 | Probe **BX** — normalised **0.3000/0.7000 unchanged**, metres (4.10, 15.00) → (4.01, 12.66) |
| | 3 | Probe **BY** — 1338 bytes, 42 venue values and 17 parameters identical, restored venue is the measured one |
| **UI-01** | 1 | §12 — **42** fields present, editable, populated, fully inside 1100 × 720; §22 closes the ids four ways |
| | 2 | §18 — indicator followed the **returned** speaker across a stub step (3 → 4); §26 — no speaker arithmetic in `venue.js` |
| | 3 | **(a)** probe BN · **(b)** §29 · **(c)** Gate 13; probe **BO** adds forward-version and malformed-root |

---

## The seven ROADMAP orphans — dispositions

| # | Criterion | Closed by | Result |
|---|---|---|---|
| 1 | Latched ping self-stops at 120 s | Probe **BT** | stopped at **5 760 000 smp** = 120 s × 48 kHz, exact |
| 2 | Ceiling holds at `trim = +6 dB` | Probe **BR** | peak −8.99 dBFS with `outputGain +12` **and** `trim +6` together |
| 3 | Auto-cycle completes 8 in 12.8 s | Probe **BS** | **614 400 samples**, derived from the prepared rate |
| 4 | Duplicate label surfaces the warning | Probe **BP** + §16 | rejected, row named; banner shows `duplicate label — speaker 3` on **both** screens |
| 5 | Label-row change confirmed **by ping** | Probe **BQ** | sounding lane followed 1 → 2 |
| 6 | Negotiated set name on the Venue screen | §17 | rendered `7.1 Surround`, compared against what `getStatus` **returned** |
| 7 | Per-speaker hull classification readout | §17 | CLASS column == payload classification, two `ON_EDGE` distinguished |

---

## The six negative controls

| # | Mutation | Fired | Evidence |
|---|---|---|---|
| **NC1** | second `(v - min) / span` in `venue.js` | ✅ §19 | `js/venue.js: const nc1 = (v, b) => (v - b.minX) / (b.maxX - b.minX);` |
| **NC2** | `nativeFn("getPingStat")` from `venue.js` | ✅ §3 | `called-not-registered: [getPingStat]` |
| **NC3** | width-bind the mini-plan | ✅ **§11 fired while §8 PASSED** | see below |
| **NC4** | label column reverts like the numerics | ✅ §15 | 4 assertions; `committed exactly once — 0`, `row1 "L", row2 "R"` |
| **NC5** | drop the 17 gesture brackets | ✅ §28 | 3 assertions, incl. "endChangeGesture runs AFTER the load" |
| **NC6** | remove `applyVenueEditChecked`'s guard | ✅ probe **BP** | `duplicate: APPLIED, venue MODIFIED, mapInvalid RAISED` |

**After every control the mutation was reverted and the tree proved byte-identical by
`shasum -a 256`**, and both gate files re-run to exit 0 on the restored tree
(31 sections, 18 sections), with both C++ targets back to 78/0.

### NC3's asymmetry, recorded with both halves

> **§8 PASSED** — `scrollWidth 1100 <= 1100`, `scrollHeight 720 <= 720`, "the Venue screen also fits
> — 1100 x 720".
> **§11 FIRED** — `the fitted plan is inside its stage — 375 <= 213`.

That asymmetry is the evidence §11 was not redundant, and it is exactly what Q11 measured: the
document check would not have caught it. **NC3 also went one better than the plan expected** — it
proved that the *specific* assertion P62 named is itself vacuous here (D-2). A negative control that
corrects an attribution is the fifth such in this project.

---

## Three things 3.3 must inherit

1. **N8's inheritance — `mapInvalid` is AUDIBLE, and it is not "the retained map".**
   `mappedOutputAvailable()` false sends `GainStage` to its `else` arm, which writes
   `out[ch][n] = ch == 0 ? sL : sR` with `numWrite 8`: **speaker 1 gets L; speakers 2–8 all get R at
   unity.** Any 3.3 probe asserting that an invalid map "retains" anything must assert it against
   the **snapshot**, never against the output buffer.
2. **The 3.3 half of the gesture obligation is still open.** P59 closed the preset half (17 brackets
   at O-Octagon's call site). **Scenes must bracket each of `w1..w8`**;
   `gesture_bracket_obligation` stays `CLOSED_3_1_PUCK__CLOSED_3_2_PRESET__OPEN_3_3_SCENES`.
3. **FUNC-05's assertion changes shape at 3.3.** At 3.2 it is *"no custom-state callback exists at
   all"* — one grep, §27, over 24 files. At 3.3 it becomes *"exactly one exists and its body touches
   only `SCENES`"*. That is why **FUNC-06/5 re-runs FUNC-05's bit-compare** rather than inheriting
   it.

**Plus, new at 3.2:** D-1's inverted Q11 comparison (the main column now has more room for a plan
than the rail does) and D-2's vacuous rail assertion both belong on the 3.3 discuss agenda, because
3.3 adds meters and an elevation strip to that same rail.

---

## Residuals — unchanged

- **D5 / QUAL-01's audible clause** — folded into the Stage 4 hall session. QUAL-01 has now carried
  an unverified clause through all of Stage 3, as accepted at 3.1.
- **The CI gap** — Stage 4. 3.2 widens it again: 78 probes and 49 gate sections, none of them wired
  into `build-and-release.yml`.
- **`COMPAT-02`** (must) and **`COMPAT-04`** (no criteria section) — Stage 4.
- **UI-04 / UI-05 descope** and **the three §8 contract re-pins** — 3.3 discuss.

---

## Next

**Gate 13 is the only thing standing between this phase and verify**, and it is a human
launch-and-look:

```
/Users/taylorbrook/Dev/VST-development/build/plugins/O-Octagon/OuariconOctagon_artefacts/Release/Standalone/O-Octagon-dev.app
```

Check, in order: the Venue screen renders at 1100 × 720 · typing a coordinate redraws the mini-plan ·
**SAVE opens a native modal and LOAD reads the file back** (UI-01/3c) · the ping sounds and the
indicator follows. **It is still not D5.**
