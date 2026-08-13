# Stage 4 — Polish · Phase 4.1 (machine gates) — Summary

**Plugin:** O-Octagon
**Stage:** 4 of 4 — Polish / Validation · **Phase 4.1 of 2**
**GSD phase:** execute
**Date:** 2026-08-13
**Branch:** `feat/o-octagon`
**Plan:** `PLAN-4.1.md` (P86–P100, 15 tasks, 18 gates, 5 negative controls)

---

## The freeze record (P100)

4.2 runs against **this**, not against whatever is on disk that afternoon.

| Item | Value |
|---|---|
| **Commit SHA** | `fba35081a5f0b0d3a8abbbd2e2d1745b91ed8def` |
| **VST3 bundle binary** | `c0fdd8f217f37e510a417a5a4f34155ba20c8caac9e931ccd962dd8bce29844a` |
| **AU bundle binary** | `1e04f0a8928ac4e5d8bc2767c9f89131431fcdeb237de5980d9e1775b7fda007` |
| **Probes passed** | **95 / 0 failures** (unit 45, harness 50) |
| **Installed as** | `O-Octagon-dev.vst3` / `O-Octagon-dev.component` (dev branding) |
| **CI run** | https://github.com/taylorbrook/O-Audio-VST-Development/actions/runs/31708358940 — **success**, both jobs |

That commit also lands **phases 2.2, 2.3, 3.1, 3.2 and 3.3**, which had never been committed. This
is P100's point rather than housekeeping: the entry check found the `ARCHITECTURE.md` pin
discrepancy **unresolvable from git precisely because nothing since Stage 0 was committed**, and a
binary built from an uncommitted tree is not frozen, it is merely current.

**Deliberately left out of the freeze commit:** uncommitted O-SpectralShaper work and two `.claude/`
housekeeping files, which are unrelated to this phase. Sweeping them in would have made the freeze
SHA ambiguous about what it contains.

---

## Requirements — the ledger moved 28 → 29

| Row | Entering 4.1 | At 4.1 close |
|---|---|---|
| `COMPAT-04` | ⚠️ partial, 2 of 3 | ✅ **complete, 3 of 3** |
| `COMPAT-01` | complete at stage-1 | ✅ complete, **re-confirmed on the final binary** |
| `COMPAT-02` | pending | pending → **4.2** (three criteria, all needing a host) |
| `QUAL-01` | complete, audible clause bounded | complete → 4.2 closes the clause |

**29 complete · 0 partial · 1 pending.** `COMPAT-02` is the only row 4.2 must close.

### `COMPAT-04` criterion 3 — the pairing, and why neither probe is sufficient

**CO** (unit target) calls `oo::rig::isRealRig` **directly** and asserts the partition as a *form*:
true for exactly the three real rigs; false for `create7point1point4()`, `octagonal()`,
`quadraphonic()`, `mono()`, `stereo()`. Rows 4 and 5 are the load-bearing ones — both are ≥ 8-channel
sets that are **not** real rigs, so the shipped complement form reports them SAFE and **the banner
goes up**. CO also asserts both are actually ≥ 8 channels, so the discriminating rows cannot quietly
go vacuous if JUCE stops offering them.

**BM** (render harness, Phase 3.1) drives all five reachable layouts through a real `prepareToPlay`
and asserts `isSafeMode()` — the *wiring*. It structurally cannot reach the discriminating sets,
because it arrives through the host negotiation that filters them out.

**NC1 turned that from an assertion into a measurement.** `isRealRig` rewritten as the rejected
`!= mono() && != stereo()` spelling: **CO failed**, naming both discriminating rows in its failure
message; **BM passed, 50/50**. Reverted, `RigPolicy.h` re-hashed byte-identical.

---

## What changed in the source

### `Source/Data/RigPolicy.h` — new, header-only (P91)

The SAFE-mode partition, extracted so a probe can reach it. **Behaviour is unchanged and that is
measured, not asserted:** BM passes untouched, and `auval` reports the same six `AUChannelInfo`
configs after the edit as before.

> **A finding the plan did not anticipate.** P91's static gate — "the three-container literal appears
> exactly once in `Source/`" — **failed at 2 after the extraction.** `isBusesLayoutSupported()`
> carried its own copy of the same three containers, as three separate `if` statements. That is
> precisely the drift the gate exists to prevent, and it was sitting in the *admission* rule the SAFE
> rule is supposed to track. Both call sites now route through `isRealRig`. The gate reads 1.
>
> This is worth recording because the gate found something the plan wrote it for and did not expect
> it to catch: the plan framed the second copy as a *future* risk, and it was already present.

**P90's inverted sentence was restated in the same edit.** The comment claimed the complement form
exists so a fourth container "cannot silently start raising the banner" — it is what makes it *raise*.
The correction mattered more than a stale comment normally would, because Task 3 moved those exact
lines: unfixed, a backwards claim would have been promoted to the doc-comment on the function CO
pins.

### `Source/Data/PresetPolicy.h` — new, header-only (P92, P93, P96)

**Six factory presets in engineering units**, every value through
`apvts.getParameterRange(id).convertTo0to1(...)` off the **live** range. Verified on disk after the
gate run: all six presets, six keys each, all thirty-six values matching the engineering table to
< 1e-6. `Concert Default`'s six are exactly the shipped defaults.

**`loadPreserving()` — the N5 fix.** The shared module resets *every* parameter to its default before
applying anything (WR-01, correct and deliberate), so a room-character preset omitting the eleven
position/scene keys does **not** leave them alone — it re-centres the source and clears the scene.
Snapshot-and-restore at O-Octagon's call site, never in the nine-plugin shared header.

The header also carries a `static_assert` that the preserved eleven and the authored six **partition**
`oo::params::kCount` — so a future 18th parameter is a build error rather than a silently-reset
control.

**Definitions live here and not in `PluginEditor.cpp`** because that TU is permanently excluded from
the render harness: rules written there are unreachable by any probe, and preset work is the one
place in 4.1 where a green result can be wrong.

### `PluginEditor.cpp` — the call site and factory init

`initializeFactoryPresets` from the **editor** constructor, never the processor. **The claim was
measured rather than reasoned:** after six pluginval runs and an `auval` pass, `~/Library/O-Octagon/`
contained **no `User/` directory at all**. The Factory store was written only when pluginval opened
the editor — which is exactly the split P92 predicted, observed rather than assumed.

---

## CI — the residual that was carried unowned for five verify boundaries

`.github/workflows/ci-tests.yml`, new: secretless, `permissions: contents: read`, `on: [push,
pull_request]`, **no `paths:` filter** (a filter is a gate that can silently not run — a green check
that means "skipped").

`build-and-release.yml`'s standing prohibition is **satisfied, not overturned**. That rule forbids
two *triggers* next to eight Apple signing secrets; a workflow with no secrets is orthogonal to it.
And it delivers the stated failure mode the original destination structurally could not: *"a JUCE
bump ships silently"* — **a JUCE bump is a commit, not a tag.**

**The gate is the `cmake --build` step, not the run step.** NC5 proved it: perturbing the generated
Layer-2 golden made the unit target **fail to compile**, exit 1, with the `static_assert` naming the
consequence. A job that only executes a cached binary would discharge nothing.

### Gate 7 and 8 — the one gate that cannot be faked

**Run [31708358940](https://github.com/taylorbrook/O-Audio-VST-Development/actions/runs/31708358940)
— `success`, both jobs, on the first push.** This closes D6, carried as prose for five verify
boundaries precisely because nobody had to produce a URL.

- **macOS job:** `45 probe(s), 0 failure(s)` and `50 probe(s), 0 failure(s)` in the runner's own log,
  after a real `cmake --build` of both targets against a JUCE downloaded and patched on the runner.
  The numbers match the local run exactly.
- **Windows job — the first MSVC compile of ~11 k lines, and it passed clean.** **Zero** `warning
  C####` in the entire log. pluginval strictness 10 exit 0, and its log shows
  `Completed tests in pluginval / Editor Automation` taking **11.2 s** — the WebView genuinely opened
  and was driven, which is the evidence that distinguishes a real pass from a silently-blank
  WebView. The plugin reported `Main bus num input channels: 1` / `output channels: 8`.

> **The plan predicted first-run MSVC failures and called them "the deliverable"** — `/permissive-`
> two-phase lookup in the templated probe helpers, `min`/`max` macros from `windows.h`, C4244/C4267
> under `juce_recommended_warning_flags`, stricter `constexpr` evaluation in the `static_assert`s.
> **None of them fired.** That is a better outcome than the plan expected, and it is worth stating
> as an observation rather than a claim: it means the code is portable, not that the risk was
> imaginary. RESEARCH-4.1 N4 had already cleared the two *named* patterns by grep; this cleared the
> four a grep could not.

**The JUCE pin is derived.** `.github/juce-version.txt` is the single source; `parse-tag` surfaces it
as an output (a workflow-level `env:` block cannot read a file, which corrects `RESEARCH-4.1` §1.4's
costing). `grep -rn "8\.0\.14" .github/` returns **exactly one** hit.

---

## The five negative controls — all as declared, tree byte-identical afterwards

| # | Mutation | Declared | Observed |
|---|---|---|---|
| **NC1** | `isRealRig` → the rejected spelling | CO fails, BM still passes | ✅ CO failed naming 7.1.4 and octagonal; BM passed **50/50** |
| **NC2** | Delete the restore from `loadPreserving` | CP's eleven-clause fails, six-clause passes | ✅ "the SIX … (applied); the ELEVEN MOVED (first: `srcX` → reset to default)" |
| **NC3** | Stub the apply entirely | Distant Field fails, Concert Default **passes** | ✅ exactly that — P94's preset choice is load-bearing, not incidental |
| **NC4** | Remove CP's hermetic delete, then edit the source definition | CP passes on the stale file | ✅ **50 probes, 0 failures** while the source said `blur = 0.20` and the probe read `0.55` off disk |
| **NC5** | Perturb the generated Layer-2 golden | The unit target **fails to compile** | ✅ exit 1, `static_assert` fired |

**A sixth observation, unplanned.** A data-only perturbation of the golden (swapping two channel
indices without touching the SHA) **compiles clean** and is caught at *runtime* by probe B. The two
layers cover different mutations, and neither subsumes the other.

**And CP's own liveness gate caught a defect in CP.** The first version set `w1 = 1.0f`, which *is*
the weight default — so one of the eleven sat where the reset would have put it, and "bit-unchanged"
was a claim about nothing for that parameter. The probe reported `armed NO — 1 of 11 SAT AT THEIR
DEFAULT, THIS PROBE IS VACUOUS` and failed. The same failure line also surfaced a mangled em-dash
(`â`) from a `juce::String(const char*)` construction — `critical_juce_string_char_ctor_is_ascii_only`,
caught by reading the output rather than by a gate.

---

## Gate results — every one run at execute, none read out of a document

| # | Gate | Result |
|---|---|---|
| 1 | Clean 3-format build + both test targets, **forced full recompile** | ✅ exit 0, **zero** `warning:` / `error:` / `FAILED` |
| 2 | Both C++ test targets | ✅ **95 probes, 0 failures** — unit 45 (exit 0), harness 50 (exit 0) |
| 3 | `node tests/ui_frontend_check.js` | ✅ exit 0, **42 sections — unchanged** |
| 4 | `node tests/ui_layout_check.js` | ✅ exit 0, **27 sections — unchanged**, not SKIPped |
| 5 | `auval -v aufx OuOc OuDv` | ✅ **AU VALIDATION SUCCEEDED**; six configs `[1,1] [1,2] [1,8] [2,1] [2,2] [2,8]` |
| 6 | pluginval s10, VST3 ×3 / AU ×3 | ✅ all six exit 0, zero `FAILED` |
| 7 | **`ci-tests.yml` macOS job green on a real push** | ✅ [run 31708358940](https://github.com/taylorbrook/O-Audio-VST-Development/actions/runs/31708358940) — `45 probe(s), 0 failure(s)` and `50 probe(s), 0 failure(s)` **in the runner's own log**, after a `cmake --build` of both targets |
| 8 | **`ci-tests.yml` Windows job green** | ✅ same run — VST3 built under **MSVC**, pluginval 10 exit 0, log uploaded |
| 9 | JUCE pin derived | ✅ **exactly one** `8.0.14` in `.github/`, in `juce-version.txt` |
| 10 | 17 params vs `parameter-spec.md`, three sides | ✅ 17/17 (spec row 7–14 is the `w1…w8` family); **4.1 adds none** |
| 11 | Unit-target link line | ✅ `juce_audio_basics` / `juce_core` / `juce_data_structures` only — `RigPolicy.h` did not widen it |
| 12 | `createEditor` guard; `PluginEditor.cpp` absent from harness | ✅ both |
| 13 | Contract checksums | ✅ BRIEF `697a4f32…`, parameter-spec `b45f88dc…`, ARCHITECTURE `2806c788…` unmoved; ROADMAP at its new pin `90c65131…` |
| 14 | The five negative controls | ✅ all five as declared; tree byte-identical |
| 15 | `gen_dbap_reference.py --check` | ✅ exit 0, 102 cases — 4.1 does not touch the solver |
| 16 | Static gates | ✅ three-container literal **once**; `.loadPreset (` **once**; forbidden-TU grep clean |
| 17 | `~/Library/O-Octagon/Presets/User/` byte-identical | ✅ — and stronger: **it was never created** |
| 18 | Install + dual-variant sweep | ✅ both bundles installed; **the `⚠ Sweeping ALTERNATE-variant` warning did NOT appear** — recorded absent |

---

## What did NOT run — stated as plainly as what did

- **Anything needing a host or an ear.** `COMPAT-02` in full, `QUAL-01`'s audible clause, the
  bounce-order and LFE-gain file checks, the verify ping through eight physical outputs. **All 4.2.**
- **The two JS gates in CI.** 69 sections, green today, gated by a human running `node`.
  **Named deferral — owner none, blocked on headless-render determinism, not on effort.**
- **Windows UI correctness.** CI's ceiling is that pluginval 10 opens the editor without timing out;
  a silently-blank WebView would surface as an Editor-Automation failure or a timeout, not as a pass.
  **Named deferral — owner none, blocked on hardware.** No human sees the Windows UI this milestone.
- **RT-safety beyond allocation.** `-fsanitize=realtime` is unsupported by Apple clang 17.0.0.
  Allocation is measured by replacing the global `operator new` family; locks and file I/O remain
  grep plus inspection. Measuring allocation soundly makes this gap **sharper, not smaller**.
- **The `checksum_referent_discrepancy_4_1_discuss` finding.** Recorded in `STATUS.md`, **owner 4.1
  verify**. It cannot be settled from git — every re-pin since Stage 0 lived only in the working tree
  until this commit, which is itself the concrete instance of what P100 costs.

---

## Next Phase

**Ready for:** `verify` — and every gate above is to be **re-run from scratch**, not read out of this
document. That discipline has caught ten mis-attributions across 2.3, 3.1, 3.2 and 3.3.

**Then:** phase 4.2, host-and-ear, against commit `fba35081` and the two bundle checksums recorded at
the top of this file.
