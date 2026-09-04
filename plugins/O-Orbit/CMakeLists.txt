cmake_minimum_required(VERSION 3.15)

include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)

# SAF configuration (BEFORE add_subdirectory)
set(SAF_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(SAF_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SAF_ENABLE_SOFA_READER_MODULE OFF CACHE BOOL "" FORCE)
set(SAF_ENABLE_TRACKER_MODULE OFF CACHE BOOL "" FORCE)
set(SAF_ENABLE_HADES_MODULE OFF CACHE BOOL "" FORCE)

add_subdirectory(libs/SAF/framework saf_build)

juce_add_plugin(OuariconOrbit
    COMPANY_NAME "${OUARICON_COMPANY_NAME}"
    PLUGIN_MANUFACTURER_CODE ${OUARICON_MANUFACTURER_CODE}
    PLUGIN_CODE OuOr
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "O-Orbit${OUARICON_DEV_SUFFIX}"
    VERSION 1.2.3
    NEEDS_WEB_BROWSER TRUE
    NEEDS_WEBVIEW2 TRUE
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    IS_MIDI_EFFECT FALSE
)

target_sources(OuariconOrbit
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/Data/SpeakerLayout.cpp
        Source/DSP/MotionEngine.cpp
        Source/DSP/VBAPRenderer.cpp
        Source/DSP/DistanceModel.cpp
        Source/DSP/VBAPDataExchange.cpp
        Source/DSP/PerlinNoise.h
        Source/DSP/DownmixEngine.h
        Source/DSP/VBAPDataExchange.h
)

# Shared modules — adds the module cpp/ include dir. The module's default JS
# copy targets Source/ui/public/modules/, but O-Orbit's UI root is
# Resources/ui/, so the vendored copy is configure-file'd there explicitly.
ouaricon_add_module(OuariconOrbit preset-manager)
configure_file(
    ${OUARICON_MODULES_DIR}/persistence/preset-manager/js/preset-manager.js
    ${CMAKE_CURRENT_SOURCE_DIR}/Resources/ui/js/modules/preset-manager.js
    COPYONLY
)

# WebView UI Resources
# (BinaryData strips hyphens: preset-manager.js -> presetmanager_js)
juce_add_binary_data(OuariconOrbit_WebUI
    SOURCES
        Resources/ui/index.html
        Resources/ui/css/styles.css
        Resources/ui/js/app.js
        # v1.2.0 - interface copy, English + French. Embedded here, served from
        # the getResource() branch in PluginEditor.cpp, imported by js/app.js.
        # All three in one commit or the page 404s at runtime with nothing
        # failing at build time. The symbol is BinaryData::i18n_js and does NOT
        # collide with BinaryData::index_js (js/juce/index.js) already served by
        # the same editor.
        Resources/ui/js/i18n.js
        Resources/ui/js/modules/preset-manager.js
        Resources/ui/js/juce/index.js
        Resources/ui/js/juce/check_native_interop.js
        Resources/ui/img/shell.png
)

target_include_directories(OuariconOrbit
    PRIVATE
        Source
        Source/DSP
        Source/Data
        libs/SAF/framework/include
)

target_link_libraries(OuariconOrbit
    PRIVATE
        OuariconOrbit_WebUI
        saf
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

juce_generate_juce_header(OuariconOrbit)

target_compile_definitions(OuariconOrbit
    PUBLIC
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=1
        JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
        JUCE_USE_CURL=0
)
