---
plugin: JUCE
date: 2025-11-06
problem_type: build_error
component: cmake
symptoms:
  - "fatal error: 'JuceHeader.h' file not found"
  - Build fails with include error
  - Occurs in modern JUCE CMake projects
root_cause: config_error
juce_version: 8.0.9
resolution_type: config_change
severity: critical
tags: [cmake, juceheader, include, build, configuration]
---

# Troubleshooting: JuceHeader.h Not Found in CMake Projects

## Problem
Compiler error when building JUCE plugin:
```
fatal error: 'JuceHeader.h' file not found
#include <JuceHeader.h>
         ^~~~~~~~~~~~~~
```

## Environment
- JUCE Version: 8.0.9
- Affected: CMake-based plugin projects
- Date: 2025-11-06

## Symptoms
- Build fails immediately with "JuceHeader.h file not found"
- Occurs in modern JUCE projects using CMake
- Error appears in PluginProcessor.h or PluginEditor.h

## What Didn't Work
**Attempted Solution 1:** Adding manual include paths to CMakeLists.txt
- Why it failed: JuceHeader.h isn't a physical file in JUCE 8.x, it must be generated

**Attempted Solution 2:** Changing to individual module includes (e.g., `#include <juce_audio_processors/juce_audio_processors.h>`)
- Why it failed: While this would work, it's not the standard JUCE approach and requires changing all source files

## Solution
Add `juce_generate_juce_header()` to CMakeLists.txt after `target_link_libraries()`:

```cmake
# Link against JUCE modules
target_link_libraries(SpectralDelayPlugin
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# Generate JuceHeader.h
juce_generate_juce_header(SpectralDelayPlugin)
```

**Commands run:**
```bash
# Reconfigure CMake
cmake -B build -G Xcode

# Rebuild
cmake --build build --target SpectralDelayPlugin_VST3
```

## Why This Works
Modern JUCE (8.x+) with CMake doesn't ship a pre-built JuceHeader.h file. Instead, `juce_generate_juce_header()` is a CMake function that:

1. Scans which JUCE modules are linked to your target via `target_link_libraries()`
2. Auto-generates a JuceHeader.h file including all those modules
3. Places it in the build artifacts directory (e.g., `build/plugins/[PluginName]/SpectralDelayPlugin_artefacts/JuceLibraryCode/JuceHeader.h`)
4. Adds the directory to the compiler's include path automatically

This is the official JUCE CMake workflow - all official JUCE example projects (AudioPerformanceTest, NetworkGraphicsDemo, DemoRunner) use this approach.

## Prevention
When creating new JUCE plugins with CMake, always include these three sections in this order:
1. `juce_add_plugin()` - define the plugin
2. `target_sources()` - add source files
3. `target_link_libraries()` - link JUCE modules
4. **`juce_generate_juce_header()`** - generate JuceHeader.h ← Often forgotten!
5. `target_compile_definitions()` - optional compile flags

## Related Issues
None yet - this is the first CMake project in this workspace.

## References
- JUCE CMake API documentation: `juce_generate_juce_header()` function
- Context7: /juce-framework/juce - "Generate JuceHeader.h with juce_generate_juce_header"
- All official JUCE examples use this pattern
