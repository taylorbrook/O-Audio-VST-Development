# O-Wind Stage 1: Foundation — Execution Plan

**Created:** 2026-04-04
**Source:** CONTEXT.md, RESEARCH.md, parameter-spec-draft.md
**Reference:** O-Bowed Stage 1 (1:1 template)

---

## Goal

Create project structure, CMakeLists.txt, APVTS with all 14 parameters + 2 tuning parameters, WebView shell editor, and placeholder HTML. Plugin builds and loads in DAW as instrument with no audio.

**Parameter count resolution:** Spec table lists 14 distinct parameters but summary says "13." All 14 are meaningful — implementing all 14. The "13" is a typo in the summary.

---

## Tasks

### 1. [ ] Create directory structure

Create the file tree:

```
plugins/O-Wind/
    CMakeLists.txt
    Source/
        PluginProcessor.h
        PluginProcessor.cpp
        PluginEditor.h
        PluginEditor.cpp
        FluteSynthSound.h
    Resources/
        ui/
            index.html
            js/
                juce/
                    index.js
                    check_native_interop.js
```

- Copy `index.js` and `check_native_interop.js` from `plugins/O-Bowed/Resources/ui/js/juce/`
- **Depends on:** nothing

### 2. [ ] Create CMakeLists.txt

Clone O-Bowed CMakeLists.txt, substituting:
- Plugin name: `O-Wind`
- Plugin code: `OWnd`
- Description: Physical Modeling Flute Synthesizer
- `IS_SYNTH TRUE`, `NEEDS_MIDI_INPUT TRUE`
- `NEEDS_WEB_BROWSER TRUE`, `NEEDS_WEBVIEW2 TRUE`
- `EDITOR_WANTS_KEYBOARD_FOCUS FALSE`

Include:
- All JUCE modules (same set as O-Bowed)
- Tuning module sources from `${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/cpp/`
- Tuning module include path
- Binary data target `O-Wind_UIResources` with: `index.html`, `index.js`, `check_native_interop.js`, `tuning-panel.js`, `tuning-panel.css`
- Compile definitions: `JUCE_WEB_BROWSER=1`, `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`, `JUCE_VST3_CAN_REPLACE_VST2=0`, `JUCE_USE_CURL=0`
- Conditional licensing block
- `juce_generate_juce_header()` AFTER `target_link_libraries()`

- **Files:** `plugins/O-Wind/CMakeLists.txt`
- **Depends on:** Task 1

### 3. [ ] Create FluteSynthSound.h

Minimal `SynthesiserSound` stub (catch-all, no voice impl at Stage 1):

```cpp
class FluteSynthSound : public juce::SynthesiserSound
{
public:
    bool appliesToNote(int) override { return true; }
    bool appliesToChannel(int) override { return true; }
};
```

- **Files:** `Source/FluteSynthSound.h`
- **Depends on:** Task 1

### 4. [ ] Create PluginProcessor.h/cpp with APVTS

**Header:** `OWindAudioProcessor` extending `juce::AudioProcessor`
- Members: `parameters` (APVTS), `synthesiser`, `tuningEngine`
- Public accessors: `getAPVTS()`, `getTuningEngine()`
- Static `createParameterLayout()`

**Implementation:**
- Constructor: output-only stereo bus, APVTS init, add `FluteSynthSound`
- `createParameterLayout()` with all 16 parameters (14 plugin + 2 tuning):

| camelCase ID | Display Name | Range | Default | Step |
|---|---|---|---|---|
| breathPressure | Breath Pressure | 0.0–1.0 | 0.5 | 0.01 |
| embouchure | Embouchure | 0.0–1.0 | 0.5 | 0.01 |
| breathNoise | Breath Noise | 0.0–1.0 | 0.15 | 0.01 |
| toneColor | Tone Color | 0.0–1.0 | 0.5 | 0.01 |
| airColumn | Air Column | 0.0–1.0 | 0.5 | 0.01 |
| jetReflection | Jet Reflection | -1.0–1.0 | 0.5 | 0.01 |
| endReflection | End Reflection | -1.0–1.0 | 0.5 | 0.01 |
| vibratoRate | Vibrato Rate | 2.0–8.0 | 5.0 | 0.01 |
| vibratoDepth | Vibrato Depth | 0.0–1.0 | 0.3 | 0.01 |
| width | Width | 0.0–2.0 | 1.0 | 0.01 |
| outputLevel | Output Level | -60.0–12.0 | 0.0 | 0.1 |
| infiniteSustain | Infinite Sustain | 0.0–1.0 | 0.0 | 0.01 |
| reversedJet | Reversed Jet | 0.0–1.0 | 0.0 | 0.01 |
| subHarmonics | Sub-Harmonics | 0.0–1.0 | 0.0 | 0.01 |
| referencePitch | Reference Pitch | 400.0–480.0 | 440.0 | 0.1 |
| tuningSystem | Tuning System | choice(0–2) | 0 | — |

- All use `juce::ParameterID { "id", 1 }` (version 1)
- `processBlock`: `ScopedNoDenormals`, `buffer.clear()` (no audio at Stage 1)
- `getStateInformation` / `setStateInformation`: APVTS XML round-trip

- **Files:** `Source/PluginProcessor.h`, `Source/PluginProcessor.cpp`
- **Depends on:** Tasks 1, 3

### 5. [ ] Create PluginEditor.h/cpp with WebView shell

**Header:** `OWindAudioProcessorEditor` extending `juce::AudioProcessorEditor`

**Member declaration order** (destruction order matters):
1. Relays (`std::unique_ptr<juce::WebSliderRelay>`) — 14 plugin params
2. WebView (`std::unique_ptr<juce::WebBrowserComponent>`)
3. Attachments (`std::unique_ptr<juce::WebSliderParameterAttachment>`) — 14 plugin params

**Constructor:**
1. Create all 14 relays
2. Build WebView with:
   - `.withBackend(webview2)`
   - `.withWinWebView2Options(...)` with temp `userDataFolder`
   - `.withNativeIntegrationEnabled()`
   - `.withResourceProvider(...)` callback
   - `.withOptionsFrom(...)` for each relay
3. `addAndMakeVisible(*webView)`
4. Create all 14 attachments
5. `webView->goToURL(getResourceProviderRoot())`
6. `setSize(900, 600)`

**Resource provider:** bare-path matching:
- `/` or `/index.html` → `BinaryData::index_html`
- `/js/juce/index.js` → `BinaryData::index_js`
- `/js/juce/check_native_interop.js` → `BinaryData::check_native_interop_js`
- `/js/tuning-panel.js` → `BinaryData::tuningpanel_js`
- `/css/tuning-panel.css` → `BinaryData::tuningpanel_css`

**resized():** `webView->setBounds(getLocalBounds())`

- **Files:** `Source/PluginEditor.h`, `Source/PluginEditor.cpp`
- **Depends on:** Tasks 1, 4

### 6. [ ] Create placeholder index.html

Dark-themed placeholder with:
- Title: "O-Wind"
- Subtitle: "Physical Modeling Flute Synthesizer"
- Stage indicator: "Stage 1 - Foundation Shell"
- Script tags for `/js/juce/index.js` and `/js/juce/check_native_interop.js`

- **Files:** `Resources/ui/index.html`
- **Depends on:** Task 1

### 7. [ ] Register O-Wind in root CMakeLists.txt

Add `add_subdirectory(plugins/O-Wind)` to the root CMakeLists.txt alongside other plugins.

- **Files:** `CMakeLists.txt` (root)
- **Depends on:** Task 2

### 8. [ ] Build and validate

```bash
cd build && cmake .. -G Ninja && ninja O-Wind_VST3 O-Wind_AU
```

Then install + verify:
```bash
killall -9 AudioComponentRegistrar 2>/dev/null || true
rm -rf ~/Library/Caches/AudioUnitCache/
rm -rf ~/Library/Caches/com.apple.audiounits.cache
rm -rf ~/Library/Audio/Plug-Ins/VST3/O-Wind.vst3
rm -rf ~/Library/Audio/Plug-Ins/Components/O-Wind.component
cp -R build/plugins/O-Wind/O-Wind_artefacts/Release/VST3/O-Wind.vst3 ~/Library/Audio/Plug-Ins/VST3/
cp -R build/plugins/O-Wind/O-Wind_artefacts/Release/AU/O-Wind.component ~/Library/Audio/Plug-Ins/Components/
auval -a | grep -i wind
```

- **Depends on:** Tasks 2–7

---

## Success Criteria

- [ ] CMake configures without errors
- [ ] `ninja O-Wind_VST3 O-Wind_AU` builds cleanly (zero warnings preferred)
- [ ] Plugin loads in DAW as instrument (not effect)
- [ ] WebView opens with placeholder UI at 900x600
- [ ] All 14 plugin parameters visible in DAW parameter list
- [ ] 2 tuning parameters visible in DAW parameter list
- [ ] State save/restore works (parameter values survive session reload)
- [ ] `auval -a | grep -i wind` shows AU component
- [ ] No crashes on open/close

---

## Execution Notes

- **Template:** Clone O-Bowed files, find-replace names/IDs. Do not hand-roll.
- **No DSP:** processBlock clears buffer. No SynthesiserVoice at this stage.
- **No tuning logic:** TuningEngine member exists but isn't called in processBlock.
- **14 relays + 14 attachments** in editor (not 16 — tuning params don't get relays at Stage 1, they're handled by the tuning panel JS directly in later stages).
