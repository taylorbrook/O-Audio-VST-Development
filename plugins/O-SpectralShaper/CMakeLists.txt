cmake_minimum_required(VERSION 3.15)

# Plugin formats: VST3, AU, Standalone
juce_add_plugin(O-SpectralShaper
    VERSION 1.6.2
    COMPANY_NAME "${OUARICON_COMPANY_NAME}"
    PLUGIN_MANUFACTURER_CODE ${OUARICON_MANUFACTURER_CODE}
    PLUGIN_CODE OSpS
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "O-SpectralShaper${OUARICON_DEV_SUFFIX}"
    NEEDS_WEB_BROWSER TRUE
    NEEDS_WEBVIEW2 TRUE
)

# Source files
target_sources(O-SpectralShaper
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/STFTProcessor.cpp
)

# Include paths
target_include_directories(O-SpectralShaper
    PRIVATE
        Source
)

# Required JUCE modules
target_link_libraries(O-SpectralShaper
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

# Generate JuceHeader.h (JUCE 8 requirement - MUST come after target_link_libraries)
juce_generate_juce_header(O-SpectralShaper)

# WebView UI Resources (Stage 3 complete)
juce_add_binary_data(O-SpectralShaper_UIResources
    SOURCES
        Resources/ui/index.html
        Resources/ui/css/styles.css
        Resources/ui/js/juce/index.js
        Resources/ui/js/juce/check_native_interop.js
        Resources/ui/js/app.js
        Resources/ui/js/components/RotaryKnob.js
        Resources/ui/js/components/CurveEditor.js
        Resources/ui/js/components/FreehandCurve.js
        Resources/ui/js/components/NodeCurve.js
        Resources/ui/js/components/Spectrogram.js
        Resources/ui/images/paper-bg.webp
        Resources/ui/images/slug-overlay.webp
        Resources/ui/modules/preset-manager.js
)

target_link_libraries(O-SpectralShaper
    PRIVATE
        O-SpectralShaper_UIResources
)

# Compile definitions
target_compile_definitions(O-SpectralShaper
    PUBLIC
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=1
        JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
        JUCE_USE_CURL=0
)
