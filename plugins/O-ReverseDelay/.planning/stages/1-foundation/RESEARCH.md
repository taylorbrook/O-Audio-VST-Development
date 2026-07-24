# Stage 1: Foundation — RESEARCH

**Plugin:** O-ReverseDelay
**Stage:** 1-foundation
**Phase:** research — complete
**Date:** 2026-07-23
**Inputs:** stages/1-foundation/CONTEXT.md (discuss decisions D1–D3), parameter-spec-draft.md, suite source survey

## Scope

Foundation only: CMake target + APVTS shell + passthrough processor that passes
pluginval strictness 10. No DSP, no harness, no WebView. Research verified every
pattern against live suite source (JUCE 8.0.14).

## 1. CMake — reference template and registration

**Best sibling template: `plugins/O-GrainScatter/CMakeLists.txt`** — strip the
WebView/BinaryData sections (Stage 3 adds them back). Structure to copy:

- `include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)` at top
- `juce_add_plugin(OuariconReverseDelay ...)` with:
  - `COMPANY_NAME "${OUARICON_COMPANY_NAME}"` / `PLUGIN_MANUFACTURER_CODE ${OUARICON_MANUFACTURER_CODE}` — top-level CMakeLists.txt:24-31 sets these per dev/release branding (dev = `OuDv` + `-dev` suffix)
  - `PLUGIN_CODE ORvD` — **verified unique**: 0 collisions across all 38 plugin CMakeLists
  - `FORMATS VST3 AU Standalone`
  - `PRODUCT_NAME "O-ReverseDelay${OUARICON_DEV_SUFFIX}"`
  - **`VERSION 1.0.0`** — MUST be `VERSION`, not `PLUGIN_VERSION` (juce_add_plugin silently ignores unknown keywords → bundle ships as PROJECT_VERSION; `critical_plugin_version_keyword_ignored_by_juce`). Verify post-install: AU component version 1.0.0 encodes as 65536 in `auval -a` / Info.plist AudioComponents.
- Full juce module link list as in O-GrainScatter (includes `juce_dsp`, needed by Stage 2, and `juce_audio_processors`)
- `juce_generate_juce_header(OuariconReverseDelay)` **AFTER** `target_link_libraries` (critical-patterns §1)
- `target_compile_definitions`: `JUCE_VST3_CAN_REPLACE_VST2=0`, `JUCE_USE_CURL=0`. **Omit** `JUCE_WEB_BROWSER`/`NEEDS_WEB_BROWSER`/`NEEDS_WEBVIEW2` — Stage 3 concern.

**Registration:** none needed. Top-level CMakeLists.txt:47-58 auto-discovers any
`plugins/*/CMakeLists.txt` via glob. Creating the folder + CMakeLists is sufficient
(re-run cmake configure once so the glob picks it up).

**Target ≠ folder** (`OuariconReverseDelay` vs `O-ReverseDelay`) is already handled:
`build-and-install.sh` resolves via `resolve_cmake_target()` (`build_script_target_name_vs_folder`).
Build targets: `ninja OuariconReverseDelay_VST3 OuariconReverseDelay_AU OuariconReverseDelay_Standalone`.

## 2. Bus layouts (D1) — no exact suite precedent; canonical pattern

Surveyed all suite `isBusesLayoutSupported` implementations. Closest is
**O-DigiDelay** (`plugins/O-DigiDelay/Source/PluginProcessor.cpp:157`): accepts
mono→mono + stereo→stereo but **requires in == out** — that rejects mono→stereo,
so it does NOT satisfy D1. No suite plugin currently allows mono→stereo; this is
new ground. Canonical implementation for D1:

```cpp
bool isBusesLayoutSupported (const BusesLayout& layouts) const override
{
    const auto& in  = layouts.getMainInputChannelSet();
    const auto& out = layouts.getMainOutputChannelSet();

    if (in.isDisabled() || out.isDisabled())
        return false;

    const bool inOk  = in  == juce::AudioChannelSet::mono() || in  == juce::AudioChannelSet::stereo();
    const bool outOk = out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();

    // Accept mono→mono, mono→stereo, stereo→stereo; reject stereo→mono (no down-mix path)
    return inOk && outOk && in.size() <= out.size();
}
```

**Passthrough consequence for the shell:** with mono→stereo, `buffer` in
processBlock has `getTotalNumOutputChannels()` channels but only input channel 0
carries signal. The Stage-1 passthrough must copy ch0 → ch1 when
`totalNumInputChannels < totalNumOutputChannels` (the stock JUCE template's
"clear extra output channels" loop would leave ch1 silent — acceptable for a
shell, but duplicating mono into both outputs matches the Stage-2 capture-ring
behavior and is what pluginval's layout tests will exercise). Constructor uses
`BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true).withOutput("Output", juce::AudioChannelSet::stereo(), true)`
— stereo defaults; hosts negotiate mono via the override.

## 3. APVTS — 10 parameters, verified patterns

**Skew pattern** — `plugins/O-simpleGrain/Source/PluginProcessor.cpp:61-69` is the
suite reference:

```cpp
juce::NormalisableRange<float> range { 50.0f, 2000.0f, 0.01f };
range.setSkewForCentre (316.0f);   // geometric mean of endpoints
params.push_back (std::make_unique<juce::AudioParameterFloat>(
    juce::ParameterID { "delayTime", 1 }, "Delay Time", range, 500.0f,
    juce::AudioParameterFloatAttributes().withLabel ("ms")));
```

- `juce::ParameterID { id, 1 }` version-hint = 1 everywhere (suite convention; required for AU parameter stability).
- Skewed params + labels per the CONTEXT.md table: delayTime centre 316, grainSize centre 158, lowCut centre 200, highCut centre 3162. Linear: density, feedback, width, mix (%).
- Contract checksum note: param ID strings are load-bearing for Stage-3 UI binding — copy them character-for-character from CONTEXT.md (`delayTime, syncMode, noteDivision, grainSize, density, feedback, lowCut, highCut, width, mix`).

**Choice pattern** — `plugins/O-DigiDelay/Source/PluginProcessor.cpp:35-38` uses
`AudioParameterChoice` with a note-division `StringArray`. CAUTION: DigiDelay's
strings/order (`"1/4", "1/8", ...`) differ from our contract. Use the contract's
13 entries **in this exact order** (index = stored value; reordering later breaks
saved sessions):
`1/16, 1/16D, 1/16T, 1/8, 1/8D, 1/8T, 1/4, 1/4D, 1/4T, 1/2, 1/2D, 1/2T, 1/1` — default index 6 (`1/4`).
`syncMode`: choices `Free, Sync`, default index 1 (`Sync`).

**State round-trip** — standard APVTS XML pattern, reference
`plugins/O-DigiDelay/Source/PluginProcessor.cpp:305` (`copyState()` →
`copyXmlToBinary`; `getXmlFromBinary` → `replaceState`). No restore-guard/AsyncUpdater
needed in Stage 1 (nothing deferred to apply).

**Skew-related latent trap for later stages** (not Stage 1 work, record now):
`pattern_factory_preset_normalized_ignores_skew` — when factory presets arrive
(Stage 4), author them in engineering units + `convertTo0to1`, never hand-written
normalized fractions; 4 params here are skewed.

## 4. Processor shell details

- `processBlock`: `juce::ScopedNoDenormals`, ignore MIDI, early-return on empty
  buffer (DigiDelay pattern), passthrough + mono→stereo duplication per §2.
- No `setLatencySamples` call (zero latency; equal-power custom mix chosen in
  Stage 0 specifically to keep latency zero).
- `createEditor`: `return new juce::GenericAudioProcessorEditor (*this);`,
  `hasEditor() = true`. **Keep PluginProcessor.h/.cpp free of any editor-only
  includes** — Phase 2.1's render harness compiles the processor standalone
  (`pattern_render_harness_breaks_on_webview_editor` becomes relevant in Stage 3;
  starting clean now avoids it).
- `acceptsMidi()/producesMidi()/isMidiEffect()` all false; `getTailLengthSeconds()`
  return 0.0 for the shell (Stage 2 revisits — feedback tail exists then).

## 5. Validation gates (D2)

- Install: `./scripts/build-and-install.sh O-ReverseDelay` — Phase 4 does the AU
  cache clear + dual-variant sweep automatically.
- auval: `auval -a | grep -i reversedelay` (dev branding: subtype under OuDv).
- pluginval strictness 10, binary at
  `/Applications/pluginval.app/Contents/MacOS/pluginval` (path confirmed; suite
  battery script uses the same binary at strictness 8 + `--skip-gui-tests`):
  ```bash
  /Applications/pluginval.app/Contents/MacOS/pluginval --strictness-level 10 \
      --validate ~/Library/Audio/Plug-Ins/VST3/O-ReverseDelay-dev.vst3
  /Applications/pluginval.app/Contents/MacOS/pluginval --strictness-level 10 \
      --validate ~/Library/Audio/Plug-Ins/Components/O-ReverseDelay-dev.component
  ```
  Run 2–3× per `pattern_ci_pluginval10_catches_latent_nan` (level-10 fuzz is
  stochastic). A passthrough shell should pass trivially; any failure here is a
  bus-layout or state bug, not DSP.
- Layout check: pluginval exercises mono/stereo layout combos; additionally
  spot-check in a DAW that the plugin loads on both a mono and a stereo track.

## 6. Module reuse

**None for Stage 1.** Stage-0 in-suite prior art (O-GrainScatter DelayBuffer /
GrainScheduler, O-simpleGrain WindowLuts + harness) is Stage-2 material — the
foundation shell has no dependencies beyond JUCE. `preset-manager` is a Stage-4
candidate (not in contract yet). No `/module-add` needed now.

## 7. Pitfall checklist (from suite knowledge base)

| Pitfall | Applies | Mitigation in Stage 1 |
|---|---|---|
| `PLUGIN_VERSION` keyword ignored | YES | Use `VERSION 1.0.0`; verify AU version = 65536 |
| `juce_generate_juce_header` before link | YES | Place after `target_link_libraries` |
| Target ≠ folder breaks scripts | YES | Handled by `resolve_cmake_target()`; nothing to do |
| Dev/release variant shadowing | YES | Use build-and-install.sh (Phase 4 sweep) |
| Param ID shadows juce:: free fn (`end`/`begin`) | NO | No such IDs in the 10-param set |
| Class name shadows juce:: type | NO | No Sampler* names planned |
| Dual BinaryData namespace collision | NOT YET | Stage 3; note for GUI research |
| Editor types leak into processor | YES | GenericAudioProcessorEditor only; no editor includes in processor |
| Preset name "/" path bug | NOT YET | Stage 4 preset work |
| Factory presets vs skew | NOT YET | Recorded in §3 for Stage 4 |

## Open questions for plan phase

None blocking. One choice the plan should make explicit: whether the Stage-1
passthrough duplicates mono→ch1 (recommended, matches Stage-2 semantics) or
clears extra outputs (stock template). Research recommends duplication.
