# Stage 4 — Polish · Phase 4.1 (machine gates) — Verification

**Plugin:** O-Octagon
**Stage:** 4 of 4 — Polish / Validation · **Phase 4.1 of 2**
**GSD phase:** verify
**Date:** 2026-08-13
**Branch:** `feat/o-octagon` @ `4952a8ca` (freeze commit `fba35081` + the SUMMARY commit)
**Inputs:** `CONTEXT-4.1.md` (D1–D10), `RESEARCH-4.1.md` (N1–N7), `PLAN-4.1.md` (P86–P100, 18 gates,
5 negative controls), `SUMMARY-4.1.md`

> **Every one of the 18 gates was re-run from scratch at this boundary. None was read out of
> `SUMMARY-4.1.md`.** That discipline is what this project credits with catching ten
> mis-attributions across 2.3, 3.1, 3.2 and 3.3. It earned its keep again here: **Gate 16b passes on
> substance and fails as spelled**, which reading the summary would not have surfaced — the summary
> records the gate under a different literal than the plan does.

---

## Entry Check — contract checksums

Re-measured before anything else (`pattern_promotion_checksum_pins_replaced_file`).

| Contract | Measured at verify | Expected (PLAN-4.1) | Result |
|---|---|---|---|
| `BRIEF.md` | `697a4f32890d7420…f6b9fbd6` | `697a4f32…` | ✅ unmoved — **ten consecutive phases** |
| `parameter-spec.md` | `b45f88dc5017ec2c…cbb9e02f` | `b45f88dc…` | ✅ unmoved — 17 parameters; 4.1 added none |
| `research/ARCHITECTURE.md` | `2806c788092d9ec9…57bceb17` | `2806c788…` | ✅ the 4.1-discuss pin |
| `ROADMAP.md` | `90c651318ac7a1cc…fa562c92` | `90c65131…` | ✅ the 4.1-plan re-pin |

**`STATUS.md`'s live `contract_checksums` block agrees with all four measured files** — the second
half of Gate 13, and the specific failure the 4.1 plan boundary found. It is corrected and it holds.

**No drift. No contract is amended at this boundary.**

---

## Goal-Backward Analysis

### Original goal (PLAN-4.1)

> *Freeze a binary that a machine has finished arguing with, and leave nothing carried in prose.*

Four concrete claims were attached to it. Each is checked against a measurement, not against the
summary's account of one.

| # | What "works" was defined as | Verified | Evidence |
|---|---|---|---|
| 1 | A push runs the probes on a GitHub runner and goes red when they fail — **a URL, not a local run described in a summary** | ✅ | Run [31708358940](https://github.com/taylorbrook/O-Audio-VST-Development/actions/runs/31708358940), `conclusion: success`, both jobs. `headSha` = `fba35081a5f0b0d3a8abbbd2e2d1745b91ed8def` — **the freeze commit exactly**. The runner's own log carries `45 probe(s), 0 failure(s)` and `50 probe(s), 0 failure(s)` |
| 2 | An MSVC compiler parses this code for the first time; a Windows VST3 passes pluginval 10 | ✅ | Same run, `octagon-windows-vst3` → success. **Zero** `warning C####` across the whole 192 KB log. Editor Automation `14:11:37.827 → 14:11:49.007` = **11.18 s** — the WebView opened and was driven. `Main bus num input channels: 1` / `output channels: 8` |
| 3 | The SAFE-mode predicate asserted as a **form**, not as behaviour on today's five sets | ✅ | CO passes; **NC1 re-run at verify** — the rejected spelling made CO fail naming both discriminating rows while BM passed 50/50 |
| 4 | Six factory presets load without moving the source or the scene, proven on the **eleven untouched** | ✅ | CP passes with `armed yes (11/11 away from default)`; **NC2 re-run** — the six-changed clause still passed while the eleven-unchanged clause failed |

### One result stronger than the goal asked for

**The freeze is bit-reproducible.** A forced full recompile from the committed tree reproduced both
bundle binaries byte-for-byte:

| Bundle | Rebuilt at verify | Freeze record | Installed on disk |
|---|---|---|---|
| VST3 | `c0fdd8f2…dce29844a` | `c0fdd8f2…dce29844a` ✅ | `c0fdd8f2…dce29844a` ✅ |
| AU | `1e04f0a8…5f7bda007` | `1e04f0a8…5f7bda007` ✅ | `1e04f0a8…5f7bda007` ✅ |

P100 asked for the freeze to be *recorded*. It turns out to be *checkable*: 4.2 can re-derive the
binary it is testing rather than trusting a hash someone wrote down. Recorded here because it changes
what a 4.2 finding can conclude — a mismatch at 4.2 would now be a real signal, not a build-nondeterminism
excuse.

---

## Automated Checks — all 18 gates, re-run

| # | Gate | Pass condition | Result at verify |
|---|---|---|---|
| 1 | Clean 3-format build + both test targets, **forced full recompile** | exit 0, zero `warning:`/`error:`/`FAILED` | ✅ `ninja -t clean` on all five targets (150 files), rebuild **exit 0**, `grep -c` for the three tokens = **0** |
| 2 | Both C++ test targets | 95 probes, 0 failures | ✅ unit **45/0** exit 0, harness **50/0** exit 0 |
| 3 | `node tests/ui_frontend_check.js` | exit 0, 42 sections | ✅ `ALL SECTIONS PASS — 42 sections` |
| 4 | `node tests/ui_layout_check.js` | exit 0, 27 sections, must not SKIP | ✅ `ALL SECTIONS PASS — 27 sections`; **zero** occurrences of "skip" in the log |
| 5 | `auval -v aufx OuOc OuDv` | SUCCEEDED + six `AUChannelInfo` | ✅ **AU VALIDATION SUCCEEDED**, exit 0, `[1, 1] [1, 2] [1, 8] [2, 1] [2, 2] [2, 8]` |
| 6 | pluginval s10, VST3 ×3 / AU ×3 | all six exit 0, zero `FAILED` | ✅ six runs, six exit 0, `grep -c FAILED` = **0** in all six logs |
| 7 | `ci-tests.yml` macOS job green **on a real push** | a run URL | ✅ run 31708358940, `octagon-probes-macos` → success, headSha = freeze commit |
| 8 | `ci-tests.yml` Windows job green | MSVC builds, pluginval 10 exit 0, log uploaded | ✅ `octagon-windows-vst3` → success; 0 MSVC warnings; 11.18 s Editor Automation |
| 9 | JUCE pin derived | exactly one `8.0.14` in `.github/` | ✅ `grep -rn "8\.0\.14" .github/` → **1 hit**, `.github/juce-version.txt:1` |
| 10 | 17 params vs `parameter-spec.md`, three sides | 17/17, 4.1 adds none | ✅ `params::id()` table 17 + `static_assert (kCount == 17)`; APVTS layout 4+2+8+2+1 = 17; spec rows 1–17 (7–14 = the `w1…w8` family). Ranges/defaults match row-for-row |
| 11 | Unit-target link line | no `juce_audio_processors`, no `juce_gui_*` | ✅ `juce_audio_basics`, `juce_core`, `juce_data_structures` only — `RigPolicy.h` did not widen it |
| 12 | `createEditor` guard; `PluginEditor.cpp` absent from harness | both | ✅ `#if JUCE_WEB_BROWSER` at `PluginProcessor.cpp:776`; zero `PluginEditor` hits in the harness CMakeLists |
| 13 | Contract checksums + `STATUS.md` agrees | four correct, ROADMAP at `90c65131…` | ✅ all four exact; `STATUS.md:1049-1052` matches the measured files |
| 14 | The five negative controls | all five as declared; tree byte-identical | ✅ **all five reproduced**, plus the sixth observation. Four mutated files re-measured byte-identical |
| 15 | `gen_dbap_reference.py --check` | exit 0, 102 cases | ✅ `--check OK — 102 cases` |
| 16 | Static gates | three-container literal once; `loadPreset` once; forbidden-TU grep clean | ⚠️ **16a ✅, 16c ✅, 16b passes on substance and FAILS AS SPELLED** — see Issues |
| 17 | `~/Library/O-Octagon/Presets/User/` byte-identical | ✓ | ✅ **still never created** after a full gate run including 6 pluginval opens and an `auval`. Factory's seven files byte-identical before and after — CP's delete-and-regenerate is deterministic |
| 18 | Install + dual-variant sweep | both installed; warning recorded either way | ✅ `build-and-install.sh O-Octagon` exit 0; `⚠ Sweeping ALTERNATE-variant` **count = 0, recorded ABSENT**; only `-dev` variants on disk |

### Gate 14 — the five negative controls, re-run mutation by mutation

| # | Mutation | Declared | Observed at verify |
|---|---|---|---|
| **NC1** | `isRealRig` → `!= mono() && != stereo()` | CO fails, BM still passes | ✅ CO **failed**: *"7.1.4 (12ch) READ AS REAL — BANNER DOWN ON AN UNMAPPED RIG"* and the same for `octagonal (8ch)`; unit 45/**1**. BM **passed, 50/0** |
| **NC2** | Delete the restore from `loadPreserving` | CP's eleven-clause fails, six-clause passes | ✅ *"the SIX → width 4.50 rolloff 3.00 … (applied); the ELEVEN MOVED (first: srcX → reset to default)"*; harness 50/**1** |
| **NC3** | Stub the apply entirely | Distant Field fails, **Concert Default passes** | ✅ Distant Field: *"the SIX → width 0.00 rolloff 4.00 blur 0.10 hullAtten 1.00 air 0.35 gain 0.00 — DID NOT MATCH Distant Field"*, 50/**1**. Re-pointed at Concert Default with the same stub: **50/0, PASS** |
| **NC4** | Remove CP's hermetic delete, then edit the source definition | CP passes on the stale file | ✅ **50 probes, 0 failures** while `PresetPolicy.h:141` said `blur = 0.20f` and the probe read `0.55` off disk. The `.factory-version` sentinel no-opped the write exactly as P95 predicts |
| **NC5** | Perturb the generated Layer-2 golden | the unit target **fails to compile** | ✅ exit 1, `static_assert` fired: *"JUCE's ChannelType enum values or 8-channel set membership have CHANGED…"* |

**The sixth observation reproduces too.** A data-only perturbation of the golden — swapping
`kCreate7point1`'s slots 6 and 7 without touching the SHA — **compiles clean (exit 0)** and is caught
at *runtime* by probe B: *"7.1: slot 6 runtime 20 != golden 21 / slot 7 runtime 21 != golden 20"*,
exit 1. The two layers cover different mutations and neither subsumes the other. This is not a
restatement of the summary; it was measured here.

**NC3's byproduct is the sharpest evidence in the phase.** Under the stub, the six read
`0.00 / 4.00 / 0.10 / 1.00 / 0.35 / 0.00` — *exactly Concert Default's authored values*. P94's rule
("CP must never use Concert Default") is not a stylistic preference; the failing output literally
prints the passing preset's value set.

**Tree byte-identical afterwards**, all four mutated files:

| File | After all five NCs |
|---|---|
| `Source/Data/RigPolicy.h` | `33e33845b323262c…` ✅ |
| `Source/Data/PresetPolicy.h` | `cfbfde773804f9b5…` ✅ |
| `tests/render-harness/main.cpp` | `6bbd0c07418e5575…` ✅ |
| `tests/unit/main.cpp` | `631b7ca34eaf087c…` ✅ |

Baseline re-run after the reverts: **45/0 and 50/0**, bundle checksums still matching the freeze,
`User/` still absent.

### Mechanics verified in the shipped workflow files, not inferred from the plan

| Claim | Site | Result |
|---|---|---|
| Secretless, `contents: read` only | `ci-tests.yml:41-42` | ✅ no `secrets.` reference anywhere in the file |
| `on: [push, pull_request]`, **no `paths:` filter** (P89) | `:39` | ✅ the only `paths:` mention is the comment at `:31` recording the decision |
| The gate is the **compile** (P86 mechanic 1) | `:123`, `:215` | ✅ `cmake --build` in both jobs |
| Never `OUARICON_RELEASE=ON` (P86 mechanic 4) | `:108`, `:199` | ✅ absent, with the reason stated at both sites |
| `SKIP_PLUGINS` containment (mechanic 2) | `:104`, `:197` | ✅ both jobs |
| JUCE pin is an **output**, not an `env` (P87) | `build-and-release.yml:93`, `:103-108` | ✅ `parse-tag` reads the file → `juce_version` output; both consumption sites (`:151` macOS, `:516` Windows) read `needs.parse-tag.outputs.juce_version`; the `env:` entry is gone (`:69` records why) |
| P13 prohibition **kept**, not overturned (A2) | `plugins/O-Octagon/CMakeLists.txt:158-171` | ✅ *"That prohibition still stands and is still the reason the file below exists"* |
| Docs (Task 12) | — | ✅ `CHANGELOG.md` and `NOTES.md` present; `PLUGINS.md:68` reads `🚧 Stage 4 (4.1 complete)` |

---

## Requirements Verification

**Stage:** 4, phase 4.1 · **Rows in scope:** 4 (`COMPAT-04`, `COMPAT-01`, `COMPAT-02`, `QUAL-01`)

| Requirement | Priority | Status | Acceptance criteria |
|---|---|---|---|
| `COMPAT-04` — defined behaviour on a stereo track | should | ✅ **Complete — 3 of 3** | All three re-verified at this boundary |
| `COMPAT-01` — pluginval VST3 + AU, strictness 10 | must | ✅ **Complete — re-confirmed** | 3/3 criteria; six runs re-run here, all exit 0, plus `auval` |
| `COMPAT-02` — Logic Pro, 8 discrete channels | must | ⏸️ **Pending → 4.2** | 3 criteria, all needing a host. 4.1 correctly does nothing to it |
| `QUAL-01` — no artifacts | must | ✅ Complete (audible clause bounded) | Untouched by 4.1; the clause closes at 4.2 |

### `COMPAT-04` — criterion by criterion, re-verified

| # | Criterion | Closed by | Re-verified here |
|---|---|---|---|
| 1 | `isBusesLayoutSupported()` accepts `(1,1)`, `(1,2)`, `(2,1)`, `(2,2)`; `auval` reports all six `AUChannelInfo` and passes | stage-1 + Gate 5 on the final binary | ✅ `auval` exit 0, **AU VALIDATION SUCCEEDED**, `Reported Channel Capabilities (explicit): [1, 1] [1, 2] [1, 8] [2, 1] [2, 2] [2, 8]` — the six, unchanged across the P91 extraction |
| 2 | Instantiation on 2-channel output is defined and non-destructive **+ the render clause** | stage-1 + **CQ** | ✅ CQ passes: `mono-in/stereo-out:finite(peak 1.000) mono-in/mono-out:finite(peak 1.000)`, parameters at live-range extremes, `isSafeMode()` asserted true before rendering |
| 3 | The banner is raised on mono/stereo and not on the three real containers, **asserted through the complement predicate**, with a fourth 8-channel set confirming the banner **is raised** | **CO** + **BM** + **NC1** | ✅ CO passes: `7.1:REAL 7.1-SDDS:REAL 5.1.2:REAL 7.1.4:SAFE* octagonal:SAFE* quad:SAFE mono:SAFE stereo:SAFE`. BM 50/0. **NC1 re-run**: CO failed on exactly the two starred rows while BM passed |

**The pairing is a measurement here, not an assertion.** CO alone cannot see a broken derivation
site; BM alone cannot see the spelling. NC1 was re-run at this boundary specifically to re-establish
that, rather than to re-read it.

**Criterion 3's discriminator reads correctly.** Per
`pattern_criterion_discriminator_states_outcome_backwards`, the A3/P90 correction was re-checked at
all three sites: `REQUIREMENTS.md` (the criterion), `ROADMAP.md` §4.1, and the shipped doc-comment
now at `RigPolicy.h:41-57`, which states *"The rejected spelling would silently **STOP** raising the
banner"* — the correct direction, with a truth table making the two spellings' disagreement explicit.

**Evidence-line accounting** (`pattern_evidence_line_orphaned_past_next_heading`): the `COMPAT-04`
section carries **3** `[x]` rows and **all** its `→ **` evidence lines fall before `### QUAL-01`.
No orphan; no pending row inherits a stray line. Checked, not assumed.

### Ledger

**29 complete · 0 partial · 1 pending — of 30.** `COMPAT-02` is the only open row and it is owned by
4.2. `REQUIREMENTS.md` frontmatter `openRows:` names it and nothing else. ✅ Matches the plan's
target exactly.

---

## Issues Found

### 1. Gate 16b's literal does not match the code it guards — **the one defect this verify found**

**Severity: low in effect, notable in kind.** The gate's substance holds today; its spelling cannot
detect the drift it exists to prevent.

`PLAN-4.1.md` Task 6 and Gate 16 both specify the static gate as:

> `presetManager.loadPreset (` appears **exactly once** in `Source/`

Measured against the frozen commit `4952a8ca`:

| Search | Hits in `Source/` |
|---|---|
| `presetManager.loadPreset (` — **the gate exactly as written** | **0** |
| `presetManager.loadPreset(` — without the space | **1**, and it is `PresetPolicy.h:202` — **the doc-comment describing the gate**, not a call |
| `\.loadPreset[[:space:]]*\(` — receiver-agnostic | **2**: the same comment at `:202`, and the real call `manager.loadPreset (presetName)` at `:222` |

The parameter is named `manager`, not `presetManager`. So the gate as spelled **counts zero call
sites**. It reports a plausible "1" only when the space is dropped, and that 1 is its own
description. `SUMMARY-4.1.md` records Gate 16 as `.loadPreset (` **once** — true of the substance,
and silently re-worded from the plan's literal, which is why reading the summary would not have
surfaced this.

**What still holds:** there is exactly one `loadPreset` call site in `Source/`, it is inside
`loadPreserving`, and P93's "one implementation, two consumers" discipline is intact. Verified
directly.

**What does not:** the gate cannot catch the likely bypass. A future call site written
`presets.loadPreset(...)` or `mgr.loadPreset(...)` is invisible to a grep anchored on
`presetManager`.

**This is the family this project keeps catching, one level up** — a check that is not looking at
what it claims to look at, the same shape as
`pattern_criterion_discriminator_states_outcome_backwards` and
`pattern_zipper_sweep_probe_needs_liveness_gate`. It is a defect in a *gate*, not in the plugin, and
no shipped behaviour is affected.

**Fix (one line, carried to 4.2):** re-spell the gate receiver-agnostically —
`grep -rnE '\.loadPreset[[:space:]]*\(' Source/` must return **exactly one non-comment hit**. Update
`PLAN-4.1.md` Gate 16, the `PresetPolicy.h:202` doc-comment, and any future stage's gate list in the
same edit, per the standing parting rule.

### 2. `checksum_referent_discrepancy_4_1_discuss` — **discharged, with a permanent residual**

Carried into this boundary with **owner: 4.1 verify**. Disposition:

**The finding is real and is confirmed.** `CONTEXT-4.1.md`'s Entry Check ticked `ARCHITECTURE.md` "on
arrival" as `a8a358f4…` ✅, a value `STATUS.md`'s own `architecture_checksum_superseded` list records
as *retired at 3.3 discuss*, and which `PLAN-3.3.md` contradicts. The check that exists to catch a
gate pointing at the wrong referent pointed at the wrong referent and reported green.

**P100 anchors the chain but does not settle this.** Git now holds `ARCHITECTURE.md` at exactly two
points:

| Commit | Blob |
|---|---|
| `12ae50dd` (Stage 0) | `bff8a83b379113ac…` — the oldest entry of the superseded list |
| `fba35081` (4.1 freeze) | `2806c788092d9ec9…` — the live pin |

The freeze commit lands *after* every intermediate re-pin, so `cd881a10…`, `a8a358f4…` and
`32a85018…` are **permanently unreconstructible from git**. That is not a gap to be closed later; it
is a fact to be recorded once and not re-investigated.

**What is settled:** the operative pins are not in doubt. All four contracts measure today exactly
what `STATUS.md` and `PLAN-4.1.md` say, verified independently at this boundary. Both ends of the
chain are now in git, so **every boundary from `fba35081` forward is independently checkable** — the
first time that has been true in this project.

**Closed as: DISCHARGED WITH RESIDUAL.** Recorded so that no future reader mistakes the 3.3-era
arrival tick for something that was ever independently confirmed. It was not, and it now cannot be.

**Standing rule this yields, for 4.2 and beyond:** an arrival check compares against `STATUS.md`'s
live `contract_checksums` block — the ledger — never against a value quoted in a prior artifact's
prose. The 4.1-discuss check failed precisely by doing the latter.

### 3. `REQUIREMENTS.md` frontmatter `lastVerified` is one phase stale — housekeeping

Reads `stage-3 phase 3.3 … STAGE 3 COMPLETE` with `lastUpdated: 2026-08-12`, while the file's body
was edited by 4.1 execute and its rows now close at stage-4 phase 4.1. Cosmetic; the row statuses,
the summary table and `openRows:` are all correct. Corrected as part of this verify.

---

## What did NOT run — restated at verify, not inherited

The summary's list was checked against the gate evidence rather than copied. All five stand, and
each is a **named deferral with an owner**, per Stage 4's own rule that prose is not a third option.

| Item | Owner | Blocked on |
|---|---|---|
| `COMPAT-02` in full; `QUAL-01`'s audible clause; bounce-order and LFE-gain file checks; verify-ping through 8 physical outputs; Gate 13's interactive half; Q5's hidden-WKWebView poll | **4.2** | A host and an ear |
| The two JS gates in CI (69 sections, green locally, run by a human) | **none** | Headless-render determinism, not effort |
| Windows **UI correctness** | **none** | Hardware. CI's ceiling is that pluginval 10 opens the editor without timing out — the 11.18 s Editor Automation is evidence the WebView opened, **not** that it looked right |
| RT-safety beyond allocation (locks, file I/O) | **none** (D10) | `-fsanitize=realtime` unsupported by Apple clang 17.0.0. Allocation is measured soundly, which makes this gap **sharper, not smaller** |

One correction to how item 3 is worded going forward: the Windows job's Editor-Automation timing is
evidence of a **live WebView**, which is stronger than "it compiled" and weaker than "it renders
correctly". Both halves matter and the summary states them correctly.

---

## Human Verification

Nothing in 4.1 requires a human. All four items below are 4.2's, listed here only so the boundary is
explicit.

- [ ] Logic Pro on a surround track — negotiated set, **and whether the choice survives session
      recall** (unobserved by anyone; 2.1 saw a fresh instantiation only)
- [ ] Verify-ping through 8 physical outputs — distinct channels, not merely 8 moving meters
- [ ] Per-parameter automation writability for `srcX`/`srcY`/`srcZ` and `w1..w8`
- [ ] `QUAL-01`/2's audible clause — the single hull-crossing gesture on HF-rich material

---

## Stage Verdict

**Status: ✅ VERIFIED**

**Phase 4.1 achieved its goal.** All 18 gates re-ran and passed; the one gate defect found
(16b's literal) is in a gate's spelling, not in shipped behaviour, and the property that gate
protects was verified directly. Both requirement halves closed: `COMPAT-04` is 3 of 3 with criterion
3 proven by a pair of probes whose non-redundancy is a measurement (NC1), and `COMPAT-01` is
re-confirmed on the binary 4.2 will actually run. The five-boundary CI residual closed with a run URL
whose `headSha` is the freeze commit itself.

**Ready for next phase: Yes — phase 4.2 (host-and-ear).**

**Blockers: none.**

**Carried to 4.2:**

1. **Re-spell Gate 16b** receiver-agnostically (Issue 1) — one line, three sites.
2. **The arrival-check rule** (Issue 2): compare against `STATUS.md`'s live block, never against a
   prior artifact's prose.
3. **4.2 runs against `fba35081`** and the two bundle checksums — which this verify confirms are
   **bit-reproducible from source**, so a mismatch at 4.2 is a real signal rather than an ambiguity.

### The freeze, re-confirmed at verify

| Item | Value |
|---|---|
| Commit SHA | `fba35081a5f0b0d3a8abbbd2e2d1745b91ed8def` |
| VST3 bundle binary | `c0fdd8f217f37e510a417a5a4f34155ba20c8caac9e931ccd962dd8bce29844a` |
| AU bundle binary | `1e04f0a8928ac4e5d8bc2767c9f89131431fcdeb237de5980d9e1775b7fda007` |
| Probes | **95 / 0 failures** (unit 45, harness 50) |
| Installed as | `O-Octagon-dev.vst3` / `O-Octagon-dev.component`, no alternate variant on disk |
| CI run | [31708358940](https://github.com/taylorbrook/O-Audio-VST-Development/actions/runs/31708358940) — success, both jobs, headSha = the freeze commit |
| **Reproducible** | **Yes — forced full recompile reproduced both binaries byte-for-byte** |

---

## Verification Environment Note

This verify ran while two sibling Claude sessions were live in the same repository. Partway through,
one of them checked the shared working tree out to `improve/o-spectralshaper-tooltips`, removing
`plugins/O-Octagon/` from disk. **No result above is affected** — every gate had already been
measured against `feat/o-octagon` @ `4952a8ca`, and the remaining static measurement (Gate 16b) was
taken read-only from the commit via `git grep`. The artifacts were written from a dedicated worktree
at `../VST-development-octagon` rather than by switching the shared checkout back, so the sibling
session's work was not disturbed.

Recorded because it is the same hazard class this stage is about: **a result whose provenance is not
stated is a result nobody can re-derive.** The provenance here is `4952a8ca`, and it is checkable.

---

## Next Phase

**Ready for:** phase 4.2 — host-and-ear, against commit `fba35081` and the two bundle checksums
above. `COMPAT-02` is the only requirement row it must close, and it carries three criteria.
