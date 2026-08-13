# Stage 4 — Polish · Phase 4.1 (machine gates) — Research

**Plugin:** O-Octagon
**Stage:** 4 of 4 — Polish / Validation · **Phase 4.1 of 2**
**GSD phase:** research
**Date:** 2026-08-12
**Branch:** `feat/o-octagon` @ `a47cef88` (2.2 / 2.3 and all of Stage 3 uncommitted)
**Answers:** Q1–Q6 in `CONTEXT-4.1.md` (Q7 was answered at the discuss boundary)
**Sources:** the repo's single workflow `.github/workflows/build-and-release.yml`, the root and
plugin `CMakeLists.txt`, `modules/persistence/preset-manager` v1.0.5, O-Octagon `Source/` and
`tests/`, and **four measured runs plus one clean CI-shaped build** performed at this boundary

---

## Entry Check — the four contracts

Re-run before anything else (`pattern_promotion_checksum_pins_replaced_file`):

| Contract | SHA-256 measured now | `CONTEXT-4.1` re-pin | Result |
|---|---|---|---|
| `BRIEF.md` | `697a4f32…f6b9fbd6` | `697a4f32…` | ✅ unmoved — eight consecutive phases |
| `parameter-spec.md` | `b45f88dc…cbb9e02f` | `b45f88dc…` | ✅ unmoved — 17 parameters |
| `research/ARCHITECTURE.md` | `2806c788…57bceb17` | `2806c788…` | ✅ **the new 4.1-discuss pin** |
| `ROADMAP.md` | `dbb0dd57…18fa6da9` | `dbb0dd57…` | ✅ **the new 4.1-discuss pin** |

**All four byte-exact.** Both pins that moved at discuss are measured here at their *new* values,
which is the check that those amendments actually landed.

**No contract is amended here.** Three amendments are **required at the plan boundary** and are
flagged rather than taken, exactly as 3.3 research scheduled its `ROADMAP` amendment: **N3**
(the CI bullet names a forbidden workflow), **N6** (`COMPAT-04` criterion 3 asserts the failing
behaviour, in *two* contracts), and **N5** (D5's preset scope is not what loading a preset does).

---

## Executive summary — what this research changes

Six findings could not have been known at discuss. **Three change a plan decision, two correct a
premise carried in from discuss, and one is a criterion that would verify green against broken
code.** Q2 is answered with numbers rather than an estimate, and it removes a contingency rather
than adding one.

| # | Finding | Consequence |
|---|---|---|
| **N1** | **Q2 measured, and it is small.** Clean CI-shaped configure **7.8 s**, clean build of both test targets **27.6 s wall / 167 s CPU** (arm64, 16 cores), run **0.39 s** for all 92 probes, **zero warnings**. The tests-only build produces `libO-Octagon-dev_SharedCode.a` and two console apps and **does not build the VST3/AU/Standalone bundles at all** — verified, those artefact dirs come out empty | **Q2's contingency is dead.** No separate job, no larger runner. Extrapolated to a 3-core `macos-14`: **~3–4 min end-to-end**. Three cost levers go into the plan (single-arch, `SKIP_PLUGINS`, no `OUARICON_RELEASE`) |
| **N2** | **The real gate is the COMPILE, not the run.** Probe C's Layer-2 comparison is a **`static_assert`** (`tests/unit/main.cpp:123-124`) against a golden header *generated at build time from parsed JUCE source*. A JUCE bump that reorders `AudioChannelSet` **fails to compile** | The CI step must **build** the unit target, not run a cached binary. This is what actually discharges D6's "a JUCE bump ships silently". Also: `vendored/JUCE-overrides` touches neither `juce_AudioChannelSet.h` nor `.cpp`, so the CI-derived golden and the local one should agree — first run is the proof |
| **N3** | **The amended ROADMAP bullet names a workflow the CMakeLists forbids.** `ROADMAP.md` §4.1 (written yesterday, D6) says the probes *"run in `build-and-release.yml` on macOS"*. `plugins/O-Octagon/CMakeLists.txt:172-176` says *"DO NOT add these to `.github/workflows/build-and-release.yml`"* — a Phase 2.1 plan decision (P13). It is the repo's **only** workflow | **A direct contradiction between two live contracts, one day old.** The plan must resolve it, not inherit it. Recommended: a **new secretless `ci-tests.yml`**, which honours both literally and delivers D6's intent. Needs a ROADMAP amendment |
| **N4** | **Q3 answered: neither MSVC pattern fires.** A lexical scan of every lambda body in `Source/**` found **zero** non-static `constexpr`. **`SafePointer` appears nowhere in `Source/` at all** — zero call sites. Portability scan also clean (no `unistd.h`, `<sys/*>`, `M_PI`, `__attribute__`, `alloca`, `ssize_t`) | The pre-CI grep D3 asked for is **discharged here**. The residual Windows risk is *not* those two patterns and the plan must stop calling it that — it is that no MSVC has ever parsed the code. Named candidates in §3 |
| **N5** | **`applyPresetJson` resets ALL 17 parameters before applying.** WR-01, module v1.0.3, `OuariconPresetManager.h:315-331` — unconditional `setValueNotifyingHost(default)` on every parameter. So a six-key "room-character only" preset does not *leave* position and weights alone: it **resets `srcX`→0.5, `srcY`→0.5, `srcZ`→0.0 and `w1..w8`→1.0** | **D5's scope statement is contradicted by the mechanism.** Loading any factory preset **un-does whatever FUNC-06 scene is applied and re-centres the source** — the exact "two mechanisms on the same parameters" collision D5 was written to prevent, arriving through the module's defensive behaviour instead of the preset's content. Fix belongs at O-Octagon's call site (§5) |
| **N6** | **`COMPAT-04` criterion 3 asserts the failing behaviour.** It asks for *"a negative control admitting a fourth 8-channel set that confirms the banner **stays down**"*. Under the shipped **complement** form the banner goes **UP** for a fourth set; "stays down" is what the `== mono \|\| == stereo` spelling D8 rejects would do. **The same wording is in `REQUIREMENTS.md` and `ROADMAP.md` §4.1** | Verified as written, the criterion passes **only if the code has the spelling D8 rejects**. Same shape as D7, one boundary later. Corrected wording and a workable probe construction in §6 |

**And one inversion of the stage's declared hazard.** `COMPAT-04` criterion 3 is not wholly open:
**probe BM already exists** (`tests/render-harness/main.cpp:3807-3850`, Phase 3.1) and drives all
five layouts — mono, stereo, 7.1, 7.1-SDDS, 5.1.2 — through `prepareToPlay`'s complement predicate,
asserting `isSafeMode()` against each. Stage 4's hazard is *a residual nobody owned*; this is its
mirror — **evidence nobody claimed**, invisible because the row had no criteria section until
yesterday. The remaining work is **one** probe, not three.

**Measured, not estimated:** every number in N1, the four gate runs below, and the engineering
tables in §7 (computed from shipped constants: `rigScale` of the §OQ4 default venue is **7.9317 m**).

**What was NOT run, and is not claimed:** no Windows compiler was invoked. N4 is a *static scan*,
and a static scan is not a compile — that distinction is the whole reason D3 exists.

### Gate state on arrival — all four green, today

| Gate | Result | Wall |
|---|---|---|
| `O-Octagon-geometry-test` | **44 probes, 0 failures**, exit 0 | 0.023 s |
| `O-Octagon-render-test` | **48 probes, 0 failures**, exit 0 | 0.370 s |
| `tests/ui_frontend_check.js` | **ALL SECTIONS PASS — 42 sections**, exit 0 | — |
| `tests/ui_layout_check.js` | **ALL SECTIONS PASS — 27 sections**, exit 0 | — |

92 C++ probes + 69 JS sections. This is the baseline 4.1 freezes for 4.2.

---

## 1. CI: how the probes actually get run (Q1)

### 1.1 The contradiction, stated plainly (N3)

Two live contracts disagree, and the disagreement is one day old:

| Source | What it says |
|---|---|
| `ROADMAP.md` §4.1, bullet 1 *(added 2026-08-12, D6)* | "the 44 unit + 48 harness C++ probes run in **`build-and-release.yml`** on macOS" |
| `plugins/O-Octagon/CMakeLists.txt:172-176` *(PLAN-2.1 P13)* | "**DO NOT** add these to `.github/workflows/build-and-release.yml`: it is tag-triggered and secrets-bearing, and carries a standing rule against widening its trigger surface. The residual gap … is logged as a repo-level todo instead" |

`build-and-release.yml` is the repo's **only** workflow. So D6 could not be written without either
overturning P13 or putting the probes somewhere that does not yet exist — and the bullet did neither
explicitly.

**The standing rule itself is narrower than the CMakeLists comment implies.** Reading the workflow
header (`:8-33`), the forbidden things are two *triggers* — `pull_request_target`, and any
`pull_request` carrying secrets — because eight Apple signing secrets live in that file and leaking
them "lets an attacker ship signed, notarised malware under the Ouaricon identity". Adding a *step*
does not widen a trigger. But P13 is a recorded plan decision, and Stage 4's own rule is that prose
is not a third option: it gets overturned by amendment or it stands.

### 1.2 Recommendation — a new, secretless `ci-tests.yml`

| Option | Honours P13 | Honours the standing rule | Delivers D6's intent | Verdict |
|---|---|---|---|---|
| **A.** Add a probe step to `build-and-release.yml` | ❌ overturns it | ✅ (no trigger change) | ⚠️ partly — tag-time only, no push feedback | Needs an amendment *and* gives less |
| **B.** New `ci-tests.yml`, push/PR, `permissions: contents: read`, **no secrets** | ✅ literally | ✅ orthogonal — it is a different file | ✅ a JUCE bump is a repo edit, which pushes | **Recommended** |
| **C.** Both | ❌ | ✅ | ✅✅ | Over-built for one plugin's first test job |

**B**, because D6's stated failure mode is *"a JUCE bump ships silently"* — and a JUCE bump is a
commit, not a tag. A tag-time gate catches it only at release; a push-time gate catches it at the
commit that made it. B also keeps D4 ("no packaging, no signing") literally true, which A does not
(see §3.3).

### 1.3 The four mechanics the job must get right

All four are **measured or verified**, not assumed:

1. **`OUARICON_BUILD_TESTS` is a shared option name across 12 plugins**
   (`O-Bowed`, `O-Contrabass`, `O-MultiBandCompressor`, `O-ReverseDelay`, `O-simple*` ×7, O-Octagon).
   A full-tree configure with `-DOUARICON_BUILD_TESTS=ON` enables **eleven other plugins'
   harnesses**, none of which has ever run in CI. Contain it with `SKIP_PLUGINS` — the Windows
   `validate_only` path (`build-and-release.yml:518-530`) already contains the exact loop that
   builds "every plugin except this one".
2. **Build single-arch.** The release job builds `arm64;x86_64`. A probe run has no reason to, and
   universal would roughly double the measured 167 s of CPU. Omit `CMAKE_OSX_ARCHITECTURES`.
3. **Do not pass `OUARICON_RELEASE=ON`.** The render harness hard-codes the *dev* codes
   (`JucePlugin_ManufacturerCode=0x4f754476` = `OuDv`, `tests/render-harness/CMakeLists.txt`), which
   is what all 48 probes were authored against. Dev branding is the default; the root
   `CMakeLists.txt:16-21` force-OFF does not apply in CI (`$ENV{CI}` is set), so this must be an
   omission, not a reliance.
4. **Build, do not just run.** Per N2, the Layer-2 tripwire is a compile-time `static_assert`.

### 1.4 The JUCE pin must be DERIVED, not mirrored

`build-and-release.yml:74` pins `JUCE_VERSION: '8.0.14'` in `env`. A second workflow carrying its own
literal is `pattern_test_fixture_mirrors_drift_silently`, and the drift would be silent **in exactly
the direction that matters**: someone bumps JUCE in the release workflow, and the probe workflow
keeps proving the *old* JUCE green. That would make the CI gate worse than absent, because it would
report green about a JUCE nobody is shipping.

| Approach | Cost | Verdict |
|---|---|---|
| `.github/juce-version.txt`, read by both workflows | one-line `env` change in the secrets workflow (no trigger, no permission change) | **Recommended** — single source, both directions |
| `ci-tests.yml` greps `JUCE_VERSION:` out of the other file | touches nothing | brittle; a YAML reformat breaks it silently |
| Two literals + a comment | zero | this is the pattern, and comments do not keep promises |

This is the same move the render harness already makes for the plugin version — it reads
`JUCE_VERSION` off the target rather than mirroring the literal, and its own comment records that
literal drifting **twice across five releases** in the sibling O-ReverseDelay.

### 1.5 Job sketch (for the plan to turn into tasks)

```
name: CI Tests            # NO secrets. permissions: contents: read.
on: [push, pull_request]  # fork-safe precisely because there is nothing to leak
runs-on: macos-14
  - checkout
  - read JUCE version from .github/juce-version.txt   -> $JUCE_VERSION
  - download + unzip JUCE $JUCE_VERSION; apply vendored/JUCE-overrides (same 2 files, same grep)
  - build SKIP list = every plugins/* except O-Octagon
  - cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
        -DOUARICON_BUILD_TESTS=ON -DSKIP_PLUGINS="$SKIP"      # no OSX_ARCHITECTURES, no RELEASE
  - cmake --build build --target O-Octagon-geometry-test O-Octagon-render-test
  - run both binaries; exit code IS the gate
  - node tests/ui_frontend_check.js / ui_layout_check.js   -> NOT here (D6; see §1.6)
```

Note the JUCE-overrides copy is retained even though O-Octagon does not use note-expression: the
step's own `grep -q "JUCE-NE-PATCH"` is a fail-fast that keeps the two workflows' JUCE **identical**,
and an identical JUCE is what makes the Layer-2 golden comparable across them.

### 1.6 The two JS gates stay out, and the reason is now measurable

D6 kept them local-only as DPR/viewport-sensitive (`pattern_tooltip_clamp_gate_viewport_sensitive`).
That holds, and §1.5 leaves them out. But **record the residual as a named deferral, not prose** —
which is the rule this stage adopted. 69 sections across two files, green today, gated by a human
running `node`. Owner: none; blocked on headless-render determinism, not on effort.

---

## 2. What CI actually costs (Q2) — measured

A clean build directory was configured and built exactly as §1.5 prescribes.

| Step | Wall | CPU | Notes |
|---|---|---|---|
| `cmake` configure, O-Octagon only, tests ON | **7.8 s** | 20.8 s | 39 plugins skipped |
| `ninja` both test targets, from clean | **27.6 s** | **167 s** | 16 cores, arm64 only, **0 warnings** |
| `O-Octagon-geometry-test` | **0.023 s** | — | 44 probes |
| `O-Octagon-render-test` | **0.370 s** | — | 48 probes, incl. block-size-invariance renders |

**Q2's premise is disproved.** The render harness was the suspected cost — 48 probes including
full renders — and it runs in **0.37 s**. There is nothing to gate to a separate job.

The cost is entirely compilation, and most of it is JUCE: the two console apps compile their own
JUCE module objects. Extrapolating 167 s of CPU to a 3-core `macos-14` runner gives ~60–90 s of
build, plus ~30 s JUCE download and ~15 s checkout: **a 3–4 minute job**.

**Verified, because it halves the number:** the tests-only build does **not** produce the plugin
bundles. `OuariconOctagon_artefacts/Release/` came out holding `libO-Octagon-dev_SharedCode.a`
(28 MB) with `VST3/`, `AU/` and `Standalone/` **empty** — `add_dependencies(O-Octagon-render-test
OuariconOctagon)` reaches the shared-code target, not the format wrappers.

---

## 3. Windows (Q3, Q4)

### 3.1 Q3 — both named MSVC patterns are clean, and the scan is discharged here

| Hazard | Method | Result |
|---|---|---|
| `critical_msvc_constexpr_lambda_capture` (C3493) | Lexical scan: every lambda introducer in `Source/**`, brace-matched body, `constexpr` not preceded by `static` | **Zero hits.** `PluginEditor.cpp:64-65`'s two envelope constants sit at namespace scope, deliberately — the file's header comment `:29-36` records why |
| `critical_msvc_safepointer_init_capture_nested_lambda` | `grep -rn SafePointer Source/` | **Zero occurrences.** Not "authored against" — **absent**. There is no call site to get wrong |
| POSIX/portability | `unistd.h`, `<sys/*>`, `M_PI`, `__PRETTY_FUNCTION__`, `__attribute__`, `alloca(`, `strcasecmp`, `ssize_t`, `typeof` across `Source/` **and** `tests/` | **Zero hits** |

Lambda density for context: 24 in `PluginEditor.cpp`, 2 each in `PluginProcessor.cpp`,
`ChannelMap.cpp`, `ConvexHull2D.cpp`, 1 each in three more. The scan is not vacuous for want of
lambdas to scan.

**So the plan should stop describing the Windows risk as "the two known patterns".** It is not.
The risk is that no MSVC has parsed ~11 k lines of plugin source, and the realistic first-run
candidates are ones grep cannot pre-clear:

- `/permissive-` two-phase name lookup in the templated probe helpers
- `min`/`max` macros from `windows.h` colliding with `std::min`/`std::max`
- C4244/C4267 narrowing surfacing as errors under `juce_recommended_warning_flags`
- MSVC's stricter `constexpr` evaluation in the `static_assert`s in `HullProcessor.h` and
  `ChannelMap.h`

None is predictable from here. That is the point of D3: **one CI run answers it and a grep cannot.**
Note the test targets are macOS-only under D6, so MSVC sees the plugin TUs only.

### 3.2 Q4 — the Windows WebView story is authored, and it is the house pattern

| Requirement | Status | Evidence |
|---|---|---|
| `NEEDS_WEBVIEW2 TRUE` | ✅ | `CMakeLists.txt:19` |
| `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` | ✅ | `CMakeLists.txt`, `target_compile_definitions` |
| `withUserDataFolder()` | ✅ | `PluginEditor.cpp:1048-1059`, under `#if JUCE_WINDOWS`, → `tempDirectory/OOctagon_WebView`, plus `withStatusBarDisabled()` and `withBuiltInErrorPageDisabled()` |
| Resource provider takes **bare paths** | ✅ | `getResource()` matches `"/"`, `"/js/app.js"` … by equality, with a comment naming `critical_webview_resource_provider_and_schemes`; `goToURL(getResourceProviderRoot())` at `:1101` |
| WebView2 NuGet on the runner | ✅ **already in CI** | `build-and-release.yml:508-511`, pinned `1.0.1901.177` |

**And it is not novel: all 40 plugins in the repo set both `NEEDS_WEBVIEW2` and the static-linking
define.** Q4's implicit worry — that O-Octagon would be the first WebView plugin to meet Windows —
is unfounded. What *is* first is this plugin.

**What the Windows job can and cannot claim.** pluginval at strictness 10 opens the editor, and the
existing timeout comment (`:559-564`) reasons explicitly about a cold WebView2 first-open. So a
silently-blank WebView would surface as an Editor-Automation failure or timeout rather than passing
green — the job can claim more than "it compiles". It **cannot** claim the UI is *correct* on
Windows: no human will see it this milestone. Record that as an explicit deferral with owner *none —
blocked on hardware, not effort*, alongside D10.

### 3.3 The Windows dispatch path exists — and it collides with D4

The mechanism D3 asks for is **already built**: `workflow_dispatch` with `plugin_name: O-Octagon`,
`validate_only: true` — the path added for O-Contrabass's COMPAT-01 gate. It configures only the
target plugin, builds the VST3, and runs pluginval 10 with a 600 s timeout.

**But `build-macos` has no `validate_only` guard** (`:113-118`), and its comment says that is
deliberate — the validate run doubles as the signing-secrets gate. So dispatching for O-Octagon
**signs, notarises and builds a PKG**. Nothing is published (`create-release` is guarded at `:651`),
but that is packaging and signing, which **D4 excludes**.

| Option | D4 intact | Cost |
|---|---|---|
| **Put a Windows VST3 + pluginval job in `ci-tests.yml`** | ✅ | one more job in a file being created anyway; needs no secrets at all |
| Dispatch `build-and-release.yml`, accept and record the deviation | ❌ literally | zero new code; burns a notarisation round-trip |
| Add `if: validate_only != 'true'` to `build-macos` | ✅ | changes shared-workflow semantics the comment says are intentional — **do not** |

**Recommended: option 1.** It keeps D4 true as written and puts every machine gate in one secretless
file. Second choice is option 2 *with the deviation recorded*, never silently.

---

## 4. Factory presets: where they live (Q5)

### 4.1 The discuss premise is stale

`CONTEXT-4.1` Q5: *"O-Octagon has no preset file, no preset directory and no preset-manager
dependency today."* Two of the three are wrong as of Stage 3:

| Claim | Actual |
|---|---|
| no preset-manager dependency | **False.** `CMakeLists.txt` puts `modules/persistence/preset-manager/cpp` on the include path; `PluginEditor.h:114` includes it; `:148` holds the member; `PluginEditor.cpp:245` constructs `presetManager (p.getAPVTS(), "O-Octagon")` |
| no preset directory | **False in effect.** `~/Library/O-Octagon/Presets/{Factory,User}/` — module `getPresetsDirectory()`. (Not *Application Support*, despite the module.yaml prose; IN-01 corrected the docstring, not the yaml) |
| no preset file | **True** — nothing has been authored yet |

Four native functions are already registered — `savePreset`, `loadPreset`, `getPresetList`,
`getCurrentPreset` — deliberately four of the module's ten (`PluginEditor.cpp:744-748`).

**Answer to Q5: the shared module, and it is already wired. 4.1 authors definitions, not plumbing.**

### 4.2 FUNC-05's separation is structural, and this confirms it

`applyPresetJson` reaches only APVTS parameters and the `customState` callback. The `VENUE`
`ValueTree` is unreachable from it; `setStateFromXml` — the one path that does `replaceState` — is
never called, and §35 of the static gate is a one-line guard over both files. `VenueFile` has no
directory logic at all: the `.venue` path is chooser-supplied. **A musical preset cannot reach the
42 measured values, by construction rather than by convention.**

### 4.3 The initialization site has no precedent here — and the departure is an improvement

Every one of ~20 precedent plugins calls `initializeFactoryPresets()` from the **processor
constructor** with a processor-owned manager (`O-AnalogEQ:222`, `O-Bass:122`, `O-Bells:1568`,
`O-Bowed:628`, `O-Bassoon:171`, …). **O-Octagon's manager is editor-owned.** There is nothing to
copy.

Editor-side initialization is the right call and is *strictly better* than the precedent:

- The module's own constructor comment says directory creation is deferred "to avoid file I/O during
  AU validation". An editor-side call keeps **every** write off the headless scan path — `auval` and
  pluginval construct processors, not editors.
- The preset list is only ever read through a native function, so by the time anything can read it,
  the editor exists.

### 4.4 The `.factory-version` sentinel is an authoring **and** a verification trap (N7)

`initializeFactoryPresets` returns early when `Factory/.factory-version` matches
`JucePlugin_VersionString` (`:614-623`). **O-Octagon ships 1.0.0 and stays there** (D4).

- **Authoring:** the second and every later edit to the definitions **writes nothing**. Iterating on
  values during 4.1 silently no-ops, and the symptom is "my change had no effect", which reads as a
  code bug.
- **Verification, and this is the sharp one:** a probe that reads the on-disk Factory JSON can be
  reading a file from an earlier iteration and **pass**. That is this project's recurring vacuity
  shape in a new place.

**Two plan requirements follow.** Delete `~/Library/O-Octagon/Presets/Factory/` before each
authoring iteration; and have the probe assert against the **in-source `FactoryPresetDef` vector**
(or delete-then-initialize inside the probe), never against the file alone.

---

## 5. N5 — the finding that changes D5

### 5.1 What the mechanism does

`OuariconPresetManager.h:315-331` (WR-01, module v1.0.3), inside `applyPresetJson`, before applying
anything:

```cpp
for (auto* param : parameters.processor.getParameters())
    if (auto* rangedParam = dynamic_cast<juce::RangedAudioParameter*>(param))
        if (rangedParam->isMetaParameter() == metaPass)
            rangedParam->setValueNotifyingHost(rangedParam->getDefaultValue());
```

Unconditional, over **every** parameter. WR-01 exists for a good reason — a preset omitting a key
would otherwise inherit stale live state (`pattern_preset_apply_needs_reset_to_defaults`).

### 5.2 Why that breaks D5

D5 scopes factory presets to six IDs — `width`, `rolloff`, `blur`, `hullAtten`, `airAmount`,
`outputGain` — *specifically* so they do not collide with position (per-cue automation) and
`w1..w8` (FUNC-06's scenes). Its rationale: *"A preset writing either puts two mechanisms on the
same parameters."*

But **omitting the other eleven does not leave them alone — it resets them:**

| Omitted | Reset to |
|---|---|
| `srcX`, `srcY` | 0.5, 0.5 — the source re-centres |
| `srcZ` | 0.0 m |
| `w1`…`w8` | 1.0 each — **every speaker un-muted** |

**Loading any factory preset un-does whatever FUNC-06 scene is applied and re-centres the source.**
That is precisely the collision D5 was written to prevent, arriving through the module's defensive
behaviour rather than the preset's content. D5's *intent* is right and survives; its *premise* —
that scoping the keys scopes the effect — does not.

### 5.3 The mechanism was known; the planning consequence was never drawn

`PluginEditor.cpp:697-707` (Phase 3.2) already documents it: *"a load from an untouched default patch
still emits 17 reset writes of values the parameters already hold."* That reasoning is about F3's
unchanged-write skip, and it is correct **for the case it considers** — a default patch, where the
writes are no-ops. From a working session they are not no-ops. Nothing looked back at D5.

### 5.4 The fix, and where it belongs

`PluginEditor.cpp:711` already rules on this: *"THE FIX IS HERE, AT O-OCTAGON'S CALL SITE, and never
in the shared module: four other plugins depend on it."* (Nine, in fact, include the shared header;
seven more carry vendored copies.)

**Snapshot-and-restore at the call site.** In the `loadPreset` native function, capture the eleven
non-room-character values before `presetManager.loadPreset(name)` and write them back after —
**inside the gesture bracket already open on all 17** (`:723-739`). Roughly eight lines, no module
bump, no blast radius, and it reuses a bracket whose correctness was established at 3.2.

Rejected alternatives: authoring all 17 keys (same outcome — the eleven still move, just explicitly);
a module opt-out flag (correct in the abstract, but it is a nine-plugin shared header and the call
site is already the ruled-on place).

### 5.5 The probe must be a negative control, or it is vacuous

Asserting "the six moved" **passes with the bug present**. The probe must:

1. set a non-default position **and** apply a non-`all` scene,
2. load a factory preset,
3. assert the **six changed to the preset's values** *and* the **eleven are bit-unchanged**.

Clause 3's second half is the whole probe. Without it this is the fifth instance of the vacuity class
Stage 3 caught five times.

---

## 6. N6 — `COMPAT-04` criterion 3 asserts the failing behaviour

### 6.1 The defect in the criterion

As written, in **both** `REQUIREMENTS.md` and `ROADMAP.md` §4.1:

> …asserted through the **complement predicate**, with a negative control admitting a fourth
> 8-channel set that confirms the banner **stays down**.

`PluginProcessor.cpp:230-238` ships the complement form:

```cpp
const bool realRig = outSet == create7point1() || outSet == create7point1SDDS()
                                               || outSet == create5point1point2();
safeMode.store (! realRig, std::memory_order_release);
```

A hypothetical fourth 8-channel container is **not** one of the three → `realRig` false → `safeMode`
**true** → **banner UP**.

"Stays down" is what the `== mono || == stereo` spelling would do — the spelling D8's own prose two
paragraphs earlier rejects: *"Written the second way, a fourth 8-channel container admitted later
would silently stop raising the banner."* The criterion's discriminator sentence states the outcome
backwards.

**Verified as written, criterion 3 passes only if the code has the spelling D8 rejects.** It is the
same shape as D7 — an acceptance criterion that is not looking at what it means — one boundary later,
in a criterion authored *specifically* to be un-vacuous. Per Stage 3's parting rule, the plan must
grep the other contracts for the same sentence before closing.

**Corrected wording:**

> …with a negative control on a fourth 8-channel set confirming the banner **is raised** — which is
> what discriminates the complement spelling from `== mono || == stereo`, under which it would stay
> down.

### 6.2 Two thirds of the criterion is already met — probe BM

`tests/render-harness/main.cpp:3807-3850`, Phase 3.1, drives five layouts through a real
`prepareToPlay` and asserts `isSafeMode()` against each:

| Layout | Expect | Covered |
|---|---|---|
| `mono()` | SAFE | ✅ |
| `stereo()` | SAFE | ✅ |
| `create7point1()` | REAL | ✅ |
| `create7point1SDDS()` | REAL | ✅ |
| `create5point1point2()` | REAL | ✅ |

Its comment even anticipates the criterion: *"this probe is also what would catch that predicate
drifting away from `isBusesLayoutSupported()`'s."* It was invisible to the ledger because
`COMPAT-04` had no criteria section until yesterday.

### 6.3 What is genuinely open, and how to write it

BM observes the predicate **only through `prepareToPlay`**, so it cannot reach a set that
`isBusesLayoutSupported()` rejects — and a runtime probe cannot make that function admit a fourth
container without editing it.

**Recommended construction — one small extraction, then one probe.** Hoist the three-container test
into a header-only free function both targets can call:

```cpp
// Source/Data/RigPolicy.h  (juce_audio_basics only — the unit target already links it)
namespace oo::rig {
    inline bool isRealRig (const juce::AudioChannelSet& s) noexcept
    {
        return s == juce::AudioChannelSet::create7point1()
            || s == juce::AudioChannelSet::create7point1SDDS()
            || s == juce::AudioChannelSet::create5point1point2();
    }
}
```

`prepareToPlay` then calls `safeMode.store (! oo::rig::isRealRig (outSet), …)`, and:

- **Probe CO (unit target, fast):** `isRealRig` is false — i.e. the banner *would* be raised — for
  `create7point1point4()`, `octagonal()`, `quadraphonic()`, `mono()`, `stereo()`; true for exactly
  the three. **That is the complement property asserted directly**, with no edit to the bus
  predicate, and it fails immediately against the `== mono || == stereo` spelling.
- **Probe BM (unchanged):** proves the shipped call site actually uses it.

Together they are non-vacuous: CO proves the **form**, BM proves the **wiring**. Neither alone does.

**This is the one source change 4.1 needs beyond presets.** It is a testability extraction — no DSP,
no UI, no parameter — so it sits inside the stage constraints, but the plan should name it as a
deliberate edit to shipped code rather than let it arrive as a side effect.

### 6.4 Criterion 2's added render clause

Criterion 2 gains "finite samples, no NaN/Inf through the SAFE fold". The harness already
constructs bus layouts programmatically and already renders under mono/stereo out, so this is a
short probe on an existing scaffold, not new machinery.

---

## 7. The preset values, in engineering units (Q6)

### 7.1 The three laws, computed from shipped constants

**`rigScale` of the §OQ4 default venue = 7.9317 m** — the 3-D RMS distance of the eight speakers
from their centroid `(6.5000, 12.4625, 4.9250)`, per `VenueModel.cpp:396-404`.

`blur` → source radius `r_s = blur · 0.5 · rigScale`, capped at 8 m (`DbapSolver.h:166-168`):

| `blur` | 0.00 | 0.10 | 0.18 | 0.25 | 0.35 | 0.55 | 0.80 | 1.00 |
|---|---|---|---|---|---|---|---|---|
| `r_s` (m, default venue) | 0.00 | 0.40 | 0.71 | 0.99 | 1.39 | 2.18 | 3.17 | 3.97 |

Air cutoff `fc(d) = 20000 · 2^(−airAmount · d / 3)`, floored at 500 Hz, ceiling additionally capped
at `0.45·fs` (`HullProcessor.h:59-78`):

| `airAmount` | d=0 | d=1 m | d=2 m | d=4 m | d=8 m |
|---|---|---|---|---|---|
| 0.00 | 20000 | 20000 | 20000 | 20000 | 20000 |
| 0.25 | 20000 | 18878 | 17818 | 15874 | 12599 |
| 0.35 | 20000 | 18446 | 17013 | 14473 | 10473 |
| 0.55 | 20000 | 17613 | 15511 | 12030 | 7236 |
| 0.85 | 20000 | 16434 | 13504 | 9117 | 4156 |

Hull trim `−hullAtten · d_hull` dB, floored at −24 dB (`HullProcessor.h:59`):

| `hullAtten` | d=1 m | d=2 m | d=4 m | d=8 m |
|---|---|---|---|---|
| 0.0 dB/m | −0.00 | −0.00 | −0.00 | −0.00 |
| 0.4 | −0.40 | −0.80 | −1.60 | −3.20 |
| 1.0 | −1.00 | −2.00 | −4.00 | −8.00 |
| 1.4 | −1.40 | −2.80 | −5.60 | −11.20 |
| 2.2 | −2.20 | −4.40 | −8.80 | −17.60 |

### 7.2 Proposed set — five, with a sixth optional

Six of the 17 IDs. Every value below is an **engineering unit**; §7.3 covers the conversion.

| Preset | `width` m | `rolloff` dB/2× | `blur` (r_s) | `hullAtten` dB/m | `airAmount` | `outputGain` dB | What it is |
|---|---|---|---|---|---|---|---|
| **Dry Point** | 0.0 | 6.0 | 0.00 (0.00 m) | 2.2 | **0.00** | 0.0 | Tightest localisation, steepest law, air **defeated** |
| **Concert Default** | 0.0 | 4.0 | 0.10 (0.40 m) | 1.0 | 0.35 | 0.0 | The shipped defaults, named — "back to neutral" in one click |
| **Chamber** | 1.5 | 4.5 | 0.18 (0.71 m) | 1.4 | 0.25 | 0.0 | Small room: slightly tightened distance law, modest spread |
| **Wide Hall** | 3.0 | 3.5 | 0.35 (1.39 m) | 0.7 | 0.55 | −1.5 | Large and diffuse; the trim offsets the wider spread |
| **Distant Field** | 4.5 | 3.0 | 0.55 (2.18 m) | 0.4 | 0.85 | −3.0 | Far and dark — fc ≈ 9.1 kHz at 4 m outside the hull |
| **Enveloping** *(optional 6th)* | 6.0 | 3.0 | 0.80 (3.17 m) | **0.0** | 0.45 | −2.0 | Maximum width, hull trim **defeated** — the no-distance-law extreme |

**Why these and not taste.** Three constraints shaped them:

1. **The axis is monotone.** `rolloff` falls 6.0 → 3.0 and `blur` rises 0.00 → 0.80 together, so the
   set reads as one "point → enveloping" continuum rather than six unrelated points. `hullAtten`
   falls with it because a diffuse image with a steep outside-hull trim fights itself.
2. **Two presets sit on a defeat path deliberately.** `airAmount = 0` is bit-transparent by
   construction (ROADMAP known-challenge 5) and `hullAtten = 0` is bit-exact unity (probe AV, 201
   swept distances). Putting a preset on each means **the factory set doubles as a reachability
   check on both exact-no-op branches** — they get exercised from a user-reachable path, not only
   from a probe.
3. **`outputGain` only trims, never boosts.** Wider spread and lower rolloff put more total energy
   into the array; −1.5 and −3.0 dB roughly level the set by ear-equivalent. Nothing boosts, so no
   preset can clip a rig that was calibrated on another.

**One caveat to state on the Venue screen or in NOTES.md:** the `r_s` metres column is illustrative
for the **default venue only**. Presets store `blur` (0–1), not metres, so they are venue-portable by
construction — on a rig with a different `rigScale`, the same `blur` gives a proportionally different
radius. That is the correct behaviour and it is worth writing down so it is not read as drift.

### 7.3 Conversion — mandatory, and for a reason that outlives the linear skews

`FactoryPresetDef::parameters` is `std::map<juce::String, float>` of **normalized** values
(`OuariconPresetManager.h:186-191`). So every value in §7.2 must go through
`range.convertTo0to1(...)` read off the **live** `NormalisableRange`
(`pattern_factory_preset_normalized_ignores_skew`).

All 17 skews are linear, so today the arithmetic is a subtraction and a divide, and it is tempting to
write the fractions directly. **Do not.** `critical_apvts_denormalised_vs_preset_normalised`: a later
range widening shifts every saved preset, and deriving from the live range is what keeps the
authored engineering value anchored to what it meant. Writing `0.75f` for `rolloff = 5.25` bakes in
`3.0–6.0` forever, silently.

---

## 8. Docs and install (Q-adjacent, but on the 4.1 list)

| Item | State | Note |
|---|---|---|
| `CHANGELOG.md` | **Absent** | 37 of 40 plugins have one. `build-and-release.yml:669-681` reads `plugins/O-Octagon/CHANGELOG.md` for release notes and falls back to a bare "Release O-Octagon v1.0.0" — not fatal, but that fallback *is* what shipping without it looks like |
| `README.md` | Absent | **Present in 1 of 40** — not the house convention, and the ROADMAP correctly does not ask for one |
| `NOTES.md` | Present | Needs the Stage 2–4 content, incl. §7.2's venue-portability note |
| `PLUGINS.md:68` | **Stale by three stages** — `\| O-Octagon \| 🚧 Stage 1 \| 1.0.0-dev \| … \| 2026-08-11 \|` | |
| Install | `./scripts/build-and-install.sh O-Octagon` | Resolver verified at this boundary: `cmake-target` → **`OuariconOctagon`**, `product-name` → **`O-Octagon`**. Artefacts land under `OuariconOctagon_artefacts/`. Watch for the `⚠ Sweeping ALTERNATE-variant` line (`critical_dev_release_variant_shadowing`) |

---

## 9. Handoff to plan

### Amendments the plan must take at its boundary

| # | Contract | Change |
|---|---|---|
| **A1** | `ROADMAP.md` §4.1 bullet 1 | Name the actual CI destination. Either overturn P13 explicitly or point at `ci-tests.yml` (§1.2 recommends the latter) |
| **A2** | `plugins/O-Octagon/CMakeLists.txt:172-176` | If A1 chooses `ci-tests.yml`, this comment becomes **wrong by omission** — it says the gap "is logged as a repo-level todo instead", and it no longer is. Update it in the same commit |
| **A3** | `REQUIREMENTS.md` `COMPAT-04` crit. 3 **and** `ROADMAP.md` §4.1 | "stays down" → "is raised" (§6.1). **Grep both other contracts for the same sentence before closing** — Stage 3's parting rule, and it has now fired twice |
| **A4** | `ROADMAP.md` §4.1 preset bullet | D5's "room-character axis only" needs the N5 clause: the scope is achieved by the call-site restore, not by omitting keys |

### Plan inputs, ordered by what blocks what

1. **`ci-tests.yml`** + `.github/juce-version.txt`, per §1.5 / §1.4. Optionally the Windows job (§3.3)
   — recommended, because it is what keeps D4 literally true.
2. **`RigPolicy.h` extraction + probe CO** (§6.3). Blocks `COMPAT-04` closure. Smallest item on the
   list and the only shipped-source edit.
3. **Factory presets**: definitions (§7.2), `convertTo0to1` conversion (§7.3), editor-side
   `initializeFactoryPresets` (§4.3), the sentinel discipline (§4.4), the call-site snapshot/restore
   (§5.4), and the negative-control probe (§5.5). This is the largest item and it has five distinct
   traps in it.
4. **Criterion 2's render clause** (§6.4) — short, existing scaffold.
5. **pluginval ×3 per format + `auval -v aufx OuOc OuDv`** on the final binary. Already green at 2.3
   ×3/×3; this is confirmation after six phases of change, not a first run.
6. **Docs** (§8) — `CHANGELOG.md` created, `NOTES.md` brought current, `PLUGINS.md:68` corrected.
7. **Install + dual-variant sweep** (§8).

Probe letters: Stage 3 closed at **CN**, so **CO** is next (§6.3), and the N5 preset control follows
it as **CP**. First plan decision is **P86**.

### Named deferrals — recorded, not carried as prose

| Item | Owner |
|---|---|
| Locks / file I/O in `processBlock` — grep + inspection only | **none** — blocked on toolchain (`-fsanitize=realtime` unsupported by Apple clang 17.0.0). Unchanged from D10 |
| Two JS gates, 69 sections, local-only | **none** — blocked on headless-render determinism (§1.6) |
| Windows **UI correctness** (as distinct from "pluginval passes") | **none** — blocked on hardware. pluginval 10 opening the editor is the ceiling of what CI can claim (§3.2) |

---

## Next Phase

Ready for: **plan** phase. The two items that could still change the plan's *shape* are **A1** (which
workflow, and whether Windows moves into it) and **item 3's** ordering — the preset work carries five
independent traps and is the only place in 4.1 where a green result can be wrong.
