cmake_minimum_required(VERSION 3.15)

include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)

juce_add_plugin(O-Reed
    COMPANY_NAME "${OUARICON_COMPANY_NAME}"
    PLUGIN_MANUFACTURER_CODE ${OUARICON_MANUFACTURER_CODE}
    PLUGIN_CODE ORed
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "O-Reed${OUARICON_DEV_SUFFIX}"
    PLUGIN_VERSION "1.1.0"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    NEEDS_WEB_BROWSER TRUE
    NEEDS_WEBVIEW2 TRUE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
)

target_sources(O-Reed
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/ReedWindVoice.cpp
        ${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/cpp/TuningEngine.cpp
        ${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/cpp/ScaleGenerator.cpp
        ${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/cpp/EmbeddedTunings.cpp
        ${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/cpp/TuningExporter.cpp
)

# Phase 24: VST3 Note Expression microtonal support (Dorico)
ouaricon_add_module(O-Reed note-expression)

target_include_directories(O-Reed
    PRIVATE
        Source
        ${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/cpp
        ${CMAKE_SOURCE_DIR}/modules/persistence/preset-manager/cpp
)

target_link_libraries(O-Reed
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

juce_generate_juce_header(O-Reed)

if(OUARICON_LICENSING)
    ouaricon_add_module(O-Reed licensing)
    target_compile_definitions(O-Reed PRIVATE OUARICON_LICENSING_ENABLED=1)
    target_link_libraries(O-Reed PRIVATE juce::juce_cryptography)
endif()

juce_add_binary_data(O-Reed_UIResources
    SOURCES
        Resources/ui/index.html
        Resources/ui/js/juce/index.js
        Resources/ui/js/juce/check_native_interop.js
        ${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/js/tuning-panel.js
        ${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/snippets/tuning-panel.css
)

target_link_libraries(O-Reed
    PRIVATE
        O-Reed_UIResources
)

target_compile_definitions(O-Reed
    PUBLIC
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=1
        JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
        JUCE_USE_CURL=0
)
