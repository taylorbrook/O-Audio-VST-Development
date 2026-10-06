# Stage 1 — Foundation / Shell: RESEARCH

**Plugin:** O-simpleWavetable
**Stage:** 1 of 4 — Foundation / Shell
**Phase:** research ✓
**Date:** 2026-10-05
**Input:** `stages/1-foundation/CONTEXT.md` (resolved decisions 1–6, locked conventions), `parameter-spec.md` (21 params, locked), `research/ARCHITECTURE.md`, `ROADMAP.md` §Stage 1, `mockups/v1-CMakeLists.txt`, `mockups/v1-PluginEditor.{h,cpp}`, `mockups/v1-integration-checklist.md`
**Method:** every `[VERIFIED: path:line]` fact was read this session from the sibling sources, the local JUCE **8.0.15** tree (`/Users/taylorbrook/JUCE/modules`, `juce_core.h:47` `version: 8.0.15`), repo scripts, or the memory notes. `[ASSUMED]` marks what could not be read or run (no build was run, per brief).
**Confidence:** HIGH. Zero novel technology; one new combination (UTF-8 choice strings plus a strip-on-load state child), both with in-repo precedent.

---

## 1. Summary

**Template: O-simpleAdditive Stage 1** (commit `88073650`): synth CMake with WebView2 flags preset, full APVTS, a `GenericAudioProcessorEditor` wrapper under the final editor class name, plain APVTS-XML state. Graft three pieces onto it:
- the current O-simpleAdditive `Synthesiser` + 16 voices + `MidiMessageCollector` + `uiLanguage` codec (`plugins/O-simpleAdditive/Source/PluginProcessor.{h,cpp}`). Its Stage 1 commit had none of these.
- O-simpleGrain's `ouaricon_add_module(... webview-drop-streaming)` line and its child-tree state shape.
- O-Strata's `#if JUCE_WEB_BROWSER` guard on `createEditor()` and its `CharPointer_UTF8` choice strings.

O-simpleSubtractive and O-simpleFM add nothing Stage 1 needs. They have no `-inf` text function and no child state; their bend stub matches the Additive pattern.

**Top 5 pitfalls (each is a defect the build, auval and pluginval do not catch):**

1. **`bank` choice strings contain U+2192 "→".** `juce::String (const char*)` is ASCII-only and mangles the arrow silently. Build "Sine → Saw" and "Sine → Square" with `juce::CharPointer_UTF8 ("Sine \xE2\x86\x92 Saw")`. Plain literals mangle the host automation names *and* the Stage 3 `<select>`, which reads `properties.choices`.
2. **`IMPORTED_BANK` stale child.** `APVTS::replaceState` is a wholesale `state = newState` `[VERIFIED: juce_audio_processors/utilities/juce_AudioProcessorValueTreeState.cpp:396-404]`, so an incoming child lands in the live tree and `copyState()` re-emits it. Detach the child **before** `replaceState`, and strip any copy from `copyState()` in `getStateInformation`. This is the preset-manager `<CustomState>` bug in another form.
3. **`uiLanguage` loses its type through XML.** Persist the *code string* and gate the restore on `isVoid()`, never on `isInt()`.
4. **A new plugin folder is not in the ninja graph.** The root `file(GLOB plugins/*)` runs only at configure time, and `build-and-install.sh` skips configure when `build/` exists. Run `cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release` once. Do **not** use `--reconfigure`: it `rm -rf`s the build dir every other session shares. The script also never builds the Standalone.
5. **`auval -a` SIGABRTs on this machine.** Use `auval -v aumu OSiW OuDv` (or `scripts/verify-au-link.sh O-simpleWavetable`). The first call after the script's cache clear is a cold registry rescan, so run it in the background.

---

## 2. User Constraints (from CONTEXT.md, verbatim)

### Locked conventions
- `PLUGIN_CODE OSiW` (sibling family: OSiA Additive, OSiF FM, OSiS Subtractive; checked unique). Manufacturer code and company name are the suite's `OUARICON_*` variables. `PRODUCT_NAME "O-simpleWavetable${OUARICON_DEV_SUFFIX}"`.
- Class names: `OSimpleWavetableAudioProcessor` / `OSimpleWavetableAudioProcessorEditor`. The editor name must match the mockup template `v1-PluginEditor.{h,cpp}` so Stage 3 can drop it in.
- `ouaricon_add_module(O-simpleWavetable webview-drop-streaming)` as in O-simpleGrain (it copies the JS at configure time; the copied file is gitignored).
- Link juce_audio_utils, juce_audio_formats, juce_dsp, juce_gui_extra (plus the juce_audio_basics, juce_audio_processors and juce_core they pull in).
- Ranges: plain `NormalisableRange<float>{start, end, interval, skew}` only. **Never a lambda range**: the WebView slider frontend cannot see one. `output_level` text function shows `"-inf"` at ≤ −60 dB. `bit_depth` index 0 text is "Full".
- `setLatencySamples (0)` in `prepareToPlay` (`getLatencySamples` is non-virtual in JUCE 8).
- No param ID shadows a `juce::` free function. Every Choice has ≥ 2 entries.
- Voice/sound classes: `WtVoice` (16, silent `renderNextBlock`) and `WtSound` (applies to all notes and channels). Pitch-wheel plumbing can be stubbed.

### Out of scope for Stage 1
- Any DSP: banks, mips, oscillator, envelopes, LFO, crossfader, importer, reaper (Stage 2).
- WebView UI, relays, attachments, binary data, viz timer (Stage 3). The CMake flags are set, but `juce_add_binary_data` for the UI tree can wait until Stage 3.
- Presets, CPU work, Windows build (Stage 4).

Resolved decisions 1–6 (silence on empty Imported, ±2 st bend, mid-rise quantizer, reject imports under 2048 samples, `lfo_div` default 1/1, per-frame normalization with a +24 dB cap) shape Stage 2 DSP. Stage 1 only declares the parameters that carry them.

---

## 3. CMake (area 1)

### 3.1 Sibling blocks to copy

| Item | Source | Verbatim |
|---|---|---|
| Branding vars | `CMakeLists.txt:23-33` (root) `[VERIFIED]` | release: `OuAu`, `""`; dev (every local build): `set(OUARICON_COMPANY_NAME "Ouaricon Audio Development")`, `set(OUARICON_MANUFACTURER_CODE OuDv)`, `set(OUARICON_DEV_SUFFIX "-dev")` |
| `juce_add_plugin` args | `plugins/O-simpleAdditive/CMakeLists.txt:11-25` `[VERIFIED]` | `COMPANY_NAME "${OUARICON_COMPANY_NAME}"` · `PLUGIN_MANUFACTURER_CODE ${OUARICON_MANUFACTURER_CODE}` · `FORMATS VST3 AU Standalone` · `IS_SYNTH TRUE` · `NEEDS_MIDI_INPUT TRUE` · `NEEDS_MIDI_OUTPUT FALSE` · `IS_MIDI_EFFECT FALSE` · `NEEDS_WEB_BROWSER TRUE` · `NEEDS_WEBVIEW2 TRUE` · `EDITOR_WANTS_KEYBOARD_FOCUS FALSE` |
| Stage 1 version | `git show 88073650:plugins/O-simpleAdditive/CMakeLists.txt` `[VERIFIED]` | `VERSION "0.1.0"`. The keyword must be `VERSION`; `PLUGIN_VERSION` is silently ignored and ships 1.0.0 (memory `critical_plugin_version_keyword_ignored_by_juce`). |
| Link list | `plugins/O-simpleAdditive/CMakeLists.txt:65-85` `[VERIFIED]` | 13 `juce::` modules (below) + the three `juce_recommended_*` flags PUBLIC |
| Compile defs | `plugins/O-simpleAdditive/CMakeLists.txt:95-101` `[VERIFIED]` | `JUCE_VST3_CAN_REPLACE_VST2=0` `JUCE_WEB_BROWSER=1` `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` `JUCE_USE_CURL=0` (PUBLIC) |
| Drop module | `plugins/O-simpleGrain/CMakeLists.txt:67` `[VERIFIED]` | `ouaricon_add_module(O-simpleGrain webview-drop-streaming)` |

**`ouaricon_add_module` at configure time** `[VERIFIED: modules/cmake/OuariconModules.cmake:30-145]`:
- It resolves `modules/core/webview-drop-streaming/` (`module.yaml` present).
- It globs `cpp/*.cpp|*.h` into `target_sources` and adds `cpp/` to the include path. That is `DropSessionGuard.h` and `WebViewDropStreaming.h`, both headers, so nothing compiles.
- It copies `js/webview-drop-streaming.js` into `${CMAKE_CURRENT_SOURCE_DIR}/Source/ui/public/modules/` with `configure_file … COPYONLY` (lines 104-117), creating the directory.
- There is no `module.cmake` hook.
- Requirements: the target must already exist (call it after `juce_add_plugin`), and `include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)` must come first.
- **Gitignore is already in place:** `.gitignore:26` `plugins/*/Source/ui/public/modules/`. `git check-ignore` confirms `plugins/O-simpleWavetable/Source/ui/public/modules/webview-drop-streaming.js` is ignored. Do not add a plugin-local `.gitignore`, and never commit that directory.

**Mockup compatibility** `[VERIFIED: mockups/v1-CMakeLists.txt:11-19,28,59-73]`. The v1 snippet's §1 flags, §2 module line, §4 `juce_gui_extra` link and §5 defines all match the block below, so Stage 3 only adds `juce_add_binary_data(O-simpleWavetable_UIResources …)` and links it. `juce_generate_juce_header` must stay because the mockup editor includes `<JuceHeader.h>` (`v1-PluginEditor.h:46`).

### 3.2 Ready-to-write `plugins/O-simpleWavetable/CMakeLists.txt`

```cmake
cmake_minimum_required(VERSION 3.15)

include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)

# O-simpleWavetable - Pedagogical 16-voice Wavetable Synthesizer
# Stage 1 (Foundation): silent synth shell, full 21-param APVTS, state + IMPORTED_BANK stub.
# WebView2 flags are set here so Stage 3 (GUI) needs no CMake rework.
juce_add_plugin(O-simpleWavetable
    COMPANY_NAME "${OUARICON_COMPANY_NAME}"
    PLUGIN_MANUFACTURER_CODE ${OUARICON_MANUFACTURER_CODE}
    PLUGIN_CODE OSiW
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "O-simpleWavetable${OUARICON_DEV_SUFFIX}"
    VERSION "0.1.0"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    NEEDS_WEB_BROWSER TRUE
    NEEDS_WEBVIEW2 TRUE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
)

target_sources(O-simpleWavetable
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
        Source/PluginEditor.cpp
        Source/PluginEditor.h
        Source/WtVoice.h
)

target_include_directories(O-simpleWavetable PRIVATE Source)

# Copies webview-drop-streaming.js into Source/ui/public/modules/ (gitignored)
# and puts the header-only C++ on the include path. Consumed in Stage 2.4/3.
ouaricon_add_module(O-simpleWavetable webview-drop-streaming)

target_link_libraries(O-simpleWavetable
    PRIVATE
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_plugin_client
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_core
        juce::juce_data_structures
        juce::juce_dsp
        juce::juce_events
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# JUCE 8: MUST come after target_link_libraries
juce_generate_juce_header(O-simpleWavetable)

# JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1 is mandatory with NEEDS_WEBVIEW2 TRUE
# (else Windows shows a blank UI).
target_compile_definitions(O-simpleWavetable
    PUBLIC
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=1
        JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
        JUCE_USE_CURL=0
)

# Test targets: off by default. Suite-wide option name, deliberately.
option(OUARICON_BUILD_TESTS "Build O-simpleWavetable test targets" OFF)
if(OUARICON_BUILD_TESTS AND APPLE)
    include(${CMAKE_SOURCE_DIR}/scripts/param-dump/ParamDump.cmake)
    ouaricon_add_param_dump(O-simpleWavetable ${CMAKE_CURRENT_SOURCE_DIR}/Source)
    ouaricon_add_processor_console(O-simpleWavetable ${CMAKE_CURRENT_SOURCE_DIR}/Source
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/state-check/main.cpp state-check)
endif()
```

- `PLUGIN_CODE OSiW` is unique. A case-insensitive scan of every `plugins/*/CMakeLists.txt` found 0 hits for `osiw` `[VERIFIED: grep]`.
- The target equals the folder name, so `build-and-install.sh`'s `resolve_cmake_target` needs no fallback (memory `build_script_target_name_vs_folder`).
- The test block follows the O-Strata wiring `[VERIFIED: plugins/O-Strata/CMakeLists.txt:120-135]` and `scripts/param-dump/ParamDump.cmake:67` (`ouaricon_add_processor_console(plugin_target plugin_source_dir main_cpp suffix)`). The helper derives every `JucePlugin_*` macro from the plugin target, compiles only non-editor TUs, and sets `JUCE_WEB_BROWSER=0` (ParamDump.cmake:150-205).
- Do **not** edit `modules/registry.yaml` `used_by` in Stage 1. It lies outside the path scope, and `scripts/regen-registry-used-by.sh` derives it.

---

## 4. Parameter layout (area 2)

### 4.1 Ready-to-implement table (spec order = layout order = host order)

Source: `parameter-spec.md:17-39` `[VERIFIED]`. Names come from BRIEF.md:52-73 ("Band-limiting", "Mod Env …"). "LFO Division" is not in the brief and is proposed `[ASSUMED]`. Interval is **0 (continuous)** unless listed (`parameter-spec.md:41`). All IDs use `juce::ParameterID { id, 1 }`.

| # | String ID | C++ ident | Type | Range `{start,end,interval,skew}` / choices | Default | Name | Text |
|---|---|---|---|---|---|---|---|
| 1 | `bank` | `bank` | Choice | `Sine → Saw`, `Sine → Square`, `Pulse Width`, `Formant`, `Drive`, `Imported` | 0 | Bank | choice string (UTF-8!) |
| 2 | `position` | `position` | Float | `{0, 1, 0, 1}` | 0.0 | Position | `fixed(3)` |
| 3 | `interp` | `interp` | Bool | — | true | Interpolation | JUCE default "On"/"Off" |
| 4 | `bandlimit` | `bandlimit` | Bool | — | true | Band-limiting | "On"/"Off" |
| 5 | `bit_depth` | `bitDepth` | Choice | `Full`,`16`,`15`,`14`,`13`,`12`,`11`,`10`,`9`,`8`,`7`,`6`,`5`,`4`,`3` (15) | 0 | Bit Depth | idx 0 → "Full" (choice string) |
| 6 | `lfo_rate` | `lfoRate` | Float | `{0.01, 20, 0, 0.3}` | 0.5 | LFO Rate | `fixed(2)`, label "Hz" |
| 7 | `lfo_sync` | `lfoSync` | Choice | `Free`,`Tempo` | 0 | LFO Sync | choice |
| 8 | `lfo_div` | `lfoDiv` | Choice | `4 bars`,`2 bars`,`1/1`,`1/2`,`1/4`,`1/8`,`1/16`,`1/32`,`1/2.`,`1/4.`,`1/8.`,`1/16.`,`1/2T`,`1/4T`,`1/8T`,`1/16T` (16) | **2** | LFO Division | choice |
| 9 | `lfo_shape` | `lfoShape` | Choice | `Sine`,`Triangle`,`Saw`,`Square`,`S&H` | **1** | LFO Shape | choice |
| 10 | `lfo_depth` | `lfoDepth` | Float | `{0, 1, 0, 1}` | 0.0 | LFO Depth | `fixed(3)` |
| 11 | `menv_attack` | `menvAttack` | Float | `{0.001, 10, 0, 0.3}` | 0.5 | Mod Env Attack | `fixed(3)`, "s" |
| 12 | `menv_decay` | `menvDecay` | Float | `{0.001, 10, 0, 0.3}` | 1.0 | Mod Env Decay | `fixed(3)`, "s" |
| 13 | `menv_sustain` | `menvSustain` | Float | `{0, 1, 0, 1}` | 0.0 | Mod Env Sustain | `fixed(3)` |
| 14 | `menv_release` | `menvRelease` | Float | `{0.001, 10, 0, 0.3}` | 0.5 | Mod Env Release | `fixed(3)`, "s" |
| 15 | `env_amount` | `envAmount` | Float | `{-1, 1, 0, 1}` | 0.0 | Env Amount | `fixed(3)` |
| 16 | `amp_attack` | `ampAttack` | Float | `{0.001, 5, 0, 0.35}` | 0.005 | Amp Attack | `fixed(3)`, "s" |
| 17 | `amp_decay` | `ampDecay` | Float | `{0.001, 5, 0, 0.35}` | 0.3 | Amp Decay | `fixed(3)`, "s" |
| 18 | `amp_sustain` | `ampSustain` | Float | `{0, 1, 0, 1}` | 0.8 | Amp Sustain | `fixed(3)` |
| 19 | `amp_release` | `ampRelease` | Float | `{0.001, 5, 0, 0.35}` | 0.2 | Amp Release | `fixed(3)`, "s" |
| 20 | `voice_mode` | `voiceMode` | Choice | `Poly`,`Mono` | 0 | Voice Mode | choice |
| 21 | `output_level` | `outputLevel` | Float | `{-60, 6, 0.1, 1}` | -6.0 | Output Level | `"-inf"` at the floor, else 1 dp; "dB" |

Counts: 13 Float, 6 Choice, 2 Bool = 21. `jassert (params.size() == 21)`. ROADMAP's 22 is the documented slip (`parameter-spec.md:13`).

**Why the `fixed(n)` text functions (recommended, not contractual):** with interval 0, JUCE's default float text prints **7 decimals** ("0.5000000") `[VERIFIED: juce_audio_processors_headless/utilities/juce_AudioParameterFloat.cpp:51-77, int numDecimalPlaces = 7 at :54]`. They change host/generic-editor text only. The range, normalisation and WebView values do not change, because the page reads `getScaledValue()` (`parameter-spec.md:41`). They are **text** lambdas, not **range** lambdas, so the "no lambda range" rule does not apply.

### 4.2 Traps, checked

| Trap | Status for this set |
|---|---|
| Lambda `NormalisableRange` invisible to the WebView slider (memory `critical_lambda_normalisable_range_invisible_to_webview_slider_frontend`) | All 13 floats use the 4-arg `{start,end,interval,skew}` ctor. `AudioParameterChoice` builds its own internal lambda range (`juce_AudioParameterChoice.cpp:47-52`); that is JUCE's own and fine, because combos bind through `getComboBoxState`. |
| Choice ≥ 2 entries | 6/6/15/2/16/5/2 entries. JUCE asserts `choices.size() > 1` `[VERIFIED: juce_AudioParameterChoice.cpp:63]`. Copy O-Strata's `jassert (choices.size() >= 2)` guard if a `choice` lambda is used. |
| `bit_depth` index 0 = "Full" | Comes straight from the choice string. No custom text function is needed (`stringFromIndexFunction` defaults to `choices[index]`, `juce_AudioParameterChoice.cpp:58-60`). |
| `output_level` "-inf" | Custom `withStringFromValueFunction` **plus** `withValueFromStringFunction`. The default parser is `text.getFloatValue()` `[VERIFIED: juce_AudioParameterFloat.cpp:80-81]`, which cannot read "-inf" back. Snap tolerance: interval 0.1 makes the floor exactly −60.0f; compare `db <= -59.95f`. |
| Param ID shadowing `juce::` free functions | Scanned `~/JUCE/modules/**/*.h` for a free-function or type declaration named each of the 21 C++ identifiers: **0 hits**. Negative control: the same regex finds `juce::end` at `juce_core/files/juce_RangedDirectoryIterator.h:195`. Keep snake_case strings and use camelCase C++ identifiers. Avoid `begin`/`end` forever (memory `critical_paramid_shadows_juce_free_function`). |
| Class names shadowing juce types | `WtVoice`/`WtSound` have no `juce::` counterpart (scan: 0 hits). Never use `Synthesiser*`, `Sampler*` or `Oscillator` (memory `critical_class_name_shadows_juce_type`). |
| Bool XML round trip | `interp`/`bandlimit` are **parameters**, restored as floats by APVTS. The type-loss trap does not apply to them, only to hand-added properties (§5). |
| ParameterID version hint | Use `{id, 1}` everywhere, as every sibling does (`O-simpleAdditive/Source/PluginProcessor.cpp:70-136`). With AU built, JUCE `jassertfalse`s once if any hint is 0 `[VERIFIED: juce_audio_processors_headless/processors/juce_AudioProcessor.cpp:448-452]`. A parameter added after v1.0 gets hint 2+. |
| Non-ASCII in `juce::String(const char*)` | **Applies to `bank`**, see the snippet. Every other choice and name is ASCII. "S&H" and "1/2." are ASCII. |
| MSVC C3493 (`constexpr` local used in a lambda) | Inside layout or text lambdas, use `static constexpr`, or capture by value as `fixed(int)` does (memory `critical_msvc_constexpr_lambda_capture`). |

### 4.3 Code (PluginProcessor.h ParamIDs + PluginProcessor.cpp layout)

```cpp
// PluginProcessor.h
namespace OSimpleWavetable::ParamIDs
{
    inline constexpr auto bank        = "bank";
    inline constexpr auto position    = "position";
    inline constexpr auto interp      = "interp";
    inline constexpr auto bandlimit   = "bandlimit";
    inline constexpr auto bitDepth    = "bit_depth";
    inline constexpr auto lfoRate     = "lfo_rate";
    inline constexpr auto lfoSync     = "lfo_sync";
    inline constexpr auto lfoDiv      = "lfo_div";
    inline constexpr auto lfoShape    = "lfo_shape";
    inline constexpr auto lfoDepth    = "lfo_depth";
    inline constexpr auto menvAttack  = "menv_attack";
    inline constexpr auto menvDecay   = "menv_decay";
    inline constexpr auto menvSustain = "menv_sustain";
    inline constexpr auto menvRelease = "menv_release";
    inline constexpr auto envAmount   = "env_amount";
    inline constexpr auto ampAttack   = "amp_attack";
    inline constexpr auto ampDecay    = "amp_decay";
    inline constexpr auto ampSustain  = "amp_sustain";
    inline constexpr auto ampRelease  = "amp_release";
    inline constexpr auto voiceMode   = "voice_mode";
    inline constexpr auto outputLevel = "output_level";

    // Spec order — single source of truth for the count/order gate (and Stage 3 loops).
    inline constexpr std::array<const char*, 21> all {
        bank, position, interp, bandlimit, bitDepth, lfoRate, lfoSync, lfoDiv, lfoShape,
        lfoDepth, menvAttack, menvDecay, menvSustain, menvRelease, envAmount,
        ampAttack, ampDecay, ampSustain, ampRelease, voiceMode, outputLevel };
}
```

```cpp
// PluginProcessor.cpp
namespace
{
    using Range = juce::NormalisableRange<float>;   // plain {start,end,interval,skew} ONLY

    Range unitRange()     { return { 0.0f,   1.0f, 0.0f, 1.0f  }; }
    Range bipolarRange()  { return { -1.0f,  1.0f, 0.0f, 1.0f  }; }
    Range menvTimeRange() { return { 0.001f, 10.0f, 0.0f, 0.3f  }; }
    Range ampTimeRange()  { return { 0.001f, 5.0f,  0.0f, 0.35f }; }

    juce::AudioParameterFloatAttributes fixed (int decimals, const juce::String& label = {})
    {
        return juce::AudioParameterFloatAttributes()
            .withLabel (label)
            .withStringFromValueFunction ([decimals] (float v, int maxLen)
            {
                juce::String s (v, decimals);
                return maxLen > 0 ? s.substring (0, maxLen) : s;
            });
    }

    juce::AudioParameterFloatAttributes outputLevelAttributes()
    {
        return juce::AudioParameterFloatAttributes()
            .withLabel ("dB")
            .withStringFromValueFunction ([] (float db, int maxLen)
            {
                const juce::String s = db <= -59.95f ? juce::String ("-inf") : juce::String (db, 1);
                return maxLen > 0 ? s.substring (0, maxLen) : s;
            })
            .withValueFromStringFunction ([] (const juce::String& text)
            {
                const auto t = text.trim();
                return t.startsWithIgnoreCase ("-inf") ? -60.0f
                                                       : juce::jlimit (-60.0f, 6.0f, t.getFloatValue());
            });
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout
OSimpleWavetableAudioProcessor::createParameterLayout()
{
    using namespace OSimpleWavetable::ParamIDs;
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    auto addFloat = [&params] (const char* id, const char* name, Range r, float def,
                               juce::AudioParameterFloatAttributes a)
    {
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { id, 1 }, name, r, def, std::move (a)));
    };
    auto addChoice = [&params] (const char* id, const char* name, juce::StringArray c, int def)
    {
        jassert (c.size() >= 2);   // critical_choice_param_needs_two_choices
        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { id, 1 }, name, std::move (c), def));
    };
    auto addBool = [&params] (const char* id, const char* name, bool def)
    {
        params.push_back (std::make_unique<juce::AudioParameterBool> (
            juce::ParameterID { id, 1 }, name, def));
    };

    // U+2192 via hex escapes: juce::String(const char*) is ASCII-only
    // (critical_juce_string_char_ctor_is_ascii_only). The source stays ASCII.
    addChoice (bank, "Bank", { juce::String (juce::CharPointer_UTF8 ("Sine \xE2\x86\x92 Saw")),
                               juce::String (juce::CharPointer_UTF8 ("Sine \xE2\x86\x92 Square")),
                               "Pulse Width", "Formant", "Drive", "Imported" }, 0);
    addFloat  (position,  "Position", unitRange(), 0.0f, fixed (3));
    addBool   (interp,    "Interpolation", true);
    addBool   (bandlimit, "Band-limiting", true);
    addChoice (bitDepth,  "Bit Depth", { "Full", "16", "15", "14", "13", "12", "11", "10",
                                         "9", "8", "7", "6", "5", "4", "3" }, 0);
    addFloat  (lfoRate,   "LFO Rate", { 0.01f, 20.0f, 0.0f, 0.3f }, 0.5f, fixed (2, "Hz"));
    addChoice (lfoSync,   "LFO Sync", { "Free", "Tempo" }, 0);
    addChoice (lfoDiv,    "LFO Division", { "4 bars", "2 bars", "1/1", "1/2", "1/4", "1/8", "1/16", "1/32",
                                            "1/2.", "1/4.", "1/8.", "1/16.", "1/2T", "1/4T", "1/8T", "1/16T" }, 2);
    addChoice (lfoShape,  "LFO Shape", { "Sine", "Triangle", "Saw", "Square", "S&H" }, 1);
    addFloat  (lfoDepth,  "LFO Depth", unitRange(), 0.0f, fixed (3));
    addFloat  (menvAttack,  "Mod Env Attack",  menvTimeRange(), 0.5f, fixed (3, "s"));
    addFloat  (menvDecay,   "Mod Env Decay",   menvTimeRange(), 1.0f, fixed (3, "s"));
    addFloat  (menvSustain, "Mod Env Sustain", unitRange(),     0.0f, fixed (3));
    addFloat  (menvRelease, "Mod Env Release", menvTimeRange(), 0.5f, fixed (3, "s"));
    addFloat  (envAmount,   "Env Amount",      bipolarRange(),  0.0f, fixed (3));
    addFloat  (ampAttack,   "Amp Attack",  ampTimeRange(), 0.005f, fixed (3, "s"));
    addFloat  (ampDecay,    "Amp Decay",   ampTimeRange(), 0.3f,   fixed (3, "s"));
    addFloat  (ampSustain,  "Amp Sustain", unitRange(),    0.8f,   fixed (3));
    addFloat  (ampRelease,  "Amp Release", ampTimeRange(), 0.2f,   fixed (3, "s"));
    addChoice (voiceMode,   "Voice Mode", { "Poly", "Mono" }, 0);
    addFloat  (outputLevel, "Output Level", { -60.0f, 6.0f, 0.1f, 1.0f }, -6.0f, outputLevelAttributes());

    jassert (params.size() == all.size());   // == 21 (ROADMAP's 22 is the documented slip)
    return { params.begin(), params.end() };
}
```

Notes:
- The `bank` initializer mixes `juce::String` and `const char*`. `std::initializer_list<const char*>` is not viable for that mix, so it resolves to the variadic `StringArray (StringRef, OtherElements&&...)` ctor `[VERIFIED: juce_core/text/juce_StringArray.h:64]`. The remaining literals are ASCII, so their `String(const char*)` conversion is safe.
- In `"\xE2\x86\x92 Saw"` the escape is followed by a space, which is not a hex digit, so the escape terminates correctly. Keep a space or another non-hex character after every `\x..` escape.
- The `AudioParameterFloat (ParameterID, String, NormalisableRange<float>, float, const AudioParameterFloatAttributes&)` signature is `[VERIFIED: juce_audio_processors_headless/utilities/juce_AudioParameterFloat.h:76-80]`. `withStringFromValueFunction` and `withValueFromStringFunction` take `std::function<String(float,int)>` and `std::function<float(const String&)>` `[VERIFIED: juce_RangedAudioParameter.h:59-66]`. The Bool ctor `(ParameterID, String, bool, attrs = {})` is at `juce_AudioParameterBool.h:74-77`, and the Choice ctor `(ParameterID, String, const StringArray&, int, attrs = {})` at `juce_AudioParameterChoice.h:75-79`.

---

## 5. State save/load (area 3)

### 5.1 What siblings do
- **O-simpleAdditive** `[VERIFIED: Source/PluginProcessor.cpp:473-511]`:
  - Save: `copyState()`, then `state.setProperty ("uiLanguage", languageCode (uiLanguage.load (acquire)), nullptr)`, then `copyXmlToBinary`.
  - Restore: `getXmlFromBinary`, check `hasTagName (parameters.state.getType())`, `replaceState (ValueTree::fromXml)`, then `if (! lang.isVoid()) uiLanguage.store (languageIndex (lang.toString()), release)`.
  - The codec is `languageCode(int)`/`languageIndex(String)` at `PluginProcessor.h:190-191`, with `std::atomic<int> uiLanguage { 0 }` at `:181`. The mockup editor calls exactly these names (`v1-PluginEditor.cpp:34-36, 275, 286`).
- **O-simpleGrain** `[VERIFIED: Source/PluginProcessor.cpp:1152-1215]`:
  - Save: `state.getOrCreateChildWithName (kSourceStateTag)` on the **copy**, so the APVTS tree is untouched.
  - Restore: read the child and `uiLanguage` from the **incoming** tree *before* `apvts.replaceState (state)`, and **leave the child in the tree**. That is harmless for Grain only because `getOrCreate…` reuses the one child rather than appending a second.

### 5.2 Do the two memory traps apply? Yes, both.

| Memory | Applies? | Why / action |
|---|---|---|
| `critical_valuetree_xml_roundtrip_loses_type` | **Yes**: `uiLanguage`, plus `IMPORTED_BANK`'s `version`/`numFrames` in Stage 2.4 | Every hand-added property comes back as a **String var**. Persist `uiLanguage` as the code ("en"/"fr"/"zh-Hans") and gate on `isVoid()` (Additive pattern). Parse `numFrames`/`version` with `.toString().getIntValue()` after an `isVoid()` check (`parameter-spec.md:212` says so). `interp`/`bandlimit` are parameters and are unaffected. |
| `critical_preset_manager_stale_customstate_child` | **Yes, by the same mechanism** | `replaceState` is `state = newState` (`juce_AudioProcessorValueTreeState.cpp:400`), so a restored `IMPORTED_BANK` stays in `parameters.state`. `copyState()` then re-emits it. Once Stage 2.4 *appends* a fresh child, a save carries **two**, and the next restore's `getChildWithName` reads the **stale first** one, which is exactly the O-Bowed `<CustomState>` defect. In Stage 1 alone, failing to strip violates "Stage 1 writes nothing" because the restored blob is re-saved. **Action:** detach every `IMPORTED_BANK` child from the incoming tree *before* `replaceState` (the live tree never holds the ~1 MB blob), *and* strip it from the `copyState()` copy in `getStateInformation`. The child must be (re)written only from processor-owned data. |
| Absent child on load | — | `replaceState` is wholesale, so the **tree** cannot keep a stale child from a previous load. The **processor-side** bank is a separate object: Stage 2.4 must retire it when the child is absent (ARCHITECTURE §State Persistence). Stage 1 has no bank, so the stub ignores this. |

### 5.3 Code (Stage 1; the seams Stage 2.4 fills are marked)

```cpp
// PluginProcessor.h (private)
static constexpr const char* kImportedBankTag = "IMPORTED_BANK";
static constexpr const char* kUiLanguageProp  = "uiLanguage";
void restoreImportedBank (const juce::ValueTree& childOrInvalid);   // Stage 1: no-op stub
void writeImportedBank   (juce::ValueTree& state) const;            // Stage 1: writes nothing

static void stripImportedBank (juce::ValueTree& tree)
{
    for (auto c = tree.getChildWithName (kImportedBankTag); c.isValid();
              c = tree.getChildWithName (kImportedBankTag))
        tree.removeChild (c, nullptr);           // ALL copies: a hand-edited/legacy blob may hold two
}
```

```cpp
void OSimpleWavetableAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    stripImportedBank (state);                    // never re-emit a child that rode in on replaceState
    state.setProperty (kUiLanguageProp,
                       languageCode (uiLanguage.load (std::memory_order_acquire)), nullptr);
    writeImportedBank (state);                    // Stage 1: no bank exists → nothing appended
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void OSimpleWavetableAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary (data, sizeInBytes);
    if (xml == nullptr) return;

    auto state = juce::ValueTree::fromXml (*xml);
    if (! state.isValid() || ! state.hasType (parameters.state.getType())) return;

    // Hand-added property: comes back as a STRING var → isVoid() is the only honest gate.
    const juce::var lang = state.getProperty (kUiLanguageProp);
    if (! lang.isVoid())
        uiLanguage.store (languageIndex (lang.toString()), std::memory_order_release);

    // Detach BEFORE replaceState so the live APVTS tree never carries the blob
    // (stale-child trap, critical_preset_manager_stale_customstate_child).
    const auto bankChild = state.getChildWithName (kImportedBankTag).createCopy();  // invalid if absent
    stripImportedBank (state);

    parameters.replaceState (state);
    restoreImportedBank (bankChild);   // Stage 1: tolerate present/absent, publish nothing.
                                       // Stage 2.4: decode → build mips → publish; absent → retire (never free here).
}
```

- `ValueTree::getChildWithName`, `removeChild (const ValueTree&, UndoManager*)` and `appendChild` are all present `[VERIFIED: juce_data_structures/values/juce_ValueTree.h:309,342,348]`. `createCopy()` on an invalid tree returns an invalid tree `[ASSUMED]`. If in doubt, guard with `isValid()` first.
- `IMPORTED_BANK` property names for Stage 2.4, verbatim from `parameter-spec.md:212` `[VERIFIED]`: `version`, `filename`, `numFrames` (parse with `getIntValue()`), `encoding` = `flac16` (fallback `pcm16gz`), `data` = base64. Stage 1 neither reads nor writes them.
- `getStateInformation` may run on any thread. Stage 1 touches only `copyState()` (which takes the APVTS lock) and an atomic. Stage 2.4 adds the CriticalSection-guarded cached blob (ROADMAP Implementation Notes).

---

## 6. Synth plumbing (area 4)

Source pattern: `plugins/O-simpleAdditive/Source/PluginProcessor.cpp:146-217, 275-290, 361-376` and `.h:113-259` `[VERIFIED]`, minus everything DSP.

### 6.1 Processor skeleton

```cpp
// PluginProcessor.h — includes
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>   // MidiMessageCollector (juce_audio_devices)
#include <array>
#include <atomic>
#include "WtVoice.h"

class OSimpleWavetableAudioProcessor : public juce::AudioProcessor
{
public:
    // … standard overrides as O-simpleAdditive .h:116-145 …
    const juce::String getName() const override { return "O-simpleWavetable"; }
    bool acceptsMidi() const override  { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 5.0; }   // amp_release max

    juce::AudioProcessorValueTreeState& getAPVTS() { return parameters; }   // mockup contract
    void handleUiMidi (int noteNumber, bool noteOn, float velocity);         // mockup contract

    std::atomic<int> uiLanguage { 0 };                                       // mockup contract
    static juce::String languageCode  (int i)                 { return i == 2 ? "zh-Hans" : i == 1 ? "fr" : "en"; }
    static int          languageIndex (const juce::String& s) { return s == "zh-Hans" ? 2 : s == "fr" ? 1 : 0; }

private:
    juce::AudioProcessorValueTreeState parameters;   // declared BEFORE synth/collector (ctor init order)
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    static constexpr int kNumVoices = 16;
    juce::Synthesiser synth;
    juce::MidiMessageCollector midiCollector;
    // + §5.3 state helpers
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OSimpleWavetableAudioProcessor)
};
```

```cpp
OSimpleWavetableAudioProcessor::OSimpleWavetableAudioProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "PARAMETERS", createParameterLayout())
{
    for (int i = 0; i < kNumVoices; ++i)
        synth.addVoice (new WtVoice());            // all allocation here, never on the audio thread
    synth.addSound (new WtSound());
    synth.setNoteStealingEnabled (true);
    midiCollector.reset (44100.0);                 // valid base before the first prepareToPlay (sibling)
}

void OSimpleWavetableAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    midiCollector.reset (sampleRate);
    synth.setCurrentPlaybackSampleRate (sampleRate);
    for (int v = 0; v < synth.getNumVoices(); ++v)              // SynthesiserVoice has no virtual prepare
        if (auto* wv = dynamic_cast<WtVoice*> (synth.getVoice (v)))
            wv->prepareToPlay (sampleRate, samplesPerBlock);
    setLatencySamples (0);                                      // getLatencySamples() is non-virtual (JUCE 8)
}

void OSimpleWavetableAudioProcessor::releaseResources() { synth.allNotesOff (0, false); }

bool OSimpleWavetableAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo()) return false;
    return layouts.getMainInputChannelSet().isDisabled();      // instrument: no input bus
}

void OSimpleWavetableAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();
    buffer.clear();                                            // silent shell; Stage 2 voices ADD into it
    if (numSamples <= 0) return;                               // collector jasserts numSamples > 0
    midiCollector.removeNextBlockOfMessages (midi, numSamples);
    synth.renderNextBlock (buffer, midi, 0, numSamples);       // consumes MIDI; WtVoice renders nothing
}
```

- `handleUiMidi`: copy verbatim from `O-simpleAdditive/Source/PluginProcessor.cpp:361-376`. It clamps the note to 0..127 with `jlimit`, catches a NaN velocity, and stamps `setTimeStamp (Time::getMillisecondCounterHiRes() * 0.001)`. `addMessageToQueue` jasserts a non-zero timestamp and that `reset()` was called `[VERIFIED: juce_audio_devices/midi_io/juce_MidiMessageCollector.cpp:66,89,92 + addMessageToQueue body]`.
- **Stage 2 note:** ARCHITECTURE §Processing Order wants `blockEntries++` first and `blockGeneration++` on **every** return path. The `numSamples <= 0` early return is one of them, so Stage 2.4 must not miss it.
- Mono routing (`renderMonoLegato`) and the per-block parameter push are Stage 2. In Stage 1, `voice_mode` is declared only.
- Not RT-perfect, but accepted by the siblings: `MidiMessageCollector` takes a CriticalSection on the audio thread, and `MidiBuffer::addEvent` may grow the host buffer. Both are the same as O-simpleAdditive.

### 6.2 `Source/WtVoice.h` (silent; Stage 2 fills it in)

```cpp
#pragma once
#include <juce_audio_basics/juce_audio_basics.h>

struct WtSound final : public juce::SynthesiserSound
{
    bool appliesToNote (int) override    { return true; }
    bool appliesToChannel (int) override { return true; }
};

class WtVoice final : public juce::SynthesiserVoice
{
public:
    void prepareToPlay (double sampleRate, int /*maxBlock*/) { sr = sampleRate; }   // non-virtual; dynamic_cast dispatch

    bool canPlaySound (juce::SynthesiserSound* s) override { return dynamic_cast<WtSound*> (s) != nullptr; }
    void startNote (int, float, juce::SynthesiserSound*, int currentPitchWheelPosition) override
    {
        pitchWheelPos = currentPitchWheelPosition;   // SEED the wheel at note-on (restrike memory)
    }
    void stopNote (float, bool) override { clearCurrentNote(); }   // no envelope yet → free at once
    void pitchWheelMoved (int v) override { pitchWheelPos = v; }   // Stage 2: ±2 st → mip level
    void controllerMoved (int, int) override {}
    void renderNextBlock (juce::AudioBuffer<float>&, int, int) override {}   // silent until Stage 2

private:
    double sr = 44100.0;
    int pitchWheelPos = 8192;   // centre
};
```

- The pure virtuals are `appliesToNote`, `appliesToChannel`, `canPlaySound`, `startNote`, `stopNote`, `pitchWheelMoved`, `controllerMoved` and `renderNextBlock(float)` `[VERIFIED: juce_audio_basics/synthesisers/juce_Synthesiser.h:69,76,129,137,154,165,170,200]`. The double overload is non-pure (`:203`).
- Stage 2 bend: `bendSemis = (pitchWheelPos - 8192) / 8192 * 2` (O-simpleSubtractive `SubVoice.h:360-361`). Also mirror its processor-side forwarding of pitch-wheel in the Mono path (`PluginProcessor.cpp:401-404`).
- Do **not** use `getCurrentlyPlayingNote() >= 0` as a "voice was idle" test in Stage 2. `Synthesiser::startVoice` sets the note before `startNote` (memory `critical_synthesiser_startvoice_sets_note_before_startnote`).

### 6.3 Editor shell (generic, under the final class name)

Copy the O-simpleAdditive Stage 1 wrapper (`git show 88073650:plugins/O-simpleAdditive/Source/PluginEditor.{h,cpp}`) `[VERIFIED]`, renamed:
- `OSimpleWavetableAudioProcessorEditor : public juce::AudioProcessorEditor`
- members `OSimpleWavetableAudioProcessor& processorRef;` and `juce::GenericAudioProcessorEditor genericEditor;`
- ctor: `addAndMakeVisible (genericEditor); setSize (420, 640);`
- `resized()`: `genericEditor.setBounds (getLocalBounds())`

The generic editor scrolls its parameter list in a `Viewport`, so 21 rows fit any size. Stage 3 replaces both files with `v1-PluginEditor.{h,cpp}`, which already use these class names (`v1-PluginEditor.h:49-53`).

`createEditor()` must be guarded so the console test targets (`JUCE_WEB_BROWSER=0`, editor TU excluded) still link `[VERIFIED: plugins/O-Strata/Source/PluginProcessor.cpp:1552-1563]`:

```cpp
#if JUCE_WEB_BROWSER
 #include "PluginEditor.h"            // NOT at the top of the file (pattern_render_harness_breaks_on_webview_editor)
#endif
juce::AudioProcessorEditor* OSimpleWavetableAudioProcessor::createEditor()
{
#if JUCE_WEB_BROWSER
    return new OSimpleWavetableAudioProcessorEditor (*this);
#else
    return new juce::GenericAudioProcessorEditor (*this);   // console-target build
#endif
}
```

---

## 7. Validation (area 5)

### 7.1 Build and install (repo root; every long call `run_in_background: true` with a scratch log, per memory `pattern_executor_watchdog_stall_run_long_commands_in_background`)

| Step | Command | Why |
|---|---|---|
| 1. Register the new folder | `cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release` | The root `file(GLOB plugins/*)` (`CMakeLists.txt:47-57`, no `CONFIGURE_DEPENDS`) only sees the folder on a configure. `build/build.ninja` has 0 `O-simpleWavetable` references today. `build-and-install.sh` skips configure when `build/` exists (`scripts/build-and-install.sh:261-271`). **Never** pass `--reconfigure` (`rm -rf build`, shared). If a *foreign* plugin's CMakeLists breaks the configure, stop and report. Do not set the `SKIP_PLUGINS` **cache** var in the shared `build/`, because it persists for every session. |
| 2. Build + install VST3/AU | `./scripts/build-and-install.sh O-simpleWavetable` | Builds `_VST3 _AU` only (`:280`), sweeps the `-dev`/unsuffixed variants, clears the AU cache, installs. |
| 3. Standalone | `ninja -C build O-simpleWavetable_Standalone` | The script never builds it (memory `pattern_build_install_skips_standalone_stale_ui`). Success criterion 1 needs all three. |
| 4. Version stamp | `/usr/libexec/PlistBuddy -c "Print AudioComponents:0:version" ~/Library/Audio/Plug-Ins/Components/O-simpleWavetable-dev.component/Contents/Info.plist` → **256** (0x000100 = 0.1.0) | Proves the `VERSION` keyword took (`critical_plugin_version_keyword_ignored_by_juce`). |
| 5. auval (targeted) | `bash scripts/verify-au-link.sh O-simpleWavetable` (runs `auval -v aumu OSiW OuDv`; type from `IS_SYNTH TRUE`, manufacturer from the root's last `set(OUARICON_MANUFACTURER_CODE …)`) `[VERIFIED: scripts/verify-au-link.sh:92-95,122-141]`. Negative control: `auval -v aumu ZZZZ OuDv` → "didn't find". | `auval -a` SIGABRTs here (NI AU + TCC). The first call after step 2's `killall AudioComponentRegistrar` is a cold rescan (83 s to 17 min observed), so background it. Expect "21 Global Scope Parameters". |
| 6. pluginval | `/Applications/pluginval.app/Contents/MacOS/pluginval --strictness-level 10 --timeout-ms 600000 --validate ~/Library/Audio/Plug-Ins/VST3/O-simpleWavetable-dev.vst3`, then the same on `…/Components/O-simpleWavetable-dev.component` | Strictness 10 is the CI release gate and fuzzes params for NaN (memory `pattern_ci_pluginval10_catches_latent_nan`). pluginval 1.0.4 is installed `[VERIFIED]`. The battery form adds `--skip-gui-tests --output-dir` (`scripts/verify-suite-battery.sh:171-173`), but keep GUI tests on here because the generic editor must open. |
| 7. DAW | Load as an **instrument** (Logic: AU instrument slot; Reaper/Live: VST3). Play MIDI: silence expected. Check the generic editor's 21 rows in spec order. Save, close, reopen. | Success criteria 3–5 (host side). |

**Logic per-version AU I/O cache** (memory `critical_logic_caches_au_io_config_per_version`): Logic keys the I/O config on identity **and VERSION**. The Stage 1 bus layout (one stereo/mono output, no input, no sidechain) is final for the plugin's life, so the cache is harmless. **If** a later stage ever changes buses, bump `VERSION`; clearing caches does not help.

### 7.2 Probes the host tools cannot see (recommended, in-tree helpers)

Neither auval nor pluginval reloads a state and checks a *hand-added* property, and the stale-child defect needs **two** reopens. Both memories call for a two-processor probe. `uiLanguage` cannot even be set in Stage 1 (no UI), so criterion 5's `uiLanguage` half is **only** testable this way.

Build the two console targets in an **out-of-repo** dir. That keeps the shared `build/` cache's `OUARICON_BUILD_TESTS=OFF` (`build/CMakeCache.txt:631`) and `SKIP_PLUGINS` untouched, and copies CI's isolation trick (`.github/workflows/ci-tests.yml:159-171`). Run it in bash, not zsh:

```bash
SKIP=$(for d in plugins/*/; do n=$(basename "$d"); [ -f "${d}CMakeLists.txt" ] && [ "$n" != O-simpleWavetable ] && printf '%s;' "$n"; done)
cmake -S . -B "$SCRATCH/build-oswt" -G Ninja -DCMAKE_BUILD_TYPE=Release -DOUARICON_BUILD_TESTS=ON "-DSKIP_PLUGINS=$SKIP"
cmake --build "$SCRATCH/build-oswt" --target O-simpleWavetable-param-dump O-simpleWavetable-state-check
find "$SCRATCH/build-oswt" -name 'O-simpleWavetable-param-dump' -perm +111 -type f   # then run it
```

| Req | Behavior | Probe | Command |
|---|---|---|---|
| Param contract | 21 rows, spec order, IDs, `defaultText` (bank "Sine → Saw", bit_depth "Full", output "-6.0"), `textAtMin` output "-inf", lfo_div default "1/1", lfo_shape "Triangle", interp/bandlimit "On" | param-dump TSV (`scripts/param-dump/README.md` columns id/name/numSteps/textAtMin/textAtMax/defaultNorm/defaultText). Check with `grep -vc '^#'` = 21 and a diff against a hand-written expected TSV from §4.1. | `O-simpleWavetable-param-dump > $SCRATCH/params.tsv` |
| UTF-8 | `bank` choice 0's bytes contain `E2 86 92` | `xxd $SCRATCH/params.tsv \| grep -c 'e286 92'` ≥ 1 (or the state-check asserts `toUTF8()`) | — |
| State P1 | `uiLanguage` 2 → save → fresh instance load → 2. Same for 1. | `tests/state-check/main.cpp` (pattern: `plugins/O-simpleSampler/tests/render-harness/main.cpp:704-768`) | `O-simpleWavetable-state-check` exit 0 |
| State P2 | A blob **without** `uiLanguage` leaves the default (0) standing | same; strip the attribute from the XML, `copyXmlToBinary`, load | same |
| State P3 | A blob **with** an `IMPORTED_BANK` child (fake `version`/`filename`/`numFrames`/`encoding`/`data`) loads without assert; afterwards `getAPVTS().state` has **0** such children | same | same |
| State P4 | Two-reopen: load(P3 blob) → save → load → save: each saved XML has **0** `IMPORTED_BANK` children (Stage 1). Stage 2.4 flips the assertion to exactly **1**. | same | same |
| State P5 | Non-default `bank`=5, `interp`=false, `bit_depth`=9, `output_level`=-60 round-trip through `setValueNotifyingHost` → save → fresh load | same | same |
| Shell P6 | `prepareToPlay(48000, 512)`, then `processBlock` with a note-on + pitch-wheel event: output all 0.0f, no NaN. A 0-sample block does not assert. | same | same |

**Wave 0 gap:** `tests/state-check/main.cpp` does not exist yet. Create it with the CMake block in §3.2. The driver constructs `OSimpleWavetableAudioProcessor` directly (the helper puts `Source/` on the include path), runs under `juce::ScopedJuceInitialiser_GUI`, and calls `setPlayConfigDetails (0, 2, fs, 512)` before `prepareToPlay` (sampler harness pattern).

---

## 8. Pitfalls relevant to a Stage 1 synth shell (area 6)

| # | Source | How to apply in Stage 1 |
|---|---|---|
| 1 | `critical_juce_string_char_ctor_is_ascii_only` | `bank` arrow via `CharPointer_UTF8` hex escapes. Grep the new sources for non-ASCII inside string literals before the first build: `LC_ALL=C grep -n '[^ -~]' Source/*.{h,cpp}` should return 0 rows, and comments must stay ASCII too. |
| 2 | `critical_preset_manager_stale_customstate_child` | Strip `IMPORTED_BANK` before `replaceState` and from `copyState()` (§5). Probe P4. |
| 3 | `critical_valuetree_xml_roundtrip_loses_type` | `uiLanguage` as a code string with an `isVoid()` gate. Probes P1/P2. |
| 4 | `critical_lambda_normalisable_range_invisible_to_webview_slider_frontend` | Only 4-arg `NormalisableRange` ctors. Text lambdas are fine. |
| 5 | `critical_choice_param_needs_two_choices` | `jassert (c.size() >= 2)` in `addChoice`. |
| 6 | `critical_paramid_shadows_juce_free_function` / `critical_class_name_shadows_juce_type` | Scan done (0 hits). Keep camelCase C++ idents; `WtVoice`/`WtSound` are collision-free. |
| 7 | `critical_plugin_version_keyword_ignored_by_juce` | `VERSION "0.1.0"`, verified via PlistBuddy = 256. |
| 8 | JUCE 8 `getLatencySamples()` non-virtual (MEMORY.md) | `setLatencySamples (0)` in `prepareToPlay`; never override. |
| 9 | `juce8-critical-patterns.md` §4, §9, §22 | `IS_SYNTH TRUE` + `NEEDS_MIDI_INPUT TRUE` + output-only `BusesProperties` (an input bus on an instrument gives "missing input"). `NEEDS_WEB_BROWSER TRUE` is already set for Stage 3. |
| 10 | `pattern_render_harness_breaks_on_webview_editor` | `#if JUCE_WEB_BROWSER` guard on the editor include and on `createEditor()` now, so Stage 3's WebView editor does not break the console targets. |
| 11 | `build_script_target_name_vs_folder` | Target = folder (`O-simpleWavetable`), so no resolver fallback. Keep it that way, because the mockup CMake and checklist assume `O-simpleWavetable_UIResources`. |
| 12 | `pattern_build_install_skips_standalone_stale_ui` | Build `_Standalone` explicitly (§7.1 step 3). |
| 13 | `pattern_auval_a_sigabrt_native_instruments_tcc` + `pattern_cold_auval_after_install_rescans_registry` | Targeted `auval -v aumu OSiW OuDv`, backgrounded, with the log polled. |
| 14 | `pattern_executor_watchdog_stall_run_long_commands_in_background` | Configure, build, auval and pluginval each run in their own background call, logging to scratch. |
| 15 | `pattern_gate_0_to_1_always_needs_force` | `run-gate.sh … 0-ideation 1-foundation` always BLOCKS (no CMakeLists yet). Use `--force --skip-review` with that justification and check `.planning/gate-bypasses.log`. |
| 16 | `pattern_gain_ramp_without_seeding_fades_in_from_silence` | Stage 1 has **no** output gain stage (defer to Stage 2's Output Stage). When Stage 2 adds the `SmoothedValue`, seed it from `output_level` in `prepareToPlay` (Additive `:183-186`). |
| 17 | MSVC `critical_msvc_constexpr_lambda_capture`, `critical_msvc_c3615_uninitialised_constexpr_local`, `critical_msvc_safepointer_init_capture_nested_lambda` | Windows is Stage 4, but write Stage 1 MSVC-clean: no bare `constexpr` locals used inside lambdas (`static constexpr`), and no uninitialised locals in `constexpr` functions. |
| 18 | `critical_binary_data_strips_hyphens` / `critical_dual_binary_data_namespace_collision` | **Stage 3 only.** Recorded so nobody adds `juce_add_binary_data` now. The v1 snippet already plans the default namespace plus `webviewdropstreaming_js`. |
| 19 | Shared checkout (CLAUDE.md, `index_git_shared_checkout`) | All Stage 1 files are **new**, and a pathspec commit takes tracked files only, so `git add` them first. `PLUGINS.md` is currently **foreign-staged** (`MM` in `git status`), so use the temp-index commit and patch only this plugin's row. Row format after Stage 1 (Additive precedent, `git show 88073650 -- PLUGINS.md`): `\| O-simpleWavetable \| 🚧 Stage 1 \| 0.1.0 \| Synth (Pedagogical Wavetable) \| <date> \|`. |

**Not relevant to this stage:** AU sidechain silence, Logic surround formats, channel-set bitset traps (mono/stereo out only), oversampling latency, WebView2 runtime gotchas (Stage 3), the preset-manager module (none planned; lesson presets are C++ snapshots), and ASan hangs (Stage 2 lifetime gates).

---

## 9. Recommended task order (each independently checkable)

1. `CMakeLists.txt` (§3.2) + `Source/WtVoice.h` (§6.2) + `PluginProcessor.{h,cpp}` (§4.3, §5.3, §6.1) + `PluginEditor.{h,cpp}` generic wrapper (§6.3).
   **Check:** step 1's configure lists the target, and `ninja -C build O-simpleWavetable_VST3 O-simpleWavetable_AU O-simpleWavetable_Standalone` builds clean.
2. `tests/state-check/main.cpp` + the out-of-repo test build.
   **Check:** param-dump TSV = 21 rows matching §4.1, and state-check P1–P6 exit 0.
3. Install + validation (§7.1 steps 2, 4–6).
   **Check:** auval PASS, pluginval PASS (VST3 + AU), Info.plist version 256.
4. DAW smoke (§7.1 step 7), then STATUS.md / PLUGINS.md row, then a path-scoped commit.

---

## 10. Project Constraints (from CLAUDE.md)

- Build and install via `./scripts/build-and-install.sh O-simpleWavetable`. It clears the AU cache and sweeps both `-dev` and unsuffixed bundles.
- Verify the AU on this machine with targeted `auval -v`. CLAUDE.md's `auval -a | grep` is unsatisfiable here (memory).
- Work trunk-based on `main`, with no branches or worktrees. Path-scope commits to `plugins/O-simpleWavetable` and `PLUGINS.md`. Never `git add -A` or `commit -a`. Re-check the branch and `git status --short` immediately before committing.
- Research docs belong in `research/`. This stage file lives in `.planning/stages/1-foundation/` per workflow.
- After the stage's last phase, give the handoff: Step 1 `/clear`, Step 2 the exact next phase command, then STOP.

## 11. Environment Availability

| Dependency | Needed by | Available | Version |
|---|---|---|---|
| JUCE | everything | ✓ `/Users/taylorbrook/JUCE` | 8.0.15 (matches `.github/juce-version.txt`) |
| CMake / Ninja | build | ✓ | 4.2.1 / 1.13.2 |
| Xcode | build | ✓ | 26.3 (macOS 26.6.2) |
| auval | AU validation | ✓ `/usr/bin/auval` | system (targeted form only) |
| pluginval | validation | ✓ `/Applications/pluginval.app` | 1.0.4 |
| `build/` (Ninja, Release) | build | ✓ | exists; needs one re-configure to see the new folder |

No external packages are installed in this phase (no npm/pip/crates), so no package-legitimacy audit applies.

## 12. Security Domain (brief)

| ASVS | Applies | Control |
|---|---|---|
| V5 Input validation | yes | `setStateInformation` parses an untrusted blob: invalid XML or the wrong root type → return; IDs come via APVTS; `uiLanguage` goes through the `languageIndex()` allow-list. `handleUiMidi` clamps the note and catches a NaN velocity. In Stage 2.4, never trust `numFrames` from the blob (clamp to ≤ 256 and verify against the decoded length). |
| V2/V3/V4/V6 | no | No auth, sessions, access control or crypto in a local plugin. |

## 13. Assumptions Log

| # | Claim | Section | Risk if wrong |
|---|---|---|---|
| A1 | The display name "LFO Division" (not in BRIEF) | §4.1 | Cosmetic. It is the host automation name; rename before v1.0 if desired. |
| A2 | `ValueTree::createCopy()` on an invalid tree returns an invalid tree | §5.3 | Low. Guard with `isValid()` to remove the doubt. |
| A3 | The `fixed(n)` decimals (3 for s/unit, 2 for Hz) | §4.1 | Cosmetic, host text only. Not part of the contract. |
| A4 | No build was run (per brief), so build-clean is predicted, not observed | all | Low. Every API was read in JUCE 8.0.15 source. |

## 14. Open items for plan phase

- None blocking. One planner call: whether `tests/state-check` lands in Stage 1, which this research **recommends**. It is the only way to verify `uiLanguage` restore and the two-reopen `IMPORTED_BANK` behaviour before Stage 2.4 depends on it.

## 15. Sources

- **Repo (read this session):** `plugins/O-simpleAdditive/{CMakeLists.txt, Source/PluginProcessor.{h,cpp}}` + commit `88073650` (Stage 1 shell); `plugins/O-simpleGrain/{CMakeLists.txt:59-142, Source/PluginProcessor.cpp:1152-1270}`; `plugins/O-simpleSubtractive/Source/{PluginProcessor.cpp, SubVoice.h}`; `plugins/O-Strata/{CMakeLists.txt:120-135, Source/PluginProcessor.cpp:108-135,1552-1563}`; `plugins/O-simpleSampler/tests/render-harness/main.cpp:704-768`; `modules/cmake/OuariconModules.cmake`; `scripts/{build-and-install.sh, verify-au-link.sh, verify-suite-battery.sh, param-dump/*}`; `.gitignore:18-26`; root `CMakeLists.txt`; `troubleshooting/patterns/juce8-critical-patterns.md` §4/§18/§22; `.github/workflows/{ui-static-gates.yml, ci-tests.yml}`.
- **JUCE 8.0.15 (local):** `juce_audio_processors_headless/utilities/juce_AudioParameter{Float,Choice,Bool}.{h,cpp}`, `juce_RangedAudioParameter.h`, `juce_AudioProcessorParameterWithID.h`, `processors/juce_AudioProcessor.cpp:440-452`; `juce_audio_processors/utilities/juce_AudioProcessorValueTreeState.cpp:396-404`; `juce_audio_devices/midi_io/juce_MidiMessageCollector.{h,cpp}`; `juce_audio_basics/synthesisers/juce_Synthesiser.h`; `juce_core/text/juce_StringArray.h`; `juce_data_structures/values/juce_ValueTree.h`.
- **Memory:** the notes named in §8.

## Metadata

- Standard stack: HIGH (verbatim sibling blocks, local JUCE source).
- Architecture: HIGH (an Additive shell plus pieces from two other siblings, each cited).
- Pitfalls: HIGH (each tied to a memory note or a JUCE source line).
- Valid until: JUCE pin change, or a change to `OuariconModules.cmake` / `build-and-install.sh`.
