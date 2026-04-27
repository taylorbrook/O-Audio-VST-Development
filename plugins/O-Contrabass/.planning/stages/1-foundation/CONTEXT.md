# Stage 1: Foundation — Context

**Date:** 2026-04-26
**Stage:** 1 of 4 (Foundation)
**Plugin:** O-Contrabass
**Mode:** Synthesis from locked Stage 0 contracts (no open questions surfaced)

---

## Discussion Summary

All Stage 1 inputs are already locked by Stage 0 artifacts. This CONTEXT.md is a synthesis, not a Q&A — the discuss phase confirmed there are zero open questions for Foundation. Architecture, parameter set, module dependencies, and build flags are all immutable contracts at this point.

**Source contracts (unchanged from Stage 0):**
- `BRIEF.md` — creative vision (sha256:6ea840bb…)
- `parameter-spec.md` — 29 parameters across 8 sections (sha256:c47fe736… — promoted from draft on 2026-04-26)
- `research/ARCHITECTURE.md` — DSP contract, 11 sections (sha256:3cb26814…)
- `ROADMAP.md` — phased implementation plan (sha256:106639f6…)

---

## Requirements Confirmed

### Build System (CMakeLists.txt)

- **Target name:** `O-Contrabass`
- **Plugin formats:** `VST3 AU` (macOS), `VST3` (Windows)
- **Plugin type:** Instrument
  - `IS_SYNTH TRUE` (juce8-critical-patterns #22)
  - `NEEDS_MIDI_INPUT TRUE`
  - `NEEDS_MIDI_OUTPUT FALSE`
- **WebView (cross-platform):**
  - `NEEDS_WEB_BROWSER TRUE`
  - `NEEDS_WEBVIEW2 TRUE`
  - Compile def `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` (memory: required to avoid silent blank UI on Windows)
- **JUCE modules:** `juce_audio_basics`, `juce_audio_processors`, `juce_audio_utils`, `juce_dsp`, `juce_audio_plugin_client`, `juce_gui_basics`, `juce_gui_extra`
- **Shared modules:** `modules/tuning/scala-tuning-engine` (v2.1.0), `modules/tuning/note-expression`
- **C++ standard:** C++17 (project default)

### PluginProcessor (Source/PluginProcessor.{h,cpp})

- **BusesProperties:** output-only `2 channels` (synth pattern — no input bus)
- **APVTS:** all 29 parameters from `parameter-spec.md`, ID strings match spec table exactly
- **prepareToPlay:**
  - Allocates oversampler (`juce::dsp::Oversampling<float>`, factor 2, `filterHalfBandPolyphaseIIR`) — Stage 2 will wire it; Stage 1 only allocates and reports latency
  - Calls `setLatencySamples(n)` once after oversampler allocates
  - Note: `getLatencySamples()` is **not virtual** in JUCE 8 — never override
- **processBlock:** bypass / silence-out stub; no DSP yet (DSP arrives in Stage 2)
- **getStateInformation / setStateInformation:** APVTS state only (preset infrastructure deferred to Stage 4)

### PluginEditor (Source/PluginEditor.{h,cpp})

- **Stage 1 = minimal stub.** Empty `Component` or trivial label only — full WebView GUI lands in Stage 3.
- No WebView wiring yet (deferred to keep Stage 1 build green even if WebView assets are absent).

### Source Layout

```
plugins/O-Contrabass/
├── CMakeLists.txt
└── Source/
    ├── PluginProcessor.h
    ├── PluginProcessor.cpp
    ├── PluginEditor.h
    └── PluginEditor.cpp
```

---

## Constraints Identified

| Constraint | Source | Implication for Stage 1 |
|---|---|---|
| `IS_SYNTH TRUE` + `NEEDS_MIDI_INPUT TRUE` mandatory | juce8-critical-patterns #22 | Both required in `juce_add_plugin()` |
| `BusesProperties` output-only | JUCE synth pattern | Constructor cannot register input bus |
| `getLatencySamples()` non-virtual in JUCE 8 | memory file | Use `setLatencySamples(n)` in `prepareToPlay`; never override the getter |
| WebView2 static linking | memory file | `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` required when `NEEDS_WEBVIEW2 TRUE` |
| pluginval strictness 10 baseline | COMPAT-01 | Bypass-mode plugin must still pass strictness 10 |
| Oversampling latency reported, algorithmic = 0 | PERF-03 | Stage 1 reports oversampler-only latency (1–3 samples) |
| 29 parameter IDs frozen | parameter-spec.md checksum | Any rename later breaks DAW automation; lock now |

---

## Approach Decisions

| Decision | Choice | Rationale |
|---|---|---|
| Parameter container | Single `AudioProcessorValueTreeState` with `ParameterLayout` | Standard JUCE pattern; supports all 29 params (Float / Int / Bool); presets in Stage 4 hang off this |
| Parameter ID style | UPPER_SNAKE_CASE matching `parameter-spec.md` | Already locked in spec; deviating would break the contract checksum |
| Oversampler allocation site | `prepareToPlay` (heap) | Sample-rate-dependent; cannot construct in ctor |
| Editor in Stage 1 | Minimal placeholder Component | Defers WebView complexity to Stage 3; keeps build green if WebView assets absent |
| Shared module wiring | CMake `target_link_libraries` to `scala-tuning-engine` and `note-expression` | Per ROADMAP §"Module dependencies"; APIs unused in Stage 1 but linked so Stage 2 can call them without revisiting CMake |
| State persistence format | APVTS XML via `copyState`/`replaceState` | Standard pattern; Stage 4 will extend with preset bank metadata |
| Channel config | Stereo out (2 ch) only | Matches BRIEF mono-source / stereo-out target; widening done in DSP (Stage 2) |
| MTS-ESP / Note Expression init in Stage 1 | Allocate but don't activate | Stage 2 Phase 2.6 wires these; Stage 1 only ensures linkage compiles |

---

## Dependencies on Earlier Work

**Stage 0 (complete):**
- ARCHITECTURE.md locked all 9 DSP components and parameter→component mappings
- ROADMAP.md sequenced 6 DSP sub-phases for Stage 2 (Phase 2.1 = highest-risk friction junction)
- 6 architecture decisions resolved with v1.1 deferrals where appropriate

**Modules (must exist before Stage 1 build):**
- `modules/tuning/scala-tuning-engine` — already exists (v2.1.0)
- `modules/tuning/note-expression` — already exists (requires `JUCE-NE-PATCH` applied to `~/JUCE/`)

---

## Open Questions

**None for Stage 1.**

Items deferred to later stages (already documented in ROADMAP §"Open Decisions Resolved"):
- Friction tier (Hyperbolic only v1.0 — locked)
- Module extraction timing for `bow-friction` (during Stage 2 Phase 2.1 — locked)
- Wood variants, sub-harmonic max depth, body size mapping (all locked for v1.0)

---

## Test Criteria (Stage 1 exit gate)

Inherited verbatim from `ROADMAP.md` §"Stage 1: Foundation → Test Criteria":

- [ ] `ninja O-Contrabass_VST3 O-Contrabass_AU` succeeds on macOS
- [ ] `cmake --build build --config Release --target O-Contrabass_VST3` succeeds on Windows
- [ ] All 29 APVTS parameters appear in DAW automation menu
- [ ] Plugin loads in Logic Pro, Ableton, Reaper, Dorico, Cubase without error
- [ ] pluginval strictness 10 passes (bypass mode — no audio output expected)
- [ ] `auval -a | grep -i contrabass` shows AU registered

---

## Next Phase

**Ready for:** research phase (Stage 1)

Research scope is narrow: confirm CMake patterns by inspecting a recent multi-format JUCE 8 plugin (O-Bowed, O-AnalogEQ as cross-platform reference) and confirm the APVTS bulk-registration idiom for 29 parameters. Most of the foundation is well-trodden territory — research will likely produce a thin RESEARCH.md.
