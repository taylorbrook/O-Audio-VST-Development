# Stage 1: Foundation — PLAN

**Plugin:** O-ReverseDelay
**Stage:** 1-foundation
**Phase:** plan — complete
**Date:** 2026-07-23
**Inputs:** CONTEXT.md (D1–D3), RESEARCH.md, parameter-spec-draft.md

## Goal

Create the O-ReverseDelay plugin project: CMake build target `OuariconReverseDelay`
(VST3 + AU + Standalone) and a 10-parameter APVTS processor shell with clean
passthrough. Builds with ninja, installs via `build-and-install.sh`, and passes
pluginval strictness 10 on both formats (COMPAT-01). No DSP, no harness, no WebView.

## Plan Decision (from RESEARCH.md open question)

Stage-1 passthrough **duplicates mono input into all extra output channels**
(ch0 → ch1 when in < out), per research recommendation — matches Stage-2
capture-ring semantics and satisfies pluginval's mono→stereo layout exercise.

## Tasks

1. [ ] Create plugin directory + CMakeLists.txt
   - Files: `plugins/O-ReverseDelay/CMakeLists.txt`
   - Template: `plugins/O-GrainScatter/CMakeLists.txt` with WebView/BinaryData sections stripped
   - `juce_add_plugin(OuariconReverseDelay ...)`: `COMPANY_NAME "${OUARICON_COMPANY_NAME}"`,
     `PLUGIN_MANUFACTURER_CODE ${OUARICON_MANUFACTURER_CODE}`, `PLUGIN_CODE ORvD`,
     `FORMATS VST3 AU Standalone`, `PRODUCT_NAME "O-ReverseDelay${OUARICON_DEV_SUFFIX}"`,
     **`VERSION 1.0.0`** (NOT `PLUGIN_VERSION`)
   - Link full juce module list incl. `juce::juce_dsp` + `juce::juce_audio_processors`;
     `juce_generate_juce_header()` AFTER `target_link_libraries`
   - `target_compile_definitions`: `JUCE_VST3_CAN_REPLACE_VST2=0`, `JUCE_USE_CURL=0`;
     omit all WebView flags
   - Depends on: none

2. [ ] Create parameter layout (APVTS, 10 params)
   - Files: `plugins/O-ReverseDelay/Source/PluginProcessor.h`, `.cpp` (createParameterLayout)
   - IDs character-exact: `delayTime, syncMode, noteDivision, grainSize, density, feedback, lowCut, highCut, width, mix`
   - `juce::ParameterID { id, 1 }` version-hint 1 on every param
   - Skewed floats via `setSkewForCentre`: delayTime 316 (50–2000 ms, def 500),
     grainSize 158 (50–500 ms, def 200), lowCut 200 (20–2000 Hz, def 100),
     highCut 3162 (500–20000 Hz, def 8000)
   - Linear floats (%): density def 60, feedback def 40, width def 60, mix def 35
   - Choices: `syncMode` {Free, Sync} def index 1; `noteDivision` 13 entries in
     contract order `1/16, 1/16D, 1/16T, 1/8, 1/8D, 1/8T, 1/4, 1/4D, 1/4T, 1/2, 1/2D, 1/2T, 1/1` def index 6
   - Labels: ms / % / Hz via `AudioParameterFloatAttributes().withLabel(...)`
   - No bypass parameter
   - Depends on: Task 1

3. [ ] Implement bus layout support (D1)
   - Files: `Source/PluginProcessor.cpp` (constructor `BusesProperties`, `isBusesLayoutSupported`)
   - Constructor: stereo in / stereo out defaults
   - Accept mono→mono, mono→stereo, stereo→stereo; reject stereo→mono and
     disabled buses (`in.size() <= out.size()` guard, exact snippet in RESEARCH.md §2)
   - Depends on: Task 2

4. [ ] Implement passthrough processBlock + shell overrides
   - Files: `Source/PluginProcessor.cpp`
   - `ScopedNoDenormals`; early-return on empty buffer; copy ch0 → extra output
     channels when inputs < outputs (plan decision above)
   - `acceptsMidi/producesMidi/isMidiEffect` false; `getTailLengthSeconds` 0.0;
     no `setLatencySamples`
   - Depends on: Task 3

5. [ ] State save/restore + generic editor
   - Files: `Source/PluginProcessor.cpp`, `Source/PluginEditor.*` NOT created
   - `getStateInformation`/`setStateInformation`: APVTS XML round-trip
     (`copyState` → `copyXmlToBinary`; `getXmlFromBinary` → `replaceState`)
   - `createEditor`: `new juce::GenericAudioProcessorEditor(*this)`; `hasEditor` true
   - PluginProcessor.h/.cpp must contain zero editor-only includes (harness constraint)
   - Depends on: Task 2

6. [ ] Configure + build
   - Commands: re-run cmake configure once (glob pickup), then
     `ninja OuariconReverseDelay_VST3 OuariconReverseDelay_AU OuariconReverseDelay_Standalone`
   - Fix any compile/link issues
   - Depends on: Tasks 1–5

7. [ ] Install + validate (D2 gates)
   - `./scripts/build-and-install.sh O-ReverseDelay` (handles cache clear + dual-variant sweep)
   - `auval -a | grep -i reversedelay` — AU listed; component version encodes 65536 (1.0.0)
   - pluginval strictness 10 on both bundles, run 2–3× each:
     `/Applications/pluginval.app/Contents/MacOS/pluginval --strictness-level 10 --validate ~/Library/Audio/Plug-Ins/VST3/O-ReverseDelay-dev.vst3`
     and same for `~/Library/Audio/Plug-Ins/Components/O-ReverseDelay-dev.component`
   - Depends on: Task 6

## Success Criteria (verify phase gates)

- [ ] Clean ninja build: `OuariconReverseDelay_VST3` + `OuariconReverseDelay_AU`
- [ ] Installs via `./scripts/build-and-install.sh O-ReverseDelay`
- [ ] `auval` lists the AU; component version encodes 1.0.0 (65536)
- [ ] pluginval strictness 10 passes on VST3 and AU, 2–3 consecutive runs (COMPAT-01)
- [ ] All 10 params visible in GenericAudioProcessorEditor with correct ranges/defaults/units
- [ ] delayTime skew spot-check: normalized 0.5 ≈ 315–316 ms
- [ ] State save/restore round-trips all 10 params
- [ ] Bus layouts per D1: mono→mono, mono→stereo, stereo→stereo accepted; stereo→mono rejected
- [ ] mono→stereo passthrough carries signal on both output channels

## Out of Scope

- Any DSP (capture ring, grains, filters, feedback) — Stage 2
- Render harness — Phase 2.1 first deliverable
- WebView UI, BinaryData, preset manager — Stages 3–4
- Windows CI — later stage
