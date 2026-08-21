# O-Emulator — Stage 1 (Foundation) Plan

**Date:** 2026-08-20
**Phase:** plan (stage 1)
**Inputs:** stages/1-foundation/RESEARCH.md, ROADMAP.md, parameter-spec-draft.md, REQUIREMENTS.md (COMPAT-01)

## Goal

Stand up the O-Emulator build target and plugin shell: CMake target `OEmulator`, APVTS with the 5 contract parameters, stereo-only pass-through effect, generic editor — verified by pluginval strictness 10 on VST3 **and** AU (COMPAT-01, the only requirement gated at stage-1). No DSP, no WebView UI, no modules.

## Decision Record

**Draft-freeze chosen over mockup-first** (RESEARCH §2 caveat / open question 1). The user invoked `/plugin-plan` directly, so `parameter-spec-draft.md` (checksummed in STATUS.md) is **frozen as the Stage-1 APVTS contract**: 5 params, IDs `console/crush/age/reverb/mix` exactly as tabled in RESEARCH §2. Rationale: draft matches BRIEF verbatim, ARCHITECTURE design-sync found no conflicts, and no presets/sessions exist yet so ID churn cost is zero today. The UI mockup (`/dream O-Emulator`) remains recommended **before Stage 3**; any mockup-driven additions are UI-side and reconcile in Stage 3 — DSP param IDs above do not change without a new contract checksum.

**Pre-create skeleton dirs** (open question 2): yes to `Source/dsp/.gitkeep` and the `OUARICON_BUILD_TESTS` CMake option; **no** placeholder code and **no** `add_subdirectory(tests/render-harness)` until the harness exists (an unconditional `add_subdirectory` into an empty dir would break configure — guard with `if(OUARICON_BUILD_TESTS AND EXISTS ...)`).

## Prerequisite (before any execute work)

Current checkout is on `improve/o-spectralshaper-v1.6.2`. Stage 1 work happens in a dedicated sibling worktree:

```bash
git -C ~/Dev/VST-development worktree add ../VST-development-emulator -b feat/o-emulator main
```

- One branch per plugin, cut from `main` (Stage 0 is already merged to main).
- Known blocker: fresh worktree can fail CMake configure on O-Orbit's untracked SAF submodule → configure with `-DSKIP_PLUGINS=O-Orbit` (root CMake supports it) or copy the dep from the main checkout.

## Tasks

1. [ ] **Create worktree + branch** `feat/o-emulator` at `../VST-development-emulator` (command above); verify `git status` is clean on the new branch.
   - Files: none (git only)
   - Depends on: none

2. [ ] **Plugin CMakeLists.txt** — `juce_add_plugin(OEmulator)` per RESEARCH §1: `PLUGIN_CODE OEmu`, `FORMATS VST3 AU Standalone`, `PRODUCT_NAME "O-Emulator${OUARICON_DEV_SUFFIX}"`, `VERSION 1.0.0` (**never** `PLUGIN_VERSION`), `NEEDS_WEB_BROWSER TRUE` / `NEEDS_WEBVIEW2 TRUE`; compile defs `JUCE_WEB_BROWSER=1`, `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`, `JUCE_USE_CURL=0`, `JUCE_VST3_CAN_REPLACE_VST2=0`; include `modules/cmake/OuariconModules.cmake`; full `juce::` module link list; `juce_generate_juce_header(OEmulator)` **after** `target_link_libraries`; `option(OUARICON_BUILD_TESTS ... OFF)` with existence-guarded subdirectory hook. **No** `juce_add_binary_data` target (Stage 3).
   - Files: `plugins/O-Emulator/CMakeLists.txt`
   - Depends on: Task 1

3. [ ] **APVTS + processor shell** — `Source/PluginProcessor.h/.cpp`:
   - Static `createParameterLayout()`; `juce::ParameterID{"id", 1}`; params exactly: `console` (Choice: SNES, PS1, NES, Game Boy, Genesis; default SNES), `crush` (0–100 %, step 0.1, linear, default 50), `age` (default 20), `reverb` (default 0), `mix` (default 100).
   - `BusesProperties()` stereo in/out; `isBusesLayoutSupported` accepts only main-in == main-out == stereo.
   - `processBlock`: `juce::ScopedNoDenormals`, clear extra output channels, pass-through. Zero allocations.
   - **No latency reporting** (0 samples) — real figure lands in Phase 2.1.
   - State: `copyState()`/`replaceState()` XML round-trip **with a `pluginVersion` attribute stamped now** (future migration gates key off it).
   - Files: `plugins/O-Emulator/Source/PluginProcessor.h`, `Source/PluginProcessor.cpp`
   - Depends on: Task 2

4. [ ] **Generic editor** — `createEditor()` returns `juce::GenericAudioProcessorEditor`; keep the editor include local to `createEditor()` so the Stage-2 render harness compiles with `JUCE_WEB_BROWSER=0` and no editor sources. No separate PluginEditor files in Stage 1.
   - Files: `Source/PluginProcessor.cpp`
   - Depends on: Task 3

5. [ ] **House hygiene** — AGPL header block on every source file (`scripts/add-agpl-headers.py`); create `Source/dsp/.gitkeep`; ASCII-only strings throughout (choice labels verified ASCII).
   - Files: all sources, `Source/dsp/.gitkeep`
   - Depends on: Task 3

6. [ ] **Configure + build** (from the worktree): `cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release [-DSKIP_PLUGINS=O-Orbit]`, then `ninja -C build OEmulator_VST3 OEmulator_AU`. Root CMake auto-discovers the plugin — no root edit.
   - Files: none (build)
   - Depends on: Tasks 2–5

7. [ ] **Install** via `./scripts/build-and-install.sh O-Emulator` — handles AU cache clear + dual-variant sweep. Never hand-copy bundles. Expect `O-Emulator-dev.{vst3,component}`.
   - Depends on: Task 6

8. [ ] **Verify (COMPAT-01 gate)** —
   ```bash
   auval -a | grep -i emulator
   pluginval --strictness-level 10 --validate ~/Library/Audio/Plug-Ins/VST3/O-Emulator-dev.vst3
   pluginval --strictness-level 10 --validate ~/Library/Audio/Plug-Ins/Components/O-Emulator-dev.component
   ```
   Strictness **10** now, not a softer smoke level.
   - Depends on: Task 7

9. [ ] **Commit + status** — single commit on `feat/o-emulator` (`feat(O-Emulator): Stage 1 foundation — CMake, APVTS, stereo shell`); update `.planning/STATUS.md` (stage 1 execute complete, COMPAT-01 → verified pending /plugin-verify).
   - Files: `.planning/STATUS.md`
   - Depends on: Task 8

## Files to Create

- `plugins/O-Emulator/CMakeLists.txt`
- `plugins/O-Emulator/Source/PluginProcessor.h`
- `plugins/O-Emulator/Source/PluginProcessor.cpp`
- `plugins/O-Emulator/Source/dsp/.gitkeep`

## Files to Modify

- `plugins/O-Emulator/.planning/STATUS.md`

## Pitfalls in Scope (from RESEARCH §7)

`VERSION` not `PLUGIN_VERSION` · ≥2 choices (have 5) · no `juce::`-shadowing IDs (checked) · install only via build-and-install.sh (variant shadowing) · target `OEmulator` vs folder `O-Emulator` is fine · O-Orbit configure blocker → SKIP_PLUGINS · ASCII-only `juce::String` literals · dedicated worktree sidesteps shared-HEAD/index races.

## Success Criteria

- [ ] `feat/o-emulator` worktree builds VST3 + AU clean (Release, Ninja)
- [ ] All 5 params appear in the generic editor with correct ranges/defaults; console shows 5 choices
- [ ] Audio passes through unchanged (stereo), no crashes, latency reports 0
- [ ] State save/load round-trips (pluginval covers this) and state XML carries `pluginVersion`
- [ ] `auval -a` lists O-Emulator-dev
- [ ] **COMPAT-01:** pluginval strictness 10 passes for VST3 **and** AU
- [ ] No DSP, no WebView, no binary-data target, no module deps introduced
