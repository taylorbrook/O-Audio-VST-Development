# ============================================================================
# WebView CMake Configuration Snippet
# ============================================================================
# IMPORTANT: This is a SNIPPET to ADD to your plugin's CMakeLists.txt
#
# DO NOT include:
#   - project() declaration (handled by root CMakeLists.txt)
#   - add_subdirectory(JUCE) (JUCE added at root level)
#
# Your plugin CMakeLists.txt should start with:
#   cmake_minimum_required(VERSION 3.15)
#   juce_add_plugin(PluginName ...)
#
# Then ADD this WebView configuration below your existing configuration.
#
# REQUIRED in juce_add_plugin() itself (Windows):
#   juce_add_plugin(PluginName
#       ...
#       NEEDS_WEBVIEW2 TRUE   # links WebView2LoaderStatic + headers on Windows
#   )
# Without it (and the static-linking define below) Windows silently falls back
# to the legacy IE control: blank page / "page not found".
# ============================================================================

# WebView UI Resources
# Embed all UI files into plugin binary
juce_add_binary_data(${PLUGIN_NAME}_UIResources
    SOURCES
        Source/ui/public/index.html
        Source/ui/public/js/juce/index.js
        Source/ui/public/css/style.css
        # Add more resources as needed
)

# Link UI resources to plugin
target_link_libraries(${PLUGIN_NAME}
    PRIVATE
        ${PLUGIN_NAME}_UIResources
        juce::juce_gui_extra  # Required for WebBrowserComponent
)

# Enable WebView on supported platforms
target_compile_definitions(${PLUGIN_NAME}
    PUBLIC
        JUCE_WEB_BROWSER=1
        JUCE_USE_CURL=0
        # Static WebView2 loader: no WebView2Loader.dll to ship, so the DLL can
        # never be missing next to the plugin (blank WebView on Windows).
        JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
)

# Platform-specific WebView configuration
if(APPLE)
    # macOS/iOS use WebKit (built-in, no additional setup)
elseif(WIN32)
    # Windows requires the WebView2 runtime (NEEDS_WEBVIEW2 TRUE above +
    # the static-linking define). The editor probes areOptionsSupported() and
    # paints a native notice when the runtime cannot start.
elseif(UNIX)
    # Linux uses WebKit2GTK (must be installed on system)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(WEBKIT2 REQUIRED webkit2gtk-4.0)
    target_link_libraries(${PLUGIN_NAME} PRIVATE ${WEBKIT2_LIBRARIES})
    target_include_directories(${PLUGIN_NAME} PRIVATE ${WEBKIT2_INCLUDE_DIRS})
endif()

# ============================================================================
# Resource Naming Convention
# ============================================================================
# File paths are converted to BinaryData identifiers:
#   Source/ui/public/index.html      -> BinaryData::index_html
#   Source/ui/public/js/juce/index.js -> BinaryData::juce_index_js
#   Source/ui/public/css/style.css   -> BinaryData::style_css
#
# Rules:
#   - Path separator (/) becomes underscore (_)
#   - Dot (.) becomes underscore (_)
#   - Extension remains as-is in the identifier
#
# Use these identifiers in your PluginEditor.cpp getResource() method
# ============================================================================
