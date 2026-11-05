# Stage 1: Foundation - Verification

## Verification Date

2026-11-04

All checks below were re-run independently in the verify session (fresh rebuild +
reinstall via `build-and-install.sh`), not copied from SUMMARY.md.

## Goal-Backward Analysis

### Original Goals (from CONTEXT.md)

1. Plugin builds VST3 + AU + Standalone from the v2 CMake scaffold (WebView plumbing wired, Generic editor — D2)
2. All 220 parameters registered in contract order (140 Float / 59 Choice / 21 Bool), 113-entry append-only mod-dest table
3. Bitwise pass-through shell across mono→mono, mono→stereo, stereo→stereo (D3); latency 0
4. State codec: int64 seed, canonical masks, shared custom scale, uiLanguage; reopen-safe
5. pitch-collections module extracted (edo, badEdo, offending token, toScala) and O-GrainScatter migrated to 3.6.0 bit-identically (D1)
6. COMPAT-01: pluginval 10 VST3 + AU, auval PASS

### Deliverables (from SUMMARY.md)

1. `CMakeLists.txt` from v2 scaffold; UI resources + `granulatorUiResource()` resource map; `createEditor()` → Generic
2. `GranulatorParamIds.h` frozen Def tables, literal `kModDestIds[113]`, `kModDestTableVersion = 1`
3. `PluginProcessor` pass-through bounded by buffer channels; `setLatencySamples(0)` in `prepareToPlay()`
4. `<Engine>`, `<CustomScale>`, `<DegreeMasks>` children written on the `copyState()` copy; restore resets first
5. `modules/tuning/pitch-collections` 1.0.0 (f7398147); O-GrainScatter 3.6.0 (a480ac41)
6. Installed `O-Granulator-dev.{vst3,component}`

### Goal Achievement

| Goal | Status | Evidence |
|------|--------|----------|
| 1 Build | ✅ Achieved | Touch-rebuild of `PluginProcessor.cpp` + `PluginEditor.cpp`: VST3/AU/Standalone/harness compile with 0 warnings, 0 errors. Standalone launches, alive after 5 s, exits on SIGTERM |
| 2 Params | ✅ Achieved | Independent diff: param-dump ID column == `parameter-spec.md` § Full ID list (220/220, order exact); spec types 140/59/21. Dump == committed `.planning/params.tsv`. Harness G1–G6, D1–D4 PASS |
| 3 Pass-through | ✅ Achieved | Harness B1–B3: bitwise in 3 layouts × {defaults, mix 0 %} × {no playhead, playing} × 6 block patterns; latency 0 |
| 4 State | ✅ Achieved | Harness S1–S4 PASS (seed > 2^63 as int64 string, edo 64 + mask 0x8000000000000001, uiLanguage code/int, two reopens → one child each) |
| 5 Module + GS | ✅ Achieved | pitch-collections tests rebuilt in scratch: 229/229. GS `pitch-null-v351` then `pitch-null`: 415/415 (baseline `0e1117b6` resolves post-rewrite) |
| 6 COMPAT-01 | ✅ Achieved | See Automated Checks |

## Requirements Verification

**Stage:** stage-1
**Requirements for this stage:** 1 total (1 must)

| Requirement | Priority | Status | Acceptance Criteria |
|-------------|----------|--------|---------------------|
| COMPAT-01: pluginval VST3 + AU, strictness 10 | must | ✅ Complete | pluginval 10 VST3 SUCCESS; pluginval 10 AU (in-process) SUCCESS; auval PASS |

Other requirements are verified at stages 2–4 (unchanged, pending).

**Requirements Summary:**
- ✅ Complete: 1
- ⚠️ Partial: 0
- ⏸️ Deferred (later stage): 29
- ❌ Failed: 0

## Automated Checks

| Check | Result | Notes |
|-------|--------|-------|
| Build (VST3 + AU + Standalone) | ✅ Pass | 0 warnings on touched TUs |
| Install | ✅ Pass | `build-and-install.sh O-Granulator` (re-run: VST3 hash had drifted after the touch rebuild) |
| Render harness | ✅ Pass | 47 PASS, 0 FAIL |
| Param ID order vs spec (independent) | ✅ Pass | 220/220, 140/59/21 |
| pitch-collections tests | ✅ Pass | 229/229 |
| O-GrainScatter pitch-null (v3.5.1 twin) | ✅ Pass | 415/415 |
| pluginval 10 VST3 | ✅ Pass | SUCCESS, rc 0, `--timeout-ms 600000` |
| pluginval 10 AU in-process | ✅ Pass | SUCCESS, rc 0; one warning "Current program is -1" (no factory programs until Stage 4 — expected) |
| auval `aufx OuGn OuDv` | ✅ Pass | AU VALIDATION SUCCEEDED; capabilities `[1, 1] [1, 2] [2, 2]` |
| `setValueNotifyingHost` in Source | ✅ Pass | 0 occurrences |
| `setLatencySamples(0)` in `prepareToPlay` | ✅ Pass | `PluginProcessor.cpp:97` |

## Human Verification

- [ ] Load `O-Granulator-dev` in a DAW (Logic AU + one VST3 host), confirm audio passes unchanged at defaults and the Generic editor lists 220 params
- [ ] Mono-track insert in Logic offers mono→mono / mono→stereo

## Issues Found

- **Deferred (Stage 3):** check-ui-labels on Granulator needs the WebView editor frame (SUMMARY deviation 3).
- **Tooling, unfixed (shared):** `scripts/resolve-target.sh` matches `juce_add_plugin(` inside comments (SUMMARY deviation 2; worked around by rewording the CMake header).
- **Carried to Stage 2:** pitch-collections `JustSevenLimit` is unsorted (12/7 after 7/4) — Granulator must sort its active table; GS fix is a separate `/improve`.
- **Carried to Stage 2:** audio thread needs its own copy of the custom table; never derive unity gain from `get()` on skewed ranges.

## Stage Verdict

**Status:** ✅ VERIFIED

**Ready for next stage:** Yes

**Blockers:** none
