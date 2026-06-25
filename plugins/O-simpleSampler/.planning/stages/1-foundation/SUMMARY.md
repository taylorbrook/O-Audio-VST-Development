# Stage 1 (Foundation) — SUMMARY

**Plugin:** O-simpleSampler · **Stage:** 1 Foundation + Shell · **Date:** 2026-06-25
**Result:** ✅ Complete — silent 16-voice synth shell builds (VST3 + AU + Standalone), passes pluginval (strictness 5) and `auval` (AU VALIDATION SUCCEEDED), exposes all 21 APVTS parameters.

## What was implemented

A valid, silent instrument shell mirroring the O-simpleGrain foundation pattern (commit `2ae282e`), adapted for the sampler (16 voices, 21 params, 30 s source cap, `OsSm` code).

### Files created
| File | Purpose |
|------|---------|
| `CMakeLists.txt` | `juce_add_plugin` synth (`IS_SYNTH`, `NEEDS_MIDI_INPUT`, `NEEDS_WEB_BROWSER`, `NEEDS_WEBVIEW2`), FORMATS VST3/AU/Standalone, `PLUGIN_CODE OsSm`, `VERSION 0.1.0`, WebView2 + `JUCE_USE_CURL=0` defs. Binary-data targets (samples + UI) **deferred** with dual-NAMESPACE TODOs. |
| `Source/PluginProcessor.h` | `OSimpleSamplerAudioProcessor`; `OSimpleSampler::ParamIDs` (21 IDs); 21 cached `std::atomic<float>*`; engine constants; custom source-identity state; built-in name table. |
| `Source/PluginProcessor.cpp` | `createParameterLayout()` (21 params, exact ranges/skews); ctor caches atomics; silent `processBlock`; output-only `isBusesLayoutSupported`; `get/setStateInformation` (APVTS tree + `SOURCE/identity` child); `setLatencySamples(0)`; `createPluginFilter`. |
| `Source/PluginEditor.{h,cpp}` | Minimal 720×480 placeholder editor painting "O-simpleSampler — Stage 1 shell". |

### The 21 parameters (as implemented, per `parameter-spec.md`)
`sourceSample` (Choice piano/vocal/flute/vinyl) · `start` (0–100%, 0) · `end` (0–100%, 100) · `loopMode` (Choice Off/Forward/Ping-Pong) · `loopStart` (0–100%, 0) · `loopEnd` (0–100%, 100) · `loopCrossfade` (0–500 ms, 10, skew 0.4) · `reverse` (Bool) · `rootKey` (Int 0–127, 60) · `pitchMode` (Choice Repitch/Stretch) · `tune` (Int −24–24, 0) · `fine` (−100–100 cents) · `vintage` (0–100%, 0) · `filterCutoff` (20–20000 Hz, 20000, skew-for-centre 1 kHz) · `filterResonance` (0–100%, 0) · `ampAttack` (0–5 s, 0.005, skew 0.35) · `ampDecay` (0–5 s, 0.3) · `ampSustain` (0–1, 1.0) · `ampRelease` (0–5 s, 0.2) · `velToAmp` (0–100%, 50) · `outputLevel` (−60–0 dB, 0).

### State persistence
`getStateInformation` serializes the APVTS tree + a custom `SOURCE` child carrying `identity` (default `embedded:piano`); `setStateInformation` restores the identity (guarded by tree-type check) then `replaceState`. Round-trips both params and loaded-source identity.

## Deviation from plan

- **`start`/`end` param-ID C++ identifiers renamed to `regionStart`/`regionEnd`.** A bare `end` under `using namespace OSimpleSampler::ParamIDs` collides with `juce::end` (RangedDirectoryIterator free function) → build error. The **APVTS string IDs remain `"start"`/`"end"`** (parameter-spec contract unchanged); only the C++ symbols carry the `region` prefix (symmetric with `loopStart`/`loopEnd`). New gotcha worth a memory note. This is the sole deviation; O-simpleGrain never hit it because none of its 18 IDs shared a name with a juce free function.

## Intentionally deferred

- **All DSP / audio** → Stage 2 (`processBlock` is silent). Voice/synth/read-head/ADSR/loop/Stretch/Vintage/filter not present yet.
- **`.wav` embedding + sample decode** → Stage 2.3 (no blobs yet; `sourceSample` is a plain choice; second `juce_add_binary_data` target documented as TODO with the dual-NAMESPACE split).
- **WebView UI / waveform editor / relays / drag-drop** → Stage 3 (UI binary-data target + `ouaricon_add_module` documented as TODO; WebView2 flags already set so Stage 3 inherits the cross-platform config).
- **Render-harness** → Stage 2.3 (TODO in CMake).
- **Presets / concept-preset tour** → Stage 3.3 / Stage 4.

## Build / validation evidence

- `ninja O-simpleSampler_{VST3,AU,Standalone}` → clean link of all three artefacts.
- pluginval `--strictness-level 5` on the VST3 → **SUCCESS** (buses: 0 in / 2 out; automatable-parameters PASS).
- `auval -v aumu OsSm OuDv` → **AU VALIDATION SUCCEEDED**; reports **21 Global Scope Parameters** (3 Indexed/Value-String = the choice params).
- Source param consistency: 21 IDs / 21 layout creations / 21 cached atomics.
