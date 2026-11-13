# O-ParametricEQ Stage 1 (Foundation): Research

**Researched:** 2026-11-12
**Domain:** JUCE 8.0.15 effect shell. 196-parameter APVTS in 25 parameter groups, host bypass parameter, mono / stereo symmetric layouts, pass-through audio, the v1 state format, and the layout / state gate G11.
**Confidence:** HIGH. Each claim is tagged **VERIFIED** (read in the repo or in `~/JUCE`, or run, this session) or **INFERRED**.
**Inputs read:** `stages/1-foundation/CONTEXT.md` (D1–D5), `parameter-spec.md`, `ROADMAP.md` § Stage 1, `research/ARCHITECTURE.md` (State Persistence, Parameter Mapping, Verification Gates, references), `mockups/v1-CMakeLists-SNIPPET.txt`, `v1-PluginEditor-TEMPLATE.cpp` (assumed processor API), `v1-integration-checklist.md` § 2, `plugins/O-AnalogEQ/{CMakeLists.txt,Source/PluginProcessor.*}`, `scripts/param-dump/*`, `modules/cmake/OuariconModules.cmake`, `modules/registry.yaml`, `scripts/resolve-target.sh`, `scripts/build-and-install.sh`, root `CMakeLists.txt`, `.github/workflows/ci-tests.yml`, the O-GranulatorInstrument Stage 1 RESEARCH and CMake test block, and the memory notes cited inline.

## User constraints (CONTEXT.md, binding; not reopened)

- **D1.** IDs `b01_…b24_`, frozen from this stage. Host names unpadded.
- **D2.** 196 host parameters. Analyzer mode is UI state.
- **D3.** Stage 1 encodes no answer to the six deferred DSP calls. `bNN_slope` is registered on every band.
- **D4.** `createEditor()` returns `juce::GenericAudioProcessorEditor`. WebView plumbing is wired in CMake. Nothing is copied into `Source/ui/`.
- **D5.** Target `OuariconParametricEQ`, `PLUGIN_CODE OuPQ`, `VERSION 0.1.0`, `FORMATS VST3 AU Standalone`, effect, no MIDI.
- **Gate:** VST3 + AU + Standalone build; `auval` and pluginval 10 pass; G11 (golden layout, exact state round trip, three foreign-state cases); `(2,1)` and `(1,2)` refused; no parameter written from a listener; reserved IDs unregistered and listed in a comment.

## Summary

1. **The suite `param-dump` tool exists but cannot produce the G11 golden.** `scripts/param-dump/main.cpp` prints `id, name, label, numSteps, textAtMin, textAtMax, defaultNorm, defaultText, flags`. It has no index, version hint, range, or skew column. **VERIFIED.** G11 needs a plugin-local console built through `ouaricon_add_processor_console` (same file, `ParamDump.cmake:67`). Recommended: start `tests/render-harness/main.cpp` now with the layout, state, bus and pass-through rows; Stage 2 extends it. Wire the suite `param-dump` as well (three lines; later UI gates use it).
2. **A missing parameter does not load at its default in JUCE. It keeps its current value.** `updateParameterConnectionsToChildTrees` appends a fresh `PARAM` child for any adapter with no child and then flushes the current value into it (`juce_AudioProcessorValueTreeState.cpp:417-440`). **VERIFIED.** CONTEXT requires "missing parameter defaults", so `setStateInformation` must add default-valued `PARAM` children to the incoming tree before `replaceState`. That is a tree edit and writes no parameter.
3. **The skewed defaults are not exact in engineering units.** With `setSkewForCentre`, the normalised default is exactly 0.5 for both `freq` and `q`, but APVTS seeds the raw value through `convertFrom0to1`, which gives **999.999878 Hz** and **0.999999762**. Reproduced in float with the JUCE formulas (`juce_NormalisableRange.h:136-173`, `:210-218`; APVTS adapter ctor `:80-82`). **VERIFIED** (C reproduction, same libm; the harness must confirm on the real processor). The linear ranges are exact (0 dB, 100 %). This does not block Stage 1. It is an input to Stage 2 discuss item 5 (Q 1.0 = Butterworth).
4. **Unknown children survive in the live tree and are re-saved.** `replaceState` is `state = newState` (`:396-404`). **VERIFIED.** For unknown `PARAM` children that is useful: a later version's session opened and saved in v1 keeps its dynamic-EQ values. For the `UI` child it is the two-reopen trap (`critical_preset_manager_stale_customstate_child`): strip `UI` from the incoming tree before `replaceState` and build a fresh one on a `copyState()` copy at save.
5. **With a bypass parameter, the wrappers never call `processBlockBypassed`.** VST3 calls it only when `getBypassParameter() == nullptr` (`juce_audio_plugin_client_VST3.cpp:3774-3777`), AU likewise (`juce_audio_plugin_client_AU_1.mm:2194-2200`). The VST3 wrapper adopts the plugin's parameter as the VST3 bypass and adds no private one (`:673-689`). **VERIFIED.** Stage 1 passes audio through in both; Stage 2 must implement the bypass crossfade inside `processBlock`.
6. **CMake lands on the O-AnalogEQ pattern with the UI binary-data block deferred.** `OuPQ` is unused in every `plugins/*/CMakeLists.txt` and elsewhere in the repo. All three module paths exist at the pinned versions. `vu-meter` is header-only (`VUMeterBridge.h`, includes `juce_core` and `juce_audio_basics` only). **VERIFIED.**
7. **No hash collisions.** JUCE's `String::hashCode` over the 196 IDs gives 196 distinct values (VST3 ParamIDs and AU parameter IDs), and over the 25 group IDs gives 25 distinct non-zero unit IDs. **VERIFIED** (computed from the spec's registration table).
8. **The shared build needs an explicit re-configure** before ninja knows the new folder: the root globs `plugins/*` without `CONFIGURE_DEPENDS` (`CMakeLists.txt:47-58`) and `build-and-install.sh` skips configure when `build/` exists (`:262-271`). `OUARICON_BUILD_TESTS` is already `ON` in `build/CMakeCache.txt`. **VERIFIED.**

---

## Q1. CMake

Base: `plugins/O-AnalogEQ/CMakeLists.txt`. **VERIFIED** (read).

```cmake
cmake_minimum_required(VERSION 3.15)
include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)

set(OPEQ_VERSION 0.1.0)   # single source; the consoles derive JucePlugin_VersionString from the target

juce_add_plugin(OuariconParametricEQ
    COMPANY_NAME "${OUARICON_COMPANY_NAME}"
    PLUGIN_MANUFACTURER_CODE ${OUARICON_MANUFACTURER_CODE}
    PLUGIN_CODE OuPQ
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "O-ParametricEQ${OUARICON_DEV_SUFFIX}"
    VERSION ${OPEQ_VERSION}
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    NEEDS_WEB_BROWSER TRUE
    NEEDS_WEBVIEW2 TRUE
)
```

- **Sources:** `Source/PluginProcessor.cpp` only. No `PluginEditor.cpp` in Stage 1 (D4). `ParamDump.cmake` requires a `PluginProcessor.cpp` in the target's `SOURCES` and skips any `PluginEditor*` TU (`:156-171`). **VERIFIED.**
- **Link list:** O-AnalogEQ's, including `juce_dsp` (Stage 2 FFT) and `juce_gui_extra`. `juce_generate_juce_header (OuariconParametricEQ)` is required: the param-dump driver includes `JuceHeader.h`. **VERIFIED.**
- **Modules:** `ouaricon_add_module (OuariconParametricEQ preset-manager)` and `… vu-meter`. Each adds the module's `cpp/*.h` to the target and its include path, and copies `js/*.js` into `Source/ui/public/modules/` at configure (`OuariconModules.cmake:59-116`). That directory is gitignored (`.gitignore:27`). `eb-garamond` has no helper; it is a direct embed in Stage 3. **VERIFIED.**
- **The configure-time JS copy does not bring the plugin into `check-i18n`.** Discovery keys on `Source/ui/public/js/i18n.js`, which Stage 1 does not create (`pattern_check_i18n_rule8_fails_stage1_placeholder_editor`; sibling result "0 plugins localized, rc 0"). **VERIFIED** (memory + sibling research). ROADMAP's "placeholder editor must carry the resource map" is superseded by D4.
- **Defer** the `juce_add_binary_data (… UIResources …)` block and its link line to Phase 3.1. Its `SOURCES` do not exist yet. Keep the snippet in `mockups/`.
- **Definitions:** `JUCE_VST3_CAN_REPLACE_VST2=0`, `JUCE_WEB_BROWSER=1`, `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`, `JUCE_USE_CURL=0`.
- **Comment rule:** `resolve-target.sh:42-49` matches the plugin-declaration command name followed by `(` anywhere, including comments, and takes the first hit. Gate: `grep -c "juce_add_plugin *(" plugins/O-ParametricEQ/CMakeLists.txt` equals 1. **VERIFIED.**
- **Test block** (off by default, `OUARICON_BUILD_TESTS`):
  ```cmake
  option(OUARICON_BUILD_TESTS "Build O-ParametricEQ test targets" OFF)
  if(OUARICON_BUILD_TESTS)
      include(${CMAKE_SOURCE_DIR}/scripts/param-dump/ParamDump.cmake)
      ouaricon_add_param_dump(OuariconParametricEQ ${CMAKE_CURRENT_SOURCE_DIR}/Source)
      ouaricon_add_processor_console(OuariconParametricEQ ${CMAKE_CURRENT_SOURCE_DIR}/Source
          ${CMAKE_CURRENT_SOURCE_DIR}/tests/render-harness/main.cpp render-harness)
      target_compile_definitions(O-ParametricEQ-render-harness PRIVATE
          GOLDEN_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/golden")
  endif()
  ```
  Console targets are named after the folder: `O-ParametricEQ-param-dump`, `O-ParametricEQ-render-harness`. They build with `JUCE_WEB_BROWSER=0` and do not inherit the plugin's C++ standard; if the processor uses C++20, add `target_compile_features (… cxx_std_20)` to the plugin and to each console (O-Granulator precedent). The root sets no `CMAKE_CXX_STANDARD`. **VERIFIED.**
- **CI:** `ci-tests.yml` builds test targets only for plugins it names (`if: inputs.plugin == '…'`). Adding the option here creates no CI obligation. **VERIFIED.**

## Q2. Parameter layout

- **Precedent:** O-GrainScatter builds `std::make_unique<juce::AudioProcessorParameterGroup> ("global", "Global", "|")` and per-band groups (`Source/PluginProcessor.cpp:41`, `:126`); O-FreqPulse, O-Octagon and O-Orbit also use groups. `APVTS::ParameterLayout` accepts groups directly (`juce_AudioProcessorValueTreeState.h:124-230`). **VERIFIED.**
- **Shape:** one table-driven builder. Group IDs `global`, `band01` … `band24`; group names `Global`, `Band 1` … `Band 24`. IDs built as `"b" + String (n).paddedLeft ('0', 2) + "_freq"`; names as `"B" + String (n) + " Freq"`.
- **Types:** `AudioParameterBool` ×50, `AudioParameterChoice` ×72 (8 / 8 / 5 choices), `AudioParameterFloat` ×74. Every one takes `juce::ParameterID { id, 1 }`. `getVersionHint()` is public (`juce_AudioProcessorParameter.h:293`), so the harness can assert it. **VERIFIED.**
- **Ranges:** `NormalisableRange<float> (10.0f, 30000.0f, 0.0f)` + `setSkewForCentre (1000.0f)`; `(0.025f, 40.0f, 0.0f)` + `setSkewForCentre (1.0f)`; linear `(-36, 36, 0)`, `(0, 200, 0)`, `(-30, 30, 0)`. Float reproduction of the JUCE math:

  | Range | skew (float) | default norm | raw default after APVTS seed | norm 0.25 / 0.75 |
  |---|---|---|---|---|
  | freq | 0.203214481 | 0.5 exactly | 999.999878 | 42.6809 Hz / 7290.55 Hz |
  | q | 0.186652407 | 0.5 exactly | 0.999999762 | 0.0487805 / 8.58399 |
  | output_gain, band gain, gain_scale | 1 | 0.5 exactly | 0, 0, 100 exactly | linear |

  Both skews match the spec to six decimals. The range ends are exact (`proportion == 0` skips the exp/log branch; `exp (log (1) / skew) == 1`). **VERIFIED** (reproduction).
- **Choice strings** are host-visible and stay English (ARCHITECTURE notes). Slope strings carry the unit: `"6 dB/oct"` … `"96 dB/oct"`.
- **AU order** is version hint, then ID hash (`juce_audio_plugin_client_AU_1.mm:2347-2350`), so it differs from the VST3 order and cannot be chosen. Groups become AU clumps (`:1086-1087`) and VST3 units (`juce_audio_plugin_client_VST3.cpp:546-560`). **VERIFIED.**
- **Reserved IDs:** a source comment next to the layout lists `bNN_dyn_range`, `bNN_dyn_threshold`, `bNN_dyn_auto` (NN 01–24, version hint 2). Harness row: `parameters.getParameter (id) == nullptr` for all 72.
- **Raw pointers:** resolve `getRawParameterValue` once in the constructor into arrays (O-AnalogEQ IN-07). Stage 1 needs none of them for audio; adding the cache now is optional.
- **No listeners in Stage 1.** No `parameterChanged`, no `setValueNotifyingHost` anywhere in `Source/`. Grep gate: both counts 0.

## Q3. Processor

- **Buses:** `BusesProperties().withInput ("Input", stereo, true).withOutput ("Output", stereo, true)`.
- **Layout predicate** (O-AnalogEQ `PluginProcessor.cpp:344-352`, **VERIFIED**):
  ```cpp
  const auto& out = layouts.getMainOutputChannelSet();
  if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo()) return false;
  return layouts.getMainInputChannelSet() == out;
  ```
  This refuses `(2,1)`, `(1,2)`, disabled buses and anything wider. Expected AU channel capabilities `[1,1] [2,2]`. **INFERRED**; record the real value at the first `auval` and treat it as frozen, because Logic caches AU I/O per version (`critical_logic_caches_au_io_config_per_version`).
- **Bypass:** `juce::AudioProcessorParameter* getBypassParameter() const override { return bypassParam; }` with the pointer taken from `parameters.getParameter ("bypass")` in the constructor. Override `processBlockBypassed` to forward to the same path, for hosts that call it directly (JUCE's own wrappers will not; Summary 5).
- **`prepareToPlay`:** `setLatencySamples (0)`. **`processBlock`:** `juce::ScopedNoDenormals`, clear any output channel beyond the input count, touch nothing else. Do not add a meter read that could alter samples; the pass-through row is a `memcmp`.
- **`createEditor`:** `return new juce::GenericAudioProcessorEditor (*this);` unconditionally in Stage 1. Phase 3.1 introduces the `#if JUCE_WEB_BROWSER` guard with the editor include inside it (O-AnalogEQ `:568-581`), so the consoles keep linking.
- **Members named for the Stage 3 template** (`v1-PluginEditor-TEMPLATE.cpp:28-40`): public `parameters`, `uiLanguage` + `languageCode` / `languageIndex`, `analyzerMode { 2 }`, `uiDisplayRange = 12`, `uiSelectedBand = 0`, `uiEditorWidth = 960`, `uiEditorHeight = 600`. `listenBand`, `editorOpen`, `displayModel`, `presetManager` and `outputLevelDB` belong to later stages.
- **Do not construct `OuariconPresetManager` in Stage 1.** Presets are Stage 4, and a processor that owns one makes every console write under the real `~/Library` unless the harness re-execs with `CFFIXED_USER_HOME` (`pattern_juce_mac_home_ignores_HOME_use_cffixed_user_home`). The module line in CMake is enough for now. **INFERRED** (recommendation).
- **`vu-meter` compile proof:** `#include "VUMeterBridge.h"` in `PluginProcessor.h` (optionally an unused `VUMeterBridge` member). The registry lists no consumer, so this is the first time the header is compiled in a plugin. It answers CONTEXT's open item at near-zero cost. **INFERRED** that it compiles; it uses only `juce::Decibels`, `juce::String`, `juce::AudioBuffer`, `std::atomic`.
- `juce::dsp::IIR::Filter` is not used. Grep gate: `grep -rn "IIR::Filter" plugins/O-ParametricEQ/Source` returns 0.

## Q4. State

**Format:**
```xml
<OParametricEQ stateVersion="1" pluginVersion="0.1.0" uiLanguage="en">
  <PARAM id="bypass" value="0.0"/>  … 196 …
  <UI analyzerMode="2" displayRange="12" selectedBand="0" editorWidth="960" editorHeight="600"/>
</OParametricEQ>
```
The `UI` child carries no `id` attribute: APVTS looks up every top-level child's `id` as a parameter ID (`setNewState`, `:406-415`). **VERIFIED.**

**Save (`getStateInformation`):**
1. `auto state = parameters.copyState();` Never mutate the live tree.
2. Remove every child named `UI` from the copy (there should be none; this makes the save idempotent).
3. Set root properties `stateVersion = 1`, `pluginVersion = JucePlugin_VersionString`, `uiLanguage = languageCode (…)`.
4. Append one fresh `UI` child from the members.
5. `copyXmlToBinary (*state.createXml(), destData)`.

**Restore (`setStateInformation`):**
1. `getXmlFromBinary`; return if null or `! xml->hasTagName (parameters.state.getType())`.
2. `auto tree = juce::ValueTree::fromXml (*xml)`.
3. Read `UI` and `uiLanguage` from `tree` with `isVoid()` guards. Every value comes back as a string var (`critical_valuetree_xml_roundtrip_loses_type`): parse with `(int) v`, then validate (`analyzerMode` 0–3, `displayRange` in {6, 12, 30}, `selectedBand` 0–24, size `jlimit` to 800–1600 × 500–1000, language through `languageIndex`). An absent property leaves the member unchanged.
4. Remove every `UI` child from `tree`.
5. For each registered parameter with no `PARAM` child in `tree`, append `PARAM id=… value=<denormalised default>` (Summary 2).
6. `parameters.replaceState (tree)`.

- `stateVersion` is read but not branched on. `"2"` loads; unknown `PARAM` children are ignored by the adapters and stay in the tree. The next save writes `stateVersion="1"` again with those children still present. **INFERRED** (design); the planner may prefer to preserve the higher version number instead, see Open Q3.
- Parameter values need no type handling of our own: `setDenormalisedValue (tree.getProperty (value, default))` converts a string var to float (`:413`, `:100-104`). **VERIFIED.**
- A raw value restored from XML re-normalises and may land one ULP away for skewed ranges. Gate the round trip on `getValue()` (normalised) bit-equality across all 196 after `A → B`, and on byte-equal XML for `A → B → save` versus `A → save`.
- `setStateInformation` can arrive off the message thread. Make the UI-state members `std::atomic<int>` (the template reads two of them as plain ints; Stage 3 adds `.load()`). **INFERRED.**

## Q5. Gate G11 and the harness

**Console:** `tests/render-harness/main.cpp`, `createPluginFilter()` only, no plugin class name needed except for the public members it reads. Rows (exit 0 only if all pass):

| Row | Check |
|---|---|
| L1 | 196 parameters; `--dump-params` output equals `tests/golden/params.tsv` byte for byte |
| L2 | every version hint is 1; every parameter automatable; types 50 / 72 / 74 |
| L3 | none of the 72 reserved IDs is registered |
| L4 | `getBypassParameter()` is the parameter with ID `bypass`; latency 0 after `prepareToPlay` |
| L5 | raw defaults: linear ranges exact; `freq` and `q` within 2e-4 relative of 1000 and 1.0; normalised defaults exactly 0.5 |
| B1 | accepted: (1,1), (2,2). Refused: (2,1), (1,2), (0,2), (2,0), 5.1 |
| P1 | pass-through: seeded noise, mono and stereo, block sizes 1, 64, 512, 4096; output `memcmp`-equal to input. Vacuity guard: the comparator fails on a buffer with one flipped sample |
| S1 | round trip: every parameter set to a seeded random normalised value through the host path, UI state non-default, language `zh-Hans`; fresh instance restores all 196 `getValue()` bit-equal and every UI field equal |
| S2 | XML shape: root attributes present, exactly 196 `PARAM`, exactly one `UI`, no `id` on `UI` |
| S3 | two reopens with a UI change between them: the second reopen shows the changed value and still exactly one `UI` child |
| S4 | unknown parameter (`b01_dyn_range`): loads, known values applied |
| S5 | missing parameter: target instance has `b07_gain` set non-default first; after restore it equals the default (without the pre-set this row is vacuous) |
| S6 | `stateVersion="2"`: loads, values applied |
| S7 | wrong root tag, truncated blob, empty blob: no crash, state unchanged |

**Golden columns:** `index, id, name, group, type, versionHint, start, end, interval, skew, defaultNorm, defaultRaw, numSteps, choices, automatable`. Floats printed with `%.9g`. Write with explicit `"\n"` (`File::replaceWithText` defaults to CRLF, `pattern_juce_replacewithtext_defaults_crlf`) or print to stdout and redirect.

**Keep the golden from certifying itself.** The golden is the committed dump; a second check ties it to the contract: a small script (`tests/golden/check_spec.py`) parses the "Full Registration Table (196)" in `parameter-spec.md` and asserts index, ID, name, type and group for every row, plus ranges, defaults and skew (tolerance 1e-5) per parameter kind. The table parses cleanly: 196 rows, 196 unique IDs. **VERIFIED** (parsed this session). Negative control: change one ID in a scratch copy of the golden and confirm both L1 and the script fail.

## Q6. Gate commands (repo root; BG = `run_in_background`, log to the scratchpad)

| # | Command | BG |
|---|---|---|
| 0 | `shasum -a 256 plugins/O-ParametricEQ/.planning/parameter-spec.md` equals STATUS; `grep -h PLUGIN_CODE plugins/*/CMakeLists.txt \| grep -c OuPQ` equals 0 before landing | no |
| 1 | `cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release` | **BG** |
| 2 | `ninja -C build OuariconParametricEQ_VST3 OuariconParametricEQ_AU OuariconParametricEQ_Standalone O-ParametricEQ-render-harness O-ParametricEQ-param-dump` | **BG** |
| 3 | `find build/plugins/O-ParametricEQ -name 'O-ParametricEQ-render-harness' -perm +111` then run it (exit 0); `--dump-params \| diff - plugins/O-ParametricEQ/tests/golden/params.tsv` | no |
| 4 | `python3 plugins/O-ParametricEQ/tests/golden/check_spec.py` | no |
| 5 | `./scripts/build-and-install.sh O-ParametricEQ` (VST3 + AU; cache clear and dual-variant sweep) | **BG** |
| 6 | `/Applications/pluginval.app/Contents/MacOS/pluginval --strictness-level 10 --timeout-ms 600000 --validate ~/Library/Audio/Plug-Ins/VST3/O-ParametricEQ-dev.vst3` | **BG** |
| 7 | same with `--validate-in-process ~/Library/Audio/Plug-Ins/Components/O-ParametricEQ-dev.component` | **BG** |
| 8 | `auval -v aufx OuPQ OuDv` plus negative control `auval -v aufx ZZZZ OuDv` (rc ≠ 0). Never `auval -a` (it aborts on this machine). The first run after install is a cold rescan | **BG** |
| 9 | Standalone smoke: launch, wait 6 s, `kill -0`, kill | **BG** |
| 10 | `node scripts/check-i18n.js` → plugin count unchanged | no |
| 11 | Grep gates: `setValueNotifyingHost` 0, `parameterChanged` 0, `IIR::Filter` 0 in `Source/`; the plugin-declaration command appears once in `CMakeLists.txt` | no |

## Pitfalls

| Pitfall | Source | Handling |
|---|---|---|
| Missing parameter keeps the old value | JUCE `:417-440` | Fill defaults into the incoming tree (Q4 step 5); S5 with a pre-set |
| Stale `UI` child shadows the second reopen | `critical_preset_manager_stale_customstate_child` | Strip on load and on the save copy; S3 |
| Non-parameter values return as strings | `critical_valuetree_xml_roundtrip_loses_type` | `isVoid()` guards, `(int) v`, validate |
| Raw 1 kHz / Q 1 defaults are inexact | Summary 3 | L5 tolerance on raw, exact on normalised; raise in Stage 2 discuss |
| Target resolver reads comments | `pattern_resolve_target_matches_juce_add_plugin_in_comment` | Gate 11 |
| New folder invisible to ninja | Summary 8 | Gate 1 before any build |
| Golden written with CRLF | `pattern_juce_replacewithtext_defaults_crlf` | stdout or explicit `"\n"` |
| `-dev` and unsuffixed bundles share the AU triple | CLAUDE.md | use `build-and-install.sh` |
| Long commands hit the 600 s watchdog | `pattern_executor_watchdog_stall_run_long_commands_in_background` | BG column |
| `modules/registry.yaml` has another session's uncommitted edits (5 lines) | `git status`, this session | add `used_by` entries with a hunk-scoped / temp-index commit, or defer them to Stage 3 when the modules are first used |
| Foreign working-tree changes | `git status`, this session | path-scope every commit to `plugins/O-ParametricEQ PLUGINS.md`; re-check branch and status right before |

## Recommended task skeleton (for PLAN, not the plan)

1. `CMakeLists.txt` (Q1), re-configure, confirm the three module lines print and the target resolves.
2. `Source/PluginProcessor.{h,cpp}`: layout builder, reserved-ID comment, buses, bypass, pass-through, Generic editor.
3. State codec (Q4).
4. `tests/render-harness/main.cpp` rows L, B, P, S; generate and commit `tests/golden/params.tsv`; `check_spec.py`; negative controls.
5. Build three formats, install, pluginval ×2, auval, Standalone smoke, grep gates.
6. `CHANGELOG.md` `## [0.1.0]` (bracket form), STATUS, PLUGINS.md.

## Open questions (none blocks planning)

1. **Where `uiLanguage` lives.** CONTEXT lists it under the `UI` child. `parameter-spec.md` ("`uiLanguage` property, suite convention") and the integration checklist (§ 2, line 85) put it on the state root, as O-AnalogEQ does. Recommendation: the root property, following the spec. S1 covers either.
2. **Invalid state.** ARCHITECTURE says "Invalid XML → defaults, no throw". Suite precedent leaves the current state unchanged. Recommendation: unchanged (a host handing over garbage should not wipe a session); S7 is written for that. Say so in SUMMARY if the planner keeps it.
3. **`stateVersion` on re-save of a newer session.** Always write `1`, or keep the higher number so the newer version can tell its data passed through v1. Recommendation: always `1`; the retained unknown children carry the data either way.
4. **Raw default inexactness** (999.99988 Hz, 0.99999976). No Stage 1 action. Carry to Stage 2 discuss with item 5.
5. **Registry `used_by`.** Add O-ParametricEQ under `preset-manager`, `vu-meter`, `eb-garamond` now or at first real use. `eb-garamond`'s list is already known to under-report (`project_eb_garamond_registry_used_by_drift`).

## Sources

- JUCE 8.0.15 at `~/JUCE/modules`: `juce_audio_processors/utilities/juce_AudioProcessorValueTreeState.{h,cpp}`, `juce_core/maths/juce_NormalisableRange.h`, `juce_audio_processors_headless/processors/juce_AudioProcessorParameter.h`, `…/utilities/juce_RangedAudioParameter.cpp`, `…/utilities/juce_AudioParameterFloat.cpp`, `juce_audio_plugin_client/juce_audio_plugin_client_VST3.cpp`, `…_AU_1.mm`.
- Repo: files listed under "Inputs read".
- Float reproduction of the skew math: a 20-line C program run this session (not kept).
