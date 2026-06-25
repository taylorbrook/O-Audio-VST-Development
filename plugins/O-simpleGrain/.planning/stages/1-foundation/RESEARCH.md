# Stage 1 (Foundation) — RESEARCH

**Plugin:** O-simpleGrain · **Stage:** 1 Foundation · **Date:** 2026-06-24
**Mode:** Non-interactive (momentum path). Foundation patterns are fully de-risked by the shipped O-simpleFM template + project memory; this captures the concrete reuse map and JUCE 8 gotchas.

## Primary Template: O-simpleFM (verbatim structural reuse)

O-simpleFM is a **shipped pedagogical synth** whose Stage-1 shell is structurally identical to what O-simpleGrain needs (its CMake comment literally states *"Stage 1 (Foundation): silent synth shell with full 17-param APVTS + state persistence"*). **Mirror it.**

Reuse map:
| O-simpleGrain file | Copy structure from | Adapt |
|---|---|---|
| `CMakeLists.txt` | `plugins/O-simpleFM/CMakeLists.txt` | `PLUGIN_CODE OsGr`; `PRODUCT_NAME "O-simpleGrain${OUARICON_DEV_SUFFIX}"`; **DROP** the `juce_add_binary_data` UI block + `ouaricon_add_module` (Stage 3); keep synth + WebView2 flags. |
| `Source/PluginProcessor.{h,cpp}` | O-simpleFM `PluginProcessor.{h,cpp}` | Replace 17 FM params with the 18 grain params; add custom loaded-source-identity child to state; silent `processBlock`. |
| `Source/PluginEditor.{h,cpp}` | O-simpleFM `PluginEditor.{h,cpp}` | Strip the WebView (Stage 3). Minimal placeholder editor that compiles. |

## CMake (critical facts)

- Root `CMakeLists.txt` **auto-discovers** any `plugins/*/CMakeLists.txt` (glob loop, lines 48–58). **No manual registration needed.**
- `include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)` first → provides `OUARICON_COMPANY_NAME`, `OUARICON_MANUFACTURER_CODE`, `OUARICON_DEV_SUFFIX`, `ouaricon_add_module`.
- Required flags for an instrument with a (future) WebView: `IS_SYNTH TRUE`, `NEEDS_MIDI_INPUT TRUE`, `NEEDS_WEB_BROWSER TRUE`, `NEEDS_WEBVIEW2 TRUE`, FORMATS `VST3 AU Standalone`.
- Compile defs: `JUCE_VST3_CAN_REPLACE_VST2=0`, `JUCE_WEB_BROWSER=1`, `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` (auto-defines `JUCE_USE_WIN_WEBVIEW2=1`), `JUCE_USE_CURL=0`.
- `juce_generate_juce_header(O-simpleGrain)` **after** `target_link_libraries` (JUCE 8 requirement).
- `PLUGIN_CODE OsGr` confirmed unique (suite scan: OSiF/OSiA/OuGS/… — no `OsGr`).

## APVTS Parameter Layout (JUCE 8)

- `static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()`; construct APVTS in the ctor init list: `apvts(*this, nullptr, "PARAMETERS", createParameterLayout())`.
- Float params: `std::make_unique<juce::AudioParameterFloat>(ParameterID{"id", 1}, "Name", juce::NormalisableRange<float>(min, max, step, skew), def)`. Use the **versioned `ParameterID{ "id", 1 }`** ctor (JUCE 8 hosts).
- Skew: `NormalisableRange` skew factor — `grainSize`~0.4, ADSR times~0.35, `density` log skew (use `NormalisableRange::setSkewForCentre` or an explicit skew <1 for fine low-end). `density` log: `setSkewForCentre(20.f)` is a reasonable musical centre.
- `freeze` → `juce::AudioParameterBool({"freeze",1}, "Freeze", false)`.
- `sourceSample` → `juce::AudioParameterChoice({"sourceSample",1}, "Source", {"fire","voice","water","piano"}, 0)`.
- `windowShape` → `juce::AudioParameterChoice({"windowShape",1}, "Window", {"Rectangular","Triangular","Welch","Gaussian","Hann"}, 4)` (default Hann = index 4).
- Cache raw atomics in `prepareToPlay`/ctor: `gainParam = apvts.getRawParameterValue("outputLevel");` etc. (read once per block in Stage 2; not used while silent).

## State Persistence (custom child — the one non-boilerplate piece)

- Standard: `getStateInformation` → `apvts.copyState().createXml()` → `copyXmlToBinary`. `setStateInformation` → `getXmlFromBinary` → `apvts.replaceState(ValueTree::fromXml(...))`.
- **Custom loaded-source identity:** add a child `ValueTree` (e.g. `"SOURCE"` with property `"identity"`, default `"embedded:fire"`) onto the state tree before serializing, and read it back in `setStateInformation`. Pattern: keep a `juce::String currentSourceIdentity { "embedded:fire" }` member; on save, `auto state = apvts.copyState(); state.setProperty(...)` OR append a child; on load, restore the member. (Stage 2.3 wires it to actual loading; Stage 1 just round-trips the string.)
- O-MicrotonalSampler persists a loaded-source path similarly — reference for the child-state shape if needed.

## JUCE 8 Gotchas (project memory — must hold)

- **`getLatencySamples()` is non-virtual** — never override. Call `setLatencySamples(0)` in `prepareToPlay`.
- WebView2 **both** flags (`NEEDS_WEBVIEW2 TRUE` + `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`) or Windows WebView silently blanks. (UI is Stage 3, but flags are set now.)
- Synth bus: no audio input. `isBusesLayoutSupported` accepts mono or stereo **output**, input disabled. `IS_SYNTH TRUE` + `NEEDS_MIDI_INPUT TRUE` or the plugin is silent/invisible as an instrument (critical-pattern #22).
- `processBlock`: `juce::ScopedNoDenormals`; clear output; consume `midiMessages` (Stage 2 routes to the synth). **Allocation-free** even on the silent path.

## Pitfalls to avoid

- Do **not** add `juce_add_binary_data` / `ouaricon_add_module` / UI files in Stage 1 — no `ui/` assets exist yet; adding empty refs breaks the build. Defer to Stage 3.
- Do **not** invent built-in `.wav` blobs — none exist. Leave `# TODO(Stage 2.3): embed built-in .wav sources` near where the binary-data target will go. `sourceSample` stays a plain choice param.
- Do **not** implement grain DSP — `processBlock` is silent in Stage 1.
