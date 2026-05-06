---
phase: 23-extract
plan: 04
status: complete-with-followup
duration: ~12min (build/install) + human Dorico smoke test
tasks: 4
files_modified: 3
requirements_satisfied: [MOD-05, LYR-03, LYR-04]
deviations: 1
follow_up: 23-05-fix-au-link-steinberg-symbols (REQUIRED before Phase 24)
---

# Plan 23-04: Version + README + Dorico Smoke Test — SUMMARY

## Outcome

O-Lyrica v2.3.0 ships with VST3 Note Expression support for Dorico microtonal playback. All 5 sub-tests of the LYR-03 acceptance gate passed in Dorico via VST3. Module README documents consumer integration, JUCE patch management, and Dorico end-user setup (MOD-05). Phase 23's primary requirements are satisfied.

**Critical follow-up:** AU build is broken at link time due to module-level architectural defect discovered during this plan. Phase 24 propagation is blocked until Plan 23-05 lands.

## Tasks completed

| # | Task | Commit | Result |
|---|------|--------|--------|
| 1 | Version bump (CMakeLists VERSION + CHANGELOG) | `e695256` | `VERSION "2.3.0"` inside `juce_add_plugin(OLyrica ...)`; new `[2.3.0] - 2026-04-24` CHANGELOG entry above [2.2.2] with all 4 sections |
| 2 | Comprehensive module README (MOD-05) | `40dfe35` | 223-line README at `modules/tuning/note-expression/README.md` covering Quick Start / Features / Installation / Dorico End-User Setup / JUCE Patch Management / Integration Approach |
| 3 | Rebuild + clean install | (no commit — system-bundle install) | VST3 v2.3.0 freshly built and installed; AU re-link FAILED (D-23-04-A); `auval -a \| grep -i lyrica` returns no hits |
| 4 | Dorico quarter-sharp smoke test (LYR-03 gate) | (human-verify) | All 5 sub-tests passed via VST3 |

Closeout commit: this SUMMARY + STATE/REQUIREMENTS/ROADMAP updates.

## Dorico smoke-test results (LYR-03)

User-reported outcome (`approved` resume signal — all pass):

| # | Test | Result |
|---|------|--------|
| 1 | Quarter-sharp C4 pitch accuracy (~269 Hz vs 261.6 Hz) | PASS |
| 2 | No attack zipper on quarter-sharp C4 | PASS |
| 3 | noteId correlation (multi-note chord) — only D4 detuned | PASS |
| 4 | TuningEngine composition with JI Scala tuning | PASS |
| 5 | Retrigger safety — `exchange(0.0)` consumes pending slot | PASS |

LYR-03 gate cleared.

## Key files

- `plugins/O-Lyrica/CMakeLists.txt:12` — `VERSION "2.3.0"` inside `juce_add_plugin(OLyrica ...)`
- `plugins/O-Lyrica/CHANGELOG.md:5-30` — `## [2.3.0] - 2026-04-24` entry with Added / Changed / Removed / Technical notes sections
- `modules/tuning/note-expression/README.md` — full rewrite (31-line stub → 223 lines)
- `~/Library/Audio/Plug-Ins/VST3/O-Lyrica-dev.vst3` — installed v2.3.0 VST3 bundle (Apr 25 08:14)
- `~/Library/Audio/Plug-Ins/Components/O-Lyrica-dev.component` — NOT INSTALLED (AU re-link failed)

## Deviations

### D-23-04-A: AU re-link failure exposed module-level architectural defect

When Task 1's VERSION change triggered a CMake reconfigure, Ninja attempted to re-link the AU bundle. It failed with:

```
Undefined symbols for architecture arm64:
  Steinberg::Vst::INoteExpressionController::iid
  Steinberg::UString::assign(char16_t const*, int)
  Steinberg::FUnknown::iid
```

**Root cause:** the `#if JucePlugin_Build_VST3` guards in `modules/tuning/note-expression/cpp/NoteExpression.h` (added in Plan 23-03 commit `f85ff38`) are evaluated at every translation unit's compile site. SharedCode (`libO-Lyrica-dev_SharedCode.a`) is compiled with `JucePlugin_Build_VST3=1` because the plugin's FORMATS list includes VST3. With LTO, SharedCode IR carries Steinberg symbol references that the AU link line (which does NOT link `pluginterfaces`) cannot resolve.

**Plan 23-03's "OLyrica_AU built clean" claim was incorrect.** The on-disk AU artefact at the time of that SUMMARY was timestamped Apr 13 — pre-dating the Plan 23-03 refactor commits. The AU re-link with Plan 23-03's source changes was never executed during Plan 23-03's verify step. Plan 23-04 Task 1's reconfigure was the first real AU re-link, which exposed the latent breakage.

**Impact:**
- VST3 path works correctly (Dorico smoke test 5/5 passed)
- AU path is broken at link time
- Affects all 7 Phase 24 propagation targets (O-Bells, O-IntonationPad, O-Prism, O-Wind, O-Reed, O-Bowed, O-Formant) — they would all hit the same link error

**Resolution path (deferred to follow-up plan):**
Plan 23-05 (`fix-au-link-steinberg-symbols`) — move the `Controller` class definition and the Steinberg-touching parts of `VST3Extensions::queryIEditController` from `NoteExpression.h` into a `.cpp` that is compiled ONLY into the VST3 target (not into SharedCode). Likely requires a small extension to `OuariconModules.cmake` to support per-format module sources.

**Why deferred, not in-place:** This is a module-level architectural change affecting downstream consumers. It deserves its own plan with proper visibility and a Phase 24 dependency edge — not a "while we're here" tweak inside Plan 23-04.

## Hand-off

**Phase 23 status after this plan:**
- ✓ All 12 stated requirements (MOD-01..08, LYR-01..04) satisfied
- ⚠ AU build broken (not stated as a phase requirement, but a real defect exposed during execution)
- ⚠ Phase 24 BLOCKED until Plan 23-05 lands

**Phase 23 stays open** with Plan 23-05 added to roadmap. Cannot start Phase 24 until 23-05 is complete and AU build is verified clean across all relevant plugins.

## Self-Check: PASSED (with documented follow-up)
