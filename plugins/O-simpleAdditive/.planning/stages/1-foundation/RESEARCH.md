# Stage 1: Foundation — RESEARCH

**Plugin:** O-simpleAdditive · **Stage:** 1 (Foundation) · **Date:** 2026-06-22
**Verdict:** Zero novel research. Foundation is a verified port of shipped O-simpleFM (v1.2.1)
with documented subtractions. Patterns below are confirmed against local JUCE 8.0.9 source and
the suite's critical-patterns knowledge base.

---

## 1. JUCE APIs (all confirmed present in O-simpleFM, JUCE 8.0.9)

| Need | API | Notes |
|------|-----|-------|
| Plugin base | `juce::AudioProcessor` | Standard. |
| Parameters | `juce::AudioProcessorValueTreeState` | `createParameterLayout()` static builder. |
| Float param | `juce::AudioParameterFloat` + `juce::NormalisableRange<float>` | Drawbars/percent stored 0–1; ADSR times 0.001–5 s with 0.35 skew. |
| Choice param | `juce::AudioParameterChoice` | `frameBSource` (4), `bitDepth` (7). **New vs O-simpleFM** (which had none) but standard API. |
| Bool param | `juce::AudioParameterBool` | Not needed for the 33-param set (no snap toggle). |
| Output gain | `juce::SmoothedValue<float>` + `juce::Decibels::decibelsToGain` | dB→lin, 20 ms ramp. Verbatim from O-simpleFM. |
| Denormals | `juce::ScopedNoDenormals` | Top of `processBlock`. |
| State | `copyXmlToBinary` / `getXmlFromBinary` + `apvts.copyState()/replaceState()` | Plain APVTS XML (no preset-manager wrapper at Foundation). |
| Latency | `setLatencySamples(0)` in `prepareToPlay` | `getLatencySamples()` is **non-virtual** in JUCE 8 — do not override (memory: confirmed). |

## 2. AudioParameterChoice — the one genuinely new pattern

O-simpleFM has no choice params; O-simpleAdditive adds two. Confirmed idiom (used across the suite, e.g. O-MicrotonalSampler `dynamics_mode`):

```cpp
params.push_back (std::make_unique<juce::AudioParameterChoice>(
    juce::ParameterID { frameBSource, 1 }, "Frame B Source",
    juce::StringArray { "Sine", "Saw", "Square", "Odd" }, 1 /*default index = Saw*/));

params.push_back (std::make_unique<juce::AudioParameterChoice>(
    juce::ParameterID { bitDepth, 1 }, "Bit Depth",
    juce::StringArray { "Off", "12", "10", "8", "6", "4", "2" }, 0 /*default index = Off*/));
```

- APVTS stores the **choice index** as the raw value; `getRawParameterValue(id)->load()` returns the index as float. DSP (Stage 2) maps index→meaning. Foundation only needs them present + persisted.
- Stage 3 GUI will bind these via `WebComboBoxRelay` / `getComboBoxState` (critical-pattern #19) — flagged for later, not this stage.

## 3. CMake config (port of O-simpleFM CMakeLists.txt)

Confirmed required for a WebView synth that Stage 3 will inherit cleanly:

```
IS_SYNTH TRUE
NEEDS_MIDI_INPUT TRUE
NEEDS_WEB_BROWSER TRUE        # set now; WebView added Stage 3
NEEDS_WEBVIEW2 TRUE          # Windows static-link prerequisite
PLUGIN_CODE OSiA             # free (OSiF=simpleFM taken; verified no collision)
FORMATS VST3 AU Standalone
```
Compile defs: `JUCE_WEB_BROWSER=1`, `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`, `JUCE_USE_CURL=0`, `JUCE_VST3_CAN_REPLACE_VST2=0`.
**Memory (critical):** `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` is mandatory whenever `NEEDS_WEBVIEW2 TRUE` — else Windows silently shows a blank UI. Setting both now means Stage 3 doesn't have to revisit CMake.

**Foundation subtractions from O-simpleFM CMake:** no `juce_add_binary_data` (no UI yet), no `ouaricon_add_module(preset-manager)` (Stage 4), no render-harness `tests/` (Stage 2). Keep `include(OuariconModules.cmake)` only if convenient; branding vars (`OUARICON_COMPANY_NAME`, `OUARICON_MANUFACTURER_CODE`, `OUARICON_DEV_SUFFIX`) come from root scope and are available regardless.

Plugins auto-register via root `file(GLOB plugins/*)` → just creating `plugins/O-simpleAdditive/CMakeLists.txt` is sufficient.

## 4. Deltas vs O-simpleFM (what to REMOVE)

| O-simpleFM machinery | Keep at Foundation? | Why |
|----------------------|---------------------|-----|
| `juce::dsp::Oversampling`, `kOsFactorLog2`, `scaledMidi`, chunked OS render | **REMOVE** | Additive band-limits exactly; no oversampling, zero latency. |
| `lastNoteHz`, `carrierSounding`, `getCarrierHz()` | **REMOVE** | FM sideband-marker viz; N/A for additive, and viz is Stage 2.3/3.2. |
| `midiCollector`, `handleUiMidi`, `MidiMessageCollector` | **REMOVE** | On-screen keyboard = Stage 3 GUI. |
| `VizRing vizRing`, viz tap in processBlock | **REMOVE** | Stage 2.3. |
| `OuariconPresetManager`, `FactoryPresets`, `initializeFactoryPresets` | **REMOVE** | Stage 4. Foundation persists plain APVTS XML. |
| `juce::Synthesiser synth`, voices, `addVoice/addSound` | **DEFER** | Add in Stage 2.1 with `AdditiveVoice`. Foundation has no voices — `processBlock` is silent (clear + gain + NaN scrub). |

## 5. Pitfalls (from suite knowledge base — apply now)

1. **Non-virtual `getLatencySamples()`** — use `setLatencySamples(0)`, never override. ✅ accounted for.
2. **AU cache shadowing** — build-and-install must sweep both `-dev` and unsuffixed bundles. Handled by `scripts/build-and-install.sh` Phase 4. (Verify only; not a code concern.)
3. **WebView2 static-linking flag** — set at Foundation (see §3) so Stage 3 inherits. ✅.
4. **`createParameterLayout` order / count** — must total exactly 33; mismatched count is the only realistic Foundation bug. Verify count in the generic editor.

## 6. Reference files

- `plugins/O-simpleFM/CMakeLists.txt` — CMake template.
- `plugins/O-simpleFM/Source/PluginProcessor.{h,cpp}` — processor + layout template.
- `plugins/O-simpleAdditive/.planning/parameter-spec.md` — the 33-param contract (IDs, ranges, defaults).
- `plugins/O-simpleAdditive/.planning/research/ARCHITECTURE.md` — DSP architecture (Stage 2 reference; informs param ranges).
