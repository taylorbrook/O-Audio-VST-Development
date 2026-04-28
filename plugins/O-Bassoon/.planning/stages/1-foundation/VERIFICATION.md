# Stage 1: Foundation — Verification

## Verification Date

2026-04-27

## Goal-Backward Analysis

### Original Goals (from CONTEXT.md / PLAN.md)

1. Stand up `plugins/O-Bassoon/` as a buildable, host-loadable, **silent** VST3/AU/Standalone synth shell.
2. Lock the 10-parameter APVTS spec (9 Float + 1 Int) and expose it through the host.
3. Wire both shared modules headless from day one — `note-expression` v1.1.0 (via `ouaricon_add_module`) and `scala-tuning-engine` v2.x (via direct file refs).
4. Commit Windows WebView CMake flags up-front (`NEEDS_WEB_BROWSER TRUE`, `NEEDS_WEBVIEW2 TRUE`, `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`) so Stage 3 only changes editor C++.
5. Ensure NE drain (`vst3Extensions.drainAndUpdate()`) runs **before** `renderNextBlock` from Stage 1 — voices ignore deltas (silent stub) until Phase 2.4.
6. `BassoonVoice` ships as a silent stub — no audio out — Phase 2.1 is first audio.
7. Absorb the three RESEARCH.md discrepancies: D1 (no `Ouaricon::note_expression` CMake target), D2 (`TuningEngine` is **global** namespace), D3 (`NEEDS_WEBVIEW2 TRUE`).
8. Verify COMPAT-01 (pluginval pass) and DSP-07 (no O-Reed dependency).

### Deliverables (from SUMMARY.md + on-disk verification)

1. `plugins/O-Bassoon/CMakeLists.txt` — `juce_add_plugin OBsn`, all flags committed (lines 11, 15, 16, 19, 20), 4 scala-tuning-engine direct sources (lines 33-36), `ouaricon_add_module(O-Bassoon note-expression)` (line 40), `juce_generate_juce_header` after `target_link_libraries` (line 72), `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` compile-def (line 86).
2. `Source/BassoonSound.h` — header-only `juce::SynthesiserSound` catch-all (`appliesToNote/Channel` → `true`).
3. `Source/BassoonVoice.{h,cpp}` — silent stub. Three setters wired (`setAPVTS`, `setTuningEngine` (global ns), `setPendingTuningSource`); `renderNextBlock` is no-op (BassoonVoice.cpp:45).
4. `Source/PluginProcessor.{h,cpp}` — `OBassoonAudioProcessor` with APVTS (10 params verified at PluginProcessor.cpp:25–99), `juce::Synthesiser`, `TuningEngine` member (PluginProcessor.h:62, global ns), `Ouaricon::NoteExpression::VST3Extensions` member (PluginProcessor.h:63), `getVST3ClientExtensions()` returns `&vst3Extensions` (PluginProcessor.h:57), 16 voices pre-allocated in ctor with all three setters fired, `processBlock` runs `vst3Extensions.drainAndUpdate()` BEFORE `synthesiser.renderNextBlock` (PluginProcessor.cpp:170, 174).
5. `Source/PluginEditor.{h,cpp}` — `juce::GenericAudioProcessorEditor` placeholder, 500x480.

### Goal Achievement

| Goal | Status | Evidence |
|------|--------|----------|
| 1. Buildable silent shell | ✅ Achieved | `cmake --build build --target O-Bassoon_VST3 O-Bassoon_AU O-Bassoon_Standalone` clean (re-run 2026-04-27 — `ninja: no work to do`); artefacts present at `build/.../Release/{VST3,AU,Standalone}/O-Bassoon-dev.{vst3,component,app}` |
| 2. 10-param APVTS spec | ✅ Achieved | All 10 IDs grepped in `PluginProcessor.cpp:25-99` matching frozen spec |
| 3. Both shared modules wired | ✅ Achieved | `ouaricon_add_module(O-Bassoon note-expression)` at CMakeLists.txt:40; 4 `scala-tuning-engine/cpp/*.cpp` at lines 33-36; `target_include_directories` adds `scala-tuning-engine/cpp` at line 46 |
| 4. Windows WebView flags committed | ✅ Achieved | `NEEDS_WEB_BROWSER TRUE` (CMakeLists.txt:19), `NEEDS_WEBVIEW2 TRUE` (line 20), `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` (line 86) |
| 5. NE drain before renderNextBlock | ✅ Achieved | `PluginProcessor.cpp:170` (`drainAndUpdate`) precedes `:174` (`renderNextBlock`) |
| 6. Silent BassoonVoice stub | ✅ Achieved | `BassoonVoice.cpp:45` `renderNextBlock` body empty; no DSP code; pluginval round-trip yielded silent output (clean exit) |
| 7. Discrepancies D1/D2/D3 absorbed | ✅ Achieved | D1: no `Ouaricon::note_expression` target_link_libraries line (grep empty). D2: no `Ouaricon::TuningEngine` token in Source/ or CMakeLists (grep empty); members declared as `TuningEngine` at PluginProcessor.h:62 + BassoonVoice.h:49. D3: `NEEDS_WEBVIEW2 TRUE` at CMakeLists.txt:20 |
| 8. COMPAT-01 + DSP-07 verified | ⚠️ Partial / ✅ | COMPAT-01: pluginval **strictness 5** SUCCESS on macOS (full strictness 10 + Windows is Stage 4). DSP-07: `grep` of `plugins/O-Bassoon/{Source,CMakeLists.txt}` for `O-Reed\|OReed` returns empty |

## Requirements Verification

**Stage:** 1-foundation
**Requirements scoped to this stage:** 2 (COMPAT-01 must, DSP-07 must)

| Requirement | Priority | Status | Acceptance Criteria |
|-------------|----------|--------|---------------------|
| DSP-07: No O-Reed dependency | must | ✅ Complete | `grep -rn "O-Reed\|OReed" plugins/O-Bassoon/{Source,CMakeLists.txt}` empty; no `#include` of O-Reed headers; CMake references no O-Reed sources |
| COMPAT-01: pluginval pass | must | ⚠️ Partial | pluginval **strictness 5** SUCCESS on macOS VST3 (this stage's gate per CONTEXT.md L90); strictness 10 + Windows VST3 are Stage 4 acceptance — re-verify at Stage 4 |

**Stage 1 requirements summary:**
- ✅ Complete: 1 (DSP-07)
- ⚠️ Partial (final gate at later stage): 1 (COMPAT-01 — strictness-5 pass at Stage 1, strictness-10 + Windows at Stage 4)
- ⏸️ Deferred: 16 (FUNC-*, DSP-01..06, UI-*, PERF-*, COMPAT-02, QUAL-*) — verifiedAt is stage-2 / stage-3 / stage-4

## Automated Checks

| Check | Result | Notes |
|-------|--------|-------|
| `cmake --build build --target O-Bassoon_VST3 O-Bassoon_AU O-Bassoon_Standalone` | ✅ Pass | Re-run 2026-04-27 — `ninja: no work to do` (no source drift since SUMMARY commit) |
| `auval -v aumu OBsn OuDv` | ✅ Pass | `AU VALIDATION SUCCEEDED` |
| `pluginval --strictness 5 ~/Library/Audio/Plug-Ins/VST3/O-Bassoon-dev.vst3` | ✅ Pass | Exit code 0; output ends `SUCCESS` |
| Source-tree grep `O-Reed\|OReed` | ✅ Pass | Empty result for `plugins/O-Bassoon/{Source,CMakeLists.txt}` (DSP-07) |
| Source-tree grep `Ouaricon::TuningEngine` | ✅ Pass | Empty result (D2 — global namespace enforced) |
| Source-tree grep `target_link_libraries.*Ouaricon::note_expression` | ✅ Pass | Empty result (D1 — no non-existent target referenced) |
| `NEEDS_WEBVIEW2 TRUE` in `juce_add_plugin` | ✅ Pass | CMakeLists.txt:20 (D3) |
| `PLUGIN_CODE OBsn` set | ✅ Pass | CMakeLists.txt:11 |
| `juce_generate_juce_header` after `target_link_libraries` | ✅ Pass | CMakeLists.txt:50 (link) → :72 (header gen) |
| 10 APVTS parameters (9 Float + 1 Int) | ✅ Pass | All 10 IDs grepped at PluginProcessor.cpp:25-99 |
| `setLatencySamples()` absent | ✅ Pass | No call in `Source/`; comment at PluginProcessor.cpp:136 documents intent |
| NE drain ordering | ✅ Pass | `drainAndUpdate` (PluginProcessor.cpp:170) precedes `renderNextBlock` (:174) |
| `getVST3ClientExtensions()` returns non-null | ✅ Pass | Returns `&vst3Extensions` (member, not stack temp) at PluginProcessor.h:57 |
| Installed VST3 + AU present | ✅ Pass | `~/Library/Audio/Plug-Ins/VST3/O-Bassoon-dev.vst3`, `~/Library/Audio/Plug-Ins/Components/O-Bassoon-dev.component` |

## Human Verification

- [x] Plugin loaded in DAW without crash (executor confirmed via auval; pluginval round-trip exercises VST3 host-load path)
- [ ] Visual check of all 10 APVTS params in DAW Generic editor (deferred — not Stage 1 blocker; Generic editor auto-renders by virtue of APVTS registration)
- [ ] Windows VST3 build (deferred to Stage 4 — Mac-only Stage 1 build per ROADMAP)

## Issues Found

None. Single deviation noted in SUMMARY (`createPluginFilter()` factory function added — required JUCE plugin entry point, functionally equivalent to O-Wind/O-Lyrica pattern; not a regression).

## Stage Verdict

**Status:** ✅ VERIFIED

**Ready for next stage:** Yes (Stage 2 — DSP, Phase 2.1 first audio)

**Blockers:** None.

**Carry-forward to Stage 2:**
- COMPAT-01 final gate (strictness-10 + Windows VST3) re-verify at Stage 4.
- Reference bassoon recording sourcing — Phase 2.2 kickoff input (per CONTEXT.md "Carried from Stage 0, deferred").
- UI mockup remains a Stage 3 prerequisite, parallel-eligible with Stage 2.

**Verifies requirements:** DSP-07 ✅; COMPAT-01 ⚠️ partial (Stage-1 gate at strictness 5 — full acceptance at Stage 4).
