# Stage 1 (Foundation + Shell) — Context

**Plugin:** O-simplePhysicalModelSynth
**Stage:** 1 of 4 (Foundation + Shell)
**Phase:** discuss
**Date:** 2026-06-26
**Mode:** manual (interactive discuss)

This file captures the discuss-phase decisions that bound Stage 1 execution. Most of the
design is already locked by Stage 0 (see `research/ARCHITECTURE.md`, `ROADMAP.md`,
`stages/0-ideation/CONTEXT.md`); this stage's open item was the parameter-spec finalization.

---

## Stage 1 Goal

Silent synth shell that loads, appears as an instrument in the DAW (IS_SYNTH), passes
pluginval (strictness 5+), with the full **17-param APVTS** wired and state persistence
working. No DSP — note input produces no audio yet, but causes no crashes.

## Discuss-phase decisions (this session)

1. **parameter-spec.md finalization → PROMOTE DRAFT.** User chose to promote
   `parameter-spec-draft.md` to the final `parameter-spec.md` now (folding in the Stage-0
   resolved DSP mappings), rather than running a UI mockup first. The UI mockup / WebView
   design happens at Stage 3 (GUI) as the workflow intends. → `parameter-spec.md` written.
2. **Parameter set → LOCK 17 AS-IS.** User locked the draft's exact 17 params / IDs /
   ranges / defaults as the zero-drift contract. No additions, removals, renames, or range
   changes. This is the binding APVTS contract for the rest of the build.

## Binding constraints for Stage 1 execution (from Stage 0)

### CMake
- `IS_SYNTH TRUE`, `NEEDS_MIDI_INPUT TRUE`, `NEEDS_WEB_BROWSER TRUE`, `NEEDS_WEBVIEW2 TRUE`
- Compile defs: `JUCE_WEB_BROWSER=1`, `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`, `JUCE_USE_CURL=0`
- `juce_generate_juce_header` AFTER `target_link_libraries`; link `juce_dsp`
- Dev branding by default (`use_development_branding: true` → `-dev` suffix, manufacturer `OuDv`)
- Single `juce_add_binary_data` target reserved for Stage 3 (WebView UI). If embedded presets
  ever become binary data, give the 2nd target a DISTINCT `NAMESPACE` (dual-namespace gotcha).

### Naming (shadowing guards)
- Voice class: a single `PhysicalModelVoice` stub for Stage 1 (engine-specific `StringVoice`/
  `ModalVoice` split, if any, is a Stage-2 DSP concern). **Never** `SamplerVoice`/`SamplerSound`
  (shadows `juce::` types under JuceHeader's `using namespace juce`).
- ParamIDs in a namespace; no bare `end`/`begin` symbols (none in this param set).

### APVTS
- All 17 params from `parameter-spec.md`, IDs verbatim, O-simpleFM `ParamIDs` pattern.
- `getStateInformation`/`setStateInformation` round-trip via APVTS state.
- 16-voice `juce::Synthesiser` with silent `PhysicalModelVoice` stub; output-only stereo bus.
- `GenericAudioProcessorEditor` placeholder (real WebView editor is Stage 3).

### Render-harness scaffold (Stage-2 gate, scaffolded here)
- `tests/render-harness/` off by default (`-DOUARICON_BUILD_TESTS=ON`).
- Compile under `JUCE_WEB_BROWSER=0`; drop `PluginEditor.cpp` from harness sources;
  `#if JUCE_WEB_BROWSER` guard around `createEditor`. (Autocorrelation pitch probe arrives in Stage 2.)

### Toolchain
- JUCE 8.0.9, CMake + Ninja, local JUCE at `/Users/taylorbrook/JUCE`.

## Direct in-house reuse (foundation patterns)
- O-simpleFM `CMakeLists.txt` / `PluginProcessor.h` — WebView2 flags, 16-voice synth structure,
  `ParamIDs` namespace, harness gate. Closest structural analog (pedagogical synth sibling).

## Stage 1 success criteria (from ROADMAP)
- [ ] VST3 + AU build; plugin appears in DAW instrument list (IS_SYNTH)
- [ ] pluginval passes (strictness 5+)
- [ ] All 17 params visible in generic editor; state save/restore round-trips
- [ ] Silent (no audio yet); no crashes on note input
- [ ] Verifies COMPAT-01 (pluginval VST3+AU), COMPAT-02 (WebView2 flags)

## Out of scope for Stage 1
- Any DSP / audio generation (Stage 2)
- WebView UI (Stage 3)
- Presets, optimization, edge-case polish (Stage 4)

---
*Discuss phase complete. Next: research phase (Foundation/shell patterns) → `/plugin-research O-simplePhysicalModelSynth`.*
