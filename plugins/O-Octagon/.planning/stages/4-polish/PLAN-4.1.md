# Stage 4 — Polish · Phase 4.1 (machine gates) — Plan

**Plugin:** O-Octagon
**Stage:** 4 of 4 — Polish / Validation · **Phase 4.1 of 2**
**GSD phase:** plan
**Date:** 2026-08-12
**Branch:** `feat/o-octagon` @ `a47cef88` (2.2 / 2.3 and all of Stage 3 uncommitted — **and P100 is about that**)
**Inputs:** `CONTEXT-4.1.md` (D1–D10), `RESEARCH-4.1.md` (N1–N7, Q1–Q6 answered, A1–A4 scheduled)
**Closes:** `COMPAT-04` (criterion 3 + criterion 2's render clause) and re-confirms `COMPAT-01`.
**Ledger target at 4.1 close: 29 of 30** — `COMPAT-02` alone remains, owned by 4.2

---

## Entry Check — contract checksums

Re-run at this boundary before anything else (`pattern_promotion_checksum_pins_replaced_file`).
**All four measured on arrival; all four byte-exact against `RESEARCH-4.1.md`:**

| Contract | SHA-256 on arrival | RESEARCH-4.1 pin | Result |
|---|---|---|---|
| `BRIEF.md` | `697a4f32…f6b9fbd6` | `697a4f32…` | ✅ unmoved — **nine consecutive phases** |
| `parameter-spec.md` | `b45f88dc…cbb9e02f` | `b45f88dc…` | ✅ unmoved — 17 parameters; 4.1 adds none |
| `research/ARCHITECTURE.md` | `2806c788…57bceb17` | `2806c788…` | ✅ the 4.1-discuss pin |
| `ROADMAP.md` | `dbb0dd57…18fa6da9` | `dbb0dd57…` | ✅ the 4.1-discuss pin |

**`ROADMAP.md` is then amended at this boundary and re-pinned** — A1, A3 and A4, exactly the three
`RESEARCH-4.1` §9 scheduled. `REQUIREMENTS.md` takes A3's other half. **A2 is source, not a
contract**, and lands in Task 1 alongside the workflow it describes.

| Contract | Superseded | New pin |
|---|---|---|
| `ROADMAP.md` | `dbb0dd57…18fa6da9` | **`90c65131…fa562c92`** |

**The other three are untouched and re-measured byte-exact after the edit:** `BRIEF.md`
`697a4f32…f6b9fbd6`, `parameter-spec.md` `b45f88dc…cbb9e02f`, `research/ARCHITECTURE.md`
`2806c788…57bceb17`. This is the **second** `ROADMAP.md` re-pin in the project.

### And the entry check itself found the stage's hazard a third time

**`STATUS.md`'s live `contract_checksums` block was stale on both moving pins.** It still held
`architecture: 32a85018…` and `roadmap: 643471ba…` — the **Stage 3.3** values — while the discuss and
research boundaries had re-pinned twice. Both re-pins were recorded in the *artifacts* and in neither
case in the *ledger*.

Worse, and this is the finding: **`CONTEXT-4.1.md`'s Entry Check ticked `ARCHITECTURE.md` "on
arrival" as `a8a358f4…9b6d4408` ✅ — a value `STATUS.md`'s own `architecture_checksum_superseded`
list records as *retired at 3.3 discuss*,** and which `PLAN-3.3.md`'s entry table contradicts (it
records the live pin as `32a85018…`, agreeing with `STATUS.md`). The check that exists to catch a
gate pointing at the wrong referent **pointed at the wrong referent and reported green.**

**Three instances of one family at one stage boundary:** D7 (a retired prediction left live in three
contracts), A3/N6 (an acceptance criterion stating its outcome backwards), and this. Stage 4 declared
its hazard as *the residual nobody owned*; the truer statement after this boundary is **the claim
nobody re-derived** — a residual, a criterion and a checksum are three surfaces of it.

**Disposition, taken now:**

- `STATUS.md`'s block is corrected to the **measured** values and both supersessions are recorded.
- The discrepancy is written into `STATUS.md` as `checksum_referent_discrepancy_4_1_discuss`,
  **owner 4.1 verify** — not as prose. (Stage 4's own rule.)
- **It cannot be settled from git**, and that is the actionable part. The only commit touching either
  contract is `12ae50dd` (Stage 0), whose blobs hash `bff8a83b` (ARCHITECTURE) and `aec7d0ce`
  (ROADMAP) — the oldest entry of each superseded list, which is at least a clean corroboration of
  the chain's start. **Every re-pin since lives only in the working tree.** Six phases of source and
  five stages of contract amendments are uncommitted. See **P100**.
- The superseded `dbb0dd57…18fa6da9` entry is recorded **elided, head and tail only**, because the
  full 64 digits were never written down and the file has since changed. A reconstructed-looking full
  hash there would be this stage's defect one step worse: wrong *and* verifiable-looking.

### Numbering

The D-series belongs to discuss (4.1 ran D1–D10). **The P-series continues from P85: 4.1 holds
P86–P100.** C++ probe letters continue from **CN** — 4.1 holds **CO, CP, CQ** (three probes,
92 → **95**). **JS gate sections do not move: 69, unchanged.** 4.1 touches no page, no stylesheet and
no `js/` module; the only editor edit is the body of one native-function lambda.

---

## Goal

**Freeze a binary that a machine has finished arguing with, and leave nothing carried in prose.**

4.1 is not a feature phase. Nine items close here and they divide into three kinds:

1. **Two requirement halves** — `COMPAT-04` criterion 3 and criterion 2's render clause, plus a
   re-run of `COMPAT-01` on the final binary.
2. **One residual owned by nobody for five verify boundaries** — the CI gap. It becomes a workflow
   file with a run URL, or it does not close.
3. **Ship-readiness that has no requirement row but blocks 4.2** — factory presets, docs, install,
   and a binary identified by a commit rather than by a working tree.

### What "works" means concretely at 4.1

- **A push to this branch runs 92 probes on a GitHub runner and goes red when they fail.** Not a
  local run described in a summary — a URL.
- **An MSVC compiler parses this code for the first time**, and a Windows VST3 passes pluginval 10.
- **The SAFE-mode predicate is asserted as a *form*, not as a behaviour on today's five sets** — the
  only spelling difference that matters is invisible to every probe that exists today.
- **Six factory presets load without moving the source or the scene** — and the probe that proves it
  asserts the *eleven untouched parameters*, because asserting the six that moved passes with the
  bug present.

### The four findings this plan must not lose

1. **N3 — two live contracts contradicted each other, one day apart.** `ROADMAP` §4.1 named
   `build-and-release.yml`; `CMakeLists.txt:172-176` (P13) forbids it. Resolved at **P86**, not
   inherited.
2. **N5 — `applyPresetJson` resets all 17 parameters first.** Scoping a preset's *keys* does not
   scope its *effect*. D5's intent survives; its mechanism is replaced (**P92, P93**).
3. **N6 — `COMPAT-04` criterion 3 asserted the failing behaviour.** Corrected in two contracts here,
   and in a **third site nobody greped** — the shipped source comment (**P90**).
4. **N1/N2 — the CI gate is the COMPILE, not the run.** Probe C's Layer-2 comparison is a
   `static_assert` against a golden generated from parsed JUCE source. A JUCE bump that reorders
   `AudioChannelSet` fails to *build*. The job must build, not run a cached binary (**P86**).

### The premise correction that shapes the phase

`RESEARCH-4.1` opens by calling `COMPAT-04` criterion 3 "the only genuinely open one". **Two thirds
of it already ships**: probe **BM** (`tests/render-harness/main.cpp:3807-3850`, Phase 3.1) drives all
five layouts through a real `prepareToPlay` and asserts `isSafeMode()` against each. It was invisible
because the row had no criteria section until 2026-08-12.

So the remaining work is **one probe, not three** — and BM's existence changes what that probe has to
be. BM proves the **wiring**; it cannot prove the **form**, because it reaches the predicate only
through `prepareToPlay` and therefore can never present a set `isBusesLayoutSupported()` rejects.
**CO proves the form; BM proves the wiring; neither alone is non-vacuous.** That pairing is the
phase's one piece of real test design.

---

## Requirement staging — read this before writing the verify report

| Row | Entering 4.1 | 4.1 does | At 4.1 close |
|---|---|---|---|
| `COMPAT-04` | ⚠️ partial, 2 of 3 | criterion 3 (**CO** + **BM**), criterion 2's render clause (**CQ**), criterion 1 re-run on the final binary | ✅ **complete, 3 of 3** |
| `COMPAT-01` | complete at stage-1 | pluginval 10 ×3 per format + `auval` on the final binary | ✅ complete, **re-confirmed** |
| `COMPAT-02` | pending | **nothing.** Needs a host | pending → **4.2** |
| `QUAL-01` | complete, audible clause bounded | **nothing.** Needs an ear | complete → 4.2 closes the clause |

**Ledger: 28 complete · 1 partial · 1 pending → 29 complete · 1 pending.** `COMPAT-02` is the only
row 4.2 must close, and it carries three criteria.

### `COMPAT-04` — how each criterion closes

| # | Criterion (as amended at this boundary) | Closed by |
|---|---|---|
| 1 | `isBusesLayoutSupported()` accepts `(1,1)`, `(1,2)`, `(2,1)`, `(2,2)`; `auval` reports all six `AUChannelInfo` configs and passes | stage-1 evidence + **Gate 5 re-run on the final binary** |
| 2 | Instantiation on 2-channel output is defined and non-destructive **+ the render clause**: finite samples, no NaN/Inf through the SAFE fold | stage-1 evidence + **CQ** |
| 3 | The banner is raised on mono/stereo and not on the three real containers, **asserted through the complement predicate**, with a negative control on a fourth 8-channel set confirming the banner **is raised** | **CO** (form) + **BM** (wiring) + **NC1** |

> **Criterion 3's discriminator sentence was corrected at this boundary (A3 / P90)** in
> `REQUIREMENTS.md` and `ROADMAP.md` §4.1. It read *"confirms the banner **stays down**"*, which is
> the outcome of the `== mono || == stereo` spelling D8 explicitly rejects. Under the shipped
> complement form a fourth container is not one of the three real rigs, so `safeMode` is **true** and
> the banner goes **up**. **Verified as written, the criterion passed only if the code carried the
> defect the criterion existed to exclude.**

---

## Plan Decisions

### P86 — The probes run in a **new secretless `.github/workflows/ci-tests.yml`** *(A1, A2, N3)*

`RESEARCH-4.1` N3 found two live contracts in direct contradiction, one day old:
`ROADMAP` §4.1 bullet 1 said the probes run in `build-and-release.yml`; `CMakeLists.txt:172-176`
(PLAN-2.1 **P13**) says *"DO NOT add these to `.github/workflows/build-and-release.yml`"*. It is the
repo's only workflow, so D6 could not be satisfied without resolving one of them.

**Resolution: a new file. P13 is satisfied, not overturned.**

The standing rule in that workflow's header (`:8-33`) forbids two *triggers* —
`pull_request_target`, and any `pull_request` carrying secrets — because eight Apple signing secrets
live there. A new workflow with **no secrets** and `permissions: contents: read` is orthogonal to
that rule, not an exception to it. And it delivers D6's stated failure mode better than the original
destination could: *"a JUCE bump ships silently"* — **a JUCE bump is a commit, not a tag**, so a
tag-time gate catches it only at release.

**The four mechanics, all measured in research and all load-bearing:**

1. **`cmake --build` the targets — do not run a cached binary.** N2: the Layer-2 tripwire is a
   compile-time `static_assert` against a golden header generated from parsed JUCE source. **The
   build failing IS the gate.** A job that only executes binaries discharges nothing.
2. **`-DSKIP_PLUGINS=` every plugin except O-Octagon.** `OUARICON_BUILD_TESTS` is a **shared option
   name across 12 plugins**; a full-tree configure with it ON enables eleven other harnesses that
   have never run in CI. The Windows `validate_only` path (`:518-530`) already contains the exact
   loop to copy.
3. **Single-arch.** Omit `CMAKE_OSX_ARCHITECTURES` — universal roughly doubles the measured 167 s of
   CPU for nothing.
4. **Never pass `OUARICON_RELEASE=ON`.** The harness hard-codes the **dev** codes
   (`JucePlugin_ManufacturerCode=0x4f754476` = `OuDv`), which all 48 probes were authored against.
   Dev is the default, but the root `CMakeLists.txt:16-21` force-OFF **does not apply in CI**
   (`$ENV{CI}` is set), so this must be a deliberate omission, not a reliance.

**Cost, measured not estimated (N1):** configure 7.8 s, clean build of both targets 27.6 s wall /
167 s CPU on 16 cores with **zero warnings**, all 92 probes in **0.39 s**. Extrapolated to a 3-core
`macos-14`: **~3–4 min**. Q2's "does the harness need its own job" contingency is **dead** — the
render harness was the suspect and it runs in 0.37 s.

**A2 lands in the same commit.** `CMakeLists.txt:172-176` says the residual "is logged as a
repo-level todo instead". After this it is not, and a comment that describes a resolved gap as open
is the same defect class as D7.

### P87 — The JUCE version is **derived from one file**, and the mechanism is an output, not an `env` *(§1.4)*

A second `JUCE_VERSION` literal in a second workflow is
`pattern_test_fixture_mirrors_drift_silently`, and the drift is silent **in exactly the direction
that matters**: bump the release workflow, and the probe workflow keeps proving the *old* JUCE green.
That is worse than no gate — a gate reporting green about a JUCE nobody ships.

**Single source: `.github/juce-version.txt`, containing `8.0.14` and nothing else.**

**Correction to `RESEARCH-4.1` §1.4's costing.** It calls this "one-line `env` change in the secrets
workflow". It cannot be: a workflow-level `env:` block cannot read a file. The mechanism is:

- `parse-tag` (already exists, `ubuntu-latest`, already has outputs, and **both build jobs already
  `needs: parse-tag`**) gains a `juce_version` output read from the file.
- The **two** consumption sites — `:128` (macOS) and `:493` (Windows) — switch from
  `${{ env.JUCE_VERSION }}` to `${{ needs.parse-tag.outputs.juce_version }}`; the `env:` entry is
  deleted.
- `ci-tests.yml` reads the file directly in its first step.

**No trigger change, no permission change, no new job.** Three touched lines in the secrets workflow.

**Gate 9 is the whole point:** after this, `grep -rn "8\.0\.14" .github/` returns **exactly one**
hit, and it is `juce-version.txt`.

### P88 — The Windows job lives in `ci-tests.yml`, and the alternative would have broken D4 *(§3.3)*

The mechanism D3 asked for already exists — `workflow_dispatch` with `validate_only: true`, added
for O-Contrabass. **But `build-macos` has no `validate_only` guard** (`:113-118`), deliberately: its
comment records that the validate run doubles as the signing-secrets gate. So dispatching for
O-Octagon **signs, notarises and builds a PKG**. Nothing is published (`create-release` is guarded at
`:651`), but signing and packaging is precisely what **D4 excludes**.

The third option — adding `if: validate_only != 'true'` to `build-macos` — changes shared-workflow
semantics that a comment says are intentional. **Do not.**

So: a `windows-latest` job in the new file. It needs WebView2 NuGet (pinned `1.0.1901.177`, already
proven at `:508-511`), the same `SKIP_PLUGINS` containment, `resolve-target.sh cmake-target` →
**`OuariconOctagon`**, and pluginval 10 with the 600 s timeout the existing job justifies against a
cold WebView2 first-open.

**What this job can and cannot claim, stated so the verify report cannot overclaim:**

- **Can:** the code compiles under MSVC; the VST3 loads; pluginval 10 **opens the editor**, so a
  silently-blank WebView surfaces as an Editor-Automation failure or timeout rather than passing
  green.
- **Cannot:** that the UI is *correct* on Windows. No human will see it this milestone. **Named
  deferral, owner none — blocked on hardware.**

**And the Windows risk is not what the ROADMAP said it was.** N4's scan found **zero** non-static
`constexpr` in any lambda in `Source/**` and **zero** occurrences of `SafePointer` anywhere in
`Source/` — not "authored against", *absent*. The portability scan is equally clean. The bullet is
amended accordingly (A1's second half). The realistic first-run candidates are ones grep cannot
pre-clear: `/permissive-` two-phase lookup in the templated probe helpers, `min`/`max` macros from
`windows.h`, C4244/C4267 under `juce_recommended_warning_flags`, and MSVC's stricter `constexpr`
evaluation in the `static_assert`s in `HullProcessor.h` and `ChannelMap.h`. **A static scan is not a
compile — that distinction is the whole reason D3 exists.**

### P89 — **No `paths:` filter on `ci-tests.yml`**

Tempting, and wrong here. A paths filter is a gate that can **silently not run**, which is this
project's most-caught defect shape wearing a CI costume: a green check mark that means "skipped".
The job costs 3–4 minutes. Run it on every push and every PR.

### P90 — A3's correction lands in **three** places, and the third is code

Two are contracts and are amended at this boundary. The third was found by applying Stage 3's parting
rule *again*, to source this time:

`PluginProcessor.cpp:229-232` — *"Written as the complement of the three REAL containers rather than
as `== mono || == stereo`, **so a future fourth 8-channel container admitted by
isBusesLayoutSupported() cannot silently start raising the banner**."*

Under the complement form, a fourth container is exactly the case that **does** raise the banner —
that is the design, and it is correct, and it is what `REQUIREMENTS.md` note 2 says ("written the
second way … would silently **stop** raising the banner"). The comment names the outcome backwards.
**The code is right; the sentence beside it is not**, and it is the same inversion as criterion 3's,
in the file the criterion is about.

**This matters more than a stale comment normally would, because Task 3 moves these exact lines.**
Extracted unchanged into `RigPolicy.h`, the inverted sentence becomes the doc-comment on the free
function probe CO exists to pin — a backwards claim promoted to the top of the file that defines the
rule. **Restate it in the same edit.** No behaviour changes.

### P91 — `Source/Data/RigPolicy.h`: one header-only extraction, and it changes no behaviour *(§6.3)*

```cpp
namespace oo::rig {
    inline bool isRealRig (const juce::AudioChannelSet& s) noexcept
    {
        return s == juce::AudioChannelSet::create7point1()
            || s == juce::AudioChannelSet::create7point1SDDS()
            || s == juce::AudioChannelSet::create5point1point2();
    }
}
```

`prepareToPlay` becomes `safeMode.store (! oo::rig::isRealRig (outSet), std::memory_order_release);`.

**Why a header and not a TU:** `juce_audio_basics` only, so it is includable from the **unit** target,
whose link line is deliberately narrow (`no juce_audio_processors, no juce_gui_*`) and whose
narrowness is a *structural* property Gate 11 protects. Header-only also means **no CMakeLists edit
on either test target** — the extraction's blast radius is three files.

**Probe CO (unit target)** asserts `isRealRig` is:

- **true** for exactly `create7point1()`, `create7point1SDDS()`, `create5point1point2()`;
- **false** — i.e. the banner *would* be raised — for `create7point1point4()`, `octagonal()`,
  `quadraphonic()`, `mono()`, `stereo()`.

`create7point1point4()` and `octagonal()` are the load-bearing rows: both are **8-or-more-channel
sets that are not real rigs**, and they are what makes this a test of the *complement form* rather
than of today's behaviour. `octagonal()` is a particularly good choice — `isBusesLayoutSupported()`'s
own comment names it as an 8-channel candidate JUCE offers and Logic ignores.

**And a static gate:** the three-container literal must appear **exactly once** in `Source/`. Two
copies is how a predicate drifts away from its call site with every probe still green.

### P92 — Factory preset definitions live in `Source/Data/PresetPolicy.h`, **not** in `PluginEditor.cpp` *(§4.3, Q6)*

Every one of ~20 precedent plugins calls `initializeFactoryPresets()` from the **processor
constructor** with a processor-owned manager. **O-Octagon's manager is editor-owned**
(`PluginEditor.cpp:245`), so there is nothing to copy, and editor-side initialization is *strictly
better* than the precedent: the module's own constructor comment defers directory creation "to avoid
file I/O during AU validation", and an editor-side call keeps **every** write off the headless scan
path — `auval` and pluginval construct processors, not editors.

**But the definitions cannot live in `PluginEditor.cpp`.** That TU is permanently excluded from the
render harness (`JUCE_WEB_BROWSER=0`; the harness CMakeLists comment forbids it by name and §11 of
the static gate greps for it). Definitions written there are **unreachable by any probe** — and the
preset work is the one place in 4.1 where a green result can be wrong.

So: a header-only `Source/Data/PresetPolicy.h` holding two free functions —

- `oo::presets::factoryDefs (const juce::AudioProcessorValueTreeState&)` → the six definitions;
- `oo::presets::loadPreserving (…)` → P93's fix.

It needs `juce_audio_processors` and the module header, both of which the harness already has:
`tests/render-harness/CMakeLists.txt` **already puts `modules/persistence/preset-manager/cpp` on the
include path**, because probes BW/BX already drive `OuariconPresetManager` directly for FUNC-05.
**The scaffold exists.** Naming mirrors `RigPolicy.h` deliberately: both are extractions whose reason
for existing is that a probe must reach the shipped rule.

> **Constraint:** the **unit** target must never include `PresetPolicy.h`. Gate 11 checks the link
> line; this is the human half of the same rule.

### P93 — The N5 fix: **snapshot-and-restore at O-Octagon's call site**, and nowhere else *(§5.4)*

`OuariconPresetManager.h:315-331` (WR-01, module v1.0.3) unconditionally resets **every** parameter
to its default before applying anything. WR-01 exists for a good reason
(`pattern_preset_apply_needs_reset_to_defaults`). The consequence D5 never drew: **omitting the
eleven non-room-character keys does not leave them alone — it resets them.** `srcX`/`srcY` → 0.5,
`srcZ` → 0 m, `w1..w8` → 1.0. Loading any factory preset would re-centre the source and un-do
whatever FUNC-06 scene is applied — *the exact two-mechanism collision D5 was written to prevent*,
arriving through the module's defensive behaviour instead of the preset's content.

`PluginEditor.cpp:711` already ruled on where this gets fixed: **"THE FIX IS HERE, AT O-OCTAGON'S
CALL SITE, and never in the shared module."** Nine plugins include that header and seven more carry
vendored copies.

```
loadPreserving (mgr, apvts, name):
    capture the NORMALISED value of the eleven: srcX, srcY, srcZ, w1..w8
    ok = mgr.loadPreset (name)
    write the eleven back with setValueNotifyingHost   // bit-exact: same float in, same float out
    return ok
```

**The eleven, restored; the six, left as the preset set them. 6 + 11 = 17.**

The editor's `loadPreset` native function keeps its **17 gesture brackets** exactly as they are
(3.2's P59) and calls `loadPreserving` inside them — the restore writes land inside a bracket whose
correctness was established at 3.2, so no new bracketing obligation is created.

**Rejected, with reasons:** authoring all 17 keys (same outcome — the eleven still move, just
explicitly); a module opt-out flag (correct in the abstract, but it is a nine-plugin shared header
and the call site is the ruled-on place).

**Static gate:** `presetManager.loadPreset (` appears **exactly once** in `Source/`, inside
`loadPreserving`. That single grep is what stops a future call site bypassing the restore — the same
"one implementation, two consumers" discipline P79 used for `SceneModel`.

### P94 — Probe CP is a **negative control by construction**, or it is the sixth vacuity instance *(§5.5)*

Asserting "the six moved" **passes with the bug present.** CP must:

1. set a **non-default position** (`srcX`, `srcY`, `srcZ`) **and** apply a non-`all` scene, so the
   eleven hold values the reset would visibly destroy;
2. `loadPreserving` a factory preset;
3. assert the **six changed to the preset's values** *and* the **eleven are bit-unchanged**.

**Clause 3's second half is the whole probe.**

**And CP must never use "Concert Default".** Its six values *are* the shipped defaults (§7.2 — that
is the preset's entire point), so after WR-01's reset the six read correct **even if the apply is
stubbed out entirely**. A probe that cannot distinguish "applied" from "reset to defaults" is
vacuous, and this is the one preset in the set that guarantees it. **Use `Distant Field`** — all six
values differ from default, in both directions.

### P95 — The `.factory-version` sentinel is an authoring trap **and** a verification trap *(N7)*

`initializeFactoryPresets` returns early when `Factory/.factory-version` matches
`JucePlugin_VersionString`, and **O-Octagon ships 1.0.0 and stays there** (D4). Two consequences:

- **Authoring:** the second and every later edit to the definitions **writes nothing**. The symptom
  is "my change had no effect", which reads as a code bug and is not one.
  → **`rm -rf ~/Library/O-Octagon/Presets/Factory/` before every authoring iteration.**
- **Verification, the sharp one:** a probe reading the on-disk Factory JSON can be reading a file
  from an earlier iteration and **pass**. → **CP deletes the Factory directory and calls
  `initializeFactoryPresets(factoryDefs(apvts))` itself**, in-probe, before loading anything.

> **`~/Library/O-Octagon/Presets/User/` is never touched by anything in 4.1** — not by a probe, not
> by an authoring iteration, not by the install step. Gate 17 asserts it byte-identical across the
> full gate run.

### P96 — The six presets, in engineering units, converted off the **live** range *(Q6, §7.2, §7.3)*

Six, not five. All six values per preset are engineering units; **none is written as a normalised
fraction.**

| Preset | `width` m | `rolloff` dB/2× | `blur` | `hullAtten` dB/m | `airAmount` | `outputGain` dB |
|---|---|---|---|---|---|---|
| **Dry Point** | 0.0 | 6.0 | 0.00 | 2.2 | **0.00** | 0.0 |
| **Concert Default** | 0.0 | 4.0 | 0.10 | 1.0 | 0.35 | 0.0 |
| **Chamber** | 1.5 | 4.5 | 0.18 | 1.4 | 0.25 | 0.0 |
| **Wide Hall** | 3.0 | 3.5 | 0.35 | 0.7 | 0.55 | −1.5 |
| **Distant Field** | 4.5 | 3.0 | 0.55 | 0.4 | 0.85 | −3.0 |
| **Enveloping** | 6.0 | 3.0 | 0.80 | **0.0** | 0.45 | −2.0 |

**Every value verified in range at this boundary** against `PluginProcessor.cpp:78-113`: `width`
0–6, `rolloff` 3–6, `blur` 0–1, `hullAtten` 0–3, `airAmount` 0–1, `outputGain` −24…+12. **Concert
Default's six are exactly the shipped defaults** (0.0 / 4.0 / 0.10 / 1.0 / 0.35 / 0.0) — verified
against the same lines, which is what makes it "back to neutral in one click" rather than a taste.

**Why six and not five.** The sixth is what puts `hullAtten = 0` on a user-reachable path. Two
presets sit deliberately on **exact-no-op branches** — `airAmount = 0` (bit-transparent by
construction, ROADMAP known-challenge 5) and `hullAtten = 0` (bit-exact unity, probe AV over 201
swept distances) — so **the factory set doubles as a reachability check on both**, exercised from
where a user actually goes rather than only from a probe. Drop `Enveloping` and `hullAtten = 0` is
reachable only by dragging a slider to its endpoint.

**The axis is monotone and that is a constraint, not a description:** `rolloff` falls 6.0 → 3.0 while
`blur` rises 0.00 → 0.80, and `hullAtten` falls with them because a diffuse image under a steep
outside-hull trim fights itself. **`outputGain` only ever trims** (0, 0, 0, −1.5, −3.0, −2.0) — no
preset can clip a rig calibrated on another.

**Conversion is mandatory (§7.3).** `FactoryPresetDef::parameters` is a map of **normalised** values,
so every number above goes through `range.convertTo0to1(...)` read off the **live**
`NormalisableRange` via `apvts.getParameterRange (id)`. All 17 skews are linear today, so the
arithmetic is a subtract and a divide and it is tempting to write the fraction — **do not.** Writing
`0.75f` for `rolloff = 5.25` bakes `3.0–6.0` in forever and silently
(`pattern_factory_preset_normalized_ignores_skew`, `critical_apvts_denormalised_vs_preset_normalised`).

**One note for `NOTES.md`:** presets store `blur` (0–1), **not metres**, so they are venue-portable by
construction — on a rig with a different `rigScale` the same `blur` gives a proportionally different
radius. Correct behaviour, and worth writing down so it is never read as drift. (`rigScale` of the
§OQ4 default venue is **7.9317 m**, so `blur = 0.55` → `r_s = 2.18 m` **there and only there**.)

### P97 — Probe accounting: **CO, CP, CQ** — three new, 92 → **95**

| Probe | Target | Closes | Fails against |
|---|---|---|---|
| **CO** | unit (`main.cpp`) | `COMPAT-04`/3 — the **form** | the `== mono \|\| == stereo` spelling |
| **CP** | render harness | N5's fix — the **eleven bit-unchanged** | a missing restore; a stubbed apply |
| **CQ** | render harness | `COMPAT-04`/2's render clause | NaN/Inf through the SAFE fold |

Unit **44 → 45**, harness **48 → 50**. **JS gates unchanged at 69 sections** — and that is an
assertion, not an omission: 4.1 edits one native-function lambda body and no page, so 42 + 27 must
still hold exactly.

**CQ's scaffold exists.** The harness already constructs bus layouts programmatically (BM) and
already renders under mono/stereo out. CQ prepares the processor mono-in/stereo-out **and**
mono-in/mono-out, renders full-scale signal with parameters at range extremes, and asserts every
sample finite. Short probe, existing machinery.

### P98 — **Five negative controls, declared at plan**, each naming the gate it must make fire

| # | Mutation | Must fire | Proves |
|---|---|---|---|
| **NC1** | `isRealRig` → `s != mono() && s != stereo()` (the rejected spelling) | **CO fails — and BM still PASSES** | The pairing. BM alone cannot discriminate the two spellings; this is the criterion's whole point made executable |
| **NC2** | Delete the restore from `loadPreserving` | **CP fails on the eleven-unchanged clause, while its six-changed clause still passes** | Clause 3's second half is the probe |
| **NC3** | Stub `loadPreserving`'s apply so nothing is written; run CP against `Concert Default` **and** against `Distant Field` | **Distant Field fails; Concert Default PASSES** | P94's preset choice is load-bearing, not incidental |
| **NC4** | Skip CP's delete-then-initialize and hand-edit a stale value into the on-disk Factory JSON | **CP passes on the stale file** | N7's verification trap is real; the delete is what makes CP hermetic |
| **NC5** | Perturb the generated Layer-2 golden header | **The unit target FAILS TO COMPILE** | N2 — the CI gate is the build step, not the run step. Local, no CI run needed |

**NC1 and NC5 are the two that justify the phase's design.** Without NC1, "CO plus BM" is an
assertion about test design; with it, it is a measurement. NC5 is what proves the CI job must run
`cmake --build` rather than a cached binary.

### P99 — `COMPAT-01` re-run: **pluginval ×3 per format, then `auval`, on the final binary**

Complete at stage-1 ≠ still true after six phases of change.
`pattern_ci_pluginval10_catches_latent_nan`: strictness 10, **VST3 ×3 and AU ×3**, all six exit 0
before any conclusion. Then `auval -v aufx OuOc OuDv` — verified codes: `JucePlugin_PluginCode
= 0x4f754f63` = `OuOc`, dev manufacturer `OuDv`.

**Install is `./scripts/build-and-install.sh O-Octagon`**, and the resolver is confirmed:
`cmake-target` → **`OuariconOctagon`**, `product-name` → **`O-Octagon`**, artefacts under
`OuariconOctagon_artefacts/` (`build_script_target_name_vs_folder` — 11 of 37 plugins differ and
O-Octagon is one of them). **Watch for the `⚠ Sweeping ALTERNATE-variant` line**
(`critical_dev_release_variant_shadowing`): 4.2 is exactly the phase where a shadowed AU registry
slot would be misread as a plugin bug.

### P100 — **The freeze is a commit, not a working tree**

D1 splits the stage on a hard dependency: *4.2 runs against a binary 4.1 has frozen.* Today,
**`a47cef88` is the last commit and phases 2.2, 2.3, 3.1, 3.2, 3.3 and all of Stage 4's planning are
uncommitted.** A binary built from an uncommitted tree is not frozen — it is merely current, and
nothing distinguishes "the 4.2 binary" from "whatever was on disk that afternoon".

The entry check proved the cost is already being paid: **the ARCHITECTURE pin discrepancy is
unresolvable precisely because nothing since Stage 0 is committed** (`12ae50dd`'s blobs hash
`bff8a83b` / `aec7d0ce` — the oldest entry of each superseded list, and the last independently
checkable point in the chain). `pattern_uncommitted_improve_versions_lost` names the general risk;
this boundary supplies a concrete instance of it costing something.

**So 4.1 closes with the work committed, and `SUMMARY-4.1.md` records the freeze as:**

- the **commit SHA** the 4.2 binary was built from;
- `shasum -a 256` of the installed `.vst3` and `.component` bundle binaries;
- the probe count and gate results that binary passed (**95 / 0 failures**);
- the `ci-tests.yml` **run URL** for that SHA.

If a 4.2 finding forces a code change, that record is what makes "re-run 4.1's gates from a forced
full recompile" a defined operation instead of a hopeful one.

---

## Tasks

**The ordering is load-bearing in two places.** Task 3 → Task 4 (CO cannot be written before the
header it pins exists), and **Tasks 5–7 must land before Task 11's pluginval runs**, because a
factory-preset write is file I/O on a path pluginval and `auval` exercise. Everything else is
independent.

### Task 1 — `ci-tests.yml` (macOS job) + `.github/juce-version.txt` + A2 *(P86, P87)*

- **Files:** `.github/workflows/ci-tests.yml` (new), `.github/juce-version.txt` (new),
  `.github/workflows/build-and-release.yml` (3 lines), `plugins/O-Octagon/CMakeLists.txt:172-176`
- **Depends on:** none
- The job, in order: checkout → read `juce-version.txt` → download + unzip JUCE for that version →
  `cp -R vendored/JUCE-overrides/modules/. JUCE/modules/` **with the two `grep -q "JUCE-NE-PATCH"`
  fail-fasts retained** → build the SKIP list → configure → **`cmake --build --target
  O-Octagon-geometry-test O-Octagon-render-test`** → run both binaries.
- `permissions: contents: read`. **No secrets.** `on: [push, pull_request]`, no `paths:` filter
  (P89). Pin any third-party action to a commit SHA, matching the house rule.
- The JUCE-overrides copy is retained **even though O-Octagon has no note-expression**: the grep is a
  fail-fast that keeps the two workflows' JUCE **identical**, and an identical JUCE is what makes the
  Layer-2 golden comparable across them.
- `build-and-release.yml`: delete the `JUCE_VERSION` `env:` entry, add the `parse-tag` output, update
  the two consumption sites (`:128`, `:493`). **No trigger and no permission changes.**
- A2: rewrite the `CMakeLists.txt` comment — it must name `ci-tests.yml` and stop describing the gap
  as an open repo-level todo. **Keep the P13 prohibition itself**; it is still true and still the
  reason the new file exists.

### Task 2 — `ci-tests.yml` Windows job *(P88)*

- **Depends on:** Task 1
- `windows-latest`; same JUCE read + overrides; WebView2 NuGet pinned `1.0.1901.177`; same
  `SKIP_PLUGINS` containment; `resolve-target.sh cmake-target` → `OuariconOctagon`; build
  `${TARGET}_VST3`; pluginval `--strictness-level 10 --timeout-ms 600000`; upload the log
  `if: always()`.
- **The C++ test targets are macOS-only** (D6) — MSVC sees the plugin TUs only.
- **Expect first-run failures and treat them as the deliverable.** This is the first MSVC compile of
  ~11 k lines. N4 cleared the two *named* patterns; the candidates in P88 are the ones grep cannot
  pre-clear.

### Task 3 — `Source/Data/RigPolicy.h` + the call site + **the comment restatement** *(P91, P90)*

- **Files:** `Source/Data/RigPolicy.h` (new), `Source/PluginProcessor.cpp:226-239`
- **Depends on:** none
- Header-only, `juce_audio_basics` only. `prepareToPlay` calls it. **Behaviour is unchanged and the
  extraction must be provably so** — BM passes before and after, untouched.
- Restate the inverted sentence (P90). The doc-comment on `isRealRig` states the property the way the
  criterion now does: *a set that is not one of the three is SAFE, and the banner is raised* — which
  is what distinguishes this spelling from `== mono || == stereo`.

### Task 4 — Probe **CO** in `tests/unit/main.cpp` *(P91, P97)*

- **Depends on:** Task 3
- Eight sets asserted: three true, five false, `create7point1point4()` and `octagonal()` among the
  false rows and named in the failure message as the discriminating cases.
- Add the static gate: the three-container literal appears **exactly once** in `Source/`.

### Task 5 — `Source/Data/PresetPolicy.h` — the six definitions *(P92, P96)*

- **Depends on:** none
- `factoryDefs (apvts)` builds six `FactoryPresetDef`s, six keys each, **through
  `apvts.getParameterRange(id).convertTo0to1(engineeringValue)`**. Never a literal fraction.
- The engineering values appear **once**, in a table beside the conversion, in the units of the
  table in P96.

### Task 6 — `loadPreserving` + the editor call site + `initializeFactoryPresets` *(P93, P92)*

- **Files:** `Source/Data/PresetPolicy.h`, `Source/PluginEditor.cpp` (native fn `(8)`, constructor)
- **Depends on:** Task 5
- The eleven captured **normalised** and written back **normalised** — bit-exact by construction.
- The 17 gesture brackets stay exactly where they are; `loadPreserving` is called between them.
- `initializeFactoryPresets (factoryDefs (…))` from the **editor constructor** — never the processor
  (P92: keeps all file I/O off the headless `auval`/pluginval path).
- Static gate: `presetManager.loadPreset (` appears **exactly once** in `Source/`.

### Task 7 — Probe **CP** in the render harness *(P94, P95)*

- **Depends on:** Task 6
- Delete `Factory/` → `initializeFactoryPresets(factoryDefs(apvts))` → set a non-default position and
  a non-`all` scene → `loadPreserving("Distant Field")` → assert **six changed to the converted
  values** and **eleven bit-unchanged**.
- Reuses the BW/BX scaffold — the harness already includes the module header and drives the manager
  directly.
- **Touches `Factory/` only. Never `User/`.**

### Task 8 — Probe **CQ** — `COMPAT-04` criterion 2's render clause *(P97)*

- **Depends on:** none
- mono-in/stereo-out **and** mono-in/mono-out; full-scale input; parameters at range extremes;
  every output sample finite, no NaN/Inf.

### Task 9 — The **five negative controls** *(P98)*

- **Depends on:** Tasks 4, 7, 8
- Each mutation applied, the named gate observed to fire (or, for NC3/NC4, observed to **wrongly
  pass**, which is the point), each reverted, and the tree confirmed byte-identical afterwards.

### Task 10 — Clean build + both test targets, forced full recompile

- **Depends on:** Tasks 3–8
- **95 probes, 0 failures.** Zero `warning:`. Re-run `node tests/ui_frontend_check.js` and
  `node tests/ui_layout_check.js` — **42 and 27 sections, unchanged.**

### Task 11 — `COMPAT-01`: pluginval ×3 per format + `auval` *(P99)*

- **Depends on:** Task 10
- Strictness 10, VST3 ×3, AU ×3, then `auval -v aufx OuOc OuDv`. **All six pluginval runs exit 0
  before any conclusion is drawn.**

### Task 12 — Docs *(§8)*

- **Depends on:** Task 11
- **`CHANGELOG.md` — create.** 37 of 40 plugins have one and `build-and-release.yml:669-681` reads it
  for release notes, falling back to a bare "Release O-Octagon v1.0.0". The fallback *is* what
  shipping without it looks like.
- **`NOTES.md`** — Stage 2–4 content, including P96's venue-portability note.
- **`PLUGINS.md:68`** — stale by three stages (`🚧 Stage 1 … 2026-08-11`).

### Task 13 — Install + dual-variant sweep *(P99)*

- **Depends on:** Task 11
- `./scripts/build-and-install.sh O-Octagon`. **Record whether the `⚠ Sweeping ALTERNATE-variant`
  warning appeared** — its absence is information too, and 4.2 needs to know which.

### Task 14 — `REQUIREMENTS.md`: close `COMPAT-04`, re-evidence `COMPAT-01`

- **Depends on:** Tasks 9, 11
- Every criterion gets its probe letter **and its measured figure**, in the row it belongs to.
- `pattern_evidence_line_orphaned_past_next_heading`: **count `[x]` against `→ **` per section**
  before closing. An evidence line written past the next heading orphans itself, and a still-pending
  row inherits the stray line.
- Update the frontmatter `openRows:` — `COMPAT-04` leaves it; `COMPAT-02` is the only entry left.

### Task 15 — **Commit**, then `SUMMARY-4.1.md` + `STATUS.md` *(P100)*

- **Depends on:** Tasks 1–14
- Commit the six uncommitted phases and this stage's planning. The freeze record in
  `SUMMARY-4.1.md` is **commit SHA + bundle checksums + probe count + CI run URL**.
- The summary states **what did not run** as plainly as what did: the two JS gates in CI, Windows UI
  correctness, RT-safety beyond grep, and everything in 4.2.

---

## Gates

Every gate is **run at execute and RE-RUN FROM SCRATCH at verify**, never read out of
`SUMMARY-4.1.md`. This is the discipline that has caught ten mis-attributions across 2.3, 3.1, 3.2
and 3.3.

| # | Gate | Pass condition |
|---|---|---|
| 1 | Clean 3-format build + both test targets, **forced full recompile** | exit 0, **zero `warning:` / `error:` / `FAILED`** |
| 2 | Both C++ test targets | **95 probes, 0 failures**, exit 0 / exit 0 |
| 3 | `node tests/ui_frontend_check.js` | exit 0, **42 sections — unchanged** |
| 4 | `node tests/ui_layout_check.js` | exit 0, **27 sections — unchanged**, must **not SKIP** |
| 5 | `auval -v aufx OuOc OuDv` | **AU VALIDATION SUCCEEDED**, and the **six** `AUChannelInfo` configs present (`COMPAT-04`/1) |
| 6 | pluginval s10, VST3 ×3 / AU ×3 | all six exit 0, zero `FAILED` |
| 7 | **`ci-tests.yml` macOS job green on a real push** | **a run URL**, not a local simulation. This is the gate that discharges D6 |
| 8 | **`ci-tests.yml` Windows job green** | VST3 builds under MSVC; pluginval 10 exits 0; log uploaded |
| 9 | **JUCE pin is derived** | `grep -rn "8\.0\.14" .github/` returns **exactly one** hit: `juce-version.txt` |
| 10 | 17 params vs `parameter-spec.md`, three sides | **17/17**; **4.1 adds none** |
| 11 | Unit-target link line | **no `juce_audio_processors`, no `juce_gui_*`** — `RigPolicy.h` is header-only and must not have widened it |
| 12 | `createEditor` guard present; `PluginEditor.cpp` absent from the harness target | both ✓ — Task 6 edits that TU, so re-check |
| 13 | Contract checksums | **BRIEF / parameter-spec / ARCHITECTURE unmoved**; **`ROADMAP.md` at its NEW pin `90c65131…`**; **and `STATUS.md`'s block matches the files** — the failure this boundary found |
| 14 | The **five** negative controls | all five behaved as declared; tree byte-identical afterwards |
| 15 | `gen_dbap_reference.py --check` | exit 0, 102 cases — 4.1 does not touch the solver, and this is what proves it |
| 16 | Static gate re-run | the three-container literal **once** in `Source/`; `presetManager.loadPreset (` **once** in `Source/`; §11's forbidden-TU grep still clean |
| 17 | **`~/Library/O-Octagon/Presets/User/` byte-identical** across the whole gate run | ✓ — FUNC-05's separation is structural, and 4.1 is the first phase to write a second real store |
| 18 | Install + dual-variant sweep | both bundles installed; the `⚠ Sweeping ALTERNATE-variant` line **recorded present or absent** |

**Gate 7 is the one that cannot be faked.** Every other gate in this table has been run locally
before by this project. A CI job is green when GitHub says it is green, and D6 has been carried as
prose for five verify boundaries precisely because nobody had to produce a URL.

---

## Execution Constraints

1. **`cmake --build` is the CI gate, not the run.** N2: Layer-2 is a compile-time `static_assert`.
   A job that only executes binaries discharges nothing (NC5 proves it).
2. **Never `OUARICON_RELEASE=ON` in `ci-tests.yml`.** The harness hard-codes the dev codes, and the
   root force-OFF does not apply in CI. Omission, not reliance.
3. **`SKIP_PLUGINS` must exclude all 39 siblings.** `OUARICON_BUILD_TESTS` is a shared option name
   across 12 plugins; eleven other harnesses have never run in CI and are not this phase's problem.
4. **Never edit `modules/persistence/preset-manager/`.** Nine plugins include that header and seven
   more carry vendored copies. `PluginEditor.cpp:711` already ruled on this.
5. **The eleven are asserted bit-unchanged.** A probe that asserts only the six passes with the bug
   present (P94, NC2).
6. **Never author a normalised literal.** Every preset value goes through `convertTo0to1` off the
   live range (P96).
7. **Delete `Factory/` before every authoring iteration and inside CP.** The sentinel silently
   no-ops writes at an unchanged version (P95).
8. **`User/` is never written.** Gate 17.
9. **The extraction changes no behaviour.** BM must pass before and after Task 3, untouched.
10. **`parameter-spec.md` does not move.** A factory preset is a set of values, not a control.
11. **No packaging, no signing, no notarisation, no `workflow_dispatch` of the release workflow**
    (D4, P88).
12. **`juce::String` construction:** any new user-facing string with non-ASCII content is built with
    `<<` onto a **named local** — `juce::String(const char*)` is ASCII-only, and
    `juce::String("...") << x` does not compile (it binds the private `operator bool`).
13. **MSVC:** any `constexpr` inside a lambda is `static`; no `SafePointer(this)` init-capture in a
    nested lambda. N4 says the current tree is clean — **keep it that way in the code written here.**
14. **pluginval ×3 per format before any conclusion.**

---

## Non-goals for Phase 4.1 — must not appear

- **Anything needing a host or an ear.** `COMPAT-02`, Gate 13's interactive half, Q5's hidden-WKWebView
  poll, D5's audible clause. All 4.2, all already checkboxes in `ROADMAP` §4.2.
- **New DSP, new UI, new parameters.** If 4.2's D5 gesture ticks, the lever is RESEARCH-2.3 H3 — a
  **discuss-boundary change opening v1.0.1**, not a fix in this stage.
- **The two JS gates in CI.** D6, and now a **named deferral** — owner none, blocked on headless-render
  determinism, not on effort. 69 sections, green today, gated by a human running `node`.
- **Overturning P13**, or adding any step to `build-and-release.yml` beyond P87's three lines.
- **A `paths:` filter** (P89).
- **Editing the shared preset-manager module** (constraint 4).
- **Claiming RT-safety beyond grep + inspection.** D10 stands: `-fsanitize=realtime` is unsupported by
  Apple clang 17.0.0. Allocation is now measured soundly, which makes this gap **sharper, not
  smaller**.
- **Claiming Windows UI correctness.** pluginval 10 opening the editor is the ceiling of what CI can
  claim (P88).
- **Re-settling R2.** Settled by observation at Phase 2.1; D7 retired the prediction in three
  contracts. 4.2 **confirms**.
- **Rewriting `CONTEXT-4.1.md`'s Entry Check table.** It stands as the record; `RESEARCH-4.1` N6 and
  this document's entry check are its corrections. History is not edited to look correct.

---

## Success Criteria

- [ ] **`COMPAT-04` closed — 3 of 3.** Criterion 3 via **CO** (form) + **BM** (wiring) + **NC1**;
      criterion 2's render clause via **CQ**; criterion 1 re-run on the final binary
- [ ] **`COMPAT-01` re-confirmed** — pluginval s10 VST3 ×3 / AU ×3 all green, `auval` green
- [ ] **Ledger at 29 of 30**, `COMPAT-02` the only open row, owned by 4.2
- [ ] **95 probes, 0 failures**, both targets, forced full recompile, zero warnings
- [ ] **69 JS gate sections unchanged** — 42 + 27, neither SKIPped
- [ ] **`ci-tests.yml` green on a real push, with a run URL** — macOS **and** Windows. **The five-boundary
      CI residual closes here or it does not close**
- [ ] **MSVC has compiled this code** — for the first time in the project
- [ ] **The JUCE pin is derived** — exactly one `8.0.14` in `.github/`
- [ ] **Six factory presets** in engineering units through `convertTo0to1`, both exact-no-op branches
      user-reachable
- [ ] **The eleven bit-unchanged across a preset load**, proven by NC2 rather than asserted
- [ ] **Five negative controls behaved as declared**, tree byte-identical afterwards
- [ ] **`User/` presets byte-identical** across the whole gate run
- [ ] **Contract checksums:** three unmoved, `ROADMAP.md` at `90c65131…`, **and `STATUS.md` agrees with
      the files**
- [ ] **Docs:** `CHANGELOG.md` created, `NOTES.md` current, `PLUGINS.md:68` corrected
- [ ] **Installed, dual-variant sweep recorded**
- [ ] **The work is committed and the freeze recorded** — SHA + bundle checksums + probe count + CI
      run URL

---

## Risks Active in This Phase

| Risk | Signature if it lands | What catches it |
|---|---|---|
| **The SAFE predicate is rewritten as `== mono \|\| == stereo`** | Every probe that exists today still passes; the banner silently stops warning on an unknown container | **CO**. **NC1** proves BM alone cannot see it |
| **A preset load re-centres the source and clears the scene** | Only visible mid-session, in a hall, during a cue | **CP**'s eleven-unchanged clause. **NC2** proves that clause is the probe |
| **CP passes against a stale on-disk preset file** | Green, and reading a file from an earlier iteration | P95's delete-then-initialize. **NC4** demonstrates the trap |
| **A preset value is authored as a normalised fraction** | Correct today; silently wrong the first time a range widens | P96's conversion off the live range; the engineering table is the single source |
| **The CI job goes green without building** | A gate that reports on a cached binary | P86 mechanic 1. **NC5** fails the *compile* |
| **The two workflows' JUCE versions diverge** | The probe workflow proves the old JUCE green — **worse than absent** | P87's single file. **Gate 9** counts the literals |
| **MSVC finds something grep cannot** | First compile of ~11 k lines | Expected, and it is the deliverable (P88). N4 cleared only the two named patterns |
| **A `paths:` filter is added later "to save minutes"** | A green check that means *skipped* | P89, stated as a decision so a future edit is a reversal rather than a tidy-up |
| **The Windows job is satisfied by dispatching the release workflow** | Signs and notarises — **D4 broken silently** | P88, and `build-macos`'s missing `validate_only` guard is documented as deliberate |
| **A carried item lands in prose again** | It survives on re-typing and is verified by nobody | Stage 4's rule: **checkbox or recorded deferral, never prose.** Three deferrals are named in `RESEARCH-4.1` §9 and re-stated in the summary |
| **The 4.2 binary cannot be identified** | "Whatever was on disk that afternoon" | **P100.** The entry check already shows this costing something |

---

## Next Phase

**Ready for:** `execute`

**15 tasks. The two that carry the phase are Task 4 (CO) and Task 7 (CP)** — they are the only two
assertions in 4.1 that a plausible-looking wrong implementation fails. Everything else passes under
both defects. Task 1's gate (a real CI run URL) is the only item that cannot be produced locally,
which is exactly why it went un-owned for five verify boundaries.

**The one ordering constraint:** Tasks 5–7 land before Task 11's pluginval runs — factory-preset
initialization is file I/O, and `auval`/pluginval are the two things the editor-side call site exists
to keep it away from.

**Closing `COMPAT-04` and re-confirming `COMPAT-01` leaves `COMPAT-02` alone in 4.2**, against a
binary this phase has frozen by commit.
