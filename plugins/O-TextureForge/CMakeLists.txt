cmake_minimum_required(VERSION 3.15)

include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)

# Fetch nanoflann (header-only KD-tree library)
include(FetchContent)
FetchContent_Declare(
    nanoflann
    GIT_REPOSITORY https://github.com/jlblancoc/nanoflann.git
    GIT_TAG 1.10.1
)
set(NANOFLANN_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(NANOFLANN_BUILD_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(nanoflann)

# Fetch umappp (UMAP dimensionality reduction — brings Eigen, knncolle, etc.)
FetchContent_Declare(
    umappp
    GIT_REPOSITORY https://github.com/libscran/umappp.git
    GIT_TAG v3.3.2
)
FetchContent_MakeAvailable(umappp)

juce_add_plugin(OuariconTextureForge
    COMPANY_NAME "${OUARICON_COMPANY_NAME}"
    PLUGIN_MANUFACTURER_CODE ${OUARICON_MANUFACTURER_CODE}
    PLUGIN_CODE OuTF
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "O-TextureForge${OUARICON_DEV_SUFFIX}"
    VERSION 1.1.0
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_WEB_BROWSER TRUE
    NEEDS_WEBVIEW2 TRUE
)

target_sources(OuariconTextureForge
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/dsp/MFCCExtractor.cpp
        Source/dsp/DescriptorExtractor.cpp
        Source/dsp/KDTreeSearch.cpp
        Source/dsp/CorpusLoader.cpp
        Source/dsp/GrainScheduler.cpp
        Source/dsp/PCAProjection.cpp
        Source/dsp/UMAPProjection.cpp
)

target_include_directories(OuariconTextureForge
    PRIVATE
        Source
)

target_link_libraries(OuariconTextureForge
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
        nanoflann::nanoflann
        libscran::umappp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

juce_generate_juce_header(OuariconTextureForge)

# WebView UI Resources
juce_add_binary_data(OuariconTextureForge_UIResources
    SOURCES
        Source/ui/public/index.html
        Source/ui/public/js/app.bundle.js
        # v1.1.0: the label table and the language runtime. EMBEDDED here AND
        # served from getResource() branches in PluginEditor.cpp, in the same
        # commit. A file embedded but not served, or served but not embedded,
        # is a 404 that presents as a page stuck in English and nothing else —
        # check-i18n assertion 8 exists for exactly this pair.
        #
        # i18n_init.js carries an UNDERSCORE, not a hyphen: juce_add_binary_data
        # STRIPS hyphens rather than converting them, so i18n-init.js would be
        # reachable only as the symbol i18ninit_js
        # (critical_binary_data_strips_hyphens). These two embed as i18n_js and
        # i18n_init_js.
        Source/ui/public/js/i18n.js
        Source/ui/public/js/i18n_init.js
        Source/ui/public/js/juce/index.js
        Source/ui/public/js/juce/check_native_interop.js
        Source/ui/public/css/ouaricon-naturalist.css
        Source/ui/public/images/fern.png
)

target_link_libraries(OuariconTextureForge
    PRIVATE
        OuariconTextureForge_UIResources
)

target_compile_definitions(OuariconTextureForge
    PUBLIC
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=1
        JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
        JUCE_USE_CURL=0
)
