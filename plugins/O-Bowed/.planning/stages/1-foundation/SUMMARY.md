# O-Bowed Stage 1: Foundation - Summary

**Completed:** 2026-04-04
**Agent:** foundation-shell-agent

## Files Created

### Plugin Source Files
- `plugins/O-Bowed/CMakeLists.txt` - Build system (IS_SYNTH, WebView2, tuning module)
- `plugins/O-Bowed/Source/PluginProcessor.h` - OBowedAudioProcessor class declaration
- `plugins/O-Bowed/Source/PluginProcessor.cpp` - APVTS with 22 parameters, empty processBlock
- `plugins/O-Bowed/Source/PluginEditor.h` - WebView editor with 21 relays + attachments
- `plugins/O-Bowed/Source/PluginEditor.cpp` - WebView setup, resource provider, parameter bindings
- `plugins/O-Bowed/Source/BowedStringSound.h` - Placeholder SynthesiserSound (all notes/channels)

### Tuning Module Integration
- `plugins/O-Bowed/Source/TuningEngine.h` - Include redirect to shared module
- `plugins/O-Bowed/Source/ScaleGenerator.h` - Include redirect to shared module
- `plugins/O-Bowed/Source/EmbeddedTunings.h` - Include redirect to shared module
- `plugins/O-Bowed/Source/TuningExporter.h` - Include redirect to shared module
- CMakeLists.txt references module .cpp files directly via CMAKE_SOURCE_DIR
- CMakeLists.txt adds module cpp/ to include paths

### WebView UI Resources
- `plugins/O-Bowed/Resources/ui/index.html` - Placeholder (dark bg, plugin name centered)
- `plugins/O-Bowed/Resources/ui/js/juce/index.js` - JUCE WebView bridge (framework file)
- `plugins/O-Bowed/Resources/ui/js/juce/check_native_interop.js` - JUCE interop check (framework file)
- Tuning JS/CSS referenced from module directory in BinaryData (not copied)

## Parameters Implemented (22)

### Bow Controls (4)
| ID | Type | Range | Default |
|----|------|-------|---------|
| bowSpeed | Float | 0.02-2.0 | 0.2 |
| bowPressure | Float | 0.01-5.0 | 0.5 |
| bowPosition | Float | 0.02-0.30 | 0.12 |
| rosin | Float | 0.0-1.0 | 0.5 |

### Body Controls (3)
| ID | Type | Range | Default |
|----|------|-------|---------|
| bodyMaterial | Float | 0.0-1.0 | 0.4 |
| bodySize | Float | 0.0-1.0 | 0.5 |
| brightness | Float | 20-20000 Hz | 8000 |

### String Configuration (7)
| ID | Type | Range | Default |
|----|------|-------|---------|
| stringCount | Int | 1-4 | 1 |
| stringTuning1 | Float | -2400-2400 cents | 0 |
| stringTuning2 | Float | -2400-2400 cents | 0 |
| stringTuning3 | Float | -2400-2400 cents | 0 |
| stringTuning4 | Float | -2400-2400 cents | 0 |
| sympatheticAmount | Float | 0.0-1.0 | 0 |
| sympatheticCount | Int | 0-12 | 0 |

### Output (2)
| ID | Type | Range | Default |
|----|------|-------|---------|
| width | Float | 0.0-2.0 | 1.0 |
| outputLevel | Float | -60 to 12 dB | 0 |

### Impossible Physics (3)
| ID | Type | Range | Default |
|----|------|-------|---------|
| infiniteSustain | Float | 0.0-1.0 | 0 |
| reversedFriction | Float | 0.0-1.0 | 0 |
| subHarmonics | Float | 0.0-1.0 | 0 |

### Tuning (2)
| ID | Type | Range | Default |
|----|------|-------|---------|
| referencePitch | Float | 220-880 Hz | 440 |
| tuningSystem | Choice | Scala/MTS/12TET | 12-TET (idx 2) |

## Critical Patterns Applied
- NEEDS_WEBVIEW2 TRUE in juce_add_plugin
- JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1 in compile definitions
- WinWebView2 userDataFolder set to tempDirectory
- Resource provider receives PATHS not URLs (direct comparison)
- Declaration order: Relays -> WebView -> Attachments (destruction safety)
- juce_generate_juce_header(O-Bowed) AFTER target_link_libraries
- include(OuariconModules.cmake) BEFORE juce_add_plugin
- Output-only stereo bus (synth, no input bus)
- juce::ParameterID { "id", 1 } format (JUCE 8)
- 20 WebSliderRelay + 1 WebComboBoxRelay = 21 relays

## Root CMakeLists.txt
No manual registration needed -- auto-discovery via `file(GLOB PLUGIN_DIRS...)` in root CMakeLists.txt.

## Next Steps
- Build verification: `cmake -B build -G Ninja && ninja -C build O-Bowed_VST3 O-Bowed_AU`
- Stage 2: DSP implementation (bow-string friction, waveguide, body resonator)
