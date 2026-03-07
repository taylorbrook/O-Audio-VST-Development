# Validation Agent Memory

## Learned Patterns
- General: Build verification differs by platform -- macOS builds VST3 + AU (ninja [PluginName]_VST3 [PluginName]_AU), Windows builds VST3 only (cmake --build). Never expect AU targets on Windows
- General: AU validation on macOS uses auval -a | grep -i [pluginname] to verify component registration -- must clear AU cache first or stale entries persist
- General: Synth plugins require IS_SYNTH TRUE + NEEDS_MIDI_INPUT TRUE in CMakeLists.txt juce_add_plugin() -- omitting either causes DAW to not route MIDI to the plugin (silent output, no error)
- General: WebView plugins require both NEEDS_WEBVIEW2 TRUE (CMake flag for linking) AND JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1 (compile definition) for Windows support -- audit found 34/35 plugins missing this
- General: Cross-platform WebView validation must verify both URL schemes work -- juce://juce.backend/ on macOS, https://juce.backend/ on Windows. Resource provider must handle bare paths, not full URLs
- General: APVTS parameter IDs must match exactly between C++ (addParameterLayout) and JavaScript UI (postMessage parameter names) -- mismatches cause silent parameter disconnection with no error
- General: Plugin cache clearing is a prerequisite for valid testing -- without clearing AU cache on macOS or Ableton cache on Windows, test results may reflect old binaries
- General: JUCE 8 API validation -- check for deprecated patterns: getLatencySamples() override (non-virtual), direct parameter value reads without smoothing, old-style AudioProcessor parameter methods
- General: WebView2 user data folder must be validated -- withUserDataFolder() should point to a temp directory, not the default location which may be access-denied in plugin hosts
- General: ANIRA/ML plugin validation -- verify dylib embedding in Contents/Frameworks/, rpath configuration, versioned symlinks, and install_name_tool fixes for shared library references

## Common Issues
- Parameter validation gap: APVTS parameters defined in C++ but never connected to UI controls -- causes controls to appear functional but have no effect on audio
- Windows-only failures often invisible during macOS development -- always validate WebView2 flags and compile definitions even when testing on macOS
- Research doc staleness: 4 documents referenced deprecated getLatencySamples() override pattern -- validation should flag this in any new code

## Last Updated
2026-03-07 (seed patterns from Phase 21)
