# Stage 1 (Foundation + Shell) — Research

**Plugin:** O-simplePhysicalModelSynth
**Stage:** 1 of 4 (Foundation + Shell)
**Phase:** research
**Date:** 2026-06-26
**Primary reference:** O-simpleFM (validated, in-repo, JUCE 8.0.9 — closest structural analog)

This stage is **foundation/shell only**: CMake + 17-param APVTS + silent 16-voice `Synthesiser`
shell + render-harness scaffold. No DSP (Stage 2), no WebView (Stage 3). The architecture is
fully locked upstream (`research/ARCHITECTURE.md`, `ROADMAP.md`, `parameter-spec.md`), so research
here is narrow: **confirm the O-simpleFM foundation pattern applies, and pin the few places where
this plugin must NOT copy O-simpleFM verbatim.** Every pattern below is lifted from a building,
pluginval-clean sibling — no external/unvalidated APIs are introduced at Stage 1.

---

## 1. Structural template — copy O-simpleFM, with 4 deliberate divergences

O-simpleFM (`plugins/O-simpleFM/`) is the foundation template: pedagogical 16-voice WebView synth,
same `IS_SYNTH` + WebView2 + render-harness shape, started life as exactly this "silent shell"
stage. Clone its skeleton. **But O-simpleFM is now at Stage 4**, so its tree carries Stage-3/4
additions that must NOT be pulled into our Stage 1:

| # | O-simpleFM has (Stage 3/4) | Our Stage 1 does instead | Why |
|---|-----------------------------|--------------------------|-----|
| D1 | `juce_add_binary_data` + WebView editor + `ui/` tree | **None.** `GenericAudioProcessorEditor` placeholder; binary-data target deferred to Stage 3 | WebView is Stage 3. CONTEXT.md §APVTS. |
| D2 | `ouaricon_add_module(preset-manager)` + `OuariconPresetManager` in getState/setState | **Plain APVTS** state round-trip; no preset module | Presets are Stage 4 (FUNC-07). |
| D3 | Percent params stored **0–1 normalized** (`unitRange()`, UI ×100) | Percent params stored **0–100** (range `{0,100,…}`) | Our `parameter-spec.md` LOCKS 0–100 ranges + 0–100 defaults. See §3 — this is the #1 copy hazard. |
| D4 | Always-on 2× `dsp::Oversampling` + `setLatencySamples(latency)` | **No oversampler**; `setLatencySamples(0)` | ARCHITECTURE: no oversampling in v1.0. Don't inherit FM's anti-alias rig. |

Everything else (ParamIDs namespace, `Synthesiser` wiring, header-only voice, processBlock
clear-buffer, `ScopedNoDenormals`, bus layout, `createPluginFilter`) ports near-verbatim.

---

## 2. CMakeLists.txt — confirmed config

From `plugins/O-simpleFM/CMakeLists.txt`, adapted. Stage-1 shape:

```cmake
cmake_minimum_required(VERSION 3.15)
include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)   # OUARICON_* vars (company, mfr, dev suffix)

juce_add_plugin(O-simplePhysicalModelSynth
    COMPANY_NAME "${OUARICON_COMPANY_NAME}"
    PLUGIN_MANUFACTURER_CODE ${OUARICON_MANUFACTURER_CODE}
    PLUGIN_CODE OsPM                       # see §8 — collision-checked, free
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "O-simplePhysicalModelSynth${OUARICON_DEV_SUFFIX}"
    VERSION "1.0.0"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    NEEDS_WEB_BROWSER TRUE                  # set NOW so Stage 3 inherits correct cross-platform config
    NEEDS_WEBVIEW2 TRUE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
)
# target_sources: PluginProcessor.{h,cpp}, PluginEditor.{h,cpp} (generic placeholder),
#                 PhysicalModelVoice.h   (NO FactoryPresets, NO ui/ at Stage 1)
# target_include_directories(... PRIVATE Source)
# target_link_libraries(... juce_dsp + the standard 13 JUCE modules ...)  ← link juce_dsp now
juce_generate_juce_header(O-simplePhysicalModelSynth)   # AFTER target_link_libraries (JUCE-8 req)
target_compile_definitions(... PUBLIC
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_WEB_BROWSER=1
    JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1          # COMPAT-02 (Windows blank-WebView guard)
    JUCE_USE_CURL=0)
option(OUARICON_BUILD_TESTS "Build render-test harness" OFF)
if(OUARICON_BUILD_TESTS)
    add_subdirectory(tests/render-harness)
endif()
```

**Confirmed:** `NEEDS_WEB_BROWSER`/`NEEDS_WEBVIEW2` + the two WebView2 compile defs belong at
Foundation even with no WebView yet (O-simpleFM does exactly this; matches the project-wide
Windows-WebView memory). `juce_dsp` must be linked now (`DelayLine<Thiran>`, `dsp::FFT` arrive
Stage 2 but the header/module dependency belongs in the foundation). **Omit** at Stage 1:
`juce_add_binary_data`, `ouaricon_add_module`, any `Source/ui/`.

---

## 3. APVTS — the 17 params (⚠ 0–100 ranges, NOT O-simpleFM's 0–1)

`ParamIDs` namespace pattern (O-simpleFM `PluginProcessor.h:27`), IDs verbatim from
`parameter-spec.md`. **CRITICAL divergence (D3):** O-simpleFM's `createParameterLayout` stores
percent params on a **0–1** `unitRange()` and lets the UI multiply by 100. **Our locked spec stores
them 0–100.** Copying O-simpleFM's `unitRange()` for the percent params would silently violate the
zero-drift contract (wrong range endpoints + wrong defaults). Use explicit `{0.0f, 100.0f, …}`.

| ID | JUCE param | Range | Default | Notes |
|----|-----------|-------|---------|-------|
| `excitationType` | `AudioParameterChoice` | {Pluck, Strike, Bow} | 0 (Pluck) | index 0 default |
| `excitationPosition` | `AudioParameterFloat` | 0–100 | 25 | percent — **0–100** |
| `excitationColor` | `AudioParameterFloat` | 0–100 | 60 | percent |
| `bowForce` | `AudioParameterFloat` | 0–100 | 50 | percent |
| `resonatorType` | `AudioParameterChoice` | {String, Modal} | 0 (String) | |
| `stringModel` | `AudioParameterChoice` | {Karplus-Strong, Waveguide} | 0 (KS) | |
| `inharmonicity` | `AudioParameterFloat` | 0–100 | 0 | percent |
| `modeBrightness` | `AudioParameterFloat` | 0–100 | 50 | percent |
| `damping` | `AudioParameterFloat` | 0–100 | 60 | percent |
| `decay` | `AudioParameterFloat` | 0–100 | 70 | percent |
| `material` | `AudioParameterFloat` | 0–100 | 30 | percent |
| `coarseTune` | `AudioParameterInt` | −24…+24 | 0 | semitones |
| `fineTune` | `AudioParameterFloat` | −100…+100 | 0 | cents |
| `ampAttack` | `AudioParameterFloat` | 0–2 | 0.001 | seconds |
| `ampRelease` | `AudioParameterFloat` | 0–5 | 0.2 | seconds |
| `velToBrightness` | `AudioParameterFloat` | 0–100 | 60 | percent |
| `outputLevel` | `AudioParameterFloat` | −60…0 | −6 | dB (`.withLabel("dB")`) |

**Patterns confirmed against O-simpleFM:**
- `juce::ParameterID { id, 1 }` (version-hint 1) on every param — spec §Implementation Notes.
- Choice params: `StringArray`, default index `0`.
- `coarseTune` → `AudioParameterInt` (the only int param).
- `outputLevel` → `AudioParameterFloatAttributes().withLabel("dB")` (O-simpleFM:99-100).
- Build via `std::vector<std::unique_ptr<RangedAudioParameter>>` → `return { params.begin(), params.end() }` (O-simpleFM:40,102).

**Judgment call for planning (does NOT change the contract):** the contract locks range endpoints
+ defaults, not the `NormalisableRange` *skew*. O-simpleFM applies a perceptual skew (~0.35) to
time params. Recommend a mild skew on `ampAttack`/`ampRelease` (and optionally `outputLevel`) for
automation feel; percent params stay linear. If in doubt, ship linear — zero-drift cares only about
endpoints/defaults, both of which are fixed above.

**Naming clearance:** none of the 17 IDs are bare `end`/`begin` (the param-ID-shadows-juce gotcha)
— clear. Voice class `PhysicalModelVoice` does not shadow any `juce::` type (the
`SamplerVoice`/`SamplerSound` hazard) — clear.

---

## 4. Synthesiser + silent voice stub

O-simpleFM constructor pattern (`PluginProcessor.cpp:106-123`), minus presets/oversampler:

```cpp
for (int i = 0; i < kNumVoices; ++i)      // kNumVoices = 16
    synth.addVoice (new PhysicalModelVoice());
synth.addSound (new PhysicalModelSound());
synth.setNoteStealingEnabled (true);
```

**Voice = header-only** (mirror `FMVoice.h` / `FMSound`):
- `PhysicalModelSound : juce::SynthesiserSound` → `appliesToNote`/`appliesToChannel` return `true`.
- `PhysicalModelVoice : juce::SynthesiserVoice` → `canPlaySound` `dynamic_cast`s the sound;
  `startNote`/`stopNote`/`pitchWheelMoved`/`controllerMoved` empty; `renderNextBlock` **no-op**
  (silent shell — Stage 1 produces no audio). `clearCurrentNote()` in `stopNote` so voices free
  and the synth doesn't wedge under pluginval note storms.
- JUCE-8 note (FMVoice.h:46-48): `SynthesiserVoice` has **no virtual `prepareToPlay`** — declare a
  non-virtual `prepareToPlay(double,int)` and dispatch it from the processor's `prepareToPlay` via
  `dynamic_cast` over `synth.getVoice(i)`. Header-only voice means **no extra .cpp in the harness**.

`processBlock` (O-simpleFM:220-227): `ScopedNoDenormals`; `buffer.clear()`; `synth.renderNextBlock(...)`
(voices add into the cleared buffer — currently silent). `setLatencySamples(0)` in `prepareToPlay`
(getter is non-virtual in JUCE 8 — memory). Bus = output-only stereo
(`BusesProperties().withOutput("Output", stereo(), true)`, O-simpleFM:107-108);
`isBusesLayoutSupported` accepts mono/stereo out.

---

## 5. State persistence — plain APVTS (preset manager is Stage 4)

O-simpleFM routes getState/setState through `OuariconPresetManager` (`PluginProcessor.cpp:350-362`)
— that's a **Stage-4** addition (D2). Stage 1 uses the canonical plain-APVTS round-trip:

```cpp
void getStateInformation (juce::MemoryBlock& destData) override {
    if (auto xml = parameters.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}
void setStateInformation (const void* data, int sizeInBytes) override {
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (parameters.state.getType()))
            parameters.replaceState (juce::ValueTree::fromXml (*xml));
}
```

APVTS handles all 17 params automatically; no custom/non-parameter state exists at Stage 1
(ARCHITECTURE §State Persistence). This satisfies the "state save/restore round-trips" criterion.

---

## 6. createEditor — generic placeholder + the WebView seam

Stage 1 editor is a `GenericAudioProcessorEditor` (all 17 params visible for the verify check).
Pre-place the `#if JUCE_WEB_BROWSER` seam now so Stage 3 swaps in the WebView editor without
re-plumbing, and so the harness (which compiles the processor under `JUCE_WEB_BROWSER=0`) never
sees WebView types:

```cpp
juce::AudioProcessorEditor* createEditor() override {
#if JUCE_WEB_BROWSER
    return new juce::GenericAudioProcessorEditor (*this);  // Stage 3: → WebView editor here
#else
    return new juce::GenericAudioProcessorEditor (*this);  // harness build
#endif
}
```

At Stage 1 both branches are identical (Generic); the guard is the forward-looking seam. Keep
`PluginEditor.cpp` a thin generic file (or inline createEditor in the processor and skip
PluginEditor.cpp entirely) — either keeps the harness clean. See §7.

---

## 7. Render-harness scaffold — binding decision diverges from O-simpleFM's current harness

⚠ **O-simpleFM's harness is the cautionary example, not the template to copy.** Its current
`tests/render-harness/CMakeLists.txt` compiles `PluginEditor.cpp`, sets `JUCE_WEB_BROWSER=1`, and
links `O-simpleFM_UIResources` — that's the *post-Stage-3 retrofit* after the WebView editor broke
the harness (memory: `pattern_render_harness_breaks_on_webview_editor`). Our CONTEXT.md picked the
**cleaner resolution** up front:

- Harness compiles **`JUCE_WEB_BROWSER=0`**.
- **Drop `PluginEditor.cpp` from harness sources** — harness links only `PluginProcessor.cpp`
  (voice/viz are header-only).
- **`#if JUCE_WEB_BROWSER` guard on `createEditor`** (§6) so the processor TU compiles with no
  editor/WebView symbols under `=0`.
- No `O-simpleFM_UIResources`-equivalent link (no binary-data target exists until Stage 3).

Scaffold to create (adapt O-simpleFM `tests/render-harness/` skeleton, strip the editor/UI bits):
- `tests/render-harness/CMakeLists.txt` — `juce_add_console_app`; sources = `main.cpp` +
  `../../Source/PluginProcessor.cpp` **only**; `add_dependencies(... O-simplePhysicalModelSynth)`
  then borrow `$<TARGET_PROPERTY:…,INCLUDE_DIRECTORIES>` for the generated JuceHeader;
  `target_compile_definitions` mirror O-simpleFM's `JucePlugin_*` block **but set
  `JUCE_WEB_BROWSER=0`** and supply our own `JucePlugin_PluginCode`/name macros; link `juce_dsp`
  + the JUCE modules (no `*_UIResources`).
- `tests/render-harness/main.cpp` — **stub** at Stage 1: instantiate the processor, `prepareToPlay`,
  push a note, render a few blocks, assert finite + no crash, return 0. The **autocorrelation pitch
  probe** (NOT spectral — the KS loop comb fools single-bin DFT; memory + ARCHITECTURE risk note)
  lands in Stage 2.1. The Stage-1 stub just proves the harness *builds and links* against the shell.
- Keep `option(OUARICON_BUILD_TESTS … OFF)`; the harness is opt-in.

This is the cheapest place to lock the seam — building the (trivial) harness once at Stage 1 proves
the `JUCE_WEB_BROWSER=0` / no-editor wiring before any DSP depends on it.

---

## 8. PLUGIN_CODE — `OsPM` (collision-checked)

AU subtype must be a unique 4-char code (collisions shadow each other in Logic's registry —
memory: `critical_dev_release_variant_shadowing`). Swept all 37 suite codes: `OsPM` is **free** and
consistent with the lowercase-"s" *simple* siblings (`OsGr` simpleGrain, `OsSm` simpleSampler).
Note `OSpS` is **taken** (O-SpectralShaper) — do not use it despite the "SpS" temptation. Has the
required uppercase chars (JUCE rejects all-lowercase codes). **Recommend `OsPM`.**

---

## 9. Pitfalls (knowledge base) — pre-cleared for this stage

| Pitfall (memory) | Status at Stage 1 |
|------------------|-------------------|
| `SamplerVoice`/`SamplerSound` shadow `juce::` types | **Avoided** — `PhysicalModelVoice`/`PhysicalModelSound`. |
| Bare `end`/`begin` param-ID shadows `juce::` free fn | **N/A** — no such ID in the 17. |
| Dual `juce_add_binary_data` namespace collision | **N/A at Stage 1** (no binary data yet); flag for Stage 3 if presets ever become binary data → distinct `NAMESPACE`. |
| Windows WebView2 blank page (static-link flag) | **Pre-empted** — `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` + `NEEDS_WEBVIEW2 TRUE` set now (COMPAT-02). |
| Render-harness breaks when editor becomes WebView | **Pre-empted** — §7 (drop PluginEditor.cpp + `JUCE_WEB_BROWSER=0` + `#if` guard from day one). |
| `getLatencySamples()` non-virtual in JUCE 8 | **Handled** — `setLatencySamples(0)` in `prepareToPlay`. |
| Dev/release variant bundle shadowing in AU registry | Use `build-and-install.sh` (Phase-4 dual-variant sweep) for the verify install; `OUARICON_DEV_SUFFIX` already in CMake. |

No new/unvalidated JUCE APIs are introduced at Stage 1 — every class (`AudioProcessorValueTreeState`,
`Synthesiser`, `SynthesiserVoice/Sound`, `GenericAudioProcessorEditor`, `MemoryBlock` XML round-trip)
is in active use across the suite at JUCE 8.0.9.

---

## 10. File manifest for the plan phase

Create under `plugins/O-simplePhysicalModelSynth/`:
- `CMakeLists.txt` (§2)
- `Source/PluginProcessor.h` — `ParamIDs` namespace (§3) + processor class (§4–6)
- `Source/PluginProcessor.cpp` — `createParameterLayout` (§3), ctor/prepare/processBlock (§4), getState/setState (§5), createEditor (§6), `createPluginFilter`
- `Source/PluginEditor.h` / `Source/PluginEditor.cpp` — thin generic editor (or inline in processor + skip)
- `Source/PhysicalModelVoice.h` — header-only silent voice + sound (§4)
- `tests/render-harness/CMakeLists.txt` + `tests/render-harness/main.cpp` — stub gate (§7)

Then register the plugin in the root `CMakeLists.txt` `add_subdirectory` list.

## 11. Verify-phase targets (carried from ROADMAP / CONTEXT)
- VST3 + AU build; appears as an instrument (IS_SYNTH).
- pluginval strictness 5+ passes (COMPAT-01).
- All 17 params in generic editor; state save/restore round-trips.
- Silent, no crash on note input.
- WebView2 flags present (COMPAT-02).
- Harness builds under `-DOUARICON_BUILD_TESTS=ON` (`JUCE_WEB_BROWSER=0`, no editor).

---

## References
- `plugins/O-simpleFM/` — CMakeLists, PluginProcessor.{h,cpp}, FMVoice.h, tests/render-harness/ (structural template; mind the 4 divergences §1).
- `research/ARCHITECTURE.md`, `ROADMAP.md` (Stage 1), `parameter-spec.md` (zero-drift 17-param contract), `stages/1-foundation/CONTEXT.md` (binding discuss decisions).
- Memory: render-harness/WebView, dev/release shadowing, getLatencySamples non-virtual, param-ID/class-name shadowing, Windows WebView2 static-link.
</content>
</invoke>
