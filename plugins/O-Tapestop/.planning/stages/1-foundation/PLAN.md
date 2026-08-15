# Stage 1: Foundation — Execution Plan

**Plugin:** O-Tapestop
**Stage:** 1-foundation
**Phase:** plan
**Date:** 2026-08-15
**Inputs:** stages/1-foundation/RESEARCH.md, parameter-spec.md (BINDING), research/ARCHITECTURE.md (BINDING, Parameter Mapping lines 164–183), ROADMAP.md Stage 1 row

## Goal

Build system + project structure + full 14-param APVTS + stereo bitwise pass-through shell that passes pluginval strictness 10 (VST3 + AU) and auval. COMPAT-01 is the only requirement mapped to this stage. **No DSP** — varispeed/transport/scratch land in Stage 2. MIX/OUTPUT_GAIN exist as params but stay unwired; the disengaged path must never touch samples (Stage-0 decision #6 — Stage 2's null probes depend on it).

## Tasks

1. [ ] **Create `plugins/O-Tapestop/CMakeLists.txt`** (copy O-ReverseDelay's shape)
   - Files: `plugins/O-Tapestop/CMakeLists.txt`
   - Depends on: none
   - Target `OuariconTapestop`, `PLUGIN_CODE OTsp` (verified unique), `VERSION 0.1.0` (keyword is `VERSION`, never `PLUGIN_VERSION`), `PRODUCT_NAME "O-Tapestop${OUARICON_DEV_SUFFIX}"`, `FORMATS VST3 AU Standalone`, `COMPANY_NAME "${OUARICON_COMPANY_NAME}"`, `PLUGIN_MANUFACTURER_CODE ${OUARICON_MANUFACTURER_CODE}` via `include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)`
   - Wire WebView now: `NEEDS_WEB_BROWSER TRUE`, `NEEDS_WEBVIEW2 TRUE`, compile defs `JUCE_WEB_BROWSER=1`, `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`
   - `target_sources` lists PluginEditor.cpp on the plugin target only (Stage-2 render harness compiles PluginProcessor.cpp with `JUCE_WEB_BROWSER=0`)
   - Scaffold the `option(OUARICON_BUILD_TESTS ...)` hook O-ReverseDelay-style (harness lands Stage 2); defer `juce_add_binary_data` to Stage 3
   - Registration is automatic (root CMakeLists globs `plugins/*`) — no root edit

2. [ ] **Implement PluginProcessor with 14-param APVTS**
   - Files: `plugins/O-Tapestop/Source/PluginProcessor.h`, `Source/PluginProcessor.cpp`
   - Depends on: Task 1
   - `createParameterLayout()` per parameter-spec.md, verbatim UPPER_SNAKE IDs, versioned `juce::ParameterID { "...", 1 }`:

   | ID | Type | Range / Choices | Default |
   |----|------|-----------------|---------|
   | ENGAGE | Bool | Off/On | Off |
   | MODE | Choice | {Stop, Scratch} | Stop |
   | SYNC_MODE | Choice | {Sync, Free} | Sync |
   | STOP_SYNC_DIV | Choice | {1/16, 1/8, 1/4, 1/2, 1 bar, 2 bars, 4 bars} | 1/2 bar |
   | STOP_FREE_MS | Float | 10–8000 ms, skew 0.35 | 500 |
   | STOP_CURVE | Float | 0–100 % | 50 |
   | START_SYNC_DIV | Choice | same 7 divisions | 1/4 bar |
   | START_FREE_MS | Float | 10–8000 ms, skew 0.35 | 250 |
   | START_CURVE | Float | 0–100 % | 50 |
   | ENV_SYNC_DIV | Choice | same 7 divisions | 1 bar |
   | ENV_FREE_MS | Float | 10–8000 ms, skew 0.35 | 1000 |
   | TONE_TRACK | Float | 0–100 % | 60 |
   | MIX | Float | 0–100 % | 100 |
   | OUTPUT_GAIN | Float | −24…+12 dB | 0 |

   - Cache `std::atomic<float>*` raw pointers once in the constructor via `getRawParameterValue` (ARCHITECTURE line 348)
   - `isBusesLayoutSupported`: stereo/mono gate, no side-chain/multi-out/MIDI
   - `processBlock`: `juce::ScopedNoDenormals` + hard pass-through only — no smoothing, no gain, no mix stage
   - Zero latency — no `setLatencySamples` call at all
   - State: standard APVTS `copyState`/`replaceState` XML round-trip (the `scratchEnvelopeJson` property rides the same tree in Stage 2.3; nothing extra now)
   - Traps: division list is the triplet-free 7-entry set (NOT O-Bitrot's); no C++ identifier named bare `begin`/`end` (juce:: free-function shadowing); all 5 Choice params already have ≥2 choices

3. [ ] **Placeholder editor behind the WebView guard**
   - Files: `plugins/O-Tapestop/Source/PluginEditor.h`, `Source/PluginEditor.cpp`
   - Depends on: Task 2
   - `juce::GenericAudioProcessorEditor` (O-Bitrot decision); `createEditor()` guarded with `#if JUCE_WEB_BROWSER` (returns nullptr / no editor otherwise) so the Stage-3 swap is mechanical and the Stage-2 harness link stays clean

4. [ ] **Build both formats**
   - Files: none (build/)
   - Depends on: Tasks 1–3
   - `cmake --build` configure if needed, then `ninja OuariconTapestop_VST3 OuariconTapestop_AU` from `build/`
   - Fix warnings; clean build required

5. [ ] **Bit-transparency probe**
   - Files: scratch-only test (not committed) — render a noise buffer through `processBlock`, `memcmp` against input
   - Depends on: Task 4
   - Cheap insurance now; seeds the Stage-2 harness scaffold. Exercise a couple of block sizes (512, 4096)

6. [ ] **Install + validate (COMPAT-01 gate)**
   - Files: none (system plugin folders)
   - Depends on: Task 5
   - `./scripts/build-and-install.sh O-Tapestop` (Phase 4 does AU cache clear + dev/release dual-variant sweep)
   - `auval -a | grep -i tapestop` lists the AU; run the full auval pass
   - pluginval **strictness 10**, VST3 + AU (O-Bitrot precedent adopted: level 10 from Stage 1 onward)

7. [ ] **Registry + state updates, commit**
   - Files: `PLUGINS.md` (O-Tapestop row ONLY, line 70 — union-merge discipline), `.planning/STATUS.md`
   - Depends on: Task 6
   - Update row to 🚧 Stage 1; STATUS.md → execute complete; single per-plugin commit on `main` (O-Bitrot is mid-Stage-1 on the same branch — keep commits disjoint)

## Known Traps (execute phase MUST read)

- **The 0-ideation→1-foundation gate ALWAYS blocks** on its unconditional build check: run with `--force --skip-review` + justification naming the missing CMakeLists; confirm the bypass logged in `.planning/gate-bypasses.log` (exit 2)
- Dev branding produces `O-Tapestop-dev.{vst3,component}` — never leave the unsuffixed variant installed (build script sweeps both; watch for its ⚠ warning)
- `PLUGIN_VERSION` keyword is silently ignored — the plan uses `VERSION`
- PluginEditor.cpp on the plugin target only; `#if JUCE_WEB_BROWSER` around `createEditor`

## Success Criteria

- [ ] `ninja OuariconTapestop_VST3 OuariconTapestop_AU` builds clean
- [ ] All 14 params present with exact IDs/types/ranges/defaults from parameter-spec.md, skew 0.35 on STOP_FREE_MS/START_FREE_MS/ENV_FREE_MS
- [ ] Pass-through is bit-transparent (memcmp probe, 512 and 4096 blocks)
- [ ] State save/restore round-trips through APVTS XML
- [ ] auval passes; pluginval strictness 10 passes on VST3 AND AU
- [ ] Installed via build-and-install.sh with cache clear + dual-variant sweep
- [ ] PLUGINS.md O-Tapestop row updated (own row only); STATUS.md advanced
