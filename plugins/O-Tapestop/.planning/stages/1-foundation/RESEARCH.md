# Stage 1: Foundation — Research

**Plugin:** O-Tapestop
**Stage:** 1-foundation
**Phase:** research (manual mode)
**Date:** 2026-08-15
**Note:** No discuss-phase CONTEXT.md exists for this stage — findings derive from the binding Stage-0 contracts (parameter-spec.md, research/ARCHITECTURE.md, ROADMAP.md) plus the O-Bitrot 1-foundation discuss precedent (same day, same workflow).

## Stage Goal

Build system + project structure + full 14-param APVTS + stereo bitwise pass-through shell that passes pluginval (COMPAT-01, the only requirement mapped to stage-1). No DSP — all varispeed/transport/scratch components land in Stage 2.

---

## 1. Build System (CMakeLists.txt)

**Reference:** `plugins/O-ReverseDelay/CMakeLists.txt` — the substrate plugin and closest structural match (effect, WebView-bound, render-harness hook). Copy its shape, not O-Polystutter's.

- **Target name:** `OuariconTapestop` (recent-plugin convention: `Ouaricon<Name>` — O-ReverseDelay, O-Octagon, O-TextureForge, O-Orbit). Ninja targets become `OuariconTapestop_VST3` / `OuariconTapestop_AU`. Folder-name ≠ target-name is fine: `build-and-install.sh` resolves the `juce_add_plugin` target from CMakeLists (repo pattern — 11/37 already differ).
- **PLUGIN_CODE:** `OTsp` — verified unique against all 40 existing codes (no `OTsp`/`OTst` collisions).
- **Version keyword is `VERSION`, never `PLUGIN_VERSION`** (silently ignored; ships as 1.0.0) — `critical_plugin_version_keyword_ignored_by_juce`. Start at `VERSION 0.1.0`.
- `COMPANY_NAME "${OUARICON_COMPANY_NAME}"`, `PLUGIN_MANUFACTURER_CODE ${OUARICON_MANUFACTURER_CODE}`, `PRODUCT_NAME "O-Tapestop${OUARICON_DEV_SUFFIX}"`, `FORMATS VST3 AU Standalone` — via `include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)`.
- **Wire WebView flags now, use later:** `NEEDS_WEB_BROWSER TRUE`, `NEEDS_WEBVIEW2 TRUE`, and compile definitions `JUCE_WEB_BROWSER=1`, `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` (Windows WebView renders silently blank without static linking — set before the first Windows CI build, not retrofitted).
- **PluginEditor.cpp goes on the plugin target only** — the Stage-2 render harness compiles PluginProcessor.cpp with `JUCE_WEB_BROWSER=0` and must never see WebView types (`pattern_render_harness_breaks_on_webview_editor`). Structure `target_sources` accordingly from day one and guard `createEditor` with `#if JUCE_WEB_BROWSER` (a Stage-0 carried constraint).
- Registration is automatic: root CMakeLists globs `plugins/*` and `add_subdirectory`s anything with a CMakeLists.txt — no root edit needed.
- Defer: `juce_add_binary_data` UI-resources target (Stage 3), `OUARICON_BUILD_TESTS` harness hook (Stage 2 — but harmless to scaffold the `option()` now, O-ReverseDelay style).

## 2. APVTS — 14 Parameters (parameter-spec.md is BINDING)

parameter-spec.md was promoted to final contract at commit e4ed46a7 (checksum unchanged — the "Draft" heading is cosmetic). IDs/types/ranges/defaults are fixed; the pending UI mockup owns layout only.

**Layout per the spec + ARCHITECTURE Parameter Mapping table:**

| ID | JUCE type | Notes |
|----|-----------|-------|
| ENGAGE | AudioParameterBool | default Off; edge-detected per block in Stage 2 — plain param here |
| MODE | AudioParameterChoice | {Stop, Scratch}, default Stop |
| SYNC_MODE | AudioParameterChoice | {Sync, Free}, default Sync |
| STOP_SYNC_DIV | AudioParameterChoice | 7 divisions {1/16, 1/8, 1/4, 1/2, 1 bar, 2 bars, 4 bars}, default 1/2 bar |
| STOP_FREE_MS | AudioParameterFloat | 10–8000 ms, **skew 0.35**, default 500 |
| STOP_CURVE | AudioParameterFloat | 0–100 %, default 50 |
| START_SYNC_DIV | AudioParameterChoice | same 7 divisions, default 1/4 bar |
| START_FREE_MS | AudioParameterFloat | 10–8000 ms, skew 0.35, default 250 |
| START_CURVE | AudioParameterFloat | 0–100 %, default 50 |
| ENV_SYNC_DIV | AudioParameterChoice | same 7 divisions, default 1 bar |
| ENV_FREE_MS | AudioParameterFloat | 10–8000 ms, skew 0.35, default 1000 |
| TONE_TRACK | AudioParameterFloat | 0–100 %, default 60 |
| MIX | AudioParameterFloat | 0–100 %, default 100 |
| OUTPUT_GAIN | AudioParameterFloat | −24…+12 dB, default 0 |

**Conventions & traps:**
- Versioned `juce::ParameterID { "ENGAGE", 1 }` for every param (suite convention; O-Polystutter/O-Bitrot precedent). Use the spec's UPPER_SNAKE IDs verbatim — the spec is the ID authority, not O-Polystutter's snake_case style.
- Cached `std::atomic<float>*` raw pointers resolved once in the constructor (`getRawParameterValue`) — ARCHITECTURE line 348 makes this explicit.
- All five Choice params have ≥2 choices — satisfies `critical_choice_param_needs_two_choices` (Release div-by-zero → NaN with a single choice).
- No param-ID identifier may shadow a `juce::` free function (`critical_paramid_shadows_juce_free_function`) — these IDs are safe; keep C++ variable names safe too (no bare `begin`/`end`).
- The division list is triplet-free (7 entries) — deliberately simpler than O-Bitrot's CLOCK_SYNC_DIV; do not copy its list.
- All labels are ASCII-safe already; no O-Bitrot-style glyph substitution needed.

## 3. Pass-Through Shell (processBlock)

- Stereo in/out only (ARCHITECTURE §File I/O: no side-chain, no multi-out, no MIDI). Standard `isBusesLayoutSupported` stereo/mono gate.
- `juce::ScopedNoDenormals` + plain pass-through. **Bitwise transparency is contractual, not cosmetic:** Stage-0 decision #6 makes "Bypassed = hard pass-through" structural to FUNC-02/DSP-03/QUAL-01 — Stage 2's null probes assume the disengaged path never touches samples. Do not add smoothing, gain staging, or a mix stage to the shell "for later"; MIX/OUTPUT_GAIN exist as params but stay unwired until Stage 2.
- Zero latency, no `setLatencySamples` call at all (Stage-0 constraint).
- State persistence: standard APVTS `copyState`/`replaceState` XML round-trip. The `scratchEnvelopeJson` string property is Stage 2.3's concern — but since it rides the same ValueTree, nothing extra is needed now; note only that `setStateInformation` must not choke on its absence/presence later.
- Placeholder editor: `juce::GenericAudioProcessorEditor` (O-Bitrot decision), created inside the `#if JUCE_WEB_BROWSER` guard structure so the Stage-3 swap is mechanical.

## 4. Verification Gate (COMPAT-01)

- Build: `ninja OuariconTapestop_VST3 OuariconTapestop_AU`
- Install: `./scripts/build-and-install.sh O-Tapestop` (Phase 4 handles AU cache clear + dev/release dual-variant sweep — `critical_dev_release_variant_shadowing`)
- `auval -a | grep -i tapestop` must list the AU
- **pluginval strictness 10, VST3 + AU** — O-Bitrot's discuss fixed strictness 10 as the gate from Stage 1 onward (repo pattern: level 10 catches latent NaN that auval misses). Recommend adopting identically; confirm at plan phase.
- Bit-transparency probe: even without the Stage-2 harness, a trivial offline check (render a buffer through processBlock, memcmp against input) is cheap insurance and seeds the harness scaffold.

## 5. Known Traps for the Execute Phase

- **The 0-ideation→1-foundation gate ALWAYS blocks** on its unconditional build check (`pattern_gate_0_to_1_always_needs_force`): run with `--force --skip-review` + justification naming the missing CMakeLists; verify the bypass is logged in `.planning/gate-bypasses.log` (exit 2).
- Dev branding produces `O-Tapestop-dev.{vst3,component}` — never leave the alternate (unsuffixed) variant installed; the build script sweeps both.
- Branching: per Stage-0/O-Bitrot practice this workflow runs on `main` (no worktree); O-Bitrot is mid-Stage-1 on the same branch — commits must stay per-plugin and PLUGINS.md edits limited to O-Tapestop's own row (union-merge duplicate check after any merge).

## 6. Module Opportunities

None for Stage 1. The O-ReverseDelay substrate (`CaptureBuffer.h`, `WindowLut.h`) and preset-manager module enter at Stages 2 and 4 respectively — foundation stays dependency-free.

---

## Inputs for /plugin-plan

- `plugins/O-Tapestop/.planning/parameter-spec.md` (BINDING)
- `plugins/O-Tapestop/.planning/research/ARCHITECTURE.md` (BINDING; Parameter Mapping lines 164–183)
- `plugins/O-Tapestop/.planning/ROADMAP.md` (Stage 1 row)
- Reference CMake/processor: `plugins/O-ReverseDelay/CMakeLists.txt`, O-Polystutter `createParameterLayout`
- Precedent: `plugins/O-Bitrot/.planning/stages/1-foundation/CONTEXT.md`

## Success Criteria (feed into PLAN.md)

- [ ] `ninja OuariconTapestop_VST3 OuariconTapestop_AU` builds clean
- [ ] All 14 params present with exact IDs/types/ranges/defaults/skew (0.35 on the three FREE_MS params) from parameter-spec.md
- [ ] Pass-through is bit-transparent (memcmp probe)
- [ ] State save/restore round-trips
- [ ] `auval` passes; pluginval strictness 10 passes (VST3 + AU)
- [ ] Installed via build-and-install.sh with cache clear + dual-variant sweep
