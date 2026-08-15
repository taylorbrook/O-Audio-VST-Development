# Stage 4 — Polish · Phase 4.1 (machine gates) — Context

**Plugin:** O-Octagon
**Stage:** 4 of 4 — Polish / Validation
**Phase:** 4.1 of 2 — CI wiring, Windows, pluginval/auval, factory presets, `COMPAT-04`, docs
**GSD phase:** discuss
**Date:** 2026-08-12
**Branch:** `feat/o-octagon` @ `a47cef88` (Stage 2 phases 2.2 / 2.3 and all of Stage 3 uncommitted)
**Participants:** Taylor Brook, Claude

> **This document also decides Stage 4's phase structure**, exactly as `CONTEXT-2.1.md` and
> `CONTEXT-3.1.md` decided theirs. `STATUS.md`'s frontmatter named the next artifact
> `stages/4-polish/CONTEXT.md` because `artifact_suffix` was still `""` with the structure undecided;
> D1 decides it, so the artifact takes the `-4.1` suffix and `artifact_suffix` becomes `-4.1`.

---

## Entry Check — carried obligations from Stage 3

The standing obligation at every boundary: *"Re-verify all four checksums — a checksum that silently
points at the wrong file is worse than no checksum, because it reports green."*
(`pattern_promotion_checksum_pins_replaced_file`)

**Re-run at this boundary, before anything else. All four byte-exact on arrival:**

| Contract | SHA-256 on arrival | Result |
|---|---|---|
| `BRIEF.md` | `697a4f32…f6b9fbd6` | ✅ matches the Stage 3 close |
| `parameter-spec.md` | `b45f88dc…cbb9e02f` | ✅ matches — **17 parameters, unmoved across all six phases of Stages 2 and 3** |
| `research/ARCHITECTURE.md` | `a8a358f4…9b6d4408` | ✅ matches the 3.3-discuss re-pin |
| `ROADMAP.md` | `aec7d0ce…0137ee29` | ✅ matches the 3.3-plan re-pin |

**No drift on arrival.** `ROADMAP.md` **and** `research/ARCHITECTURE.md` are then **both amended at
this boundary** — see §Contract amendments. `BRIEF.md` and `parameter-spec.md` are not touched and
their pins are unmoved.

### Carried obligations, and their disposition here

| Carried from | Obligation | Disposition at this boundary |
|---|---|---|
| 2.1 verify onward, widened by **every** phase since | **CI gap** — 92 C++ probes and both JS gates are local-only under `-DOUARICON_BUILD_TESTS=ON`; a JUCE bump ships silently | **TAKEN as 4.1 work — D6.** And see §The residual nobody owned |
| 2.2 verify | **`COMPAT-04` retroactive acceptance criteria** — the only summary row with no criteria section | **TAKEN as 4.1 work — D8.** Criteria derived below, from shipped source |
| 3.1 discuss (D2), unchanged since | **D5 / QUAL-01 criterion 2's *audible* clause** — the ~15 min Logic session | **TAKEN as 4.2 work — D2**, and its premise is **corrected**: it does not need a hall |
| 3.3 verify | **Gate 13's interactive half** — ~15 min human, synthetic clicks return `-25208` here | **TAKEN as 4.2 work — D9** |
| 3.3 verify | **Q5 — a 30 Hz meter poll against a HIDDEN WKWebView.** Unrun by four consecutive phases | **TAKEN as 4.2 work — D9.** The blocker is now removed: it needed *a human with signal running*, which the desk rig supplies |
| 3.3 verify | **Locks and file I/O in `processBlock`** — grep + inspection only; `-fsanitize=realtime` unsupported by Apple clang 17.0.0 | **Unchanged, and deliberately not closed.** See D10 |
| 3.3 verify | **Windows** — no MSVC compiler has ever seen this code | **TAKEN as 4.1 work — D3** |
| Stage 3 close | **The parting rule:** *when an amendment corrects a claim, grep the other contracts for the same claim before closing* | **APPLIED — and it fired immediately.** See D7 |

### Numbering

The D-series **restarts at D1 each stage** (Stage 2 ran its own D1–D5; Stage 3 ran D1–D28 across its
three phases). **Stage 4 restarts: 4.1's decisions are D1–D10.** The P-series and the C++ probe
letters **do not restart** — Stage 3 closed at **P85** and probe **CN**, so 4.1's first plan decision
is **P86** and its first new probe is **CO**.

---

## Discussion Summary

Stage 3 closed with **40 criteria across nine rows, zero partials**, and left a
ledger that is unusually honest about what it did *not* do. Stage 4 is that ledger, and nothing else.
There is no new DSP, no new UI, and no new parameter — `parameter-spec.md` has not moved in six
phases and will not move here.

What is left divides on one axis and one axis only: **can it be closed without a host and without an
ear?** Nine items can. Five cannot. That division is not a matter of taste — it is a hard dependency,
because the human items should be run against a binary the machine items have already frozen.
Running them as one pass inverts that.

The stage's own risk is different in kind from Stages 2 and 3. Those stages' risk was *a green
assertion that is not looking* — the vacuity discipline, caught five times in five shapes. **Stage
4's risk is a residual that nobody owns**: an item carried in prose across four verifies, never
written into an acceptance list, and therefore never verified against. One such item was found at
this boundary within minutes of applying Stage 3's own parting rule. §The residual nobody owned
treats that as the stage's defining hazard rather than as a one-off.

---

## Decisions

| # | Decision | Choice | Rationale |
|---|---|---|---|
| **D1** | Stage 4 phase structure | **Two phases: 4.1 machine, 4.2 host-and-ear** | ROADMAP said "single pass". The work splits on a hard dependency, not a preference — 4.2 should run against a binary 4.1 has frozen. A split also means a scheduling delay on the Logic session blocks 4.2 only, not the stage |
| **D2** | Hall access | **Logic + an 8-channel interface at the desk. No hall.** | And the cost is **smaller than the ROADMAP goal line implied** — see below |
| **D3** | Windows scope | **In scope. Wire the Windows VST3 job + MSVC pattern scans at 4.1** | No MSVC compiler has ever seen this code. Two repo patterns are live candidates (`critical_msvc_constexpr_lambda_capture`, `critical_msvc_safepointer_init_capture_nested_lambda`). Finding them now costs a CI run; finding them after release costs a release |
| **D4** | Ship target | **Verified + installed locally. No packaging, no signing, no public release** | Publishing becomes a separate, deliberate step. Note the repo's three public-release readiness steps (4, 11, 14) are still open, so this is also the honest target |
| **D5** | Factory preset scope | **5–6 presets, room-character axis only** — `width`, `rolloff`, `blur`, `hullAtten`, `airAmount`, `outputGain` | Position is per-cue automation; the 8 weights are already FUNC-06's scenes. A preset writing either puts **two mechanisms on the same parameters**. Authored in dB/metres through `convertTo0to1` (`pattern_factory_preset_normalized_ignores_skew`) |
| **D6** | CI scope | **92 C++ probes on macOS + the Windows VST3/pluginval job. The two JS gates stay local-only** | The C++ probes are hermetic console apps with exit codes — exactly what CI is good at. The JS layout gates are **DPR- and viewport-sensitive** (`pattern_tooltip_clamp_gate_viewport_sensitive`), and a flaky gate that gets muted is worse than a local one that is named |
| **D7** | **The retired R2 prediction, still live in three contracts** | **Amend `ARCHITECTURE.md` §3.2.2 + §R2 and two `ROADMAP.md` bullets now** | Found by applying Stage 3's parting rule. Detail below — this is the stage's first instance of its own defining hazard |
| **D8** | `COMPAT-04` criteria | **Derived at this boundary, from shipped source, before any 4.1 implementation exists to shape them** | The habit Stage 3 established for its five criteria-less rows. Derived below |
| **D9** | Gate 13 interactive + Q5 | **Both fold into 4.2's single session** | Both need a running host and a human. Q5 additionally needs *signal*, which is exactly what the desk rig adds |
| **D10** | RT-safety: locks and file I/O | **Stays grep + inspection. Not closed, not claimed closed** | `-fsanitize=realtime` is unsupported by Apple clang 17.0.0. Allocation is now measured **soundly** (3.3 verify fixed the process-wide counter), which makes this gap **sharper, not smaller** — and that is the reason to keep stating it |

### D2 — what "no hall" actually costs, named rather than hidden

`ROADMAP.md`'s Stage 4 goal line read *"Logic Pro, **in the hall**, with the bounce path confirmed."*
That line overstated the dependency, and it is corrected at this boundary.

Walk the list. **Every** Stage 4 criterion closes at a desk with an 8-channel interface:

- The bounce-order and LFE-gain tests are **file-based** — you read the interleaved bounce, you do
  not listen to it.
- Verify-ping's "8 outputs reach distinct physical channels" needs 8 *physical outputs*, which the
  interface is.
- Automation writability, the negotiated-set read, Gate 13 and Q5 are all screen-and-host.

`D5`'s **only unique coverage is QUAL-01 criterion 2's *audible* clause**, and that clause's open
question is specific and small:

> *Does ~15 % of an 8 kHz component, delivered as a **one-sample step** on a **single deliberate
> hull-crossing gesture**, tick audibly on HF-rich material?*

That is a one-gesture, any-monitoring judgement. It does not need a room. Everything measurement
could settle about it **is** settled — the entry edge is bit-exact at 1 kHz and 8 kHz, the exit step
matches its prediction `A·|H_20k(f) − 1|` to **0.000 %**, and the DC path is continuous across 12
crossings, all with a negative control that blows the step 33×.

**What a hall would add is spatial-coherence judgement — and no requirement row asks for it.** There
is no row that says "sounds convincing in a real room". Stating that plainly is the point: if a hall
session later reveals something, it will be a **new** finding, not a criterion this stage skipped.

**If the H2 gesture does tick**, the lever is `RESEARCH-2.3` H3 — raising `fc(d_hull = 0)` toward
Nyquist. That re-tunes the entire musical air curve, so it is a **discuss-boundary change, not a
fix**, and it would open a v1.0.1 cycle rather than being patched inside 4.2.

---

## Contract amendments taken at this boundary

Two contracts move. Both because a **measurement or an observation disqualified what the contract
said**, which is the only reason Stages 2 and 3 ever moved a pin either.

### D7 — the R2 prediction was retired at Phase 2.1 and three contracts never heard

Stage 3's parting rule was: *when an amendment corrects a claim, grep the other contracts for the
same claim before closing.* Applied here as the first act of Stage 4, it fired immediately.

At the **Phase 2.1 manual gate (2026-08-11)** Logic negotiated plain **`create7point1()`** and all 8
surround-meter lanes moved. This **contradicts the Stage-0 R2 prediction of 7.1-SDDS**. The
retirement was written into **`REQUIREMENTS.md` COMPAT-02 and nowhere else.** Three further phases
ran. The grep at this boundary found the prediction still stated as **live** in three places:

| Site | What it still said |
|---|---|
| `ARCHITECTURE.md` §3.2.2 | *"NEW FINDING — Logic **may** negotiate 7.1 (SDDS), not plain 7.1"* |
| `ARCHITECTURE.md` §R2 | *"Logic negotiates a different 8-channel set than expected (**HIGH**)"* |
| `ROADMAP.md` :401-402 | *"…this is risk R2 and **Stage 4 is where it is settled**"* |
| `ROADMAP.md` :472-474 | Known challenge 2 — *"**Settled at Stage 4**"* |

**Why this one mattered more than a stale sentence.** `ROADMAP.md`:401 is not prose — it is a
**Stage 4 acceptance criterion**. Verified as written, 4.2 would have set out to *settle* a fact
settled nine days earlier, against a premise the evidence had already overturned. That is the same
family as the defect Stage 3 caught five times, one level up: **an acceptance criterion that is not
looking at the current state of the world.**

**Does the 2.1 observation carry to the shipping binary?** Checked, not assumed. The function that
determines what Logic is offered — `OOctagonProcessor::isBusesLayoutSupported()` — was extracted from
commit `a47cef88` (the commit the observation was made at) and from the working tree, and diffed:

> **`IDENTICAL` — 31 lines, byte-for-byte.** Nothing since Stage 2 can have changed what Logic sees.

**What the amendments say.** The *analysis* stands — `kAudioChannelLayoutTag_Emagic_Default_7_1` is
real and the three-container mitigation **stays shipped**, because accepting all three costs three
lines and is precisely what makes one observation safe to rely on rather than merely true on one
machine. The *prediction* is retired. **Stage 4 confirms; it does not settle.**

**And 4.2 gains one genuinely unobserved thing in its place:** whether Logic's choice is **stable
across session recall**. 2.1 observed a fresh instantiation only. A host that renegotiates on reopen
is a real failure mode and nobody has looked at it.

### D1 / D2 / D5 / D6 / D9 — the ROADMAP Stage 4 section is restructured

`ROADMAP.md` §Stage 4 is split into 4.1 and 4.2, the goal line's "in the hall" is corrected per D2,
the summary table's *"single pass"* becomes *"4.1, 4.2"*, and **four criteria that existed only as
carried prose are written into the list for the first time**: CI wiring (D6), `COMPAT-04`'s criteria
(D8), Gate 13's interactive half and Q5 (D9), and D5's audible clause (D2).

### Re-pinned checksums

| Contract | New SHA-256 | Status |
|---|---|---|
| `BRIEF.md` | `697a4f32890d7420…` | **unmoved** — and now unmoved across seven consecutive phases |
| `parameter-spec.md` | `b45f88dc5017ec2c…` | **unmoved.** 17 parameters; Stage 4 adds none |
| `ROADMAP.md` | `dbb0dd5796b07ba2…` | **re-pinned** (was `aec7d0ce…`) |
| `research/ARCHITECTURE.md` | `2806c788092d9ec9…` | **re-pinned** (was `a8a358f4…`) |

---

## The residual nobody owned — this stage's defining hazard

Stage 2's hazard was the silent channel map. Stage 3's was the vacuous assertion. **Stage 4's is the
residual carried in prose and never promoted to a criterion.**

The CI gap is the specimen. It has been stated at **every verify boundary since Phase 2.1** — five
consecutive times, each time noting it had *widened* — and it appeared in **no acceptance list
anywhere**, including `ROADMAP.md`'s own Stage 4 section, which is the document that decides what
Stage 4 is. A residual restated five times and owned by no criterion is functionally a residual that
was dropped; it survived only because the same person kept re-typing it.

The R2 prediction (D7) is the same shape from the other side: a fact **corrected** in one document
and left standing in three, one of which was an acceptance criterion.

**The rule this stage adopts, and the one 4.1's plan must enforce:** *every item on the "carried
forward" list is either a checkbox in `ROADMAP.md` §Stage 4 or an explicitly recorded deferral with
a named owner phase. Prose is not a third option.* The four promotions above discharge the backlog;
the rule prevents the next one.

**Applied now**, the carried list closes out as:

| Carried item | Now |
|---|---|
| CI gap | ✅ criterion, 4.1 (D6) |
| `COMPAT-04` criteria | ✅ criterion, 4.1 (D8) |
| Gate 13 interactive | ✅ criterion, 4.2 (D9) |
| Q5 hidden-WKWebView poll | ✅ criterion, 4.2 (D9) |
| D5 audible clause | ✅ criterion, 4.2 (D2) |
| Windows | ✅ criterion, 4.1 (D3) — was already listed |
| Locks / file I/O in `processBlock` | ⚠️ **explicit deferral, D10.** Named owner: none — blocked on toolchain, not on effort |

---

## `COMPAT-04` — acceptance criteria derived (D8)

> **Derived 2026-08-12 at the Stage 4 discuss boundary**, from shipped source and from
> `ROADMAP.md`/`ARCHITECTURE.md`, **before any 4.1 implementation exists to shape them** — the habit
> Stage 3 established for `FUNC-06` and `UI-02..05`. This is the last of the project's 30 rows to get
> a criteria section, and it has been owed since Phase 2.2 verify.

**The row:** *"Defined, non-crashing behaviour when instantiated on a stereo track (exact policy
resolved at Stage 0)."* Priority `should`. Marked complete at stage-1.

**The shipped policy is SAFE mode** (`PluginProcessor.cpp:193-196`, `:226-239`). Two properties of
how it is written are load-bearing and the criteria must assert both, because either could regress
without a compile error:

1. `isBusesLayoutSupported()` admits `mono()` and `stereo()` output, and this is **load-bearing for
   AU** — JUCE derives `AUChannelInfo = {(1,1),(1,2),(1,8),(2,1),(2,2),(2,8)}` from the predicate,
   and `auval` exercises all six. Narrowing it does not merely drop Standalone support; it changes
   what `auval` tests.
2. `safeMode` is derived in `prepareToPlay()` as the **complement of the three real containers**, not
   as `== mono || == stereo`. Written the second way, a fourth 8-channel container admitted later
   would silently stop raising the banner. **The criteria must assert the complement form, not the
   behaviour on today's set** — asserting only the behaviour passes for both spellings.

**Criteria, written into `REQUIREMENTS.md` at this boundary. The row lands ⚠️ partial, 2 of 3:**

- [x] `isBusesLayoutSupported()` accepts `(1,1)`, `(1,2)`, `(2,1)`, `(2,2)`; `auval` reports all six
      `AUChannelInfo` configs and passes — **met at stage-1 with real evidence.** Re-run at 4.1 on
      the final binary
- [x] Instantiation on a 2-channel output is defined and non-destructive; Standalone opens and stays
      running — **met at stage-1.** 4.1 **adds the render clause** (finite samples, no NaN/Inf
      through the SAFE fold), which stage-1 had no DSP to exercise
- [ ] The SAFE banner is raised on mono/stereo out and **not** on the three real containers, asserted
      through the **complement predicate**, with a negative control admitting a fourth 8-channel set
      that confirms the banner stays down — **NEW, and the only genuinely open one**

### The shape this row turns out to have, which is not the one expected

Entering this boundary, `COMPAT-04` was described — by `STATUS.md`, by the Stage 3 verification and
by the first draft of this document — as *"ticked against prose"*. **Reading the stage-1 evidence
disproves that.** Criteria 1 and 2 were genuinely measured: `auval` exercised all four configs and
Standalone ran on a 2-channel device. The tick was **under-documented, not unearned.**

The actual gap is different and more interesting. **`safeMode` did not exist when the row was
ticked** — it landed at Phase 3.1 (P43), two stages later. So criterion 3 tests code written *after*
the requirement it belongs to was closed.

That is a distinct failure shape from the five Stage 3 catalogued, and it deserves its own name:
**a completed row whose subject matter grew.** No assertion went vacuous and nothing regressed; the
requirement simply acquired new surface after it was signed off, and nothing in the process looks
back. It is worth carrying because O-Octagon has now shipped `safeMode`, the SAFE banner, the
negotiated-set readout and the `mapInvalid` fold — **four behaviours on the stereo/degenerate path,
three of them added after COMPAT-04 closed.**

**Consequence for the ledger:** the project stands at **28 of 30 complete, 1 partial, 1 pending** —
not the 29 of 30 the Stage 3 close recorded. That is a correction to the count, not a regression in
the code, and 4.1 closes both remaining rows' machine half.

---

## `COMPAT-02` — the three criteria already on the row, and what changes

`COMPAT-02` is the **only** requirement row still `pending`, and it already carries three criteria,
all unticked. They stand; two get a correction and one gains a clause.

| # | Criterion | State entering Stage 4 |
|---|---|---|
| 1 | Instantiates on a surround track with 7.1 output | **Observed at 2.1** — Logic negotiated `7.1`, all 8 meter lanes moved. Left unticked because COMPAT-02 verifies at stage-4. **Per D7 this now reads as *confirm*, not *settle*, + the new session-recall clause** |
| 2 | Verify-ping confirms all 8 outputs reach distinct physical channels | Needs FUNC-04 (shipped at 3.2) and 8 physical outputs (the desk rig). **The 2.1 meter check proves negotiation and writability only — all 8 lanes carried identical signal. Do not over-read it** |
| 3 | Automation of `srcX`/`srcY`/`srcZ` and `w1..w8` visible and writable | **Visibility observed at 2.1** — 17 parameters under 5 groups, matching the `auval` clump dump. **Writability not yet exercised per-parameter.** FUNC-06's scenes now write `w1..w8` as eight bracketed gestures, so 4.2 exercises that path too |

The negotiated-set name is **already surfaced** — `Source/ui/public/js/app.js:386` ("the negotiated
output-set name", ROADMAP orphan 6), fed by `PluginEditor.cpp:463`'s status payload. 4.2 reads it off
the screen; it does not have to build it.

---

## Open questions for the research phase

| # | Question | Why it needs research rather than a decision |
|---|---|---|
| **Q1** | How do the 92 C++ probes get invoked from `build-and-release.yml` — one aggregate target or per-suite steps, and under which CMake option and build type? | The probes build only under `-DOUARICON_BUILD_TESTS=ON` and the repo's workflow has **never run a test target for any plugin** (`project_no_unit_test_framework_ci_never_runs_tests`). O-Octagon would be the first, so there is no in-repo precedent to copy |
| **Q2** | What does the render-harness cost in CI wall-clock, and does it need a runner with more than the default resources? | 48 harness probes including block-size-invariance renders. If it is slow enough to make CI painful it gets gated to a separate job, and that is a plan decision that needs a number first |
| **Q3** | Does the Windows VST3 build even compile, and which of the two known MSVC patterns fire? | `critical_msvc_constexpr_lambda_capture` (C3493) and `critical_msvc_safepointer_init_capture_nested_lambda`. Both are *authored against* per the Stage 3 close, but **authored-against is not compiled**. A grep pass before the first CI run is cheaper than the run |
| **Q4** | Does the WebView UI have a Windows story at all? | `critical_webview2_static_linking` and `critical_webview2_runtime_gotchas_windows` — the static-linking define and `withUserDataFolder()`. Missing either gives a **silently blank** WebView that passes pluginval. Bears directly on whether D3's Windows job can claim more than "it compiles" |
| **Q5** | Where do factory presets live and load from for this plugin — the shared `preset-manager` module, or a plugin-local mechanism? | O-Octagon has **no preset file, no preset directory and no preset-manager dependency** today. The venue store (3.2) is a separate mechanism that must not be confused with it — `FUNC-05` is the requirement that they stay separate, and adding a musical preset store is the first time that separation is tested by a *second real store* rather than by a bit-compare |
| **Q6** | What are the 5–6 presets' actual values, in engineering units? | D5 fixes the axis, not the numbers. The numbers should come from the DBAP behaviour actually measured in Stage 2 (`hullAtten` default 1.0 dB/m, the air curve, the blur normalisation), not from taste applied to a slider range |
| ~~**Q7**~~ | ~~Is `VERSION` (not `PLUGIN_VERSION`) set in `CMakeLists.txt`?~~ | **ANSWERED AT THIS BOUNDARY — ✅ correct.** `CMakeLists.txt:15` reads `VERSION 1.0.0`, with the explanatory `# NOT PLUGIN_VERSION` comment beside it. `critical_plugin_version_keyword_ignored_by_juce` does not apply here. Asked because Stage 1 verify greps only for the forbidden keyword's *absence*, and absence-of-wrong is not presence-of-right — but in this case both hold |

---

## Constraints

- **`parameter-spec.md` does not move.** 17 parameters. Stage 4 adds no parameter and no automation
  lane; a factory preset is a set of values, not a new control.
- **No new DSP and no new UI.** If 4.2's D5 gesture ticks audibly, the lever (RESEARCH-2.3 H3) is a
  **discuss-boundary change opening a v1.0.1 cycle**, not a fix inside this stage.
- **4.2 runs against a frozen 4.1 binary.** If a 4.2 finding forces a code change, 4.1's gates re-run
  from a forced full recompile — the discipline every Stage 2 and Stage 3 verify held to.
- **Dual-variant install sweep is mandatory** (`critical_dev_release_variant_shadowing`). Dev
  branding produces a `-dev` bundle; leaving an alternate-variant bundle on disk pins Logic's AU
  registry slot, and 4.2 is the phase where a shadowed AU slot would be misread as a plugin bug.
  Use `./scripts/build-and-install.sh` and check for its `⚠ Sweeping ALTERNATE-variant` warning.
- **The folder name and the CMake target differ, and it is confirmed, not suspected.** Folder
  `plugins/O-Octagon/`; target **`OuariconOctagon`** (`CMakeLists.txt:9`). This is
  `build_script_target_name_vs_folder` — 11 of 37 plugins in this repo differ, and **O-Octagon is
  one of them.** Every CI step, install path and packaging script written at 4.1 must resolve the
  *target*, and artefacts land under `OuariconOctagon_artefacts/`, not `O-Octagon_artefacts/`.
- **pluginval 10 runs 2–3 times per format before any conclusion**
  (`pattern_ci_pluginval10_catches_latent_nan`).
- **No packaging, no signing, no public release** (D4).

---

## Requirements in scope

| Requirement | Priority | Status entering Stage 4 | Phase | Notes |
|---|---|---|---|---|
| `COMPAT-02` — Logic Pro, 8 discrete channels | must | **pending** — the only incomplete row | **4.2** | 3 criteria, all unticked; criteria 1 and 3 partly observed at 2.1 |
| `COMPAT-04` — defined behaviour on a stereo track | should | **⚠️ partial, 2 of 3** *(was recorded complete)* | **4.1** | D8 derives the criteria — the last of 30 rows to get a section. Criteria 1–2 carry real stage-1 evidence; criterion 3 is new because `safeMode` postdates the tick |
| `COMPAT-01` — pluginval VST3 + AU, strictness 10 | must | complete at stage-1 | **4.1** | Re-run on the final binary. Complete ≠ still true after six phases of change |
| `QUAL-01` — no artifacts | must | complete at 2.3, **audible clause bounded not concluded** | **4.2** | The ✅ is honest — every *measurable* clause is met. D5 addresses the named residual |

**Ledger entering Stage 4: 28 complete · 1 partial (`COMPAT-04`) · 1 pending (`COMPAT-02`)** — a
correction to the Stage 3 close's "29 of 30", made by reading the stage-1 evidence rather than the
summary of it. **Target at stage close: 30 of 30.**

---

## Next Phase

Ready for: **research** phase — Q1–Q7, with Q1/Q2 (CI mechanics) and Q3/Q4 (MSVC + WebView2 on
Windows) as the two that could actually change 4.1's plan shape.
