# Stage 1: Foundation — Research

**Date:** 2026-04-26
**Plugin:** O-Contrabass
**Stage:** 1 of 4 (Foundation)
**Scope:** Confirm CMake / APVTS / build patterns. Inputs are locked by Stage 0 contracts (see `CONTEXT.md`); zero open questions surfaced.

---

## Research Strategy

The discuss phase confirmed all Foundation inputs are locked by Stage 0 (`BRIEF.md`, `parameter-spec.md`, `research/ARCHITECTURE.md`, `ROADMAP.md`). Research is therefore a **thin pattern-confirmation pass**: identify the closest reference plugins, lift the canonical CMake + APVTS shapes verbatim, and pre-empt the well-known JUCE 8 pitfalls before they hit the build.

Three reference plugins inspected:
- **`plugins/O-Bowed/`** — architectural source (waveguide + friction + body); same `IS_SYNTH` shape, same scala-tuning-engine wiring, MPESynthesiser pattern.
- **`plugins/O-Bells/`** — most recent `note-expression` consumer (2026-04-25). Demonstrates `ouaricon_add_module(O-Bells note-expression)` integration.
- **`plugins/O-Lyrica/`** — original `note-expression` consumer (Phase 23 reference). Shows `NoteExpression.h` include + processor-level wiring.
- **`plugins/O-Reed/`** — minimal recent `juce_add_plugin` shape with scala-tuning-engine sourced via explicit file references (alternate pattern).

---

## 1. CMake Patterns

### 1.1 Canonical `juce_add_plugin` Block

The closest analog to O-Contrabass's exact requirements (synth + MIDI in + WebView + scala-tuning-engine + note-expression) is **O-Bells** (2026-04-25). Confirmed shape:

```cmake
juce_add_plugin(O-Contrabass
    COMPANY_NAME "${OUARICON_COMPANY_NAME}"
    PLUGIN_MANUFACTURER_CODE ${OUARICON_MANUFACTURER_CODE}
    PLUGIN_CODE OCbs                       # 4-char unique code, see §1.2
    FORMATS VST3 AU Standalone             # Standalone is free + useful for Stage 1 audition
    PRODUCT_NAME "O-Contrabass${OUARICON_DEV_SUFFIX}"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    NEEDS_WEB_BROWSER TRUE
    NEEDS_WEBVIEW2 TRUE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
)
```

**Notes:**
- `OUARICON_*` macros come from the project's root CMakeLists.txt and the `OuariconModules.cmake` include — no per-plugin definitions needed.
- `EDITOR_WANTS_KEYBOARD_FOCUS FALSE` matches every recent plugin (Bowed, Bells, Reed) — the WebView UI handles its own keyboard focus.
- `Standalone` format is included by both O-Bowed and O-Bells; cheap to keep and lets Stage 1 verification run without a DAW.

### 1.2 PLUGIN_CODE Selection

Existing 4-char codes: `OBwd` (Bowed), `OBls` (Bells), `OLyr` (Lyrica), `ORed` (Reed). **Recommended for Contrabass: `OCbs`** (O + Cb for "contrabass" + s suffix). Verified no collision in `plugins/*/CMakeLists.txt`. Lock this in Stage 1 — changing later breaks DAW preset persistence.

### 1.3 Compile Definitions (Cross-Platform WebView)

```cmake
target_compile_definitions(O-Contrabass
    PUBLIC
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=1
        JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
        JUCE_USE_CURL=0
)
```

**Critical (from memory file):** `NEEDS_WEBVIEW2 TRUE` alone is **not enough** on Windows — without `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`, JUCE tries to dynamically load `WebView2Loader.dll` at runtime, which is not distributed → silent blank UI. The static-linking define is mandatory whenever `NEEDS_WEBVIEW2 TRUE` is set. Verified in O-Bowed, O-Bells, O-Lyrica, O-Reed CMakeLists — every recent plugin sets both.

### 1.4 Module Wiring

Two patterns exist in the codebase:

**Pattern A (preferred, used by O-Bells / O-Lyrica) — `ouaricon_add_module()`:**
```cmake
include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)
ouaricon_add_module(O-Contrabass note-expression)
```
- Single line per module. Auto-handles per-format routing (`cpp/vst3/NoteExpression_VST3.cpp` goes to the VST3 link line only via `OuariconModules.cmake` D-23-04-A logic).
- For `note-expression` specifically, the module's `module.cmake` hook auto-verifies the `JUCE-NE-PATCH` marker in `~/JUCE/`. Failure here is fail-loud at configure time, not link time.

**Pattern B (used by O-Bowed / O-Reed) — explicit file references for scala-tuning-engine:**
```cmake
target_sources(O-Contrabass PRIVATE
    ${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/cpp/TuningEngine.cpp
    ${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/cpp/ScaleGenerator.cpp
    ${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/cpp/EmbeddedTunings.cpp
    ${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/cpp/TuningExporter.cpp
)
target_include_directories(O-Contrabass PRIVATE
    ${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/cpp
)
```

**Recommendation for O-Contrabass:**
- **`note-expression`** → Pattern A (`ouaricon_add_module`). The note-expression module *requires* per-format routing for `cpp/vst3/NoteExpression_VST3.cpp` (the Steinberg-touching TU); Pattern A handles this automatically. Pattern B would link the VST3 TU into AU/Standalone and break.
- **`scala-tuning-engine`** → Pattern B (explicit file references) is what every recent plugin uses (Bowed, Bells, Reed). `scala-tuning-engine` has no `module.cmake`/no per-format routing, so Pattern B and Pattern A are equivalent for it; Pattern B is the established convention. Use it for symmetry with siblings.

### 1.5 Required JUCE Modules

Same set as every recent plugin (O-Bowed verbatim):
```cmake
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
```

CONTEXT.md's shorter list (7 modules) was a minimum-viable list; the canonical list above (13 modules) is what every shipping plugin uses. Use the canonical list — `juce_audio_devices` is needed for Standalone, `juce_audio_formats` for asset loading, `juce_data_structures` for ValueTree (APVTS dependency), `juce_events` for MessageManager, `juce_graphics` for any future canvas use.

### 1.6 `juce_generate_juce_header` Order (CRITICAL)

```cmake
juce_generate_juce_header(O-Contrabass)   # MUST come AFTER target_link_libraries
```

Per `troubleshooting/patterns/juce8-critical-patterns.md` and inline comment in O-Bowed:CMakeLists.txt:80, the header must be generated *after* JUCE modules are linked, otherwise `JuceHeader.h` is empty and every translation unit that includes it errors out cryptically.

### 1.7 Binary Data (Stage 1 — empty placeholder)

WebView resources land in Stage 3. For Stage 1, we need a `juce_add_binary_data` block ready (so the link target name `O-Contrabass_UIResources` exists), but it can be empty / single placeholder. Or omit entirely and add at Stage 3. **Recommendation: omit at Stage 1.** Stage 1 editor is a stub Component (no WebView) — adding empty binary data now is dead weight that only matters once the WebView is wired up. Stage 3's plan task adds it then.

### 1.8 Resources Directory

CONTEXT.md doesn't mention a `Resources/` directory — Stage 3 will create it for WebView UI assets. Stage 1 doesn't need it. (Keeps the foundation directory minimal.)

---

## 2. PluginProcessor / APVTS Patterns

### 2.1 BusesProperties (Output-Only Synth)

Verbatim from O-Bowed:PluginProcessor.cpp:238 — confirmed canonical:
```cpp
OContrabassAudioProcessor::OContrabassAudioProcessor()
    : AudioProcessor(BusesProperties()
                        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , parameters(*this, nullptr, "Parameters", createParameterLayout())
{
    // synthesiser/voice setup deferred to Stage 2
}
```

Per `juce8-critical-patterns.md` §22, `IS_SYNTH TRUE` *requires* output-only `BusesProperties` — no input bus. Adding `.withInput(...)` here will produce silent failures: plugin loads but DAW won't route MIDI.

### 2.2 APVTS Layout — 29 Parameters

Pattern: `parameters(*this, nullptr, "Parameters", createParameterLayout())` where `createParameterLayout()` returns `juce::AudioProcessorValueTreeState::ParameterLayout`. Parameters are added via `layout.add(std::make_unique<juce::AudioParameterFloat/Int/Bool/Choice>(...))`.

**Parameter ID convention:** `parameter-spec.md` uses `UPPER_SNAKE_CASE` (e.g., `BOW_SPEED`). O-Bowed uses `lowerCamelCase` (`bowSpeed`). **Decision needed:** stick with parameter-spec.md's `UPPER_SNAKE_CASE` — that's the locked contract and changing it invalidates the parameter-spec.md sha256 checksum tracked in STATUS.md. Add a comment block at the top of `createParameterLayout` noting the convention difference from sibling plugins.

**ParameterID version:** All examples use `juce::ParameterID { "ID", 1 }` — version `1`. Do not bump the version in Stage 1; reserve bumps for breaking parameter changes post-release.

**29-parameter breakdown** (from `parameter-spec.md`):

| Category | Count | Types |
|---|---|---|
| Primary (Tier 1) | 5 | 5 × Float |
| Secondary (Tier 2) | 5 | 5 × Float |
| String Configuration (Tier 3) | 3 | 2 × Float + 1 × Int |
| Per-String Detune | 4 | 4 × Float (range −1200…+1200 cents) |
| Expression | 6 | 6 × Float |
| Drone Features | 2 | 2 × Float |
| Output | 1 | 1 × Float |
| Microtonal | 3 | 1 × Float + 1 × Choice + 1 × Bool |
| **Total** | **29** | 26 Float + 1 Int + 1 Choice + 1 Bool |

**Skewed ranges:** Several params benefit from non-linear `NormalisableRange`. Match O-Bowed's idioms:
- `BOW_SPEED` (0.02 → 1.5, default 0.15): use `NormalisableRange<float>(0.02f, 1.5f, 0.001f, 0.5f)` (skew factor 0.5 = log-ish).
- `BOW_PRESSURE` (0.05 → 8.0, default 1.0): `NormalisableRange<float>(0.05f, 8.0f, 0.01f, 0.5f)`.
- `BRIGHTNESS` (80 → 12000 Hz, default 4500): `NormalisableRange<float>(80.0f, 12000.0f, 1.0f, 0.25f)` — heavy skew toward low end (matches O-Bowed brightness).
- `OUTPUT_GAIN` (−60 → +12 dB): linear is fine; users expect linear dB.
- `VIBRATO_ONSET` (0 → 3000 ms): `NormalisableRange<float>(0.0f, 3000.0f, 1.0f, 0.5f)` — short onsets are more useful, skew low.
- All `DETUNE_*` (−1200 → +1200 cents): linear; symmetric range.

**TUNING_SYSTEM Choice param** — uses `juce::AudioParameterChoice` with `juce::StringArray { "Scala/TUN", "MTS-ESP", "12-TET" }` and default index `2` (12-TET) per parameter-spec.md.

**NOTE_EXPRESSION Bool param** — uses `juce::AudioParameterBool` with default `true` per spec (Note Expression on by default for Dorico playback).

### 2.3 prepareToPlay & Latency

Per architecture §"Oversampler latency" (line 970):
> Oversampler latency: ~1–3 samples (polyphase IIR), reported via `setLatencySamples(static_cast<int>(std::ceil(oversampler.getLatencyInSamples())))` in `prepareToPlay`.

**For Stage 1:** No oversampler exists yet (it's a Stage 2 component, lives inside the voice per O-Bowed pattern). So Stage 1's `prepareToPlay` should:
- Call `setLatencySamples(0)` explicitly. (Bypass-mode plugin reports zero latency.)
- Allocate nothing else — voice/oversampler/body/etc. all arrive in Stage 2.

CONTEXT.md says Stage 1 should "allocate oversampler" — but the architecture and O-Bowed reference both place the oversampler *per voice*, not on the processor. Allocating it on the processor in Stage 1 only to throw it away in Stage 2 when voice DSP arrives is wasted code. **Recommendation: defer oversampler allocation to Stage 2 entirely.** Stage 1 reports `setLatencySamples(0)` to keep pluginval strictness 10 honest; Stage 2 updates this when the voice ships.

This is a refinement to CONTEXT.md, not a contradiction — CONTEXT.md notes "Stage 2 will wire it; Stage 1 only allocates and reports latency", but allocation is a Stage 2 concern given the voice-owned ownership model. Plan phase should record this as the chosen approach.

### 2.4 getLatencySamples — DO NOT OVERRIDE

From memory file + `juce8-critical-patterns.md`: `AudioProcessor::getLatencySamples()` is **non-virtual** in JUCE 8. Overriding it compiles cleanly (no `override` keyword to catch the mistake) but the override is never called — the host always reads the internal member set by `setLatencySamples()`. Symptom: latency reported to DAW does not match what the override returns; PDC misalignment.

**For Stage 1 implementation:** do not declare `getLatencySamples()` in `PluginProcessor.h` at all. Use `setLatencySamples(N)` exclusively from `prepareToPlay`.

### 2.5 processBlock Stub (Stage 1)

```cpp
void OContrabassAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                             juce::MidiBuffer& /*midi*/)
{
    juce::ScopedNoDenormals noDenormals;

    // Clear any output channels above input count (synth: clear all output)
    for (auto i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // Stage 1: bypass / silent output. DSP arrives in Stage 2.
    buffer.clear();
}
```

`juce::ScopedNoDenormals` at block entry is universal across all sibling plugins — adopt now even though the body is a stub, so Stage 2 can ignore the concern.

### 2.6 getStateInformation / setStateInformation

Stage 1 uses the bare APVTS state pattern (preset bank metadata is Stage 4):
```cpp
void OContrabassAudioProcessor::getStateInformation(juce::MemoryBlock& destData) {
    if (auto xml = parameters.copyState().createXml())
        copyXmlToBinary(*xml, destData);
}
void OContrabassAudioProcessor::setStateInformation(const void* data, int sizeInBytes) {
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
        parameters.replaceState(juce::ValueTree::fromXml(*xml));
}
```

This is the JUCE textbook pattern. `OuariconPresetManager` wiring (used by O-Bowed/O-Lyrica) is Stage 4 polish work, not Stage 1.

### 2.7 isBusesLayoutSupported

Synth plugins should override to whitelist mono and stereo output:
```cpp
bool OContrabassAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const override {
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}
```
(Stage 1 architecture says stereo-only — but allowing mono out is cheap insurance for DAWs that prefer to negotiate mono. Width param is post-DSP, mono-out is well-defined as M-only.)

---

## 3. PluginEditor Stub (Stage 1)

CONTEXT.md says "minimal Component or trivial label" — confirmed correct.

```cpp
class OContrabassAudioProcessorEditor : public juce::AudioProcessorEditor {
public:
    explicit OContrabassAudioProcessorEditor(OContrabassAudioProcessor& p)
        : AudioProcessorEditor(&p), processorRef(p) { setSize(600, 400); }
    void paint(juce::Graphics& g) override {
        g.fillAll(juce::Colours::black);
        g.setColour(juce::Colours::white);
        g.drawText("O-Contrabass — Stage 1 (Foundation)", getLocalBounds(),
                   juce::Justification::centred);
    }
    void resized() override {}
private:
    OContrabassAudioProcessor& processorRef;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OContrabassAudioProcessorEditor)
};
```

No WebView wiring, no relays, no resource provider. Adding all of that now risks blank-UI debugging on a Stage 1 build that has no DSP to validate against. Defer to Stage 3.

`hasEditor()` returns `true`; `createEditor()` returns `new OContrabassAudioProcessorEditor(*this)`. Standard JUCE pattern.

---

## 4. JUCE 8 Pitfalls Already Addressed

These are well-known traps from `troubleshooting/patterns/juce8-critical-patterns.md` and the project memory file. All covered here, all preventable in Stage 1:

| # | Pitfall | Prevention in Stage 1 |
|---|---|---|
| 1 | `IS_SYNTH FALSE` → no MIDI routing | `IS_SYNTH TRUE` set explicitly; verified in §1.1 |
| 2 | Input bus on synth → MIDI not routed | `BusesProperties` output-only; verified §2.1 |
| 3 | `getLatencySamples()` override silently ignored | Use `setLatencySamples(0)` only; never declare the override (§2.4) |
| 4 | Windows WebView2 silent fallback to IE | Both `NEEDS_WEBVIEW2 TRUE` *and* `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` set (§1.3) |
| 5 | `JuceHeader.h` empty if generated before `target_link_libraries` | Order enforced in §1.6 |
| 6 | `note-expression` VST3 TU leaking into AU/Standalone | `ouaricon_add_module` per-format routing (§1.4) |
| 7 | Missing `JUCE-NE-PATCH` → undefined behavior | `note-expression`'s `module.cmake` fails configure-time if marker absent (§1.4) |
| 8 | Denormals on bypass build | `juce::ScopedNoDenormals` at processBlock entry (§2.5) |
| 9 | Plugin code collision in DAW | `OCbs` verified unique (§1.2) |

---

## 5. Reusable Modules (No New Modules in Stage 1)

| Module | Path | Stage 1 use | Stage 2+ use |
|---|---|---|---|
| `scala-tuning-engine` | `modules/tuning/scala-tuning-engine` | Linked via Pattern B (explicit file refs); no API calls | Voice-side `TuningEngine::frequencyForMidiNote()` |
| `note-expression` | `modules/tuning/note-expression` | Linked via Pattern A (`ouaricon_add_module`); patch marker verified at configure | Voice-side `applyPendingTuning(table, note, freq)` (§Phase 2.6) |
| `bow-friction` | (TO BE CREATED Phase 2.1b) | N/A | Phase 2.1b extracts O-Bowed's `HyperbolicFriction.h` to `modules/dsp/bow-friction/` |

Stage 1 surfaces no new modules. The `bow-friction` extraction is explicitly scheduled in `ROADMAP.md` Phase 2.1b — not Stage 1 work.

---

## 6. Source Layout (Stage 1 Final State)

```
plugins/O-Contrabass/
├── CMakeLists.txt
└── Source/
    ├── PluginProcessor.h
    ├── PluginProcessor.cpp
    ├── PluginEditor.h
    └── PluginEditor.cpp
```

Four files. No `Source/DSP/` subdirectory yet (added in Phase 2.1a). No `Resources/` (added in Stage 3). No `improvements/` (created post-v1.0 per `/improve` workflow).

---

## 7. Verification Strategy (Stage 1 Exit Gate)

Tests inherited from `ROADMAP.md`:
1. `ninja O-Contrabass_VST3 O-Contrabass_AU O-Contrabass_Standalone` succeeds on macOS (Standalone added per §1.1 reasoning).
2. macOS install protocol from `CLAUDE.md` (clear AU cache, install fresh binaries).
3. `auval -a | grep -i contrabass` — AU registers.
4. `pluginval --strictness-level 10 --validate-in-process build/.../O-Contrabass.vst3` — passes (bypass mode; no audio expected, but the validator's parameter / state / threading / latency checks all run).
5. DAW load test: Logic Pro (AU), Ableton (VST3), Reaper (VST3), Dorico (VST3 + Note Expression detection — Note Expression API surfaces even on a silent plugin once `note-expression` module is linked), Cubase (VST3).
6. Verify all 29 parameters appear in DAW automation menu (Logic: track header → Smart Controls → automation; Ableton: device parameter list; Reaper: param window).

**Gotchas to watch:**
- Logic Pro 11 caches AU validation — `killall AudioComponentRegistrar` + cache clear (CLAUDE.md protocol) is mandatory between rebuilds, not optional.
- pluginval strictness 10 will exercise `setStateInformation` with malformed XML — the `juce::ValueTree::fromXml` pattern in §2.6 returns an empty ValueTree on parse failure (graceful), passing the test.
- Dorico Note Expression detection requires the JUCE-NE-PATCH to be applied. If `auval` passes but Dorico doesn't see Note Expression, that's a missing patch (re-run `scripts/apply-juce-patches.sh`).

---

## 8. Open Questions

**None.** All decisions resolved during Stage 0 + discuss; this research only confirmed patterns.

Two minor clarifications captured for the plan phase to record explicitly:
1. **Oversampler ownership:** voice-level (Stage 2), not processor-level (Stage 1). CONTEXT.md should be read as "Stage 1 reports zero latency; Stage 2 reports oversampler latency once the voice exists." (§2.3)
2. **Module wiring patterns:** Pattern A (`ouaricon_add_module`) for `note-expression` (per-format routing required); Pattern B (explicit file refs) for `scala-tuning-engine` (matches sibling-plugin convention). Both patterns ship in the project. (§1.4)

---

## 9. Summary for Plan Phase

**Stage 1 is a thin foundation pass with no novel research findings.** Every required pattern has a 1:1 reference in a recently-shipped sibling plugin. The plan phase should:

1. Lift the CMakeLists.txt shape from O-Bells (most recent + closest match: synth + WebView + note-expression) and substitute O-Contrabass identifiers + `OCbs` plugin code.
2. Build `createParameterLayout()` from the `parameter-spec.md` table — 29 parameters in 8 categories, ID strings UPPER_SNAKE_CASE verbatim, skewed ranges per §2.2.
3. PluginProcessor.h: declare APVTS member, `BusesProperties` output-only, `setLatencySamples(0)`, no `getLatencySamples` override, APVTS save/restore stubs.
4. PluginEditor.h/.cpp: minimal Component with a centered text label.
5. Verification: 6-point exit gate from §7 — DAW loads, parameter visibility, pluginval strictness 10.

**Estimated effort:** 2–4 hours of focused implementation. Stage 1 is well-scoped because Stage 0 did the hard work.

---

## References

- `plugins/O-Bells/CMakeLists.txt` — closest CMake analog (synth + note-expression + WebView, 2026-04-25).
- `plugins/O-Bowed/Source/PluginProcessor.cpp:16-275` — APVTS layout idiom + prepareToPlay shape.
- `plugins/O-Lyrica/Source/PluginProcessor.h:22` — `note-expression` include site.
- `plugins/O-Reed/CMakeLists.txt` — minimal recent shape (no note-expression yet); shows scala-tuning-engine Pattern B.
- `modules/cmake/OuariconModules.cmake` — `ouaricon_add_module` semantics (per-format routing rules at lines 82-101).
- `modules/tuning/note-expression/module.cmake` — JUCE-NE-PATCH configure-time verification.
- `modules/tuning/note-expression/cpp/NoteExpression.h:33-79` — public API; `applyPendingTuning` is the voice-side helper.
- `troubleshooting/patterns/juce8-critical-patterns.md` §22 — `IS_SYNTH` / `BusesProperties` correlation.
- Project memory (CLAUDE.md) — Windows WebView2 static linking; `getLatencySamples` non-virtual.
- `plugins/O-Contrabass/.planning/research/ARCHITECTURE.md:970-971` — oversampler latency reporting contract.
- `plugins/O-Contrabass/.planning/parameter-spec.md` — locked 29-parameter table (sha256:c47fe736…).
