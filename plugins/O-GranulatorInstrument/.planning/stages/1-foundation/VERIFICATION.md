# Stage 1: Foundation — Verification

## Verification Date

2026-11-07 (verify session, independent re-run against `74606bb3` + the installed shipping build)

## Goal-Backward Analysis

### Original Goals (from CONTEXT.md)

1. The plugin builds as VST3 + AU + Standalone.
2. All 261 parameters are registered in contract order (176 Float / 70 Choice / 15 Bool), with the dest table (126) and the source table (18) frozen.
3. The output is silent. The sidechain never reaches the output.
4. The buses and MIDI are correct: stereo `Sidechain` input typed `kAux` on VST3, stereo output, MIDI consumed (D9).
5. The state scaffold works: `<Engine>` (int64 seed, stateVersion, modDestTableVersion), an empty `<Slots>`, and root `uiLanguage` (P1).
6. pluginval 10 passes on VST3 + AU with the sidechain active and inactive, and auval passes (COMPAT-01).
7. The NE configure check passes. The WebView plumbing is wired, and no UI files land (D8).

### Deliverables (from SUMMARY.md)

1. `CMakeLists.txt` (from `v1-CMakeLists.txt`): `aumu OuGI OuDv`, 0.1.0, VST3/AU/Standalone + render-harness + param-dump targets.
2. `Source/GiParamIds.h`: loop-generated IDs, literal `kModDestIds[126]` / `kModSources[18]`, static_asserts. Goldens come from `tests/golden/extract_spec.py`.
3. `processBlock` clears every channel and reads no sidechain sample or parameter.
4. `makeBuses()` plus `isBusesLayoutSupported` (bitset `==`), and `GiVst3Extensions::getPluginHasMainInput() = false`.
5. A state codec ported from the parent, with the seed saved as an int64 string.
6. The render harness has 55 rows. Execute ran pluginval VST3 ×3 + AU on the active sidechain and VST3 ×2 + AU on the inactive variant, plus auval.
7. The configure log shows `JUCE-NE-PATCH markers verified`. A placeholder `PluginEditor.cpp` exists, with no `PluginEditor.h` and no `Source/ui/`.

### Goal Achievement

| Goal | Status | Evidence |
|------|--------|----------|
| 1. Builds | ✅ Achieved | Execute #3 rc 0 with 0 plugin warnings; Standalone ALIVE. Verify: harness + param-dump rebuild rc 0. The installed bundles are newer than every `Source/` and `CMakeLists.txt` file. |
| 2. 261 params, contract order | ✅ Achieved | Verify: the extractor re-run prints `OK: 261 params (176/70/15), 126 dests, 18 sources`, and the regenerated TSVs are byte-identical to the committed goldens. The param-dump re-run is byte-identical to `.planning/params.tsv` (261 rows). Harness G1–G6, D1–D5 and SRC1 PASS. |
| 3. Silent output, SC isolated | ✅ Achieved | Harness R1/R1v/R1n PASS (stereo + mono SC, MIDI present → bitwise zero). Code: `PluginProcessor.cpp:123-138` clears every channel. |
| 4. Buses / MIDI / kAux | ✅ Achieved | Harness B1, B3, M1 and H2 PASS. Code: `PluginProcessor.cpp:112-121` uses bitset `==`, and `PluginProcessor.h:45` sets `getPluginHasMainInput() = false`. auval prints `[1, 2] [2, 2]` (matches the frozen block). |
| 5. State scaffold | ✅ Achieved | Harness S1a (seed `0x80000001`), S1b (> 2^63 reads back as an int64), and S2–S5 PASS. |
| 6. COMPAT-01 | ✅ Achieved | See Automated Checks. Execute ran the active and inactive arms. Verify re-ran pluginval 10 VST3 + AU and auval on the installed shipping build. |
| 7. NE check, D8 plumbing | ✅ Achieved | The execute configure log shows the `JUCE-NE-PATCH` line. There is no `Source/ui/` and no `PluginEditor.h`. `createEditor` returns `GenericAudioProcessorEditor`. |

## Requirements Verification

**Stage:** stage-1
**Requirements for this stage:** 1 total (1 must)

| Requirement | Priority | Status | Acceptance Criteria |
|-------------|----------|--------|---------------------|
| COMPAT-01: Passes pluginval validation (VST3 and AU) | must | ✅ Complete | "pluginval strictness 10 passes for VST3 and AU". Met with the SC active and inactive, and auval PASS. COMPAT-01 is re-checked by the Stage 4 bar. |

**Requirements Summary:**
- ✅ Complete: 1
- ⚠️ Partial: 0
- ⏸️ Deferred (later stage): 28 (stage-2 / 3 / 4)
- ❌ Failed: 0

## Automated Checks

| Check | Result | Notes |
|-------|--------|-------|
| Spec hashes (#0) | ✅ Pass | Whole-file `b6bc70e3…ca751b` and body `c5553e0c…543191` match STATUS. The spec is untouched. |
| Goldens re-extract | ✅ Pass | 3 TSVs regenerated into the scratchpad are byte-identical to `tests/golden/`. |
| Render harness | ✅ Pass | `55 PASS, 0 FAIL`, rc 0. All 26 row IDs are present. |
| param-dump | ✅ Pass | Byte-identical to `.planning/params.tsv`. |
| auval `aumu OuGI OuDv` | ✅ Pass | `AU VALIDATION SUCCEEDED`, `[1, 2] [2, 2]`. |
| pluginval 10 VST3 (installed `-dev`) | ✅ Pass | `SUCCESS`, rc 0; `Main bus num input channels: 2` / output 2. |
| pluginval 10 AU in-process (installed `-dev`) | ✅ Pass | `SUCCESS`, rc 0; `Main bus num input channels: 2` / output 2. |
| SC-inactive arm | ✅ Pass (execute) | Execute ran VST3 ×2 + AU + auval with `Main bus num input channels: 0`. Not re-run in verify, because re-running it would install a do-not-ship variant. |
| Shipping cache | ✅ Pass | `OGI_GATE_SC_DEFAULT_OFF:BOOL=OFF`. Exactly the two `-dev` bundles are installed. |
| Listener / notify / lambda-range greps | ✅ Pass | 0 hits in `Source/` (excluding the configure-copied `ui/public/modules`). |
| Git | ✅ Pass | No uncommitted changes under `plugins/O-GranulatorInstrument`. No `O-GranulatorInstrument*` tag. |

## Human Verification

None required for Stage 1. The output is silent and the editor is the generic test surface. Deferred items:
- The `kAux` input typing check in Live / Reaper → Stage 4 DAW pass (P3).
- Logic sidechain Record-in → Stage 2 / 4 (D6).

## Issues Found

- None blocking.
- Note: the verify harness build relinked the harness (8/9 steps), most likely because shared module files changed since execute. It still gives 55/0, so this is not a regression.
- Carried from execute: 5 foreign O-Granulator `used_by` lines in `modules/registry.yaml` remain uncommitted in the working tree for the O-Granulator owner. Not this plugin's change.

## Stage Verdict

**Status:** ✅ VERIFIED

**Ready for next stage:** Yes. Stage 2 (DSP), Phase 2.1: fork the grain-engine leaves. Confirm the pin SHA at 2.1 discuss (O-Granulator 3.3 closed at `1455e501` / `00e462b8`; record its Stage 4 D43 leaf drift in FORK-MANIFEST).

**Blockers:** none.
