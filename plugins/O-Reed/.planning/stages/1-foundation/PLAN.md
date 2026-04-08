# O-Reed Stage 1: Foundation - Execution Plan

**Created:** 2026-04-04
**Goal:** Create buildable plugin shell — CMakeLists.txt, APVTS with all 35 parameters, MPESynthesiser with silent voice stubs, WebView editor with relay/attachment bindings, tuning module link. Builds and loads in DAW as instrument (no audio).

**Reference plugins:** O-Bowed (CMake, WebView, tuning), O-Formant (MPESynthesiser, MPESynthesiserVoice)

---

## Tasks

### 1. [ ] Create CMakeLists.txt

**Files:** `plugins/O-Reed/CMakeLists.txt`
**Depends on:** none

Build configuration matching O-Bowed pattern:
- `include(OuariconModules.cmake)` before `juce_add_plugin`
- Plugin code `ORed`, IS_SYNTH TRUE, NEEDS_MIDI_INPUT TRUE
- NEEDS_WEB_BROWSER TRUE, NEEDS_WEBVIEW2 TRUE
- EDITOR_WANTS_KEYBOARD_FOCUS FALSE
- Source files: PluginProcessor.cpp, PluginEditor.cpp, ReedWindVoice.cpp
- Tuning module: 4 .cpp files from `modules/tuning/scala-tuning-engine/cpp/`
- Include dirs: Source, tuning module cpp dir
- Link: all standard JUCE modules (audio_basics through gui_extra) + juce_dsp
- `juce_generate_juce_header(O-Reed)` after target_link_libraries
- BinaryData: index.html, index.js, check_native_interop.js, tuning-panel.js, tuning-panel.css
- Compile defs: JUCE_VST3_CAN_REPLACE_VST2=0, JUCE_WEB_BROWSER=1, JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1, JUCE_USE_CURL=0
- Conditional licensing block (ouaricon_add_module pattern)

### 2. [ ] Create ReedWindVoice (MPESynthesiserVoice skeleton)

**Files:** `plugins/O-Reed/Source/ReedWindVoice.h`, `plugins/O-Reed/Source/ReedWindVoice.cpp`
**Depends on:** none

Silent stub implementing all 7 MPESynthesiserVoice pure virtuals:
- noteStarted, noteStopped, notePressureChanged, notePitchbendChanged, noteTimbreChanged, noteKeyStateChanged, renderNextBlock
- All methods are no-ops; renderNextBlock outputs silence
- `setAPVTS(AudioProcessorValueTreeState*)` method caching all 35 atomic parameter pointers
- Constructor takes voiceIndex
- No SynthesiserSound needed (MPESynthesiser manages voices directly)

### 3. [ ] Create PluginProcessor with APVTS + MPESynthesiser

**Files:** `plugins/O-Reed/Source/PluginProcessor.h`, `plugins/O-Reed/Source/PluginProcessor.cpp`
**Depends on:** Task 2

OReedAudioProcessor with:
- BusesProperties: output-only stereo
- Static `createParameterLayout()` with all 35 parameters:
  - 27 AudioParameterFloat (0-1 normalized, except TONE_HOLE_CUTOFF 200-8000 Hz skew 0.3, VIBRATO_RATE 1-10 Hz, REFERENCE_PITCH 220-880 Hz, DRONE_PITCH -24 to +24 st, OUTPUT_GAIN -60 to +12 dB)
  - 6 AudioParameterChoice (INSTRUMENT_PRESET 21 choices, BORE_PROFILE 2, VIBRATO_SOURCE 3, TUNING_SYSTEM 3, POLY_MODE 2, OVERSAMPLING 2)
  - 1 AudioParameterBool (DUAL_BORE)
  - 1 AudioParameterInt (MAX_VOICES 1-16 default 8)
- Parameter IDs: camelCase (breathPressure, embouchure, reedHardness, etc.)
- MPESynthesiser with 16 ReedWindVoice instances
- `enableLegacyMode(2, Range<int>(1, 17))` AFTER addVoice calls
- TuningEngine member + public accessor
- processBlock: ScopedNoDenormals, buffer.clear(), synthesiser.renderNextBlock()
- getStateInformation / setStateInformation (APVTS XML pattern)
- isBusesLayoutSupported: stereo output only
- Plugin name "O-Reed", acceptsMidi true, producesMidi false, isSynth true

### 4. [ ] Create PluginEditor with WebView + 35 Relays/Attachments

**Files:** `plugins/O-Reed/Source/PluginEditor.h`, `plugins/O-Reed/Source/PluginEditor.cpp`
**Depends on:** Task 3

OReedAudioProcessorEditor with:
- Member declaration order: RELAYS first, WEBVIEW second, ATTACHMENTS last
- 28 WebSliderRelay (27 float + 1 int for maxVoices)
- 6 WebComboBoxRelay (choice params)
- 1 WebToggleButtonRelay (dualBore)
- WebBrowserComponent with:
  - Backend::webview2
  - WinWebView2 userDataFolder in temp dir
  - withNativeIntegrationEnabled()
  - withResourceProvider (bare path matching)
  - .withOptionsFrom() for all 35 relays
- 28 WebSliderParameterAttachment
- 6 WebComboBoxParameterAttachment
- 1 WebToggleButtonParameterAttachment
- getResource() matching O-Bowed pattern (direct path equality, no scheme stripping)
- makeResource() helper
- Window size: 900x600, setResizable false

### 5. [ ] Create WebView resources (HTML + JS bridge files)

**Files:**
- `plugins/O-Reed/Resources/ui/index.html`
- `plugins/O-Reed/Resources/ui/js/juce/index.js` (copy from O-Bowed)
- `plugins/O-Reed/Resources/ui/js/juce/check_native_interop.js` (copy from O-Bowed)

**Depends on:** none

- Placeholder HTML: dark theme, "O-Reed" title, "Physical Modeling Reed Wind Synthesizer" subtitle, "Stage 1 - Foundation Shell" label
- Script tags for /js/juce/index.js and /js/juce/check_native_interop.js
- Copy JUCE bridge JS files verbatim from O-Bowed

### 6. [ ] Verify build compiles

**Depends on:** Tasks 1-5

- Run `cmake -B build -G Ninja` from project root
- Run `ninja -C build O-Reed_VST3 O-Reed_AU`
- Fix any compilation errors
- Verify plugin binary exists in build output

---

## Success Criteria

- [ ] CMake configures without errors
- [ ] Builds VST3 + AU + Standalone without warnings (beyond JUCE internal)
- [ ] Plugin loads in DAW as instrument (not effect)
- [ ] MIDI input accepted (shown in DAW I/O)
- [ ] WebView displays placeholder UI at 900x600
- [ ] All 35 parameters visible in DAW parameter list
- [ ] State save/load works (DAW session recall)
- [ ] No audio output (silent — expected for Stage 1)
- [ ] No crashes on load, parameter change, or close

---

## Notes

- INSTRUMENT_PRESET choices: Bb Clarinet, Bass Clarinet, Alto Saxophone, Tenor Saxophone, Soprano Saxophone, Baritone Saxophone, Oboe, English Horn, Bassoon, Duduk, Shehnai, Suona, Hichiriki, Zurna, Piri, Arghul, Launeddas, Mijwiz, Glass Reed, Metal Wind, Impossible Bore
- BORE_PROFILE choices: Simple, Multi-segment
- VIBRATO_SOURCE choices: Lip, Breath, Throat
- TUNING_SYSTEM choices: Scala/TUN, MTS-ESP, 12-TET
- POLY_MODE choices: Monophonic, Polyphonic
- OVERSAMPLING choices: 2x, 4x
- enableLegacyMode MUST be called AFTER addVoice — order matters
- TONE_HOLE_CUTOFF uses skew 0.3f for finer low-frequency control
- WebToggleButtonRelay verified in JUCE 8.0.4 source for bool params
