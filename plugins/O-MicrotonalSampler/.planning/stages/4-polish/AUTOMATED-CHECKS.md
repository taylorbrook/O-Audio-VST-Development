---
title: "O-MicrotonalSampler Stage 4 — Automated checks (working note)"
created: 2026-04-28
phase: 4.4-partial
status: automated_portions_green; awaiting_human_in_loop
inputs:
  - .planning/stages/4-polish/PLAN.md
---

# Stage 4 — Automated checks complete (working note)

This is a **working note**, not the final `VERIFICATION.md`. It captures
the Phase 4.4 tasks that can be executed without human-in-loop work.
The final `VERIFICATION.md` + atomic commit lands at the end of Phase
4.4 once Phases 4.2 (PERF-02), 4.3 (QUAL-01), and the Logic + Dorico
smoke tests are in.

## Bundle under test

- VST3: `~/Library/Audio/Plug-Ins/VST3/O-MicrotonalSampler-dev.vst3`
- AU:   `~/Library/Audio/Plug-Ins/Components/O-MicrotonalSampler-dev.component`
- Source build: `build/plugins/O-MicrotonalSampler/O-MicrotonalSampler_artefacts/Release/`
- Branding: dev (`OUARICON_DEV_SUFFIX=-dev`, `MANUFACTURER_CODE=OuDv`)
- Plugin code: `aumu OMtS OuDv`
- `PLUGIN_VERSION` (CMakeLists.txt:14) = `1.0.0`
- **`OMTS_PHASE_2_1_TEST_FIXTURE` = OFF** (canonical ship config — see "Build-config rectification" below)
- Pre-install state: triple build green (clean rebuild after cache flip); cache-clear + fresh install per CLAUDE.md.

## Build-config rectification (pre-gate fix)

**Issue surfaced during user-side testing prep.** The default-load
sound played one-shot for ~0.5 s and stopped, regardless of MIDI note
length. Root cause: `build/CMakeCache.txt` had
`OMTS_PHASE_2_1_TEST_FIXTURE:BOOL=ON` despite the CMakeLists.txt
default being `OFF` (CMakeLists.txt:118-119). The Phase 2.1 fixture
(`PluginProcessor.cpp:150-230`) generates 88 per-MIDI-note 0.25 s
sine bursts with `loopStart=0; loopEnd=0; // one-shot in 2.1` —
i.e. **one-shot by design**. Fixture was meant for Phase 2.1 dev
testing only and was supposed to be flipped OFF after Phase 2.2
(comment on CMakeLists.txt:115-117 confirms intent: "Phase 2.2 flipped
this OFF (real folder load via SampleLoader supersedes the fixture)").
The cache had drifted from project intent silently — every Stage 2,
Stage 3, and Phase 4.1 build (including Stage 3 verify and the prior
strictness-10 pass-attempt at the start of this gate run) was against
the wrong configuration.

**Not a DSP regression.** Voice DSP, ADSR, loop-detect, and
loader code all correct. The fixture path was simply masking real
ship behavior (start empty → user drops folder → sustain via Phase
2.5 loop-detect). Stage 2 sub-phases stay green; no Stage 2 reopen
required.

**Fix:** `cmake -B build -DOMTS_PHASE_2_1_TEST_FIXTURE=OFF` →
clean triple build → cache-clear + reinstall. All Phase 4.4
automated checks were re-run against the canonical ship binary
(results below); prior fixture-ON pluginval-10 / auval logs were
overwritten — only the canonical run is preserved in the logs/
directory.

**Follow-up note for v1.1:** consider hardening
`OMTS_PHASE_2_1_TEST_FIXTURE` so the cache cannot drift — e.g. force
to `OFF` in CMakeLists for Release builds, or guard with
`if(NOT CMAKE_BUILD_TYPE STREQUAL "Release")`.

## Tasks 12, 13, 14, 15, 18 — results

### Task 12 — Triple build + cache-clear + install
- [x] `ninja O-MicrotonalSampler_VST3 O-MicrotonalSampler_AU O-MicrotonalSampler_Standalone` from `build/` → `ninja: no work to do` (artefacts current).
- [x] `killall -9 AudioComponentRegistrar` + `rm -rf` AU caches and old bundles + fresh `cp -R` of VST3 + AU into `~/Library/Audio/Plug-Ins/`.

### Task 13 — `pluginval --strictness-level 10 --skip-gui-tests`
```
pluginval --strictness-level 10 --validate-in-process \
  --skip-gui-tests --random-seed 0xc0ffee --timeout-ms 120000 \
  ~/Library/Audio/Plug-Ins/VST3/O-MicrotonalSampler-dev.vst3
```
- **Exit:** `0`
- **Tests completed:** 21 (Listing buses, Enabling/Disabling/Restoring buses, Plugin state restoration {x4 numTests}, Audio processing {default + named layouts}, Background thread state, Parameter info / threading / state, Editor consistency [skipped — no GUI], Bus layouts, Reset, Channel modes, Audio sub-block, FuzzParameters)
- **Failures / errors / warnings:** 0
- **Result:** `SUCCESS`
- Log: `.planning/stages/4-polish/logs/pluginval-10-skipgui.log`

**Note on hex seed:** pluginval rejects uppercase hex (`0xC0FFEE` → "Invalid random seed argument"). Use lowercase (`0xc0ffee`). PLAN line referencing `0xC0FFEE` is shape-correct but case-sensitive at the CLI; lowercase form is canonical.

### Task 14 — `pluginval --strictness-level 10` (with GUI)
```
pluginval --strictness-level 10 --validate-in-process \
  --random-seed 0xc0ffee --timeout-ms 120000 \
  ~/Library/Audio/Plug-Ins/VST3/O-MicrotonalSampler-dev.vst3
```
- **Exit:** `0`
- **Tests completed:** 25 (= 21 skip-gui + 4 GUI: `Editor`, `Open editor whilst processing`, `Editor Automation`, plus the editor instantiation pass folded into Plugin state restoration)
- **Failures / errors / warnings:** 0
- **Result:** `SUCCESS`
- Log: `.planning/stages/4-polish/logs/pluginval-10-withgui.log`

This is the first true strictness-10 with-GUI pass for this codebase. WebView shell + parameter relays + native function bindings survive `EditorTest`, `EditorWhilstProcessingTest`, `EditorAutomationTest` at full strictness with no transient failures (no re-runs needed).

### Task 15 — `auval -v aumu OMtS OuDv`
- **Exit:** `0`
- **Result:** `AU VALIDATION SUCCEEDED.`
- DEF-24-01 static-check finding (per `o_lyrica_spike_reference.md`) is benign and does not surface here.
- Log: `.planning/stages/4-polish/logs/auval.log`

### Task 18 — Invariant greps
| Invariant | Command | Expected | Got | Status |
|---|---|---|---|---|
| Latency-zero | `grep -rn setLatencySamples plugins/O-MicrotonalSampler/Source/` | 1 comment-only hit | `PluginProcessor.cpp:133:    // Sampler is feed-forward; latency = 0 — do NOT call setLatencySamples.` | ✅ |
| WebView2 flags (3/3) | `grep -n "NEEDS_WEBVIEW2\|JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING\|withUserDataFolder" CMakeLists.txt PluginEditor.cpp` | All 3 present | `CMakeLists.txt:20 NEEDS_WEBVIEW2 TRUE`; `CMakeLists.txt:109 JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`; `PluginEditor.cpp:58 .withUserDataFolder(...)` | ✅ |
| No `v0.1.0` literal | `grep -rn "v0\.1\.0" plugins/O-MicrotonalSampler/Resources/ plugins/O-MicrotonalSampler/Source/` | zero hits | zero hits | ✅ |
| No new module deps | check `plugins/O-MicrotonalSampler/modules.json` | unchanged since Stage 3 | `modules.json` does not exist (trivially holds — no module deps were declared at any Stage) | ✅ |

## What is still pending (human-in-loop)

| Task | Phase | Why not automated |
|---|---|---|
| 5 | 4.2 | Logic Pro Audio settings (48 k / 256) configured by hand |
| 6 | 4.2 | Logic Pro Performance Meter readings (3 runs × 16-voice held chord) |
| 7 | 4.2 | Decide whether `delta_CPU_pct ≤ 5 %` and flip PERF-02 |
| 8 | 4.2 | Phase 4.2 atomic commit (data only) |
| 9 | 4.3 | 7-item listening checklist |
| 10 | 4.3 | Decide whether all items pass and flip QUAL-01 |
| 11 | 4.3 | Phase 4.3 atomic commit (data only) |
| 16 | 4.4 | Logic Pro AU smoke (16-voice load + audition) |
| 17 | 4.4 | Dorico microtonal smoke (NE expression-map procedure) |
| 19 | 4.4 | Write final `VERIFICATION.md` + STATUS update — depends on 4.2 + 4.3 + 16 + 17 results |
| 20 | 4.4 | Phase 4.4 atomic commit |

## Plan deviations / observations

1. **Hex seed casing.** pluginval requires lowercase hex; `0xc0ffee` was used. Future `--random-seed` references in this project should standardise on lowercase. (Not a defect; flag for the v1.0 reproducibility note.)
2. **No source code changed during automated 4.4 checks.** Phase 4.4 is verification-only at this stage — only `~/Library/Audio/Plug-Ins/...` was touched (cache-clear + reinstall).
3. **No commit landed for this run.** Per PLAN, atomic commits are per-phase. 4.2 + 4.3 + 4.4 commits are still pending.

## Resume protocol

When 4.2 + 4.3 + Logic/Dorico smoke results are in:
1. Append PERF-02 + QUAL-01 results tables to a fresh
   `.planning/stages/4-polish/VERIFICATION.md`.
2. Flip PERF-02 + QUAL-01 in `.planning/REQUIREMENTS.md` from
   `partial` → `complete`, `verified at` = `stage-4`.
3. Append the Logic + Dorico smoke results plus the four invariant
   greps and the three pluginval/auval outcomes already captured in
   this file.
4. Atomic commit Phase 4.2 (data only, before 4.3 starts).
5. Atomic commit Phase 4.3 (data only, before 4.4 starts).
6. Atomic commit Phase 4.4 closing the stage.
7. Update STATUS.md to `stage_4_complete; v1.0 ready for internal use`.

This file (`AUTOMATED-CHECKS.md`) is a working note and can be deleted
once the data is folded into the final `VERIFICATION.md`.
