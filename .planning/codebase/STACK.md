# Technology Stack

**Analysis Date:** 2026-01-29

## Languages

**Primary:**
- C++ (C++17 minimum) - Audio plugin engine and DSP algorithms
- JavaScript/HTML/CSS - WebView UI for plugin interfaces

**Secondary:**
- Bash - Build and installation scripts
- YAML - Module registry and workflow configuration
- CMake - Cross-platform build configuration

## Runtime

**Environment:**
- macOS 10.13+ (deployment target, Sonoma+ recommended for development)
- Apple Silicon (ARM64) and Intel (x86_64) universal binaries supported

**Compiler:**
- Clang/LLVM (Apple's default C++ compiler via Xcode)
- CMake 3.22+ (build system generator)
- Xcode Command Line Tools

## Frameworks

**Core:**
- JUCE 8.0.9 - Cross-platform audio application and VST/AU plugin framework
  - Purpose: Audio processing, parameter management, DAW integration, WebView support

**Plugin Formats:**
- VST3 - Primary plugin format for cross-DAW compatibility
- AU (AudioUnit) - macOS native plugin format
- Standalone - Self-contained application builds for testing

**Audio Processing:**
- JUCE::juce_dsp - Digital Signal Processing module (filters, convolution, windowing)
- JUCE::juce_audio_basics - Audio buffer management and core audio utilities
- JUCE::juce_audio_processors - Plugin parameter and state management

**UI Framework:**
- JUCE::juce_gui_basics - Native UI components (fallback for non-WebView plugins)
- JUCE WebView - HTML/CSS/JavaScript rendering for custom plugin UIs
- JUCE::juce_events - Event handling and message threading

**Supporting JUCE Modules:**
- juce_audio_devices - Audio device enumeration and configuration
- juce_audio_formats - Audio file I/O support
- juce_audio_utils - Utilities (tuning, pitch detection, audio analysis)
- juce_core - File I/O, threading, memory management
- juce_data_structures - Collections, trees, value trees
- juce_graphics - 2D drawing and image handling
- juce_gui_extra - Advanced GUI components

## Key Dependencies

**Critical:**
- JUCE 8.0.9 (`/Users/taylorbrook/JUCE/`) - Entire plugin development framework
- Xcode Command Line Tools - C++ compiler and standard library

**Build Tools:**
- CMake 4.2.1+ - Project configuration and build generation
- Ninja - Fast parallel build execution (configured via CMake)

**Validation & Testing:**
- pluginval - Validates VST3 and AU plugin compliance
- Python 3.8+ - Build script execution

**No External Runtime Dependencies:**
- Plugins are self-contained binaries with JUCE statically linked
- No network dependencies (JUCE_USE_CURL=0 across all plugins)
- No database or external service connections required

## Configuration

**Environment:**
- macOS deployment target: `CMAKE_OSX_DEPLOYMENT_TARGET = 10.13`
- Architecture: Universal binary (arm64 + x86_64)
- C++ standard: C++17+

**Build Configuration:**
- CMakeLists.txt at project root: `/Users/taylorbrook/Dev/VST-development/CMakeLists.txt`
- JUCE root: `/Users/taylorbrook/JUCE/` (external directory, version 8.0.9)
- Build output: `/Users/taylorbrook/Dev/VST-development/build/`
- Each plugin has its own CMakeLists.txt: `plugins/[PluginName]/CMakeLists.txt`

**Plugin Configuration Pattern:**
```cmake
juce_add_plugin(PluginName
    FORMATS VST3 AU Standalone
    NEEDS_WEB_BROWSER TRUE
)

target_compile_definitions(PluginName
    PUBLIC
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=1
        JUCE_USE_CURL=0
)
```

**WebView Resources:**
- HTML/CSS/JavaScript files embedded as binary data via `juce_add_binary_data()`
- No external resource loading or CDN dependencies
- Resources compiled into plugin binary

## Platform Requirements

**Development:**
- macOS 13.0+ (Sonoma 14+ recommended)
- 16GB RAM recommended (8GB minimum)
- 2GB free disk space per plugin
- Xcode 14+ with Command Line Tools

**Production (DAW Integration):**
- VST3 plugins: `~/Library/Audio/Plug-Ins/VST3/[PluginName].vst3/`
- AU plugins: `~/Library/Audio/Plug-Ins/Components/[PluginName].component/`
- Compatible with any DAW supporting VST3 or AU (Ableton, Logic Pro, Reaper, etc.)
- No additional runtime dependencies required

## Build System Details

**Build Workflow:**
1. CMake generates Ninja build files from plugin CMakeLists.txt
2. Ninja compiles C++ sources in parallel
3. `juce_generate_juce_header()` creates platform-specific header (JUCE 8 requirement)
4. WebView resources embedded as binary data during compilation
5. VST3 and AU formats built simultaneously from single source
6. Binaries installed to system plugin directories

**Build Script:**
- Location: `scripts/build-and-install.sh`
- 7-phase pipeline: validation → parallel build → extraction → cleanup → installation → cache clearing → verification
- Supports flags: `--dry-run`, `--no-install`, `--verbose`, `--reconfigure`
- Logging: `logs/[PluginName]/build_YYYYMMDD_HHMMSS.log`

**Key Build Flags:**
- `JUCE_WEB_BROWSER=1` - Enable HTML/CSS/JS UI rendering
- `JUCE_USE_CURL=0` - Disable network functionality
- `JUCE_VST3_CAN_REPLACE_VST2=0` - Maintain VST2 compatibility layer
- `-flto` (Link Time Optimization) - Applied via recommended flags

---

*Stack analysis: 2026-01-29*
