# O-Emulator — Stage 1 (Foundation) Research

**Date:** 2026-08-20
**Phase:** research (stage 1)
**Inputs:** ROADMAP.md, ARCHITECTURE.md (checksummed contract), parameter-spec-draft.md, REQUIREMENTS.md (COMPAT-01)

## Stage 1 Scope

CMake target, APVTS (5 params incl. 5-entry choice), stereo effect shell (pass-through processBlock), pluginval smoke VST3 + AU (COMPAT-01). No DSP, no WebView UI.

---

## 1. CMake — house pattern (modeled on O-Bitrot, the closest reference)

Root `CMakeLists.txt` auto-discovers `plugins/*/CMakeLists.txt` — **no root edit needed**. Branding vars (`OUARICON_COMPANY_NAME`, `OUARICON_MANUFACTURER_CODE`, `OUARICON_DEV_SUFFIX`) come from root; local builds always get `-dev` suffix (forced OFF outside CI).

```cmake
cmake_minimum_required(VERSION 3.15)
include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)

juce_add_plugin(OEmulator
    COMPANY_NAME "${OUARICON_COMPANY_NAME}"
    PLUGIN_MANUFACTURER_CODE ${OUARICON_MANUFACTURER_CODE}
    PLUGIN_CODE OEmu                       # verified unused across all 43 existing plugins
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "O-Emulator${OUARICON_DEV_SUFFIX}"
    VERSION 1.0.0                          # VERSION, not PLUGIN_VERSION (see pitfalls)
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_WEB_BROWSER TRUE
    NEEDS_WEBVIEW2 TRUE
)
```

- **Target name `OEmulator`** (no hyphen) inside folder `plugins/O-Emulator/` — `build-and-install.sh` resolves the `juce_add_plugin` target from the CMakeLists, not the folder name, so this split is safe and standard here.
- **`PLUGIN_CODE OEmu`** — checked against a grep of all 43 plugin CMakeLists; unused. Keep the leading `O` + mixed case convention.
- Set `NEEDS_WEB_BROWSER TRUE` / `NEEDS_WEBVIEW2 TRUE` + the compile defs (`JUCE_WEB_BROWSER=1`, `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`, `JUCE_USE_CURL=0`, `JUCE_VST3_CAN_REPLACE_VST2=0`) **now** so Stage 3 doesn't touch plugin identity/format config later. The `juce_add_binary_data` UI-resources target is Stage 3 work — do NOT create it in Stage 1.
- Full `juce::` module link list + `juce_generate_juce_header(OEmulator)` **after** `target_link_libraries` (JUCE 8 requirement).
- Test-harness hook: `option(OUARICON_BUILD_TESTS ... OFF)` + `if(OUARICON_BUILD_TESTS) add_subdirectory(tests/render-harness) endif()` — declare the option in Stage 1 (empty dir guard can wait; harness itself is Stage 2.1).
- AGPL header block on every source file (house standard since 2026-08-01; `scripts/add-agpl-headers.py` exists).

## 2. APVTS — 5 parameters from the draft spec

| ID | Type | Range/Choices | Default |
|----|------|---------------|---------|
| `console` | Choice | SNES, PS1, NES, Game Boy, Genesis | SNES (0) |
| `crush` | Float | 0–100 %, step 0.1, linear | 50 |
| `age` | Float | 0–100 %, step 0.1, linear | 20 |
| `reverb` | Float | 0–100 %, step 0.1, linear | 0 |
| `mix` | Float | 0–100 %, step 0.1, linear | 100 |

- Static `createParameterLayout()` (house pattern), `juce::ParameterID{"id", 1}` version hint 1.
- **ID safety check done:** none of `console/crush/age/reverb/mix` shadow `juce::` free functions (the known trap is `end`/`begin`). All ASCII. No `/` anywhere. 5-entry choice satisfies the ≥2-choices rule (a 1-choice param NaNs in Release).
- Percent ranges stay linear (no skew) → factory presets authored later as fractions are safe from the skew trap.
- State save/load: standard `copyState()`/`replaceState()` XML round-trip, and **stamp a `pluginVersion` attribute into the state now** — every house preset-migration gate (e.g. O-Bitrot `presetVersionIsPre170`) keys off it, and it's free to add at v1.0.0.

### ⚠ Contract caveat — parameter-spec is still DRAFT

`STATUS.md` says `next_action: mockup_then_foundation`: the UI mockup (`/dream O-Emulator`) is supposed to finalize `parameter-spec.md` before Stage 1. The draft is checksummed and matches BRIEF exactly, and ARCHITECTURE's design-sync check found no conflicts — so the risk of the mockup changing *DSP* params is low, but a mockup could still add UI-side niceties. **Decision for the plan phase:** either run the mockup first (per STATUS.md), or explicitly freeze the draft as the Stage-1 APVTS contract and let Stage 3 reconcile. Recommend the first — it's the documented flow and avoids param-ID churn after presets exist.

## 3. Buses & shell processor

- `BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true).withOutput("Output", ..., true)` (juce8-critical-patterns #4). ARCHITECTURE: standard stereo in/out only, no sidechain.
- `isBusesLayoutSupported`: accept only main-in == main-out == stereo. Keep it strict — the whole pipeline (codecs L/R, SPU stereo-cross reverb) is stereo-native; pluginval handles a stereo-only effect fine.
- `processBlock` Stage 1: `juce::ScopedNoDenormals`, clear any extra output channels, pass audio through untouched. Zero allocations already (PERF-01 habit starts here).
- **Latency: report nothing in Stage 1** (0 samples). The real figure (~100–130 samples @ 48 kHz, constant worst-case across modes) is computed in `prepareToPlay` in Phase 2.1 and mirrored into `DryWetMixer::setWetLatency`. `setLatencySamples` is non-virtual in JUCE 8 — call it, never override `getLatencySamples()`.
- **Editor:** `juce::GenericAudioProcessorEditor` for Stages 1–2. Structure `PluginProcessor.cpp` the house way from day one: editor include guarded so the Stage-2 render harness compiles with `JUCE_WEB_BROWSER=0` and no editor sources (`pattern_render_harness_breaks_on_webview_editor`). With a GenericAudioProcessorEditor this is trivial; just keep the include local to `createEditor()`.

## 4. Module reuse

- **Stage 1: none.** `preset-manager` (now v1.0.6) pairs with the WebView UI — add via `ouaricon_add_module` in Stage 3, not now.
- `modules/cmake/OuariconModules.cmake` is still included in Stage 1's CMakeLists so the Stage 3 one-liner needs no restructuring.

## 5. Branch / worktree prerequisite

Current checkout sits on `improve/o-spectralshaper-v1.6.2`. O-Emulator Stage 0 is already merged to `main`. Before Stage 1 execute:

```bash
git worktree add ../VST-development-emulator -b feat/o-emulator main
```

(one branch per plugin, cut from main; sibling worktree named `VST-development-<slug>`). **Known blocker:** a fresh worktree can fail CMake configure on O-Orbit's untracked SAF submodule (`pattern_fresh_worktree_missing_untracked_build_deps`) — mitigate with `-DSKIP_PLUGINS=O-Orbit` (root CMake supports it) or copy the dep from the main checkout.

## 6. Build, install, verify (COMPAT-01 gate)

```bash
# from the worktree
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release [-DSKIP_PLUGINS=O-Orbit]
ninja -C build OEmulator_VST3 OEmulator_AU
./scripts/build-and-install.sh O-Emulator   # does AU cache clear + dual-variant sweep (Phase 4)
auval -a | grep -i emulator
pluginval --strictness-level 10 --validate ~/Library/Audio/Plug-Ins/VST3/O-Emulator-dev.vst3
pluginval --strictness-level 10 --validate ~/Library/Audio/Plug-Ins/Components/O-Emulator-dev.component
```

- Local bundles are `O-Emulator-dev.{vst3,component}`. If an unsuffixed variant ever lands, the dev/release AU-registry shadowing trap fires — the install script's sweep handles it, but never hand-copy around it.
- REQUIREMENTS says COMPAT-01 verifies at stage-1 and its acceptance criterion is strictness **10** — run 10 now, not a softer smoke level, so Stage 2 inherits a clean baseline. (CI Windows pluginval-10 has caught NaNs that auval missed; the habit pays off.)

## 7. Pitfalls checklist (from knowledge base, filtered to Stage 1)

1. **`PLUGIN_VERSION` is not a `juce_add_plugin` keyword** — silently ships 1.0.0 regardless. Use `VERSION`.
2. **AudioParameterChoice needs ≥2 choices** — satisfied (5), noted so nobody ever collapses it.
3. **Param IDs shadowing `juce::` free functions** — checked clean for all 5 IDs.
4. **Dev/release variant bundles shadow each other in the AU registry** — use `build-and-install.sh`, never manual copies.
5. **`build-and-install.sh` resolves the CMake target, not the folder name** — `OEmulator` target in `O-Emulator/` folder is fine.
6. **`juce_add_binary_data` strips hyphens / two binary-data targets collide on the namespace** — Stage 3 concern; keep exactly one UI-resources target when it arrives.
7. **Fresh worktree CMake configure blocked by O-Orbit** — see §5.
8. **`juce::String(const char*)` is ASCII-only** — all names/labels here are ASCII; keep it that way in choice labels ("Game Boy" is fine).
9. Concurrent-session checkout hazards (shared HEAD/index) — using a dedicated worktree sidesteps both.

## 8. Open questions for the plan phase

1. **Mockup-first or draft-freeze?** (§2 caveat — recommend mockup-first per STATUS.md `next_action`.)
2. Whether Stage 1 pre-creates `Source/dsp/` and `tests/render-harness/` skeleton dirs (cheap, keeps Phase 2.1 diff focused) — recommend yes for the directory + CMake option, no placeholder code.
