# Stage 1 (Foundation) — PLAN

**Plugin:** O-simpleWavetable · **Stage:** 1 of 4 (Foundation / Shell) · **Date:** 2026-10-05
**Inputs:** `CONTEXT.md` (authoritative, user decisions), `RESEARCH.md` (authoritative on how), `parameter-spec.md` (LOCKED, 21 params), `research/ARCHITECTURE.md`, `ROADMAP.md` §Stage 1, `BRIEF.md`, `mockups/v1-CMakeLists.txt`, `mockups/v1-integration-checklist.md`, sibling `plugins/O-simpleAdditive` (Stage 1 commit `88073650` + current source).
**Precedence when sources disagree:** CONTEXT > RESEARCH > parameter-spec > ARCHITECTURE > ROADMAP > agent-definition template defaults.

---

## Contract discrepancies (found while planning; resolution applied in this plan)

| # | Where | Conflict | Resolution |
|---|-------|----------|------------|
| C1 | ROADMAP §Stage 1 + §3.1, ARCHITECTURE §Parameter Mapping | Say "22 parameters" / `jassert(params.size() == 22)` but list 21 IDs | **21** (parameter-spec.md:13, CONTEXT). `jassert (params.size() == 21)`. |
| C2 | ROADMAP §Stage 1: "getState/setState with an empty `IMPORTED_BANK` round-trip stub" | Readable as "write an empty child" | **CONTEXT wins:** the child is written only when a bank exists, so Stage 1 writes **no** child; restore tolerates absent, present, and duplicated children. |
| C3 | ARCHITECTURE §State Persistence (restore = `replaceState`, then read the child, which stays in the live tree as in O-simpleGrain) vs RESEARCH §5.2/§5.3 | Different mechanism | **RESEARCH wins:** copy the child out, strip **every** `IMPORTED_BANK` from the incoming tree **before** `replaceState`, and strip again from `copyState()` in `getStateInformation` (stale-child trap). |
| C4 | ARCHITECTURE writes `Sine→Saw` / `Sine→Square` (no spaces) | parameter-spec.md and mockup tooltips use `Sine → Saw` / `Sine → Square` | **parameter-spec wins:** spaced, U+2192 built with `juce::CharPointer_UTF8 ("Sine \xE2\x86\x92 Saw")`. |
| C5 | `.claude/agents/foundation-shell-agent.md` template defaults | It hardcodes company and manufacturer from `branding.json`, uses `Ou`+2-char codes, version 1.0.0, state type `"Parameters"`, a top-of-file `#include "PluginEditor.h"`, and has the agent update STATUS.md + PLUGINS.md | **CONTEXT/RESEARCH win:** `OUARICON_*` CMake vars, `PLUGIN_CODE OSiW`, `VERSION "0.1.0"`, `"PARAMETERS"`, guarded editor include, and state files are orchestrator-only (Task 15). The overrides are restated in each task. |
| C6 | Project rule "commit with `git commit -- plugins/O-simpleWavetable PLUGINS.md`" vs live repo state | `PLUGINS.md` is **foreign-staged (`MM`)** and its working tree holds other sessions' uncommitted rows. A pathspec commit would commit those foreign rows, and it skips the 7 untracked new files (pathspec commit = tracked only). | Same path scope, done as a **temp-index commit**: `PLUGINS.md` blob = `HEAD` + this plugin's row only (Task 15, memory `index_git_shared_checkout`). |
| C7 | CLAUDE.md "verify AU with `auval -a \| grep`" | `auval -a` SIGABRTs on this machine (NI AU + TCC) | Targeted `auval -v aumu OSiW OuDv` via `scripts/verify-au-link.sh` (CONTEXT, RESEARCH §7.1). |
| C8 | ARCHITECTURE §Design Sync Check: "No mockup exists" | Stale: mockup v1 is finalized | Informational. Mockup v1 is the Stage 3 source; Stage 1 matches its class names and processor API surface. |

**Planner choices (not contradictions):**
- **P-A:** The probe targets (Task 12) build in **Debug**, not Release as RESEARCH §7.2 has it. In Debug every `jassert` is compiled in and logs `JUCE Assertion failure in <file>:<line>` to stderr (`juce_PlatformDefs.h:66-67,167`, `juce_Logger.cpp:61-70`). That makes `jassert (params.size() == 21)`, JUCE's Choice ≥ 2 assert, the MidiMessageCollector asserts and the leak detector **runtime-checkable**. In a Release build all of them compile out.
- **P-B:** RESEARCH assumption A2 is now resolved: `ValueTree::createCopy()` on an invalid tree returns an invalid tree `[VERIFIED: juce_ValueTree.cpp:667-673]`. No extra guard is needed. A harmless `isValid()` check is allowed.
- **P-C:** `tests/state-check` lands in Stage 1, as RESEARCH §14 recommends. It is the only way to prove the `uiLanguage` restore and the `IMPORTED_BANK` two-reopen behaviour before Stage 2.4 depends on them.

---

## Goal

A buildable, host-loadable **silent synth shell**, structured like O-simpleAdditive:
- synth CMake (MIDI in, stereo out, WebView2 flags already set for Stage 3);
- the complete **21-parameter** APVTS in parameter-spec order;
- a `juce::Synthesiser` with 16 silent `WtVoice`s and a `MidiMessageCollector`;
- zero latency;
- APVTS-XML state with the `uiLanguage` property and a strip-on-load `IMPORTED_BANK` stub;
- a `GenericAudioProcessorEditor` shell under the final editor class name.

It must pass VST3 + AU + Standalone builds, targeted auval and pluginval. **No audio until Stage 2. No WebView UI until Stage 3.**

---

## Execution model

| Who | Tools | Tasks |
|-----|-------|-------|
| **foundation-shell-agent** (one dispatch) | Read / Write / Edit only. No Bash, no builds. | 2–9: writes all 7 source/test files and returns the JSON report. |
| **Orchestrator** (plugin-workflow execute phase) | Bash. Every long command runs with `run_in_background: true` and logs to `$SCRATCH` (the execute session's scratchpad, never `/tmp` or the repo). Memory `pattern_executor_watchdog_stall_run_long_commands_in_background`. | 1, 10–13, 15 |
| **Taylor** (human checkpoint) | DAW / Standalone | 14 |

**Agent overrides (apply to every agent task; they supersede the agent template):**
- **Contracts to read:** `CONTEXT.md`, `RESEARCH.md` §3.2 / §4 / §5 / §6 / §7.2, `parameter-spec.md`, `troubleshooting/patterns/stage-1-patterns.md`. Where RESEARCH gives ready code, **transcribe it** with the amendments listed in each task rather than re-deriving it from ARCHITECTURE.
- **Branding:** company and manufacturer come from the CMake variables `${OUARICON_COMPANY_NAME}` and `${OUARICON_MANUFACTURER_CODE}`, **never** literals from `branding.json`. A file-header comment may name "Ouaricon Audio" and "Developer: Taylor Brook".
- **ASCII-only sources, comments included.** RESEARCH's snippets contain non-ASCII characters in comments (→, —, …, ±). Replace them while transcribing (`->`, `-`, `...`, `+/-`). The only non-ASCII *content* is U+2192, and it lives in hex escapes.
- **Do NOT** run the template's "State Management" steps: no STATUS.md edit, no PLUGINS.md edit. Report `"stateUpdated": false, "stateUpdateNote": "deferred to orchestrator per PLAN.md Task 15"`.
- **Do NOT** copy `mockups/v1-*` into `Source/` (Stage 3). Do not add `juce_add_binary_data`, presets, CHANGELOG/NOTES, `Source/ui/public/` files, or any `.gitignore`.
- The JSON report must carry `parameter_count: 21` and `parameters_implemented` = the 21 IDs in spec order.

---

## Tasks

### Task 1 — Pre-flight + 0→1 gate bypass  *(orchestrator)*
- **Files:** none created by hand. The gate script writes `plugins/O-simpleWavetable/.planning/gate-bypasses.log` and `plugins/O-simpleWavetable/.planning/stages/0-ideation/gate-report.json`.
- **Do:**
  1. Run `git branch --show-current` (must be `main`).
  2. Confirm `plugins/O-simpleWavetable/Source` and `plugins/O-simpleWavetable/CMakeLists.txt` do **not** exist, so nothing is overwritten.
  3. Confirm `build/build.ninja` exists.
  4. Run `grep -c O-simpleWavetable build/build.ninja`. It should be 0 now; Task 11 fixes that.
  5. Contract tamper check: `shasum -a 256` of `BRIEF.md`, `parameter-spec.md`, `research/ARCHITECTURE.md` and `ROADMAP.md` must equal the `contract_checksums` in `STATUS.md`. On a mismatch, stop and report; do not proceed on a drifted contract.
  6. Gate: the 0→1 gate always BLOCKS because no CMakeLists exists yet (memory `pattern_gate_0_to_1_always_needs_force`). Run:
     `echo "0->1 build check cannot pass by construction: plugins/O-simpleWavetable/CMakeLists.txt does not exist until Stage 1 creates it" | .planning/workflow/scripts/run-gate.sh O-simpleWavetable 0-ideation 1-foundation --force --skip-review`
     The justification goes in on stdin because the script reads it with `read -p`.
- **Depends on:** none.
- **Verify:** gate exit code **2** (BYPASSED), and `tail -5 plugins/O-simpleWavetable/.planning/gate-bypasses.log` shows the 0→1 entry. Checksums match.
- **Done:** on `main`, the target paths are empty, the contracts are unchanged, and the bypass is logged. Exit 1 here means the justification was not read: re-run it. Do not "fix" the build check.

### Task 2 — `CMakeLists.txt`  *(agent)*
- **Files:** `plugins/O-simpleWavetable/CMakeLists.txt`
- **Do:** transcribe RESEARCH §3.2 **verbatim**.
  - `include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)` first.
  - `juce_add_plugin(O-simpleWavetable …)` with:
    - `COMPANY_NAME "${OUARICON_COMPANY_NAME}"`, `PLUGIN_MANUFACTURER_CODE ${OUARICON_MANUFACTURER_CODE}`, `PLUGIN_CODE OSiW`
    - `FORMATS VST3 AU Standalone`, `PRODUCT_NAME "O-simpleWavetable${OUARICON_DEV_SUFFIX}"`, `VERSION "0.1.0"`
    - `IS_SYNTH TRUE`, `NEEDS_MIDI_INPUT TRUE`, `NEEDS_MIDI_OUTPUT FALSE`, `IS_MIDI_EFFECT FALSE`
    - `NEEDS_WEB_BROWSER TRUE`, `NEEDS_WEBVIEW2 TRUE`, `EDITOR_WANTS_KEYBOARD_FOCUS FALSE`
    - (CONTEXT Locked conventions; COMPAT-01, COMPAT-02 prep)
  - `target_sources`: the 5 Source files (`PluginProcessor.{cpp,h}`, `PluginEditor.{cpp,h}`, `WtVoice.h`). `target_include_directories(… PRIVATE Source)`.
  - `ouaricon_add_module(O-simpleWavetable webview-drop-streaming)` **after** `juce_add_plugin` (CONTEXT; RESEARCH §3.1).
  - `target_link_libraries`: the 13 `juce::` modules from §3.2, including `juce_audio_utils`, `juce_audio_formats`, `juce_dsp` and `juce_gui_extra`, plus the 3 `juce_recommended_*` flags PUBLIC.
  - `juce_generate_juce_header(O-simpleWavetable)` **after** `target_link_libraries`. Stage 3's mockup editor includes `<JuceHeader.h>`.
  - PUBLIC defines `JUCE_VST3_CAN_REPLACE_VST2=0`, `JUCE_WEB_BROWSER=1`, `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`, `JUCE_USE_CURL=0`. These match `mockups/v1-CMakeLists.txt` §1/§2/§4/§5, so Stage 3 only adds the `O-simpleWavetable_UIResources` binary-data target and its link line.
  - The test block `option(OUARICON_BUILD_TESTS … OFF)` + `if(OUARICON_BUILD_TESTS AND APPLE)` → `ouaricon_add_param_dump(O-simpleWavetable …/Source)` and `ouaricon_add_processor_console(O-simpleWavetable …/Source …/tests/state-check/main.cpp state-check)`, exactly as in §3.2.
- **Must not:**
  - write the token `PLUGIN_VERSION` anywhere, comments included (JUCE ignores that keyword);
  - write the literals `OuAu` / `OuDv`;
  - add `project()`, `add_subdirectory(JUCE)` or `juce_add_binary_data`;
  - edit `modules/registry.yaml`.
- **Depends on:** none.
- **Verify** (orchestrator, Task 10):
  - `grep -c 'PLUGIN_CODE OSiW' $F` = 1 and `grep -c 'VERSION "0.1.0"' $F` = 1;
  - `grep -c PLUGIN_VERSION $F` = 0 and `grep -cE 'OuAu|OuDv' $F` = 0;
  - `grep -v '^[[:space:]]*#' $F | grep -c juce_add_binary_data` = 0;
  - `grep -c 'OUARICON_COMPANY_NAME\|OUARICON_MANUFACTURER_CODE\|OUARICON_DEV_SUFFIX' $F` = 3;
  - line order from `grep -n`: `include(` < `juce_add_plugin` < `ouaricon_add_module`, and `target_link_libraries` < `juce_generate_juce_header`.
- **Done:** the file matches §3.2 token for token. Its build is proven in Task 11.

### Task 3 — `Source/WtVoice.h`  *(agent)*
- **Files:** `plugins/O-simpleWavetable/Source/WtVoice.h`
- **Do:** transcribe RESEARCH §6.2 with ASCII-only comments.
  - `struct WtSound final : juce::SynthesiserSound`, where `appliesToNote` and `appliesToChannel` both return true.
  - `class WtVoice final : juce::SynthesiserVoice`:
    - non-virtual `prepareToPlay (double, int)` that stores `sr`;
    - `canPlaySound` checks `dynamic_cast<WtSound*>`;
    - `startNote` **seeds** `pitchWheelPos` from `currentPitchWheelPosition`;
    - `stopNote` calls `clearCurrentNote()`, because there is no envelope yet;
    - `pitchWheelMoved` stores the value (the D2 ±2 st seam for Stage 2);
    - `controllerMoved` is empty;
    - `renderNextBlock(float)` is **empty** (silent);
    - members `double sr = 44100.0; int pitchWheelPos = 8192;`.
  - Include only `<juce_audio_basics/juce_audio_basics.h>`.
  - No `juce::` type names (`Synthesiser*`, `Sampler*`, `Oscillator`) as class names (memory `critical_class_name_shadows_juce_type`).
- **Depends on:** none.
- **Verify** (Task 10): `grep -c 'renderNextBlock' $F` = 1; `grep -c 'pitchWheelPos = currentPitchWheelPosition' $F` = 1; the ASCII gate is clean.
- **Done:** every pure virtual is overridden (`juce_Synthesiser.h:69,76,129,137,154,165,170,200`). Compile is proven in Task 11.

### Task 4 — `Source/PluginProcessor.h`  *(agent)*
- **Files:** `plugins/O-simpleWavetable/Source/PluginProcessor.h`
- **Do:** combine RESEARCH §4.3 (`ParamIDs`), §6.1 (class) and §5.3 (state helpers).
  - Includes: `<juce_audio_processors/juce_audio_processors.h>`, `<juce_audio_utils/juce_audio_utils.h>`, `<array>`, `<atomic>`, `"WtVoice.h"`. `<array>` must come **before** the `ParamIDs` block.
  - `namespace OSimpleWavetable::ParamIDs`: the 21 `inline constexpr auto` IDs, **exact snake_case strings** from parameter-spec.md, and `inline constexpr std::array<const char*, 21> all { … }` in spec order.
  - `class OSimpleWavetableAudioProcessor : public juce::AudioProcessor`:
    - standard overrides as O-simpleAdditive `.h:116-145`;
    - `getName()` returns `"O-simpleWavetable"`;
    - `acceptsMidi` true, `producesMidi` false, `isMidiEffect` false;
    - `getTailLengthSeconds` returns `5.0`;
    - `hasEditor` true; programs 1/0.
  - **Mockup-contract public API**, names exactly as `mockups/v1-PluginEditor.cpp` calls them (CONTEXT: the editor drops in at Stage 3):
    - `getAPVTS()`
    - `handleUiMidi (int noteNumber, bool noteOn, float velocity)`
    - public `std::atomic<int> uiLanguage { 0 }`
    - static `languageCode (int)`, which maps 2 to "zh-Hans", 1 to "fr", else "en"
    - static `languageIndex (const juce::String&)`, which maps unknown codes to 0
  - **Private, in this order** (constructor init order):
    1. `juce::AudioProcessorValueTreeState parameters;`
    2. `static createParameterLayout()`
    3. `static constexpr int kNumVoices = 16;`
    4. `juce::Synthesiser synth;`
    5. `juce::MidiMessageCollector midiCollector;`
    6. the §5.3 helpers: `kImportedBankTag = "IMPORTED_BANK"`, `kUiLanguageProp = "uiLanguage"`, `restoreImportedBank (const juce::ValueTree&)`, `writeImportedBank (juce::ValueTree&) const`, and `static void stripImportedBank (juce::ValueTree&)`, which removes **all** copies in a loop.
  - End with `JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR`.
  - **Do not** stub any Stage 2.4/3 API (`importFromFile`, `buildCycleView`, `applyFactoryPreset`, …). Those land with their implementations.
- **Depends on:** Task 3 (it includes `WtVoice.h`).
- **Verify** (Task 10):
  - the IDs extracted with `grep -oE 'inline constexpr auto [a-zA-Z]+ += "[a-z_]+"' $F | sed -E 's/.*"(.*)"/\1/' | paste -sd, -` equal `bank,position,interp,bandlimit,bit_depth,lfo_rate,lfo_sync,lfo_div,lfo_shape,lfo_depth,menv_attack,menv_decay,menv_sustain,menv_release,env_amount,amp_attack,amp_decay,amp_sustain,amp_release,voice_mode,output_level`;
  - `grep -c 'kNumVoices = 16' $F` = 1;
  - `grep -c 'std::atomic<int> uiLanguage' $F` = 1.
- **Done:** the header declares the full Stage 1 surface and nothing beyond it.

### Task 5 — `Source/PluginProcessor.cpp`: parameter layout  *(agent)*
- **Files:** `plugins/O-simpleWavetable/Source/PluginProcessor.cpp` (created here; Tasks 6–7 extend it)
- **Do:** transcribe RESEARCH §4.3 `createParameterLayout()` and the anonymous-namespace helpers.
  - Helpers: `Range` alias, `unitRange`, `bipolarRange`, `menvTimeRange`, `ampTimeRange`, `fixed (int decimals, label)`, `outputLevelAttributes()`.
  - The `addFloat` / `addChoice` / `addBool` lambdas each use `juce::ParameterID { id, 1 }`.
  - The 21 add-calls in **spec order**, with the ranges, defaults, names and labels of §4.1 / parameter-spec.md.
  - `bank` choices 0 and 1 via `juce::String (juce::CharPointer_UTF8 ("Sine \xE2\x86\x92 Saw"))` and `"…Square"`. Keep a non-hex character (the space) after each `\x92` escape. Imported is last, at index 5 (D1 seam).
  - `bit_depth` = `Full,16,15,…,3` (15 entries, mid-rise D3 seam).
  - `lfo_div` has 16 entries, default index **2** ("1/1", D5).
  - `lfo_shape` default index **1** (Triangle).
  - `output_level`: `{-60, 6, 0.1, 1}`, default -6. Its text function prints "-inf" at `db <= -59.95f` and its parser maps "-inf" to -60 (both functions, §4.2).
  - Floats use **only** the 4-arg `NormalisableRange<float>{start,end,interval,skew}` ctor, never a lambda range (CONTEXT). The `fixed()` / `outputLevelAttributes()` **text** lambdas are allowed.
  - `addChoice` keeps `jassert (c.size() >= 2)`.
  - End with **both** `jassert (params.size() == 21);` (the literal CONTEXT asks for) and `jassert (params.size() == OSimpleWavetable::ParamIDs::all.size());`, then `return { params.begin(), params.end() };`.
  - MSVC-clean: no bare `constexpr` locals used inside lambdas.
- **Depends on:** Task 4.
- **Verify:**
  - Task 10: `grep -cE '^[[:space:]]*add(Float|Choice|Bool)[[:space:]]*\(' $F` = 21 (call sites; the lambda definitions start with `auto`); `grep -c 'jassert (params.size() == 21)' $F` = 1; `grep -n 'NormalisableRange' $F | grep -c '\['` = 0 (no lambda ranges); `grep -c 'CharPointer_UTF8' $F` ≥ 2.
  - Task 12: the runtime param-dump table.
- **Done:** 13 Float + 6 Choice + 2 Bool, in spec order, with the exact IDs, ranges, defaults and skews.

### Task 6 — `PluginProcessor.cpp`: lifecycle, buses, silent `processBlock`, MIDI  *(agent)*
- **Files:** `plugins/O-simpleWavetable/Source/PluginProcessor.cpp`
- **Do:** transcribe RESEARCH §6.1.
  - Constructor: `BusesProperties().withOutput ("Output", stereo(), true)`, with **no input bus**; `parameters (*this, nullptr, "PARAMETERS", createParameterLayout())`; 16 `addVoice (new WtVoice())`; one `addSound (new WtSound())`; `setNoteStealingEnabled (true)`; `midiCollector.reset (44100.0)`. All allocation happens here.
  - `prepareToPlay`: `midiCollector.reset (sampleRate)`; `synth.setCurrentPlaybackSampleRate`; per-voice `dynamic_cast<WtVoice*>` → `prepareToPlay`; then **`setLatencySamples (0)`**. Never override `getLatencySamples`.
  - `releaseResources`: `synth.allNotesOff (0, false)`.
  - `isBusesLayoutSupported`: main out mono or stereo only, and the main input must be disabled.
  - `processBlock`: `ScopedNoDenormals`; `buffer.clear()`; return on `numSamples <= 0`, because the collector asserts `> 0`; `midiCollector.removeNextBlockOfMessages`; `synth.renderNextBlock (buffer, midi, 0, numSamples)`. No allocation, no locks beyond the collector's sibling-accepted CriticalSection, and no output gain stage (RESEARCH §8 #16 defers it to Stage 2).
  - `handleUiMidi`: copy O-simpleAdditive `PluginProcessor.cpp:361-376` verbatim (`jlimit` on the note, NaN-velocity guard, `setTimeStamp (Time::getMillisecondCounterHiRes() * 0.001)`).
  - `createEditor()` guarded exactly as in RESEARCH §6.3: `#if JUCE_WEB_BROWSER` + `#include "PluginEditor.h"` placed **directly above `createEditor()`, not at the top of the file**; `#else` returns `new juce::GenericAudioProcessorEditor (*this)`. This keeps the console targets (`JUCE_WEB_BROWSER=0`) linking (memory `pattern_render_harness_breaks_on_webview_editor`).
  - Factory: `createPluginFilter()` returns `new OSimpleWavetableAudioProcessor()`.
- **Depends on:** Task 5 (same file).
- **Verify:**
  - Task 10: `grep -c '#include "PluginEditor.h"' $F` = 1 and `grep -B1 '#include "PluginEditor.h"' $F | grep -c '#if JUCE_WEB_BROWSER'` = 1; `grep -c 'setLatencySamples (0)' $F` = 1; `grep -c 'withInput' $F` = 0.
  - Task 12: probes P6 and P7.
- **Done:** an instrument-shaped, zero-latency, silent processor that consumes MIDI.

### Task 7 — `PluginProcessor.cpp`: state save/restore + `IMPORTED_BANK` stub  *(agent)*
- **Files:** `plugins/O-simpleWavetable/Source/PluginProcessor.cpp`
- **Do:** transcribe RESEARCH §5.3 (resolutions C2/C3).
  - **`getStateInformation`:**
    1. `copyState()`
    2. `stripImportedBank (state)`
    3. `setProperty (kUiLanguageProp, languageCode (uiLanguage.load (acquire)))`. Persist the **code string**, never the int.
    4. `writeImportedBank (state)`. In Stage 1 this is a no-op that appends nothing.
    5. `copyXmlToBinary`.
  - **`setStateInformation`:**
    1. `getXmlFromBinary`; return on null.
    2. `ValueTree::fromXml`; return if it is invalid or its type is not `parameters.state.getType()` (V5 input validation).
    3. Read `uiLanguage` from the **incoming** tree, gated on `! lang.isVoid()` (never `isInt()`, memory `critical_valuetree_xml_roundtrip_loses_type`), then `uiLanguage.store (languageIndex (lang.toString()), release)`.
    4. `bankChild = state.getChildWithName (kImportedBankTag).createCopy()`.
    5. `stripImportedBank (state)`, which removes **all** copies.
    6. `parameters.replaceState (state)`.
    7. `restoreImportedBank (bankChild)`. In Stage 1 this is a no-op that tolerates both a present and an invalid child.
  - Mark the Stage 2.4 seams in ASCII comments:
    - decode → build mips → publish;
    - absent child → retire the bank (never free it here);
    - `numFrames` / `version` parse via `.toString().getIntValue()`;
    - clamp `numFrames` to ≤ 256.
- **Depends on:** Task 6 (same file).
- **Verify:**
  - Task 10: `grep -c 'stripImportedBank' $F` ≥ 2 (two call sites); `grep -c 'isVoid' $F` ≥ 1; `grep -c 'isInt' $F` = 0.
  - Task 12: probes P1–P5 and P8.
- **Done:** the round trip carries all 21 parameters plus `uiLanguage`. No `IMPORTED_BANK` child is ever re-emitted or kept in the live tree.

### Task 8 — `Source/PluginEditor.{h,cpp}` (generic shell)  *(agent)*
- **Files:** `plugins/O-simpleWavetable/Source/PluginEditor.h`, `plugins/O-simpleWavetable/Source/PluginEditor.cpp`
- **Do:** port `git show 88073650:plugins/O-simpleAdditive/Source/PluginEditor.{h,cpp}` (RESEARCH §6.3), renamed.
  - `class OSimpleWavetableAudioProcessorEditor : public juce::AudioProcessorEditor`, with `explicit OSimpleWavetableAudioProcessorEditor (OSimpleWavetableAudioProcessor&)`. The name and ctor must match `mockups/v1-PluginEditor.h:49-53`.
  - Members `OSimpleWavetableAudioProcessor& processorRef;` and `juce::GenericAudioProcessorEditor genericEditor;`, the latter initialised with `(p)`.
  - Ctor: `addAndMakeVisible (genericEditor); setSize (420, 640);`.
  - `paint` fills the background; `resized` sets `genericEditor.setBounds (getLocalBounds())`.
  - The header includes `"PluginProcessor.h"`. Comment it as a Stage 1 placeholder that Stage 3 replaces with `v1-PluginEditor.{h,cpp}`.
- **Depends on:** Task 4 (processor class name).
- **Verify:** Task 10: `grep -c 'class OSimpleWavetableAudioProcessorEditor' PluginEditor.h` = 1; `grep -c 'GenericAudioProcessorEditor genericEditor' PluginEditor.h` = 1. Task 13: the pluginval GUI tests open it.
- **Done:** all 21 parameters are editable in any host. Stage 3 can drop the mockup editor in with no rename.

### Task 9 — `tests/state-check/main.cpp` (probe driver)  *(agent)*
- **Files:** `plugins/O-simpleWavetable/tests/state-check/main.cpp`
- **Do:** write a console driver (RESEARCH §7.2; pattern `plugins/O-simpleSampler/tests/render-harness/main.cpp:704-768`).
  - **Ground rules:**
    - Run under `juce::ScopedJuceInitialiser_GUI`.
    - Construct `OSimpleWavetableAudioProcessor` directly, including only `"PluginProcessor.h"`, never the editor.
    - Call `setPlayConfigDetails (0, 2, fs, 512)` before every `prepareToPlay`.
    - Use only the public API: `getParameters()`, `getAPVTS()`, `uiLanguage`, `get/setStateInformation`, `processBlock`, `handleUiMidi`, `checkBusesLayoutSupported`. `isBusesLayoutSupported` is protected in JUCE (`juce_AudioProcessor.h:1432`), so do not call it.
    - Use **explicit** checks, never `jassert` as the test mechanism.
    - Print exactly one line per probe, `PASS Pn <name>` or `FAIL Pn <name>: <detail>`.
    - Return 0 only if all probes pass, else 1.
    - ASCII-only source; build U+2192 with hex escapes.
  - **Probes:**
    - **P0 param-contract:**
      - `getParameters().size() == 21`;
      - each `getParameterID()` equals `ParamIDs::all[i]` in order;
      - `bank` choice 0 `toUTF8()` contains the bytes `E2 86 92` and equals `CharPointer_UTF8 ("Sine \xE2\x86\x92 Saw")`;
      - `output_level` `getText (0.0f, 64) == "-inf"` and `getValueForText ("-inf") == 0.0f`;
      - `bit_depth` default text is "Full", `lfo_div` default text is "1/1", `lfo_shape` default text is "Triangle".
    - **P1 uiLanguage round trip:**
      - for `uiLanguage` = 2 and = 1: save, load into a fresh instance, and expect the same value;
      - the saved XML attribute `uiLanguage` must be the string "zh-Hans" (respectively "fr").
    - **P2 uiLanguage absent / unknown:**
      - a blob with the attribute removed leaves a fresh instance at 0;
      - a blob carrying `uiLanguage="de"` gives 0 (allow-list).
    - **P3 IMPORTED_BANK tolerance:**
      - add one fake `IMPORTED_BANK` child (`version="1" filename="fake.wav" numFrames="3" encoding="flac16" data="AAAA"`) to a saved blob, then repeat with **two** children;
      - each loads, the parameters are restored, and the live `getAPVTS().state` has **0** children of that type.
    - **P4 two-reopen:**
      - load the P3 blob → save → load → save;
      - each saved XML has **0** `IMPORTED_BANK` children (Stage 2.4 flips this to exactly 1).
    - **P5 non-default params:**
      - set `bank`=5, `interp`=false, `bandlimit`=false, `bit_depth`=9, `lfo_div`=15, `voice_mode`=1, `position`=0.37, `env_amount`=-0.5, `output_level`=-60 via `setValueNotifyingHost (convertTo0to1 (x))`;
      - save, load into a fresh instance, and compare `getRawParameterValue` (exact for choice/bool, |Δ| ≤ 1e-4 for floats).
    - **P6 silent render:**
      - `prepareToPlay (48000, 512)`;
      - pre-fill the buffer with 1.0f, then `processBlock` with note-on 60, pitch-wheel 12000 at sample 100 and note-off at 400, then a block with 20 simultaneous note-ons;
      - every sample must be exactly `0.0f` and finite;
      - a 0-sample buffer returns cleanly;
      - `handleUiMidi (60, true, NaN)` and `handleUiMidi (200, true, 0.8f)`, then `processBlock`: still silent, no crash.
    - **P7 shell identity:**
      - `acceptsMidi()`, `! producesMidi()`, `! isMidiEffect()`;
      - `getBusCount (true) == 0` and `getBusCount (false) == 1`;
      - `getLatencySamples() == 0` after prepare;
      - `getTailLengthSeconds() == 5.0`;
      - `checkBusesLayoutSupported` accepts {in: none, out: stereo} and {out: mono} and rejects {out: 5.1}.
    - **P8 hostile blobs** (ASVS V5):
      - garbage bytes, valid XML with the wrong root tag, and a zero-length blob → no crash, and the parameters keep their pre-call values.
- **Depends on:** Tasks 4 and 7 (API + state semantics).
- **Verify:** Task 12 shows exit 0, 9 `PASS` lines, 0 `FAIL` lines and 0 `JUCE Assertion failure` lines.
- **Done:** the only machine check of criteria 4 (text) and 5 (`uiLanguage` + `IMPORTED_BANK`) exists and passes.

### Task 10 — Static source gates  *(orchestrator, after the agent returns)*
- **Files:** read-only.
- **Do:**
  1. Run every grep in the **Verify** lines of Tasks 2–8. Run in `bash`, with `$F` set to each file.
  2. Run the ASCII gate (RESEARCH §8 #1): `LC_ALL=C grep -n $'[^\t -~]' plugins/O-simpleWavetable/CMakeLists.txt plugins/O-simpleWavetable/Source/*.h plugins/O-simpleWavetable/Source/*.cpp plugins/O-simpleWavetable/tests/state-check/main.cpp`. It must print **nothing**.
  3. Check the agent JSON: `parameter_count == 21`, the IDs are in spec order, and `stateUpdated: false`.
  4. Confirm `git status --short -- plugins/O-simpleWavetable` lists only the 7 new files, plus the pre-existing `STATUS.md` / `RESEARCH.md` / `PLAN.md` / gate files.
- **Depends on:** Tasks 2–9.
- **Verify:** all counts equal their targets and the ASCII gate output is empty.
- **Done:** on any miss, fix it with a targeted Edit (orchestrator) or re-dispatch the agent with the failing gate quoted. Never edit a gate's expected value to match the code.

### Task 11 — Configure, build all three formats, install  *(orchestrator; this is the end-to-end proof)*
- **Files:** none in the tree. Configure writes the gitignored `plugins/O-simpleWavetable/Source/ui/public/modules/webview-drop-streaming.js`.
- **Do** (each step in the background, logged to `$SCRATCH`):
  1. `cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release` from the repo root, once, to register the new folder (RESEARCH §1 pitfall 4).
     - **Never** pass `--reconfigure`: it `rm -rf`s the shared `build/`.
     - **Never** set the `SKIP_PLUGINS` or `OUARICON_BUILD_TESTS` cache vars in `build/`.
     - If a **foreign** plugin's CMakeLists breaks the configure (O-Bassoon, O-Freeze, O-Gain, O-Reed and O-simpleFM all have uncommitted CMake edits right now), stop and report. Do not touch their files.
  2. `./scripts/build-and-install.sh O-simpleWavetable > $SCRATCH/build-install.log 2>&1`. It builds `_VST3` and `_AU`, sweeps the `-dev`/unsuffixed variants, clears the AU cache and installs.
  3. `ninja -C build O-simpleWavetable_Standalone > $SCRATCH/standalone.log 2>&1`. The script never builds the Standalone (memory `pattern_build_install_skips_standalone_stale_ui`).
- **Depends on:** Task 10.
- **Verify:**
  - `grep -c 'O-simpleWavetable_VST3' build/build.ninja` ≥ 1.
  - `git check-ignore -q plugins/O-simpleWavetable/Source/ui/public/modules/webview-drop-streaming.js` exits 0.
  - Both logs have zero errors, and `grep -E 'plugins/O-simpleWavetable/(Source|tests)/.*(warning|error):' $SCRATCH/*.log | wc -l` = 0.
  - These all exist:
    - `~/Library/Audio/Plug-Ins/VST3/O-simpleWavetable-dev.vst3`
    - `~/Library/Audio/Plug-Ins/Components/O-simpleWavetable-dev.component`
    - `build/plugins/O-simpleWavetable/O-simpleWavetable_artefacts/Release/Standalone/O-simpleWavetable-dev.app`
  - Version stamp: `/usr/libexec/PlistBuddy -c "Print AudioComponents:0:version" ~/Library/Audio/Plug-Ins/Components/O-simpleWavetable-dev.component/Contents/Info.plist` = **256** (0.1.0; memory `critical_plugin_version_keyword_ignored_by_juce`).
- **Done:** success criterion 1 is met. A compile error routes back to Task 10's fix loop with the error quoted.

### Task 12 — Probe build + run (param-dump, state-check)  *(orchestrator; out-of-repo, Debug per P-A)*
- **Files:** `$SCRATCH/build-oswt/`, `$SCRATCH/params.tsv` and `$SCRATCH/*.log`. Nothing lands in the repo.
- **Do:** follow RESEARCH §7.2, in **bash**, not zsh.
  1. Build the skip list `SKIP=$(for d in plugins/*/; do n=$(basename "$d"); [ -f "${d}CMakeLists.txt" ] && [ "$n" != O-simpleWavetable ] && printf '%s;' "$n"; done)`.
  2. Configure: `cmake -S . -B "$SCRATCH/build-oswt" -G Ninja -DCMAKE_BUILD_TYPE=Debug -DOUARICON_BUILD_TESTS=ON "-DSKIP_PLUGINS=$SKIP"`.
  3. Build: `cmake --build "$SCRATCH/build-oswt" --target O-simpleWavetable-param-dump O-simpleWavetable-state-check`. Run it in the background.
  4. Locate both binaries with `find "$SCRATCH/build-oswt" -name '<target>' -perm +111 -type f`.
  5. Run `param-dump > $SCRATCH/params.tsv 2> $SCRATCH/param-dump.err` and `state-check > $SCRATCH/state-check.log 2>&1; echo "exit=$?"`.
- **Depends on:** Task 11 (the main build is proven first, so compile errors surface there), and Task 9.
- **Verify:**
  - `grep -vc '^#' $SCRATCH/params.tsv` = **21**, and the `# params` header line reads 21.
  - `grep -v '^#' $SCRATCH/params.tsv | cut -f1 | paste -sd, -` equals the 21-ID string from Task 4.
  - Columns `label` / `textAtMin` / `textAtMax` / `defaultText` (TSV cols 3/5/6/8) match the table below.
  - `LC_ALL=C grep -c $'Sine \xe2\x86\x92 Saw' $SCRATCH/params.tsv` = 1 (UTF-8 survived).
  - state-check: `exit=0`, `grep -c '^PASS P' $SCRATCH/state-check.log` = 9, `grep -c '^FAIL' …` = 0.
  - `grep -c 'JUCE Assertion failure' $SCRATCH/state-check.log $SCRATCH/param-dump.err` = 0 for both. This is the live check of `jassert (params.size() == 21)`, Choice ≥ 2, the collector asserts and leak detection.
- **Done:** success criteria 4 and 5 are proven by machine. A mismatch is a finding: fix the source, never the expected table.

**Expected param-dump columns** (label · textAtMin · textAtMax · defaultText):

| id | label | min | max | default |
|----|-------|-----|-----|---------|
| bank | | Sine → Saw | Imported | Sine → Saw |
| position | | 0.000 | 1.000 | 0.000 |
| interp | | Off | On | On |
| bandlimit | | Off | On | On |
| bit_depth | | Full | 3 | Full |
| lfo_rate | Hz | 0.01 | 20.00 | 0.50 |
| lfo_sync | | Free | Tempo | Free |
| lfo_div | | 4 bars | 1/16T | 1/1 |
| lfo_shape | | Sine | S&H | Triangle |
| lfo_depth | | 0.000 | 1.000 | 0.000 |
| menv_attack | s | 0.001 | 10.000 | 0.500 |
| menv_decay | s | 0.001 | 10.000 | 1.000 |
| menv_sustain | | 0.000 | 1.000 | 0.000 |
| menv_release | s | 0.001 | 10.000 | 0.500 |
| env_amount | | -1.000 | 1.000 | 0.000 |
| amp_attack | s | 0.001 | 5.000 | 0.005 |
| amp_decay | s | 0.001 | 5.000 | 0.300 |
| amp_sustain | | 0.000 | 1.000 | 0.800 |
| amp_release | s | 0.001 | 5.000 | 0.200 |
| voice_mode | | Poly | Mono | Poly |
| output_level | dB | -inf | 6.0 | -6.0 |

### Task 13 — Host validation: targeted auval + pluginval  *(orchestrator)*
- **Files:** `$SCRATCH/auval.log`, `$SCRATCH/pluginval-vst3.log`, `$SCRATCH/pluginval-au.log`
- **Do:** run each in its own background call and poll its log.
  1. auval: `bash scripts/verify-au-link.sh O-simpleWavetable > $SCRATCH/auval.log 2>&1`. It runs `auval -v aumu OSiW OuDv`. **Never use `auval -a`.** The first call after Task 11's `killall AudioComponentRegistrar` is a cold registry rescan (83 s to 17 min observed), so wait it out. Do not kill it.
  2. Negative control: `auval -v aumu ZZZZ OuDv` must report "didn't find". This proves the lookup is real.
  3. pluginval on the VST3: `/Applications/pluginval.app/Contents/MacOS/pluginval --strictness-level 10 --timeout-ms 600000 --validate ~/Library/Audio/Plug-Ins/VST3/O-simpleWavetable-dev.vst3`. Keep the GUI tests **on**: the generic editor must open.
  4. pluginval on the AU: the same command on `~/Library/Audio/Plug-Ins/Components/O-simpleWavetable-dev.component`.
- **Depends on:** Task 11. It can run in parallel with Task 12, because the background builds are independent.
- **Verify:**
  - `grep -c 'AU VALIDATION SUCCEEDED' $SCRATCH/auval.log` = 1;
  - `grep -c '21 Global Scope Parameters' $SCRATCH/auval.log` = 1;
  - the auval MIDI test PASSes and the latency property is 0;
  - both pluginval runs exit 0, with a `SUCCESS` summary and no `FAILED` test.
- **Done:** success criterion 2 is met. A single auval warning, "Parameter did not retain default value when set", on a skewed time/rate parameter is the known-benign suite behaviour (O-simpleAdditive Stage 1 VERIFICATION): record it, do not fail on it. Any other warning is a finding.

### Task 14 — DAW / Standalone smoke  *(checkpoint: human-verify, Taylor; does not block Task 15, its result feeds the verify phase)*
- **Files:** none.
- **Do:**
  1. **Logic:** new Software Instrument track → AU Instruments → Ouaricon Audio Development → `O-simpleWavetable-dev`.
     - It loads as an **instrument**.
     - The generic editor lists **21** rows in spec order (Bank … Output Level).
     - The text reads `Sine → Saw` (real arrow, no mojibake), `Full`, `-6.0 dB`, and `-inf` at the Output Level floor.
     - Play MIDI: **silence**, no crash, no CPU spike.
     - Change Bank → Imported, Bit Depth → 8 and Output Level → -60. Save the project, quit Logic, reopen: the values are restored.
  2. **Reaper or Live:** repeat with the VST3.
  3. **Standalone:** open `O-simpleWavetable-dev.app` (`/show-standalone O-simpleWavetable`). The window opens with the generic editor and no crash.
- **Depends on:** Task 11 (artefacts) and Task 13 (validated binaries).
- **Verify:** Taylor replies "pass", or lists the defects. The result is recorded in VERIFICATION.md (verify phase).
- **Done:** host-side criteria 3–5 are confirmed. `uiLanguage` cannot be set without a UI, so P1/P2 are its only evidence.

### Task 15 — SUMMARY, STATUS, PLUGINS row, path-scoped commit  *(orchestrator)*
- **Files:**
  - create `plugins/O-simpleWavetable/.planning/stages/1-foundation/SUMMARY.md`;
  - edit `plugins/O-simpleWavetable/.planning/STATUS.md`;
  - edit `PLUGINS.md`, one row only.
- **Do:**
  1. **SUMMARY.md** in the O-simpleAdditive SUMMARY format:
     - the files created;
     - the 21-param layout;
     - the RESEARCH amendments applied (ASCII comments, literal-21 jassert, Debug probes);
     - deviations;
     - the build/validation evidence, with log excerpts from Tasks 11–13.
  2. **STATUS.md:**
     - frontmatter `status: stage_1_execute_complete`, `current_phase: verify`, `next_action: plugin_verify_stage_1`, `last_updated`;
     - Stage 1 phase table: execute ✓ with the date;
     - append a Stage 1 execute line under "Completed So Far";
     - leave `contract_checksums` untouched (the contracts did not change).
  3. **PLUGINS.md** working-tree row 63 becomes `| O-simpleWavetable | 🚧 Stage 1 | 0.1.0 | Synth (Pedagogical Wavetable) | <YYYY-MM-DD> |`. Edit only that line.
  4. Re-check **immediately before** committing: `git branch --show-current` (`main`), `git status --short`, `git diff --cached --stat`. `PLUGINS.md` is foreign-staged (`MM`) at plan time; re-confirm.
  5. **Temp-index CAS commit** (resolution C6; memories `pattern_shared_checkout_index_race_between_sessions`, `pattern_stale_base_temp_index_commit_reverts_shared_row`). Run in `bash -c` to dodge the zsh word-splitting and `:`-modifier traps.
     1. Set `GIT_INDEX_FILE=$SCRATCH/idx`; `old=$(git rev-parse HEAD)`; `git read-tree $old`.
     2. `git add -- plugins/O-simpleWavetable`. This respects `.gitignore`, so the drop JS stays out.
     3. Build the `PLUGINS.md` blob from `git show "${old}:PLUGINS.md"` with **only** the `O-simpleWavetable` row substituted. Add it with `git hash-object -w`, then `git update-index --cacheinfo "100644,<blob>,PLUGINS.md"`.
     4. `git write-tree`, then `git commit-tree -p $old`.
     5. `git update-ref refs/heads/main <new> $old`, a compare-and-swap. If HEAD moved, retry the loop from the new HEAD.
     6. Message: `feat(O-simpleWavetable): Stage 1 foundation - silent 16-voice synth shell, 21-param APVTS, state stub`, with a body listing the CMake identity (OSiW, 0.1.0), the params, the state behaviour and the validation results.
     7. Unset `GIT_INDEX_FILE`.
  6. **Post-commit resync:**
     1. `git reset -q -- plugins/O-simpleWavetable` (own paths only). **Not** `PLUGINS.md`, which is foreign-staged.
     2. Inspect `git diff --cached -- PLUGINS.md`. If the other session's staged blob would revert this row, patch **only this row** inside their staged blob (`git show :PLUGINS.md | sed … | git hash-object -w --stdin` → `update-index --cacheinfo`). If their staged blob holds nothing but a stale copy of this row, `git reset -q -- PLUGINS.md` instead.
  7. **Never** `git add -A`, `git commit -a`, or a bare `git reset`.
- **Depends on:** Tasks 11, 12 and 13 must pass. Task 14 is not a precondition.
- **Verify:**
  - `git show --stat HEAD` lists only `plugins/O-simpleWavetable/**` and `PLUGINS.md`;
  - `git show HEAD:PLUGINS.md | grep '^| O-simpleWavetable'` shows the Stage 1 row;
  - `git diff "HEAD~1" HEAD -- PLUGINS.md` changes exactly 1 row;
  - `git ls-files plugins/O-simpleWavetable/Source/ui` is empty;
  - `git cat-file -e HEAD:plugins/O-simpleWavetable/tests/state-check/main.cpp` succeeds;
  - `git diff --cached -- PLUGINS.md` shows only foreign rows, or nothing.
- **Done:** the commit is on `main`, scoped to this plugin plus its one row, with no foreign rows swept in. Then hand off: **Step 1** `/clear`, **Step 2** `/plugin-verify O-simpleWavetable 1-foundation`, and STOP.

---

## Files to create / modify

| Path | Action | By | Task |
|------|--------|----|------|
| `plugins/O-simpleWavetable/CMakeLists.txt` | create | agent | 2 |
| `plugins/O-simpleWavetable/Source/WtVoice.h` | create | agent | 3 |
| `plugins/O-simpleWavetable/Source/PluginProcessor.h` | create | agent | 4 |
| `plugins/O-simpleWavetable/Source/PluginProcessor.cpp` | create + extend | agent | 5, 6, 7 |
| `plugins/O-simpleWavetable/Source/PluginEditor.h` | create | agent | 8 |
| `plugins/O-simpleWavetable/Source/PluginEditor.cpp` | create | agent | 8 |
| `plugins/O-simpleWavetable/tests/state-check/main.cpp` | create | agent | 9 |
| `plugins/O-simpleWavetable/.planning/stages/1-foundation/SUMMARY.md` | create | orchestrator | 15 |
| `plugins/O-simpleWavetable/.planning/STATUS.md` | modify | orchestrator | 15 |
| `PLUGINS.md` (O-simpleWavetable row only) | modify | orchestrator | 15 |
| `plugins/O-simpleWavetable/.planning/gate-bypasses.log`, `stages/0-ideation/gate-report.json` | created by `run-gate.sh` | orchestrator | 1 |
| `plugins/O-simpleWavetable/Source/ui/public/modules/webview-drop-streaming.js` | generated at configure; **gitignored, never commit** | cmake | 11 |

**Must not touch:**
- root `CMakeLists.txt`, `.gitignore`, `modules/registry.yaml` (derived by `scripts/regen-registry-used-by.sh`);
- `build/CMakeCache.txt` options;
- any other plugin's files;
- `mockups/` (Stage 3 input, read-only).

---

## Dependency graph / waves

```
Wave 0  [orch]  T1 pre-flight + gate bypass
           |
Wave 1  [agent, single dispatch]
           T2 CMakeLists ----------------------------------+
           T3 WtVoice.h -> T4 PluginProcessor.h -+-> T5 -> T6 -> T7 (PluginProcessor.cpp, sequential)
                                                 +-> T8 PluginEditor.{h,cpp}
                                                 +-> T9 state-check main.cpp (needs T7 semantics)
           |
Wave 2  [orch]  T10 static gates -> T11 configure + build VST3/AU/Standalone + install
           |
Wave 3  [orch, parallel background]   T12 probe build+run (Debug, out-of-repo)
                                      T13 auval (targeted) + pluginval VST3/AU
           |
Wave 4  [orch]  T15 SUMMARY + STATUS + PLUGINS row + temp-index commit     (needs T11-T13 green)
        [human] T14 DAW/Standalone smoke  (after T13; parallel to T15; feeds verify phase)
```

---

## Success criteria (goal-backward; all must be TRUE for the verify phase)

| # | Observable truth | Evidence (measurable) | Task |
|---|------------------|-----------------------|------|
| S1 | Builds clean in all 3 formats | VST3 + AU via `build-and-install.sh`, and Standalone via `ninja -C build O-simpleWavetable_Standalone`. 0 errors, 0 warnings from `plugins/O-simpleWavetable/{Source,tests}`. 3 artefacts exist. | 11 |
| S2 | Correct identity + version | AU Info.plist `AudioComponents:0:version` = 256 (0.1.0); triple `aumu`/`OSiW`/`OuDv` (dev branding); bundles named `O-simpleWavetable-dev.*`. | 11, 13 |
| S3 | AU validates (targeted) | `auval -v aumu OSiW OuDv` gives `AU VALIDATION SUCCEEDED` and `21 Global Scope Parameters`; the negative control `ZZZZ` gives "didn't find"; `auval -a` is never run. | 13 |
| S4 | pluginval passes | strictness 10, GUI tests on, exit 0, for both the VST3 and the AU. | 13 |
| S5 | Exactly 21 params, spec order, exact IDs | param-dump: 21 data rows, ID column == spec string; source has `jassert (params.size() == 21)`; the Debug probe run logs 0 `JUCE Assertion failure`. | 5, 12 |
| S6 | Correct ranges, defaults, text | param-dump matches the expected table, including `Full`, `-inf`, `-6.0`, `1/1`, `Triangle`, `On`. The `bank` UTF-8 bytes `E2 86 92` are intact. Only plain 4-arg `NormalisableRange` (static gate). | 5, 10, 12 |
| S7 | Instrument that accepts MIDI and stays silent | `IS_SYNTH`/`NEEDS_MIDI_INPUT`; 0 input buses / 1 output bus (P7); auval MIDI test PASS; P6 gives all samples exactly 0.0f under note-on, pitch-wheel and 20-note load; 0-sample block safe; latency 0. | 6, 12, 13 |
| S8 | State round trip incl. `uiLanguage` | P1 (2 and 1 survive; persisted as code strings), P2 (absent → 0; unknown → 0), P5 (9 non-default params survive a fresh-instance load). | 7, 12 |
| S9 | `IMPORTED_BANK` stub tolerance | P3 (1 or 2 children load; the live tree holds 0), P4 (two reopens → 0 children in every save). Stage 1 writes no child. | 7, 12 |
| S10 | Hostile state is harmless | P8 (garbage / wrong root / empty → no crash, params unchanged). | 7, 12 |
| S11 | Stage 3 needs no CMake or name rework | CMake carries the `NEEDS_WEB_BROWSER`/`NEEDS_WEBVIEW2` flags, the 4 WebView defines, `juce_gui_extra`, `ouaricon_add_module(… webview-drop-streaming)` and `juce_generate_juce_header`. The editor class/ctor and the processor API (`getAPVTS`, `handleUiMidi`, `uiLanguage`, `languageCode`/`languageIndex`) match `mockups/v1-PluginEditor.*`. The drop JS is copied and gitignored. | 2, 4, 8, 11 |
| S12 | RT-safe silent `processBlock` | `ScopedNoDenormals`, `buffer.clear()`, no allocation (all 16 voices and the sound are allocated in the ctor), early return before the collector on 0 samples. | 6, 10 |
| S13 | Clean, scoped commit | HEAD touches only `plugins/O-simpleWavetable/**` + 1 `PLUGINS.md` row; the generated JS is not tracked; no foreign rows are swept in. | 15 |
| S14 | Host-side confirmation | Taylor confirms the Task 14 smoke (instrument load, 21 rows, silence, project save/reopen). | 14 |

### Coverage audit

| Source item | Covered by |
|-------------|-----------|
| GOAL: silent synth shell, Additive structure | T2–T8, S1–S12 |
| COMPAT-01 build/validate | T11, T13 (S1–S4) |
| COMPAT-02 prep (WebView2 flags) | T2 (S11) |
| Parameter contract (21, order, IDs, ranges, skews, text) | T4, T5, T12 (S5, S6) |
| MIDI plumbing (`IS_SYNTH`, MIDI in, stereo out, no input, `MidiMessageCollector`) | T2, T6 (S7) |
| State (`copyState` round trip, `uiLanguage`, `IMPORTED_BANK` stub) | T7, T9, T12 (S8–S10) |
| D1 silence on empty Imported | `bank` choice "Imported" at index 5 (T5). Silence itself is Stage 2. |
| D2 ±2 st bend | `WtVoice` seeds/stores `pitchWheelPos` (T3). The bend maths is Stage 2. |
| D3 mid-rise quantizer | `bit_depth` = Full, 16…3 (15 entries) (T5). The quantizer is Stage 2. |
| D4 reject < 2048-sample imports | none in Stage 1 (importer is Stage 2.4); nothing to declare |
| D5 `lfo_div` default 1/1 | T5 (index 2), checked in T12 (S6) |
| D6 per-frame +24 dB normalization | none in Stage 1 (Stage 2.4); nothing to declare |
| CONTEXT locked conventions (OSiW, `OUARICON_*`, class names, module line, links, plain ranges, "-inf"/"Full", `setLatencySamples(0)`, no shadowing, Choice ≥ 2, `WtVoice`/`WtSound`) | T2–T8, gates in T10/T12 |
| RESEARCH top-5 pitfalls (UTF-8 arrow, stale child, `uiLanguage` type loss, ninja graph, `auval -a`) | T5/T12, T7/T12, T7/T12, T11, T13 |
| RESEARCH §14 open item (state-check in Stage 1) | T9, T12 (P-C) |

No unplanned items.

---

## Out of scope (Stage 1)

- **Any DSP or audio:** banks, `BankFactory`, mipmaps, oscillator read, frame interp/latch, frozen-cycle crossfader, quantizer, amp/mod ADSR, LFO, Position smoothing, Mono `renderMonoLegato`, bend maths, output gain `SmoothedValue`, `isfinite` scrub, display atomics, `noteAge`, reaper counters (`blockEntries`/`blockGeneration`), importer, FLAC/base64 encoding, the real `IMPORTED_BANK` read/write (all Stage 2).
- **WebView UI:**
  - relays, attachments, the `juce_add_binary_data` UI target, `Source/ui/public/**`, native functions, the viz timer, `cycleUpdate`/`bankUpdate`/`importStatus`, `uiReady`, i18n files, `tests/i18n-states.json`, UI gates (Stage 3);
  - any Stage 2.4/3 processor API stubs (`importFromFile`, `importFromMemory`, `getImportStatus*`, `getBankDisplayGeneration`, `getBankThumbnails`, `buildCycleView`, `applyFactoryPreset`).
- **Polish:** presets, CPU work, Windows build, CHANGELOG, registry `used_by` (Stage 4 or script-derived).

---

## Risks and gotchas (carried from RESEARCH, plus planning findings)

| # | Risk | Mitigation in this plan |
|---|------|-------------------------|
| R1 | U+2192 in `bank` mangled by `juce::String (const char*)` (memory `critical_juce_string_char_ctor_is_ascii_only`). Silent: build, auval and pluginval all pass. | `CharPointer_UTF8` hex escapes (T5); byte check in P0 and the TSV grep (T12); visual check in T14. |
| R2 | Stale `IMPORTED_BANK` child re-emitted after `replaceState` (memory `critical_preset_manager_stale_customstate_child`). It needs two reopens to show. | Strip before `replaceState` and from `copyState()` (T7); P3/P4 two-reopen probe (T12). |
| R3 | `uiLanguage` loses its type through XML (memory `critical_valuetree_xml_roundtrip_loses_type`). | Persist the code string; `isVoid()` gate; static gate `isInt` = 0 (T7/T10); P1/P2. |
| R4 | The new folder is invisible to ninja until a configure; `--reconfigure` wipes the shared `build/`. | A single plain configure (T11); never `--reconfigure`, never cache vars in `build/`. |
| R5 | `auval -a` SIGABRT; the cold rescan after a cache clear takes minutes (memories `pattern_auval_a_sigabrt_native_instruments_tcc`, `pattern_cold_auval_after_install_rescans_registry`). | Targeted `verify-au-link.sh`, backgrounded and polled, with a negative control (T13). |
| R6 | **Transcription hazard (planning finding):** RESEARCH snippets carry non-ASCII comment characters (→ — … ±), which would fail RESEARCH's own ASCII gate. | Agent override: ASCII comments; T10 ASCII gate. |
| R7 | Agent template defaults contradict CONTEXT (branding literals, `Ou` codes, 1.0.0, `"Parameters"`, top-of-file editor include, self-updating STATUS/PLUGINS) (C5). | Explicit overrides at the top of the plan and per task; T10 greps for `OuAu|OuDv`, `PLUGIN_VERSION`, include placement and `stateUpdated:false`. |
| R8 | `jassert` compiles out in Release, so "jassert(size==21)" proves nothing at runtime. | Debug probe build logs every assertion; the 0-assertion gate (T12, P-A). |
| R9 | A foreign in-flight CMake edit (5 plugins dirty right now) breaks the shared configure. | Stop and report; never edit foreign files (T11). The out-of-repo probe build uses `SKIP_PLUGINS`, so it is immune. |
| R10 | `PLUGINS.md` is foreign-staged; a pathspec commit would sweep foreign working-tree rows, and a later foreign commit could revert this row (C6). | Temp-index CAS commit with a HEAD-based blob; patch only this row in the foreign staged blob; post-commit row check (T15). |
| R11 | `PLUGIN_VERSION` keyword silently ignored, so 1.0.0 ships. | `VERSION "0.1.0"`; grep gate; PlistBuddy = 256 (T2/T11). |
| R12 | Lambda `NormalisableRange` invisible to the Stage 3 WebView slider. | Only 4-arg ranges; static gate (T5/T10). Text lambdas are allowed. |
| R13 | The editor include at file top breaks the console targets once Stage 3's WebView editor lands (memory `pattern_render_harness_breaks_on_webview_editor`). | Guarded include + guarded `createEditor()` (T6); T12 proves the console link today. |
| R14 | Benign auval "did not retain default value" on skewed params misread as a failure. | Documented allowance in T13. Any other warning is a finding. |
| R15 | Logic caches AU I/O per VERSION (memory `critical_logic_caches_au_io_config_per_version`). | The Stage 1 bus layout is final for the plugin's life. If a later stage changes buses, bump VERSION; clearing caches does not help. |
| R16 | Input validation (ASVS V5): `setStateInformation` parses untrusted blobs, and `handleUiMidi` takes WebView ints/floats. | Root-type check, allow-listed `languageIndex`, note `jlimit`, NaN-velocity guard (T6/T7); P6/P8. Stage 2.4 must clamp `numFrames` ≤ 256 and verify it against the decoded length (seam comment, T7). |
| R17 | Host text for unit params is 0..1 with 3 decimals (`fixed(3)`), while parameter-spec "displays 0–100 %" (UI). | Cosmetic and non-contractual (RESEARCH A3). The WebView reads `getScaledValue()`. A "%" host text would be a text-lambda change later, with no range change. |
| R18 | "LFO Division" display name is not in BRIEF (RESEARCH A1). | Host automation name only; rename before v1.0 if wanted. |
