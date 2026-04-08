# O-Bowed Stage 1: Foundation — Plan

**Created:** 2026-04-04
**Phase:** plan
**Goal:** Buildable plugin shell with all 22 APVTS parameters, WebView editor, tuning module — loads in DAW as instrument

---

## Success Criteria

1. `ninja O-Bowed_VST3 O-Bowed_AU` builds without errors
2. Plugin loads in DAW as instrument (not effect)
3. All 22 parameters visible in DAW parameter list
4. WebView opens at 900x600 with placeholder content
5. No audio output (empty processBlock — expected)

---

## Tasks

### Task 1: Project structure and file scaffolding

Create directory layout and copy tuning module files.

**Actions:**
- Create `plugins/O-Bowed/Source/`
- Create `plugins/O-Bowed/Resources/ui/js/juce/`
- Create `plugins/O-Bowed/Resources/ui/css/`
- Copy tuning C++ files from `modules/tuning/scala-tuning-engine/cpp/` → `Source/`
  - TuningEngine.h/cpp, ScaleGenerator.h/cpp, EmbeddedTunings.h/cpp, TuningExporter.h/cpp
- Copy tuning JS/CSS from module → `Resources/ui/js/` and `Resources/ui/css/`
  - tuning-panel.js, tuning-panel.css
- Copy JUCE bridge files from sibling plugin → `Resources/ui/js/juce/`
  - index.js, check_native_interop.js

**Depends on:** nothing

---

### Task 2: CMakeLists.txt

Create CMakeLists.txt following O-Lyrica/O-Bells pattern.

**Key config:**
- `IS_SYNTH TRUE`, `NEEDS_MIDI_INPUT TRUE`, `NEEDS_WEB_BROWSER TRUE`, `NEEDS_WEBVIEW2 TRUE`
- Plugin code: `OBwd`
- Plugin ID: `ouaricon-bowed`
- All JUCE modules: audio_basics, audio_devices, audio_formats, audio_plugin_client, audio_processors, audio_utils, core, data_structures, dsp, events, graphics, gui_basics, gui_extra
- `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`
- BinaryData from Resources/ui/
- Licensing module (conditional)
- `juce_generate_juce_header(O-Bowed)` after target_link_libraries

**Depends on:** Task 1

---

### Task 3: PluginProcessor.h/cpp

Create processor with APVTS and empty processBlock.

**Key details:**
- Class: `OBowedAudioProcessor`
- BusesProperties: output-only stereo
- `acceptsMidi() = true`, `producesMidi() = false`, `isMidiEffect() = false`
- Static `createParameterLayout()` with all 22 parameters:
  - 18 AudioParameterFloat (bow, body, strings, impossible, output, tuning)
  - 2 AudioParameterInt (stringCount 1-4, sympatheticCount 0-12)
  - 1 AudioParameterChoice (tuningSystem: Scala/MTS/12TET)
- Skew factors: brightness ~0.25, bowSpeed ~0.5, bowPressure ~0.5
- `getAPVTS()` accessor for editor
- Empty processBlock (clear buffer, no audio)
- BowedStringSound placeholder class (for juce::Synthesiser::addSound)

**Depends on:** Task 2

---

### Task 4: PluginEditor.h/cpp

Create editor with WebView shell.

**Key details:**
- Class: `OBowedAudioProcessorEditor`
- **CRITICAL order:** Relays → WebView → Attachments (declaration order in header)
- 20 WebSliderRelay (all float + int params)
- 1 WebComboBoxRelay (tuningSystem)
- WebView with:
  - Backend::webview2
  - WinWebView2 userDataFolder → tempDirectory
  - nativeIntegrationEnabled
  - resourceProvider → getResource()
  - withOptionsFrom for all 21 relays
- 20 WebSliderParameterAttachment + 1 WebComboBoxParameterAttachment
- Resource provider: serves index.html, JUCE bridge JS, tuning JS/CSS
- Window size: 900x600
- paint(): no-op (WebView handles painting)
- resized(): webView->setBounds(getLocalBounds())

**Depends on:** Task 3

---

### Task 5: Placeholder index.html

Create minimal HTML that loads JUCE bridge and shows plugin name.

**Content:**
- Load `/js/juce/index.js` and `/js/juce/check_native_interop.js`
- Display "O-Bowed" centered
- Dark background matching Ouaricon aesthetic
- No parameter controls yet (Stage 3)

**Depends on:** Task 1

---

### Task 6: Register in top-level CMakeLists.txt

Add `add_subdirectory(plugins/O-Bowed)` to the project root CMakeLists.txt.

**Depends on:** Task 2

---

### Task 7: Build and validate

- Run `cmake -B build -G Ninja` to regenerate
- Run `ninja -C build O-Bowed_VST3 O-Bowed_AU`
- Fix any build errors
- Validate against success criteria

**Depends on:** Tasks 1-6

---

## Execution Agent

`foundation-shell-agent` — handles Tasks 1-6 as a single execution pass.
Task 7 (build validation) runs after agent completes.

## Risk Assessment

- **Low risk:** All patterns established in 3+ sibling plugins
- **Known pitfall:** WebView2 flags (mitigated — documented in research)
- **Known pitfall:** Relay/attachment declaration order (mitigated — documented in research)
- **Known pitfall:** Resource provider receives paths not URLs (mitigated — documented in research)
