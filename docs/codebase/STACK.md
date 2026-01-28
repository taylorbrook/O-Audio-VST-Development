# Technology Stack

**Analysis Date:** 2026-01-22

## Languages

**Primary:**
- C++ (C++17 standard) - Core DSP engine, plugin logic, and voice processing
  - Used in: `plugins/OuariconLyrica/Source/` and all plugin implementations
  - JUCE-based audio processing with physical modeling synthesis

**Secondary:**
- JavaScript (ES6) - WebView UI controllers and interop bridge
  - Used in: `plugins/OuariconLyrica/Resources/ui/js/` for web-based plugin interface
  - HTML/CSS for visual UI rendering within JUCE WebBrowserComponent

## Runtime

**Environment:**
- macOS 10.13+ (deployment target set in CMake)
- JUCE framework handles OS abstraction

**Compiler:**
- CMake 3.22+ with system compiler (Clang on macOS)
- C++ standard: C++17

## Frameworks

**Core:**
- JUCE 8.0.9 - Cross-platform audio plugin framework
  - Audio processing modules: juce::juce_audio_basics, juce::juce_audio_processors, juce::juce_audio_devices
  - DSP module: juce::juce_dsp for filter design and signal processing
  - GUI framework: juce::juce_gui_basics, juce::juce_gui_extra, juce::juce_graphics
  - Web UI: juce::juce_webview for embedded WebView UI
  - Data structures: juce::juce_data_structures, juce::juce_core
  - Events and timers: juce::juce_events

**Build:**
- CMake 3.15+ - Multi-plugin build system with auto-discovery
- JUCE's CMake helper functions (juce_add_plugin, juce_add_binary_data)

**Testing:**
- Not detected (no test framework configured in codebase)

## Key Dependencies

**Critical:**
- JUCE framework 8.0.9 - Absolute requirement
  - Location: `/Users/taylorbrook/JUCE` (external installation)
  - All plugins depend on JUCE modules for audio I/O, parameter management, and UI rendering
  - Referenced in: `CMakeLists.txt` line 9: `add_subdirectory(/Users/taylorbrook/JUCE JUCE)`

**Standard Library:**
- std::atomic - Lock-free synchronization for MIDI event queue
  - Used in: `plugins/OuariconLyrica/Source/PluginProcessor.h` (MidiEventQueue)
- std::array - Fixed-size container for MIDI events
- std::vector - Dynamic array for frequency data, preset lists
- std::unique_ptr - Smart memory management throughout codebase
- std::functional - Callback support for preset manager custom state

**Persistence:**
- JSON (JUCE's native JSON support) - Preset serialization
  - Used in: `modules/persistence/preset-manager/cpp/OuariconPresetManager.h`
  - Files stored as: `~/Library/{pluginName}/Presets/Factory/` and `~/Library/{pluginName}/Presets/User/`
- XML (JUCE's XmlElement) - DAW session state serialization
  - Used for: getStateInformation/setStateInformation plugin API calls

## Configuration

**CMake Configuration:**
- `CMakeLists.txt` at project root - Master build config
- Per-plugin CMakeLists.txt files:
  - `plugins/OuariconLyrica/CMakeLists.txt` - Plugin format configuration
  - Plugin formats: VST3, AU (Audio Unit), Standalone app
- Deployment target: macOS 10.13 (CMAKE_OSX_DEPLOYMENT_TARGET in root CMakeLists.txt)

**Plugin Metadata:**
- Company: "Ouaricon Audio"
- Plugin manufacturer code: OuAu
- Plugin codes vary per plugin (e.g., OLyr for OuariconLyrica)

**Binary Data Embedding:**
- Web UI resources embedded via juce_add_binary_data:
  - HTML: `Resources/ui/index.html`
  - CSS: `Resources/ui/css/styles.css`
  - JavaScript: `Resources/ui/js/app.js`, `Resources/ui/js/juce/index.js`
  - Images: `Resources/ui/images/`
  - Used in: `plugins/OuariconLyrica/CMakeLists.txt` lines 68-77

**Compile Definitions:**
- JUCE_VST3_CAN_REPLACE_VST2=0 - Disable VST2 replacement in VST3
- JUCE_WEB_BROWSER=1 - Enable WebView UI support
- JUCE_USE_CURL=0 - Disable network/curl (no external HTTP)

## Platform Requirements

**Development:**
- macOS 10.13 or later
- CMake 3.22+
- C++17 compatible compiler (Clang)
- JUCE 8.0.9 installation (prebuilt, located at `/Users/taylorbrook/JUCE`)

**Production (Plugin Installation):**
- macOS 10.13+
- VST3-compatible DAW (for VST3 plugin)
- AU-compatible DAW (for AU plugin)
- Standalone executable requires no DAW

## No External Dependencies

The codebase explicitly disables external dependencies:
- No network calls (JUCE_USE_CURL=0)
- No web APIs or webhooks
- No third-party DSP libraries (all DSP custom-implemented)
- All persistence is local filesystem only
- Audio I/O managed through JUCE's abstraction layer

---

*Stack analysis: 2026-01-22*
