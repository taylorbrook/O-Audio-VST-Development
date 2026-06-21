# Stage 1 (Foundation) — RESEARCH

**Plugin:** O-simpleFM · **Stage:** 1 Foundation · **Date:** 2026-06-20
**Note:** Foundation research is fully covered by the Stage 0 ARCHITECTURE.md (JUCE modules verified against local 8.0.9) + in-repo reference plugins. This file records the concrete patterns reused, so execute needs no further investigation.

## Build registration

- Top-level `CMakeLists.txt` auto-discovers plugins via `file(GLOB PLUGIN_DIRS "plugins/*")` + `add_subdirectory`. **No registration edit needed** — creating `plugins/O-simpleFM/CMakeLists.txt` is sufficient (re-run cmake configure to pick it up).
- `modules/cmake/OuariconModules.cmake` provides `OUARICON_COMPANY_NAME`, `OUARICON_MANUFACTURER_CODE`, `OUARICON_DEV_SUFFIX`, and `ouaricon_add_module()`.

## Reference patterns (verbatim sources)

| Concern | Reference | What to copy |
|---------|-----------|--------------|
| Synth + WebView2 CMake | `plugins/O-Bassoon/CMakeLists.txt` | `juce_add_plugin` flags, JUCE module link list, compile defs. **Drop** note-expression/tuning/preset-manager/UI-resources (not needed at Foundation). |
| Cross-platform WebView2 flags | `plugins/O-AnalogEQ/CMakeLists.txt` | `NEEDS_WEB_BROWSER TRUE`, `NEEDS_WEBVIEW2 TRUE`, defs `JUCE_WEB_BROWSER=1`, `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`, `JUCE_USE_CURL=0`. |
| Synth processor shape | `plugins/O-Bassoon/Source/PluginProcessor.h` | `AudioProcessor` overrides, `acceptsMidi()=true`, `getAPVTS()`, `createParameterLayout()` static, member-init order. |
| APVTS param construction | ARCHITECTURE.md → Parameter Mapping | `ParameterID{id,1}` + `NormalisableRange(min,max,step,skew)`; bool via `AudioParameterBool`. |

## JUCE 8 APIs (verified, ARCHITECTURE.md §Research References)

- `juce::AudioProcessorValueTreeState`, `juce::AudioParameterFloat/Bool`, `juce::NormalisableRange<float>` — param layout.
- `juce::ScopedNoDenormals` — top of processBlock (feedback loops stall on denormals; harmless to add now).
- `juce::GenericAudioProcessorEditor` — temporary Stage 1 editor (replaced by WebView in Stage 3).
- `juce::Synthesiser` / `SynthesiserVoice` — **declared/wired in Stage 2**, not Foundation.

## Decisions for execute

1. **PLUGIN_CODE = `OSiF`** (unique; `OuFm` taken). Manufacturer code inherited from suite var.
2. **WebView2 flags included at Foundation** (ROADMAP: "CMake (synth + WebView2 flags)") but **no `juce_add_binary_data`** and **no WebView editor** yet — UI files don't exist until Stage 3. Plugin builds clean with the flags present and a `GenericAudioProcessorEditor`.
3. **Silent processBlock**: `ScopedNoDenormals` + `buffer.clear()`; consume the MidiBuffer (no-op). No voice rendering.
4. **Bus layout**: output-only stereo (`BusesProperties().withOutput("Output", stereo(), true)`); `isBusesLayoutSupported` accepts mono/stereo output.
5. **No shared modules** at Foundation (preset-manager added in Stage 4 if needed; note-expression N/A — O-simpleFM is not a Dorico/microtonal plugin).

## Pitfalls guarded (from project memory)

- Omitting `IS_SYNTH`/`NEEDS_MIDI_INPUT` → silent plugin / no MIDI (critical-pattern #22).
- WebView2 static-linking flag must accompany `NEEDS_WEBVIEW2 TRUE` (memory: WebView2 static vs dynamic) — included now so Stage 3 inherits a correct config.
- Dev/release variant shadowing in AU registry — handled at install time by `build-and-install.sh` Phase 4, not a Foundation concern.
