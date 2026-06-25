# Stage 1 (Foundation) — RESEARCH

**Plugin:** O-simpleSampler · **Stage:** 1 Foundation + Shell · **Date:** 2026-06-25
**Depth:** Lean. Foundation is a near-verbatim adaptation of the **O-simpleGrain Stage-1 shell** (commit `2ae282e`) — same plugin class shape, same APVTS+custom-state idiom, same WebView/CMake config. No novel research; this file records the reused pattern, the JUCE 8 APIs, and the traps to avoid.

## 1. Reference implementation (copy, don't reinvent)

O-simpleGrain's foundation commit is the exact precedent — a silent synth shell with a full APVTS + a custom non-APVTS source-identity child + WebView2 flags set up front, with binary-data targets deferred. We mirror it and change only:

| Aspect | O-simpleGrain (8-voice grain) | O-simpleSampler (16-voice sampler) |
|--------|-------------------------------|-------------------------------------|
| Class / namespace | `OSimpleGrainAudioProcessor` / `OSimpleGrain::ParamIDs` | `OSimpleSamplerAudioProcessor` / `OSimpleSampler::ParamIDs` |
| PLUGIN_CODE | `OsGr` | `OsSm` |
| Param count | 18 | **21** (parameter-spec.md) |
| Polyphony const | `kMaxVoices = 8` | `kMaxVoices = 16` |
| Source cap | `kMaxSourceSeconds = 10` | `kMaxSourceSeconds = 30` |
| Built-in choice | fire/voice/water/piano | piano/vocal/flute/vinyl |
| Default identity | `embedded:fire` | `embedded:piano` |

Everything else (BusesProperties output-only, `isBusesLayoutSupported` mono/stereo, silent `processBlock`, `get/setStateInformation` APVTS-tree + `SOURCE/identity` child, `createPluginFilter`, placeholder editor) is structurally identical.

## 2. JUCE 8 APIs used (verified against local 8.0.9 + shipped siblings)

- `juce::AudioProcessorValueTreeState` + `createParameterLayout()` returning `ParameterLayout`. Versioned `juce::ParameterID { id, 1 }`.
- Param types: `juce::AudioParameterFloat` (+ `AudioParameterFloatAttributes().withLabel(...)`), `juce::AudioParameterChoice`, `juce::AudioParameterBool`, `juce::AudioParameterInt` (for `rootKey`, `tune`).
- `juce::NormalisableRange<float>` — explicit `{min,max,interval,skew}` for `loopCrossfade`/ADSR; `setSkewForCentre(...)` for `filterCutoff` (log feel, centre ≈1 kHz). Linear ranges for the percent params.
- `apvts.getRawParameterValue(id)` → cached `std::atomic<float>*` (assigned in ctor).
- State: `apvts.copyState()` → `ValueTree`; `getOrCreateChildWithName("SOURCE")` + `setProperty("identity", …)`; `createXml`/`copyXmlToBinary`; restore via `getXmlFromBinary`/`ValueTree::fromXml`/`apvts.replaceState`. Guard `state.getType() == apvts.state.getType()`.
- `setLatencySamples(0)` in `prepareToPlay`. **`getLatencySamples()` is non-virtual in JUCE 8 — never override** (project memory).
- `juce::ScopedNoDenormals` at top of `processBlock`.

## 3. CMake (mirror O-simpleGrain foundation)

- `include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)` first → provides `OUARICON_COMPANY_NAME`, `OUARICON_MANUFACTURER_CODE`, `OUARICON_DEV_SUFFIX`.
- `juce_add_plugin`: `IS_SYNTH TRUE`, `NEEDS_MIDI_INPUT TRUE`, `NEEDS_MIDI_OUTPUT FALSE`, `IS_MIDI_EFFECT FALSE`, `NEEDS_WEB_BROWSER TRUE`, `NEEDS_WEBVIEW2 TRUE`, `EDITOR_WANTS_KEYBOARD_FOCUS FALSE`, FORMATS `VST3 AU Standalone`.
- `target_sources`: the four PluginProcessor/Editor files only.
- Standard JUCE module link set (audio_basics/devices/formats/plugin_client/processors/utils, core, data_structures, dsp, events, graphics, gui_basics, gui_extra) + recommended config/lto/warning flags.
- `juce_generate_juce_header` AFTER `target_link_libraries` (JUCE 8 requirement).
- `target_compile_definitions PUBLIC`: `JUCE_VST3_CAN_REPLACE_VST2=0`, `JUCE_WEB_BROWSER=1`, `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`, `JUCE_USE_CURL=0`.
- Root CMake auto-discovers via `file(GLOB plugins/*)` + `add_subdirectory` — no root edit needed.

## 4. Pitfalls (from project memory + sibling lessons)

1. **Dual `juce_add_binary_data` NAMESPACE collision** (O-simpleGrain Stage-3.1, in MEMORY.md): embedded samples + WebView UI both default to `NAMESPACE BinaryData` → duplicate-symbol link failure. Fix: distinct `NAMESPACE` *and* `HEADER_NAME` (`BinaryData`/`BinaryData.h` for samples, `UIBinaryData`/`UIBinaryData.h` for UI). **Not built in Stage 1** (no assets) — encoded as TODO comments so it's right when the targets land.
2. **WebView2 both-flags rule** (MEMORY.md): `NEEDS_WEBVIEW2 TRUE` + `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` must BOTH be set or Windows WebView silently blanks. Set now even though the WebView is Stage 3.
3. **Referencing non-existent binary-data sources breaks configure** — defer both binary-data targets; do not invent placeholder blobs.
4. **`getLatencySamples()` non-virtual in JUCE 8** — use `setLatencySamples(0)`.
5. **Dev/release variant shadowing** (MEMORY.md) — handled at install time by `build-and-install.sh` Phase 4; not a Stage-1 code concern, but `OUARICON_DEV_SUFFIX` in `PRODUCT_NAME` is what makes the `-dev` bundle, so keep it.

## 5. What Stage 1 deliberately does NOT include

No `juce::Synthesiser`/voice/sound members, no `MidiMessageCollector`, no viz taps, no source-decode helpers, no drag-drop session state — all of those entered O-simpleGrain in Stage 2/3, not the foundation. The Stage-1 shell is intentionally minimal: APVTS + cached atomics + custom state + silent `processBlock` + placeholder editor.

## Recommendation

Implement directly by mirroring the O-simpleGrain foundation commit, adapting the table in §1 and building the 21-param `createParameterLayout()` from `parameter-spec.md`. Verify with a clean `ninja` build of all three formats + a pluginval/param check.
