cmake_minimum_required(VERSION 3.15)

# Include Ouaricon Module System
include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)

# Plugin formats: VST3, AU, Standalone
juce_add_plugin(O-simpleReverb
    VERSION "3.0.0"
    COMPANY_NAME "${OUARICON_COMPANY_NAME}"
    PLUGIN_MANUFACTURER_CODE ${OUARICON_MANUFACTURER_CODE}
    PLUGIN_CODE OuSr
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "O-simpleReverb${OUARICON_DEV_SUFFIX}"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    NEEDS_WEB_BROWSER TRUE
    NEEDS_WEBVIEW2 TRUE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    # v3.0.0: browsable as a reverb in VST3 hosts. Metadata only: the plugin
    # UID (OuSr + manufacturer) and the saved state are unchanged.
    VST3_CATEGORIES "Fx" "Reverb"
)

# Source files
# Source files. PluginProcessor.cpp includes PluginEditor.h only under
# #if JUCE_WEB_BROWSER (Generic editor in the #else), so render-check and
# param-dump (JUCE_WEB_BROWSER=0, no editor sources) keep building.
# MeterFramePayload.h is JUCE-free and shared with render-check.
target_sources(O-simpleReverb
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/PluginProcessor.h
        Source/PluginEditor.h
        Source/PresetRecipes.h
        Source/MeterFrame.h
        Source/MeterFramePayload.h
        Source/TripleBuffer.h
        Source/ModulationFx.h
)

# Include paths
target_include_directories(O-simpleReverb
    PRIVATE
        Source
)

# Required JUCE modules
target_link_libraries(O-simpleReverb
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

# Generate JuceHeader.h (JUCE 8 requirement)
juce_generate_juce_header(O-simpleReverb)

# Add Ouaricon Preset Manager module
ouaricon_add_module(O-simpleReverb preset-manager)

# WebView UI Resources
juce_add_binary_data(O-simpleReverb_UIResources
    SOURCES
        Source/ui/public/index.html
        Source/ui/public/js/i18n.js
        Source/ui/public/js/juce/index.js
        Source/ui/public/js/juce/check_native_interop.js
        # v3.0.0: the shared preset bar module (configure-copied from
        # modules/persistence/preset-manager/js, gitignored) and the family
        # insects overlay. paper.jpg and flora.png were the v2 page's.
        Source/ui/public/modules/preset-manager.js
        Source/ui/public/img/insects.png
        # v1.10.0 - bundled EB Garamond, direct embed from the shared
        # module; served by four getResource branches in PluginEditor.cpp.
        ${CMAKE_SOURCE_DIR}/modules/ui/eb-garamond/css/eb-garamond.css
        ${CMAKE_SOURCE_DIR}/modules/ui/eb-garamond/fonts/EBGaramond-Regular.woff2
        ${CMAKE_SOURCE_DIR}/modules/ui/eb-garamond/fonts/EBGaramond-Italic.woff2
        ${CMAKE_SOURCE_DIR}/modules/ui/eb-garamond/fonts/EBGaramond-Bold.woff2
)

target_link_libraries(O-simpleReverb
    PRIVATE
        O-simpleReverb_UIResources
)

# Compile definitions
target_compile_definitions(O-simpleReverb
    PUBLIC
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=1
        JUCE_USE_CURL=0
        JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
)

# Test targets — off by default, enable with -DOUARICON_BUILD_TESTS=ON.
# Suite-wide cache option (same name as the other plugins' gates, deliberately).
option(OUARICON_BUILD_TESTS "Build O-simpleReverb test targets" OFF)

if(OUARICON_BUILD_TESTS)
    # Runtime parameter inventory (scripts/param-dump). A regex over
    # createParameterLayout() is not authoritative: IDs can be concatenated and
    # parameters can come from a factory lambda. Only a walk of
    # AudioProcessor::getParameters() on a constructed processor sees the real set.
    include(${CMAKE_SOURCE_DIR}/scripts/param-dump/ParamDump.cmake)
    ouaricon_add_param_dump(O-simpleReverb ${CMAKE_CURRENT_SOURCE_DIR}/Source)

    # Render check: factory TYPE recall and bank levels, click-free CHARACTER,
    # LOW CUT, TYPE, SIZE and DECAY moves, per-type level match and the level
    # law, VU peak hold, and (v2.0.0) the reverb engines - decay time against
    # target, early reflections, stereo, ring-out, no allocation.
    # See tests/render-check/main.cpp.
    ouaricon_add_processor_console(O-simpleReverb ${CMAKE_CURRENT_SOURCE_DIR}/Source
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/render-check/main.cpp render-check)
    # Mutants and the NaN injection hook exist in this target only (see
    # OSR_MUTANT in Source/PluginProcessor.h); the plugin is built without them.
    target_compile_definitions(O-simpleReverb-render-check PRIVATE OSIMPLEREVERB_TEST_HOOKS=1)
endif()
