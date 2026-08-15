# Stage 2 — DSP · Phase 2.1 (Geometry Core) — Research

**Plugin:** O-Octagon
**Stage:** 2 of 4 — DSP
**Phase:** 2.1 of 3 — Geometry Core
**GSD phase:** research
**Date:** 2026-08-11
**Branch:** `feat/o-octagon` @ `f135b3da`
**Depth:** DEEP (complexity tier 6)

---

## Entry Check — contract checksums

Re-run at this phase per C6. All four byte-exact against `STATUS.md` frontmatter:

| Contract | SHA-256 | Result |
|---|---|---|
| `BRIEF.md` | `697a4f32…9fbd6` | ✅ |
| `parameter-spec.md` | `b45f88dc…b9e02f` | ✅ |
| `research/ARCHITECTURE.md` | `bff8a83b…06cfe` | ✅ |
| `ROADMAP.md` | `aec7d0ce…7ee29` | ✅ |

No drift.

## Scope discipline

Per CONTEXT-2.1 §Next Phase, this research did **not** re-derive the hull algorithm (§3.1), the
channel-map construction (§3.2.3), the venue schema (§4.1) or the numeric defaults (§OQ3/OQ4). All
findings below are either (a) answers to Q1–Q5, or (b) defects/hazards found *in the specification
itself* while verifying it against JUCE 8.0.14 source and the repo's actual conventions.

All JUCE claims are verified against the local tree at `/Users/taylorbrook/JUCE` (8.0.14) with
file:line references. Context7 was not consulted — local source is the stronger authority, matching
Stage 0/1 practice.

---

## Answers to the Open Questions

### Q1 — What replaces the `PHASE-2.2-REPLACE` block at 2.1?

**Answer: route the mono sum through `speakerToBuffer`, but the routing must be bounded by
`buffer.getNumChannels()` *at the point of indexing*, not merely gated on map validity.**
CONTEXT's recommendation is confirmed, with one correction that is load-bearing.

The gate wording is ROADMAP:141 — *"`grep -rn` confirms **zero hardcoded output channel indices**
outside `ChannelMap`."* A loop variable is not a hardcoded index; a literal `8` or a constant slot
number is. Routing the mono sum through the map satisfies the gate literally, retires C2's block
rather than grandfathering it, and exercises the map with real audio a phase early. All eight lanes
still carry identical signal, which is correct — independence is FUNC-01, re-mapped to 2.2.

**The correction — G1 below is the reason.** `rebuildChannelMap()` derives validity from
`getBusesLayout().getMainOutputChannelSet()` (ARCHITECTURE §3.2.3), i.e. from the *layout*. Under the
F3 hazard the layout is exactly the thing that lies: in Release on a 3–7 output device the layout
stays 7.1 while the buffer holds `n < 8` channels. `outSet.size() == 8` is then **true**,
`mapInvalid` is **false**, and `speakerToBuffer` holds indices 0..7 — so an inner loop that trusts
map validity writes past the end of the buffer. A valid map is *not* evidence that the buffer has 8
channels.

Shape recommended to plan (structure, not final code):

- **REAL path** — taken only when `buffer.getNumChannels() == 8` *and* `!mapInvalid`. Writes via
  `speakerToBuffer[i]`.
- **SAFE path** — every other case, including the F3 state. Plain loop `for (ch = 0; ch < numOut;
  ++ch)`, bounded by `buffer.getNumChannels()`. No literal index, so the grep gate still passes.
- The read-before-write sample-interleaved sum at `PluginProcessor.cpp:209-223` is preserved as-is —
  `out[0]` and `in[0]` alias, and that reasoning is unchanged by the map.

This keeps C3 intact and makes the F3 hazard a *tested* path rather than a reasoned one (C5).

### Q2 — How does `gen_juce_channel_order.py` locate the JUCE modules portably?

**Answer: `JUCE_MODULES_DIR`, passed from CMake to the script as an argument. Never a path literal.**

`JUCE/CMakeLists.txt:41` sets

```cmake
set(JUCE_MODULES_DIR "${JUCE_SOURCE_DIR}/modules" CACHE INTERNAL "The path to JUCE modules")
```

`CACHE INTERNAL` makes it global once the root adds JUCE (root `CMakeLists.txt:36-41`), which happens
**before** the plugin glob loop at `:47-60`. Every plugin `CMakeLists.txt` therefore sees it already
resolved, whichever of the three branches the root took.

This is portable across both environments because the root's resolution is itself already portable:

| Environment | Root branch taken | `JUCE_MODULES_DIR` resolves to |
|---|---|---|
| Local dev | `/Users/taylorbrook/JUCE` fallback | local tree (8.0.14 + NE patch) |
| CI macOS | `$ENV{JUCE_DIR}` — set at `build-and-release.yml:131` | the downloaded 8.0.14 zip + vendored overrides |
| CI Windows | `$ENV{JUCE_DIR}` — set in the Windows *Setup JUCE* step | same |

**Alternative, marginally more robust:** the per-module target property
`INTERFACE_JUCE_MODULE_PATH` (set at `JUCEModuleSupport.cmake:500`), which is what JUCE's own code
uses to locate module sources (`JUCEUtils.cmake:1068`, `:1216`). It needs a generator expression or
`get_target_property`, so it is build-time rather than configure-time. **Recommend `JUCE_MODULES_DIR`**
— plain variable, usable directly in `add_custom_command`, and one line.

**Mandatory guard.** Q2's stated fear — *"Layer 2 becomes a local-only gate that silently no-ops in
CI"* — is real and must be closed explicitly:

```cmake
if(NOT JUCE_MODULES_DIR)
    message(FATAL_ERROR "JUCE_MODULES_DIR is empty — Layer 2's golden would be generated from "
                        "nothing and the COMPAT-03 gate would pass vacuously.")
endif()
```

A missing path must fail configure, not produce an empty golden. The same reasoning applies inside
the script: if the enum block or an initializer list fails to parse, exit non-zero — never emit a
header with zero entries.

### Q3 — Does the unit target run under CI, or locally only?

**Answer: land it local-only at 2.1. Do not touch `build-and-release.yml`. Record the residual gap
as a Stage-4 item rather than pretending it is closed.**

Three facts settle this, all verified:

1. **The repo has exactly one workflow**, `.github/workflows/build-and-release.yml`, triggered on
   `push: tags: '*-v*'` and `workflow_dispatch` only (`:36-60`). **There is no push or PR CI.**
2. **CI has never built any test target.** `-DOUARICON_BUILD_TESTS` appears nowhere in the workflow;
   the configure step (`:143-149`) does not set it and the build step (`:159-162`) builds only
   `${TARGET}_VST3` and `${TARGET}_AU`. All twelve existing harnesses are local-only.
3. The workflow is **secrets-bearing** and carries a standing rule at `:7-33` against widening its
   trigger surface. It is the *release* path: wiring a JUCE-drift gate into it means a JUCE upgrade
   blocks shipping an unrelated plugin.

So Q3's premise — *"only true if the test is wired into the build CI actually runs"* — cannot be
satisfied by the existing workflow without changing what that workflow is for. The honest position:

- Layer 2's *build-failure* property holds locally: with `-DOUARICON_BUILD_TESTS=ON` the checksum
  comparison fails the build, and a JUCE upgrade is a deliberate local act (the local tree is the
  pinned 8.0.14 + NE patch).
- **Residual gap, stated plainly:** a JUCE bump performed without running the test target ships
  silently. This is not closed at 2.1.
- **Correct home for the fix:** a separate, secrets-free `push`/`pull_request` workflow that
  configures with `-DOUARICON_BUILD_TESTS=ON` and runs the unit target. That is a repo-wide change
  benefiting all twelve existing harnesses, not an O-Octagon change — it belongs at Stage 4 or as
  standalone repo work, and should be logged as such, not smuggled into 2.1.

**Partial mitigation available now at near-zero cost:** Layer 1 is a pure runtime invariant with no
generated header and no build-time script. It can additionally run as a `jassert`-guarded check
inside `rebuildChannelMap()` in Debug builds, so a developer who never builds the test target still
trips it on the first `prepareToPlay()`. Offered to plan as an option; it does not replace the CI
workflow.

### Q4 — Where does hull classification surface before the Venue screen exists?

**Answer: make `VERTEX`/`ON_EDGE`/`INTERIOR` a first-class return value of the `ConvexHull2D` API
itself. No processor accessor, no test-only hook.**

Q4's dead-code concern dissolves once the classification lives on the hull object rather than on the
processor. `ConvexHull2D` must compute the classification anyway to pop collinear points
(ARCHITECTURE §3.1: `<= EPS_CROSS` → strict vertices only) — the `ON_EDGE` set is precisely the
points it popped. Exposing it is exposing state the object already holds.

- **At 2.1** the unit target constructs `ConvexHull2D` directly with the §OQ4 default layout and
  queries classification for speakers 1–8. The DSP-03 gate (*vertices 1, 2, 4, 5, 6, 7; speakers 3
  and 8 `ON_EDGE`*) is asserted against that call. No `OOctagonProcessor` involvement at all.
- **At 3.2** the Venue screen reads the *same* accessor through the venue-side path. It duplicates
  nothing, because there is only ever one implementation.

A test-only accessor on the processor would be the wrong answer for exactly the reason Q4 names: the
UI would later need its own, and the two would drift. Keeping it on the hull makes drift
structurally impossible.

### Q5 — Which unit-test framework?

**Answer: none. `juce_add_console_app` with hand-rolled assertions, matching all twelve existing
harnesses. Do not introduce Catch2, GoogleTest, doctest, or `juce::UnitTest`.**

The survey was exhaustive across the repo (excluding `build/`, `_deps/`, `node_modules/`):

| Token | Hits in plugin source or any `CMakeLists.txt` |
|---|---|
| `Catch2` / `catch2` / `catch_test_macros` | **0** — only `.claude/skills/plugin-testing/` templates and `docs/codebase/TESTING.md` |
| `gtest` | **0** — sole hit is a transitive entry in `O-TextureForge/Source/ui/package-lock.json` |
| `GoogleTest`, `doctest`, `juce_UnitTest`, `UnitTestRunner` | **0** |
| `enable_testing()`, `add_test()` | **0** |

**The Catch2 references in `docs/codebase/TESTING.md` and the `plugin-testing` skill templates
describe an intent that was never implemented. Do not follow them** — treating documentation as
evidence of convention here would introduce the repo's first test dependency on the strength of a
file that no build reads.

The actual, twelve-times-repeated convention:

```cmake
# plugins/<Name>/CMakeLists.txt
option(OUARICON_BUILD_TESTS "Build <Name> test targets" OFF)
if(OUARICON_BUILD_TESTS)
    add_subdirectory(tests/<target>)
endif()
```

and in the subdirectory: `juce_add_console_app()`, `target_sources` reaching into `../../Source/*.cpp`,
`add_dependencies(<test> <pluginTarget>)` for `JuceHeader.h`, then
`target_include_directories(... $<TARGET_PROPERTY:<pluginTarget>,INCLUDE_DIRECTORIES>)`, plus the
`JucePlugin_*` compile definitions the processor needs.

Pass/fail idiom (`O-ReverseDelay/tests/render-harness/main.cpp:808-824`): an `int failures` counter, a
`check(name, ok, detail)` lambda printing `[PASS]`/`[FAIL]`, and `return failures == 0 ? 0 : 1`.
Hard exit codes; exit 0 iff every probe passes.

**Consequence for D3 — the two targets are two console apps, not two frameworks.** Recommended split:

| Target | Links | Purpose |
|---|---|---|
| `tests/unit/` → `O-Octagon-geometry-test` | `VenueModel.cpp`, `ConvexHull2D.cpp`, `ChannelMap.cpp` only — **not** `PluginProcessor.cpp` | Layers 1–2, hull, venue, degeneracy matrix |
| `tests/render-harness/` → `O-Octagon-render-test` | `PluginProcessor.cpp` | C4 unity gain, C5 bus layouts, Layer 3 at 2.2 |

The unit target's exclusion of `PluginProcessor.cpp` is worth the split on its own: it needs **no**
`JucePlugin_*` macro block, compiles in a fraction of the time, and cannot be broken by the Stage-3
WebView editor swap. One `OUARICON_BUILD_TESTS` option gates both subdirectories, per convention.

---

## Findings beyond Q1–Q5

Numbered G1… continuing Stage 1's F-series.

### G1 — A valid channel map is not evidence of an 8-channel buffer *(HIGH — new instance of F3)*

`rebuildChannelMap()` validates against `getBusesLayout().getMainOutputChannelSet()`. Under F3
(`pattern_standalone_canonical_channelset_oob`) that layout reports 7.1 while the buffer holds `n < 8`
channels, so `outSet.size() == 8` passes, the permutation check passes, `mapInvalid` stays false, and
`speakerToBuffer` contains indices up to 7. Any inner loop that treats map validity as permission to
index 0..7 writes out of bounds.

The map is derived from the accessor that lies. **Bound the application of the map by
`buffer.getNumChannels()`, independently of `mapInvalid`.** This is the correction folded into Q1, and
it applies unchanged to 2.2's `GainStage` inner loop — carry it forward.

### G2 — Layer 1's `bit < 64` scan is too small, and fails *silently* *(HIGH — defect in the spec)*

ARCHITECTURE §3.2.5 Layer 1 reconstructs the expected order with `for (int bit = 0; bit < 64; ++bit)`.
Verified against source:

- Named `ChannelType` values run to **99** (`juce_AudioChannelSet.h:402-543`).
- `discreteChannel0 = 128` (`:543`), with discrete channels indexed upward from there.
- `channels` is a **`BigInteger`** (`:641`) — unbounded, not a 64-bit word.

For the three accepted 8-channel containers the maximum value is 29 (`topSideRight`), so the loop
happens to work today. That is the same *"it works by luck"* shape as the initializer-order
coincidence the architecture already calls out at §3.2.1 — and here it is worse, because the failure
is silent in the direction that matters: if a JUCE upgrade renumbers a type above 63, the
reconstruction drops it, the truncated list is *still strictly increasing*, and Layer 1 passes. The
test that exists to catch a JUCE enum change would be blinded by exactly that change.

**Fix:** raise the bound past `discreteChannel0` and — the part that actually closes it — assert
`expected.size() == set.size()`. The size assertion makes truncation loud regardless of the bound.

### G3 — The enum's source order is not its value order, and it contains an alias *(HIGH — Layer 2 parser)*

Two traps in the block `gen_juce_channel_order.py` must parse (`juce_AudioChannelSet.h:402-543`):

1. **Declaration order ≠ value order.** `topSideLeft = 28` and `topSideRight = 29` are declared at
   `:436-437`, *before* `ambisonicACN0 = 24` … `ambisonicACN3 = 27` at `:443-446`. A parser that
   assigns values by line position is wrong for precisely the two types `create5point1point2()` uses.
2. **Aliases exist.** `surround = centreSurround` (`:418`) — the right-hand side is an identifier, not
   an integer.

The parser must extract `name = <integer>` pairs, skip or resolve identifier-valued entries, and
**assert every name referenced by an initializer list was found**. A silently-missing name would
otherwise drop a channel from the golden, and the golden's whole job is to be independent of the
implementation's assumptions.

Container membership is a clean single-line parse — `juce_AudioChannelSet.cpp:567`, `:568`, `:574` —
one initializer list each, no line continuations.

### G4 — JUCE already ships the reverse name→type lookup, with one sharp edge *(MEDIUM)*

ARCHITECTURE §3.2.4 says to resolve stored label strings "by a small name→type table built from
`outSet.getChannelTypes()`". JUCE already provides
`AudioChannelSet::getChannelTypeFromAbbreviation(const String&)` — declared `juce_AudioChannelSet.h:553`,
defined `.cpp:282-389`. Use it; a hand-rolled table is one more thing that drifts.

Two behaviours to design against:

- Unrecognised strings return `unknown` (`:389`). Good — that is the FUNC-03 missing-label detector,
  and `unknown` is in no accepted set, so `getChannelIndexForType()` returns `-1`.
- **`.cpp:283-285` has an unvalidated numeric branch:** any string whose first character is a digit
  becomes `discreteChannel0 + N - 1`, no range check. A corrupt `.venue` label of `"7"` therefore
  yields a *plausible-looking* `ChannelType` rather than `unknown`. It still fails safe — the type is
  absent from every accepted 8-channel set, so the permutation check rejects the map and sets
  `mapInvalid` — but it fails via the permutation check, not via a parse error. **Worth an explicit
  test case**, because it is the one input class that turns garbage into a well-formed type.

### G5 — Every 2.1 channel-map test must drive a non-identity label map *(HIGH — C1, restated concretely)*

The shipped default (ARCHITECTURE §3.2.4) is the identity map: speaker N → 7.1 buffer index N−1. Under
`create7point1()` it produces `speakerToBuffer == {0,1,2,3,4,5,6,7}` — byte-identical to a hardcoded
map. A test using only the default is **vacuous**, and F1 established that no choice of accepted
container can rescue it.

Two cheap non-vacuous cases, both available at 2.1 with no hardware:

- **Permuted labels, same container.** Assign a rotated or reversed label set under `create7point1()`
  and assert the resulting `speakerToBuffer` equals a known non-identity permutation. A hardcoded map
  fails this immediately.
- **Cross-container, free.** Store 7.1 types, negotiate `create7point1SDDS()`. The two sets differ in
  4 of 8 types (`leftSurroundSide`/`rightSurroundSide`/`leftSurroundRear`/`rightSurroundRear` vs
  `leftSurround`/`rightSurround`/`leftCentre`/`rightCentre` — `.cpp:567` vs `:568`), so four lookups
  return `-1`. That *is* the ROADMAP:131 missing-label test, obtained for free from real JUCE sets
  rather than a synthetic one.

### G6 — ARCHITECTURE's Layer 1 snippet uses `EXPECT_EQ`, which does not exist here *(LOW, but flag it)*

§3.2.5's Layer 1 pseudocode calls `EXPECT_EQ` — a GoogleTest macro. Per Q5 there is no GoogleTest in
this repo. This is illustrative pseudocode, not a dependency declaration; transcribe it to the console-app
`check(...)` idiom. Flagged so the executor does not resolve the symbol by adding the framework.

### G7 — Derive the harness's plugin version from the target *(MEDIUM, cheap now)*

`O-ReverseDelay/tests/render-harness/CMakeLists.txt` documents `JucePlugin_VersionString` drifting from
the plugin **twice** (stuck at 1.2.0 across two releases, then at 1.5.0 across three more) because a
literal plus a "keep in sync" comment is not a mechanism — `pattern_test_fixture_mirrors_drift_silently`
again. The fix in place there:

```cmake
get_target_property(_VER OuariconOctagon JUCE_VERSION)   # juce_add_plugin's own record
```

O-Octagon has no factory presets at 2.1, so the value is not yet load-bearing — which is exactly why
it costs nothing to do correctly now, before something depends on it.

### G8 — Guard `createEditor()` with `#if JUCE_WEB_BROWSER` at 2.1, not at Stage 3 *(MEDIUM)*

`plugins/O-Octagon/CMakeLists.txt:73-78` already sets `JUCE_WEB_BROWSER=1` on the plugin target, with a
comment stating its purpose is to let the harness write this guard *before* the Stage-3 swap
(`pattern_render_harness_breaks_on_webview_editor`). The harness will define `JUCE_WEB_BROWSER=0`.

Today `createEditor()` returns `GenericAudioProcessorEditor` unconditionally
(`PluginProcessor.cpp:232-239`) and both builds compile. Write the guard anyway while it is two lines
and provably inert — at Stage 3.1 it becomes a build break in a target nobody is looking at.

### G9 — `-Wswitch-enum` confirmed in `juce_recommended_warning_flags` *(confirms a CONTEXT constraint)*

Verified at `JUCEHelperTargets.cmake:73` (Clang) and `:103` (GCC), inside the
`juce_recommended_warning_flags` interface target that `O-Octagon/CMakeLists.txt:63` links `PUBLIC`.
The constraint holds: no `switch` on `AudioChannelSet::ChannelType`. JUCE's own
`getChannelTypeFromAbbreviation` (`.cpp:282-389`) is a flat `if`-chain for the same reason — use it as
the model, and prefer it outright per G4.

---

## Reuse — existing modules and precedent

| Source | Reuse | Notes |
|---|---|---|
| `O-ReverseDelay/tests/render-harness/CMakeLists.txt` | **Template for both new targets** | Closest analog: effect, Stage-2 DSP gate, WebView editor, version-derivation already solved (G7) |
| `O-ReverseDelay/tests/render-harness/main.cpp:808-824` | Pass/fail idiom | `int failures` + `check()` + hard exit code |
| `juce::AudioChannelSet::getChannelTypeFromAbbreviation` | Replaces the hand-rolled §3.2.4 table | G4 — mind the numeric branch |
| `juce::AudioChannelSet::getAbbreviatedChannelTypeName` (`.h:550`) | Venue-tree serialisation | The store direction; already specified |
| `O-MicrotonalSampler/tests/fixtures/4-layer/generate.py` | Only Python-under-`tests/` precedent in the repo | Thin — `tests/tools/` is new ground; D4's DBAP reference lands beside `gen_juce_channel_order.py` |
| `modules/` registry | **No applicable shared module** | Checked: nothing covers geometry, hull, or channel mapping. Nothing to `/module-add`. |

No new third-party dependency is required or recommended for this phase.

---

## Pitfalls carried from the knowledge base

| Pattern | Bites where |
|---|---|
| `critical_audiochannelset_is_a_bitset_not_an_order` | The whole phase. G5 is its concrete test consequence |
| `pattern_standalone_canonical_channelset_oob` | **G1** — reappears at the map-application site, not just the loop bound |
| `pattern_test_fixture_mirrors_drift_silently` | G2, G3, G7 — three separate instances in this phase alone |
| `pattern_worktree_isolation_wrong_for_untracked_scope` | `stages/2-dsp/` is untracked. **Do not execute 2.1 in an isolated worktree** — the scope would vanish and every gate would pass vacuously |
| `pattern_render_harness_breaks_on_webview_editor` | G8 |
| `build_script_target_name_vs_folder` | Target is **`OuariconOctagon`**, folder is `O-Octagon`. Both test CMakeLists must use the target name |
| `critical_dual_binary_data_namespace_collision` | Not yet — no `juce_add_binary_data` until 3.1 |

---

## Open items handed to the plan phase

1. **Test-target naming.** `O-Octagon-geometry-test` / `O-Octagon-render-test` proposed. Sibling
   products use the folder name in `PRODUCT_NAME` while depending on the CMake target — keep that
   split.
2. **Where the SAFE/REAL branch lives** in `processBlock` (Q1). A local branch at 2.1 is fine; 2.2
   replaces the body wholesale. Plan should decide whether the branch predicate is a small named
   helper so `GainStage` inherits it rather than re-deriving G1.
3. **Whether Layer 1 also runs as a Debug `jassert` in `rebuildChannelMap()`** (Q3 partial
   mitigation). Cost is ~6 lines; benefit is a JUCE-drift signal that does not depend on anyone
   running the test target.
4. **The 200-point hull-projection fixture** (ROADMAP:135) — generated in-test from a fixed seed, or
   committed? Prefer generated-with-a-pinned-seed: no binary fixture to drift, and the brute-force
   oracle is in the same file.
5. **Log the CI gap** (Q3) as an explicit Stage-4 or repo-level item — a secrets-free push/PR
   workflow running `-DOUARICON_BUILD_TESTS=ON`. It benefits all twelve existing harnesses and must
   not be quietly dropped.

## Still-open manual gate

**Task 13 — Logic 8-channel negotiation** remains due **before 2.1 execute** (D2). Unchanged by this
research; nothing above depends on its result.
