# CMake WebView Configuration

## Required Settings

To enable WebView in JUCE plugin:

```cmake
juce_add_plugin(${PROJECT_NAME}
    # ... existing settings ...
    NEEDS_WEB_BROWSER TRUE  # Enable WebView support
)
```

## Bundle HTML Resources

**Option 1: Binary Data (Recommended)**

Embed HTML directly in plugin binary:

```cmake
# Create binary data target
juce_add_binary_data(${PROJECT_NAME}Assets
    SOURCES
        Source/ui/index.html
        Source/ui/style.css  # If using external CSS
        Source/ui/script.js  # If using external JS
)

# Link binary data to plugin
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        juce::juce_audio_plugin_client
        juce::juce_audio_utils
        juce::juce_gui_extra  # WebView support
        ${PROJECT_NAME}Assets  # HTML resources
)
```

**Benefits:**

- Single binary file
- No external dependencies
- Works in sandboxed environments
- Faster load times

**Option 2: External Files**

Load HTML from file system:

```cmake
# No binary data target needed
# HTML loaded from file path at runtime
```

**Benefits:**

- Hot reload during development
- Easier to edit without rebuilding

**Drawbacks:**

- Must distribute HTML files with plugin
- Path resolution issues
- May not work in sandboxed DAWs

**Recommendation:** Use binary data for production, external files for development.

## Complete Example

```cmake
cmake_minimum_required(VERSION 3.22)
project(MyCompressor VERSION 1.0.0)

# JUCE setup
add_subdirectory(JUCE)

# Plugin target
juce_add_plugin(${PROJECT_NAME}
    COMPANY_NAME "YourCompany"
    PLUGIN_MANUFACTURER_CODE "Ycom"
    PLUGIN_CODE "Mcmp"
    FORMATS VST3 AU
    PRODUCT_NAME "My Compressor"
    NEEDS_WEB_BROWSER TRUE  # Enable WebView
)

# Source files
target_sources(${PROJECT_NAME}
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
)

# Binary data (HTML resources)
juce_add_binary_data(${PROJECT_NAME}Assets
    SOURCES
        Source/ui/index.html
)

# Link libraries
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        juce::juce_audio_plugin_client
        juce::juce_audio_utils
        juce::juce_gui_extra  # WebView
        ${PROJECT_NAME}Assets
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
```

## Platform-Specific Notes

### macOS

Uses WebKit (Safari engine):

```cmake
# No additional settings needed
```

### Windows

Uses Chromium (Edge WebView2):

```cmake
# Ensure Edge WebView2 runtime installed on user system
# Or bundle WebView2 fixed runtime
```

### Linux

Uses WebKitGTK:

```cmake
# May need additional dependencies
find_package(PkgConfig REQUIRED)
pkg_check_modules(WEBKIT webkit2gtk-4.0 REQUIRED)

target_include_directories(${PROJECT_NAME}
    PRIVATE ${WEBKIT_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE ${WEBKIT_LIBRARIES}
)
```

## Development vs Production

**Development:**

```cmake
# External files for hot reload
set(HTML_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Source/ui")
target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        HTML_PATH="${HTML_SOURCE_DIR}/index.html"
)
```

**Production:**

```cmake
# Binary data for distribution
juce_add_binary_data(${PROJECT_NAME}Assets
    SOURCES Source/ui/index.html
)
```

## Troubleshooting

### WebView not loading

**Check:**

```cmake
# Verify NEEDS_WEB_BROWSER enabled
NEEDS_WEB_BROWSER TRUE

# Verify juce_gui_extra linked
target_link_libraries(${PROJECT_NAME}
    PRIVATE juce::juce_gui_extra
)
```

### HTML not found

**Check binary data linkage:**

```cmake
target_link_libraries(${PROJECT_NAME}
    PRIVATE ${PROJECT_NAME}Assets  # Must match binary data target name
)
```

### Wrong path in PluginEditor.cpp

**Ensure:**

```cpp
// For binary data:
auto html = String(BinaryData::index_html, BinaryData::index_htmlSize);
webView.goToURL("data:text/html," + html);

// For external file:
webView.goToURL(File(HTML_PATH).getFullPathName());
```
