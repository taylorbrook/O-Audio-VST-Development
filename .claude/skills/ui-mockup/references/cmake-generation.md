# CMake Configuration Snippet Generation

## Purpose
Generate v[N]-CMakeLists.txt snippet for adding WebView UI files to plugin build.

## Template Structure

```cmake
# WebView UI Files (v[N])
target_sources([PluginName]
    PRIVATE
        v[N]-ui.html
)

# Copy HTML to build directory at build time
add_custom_command(
    TARGET [PluginName] POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/v[N]-ui.html
        $<TARGET_FILE_DIR:[PluginName]>/v[N]-ui.html
    COMMENT "Copying v[N] WebView UI files to build directory"
)

# Optional: Copy YAML for reference
add_custom_command(
    TARGET [PluginName] POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/v[N]-ui.yaml
        $<TARGET_FILE_DIR:[PluginName]>/v[N]-ui.yaml
    COMMENT "Copying v[N] YAML spec to build directory"
)
```

## Replacement Variables

- `[PluginName]` - Plugin name in PascalCase (e.g., LushVerb)
- `[N]` - Current version number (e.g., 1, 2, 3)

## Integration Instructions

**Add to existing CMakeLists.txt:**
1. Locate the `target_sources` section for the plugin target
2. Add the generated snippet after existing source files
3. Ensure HTML files are copied to the correct build directory

**Multiple versions:**
If multiple UI versions exist (v1, v2, v3), user must choose which version's HTML to copy. Only finalized version should be included in production builds.

## Notes

- YAML file copy is optional (for debugging/reference)
- HTML file MUST be in same directory as plugin binary at runtime
- CMAKE_COMMAND ensures cross-platform compatibility
- POST_BUILD ensures HTML is always fresh after compilation
