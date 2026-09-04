cmake_minimum_required(VERSION 3.22)

include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)

# =============================================================================
# ONNX Runtime, provisioned by the ANIRA fetch. Inference runs on our own
# thread via the raw ORT C++ API, so anira itself is NOT linked or embedded —
# the fetch is kept solely for its per-platform ONNX Runtime download logic,
# which exports ANIRA_ONNXRUNTIME_SHARED_LIB_PATH (arch/OS-correct root dir).
# =============================================================================
include(FetchContent)
FetchContent_Declare(
    anira
    GIT_REPOSITORY https://github.com/anira-project/anira.git
    GIT_TAG v2.1.0
)
set(ANIRA_WITH_ONNXRUNTIME ON CACHE BOOL "" FORCE)
set(ANIRA_WITH_LIBTORCH OFF CACHE BOOL "" FORCE)
set(ANIRA_WITH_TFLITE OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(anira)

set(ONNXRUNTIME_VERSION 1.19.2)
if(APPLE)
    set(ONNXRUNTIME_LIB "${ANIRA_ONNXRUNTIME_SHARED_LIB_PATH}/lib/libonnxruntime.${ONNXRUNTIME_VERSION}.dylib")
elseif(WIN32)
    set(ONNXRUNTIME_LIB "${ANIRA_ONNXRUNTIME_SHARED_LIB_PATH}/lib/onnxruntime.lib")
else()
    set(ONNXRUNTIME_LIB "${ANIRA_ONNXRUNTIME_SHARED_LIB_PATH}/lib/libonnxruntime.so")
endif()

# =============================================================================
# Plugin target
# =============================================================================
set(PROJECT_NAME OuariconTexture)

juce_add_plugin(${PROJECT_NAME}
    COMPANY_NAME "${OUARICON_COMPANY_NAME}"
    PLUGIN_MANUFACTURER_CODE ${OUARICON_MANUFACTURER_CODE}
    PLUGIN_CODE OuTx
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "O-Texture${OUARICON_DEV_SUFFIX}"
    VERSION 0.4.0
    IS_SYNTH TRUE
    # MIDI input is required by the AU spec for MusicDevice (aumu) — auval
    # fails without it — even though v0.1.x ignores all MIDI (IN-07).
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    NEEDS_WEB_BROWSER TRUE
    NEEDS_WEBVIEW2 TRUE
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

target_sources(${PROJECT_NAME}
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/DSP/HannWindow.h
        Source/DSP/OverlapAddProcessor.h
        Source/DSP/PerlinNoise1D.h
        Source/DSP/TiltFilter.h
)

target_include_directories(${PROJECT_NAME}
    PRIVATE
        Source
        "${ANIRA_ONNXRUNTIME_SHARED_LIB_PATH}/include"
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        "${ONNXRUNTIME_LIB}"
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

juce_generate_juce_header(${PROJECT_NAME})

# =============================================================================
# Binary data: ONNX models
# =============================================================================
# encoder.onnx / prior.onnx stay on disk but are not embedded — only the
# decoder and dim map are loaded until Transform mode lands.
juce_add_binary_data(${PROJECT_NAME}_ModelData
    NAMESPACE ModelData
    HEADER_NAME ModelData.h
    SOURCES
        Resources/models/placeholder/decoder.onnx
        Resources/models/rain/dim_map_rain.json
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        ${PROJECT_NAME}_ModelData
)

# =============================================================================
# Binary data: WebView UI resources
# =============================================================================
juce_add_binary_data(${PROJECT_NAME}_UIResources
    SOURCES
        Source/ui/public/index.html
        Source/ui/public/css/ouaricon-naturalist.css
        Source/ui/public/js/juce/index.js
        Source/ui/public/js/juce/check_native_interop.js
        Source/ui/public/js/main.js
        # v0.2.0: the label table. EMBEDDED here AND served from a
        # getResource() branch in PluginEditor.cpp, in the same commit. A file
        # embedded but not served, or served but not embedded, is a 404 that
        # presents as a page stuck in English and nothing else — check-i18n
        # assertion 8 exists for exactly this pair.
        Source/ui/public/js/i18n.js
        Source/ui/public/img/fern.png
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        ${PROJECT_NAME}_UIResources
)

# =============================================================================
# Compile definitions
# =============================================================================
target_compile_definitions(${PROJECT_NAME}
    PUBLIC
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=1
        JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
        JUCE_USE_CURL=0
)

# =============================================================================
# macOS: Embed the ONNX Runtime shared library in plugin bundles.
# The plugin links libonnxruntime directly (install name
# @rpath/libonnxruntime.<version>.dylib), resolved via INSTALL_RPATH.
# =============================================================================
if(APPLE)
    foreach(FORMAT VST3 AU Standalone)
        set(TARGET_NAME ${PROJECT_NAME}_${FORMAT})
        if(TARGET ${TARGET_NAME})
            # Set rpath so the plugin finds libraries in its Frameworks/ directory
            set_target_properties(${TARGET_NAME} PROPERTIES
                BUILD_RPATH ""
                INSTALL_RPATH "@loader_path/../Frameworks"
                BUILD_WITH_INSTALL_RPATH TRUE
            )

            # Copy the shared library into the bundle's Frameworks/ directory after build
            add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory
                    "$<TARGET_BUNDLE_DIR:${TARGET_NAME}>/Contents/Frameworks"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${ONNXRUNTIME_LIB}"
                    "$<TARGET_BUNDLE_DIR:${TARGET_NAME}>/Contents/Frameworks/"
                COMMAND ${CMAKE_COMMAND} -E create_symlink
                    "libonnxruntime.${ONNXRUNTIME_VERSION}.dylib"
                    "$<TARGET_BUNDLE_DIR:${TARGET_NAME}>/Contents/Frameworks/libonnxruntime.dylib"
                COMMENT "Embedding ONNX Runtime in ${FORMAT} bundle"
            )
        endif()
    endforeach()
endif()

# Test targets — off by default, enable with -DOUARICON_BUILD_TESTS=ON.
# Suite-wide cache option (same name as the other plugins' gates, deliberately).
option(OUARICON_BUILD_TESTS "Build O-Texture test targets" OFF)

if(OUARICON_BUILD_TESTS)
    # Runtime parameter inventory (scripts/param-dump). A regex over
    # createParameterLayout() is not authoritative: IDs can be concatenated and
    # parameters can come from a factory lambda. Only a walk of
    # AudioProcessor::getParameters() on a constructed processor sees the real set.
    include(${CMAKE_SOURCE_DIR}/scripts/param-dump/ParamDump.cmake)
    ouaricon_add_param_dump(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/Source)
endif()
