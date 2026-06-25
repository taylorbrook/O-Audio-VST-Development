# Stage 1 (Foundation) — PLAN

**Goal:** A silent, loadable O-simpleBeatmaker shell: VST3 + AU + Standalone, 42 APVTS params via generic editor, custom 6×32 PATTERN ValueTree state persisted alongside APVTS, passes pluginval. No DSP, no WebView.

## Tasks

### T1 — CMakeLists.txt
- Copy the O-simpleSubtractive Foundation CMake; set `PLUGIN_CODE OSiB`, `PRODUCT_NAME "O-simpleBeatmaker${OUARICON_DEV_SUFFIX}"`, `VERSION "1.0.0"`.
- `IS_SYNTH TRUE`, `NEEDS_MIDI_INPUT TRUE`, `FORMATS VST3 AU Standalone`, WebView2 flags + compile defs as in sibling.
- `target_sources`: `PluginProcessor.{h,cpp}`, `PluginEditor.{h,cpp}` only.
- Link the standard JUCE module set; `juce_generate_juce_header`.
- **No** binary-data target, **no** render-harness block (both Stage 2/3).

### T2 — PluginProcessor.h
- `namespace OSimpleBeatmaker::ParamIDs` — 5 timing + 36 voice (6×6) + 1 master = 42 IDs.
- Voice enum/order: Kick, Snare, Clap, ClosedHat, OpenHat, Tom; GM map 36/38/39/42/46/45 (constexpr array, used Stage 2).
- Grid storage: `static constexpr int kNumVoices = 6, kMaxSteps = 32;` flat `std::array<std::atomic<uint8_t>, kNumVoices*kMaxSteps> grid;`.
- Public grid API (message thread): `toggleStep(v,s)`, `setStepVelocity(v,s,vel)`, `int getStep(v,s) const`, `clearGrid()`.
- `getAPVTS()` accessor; standard AudioProcessor overrides; `acceptsMidi()=true`, `getTailLengthSeconds()≈3.0`.

### T3 — PluginProcessor.cpp
- Anonymous-namespace range helpers: `unitRange()` (0–1), `dbRange()` (−60…0), `semitoneRange()` (−12…+12), `tempoRange()` (40–240).
- `createParameterLayout()`: build the 5 timing params explicitly; loop the 6 voices × 6 params via a helper that composes IDs (`kickTune`…`tomSolo`) and labels.
- Constructor: `parameters(*this, nullptr, "PARAMETERS", createParameterLayout())`; zero-init grid atomics; seed nothing yet.
- `prepareToPlay`: store sampleRate; `setLatencySamples(0)`.
- `isBusesLayoutSupported`: output-only mono/stereo, input disabled.
- `processBlock`: `ScopedNoDenormals`; `buffer.clear()`; `ignoreUnused(midi)`.
- `getStateInformation`: `auto state = parameters.copyState();` defensively `removeChild(getChildWithName("PATTERN"))`; append fresh PATTERN child (rows/cols + base64 `cells`); `createXml`→`copyXmlToBinary`.
- `setStateInformation`: parse xml; if tag matches, read PATTERN child into atomics (`clearGrid()` first), `removeChild(PATTERN)`, `replaceState(tree)`.
- Grid API impls (atomic load/store, bounds-checked, velocity clamp 0–127; toggle uses default vel 100).
- `createEditor` → new editor; `createPluginFilter()`.

### T4 — PluginEditor.h / .cpp
- Thin `AudioProcessorEditor` hosting `GenericAudioProcessorEditor genericEditor`; resizable; initial size tall enough for 42 rows (e.g. 680×900, limits 480×360…1200×1400).

## Files
- **Create:** `plugins/O-simpleBeatmaker/CMakeLists.txt`, `Source/PluginProcessor.{h,cpp}`, `Source/PluginEditor.{h,cpp}`.
- Root CMake: none (glob auto-discovers).

## Success criteria
1. Configure + build `O-simpleBeatmaker_VST3`, `_AU`, `_Standalone` with no errors/warnings beyond JUCE's recommended set.
2. `pluginval --strictness-level 8` (or repo default) passes for VST3 **and** AU.
3. `auval -a | grep -i beatmaker` lists the AU (after install) — deferred to install; pluginval is the Stage-1 gate.
4. Host generic editor shows all 42 params with correct ranges/defaults (swing 0, quantize 100%, pattern length 16, tune ±12 st, levels 0 dB).
5. Save/restore round-trips: set a few grid cells + move params, reload state → grid + params restored (verified via a unit-style state round-trip in the harness OR a reasoned code-path audit at Foundation; full DAW round-trip confirmed at install).
6. `processBlock` outputs silence; no crash on MIDI input.

## Risks
- **None novel.** PATTERN persistence is the only non-boilerplate piece; mitigated by symmetric `MemoryBlock` base64 + remove-before-replace to avoid child duplication. Verified by code-path audit (no harness until Stage 2).
