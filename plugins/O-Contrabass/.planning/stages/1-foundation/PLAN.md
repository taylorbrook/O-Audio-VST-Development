# Stage 1: Foundation — Plan

**Date:** 2026-04-26
**Plugin:** O-Contrabass
**Stage:** 1 of 4 (Foundation)
**Mode:** Synthesis from locked Stage 0 contracts + Stage 1 RESEARCH.md
**Source contracts:**
- `BRIEF.md` (sha256:6ea840bb…)
- `parameter-spec.md` (sha256:c47fe736…)
- `research/ARCHITECTURE.md` (sha256:3cb26814…)
- `ROADMAP.md` (sha256:106639f6…)
- `stages/1-foundation/CONTEXT.md`, `stages/1-foundation/RESEARCH.md`

---

## Goal

Stand up a buildable, host-loadable JUCE 8 plugin shell for O-Contrabass: `CMakeLists.txt`, `PluginProcessor.{h,cpp}` with all 29 APVTS parameters from `parameter-spec.md`, and a minimal `PluginEditor.{h,cpp}` placeholder. The plugin must build VST3 + AU on macOS, register with `auval`, pass `pluginval` strictness 10 in bypass mode, and load in five target DAWs (Logic, Ableton, Reaper, Dorico, Cubase) with all 29 parameters visible in automation menus. **No DSP yet** — Stage 2 territory. **No WebView UI yet** — Stage 3 territory.

---

## Tasks

### Task 1 — Create `CMakeLists.txt`

- **Files:** `plugins/O-Contrabass/CMakeLists.txt` (new)
- **Depends on:** none
- **Plugin auto-discovery:** root `CMakeLists.txt:72-82` globs `plugins/*` and calls `add_subdirectory()` automatically — no root edit needed.

**Required content (per RESEARCH.md §1):**

```cmake
juce_add_plugin(O-Contrabass
    COMPANY_NAME "${OUARICON_COMPANY_NAME}"
    PLUGIN_MANUFACTURER_CODE ${OUARICON_MANUFACTURER_CODE}
    PLUGIN_CODE OCbs                       # locked unique code, RESEARCH.md §1.2
    FORMATS VST3 AU Standalone             # Standalone for cheap audition
    PRODUCT_NAME "O-Contrabass${OUARICON_DEV_SUFFIX}"
    IS_SYNTH TRUE                          # juce8-critical-patterns #22
    NEEDS_MIDI_INPUT TRUE                  # juce8-critical-patterns #22
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    NEEDS_WEB_BROWSER TRUE
    NEEDS_WEBVIEW2 TRUE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
)

target_sources(O-Contrabass PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    # scala-tuning-engine — Pattern B (explicit file refs, sibling-plugin convention)
    ${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/cpp/TuningEngine.cpp
    ${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/cpp/ScaleGenerator.cpp
    ${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/cpp/EmbeddedTunings.cpp
    ${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/cpp/TuningExporter.cpp
)

target_include_directories(O-Contrabass PRIVATE
    Source
    ${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/cpp
)

target_compile_definitions(O-Contrabass
    PUBLIC
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=1
        JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1   # MEMORY: required when NEEDS_WEBVIEW2 TRUE
        JUCE_USE_CURL=0
)

target_link_libraries(O-Contrabass
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

# note-expression — Pattern A (per-format VST3 routing required)
include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)
ouaricon_add_module(O-Contrabass note-expression)

# CRITICAL ORDER: header generation MUST come AFTER target_link_libraries
juce_generate_juce_header(O-Contrabass)
```

**Acceptance:** CMake configure step succeeds with no warnings; the `O-Contrabass_VST3`, `O-Contrabass_AU`, `O-Contrabass_Standalone` targets are visible.

---

### Task 2 — Create `PluginProcessor.h`

- **Files:** `plugins/O-Contrabass/Source/PluginProcessor.h` (new)
- **Depends on:** Task 1

**Required content:**

```cpp
#pragma once
#include <JuceHeader.h>

class OContrabassAudioProcessor : public juce::AudioProcessor
{
public:
    OContrabassAudioProcessor();
    ~OContrabassAudioProcessor() override = default;

    // AudioProcessor overrides
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override                            { return true; }

    const juce::String getName() const override                { return JucePlugin_Name; }
    bool acceptsMidi() const override                          { return true; }
    bool producesMidi() const override                         { return false; }
    bool isMidiEffect() const override                         { return false; }
    double getTailLengthSeconds() const override               { return 0.0; }

    int getNumPrograms() override                              { return 1; }
    int getCurrentProgram() override                           { return 0; }
    void setCurrentProgram(int) override                       {}
    const juce::String getProgramName(int) override            { return {}; }
    void changeProgramName(int, const juce::String&) override  {}

    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    // APVTS — public so editor (and Stage 2 voice) can attach.
    juce::AudioProcessorValueTreeState parameters;

    // DO NOT declare getLatencySamples() — it is non-virtual in JUCE 8.
    // Use setLatencySamples(N) inside prepareToPlay (RESEARCH.md §2.4).

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OContrabassAudioProcessor)
};
```

**Acceptance:** Compiles standalone; no `getLatencySamples()` override anywhere in the header (memory file constraint).

---

### Task 3 — Implement `createParameterLayout()` — 29 parameters

- **Files:** `plugins/O-Contrabass/Source/PluginProcessor.cpp` (new — partial)
- **Depends on:** Task 2
- **Source of truth:** `parameter-spec.md` (sha256:c47fe736…). Parameter IDs are UPPER_SNAKE_CASE verbatim. Any deviation breaks the contract checksum.

**29 parameters in 8 sections — exact spec:**

| # | ID | Type | Range | Default | Skew |
|---|---|---|---|---|---|
| 1 | `BOW_SPEED` | Float | 0.02 – 1.5 m/s | 0.15 | 0.5 (log-ish low) |
| 2 | `BOW_PRESSURE` | Float | 0.05 – 8.0 N | 1.0 | 0.5 |
| 3 | `BOW_POSITION` | Float | 0.02 – 0.25 | 0.10 | 1.0 (linear) |
| 4 | `BRIGHTNESS` | Float | 80.0 – 12000.0 Hz | 4500.0 | 0.25 (heavy log low) |
| 5 | `OUTPUT_GAIN` | Float | -60.0 – 12.0 dB | 0.0 | 1.0 |
| 6 | `ROSIN` | Float | 0.0 – 1.0 | 0.65 | 1.0 |
| 7 | `BOW_NOISE` | Float | 0.0 – 1.0 | 0.35 | 1.0 |
| 8 | `BODY_SIZE` | Float | 0.0 – 1.0 | 0.75 | 1.0 |
| 9 | `BODY_DAMPING` | Float | 0.0 – 1.0 | 0.40 | 1.0 |
| 10 | `BODY_MIX` | Float | 0.0 – 1.0 | 0.80 | 1.0 |
| 11 | `STRING_TENSION` | Float | 0.0 – 1.0 | 0.50 | 1.0 |
| 12 | `STRING_STIFFNESS` | Float | 0.0 – 1.0 | 0.30 | 1.0 |
| 13 | `ACTIVE_STRINGS` | Int | 1 – 4 | 4 | n/a |
| 14 | `DETUNE_E` | Float | -1200.0 – 1200.0 cents | 0.0 | 1.0 |
| 15 | `DETUNE_A` | Float | -1200.0 – 1200.0 cents | 0.0 | 1.0 |
| 16 | `DETUNE_D` | Float | -1200.0 – 1200.0 cents | 0.0 | 1.0 |
| 17 | `DETUNE_G` | Float | -1200.0 – 1200.0 cents | 0.0 | 1.0 |
| 18 | `VIBRATO_RATE` | Float | 0.1 – 12.0 Hz | 5.0 | 1.0 |
| 19 | `VIBRATO_DEPTH` | Float | 0.0 – 50.0 cents | 12.0 | 1.0 |
| 20 | `VIBRATO_ONSET` | Float | 0.0 – 3000.0 ms | 600.0 | 0.5 |
| 21 | `SLOW_LFO_RATE` | Float | 0.05 – 2.0 Hz | 0.3 | 1.0 |
| 22 | `SLOW_LFO_DEPTH` | Float | 0.0 – 1.0 | 0.0 | 1.0 |
| 23 | `EXPRESSION_MACRO` | Float | 0.0 – 1.0 | 0.50 | 1.0 |
| 24 | `INFINITE_SUSTAIN` | Float | 0.0 – 1.0 | 0.0 | 1.0 |
| 25 | `SUB_HARMONICS` | Float | 0.0 – 1.0 | 0.0 | 1.0 |
| 26 | `WIDTH` | Float | 0.0 – 2.0 | 1.0 | 1.0 |
| 27 | `REFERENCE_PITCH` | Float | 220.0 – 880.0 Hz | 440.0 | 1.0 |
| 28 | `TUNING_SYSTEM` | Choice | {Scala/TUN, MTS-ESP, 12-TET} | 2 (12-TET) | n/a |
| 29 | `NOTE_EXPRESSION` | Bool | false / true | true | n/a |

**Implementation pattern (per-parameter, all use `juce::ParameterID { "ID", 1 }`):**

```cpp
juce::AudioProcessorValueTreeState::ParameterLayout
OContrabassAudioProcessor::createParameterLayout()
{
    using APF = juce::AudioParameterFloat;
    using API = juce::AudioParameterInt;
    using APC = juce::AudioParameterChoice;
    using APB = juce::AudioParameterBool;
    using NR  = juce::NormalisableRange<float>;
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // -- Tier 1 Primary
    layout.add(std::make_unique<APF>(juce::ParameterID{"BOW_SPEED",1},     "Bow Speed",
        NR(0.02f, 1.5f, 0.001f, 0.5f),  0.15f));
    layout.add(std::make_unique<APF>(juce::ParameterID{"BOW_PRESSURE",1},  "Bow Pressure",
        NR(0.05f, 8.0f, 0.01f, 0.5f),   1.0f));
    layout.add(std::make_unique<APF>(juce::ParameterID{"BOW_POSITION",1},  "Bow Position",
        NR(0.02f, 0.25f, 0.001f),        0.10f));
    layout.add(std::make_unique<APF>(juce::ParameterID{"BRIGHTNESS",1},    "Brightness",
        NR(80.0f, 12000.0f, 1.0f, 0.25f), 4500.0f));
    layout.add(std::make_unique<APF>(juce::ParameterID{"OUTPUT_GAIN",1},   "Output Level",
        NR(-60.0f, 12.0f, 0.1f),          0.0f));
    // ... continue for all 29 (Tier 2, String Config, Detune, Expression, Drone, Output, Microtonal)

    layout.add(std::make_unique<APC>(juce::ParameterID{"TUNING_SYSTEM",1}, "Tuning System",
        juce::StringArray { "Scala/TUN", "MTS-ESP", "12-TET" }, 2));
    layout.add(std::make_unique<APB>(juce::ParameterID{"NOTE_EXPRESSION",1},"Note Expression", true));

    return layout;
}
```

**Acceptance:** All 29 parameters present; IDs match spec table verbatim; defaults match spec; ranges match spec.

---

### Task 4 — Implement `PluginProcessor.cpp` shell (constructor, prepareToPlay, processBlock, state)

- **Files:** `plugins/O-Contrabass/Source/PluginProcessor.cpp` (continued from Task 3)
- **Depends on:** Tasks 2, 3

**Required pieces (per RESEARCH.md §2):**

```cpp
// Constructor — output-only BusesProperties (synth pattern, juce8-critical-patterns #22)
OContrabassAudioProcessor::OContrabassAudioProcessor()
    : AudioProcessor(BusesProperties()
                       .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "Parameters", createParameterLayout())
{
}

bool OContrabassAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

void OContrabassAudioProcessor::prepareToPlay(double /*sampleRate*/, int /*samplesPerBlock*/)
{
    // Stage 1: bypass-mode plugin reports zero latency. Stage 2 will set non-zero
    // once the voice (which owns the oversampler) is wired up. RESEARCH.md §2.3.
    setLatencySamples(0);
}

void OContrabassAudioProcessor::releaseResources() {}

void OContrabassAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                             juce::MidiBuffer& /*midi*/)
{
    juce::ScopedNoDenormals noDenormals;

    for (auto i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // Stage 1 = silent output. DSP arrives in Stage 2.
    buffer.clear();
}

void OContrabassAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    if (auto xml = parameters.copyState().createXml())
        copyXmlToBinary(*xml, destData);
}

void OContrabassAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
        parameters.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessorEditor* OContrabassAudioProcessor::createEditor()
{
    return new OContrabassAudioProcessorEditor(*this);
}

// JUCE plugin entry point
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OContrabassAudioProcessor();
}
```

**Acceptance:**
- Constructor compiles with output-only `BusesProperties` (no `withInput` — synth contract).
- `prepareToPlay` calls `setLatencySamples(0)` exactly once, no oversampler allocation.
- `processBlock` clears all channels under `juce::ScopedNoDenormals`.
- State save/restore round-trips through APVTS XML.
- `createPluginFilter()` returns a fresh instance (required by JUCE plugin client).

---

### Task 5 — Create `PluginEditor.{h,cpp}` (minimal stub)

- **Files:**
  - `plugins/O-Contrabass/Source/PluginEditor.h` (new)
  - `plugins/O-Contrabass/Source/PluginEditor.cpp` (new)
- **Depends on:** Task 2

**Required content (per RESEARCH.md §3):**

`PluginEditor.h`:
```cpp
#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class OContrabassAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit OContrabassAudioProcessorEditor(OContrabassAudioProcessor&);
    ~OContrabassAudioProcessorEditor() override = default;

    void paint(juce::Graphics&) override;
    void resized() override {}

private:
    OContrabassAudioProcessor& processorRef;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OContrabassAudioProcessorEditor)
};
```

`PluginEditor.cpp`:
```cpp
#include "PluginEditor.h"

OContrabassAudioProcessorEditor::OContrabassAudioProcessorEditor(OContrabassAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    setSize(600, 400);
}

void OContrabassAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black);
    g.setColour(juce::Colours::white);
    g.setFont(18.0f);
    g.drawText("O-Contrabass — Stage 1 (Foundation)",
               getLocalBounds(), juce::Justification::centred);
}
```

**Acceptance:** Editor opens in DAW; shows the title text on a black background. No WebView, no relays, no resource provider — those are Stage 3.

---

### Task 6 — macOS build verification

- **Depends on:** Tasks 1–5
- **No file changes** — verification only.

**Steps:**
```bash
# Configure (only required if CMakeCache is missing or root CMakeLists changed)
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release

# Build all three formats
ninja -C build O-Contrabass_VST3 O-Contrabass_AU O-Contrabass_Standalone
```

**Acceptance:**
- All three targets build clean with no warnings (warning-as-error from `juce_recommended_warning_flags`).
- Artefacts present at:
  - `build/plugins/O-Contrabass/O-Contrabass_artefacts/Release/VST3/O-Contrabass.vst3`
  - `build/plugins/O-Contrabass/O-Contrabass_artefacts/Release/AU/O-Contrabass.component`
  - `build/plugins/O-Contrabass/O-Contrabass_artefacts/Release/Standalone/O-Contrabass.app`

---

### Task 7 — Install + AU registration verification (macOS, per CLAUDE.md protocol)

- **Depends on:** Task 6
- **No file changes** — verification only.

**Steps:**
```bash
# MANDATORY cache clear (from CLAUDE.md)
killall -9 AudioComponentRegistrar 2>/dev/null || true
rm -rf ~/Library/Caches/AudioUnitCache/
rm -rf ~/Library/Caches/com.apple.audiounits.cache

# Remove old + install fresh
rm -rf ~/Library/Audio/Plug-Ins/VST3/O-Contrabass-dev.vst3
rm -rf ~/Library/Audio/Plug-Ins/Components/O-Contrabass-dev.component
cp -R build/plugins/O-Contrabass/O-Contrabass_artefacts/Release/VST3/O-Contrabass-dev.vst3 \
      ~/Library/Audio/Plug-Ins/VST3/
cp -R build/plugins/O-Contrabass/O-Contrabass_artefacts/Release/AU/O-Contrabass-dev.component \
      ~/Library/Audio/Plug-Ins/Components/

# Verify AU registration
auval -a | grep -i contrabass
```

**Acceptance:**
- `auval -a` lists `O-Contrabass-dev` (or similar) under `aumu` (Music Device / instrument component type).
- `auval -v aumu OCbs OuDv` (or whatever the registered triplet is) returns "AU VALIDATION SUCCEEDED".

---

### Task 8 — pluginval strictness 10

- **Depends on:** Task 6
- **No file changes** — verification only.

**Steps:**
```bash
pluginval --strictness-level 10 --validate-in-process \
  build/plugins/O-Contrabass/O-Contrabass_artefacts/Release/VST3/O-Contrabass-dev.vst3
```

**Acceptance:** Exit code 0. Bypass-mode plugin (silent output) is expected — pluginval's parameter / state / threading / latency / buses checks must still pass.

**Watch-fors (per RESEARCH.md §7):**
- Param ID stability across `getStateInformation` round-trip.
- `setStateInformation` graceful on malformed XML (`juce::ValueTree::fromXml` returns invalid tree → `replaceState` no-ops).
- No threading violations from `prepareToPlay` / `releaseResources` calls.

---

### Task 9 — DAW load smoke test (5 hosts)

- **Depends on:** Task 7
- **No file changes** — manual verification.

For each host, load the plugin on an instrument track and confirm:
1. Plugin instantiates without error.
2. Editor window opens (shows the Stage 1 placeholder text).
3. Automation menu lists all 29 parameter names (verify count + names match `parameter-spec.md`).
4. State save/restore: save preset, modify params, restore — values match.

**Hosts (per ROADMAP exit gate):**
- Logic Pro (AU)
- Ableton Live (VST3)
- Reaper (VST3)
- Dorico (VST3 + Note Expression visibility — flag enables surfacing the Note Expression API)
- Cubase (VST3)

**Acceptance:** All five hosts load + show editor + show 29 automatable parameters.

**Gotchas (RESEARCH.md §7):**
- Logic 11 caches AU validation aggressively — the cache-clear protocol in Task 7 is mandatory before each rebuild test.
- If Dorico doesn't surface Note Expression: re-run `scripts/apply-juce-patches.sh` (JUCE-NE-PATCH missing).

---

## Dependencies

```
Task 1 (CMakeLists)                   ── precondition for all builds
   │
   ├──> Task 2 (PluginProcessor.h)
   │       │
   │       ├──> Task 3 (createParameterLayout — 29 params)
   │       │       │
   │       │       └──> Task 4 (PluginProcessor.cpp shell)
   │       │
   │       └──> Task 5 (PluginEditor.h/.cpp stub)
   │
   └──> Task 6 (macOS build) ──> Task 7 (install + auval)
                              ──> Task 8 (pluginval strictness 10)
                                                  │
                                                  └──> Task 9 (5-DAW smoke test)
```

Tasks 2–5 are file-creation; build (Task 6) is the first integration point. Verification (7–9) is sequential after install, but Tasks 8 and 9 are independent of each other (could run in parallel).

---

## Files Created / Modified

**Created (5 files):**
- `plugins/O-Contrabass/CMakeLists.txt`
- `plugins/O-Contrabass/Source/PluginProcessor.h`
- `plugins/O-Contrabass/Source/PluginProcessor.cpp`
- `plugins/O-Contrabass/Source/PluginEditor.h`
- `plugins/O-Contrabass/Source/PluginEditor.cpp`

**Modified:** none (root `CMakeLists.txt` auto-discovers via glob — no edits required).

---

## Success Criteria (Stage 1 Exit Gate)

Verbatim from `ROADMAP.md` §"Stage 1: Foundation → Test Criteria" + RESEARCH.md §7:

- [ ] `ninja O-Contrabass_VST3 O-Contrabass_AU O-Contrabass_Standalone` succeeds on macOS — clean build, no warnings.
- [ ] (Windows deferred to Phase 2.x — not blocking Stage 1 sign-off; macOS is dev primary.)
- [ ] All 29 APVTS parameters appear in DAW automation menu (verified per host in Task 9).
- [ ] Plugin loads in Logic Pro, Ableton, Reaper, Dorico, Cubase without error.
- [ ] `pluginval --strictness-level 10 --validate-in-process` passes (Task 8).
- [ ] `auval -a | grep -i contrabass` shows the AU registered (Task 7).
- [ ] State save/restore round-trips through APVTS XML (Task 9).
- [ ] No `getLatencySamples()` override anywhere in the source — `setLatencySamples(0)` only (memory file constraint).
- [ ] Both `NEEDS_WEBVIEW2 TRUE` and `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` set in CMake (memory file constraint, even though Stage 1 has no WebView UI yet).
- [ ] Parameter ID strings match `parameter-spec.md` (sha256:c47fe736…) verbatim — UPPER_SNAKE_CASE, no renames.

---

## Estimated Effort

**2–4 hours of focused implementation** (per RESEARCH.md §9):
- Tasks 1–5 (file creation): ~1.5–2.5 h
- Tasks 6–8 (build + auval + pluginval): ~30 min
- Task 9 (5-DAW smoke test): ~1 h (depends on host startup time)

Stage 1 is well-scoped because Stage 0 already locked every architectural decision. No novel research; pattern-matching pass against O-Bells / O-Bowed.

---

## Out of Scope (Deferred to Later Stages)

- **DSP implementation** (waveguide, friction, body resonator, vibrato, drone features, etc.) → Stage 2 (6 sub-phases per ROADMAP).
- **WebView UI** (HTML/JS/CSS, resource provider, parameter relays) → Stage 3.
- **Oversampler allocation** (voice-owned, not processor-owned) → Stage 2 Phase 2.1a.
- **Preset bank infrastructure** (`OuariconPresetManager`) → Stage 4 polish.
- **Windows build verification** — macOS is dev primary; Windows lights up before v1.0 release.
- **Note Expression activation** — module is linked at Stage 1 so the API surfaces; voice-side `applyPendingTuning` wiring lands in Stage 2 Phase 2.6.
- **MTS-ESP / Scala/TUN active tuning** — same as above; module linked, voice-side wiring deferred.
- **Module extraction (`bow-friction`)** — explicitly Stage 2 Phase 2.1b per ROADMAP.

---

## References

- `stages/1-foundation/CONTEXT.md` — discuss-phase synthesis (locked decisions)
- `stages/1-foundation/RESEARCH.md` — pattern confirmation pass (CMake / APVTS / latency)
- `parameter-spec.md` — 29-parameter contract (locked checksum)
- `research/ARCHITECTURE.md` — DSP contract (Stage 2+ scope)
- `ROADMAP.md` — Stage 1 component list + test criteria
- `plugins/O-Bells/CMakeLists.txt` — closest CMake analog
- `plugins/O-Bowed/Source/PluginProcessor.cpp` — APVTS + prepareToPlay idiom
- `troubleshooting/patterns/juce8-critical-patterns.md` §22 — synth contract enforcement
- Project memory (`CLAUDE.md`) — Windows WebView2 static linking, `getLatencySamples` non-virtual
