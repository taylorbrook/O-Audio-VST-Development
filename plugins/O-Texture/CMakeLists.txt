cmake_minimum_required(VERSION 3.22)

include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)

# =============================================================================
# ANIRA - Neural network inference for real-time audio
# =============================================================================
include(FetchContent)
FetchContent_Declare(
    anira
    GIT_REPOSITORY https://github.com/anira-project/anira.git
    GIT_TAG v2.0.3
)
set(ANIRA_WITH_ONNXRUNTIME ON CACHE BOOL "" FORCE)
set(ANIRA_WITH_LIBTORCH OFF CACHE BOOL "" FORCE)
set(ANIRA_WITH_TFLITE OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(anira)

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
    VERSION 0.1.0
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    NEEDS_WEB_BROWSER TRUE
    NEEDS_WEBVIEW2 TRUE
)

# ANIRA headers require C++20 (std::binary_semaphore)
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
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        anira::anira
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
juce_add_binary_data(${PROJECT_NAME}_ModelData
    NAMESPACE ModelData
    HEADER_NAME ModelData.h
    SOURCES
        Resources/models/placeholder/decoder.onnx
        Resources/models/placeholder/encoder.onnx
        Resources/models/placeholder/prior.onnx
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
# macOS: Embed ANIRA + ONNX Runtime shared libraries in plugin bundles
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

            # Copy shared libraries into the bundle's Frameworks/ directory after build
            add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory
                    "$<TARGET_BUNDLE_DIR:${TARGET_NAME}>/Contents/Frameworks"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "$<TARGET_FILE:anira>"
                    "$<TARGET_BUNDLE_DIR:${TARGET_NAME}>/Contents/Frameworks/"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${anira_SOURCE_DIR}/modules/onnxruntime-1.19.2-macOS-arm64/lib/libonnxruntime.1.19.2.dylib"
                    "$<TARGET_BUNDLE_DIR:${TARGET_NAME}>/Contents/Frameworks/"
                # Create versioned symlinks
                COMMAND ${CMAKE_COMMAND} -E create_symlink
                    "libonnxruntime.1.19.2.dylib"
                    "$<TARGET_BUNDLE_DIR:${TARGET_NAME}>/Contents/Frameworks/libonnxruntime.dylib"
                COMMAND ${CMAKE_COMMAND} -E create_symlink
                    "$<TARGET_FILE_NAME:anira>"
                    "$<TARGET_BUNDLE_DIR:${TARGET_NAME}>/Contents/Frameworks/libanira.2.dylib"
                COMMAND ${CMAKE_COMMAND} -E create_symlink
                    "$<TARGET_FILE_NAME:anira>"
                    "$<TARGET_BUNDLE_DIR:${TARGET_NAME}>/Contents/Frameworks/libanira.dylib"
                # Fix anira's own reference to onnxruntime
                COMMAND install_name_tool -change
                    "@rpath/libonnxruntime.1.19.2.dylib"
                    "@loader_path/libonnxruntime.1.19.2.dylib"
                    "$<TARGET_BUNDLE_DIR:${TARGET_NAME}>/Contents/Frameworks/$<TARGET_FILE_NAME:anira>"
                COMMENT "Embedding ANIRA + ONNX Runtime in ${FORMAT} bundle"
            )
        endif()
    endforeach()
endif()
