# Stage 1 (Foundation) — CONTEXT

**Plugin:** O-simpleSubtractive
**Date:** 2026-06-25
**Phase:** discuss
**Source:** Compiled from locked Stage-0 contracts (BRIEF.md, ARCHITECTURE.md, ROADMAP.md, parameter-spec-draft.md). No open DSP/UI decisions at this gate — the foundation scope is fully determined by contracts.

---

## Stage Goal

A **silent, loadable, valid plugin shell**: CMake build (VST3 + AU on macOS, VST3 on Windows) configured as a MIDI synth with WebView2, the complete 20-parameter APVTS, and state persistence. No DSP yet — `processBlock` clears the buffer (silence). Proves the project compiles, loads, validates (pluginval), and round-trips parameter state before any audio code is written.

## Scope (in)

1. **CMakeLists.txt** — mirror O-simpleFM (primary template):
   - `juce_add_plugin` with `IS_SYNTH TRUE`, `NEEDS_MIDI_INPUT TRUE`, `FORMATS VST3 AU Standalone`.
   - Ouaricon branding vars: `COMPANY_NAME ${OUARICON_COMPANY_NAME}`, `PLUGIN_MANUFACTURER_CODE ${OUARICON_MANUFACTURER_CODE}`, `PRODUCT_NAME "O-simpleSubtractive${OUARICON_DEV_SUFFIX}"`, `PLUGIN_CODE` = new unique 4-char (proposed `OSiS`).
   - WebView2 (Windows): `NEEDS_WEBVIEW2 TRUE` **and** `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` (memory: both flags or blank WebView).
   - One `juce_add_binary_data` target in v1.0 (WebView UI). If a 2nd is ever added, give it a distinct `NAMESPACE` (O-simpleGrain lesson).
   - Version `1.0.0`.
2. **PluginProcessor (.h/.cpp)** — APVTS with all **20 parameters** exactly per ARCHITECTURE.md §Parameter Mapping (IDs, types, ranges, defaults, skews). `prepareToPlay`/`releaseResources` stubs; `processBlock` = `buffer.clear()` + `ScopedNoDenormals`; `setLatencySamples(0)`. State persistence via `apvts.state` → `getStateInformation`/`setStateInformation` (XML round-trip).
3. **PluginEditor (.h/.cpp)** — minimal `WebBrowserComponent` shell (or generic editor placeholder) sufficient to load; full UI is Stage 3. WebView user-data-folder set to temp dir on Windows (memory).
4. **Project structure** — `Source/`, `Source/ui/` placeholder, matching sibling layout.

## Scope (out — deferred to later stages)

- All DSP (oscillators, filter, ADSRs, voices) → **Stage 2**.
- WebView UI / parameter binding / visualization → **Stage 3**.
- Presets, optimization, edge cases → **Stage 4**.

## The 20 parameters (authoritative — ARCHITECTURE.md §Parameter Mapping)

`oscWave`(choice Saw/Square/Tri/Sine=Saw), `subLevel`(0–100%=0), `noiseLevel`(0–100%=0),
`filterType`(choice LP/HP/BP/Notch=LP), `filterSlope`(choice 6/12/24=24), `cutoff`(20–20000 Hz, log skew≈0.25 =2000), `resonance`(0–100%=10), `filterEnvAmount`(−100..+100% bipolar =+50), `keyTrack`(0–100%=0),
`filterAttack/Decay/Sustain/Release`(0–5s/0–100% =0.005/0.3/40%/0.2),
`ampAttack/Decay/Sustain/Release`(0–5s/0–100% =0.005/0.3/80%/0.1),
`voiceMode`(choice Poly/Mono/Legato=Poly), `glide`(0–1s=0), `outputLevel`(−inf..0 dB=0).

Time params skew 0.35; `glide` skew 0.5; `outputLevel` dB→lin with 20 ms smoothing applied in DSP (Stage 2), but declared here.

## Requirements covered this stage

- **COMPAT-01** — loads in DAW, passes pluginval shell validation.
- 20-param APVTS present and persisted (foundation contract for Stage 2/3 binding).

## Constraints honored

- JUCE 8.0.9; CMake + Ninja; local JUCE at `/Users/taylorbrook/JUCE`.
- macOS: build/install both VST3 + AU with cache-clear + dual-variant sweep (CLAUDE.md). Windows: VST3 only.
- RT-safe `processBlock` (no alloc/lock/file-IO) even as a silent stub.
- `getLatencySamples()` non-virtual → use `setLatencySamples(0)` in `prepareToPlay`.

## Open decisions

**None at the contract level.** One process choice for the orchestrator: workflow pace through Stage 1 (manual phase-by-phase vs express). Surfaced to user at discuss handoff.

## Sibling references (read at execute)

O-simpleFM (primary — CMake, processor/editor skeleton, APVTS pattern), O-simpleAdditive (WebView CMake template), O-simpleGrain (BinaryData namespace discipline).

## Success criteria (verify phase will check)

- [ ] `ninja O-simpleSubtractive_VST3 O-simpleSubtractive_AU` builds clean.
- [ ] Loads in a DAW as an **instrument**; `auval` passes; pluginval passes shell checks.
- [ ] All 20 parameters appear in the host's generic parameter list with correct ranges/defaults.
- [ ] Parameter state round-trips (save/reload preset restores values).
- [ ] `processBlock` outputs silence (no crash, no denormals, no audio).
