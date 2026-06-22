# Stage 1: Foundation — PLAN

**Plugin:** O-simpleAdditive · **Stage:** 1 (Foundation) · **Date:** 2026-06-22
**Goal:** A silent, buildable 16-voice additive synth shell with the full 33-param APVTS,
state persistence, zero latency, and a generic editor placeholder. Port of O-simpleFM minus
oversampling/viz/preset machinery.

---

## Files to create

| File | Purpose |
|------|---------|
| `plugins/O-simpleAdditive/CMakeLists.txt` | Synth + WebView2 CMake target (`PLUGIN_CODE OSiA`). |
| `plugins/O-simpleAdditive/Source/PluginProcessor.h` | `ParamIDs` namespace (33 ids) + processor class. |
| `plugins/O-simpleAdditive/Source/PluginProcessor.cpp` | 33-param `createParameterLayout`, silent `processBlock`, state persistence, `setLatencySamples(0)`. |
| `plugins/O-simpleAdditive/Source/PluginEditor.h` | Thin wrapper around `GenericAudioProcessorEditor`. |
| `plugins/O-simpleAdditive/Source/PluginEditor.cpp` | Placeholder editor (WebView replaces it in Stage 3). |

## Tasks

### T1 — CMakeLists.txt
- Port O-simpleFM CMake. Set `PLUGIN_CODE OSiA`, `PRODUCT_NAME "O-simpleAdditive${OUARICON_DEV_SUFFIX}"`, `VERSION "0.1.0"`.
- `IS_SYNTH TRUE`, `NEEDS_MIDI_INPUT TRUE`, `NEEDS_WEB_BROWSER TRUE`, `NEEDS_WEBVIEW2 TRUE`, `FORMATS VST3 AU Standalone`.
- Compile defs: `JUCE_WEB_BROWSER=1`, `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`, `JUCE_USE_CURL=0`, `JUCE_VST3_CAN_REPLACE_VST2=0`.
- Sources = the 4 Source files only. **No** `juce_add_binary_data`, **no** preset module, **no** tests/ subdir (those are later stages).
- Link the standard JUCE module set (incl. `juce_dsp`, `juce_gui_extra`).
- `juce_generate_juce_header` after `target_link_libraries`.

### T2 — PluginProcessor.h
- `namespace OSimpleAdditive::ParamIDs` with all 33 ids:
  `partial1..partial16`, `frameBSource`, `scanPosition`, `scanLfoRate`, `scanLfoDepth`, `scanEnvAmount`, `spectralDecay`, `bitDepth`, `velToDecay`, `ampAttack/Decay/Sustain/Release`, `modAttack/Decay/Sustain/Release`, `outputLevel`.
- Processor class `OSimpleAdditiveAudioProcessor : juce::AudioProcessor`: standard overrides, `acceptsMidi()=true`, `producesMidi()=false`, `isMidiEffect()=false`, `getTailLengthSeconds()=5.0`.
- Members: `juce::AudioProcessorValueTreeState parameters;`, `static createParameterLayout()`, `juce::SmoothedValue<float> outputGain`, `double currentSampleRate`. Public `getAPVTS()`.
- **No** synth/voices/oversampler/vizRing/midiCollector/presetManager yet.

### T3 — PluginProcessor.cpp — createParameterLayout (33 params)
- Reuse O-simpleFM helpers: `adsrTimeRange()` (0.001–5 s, step 0.0001, skew 0.35), `unitRange()` (0–1, step 0.0001).
- 16 × `partialN`: `unitRange()`; default **partial1 = 1.0**, partial2..16 = **0.0**. Display name `"Partial N Level"`.
- `frameBSource`: `AudioParameterChoice {Sine,Saw,Square,Odd}`, default index 1 (Saw).
- `scanPosition`: `unitRange()`, default 0.
- `scanLfoRate`: `NormalisableRange {0.01, 20.0, 0.0, 0.3}` (log-ish skew), default 0.5 Hz, label "Hz".
- `scanLfoDepth`: `unitRange()`, default 0.
- `scanEnvAmount`: `NormalisableRange {-1.0, 1.0, 0.0001}`, default 0 (bipolar).
- `spectralDecay`: `unitRange()`, default 0.
- `bitDepth`: `AudioParameterChoice {Off,12,10,8,6,4,2}`, default index 0 (Off).
- `velToDecay`: `unitRange()`, default 0.
- Amp ADSR: attack 0.005, decay 0.3, sustain 0.8 (`unitRange`), release 0.1 (times via `adsrTimeRange()`).
- Mod ADSR: attack 0.005, decay 0.3, sustain 0.8, release 0.1.
- `outputLevel`: `NormalisableRange {-60, 0, 0.1}`, default 0, label "dB".
- **Assert/verify total == 33.**

### T4 — PluginProcessor.cpp — lifecycle + silent processBlock
- Ctor: `BusesProperties().withOutput("Output", stereo, true)`, `parameters(*this, nullptr, "PARAMETERS", createParameterLayout())`. No voice allocation.
- `prepareToPlay`: `currentSampleRate = sampleRate`; `outputGain.reset(sampleRate, 0.02)` + seed from `outputLevel`; **`setLatencySamples(0)`**.
- `isBusesLayoutSupported`: mono/stereo out, no input bus (verbatim from O-simpleFM).
- `processBlock`: `ScopedNoDenormals`; `buffer.clear()`; smoothed output-gain ramp (dB→lin); NaN scrub. **No rendering** (silent shell).
- State: `getStateInformation` → `copyXmlToBinary(*apvts.copyState().createXml(), destData)`; `setStateInformation` → `apvts.replaceState(ValueTree::fromXml(*xml))` guarded by tag check.
- `createEditor` → `new OSimpleAdditiveAudioProcessorEditor(*this)`.

### T5 — PluginEditor.{h,cpp}
- Minimal editor owning a `juce::GenericAudioProcessorEditor`; size ~420×640; resizes child to bounds. Comment: "Placeholder — replaced by WebView in Stage 3."

### T6 — SUMMARY.md
- Record files created, the 33-param layout, the O-simpleFM deltas applied, and any deviations.

## Dependencies
T1 ∥ T5 independent · T2 → T3 → T4 (same file group) · T6 last.

## Success criteria (gate to verify phase)
1. Builds clean: `cmake --build build --target O-simpleAdditive_VST3 O-simpleAdditive_AU`.
2. `auval -v aumu OSiA OuDv` passes (or release triple) — loads as instrument.
3. Generic editor lists **exactly 33** params; defaults: partial1=100%, partials2–16=0%, frameBSource=Saw, bitDepth=Off, ampSustain=80%, output=0 dB.
4. State save/reload restores all 33.
5. MIDI input causes no crash; output is silent.
6. Reported latency = 0 samples.

## Out of scope
Additive DSP (Stage 2), WebView/relays/viz/tooltips/presets (Stage 3–4).
