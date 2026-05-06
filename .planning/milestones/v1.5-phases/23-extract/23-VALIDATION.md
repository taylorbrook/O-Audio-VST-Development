---
phase: 23
slug: extract
status: manual-only
nyquist_compliant: false
wave_0_complete: n/a
created: 2026-04-26
reconstructed_from: SUMMARY artifacts (State B — phase already executed and verified)
---

# Phase 23 — Validation Strategy (Reconstructed)

> Phase 23 extracts the spike's note-expression code into a registered shared module
> (`modules/tuning/note-expression/` v1.0.0) and refactors O-Lyrica to consume it.
> Output is build-system infrastructure + a C++ header library + bash/CMake tooling
> — there is no runtime UI, no API surface, and no algorithmic DSP being introduced
> (the helpers are 1:1 extractions of spike-validated math).
>
> Verification during execution leaned on grep audits, build verification, the
> human-verified Dorico smoke test (LYR-03), and `auval` runtime validation.
> Per the developer's choice during validate-phase, no new automated tests are
> being added retroactively — the existing verification mechanisms are recorded
> here as manual-only with their concrete recovery commands.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | None (no pytest/jest/CTest in this repo); JUCE-CMake build + bash verifiers |
| **Config file** | `CMakeLists.txt` (build is the integration test) |
| **Quick run command** | `cmake --build build --target OLyrica_VST3 OLyrica_AU OLyrica_Standalone` |
| **Full suite command** | `cmake --build build` + `scripts/verify-au-link.sh OLyrica` + manual Dorico smoke test |
| **Plugin validator** | `pluginval` (validated at SessionStart) — applies post-build, not per-requirement |
| **Estimated runtime** | ~2-5 min for clean build; ~10 min including Dorico smoke test |

---

## Sampling Rate

- **After every task commit:** Build only the changed plugin/module target.
- **After every plan wave:** Full clean rebuild + `scripts/verify-au-link.sh OLyrica`.
- **Before `/gsd-verify-work`:** All three OLyrica targets link clean + Dorico LYR-03 5/5 PASS.
- **Max feedback latency:** N/A (no automated test loop in this phase).

---

## Per-Task Verification Map

| Task ID | Plan | Requirement | Verified Via | Evidence | Status |
|---------|------|-------------|--------------|----------|--------|
| 23-01-01 | 01 | MOD-01, MOD-08 | Filesystem + grep audit | `modules/tuning/note-expression/module.yaml` exists; registry entry confirmed | ✅ green (manual) |
| 23-01-02 | 01 | MOD-02, MOD-03, MOD-04, MOD-06 | grep audit (15-check matrix in 23-01 SUMMARY) | `class Controller`, `class VST3Extensions`, `inline double applyPendingTuning` present; 0 hits for `neTrace`/`iidToHex`/`<fstream>`/`<mutex>` | ✅ green (manual) |
| 23-02-01 | 02 | MOD-07 | `patch -p1 --dry-run` against pristine JUCE 8.0.4 | exit 0 (verified during Plan 23-02 execution) | ✅ green (manual) |
| 23-02-02 | 02 | MOD-07 | Live exec of `apply-juce-patches.sh` (idempotency double-run + preflight failure) | First run: skip exit 0; second run: identical output exit 0; missing JUCE_DIR: red error exit 1 | ✅ green (manual) |
| 23-02-03 | 02 | (D-15) | `cd build && cmake ..` against full plugin tree | hook fires for 0 existing plugins (correctly scoped); configure exits 0 across 15 plugins | ✅ green (manual) |
| 23-03-01 | 03 | LYR-01 | grep `ouaricon_add_module(OLyrica note-expression)` in `plugins/O-Lyrica/CMakeLists.txt:79` | 1 hit | ✅ green (manual) |
| 23-03-02 | 03 | LYR-01 | grep + filesystem | `NoteExpressionSupport.h` deleted; `Ouaricon::NoteExpression::VST3Extensions vst3Extensions` declared in PluginProcessor.h | ✅ green (manual) |
| 23-03-03 | 03 | LYR-02 | Line-ordering grep in `HarpSynthVoice::startNote` | `getFrequency` at relative line 11; `applyPendingTuning` at relative line 41 (TuningEngine first, D-10) | ✅ green (manual) |
| 23-03-04 | 03 | (build gate) | `ninja OLyrica_VST3 OLyrica_AU` clean | VST3 built; AU surfaced D-23-04-A → escalated to Plan 23-05 | ✅ green (after 23-05) |
| 23-04-01 | 04 | LYR-04 | grep `VERSION "2.3.0"` + `## [2.3.0] - 2026-04-24` heading | both confirmed in `CMakeLists.txt:12` and `CHANGELOG.md:5-30` | ✅ green (manual) |
| 23-04-02 | 04 | MOD-05 | grep README sections (6 required H2s) | all 6 present in 223-line README | ✅ green (manual) |
| 23-04-03 | 04 | (deploy) | macOS Plugin Cache Clearing protocol per CLAUDE.md | VST3 + AU bundles re-installed | ✅ green (manual) |
| 23-04-04 | 04 | LYR-03 | Dorico quarter-sharp smoke test 5/5 | user-confirmed PASS via VST3 | ✅ green (human-verified) |
| 23-05-01 | 05 | (D-22) | grep `<pluginterfaces/` in `cpp/NoteExpression.h` + `cpp/NoteExpression.cpp` | 0 hits (regression guard for D-23-04-A) | ✅ green (manual) |
| 23-05-02 | 05 | (D-22) | grep `Steinberg::` / `<pluginterfaces/` in `cpp/vst3/NoteExpression_VST3.cpp` | 82 hits (consolidation correct) | ✅ green (manual) |
| 23-05-03 | 05 | (D-25-28) | grep `OuariconModules.cmake` for narrowed `file(GLOB ` (not GLOB_RECURSE) + per-format loop | both confirmed (lines 75-90) | ✅ green (manual) |
| 23-05-04 | 05 | (D-30) | `ninja OLyrica_VST3 OLyrica_AU OLyrica_Standalone` | three artefacts produced; zero undefined-symbol failures | ✅ green (manual) |
| 23-05-05 | 05 | (D-30) | `scripts/verify-au-link.sh OLyrica` → `auval -v aumu OLyr OuDv` | AU bundle loads, RENDER + MIDI tests PASS (APVTS Meta-Flag finding deferred — see deferred-items.md) | ✅ green (modulo deferred item) |
| 23-05-06 | 05 | LYR-03 | Re-run Dorico quarter-sharp smoke test 5/5 against rebuilt VST3 | user-confirmed PASS — zero behavioral regression from AU-link refactor | ✅ green (human-verified) |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

Existing infrastructure (build + bash verifiers + grep audits + Dorico smoke test) covers all phase requirements via manual-only verification. No Wave 0 work required because no automated test framework was introduced (or is being introduced) for this phase.

---

## Manual-Only Verifications

All Phase 23 verifications are manual-only by developer choice (validate-phase decision 2026-04-26). The grep audits, build steps, and runtime checks below were the actual verification mechanisms used during execution and are the canonical replay procedure for any future regression check.

| # | Behavior | Requirement | Why Manual | Test Instructions |
|---|----------|-------------|------------|-------------------|
| 1 | Module exists at `modules/tuning/note-expression/` with `module.yaml`, `README.md`, `cpp/NoteExpression.{h,cpp}`, `cpp/vst3/NoteExpression_VST3.cpp`, `module.cmake` | MOD-01 | Filesystem assertion; grep / `ls` is the natural form | `ls modules/tuning/note-expression/{module.yaml,README.md,module.cmake,cpp/NoteExpression.h,cpp/NoteExpression.cpp,cpp/vst3/NoteExpression_VST3.cpp}` — all six paths must exist |
| 2 | Module advertises `Controller` for `kTuningTypeID` | MOD-02 | Static structural check | `grep -l "class Controller" modules/tuning/note-expression/cpp/NoteExpression.h modules/tuning/note-expression/cpp/vst3/NoteExpression_VST3.cpp` — both files must match |
| 3 | `VST3Extensions` subclass with raw-event queue + `queryIEditController` | MOD-03 | Static structural check | `grep -c "class VST3Extensions : public juce::VST3ClientExtensions" modules/tuning/note-expression/cpp/NoteExpression.h` — must be ≥1 |
| 4 | Header-only voice helper `applyPendingTuning` (single `std::pow` call site in module) | MOD-04 | Pure-function correctness was validated in spike (LYR-03 covers runtime); structure is grep-checkable | `grep -c "inline double applyPendingTuning" modules/tuning/note-expression/cpp/NoteExpression.h` — must be 1; `grep -c "std::pow" modules/tuning/note-expression/cpp/NoteExpression.h` — must be 1 |
| 5 | Module README has all 6 required H2 sections (Quick Start, Features, Installation, Dorico End-User Setup, JUCE Patch Management, Integration Approach) | MOD-05 | Documentation completeness; grep is appropriate | `for h in 'Quick Start' 'Features' 'Installation' 'Dorico End-User Setup' 'JUCE Patch Management' 'Integration Approach'; do grep -q "^## $h" modules/tuning/note-expression/README.md || echo MISSING: $h; done` |
| 6 | Zero diagnostic spike code in module tree | MOD-06 / D-18 | Forbidden-pattern grep | `grep -rE "neTrace\|iidToHex\|<fstream>\|<mutex>\|OLyrica::detail\|namespace OLyrica" modules/tuning/note-expression/` must return 0 matches |
| 7 | JUCE patch is committed `.patch` file with re-apply procedure | MOD-07 / D-12 | Bash test against existing artifact | `head -25 scripts/juce-patches/note-expression-juce-8.0.4.patch \| grep -q "JUCE-NE-PATCH"` (header marker); `grep -c "^--- a/" scripts/juce-patches/note-expression-juce-8.0.4.patch` must be 2 (two hunks); `bash -n scripts/apply-juce-patches.sh` must exit 0 |
| 8 | `apply-juce-patches.sh` idempotency: double-run produces identical output | MOD-07 / D-14 | Side-effect-free repeatability check; depends on JUCE_DIR being patched (developer machine state) | `./scripts/apply-juce-patches.sh > /tmp/apply-1.log 2>&1 && ./scripts/apply-juce-patches.sh > /tmp/apply-2.log 2>&1 && diff /tmp/apply-1.log /tmp/apply-2.log` — diff must be empty, both runs exit 0 |
| 9 | `apply-juce-patches.sh` preflight fails when `JUCE_DIR` is missing | MOD-07 / T-23-04 | Negative test — easier to run interactively than wire as automated | `JUCE_DIR=/tmp/nonexistent-juce-dir ./scripts/apply-juce-patches.sh; echo $?` — must print `1` and surface a red error message naming the script |
| 10 | Module is registered in registry with semver alignment between `module.yaml` and `registry.yaml` | MOD-08 | YAML consistency check | `grep -A2 "name: note-expression$" modules/registry.yaml \| grep "version: 1.0.0"` and `grep "version: \"1.0.0\"\|version: 1.0.0" modules/tuning/note-expression/module.yaml` — both must match |
| 11 | O-Lyrica consumes the module; spike header is deleted; no plugin-local NE state remains | LYR-01 | Filesystem + forbidden-pattern grep | `test ! -e plugins/O-Lyrica/Source/VST3/NoteExpressionSupport.h` (must succeed); `grep -c "ouaricon_add_module(OLyrica note-expression)" plugins/O-Lyrica/CMakeLists.txt` must be 1; `grep -rE "pendingTuningSemis\|rawEventScratch\|LyricaVST3Extensions" plugins/O-Lyrica/Source/` must return 0 matches |
| 12 | NE composes with TuningEngine (`getFrequency` precedes `applyPendingTuning` in voice startup) | LYR-02 / D-10 | Source-line ordering inspection | Open `plugins/O-Lyrica/Source/HarpSynthVoice.cpp` `startNote`. `getFrequency` call must appear before `applyPendingTuning` call. The remaining `std::pow` calls in the file (humanize and glissando math) are NOT on the NE path and are out of scope |
| 13 | **Dorico quarter-sharp smoke test 5/5** | LYR-03 | Requires Dorico + a JUCE host wrapping a real VST3 (no NE-event-injection test harness exists in this codebase); the canonical Dorico-microtonal acceptance gate | (a) Open Dorico, load a project that targets O-Lyrica via VST3 with a quarter-sharp accidental on C4. (b) Confirm pitch ≈ 269.29 Hz (+50¢ above 261.63 Hz). (c) Confirm no attack zipper. (d) Confirm noteId correlation: a multi-note chord with a single quarter-sharp detunes only that note. (e) Compose with a JI Scala tuning — quarter-sharp should sit +50¢ above the JI base. (f) Retrigger the same pitch — second hit should be 12-TET (proves `exchange(0.0)` consumed the slot). All five sub-tests must PASS |
| 14 | O-Lyrica VERSION bumped to 2.3.0 with CHANGELOG entry | LYR-04 | Static metadata grep | `grep -E 'VERSION "2\.3\.0"' plugins/O-Lyrica/CMakeLists.txt` (1 hit, inside `juce_add_plugin(OLyrica ...)` block); `grep -E "^## \[2\.3\.0\] - 2026-04-24" plugins/O-Lyrica/CHANGELOG.md` (1 hit) |
| 15 | SharedCode TU is Steinberg-free; VST3 TU consolidates Steinberg refs (D-22 regression guard for D-23-04-A) | Plan 23-05 D-22 | Anti-regression structural check | `grep -c "<pluginterfaces/" modules/tuning/note-expression/cpp/NoteExpression.{h,cpp}` must be 0 in both; `grep -c "<pluginterfaces/" modules/tuning/note-expression/cpp/vst3/NoteExpression_VST3.cpp` must be ≥1 (≥4 in current state) |
| 16 | Per-format CMake routing convention is in place | Plan 23-05 D-25-28 | Anti-regression structural check on `OuariconModules.cmake` | `grep -E "^\s*file\(GLOB " modules/cmake/OuariconModules.cmake` (non-recursive top-level glob; D-27); `grep -E "_OUA_JUCE_FORMATS\s+vst3 au standalone" modules/cmake/OuariconModules.cmake` (per-format loop iteration; D-25-28); `grep -c "target_sources(\${TARGET_NAME}_\${_FMT_UPPER}" modules/cmake/OuariconModules.cmake` must be ≥1 |
| 17 | OLyrica VST3 + AU + Standalone all link cleanly (D-30 acceptance gate) | Plan 23-05 D-30 | Build-system test; reuse `verify-au-link.sh` pattern | `cmake --build build --target OLyrica_VST3 OLyrica_AU OLyrica_Standalone` must produce all three artefacts with zero `Undefined symbols for architecture arm64` errors mentioning either `Steinberg::*` or `Ouaricon::NoteExpression::VST3Extensions::*` |
| 18 | OLyrica AU bundle loads in `auval` (D-30 runtime gate) | Plan 23-05 D-30 | Existing wrapper script | `scripts/verify-au-link.sh OLyrica` — RENDER + MIDI tests must PASS. Note: `auval` exits 255 on the deferred APVTS Meta-Flag finding (parameter ID 1275870432) — this is pre-existing O-Lyrica parameter-implementation issue, NOT a Phase 23 regression. See `.planning/phases/23-extract/deferred-items.md` |

---

## Validation Sign-Off

- [x] All tasks have a manual verification path documented above
- [x] All MISSING gaps explicitly classified as manual-only by developer decision
- [x] Manual-only items 1-18 have concrete reproduction commands
- [x] Existing tooling (`scripts/verify-au-link.sh`, `scripts/apply-juce-patches.sh`, `scripts/verify-backup.sh`) covers the heavy verification surface
- [x] Dorico smoke test (item 13) is human-verified and signed off twice (Plan 23-04 + Plan 23-05 re-run)
- [ ] `nyquist_compliant: true` — NOT set; Phase 23 is intentionally manual-only (audio-host-dependent + build-system phase, no algorithmic surface to unit-test in isolation)

**Approval:** approved 2026-04-26 (manual-only acknowledged; Phase 24 propagation may proceed)

---

## Validation Audit 2026-04-26

| Metric | Count |
|--------|-------|
| Gaps found | 18 |
| Resolved (automated) | 0 |
| Documented as manual-only | 18 |
| Escalated | 0 |

**Decision rationale (developer-selected):** Phase 23 produces build-system infrastructure + a header-only C++ helper module + bash/CMake tooling. The runtime correctness is dominated by the Dorico microtonal smoke test (LYR-03), which inherently requires a real VST3 host driving NE events from a notation environment — not unit-testable without authoring a substantial VST3 host stub that is itself out of scope for this phase. Static-structural truths (forbidden patterns, file existence, CMake convention) are correctly captured as manual grep audits replayable in seconds. Adding bespoke bash test scripts for them would duplicate the verification work already performed and committed in `23-VERIFICATION.md` without changing the regression risk profile, given that Phase 24's per-plugin verify gates re-run the same audits per plugin.
